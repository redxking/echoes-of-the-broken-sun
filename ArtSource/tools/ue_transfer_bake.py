"""ue_transfer_bake.py — bake a blockout's atlas textures onto a detailed mesh's new UVs, in Unreal.

Author: Angelis Pseftis. Runs inside Unreal.

A topology change invalidates a chart-packed atlas (PF-024): the detailed mesh's regenerated UV0 has
no relationship to the blockout's charts. This transfers the shipped maps across by sampling the
SOURCE mesh's atlas at each TARGET texel, and bakes tangent-normal / AO / curvature from the same
high-poly detail chain at the same time, so the detailed mesh carries one coherent set.

    source = blockout (production atlas UVs, the baked textures applied)
    target = detailed mesh (regenerated UVs)

Environment: EBS_TB_SOURCE_PATH, EBS_TB_TARGET_PATH, EBS_TB_TEX_DIR, EBS_TB_OUT, EBS_TB_PREFIX
"""
import unreal, json, os

OUT = os.environ["EBS_TB_OUT"]; os.makedirs(OUT, exist_ok=True)
PREFIX = os.environ["EBS_TB_PREFIX"]; TEX_DIR = os.environ["EBS_TB_TEX_DIR"]
DEST = "/Game/Echoes/VertexIDCheck/ArtTextures"
P = {"resolution": 2048, "samples_per_pixel": 4, "crease_angle": 15.0, "bevel_distance": 1.6,
     "bevel_subdivisions": 2, "tessellation": 1, "noise_magnitude": 1.1, "noise_frequency": 0.05,
     "weld_tolerance": 0.01}
P.update(json.loads(os.environ.get("EBS_TB_PARAMS", "{}")))
report = {"params": P, "errors": [], "stages": [], "maps": {}}

AU=unreal.GeometryScript_AssetUtils; MQ=unreal.GeometryScript_MeshQueries
PG=unreal.GeometryScript_PolyGroups; MM=unreal.GeometryScript_MeshModeling
RP=unreal.GeometryScript_MeshRepair; SD=unreal.GeometryScript_MeshSubdivide
DF=unreal.GeometryScript_MeshDeformers; NR=unreal.GeometryScript_Normals
DEC=unreal.GeometryScript_MeshDecomposition; BAKE=unreal.GeometryScript_Bake

def stage(name, fn):
    try:
        fn(); report["stages"].append({"stage": name, "ok": True}); return True
    except Exception as error:
        report["stages"].append({"stage": name, "ok": False, "error": f"{type(error).__name__}: {error}"[:250]})
        report["errors"].append(name); return False

def counts(mesh):
    s = MQ.get_mesh_info_string(mesh)
    def n(k): return int(s.split(k)[1].split()[0]) if k in s else -1
    return {"vertices": n("Vertices count"), "triangles": n("Triangles count")}

reg = unreal.AssetRegistryHelpers.get_asset_registry()
def find(path, exclude="LOD1"):
    for a in reg.get_assets_by_path(path, recursive=True):
        if str(a.asset_class_path.asset_name) == "SkeletalMesh" and exclude not in str(a.asset_name):
            return a.get_asset()
    return None
source_asset = find(os.environ["EBS_TB_SOURCE_PATH"]); target_asset = find(os.environ["EBS_TB_TARGET_PATH"])
report["source"] = source_asset.get_path_name() if source_asset else None
report["target"] = target_asset.get_path_name() if target_asset else None
if not (source_asset and target_asset):
    report["errors"].append("source or target not found")
    json.dump(report, open(os.path.join(OUT, "transfer-bake-report.json"), "w"), indent=1)
    print("[EBS_TB]" + json.dumps({"errors": report["errors"]})); raise SystemExit(0)

source = unreal.DynamicMesh(); target = unreal.DynamicMesh(); high = unreal.DynamicMesh()
AU.copy_mesh_from_skeletal_mesh(source_asset, source, unreal.GeometryScriptCopyMeshFromAssetOptions(), unreal.GeometryScriptMeshReadLOD())
AU.copy_mesh_from_skeletal_mesh(target_asset, target, unreal.GeometryScriptCopyMeshFromAssetOptions(), unreal.GeometryScriptMeshReadLOD())
report["source_counts"] = counts(source); report["target_counts"] = counts(target)

# high-poly detail source, from the SAME chain the detailed mesh came from
DEC.copy_mesh_to_mesh(source, high)
weld = unreal.GeometryScriptWeldEdgesOptions(); weld.set_editor_property("tolerance", P["weld_tolerance"]); weld.set_editor_property("only_unique_pairs", False)
group_layer = unreal.GeometryScriptGroupLayer()
bevel = unreal.GeometryScriptMeshBevelOptions()
bevel.set_editor_property("bevel_distance", P["bevel_distance"]); bevel.set_editor_property("subdivisions", P["bevel_subdivisions"])
noise = unreal.GeometryScriptPerlinNoiseOptions(); bl = noise.get_editor_property("base_layer")
bl.set_editor_property("magnitude", P["noise_magnitude"]); bl.set_editor_property("frequency", P["noise_frequency"]); bl.set_editor_property("random_seed", 7)
noise.set_editor_property("base_layer", bl)
try: noise.set_editor_property("apply_along_normal", True)
except Exception: pass
stage("high_weld", lambda: RP.weld_mesh_edges(high, weld))
stage("high_remove_hidden", lambda: RP.remove_hidden_triangles(high, unreal.GeometryScriptRemoveHiddenTrianglesOptions()))
stage("high_groups", lambda: (PG.enable_polygroups(high), PG.compute_polygroups_from_angle_threshold(high, group_layer, P["crease_angle"], 1)))
stage("high_bevel", lambda: MM.apply_mesh_polygroup_bevel(high, bevel))
stage("high_tessellate", lambda: SD.apply_uniform_tessellation(high, P["tessellation"]))
stage("high_displace", lambda: DF.apply_perlin_noise_to_mesh(high, unreal.GeometryScriptMeshSelection(), noise, None))
stage("high_normals", lambda: NR.recompute_normals(high, unreal.GeometryScriptCalculateNormalsOptions()))
report["high_counts"] = counts(high)

# the blockout's shipped maps, imported so they can be sampled as bake sources
src_textures = {}
for key, (fname, srgb) in {"BaseColor": (f"{PREFIX}_BaseColor.png", True),
                           "MRE": (f"{PREFIX}_MRE.png", False),
                           "StateMask": (f"{PREFIX}_StateMask.png", False)}.items():
    path = os.path.join(TEX_DIR, fname)
    if not os.path.exists(path):
        report["errors"].append(f"missing source texture {fname}"); continue
    task = unreal.AssetImportTask(); task.filename = path; task.destination_path = DEST
    task.replace_existing = True; task.automated = True; task.save = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    tex = unreal.EditorAssetLibrary.load_asset(f"{DEST}/{fname[:-4]}")
    if tex:
        tex.set_editor_property("srgb", srgb); tex.set_editor_property("never_stream", True)
        unreal.EditorAssetLibrary.save_loaded_asset(tex); src_textures[key] = tex

target_options = unreal.GeometryScriptBakeTargetMeshOptions(); target_options.set_editor_property("target_uv_layer", 0)
source_options = unreal.GeometryScriptBakeSourceMeshOptions()
bake_options = unreal.GeometryScriptBakeTextureOptions()
res = getattr(unreal.GeometryScriptBakeResolution, f"RESOLUTION{P['resolution']}", None)
spp = getattr(unreal.GeometryScriptBakeSamplesPerPixel, f"SAMPLE{P['samples_per_pixel']}", None)
for key, value in (("resolution", res), ("samples_per_pixel", spp), ("gutter_size", 4)):
    if value is not None:
        try: bake_options.set_editor_property(key, value)
        except Exception as error: report["errors"].append(f"bake_options.{key}: {error}")

def export(tex, name):
    path = os.path.join(OUT, f"{PREFIX}_Detail{name}.png")
    task = unreal.AssetExportTask(); task.object = tex; task.filename = path
    task.automated = True; task.prompt = False; task.exporter = unreal.TextureExporterPNG()
    ok = unreal.Exporter.run_asset_export_task(task)
    report["maps"][name] = {"file": os.path.basename(path), "exported": bool(ok),
                            "bytes": os.path.getsize(path) if os.path.exists(path) else 0}

# 1. transfer the shipped atlas maps: source mesh (atlas UVs + texture) -> target mesh (new UVs)
for key, tex in src_textures.items():
    def do(key=key, tex=tex):
        bt = BAKE.make_bake_type_texture(tex, 0)
        result = BAKE.bake_texture(target, unreal.Transform(), target_options,
                                   source, unreal.Transform(), source_options, [bt], bake_options)
        out = list(result)[0] if result else None
        if out is None: raise RuntimeError("bake returned no texture")
        export(out, key)
    stage(f"transfer_{key}", do)

# 2. detail maps from the high-poly onto the same target UVs
def do_detail():
    types = [("Normal", BAKE.make_bake_type_tangent_normal()), ("AO", BAKE.make_bake_type_ambient_occlusion()),
             ("Curvature", BAKE.make_bake_type_curvature())]
    result = BAKE.bake_texture(target, unreal.Transform(), target_options,
                               high, unreal.Transform(), source_options, [t for _n, t in types], bake_options)
    for (name, _t), tex in zip(types, list(result)):
        if tex is not None: export(tex, name)
stage("detail_maps", do_detail)

json.dump(report, open(os.path.join(OUT, "transfer-bake-report.json"), "w"), indent=1)
print("[EBS_TB]" + json.dumps({"source": report["source_counts"], "target": report["target_counts"],
                               "high": report.get("high_counts"), "maps": {k: v["bytes"] for k, v in report["maps"].items()},
                               "errors": report["errors"]}))
