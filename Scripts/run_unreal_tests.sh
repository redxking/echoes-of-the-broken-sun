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
  -unattended -nop4 -nosplash -nullrhi \
  -ExecCmds="Automation RunTests Echoes; Quit" \
  -TestExit="Automation Test Queue Empty" \
  -ReportExportPath="$report_dir"

if [[ ! -f "$report" ]]; then
  print -u2 "Unreal did not produce the expected automation report: $report"
  exit 3
fi

read_report_value() {
  /usr/bin/plutil -extract "$1" raw "$report"
}

if [[ "$(read_report_value succeeded)" != "9" ||
      "$(read_report_value succeededWithWarnings)" != "0" ||
      "$(read_report_value failed)" != "0" ||
      "$(read_report_value notRun)" != "0" ||
      "$(read_report_value inProcess)" != "0" ]]; then
  print -u2 "Unreal automation totals did not match the accepted 9/9 clean result."
  print -u2 "Inspect: $report"
  exit 4
fi

expected_tests=(
  "Echoes.Runtime.Bootstrap.ClassesAndCore"
  "Echoes.Runtime.Controls.ControlGroups"
  "Echoes.Runtime.Gameplay.CompleteSkirmish"
  "Echoes.Runtime.Gameplay.HoldPosition"
  "Echoes.Runtime.Gameplay.Guard"
  "Echoes.Runtime.Gameplay.ProductionPauseRestart"
  "Echoes.Runtime.Map.GlassScar"
  "Echoes.Runtime.Persistence.QuickSaveLoad"
  "Echoes.Runtime.Visibility.ActorLifecycle"
)

for expected_test in "${expected_tests[@]}"; do
  matched=false
  for test_index in 0 1 2 3 4 5 6 7 8; do
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

print "Unreal automation passed: 9/9 Echoes tests, 0 warnings, 0 errors."
print "Evidence report: $report"
