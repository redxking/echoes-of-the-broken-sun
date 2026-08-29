#!/bin/zsh
set -euo pipefail

project_root="${0:A:h:h}"
ue_root="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
editor="$ue_root/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
project="$project_root/EchoesOfTheBrokenSun.uproject"
log="${ECHOES_KHARUUN_SYSTEMS_LOG:-$project_root/BuildArtifacts/KharuunSystemsSmoke.log}"
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
  -EchoesFaction=Kharuun -EchoesKharuunSystemsPresentation \
  -benchmark -fps=20 -benchmarkseconds=4 -AbsLog="$log"

if ! /usr/bin/grep -q '\[ECHOES_CONTENT_READY\]' "$log" ||
   ! /usr/bin/grep -q '\[ECHOES_SIM_RULES_READY\]' "$log" ||
   ! /usr/bin/grep -q '\[ECHOES_FACTION_SCENARIO_READY\] local=KharuunAssemblies opposition=MeridianCompact selectable=true' "$log" ||
   ! /usr/bin/grep -q '\[ECHOES_KHARUUN_SYSTEMS_PRESENTATION_READY\] commands=3 hiddenMovers=1 controlled=true release=false' "$log" ||
   ! /usr/bin/grep -Eq '\[ECHOES_KHARUUN_SYSTEMS_PRESENTATION_ACTIVE\] waystone=uprooting warform=carapace_molt cover=active vibrationContacts=[1-9][0-9]* anonymous=true hiddenSourceDisclosed=false paused=true controlled=true release=false' "$log" ||
   ! /usr/bin/grep -q '\[ECHOES_BOOT_READY\]' "$log"; then
  print -u2 "Controlled Kharuun systems markers were incomplete. Inspect: $log"
  exit 3
fi

if /usr/bin/grep -Eq '\[ECHOES_KHARUUN_SYSTEMS_PRESENTATION_FAILED\]|\[ECHOES_PRESENTATION_MODE_FAILED\]|\[ECHOES_BOOT_INCOMPLETE\]|\[ECHOES_CONTENT_FAILED\]|\[ECHOES_SIM_CONTENT_REJECTED\]|\[ECHOES_SIM_VIEW_SYNC_FAILED\]|Fatal error:|Assertion failed:' "$log"; then
  print -u2 "Controlled Kharuun systems presentation reported a boot or fatal failure. Inspect: $log"
  exit 4
fi

print "Controlled Kharuun systems presentation passed: migration, adaptation, mineral cover, and anonymous vibration detection were active together."
print "Evidence log: $log"
