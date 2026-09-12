#!/usr/bin/env python3
"""
run_ai_balance_matrix.py — Automated Headless AI Balance Matrix Runner

Master verification harness for SPEC-BAL-001 through SPEC-BAL-008.
Executes a 1,000-match automated AI balance matrix across all 9 faction
pairings on symmetric tournament terrain, asserting statistical competitive
balance, spawn fairness, strategy primacy, replay determinism, and AI competence.
"""

import json
import os
import subprocess
import sys
import time
from pathlib import Path


def get_git_commit(project_dir: Path) -> str:
    try:
        res = subprocess.run(
            ["git", "rev-parse", "HEAD"],
            cwd=project_dir,
            capture_output=True,
            text=True,
            check=True,
        )
        return res.stdout.strip()
    except Exception:
        return "UNKNOWN_GIT_COMMIT"


def build_harness(project_dir: Path, bin_path: Path) -> None:
    print("=== Compiling Headless AI Balance Harness (Optimized -O2) ===")
    cxx = os.environ.get("CXX", "clang++")
    cmd = [
        cxx,
        "-std=c++20",
        "-O2",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-I",
        str(project_dir / "Source/EchoesSimCore/Public"),
        str(project_dir / "Source/EchoesSimCore/Private/Simulation.cpp"),
        str(project_dir / "Source/EchoesSimCore/Private/NetworkProtocol.cpp"),
        str(project_dir / "Tests/Native/AiBalanceHarness.cpp"),
        "-o",
        str(bin_path),
    ]
    t0 = time.time()
    subprocess.run(cmd, cwd=project_dir, check=True)
    t1 = time.time()
    print(f"Compilation succeeded in {t1 - t0:.2f}s -> {bin_path}")


def main() -> int:
    script_dir = Path(__file__).resolve().parent
    project_dir = script_dir.parent
    artifacts_dir = project_dir / "BuildArtifacts" / "AiBalance"
    artifacts_dir.mkdir(parents=True, exist_ok=True)

    harness_bin = artifacts_dir / "ai_balance_harness"
    report_json = artifacts_dir / "balance_matrix_report.json"
    report_md = artifacts_dir / "balance_report.md"

    git_commit = get_git_commit(project_dir)
    print(f"Simulation Rules Commit SHA: {git_commit}")

    # 1. Compile native binary
    build_harness(project_dir, harness_bin)

    # 2. Run the diagnostic matrix. Only authoritative SimCore outcomes count;
    # ongoing runs remain actionable stalls and never receive inferred winners.
    total_matches = int(os.environ.get("ECHOES_BALANCE_MATCHES", "1000"))
    threads = int(os.environ.get("ECHOES_BALANCE_THREADS", "0"))
    cmd = [
        str(harness_bin),
        "--matches",
        str(total_matches),
        "--output",
        str(report_json),
    ]
    if threads > 0:
        cmd.extend(["--threads", str(threads)])

    print(f"\n=== Executing {total_matches}-Match Balance Matrix ===")
    t_start = time.time()
    res = subprocess.run(cmd, cwd=project_dir)
    t_elapsed = time.time() - t_start

    if not report_json.exists():
        print(f"FATAL: Report JSON was not generated at {report_json}", file=sys.stderr)
        return 1

    with open(report_json, "r", encoding="utf-8") as f:
        data = json.load(f)

    data["git_commit"] = git_commit
    with open(report_json, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2)

    # 3. Generate Markdown summary
    spawn = data.get("spawn_fairness", {})
    asym = data.get("asymmetry_balance", {})
    primacy = data.get("strategy_primacy", {})
    determinism = data.get("determinism", {})
    battery = data.get("ai_competence_battery", {})
    terminal_matches = data.get("authoritative_terminal_matches", 0)
    actionable_stalls = data.get("actionable_stalls", 0)
    sampling = data.get("condition_sampling", {})
    cond_rows = sampling.get("rows", [])
    degenerate_rows = [r for r in cond_rows if r.get("degenerate")]

    # A condition is a faction pair, a personality pair and a planning
    # order; its matches differ only by seed. The simulation is
    # deterministic and the seed reaches play through one fallback branch
    # a tasked planner never enters, so a condition whose matches all end
    # on the same tick is ONE observation replayed. Reporting an interval
    # over such rows states a precision that does not exist.
    if degenerate_rows:
        degenerate_table = "\n".join(
            "| `{}` | {} | {} | {} | {} | {} |".format(
                r["condition"], r["matches"], r["distinct_finishing_ticks"],
                r["seat0_wins"], r["seat1_wins"], r["unresolved"])
            for r in sorted(degenerate_rows, key=lambda r: r["condition"]))
    else:
        degenerate_table = "| _none_ | | | | | |"

    seat_split = {}
    for r in cond_rows:
        if r.get("degenerate"):
            continue
        order = "seat1-first" if r["condition"].endswith("seat1-first") else "seat0-first"
        agg = seat_split.setdefault(order, {"seat0": 0, "seat1": 0})
        agg["seat0"] += r["seat0_wins"]
        agg["seat1"] += r["seat1_wins"]
    seat_rows = "\n".join(
        "| {} | {} | {} |".format(order, v["seat0"], v["seat1"])
        for order, v in sorted(seat_split.items()))
    if not seat_rows:
        seat_rows = "| _no sampled conditions_ | | |"

    md_content = f"""# Headless AI Match Diagnostic Report

**Rules Commit SHA:** `{git_commit}`
**Total Matches:** {data.get('total_matches', 0)}
**Authoritative Terminal Matches:** {terminal_matches}
**Actionable Stalls:** {actionable_stalls}
**Execution Duration:** {data.get('elapsed_seconds', 0.0):.2f} seconds ({data.get('throughput_matches_per_sec', 0.0):.1f} matches/sec)
**Evidence Boundary:** Diagnostic only; the fixture is one synthetic map and the competence battery is incomplete.

---

## 1. Executive Balance Summary

| Metric | Target Window | Observed Rate | 95% Confidence Interval | Margin of Error | Status |
|---|---|---|---|---|---|
| **Spawn Symmetry (Slot 0)** (`SPEC-BAL-004`) | 45.0% – 55.0% | {spawn.get('rate', 0.0)*100:.1f}% | [{spawn.get('ci_lower', 0.0)*100:.1f}%, {spawn.get('ci_upper', 0.0)*100:.1f}%] | ±{spawn.get('margin_of_error', 0.0)*100:.1f}% | {'✅ PASS' if spawn.get('passed') else '❌ FAIL'} |
| **Meridian vs Kharuun** (`SPEC-BAL-003`) | 40.0% – 60.0% | {asym.get('meridian_vs_kharuun', {}).get('rate', 0.0)*100:.1f}% | [{asym.get('meridian_vs_kharuun', {}).get('ci_lower', 0.0)*100:.1f}%, {asym.get('meridian_vs_kharuun', {}).get('ci_upper', 0.0)*100:.1f}%] | ±{asym.get('meridian_vs_kharuun', {}).get('margin', 0.0)*100:.1f}% | {'✅ PASS' if asym.get('meridian_vs_kharuun', {}).get('passed') else '❌ FAIL'} |
| **Meridian vs Hollow Choir** (`SPEC-BAL-003`) | 40.0% – 60.0% | {asym.get('meridian_vs_choir', {}).get('rate', 0.0)*100:.1f}% | [{asym.get('meridian_vs_choir', {}).get('ci_lower', 0.0)*100:.1f}%, {asym.get('meridian_vs_choir', {}).get('ci_upper', 0.0)*100:.1f}%] | ±{asym.get('meridian_vs_choir', {}).get('margin', 0.0)*100:.1f}% | {'✅ PASS' if asym.get('meridian_vs_choir', {}).get('passed') else '❌ FAIL'} |
| **Kharuun vs Hollow Choir** (`SPEC-BAL-003`) | 40.0% – 60.0% | {asym.get('kharuun_vs_choir', {}).get('rate', 0.0)*100:.1f}% | [{asym.get('kharuun_vs_choir', {}).get('ci_lower', 0.0)*100:.1f}%, {asym.get('kharuun_vs_choir', {}).get('ci_upper', 0.0)*100:.1f}%] | ±{asym.get('kharuun_vs_choir', {}).get('margin', 0.0)*100:.1f}% | {'✅ PASS' if asym.get('kharuun_vs_choir', {}).get('passed') else '❌ FAIL'} |
| **Strategy Primacy (Adaptive vs Econ)** (`SPEC-BAL-005`) | > 75.0% | {primacy.get('rate', 0.0)*100:.1f}% | — | ±{primacy.get('margin', 0.0)*100:.1f}% | {'✅ PASS' if primacy.get('passed') else '❌ FAIL'} |
| **Duplicate Deterministic Rerun** | Final state and termination match | {'MATCHED' if determinism.get('passed') else 'DIVERGED'} | — | — | {'✅ PASS' if determinism.get('passed') else '❌ FAIL'} |
| **AI Competence Battery** (`SPEC-BAL-008`) | 4/4 Tests Pass | {battery.get('implemented_checks', 0)}/{battery.get('required_checks', 4)} implemented | — | — | INCOMPLETE |

---

## 2. Requirement Crosswalk & Qualification

- **Terminal integrity:** {terminal_matches}/{data.get('total_matches', 0)} runs reached an authoritative SimCore outcome. The remaining {actionable_stalls} runs are excluded from win-rate claims.
- **Rules scope:** `{data.get('rules_source', 'unknown')}` archetypes, digest `{data.get('rules_archetype_checksum', 'unknown')}`. A report whose rules_source is not `content-data` measured `DefaultSimulationRules` rather than the ruleset the game builds, and none of its numbers may be quoted.
- **Map scope:** The native harness uses `TournamentSymmetric{data.get('map_grid_tiles', '?')}`, which matches the shipping grid size but not the terrain, resources or spawns of the three shipping skirmish maps.
- **Replay scope:** Duplicate execution {'matched' if determinism.get('passed') else 'diverged'}. This is deterministic rerun evidence, not full replay-path qualification.
- **Competence scope:** {battery.get('implemented_checks', 0)}/{battery.get('required_checks', 4)} required checks are implemented. Focus fire, reconnaissance, and saturation remain unproven here.
- **Revision:** Diagnostic generated from commit `{git_commit}`. A commit label does not qualify incomplete evidence.

---

## 3. Condition Sampling (read before quoting any rate above)

Matches are not samples. A **condition** is a faction pair, a personality
pair and a planning order; matches within one differ only by seed. The
simulation is deterministic and the seed reaches gameplay through a single
fallback branch a tasked planner never enters, so a condition whose matches
all finish on the same tick is **one observation replayed**, however many
matches it contains.

| Conditions | Degenerate | Matches in degenerate conditions | Effective observations |
|---|---|---|---|
| {sampling.get('conditions', 0)} | {sampling.get('degenerate_conditions', 0)} | {sampling.get('degenerate_matches', 0)} | {sampling.get('effective_observations', 0)} |

Every rate and interval in section 1 excludes the degenerate rows below.

| Degenerate condition | Matches | Distinct finishing ticks | Seat 0 | Seat 1 | Unresolved |
|---|---|---|---|---|---|
{degenerate_table}

### Planning order, reported rather than smoothed

Seat 0's commands were queued first on every planning tick, and in a
symmetric race that alone decided the winner. Order is now varied across
conditions and reported: a large gap between these rows is a finding about
the game, not noise to average away.

| Planning order | Seat 0 wins | Seat 1 wins |
|---|---|---|
{seat_rows}
"""

    with open(report_md, "w", encoding="utf-8") as f:
        f.write(md_content)

    print(f"\nReport Markdown written to: {report_md}")

    # Check overall qualification
    overall_passed = (
        data.get("overall_passed", False)
        and (t_elapsed < 1800.0)
    )

    if overall_passed:
        print("\n>>> DIAGNOSTIC MATRIX COMPLETED WITHOUT QUALIFICATION GAPS <<<")
        return 0
    else:
        print("\n>>> DIAGNOSTIC COMPLETE; BALANCE QUALIFICATION REMAINS OPEN <<<", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
