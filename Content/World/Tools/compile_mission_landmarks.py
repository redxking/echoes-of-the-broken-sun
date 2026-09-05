#!/usr/bin/env python3
"""Strict compiler for fog-scoped, presentation-only campaign landmark packs."""
from __future__ import annotations
import argparse, hashlib, importlib.util, json, os, re, sys, tempfile
from pathlib import Path, PurePosixPath

W = H = 64
YAW = {0, 90, 180, 270}
ID = re.compile(r"^[a-z][a-z0-9-]{0,63}$")
PACK_FILE = re.compile(r"^m([0-9]{2})_[a-z0-9-]+_landmarks_v1\.json$")
# Kinds describe a cosmetic mesh family's legal terrain only. Simulation terrain
# remains authoritative and every record is checked across all doctrine variants.
MISSIONS = {
    "M01": ("glass-scar-evacuation-margin", "CampaignPrologue", (
        ("ArchiveCradle", True, None), ("ArchiveFrame", True, None),
        ("RoutePaving", False, None), ("ServiceConduit", True, None),
        ("ArchiveApron", False, (5, 4)), ("ArchiveLoadingFace", True, (5, 1)))),
    "M03": ("ark-city-reserve-service", "CampaignCityReserve", (
        ("LifeSupportBank", True, "rotated-3x1"), ("TransitSupport", True, "rotated-3x1"),
        ("ArchiveStack", True, "rotated-3x1"), ("ReservePaving", False, None))),
    "M02": ("shivergrass-migration-basin", "CampaignSevenAccounts", (
        ("ObservationSill", True, "rotated-3x1"), ("RootingShoulder", True, None),
        ("PassagePaving", False, None))),
}

class CompileError(RuntimeError): pass

def dup(pairs):
    value = {}
    for key, item in pairs:
        if key in value: raise CompileError("duplicate JSON key: %r" % key)
        value[key] = item
    return value

def load(path):
    if not path.is_file(): raise CompileError("missing regular input: %s" % path)
    raw = path.read_bytes()
    try:
        value = json.loads(raw.decode("utf-8"), object_pairs_hook=dup,
            parse_constant=lambda _: (_ for _ in ()).throw(CompileError("nonfinite JSON")))
    except (UnicodeDecodeError, json.JSONDecodeError) as error:
        raise CompileError("invalid JSON %s: %s" % (path, error))
    if not isinstance(value, dict): raise CompileError("root must be object")
    return value, hashlib.sha256(raw).hexdigest()

def exact(value, keys, label):
    if not isinstance(value, dict) or set(value) != set(keys):
        raise CompileError("%s: exact-key mismatch" % label)
    return value

def string(value, label):
    if not isinstance(value, str) or not value: raise CompileError("%s: nonempty string required" % label)
    return value

def integer(value, label):
    if isinstance(value, bool) or not isinstance(value, int): raise CompileError("%s: integer required" % label)
    return value

def confined(root, supplied, prefix, label, required):
    path = Path(supplied)
    if path.is_absolute(): raise CompileError("%s: absolute path refused" % label)
    rel = PurePosixPath(path.as_posix())
    if ".." in rel.parts or not rel.is_relative_to(prefix):
        raise CompileError("%s: path must remain under %s" % (label, prefix))
    candidate, current = root.joinpath(*rel.parts), root
    for part in rel.parts:
        current /= part
        if current.is_symlink(): raise CompileError("%s: symlink refused" % label)
    if required and not candidate.is_file(): raise CompileError("%s: source missing" % label)
    if not required and candidate.exists() and not candidate.is_file():
        raise CompileError("%s: output must be a regular file" % label)
    return candidate

def terrain(root, value):
    text = string(value, "terrain_source_path")
    pure, prefix = PurePosixPath(text), PurePosixPath("Content/World/Source/Campaign")
    if pure.is_absolute() or ".." in pure.parts or not pure.is_relative_to(prefix) or pure.suffix != ".json":
        raise CompileError("terrain_source_path: campaign source path refused")
    return confined(root, pure, prefix, "terrain_source_path", True)

def variants(source, digest, mission, map_id, operation):
    spec = importlib.util.spec_from_file_location("campaign_map_pack", Path(__file__).with_name("compile_campaign_map_pack.py"))
    module = importlib.util.module_from_spec(spec); spec.loader.exec_module(module)
    try:
        parsed = module.parse_source(source, digest,
            {"mission_code": mission, "map_id": map_id, "operation_mode": operation},
            "landmark terrain source")
    except module.CompileError as error:
        raise CompileError("referenced campaign terrain refused: %s" % error)
    return [{i for i, value in enumerate(row["movement_mask"]) if value == 0}
            for row in parsed["variants"]]

def parse_pack(root, supplied):
    path = confined(root, supplied, PurePosixPath("Content/World/Source/Presentation"), "source", True)
    match = PACK_FILE.fullmatch(path.name)
    # The production discovery path requires the stable filename.  Keep the
    # callable compiler usable for confined temporary fixtures as well.
    doc, digest = load(path)
    exact(doc, ["format", "version", "author", "mission_code", "map_id", "production_brief",
                "terrain_source_path", "terrain_source_sha256", "records"], "landmarks")
    mission = doc["mission_code"]
    if mission not in MISSIONS or (match and mission != "M" + match.group(1)):
        raise CompileError("unsupported or filename-mismatched landmark mission")
    map_id, operation, kind_specs = MISSIONS[mission]
    ordinal = int(mission[1:])
    if doc["format"] != "echoes-mission-landmarks" or integer(doc["version"], "version") != 1 or doc["author"] != "Angelis Pseftis":
        raise CompileError("landmarks identity invalid")
    if doc["map_id"] != map_id: raise CompileError("landmark map identity mismatch")
    brief = exact(doc["production_brief"], ["mission_requirement", "map_concepts_reference"], "brief")
    if brief["mission_requirement"] != "SPEC-MSN-%03d" % ordinal or brief["map_concepts_reference"] != "MapConcepts#%s" % mission:
        raise CompileError("production brief mismatch")
    terrain_path = terrain(root, doc["terrain_source_path"])
    terrain_doc, terrain_digest = load(terrain_path)
    if doc["terrain_source_sha256"] != terrain_digest: raise CompileError("terrain source SHA-256 mismatch")
    if terrain_doc.get("mission_code") != mission or terrain_doc.get("map_id") != map_id:
        raise CompileError("terrain identity mismatch")
    masks = variants(terrain_doc, terrain_digest, mission, map_id, operation)
    kinds = {name: (i, solid, footprint) for i, (name, solid, footprint) in enumerate(kind_specs)}
    records = doc["records"]
    if not isinstance(records, list) or not records or len(records) > 128: raise CompileError("records required and bounded")
    ids, occupied, parsed = set(), set(), []
    for position, record in enumerate(records):
        kind = record.get("kind") if isinstance(record, dict) else None
        footprint_spec = kinds.get(kind, (None, None, None))[2]
        exact(record, ["id", "kind", "x", "y", "yaw"] + (["footprint", "pivot"] if footprint_spec else []), "records[%d]" % position)
        identifier = string(record["id"], "id")
        if not ID.fullmatch(identifier): raise CompileError("landmark ID invalid")
        if identifier in ids: raise CompileError("duplicate landmark ID")
        ids.add(identifier)
        if kind not in kinds: raise CompileError("unknown landmark kind")
        kind_index, solid, footprint_spec = kinds[kind]
        x, y, yaw = integer(record["x"], "x"), integer(record["y"], "y"), integer(record["yaw"], "yaw")
        if not (0 <= x < W and 0 <= y < H) or yaw not in YAW: raise CompileError("record bounds or yaw invalid")
        footprint, pivot = (x, x, y, y), (0, 0)
        if footprint_spec:
            value = exact(record["footprint"], ["x0", "x1", "y0", "y1"], "footprint")
            footprint = tuple(integer(value[n], "footprint.%s" % n) for n in ("x0", "x1", "y0", "y1"))
            x0, x1, y0, y1 = footprint
            pv = exact(record["pivot"], ["x_half_tiles", "y_half_tiles"], "pivot")
            pivot = integer(pv["x_half_tiles"], "pivot.x_half_tiles"), integer(pv["y_half_tiles"], "pivot.y_half_tiles")
            if not (0 <= x0 <= x1 < W and 0 <= y0 <= y1 < H) or not (-127 <= pivot[0] <= 127 and -127 <= pivot[1] <= 127):
                raise CompileError("%s footprint or pivot bounds invalid" % kind)
            if not (x0 <= x <= x1 and y0 <= y <= y1): raise CompileError("%s anchor must lie within footprint" % kind)
            expected_footprint = ((3, 1) if yaw in (0, 180) else (1, 3)) \
                if footprint_spec == "rotated-3x1" else footprint_spec
            if (x1-x0+1, y1-y0+1) != expected_footprint:
                raise CompileError("%s footprint dimensions invalid" % kind)
            if footprint_spec != "rotated-3x1" and yaw != 0:
                raise CompileError("%s yaw must be zero" % kind)
            if 2*x+pivot[0] != x0+x1 or 2*y+pivot[1] != y0+y1:
                raise CompileError("%s pivot must identify footprint center" % kind)
        cells = {row*W+column for column in range(footprint[0], footprint[1]+1) for row in range(footprint[2], footprint[3]+1)}
        if occupied.intersection(cells): raise CompileError("duplicate landmark cell")
        occupied.update(cells)
        if any(any((cell in mask) != solid for cell in cells) for mask in masks):
            raise CompileError("record terrain mismatch across doctrine variants")
        parsed.append((identifier, kind_index, x, y, yaw, solid, *footprint, *pivot))
    return {"mission": mission, "ordinal": ordinal, "map_id": map_id, "operation": operation, "kinds": kind_specs,
            "digest": digest, "terrain_digest": terrain_digest, "records": parsed}

def render(packs):
    lines = ["// GENERATED FILE - do not edit by hand.", "#pragma once", "#include <array>", "#include <cstddef>", "#include <cstdint>", "#include <string_view>",
        "namespace echoes::world::mission_landmarks {",
        "struct Record { const char* id; std::uint8_t kind, x, y; std::uint16_t yaw; bool requires_blocked; std::uint8_t footprint_x0, footprint_x1, footprint_y0, footprint_y1; std::int8_t pivot_x_half_tiles, pivot_y_half_tiles; };",
        "struct Pack { std::string_view mission_code, map_id, operation_mode, terrain_source_sha256, source_sha256; std::uint8_t mission_ordinal; const Record* records; std::size_t record_count; const char* const* kind_names; std::size_t kind_count; };", ""]
    for pack in packs:
        ns = pack["mission"].lower()
        lines += ["namespace %s {" % ns, 'inline constexpr const char* kMapId = "%s";' % pack["map_id"],
            'inline constexpr const char* kOperationMode = "%s";' % pack["operation"],
            "inline constexpr std::uint8_t kMissionOrdinal = %d;" % pack["ordinal"],
            'inline constexpr const char* kTerrainSourceSha256 = "%s";' % pack["terrain_digest"],
            'inline constexpr const char* kSourceSha256 = "%s";' % pack["digest"],
            "inline constexpr std::array<const char*, %d> kKindNames{{%s}};" % (len(pack["kinds"]), ", ".join('"%s"' % k[0] for k in pack["kinds"])),
            "inline constexpr std::array<Record, %d> kRecords{{" % len(pack["records"])]
        lines += ['    Record{"%s", %d, %d, %d, %d, %s, %d, %d, %d, %d, %d, %d},' % (a,b,c,d,e,"true" if f else "false",g,h,i,j,k,l) for a,b,c,d,e,f,g,h,i,j,k,l in pack["records"]]
        lines += ["}};", "} // namespace %s" % ns, ""]
    lines += ["inline constexpr std::array<Pack, %d> kPacks{{" % len(packs)]
    for pack in packs:
        ns = pack["mission"].lower()
        lines.append('    {"%s", %s::kMapId, %s::kOperationMode, %s::kTerrainSourceSha256, %s::kSourceSha256, %s::kMissionOrdinal, %s::kRecords.data(), %s::kRecords.size(), %s::kKindNames.data(), %s::kKindNames.size()},' % (pack["mission"],ns,ns,ns,ns,ns,ns,ns,ns,ns))
    lines += ["}};", "inline constexpr const Pack* FindPack(std::uint8_t ordinal, std::string_view map_id) { for (const auto& pack : kPacks) if (pack.mission_ordinal == ordinal && pack.map_id == map_id) return &pack; return nullptr; }", "",
        "// Legacy M01 aliases preserve existing generated-header consumers.",
        "inline constexpr const char* kMapId = m01::kMapId;", "inline constexpr std::uint8_t kMissionOrdinal = m01::kMissionOrdinal;",
        "inline constexpr const char* kTerrainSourceSha256 = m01::kTerrainSourceSha256;", "inline constexpr const char* kSourceSha256 = m01::kSourceSha256;",
        "inline constexpr const auto& kRecords = m01::kRecords;"]
    m01 = next(pack for pack in packs if pack["mission"] == "M01")
    for index, (kind, _, _) in enumerate(m01["kinds"]):
        lines.append("inline constexpr std::uint32_t k%sCount = %d;" % (kind, sum(record[1] == index for record in m01["records"])))
    return ("\n".join(lines + ["} // namespace echoes::world::mission_landmarks", ""])).encode()

def sources_from_directory(root):
    directory = root / "Content/World/Source/Presentation"
    return [path.relative_to(root) for path in sorted(directory.glob("m*_landmarks_v1.json")) if PACK_FILE.fullmatch(path.name)]

def compile_many(root, source_paths, header):
    root = Path(root).resolve()
    output = confined(root, header, PurePosixPath("Content/World/Generated/Presentation"), "header", False)
    packs = [parse_pack(root, source) for source in source_paths]
    if not packs: raise CompileError("at least one landmark pack required")
    missions = [pack["mission"] for pack in packs]
    if len(set(missions)) != len(missions): raise CompileError("duplicate landmark mission pack")
    if "M01" not in missions: raise CompileError("M01 landmark pack required for legacy consumers")
    return {output: render(sorted(packs, key=lambda pack: pack["ordinal"]))}

def compile(root, source_path, header): return compile_many(root, [source_path], header)

def write(outputs):
    staged = []
    try:
        for path, contents in outputs.items():
            path.parent.mkdir(parents=True, exist_ok=True)
            fd, temporary = tempfile.mkstemp(dir=path.parent, prefix=".tmp-")
            os.write(fd, contents); os.close(fd); staged.append((path, Path(temporary)))
        for path, temporary in staged: os.replace(temporary, path)
    finally:
        for _, temporary in staged:
            if temporary.exists(): temporary.unlink()

def main(argv=None):
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", default=".")
    parser.add_argument("--source", action="append", help="repeat to compile a fixed pack set")
    parser.add_argument("--header", default="Content/World/Generated/Presentation/EchoesMissionLandmarks.h")
    mode = parser.add_mutually_exclusive_group(required=True); mode.add_argument("--write", action="store_true"); mode.add_argument("--check", action="store_true")
    args = parser.parse_args(argv); root = Path(args.root).resolve()
    sources = [Path(value) for value in args.source] if args.source else sources_from_directory(root)
    outputs = compile_many(root, sources, Path(args.header))
    if args.write: write(outputs); print("mission landmark packs written"); return 0
    stale = [str(path) for path, contents in outputs.items() if not path.is_file() or path.read_bytes() != contents]
    if stale: print("mission landmark packs stale: " + ", ".join(stale), file=sys.stderr); return 2
    print("mission landmark packs current"); return 0

if __name__ == "__main__":
    try: raise SystemExit(main())
    except CompileError as error:
        print("mission landmark packs refused: " + str(error), file=sys.stderr); raise SystemExit(2)
