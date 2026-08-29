#!/bin/zsh
set -euo pipefail

project_root="${0:A:h:h}"
ue_root="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
editor="$ue_root/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
project="$project_root/EchoesOfTheBrokenSun.uproject"
artifact_dir="$project_root/BuildArtifacts/SevenAccounts"
ledger="$artifact_dir/FixtureCampaignProgress.bin"
log="${ECHOES_SEVEN_ACCOUNTS_LOG:-$artifact_dir/SevenAccountsSmoke.log}"
if [[ "$log" != /* ]]; then
  log="$project_root/$log"
fi

if [[ ! -x "$editor" ]]; then
  print -u2 "Unreal Editor is not available at: $editor"
  exit 2
fi

mkdir -p "$artifact_dir"
python3 "$project_root/Scripts/create_campaign_fixture.py" \
  "$ledger" --choice Preserve
: > "$log"

"$editor" "$project" /Engine/Maps/Entry \
  -game -unattended -nop4 -nosplash -nullrhi -nosound \
  -EchoesCampaignSevenAccounts -EchoesAutoStart \
  -EchoesCampaignProgressPath="$ledger" \
  -benchmark -fps=20 -benchmarkseconds=3 -AbsLog="$log"

required_markers=(
  '\[ECHOES_CAMPAIGN_LEDGER_LOAD\] available=true records=1'
  '\[ECHOES_OPERATION_REQUESTED\] operation=SevenAccountsOfRain accepted=true'
  '\[ECHOES_FACTION_SCENARIO_READY\] local=KharuunAssemblies opposition=MeridianCompact selectable=true'
  '\[ECHOES_SEVEN_ACCOUNTS_READY\] branch=archive_verified_central_account waystone=[1-9][0-9]* bearer=[1-9][0-9]* anchor=\(35,40\) account=\(38,43\) terrainDelta=0 blocked=165'
  '\[ECHOES_BOOT_READY\]'
  '\[ECHOES_SIM_FIRST_TICK\]'
)
for marker in "${required_markers[@]}"; do
  if ! /usr/bin/grep -Eq "$marker" "$log"; then
    print -u2 "Seven Accounts runtime marker was absent: $marker"
    print -u2 "Inspect: $log"
    exit 3
  fi
done

if /usr/bin/grep -Eq '\[ECHOES_OPERATION_REQUEST_REJECTED\]|\[ECHOES_SEVEN_ACCOUNTS_LOCKED\]|\[ECHOES_SEVEN_ACCOUNTS_INIT_FAILED\]|\[ECHOES_BOOT_INCOMPLETE\]|Fatal error:|Assertion failed:' "$log"; then
  print -u2 "Seven Accounts runtime reported an initialization or fatal failure. Inspect: $log"
  exit 4
fi

print "Seven Accounts runtime passed: inherited Preserve ledger, Kharuun force, branch-specific route, Waystone, Oruun, and first fixed tick initialized."
print "Evidence log: $log"
