#!/usr/bin/env python3

from __future__ import annotations

import configparser
import hashlib
import pathlib
import re
import unittest


PROJECT_ROOT = pathlib.Path(__file__).resolve().parents[2]
CONFIG_PATH = PROJECT_ROOT / "Config" / "DefaultGame.ini"
NETWORK_SOURCE = (
    PROJECT_ROOT
    / "Source"
    / "EchoesOfTheBrokenSun"
    / "Private"
    / "EchoesNetworkSession.cpp"
)
SIMULATION_HEADER = (
    PROJECT_ROOT
    / "Source"
    / "EchoesSimCore"
    / "Public"
    / "EchoesSimCore"
    / "Simulation.h"
)


def resolve_constant(source: str, name: str) -> str | None:
    """Read an integer constant that may be declared as a literal or as a
    one-level alias of another constant in the same header."""
    for _ in range(2):
        match = re.search(rf"{re.escape(name)}\s*=\s*([A-Za-z_][A-Za-z0-9_]*|\d+)\s*;", source)
        if match is None:
            return None
        value = match.group(1)
        if value.isdigit():
            return value
        name = value
    return None


class BuildIdentityTests(unittest.TestCase):
    def test_build_identity_tracks_product_and_snapshot_versions(self) -> None:
        config = configparser.ConfigParser(strict=False)
        config.optionxform = str
        config.read(CONFIG_PATH, encoding="utf-8")
        project_version = config["/Script/EngineSettings.GeneralProjectSettings"][
            "ProjectVersion"
        ]

        network_source = NETWORK_SOURCE.read_text(encoding="utf-8")
        material_match = re.search(
            r'BuildIdentityMaterial\s*=\s*\n?\s*"([^"]+)";', network_source
        )
        self.assertIsNotNone(material_match)
        material = material_match.group(1)

        snapshot_header = SIMULATION_HEADER.read_text(encoding="utf-8")
        # The schema constants are written as a named era constant plus an
        # alias (kReplayVersion has been an alias for some time, and
        # kSnapshotVersion became one at schema 32). Resolve one level of
        # aliasing rather than requiring the declaration to be a bare literal:
        # naming the era is the more readable form and the test should follow
        # the source, not the source bend to suit a regex.
        snapshot_version = resolve_constant(snapshot_header, "kSnapshotVersion")
        self.assertIsNotNone(snapshot_version)
        protocol_header = SIMULATION_HEADER.with_name("NetworkProtocol.h").read_text(encoding="utf-8")
        protocol_match = re.search(r"kProtocolVersion\s*=\s*(\d+)\s*;", protocol_header)
        self.assertIsNotNone(protocol_match)
        protocol_version = protocol_match.group(1)
        view_match = re.search(r"kPlayerViewSchemaVersion\s*=\s*(\d+)\s*;", protocol_header)
        self.assertIsNotNone(view_match)
        view_version = view_match.group(1)
        self.assertEqual(
            material,
            f"EchoesOfTheBrokenSun:{project_version}:protocol-{protocol_version}:"
            f"snapshot-{snapshot_version}:view-{view_version}",
        )

        digest_match = re.search(
            r"constexpr sim::net::Digest256 BuildId\{([^}]+)\};",
            network_source,
            re.DOTALL,
        )
        self.assertIsNotNone(digest_match)
        digest_bytes = bytes(
            int(value, 16)
            for value in re.findall(r"0x([0-9a-fA-F]{2})", digest_match.group(1))
        )
        self.assertEqual(len(digest_bytes), 32)
        self.assertEqual(digest_bytes, hashlib.sha256(material.encode()).digest())

    def test_canonical_rules_pack_tracks_the_generated_content_pack(self) -> None:
        # CanonicalRulesPack is the compatibility fallback for a session with no
        # live simulation, so a stale value negotiates compatibility against a
        # content pack that no longer exists. Unlike BuildId nothing bound it,
        # meaning anyone editing Content/Data/Source and regenerating would
        # leave it silently wrong. Found by the strategy-validation lane while
        # fixing the build identity, which is the same failure this catches.
        network_source = NETWORK_SOURCE.read_text(encoding="utf-8")
        pack_match = re.search(
            r"constexpr sim::net::Digest256 CanonicalRulesPack\{([^}]+)\};",
            network_source,
            re.DOTALL,
        )
        self.assertIsNotNone(pack_match)
        pack_bytes = bytes(
            int(value, 16)
            for value in re.findall(r"0x([0-9a-fA-F]{2})", pack_match.group(1))
        )
        self.assertEqual(len(pack_bytes), 32)
        generated = (
            PROJECT_ROOT / "Content" / "Data" / "Generated" / "EchoesContentPack.json.sha256"
        )
        self.assertTrue(generated.is_file())
        self.assertEqual(pack_bytes.hex(), generated.read_text(encoding="utf-8").strip())


if __name__ == "__main__":
    unittest.main(verbosity=2)
