#!/bin/zsh
set -euo pipefail

zmodload zsh/parameter
zmodload zsh/zselect
zmodload -F zsh/files b:mkdir

umask 077

project_root="${0:A:h:h}"
ue_root="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
editor="$ue_root/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
project="$project_root/EchoesOfTheBrokenSun.uproject"
port="${ECHOES_NETWORK_PORT:-7799}"
evidence_root="${ECHOES_NETWORK_RECONNECT_EVIDENCE_ROOT:-$project_root/BuildArtifacts/NetworkReconnectRuns}"
game_module="$project_root/Binaries/Mac/libUnrealEditor-EchoesOfTheBrokenSun.dylib"
sim_core_module="$project_root/Binaries/Mac/libUnrealEditor-EchoesSimCore.dylib"
editor_target="$project_root/Binaries/Mac/EchoesOfTheBrokenSunEditor.target"
engine_version_file="$ue_root/Engine/Build/Build.version"
lifecycle_self_test="${ECHOES_NETWORK_RECONNECT_LIFECYCLE_SELF_TEST:-}"
lifecycle_result_file="${ECHOES_NETWORK_RECONNECT_LIFECYCLE_RESULT_FILE:-}"
cleanup_term_poll_count=10
cleanup_kill_poll_count=20
cleanup_poll_ticks=5
cleanup_remove_run_dir=false
cleanup_started=false
run_dir=""
run_dir_cleanup_armed=false
run_dir_ownership_resolved=true
pending_signal_name=""
pending_signal_status=0
manifest=""
server_log=""
phase_one_log=""
invalid_log=""
phase_two_log=""
valid_credential_file=""
invalid_credential_file=""
server_user_dir=""
server_save_dir=""
phase_one_user_dir=""
phase_one_save_dir=""
invalid_user_dir=""
invalid_save_dir=""
phase_two_user_dir=""
phase_two_save_dir=""
server_pid=""
server_job_id=""
client_pid=""
client_job_id=""
phase_one_pid=""
phase_one_job_id=""
invalid_pid=""
invalid_job_id=""

case "$lifecycle_self_test" in
  ""|creation-window-int|creation-window-term|initialization-int|initialization-term|ordinary-exit|term-ignoring-child) ;;
  *)
    print -u2 "Unsupported reconnect lifecycle self-test scenario."
    exit 2
    ;;
esac
if [[ -n "$lifecycle_self_test" ]]; then
  if [[ -z "${ECHOES_NETWORK_RECONNECT_EVIDENCE_ROOT:-}" ||
        -z "$lifecycle_result_file" ||
        "$lifecycle_result_file" != /* ||
        -e "$lifecycle_result_file" ||
        ! -d "${lifecycle_result_file:h}" ]]; then
    print -u2 "Lifecycle self-tests require a fresh absolute result file and an explicit evidence root."
    exit 2
  fi
  cleanup_remove_run_dir=true
else
  if [[ ! -x "$editor" ]]; then
    print -u2 "Unreal Editor is not available at: $editor"
    exit 2
  fi
  for required_binary in \
      "$game_module" "$sim_core_module" "$editor_target" \
      "$engine_version_file"; do
    if [[ ! -f "$required_binary" ]]; then
      print -u2 "Required built/runtime provenance file is missing: $required_binary"
      exit 2
    fi
  done
  if [[ "$port" != <-> || "$port" -lt 1024 || "$port" -gt 65535 ]]; then
    print -u2 "ECHOES_NETWORK_PORT must be an integer from 1024 through 65535."
    exit 2
  fi
fi

record_lifecycle_line() {
  local line="$1"
  if [[ -n "$manifest" && -f "$manifest" && ! -L "$manifest" ]]; then
    print -r -- "$line" >> "$manifest" 2>/dev/null || true
  fi
  if [[ -n "$lifecycle_self_test" &&
        -n "$lifecycle_result_file" &&
        ! -L "$lifecycle_result_file" ]]; then
    print -r -- "$line" >> "$lifecycle_result_file" 2>/dev/null || true
  fi
  return 0
}

create_scoped_run_dir() {
  local candidate
  local creation_signal=""
  local mkdir_status
  local run_nonce
  local -i attempt
  case "$lifecycle_self_test" in
    creation-window-int) creation_signal=INT ;;
    creation-window-term) creation_signal=TERM ;;
  esac
  run_dir_ownership_resolved=false
  for (( attempt = 0; attempt < 16; ++attempt )); do
    printf -v run_nonce '%04x%04x%04x%04x%04x%04x%04x%04x' \
      "$RANDOM" "$RANDOM" "$RANDOM" "$RANDOM" \
      "$RANDOM" "$RANDOM" "$RANDOM" "$RANDOM"
    run_dir="$evidence_root/run.$$.${run_nonce}"
    if [[ -n "$creation_signal" ]]; then
      record_lifecycle_line "lifecycle_self_test=$lifecycle_self_test"
      record_lifecycle_line "run_dir_candidate=$run_dir"
    fi
    if [[ -n "$pending_signal_name" ]]; then
      run_dir=""
      run_dir_ownership_resolved=true
      dispatch_pending_signal
    fi
    if builtin mkdir -m 0700 -- "$run_dir"; then
      if [[ -n "$creation_signal" ]]; then
        kill "-$creation_signal" "$$"
        if [[ "$pending_signal_name" != "$creation_signal" ]]; then
          run_dir_cleanup_armed=true
          run_dir_ownership_resolved=true
          print -u2 "Lifecycle creation-window signal was not deferred."
          return 2
        fi
      fi
      run_dir_cleanup_armed=true
      run_dir_ownership_resolved=true
      dispatch_pending_signal
      return 0
    else
      mkdir_status=$?
      candidate="$run_dir"
      run_dir=""
      run_dir_cleanup_armed=false
      if [[ -n "$pending_signal_name" ]]; then
        run_dir_ownership_resolved=true
        dispatch_pending_signal
      fi
      if [[ -e "$candidate" || -L "$candidate" ]]; then
        continue
      fi
      run_dir_ownership_resolved=true
      dispatch_pending_signal
      print -u2 "Could not create the reconnect evidence run directory."
      return "$mkdir_status"
    fi
  done
  run_dir_ownership_resolved=true
  dispatch_pending_signal
  print -u2 "Could not allocate a unique reconnect evidence run directory."
  return 1
}

dispatch_pending_signal() {
  if [[ -z "$pending_signal_name" ]]; then
    return 0
  fi
  handle_signal "$pending_signal_name" "$pending_signal_status"
}

register_process() {
  local process_variable="$1"
  local job_variable="$2"
  local process_id="$3"
  local job_id
  local -a matching_jobs
  matching_jobs=( ${(k)jobstates[(R)*:${process_id}=*]} )
  job_id="${matching_jobs[1]:-}"
  typeset -g "$process_variable=$process_id"
  typeset -g "$job_variable=$job_id"
  if [[ "$process_id" != <-> || "$process_id" -le 1 ||
        "$job_id" != <-> ||
        "${jobstates[$job_id]-}" != *":${process_id}="* ]]; then
    print -u2 "Could not register an owned background job for bounded cleanup."
    return 1
  fi
}

registered_job_active() {
  local process_id="$1"
  local job_id="$2"
  [[ "$process_id" == <-> && "$process_id" -gt 1 &&
     "$job_id" == <-> &&
     "${jobstates[$job_id]-}" == *":${process_id}="* ]]
}

cleanup_registered_child() {
  local process_variable="$1"
  local job_variable="$2"
  local role="$3"
  local process_id="${(P)process_variable}"
  local job_id="${(P)job_variable}"
  local active_job_id
  local exit_status
  local -i attempt
  typeset -g "$process_variable="
  typeset -g "$job_variable="
  if [[ -z "$process_id" && -z "$job_id" ]]; then
    return 0
  fi
  if [[ "$process_id" != <-> || "$process_id" -le 1 ||
        "$job_id" != <-> ]]; then
    record_lifecycle_line "${role}_cleanup_invalid_job_lease=true"
    return 1
  fi
  if registered_job_active "$process_id" "$job_id"; then
    record_lifecycle_line "${role}_cleanup_signal=TERM"
    kill -TERM -- "%${job_id}" 2>/dev/null || true
    for (( attempt = 0; attempt < cleanup_term_poll_count; ++attempt )); do
      registered_job_active "$process_id" "$job_id" || break
      zselect -t "$cleanup_poll_ticks" 2>/dev/null || true
    done
  fi
  if registered_job_active "$process_id" "$job_id"; then
    record_lifecycle_line "${role}_cleanup_signal=KILL"
    kill -KILL -- "%${job_id}" 2>/dev/null || true
    for (( attempt = 0; attempt < cleanup_kill_poll_count; ++attempt )); do
      registered_job_active "$process_id" "$job_id" || break
      zselect -t "$cleanup_poll_ticks" 2>/dev/null || true
    done
  fi
  if registered_job_active "$process_id" "$job_id"; then
    record_lifecycle_line "${role}_cleanup_timeout=true"
    return 1
  fi
  active_job_id="${(k)jobstates[(R)*:${process_id}=*]}"
  if [[ -n "$active_job_id" ]]; then
    record_lifecycle_line "${role}_cleanup_job_lease_mismatch=true"
    return 1
  fi
  if wait "$process_id" 2>/dev/null; then
    exit_status=0
  else
    exit_status=$?
  fi
  record_lifecycle_line "${role}_cleanup_wait_status=$exit_status"
  return 0
}

remove_scoped_run_dir() {
  if [[ "$run_dir_cleanup_armed" != true ]]; then
    return 0
  fi
  if [[ -z "$run_dir" ]]; then
    record_lifecycle_line "run_fixture_cleanup_refused=true"
    return 1
  fi
  if [[ ! -e "$run_dir" && ! -L "$run_dir" ]]; then
    run_dir_cleanup_armed=false
    run_dir=""
    record_lifecycle_line "run_fixture_absent=true"
    return 0
  fi
  if [[ -z "$evidence_root" || -L "$run_dir" ||
        "${run_dir:t}" != run.* ||
        "${run_dir:A:h}" != "${evidence_root:A}" ]]; then
    record_lifecycle_line "run_fixture_cleanup_refused=true"
    return 1
  fi
  if ! /bin/rm -rf -- "$run_dir"; then
    record_lifecycle_line "run_fixture_removed=false"
    return 1
  fi
  run_dir_cleanup_armed=false
  run_dir=""
  record_lifecycle_line "run_fixture_removed=true"
  return 0
}

cleanup_processes() {
  local credential_file
  local cleanup_failed=false
  cleanup_registered_child client_pid client_job_id client || cleanup_failed=true
  cleanup_registered_child phase_one_pid phase_one_job_id phase_one_client || cleanup_failed=true
  cleanup_registered_child invalid_pid invalid_job_id invalid_client || cleanup_failed=true
  cleanup_registered_child server_pid server_job_id authority || cleanup_failed=true
  for credential_file in "$valid_credential_file" "$invalid_credential_file"; do
    if [[ -n "$credential_file" &&
          -n "$run_dir" &&
          "$credential_file" == "$run_dir/"* &&
          ( -e "$credential_file" || -L "$credential_file" ) ]]; then
      if ! /bin/rm -f -- "$credential_file"; then
        cleanup_failed=true
      fi
    elif [[ -n "$credential_file" &&
            ( -e "$credential_file" || -L "$credential_file" ) ]]; then
      record_lifecycle_line "credential_cleanup_refused=true"
      cleanup_failed=true
    fi
  done
  if [[ "$cleanup_remove_run_dir" == true ||
        ( -n "$run_dir" && ( -z "$manifest" || ! -f "$manifest" ) ) ]]; then
    remove_scoped_run_dir || cleanup_failed=true
  fi
  [[ "$cleanup_failed" == false ]]
}

handle_signal() {
  local signal_name="$1"
  local exit_status="$2"
  local cleanup_status=0
  trap '' INT TERM
  if [[ "$run_dir_ownership_resolved" != true ]]; then
    if [[ -z "$pending_signal_name" ]]; then
      pending_signal_name="$signal_name"
      pending_signal_status="$exit_status"
      record_lifecycle_line "creation_window_signal_deferred=$signal_name"
    fi
    return 0
  fi
  if [[ -n "$pending_signal_name" ]]; then
    signal_name="$pending_signal_name"
    exit_status="$pending_signal_status"
    pending_signal_name=""
    pending_signal_status=0
  fi
  trap - EXIT
  cleanup_started=true
  set +e
  if [[ "$lifecycle_self_test" == creation-window-int ||
        "$lifecycle_self_test" == creation-window-term ]]; then
    if [[ "$run_dir_cleanup_armed" == true &&
          -d "$run_dir" && ! -L "$run_dir" ]]; then
      record_lifecycle_line "creation_window_directory_observed=true"
    else
      record_lifecycle_line "creation_window_directory_observed=false"
    fi
  fi
  record_lifecycle_line "run_interrupted_utc=$(/bin/date -u +%Y-%m-%dT%H:%M:%SZ)"
  record_lifecycle_line "result=interrupted"
  record_lifecycle_line "signal=$signal_name"
  cleanup_processes || cleanup_status=$?
  record_lifecycle_line "signal_cleanup_status=$cleanup_status"
  exit "$exit_status"
}

handle_exit() {
  local original_status="$1"
  local cleanup_status=0
  if [[ "$cleanup_started" == true ]]; then
    return 0
  fi
  trap '' INT TERM
  trap - EXIT
  cleanup_started=true
  set +e
  cleanup_processes || cleanup_status=$?
  record_lifecycle_line "exit_cleanup_status=$cleanup_status"
  exit "$original_status"
}

require_process_success() {
  local process_variable="$1"
  local job_variable="$2"
  local role="$3"
  local process_id="${(P)process_variable}"
  local exit_status
  if wait "$process_id" 2>/dev/null; then
    exit_status=0
  else
    exit_status=$?
  fi
  typeset -g "$process_variable="
  typeset -g "$job_variable="
  print "${role}_exit_status=$exit_status" >> "$manifest"
  if (( exit_status != 0 )); then
    print -u2 "The ${role} process exited with status ${exit_status}; expected 0."
    exit 1
  fi
}

trap 'handle_exit $?' EXIT
trap 'handle_signal INT 130' INT
trap 'handle_signal TERM 143' TERM

/bin/mkdir -p "$evidence_root"
create_scoped_run_dir
case "$lifecycle_self_test" in
  initialization-int)
    kill -INT $$
    print -u2 "Lifecycle INT self-test resumed unexpectedly."
    exit 2
    ;;
  initialization-term)
    kill -TERM $$
    print -u2 "Lifecycle TERM self-test resumed unexpectedly."
    exit 2
    ;;
  ordinary-exit)
    record_lifecycle_line "lifecycle_self_test=ordinary-exit"
    exit 37
    ;;
esac
manifest="$run_dir/manifest.txt"
: > "$manifest"
/bin/chmod 0600 "$manifest"
server_log="$run_dir/Server.log"
phase_one_log="$run_dir/ClientPhaseOne.log"
invalid_log="$run_dir/ClientInvalid.log"
phase_two_log="$run_dir/ClientPhaseTwo.log"
valid_credential_file="$run_dir/EchoesResumeCredential.bin"
invalid_credential_file="$run_dir/EchoesInvalidResumeCredential.bin"
server_user_dir="$run_dir/server-user"
server_save_dir="$run_dir/server-saves"
phase_one_user_dir="$run_dir/phase-one-user"
phase_one_save_dir="$run_dir/phase-one-saves"
invalid_user_dir="$run_dir/invalid-user"
invalid_save_dir="$run_dir/invalid-saves"
phase_two_user_dir="$run_dir/phase-two-user"
phase_two_save_dir="$run_dir/phase-two-saves"
/bin/mkdir -p \
  "$server_user_dir" "$server_save_dir" \
  "$phase_one_user_dir" "$phase_one_save_dir" \
  "$invalid_user_dir" "$invalid_save_dir" \
  "$phase_two_user_dir" "$phase_two_save_dir"
: > "$server_log"
: > "$phase_one_log"
: > "$invalid_log"
: > "$phase_two_log"
: > "$valid_credential_file"
print -rn -- '00000000000000000000000000000000' > "$invalid_credential_file"
/bin/chmod 0600 \
  "$server_log" "$phase_one_log" "$invalid_log" "$phase_two_log" \
  "$valid_credential_file" "$invalid_credential_file"

if [[ "$lifecycle_self_test" == term-ignoring-child ]]; then
  lifecycle_ready_file="$run_dir/term-ignoring-child.ready"
  record_lifecycle_line "lifecycle_self_test=term-ignoring-child"
  /bin/zsh -fc '
    trap "" TERM
    print -r -- ready > "$1"
    while true; do :; done
  ' -- "$lifecycle_ready_file" &
  register_process client_pid client_job_id "$!"
  lifecycle_child_ready=false
  for _ in {1..40}; do
    if [[ -f "$lifecycle_ready_file" ]]; then
      lifecycle_child_ready=true
      break
    fi
    /bin/sleep 0.05
  done
  if [[ "$lifecycle_child_ready" != true ]]; then
    print -u2 "TERM-ignoring lifecycle child did not become ready."
    exit 2
  fi
  record_lifecycle_line "term_ignoring_child_pid=$client_pid"
  kill -TERM $$
  print -u2 "Lifecycle TERM self-test resumed unexpectedly."
  exit 2
fi

source_sha="$(git -C "$project_root" rev-parse HEAD)"
source_branch="$(git -C "$project_root" branch --show-current)"
tracked_dirty=false
if [[ -n "$(git -C "$project_root" status --porcelain --untracked-files=all)" ]]; then
  tracked_dirty=true
fi
editor_sha256="$(/usr/bin/shasum -a 256 "$editor" | /usr/bin/awk '{print $1}')"
game_module_sha256="$(/usr/bin/shasum -a 256 "$game_module" | /usr/bin/awk '{print $1}')"
sim_core_module_sha256="$(/usr/bin/shasum -a 256 "$sim_core_module" | /usr/bin/awk '{print $1}')"
editor_target_sha256="$(/usr/bin/shasum -a 256 "$editor_target" | /usr/bin/awk '{print $1}')"
engine_version_sha256="$(/usr/bin/shasum -a 256 "$engine_version_file" | /usr/bin/awk '{print $1}')"
{
  print "run_started_utc=$(/bin/date -u +%Y-%m-%dT%H:%M:%SZ)"
  print "source_sha=$source_sha"
  print "source_branch=$source_branch"
  print "tracked_dirty=$tracked_dirty"
  print "editor_path=$editor"
  print "editor_sha256=$editor_sha256"
  print "game_module_path=$game_module"
  print "game_module_sha256=$game_module_sha256"
  print "sim_core_module_path=$sim_core_module"
  print "sim_core_module_sha256=$sim_core_module_sha256"
  print "editor_target_path=$editor_target"
  print "editor_target_sha256=$editor_target_sha256"
  print "engine_version_file=$engine_version_file"
  print "engine_version_sha256=$engine_version_sha256"
  print "port=$port"
  print "bind=127.0.0.1"
  print "security_posture=development_loopback_only"
  print "process_model=separate_process_localhost"
  print "credential_transport=owner_only_one_use_file_then_reliable_rpc"
  print "credential_path_swap_boundary=same_uid_concurrent_directory_mutation_out_of_scope"
  print "required_process_exit_status=0"
  print "socket_evidence_scope=authority_game_socket_before_clients_start"
  print "claim_boundary=no_identity_no_encryption_no_lan_wan_or_internet_readiness"
} > "$manifest"
/bin/chmod 0600 "$manifest"
if [[ "$tracked_dirty" == true ]]; then
  print -u2 "Reconnect evidence requires a clean tracked source tree: $project_root"
  exit 2
fi

assert_private_file() {
  local candidate="$1"
  local expected_size="$2"
  if [[ ! -f "$candidate" ||
        "$(/usr/bin/stat -f '%u' "$candidate")" != "$(/usr/bin/id -u)" ||
        "$(/usr/bin/stat -f '%Lp' "$candidate")" != "600" ||
        "$(/usr/bin/stat -f '%z' "$candidate")" != "$expected_size" ]]; then
    print -u2 "Credential staging file is not owner-only with the expected bounded size."
    return 1
  fi
}

secret_in_files() {
  local secret="$1"
  shift
  print -rn -- "$secret" | /usr/bin/grep -aFqf - "$@"
}

assert_secret_absent_from_registered_processes() {
  local secret="$1"
  shift
  local process_variable job_variable process_id job_id command_line
  while (( $# > 0 )); do
    process_variable="$1"
    job_variable="$2"
    shift 2
    process_id="${(P)process_variable}"
    job_id="${(P)job_variable}"
    if ! registered_job_active "$process_id" "$job_id"; then
      continue
    fi
    command_line="$(/bin/ps -ww -o command= -p "$process_id" 2>/dev/null || true)"
    if [[ "$command_line" == *"$secret"* ]]; then
      print -u2 "A reconnect credential was exposed in a live process command line."
      return 1
    fi
  done
}

"$editor" "$project" "/Engine/Maps/Entry?listen" \
  -game -unattended -nop4 -nosplash -nullrhi -nosound \
  -MULTIHOME=127.0.0.1 -port="$port" \
  -EchoesAutoStart -EchoesNetworkReconnectSmoke \
  "-UserDir=$server_user_dir" \
  "-EchoesSaveGameDirectory=$server_save_dir" \
  -AbsLog="$server_log" &
register_process server_pid server_job_id "$!"
print "server_pid=$server_pid" >> "$manifest"

server_ready=false
for _ in {1..160}; do
  if /usr/bin/grep -q '\[ECHOES_BOOT_READY\]' "$server_log" &&
     /usr/bin/grep -Eq "GameNetDriver .* listening on port ${port}|IpNetDriver listening on port ${port}" "$server_log"; then
    server_ready=true
    break
  fi
  if ! registered_job_active "$server_pid" "$server_job_id"; then
    break
  fi
  sleep 0.25
done
if [[ "$server_ready" != true ]]; then
  print -u2 "Reconnect listen server did not become ready. Inspect: $server_log"
  exit 3
fi

authority_game_socket_evidence="$(/usr/sbin/lsof -nP -a -p "$server_pid" -iUDP:"$port" -Fn 2>/dev/null || true)"
{
  print "authority_game_socket_lsof_begin"
  print -- "$authority_game_socket_evidence"
  print "authority_game_socket_lsof_end"
} >> "$manifest"
authority_game_socket_endpoints=()
for socket_line in "${(@f)authority_game_socket_evidence}"; do
  if [[ "$socket_line" == n* ]]; then
    authority_game_socket_endpoints+=("${socket_line#n}")
  fi
done
if (( ${#authority_game_socket_endpoints[@]} == 0 )); then
  print -u2 "No bound authority game socket was retained on the assigned UDP port."
  exit 3
fi
print "authority_game_socket_count=${#authority_game_socket_endpoints[@]}" >> "$manifest"
for socket_endpoint in "${authority_game_socket_endpoints[@]}"; do
  if [[ "$socket_endpoint" != "127.0.0.1:${port}" ]]; then
    print -u2 "The authority game socket was not bound to 127.0.0.1:${port}."
    exit 3
  fi
  print "authority_game_socket_endpoint=$socket_endpoint" >> "$manifest"
done

"$editor" "$project" "127.0.0.1:${port}" \
  -game -unattended -nop4 -nosplash -nullrhi -nosound \
  -EchoesNetworkReconnectPhaseOne \
  "-EchoesNetworkResumeCredentialFile=$valid_credential_file" \
  "-UserDir=$phase_one_user_dir" \
  "-EchoesSaveGameDirectory=$phase_one_save_dir" \
  -AbsLog="$phase_one_log" &
register_process client_pid client_job_id "$!"
print "phase_one_pid=$client_pid" >> "$manifest"

phase_one_passed=false
seat_reserved=false
for _ in {1..240}; do
  /usr/bin/grep -q '\[ECHOES_NETWORK_RECONNECT_PHASE_ONE_PASSED\]' "$phase_one_log" &&
    phase_one_passed=true
  /usr/bin/grep -q '\[ECHOES_NETWORK_SEAT_RESERVED\]' "$server_log" &&
    seat_reserved=true
  if [[ "$phase_one_passed" == true && "$seat_reserved" == true ]]; then
    break
  fi
  if ! registered_job_active "$server_pid" "$server_job_id"; then
    break
  fi
  sleep 0.25
done
if [[ "$phase_one_passed" != true || "$seat_reserved" != true ]]; then
  print -u2 "Reconnect phase one did not reserve the authoritative seat."
  print -u2 "Server log: $server_log"
  print -u2 "Phase-one log: $phase_one_log"
  exit 4
fi
phase_one_pid="$client_pid"
phase_one_job_id="$client_job_id"
client_pid=""
client_job_id=""

assert_private_file "$valid_credential_file" 32
resume_token="$(<"$valid_credential_file")"
if [[ ! "$resume_token" =~ '^[0-9A-Fa-f]{32}$' ]]; then
  print -u2 "Phase one did not stage a bounded 128-bit Development resume credential."
  exit 5
fi
assert_secret_absent_from_registered_processes \
  "$resume_token" server_pid server_job_id phase_one_pid phase_one_job_id
require_process_success phase_one_pid phase_one_job_id phase_one_client

"$editor" "$project" "127.0.0.1:${port}" \
  -game -unattended -nop4 -nosplash -nullrhi -nosound \
  -EchoesNetworkReconnectPhaseTwo \
  "-EchoesNetworkResumeCredentialFile=$invalid_credential_file" \
  "-UserDir=$invalid_user_dir" \
  "-EchoesSaveGameDirectory=$invalid_save_dir" \
  -AbsLog="$invalid_log" &
register_process invalid_pid invalid_job_id "$!"
print "invalid_pid=$invalid_pid" >> "$manifest"
assert_secret_absent_from_registered_processes \
  "$resume_token" \
  server_pid server_job_id \
  phase_one_pid phase_one_job_id \
  invalid_pid invalid_job_id

invalid_rejected=false
reservation_preserved=false
for _ in {1..160}; do
  /usr/bin/grep -q '\[ECHOES_NETWORK_RESUME_CREDENTIAL_RESULT\] accepted=false reason=NET_RESUME_CREDENTIAL_INVALID_OR_UNAVAILABLE credentialLogged=false' "$invalid_log" &&
    invalid_rejected=true
  /usr/bin/grep -q '\[ECHOES_NETWORK_RESUME_ATTEMPT_ENDED\] player=1 seatReservationPreserved=true' "$server_log" &&
    reservation_preserved=true
  if [[ "$invalid_rejected" == true && "$reservation_preserved" == true ]]; then
    break
  fi
  if ! registered_job_active "$server_pid" "$server_job_id"; then
    break
  fi
  sleep 0.25
done
if [[ "$invalid_rejected" != true || "$reservation_preserved" != true ]]; then
  print -u2 "An invalid resume credential did not fail closed while preserving the seat reservation."
  print -u2 "Server log: $server_log"
  print -u2 "Invalid-client log: $invalid_log"
  exit 6
fi
if [[ -e "$invalid_credential_file" ]]; then
  print -u2 "The rejected one-use credential file was not consumed."
  exit 6
fi
require_process_success invalid_pid invalid_job_id invalid_client

"$editor" "$project" "127.0.0.1:${port}" \
  -game -unattended -nop4 -nosplash -nullrhi -nosound \
  -EchoesNetworkReconnectPhaseTwo \
  "-EchoesNetworkResumeCredentialFile=$valid_credential_file" \
  "-UserDir=$phase_two_user_dir" \
  "-EchoesSaveGameDirectory=$phase_two_save_dir" \
  -AbsLog="$phase_two_log" &
register_process client_pid client_job_id "$!"
print "phase_two_pid=$client_pid" >> "$manifest"
assert_secret_absent_from_registered_processes \
  "$resume_token" \
  server_pid server_job_id \
  phase_one_pid phase_one_job_id \
  client_pid client_job_id

phase_two_passed=false
server_passed=false
for _ in {1..240}; do
  /usr/bin/grep -q '\[ECHOES_NETWORK_RECONNECT_PHASE_TWO_PASSED\]' "$phase_two_log" &&
    phase_two_passed=true
  /usr/bin/grep -q '\[ECHOES_NETWORK_RECONNECT_SERVER_PASSED\]' "$server_log" &&
    server_passed=true
  if [[ "$phase_two_passed" == true && "$server_passed" == true ]]; then
    break
  fi
  if ! registered_job_active "$client_pid" "$client_job_id" &&
     [[ "$phase_two_passed" != true ]]; then
    break
  fi
  sleep 0.25
done
if [[ "$phase_two_passed" != true || "$server_passed" != true ]]; then
  print -u2 "Reconnect phase two did not complete exact state and command resynchronization."
  print -u2 "Server log: $server_log"
  print -u2 "Phase-two log: $phase_two_log"
  exit 7
fi
if [[ -e "$valid_credential_file" ]]; then
  print -u2 "The accepted one-use credential file was not consumed before submission."
  exit 7
fi

required_server_markers=(
  '\[ECHOES_NETWORK_SEAT_BOUND\] player=1 connectionBound=true sharedControl=false'
  '\[ECHOES_NETWORK_COMMAND_BATCH_ADMISSION\] player=1 batch=1 intents=1 accepted=1 rejected=0 .* lastAcceptedSequence=1'
  '\[ECHOES_NETWORK_SEAT_RESERVED\] player=1 disconnectTick=.* lastAcceptedBatch=1 matchStarted=true graceSeconds=120 authorityPaused=true aiControl=false credentialLogged=false'
  '\[ECHOES_NETWORK_RESUME_VALIDATION_PENDING\] player=1 disconnectTick=.* timeoutSeconds=5 seatActivated=false'
  '\[ECHOES_NETWORK_SEAT_RESUMED\] player=1 disconnectTick=.* lastAcceptedBatch=1 credentialMatched=true credentialRotated=true credentialLogged=false sharedControl=false'
  '\[ECHOES_NETWORK_MATCH_RESUMED\] player=1 disconnectTick=.* authorityTick=.* lastAcceptedSequence=1 nextBatch=2 fullKeyframe=true aiControl=false'
  '\[ECHOES_NETWORK_COMMAND_BATCH_ADMISSION\] player=1 batch=2 intents=1 accepted=1 rejected=0 .* lastAcceptedSequence=2'
  '\[ECHOES_NETWORK_RECONNECT_SERVER_PASSED\] player=1 .* lastAcceptedSequence=2 batch=2 seatReservationConsumed=true credentialMatched=true credentialRotated=true aiControl=false fullKeyframeResync=true commandExecuted=true separateProcess=true'
)
required_phase_one_markers=(
  '\[ECHOES_NETWORK_RESUME_CREDENTIAL\] issued=true credentialLogged=false credentialStaged=true storage=owner_only_one_use_file graceSeconds=120 exposure=developmentLoopbackSmokeOnly'
  '\[ECHOES_NETWORK_RECONNECT_ORDER\] phase=1 submitted=true .* batch=1 expectedSequence=1'
  '\[ECHOES_NETWORK_COMMAND_BATCH_RESULT\] batch=1 accepted=1 rejected=0'
  '\[ECHOES_NETWORK_RECONNECT_PHASE_ONE_PASSED\] credentialStaged=true credentialLogged=false .* lastAcceptedSequence=1 nextBatch=2 commandAdmitted=true intentionalDisconnect=true'
)
required_phase_two_markers=(
  '\[ECHOES_NETWORK_RESUME_CREDENTIAL_FILE\] consumed=true deletedBeforeSubmit=true mode=owner_only credentialLogged=false'
  '\[ECHOES_NETWORK_RESUME_CREDENTIAL_SENT\] bytes=32 transport=reliableRpc credentialLogged=false compatibilityDeferred=true'
  '\[ECHOES_NETWORK_RESUME_CREDENTIAL_RESULT\] accepted=true reason=NET_RESUME_CREDENTIAL_ACCEPTED credentialLogged=false'
  '\[ECHOES_NETWORK_COMPATIBILITY_RESULT\] accepted=true reason=NET_COMPATIBLE'
  '\[ECHOES_NETWORK_RESUME_STATE\] resumed=true seat=1 .* lastAcceptedSequence=1 nextBatch=2 exactSequence=true exactBatch=true'
  '\[ECHOES_NETWORK_KEYFRAME_RECEIVED\] player=1 snapshot=1 previous=0 .* lineage=NET_VIEW_ACCEPTED_FIRST hiddenAuthorityExcluded=true'
  '\[ECHOES_NETWORK_RECONNECT_ORDER\] phase=2 submitted=true .* batch=2 expectedSequence=2'
  '\[ECHOES_NETWORK_COMMAND_BATCH_RESULT\] batch=2 accepted=1 rejected=0'
  '\[ECHOES_NETWORK_RECONNECT_PHASE_TWO_PASSED\] seat=1 .* lastAcceptedSequence=2 batch=2 commandExecuted=true fullKeyframeResync=true credentialRotated=true separateProcess=true'
)
for marker in "${required_server_markers[@]}"; do
  if ! /usr/bin/grep -Eq "$marker" "$server_log"; then
    print -u2 "Reconnect server marker missing: $marker"
    exit 8
  fi
done
for marker in "${required_phase_one_markers[@]}"; do
  if ! /usr/bin/grep -Eq "$marker" "$phase_one_log"; then
    print -u2 "Reconnect phase-one marker missing: $marker"
    exit 8
  fi
done
for marker in "${required_phase_two_markers[@]}"; do
  if ! /usr/bin/grep -Eq "$marker" "$phase_two_log"; then
    print -u2 "Reconnect phase-two marker missing: $marker"
    exit 8
  fi
done

all_logs=("$server_log" "$phase_one_log" "$invalid_log" "$phase_two_log")
if secret_in_files "$resume_token" "${all_logs[@]}"; then
  print -u2 "The Development resume credential was exposed in a retained log."
  exit 9
fi
if secret_in_files '00000000000000000000000000000000' "${all_logs[@]}"; then
  print -u2 "The rejected Development credential was exposed in a retained log."
  exit 9
fi
if /usr/bin/grep -aFq 'EchoesNetworkResumeToken=' "${all_logs[@]}"; then
  print -u2 "The retired bearer-token command-line option appeared in retained evidence."
  exit 9
fi
assert_secret_absent_from_registered_processes \
  "$resume_token" \
  server_pid server_job_id \
  phase_one_pid phase_one_job_id \
  client_pid client_job_id

disconnect_tick="$(/usr/bin/grep '\[ECHOES_NETWORK_SEAT_RESERVED\]' "$server_log" | /usr/bin/head -1 | /usr/bin/sed -E 's/.*disconnectTick=([0-9]+).*/\1/')"
resume_tick="$(/usr/bin/grep '\[ECHOES_NETWORK_MATCH_RESUMED\]' "$server_log" | /usr/bin/tail -1 | /usr/bin/sed -E 's/.*authorityTick=([0-9]+).*/\1/')"
if [[ "$disconnect_tick" != <-> || "$resume_tick" != <-> ||
      "$resume_tick" -ne "$disconnect_tick" ]]; then
  print -u2 "The authoritative simulation did not remain frozen across the reconnect reservation window."
  exit 10
fi

if [[ -n "$client_pid" ]]; then
  require_process_success client_pid client_job_id phase_two_client
fi
if [[ -n "$phase_one_pid" ]]; then
  require_process_success phase_one_pid phase_one_job_id phase_one_client
fi
if [[ -n "$server_pid" ]]; then
  require_process_success server_pid server_job_id authority
fi

server_hash="$(/usr/bin/shasum -a 256 "$server_log" | /usr/bin/awk '{print $1}')"
phase_one_hash="$(/usr/bin/shasum -a 256 "$phase_one_log" | /usr/bin/awk '{print $1}')"
invalid_hash="$(/usr/bin/shasum -a 256 "$invalid_log" | /usr/bin/awk '{print $1}')"
phase_two_hash="$(/usr/bin/shasum -a 256 "$phase_two_log" | /usr/bin/awk '{print $1}')"
{
  print "run_completed_utc=$(/bin/date -u +%Y-%m-%dT%H:%M:%SZ)"
  print "result=passed"
  print "credential_files_consumed=true"
  print "credential_absent_from_process_arguments=true"
  print "credential_absent_from_all_logs=true"
  print "authority_game_socket_checked=true"
  print "client_udp_sockets_inspected=false"
  print "server_log_sha256=$server_hash"
  print "phase_one_log_sha256=$phase_one_hash"
  print "invalid_log_sha256=$invalid_hash"
  print "phase_two_log_sha256=$phase_two_hash"
} >> "$manifest"

trap - EXIT INT TERM
print "Network reconnect Development-loopback smoke passed."
print "Evidence directory: $run_dir"
print "Manifest: $manifest"
print "Server log SHA-256: $server_hash"
print "Phase-one log SHA-256: $phase_one_hash"
print "Invalid-client log SHA-256: $invalid_hash"
print "Phase-two log SHA-256: $phase_two_hash"
