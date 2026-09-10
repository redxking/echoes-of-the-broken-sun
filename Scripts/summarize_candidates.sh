#!/bin/bash
SALVAGE="BuildArtifacts/Evidence/worktree-salvage-20260910T000500Z"
for d in "$SALVAGE"/*; do
  if [ -d "$d" ]; then
    echo "=== $(basename "$d") ==="
    grep "^+++ b/" "$d/tracked.patch" | head -n 15 | sed 's/+++ b\///'
    echo "..."
  fi
done
