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

if ! rg -q '\[ECHOES_ART_COMPLETE\] generated=39 roster=16 landmarks=4 environment=7 vfx=9 destructionVfx=3' "$log"; then
  print -u2 "The Unreal art generator did not report all 39 assets."
  print -u2 "Inspect: $log"
  exit 3
fi

if ! rg -q '\[ECHOES_DESTRUCTION_VFX_READY\].*revision=destruction-vfx-v1.*assets=3 lods=2 simpleCollision=0.*reducedMotion=steady.*reducedFlashing=steadyLowEmission' "$log"; then
  print -u2 "The destruction VFX asset audit did not pass."
  print -u2 "Inspect: $log"
  exit 7
fi

if ! rg -q '\[ECHOES_PRESENTATION_VFX_READY\].*revision=selection-command-vfx-v2.*assets=9 selection=1 commands=7 orbit=1 lods=2 simpleCollision=0.*reducedMotion=steady.*reducedFlashing=steadyLowEmission' "$log"; then
  print -u2 "The selection and command VFX asset audit did not pass."
  print -u2 "Inspect: $log"
  exit 6
fi

if ! rg -q '\[ECHOES_ASH_CUT_READY\].*revision=ash-cut-production-v1.*uvChannels=2,2.*materials=4.*simpleCollision=1' "$log"; then
  print -u2 "The Ash Cut route-kit audit did not pass."
  print -u2 "Inspect: $log"
  exit 5
fi

if ! rg -q '\[ECHOES_BURIED_CAUSEWAY_READY\].*revision=buried-causeway-production-v1.*uvChannels=2,2.*materials=4.*simpleCollision=1' "$log"; then
  print -u2 "The Buried Causeway route-kit audit did not pass."
  print -u2 "Inspect: $log"
  exit 8
fi

if ! rg -q '\[ECHOES_FOLDED_VERGE_READY\].*revision=folded-verge-production-v1.*uvChannels=2,2.*materials=4.*simpleCollision=1' "$log"; then
  print -u2 "The Folded Verge route-kit audit did not pass."
  print -u2 "Inspect: $log"
  exit 9
fi

if rg -q 'LogPython: Error:|LogGeometry: Error:|LogStaticMesh: Error:|LogEditorAssetSubsystem: Error:' "$log"; then
  print -u2 "The Unreal art generator reported an error."
  print -u2 "Inspect: $log"
  exit 4
fi

print "Generated 16 roster meshes, 4 Future Well meshes, 7 Glass Scar environment meshes, 9 selection/command VFX meshes, 3 destruction VFX meshes, and their authored material families."
print "Evidence log: $log"
