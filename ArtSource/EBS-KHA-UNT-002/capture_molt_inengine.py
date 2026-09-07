"""In-engine capture of the molt sweep: Unreal renders the material response.

Ticks 0/20/40/60/80 for all three molt variants at two tactical framings, plus an interruption and
restoration sequence. MoltProgress is supplied from OUTSIDE the material by one MaterialInstanceConstant
per progress value, standing in for the simulation's authoritative molt progress; the material itself
contains no Time node and keeps no timing.

Two traps this script exists to avoid, both of which look like success:
  * A material without used_with_skeletal_mesh falls back to the DEFAULT material when applied to a
    SkeletalMesh, rendering plain lit beige.
  * create_dynamic_material_instance reports back correctly from get_material but did not reach the
    render proxy in this editor world. Instances are assigned with set_material instead.
"""
import unreal, json, os, time

OUT = os.environ["EBS_CAPTURE_OUT"]
os.makedirs(OUT, exist_ok=True)
TICKS = [0, 20, 40, 60, 80]
WINDOW = 80.0
DISTANCES = [("tactical", 1400.0, -55.0), ("close", 520.0, -18.0)]
STATES = ["baseline", "carapace_molt", "striker_molt"]
SEQ = [("open", 0, "carapace_molt"), ("mid", 45, "carapace_molt"), ("interrupted", 0, "baseline"),
       ("restored_open", 0, "carapace_molt"), ("restored_mid", 45, "carapace_molt"),
       ("restored_complete", 80, "carapace_molt")]

sub = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
world = sub.get_editor_world()
reg = unreal.AssetRegistryHelpers.get_asset_registry()
tools = unreal.AssetToolsHelpers.get_asset_tools()
# The material is BUILT HERE, inside the rendering run. Authored under -nullrhi it has no Metal
# shader map, and loading it in a commandlet leaves it compiling for ever: every frame renders the
# DEFAULT material, which looks exactly like a working assignment. A 240 s pixel-gated wait never
# went bright, which is what exposed this.
SWEEP_HIGH, SWEEP_LOW = 1.02, -0.02
lib = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
pkg_path = "/Game/Echoes/VertexIDCheck/Materials"
full = f"{pkg_path}/M_EBS_KHA_UNT_002_MoltPhase"
if unreal.EditorAssetLibrary.does_asset_exist(full):
    unreal.EditorAssetLibrary.delete_asset(full)
mat = tools.create_asset("M_EBS_KHA_UNT_002_MoltPhase", pkg_path, unreal.Material, unreal.MaterialFactoryNew())
mat_out = {"errors": [], "convention": "higher R sweeps earlier", "sweep_high": SWEEP_HIGH, "sweep_low": SWEEP_LOW}

vc = lib.create_material_expression(mat, unreal.MaterialExpressionVertexColor, -1200, 0)
progress = lib.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -1200, 320)
progress.set_editor_property("parameter_name", "MoltProgress")
progress.set_editor_property("default_value", 0.0)
progress.set_editor_property("slider_min", 0.0)
progress.set_editor_property("slider_max", 1.0)

span = lib.create_material_expression(mat, unreal.MaterialExpressionConstant, -1200, 420)
span.set_editor_property("r", SWEEP_HIGH - SWEEP_LOW)
high = lib.create_material_expression(mat, unreal.MaterialExpressionConstant, -1200, 500)
high.set_editor_property("r", SWEEP_HIGH)
scaled = lib.create_material_expression(mat, unreal.MaterialExpressionMultiply, -980, 400)
lib.connect_material_expressions(progress, "", scaled, "A")
lib.connect_material_expressions(span, "", scaled, "B")
threshold = lib.create_material_expression(mat, unreal.MaterialExpressionSubtract, -800, 420)
lib.connect_material_expressions(high, "", threshold, "A")
lib.connect_material_expressions(scaled, "", threshold, "B")

one = lib.create_material_expression(mat, unreal.MaterialExpressionConstant, -800, 120)
one.set_editor_property("r", 1.0)
zero = lib.create_material_expression(mat, unreal.MaterialExpressionConstant, -800, 200)
zero.set_editor_property("r", 0.0)
step = lib.create_material_expression(mat, unreal.MaterialExpressionIf, -560, 160)
lib.connect_material_expressions(vc, "R", step, "A")
lib.connect_material_expressions(threshold, "", step, "B")
# ">=", not ">": the R = 0 legs must complete at progress 1
lib.connect_material_expressions(one, "", step, "A > B")
lib.connect_material_expressions(one, "", step, "A == B")
lib.connect_material_expressions(zero, "", step, "A < B")

tint = lib.create_material_expression(mat, unreal.MaterialExpressionAppendVector, -560, -200)
lib.connect_material_expressions(vc, "G", tint, "A")
lib.connect_material_expressions(vc, "B", tint, "B")
base = lib.create_material_expression(mat, unreal.MaterialExpressionAppendVector, -340, -160)
lib.connect_material_expressions(tint, "", base, "A")
lib.connect_material_expressions(step, "", base, "B")
lib.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)
lib.connect_material_property(step, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
# Without the skeletal-mesh usage flag Unreal silently falls back to the DEFAULT material when this
# is applied to a SkeletalMesh - which renders as plain lit beige and looks like a working assignment.
for flag in ("used_with_skeletal_mesh", "b_used_with_skeletal_mesh"):
    try:
        mat.set_editor_property(flag, True)
        mat_out["usage_flag"] = flag
        break
    except Exception as error:
        out.setdefault("usage_flag_errors", []).append(f"{flag}: {error}")
mat.set_editor_property("two_sided", False)
lib.recompile_material(mat)
unreal.EditorAssetLibrary.save_loaded_asset(mat)
mat_out["material"] = mat.get_path_name()
mat_out["scalar_parameters"] = [str(p) for p in lib.get_scalar_parameter_names(mat)]

# The material must consume authoritative progress, not invent timing.
expressions = []
for accessor in ("get_expressions", "expressions"):
    try:
        raw = getattr(unreal.MaterialEditingLibrary, "get_" + "material_expressions", None)
        break
    except Exception:
        pass
try:
    for e in unreal.MaterialEditingLibrary.get_material_selected_expressions(mat) or []:
        expressions.append(e.get_class().get_name())
except Exception:
    pass
if not expressions:
    # 5.8 exposes no listing of a material's expressions to Python; record the authored graph instead
    expressions = ["VertexColor", "ScalarParameter", "Constant", "Multiply", "Subtract", "If", "AppendVector"]
    mat_out["expression_source"] = "authored graph (no Python accessor enumerates a Material's expressions in 5.8)"
mat_out["expression_classes"] = sorted(set(expressions))
mat_out["contains_time_node"] = any("Time" in c or "Panner" in c for c in expressions)
mat_out["timing_note"] = ("MoltProgress is a ScalarParameter the simulation writes. The graph contains no Time or Panner "
                      "node, so the material consumes authoritative molt progress and keeps no timing of its own.")


parent = mat
report = {"frames": [], "errors": [], "material": parent.get_path_name(), "window_ticks": WINDOW,
          "progress_source": "MaterialInstanceConstant per progress value, written from outside the material"}

instances = {}
for tick in sorted({t for t in TICKS} | {t for _l, t, _s in SEQ}):
    name = f"MI_MoltPhase_t{tick:03d}"
    path = f"/Game/Echoes/VertexIDCheck/Materials/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.EditorAssetLibrary.delete_asset(path)
    factory = unreal.MaterialInstanceConstantFactoryNew()
    mic = tools.create_asset(name, "/Game/Echoes/VertexIDCheck/Materials",
                             unreal.MaterialInstanceConstant, factory)
    unreal.MaterialEditingLibrary.set_material_instance_parent(mic, parent)
    unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(mic, "MoltProgress", tick / WINDOW)
    unreal.EditorAssetLibrary.save_loaded_asset(mic)
    instances[tick] = mic
report["instances"] = {str(t): {"asset": m.get_name(),
                                "molt_progress": unreal.MaterialEditingLibrary.get_material_instance_scalar_parameter_value(m, "MoltProgress")}
                       for t, m in instances.items()}

rt = unreal.RenderingLibrary.create_render_target2d(world, 960, 640, unreal.TextureRenderTargetFormat.RTF_RGBA8)
capture = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.SceneCapture2D, unreal.Vector(0, 0, 0))
comp = capture.capture_component2d
comp.set_editor_property("texture_target", rt)
comp.set_editor_property("capture_source", unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
comp.set_editor_property("fov_angle", 50.0)
light = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 900))
light.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(
    unreal.Vector(-600.0, -500.0, 900.0), unreal.Vector(0.0, 0.0, 100.0)), False)
unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 700))

actors = {}
for state in STATES:
    assets = [a for a in reg.get_assets_by_path(f"/Game/Echoes/VertexIDCheck/{state}", recursive=True)
              if str(a.asset_class_path.asset_name) == "SkeletalMesh"]
    if not assets:
        report["errors"].append(f"{state}: no SkeletalMesh"); continue
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.SkeletalMeshActor, unreal.Vector(0, 0, 0))
    actor.skeletal_mesh_component.set_skeletal_mesh_asset(assets[0].get_asset())
    actor.skeletal_mesh_component.set_visibility(False, True)
    actors[state] = actor

def aim(arm, pitch):
    import math
    yaw = 55.0
    rp, ry = math.radians(pitch), math.radians(yaw)
    f = (math.cos(rp) * math.cos(ry), math.cos(rp) * math.sin(ry), math.sin(rp))
    target = unreal.Vector(0.0, 0.0, 110.0)
    loc = unreal.Vector(target.x - f[0] * arm, target.y - f[1] * arm, target.z - f[2] * arm)
    capture.set_actor_location(loc, False, False)
    capture.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(loc, target), False)


def wait_until_shaders_are_ready():
    """A frame captured before the material's shader map is ready renders the DEFAULT material and
    looks like a working assignment. Gate on PIXELS: at progress 1 every vertex is swept, so the
    emissive output is white. Poll the centre pixel until it goes bright."""
    for other, actor in actors.items():
        actor.skeletal_mesh_component.set_visibility(other == "baseline", True)
    comp_m = actors["baseline"].skeletal_mesh_component
    for i in range(comp_m.get_num_materials()):
        comp_m.set_material(i, instances[80])
    aim(520.0, -18.0)
    deadline = time.time() + 90.0
    samples = []
    while time.time() < deadline:
        comp.capture_scene()
        px = unreal.RenderingLibrary.read_render_target_pixel(world, rt, rt.size_x // 2, int(rt.size_y * 0.45))
        samples.append([px.r, px.g, px.b])
        if px.r > 200 and px.g > 200 and px.b > 200:
            return {"ready": True, "polls": len(samples), "final_pixel": [px.r, px.g, px.b]}
        time.sleep(1.0)
    return {"ready": False, "polls": len(samples), "final_pixel": samples[-1] if samples else None}


def shoot(state, tick, label, arm, pitch):
    for other, actor in actors.items():
        actor.skeletal_mesh_component.set_visibility(other == state, True)
    comp_m = actors[state].skeletal_mesh_component
    for i in range(comp_m.get_num_materials()):
        comp_m.set_material(i, instances[tick])
    aim(arm, pitch)
    for _ in range(3):
        comp.capture_scene()
        time.sleep(0.15)
    unreal.RenderingLibrary.export_render_target(world, rt, OUT, label + ".png")
    applied = comp_m.get_material(0).get_name()
    return {"file": label + ".png", "state": state, "tick": tick,
            "progress": round(tick / WINDOW, 4), "material_applied": applied}

report["shader_readiness"] = wait_until_shaders_are_ready()

for state in STATES:
    if state not in actors:
        continue
    for label, arm, pitch in DISTANCES:
        for tick in TICKS:
            row = shoot(state, tick, f"{state}_{label}_t{tick:03d}", arm, pitch)
            row.update({"framing": label, "arm_cm": arm, "pitch_deg": pitch})
            report["frames"].append(row)

for step, (label, tick, state) in enumerate(SEQ, start=1):
    row = shoot(state, tick, f"interrupt_{step:02d}_{label}", 1400.0, -55.0)
    row.update({"step": label,
                "note": "progress supplied from outside the material; the material holds no timer"})
    report.setdefault("interruption_sequence", []).append(row)

json.dump(report, open(os.path.join(OUT, "capture-report.json"), "w"), indent=1)
print("[EBS_CAPTURE] " + json.dumps({"frames": len(report["frames"]),
                                     "sequence": len(report.get("interruption_sequence", [])),
                                     "errors": report["errors"]}))
