"""Compile authored narrative source into the digest-verified runtime pack.

Reads `Content/Narrative/Source` (canon continuity plus every registered
mission contract), revalidates it with the authoritative validator, and emits
`Content/Narrative/Generated/EchoesNarrativePack.json` — a compact runtime
projection the game loads fail-closed. Never hand-edit the output; edit the
source and recompile.

    python3 Scripts/compile_narrative.py

Author and owner: Angelis Pseftis
"""

from __future__ import annotations

import hashlib
import importlib.util
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
VALIDATOR_PATH = ROOT / "Content/Narrative/Schema/validate_narrative.py"
SOURCE_DIR = ROOT / "Content/Narrative/Source"
OUTPUT_PATH = ROOT / "Content/Narrative/Generated/EchoesNarrativePack.json"

PACK_SCHEMA_VERSION = 1


def load_validator():
    spec = importlib.util.spec_from_file_location("validate_narrative", VALIDATOR_PATH)
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def project_mission(mission: dict) -> dict:
    """The runtime projection of one authored mission contract."""
    speakers = {
        s["id"]: s["display_name"] for s in mission["speakers"]
    }
    triggers = {
        t["id"]: t["runtime_signal"] for t in mission["triggers"]
    }
    lines = [
        {
            "id": line["id"],
            "speaker": speakers[line["speaker_id"]],
            "signal": triggers[line["trigger_id"]],
            "text": line["source_text"],
        }
        for line in mission["lines"]
    ]
    return {
        "content_id": mission["content_id"],
        "mission_id": mission["runtime_binding"]["mission_id"],
        "title": mission["canon"]["title"],
        "briefing": mission["ui_copy"]["briefing"]["source_text"],
        "objectives": [
            objective["source_text"]
            for objective in mission["ui_copy"]["objectives"]
        ],
        "lines": lines,
        "results": {
            variant["status"]: variant["copy"]["source_text"]
            for variant in mission["result_variants"]
        },
        "failures": {
            variant["reason_code"]: variant["source_condition"]
            for variant in mission["failure_retry"]["failure_variants"]
        },
        "retry": mission["failure_retry"]["retry_copy"]["source_text"],
    }


def main() -> int:
    validator = load_validator()
    try:
        counts = validator.validate_source_tree(ROOT)
    except validator.NarrativeValidationError as exc:
        print(f"NARRATIVE_COMPILE_FAILED validation: {exc}")
        return 1

    missions_dir = SOURCE_DIR / "missions"
    operations: dict[str, dict] = {}
    for path in sorted(missions_dir.glob("*.json")):
        mission = validator.load_json_document(path)
        operation_mode = mission["runtime_binding"]["operation_mode"]
        operations[operation_mode] = project_mission(mission)

    pack = {
        "pack_format": "echoes-narrative-pack",
        "pack_version": PACK_SCHEMA_VERSION,
        "author": "Angelis Pseftis",
        "authored_missions": counts["authored_missions"],
        "line_count": counts["lines"],
        "operations": operations,
    }
    encoded = json.dumps(
        pack, ensure_ascii=False, indent=1, sort_keys=True
    ).encode("utf-8") + b"\n"
    digest = hashlib.sha256(encoded).hexdigest()

    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    with open(OUTPUT_PATH, "wb") as handle:
        handle.write(encoded)
    with open(str(OUTPUT_PATH) + ".sha256", "w", encoding="utf-8") as handle:
        handle.write(digest + "\n")
    print(
        "NARRATIVE_COMPILE_OK "
        f"operations={len(operations)} lines={counts['lines']} "
        f"digest={digest}"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
