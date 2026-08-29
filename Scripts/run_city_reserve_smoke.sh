#!/bin/zsh
set -euo pipefail

project_root="${0:A:h:h}"
ue_root="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
editor="$ue_root/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
project="$project_root/EchoesOfTheBrokenSun.uproject"
artifact_dir="$project_root/BuildArtifacts/CityReserve"
ledger="$artifact_dir/FixtureCampaignProgress.bin"
log="${ECHOES_CITY_RESERVE_LOG:-$artifact_dir/CityReserveSmoke.log}"
if [[ "$log" != /* ]]; then
  log="$project_root/$log"
fi

if [[ ! -x "$editor" ]]; then
  print -u2 "Unreal Editor is not available at: $editor"
  exit 2
fi

mkdir -p "$artifact_dir"
python3 "$project_root/Scripts/create_campaign_fixture.py" \
  "$ledger" --choice Preserve --through-mission 2
: > "$log"

"$editor" "$project" /Engine/Maps/Entry \
  -game -unattended -nop4 -nosplash -nullrhi -nosound \
  -EchoesCampaignCityReserve -EchoesAutoStart \
  -EchoesCampaignProgressPath="$ledger" \
  -benchmark -fps=20 -benchmarkseconds=3 -AbsLog="$log"

required_markers=(
  '\[ECHOES_CAMPAIGN_LEDGER_LOAD\] available=true records=2'
  '\[ECHOES_OPERATION_REQUESTED\] operation=ACityOnReserve accepted=true'
  '\[ECHOES_FACTION_SCENARIO_READY\] local=MeridianCompact opposition=KharuunAssemblies selectable=true'
  '\[ECHOES_CITY_RESERVE_READY\] branch=continuity_reserve priority=ARCHIVE CONTINUITY secondary=LIFE SUPPORT final=TRANSIT life=[1-9][0-9]* transit=[1-9][0-9]* archive=[1-9][0-9]* powered=0 inheritedRecords=2'
  '\[ECHOES_BOOT_READY\]'
  '\[ECHOES_SIM_FIRST_TICK\]'
)
for marker in "${required_markers[@]}"; do
  if ! /usr/bin/grep -Eq "$marker" "$log"; then
    print -u2 "City on Reserve runtime marker was absent: $marker"
    print -u2 "Inspect: $log"
    exit 3
  fi
done

if /usr/bin/grep -Eq '\[ECHOES_OPERATION_REQUEST_REJECTED\]|\[ECHOES_CITY_RESERVE_LOCKED\]|\[ECHOES_CITY_RESERVE_INIT_FAILED\]|\[ECHOES_BOOT_INCOMPLETE\]|Fatal error:|Assertion failed:' "$log"; then
  print -u2 "City on Reserve runtime reported an initialization or fatal failure. Inspect: $log"
  exit 4
fi

print "City on Reserve runtime passed: two-record Preserve ledger, Mara's Meridian force, three disconnected district posts, inherited priority, and first fixed tick initialized."
print "Evidence log: $log"
