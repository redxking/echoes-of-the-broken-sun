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

if [[ "${ECHOES_M01_BULWARK_PARTS_ONLY:-0}" == "1" ]]; then
  "$editor" "$project" -unattended -nop4 -nosplash -nullrhi -NoSound -SCCProvider=None \
    -ExecutePythonScript="$generator" -abslog="$log"
  rg -q '\[ECHOES_M01_BULWARK_PARTS_READY\].*revision=m01-bulwark-deployment-parts-v1.*assets=3 lods=2 collision=0' "$log"
  if rg -q 'LogPython: Error:|LogGeometry: Error:|LogStaticMesh: Error:|Failed to compile Material' "$log"; then
    exit 13
  fi
  exit 0
fi

if [[ "${ECHOES_M01_SURVEYOR_PARTS_ONLY:-0}" == "1" ]]; then
  "$editor" "$project" -unattended -nop4 -nosplash -nullrhi -NoSound -SCCProvider=None \
    -ExecutePythonScript="$generator" -abslog="$log"
  rg -q '\[ECHOES_M01_SURVEYOR_PARTS_READY\].*revision=m01-surveyor-articulation-v1.*assets=4 lods=2 collision=0' "$log"
  if rg -q 'LogPython: Error:|LogGeometry: Error:|LogStaticMesh: Error:|Failed to compile Material' "$log"; then
    exit 13
  fi
  exit 0
fi

if [[ "${ECHOES_M01_SHROUD_ONLY:-0}" == "1" ]]; then
  "$editor" "$project" -unattended -nop4 -nosplash -nullrhi -NoSound -SCCProvider=None \
    -ExecutePythonScript="$generator" -abslog="$log"
  rg -q '\[ECHOES_M01_SHROUD_READY\].*revision=m01-shroud-unlit-v3 assets=2 opaque=true' "$log"
  if rg -q 'LogPython: Error:|Failed to compile Material' "$log"; then
    exit 13
  fi
  exit 0
fi

if [[ "${ECHOES_CLIFF_MATERIAL_ONLY:-0}" == "1" ]]; then
  "$editor" "$project" -unattended -nop4 -nosplash -nullrhi -NoSound -SCCProvider=None \
    -ExecutePythonScript="$generator" -abslog="$log"
  rg -q '\[ECHOES_CLIFF_MATERIAL_READY\].*revision=cliff-surface-3d-basalt-v4 assets=1 emissive=false' "$log"
  if rg -q 'LogPython: Error:|Failed to compile Material' "$log"; then
    exit 13
  fi
  exit 0
fi

if [[ "${ECHOES_MERIDIAN_FACING_ONLY:-0}" == "1" ]]; then
  "$editor" "$project" -unattended -nop4 -nosplash -nullrhi -NoSound -SCCProvider=None \
    -ExecutePythonScript="$generator" -abslog="$log"
  rg -q '\[ECHOES_MERIDIAN_FACING_READY\].*revision=meridian-forward-axis-v4 assets=2 lods=2' "$log"
  if rg -q 'LogPython: Error:|LogGeometry: Error:|LogStaticMesh: Error:|Failed to compile Material' "$log"; then
    exit 13
  fi
  exit 0
fi

if [[ "${ECHOES_EVACUATION_PROPS_ONLY:-0}" == "1" ]]; then
  "$editor" "$project" -unattended -nop4 -nosplash -nullrhi -NoSound -SCCProvider=None \
    -ExecutePythonScript="$generator" -abslog="$log"
  rg -q '\[ECHOES_EVACUATION_PROPS_READY\].*assets=6 lods=2 collision=0' "$log"
  if rg -q 'LogPython: Error:|LogGeometry: Error:|LogStaticMesh: Error:|Failed to compile Material' "$log"; then
    exit 13
  fi
  exit 0
fi

if [[ "${ECHOES_ABILITY_RING_ONLY:-0}" == "1" ]]; then
  "$editor" "$project" -unattended -nop4 -nosplash -nullrhi -NoSound -SCCProvider=None \
    -ExecutePythonScript="$generator" -abslog="$log"
  rg -q '\[ECHOES_ABILITY_RING_READY\].*assets=1 lods=2 collision=0' "$log"
  if rg -q 'LogPython: Error:|LogGeometry: Error:|LogStaticMesh: Error:|Failed to compile Material' "$log"; then
    exit 13
  fi
  exit 0
fi

# World-only regeneration leaves roster, VFX, texture masters and unrelated assets intact.
if [[ "${ECHOES_WORLD_KITS_ONLY:-0}" == "1" ]]; then
  "$editor" "$project" -unattended -nop4 -nosplash -nullrhi -NoSound -SCCProvider=None \
    -ExecutePythonScript="$generator" -abslog="$log"
  if ! rg -q '\[ECHOES_WORLD_KITS_READY\].*assets=16.*authority=presentation' "$log"; then
    print -u2 "World-kit generation did not finish; inspect $log"
    exit 12
  fi
  if rg -q 'LogPython: Error:|LogGeometry: Error:|LogStaticMesh: Error:|Failed to compile Material|Missing Clamp input' "$log"; then
    print -u2 "World-kit generation reported an error; inspect $log"
    exit 13
  fi
  print "World-kit candidates generated; rendered qualification remains separate."
  exit 0
fi

purge="$project_root/Scripts/purge_stale_art_masters.py"
purge_log="$project_root/Saved/Logs/ArtAssetPurge.log"

"$editor" "$project" \
  -unattended -nop4 -nosplash -nullrhi -NoSound -SCCProvider=None \
  -ExecutePythonScript="$purge" \
  -abslog="$purge_log"

if ! grep -q '\[ECHOES_ART_PURGE_READY\]' "$purge_log"; then
  print -u2 "The stale-art purge pass did not complete."
  print -u2 "Inspect: $purge_log"
  exit 3
fi

"$editor" "$project" \
  -unattended -nop4 -nosplash -nullrhi -NoSound -SCCProvider=None \
  -ExecutePythonScript="$generator" \
  -abslog="$log"

if ! grep -Eq '\[ECHOES_ART_COMPLETE\] generated=49 roster=24 landmarks=4 environment=9 vfx=9 destructionVfx=3' "$log"; then
  print -u2 "The Unreal art generator did not report all 49 assets."
  print -u2 "Inspect: $log"
  exit 3
fi

if ! grep -Eq '\[ECHOES_ROSTER_READY\].*revision=roster-silhouette-v2.*assets=24.*lods=2.*runtimeAuthority=presentation' "$log"; then
  print -u2 "The roster mesh audit did not pass."
  print -u2 "Inspect: $log"
  exit 11
fi

if ! grep -Eq '\[ECHOES_WORLD_SURFACE_READY\].*revision=world-surface-textured-v7.*action=(created|repaired|reused).*instancedStaticMeshes=true' "$log"; then
  print -u2 "The world-surface material is not qualified for instanced terrain."
  print -u2 "Inspect: $log"
  exit 10
fi

if ! grep -Eq '\[ECHOES_DESTRUCTION_VFX_READY\].*revision=destruction-vfx-v1.*assets=3 lods=2 simpleCollision=0.*reducedMotion=steady.*reducedFlashing=steadyLowEmission' "$log"; then
  print -u2 "The destruction VFX asset audit did not pass."
  print -u2 "Inspect: $log"
  exit 7
fi

if ! grep -Eq '\[ECHOES_PRESENTATION_VFX_READY\].*revision=selection-command-vfx-v2.*assets=9 selection=1 commands=7 orbit=1 lods=2 simpleCollision=0.*reducedMotion=steady.*reducedFlashing=steadyLowEmission' "$log"; then
  print -u2 "The selection and command VFX asset audit did not pass."
  print -u2 "Inspect: $log"
  exit 6
fi

if ! grep -Eq '\[ECHOES_ASH_CUT_READY\].*revision=ash-cut-production-v1.*uvChannels=2,2.*materials=4.*simpleCollision=1' "$log"; then
  print -u2 "The Ash Cut route-kit audit did not pass."
  print -u2 "Inspect: $log"
  exit 5
fi

if ! grep -Eq '\[ECHOES_BURIED_CAUSEWAY_READY\].*revision=buried-causeway-production-v3.*uvChannels=2,2.*materials=4.*simpleCollision=1' "$log"; then
  print -u2 "The Buried Causeway route-kit audit did not pass."
  print -u2 "Inspect: $log"
  exit 8
fi

if ! grep -Eq '\[ECHOES_FOLDED_VERGE_READY\].*revision=folded-verge-production-v1.*uvChannels=2,2.*materials=4.*simpleCollision=1' "$log"; then
  print -u2 "The Folded Verge route-kit audit did not pass."
  print -u2 "Inspect: $log"
  exit 9
fi

if grep -Eq 'LogPython: Error:|LogGeometry: Error:|LogStaticMesh: Error:|LogEditorAssetSubsystem: Error:' "$log"; then
  print -u2 "The Unreal art generator reported an error."
  print -u2 "Inspect: $log"
  exit 4
fi

print "Generated 24 roster meshes, 4 Future Well meshes, 8 Glass Scar environment meshes, 9 selection/command VFX meshes, 3 destruction VFX meshes, and their authored material families."
print "Evidence log: $log"
