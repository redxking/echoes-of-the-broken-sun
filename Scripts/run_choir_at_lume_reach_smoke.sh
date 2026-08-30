#!/bin/zsh
set -euo pipefail

project_root="${0:A:h:h}"
ue_root="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
editor="$ue_root/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
project="$project_root/EchoesOfTheBrokenSun.uproject"
artifact_dir="$project_root/BuildArtifacts/ChoirAtLumeReach"
ledger="$artifact_dir/FixtureCampaignProgress.bin"
log="${ECHOES_CHOIR_AT_LUME_REACH_LOG:-$artifact_dir/ChoirAtLumeReachSmoke.log}"
if [[ "$log" != /* ]]; then
  log="$project_root/$log"
fi

if [[ ! -x "$editor" ]]; then
  print -u2 "Unreal Editor is not available at: $editor"
  exit 2
fi

mkdir -p "$artifact_dir"
python3 "$project_root/Scripts/create_campaign_fixture.py" \
  "$ledger" --choice Preserve --through-mission 9
: > "$log"

"$editor" "$project" /Engine/Maps/Entry \
  -game -unattended -nop4 -nosplash -nullrhi -nosound -Multiprocess \
  -EchoesCampaignChoirAtLumeReach -EchoesAutoStart \
  -EchoesCampaignProgressPath="$ledger" \
  -benchmark -fps=20 -benchmarkseconds=3 -AbsLog="$log"

required_markers=(
  '\[ECHOES_CAMPAIGN_LEDGER_LOAD\] available=true records=9'
  '\[ECHOES_OPERATION_REQUESTED\] operation=ChoirAtLumeReach accepted=true'
  '\[ECHOES_LUME_REACH_TERRAIN_READY\] blocked=223 publicGates=3 well=\(32,43\) inheritedBranch=2'
  '\[ECHOES_FACTION_SCENARIO_READY\] local=KharuunAssemblies opposition=MeridianCompact selectable=true'
  '\[ECHOES_CHOIR_AT_LUME_REACH_READY\] approach=held_vault_approach priorBranch=2 deferredDistrict=2 oruun=[1-9][0-9]* waystone=[1-9][0-9]* well=[1-9][0-9]* contact=\(32,20\) liability=\(46,34\) anchors=\(28,39\):\(36,39\) wellSite=\(32,43\) localFaction=KharuunAssemblies maraPresence=liaisonOnly choirPresence=nonPlayablePublicContact opposition=meridianMechanicalQuarantineProxies maraInvolvementUnmodeled=true compactWideActionUnproven=true mixedFactionCommand=false hiddenAttribution=false inheritedRecords=9 blocked=223'
  '\[ECHOES_BOOT_READY\]'
  '\[ECHOES_SIM_FIRST_TICK\]'
)
for marker in "${required_markers[@]}"; do
  if ! /usr/bin/grep -Eq "$marker" "$log"; then
    print -u2 "The Choir at Lume Reach runtime marker was absent: $marker"
    print -u2 "Inspect: $log"
    exit 3
  fi
done

if /usr/bin/grep -Eq '\[ECHOES_OPERATION_REQUEST_REJECTED\]|\[ECHOES_CHOIR_AT_LUME_REACH_LOCKED\]|\[ECHOES_CHOIR_AT_LUME_REACH_INIT_FAILED\]|\[ECHOES_BOOT_INCOMPLETE\]|Fatal error:|Assertion failed:' "$log"; then
  print -u2 "The Choir at Lume Reach runtime reported an initialization or fatal failure. Inspect: $log"
  exit 4
fi

print "The Choir at Lume Reach startup smoke passed: nine-record Preserve ledger, dedicated 223-tile Lume topology, Kharuun authority, public contact/anchor/Well sites, and explicit liaison/non-playable-Choir boundaries initialized through the first fixed tick."
print "Evidence log: $log"
