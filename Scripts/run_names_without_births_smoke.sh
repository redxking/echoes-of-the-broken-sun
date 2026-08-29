#!/bin/zsh
set -euo pipefail

project_root="${0:A:h:h}"
ue_root="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
editor="$ue_root/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
project="$project_root/EchoesOfTheBrokenSun.uproject"
artifact_dir="$project_root/BuildArtifacts/NamesWithoutBirths"
ledger="$artifact_dir/FixtureCampaignProgress.bin"
log="${ECHOES_NAMES_WITHOUT_BIRTHS_LOG:-$artifact_dir/NamesWithoutBirthsSmoke.log}"
if [[ "$log" != /* ]]; then
  log="$project_root/$log"
fi

if [[ ! -x "$editor" ]]; then
  print -u2 "Unreal Editor is not available at: $editor"
  exit 2
fi

mkdir -p "$artifact_dir"
python3 "$project_root/Scripts/create_campaign_fixture.py" \
  "$ledger" --choice Preserve --through-mission 5
: > "$log"

"$editor" "$project" /Engine/Maps/Entry \
  -game -unattended -nop4 -nosplash -nullrhi -nosound -Multiprocess \
  -EchoesCampaignNamesWithoutBirths -EchoesAutoStart \
  -EchoesCampaignProgressPath="$ledger" \
  -benchmark -fps=20 -benchmarkseconds=3 -AbsLog="$log"

required_markers=(
  '\[ECHOES_CAMPAIGN_LEDGER_LOAD\] available=true records=5'
  '\[ECHOES_OPERATION_REQUESTED\] operation=NamesWithoutBirths accepted=true'
  '\[ECHOES_FACTION_SCENARIO_READY\] local=MeridianCompact opposition=KharuunAssemblies selectable=true'
  '\[ECHOES_NAMES_WITHOUT_BIRTHS_READY\] branch=missing_quarter talar=[1-9][0-9]* archive=[1-9][0-9]* civilianA=[1-9][0-9]* civilianB=[1-9][0-9]* census=\(32,22\) shelter=\(32,48\) extraction=\(32,44\) pressureProxies=3 pressureFaction=KharuunAssemblies pressureBehavior=genericAdaptive inheritedRecords=5'
  '\[ECHOES_BOOT_READY\]'
  '\[ECHOES_SIM_FIRST_TICK\]'
)
for marker in "${required_markers[@]}"; do
  if ! /usr/bin/grep -Eq "$marker" "$log"; then
    print -u2 "Names Without Births runtime marker was absent: $marker"
    print -u2 "Inspect: $log"
    exit 3
  fi
done

if /usr/bin/grep -Eq '\[ECHOES_OPERATION_REQUEST_REJECTED\]|\[ECHOES_NAMES_WITHOUT_BIRTHS_LOCKED\]|\[ECHOES_NAMES_WITHOUT_BIRTHS_INIT_FAILED\]|\[ECHOES_BOOT_INCOMPLETE\]|Fatal error:|Assertion failed:' "$log"; then
  print -u2 "Names Without Births runtime reported an initialization or fatal failure. Inspect: $log"
  exit 4
fi

print "Names Without Births runtime passed: five-record Preserve ledger, Meridian-authoritative protected proxies, inherited missing-quarter census geometry, generic unresolved pressure, and first fixed tick initialized."
print "Evidence log: $log"
