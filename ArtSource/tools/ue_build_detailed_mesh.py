"""ue_build_detailed_mesh.py — write a detailed SkeletalMesh from a blockout, in Unreal.

Author: Angelis Pseftis. Runs inside Unreal.

weld -> remove hidden triangles -> polygroups -> bevel -> (optional displace) -> transfer bone
weights and vertex colours from the shipped mesh -> recompute normals -> new SkeletalMesh asset.

Removing hidden triangles matters: the blockouts are assembled from INTERPENETRATING solids, so faces
buried inside other boxes ship as geometry and bake as fully-occluded black. They also cost triangles.

Whether UV0 (the production atlas) survives the bevel is the open question this script exists to
answer, so it reports the UV channel count and area before and after, and the render that follows
shows whether the atlas still lands where it should.
"""
import unreal, json, os

OUT = os.environ["EBS_DETAIL_OUT"]; os.makedirs(OUT, exist_ok=True)
ASSET_PATH = os.environ["EBS_DETAIL_ASSET_PATH"]
DEST = os.environ.get("EBS_DETAIL_DEST", "/Game/Echoes/Detail")
P = {"crease_angle": 15.0, "bevel_distance": 1.6, "bevel_subdivisions": 2,
     "remove_hidden": True, "displace": False, "noise_magnitude": 0.6, "noise_frequency": 0.06,
     "weld_tolerance": 0.01, "budget": 7500}
P.update(json.loads(os.environ.get("EBS_DETAIL_PARAMS", "{}")))
report = {"params": P, "errors": [], "stages": []}

AU=unreal.GeometryScript_AssetUtils; MQ=unreal.GeometryScript_MeshQueries
PG=unreal.GeometryScript_PolyGroups; MM=unreal.GeometryScript_MeshModeling
RP=unreal.GeometryScript_MeshRepair; DF=unreal.GeometryScript_MeshDeformers
NR=unreal.GeometryScript_Normals; BW=unreal.GeometryScript_BoneWeights
VC=unreal.GeometryScript_VertexColors; DEC=unreal.GeometryScript_MeshDecomposition
UV=unreal.GeometryScript_UVs; NEW=unreal.GeometryScript_NewAssetUtils
SIM=unreal.GeometryScript_MeshSimplification

def counts(mesh, label):
    s = MQ.get_mesh_info_string(mesh)
    def n(k): return int(s.split(k)[1].split()[0]) if k in s else -1
    box = MQ.get_mesh_bounding_box(mesh)
    out = {"vertices": n("Vertices count"), "triangles": n("Triangles count"),
           "height_cm": round(box.max.z - box.min.z, 2),
           "has_vertex_colors": MQ.get_has_vertex_colors(mesh)}
    try:
        out["uv_area_uv0"] = round(MQ.get_mesh_uv_area(mesh, 0)[1], 4)
    except Exception as error:
        out["uv_area_error"] = str(error)[:120]
    report[label] = out
    return out

def stage(name, fn):
    try:
        fn(); report["stages"].append({"stage": name, "ok": True}); return True
    except Exception as error:
        report["stages"].append({"stage": name, "ok": False, "error": f"{type(error).__name__}: {error}"[:250]})
        report["errors"].append(name); return False

reg = unreal.AssetRegistryHelpers.get_asset_registry()
found = [a for a in reg.get_assets_by_path(ASSET_PATH, recursive=True)
         if str(a.asset_class_path.asset_name) == "SkeletalMesh" and "LOD1" not in str(a.asset_name)]
skm = found[0].get_asset(); report["source"] = skm.get_path_name()
skeleton = skm.get_editor_property("skeleton")

low = unreal.DynamicMesh(); mesh = unreal.DynamicMesh()
AU.copy_mesh_from_skeletal_mesh(skm, low, unreal.GeometryScriptCopyMeshFromAssetOptions(), unreal.GeometryScriptMeshReadLOD())
DEC.copy_mesh_to_mesh(low, mesh)
counts(low, "shipped")

weld = unreal.GeometryScriptWeldEdgesOptions()
weld.set_editor_property("tolerance", P["weld_tolerance"]); weld.set_editor_property("only_unique_pairs", False)
stage("weld", lambda: RP.weld_mesh_edges(mesh, weld))
counts(mesh, "after_weld")
if P["remove_hidden"]:
    stage("remove_hidden", lambda: RP.remove_hidden_triangles(mesh, unreal.GeometryScriptRemoveHiddenTrianglesOptions()))
    counts(mesh, "after_remove_hidden")
group_layer = unreal.GeometryScriptGroupLayer()
stage("polygroups", lambda: (PG.enable_polygroups(mesh),
                             PG.compute_polygroups_from_angle_threshold(mesh, group_layer, P["crease_angle"], 1)))
bevel = unreal.GeometryScriptMeshBevelOptions()
bevel.set_editor_property("bevel_distance", P["bevel_distance"]); bevel.set_editor_property("subdivisions", P["bevel_subdivisions"])
stage("bevel", lambda: MM.apply_mesh_polygroup_bevel(mesh, bevel))
counts(mesh, "after_bevel")
if P["displace"]:
    noise = unreal.GeometryScriptPerlinNoiseOptions()
    bl = noise.get_editor_property("base_layer")
    bl.set_editor_property("magnitude", P["noise_magnitude"]); bl.set_editor_property("frequency", P["noise_frequency"]); bl.set_editor_property("random_seed", 7)
    noise.set_editor_property("base_layer", bl)
    try: noise.set_editor_property("apply_along_normal", True)
    except Exception: pass
    stage("displace", lambda: DF.apply_perlin_noise_to_mesh(mesh, unreal.GeometryScriptMeshSelection(), noise, None))
    counts(mesh, "after_displace")
if report.get("after_bevel", {}).get("triangles", 0) > P["budget"]:
    stage("simplify_to_budget", lambda: SIM.apply_simplify_to_triangle_count(mesh, P["budget"], unreal.GeometryScriptSimplifyMeshOptions()))
    counts(mesh, "after_simplify")
stage("transfer_bone_weights", lambda: BW.transfer_bone_weights_from_mesh(low, mesh))
stage("transfer_vertex_colors", lambda: VC.transfer_vertex_colors_from_mesh(low, mesh))
stage("normals", lambda: NR.recompute_normals(mesh, unreal.GeometryScriptCalculateNormalsOptions()))

# --- UVs -------------------------------------------------------------------------------------
# Bevel preserves UV0 on the ORIGINAL faces but gives the NEW bevel faces UVs in unpainted atlas
# space, so the chamfers render as pale strips. Regenerate UV0 for the whole detailed mesh; the
# source textures are transferred onto the new layout by a texture-type bake (ue_detail_bake.py).
if P.get("regenerate_uvs", True):
    uv_options = unreal.GeometryScriptPatchBuilderOptions()
    for key, value in (("initial_patch_count", P.get("uv_patch_count", 220)),
                       ("merging_threshold", 1.5), ("max_distortion", 5.0),
                       ("min_patch_size", 2), ("auto_pack", True),
                       ("packing_target_width", P.get("uv_resolution", 2048))):
        try:
            uv_options.set_editor_property(key, value)
        except Exception as error:
            report["errors"].append(f"patch_builder.{key}: {error}")
    report["patch_builder_props"] = sorted(x for x in dir(uv_options) if not x.startswith("_"))
    stage("regenerate_uv0", lambda: UV.auto_generate_patch_builder_mesh_u_vs(mesh, 0, uv_options))
    counts(mesh, "after_uv")
counts(mesh, "final")

name = "SK_EBS_KHA_UNT_002_Detail"
path = f"{DEST}/{name}"
if unreal.EditorAssetLibrary.does_asset_exist(path):
    unreal.EditorAssetLibrary.delete_asset(path)
created = None
def make():
    global created
    options = unreal.GeometryScriptCreateNewSkeletalMeshAssetOptions()
    try: options.set_editor_property("enable_recompute_normals", False)
    except Exception: pass
    created = NEW.create_new_skeletal_mesh_asset_from_mesh(mesh, skeleton, path, options)
stage("create_skeletal_asset", make)
if created is not None:
    asset = created[0] if isinstance(created, tuple) else created
    report["created"] = asset.get_path_name() if asset else None
    if asset:
        unreal.EditorAssetLibrary.save_loaded_asset(asset)
json.dump(report, open(os.path.join(OUT, "detailed-mesh-report.json"), "w"), indent=1)
print("[EBS_MESH]" + json.dumps({k: report.get(k) for k in ("shipped","after_weld","after_remove_hidden","after_bevel","final","created","errors")}))
