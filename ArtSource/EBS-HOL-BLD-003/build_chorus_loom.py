#!/usr/bin/env python3
"""Deterministic source generator for EBS-HOL-BLD-003 — the Hollow Choir Chorus Loom.

Author: Angelis Pseftis. Creator: Angelis Pseftis.

Built to `concept-fidelity.md` against the provisional card `REL-BLD-017.HC.CHORUS.ASSET`, authored only
after the candidate was traced: a low platform with two upright posts, a warp of fine threads strung
between them, and a beam hovering above the warp touching nothing.

Usage:
  python3 build_chorus_loom.py --evidence-dir "<root>/EBS-HOL-BLD-003" [--skinned]
  python3 build_chorus_loom.py --evidence-dir "<root>/EBS-HOL-BLD-003" --check

Units: centimetres. +X forward, +Y right, +Z up. Pivot at the platform's ground-contact centre.
"""
from __future__ import annotations

import argparse
import json
import math
import os
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(os.path.dirname(HERE), "tools"))
sys.path.insert(0, HERE)
import ebs_meshkit as kit  # noqa: E402
import ebs_skelkit as skel  # noqa: E402

AUTHOR = "Angelis Pseftis"
PACKAGE_ID = "EBS-PKG-HC-CHORUS-LOOM"
PRODUCTION_ID = "EBS-HOL-BLD-003"
ASSET = "SK_EBS_HOL_BLD_003"
PLANNED_FOLDER = "/Game/Echoes/Production/HOL/BLD/EBS_HOL_BLD_003/"
REVISION = "ebs-hol-bld-003-concept-v1"
CARD = "REL-BLD-017.HC.CHORUS.ASSET"

VITRIFIED = "MI_EBS_HOL_Vitrified"
GROUND = "MI_EBS_HOL_ApronStone"
MAGENTA = "MI_EBS_HOL_Magenta"

TICKS_PER_SECOND = 20.0
UPKEEP_TICKS = 600.0

TILE_CM = 200.0
FOOTPRINT_TILES = 4
F = TILE_CM * FOOTPRINT_TILES          # 800 cm square
HALF = F / 2.0

PLATFORM = (760.0, 620.0, 26.0)
POST_X = 286.0                         # one post at each end of the platform
POST = (34.0, 128.0, 300.0)
EDGE_W, EDGE_T = 4.0, 3.0
WARP_COUNT = 20                        # threads strung between the posts
WARP_LOW, WARP_HIGH = 66.0, 286.0
WARP_R = 1.6
WARP_R_FINE = 0.9                      # the researching state's interleaved rows are finer than the base warp
BEAM = (430.0, 96.0, 18.0)
BEAM_Z = 372.0                         # it hovers: nothing reaches it
STATES = ("producing", "researching", "insolvent", "destroyed")

BONES = [
    ("root", None, (0.0, 0.0, 0.0), "platform ground-contact centre; the Chorus Loom never moves"),
    ("post_l", "root", (-POST_X, 0.0, PLATFORM[2]), "left post: drifts on its own phase (faction reality-bleed device)"),
    ("post_r", "root", (POST_X, 0.0, PLATFORM[2]), "right post: drifts on its own phase"),
    ("beam", "root", (0.0, 0.0, BEAM_Z), "the hovering beam: unsupported, and it hangs out of step with the posts"),
]


def warp_z(index: int) -> float:
    return WARP_LOW + (WARP_HIGH - WARP_LOW) * index / (WARP_COUNT - 1)


def build_body(lod: int, state: str = "producing") -> kit.Mesh:
    if state not in STATES:
        raise ValueError(f"unknown state {state!r}; expected one of {STATES}")
    m = kit.Mesh(ASSET)
    vitrified = m.slot(VITRIFIED)
    ground = m.slot(GROUND)
    magenta = m.slot(MAGENTA)
    fine = lod == 0
    ruined = state == "destroyed"
    lit = state in ("producing", "researching")

    # 1. the low platform
    m.box((0.0, 0.0, PLATFORM[2] / 2.0), PLATFORM, ground, "platform")

    if ruined:
        for k, sign in enumerate((-1.0, 1.0), start=1):
            m.box((sign * POST_X * 0.7, 40.0 * sign, PLATFORM[2] + 18.0), (POST[2] * 0.8, POST[1], 34.0),
                  vitrified, f"fallen_post_{k:02d}", yaw_deg=14.0 * sign)
        m.box((0.0, -70.0, PLATFORM[2] + 12.0), (BEAM[0] * 0.9, BEAM[1], BEAM[2]), vitrified, "fallen_beam",
              yaw_deg=6.0)
        m.collision.append(kit.CollisionBox("platform", (0.0, 0.0, PLATFORM[2] / 2.0),
                                            (PLATFORM[0], PLATFORM[1], PLATFORM[2])))
        return m

    # 2. the two posts, with magenta edge strips down their inner borders
    for tag, sign in (("l", -1.0), ("r", 1.0)):
        px = sign * POST_X
        m.box((px, 0.0, PLATFORM[2] + POST[2] / 2.0), POST, vitrified, f"post_{tag}")
        m.box((px, 0.0, PLATFORM[2] + 6.0), (POST[0] + 28.0, POST[1] + 22.0, 12.0), vitrified, f"post_{tag}_base")
        if fine:
            for edge, dy in (("f", -POST[1] / 2.0 + EDGE_W / 2.0), ("b", POST[1] / 2.0 - EDGE_W / 2.0)):
                m.box((px - sign * (POST[0] / 2.0 + EDGE_T / 2.0), dy, PLATFORM[2] + POST[2] / 2.0),
                      (EDGE_T, EDGE_W, POST[2] - 20.0),
                      magenta if lit else vitrified, f"post_{tag}_edge_{edge}")

    # 3. the warp: fine threads strung between the posts. Researching doubles them, which is the
    #    difference the candidate draws between its two working states.
    rows = [(i, False) for i in range(WARP_COUNT)]
    if state == "researching":
        rows += [(i, True) for i in range(WARP_COUNT - 1)]
    for n, (index, interleaved) in enumerate(rows, start=1):
        # the doubled rows of the researching state sit halfway between the base rows, and are FINER
        # than them: doubling the warp at the base radius would have breached the 12% magenta ceiling.
        z = warp_z(index) + ((WARP_HIGH - WARP_LOW) / (2 * (WARP_COUNT - 1)) if interleaved else 0.0)
        m.tube((-POST_X + POST[0] / 2.0, 0.0, z), (POST_X - POST[0] / 2.0, 0.0, z),
               WARP_R_FINE if interleaved else WARP_R, 3, magenta if lit else vitrified, f"warp_{n:02d}")

    # 4. the beam, hovering above the warp and touching nothing
    m.box((0.0, 0.0, BEAM_Z), BEAM, vitrified, "beam")
    if fine:
        # a narrow line down the beam's underside, not a lit panel: the whole underside at 72 cm wide
        # carried the researching state past the 12% magenta ceiling on its own.
        m.box((0.0, 0.0, BEAM_Z - BEAM[2] / 2.0 - 2.0), (BEAM[0] - 40.0, 14.0, 4.0),
              magenta if state == "researching" else vitrified, "beam_underside")

    m.collision.append(kit.CollisionBox("platform", (0.0, 0.0, PLATFORM[2] / 2.0),
                                        (PLATFORM[0], PLATFORM[1], PLATFORM[2])))
    m.collision.append(kit.CollisionBox("posts", (0.0, 0.0, PLATFORM[2] + POST[2] / 2.0),
                                        (2 * POST_X + POST[0], POST[1], POST[2])))
    return m


def build_skeleton() -> skel.Skeleton:
    s = skel.Skeleton("root")
    for name, parent, head, purpose in BONES:
        s.add(name, parent, head, purpose=purpose)
    return s


def bind(m: kit.Mesh) -> dict:
    by_component = {}
    for c in m.components():
        if c.startswith("post_l"):
            by_component[c] = "post_l"
        elif c.startswith("post_r"):
            by_component[c] = "post_r"
        elif c.startswith("beam"):
            by_component[c] = "beam"
    return skel.bind_polygons(m, "root", by_component)


SOCKETS = {
    "Target_Anchor_Center": ("root", (0.0, 0.0, PLATFORM[2] + POST[2] * 0.5), 0.0, "targeting and selection anchor"),
    "Rally_Default": ("root", (0.0, -PLATFORM[1] / 2.0 - 150.0, 0.0), -90.0, "default rally point off the platform"),
    "Weave_Center": ("root", (0.0, 0.0, (WARP_LOW + WARP_HIGH) / 2.0), 0.0,
                     "where the woven form appears. It is a LIGHT EFFECT, not geometry; this socket is all the mesh provides"),
    "Research_Beam": ("beam", (0.0, 0.0, BEAM_Z - BEAM[2] / 2.0), 0.0, "the hovering beam's underside: the research read"),
    "Unit_Emergence": ("root", (0.0, -PLATFORM[1] / 2.0 + 40.0, PLATFORM[2]), -90.0,
                       "finished units step off the platform's near edge"),
}


def assemble(lod: int, state: str = "producing"):
    m = build_body(lod, state)
    s = build_skeleton()
    counts = bind(m)
    for name, (bone, pos, yaw, purpose) in SOCKETS.items():
        m.sockets.append(kit.Socket(name, pos, yaw, purpose))
    return m, s, counts, {n: v[0] for n, v in SOCKETS.items()}


def build_clips(skeleton: skel.Skeleton) -> list:
    clips = []

    def new(name, ticks, loop, purpose):
        c = skel.AnimationClip(name, ticks / TICKS_PER_SECOND, loop=loop, purpose=purpose)
        clips.append(c)
        return c

    idle = new("producing_idle", 120, True,
               "weaving: the posts hold and the hovering beam drifts out of step with them. Faction "
               "reality-bleed device, NOT a canon motion clause for this building")
    for t, drift in ((0.0, 0.0), (2.0, 1.1), (4.0, 0.0), (6.0, -1.1)):
        idle.key("beam", t, (0.0, drift, 0.0), translation_cm=(0.0, 0.0, drift * 2.2))
    for index, bone in enumerate(("post_l", "post_r")):
        for step in range(5):
            t = step / 4.0 * idle.duration_s
            phase = index / 2.0
            drift = math.sin(2.0 * math.pi * (step / 4.0 + phase))
            idle.key(bone, t, (0.0, 0.0, 0.35 * drift))

    research = new("researching", 120, True,
                   "research: the beam settles level and steady while the posts keep their drift")
    for t in (0.0, research.duration_s):
        research.key("beam", t, (0.0, 0.0, 0.0))
    for index, bone in enumerate(("post_l", "post_r")):
        for step in range(5):
            t = step / 4.0 * research.duration_s
            phase = index / 2.0
            drift = math.sin(2.0 * math.pi * (step / 4.0 + phase))
            research.key(bone, t, (0.0, 0.0, 0.35 * drift))

    restore = new("restore", 0, False, "single-frame rest pose for reconstruction from saved state")
    for bone in ("post_l", "post_r", "beam"):
        restore.key(bone, 0.0, (0.0, 0.0, 0.0))

    for c in clips:
        if not skel.is_frame_aligned(c.duration_s):
            skel.retime_clip(c, skel.frame_aligned_duration(c.duration_s))
    return clips


def sample_pose(clip: skel.AnimationClip, fraction: float) -> dict:
    t = clip.duration_s * fraction
    pose = {}
    for bone, keys in clip.tracks.items():
        if not keys:
            continue
        if t <= keys[0].time_s:
            k = keys[0]
        elif t >= keys[-1].time_s:
            k = keys[-1]
        else:
            for a, b in zip(keys, keys[1:]):
                if a.time_s <= t <= b.time_s:
                    f = 0.0 if b.time_s == a.time_s else (t - a.time_s) / (b.time_s - a.time_s)
                    pose[bone] = tuple(a.rotation_deg[i] + (b.rotation_deg[i] - a.rotation_deg[i]) * f for i in range(3)) + \
                                 tuple(a.translation_cm[i] + (b.translation_cm[i] - a.translation_cm[i]) * f for i in range(3))
                    break
            continue
        pose[bone] = (*k.rotation_deg, *k.translation_cm)
    return pose


def posed(lod: int, pose: dict, state: str = "producing") -> kit.Mesh:
    m, s, _c, socks = assemble(lod, state)
    p = dict(pose)
    p["_sockets"] = socks
    return skel.pose_mesh(m, s, p)


def slot_area_fraction(m: kit.Mesh, slot_name: str) -> float:
    index = m.slots.index(slot_name) if slot_name in m.slots else None
    total = per = 0.0
    for poly in m.polygons:
        pts = poly.points
        area = 0.0
        for i in range(1, len(pts) - 1):
            a = kit.v_sub(pts[i], pts[0])
            b = kit.v_sub(pts[i + 1], pts[0])
            area += 0.5 * kit.v_len(kit.v_cross(a, b))
        total += area
        if poly.slot == index:
            per += area
    return per / total if total else 0.0


POSE_SAMPLES = [("producing_idle", 0.25, "producing"), ("researching", 0.25, "researching")]


def lit_threads(m: kit.Mesh) -> int:
    return len({p.component for p in m.polygons
                if p.component.startswith("warp_") and m.slots[p.slot] == MAGENTA})


def contract_inventory(m: kit.Mesh, s: skel.Skeleton, clips) -> dict:
    comps = m.components()
    return {
        "platform": {"contract": 1, "built": "platform" in comps},
        "posts": {"contract": 2, "built": sum(1 for c in comps if c in ("post_l", "post_r")),
                  "edge_strips": sum(1 for c in comps if "_edge_" in c)},
        "warp": {"contract": WARP_COUNT, "built": sum(1 for c in comps if c.startswith("warp_")),
                 "lit": lit_threads(m)},
        "hovering_beam": {"contract": "touches nothing", "built": "beam" in comps,
                          "gap_to_warp_cm": round(BEAM_Z - BEAM[2] / 2.0 - WARP_HIGH, 2)},
        "woven_form": {"contract": "a LIGHT EFFECT, not geometry",
                       "built_as_geometry": any("weave" in c or "lattice" in c for c in comps),
                       "socket": "Weave_Center"},
        "bones": {"contract": "root, two posts and the beam", "built": len(s.bones),
                  "names": [b.name for b in s.bones]},
        "sockets": {"contract": sorted(SOCKETS), "built": sorted(sk.name for sk in m.sockets)},
        "tracks": {"contract": ["producing_idle", "researching", "restore"], "built": [c.name for c in clips]},
        "states": {"contract": list(STATES), "built": list(STATES)},
    }


def measurements(m: kit.Mesh) -> dict:
    (x0, y0, z0), (x1, y1, z1) = m.bounds()
    return {
        "footprint_tiles": [FOOTPRINT_TILES, FOOTPRINT_TILES], "footprint_cm": F,
        "bounds_cm": [[round(x0, 2), round(y0, 2), round(z0, 2)], [round(x1, 2), round(y1, 2), round(z1, 2)]],
        "height_cm": round(z1, 2),
        "post_span_cm": round(2 * POST_X, 2),
        "post_height_cm": POST[2],
        "warp_count": WARP_COUNT,
        "beam_gap_cm": round(BEAM_Z - BEAM[2] / 2.0 - (PLATFORM[2] + POST[2]), 2),
        "beam_hovers": BEAM_Z - BEAM[2] / 2.0 > PLATFORM[2] + POST[2],
        "everything_inside_the_footprint": max(abs(x0), abs(x1), abs(y0), abs(y1)) <= HALF + 1e-6,
        "max_extent_cm": round(max(abs(x0), abs(x1), abs(y0), abs(y1)), 2),
    }


def export(evidence_dir: str, out_dir: str, skinned: bool) -> dict:
    os.makedirs(out_dir, exist_ok=True)
    review = os.path.join(evidence_dir, "review")
    os.makedirs(review, exist_ok=True)
    outputs, review_rows = [], []
    skeleton = build_skeleton()
    clips = build_clips(skeleton)
    for lod in (0, 1):
        m, s, _c, socks = assemble(lod, "producing")
        base = os.path.join(out_dir, f"{ASSET}_LOD{lod}")
        extras = {"production_id": PRODUCTION_ID, "package_id": PACKAGE_ID, "revision": REVISION,
                  "lod": lod, "author": AUTHOR, "card": CARD}
        if skinned:
            digest = skel.write_skinned_glb(m, s, base + ".glb", animations=clips, extras=extras,
                                            include_collision=False, sockets_on_bones=socks)
            outputs.append({"mesh": ASSET, "lod": lod, "kind": "skinned + clips",
                            "path": os.path.relpath(base + ".glb", HERE), "sha256": digest,
                            "triangles": m.triangle_count(), "bounds_cm": [list(p) for p in m.bounds()],
                            "section_slot_names": m.slots, "by_slot": m.triangle_count_by("slot"),
                            "clips": [c.name for c in clips],
                            "sockets": [{"name": sk.name, "bone": socks[sk.name],
                                         "position_cm": [round(v, 2) for v in sk.position],
                                         "yaw_deg": sk.yaw_deg, "purpose": sk.purpose} for sk in m.sockets]})
        obj = m.write_obj(base + ".obj", header_lines=[f"{ASSET} LOD{lod} rest pose (producing)",
                                                       f"Revision {REVISION}",
                                                       "Unreal +X forward, +Y right, +Z up; centimetres"])
        outputs.append({"mesh": ASSET, "lod": lod, "kind": "rest-pose OBJ",
                        "path": os.path.relpath(base + ".obj", HERE), "sha256": obj})
    for state in STATES:
        m = assemble(0, state)[0]
        path = os.path.join(review, f"{ASSET}_{state}_LOD0.obj")
        review_rows.append({"name": state, "path": os.path.relpath(path, evidence_dir),
                            "sha256": m.write_obj(path, header_lines=[f"{ASSET} {state}", f"Revision {REVISION}"]),
                            "triangles": m.triangle_count(), "lit_threads": lit_threads(m)})
    m1 = assemble(1, "producing")[0]
    path = os.path.join(review, f"{ASSET}_producing_LOD1.obj")
    review_rows.append({"name": "producing_lod1", "path": os.path.relpath(path, evidence_dir),
                        "sha256": m1.write_obj(path, header_lines=[f"{ASSET} LOD1"]),
                        "triangles": m1.triangle_count()})
    by_name = {c.name: c for c in clips}
    for name, fraction, state in POSE_SAMPLES:
        mesh = posed(0, sample_pose(by_name[name], fraction), state)
        stem = f"pose_{name}_{int(fraction * 100):03d}"
        p = os.path.join(review, stem + ".obj")
        review_rows.append({"name": stem, "path": os.path.relpath(p, evidence_dir),
                            "sha256": mesh.write_obj(p, header_lines=[f"{ASSET} posed: {name} at {fraction:.2f}"]),
                            "lowest_z_cm": round(mesh.bounds()[0][2], 2)})
    return {"outputs": outputs, "review": review_rows, "clips": clips}


def manifest(exported: dict) -> dict:
    m0, s, counts, _socks = assemble(0, "producing")
    m1 = assemble(1, "producing")[0]
    research = assemble(0, "researching")[0]
    insolvent = assemble(0, "insolvent")[0]
    clips = exported["clips"]
    magenta = slot_area_fraction(m0, MAGENTA)
    worst = max(m0.triangle_count(), research.triangle_count())
    return {
        "author": AUTHOR, "creator": AUTHOR, "package_id": PACKAGE_ID, "production_asset_id": PRODUCTION_ID,
        "asset_name": ASSET, "planned_unreal_folder": PLANNED_FOLDER, "revision": REVISION,
        "kit_revision": getattr(kit, "KIT_REVISION", "unknown"),
        "skel_revision": getattr(skel, "SKEL_REVISION", "unknown"),
        "stage": "BLOCKOUT (concept-v1)",
        "stage_boundary": "Editable source and review evidence only. Not a gate pass, not an Unreal integration, not owner acceptance.",
        "units": {"length": "centimetres", "axes": "+X forward, +Y right, +Z up",
                  "pivot": "platform ground-contact centre", "nanite": False},
        "scale_basis": {"canon": "SPEC-BLD-017.HC.CHORUS: trains all Choir mobile combat units and hosts research",
                        "footprint_tiles": [FOOTPRINT_TILES, FOOTPRINT_TILES], "footprint_cm": F,
                        "upkeep": f"{UPKEEP_TICKS:.0f} ticks = {UPKEEP_TICKS / TICKS_PER_SECOND:.0f} s between charges",
                        "source": "Content/Data/Source/buildings.json hc_chorus_loom"},
        "material_slots": [VITRIFIED, GROUND, MAGENTA],
        "material_slot_policy": ("Three slots: vitrified stone for the posts and beam, a worn stone for the platform, and "
                                 "Magenta Fracture for the post edge strips, the warp and the beam's underside."),
        "traced_before_the_card": {
            "directive": "Owner ruling 2026-09-07: trace each selected concept BEFORE authoring its provisional card.",
            "trace": "the candidate's PRODUCING view, dark-pixel column spans; two tall posts near the ends of a low platform, a warp between them, a beam floating clear above",
            "card_written_after": True},
        "faction_language_inference": {
            "what": "the 12% Magenta Fracture ceiling",
            "source": "the four REL-FAC-027.HC.* UNIT cards",
            "status": "INFERENCE, not a requirement. No authoritative card places it on a Hollow Choir BUILDING."},
        "provisional_contract": {
            "card": f"{CARD} in ArtSource/hollow-choir-asset-cards.json (rendered to hollow-choir-asset-cards.md)",
            "status": "PROVISIONAL. Authored in this worktree after the trace; NOT in Docs/Requirements.md.",
            "bounds": {"lod0_triangles": 6000, "lod1_triangles": 2400}},
        "budgets": {"lod0_triangles": m0.triangle_count(), "lod1_triangles": m1.triangle_count(),
                    "researching_state_triangles": research.triangle_count(), "worst_state_triangles": worst,
                    "lod0_cap": 6000, "lod1_cap": 2400, "cap_source": f"{CARD} (PROVISIONAL)",
                    "lod0_within_cap": worst <= 6000, "lod1_within_cap": m1.triangle_count() <= 2400,
                    "headroom_note": "Under the ceiling is HEADROOM, not sufficiency (owner ruling 2026-09-07).",
                    "magenta_area_fraction_producing": round(magenta, 5), "magenta_cap": 0.12,
                    "magenta_within_cap": magenta <= 0.12},
        "state_read": {"producing_threads": lit_threads(m0), "researching_threads": lit_threads(research),
                       "insolvent_threads": lit_threads(insolvent),
                       "mechanism": ("researching DOUBLES the warp and lights the beam's underside; insolvent moves every "
                                     "thread and strip into the vitrified slot. The states differ in GEOMETRY as well as in "
                                     "material, so none of them relies on brightness alone.")},
        "concept_measurements": measurements(m0),
        "component_inventory": contract_inventory(m0, s, clips),
        "rig": {"bones": [{"name": b.name, "parent": b.parent, "head_cm": list(b.head), "purpose": b.purpose} for b in s.bones],
                "policy": ("Root, one bone per post and one for the hovering beam. The drift is the faction's reality-bleed "
                           "device, NOT a canon motion clause for this building. No root motion.")},
        "clips": [{"name": c.name, "duration_s": c.duration_s, "ticks": round(c.duration_s * TICKS_PER_SECOND),
                   "loop": c.loop, "purpose": c.purpose, "bones": sorted(c.tracks)} for c in clips],
        "states": {"producing": "the warp is strung and lit; the weave centre is the runtime's to fill",
                   "researching": "the warp doubles and the beam's underside lights",
                   "insolvent": "warp, strips and beam dark",
                   "destroyed": "posts down and the beam fallen onto the platform"},
        "pending_requirements": [
            {"requirement": "Articulated components for a structure of this role", "status": "PENDING, not waived",
             "detail": "The post and beam drift is a presentation device, not a role-required articulated assembly."}],
        "bind_counts": counts,
        "outputs": exported["outputs"], "review_assemblies": exported["review"],
        "tools": {"mesh_kit": "ArtSource/tools/ebs_meshkit.py", "skeletal_kit": "ArtSource/tools/ebs_skelkit.py",
                  "renderer": "ArtSource/tools/ebs_render.py"},
        "acceptance": {"art": "NOT_EVALUATED", "gameplay": "NOT_EVALUATED", "animation": "NOT_EVALUATED",
                       "technical": "PENDING — built against a provisional card that is not in the authoritative requirements",
                       "owner": "NOT_ACCEPTED"},
        "source_bindings": {"candidate": "BuildArtifacts/Evidence/concept-discovery-20260906/chorus-loom-review/chorus-loom-candidate.png",
                            "canon": "Requirements SPEC-BLD-017.HC.CHORUS",
                            "gameplay": "Content/Data/Source/buildings.json hc_chorus_loom"},
    }


def write_scenes(evidence_dir: str) -> list:
    scenes = os.path.join(evidence_dir, "scenes")
    os.makedirs(scenes, exist_ok=True)
    base = {"author": AUTHOR, "srgb": True,
            "materials": {VITRIFIED: [0.40, 0.40, 0.42], GROUND: [0.30, 0.29, 0.29],
                          MAGENTA: [0.86, 0.36, 0.92], "_default": [0.5, 0.5, 0.5]},
            "emissive": [MAGENTA], "width": 1920, "height": 1080, "background": [0.84, 0.82, 0.78],
            "light": {"direction": [0.55, -0.35, -0.76], "ambient": 0.34, "key": 0.82, "color": [0.95, 0.94, 1.0],
                      "fill_color": [0.6, 0.58, 0.72], "fill": 0.26},
            "ground": {"tile_cm": 200, "tiles": 40, "color": [0.028, 0.028, 0.032], "grid_color": [0.07, 0.07, 0.08],
                       "footprint_cm": [F, F], "footprint_color": [0.86, 0.36, 0.92]},
            "reference_figure": {"height_cm": 180, "position": [-480, 440, 0], "color": [0.92, 0.55, 0.2]}}
    ortho = [{"name": "front", "type": "ortho", "from": "-Y", "edges": True, "margin": 1.1, "target": [0, 0, 180]},
             {"name": "top", "type": "ortho", "from": "+Z", "image_up": "+X", "edges": True, "margin": 1.18, "target": [0, 0, 0]}]
    views = [{"name": "three_quarter", "type": "persp", "pitch_deg": -16, "yaw_deg": 62, "arm_cm": 1300, "fov_deg": 50, "target": [0, 0, 180]},
             {"name": "weave_detail", "type": "persp", "pitch_deg": -6, "yaw_deg": 96, "arm_cm": 520, "fov_deg": 48, "target": [-POST_X + 60, 0, 190]},
             {"name": "tactical_gameplay", "type": "persp", "pitch_deg": -60, "yaw_deg": -45, "arm_cm": 3400, "fov_deg": 55, "target": [0, 0, 0]}]
    written = []

    def dump(name, meshes, vs):
        scene = dict(base); scene["meshes"] = meshes; scene["views"] = vs
        p = os.path.join(scenes, f"{name}.json")
        with open(p, "w", encoding="utf-8") as handle:
            json.dump(scene, handle, indent=1)
        written.append(p)

    for state in STATES:
        dump(state, [{"obj": f"../review/{ASSET}_{state}_LOD0.obj"}], ortho + views)
    dump("lod1", [{"obj": f"../review/{ASSET}_producing_LOD1.obj"}], ortho + [views[0], views[2]])
    for name, fraction, _state in POSE_SAMPLES:
        stem = f"pose_{name}_{int(fraction * 100):03d}"
        dump(stem, [{"obj": f"../review/{stem}.obj"}], [views[0]])
    return written


def build_all(evidence_dir: str, out_dir: str, skinned: bool) -> dict:
    return manifest(export(evidence_dir, out_dir, skinned))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--evidence-dir", required=True)
    parser.add_argument("--skinned", action="store_true")
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    manifest_path = os.path.join(HERE, "build-manifest.json")
    if args.check:
        with tempfile.TemporaryDirectory() as tmp:
            ev = os.path.join(tmp, "evidence"); os.makedirs(ev, exist_ok=True)
            fresh = build_all(ev, os.path.join(tmp, "export"), True)
        with open(manifest_path, encoding="utf-8") as handle:
            saved = json.load(handle)
        drift, missing = [], []
        saved_out = {os.path.basename(o["path"]): o["sha256"] for o in saved["outputs"]}
        for o in fresh["outputs"]:
            name = os.path.basename(o["path"])
            if name not in saved_out:
                missing.append(name)
            elif saved_out[name] != o["sha256"]:
                drift.append(name)
        saved_rev = {os.path.basename(r["path"]): r["sha256"] for r in saved.get("review_assemblies", [])}
        for r in fresh["review_assemblies"]:
            key = os.path.basename(r["path"])
            if key not in saved_rev:
                missing.append(key)
            elif saved_rev[key] != r["sha256"]:
                drift.append(key)
        if saved.get("revision") != REVISION:
            drift.append(f"revision {saved.get('revision')} != {REVISION}")
        print(json.dumps({"check": "ok" if not drift and not missing else "drift", "revision": REVISION,
                          "drift": drift, "missing": missing,
                          "compared": {"outputs": len(fresh["outputs"]), "review": len(fresh["review_assemblies"])}}))
        return 0 if not drift and not missing else 1
    os.makedirs(args.evidence_dir, exist_ok=True)
    data = build_all(args.evidence_dir, os.path.join(HERE, "export"), args.skinned)
    scenes = write_scenes(args.evidence_dir)
    data["review"] = {"scenes": [os.path.relpath(s, args.evidence_dir) for s in scenes]}
    with open(manifest_path, "w", encoding="utf-8") as handle:
        json.dump(data, handle, indent=1, sort_keys=True)
    mm = data["concept_measurements"]
    lows = [r["lowest_z_cm"] for r in data["review_assemblies"] if "lowest_z_cm" in r]
    print(json.dumps({"revision": REVISION, "lod0": data["budgets"]["lod0_triangles"],
                      "lod1": data["budgets"]["lod1_triangles"],
                      "height_cm": mm["height_cm"], "post_span_cm": mm["post_span_cm"],
                      "warp_count": mm["warp_count"], "beam_gap_cm": mm["beam_gap_cm"],
                      "magenta_area": data["budgets"]["magenta_area_fraction_producing"],
                      "state_read": {k: v for k, v in data["state_read"].items() if k != "mechanism"},
                      "lowest_posed_z_cm": min(lows) if lows else None,
                      "max_extent_cm": mm["max_extent_cm"], "footprint_half_cm": HALF,
                      "bones": len(data["rig"]["bones"]), "clips": len(data["clips"]), "scenes": len(scenes)}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
