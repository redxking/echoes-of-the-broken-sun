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
wrapper_pid=$$
wrapper_pgid="$(LC_ALL=C /bin/ps -o pgid= -p "$wrapper_pid" 2>/dev/null | \
  LC_ALL=C /usr/bin/awk 'NF { print $1; exit }')"
if [[ "$wrapper_pgid" != <-> ]] || (( wrapper_pgid <= 0 )); then
  print -u2 "The wrapper process group could not be captured exactly."
  exit 19
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
abort_record="$evidence_dir/qualification_abort.txt"
evidence_files=()

game_pid=""
game_pgid=""
launched_game_pid=none
launched_game_pgid=none
elapsed=-1
next_sample=-1
abort_reason=unexpected_wrapper_exit
abort_detail="The wrapper exited unexpectedly without a classified reason."
termination_signal=none
cleanup_signal=none
finalizer_pid=""
finalizer_reaped=0
publication_committed=0

cleanup_child() {
  cleanup_signal=none
  if [[ -n "$game_pid" ]] && kill -0 "$game_pid" 2>/dev/null; then
    cleanup_signal=TERM
    kill -TERM "$game_pid" 2>/dev/null || true
    local cleanup_deadline=$(( $(date +%s) + 10 ))
    while kill -0 "$game_pid" 2>/dev/null &&
          (( $(date +%s) < cleanup_deadline )); do
      sleep 1
    done
    if kill -0 "$game_pid" 2>/dev/null; then
      cleanup_signal=TERM+KILL
      kill -KILL "$game_pid" 2>/dev/null || true
    fi
  fi
  if [[ -n "$game_pid" ]]; then
    wait "$game_pid" 2>/dev/null || true
    game_pid=""
  fi
  if [[ -n "$finalizer_pid" ]] && (( ! finalizer_reaped )) &&
     kill -0 "$finalizer_pid" 2>/dev/null; then
    cleanup_signal=TERM
    kill -TERM "$finalizer_pid" 2>/dev/null || true
    local finalizer_cleanup_deadline=$(( $(date +%s) + 10 ))
    while kill -0 "$finalizer_pid" 2>/dev/null &&
          (( $(date +%s) < finalizer_cleanup_deadline )); do
      sleep 1
    done
    if kill -0 "$finalizer_pid" 2>/dev/null; then
      cleanup_signal=TERM+KILL
      kill -KILL "$finalizer_pid" 2>/dev/null || true
    fi
    wait "$finalizer_pid" 2>/dev/null || true
    finalizer_reaped=1
  fi
}

finalize_exit() {
  local exit_status=$?
  local aborted_utc observed_game_pid observed_game_pgid
  trap '' INT TERM HUP QUIT
  aborted_utc="$(date -u +%Y%m%dT%H%M%SZ 2>/dev/null || print unknown)"
  observed_game_pid="$launched_game_pid"
  observed_game_pgid="$launched_game_pgid"
  cleanup_child
  if (( exit_status != 0 )); then
    if [[ ! -f "$abort_record" ]]; then
      /usr/bin/python3 - "$abort_record" "$aborted_utc" "$abort_reason" \
      "$abort_detail" "$wrapper_pid" "$wrapper_pgid" "$observed_game_pid" \
      "$observed_game_pgid" "$elapsed" "$next_sample" "$exit_status" \
      "$cleanup_signal" "$termination_signal" <<'PY_ABORT' || true
import pathlib
import re
import sys


(
    path,
    aborted_utc,
    reason,
    detail,
    wrapper_pid,
    wrapper_pgid,
    game_pid,
    game_pgid,
    elapsed_seconds,
    next_sample_boundary_seconds,
    wrapper_exit_code,
    cleanup_signal,
    termination_signal,
) = sys.argv[1:]
fields = {
    "aborted_utc": aborted_utc,
    "reason": reason,
    "detail": detail,
    "wrapper_pid": wrapper_pid,
    "wrapper_pgid": wrapper_pgid,
    "game_pid": game_pid,
    "game_pgid": game_pgid,
    "elapsed_seconds": elapsed_seconds,
    "next_sample_boundary_seconds": next_sample_boundary_seconds,
    "wrapper_status": "failed",
    "wrapper_exit_code": wrapper_exit_code,
    "cleanup_signal": cleanup_signal,
    "termination_signal": termination_signal,
}
for key, value in fields.items():
    if not value or "\n" in value or "\r" in value:
        raise SystemExit(f"Invalid abort-provenance value: {key}")
if re.fullmatch(r"[0-9]{8}T[0-9]{6}Z", aborted_utc) is None:
    raise SystemExit("Invalid abort-provenance UTC time")
if re.fullmatch(r"[a-z0-9_]+", reason) is None:
    raise SystemExit("Invalid abort-provenance reason")
if any(int(value) <= 0 for value in (wrapper_pid, wrapper_pgid)):
    raise SystemExit("Invalid wrapper process provenance")
if (game_pid == "none") != (game_pgid == "none"):
    raise SystemExit("Incomplete game process provenance")
if game_pid != "none" and any(int(value) <= 0 for value in (game_pid, game_pgid)):
    raise SystemExit("Invalid game process provenance")
if int(elapsed_seconds) < -1 or int(next_sample_boundary_seconds) < -1:
    raise SystemExit("Invalid qualification-boundary provenance")
if not 1 <= int(wrapper_exit_code) <= 255:
    raise SystemExit("Invalid wrapper-exit provenance")
if cleanup_signal not in {"none", "TERM", "TERM+KILL"}:
    raise SystemExit("Invalid cleanup-signal provenance")
if termination_signal not in {"none", "INT", "TERM", "HUP", "QUIT"}:
    raise SystemExit("Invalid termination-signal provenance")
pathlib.Path(path).write_text(
    "".join(f"{key}={value}\n" for key, value in fields.items()),
    encoding="utf-8",
)
PY_ABORT
    fi
    /bin/rm -f "$evidence_manifest" "$evidence_dir/qualification_completion.json"
    if [[ -f "$summary" ]]; then
      /usr/bin/python3 - "$summary" <<'PY_FAILED_SUMMARY' || true
import json
import pathlib
import sys

path = pathlib.Path(sys.argv[1])
result = json.loads(path.read_text(encoding="utf-8"))
result["qualified_one_hour"] = False
path.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
PY_FAILED_SUMMARY
    fi
  fi
  return "$exit_status"
}

abort_run() {
  abort_reason="$1"
  local exit_status="$2"
  shift 2
  abort_detail="$*"
  print -u2 "$*"
  exit "$exit_status"
}

handle_signal() {
  if (( publication_committed )); then
    return 0
  fi
  if [[ -n "$finalizer_pid" ]]; then
    if [[ "$termination_signal" == none ]]; then
      termination_signal="$1"
      abort_reason="external_signal_${1:l}"
      abort_detail="The qualification wrapper received external signal $1 during terminal evidence publication."
      kill -"$1" "$finalizer_pid" 2>/dev/null || true
    fi
    return 0
  fi
  termination_signal="$1"
  abort_reason="external_signal_${1:l}"
  abort_detail="The qualification wrapper received external signal $1."
  exit "$2"
}

wait_for_terminal_finalizer() {
  local observed_status
  while true; do
    wait "$finalizer_pid"
    observed_status=$?
    if ! kill -0 "$finalizer_pid" 2>/dev/null; then
      finalizer_reaped=1
      return "$observed_status"
    fi
  done
}

trap finalize_exit EXIT
trap 'handle_signal INT 130' INT
trap 'handle_signal TERM 143' TERM
trap 'handle_signal HUP 129' HUP
trap 'handle_signal QUIT 131' QUIT

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
  if ! /usr/bin/python3 "$preflight_verifier_copy" \
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
    --preflight-verifier-sha256 "$preflight_verifier_sha256" >/dev/null; then
    abort_run preflight_binding_verification_failed 6 \
      "The one-hour preflight binding could not be verified."
  fi
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

forbidden_pattern='\[ECHOES_[A-Z0-9_]*FAILED\]|\[ECHOES_MATCH_FINISHED\]|\[ECHOES_MATCH_PAUSE\] paused=true|\[ECHOES_SIM_TIME_CLAMP\]|\[ECHOES_SIM_VIEW_SYNC_FAILED\]|\[ECHOES_BOOT_INCOMPLETE\]|\[ECHOES_BOOT_NO_SUBSYSTEM\]|\[ECHOES_STRESS_READY\]|\[ECHOES_STRESS_ORDERS_READY\]|\[ECHOES_STRESS_COMBAT_ACTIVE\]|Fatal error:|LowLevelFatalError|Assertion failed:|GPU Crashed|Out of memory|Ran out of memory|Out of video memory|Segmentation fault|signal 11|ensure condition failed|unhandled exception|SIGABRT|SIGBUS|signal 6|signal 10'
exec_commands="r.SetRes 1280x720w,r.VSync 0,t.MaxFPS 60,t.IdleWhenNotForeground 0,sg.ResolutionQuality 100,sg.ViewDistanceQuality 1,sg.AntiAliasingQuality 1,sg.ShadowQuality 1,sg.GlobalIlluminationQuality 1,sg.ReflectionQuality 1,sg.PostProcessQuality 1,sg.TextureQuality 1,sg.EffectsQuality 1,sg.FoliageQuality 1,sg.ShadingQuality 1,sg.LandscapeQuality 1,r.AntiAliasingMethod 2"

log_scan_offset=0
log_scan_device=0
log_scan_inode=0
log_scan_size=0
log_scan_total_bytes=0
log_scan_partial_bytes=0
forbidden_count=0
ready_count=0
qualified_count=0
final_tick=0

scan_appended_log() {
  local scan_result
  local new_offset new_device new_inode new_size new_total_bytes
  local new_partial_bytes new_forbidden_count new_ready_count
  local new_qualified_count new_final_tick
  local numeric_value

  if ! scan_result="$(
    /usr/bin/python3 - "$raw_log" "$log_scan_offset" "$log_scan_device" \
      "$log_scan_inode" "$log_scan_size" "$log_scan_total_bytes" \
      "$log_scan_partial_bytes" "$forbidden_count" "$ready_count" \
      "$qualified_count" "$final_tick" "$forbidden_pattern" <<'PY_SCAN'
import os
import re
import stat
import sys


(
    path,
    raw_offset,
    raw_device,
    raw_inode,
    raw_previous_size,
    raw_total_bytes,
    raw_previous_partial_bytes,
    raw_forbidden_count,
    raw_ready_count,
    raw_qualified_count,
    raw_final_tick,
    forbidden_pattern,
) = sys.argv[1:]
(
    offset,
    expected_device,
    expected_inode,
    previous_size,
    total_bytes,
    previous_partial_bytes,
    forbidden_count,
    ready_count,
    qualified_count,
    final_tick,
) = map(
    int,
    (
        raw_offset,
        raw_device,
        raw_inode,
        raw_previous_size,
        raw_total_bytes,
        raw_previous_partial_bytes,
        raw_forbidden_count,
        raw_ready_count,
        raw_qualified_count,
        raw_final_tick,
    ),
)
if any(
    value < 0
    for value in (
        offset,
        expected_device,
        expected_inode,
        previous_size,
        total_bytes,
        previous_partial_bytes,
        forbidden_count,
        ready_count,
        qualified_count,
        final_tick,
    )
):
    raise SystemExit("The sustained log monitor state cannot be negative")
if (expected_device == 0) != (expected_inode == 0):
    raise SystemExit("The sustained log identity state is malformed")
if previous_partial_bytes > 65536:
    raise SystemExit("The sustained log partial-line state is oversized")
if previous_size - offset != previous_partial_bytes:
    raise SystemExit("The sustained log cursor and partial-line state disagree")
if total_bytes != offset:
    raise SystemExit("The sustained log processed-byte state disagrees with its cursor")

with open(path, "rb") as handle:
    descriptor_state = os.fstat(handle.fileno())
    path_state = os.stat(path, follow_symlinks=False)
    if not stat.S_ISREG(descriptor_state.st_mode) or not stat.S_ISREG(
        path_state.st_mode
    ):
        raise SystemExit("The sustained log is not a regular file")
    identity = (descriptor_state.st_dev, descriptor_state.st_ino)
    if identity != (path_state.st_dev, path_state.st_ino):
        raise SystemExit("The sustained log changed identity while opening")
    if expected_device and identity != (expected_device, expected_inode):
        raise SystemExit("The sustained log changed identity during monitoring")
    size = descriptor_state.st_size
    if size < previous_size:
        raise SystemExit("The sustained log shrank during monitoring")
    if offset > size:
        raise SystemExit("The sustained log cursor exceeds the file size")
    handle.seek(offset)
    chunk = handle.read(size - offset)
    if len(chunk) != size - offset:
        raise SystemExit("The sustained log changed length during monitoring")

complete_length = chunk.rfind(b"\n") + 1
complete = chunk[:complete_length]
new_offset = offset + complete_length
partial_bytes = len(chunk) - complete_length
if partial_bytes > 65536:
    raise SystemExit("The sustained log contains an oversized partial line")
total_bytes += complete_length
forbidden_count += len(
    re.findall(forbidden_pattern.encode("utf-8"), complete, re.IGNORECASE)
)
ready_count += complete.count(b"[ECHOES_STRESS_SUSTAINED_READY]")
qualified_count += complete.count(b"[ECHOES_STRESS_SUSTAINED_QUALIFIED]")
heartbeat_ticks = re.findall(
    rb"\[ECHOES_STRESS_SUSTAINED_HEARTBEAT\][^\r\n]*\btick=([0-9]+)\b",
    complete,
)
if heartbeat_ticks:
    final_tick = int(heartbeat_ticks[-1])
print(
    new_offset,
    identity[0],
    identity[1],
    size,
    total_bytes,
    partial_bytes,
    forbidden_count,
    ready_count,
    qualified_count,
    final_tick,
)
PY_SCAN
  )"; then
    return 1
  fi

  IFS=' ' read -r new_offset new_device new_inode new_size new_total_bytes \
    new_partial_bytes new_forbidden_count new_ready_count new_qualified_count \
    new_final_tick <<< "$scan_result"
  for numeric_value in "$new_offset" "$new_device" "$new_inode" "$new_size" \
    "$new_total_bytes" "$new_partial_bytes" "$new_forbidden_count" \
    "$new_ready_count" "$new_qualified_count" "$new_final_tick"; do
    if [[ "$numeric_value" != <-> ]]; then
      return 1
    fi
  done
  if (( new_offset < log_scan_offset || new_size < log_scan_size ||
        new_total_bytes < log_scan_total_bytes || new_partial_bytes > 65536 ||
        new_total_bytes != new_offset ||
        new_size - new_offset != new_partial_bytes ||
        new_forbidden_count < forbidden_count || new_ready_count < ready_count ||
        new_qualified_count < qualified_count )); then
    return 1
  fi

  log_scan_offset=$new_offset
  log_scan_device=$new_device
  log_scan_inode=$new_inode
  log_scan_size=$new_size
  log_scan_total_bytes=$new_total_bytes
  log_scan_partial_bytes=$new_partial_bytes
  forbidden_count=$new_forbidden_count
  ready_count=$new_ready_count
  qualified_count=$new_qualified_count
  final_tick=$new_final_tick
}

monotonic_nanoseconds() {
  /usr/bin/python3 -c 'import ctypes; library=ctypes.CDLL(None); clock=library.mach_absolute_time; clock.restype=ctypes.c_uint64; scale=(ctypes.c_uint32*2)(); library.mach_timebase_info(scale); print(clock()*scale[0]//scale[1])'
}

sample_elapsed_after_observation() {
  /usr/bin/python3 - "$@" <<'PY_SAMPLE_TIMING'
import sys
import ctypes


def host_monotonic_nanoseconds() -> int:
    library = ctypes.CDLL(None)
    clock = library.mach_absolute_time
    clock.restype = ctypes.c_uint64
    scale = (ctypes.c_uint32 * 2)()
    library.mach_timebase_info(scale)
    if scale[0] <= 0 or scale[1] <= 0:
        raise SystemExit("The mach monotonic timebase is invalid")
    return clock() * scale[0] // scale[1]


if len(sys.argv) not in {3, 4}:
    raise SystemExit("Expected ready time, sample boundary, and optional observation time")
ready_nanoseconds = int(sys.argv[1])
sample_boundary_seconds = int(sys.argv[2])
observed_nanoseconds = (
    int(sys.argv[3]) if len(sys.argv) == 4 else host_monotonic_nanoseconds()
)
if ready_nanoseconds < 0 or sample_boundary_seconds < 0:
    raise SystemExit("Sample timing state cannot be negative")
target_nanoseconds = ready_nanoseconds + sample_boundary_seconds * 1_000_000_000
if observed_nanoseconds < target_nanoseconds:
    raise SystemExit("Process sample was observed before its requested boundary")
lateness_nanoseconds = observed_nanoseconds - target_nanoseconds
elapsed_seconds = (observed_nanoseconds - ready_nanoseconds) // 1_000_000_000
print(elapsed_seconds, lateness_nanoseconds)
if lateness_nanoseconds > 2_000_000_000:
    raise SystemExit(15)
PY_SAMPLE_TIMING
}

sustained_completion_ready() {
  (( elapsed >= duration_seconds &&
     next_sample > last_sample_boundary &&
     final_tick >= required_tick &&
     (! requires_qualification || qualified_count == 1) ))
}

if ! launch_monotonic_ns="$(monotonic_nanoseconds)" ||
   [[ "$launch_monotonic_ns" != <-> ]]; then
  abort_run monotonic_clock_unavailable 15 \
    "The monotonic clock could not be read before launch."
fi
"$binary" /Engine/Maps/Entry \
  -game -unattended -nop4 -nosplash -nosound \
  -EchoesStress400Sustained \
  "-EchoesSaveGameDirectory=$runtime_state/save-games" \
  "-UserDir=$runtime_state/user-dir" \
  -stdout -FullStdOutLogOutput \
  -windowed -ForceRes -ResX=1280 -ResY=720 \
  "-ExecCmds=$exec_commands" > "$raw_log" 2>&1 &
game_pid=$! game_pgid=$wrapper_pgid \
  launched_game_pid=$! launched_game_pgid=$wrapper_pgid
observed_game_pgid="$(LC_ALL=C /bin/ps -o pgid= -p "$game_pid" 2>/dev/null | \
  LC_ALL=C /usr/bin/awk 'NF { print $1; exit }')"
if [[ "$observed_game_pgid" != <-> ]] ||
   (( observed_game_pgid <= 0 || observed_game_pgid != game_pgid )); then
  abort_run game_process_group_capture_failed 19 \
    "The packaged game process group could not be captured exactly."
fi

ready_monotonic_ns=0
while (( ready_monotonic_ns == 0 )); do
  if ! kill -0 "$game_pid" 2>/dev/null; then
    set +e
    wait "$game_pid"
    game_status=$?
    set -e
    game_pid=""
    abort_run game_exited_before_readiness 7 \
      "The packaged game exited before readiness with status $game_status. Inspect: $raw_log"
  fi
  if ! scan_appended_log; then
    abort_run startup_log_monitor_failed 8 \
      "The packaged game log could not be incrementally monitored before readiness. Inspect: $raw_log"
  fi
  if (( forbidden_count > 0 )); then
    abort_run startup_forbidden_marker 8 \
      "The packaged game emitted a rejected marker before readiness. Inspect: $raw_log"
  fi
  if (( ready_count > 1 )); then
    abort_run duplicate_ready_marker 8 \
      "The packaged game emitted duplicate readiness markers. Inspect: $raw_log"
  fi
  if (( ready_count == 1 )); then
    if ! ready_monotonic_ns="$(monotonic_nanoseconds)" ||
       [[ "$ready_monotonic_ns" != <-> ]]; then
      abort_run monotonic_clock_unavailable 15 \
        "The monotonic clock could not be read at readiness."
    fi
    break
  fi
  if ! now_monotonic_ns="$(monotonic_nanoseconds)" ||
     [[ "$now_monotonic_ns" != <-> ]] ||
     (( now_monotonic_ns < launch_monotonic_ns )); then
    abort_run monotonic_clock_unavailable 15 \
      "The monotonic clock became invalid before readiness."
  fi
  startup_elapsed=$(((now_monotonic_ns - launch_monotonic_ns) / 1000000000))
  if (( startup_elapsed >= startup_timeout )); then
    abort_run startup_timeout 9 \
      "The sustained fixture did not become ready within ${startup_timeout} seconds. Inspect: $raw_log"
  fi
  sleep 1
done

next_sample=0
completion_reached=0
required_tick=$((duration_seconds * 20))
requires_qualification=0
if (( duration_seconds >= 3600 )); then
  requires_qualification=1
fi
while (( ! completion_reached )); do
  if ! kill -0 "$game_pid" 2>/dev/null; then
    set +e
    wait "$game_pid"
    game_status=$?
    set -e
    game_pid=""
    abort_run game_exited_before_sustained_boundary 10 \
      "The packaged game exited before the sustained boundary with status $game_status. Inspect: $raw_log"
  fi

  if ! now_monotonic_ns="$(monotonic_nanoseconds)" ||
     [[ "$now_monotonic_ns" != <-> ]] ||
     (( now_monotonic_ns < ready_monotonic_ns )); then
    abort_run monotonic_clock_unavailable 15 \
      "The monotonic clock became invalid during qualification."
  fi
  elapsed_nanoseconds=$((now_monotonic_ns - ready_monotonic_ns))
  elapsed=$((elapsed_nanoseconds / 1000000000))
  if (( next_sample <= last_sample_boundary &&
        elapsed_nanoseconds >= next_sample * 1000000000 )); then
    process_sample="$(LC_ALL=C /bin/ps -o rss=,%cpu= -p "$game_pid" | LC_ALL=C /usr/bin/awk 'NF >= 2 { print $1 "," $2 }' || true)"
    if [[ -z "$process_sample" ]]; then
      abort_run process_sample_unavailable 15 \
        "Process sampling failed while the packaged game was expected to be alive."
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
      abort_run process_sample_invalid 15 \
        "Process sampling returned a non-numeric or invalid resource value."
    fi
    set +e
    sample_timing="$(sample_elapsed_after_observation \
      "$ready_monotonic_ns" "$next_sample")"
    sample_timing_status=$?
    set -e
    if (( sample_timing_status == 15 )); then
      IFS=' ' read -r sample_elapsed sample_lateness_nanoseconds <<< "$sample_timing"
      if [[ "$sample_elapsed" == <-> ]]; then
        elapsed=$sample_elapsed
      fi
      abort_run process_sample_cadence_missed 15 \
        "Process sampling missed its ${sample_interval}-second cadence."
    elif (( sample_timing_status != 0 )); then
      abort_run sample_timing_monitor_failed 15 \
        "Process sample timing could not be validated."
    fi
    IFS=' ' read -r sample_elapsed sample_lateness_nanoseconds <<< "$sample_timing"
    if [[ "$sample_elapsed" != <-> || "$sample_lateness_nanoseconds" != <-> ]] ||
       (( sample_elapsed < next_sample || sample_elapsed > next_sample + 2 )); then
      abort_run sample_timing_state_invalid 15 \
        "Process sample timing returned malformed state."
    fi
    /usr/bin/awk -v elapsed="$sample_elapsed" -v rss_kib="$rss_kib" \
      -v cpu="$cpu_percent" \
      'BEGIN { printf "%d,%.3f,%.3f\n", elapsed, rss_kib / 1024.0, cpu }' >> "$samples"
    elapsed=$sample_elapsed
    next_sample=$((next_sample + sample_interval))
  fi

  if ! scan_appended_log; then
    abort_run runtime_log_monitor_failed 11 \
      "The sustained fixture log could not be incrementally monitored. Inspect: $raw_log"
  fi
  if (( forbidden_count > 0 )); then
    abort_run rejected_runtime_marker 11 \
      "The sustained fixture emitted a rejected runtime marker. Inspect: $raw_log"
  fi
  if (( qualified_count > 1 )); then
    abort_run duplicate_qualification_marker 12 \
      "The sustained fixture emitted duplicate qualification markers. Inspect: $raw_log"
  fi
  if ! now_monotonic_ns="$(monotonic_nanoseconds)" ||
     [[ "$now_monotonic_ns" != <-> ]] ||
     (( now_monotonic_ns < ready_monotonic_ns )); then
    abort_run monotonic_clock_unavailable 15 \
      "The monotonic clock became invalid after log monitoring."
  fi
  elapsed=$(((now_monotonic_ns - ready_monotonic_ns) / 1000000000))
  if sustained_completion_ready; then
    completion_reached=1
    break
  fi
  if (( elapsed > duration_seconds + completion_grace )); then
    abort_run qualification_boundary_missed 13 \
      "The sustained fixture missed its tick or qualification boundary within the grace period. Inspect: $raw_log"
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
  abort_run game_termination_timeout 14 \
    "The packaged game did not terminate within 30 seconds after the accepted boundary."
fi
set +e
wait "$game_pid"
game_status=$?
set -e
game_pid=""
print "termination_status=$game_status" >> "$metadata"
if (( game_status != 0 && game_status != 143 )); then
  abort_run unexpected_game_termination_status 14 \
    "The packaged game returned unexpected status $game_status after requested termination."
fi

if ! /usr/bin/python3 "$project_root/Scripts/validate_sustained_soak_log.py" \
  --log "$raw_log" \
  --duration-seconds "$duration_seconds" \
  --json-output "$validation"; then
  abort_run full_log_validation_failed 17 \
    "The completed sustained runtime log failed authoritative validation."
fi

if ! /usr/bin/python3 - "$samples" "$validation" "$summary" "$duration_seconds" \
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
then
  abort_run sustained_measurement_summary_failed 18 \
    "The sustained process measurements or configured budgets failed validation."
fi

if ! verify_source_binding || ! verify_package_integrity; then
  abort_run post_run_source_or_package_verification_failed 16 \
    "Post-run source or package integrity verification failed."
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
abort_reason=evidence_finalization_failed
abort_detail="The terminal evidence finalizer could not execute."
/usr/bin/python3 "$evidence_finalizer_copy" \
  --staging-dir "$evidence_dir" \
  --final-dir "$final_evidence_dir" \
  --trusted-preflight-verifier "$project_root/Scripts/validate_sustained_preflight.py" \
  --abort-provenance "$abort_record" \
  --abort-reason "$abort_reason" \
  --game-pid "$launched_game_pid" \
  --game-pgid "$launched_game_pgid" \
  --elapsed-seconds "$elapsed" \
  --next-sample-boundary-seconds "$next_sample" \
  --cleanup-signal none \
  --wrapper-pid "$wrapper_pid" \
  --wrapper-pgid "$wrapper_pgid" \
  "${finalizer_arguments[@]}" \
  --terminal &
finalizer_pid=$!
set +e
wait_for_terminal_finalizer
finalizer_status=$?
set -e

# The finalizer's exclusive rename is the commit point. Once that directory
# exists and staging no longer does, a late signal cannot revoke the result.
if [[ -d "$final_evidence_dir" && ! -e "$evidence_dir" ]]; then
  publication_committed=1
  finalizer_pid=""
  exit 0
fi
finalizer_pid=""
if (( finalizer_status == 0 )); then
  abort_run terminal_publication_state_invalid 20 \
    "The terminal finalizer exited successfully without the atomic evidence publication."
fi
if [[ "$termination_signal" != none && ! -f "$abort_record" ]]; then
  case "$termination_signal" in
    INT) finalizer_status=130 ;;
    TERM) finalizer_status=143 ;;
    HUP) finalizer_status=129 ;;
    QUIT) finalizer_status=131 ;;
  esac
fi
if (( finalizer_status < 1 || finalizer_status > 255 )); then
  finalizer_status=1
fi
abort_detail="The terminal evidence finalizer exited with status $finalizer_status before publication."
exit "$finalizer_status"
