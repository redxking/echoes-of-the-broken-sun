#!/bin/zsh
set -euo pipefail

project_root="${0:A:h:h}"
ue_root="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
editor="$ue_root/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
project="$project_root/EchoesOfTheBrokenSun.uproject"
port="${ECHOES_NETWORK_PORT:-7799}"
server_log="${ECHOES_NETWORK_SERVER_LOG:-$project_root/BuildArtifacts/NetworkReconnectServer.log}"
phase_one_log="${ECHOES_NETWORK_PHASE_ONE_LOG:-$project_root/BuildArtifacts/NetworkReconnectClientPhaseOne.log}"
invalid_log="${ECHOES_NETWORK_INVALID_LOG:-$project_root/BuildArtifacts/NetworkReconnectClientInvalid.log}"
phase_two_log="${ECHOES_NETWORK_PHASE_TWO_LOG:-$project_root/BuildArtifacts/NetworkReconnectClientPhaseTwo.log}"

if [[ ! -x "$editor" ]]; then
  print -u2 "Unreal Editor is not available at: $editor"
  exit 2
fi
if [[ "$port" != <-> || "$port" -lt 1024 || "$port" -gt 65535 ]]; then
  print -u2 "ECHOES_NETWORK_PORT must be an integer from 1024 through 65535."
  exit 2
fi

mkdir -p "$project_root/BuildArtifacts"
: > "$server_log"
: > "$phase_one_log"
: > "$invalid_log"
: > "$phase_two_log"
server_pid=""
client_pid=""
phase_one_pid=""
invalid_pid=""

cleanup_processes() {
  if [[ -n "$client_pid" ]] && kill -0 "$client_pid" 2>/dev/null; then
    kill "$client_pid" 2>/dev/null || true
    wait "$client_pid" 2>/dev/null || true
  fi
  if [[ -n "$phase_one_pid" ]] && kill -0 "$phase_one_pid" 2>/dev/null; then
    kill "$phase_one_pid" 2>/dev/null || true
    wait "$phase_one_pid" 2>/dev/null || true
  fi
  if [[ -n "$invalid_pid" ]] && kill -0 "$invalid_pid" 2>/dev/null; then
    kill "$invalid_pid" 2>/dev/null || true
    wait "$invalid_pid" 2>/dev/null || true
  fi
  if [[ -n "$server_pid" ]] && kill -0 "$server_pid" 2>/dev/null; then
    kill "$server_pid" 2>/dev/null || true
    wait "$server_pid" 2>/dev/null || true
  fi
}
trap cleanup_processes EXIT INT TERM

"$editor" "$project" "/Engine/Maps/Entry?listen" \
  -game -unattended -nop4 -nosplash -nullrhi -nosound \
  -port="$port" -EchoesAutoStart -EchoesNetworkReconnectSmoke \
  -AbsLog="$server_log" &
server_pid=$!

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

"$editor" "$project" "127.0.0.1:${port}" \
  -game -unattended -nop4 -nosplash -nullrhi -nosound \
  -EchoesNetworkReconnectPhaseOne -AbsLog="$phase_one_log" &
client_pid=$!

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

resume_token="$(/usr/bin/grep '\[ECHOES_NETWORK_RECONNECT_PHASE_ONE_PASSED\]' "$phase_one_log" | /usr/bin/tail -1 | /usr/bin/sed -E 's/.* token=([0-9A-Fa-f]+) .*/\1/')"
if [[ ! "$resume_token" =~ '^[0-9A-Fa-f]{32}$' ]]; then
  print -u2 "Phase one did not emit a bounded 128-bit development resume token."
  exit 5
fi

"$editor" "$project" "127.0.0.1:${port}" \
  -game -unattended -nop4 -nosplash -nullrhi -nosound \
  -EchoesNetworkReconnectPhaseTwo \
  -EchoesNetworkResumeToken="00000000000000000000000000000000" \
  -AbsLog="$invalid_log" &
invalid_pid=$!

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

"$editor" "$project" "127.0.0.1:${port}" \
  -game -unattended -nop4 -nosplash -nullrhi -nosound \
  -EchoesNetworkReconnectPhaseTwo \
  -EchoesNetworkResumeToken="$resume_token" -AbsLog="$phase_two_log" &
client_pid=$!

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

required_server_markers=(
  '\[ECHOES_NETWORK_SEAT_BOUND\] player=1 connectionBound=true sharedControl=false'
  '\[ECHOES_NETWORK_COMMAND_BATCH_ADMISSION\] player=1 batch=1 intents=1 accepted=1 rejected=0 .* lastAcceptedSequence=1'
  '\[ECHOES_NETWORK_SEAT_RESERVED\] player=1 disconnectTick=.* lastAcceptedBatch=1 matchStarted=true graceSeconds=120 aiControl=false credentialLogged=false'
  '\[ECHOES_NETWORK_RESUME_VALIDATION_PENDING\] player=1 disconnectTick=.* timeoutSeconds=5 seatActivated=false'
  '\[ECHOES_NETWORK_SEAT_RESUMED\] player=1 disconnectTick=.* lastAcceptedBatch=1 credentialMatched=true credentialRotated=true credentialLogged=false sharedControl=false'
  '\[ECHOES_NETWORK_MATCH_RESUMED\] player=1 disconnectTick=.* authorityTick=.* lastAcceptedSequence=1 nextBatch=2 fullKeyframe=true aiControl=false'
  '\[ECHOES_NETWORK_COMMAND_BATCH_ADMISSION\] player=1 batch=2 intents=1 accepted=1 rejected=0 .* lastAcceptedSequence=2'
  '\[ECHOES_NETWORK_RECONNECT_SERVER_PASSED\] player=1 .* lastAcceptedSequence=2 batch=2 seatReservationConsumed=true credentialMatched=true credentialRotated=true aiControl=false fullKeyframeResync=true commandExecuted=true separateProcess=true'
)
required_phase_one_markers=(
  '\[ECHOES_NETWORK_RESUME_CREDENTIAL\] issued=true token=[0-9A-Fa-f]{32} graceSeconds=120 exposure=developmentSmokeOnly'
  '\[ECHOES_NETWORK_RECONNECT_ORDER\] phase=1 submitted=true .* batch=1 expectedSequence=1'
  '\[ECHOES_NETWORK_COMMAND_BATCH_RESULT\] batch=1 accepted=1 rejected=0'
  '\[ECHOES_NETWORK_RECONNECT_PHASE_ONE_PASSED\] .* lastAcceptedSequence=1 nextBatch=2 commandAdmitted=true intentionalDisconnect=true'
)
required_phase_two_markers=(
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

if /usr/bin/grep -q "$resume_token" "$server_log"; then
  print -u2 "The development resume token was exposed in the server log."
  exit 9
fi
if /usr/bin/grep -q '00000000000000000000000000000000' "$server_log"; then
  print -u2 "The rejected development token was exposed in the server log."
  exit 9
fi

disconnect_tick="$(/usr/bin/grep '\[ECHOES_NETWORK_SEAT_RESERVED\]' "$server_log" | /usr/bin/head -1 | /usr/bin/sed -E 's/.*disconnectTick=([0-9]+).*/\1/')"
resume_tick="$(/usr/bin/grep '\[ECHOES_NETWORK_MATCH_RESUMED\]' "$server_log" | /usr/bin/tail -1 | /usr/bin/sed -E 's/.*authorityTick=([0-9]+).*/\1/')"
if [[ "$disconnect_tick" != <-> || "$resume_tick" != <-> ||
      "$resume_tick" -le "$disconnect_tick" ]]; then
  print -u2 "The authoritative simulation did not advance across the disconnect window."
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
if [[ -n "$invalid_pid" ]]; then
  wait "$invalid_pid" 2>/dev/null || true
  invalid_pid=""
fi
if [[ -n "$server_pid" ]]; then
  wait "$server_pid" 2>/dev/null || true
  server_pid=""
fi

server_hash="$(/usr/bin/shasum -a 256 "$server_log" | /usr/bin/awk '{print $1}')"
phase_one_hash="$(/usr/bin/shasum -a 256 "$phase_one_log" | /usr/bin/awk '{print $1}')"
invalid_hash="$(/usr/bin/shasum -a 256 "$invalid_log" | /usr/bin/awk '{print $1}')"
phase_two_hash="$(/usr/bin/shasum -a 256 "$phase_two_log" | /usr/bin/awk '{print $1}')"
print "Network reconnect smoke passed."
print "Server log: $server_log"
print "Phase-one log: $phase_one_log"
print "Invalid-client log: $invalid_log"
print "Phase-two log: $phase_two_log"
print "Server SHA-256: $server_hash"
print "Phase-one SHA-256: $phase_one_hash"
print "Invalid-client SHA-256: $invalid_hash"
print "Phase-two SHA-256: $phase_two_hash"
