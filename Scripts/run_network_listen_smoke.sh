#!/bin/zsh
set -euo pipefail

project_root="${0:A:h:h}"
ue_root="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
editor="$ue_root/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
project="$project_root/EchoesOfTheBrokenSun.uproject"
port="${ECHOES_NETWORK_PORT:-7797}"
server_log="${ECHOES_NETWORK_SERVER_LOG:-$project_root/BuildArtifacts/NetworkListenServer.log}"
client_log="${ECHOES_NETWORK_CLIENT_LOG:-$project_root/BuildArtifacts/NetworkListenClient.log}"
fault_mode="${ECHOES_NETWORK_FAULT_MODE:-none}"
transport_profile="${ECHOES_NETWORK_TRANSPORT_PROFILE:-none}"

if [[ ! -x "$editor" ]]; then
  print -u2 "Unreal Editor is not available at: $editor"
  exit 2
fi
if [[ "$port" != <-> || "$port" -lt 1024 || "$port" -gt 65535 ]]; then
  print -u2 "ECHOES_NETWORK_PORT must be an integer from 1024 through 65535."
  exit 2
fi
case "$fault_mode" in
  none|drop-first-delta|delay-first-delta|duplicate-first-delta|reorder-first-two-deltas|drop-delta-burst) ;;
  *)
    print -u2 "ECHOES_NETWORK_FAULT_MODE must be none, drop-first-delta, delay-first-delta, duplicate-first-delta, reorder-first-two-deltas, or drop-delta-burst."
    exit 2
    ;;
esac
case "$transport_profile" in
  none|latency-jitter|loss) ;;
  *)
    print -u2 "ECHOES_NETWORK_TRANSPORT_PROFILE must be none, latency-jitter, or loss."
    exit 2
    ;;
esac

transport_args=()
transport_markers=()
case "$transport_profile" in
  latency-jitter)
    transport_args+=("-GameNetDriverPktLag=75" "-GameNetDriverPktLagVariance=25")
    transport_markers+=(
      'PktLag set to 75'
      'PktLagVariance set to 25'
    )
    ;;
  loss)
    transport_args+=("-GameNetDriverPktLoss=10")
    transport_markers+=(
      'PktLoss set to 10'
    )
    ;;
esac

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
  -EchoesAutoStart -EchoesNetworkListenSmoke \
  "${transport_args[@]}" \
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

client_fault_args=()
case "$fault_mode" in
  drop-first-delta) client_fault_args+=("-EchoesNetworkDropFirstDelta") ;;
  delay-first-delta) client_fault_args+=("-EchoesNetworkDelayFirstDelta") ;;
  duplicate-first-delta) client_fault_args+=("-EchoesNetworkDuplicateFirstDelta") ;;
  reorder-first-two-deltas) client_fault_args+=("-EchoesNetworkReorderFirstTwoDeltas") ;;
  drop-delta-burst) client_fault_args+=("-EchoesNetworkDropDeltaBurst") ;;
esac

"$editor" "$project" "127.0.0.1:${port}" \
  -game -unattended -nop4 -nosplash -nullrhi -nosound \
  -EchoesNetworkClientSmoke "${client_fault_args[@]}" \
  "${transport_args[@]}" \
  -AbsLog="$client_log" &
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
  '\[ECHOES_NETWORK_DELTA_SENT\] player=1 .* base=.* hiddenAuthorityExcluded=true'
  '\[ECHOES_NETWORK_KEYFRAME_ACKNOWLEDGED\] player=1 .* lineageExact=true'
  '\[ECHOES_NETWORK_COMMAND_ADMISSION\] player=1 status=NET_CMD_ACCEPTED .* requestedExecuteTick=0 assignedExecuteTick=.* serverTick=.* authorityAssigned=true'
  '\[ECHOES_NETWORK_COMMAND_EXECUTION\] executed=true'
  '\[ECHOES_NETWORK_HOST_COMMAND_EXECUTION\] executed=true .* delayTicks=3'
  '\[ECHOES_NETWORK_SERVER_SMOKE_PASSED\] .* separateProcess=true readyGate=true periodicState=true hostRemoteDelayParity=true authorityAssignedCommands=true connectionBound=true hiddenAuthorityExcluded=true'
)
required_client_markers=(
  '\[ECHOES_NETWORK_HELLO_SENT\]'
  '\[ECHOES_NETWORK_COMPATIBILITY_RESULT\] accepted=true reason=NET_COMPATIBLE'
  '\[ECHOES_NETWORK_LOBBY_RESULT\] compatible=true started=false seat=1 authorityTick=0 inputDelayTicks=3'
  '\[ECHOES_NETWORK_LOBBY_RESULT\] compatible=true started=true seat=1 authorityTick=0 inputDelayTicks=3'
  '\[ECHOES_NETWORK_KEYFRAME_RECEIVED\] player=1 .* hiddenAuthorityExcluded=true'
  '\[ECHOES_NETWORK_PRESENTATION_SYNCED\] .* ground=true terrain=true fog=true lighting=true scopedOnly=true rendered=true'
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

server_ack_count="$(/usr/bin/grep -c '\[ECHOES_NETWORK_KEYFRAME_ACKNOWLEDGED\]' "$server_log" || true)"
client_keyframe_count="$(/usr/bin/grep -c '\[ECHOES_NETWORK_KEYFRAME_RECEIVED\]' "$client_log" || true)"
client_delta_count="$(/usr/bin/grep -c '\[ECHOES_NETWORK_DELTA_RECEIVED\]' "$client_log" || true)"
if [[ "$server_ack_count" -lt 2 ||
      "$((client_keyframe_count + client_delta_count))" -lt 2 ]]; then
  print -u2 "Network smoke did not prove repeated keyframe/delta delivery and acknowledgement."
  exit 6
fi
if [[ "$fault_mode" == "none" && "$client_delta_count" -lt 1 ]]; then
  print -u2 "Normal network smoke did not prove accepted base-linked delta delivery."
  exit 6
fi

admission_line="$(/usr/bin/grep '\[ECHOES_NETWORK_COMMAND_ADMISSION\] player=1 status=NET_CMD_ACCEPTED' "$server_log" | /usr/bin/tail -1)"
assigned_tick="$(print -r -- "$admission_line" | /usr/bin/sed -E 's/.*assignedExecuteTick=([0-9]+).*/\1/')"
server_tick="$(print -r -- "$admission_line" | /usr/bin/sed -E 's/.*serverTick=([0-9]+).*/\1/')"
if [[ "$assigned_tick" != <-> || "$server_tick" != <-> ||
      "$assigned_tick" -ne "$((server_tick + 3))" ]]; then
  print -u2 "Remote command was not assigned exactly three ticks from authoritative receipt."
  exit 6
fi

fault_markers=()
case "$fault_mode" in
  drop-first-delta)
    fault_markers+=(
      '\[ECHOES_NETWORK_DELTA_DROPPED\] injected=true'
      '\[ECHOES_NETWORK_DELTA_REJECTED\] .* reason=NET_DELTA_BASE_MISSING'
      '\[ECHOES_NETWORK_KEYFRAME_RECOVERY_REQUESTED\] .* reason=NET_DELTA_BASE_MISSING rateLimited=true'
      '\[ECHOES_NETWORK_KEYFRAME_REQUESTED\] player=1 .* recovery=fullKeyframe'
      '\[ECHOES_NETWORK_KEYFRAME_ACKNOWLEDGED\] player=1 snapshot=.* retired=3 pendingSnapshots=0 lineageExact=true'
    )
    ;;
  delay-first-delta)
    fault_markers+=(
      '\[ECHOES_NETWORK_DELTA_DELAYED\] injected=true delayMilliseconds=250'
      '\[ECHOES_NETWORK_DELTA_DELAY_COMPLETE\] injected=true'
      '\[ECHOES_NETWORK_DELTA_RECEIVED\] player=1 .* lineage=NET_VIEW_ACCEPTED_DELTA'
    )
    ;;
  duplicate-first-delta)
    fault_markers+=(
      '\[ECHOES_NETWORK_DELTA_DUPLICATED\] injected=true'
      '\[ECHOES_NETWORK_DELTA_IGNORED\] .* reason=NET_VIEW_STALE_OR_DUPLICATE recoveryRequested=false'
    )
    ;;
  reorder-first-two-deltas)
    fault_markers+=(
      '\[ECHOES_NETWORK_DELTA_REORDER_HELD\] injected=true'
      '\[ECHOES_NETWORK_DELTA_REORDERED\] injected=true .* deliveryOrder=newerThenOlder'
      '\[ECHOES_NETWORK_DELTA_REJECTED\] .* reason=NET_DELTA_BASE_MISSING'
      '\[ECHOES_NETWORK_KEYFRAME_RECOVERY\] .* fullKeyframe=true'
    )
    ;;
  drop-delta-burst)
    fault_markers+=(
      '\[ECHOES_NETWORK_DELTA_BURST_DROPPED\] injected=true ordinal=1 burstSize=3'
      '\[ECHOES_NETWORK_DELTA_BURST_DROPPED\] injected=true ordinal=2 burstSize=3'
      '\[ECHOES_NETWORK_DELTA_BURST_DROPPED\] injected=true ordinal=3 burstSize=3'
      '\[ECHOES_NETWORK_DELTA_REJECTED\] .* reason=NET_DELTA_BASE_MISSING'
      '\[ECHOES_NETWORK_KEYFRAME_RECOVERY\] .* fullKeyframe=true'
    )
    ;;
esac
if [[ "${#fault_markers[@]}" -gt 0 ]]; then
  for marker in "${fault_markers[@]}"; do
    if ! /usr/bin/grep -Eq "$marker" "$server_log" "$client_log"; then
      print -u2 "Fault-recovery marker missing: $marker"
      exit 6
    fi
  done
fi
for marker in "${required_client_markers[@]}"; do
  if ! /usr/bin/grep -Eq "$marker" "$client_log"; then
    print -u2 "Client marker missing: $marker"
    exit 6
  fi
done

if [[ "${#transport_markers[@]}" -gt 0 ]]; then
  for marker in "${transport_markers[@]}"; do
    if ! /usr/bin/grep -Fq "$marker" "$server_log" ||
       ! /usr/bin/grep -Fq "$marker" "$client_log"; then
      print -u2 "NetDriver packet-simulation marker missing from one or both processes: $marker"
      exit 6
    fi
  done
fi

if [[ "$transport_profile" == "loss" ]] &&
   /usr/bin/grep -Eq '\[ECHOES_NETWORK_DELTA_REJECTED\].*reason=NET_DELTA_BASE_MISSING' "$client_log"; then
  transport_recovery_markers=(
    '\[ECHOES_NETWORK_KEYFRAME_RECOVERY_REQUESTED\] .* reason=NET_DELTA_BASE_MISSING rateLimited=true'
    '\[ECHOES_NETWORK_KEYFRAME_REQUESTED\] player=1 .* recovery=fullKeyframe'
    '\[ECHOES_NETWORK_KEYFRAME_RECOVERY\] .* fullKeyframe=true'
    '\[ECHOES_NETWORK_KEYFRAME_ACKNOWLEDGED\] player=1 .* pendingSnapshots=0 lineageExact=true'
  )
  for marker in "${transport_recovery_markers[@]}"; do
    if ! /usr/bin/grep -Eq "$marker" "$server_log" "$client_log"; then
      print -u2 "Transport-loss recovery marker missing: $marker"
      exit 6
    fi
  done
fi

failure_pattern='\[ECHOES_NETWORK_.*(FAILED|REJECTED)\]|Fatal error:|Assertion failed:|Ensure condition failed:|SIGSEGV:|=== Critical error:'
failure_lines="$(/usr/bin/grep -E "$failure_pattern" "$server_log" "$client_log" || true)"
if [[ "$fault_mode" == "drop-first-delta" ||
      "$fault_mode" == "reorder-first-two-deltas" ||
      "$fault_mode" == "drop-delta-burst" ||
      "$transport_profile" == "loss" ]]; then
  failure_lines="$(print -r -- "$failure_lines" | /usr/bin/grep -v '\[ECHOES_NETWORK_DELTA_REJECTED\].*NET_DELTA_BASE_MISSING' || true)"
fi
if [[ -n "$failure_lines" ]]; then
  print -u2 "Network smoke reported a controlled failure or fatal marker."
  print -u2 "$failure_lines"
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

print "Separate-process Unreal listen-server smoke passed: connection-bound seat 1, exact compatibility admission, explicit ready/start, matched three-tick host/remote authority scheduling, rendered scoped client state, base-linked deltas with exact acknowledgements, authoritative execution, fault mode $fault_mode, and NetDriver transport profile $transport_profile."
print "Server evidence log: $server_log"
print "Client evidence log: $client_log"
