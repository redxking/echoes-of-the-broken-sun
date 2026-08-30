#!/bin/zsh
set -euo pipefail

project_root="${0:A:h:h}"
ue_root="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
editor="$ue_root/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
project="$project_root/EchoesOfTheBrokenSun.uproject"
artifact_dir="$project_root/BuildArtifacts/BrokenSun"
ledger="$artifact_dir/FixtureCampaignProgress.bin"
log="${ECHOES_BROKEN_SUN_LOG:-$artifact_dir/BrokenSunSmoke.log}"
if [[ "$log" != /* ]]; then
  log="$project_root/$log"
fi

if [[ ! -x "$editor" ]]; then
  print -u2 "Unreal Editor is not available at: $editor"
  exit 2
fi

mkdir -p "$artifact_dir"
python3 "$project_root/Scripts/create_campaign_fixture.py" \
  "$ledger" --choice Harvest --lume-choice Preserve --through-mission 14
: > "$log"

"$editor" "$project" /Engine/Maps/Entry \
  -game -unattended -nop4 -nosplash -nullrhi -nosound -Multiprocess \
  -EchoesCampaignTheBrokenSun -EchoesAutoStart \
  -EchoesCampaignProgressPath="$ledger" \
  -benchmark -fps=20 -benchmarkseconds=3 -AbsLog="$log"

required_markers=(
  '\[ECHOES_CAMPAIGN_LEDGER_LOAD\] available=true records=14'
  '\[ECHOES_OPERATION_REQUESTED\] operation=TheBrokenSun accepted=true'
  '\[ECHOES_BROKEN_SUN_WITNESSES_SPAWNED\] mara=[1-9][0-9]* oruun=[1-9][0-9]* talar=[1-9][0-9]* neme=[1-9][0-9]* neutralWitnesses=true mixedFactionCommand=false success=true'
  '\[ECHOES_FACTION_SCENARIO_READY\] local=HollowChoir opposition=MeridianCompact selectable=true'
  '\[ECHOES_BROKEN_SUN_READY\] planKey=7 founding=1 route=ash_cut recordedProtocol=2 protocol=preserve_hold availability=0x07 voice=[1-9][0-9]* heavy=[1-9][0-9]* neme=[1-9][0-9]* worker=[1-9][0-9]* mara=[1-9][0-9]* oruun=[1-9][0-9]* talar=[1-9][0-9]* approach=\(32,56\) maraSite=\(18,35\) oruunSite=\(18,30\) nemeSite=\(38,43\) talarSite=\(26,43\) restoration=\(32,49\) controlled=\(32,44\) extinguishment=\(26,49\) evolution=\(38,49\) baseHoldTicks=240 inheritedRecords=14 localFaction=HollowChoir localAuthority=HollowChoir namedWitnesses=protectedNeutral mixedFactionCommand=false explicitEndingEligibility=true hiddenMoralityScore=false oldLedgerMigration=true oldCheckpointCompatibility=false broadConsequencesUnmodeled=true campaignBalanceUnproven=true ordinaryHumanCompletionUnproven=true releaseReadinessUnproven=true blocked=223'
  '\[ECHOES_BOOT_READY\]'
  '\[ECHOES_SIM_FIRST_TICK\]'
)
for marker in "${required_markers[@]}"; do
  if ! /usr/bin/grep -Eq "$marker" "$log"; then
    print -u2 "The Broken Sun runtime marker was absent: $marker"
    print -u2 "Inspect: $log"
    exit 3
  fi
done

if /usr/bin/grep -Eq '\[ECHOES_OPERATION_REQUEST_REJECTED\]|\[ECHOES_BROKEN_SUN_LOCKED\]|\[ECHOES_THE_BROKEN_SUN_INIT_FAILED\]|\[ECHOES_BROKEN_SUN_CONTRACT_FAILED\]|\[ECHOES_BOOT_INCOMPLETE\]|Fatal error:|Assertion failed:' "$log"; then
  print -u2 "The Broken Sun reported an initialization, contract, or fatal failure. Inspect: $log"
  exit 4
fi

print "The Broken Sun startup smoke passed: the migrated fourteen-record ledger admitted final plan 07, Hollow Choir command authority, three protected neutral witnesses, three explicit earned endings, distinct approach/convergence geometry, and bounded claim language through the first fixed tick."
print "Evidence log: $log"
