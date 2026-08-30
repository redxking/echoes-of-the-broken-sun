#!/bin/zsh
set -euo pipefail

project_root="${0:A:h:h}"
ue_root="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
editor="$ue_root/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
project="$project_root/EchoesOfTheBrokenSun.uproject"
artifact_dir="$project_root/BuildArtifacts/ReserveAuthority"
ledger="$artifact_dir/FixtureCampaignProgress.bin"
log="${ECHOES_RESERVE_AUTHORITY_LOG:-$artifact_dir/ReserveAuthoritySmoke.log}"
if [[ "$log" != /* ]]; then
  log="$project_root/$log"
fi

if [[ ! -x "$editor" ]]; then
  print -u2 "Unreal Editor is not available at: $editor"
  exit 2
fi

mkdir -p "$artifact_dir"
python3 "$project_root/Scripts/create_campaign_fixture.py" \
  "$ledger" --choice Preserve --through-mission 8
: > "$log"

"$editor" "$project" /Engine/Maps/Entry \
  -game -unattended -nop4 -nosplash -nullrhi -nosound -Multiprocess \
  -EchoesCampaignReserveAuthority -EchoesAutoStart \
  -EchoesCampaignProgressPath="$ledger" \
  -benchmark -fps=20 -benchmarkseconds=3 -AbsLog="$log"

required_markers=(
  '\[ECHOES_CAMPAIGN_LEDGER_LOAD\] available=true records=8'
  '\[ECHOES_OPERATION_REQUESTED\] operation=ReserveAuthority accepted=true'
  '\[ECHOES_FACTION_SCENARIO_READY\] local=MeridianCompact opposition=KharuunAssemblies selectable=true'
  '\[ECHOES_RESERVE_AUTHORITY_READY\] branch=continuity_reserve mara=[1-9][0-9]* districts=[1-9][0-9]*,[1-9][0-9]*,[1-9][0-9]* authority=\(15,15\) recommended=ARCHIVE CONTINUITY allocation=exactlyTwo deferredMustSurvive=true localDecisionOnly=true widerCityRestored=false civilianSurvivalUnmodeled=true inheritedRecords=8'
  '\[ECHOES_BOOT_READY\]'
  '\[ECHOES_SIM_FIRST_TICK\]'
)
for marker in "${required_markers[@]}"; do
  if ! /usr/bin/grep -Eq "$marker" "$log"; then
    print -u2 "Reserve Authority runtime marker was absent: $marker"
    print -u2 "Inspect: $log"
    exit 3
  fi
done

if /usr/bin/grep -Eq '\[ECHOES_OPERATION_REQUEST_REJECTED\]|\[ECHOES_RESERVE_AUTHORITY_LOCKED\]|\[ECHOES_RESERVE_AUTHORITY_INIT_FAILED\]|\[ECHOES_BOOT_INCOMPLETE\]|Fatal error:|Assertion failed:' "$log"; then
  print -u2 "Reserve Authority runtime reported an initialization or fatal failure. Inspect: $log"
  exit 4
fi

print "Reserve Authority startup smoke passed: eight-record Preserve ledger, Meridian authority, branch-specific exact-two allocation requirement, explicit local-decision boundary, and first fixed tick initialized."
print "Evidence log: $log"
