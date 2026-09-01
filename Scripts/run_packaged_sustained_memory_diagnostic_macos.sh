#!/bin/zsh
set -euo pipefail

project_root="${0:A:h:h}"
if (( $# != 1 )); then
  print -u2 "Usage: ${0:t} /Volumes/Seagate\\ Game\\ Archive/EchoesOfTheBrokenSun/BuildArtifacts/SustainedMemoryDiagnostics/unique-final-directory"
  exit 2
fi

package_source_commit="ae2e8494e7ccc23524871ec2754916140ff4ab01"
package_root="/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/BuildArtifacts/Packages/Mac-Development-20260831T083359Z-m13-pooling-ae2e849"
app="$package_root/EchoesOfTheBrokenSun.app"
binary="$app/Contents/MacOS/EchoesOfTheBrokenSun"
plist="$app/Contents/Info.plist"
manifest="$package_root/EchoesOfTheBrokenSun.manifest.txt"
manifest_digest="$package_root/EchoesOfTheBrokenSun.manifest.sha256"
normal_smoke="$package_root/EchoesOfTheBrokenSun.normal-startup-smoke.log"
stress_smoke="$package_root/EchoesOfTheBrokenSun.legacy-stress-startup-smoke.log"
external_package_seal="/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/BuildArtifacts/PackageGateSeals/Mac-Development-20260831T083359Z-m13-pooling-ae2e849-Seal-20260831T085230Z/EchoesOfTheBrokenSun.external-package-gate-seal.txt"
diagnostic_root="/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun/BuildArtifacts/SustainedMemoryDiagnostics"
unreal_insights="/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealInsights.app/Contents/MacOS/UnrealInsights"
unreal_insights_version_file="/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealInsights.version"
duration_seconds=600
sample_interval=5
last_sample_boundary=600
startup_timeout=120
completion_grace=120
insights_timeout=120
game_cleanup_timeout=10
publisher_timeout=1800
publisher_interrupt_grace=10
publisher_kill_grace=5
expected_package_manifest_sha256="c020390d2a13bb2e47991f1b56a8b42eb6a7c9595055277b85728ef385ad1260"
expected_package_manifest_digest_sha256="0f479c60df6c426c054b3bedd65ed48e4b9d58518987aed30f51b1f254f5b94c"
expected_package_executable_sha256="2702302a90d42b22895fefd02fe030868aa2e23a31c4e76a3bb0690e44d68922"
expected_package_external_seal_sha256="ff5ab774a1737c2cc8c15185a59a580955edc7bc657dc51b2e5ac345762fe949"
expected_runtime_validator_sha256="4d27edc7276a7d7e97afd91fc2789cdefebb11f8d69c52df499e154008826b93"
expected_package_verifier_sha256="a7bbc35ee84f95b072d1f919436af5b980fe2fc06620f2b5923b22ea52062c70"
expected_unreal_insights_sha256="b475184d29fbf4d4b342402929e5e7d3fab779cb77a3b615ac41607ea09fccbb"
expected_diff_paths=$'Docs/Archive/SetupAndBuild.md\nScripts/run_packaged_sustained_memory_diagnostic_macos.sh\nScripts/validate_sustained_memory_diagnostic.py\nTests/Content/test_sustained_memory_diagnostic.py'
diagnostic_diff_paths="Docs/Archive/SetupAndBuild.md,Scripts/run_packaged_sustained_memory_diagnostic_macos.sh,Scripts/validate_sustained_memory_diagnostic.py,Tests/Content/test_sustained_memory_diagnostic.py"
claim_boundary="This capture is diagnostic evidence only. It does not pass or replace the 600-second preflight, establish a root cause, prove leak freedom, qualify a one-hour run, or establish release readiness."

requested_final="$1"
if [[ "$requested_final" != /* ]]; then
  print -u2 "The diagnostic evidence directory must be an explicit absolute path."
  exit 2
fi
final_name="${requested_final:t}"
if ! print -r -- "$final_name" | /usr/bin/grep -Eq '^[A-Za-z0-9][A-Za-z0-9._-]{0,127}$'; then
  print -u2 "The diagnostic evidence basename is unsafe."
  exit 2
fi
/bin/mkdir -p "$diagnostic_root"
if [[ ! -d "$diagnostic_root" || -L "$diagnostic_root" ||
      "${requested_final:h:A}" != "${diagnostic_root:A}" ]]; then
  print -u2 "Diagnostic evidence must be a direct child of: $diagnostic_root"
  exit 2
fi
final_evidence_dir="${diagnostic_root:A}/$final_name"
if [[ -e "$final_evidence_dir" || -L "$final_evidence_dir" ]]; then
  print -u2 "Refusing to overwrite an existing diagnostic path: $final_evidence_dir"
  exit 4
fi

read_remote_main() {
  local remote_record
  remote_record="$(GIT_TERMINAL_PROMPT=0 /usr/bin/git -C "$project_root" \
    ls-remote --exit-code origin refs/heads/main 2>/dev/null)" || return 1
  print "${remote_record%%[[:space:]]*}"
}

diagnostic_tooling_commit="$(/usr/bin/git -C "$project_root" rev-parse --verify HEAD 2>/dev/null || print unknown)"

verify_tooling_binding() {
  local observed_commit observed_origin observed_remote observed_status observed_paths
  observed_commit="$(/usr/bin/git -C "$project_root" rev-parse --verify HEAD 2>/dev/null || print unknown)"
  observed_origin="$(/usr/bin/git -C "$project_root" rev-parse --verify origin/main 2>/dev/null || print unknown)"
  observed_remote="$(read_remote_main || print unknown)"
  observed_status="$(/usr/bin/git -C "$project_root" status --porcelain --untracked-files=all 2>/dev/null || print status-unavailable)"
  observed_paths="$(/usr/bin/git -C "$project_root" diff --name-only \
    "$package_source_commit..$observed_commit" 2>/dev/null | LC_ALL=C /usr/bin/sort)"
  [[ "$observed_commit" == "$diagnostic_tooling_commit" &&
     "$observed_commit" != "$package_source_commit" &&
     "$observed_origin" == "$observed_commit" &&
     "$observed_remote" == "$observed_commit" &&
     -z "$observed_status" &&
     "$observed_paths" == "$expected_diff_paths" ]] || return 1
  /usr/bin/git -C "$project_root" merge-base --is-ancestor \
    "$package_source_commit" "$observed_commit" >/dev/null 2>&1 || return 1
  /usr/bin/git -C "$project_root" diff --check \
    "$package_source_commit..$observed_commit" >/dev/null 2>&1
}

verify_local_retention() {
  /usr/bin/python3 -I - "$package_root" "${external_package_seal:h}" <<'PY_RETENTION'
import os
import pathlib
import stat
import sys

for raw_root in sys.argv[1:]:
    root = pathlib.Path(raw_root)
    info = root.lstat()
    if not stat.S_ISDIR(info.st_mode) or root.is_symlink():
        raise SystemExit(f"unsafe retained root: {root}")
    for directory, directory_names, file_names in os.walk(root, followlinks=False):
        for item in [
            pathlib.Path(directory),
            *(pathlib.Path(directory) / name for name in directory_names + file_names),
        ]:
            item_info = item.lstat()
            if stat.S_ISLNK(item_info.st_mode):
                raise SystemExit(f"retained package/seal contains a symlink: {item}")
            if item_info.st_mode & 0o222:
                raise SystemExit(f"retained package/seal is writable: {item}")
            if not (item_info.st_flags & stat.UF_IMMUTABLE):
                raise SystemExit(f"retained package/seal lacks uchg: {item}")
PY_RETENTION
}

verify_package_integrity() {
  local package_verifier_path="${1:-$project_root/Scripts/verify_packaged_app.py}"
  local runtime_validator_path="${2:-$project_root/Scripts/validate_sustained_soak_log.py}"
  local observed_manifest observed_manifest_digest observed_executable
  local observed_external_seal observed_runtime_validator observed_package_verifier
  [[ -d "$package_root" && ! -L "$package_root" &&
     -d "$app" && ! -L "$app" &&
     -x "$binary" && ! -L "$binary" &&
     -f "$plist" && -f "$manifest" && -f "$manifest_digest" &&
     -f "$normal_smoke" && -f "$stress_smoke" &&
     -f "$external_package_seal" && ! -L "$external_package_seal" ]] || return 1
  observed_manifest="$(/usr/bin/shasum -a 256 "$manifest" | /usr/bin/awk '{print $1}')"
  observed_manifest_digest="$(/usr/bin/shasum -a 256 "$manifest_digest" | /usr/bin/awk '{print $1}')"
  observed_executable="$(/usr/bin/shasum -a 256 "$binary" | /usr/bin/awk '{print $1}')"
  observed_external_seal="$(/usr/bin/shasum -a 256 "$external_package_seal" | /usr/bin/awk '{print $1}')"
  observed_runtime_validator="$(/usr/bin/shasum -a 256 "$runtime_validator_path" | /usr/bin/awk '{print $1}')"
  observed_package_verifier="$(/usr/bin/shasum -a 256 "$package_verifier_path" | /usr/bin/awk '{print $1}')"
  [[ "$observed_manifest" == "$expected_package_manifest_sha256" &&
     "$observed_manifest_digest" == "$expected_package_manifest_digest_sha256" &&
     "$observed_executable" == "$expected_package_executable_sha256" &&
     "$observed_external_seal" == "$expected_package_external_seal_sha256" &&
     "$observed_runtime_validator" == "$expected_runtime_validator_sha256" &&
     "$observed_package_verifier" == "$expected_package_verifier_sha256" ]] || return 1
  [[ "$(/usr/bin/awk -F= '$1 == "source_commit" { print $2; exit }' "$manifest")" == "$package_source_commit" &&
     "$(/usr/bin/awk -F= '$1 == "origin_main" { print $2; exit }' "$manifest")" == "$package_source_commit" &&
     "$(/usr/bin/awk -F= '$1 == "remote_main" { print $2; exit }' "$manifest")" == "$package_source_commit" &&
     "$(/usr/bin/awk -F= '$1 == "source_tree" { print $2; exit }' "$manifest")" == clean &&
     "$(/usr/bin/awk -F= '$1 == "source_binding" { print $2; exit }' "$manifest")" == clean-pushed-main &&
     "$(/usr/bin/awk -F= '$1 == "configuration" { print $2; exit }' "$manifest")" == Development &&
     "$(/usr/bin/awk -F= '$1 == "platform" { print $2; exit }' "$manifest")" == Mac-arm64 ]] || return 1
  /usr/bin/codesign --verify --deep --strict "$app" >/dev/null 2>&1 || return 1
  /usr/bin/python3 -I "$package_verifier_path" \
    --app "$app" --manifest "$manifest" \
    --manifest-digest "$manifest_digest" >/dev/null || return 1
  verify_local_retention
}

git_blob_sha256() {
  local relative_path="$1"
  /usr/bin/git -C "$project_root" cat-file blob \
    "$diagnostic_tooling_commit:$relative_path" | \
    /usr/bin/shasum -a 256 | /usr/bin/awk '{print $1}'
}

verify_retained_tooling_copies() {
  local head_runner_sha256 head_validator_sha256
  local retained_runner_sha256 retained_validator_sha256
  local retained_runtime_validator_sha256 retained_package_verifier_sha256
  head_runner_sha256="$(git_blob_sha256 Scripts/run_packaged_sustained_memory_diagnostic_macos.sh)" || return 1
  head_validator_sha256="$(git_blob_sha256 Scripts/validate_sustained_memory_diagnostic.py)" || return 1
  retained_runner_sha256="$(/usr/bin/shasum -a 256 "$runner_copy" | /usr/bin/awk '{print $1}')"
  retained_validator_sha256="$(/usr/bin/shasum -a 256 "$validator_copy" | /usr/bin/awk '{print $1}')"
  retained_runtime_validator_sha256="$(/usr/bin/shasum -a 256 "$runtime_validator_copy" | /usr/bin/awk '{print $1}')"
  retained_package_verifier_sha256="$(/usr/bin/shasum -a 256 "$package_verifier_copy" | /usr/bin/awk '{print $1}')"
  [[ "$retained_runner_sha256" == "$head_runner_sha256" &&
     "$retained_validator_sha256" == "$head_validator_sha256" &&
     "$retained_runtime_validator_sha256" == "$expected_runtime_validator_sha256" &&
     "$retained_package_verifier_sha256" == "$expected_package_verifier_sha256" ]]
}

verify_trace_launcher_identity() {
  local observed_hash observed_version observed_build_id
  [[ -x "$unreal_insights" && ! -L "$unreal_insights" &&
     -f "$unreal_insights_version_file" && ! -L "$unreal_insights_version_file" ]] || return 1
  observed_hash="$(/usr/bin/shasum -a 256 "$unreal_insights" | /usr/bin/awk '{print $1}')"
  IFS=' ' read -r observed_version observed_build_id <<< "$(/usr/bin/python3 -I - "$unreal_insights_version_file" <<'PY_VERSION'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as handle:
    version = json.load(handle)
print(
    f'{version["MajorVersion"]}.{version["MinorVersion"]}.{version["PatchVersion"]}',
    version["BuildId"],
)
PY_VERSION
)"
  [[ "$observed_hash" == "$expected_unreal_insights_sha256" &&
     "$observed_version" == 5.8.2 && "$observed_build_id" == 55116800 ]]
}

if ! verify_tooling_binding; then
  print -u2 "The diagnostic tooling must be a clean pushed descendant whose package-source diff contains only the four authorized diagnostic files."
  exit 6
fi
if ! verify_package_integrity; then
  print -u2 "The sealed ae2e849 package, manifest, signature, dependency scripts, or external package seal failed verification."
  exit 5
fi
if ! verify_trace_launcher_identity; then
  print -u2 "The installed UE 5.8.2 Unreal Insights launcher executable or version is missing or drifted."
  exit 5
fi

wrapper_pid=$$
wrapper_pgid="$(LC_ALL=C /bin/ps -o pgid= -p "$wrapper_pid" 2>/dev/null | LC_ALL=C /usr/bin/awk 'NF { print $1; exit }')"
if [[ "$wrapper_pgid" != <-> ]] || (( wrapper_pgid <= 0 )); then
  print -u2 "The diagnostic wrapper process group could not be captured."
  exit 19
fi

staging_evidence_dir="$(/usr/bin/mktemp -d "$diagnostic_root/.${final_name}.incomplete.XXXXXX")"
evidence_dir="$staging_evidence_dir"
raw_log="$evidence_dir/packaged_sustained_memory_diagnostic.log"
samples="$evidence_dir/process_samples.csv"
metadata="$evidence_dir/run_metadata.txt"
trace_file="$evidence_dir/echoes-memory.utrace"
launch_command="$evidence_dir/launch_command.txt"
insights_log="$evidence_dir/unreal-insights-parse.log"
trace_instructions="$evidence_dir/trace_analysis_instructions.txt"
runtime_state="$evidence_dir/runtime-state"
runtime_state_inventory="$evidence_dir/runtime_state_inventory.json"
player_save_dir="$project_root/Saved/SaveGames"
player_save_before="$evidence_dir/player-save-before.manifest"
player_save_after="$evidence_dir/player-save-after.manifest"
runner_copy="$evidence_dir/run_packaged_sustained_memory_diagnostic_macos.used.sh"
validator_copy="$evidence_dir/validate_sustained_memory_diagnostic.used.py"
runtime_validator_copy="$evidence_dir/validate_sustained_soak_log.used.py"
package_verifier_copy="$evidence_dir/verify_packaged_app.used.py"
game_group_handshake="$evidence_dir/.game-process-group-handshake"
game_pid=""
game_pgid=""
insights_pid=""
publisher_pid=""
publisher_reaped=0
publisher_deadline=0
publisher_signal_deadline=0
publisher_kill_deadline=0
launched_game_pid=none
launched_game_pgid=none
game_status=none
game_group_kill_escalated=0
elapsed=-1
next_sample=-1
termination_signal=none
cleanup_signal=none
publication_committed=0
abort_reason=unexpected_wrapper_exit
abort_detail="The diagnostic wrapper exited unexpectedly."

spawn_game_in_isolated_group() {
  /usr/bin/python3 -I - "$game_group_handshake" "$binary" "${launch_arguments[@]}" <<'PY_GAME_LAUNCH' > "$raw_log" 2>&1 &
import os
import sys

os.setsid()
pid = os.getpid()
pgid = os.getpgrp()
if pid != pgid:
    raise RuntimeError("isolated game process is not its process-group leader")
with open(sys.argv[1], "x", encoding="ascii", newline="\n") as handshake:
    handshake.write(f"{pid} {pgid}\n")
    handshake.flush()
    os.fsync(handshake.fileno())
os.execv(sys.argv[2], sys.argv[2:])
PY_GAME_LAUNCH
  game_pid=$!
  launched_game_pid=$game_pid
}

launch_game_in_isolated_group() {
  spawn_game_in_isolated_group
  capture_game_process_group
}

read_game_group_handshake() {
  local capture_deadline handshake_pid handshake_pgid handshake_record
  [[ "$game_pid" == <-> ]] && (( game_pid > 1 )) || return 1
  capture_deadline=$(( $(/bin/date +%s) + 5 ))
  while true; do
    if [[ -f "$game_group_handshake" && ! -L "$game_group_handshake" ]]; then
      handshake_record="$(<"$game_group_handshake")"
      IFS=' ' read -r handshake_pid handshake_pgid <<< "$handshake_record"
      if [[ "$handshake_pid" == <-> && "$handshake_pgid" == <-> &&
            "$handshake_pid" == "$game_pid" &&
            "$handshake_pgid" == "$game_pid" &&
            "$handshake_pgid" != "$wrapper_pgid" &&
            "$handshake_pgid" != "$wrapper_pid" ]] && (( handshake_pgid > 1 )); then
        game_pgid="$handshake_pgid"
        launched_game_pgid="$handshake_pgid"
        return 0
      fi
    fi
    kill -0 "$game_pid" 2>/dev/null || break
    (( $(/bin/date +%s) < capture_deadline )) || break
    /bin/sleep 0.05
  done
  return 1
}

capture_game_process_group() {
  local capture_deadline observed_pgid observed_state
  read_game_group_handshake || return 1
  capture_deadline=$(( $(/bin/date +%s) + 5 ))
  while game_leader_is_live; do
    IFS=' ' read -r observed_pgid observed_state <<< \
      "$(LC_ALL=C /bin/ps -o pgid=,state= -p "$game_pid" 2>/dev/null | LC_ALL=C /usr/bin/awk 'NF { print $1, $2; exit }')"
    if [[ "$observed_pgid" == "$game_pgid" && -n "$observed_state" &&
          "$observed_state" != Z* ]]; then
      /bin/unlink "$game_group_handshake" || return 1
      return 0
    fi
    (( $(/bin/date +%s) < capture_deadline )) || break
    /bin/sleep 0.05
  done
  return 1
}

game_leader_is_live() {
  local observed_state
  [[ -n "$game_pid" ]] && kill -0 "$game_pid" 2>/dev/null || return 1
  observed_state="$(LC_ALL=C /bin/ps -o state= -p "$game_pid" 2>/dev/null | /usr/bin/awk 'NF { print $1; exit }')"
  [[ -z "$observed_state" || "$observed_state" != Z* ]]
}

game_group_is_safe() {
  [[ "$launched_game_pid" == <-> && "$launched_game_pgid" == <-> &&
     "$game_pgid" == <-> &&
     "$launched_game_pid" == "$launched_game_pgid" &&
     "$game_pgid" == "$launched_game_pgid" &&
     "$game_pgid" != "$wrapper_pgid" && "$game_pgid" != "$wrapper_pid" ]] &&
    (( launched_game_pid > 1 && game_pgid > 1 ))
}

game_group_has_live_members() {
  local process_table
  game_group_is_safe || return 2
  process_table="$(LC_ALL=C /bin/ps -axo pgid=,state= 2>/dev/null)" || return 0
  print -r -- "$process_table" | LC_ALL=C /usr/bin/awk \
    -v target="$game_pgid" \
    '$1 == target && $2 !~ /^Z/ { found=1; exit } END { exit !found }'
}

signal_game_group() {
  local requested_signal="$1"
  game_group_is_safe || return 1
  /bin/kill -"$requested_signal" "-$game_pgid" 2>/dev/null || true
}

reap_game_leader() {
  local observed_status=0
  if [[ -n "$game_pid" ]]; then
    wait "$game_pid" 2>/dev/null || observed_status=$?
    game_status=$observed_status
    game_pid=""
  fi
}

terminate_game_group() {
  local group_cleanup_deadline
  game_group_kill_escalated=0
  game_group_is_safe || return 1
  if game_group_has_live_members; then
    cleanup_signal=TERM
    signal_game_group TERM
    group_cleanup_deadline=$(( $(/bin/date +%s) + game_cleanup_timeout ))
    while game_group_has_live_members &&
          (( $(/bin/date +%s) < group_cleanup_deadline )); do
      /bin/sleep 1
    done
    if game_group_has_live_members; then
      game_group_kill_escalated=1
      cleanup_signal=TERM+KILL
      signal_game_group KILL
      # Escalation is bounded, but ownership is retained until every non-zombie
      # member of the captured launch group is gone and the leader is reaped.
      while game_group_has_live_members; do
        /bin/sleep 1
      done
    fi
  fi
  reap_game_leader
}

wait_for_terminal_publisher() {
  local now observed_state observed_status=0
  while true; do
    if [[ -d "$final_evidence_dir" && ! -L "$final_evidence_dir" &&
          ! -e "$evidence_dir" && ! -L "$evidence_dir" ]]; then
      publication_committed=1
    fi
    if ! kill -0 "$publisher_pid" 2>/dev/null; then
      break
    fi
    observed_state="$(LC_ALL=C /bin/ps -o state= -p "$publisher_pid" 2>/dev/null | /usr/bin/awk 'NF { print $1; exit }')"
    if [[ "$observed_state" == Z* ]]; then
      break
    fi
    now="$(/bin/date +%s)"
    if (( publisher_signal_deadline > 0 && now >= publisher_signal_deadline )); then
      cleanup_signal=TERM+KILL
      kill -KILL "$publisher_pid" 2>/dev/null || true
      publisher_signal_deadline=0
      publisher_kill_deadline=$(( now + publisher_kill_grace ))
    elif (( publisher_deadline > 0 && now >= publisher_deadline )); then
      abort_reason=terminal_publication_timeout
      abort_detail="The terminal diagnostic publisher exceeded its bounded publication window."
      cleanup_signal=TERM
      kill -TERM "$publisher_pid" 2>/dev/null || true
      publisher_deadline=0
      publisher_signal_deadline=$(( now + publisher_interrupt_grace ))
    fi
    /bin/sleep 1
  done
  wait "$publisher_pid" 2>/dev/null || observed_status=$?
  publisher_reaped=1
  return "$observed_status"
}

cleanup_children() {
  local child_cleanup_deadline
  if ! game_group_is_safe && [[ -n "$game_pid" ]]; then
    read_game_group_handshake || true
  fi
  if game_group_is_safe; then
    terminate_game_group || true
  elif [[ -n "$game_pid" ]]; then
    cleanup_signal=TERM
    kill -TERM "$game_pid" 2>/dev/null || true
    child_cleanup_deadline=$(( $(/bin/date +%s) + game_cleanup_timeout ))
    while kill -0 "$game_pid" 2>/dev/null &&
          (( $(/bin/date +%s) < child_cleanup_deadline )); do
      /bin/sleep 1
    done
    if kill -0 "$game_pid" 2>/dev/null; then
      cleanup_signal=TERM+KILL
      kill -KILL "$game_pid" 2>/dev/null || true
    fi
    reap_game_leader
  fi
  if [[ -n "$insights_pid" ]]; then
    if kill -0 "$insights_pid" 2>/dev/null; then
      cleanup_signal=TERM
      kill -TERM "$insights_pid" 2>/dev/null || true
      child_cleanup_deadline=$(( $(/bin/date +%s) + 10 ))
      while kill -0 "$insights_pid" 2>/dev/null &&
            (( $(/bin/date +%s) < child_cleanup_deadline )); do
        /bin/sleep 1
      done
      if kill -0 "$insights_pid" 2>/dev/null; then
        cleanup_signal=TERM+KILL
        kill -KILL "$insights_pid" 2>/dev/null || true
      fi
    fi
    wait "$insights_pid" 2>/dev/null || true
    insights_pid=""
  fi
  if [[ -n "$publisher_pid" ]] && (( ! publisher_reaped )); then
    if [[ -d "$final_evidence_dir" && ! -L "$final_evidence_dir" &&
          ! -e "$evidence_dir" && ! -L "$evidence_dir" ]]; then
      publication_committed=1
    fi
    if kill -0 "$publisher_pid" 2>/dev/null &&
       (( publisher_signal_deadline == 0 && publisher_kill_deadline == 0 )); then
      cleanup_signal=TERM
      kill -TERM "$publisher_pid" 2>/dev/null || true
      publisher_deadline=0
      publisher_signal_deadline=$(( $(/bin/date +%s) + publisher_interrupt_grace ))
    fi
    wait_for_terminal_publisher || true
  fi
  if (( publisher_reaped )); then
    publisher_pid=""
  fi
}

finalize_exit() {
  local exit_status=$?
  trap '' INT TERM HUP QUIT
  cleanup_children
  if (( publication_committed )); then
    exit_status=0
  elif (( exit_status == 0 )); then
    exit_status=20
    abort_reason=publication_not_committed
    abort_detail="The diagnostic wrapper reached exit without exclusive evidence publication."
  fi
  if (( exit_status != 0 )) && [[ -d "$evidence_dir" ]]; then
    if [[ ! -e "$evidence_dir/diagnostic_abort.txt" ]]; then
      {
        print "aborted_utc=$(/bin/date -u +%Y%m%dT%H%M%SZ)"
        print "reason=$abort_reason"
        print "detail=$abort_detail"
        print "wrapper_pid=$wrapper_pid"
        print "wrapper_pgid=$wrapper_pgid"
        print "game_pid=$launched_game_pid"
        print "game_pgid=$launched_game_pgid"
        print "elapsed_seconds=$elapsed"
        print "next_sample_boundary_seconds=$next_sample"
        print "wrapper_exit_code=$exit_status"
        print "cleanup_signal=$cleanup_signal"
        print "termination_signal=$termination_signal"
        print "qualification_eligible=false"
      } > "$evidence_dir/diagnostic_abort.txt" 2>/dev/null || true
    fi
  fi
  trap - EXIT
  exit "$exit_status"
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
  if [[ -d "$final_evidence_dir" && ! -L "$final_evidence_dir" &&
        ! -e "$evidence_dir" && ! -L "$evidence_dir" ]]; then
    publication_committed=1
    return 0
  fi
  if [[ -n "$publisher_pid" ]] && (( ! publisher_reaped )); then
    if [[ "$termination_signal" == none ]]; then
      termination_signal="$1"
      abort_reason="external_signal_${1:l}"
      abort_detail="The memory diagnostic wrapper received external signal $1 during terminal evidence publication."
      kill -"$1" "$publisher_pid" 2>/dev/null || true
      publisher_signal_deadline=$(( $(/bin/date +%s) + publisher_interrupt_grace ))
    else
      cleanup_signal=TERM+KILL
      kill -KILL "$publisher_pid" 2>/dev/null || true
      publisher_kill_deadline=$(( $(/bin/date +%s) + publisher_kill_grace ))
    fi
    return 0
  fi
  termination_signal="$1"
  abort_reason="external_signal_${1:l}"
  abort_detail="The memory diagnostic wrapper received external signal $1."
  exit "$2"
}

trap finalize_exit EXIT
trap 'handle_signal INT 130' INT
trap 'handle_signal TERM 143' TERM
trap 'handle_signal HUP 129' HUP
trap 'handle_signal QUIT 131' QUIT

snapshot_player_saves() {
  local destination="$1"
  /usr/bin/python3 -I - "$player_save_dir" "$destination" <<'PY_SAVE_SNAPSHOT'
import hashlib
import json
import os
import stat
import sys

root, destination = sys.argv[1:]


def signature(info):
    return {
        "device": info.st_dev,
        "inode": info.st_ino,
        "mode": info.st_mode,
        "size": info.st_size,
        "mtime_ns": info.st_mtime_ns,
    }


entries = []
if not os.path.lexists(root):
    entries.append({"path": "", "type": "root-absent"})
else:
    root_before = os.lstat(root)
    if not stat.S_ISDIR(root_before.st_mode):
        raise RuntimeError("Saved/SaveGames exists but is not a directory")

    def walk(directory, relative_directory):
        directory_before = os.lstat(directory)
        entries.append({
            "path": relative_directory,
            "type": "directory",
            **signature(directory_before),
        })
        children = sorted(list(os.scandir(directory)), key=lambda child: os.fsencode(child.name))
        for child in children:
            relative_path = os.path.join(relative_directory, child.name)
            child_info = child.stat(follow_symlinks=False)
            record = {"path": relative_path, **signature(child_info)}
            if stat.S_ISLNK(child_info.st_mode):
                record.update(type="symlink", target=os.readlink(child.path))
                entries.append(record)
            elif stat.S_ISDIR(child_info.st_mode):
                walk(child.path, relative_path)
            elif stat.S_ISREG(child_info.st_mode):
                digest = hashlib.sha256()
                with open(child.path, "rb") as source:
                    for block in iter(lambda: source.read(1024 * 1024), b""):
                        digest.update(block)
                child_after = os.lstat(child.path)
                if signature(child_info) != signature(child_after):
                    raise RuntimeError(f"Save file changed while being hashed: {relative_path!r}")
                record.update(type="file", sha256=digest.hexdigest())
                entries.append(record)
            else:
                record.update(type="other")
                entries.append(record)
        directory_after = os.lstat(directory)
        if signature(directory_before) != signature(directory_after):
            raise RuntimeError(f"Save directory changed during inventory: {relative_directory!r}")

    walk(root, "")
    root_after = os.lstat(root)
    if signature(root_before) != signature(root_after):
        raise RuntimeError("Saved/SaveGames changed during snapshot collection")

with open(destination, "x", encoding="utf-8", newline="\n") as output:
    json.dump(entries, output, ensure_ascii=True, sort_keys=True, separators=(",", ":"))
    output.write("\n")
PY_SAVE_SNAPSHOT
}

monotonic_nanoseconds() {
  /usr/bin/python3 -I -c 'import ctypes; library=ctypes.CDLL(None); clock=library.mach_absolute_time; clock.restype=ctypes.c_uint64; scale=(ctypes.c_uint32*2)(); library.mach_timebase_info(scale); print(clock()*scale[0]//scale[1])'
}

log_scan_device=0
log_scan_inode=0
log_scan_size=0
forbidden_pattern='\[ECHOES_[A-Z0-9_]*FAILED\]|\[ECHOES_STRESS_SUSTAINED_QUALIFIED\]|\[ECHOES_MATCH_FINISHED\]|\[ECHOES_MATCH_PAUSE\] paused=true|\[ECHOES_SIM_TIME_CLAMP\]|\[ECHOES_SIM_VIEW_SYNC_FAILED\]|\[ECHOES_BOOT_INCOMPLETE\]|\[ECHOES_BOOT_NO_SUBSYSTEM\]|\[ECHOES_STRESS_READY\]|\[ECHOES_STRESS_ORDERS_READY\]|\[ECHOES_STRESS_COMBAT_ACTIVE\]|Fatal error:|LowLevelFatalError|Assertion failed:|GPU Crashed|Out of memory|Ran out of memory|Out of video memory|Segmentation fault|signal 11|ensure condition failed|unhandled exception|SIGABRT|SIGBUS|signal 6|signal 10|forcedGc=true'

verify_live_log() {
  local observed_record observed_device observed_inode observed_size
  observed_record="$(/usr/bin/stat -f '%d %i %z' "$raw_log" 2>/dev/null)" || return 1
  IFS=' ' read -r observed_device observed_inode observed_size <<< "$observed_record"
  [[ "$observed_device" == <-> && "$observed_inode" == <-> &&
     "$observed_size" == <-> ]] || return 1
  if (( log_scan_device == 0 )); then
    log_scan_device=$observed_device
    log_scan_inode=$observed_inode
    log_scan_size=$observed_size
  elif (( observed_device != log_scan_device || observed_inode != log_scan_inode ||
          observed_size < log_scan_size )); then
    return 1
  else
    log_scan_size=$observed_size
  fi
  if LC_ALL=C /usr/bin/grep -Eiq "$forbidden_pattern" "$raw_log"; then
    return 2
  fi
}

marker_count() {
  local marker="$1"
  LC_ALL=C /usr/bin/awk -v marker="$marker" 'index($0, marker) { count++ } END { print count + 0 }' "$raw_log"
}

anchor_present() {
  local marker="$1"
  local tick="$2"
  LC_ALL=C /usr/bin/awk -v marker="$marker" -v token="tick=$tick" '
    index($0, marker) && index(" " $0 " ", " " token " ") { found=1 }
    END { exit !found }
  ' "$raw_log"
}

/bin/mkdir -p "$runtime_state/save-games" "$runtime_state/user-dir" "$runtime_state/insights-user-dir"
: > "$raw_log"
print 'elapsed_seconds,rss_mib,cpu_percent' > "$samples"
/bin/cp "$manifest" "$evidence_dir/EchoesOfTheBrokenSun.manifest.txt"
/bin/cp "$manifest_digest" "$evidence_dir/EchoesOfTheBrokenSun.manifest.sha256"
/bin/cp "$normal_smoke" "$evidence_dir/EchoesOfTheBrokenSun.normal-startup-smoke.log"
/bin/cp "$stress_smoke" "$evidence_dir/EchoesOfTheBrokenSun.legacy-stress-startup-smoke.log"
/bin/cp "$project_root/Scripts/run_packaged_sustained_memory_diagnostic_macos.sh" "$runner_copy"
/bin/cp "$project_root/Scripts/validate_sustained_memory_diagnostic.py" "$validator_copy"
/bin/cp "$project_root/Scripts/validate_sustained_soak_log.py" "$runtime_validator_copy"
/bin/cp "$project_root/Scripts/verify_packaged_app.py" "$package_verifier_copy"
if ! verify_retained_tooling_copies; then
  abort_run retained_tooling_binding_failed 16 \
    "The retained diagnostic tools do not match the recorded HEAD blobs or unchanged dependency hashes."
fi

if ! snapshot_player_saves "$player_save_before"; then
  abort_run player_save_snapshot_failed 9 \
    "The real player SaveGames tree could not be captured before the diagnostic."
fi

exec_commands="r.SetRes 1280x720w,r.VSync 0,t.MaxFPS 60,t.IdleWhenNotForeground 0,sg.ResolutionQuality 100,sg.ViewDistanceQuality 1,sg.AntiAliasingQuality 1,sg.ShadowQuality 1,sg.GlobalIlluminationQuality 1,sg.ReflectionQuality 1,sg.PostProcessQuality 1,sg.TextureQuality 1,sg.EffectsQuality 1,sg.FoliageQuality 1,sg.ShadingQuality 1,sg.LandscapeQuality 1,r.AntiAliasingMethod 2"
launch_arguments=(
  /Engine/Maps/Entry
  -game -unattended -nop4 -nosplash -nosound
  -EchoesStress400Sustained
  -EchoesMemoryDiagnostic
  '-trace=default,Memory'
  "-tracefile=$trace_file"
  -tracefiletrunc
  "-EchoesSaveGameDirectory=$runtime_state/save-games"
  "-UserDir=$runtime_state/user-dir"
  -stdout -FullStdOutLogOutput
  -windowed -ForceRes -ResX=1280 -ResY=720
  "-ExecCmds=$exec_commands"
)
/usr/bin/python3 -I - "$launch_command" "$binary" "${launch_arguments[@]}" <<'PY_COMMAND'
import pathlib
import shlex
import sys

path = pathlib.Path(sys.argv[1])
with path.open("x", encoding="utf-8", newline="\n") as output:
    output.write(shlex.join(sys.argv[2:]) + "\n")
PY_COMMAND
if [[ -e "$trace_file" || -L "$trace_file" ]]; then
  abort_run trace_overwrite_refused 4 "The unique staging trace path already exists."
fi

created_utc="$(/bin/date -u +%Y%m%dT%H%M%SZ)"
if ! launch_monotonic_ns="$(monotonic_nanoseconds)" || [[ "$launch_monotonic_ns" != <-> ]]; then
  abort_run monotonic_clock_unavailable 15 "The monotonic clock was unavailable before launch."
fi
if ! launch_game_in_isolated_group; then
  abort_run game_process_group_capture_failed 19 \
    "The packaged game was not observed in its own verified process group after launch."
fi
if ! verify_live_log; then
  abort_run runtime_log_identity_failed 11 "The runtime log could not be bound after launch."
fi

ready_monotonic_ns=0
while (( ready_monotonic_ns == 0 )); do
  if ! game_leader_is_live; then
    set +e
    wait "$game_pid"
    game_status=$?
    set -e
    game_pid=""
    abort_run game_exited_before_readiness 7 \
      "The packaged game exited before sustained readiness with status $game_status."
  fi
  set +e
  verify_live_log
  log_guard_status=$?
  set -e
  if (( log_guard_status == 1 )); then
    abort_run runtime_log_identity_failed 11 "The runtime log changed identity or shrank."
  elif (( log_guard_status == 2 )); then
    abort_run rejected_runtime_marker 11 "The runtime log emitted a forbidden diagnostic marker."
  fi
  ready_count="$(marker_count '[ECHOES_STRESS_SUSTAINED_READY]')"
  if (( ready_count > 1 )); then
    abort_run duplicate_ready_marker 8 "The sustained fixture emitted duplicate readiness markers."
  elif (( ready_count == 1 )); then
    ready_monotonic_ns="$(monotonic_nanoseconds)"
    [[ "$ready_monotonic_ns" == <-> ]] || \
      abort_run monotonic_clock_unavailable 15 "The monotonic clock was unavailable at readiness."
    break
  fi
  now_monotonic_ns="$(monotonic_nanoseconds)"
  [[ "$now_monotonic_ns" == <-> ]] || \
    abort_run monotonic_clock_unavailable 15 "The monotonic clock was unavailable before readiness."
  startup_elapsed=$(((now_monotonic_ns - launch_monotonic_ns) / 1000000000))
  if (( startup_elapsed >= startup_timeout )); then
    abort_run startup_timeout 9 "The sustained fixture did not become ready within 120 seconds."
  fi
  /bin/sleep 1
done

next_sample=0
last_guard_second=-1
while (( next_sample <= last_sample_boundary )); do
  if ! game_leader_is_live; then
    set +e
    wait "$game_pid"
    game_status=$?
    set -e
    game_pid=""
    abort_run game_exited_before_diagnostic_boundary 10 \
      "The packaged game exited before the 600-second diagnostic boundary with status $game_status."
  fi
  now_monotonic_ns="$(monotonic_nanoseconds)"
  if [[ "$now_monotonic_ns" != <-> ]] || (( now_monotonic_ns < ready_monotonic_ns )); then
    abort_run monotonic_clock_unavailable 15 "The monotonic clock drifted during the diagnostic."
  fi
  elapsed_nanoseconds=$((now_monotonic_ns - ready_monotonic_ns))
  elapsed=$((elapsed_nanoseconds / 1000000000))
  if (( elapsed != last_guard_second )); then
    set +e
    verify_live_log
    log_guard_status=$?
    set -e
    if (( log_guard_status == 1 )); then
      abort_run runtime_log_identity_failed 11 "The runtime log changed identity or shrank."
    elif (( log_guard_status == 2 )); then
      abort_run rejected_runtime_marker 11 "The runtime log emitted a forbidden diagnostic marker."
    fi
    last_guard_second=$elapsed
  fi
  target_nanoseconds=$((ready_monotonic_ns + next_sample * 1000000000))
  if (( now_monotonic_ns >= target_nanoseconds )); then
    lateness_nanoseconds=$((now_monotonic_ns - target_nanoseconds))
    if (( lateness_nanoseconds > 2000000000 || elapsed != next_sample )); then
      abort_run process_sample_cadence_missed 15 \
        "Process sampling missed the exact ${sample_interval}-second cadence."
    fi
    process_sample="$(LC_ALL=C /bin/ps -o rss=,%cpu= -p "$game_pid" | LC_ALL=C /usr/bin/awk 'NF >= 2 { print $1 "," $2 }' || true)"
    if [[ -z "$process_sample" ]]; then
      abort_run process_sample_unavailable 15 "PID-specific process sampling failed."
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
      abort_run process_sample_invalid 15 "PID-specific process sampling returned an invalid value."
    fi
    /usr/bin/awk -v elapsed="$next_sample" -v rss_kib="$rss_kib" -v cpu="$cpu_percent" \
      'BEGIN { printf "%d,%.3f,%.3f\n", elapsed, rss_kib / 1024.0, cpu }' >> "$samples"
    next_sample=$((next_sample + sample_interval))
  else
    /bin/sleep 0.2
  fi
done

anchor_deadline_ns=$((ready_monotonic_ns + (duration_seconds + completion_grace) * 1000000000))
while true; do
  set +e
  verify_live_log
  log_guard_status=$?
  set -e
  if (( log_guard_status == 1 )); then
    abort_run runtime_log_identity_failed 11 "The runtime log changed identity or shrank."
  elif (( log_guard_status == 2 )); then
    abort_run rejected_runtime_marker 11 "The runtime log emitted a forbidden diagnostic marker."
  fi
  if anchor_present '[ECHOES_STRESS_SUSTAINED_HEARTBEAT]' 2400 &&
     anchor_present '[ECHOES_STRESS_SUSTAINED_MEMORY]' 2400 &&
     anchor_present '[ECHOES_STRESS_SUSTAINED_HEARTBEAT]' 12000 &&
     anchor_present '[ECHOES_STRESS_SUSTAINED_MEMORY]' 12000; then
    break
  fi
  if ! game_leader_is_live; then
    abort_run game_exited_before_runtime_anchors 10 "The game exited before both diagnostic anchor pairs were retained."
  fi
  now_monotonic_ns="$(monotonic_nanoseconds)"
  if [[ "$now_monotonic_ns" != <-> ]] || (( now_monotonic_ns > anchor_deadline_ns )); then
    abort_run runtime_anchor_timeout 13 "The runtime missed tick-2400 or tick-12000 anchors."
  fi
  /bin/sleep 1
done

if ! terminate_game_group; then
  abort_run game_process_group_invalid 19 \
    "The captured packaged-game process group failed its cleanup safety invariant."
fi
if (( game_group_kill_escalated )); then
  abort_run game_termination_timeout 14 "The game did not terminate cleanly after the diagnostic boundary."
fi
if (( game_status != 0 && game_status != 143 )); then
  abort_run unexpected_game_termination_status 14 \
    "The packaged game returned unexpected status $game_status."
fi

if [[ ! -f "$trace_file" || -L "$trace_file" ||
      "$(/usr/bin/stat -f '%z' "$trace_file" 2>/dev/null || print 0)" -lt 4096 ]]; then
  abort_run trace_missing_or_unsafe 21 "The UE Memory trace is missing, linked, or too small."
fi
if ! verify_trace_launcher_identity; then
  abort_run trace_tooling_drift_before_parse 22 \
    "The installed UE 5.8.2 Unreal Insights launcher executable or version drifted before trace analysis."
fi
"$unreal_insights" \
  "-OpenTraceFile=$trace_file" -NoUI -AutoQuit \
  "-ABSLOG=$insights_log" -log \
  "-UserDir=$runtime_state/insights-user-dir" >/dev/null 2>&1 &
insights_pid=$!
insights_deadline=$(( $(/bin/date +%s) + insights_timeout ))
while kill -0 "$insights_pid" 2>/dev/null && (( $(/bin/date +%s) < insights_deadline )); do
  /bin/sleep 1
done
if kill -0 "$insights_pid" 2>/dev/null; then
  kill -TERM "$insights_pid" 2>/dev/null || true
  /bin/sleep 2
  kill -KILL "$insights_pid" 2>/dev/null || true
  wait "$insights_pid" 2>/dev/null || true
  insights_pid=""
  abort_run trace_parse_timeout 22 "UE 5.8 Insights did not finish trace analysis within 120 seconds."
fi
set +e
wait "$insights_pid"
insights_status=$?
set -e
insights_pid=""
if (( insights_status != 0 )) || [[ ! -f "$insights_log" || -L "$insights_log" ]] ||
   ! LC_ALL=C /usr/bin/grep -Fq 'Analysis has completed' "$insights_log" ||
   LC_ALL=C /usr/bin/grep -Fiq 'session analysis failed to start' "$insights_log"; then
  abort_run trace_parse_failed 22 \
    "The Unreal Insights launcher did not exit zero with the required Analysis has completed marker."
fi
if ! verify_trace_launcher_identity; then
  abort_run trace_tooling_drift_after_parse 22 \
    "The installed UE 5.8.2 Unreal Insights launcher executable or version drifted during trace analysis."
fi

if ! snapshot_player_saves "$player_save_after"; then
  abort_run player_save_snapshot_failed 9 \
    "The real player SaveGames tree could not be captured after the diagnostic."
fi
if ! /usr/bin/cmp -s "$player_save_before" "$player_save_after"; then
  /usr/bin/diff -u "$player_save_before" "$player_save_after" \
    > "$evidence_dir/player-save-diff.txt" || true
  abort_run player_save_drift 23 \
    "The diagnostic changed the sampled real player SaveGames tree; no files were deleted or restored."
fi

cat > "$trace_instructions" <<TRACE_INSTRUCTIONS
Echoes packaged sustained memory diagnostic — post-run analysis procedure

This capture requested the UE 5.8 default and Memory trace channels. The automatic
gate loaded the complete .utrace with the installed UE 5.8.2 Unreal Insights app-bundle
launcher and observed launcher exit zero plus its "Analysis has completed" record. The
pinned SHA-256 covers that launcher executable only; dynamically loaded parser modules
were not inventoried. This observation does not establish parser-closure identity or
semantic correctness. The shipped Mac tools do not expose a generic headless command
that independently enumerates every captured channel, so the launch record is requested
configuration provenance, not proof that every requested event arrived.

1. Keep the published capture read-only. Create a separate directory for every derived
   query, snapshot, export, note, and attribution record.
2. Set these paths, then open the retained trace directly from the published evidence:
   TRACE_EVIDENCE_DIR='$final_evidence_dir'
   DERIVED='/absolute/no-spaces/derived-analysis-directory'
   "$unreal_insights" -OpenTraceFile="\$TRACE_EVIDENCE_DIR/echoes-memory.utrace"
3. Confirm that Memory Insights exposes allocation events, tags, heaps, and callstacks.
   If Memory Insights has no usable allocation provider, stop: the channel-presence gate fails.
4. In Log View, confirm the exact retained [ECHOES_STRESS_SUSTAINED_HEARTBEAT] or
   [ECHOES_STRESS_SUSTAINED_MEMORY] lines for tick 2400 and tick 12000. Record their
   trace-relative timestamps as T2400 and T12000. The capture does not retain an
   automated runtime-log-to-trace mapping; if either trace log event is absent, do not
   claim exact 120-second-to-600-second alignment.
5. Pre-create \$DERIVED and \$DERIVED/user, use paths without spaces, then export the
   allocation-growth set with the analyst-confirmed trace-relative timestamps:
   T2400='<trace-relative-seconds-for-tick-2400>'
   T12000='<trace-relative-seconds-for-tick-12000>'
   "$unreal_insights" "-OpenTraceFile=\$TRACE_EVIDENCE_DIR/echoes-memory.utrace" -NoUI -AutoQuit "-ABSLOG=\$DERIVED/export.log" -log "-UserDir=\$DERIVED/user" "-ExecOnAnalysisCompleteCmd=MemoryInsights.ExportAllocs -Rule=AaBf -TimeA=\$T2400 -TimeB=\$T12000 -Output=\$DERIVED/growth.csv -Columns=Size,Tag,AllocThread,AllocFunction,AllocSourceFile,AllocCallstack,Asset,ClassName,Package"
   AaBf selects allocations created between A and B that remain live at B; it is a
   retained-growth query, not by itself a root-cause conclusion.
6. Compare the export's size/tag/callstack groupings, retain and hash growth.csv and
   export.log under \$DERIVED, and record the two source log events and timestamps.
7. Reconcile any suspected allocation family against process_samples.csv and the native
   [ECHOES_STRESS_SUSTAINED_MEMORY] telemetry before proposing a code change.

This is not qualification evidence. The sealed capture permanently retains
root_cause_established=false. Record any later attribution in a separate, hash-bound
derived-analysis record; never edit or reseal the captured evidence.
TRACE_INSTRUCTIONS

if ! /usr/bin/python3 -I "$validator_copy" \
  --evidence-dir "$evidence_dir" --write-runtime-inventory >/dev/null; then
  abort_run runtime_inventory_failed 24 "The isolated runtime-state inventory could not be created."
fi

manifest_sha256="$(/usr/bin/shasum -a 256 "$evidence_dir/EchoesOfTheBrokenSun.manifest.txt" | /usr/bin/awk '{print $1}')"
manifest_digest_sha256="$(/usr/bin/shasum -a 256 "$evidence_dir/EchoesOfTheBrokenSun.manifest.sha256" | /usr/bin/awk '{print $1}')"
runner_sha256="$(/usr/bin/shasum -a 256 "$runner_copy" | /usr/bin/awk '{print $1}')"
diagnostic_validator_sha256="$(/usr/bin/shasum -a 256 "$validator_copy" | /usr/bin/awk '{print $1}')"
runtime_validator_sha256="$(/usr/bin/shasum -a 256 "$runtime_validator_copy" | /usr/bin/awk '{print $1}')"
package_verifier_sha256="$(/usr/bin/shasum -a 256 "$package_verifier_copy" | /usr/bin/awk '{print $1}')"
normal_smoke_sha256="$(/usr/bin/shasum -a 256 "$evidence_dir/EchoesOfTheBrokenSun.normal-startup-smoke.log" | /usr/bin/awk '{print $1}')"
stress_smoke_sha256="$(/usr/bin/shasum -a 256 "$evidence_dir/EchoesOfTheBrokenSun.legacy-stress-startup-smoke.log" | /usr/bin/awk '{print $1}')"
launch_command_sha256="$(/usr/bin/shasum -a 256 "$launch_command" | /usr/bin/awk '{print $1}')"
runtime_log_sha256="$(/usr/bin/shasum -a 256 "$raw_log" | /usr/bin/awk '{print $1}')"
process_samples_sha256="$(/usr/bin/shasum -a 256 "$samples" | /usr/bin/awk '{print $1}')"
trace_sha256="$(/usr/bin/shasum -a 256 "$trace_file" | /usr/bin/awk '{print $1}')"
trace_size_bytes="$(/usr/bin/stat -f '%z' "$trace_file")"
trace_parseability_log_sha256="$(/usr/bin/shasum -a 256 "$insights_log" | /usr/bin/awk '{print $1}')"
trace_analysis_instructions_sha256="$(/usr/bin/shasum -a 256 "$trace_instructions" | /usr/bin/awk '{print $1}')"
player_save_before_sha256="$(/usr/bin/shasum -a 256 "$player_save_before" | /usr/bin/awk '{print $1}')"
player_save_after_sha256="$(/usr/bin/shasum -a 256 "$player_save_after" | /usr/bin/awk '{print $1}')"
runtime_state_inventory_sha256="$(/usr/bin/shasum -a 256 "$runtime_state_inventory" | /usr/bin/awk '{print $1}')"
package_version="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleShortVersionString' "$plist")"
host_model="$(/usr/sbin/sysctl -n hw.model)"
cpu_brand="$(/usr/sbin/sysctl -n machdep.cpu.brand_string)"
physical_memory_bytes="$(/usr/sbin/sysctl -n hw.memsize)"
macos_version="$(/usr/bin/sw_vers -productVersion)"
macos_build="$(/usr/bin/sw_vers -buildVersion)"
host_architecture="$(/usr/bin/uname -m)"
completed_utc="$(/bin/date -u +%Y%m%dT%H%M%SZ)"

{
  print "record_type=Echoes packaged sustained memory diagnostic"
  print "evidence_class=non_qualifying_memory_diagnostic"
  print "fixture=Stress400Sustained"
  print "created_utc=$created_utc"
  print "completed_utc=$completed_utc"
  print "requested_active_seconds=$duration_seconds"
  print "sample_interval_seconds=$sample_interval"
  print "process_sample_count=121"
  print "qualification_eligible=false"
  print "diagnostic_instrumentation=UE5.8_MemoryTrace_full"
  print "forced_gc=false"
  print "forced_gc_log_observed=false"
  print "thresholds_modified=false"
  print "root_cause_established=false"
  print "package_source_commit=$package_source_commit"
  print "diagnostic_tooling_commit=$diagnostic_tooling_commit"
  print "diagnostic_tooling_origin_main=$diagnostic_tooling_commit"
  print "diagnostic_tooling_remote_main=$diagnostic_tooling_commit"
  print "diagnostic_tooling_source_tree=clean"
  print "diagnostic_tooling_binding=clean-pushed-main"
  print "package_source_is_ancestor=true"
  print "diagnostic_diff_paths=$diagnostic_diff_paths"
  print "application=$app"
  print "capture_staging_directory=$staging_evidence_dir"
  print "requested_final_directory=$final_evidence_dir"
  print "configuration=Development"
  print "platform=Mac-arm64"
  print "package_version=$package_version"
  print "package_manifest_sha256=$manifest_sha256"
  print "package_manifest_digest_sha256=$manifest_digest_sha256"
  print "package_executable_sha256=$expected_package_executable_sha256"
  print "package_external_seal_sha256=$expected_package_external_seal_sha256"
  print "runner_sha256=$runner_sha256"
  print "diagnostic_validator_sha256=$diagnostic_validator_sha256"
  print "runtime_validator_sha256=$runtime_validator_sha256"
  print "package_verifier_sha256=$package_verifier_sha256"
  print "normal_startup_smoke_sha256=$normal_smoke_sha256"
  print "legacy_stress_startup_smoke_sha256=$stress_smoke_sha256"
  print "launch_command_sha256=$launch_command_sha256"
  print "runtime_log_sha256=$runtime_log_sha256"
  print "process_samples_sha256=$process_samples_sha256"
  print "trace_sha256=$trace_sha256"
  print "trace_size_bytes=$trace_size_bytes"
  print "trace_channels_requested=default,Memory"
  print "trace_channels_observed=not_independently_enumerated"
  print "trace_parseability=launcher_exit_zero_and_analysis_completed_marker"
  print "trace_parseability_log_sha256=$trace_parseability_log_sha256"
  print "runtime_log_trace_anchor_mapping=analyst_verification_required"
  print "trace_analysis_required=true"
  print "trace_analysis_instructions_sha256=$trace_analysis_instructions_sha256"
  print "tick_2400_anchor_present=true"
  print "tick_12000_anchor_present=true"
  print "player_save_before_sha256=$player_save_before_sha256"
  print "player_save_after_sha256=$player_save_after_sha256"
  print "player_save_unchanged=true"
  print "isolated_save_game_directory_during_capture=$runtime_state/save-games"
  print "isolated_user_directory_during_capture=$runtime_state/user-dir"
  print "runtime_state_inventory_sha256=$runtime_state_inventory_sha256"
  print "termination_status=$game_status"
  print "wrapper_pid=$wrapper_pid"
  print "wrapper_pgid=$wrapper_pgid"
  print "game_pid=$launched_game_pid"
  print "game_pgid=$launched_game_pgid"
  print "host_model=$host_model"
  print "cpu_brand=$cpu_brand"
  print "physical_memory_bytes=$physical_memory_bytes"
  print "macos_version=$macos_version"
  print "macos_build=$macos_build"
  print "host_architecture=$host_architecture"
  print "unreal_insights_path=$unreal_insights"
  print "unreal_insights_sha256=$expected_unreal_insights_sha256"
  print "unreal_insights_sha256_scope=launcher_executable_only"
  print "unreal_insights_parser_module_closure_inventoried=false"
  print "unreal_insights_version=5.8.2"
  print "unreal_insights_build_id=55116800"
  print "claim_boundary=$claim_boundary"
} > "$metadata"

if ! /usr/bin/python3 -I "$validator_copy" \
  --evidence-dir "$evidence_dir" --seal >/dev/null; then
  abort_run diagnostic_validation_failed 25 \
    "The completed capture failed the non-qualifying diagnostic validator."
fi
if ! /usr/bin/python3 -I "$validator_copy" \
  --evidence-dir "$evidence_dir" --verify-seal >/dev/null; then
  abort_run diagnostic_evidence_seal_failed 25 \
    "The exact diagnostic evidence inventory or hashes failed verification."
fi
if ! verify_tooling_binding; then
  abort_run post_run_tooling_binding_failed 16 \
    "The diagnostic tooling checkout became dirty, unpushed, or changed during the run."
fi
if ! verify_package_integrity "$package_verifier_copy" "$runtime_validator_copy"; then
  abort_run post_run_package_verification_failed 16 \
    "The sealed package, manifest, signature, dependencies, or package seal drifted during the run."
fi
if ! /usr/bin/cmp -s "$player_save_before" "$player_save_after"; then
  abort_run post_run_player_save_drift 23 \
    "The final real player SaveGames comparison no longer matches."
fi
if [[ -e "$final_evidence_dir" || -L "$final_evidence_dir" ]]; then
  abort_run final_path_overwrite_refused 4 \
    "The requested final diagnostic path appeared before publication."
fi

abort_reason=diagnostic_publication_failed
abort_detail="The terminal diagnostic publisher could not execute."
/usr/bin/python3 -I "$validator_copy" \
  --evidence-dir "$evidence_dir" \
  --final-dir "$final_evidence_dir" \
  --publish &
publisher_pid=$!
publisher_deadline=$(( $(/bin/date +%s) + publisher_timeout ))
set +e
wait_for_terminal_publisher
publisher_status=$?
set -e
if (( ! publisher_reaped )); then
  abort_run terminal_publisher_not_reaped 26 \
    "The terminal publisher returned control without confirmed child reaping."
fi

# The publisher's exclusive rename is the only commit point. Once final exists
# and staging no longer does, a late signal cannot revoke the diagnostic record.
if [[ -d "$final_evidence_dir" && ! -L "$final_evidence_dir" &&
      ! -e "$evidence_dir" && ! -L "$evidence_dir" ]]; then
  publication_committed=1
  publisher_pid=""
  exit 0
fi
# Reaping, not a pending signal, is the precommit ownership-release boundary.
publisher_pid=""
if (( publisher_status == 0 )); then
  abort_run terminal_publication_state_invalid 26 \
    "The terminal publisher exited successfully without atomic diagnostic publication."
fi
if [[ "$termination_signal" != none ]]; then
  case "$termination_signal" in
    INT) publisher_status=130 ;;
    TERM) publisher_status=143 ;;
    HUP) publisher_status=129 ;;
    QUIT) publisher_status=131 ;;
  esac
fi
if (( publisher_status < 1 || publisher_status > 255 )); then
  publisher_status=1
fi
abort_detail="The terminal diagnostic publisher exited with status $publisher_status before publication."
exit "$publisher_status"
