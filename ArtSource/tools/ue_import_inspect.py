"""Isolated Unreal import and inspection for ArtSource static meshes.

Author: Angelis Pseftis.

Runs inside UnrealEditor-Cmd (PythonScriptPlugin) against the isolated preview
project, never against the shared game. Reads a job JSON from the
EBS_IMPORT_JOB environment variable:

{
 "destination": "/Game/Echoes/Production/MER/BLD/EBS_MER_BLD_002",
 "assets": [{"name": "SM_EBS_MER_BLD_002", "lod0": "/abs/.glb", "lod1": "/abs/.glb"}, ...],
 "report": "/abs/import-report.json",
 "expected": {"SM_EBS_MER_BLD_002": {"lod0_triangles": 1118, "lod1_triangles": 838, "height_cm": 1244.0, "sockets": [...]}}
}

Import path: AssetImportTask (synchronous) with an InterchangePipelineStackOverride
carrying one explicitly configured InterchangeGenericAssetsPipeline, as consumed by
AssetTools.cpp (Interchange branch). LOD1 is imported as a separate static mesh and
attached with StaticMeshEditorSubsystem.set_lod_from_static_mesh. Every pipeline
property that cannot be set is reported, never silently skipped. The second import
pass of LOD0 (replace existing) records reimport reproducibility.
"""
import json
import os
import time
import traceback

import unreal

AUTHOR = "Angelis Pseftis"
EXPECTED_SOCKET_NAMES = {}


def log(message):
    unreal.log(f"[EBS_IMPORT] {message}")


def set_prop(obj, name, value, failures, label):
    try:
        obj.set_editor_property(name, value)
        return True
    except Exception as error:  # noqa: BLE001 - report every failure
        failures.append({"object": label, "property": name, "value": str(value), "error": str(error)})
        return False


def make_pipeline(failures):
    pipeline = unreal.InterchangeGenericAssetsPipeline()
    set_prop(pipeline, "use_source_name_for_asset", False, failures, "assets_pipeline")
    common = pipeline.get_editor_property("common_meshes_properties")
    set_prop(common, "force_all_mesh_as_type", unreal.InterchangeForceMeshType.IFMT_STATIC_MESH, failures, "common_meshes")
    set_prop(common, "import_sockets", True, failures, "common_meshes")
    set_prop(common, "import_lods", True, failures, "common_meshes")
    set_prop(common, "bake_meshes", True, failures, "common_meshes")
    set_prop(common, "recompute_normals", False, failures, "common_meshes")
    set_prop(common, "recompute_tangents", True, failures, "common_meshes")
    set_prop(common, "use_full_precision_u_vs", False, failures, "common_meshes")
    mesh = pipeline.get_editor_property("mesh_pipeline")
    set_prop(mesh, "import_static_meshes", True, failures, "mesh_pipeline")
    set_prop(mesh, "import_skeletal_meshes", False, failures, "mesh_pipeline")
    set_prop(mesh, "collision", True, failures, "mesh_pipeline")
    set_prop(mesh, "import_collision_according_to_mesh_name", True, failures, "mesh_pipeline")
    set_prop(mesh, "force_collision_primitive_generation", False, failures, "mesh_pipeline")
    set_prop(mesh, "build_nanite", False, failures, "mesh_pipeline")
    set_prop(mesh, "generate_lightmap_u_vs", False, failures, "mesh_pipeline")
    set_prop(mesh, "combine_static_meshes_behavior", unreal.InterchangeCombineStaticMeshesBehavior.DO_NOT_COMBINE, failures, "mesh_pipeline")
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
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("replace_existing_settings", True)
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


def inspect_mesh(mesh, subsystem):
    bounds = mesh.get_bounds()
    origin, extent = bounds.origin, bounds.box_extent
    info = {
        "path": mesh.get_path_name(),
        "num_lods": mesh.get_num_lods(),
        "lods": [],
        "bounds_cm": {"origin": vec(origin), "extent": vec(extent),
                      "min": [round(origin.x - extent.x, 3), round(origin.y - extent.y, 3), round(origin.z - extent.z, 3)],
                      "max": [round(origin.x + extent.x, 3), round(origin.y + extent.y, 3), round(origin.z + extent.z, 3)]},
        "materials": [],
        "sockets": [],
        "simple_collision_count": subsystem.get_simple_collision_count(mesh),
        "collision_complexity": str(subsystem.get_collision_complexity(mesh)),
        "light_map_coordinate_index": mesh.get_editor_property("light_map_coordinate_index"),
        "light_map_resolution": mesh.get_editor_property("light_map_resolution"),
    }
    try:
        info["nanite_enabled"] = bool(mesh.get_editor_property("nanite_settings").get_editor_property("enabled"))
    except Exception as error:  # noqa: BLE001
        info["nanite_enabled"] = f"unreadable: {error}"
    for lod in range(mesh.get_num_lods()):
        build = subsystem.get_lod_build_settings(mesh, lod)
        entry = {"lod": lod, "triangles": mesh.get_num_triangles(lod), "vertices": subsystem.get_number_verts(mesh, lod),
                 "sections": mesh.get_num_sections(lod), "uv_channels": subsystem.get_num_uv_channels(mesh, lod)}
        for prop in ("generate_lightmap_u_vs", "recompute_normals", "recompute_tangents", "use_full_precision_u_vs", "build_scale3d",
                     "min_lightmap_resolution", "src_lightmap_index", "dst_lightmap_index"):
            try:
                value = build.get_editor_property(prop)
                entry[prop] = vec(value) if isinstance(value, unreal.Vector) else value
            except Exception as error:  # noqa: BLE001
                entry[prop] = f"unreadable: {error}"
        sections = []
        for section in range(mesh.get_num_sections(lod)):
            sections.append({"section": section, "material_slot": subsystem.get_lod_material_slot(mesh, lod, section)})
        entry["section_slots"] = sections
        info["lods"].append(entry)
    for material in mesh.get_editor_property("static_materials"):
        interface = material.get_editor_property("material_interface")
        info["materials"].append({"slot_name": str(material.get_editor_property("material_slot_name")),
                                  "imported_name": str(material.get_editor_property("imported_material_slot_name")),
                                  "material": interface.get_path_name() if interface else None})
    # The Sockets array is not readable from Python; enumerate through the exposed accessors instead.
    sockets = []
    try:
        sockets = list(mesh.get_sockets_by_tag(""))
    except Exception as error:  # noqa: BLE001
        info["socket_enumeration_error"] = str(error)
    seen = set()
    for socket in sockets:
        seen.add(str(socket.get_editor_property("socket_name")))
    for name in EXPECTED_SOCKET_NAMES.get(mesh.get_name(), []):
        for candidate in (name, f"{mesh.get_name()}_{name}", f"SOCKET_{mesh.get_name()}_{name}"):
            if candidate in seen:
                continue
            found = mesh.find_socket(unreal.Name(candidate))
            if found is not None:
                sockets.append(found)
                seen.add(candidate)
    for socket in sockets:
        rotation = socket.get_editor_property("relative_rotation")
        info["sockets"].append({"name": str(socket.get_editor_property("socket_name")),
                                "location_cm": vec(socket.get_editor_property("relative_location")),
                                "rotation_deg": [round(rotation.pitch, 3), round(rotation.yaw, 3), round(rotation.roll, 3)],
                                "scale": vec(socket.get_editor_property("relative_scale")),
                                "tag": str(socket.get_editor_property("tag"))})
    return info


def main():
    job_path = os.environ["EBS_IMPORT_JOB"]
    with open(job_path, "r", encoding="utf-8") as handle:
        job = json.load(handle)
    report = {"author": AUTHOR, "creator": AUTHOR, "job": job_path, "engine": str(unreal.SystemLibrary.get_engine_version()),
              "project": unreal.Paths.get_project_file_path(), "pipeline_property_failures": [], "assets": [], "errors": []}
    failures = report["pipeline_property_failures"]
    for asset_name, expected in job.get("expected", {}).items():
        EXPECTED_SOCKET_NAMES[asset_name] = list(expected.get("sockets", []))
    pipeline = make_pipeline(failures)
    subsystem = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    destination = job["destination"]
    lod_source_folder = destination.rstrip("/") + "/LODSource"
    # Clean import every run: the sandbox holds nothing else, and stale slots from an earlier
    # experiment must not survive a replace-import into the record.
    if unreal.EditorAssetLibrary.does_directory_exist(destination):
        report["destination_cleared"] = bool(unreal.EditorAssetLibrary.delete_directory(destination))
    for asset in job["assets"]:
        entry = {"name": asset["name"], "lod0_file": asset["lod0"], "lod1_file": asset.get("lod1")}
        try:
            paths, elapsed = import_glb(asset["lod0"], destination, asset["name"], pipeline)
            entry["imported_object_paths"] = paths
            entry["import_seconds"] = round(elapsed, 2)
            mesh_path = f"{destination}/{asset['name']}.{asset['name']}"
            mesh = unreal.EditorAssetLibrary.load_asset(mesh_path)
            if mesh is None:
                for candidate in paths:
                    loaded = unreal.EditorAssetLibrary.load_asset(candidate)
                    if isinstance(loaded, unreal.StaticMesh):
                        mesh = loaded
                        mesh_path = candidate
                        break
            if mesh is None:
                raise RuntimeError(f"no StaticMesh produced for {asset['name']}: {paths}")
            entry["after_lod0_import"] = inspect_mesh(mesh, subsystem)
            if asset.get("lod1"):
                # Path A: Interchange custom-LOD import through the static mesh editor subsystem.
                started = time.time()
                try:
                    import_lod_result = subsystem.import_lod(mesh, 1, asset["lod1"])
                except Exception as error:  # noqa: BLE001
                    import_lod_result = f"exception: {error}"
                entry["lod1_import_lod_result"] = import_lod_result
                entry["lod1_import_lod_seconds"] = round(time.time() - started, 2)
                if isinstance(import_lod_result, int) and import_lod_result >= 0 and mesh.get_num_lods() >= 2:
                    # The custom-LOD path maps every section to slot 0; restore the authored slot per section
                    # from the job's primitive-order slot names.
                    dest_names = [str(m.get_editor_property("material_slot_name")) for m in mesh.get_editor_property("static_materials")]
                    names = asset.get("lod1_section_slot_names", [])
                    remap = []
                    for section in range(min(mesh.get_num_sections(1), len(names))):
                        if names[section] in dest_names:
                            subsystem.set_lod_material_slot(mesh, dest_names.index(names[section]), 1, section)
                            remap.append({"section": section, "slot_name": names[section], "dest_slot": dest_names.index(names[section])})
                    entry["lod1_slot_remap"] = remap
                if not (isinstance(import_lod_result, int) and import_lod_result >= 0) or mesh.get_num_lods() < 2:
                    # Path B: import LOD1 as its own asset, attach it, then re-map sections to the slots by name.
                    lod_paths, lod_elapsed = import_glb(asset["lod1"], lod_source_folder, asset["name"] + "_LOD1Source", pipeline)
                    lod_mesh = None
                    for candidate in lod_paths:
                        loaded = unreal.EditorAssetLibrary.load_asset(candidate)
                        if isinstance(loaded, unreal.StaticMesh):
                            lod_mesh = loaded
                            break
                    if lod_mesh is None:
                        raise RuntimeError(f"no StaticMesh produced for LOD1 of {asset['name']}: {lod_paths}")
                    entry["lod1_source_inspection"] = inspect_mesh(lod_mesh, subsystem)
                    result = subsystem.set_lod_from_static_mesh(mesh, 1, lod_mesh, 0, True)
                    entry["lod1_attach_result"] = result
                    entry["lod1_source_import_seconds"] = round(lod_elapsed, 2)
                    # Slot fix-up: match the LOD1 source's section slot names to the destination slot names.
                    dest_names = [str(m.get_editor_property("material_slot_name")) for m in mesh.get_editor_property("static_materials")]
                    src_names = [str(m.get_editor_property("material_slot_name")) for m in lod_mesh.get_editor_property("static_materials")]
                    remap = []
                    for section in range(lod_mesh.get_num_sections(0)):
                        src_slot = subsystem.get_lod_material_slot(lod_mesh, 0, section)
                        name = src_names[src_slot] if 0 <= src_slot < len(src_names) else None
                        if name in dest_names:
                            subsystem.set_lod_material_slot(mesh, dest_names.index(name), 1, section)
                            remap.append({"section": section, "slot_name": name, "dest_slot": dest_names.index(name)})
                    entry["lod1_slot_remap"] = remap
                unreal.EditorAssetLibrary.save_asset(mesh.get_path_name())
            entry["after_lod1_attach"] = inspect_mesh(mesh, subsystem)
            # Reimport reproducibility: replace LOD0 in place with the same source and settings.
            paths2, elapsed2 = import_glb(asset["lod0"], destination, asset["name"], pipeline)
            mesh2 = unreal.EditorAssetLibrary.load_asset(mesh_path)
            entry["reimport_replace"] = {"imported_object_paths": paths2, "seconds": round(elapsed2, 2),
                                         "inspection": inspect_mesh(mesh2, subsystem) if mesh2 else None}
            expected = job.get("expected", {}).get(asset["name"])
            if expected and mesh2:
                after = entry["reimport_replace"]["inspection"]
                checks = {
                    "lod0_triangles_match": after["lods"][0]["triangles"] == expected.get("lod0_triangles"),
                    "height_cm_match": abs(after["bounds_cm"]["max"][2] - expected.get("height_cm", 0)) < 0.5,
                    "sockets_present": sorted(s["name"] for s in after["sockets"]),
                    "sockets_expected": sorted(expected.get("sockets", [])),
                    "simple_collision_expected": expected.get("collision_boxes"),
                    "simple_collision_count": after["simple_collision_count"],
                    "nanite_off": after.get("nanite_enabled") is False,
                }
                checks["sockets_match"] = checks["sockets_present"] == checks["sockets_expected"]
                if len(after["lods"]) > 1:
                    checks["lod1_triangles"] = after["lods"][1]["triangles"]
                    checks["lod1_triangles_match"] = after["lods"][1]["triangles"] == expected.get("lod1_triangles")
                    checks["lod1_section_slots"] = [sec["material_slot"] for sec in after["lods"][1]["section_slots"]]
                rot_expected = expected.get("socket_rotations", {})
                rot_check = {}
                for sock in after["sockets"]:
                    if sock["name"] in rot_expected:
                        want = rot_expected[sock["name"]]
                        rot_check[sock["name"]] = {"got": sock["rotation_deg"], "want": want,
                                                  "ok": all(abs(((g - w + 180) % 360) - 180) < 0.5 for g, w in zip(sock["rotation_deg"], want))}
                checks["socket_rotations"] = rot_check
                entry["checks"] = checks
        except Exception as error:  # noqa: BLE001
            entry["error"] = f"{error}\n{traceback.format_exc()}"
            report["errors"].append(asset["name"])
        report["assets"].append(entry)
    try:
        unreal.EditorAssetLibrary.save_directory(destination, only_if_is_dirty=False, recursive=True)
    except Exception as error:  # noqa: BLE001
        report["errors"].append(f"save_directory: {error}")
    with open(job["report"], "w", encoding="utf-8") as handle:
        json.dump(report, handle, indent=1, sort_keys=True)
    log(f"report written to {job['report']} errors={len(report['errors'])} property_failures={len(failures)}")


main()
