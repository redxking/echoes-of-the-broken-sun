#!/bin/zsh
set -euo pipefail

project_root="${0:A:h:h}"
if (( $# < 3 || $# > 4 )); then
  print -u2 "Usage: ${0:t} /absolute/path/EchoesOfTheBrokenSun.app /absolute/evidence/directory duration_seconds [/absolute/successful-600s-preflight-evidence]"
  exit 2
fi

app="$1"
evidence_dir="$2"
duration_seconds="$3"
preflight_evidence_dir="${4:-}"
sample_interval=5
minimum_steady_samples=10
startup_timeout=120
completion_grace=120
warmup_seconds=120

if [[ "$app" != /* || "$evidence_dir" != /* ]]; then
  print -u2 "The application and evidence directory must both be explicit absolute paths."
  exit 2
fi
if [[ "$duration_seconds" != <-> || "$duration_seconds" -lt 60 ||
      "$duration_seconds" -gt 3600 ]]; then
  print -u2 "duration_seconds must be an integer from 60 through 3600."
  exit 2
fi
last_sample_boundary=$((duration_seconds / sample_interval * sample_interval))
maximum_warmup_for_samples=$((
  last_sample_boundary - (minimum_steady_samples - 1) * sample_interval
))
if (( warmup_seconds > maximum_warmup_for_samples )); then
  warmup_seconds=$maximum_warmup_for_samples
fi
if (( duration_seconds == 3600 )); then
  run_class=one_hour_qualification
elif (( duration_seconds == 600 )); then
  run_class=ten_minute_preflight
else
  run_class=diagnostic
fi
if [[ "$run_class" == one_hour_qualification ]]; then
  if (( $# != 4 )) || [[ "$preflight_evidence_dir" != /* ]]; then
    print -u2 "A one-hour qualification requires an absolute successful 600-second preflight evidence directory as the fourth argument."
    exit 2
  fi
elif (( $# != 3 )); then
  print -u2 "Only a one-hour qualification accepts a preflight evidence argument."
  exit 2
fi

app="${app:A}"
evidence_dir="${evidence_dir:A}"
final_evidence_dir="$evidence_dir"
if [[ -n "$preflight_evidence_dir" ]]; then
  preflight_evidence_dir="${preflight_evidence_dir:A}"
fi
binary="$app/Contents/MacOS/EchoesOfTheBrokenSun"
plist="$app/Contents/Info.plist"
manifest="${app:h}/EchoesOfTheBrokenSun.manifest.txt"
manifest_digest="${app:h}/EchoesOfTheBrokenSun.manifest.sha256"

read_remote_main() {
  local remote_record
  remote_record="$(GIT_TERMINAL_PROMPT=0 git -C "$project_root" ls-remote --exit-code origin refs/heads/main 2>/dev/null)" || return 1
  print "${remote_record%%[[:space:]]*}"
}

verify_source_binding() {
  local observed_commit observed_origin observed_remote observed_status
  observed_commit="$(git -C "$project_root" rev-parse --verify HEAD 2>/dev/null || print unknown)"
  observed_origin="$(git -C "$project_root" rev-parse --verify origin/main 2>/dev/null || print unknown)"
  observed_remote="$(read_remote_main || print unknown)"
  observed_status="$(git -C "$project_root" status --porcelain --untracked-files=normal 2>/dev/null || print status-unavailable)"
  [[ "$observed_commit" == "$source_commit" &&
     "$observed_origin" == "$source_commit" &&
     "$observed_remote" == "$source_commit" && -z "$observed_status" ]]
}

verify_package_integrity() {
  if ! /usr/bin/codesign --verify --deep --strict "$app"; then
    return 1
  fi
  /usr/bin/python3 "$project_root/Scripts/verify_packaged_app.py" \
    --app "$app" --manifest "$manifest" \
    --manifest-digest "$manifest_digest" >/dev/null
}

if [[ ! -x "$binary" || ! -f "$plist" || ! -f "$manifest" ||
      ! -f "$manifest_digest" ]]; then
  print -u2 "The supplied application is not a manifested Echoes package: $app"
  exit 3
fi
if [[ -e "$evidence_dir" ]]; then
  print -u2 "Refusing to mix evidence with an existing path: $evidence_dir"
  exit 4
fi
if ! verify_package_integrity; then
  print -u2 "The package signature, manifest, file set, or symlink set is invalid: $app"
  exit 5
fi

source_commit="$(/usr/bin/awk -F= '$1 == "source_commit" { print $2; exit }' "$manifest")"
source_tree="$(/usr/bin/awk -F= '$1 == "source_tree" { print $2; exit }' "$manifest")"
source_binding="$(/usr/bin/awk -F= '$1 == "source_binding" { print $2; exit }' "$manifest")"
manifest_origin="$(/usr/bin/awk -F= '$1 == "origin_main" { print $2; exit }' "$manifest")"
manifest_remote="$(/usr/bin/awk -F= '$1 == "remote_main" { print $2; exit }' "$manifest")"
normal_smoke_name="$(/usr/bin/awk -F= '$1 == "normal_startup_smoke" { print $2; exit }' "$manifest")"
stress_smoke_name="$(/usr/bin/awk -F= '$1 == "legacy_stress_startup_smoke" { print $2; exit }' "$manifest")"
configuration="$(/usr/bin/awk -F= '$1 == "configuration" { print $2; exit }' "$manifest")"
platform="$(/usr/bin/awk -F= '$1 == "platform" { print $2; exit }' "$manifest")"
current_commit="$(git -C "$project_root" rev-parse HEAD)"
origin_commit="$(git -C "$project_root" rev-parse origin/main)"
if [[ ${#source_commit} -ne 40 || "$source_commit" == *[^0-9a-f]* ||
      "$source_tree" != clean || "$source_binding" != clean-pushed-main ||
      "$manifest_origin" != "$source_commit" ||
      "$manifest_remote" != "$source_commit" ||
      "$configuration" != Development ||
      "$platform" != Mac-arm64 || "$source_commit" != "$current_commit" ||
      "$source_commit" != "$origin_commit" ]]; then
  print -u2 "The package is not bound to the clean pushed main commit in this checkout."
  exit 6
fi
if ! verify_source_binding; then
  print -u2 "The authoritative checkout must be clean before packaged qualification."
  exit 6
fi

/bin/mkdir -p "${final_evidence_dir:h}"
staging_evidence_dir="$(/usr/bin/mktemp -d \
  "${final_evidence_dir:h}/.${final_evidence_dir:t}.incomplete.XXXXXX")"
evidence_dir="$staging_evidence_dir"
raw_log="$evidence_dir/packaged_sustained_soak.log"
samples="$evidence_dir/process_samples.csv"
validation="$evidence_dir/sustained_log_validation.json"
summary="$evidence_dir/packaged_sustained_soak_summary.json"
metadata="$evidence_dir/run_metadata.txt"
runner_copy="$evidence_dir/soak_packaged_sustained_macos.used.sh"
validator_copy="$evidence_dir/validate_sustained_soak_log.used.py"
packager_copy="$evidence_dir/package_macos.used.sh"
package_verifier_copy="$evidence_dir/verify_packaged_app.used.py"
evidence_finalizer_copy="$evidence_dir/finalize_sustained_evidence.used.py"
preflight_verifier_copy="$evidence_dir/validate_sustained_preflight.used.py"
preflight_binding="$evidence_dir/preflight_binding.json"
preflight_snapshot="$evidence_dir/preflight_evidence_snapshot.zip"
runtime_state="$evidence_dir/runtime-state"
runtime_state_inventory="$evidence_dir/runtime_state_inventory.txt"
evidence_manifest="$evidence_dir/sustained_evidence.sha256"
evidence_files=()

mkdir -p "$runtime_state/save-games" "$runtime_state/user-dir"
: > "$raw_log"
print 'elapsed_seconds,rss_mib,cpu_percent' > "$samples"
/bin/cp "$manifest" "$evidence_dir/"
/bin/cp "$manifest_digest" "$evidence_dir/"
/bin/cp "$project_root/Scripts/soak_packaged_sustained_macos.sh" "$runner_copy"
/bin/cp "$project_root/Scripts/validate_sustained_soak_log.py" "$validator_copy"
/bin/cp "$project_root/Scripts/package_macos.sh" "$packager_copy"
/bin/cp "$project_root/Scripts/verify_packaged_app.py" "$package_verifier_copy"
/bin/cp "$project_root/Scripts/finalize_sustained_evidence.py" "$evidence_finalizer_copy"
/bin/cp "$project_root/Scripts/validate_sustained_preflight.py" "$preflight_verifier_copy"
/bin/cp "${manifest:h}/$normal_smoke_name" "$evidence_dir/$normal_smoke_name"
/bin/cp "${manifest:h}/$stress_smoke_name" "$evidence_dir/$stress_smoke_name"
manifest_sha256="$(/usr/bin/shasum -a 256 "$manifest" | /usr/bin/awk '{print $1}')"
runner_sha256="$(/usr/bin/shasum -a 256 "$runner_copy" | /usr/bin/awk '{print $1}')"
validator_sha256="$(/usr/bin/shasum -a 256 "$validator_copy" | /usr/bin/awk '{print $1}')"
packager_sha256="$(/usr/bin/shasum -a 256 "$packager_copy" | /usr/bin/awk '{print $1}')"
package_verifier_sha256="$(/usr/bin/shasum -a 256 "$package_verifier_copy" | /usr/bin/awk '{print $1}')"
evidence_finalizer_sha256="$(/usr/bin/shasum -a 256 "$evidence_finalizer_copy" | /usr/bin/awk '{print $1}')"
preflight_verifier_sha256="$(/usr/bin/shasum -a 256 "$preflight_verifier_copy" | /usr/bin/awk '{print $1}')"
normal_smoke_sha256="$(/usr/bin/shasum -a 256 "$evidence_dir/$normal_smoke_name" | /usr/bin/awk '{print $1}')"
stress_smoke_sha256="$(/usr/bin/shasum -a 256 "$evidence_dir/$stress_smoke_name" | /usr/bin/awk '{print $1}')"
package_version="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleShortVersionString' "$plist")"
host_model="$(/usr/sbin/sysctl -n hw.model)"
cpu_brand="$(/usr/sbin/sysctl -n machdep.cpu.brand_string)"
physical_memory_bytes="$(/usr/sbin/sysctl -n hw.memsize)"
macos_version="$(/usr/bin/sw_vers -productVersion)"
macos_build="$(/usr/bin/sw_vers -buildVersion)"
host_architecture="$(/usr/bin/uname -m)"
created_utc="$(date -u +%Y%m%dT%H%M%SZ)"
preflight_evidence_manifest_sha256=""
preflight_snapshot_sha256=""
if [[ "$run_class" == one_hour_qualification ]]; then
  /usr/bin/python3 "$preflight_verifier_copy" \
    --preflight-dir "$preflight_evidence_dir" \
    --snapshot-output "$preflight_snapshot" \
    --json-output "$preflight_binding" \
    --artifact EchoesOfTheBrokenSun.app \
    --configuration "$configuration" \
    --platform "$platform" \
    --package-version "$package_version" \
    --source-commit "$source_commit" \
    --manifest-sha256 "$manifest_sha256" \
    --normal-startup-smoke-sha256 "$normal_smoke_sha256" \
    --legacy-stress-startup-smoke-sha256 "$stress_smoke_sha256" \
    --runner-sha256 "$runner_sha256" \
    --validator-sha256 "$validator_sha256" \
    --packager-sha256 "$packager_sha256" \
    --package-verifier-sha256 "$package_verifier_sha256" \
    --evidence-finalizer-sha256 "$evidence_finalizer_sha256" \
    --preflight-verifier-sha256 "$preflight_verifier_sha256" >/dev/null
  preflight_evidence_manifest_sha256="$(/usr/bin/python3 -c \
    'import json,sys; print(json.load(open(sys.argv[1], encoding="utf-8"))["preflight_evidence_manifest_sha256"])' \
    "$preflight_binding")"
  preflight_snapshot_sha256="$(/usr/bin/python3 -c \
    'import json,sys; print(json.load(open(sys.argv[1], encoding="utf-8"))["preflight_snapshot_sha256"])' \
    "$preflight_binding")"
fi
{
  print "fixture=Stress400Sustained"
  print "created_utc=$created_utc"
  print "application=$app"
  print "evidence_directory=$final_evidence_dir"
  print "requested_active_seconds=$duration_seconds"
  print "run_class=$run_class"
  print "source_commit=$source_commit"
  print "origin_main=$origin_commit"
  print "remote_main=$manifest_remote"
  print "source_tree=clean"
  print "source_binding=clean-pushed-main"
  print "package_version=$package_version"
  print "configuration=$configuration"
  print "platform=$platform"
  print "manifest_sha256=$manifest_sha256"
  print "runner_sha256=$runner_sha256"
  print "validator_sha256=$validator_sha256"
  print "packager_sha256=$packager_sha256"
  print "package_verifier_sha256=$package_verifier_sha256"
  print "evidence_finalizer_sha256=$evidence_finalizer_sha256"
  print "preflight_verifier_sha256=$preflight_verifier_sha256"
  print "preflight_evidence_directory=$preflight_evidence_dir"
  print "preflight_evidence_manifest_sha256=$preflight_evidence_manifest_sha256"
  print "preflight_snapshot_sha256=$preflight_snapshot_sha256"
  print "normal_startup_smoke_sha256=$normal_smoke_sha256"
  print "legacy_stress_startup_smoke_sha256=$stress_smoke_sha256"
  print "isolated_save_game_directory=$final_evidence_dir/runtime-state/save-games"
  print "isolated_user_directory=$final_evidence_dir/runtime-state/user-dir"
  print "host_model=$host_model"
  print "cpu_brand=$cpu_brand"
  print "physical_memory_bytes=$physical_memory_bytes"
  print "macos_version=$macos_version"
  print "macos_build=$macos_build"
  print "host_architecture=$host_architecture"
} > "$metadata"

game_pid=""
cleanup_child() {
  if [[ -n "$game_pid" ]] && kill -0 "$game_pid" 2>/dev/null; then
    kill -TERM "$game_pid" 2>/dev/null || true
    local cleanup_deadline=$(( $(date +%s) + 10 ))
    while kill -0 "$game_pid" 2>/dev/null &&
          (( $(date +%s) < cleanup_deadline )); do
      sleep 1
    done
    if kill -0 "$game_pid" 2>/dev/null; then
      kill -KILL "$game_pid" 2>/dev/null || true
    fi
  fi
  if [[ -n "$game_pid" ]]; then
    wait "$game_pid" 2>/dev/null || true
    game_pid=""
  fi
}

finalize_exit() {
  local exit_status=$?
  cleanup_child
  if (( exit_status != 0 )) && [[ -f "$summary" ]]; then
    /usr/bin/python3 - "$summary" <<'PY' || true
import json
import pathlib
import sys

path = pathlib.Path(sys.argv[1])
result = json.loads(path.read_text(encoding="utf-8"))
result["qualified_one_hour"] = False
path.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
completion = path.parent / "qualification_completion.json"
try:
    completion.unlink()
except FileNotFoundError:
    pass
PY
    if (( ${#evidence_files[@]} > 0 )); then
      (cd "$evidence_dir" &&
        /usr/bin/shasum -a 256 "${evidence_files[@]}" > "${evidence_manifest:t}") ||
        /bin/rm -f "$evidence_manifest"
    fi
  fi
  return "$exit_status"
}

trap finalize_exit EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

forbidden_pattern='\[ECHOES_[A-Z0-9_]*FAILED\]|\[ECHOES_MATCH_FINISHED\]|\[ECHOES_MATCH_PAUSE\] paused=true|\[ECHOES_SIM_TIME_CLAMP\]|\[ECHOES_SIM_VIEW_SYNC_FAILED\]|\[ECHOES_BOOT_INCOMPLETE\]|\[ECHOES_BOOT_NO_SUBSYSTEM\]|\[ECHOES_STRESS_READY\]|\[ECHOES_STRESS_ORDERS_READY\]|\[ECHOES_STRESS_COMBAT_ACTIVE\]|Fatal error:|LowLevelFatalError|Assertion failed:|GPU Crashed|Out of memory|Ran out of memory|Out of video memory|Segmentation fault|signal 11|ensure condition failed|unhandled exception|SIGABRT|SIGBUS|signal 6|signal 10'
exec_commands="r.SetRes 1280x720w,r.VSync 0,t.MaxFPS 60,t.IdleWhenNotForeground 0,sg.ResolutionQuality 100,sg.ViewDistanceQuality 1,sg.AntiAliasingQuality 1,sg.ShadowQuality 1,sg.GlobalIlluminationQuality 1,sg.ReflectionQuality 1,sg.PostProcessQuality 1,sg.TextureQuality 1,sg.EffectsQuality 1,sg.FoliageQuality 1,sg.ShadingQuality 1,sg.LandscapeQuality 1,r.AntiAliasingMethod 2"

launch_epoch="$(date +%s)"
"$binary" /Engine/Maps/Entry \
  -game -unattended -nop4 -nosplash -nosound \
  -EchoesStress400Sustained \
  "-EchoesSaveGameDirectory=$runtime_state/save-games" \
  "-UserDir=$runtime_state/user-dir" \
  -stdout -FullStdOutLogOutput \
  -windowed -ForceRes -ResX=1280 -ResY=720 \
  "-ExecCmds=$exec_commands" > "$raw_log" 2>&1 &
game_pid=$!

ready_epoch=0
while (( ready_epoch == 0 )); do
  if ! kill -0 "$game_pid" 2>/dev/null; then
    set +e
    wait "$game_pid"
    game_status=$?
    set -e
    game_pid=""
    print -u2 "The packaged game exited before readiness with status $game_status. Inspect: $raw_log"
    exit 7
  fi
  if /usr/bin/grep -Eqi "$forbidden_pattern" "$raw_log"; then
    print -u2 "The packaged game emitted a rejected marker before readiness. Inspect: $raw_log"
    exit 8
  fi
  ready_count="$(/usr/bin/grep -c '\[ECHOES_STRESS_SUSTAINED_READY\]' "$raw_log" || true)"
  if (( ready_count > 1 )); then
    print -u2 "The packaged game emitted duplicate readiness markers. Inspect: $raw_log"
    exit 8
  fi
  if (( ready_count == 1 )); then
    ready_epoch="$(date +%s)"
    break
  fi
  if (( $(date +%s) - launch_epoch >= startup_timeout )); then
    print -u2 "The sustained fixture did not become ready within ${startup_timeout} seconds. Inspect: $raw_log"
    exit 9
  fi
  sleep 1
done

next_sample=0
completion_reached=0
while (( ! completion_reached )); do
  if ! kill -0 "$game_pid" 2>/dev/null; then
    set +e
    wait "$game_pid"
    game_status=$?
    set -e
    game_pid=""
    print -u2 "The packaged game exited before the sustained boundary with status $game_status. Inspect: $raw_log"
    exit 10
  fi
  if /usr/bin/grep -Eqi "$forbidden_pattern" "$raw_log"; then
    print -u2 "The sustained fixture emitted a rejected runtime marker. Inspect: $raw_log"
    exit 11
  fi

  now_epoch="$(date +%s)"
  elapsed=$((now_epoch - ready_epoch))
  if (( next_sample <= last_sample_boundary && elapsed >= next_sample )); then
    if (( elapsed - next_sample > 2 )); then
      print -u2 "Process sampling missed its ${sample_interval}-second cadence."
      exit 15
    fi
    process_sample="$(/bin/ps -o rss=,%cpu= -p "$game_pid" | /usr/bin/awk 'NF >= 2 { print $1 "," $2 }' || true)"
    if [[ -z "$process_sample" ]]; then
      print -u2 "Process sampling failed while the packaged game was expected to be alive."
      exit 15
    fi
    rss_kib="${process_sample%%,*}"
    cpu_percent="${process_sample#*,}"
    if ! /usr/bin/awk -v rss_kib="$rss_kib" -v cpu="$cpu_percent" '
      BEGIN {
        valid_rss = rss_kib ~ /^[0-9]+$/ && rss_kib + 0 > 0
        valid_cpu = cpu ~ /^[0-9]+([.][0-9]+)?$/ && cpu + 0 >= 0
        exit !(valid_rss && valid_cpu)
      }
    '; then
      print -u2 "Process sampling returned a non-numeric or invalid resource value."
      exit 15
    fi
    /usr/bin/awk -v elapsed="$elapsed" -v rss_kib="$rss_kib" -v cpu="$cpu_percent" \
      'BEGIN { printf "%d,%.3f,%.3f\n", elapsed, rss_kib / 1024.0, cpu }' >> "$samples"
    next_sample=$((next_sample + sample_interval))
  fi

  final_tick="$(/usr/bin/awk '
    /\[ECHOES_STRESS_SUSTAINED_HEARTBEAT\]/ {
      for (field_index = 1; field_index <= NF; ++field_index) {
        if ($field_index ~ /^tick=[0-9]+$/) {
          split($field_index, pair, "="); tick = pair[2]
        }
      }
    }
    END { print tick + 0 }
  ' "$raw_log")"
  qualified_count="$(/usr/bin/grep -c '\[ECHOES_STRESS_SUSTAINED_QUALIFIED\]' "$raw_log" || true)"
  if (( qualified_count > 1 )); then
    print -u2 "The sustained fixture emitted duplicate qualification markers. Inspect: $raw_log"
    exit 12
  fi
  required_tick=$((duration_seconds * 20))
  requires_qualification=0
  if (( duration_seconds >= 3600 )); then
    requires_qualification=1
  fi
  if (( elapsed >= duration_seconds && final_tick >= required_tick &&
        (! requires_qualification || qualified_count == 1) )); then
    completion_reached=1
    break
  fi
  if (( elapsed > duration_seconds + completion_grace )); then
    print -u2 "The sustained fixture missed its tick or qualification boundary within the grace period. Inspect: $raw_log"
    exit 13
  fi
  sleep 1
done

kill -TERM "$game_pid" 2>/dev/null || true
termination_deadline=$(( $(date +%s) + 30 ))
while kill -0 "$game_pid" 2>/dev/null && (( $(date +%s) < termination_deadline )); do
  sleep 1
done
if kill -0 "$game_pid" 2>/dev/null; then
  kill -KILL "$game_pid" 2>/dev/null || true
  wait "$game_pid" 2>/dev/null || true
  game_pid=""
  print -u2 "The packaged game did not terminate within 30 seconds after the accepted boundary."
  exit 14
fi
set +e
wait "$game_pid"
game_status=$?
set -e
game_pid=""
print "termination_status=$game_status" >> "$metadata"
if (( game_status != 0 && game_status != 143 )); then
  print -u2 "The packaged game returned unexpected status $game_status after requested termination."
  exit 14
fi

/usr/bin/python3 "$project_root/Scripts/validate_sustained_soak_log.py" \
  --log "$raw_log" \
  --duration-seconds "$duration_seconds" \
  --json-output "$validation"

/usr/bin/python3 - "$samples" "$validation" "$summary" "$duration_seconds" \
  "$warmup_seconds" "$sample_interval" "$minimum_steady_samples" \
  "$package_version" "$source_commit" "$manifest_sha256" \
  "$runner_sha256" "$validator_sha256" "$packager_sha256" \
  "$package_verifier_sha256" "$evidence_finalizer_sha256" \
  "$preflight_verifier_sha256" "$preflight_evidence_dir" \
  "$preflight_evidence_manifest_sha256" "$preflight_snapshot_sha256" \
  "$normal_smoke_sha256" "$stress_smoke_sha256" \
  "$host_model" "$cpu_brand" "$physical_memory_bytes" "$macos_version" \
  "$macos_build" "$host_architecture" "$run_class" <<'PY'
import csv
import json
import math
import pathlib
import statistics
import sys

(
    samples_path,
    validation_path,
    summary_path,
    duration_seconds,
    warmup_seconds,
    sample_interval,
    minimum_steady_samples,
    package_version,
    source_commit,
    manifest_sha256,
    runner_sha256,
    validator_sha256,
    packager_sha256,
    package_verifier_sha256,
    evidence_finalizer_sha256,
    preflight_verifier_sha256,
    preflight_evidence_directory,
    preflight_evidence_manifest_sha256,
    preflight_snapshot_sha256,
    normal_smoke_sha256,
    stress_smoke_sha256,
    host_model,
    cpu_brand,
    physical_memory_bytes,
    macos_version,
    macos_build,
    host_architecture,
    run_class,
) = sys.argv[1:]
duration_seconds = int(duration_seconds)
warmup_seconds = int(warmup_seconds)
sample_interval = int(sample_interval)
minimum_steady_samples = int(minimum_steady_samples)
with open(samples_path, newline="", encoding="utf-8") as handle:
    observations = [
        (int(row["elapsed_seconds"]), float(row["rss_mib"]), float(row["cpu_percent"]))
        for row in csv.DictReader(handle)
    ]
expected_samples = duration_seconds // sample_interval + 1
if len(observations) != expected_samples:
    raise SystemExit(
        f"Process sample count drifted: expected {expected_samples}, observed {len(observations)}"
    )
for index, (elapsed_value, rss_value, cpu_value) in enumerate(observations):
    expected_elapsed = index * sample_interval
    if not expected_elapsed <= elapsed_value <= expected_elapsed + 2:
        raise SystemExit(
            f"Process sample cadence drifted at index {index}: observed {elapsed_value}"
        )
    if not math.isfinite(rss_value) or not math.isfinite(cpu_value) or rss_value <= 0 or cpu_value < 0:
        raise SystemExit("Process samples contain an invalid resource value")
if observations[0][0] > 2:
    raise SystemExit("Process sampling did not cover the readiness boundary")
if duration_seconds - observations[-1][0] >= sample_interval:
    raise SystemExit("Process sampling did not cover the requested active boundary")
steady = [row for row in observations if row[0] >= warmup_seconds]
if len(steady) < minimum_steady_samples:
    raise SystemExit("Insufficient post-warm-up process samples")
elapsed = [row[0] for row in steady]
rss = [row[1] for row in steady]
cpu = [row[2] for row in steady]
all_rss = [row[1] for row in observations]
all_cpu = [row[2] for row in observations]
mean_elapsed = statistics.fmean(elapsed)
mean_rss = statistics.fmean(rss)
denominator = sum((value - mean_elapsed) ** 2 for value in elapsed)
slope_per_hour = (
    sum((x - mean_elapsed) * (y - mean_rss) for x, y in zip(elapsed, rss))
    / denominator
    * 3600.0
    if denominator
    else 0.0
)
window_size = max(5, len(rss) // 10)
window_growth = statistics.fmean(rss[-window_size:]) - statistics.fmean(rss[:window_size])

def percentile(values, fraction):
    ordered = sorted(values)
    return ordered[math.ceil(fraction * len(ordered)) - 1]

budgets = {
    "structured_sustained_contract": True,
    "sampled_active_window_resident_memory_peak_mib_le_10240": max(all_rss) <= 10240.0,
    "steady_window_growth_mib_le_64": window_growth <= 64.0,
    "steady_linear_growth_mib_per_hour_le_128": slope_per_hour <= 128.0,
}
runtime_contract = json.loads(pathlib.Path(validation_path).read_text(encoding="utf-8"))
all_measured_budgets_pass = all(budgets.values())
runtime_contract_qualified_one_hour = (
    run_class == "one_hour_qualification"
    and bool(runtime_contract["qualified_one_hour"])
)
result = {
    "artifact": "EchoesOfTheBrokenSun.app",
    "fixture": "Stress400Sustained",
    "run_class": run_class,
    "configuration": "Development",
    "platform": "Mac-arm64",
    "package_version": package_version,
    "source_commit": source_commit,
    "manifest_sha256": manifest_sha256,
    "runner_sha256": runner_sha256,
    "validator_sha256": validator_sha256,
    "packager_sha256": packager_sha256,
    "package_verifier_sha256": package_verifier_sha256,
    "evidence_finalizer_sha256": evidence_finalizer_sha256,
    "preflight_verifier_sha256": preflight_verifier_sha256,
    "preflight_evidence_directory": preflight_evidence_directory or None,
    "preflight_evidence_manifest_sha256": preflight_evidence_manifest_sha256 or None,
    "preflight_snapshot_sha256": preflight_snapshot_sha256 or None,
    "normal_startup_smoke_sha256": normal_smoke_sha256,
    "legacy_stress_startup_smoke_sha256": stress_smoke_sha256,
    "host": {
        "model": host_model,
        "cpu": cpu_brand,
        "physical_memory_bytes": int(physical_memory_bytes),
        "macos_version": macos_version,
        "macos_build": macos_build,
        "architecture": host_architecture,
    },
    "requested_active_seconds": duration_seconds,
    "warmup_seconds": warmup_seconds,
    "samples": len(observations),
    "steady_samples": len(steady),
    "runtime_contract": runtime_contract,
    "runtime_contract_qualified_one_hour": runtime_contract_qualified_one_hour,
    "sampled_active_window_resident_memory_mib": {
        "peak": round(max(all_rss), 6),
        "steady_mean": round(mean_rss, 6),
        "steady_p95": round(percentile(rss, 0.95), 6),
        "steady_peak": round(max(rss), 6),
        "steady_window_growth": round(window_growth, 6),
        "steady_linear_slope_mib_per_hour": round(slope_per_hour, 6),
    },
    "sampled_active_window_cpu_percent": {
        "peak": round(max(all_cpu), 6),
        "steady_mean": round(statistics.fmean(cpu), 6),
        "steady_p95": round(percentile(cpu, 0.95), 6),
        "steady_peak": round(max(cpu), 6),
    },
    "budgets": budgets,
    "all_measured_budgets_pass": all_measured_budgets_pass,
    "qualified_one_hour": False,
    "claim_boundary": (
        "Local manifested Mac-arm64 Development package under the deterministic "
        "four-team sustained fixture. This is not notarization, clean-machine, "
        "network transport, broad balance, ordinary-human completion, or release readiness."
    ),
}
pathlib.Path(summary_path).write_text(
    json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8"
)
print(json.dumps(result, indent=2, sort_keys=True))
if not result["all_measured_budgets_pass"]:
    raise SystemExit("One or more sustained process-memory budgets failed")
PY

if ! verify_source_binding || ! verify_package_integrity; then
  print -u2 "Post-run source or package integrity verification failed."
  exit 16
fi
completed_utc="$(date -u +%Y%m%dT%H%M%SZ)"
{
  print "completed_utc=$completed_utc"
  print "post_run_source_commit=$(/usr/bin/git -C "$project_root" rev-parse HEAD)"
  print "post_run_origin_main=$(/usr/bin/git -C "$project_root" rev-parse origin/main)"
  print "post_run_remote_main=$(read_remote_main)"
  print "post_run_source_tree=clean"
  print "post_run_package_integrity=verified"
} >> "$metadata"

{
  print "runtime_state=$final_evidence_dir/runtime-state"
  print "save_game_directory=$final_evidence_dir/runtime-state/save-games"
  print "user_directory=$final_evidence_dir/runtime-state/user-dir"
  runtime_file_count="$(/usr/bin/find "$runtime_state" -type f | /usr/bin/wc -l | /usr/bin/tr -d ' ')"
  print "file_count=$runtime_file_count"
  LC_ALL=C /usr/bin/find "$runtime_state" -type f -print |
    LC_ALL=C /usr/bin/sort |
    while IFS= read -r runtime_file; do
      runtime_relative="${runtime_file#$runtime_state/}"
      runtime_hash="$(/usr/bin/shasum -a 256 "$runtime_file" | /usr/bin/awk '{print $1}')"
      print "$runtime_hash  $runtime_relative"
    done
} > "$runtime_state_inventory"

evidence_files=(
  "${manifest:t}"
  "${manifest_digest:t}"
  "${raw_log:t}"
  "${samples:t}"
  "${validation:t}"
  "${summary:t}"
  "${metadata:t}"
  "${runner_copy:t}"
  "${validator_copy:t}"
  "${packager_copy:t}"
  "${package_verifier_copy:t}"
  "${evidence_finalizer_copy:t}"
  "${preflight_verifier_copy:t}"
  "$normal_smoke_name"
  "$stress_smoke_name"
  "${runtime_state_inventory:t}"
)
if [[ "$run_class" == one_hour_qualification ]]; then
  evidence_files+=("${preflight_binding:t}" "${preflight_snapshot:t}")
fi
finalizer_arguments=()
for evidence_file in "${evidence_files[@]}"; do
  finalizer_arguments+=(--evidence-file "$evidence_file")
done
exec /usr/bin/python3 "$evidence_finalizer_copy" \
  --staging-dir "$evidence_dir" \
  --final-dir "$final_evidence_dir" \
  --trusted-preflight-verifier "$project_root/Scripts/validate_sustained_preflight.py" \
  "${finalizer_arguments[@]}" \
  --terminal
