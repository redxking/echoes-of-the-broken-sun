#!/usr/bin/env python3
"""Write every EBS-MER-UNT-003 review scene, with the exact ebs_render.py commands recorded.

Author: Angelis Pseftis.
Usage: python3 make_scenes.py --evidence-dir <root>/EBS-MER-UNT-003 [--render] [--dry-run]

Four review assemblies are rendered: the shipped `deployed` and `packed` meshes, plus two baked
material states of the SAME geometry - `packed_field_dark` (the field and the pane rims re-slotted
to charcoal: the runtime's packed state, nothing removed) and `deployed_glass_cutaway` (the field
polygons cut, a stand-in for translucency so the operators read through the pane as the reference
draws them). Every fidelity check is judged on a shipped mesh; the two states are supporting tiles.

Orthographic front/right/rear/left/top at authored scale with a 180 cm reference figure, the
project's RTS tactical framing (the view blocks are copied verbatim from
EBS-MER-UNT-001/scenes/rest_tactical.json, which reproduces AEchoesRTSCameraPawn), the packed and
deployed states, every clip key pose, a LOD1 sheet and a context scene. Tactical scenes are written
twice: at unit scale 1.0 (the authored asset) and at PresentationScale 1.75, the value
EchoesEntityView.cpp:1817 draws EntityType::HeavyUnit at (README section 8).
Standard library only.
"""
from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
RENDER = os.path.join(os.path.dirname(HERE), "tools", "ebs_render.py")
ASSET = "SK_EBS_MER_UNT_003"
PRESENTATION_SCALE = 1.75

MATERIALS = {
    "MI_EBS_MER_UnitFrame": [0.05, 0.05, 0.055],
    "MI_EBS_MER_UnitCeramic": [0.72, 0.70, 0.66],
    "MI_EBS_MER_ShieldField": [0.44, 0.64, 0.79],
    "MI_EBS_MER_StatusCyan": [0.16, 0.86, 0.96],
    "MI_EBS_MER_CeramicCivic": [0.72, 0.70, 0.66],
    "MI_EBS_MER_CompactFrame": [0.045, 0.045, 0.05],
    "_default": [0.5, 0.5, 0.5],
}
LIGHT = {"direction": [0.55, -0.35, -0.76], "ambient": 0.3, "key": 0.8,
         "color": [1.0, 0.82, 0.62], "fill_color": [0.48, 0.6, 0.88], "fill": 0.22}
GROUND = {"tile_cm": 200, "tiles": 40, "color": [0.028, 0.028, 0.032], "grid_color": [0.07, 0.07, 0.08],
          "footprint_cm": [400, 400], "footprint_color": [0.16, 0.86, 0.96]}
FIGURE = {"height_cm": 180, "position": [300, 330, 0], "color": [0.92, 0.55, 0.2]}
TARGET = [20, 0, 210]

ORTHO_VIEWS = [
    {"name": "front", "type": "ortho", "from": "+X", "edges": True, "margin": 1.12, "target": TARGET},
    {"name": "right", "type": "ortho", "from": "+Y", "edges": True, "margin": 1.12, "target": TARGET},
    {"name": "rear", "type": "ortho", "from": "-X", "edges": True, "margin": 1.12, "target": TARGET},
    {"name": "left", "type": "ortho", "from": "-Y", "edges": True, "margin": 1.12, "target": TARGET},
    {"name": "top", "type": "ortho", "from": "+Z", "image_up": "+X", "edges": True, "margin": 1.2, "target": [20, 0, 0]},
]
# Copied verbatim from EBS-MER-UNT-001/scenes/rest_tactical.json (AEchoesRTSCameraPawn framing).
TACTICAL_VIEWS = [
    {"name": "tactical_default", "type": "persp", "pitch_deg": -48, "yaw_deg": -45, "arm_cm": 3800, "fov_deg": 55, "target": [0, 0, 0]},
    {"name": "tactical_gameplay", "type": "persp", "pitch_deg": -60, "yaw_deg": -45, "arm_cm": 3800, "fov_deg": 55, "target": [0, 0, 0]},
    {"name": "tactical_mono", "type": "persp", "pitch_deg": -60, "yaw_deg": -45, "arm_cm": 3800, "fov_deg": 55, "target": [0, 0, 0], "grayscale": True},
    {"name": "tactical_near", "type": "persp", "pitch_deg": -48, "yaw_deg": -45, "arm_cm": 1400, "fov_deg": 55, "target": [0, 0, 120]},
    {"name": "tactical_front_quarter", "type": "persp", "pitch_deg": -34, "yaw_deg": 150, "arm_cm": 1700, "fov_deg": 55, "target": [0, 0, 150]},
]
# The concept panels are three-quarter views from the front-left and the rear-left; these match them.
CONCEPT_VIEWS = [
    {"name": "concept_travel", "type": "persp", "pitch_deg": -22, "yaw_deg": 152, "arm_cm": 1050, "fov_deg": 42, "target": [0, 0, 170]},
    {"name": "concept_rear", "type": "persp", "pitch_deg": -20, "yaw_deg": -28, "arm_cm": 1050, "fov_deg": 42, "target": [0, 0, 170]},
]

POSES = ["pose_idle_packed_000", "pose_move_packed_025", "pose_deploy_015", "pose_deploy_055", "pose_deploy_082",
         "pose_idle_deployed_050", "pose_drag_deployed_025", "pose_pack_055", "pose_damage_020",
         "pose_death_100", "pose_cancel_000", "pose_restore_000"]


def scene(meshes: list, views: list, figure: bool = True) -> dict:
    out = {"author": "Angelis Pseftis", "srgb": True, "materials": MATERIALS,
           "emissive": ["MI_EBS_MER_StatusCyan"], "width": 1920, "height": 1080,
           "background": [0.84, 0.82, 0.78], "light": LIGHT, "ground": GROUND}
    if figure:
        out["reference_figure"] = FIGURE
    out["meshes"] = meshes
    out["views"] = views
    return out


def plan(evidence_dir: str) -> list:
    scenes = []
    for state in ("deployed", "packed", "deployed_glass_cutaway", "packed_field_dark"):
        obj = f"../review/{ASSET}_{state}_LOD0.obj"
        scenes.append((state, scene([{"obj": obj}], ORTHO_VIEWS)))
        scenes.append((f"{state}_tactical", scene([{"obj": obj}], TACTICAL_VIEWS + CONCEPT_VIEWS)))
        if state in ("deployed", "packed"):
            scenes.append((f"{state}_tactical_x{int(PRESENTATION_SCALE * 100)}",
                           scene([{"obj": obj, "scale": PRESENTATION_SCALE}], TACTICAL_VIEWS)))
            scenes.append((f"{state}_lod1", scene([{"obj": f"../review/{ASSET}_{state}_LOD1.obj"}], ORTHO_VIEWS)))
    for stem in POSES:
        views = [ORTHO_VIEWS[0], ORTHO_VIEWS[1], ORTHO_VIEWS[2], TACTICAL_VIEWS[3], CONCEPT_VIEWS[0]]
        scenes.append((stem, scene([{"obj": f"../review/{stem}.obj"}], views)))
    # context: a deployed screen with two packed teammates behind it, at the gameplay framing
    context = scene([
        {"obj": f"../review/{ASSET}_deployed_LOD0.obj", "translate": [0, 0, 0], "yaw_deg": 0},
        {"obj": f"../review/{ASSET}_packed_LOD0.obj", "translate": [-560, -380, 0], "yaw_deg": 12},
        {"obj": f"../review/{ASSET}_packed_LOD0.obj", "translate": [-520, 420, 0], "yaw_deg": -18},
        {"obj": "../../EBS-MER-UNT-001/review/SK_EBS_MER_UNT_001_rest_LOD0.obj", "translate": [-820, 60, 0], "yaw_deg": 30, "scale": 1.5},
        {"obj": "../../EBS-MER-UNT-001/review/SK_EBS_MER_UNT_001_rest_LOD0.obj", "translate": [-900, -180, 0], "yaw_deg": -20, "scale": 1.5},
    ], [TACTICAL_VIEWS[1], TACTICAL_VIEWS[0], TACTICAL_VIEWS[2],
        {"name": "tactical_near", "type": "persp", "pitch_deg": -48, "yaw_deg": -45, "arm_cm": 2100, "fov_deg": 55, "target": [-420, 0, 100]},
        {"name": "tactical_flank", "type": "persp", "pitch_deg": -48, "yaw_deg": 45, "arm_cm": 2600, "fov_deg": 55, "target": [-380, 0, 60]}])
    context["ground"] = dict(GROUND, footprint_cm=[400, 400])
    scenes.append(("context", context))
    return scenes


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--evidence-dir", required=True)
    parser.add_argument("--render", action="store_true")
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument("--only", default=None, help="comma-separated scene names to render")
    args = parser.parse_args()
    scenes_dir = os.path.join(args.evidence_dir, "scenes")
    renders_dir = os.path.join(args.evidence_dir, "renders")
    os.makedirs(scenes_dir, exist_ok=True)
    os.makedirs(renders_dir, exist_ok=True)
    only = set(args.only.split(",")) if args.only else None
    written = []
    for name, data in plan(args.evidence_dir):
        path = os.path.join(scenes_dir, f"{name}.json")
        with open(path, "w", encoding="utf-8", newline="\n") as handle:
            handle.write(json.dumps(data, indent=1) + "\n")
        written.append(name)
        if only and name not in only:
            continue
        cmd = [sys.executable, RENDER, "--scene", path, "--out", os.path.join(renders_dir, name)]
        print(" ".join(f'"{c}"' if " " in c else c for c in cmd))
        if args.render and not args.dry_run:
            subprocess.run(cmd, check=True)
    print(json.dumps({"scenes": len(written), "names": written}))
    return 0


if __name__ == "__main__":
    sys.exit(main())
