#!/bin/zsh
set -euo pipefail

project_root="${0:A:h:h}"
ue_root="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
editor="$ue_root/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
project="$project_root/EchoesOfTheBrokenSun.uproject"
local_faction="${ECHOES_LOCAL_FACTION:-Meridian}"
case "$local_faction" in
  Meridian|MeridianCompact)
    faction_args=()
    expected_technology="MeridianPrismaticTargeting"
    ;;
  Kharuun|KharuunAssemblies)
    faction_args=(-EchoesFaction=Kharuun)
    expected_technology="KharuunEchoCartography"
    ;;
  *)
    print -u2 "Unsupported ECHOES_LOCAL_FACTION: $local_faction"
    exit 2
    ;;
esac
log="${ECHOES_RESEARCH_INTERRUPTION_LOG:-$project_root/BuildArtifacts/ResearchInterruptionSmoke.log}"
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
  -EchoesResearchInterruptionPresentation "${faction_args[@]}" \
  -benchmark -fps=20 -benchmarkseconds=12 -AbsLog="$log"

if ! /usr/bin/grep -q '\[ECHOES_CONTENT_READY\]' "$log" ||
   ! /usr/bin/grep -q '\[ECHOES_SIM_RULES_READY\]' "$log" ||
   ! /usr/bin/grep -q "\[ECHOES_RESEARCH_INTERRUPTION_PRESENTATION_READY\] technology=$expected_technology attackers=32 attackTick=60 controlled=true release=false" "$log" ||
   ! /usr/bin/grep -Eq "\[ECHOES_RESEARCH_PRESENTATION_ACTIVE\] technology=$expected_technology progress=[0-9]+ required=[1-9][0-9]* controlled=true" "$log" ||
   ! /usr/bin/grep -q "\[ECHOES_RESEARCH_PRESENTATION_INTERRUPTED\] technology=$expected_technology producerDestroyed=true costsRefunded=false controlled=true" "$log" ||
   ! /usr/bin/grep -q '\[ECHOES_RESEARCH_INTERRUPTION_PANEL_READY\] visible=true paused=true controlled=true release=false' "$log" ||
   ! /usr/bin/grep -q '\[ECHOES_BOOT_READY\]' "$log"; then
  print -u2 "Controlled research interruption markers were incomplete. Inspect: $log"
  exit 3
fi

if /usr/bin/grep -Eq '\[ECHOES_RESEARCH_PRESENTATION_COMPLETE\]|\[ECHOES_RESEARCH_PRESENTATION_FAILED\]|\[ECHOES_BOOT_INCOMPLETE\]|\[ECHOES_CONTENT_FAILED\]|\[ECHOES_SIM_CONTENT_REJECTED\]|\[ECHOES_SIM_VIEW_SYNC_FAILED\]|Fatal error:|Assertion failed:' "$log"; then
  print -u2 "Controlled research interruption reported an invalid completion, boot, or fatal state. Inspect: $log"
  exit 4
fi

print "Controlled research interruption passed for $local_faction: active progress was cancelled by producer destruction without refund."
print "Evidence log: $log"
