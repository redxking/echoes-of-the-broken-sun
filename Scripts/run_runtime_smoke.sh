#!/bin/zsh
set -euo pipefail

project_root="${0:A:h:h}"
ue_root="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
editor="$ue_root/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
project="$project_root/EchoesOfTheBrokenSun.uproject"
log="$project_root/BuildArtifacts/RuntimeSmoke.log"

if [[ ! -x "$editor" ]]; then
  print -u2 "Unreal Editor is not available at: $editor"
  exit 2
fi

mkdir -p "$project_root/BuildArtifacts"
: > "$log"

"$editor" "$project" /Engine/Maps/Entry \
  -game -unattended -nop4 -nosplash -nullrhi -nosound \
  -benchmark -fps=20 -benchmarkseconds=3 -AbsLog="$log"

if ! /usr/bin/grep -q '\[ECHOES_ENV_READY\]' "$log" ||
   ! /usr/bin/grep -q '\[ECHOES_SIM_READY\]' "$log" ||
   ! /usr/bin/grep -q '\[ECHOES_BOOT_READY\]' "$log" ||
   ! /usr/bin/grep -q '\[ECHOES_SIM_FIRST_TICK\]' "$log"; then
  print -u2 "Runtime smoke markers were incomplete. Inspect: $log"
  exit 3
fi

if /usr/bin/grep -Eq '\[ECHOES_BOOT_INCOMPLETE\]|\[ECHOES_BOOT_NO_SUBSYSTEM\]|\[ECHOES_SIM_VIEW_SYNC_FAILED\]|Fatal error:|Assertion failed:' "$log"; then
  print -u2 "Runtime smoke reported a boot or fatal failure. Inspect: $log"
  exit 4
fi

print "Runtime bootstrap passed: environment, 20 Hz simulation, and first fixed tick initialized."
print "Evidence log: $log"
