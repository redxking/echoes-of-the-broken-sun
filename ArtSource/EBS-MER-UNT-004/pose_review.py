#!/usr/bin/env python3
"""Bake key poses of the Relay Skiff clips into review OBJs and their render scenes.

Author: Angelis Pseftis.
Usage: python3 pose_review.py --evidence-dir <evidence root>/EBS-MER-UNT-004

Samples each listed clip at a normalized time with the linear interpolation the glTF samplers use and
writes review/pose_<clip>_<t>.obj with the parts posed by forward kinematics (ebs_skelkit.pose_mesh),
plus scenes/pose_<clip>_<t>.json (right / front / near tactical) and, for the state poses that carry the
game read, a _tactical scene at the game's orthographic framing. The death pose is also baked with the
archive cradle hidden, the state the runtime shows when nothing is bound. Stills for review, not
animation evidence.
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
import build_relay_skiff as rs  # noqa: E402

SAMPLES = [("idle", 0.25), ("move", 0.5), ("turn", 0.25), ("stop", 0.4), ("relay_extend", 1.0), ("relay_hold", 0.5),
           ("relay_expiry", 0.7), ("attack_anticipation", 1.0), ("attack_execution", 0.3), ("attack_recovery", 0.5),
           ("damage", 0.33), ("death", 1.0), ("cancel", 0.0)]
TACTICAL_SAMPLES = {("relay_extend", 1.0), ("death", 1.0), ("move", 0.5)}
UNLOADED_SAMPLES = {("death", 1.0), ("relay_extend", 1.0)}


def sample(clip: skel.AnimationClip, fraction: float) -> dict:
    """Pose dict at `fraction` of the clip duration (linear between keys, as the glTF samplers do)."""
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
    scenes_dir = os.path.join(args.evidence_dir, "scenes")
    os.makedirs(review, exist_ok=True)
    os.makedirs(scenes_dir, exist_ok=True)
    mesh, skeleton, _counts, sockets_on_bones = rs.assemble(0)
    clips = {c.name: c for c in rs.build_clips(skeleton)}
    pose_views = [
        {"name": "right", "type": "ortho", "from": "+Y", "edges": True, "margin": 1.12, "target": [0, 0, 110]},
        {"name": "front", "type": "ortho", "from": "+X", "edges": True, "margin": 1.15, "target": [0, 0, 110]},
        {"name": "tactical_near", "type": "persp", "pitch_deg": -48, "yaw_deg": -45, "arm_cm": 1400, "fov_deg": 55, "target": [0, 0, 60]},
    ]
    written = []
    for name, fraction in SAMPLES:
        pose = sample(clips[name], fraction)
        pose["_sockets"] = sockets_on_bones
        posed = skel.pose_mesh(mesh, skeleton, pose)
        stem = f"pose_{name}_{int(round(fraction * 100)):03d}"
        path = os.path.join(review, f"{stem}.obj")
        digest = posed.write_obj(path, header_lines=[f"Posed still: clip {name} at {fraction:.2f} of its duration"])
        (x0, y0, z0), (x1, y1, z1) = posed.bounds()
        record = {"clip": name, "fraction": fraction, "path": path, "sha256": digest,
                  "bounds_cm": [[round(x0, 2), round(y0, 2), round(z0, 2)], [round(x1, 2), round(y1, 2), round(z1, 2)]],
                  "lowest_vertex_cm": round(z0, 2),
                  "sockets": [{"name": s.name, "position_cm": [round(c, 2) for c in s.position],
                               "rotation_deg": [round(c, 2) for c in getattr(s, "rotation_deg", (0.0, s.yaw_deg, 0.0))]} for s in posed.sockets]}
        scene = rs.scene([{"obj": f"../review/{stem}.obj"}], pose_views)
        with open(os.path.join(scenes_dir, f"{stem}.json"), "w", encoding="utf-8", newline="\n") as handle:
            handle.write(json.dumps(scene, indent=1) + "\n")
        record["scene"] = os.path.join(scenes_dir, f"{stem}.json")
        if (name, fraction) in TACTICAL_SAMPLES:
            tactical = rs.scene([{"obj": f"../review/{stem}.obj"}], [rs.TACTICAL_VIEWS[1], rs.TACTICAL_VIEWS[3], rs.TACTICAL_VIEWS[2]])
            with open(os.path.join(scenes_dir, f"{stem}_tactical.json"), "w", encoding="utf-8", newline="\n") as handle:
                handle.write(json.dumps(tactical, indent=1) + "\n")
            record["scene_tactical"] = os.path.join(scenes_dir, f"{stem}_tactical.json")
        written.append(record)
        if (name, fraction) in UNLOADED_SAMPLES:
            unloaded = rs.unloaded_mesh(posed)
            ustem = f"{stem}_unloaded"
            upath = os.path.join(review, f"{ustem}.obj")
            udigest = unloaded.write_obj(upath, header_lines=[f"Posed still: clip {name} at {fraction:.2f}, archive cradle hidden"])
            (ux0, uy0, uz0), (ux1, uy1, uz1) = unloaded.bounds()
            uscene = rs.scene([{"obj": f"../review/{ustem}.obj"}], pose_views)
            with open(os.path.join(scenes_dir, f"{ustem}.json"), "w", encoding="utf-8", newline="\n") as handle:
                handle.write(json.dumps(uscene, indent=1) + "\n")
            written.append({"clip": name, "fraction": fraction, "path": upath, "sha256": udigest, "state": "unloaded",
                            "bounds_cm": [[round(ux0, 2), round(uy0, 2), round(uz0, 2)], [round(ux1, 2), round(uy1, 2), round(uz1, 2)]],
                            "lowest_vertex_cm": round(uz0, 2), "scene": os.path.join(scenes_dir, f"{ustem}.json")})
    out = os.path.join(review, "pose-manifest.json")
    with open(out, "w", encoding="utf-8", newline="\n") as handle:
        handle.write(json.dumps({"author": "Angelis Pseftis", "revision": rs.REVISION, "poses": written}, indent=1) + "\n")
    print(json.dumps([(w["clip"], w["fraction"], w.get("state", "loaded"), w["lowest_vertex_cm"], w["bounds_cm"][1][2]) for w in written], indent=0))
    return 0


if __name__ == "__main__":
    sys.exit(main())
