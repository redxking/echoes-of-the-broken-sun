#!/bin/zsh
set -euo pipefail

project_root="${0:A:h:h}"
review_script="$project_root/Scripts/run_pointer_combat_guard_review.sh"
matrix_root="${ECHOES_POINTER_RESOLUTION_MATRIX_DIR:-$project_root/BuildArtifacts/PointerCombatGuardResolutionMatrix}"

ECHOES_POINTER_REVIEW_DIR="$matrix_root/Compact-1280x720" \
  "$review_script" Compact 1280 720
ECHOES_POINTER_REVIEW_DIR="$matrix_root/Mac16x10-1440x900" \
  "$review_script" Mac16x10 1440 900
ECHOES_POINTER_REVIEW_DIR="$matrix_root/FullHD-1920x1080" \
  "$review_script" FullHD 1920 1080

print "Pointer combat/Guard resolution and aspect-ratio matrix passed: Compact 1280x720, Mac 16:10 1440x900, and Full HD 1920x1080."
print "Matrix evidence: $matrix_root"
