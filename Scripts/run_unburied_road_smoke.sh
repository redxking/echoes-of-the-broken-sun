#!/bin/zsh
set -euo pipefail

project_root="${0:A:h:h}"
ue_root="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
editor="$ue_root/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
project="$project_root/EchoesOfTheBrokenSun.uproject"
artifact_dir="$project_root/BuildArtifacts/UnburiedRoad"
ledger="$artifact_dir/FixtureCampaignProgress.bin"
log="${ECHOES_UNBURIED_ROAD_LOG:-$artifact_dir/UnburiedRoadSmoke.log}"
if [[ "$log" != /* ]]; then
  log="$project_root/$log"
fi

if [[ ! -x "$editor" ]]; then
  print -u2 "Unreal Editor is not available at: $editor"
  exit 2
fi

mkdir -p "$artifact_dir"
python3 "$project_root/Scripts/create_campaign_fixture.py" \
  "$ledger" --choice Preserve --through-mission 3
: > "$log"

"$editor" "$project" /Engine/Maps/Entry \
  -game -unattended -nop4 -nosplash -nullrhi -nosound \
  -EchoesCampaignUnburiedRoad -EchoesAutoStart \
  -EchoesCampaignProgressPath="$ledger" \
  -benchmark -fps=20 -benchmarkseconds=3 -AbsLog="$log"

required_markers=(
  '\[ECHOES_CAMPAIGN_LEDGER_LOAD\] available=true records=3'
  '\[ECHOES_OPERATION_REQUESTED\] operation=TheUnburiedRoad accepted=true'
  '\[ECHOES_FACTION_SCENARIO_READY\] local=KharuunAssemblies opposition=MeridianCompact selectable=true'
  '\[ECHOES_UNBURIED_ROAD_READY\] branch=buried_causeway waystone=[1-9][0-9]* bearer=[1-9][0-9]* roadhead=\(32,28\) listeningSpine=\(32,37\) shard=\(38,43\) terrainDelta=40 blocked=205 inheritedRecords=3'
  '\[ECHOES_BOOT_READY\]'
  '\[ECHOES_SIM_FIRST_TICK\]'
)
for marker in "${required_markers[@]}"; do
  if ! /usr/bin/grep -Eq "$marker" "$log"; then
    print -u2 "The Unburied Road runtime marker was absent: $marker"
    print -u2 "Inspect: $log"
    exit 3
  fi
done

if /usr/bin/grep -Eq '\[ECHOES_OPERATION_REQUEST_REJECTED\]|\[ECHOES_UNBURIED_ROAD_LOCKED\]|\[ECHOES_UNBURIED_ROAD_INIT_FAILED\]|\[ECHOES_BOOT_INCOMPLETE\]|Fatal error:|Assertion failed:' "$log"; then
  print -u2 "The Unburied Road runtime reported an initialization or fatal failure. Inspect: $log"
  exit 4
fi

print "The Unburied Road runtime passed: three-record Preserve ledger, Oruun's Kharuun force, inherited buried causeway, mobile Waystone, and first fixed tick initialized."
print "Evidence log: $log"
