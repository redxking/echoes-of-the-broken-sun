"""ue_detail_bake.py — in-Unreal geometric detail pass and high-to-low bake.

Author: Angelis Pseftis. Runs INSIDE Unreal (UnrealEditor-Cmd -ExecutePythonScript), never standalone.

The blockout generators build hard-faceted boxes and prisms; the concepts show chamfered, layered
mineral plates with surface irregularity. This pass adds that detail with Unreal's Geometry Script and
bakes it back onto the shipped low-poly, which already carries the production UV atlas:

    skeletal mesh -> DynamicMesh -> weld -> polygroups from crease angle -> polygroup bevel
                                        -> uniform tessellation -> Perlin displacement   (HIGH)
    bake tangent-normal / AO / curvature   HIGH -> LOW (UV0 = the production atlas)

WELDING IS NOT OPTIONAL. ebs_meshkit writes unindexed triangles (the Riftstalker: 542 triangles,
1626 vertices, no shared edges), and bevel on triangle soup is a silent no-op that returns its mesh
unchanged. Welding first takes that mesh to 351 vertices and bevel then works.

Environment:
    EBS_DETAIL_ASSET_PATH   /Game path searched for the SkeletalMesh (required)
    EBS_DETAIL_OUT          directory for the PNGs and the report (required)
    EBS_DETAIL_PREFIX       texture file prefix, e.g. T_EBS_KHA_UNT_002 (required)
    EBS_DETAIL_PARAMS       optional JSON: crease_angle, bevel_distance, bevel_subdivisions,
                            tessellation, noise_magnitude, noise_frequency, resolution
"""
import unreal, json, os

OUT = os.environ["EBS_DETAIL_OUT"]
PREFIX = os.environ["EBS_DETAIL_PREFIX"]
ASSET_PATH = os.environ["EBS_DETAIL_ASSET_PATH"]
P = {"crease_angle": 15.0, "bevel_distance": 1.6, "bevel_subdivisions": 2, "tessellation": 1,
     "noise_magnitude": 1.1, "noise_frequency": 0.05, "noise_seed": 7, "resolution": 2048,
     "samples_per_pixel": 4, "weld_tolerance": 0.01, "target_uv_layer": 0}
P.update(json.loads(os.environ.get("EBS_DETAIL_PARAMS", "{}")))
os.makedirs(OUT, exist_ok=True)
report = {"params": P, "errors": [], "stages": []}

AU = unreal.GeometryScript_AssetUtils; MQ = unreal.GeometryScript_MeshQueries
PG = unreal.GeometryScript_PolyGroups; MM = unreal.GeometryScript_MeshModeling
RP = unreal.GeometryScript_MeshRepair; SD = unreal.GeometryScript_MeshSubdivide
DF = unreal.GeometryScript_MeshDeformers; NR = unreal.GeometryScript_Normals
DEC = unreal.GeometryScript_MeshDecomposition; BAKE = unreal.GeometryScript_Bake


def counts(mesh):
    s = MQ.get_mesh_info_string(mesh)
    def n(key):
        return int(s.split(key)[1].split()[0]) if key in s else -1
    box = MQ.get_mesh_bounding_box(mesh)
    return {"vertices": n("Vertices count"), "triangles": n("Triangles count"),
            "height_cm": round(box.max.z - box.min.z, 2)}


def stage(name, fn):
    try:
        fn(); report["stages"].append({"stage": name, "ok": True})
        return True
    except Exception as error:
        report["stages"].append({"stage": name, "ok": False, "error": f"{type(error).__name__}: {error}"[:300]})
        report["errors"].append(name)
        return False


reg = unreal.AssetRegistryHelpers.get_asset_registry()
found = [a for a in reg.get_assets_by_path(ASSET_PATH, recursive=True)
         if str(a.asset_class_path.asset_name) == "SkeletalMesh" and "LOD1" not in str(a.asset_name)]
if not found:
    report["errors"].append("no SkeletalMesh under " + ASSET_PATH)
    json.dump(report, open(os.path.join(OUT, "detail-bake-report.json"), "w"), indent=1)
    print("[EBS_DETAIL]" + json.dumps({"errors": report["errors"]})); raise SystemExit(0)
skm = found[0].get_asset(); report["source"] = skm.get_path_name()

low = unreal.DynamicMesh(); high = unreal.DynamicMesh()
AU.copy_mesh_from_skeletal_mesh(skm, low, unreal.GeometryScriptCopyMeshFromAssetOptions(),
                                unreal.GeometryScriptMeshReadLOD())
report["low_as_shipped"] = counts(low)
DEC.copy_mesh_to_mesh(low, high)

weld_options = unreal.GeometryScriptWeldEdgesOptions()
weld_options.set_editor_property("tolerance", P["weld_tolerance"])
weld_options.set_editor_property("only_unique_pairs", False)
group_layer = unreal.GeometryScriptGroupLayer()
bevel_options = unreal.GeometryScriptMeshBevelOptions()
bevel_options.set_editor_property("bevel_distance", P["bevel_distance"])
bevel_options.set_editor_property("subdivisions", P["bevel_subdivisions"])
noise = unreal.GeometryScriptPerlinNoiseOptions()
base_layer = noise.get_editor_property("base_layer")
base_layer.set_editor_property("magnitude", P["noise_magnitude"])
base_layer.set_editor_property("frequency", P["noise_frequency"])
base_layer.set_editor_property("random_seed", P["noise_seed"])
noise.set_editor_property("base_layer", base_layer)
try:
    noise.set_editor_property("apply_along_normal", True)
except Exception as error:
    report["errors"].append(f"apply_along_normal: {error}")

stage("weld", lambda: RP.weld_mesh_edges(high, weld_options))
report["after_weld"] = counts(high)
stage("polygroups", lambda: (PG.enable_polygroups(high),
                             PG.compute_polygroups_from_angle_threshold(high, group_layer, P["crease_angle"], 1)))
stage("bevel", lambda: MM.apply_mesh_polygroup_bevel(high, bevel_options))
report["after_bevel"] = counts(high)
if P["tessellation"] > 0:
    stage("tessellate", lambda: SD.apply_uniform_tessellation(high, P["tessellation"]))
if P["noise_magnitude"] > 0:
    stage("displace", lambda: DF.apply_perlin_noise_to_mesh(high, unreal.GeometryScriptMeshSelection(), noise, None))
stage("normals", lambda: NR.recompute_normals(high, unreal.GeometryScriptCalculateNormalsOptions()))
report["high"] = counts(high)

# --- bake HIGH -> LOW on the production atlas -----------------------------------------------------
target_options = unreal.GeometryScriptBakeTargetMeshOptions()
target_options.set_editor_property("target_uv_layer", P["target_uv_layer"])
source_options = unreal.GeometryScriptBakeSourceMeshOptions()
bake_options = unreal.GeometryScriptBakeTextureOptions()
for key, value in (("resolution", getattr(unreal.GeometryScriptBakeResolution, f"RESOLUTION{P['resolution']}", None)),
                   ("samples_per_pixel", getattr(unreal.GeometryScriptBakeSamplesPerPixel, f"SAMPLE{P['samples_per_pixel']}", None)),
                   ("gutter_size", 4)):
    if value is not None:
        try:
            bake_options.set_editor_property(key, value)
        except Exception as error:
            report["errors"].append(f"bake_options.{key}: {error}")

bake_types = [("Normal", BAKE.make_bake_type_tangent_normal()),
              ("AO", BAKE.make_bake_type_ambient_occlusion()),
              ("Curvature", BAKE.make_bake_type_curvature())]
textures = None
def do_bake():
    global textures
    textures = BAKE.bake_texture(low, unreal.Transform(), target_options,
                                 high, unreal.Transform(), source_options,
                                 [t for _n, t in bake_types], bake_options)
stage("bake", do_bake)

# --- write the baked textures out as PNG ----------------------------------------------------------
written = {}
if textures:
    for (name, _t), tex in zip(bake_types, list(textures)):
        if tex is None:
            report["errors"].append(f"bake returned None for {name}"); continue
        path = os.path.join(OUT, f"{PREFIX}_Baked{name}.png")
        task = unreal.AssetExportTask()
        task.object = tex; task.filename = path; task.automated = True; task.prompt = False
        task.exporter = unreal.TextureExporterPNG()
        ok = unreal.Exporter.run_asset_export_task(task)
        written[name] = {"file": os.path.basename(path), "exported": bool(ok),
                         "size": [tex.blueprint_get_size_x(), tex.blueprint_get_size_y()],
                         "bytes": os.path.getsize(path) if os.path.exists(path) else 0}
report["baked_maps"] = written
json.dump(report, open(os.path.join(OUT, "detail-bake-report.json"), "w"), indent=1)
print("[EBS_DETAIL]" + json.dumps({"low": report["low_as_shipped"], "high": report.get("high"),
                                   "maps": {k: v["bytes"] for k, v in written.items()},
                                   "errors": report["errors"]}))
