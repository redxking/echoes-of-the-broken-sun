#!/bin/zsh
set -euo pipefail

project_root="${0:A:h:h}"
ue_root="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
editor="$ue_root/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
project="$project_root/EchoesOfTheBrokenSun.uproject"
artifact_dir="$project_root/BuildArtifacts/NoNeutralLedger"
ledger="$artifact_dir/FixtureCampaignProgress.bin"
log="${ECHOES_NO_NEUTRAL_LEDGER_LOG:-$artifact_dir/NoNeutralLedgerSmoke.log}"
if [[ "$log" != /* ]]; then
  log="$project_root/$log"
fi

if [[ ! -x "$editor" ]]; then
  print -u2 "Unreal Editor is not available at: $editor"
  exit 2
fi

mkdir -p "$artifact_dir"
python3 "$project_root/Scripts/create_campaign_fixture.py" \
  "$ledger" --choice Harvest --lume-choice Preserve --through-mission 10
: > "$log"

"$editor" "$project" /Engine/Maps/Entry \
  -game -unattended -nop4 -nosplash -nullrhi -nosound -Multiprocess \
  -EchoesCampaignNoNeutralLedger -EchoesAutoStart \
  -EchoesCampaignProgressPath="$ledger" \
  -benchmark -fps=20 -benchmarkseconds=3 -AbsLog="$log"

required_markers=(
  '\[ECHOES_CAMPAIGN_LEDGER_LOAD\] available=true records=10'
  '\[ECHOES_OPERATION_REQUESTED\] operation=NoNeutralLedger accepted=true'
  '\[ECHOES_LUME_CONCORDANCE_TERRAIN_READY\] blocked=223 publicGates=3 well=\(32,49\) inheritedBranch=1 planKey=7'
  '\[ECHOES_FACTION_SCENARIO_READY\] local=KharuunAssemblies opposition=MeridianCompact selectable=true'
  '\[ECHOES_NO_NEUTRAL_LEDGER_READY\] planKey=7 founding=1 route=ash_cut districtA=LIFE SUPPORT districtB=TRANSIT deferred=ARCHIVE CONTINUITY lumeProtocol=2 protocol=preserve_hold oruun=[1-9][0-9]* waystone=[1-9][0-9]* witness=[1-9][0-9]* districtInterfaces=[1-9][0-9]*:[1-9][0-9]* evidenceInterfaces=[1-9][0-9]*:[1-9][0-9]* well=[1-9][0-9]* inheritedRecords=10 localFaction=KharuunAssemblies meridianPresence=neutralPoweredPublicInterfacesOnly choirPresence=nonPlayablePublicContact mixedFactionCommand=false hiddenTrust=false survivorVarianceUnmodeled=true proxyAttribution=false blocked=223'
  '\[ECHOES_BOOT_READY\]'
  '\[ECHOES_SIM_FIRST_TICK\]'
)
for marker in "${required_markers[@]}"; do
  if ! /usr/bin/grep -Eq "$marker" "$log"; then
    print -u2 "The No Neutral Ledger runtime marker was absent: $marker"
    print -u2 "Inspect: $log"
    exit 3
  fi
done

if /usr/bin/grep -Eq '\[ECHOES_OPERATION_REQUEST_REJECTED\]|\[ECHOES_NO_NEUTRAL_LEDGER_LOCKED\]|\[ECHOES_NO_NEUTRAL_LEDGER_INIT_FAILED\]|\[ECHOES_BOOT_INCOMPLETE\]|Fatal error:|Assertion failed:' "$log"; then
  print -u2 "No Neutral Ledger reported an initialization or fatal failure. Inspect: $log"
  exit 4
fi

print "No Neutral Ledger startup smoke passed: exact ten-record ledger, independent Harvest founding and Preserve Lume choices, plan 07, dedicated 223-tile topology, Kharuun authority, and public non-commandable contribution boundaries initialized through the first fixed tick."
print "Evidence log: $log"
