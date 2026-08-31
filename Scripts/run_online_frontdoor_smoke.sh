#!/bin/zsh
set -euo pipefail

project_root="${0:A:h:h}"
ue_root="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
editor="$ue_root/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
project="$project_root/EchoesOfTheBrokenSun.uproject"
port="${ECHOES_ONLINE_FRONTDOOR_PORT:-7811}"
server_log="${ECHOES_ONLINE_FRONTDOOR_SERVER_LOG:-$project_root/BuildArtifacts/OnlineFrontDoorServer.log}"
client_log="${ECHOES_ONLINE_FRONTDOOR_CLIENT_LOG:-$project_root/BuildArtifacts/OnlineFrontDoorClient.log}"

if [[ ! -x "$editor" ]]; then
  print -u2 "Unreal Editor is not available at: $editor"
  exit 2
fi
if [[ "$port" != <-> || "$port" -lt 1024 || "$port" -gt 65535 ]]; then
  print -u2 "ECHOES_ONLINE_FRONTDOOR_PORT must be an integer from 1024 through 65535."
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

"$editor" "$project" /Engine/Maps/Entry \
  -game -unattended -nop4 -nosplash -nullrhi -nosound \
  -MULTIHOME=127.0.0.1 -port="$port" -EchoesAutoStart \
  -EchoesOnlineFrontDoorHostSmoke -EchoesNetworkMatchSmoke \
  -AbsLog="$server_log" &
server_pid=$!

server_ready=false
for _ in {1..240}; do
  if /usr/bin/grep -q '\[ECHOES_ONLINE_FRONT_DOOR_HOST_SMOKE\] apiPath=true requested=true' "$server_log" &&
     /usr/bin/grep -q '\[ECHOES_ONLINE_FIXED_RULES_READY\] map=GLASS_SCAR player0=MERIDIAN_COMPACT player1=KHARUUN_ASSEMBLIES resources=STANDARD protocol=fixed_1v1' "$server_log" &&
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
  print -u2 "The player-facing Host flow did not open a fixed-rules listen match."
  print -u2 "Inspect: $server_log"
  exit 3
fi

"$editor" "$project" /Engine/Maps/Entry \
  -game -unattended -nop4 -nosplash -nullrhi -nosound \
  -EchoesAutoStart \
  -EchoesOnlineFrontDoorClientSmoke="127.0.0.1:${port}" \
  -EchoesNetworkMatchClientSmoke -AbsLog="$client_log" &
client_pid=$!

client_passed=false
server_passed=false
for _ in {1..520}; do
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
  print -u2 "The player-facing Host/Direct Join journey did not complete."
  print -u2 "Server log: $server_log"
  print -u2 "Client log: $client_log"
  exit 4
fi

required_server_markers=(
  "\\[ECHOES_ONLINE_FRONT_DOOR_HOST\\] requested=true map=/Engine/Maps/Entry listen=true rules=fixed_glass_scar port=${port}"
  '\[ECHOES_ONLINE_FRONT_DOOR_HOST_SMOKE\] apiPath=true requested=true'
  '\[ECHOES_ONLINE_FIXED_RULES_READY\] map=GLASS_SCAR player0=MERIDIAN_COMPACT player1=KHARUUN_ASSEMBLIES resources=STANDARD protocol=fixed_1v1'
  '\[ECHOES_NETWORK_MATCH_STARTED\] player=1 .* readyGate=true'
  '\[ECHOES_NETWORK_MATCH_RESULT_SENT\] player=1 outcome=2 .* reliableFinalKeyframe=true'
  '\[ECHOES_NETWORK_RESULT_ACKNOWLEDGED\] outcome=2 .* exact=true'
  '\[ECHOES_NETWORK_MATCH_SERVER_SMOKE_PASSED\] player=1 outcome=2 .* exactAcknowledgement=true separateProcess=true'
)
required_client_markers=(
  "\\[ECHOES_ONLINE_FRONT_DOOR_CLIENT_SMOKE\\] apiPath=true requested=true endpoint=127.0.0.1:${port}"
  "\\[ECHOES_ONLINE_FRONT_DOOR_JOIN\\] requested=true endpoint=127.0.0.1:${port} options=false fixedRules=true"
  '\[ECHOES_NETWORK_COMPATIBILITY_RESULT\] accepted=true reason=NET_COMPATIBLE'
  '\[ECHOES_NETWORK_MATCH_RESULT_RECEIVED\] seat=1 outcome=2 .* stateMatched=true'
  '\[ECHOES_RESULT_PRESENTED\] outcome=2 primaryAction=online_menu keyboardRestart=false'
  '\[ECHOES_NETWORK_MATCH_CLIENT_SMOKE_PASSED\] seat=1 outcome=2 .* finalStateMatched=true separateProcess=true'
)
for marker in "${required_server_markers[@]}"; do
  if ! /usr/bin/grep -Eq "$marker" "$server_log"; then
    print -u2 "Host marker missing: $marker"
    exit 5
  fi
done
for marker in "${required_client_markers[@]}"; do
  if ! /usr/bin/grep -Eq "$marker" "$client_log"; then
    print -u2 "Direct Join marker missing: $marker"
    exit 5
  fi
done

failure_pattern='\[ECHOES_(ONLINE|NETWORK)_.*(FAILED|REJECTED)\]|Fatal error:|Assertion failed:|Ensure condition failed:|SIGSEGV:|=== Critical error:'
if /usr/bin/grep -Eq "$failure_pattern" "$server_log" "$client_log"; then
  print -u2 "The player-facing online journey reported a controlled failure or fatal marker."
  /usr/bin/grep -E "$failure_pattern" "$server_log" "$client_log" >&2 || true
  exit 6
fi

wait "$client_pid"
client_pid=""
wait "$server_pid"
server_pid=""
trap - EXIT INT TERM

print "Player-facing Online 1v1 passed: standalone Operations entry, Host travel, fixed Glass Scar rules, Direct Join validation, ready gate, complete replicated match, exact result acknowledgement, and Online-menu result routing."
print "Server log: $server_log"
print "Client log: $client_log"
