#!/usr/bin/env python3
"""Adversarial checks for the authored Mission 01 narrative contract."""

from __future__ import annotations

import copy
import importlib.util
import json
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
VALIDATOR_PATH = ROOT / "Content/Narrative/Schema/validate_narrative.py"
SPEC = importlib.util.spec_from_file_location("validate_narrative", VALIDATOR_PATH)
assert SPEC is not None and SPEC.loader is not None
VALIDATOR = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(VALIDATOR)

CANON_PATH = ROOT / "Content/Narrative/Source/campaign_canon_continuity.json"
MISSION_PATH = ROOT / "Content/Narrative/Source/missions/m01_what_the_ledger_keeps.json"
CANON_SCHEMA_PATH = ROOT / "Content/Narrative/Schema/campaign_canon_continuity.schema.json"
MISSION_SCHEMA_PATH = ROOT / "Content/Narrative/Schema/mission_contract.schema.json"


class Mission01NarrativeContractTests(unittest.TestCase):
    def setUp(self) -> None:
        self.canon = VALIDATOR.load_json_document(CANON_PATH)
        self.mission = VALIDATOR.load_json_document(MISSION_PATH)

    def assert_invalid_canon(self, expected: str) -> None:
        with self.assertRaisesRegex(VALIDATOR.NarrativeValidationError, expected):
            VALIDATOR.validate_campaign_canon(self.canon)

    def assert_invalid_mission(self, expected: str) -> None:
        with self.assertRaisesRegex(VALIDATOR.NarrativeValidationError, expected):
            VALIDATOR.validate_mission_contract(self.mission, self.canon)

    def test_valid_source_tree_passes_with_explicit_counts(self) -> None:
        self.assertEqual(
            VALIDATOR.validate_source_tree(ROOT),
            {
                "missions": 15,
                "characters": 5,
                "factions": 3,
                "lines": 28,
                "branches": 3,
                "failures": 5,
                "results": 4,
                "shots": 4,
            },
        )

    def test_authorship_and_authority_are_exact(self) -> None:
        self.assertEqual(self.canon["metadata"]["author"], "Angelis Pseftis")
        self.assertEqual(self.canon["metadata"]["creator"], "Angelis Pseftis")
        self.assertEqual(self.mission["metadata"]["author"], "Angelis Pseftis")
        self.assertEqual(self.mission["metadata"]["creator"], "Angelis Pseftis")
        self.assertEqual(
            self.mission["metadata"]["rights_status"],
            "author_attributed_project_source",
        )
        self.assertEqual(
            self.mission["metadata"]["originality_review_status"],
            "human_review_required",
        )
        self.assertEqual(self.canon["metadata"]["authority_path"], "Docs/Archive/DevelopmentBible.md")
        self.assertEqual(self.mission["metadata"]["authority_path"], "Docs/Archive/DevelopmentBible.md")

    def test_canon_matrix_is_exact_m01_through_m15_and_terminal(self) -> None:
        self.assertEqual(
            [
                (item["sequence"], item["title"], item["mission_id"], item["operation_mode"])
                for item in self.canon["missions"]
            ],
            VALIDATOR.EXPECTED_MISSIONS,
        )
        self.assertEqual(self.canon["campaign"]["mission_count"], 15)
        self.assertEqual(self.canon["campaign"]["terminal_mission_id"], "TheBrokenSun")
        self.assertFalse(self.canon["campaign"]["mission_16_exists"])

    def test_mission_16_is_rejected(self) -> None:
        extra = copy.deepcopy(self.canon["missions"][-1])
        extra.update(
            {
                "sequence": 16,
                "title": "Unsupported Continuation",
                "mission_id": "UnsupportedContinuation",
                "operation_mode": "CampaignUnsupportedContinuation",
            }
        )
        self.canon["missions"].append(extra)
        self.assert_invalid_canon("Mission 16 is unsupported")

    def test_founding_and_lume_protocol_chains_are_exact(self) -> None:
        self.assertEqual(
            self.canon["campaign"]["founding_well_doctrine_missions"],
            [item[2] for item in VALIDATOR.EXPECTED_MISSIONS[:9]],
        )
        self.assertEqual(
            self.canon["campaign"]["lume_protocol_receipt_missions"],
            [item[2] for item in VALIDATOR.EXPECTED_MISSIONS[10:]],
        )
        self.assertEqual(
            self.canon["campaign"]["final_resolutions"],
            VALIDATOR.EXPECTED_RESOLUTIONS,
        )

    def test_character_relationship_projection_is_exact(self) -> None:
        actual = {
            character["id"]: (
                character["faction_id"],
                tuple(character["mission_ids"]),
            )
            for character in self.canon["characters"]
        }
        self.assertEqual(actual, VALIDATOR.EXPECTED_CHARACTER_RELATIONSHIPS)

    def test_character_faction_remap_to_valid_faction_is_rejected(self) -> None:
        self.canon["characters"][0]["faction_id"] = "kharuun_assemblies"
        self.assert_invalid_canon("exact_character_relationship")

    def test_character_mission_remap_to_valid_mission_is_rejected(self) -> None:
        self.canon["characters"][0]["mission_ids"][1] = "SevenAccountsOfRain"
        self.assert_invalid_canon("exact_character_relationship")

    def test_character_canon_prose_drift_is_rejected(self) -> None:
        for field in ("arc", "continuity_boundary"):
            with self.subTest(field=field):
                candidate = copy.deepcopy(self.canon)
                candidate["characters"][0][field] += " Unreviewed revision."
                with self.assertRaisesRegex(
                    VALIDATOR.NarrativeValidationError,
                    "reviewed canonical prose projection changed",
                ):
                    VALIDATOR.validate_campaign_canon(candidate)

    def test_world_and_faction_canon_prose_drift_is_rejected(self) -> None:
        mutations = [
            ("world", "crownfall_rule"),
            ("world", "future_well_rule"),
            ("faction", "canon"),
            ("faction", "voice_rule"),
            ("faction", "prohibited_reduction"),
        ]
        for record_type, field in mutations:
            with self.subTest(record_type=record_type, field=field):
                candidate = copy.deepcopy(self.canon)
                if record_type == "world":
                    candidate["world"][field] += " Unreviewed revision."
                else:
                    candidate["world"]["factions"][0][field] += " Unreviewed revision."
                with self.assertRaisesRegex(
                    VALIDATOR.NarrativeValidationError,
                    "reviewed canonical prose projection changed",
                ):
                    VALIDATOR.validate_campaign_canon(candidate)

    def test_mission_relationship_projection_is_exact(self) -> None:
        actual = {
            mission["mission_id"]: (
                mission["command_authority"],
                mission["command_faction"],
                tuple(mission["named_participants"]),
            )
            for mission in self.canon["missions"]
        }
        self.assertEqual(actual, VALIDATOR.EXPECTED_MISSION_RELATIONSHIPS)

    def test_mission_authority_remap_to_valid_character_is_rejected(self) -> None:
        self.canon["missions"][1]["command_authority"] = "mara_vey"
        self.assert_invalid_canon("exact_mission_relationship")

    def test_mission_faction_remap_to_valid_faction_is_rejected(self) -> None:
        self.canon["missions"][1]["command_faction"] = "meridian_compact"
        self.assert_invalid_canon("exact_mission_relationship")

    def test_mission_participant_remap_to_valid_character_is_rejected(self) -> None:
        self.canon["missions"][1]["named_participants"] = ["mara_vey"]
        self.assert_invalid_canon("exact_mission_relationship")

    def test_mission_canon_prose_drift_is_rejected(self) -> None:
        for field in ("continuity_input", "established_output", "prohibited_inferences"):
            with self.subTest(field=field):
                candidate = copy.deepcopy(self.canon)
                candidate["missions"][1][field][0] += " Unreviewed revision."
                with self.assertRaisesRegex(
                    VALIDATOR.NarrativeValidationError,
                    "reviewed canonical prose projection changed",
                ):
                    VALIDATOR.validate_campaign_canon(candidate)

    def test_unknown_fields_fail_closed(self) -> None:
        self.mission["unreviewed_runtime_path"] = "/Game/Narrative/M01"
        self.assert_invalid_mission("unknown fields: unreviewed_runtime_path")

    def test_duplicate_json_keys_are_rejected(self) -> None:
        with tempfile.TemporaryDirectory(prefix="echoes-narrative-duplicate-") as directory:
            path = Path(directory) / "duplicate.json"
            path.write_text('{"schema_version":1,"schema_version":2}', encoding="utf-8")
            with self.assertRaisesRegex(VALIDATOR.NarrativeValidationError, "duplicate JSON key"):
                VALIDATOR.load_json_document(path)

    def test_nonfinite_json_numbers_are_rejected(self) -> None:
        with tempfile.TemporaryDirectory(prefix="echoes-narrative-nonfinite-") as directory:
            path = Path(directory) / "nonfinite.json"
            path.write_text('{"editorial_target_seconds":NaN}', encoding="utf-8")
            with self.assertRaisesRegex(VALIDATOR.NarrativeValidationError, "non-finite JSON number"):
                VALIDATOR.load_json_document(path)

    def test_control_characters_in_json_keys_are_rejected(self) -> None:
        with tempfile.TemporaryDirectory(prefix="echoes-narrative-key-") as directory:
            path = Path(directory) / "control-key.json"
            path.write_text(json.dumps({"unsafe\u0007key": "value"}), encoding="utf-8")
            with self.assertRaisesRegex(VALIDATOR.NarrativeValidationError, "control characters"):
                VALIDATOR.load_json_document(path)

    def test_source_size_bound_fails_closed(self) -> None:
        with tempfile.TemporaryDirectory(prefix="echoes-narrative-size-") as directory:
            path = Path(directory) / "oversize.json"
            path.write_text("{" + (" " * VALIDATOR.MAX_SOURCE_BYTES) + "}", encoding="utf-8")
            with self.assertRaisesRegex(VALIDATOR.NarrativeValidationError, "outside"):
                VALIDATOR.load_json_document(path)

    def test_schema_version_mismatch_is_rejected(self) -> None:
        self.mission["schema_version"] = 2
        self.assert_invalid_mission("mission.schema_version")

    def test_runtime_identifiers_are_exact_and_source_only(self) -> None:
        binding = self.mission["runtime_binding"]
        self.assertEqual(binding["mission_id"], "WhatTheLedgerKeeps")
        self.assertEqual(binding["operation_mode"], "CampaignPrologue")
        self.assertEqual(binding["phases"], VALIDATOR.EXPECTED_PHASES)
        self.assertEqual(binding["well_choices"], VALIDATOR.EXPECTED_CHOICES)
        self.assertEqual(binding["campaign_commit_statuses"], VALIDATOR.EXPECTED_COMMIT_STATUSES)
        self.assertFalse(binding["runtime_consumed"])

    def test_runtime_consumed_cannot_be_claimed(self) -> None:
        self.mission["runtime_binding"]["runtime_consumed"] = True
        self.assert_invalid_mission("runtime_consumed")

    def test_m01_mechanical_contract_is_exact(self) -> None:
        canon = self.mission["canon"]
        self.assertEqual(canon["archive_carrier_role"], "meridian_scout")
        self.assertEqual(canon["recovery_tile"], {"x": 22, "y": 18})
        self.assertEqual(canon["evacuation_tile"], {"x": 6, "y": 17})
        self.assertTrue(canon["carrier_must_hold_recovery_during_well_commit"])
        self.assertFalse(canon["hostile_core_destruction_substitutes_for_evacuation"])

    def test_m01_canon_prose_drift_is_rejected(self) -> None:
        mutations = (
            ("purpose", None),
            ("canonical_facts", 0),
            ("prohibited_inferences", 0),
        )
        for field, index in mutations:
            with self.subTest(field=field):
                candidate = copy.deepcopy(self.mission)
                if index is None:
                    candidate["canon"][field] += " Unreviewed revision."
                else:
                    candidate["canon"][field][index] += " Unreviewed revision."
                with self.assertRaisesRegex(
                    VALIDATOR.NarrativeValidationError,
                    "reviewed canonical prose projection changed",
                ):
                    VALIDATOR.validate_mission_contract(candidate, self.canon)

    def test_speaker_projection_is_exact(self) -> None:
        actual = {
            speaker["id"]: {
                key: value
                for key, value in speaker.items()
                if key != "id"
            }
            for speaker in self.mission["speakers"]
        }
        self.assertEqual(actual, VALIDATOR.EXPECTED_SPEAKER_RECORDS)

    def test_speaker_identity_swap_is_rejected(self) -> None:
        first, second = self.mission["speakers"][:2]
        first["id"], second["id"] = second["id"], first["id"]
        first["command_authority"], second["command_authority"] = (
            second["command_authority"],
            first["command_authority"],
        )
        self.assert_invalid_mission("reviewed_speaker_mapping")

    def test_speaker_relationship_field_remaps_are_rejected(self) -> None:
        mutations = {
            "display_name": "Mara Venn",
            "faction_id": "kharuun_assemblies",
            "role_in_mission": "archive_recovery_requester",
            "delivery_channel": "operations_radio",
        }
        for field, replacement in mutations.items():
            with self.subTest(field=field):
                candidate = copy.deepcopy(self.mission)
                candidate["speakers"][0][field] = replacement
                with self.assertRaisesRegex(
                    VALIDATOR.NarrativeValidationError,
                    "reviewed_speaker_mapping",
                ):
                    VALIDATOR.validate_mission_contract(candidate, self.canon)

    def test_all_three_well_branches_have_equal_structure(self) -> None:
        branches = self.mission["branch_variants"]
        self.assertEqual(list(branches), VALIDATOR.EXPECTED_CHOICES)
        self.assertEqual({len(branch["dialogue_line_ids"]) for branch in branches.values()}, {3})
        self.assertNotIn("Dormant", branches)
        self.assertFalse(self.mission["canon"]["hidden_moral_score"])

    def test_dormant_terminal_branch_is_rejected(self) -> None:
        self.mission["branch_variants"]["Dormant"] = copy.deepcopy(
            self.mission["branch_variants"]["Harvest"]
        )
        self.assert_invalid_mission("unknown fields: Dormant")

    def test_limited_explicit_moral_ranking_lexeme_is_rejected(self) -> None:
        self.mission["branch_variants"]["Harvest"]["design_target_tradeoff"] = (
            "Target behavior says Harvest is the correct choice after the 180-tick commitment."
        )
        self.assert_invalid_mission("limited explicit moral-ranking lexical guard")

    def test_branch_editorial_assurance_scope_does_not_claim_semantic_detection(self) -> None:
        self.assertEqual(
            VALIDATOR.BRANCH_EDITORIAL_ASSURANCE_SCOPE,
            "exact_reviewed_projection_drift_detection_plus_limited_explicit_lexical_"
            "guard_with_mandatory_human_review",
        )

    def test_one_sided_branch_dialogue_rewrite_is_rejected_as_unreviewed_drift(self) -> None:
        line = next(
            line
            for line in self.mission["lines"]
            if line["id"] == "nar_m01_line_mara_harvest_001"
        )
        line["source_text"] = (
            "Harvest settles the matter. Preserve and Reshape would only indulge doubt."
        )
        self.assert_invalid_mission("reviewed_branch_lines")

    def test_branch_runtime_prose_drift_preserving_required_fragments_is_rejected(self) -> None:
        self.mission["branch_variants"]["Harvest"]["current_runtime_behavior"] = (
            "Current source immediately grants 500 Dawn and collapses the Well, "
            "demonstrating the careless option."
        )
        self.assert_invalid_mission("reviewed_branch_projection.current_runtime_behavior")

    def test_branch_target_prose_semantic_drift_is_rejected(self) -> None:
        self.mission["branch_variants"]["Preserve"]["design_target_tradeoff"] = (
            "Target behavior presents Preserve as the only responsible way forward."
        )
        self.assert_invalid_mission("reviewed_branch_projection.design_target_tradeoff")

    def test_branch_choice_ui_prose_drift_is_rejected(self) -> None:
        choice_ui = self.mission["branch_variants"]["Preserve"]["design_target_choice_ui"]
        choice_ui["source_text"] = (
            "Preserve: keep the Well intact and take the responsible path."
        )
        self.assert_invalid_mission("reviewed_branch_projection.design_target_choice_ui")

    def test_cross_branch_line_reassignment_is_rejected(self) -> None:
        branches = self.mission["branch_variants"]
        branches["Harvest"]["dialogue_line_ids"], branches["Reshape"]["dialogue_line_ids"] = (
            branches["Reshape"]["dialogue_line_ids"],
            branches["Harvest"]["dialogue_line_ids"],
        )
        self.assert_invalid_mission("reviewed_branch_projection.dialogue_line_ids")

    def test_branch_line_speaker_and_matching_channel_reassignment_is_rejected(self) -> None:
        line = next(
            line
            for line in self.mission["lines"]
            if line["id"] == "nar_m01_line_mara_harvest_001"
        )
        line["speaker_id"] = "spk_talar_venn"
        line["delivery_channel"] = "operations_radio"
        self.assert_invalid_mission("reviewed_branch_lines")

    def test_current_runtime_and_design_target_are_separate_and_exact(self) -> None:
        branches = self.mission["branch_variants"]
        for choice, branch in branches.items():
            self.assertTrue(branch["current_runtime_behavior"].startswith("Current source"))
            self.assertTrue(branch["design_target_tradeoff"].startswith("Target behavior"))
            self.assertEqual(
                branch["runtime_alignment"],
                VALIDATOR.EXPECTED_BRANCH_ALIGNMENT[choice],
            )
            self.assertIn("design_target_choice_ui", branch)
            self.assertNotIn("choice_ui", branch)
        self.assertNotIn("180-tick", branches["Harvest"]["current_runtime_behavior"])
        self.assertNotIn("180-tick", branches["Reshape"]["current_runtime_behavior"])
        self.assertNotIn("pre-expiry warning", branches["Reshape"]["current_runtime_behavior"])
        self.assertIn("180-tick", branches["Harvest"]["design_target_tradeoff"])
        self.assertIn("180-tick", branches["Reshape"]["design_target_tradeoff"])
        self.assertIn("pre-expiry warning", branches["Reshape"]["design_target_tradeoff"])

    def test_design_target_language_cannot_be_moved_into_current_runtime(self) -> None:
        self.mission["branch_variants"]["Reshape"]["current_runtime_behavior"] += (
            " A pre-expiry warning is shown."
        )
        self.assert_invalid_mission("design-target fragment")

    def test_unresolved_line_reference_is_rejected(self) -> None:
        self.mission["dialogue_sequences"][0]["line_ids"][0] = "nar_m01_line_missing"
        self.assert_invalid_mission("unresolved line reference")

    def test_trigger_prerequisite_order_is_exact(self) -> None:
        self.mission["triggers"][1]["prerequisite_ids"] = [
            "nar_m01_evt_well_decision_committed"
        ]
        self.assert_invalid_mission("prerequisite_ids")

    def test_line_cannot_belong_to_multiple_dialogue_sequences(self) -> None:
        opening_line = self.mission["dialogue_sequences"][0]["line_ids"][0]
        self.mission["dialogue_sequences"][1]["trigger_id"] = (
            "nar_m01_evt_operation_started"
        )
        self.mission["dialogue_sequences"][1]["line_ids"] = [opening_line]
        self.assert_invalid_mission("only one dialogue sequence or variant")

    def test_duplicate_line_identifier_is_rejected(self) -> None:
        self.mission["lines"][1]["id"] = self.mission["lines"][0]["id"]
        self.mission["lines"][1]["loc_key"] = self.mission["lines"][0]["id"]
        self.assert_invalid_mission("duplicate Mission 01 line identifier")

    def test_placeholder_declarations_must_match_source(self) -> None:
        self.mission["result_variants"][0]["copy"]["placeholders"] = {}
        self.assert_invalid_mission("do not match source placeholders")

    def test_text_budget_must_cover_source(self) -> None:
        self.mission["ui_copy"]["briefing"]["text_budget"]["max_characters"] = 5
        self.assert_invalid_mission("does not cover")

    def test_control_characters_are_rejected_at_load(self) -> None:
        with tempfile.TemporaryDirectory(prefix="echoes-narrative-control-") as directory:
            path = Path(directory) / "control.json"
            path.write_text(json.dumps({"source_text": "unsafe\u0007text"}), encoding="utf-8")
            with self.assertRaisesRegex(VALIDATOR.NarrativeValidationError, "control characters"):
                VALIDATOR.load_json_document(path)

    def test_concrete_runtime_asset_paths_are_rejected(self) -> None:
        self.mission["cinematic"]["shots"][0]["visual_direction"] = (
            "Load /Game/Narrative/M01/Opening.uasset."
        )
        self.assert_invalid_mission("concrete asset/path fragment")

    def test_concrete_paths_are_rejected_in_presentation_metadata(self) -> None:
        self.mission["cinematic"]["title"] = "/Game/Narrative/M01/Opening"
        self.assert_invalid_mission("concrete asset/path fragment")

    def test_failure_variants_cover_reducer_and_generic_fallback(self) -> None:
        failures = self.mission["failure_retry"]["failure_variants"]
        self.assertEqual(
            [failure["reason_code"] for failure in failures],
            VALIDATOR.EXPECTED_FAILURE_REASONS,
        )
        self.assertEqual(
            [failure["binding_status"] for failure in failures[:-1]],
            ["reason_code_requested"] * 4,
        )
        self.assertEqual(failures[-1]["binding_status"], "fallback_available")

    def test_failure_reason_omission_is_rejected(self) -> None:
        del self.mission["failure_retry"]["failure_variants"][2]
        self.assert_invalid_mission("exact four reducer failures plus generic fallback")

    def test_failure_line_cannot_be_assigned_to_multiple_reasons(self) -> None:
        shared = self.mission["failure_retry"]["failure_variants"][0][
            "dialogue_line_ids"
        ]
        self.mission["failure_retry"]["failure_variants"][1][
            "dialogue_line_ids"
        ] = list(shared)
        self.assert_invalid_mission("failure lines must be singly assigned")

    def test_result_variants_preserve_all_persistence_states(self) -> None:
        results = self.mission["result_variants"]
        self.assertEqual([result["status"] for result in results], VALIDATOR.EXPECTED_COMMIT_STATUSES)
        replay_text = next(
            result["copy"]["source_text"]
            for result in results
            if result["status"] == "ReplayConflict"
        )
        self.assertIn("campaign ledger remains {recorded_choice}", replay_text)
        storage_text = next(
            result["copy"]["source_text"]
            for result in results
            if result["status"] == "StorageFailure"
        )
        self.assertIn("was not saved", storage_text)

    def test_result_status_reordering_is_rejected(self) -> None:
        self.mission["result_variants"].reverse()
        self.assert_invalid_mission("result_variants.*status")

    def test_subtitle_and_transcript_are_projections_of_one_source(self) -> None:
        self.assertEqual(self.mission["projections"]["subtitle_source"], "lines[].source_text")
        self.assertEqual(self.mission["projections"]["transcript_source"], "lines[].source_text")
        self.assertEqual(
            self.mission["projections"]["duplication_policy"],
            "generated_from_canonical_source_text",
        )
        self.assertTrue(all(line["subtitle"]["enabled"] for line in self.mission["lines"]))
        self.assertTrue(all(line["transcript_included"] for line in self.mission["lines"]))

    def test_voice_and_cinematic_hooks_are_logical_and_absent(self) -> None:
        self.assertTrue(
            all(line["voice_hook"]["asset_status"] == "absent" for line in self.mission["lines"])
        )
        self.assertTrue(
            all(hook["asset_status"] == "absent" for hook in self.mission["asset_hooks"])
        )
        self.assertEqual(self.mission["cinematic"]["implementation_status"], "absent")
        self.assertFalse(self.mission["cinematic"]["named_character_physical_presence_asserted"])

    def test_runtime_path_field_on_absent_hook_is_rejected(self) -> None:
        self.mission["asset_hooks"][0]["runtime_path"] = "/Game/Narrative/M01"
        self.assert_invalid_mission("unknown fields: runtime_path")

    def test_asset_hook_inventory_is_exact(self) -> None:
        self.mission["asset_hooks"][3]["id"] = "vis_m01_unreviewed_extra"
        self.mission["cinematic"]["shots"][3]["visual_hook_ids"] = [
            "vis_m01_unreviewed_extra"
        ]
        self.assert_invalid_mission("logical inventory")

    def test_every_declared_asset_hook_is_referenced(self) -> None:
        self.mission["cinematic"]["shots"][3]["visual_hook_ids"] = [
            "vis_m01_glass_scar_overview"
        ]
        self.assert_invalid_mission("asset hook coverage")

    def test_every_authored_line_is_referenced(self) -> None:
        removed = self.mission["dialogue_sequences"][3]["line_ids"].pop()
        self.assertEqual(removed, "nar_m01_line_mara_005")
        self.assert_invalid_mission("unreferenced canonical lines")

    def test_implementation_axes_do_not_overclaim(self) -> None:
        self.assertEqual(
            self.mission["implementation"],
            {
                "mechanics": "implemented_current_source_with_known_presentation_gaps",
                "narrative_contract": "authored_source_only",
                "runtime_consumption": "unimplemented",
                "subtitles": "authored_unbound",
                "voice_assets": "absent",
                "cinematics": "absent",
                "localization_pipeline": "unimplemented",
                "well_telegraphs": "unimplemented",
                "reshape_expiry_warning": "unimplemented",
                "manual_observation": "not_run",
                "packaged_build": "not_run",
            },
        )

    def test_published_json_schemas_are_valid_and_accept_sources_when_available(self) -> None:
        try:
            import jsonschema
        except ImportError:
            self.skipTest("optional jsonschema package is unavailable; dependency-free validator still ran")
        canon_schema = VALIDATOR.load_json_document(CANON_SCHEMA_PATH)
        mission_schema = VALIDATOR.load_json_document(MISSION_SCHEMA_PATH)
        jsonschema.Draft202012Validator.check_schema(canon_schema)
        jsonschema.Draft202012Validator.check_schema(mission_schema)
        jsonschema.Draft202012Validator(canon_schema).validate(self.canon)
        jsonschema.Draft202012Validator(mission_schema).validate(self.mission)
        extra_hook_mission = copy.deepcopy(self.mission)
        extra_hook_mission["asset_hooks"].append(
            copy.deepcopy(extra_hook_mission["asset_hooks"][0])
        )
        with self.assertRaises(jsonschema.ValidationError):
            jsonschema.Draft202012Validator(mission_schema).validate(extra_hook_mission)


if __name__ == "__main__":
    unittest.main()
