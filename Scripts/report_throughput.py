#!/usr/bin/env python3
"""Report landed-change throughput and evidence cost per period.

Author and owner: Angelis Pseftis

Nothing in this repository measured cycle time, landed-change rate or cost per
closed gate, which is why an 82% collapse in landed commits stayed invisible for
seven days while the protocol causing it kept being applied. This reports the
numbers directly from git history and the evidence tree. It makes no judgement
about whether a rate is good; it only makes the rate visible.
"""
import subprocess, sys, datetime, pathlib, collections, os

ROOT = pathlib.Path(__file__).resolve().parent.parent

# Everything is computed in UTC. Commit timestamps are local (-0400 here) while
# evidence directories are UTC-stamped, so mixing the two silently drops any commit
# made after 20:00 local from "today" — which hid this session's own commits.
UTC_ENV = {**os.environ, "TZ": "UTC"}

def git(*args):
    return subprocess.run(["git", "-C", str(ROOT), *args],
                          capture_output=True, text=True, check=True,
                          env=UTC_ENV).stdout

def commits_between(start, end_exclusive):
    """Commits in [start, end) as (sha, unix_seconds), newest first."""
    out = git("log", "--pretty=%H %at",
              f"--since={start}T00:00:00+0000",
              f"--until={end_exclusive}T00:00:00+0000")
    rows = []
    for line in out.splitlines():
        if line.strip():
            sha, at = line.split()
            rows.append((sha, int(at)))
    return rows

def evidence_dirs():
    """Evidence directories by mtime date — the per-gate ceremony cost."""
    base = ROOT / "BuildArtifacts" / "Evidence"
    by_day = collections.Counter()
    if base.is_dir():
        for d in base.iterdir():
            if d.is_dir():
                day = datetime.datetime.fromtimestamp(
                    d.stat().st_mtime, datetime.timezone.utc).date().isoformat()
                by_day[day] += 1
    return by_day

def period(label, start, end_exclusive, ev):
    """Report a window.

    Rate is computed over the ELAPSED time actually spanned by the commits, not the
    calendar span of the window. Dividing a 66-minute sample by a 2-day window reported
    11/day for a window running at roughly 11/hour, and printed it as a regression.
    Short windows report commits and elapsed time and are not projected to a daily rate.
    """
    rows = commits_between(start, end_exclusive)
    n = len(rows)
    ev_n = sum(c for day, c in ev.items() if start <= day < end_exclusive)
    per_commit = (ev_n / n) if n else 0.0
    calendar_days = (datetime.date.fromisoformat(end_exclusive)
                     - datetime.date.fromisoformat(start)).days

    if n >= 2:
        times = [t for _, t in rows]
        elapsed_s = max(times) - min(times)
    else:
        elapsed_s = 0
    elapsed_h = elapsed_s / 3600.0

    # Only extrapolate to a daily rate once the sample actually spans about a day.
    if elapsed_h >= 24.0:
        rate = n / (elapsed_h / 24.0)
        rate_text = f"per_day={rate:<6.1f}"
    else:
        rate = None
        hours = f"{elapsed_h:.1f}h" if n >= 2 else "n/a"
        rate_text = f"elapsed={hours:<6} (short sample: not projected)"

    print(f"{label:<26} {start}..{end_exclusive}  "
          f"commits={n:<5} {rate_text} "
          f"evidence_dirs={ev_n:<4} dirs_per_commit={per_commit:.2f}")
    if rate is None and n:
        print(f"{'':<26}   {n} commits over {elapsed_h:.2f}h "
              f"({calendar_days}-day window); divide yourself if you need a rate")
    return n, rate

def main():
    ev = evidence_dirs()
    print("Landed-change throughput and evidence cost")
    print("(commit dates are author dates; evidence dirs counted by mtime)\n")
    before = period("pre-protocol", "2026-08-28", "2026-09-03", ev)
    after  = period("per-candidate protocol", "2026-09-03", "2026-09-10", ev)
    today = datetime.datetime.now(datetime.timezone.utc).date()
    period("current sprint", "2026-09-09", (today + datetime.timedelta(days=1)).isoformat(), ev)
    if before[1] and after[1]:
        delta = (after[1] - before[1]) / before[1] * 100.0
        print(f"\nchange in landed commits per day: {delta:+.1f}%")
    else:
        print("\nno rate comparison: at least one window is too short to project.")
    print("\nThis measures rate and evidence volume only. It does not measure "
          "change size, quality, or whether the evidence was warranted.\n"
          "An early sprint lands its cheap wins first, so a first-hour rate is not a "
          "sustainable rate and must not be projected forward.")

if __name__ == "__main__":
    sys.exit(main())
