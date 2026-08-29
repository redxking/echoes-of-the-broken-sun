#!/bin/zsh
set -euo pipefail

project_root="${0:A:h:h}"
default_app="$project_root/BuildArtifacts/Packages/Mac-Development-v0.23.1-mission-briefing/EchoesOfTheBrokenSun.app"
app="${1:-$default_app}"
timestamp="$(date -u +%Y%m%dT%H%M%SZ)"
evidence_dir="${2:-$project_root/BuildArtifacts/Performance/Soak/$timestamp}"
duration_seconds="${ECHOES_SOAK_SECONDS:-3600}"
sample_interval="${ECHOES_SOAK_INTERVAL_SECONDS:-5}"
warmup_seconds="${ECHOES_SOAK_WARMUP_SECONDS:-120}"
resolution_x="${ECHOES_SOAK_RES_X:-1280}"
resolution_y="${ECHOES_SOAK_RES_Y:-720}"

if [[ "$app" != /* ]]; then
  app="$project_root/$app"
fi
if [[ "$evidence_dir" != /* ]]; then
  evidence_dir="$project_root/$evidence_dir"
fi
app="${app:A}"
evidence_dir="${evidence_dir:A}"

binary="$app/Contents/MacOS/EchoesOfTheBrokenSun"
plist="$app/Contents/Info.plist"
manifest="${app:h}/EchoesOfTheBrokenSun.manifest.txt"
raw_log="$evidence_dir/packaged_soak.log"
samples="$evidence_dir/process_samples.csv"
summary="$evidence_dir/packaged_soak_summary.json"

for value_name value in \
  duration_seconds "$duration_seconds" \
  sample_interval "$sample_interval" \
  warmup_seconds "$warmup_seconds" \
  resolution_x "$resolution_x" \
  resolution_y "$resolution_y"; do
  if [[ "$value" != <-> || "$value" -lt 1 ]]; then
    print -u2 "$value_name must be a positive integer."
    exit 2
  fi
done
if (( warmup_seconds >= duration_seconds )); then
  print -u2 "ECHOES_SOAK_WARMUP_SECONDS must be smaller than ECHOES_SOAK_SECONDS."
  exit 2
fi
if [[ ! -x "$binary" || ! -f "$plist" || ! -f "$manifest" ]]; then
  print -u2 "The supplied application is not an accepted manifested Echoes package: $app"
  exit 3
fi
if ! /usr/bin/codesign --verify --deep --strict "$app"; then
  print -u2 "The package signature seal is invalid: $app"
  exit 4
fi
if [[ -e "$evidence_dir" ]]; then
  print -u2 "Refusing to mix a new soak with an existing evidence directory: $evidence_dir"
  exit 5
fi

mkdir -p "$evidence_dir"
: > "$raw_log"
print 'elapsed_seconds,rss_mib,cpu_percent' > "$samples"

exec_commands="r.SetRes ${resolution_x}x${resolution_y}w,r.VSync 0,t.MaxFPS 60,sg.ResolutionQuality 100,sg.ViewDistanceQuality 1,sg.AntiAliasingQuality 1,sg.ShadowQuality 1,sg.GlobalIlluminationQuality 1,sg.ReflectionQuality 1,sg.PostProcessQuality 1,sg.TextureQuality 1,sg.EffectsQuality 1,sg.FoliageQuality 1,sg.ShadingQuality 1,sg.LandscapeQuality 1,r.AntiAliasingMethod 2"

start_epoch="$(date +%s)"
"$binary" /Engine/Maps/Entry \
  -game -unattended -nop4 -nosplash -nosound -EchoesStress400 \
  -stdout -FullStdOutLogOutput \
  -windowed -ForceRes -ResX="$resolution_x" -ResY="$resolution_y" \
  "-ExecCmds=$exec_commands" > "$raw_log" 2>&1 &
game_pid=$!

survived_full_duration=0
while kill -0 "$game_pid" 2>/dev/null; do
  now_epoch="$(date +%s)"
  elapsed=$((now_epoch - start_epoch))
  if (( elapsed >= duration_seconds )); then
    survived_full_duration=1
    kill -TERM "$game_pid" 2>/dev/null || true
    break
  fi

  process_sample="$(/bin/ps -o rss=,%cpu= -p "$game_pid" | /usr/bin/awk 'NF >= 2 { print $1 "," $2 }' || true)"
  if [[ -n "$process_sample" ]]; then
    rss_kib="${process_sample%%,*}"
    cpu_percent="${process_sample#*,}"
    /usr/bin/awk -v elapsed="$elapsed" -v rss_kib="$rss_kib" -v cpu="$cpu_percent" \
      'BEGIN { printf "%d,%.3f,%.3f\n", elapsed, rss_kib / 1024.0, cpu }' >> "$samples"
  fi
  sleep "$sample_interval"
done

set +e
wait "$game_pid"
game_status=$?
set -e

if (( ! survived_full_duration )); then
  print -u2 "The packaged game exited before the ${duration_seconds}-second soak boundary with status $game_status. Inspect: $raw_log"
  exit 6
fi
if ! /usr/bin/grep -q '\[ECHOES_STRESS_READY\] units=400 teams=4 entities=401 visibleViews=401' "$raw_log"; then
  print -u2 "The soak did not reach the accepted four-team scale boundary. Inspect: $raw_log"
  exit 7
fi
if /usr/bin/grep -q '\[ECHOES_STRESS_ORDERS_READY\]' "$raw_log" &&
   ! /usr/bin/grep -q '\[ECHOES_STRESS_ORDERS_READY\] attackMove=396 teams=4 executeTick=1' "$raw_log"; then
  print -u2 "The soak reported an unexpected four-team broad-order fixture. Inspect: $raw_log"
  exit 8
fi
if /usr/bin/grep -Eq 'Fatal error:|Assertion failed:|GPU Crashed|Out of memory|Ran out of memory' "$raw_log"; then
  print -u2 "The soak log contains a rejected runtime failure. Inspect: $raw_log"
  exit 9
fi

package_version="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleShortVersionString' "$plist")"
source_commit="$(/usr/bin/awk -F= '$1 == "source_commit" { print $2; exit }' "$manifest")"
host_model="$(/usr/sbin/sysctl -n hw.model)"
cpu_brand="$(/usr/sbin/sysctl -n machdep.cpu.brand_string)"
macos_version="$(/usr/bin/sw_vers -productVersion)"

/usr/bin/python3 - "$samples" "$summary" "$duration_seconds" "$warmup_seconds" \
  "$sample_interval" "$resolution_x" "$resolution_y" "$package_version" \
  "$source_commit" "$host_model" "$cpu_brand" "$macos_version" <<'PY'
import csv
import json
import math
import pathlib
import statistics
import sys

(
    samples_path,
    summary_path,
    duration_seconds,
    warmup_seconds,
    sample_interval,
    resolution_x,
    resolution_y,
    package_version,
    source_commit,
    host_model,
    cpu_brand,
    macos_version,
) = sys.argv[1:]

duration_seconds = int(duration_seconds)
warmup_seconds = int(warmup_seconds)
sample_interval = int(sample_interval)

with open(samples_path, newline="", encoding="utf-8") as handle:
    rows = list(csv.DictReader(handle))

observations = [
    (int(row["elapsed_seconds"]), float(row["rss_mib"]), float(row["cpu_percent"]))
    for row in rows
]
steady = [row for row in observations if row[0] >= warmup_seconds]
if len(steady) < 10:
    raise SystemExit("Insufficient post-warm-up soak samples")

elapsed = [row[0] for row in steady]
rss = [row[1] for row in steady]
cpu = [row[2] for row in steady]

def percentile_nearest_rank(values, percentile):
    ordered = sorted(values)
    return ordered[math.ceil(percentile * len(ordered)) - 1]

mean_elapsed = statistics.fmean(elapsed)
mean_rss = statistics.fmean(rss)
denominator = sum((value - mean_elapsed) ** 2 for value in elapsed)
slope_mib_per_second = (
    sum((x - mean_elapsed) * (y - mean_rss) for x, y in zip(elapsed, rss)) / denominator
    if denominator else 0.0
)
slope_mib_per_hour = slope_mib_per_second * 3600.0
window_size = max(5, len(rss) // 10)
first_window_mean = statistics.fmean(rss[:window_size])
last_window_mean = statistics.fmean(rss[-window_size:])
window_growth = last_window_mean - first_window_mean

budgets = {
    "survived_requested_duration": True,
    "resident_memory_peak_mib_le_10240": max(rss) <= 10240.0,
    "steady_window_growth_mib_le_64": window_growth <= 64.0,
    "steady_linear_growth_mib_per_hour_le_128": slope_mib_per_hour <= 128.0,
}

result = {
    "artifact": "EchoesOfTheBrokenSun.app",
    "package_version": package_version,
    "source_commit": source_commit,
    "scenario": "four-team-scale-400",
    "configuration": "Development",
    "platform": "Mac",
    "rhi": "Metal",
    "host_model": host_model,
    "cpu": cpu_brand,
    "macos": macos_version,
    "display_mode": "windowed",
    "resolution": {"width": int(resolution_x), "height": int(resolution_y)},
    "quality_preset": "medium-groups-native-resolution-TAA",
    "requested_duration_seconds": duration_seconds,
    "warmup_seconds": warmup_seconds,
    "sample_interval_seconds": sample_interval,
    "samples": len(observations),
    "steady_samples": len(steady),
    "resident_memory_mib": {
        "initial_post_warmup": round(rss[0], 6),
        "final": round(rss[-1], 6),
        "mean": round(mean_rss, 6),
        "p95": round(percentile_nearest_rank(rss, 0.95), 6),
        "max": round(max(rss), 6),
        "first_window_mean": round(first_window_mean, 6),
        "last_window_mean": round(last_window_mean, 6),
        "window_growth": round(window_growth, 6),
        "linear_slope_mib_per_hour": round(slope_mib_per_hour, 6),
    },
    "cpu_percent": {
        "mean": round(statistics.fmean(cpu), 6),
        "p95": round(percentile_nearest_rank(cpu, 0.95), 6),
        "max": round(max(cpu), 6),
    },
    "budgets": budgets,
    "all_measured_budgets_pass": all(budgets.values()),
    "claim_boundary": (
        "Local packaged Mac Development durability and process-memory observation "
        "of the 400-unit/four-team visibility-scoped proxy fixture. The fixture "
        "omits authored weather/effects, formations, and representative broad combat "
        "orders. This is not clean-machine, networked, gameplay-completeness, or "
        "release qualification."
    ),
}

pathlib.Path(summary_path).write_text(
    json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8"
)
print(json.dumps(result, indent=2, sort_keys=True))
PY

print "Packaged soak completed."
print "Summary: $summary"
print "Process samples: $samples"
print "Runtime log: $raw_log"

if ! /usr/bin/grep -q '"all_measured_budgets_pass": true' "$summary"; then
  print -u2 "The packaged soak completed, but one or more recorded budgets failed."
  exit 9
fi
