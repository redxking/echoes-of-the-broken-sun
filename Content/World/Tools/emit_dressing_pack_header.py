#!/usr/bin/env python3
"""Emit the checked-in C++ constant-table header for the Glass Scar dressing pack.

Reads the frozen dressing pack and its digest sidecar together with the compiled
map pack it is authored against, fails closed on any identity, structural, or
conformance mismatch, and deterministically renders
``Source/EchoesOfTheBrokenSun/Public/EchoesGlassScarDressingPack.h``.

Conformance is re-derived here rather than trusted from the pack: every record
must sit on a cell the compiled map pack marks blocked, because a dressing
silhouette on passable ground would imply an affordance the simulation does not
provide. The emitted header is a pure function of the two pack byte strings.
``--write`` may replace only the one generated header path; ``--check`` (the
default) only compares. Author and owner: Angelis Pseftis.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path
from typing import Any

PACK_RELATIVE_PATH = Path("Content/World/Generated/Dressing/glass_scar_dressing_pack_v1.json")
SIDECAR_RELATIVE_PATH = Path("Content/World/Generated/Dressing/glass_scar_dressing_pack_v1.sha256")
BASE_PACK_RELATIVE_PATH = Path("Content/World/Generated/GlassScar/glass_scar_compiled_map_pack_v1.json")
BASE_SIDECAR_RELATIVE_PATH = Path("Content/World/Generated/GlassScar/glass_scar_compiled_map_pack_v1.sha256")
HEADER_RELATIVE_PATH = Path("Source/EchoesOfTheBrokenSun/Public/EchoesGlassScarDressingPack.h")

EXPECTED_PACK_FORMAT = "echoes-map-dressing-pack"
EXPECTED_PACK_VERSION = 1
EXPECTED_SCHEMA_VERSION = 1
EXPECTED_SITE_ID = "glass-scar"
EXPECTED_GRID = 64
EXPECTED_CELL_COUNT = EXPECTED_GRID * EXPECTED_GRID
CLASS_ORDER = ("vitrified_shelf", "glass_shard")
MAX_ORIENTATION_ORDINAL = 3
MAX_SCALE_BAND = 2


class EmitError(RuntimeError):
    """Raised for any condition that must fail closed."""


def reject_duplicate_keys(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        if key in result:
            raise EmitError(f"duplicate JSON object key: {key!r}")
        result[key] = value
    return result


def reject_nonfinite(constant: str) -> None:
    raise EmitError(f"non-finite JSON constant rejected: {constant}")


def load_digest_pinned(repo_root: Path, pack_relative: Path, sidecar_relative: Path, label: str) -> tuple[dict[str, Any], str]:
    pack_path = repo_root / pack_relative
    sidecar_path = repo_root / sidecar_relative
    if not pack_path.is_file():
        raise EmitError(f"{label} missing: {pack_path}")
    if not sidecar_path.is_file():
        raise EmitError(f"{label} digest sidecar missing: {sidecar_path}")
    raw = pack_path.read_bytes()
    digest = hashlib.sha256(raw).hexdigest()
    sidecar = sidecar_path.read_text(encoding="utf-8").strip()
    if len(sidecar) != 64 or any(c not in "0123456789abcdef" for c in sidecar):
        raise EmitError(f"{label} sidecar is not a bare lowercase SHA-256 hex digest: {sidecar!r}")
    if digest != sidecar:
        raise EmitError(f"{label} digest mismatch: computed {digest} but sidecar records {sidecar}")
    pack = json.loads(raw.decode("utf-8"), object_pairs_hook=reject_duplicate_keys, parse_constant=reject_nonfinite)
    if not isinstance(pack, dict):
        raise EmitError(f"{label} root is not a JSON object")
    return pack, digest


def require_int(value: Any, label: str) -> int:
    if isinstance(value, bool) or not isinstance(value, int):
        raise EmitError(f"{label} must be a JSON integer, got {value!r}")
    return value


def require_str(value: Any, label: str) -> str:
    if not isinstance(value, str):
        raise EmitError(f"{label} must be a JSON string, got {value!r}")
    return value


def blocked_cells(base_pack: dict[str, Any], site_id: str = EXPECTED_SITE_ID) -> list[bool]:
    if base_pack.get("pack_format") == "echoes-overlay-map-pack":
        variants = base_pack.get("variants", [])
        matching = [
            v for v in variants
            if isinstance(v, dict) and (
                v.get("family") == site_id
                or v.get("id") == site_id
                or v.get("id") == f"skirmish-{site_id}"
                or str(v.get("id", "")).startswith(f"{site_id}-")
            )
        ]
        if not matching:
            raise EmitError(f"no overlay variants found for site {site_id!r}")
        common = set.intersection(*(set(v["blocked_cell_indices"]) for v in matching))
        return [index in common for index in range(EXPECTED_CELL_COUNT)]
    mask = base_pack.get("cells", {}).get("movement_mask")
    if not isinstance(mask, list) or len(mask) != EXPECTED_CELL_COUNT:
        raise EmitError("compiled map pack movement_mask is not a 4096-entry array")
    return [(require_int(value, f"movement_mask[{index}]") & 1) == 0 for index, value in enumerate(mask)]


def validate(
    pack: dict[str, Any],
    base_pack: dict[str, Any],
    base_digest: str,
    base_pack_relative: Path = BASE_PACK_RELATIVE_PATH,
    expected_site_id: str = EXPECTED_SITE_ID,
    class_order: tuple[str, ...] = CLASS_ORDER,
) -> tuple[list[dict[str, Any]], list[dict[str, Any]]]:
    if require_str(pack.get("pack_format"), "pack_format") != EXPECTED_PACK_FORMAT:
        raise EmitError("unexpected pack_format")
    if require_int(pack.get("pack_version"), "pack_version") != EXPECTED_PACK_VERSION:
        raise EmitError("unexpected pack_version")
    if require_int(pack.get("schema_version"), "schema_version") != EXPECTED_SCHEMA_VERSION:
        raise EmitError("unexpected schema_version")
    grid = pack.get("grid")
    if not isinstance(grid, dict) or (
        require_int(grid.get("width_tiles"), "grid.width_tiles") != EXPECTED_GRID
        or require_int(grid.get("height_tiles"), "grid.height_tiles") != EXPECTED_GRID
        or require_str(grid.get("index_formula"), "grid.index_formula") != "y*width+x"
    ):
        raise EmitError("grid contract drifted from the 64x64 row-major layout")
    base_contract = pack.get("base_contract")
    if not isinstance(base_contract, dict):
        raise EmitError("base_contract missing")
    if require_str(base_contract.get("compiled_pack_path"), "base_contract.compiled_pack_path") != base_pack_relative.as_posix():
        raise EmitError("base_contract names a different compiled pack path")
    if require_str(base_contract.get("compiled_pack_sha256"), "base_contract.compiled_pack_sha256") != base_digest:
        raise EmitError(
            "base_contract digest does not match the compiled map pack on disk: "
            f"pack records {base_contract.get('compiled_pack_sha256')} but the compiled pack is {base_digest}"
        )

    classes = pack.get("dressing_classes")
    if not isinstance(classes, list) or not classes:
        raise EmitError("dressing_classes must be a non-empty array")
    class_by_id: dict[str, dict[str, Any]] = {}
    for entry in classes:
        if not isinstance(entry, dict):
            raise EmitError("dressing class entry is not an object")
        class_id = require_str(entry.get("id"), "class.id")
        if class_id in class_by_id:
            raise EmitError(f"duplicate dressing class {class_id!r}")
        if require_str(entry.get("permitted_cell_state"), f"class.{class_id}.permitted_cell_state") != "blocked":
            raise EmitError(f"class {class_id!r} is permitted on a cell state the runtime cannot honour")
        if entry.get("occluder") is not True:
            raise EmitError(f"class {class_id!r} must be an occluder to stand on blocked cells")
        require_str(entry.get("mesh_reference"), f"class.{class_id}.mesh_reference")
        class_by_id[class_id] = entry
    if tuple(sorted(class_by_id)) != tuple(sorted(class_order)):
        raise EmitError(f"dressing class vocabulary drifted: {sorted(class_by_id)} versus {sorted(class_order)}")
    ordered_classes = [class_by_id[class_id] for class_id in class_order]

    sites = pack.get("sites")
    if not isinstance(sites, list):
        raise EmitError("sites must be an array")
    site = next((entry for entry in sites if isinstance(entry, dict) and entry.get("id") == expected_site_id), None)
    if site is None:
        raise EmitError(f"site {expected_site_id!r} missing")
    records = site.get("records")
    if not isinstance(records, list) or not records:
        raise EmitError(f"{expected_site_id} site has no records")
    if require_int(site.get("record_count"), "site.record_count") != len(records):
        raise EmitError("site.record_count disagrees with the records array")
    if require_int(pack.get("total_record_count"), "total_record_count") != sum(
        len(entry.get("records", [])) for entry in sites if isinstance(entry, dict)
    ):
        raise EmitError("total_record_count disagrees with the sites")

    blocked = blocked_cells(base_pack, expected_site_id)
    seen_ids: set[str] = set()
    seen_cells: set[int] = set()
    for record in records:
        if not isinstance(record, dict):
            raise EmitError("record is not an object")
        record_id = require_str(record.get("id"), "record.id")
        if record_id in seen_ids:
            raise EmitError(f"duplicate record id {record_id!r}")
        seen_ids.add(record_id)
        class_id = require_str(record.get("class"), f"record {record_id} class")
        if class_id not in class_by_id:
            raise EmitError(f"record {record_id} uses unknown class {class_id!r}")
        x = require_int(record.get("x"), f"record {record_id} x")
        y = require_int(record.get("y"), f"record {record_id} y")
        if not (0 <= x < EXPECTED_GRID and 0 <= y < EXPECTED_GRID):
            raise EmitError(f"record {record_id} is outside the grid")
        cell_index = require_int(record.get("cell_index"), f"record {record_id} cell_index")
        if cell_index != y * EXPECTED_GRID + x:
            raise EmitError(f"record {record_id} cell_index {cell_index} disagrees with ({x},{y})")
        if cell_index in seen_cells:
            raise EmitError(f"record {record_id} shares cell {cell_index} with another record")
        seen_cells.add(cell_index)
        orientation = require_int(record.get("orientation_ordinal"), f"record {record_id} orientation_ordinal")
        if not (0 <= orientation <= MAX_ORIENTATION_ORDINAL):
            raise EmitError(f"record {record_id} orientation_ordinal out of range")
        scale_band = require_int(record.get("scale_band"), f"record {record_id} scale_band")
        if not (0 <= scale_band <= MAX_SCALE_BAND):
            raise EmitError(f"record {record_id} scale_band out of range")
        if not blocked[cell_index]:
            raise EmitError(
                f"record {record_id} ({class_id}) stands on passable cell {cell_index}; "
                "a dressing occluder may only stand on a blocked cell"
            )
    return ordered_classes, records


def cpp_string(value: str) -> str:
    escaped = value.replace("\\", "\\\\").replace('"', '\\"')
    return f'"{escaped}"'


def render_header(
    pack: dict[str, Any],
    digest: str,
    base_pack: dict[str, Any],
    base_digest: str,
    pack_relative: Path = PACK_RELATIVE_PATH,
    base_pack_relative: Path = BASE_PACK_RELATIVE_PATH,
    header_relative: Path = HEADER_RELATIVE_PATH,
    expected_site_id: str = EXPECTED_SITE_ID,
    cpp_namespace: str = "glass_scar_dressing",
    class_order: tuple[str, ...] = CLASS_ORDER,
) -> str:
    classes, records = validate(
        pack,
        base_pack,
        base_digest,
        base_pack_relative=base_pack_relative,
        expected_site_id=expected_site_id,
        class_order=class_order,
    )
    lines: list[str] = []
    push = lines.append
    push(f"// {header_relative.name}")
    push("// GENERATED FILE - do not edit by hand.")
    push("// Generated by Content/World/Tools/emit_dressing_pack_header.py from the")
    push(f"// frozen dressing pack {pack_relative.as_posix()}")
    push(f"// authored against {base_pack_relative.as_posix()}.")
    push("// Verify:   python3 Content/World/Tools/emit_dressing_pack_header.py --check")
    push("// Rewrite:  python3 Content/World/Tools/emit_dressing_pack_header.py --write")
    push("// Every record below was re-verified at emission to stand on a cell the")
    push("// compiled map pack marks blocked. The runtime re-checks against live terrain")
    push("// and hides any record whose tile is unexplored or reshaped open. Presentation")
    push("// only: nothing here enters simulation, fog authority, saves, or checksums.")
    push("")
    push("#pragma once")
    push("")
    push("#include <cstdint>")
    push("")
    push(f"namespace echoes::world::{cpp_namespace}")
    push("{")
    push("")
    push(f"inline constexpr std::int32_t kGridWidthTiles = {EXPECTED_GRID};")
    push(f"inline constexpr std::int32_t kGridHeightTiles = {EXPECTED_GRID};")
    push(f"inline constexpr char kPackFormat[] = {cpp_string(pack['pack_format'])};")
    push(f"inline constexpr char kSiteId[] = {cpp_string(expected_site_id)};")
    push(f"inline constexpr std::int32_t kPackVersion = {pack['pack_version']};")
    push(f"inline constexpr std::int32_t kSchemaVersion = {pack['schema_version']};")
    push(f"inline constexpr char kPackSha256[] = {cpp_string(digest)};")
    push("// The compiled map pack these records were authored and verified against.")
    push("// The terrain view static_asserts this equals the compiled pack it binds.")
    push(f"inline constexpr char kBaseCompiledPackSha256[] = {cpp_string(base_digest)};")
    push("")
    push("enum class EDressingClass : std::uint8_t")
    push("{")
    for index, class_id in enumerate(class_order):
        push(f"    {to_enum(class_id)} = {index},")
    push("};")
    push("")
    push("struct FDressingClass")
    push("{")
    push("    EDressingClass Class;")
    push("    const char* Id;")
    push("    const char* MeshReference;")
    push("    const char* ReadsAs;")
    push("};")
    push("")
    push(f"inline constexpr FDressingClass kClasses[{len(classes)}] = {{")
    for entry in classes:
        push(
            "    {"
            f"EDressingClass::{to_enum(entry['id'])}, {cpp_string(entry['id'])}, "
            f"{cpp_string(entry['mesh_reference'])}, {cpp_string(require_str(entry.get('reads_as', ''), 'reads_as'))}"
            "},"
        )
    push("};")
    push("")
    push("struct FDressingRecord")
    push("{")
    push("    EDressingClass Class;")
    push("    std::uint8_t X;")
    push("    std::uint8_t Y;")
    push("    std::uint8_t OrientationOrdinal;")
    push("    std::uint8_t ScaleBand;")
    push("    std::int32_t CellIndex;")
    push("    const char* Id;")
    push("};")
    push("")
    push(f"inline constexpr std::int32_t kRecordCount = {len(records)};")
    push(f"inline constexpr FDressingRecord kRecords[kRecordCount] = {{")
    for record in records:
        push(
            "    {"
            f"EDressingClass::{to_enum(record['class'])}, {record['x']}, {record['y']}, "
            f"{record['orientation_ordinal']}, {record['scale_band']}, {record['cell_index']}, "
            f"{cpp_string(record['id'])}"
            "},"
        )
    push("};")
    push("")
    push(f"}}  // namespace echoes::world::{cpp_namespace}")
    return "\n".join(lines) + "\n"


def to_enum(class_id: str) -> str:
    return "".join(part.capitalize() for part in class_id.split("_"))


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", type=Path, default=Path("."), help="Project root containing Content/ and Source/.")
    parser.add_argument("--dressing-pack", type=Path, default=PACK_RELATIVE_PATH, help="Relative path to dressing pack JSON.")
    parser.add_argument("--base-pack", type=Path, default=BASE_PACK_RELATIVE_PATH, help="Relative path to base compiled map pack JSON.")
    parser.add_argument("--header", type=Path, default=HEADER_RELATIVE_PATH, help="Relative path to output C++ header.")
    parser.add_argument("--site-id", type=str, default=EXPECTED_SITE_ID, help="Expected site ID.")
    parser.add_argument("--namespace", type=str, default="glass_scar_dressing", help="C++ namespace inside echoes::world.")
    parser.add_argument("--classes", type=str, default=",".join(CLASS_ORDER), help="Comma-separated class IDs in order.")
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--check", action="store_true", help="Verify the checked-in header matches the packs (default).")
    mode.add_argument("--write", action="store_true", help="Rewrite the generated header from the packs.")
    mode.add_argument("--stdout", action="store_true", help="Render the header to standard output.")
    args = parser.parse_args(argv)

    repo_root = args.repo_root.resolve()
    pack_rel = args.dressing_pack
    sidecar_rel = pack_rel.with_suffix(".sha256")
    base_rel = args.base_pack
    base_sidecar_rel = base_rel.with_suffix(".sha256")
    header_rel = args.header
    site_id = args.site_id
    cpp_namespace = args.namespace
    class_order = tuple(c.strip() for c in args.classes.split(","))

    try:
        base_pack, base_digest = load_digest_pinned(repo_root, base_rel, base_sidecar_rel, "compiled map pack")
        pack, digest = load_digest_pinned(repo_root, pack_rel, sidecar_rel, "dressing pack")
        rendered = render_header(
            pack,
            digest,
            base_pack,
            base_digest,
            pack_relative=pack_rel,
            base_pack_relative=base_rel,
            header_relative=header_rel,
            expected_site_id=site_id,
            cpp_namespace=cpp_namespace,
            class_order=class_order,
        )
    except EmitError as error:
        print(f"REFUSED: {error}", file=sys.stderr)
        return 1

    header_path = repo_root / header_rel
    if args.stdout:
        sys.stdout.write(rendered)
        return 0
    if args.write:
        header_path.parent.mkdir(parents=False, exist_ok=True)
        header_path.write_bytes(rendered.encode("utf-8"))
        print(f"wrote {header_path} ({len(rendered)} bytes) digest={digest} base={base_digest}")
        return 0
    if not header_path.is_file():
        print(f"REFUSED: checked-in header missing: {header_path}", file=sys.stderr)
        return 1
    if header_path.read_text(encoding="utf-8") != rendered:
        print("REFUSED: checked-in header does not match the packs; regenerate with --write and re-review", file=sys.stderr)
        return 1
    print(f"header matches packs; digest={digest} base={base_digest}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
