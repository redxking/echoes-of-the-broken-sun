#!/bin/zsh
set -euo pipefail

project_root="${0:A:h:h}"
ue_root="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
builder="$ue_root/Engine/Build/BatchFiles/Mac/Build.sh"
project="$project_root/EchoesOfTheBrokenSun.uproject"

if [[ ! -x "$builder" ]]; then
  print -u2 "Unreal build script not found at: $builder"
  print -u2 "Complete UE 5.8 installation or set UE_ROOT."
  exit 2
fi

"$builder" EchoesOfTheBrokenSunEditor Mac Development "$project" -waitmutex

