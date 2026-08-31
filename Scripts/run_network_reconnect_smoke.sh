#!/bin/zsh
set -euo pipefail

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

mkdir -p "$evidence_root"
run_dir="$(/usr/bin/mktemp -d "$evidence_root/run.XXXXXX")"
/bin/chmod 0700 "$run_dir"
server_log="$run_dir/Server.log"
phase_one_log="$run_dir/ClientPhaseOne.log"
invalid_log="$run_dir/ClientInvalid.log"
phase_two_log="$run_dir/ClientPhaseTwo.log"
manifest="$run_dir/manifest.txt"
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
mkdir -p \
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

assert_secret_absent_from_processes() {
  local secret="$1"
  shift
  local pid command_line
  for pid in "$@"; do
    if [[ -z "$pid" ]] || ! kill -0 "$pid" 2>/dev/null; then
      continue
    fi
    command_line="$(/bin/ps -ww -o command= -p "$pid" 2>/dev/null || true)"
    if [[ "$command_line" == *"$secret"* ]]; then
      print -u2 "A reconnect credential was exposed in a live process command line."
      return 1
    fi
  done
}

server_pid=""
client_pid=""
phase_one_pid=""
invalid_pid=""

cleanup_processes() {
  local pid credential_file
  for pid in "$client_pid" "$phase_one_pid" "$invalid_pid" "$server_pid"; do
    if [[ -n "$pid" ]] && kill -0 "$pid" 2>/dev/null; then
      kill "$pid" 2>/dev/null || true
      wait "$pid" 2>/dev/null || true
    fi
  done
  for credential_file in \
      "$valid_credential_file" "$invalid_credential_file"; do
    if [[ -e "$credential_file" ]]; then
      /bin/rm -f -- "$credential_file"
    fi
  done
}
trap cleanup_processes EXIT INT TERM

"$editor" "$project" "/Engine/Maps/Entry?listen" \
  -game -unattended -nop4 -nosplash -nullrhi -nosound \
  -MULTIHOME=127.0.0.1 -port="$port" \
  -EchoesAutoStart -EchoesNetworkReconnectSmoke \
  "-UserDir=$server_user_dir" \
  "-EchoesSaveGameDirectory=$server_save_dir" \
  -AbsLog="$server_log" &
server_pid=$!
print "server_pid=$server_pid" >> "$manifest"

server_ready=false
for _ in {1..160}; do
  if /usr/bin/grep -q '\[ECHOES_BOOT_READY\]' "$server_log" &&
     /usr/bin/grep -Eq "GameNetDriver .* listening on port ${port}|IpNetDriver listening on port ${port}" "$server_log"; then
    server_ready=true
    break
  fi
  if ! kill -0 "$server_pid" 2>/dev/null; then
    break
  fi
  sleep 0.25
done
if [[ "$server_ready" != true ]]; then
  print -u2 "Reconnect listen server did not become ready. Inspect: $server_log"
  exit 3
fi

socket_evidence="$(/usr/sbin/lsof -nP -a -p "$server_pid" -iUDP:"$port" -Fn 2>/dev/null || true)"
print -- "$socket_evidence" >> "$manifest"
socket_endpoints=()
for socket_line in "${(@f)socket_evidence}"; do
  if [[ "$socket_line" == n* ]]; then
    socket_endpoints+=("${socket_line#n}")
  fi
done
if (( ${#socket_endpoints[@]} == 0 )); then
  print -u2 "No bound UDP endpoint was retained for the authority process."
  exit 3
fi
for socket_endpoint in "${socket_endpoints[@]}"; do
  if [[ "$socket_endpoint" != "127.0.0.1:${port}" ]]; then
    print -u2 "The authority exposed a UDP endpoint outside 127.0.0.1:${port}."
    exit 3
  fi
done

"$editor" "$project" "127.0.0.1:${port}" \
  -game -unattended -nop4 -nosplash -nullrhi -nosound \
  -EchoesNetworkReconnectPhaseOne \
  "-EchoesNetworkResumeCredentialFile=$valid_credential_file" \
  "-UserDir=$phase_one_user_dir" \
  "-EchoesSaveGameDirectory=$phase_one_save_dir" \
  -AbsLog="$phase_one_log" &
client_pid=$!
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
  if ! kill -0 "$server_pid" 2>/dev/null; then
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
client_pid=""

assert_private_file "$valid_credential_file" 32
resume_token="$(<"$valid_credential_file")"
if [[ ! "$resume_token" =~ '^[0-9A-Fa-f]{32}$' ]]; then
  print -u2 "Phase one did not stage a bounded 128-bit Development resume credential."
  exit 5
fi
assert_secret_absent_from_processes \
  "$resume_token" "$server_pid" "$phase_one_pid"

"$editor" "$project" "127.0.0.1:${port}" \
  -game -unattended -nop4 -nosplash -nullrhi -nosound \
  -EchoesNetworkReconnectPhaseTwo \
  "-EchoesNetworkResumeCredentialFile=$invalid_credential_file" \
  "-UserDir=$invalid_user_dir" \
  "-EchoesSaveGameDirectory=$invalid_save_dir" \
  -AbsLog="$invalid_log" &
invalid_pid=$!
print "invalid_pid=$invalid_pid" >> "$manifest"
assert_secret_absent_from_processes \
  "$resume_token" "$server_pid" "$phase_one_pid" "$invalid_pid"

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
  if ! kill -0 "$server_pid" 2>/dev/null; then
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
wait "$invalid_pid" 2>/dev/null || true
invalid_pid=""

"$editor" "$project" "127.0.0.1:${port}" \
  -game -unattended -nop4 -nosplash -nullrhi -nosound \
  -EchoesNetworkReconnectPhaseTwo \
  "-EchoesNetworkResumeCredentialFile=$valid_credential_file" \
  "-UserDir=$phase_two_user_dir" \
  "-EchoesSaveGameDirectory=$phase_two_save_dir" \
  -AbsLog="$phase_two_log" &
client_pid=$!
print "phase_two_pid=$client_pid" >> "$manifest"
assert_secret_absent_from_processes \
  "$resume_token" "$server_pid" "$phase_one_pid" "$client_pid"

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
  if ! kill -0 "$client_pid" 2>/dev/null && [[ "$phase_two_passed" != true ]]; then
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
assert_secret_absent_from_processes \
  "$resume_token" "$server_pid" "$phase_one_pid" "$client_pid"

disconnect_tick="$(/usr/bin/grep '\[ECHOES_NETWORK_SEAT_RESERVED\]' "$server_log" | /usr/bin/head -1 | /usr/bin/sed -E 's/.*disconnectTick=([0-9]+).*/\1/')"
resume_tick="$(/usr/bin/grep '\[ECHOES_NETWORK_MATCH_RESUMED\]' "$server_log" | /usr/bin/tail -1 | /usr/bin/sed -E 's/.*authorityTick=([0-9]+).*/\1/')"
if [[ "$disconnect_tick" != <-> || "$resume_tick" != <-> ||
      "$resume_tick" -ne "$disconnect_tick" ]]; then
  print -u2 "The authoritative simulation did not remain frozen across the reconnect reservation window."
  exit 10
fi

if [[ -n "$client_pid" ]]; then
  wait "$client_pid" 2>/dev/null || true
  client_pid=""
fi
if [[ -n "$phase_one_pid" ]]; then
  wait "$phase_one_pid" 2>/dev/null || true
  phase_one_pid=""
fi
if [[ -n "$server_pid" ]]; then
  wait "$server_pid" 2>/dev/null || true
  server_pid=""
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
