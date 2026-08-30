#!/bin/zsh
set -euo pipefail

project_root="${0:A:h:h}"
ue_root="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
editor="$ue_root/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
project="$project_root/EchoesOfTheBrokenSun.uproject"
artifact_dir="$project_root/BuildArtifacts/AssemblyOfTheMissing"
ledger="$artifact_dir/FixtureCampaignProgress.bin"
log="${ECHOES_ASSEMBLY_OF_THE_MISSING_LOG:-$artifact_dir/AssemblyOfTheMissingSmoke.log}"
if [[ "$log" != /* ]]; then
  log="$project_root/$log"
fi

if [[ ! -x "$editor" ]]; then
  print -u2 "Unreal Editor is not available at: $editor"
  exit 2
fi

mkdir -p "$artifact_dir"
python3 "$project_root/Scripts/create_campaign_fixture.py" \
  "$ledger" --choice Harvest --lume-choice Preserve --through-mission 12
: > "$log"

"$editor" "$project" /Engine/Maps/Entry \
  -game -unattended -nop4 -nosplash -nullrhi -nosound -Multiprocess \
  -EchoesCampaignAssemblyOfTheMissing -EchoesAutoStart \
  -EchoesCampaignProgressPath="$ledger" \
  -benchmark -fps=20 -benchmarkseconds=3 -AbsLog="$log"

required_markers=(
  '\[ECHOES_CAMPAIGN_LEDGER_LOAD\] available=true records=12'
  '\[ECHOES_OPERATION_REQUESTED\] operation=AssemblyOfTheMissing accepted=true'
  '\[ECHOES_ASSEMBLY_OF_THE_MISSING_SPAWN\] planKey=7 oruun=[1-9][0-9]* verifier=[1-9][0-9]* publicRecords=[1-9][0-9]*:[1-9][0-9]* crownfallIndex=[1-9][0-9]* protocol=2 publicInterfacesNeutral=true success=true'
  '\[ECHOES_CROWNFALL_PUBLIC_INDEX_TERRAIN_READY\] blocked=223 publicGates=3 index=\(32,56\) inheritedBranch=1 planKey=7'
  '\[ECHOES_FACTION_SCENARIO_READY\] local=KharuunAssemblies opposition=MeridianCompact selectable=true'
  '\[ECHOES_ASSEMBLY_OF_THE_MISSING_READY\] planKey=7 founding=1 route=ash_cut districtA=LIFE SUPPORT districtB=TRANSIT deferred=ARCHIVE CONTINUITY recordedProtocol=2 protocol=preserve_hold oruun=[1-9][0-9]* verifier=[1-9][0-9]* publicRecords=[1-9][0-9]*:[1-9][0-9]* crownfallIndex=[1-9][0-9]* inheritedRecords=12 localFaction=KharuunAssemblies meridianPresence=neutralPublicRecordInterfaceOnly kharuunPublicRecord=neutralInterface crownfallIndex=neutralPublicInterface mixedFactionCommand=false responsibilityUnassigned=true hiddenAuthorshipUnproven=true trustUnproven=true consentUnproven=true civilianStateUnmodeled=true blocked=223'
  '\[ECHOES_BOOT_READY\]'
  '\[ECHOES_SIM_FIRST_TICK\]'
)
for marker in "${required_markers[@]}"; do
  if ! /usr/bin/grep -Eq "$marker" "$log"; then
    print -u2 "Assembly of the Missing runtime marker was absent: $marker"
    print -u2 "Inspect: $log"
    exit 3
  fi
done

if /usr/bin/grep -Eq '\[ECHOES_OPERATION_REQUEST_REJECTED\]|\[ECHOES_ASSEMBLY_OF_THE_MISSING_LOCKED\]|\[ECHOES_ASSEMBLY_OF_THE_MISSING_INIT_FAILED\]|\[ECHOES_BOOT_INCOMPLETE\]|Fatal error:|Assertion failed:' "$log"; then
  print -u2 "Assembly of the Missing reported an initialization or fatal failure. Inspect: $log"
  exit 4
fi

print "Assembly of the Missing startup smoke passed: exact twelve-record ledger, Harvest founding and Preserve Lume protocol, plan 07, dedicated 223-tile topology, Kharuun-only command authority, three neutral public interfaces, and bounded public-assembly claims initialized through the first fixed tick."
print "Evidence log: $log"
