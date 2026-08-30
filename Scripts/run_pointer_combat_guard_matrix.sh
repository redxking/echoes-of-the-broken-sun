#!/bin/zsh
set -euo pipefail

project_root="${0:A:h:h}"
review_script="$project_root/Scripts/run_pointer_combat_guard_review.sh"
matrix_root="${ECHOES_POINTER_MATRIX_DIR:-$project_root/BuildArtifacts/PointerCombatGuardMatrix}"

for variant in Default Offset MinHud MaxHud; do
  ECHOES_POINTER_REVIEW_DIR="$matrix_root/$variant" \
    "$review_script" "$variant" 1600 900
done

print "Pointer combat/Guard adverse camera and HUD-scale matrix passed: Default, Offset, MinHud, and MaxHud."
print "Matrix evidence: $matrix_root"
