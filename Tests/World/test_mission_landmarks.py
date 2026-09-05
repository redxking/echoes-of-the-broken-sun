#!/usr/bin/env python3
import hashlib, importlib.util, json, tempfile, unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent
spec = importlib.util.spec_from_file_location(
    "landmarks", ROOT / "Content/World/Tools/compile_mission_landmarks.py"
)
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)


def write(path, value):
    raw = json.dumps(value, sort_keys=True, separators=(",", ":")).encode()
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(raw)
    return hashlib.sha256(raw).hexdigest()


class Landmarks(unittest.TestCase):
    def setUp(self):
        self.t = tempfile.TemporaryDirectory()
        self.root = Path(self.t.name)
        self.terrain = self.root / "Content/World/Source/Campaign/m01.json"
        self.source = self.root / "Content/World/Source/Presentation/m01.json"
        self.header = (
            self.root / "Content/World/Generated/Presentation/EchoesMissionLandmarks.h"
        )
        op = {"id": "solid", "op": "block", "x0": 3, "x1": 3, "y0": 3, "y1": 3}
        variant = lambda doctrine: {
            "doctrine": doctrine,
            "terrain_region_ops": [],
            "expected_blocked_cell_count": 1,
        }
        self.map = {
            "source_format": "echoes-campaign-map-source",
            "source_version": 1,
            "author": "Angelis Pseftis",
            "mission_code": "M01",
            "map_id": "glass-scar-evacuation-margin",
            "operation_mode": "CampaignPrologue",
            "production_brief": {
                "mission_requirement": "SPEC-MSN-001",
                "map_concepts_reference": "MapConcepts#M01",
            },
            "grid": {
                "width_tiles": 64,
                "height_tiles": 64,
                "index_formula": "y*width+x",
                "coordinate_origin": "southwest",
            },
            "base_terrain": "open",
            "terrain_region_ops": [op],
            "founding_doctrine_variants": [
                variant("Harvest"),
                variant("Preserve"),
                variant("Reshape"),
            ],
            "required_clearance": [],
        }
        self.hash = write(self.terrain, self.map)
        self.doc = {
            "format": "echoes-mission-landmarks",
            "version": 1,
            "author": "Angelis Pseftis",
            "mission_code": "M01",
            "map_id": "glass-scar-evacuation-margin",
            "production_brief": {
                "mission_requirement": "SPEC-MSN-001",
                "map_concepts_reference": "MapConcepts#M01",
            },
            "terrain_source_path": "Content/World/Source/Campaign/m01.json",
            "terrain_source_sha256": self.hash,
            "records": [
                {"id": "cradle", "kind": "ArchiveCradle", "x": 3, "y": 3, "yaw": 0},
                {"id": "paving", "kind": "RoutePaving", "x": 4, "y": 3, "yaw": 90},
                {
                    "id": "apron",
                    "kind": "ArchiveApron",
                    "x": 7,
                    "y": 6,
                    "yaw": 0,
                    "footprint": {"x0": 5, "x1": 9, "y0": 5, "y1": 8},
                    "pivot": {"x_half_tiles": 0, "y_half_tiles": 1},
                },
            ],
        }
        write(self.source, self.doc)

    def tearDown(self):
        self.t.cleanup()

    def build(self):
        return mod.compile(
            self.root,
            self.source.relative_to(self.root),
            self.header.relative_to(self.root),
        )

    def test_deterministic_write_and_check(self):
        a = self.build()
        self.assertEqual(a, self.build())
        mod.write(a)
        self.assertEqual(
            mod.main(
                [
                    "--root",
                    str(self.root),
                    "--source",
                    str(self.source.relative_to(self.root)),
                    "--header",
                    str(self.header.relative_to(self.root)),
                    "--check",
                ]
            ),
            0,
        )
        self.assertIn(b"kArchiveCradleCount = 1", self.header.read_bytes())
        self.assertIn(b"kArchiveApronCount = 1", self.header.read_bytes())
        self.assertIn(b'"apron", 4, 7, 6, 0, false, 5, 9, 5, 8, 0, 1', self.header.read_bytes())

    def test_refuses_hash_duplicate_and_terrain_mismatch(self):
        self.doc["terrain_source_sha256"] = "0" * 64
        write(self.source, self.doc)
        with self.assertRaisesRegex(mod.CompileError, "SHA"):
            self.build()
        self.doc["terrain_source_sha256"] = self.hash
        self.doc["records"].append(
            {"id": "again", "kind": "ArchiveFrame", "x": 3, "y": 3, "yaw": 0}
        )
        write(self.source, self.doc)
        with self.assertRaisesRegex(mod.CompileError, "duplicate landmark cell"):
            self.build()
        self.doc["records"].pop()
        self.doc["records"][0]["x"] = 4
        write(self.source, self.doc)
        with self.assertRaisesRegex(mod.CompileError, "terrain mismatch"):
            self.build()

    def test_refuses_unsafe_identifier_duplicate_json_and_malformed_record(self):
        self.doc["records"][0]["id"] = 'bad"\nidentifier'
        write(self.source, self.doc)
        with self.assertRaisesRegex(mod.CompileError, "ID invalid"):
            self.build()
        self.doc["records"][0]["id"] = "cradle"
        self.doc["records"][0]["x"] = "3"
        write(self.source, self.doc)
        with self.assertRaisesRegex(mod.CompileError, "integer required"):
            self.build()
        raw = b'{"format":"a","format":"b"}'
        self.source.write_bytes(raw)
        with self.assertRaisesRegex(mod.CompileError, "duplicate JSON key"):
            self.build()

    def test_refuses_paths_symlinks_and_mode_conflict(self):
        with self.assertRaisesRegex(mod.CompileError, "absolute path refused"):
            mod.compile(self.root, self.source, self.header.relative_to(self.root))
        with self.assertRaisesRegex(mod.CompileError, "path must remain"):
            mod.compile(
                self.root,
                Path("Content/World/Source/Presentation/../Campaign/m01.json"),
                self.header.relative_to(self.root),
            )
        link = self.root / "Content/World/Source/Presentation/link.json"
        link.symlink_to(self.source)
        with self.assertRaisesRegex(mod.CompileError, "symlink refused"):
            mod.compile(
                self.root,
                link.relative_to(self.root),
                self.header.relative_to(self.root),
            )
        output_link = self.root / "Content/World/Generated/Presentation/link.h"
        output_link.parent.mkdir(parents=True)
        output_link.symlink_to(self.header)
        with self.assertRaisesRegex(mod.CompileError, "symlink refused"):
            mod.compile(
                self.root,
                self.source.relative_to(self.root),
                output_link.relative_to(self.root),
            )
        with self.assertRaises(SystemExit):
            mod.main(["--root", str(self.root), "--check", "--write"])

    def test_refuses_missing_variant_and_invalid_terrain(self):
        self.map["founding_doctrine_variants"].pop()
        self.hash = write(self.terrain, self.map)
        self.doc["terrain_source_sha256"] = self.hash
        write(self.source, self.doc)
        with self.assertRaisesRegex(mod.CompileError, "requires exactly Harvest"):
            self.build()
        self.map["founding_doctrine_variants"].append(
            {
                "doctrine": "Reshape",
                "terrain_region_ops": [],
                "expected_blocked_cell_count": 1,
            }
        )
        self.map["base_terrain"] = "lava"
        self.hash = write(self.terrain, self.map)
        self.doc["terrain_source_sha256"] = self.hash
        write(self.source, self.doc)
        with self.assertRaisesRegex(mod.CompileError, "base_terrain"):
            self.build()
        self.map["base_terrain"] = "open"
        self.map["terrain_region_ops"][0]["op"] = "paint"
        self.hash = write(self.terrain, self.map)
        self.doc["terrain_source_sha256"] = self.hash
        write(self.source, self.doc)
        with self.assertRaisesRegex(mod.CompileError, "expected block or open"):
            self.build()

    def test_refuses_malformed_or_blocked_apron_footprint(self):
        apron = self.doc["records"][2]
        apron["footprint"]["x0"] = 8
        apron["footprint"]["x1"] = 9
        write(self.source, self.doc)
        with self.assertRaisesRegex(mod.CompileError, "anchor must lie"):
            self.build()
        self.map["terrain_region_ops"].append(
            {"id": "blocked-apron", "op": "block", "x0": 40, "x1": 40, "y0": 40, "y1": 40}
        )
        for variant in self.map["founding_doctrine_variants"]:
            variant["expected_blocked_cell_count"] = 2
        self.hash = write(self.terrain, self.map)
        self.doc["terrain_source_sha256"] = self.hash
        apron["footprint"] = {"x0": 38, "x1": 42, "y0": 39, "y1": 42}
        apron["x"] = 40
        apron["y"] = 40
        apron["pivot"] = {"x_half_tiles": 0, "y_half_tiles": 1}
        write(self.source, self.doc)
        with self.assertRaisesRegex(mod.CompileError, "terrain mismatch"):
            self.build()

    def test_refuses_shared_recipe_dimension_pivot_and_yaw_drift(self):
        apron = self.doc["records"][2]
        apron["footprint"]["x1"] = 8
        write(self.source, self.doc)
        with self.assertRaisesRegex(mod.CompileError, "footprint dimensions"):
            self.build()
        apron["footprint"]["x1"] = 9
        apron["pivot"]["x_half_tiles"] = 1
        write(self.source, self.doc)
        with self.assertRaisesRegex(mod.CompileError, "pivot must identify"):
            self.build()
        apron["pivot"]["x_half_tiles"] = 0
        apron["yaw"] = 90
        write(self.source, self.doc)
        with self.assertRaisesRegex(mod.CompileError, "yaw must be zero"):
            self.build()

    def test_m02_pack_selects_its_own_kinds_and_refuses_drift(self):
        campaign = ROOT / "Content/World/Source/Campaign/m02_shivergrass-migration-basin_v1.json"
        presentation = ROOT / "Content/World/Source/Presentation/m02_migration_landmarks_v1.json"
        m02_terrain = self.root / "Content/World/Source/Campaign/m02.json"
        m02_source = self.root / "Content/World/Source/Presentation/m02.json"
        m02_terrain.parent.mkdir(parents=True, exist_ok=True)
        m02_terrain.write_bytes(campaign.read_bytes())
        m02_doc = json.loads(presentation.read_text())
        m02_doc["terrain_source_path"] = "Content/World/Source/Campaign/m02.json"
        m02_doc["terrain_source_sha256"] = hashlib.sha256(m02_terrain.read_bytes()).hexdigest()
        for record in m02_doc["records"]:
            if record["kind"] != "ObservationSill":
                continue
            if record["yaw"] in (0, 180):
                record["footprint"] = {"x0": record["x"] - 1, "x1": record["x"] + 1, "y0": record["y"], "y1": record["y"]}
            else:
                record["footprint"] = {"x0": record["x"], "x1": record["x"], "y0": record["y"] - 1, "y1": record["y"] + 1}
            record["pivot"] = {"x_half_tiles": 0, "y_half_tiles": 0}
        sill = next(record for record in m02_doc["records"] if record["kind"] == "ObservationSill")
        m02_doc["records"] = [sill]
        write(m02_source, m02_doc)
        outputs = mod.compile_many(
            self.root,
            [self.source.relative_to(self.root), m02_source.relative_to(self.root)],
            self.header.relative_to(self.root),
        )
        rendered = next(iter(outputs.values()))
        self.assertIn(b"namespace m02", rendered)
        self.assertIn(b'"ObservationSill", "RootingShoulder", "PassagePaving"', rendered)
        self.assertIn(b'{"M02", m02::kMapId', rendered)
        self.assertIn((
            'Record{"%s", 0, %d, %d, %d, true, %d, %d, %d, %d, 0, 0}' %
            (sill["id"], sill["x"], sill["y"], sill["yaw"],
             sill["footprint"]["x0"], sill["footprint"]["x1"],
             sill["footprint"]["y0"], sill["footprint"]["y1"])
        ).encode(), rendered)
        m02_doc["terrain_source_sha256"] = "0" * 64
        write(m02_source, m02_doc)
        with self.assertRaisesRegex(mod.CompileError, "SHA"):
            mod.compile_many(self.root, [self.source.relative_to(self.root), m02_source.relative_to(self.root)], self.header.relative_to(self.root))
        m02_doc["terrain_source_sha256"] = hashlib.sha256(m02_terrain.read_bytes()).hexdigest()
        m02_doc["records"][0]["x"] = 3  # open in every M02 doctrine; a sill may not mask it.
        m02_doc["records"][0]["footprint"] = {"x0": 2, "x1": 4, "y0": m02_doc["records"][0]["y"], "y1": m02_doc["records"][0]["y"]}
        write(m02_source, m02_doc)
        with self.assertRaisesRegex(mod.CompileError, "terrain mismatch"):
            mod.compile_many(self.root, [self.source.relative_to(self.root), m02_source.relative_to(self.root)], self.header.relative_to(self.root))

    def test_m02_sill_footprint_rotation_and_pivot_are_exact(self):
        campaign = ROOT / "Content/World/Source/Campaign/m02_shivergrass-migration-basin_v1.json"
        presentation = ROOT / "Content/World/Source/Presentation/m02_migration_landmarks_v1.json"
        terrain = self.root / "Content/World/Source/Campaign/m02.json"
        source = self.root / "Content/World/Source/Presentation/m02.json"
        terrain.parent.mkdir(parents=True, exist_ok=True)
        terrain.write_bytes(campaign.read_bytes())
        document = json.loads(presentation.read_text())
        document["terrain_source_path"] = "Content/World/Source/Campaign/m02.json"
        document["terrain_source_sha256"] = hashlib.sha256(terrain.read_bytes()).hexdigest()
        for record in document["records"]:
            if record["kind"] != "ObservationSill":
                continue
            if record["yaw"] in (0, 180):
                record["footprint"] = {"x0": record["x"] - 1, "x1": record["x"] + 1, "y0": record["y"], "y1": record["y"]}
            else:
                record["footprint"] = {"x0": record["x"], "x1": record["x"], "y0": record["y"] - 1, "y1": record["y"] + 1}
            record["pivot"] = {"x_half_tiles": 0, "y_half_tiles": 0}
        sill = next(record for record in document["records"] if record["kind"] == "ObservationSill")
        document["records"] = [sill]
        write(source, document)
        self.assertTrue(mod.compile_many(self.root, [self.source.relative_to(self.root), source.relative_to(self.root)], self.header.relative_to(self.root)))
        sill["footprint"] = {"x0": sill["x"], "x1": sill["x"], "y0": sill["y"], "y1": sill["y"] + 1}
        write(source, document)
        with self.assertRaisesRegex(mod.CompileError, "footprint dimensions"):
            mod.compile_many(self.root, [self.source.relative_to(self.root), source.relative_to(self.root)], self.header.relative_to(self.root))
        sill["footprint"] = {"x0": sill["x"] - 1, "x1": sill["x"] + 1, "y0": sill["y"], "y1": sill["y"]}
        sill["yaw"] = 0
        sill["pivot"] = {"x_half_tiles": 1, "y_half_tiles": 0}
        write(source, document)
        with self.assertRaisesRegex(mod.CompileError, "pivot must identify"):
            mod.compile_many(self.root, [self.source.relative_to(self.root), source.relative_to(self.root)], self.header.relative_to(self.root))
        sill["pivot"] = {"x_half_tiles": 0, "y_half_tiles": 0}
        write(source, document)
        self.assertTrue(mod.compile_many(self.root, [self.source.relative_to(self.root), source.relative_to(self.root)], self.header.relative_to(self.root)))


if __name__ == "__main__":
    unittest.main()
