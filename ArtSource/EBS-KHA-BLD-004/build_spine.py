#!/usr/bin/env python3
"""Deterministic source generator for EBS-KHA-BLD-004 — the Kharuun Assemblies Listening Spine.

Author: Angelis Pseftis. Creator: Angelis Pseftis.

Built to `concept-fidelity.md` against the provisional card `REL-BLD-016.KA.SPINE.ASSET`: one tall
curved tapering rib of plated strata rising from a rooted socket, with eleven amber sensor nodules
climbing its leading edge. A spine, not a weapon — no dish, antenna, radar or mast, and the rib leans
rather than aims.

Usage:
  python3 build_spine.py --evidence-dir "<root>/EBS-KHA-BLD-004" [--skinned]
  python3 build_spine.py --evidence-dir "<root>/EBS-KHA-BLD-004" --check

Units: centimetres. +X forward, +Y right, +Z up. Pivot at the socket's ground-contact centre. Nanite
off. Nothing below z = 0; nothing outside the 2x2 tile footprint.
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
PACKAGE_ID = "EBS-PKG-KA-LISTENING-SPINE"
PRODUCTION_ID = "EBS-KHA-BLD-004"
ASSET = "SK_EBS_KHA_BLD_004"
PLANNED_FOLDER = "/Game/Echoes/Production/KHA/BLD/EBS_KHA_BLD_004/"
REVISION = "ebs-kha-bld-004-concept-v1"
CARD = "REL-BLD-016.KA.SPINE.ASSET"

STRATA = "MI_EBS_KHA_Strata"      # plated faceted strata: the rib, the socket collar, the roots
AMBER = "MI_EBS_KHA_Amber"        # the sensor nodules — the only emissive

TICKS_PER_SECOND = 20.0

TILE_CM = 200.0
FOOTPRINT_TILES = 2
F = TILE_CM * FOOTPRINT_TILES          # 400 cm square
HALF = F / 2.0

ROOT_DISC = 380.0                      # the rooted socket's root spread, inside the 400 cm square
TOTAL_Z = 689.0                        # the rib's last course; the tip cone adds 22 more, giving the
                                       # traced 711 cm total = 1.87 of the 380 cm root disc
RIB_BASE_W = 125.0                     # 0.176 of the height, as traced
TIP_OFFSET = 89.0                      # 0.125 of the height, as traced: the rib finishes off its axis
RIB_SEGMENTS = 9                       # plate courses spiralling up the rib
SOCKET_Z = 118.0                       # the stepped collar the rib rises from
SOCKET_STEPS = ((104.0, 0.0, 46.0), (92.0, 46.0, 86.0), (80.0, 86.0, SOCKET_Z))
ROOT_COUNT = 10
ROOT_LEN = 60.0                        # sized so the root disc lands at 380 cm, inside the square
ROOT_FLAT_R = 8.0                      # a flat root's radius; its top must clear 20 cm (REL-ART-030)
ROOT_FLAT_Z = 12.0
NODULE_COUNT = 11                      # counted on the candidate's DETECTING view
STATES = ("listening", "contact", "damaged", "destroyed")

BONES = [
    ("root", None, (0.0, 0.0, 0.0), "socket ground-contact centre; the spine never moves"),
    ("socket", "root", (0.0, 0.0, SOCKET_Z / 2.0), "the rooted collar: fixed, and the rib's foundation"),
    ("spine_lower", "socket", (0.0, 0.0, SOCKET_Z), "card .ANIM_RIG lean bone 1: the rib's lower bend"),
    ("spine_upper", "spine_lower", (0.0, 0.0, SOCKET_Z + (TOTAL_Z - SOCKET_Z) * 0.46),
     "card .ANIM_RIG lean bone 2: the rib's upper bend. The rib LEANS; it does not aim"),
]


def rib_curve(t: float):
    """(x offset, z, half-width) at normalised height t along the rib.

    The candidate's rib is not vertical: its tip finishes 0.125 of the height off the axis of its base.
    The offset follows t^2 so the lower rib stands nearly upright and the curve builds toward the tip.
    """
    z = SOCKET_Z + (TOTAL_Z - SOCKET_Z) * t
    x = TIP_OFFSET * t * t
    half = (RIB_BASE_W / 2.0) * (1.0 - t) ** 1.35 + 5.0
    return x, z, half


def _yaw_point(radius: float, yaw_deg: float):
    a = math.radians(yaw_deg)
    return (radius * math.cos(a), radius * math.sin(a))


def build_body(lod: int, state: str = "listening") -> kit.Mesh:
    if state not in STATES:
        raise ValueError(f"unknown state {state!r}; expected one of {STATES}")
    m = kit.Mesh(ASSET)
    strata = m.slot(STRATA)
    amber = m.slot(AMBER)
    fine = lod == 0
    sides = 8 if fine else 5
    segments = RIB_SEGMENTS if fine else 5
    ruined = state == "destroyed"

    # 1. the rooted socket: a stepped collar with roots splaying flat across the ground
    for step, (hw, z0, z1) in enumerate(SOCKET_STEPS, start=1):
        if ruined:
            z0, z1 = z0 * 0.6, z1 * 0.6
        m.prism(kit.regular_polygon(hw, sides, phase_deg=9.0 * step), z0, z1, strata,
                f"socket_step_{step:02d}", cap_bottom=(step == 1))
    count = ROOT_COUNT if fine else ROOT_COUNT // 2
    inner = SOCKET_STEPS[0][0] - 8.0
    for k in range(count):
        yaw = 360.0 * k / count
        a = math.radians(yaw)
        ox, oy = math.cos(a) * (inner + ROOT_LEN), math.sin(a) * (inner + ROOT_LEN)
        m.tube((math.cos(a) * inner, math.sin(a) * inner, ROOT_FLAT_Z),
               (ox, oy, ROOT_FLAT_R), ROOT_FLAT_R, 4, strata, f"root_{k + 1:02d}")
        if fine:
            b = a + 0.19
            m.tube((ox, oy, ROOT_FLAT_R),
                   (math.cos(b) * (inner + ROOT_LEN + 34.0), math.sin(b) * (inner + ROOT_LEN + 34.0), ROOT_FLAT_R * 0.7),
                   ROOT_FLAT_R * 0.7, 4, strata, f"root_{k + 1:02d}_tip")

    if ruined:
        # the rib snaps at the socket, per the card: the socket stays, the rib lies broken beside it
        for k in range(3):
            # the broken rib has to land inside the placement envelope like everything else
            m.box((58.0 + 47.0 * k, 22.0 * (k % 2) - 11.0, 30.0 - 6.0 * k),
                  (90.0 - 14.0 * k, 58.0 - 12.0 * k, 56.0 - 10.0 * k), strata,
                  f"fallen_rib_{k + 1:02d}", yaw_deg=6.0 * k)
        m.collision.append(kit.CollisionBox("socket", (0.0, 0.0, SOCKET_Z * 0.3),
                                            (2 * SOCKET_STEPS[0][0], 2 * SOCKET_STEPS[0][0], SOCKET_Z * 0.6)))
        return m

    # 2. the rib: plate courses spiralling up a curved, tapering horn
    for i in range(segments):
        t0, t1 = i / segments, (i + 1) / segments
        x0, z0, h0 = rib_curve(t0)
        x1, z1, h1 = rib_curve(t1)
        low = [(x0 + p[0] * h0 / max(h0, 1e-6), p[1] * h0 / max(h0, 1e-6), z0)
               for p in kit.regular_polygon(h0, sides, phase_deg=14.0 * i)]
        high = [(x1 + p[0] * h1 / max(h1, 1e-6), p[1] * h1 / max(h1, 1e-6), z1)
                for p in kit.regular_polygon(h1, sides, phase_deg=14.0 * (i + 1))]
        for k in range(sides):
            j = (k + 1) % sides
            outward = ((low[k][0] + low[j][0]) / 2.0 - x0, (low[k][1] + low[j][1]) / 2.0, 0.0)
            m.add_polygon([low[k], low[j], high[j], high[k]], strata, f"rib_course_{i + 1:02d}",
                          normal=outward)
        if i == 0:
            m.add_polygon(list(reversed(low)), strata, f"rib_course_{i + 1:02d}", normal=(0.0, 0.0, -1.0))
    tip_x, tip_z, _ = rib_curve(1.0)
    x_last, z_last, h_last = rib_curve((segments - 1) / segments)
    last = [(x_last + p[0], p[1], z_last) for p in kit.regular_polygon(h_last * 0.5, sides)]
    for k in range(sides):
        j = (k + 1) % sides
        m.add_polygon([last[k], last[j], (tip_x, 0.0, tip_z + 22.0)], strata, "rib_tip",
                      normal=((last[k][0] + last[j][0]) / 2.0 - x_last, (last[k][1] + last[j][1]) / 2.0, 0.0))

    # 3. the sensor nodules: one line up the rib's leading edge, each its own component so the runtime
    #    can light them in sequence. The sequence is a per-nodule material animation, not geometry.
    lit = state == "contact"
    for n in range(NODULE_COUNT):
        t = 0.06 + 0.84 * n / (NODULE_COUNT - 1)
        x, z, half = rib_curve(t)
        size = 30.0 * (1.0 - 0.55 * t)
        # the nodules run up the CONCAVE side of the curve, as the candidate draws them: the rib bends
        # toward +X, so its leading concave face is -X
        m.box((x - half - size * 0.28, 0.0, z), (size * 0.7, size, size * 1.25),
              amber if lit else strata, f"nodule_{n + 1:02d}")

    if state == "damaged":
        for k, t in enumerate((0.34, 0.62), start=1):
            x, z, half = rib_curve(t)
            m.box((x + half + 6.0, 0.0, z), (26.0, 34.0, 62.0), strata, f"chip_{k:02d}")

    m.collision.append(kit.CollisionBox("socket", (0.0, 0.0, SOCKET_Z / 2.0),
                                        (2 * SOCKET_STEPS[0][0], 2 * SOCKET_STEPS[0][0], SOCKET_Z)))
    m.collision.append(kit.CollisionBox("rib", (TIP_OFFSET * 0.25, 0.0, (TOTAL_Z + SOCKET_Z) / 2.0),
                                        (RIB_BASE_W + TIP_OFFSET, RIB_BASE_W, TOTAL_Z - SOCKET_Z)))
    return m


def build_skeleton() -> skel.Skeleton:
    s = skel.Skeleton("root")
    for name, parent, head, purpose in BONES:
        s.add(name, parent, head, purpose=purpose)
    return s


def bind(m: kit.Mesh) -> dict:
    by_component = {}
    upper_from = SOCKET_Z + (TOTAL_Z - SOCKET_Z) * 0.46
    for c in m.components():
        if c.startswith(("socket_step_", "root_")):
            by_component[c] = "socket"
        elif c.startswith(("rib_", "nodule_", "chip_")):
            centre = m.component_bounds(c)
            mid_z = (centre[0][2] + centre[1][2]) / 2.0
            by_component[c] = "spine_upper" if mid_z >= upper_from else "spine_lower"
    return skel.bind_polygons(m, "root", by_component)


SOCKETS = {
    "Target_Anchor_Center": ("spine_lower", (rib_curve(0.35)[0], 0.0, rib_curve(0.35)[1]), 0.0,
                             "targeting and selection anchor on the lower rib"),
    "Nodule_Base": ("spine_lower", (rib_curve(0.06)[0] - 40.0, 0.0, rib_curve(0.06)[1]), 0.0,
                    "the first nodule in the sequence: where a detection run starts"),
    "Nodule_Tip": ("spine_upper", (rib_curve(0.90)[0] - 18.0, 0.0, rib_curve(0.90)[1]), 0.0,
                   "the last nodule in the sequence"),
    "Socket_Root": ("root", (0.0, 0.0, 8.0), 0.0, "the rooted socket's ground contact"),
}


def assemble(lod: int, state: str = "listening"):
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

    idle = new("idle_pulse", 120, True,
               "listening: a slow lean cycle, the visual partner of canon's slow pulse. The pulse itself "
               "is carried by the nodules' material, which the runtime drives")
    for t, lean in ((0.0, 0.0), (3.0, 1.6), (6.0, 0.0)):
        idle.key("spine_lower", t, (0.0, lean * 0.5, 0.0))
        idle.key("spine_upper", t, (0.0, lean, 0.0))

    sweep = new("detect_sweep", 40, False,
                "a contact: the rib LEANS toward the source while the nodules run their sequence. The "
                "runtime supplies the bearing; this clip carries the lean, not an aim")
    for t, lower, upper in ((0.0, 0.0, 0.0), (0.8, 3.0, 7.0), (2.0, 2.2, 5.5)):
        sweep.key("spine_lower", t, (0.0, lower, 0.0))
        sweep.key("spine_upper", t, (0.0, upper, 0.0))

    off = new("offline", 30, False, "power or structure lost: the rib slumps off its lean and holds")
    for t, lower, upper in ((0.0, 0.0, 0.0), (1.5, -4.0, -9.0)):
        off.key("spine_lower", t, (0.0, lower, 0.0))
        off.key("spine_upper", t, (0.0, upper, 0.0))

    restore = new("restore", 0, False, "single-frame listening rest pose for reconstruction from saved state")
    for b in ("spine_lower", "spine_upper"):
        restore.key(b, 0.0, (0.0, 0.0, 0.0))

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


def posed(lod: int, pose: dict, state: str = "listening") -> kit.Mesh:
    m, s, _c, socks = assemble(lod, state)
    p = dict(pose)
    p["_sockets"] = socks
    return skel.pose_mesh(m, s, p)


POSE_SAMPLES = [("detect_sweep", 0.4, "contact"), ("detect_sweep", 1.0, "contact"),
                ("offline", 1.0, "listening")]


def slot_area_fraction(m: kit.Mesh, slot_name: str) -> float:
    """Share of the mesh's SURFACE AREA carried by one slot; REL-ART-029 caps amber by area."""
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


def lit_nodules(m: kit.Mesh) -> list:
    return sorted({int(p.component.split("_")[1]) for p in m.polygons
                   if p.component.startswith("nodule_") and m.slots[p.slot] == AMBER})


def contract_inventory(m: kit.Mesh, s: skel.Skeleton, clips) -> dict:
    comps = m.components()
    return {
        "single_rib": {"contract": "one rib, curved and tapering",
                       "courses": sum(1 for c in comps if c.startswith("rib_course_")),
                       "tip": "rib_tip" in comps},
        "sensor_nodules": {"contract": NODULE_COUNT,
                           "built": sum(1 for c in comps if c.startswith("nodule_")),
                           "individually_addressable": True,
                           "note": "one component per nodule so the runtime can light them in sequence"},
        "rooted_socket": {"contract": len(SOCKET_STEPS),
                          "steps": sum(1 for c in comps if c.startswith("socket_step_")),
                          "roots": sum(1 for c in comps if c.startswith("root_") and not c.endswith("_tip"))},
        "not_a_weapon": {"contract": "no dish, antenna, radar, mast, turret ring, trunnion or muzzle",
                         "violations": [c for c in comps
                                        if any(w in c for w in ("dish", "antenna", "radar", "mast",
                                                                "turret", "trunnion", "muzzle", "barrel"))]},
        "bones": {"contract": "root, socket and a two-bone leaning rib (card REL-BLD-016.KA.SPINE.ASSET)",
                  "built": len(s.bones), "names": [b.name for b in s.bones]},
        "sockets": {"contract": sorted(SOCKETS), "built": sorted(sk.name for sk in m.sockets)},
        "tracks": {"contract": ["idle_pulse", "detect_sweep", "offline", "restore"],
                   "built": [c.name for c in clips]},
        "states": {"contract": list(STATES), "built": list(STATES)},
    }


def measurements(m: kit.Mesh) -> dict:
    (x0, y0, z0), (x1, y1, z1) = m.bounds()
    root_span = 0.0
    for c in m.components():
        if c.startswith("root_"):
            b = m.component_bounds(c)
            root_span = max(root_span, 2 * max(abs(v) for p in b for v in p[:2]))
    return {
        "footprint_tiles": [FOOTPRINT_TILES, FOOTPRINT_TILES], "footprint_cm": F,
        "bounds_cm": [[round(x0, 2), round(y0, 2), round(z0, 2)], [round(x1, 2), round(y1, 2), round(z1, 2)]],
        "height_cm": round(z1, 2),
        "root_disc_cm": round(root_span, 2),
        "height_over_root_disc": round(z1 / root_span, 4) if root_span else None,
        "rib_base_width_over_height": round(RIB_BASE_W / z1, 4),
        "tip_offset_over_height": round(rib_curve(1.0)[0] / z1, 4),
        "nodule_count": NODULE_COUNT,
        "flat_root_top_cm": ROOT_FLAT_Z + ROOT_FLAT_R,
        "flat_roots_under_ground_clutter_ceiling": ROOT_FLAT_Z + ROOT_FLAT_R <= 20.0,
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
        m, s, _c, socks = assemble(lod, "listening")
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
        obj = m.write_obj(base + ".obj", header_lines=[f"{ASSET} LOD{lod} rest pose (listening)",
                                                       f"Revision {REVISION}",
                                                       "Unreal +X forward, +Y right, +Z up; centimetres"])
        outputs.append({"mesh": ASSET, "lod": lod, "kind": "rest-pose OBJ",
                        "path": os.path.relpath(base + ".obj", HERE), "sha256": obj})
    for state in STATES:
        m = assemble(0, state)[0]
        path = os.path.join(review, f"{ASSET}_{state}_LOD0.obj")
        review_rows.append({"name": state, "path": os.path.relpath(path, evidence_dir),
                            "sha256": m.write_obj(path, header_lines=[f"{ASSET} {state}", f"Revision {REVISION}"]),
                            "triangles": m.triangle_count(), "lit_nodules": lit_nodules(m),
                            "height_cm": round(m.bounds()[1][2], 2)})
    m1 = assemble(1, "listening")[0]
    path = os.path.join(review, f"{ASSET}_listening_LOD1.obj")
    review_rows.append({"name": "listening_lod1", "path": os.path.relpath(path, evidence_dir),
                        "sha256": m1.write_obj(path, header_lines=[f"{ASSET} LOD1"]),
                        "triangles": m1.triangle_count()})
    by_name = {c.name: c for c in clips}
    for name, fraction, state in POSE_SAMPLES:
        mesh = posed(0, sample_pose(by_name[name], fraction), state)
        stem = f"pose_{name}_{int(fraction * 100):03d}"
        p = os.path.join(review, stem + ".obj")
        review_rows.append({"name": stem, "path": os.path.relpath(p, evidence_dir),
                            "sha256": mesh.write_obj(p, header_lines=[f"{ASSET} posed: {name} at {fraction:.2f} ({state})"]),
                            "lowest_z_cm": round(mesh.bounds()[0][2], 2),
                            "max_extent_cm": round(max(abs(v) for b in mesh.bounds() for v in b[:2]), 2)})
    return {"outputs": outputs, "review": review_rows, "clips": clips}


def manifest(exported: dict) -> dict:
    m0, s, counts, _socks = assemble(0, "listening")
    m1 = assemble(1, "listening")[0]
    contact = assemble(0, "contact")[0]
    clips = exported["clips"]
    amber = slot_area_fraction(contact, AMBER)
    return {
        "author": AUTHOR, "creator": AUTHOR, "package_id": PACKAGE_ID, "production_asset_id": PRODUCTION_ID,
        "asset_name": ASSET, "planned_unreal_folder": PLANNED_FOLDER, "revision": REVISION,
        "kit_revision": getattr(kit, "KIT_REVISION", "unknown"),
        "skel_revision": getattr(skel, "SKEL_REVISION", "unknown"),
        "stage": "BLOCKOUT (concept-v1)",
        "stage_boundary": "Editable source and review evidence only. Not a gate pass, not an Unreal integration, not owner acceptance.",
        "units": {"length": "centimetres", "axes": "+X forward, +Y right, +Z up",
                  "pivot": "socket ground-contact centre", "nanite": False},
        "scale_basis": {"canon": "SPEC-BLD-016 Listening Spine row: a single tall rib of strata with amber sensor nodules climbing it, set into a rooted socket. A spine, not a weapon",
                        "footprint_tiles": [FOOTPRINT_TILES, FOOTPRINT_TILES], "footprint_cm": F,
                        "source": "Content/Data/Source/buildings.json ka_listening_spine.footprint_cells",
                        "tick_rate": f"{TICKS_PER_SECOND:.0f} ticks per second"},
        "material_slots": [STRATA, AMBER],
        "material_slot_policy": ("Two slots: plated faceted strata for the rib, collar and roots, and amber for the sensor "
                                 "nodules, which are the only emissive. No dish, antenna, radar or mast language anywhere — "
                                 "canon says a spine, not a weapon, and a test asserts no component is named like one."),
        "provisional_contract": {
            "card": f"{CARD} in ArtSource/kharuun-asset-cards.json (rendered to kharuun-asset-cards.md)",
            "status": ("PROVISIONAL. Authored in this worktree under the owner ruling of 2026-09-07; NOT incorporated into "
                       "Docs/Requirements.md and not an existing authoritative per-asset requirement."),
            "bounds": {"lod0_triangles": 3000, "lod1_triangles": 1200,
                       "basis": "well under the faction default because canon specifies one rib in a socket"}},
        "budgets": {"lod0_triangles": m0.triangle_count(), "lod1_triangles": m1.triangle_count(),
                    "contact_state_triangles": contact.triangle_count(),
                    "lod0_cap": 3000, "lod1_cap": 1200, "cap_source": f"{CARD} (PROVISIONAL)",
                    "cap_scope": "the complete assembly including the rooted socket",
                    "lod0_within_cap": max(m0.triangle_count(), contact.triangle_count()) <= 3000,
                    "lod1_within_cap": m1.triangle_count() <= 1200,
                    "amber_area_fraction_contact": round(amber, 5), "amber_cap": 0.15,
                    "amber_measure": "surface area in the fully lit contact state, per REL-ART-029; the cap is a ceiling, not a target",
                    "amber_within_cap": amber <= 0.15},
        "concept_measurements": measurements(m0),
        "component_inventory": contract_inventory(m0, s, clips),
        "rig": {"bones": [{"name": b.name, "parent": b.parent, "head_cm": list(b.head), "purpose": b.purpose} for b in s.bones],
                "policy": ("The card's own rig: root, socket and a two-bone rib. It is NOT a turret rig — the rib LEANS toward a "
                           "contact, it does not aim, and there is no yaw ring, pitch trunnion or muzzle. No root motion.")},
        "clips": [{"name": c.name, "duration_s": c.duration_s, "ticks": round(c.duration_s * TICKS_PER_SECOND),
                   "loop": c.loop, "purpose": c.purpose, "bones": sorted(c.tracks)} for c in clips],
        "states": {name: purpose for name, purpose in (
            ("listening", "the nodules sit in the strata slot and the rib stands at rest"),
            ("contact", "every nodule is in the amber slot; the SEQUENCE toward the source is a per-nodule material animation the runtime drives, not geometry"),
            ("damaged", "chips out of the rib's trailing edge"),
            ("destroyed", "the rib snapped at the socket: the socket and roots remain, the rib lies broken beside them"))},
        "detection_readout": {"nodule_count": NODULE_COUNT,
                              "addressing": "one component per nodule, ordered base to tip, so the runtime can light them in sequence",
                              "note": ("The state assemblies show all-dark and all-lit only. The ordered sequence toward a source "
                                       "direction is a material animation over these components and is not represented in geometry.")},
        "bind_counts": counts,
        "outputs": exported["outputs"], "review_assemblies": exported["review"],
        "tools": {"mesh_kit": "ArtSource/tools/ebs_meshkit.py", "skeletal_kit": "ArtSource/tools/ebs_skelkit.py",
                  "renderer": "ArtSource/tools/ebs_render.py"},
        "acceptance": {"art": "NOT_EVALUATED", "gameplay": "NOT_EVALUATED",
                       "technical": ("PENDING — built against the provisional card; final technical acceptance waits on that card "
                                     "being incorporated into the authoritative requirements and its checks passing"),
                       "owner": "NOT_ACCEPTED"},
        "source_bindings": {"candidate": "BuildArtifacts/Evidence/concept-discovery-20260906/listening-spine-review/listening-spine-candidate.png",
                            "concepts": ["EBS-CON-KHA-BLD-004", "EBS-CON-KHA-BLD-008"],
                            "canon": "DevelopmentBible.md SPEC-BLD-016 Listening Spine row",
                            "gameplay": "Content/Data/Source/buildings.json ka_listening_spine"},
    }


def write_scenes(evidence_dir: str) -> list:
    scenes = os.path.join(evidence_dir, "scenes")
    os.makedirs(scenes, exist_ok=True)
    base = {"author": AUTHOR, "srgb": True,
            "materials": {STRATA: [0.21, 0.205, 0.20], AMBER: [0.98, 0.66, 0.22], "_default": [0.5, 0.5, 0.5]},
            "emissive": [AMBER], "width": 1920, "height": 1080, "background": [0.84, 0.82, 0.78],
            "light": {"direction": [0.55, -0.35, -0.76], "ambient": 0.32, "key": 0.85, "color": [1.0, 0.86, 0.7],
                      "fill_color": [0.5, 0.55, 0.7], "fill": 0.24},
            "ground": {"tile_cm": 200, "tiles": 40, "color": [0.028, 0.028, 0.032], "grid_color": [0.07, 0.07, 0.08],
                       "footprint_cm": [F, F], "footprint_color": [0.98, 0.66, 0.22]},
            "reference_figure": {"height_cm": 180, "position": [-330, 300, 0], "color": [0.92, 0.55, 0.2]}}
    ortho = [{"name": "front", "type": "ortho", "from": "+Y", "edges": True, "margin": 1.1, "target": [0, 0, 350]},
             {"name": "side", "type": "ortho", "from": "-X", "edges": True, "margin": 1.1, "target": [0, 0, 350]},
             {"name": "top", "type": "ortho", "from": "+Z", "image_up": "+X", "edges": True, "margin": 1.2, "target": [0, 0, 0]}]
    views = [{"name": "three_quarter", "type": "persp", "pitch_deg": -8, "yaw_deg": 62, "arm_cm": 1250, "fov_deg": 52, "target": [0, 0, 360]},
             {"name": "socket_detail", "type": "persp", "pitch_deg": -26, "yaw_deg": 52, "arm_cm": 520, "fov_deg": 50, "target": [0, 0, 80]},
             {"name": "tactical_gameplay", "type": "persp", "pitch_deg": -60, "yaw_deg": -45, "arm_cm": 2600, "fov_deg": 55, "target": [0, 0, 0]}]
    written = []

    def dump(name, meshes, vs):
        scene = dict(base); scene["meshes"] = meshes; scene["views"] = vs
        p = os.path.join(scenes, f"{name}.json")
        with open(p, "w", encoding="utf-8") as handle:
            json.dump(scene, handle, indent=1)
        written.append(p)

    for state in STATES:
        dump(state, [{"obj": f"../review/{ASSET}_{state}_LOD0.obj"}], ortho + views)
    dump("lod1", [{"obj": f"../review/{ASSET}_listening_LOD1.obj"}], ortho[:2] + [views[0], views[2]])
    for name, fraction, _state in POSE_SAMPLES:
        stem = f"pose_{name}_{int(fraction * 100):03d}"
        dump(stem, [{"obj": f"../review/{stem}.obj"}], [ortho[0], views[0]])
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
    ext = [r["max_extent_cm"] for r in data["review_assemblies"] if "max_extent_cm" in r]
    print(json.dumps({"revision": REVISION, "lod0": data["budgets"]["lod0_triangles"],
                      "lod1": data["budgets"]["lod1_triangles"],
                      "height_cm": mm["height_cm"], "h_over_disc": mm["height_over_root_disc"],
                      "rib_w_over_h": mm["rib_base_width_over_height"],
                      "tip_offset_over_h": mm["tip_offset_over_height"],
                      "nodules": mm["nodule_count"],
                      "amber_area": data["budgets"]["amber_area_fraction_contact"],
                      "lowest_posed_z_cm": min(lows) if lows else None,
                      "max_posed_extent_cm": max(ext) if ext else None,
                      "max_extent_cm": mm["max_extent_cm"], "footprint_half_cm": HALF,
                      "bones": len(data["rig"]["bones"]), "clips": len(data["clips"]), "scenes": len(scenes)}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
