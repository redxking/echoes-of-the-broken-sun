#!/usr/bin/env python3
"""Fail-closed validation for authored Echoes narrative source.

This validator intentionally has no runtime or third-party dependency. It checks
the structured projection of the authoritative Development Bible and the first
authored Mission 01 presentation contract. Passing validation does not mean the
runtime consumes the contract or that voice, localization, or cinematic assets
exist.
"""

from __future__ import annotations

import argparse
import json
import math
import re
from pathlib import Path
from typing import Any, Iterable


MAX_SOURCE_BYTES = 256_000
NARRATIVE_ID = re.compile(r"^nar_[a-z0-9_]+$")
SPEAKER_ID = re.compile(r"^spk_[a-z0-9_]+$")
ASSET_ID = re.compile(r"^(?:aud|vis)_m01_[a-z0-9_]+$")
PLACEHOLDER = re.compile(r"\{([a-z][a-z0-9_]*)\}")

EXPECTED_CHOICES = ["Harvest", "Preserve", "Reshape"]
EXPECTED_PHASES = [
    "Inactive",
    "RecoverArchive",
    "DecideFutureWell",
    "Withdraw",
    "Complete",
    "Failed",
]
EXPECTED_COMMIT_STATUSES = [
    "Added",
    "AlreadyRecorded",
    "ReplayConflict",
    "StorageFailure",
]
EXPECTED_FAILURE_REASONS = [
    "local_core_lost",
    "archive_carrier_lost",
    "future_well_lost",
    "terminal_match_outcome",
    "generic",
]
EXPECTED_RESOLUTIONS = [
    "Restoration",
    "ControlledStabilization",
    "Extinguishment",
    "OpenEvolution",
]
EXPECTED_MISSIONS = [
    (1, "What the Ledger Keeps", "WhatTheLedgerKeeps", "CampaignPrologue"),
    (2, "Seven Accounts of Rain", "SevenAccountsOfRain", "CampaignSevenAccounts"),
    (3, "A City on Reserve", "ACityOnReserve", "CampaignCityReserve"),
    (4, "The Unburied Road", "TheUnburiedRoad", "CampaignUnburiedRoad"),
    (5, "Terms of Continuance", "TermsOfContinuance", "CampaignTermsOfContinuance"),
    (6, "Names Without Births", "NamesWithoutBirths", "CampaignNamesWithoutBirths"),
    (7, "The Shape of Silence", "TheShapeOfSilence", "CampaignShapeOfSilence"),
    (8, "The Shape Beside Us", "TheShapeBesideUs", "CampaignShapeBesideUs"),
    (9, "Reserve Authority", "ReserveAuthority", "CampaignReserveAuthority"),
    (10, "The Choir at Lume Reach", "ChoirAtLumeReach", "CampaignChoirAtLumeReach"),
    (11, "No Neutral Ledger", "NoNeutralLedger", "CampaignNoNeutralLedger"),
    (12, "The Future That Won", "TheFutureThatWon", "CampaignFutureThatWon"),
    (13, "Assembly of the Missing", "AssemblyOfTheMissing", "CampaignAssemblyOfTheMissing"),
    (14, "Several Voices, One Command", "SeveralVoicesOneCommand", "CampaignSeveralVoicesOneCommand"),
    (15, "The Broken Sun", "TheBrokenSun", "CampaignTheBrokenSun"),
]
EXPECTED_CHARACTER_IDS = {
    "mara_vey",
    "oruun_of_seven_stones",
    "talar_venn",
    "neme",
    "cael_rhyse",
}
EXPECTED_FACTIONS = {
    "meridian_compact",
    "kharuun_assemblies",
    "hollow_choir",
}
EXPECTED_TRIGGER_SIGNALS = {
    "nar_m01_evt_operation_started": "operation_ready:CampaignPrologue:RecoverArchive",
    "nar_m01_evt_archive_recovered": "phase_entered:DecideFutureWell",
    "nar_m01_evt_well_decision_committed": "phase_entered:Withdraw",
    "nar_m01_evt_withdrawal_complete": "phase_entered:Complete",
    "nar_m01_evt_mission_failed": "phase_entered:Failed",
    "nar_m01_evt_campaign_result_presented": "campaign_commit_status_presented",
    "nar_m01_evt_retry_requested": "player_requested_mission_retry",
}
EXPECTED_TRIGGER_PREREQUISITES = {
    "nar_m01_evt_operation_started": [],
    "nar_m01_evt_archive_recovered": ["nar_m01_evt_operation_started"],
    "nar_m01_evt_well_decision_committed": ["nar_m01_evt_archive_recovered"],
    "nar_m01_evt_withdrawal_complete": ["nar_m01_evt_well_decision_committed"],
    "nar_m01_evt_mission_failed": ["nar_m01_evt_operation_started"],
    "nar_m01_evt_campaign_result_presented": ["nar_m01_evt_withdrawal_complete"],
    "nar_m01_evt_retry_requested": ["nar_m01_evt_mission_failed"],
}
EXPECTED_ASSET_HOOKS = {
    "vis_m01_glass_scar_overview": "visual",
    "vis_m01_archive_route_overlay": "visual",
    "vis_m01_future_well_propagation": "visual",
    "vis_m01_tactical_handoff": "visual",
    "aud_m01_ambience_glass_scar": "audio",
}
EXPECTED_BRANCH_ALIGNMENT = {
    "Harvest": "requires_telegraph_before_binding",
    "Preserve": "bounded_values_aligned_runtime_consumption_unimplemented",
    "Reshape": "requires_telegraph_and_expiry_warning_before_binding",
}
EXPECTED_BRANCH_RUNTIME_FRAGMENTS = {
    "Harvest": ("Current source", "immediately", "500 Dawn", "collapses"),
    "Preserve": ("Current source", "15 Dawn", "300 ticks", "1,400 cm"),
    "Reshape": (
        "Current source",
        "immediately",
        "120 Dawn",
        "1,800 ticks",
        "deterministic fallback",
    ),
}
EXPECTED_BRANCH_TARGET_FRAGMENTS = {
    "Harvest": ("Target behavior", "180-tick"),
    "Preserve": ("Target behavior",),
    "Reshape": ("Target behavior", "180-tick", "pre-expiry warning"),
}
FORBIDDEN_CURRENT_RUNTIME_FRAGMENTS = {
    "Harvest": ("180-tick",),
    "Preserve": (),
    "Reshape": ("180-tick", "pre-expiry warning"),
}
FORBIDDEN_PRESENTATION_FRAGMENTS = (
    "/Game/",
    ".uasset",
    ".umap",
    "file://",
    "javascript:",
    "<script",
    "$(",
    "`",
)
MORAL_RANKING_FRAGMENTS = (
    "good choice",
    "bad choice",
    "correct choice",
    "best ending",
    "moral score",
    "canonical choice",
)


class NarrativeValidationError(ValueError):
    """Raised when narrative source violates its closed contract."""


class _DuplicateKeyError(ValueError):
    pass


def _reject_nonfinite_constant(value: str) -> None:
    raise ValueError(f"non-finite JSON number is not permitted: {value}")


def _without_duplicate_keys(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        if key in result:
            raise _DuplicateKeyError(f"duplicate JSON key: {key}")
        result[key] = value
    return result


def load_json_document(path: Path) -> dict[str, Any]:
    """Load one bounded UTF-8 JSON object while rejecting duplicate keys."""
    try:
        size = path.stat().st_size
    except OSError as exc:
        raise NarrativeValidationError(f"{path}: cannot stat source: {exc}") from exc
    if size <= 0 or size > MAX_SOURCE_BYTES:
        raise NarrativeValidationError(
            f"{path}: source size {size} is outside 1..{MAX_SOURCE_BYTES} bytes"
        )
    try:
        text = path.read_text(encoding="utf-8")
    except (OSError, UnicodeError) as exc:
        raise NarrativeValidationError(f"{path}: cannot read UTF-8 source: {exc}") from exc
    if text.startswith("\ufeff"):
        raise NarrativeValidationError(f"{path}: UTF-8 BOM is not permitted")
    try:
        value = json.loads(
            text,
            object_pairs_hook=_without_duplicate_keys,
            parse_constant=_reject_nonfinite_constant,
        )
    except (_DuplicateKeyError, json.JSONDecodeError, ValueError) as exc:
        raise NarrativeValidationError(f"{path}: invalid JSON: {exc}") from exc
    if not isinstance(value, dict):
        raise NarrativeValidationError(f"{path}: top-level value must be an object")
    _validate_all_strings(value, str(path))
    return value


def _validate_all_strings(value: Any, path: str) -> None:
    if isinstance(value, str):
        if any(ord(character) < 32 or ord(character) == 127 for character in value):
            raise NarrativeValidationError(f"{path}: control characters are not permitted")
        if len(value) > 2_000:
            raise NarrativeValidationError(f"{path}: string exceeds the 2,000-character bound")
        folded = value.casefold()
        for fragment in FORBIDDEN_PRESENTATION_FRAGMENTS:
            if fragment.casefold() in folded:
                raise NarrativeValidationError(
                    f"{path}: executable or concrete asset/path fragment {fragment!r} is not permitted"
                )
        return
    if isinstance(value, list):
        for index, item in enumerate(value):
            _validate_all_strings(item, f"{path}[{index}]")
        return
    if isinstance(value, dict):
        for key, item in value.items():
            _validate_all_strings(key, f"{path}.<key>")
            _validate_all_strings(item, f"{path}.{key}")


def _expect_object(value: Any, path: str) -> dict[str, Any]:
    if not isinstance(value, dict):
        raise NarrativeValidationError(f"{path}: expected object")
    return value


def _expect_list(value: Any, path: str) -> list[Any]:
    if not isinstance(value, list):
        raise NarrativeValidationError(f"{path}: expected array")
    return value


def _expect_string(value: Any, path: str) -> str:
    if not isinstance(value, str) or not value:
        raise NarrativeValidationError(f"{path}: expected nonempty string")
    return value


def _expect_symbol(value: Any, path: str) -> str:
    symbol = _expect_string(value, path)
    if re.fullmatch(r"[a-z][a-z0-9_]*", symbol) is None:
        raise NarrativeValidationError(f"{path}: expected lowercase stable symbol")
    return symbol


def _expect_bool(value: Any, path: str) -> bool:
    if not isinstance(value, bool):
        raise NarrativeValidationError(f"{path}: expected boolean")
    return value


def _expect_int(value: Any, path: str) -> int:
    if isinstance(value, bool) or not isinstance(value, int):
        raise NarrativeValidationError(f"{path}: expected integer")
    return value


def _exact_keys(value: Any, expected: Iterable[str], path: str) -> dict[str, Any]:
    obj = _expect_object(value, path)
    expected_set = set(expected)
    actual = set(obj)
    missing = sorted(expected_set - actual)
    unknown = sorted(actual - expected_set)
    if missing:
        raise NarrativeValidationError(f"{path}: missing required fields: {', '.join(missing)}")
    if unknown:
        raise NarrativeValidationError(f"{path}: unknown fields: {', '.join(unknown)}")
    return obj


def _expect_exact(value: Any, expected: Any, path: str) -> None:
    if value != expected:
        raise NarrativeValidationError(f"{path}: expected {expected!r}, received {value!r}")


def _expect_unique_strings(value: Any, path: str, *, minimum: int = 0) -> list[str]:
    items = _expect_list(value, path)
    if len(items) < minimum:
        raise NarrativeValidationError(f"{path}: expected at least {minimum} items")
    strings = [_expect_string(item, f"{path}[{index}]") for index, item in enumerate(items)]
    if len(strings) != len(set(strings)):
        raise NarrativeValidationError(f"{path}: duplicate values are not permitted")
    return strings


def _validate_narrative_id(value: Any, path: str) -> str:
    identifier = _expect_string(value, path)
    if NARRATIVE_ID.fullmatch(identifier) is None:
        raise NarrativeValidationError(f"{path}: invalid narrative identifier {identifier!r}")
    return identifier


def _validate_source_text(value: Any, path: str) -> str:
    text = _expect_string(value, path)
    folded = text.casefold()
    for fragment in FORBIDDEN_PRESENTATION_FRAGMENTS:
        if fragment.casefold() in folded:
            raise NarrativeValidationError(
                f"{path}: executable or concrete asset/path fragment {fragment!r} is not permitted"
            )
    return text


def _validate_text_budget(value: Any, source_text: str, path: str) -> None:
    budget = _exact_keys(
        value,
        {"max_characters", "localization_expansion_percent"},
        path,
    )
    maximum = _expect_int(budget["max_characters"], f"{path}.max_characters")
    expansion = _expect_int(
        budget["localization_expansion_percent"],
        f"{path}.localization_expansion_percent",
    )
    if maximum < len(source_text) or maximum > 320:
        raise NarrativeValidationError(
            f"{path}.max_characters: {maximum} does not cover {len(source_text)} characters within the 320-character ceiling"
        )
    if expansion < 20 or expansion > 50:
        raise NarrativeValidationError(
            f"{path}.localization_expansion_percent: expected 20..50"
        )


def _validate_placeholders(value: Any, source_text: str, path: str) -> None:
    placeholders = _expect_object(value, path)
    for key, description in placeholders.items():
        if re.fullmatch(r"[a-z][a-z0-9_]*", key) is None:
            raise NarrativeValidationError(f"{path}: invalid placeholder name {key!r}")
        _expect_string(description, f"{path}.{key}")
    used = set(PLACEHOLDER.findall(source_text))
    if used != set(placeholders):
        raise NarrativeValidationError(
            f"{path}: declared placeholders {sorted(placeholders)} do not match source placeholders {sorted(used)}"
        )


def _validate_localized_text(
    value: Any,
    path: str,
    content_ids: set[str],
    loc_keys: set[str],
) -> str:
    record = _exact_keys(
        value,
        {"id", "loc_key", "source_text", "placeholders", "text_budget", "binding_status"},
        path,
    )
    identifier = _validate_narrative_id(record["id"], f"{path}.id")
    if identifier in content_ids:
        raise NarrativeValidationError(f"{path}.id: duplicate content identifier {identifier}")
    content_ids.add(identifier)
    loc_key = _validate_narrative_id(record["loc_key"], f"{path}.loc_key")
    if loc_key != identifier:
        raise NarrativeValidationError(f"{path}.loc_key: must equal the canonical source id")
    if loc_key in loc_keys:
        raise NarrativeValidationError(f"{path}.loc_key: duplicate localization key {loc_key}")
    loc_keys.add(loc_key)
    source_text = _validate_source_text(record["source_text"], f"{path}.source_text")
    _validate_placeholders(record["placeholders"], source_text, f"{path}.placeholders")
    _validate_text_budget(record["text_budget"], source_text, f"{path}.text_budget")
    _expect_exact(record["binding_status"], "authored_unbound", f"{path}.binding_status")
    return identifier


def validate_campaign_canon(value: dict[str, Any]) -> dict[str, int]:
    """Validate the exact M01-M15 canon and continuity projection."""
    top = _exact_keys(
        value,
        {"schema_version", "namespace", "content_id", "metadata", "campaign", "world", "characters", "missions"},
        "canon",
    )
    _expect_exact(top["schema_version"], 1, "canon.schema_version")
    _expect_exact(top["namespace"], "echoes.narrative", "canon.namespace")
    _expect_exact(top["content_id"], "nar_campaign_canon_continuity_v1", "canon.content_id")

    metadata = _exact_keys(
        top["metadata"],
        {"author", "creator", "source_locale", "authority_path", "authority_status", "content_status", "runtime_consumed"},
        "canon.metadata",
    )
    expected_metadata = {
        "author": "Angelis Pseftis",
        "creator": "Angelis Pseftis",
        "source_locale": "en-US",
        "authority_path": "Docs/DevelopmentBible.md",
        "authority_status": "structured_projection_of_authoritative_bible",
        "content_status": "validated_authored_source",
        "runtime_consumed": False,
    }
    for key, expected in expected_metadata.items():
        _expect_exact(metadata[key], expected, f"canon.metadata.{key}")

    campaign = _exact_keys(
        top["campaign"],
        {"mission_count", "terminal_mission_id", "mission_16_exists", "hidden_moral_score", "founding_well_doctrine_missions", "independent_lume_choice_mission", "lume_protocol_receipt_missions", "final_resolutions"},
        "canon.campaign",
    )
    _expect_exact(campaign["mission_count"], 15, "canon.campaign.mission_count")
    _expect_exact(campaign["terminal_mission_id"], "TheBrokenSun", "canon.campaign.terminal_mission_id")
    _expect_exact(campaign["mission_16_exists"], False, "canon.campaign.mission_16_exists")
    _expect_exact(campaign["hidden_moral_score"], False, "canon.campaign.hidden_moral_score")
    founding_ids = [mission[2] for mission in EXPECTED_MISSIONS[:9]]
    _expect_exact(
        campaign["founding_well_doctrine_missions"],
        founding_ids,
        "canon.campaign.founding_well_doctrine_missions",
    )
    _expect_exact(
        campaign["independent_lume_choice_mission"],
        "ChoirAtLumeReach",
        "canon.campaign.independent_lume_choice_mission",
    )
    _expect_exact(
        campaign["lume_protocol_receipt_missions"],
        [mission[2] for mission in EXPECTED_MISSIONS[10:]],
        "canon.campaign.lume_protocol_receipt_missions",
    )
    _expect_exact(campaign["final_resolutions"], EXPECTED_RESOLUTIONS, "canon.campaign.final_resolutions")

    world = _exact_keys(
        top["world"],
        {"setting", "crownfall_rule", "future_well_rule", "future_well_choices", "factions"},
        "canon.world",
    )
    _expect_exact(world["setting"], "Soryn", "canon.world.setting")
    _validate_source_text(world["crownfall_rule"], "canon.world.crownfall_rule")
    _validate_source_text(world["future_well_rule"], "canon.world.future_well_rule")
    _expect_exact(world["future_well_choices"], EXPECTED_CHOICES, "canon.world.future_well_choices")
    factions = _expect_list(world["factions"], "canon.world.factions")
    if len(factions) != 3:
        raise NarrativeValidationError("canon.world.factions: expected exactly three factions")
    faction_ids: set[str] = set()
    for index, raw_faction in enumerate(factions):
        path = f"canon.world.factions[{index}]"
        faction = _exact_keys(raw_faction, {"id", "canon", "voice_rule", "prohibited_reduction"}, path)
        faction_id = _expect_string(faction["id"], f"{path}.id")
        faction_ids.add(faction_id)
        for key in ("canon", "voice_rule", "prohibited_reduction"):
            _validate_source_text(faction[key], f"{path}.{key}")
    _expect_exact(faction_ids, EXPECTED_FACTIONS, "canon.world.factions[].id")

    characters = _expect_list(top["characters"], "canon.characters")
    if len(characters) != 5:
        raise NarrativeValidationError("canon.characters: expected exactly five principal character records")
    known_mission_ids = {mission[2] for mission in EXPECTED_MISSIONS}
    character_ids: set[str] = set()
    for index, raw_character in enumerate(characters):
        path = f"canon.characters[{index}]"
        character = _exact_keys(raw_character, {"id", "faction_id", "arc", "mission_ids", "continuity_boundary"}, path)
        character_id = _expect_string(character["id"], f"{path}.id")
        if re.fullmatch(r"[a-z][a-z0-9_]*", character_id) is None or character_id in character_ids:
            raise NarrativeValidationError(f"{path}.id: invalid or duplicate character identifier")
        character_ids.add(character_id)
        if character["faction_id"] not in EXPECTED_FACTIONS:
            raise NarrativeValidationError(f"{path}.faction_id: unknown faction")
        _validate_source_text(character["arc"], f"{path}.arc")
        mission_ids = _expect_unique_strings(character["mission_ids"], f"{path}.mission_ids", minimum=1)
        if not set(mission_ids).issubset(known_mission_ids):
            raise NarrativeValidationError(f"{path}.mission_ids: unknown campaign mission reference")
        _validate_source_text(character["continuity_boundary"], f"{path}.continuity_boundary")
    _expect_exact(character_ids, EXPECTED_CHARACTER_IDS, "canon.characters[].id")

    missions = _expect_list(top["missions"], "canon.missions")
    if len(missions) != 15:
        raise NarrativeValidationError("canon.missions: expected exactly M01 through M15; Mission 16 is unsupported")
    permitted_authorities = character_ids | {"meridian_authority", "local_hollow_choir_authority"}
    for index, raw_mission in enumerate(missions):
        expected = EXPECTED_MISSIONS[index]
        path = f"canon.missions[{index}]"
        mission = _exact_keys(
            raw_mission,
            {"sequence", "title", "mission_id", "operation_mode", "command_authority", "command_faction", "named_participants", "continuity_input", "established_output", "prohibited_inferences", "mechanics_status", "narrative_presentation_status"},
            path,
        )
        actual_identity = (
            _expect_int(mission["sequence"], f"{path}.sequence"),
            _expect_string(mission["title"], f"{path}.title"),
            _expect_string(mission["mission_id"], f"{path}.mission_id"),
            _expect_string(mission["operation_mode"], f"{path}.operation_mode"),
        )
        _expect_exact(actual_identity, expected, f"{path}.identity")
        authority = _expect_string(mission["command_authority"], f"{path}.command_authority")
        if authority not in permitted_authorities:
            raise NarrativeValidationError(f"{path}.command_authority: unknown authority {authority!r}")
        if mission["command_faction"] not in EXPECTED_FACTIONS:
            raise NarrativeValidationError(f"{path}.command_faction: unknown faction")
        participants = _expect_unique_strings(mission["named_participants"], f"{path}.named_participants")
        if not set(participants).issubset(character_ids):
            raise NarrativeValidationError(f"{path}.named_participants: unknown named character")
        for key in ("continuity_input", "established_output", "prohibited_inferences"):
            entries = _expect_list(mission[key], f"{path}.{key}")
            if not entries:
                raise NarrativeValidationError(f"{path}.{key}: at least one evidence-bounded statement is required")
            for entry_index, entry in enumerate(entries):
                _validate_source_text(entry, f"{path}.{key}[{entry_index}]")
        _expect_exact(
            mission["mechanics_status"],
            "bounded_operation_implemented_current_source",
            f"{path}.mechanics_status",
        )
        _expect_exact(
            mission["narrative_presentation_status"],
            "final_dialogue_cinematics_unqualified",
            f"{path}.narrative_presentation_status",
        )
    return {"missions": len(missions), "characters": len(characters), "factions": len(factions)}


def validate_mission_contract(value: dict[str, Any], canon: dict[str, Any]) -> dict[str, int]:
    """Validate the closed Mission 01 authored-source contract."""
    top = _exact_keys(
        value,
        {"schema_version", "namespace", "content_id", "metadata", "runtime_binding", "canon", "speakers", "triggers", "ui_copy", "lines", "dialogue_sequences", "branch_variants", "result_variants", "failure_retry", "cinematic", "asset_hooks", "projections", "implementation"},
        "mission",
    )
    _expect_exact(top["schema_version"], 1, "mission.schema_version")
    _expect_exact(top["namespace"], "echoes.narrative", "mission.namespace")
    _expect_exact(top["content_id"], "nar_m01_what_the_ledger_keeps", "mission.content_id")

    metadata = _exact_keys(
        top["metadata"],
        {"author", "creator", "source_locale", "rights_status", "originality_review_status", "authority_path", "content_status"},
        "mission.metadata",
    )
    expected_metadata = {
        "author": "Angelis Pseftis",
        "creator": "Angelis Pseftis",
        "source_locale": "en-US",
        "rights_status": "author_attributed_project_source",
        "originality_review_status": "human_review_required",
        "authority_path": "Docs/DevelopmentBible.md",
        "content_status": "authored_source_only",
    }
    for key, expected in expected_metadata.items():
        _expect_exact(metadata[key], expected, f"mission.metadata.{key}")

    runtime = _exact_keys(
        top["runtime_binding"],
        {"mission_id", "operation_mode", "phases", "well_choices", "well_uncommitted_state", "campaign_commit_statuses", "runtime_consumed", "localization_runtime_status", "failure_reason_binding"},
        "mission.runtime_binding",
    )
    _expect_exact(runtime["mission_id"], "WhatTheLedgerKeeps", "mission.runtime_binding.mission_id")
    _expect_exact(runtime["operation_mode"], "CampaignPrologue", "mission.runtime_binding.operation_mode")
    _expect_exact(runtime["phases"], EXPECTED_PHASES, "mission.runtime_binding.phases")
    _expect_exact(runtime["well_choices"], EXPECTED_CHOICES, "mission.runtime_binding.well_choices")
    _expect_exact(runtime["well_uncommitted_state"], "Dormant", "mission.runtime_binding.well_uncommitted_state")
    _expect_exact(runtime["campaign_commit_statuses"], EXPECTED_COMMIT_STATUSES, "mission.runtime_binding.campaign_commit_statuses")
    _expect_exact(runtime["runtime_consumed"], False, "mission.runtime_binding.runtime_consumed")
    _expect_exact(runtime["localization_runtime_status"], "unimplemented", "mission.runtime_binding.localization_runtime_status")
    _expect_exact(runtime["failure_reason_binding"], "requested", "mission.runtime_binding.failure_reason_binding")

    matrix_m01 = canon["missions"][0]
    _expect_exact(matrix_m01["mission_id"], runtime["mission_id"], "mission.runtime_binding.matrix_mission_id")
    _expect_exact(matrix_m01["operation_mode"], runtime["operation_mode"], "mission.runtime_binding.matrix_operation_mode")

    canon_record = _exact_keys(
        top["canon"],
        {"title", "purpose", "player_pov", "command_faction", "archive_carrier_role", "recovery_tile", "evacuation_tile", "carrier_must_hold_recovery_during_well_commit", "hostile_core_destruction_substitutes_for_evacuation", "hidden_moral_score", "canonical_facts", "prohibited_inferences"},
        "mission.canon",
    )
    _expect_exact(canon_record["title"], matrix_m01["title"], "mission.canon.title")
    _validate_source_text(canon_record["purpose"], "mission.canon.purpose")
    _expect_exact(canon_record["player_pov"], "mara_vey_command_authority", "mission.canon.player_pov")
    _expect_exact(canon_record["command_faction"], "meridian_compact", "mission.canon.command_faction")
    _expect_exact(canon_record["archive_carrier_role"], "meridian_scout", "mission.canon.archive_carrier_role")
    _expect_exact(canon_record["recovery_tile"], {"x": 22, "y": 18}, "mission.canon.recovery_tile")
    _expect_exact(canon_record["evacuation_tile"], {"x": 6, "y": 17}, "mission.canon.evacuation_tile")
    _expect_exact(canon_record["carrier_must_hold_recovery_during_well_commit"], True, "mission.canon.carrier_must_hold_recovery_during_well_commit")
    _expect_exact(canon_record["hostile_core_destruction_substitutes_for_evacuation"], False, "mission.canon.hostile_core_destruction_substitutes_for_evacuation")
    _expect_exact(canon_record["hidden_moral_score"], False, "mission.canon.hidden_moral_score")
    for key, minimum in (("canonical_facts", 7), ("prohibited_inferences", 6)):
        entries = _expect_list(canon_record[key], f"mission.canon.{key}")
        if len(entries) < minimum:
            raise NarrativeValidationError(f"mission.canon.{key}: expected at least {minimum} statements")
        for index, entry in enumerate(entries):
            _validate_source_text(entry, f"mission.canon.{key}[{index}]")

    speakers = _expect_list(top["speakers"], "mission.speakers")
    if len(speakers) != 3:
        raise NarrativeValidationError("mission.speakers: expected exactly Mara, Talar, and Oruun")
    speaker_ids: set[str] = set()
    speaker_channels: dict[str, str] = {}
    command_speakers: list[str] = []
    for index, raw_speaker in enumerate(speakers):
        path = f"mission.speakers[{index}]"
        speaker = _exact_keys(
            raw_speaker,
            {"id", "display_name", "faction_id", "role_in_mission", "command_authority", "delivery_channel", "physical_presence_status", "voice_asset_status"},
            path,
        )
        speaker_id = _expect_string(speaker["id"], f"{path}.id")
        if SPEAKER_ID.fullmatch(speaker_id) is None or speaker_id in speaker_ids:
            raise NarrativeValidationError(f"{path}.id: invalid or duplicate speaker identifier")
        speaker_ids.add(speaker_id)
        _validate_source_text(speaker["display_name"], f"{path}.display_name")
        if speaker["faction_id"] not in EXPECTED_FACTIONS:
            raise NarrativeValidationError(f"{path}.faction_id: unknown faction")
        _expect_symbol(speaker["role_in_mission"], f"{path}.role_in_mission")
        if _expect_bool(speaker["command_authority"], f"{path}.command_authority"):
            command_speakers.append(speaker_id)
        channel = _expect_symbol(speaker["delivery_channel"], f"{path}.delivery_channel")
        speaker_channels[speaker_id] = channel
        _expect_exact(speaker["physical_presence_status"], "not_asserted_by_contract", f"{path}.physical_presence_status")
        _expect_exact(speaker["voice_asset_status"], "absent", f"{path}.voice_asset_status")
    _expect_exact(
        speaker_ids,
        {"spk_mara_vey", "spk_talar_venn", "spk_oruun_seven_stones"},
        "mission.speakers[].id",
    )
    _expect_exact(command_speakers, ["spk_mara_vey"], "mission.speakers[].command_authority")

    triggers = _expect_list(top["triggers"], "mission.triggers")
    if len(triggers) != len(EXPECTED_TRIGGER_SIGNALS):
        raise NarrativeValidationError("mission.triggers: exact seven-trigger contract required")
    trigger_ids: set[str] = set()
    trigger_prerequisites: dict[str, list[str]] = {}
    for index, raw_trigger in enumerate(triggers):
        path = f"mission.triggers[{index}]"
        trigger = _exact_keys(
            raw_trigger,
            {"id", "runtime_signal", "prerequisite_ids", "occurrence", "reset_behavior", "binding_status"},
            path,
        )
        trigger_id = _validate_narrative_id(trigger["id"], f"{path}.id")
        if trigger_id in trigger_ids:
            raise NarrativeValidationError(f"{path}.id: duplicate trigger identifier")
        trigger_ids.add(trigger_id)
        signal = _expect_string(trigger["runtime_signal"], f"{path}.runtime_signal")
        if EXPECTED_TRIGGER_SIGNALS.get(trigger_id) != signal:
            raise NarrativeValidationError(f"{path}.runtime_signal: does not match the stable source signal")
        prereqs = _expect_unique_strings(trigger["prerequisite_ids"], f"{path}.prerequisite_ids")
        trigger_prerequisites[trigger_id] = prereqs
        if trigger["occurrence"] not in {"once_per_attempt", "once_per_completion", "repeatable_after_failure"}:
            raise NarrativeValidationError(f"{path}.occurrence: unsupported occurrence policy")
        if trigger["reset_behavior"] not in {"reset_on_mission_retry", "reset_on_mission_replay", "starts_new_mission_attempt"}:
            raise NarrativeValidationError(f"{path}.reset_behavior: unsupported reset policy")
        _expect_exact(trigger["binding_status"], "authored_unbound", f"{path}.binding_status")
    _expect_exact(set(EXPECTED_TRIGGER_SIGNALS), trigger_ids, "mission.triggers[].id")
    for trigger_id, prereqs in trigger_prerequisites.items():
        _expect_exact(
            prereqs,
            EXPECTED_TRIGGER_PREREQUISITES[trigger_id],
            f"mission.trigger.{trigger_id}.prerequisite_ids",
        )

    content_ids: set[str] = {top["content_id"]}
    loc_keys: set[str] = set()
    ui = _exact_keys(top["ui_copy"], {"briefing", "objectives"}, "mission.ui_copy")
    _validate_localized_text(ui["briefing"], "mission.ui_copy.briefing", content_ids, loc_keys)
    objectives = _expect_list(ui["objectives"], "mission.ui_copy.objectives")
    if len(objectives) != 3:
        raise NarrativeValidationError("mission.ui_copy.objectives: expected exact three-phase objective copy")
    for index, objective in enumerate(objectives):
        _validate_localized_text(objective, f"mission.ui_copy.objectives[{index}]", content_ids, loc_keys)

    lines = _expect_list(top["lines"], "mission.lines")
    if len(lines) != 28:
        raise NarrativeValidationError("mission.lines: expected exact 28-line first-slice contract")
    line_ids: set[str] = set()
    voice_hook_ids: set[str] = set()
    line_triggers: dict[str, str] = {}
    for index, raw_line in enumerate(lines):
        path = f"mission.lines[{index}]"
        line = _exact_keys(
            raw_line,
            {"id", "loc_key", "speaker_id", "trigger_id", "delivery_channel", "source_text", "placeholders", "text_budget", "subtitle", "transcript_included", "voice_hook", "binding_status"},
            path,
        )
        line_id = _validate_narrative_id(line["id"], f"{path}.id")
        if not line_id.startswith("nar_m01_line_") or line_id in content_ids:
            raise NarrativeValidationError(f"{path}.id: invalid or duplicate Mission 01 line identifier")
        content_ids.add(line_id)
        line_ids.add(line_id)
        loc_key = _validate_narrative_id(line["loc_key"], f"{path}.loc_key")
        if loc_key != line_id or loc_key in loc_keys:
            raise NarrativeValidationError(f"{path}.loc_key: must be unique and equal the line id")
        loc_keys.add(loc_key)
        speaker_id = _expect_string(line["speaker_id"], f"{path}.speaker_id")
        if speaker_id not in speaker_ids:
            raise NarrativeValidationError(f"{path}.speaker_id: unresolved speaker reference")
        trigger_id = _expect_string(line["trigger_id"], f"{path}.trigger_id")
        if trigger_id not in trigger_ids:
            raise NarrativeValidationError(f"{path}.trigger_id: unresolved trigger reference")
        line_triggers[line_id] = trigger_id
        _expect_exact(line["delivery_channel"], speaker_channels[speaker_id], f"{path}.delivery_channel")
        source_text = _validate_source_text(line["source_text"], f"{path}.source_text")
        _validate_placeholders(line["placeholders"], source_text, f"{path}.placeholders")
        _validate_text_budget(line["text_budget"], source_text, f"{path}.text_budget")
        subtitle = _exact_keys(line["subtitle"], {"enabled", "timing_status"}, f"{path}.subtitle")
        _expect_exact(subtitle["enabled"], True, f"{path}.subtitle.enabled")
        _expect_exact(subtitle["timing_status"], "unassigned", f"{path}.subtitle.timing_status")
        _expect_exact(line["transcript_included"], True, f"{path}.transcript_included")
        voice_hook = _exact_keys(line["voice_hook"], {"id", "asset_status"}, f"{path}.voice_hook")
        voice_id = _expect_string(voice_hook["id"], f"{path}.voice_hook.id")
        if ASSET_ID.fullmatch(voice_id) is None or not voice_id.startswith("aud_m01_vo_") or voice_id in voice_hook_ids:
            raise NarrativeValidationError(f"{path}.voice_hook.id: invalid or duplicate logical voice hook")
        voice_hook_ids.add(voice_id)
        _expect_exact(voice_hook["asset_status"], "absent", f"{path}.voice_hook.asset_status")
        _expect_exact(line["binding_status"], "authored_unbound", f"{path}.binding_status")

    used_line_ids: set[str] = set()
    sequences = _expect_list(top["dialogue_sequences"], "mission.dialogue_sequences")
    if len(sequences) != 4:
        raise NarrativeValidationError("mission.dialogue_sequences: expected opening, recovery, withdrawal, and completion")
    sequence_ids: set[str] = set()
    sequence_lines: dict[str, list[str]] = {}
    for index, raw_sequence in enumerate(sequences):
        path = f"mission.dialogue_sequences[{index}]"
        sequence = _exact_keys(raw_sequence, {"id", "beat", "trigger_id", "line_ids", "binding_status"}, path)
        sequence_id = _validate_narrative_id(sequence["id"], f"{path}.id")
        if sequence_id in sequence_ids or sequence_id in content_ids:
            raise NarrativeValidationError(f"{path}.id: invalid or duplicate sequence identifier")
        sequence_ids.add(sequence_id)
        content_ids.add(sequence_id)
        _expect_symbol(sequence["beat"], f"{path}.beat")
        trigger_id = _expect_string(sequence["trigger_id"], f"{path}.trigger_id")
        if trigger_id not in trigger_ids:
            raise NarrativeValidationError(f"{path}.trigger_id: unresolved trigger reference")
        refs = _expect_unique_strings(sequence["line_ids"], f"{path}.line_ids", minimum=1)
        if not set(refs).issubset(line_ids):
            raise NarrativeValidationError(f"{path}.line_ids: unresolved line reference")
        if any(line_triggers[ref] != trigger_id for ref in refs):
            raise NarrativeValidationError(f"{path}.line_ids: line trigger does not match sequence trigger")
        if used_line_ids.intersection(refs):
            raise NarrativeValidationError(f"{path}.line_ids: a line may belong to only one dialogue sequence or variant")
        used_line_ids.update(refs)
        sequence_lines[sequence_id] = refs
        _expect_exact(sequence["binding_status"], "authored_unbound", f"{path}.binding_status")

    branches = _exact_keys(top["branch_variants"], EXPECTED_CHOICES, "mission.branch_variants")
    branch_line_counts: set[int] = set()
    branch_line_ids: set[str] = set()
    for choice in EXPECTED_CHOICES:
        path = f"mission.branch_variants.{choice}"
        branch = _exact_keys(
            branches[choice],
            {
                "choice",
                "current_runtime_behavior",
                "design_target_tradeoff",
                "runtime_alignment",
                "trigger_id",
                "dialogue_line_ids",
                "design_target_choice_ui",
                "binding_status",
            },
            path,
        )
        _expect_exact(branch["choice"], choice, f"{path}.choice")
        current_behavior = _validate_source_text(
            branch["current_runtime_behavior"],
            f"{path}.current_runtime_behavior",
        )
        target_tradeoff = _validate_source_text(
            branch["design_target_tradeoff"],
            f"{path}.design_target_tradeoff",
        )
        for field_name, text in (
            ("current_runtime_behavior", current_behavior),
            ("design_target_tradeoff", target_tradeoff),
        ):
            folded_text = text.casefold()
            if any(fragment in folded_text for fragment in MORAL_RANKING_FRAGMENTS):
                raise NarrativeValidationError(
                    f"{path}.{field_name}: moral ranking is prohibited"
                )
        for fragment in EXPECTED_BRANCH_RUNTIME_FRAGMENTS[choice]:
            if fragment.casefold() not in current_behavior.casefold():
                raise NarrativeValidationError(
                    f"{path}.current_runtime_behavior: missing required current-source fragment {fragment!r}"
                )
        for fragment in FORBIDDEN_CURRENT_RUNTIME_FRAGMENTS[choice]:
            if fragment.casefold() in current_behavior.casefold():
                raise NarrativeValidationError(
                    f"{path}.current_runtime_behavior: design-target fragment {fragment!r} is not current runtime behavior"
                )
        for fragment in EXPECTED_BRANCH_TARGET_FRAGMENTS[choice]:
            if fragment.casefold() not in target_tradeoff.casefold():
                raise NarrativeValidationError(
                    f"{path}.design_target_tradeoff: missing required design-target fragment {fragment!r}"
                )
        _expect_exact(
            branch["runtime_alignment"],
            EXPECTED_BRANCH_ALIGNMENT[choice],
            f"{path}.runtime_alignment",
        )
        _expect_exact(branch["trigger_id"], "nar_m01_evt_well_decision_committed", f"{path}.trigger_id")
        refs = _expect_unique_strings(branch["dialogue_line_ids"], f"{path}.dialogue_line_ids", minimum=1)
        if len(refs) != 3 or not set(refs).issubset(line_ids):
            raise NarrativeValidationError(f"{path}.dialogue_line_ids: exact three-line resolved branch required")
        if any(line_triggers[ref] != branch["trigger_id"] for ref in refs):
            raise NarrativeValidationError(f"{path}.dialogue_line_ids: branch line trigger mismatch")
        if branch_line_ids.intersection(refs) or used_line_ids.intersection(refs):
            raise NarrativeValidationError(f"{path}.dialogue_line_ids: branch lines must be mutually exclusive and singly assigned")
        branch_line_ids.update(refs)
        used_line_ids.update(refs)
        branch_line_counts.add(len(refs))
        _validate_localized_text(
            branch["design_target_choice_ui"],
            f"{path}.design_target_choice_ui",
            content_ids,
            loc_keys,
        )
        _expect_exact(branch["binding_status"], "authored_unbound", f"{path}.binding_status")
    _expect_exact(branch_line_counts, {3}, "mission.branch_variants structural parity")
    branch_serialized = json.dumps(branches, sort_keys=True).casefold()
    if "dormant" in branch_serialized:
        raise NarrativeValidationError("mission.branch_variants: Dormant is not a terminal narrative branch")
    if any(fragment in branch_serialized for fragment in MORAL_RANKING_FRAGMENTS):
        raise NarrativeValidationError("mission.branch_variants: moral ranking language is prohibited")

    results = _expect_list(top["result_variants"], "mission.result_variants")
    if len(results) != 4:
        raise NarrativeValidationError("mission.result_variants: exact four campaign persistence states required")
    result_statuses: list[str] = []
    for index, raw_result in enumerate(results):
        path = f"mission.result_variants[{index}]"
        result = _exact_keys(raw_result, {"status", "copy"}, path)
        status = _expect_string(result["status"], f"{path}.status")
        result_statuses.append(status)
        _validate_localized_text(result["copy"], f"{path}.copy", content_ids, loc_keys)
    _expect_exact(result_statuses, EXPECTED_COMMIT_STATUSES, "mission.result_variants[].status")

    failure_retry = _exact_keys(
        top["failure_retry"],
        {"failure_variants", "retry_copy", "campaign_state_effect", "runtime_delivery_status"},
        "mission.failure_retry",
    )
    failures = _expect_list(failure_retry["failure_variants"], "mission.failure_retry.failure_variants")
    if len(failures) != 5:
        raise NarrativeValidationError("mission.failure_retry.failure_variants: exact four reducer failures plus generic fallback required")
    failure_reasons: list[str] = []
    for index, raw_failure in enumerate(failures):
        path = f"mission.failure_retry.failure_variants[{index}]"
        failure = _exact_keys(raw_failure, {"id", "reason_code", "source_condition", "binding_status", "dialogue_line_ids"}, path)
        failure_id = _validate_narrative_id(failure["id"], f"{path}.id")
        if failure_id in content_ids:
            raise NarrativeValidationError(f"{path}.id: duplicate failure identifier")
        content_ids.add(failure_id)
        reason = _expect_string(failure["reason_code"], f"{path}.reason_code")
        failure_reasons.append(reason)
        _validate_source_text(failure["source_condition"], f"{path}.source_condition")
        expected_binding = "fallback_available" if reason == "generic" else "reason_code_requested"
        _expect_exact(failure["binding_status"], expected_binding, f"{path}.binding_status")
        refs = _expect_unique_strings(failure["dialogue_line_ids"], f"{path}.dialogue_line_ids", minimum=1)
        if len(refs) != 1 or not set(refs).issubset(line_ids):
            raise NarrativeValidationError(f"{path}.dialogue_line_ids: exact one resolved failure line required")
        if any(line_triggers[ref] != "nar_m01_evt_mission_failed" for ref in refs):
            raise NarrativeValidationError(f"{path}.dialogue_line_ids: failure line trigger mismatch")
        if used_line_ids.intersection(refs):
            raise NarrativeValidationError(f"{path}.dialogue_line_ids: failure lines must be singly assigned")
        used_line_ids.update(refs)
    _expect_exact(failure_reasons, EXPECTED_FAILURE_REASONS, "mission.failure_retry.failure_variants[].reason_code")
    _validate_localized_text(failure_retry["retry_copy"], "mission.failure_retry.retry_copy", content_ids, loc_keys)
    _expect_exact(
        failure_retry["campaign_state_effect"],
        "A failed Mission 01 attempt appends no campaign decision record.",
        "mission.failure_retry.campaign_state_effect",
    )
    _expect_exact(failure_retry["runtime_delivery_status"], "unimplemented", "mission.failure_retry.runtime_delivery_status")

    asset_hooks = _expect_list(top["asset_hooks"], "mission.asset_hooks")
    if len(asset_hooks) != 5:
        raise NarrativeValidationError("mission.asset_hooks: expected four visual hooks and one ambience hook")
    asset_hook_ids: set[str] = set()
    asset_kinds: dict[str, str] = {}
    for index, raw_hook in enumerate(asset_hooks):
        path = f"mission.asset_hooks[{index}]"
        hook = _exact_keys(raw_hook, {"id", "kind", "asset_status", "binding_status"}, path)
        hook_id = _expect_string(hook["id"], f"{path}.id")
        if ASSET_ID.fullmatch(hook_id) is None or hook_id in asset_hook_ids:
            raise NarrativeValidationError(f"{path}.id: invalid or duplicate logical asset hook")
        asset_hook_ids.add(hook_id)
        kind = _expect_string(hook["kind"], f"{path}.kind")
        if kind not in {"audio", "visual"} or not hook_id.startswith("aud_" if kind == "audio" else "vis_"):
            raise NarrativeValidationError(f"{path}.kind: does not match logical hook prefix")
        asset_kinds[hook_id] = kind
        _expect_exact(hook["asset_status"], "absent", f"{path}.asset_status")
        _expect_exact(hook["binding_status"], "unimplemented", f"{path}.binding_status")
    _expect_exact(asset_kinds, EXPECTED_ASSET_HOOKS, "mission.asset_hooks logical inventory")

    cinematic = _exact_keys(
        top["cinematic"],
        {"id", "title", "trigger_id", "format", "implementation_status", "binding_status", "named_character_physical_presence_asserted", "shots"},
        "mission.cinematic",
    )
    cinematic_id = _validate_narrative_id(cinematic["id"], "mission.cinematic.id")
    if cinematic_id in content_ids:
        raise NarrativeValidationError("mission.cinematic.id: duplicate content identifier")
    content_ids.add(cinematic_id)
    _validate_source_text(cinematic["title"], "mission.cinematic.title")
    _expect_exact(cinematic["trigger_id"], "nar_m01_evt_operation_started", "mission.cinematic.trigger_id")
    _expect_exact(cinematic["format"], "in_engine_storyboard", "mission.cinematic.format")
    _expect_exact(cinematic["implementation_status"], "absent", "mission.cinematic.implementation_status")
    _expect_exact(cinematic["binding_status"], "authored_unbound", "mission.cinematic.binding_status")
    _expect_exact(cinematic["named_character_physical_presence_asserted"], False, "mission.cinematic.named_character_physical_presence_asserted")
    shots = _expect_list(cinematic["shots"], "mission.cinematic.shots")
    if len(shots) != 4:
        raise NarrativeValidationError("mission.cinematic.shots: expected exact four-shot opening storyboard")
    cinematic_line_order: list[str] = []
    shot_ids: set[str] = set()
    referenced_asset_hook_ids: set[str] = set()
    for index, raw_shot in enumerate(shots):
        path = f"mission.cinematic.shots[{index}]"
        shot = _exact_keys(raw_shot, {"id", "editorial_target_seconds", "visual_direction", "line_ids", "visual_hook_ids", "audio_hook_ids"}, path)
        shot_id = _validate_narrative_id(shot["id"], f"{path}.id")
        if shot_id in shot_ids or shot_id in content_ids:
            raise NarrativeValidationError(f"{path}.id: invalid or duplicate shot identifier")
        shot_ids.add(shot_id)
        content_ids.add(shot_id)
        duration = shot["editorial_target_seconds"]
        if (
            isinstance(duration, bool)
            or not isinstance(duration, (int, float))
            or not math.isfinite(duration)
            or duration <= 0
            or duration > 30
        ):
            raise NarrativeValidationError(f"{path}.editorial_target_seconds: expected value in (0, 30]")
        _validate_source_text(shot["visual_direction"], f"{path}.visual_direction")
        line_refs = _expect_unique_strings(shot["line_ids"], f"{path}.line_ids", minimum=1)
        if not set(line_refs).issubset(line_ids):
            raise NarrativeValidationError(f"{path}.line_ids: unresolved line reference")
        cinematic_line_order.extend(line_refs)
        visual_refs = _expect_unique_strings(shot["visual_hook_ids"], f"{path}.visual_hook_ids", minimum=1)
        audio_refs = _expect_unique_strings(shot["audio_hook_ids"], f"{path}.audio_hook_ids", minimum=1)
        if any(asset_kinds.get(ref) != "visual" for ref in visual_refs):
            raise NarrativeValidationError(f"{path}.visual_hook_ids: unresolved or nonvisual hook")
        if any(asset_kinds.get(ref) != "audio" for ref in audio_refs):
            raise NarrativeValidationError(f"{path}.audio_hook_ids: unresolved or nonaudio hook")
        referenced_asset_hook_ids.update(visual_refs)
        referenced_asset_hook_ids.update(audio_refs)
    _expect_exact(cinematic_line_order, sequence_lines["nar_m01_seq_opening"], "mission.cinematic.shots opening line order")
    _expect_exact(
        referenced_asset_hook_ids,
        asset_hook_ids,
        "mission.cinematic.shots asset hook coverage",
    )

    projections = _exact_keys(top["projections"], {"subtitle_source", "transcript_source", "duplication_policy"}, "mission.projections")
    _expect_exact(projections["subtitle_source"], "lines[].source_text", "mission.projections.subtitle_source")
    _expect_exact(projections["transcript_source"], "lines[].source_text", "mission.projections.transcript_source")
    _expect_exact(projections["duplication_policy"], "generated_from_canonical_source_text", "mission.projections.duplication_policy")

    implementation = _exact_keys(
        top["implementation"],
        {
            "mechanics",
            "narrative_contract",
            "runtime_consumption",
            "subtitles",
            "voice_assets",
            "cinematics",
            "localization_pipeline",
            "well_telegraphs",
            "reshape_expiry_warning",
            "manual_observation",
            "packaged_build",
        },
        "mission.implementation",
    )
    expected_implementation = {
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
    }
    for key, expected in expected_implementation.items():
        _expect_exact(implementation[key], expected, f"mission.implementation.{key}")

    if used_line_ids != line_ids:
        missing = sorted(line_ids - used_line_ids)
        raise NarrativeValidationError(f"mission.lines: unreferenced canonical lines: {', '.join(missing)}")
    return {
        "lines": len(lines),
        "branches": len(branches),
        "failures": len(failures),
        "results": len(results),
        "shots": len(shots),
    }


def validate_schema_documents(schema_dir: Path) -> None:
    expected = {
        "campaign_canon_continuity.schema.json": "echoes://narrative/schema/campaign-canon-continuity-v1",
        "mission_contract.schema.json": "echoes://narrative/schema/mission-contract-v1",
    }
    for filename, schema_id in expected.items():
        schema = load_json_document(schema_dir / filename)
        _expect_exact(schema.get("$schema"), "https://json-schema.org/draft/2020-12/schema", f"schema.{filename}.$schema")
        _expect_exact(schema.get("$id"), schema_id, f"schema.{filename}.$id")
        _expect_exact(schema.get("type"), "object", f"schema.{filename}.type")
        _expect_exact(schema.get("additionalProperties"), False, f"schema.{filename}.additionalProperties")


def validate_source_tree(root: Path) -> dict[str, int]:
    source_dir = root / "Content/Narrative/Source"
    schema_dir = root / "Content/Narrative/Schema"
    validate_schema_documents(schema_dir)
    canon = load_json_document(source_dir / "campaign_canon_continuity.json")
    canon_counts = validate_campaign_canon(canon)
    mission = load_json_document(source_dir / "missions/m01_what_the_ledger_keeps.json")
    mission_counts = validate_mission_contract(mission, canon)
    return {**canon_counts, **mission_counts}


def _default_root() -> Path:
    return Path(__file__).resolve().parents[3]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=_default_root())
    arguments = parser.parse_args()
    try:
        counts = validate_source_tree(arguments.root.resolve())
    except NarrativeValidationError as exc:
        print(f"NARRATIVE_VALIDATION_FAILED {exc}")
        return 1
    print(
        "NARRATIVE_VALIDATION_OK "
        f"missions={counts['missions']} characters={counts['characters']} "
        f"factions={counts['factions']} lines={counts['lines']} "
        f"branches={counts['branches']} failures={counts['failures']} "
        f"results={counts['results']} shots={counts['shots']} "
        "runtime_consumed=false"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
