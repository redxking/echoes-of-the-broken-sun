#!/bin/zsh
set -euo pipefail

project_root="${0:A:h:h}"
ue_root="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
editor="$ue_root/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
project="$project_root/EchoesOfTheBrokenSun.uproject"

if [[ ! -x "$editor" ]]; then
  print -u2 "Unreal Editor is not available at: $editor"
  exit 2
fi

"$editor" "$project" \
  -unattended -nop4 -nosplash -nullrhi \
  -ExecCmds="Automation RunTests Echoes; Quit" \
  -TestExit="Automation Test Queue Empty" \
  -ReportOutputPath="$project_root/BuildArtifacts/Automation"
