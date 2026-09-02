#!/usr/bin/env python3
"""Compile the Glass Scar map-dressing pack from its authoring source.

Reads the dressing source, digest-gates the compiled map pack it declares, and
enforces the dressing conformance rule: every placed record must sit on a cell
whose gameplay state matches the state its class is permitted to occupy. An
occluder may only stand on a contract-blocked cell; walkable dressing may only
stand on a passable one. That is what keeps presentation from implying a
gameplay affordance the simulation does not provide.

Fail-closed: any structural, digest, duplicate, range, or conformance mismatch
refuses compilation. The tool writes only the two paths under
``Content/World/Generated/Dressing/`` and never touches the authoring source,
the accepted map/overlay fixtures, or the accepted compilers.

Determinism: the emitted pack is a pure function of the two input files; two
runs are byte-identical. Orientation and scale are authored ordinals, not
random draws, so runtime placement is reproducible without an RNG.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path
from typing import Any

GRID_WIDTH = 64
GRID_HEIGHT = 64
CELL_COUNT = GRID_WIDTH * GRID_HEIGHT

SOURCE_RELATIVE_PATH = Path(
    "Content/World/Source/GlassScar/glass_scar_dressing_v1.json"
)
OUTPUT_PACK_RELATIVE_PATH = Path(
    "Content/World/Generated/Dressing/glass_scar_dressing_pack_v1.json"
)
OUTPUT_SIDECAR_RELATIVE_PATH = Path(
    "Content/World/Generated/Dressing/glass_scar_dressing_pack_v1.sha256"
)

PERMITTED_CELL_STATES = ("blocked", "passable")
MAX_ORIENTATION_ORDINAL = 3
MAX_SCALE_BAND = 2


class CompileError(RuntimeError):
    """Raised for any condition that must fail closed."""


def reject_duplicate_keys(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        if key in result:
            raise CompileError(f"duplicate JSON object key: {key!r}")
        result[key] = value
    return result


def reject_nonfinite(constant: str) -> None:
    raise CompileError(f"non-finite JSON constant rejected: {constant}")


def load_strict_json(path: Path) -> tuple[Any, str]:
    if not path.is_file():
        raise CompileError(f"input missing: {path}")
    raw = path.read_bytes()
    digest = hashlib.sha256(raw).hexdigest()
    value = json.loads(
        raw.decode("utf-8"),
        object_pairs_hook=reject_duplicate_keys,
        parse_constant=reject_nonfinite,
    )
    return value, digest


def require_int(value: Any, label: str) -> int:
    if isinstance(value, bool) or not isinstance(value, int):
        raise CompileError(f"{label} must be a JSON integer, got {value!r}")
    return value


def require_str(value: Any, label: str) -> str:
    if not isinstance(value, str) or not value:
        raise CompileError(f"{label} must be a non-empty JSON string")
    return value


def exact_keys(value: Any, allowed: set[str], label: str) -> dict[str, Any]:
    if not isinstance(value, dict):
        raise CompileError(f"{label} must be a JSON object")
    present = set(value)
    if present != allowed:
        unexpected = sorted(present - allowed)
        missing = sorted(allowed - present)
        raise CompileError(
            f"{label}: exact-key mismatch (unexpected: {unexpected}; missing: {missing})"
        )
    return value


def load_blocked_cells(repo_root: Path, base: dict[str, Any]) -> frozenset[int]:
    pack_path = repo_root / require_str(
        base["compiled_pack_path"], "base_contract.compiled_pack_path"
    )
    pinned = require_str(
        base["compiled_pack_sha256"], "base_contract.compiled_pack_sha256"
    )
    if not pack_path.is_file():
        raise CompileError(f"compiled map pack missing: {pack_path}")
    raw = pack_path.read_bytes()
    digest = hashlib.sha256(raw).hexdigest()
    if digest != pinned:
        raise CompileError(
            f"compiled map pack digest mismatch: computed {digest}, source pins {pinned}"
        )
    pack = json.loads(
        raw.decode("utf-8"),
        object_pairs_hook=reject_duplicate_keys,
        parse_constant=reject_nonfinite,
    )
    mask = pack["cells"]["movement_mask"]
    if not isinstance(mask, list) or len(mask) != CELL_COUNT:
        raise CompileError("compiled map pack movement mask is not a 4096-entry array")
    return frozenset(
        index for index, value in enumerate(mask) if (value & 1) == 0
    )


def compile_pack(repo_root: Path) -> dict[str, Any]:
    source, source_digest = load_strict_json(repo_root / SOURCE_RELATIVE_PATH)
    top = exact_keys(
        source,
        {
            "source_format",
            "source_version",
            "display_name",
            "grid",
            "authority_note",
            "base_contract",
            "dressing_classes",
            "deferred_classes",
            "sites",
            "claim_boundary",
        },
        "source",
    )
    if top["source_format"] != "echoes-map-dressing-source":
        raise CompileError("source: unexpected source_format")
    if top["source_version"] != 1:
        raise CompileError("source: unexpected source_version")
    grid = exact_keys(
        top["grid"],
        {"width_tiles", "height_tiles", "index_formula", "coordinate_origin"},
        "source.grid",
    )
    if (
        grid["width_tiles"] != GRID_WIDTH
        or grid["height_tiles"] != GRID_HEIGHT
        or grid["index_formula"] != "y*width+x"
        or grid["coordinate_origin"] != "southwest"
    ):
        raise CompileError("source.grid: grid contract drifted")

    base = exact_keys(
        top["base_contract"],
        {"compiled_pack_path", "compiled_pack_sha256"},
        "source.base_contract",
    )
    blocked = load_blocked_cells(repo_root, base)

    classes: dict[str, dict[str, Any]] = {}
    for index, item in enumerate(top["dressing_classes"]):
        record = exact_keys(
            item,
            {"id", "permitted_cell_state", "mesh_reference", "reads_as", "occluder"},
            f"source.dressing_classes[{index}]",
        )
        class_id = require_str(record["id"], f"source.dressing_classes[{index}].id")
        if class_id in classes:
            raise CompileError(f"source.dressing_classes: duplicate class {class_id!r}")
        state = require_str(
            record["permitted_cell_state"],
            f"source.dressing_classes[{index}].permitted_cell_state",
        )
        if state not in PERMITTED_CELL_STATES:
            raise CompileError(
                f"source.dressing_classes[{index}].permitted_cell_state: "
                f"expected one of {list(PERMITTED_CELL_STATES)}"
            )
        require_str(
            record["mesh_reference"], f"source.dressing_classes[{index}].mesh_reference"
        )
        require_str(record["reads_as"], f"source.dressing_classes[{index}].reads_as")
        if not isinstance(record["occluder"], bool):
            raise CompileError(
                f"source.dressing_classes[{index}].occluder must be a JSON boolean"
            )
        if record["occluder"] and state != "blocked":
            raise CompileError(
                f"source.dressing_classes[{index}]: an occluder must be permitted only on "
                "blocked cells, otherwise it implies impassability the contract denies"
            )
        classes[class_id] = record

    deferred: list[dict[str, Any]] = []
    for index, item in enumerate(top["deferred_classes"]):
        record = exact_keys(
            item,
            {"id", "mesh_reference", "deferred_reason"},
            f"source.deferred_classes[{index}]",
        )
        deferred_id = require_str(record["id"], f"source.deferred_classes[{index}].id")
        if deferred_id in classes:
            raise CompileError(
                f"source.deferred_classes[{index}]: {deferred_id!r} is also an active class"
            )
        require_str(
            record["mesh_reference"], f"source.deferred_classes[{index}].mesh_reference"
        )
        require_str(
            record["deferred_reason"],
            f"source.deferred_classes[{index}].deferred_reason",
        )
        deferred.append(
            {
                "id": deferred_id,
                "mesh_reference": record["mesh_reference"],
                "deferred_reason": record["deferred_reason"],
            }
        )

    sites: list[dict[str, Any]] = []
    seen_ids: set[str] = set()
    for index, item in enumerate(top["sites"]):
        site_id = require_str(item.get("id"), f"source.sites[{index}].id")
        status = require_str(
            item.get("vocabulary_status"), f"source.sites[{index}].vocabulary_status"
        )
        if status == "populated":
            record = exact_keys(
                item,
                {"id", "vocabulary_status", "records"},
                f"source.sites[{index}]",
            )
        elif status == "empty":
            record = exact_keys(
                item,
                {"id", "vocabulary_status", "empty_reason", "records"},
                f"source.sites[{index}]",
            )
            require_str(
                record["empty_reason"], f"source.sites[{index}].empty_reason"
            )
            if record["records"]:
                raise CompileError(
                    f"source.sites[{index}]: an empty vocabulary must carry no records"
                )
        else:
            raise CompileError(
                f"source.sites[{index}].vocabulary_status: expected populated or empty"
            )

        compiled_records: list[dict[str, Any]] = []
        for position, entry in enumerate(record["records"]):
            where = f"source.sites[{index}].records[{position}]"
            fields = exact_keys(
                entry,
                {"id", "class", "x", "y", "orientation_ordinal", "scale_band"},
                where,
            )
            record_id = require_str(fields["id"], f"{where}.id")
            if record_id in seen_ids:
                raise CompileError(f"{where}.id: duplicate {record_id!r}")
            seen_ids.add(record_id)
            class_id = require_str(fields["class"], f"{where}.class")
            if class_id not in classes:
                raise CompileError(f"{where}.class: unknown class {class_id!r}")
            x = require_int(fields["x"], f"{where}.x")
            y = require_int(fields["y"], f"{where}.y")
            if not (0 <= x < GRID_WIDTH and 0 <= y < GRID_HEIGHT):
                raise CompileError(f"{where}: tile ({x},{y}) is out of range")
            orientation = require_int(
                fields["orientation_ordinal"], f"{where}.orientation_ordinal"
            )
            if not 0 <= orientation <= MAX_ORIENTATION_ORDINAL:
                raise CompileError(
                    f"{where}.orientation_ordinal: expected 0..{MAX_ORIENTATION_ORDINAL}"
                )
            scale_band = require_int(fields["scale_band"], f"{where}.scale_band")
            if not 0 <= scale_band <= MAX_SCALE_BAND:
                raise CompileError(f"{where}.scale_band: expected 0..{MAX_SCALE_BAND}")

            # Conformance gate: the placed record must agree with the gameplay
            # contract about what this cell is.
            cell_index = y * GRID_WIDTH + x
            actual_state = "blocked" if cell_index in blocked else "passable"
            required = classes[class_id]["permitted_cell_state"]
            if actual_state != required:
                raise CompileError(
                    f"{where}: class {class_id!r} may only stand on {required} cells, but "
                    f"({x},{y}) is {actual_state} in the compiled map contract"
                )
            compiled_records.append(
                {
                    "id": record_id,
                    "class": class_id,
                    "cell_index": cell_index,
                    "x": x,
                    "y": y,
                    "orientation_ordinal": orientation,
                    "scale_band": scale_band,
                }
            )
        compiled_records.sort(key=lambda item: item["id"])

        compiled_site: dict[str, Any] = {
            "id": site_id,
            "vocabulary_status": status,
            "record_count": len(compiled_records),
            "records": compiled_records,
        }
        if status == "empty":
            compiled_site["empty_reason"] = record["empty_reason"]
        sites.append(compiled_site)
    sites.sort(key=lambda item: item["id"])

    return {
        "pack_format": "echoes-map-dressing-pack",
        "pack_version": 1,
        "schema_version": 1,
        "authority": "checked-in-source-fixture",
        "runtime_binding": "none",
        "grid": {
            "width_tiles": GRID_WIDTH,
            "height_tiles": GRID_HEIGHT,
            "cell_count": CELL_COUNT,
            "index_formula": "y*width+x",
            "coordinate_origin": "southwest",
        },
        "base_contract": {
            "compiled_pack_path": base["compiled_pack_path"],
            "compiled_pack_sha256": base["compiled_pack_sha256"],
        },
        "source_provenance": {
            "path": SOURCE_RELATIVE_PATH.as_posix(),
            "raw_sha256": source_digest,
        },
        "dressing_classes": [
            {
                "id": classes[key]["id"],
                "permitted_cell_state": classes[key]["permitted_cell_state"],
                "mesh_reference": classes[key]["mesh_reference"],
                "reads_as": classes[key]["reads_as"],
                "occluder": classes[key]["occluder"],
            }
            for key in sorted(classes)
        ],
        "deferred_classes": sorted(deferred, key=lambda item: item["id"]),
        "sites": sites,
        "total_record_count": sum(site["record_count"] for site in sites),
        "claim_boundary": top["claim_boundary"],
    }


def render_bytes(pack: dict[str, Any]) -> bytes:
    return json.dumps(
        pack, sort_keys=True, separators=(",", ":"), ensure_ascii=True
    ).encode("utf-8")


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", type=Path, default=Path("."))
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--check", action="store_true", help="Verify (default mode).")
    mode.add_argument("--write", action="store_true", help="Rewrite pack and sidecar.")
    args = parser.parse_args(argv)
    repo_root = args.repo_root.resolve()
    try:
        pack = compile_pack(repo_root)
    except CompileError as error:
        print(f"REFUSED: {error}", file=sys.stderr)
        return 1
    rendered = render_bytes(pack)
    digest = hashlib.sha256(rendered).hexdigest()
    pack_path = repo_root / OUTPUT_PACK_RELATIVE_PATH
    sidecar_path = repo_root / OUTPUT_SIDECAR_RELATIVE_PATH
    if args.write:
        pack_path.parent.mkdir(parents=True, exist_ok=True)
        pack_path.write_bytes(rendered)
        sidecar_path.write_text(digest + "\n", encoding="utf-8", newline="\n")
        print(f"wrote {pack_path} ({len(rendered)} bytes); digest={digest}")
        return 0
    if not pack_path.is_file() or not sidecar_path.is_file():
        print(
            "REFUSED: checked-in dressing pack or sidecar missing; "
            "compile with --write and re-review",
            file=sys.stderr,
        )
        return 1
    if (
        pack_path.read_bytes() != rendered
        or sidecar_path.read_text(encoding="utf-8").strip() != digest
    ):
        print(
            "REFUSED: checked-in dressing pack or sidecar does not match the source; "
            "recompile with --write and re-review",
            file=sys.stderr,
        )
        return 1
    print(f"dressing pack matches source; digest={digest}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
