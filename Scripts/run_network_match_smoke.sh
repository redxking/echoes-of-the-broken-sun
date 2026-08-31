#!/bin/zsh
set -euo pipefail

project_root="${0:A:h:h}"
ue_root="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
editor="$ue_root/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
project="$project_root/EchoesOfTheBrokenSun.uproject"
port="${ECHOES_NETWORK_MATCH_PORT:-7798}"
server_log="${ECHOES_NETWORK_MATCH_SERVER_LOG:-$project_root/BuildArtifacts/NetworkMatchServer.log}"
client_log="${ECHOES_NETWORK_MATCH_CLIENT_LOG:-$project_root/BuildArtifacts/NetworkMatchClient.log}"

if [[ ! -x "$editor" ]]; then
  print -u2 "Unreal Editor is not available at: $editor"
  exit 2
fi
if [[ "$port" != <-> || "$port" -lt 1024 || "$port" -gt 65535 ]]; then
  print -u2 "ECHOES_NETWORK_MATCH_PORT must be an integer from 1024 through 65535."
  exit 2
fi

mkdir -p "$project_root/BuildArtifacts"
: > "$server_log"
: > "$client_log"
server_pid=""
client_pid=""

cleanup_processes() {
  if [[ -n "$client_pid" ]] && kill -0 "$client_pid" 2>/dev/null; then
    kill "$client_pid" 2>/dev/null || true
    wait "$client_pid" 2>/dev/null || true
  fi
  if [[ -n "$server_pid" ]] && kill -0 "$server_pid" 2>/dev/null; then
    kill "$server_pid" 2>/dev/null || true
    wait "$server_pid" 2>/dev/null || true
  fi
}
trap cleanup_processes EXIT INT TERM

"$editor" "$project" "/Engine/Maps/Entry?listen" \
  -game -unattended -nop4 -nosplash -nullrhi -nosound \
  -MULTIHOME=127.0.0.1 -port="$port" \
  -EchoesAutoStart -EchoesNetworkMatchSmoke \
  -AbsLog="$server_log" &
server_pid=$!

server_ready=false
for _ in {1..160}; do
  if /usr/bin/grep -q '\[ECHOES_NETWORK_MATCH_FIXTURE\]' "$server_log" &&
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
  print -u2 "Network match authority did not become ready. Inspect: $server_log"
  exit 3
fi

"$editor" "$project" "127.0.0.1:${port}" \
  -game -unattended -nop4 -nosplash -nullrhi -nosound \
  -EchoesNetworkMatchClientSmoke -AbsLog="$client_log" &
client_pid=$!

client_passed=false
server_passed=false
for _ in {1..400}; do
  /usr/bin/grep -q '\[ECHOES_NETWORK_MATCH_CLIENT_SMOKE_PASSED\]' "$client_log" &&
    client_passed=true
  /usr/bin/grep -q '\[ECHOES_NETWORK_MATCH_SERVER_SMOKE_PASSED\]' "$server_log" &&
    server_passed=true
  if [[ "$client_passed" == true && "$server_passed" == true ]]; then
    break
  fi
  if ! kill -0 "$client_pid" 2>/dev/null && [[ "$client_passed" != true ]]; then
    break
  fi
  sleep 0.25
done

if [[ "$client_passed" != true || "$server_passed" != true ]]; then
  print -u2 "Separate-process network match did not complete."
  print -u2 "Server log: $server_log"
  print -u2 "Client log: $client_log"
  exit 4
fi

required_server_markers=(
  '\[ECHOES_NETWORK_MATCH_FIXTURE\] .* remoteAttackers=24 .* controlledNonshipping=true ordinaryCombatResolution=true'
  '\[ECHOES_NETWORK_MATCH_STARTED\] player=1 .* readyGate=true'
  '\[ECHOES_NETWORK_COMMAND_BATCH_ADMISSION\] player=1 batch=1 intents=24 accepted=24 rejected=0 .* authorityAssigned=true firstRejection=none'
  '\[ECHOES_NETWORK_KEYFRAME_SENT\] player=1 .* fallback=finalReliableKeyframe hiddenAuthorityExcluded=true'
  '\[ECHOES_NETWORK_MATCH_RESULT_SENT\] player=1 outcome=2 .* reliableFinalKeyframe=true'
  '\[ECHOES_NETWORK_MATCH_SERVER_SMOKE_PASSED\] player=1 outcome=2 .* batchAuthority=true ordinaryCombatResolution=true reliableFinalKeyframe=true exactAcknowledgement=true separateProcess=true'
)
required_client_markers=(
  '\[ECHOES_NETWORK_COMPATIBILITY_RESULT\] accepted=true reason=NET_COMPATIBLE'
  '\[ECHOES_NETWORK_MATCH_ORDER_SUBMITTED\] submitted=true selectedActors=24 .* selectionAdapter=true orderAdapter=true batched=true'
  '\[ECHOES_NETWORK_COMMAND_BATCH_RESULT\] batch=1 accepted=24 rejected=0 .* firstRejection=none'
  '\[ECHOES_NETWORK_MATCH_RESULT_RECEIVED\] seat=1 outcome=2 .* stateMatched=true'
  '\[ECHOES_RESULT_PRESENTED\] outcome=2 primaryAction=online_menu keyboardRestart=false'
  '\[ECHOES_NETWORK_MATCH_CLIENT_SMOKE_PASSED\] seat=1 outcome=2 .* batchAdmitted=true ordinaryCombatResolution=true presentedVictory=true finalStateMatched=true separateProcess=true'
)
for marker in "${required_server_markers[@]}"; do
  if ! /usr/bin/grep -Eq "$marker" "$server_log"; then
    print -u2 "Server marker missing: $marker"
    exit 5
  fi
done
for marker in "${required_client_markers[@]}"; do
  if ! /usr/bin/grep -Eq "$marker" "$client_log"; then
    print -u2 "Client marker missing: $marker"
    exit 5
  fi
done

failure_pattern='\[ECHOES_NETWORK_.*(FAILED|REJECTED)\]|Fatal error:|Assertion failed:|Ensure condition failed:|SIGSEGV:|=== Critical error:'
if /usr/bin/grep -Eq "$failure_pattern" "$server_log" "$client_log"; then
  print -u2 "Network match smoke reported a controlled failure or fatal marker."
  /usr/bin/grep -E "$failure_pattern" "$server_log" "$client_log" >&2 || true
  exit 6
fi

if ! wait "$client_pid"; then
  print -u2 "Network match client exited unsuccessfully. Inspect: $client_log"
  exit 7
fi
client_pid=""
if ! wait "$server_pid"; then
  print -u2 "Network match authority exited unsuccessfully. Inspect: $server_log"
  exit 8
fi
server_pid=""
trap - EXIT INT TERM

print "Separate-process Unreal network match passed: 24-actor remote selection/order batching, authority-owned sequence and tick assignment, ordinary combat resolution, reliable exact final state, seat-aware victory presentation, and final acknowledgement."
print "Server evidence log: $server_log"
print "Client evidence log: $client_log"
