#!/usr/bin/env python3
"""Deterministic source generator for EBS-MER-BLD-001 — the Meridian Compact Anchor.

Author: Angelis Pseftis. Creator: Angelis Pseftis.

Built to `concept-fidelity.md`: the anchor-candidate sheet reconciles the two REWORK inputs into a
squat two-tier ceramic drum on a charcoal plinth with three numbered worker bays and their ramps, a
separate Matter intake chute, eight radial conduit roots ending at edge nodes, and a slim sensor mast
(not a reactor spire). Canon row `SPEC-BLD-015.MC.ANCHOR`; 5x5 tile footprint from the buildings
record; budgets bounded by the tighter Meridian roster rule `REL-ART-028` (8,000 / 3,500) rather than
the looser `REL-BLD-015.MC.CORE` card (12,000 / 4,500), so both hold.

Usage:
  python3 build_anchor.py --evidence-dir "<evidence root>/EBS-MER-BLD-001"          # write everything
  python3 build_anchor.py --evidence-dir "<evidence root>/EBS-MER-BLD-001" --check  # verify, write nothing

Units: centimetres. +X forward (the bay arc faces +X), +Y right, +Z up. Pivot at the ground-contact
centre. Nanite off. No geometry is authored below z = 0.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
import shutil
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(os.path.dirname(HERE), "tools"))
sys.path.insert(0, HERE)
import ebs_meshkit as kit  # noqa: E402

AUTHOR = "Angelis Pseftis"
PACKAGE_ID = "EBS-PKG-MC-ANCHOR"
PRODUCTION_ID = "EBS-MER-BLD-001"
ASSET = "SM_EBS_MER_BLD_001"
PLANNED_FOLDER = "/Game/Echoes/Production/MER/BLD/EBS_MER_BLD_001/"
REVISION = "ebs-mer-bld-001-concept-v1"

CERAMIC = "MI_EBS_MER_CeramicCivic"    # pale civic ceramic panels
FRAME = "MI_EBS_MER_CompactFrame"      # charcoal structural frame, plinth, conduits
STATUS = "MI_EBS_MER_StatusCyan"       # cyan bands, bay interiors, chute slot, mast head

# --- scale (concept-fidelity.md; every ratio is of the 5x5 tile footprint F = 1000 cm) ------------
TILE_CM = 200.0
FOOTPRINT_TILES = 5
F = TILE_CM * FOOTPRINT_TILES          # 1000 cm across; half-width 500
HALF = F / 2.0

# Measured on the candidate's main view (silhouette traced against the paper, title block excluded):
# the drum dominates the width and the conduit arms are stubs just past it, and the mast makes up
# 0.345 of the total height. A first pass had a 600 cm drum with long arms and a 478 cm mast, which
# inverted both readings; these constants follow the pixels within the bounds the rules fix.
PLINTH_R = 400.0                       # charcoal plinth, 0.80 F across
PLINTH_Z = 26.0
STEP_R = 380.0                         # upper plinth step the drum sits on
STEP_Z = 44.0
DRUM_R = 360.0                         # lower tier: the panel-bay wall, 720 cm = 0.72 F across
DRUM_Z = 300.0
UPPER_R = 290.0                        # stepped upper tier
UPPER_Z = 390.0
CAP_R = 180.0                          # low dome cap (a step, not a bunker dome)
CAP_Z = 432.0                          # drum mass 432 cm on 720 cm = 0.60 H/W: squat and wide (canon)
COLLAR_R = 90.0
COLLAR_Z = 464.0
MAST_Z = 630.0                         # lattice top
HEAD_Z = 680.0                         # sensor head top
WHIP_Z = 700.0                         # antenna tips: mast is 0.383 of the total height (re-measured on
                                       # the comparison sheet: the candidate's mast runs 0.38-0.40 of it)

SIDES = 16                             # rotunda facets (LOD1 halves this)
BAY_YAWS = (-32.0, 0.0, 32.0)          # three numbered worker bays on the +X arc
BAY_W, BAY_H, BAY_D = 96.0, 150.0, 46.0
RAMP_LEN, RAMP_W = 96.0, 120.0    # portal lip (r 398) to r 494, inside the 5x5 square's half-width
                                  # of 500; nothing may cross the footprint
CHUTE_YAW = 66.0                       # Matter intake, right of the bays and clearly not a door
CHUTE_W, CHUTE_H, CHUTE_D = 150.0, 120.0, 120.0
ARMS = 8                               # conduit roots
ARM_R0, ARM_R1 = 396.0, 450.0     # short stubs past the plinth, as the candidate draws them
NODE_R = 468.0
NODE_SIZE = (60.0, 60.0, 50.0)
ARM_TUBE_R = 13.0
ARM_TUBE_DY = 24.0                     # the pair separation
BAND_Z = (280.0, 374.0)                # cyan bands ringing the two tiers
BAND_H = 10.0

STATES = ("working", "damaged")


def _poly(radius: float, sides: int, phase: float = 0.0):
    return kit.regular_polygon(radius, sides, phase)


def _sides(lod: int) -> int:
    return SIDES if lod == 0 else SIDES // 2


def _yaw_point(radius: float, yaw_deg: float):
    a = math.radians(yaw_deg)
    return (radius * math.cos(a), radius * math.sin(a))


def build_main(lod: int, state: str = "working") -> kit.Mesh:
    """The whole structure: plinth, two tiers, bays and ramps, chute, conduit roots, mast."""
    if state not in STATES:
        raise ValueError(f"unknown state {state!r}; expected one of {STATES}")
    m = kit.Mesh(ASSET)
    ceramic = m.slot(CERAMIC)
    frame = m.slot(FRAME)
    status = m.slot(STATUS)
    n = _sides(lod)

    # 1. charcoal plinth and its step
    m.prism(_poly(PLINTH_R, n), 0.0, PLINTH_Z, frame, "plinth")
    m.prism(_poly(STEP_R, n), PLINTH_Z, STEP_Z, frame, "plinth_step")

    # 2. lower tier: the pale panel-bay wall
    m.prism(_poly(DRUM_R, n), STEP_Z, DRUM_Z, ceramic, "drum_lower")
    # vertical pilasters between the panel bays read as the engineered load frame (REL-ART-006)
    if lod == 0:
        for i in range(n):
            yaw = 360.0 / n * i
            px, py = _yaw_point(DRUM_R - 4.0, yaw)
            m.box((px, py, (STEP_Z + DRUM_Z) / 2.0), (18.0, 12.0, DRUM_Z - STEP_Z - 16.0), frame, f"pilaster_{i:02d}", yaw_deg=yaw)

    # 3. upper tier and the low cap
    m.prism(_poly(UPPER_R, n), DRUM_Z, UPPER_Z, ceramic, "drum_upper")
    # The candidate's top is a DOME, not a pair of flat discs: step it in four courses so the
    # silhouette curves toward the mast collar (review decision B keeps it low, not bunker-like).
    steps = 4 if lod == 0 else 2
    for i in range(steps):
        r0 = UPPER_R - (UPPER_R - CAP_R) * (i / steps)
        r1 = UPPER_R - (UPPER_R - CAP_R) * ((i + 1) / steps)
        z0 = UPPER_Z + (CAP_Z - UPPER_Z) * (i / steps)
        z1 = UPPER_Z + (CAP_Z - UPPER_Z) * ((i + 1) / steps)
        m.prism(_poly(r1 if i + 1 == steps else r0, n), z0, z1, ceramic, f"cap_course_{i + 1:02d}")
        _ = r1
    m.prism(_poly(COLLAR_R, max(8, n // 2)), CAP_Z, COLLAR_Z, frame, "mast_collar")

    # 4. cyan status bands ringing both tiers; in the damaged state the lower band is dark (frame slot)
    lower_slot = frame if state == "damaged" else status
    m.ring(_poly(DRUM_R + 3.0, n), _poly(DRUM_R - 3.0, n), BAND_Z[0], BAND_Z[0] + BAND_H, lower_slot, "band_lower")
    m.ring(_poly(UPPER_R + 3.0, n), _poly(UPPER_R - 3.0, n), BAND_Z[1], BAND_Z[1] + BAND_H, status, "band_upper")

    # 5. three numbered worker bays with their ramps
    for index, yaw in enumerate(BAY_YAWS, start=1):
        # The lit interior must be VISIBLE from outside, so the portal stands proud of the drum wall
        # rather than sitting inside it (a box buried in the prism reads as a blank wall at every
        # camera). Depth BAY_D spans DRUM_R - 8 .. DRUM_R + 38: seated in the wall, reading outward.
        bx, by = _yaw_point(DRUM_R + BAY_D / 2.0 - 8.0, yaw)
        m.box((bx, by, STEP_Z + BAY_H / 2.0), (BAY_D, BAY_W, BAY_H), status, f"bay_{index:02d}_interior", yaw_deg=yaw)
        sx, sy = _yaw_point(DRUM_R + BAY_D - 12.0, yaw)
        m.box((sx, sy, STEP_Z + BAY_H / 2.0), (10.0, BAY_W - 26.0, BAY_H - 26.0), frame, f"bay_{index:02d}_shade", yaw_deg=yaw)
        hx, hy = _yaw_point(DRUM_R + BAY_D / 2.0 - 6.0, yaw)
        m.box((hx, hy, STEP_Z + BAY_H + 14.0), (BAY_D + 8.0, BAY_W + 30.0, 26.0), frame, f"bay_{index:02d}_lintel", yaw_deg=yaw)
        for sign in (-1.0, 1.0):
            jx, jy = _yaw_point(DRUM_R + BAY_D / 2.0 - 8.0, yaw + sign * 6.4)
            m.box((jx, jy, STEP_Z + BAY_H / 2.0), (BAY_D + 4.0, 22.0, BAY_H + 10.0), frame, f"bay_{index:02d}_jamb_{'r' if sign > 0 else 'l'}", yaw_deg=yaw)
        rx, ry = _yaw_point(DRUM_R + BAY_D + RAMP_LEN / 2.0 - 14.0, yaw)
        m.box((rx, ry, STEP_Z / 2.0), (RAMP_LEN, RAMP_W, STEP_Z), frame, f"bay_{index:02d}_ramp", yaw_deg=yaw)

    # 6. Matter intake chute: its own housing with an angled throat and a lit slot
    cx, cy = _yaw_point(DRUM_R + CHUTE_D / 2.0 - 18.0, CHUTE_YAW)
    m.box((cx, cy, STEP_Z + CHUTE_H / 2.0), (CHUTE_D, CHUTE_W, CHUTE_H), ceramic, "chute_housing", yaw_deg=CHUTE_YAW)
    tx, ty = _yaw_point(DRUM_R + CHUTE_D - 30.0, CHUTE_YAW)
    m.box((tx, ty, STEP_Z + CHUTE_H * 0.62), (46.0, CHUTE_W - 30.0, 40.0), frame, "chute_throat", yaw_deg=CHUTE_YAW)
    sx, sy = _yaw_point(DRUM_R + CHUTE_D - 8.0, CHUTE_YAW)
    m.box((sx, sy, STEP_Z + CHUTE_H * 0.62), (10.0, CHUTE_W - 54.0, 22.0), status, "chute_slot", yaw_deg=CHUTE_YAW)

    # 7. eight conduit roots to edge nodes (the retained network-root organisation)
    for i in range(ARMS):
        yaw = 45.0 * i + 22.5
        a = math.radians(yaw)
        for sign in (-1.0, 1.0):
            off = sign * ARM_TUBE_DY / 2.0
            p0 = (ARM_R0 * math.cos(a) - off * math.sin(a), ARM_R0 * math.sin(a) + off * math.cos(a), 26.0)
            p1 = (ARM_R1 * math.cos(a) - off * math.sin(a), ARM_R1 * math.sin(a) + off * math.cos(a), 26.0)
            m.tube(p0, p1, ARM_TUBE_R, 6 if lod == 0 else 4, frame, f"conduit_{i + 1:02d}_{'a' if sign < 0 else 'b'}")
        nx, ny = _yaw_point(NODE_R, yaw)
        m.box((nx, ny, NODE_SIZE[2] / 2.0), NODE_SIZE, frame, f"conduit_node_{i + 1:02d}", yaw_deg=yaw)
        if lod == 0:
            m.box((nx, ny, NODE_SIZE[2] - 4.0), (26.0, 26.0, 8.0), status, f"conduit_node_{i + 1:02d}_lamp", yaw_deg=yaw)

    # 8. slim sensor mast: four columns, rungs, a small head and two whips. Not a reactor spire.
    for i in range(4):
        a = math.radians(45.0 + 90.0 * i)
        px, py = 30.0 * math.cos(a), 30.0 * math.sin(a)
        m.box((px, py, (COLLAR_Z + MAST_Z) / 2.0), (14.0, 14.0, MAST_Z - COLLAR_Z), frame, f"mast_column_{i + 1:02d}")
    if lod == 0:
        rungs = int((MAST_Z - COLLAR_Z) // 90.0)
        for r in range(rungs):
            z = COLLAR_Z + 60.0 + r * 90.0
            m.box((0.0, 0.0, z), (84.0, 84.0, 9.0), frame, f"mast_rung_{r + 1:02d}")
    m.box((0.0, 0.0, (MAST_Z + HEAD_Z) / 2.0), (54.0, 54.0, HEAD_Z - MAST_Z), ceramic, "mast_head")
    m.box((0.0, 0.0, HEAD_Z - 14.0), (58.0, 18.0, 12.0), status, "mast_head_lens")
    for i, y in enumerate((-18.0, 18.0)):
        m.box((0.0, y, (HEAD_Z + WHIP_Z) / 2.0), (6.0, 6.0, WHIP_Z - HEAD_Z), frame, f"mast_whip_{i + 1:02d}")
    if lod == 0:
        m.box((30.0, 0.0, HEAD_Z - 34.0), (18.0, 40.0, 10.0), frame, "mast_dish_bracket")

    # 9. damaged state: one upper-tier panel section is gone and its frame ribs are exposed
    if state == "damaged":
        yaw = 22.0
        px, py = _yaw_point(UPPER_R - 10.0, yaw)
        for r in range(4):
            m.box((px, py, DRUM_Z + 16.0 + r * 22.0), (26.0, 120.0, 8.0), frame, f"damage_rib_{r + 1:02d}", yaw_deg=yaw)
        m.box((px, py, DRUM_Z + (UPPER_Z - DRUM_Z) / 2.0), (12.0, 128.0, UPPER_Z - DRUM_Z - 14.0), frame, "damage_bay_back", yaw_deg=yaw)

    # collision: the drum mass and its plinth only. Ramps, conduit arms and nodes stay out of it so
    # they never read as blockers on the approach (REL-ART-016 / REL-ART-030).
    m.collision.append(kit.CollisionBox("plinth", (0.0, 0.0, PLINTH_Z / 2.0), (2 * STEP_R, 2 * STEP_R, PLINTH_Z)))
    m.collision.append(kit.CollisionBox("drum", (0.0, 0.0, (STEP_Z + UPPER_Z) / 2.0),
                                        (2 * DRUM_R * 0.92, 2 * DRUM_R * 0.92, UPPER_Z - STEP_Z)))

    for socket in build_sockets():
        m.sockets.append(socket)
    return m


def build_sockets() -> list:
    out = [kit.Socket("Target_Anchor_Center", (0.0, 0.0, UPPER_Z / 2.0), 0.0,
                      "targeting and selection anchor at the drum's centre of mass")]
    for index, yaw in enumerate(BAY_YAWS, start=1):
        x, y = _yaw_point(DRUM_R + 8.0, yaw)
        out.append(kit.Socket(f"Worker_Bay_{index:02d}", (x, y, STEP_Z), yaw,
                              f"worker bay {index:02d} mouth: emergence and exit reference, facing out of the drum"))
    cx, cy = _yaw_point(DRUM_R + CHUTE_D - 6.0, CHUTE_YAW)
    out.append(kit.Socket("Matter_Intake_Chute", (cx, cy, STEP_Z + CHUTE_H * 0.62), CHUTE_YAW,
                          "Matter delivery mouth: drop-off reference and the chute clatter origin; separate from the worker bays"))
    for i in range(ARMS):
        yaw = 45.0 * i + 22.5
        x, y = _yaw_point(NODE_R, yaw)
        out.append(kit.Socket(f"Conduit_Node_{i + 1:02d}", (x, y, NODE_SIZE[2]), yaw,
                              f"network root {i + 1:02d}: the conduit line to the neighbouring node leaves here"))
    out.append(kit.Socket("Mast_Top", (0.0, 0.0, HEAD_Z), 0.0, "sensor head top: mast effect and status origin"))
    return out


def assemble(lod: int, state: str = "working") -> kit.Mesh:
    return build_main(lod, state)


def contract_inventory(m: kit.Mesh) -> dict:
    comps = m.components()
    return {
        "ceramic_drum": {"contract": 1, "built": sum(1 for c in comps if c in ("drum_lower", "drum_upper")),
                         "tiers": 2, "evidence": "drum_lower + drum_upper + cap"},
        "charcoal_plinth": {"contract": 1, "built": sum(1 for c in comps if c.startswith("plinth"))},
        "worker_bays": {"contract": 3, "built": sum(1 for c in comps if c.endswith("_interior") and c.startswith("bay_")),
                        "ramps": sum(1 for c in comps if c.endswith("_ramp"))},
        "matter_intake_chute": {"contract": 1, "built": sum(1 for c in comps if c == "chute_housing"),
                                "evidence": "separate housing with an angled throat and a lit slot; not a bay"},
        "conduit_roots": {"contract": ARMS, "built": sum(1 for c in comps if c.startswith("conduit_node_") and not c.endswith("_lamp"))},
        "central_mast": {"contract": 1, "built": 1 if any(c == "mast_head" for c in comps) else 0,
                         "columns": sum(1 for c in comps if c.startswith("mast_column_")),
                         "note": "slim sensor core, not a reactor spire (review decision A)"},
        "sockets": {"contract": ["Target_Anchor_Center", "Worker_Bay_01", "Worker_Bay_02", "Worker_Bay_03",
                                 "Matter_Intake_Chute"] + [f"Conduit_Node_{i:02d}" for i in range(1, ARMS + 1)] + ["Mast_Top"],
                    "built": [s.name for s in m.sockets]},
        "states": {"contract": list(STATES), "built": list(STATES)},
    }


def measurements(m: kit.Mesh) -> dict:
    (x0, y0, z0), (x1, y1, z1) = m.bounds()
    return {
        "footprint_tiles": [FOOTPRINT_TILES, FOOTPRINT_TILES],
        "footprint_cm": F,
        "bounds_cm": [[round(x0, 2), round(y0, 2), round(z0, 2)], [round(x1, 2), round(y1, 2), round(z1, 2)]],
        "height_cm": round(z1, 2),
        "drum_diameter_cm": 2 * DRUM_R,
        "drum_diameter_over_footprint": round(2 * DRUM_R / F, 4),
        "drum_height_over_diameter": round(UPPER_Z / (2 * DRUM_R), 4),
        "mast_top_over_drum_diameter": round(WHIP_Z / (2 * DRUM_R), 4),
        "everything_inside_the_footprint": max(abs(x0), abs(x1), abs(y0), abs(y1)) <= HALF + 1e-6,
        "max_radius_cm": round(max(abs(x0), abs(x1), abs(y0), abs(y1)), 2),
    }


# --- export -------------------------------------------------------------------------------------
def sha256_file(path: str) -> str:
    return hashlib.sha256(open(path, "rb").read()).hexdigest()


def export(evidence_dir: str, out_dir: str) -> dict:
    os.makedirs(out_dir, exist_ok=True)
    review = os.path.join(evidence_dir, "review")
    os.makedirs(review, exist_ok=True)
    outputs, review_rows = [], []
    for lod in (0, 1):
        m = assemble(lod, "working")
        base = os.path.join(out_dir, f"{ASSET}_LOD{lod}")
        extras = {"production_id": PRODUCTION_ID, "package_id": PACKAGE_ID, "revision": REVISION, "lod": lod,
                  "author": AUTHOR, "frame": "Unreal +X forward, +Y right, +Z up; centimetres"}
        glb = m.write_glb(base + ".glb", extras=extras, include_collision=(lod == 0))
        obj = m.write_obj(base + ".obj", header_lines=[f"{ASSET} LOD{lod}", f"Revision {REVISION}",
                                                       "Unreal +X forward, +Y right, +Z up; centimetres; no axis conversion"])
        outputs.append({"mesh": ASSET, "lod": lod, "path": os.path.relpath(base + ".glb", HERE), "sha256": glb,
                        "triangles": m.triangle_count(), "bounds_cm": [list(p) for p in m.bounds()],
                        "section_slot_names": kit.slot_names_in_primitive_order(m) if hasattr(kit, "slot_names_in_primitive_order") else m.slots,
                        "by_slot": m.triangle_count_by("slot"),
                        "collision_boxes": [{"name": b.name, "center_cm": list(b.center), "size_cm": list(b.size)} for b in m.collision] if lod == 0 else [],
                        "sockets": [{"name": s.name, "position_cm": [round(v, 2) for v in s.position],
                                     "yaw_deg": s.yaw_deg, "purpose": s.purpose} for s in m.sockets]})
        outputs.append({"mesh": ASSET, "lod": lod, "path": os.path.relpath(base + ".obj", HERE), "sha256": obj})
    for state in STATES:
        for lod in (0, 1):
            if lod == 1 and state != "working":
                continue
            m = assemble(lod, state)
            path = os.path.join(review, f"{ASSET}_assembly_{state}_LOD{lod}.obj")
            digest = m.write_obj(path, header_lines=[f"{ASSET} review assembly: {state} LOD{lod}", f"Revision {REVISION}"])
            review_rows.append({"state": state, "lod": lod, "path": os.path.relpath(path, evidence_dir), "sha256": digest,
                                "triangles": m.triangle_count()})
    return {"outputs": outputs, "review_assemblies": review_rows}


def manifest(exported: dict) -> dict:
    m0, m1 = assemble(0, "working"), assemble(1, "working")
    return {
        "author": AUTHOR, "creator": AUTHOR, "package_id": PACKAGE_ID, "production_asset_id": PRODUCTION_ID,
        "asset_name": ASSET, "planned_unreal_folder": PLANNED_FOLDER, "revision": REVISION,
        "kit_revision": getattr(kit, "KIT_REVISION", "unknown"),
        "stage": "BLOCKOUT (concept-v1)",
        "stage_boundary": "Editable source and review evidence only. Not a gate pass, not an Unreal integration, not owner acceptance.",
        "units": {"length": "centimetres", "axes": "+X forward (bay arc), +Y right, +Z up",
                  "pivot": "ground-contact centre", "nanite": False},
        "scale_basis": {
            "footprint": "5x5 tiles = 1,000 cm (buildings record mc_anchor.footprint_cells)",
            "concept": "concept-fidelity.md: squat two-tier drum, low dome, slim mast, arms out to the footprint edge",
            "note": ("The drum is 600 cm across (0.60 of the footprint) rather than filling it, because the candidate's "
                     "conduit arms and bay ramps extend well past the drum and everything must stay inside the 5x5 square. "
                     "concept-fidelity.md's 'D = 1,000 cm drum diameter' could not hold both readings; the drum diameter is "
                     "the measured ratio and the footprint is the outer bound."),
        },
        "material_slots": [CERAMIC, FRAME, STATUS],
        "material_slot_policy": "Pale civic ceramic, charcoal frame, cyan status. Numerals, hazard striping and plate seams are texture, not geometry.",
        "budgets": {
            "lod0_triangles": m0.triangle_count(), "lod1_triangles": m1.triangle_count(),
            "whole_asset_lod0_triangles": m0.triangle_count(), "whole_asset_lod1_triangles": m1.triangle_count(),
            "lod0_cap": 8000, "lod1_cap": 3500,
            "cap_source": ("CONFIRMED by the owner ruling 2026-09-07: the tighter Meridian limits of REL-ART-028 govern "
                           "(8,000 / 3,500) rather than REL-BLD-015.MC.CORE's 12,000 / 4,500, matching the recorded "
                           "decision, and the ceilings apply to the COMPLETE asset including any articulated components. "
                           "There is one mesh today, so the whole-asset figures equal the primary mesh's."),
            "lod0_within_cap": m0.triangle_count() <= 8000, "lod1_within_cap": m1.triangle_count() <= 3500,
        },
        "pending_requirements": [
            {"requirement": "REL-BLD-015.MC.CORE .ANIM_RIG — 4-bone rig for core exhaust vanes and data-grid extensions, "
                            "plus the required damage presentation",
             "status": "PENDING (owner ruling 2026-09-07)",
             "ruling": ("A static mesh is acceptable for this blockout and is NOT closure of the production requirement. The "
                        "pipeline specifies a static primary structure with role-required articulated components, so the "
                        "four-bone requirement stays open for a small articulated assembly. Its absence from the concept is "
                        "missing reference detail, not grounds to waive the card."),
             "consequence": "This asset must not be marked compliant until the assembly is implemented or the card is explicitly amended.",
             "counts_toward": "the same 8,000 / 3,500 whole-asset ceilings"},
        ],
        "concept_measurements": measurements(m0),
        "component_inventory": contract_inventory(m0),
        "states": {s: "review assembly" for s in STATES},
        "outputs": exported["outputs"], "review_assemblies": exported["review_assemblies"],
        "tools": {"mesh_kit": "ArtSource/tools/ebs_meshkit.py", "renderer": "ArtSource/tools/ebs_render.py"},
        "acceptance": {"art": "NOT_EVALUATED", "gameplay": "NOT_EVALUATED",
                       "technical": "BLOCKED — the card's 4-bone articulated assembly is pending (see pending_requirements)",
                       "owner": "NOT_ACCEPTED"},
        "source_bindings": {
            "candidate": "BuildArtifacts/Evidence/concept-discovery-20260906/anchor-review/anchor-candidate.png",
            "concepts": ["EBS-CON-MER-BLD-001 (REWORK)", "EBS-CON-MER-BLD-005 (REWORK)"],
            "canon": "DevelopmentBible.md line 513 (SPEC-BLD-015.MC.ANCHOR)",
            "gameplay": "Content/Data/Source/buildings.json mc_anchor",
        },
    }


def write_scenes(evidence_dir: str) -> list:
    scenes_dir = os.path.join(evidence_dir, "scenes")
    os.makedirs(scenes_dir, exist_ok=True)
    base = {
        "author": AUTHOR, "srgb": True,
        "materials": {CERAMIC: [0.72, 0.70, 0.66], FRAME: [0.05, 0.05, 0.055], STATUS: [0.16, 0.86, 0.96],
                      "_default": [0.5, 0.5, 0.5]},
        "emissive": [STATUS], "width": 1920, "height": 1080, "background": [0.84, 0.82, 0.78],
        "light": {"direction": [0.55, -0.35, -0.76], "ambient": 0.3, "key": 0.8, "color": [1.0, 0.82, 0.62],
                  "fill_color": [0.48, 0.6, 0.88], "fill": 0.22},
        "ground": {"tile_cm": 200, "tiles": 40, "color": [0.028, 0.028, 0.032], "grid_color": [0.07, 0.07, 0.08],
                   "footprint_cm": [F, F], "footprint_color": [0.16, 0.86, 0.96]},
        "reference_figure": {"height_cm": 180, "position": [560, 560, 0], "color": [0.92, 0.55, 0.2]},
    }
    ortho = [{"name": "front", "type": "ortho", "from": "+X", "edges": True, "margin": 1.12, "target": [0, 0, 420]},
             {"name": "right", "type": "ortho", "from": "+Y", "edges": True, "margin": 1.12, "target": [0, 0, 420]},
             {"name": "rear", "type": "ortho", "from": "-X", "edges": True, "margin": 1.12, "target": [0, 0, 420]},
             {"name": "top", "type": "ortho", "from": "+Z", "image_up": "+X", "edges": True, "margin": 1.15, "target": [0, 0, 0]}]
    tactical = [{"name": "tactical_default", "type": "persp", "pitch_deg": -48, "yaw_deg": -45, "arm_cm": 3800, "fov_deg": 55, "target": [0, 0, 200]},
                {"name": "tactical_gameplay", "type": "persp", "pitch_deg": -60, "yaw_deg": -45, "arm_cm": 3800, "fov_deg": 55, "target": [0, 0, 200]},
                {"name": "tactical_near", "type": "persp", "pitch_deg": -42, "yaw_deg": 20, "arm_cm": 1800, "fov_deg": 55, "target": [200, 0, 180]},
                {"name": "concept_quarter", "type": "persp", "pitch_deg": -28, "yaw_deg": 135, "arm_cm": 1250, "fov_deg": 50, "target": [40, 0, 250]},
                {"name": "tactical_mono", "type": "persp", "pitch_deg": -60, "yaw_deg": -45, "arm_cm": 3800, "fov_deg": 55, "target": [0, 0, 200], "grayscale": True}]
    written = []
    for state in STATES:
        scene = dict(base)
        scene["meshes"] = [{"obj": f"../review/{ASSET}_assembly_{state}_LOD0.obj"}]
        scene["views"] = ortho + tactical
        path = os.path.join(scenes_dir, f"{state}.json")
        with open(path, "w", encoding="utf-8") as handle:
            json.dump(scene, handle, indent=1)
        written.append(path)
    lod1 = dict(base)
    lod1["meshes"] = [{"obj": f"../review/{ASSET}_assembly_working_LOD1.obj"}]
    lod1["views"] = ortho + [tactical[0]]
    path = os.path.join(scenes_dir, "lod1.json")
    with open(path, "w", encoding="utf-8") as handle:
        json.dump(lod1, handle, indent=1)
    written.append(path)
    return written


def build_all(evidence_dir: str, out_dir: str) -> dict:
    exported = export(evidence_dir, out_dir)
    return manifest(exported)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--evidence-dir", required=True)
    parser.add_argument("--check", action="store_true", help="rebuild into a temporary directory and compare hashes; writes nothing")
    args = parser.parse_args()
    manifest_path = os.path.join(HERE, "build-manifest.json")
    if args.check:
        with tempfile.TemporaryDirectory() as tmp:
            ev = os.path.join(tmp, "evidence")
            os.makedirs(ev, exist_ok=True)
            fresh = build_all(ev, os.path.join(tmp, "export"))
        with open(manifest_path, "r", encoding="utf-8") as handle:
            saved = json.load(handle)
        drift, missing = [], []
        # Compare by file NAME, not by path: --check builds into a temporary directory, so the paths
        # recorded there are a long ../ chain back out of /var/folders and would never match.
        saved_hashes = {os.path.basename(o["path"]): o["sha256"] for o in saved["outputs"]}
        for o in fresh["outputs"]:
            name = os.path.basename(o["path"])
            if name not in saved_hashes:
                missing.append(name)
            elif saved_hashes[name] != o["sha256"]:
                drift.append(name)
        saved_review = {r["path"]: r["sha256"] for r in saved.get("review_assemblies", [])}
        for r in fresh["review_assemblies"]:
            if r["path"] not in saved_review:
                missing.append(r["path"])
            elif saved_review[r["path"]] != r["sha256"]:
                drift.append(r["path"])
        if saved.get("revision") != REVISION:
            drift.append(f"revision {saved.get('revision')} != {REVISION}")
        print(json.dumps({"check": "ok" if not drift and not missing else "drift", "revision": REVISION,
                          "manifest_revision": saved.get("revision"), "drift": drift, "missing": missing,
                          "compared": {"outputs": len(fresh["outputs"]), "review_assemblies": len(fresh["review_assemblies"])}}))
        return 0 if not drift and not missing else 1
    os.makedirs(args.evidence_dir, exist_ok=True)
    data = build_all(args.evidence_dir, os.path.join(HERE, "export"))
    scenes = write_scenes(args.evidence_dir)
    data["review"] = {"scenes": [os.path.relpath(s, args.evidence_dir) for s in scenes]}
    with open(manifest_path, "w", encoding="utf-8") as handle:
        json.dump(data, handle, indent=1, sort_keys=True)
    print(json.dumps({"revision": REVISION, "lod0": data["budgets"]["lod0_triangles"],
                      "lod1": data["budgets"]["lod1_triangles"], "height_cm": data["concept_measurements"]["height_cm"],
                      "inside_footprint": data["concept_measurements"]["everything_inside_the_footprint"],
                      "sockets": len(data["outputs"][0]["sockets"]), "scenes": len(scenes)}))
    return 0


if __name__ == "__main__":
    sys.exit(main())
