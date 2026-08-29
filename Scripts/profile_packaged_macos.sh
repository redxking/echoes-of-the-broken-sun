#!/bin/zsh
set -euo pipefail

project_root="${0:A:h:h}"
default_app="$project_root/BuildArtifacts/Packages/Mac-Development-v0.15.0-four-team-scale/EchoesOfTheBrokenSun.app"
app="${1:-$default_app}"
timestamp="$(date -u +%Y%m%dT%H%M%SZ)"
evidence_dir="${2:-$project_root/BuildArtifacts/Performance/Packaged/$timestamp}"
frames="${ECHOES_PROFILE_FRAMES:-600}"
warmup_frames="${ECHOES_PROFILE_WARMUP_FRAMES:-120}"
timeout_seconds="${ECHOES_PROFILE_TIMEOUT_SECONDS:-180}"
resolution_x="${ECHOES_PROFILE_RES_X:-2560}"
resolution_y="${ECHOES_PROFILE_RES_Y:-1440}"
sample_interval="${ECHOES_PROFILE_RSS_INTERVAL_SECONDS:-0.25}"
anti_aliasing_method="${ECHOES_PROFILE_AA_METHOD:-2}"
resolution_quality="${ECHOES_PROFILE_RESOLUTION_QUALITY:-100}"
stress400="${ECHOES_PROFILE_STRESS400:-0}"

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
raw_log="$evidence_dir/packaged_profile.log"
rss_samples="$evidence_dir/resident_memory_samples.csv"
raw_csv="$evidence_dir/unreal_profile.csv"
summary="$evidence_dir/packaged_profile_summary.json"

for value_name value in \
  frames "$frames" \
  warmup_frames "$warmup_frames" \
  timeout_seconds "$timeout_seconds" \
  resolution_x "$resolution_x" \
  resolution_y "$resolution_y" \
  resolution_quality "$resolution_quality"; do
  if [[ "$value" != <-> || "$value" -lt 1 ]]; then
    print -u2 "$value_name must be a positive integer."
    exit 2
  fi
done

if [[ "$anti_aliasing_method" != <-> || "$anti_aliasing_method" -gt 5 ]]; then
  print -u2 "ECHOES_PROFILE_AA_METHOD must be an Unreal anti-aliasing method from 0 through 5."
  exit 2
fi
if (( resolution_quality > 100 )); then
  print -u2 "ECHOES_PROFILE_RESOLUTION_QUALITY must not exceed 100."
  exit 2
fi
if [[ "$stress400" != "0" && "$stress400" != "1" ]]; then
  print -u2 "ECHOES_PROFILE_STRESS400 must be 0 or 1."
  exit 2
fi

if (( warmup_frames >= frames )); then
  print -u2 "ECHOES_PROFILE_WARMUP_FRAMES must be smaller than ECHOES_PROFILE_FRAMES."
  exit 2
fi

if [[ "$sample_interval" != <->(|.<->) || "$sample_interval" == "0" || "$sample_interval" == "0.0" ]]; then
  print -u2 "ECHOES_PROFILE_RSS_INTERVAL_SECONDS must be a positive decimal number."
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
  print -u2 "Refusing to mix a new profile with an existing evidence directory: $evidence_dir"
  exit 5
fi

mkdir -p "$evidence_dir"
: > "$raw_log"
print 'elapsed_seconds,rss_mib' > "$rss_samples"

exec_commands="r.SetRes ${resolution_x}x${resolution_y}f,r.VSync 0,rhi.SyncInterval 0,t.MaxFPS 0,sg.ResolutionQuality $resolution_quality,sg.ViewDistanceQuality 1,sg.AntiAliasingQuality 1,sg.ShadowQuality 1,sg.GlobalIlluminationQuality 1,sg.ReflectionQuality 1,sg.PostProcessQuality 1,sg.TextureQuality 1,sg.EffectsQuality 1,sg.FoliageQuality 1,sg.ShadingQuality 1,sg.LandscapeQuality 1,r.AntiAliasingMethod $anti_aliasing_method,csv.TrackMemoryUse 1,csvprofile exitoncompletion,csvprofile frames=$frames"
stress_arguments=()
if [[ "$stress400" == "1" ]]; then
  stress_arguments=(-EchoesStress400)
fi

start_epoch="$(date +%s)"
"$binary" /Engine/Maps/Entry \
  -game -unattended -nop4 -nosplash -nosound \
  "${stress_arguments[@]}" \
  -stdout -FullStdOutLogOutput \
  -fullscreen -ForceRes -ResX="$resolution_x" -ResY="$resolution_y" \
  -csvGpuStats "-ExecCmds=$exec_commands" > "$raw_log" 2>&1 &
game_pid=$!

timed_out=0
while kill -0 "$game_pid" 2>/dev/null; do
  now_epoch="$(date +%s)"
  elapsed=$((now_epoch - start_epoch))
  if (( elapsed >= timeout_seconds )); then
    timed_out=1
    kill -TERM "$game_pid" 2>/dev/null || true
    break
  fi

  rss_kib="$(/bin/ps -o rss= -p "$game_pid" | /usr/bin/tr -d ' ' || true)"
  if [[ -n "$rss_kib" && "$rss_kib" == <-> ]]; then
    /usr/bin/awk -v elapsed="$elapsed" -v rss_kib="$rss_kib" \
      'BEGIN { printf "%.3f,%.3f\n", elapsed, rss_kib / 1024.0 }' >> "$rss_samples"
  fi
  sleep "$sample_interval"
done

set +e
wait "$game_pid"
game_status=$?
set -e

if (( timed_out )); then
  print -u2 "Packaged profile exceeded the ${timeout_seconds}-second bound. Inspect: $raw_log"
  exit 6
fi
if (( game_status != 0 )); then
  print -u2 "The packaged game exited with status $game_status. Inspect: $raw_log"
  exit 7
fi

generated_csv="$(/usr/bin/sed -n 's/^.*Writing CSV to file : //p' "$raw_log" | /usr/bin/tail -1)"
if [[ -z "$generated_csv" || ! -f "$generated_csv" ]]; then
  print -u2 "Unreal did not report a completed CSV capture. Inspect: $raw_log"
  exit 8
fi
cp "$generated_csv" "$raw_csv"

if /usr/bin/grep -Eq 'Fatal error:|Assertion failed:|GPU Crashed|Capture Stop requested, but no capture was running' "$raw_log"; then
  print -u2 "The packaged profile log contains a rejected runtime failure. Inspect: $raw_log"
  exit 9
fi
if ! /usr/bin/grep -q '\[ECHOES_WEATHER_READY\] glassScarDrift=active reducedMotionAware=true finalArt=false' "$raw_log"; then
  print -u2 "The packaged profile did not initialize the accepted Glass Scar atmosphere. Inspect: $raw_log"
  exit 10
fi
if [[ "$stress400" == "1" ]] &&
   ! /usr/bin/grep -q '\[ECHOES_STRESS_READY\] units=400 teams=4 entities=401 visibleViews=401' "$raw_log"; then
  print -u2 "The packaged profile did not reach the accepted four-team scale boundary. Inspect: $raw_log"
  exit 11
fi
if [[ "$stress400" == "1" ]] &&
   ! /usr/bin/grep -q '\[ECHOES_STRESS_ORDERS_READY\] attackMove=396 teams=4 executeTick=1' "$raw_log"; then
  print -u2 "The packaged profile did not queue the accepted four-team broad-order fixture. Inspect: $raw_log"
  exit 12
fi
if [[ "$stress400" == "1" ]] &&
   ! /usr/bin/grep -q '\[ECHOES_STRESS_COMBAT_ACTIVE\]' "$raw_log"; then
  print -u2 "The packaged profile did not observe active four-team combat. Inspect: $raw_log"
  exit 13
fi

package_version="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleShortVersionString' "$plist")"
source_commit="$(/usr/bin/awk -F= '$1 == "source_commit" { print $2; exit }' "$manifest")"
host_model="$(/usr/sbin/sysctl -n hw.model)"
cpu_brand="$(/usr/sbin/sysctl -n machdep.cpu.brand_string)"
macos_version="$(/usr/bin/sw_vers -productVersion)"

/usr/bin/python3 - "$raw_csv" "$rss_samples" "$summary" \
  "$frames" "$warmup_frames" "$resolution_x" "$resolution_y" \
  "$anti_aliasing_method" "$resolution_quality" \
  "$package_version" "$source_commit" "$host_model" "$cpu_brand" "$macos_version" \
  "$stress400" <<'PY'
import csv
import json
import math
import pathlib
import sys

(
    csv_path,
    rss_path,
    summary_path,
    expected_frames,
    warmup_frames,
    expected_x,
    expected_y,
    anti_aliasing_method,
    resolution_quality,
    package_version,
    source_commit,
    host_model,
    cpu_brand,
    macos_version,
    stress400,
) = sys.argv[1:]

expected_frames = int(expected_frames)
warmup_frames = int(warmup_frames)
expected_x = int(expected_x)
expected_y = int(expected_y)
anti_aliasing_method = int(anti_aliasing_method)
resolution_quality = int(resolution_quality)
stress400 = stress400 == "1"

with open(csv_path, newline="", encoding="utf-8-sig") as handle:
    rows = list(csv.reader(handle))

if len(rows) < 4:
    raise SystemExit("Unreal CSV capture is incomplete")

header = rows[0]
data_rows = []
metadata_row = None
for row in rows[1:]:
    if row and row[0] == "EVENTS":
        continue
    if row and row[0] == "[HasHeaderRowAtEnd]":
        metadata_row = row
        break
    # Unreal can register additional CSV stats during a capture and append their
    # columns to later rows. The initial columns remain stable, so accept rows
    # that contain at least the initial header rather than dropping wider rows.
    if len(row) >= len(header):
        data_rows.append(row)

if metadata_row is None:
    raise SystemExit("Unreal CSV metadata row is absent")
if len(data_rows) != expected_frames:
    raise SystemExit(
        f"Expected {expected_frames} captured frames, observed {len(data_rows)}"
    )

metadata = {}
for index in range(0, len(metadata_row) - 1, 2):
    key = metadata_row[index]
    if key.startswith("[") and key.endswith("]"):
        metadata[key[1:-1].lower()] = metadata_row[index + 1]

observed_x = int(float(metadata.get("systemresolution.resx", "0")))
observed_y = int(float(metadata.get("systemresolution.resy", "0")))
if (observed_x, observed_y) != (expected_x, expected_y):
    raise SystemExit(
        f"Requested {expected_x}x{expected_y}, Unreal recorded {observed_x}x{observed_y}"
    )

indices = {name: position for position, name in enumerate(header)}
required = ["FrameTime", "GameThreadTime", "RenderThreadTime", "GPUTime"]
missing = [name for name in required if name not in indices]
if missing:
    raise SystemExit(f"Required Unreal CSV columns are absent: {', '.join(missing)}")

sample_rows = data_rows[warmup_frames:]

def percentile_nearest_rank(values, percentile):
    ordered = sorted(values)
    return ordered[math.ceil(percentile * len(ordered)) - 1]

def summarize(values):
    ordered = sorted(values)
    middle = len(ordered) // 2
    if len(ordered) % 2:
        median = ordered[middle]
    else:
        median = (ordered[middle - 1] + ordered[middle]) / 2.0
    return {
        "samples": len(ordered),
        "mean": round(sum(ordered) / len(ordered), 6),
        "median": round(median, 6),
        "p95": round(percentile_nearest_rank(ordered, 0.95), 6),
        "max": round(ordered[-1], 6),
    }

timings = {}
for name in required:
    position = indices[name]
    values = [float(row[position]) for row in sample_rows if row[position] != ""]
    if len(values) != len(sample_rows):
        raise SystemExit(f"Unreal CSV column {name} has missing samples")
    timings[name] = summarize(values)

with open(rss_path, newline="", encoding="utf-8") as handle:
    rss_rows = list(csv.DictReader(handle))
rss_values = [float(row["rss_mib"]) for row in rss_rows if row.get("rss_mib")]
if not rss_values:
    raise SystemExit("No resident-memory samples were recorded")

budgets = {
    "frame_time_p95_ms_le_16_67": timings["FrameTime"]["p95"] <= 16.67,
    "game_thread_p95_ms_le_4": timings["GameThreadTime"]["p95"] <= 4.0,
    "render_thread_p95_ms_le_11": timings["RenderThreadTime"]["p95"] <= 11.0,
    "gpu_p95_ms_le_11": timings["GPUTime"]["p95"] <= 11.0,
    "resident_memory_max_mib_le_10240": max(rss_values) <= 10240.0,
}

result = {
    "artifact": "EchoesOfTheBrokenSun.app",
    "package_version": package_version,
    "source_commit": source_commit,
    "configuration": metadata.get("config", "unknown"),
    "platform": metadata.get("platform", "unknown"),
    "rhi": metadata.get("rhiname", "unknown"),
    "gpu": metadata.get("gpu", "unknown"),
    "host_model": host_model,
    "cpu": cpu_brand,
    "macos": macos_version,
    "resolution": {"width": observed_x, "height": observed_y},
    "display_mode": "fullscreen",
    "scenario": "four-team-scale-400" if stress400 else "placeholder-startup-25",
    "quality_preset": {
        "scalability_group_level": 1,
        "resolution_quality_percent": resolution_quality,
        "anti_aliasing_method": {
            0: "None",
            1: "FXAA",
            2: "TAA",
            3: "MSAA",
            4: "TSR",
            5: "SMAA",
        }.get(anti_aliasing_method, "Unknown"),
    },
    "capture_frames": len(data_rows),
    "discarded_warmup_frames": warmup_frames,
    "measured_frames": len(sample_rows),
    "timing_ms": timings,
    "resident_memory_mib": summarize(rss_values),
    "budgets": budgets,
    "all_measured_budgets_pass": all(budgets.values()),
    "claim_boundary": (
        "Local packaged Mac Development measurement of the 400-unit/four-team "
        "visibility-scoped scale scenario. All 400 units are visible to the local "
        "presentation and begin with 396 deterministic attack-move orders split "
        "evenly across four teams. Damage drives placeholder health bars and a "
        "reduced-flashing-aware combat pulse. A lightweight, reduced-motion-aware "
        "procedural Glass Scar atmosphere is active. The fixture does not include "
        "final effects or formations. It is "
        "not a soak test, clean-machine result, or release qualification."
        if stress400 else
        "Local packaged Mac Development measurement of the current 25-entity "
        "placeholder Glass Scar startup scene. It is not a 400-unit representative "
        "combat/weather workload, a soak test, a clean-machine result, or release qualification."
    ),
}

pathlib.Path(summary_path).write_text(
    json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8"
)
print(json.dumps(result, indent=2, sort_keys=True))
PY

print "Packaged profile completed."
print "Summary: $summary"
print "Unreal CSV: $raw_csv"
print "Resident-memory samples: $rss_samples"
print "Runtime log: $raw_log"
