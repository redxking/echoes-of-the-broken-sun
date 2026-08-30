#!/bin/zsh
set -euo pipefail

project_root="${0:A:h:h}"
ue_root="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
editor="$ue_root/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
project="$project_root/EchoesOfTheBrokenSun.uproject"
artifact_dir="$project_root/BuildArtifacts/ShapeBesideUs"
ledger="$artifact_dir/FixtureCampaignProgress.bin"
log="${ECHOES_SHAPE_BESIDE_US_LOG:-$artifact_dir/ShapeBesideUsSmoke.log}"
if [[ "$log" != /* ]]; then
  log="$project_root/$log"
fi

if [[ ! -x "$editor" ]]; then
  print -u2 "Unreal Editor is not available at: $editor"
  exit 2
fi

mkdir -p "$artifact_dir"
python3 "$project_root/Scripts/create_campaign_fixture.py" \
  "$ledger" --choice Preserve --through-mission 7
: > "$log"

"$editor" "$project" /Engine/Maps/Entry \
  -game -unattended -nop4 -nosplash -nullrhi -nosound -Multiprocess \
  -EchoesCampaignShapeBesideUs -EchoesAutoStart \
  -EchoesCampaignProgressPath="$ledger" \
  -benchmark -fps=20 -benchmarkseconds=3 -AbsLog="$log"

required_markers=(
  '\[ECHOES_CAMPAIGN_LEDGER_LOAD\] available=true records=7'
  '\[ECHOES_OPERATION_REQUESTED\] operation=TheShapeBesideUs accepted=true'
  '\[ECHOES_FACTION_SCENARIO_READY\] local=MeridianCompact opposition=KharuunAssemblies selectable=true'
  '\[ECHOES_SHAPE_BESIDE_US_READY\] branch=held_echo talar=[1-9][0-9]* witnessA=[1-9][0-9]* witnessB=[1-9][0-9]* firstEcho=\(32,28\) relay=\(32,38\) stateSites=\(28,45\):\(36,45\) convergence=\(32,50\) reciprocalContactOnly=true hollowChoirFactionImplemented=false hiddenAttribution=false inheritedRecords=7'
  '\[ECHOES_BOOT_READY\]'
  '\[ECHOES_SIM_FIRST_TICK\]'
)
for marker in "${required_markers[@]}"; do
  if ! /usr/bin/grep -Eq "$marker" "$log"; then
    print -u2 "The Shape Beside Us runtime marker was absent: $marker"
    print -u2 "Inspect: $log"
    exit 3
  fi
done

if /usr/bin/grep -Eq '\[ECHOES_OPERATION_REQUEST_REJECTED\]|\[ECHOES_SHAPE_BESIDE_US_LOCKED\]|\[ECHOES_SHAPE_BESIDE_US_INIT_FAILED\]|\[ECHOES_BOOT_INCOMPLETE\]|Fatal error:|Assertion failed:' "$log"; then
  print -u2 "The Shape Beside Us runtime reported an initialization or fatal failure. Inspect: $log"
  exit 4
fi

print "The Shape Beside Us runtime passed: seven-record Preserve ledger, Meridian-authoritative witnesses, inherited held-echo geometry, explicit reciprocal-contact boundary, and first fixed tick initialized."
print "Evidence log: $log"
