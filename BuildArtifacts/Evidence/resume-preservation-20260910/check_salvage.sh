#!/bin/bash
set -e

WORKSPACE_ROOT="/Volumes/Seagate Game Archive/EchoesOfTheBrokenSun"
SALVAGE_DIR="$WORKSPACE_ROOT/Project/BuildArtifacts/Evidence/worktree-salvage-20260910T000500Z"
echo "--- Verifying salvage archives ---"

for wtdir in "$SALVAGE_DIR"/*; do
  if [ -d "$wtdir" ]; then
    wtname=$(basename "$wtdir")
    echo "Checking $wtname"
    head_file="$wtdir/identity.json"
    
    # Check if tarball is readable
    tar -tzf "$wtdir/untracked.tar.gz" > /dev/null
    echo "Archive readable."
    
    wtpath_rel=$(jq -r '.worktree' "$head_file")
    wtpath="$WORKSPACE_ROOT/$wtpath_rel"
    
    if [ -d "$wtpath" ]; then
      cd "$wtpath"
      current_head=$(git rev-parse HEAD)
      echo "Current HEAD: $current_head"
      git diff HEAD --binary > "$WORKSPACE_ROOT/Project/BuildArtifacts/Evidence/resume-preservation-20260910/$wtname-current.patch"
      cd - > /dev/null
      
      diff -q "$wtdir/tracked.patch" "$WORKSPACE_ROOT/Project/BuildArtifacts/Evidence/resume-preservation-20260910/$wtname-current.patch" && echo "Patch unchanged." || echo "Patch CHANGED."
    else
      echo "Worktree $wtpath not found."
    fi
    echo ""
  fi
done
