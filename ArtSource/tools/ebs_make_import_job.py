#!/usr/bin/env python3
"""Write an isolated-import job JSON from a package build manifest.

Author: Angelis Pseftis.
Usage:
  python3 ebs_make_import_job.py --manifest ArtSource/<ID>/build-manifest.json \
      --evidence-dir "<evidence root>/<ID>" --destination /Game/Echoes/Production/<DOM>/<TYP>/<ID_> \
      [--kind static|skeletal|auto] [--mesh NAME ...] [--out <path>]

The job feeds `ue_import_inspect.py` (static) or `ue_import_inspect_skeletal.py` (skeletal). Both read
the path in EBS_IMPORT_JOB. Expectations are derived from the manifest so the inspector compares the
engine against the generator's own numbers instead of numbers retyped by hand:

  static   : per mesh LOD0/LOD1 GLB paths, LOD1 section slot names, LOD0/LOD1 triangles, height,
             collision box count, socket names and yaw rotations.
  skeletal : per asset the skinned GLB, the clip name list, and expected bones, sockets, clips and
             LOD0 triangles.

The manifest is the contract: a missing field is an error here rather than a silent default, because a
job that omits an expectation makes the inspector pass vacuously. Stdlib only.
"""
from __future__ import annotations

import argparse
import json
import os
import sys


def load(path: str) -> dict:
    with open(path, "r", encoding="utf-8") as handle:
        return json.load(handle)


def glb_rows(manifest: dict, skinned: bool | None = None) -> list:
    """Every GLB row in the manifest's outputs list, in manifest order.

    A package can export two GLBs for the same (mesh, lod) — the skinned mesh and a rest-pose static
    twin (``*_static.glb``) — so rows are kept as a list; keying them by (mesh, lod) silently drops one
    and was the reason an early version of this tool emitted a skeletal job with no sockets.
    ``skinned=True`` keeps only skinned rows, ``False`` only unskinned ones.
    """
    out = []
    for o in manifest.get("outputs") or []:
        if not isinstance(o, dict) or "path" not in o or not str(o["path"]).endswith(".glb"):
            continue
        if "mesh" not in o or "lod" not in o:
            continue
        is_skinned = o.get("kind") == "skinned" or (str(o["mesh"]).startswith("SK_") and not str(o["path"]).endswith("_static.glb"))
        if skinned is None or is_skinned == skinned:
            out.append(o)
    return out


def pick(rows: list, mesh: str, lod: int):
    for o in rows:
        if o["mesh"] == mesh and int(o["lod"]) == lod:
            return o
    return None


def mesh_order(rows: list) -> list:
    names = []
    for o in rows:
        if o["mesh"] not in names:
            names.append(o["mesh"])
    return names


def require(row: dict, key: str, where: str):
    if key not in row:
        raise SystemExit(f"manifest row {where} has no '{key}'; the generator must record it")
    return row[key]


def static_job(manifest: dict, source_root: str, destination: str, report: str, meshes) -> dict:
    rows = glb_rows(manifest, skinned=False)
    names = meshes or mesh_order(rows)
    assets, expected = [], {}
    for name in names:
        o0, o1 = pick(rows, name, 0), pick(rows, name, 1)
        if o0 is None or o1 is None:
            raise SystemExit(f"{name}: manifest lacks a LOD0 and LOD1 GLB row")
        socks = o0.get("sockets") or []
        assets.append({
            "name": name,
            "lod0": os.path.join(source_root, require(o0, "path", f"{name}/LOD0")),
            "lod1": os.path.join(source_root, require(o1, "path", f"{name}/LOD1")),
            "lod1_section_slot_names": require(o1, "section_slot_names", f"{name}/LOD1"),
        })
        expected[name] = {
            "lod0_triangles": require(o0, "triangles", f"{name}/LOD0"),
            "lod1_triangles": require(o1, "triangles", f"{name}/LOD1"),
            "height_cm": round(float(require(o0, "bounds_cm", f"{name}/LOD0")[1][2]), 2),
            "collision_boxes": len(o0.get("collision_boxes") or []),
            "sockets": sorted(s["name"] for s in socks),
            "socket_rotations": {s["name"]: [0, float(s.get("yaw_deg", 0.0)), 0] for s in socks},
        }
    return {"author": "Angelis Pseftis", "kind": "static", "destination": destination, "report": report,
            "revision": manifest.get("revision"), "assets": assets, "expected": expected}


def skeletal_job(manifest: dict, source_root: str, destination: str, report: str, meshes) -> dict:
    asset = manifest.get("asset_name") or manifest.get("production_asset_id")
    clips = [c["name"] for c in manifest.get("clips") or []]
    if not clips:
        raise SystemExit("manifest has no clips; a skeletal job needs the clip list")
    bones = [b["name"] for b in (manifest.get("rig") or {}).get("bones") or []]
    if not bones:
        raise SystemExit("manifest has no rig.bones")
    rows = glb_rows(manifest, skinned=True)
    if not rows:
        raise SystemExit("manifest has no skinned SK_* GLB rows; run the generator with --skinned")
    # A package may ship more than one skinned mesh: the unit itself plus skinned sub-objects (for
    # example the Bulwark's barrier pane assembly, which follows the same cell bones). Only the rows
    # whose mesh IS the package asset are the unit's LODs; every other skinned mesh is its own asset.
    # Treating a sub-object as the unit's LOD1 makes two entries import under one name, and the second
    # returns no objects — which is exactly how this went wrong once.
    primary = [o for o in rows if o["mesh"] == asset]
    others = [o for o in rows if o["mesh"] != asset]
    assets = []
    for row in sorted(primary, key=lambda o: int(o["lod"])):
        lod = int(row["lod"])
        entry_name = asset if lod == 0 else f"{asset}_LOD{lod}Source"
        if meshes and entry_name not in meshes:
            continue
        assets.append({"name": entry_name, "file": os.path.join(source_root, row["path"]), "clips": clips})
    for row in sorted(others, key=lambda o: (o["mesh"], int(o["lod"]))):
        lod = int(row["lod"])
        entry_name = row["mesh"] if lod == 0 else f"{row['mesh']}_LOD{lod}Source"
        if meshes and entry_name not in meshes:
            continue
        assets.append({"name": entry_name, "file": os.path.join(source_root, row["path"]),
                       "clips": clips, "sub_object_of": asset})
    o0 = pick(primary, asset, 0) or primary[0]
    socks = sorted(s["name"] for s in (o0.get("sockets") or []))
    if not socks:
        inv = (manifest.get("component_inventory") or {}).get("sockets") or {}
        socks = sorted(inv.get("built") or inv.get("contract") or [])
    if not socks:
        raise SystemExit("no sockets found in the skinned GLB row or component_inventory; the generator must record them")
    expected = {asset: {
        "revision": manifest.get("revision"),
        "bones": bones,
        "sockets": socks,
        "clips": {c["name"]: {"duration_s": c["duration_s"], "bones": c.get("bones")} for c in manifest["clips"]},
        "lod0_triangles": (manifest.get("budgets") or {}).get("lod0_triangles"),
    }}
    return {"author": "Angelis Pseftis", "kind": "skeletal", "destination": destination, "report": report,
            "revision": manifest.get("revision"), "assets": assets, "expected": expected}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", required=True)
    parser.add_argument("--evidence-dir", required=True, help="<evidence root>/<production id>")
    parser.add_argument("--destination", required=True)
    parser.add_argument("--kind", default="auto", choices=("auto", "static", "skeletal"))
    parser.add_argument("--mesh", action="append", default=[])
    parser.add_argument("--out", default=None)
    parser.add_argument("--report", default=None)
    args = parser.parse_args()
    manifest = load(args.manifest)
    source_root = os.path.dirname(os.path.abspath(args.manifest))
    kind = args.kind
    if kind == "auto":
        kind = "skeletal" if manifest.get("clips") and (manifest.get("rig") or {}).get("bones") else "static"
    rev = (manifest.get("revision") or "job").replace(".", "-")
    out = args.out or os.path.join(args.evidence_dir, "import", f"import-job-{rev}.json")
    report = args.report or os.path.join(args.evidence_dir, "import", f"import-report-{rev}.json")
    os.makedirs(os.path.dirname(out), exist_ok=True)
    job = (skeletal_job if kind == "skeletal" else static_job)(manifest, source_root, args.destination, report, args.mesh)
    with open(out, "w", encoding="utf-8") as handle:
        json.dump(job, handle, indent=1)
    print(json.dumps({"job": out, "kind": kind, "revision": job["revision"], "assets": [a["name"] for a in job["assets"]],
                      "report": report, "script": "ue_import_inspect_skeletal.py" if kind == "skeletal" else "ue_import_inspect.py"}))
    return 0


if __name__ == "__main__":
    sys.exit(main())
