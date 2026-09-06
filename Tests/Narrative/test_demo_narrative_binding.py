#!/usr/bin/env python3
"""Focused fail-closed checks for demo narrative runtime-binding claims."""

from __future__ import annotations

import copy
import importlib.util
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
VALIDATOR_PATH = ROOT / "Content/Narrative/Schema/validate_narrative.py"
SPEC = importlib.util.spec_from_file_location("validate_narrative_demo", VALIDATOR_PATH)
assert SPEC is not None and SPEC.loader is not None
VALIDATOR = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(VALIDATOR)

DEMO_DIR = ROOT / "Content/Narrative/Source/demo"


class DemoNarrativeBindingTests(unittest.TestCase):
    def setUp(self) -> None:
        self.tutorial = VALIDATOR.load_json_document(
            DEMO_DIR / "tutorial_readiness_check.json"
        )
        self.entry = VALIDATOR.DEMO_CONTRACT_REGISTRY["tutorial_readiness_check.json"]

    def validate_tutorial(self, value: dict) -> None:
        VALIDATOR.validate_demo_contract(value, self.entry, None, ROOT)

    def test_current_tutorial_partial_binding_is_valid(self) -> None:
        self.validate_tutorial(self.tutorial)

    def test_authoritative_source_digest_drift_is_rejected(self) -> None:
        candidate = copy.deepcopy(self.tutorial)
        candidate["metadata"]["source_document_sha256"] = "0" * 64
        with self.assertRaisesRegex(
            VALIDATOR.NarrativeValidationError,
            "demo.metadata.source_document_sha256",
        ):
            self.validate_tutorial(candidate)

    def test_trigger_outside_runtime_allowlist_is_rejected(self) -> None:
        candidate = copy.deepcopy(self.tutorial)
        hint = next(
            trigger
            for trigger in candidate["triggers"]
            if trigger["id"] == "nar_demo_evt_tut_survey_hint"
        )
        hint["binding_status"] = "runtime_signal_bound"
        with self.assertRaisesRegex(
            VALIDATOR.NarrativeValidationError,
            "demo.triggers.*binding_status",
        ):
            self.validate_tutorial(candidate)

    def test_resolved_binding_token_line_must_claim_runtime_subtitle_binding(self) -> None:
        candidate = copy.deepcopy(self.tutorial)
        resolved_token_line = next(
            line
            for line in candidate["lines"]
            if line["id"] == "nar_demo_line_tut_survey_02"
        )
        resolved_token_line["binding_status"] = "authored_unbound"
        with self.assertRaisesRegex(
            VALIDATOR.NarrativeValidationError,
            "demo.lines.*binding_status",
        ):
            self.validate_tutorial(candidate)


if __name__ == "__main__":
    unittest.main()
