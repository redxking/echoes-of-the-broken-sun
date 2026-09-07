#!/usr/bin/env python3
"""Deterministic source generator for EBS-KHA-BLD-001 — the Kharuun Assemblies Memory Hearth.

Author: Angelis Pseftis. Creator: Angelis Pseftis.

Built to `concept-fidelity.md`: a wide grown dome of banded strata, five arched worker hollows at the
base, a matter-intake cleft on the right flank whose silhouette differs from the arches, and a crown of
seven rooted adaptation spires. Faceted throughout with zero organic smoothing (`REL-ART-029`). Static
mesh — canon's only motion is a glow breath and a cleft settle, neither of them geometry.

Usage:
  python3 build_hearth.py --evidence-dir "<root>/EBS-KHA-BLD-001"
  python3 build_hearth.py --evidence-dir "<root>/EBS-KHA-BLD-001" --check

Units: centimetres. +X forward, +Y right, +Z up. Pivot at the dome's ground-contact centre. Nanite off.
Nothing below z = 0; nothing outside the 5x5 tile footprint.
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

AUTHOR = "Angelis Pseftis"
PACKAGE_ID = "EBS-PKG-KA-MEMORY-HEARTH"
PRODUCTION_ID = "EBS-KHA-BLD-001"
ASSET = "SM_EBS_KHA_BLD_001"
PLANNED_FOLDER = "/Game/Echoes/Production/KHA/BLD/EBS_KHA_BLD_001/"
REVISION = "ebs-kha-bld-001-concept-v1"

STRATA = "MI_EBS_KHA_Strata"      # faceted banded mineral-ceramic: the shell, spires and cleft lining
FIBRE = "MI_EBS_KHA_Fibre"        # the tended thresholds and the cleft apron
AMBER = "MI_EBS_KHA_Amber"        # interior glow, growth seams, spire veins — the only emissive

TILE_CM = 200.0
FOOTPRINT_TILES = 5
F = TILE_CM * FOOTPRINT_TILES          # 1000 cm square
HALF = F / 2.0

BASE_R = 440.0                         # dome base radius: 880 cm across; the thresholds, cleft apron
                                       # and delivered matter all have to fit inside the 1000 cm square too
SHELL_Z = 387.0                        # 0.44 of the base width, as traced
TOTAL_Z = 607.0                        # 0.69 of the base width: the tallest spire tip
BANDS = 8                              # stacked strata bands; each steps in as it rises
HOLLOW_COUNT = 5
HOLLOW_W, HOLLOW_Z = 118.0, 156.0      # arched worker hollows at the base
HOLLOW_YAWS = (-52.0, -26.0, 0.0, 26.0, 52.0)   # spread across the front arc, facing -X
CLEFT_YAW = 96.0                       # the matter-intake cleft on the right flank
CLEFT_W, CLEFT_Z, CLEFT_LIP = 232.0, 96.0, 64.0  # wider and lower than an arch: a different silhouette
SPIRES = (                             # (yaw, radius from centre, height above the shell at that radius)
    (0.0, 0.0, 246.0), (-38.0, 96.0, 168.0), (54.0, 104.0, 176.0),
    (-115.0, 172.0, 122.0), (128.0, 182.0, 116.0),
    (6.0, 212.0, 86.0), (188.0, 214.0, 80.0))   # the outer pair sits across the crown, not beside it
SPIRE_SPREAD = 528.0                   # 0.55 of the base width, as traced
STATES = ("working", "damaged", "destroyed")


def band_profile(index: int, bands: int = BANDS):
    """Elliptical shell profile: (radius, z_bottom, z_top) for one strata band.

    The profile is sampled, not smoothed — each band is a straight-sided prism, which is what
    'faceted with zero organic smoothing' means for a dome.
    """
    z0 = SHELL_Z * index / bands
    z1 = SHELL_Z * (index + 1) / bands
    # radius taken at the band's MID height: sampling the ellipse at the top made each band a tall
    # vertical cylinder and the stack read as a stepped cake rather than a dome
    zm = (z0 + z1) / 2.0
    r = BASE_R * math.sqrt(max(0.0, 1.0 - (zm / SHELL_Z) ** 2 * 0.94))
    return r, z0, z1


def shell_radius_at(z: float) -> float:
    for i in range(BANDS):
        r, z0, z1 = band_profile(i)
        if z0 <= z <= z1:
            return r
    return band_profile(BANDS - 1)[0]


def _yaw_point(radius: float, yaw_deg: float):
    a = math.radians(yaw_deg)
    return (radius * math.cos(a), radius * math.sin(a))



def _spire(m: kit.Mesh, cx: float, cy: float, z0: float, height: float, base_r: float,
           sides: int, slot: int, component: str):
    """A tapered rooted spire: a wide rooted foot, a narrow waist, then a point.

    ``tube`` gives a constant radius, which read as a rectangular block rather than the concept's
    graceful tapered spire, so the spire is built as two stacked frusta closed by an apex fan.
    """
    waist_z = z0 + height * 0.34
    ring0 = [(cx + x, cy + y, z0) for x, y in kit.regular_polygon(base_r, sides)]
    ring1 = [(cx + x, cy + y, waist_z) for x, y in kit.regular_polygon(base_r * 0.44, sides)]
    apex = (cx, cy, z0 + height)
    for k in range(sides):
        j = (k + 1) % sides
        outward = (ring0[k][0] + ring0[j][0]) / 2.0 - cx, (ring0[k][1] + ring0[j][1]) / 2.0 - cy, 0.0
        m.add_polygon([ring0[k], ring0[j], ring1[j], ring1[k]], slot, component, normal=outward)
        m.add_polygon([ring1[k], ring1[j], apex], slot, component, normal=outward)
    m.add_polygon(list(ring0), slot, component, normal=(0.0, 0.0, -1.0))


def build_body(lod: int, state: str = "working") -> kit.Mesh:
    if state not in STATES:
        raise ValueError(f"unknown state {state!r}; expected one of {STATES}")
    m = kit.Mesh(ASSET)
    strata = m.slot(STRATA)
    fibre = m.slot(FIBRE)
    amber = m.slot(AMBER)
    fine = lod == 0
    sides = 12 if fine else 8
    collapsed = state == "destroyed"

    # 1. the shell: stacked strata bands, each a faceted prism stepping in above the one below
    for i in range(BANDS):
        r, z0, z1 = band_profile(i)
        if collapsed:
            # ceramic collapse inward: the upper bands fall in and settle low
            fall = (i / BANDS) ** 1.4
            z0, z1 = z0 * (1.0 - 0.72 * fall), z1 * (1.0 - 0.72 * fall)
            r = r * (1.0 - 0.10 * fall)
        m.prism(kit.regular_polygon(r, sides, phase_deg=15.0 * i), z0, z1, strata, f"strata_{i + 1:02d}",
                cap_bottom=(i == 0), cap_top=(i == BANDS - 1))
        if fine and not collapsed and i < BANDS - 1:
            # a narrow growth seam lighting the step between bands
            m.ring(kit.regular_polygon(r + 1.5, sides, phase_deg=15.0 * i),
                   kit.regular_polygon(r - 1.5, sides, phase_deg=15.0 * i), z1 - 2.5, z1, amber,
                   f"growth_seam_{i + 1:02d}")

    if collapsed:
        # a low rubble ring where the shell came down, and nothing else: no hollows, cleft or spires
        for k in range(8 if fine else 5):
            yaw = 360.0 * k / (8 if fine else 5)
            px, py = _yaw_point(BASE_R * 0.82, yaw)
            m.box((px, py, 34.0), (150.0, 110.0, 68.0), strata, f"rubble_{k + 1:02d}", yaw_deg=yaw)
        m.collision.append(kit.CollisionBox("mound", (0.0, 0.0, 60.0), (2 * BASE_R, 2 * BASE_R, 120.0)))
        return m

    # 2. arched worker hollows at the base. The opening is framed by two jambs and a lintel standing
    #    proud of the shell, with the lit interior panel set BEHIND their outer face. A first pass made
    #    the frame one solid block, which enclosed the panel and read as a plate stuck on the wall.
    for index, yaw in enumerate(HOLLOW_YAWS, start=1):
        yaw_from_front = 180.0 + yaw       # the hollows face -X, away from the cleft flank
        r = shell_radius_at(HOLLOW_Z * 0.5)
        a = math.radians(yaw_from_front)
        out = (math.cos(a), math.sin(a))
        side = (-math.sin(a), math.cos(a))

        def at(radius, lateral, z):
            return (out[0] * radius + side[0] * lateral, out[1] * radius + side[1] * lateral, z)

        for sign, tag in ((-1.0, "l"), (1.0, "r")):
            m.box(at(r + 16.0, sign * (HOLLOW_W / 2.0 + 13.0), HOLLOW_Z / 2.0), (30.0, 26.0, HOLLOW_Z),
                  strata, f"hollow_{index:02d}_jamb_{tag}", yaw_deg=yaw_from_front)
        m.box(at(r + 16.0, 0.0, HOLLOW_Z + 15.0), (30.0, HOLLOW_W + 52.0, 30.0), strata,
              f"hollow_{index:02d}_lintel", yaw_deg=yaw_from_front)
        m.box(at(r + 14.0, 0.0, HOLLOW_Z - 16.0), (26.0, HOLLOW_W - 4.0, 34.0), strata,
              f"hollow_{index:02d}_arch_head", yaw_deg=yaw_from_front)
        m.box(at(r - 6.0, 0.0, HOLLOW_Z / 2.0 - 18.0), (6.0, HOLLOW_W - 40.0, HOLLOW_Z - 62.0), amber,
              f"hollow_{index:02d}_interior", yaw_deg=yaw_from_front)
        m.box(at(r + 34.0, 0.0, 5.0), (44.0, HOLLOW_W + 16.0, 10.0), fibre,
              f"hollow_{index:02d}_threshold", yaw_deg=yaw_from_front)

    # 3. the matter-intake cleft: a low receiving lip into a mineral-lined recess on the right flank.
    #    Wider and lower than an arch, with an apron in front of it, so the silhouette differs.
    r = shell_radius_at(CLEFT_Z * 0.5)
    cx, cy = _yaw_point(r + 14.0, CLEFT_YAW)
    m.box((cx, cy, CLEFT_Z / 2.0 + 26.0), (44.0, CLEFT_W + 56.0, CLEFT_Z + 52.0), strata, "cleft_hood",
          yaw_deg=CLEFT_YAW)
    m.box((cx, cy, CLEFT_Z / 2.0), (48.0, CLEFT_W, CLEFT_Z), strata, "cleft_recess", yaw_deg=CLEFT_YAW)
    lx, ly = _yaw_point(r + 14.0, CLEFT_YAW)
    m.box((lx, ly, 7.0), (CLEFT_LIP + 4.0, CLEFT_W - 20.0, 14.0), fibre, "cleft_lip", yaw_deg=CLEFT_YAW)
    if fine:
        for k, off in enumerate((-64.0, -8.0, 52.0)):
            mx, my = _yaw_point(r + 28.0, CLEFT_YAW)
            m.box((mx + off * math.sin(math.radians(CLEFT_YAW)), my - off * math.cos(math.radians(CLEFT_YAW)), 20.0),
                  (34.0, 30.0, 28.0), strata, f"raw_matter_{k + 1:02d}", yaw_deg=CLEFT_YAW + 12.0 * k)

    # 4. the crown: seven tapered spires rooted IN the shell strata, one tallest at the centre
    for index, (yaw, radius, rise) in enumerate(SPIRES, start=1):
        px, py = _yaw_point(radius, yaw)
        base_z = shell_z_at(radius) - 26.0
        tip = base_z + rise
        dark = state == "damaged" and index == 2      # damaged: one spire dark, per canon
        spire_sides = 6 if fine else 4
        _spire(m, px, py, base_z, tip - base_z, 44.0 if radius == 0.0 else 34.0, spire_sides,
               strata, f"spire_{index:02d}")
        if fine:
            m.tube((px, py, base_z + rise * 0.10), (px, py, base_z + rise * 0.42), 7.0, 4,
                   strata if dark else amber, f"spire_{index:02d}_vein")

    if state == "damaged":
        # cracked strata: a dark fracture stepping down the shell's front flank
        for k, (dz, dr) in enumerate(((0.72, 0.0), (0.50, 22.0), (0.28, 44.0))):
            z = SHELL_Z * dz
            rr = shell_radius_at(z)
            px, py = _yaw_point(rr + 4.0, 196.0 + dr * 0.2)
            m.box((px, py, z), (26.0, 30.0, 78.0), strata, f"crack_{k + 1:02d}", yaw_deg=196.0)

    m.collision.append(kit.CollisionBox("shell_lower", (0.0, 0.0, SHELL_Z * 0.25),
                                        (2 * BASE_R * 0.94, 2 * BASE_R * 0.94, SHELL_Z * 0.5)))
    m.collision.append(kit.CollisionBox("shell_upper", (0.0, 0.0, SHELL_Z * 0.72),
                                        (2 * BASE_R * 0.52, 2 * BASE_R * 0.52, SHELL_Z * 0.44)))
    return m


def shell_z_at(radius: float) -> float:
    """Height of the shell surface at a given radius from the centre (the inverse of the profile)."""
    best = 0.0
    for i in range(BANDS):
        r, _z0, z1 = band_profile(i)
        if r >= radius:
            best = z1
    return best if best else SHELL_Z


SOCKETS = {
    "Target_Anchor_Center": ((0.0, 0.0, SHELL_Z * 0.55), 0.0, "targeting and selection anchor inside the shell"),
    "Unit_Emergence": (None, 180.0, "Tenders emerge from the centre worker hollow"),
    "Rally_Default": ((-BASE_R - 120.0, 0.0, 0.0), 180.0, "default emergence rally point, clear of the thresholds"),
    "Matter_Dropoff": (None, CLEFT_YAW, "the intake cleft mouth: where workers deliver matter"),
    "Adaptation_Crown": ((0.0, 0.0, TOTAL_Z - 20.0), 0.0, "the adaptation-root effect anchor at the tallest spire"),
}


def assemble(lod: int, state: str = "working") -> kit.Mesh:
    m = build_body(lod, state)
    r_hollow = shell_radius_at(HOLLOW_Z * 0.5)
    r_cleft = shell_radius_at(CLEFT_Z * 0.5)
    hx, hy = _yaw_point(r_hollow + 56.0, 180.0)
    mx, my = _yaw_point(r_cleft + 96.0, CLEFT_YAW)
    resolved = {
        "Unit_Emergence": (hx, hy, 6.0),
        "Matter_Dropoff": (mx, my, 10.0),
    }
    for name, (pos, yaw, purpose) in SOCKETS.items():
        m.sockets.append(kit.Socket(name, resolved.get(name, pos), yaw, purpose))
    return m


def contract_inventory(m: kit.Mesh) -> dict:
    comps = m.components()
    return {
        "banded_shell": {"contract": f"{BANDS} stacked strata bands, faceted",
                         "built": sum(1 for c in comps if c.startswith("strata_"))},
        "worker_hollows": {"contract": HOLLOW_COUNT,
                           "jambs": sum(1 for c in comps if "_jamb_" in c),
                           "lintels": sum(1 for c in comps if c.endswith("_lintel")),
                           "interiors": sum(1 for c in comps if c.endswith("_interior")),
                           "arch_heads": sum(1 for c in comps if c.endswith("_arch_head")),
                           "thresholds": sum(1 for c in comps if c.endswith("_threshold"))},
        "intake_cleft": {"contract": "one cleft, wider and lower than an arch",
                         "hood": "cleft_hood" in comps, "recess": "cleft_recess" in comps,
                         "lip": "cleft_lip" in comps,
                         "raw_matter": sum(1 for c in comps if c.startswith("raw_matter_"))},
        "adaptation_crown": {"contract": len(SPIRES),
                             "built": sum(1 for c in comps if c.startswith("spire_") and "_vein" not in c),
                             "veins": sum(1 for c in comps if c.endswith("_vein"))},
        "growth_seams": {"contract": "amber seams between the bands",
                         "built": sum(1 for c in comps if c.startswith("growth_seam_"))},
        "sockets": {"contract": sorted(SOCKETS), "built": sorted(s.name for s in m.sockets)},
        "states": {"contract": list(STATES), "built": list(STATES)},
    }


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


def measurements(m: kit.Mesh) -> dict:
    (x0, y0, z0), (x1, y1, z1) = m.bounds()
    base = 2 * BASE_R
    spires = [c for c in m.components() if c.startswith("spire_") and "_vein" not in c]
    spread = 0.0
    if spires:
        for axis in (0, 1):
            vals = [v for c in spires for v in (m.component_bounds(c)[0][axis], m.component_bounds(c)[1][axis])]
            spread = max(spread, max(vals) - min(vals))
    return {
        "footprint_tiles": [FOOTPRINT_TILES, FOOTPRINT_TILES], "footprint_cm": F,
        "bounds_cm": [[round(x0, 2), round(y0, 2), round(z0, 2)], [round(x1, 2), round(y1, 2), round(z1, 2)]],
        "height_cm": round(z1, 2), "base_width_cm": base,
        "shell_height_over_base_width": round(SHELL_Z / base, 4),
        "total_height_over_base_width": round(z1 / base, 4),
        "spire_spread_over_base_width": round(spread / base, 4),
        "spires_over_total_height": round((z1 - SHELL_Z) / z1, 4),
        "breadth_greater_than_shell_height": base > SHELL_Z,
        "everything_inside_the_footprint": max(abs(x0), abs(x1), abs(y0), abs(y1)) <= HALF + 1e-6,
        "max_extent_cm": round(max(abs(x0), abs(x1), abs(y0), abs(y1)), 2),
    }


def export(evidence_dir: str, out_dir: str) -> dict:
    os.makedirs(out_dir, exist_ok=True)
    review = os.path.join(evidence_dir, "review")
    os.makedirs(review, exist_ok=True)
    outputs, review_rows = [], []
    for lod in (0, 1):
        m = assemble(lod, "working")
        base = os.path.join(out_dir, f"{ASSET}_LOD{lod}")
        extras = {"production_id": PRODUCTION_ID, "package_id": PACKAGE_ID, "revision": REVISION,
                  "lod": lod, "author": AUTHOR}
        # collision belongs in the LOD0 file ONLY: StaticMeshEditorSubsystem.import_lod does not
        # honour UBX_ naming, so a LOD1 file carrying collision folds those boxes into the render
        # mesh (it cost this asset 24 phantom LOD1 triangles before the first import caught it)
        digest = m.write_glb(base + ".glb", extras=extras, include_collision=(lod == 0))
        outputs.append({"mesh": ASSET, "lod": lod, "kind": "static", "path": os.path.relpath(base + ".glb", HERE),
                        "sha256": digest, "triangles": m.triangle_count(),
                        "bounds_cm": [list(p) for p in m.bounds()],
                        "section_slot_names": m.slots, "by_slot": m.triangle_count_by("slot"),
                        "sockets": [{"name": s.name, "position_cm": [round(v, 2) for v in s.position],
                                     "yaw_deg": s.yaw_deg, "purpose": s.purpose} for s in m.sockets],
                        "collision_boxes": [{"name": c.name, "center_cm": [round(v, 2) for v in c.center],
                                             "size_cm": [round(v, 2) for v in c.size]} for c in m.collision]})
        obj = m.write_obj(base + ".obj", header_lines=[f"{ASSET} LOD{lod} (working)", f"Revision {REVISION}",
                                                       "Unreal +X forward, +Y right, +Z up; centimetres"])
        outputs.append({"mesh": ASSET, "lod": lod, "kind": "OBJ", "path": os.path.relpath(base + ".obj", HERE), "sha256": obj})
    for state in STATES:
        m = assemble(0, state)
        path = os.path.join(review, f"{ASSET}_{state}_LOD0.obj")
        review_rows.append({"name": state, "path": os.path.relpath(path, evidence_dir),
                            "sha256": m.write_obj(path, header_lines=[f"{ASSET} {state}", f"Revision {REVISION}"]),
                            "triangles": m.triangle_count(),
                            "height_cm": round(m.bounds()[1][2], 2),
                            "max_extent_cm": round(max(abs(v) for p in m.bounds() for v in p[:2]), 2)})
    m1 = assemble(1, "working")
    path = os.path.join(review, f"{ASSET}_working_LOD1.obj")
    review_rows.append({"name": "working_lod1", "path": os.path.relpath(path, evidence_dir),
                        "sha256": m1.write_obj(path, header_lines=[f"{ASSET} LOD1"]), "triangles": m1.triangle_count()})
    return {"outputs": outputs, "review": review_rows}


def manifest(exported: dict) -> dict:
    m0 = assemble(0, "working")
    m1 = assemble(1, "working")
    amber = slot_area_fraction(m0, AMBER)
    return {
        "author": AUTHOR, "creator": AUTHOR, "package_id": PACKAGE_ID, "production_asset_id": PRODUCTION_ID,
        "asset_name": ASSET, "planned_unreal_folder": PLANNED_FOLDER, "revision": REVISION,
        "kit_revision": getattr(kit, "KIT_REVISION", "unknown"),
        "stage": "BLOCKOUT (concept-v1)",
        "stage_boundary": "Editable source and review evidence only. Not a gate pass, not an Unreal integration, not owner acceptance.",
        "units": {"length": "centimetres", "axes": "+X forward, +Y right, +Z up",
                  "pivot": "dome ground-contact centre", "nanite": False},
        "scale_basis": {"canon": "SPEC-BLD-016.KA.HEARTH (Bible line 555): a wide grown dome of banded strata with arched worker hollows, a matter-intake cleft and a crown of rooted adaptation spires",
                        "footprint_tiles": [FOOTPRINT_TILES, FOOTPRINT_TILES], "footprint_cm": F,
                        "source": "Content/Data/Source/buildings.json ka_memory_hearth.footprint_cells"},
        "material_slots": [STRATA, FIBRE, AMBER],
        "material_slot_policy": ("Faceted banded mineral-ceramic for the shell, spires and cleft lining; a tended-surface slot for "
                                 "the thresholds and the cleft apron; amber for the interior glow, the growth seams and the spire "
                                 "veins, which is the only emissive. REL-ART-029 forbids organic smoothing, so every band is a "
                                 "straight-sided prism rather than a subdivided dome."),
        "provisional_contract": {
            "card": "REL-BLD-016.KA.HEARTH.ASSET in ArtSource/kharuun-asset-cards.json (rendered to kharuun-asset-cards.md)",
            "status": ("PROVISIONAL. Docs/Requirements.md section 18.2 still carries Meridian cards only. The owner confirmed "
                       "8,000/3,500 as this asset's provisional ceilings on 2026-09-07 and directed that a dedicated Kharuun card "
                       "be authored; that card lives in this worktree and has NOT been incorporated into the authoritative "
                       "requirements. It is not an existing authoritative per-asset requirement and not owner acceptance."),
            "binding_requirements": ["REL-ART-029 (faceted basalt, zero organic smoothing, 2048^2 PBR, amber <= 15.0% surface area)",
                                     "REL-BLD-016.KA.HEARTH (1,300 health, 800 cm sight, +12 logistics, 5x5 footprint, produces Tenders)"],
            "provisional_bounds": {"lod0_triangles": 8000, "lod1_triangles": 3500,
                                   "basis": "confirmed by the owner on 2026-09-07 as this asset's provisional ceilings"}},
        "budgets": {"lod0_triangles": m0.triangle_count(), "lod1_triangles": m1.triangle_count(),
                    "lod0_cap": 8000, "lod1_cap": 3500,
                    "cap_source": "REL-BLD-016.KA.HEARTH.ASSET (PROVISIONAL, ArtSource/kharuun-asset-cards.json; owner-confirmed 2026-09-07)",
                    "cap_scope": "Owner ruling 2026-09-07: ceilings apply to the complete asset including articulated components",
                    "lod0_within_cap": m0.triangle_count() <= 8000, "lod1_within_cap": m1.triangle_count() <= 3500,
                    "amber_area_fraction_lod0": round(amber, 5), "amber_cap": 0.15,
                    "amber_measure": "surface area, as REL-ART-029 states",
                    "amber_within_cap": amber <= 0.15},
        "concept_measurements": measurements(m0),
        "component_inventory": contract_inventory(m0),
        "states": {name: purpose for name, purpose in (
            ("working", "the shell whole, five lit hollows, seven spires with lit veins, amber growth seams between the bands"),
            ("damaged", "one spire's vein dark and a fracture stepping down the front flank; everything else unchanged"),
            ("destroyed", "ceramic collapse inward: the upper bands fall and settle, hollows, cleft and spires gone, a rubble ring remains"))},
        "pending_requirements": [
            {"requirement": "Articulated components for a structure of this role",
             "status": "PENDING, not waived",
             "detail": ("Canon gives the Hearth a slow interior glow breath and a cleft settle on delivery, neither of which is "
                        "geometric articulation, so this blockout is a static mesh. Per the owner's Anchor ruling of 2026-09-07 a "
                        "static primary structure is acceptable for a blockout but does not close the pipeline's articulated-component "
                        "requirement. Do not mark this asset compliant until the requirement is implemented or explicitly amended.")}],
        "outputs": exported["outputs"], "review_assemblies": exported["review"],
        "tools": {"mesh_kit": "ArtSource/tools/ebs_meshkit.py", "renderer": "ArtSource/tools/ebs_render.py"},
        "acceptance": {"art": "NOT_EVALUATED", "gameplay": "NOT_EVALUATED",
                       "technical": ("PENDING — built against the provisional card REL-BLD-016.KA.HEARTH.ASSET; final technical "
                                     "acceptance waits on that card being incorporated into the authoritative requirements and its "
                                     "checks passing (owner ruling 2026-09-07)"),
                       "owner": "NOT_ACCEPTED"},
        "source_bindings": {"candidate": "BuildArtifacts/Evidence/concept-discovery-20260906/memory-hearth-review/memory-hearth-candidate.png",
                            "concepts": ["EBS-CON-KHA-BLD-005 (KEEP, design identity)",
                                         "EBS-CON-KHA-BLD-001 (REPLACE, retained history only)"],
                            "canon": "DevelopmentBible.md line 555 (SPEC-BLD-016.KA.HEARTH)",
                            "gameplay": "Content/Data/Source/buildings.json ka_memory_hearth"},
    }


def write_scenes(evidence_dir: str) -> list:
    scenes = os.path.join(evidence_dir, "scenes")
    os.makedirs(scenes, exist_ok=True)
    base = {"author": AUTHOR, "srgb": True,
            "materials": {STRATA: [0.20, 0.195, 0.19], FIBRE: [0.34, 0.30, 0.25], AMBER: [0.98, 0.66, 0.22],
                          "_default": [0.5, 0.5, 0.5]},
            "emissive": [AMBER], "width": 1920, "height": 1080, "background": [0.84, 0.82, 0.78],
            "light": {"direction": [0.55, -0.35, -0.76], "ambient": 0.32, "key": 0.85, "color": [1.0, 0.86, 0.7],
                      "fill_color": [0.5, 0.55, 0.7], "fill": 0.24},
            "ground": {"tile_cm": 200, "tiles": 40, "color": [0.028, 0.028, 0.032], "grid_color": [0.07, 0.07, 0.08],
                       "footprint_cm": [F, F], "footprint_color": [0.98, 0.66, 0.22]},
            "reference_figure": {"height_cm": 180, "position": [-620, 560, 0], "color": [0.92, 0.55, 0.2]}}
    ortho = [{"name": "front", "type": "ortho", "from": "-X", "edges": True, "margin": 1.12, "target": [0, 0, 300]},
             {"name": "right", "type": "ortho", "from": "+Y", "edges": True, "margin": 1.12, "target": [0, 0, 300]},
             {"name": "rear", "type": "ortho", "from": "+X", "edges": True, "margin": 1.12, "target": [0, 0, 300]},
             {"name": "top", "type": "ortho", "from": "+Z", "image_up": "+X", "edges": True, "margin": 1.25, "target": [0, 0, 0]}]
    views = [{"name": "three_quarter", "type": "persp", "pitch_deg": -14, "yaw_deg": 28, "arm_cm": 1450, "fov_deg": 52, "target": [0, 0, 250]},
             {"name": "cleft_flank", "type": "persp", "pitch_deg": -16, "yaw_deg": -78, "arm_cm": 1150, "fov_deg": 50, "target": [0, 260, 130]},
             {"name": "tactical_gameplay", "type": "persp", "pitch_deg": -60, "yaw_deg": -45, "arm_cm": 4200, "fov_deg": 55, "target": [0, 0, 0]}]
    written = []

    def dump(name, meshes, vs):
        scene = dict(base); scene["meshes"] = meshes; scene["views"] = vs
        p = os.path.join(scenes, f"{name}.json")
        with open(p, "w", encoding="utf-8") as handle:
            json.dump(scene, handle, indent=1)
        written.append(p)

    for state in STATES:
        dump(state, [{"obj": f"../review/{ASSET}_{state}_LOD0.obj"}], ortho + views)
    dump("lod1", [{"obj": f"../review/{ASSET}_working_LOD1.obj"}], ortho[:2] + [views[0], views[2]])
    return written


def build_all(evidence_dir: str, out_dir: str) -> dict:
    return manifest(export(evidence_dir, out_dir))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--evidence-dir", required=True)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    manifest_path = os.path.join(HERE, "build-manifest.json")
    if args.check:
        with tempfile.TemporaryDirectory() as tmp:
            ev = os.path.join(tmp, "evidence"); os.makedirs(ev, exist_ok=True)
            fresh = build_all(ev, os.path.join(tmp, "export"))
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
    data = build_all(args.evidence_dir, os.path.join(HERE, "export"))
    scenes = write_scenes(args.evidence_dir)
    data["review"] = {"scenes": [os.path.relpath(s, args.evidence_dir) for s in scenes]}
    with open(manifest_path, "w", encoding="utf-8") as handle:
        json.dump(data, handle, indent=1, sort_keys=True)
    mm = data["concept_measurements"]
    print(json.dumps({"revision": REVISION, "lod0": data["budgets"]["lod0_triangles"], "lod1": data["budgets"]["lod1_triangles"],
                      "height_cm": mm["height_cm"], "shell_over_base": mm["shell_height_over_base_width"],
                      "total_over_base": mm["total_height_over_base_width"],
                      "spire_spread_over_base": mm["spire_spread_over_base_width"],
                      "amber_area": data["budgets"]["amber_area_fraction_lod0"],
                      "max_extent_cm": mm["max_extent_cm"], "footprint_half_cm": HALF, "scenes": len(scenes)}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
