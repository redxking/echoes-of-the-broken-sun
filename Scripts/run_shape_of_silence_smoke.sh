#!/bin/zsh
set -euo pipefail

project_root="${0:A:h:h}"
ue_root="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
editor="$ue_root/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
project="$project_root/EchoesOfTheBrokenSun.uproject"
artifact_dir="$project_root/BuildArtifacts/ShapeOfSilence"
ledger="$artifact_dir/FixtureCampaignProgress.bin"
log="${ECHOES_SHAPE_OF_SILENCE_LOG:-$artifact_dir/ShapeOfSilenceSmoke.log}"
if [[ "$log" != /* ]]; then
  log="$project_root/$log"
fi

if [[ ! -x "$editor" ]]; then
  print -u2 "Unreal Editor is not available at: $editor"
  exit 2
fi

mkdir -p "$artifact_dir"
python3 "$project_root/Scripts/create_campaign_fixture.py" \
  "$ledger" --choice Preserve --through-mission 6
: > "$log"

"$editor" "$project" /Engine/Maps/Entry \
  -game -unattended -nop4 -nosplash -nullrhi -nosound -Multiprocess \
  -EchoesCampaignShapeOfSilence -EchoesAutoStart \
  -EchoesCampaignProgressPath="$ledger" \
  -benchmark -fps=20 -benchmarkseconds=3 -AbsLog="$log"

required_markers=(
  '\[ECHOES_CAMPAIGN_LEDGER_LOAD\] available=true records=6'
  '\[ECHOES_OPERATION_REQUESTED\] operation=TheShapeOfSilence accepted=true'
  '\[ECHOES_FACTION_SCENARIO_READY\] local=KharuunAssemblies opposition=MeridianCompact selectable=true'
  '\[ECHOES_SHAPE_OF_SILENCE_READY\] branch=held_hollow oruun=[1-9][0-9]* witnessA=[1-9][0-9]* witnessB=[1-9][0-9]* waystone=[1-9][0-9]* anchor=\(32,28\) spine=\(32,38\) witnessSites=\(28,45\):\(36,45\) confluence=\(32,50\) observedCorrespondenceOnly=true hiddenAttribution=false inheritedRecords=6'
  '\[ECHOES_BOOT_READY\]'
  '\[ECHOES_SIM_FIRST_TICK\]'
)
for marker in "${required_markers[@]}"; do
  if ! /usr/bin/grep -Eq "$marker" "$log"; then
    print -u2 "The Shape of Silence runtime marker was absent: $marker"
    print -u2 "Inspect: $log"
    exit 3
  fi
done

if /usr/bin/grep -Eq '\[ECHOES_OPERATION_REQUEST_REJECTED\]|\[ECHOES_SHAPE_OF_SILENCE_LOCKED\]|\[ECHOES_SHAPE_OF_SILENCE_INIT_FAILED\]|\[ECHOES_BOOT_INCOMPLETE\]|Fatal error:|Assertion failed:' "$log"; then
  print -u2 "The Shape of Silence runtime reported an initialization or fatal failure. Inspect: $log"
  exit 4
fi

print "The Shape of Silence runtime passed: six-record Preserve ledger, Kharuun-authoritative witnesses, inherited held-hollow geometry, explicit correspondence-only boundary, and first fixed tick initialized."
print "Evidence log: $log"
