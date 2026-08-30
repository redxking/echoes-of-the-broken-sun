#!/bin/zsh
set -euo pipefail

project_root="${0:A:h:h}"
review_mode="${1:-lobby}"
ue_root="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
editor="$ue_root/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
project="$project_root/EchoesOfTheBrokenSun.uproject"
port="${ECHOES_NETWORK_PORT:-7800}"
evidence_dir="${ECHOES_NETWORK_VISUAL_DIR:-$project_root/BuildArtifacts/NetworkVisual/$review_mode}"
server_log="$evidence_dir/Server.log"
client_log="$evidence_dir/Client.log"
capture="$evidence_dir/${review_mode}.png"

if [[ ! -x "$editor" ]]; then
  print -u2 "Unreal Editor is not available at: $editor"
  exit 2
fi
if [[ "$review_mode" != "lobby" && "$review_mode" != "battlefield" ]]; then
  print -u2 "Review mode must be lobby or battlefield."
  exit 2
fi
if [[ "$port" != <-> || "$port" -lt 1024 || "$port" -gt 65535 ]]; then
  print -u2 "ECHOES_NETWORK_PORT must be an integer from 1024 through 65535."
  exit 2
fi

mkdir -p "$evidence_dir"
rm -f "$server_log" "$client_log" "$capture"
: > "$server_log"
: > "$client_log"
server_pid=""

cleanup_server() {
  if [[ -n "$server_pid" ]] && kill -0 "$server_pid" 2>/dev/null; then
    kill "$server_pid" 2>/dev/null || true
    wait "$server_pid" 2>/dev/null || true
  fi
}
trap cleanup_server EXIT INT TERM

"$editor" "$project" "/Engine/Maps/Entry?listen" \
  -game -unattended -nop4 -nosplash -nullrhi -nosound \
  -port="$port" -EchoesAutoStart -AbsLog="$server_log" &
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
  print -u2 "Visual-review listen server did not become ready. Inspect: $server_log"
  exit 3
fi

client_mode_args=()
if [[ "$review_mode" == "battlefield" ]]; then
  client_mode_args+=("-EchoesNetworkVisualReview")
fi

"$editor" "$project" "127.0.0.1:${port}" \
  -game -nop4 -nosplash -nosound -windowed \
  -ResX=1440 -ResY=900 \
  "${client_mode_args[@]}" \
  -EchoesArtReview -EchoesArtReviewOutput="$capture" \
  -benchmark -fps=20 -benchmarkseconds=5 -AbsLog="$client_log"

common_markers=(
  '\[ECHOES_NETWORK_COMPATIBILITY_RESULT\] accepted=true reason=NET_COMPATIBLE'
  '\[ECHOES_NETWORK_LOBBY_RESULT\] compatible=true started=false seat=1 authorityTick=0 inputDelayTicks=3'
  '\[ECHOES_ART_REVIEW_CAPTURE\] requested=true showUI=true delay=1.5'
)
for marker in "${common_markers[@]}"; do
  if ! /usr/bin/grep -Eq "$marker" "$client_log"; then
    print -u2 "Visual-review client marker missing: $marker"
    exit 4
  fi
done

if [[ "$review_mode" == "battlefield" ]]; then
  battlefield_markers=(
    '\[ECHOES_NETWORK_LOBBY_RESULT\] compatible=true started=true seat=1 authorityTick=0 inputDelayTicks=3'
    '\[ECHOES_NETWORK_VISUAL_REVIEW_CAMERA\] scopedSeat=1 centerTile=\(52,52\) zoom=2600 editorOnly=true'
    '\[ECHOES_NETWORK_KEYFRAME_RECEIVED\] player=1 .* hiddenAuthorityExcluded=true'
    '\[ECHOES_NETWORK_DELTA_RECEIVED\] player=1 .* hiddenAuthorityExcluded=true'
    '\[ECHOES_NETWORK_PRESENTATION_SYNCED\] .* ground=true terrain=true fog=true lighting=true scopedOnly=true rendered=true'
  )
  for marker in "${battlefield_markers[@]}"; do
    if ! /usr/bin/grep -Eq "$marker" "$client_log"; then
      print -u2 "Battlefield review marker missing: $marker"
      exit 5
    fi
  done
elif /usr/bin/grep -q '\[ECHOES_NETWORK_LOBBY_RESULT\] compatible=true started=true' "$client_log"; then
  print -u2 "Lobby review advanced into the match before capture."
  exit 5
fi

if /usr/bin/grep -Eq '\[ECHOES_NETWORK_.*(FAILED|REJECTED)\]|Fatal error:|Assertion failed:|Ensure condition failed:' "$server_log" "$client_log"; then
  print -u2 "Network visual review reported a controlled failure or fatal marker."
  exit 6
fi
if [[ ! -s "$capture" ]]; then
  print -u2 "Network visual-review capture was not written: $capture"
  exit 7
fi
dimensions="$(/usr/bin/sips -g pixelWidth -g pixelHeight "$capture" 2>/dev/null)"
if [[ "$dimensions" != *"pixelWidth: 1440"* ||
      "$dimensions" != *"pixelHeight: 900"* ]]; then
  print -u2 "Network visual-review capture is not 1440 x 900: $capture"
  exit 8
fi

cleanup_server
server_pid=""
trap - EXIT INT TERM

print "Rendered network $review_mode review passed at 1440 x 900."
print "Client evidence log: $client_log"
print "Server evidence log: $server_log"
print "Rendered capture: $capture"
