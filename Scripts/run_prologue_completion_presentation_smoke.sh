#!/bin/zsh
set -euo pipefail

project_root="${0:A:h:h}"
ue_root="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
editor="$ue_root/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
project="$project_root/EchoesOfTheBrokenSun.uproject"
evidence_dir="$project_root/BuildArtifacts/PrologueCompletion"
log="$evidence_dir/PrologueCompletionSmoke.log"
ledger="$evidence_dir/FixtureCampaignProgress.bin"

if [[ ! -x "$editor" ]]; then
  print -u2 "Unreal Editor is not available at: $editor"
  exit 2
fi

mkdir -p "$evidence_dir"
rm -f "$log" "$ledger" "$ledger.bak" "$ledger.tmp"

"$editor" "$project" /Engine/Maps/Entry \
  -game -unattended -nop4 -nosplash -nullrhi -nosound \
  -EchoesCampaignPrologue -EchoesAutoStart \
  -EchoesPrologueCompletionPresentation \
  -EchoesCampaignProgressPath="$ledger" \
  -benchmark -fps=20 -benchmarkseconds=30 -AbsLog="$log"

required_markers=(
  '\[ECHOES_PROLOGUE_COMPLETION_PRESENTATION_READY\].*protocol=Preserve.*controlled=true'
  '\[ECHOES_PROLOGUE_COMPLETION_PRESENTATION_STAGE\] stage=recover_archive command=ordinary_move'
  '\[ECHOES_PROLOGUE_COMPLETION_PRESENTATION_STAGE\] stage=decide_well command=ordinary_move protocol=Preserve'
  '\[ECHOES_PROLOGUE_COMPLETION_PRESENTATION_STAGE\] stage=preserve command=ordinary_future_well'
  '\[ECHOES_PROLOGUE_COMPLETION_PRESENTATION_STAGE\] stage=withdraw command=ordinary_move'
  '\[ECHOES_PROLOGUE_FINISHED\] result=success phase=complete consequence=2 recordedConsequence=2 campaignStatus=1'
  '\[ECHOES_PROLOGUE_COMPLETION_PRESENTATION_COMPLETE\] phase=complete resultPresented=true ledgerRecords=1 controlled=true release=false'
)
for marker in "${required_markers[@]}"; do
  if ! /usr/bin/grep -Eq "$marker" "$log"; then
    print -u2 "Prologue completion marker was absent: $marker"
    print -u2 "Inspect: $log"
    exit 3
  fi
done

if [[ ! -s "$ledger" ]]; then
  print -u2 "The controlled campaign ledger was not committed: $ledger"
  exit 4
fi

if /usr/bin/grep -Eq '\[ECHOES_PROLOGUE_COMPLETION_PRESENTATION_FAILED\]|\[CAMPAIGN_.*FAILED\]|Fatal error:|Assertion failed:' "$log"; then
  print -u2 "The controlled prologue completion reported a failure. Inspect: $log"
  exit 5
fi

print "Campaign completion presentation passed: ordinary recovery, Well decision, withdrawal, result, and isolated ledger commit completed."
print "Evidence log: $log"
print "Isolated ledger: $ledger"
