#!/bin/zsh
set -euo pipefail

project_root="${0:A:h:h}"
ue_root="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
editor="$ue_root/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
project="$project_root/EchoesOfTheBrokenSun.uproject"
port="${ECHOES_NETWORK_PORT:-7797}"
server_log="${ECHOES_NETWORK_SERVER_LOG:-$project_root/BuildArtifacts/NetworkListenServer.log}"
client_log="${ECHOES_NETWORK_CLIENT_LOG:-$project_root/BuildArtifacts/NetworkListenClient.log}"

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
  -port="$port" -EchoesAutoStart -EchoesNetworkListenSmoke \
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
  print -u2 "Listen server did not become ready. Inspect: $server_log"
  exit 3
fi

"$editor" "$project" "127.0.0.1:${port}" \
  -game -unattended -nop4 -nosplash -nullrhi -nosound \
  -EchoesNetworkClientSmoke -AbsLog="$client_log" &
client_pid=$!

client_passed=false
server_passed=false
for _ in {1..240}; do
  /usr/bin/grep -q '\[ECHOES_NETWORK_CLIENT_SMOKE_PASSED\]' "$client_log" &&
    client_passed=true
  /usr/bin/grep -q '\[ECHOES_NETWORK_SERVER_SMOKE_PASSED\]' "$server_log" &&
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
  print -u2 "Separate-process network smoke did not complete."
  print -u2 "Server log: $server_log"
  print -u2 "Client log: $client_log"
  exit 4
fi

required_server_markers=(
  '\[ECHOES_NETWORK_SEAT_BOUND\] player=1 connectionBound=true sharedControl=false'
  '\[ECHOES_NETWORK_AUTHORITY_WAITING\] tick=0 paused=true player=1 readyGate=true'
  '\[ECHOES_NETWORK_LOBBY\] player=1 compatible=true ready=true started=false'
  '\[ECHOES_NETWORK_MATCH_STARTED\] player=1 authorityTick=0 inputDelayTicks=3 readyGate=true'
  '\[ECHOES_NETWORK_HOST_COMMAND_QUEUED\] player=0 .* assignedExecuteTick=3 authorityTick=0 delayTicks=3'
  '\[ECHOES_NETWORK_KEYFRAME_SENT\] player=1 .* hiddenAuthorityExcluded=true'
  '\[ECHOES_NETWORK_KEYFRAME_ACKNOWLEDGED\] player=1 snapshot=2 .* lineageExact=true'
  '\[ECHOES_NETWORK_COMMAND_ADMISSION\] player=1 status=NET_CMD_ACCEPTED .* requestedExecuteTick=0 assignedExecuteTick=3 .* serverTick=0 authorityAssigned=true'
  '\[ECHOES_NETWORK_COMMAND_EXECUTION\] executed=true'
  '\[ECHOES_NETWORK_HOST_COMMAND_EXECUTION\] executed=true .* delayTicks=3'
  '\[ECHOES_NETWORK_SERVER_SMOKE_PASSED\] .* separateProcess=true readyGate=true periodicState=true hostRemoteDelayParity=true authorityAssignedCommands=true connectionBound=true hiddenAuthorityExcluded=true'
)
required_client_markers=(
  '\[ECHOES_NETWORK_HELLO_SENT\]'
  '\[ECHOES_NETWORK_COMPATIBILITY_RESULT\] accepted=true reason=NET_COMPATIBLE'
  '\[ECHOES_NETWORK_LOBBY_RESULT\] compatible=true started=false authorityTick=0 inputDelayTicks=3'
  '\[ECHOES_NETWORK_LOBBY_RESULT\] compatible=true started=true authorityTick=0 inputDelayTicks=3'
  '\[ECHOES_NETWORK_KEYFRAME_RECEIVED\] player=1 .* hiddenAuthorityExcluded=true'
  '\[ECHOES_NETWORK_COMMAND_SENT\] .* requestedExecuteTick=0 .* authorityAssignsTick=true'
  '\[ECHOES_NETWORK_COMMAND_RESULT\] status=NET_CMD_ACCEPTED'
  '\[ECHOES_NETWORK_EXECUTION_RESULT\] executed=true'
  '\[ECHOES_NETWORK_CLIENT_SMOKE_PASSED\] .* separateProcess=true readyGate=true periodicState=true authorityAssignedCommands=true connectionBound=true'
)
for marker in "${required_server_markers[@]}"; do
  if ! /usr/bin/grep -Eq "$marker" "$server_log"; then
    print -u2 "Server marker missing: $marker"
    exit 5
  fi
done

if [[ "$(/usr/bin/grep -c '\[ECHOES_NETWORK_KEYFRAME_SENT\]' "$server_log")" -lt 2 ||
      "$(/usr/bin/grep -c '\[ECHOES_NETWORK_KEYFRAME_ACKNOWLEDGED\]' "$server_log")" -lt 2 ||
      "$(/usr/bin/grep -c '\[ECHOES_NETWORK_KEYFRAME_RECEIVED\]' "$client_log")" -lt 2 ]]; then
  print -u2 "Network smoke did not prove repeated scoped-state delivery and acknowledgement."
  exit 6
fi
for marker in "${required_client_markers[@]}"; do
  if ! /usr/bin/grep -Eq "$marker" "$client_log"; then
    print -u2 "Client marker missing: $marker"
    exit 6
  fi
done

if /usr/bin/grep -Eq '\[ECHOES_NETWORK_.*(FAILED|REJECTED)\]|Fatal error:|Assertion failed:|Ensure condition failed:|SIGSEGV:|=== Critical error:' "$server_log" "$client_log"; then
  print -u2 "Network smoke reported a controlled failure or fatal marker."
  exit 7
fi

if ! wait "$client_pid"; then
  print -u2 "Network client exited unsuccessfully. Inspect: $client_log"
  exit 8
fi
client_pid=""
if ! wait "$server_pid"; then
  print -u2 "Listen server exited unsuccessfully. Inspect: $server_log"
  exit 9
fi
server_pid=""
trap - EXIT INT TERM

print "Separate-process Unreal listen-server smoke passed: connection-bound seat 1, exact compatibility admission, explicit ready/start, matched three-tick host/remote authority scheduling, repeated visibility-scoped keyframes with exact acknowledgements, and authoritative execution."
print "Server evidence log: $server_log"
print "Client evidence log: $client_log"
