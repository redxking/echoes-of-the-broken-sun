#!/bin/zsh
set -euo pipefail

# SPEC-OUT-002 / SPEC-OUT-006 rendered review.
#
# SPEC-OUT-002 separates the two ways a player loses: the final Command Core
# destroyed, or a confirmed concession. Simulation::ForfeitPlayer retires the
# conceding seat's Core to end the match deterministically, so the outcome enum
# alone reports every concession as a Corefall - and the end-of-match banner told
# a conceding player "DEFEAT - your Command Core has fallen." when it had not.
#
# This drives the ordinary player route in a real rendered window: title ->
# Skirmish -> deployment review -> Deploy -> pause -> Concede -> Confirm, then
# reads the banner and the result dossier on the FIRST composed result frame,
# before the async replay archive publishes. That window is where a player first
# reads the screen and where the cause used to be unavailable.
#
# Agent-driven in-process review of the ordinary shell actions. It is a rendered
# observation, not physical input, not packaged execution and not owner acceptance.

project_root="${0:A:h:h}"
ue_root="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
editor="$ue_root/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
project="$project_root/EchoesOfTheBrokenSun.uproject"
evidence_dir="${ECHOES_CONCESSION_REVIEW_DIR:-$project_root/BuildArtifacts/ConcessionResultReview}"
log="$evidence_dir/ConcessionResultReview.log"
capture="$evidence_dir/ConcessionResultReview.png"
scope_root="$evidence_dir/Scope"

if [[ ! -x "$editor" ]]; then
  print -u2 "Unreal Editor is not available at: $editor"
  exit 2
fi

rm -rf "$evidence_dir"
mkdir -p "$evidence_dir" "$scope_root/SaveGames" "$scope_root/UserDir"

# The run owns an isolated UserDir and save directory, so it can neither read nor
# rewrite the player's own profile, settings, saves or replay archive.
"$editor" "$project" /Engine/Maps/Entry \
  -game -nop4 -nosplash -nosound \
  -windowed -ResX=1280 -ResY=720 -ForceRes \
  -UserDir="$scope_root/UserDir" \
  -EchoesSaveGameDirectory="$scope_root/SaveGames" \
  -EchoesConcessionResultReview \
  -EchoesConcessionReviewOutput="$capture" \
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

completed=0
for _ in {1..480}; do
  if [[ -f "$log" ]] && /usr/bin/grep -q 'ECHOES_CONCESSION_REVIEW_COMPLETE' "$log"; then
    completed=1
    break
  fi
  kill -0 "$editor_pid" 2>/dev/null || break
  sleep 0.5
done

# Give the requested screenshot a moment to reach disk before the process ends.
sleep 3
cleanup
trap - EXIT INT TERM

if (( completed != 1 )); then
  print -u2 "Concession result review did not reach its completion marker."
  print -u2 "Inspect: $log"
  exit 3
fi

required_markers=(
  '\[ECHOES_CONCESSION_REVIEW_STARTED\] contract=SPEC-OUT-002\+SPEC-OUT-006'
  # The route must reach a deployed match, or a "no Corefall claimed" result is vacuous.
  '\[ECHOES_CONCESSION_REVIEW_DEPLOYED\] screen=gameplay'
  # The Core must be standing when the concession is issued.
  '\[ECHOES_CONCESSION_REVIEW_PRECONCEDE\] tick=[0-9]+ outcome=0 forfeitingSeat=255'
  # The seat that conceded is seat 0, recorded authoritatively by the simulation.
  '\[ECHOES_CONCESSION_REVIEW_RESULT\] .*forfeitingSeat=0 bannerTruthful=1 dossierTruthful=1'
  '\[ECHOES_CONCESSION_REVIEW_COMPLETE\] result=PASSED .*detail=CONCESSION_NAMED_ON_BANNER_AND_DOSSIER'
)
for marker in "${required_markers[@]}"; do
  if ! /usr/bin/grep -Eq "$marker" "$log"; then
    print -u2 "Concession result review marker was absent: $marker"
    print -u2 "Inspect: $log"
    exit 4
  fi
done

# The whole point of the repair: the false sentence must not be spoken to the
# player anywhere in this run.
if /usr/bin/grep -Eq '\[ECHOES_(PLAYER_FEEDBACK|CONCESSION_REVIEW_RESULT|CONCESSION_REVIEW_DOSSIER)\].*Command Core has fallen' "$log"; then
  print -u2 "A conceded match still told the player a Command Core had fallen."
  print -u2 "Inspect: $log"
  exit 5
fi

print "Concession result review passed: a conceded match names the concession on both the banner and the result dossier, before the replay archive publishes."
print "Evidence log: $log"
if [[ -f "$capture" ]]; then
  print "Rendered capture: $capture"
else
  print "Rendered capture was not written; the log remains the observation of record."
fi
