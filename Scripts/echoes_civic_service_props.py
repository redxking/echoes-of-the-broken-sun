"""Original M03 ark-city reserve-service presentation recipes.

These static meshes explain the three maintained reserve branches without
creating a powered state, route, collision, or playable structure.  Dimensions
are centimetres and the material slots are civic ceramic, dark hardware, service
ceramic, and muted maintenance inset respectively.

Author: Angelis Pseftis.
"""
from __future__ import annotations

import math

from echoes_evacuation_props import angled_beam, chamfered_prism, tapered_housing, tube_x

REVISION = "m03-civic-service-props-v2"
KINDS = ("LifeSupportBank", "TransitSupport", "ArchiveStack", "ReservePaving")


def life_support_bank(mesh, high: bool):
    """Three-cell life-support manifold with twin exchange housings.

    The paired service bodies, removable collar plates, and side discharge
    trunks make a maintained air-processing bank rather than a generic tower.
    """
    chamfered_prism(mesh, 590, 184, 0, 24, 2, 0, 0, 16)
    chamfered_prism(mesh, 568, 28, 24, 9, 1, 0, -66, 7)
    for x in (-188., 188.):
        tapered_housing(mesh, 146, 130, 24, 188, 0, x, 0, 15, .82)
        tapered_housing(mesh, 109, 94, 212, 42, 2, x, 0, 11, .77)
        chamfered_prism(mesh, 104, 10, 64, 18, 1, x, -70, 3)
        chamfered_prism(mesh, 82, 18, 145, 12, 3, x, -67, 4)
        tube_x(mesh, 96, 10, x, 72, 82, 1, 8)
        tube_x(mesh, 84, 7, x, 72, 119, 1, 8)
        if high:
            for z in (92., 166.):
                # These rub strips are bolted directly into the front shell.
                angled_beam(mesh, 107, 3, z, .8, 3, x, -66.5)
    tapered_housing(mesh, 116, 144, 24, 254, 0, 0, 0, 14, .75)
    chamfered_prism(mesh, 84, 92, 278, 24, 2, 0, 0, 8)
    for x in (-44., 44.):
        angled_beam(mesh, 184, 12, 292, 12, 1, x, 0, math.pi * .5)
    # Crossbar bears on the central cap; its small inspection plate seats on it.
    chamfered_prism(mesh, 230, 25, 302, 9, 1, 0, 0, 6)
    chamfered_prism(mesh, 25, 24, 311, 12, 3, 0, -36, 3)


def transit_support(mesh, high: bool):
    """Three-cell elevated transit support with a retained rail throat.

    Broad splayed feet, paired pylons and the suspended rail deliberately read
    as support infrastructure.  No deck is present, so it cannot imply a new
    crossing or passable bridge.
    """
    chamfered_prism(mesh, 594, 188, 0, 26, 2, 0, 0, 18)
    for x in (-222., 0., 222.):
        tapered_housing(mesh, 108, 138, 26, 35, 0, x, 0, 14, .80)
        # Pylons rise continuously into the retained rail throat above.
        tapered_housing(mesh, 66, 72, 61, 304, 1, x, 0, 8, .88)
        chamfered_prism(mesh, 92, 38, 189, 16, 2, x, 0, 6)
        angled_beam(mesh, 94, 12, 294, 11, 1, x, 0, math.pi * .5)
        chamfered_prism(mesh, 24, 22, 353, 14, 3, x, -36, 3)
    angled_beam(mesh, 584, 34, 365, 23, 1, 0, 0)
    angled_beam(mesh, 572, 10, 388, 8, 0, 0, 0)
    for y in (-48., 48.):
        angled_beam(mesh, 546, 8, 338, 8, 2, 0, y)
    if high:
        for x in (-111., 111.):
            # Each brace reaches the adjacent pylon faces at both ends.
            angled_beam(mesh, 174, 9, 319, 9, 1, x, 0, .44)
            angled_beam(mesh, 174, 9, 319, 9, 1, x, 0, -.44)


def archive_stack(mesh, high: bool):
    """Three protected archive cassettes under a common restraint frame."""
    chamfered_prism(mesh, 592, 184, 0, 22, 2, 0, 0, 16)
    for x in (-194., 0., 194.):
        tapered_housing(mesh, 156, 136, 22, 218, 0, x, 0, 14, .80)
        chamfered_prism(mesh, 116, 14, 78, 13, 1, x, -72, 4)
        chamfered_prism(mesh, 92, 30, 184, 16, 2, x, 0, 7)
        chamfered_prism(mesh, 26, 22, 218, 14, 3, x, -42, 4)
        for y in (-44., 44.):
            # Retention rails sit directly on each cassette cap.
            angled_beam(mesh, 124, 7, 240, 7, 1, x, y)
        if high:
            chamfered_prism(mesh, 12, 94, 129, 10, 1, x + 50, 0, 3)
    for x in (-276., 276.):
        tapered_housing(mesh, 43, 76, 22, 292, 1, x, 0, 8, .88)
    angled_beam(mesh, 562, 24, 314, 17, 1, 0, 0)
    angled_beam(mesh, 548, 8, 331, 7, 2, 0, 0)
    chamfered_prism(mesh, 34, 24, 338, 14, 3, 78, 0, 4)


def reserve_paving(mesh, high: bool):
    """Low continuous reserve trunk marker, flush enough for known open tiles.

    Its fitted seams and replacement plates indicate maintenance direction but
    do not depict a bridge, address, signal, or interactable connection.
    """
    chamfered_prism(mesh, 196, 196, 0, 2.55, 2, 0, 0, 11)
    chamfered_prism(mesh, 174, 164, 2.55, .55, 0, 0, 0, 9)
    angled_beam(mesh, 168, 3, 3.10, .34, 1, 0, -48)
    angled_beam(mesh, 168, 3, 3.10, .34, 1, 0, 48)
    chamfered_prism(mesh, 52, 32, 3.10, .52, 3, -47, 0, 6)
    chamfered_prism(mesh, 38, 30, 3.10, .52, 3, 48, 0, 5)
    if high:
        chamfered_prism(mesh, 66, 3, 3.62, .20, 1, 0, -16, 1)
        chamfered_prism(mesh, 4, 54, 3.62, .20, 1, 14, 20, 1)


def build(mesh, high: bool, kind: str):
    if kind not in KINDS:
        raise ValueError("Unknown civic-service prop kind: %s" % kind)
    {
        "LifeSupportBank": life_support_bank,
        "TransitSupport": transit_support,
        "ArchiveStack": archive_stack,
        "ReservePaving": reserve_paving,
    }[kind](mesh, high)
