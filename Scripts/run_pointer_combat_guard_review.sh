#!/bin/zsh
set -euo pipefail

project_root="${0:A:h:h}"
variant="${1:-Default}"
expected_width="${2:-1600}"
expected_height="${3:-900}"
ue_root="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
editor="$ue_root/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
project="$project_root/EchoesOfTheBrokenSun.uproject"
evidence_dir="${ECHOES_POINTER_REVIEW_DIR:-$project_root/BuildArtifacts/PointerCombatGuard}"
log="$evidence_dir/PointerCombatGuard.log"
capture="$evidence_dir/PointerCombatGuard.png"

if [[ ! -x "$editor" ]]; then
  print -u2 "Unreal Editor is not available at: $editor"
  exit 2
fi

mkdir -p "$evidence_dir"
rm -f "$log" "$capture"

"$editor" "$project" /Engine/Maps/Entry \
  -game -nop4 -nosplash -nosound -windowed \
  -ResX="$expected_width" -ResY="$expected_height" \
  -EchoesAutoStart -EchoesPointerCombatGuardReview \
  -EchoesPointerCombatGuardReviewVariant="$variant" \
  -EchoesPointerCombatGuardReviewOutput="$capture" \
  -benchmark -fps=20 -benchmarkseconds=12 -AbsLog="$log"

required_markers=(
  '\[ECHOES_POINTER_COMBAT_GUARD_FIXTURE\].*authoritativeCommands=true.*controlledNonshipping=true'
  "\\[ECHOES_POINTER_COMBAT_GUARD_REVIEW_STARTED\\] variant=$variant .*nonOcclusionRequired=true"
  "\\[ECHOES_POINTER_COMBAT_GUARD_REVIEW_CAMERA\\] variant=$variant .*exactScreenProjection=true"
  "\\[ECHOES_POINTER_REVIEW_COORDINATE\\] variant=$variant stage=select_defender .*viewport=\\($expected_width,$expected_height\\).*fullBoundsVisible=true hudOcclusion=false"
  "\\[ECHOES_POINTER_REVIEW_COORDINATE\\] variant=$variant stage=guard_target .*viewport=\\($expected_width,$expected_height\\).*fullBoundsVisible=true hudOcclusion=false"
  "\\[ECHOES_POINTER_REVIEW_COORDINATE\\] variant=$variant stage=direct_attack_target .*viewport=\\($expected_width,$expected_height\\).*fullBoundsVisible=true hudOcclusion=false"
  '\[ECHOES_POINTER_SELECTION\].*entity=[1-9][0-9]*.*ownerScoped=true'
  '\[ECHOES_GUARD_ACCEPTED\] source=pointer.*accepted=1.*ownerScoped=true'
  '\[ECHOES_POINTER_GUARD_OBSERVED\].*order=Guard.*authoritativeState=true'
  '\[ECHOES_CONTEXT_ORDER_ACCEPTED\] source=pointer.*command=ATTACK.*target=[1-9][0-9]*.*visibleHit=true'
  "\\[ECHOES_POINTER_COMBAT_GUARD_REVIEW_COMPLETE\\] variant=$variant .*hudOcclusion=false.*authoritativeCommands=true.*authoritativeDamage=true.*osInjection=false.*unaidedHuman=false.*controlledNonshipping=true"
)
for marker in "${required_markers[@]}"; do
  if ! /usr/bin/grep -Eq "$marker" "$log"; then
    print -u2 "Pointer combat/Guard review marker was absent: $marker"
    print -u2 "Inspect: $log"
    exit 3
  fi
done

if /usr/bin/grep -Eq '\[ECHOES_POINTER_COMBAT_GUARD_REVIEW_FAILED\]|\[ECHOES_POINTER_REVIEW_OCCLUDED\]|\[ECHOES_POINTER_REVIEW_VIEWPORT_MISMATCH\]|\[ECHOES_BOOT_INCOMPLETE\]|\[ECHOES_CONTENT_FAILED\]|\[ECHOES_SIM_VIEW_SYNC_FAILED\]|Fatal error:|Assertion failed:' "$log"; then
  print -u2 "The controlled pointer combat/Guard review reported a failure. Inspect: $log"
  exit 4
fi

if [[ ! -s "$capture" ]]; then
  print -u2 "The controlled pointer combat/Guard capture was not written: $capture"
  exit 5
fi

dimensions="$(/usr/bin/sips -g pixelWidth -g pixelHeight "$capture" 2>/dev/null)"
if [[ "$dimensions" != *"pixelWidth: $expected_width"* ||
      "$dimensions" != *"pixelHeight: $expected_height"* ]]; then
  print -u2 "The pointer combat/Guard capture is not $expected_width x $expected_height. Inspect: $capture"
  exit 6
fi

print "Controlled pointer combat/Guard review passed for $variant: unobscured exact-coordinate LMB selection, J Guard, RMB direct attack, and authoritative damage were observed."
print "Evidence log: $log"
print "Rendered capture: $capture"
