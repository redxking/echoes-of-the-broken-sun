#!/bin/zsh
set -euo pipefail

project_root="${0:A:h:h}"
ue_root="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
editor="$ue_root/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
project="$project_root/EchoesOfTheBrokenSun.uproject"
report_dir="$project_root/BuildArtifacts/Automation"
report="$report_dir/index.json"

if [[ ! -x "$editor" ]]; then
  print -u2 "Unreal Editor is not available at: $editor"
  exit 2
fi

mkdir -p "$project_root/BuildArtifacts"
rm -rf "$report_dir"

"$editor" "$project" \
  -unattended -nop4 -nosplash -nullrhi -Multiprocess \
  -ExecCmds="Automation RunTests Echoes; Quit" \
  -TestExit="Automation Test Queue Empty" \
  -ReportExportPath="$report_dir"

if [[ ! -f "$report" ]]; then
  for report_attempt in {1..40}; do
    [[ -f "$report" ]] && break
    sleep 0.25
  done
fi

if [[ ! -f "$report" ]]; then
  print -u2 "Unreal did not produce the expected automation report: $report"
  exit 3
fi

read_report_value() {
  /usr/bin/plutil -extract "$1" raw "$report"
}

if [[ "$(read_report_value succeeded)" != "34" ||
      "$(read_report_value succeededWithWarnings)" != "0" ||
      "$(read_report_value failed)" != "0" ||
      "$(read_report_value notRun)" != "0" ||
      "$(read_report_value inProcess)" != "0" ]]; then
  print -u2 "Unreal automation totals did not match the expected 34/34 clean result."
  print -u2 "Inspect: $report"
  exit 4
fi

expected_tests=(
  "Echoes.Runtime.Accessibility.GameUserSettings"
  "Echoes.Runtime.Bootstrap.ClassesAndCore"
  "Echoes.Runtime.Campaign.WhatTheLedgerKeeps"
  "Echoes.Runtime.Campaign.SevenAccountsOfRain"
  "Echoes.Runtime.Campaign.ACityOnReserve"
  "Echoes.Runtime.Campaign.TheUnburiedRoad"
  "Echoes.Runtime.Campaign.TermsOfContinuance"
  "Echoes.Runtime.Campaign.NamesWithoutBirths"
  "Echoes.Runtime.Campaign.TheShapeOfSilence"
  "Echoes.Runtime.Controls.ControlGroups"
  "Echoes.Runtime.Content.CanonicalPack"
  "Echoes.Runtime.Gameplay.CompleteSkirmish"
  "Echoes.Runtime.Gameplay.BulwarkDeployment"
  "Echoes.Runtime.Gameplay.RelaySupply"
  "Echoes.Runtime.Gameplay.WaystoneMigration"
  "Echoes.Runtime.Gameplay.WarformAdaptation"
  "Echoes.Runtime.Gameplay.MineralCover"
  "Echoes.Runtime.Gameplay.VibrationDetection"
  "Echoes.Runtime.Gameplay.PoweredAegis"
  "Echoes.Runtime.Gameplay.FactionSelection"
  "Echoes.Runtime.Gameplay.FactionResearch"
  "Echoes.Runtime.Gameplay.HoldPosition"
  "Echoes.Runtime.Gameplay.Guard"
  "Echoes.Runtime.Gameplay.Patrol"
  "Echoes.Runtime.Gameplay.ProductionPauseRestart"
  "Echoes.Runtime.Map.GlassScar"
  "Echoes.Runtime.Performance.FourTeamScale"
  "Echoes.Runtime.Presentation.CommandMarkers"
  "Echoes.Runtime.Presentation.FormationLayout"
  "Echoes.Runtime.Presentation.CommandDeckModel"
  "Echoes.Runtime.Presentation.ContactIndicatorLayout"
  "Echoes.Runtime.Persistence.QuickSaveLoad"
  "Echoes.Runtime.Persistence.CampaignProgress"
  "Echoes.Runtime.Visibility.ActorLifecycle"
)

for expected_test in "${expected_tests[@]}"; do
  matched=false
  for test_index in {0..33}; do
    if [[ "$(read_report_value tests.$test_index.fullTestPath)" == "$expected_test" ]]; then
      matched=true
      if [[ "$(read_report_value tests.$test_index.state)" != "Success" ||
            "$(read_report_value tests.$test_index.errors)" != "0" ||
            "$(read_report_value tests.$test_index.warnings)" != "0" ]]; then
        print -u2 "Unreal automation test was not clean: $expected_test"
        print -u2 "Inspect: $report"
        exit 5
      fi
    fi
  done
  if [[ "$matched" != true ]]; then
    print -u2 "Expected Unreal automation test was absent: $expected_test"
    print -u2 "Inspect: $report"
    exit 6
  fi
done

print "Unreal automation passed: 34/34 Echoes tests, 0 warnings, 0 errors."
print "Evidence report: $report"
