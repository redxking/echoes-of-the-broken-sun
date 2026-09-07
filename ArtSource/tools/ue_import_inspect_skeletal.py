"""Isolated Unreal import and inspection for ArtSource skinned (skeletal) glTF meshes.

Author: Angelis Pseftis.

Runs inside UnrealEditor-Cmd (PythonScriptPlugin) against the isolated preview
project, never against the shared game. Reads a job JSON from EBS_IMPORT_JOB:

{
 "destination": "/Game/Echoes/SkeletalProbe",
 "assets": [{"name": "SK_...", "file": "/abs/.glb"}],
 "report": "/abs/probe-report.json",
 "expected": {...}   # passed through to the report untouched
}

Import path: AssetImportTask + InterchangePipelineStackOverride with one explicitly
configured InterchangeGenericAssetsPipeline (skeletal mesh + animation, no materials,
no physics asset). Every pipeline property that cannot be set is reported.

Idempotency (the run-3 lesson): if the destination package still exists when the import
starts, UInterchangeManager silently converts the import into a re-import of the existing
SkeletalMesh and creates no AnimSequences. So every run imports into a fresh, unique
sub-folder ``<destination>/Run_<UTC stamp>`` with replace_existing False, after a
best-effort delete_directory of ``<destination>`` (recorded as destination_cleared). The
run is FATAL - the report is written with report["errors"] and the script raises, which
UnrealEditor-Cmd turns into a non-zero exit when launched with -ScriptErrorsAreFatal - when
the base folder cannot be cleared (verified on disk and in the asset registry - stale
package files that the engine's force-delete leaves behind are removed by hand and listed),
when the unique folder already holds assets, when the import produced anything other than
skeleton + skeletal mesh + one AnimSequence per expected clip, or when an inspection
step throws. skeletal_probe.py check re-validates these fields and the editor log.

After import the report lists skeletal meshes (bones in index order with parents and
reference-pose transforms, sockets, LODs, material slots), skeletons, and every
AnimSequence with its bone transforms evaluated at t=0 and t=end
(AnimationLibrary.get_bone_pose_for_time local pose, plus AnimPoseExtensions
component-space pose) so the encoding can be read back numerically.
"""
import datetime
import json
import os
import re
import shutil
import time
import traceback

import unreal

AUTHOR = "Angelis Pseftis"


def log(message):
    unreal.log(f"[EBS_SKEL_IMPORT] {message}")


def set_prop(obj, name, value, failures, label):
    try:
        obj.set_editor_property(name, value)
        return True
    except Exception as error:  # noqa: BLE001 - report every failure
        failures.append({"object": label, "property": name, "value": str(value), "error": str(error)})
        return False


def set_first(obj, names, value, failures, label):
    """Try alternative Python property names (reflection naming of e.g. bUse30Hz... is not obvious)."""
    scratch = []
    for name in names:
        if set_prop(obj, name, value, scratch, label):
            return name
    failures.append({"object": label, "property": "|".join(names), "value": str(value), "error": "; ".join(f["error"] for f in scratch)})
    return None


def make_pipeline(failures):
    pipeline = unreal.InterchangeGenericAssetsPipeline()
    set_prop(pipeline, "use_source_name_for_asset", False, failures, "assets_pipeline")
    common = pipeline.get_editor_property("common_meshes_properties")
    set_prop(common, "force_all_mesh_as_type", unreal.InterchangeForceMeshType.IFMT_SKELETAL_MESH, failures, "common_meshes")
    set_prop(common, "import_sockets", True, failures, "common_meshes")
    set_prop(common, "import_lods", True, failures, "common_meshes")
    set_prop(common, "bake_meshes", True, failures, "common_meshes")
    set_prop(common, "recompute_normals", False, failures, "common_meshes")
    set_prop(common, "recompute_tangents", True, failures, "common_meshes")
    set_prop(common, "use_full_precision_u_vs", False, failures, "common_meshes")
    common_skel = pipeline.get_editor_property("common_skeletal_meshes_and_animations_properties")
    set_prop(common_skel, "import_only_animations", False, failures, "common_skeletal")
    set_prop(common_skel, "try_auto_select_skeleton", False, failures, "common_skeletal")
    set_prop(common_skel, "use_t0_as_ref_pose", False, failures, "common_skeletal")
    mesh = pipeline.get_editor_property("mesh_pipeline")
    set_prop(mesh, "import_static_meshes", False, failures, "mesh_pipeline")
    set_prop(mesh, "import_skeletal_meshes", True, failures, "mesh_pipeline")
    set_prop(mesh, "create_physics_asset", False, failures, "mesh_pipeline")
    set_prop(mesh, "import_morph_targets", False, failures, "mesh_pipeline")
    set_prop(mesh, "collision", False, failures, "mesh_pipeline")
    set_prop(mesh, "build_nanite", False, failures, "mesh_pipeline")
    set_prop(mesh, "skeletal_mesh_import_content_type", unreal.InterchangeSkeletalMeshContentType.ALL, failures, "mesh_pipeline")
    animation = pipeline.get_editor_property("animation_pipeline")
    set_prop(animation, "import_animations", True, failures, "animation_pipeline")
    set_prop(animation, "import_bone_tracks", True, failures, "animation_pipeline")
    set_prop(animation, "animation_range", unreal.InterchangeAnimationRange.TIMELINE, failures, "animation_pipeline")
    set_first(animation, ["use30_hz_to_bake_bone_animation", "use_30hz_to_bake_bone_animation", "use30hz_to_bake_bone_animation"], False, failures, "animation_pipeline")
    set_prop(animation, "custom_bone_animation_sample_rate", 30, failures, "animation_pipeline")
    set_prop(animation, "import_custom_attribute", False, failures, "animation_pipeline")
    material = pipeline.get_editor_property("material_pipeline")
    set_prop(material, "import_materials", False, failures, "material_pipeline")
    return pipeline


def import_glb(path, destination, name, pipeline):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", path)
    task.set_editor_property("destination_path", destination)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    # Never replace: the destination is a fresh per-run folder, and a replace-import of an
    # existing package is exactly the silent re-import that drops the animation clips.
    task.set_editor_property("replace_existing", False)
    task.set_editor_property("replace_existing_settings", False)
    try:
        task.set_editor_property("async_", False)
    except Exception:  # noqa: BLE001
        pass
    override = unreal.InterchangePipelineStackOverride()
    override.add_pipeline(pipeline)
    task.set_editor_property("options", override)
    started = time.time()
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    elapsed = time.time() - started
    paths = [str(p) for p in task.get_editor_property("imported_object_paths")]
    return paths, elapsed


def vec(v):
    return [round(v.x, 4), round(v.y, 4), round(v.z, 4)]


def rot(r):
    return [round(r.pitch, 4), round(r.yaw, 4), round(r.roll, 4)]


def quat(q):
    return [round(q.x, 6), round(q.y, 6), round(q.z, 6), round(q.w, 6)]


def transform_dict(t):
    return {"translation_cm": vec(t.translation), "rotation_deg": rot(t.rotation.rotator()), "quaternion": quat(t.rotation), "scale": vec(t.scale3d)}


def try_call(entry, key, func):
    try:
        entry[key] = func()
        return entry[key]
    except Exception as error:  # noqa: BLE001
        entry[key + "_error"] = f"{error}"
        return None


def socket_dict(socket):
    rotation = socket.get_editor_property("relative_rotation")
    return {"name": str(socket.get_editor_property("socket_name")), "bone": str(socket.get_editor_property("bone_name")),
            "location_cm": vec(socket.get_editor_property("relative_location")), "rotation_deg": rot(rotation),
            "scale": vec(socket.get_editor_property("relative_scale"))}


def inspect_bones(mesh, skeleton, entry):
    bones = []
    pose = None
    try:
        pose = unreal.AnimPoseExtensions.get_reference_pose(skeleton)
        names = [str(n) for n in unreal.AnimPoseExtensions.get_bone_names(pose)]
        entry["bone_enumeration"] = "AnimPoseExtensions.get_reference_pose/get_bone_names"
    except Exception as error:  # noqa: BLE001
        entry["bone_enumeration_error"] = f"{error}"
        names = []
    if not names:
        # Fall back to the editor subsystem child walk from the root bone name (unknown): walk children of every socket-less bone
        try:
            root = None
            for candidate in ("root", "Root", "pelvis"):
                if unreal.SkeletalMeshEditorSubsystem.get_bone_children(mesh, candidate):
                    root = candidate
                    break
            if root:
                stack = [root]
                while stack:
                    b = stack.pop(0)
                    names.append(b)
                    stack = [str(c) for c in unreal.SkeletalMeshEditorSubsystem.get_bone_children(mesh, b)] + stack
                entry["bone_enumeration"] = "SkeletalMeshEditorSubsystem.get_bone_children walk"
        except Exception as error:  # noqa: BLE001
            entry["bone_enumeration_fallback_error"] = f"{error}"
    for index, name in enumerate(names):
        bone = {"index": index, "name": name}
        try:
            parent = unreal.SkeletalMeshEditorSubsystem.get_bone_parent(mesh, name)
            bone["parent"] = None if str(parent) in ("None", "") else str(parent)
        except Exception as error:  # noqa: BLE001
            bone["parent_error"] = f"{error}"
        if pose is not None:
            try:
                local = unreal.AnimPoseExtensions.get_ref_bone_pose(pose, name, unreal.AnimPoseSpaces.LOCAL)
                world = unreal.AnimPoseExtensions.get_ref_bone_pose(pose, name, unreal.AnimPoseSpaces.WORLD)
                bone["ref_local"] = transform_dict(local)
                bone["ref_component"] = transform_dict(world)
                bone["ref_component_location_cm"] = vec(world.translation)
                bone["ref_component_rotation_deg"] = rot(world.rotation.rotator())
            except Exception as error:  # noqa: BLE001
                bone["ref_pose_error"] = f"{error}"
        bones.append(bone)
    entry["bones"] = bones
    entry["bone_count"] = len(bones)


def inspect_skeletal_mesh(mesh):
    entry = {"path": mesh.get_path_name(), "name": mesh.get_name()}
    skeleton = try_call(entry, "skeleton_object", lambda: mesh.get_editor_property("skeleton"))
    entry["skeleton_path"] = skeleton.get_path_name() if skeleton else None
    entry.pop("skeleton_object", None)
    bounds = mesh.get_bounds()
    entry["bounds_cm"] = {"origin": vec(bounds.origin), "extent": vec(bounds.box_extent)}
    try_call(entry, "imported_bounds_cm", lambda: {"origin": vec(mesh.get_imported_bounds().origin), "extent": vec(mesh.get_imported_bounds().box_extent)})
    lod_count = try_call(entry, "num_lods", lambda: unreal.SkeletalMeshEditorSubsystem.get_lod_count(mesh)) or 0
    subsystem = unreal.get_editor_subsystem(unreal.SkeletalMeshEditorSubsystem)
    lods = []
    for lod in range(lod_count):
        item = {"lod": lod}
        try_call(item, "vertices", lambda lod=lod: subsystem.get_num_verts(mesh, lod))
        sections = try_call(item, "sections", lambda lod=lod: subsystem.get_num_sections(mesh, lod)) or 0
        item["section_slots"] = []
        for section in range(sections):
            item["section_slots"].append(try_call({}, "slot", lambda lod=lod, section=section: subsystem.get_lod_material_slot(mesh, lod, section)))
        # No Python-exposed triangle count exists for USkeletalMesh in 5.8; record what the API offers.
        item["triangles"] = None
        item["triangles_note"] = "USkeletalMesh exposes no triangle count to Python; see vertices/sections"
        lods.append(item)
    entry["lods"] = lods
    materials = []
    try:
        for material in mesh.get_editor_property("materials"):
            interface = material.get_editor_property("material_interface")
            materials.append({"slot_name": str(material.get_editor_property("material_slot_name")),
                              "imported_name": str(material.get_editor_property("imported_material_slot_name")),
                              "material": interface.get_path_name() if interface else None})
    except Exception as error:  # noqa: BLE001
        entry["materials_error"] = f"{error}"
    entry["materials"] = materials
    sockets = []
    try:
        for index in range(mesh.num_sockets()):
            sockets.append(socket_dict(mesh.get_socket_by_index(index)))
    except Exception as error:  # noqa: BLE001
        entry["sockets_error"] = f"{error}"
    entry["sockets"] = sockets
    if skeleton:
        inspect_bones(mesh, skeleton, entry)
        skel_sockets = []
        try:
            for socket in skeleton.get_editor_property("sockets"):
                skel_sockets.append(socket_dict(socket))
        except Exception as error:  # noqa: BLE001
            entry["skeleton_sockets_error"] = f"{error}"
        entry["skeleton_sockets"] = skel_sockets
    return entry


def evaluate_pose(sequence, bone_names, t, mesh, ref_local):
    out = {}
    options = unreal.AnimPoseEvaluationOptions()
    try:
        options.set_editor_property("evaluation_type", unreal.AnimDataEvalType.RAW)
    except Exception:  # noqa: BLE001
        pass
    try:
        options.set_editor_property("optional_skeletal_mesh", mesh)
    except Exception:  # noqa: BLE001
        pass
    pose = None
    pose_error = None
    try:
        pose = unreal.AnimPoseExtensions.get_anim_pose_at_time(sequence, float(t), options)
    except Exception as error:  # noqa: BLE001
        pose_error = f"{error}"
    for name in bone_names:
        item = {}
        try:
            local = unreal.AnimationLibrary.get_bone_pose_for_time(sequence, name, float(t), False)
            item["local"] = transform_dict(local)
            item["local_rotation_deg"] = rot(local.rotation.rotator())
            item["local_translation_cm"] = vec(local.translation)
            if name in ref_local:
                item["local_translation_delta_cm"] = [round(a - b, 4) for a, b in zip(vec(local.translation), ref_local[name])]
        except Exception as error:  # noqa: BLE001
            item["local_error"] = f"{error}"
        if pose is not None:
            try:
                item["component"] = transform_dict(unreal.AnimPoseExtensions.get_bone_pose(pose, name, unreal.AnimPoseSpaces.WORLD))
                item["pose_local"] = transform_dict(unreal.AnimPoseExtensions.get_bone_pose(pose, name, unreal.AnimPoseSpaces.LOCAL))
            except Exception as error:  # noqa: BLE001
                item["component_error"] = f"{error}"
        elif pose_error:
            item["component_error"] = pose_error
        out[name] = item
    return out


def inspect_animation(sequence, mesh, bone_names, ref_local):
    entry = {"path": sequence.get_path_name(), "name": sequence.get_name()}
    try_call(entry, "num_frames", lambda: unreal.AnimationLibrary.get_num_frames(sequence))
    try_call(entry, "num_keys", lambda: unreal.AnimationLibrary.get_num_keys(sequence))
    length = try_call(entry, "length_s", lambda: unreal.AnimationLibrary.get_sequence_length(sequence))
    try_call(entry, "rate_scale", lambda: unreal.AnimationLibrary.get_rate_scale(sequence))
    try_call(entry, "track_names", lambda: [str(n) for n in unreal.AnimationLibrary.get_animation_track_names(sequence)])
    try_call(entry, "skeleton_path", lambda: sequence.get_editor_property("skeleton").get_path_name())
    entry["start"] = evaluate_pose(sequence, bone_names, 0.0, mesh, ref_local)
    if length is not None:
        entry["end"] = evaluate_pose(sequence, bone_names, float(length), mesh, ref_local)
        entry["mid"] = evaluate_pose(sequence, bone_names, float(length) / 2.0, mesh, ref_local)
    return entry


class FatalInspection(RuntimeError):
    """Raised after the report is written so the editor exits with an error."""


def command_line_log_path():
    """The -abslog=<path> of this editor process (quoted or bare), or None."""
    try:
        line = str(unreal.SystemLibrary.get_command_line())
    except Exception:  # noqa: BLE001
        return None, None
    match = re.search(r'-abslog=(?:"([^"]+)"|(\S+))', line, flags=re.IGNORECASE)
    return (match.group(1) or match.group(2)) if match else None, line


def expected_clip_names(job, asset):
    if asset.get("clips") is not None:
        return list(asset["clips"])
    return list(job.get("expected", {}).get(asset["name"], {}).get("clips", {}).keys())


ASSET_FILE_SUFFIXES = (".uasset", ".uexp", ".ubulk", ".uptnl", ".umap")


def project_content_dir():
    """Absolute project Content directory (KismetSystemLibrary first, BlueprintPathsLibrary fallback)."""
    try:
        return os.path.normpath(str(unreal.SystemLibrary.get_project_content_directory()))
    except Exception:  # noqa: BLE001
        content = str(unreal.Paths.project_content_dir())
        try:
            content = str(unreal.Paths.convert_relative_path_to_full(content))
        except Exception:  # noqa: BLE001
            pass
        return os.path.normpath(content)


def disk_path_for(long_package_path):
    if not long_package_path.startswith("/Game/"):
        raise ValueError(f"only /Game/ paths are supported: {long_package_path}")
    return os.path.normpath(os.path.join(project_content_dir(), long_package_path[len("/Game/"):]))


def disk_files_under(path):
    files = []
    for root, _dirs, names in os.walk(path):
        for name in names:
            files.append(os.path.relpath(os.path.join(root, name), path))
    return sorted(files)


def prepare_destination(job, report):
    """Clear the base destination for real, then create a fresh unique run folder that must be empty.

    EditorAssetLibrary.delete_directory force-deletes the registry's assets but (runs 3 and 4 in the
    evidence) can leave the package files on disk and then reports False; a surviving package is what
    turns the next import into a silent re-import. So the clear is verified on disk and, when only
    stale package files (.uasset/.uexp/...) remain inside the sandbox Content folder, they are removed
    here and listed in the report. ``destination_cleared`` is True only when the folder is gone from
    disk and the registry lists nothing under it."""
    base = job["destination"].rstrip("/")
    report["destination"] = base
    disk_base = disk_path_for(base)
    report["destination_disk_path"] = disk_base
    report["destination_removed_files"] = []
    engine_result = None
    if unreal.EditorAssetLibrary.does_directory_exist(base):
        engine_result = bool(unreal.EditorAssetLibrary.delete_directory(base))
        if not engine_result:
            report["warnings"].append(f"delete_directory({base}) returned False")
    report["destination_engine_delete"] = engine_result
    leftovers = disk_files_under(disk_base) if os.path.isdir(disk_base) else []
    report["destination_disk_files_after_engine_delete"] = leftovers
    if leftovers:
        foreign = [f for f in leftovers if not f.lower().endswith(ASSET_FILE_SUFFIXES)]
        inside_sandbox = os.path.commonpath([disk_base, project_content_dir()]) == project_content_dir()
        if foreign or not inside_sandbox:
            report["errors"].append(f"cannot clear {disk_base}: unexpected files {foreign or leftovers} (inside_sandbox={inside_sandbox})")
        else:
            shutil.rmtree(disk_base)
            report["destination_removed_files"] = leftovers
            report["warnings"].append(f"removed {len(leftovers)} stale package file(s) left on disk under {disk_base}: {leftovers}")
    try:
        unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous([base], True)
    except Exception as error:  # noqa: BLE001
        report["warnings"].append(f"scan_paths_synchronous({base}): {error}")
    try:
        registry_assets = sorted(str(p) for p in unreal.EditorAssetLibrary.list_assets(base, recursive=True, include_folder=False))
    except Exception as error:  # noqa: BLE001
        registry_assets = [f"<list_assets error: {error}>"]
    report["destination_registry_assets_after_clear"] = registry_assets
    report["destination_disk_files_after_clear"] = disk_files_under(disk_base) if os.path.isdir(disk_base) else []
    cleared = not report["destination_disk_files_after_clear"] and not registry_assets
    report["destination_cleared"] = cleared
    if not cleared:
        report["errors"].append(f"destination {base} not cleared: registry={registry_assets} disk={report['destination_disk_files_after_clear']}")
        return None
    stamp = datetime.datetime.now(datetime.timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    used = f"{base}/Run_{stamp}"
    report["destination_used"] = used
    report["destination_unique_per_run"] = True
    preexisting = []
    try:
        if unreal.EditorAssetLibrary.does_directory_exist(used) or os.path.isdir(disk_path_for(used)):
            preexisting = sorted(str(p) for p in unreal.EditorAssetLibrary.list_assets(used, recursive=True, include_folder=False))
            preexisting += disk_files_under(disk_path_for(used))
            if not preexisting:
                preexisting = [f"<directory exists: {used}>"]
    except Exception as error:  # noqa: BLE001
        preexisting = [f"<list_assets error: {error}>"]
    report["destination_preexisting_assets"] = preexisting
    if preexisting:
        report["errors"].append(f"destination {used} is not empty before import: {preexisting}")
        return None
    return used


def main():
    job_path = os.environ["EBS_IMPORT_JOB"]
    with open(job_path, "r", encoding="utf-8") as handle:
        job = json.load(handle)
    log_path, command_line = command_line_log_path()
    report = {"author": AUTHOR, "creator": AUTHOR, "job": job_path, "engine": str(unreal.SystemLibrary.get_engine_version()),
              "project": unreal.Paths.get_project_file_path(), "log_path": log_path, "command_line": command_line,
              "pipeline_property_failures": [], "imports": [], "skeletal_meshes": [], "skeletons": [], "animations": [],
              "other_assets": [], "errors": [], "warnings": [], "expected": job.get("expected", {})}
    failures = report["pipeline_property_failures"]
    destination = prepare_destination(job, report)
    meshes, sequences = [], []
    if destination is not None:
        pipeline = make_pipeline(failures)
        for asset in job["assets"]:
            clips = expected_clip_names(job, asset)
            entry = {"name": asset["name"], "file": asset["file"], "expected_clips": clips}
            try:
                paths, elapsed = import_glb(asset["file"], destination, asset["name"], pipeline)
                entry["imported_object_paths"] = paths
                entry["import_seconds"] = round(elapsed, 2)
                entry["expected_imported_object_count"] = 2 + len(clips)
                if len(paths) != 2 + len(clips):
                    report["errors"].append(f"{asset['name']}: imported {len(paths)} objects, expected {2 + len(clips)} "
                                            f"(skeleton + skeletal mesh + {len(clips)} clips): {paths}")
            except Exception as error:  # noqa: BLE001
                entry["error"] = f"{error}\n{traceback.format_exc()}"
                report["errors"].append(asset["name"])
            report["imports"].append(entry)
        try:
            asset_paths = sorted(str(p) for p in unreal.EditorAssetLibrary.list_assets(destination, recursive=True, include_folder=False))
        except Exception as error:  # noqa: BLE001
            asset_paths = []
            report["errors"].append(f"list_assets: {error}")
        report["asset_paths"] = asset_paths
        for path in asset_paths:
            try:
                obj = unreal.EditorAssetLibrary.load_asset(path)
            except Exception as error:  # noqa: BLE001
                report["errors"].append(f"load {path}: {error}")
                continue
            if isinstance(obj, unreal.SkeletalMesh):
                meshes.append(obj)
            elif isinstance(obj, unreal.AnimSequence):
                sequences.append(obj)
            elif isinstance(obj, unreal.Skeleton):
                report["skeletons"].append({"path": obj.get_path_name(), "name": obj.get_name()})
            else:
                report["other_assets"].append({"path": path, "class": obj.get_class().get_name() if obj else None})
        expected_clip_total = sum(len(expected_clip_names(job, asset)) for asset in job["assets"])
        if len(sequences) != expected_clip_total:
            report["errors"].append(f"{len(sequences)} AnimSequence assets in {destination}, expected {expected_clip_total}")
        if len(meshes) != len(job["assets"]):
            report["errors"].append(f"{len(meshes)} SkeletalMesh assets in {destination}, expected {len(job['assets'])}")
        # PF-011 regression check: collision geometry left in a skinned GLB arrives as an EXTRA
        # SkeletalMesh with its own Skeleton. Name the cause rather than only the count.
        collision_meshes = sorted(m.get_name() for m in meshes
                                  if m.get_name().startswith(("UBX_", "UCX_", "USP_", "UCP_")))
        if collision_meshes:
            report["errors"].append(
                f"collision meshes imported as SkeletalMesh assets: {collision_meshes}. Skeletal packages must "
                f"export with include_collision=False; collision comes from a physics asset, not UBX geometry")
        if len(report["skeletons"]) != len(job["assets"]):
            report["errors"].append(
                f"{len(report['skeletons'])} Skeleton assets in {destination}, expected {len(job['assets'])}: "
                f"{sorted(sk['name'] for sk in report['skeletons'])}")
        report["collision_meshes_rejected"] = collision_meshes
    bone_names, ref_local, first_mesh = [], {}, None
    for mesh in meshes:
        try:
            entry = inspect_skeletal_mesh(mesh)
        except Exception as error:  # noqa: BLE001
            entry = {"path": mesh.get_path_name(), "error": f"{error}\n{traceback.format_exc()}"}
            report["errors"].append(mesh.get_path_name())
        report["skeletal_meshes"].append(entry)
        if first_mesh is None:
            first_mesh = mesh
            bone_names = [b["name"] for b in entry.get("bones", [])]
            ref_local = {b["name"]: b["ref_local"]["translation_cm"] for b in entry.get("bones", []) if "ref_local" in b}
    for sequence in sequences:
        try:
            report["animations"].append(inspect_animation(sequence, first_mesh, bone_names, ref_local))
        except Exception as error:  # noqa: BLE001
            report["animations"].append({"path": sequence.get_path_name(), "error": f"{error}\n{traceback.format_exc()}"})
            report["errors"].append(sequence.get_path_name())
    if destination is not None:
        try:
            unreal.EditorAssetLibrary.save_directory(destination, only_if_is_dirty=False, recursive=True)
        except Exception as error:  # noqa: BLE001
            report["errors"].append(f"save_directory: {error}")
    report["fatal"] = bool(report["errors"])
    with open(job["report"], "w", encoding="utf-8") as handle:
        json.dump(report, handle, indent=1, sort_keys=True)
    log(f"report written to {job['report']} destination={report.get('destination_used')} errors={len(report['errors'])} "
        f"property_failures={len(failures)} meshes={len(meshes)} sequences={len(sequences)}")
    if report["errors"]:
        for error in report["errors"]:
            unreal.log_error(f"[EBS_SKEL_IMPORT] {error}")
        raise FatalInspection(f"skeletal inspection failed: {report['errors']}")


main()
