#!/usr/bin/env python3
"""Fail-closed checks for M01 retained voice-candidate preparation."""

from __future__ import annotations

import copy
import importlib.util
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SCRIPT_PATH = ROOT / "Scripts/prepare_m01_voice_bindings.py"
SPEC = importlib.util.spec_from_file_location("prepare_m01_voice_bindings", SCRIPT_PATH)
assert SPEC is not None and SPEC.loader is not None
BINDINGS = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(BINDINGS)

CANDIDATE_ROOT = ROOT / BINDINGS.CANDIDATE_RELATIVE
CANDIDATE_MANIFEST_PATH = CANDIDATE_ROOT / "manifest.json"
PROFILE_PATH = ROOT / "Content/Audio/VoiceDirection/character_voice_profiles.json"


class M01VoiceBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.candidate_manifest = BINDINGS.load_json(CANDIDATE_MANIFEST_PATH)
        cls.valid_manifest = BINDINGS.build_manifest(ROOT, CANDIDATE_ROOT)

    def candidate_with(self, field: str, value: object) -> dict:
        candidate = copy.deepcopy(self.candidate_manifest)
        candidate["lines"][0][field] = value
        return candidate

    def test_manifest_schema_matches_runtime_contract_and_current_signals(self) -> None:
        manifest = self.valid_manifest
        self.assertEqual(manifest["schema_version"], 1)
        self.assertEqual(manifest["author"], "Angelis Pseftis")
        self.assertEqual(len(manifest["lines"]), 28)
        self.assertEqual(
            len({line["asset_path"] for line in manifest["lines"]}), 28
        )
        self.assertTrue(
            all(
                line["asset_path"]
                == f"/Game/Audio/Voice/{line['voice_hook']}.{line['voice_hook']}"
                for line in manifest["lines"]
            )
        )
        # Validation against current source/pack without prepared copies proves
        # the manifest tuple and authority hashes before --write is invoked.
        BINDINGS.validate_prepared_manifest(
            manifest, ROOT, verify_prepared_wavs=False
        )

    def test_candidate_wav_hash_mismatch_is_rejected(self) -> None:
        candidate = self.candidate_with("sha256_file", "0" * 64)
        with self.assertRaisesRegex(BINDINGS.VoiceBindingError, "WAV SHA-256 mismatch"):
            BINDINGS.build_manifest(ROOT, CANDIDATE_ROOT, candidate)

    def test_candidate_text_mismatch_is_rejected(self) -> None:
        candidate = self.candidate_with("text", "Stale candidate text.")
        with self.assertRaisesRegex(BINDINGS.VoiceBindingError, "candidate text differs"):
            BINDINGS.build_manifest(ROOT, CANDIDATE_ROOT, candidate)

    def test_candidate_speaker_mismatch_is_rejected(self) -> None:
        candidate = self.candidate_with("speaker", "Mara Vey")
        with self.assertRaisesRegex(BINDINGS.VoiceBindingError, "candidate speaker differs"):
            BINDINGS.build_manifest(ROOT, CANDIDATE_ROOT, candidate)

    def test_authoritative_source_hash_mismatch_is_rejected(self) -> None:
        manifest = copy.deepcopy(self.valid_manifest)
        manifest["source_sha256"] = "0" * 64
        with self.assertRaisesRegex(BINDINGS.VoiceBindingError, "authoritative SHA-256 mismatch"):
            BINDINGS.validate_prepared_manifest(
                manifest, ROOT, verify_prepared_wavs=False
            )

    def test_opening_shots_cover_current_voice_and_subtitle_lane(self) -> None:
        source = BINDINGS.load_json(ROOT / "Content/Narrative/Source/missions/m01_what_the_ledger_keeps.json")
        lines = {row["line_id"]: row for row in self.valid_manifest["lines"]}
        for shot in source["cinematic"]["shots"]:
            duration = sum(max(min(9.0, max(3.0, 2.4 + 0.045 * len(lines[line_id]["text"]))),
                               lines[line_id]["duration_seconds"]) + 0.1
                           for line_id in shot["line_ids"])
            self.assertGreaterEqual(shot["editorial_target_seconds"], duration, shot["id"])
        self.assertLessEqual(sum(shot["editorial_target_seconds"] for shot in source["cinematic"]["shots"]), 90)

    def test_approved_voice_and_ducking_pins_are_exact(self) -> None:
        profiles = BINDINGS.load_json(PROFILE_PATH)
        expected_voices = {
            "spk_mara_vey": ("af_sarah", 1.0),
            "spk_talar_venn": ("am_michael", 1.0),
            "spk_oruun": ("bm_george", 0.92),
        }
        actual = {
            character["speaker_id"]: (
                character["tts_binding"]["kokoro_voice_id"],
                character["tts_binding"]["rate_adjustment"],
            )
            for character in profiles["characters"]
            if character["speaker_id"] in expected_voices
        }
        self.assertEqual(actual, expected_voices)
        ducking = {
            item["category"]: (
                item["attenuation_db"],
                item["attack_ms"],
                item["hold_ms"],
                item["release_ms"],
            )
            for item in profiles["submix_ducking_policy"]["ducked_categories"]
        }
        self.assertEqual(
            ducking,
            {"Music": (-6.0, 150, 0, 500), "Ambience": (-4.0, 150, 0, 500)},
        )


if __name__ == "__main__":
    unittest.main()
