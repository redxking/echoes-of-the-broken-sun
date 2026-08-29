#!/bin/zsh
set -euo pipefail

project_root="${0:A:h:h}"
ue_root="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
editor="$ue_root/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
project="$project_root/EchoesOfTheBrokenSun.uproject"
log="${ECHOES_PROLOGUE_LOG:-$project_root/BuildArtifacts/PrologueSmoke.log}"
if [[ "$log" != /* ]]; then
  log="$project_root/$log"
fi

if [[ ! -x "$editor" ]]; then
  print -u2 "Unreal Editor is not available at: $editor"
  exit 2
fi

mkdir -p "$project_root/BuildArtifacts"
: > "$log"

"$editor" "$project" /Engine/Maps/Entry \
  -game -unattended -nop4 -nosplash -nullrhi -nosound \
  -EchoesCampaignPrologue -EchoesAutoStart \
  -benchmark -fps=20 -benchmarkseconds=3 -AbsLog="$log"

required_markers=(
  '\[ECHOES_OPERATION_REQUESTED\] operation=WhatTheLedgerKeeps accepted=true'
  '\[ECHOES_FACTION_SCENARIO_READY\] local=MeridianCompact opposition=KharuunAssemblies selectable=true'
  '\[ECHOES_PROLOGUE_READY\] mission=WhatTheLedgerKeeps carrier=[1-9][0-9]* archive=\(22,18\) evacuation=\(6,17\) faction=MeridianCompact completion=withdrawal'
  '\[ECHOES_BOOT_READY\]'
  '\[ECHOES_SIM_FIRST_TICK\]'
)
for marker in "${required_markers[@]}"; do
  if ! /usr/bin/grep -Eq "$marker" "$log"; then
    print -u2 "Campaign prologue runtime marker was absent: $marker"
    print -u2 "Inspect: $log"
    exit 3
  fi
done

if /usr/bin/grep -Eq '\[ECHOES_OPERATION_REQUEST_REJECTED\]|\[ECHOES_PROLOGUE_INIT_FAILED\]|\[ECHOES_BOOT_INCOMPLETE\]|Fatal error:|Assertion failed:' "$log"; then
  print -u2 "Campaign prologue runtime reported an initialization or fatal failure. Inspect: $log"
  exit 4
fi

print "Campaign prologue runtime passed: Mara Vey's Meridian operation, archive carrier, authored sites, adaptive opposition, and first fixed tick initialized."
print "Evidence log: $log"
