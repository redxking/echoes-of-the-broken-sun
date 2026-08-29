#!/bin/zsh
set -euo pipefail

project_root="${0:A:h:h}"
ue_root="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
editor="$ue_root/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
project="$project_root/EchoesOfTheBrokenSun.uproject"
artifact_dir="$project_root/BuildArtifacts/TermsOfContinuance"
ledger="$artifact_dir/FixtureCampaignProgress.bin"
log="${ECHOES_TERMS_OF_CONTINUANCE_LOG:-$artifact_dir/TermsOfContinuanceSmoke.log}"
if [[ "$log" != /* ]]; then
  log="$project_root/$log"
fi

if [[ ! -x "$editor" ]]; then
  print -u2 "Unreal Editor is not available at: $editor"
  exit 2
fi

mkdir -p "$artifact_dir"
python3 "$project_root/Scripts/create_campaign_fixture.py" \
  "$ledger" --choice Preserve --through-mission 4
: > "$log"

"$editor" "$project" /Engine/Maps/Entry \
  -game -unattended -nop4 -nosplash -nullrhi -nosound \
  -EchoesCampaignTermsOfContinuance -EchoesAutoStart \
  -EchoesCampaignProgressPath="$ledger" \
  -benchmark -fps=20 -benchmarkseconds=3 -AbsLog="$log"

required_markers=(
  '\[ECHOES_CAMPAIGN_LEDGER_LOAD\] available=true records=4'
  '\[ECHOES_OPERATION_REQUESTED\] operation=TermsOfContinuance accepted=true'
  '\[ECHOES_FACTION_SCENARIO_READY\] local=MeridianCompact opposition=KharuunAssemblies selectable=true'
  '\[ECHOES_TERMS_OF_CONTINUANCE_READY\] branch=witness_clause meridianRelay=[1-9][0-9]* kharuunSpine=[1-9][0-9]* meridianWitness=[1-9][0-9]* kharuunWitness=[1-9][0-9]* relay=\(32,27\) spine=\(32,39\) extraction=\(32,47\) holdUntil=900 apparentAttackers=2 terrainDelta=40 blocked=205 inheritedRecords=4'
  '\[ECHOES_BOOT_READY\]'
  '\[ECHOES_SIM_FIRST_TICK\]'
)
for marker in "${required_markers[@]}"; do
  if ! /usr/bin/grep -Eq "$marker" "$log"; then
    print -u2 "Terms of Continuance runtime marker was absent: $marker"
    print -u2 "Inspect: $log"
    exit 3
  fi
done

if /usr/bin/grep -Eq '\[ECHOES_OPERATION_REQUEST_REJECTED\]|\[ECHOES_TERMS_OF_CONTINUANCE_LOCKED\]|\[ECHOES_TERMS_OF_CONTINUANCE_INIT_FAILED\]|\[ECHOES_BOOT_INCOMPLETE\]|Fatal error:|Assertion failed:' "$log"; then
  print -u2 "Terms of Continuance runtime reported an initialization or fatal failure. Inspect: $log"
  exit 4
fi

print "Terms of Continuance runtime passed: four-record Preserve ledger, joint witness detachment, inherited witness clause, apparent attackers, and first fixed tick initialized."
print "Evidence log: $log"
