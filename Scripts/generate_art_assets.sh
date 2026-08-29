#!/bin/zsh
set -euo pipefail

project_root="${0:A:h:h}"
ue_root="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
editor="$ue_root/Engine/Binaries/Mac/UnrealEditor-Cmd"
project="$project_root/EchoesOfTheBrokenSun.uproject"
generator="$project_root/Scripts/generate_art_assets.py"
log="$project_root/Saved/Logs/ArtAssetGeneration.log"

if [[ ! -x "$editor" ]]; then
  print -u2 "Unreal command editor is not available at: $editor"
  exit 2
fi

mkdir -p "$project_root/Saved/Logs"

"$editor" "$project" \
  -unattended -nop4 -nosplash -nullrhi -NoSound \
  -ExecutePythonScript="$generator" \
  -abslog="$log"

if ! rg -q '\[ECHOES_ART_COMPLETE\] generated=27 roster=16 landmarks=4 environment=7' "$log"; then
  print -u2 "The Unreal art generator did not report all 27 assets."
  print -u2 "Inspect: $log"
  exit 3
fi

if rg -q 'LogPython: Error:|LogGeometry: Error:|LogStaticMesh: Error:' "$log"; then
  print -u2 "The Unreal art generator reported an error."
  print -u2 "Inspect: $log"
  exit 4
fi

print "Generated 16 roster meshes, 4 Future Well meshes, 7 Glass Scar environment meshes, and the shared surface material."
print "Evidence log: $log"
