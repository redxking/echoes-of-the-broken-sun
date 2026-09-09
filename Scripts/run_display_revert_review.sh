#!/bin/zsh
set -euo pipefail

# SPEC-UI-009.CONFIRM / SPEC-UI-009.TIMEOUT rendered review.
#
# Reproduces the condition the connected-input route observed on 2026-09-09: the
# stored display preference says Borderless while the process was launched
# windowed, so UGameUserSettings describes a window that was never adopted. The
# game then applies a display change and is left alone until the fifteen-second
# wall-time deadline expires. The review reports the window the player is left
# with, which is the clause's actual subject.
#
# The run stays on the title screen, which is where a player reaches Options
# before a match. It is deliberately not -EchoesAutoStart: the gameplay shell
# view offers no Options button, so a match would not exercise this route.
#
# Agent-driven in-process review of the ordinary shell actions. It is a rendered
# observation, not physical input, not packaged execution and not owner acceptance.

project_root="${0:A:h:h}"
ue_root="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
editor="$ue_root/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
project="$project_root/EchoesOfTheBrokenSun.uproject"
evidence_dir="${ECHOES_DISPLAY_REVERT_REVIEW_DIR:-$project_root/BuildArtifacts/DisplayRevertReview}"
stored_mode="${ECHOES_DISPLAY_REVERT_STORED_MODE:-1}"   # 0 Fullscreen, 1 Borderless, 2 Windowed
launch_width="${ECHOES_DISPLAY_REVERT_WIDTH:-1280}"
launch_height="${ECHOES_DISPLAY_REVERT_HEIGHT:-720}"
log="$evidence_dir/DisplayRevertReview.log"
capture="$evidence_dir/DisplayRevertReview.png"
scope_root="$evidence_dir/Scope"

if [[ ! -x "$editor" ]]; then
  print -u2 "Unreal Editor is not available at: $editor"
  exit 2
fi

rm -rf "$evidence_dir"
mkdir -p "$evidence_dir" "$scope_root/SaveGames" "$scope_root/UserDir/Saved/Config/MacEditor"

# The run owns an isolated UserDir and save directory, so it can neither read nor
# rewrite the player's own settings. UGameUserSettings persists itself at
# shutdown; without this the review would leave its probe resolution behind.
settings_ini="$scope_root/UserDir/Saved/Config/MacEditor/GameUserSettings.ini"
cat > "$settings_ini" <<EOF
[/Script/EchoesOfTheBrokenSun.EchoesGameUserSettings]
ResolutionSizeX=$launch_width
ResolutionSizeY=$launch_height
LastUserConfirmedResolutionSizeX=$launch_width
LastUserConfirmedResolutionSizeY=$launch_height
FullscreenMode=$stored_mode
LastConfirmedFullscreenMode=$stored_mode
PreferredFullscreenMode=$stored_mode
Version=5
EOF

"$editor" "$project" /Engine/Maps/Entry \
  -game -nop4 -nosplash -nosound \
  -windowed -ResX="$launch_width" -ResY="$launch_height" -ForceRes \
  -UserDir="$scope_root/UserDir" \
  -EchoesSaveGameDirectory="$scope_root/SaveGames" \
  -EchoesDisplayRevertReview \
  -EchoesDisplayRevertReviewOutput="$capture" \
  -LogCmds="LogViewport Verbose" \
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
  if [[ -f "$log" ]] && /usr/bin/grep -q 'ECHOES_DISPLAY_REVERT_REVIEW_COMPLETE' "$log"; then
    completed=1
    break
  fi
  if ! kill -0 "$editor_pid" 2>/dev/null; then
    break
  fi
  sleep 0.5
done

# Give the requested screenshot a moment to reach disk before the process ends.
sleep 3
cleanup
trap - EXIT INT TERM

if (( completed != 1 )); then
  print -u2 "Display revert review did not reach its completion marker."
  print -u2 "Inspect: $log"
  exit 3
fi

required_markers=(
  '\[ECHOES_DISPLAY_REVERT_REVIEW_STARTED\] contract=SPEC-UI-009\.CONFIRM\+SPEC-UI-009\.TIMEOUT'
  # The stored preference must actually disagree with the window, or the run did
  # not reproduce the condition under review and proves nothing about it.
  "\\[ECHOES_DISPLAY_REVERT_REVIEW_PRESENTATION\\] stage=settled live=1 liveWindow=\\($launch_width,$launch_height\\) liveMode=Windowed .*matches=0"
  '\[ECHOES_DISPLAY_REVERT_REVIEW_OPTIONS\] applyPresent=1 applyEnabled=1 .*pendingMode=Windowed'
  '\[ECHOES_DISPLAY_REVERT_REVIEW_CONFIRMATION\] body=.*reverts automatically in '
  '\[ECHOES_DISPLAY_REVERT_REVIEW_TIMEOUT\] .*restored=1'
  '\[ECHOES_DISPLAY_REVERT_REVIEW_COMPLETE\] result=PASSED .*detail=WINDOW_RESTORED_TO_ENTRY_PRESENTATION'
)
for marker in "${required_markers[@]}"; do
  if ! /usr/bin/grep -Eq "$marker" "$log"; then
    print -u2 "Display revert review marker was absent: $marker"
    print -u2 "Inspect: $log"
    exit 4
  fi
done

if /usr/bin/grep -Eq '\[ECHOES_DISPLAY_REVERT_REVIEW_PRESENTATION\] stage=final .*liveMode=(Borderless|Fullscreen)' "$log"; then
  print -u2 "Display revert left the window in a fullscreen presentation the player did not choose."
  print -u2 "Inspect: $log"
  exit 5
fi

print "Display revert review passed: the window returned to its entry presentation after the unattended deadline."
print "Evidence log: $log"
if [[ -f "$capture" ]]; then
  print "Rendered capture: $capture"
else
  print "Rendered capture was not written; the log remains the observation of record."
fi
