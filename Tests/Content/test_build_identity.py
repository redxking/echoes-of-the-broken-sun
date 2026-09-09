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
        snapshot_match = re.search(
            r"kSnapshotVersion\s*=\s*(\d+)\s*;", snapshot_header
        )
        self.assertIsNotNone(snapshot_match)
        snapshot_version = snapshot_match.group(1)
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


if __name__ == "__main__":
    unittest.main(verbosity=2)
