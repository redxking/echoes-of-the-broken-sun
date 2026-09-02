#!/usr/bin/env python3
"""Fail-closed validation for authored Echoes narrative source.

This validator intentionally has no runtime or third-party dependency. It checks
the structured projection of the authoritative Development Bible and the first
authored Mission 01 presentation contract. Passing validation does not mean the
runtime consumes the contract or that voice, localization, or cinematic assets
exist.

Branch editorial assurance is deterministic drift detection against one exact,
reviewed Mission 01 projection plus a limited explicit-phrase guard. It is not
general semantic moral-bias detection, and human editorial review remains
mandatory before any prose change is accepted.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import re
from pathlib import Path
from typing import Any, Iterable


MAX_SOURCE_BYTES = 256_000
NARRATIVE_ID = re.compile(r"^nar_[a-z0-9_]+$")
SPEAKER_ID = re.compile(r"^spk_[a-z0-9_]+$")
ASSET_ID = re.compile(r"^(?:aud|vis)_m01_[a-z0-9_]+$")
GENERAL_ASSET_ID = re.compile(r"^(?:aud|vis)_m(?:0[1-9]|1[0-5])_[a-z0-9_]+$")


def _mission_prefix_patterns(prefix: str) -> dict[str, re.Pattern[str]]:
    """Compiled per-mission identifier patterns for a stem prefix like m02."""
    return {
        "line": re.compile(rf"^nar_{prefix}_line_[a-z0-9_]+$"),
        "event": re.compile(rf"^nar_{prefix}_evt_[a-z0-9_]+$"),
        "voice": re.compile(rf"^aud_{prefix}_vo_[a-z0-9_]+$"),
        "asset": re.compile(rf"^(?:aud|vis)_{prefix}_[a-z0-9_]+$"),
    }
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
# Missions whose failure reason codes the runtime derives and binds today.
RUNTIME_BOUND_FAILURE_MISSIONS = {
    "m01_what_the_ledger_keeps",
    "m02_seven_accounts_of_rain",
    "m03_a_city_on_reserve",
    "m04_the_unburied_road",
    "m05_terms_of_continuance",
    "m06_names_without_births",
    "m07_the_shape_of_silence",
    "m08_the_shape_beside_us",
    "m09_reserve_authority",
    "m10_the_choir_at_lume_reach",
    "m11_no_neutral_ledger",
    "m12_the_future_that_won",
    "m13_assembly_of_the_missing",
    "m14_several_voices_one_command",
    "m15_the_broken_sun",
}

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
EXPECTED_CHARACTER_RELATIONSHIPS = {
    "mara_vey": (
        "meridian_compact",
        (
            "WhatTheLedgerKeeps",
            "ACityOnReserve",
            "TermsOfContinuance",
            "ReserveAuthority",
            "ChoirAtLumeReach",
            "TheBrokenSun",
        ),
    ),
    "oruun_of_seven_stones": (
        "kharuun_assemblies",
        (
            "WhatTheLedgerKeeps",
            "SevenAccountsOfRain",
            "TheUnburiedRoad",
            "TheShapeOfSilence",
            "ChoirAtLumeReach",
            "NoNeutralLedger",
            "TheFutureThatWon",
            "AssemblyOfTheMissing",
            "TheBrokenSun",
        ),
    ),
    "talar_venn": (
        "meridian_compact",
        (
            "WhatTheLedgerKeeps",
            "NamesWithoutBirths",
            "TheShapeBesideUs",
            "TheBrokenSun",
        ),
    ),
    "neme": (
        "hollow_choir",
        ("TheShapeBesideUs", "SeveralVoicesOneCommand", "TheBrokenSun"),
    ),
    "cael_rhyse": ("meridian_compact", ("TheFutureThatWon",)),
}
EXPECTED_MISSION_RELATIONSHIPS = {
    "WhatTheLedgerKeeps": (
        "mara_vey",
        "meridian_compact",
        ("mara_vey", "talar_venn", "oruun_of_seven_stones"),
    ),
    "SevenAccountsOfRain": (
        "oruun_of_seven_stones",
        "kharuun_assemblies",
        ("oruun_of_seven_stones",),
    ),
    "ACityOnReserve": ("mara_vey", "meridian_compact", ("mara_vey",)),
    "TheUnburiedRoad": (
        "oruun_of_seven_stones",
        "kharuun_assemblies",
        ("oruun_of_seven_stones",),
    ),
    "TermsOfContinuance": ("mara_vey", "meridian_compact", ("mara_vey",)),
    "NamesWithoutBirths": (
        "meridian_authority",
        "meridian_compact",
        ("talar_venn",),
    ),
    "TheShapeOfSilence": (
        "oruun_of_seven_stones",
        "kharuun_assemblies",
        ("oruun_of_seven_stones",),
    ),
    "TheShapeBesideUs": (
        "meridian_authority",
        "meridian_compact",
        ("talar_venn", "neme"),
    ),
    "ReserveAuthority": ("mara_vey", "meridian_compact", ("mara_vey",)),
    "ChoirAtLumeReach": (
        "oruun_of_seven_stones",
        "kharuun_assemblies",
        ("oruun_of_seven_stones", "mara_vey"),
    ),
    "NoNeutralLedger": (
        "oruun_of_seven_stones",
        "kharuun_assemblies",
        ("oruun_of_seven_stones",),
    ),
    "TheFutureThatWon": (
        "oruun_of_seven_stones",
        "kharuun_assemblies",
        ("oruun_of_seven_stones", "cael_rhyse"),
    ),
    "AssemblyOfTheMissing": (
        "oruun_of_seven_stones",
        "kharuun_assemblies",
        ("oruun_of_seven_stones",),
    ),
    "SeveralVoicesOneCommand": (
        "local_hollow_choir_authority",
        "hollow_choir",
        ("neme",),
    ),
    "TheBrokenSun": (
        "local_hollow_choir_authority",
        "hollow_choir",
        ("neme", "mara_vey", "oruun_of_seven_stones", "talar_venn"),
    ),
}
EXPECTED_SPEAKER_RECORDS = {
    "spk_mara_vey": {
        "display_name": "Mara Vey",
        "faction_id": "meridian_compact",
        "role_in_mission": "player_command_authority",
        "command_authority": True,
        "delivery_channel": "command_radio",
        "physical_presence_status": "not_asserted_by_contract",
        "voice_asset_status": "absent",
    },
    "spk_talar_venn": {
        "display_name": "Talar Venn",
        "faction_id": "meridian_compact",
        "role_in_mission": "archive_recovery_requester",
        "command_authority": False,
        "delivery_channel": "operations_radio",
        "physical_presence_status": "not_asserted_by_contract",
        "voice_asset_status": "absent",
    },
    "spk_oruun_seven_stones": {
        "display_name": "Oruun-of-Seven-Stones",
        "faction_id": "kharuun_assemblies",
        "role_in_mission": "birthing_cavern_interlocutor",
        "command_authority": False,
        "delivery_channel": "cross_faction_radio",
        "physical_presence_status": "not_asserted_by_contract",
        "voice_asset_status": "absent",
    },
}
SPEAKER_CHARACTER_IDS = {
    "spk_mara_vey": "mara_vey",
    "spk_talar_venn": "talar_venn",
    "spk_oruun_seven_stones": "oruun_of_seven_stones",
    "spk_neme": "neme",
    "spk_cael_rhyse": "cael_rhyse",
}

# Identity fields every appearance of a speaker must carry. Role and command
# authority vary per mission and are pinned by each mission's registry entry.
EXPECTED_SPEAKER_IDENTITIES = {
    "spk_mara_vey": {
        "display_name": "Mara Vey",
        "faction_id": "meridian_compact",
    },
    "spk_talar_venn": {
        "display_name": "Talar Venn",
        "faction_id": "meridian_compact",
    },
    "spk_oruun_seven_stones": {
        "display_name": "Oruun-of-Seven-Stones",
        "faction_id": "kharuun_assemblies",
    },
    "spk_neme": {
        "display_name": "Neme",
        "faction_id": "hollow_choir",
    },
    "spk_cael_rhyse": {
        "display_name": "Chancellor Cael Rhyse",
        "faction_id": "meridian_compact",
    },
}

EXPECTED_CAMPAIGN_CANON_PROSE_SHA256 = (
    "fe6c1e34636876c0ad78d68eea7883bd88b4f9e6862b9ebe40f64624bdd5e38d"
)
EXPECTED_M01_CANON_PROSE_SHA256 = (
    "c487fad938e01f31e21f867840617819150a65d58a89a9fd6b0050436556209d"
)
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
EXPECTED_REVIEWED_BRANCHES = {
    "Harvest": {
        "current_runtime_behavior": (
            "Current source applies Harvest immediately, grants 500 Dawn, collapses "
            "the Well permanently, and scars nearby open terrain."
        ),
        "design_target_tradeoff": (
            "Target behavior adds a public 180-tick commitment before the same "
            "irreversible collapse."
        ),
        "dialogue_line_ids": (
            "nar_m01_line_mara_harvest_001",
            "nar_m01_line_oruun_harvest_001",
            "nar_m01_line_talar_harvest_001",
        ),
        "design_target_choice_ui_source_text": (
            "Harvest: gain 500 Dawn after the public 180-tick commitment; the Well "
            "collapses permanently."
        ),
    },
    "Preserve": {
        "current_runtime_behavior": (
            "Current source keeps the Well intact and, while controlled, grants 15 "
            "Dawn every 300 ticks plus faction-dependent intelligence within 1,400 cm."
        ),
        "design_target_tradeoff": (
            "Target behavior retains the implemented cadence and makes continued "
            "control, delayed value, and changing ownership explicit to the player."
        ),
        "dialogue_line_ids": (
            "nar_m01_line_mara_preserve_001",
            "nar_m01_line_oruun_preserve_001",
            "nar_m01_line_talar_preserve_001",
        ),
        "design_target_choice_ui_source_text": (
            "Preserve: keep the Well intact; while controlled, receive 15 Dawn every "
            "300 ticks and faction-dependent intelligence within 1,400 cm."
        ),
    },
    "Reshape": {
        "current_runtime_behavior": (
            "Current source spends 120 Dawn immediately, manifests the authored "
            "terrain possibility for 1,800 ticks, and uses deterministic fallback "
            "displacement at expiry."
        ),
        "design_target_tradeoff": (
            "Target behavior adds a public 180-tick commitment and a pre-expiry "
            "warning before the implemented deterministic fallback displacement."
        ),
        "dialogue_line_ids": (
            "nar_m01_line_mara_reshape_001",
            "nar_m01_line_oruun_reshape_001",
            "nar_m01_line_talar_reshape_001",
        ),
        "design_target_choice_ui_source_text": (
            "Reshape: spend 120 Dawn to manifest the map-authored possibility for "
            "1,800 ticks after the public 180-tick commitment; expiration is warned "
            "and uses authored fallback displacement."
        ),
    },
}
EXPECTED_REVIEWED_BRANCH_LINES = {
    "nar_m01_line_mara_harvest_001": (
        "spk_mara_vey",
        "Harvest. Take the Dawn. The reserve gets time; the Well does not.",
    ),
    "nar_m01_line_oruun_harvest_001": (
        "spk_oruun_seven_stones",
        "The city gains time. The cavern loses a possibility. Record both.",
    ),
    "nar_m01_line_talar_harvest_001": (
        "spk_talar_venn",
        "Both are in the ledger.",
    ),
    "nar_m01_line_mara_preserve_001": (
        "spk_mara_vey",
        "Preserve. Keep the Well intact. We hold this ground without immediate "
        "reserve relief.",
    ),
    "nar_m01_line_oruun_preserve_001": (
        "spk_oruun_seven_stones",
        "Possibility survives, and every force here has reason to keep fighting.",
    ),
    "nar_m01_line_talar_preserve_001": (
        "spk_talar_venn",
        "The delay and the open Well are recorded.",
    ),
    "nar_m01_line_mara_reshape_001": (
        "spk_mara_vey",
        "Reshape. We spend Dawn for a temporary option. Publish the expiry.",
    ),
    "nar_m01_line_oruun_reshape_001": (
        "spk_oruun_seven_stones",
        "Then no one mistakes a borrowed path for a permanent one.",
    ),
    "nar_m01_line_talar_reshape_001": (
        "spk_talar_venn",
        "Cost and expiry recorded. The carrier is ready.",
    ),
}
BRANCH_EDITORIAL_ASSURANCE_SCOPE = (
    "exact_reviewed_projection_drift_detection_plus_limited_explicit_lexical_guard_"
    "with_mandatory_human_review"
)
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
EXPLICIT_MORAL_RANKING_LEXEMES = (
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


def _canonical_projection_sha256(value: Any) -> str:
    encoded = json.dumps(
        value,
        ensure_ascii=False,
        separators=(",", ":"),
        sort_keys=True,
    ).encode("utf-8")
    return hashlib.sha256(encoded).hexdigest()


def _expect_canonical_projection(
    value: Any,
    expected_sha256: str,
    path: str,
) -> None:
    actual_sha256 = _canonical_projection_sha256(value)
    if actual_sha256 != expected_sha256:
        raise NarrativeValidationError(
            f"{path}: reviewed canonical prose projection changed "
            f"(expected sha256 {expected_sha256}, received {actual_sha256})"
        )


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
        "authority_path": "Docs/Archive/DevelopmentBible.md",
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
        faction_id = _expect_string(character["faction_id"], f"{path}.faction_id")
        if faction_id not in EXPECTED_FACTIONS:
            raise NarrativeValidationError(f"{path}.faction_id: unknown faction")
        _validate_source_text(character["arc"], f"{path}.arc")
        mission_ids = _expect_unique_strings(character["mission_ids"], f"{path}.mission_ids", minimum=1)
        if not set(mission_ids).issubset(known_mission_ids):
            raise NarrativeValidationError(f"{path}.mission_ids: unknown campaign mission reference")
        _validate_source_text(character["continuity_boundary"], f"{path}.continuity_boundary")
        expected_relationship = EXPECTED_CHARACTER_RELATIONSHIPS.get(character_id)
        if expected_relationship is None:
            raise NarrativeValidationError(f"{path}.id: character is not in the reviewed relationship projection")
        _expect_exact(
            (faction_id, tuple(mission_ids)),
            expected_relationship,
            f"{path}.exact_character_relationship",
        )
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
        command_faction = _expect_string(mission["command_faction"], f"{path}.command_faction")
        if command_faction not in EXPECTED_FACTIONS:
            raise NarrativeValidationError(f"{path}.command_faction: unknown faction")
        participants = _expect_unique_strings(mission["named_participants"], f"{path}.named_participants")
        if not set(participants).issubset(character_ids):
            raise NarrativeValidationError(f"{path}.named_participants: unknown named character")
        _expect_exact(
            (authority, command_faction, tuple(participants)),
            EXPECTED_MISSION_RELATIONSHIPS[mission["mission_id"]],
            f"{path}.exact_mission_relationship",
        )
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

    campaign_canon_prose = {
        "world": {
            "crownfall_rule": world["crownfall_rule"],
            "future_well_rule": world["future_well_rule"],
            "factions": [
                {
                    "id": faction["id"],
                    "canon": faction["canon"],
                    "voice_rule": faction["voice_rule"],
                    "prohibited_reduction": faction["prohibited_reduction"],
                }
                for faction in factions
            ],
        },
        "characters": [
            {
                "id": character["id"],
                "arc": character["arc"],
                "continuity_boundary": character["continuity_boundary"],
            }
            for character in characters
        ],
        "missions": [
            {
                "mission_id": mission["mission_id"],
                "continuity_input": mission["continuity_input"],
                "established_output": mission["established_output"],
                "prohibited_inferences": mission["prohibited_inferences"],
            }
            for mission in missions
        ],
    }
    _expect_canonical_projection(
        campaign_canon_prose,
        EXPECTED_CAMPAIGN_CANON_PROSE_SHA256,
        "canon.reviewed_canonical_prose",
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
        "authority_path": "Docs/Archive/DevelopmentBible.md",
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
    _expect_exact(
        runtime["failure_reason_binding"],
        "bound_runtime"
        if "m01_what_the_ledger_keeps" in RUNTIME_BOUND_FAILURE_MISSIONS
        else "requested",
        "mission.runtime_binding.failure_reason_binding",
    )

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
    _expect_canonical_projection(
        {
            "purpose": canon_record["purpose"],
            "canonical_facts": canon_record["canonical_facts"],
            "prohibited_inferences": canon_record["prohibited_inferences"],
        },
        EXPECTED_M01_CANON_PROSE_SHA256,
        "mission.canon.reviewed_canonical_prose",
    )

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
        expected_speaker = EXPECTED_SPEAKER_RECORDS.get(speaker_id)
        if expected_speaker is None:
            raise NarrativeValidationError(f"{path}.id: speaker is not in the reviewed speaker projection")
        for field, expected_value in expected_speaker.items():
            _expect_exact(
                speaker[field],
                expected_value,
                f"{path}.reviewed_speaker_mapping.{field}",
            )
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
    line_speakers: dict[str, str] = {}
    line_source_texts: dict[str, str] = {}
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
        line_speakers[line_id] = speaker_id
        trigger_id = _expect_string(line["trigger_id"], f"{path}.trigger_id")
        if trigger_id not in trigger_ids:
            raise NarrativeValidationError(f"{path}.trigger_id: unresolved trigger reference")
        line_triggers[line_id] = trigger_id
        _expect_exact(line["delivery_channel"], speaker_channels[speaker_id], f"{path}.delivery_channel")
        source_text = _validate_source_text(line["source_text"], f"{path}.source_text")
        line_source_texts[line_id] = source_text
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

    for line_id, expected_projection in EXPECTED_REVIEWED_BRANCH_LINES.items():
        actual_projection = (
            line_speakers.get(line_id),
            line_source_texts.get(line_id),
        )
        _expect_exact(
            actual_projection,
            expected_projection,
            f"mission.reviewed_branch_lines.{line_id}",
        )

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
            if any(lexeme in folded_text for lexeme in EXPLICIT_MORAL_RANKING_LEXEMES):
                raise NarrativeValidationError(
                    f"{path}.{field_name}: limited explicit moral-ranking lexical guard matched"
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
        reviewed_branch = EXPECTED_REVIEWED_BRANCHES[choice]
        _expect_exact(
            current_behavior,
            reviewed_branch["current_runtime_behavior"],
            f"{path}.reviewed_branch_projection.current_runtime_behavior",
        )
        _expect_exact(
            target_tradeoff,
            reviewed_branch["design_target_tradeoff"],
            f"{path}.reviewed_branch_projection.design_target_tradeoff",
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
        _expect_exact(
            tuple(refs),
            reviewed_branch["dialogue_line_ids"],
            f"{path}.reviewed_branch_projection.dialogue_line_ids",
        )
        _validate_localized_text(
            branch["design_target_choice_ui"],
            f"{path}.design_target_choice_ui",
            content_ids,
            loc_keys,
        )
        _expect_exact(
            branch["design_target_choice_ui"]["source_text"],
            reviewed_branch["design_target_choice_ui_source_text"],
            f"{path}.reviewed_branch_projection.design_target_choice_ui.source_text",
        )
        _expect_exact(branch["binding_status"], "authored_unbound", f"{path}.binding_status")
    _expect_exact(branch_line_counts, {3}, "mission.branch_variants structural parity")
    branch_serialized = json.dumps(branches, sort_keys=True).casefold()
    if "dormant" in branch_serialized:
        raise NarrativeValidationError("mission.branch_variants: Dormant is not a terminal narrative branch")
    if any(lexeme in branch_serialized for lexeme in EXPLICIT_MORAL_RANKING_LEXEMES):
        raise NarrativeValidationError(
            "mission.branch_variants: limited explicit moral-ranking lexical guard matched"
        )

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
        "runtime_consumption": "partial_briefing_lines_results",
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


# ---------------------------------------------------------------------------
# Registered mission contracts (Missions 02-15)
# ---------------------------------------------------------------------------
#
# Each authored mission is pinned here after review, exactly as Mission 01 is
# pinned by the constants above. An authored file without a registry entry, or
# an entry without its file, fails validation. The prose and line pins are
# canonical-projection SHA-256 digests, so any post-review edit to reviewed
# text is detected.

MISSION_REGISTRY: dict[str, dict[str, Any]] = {'m02_seven_accounts_of_rain': {'asset_hooks': {'aud_m02_ambience_route': 'audio',
                                                'vis_m02_account_overlays': 'visual',
                                                'vis_m02_argued_ridge': 'visual',
                                                'vis_m02_migration_basin': 'visual',
                                                'vis_m02_waystone_uproot': 'visual'},
                                'branch_keys': [],
                                'campaign_state_effect': 'A failed Mission 02 attempt appends no '
                                                         'campaign decision record.',
                                'canon_prose_sha256': 'b68a740c8ab5e519ac6632849514bb802380a6f55c6ed8ba3e821085a00747a2',
                                'command_speaker_ids': ['spk_oruun_seven_stones'],
                                'content_id': 'nar_m02_seven_accounts_of_rain',
                                'counts': {'lines': 17,
                                           'objectives': 2,
                                           'sequences': 3,
                                           'shots': 4},
                                'decision_kind': 'none',
                                'decision_trigger_id': None,
                                'failed_trigger_id': 'nar_m02_evt_mission_failed',
                                'failure_reason_codes': ['local_core_lost',
                                                         'memory_bearer_lost',
                                                         'waystone_lost',
                                                         'terminal_match_outcome',
                                                         'generic'],
                                'file': 'm02_seven_accounts_of_rain.json',
                                'lines_projection_sha256': '349ec7b17f3094691ff4c7196952194f6a2743e0747fe1767c20b243d1e9a73c',
                                'mission_id': 'SevenAccountsOfRain',
                                'mission_index': 1,
                                'operation_mode': 'CampaignSevenAccounts',
                                'phases': ['Inactive',
                                           'EstablishWaystone',
                                           'RecallMemory',
                                           'Complete',
                                           'Failed'],
                                'prefix': 'm02',
                                'speakers': {'spk_oruun_seven_stones': {'command_authority': True,
                                                                        'delivery_channel': 'command_radio',
                                                                        'role_in_mission': 'player_command_authority'}},
                                'trigger_prerequisites': {'nar_m02_evt_campaign_result_presented': ['nar_m02_evt_memory_recalled'],
                                                          'nar_m02_evt_memory_recalled': ['nar_m02_evt_waystone_rooted'],
                                                          'nar_m02_evt_mission_failed': ['nar_m02_evt_operation_started'],
                                                          'nar_m02_evt_operation_started': [],
                                                          'nar_m02_evt_retry_requested': ['nar_m02_evt_mission_failed'],
                                                          'nar_m02_evt_waystone_rooted': ['nar_m02_evt_operation_started']},
                                'trigger_signals': {'nar_m02_evt_campaign_result_presented': 'campaign_commit_status_presented',
                                                    'nar_m02_evt_memory_recalled': 'phase_entered:Complete',
                                                    'nar_m02_evt_mission_failed': 'phase_entered:Failed',
                                                    'nar_m02_evt_operation_started': 'operation_ready:CampaignSevenAccounts:EstablishWaystone',
                                                    'nar_m02_evt_retry_requested': 'player_requested_mission_retry',
                                                    'nar_m02_evt_waystone_rooted': 'phase_entered:RecallMemory'}},
 'm03_a_city_on_reserve': {'asset_hooks': {'aud_m03_ambience_reserve': 'audio',
                                           'vis_m03_district_interface': 'visual',
                                           'vis_m03_district_skyline': 'visual',
                                           'vis_m03_grid_schematic': 'visual',
                                           'vis_m03_worker_column': 'visual'},
                           'branch_keys': [],
                           'campaign_state_effect': 'A failed Mission 03 attempt appends no '
                                                    'campaign decision record.',
                           'canon_prose_sha256': '29b5e8a8c082ae15cc2098f47dfb47550831d6ea14bd1147a4fade35eb334b21',
                           'command_speaker_ids': ['spk_mara_vey'],
                           'content_id': 'nar_m03_a_city_on_reserve',
                           'counts': {'lines': 15, 'objectives': 3, 'sequences': 4, 'shots': 4},
                           'decision_kind': 'none',
                           'decision_trigger_id': None,
                           'failed_trigger_id': 'nar_m03_evt_mission_failed',
                           'failure_reason_codes': ['local_core_lost',
                                                    'district_structure_lost',
                                                    'terminal_match_outcome',
                                                    'generic'],
                           'file': 'm03_a_city_on_reserve.json',
                           'lines_projection_sha256': '296d922b3c1d6ada34b66b020c92306274feea481d3bbbd1b34be4ab4a8c8c14',
                           'mission_id': 'ACityOnReserve',
                           'mission_index': 2,
                           'operation_mode': 'CampaignCityReserve',
                           'phases': ['Inactive',
                                      'StabilizePriority',
                                      'StabilizeSecondary',
                                      'StabilizeFinal',
                                      'Complete',
                                      'Failed'],
                           'prefix': 'm03',
                           'speakers': {'spk_mara_vey': {'command_authority': True,
                                                         'delivery_channel': 'command_radio',
                                                         'role_in_mission': 'player_command_authority'}},
                           'trigger_prerequisites': {'nar_m03_evt_campaign_result_presented': ['nar_m03_evt_grid_stabilized'],
                                                     'nar_m03_evt_grid_stabilized': ['nar_m03_evt_secondary_stabilized'],
                                                     'nar_m03_evt_mission_failed': ['nar_m03_evt_operation_started'],
                                                     'nar_m03_evt_operation_started': [],
                                                     'nar_m03_evt_priority_stabilized': ['nar_m03_evt_operation_started'],
                                                     'nar_m03_evt_retry_requested': ['nar_m03_evt_mission_failed'],
                                                     'nar_m03_evt_secondary_stabilized': ['nar_m03_evt_priority_stabilized']},
                           'trigger_signals': {'nar_m03_evt_campaign_result_presented': 'campaign_commit_status_presented',
                                               'nar_m03_evt_grid_stabilized': 'phase_entered:Complete',
                                               'nar_m03_evt_mission_failed': 'phase_entered:Failed',
                                               'nar_m03_evt_operation_started': 'operation_ready:CampaignCityReserve:StabilizePriority',
                                               'nar_m03_evt_priority_stabilized': 'phase_entered:StabilizeSecondary',
                                               'nar_m03_evt_retry_requested': 'player_requested_mission_retry',
                                               'nar_m03_evt_secondary_stabilized': 'phase_entered:StabilizeFinal'}},
 'm04_the_unburied_road': {'asset_hooks': {'aud_m04_ambience_vaults': 'audio',
                                           'vis_m04_record_gap': 'visual',
                                           'vis_m04_three_roads': 'visual',
                                           'vis_m04_vault_ceiling': 'visual',
                                           'vis_m04_waystone_column': 'visual'},
                           'branch_keys': [],
                           'campaign_state_effect': 'A failed Mission 04 attempt appends no '
                                                    'campaign decision record.',
                           'canon_prose_sha256': '471d0cddef92a185f4438ff8fc8282ccafed17b542997709850261269392fe95',
                           'command_speaker_ids': ['spk_oruun_seven_stones'],
                           'content_id': 'nar_m04_the_unburied_road',
                           'counts': {'lines': 16, 'objectives': 3, 'sequences': 4, 'shots': 4},
                           'decision_kind': 'none',
                           'decision_trigger_id': None,
                           'failed_trigger_id': 'nar_m04_evt_mission_failed',
                           'failure_reason_codes': ['local_core_lost',
                                                    'memory_bearer_lost',
                                                    'waystone_lost',
                                                    'terminal_match_outcome',
                                                    'generic'],
                           'file': 'm04_the_unburied_road.json',
                           'lines_projection_sha256': 'a07231420279a29365303205acf436c1abe485ef6f7707b8eda363e560289db3',
                           'mission_id': 'TheUnburiedRoad',
                           'mission_index': 3,
                           'operation_mode': 'CampaignUnburiedRoad',
                           'phases': ['Inactive',
                                      'EstablishRoadhead',
                                      'RaiseListeningSpine',
                                      'RecoverMemoryShard',
                                      'Complete',
                                      'Failed'],
                           'prefix': 'm04',
                           'speakers': {'spk_oruun_seven_stones': {'command_authority': True,
                                                                   'delivery_channel': 'command_radio',
                                                                   'role_in_mission': 'player_command_authority'}},
                           'trigger_prerequisites': {'nar_m04_evt_campaign_result_presented': ['nar_m04_evt_shard_recovered'],
                                                     'nar_m04_evt_mission_failed': ['nar_m04_evt_operation_started'],
                                                     'nar_m04_evt_operation_started': [],
                                                     'nar_m04_evt_retry_requested': ['nar_m04_evt_mission_failed'],
                                                     'nar_m04_evt_roadhead_established': ['nar_m04_evt_operation_started'],
                                                     'nar_m04_evt_shard_recovered': ['nar_m04_evt_spine_raised'],
                                                     'nar_m04_evt_spine_raised': ['nar_m04_evt_roadhead_established']},
                           'trigger_signals': {'nar_m04_evt_campaign_result_presented': 'campaign_commit_status_presented',
                                               'nar_m04_evt_mission_failed': 'phase_entered:Failed',
                                               'nar_m04_evt_operation_started': 'operation_ready:CampaignUnburiedRoad:EstablishRoadhead',
                                               'nar_m04_evt_retry_requested': 'player_requested_mission_retry',
                                               'nar_m04_evt_roadhead_established': 'phase_entered:RaiseListeningSpine',
                                               'nar_m04_evt_shard_recovered': 'phase_entered:Complete',
                                               'nar_m04_evt_spine_raised': 'phase_entered:RecoverMemoryShard'}},
 'm05_terms_of_continuance': {'asset_hooks': {'aud_m05_ambience_accord': 'audio',
                                              'vis_m05_draft_unsigned': 'visual',
                                              'vis_m05_held_field': 'visual',
                                              'vis_m05_two_masts': 'visual',
                                              'vis_m05_witness_walk': 'visual'},
                              'branch_keys': [],
                              'campaign_state_effect': 'A failed Mission 05 attempt appends no '
                                                       'campaign decision record.',
                              'canon_prose_sha256': '49f79f42757f3642fe8bb6d4220ab8b356ffafd5b0eaee2ea4527fa5f4cd0fe8',
                              'command_speaker_ids': ['spk_mara_vey'],
                              'content_id': 'nar_m05_terms_of_continuance',
                              'counts': {'lines': 18, 'objectives': 3, 'sequences': 4, 'shots': 4},
                              'decision_kind': 'none',
                              'decision_trigger_id': None,
                              'failed_trigger_id': 'nar_m05_evt_mission_failed',
                              'failure_reason_codes': ['local_core_lost',
                                                       'meridian_relay_lost',
                                                       'kharuun_spine_lost',
                                                       'witness_lost',
                                                       'continuance_window_compromised',
                                                       'terminal_match_outcome',
                                                       'generic'],
                              'file': 'm05_terms_of_continuance.json',
                              'lines_projection_sha256': '2c309544fb13be595362cbee2e99581abddc88bf21b745c284f63d7dce235073',
                              'mission_id': 'TermsOfContinuance',
                              'mission_index': 4,
                              'operation_mode': 'CampaignTermsOfContinuance',
                              'phases': ['Inactive',
                                         'SynchronizeNetworks',
                                         'HoldContinuanceWindow',
                                         'ExtractWitnesses',
                                         'Complete',
                                         'Failed'],
                              'prefix': 'm05',
                              'speakers': {'spk_mara_vey': {'command_authority': True,
                                                            'delivery_channel': 'command_radio',
                                                            'role_in_mission': 'player_command_authority'}},
                              'trigger_prerequisites': {'nar_m05_evt_campaign_result_presented': ['nar_m05_evt_witnesses_extracted'],
                                                        'nar_m05_evt_mission_failed': ['nar_m05_evt_operation_started'],
                                                        'nar_m05_evt_networks_synchronized': ['nar_m05_evt_operation_started'],
                                                        'nar_m05_evt_operation_started': [],
                                                        'nar_m05_evt_retry_requested': ['nar_m05_evt_mission_failed'],
                                                        'nar_m05_evt_window_held': ['nar_m05_evt_networks_synchronized'],
                                                        'nar_m05_evt_witnesses_extracted': ['nar_m05_evt_window_held']},
                              'trigger_signals': {'nar_m05_evt_campaign_result_presented': 'campaign_commit_status_presented',
                                                  'nar_m05_evt_mission_failed': 'phase_entered:Failed',
                                                  'nar_m05_evt_networks_synchronized': 'phase_entered:HoldContinuanceWindow',
                                                  'nar_m05_evt_operation_started': 'operation_ready:CampaignTermsOfContinuance:SynchronizeNetworks',
                                                  'nar_m05_evt_retry_requested': 'player_requested_mission_retry',
                                                  'nar_m05_evt_window_held': 'phase_entered:ExtractWitnesses',
                                                  'nar_m05_evt_witnesses_extracted': 'phase_entered:Complete'}},
 'm06_names_without_births': {'asset_hooks': {'aud_m06_ambience_records': 'audio',
                                              'vis_m06_erasure_marks': 'visual',
                                              'vis_m06_exposed_residences': 'visual',
                                              'vis_m06_register_void': 'visual',
                                              'vis_m06_trace_overlay': 'visual'},
                              'branch_keys': [],
                              'campaign_state_effect': 'A failed Mission 06 attempt appends no '
                                                       'campaign decision record.',
                              'canon_prose_sha256': '52fd8b0565ad0ac3c4edd7b84e0ac5a3834cd32d4c45344fb77f5d83dff3d4e6',
                              'command_speaker_ids': ['spk_talar_venn'],
                              'content_id': 'nar_m06_names_without_births',
                              'counts': {'lines': 19, 'objectives': 4, 'sequences': 5, 'shots': 4},
                              'decision_kind': 'none',
                              'decision_trigger_id': None,
                              'failed_trigger_id': 'nar_m06_evt_mission_failed',
                              'failure_reason_codes': ['local_core_lost',
                                                       'talar_lost',
                                                       'archive_lost',
                                                       'civilian_proxy_lost',
                                                       'terminal_match_outcome',
                                                       'generic'],
                              'file': 'm06_names_without_births.json',
                              'lines_projection_sha256': '17141f1bec2bc50ee6992e0b917a88e0fa9bf94ecae27c8d2d035e2b265de59a',
                              'mission_id': 'NamesWithoutBirths',
                              'mission_index': 5,
                              'operation_mode': 'CampaignNamesWithoutBirths',
                              'phases': ['Inactive',
                                         'LocateCensus',
                                         'StabilizeArchive',
                                         'ShelterCivilians',
                                         'ExtractEvidence',
                                         'Complete',
                                         'Failed'],
                              'prefix': 'm06',
                              'speakers': {'spk_talar_venn': {'command_authority': True,
                                                              'delivery_channel': 'command_radio',
                                                              'role_in_mission': 'player_command_authority'}},
                              'trigger_prerequisites': {'nar_m06_evt_archive_stabilized': ['nar_m06_evt_census_located'],
                                                        'nar_m06_evt_campaign_result_presented': ['nar_m06_evt_evidence_extracted'],
                                                        'nar_m06_evt_census_located': ['nar_m06_evt_operation_started'],
                                                        'nar_m06_evt_civilians_sheltered': ['nar_m06_evt_archive_stabilized'],
                                                        'nar_m06_evt_evidence_extracted': ['nar_m06_evt_civilians_sheltered'],
                                                        'nar_m06_evt_mission_failed': ['nar_m06_evt_operation_started'],
                                                        'nar_m06_evt_operation_started': [],
                                                        'nar_m06_evt_retry_requested': ['nar_m06_evt_mission_failed']},
                              'trigger_signals': {'nar_m06_evt_archive_stabilized': 'phase_entered:ShelterCivilians',
                                                  'nar_m06_evt_campaign_result_presented': 'campaign_commit_status_presented',
                                                  'nar_m06_evt_census_located': 'phase_entered:StabilizeArchive',
                                                  'nar_m06_evt_civilians_sheltered': 'phase_entered:ExtractEvidence',
                                                  'nar_m06_evt_evidence_extracted': 'phase_entered:Complete',
                                                  'nar_m06_evt_mission_failed': 'phase_entered:Failed',
                                                  'nar_m06_evt_operation_started': 'operation_ready:CampaignNamesWithoutBirths:LocateCensus',
                                                  'nar_m06_evt_retry_requested': 'player_requested_mission_retry'}},
 'm07_the_shape_of_silence': {'asset_hooks': {'aud_m07_ambience_hollow': 'audio',
                                              'vis_m07_corresponding_voids': 'visual',
                                              'vis_m07_processional_column': 'visual',
                                              'vis_m07_quiet_span': 'visual',
                                              'vis_m07_standing_witnesses': 'visual'},
                              'branch_keys': [],
                              'campaign_state_effect': 'A failed Mission 07 attempt appends no '
                                                       'campaign decision record.',
                              'canon_prose_sha256': '50a2445fda2a95de23d8b35a2da54517c2973df46b7ad0e8bf74ae9cc6c1d9a3',
                              'command_speaker_ids': ['spk_oruun_seven_stones'],
                              'content_id': 'nar_m07_the_shape_of_silence',
                              'counts': {'lines': 19, 'objectives': 4, 'sequences': 5, 'shots': 4},
                              'decision_kind': 'none',
                              'decision_trigger_id': None,
                              'failed_trigger_id': 'nar_m07_evt_mission_failed',
                              'failure_reason_codes': ['local_core_lost',
                                                       'oruun_lost',
                                                       'waystone_lost',
                                                       'memory_witness_lost',
                                                       'terminal_match_outcome',
                                                       'generic'],
                              'file': 'm07_the_shape_of_silence.json',
                              'lines_projection_sha256': 'a13a6da51874b2b6467fb28318cc9c7873635e04c44d43dd07281cb6ad09a989',
                              'mission_id': 'TheShapeOfSilence',
                              'mission_index': 6,
                              'operation_mode': 'CampaignShapeOfSilence',
                              'phases': ['Inactive',
                                         'RootWaystone',
                                         'RaiseListeningSpine',
                                         'PositionMemoryWitnesses',
                                         'ReachConfluence',
                                         'Complete',
                                         'Failed'],
                              'prefix': 'm07',
                              'speakers': {'spk_oruun_seven_stones': {'command_authority': True,
                                                                      'delivery_channel': 'command_radio',
                                                                      'role_in_mission': 'player_command_authority'}},
                              'trigger_prerequisites': {'nar_m07_evt_campaign_result_presented': ['nar_m07_evt_confluence_reached'],
                                                        'nar_m07_evt_confluence_reached': ['nar_m07_evt_witnesses_positioned'],
                                                        'nar_m07_evt_mission_failed': ['nar_m07_evt_operation_started'],
                                                        'nar_m07_evt_operation_started': [],
                                                        'nar_m07_evt_retry_requested': ['nar_m07_evt_mission_failed'],
                                                        'nar_m07_evt_spine_raised': ['nar_m07_evt_waystone_rooted'],
                                                        'nar_m07_evt_waystone_rooted': ['nar_m07_evt_operation_started'],
                                                        'nar_m07_evt_witnesses_positioned': ['nar_m07_evt_spine_raised']},
                              'trigger_signals': {'nar_m07_evt_campaign_result_presented': 'campaign_commit_status_presented',
                                                  'nar_m07_evt_confluence_reached': 'phase_entered:Complete',
                                                  'nar_m07_evt_mission_failed': 'phase_entered:Failed',
                                                  'nar_m07_evt_operation_started': 'operation_ready:CampaignShapeOfSilence:RootWaystone',
                                                  'nar_m07_evt_retry_requested': 'player_requested_mission_retry',
                                                  'nar_m07_evt_spine_raised': 'phase_entered:PositionMemoryWitnesses',
                                                  'nar_m07_evt_waystone_rooted': 'phase_entered:RaiseListeningSpine',
                                                  'nar_m07_evt_witnesses_positioned': 'phase_entered:ReachConfluence'}},
 'm08_the_shape_beside_us': {'asset_hooks': {'aud_m08_ambience_overlap': 'audio',
                                             'vis_m08_doubled_ground': 'visual',
                                             'vis_m08_first_echo': 'visual',
                                             'vis_m08_proxy_file': 'visual',
                                             'vis_m08_repeating_edge': 'visual'},
                             'branch_keys': [],
                             'campaign_state_effect': 'A failed Mission 08 attempt appends no '
                                                      'campaign decision record.',
                             'canon_prose_sha256': '9bb89dedde77373f61d8b8a660bb06d8c6865791492b7b74fb899eb5a3e9ceba',
                             'command_speaker_ids': ['spk_talar_venn'],
                             'content_id': 'nar_m08_the_shape_beside_us',
                             'counts': {'lines': 18, 'objectives': 4, 'sequences': 5, 'shots': 4},
                             'decision_kind': 'none',
                             'decision_trigger_id': None,
                             'failed_trigger_id': 'nar_m08_evt_mission_failed',
                             'failure_reason_codes': ['local_core_lost',
                                                      'talar_lost',
                                                      'state_witness_lost',
                                                      'terminal_match_outcome',
                                                      'generic'],
                             'file': 'm08_the_shape_beside_us.json',
                             'lines_projection_sha256': 'd918056ed7b5c956edbb21d27bfa56ed761ba46c98345a194a7939f34f5882e5',
                             'mission_id': 'TheShapeBesideUs',
                             'mission_index': 7,
                             'operation_mode': 'CampaignShapeBesideUs',
                             'phases': ['Inactive',
                                        'ReachFirstEcho',
                                        'RaiseEchoRelay',
                                        'TraversePairedStates',
                                        'ReachConvergence',
                                        'Complete',
                                        'Failed'],
                             'prefix': 'm08',
                             'speakers': {'spk_neme': {'command_authority': False,
                                                       'delivery_channel': 'cross_faction_radio',
                                                       'role_in_mission': 'reciprocal_choir_contact'},
                                          'spk_talar_venn': {'command_authority': True,
                                                             'delivery_channel': 'command_radio',
                                                             'role_in_mission': 'player_command_authority'}},
                             'trigger_prerequisites': {'nar_m08_evt_campaign_result_presented': ['nar_m08_evt_convergence_reached'],
                                                       'nar_m08_evt_convergence_reached': ['nar_m08_evt_states_traversed'],
                                                       'nar_m08_evt_first_echo_observed': ['nar_m08_evt_operation_started'],
                                                       'nar_m08_evt_mission_failed': ['nar_m08_evt_operation_started'],
                                                       'nar_m08_evt_operation_started': [],
                                                       'nar_m08_evt_relay_raised': ['nar_m08_evt_first_echo_observed'],
                                                       'nar_m08_evt_retry_requested': ['nar_m08_evt_mission_failed'],
                                                       'nar_m08_evt_states_traversed': ['nar_m08_evt_relay_raised']},
                             'trigger_signals': {'nar_m08_evt_campaign_result_presented': 'campaign_commit_status_presented',
                                                 'nar_m08_evt_convergence_reached': 'phase_entered:Complete',
                                                 'nar_m08_evt_first_echo_observed': 'phase_entered:RaiseEchoRelay',
                                                 'nar_m08_evt_mission_failed': 'phase_entered:Failed',
                                                 'nar_m08_evt_operation_started': 'operation_ready:CampaignShapeBesideUs:ReachFirstEcho',
                                                 'nar_m08_evt_relay_raised': 'phase_entered:TraversePairedStates',
                                                 'nar_m08_evt_retry_requested': 'player_requested_mission_retry',
                                                 'nar_m08_evt_states_traversed': 'phase_entered:ReachConvergence'}},
 'm09_reserve_authority': {'asset_hooks': {'aud_m09_ambience_reserve_hall': 'audio',
                                           'vis_m09_advisory_overlay': 'visual',
                                           'vis_m09_allocation_console': 'visual',
                                           'vis_m09_reserve_gauges': 'visual',
                                           'vis_m09_reserve_street': 'visual'},
                           'branch_keys': [],
                           'campaign_state_effect': 'A failed Mission 09 attempt appends no '
                                                    'campaign decision record.',
                           'canon_prose_sha256': 'e09f9ab4fe89d6d2ac573a76cf80dfb05e1af0b36a047b8473e47e16d2f0d17b',
                           'command_speaker_ids': ['spk_mara_vey'],
                           'content_id': 'nar_m09_reserve_authority',
                           'counts': {'lines': 17, 'objectives': 4, 'sequences': 5, 'shots': 4},
                           'decision_kind': 'none',
                           'decision_trigger_id': None,
                           'failed_trigger_id': 'nar_m09_evt_mission_failed',
                           'failure_reason_codes': ['local_core_lost',
                                                    'mara_lost',
                                                    'district_structure_lost',
                                                    'terminal_match_outcome',
                                                    'generic'],
                           'file': 'm09_reserve_authority.json',
                           'lines_projection_sha256': '7689b25f603f7539203b5b9af58d90532bb07930858c51f6fa45fc349f2d3405',
                           'mission_id': 'ReserveAuthority',
                           'mission_index': 8,
                           'operation_mode': 'CampaignReserveAuthority',
                           'phases': ['Inactive',
                                      'SecureAuthority',
                                      'AllocateFirstDistrict',
                                      'AllocateSecondDistrict',
                                      'ReachDeferredDistrict',
                                      'Complete',
                                      'Failed'],
                           'prefix': 'm09',
                           'speakers': {'spk_mara_vey': {'command_authority': True,
                                                         'delivery_channel': 'command_radio',
                                                         'role_in_mission': 'player_command_authority'}},
                           'trigger_prerequisites': {'nar_m09_evt_authority_secured': ['nar_m09_evt_operation_started'],
                                                     'nar_m09_evt_campaign_result_presented': ['nar_m09_evt_deferred_confirmed'],
                                                     'nar_m09_evt_deferred_confirmed': ['nar_m09_evt_second_allocated'],
                                                     'nar_m09_evt_first_allocated': ['nar_m09_evt_authority_secured'],
                                                     'nar_m09_evt_mission_failed': ['nar_m09_evt_operation_started'],
                                                     'nar_m09_evt_operation_started': [],
                                                     'nar_m09_evt_retry_requested': ['nar_m09_evt_mission_failed'],
                                                     'nar_m09_evt_second_allocated': ['nar_m09_evt_first_allocated']},
                           'trigger_signals': {'nar_m09_evt_authority_secured': 'phase_entered:AllocateFirstDistrict',
                                               'nar_m09_evt_campaign_result_presented': 'campaign_commit_status_presented',
                                               'nar_m09_evt_deferred_confirmed': 'phase_entered:Complete',
                                               'nar_m09_evt_first_allocated': 'phase_entered:AllocateSecondDistrict',
                                               'nar_m09_evt_mission_failed': 'phase_entered:Failed',
                                               'nar_m09_evt_operation_started': 'operation_ready:CampaignReserveAuthority:SecureAuthority',
                                               'nar_m09_evt_retry_requested': 'player_requested_mission_retry',
                                               'nar_m09_evt_second_allocated': 'phase_entered:ReachDeferredDistrict'}},
 'm10_the_choir_at_lume_reach': {'asset_hooks': {'aud_m10_ambience_reach': 'audio',
                                                 'vis_m10_lume_reach_evening': 'visual',
                                                 'vis_m10_lume_well_offers': 'visual',
                                                 'vis_m10_quarantine_line': 'visual',
                                                 'vis_m10_standing_liability': 'visual'},
                                 'branch_keys': ['Harvest', 'Preserve', 'Reshape'],
                                 'branches_projection_sha256': '81cd22f038f651c6ed1a43d2da5c627c7ac8b057a3bbea53738144935a74bca5',
                                 'campaign_state_effect': 'A failed Mission 10 attempt appends no '
                                                          'campaign decision record.',
                                 'canon_prose_sha256': 'be11f78d77a55ea6fefb8d1653fa921d82818d74776964f731706d96a62afad4',
                                 'command_speaker_ids': ['spk_oruun_seven_stones'],
                                 'content_id': 'nar_m10_the_choir_at_lume_reach',
                                 'counts': {'lines': 27,
                                            'objectives': 6,
                                            'sequences': 6,
                                            'shots': 4},
                                 'decision_kind': 'well_choice',
                                 'decision_trigger_id': 'nar_m10_evt_protocol_committed',
                                 'failed_trigger_id': 'nar_m10_evt_mission_failed',
                                 'failure_reason_codes': ['local_core_lost',
                                                          'oruun_lost',
                                                          'waystone_lost',
                                                          'future_well_lost',
                                                          'reshape_window_expired',
                                                          'terminal_match_outcome',
                                                          'generic'],
                                 'file': 'm10_the_choir_at_lume_reach.json',
                                 'lines_projection_sha256': '9b52541087503b085dda4ca685979685d8f3d0df37b58fabd02b68549dbccf3e',
                                 'mission_id': 'ChoirAtLumeReach',
                                 'mission_index': 9,
                                 'operation_mode': 'CampaignChoirAtLumeReach',
                                 'phases': ['Inactive',
                                            'EstablishContact',
                                            'ResolveDeferredLiability',
                                            'RaiseFirstAnchor',
                                            'RaiseSecondAnchor',
                                            'CommitFutureWell',
                                            'ResolveFutureWell',
                                            'Complete',
                                            'Failed'],
                                 'prefix': 'm10',
                                 'speakers': {'spk_mara_vey': {'command_authority': False,
                                                               'delivery_channel': 'operations_radio',
                                                               'role_in_mission': 'offmap_meridian_liaison'},
                                              'spk_oruun_seven_stones': {'command_authority': True,
                                                                         'delivery_channel': 'command_radio',
                                                                         'role_in_mission': 'player_command_authority'}},
                                 'trigger_prerequisites': {'nar_m10_evt_campaign_result_presented': ['nar_m10_evt_protocol_resolved'],
                                                           'nar_m10_evt_contact_established': ['nar_m10_evt_operation_started'],
                                                           'nar_m10_evt_first_anchor_raised': ['nar_m10_evt_liability_resolved'],
                                                           'nar_m10_evt_liability_resolved': ['nar_m10_evt_contact_established'],
                                                           'nar_m10_evt_mission_failed': ['nar_m10_evt_operation_started'],
                                                           'nar_m10_evt_operation_started': [],
                                                           'nar_m10_evt_protocol_committed': ['nar_m10_evt_second_anchor_raised'],
                                                           'nar_m10_evt_protocol_resolved': ['nar_m10_evt_protocol_committed'],
                                                           'nar_m10_evt_retry_requested': ['nar_m10_evt_mission_failed'],
                                                           'nar_m10_evt_second_anchor_raised': ['nar_m10_evt_first_anchor_raised']},
                                 'trigger_signals': {'nar_m10_evt_campaign_result_presented': 'campaign_commit_status_presented',
                                                     'nar_m10_evt_contact_established': 'phase_entered:ResolveDeferredLiability',
                                                     'nar_m10_evt_first_anchor_raised': 'phase_entered:RaiseSecondAnchor',
                                                     'nar_m10_evt_liability_resolved': 'phase_entered:RaiseFirstAnchor',
                                                     'nar_m10_evt_mission_failed': 'phase_entered:Failed',
                                                     'nar_m10_evt_operation_started': 'operation_ready:CampaignChoirAtLumeReach:EstablishContact',
                                                     'nar_m10_evt_protocol_committed': 'phase_entered:ResolveFutureWell',
                                                     'nar_m10_evt_protocol_resolved': 'phase_entered:Complete',
                                                     'nar_m10_evt_retry_requested': 'player_requested_mission_retry',
                                                     'nar_m10_evt_second_anchor_raised': 'phase_entered:CommitFutureWell'}},
 'm11_no_neutral_ledger': {'asset_hooks': {'aud_m11_ambience_ledger': 'audio',
                                           'vis_m11_paired_interfaces': 'visual',
                                           'vis_m11_plan_lattice': 'visual',
                                           'vis_m11_recorded_well': 'visual',
                                           'vis_m11_ten_records': 'visual'},
                           'branch_keys': [],
                           'campaign_state_effect': 'A failed Mission 11 attempt appends no '
                                                    'campaign decision record.',
                           'canon_prose_sha256': 'a3d23b51f53d1aa3ed3819a9a52326a5aca0b3d1d3971f2e35001f2cead33b42',
                           'command_speaker_ids': ['spk_oruun_seven_stones'],
                           'content_id': 'nar_m11_no_neutral_ledger',
                           'counts': {'lines': 25, 'objectives': 5, 'sequences': 6, 'shots': 4},
                           'decision_kind': 'none',
                           'decision_trigger_id': None,
                           'failed_trigger_id': 'nar_m11_evt_mission_failed',
                           'failure_reason_codes': ['local_core_lost',
                                                    'oruun_lost',
                                                    'waystone_lost',
                                                    'ledger_witness_lost',
                                                    'future_well_lost',
                                                    'public_interface_lost',
                                                    'conflicting_protocol_applied',
                                                    'reshape_window_expired',
                                                    'terminal_match_outcome',
                                                    'generic'],
                           'file': 'm11_no_neutral_ledger.json',
                           'lines_projection_sha256': 'ace51659bffbadea9a6bcd9256db2bee5ff7cc412324b4b752c4e79b18cfb2ee',
                           'mission_id': 'NoNeutralLedger',
                           'mission_index': 10,
                           'operation_mode': 'CampaignNoNeutralLedger',
                           'phases': ['Inactive',
                                      'SecureInheritedRoute',
                                      'IntegrateDistrictContributions',
                                      'AttestEvidenceChannels',
                                      'ApplyRecordedProtocol',
                                      'RallyCoalition',
                                      'Complete',
                                      'Failed'],
                           'prefix': 'm11',
                           'speakers': {'spk_oruun_seven_stones': {'command_authority': True,
                                                                   'delivery_channel': 'command_radio',
                                                                   'role_in_mission': 'player_command_authority'}},
                           'trigger_prerequisites': {'nar_m11_evt_campaign_result_presented': ['nar_m11_evt_coalition_rallied'],
                                                     'nar_m11_evt_coalition_rallied': ['nar_m11_evt_protocol_applied'],
                                                     'nar_m11_evt_districts_integrated': ['nar_m11_evt_route_secured'],
                                                     'nar_m11_evt_evidence_attested': ['nar_m11_evt_districts_integrated'],
                                                     'nar_m11_evt_mission_failed': ['nar_m11_evt_operation_started'],
                                                     'nar_m11_evt_operation_started': [],
                                                     'nar_m11_evt_protocol_applied': ['nar_m11_evt_evidence_attested'],
                                                     'nar_m11_evt_retry_requested': ['nar_m11_evt_mission_failed'],
                                                     'nar_m11_evt_route_secured': ['nar_m11_evt_operation_started']},
                           'trigger_signals': {'nar_m11_evt_campaign_result_presented': 'campaign_commit_status_presented',
                                               'nar_m11_evt_coalition_rallied': 'phase_entered:Complete',
                                               'nar_m11_evt_districts_integrated': 'phase_entered:AttestEvidenceChannels',
                                               'nar_m11_evt_evidence_attested': 'phase_entered:ApplyRecordedProtocol',
                                               'nar_m11_evt_mission_failed': 'phase_entered:Failed',
                                               'nar_m11_evt_operation_started': 'operation_ready:CampaignNoNeutralLedger:SecureInheritedRoute',
                                               'nar_m11_evt_protocol_applied': 'phase_entered:RallyCoalition',
                                               'nar_m11_evt_retry_requested': 'player_requested_mission_retry',
                                               'nar_m11_evt_route_secured': 'phase_entered:IntegrateDistrictContributions'}},
 'm12_the_future_that_won': {'asset_hooks': {'aud_m12_ambience_plaza': 'audio',
                                             'vis_m12_audit_column': 'visual',
                                             'vis_m12_demonstrator_plaza': 'visual',
                                             'vis_m12_paired_readbacks': 'visual',
                                             'vis_m12_separate_well': 'visual'},
                             'branch_keys': [],
                             'campaign_state_effect': 'A failed Mission 12 attempt appends no '
                                                      'campaign decision record.',
                             'canon_prose_sha256': '9fc4c82056f0c5ec88acf9e66c09ee36f85502732ab7369678c1cf0ac517ff0d',
                             'command_speaker_ids': ['spk_oruun_seven_stones'],
                             'content_id': 'nar_m12_the_future_that_won',
                             'counts': {'lines': 23, 'objectives': 5, 'sequences': 6, 'shots': 4},
                             'decision_kind': 'none',
                             'decision_trigger_id': None,
                             'failed_trigger_id': 'nar_m12_evt_mission_failed',
                             'failure_reason_codes': ['local_core_lost',
                                                      'oruun_lost',
                                                      'verifier_lost',
                                                      'future_well_lost',
                                                      'public_interface_lost',
                                                      'conflicting_protocol_bound',
                                                      'terminal_match_outcome',
                                                      'generic'],
                             'file': 'm12_the_future_that_won.json',
                             'lines_projection_sha256': '039d400e287ee77d1f66d46b48812ee47f55778cad1407faadb6d723b58764ff',
                             'mission_id': 'TheFutureThatWon',
                             'mission_index': 11,
                             'operation_mode': 'CampaignFutureThatWon',
                             'phases': ['Inactive',
                                        'EstablishIndependentReadback',
                                        'VerifyRecordedInputs',
                                        'BindRecordedProtocol',
                                        'HoldStabilityWindow',
                                        'ObserveDistrictReadbacks',
                                        'Complete',
                                        'Failed'],
                             'prefix': 'm12',
                             'speakers': {'spk_cael_rhyse': {'command_authority': False,
                                                             'delivery_channel': 'public_address',
                                                             'role_in_mission': 'neutral_public_demonstrator_voice'},
                                          'spk_oruun_seven_stones': {'command_authority': True,
                                                                     'delivery_channel': 'command_radio',
                                                                     'role_in_mission': 'player_command_authority'}},
                             'trigger_prerequisites': {'nar_m12_evt_campaign_result_presented': ['nar_m12_evt_readbacks_observed'],
                                                       'nar_m12_evt_inputs_verified': ['nar_m12_evt_readback_established'],
                                                       'nar_m12_evt_mission_failed': ['nar_m12_evt_operation_started'],
                                                       'nar_m12_evt_operation_started': [],
                                                       'nar_m12_evt_protocol_bound': ['nar_m12_evt_inputs_verified'],
                                                       'nar_m12_evt_readback_established': ['nar_m12_evt_operation_started'],
                                                       'nar_m12_evt_readbacks_observed': ['nar_m12_evt_window_held'],
                                                       'nar_m12_evt_retry_requested': ['nar_m12_evt_mission_failed'],
                                                       'nar_m12_evt_window_held': ['nar_m12_evt_protocol_bound']},
                             'trigger_signals': {'nar_m12_evt_campaign_result_presented': 'campaign_commit_status_presented',
                                                 'nar_m12_evt_inputs_verified': 'phase_entered:BindRecordedProtocol',
                                                 'nar_m12_evt_mission_failed': 'phase_entered:Failed',
                                                 'nar_m12_evt_operation_started': 'operation_ready:CampaignFutureThatWon:EstablishIndependentReadback',
                                                 'nar_m12_evt_protocol_bound': 'phase_entered:HoldStabilityWindow',
                                                 'nar_m12_evt_readback_established': 'phase_entered:VerifyRecordedInputs',
                                                 'nar_m12_evt_readbacks_observed': 'phase_entered:Complete',
                                                 'nar_m12_evt_retry_requested': 'player_requested_mission_retry',
                                                 'nar_m12_evt_window_held': 'phase_entered:ObserveDistrictReadbacks'}},
 'm13_assembly_of_the_missing': {'asset_hooks': {'aud_m13_ambience_assembly': 'audio',
                                                 'vis_m13_converging_absences': 'visual',
                                                 'vis_m13_index_link': 'visual',
                                                 'vis_m13_public_records': 'visual',
                                                 'vis_m13_separate_witnesses': 'visual'},
                                 'branch_keys': [],
                                 'campaign_state_effect': 'A failed Mission 13 attempt appends no '
                                                          'campaign decision record.',
                                 'canon_prose_sha256': '7106b6c7e886a12722ff3e9001f0888498a2f761b1dab56b89e64dcbb1dc1b30',
                                 'command_speaker_ids': ['spk_oruun_seven_stones'],
                                 'content_id': 'nar_m13_assembly_of_the_missing',
                                 'counts': {'lines': 17,
                                            'objectives': 3,
                                            'sequences': 4,
                                            'shots': 4},
                                 'decision_kind': 'none',
                                 'decision_trigger_id': None,
                                 'failed_trigger_id': 'nar_m13_evt_mission_failed',
                                 'failure_reason_codes': ['local_core_lost',
                                                          'oruun_lost',
                                                          'verifier_lost',
                                                          'public_interface_lost',
                                                          'terminal_match_outcome',
                                                          'generic'],
                                 'file': 'm13_assembly_of_the_missing.json',
                                 'lines_projection_sha256': 'e9a3bbb46984ca23fdf02986668efc980251a365d754fe8fb27dc819f52cedac',
                                 'mission_id': 'AssemblyOfTheMissing',
                                 'mission_index': 12,
                                 'operation_mode': 'CampaignAssemblyOfTheMissing',
                                 'phases': ['Inactive',
                                            'EstablishPublicRecordReadback',
                                            'LinkCrownfallIndex',
                                            'ObserveAssembly',
                                            'Complete',
                                            'Failed'],
                                 'prefix': 'm13',
                                 'speakers': {'spk_oruun_seven_stones': {'command_authority': True,
                                                                         'delivery_channel': 'command_radio',
                                                                         'role_in_mission': 'player_command_authority'}},
                                 'trigger_prerequisites': {'nar_m13_evt_assembly_observed': ['nar_m13_evt_index_linked'],
                                                           'nar_m13_evt_campaign_result_presented': ['nar_m13_evt_assembly_observed'],
                                                           'nar_m13_evt_index_linked': ['nar_m13_evt_readback_established'],
                                                           'nar_m13_evt_mission_failed': ['nar_m13_evt_operation_started'],
                                                           'nar_m13_evt_operation_started': [],
                                                           'nar_m13_evt_readback_established': ['nar_m13_evt_operation_started'],
                                                           'nar_m13_evt_retry_requested': ['nar_m13_evt_mission_failed']},
                                 'trigger_signals': {'nar_m13_evt_assembly_observed': 'phase_entered:Complete',
                                                     'nar_m13_evt_campaign_result_presented': 'campaign_commit_status_presented',
                                                     'nar_m13_evt_index_linked': 'phase_entered:ObserveAssembly',
                                                     'nar_m13_evt_mission_failed': 'phase_entered:Failed',
                                                     'nar_m13_evt_operation_started': 'operation_ready:CampaignAssemblyOfTheMissing:EstablishPublicRecordReadback',
                                                     'nar_m13_evt_readback_established': 'phase_entered:LinkCrownfallIndex',
                                                     'nar_m13_evt_retry_requested': 'player_requested_mission_retry'}},
 'm14_several_voices_one_command': {'asset_hooks': {'aud_m14_ambience_crisis': 'audio',
                                                    'vis_m14_choir_formation': 'visual',
                                                    'vis_m14_crisis_socket': 'visual',
                                                    'vis_m14_research_loom': 'visual',
                                                    'vis_m14_separate_sites': 'visual'},
                                    'branch_keys': [],
                                    'campaign_state_effect': 'A failed Mission 14 attempt appends '
                                                             'no campaign decision record.',
                                    'canon_prose_sha256': 'f3b20131f58be1c7103e45f700ed1bf680b9e37f7482c87820b1ef558bce92d2',
                                    'command_speaker_ids': ['spk_neme'],
                                    'content_id': 'nar_m14_several_voices_one_command',
                                    'counts': {'lines': 22,
                                               'objectives': 5,
                                               'sequences': 6,
                                               'shots': 4},
                                    'decision_kind': 'none',
                                    'decision_trigger_id': None,
                                    'failed_trigger_id': 'nar_m14_evt_mission_failed',
                                    'failure_reason_codes': ['local_core_lost',
                                                             'protected_voice_lost',
                                                             'neme_lost',
                                                             'research_loom_lost',
                                                             'crisis_contract_breached',
                                                             'terminal_match_outcome',
                                                             'generic'],
                                    'file': 'm14_several_voices_one_command.json',
                                    'lines_projection_sha256': '6c9d6d8b819df04f662d74abb0c432a48f13767dca934840721f972af4f82ddf',
                                    'mission_id': 'SeveralVoicesOneCommand',
                                    'mission_index': 13,
                                    'operation_mode': 'CampaignSeveralVoicesOneCommand',
                                    'phases': ['Inactive',
                                               'ResearchHeldAlternatives',
                                               'ResolveIncompatibleVoices',
                                               'ResearchSharedResolution',
                                               'AnchorCrisis',
                                               'HoldSharedResolution',
                                               'Complete',
                                               'Failed'],
                                    'prefix': 'm14',
                                    'speakers': {'spk_neme': {'command_authority': True,
                                                              'delivery_channel': 'command_radio',
                                                              'role_in_mission': 'player_command_authority'}},
                                    'trigger_prerequisites': {'nar_m14_evt_alternatives_researched': ['nar_m14_evt_operation_started'],
                                                              'nar_m14_evt_campaign_result_presented': ['nar_m14_evt_resolution_held'],
                                                              'nar_m14_evt_crisis_anchored': ['nar_m14_evt_resolution_researched'],
                                                              'nar_m14_evt_mission_failed': ['nar_m14_evt_operation_started'],
                                                              'nar_m14_evt_operation_started': [],
                                                              'nar_m14_evt_resolution_held': ['nar_m14_evt_crisis_anchored'],
                                                              'nar_m14_evt_resolution_researched': ['nar_m14_evt_voices_resolved'],
                                                              'nar_m14_evt_retry_requested': ['nar_m14_evt_mission_failed'],
                                                              'nar_m14_evt_voices_resolved': ['nar_m14_evt_alternatives_researched']},
                                    'trigger_signals': {'nar_m14_evt_alternatives_researched': 'phase_entered:ResolveIncompatibleVoices',
                                                        'nar_m14_evt_campaign_result_presented': 'campaign_commit_status_presented',
                                                        'nar_m14_evt_crisis_anchored': 'phase_entered:HoldSharedResolution',
                                                        'nar_m14_evt_mission_failed': 'phase_entered:Failed',
                                                        'nar_m14_evt_operation_started': 'operation_ready:CampaignSeveralVoicesOneCommand:ResearchHeldAlternatives',
                                                        'nar_m14_evt_resolution_held': 'phase_entered:Complete',
                                                        'nar_m14_evt_resolution_researched': 'phase_entered:AnchorCrisis',
                                                        'nar_m14_evt_retry_requested': 'player_requested_mission_retry',
                                                        'nar_m14_evt_voices_resolved': 'phase_entered:ResearchSharedResolution'}},
 'm15_the_broken_sun': {'asset_hooks': {'aud_m15_ambience_crownfall': 'audio',
                                        'vis_m15_approach_path': 'visual',
                                        'vis_m15_crownfall_sky': 'visual',
                                        'vis_m15_open_register': 'visual',
                                        'vis_m15_witness_stations': 'visual'},
                        'branch_keys': ['Restoration',
                                        'ControlledStabilization',
                                        'Extinguishment',
                                        'OpenEvolution'],
                        'branches_projection_sha256': '28f6da0a22742db3fb5b7b5152fd291f2d51212cf7bedf99ae3171c1ccffc34e',
                        'campaign_state_effect': 'A failed Mission 15 attempt appends no campaign '
                                                 'decision record.',
                        'canon_prose_sha256': '0add271beb5b60f04e837e0c5f7558aaf4fb35519295420ad6a49cc504d83a3d',
                        'command_speaker_ids': ['spk_neme'],
                        'content_id': 'nar_m15_the_broken_sun',
                        'counts': {'lines': 27, 'objectives': 5, 'sequences': 5, 'shots': 4},
                        'decision_kind': 'final_resolution',
                        'decision_trigger_id': 'nar_m15_evt_resolution_selected',
                        'failed_trigger_id': 'nar_m15_evt_mission_failed',
                        'failure_reason_codes': ['local_core_lost',
                                                 'protected_witness_lost',
                                                 'command_force_lost',
                                                 'resolution_contract_breached',
                                                 'terminal_match_outcome',
                                                 'generic'],
                        'file': 'm15_the_broken_sun.json',
                        'lines_projection_sha256': 'a3e3d5f85a049641664d627aba9ae8032b35c7a95d8da3b3e4830f3b95061928',
                        'mission_id': 'TheBrokenSun',
                        'mission_index': 14,
                        'operation_mode': 'CampaignTheBrokenSun',
                        'phases': ['Inactive',
                                   'SecureCrownfallApproach',
                                   'AssembleAccord',
                                   'ChooseFinalResolution',
                                   'RaiseResolutionConduit',
                                   'HoldFinalResolution',
                                   'Complete',
                                   'Failed'],
                        'prefix': 'm15',
                        'speakers': {'spk_mara_vey': {'command_authority': False,
                                                      'delivery_channel': 'operations_radio',
                                                      'role_in_mission': 'protected_neutral_witness'},
                                     'spk_neme': {'command_authority': True,
                                                  'delivery_channel': 'command_radio',
                                                  'role_in_mission': 'player_command_authority'},
                                     'spk_oruun_seven_stones': {'command_authority': False,
                                                                'delivery_channel': 'cross_faction_radio',
                                                                'role_in_mission': 'protected_neutral_witness'},
                                     'spk_talar_venn': {'command_authority': False,
                                                        'delivery_channel': 'operations_radio',
                                                        'role_in_mission': 'protected_record_keeper'}},
                        'trigger_prerequisites': {'nar_m15_evt_accord_assembled': ['nar_m15_evt_approach_secured'],
                                                  'nar_m15_evt_approach_secured': ['nar_m15_evt_operation_started'],
                                                  'nar_m15_evt_campaign_result_presented': ['nar_m15_evt_resolution_held'],
                                                  'nar_m15_evt_conduit_raised': ['nar_m15_evt_resolution_selected'],
                                                  'nar_m15_evt_mission_failed': ['nar_m15_evt_operation_started'],
                                                  'nar_m15_evt_operation_started': [],
                                                  'nar_m15_evt_resolution_held': ['nar_m15_evt_conduit_raised'],
                                                  'nar_m15_evt_resolution_selected': ['nar_m15_evt_accord_assembled'],
                                                  'nar_m15_evt_retry_requested': ['nar_m15_evt_mission_failed']},
                        'trigger_signals': {'nar_m15_evt_accord_assembled': 'phase_entered:ChooseFinalResolution',
                                            'nar_m15_evt_approach_secured': 'phase_entered:AssembleAccord',
                                            'nar_m15_evt_campaign_result_presented': 'campaign_commit_status_presented',
                                            'nar_m15_evt_conduit_raised': 'phase_entered:HoldFinalResolution',
                                            'nar_m15_evt_mission_failed': 'phase_entered:Failed',
                                            'nar_m15_evt_operation_started': 'operation_ready:CampaignTheBrokenSun:SecureCrownfallApproach',
                                            'nar_m15_evt_resolution_held': 'phase_entered:Complete',
                                            'nar_m15_evt_resolution_selected': 'phase_entered:RaiseResolutionConduit',
                                            'nar_m15_evt_retry_requested': 'player_requested_mission_retry'}}}


def _validate_canon_value(value: Any, path: str) -> None:
    """Mission-specific canon fields carry strings, numbers, booleans, tile
    objects, or lists of those. Anything else fails closed."""
    if isinstance(value, str):
        _validate_source_text(value, path)
    elif isinstance(value, bool) or isinstance(value, int):
        return
    elif isinstance(value, dict):
        if set(value.keys()) == {"x", "y"}:
            _expect_int(value["x"], f"{path}.x")
            _expect_int(value["y"], f"{path}.y")
        else:
            for key, entry in value.items():
                _expect_symbol(key, f"{path}.{key}")
                _validate_canon_value(entry, f"{path}.{key}")
    elif isinstance(value, list):
        for index, entry in enumerate(value):
            _validate_canon_value(entry, f"{path}[{index}]")
    else:
        raise NarrativeValidationError(f"{path}: unsupported canon value type")


def validate_registered_mission_contract(
    value: dict[str, Any],
    canon: dict[str, Any],
    entry: dict[str, Any],
) -> dict[str, int]:
    """Validate one registered Mission 02-15 authored-source contract."""
    prefix = entry["prefix"]
    patterns = _mission_prefix_patterns(prefix)

    top = _exact_keys(
        value,
        {"schema_version", "namespace", "content_id", "metadata", "runtime_binding", "canon", "speakers", "triggers", "ui_copy", "lines", "dialogue_sequences", "branch_variants", "result_variants", "failure_retry", "cinematic", "asset_hooks", "projections", "implementation"},
        "mission",
    )
    _expect_exact(top["schema_version"], 2, "mission.schema_version")
    _expect_exact(top["namespace"], "echoes.narrative", "mission.namespace")
    _expect_exact(top["content_id"], entry["content_id"], "mission.content_id")

    metadata = _exact_keys(
        top["metadata"],
        {"author", "creator", "source_locale", "rights_status", "originality_review_status", "authority_path", "content_status"},
        "mission.metadata",
    )
    for key, expected in {
        "author": "Angelis Pseftis",
        "creator": "Angelis Pseftis",
        "source_locale": "en-US",
        "rights_status": "author_attributed_project_source",
        "originality_review_status": "human_review_required",
        "authority_path": "Docs/Archive/DevelopmentBible.md",
        "content_status": "authored_source_only",
    }.items():
        _expect_exact(metadata[key], expected, f"mission.metadata.{key}")

    runtime = _exact_keys(
        top["runtime_binding"],
        {"mission_id", "operation_mode", "phases", "well_choices", "well_uncommitted_state", "campaign_commit_statuses", "runtime_consumed", "localization_runtime_status", "failure_reason_binding"},
        "mission.runtime_binding",
    )
    _expect_exact(runtime["mission_id"], entry["mission_id"], "mission.runtime_binding.mission_id")
    _expect_exact(runtime["operation_mode"], entry["operation_mode"], "mission.runtime_binding.operation_mode")
    _expect_exact(runtime["phases"], entry["phases"], "mission.runtime_binding.phases")
    expected_choices = EXPECTED_CHOICES if entry["decision_kind"] == "well_choice" else []
    _expect_exact(runtime["well_choices"], expected_choices, "mission.runtime_binding.well_choices")
    _expect_exact(runtime["well_uncommitted_state"], "Dormant", "mission.runtime_binding.well_uncommitted_state")
    _expect_exact(runtime["campaign_commit_statuses"], EXPECTED_COMMIT_STATUSES, "mission.runtime_binding.campaign_commit_statuses")
    _expect_exact(runtime["runtime_consumed"], False, "mission.runtime_binding.runtime_consumed")
    _expect_exact(runtime["localization_runtime_status"], "unimplemented", "mission.runtime_binding.localization_runtime_status")
    _expect_exact(
        runtime["failure_reason_binding"],
        "bound_runtime"
        if entry["file"].removesuffix(".json") in RUNTIME_BOUND_FAILURE_MISSIONS
        else "requested",
        "mission.runtime_binding.failure_reason_binding",
    )

    matrix = canon["missions"][entry["mission_index"]]
    _expect_exact(matrix["mission_id"], runtime["mission_id"], "mission.runtime_binding.matrix_mission_id")
    _expect_exact(matrix["operation_mode"], runtime["operation_mode"], "mission.runtime_binding.matrix_operation_mode")

    canon_record = _expect_object(top["canon"], "mission.canon")
    for required_key in ("title", "purpose", "player_pov", "command_faction", "hidden_moral_score", "canonical_facts", "prohibited_inferences"):
        if required_key not in canon_record:
            raise NarrativeValidationError(f"mission.canon.{required_key}: required canon field is absent")
    _expect_exact(canon_record["title"], matrix["title"], "mission.canon.title")
    _validate_source_text(canon_record["purpose"], "mission.canon.purpose")
    _expect_symbol(canon_record["player_pov"], "mission.canon.player_pov")
    if canon_record["command_faction"] not in EXPECTED_FACTIONS:
        raise NarrativeValidationError("mission.canon.command_faction: unknown faction")
    _expect_exact(canon_record["hidden_moral_score"], False, "mission.canon.hidden_moral_score")
    for key, minimum in (("canonical_facts", 7), ("prohibited_inferences", 6)):
        entries = _expect_list(canon_record[key], f"mission.canon.{key}")
        if len(entries) < minimum:
            raise NarrativeValidationError(f"mission.canon.{key}: expected at least {minimum} statements")
        for index, statement in enumerate(entries):
            _validate_source_text(statement, f"mission.canon.{key}[{index}]")
    for key, extra in canon_record.items():
        if key in {"title", "purpose", "player_pov", "command_faction", "hidden_moral_score", "canonical_facts", "prohibited_inferences"}:
            continue
        _expect_symbol(key, f"mission.canon.{key} (key)")
        _validate_canon_value(extra, f"mission.canon.{key}")
    _expect_canonical_projection(
        {
            "purpose": canon_record["purpose"],
            "canonical_facts": canon_record["canonical_facts"],
            "prohibited_inferences": canon_record["prohibited_inferences"],
        },
        entry["canon_prose_sha256"],
        "mission.canon.reviewed_canonical_prose",
    )

    expected_speakers: dict[str, dict[str, Any]] = entry["speakers"]
    speakers = _expect_list(top["speakers"], "mission.speakers")
    if len(speakers) != len(expected_speakers):
        raise NarrativeValidationError("mission.speakers: cast does not match the reviewed registry cast")
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
        if speaker_id not in expected_speakers:
            raise NarrativeValidationError(f"{path}.id: speaker is not in the reviewed registry cast")
        speaker_ids.add(speaker_id)
        identity = EXPECTED_SPEAKER_IDENTITIES[speaker_id]
        _expect_exact(speaker["display_name"], identity["display_name"], f"{path}.display_name")
        _expect_exact(speaker["faction_id"], identity["faction_id"], f"{path}.faction_id")
        character_id = SPEAKER_CHARACTER_IDS[speaker_id]
        related_missions = EXPECTED_CHARACTER_RELATIONSHIPS[character_id][1]
        if entry["mission_id"] not in related_missions:
            raise NarrativeValidationError(
                f"{path}.id: {character_id} is not recorded for {entry['mission_id']} in canon continuity"
            )
        pinned = expected_speakers[speaker_id]
        _expect_exact(speaker["role_in_mission"], pinned["role_in_mission"], f"{path}.role_in_mission")
        _expect_exact(speaker["command_authority"], pinned["command_authority"], f"{path}.command_authority")
        _expect_exact(speaker["delivery_channel"], pinned["delivery_channel"], f"{path}.delivery_channel")
        if pinned["command_authority"]:
            command_speakers.append(speaker_id)
        speaker_channels[speaker_id] = pinned["delivery_channel"]
        _expect_exact(speaker["physical_presence_status"], "not_asserted_by_contract", f"{path}.physical_presence_status")
        _expect_exact(speaker["voice_asset_status"], "absent", f"{path}.voice_asset_status")
    _expect_exact(command_speakers, entry["command_speaker_ids"], "mission.speakers[].command_authority")

    expected_signals: dict[str, str] = entry["trigger_signals"]
    expected_prereqs: dict[str, list[str]] = entry["trigger_prerequisites"]
    triggers = _expect_list(top["triggers"], "mission.triggers")
    if len(triggers) != len(expected_signals):
        raise NarrativeValidationError("mission.triggers: trigger set does not match the reviewed registry contract")
    trigger_ids: set[str] = set()
    for index, raw_trigger in enumerate(triggers):
        path = f"mission.triggers[{index}]"
        trigger = _exact_keys(
            raw_trigger,
            {"id", "runtime_signal", "prerequisite_ids", "occurrence", "reset_behavior", "binding_status"},
            path,
        )
        trigger_id = _validate_narrative_id(trigger["id"], f"{path}.id")
        if patterns["event"].fullmatch(trigger_id) is None or trigger_id in trigger_ids:
            raise NarrativeValidationError(f"{path}.id: invalid or duplicate trigger identifier")
        trigger_ids.add(trigger_id)
        signal = _expect_string(trigger["runtime_signal"], f"{path}.runtime_signal")
        if expected_signals.get(trigger_id) != signal:
            raise NarrativeValidationError(f"{path}.runtime_signal: does not match the stable source signal")
        prereqs = _expect_unique_strings(trigger["prerequisite_ids"], f"{path}.prerequisite_ids")
        if prereqs != expected_prereqs.get(trigger_id):
            raise NarrativeValidationError(f"{path}.prerequisite_ids: does not match the reviewed prerequisite chain")
        if trigger["occurrence"] not in {"once_per_attempt", "once_per_completion", "repeatable_after_failure"}:
            raise NarrativeValidationError(f"{path}.occurrence: unsupported occurrence policy")
        if trigger["reset_behavior"] not in {"reset_on_mission_retry", "reset_on_mission_replay", "starts_new_mission_attempt"}:
            raise NarrativeValidationError(f"{path}.reset_behavior: unsupported reset policy")
        _expect_exact(trigger["binding_status"], "authored_unbound", f"{path}.binding_status")
    _expect_exact(set(expected_signals), trigger_ids, "mission.triggers[].id")

    content_ids: set[str] = {top["content_id"]}
    loc_keys: set[str] = set()
    ui = _exact_keys(top["ui_copy"], {"briefing", "objectives"}, "mission.ui_copy")
    _validate_localized_text(ui["briefing"], "mission.ui_copy.briefing", content_ids, loc_keys)
    objectives = _expect_list(ui["objectives"], "mission.ui_copy.objectives")
    if len(objectives) != entry["counts"]["objectives"]:
        raise NarrativeValidationError("mission.ui_copy.objectives: objective count does not match the reviewed contract")
    for index, objective in enumerate(objectives):
        _validate_localized_text(objective, f"mission.ui_copy.objectives[{index}]", content_ids, loc_keys)

    lines = _expect_list(top["lines"], "mission.lines")
    if len(lines) != entry["counts"]["lines"]:
        raise NarrativeValidationError("mission.lines: line count does not match the reviewed contract")
    line_ids: set[str] = set()
    voice_hook_ids: set[str] = set()
    line_triggers: dict[str, str] = {}
    line_projection: list[list[str]] = []
    for index, raw_line in enumerate(lines):
        path = f"mission.lines[{index}]"
        line = _exact_keys(
            raw_line,
            {"id", "loc_key", "speaker_id", "trigger_id", "delivery_channel", "source_text", "placeholders", "text_budget", "subtitle", "transcript_included", "voice_hook", "binding_status"},
            path,
        )
        line_id = _validate_narrative_id(line["id"], f"{path}.id")
        if patterns["line"].fullmatch(line_id) is None or line_id in content_ids:
            raise NarrativeValidationError(f"{path}.id: invalid or duplicate line identifier")
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
        line_projection.append([line_id, speaker_id, source_text])
        _validate_placeholders(line["placeholders"], source_text, f"{path}.placeholders")
        _validate_text_budget(line["text_budget"], source_text, f"{path}.text_budget")
        subtitle = _exact_keys(line["subtitle"], {"enabled", "timing_status"}, f"{path}.subtitle")
        _expect_exact(subtitle["enabled"], True, f"{path}.subtitle.enabled")
        _expect_exact(subtitle["timing_status"], "unassigned", f"{path}.subtitle.timing_status")
        _expect_exact(line["transcript_included"], True, f"{path}.transcript_included")
        voice_hook = _exact_keys(line["voice_hook"], {"id", "asset_status"}, f"{path}.voice_hook")
        voice_id = _expect_string(voice_hook["id"], f"{path}.voice_hook.id")
        if patterns["voice"].fullmatch(voice_id) is None or voice_id in voice_hook_ids:
            raise NarrativeValidationError(f"{path}.voice_hook.id: invalid or duplicate logical voice hook")
        voice_hook_ids.add(voice_id)
        _expect_exact(voice_hook["asset_status"], "absent", f"{path}.voice_hook.asset_status")
        _expect_exact(line["binding_status"], "authored_unbound", f"{path}.binding_status")
    _expect_canonical_projection(
        sorted(line_projection),
        entry["lines_projection_sha256"],
        "mission.lines.reviewed_projection",
    )

    used_line_ids: set[str] = set()
    sequences = _expect_list(top["dialogue_sequences"], "mission.dialogue_sequences")
    if len(sequences) != entry["counts"]["sequences"]:
        raise NarrativeValidationError("mission.dialogue_sequences: sequence count does not match the reviewed contract")
    sequence_ids: set[str] = set()
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
        _expect_exact(sequence["binding_status"], "authored_unbound", f"{path}.binding_status")

    branch_keys: list[str] = entry["branch_keys"]
    branches = _exact_keys(top["branch_variants"], branch_keys, "mission.branch_variants")
    for choice in branch_keys:
        path = f"mission.branch_variants.{choice}"
        branch = _exact_keys(
            branches[choice],
            {"choice", "current_runtime_behavior", "design_target_tradeoff", "runtime_alignment", "trigger_id", "dialogue_line_ids", "design_target_choice_ui", "binding_status"},
            path,
        )
        _expect_exact(branch["choice"], choice, f"{path}.choice")
        for field_name in ("current_runtime_behavior", "design_target_tradeoff"):
            text = _validate_source_text(branch[field_name], f"{path}.{field_name}")
            if any(lexeme in text.casefold() for lexeme in EXPLICIT_MORAL_RANKING_LEXEMES):
                raise NarrativeValidationError(
                    f"{path}.{field_name}: limited explicit moral-ranking lexical guard matched"
                )
        _expect_symbol(branch["runtime_alignment"], f"{path}.runtime_alignment")
        _expect_exact(branch["trigger_id"], entry["decision_trigger_id"], f"{path}.trigger_id")
        refs = _expect_unique_strings(branch["dialogue_line_ids"], f"{path}.dialogue_line_ids", minimum=1)
        if len(refs) > 3 or not set(refs).issubset(line_ids):
            raise NarrativeValidationError(f"{path}.dialogue_line_ids: expected one to three resolved branch lines")
        if any(line_triggers[ref] != branch["trigger_id"] for ref in refs):
            raise NarrativeValidationError(f"{path}.dialogue_line_ids: branch line trigger mismatch")
        if used_line_ids.intersection(refs):
            raise NarrativeValidationError(f"{path}.dialogue_line_ids: branch lines must be singly assigned")
        used_line_ids.update(refs)
        _validate_localized_text(branch["design_target_choice_ui"], f"{path}.design_target_choice_ui", content_ids, loc_keys)
        _expect_exact(branch["binding_status"], "authored_unbound", f"{path}.binding_status")
    if branch_keys:
        branch_serialized = json.dumps(branches, sort_keys=True).casefold()
        if entry["decision_kind"] == "well_choice" and "dormant" in branch_serialized:
            raise NarrativeValidationError("mission.branch_variants: Dormant is not a terminal narrative branch")
        if any(lexeme in branch_serialized for lexeme in EXPLICIT_MORAL_RANKING_LEXEMES):
            raise NarrativeValidationError("mission.branch_variants: limited explicit moral-ranking lexical guard matched")
        _expect_canonical_projection(
            branches,
            entry["branches_projection_sha256"],
            "mission.branch_variants.reviewed_projection",
        )

    results = _expect_list(top["result_variants"], "mission.result_variants")
    if len(results) != 4:
        raise NarrativeValidationError("mission.result_variants: exact four campaign persistence states required")
    result_statuses: list[str] = []
    for index, raw_result in enumerate(results):
        path = f"mission.result_variants[{index}]"
        result = _exact_keys(raw_result, {"status", "copy"}, path)
        result_statuses.append(_expect_string(result["status"], f"{path}.status"))
        _validate_localized_text(result["copy"], f"{path}.copy", content_ids, loc_keys)
    _expect_exact(result_statuses, EXPECTED_COMMIT_STATUSES, "mission.result_variants[].status")

    failure_retry = _exact_keys(
        top["failure_retry"],
        {"failure_variants", "retry_copy", "campaign_state_effect", "runtime_delivery_status"},
        "mission.failure_retry",
    )
    failures = _expect_list(failure_retry["failure_variants"], "mission.failure_retry.failure_variants")
    expected_reasons: list[str] = entry["failure_reason_codes"]
    if len(failures) != len(expected_reasons):
        raise NarrativeValidationError("mission.failure_retry.failure_variants: failure set does not match the reviewed contract")
    failure_reasons: list[str] = []
    failed_trigger_id = entry["failed_trigger_id"]
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
        if any(line_triggers[ref] != failed_trigger_id for ref in refs):
            raise NarrativeValidationError(f"{path}.dialogue_line_ids: failure line trigger mismatch")
        if used_line_ids.intersection(refs):
            raise NarrativeValidationError(f"{path}.dialogue_line_ids: failure lines must be singly assigned")
        used_line_ids.update(refs)
    _expect_exact(failure_reasons, expected_reasons, "mission.failure_retry.failure_variants[].reason_code")
    _validate_localized_text(failure_retry["retry_copy"], "mission.failure_retry.retry_copy", content_ids, loc_keys)
    _expect_exact(failure_retry["campaign_state_effect"], entry["campaign_state_effect"], "mission.failure_retry.campaign_state_effect")
    _expect_exact(failure_retry["runtime_delivery_status"], "unimplemented", "mission.failure_retry.runtime_delivery_status")

    asset_hooks = _expect_list(top["asset_hooks"], "mission.asset_hooks")
    expected_hooks: dict[str, str] = entry["asset_hooks"]
    if len(asset_hooks) != len(expected_hooks):
        raise NarrativeValidationError("mission.asset_hooks: hook set does not match the reviewed contract")
    asset_hook_ids: set[str] = set()
    asset_kinds: dict[str, str] = {}
    for index, raw_hook in enumerate(asset_hooks):
        path = f"mission.asset_hooks[{index}]"
        hook = _exact_keys(raw_hook, {"id", "kind", "asset_status", "binding_status"}, path)
        hook_id = _expect_string(hook["id"], f"{path}.id")
        if patterns["asset"].fullmatch(hook_id) is None or hook_id in asset_hook_ids:
            raise NarrativeValidationError(f"{path}.id: invalid or duplicate logical asset hook")
        asset_hook_ids.add(hook_id)
        kind = _expect_string(hook["kind"], f"{path}.kind")
        if kind not in {"audio", "visual"} or not hook_id.startswith("aud_" if kind == "audio" else "vis_"):
            raise NarrativeValidationError(f"{path}.kind: does not match logical hook prefix")
        asset_kinds[hook_id] = kind
        _expect_exact(hook["asset_status"], "absent", f"{path}.asset_status")
        _expect_exact(hook["binding_status"], "unimplemented", f"{path}.binding_status")
    _expect_exact(asset_kinds, expected_hooks, "mission.asset_hooks logical inventory")

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
    if cinematic["trigger_id"] not in trigger_ids:
        raise NarrativeValidationError("mission.cinematic.trigger_id: unresolved trigger reference")
    _expect_exact(cinematic["format"], "in_engine_storyboard", "mission.cinematic.format")
    _expect_exact(cinematic["implementation_status"], "absent", "mission.cinematic.implementation_status")
    _expect_exact(cinematic["binding_status"], "authored_unbound", "mission.cinematic.binding_status")
    _expect_exact(cinematic["named_character_physical_presence_asserted"], False, "mission.cinematic.named_character_physical_presence_asserted")
    shots = _expect_list(cinematic["shots"], "mission.cinematic.shots")
    if len(shots) != entry["counts"]["shots"]:
        raise NarrativeValidationError("mission.cinematic.shots: shot count does not match the reviewed contract")
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
        visual_refs = _expect_unique_strings(shot["visual_hook_ids"], f"{path}.visual_hook_ids", minimum=1)
        audio_refs = _expect_unique_strings(shot["audio_hook_ids"], f"{path}.audio_hook_ids", minimum=1)
        if any(asset_kinds.get(ref) != "visual" for ref in visual_refs):
            raise NarrativeValidationError(f"{path}.visual_hook_ids: unresolved or nonvisual hook")
        if any(asset_kinds.get(ref) != "audio" for ref in audio_refs):
            raise NarrativeValidationError(f"{path}.audio_hook_ids: unresolved or nonaudio hook")
        referenced_asset_hook_ids.update(visual_refs)
        referenced_asset_hook_ids.update(audio_refs)
    _expect_exact(referenced_asset_hook_ids, asset_hook_ids, "mission.cinematic.shots asset hook coverage")

    projections = _exact_keys(top["projections"], {"subtitle_source", "transcript_source", "duplication_policy"}, "mission.projections")
    _expect_exact(projections["subtitle_source"], "lines[].source_text", "mission.projections.subtitle_source")
    _expect_exact(projections["transcript_source"], "lines[].source_text", "mission.projections.transcript_source")
    _expect_exact(projections["duplication_policy"], "generated_from_canonical_source_text", "mission.projections.duplication_policy")

    implementation = _exact_keys(
        top["implementation"],
        {"mechanics", "narrative_contract", "runtime_consumption", "subtitles", "voice_assets", "cinematics", "localization_pipeline", "well_telegraphs", "reshape_expiry_warning", "manual_observation", "packaged_build"},
        "mission.implementation",
    )
    for key, expected in {
        "mechanics": "implemented_current_source_with_known_presentation_gaps",
        "narrative_contract": "authored_source_only",
        "runtime_consumption": "partial_briefing_lines_results",
        "subtitles": "authored_unbound",
        "voice_assets": "absent",
        "cinematics": "absent",
        "localization_pipeline": "unimplemented",
        "well_telegraphs": "unimplemented",
        "reshape_expiry_warning": "unimplemented",
        "manual_observation": "not_run",
        "packaged_build": "not_run",
    }.items():
        _expect_exact(implementation[key], expected, f"mission.implementation.{key}")

    if used_line_ids != line_ids:
        missing = sorted(line_ids - used_line_ids)
        raise NarrativeValidationError(f"mission.lines: unreferenced canonical lines: {', '.join(missing)}")
    return {
        "lines": len(lines),
        "branches": len(branch_keys),
        "failures": len(failures),
        "results": len(results),
        "shots": len(shots),
    }


def validate_schema_documents(schema_dir: Path) -> None:
    expected = {
        "campaign_canon_continuity.schema.json": "echoes://narrative/schema/campaign-canon-continuity-v1",
        "mission_contract.schema.json": "echoes://narrative/schema/mission-contract-v2",
    }
    for filename, schema_id in expected.items():
        schema = load_json_document(schema_dir / filename)
        _expect_exact(schema.get("$schema"), "https://json-schema.org/draft/2020-12/schema", f"schema.{filename}.$schema")
        _expect_exact(schema.get("$id"), schema_id, f"schema.{filename}.$id")
        _expect_exact(schema.get("type"), "object", f"schema.{filename}.type")
        _expect_exact(schema.get("additionalProperties"), False, f"schema.{filename}.additionalProperties")


DEMO_CONTRACT_REGISTRY: dict[str, dict[str, Any]] = {
    "tutorial_readiness_check.json": {
        "content_id": "demo_tutorial_readiness_check",
        "surface": "tutorial",
        "scope": "prologue_tutorial",
        "speaker_ids": {"spk_mara_vey"},
        "counts": {"triggers": 32, "lines": 43},
    },
    "system_voice_annunciator.json": {
        "content_id": "demo_system_voice_annunciator",
        "surface": "system_voice",
        "scope": "global",
        "speaker_ids": {"spk_annunciator"},
        "counts": {"triggers": 12, "lines": 12},
    },
}

# The Annunciator's audio-side copy constraints, enforced mechanically so an
# out-of-budget alert line fails the pipeline instead of reaching a take pool:
# 2-4 words, one breath group, a stable distinct opening word per class, and no
# second-person address (accepted system-voice spec; owner rulings 4, 10, 20).
SYSTEM_VOICE_MAX_WORDS = 4
# One word is the ideal case, not a violation: the binding constraint is the
# 0.8 s / one-breath-group budget, and the accepted spec's own register anchor
# for the contact alert is the single word "Contact." (owner rulings 19, 20).
SYSTEM_VOICE_MIN_WORDS = 1
# One breath group: no comma, and exactly one terminal sentence mark. Matching
# the whole line rejects "Alert! Contact." and "Contact? Alert." as well as a
# second period-terminated sentence.
SYSTEM_VOICE_ONE_BREATH_GROUP = re.compile(r"[^.!?]*\.")
# Stem match, so "yourself" and "you're" are caught alongside "you"/"your".
SYSTEM_VOICE_PLAYER_ADDRESS = re.compile(r"\byou\w*\b", re.IGNORECASE)


def _validate_system_voice_copy(lines: list[dict[str, Any]], path: str) -> None:
    openings: dict[str, str] = {}
    for index, line in enumerate(lines):
        text = line["source_text"]
        where = f"{path}[{index}].source_text"
        words = text.split()
        if not SYSTEM_VOICE_MIN_WORDS <= len(words) <= SYSTEM_VOICE_MAX_WORDS:
            raise NarrativeValidationError(
                f"{where}: system-voice copy must be "
                f"{SYSTEM_VOICE_MIN_WORDS}..{SYSTEM_VOICE_MAX_WORDS} words, found {len(words)}"
            )
        if "," in text or SYSTEM_VOICE_ONE_BREATH_GROUP.fullmatch(text) is None:
            raise NarrativeValidationError(
                f"{where}: system-voice copy must be one breath group ending in a single period"
            )
        address = SYSTEM_VOICE_PLAYER_ADDRESS.search(text)
        if address is not None:
            raise NarrativeValidationError(
                f"{where}: system-voice copy must not address the player ({address.group(0)!r})"
            )
        opening = words[0].casefold()
        if opening in openings:
            raise NarrativeValidationError(
                f"{where}: opening word {words[0]!r} already used by {openings[opening]}"
            )
        openings[opening] = line["id"]


def validate_demo_contract(
    value: dict[str, Any],
    entry: dict[str, Any],
) -> dict[str, int]:
    """Validate one additive demo-surface contract.

    Demo contracts carry tutorial and system-voice copy that is not mission
    dialogue. They are deliberately separate from the fifteen pinned mission
    contracts: every mission pin stays exactly as authored, and these rules
    apply only to this namespace.
    """
    top = _exact_keys(
        value,
        {
            "schema_version",
            "namespace",
            "content_id",
            "metadata",
            "runtime_binding",
            "speakers",
            "triggers",
            "lines",
        },
        "demo",
    )
    _validate_all_strings(top, "demo")
    _expect_exact(top["schema_version"], 1, "demo.schema_version")
    _expect_exact(top["namespace"], "echoes.narrative.demo", "demo.namespace")
    _expect_exact(top["content_id"], entry["content_id"], "demo.content_id")

    metadata = _exact_keys(
        top["metadata"],
        {"author", "status", "source_document", "source_document_sha256"},
        "demo.metadata",
    )
    _expect_exact(metadata["author"], "Angelis Pseftis", "demo.metadata.author")
    _expect_exact(metadata["status"], "authored_unbound", "demo.metadata.status")
    _expect_string(metadata["source_document"], "demo.metadata.source_document")
    if re.fullmatch(r"[0-9a-f]{64}", _expect_string(
        metadata["source_document_sha256"], "demo.metadata.source_document_sha256"
    )) is None:
        raise NarrativeValidationError(
            "demo.metadata.source_document_sha256: expected a lowercase sha256 digest"
        )

    binding = _exact_keys(
        top["runtime_binding"],
        {"surface", "scope", "opens_after_signal", "binding_status"},
        "demo.runtime_binding",
    )
    _expect_exact(binding["surface"], entry["surface"], "demo.runtime_binding.surface")
    _expect_exact(binding["scope"], entry["scope"], "demo.runtime_binding.scope")
    opens_after = _expect_string(
        binding["opens_after_signal"], "demo.runtime_binding.opens_after_signal"
    )
    if entry["scope"] == "global":
        _expect_exact(opens_after, "none", "demo.runtime_binding.opens_after_signal")
    elif opens_after == "none":
        raise NarrativeValidationError(
            "demo.runtime_binding.opens_after_signal: a scoped surface must name its opening signal"
        )
    _expect_exact(
        binding["binding_status"], "authored_unbound", "demo.runtime_binding.binding_status"
    )

    speakers = _expect_list(top["speakers"], "demo.speakers")
    speaker_ids: set[str] = set()
    for index, speaker in enumerate(speakers):
        record = _exact_keys(
            speaker,
            {
                "id",
                "display_name",
                "faction_id",
                "role_in_surface",
                "delivery_channel",
                "voice_asset_status",
            },
            f"demo.speakers[{index}]",
        )
        speaker_ids.add(_expect_symbol(record["id"], f"demo.speakers[{index}].id"))
        _expect_string(record["display_name"], f"demo.speakers[{index}].display_name")
        _expect_symbol(record["faction_id"], f"demo.speakers[{index}].faction_id")
        _expect_symbol(record["role_in_surface"], f"demo.speakers[{index}].role_in_surface")
        _expect_symbol(record["delivery_channel"], f"demo.speakers[{index}].delivery_channel")
        _expect_exact(
            record["voice_asset_status"], "absent", f"demo.speakers[{index}].voice_asset_status"
        )
    if speaker_ids != entry["speaker_ids"]:
        raise NarrativeValidationError(
            f"demo.speakers: expected exactly {sorted(entry['speaker_ids'])}"
        )

    triggers = _expect_list(top["triggers"], "demo.triggers")
    if len(triggers) != entry["counts"]["triggers"]:
        raise NarrativeValidationError(
            f"demo.triggers: expected exactly {entry['counts']['triggers']} triggers"
        )
    trigger_ids: set[str] = set()
    for index, item in enumerate(triggers):
        record = _exact_keys(
            item,
            {
                "id",
                "runtime_signal",
                "prerequisite_ids",
                "occurrence",
                "reset_behavior",
                "binding_status",
            },
            f"demo.triggers[{index}]",
        )
        tid = _validate_narrative_id(record["id"], f"demo.triggers[{index}].id")
        if tid in trigger_ids:
            raise NarrativeValidationError(f"demo.triggers[{index}].id: duplicate {tid!r}")
        trigger_ids.add(tid)
        _expect_string(record["runtime_signal"], f"demo.triggers[{index}].runtime_signal")
        if record["occurrence"] not in {"once_per_attempt", "repeatable"}:
            raise NarrativeValidationError(
                f"demo.triggers[{index}].occurrence: expected once_per_attempt or repeatable"
            )
        _expect_exact(
            record["reset_behavior"],
            "reset_on_mission_retry",
            f"demo.triggers[{index}].reset_behavior",
        )
        _expect_exact(
            record["binding_status"],
            "authored_unbound",
            f"demo.triggers[{index}].binding_status",
        )
    for index, item in enumerate(triggers):
        for prerequisite in _expect_list(
            item["prerequisite_ids"], f"demo.triggers[{index}].prerequisite_ids"
        ):
            if prerequisite not in trigger_ids:
                raise NarrativeValidationError(
                    f"demo.triggers[{index}].prerequisite_ids: unknown trigger {prerequisite!r}; "
                    "demo prerequisites resolve inside their own contract"
                )

    lines = _expect_list(top["lines"], "demo.lines")
    if len(lines) != entry["counts"]["lines"]:
        raise NarrativeValidationError(
            f"demo.lines: expected exactly {entry['counts']['lines']} lines"
        )
    line_ids: set[str] = set()
    for index, item in enumerate(lines):
        record = _exact_keys(
            item,
            {
                "id",
                "loc_key",
                "speaker_id",
                "trigger_id",
                "delivery_channel",
                "source_text",
                "placeholders",
                "text_budget",
                "subtitle",
                "transcript_included",
                "voice_hook",
                "binding_status",
            },
            f"demo.lines[{index}]",
        )
        lid = _validate_narrative_id(record["id"], f"demo.lines[{index}].id")
        if lid in line_ids:
            raise NarrativeValidationError(f"demo.lines[{index}].id: duplicate {lid!r}")
        line_ids.add(lid)
        _expect_exact(record["loc_key"], lid, f"demo.lines[{index}].loc_key")
        if record["speaker_id"] not in speaker_ids:
            raise NarrativeValidationError(
                f"demo.lines[{index}].speaker_id: unknown speaker {record['speaker_id']!r}"
            )
        if record["trigger_id"] not in trigger_ids:
            raise NarrativeValidationError(
                f"demo.lines[{index}].trigger_id: unknown trigger {record['trigger_id']!r}"
            )
        _expect_symbol(record["delivery_channel"], f"demo.lines[{index}].delivery_channel")
        text = _validate_source_text(record["source_text"], f"demo.lines[{index}].source_text")
        _validate_placeholders(
            record["placeholders"], text, f"demo.lines[{index}].placeholders"
        )
        _validate_text_budget(record["text_budget"], text, f"demo.lines[{index}].text_budget")
        subtitle = _exact_keys(
            record["subtitle"], {"enabled", "timing_status"}, f"demo.lines[{index}].subtitle"
        )
        _expect_exact(subtitle["enabled"], True, f"demo.lines[{index}].subtitle.enabled")
        _expect_exact(
            subtitle["timing_status"], "unassigned", f"demo.lines[{index}].subtitle.timing_status"
        )
        _expect_exact(
            record["transcript_included"], True, f"demo.lines[{index}].transcript_included"
        )
        voice_hook = _exact_keys(
            record["voice_hook"], {"id", "asset_status"}, f"demo.lines[{index}].voice_hook"
        )
        _expect_symbol(voice_hook["id"], f"demo.lines[{index}].voice_hook.id")
        _expect_exact(
            voice_hook["asset_status"], "absent", f"demo.lines[{index}].voice_hook.asset_status"
        )
        _expect_exact(
            record["binding_status"],
            "authored_unbound",
            f"demo.lines[{index}].binding_status",
        )

    if entry["surface"] == "system_voice":
        _validate_system_voice_copy(lines, "demo.lines")

    return {"demo_contracts": 1, "demo_lines": len(lines), "demo_triggers": len(triggers)}


def validate_source_tree(root: Path) -> dict[str, int]:
    source_dir = root / "Content/Narrative/Source"
    schema_dir = root / "Content/Narrative/Schema"
    validate_schema_documents(schema_dir)
    canon = load_json_document(source_dir / "campaign_canon_continuity.json")
    canon_counts = validate_campaign_canon(canon)

    missions_dir = source_dir / "missions"
    expected_files = {"m01_what_the_ledger_keeps.json"} | {
        entry["file"] for entry in MISSION_REGISTRY.values()
    }
    actual_files = {path.name for path in missions_dir.glob("*.json")}
    if actual_files != expected_files:
        unexpected = sorted(actual_files - expected_files)
        missing = sorted(expected_files - actual_files)
        raise NarrativeValidationError(
            "missions: authored files and registry disagree "
            f"(unregistered: {unexpected}; absent: {missing})"
        )

    mission = load_json_document(missions_dir / "m01_what_the_ledger_keeps.json")
    totals = dict(validate_mission_contract(mission, canon))
    for stem in sorted(MISSION_REGISTRY):
        entry = MISSION_REGISTRY[stem]
        registered = load_json_document(missions_dir / entry["file"])
        counts = validate_registered_mission_contract(registered, canon, entry)
        for key, count in counts.items():
            totals[key] += count
    totals["authored_missions"] = 1 + len(MISSION_REGISTRY)

    # Additive demo namespace. Mission contracts above are untouched by this
    # block: demo contracts live in their own directory, carry their own
    # registry and rules, and cannot relax any mission pin.
    demo_dir = source_dir / "demo"
    expected_demo = set(DEMO_CONTRACT_REGISTRY)
    actual_demo = {path.name for path in demo_dir.glob("*.json")} if demo_dir.is_dir() else set()
    if actual_demo != expected_demo:
        unexpected = sorted(actual_demo - expected_demo)
        missing = sorted(expected_demo - actual_demo)
        raise NarrativeValidationError(
            "demo: authored files and registry disagree "
            f"(unregistered: {unexpected}; absent: {missing})"
        )
    demo_totals = {"demo_contracts": 0, "demo_lines": 0, "demo_triggers": 0}
    for name in sorted(DEMO_CONTRACT_REGISTRY):
        document = load_json_document(demo_dir / name)
        counts = validate_demo_contract(document, DEMO_CONTRACT_REGISTRY[name])
        for key, count in counts.items():
            demo_totals[key] += count

    return {**canon_counts, **totals, **demo_totals}


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
        f"missions={counts['missions']} authored={counts['authored_missions']} "
        f"characters={counts['characters']} "
        f"factions={counts['factions']} lines={counts['lines']} "
        f"branches={counts['branches']} failures={counts['failures']} "
        f"results={counts['results']} shots={counts['shots']} "
        f"demo_contracts={counts['demo_contracts']} demo_lines={counts['demo_lines']} "
        "runtime_consumed=false"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
