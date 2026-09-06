#!/usr/bin/env python3
"""Bake key poses of the Surveyor clips into review OBJs (rest-stance mesh + skeletal kit pose_mesh).

Author: Angelis Pseftis. Usage: python3 pose_review.py --evidence-dir <root>/EBS-MER-UNT-001
Samples each listed clip at a normalized time (linear interpolation between keyframes, the same
interpolation glTF samplers use) and writes review/pose_<clip>_<t>.obj with the parts posed by
forward kinematics. These are stills for the review renderer, not animation evidence.
"""
from __future__ import annotations

import argparse
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(os.path.dirname(HERE), "tools"))
sys.path.insert(0, HERE)
import ebs_skelkit as skel  # noqa: E402
import build_surveyor as sv  # noqa: E402

SAMPLES = [("move", 0.25), ("move", 0.5), ("carry", 0.25), ("gather", 0.5), ("build", 0.5), ("repair_when_authorized", 0.5),
           ("deliver", 0.5), ("damage", 0.33), ("death", 1.0), ("cancel", 0.0), ("turn", 0.25), ("stop", 0.0)]


def sample(clip: skel.AnimationClip, fraction: float) -> dict:
    t = clip.duration_s * fraction
    pose = {}
    for bone, keys in clip.tracks.items():
        if not keys:
            continue
        if t <= keys[0].time_s:
            k = keys[0]
            pose[bone] = (*k.rotation_deg, *k.translation_cm)
            continue
        if t >= keys[-1].time_s:
            k = keys[-1]
            pose[bone] = (*k.rotation_deg, *k.translation_cm)
            continue
        for a, b in zip(keys, keys[1:]):
            if a.time_s <= t <= b.time_s:
                f = 0.0 if b.time_s == a.time_s else (t - a.time_s) / (b.time_s - a.time_s)
                rot = tuple(a.rotation_deg[i] + (b.rotation_deg[i] - a.rotation_deg[i]) * f for i in range(3))
                tr = tuple(a.translation_cm[i] + (b.translation_cm[i] - a.translation_cm[i]) * f for i in range(3))
                pose[bone] = (*rot, *tr)
                break
    return pose


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--evidence-dir", required=True)
    args = parser.parse_args()
    review = os.path.join(args.evidence_dir, "review")
    os.makedirs(review, exist_ok=True)
    mesh, skeleton, _counts, sockets_on_bones = sv.assemble(0)
    clips = {c.name: c for c in sv.build_clips(skeleton)}
    written = []
    for name, fraction in SAMPLES:
        pose = sample(clips[name], fraction)
        pose["_sockets"] = sockets_on_bones
        posed = skel.pose_mesh(mesh, skeleton, pose)
        path = os.path.join(review, f"pose_{name}_{int(fraction * 100):03d}.obj")
        digest = posed.write_obj(path, header_lines=[f"Posed still: clip {name} at {fraction:.2f} of its duration"])
        (x0, y0, z0), (x1, y1, z1) = posed.bounds()
        written.append({"clip": name, "fraction": fraction, "path": path, "sha256": digest, "bounds_cm": [[round(x0, 1), round(y0, 1), round(z0, 1)], [round(x1, 1), round(y1, 1), round(z1, 1)]],
                        "sockets": [{"name": s.name, "position_cm": [round(c, 1) for c in s.position]} for s in posed.sockets]})
    scenes = os.path.join(args.evidence_dir, "scenes")
    base_path = os.path.join(scenes, "rest.json")
    if os.path.exists(base_path):
        with open(base_path, "r", encoding="utf-8") as handle:
            base = json.load(handle)
        for w in written:
            stem = os.path.splitext(os.path.basename(w["path"]))[0]
            scene = {k: v for k, v in base.items() if k not in ("meshes", "views", "reference_figure")}
            scene["reference_figure"] = dict(base.get("reference_figure", {"height_cm": 180, "position": [-160, 160, 0], "color": [0.92, 0.55, 0.2]}))
            scene["meshes"] = [{"obj": f"../review/{stem}.obj"}]
            scene["views"] = [
                {"name": "right", "type": "ortho", "from": "+Y", "edges": True, "margin": 1.15, "target": [20, 0, 90]},
                {"name": "front", "type": "ortho", "from": "+X", "edges": True, "margin": 1.15, "target": [20, 0, 90]},
                {"name": "tactical_near", "type": "persp", "pitch_deg": -48, "yaw_deg": -45, "arm_cm": 1400, "fov_deg": 55, "target": [0, 0, 60]},
            ]
            with open(os.path.join(scenes, f"{stem}.json"), "w", encoding="utf-8") as handle:
                json.dump(scene, handle, indent=1)
            w["scene"] = os.path.join(scenes, f"{stem}.json")
            if w["clip"] in ("gather", "build", "deliver", "death"):
                # the game read: unit drawn at the runtime PresentationScale x1.5 at the orthographic gameplay framing
                tactical = dict(scene)
                tactical["meshes"] = [{"obj": f"../review/{stem}.obj", "scale": 1.5}]
                tactical["views"] = [
                    {"name": "tactical_gameplay", "type": "persp", "pitch_deg": -60, "yaw_deg": -45, "arm_cm": 3800, "fov_deg": 55, "target": [0, 0, 0]},
                    {"name": "tactical_near", "type": "persp", "pitch_deg": -48, "yaw_deg": -45, "arm_cm": 1400, "fov_deg": 55, "target": [0, 0, 60]},
                ]
                with open(os.path.join(scenes, f"{stem}_tactical.json"), "w", encoding="utf-8") as handle:
                    json.dump(tactical, handle, indent=1)
                w["scene_tactical"] = os.path.join(scenes, f"{stem}_tactical.json")
    out = os.path.join(review, "pose-manifest.json")
    with open(out, "w", encoding="utf-8") as handle:
        json.dump({"author": "Angelis Pseftis", "poses": written}, handle, indent=1)
    print(json.dumps([(w["clip"], w["fraction"], w["bounds_cm"][0][2], w["bounds_cm"][1][2]) for w in written]))
    return 0


if __name__ == "__main__":
    sys.exit(main())
