#!/bin/zsh
set -euo pipefail

project_root="${0:A:h:h}"
ue_root="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
generator="$ue_root/Engine/Build/BatchFiles/Mac/GenerateProjectFiles.sh"
project="$project_root/EchoesOfTheBrokenSun.uproject"

if [[ ! -x "$generator" ]]; then
  print -u2 "Unreal project generator not found at: $generator"
  print -u2 "Set UE_ROOT to the completed UE 5.8 installation."
  exit 2
fi

"$generator" -project="$project" -game

