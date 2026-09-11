#!/bin/zsh
set -euo pipefail

# DeliveryPlan D2 exit chain, rendered review.
#
# Drives the ordinary player route in a real rendered window on Glass Scar:
# title -> Skirmish -> Deploy, then gather to delivery, place and finish a
# structure, harvest the Future Well, train to the 30-entity limit and read the
# refusal, move and fight with visible health change, quick-save and quick-load,
# repair a damaged owned target, and end with a truthful result. One capture per
# stage lands in the evidence directory beside the engine log.
#
# Agent-driven in-process review of the ordinary controller and bridge actions.
# It is a rendered observation, not physical input, not packaged execution and
# not owner acceptance. Author and owner: Angelis Pseftis.

project_root="${0:A:h:h}"
ue_root="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
editor="$ue_root/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
project="$project_root/EchoesOfTheBrokenSun.uproject"
stamp="$(date -u +%Y%m%dT%H%M%SZ)"
evidence_dir="${ECHOES_D2_EXIT_REVIEW_DIR:-$project_root/BuildArtifacts/Evidence/d2-exit-review-$stamp}"
log="$evidence_dir/D2ExitReview.log"
captures="$evidence_dir/captures"
scope_root="$evidence_dir/Scope"
budget_seconds="${ECHOES_D2_EXIT_REVIEW_BUDGET:-1500}"

if [[ ! -x "$editor" ]]; then
  print -u2 "Unreal Editor is not available at: $editor"
  exit 2
fi
if pgrep -x UnrealEditor >/dev/null; then
  print -u2 "An UnrealEditor process is already running; the review needs the window and the build slot to itself."
  exit 6
fi

# The Metal RHI constructor runs the Xcode Metal compiler check and, when it
# fails, opens a modal dialog that a windowed -game process can never dismiss.
# Fail here with the reason instead (2026-09-11: a Command Line Tools update
# left the Metal toolchain unavailable for a while).
if ! /usr/bin/xcrun -sdk macosx metal -v >/dev/null 2>&1; then
  print -u2 "The Xcode Metal compiler is unavailable (xcrun -sdk macosx metal -v failed); the engine would stop on a modal dialog."
  print -u2 "Check Xcode and the Metal toolchain: xcodebuild -showComponent MetalToolchain"
  exit 8
fi

mkdir -p "$evidence_dir" "$captures" "$scope_root/SaveGames" "$scope_root/UserDir"
{
  print "commit=$(git -C "$project_root" rev-parse HEAD)"
  print "dirty=$(git -C "$project_root" status --short | wc -l | tr -d ' ')"
  print "started_utc=$stamp"
  print "editor=$editor"
  shasum -a 256 "$project_root/Binaries/Mac/libUnrealEditor-EchoesOfTheBrokenSun.dylib" "$project_root/Binaries/Mac/libUnrealEditor-EchoesSimCore.dylib"
} > "$evidence_dir/identity.txt"

# The run owns an isolated UserDir and save directory, so it can neither read nor
# rewrite the player's own profile, settings, saves or replay archive.
# No -unattended here: the project reads FApp::IsUnattended in its game mode,
# shell and HUD and changes the player route under it, so the review would no
# longer walk the player's path. The startup guard below covers the one stall
# that flag would have avoided.
"$editor" "$project" /Engine/Maps/Entry \
  -game -nop4 -nosplash -nosound \
  -windowed -ResX=1280 -ResY=720 -ForceRes \
  -UserDir="$scope_root/UserDir" \
  -EchoesSaveGameDirectory="$scope_root/SaveGames" \
  -EchoesD2ExitReview \
  -EchoesD2ExitReviewOutputDir="$captures" \
  ${=ECHOES_D2_EXIT_REVIEW_EXTRA_ARGS:-} \
  -AbsLog="$log" &
editor_pid=$!

cleanup() {
  if kill -0 "$editor_pid" 2>/dev/null; then
    kill -TERM "$editor_pid" 2>/dev/null || true
    for _ in {1..40}; do
      kill -0 "$editor_pid" 2>/dev/null || break
      sleep 0.25
    done
    kill -9 "$editor_pid" 2>/dev/null || true
  fi
}
trap cleanup EXIT INT TERM

# Startup guard: the engine must reach a running game world quickly. A process
# that is alive but has not initialized is parked on something no one can
# answer; sample it for the record and stop.
startup_elapsed=0
until [[ -f "$log" ]] && /usr/bin/grep -q 'Game Engine Initialized' "$log"; do
  if ! kill -0 "$editor_pid" 2>/dev/null; then
    print -u2 "The editor exited before the engine initialized. Inspect: $log"
    exit 5
  fi
  if (( startup_elapsed >= 300 )); then
    /usr/bin/sample "$editor_pid" 3 -file "$evidence_dir/startup-stall.sample.txt" >/dev/null 2>&1 || true
    print -u2 "The engine did not initialize within 300s; process sampled to $evidence_dir/startup-stall.sample.txt"
    print -u2 "Check for a modal engine dialog in the sample (MessageBoxExt) and the Metal toolchain: xcrun -sdk macosx metal -v"
    exit 7
  fi
  sleep 2
  (( startup_elapsed += 2 ))
done

completed=0
elapsed=0
while (( elapsed < budget_seconds )); do
  if [[ -f "$log" ]] && /usr/bin/grep -q 'ECHOES_D2_EXIT_REVIEW_COMPLETE' "$log"; then
    completed=1
    break
  fi
  kill -0 "$editor_pid" 2>/dev/null || break
  sleep 1
  (( elapsed += 1 ))
done

# Give the last requested screenshot a moment to reach disk before the process ends.
sleep 4
cleanup
trap - EXIT INT TERM

if (( completed != 1 )); then
  if [[ -f "$log" ]] && /usr/bin/grep -q 'Engine exit requested' "$log" && ! /usr/bin/grep -q 'ECHOES_D2_EXIT_REVIEW_COMPLETE' "$log"; then
    print -u2 "The engine exited before the review completed; last review markers:"
    /usr/bin/grep -E 'ECHOES_D2_EXIT_REVIEW_' "$log" | tail -5 >&2
  fi
  print -u2 "D2 exit review did not reach its completion marker within ${budget_seconds}s."
  print -u2 "Inspect: $log"
  exit 3
fi

print "D2 exit review completed. Stage record:"
/usr/bin/grep -E '\[ECHOES_D2_EXIT_REVIEW_(STAGE|CAPTURE|COMPLETE)\]' "$log" | sed 's/^.*LogEchoes: Display: //'
print "Captures:"
ls -1 "$captures"
print "Evidence directory: $evidence_dir"
if /usr/bin/grep -q 'ECHOES_D2_EXIT_REVIEW_COMPLETE\] result=PASSED' "$log"; then
  exit 0
fi
print -u2 "The review completed with unproven stages; read the COMPLETE line above."
exit 4
