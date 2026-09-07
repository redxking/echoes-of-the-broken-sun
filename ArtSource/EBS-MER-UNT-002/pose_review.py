#!/usr/bin/env python3
"""Bake key poses of the Lancer clips into review OBJs (rest-stance mesh + skeletal kit pose_mesh).

Author: Angelis Pseftis. Usage: python3 pose_review.py --evidence-dir <root>/EBS-MER-UNT-002
Samples each listed clip at an absolute time (linear interpolation between keyframes, the same
interpolation glTF samplers use) and writes review/pose_<label>.obj with the parts posed by forward
kinematics, plus the render scene for each. These are stills for the review renderer, not animation
evidence. The fire samples are named for the canon phases (halt, plant, aim, shot, recover).
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
import build_lancer as bl  # noqa: E402

# (clip, time_s, label, also render at the runtime PresentationScale)
SAMPLES = [
    ("idle", 1.2, "idle", False),
    ("move", 0.00, "move_contact", False),
    ("move", 0.15, "move_stance", True),
    ("move", 0.30, "move_pass", False),
    ("move", 0.45, "move_swing", False),
    ("turn", 0.15, "turn_lift", False),
    ("stop", 0.00, "stop_travel", False),
    ("stop", 0.20, "stop_plant", False),
    ("fire", 0.00, "fire_halt", False),
    ("fire", 0.35, "fire_plant", True),
    ("fire", 0.85, "fire_aim", True),
    ("fire", 1.00, "fire_shot", True),
    ("fire", 1.45, "fire_recover", False),
    ("damage", 0.10, "damage", False),
    ("death", 0.50, "death_fold", False),
    ("death", 1.40, "death_hold", True),
    ("cancel", 0.00, "cancel_start", False),
]


def sample(clip: skel.AnimationClip, t: float) -> dict:
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
                pose[bone] = tuple(a.rotation_deg[i] + (b.rotation_deg[i] - a.rotation_deg[i]) * f for i in range(3)) + \
                             tuple(a.translation_cm[i] + (b.translation_cm[i] - a.translation_cm[i]) * f for i in range(3))
                break
    return pose


POSE_VIEWS = [
    {"name": "right", "type": "ortho", "from": "+Y", "edges": True, "margin": 1.12, "target": bl.ORTHO_TARGET},
    {"name": "front", "type": "ortho", "from": "+X", "edges": True, "margin": 1.15, "target": bl.ORTHO_TARGET},
    {"name": "top", "type": "ortho", "from": "+Z", "image_up": "+X", "edges": True, "margin": 1.2, "target": [50, 0, 0]},
    {"name": "tactical_near", "type": "persp", "pitch_deg": -48, "yaw_deg": -45, "arm_cm": 1400, "fov_deg": 55, "target": [0, 0, 60]},
]
TACTICAL_VIEWS = [
    {"name": "tactical_gameplay", "type": "persp", "pitch_deg": -60, "yaw_deg": -45, "arm_cm": 3800, "fov_deg": 55, "target": [0, 0, 0]},
    {"name": "tactical_near", "type": "persp", "pitch_deg": -48, "yaw_deg": -45, "arm_cm": 1400, "fov_deg": 55, "target": [0, 0, 60]},
]


def bake(evidence_dir: str) -> list:
    """Write every posed review OBJ and its render scene under ``evidence_dir`` and return the record
    (one entry per sample, each carrying the OBJ sha256, the bounds, the posed sockets and the scene
    paths). Called by ``main`` and, into a temporary directory, by ``build_lancer.py --check`` so the
    reproducibility gate covers the posed bake and the pose scenes, not only the exports (README §7)."""
    review = os.path.join(evidence_dir, "review")
    scenes = os.path.join(evidence_dir, "scenes")
    os.makedirs(review, exist_ok=True)
    os.makedirs(scenes, exist_ok=True)
    mesh, skeleton, _counts, sockets_on_bones = bl.assemble(0)
    clips = {c.name: c for c in bl.build_clips(skeleton)}
    written = []
    for name, t, label, tactical in SAMPLES:
        pose = sample(clips[name], t)
        pose["_sockets"] = sockets_on_bones
        posed = skel.pose_mesh(mesh, skeleton, pose)
        stem = f"pose_{label}"
        path = os.path.join(review, f"{stem}.obj")
        digest = posed.write_obj(path, header_lines=[f"Posed still: clip {name} at t = {t:.2f} s ({label})"])
        (x0, y0, z0), (x1, y1, z1) = posed.bounds()
        entry = {"clip": name, "time_s": t, "label": label, "path": path, "sha256": digest,
                 "bounds_cm": [[round(x0, 1), round(y0, 1), round(z0, 1)], [round(x1, 1), round(y1, 1), round(z1, 1)]],
                 "sockets": [{"name": s.name, "position_cm": [round(c, 1) for c in s.position],
                              "rotation_deg": [round(c, 1) for c in getattr(s, "rotation_deg", (0.0, s.yaw_deg, 0.0))]} for s in posed.sockets]}
        doc = bl.scene([{"obj": f"../review/{stem}.obj"}], POSE_VIEWS)
        with open(os.path.join(scenes, f"{stem}.json"), "w", encoding="utf-8", newline="\n") as handle:
            json.dump(doc, handle, indent=1)
        entry["scene"] = os.path.join(scenes, f"{stem}.json")
        if tactical:
            doc = bl.scene([{"obj": f"../review/{stem}.obj", "scale": bl.PRESENTATION_SCALE}], TACTICAL_VIEWS)
            with open(os.path.join(scenes, f"{stem}_tactical.json"), "w", encoding="utf-8", newline="\n") as handle:
                json.dump(doc, handle, indent=1)
            entry["scene_tactical"] = os.path.join(scenes, f"{stem}_tactical.json")
        written.append(entry)
    return written


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--evidence-dir", required=True)
    args = parser.parse_args()
    written = bake(args.evidence_dir)
    out = os.path.join(args.evidence_dir, "review", "pose-manifest.json")
    with open(out, "w", encoding="utf-8", newline="\n") as handle:
        json.dump({"author": "Angelis Pseftis", "revision": bl.REVISION, "poses": written}, handle, indent=1)
    print(json.dumps([(w["label"], w["bounds_cm"][0][2], w["bounds_cm"][1][2]) for w in written]))
    return 0


if __name__ == "__main__":
    sys.exit(main())
