#!/bin/zsh
set -euo pipefail

project_root="${0:A:h:h}"
ue_root="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
editor="$ue_root/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
project="$project_root/EchoesOfTheBrokenSun.uproject"
artifact_dir="$project_root/BuildArtifacts/SeveralVoicesOneCommand"
ledger="$artifact_dir/FixtureCampaignProgress.bin"
log="${ECHOES_SEVERAL_VOICES_ONE_COMMAND_LOG:-$artifact_dir/SeveralVoicesOneCommandSmoke.log}"
if [[ "$log" != /* ]]; then
  log="$project_root/$log"
fi

if [[ ! -x "$editor" ]]; then
  print -u2 "Unreal Editor is not available at: $editor"
  exit 2
fi

mkdir -p "$artifact_dir"
python3 "$project_root/Scripts/create_campaign_fixture.py" \
  "$ledger" --choice Harvest --lume-choice Preserve --through-mission 13
: > "$log"

"$editor" "$project" /Engine/Maps/Entry \
  -game -unattended -nop4 -nosplash -nullrhi -nosound -Multiprocess \
  -EchoesCampaignSeveralVoicesOneCommand -EchoesAutoStart \
  -EchoesCampaignProgressPath="$ledger" \
  -benchmark -fps=20 -benchmarkseconds=3 -AbsLog="$log"

required_markers=(
  '\[ECHOES_CAMPAIGN_LEDGER_LOAD\] available=true records=13'
  '\[ECHOES_OPERATION_REQUESTED\] operation=SeveralVoicesOneCommand accepted=true'
  '\[ECHOES_SEVERAL_VOICES_ONE_COMMAND_SPAWN\] planKey=7 possibleVoice=[1-9][0-9]* manifestVoice=[1-9][0-9]* neme=[1-9][0-9]* researchLoom=[1-9][0-9]* possibleSite=\(18,35\) manifestSite=\(18,30\) nemeSite=\(38,43\) anchorSite=\(32,56\) protocol=2 localAuthority=HollowChoir incompatibleStates=true success=true'
  '\[ECHOES_CHOIR_CRISIS_TERRAIN_READY\] blocked=223 publicGates=3 possible=\(18,35\) manifest=\(18,30\) neme=\(38,43\) anchor=\(32,56\) inheritedBranch=1 planKey=7'
  '\[ECHOES_FACTION_SCENARIO_READY\] local=HollowChoir opposition=MeridianCompact selectable=true'
  '\[ECHOES_SEVERAL_VOICES_ONE_COMMAND_READY\] planKey=7 founding=1 route=ash_cut recordedProtocol=2 protocol=preserve_hold possibleVoice=[1-9][0-9]* manifestVoice=[1-9][0-9]* neme=[1-9][0-9]* researchLoom=[1-9][0-9]* possibleSite=\(18,35\) manifestSite=\(18,30\) nemeSite=\(38,43\) anchorSite=\(32,56\) identityResolveTicks=160 crisisHoldTicks=160 inheritedRecords=13 localFaction=HollowChoir localAuthority=HollowChoir incompatibleStates=true visibleTimers=true finalChoirFateDecided=false campaignBalanceUnproven=true blocked=223'
  '\[ECHOES_BOOT_READY\]'
  '\[ECHOES_SIM_FIRST_TICK\]'
)
for marker in "${required_markers[@]}"; do
  if ! /usr/bin/grep -Eq "$marker" "$log"; then
    print -u2 "Several Voices, One Command runtime marker was absent: $marker"
    print -u2 "Inspect: $log"
    exit 3
  fi
done

if /usr/bin/grep -Eq '\[ECHOES_OPERATION_REQUEST_REJECTED\]|\[ECHOES_SEVERAL_VOICES_ONE_COMMAND_LOCKED\]|\[ECHOES_SEVERAL_VOICES_ONE_COMMAND_INIT_FAILED\]|\[ECHOES_BOOT_INCOMPLETE\]|Fatal error:|Assertion failed:' "$log"; then
  print -u2 "Several Voices, One Command reported an initialization or fatal failure. Inspect: $log"
  exit 4
fi

print "Several Voices, One Command startup smoke passed: exact thirteen-record ledger, Harvest founding and Preserve Lume protocol, plan 07, dedicated 223-tile crisis topology, Hollow Choir command authority, protected Possible/Manifest/Neme roles, visible timer contract, and bounded final-fate claims initialized through the first fixed tick."
print "Evidence log: $log"
