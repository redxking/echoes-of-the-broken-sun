#!/usr/bin/env python3
"""EBS-FWL-SYS-001 Future Well: courtyard bowl, ring wall and one faceted shard (concept-v2 blockout).

Author: Angelis Pseftis.

Concept target (authoritative, owner ruling 2026-09-06): concept-fidelity.md in this folder. The
selected candidate (future-well-candidate.png, four states plus the spent inset) defines the form:
a broad circular courtyard of dark vitrified basalt masonry ringed by a low wall of stacked blocks
with a ruined rhythm and four taller broken segments, flat radial paving with concentric courses
and fine fracture seams inside, a fractured apron crumbling into the ground outside, and ONE
four-sided faceted shard on a small circular dais at the centre (S = shard height = 220 cm); the dais
is a step and a rim around a round vitrified socket recessed below the rim, the dark cracked bowl
centre the shard stands in. Harvest: amber climbs the shard's edge seams, the shard tilts and sinks
into the socket, light cuts, the spent courtyard stays dark and cracked with the shard gone below the
paving and the round dark socket with its fracture veins remaining (candidate spent inset). Preserve: a flat
cyan ring on the paving inside the wall. Reshape: a magenta trace from the dais through the +X
wall gap to the apron edge (the crossing itself is map-owned).

Package contract: Docs/VisualAssetPipeline/motion/gap-decisions.json
  production_policy[package_id == "EBS-PKG-EBS-FAM-FWL-001"], gap GAP-01. Prepared amendment
  FWL-HARVEST (REL-ART-014, cyan geyser -> amber rise and fold) is the selected direction; it is
  not applied to the master (conflict recorded, not resolved here).
Canon: Docs/Archive/DevelopmentBible.md "Future Wells". Book: paragraphs 243-247.
Simulation envelope (authoritative): EchoesSimCore Simulation.cpp - the Well's blocking footprint
  is one tile (footprintHalfExtentRaw = kFixedScale/2), capture radius 4.2 tiles, scar radius
  6 tiles; presentation tile = 200 cm. Impassable and indestructible (SPEC-WEL-004).

Rules that bound the concept (every forced deviation is recorded in README section 8):
  * The dais (the only raised impassable mass of the main mesh) and the UBX collision box stay
    inside the one-tile footprint. The shard is a separate part instanced at Spire_Hinge.
  * The ring wall (outer radius 450 cm, well outside the tile) is its own component group and a
    separate collision-less part SM_EBS_FWL_SYS_001_Wall with four cardinal gaps >= 140 cm so
    units reach the capture zone from every side. The concept shows one break (+X); the other
    three gaps are a rule-forced deviation. The footprint conflict (OWNER-QUESTION) is recorded
    in the README, not resolved here.
  * Inner paving and apron relief <= 20 cm (REL-ART-016 / REL-ART-030 walkable dressing).
  * LOD0 <= 8,000 / LOD1 <= 3,500 assembled triangles; three slots; Nanite off.

Parts: SM_EBS_FWL_SYS_001 (dais with rim and recessed socket, inner paving, apron, state channels, sockets, UBX),
SM_EBS_FWL_SYS_001_Spire (the shard, hinge at its base centre), SM_EBS_FWL_SYS_001_Wall (ring
wall, no collision). Harvest commit = part pose of the shard (pitch about the hinge, then sink).
All state colour lives in the state slot's masks, driven only by authoritative state. Review assemblies
park the channels a state does not light in a review-only off slot (Dormant lights the fracture seams and
the shard edge seams only); the "all_channels" assembly lights every channel for the emissive-area check.

concept-v2 (after inspecting the concept-v1 sheets against the concept pixels): the square vitrified plate
became a round recessed socket inside a rim so the spent centre reads as a dark cracked bowl centre, the
dais fracture veins sit on the socket top instead of inside the plate, and the Dormant review no longer
lights the Preserve ring and the Reshape trace.

concept-v3 (pixel measurement of the candidate against the concept-v2 sheets): the courtyard is rescaled to the
painting (wall outer radius 450 cm = 2.05 S; the fidelity file's earlier 1.6-1.8 S / 360 cm estimate is
superseded there), the Preserve ring hugs the wall's inner face (0.90 R_out), the Reshape trace is a flush
light line (<= 1.5 cm proud of the band, the paving and the apron; rung marks are a mask feature, not
geometry) so nothing implies Reshape in the other states, one dominant far-side wall peak (0.89 S) stands
over three lower stubs, and the apron is a torn field of sectors with bites and a +X tongue of broken slabs
toward Reshape_Trace_End. --check is read-only (temporary directory, compares exports and review assemblies).
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
import platform
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(os.path.dirname(HERE), "tools"))
import ebs_meshkit as kit  # noqa: E402

AUTHOR = "Angelis Pseftis"
PACKAGE_ID = "EBS-PKG-EBS-FAM-FWL-001"
PRODUCTION_ID = "EBS-FWL-SYS-001"
ASSET = "SM_EBS_FWL_SYS_001"
SPIRE_ASSET = f"{ASSET}_Spire"
WALL_ASSET = f"{ASSET}_Wall"
REVISION = "ebs-fwl-sys-001-concept-v3"  # concept-v3: courtyard rescaled to the candidate pixels (wall 450 = 2.05 S), flush trace and ring, one dominant wall peak, torn apron with the +X tongue; concept-v2: recessed round vitrified socket in a dais rim, veins on the socket top, hinge on the rim; concept-v1: single faceted shard on a dais, courtyard ring wall part, radial paving, apron
PLANNED_FOLDER = "/Game/Echoes/Production/FWL/SYS/EBS_FWL_SYS_001/"

BASALT = "MI_EBS_FWL_Basalt"        # charcoal vitrified basalt masonry (dais, paving, wall, apron)
VITRIFIED = "MI_EBS_FWL_Vitrified"  # charcoal glass with magenta micro-fracture (the shard, the dais plate)
STATE = "MI_EBS_FWL_State"          # state-masked emissive: fracture seams, shard edge seams, preserve ring, reshape trace

TILE_CM = 200.0
FOOTPRINT = (TILE_CM, TILE_CM)                       # one tile: the impassable dais mass
CAPTURE_RADIUS_CM = 4.2 * TILE_CM

# --- concept measurements (candidate Dormant panel, S = shard height) --------------------------
SPIRE_HEIGHT = 220.0                                  # S, concept-fidelity.md item 2
SPIRE_BASE_HALF = 66.0                                # base 132 cm across the flats (~0.6 S; candidate ~0.63 S)
SPIRE_BASE_YAW = 12.0                                 # base square turned off the axes: slightly irregular shard
SPIRE_BASE_CORNER_SCALE = (1.0, 0.95, 1.0, 0.91)      # per-corner radius scale: irregular footprint
SPIRE_MID_Z_FRACTION, SPIRE_MID_BULGE, SPIRE_MID_TWIST_DEG = 0.38, 1.06, 9.0
SPIRE_APEX_OFFSET = (7.0, -5.0)
DAIS_R, DAIS_STEP_R, DAIS_H, DAIS_STEP_Z = 98.0, 94.0, 14.0, 10.0   # small circular dais inside the tile: a step, then a rim
SOCKET_R, SOCKET_DROP = 82.0, 1.5                     # round vitrified socket recessed below the rim: the dark cracked bowl centre the shard folds into and that remains once spent
DAIS_VEINS = 3                                        # fracture veins across the socket top (state slot): faint in Dormant, dark cracks once spent
HINGE_Z = DAIS_H + 0.2                                # the shard's base corners rest on the rim
PAVING_R, PAVING_Z = 412.0, 6.0                       # inner paving disc, tucks under the wall
PAVING_COURSES = (165.0, 255.0, 345.0)                # concentric courses (0.40 / 0.62 / 0.84 of the wall's inner face, as in concept-v2)
SPOKE_W, SPOKE_TOP_Z, SPOKE_R_END = 28.0, 11.0, 360.0   # four cardinal paving bands (states sheet), 5 cm proud, ending inside the Preserve ring
SEAM_COUNT, SEAM_W = 12, 3.0                          # fine radial fracture seams (state slot)
WALL_R_OUT, WALL_R_IN = 450.0, 410.0                  # candidate Dormant/Preserve panels: R_out = 2.0-2.2 S (2.05 S built); concept-fidelity.md Scale, amended concept-v3
WALL_H = 84.0                                         # course-2 tops ~0.27-0.41 S; crest mean ~0.30 S measured (concept 0.26-0.35 S)
WALL_COURSE_Z = 40.0
WALL_RHYTHM = (0.78, 0.92, 1.0, 1.08, 1.02, 0.88, 0.96, 0.70)   # course-2 top as a fraction of WALL_H (ruined rhythm)
WALL_TALL_H = (138.0, 124.0, 142.0, 195.0)            # three broken stubs (0.56-0.65 S) and ONE dominant far-side peak (0.89 S; candidate Dormant back peak ~0.85-0.95 S)
WALL_PEAK_MIN_H = 170.0                               # segments at or above this get the taller, narrower ridge cap (the peak)
WALL_BLOCKS_PER_ARC = 10                              # ~52 cm blocks at the wall's mid radius
WALL_GAP_WIDTH = {0: 176.0, 90: 150.0, 180: 150.0, 270: 150.0}   # chord at the outer radius, >= 140 cm each; +X is the concept's break
WALL_GAP_MIN_CM = 140.0
APRON_R_BASE, APRON_Z = 580.0, 4.0                    # torn apron field crumbling into the ground, ~1.3 R_out like the painting (relief <= 20 cm)
APRON_SECTORS = (36, 18)                              # LOD0 / LOD1 convex sectors (the kit fans caps, so the concave outline is built from sectors)
APRON_BITES = ((40.0, 55.0), (105.0, 45.0), (150.0, 60.0), (215.0, 40.0), (262.0, 50.0), (330.0, 45.0))   # (angle deg, depth cm): torn bites in the outline
APRON_TONGUE_END_X = 640.0                            # broken-slab tongue on +X toward the map-owned crossing (candidate: ~1.4-1.5 R_out)
PRESERVE_RING_R, PRESERVE_RING_W = 404.0, 10.0        # flat cyan ring hugging the wall's inner face (0.90 R_out; candidate 03: the ring touches the wall)
RESHAPE_TRACE_W, RESHAPE_TRACE_END_X = 8.0, 600.0     # flush light line from the dais through the +X gap onto the tongue (candidate 04: a thin line, not a rail)
FLUSH_PROUD = 1.5                                     # light-only channels ride <= 1.5 cm proud of their substrate (like the paving seams) so they vanish when unlit
FOLD_PITCH_DEG = 24.0                                 # harvest commit: the shard tilts about the hinge (apex toward -X) ...
FOLD_SINK_CM = 224.0                                  # ... and sinks through the plate until the apex is below the paving
POSE_AMOUNT = {"dormant": 0.0, "harvest_telegraph": 0.0, "harvest_fold_mid": 0.5, "harvest_commit": 0.85, "spent": 1.0,
               "preserve": 0.0, "reshape": 0.0, "all_channels": 0.0}
REVIEW_STATES = ("dormant", "harvest_telegraph", "harvest_fold_mid", "harvest_commit", "spent", "preserve", "reshape", "all_channels")
STATE_OFF_REVIEW = "MI_EBS_FWL_State_Off"         # REVIEW ASSEMBLIES ONLY: seam channels a state does not light read as dark fracture lines (the export keeps the three slots; masks select channels in-engine)
STATE_HIDDEN_REVIEW = "MI_EBS_FWL_State_Hidden"   # REVIEW ASSEMBLIES ONLY: light-only channels (ring, trace) a state does not light vanish into the paving (the scenes colour it as the basalt)
STATE_CHANNELS = {"preserve_ring": "preserve_ring", "reshape_trace": "reshape_trace"}
LIGHT_ONLY_CHANNELS = ("preserve_ring", "reshape_trace")   # pure light in the concept: no ring or trace shows in the candidate's Dormant, Preserve (trace) or spent panels
# Channels each review assembly lights; "all_channels" (None) keeps every channel lit for the emissive-area worst case only.
ACTIVE_CHANNELS = {"dormant": ("seams", "spire_seam"), "harvest_telegraph": ("seams", "spire_seam"), "harvest_fold_mid": ("seams", "spire_seam"), "harvest_commit": ("seams", "spire_seam"),
                   "spent": (), "preserve": ("preserve_ring",), "reshape": ("reshape_trace",), "all_channels": None}


def state_channel(component: str) -> str:
    name = component.split("spire_", 1)[-1] if component.startswith("spire_spire_") else component
    if name.startswith("spire_seam_"):
        return "spire_seam"
    if name.startswith(("paving_seam_", "dais_vein_")):
        return "seams"
    return STATE_CHANNELS.get(name, "seams")


def polygon_outline(radius: float, sides: int, phase_deg: float = 0.0):
    return kit.regular_polygon(radius, sides, phase_deg)


def polar(r: float, a_deg: float, z: float = 0.0):
    return (r * math.cos(math.radians(a_deg)), r * math.sin(math.radians(a_deg)), z)


def gap_half_deg(cardinal: int) -> float:
    return math.degrees(math.asin(WALL_GAP_WIDTH[cardinal] / 2.0 / WALL_R_OUT))


def wall_arcs():
    """Four wall arcs (start_deg, end_deg) between the cardinal gaps, counter-clockwise from +X."""
    arcs = []
    for k in range(4):
        a0 = k * 90 + gap_half_deg(k * 90)
        a1 = (k + 1) * 90 - gap_half_deg(((k + 1) * 90) % 360)
        arcs.append((a0, a1))
    return arcs


def apron_sector_radii(sectors: int) -> list:
    """Outer radius of each apron sector (sector k is centred on 360k/sectors deg): a slow undulation, a per-sector
    step so neighbouring slab ends never line up, and the torn bites of APRON_BITES wherever their angle falls."""
    radii = []
    half = 180.0 / sectors
    for k in range(sectors):
        a_deg = 360.0 * k / sectors
        a = math.radians(a_deg)
        r = APRON_R_BASE + 22.0 * math.sin(3 * a + 0.4) + 8.0 * math.cos(5 * a) + ((k * 13) % 5) * 5.0 - 10.0
        for bite_deg, depth in APRON_BITES:
            if abs(((bite_deg - a_deg + 180.0) % 360.0) - 180.0) <= half:
                r -= depth
        radii.append(r)
    return radii


# --- main mesh: dais, inner paving, apron, state channels -----------------------------------------
def build_main(lod: int) -> kit.Mesh:
    m = kit.Mesh(ASSET)
    basalt, vitrified, state = m.slot(BASALT), m.slot(VITRIFIED), m.slot(STATE)
    hi = lod == 0

    # Dais: two shallow steps (the small circular dais the shard stands on), inside the one-tile footprint.
    dais_sides = 16 if hi else 12
    m.prism(polygon_outline(DAIS_R, dais_sides, 11.25), 0.0, DAIS_STEP_Z, basalt, "dais_step_01", cap_bottom=False)
    # Rim: the second step is an annulus; the shard's base corners rest on it.
    m.ring(polygon_outline(DAIS_STEP_R, dais_sides, 11.25), polygon_outline(SOCKET_R, dais_sides, 11.25), DAIS_STEP_Z, DAIS_H, basalt, "dais_step_02")
    # Round vitrified socket recessed below the rim: the dark bowl centre the shard folds into (book 245) and the
    # cracked dark centre that remains once spent (candidate inset). No bottom cap: it sits on the first step.
    m.prism(polygon_outline(SOCKET_R, dais_sides, 11.25), DAIS_STEP_Z, DAIS_H - SOCKET_DROP, vitrified, "dais_socket", cap_bottom=False)
    # Fracture veins across the socket top (state slot), aligned with six of the twelve paving seams.
    for k in range(DAIS_VEINS):
        m.box((0.0, 0.0, DAIS_H - SOCKET_DROP + 0.4), (2 * SOCKET_R - 6.0, SEAM_W, 0.8), state, f"dais_vein_{k + 1:02d}", yaw_deg=15.0 + 60.0 * k)

    # Inner paving: flat disc with concentric courses, four cardinal bands and fine radial fracture seams.
    paving_sides = 32 if hi else 16
    m.prism(polygon_outline(PAVING_R, paving_sides), 0.0, PAVING_Z, basalt, "paving_disc", cap_bottom=False)
    course_sides = 24 if hi else 12
    for k, r in enumerate(PAVING_COURSES if hi else PAVING_COURSES[1:]):  # flat dark joint rings (the candidate's courses are lines, not curbs)
        m.ring(polygon_outline(r + 5.0, course_sides), polygon_outline(r - 5.0, course_sides), PAVING_Z - 1.0, PAVING_Z + 1.0, vitrified, f"paving_course_{k + 1:02d}")
    for k in range(4):
        yaw = 90.0 * k
        r0, r1 = DAIS_R + 2.0, SPOKE_R_END
        m.box((((r0 + r1) / 2.0) * math.cos(math.radians(yaw)), ((r0 + r1) / 2.0) * math.sin(math.radians(yaw)), (PAVING_Z - 1.0 + SPOKE_TOP_Z) / 2.0),
              (r1 - r0, SPOKE_W, SPOKE_TOP_Z - (PAVING_Z - 1.0)), basalt, f"paving_spoke_{k + 1:02d}", yaw_deg=yaw)
    seam_count = SEAM_COUNT if hi else SEAM_COUNT // 2
    for k in range(seam_count):
        yaw = 15.0 + 360.0 * k / seam_count
        r0, r1 = DAIS_R + 8.0, PRESERVE_RING_R - PRESERVE_RING_W / 2.0 - 4.0   # seams stop short of the ring (no crossing channels)
        m.box((((r0 + r1) / 2.0) * math.cos(math.radians(yaw)), ((r0 + r1) / 2.0) * math.sin(math.radians(yaw)), PAVING_Z + 0.25),
              (r1 - r0, SEAM_W, 2.5), state, f"paving_seam_{k + 1:02d}", yaw_deg=yaw)

    # Apron: a torn paving field outside the wall (candidate: a ragged rubble field ~1.3 R_out with bites in its outline
    # and a broken tongue on +X). Built from convex sectors tucked under the paving, each with its own outer radius, so
    # the edge steps and notches between neighbours read as broken slab ends; the fine rubble is scatter/texture work.
    sectors = APRON_SECTORS[0] if hi else APRON_SECTORS[1]
    radii = apron_sector_radii(sectors)
    r_in = PAVING_R - 20.0
    for k, r in enumerate(radii):
        a0 = 360.0 * (k - 0.5) / sectors
        a1 = a0 + 360.0 / sectors
        outline = [polar(r_in, a0)[:2], polar(r, a0)[:2], polar(r, a1)[:2], polar(r_in, a1)[:2]]
        m.prism(outline, 0.0, APRON_Z, basalt, f"apron_sector_{k + 1:02d}", cap_bottom=False, skip_edges=(3,))   # inner wall hidden under the paving
    # The +X tongue: a broken causeway stub carrying the trace out of the courtyard toward the map-owned crossing,
    # its slabs sinking into the ground at the tip.
    root = min(radii[0], radii[-1]) - 40.0
    m.prism([(root, -60.0), (APRON_TONGUE_END_X, -42.0), (APRON_TONGUE_END_X, 42.0), (root, 60.0)], 0.0, APRON_Z, basalt, "apron_tongue", cap_bottom=False, skip_edges=(3,))
    tongue_slabs = [((APRON_TONGUE_END_X + 14.0, 18.0, 1.75), (30.0, 26.0, 3.5), 14.0), ((APRON_TONGUE_END_X + 24.0, -22.0, 1.5), (26.0, 22.0, 3.0), -25.0),
                    ((APRON_TONGUE_END_X - 2.0, 48.0, 1.75), (22.0, 20.0, 3.5), 30.0), ((APRON_TONGUE_END_X - 26.0, -60.0, 2.0), (28.0, 18.0, 4.0), 8.0),
                    ((APRON_TONGUE_END_X - 55.0, 30.0, 6.5), (40.0, 34.0, 5.0), 5.0), ((APRON_TONGUE_END_X - 30.0, -28.0, 6.0), (36.0, 30.0, 4.0), -12.0)]
    for centre, size, yaw in (tongue_slabs if hi else tongue_slabs[:2]):
        m.box(centre, size, basalt, "apron_tongue", yaw_deg=yaw)
    crumble = 22 if hi else 10
    n = 0
    for k in range(crumble):
        a = 360.0 * k / crumble + 7.0 * ((k * 5) % 3)
        if min(abs(((a - c + 180.0) % 360.0) - 180.0) for c in (0.0, 90.0, 180.0, 270.0)) < 16.0:
            continue  # keep the four gaps and the tongue clear of rubble
        r = WALL_R_OUT + 38.0 + ((k * 37) % 70)
        size = (28.0 + (k * 11) % 30, 20.0 + (k * 7) % 22, 7.0 + (k * 3) % 7)
        n += 1
        m.box((r * math.cos(math.radians(a)), r * math.sin(math.radians(a)), size[2] / 2.0), size, basalt, f"apron_crumble_{n:02d}", yaw_deg=a + (k * 23) % 40)   # bedded on the ground, 3-9 cm proud of the apron
    # Fallen wall blocks flanking each gap: the wall breaks down into the openings.
    fallen = 0
    for c in (0, 90, 180, 270):
        for sign in ((-1, 1) if hi else (1,)):
            a = c + sign * (gap_half_deg(c) + 9.0)
            fallen += 1
            m.box(((WALL_R_OUT + 16.0) * math.cos(math.radians(a)), (WALL_R_OUT + 16.0) * math.sin(math.radians(a)), APRON_Z + 6.0), (44.0, 30.0, 14.0), basalt, f"apron_fallen_{fallen:02d}", yaw_deg=a + 18.0 * sign)

    # Preserve custody ring: a flat light ring on the paving hugging the wall's inner face (candidate 03), not a
    # floating hoop; 1 cm proud, so it vanishes into the paving when its mask is 0.
    ring_sides = 32 if hi else 16
    m.ring(polygon_outline(PRESERVE_RING_R + PRESERVE_RING_W / 2.0, ring_sides), polygon_outline(PRESERVE_RING_R - PRESERVE_RING_W / 2.0, ring_sides),
           PAVING_Z - 1.0, PAVING_Z + 1.0, state, "preserve_ring")
    # Reshape trace: a flush light line (<= FLUSH_PROUD above whatever it rides on: the +X band, then the paving, then the
    # apron and the tongue) from the dais through the +X gap to Reshape_Trace_End, where the map-owned crossing continues.
    # Candidate 04 shows a thin line on the paving surface, and candidates 01/03 and the spent inset show no line at all, so
    # the trace is no relief: with its mask at 0 it disappears. Rung marks that fill toward the end are a mask feature.
    for x0, x1, top in ((DAIS_R - 4.0, SPOKE_R_END, SPOKE_TOP_Z + 1.0), (SPOKE_R_END - 2.0, PAVING_R - 2.0, PAVING_Z + FLUSH_PROUD),
                        (PAVING_R - 4.0, RESHAPE_TRACE_END_X, APRON_Z + FLUSH_PROUD)):
        m.box(((x0 + x1) / 2.0, 0.0, top - 1.0), (x1 - x0, RESHAPE_TRACE_W, 2.0), state, "reshape_trace")

    # Sockets.
    hinge_z = HINGE_Z
    m.sockets.append(kit.Socket("Target_Anchor_Center", (0.0, 0.0, DAIS_H), 0.0, "capture / protocol targeting anchor at the dais centre"))
    m.sockets.append(kit.Socket("State_VFX_Origin", (0.0, 0.0, hinge_z), 0.0, "Harvest amber climb, Preserve pulse and Reshape trace effects originate at the shard base"))
    m.sockets.append(kit.Socket("Spire_Hinge", (0.0, 0.0, hinge_z), 0.0,
                                f"shard part origin (base centre); Harvest commit pitches the part +{FOLD_PITCH_DEG:g} deg about the socket Y axis and sinks it {FOLD_SINK_CM:g} cm (part pose)"))
    m.sockets.append(kit.Socket("Reshape_Trace_End", (RESHAPE_TRACE_END_X, 0.0, APRON_Z), 0.0, "where the map-authored temporary crossing's own effect continues"))
    for k in range(4):
        yaw = 90.0 * k
        r = (WALL_R_OUT + WALL_R_IN) / 2.0
        m.sockets.append(kit.Socket(f"Wall_Gap_{k + 1:02d}", (r * math.cos(math.radians(yaw)), r * math.sin(math.radians(yaw)), 0.0), yaw,
                                    f"centre of the {WALL_GAP_WIDTH[int(yaw)]:g} cm wall gap facing {yaw:g} deg; units enter the courtyard here"))
    m.collision.append(kit.CollisionBox("dais", (0.0, 0.0, 60.0), (2 * DAIS_R, 2 * DAIS_R, 120.0)))
    return m


# --- shard part -------------------------------------------------------------------------------
def spire_rings():
    base, mid = [], []
    mid_z = SPIRE_HEIGHT * SPIRE_MID_Z_FRACTION
    mid_scale = (1.0 - SPIRE_MID_Z_FRACTION) * SPIRE_MID_BULGE
    for k in range(4):
        a = SPIRE_BASE_YAW + 45.0 + 90.0 * k
        r = SPIRE_BASE_HALF * math.sqrt(2.0) * SPIRE_BASE_CORNER_SCALE[k]
        base.append(polar(r, a, 0.0))
        am = a + SPIRE_MID_TWIST_DEG
        mid.append((SPIRE_APEX_OFFSET[0] * SPIRE_MID_Z_FRACTION + r * mid_scale * math.cos(math.radians(am)),
                    SPIRE_APEX_OFFSET[1] * SPIRE_MID_Z_FRACTION + r * mid_scale * math.sin(math.radians(am)), mid_z))
    apex = (SPIRE_APEX_OFFSET[0], SPIRE_APEX_OFFSET[1], SPIRE_HEIGHT)
    return base, mid, apex


def edge_bead(m: kit.Mesh, p0, p1, slot: int, component: str, half_w: float = 2.6, proud: float = 3.5):
    """Slim triangular prism riding an outward edge from p0 to p1 (the shard's seam light)."""
    d = kit.v_sub(p1, p0)
    length = kit.v_len(d)
    d = kit.v_norm(d)
    o = kit.v_norm((p0[0] + p1[0], p0[1] + p1[1], 0.0))
    p = kit.v_norm(kit.v_cross(d, o))
    s = kit.v_add(p0, kit.v_mul(d, length * 0.05))
    e = kit.v_add(p0, kit.v_mul(d, length * 0.93))
    s0, s1, s2 = kit.v_add(s, kit.v_mul(o, proud)), kit.v_add(kit.v_add(s, kit.v_mul(o, 0.8)), kit.v_mul(p, half_w)), kit.v_add(kit.v_add(s, kit.v_mul(o, 0.8)), kit.v_mul(p, -half_w))
    e0, e1, e2 = kit.v_add(e, kit.v_mul(o, proud)), kit.v_add(kit.v_add(e, kit.v_mul(o, 0.8)), kit.v_mul(p, half_w)), kit.v_add(kit.v_add(e, kit.v_mul(o, 0.8)), kit.v_mul(p, -half_w))
    m.add_convex_solid([[s0, s1, s2], [e2, e1, e0], [s0, s1, e1, e0], [s1, s2, e2, e1], [s2, s0, e0, e2]], slot, component)


def build_spire(lod: int) -> kit.Mesh:
    """One faceted four-sided shard, origin at its base centre (the Spire_Hinge socket)."""
    m = kit.Mesh(SPIRE_ASSET)
    vitrified, state = m.slot(VITRIFIED), m.slot(STATE)
    hi = lod == 0
    base, mid, apex = spire_rings()
    faces = [[base[3], base[2], base[1], base[0]]]
    for i in range(4):
        j = (i + 1) % 4
        faces.append([base[i], base[j], mid[j]])
        faces.append([base[i], mid[j], mid[i]])
        faces.append([mid[i], mid[j], apex])
    m.add_convex_solid(faces, vitrified, "spire_body")
    # Edge seams: the Harvest telegraph light climbs these (state mask ramps with progress).
    for i in range(4):
        edge_bead(m, mid[i], apex, state, f"spire_seam_{i + 1:02d}")
        if hi:
            edge_bead(m, base[i], mid[i], state, f"spire_seam_{i + 1:02d}", half_w=2.2, proud=3.0)
    return m


# --- wall part --------------------------------------------------------------------------------
def arc_block(m: kit.Mesh, r0: float, r1: float, a0: float, a1: float, z0: float, z1: float, slot: int, component: str, joint_deg: float = 0.12):
    outline = [polar(r1, a0 + joint_deg)[:2], polar(r1, a1 - joint_deg)[:2], polar(r0, a1 - joint_deg)[:2], polar(r0, a0 + joint_deg)[:2]]
    m.prism(outline, z0, z1, slot, component, cap_bottom=False)


def build_wall(lod: int) -> kit.Mesh:
    """Courtyard ring wall: stacked basalt blocks with a ruined rhythm, four taller broken segments on the
    diagonals, four cardinal gaps. No collision (the footprint conflict is recorded, not resolved)."""
    m = kit.Mesh(WALL_ASSET)
    basalt = m.slot(BASALT)
    hi = lod == 0
    block = 0
    coping = 0
    tall_blocks = (WALL_BLOCKS_PER_ARC // 2 - 1, WALL_BLOCKS_PER_ARC // 2)   # the two blocks centred on the arc's diagonal
    for k, (a0, a1) in enumerate(wall_arcs()):
        da = (a1 - a0) / WALL_BLOCKS_PER_ARC
        tall_lo, tall_hi = a0 + tall_blocks[0] * da, a0 + (tall_blocks[1] + 1) * da
        low_blocks = [i for i in range(WALL_BLOCKS_PER_ARC) if i not in tall_blocks]
        if hi:
            for i in low_blocks:  # course 1: full lower course
                block += 1
                arc_block(m, WALL_R_IN, WALL_R_OUT, a0 + i * da, a0 + (i + 1) * da, 0.0, WALL_COURSE_Z, basalt, f"wall_block_{block:02d}")
            for j in range(WALL_BLOCKS_PER_ARC + 1):  # course 2: staggered half a block, some blocks fallen
                b0, b1 = max(a0, a0 + (j - 0.5) * da), min(a1, a0 + (j + 0.5) * da)
                if b1 <= tall_lo + 1e-6 or b0 >= tall_hi - 1e-6:
                    if (k * 2 + j) % 7 == 1:
                        continue  # a fallen course-2 block exposes the lower course
                    top = WALL_H * WALL_RHYTHM[(j + 2 * k) % len(WALL_RHYTHM)]
                    block += 1
                    arc_block(m, WALL_R_IN, WALL_R_OUT, b0, b1, WALL_COURSE_Z, top, basalt, f"wall_block_{block:02d}")
                    if j % 2 == 0 and b1 - b0 > da * 0.8:
                        coping += 1
                        mid = (b0 + b1) / 2.0
                        c = polar((WALL_R_IN + WALL_R_OUT) / 2.0 + 3.0, mid, top + 4.0)
                        m.box(c, (24.0, 16.0, 8.0), basalt, f"wall_coping_{coping:02d}", yaw_deg=mid + 7.0)
        else:
            for i in low_blocks:  # LOD1: one course carrying the rhythm
                top = WALL_H * WALL_RHYTHM[(i + 2 * k) % len(WALL_RHYTHM)]
                block += 1
                arc_block(m, WALL_R_IN, WALL_R_OUT, a0 + i * da, a0 + (i + 1) * da, 0.0, top, basalt, f"wall_block_{block:02d}")
        # Taller broken segment: three tiers and a jagged ridge cap; the one peak (>= WALL_PEAK_MIN_H, the far diagonal)
        # gets a taller, narrower cap so it reads as the candidate's single dominant spike over three lower stubs.
        tall = WALL_TALL_H[k]
        name = f"wall_tall_{k + 1:02d}"
        ridge_h, hi_half = (58.0, 0.18 * da) if tall >= WALL_PEAK_MIN_H else (32.0, 0.28 * da)
        arc_block(m, WALL_R_IN - 2.0, WALL_R_OUT + 2.0, tall_lo, tall_hi, 0.0, tall * 0.55, basalt, name, joint_deg=0.15)
        arc_block(m, WALL_R_IN + 2.0, WALL_R_OUT - 2.0, tall_lo + 0.2 * da, tall_hi - 0.15 * da, tall * 0.55, tall - ridge_h, basalt, name, joint_deg=0.0)
        mid = (tall_lo + tall_hi) / 2.0 + 0.05 * da
        lo_half = 0.55 * da
        z0, z1 = tall - ridge_h, tall
        r_in, r_out, r_mid = WALL_R_IN + 4.0, WALL_R_OUT - 4.0, (WALL_R_IN + WALL_R_OUT) / 2.0 + 3.0
        bottom = [polar(r_in, mid - lo_half, z0), polar(r_in, mid + lo_half, z0), polar(r_out, mid + lo_half, z0), polar(r_out, mid - lo_half, z0)]
        ridge = [polar(r_mid, mid - hi_half, z1), polar(r_mid, mid + hi_half, z1)]
        # symmetric about the mid angle so both slopes are planar trapezoids
        m.add_convex_solid([bottom, [bottom[0], bottom[1], ridge[1], ridge[0]], [bottom[2], bottom[3], ridge[0], ridge[1]],
                            [bottom[0], ridge[0], bottom[3]], [bottom[1], bottom[2], ridge[1]]], basalt, name)
    return m


# --- assemblies ---------------------------------------------------------------------------------
def assemble(lod: int, state: str) -> kit.Mesh:
    main, spire, wall = build_main(lod), build_spire(lod), build_wall(lod)
    scene = kit.Mesh(f"{ASSET}_assembly_{state}_LOD{lod}")
    scene.merge(main)
    amount = POSE_AMOUNT[state]
    hinge = next(s for s in main.sockets if s.name == "Spire_Hinge")
    sunk = (hinge.position[0], hinge.position[1], hinge.position[2] - FOLD_SINK_CM * amount)
    scene.merge(spire, translate=sunk, yaw_deg=hinge.yaw_deg, pitch_deg=FOLD_PITCH_DEG * amount, component_prefix="spire_", include_sockets=False)
    scene.merge(wall, include_sockets=False)
    active = ACTIVE_CHANNELS[state]
    if active is not None:  # review only: park the channels this state does not light
        parked = {}
        for poly in scene.polygons:
            if scene.slots[poly.slot] != STATE:
                continue
            channel = state_channel(poly.component)
            if channel in active:
                continue
            # seams unlit = dark fracture lines in the glass; ring/trace unlit = nothing (they are pure light in the concept)
            name = STATE_HIDDEN_REVIEW if channel in LIGHT_ONLY_CHANNELS else STATE_OFF_REVIEW
            if name not in parked:
                parked[name] = scene.slot(name)
            poly.slot = parked[name]
    return scene


STATE_PRESENTATIONS = [
    {"track": "dormant", "spire": "standing", "state_mask": {"seams": 0.25, "spire_seam": 0.15, "preserve_ring": 0.0, "reshape_trace": 0.0}, "colour": "quiet magenta/amber fracture, low glow (held potential)", "extra": "doubled-outline/shadow cue is a lighting/decal task (Crownfall dual key/fill); reduced motion: static", "authority": "wellChoice == Dormant"},
    {"track": "harvest_telegraph", "spire": "standing", "state_mask": {"spire_seam": "ramps 0.15 -> 1.0 up the shard edges over the authoritative 180-tick telegraph progress", "seams": 0.4}, "colour": "broken-sun amber", "extra": "amber climb VFX at State_VFX_Origin; public siren/ping are simulation-owned", "authority": "harvest telegraph progress"},
    {"track": "harvest_cancel", "spire": "standing", "state_mask": {"spire_seam": "returns to dormant without a spent pose or success cue"}, "colour": "dormant", "extra": "no payout cue", "authority": "control broken before tick 180"},
    {"track": "harvest_commit", "spire": f"tilts 0 -> {FOLD_PITCH_DEG:g} deg about Spire_Hinge and sinks {FOLD_SINK_CM:g} cm into the dais over ~20 ticks", "state_mask": {"spire_seam": "1.0 then hard cut to 0"}, "colour": "amber cut to dark", "extra": "light and harmonic cut hard on commit (book 246)", "authority": "harvest committed"},
    {"track": "spent", "spire": "gone below the paving (part fully sunk; the round recessed vitrified socket with its dark veins remains as the cracked bowl centre)", "state_mask": {"all": 0.0}, "colour": "dark cracked charcoal", "extra": "permanent; scar radius 6 tiles is terrain-owned; Well non-interactive; nothing rises above the wall", "authority": "permanent_state == collapsed"},
    {"track": "preserve_hold", "spire": "standing", "state_mask": {"preserve_ring": "1.0 with a slow custody pulse", "spire_seam": 0.3}, "colour": "cyan custody", "extra": "flat ring on the paving inside the wall; 1,400 cm intelligence radius is simulation-owned; ring carries no gameplay radius", "authority": "wellChoice == Preserve and controlled"},
    {"track": "preserve_loss", "spire": "standing", "state_mask": {"preserve_ring": "0.0"}, "colour": "ring dark", "extra": "ownership transfer routes benefits authoritatively", "authority": "control lost"},
    {"track": "reshape_telegraph", "spire": "standing", "state_mask": {"reshape_trace": "ramps 0 -> 1.0 over the 180-tick telegraph"}, "colour": "magenta fracture", "extra": "the trace mask fills from the dais through Wall_Gap_01 toward Reshape_Trace_End (rung marks are a mask/texture feature, not geometry; the trace is flush and invisible at mask 0)", "authority": "reshape telegraph progress"},
    {"track": "reshape_cancel", "spire": "standing", "state_mask": {"reshape_trace": "returns to 0"}, "colour": "dormant", "extra": "", "authority": "telegraph interrupted"},
    {"track": "reshape_manifest", "spire": "standing", "state_mask": {"reshape_trace": 1.0}, "colour": "magenta", "extra": "the temporary crossing itself is a separate map-authored asset beyond Reshape_Trace_End", "authority": "reshape manifest active (1,800 ticks)"},
    {"track": "reshape_warning", "spire": "standing", "state_mask": {"reshape_trace": "slow warning cadence (no rapid flash)"}, "colour": "magenta", "extra": "expiry warning is simulation-owned; reduced flashing holds steady", "authority": "expiry warning window"},
    {"track": "reshape_expiry", "spire": "standing", "state_mask": {"reshape_trace": 0.0}, "colour": "dormant", "extra": "authored fallback displacement is simulation-owned", "authority": "manifest expired"},
    {"track": "restore", "spire": "per current state", "state_mask": {"all": "per current state"}, "colour": "per state", "extra": "reconstruct from saved state; never replay payout, collapse or terrain creation", "authority": "load / replay"},
]


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


# --- measurements ---------------------------------------------------------------------------------
def measure_gaps(wall: kit.Mesh) -> dict:
    """Chord width of each cardinal gap: distance between the nearest wall points on either side (XY)."""
    pts = [p for poly in wall.polygons for p in poly.points]
    result = {}
    for c in (0, 90, 180, 270):
        best_pos, best_neg = None, None
        for p in pts:
            d = (math.degrees(math.atan2(p[1], p[0])) - c + 180.0) % 360.0 - 180.0
            if d > 0 and (best_pos is None or d < best_pos[0]):
                best_pos = (d, p)
            if d < 0 and (best_neg is None or -d < best_neg[0]):
                best_neg = (-d, p)
        a, b = best_pos[1], best_neg[1]
        result[f"Wall_Gap_{c // 90 + 1:02d}"] = {"facing_deg": c, "chord_cm": round(math.hypot(a[0] - b[0], a[1] - b[1]), 2), "angular_deg": round(best_pos[0] + best_neg[0], 2)}
    return result


def measure_wall(wall: kit.Mesh) -> dict:
    comps = wall.components()
    low = [c for c in comps if c.startswith("wall_block_")]
    tall = [c for c in comps if c.startswith("wall_tall_")]
    tall_tops = [wall.component_bounds(c)[1][2] for c in tall]
    radial = max(math.hypot(p[0], p[1]) for poly in wall.polygons for p in poly.points)
    crest = wall_crest_profile(wall)
    return {"blocks": len(low), "tall_segments": len(tall), "outer_radius_cm": round(radial, 2), "low_crest_max_cm": round(max(crest), 2), "low_crest_min_cm": round(min(crest), 2),
            "low_crest_mean_cm": round(sum(crest) / len(crest), 2), "tall_heights_cm": tall_tops, "gaps": measure_gaps(wall),
            "ratios_to_S": {"outer_radius": round(radial / SPIRE_HEIGHT, 3), "low_wall_crest_mean": round(sum(crest) / len(crest) / SPIRE_HEIGHT, 3), "tall_max": round(max(tall_tops) / SPIRE_HEIGHT, 3)}}


def wall_crest_profile(wall: kit.Mesh, step_deg: float = 1.0) -> list:
    """Top of the low wall sampled around the ring (max block top covering each angle; tall segments and gaps excluded)."""
    spans = []
    for c in wall.components():
        if not c.startswith("wall_block_"):
            continue
        pts = [p for poly in wall.polygons if poly.component == c for p in poly.points]
        angles = [math.degrees(math.atan2(p[1], p[0])) % 360.0 for p in pts]
        lo, hi = min(angles), max(angles)
        if hi - lo > 180.0:  # span straddles 0 deg
            lo, hi = min(a for a in angles if a > 180.0), max(a for a in angles if a < 180.0) + 360.0
        spans.append((lo, hi, max(p[2] for p in pts)))
    crest = []
    a = 0.0
    while a < 360.0:
        tops = [top for lo, hi, top in spans if lo <= a <= hi or lo <= a + 360.0 <= hi]
        if tops:
            crest.append(max(tops))
        a += step_deg
    return crest


def measure_apron(main: kit.Mesh) -> dict:
    """Outer radius of every apron sector (the torn outline), the tongue reach and the relief of the whole apron."""
    radii = []
    for c in main.components():
        if c.startswith("apron_sector_"):
            radii.append(max(math.hypot(p[0], p[1]) for poly in main.polygons if poly.component == c for p in poly.points))
    (_, _, _), (tx1, _, _) = main.component_bounds("apron_tongue")
    relief = max(main.component_bounds(c)[1][2] for c in main.components() if c.startswith("apron_"))
    return {"sectors": len(radii), "outer_radius_min_cm": round(min(radii), 2), "outer_radius_max_cm": round(max(radii), 2), "outer_radius_mean_cm": round(sum(radii) / len(radii), 2),
            "bites": len(APRON_BITES), "tongue_reach_x_cm": round(tx1, 2), "relief_cm": relief,
            "ratios_to_wall_R_out": {"outer_radius_mean": round(sum(radii) / len(radii) / WALL_R_OUT, 3), "tongue_reach": round(tx1 / WALL_R_OUT, 3)}}


def measure_spire(spire: kit.Mesh) -> dict:
    (x0, y0, z0), (x1, y1, z1) = spire.component_bounds("spire_body")
    return {"height_cm": z1 - z0, "base_extent_cm": [round(x1 - x0, 2), round(y1 - y0, 2)], "base_to_height": round(max(x1 - x0, y1 - y0) / (z1 - z0), 3),
            "facets": spire.triangle_count_by("component")["spire_body"], "seams": sum(1 for c in spire.components() if c.startswith("spire_seam_"))}


def contract_inventory(main: kit.Mesh, spire: kit.Mesh, wall: kit.Mesh) -> dict:
    comps = main.components()
    return {
        "courtyard_bowl": {"contract": 1, "built": 1 if "paving_disc" in comps else 0, "dais_steps": sum(1 for c in comps if c.startswith("dais_step_")),
                           "dais_socket": 1 if "dais_socket" in comps else 0, "dais_veins": sum(1 for c in comps if c.startswith("dais_vein_")),
                           "paving_courses": sum(1 for c in comps if c.startswith("paving_course_")), "paving_spokes": sum(1 for c in comps if c.startswith("paving_spoke_")),
                           "paving_seams": sum(1 for c in comps if c.startswith("paving_seam_")), "apron_sectors": sum(1 for c in comps if c.startswith("apron_sector_")),
                           "apron_tongue": 1 if "apron_tongue" in comps else 0, "apron_crumble": sum(1 for c in comps if c.startswith("apron_crumble_"))},
        "ring_wall": {"contract": 1, "built": 1 if any(c.startswith("wall_block_") for c in wall.components()) else 0, "part": WALL_ASSET, "collision": len(wall.collision),
                      "component_group": "wall_*", "blocks": sum(1 for c in wall.components() if c.startswith("wall_block_")), "tall_segments": sum(1 for c in wall.components() if c.startswith("wall_tall_")),
                      "gaps": 4},
        "core_spire": {"contract": 1, "built": 1 if "spire_body" in spire.components() else 0, "evidence": f"one faceted shard part {SPIRE_ASSET} instanced at Spire_Hinge (single hinge socket)", "hinge_sockets": sum(1 for s in main.sockets if s.name == "Spire_Hinge")},
        "state_family": {"contract": 4, "built": 4, "states": ["Dormant", "Harvest", "Preserve", "Reshape"], "presentations": len(STATE_PRESENTATIONS)},
        "sockets": {"contract": ["Target_Anchor_Center", "State_VFX_Origin", "Spire_Hinge", "Reshape_Trace_End", "Wall_Gap_01", "Wall_Gap_02", "Wall_Gap_03", "Wall_Gap_04"], "built": [s.name for s in main.sockets]},
    }


def build_outputs(export_dir: str, review_dir: str) -> tuple:
    """Write every part (both LODs) into export_dir and every review assembly into review_dir; return
    (outputs, review, meshes). Output paths are recorded relative to the package so --check can compare them
    against the manifest wherever they were written."""
    os.makedirs(export_dir, exist_ok=True)
    os.makedirs(review_dir, exist_ok=True)
    outputs, review, meshes = [], [], {}
    for lod in (0, 1):
        for builder in (build_main, build_spire, build_wall):
            mesh = builder(lod)
            meshes[(mesh.name, lod)] = mesh
            stem = f"{mesh.name}_LOD{lod}"
            base = os.path.join(export_dir, stem)
            glb = mesh.write_glb(base + ".glb", extras={"production_id": PRODUCTION_ID, "package_id": PACKAGE_ID, "revision": REVISION, "lod": lod}, include_collision=(lod == 0))
            obj = mesh.write_obj(base + ".obj", header_lines=[f"Production ID {PRODUCTION_ID}", f"Revision {REVISION}", f"LOD{lod}"])
            outputs.append({"path": f"export/{stem}.glb", "sha256": glb, "lod": lod, "mesh": mesh.name, "triangles": mesh.triangle_count(),
                            "by_slot": mesh.triangle_count_by("slot"), "bounds_cm": mesh.bounds(), "section_slot_names": mesh.slot_names_in_primitive_order(),
                            "sockets": [{"name": s.name, "position_cm": s.position, "yaw_deg": s.yaw_deg, "purpose": s.purpose} for s in mesh.sockets],
                            "collision_boxes": [{"name": c.name, "center_cm": c.center, "size_cm": c.size} for c in mesh.collision] if lod == 0 else []})
            outputs.append({"path": f"export/{stem}.obj", "sha256": obj, "lod": lod, "mesh": mesh.name})
    for lod, state in [(0, s) for s in REVIEW_STATES] + [(1, "dormant")]:
        scene = assemble(lod, state)
        path = os.path.join(review_dir, f"{scene.name}.obj")
        review.append({"path": path, "sha256": scene.write_obj(path, header_lines=[f"Assembled review geometry {state} LOD{lod} (main + shard pose {POSE_AMOUNT[state]:g} + wall)"]),
                       "lod": lod, "state": state, "pose_amount": POSE_AMOUNT[state], "triangles": scene.triangle_count(), "bounds_cm": scene.bounds(),
                       "review_slots": scene.slots})
    return outputs, review, meshes


def build_manifest(outputs: list, review: list, meshes: dict) -> dict:
    main0, main1 = meshes[(ASSET, 0)], meshes[(ASSET, 1)]
    spire0, spire1 = meshes[(SPIRE_ASSET, 0)], meshes[(SPIRE_ASSET, 1)]
    wall0, wall1 = meshes[(WALL_ASSET, 0)], meshes[(WALL_ASSET, 1)]
    walkable_max_z = max(main0.component_bounds(c)[1][2] for c in main0.components() if not c.startswith("dais_"))
    dormant0, spent0 = assemble(0, "dormant"), assemble(0, "spent")
    spent_spire_top = max(spent0.component_bounds(c)[1][2] for c in spent0.components() if c.startswith("spire_"))
    with open(os.path.abspath(__file__), "rb") as handle:
        builder_sha = sha256_bytes(handle.read())
    wall_measure, spire_measure, apron_measure = measure_wall(wall0), measure_spire(spire0), measure_apron(main0)
    lod0_total = main0.triangle_count() + spire0.triangle_count() + wall0.triangle_count()
    lod1_total = main1.triangle_count() + spire1.triangle_count() + wall1.triangle_count()
    trace = main0.component_bounds("reshape_trace")
    manifest = {
        "author": AUTHOR, "creator": AUTHOR, "production_asset_id": PRODUCTION_ID, "package_id": PACKAGE_ID, "asset_name": ASSET,
        "parts": [ASSET, SPIRE_ASSET, WALL_ASSET],
        "revision": REVISION, "kit_revision": kit.KIT_REVISION, "stage": "BLOCKOUT (concept-v3)",
        "stage_boundary": "Editable source geometry (main, shard part, collision-less wall part), state-presentation plan and review views built to the concept-fidelity target; no textures, no import, no gate acceptance, no owner acceptance.",
        "comparison_baseline": {"revision": "ebs-fwl-sys-001-blockout-v1", "commit": "5a59c855fb82b97b0c684af513420478231afed8", "main_lod0_glb_sha256_prefix": "538ca37c",
                                "note": "the committed revision this source is compared against; concept-v1 and concept-v2 were never committed or receipted, so their hashes are not a reproducible baseline"},
        "planned_unreal_folder": PLANNED_FOLDER,
        "units": {"authored": "centimeters", "axes": "+X forward (Reshape trace direction), +Y right, +Z up", "pivot": "ground-contact centre of the one-tile footprint (dais centre)",
                  "nanite": False, "import_policy": "Nanite off on every part; LOD0/LOD1 authored (no auto LOD); UBX collision from the main part only"},
        "scale_basis": {
            "concept_S_cm": SPIRE_HEIGHT,
            "source_concept": "concept-fidelity.md (amended concept-v3) and the candidate pixels: Dormant/Preserve panels wall outer radius 2.0-2.4 S (2.05 S built), low wall ~0.3 S, one dominant back peak ~0.85-0.95 S over stubs ~0.55-0.65 S, shard base ~0.63 S, Preserve ring at 0.90 R_out, apron ~1.3 R_out with a +X tongue ~1.4-1.5 R_out",
            "superseded_estimate": "concept-fidelity.md's earlier 1.6-1.8 S / 360 cm wall radius (concept-v1/v2 built 360 = 1.64 S); superseded by the pixel measurement, recorded in the fidelity file and README section 8",
            "footprint_tiles": [1, 1], "footprint_cm": FOOTPRINT, "capture_radius_cm_presentation": CAPTURE_RADIUS_CM, "scar_radius_cm_presentation": 6 * TILE_CM,
            "simulation_source": "Source/EchoesSimCore/Private/Simulation.cpp: FutureWell footprintHalfExtentRaw = kFixedScale/2; kFutureWellCaptureRadiusRaw = 21*kFixedScale/5; kFutureWellScarRadiusRaw = 6*kFixedScale; presentation TileWorldSize 200",
            "impassable_mass_max_radius_cm": DAIS_R, "wall_outer_radius_cm": WALL_R_OUT, "apron_radius_cm_max": apron_measure["outer_radius_max_cm"], "assembled_reach_x_cm": dormant0.bounds()[1][0],
            "walkable_relief_cm": walkable_max_z, "walkable_rule": "REL-ART-016 / REL-ART-030: passable dressing <= 20 cm and not reading impassable",
            "footprint_conflict": f"The {WALL_R_OUT:g} cm wall ring lies outside the one-tile blocking footprint and reads impassable; recorded as an OWNER-QUESTION in README section 8 (not resolved here). The wall part carries no collision. Everything (apron, tongue, trace end) stays inside the {CAPTURE_RADIUS_CM:g} cm capture radius.",
            "status": "CONCEPT-V3 BLOCKOUT; dimensions follow concept-fidelity.md as amended by the pixel measurement",
        },
        "dimensions_cm": {"spire_height_S": SPIRE_HEIGHT, "spire_base_across_flats": 2 * SPIRE_BASE_HALF, "spire_apex_z": HINGE_Z + SPIRE_HEIGHT, "hinge_z": HINGE_Z, "dais_radius": DAIS_R, "dais_rim_inner_radius": SOCKET_R, "dais_rim_top_z": DAIS_H, "socket_radius": SOCKET_R, "socket_top_z": DAIS_H - SOCKET_DROP, "dais_veins": DAIS_VEINS,
                          "paving_radius": PAVING_R, "paving_z": PAVING_Z, "paving_courses": list(PAVING_COURSES), "spoke_r_end": SPOKE_R_END, "wall_outer_radius": WALL_R_OUT, "wall_inner_radius": WALL_R_IN, "wall_low_height": WALL_H, "wall_tall_heights": list(WALL_TALL_H),
                          "wall_gap_widths": {f"Wall_Gap_{c // 90 + 1:02d}": w for c, w in WALL_GAP_WIDTH.items()}, "apron_z": APRON_Z, "apron_base_radius": APRON_R_BASE, "apron_tongue_end_x": APRON_TONGUE_END_X,
                          "preserve_ring_radius": PRESERVE_RING_R, "preserve_ring_top_z": PAVING_Z + 1.0, "reshape_trace_width": RESHAPE_TRACE_W, "reshape_trace_end_x": RESHAPE_TRACE_END_X, "reshape_trace_top_z_max": trace[1][2], "flush_proud_max": FLUSH_PROUD,
                          "fold_pitch_deg": FOLD_PITCH_DEG, "fold_sink_cm": FOLD_SINK_CM, "spent_spire_top_z": spent_spire_top,
                          "assembled_bounds_dormant": dormant0.bounds()},
        "concept_measurements": {"wall": wall_measure, "spire": spire_measure, "apron": apron_measure,
                                 "ratios_to_S": {"wall_outer_radius": round(WALL_R_OUT / SPIRE_HEIGHT, 3), "preserve_ring_radius": round(PRESERVE_RING_R / SPIRE_HEIGHT, 3), "apron_mean_radius": round(apron_measure["outer_radius_mean_cm"] / SPIRE_HEIGHT, 3)},
                                 "preserve_ring_to_wall_R_out": round(PRESERVE_RING_R / WALL_R_OUT, 3)},
        "material_slots": [BASALT, VITRIFIED, STATE],
        "review_only_slots": {STATE_OFF_REVIEW: "seam channels a state does not light (dark fracture lines)", STATE_HIDDEN_REVIEW: "light-only channels (preserve_ring, reshape_trace) a state does not light (coloured as the basalt: nothing shows)"},
        "component_inventory": contract_inventory(main0, spire0, wall0),
        "state_presentations": STATE_PRESENTATIONS,
        "pose_amounts": POSE_AMOUNT,
        "budgets": {"lod0_assembled_triangles": lod0_total, "lod1_assembled_triangles": lod1_total, "lod0_cap": 8000, "lod1_cap": 3500,
                    "per_part": {name: {"lod0": meshes[(name, 0)].triangle_count(), "lod1": meshes[(name, 1)].triangle_count()} for name in (ASSET, SPIRE_ASSET, WALL_ASSET)}},
        "outputs": outputs, "review_assemblies": review,
        "tools": {"builder_sha256": builder_sha, "python": platform.python_version(), "platform": platform.platform()},
        "source_bindings": {
            "concept_fidelity": {"path": "ArtSource/EBS-FWL-SYS-001/concept-fidelity.md", "amended": "concept-v3: Scale section (wall radius 2.0-2.2 S / 450 cm) and item 1 (one dominant peak)"},
            "candidate_image": {"path": "/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/BuildArtifacts/Evidence/concept-discovery-20260906/future-well-review/future-well-candidate.png", "sha256": "b16e84cb4465c3c3cee46a71fb4b661256c0bdcbb9a030994fd7d7262ed60ba7"},
            "concept_sheet": {"path": "Project/site/assets/concepts/future-well-states.png", "regions": "TL Dormant, TR Harvest (material and courtyard read)"},
            "composition_reference": {"path": "Project/site/assets/concepts/target-render-vertical-slice.jpg", "region": [0.37, 0.29, 0.63, 0.68], "role": "composition only"},
            "state_board": {"path": "Docs/VisualAssetPipeline/motion/storyboards/EBS-PKG-EBS-FAM-FWL-001-GAP-01.svg"},
            "motion_package": {"path": "Docs/VisualAssetPipeline/motion/motion-packages.json", "gap_id": "EBS-PKG-EBS-FAM-FWL-001-GAP-01"},
            "amendment": "gap-decisions.json#/master_amendments FWL-HARVEST (PREPARED_NOT_APPLIED)",
            "book": {"paragraphs": [243, 244, 245, 246, 247], "sha256": "994e7df5d37a6ef1532c65a7b742d71e81b4ea7752311f5e5acc2e04c817ef83"},
            "gameplay_record": {"path": "Content/Data/Source/future_wells.json", "sha256": "a80187648a96551dcd121b03bca2c6e31ad16e75570ef7f5b33204d18e5554e9"},
            "requirements": ["SPEC-WEL-001..004", "SPEC-WELLP-001..003", "REL-WEL-005/006/010/015/016", "REL-ART-014 (amendment pending)", "REL-ART-016", "REL-ART-030"],
        },
        "acceptance": {"art_gate": "NOT_EVALUATED", "gameplay_gate": "NOT_EVALUATED", "technical_gate": "NOT_EVALUATED", "owner": "NOT_ACCEPTED"},
    }
    manifest["budgets"]["lod0_within_cap"] = lod0_total <= 8000
    manifest["budgets"]["lod1_within_cap"] = lod1_total <= 3500
    return manifest


def main_cli() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--evidence-dir", required=True)
    parser.add_argument("--check", action="store_true", help="read-only: rebuild into a temporary directory and compare every export and review assembly hash with build-manifest.json")
    args = parser.parse_args()
    manifest_path = os.path.join(HERE, "build-manifest.json")
    if args.check:
        if not os.path.exists(manifest_path):
            print(json.dumps({"check": "no-manifest"}))
            return 2
        with open(manifest_path, "r", encoding="utf-8") as handle:
            previous = json.load(handle)
        with tempfile.TemporaryDirectory(prefix="ebs-fwl-check-") as tmp:
            outputs, review, _meshes = build_outputs(os.path.join(tmp, "export"), os.path.join(tmp, "review"))
        prev_out = {o["path"]: o["sha256"] for o in previous.get("outputs", [])}
        prev_review = {os.path.basename(r["path"]): r["sha256"] for r in previous.get("review_assemblies", [])}
        drift = [o["path"] for o in outputs if prev_out.get(o["path"]) != o["sha256"]]
        drift += [os.path.basename(r["path"]) for r in review if prev_review.get(os.path.basename(r["path"])) != r["sha256"]]
        missing = [p for p in prev_out if not os.path.exists(os.path.join(HERE, p))] + [p for p in prev_review if not os.path.exists(os.path.join(args.evidence_dir, "review", p))]
        ok = not drift and not missing and previous.get("revision") == REVISION
        print(json.dumps({"check": "ok" if ok else "drift", "revision": REVISION, "manifest_revision": previous.get("revision"), "drift": drift, "missing": missing,
                          "compared": {"outputs": len(outputs), "review_assemblies": len(review)}}))
        return 0 if ok else 3
    outputs, review, meshes = build_outputs(os.path.join(HERE, "export"), os.path.join(args.evidence_dir, "review"))
    manifest = build_manifest(outputs, review, meshes)
    with open(manifest_path, "w", encoding="utf-8", newline="\n") as handle:
        handle.write(json.dumps(manifest, indent=1, sort_keys=True) + "\n")
    print(json.dumps({"revision": REVISION, "budgets": manifest["budgets"], "wall": manifest["concept_measurements"]["wall"], "spire": manifest["concept_measurements"]["spire"],
                      "apron": manifest["concept_measurements"]["apron"], "walkable_relief_cm": manifest["scale_basis"]["walkable_relief_cm"],
                      "spent_spire_top_z": manifest["dimensions_cm"]["spent_spire_top_z"], "inventory": {k: v.get("built") for k, v in manifest["component_inventory"].items()}}, indent=1))
    return 0


if __name__ == "__main__":
    sys.exit(main_cli())
