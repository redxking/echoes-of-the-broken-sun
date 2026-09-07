"""Import the baked Riftstalker texture stack, build the ART material on it, and let Unreal render
the molt sweep with it: ticks 0/40/80, all three variants, tactical and close framing.

This is the gap the vertex-ID gate left open: the earlier capture used a DEBUG material whose base
colour was the channels. Here the base colour, normal, roughness and emissive come from the baked
2048^2 stack, the molt sweep from COLOR_0.R against MoltProgress, the emissive from MRE.B in Broken-Sun
Amber, and the team tint from TeamColor x StateMask.B.

Built inside the rendering run (PF-016), captured through instances assigned with set_material
(PF-017), exposure chosen by a measured bracket rather than a guess (a guessed fixed exposure blew one
capture out to a uniform tan and drove the next to black), and gated on pixels rather than a sleep.
"""
import unreal, json, os, time, math

TEX_DIR = os.environ["EBS_TEX_DIR"]
OUT = os.environ["EBS_ART_OUT"]
os.makedirs(OUT, exist_ok=True)
DEST = "/Game/Echoes/VertexIDCheck/ArtTextures"
MAT_DIR = "/Game/Echoes/VertexIDCheck/Materials"
TICKS = [0, 40, 80]
WINDOW = 80.0
STATES = ["baseline", "carapace_molt", "striker_molt"]
SWEEP_HIGH, SWEEP_LOW = 1.02, -0.02
AMBER = (1.0, 0.82, 0.62)
report = {"errors": [], "textures": {}, "frames": []}

# --- textures ------------------------------------------------------------------------------
tools = unreal.AssetToolsHelpers.get_asset_tools()
maps = {"BaseColor": ("T_EBS_KHA_UNT_002_BaseColor.png", True, False),
        "Normal": ("T_EBS_KHA_UNT_002_Normal.png", False, True),
        "MRE": ("T_EBS_KHA_UNT_002_MRE.png", False, False),
        "StateMask": ("T_EBS_KHA_UNT_002_StateMask.png", False, False),
        "MoltBlend": ("T_EBS_KHA_UNT_002_MoltBlend.png", False, False)}
textures = {}
for key, (fname, srgb, is_normal) in maps.items():
    path = os.path.join(TEX_DIR, fname)
    if not os.path.exists(path):
        report["errors"].append(f"missing {fname}"); continue
    task = unreal.AssetImportTask()
    task.filename = path
    task.destination_path = DEST
    task.replace_existing = True
    task.automated = True
    task.save = True
    tools.import_asset_tasks([task])
    tex = unreal.EditorAssetLibrary.load_asset(f"{DEST}/{fname[:-4]}")
    if tex is None:
        report["errors"].append(f"import failed {fname}"); continue
    tex.set_editor_property("srgb", srgb)
    # A headless run streams only the 32x32 mips in; the capture must have the full texture resident.
    tex.set_editor_property("never_stream", True)
    if is_normal:
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
    else:
        tex.set_editor_property("compression_settings",
                                unreal.TextureCompressionSettings.TC_DEFAULT if srgb else unreal.TextureCompressionSettings.TC_MASKS)
    unreal.EditorAssetLibrary.save_loaded_asset(tex)
    textures[key] = tex
    report["textures"][key] = {"asset": tex.get_path_name(), "srgb": srgb, "never_stream": True,
                               "size_at_import": [tex.blueprint_get_size_x(), tex.blueprint_get_size_y()]}

# --- art material ---------------------------------------------------------------------------
lib = unreal.MaterialEditingLibrary
report["connections"] = {"ok": 0, "failed": []}

def connect(src, src_pin, dst, dst_pin):
    ok = lib.connect_material_expressions(src, src_pin, dst, dst_pin)
    if ok:
        report["connections"]["ok"] += 1
    else:
        report["connections"]["failed"].append(f"{src.get_name()}.{src_pin or 'out'} -> {dst.get_name()}.{dst_pin}")
    return ok

def connect_prop(src, src_pin, prop):
    ok = lib.connect_material_property(src, src_pin, prop)
    if ok:
        report["connections"]["ok"] += 1
    else:
        report["connections"]["failed"].append(f"{src.get_name()}.{src_pin or 'out'} -> {prop}")
    return ok
full = f"{MAT_DIR}/M_EBS_KHA_UNT_002_Art"
if unreal.EditorAssetLibrary.does_asset_exist(full):
    unreal.EditorAssetLibrary.delete_asset(full)
mat = tools.create_asset("M_EBS_KHA_UNT_002_Art", MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property("used_with_skeletal_mesh", True)

def tex_node(key, x, y, sampler=None):
    n = lib.create_material_expression(mat, unreal.MaterialExpressionTextureSample, x, y)
    n.set_editor_property("texture", textures[key])
    if sampler is not None:
        n.set_editor_property("sampler_type", sampler)
    return n

base = tex_node("BaseColor", -1400, -400, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
normal = tex_node("Normal", -1400, -100, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
mre = tex_node("MRE", -1400, 200, unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
state = tex_node("StateMask", -1400, 500, unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
vc = lib.create_material_expression(mat, unreal.MaterialExpressionVertexColor, -1400, 800)

# molt sweep from COLOR_0.R against authoritative progress (same convention as the debug material)
progress = lib.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -1400, 1000)
progress.set_editor_property("parameter_name", "MoltProgress")
progress.set_editor_property("default_value", 0.0)
span = lib.create_material_expression(mat, unreal.MaterialExpressionConstant, -1400, 1100)
span.set_editor_property("r", SWEEP_HIGH - SWEEP_LOW)
high = lib.create_material_expression(mat, unreal.MaterialExpressionConstant, -1400, 1180)
high.set_editor_property("r", SWEEP_HIGH)
scaled = lib.create_material_expression(mat, unreal.MaterialExpressionMultiply, -1150, 1050)
connect(progress, "", scaled, "A")
connect(span, "", scaled, "B")
threshold = lib.create_material_expression(mat, unreal.MaterialExpressionSubtract, -950, 1080)
connect(high, "", threshold, "A")
connect(scaled, "", threshold, "B")
one = lib.create_material_expression(mat, unreal.MaterialExpressionConstant, -950, 850)
one.set_editor_property("r", 1.0)
zero = lib.create_material_expression(mat, unreal.MaterialExpressionConstant, -950, 920)
swept = lib.create_material_expression(mat, unreal.MaterialExpressionIf, -700, 900)
connect(vc, "R", swept, "A")
connect(threshold, "", swept, "B")
connect(one, "", swept, "A > B")
connect(one, "", swept, "A == B")
connect(zero, "", swept, "A < B")

# team tint: lerp(base, TeamColor, StateMask.B)
team = lib.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -1150, 350)
team.set_editor_property("parameter_name", "TeamColor")
team.set_editor_property("default_value", unreal.LinearColor(0.16, 0.86, 0.96, 1.0))
teamed = lib.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, -900, -350)
connect(base, "", teamed, "A")
connect(team, "", teamed, "B")
connect(state, "B", teamed, "Alpha")

# molt-window skin: while swept AND progress < 1, TINT the base toward the translucent core tone
core_tone = lib.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -900, -200)
core_tone.set_editor_property("constant", unreal.LinearColor(0.55, 0.28, 0.10, 1.0))
window = lib.create_material_expression(mat, unreal.MaterialExpressionMultiply, -700, 700)
inv_p = lib.create_material_expression(mat, unreal.MaterialExpressionOneMinus, -950, 1250)
connect(progress, "", inv_p, "")
connect(swept, "", window, "A")
connect(inv_p, "", window, "B")
half = lib.create_material_expression(mat, unreal.MaterialExpressionConstant, -700, 780)
half.set_editor_property("r", 0.45)
window_soft = lib.create_material_expression(mat, unreal.MaterialExpressionMultiply, -550, 720)
connect(window, "", window_soft, "A")
connect(half, "", window_soft, "B")
skinned = lib.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, -450, -300)
connect(teamed, "", skinned, "A")
connect(core_tone, "", skinned, "B")
connect(window_soft, "", skinned, "Alpha")
connect_prop(skinned, "", unreal.MaterialProperty.MP_BASE_COLOR)

# normal, roughness, metallic straight from the stack
connect_prop(normal, "", unreal.MaterialProperty.MP_NORMAL)
connect_prop(mre, "G", unreal.MaterialProperty.MP_ROUGHNESS)
connect_prop(mre, "R", unreal.MaterialProperty.MP_METALLIC)

# emissive: MRE.B x Broken-Sun Amber x strength; brighter through the window (swept x (1-p))
amber = lib.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -900, 150)
amber.set_editor_property("constant", unreal.LinearColor(AMBER[0], AMBER[1], AMBER[2], 1.0))
strength = lib.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -900, 250)
strength.set_editor_property("parameter_name", "EmissiveStrength")
strength.set_editor_property("default_value", 0.5)  # 4.0 and 1.2 both clipped the seams to white under EV+11; they must read amber
e1 = lib.create_material_expression(mat, unreal.MaterialExpressionMultiply, -650, 180)
connect(mre, "B", e1, "A")
connect(amber, "", e1, "B")
e2 = lib.create_material_expression(mat, unreal.MaterialExpressionMultiply, -450, 200)
connect(e1, "", e2, "A")
connect(strength, "", e2, "B")
boost = lib.create_material_expression(mat, unreal.MaterialExpressionAdd, -450, 500)
connect(one, "", boost, "A")
connect(window, "", boost, "B")
e3 = lib.create_material_expression(mat, unreal.MaterialExpressionMultiply, -250, 250)
connect(e2, "", e3, "A")
connect(boost, "", e3, "B")
connect_prop(e3, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
lib.recompile_material(mat)
unreal.EditorAssetLibrary.save_loaded_asset(mat)
report["material"] = mat.get_path_name()
if report["connections"]["failed"]:
    report["errors"].append("material connections failed: " + "; ".join(report["connections"]["failed"]))
report["parameters"] = {"scalar": [str(p) for p in lib.get_scalar_parameter_names(mat)],
                        "vector": [str(p) for p in lib.get_vector_parameter_names(mat)]}

# --- instances per progress value ------------------------------------------------------------
instances = {}
for tick in TICKS:
    name = f"MI_Art_t{tick:03d}"
    path = f"{MAT_DIR}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.EditorAssetLibrary.delete_asset(path)
    mic = tools.create_asset(name, MAT_DIR, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    lib.set_material_instance_parent(mic, mat)
    lib.set_material_instance_scalar_parameter_value(mic, "MoltProgress", tick / WINDOW)
    unreal.EditorAssetLibrary.save_loaded_asset(mic)
    instances[tick] = mic

# --- scene ----------------------------------------------------------------------------------
sub = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
world = sub.get_editor_world()
reg = unreal.AssetRegistryHelpers.get_asset_registry()
rt = unreal.RenderingLibrary.create_render_target2d(world, 1280, 800, unreal.TextureRenderTargetFormat.RTF_RGBA8)
capture = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.SceneCapture2D, unreal.Vector(0, 0, 0))
comp = capture.capture_component2d
comp.set_editor_property("texture_target", rt)
comp.set_editor_property("capture_source", unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
comp.set_editor_property("fov_angle", 50.0)
# Manual exposure, chosen by MEASUREMENT (bracket below).
pp = comp.get_editor_property("post_process_settings")
def set_pp(prop, val):
    try:
        pp.set_editor_property(prop, val)
    except Exception as error:
        report.setdefault("postprocess_warnings", []).append(f"{prop}: {error}")
set_pp("override_auto_exposure_method", True)
set_pp("auto_exposure_method", unreal.AutoExposureMethod.AEM_MANUAL)
set_pp("override_auto_exposure_bias", True)
set_pp("auto_exposure_bias", 0.0)
comp.set_editor_property("post_process_settings", pp)
comp.set_editor_property("post_process_blend_weight", 1.0)

light = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 900))
light.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(unreal.Vector(-600, -500, 900), unreal.Vector(0, 0, 100)), False)
light.light_component.set_intensity(3.0)
light.light_component.set_light_color(unreal.LinearColor(1.0, 0.82, 0.62, 1.0))
sky = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 700))
sky.light_component.set_intensity(0.6)
sky.light_component.set_light_color(unreal.LinearColor(0.48, 0.60, 0.88, 1.0))

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

def aim(arm, pitch, yaw=55.0):
    rp, ry = math.radians(pitch), math.radians(yaw)
    f = (math.cos(rp) * math.cos(ry), math.cos(rp) * math.sin(ry), math.sin(rp))
    target = unreal.Vector(0.0, 0.0, 110.0)
    loc = unreal.Vector(target.x - f[0] * arm, target.y - f[1] * arm, target.z - f[2] * arm)
    capture.set_actor_location(loc, False, False)
    capture.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(loc, target), False)

def sample(x_frac, y_frac):
    px = unreal.RenderingLibrary.read_render_target_pixel(world, rt, int(rt.size_x * x_frac), int(rt.size_y * y_frac))
    return (px.r, px.g, px.b)

def creature_and_ground():
    # Probe the crest ABOVE the team band (shell_01, sweep order 0.60): it is charcoal at tick 0 and
    # takes the window tint at mid-progress, so it is the one region that both reads the body and
    # changes between the t000 and t040 instances. The band itself reads the team colour by design;
    # the lower shells and legs have sweep orders the mid-window threshold never reaches.
    pts = [sample(0.57 + 0.04 * i, 0.225 + 0.015 * j) for i in range(4) for j in range(3)]
    lum = sum(0.2126 * r + 0.7152 * g + 0.0722 * b for r, g, b in pts) / len(pts)
    return lum, sample(0.06, 0.06), pts

def set_bias(bias):
    p = comp.get_editor_property("post_process_settings")
    p.set_editor_property("auto_exposure_bias", float(bias))
    comp.set_editor_property("post_process_settings", p)

def show(state, tick):
    for other, actor in actors.items():
        actor.skeletal_mesh_component.set_visibility(other == state, True)
    c = actors[state].skeletal_mesh_component
    for i in range(c.get_num_materials()):
        c.set_material(i, instances[tick])
    return c

def creature_lum():
    lum, _g, _p = creature_and_ground()
    return lum

def bracket_exposure():
    # Keyed to the GROUND (the engine's default grid, a known mid albedo), not the creature: a bracket
    # keyed to the creature lifted a charcoal body to the ground's brightness and hid the problem.
    show("baseline", 0)
    aim(520.0, -18.0)
    rows, chosen = [], None
    for bias in (6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0):
        set_bias(bias)
        for _ in range(3):
            comp.capture_scene(); time.sleep(0.12)
        lum, ground, pts = creature_and_ground()
        glum = 0.2126 * ground[0] + 0.7152 * ground[1] + 0.0722 * ground[2]
        ok = 95.0 <= glum <= 150.0
        rows.append({"bias_ev": bias, "ground_luminance": round(glum, 1), "creature_luminance": round(lum, 1), "in_range": ok})
        if ok and chosen is None:
            chosen = bias
    if chosen is None:
        chosen = min(rows, key=lambda r: abs(r["ground_luminance"] - 120.0))["bias_ev"]
        rows.append({"note": "no bias landed in range; nearest to ground luminance 120 chosen"})
    set_bias(chosen)
    return {"bracket": rows, "chosen_bias_ev": chosen}

def ready():
    """Gate on the property the capture exists to show: the t000 and t040 instances must render
    DIFFERENTLY on the creature. NOT t000 vs t080: the window term swept x (1 - progress) is zero at
    BOTH ends by design, so those two are identical in material and differ only in geometry; a gate
    on them waited out its full timeout once. A material whose textures are still compiling renders
    the default material - identical across instances - which an exposure-dependent colour window
    let through."""
    aim(520.0, -18.0)
    deadline = time.time() + 240.0
    polls = 0
    history = []
    while time.time() < deadline:
        show("baseline", 0)
        for _ in range(2):
            comp.capture_scene(); time.sleep(0.1)
        l0, g0, p0 = creature_and_ground()
        show("baseline", 40)
        for _ in range(2):
            comp.capture_scene(); time.sleep(0.1)
        l80, g80, p80 = creature_and_ground()  # the mid-window instance
        glum = 0.2126 * g0[0] + 0.7152 * g0[1] + 0.0722 * g0[2]
        polls += 1
        differs = abs(l80 - l0) > 6.0
        history.append([round(l0, 1), round(l80, 1), round(glum, 1)])
        if differs:
            return {"ready": True, "polls": polls, "t000_creature": round(l0, 1), "t040_creature": round(l80, 1),
                    "ground": round(glum, 1), "body_over_ground": round(l0 / max(glum, 1.0), 2),
                    "seconds": round(time.time() - (deadline - 480.0), 1)}
        time.sleep(3.0)
    return {"ready": False, "polls": polls, "history_tail": history[-5:],
            "note": "the t000 and t040 instances never rendered differently"}

report["texture_resident_size_before_capture"] = {k: [t.blueprint_get_size_x(), t.blueprint_get_size_y()] for k, t in textures.items()}
report["exposure"] = bracket_exposure()
report["readiness"] = ready()

for state in STATES:
    if state not in actors:
        continue
    for label, arm, pitch in (("tactical", 1400.0, -55.0), ("close", 520.0, -18.0)):
        for tick in TICKS:
            c = show(state, tick)
            aim(arm, pitch)
            for _ in range(3):
                comp.capture_scene(); time.sleep(0.15)
            name = f"art_{state}_{label}_t{tick:03d}.png"
            unreal.RenderingLibrary.export_render_target(world, rt, OUT, name)
            report["frames"].append({"file": name, "state": state, "tick": tick, "framing": label,
                                     "material": c.get_material(0).get_name()})
json.dump(report, open(os.path.join(OUT, "art-capture-report.json"), "w"), indent=1)
print("[EBS_ART] " + json.dumps({"frames": len(report["frames"]), "errors": report["errors"],
                                 "exposure": report["exposure"]["chosen_bias_ev"], "readiness": report["readiness"]}))
