#!/bin/zsh
set -euo pipefail

project_root="${0:A:h:h}"
ue_root="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
editor="$ue_root/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
project="$project_root/EchoesOfTheBrokenSun.uproject"
report_dir="${ECHOES_AUTOMATION_REPORT_DIR:-$project_root/BuildArtifacts/Automation/$(date -u +%Y%m%dT%H%M%SZ)-$$}"
report="$report_dir/index.json"

if [[ ! -x "$editor" ]]; then
  print -u2 "Unreal Editor is not available at: $editor"
  exit 2
fi

mkdir -p "$project_root/BuildArtifacts"
if [[ -e "$report_dir" ]]; then
  print -u2 "Automation report directory already exists; choose a fresh directory: $report_dir"
  exit 3
fi

set +e
/usr/bin/python3 "$project_root/Scripts/echoes_test_sandbox.py" \
  --editor "$editor" \
  --project "$project" \
  --report-dir "$report_dir" \
  -- \
  -unattended -nop4 -nosplash -nullrhi -Multiprocess \
  -ExecCmds="Automation RunTests Echoes.; Quit" \
  -TestExit="Automation Test Queue Empty" \
  -ReportExportPath="$report_dir" \
  -abslog="$report_dir/Engine.log"
editor_status=$?
set -e

isolation_report="$report_dir/SaveIsolation/launcher-result.json"
if [[ ! -f "$isolation_report" ]]; then
  print -u2 "[ECHOES_TEST_SANDBOX_REPORT_MISSING] The isolated launcher did not produce a result report."
  exit 9
fi
if ! /usr/bin/python3 - "$isolation_report" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as source:
    report = json.load(source)
if not report.get("synthetic_denial_probe"):
    raise SystemExit("synthetic sandbox denial probe did not pass")
if not report.get("protected_policy_clauses_verified"):
    raise SystemExit("targeted production deny clauses were not verified")
if not report.get("scoped_save_directory_empty_after_run"):
    raise SystemExit("scoped SaveGames directory was not empty after the suite")
if not report.get("cleanup_succeeded") or report.get("prelaunch_failure"):
    raise SystemExit("sandbox launch or cleanup did not complete successfully")
PY
then
  print -u2 "[ECHOES_TEST_SANDBOX_GUARD_FAILED] Inspect: $isolation_report"
  exit 8
fi

if (( editor_status != 0 )); then
  print -u2 "Unreal Editor exited with status $editor_status."
  exit "$editor_status"
fi

if [[ ! -f "$report" ]]; then
  for report_attempt in {1..240}; do
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

expected_tests=(
  "Echoes.Runtime.AI.DifficultyController"
  "Echoes.Runtime.AI.StandardLongRunCorefall"
  "Echoes.Runtime.Gameplay.CompleteSkirmishDefeat"
  "Echoes.Runtime.Input.BuildPlacementPreview"
  "Echoes.Runtime.Input.ContextCursorModel"
  "Echoes.Runtime.Persistence.ImmutableCheckpointWorker"
  "Echoes.Runtime.Persistence.AsyncCheckpointLifecycle"
  "Echoes.Runtime.Persistence.AsyncCheckpointStressBudget"
  "Echoes.Runtime.Persistence.AsyncCheckpointReplayBindingFailure"
  "Echoes.Runtime.Replay.AuthorityStorageAndTransport"
  "Echoes.Runtime.Replay.DetachedEntityPresentation"
  "Echoes.Runtime.Replay.ScopedTerrainMemory"
  "Echoes.Runtime.Replay.AsyncBrowserGeneration"
  "Echoes.Runtime.UI.ShellWidgetRefreshAndFocus"
  "Echoes.Runtime.Presentation.FieldHudAuthority"
  "Echoes.Runtime.UI.FieldHudWidget"
  "Echoes.Runtime.FieldHud.ControllerAuthorityRoutes"
  "Echoes.Runtime.UI.PlayerShellRoutes"
  "Echoes.Runtime.Persistence.PlayerProfile"
  "Echoes.Runtime.Map.ContinuousCliffGeometry"
  "Echoes.Runtime.Map.M01ExteriorBanks"
  "Echoes.Runtime.Accessibility.GameUserSettings"
  "Echoes.Runtime.Audio.MixArchitecture"
  "Echoes.Runtime.Audio.MusicAmbience"
  "Echoes.Runtime.Audio.InterfaceCues"
  "Echoes.Runtime.Audio.GameplayCues"
  "Echoes.Runtime.Narrative.PackBinding"
  "Echoes.Runtime.AI.GuardEscortSemantics"
  "Echoes.Runtime.AI.SkirmishDeterminismSmoke"
  "Echoes.Runtime.Bootstrap.ClassesAndCore"
  "Echoes.Runtime.Camera.OrthographicFraming"
  "Echoes.Runtime.Campaign.WhatTheLedgerKeeps"
  "Echoes.Runtime.Cinematics.ReferenceSequence"
  "Echoes.Runtime.Campaign.Journey"
  "Echoes.Runtime.Campaign.FreshJourney"
  "Echoes.Runtime.Campaign.SevenAccountsOfRain"
  "Echoes.Runtime.Campaign.ACityOnReserve"
  "Echoes.Runtime.Campaign.TheUnburiedRoad"
  "Echoes.Runtime.Campaign.TermsOfContinuance"
  "Echoes.Runtime.Campaign.NamesWithoutBirths"
  "Echoes.Runtime.Campaign.PlanMatrix"
  "Echoes.Runtime.Campaign.FailureReasons"
  "Echoes.Runtime.Campaign.TheShapeOfSilence"
  "Echoes.Runtime.Campaign.TheShapeBesideUs"
  "Echoes.Runtime.Campaign.ReserveAuthority"
  "Echoes.Runtime.Campaign.ChoirAtLumeReach"
  "Echoes.Runtime.Campaign.NoNeutralLedger"
  "Echoes.Runtime.Campaign.FutureThatWon"
  "Echoes.Runtime.Campaign.AssemblyOfTheMissing"
  "Echoes.Runtime.Campaign.SeveralVoicesOneCommand"
  "Echoes.Runtime.Campaign.TheBrokenSun"
  "Echoes.Runtime.Campaign.TheBrokenSunAlternateResolutionPersistence"
  "Echoes.Runtime.Campaign.TutorialCurriculum"
  "Echoes.Runtime.Campaign.TutorialPracticeState"
  "Echoes.Runtime.Campaign.TutorialSurveyObservation"
  "Echoes.Runtime.Campaign.TutorialSelectionObservation"
  "Echoes.Runtime.Campaign.TutorialOrderObservation"
  "Echoes.Runtime.Campaign.TutorialAnchorSelectionProgression"
  "Echoes.Runtime.Training.ReadinessOperationPersistenceAndReplay"
  "Echoes.Runtime.Controls.ContextOrderBanner"
  "Echoes.Runtime.Controls.ControlGroups"
  "Echoes.Runtime.Controls.PointerSurfaceCoverage"
  "Echoes.Runtime.Controls.SharedArrowDispatch"
  "Echoes.Runtime.Controls.SharedKeyDispatch"
  "Echoes.Runtime.Content.CanonicalPack"
  "Echoes.Runtime.Gameplay.CompleteSkirmish"
  "Echoes.Runtime.Gameplay.BulwarkDeployment"
  "Echoes.Runtime.Gameplay.WorkerMaintenanceActions"
  "Echoes.Runtime.Gameplay.ActionFeedback"
  "Echoes.Runtime.Gameplay.RelaySupply"
  "Echoes.Runtime.Gameplay.WaystoneMigration"
  "Echoes.Runtime.Gameplay.WarformAdaptation"
  "Echoes.Runtime.Gameplay.MineralCover"
  "Echoes.Runtime.Gameplay.VibrationDetection"
  "Echoes.Runtime.Gameplay.PoweredAegis"
  "Echoes.Runtime.Gameplay.FactionSelection"
  "Echoes.Runtime.Gameplay.SkirmishSetup"
  "Echoes.Runtime.Gameplay.FactionResearch"
  "Echoes.Runtime.Gameplay.HoldPosition"
  "Echoes.Runtime.Gameplay.Guard"
  "Echoes.Runtime.Gameplay.Patrol"
  "Echoes.Runtime.Gameplay.ProductionPauseRestart"
  "Echoes.Runtime.Map.CompiledMapBinding"
  "Echoes.Runtime.Map.GlassScar"
  "Echoes.Runtime.Map.GlassScarDressing"
  "Echoes.Runtime.Map.LumeReachDressing"
  "Echoes.Runtime.Map.PresentationProfiles"
  "Echoes.Runtime.Map.WorldKitVisibility"
  "Echoes.Runtime.Map.MissionLandmarkVisibility"
  "Echoes.Runtime.Map.CampaignTerrain"
  "Echoes.Runtime.Persistence.CampaignMapCheckpoint"
  "Echoes.Runtime.Persistence.CampaignMapAdmission"
  "Echoes.Runtime.Network.ProtocolAdmission"
  "Echoes.Runtime.Network.BulwarkActionDispatch"
  "Echoes.Runtime.Network.BulwarkPendingReconciliation"
  "Echoes.Runtime.Network.OnlineFrontDoor"
  "Echoes.Runtime.Performance.FourTeamScale"
  "Echoes.Runtime.Performance.SustainedFourTeamScale"
  "Echoes.Runtime.Presentation.AudioConfirmation"
  "Echoes.Runtime.Presentation.CombatEffects"
  "Echoes.Runtime.Presentation.CommandDeckModel"
  "Echoes.Runtime.Presentation.CommandMarkers"
  "Echoes.Runtime.Presentation.ContactIndicatorLayout"
  "Echoes.Runtime.Presentation.DestructionVFX"
  "Echoes.Runtime.Presentation.FormationLayout"
  "Echoes.Runtime.Presentation.FutureWellHarvestCollapse"
  "Echoes.Runtime.Presentation.MotionFamilies"
  "Echoes.Runtime.Presentation.M01BulwarkParts"
  "Echoes.Runtime.Presentation.M01SurveyorRig"
  "Echoes.Runtime.Presentation.M01WellExpiry"
  "Echoes.Runtime.Presentation.M01WorkContact"
  "Echoes.Runtime.Presentation.Pooling"
  "Echoes.Runtime.Presentation.ProductionFog"
  "Echoes.Runtime.Persistence.QuickSaveLoad"
  "Echoes.Runtime.Persistence.CampaignProgress"
  "Echoes.Runtime.Persistence.CampaignSlots"
  "Echoes.Runtime.Persistence.AutosaveRecovery"
  "Echoes.Runtime.Presentation.VisualTheme"
  "Echoes.Runtime.Visibility.ActorLifecycle"
  "Echoes.Runtime.Campaign.OperationsMap"
  "Echoes.Runtime.Campaign.FailureReasonDisplay"
  # HUD revision 2: controls, feedback history, resource monitor, selection guidance.
  "Echoes.Runtime.Controls.ActiveCommandPrompts"
  "Echoes.Runtime.Controls.BindingModel"
  "Echoes.Runtime.Controls.TabContextDispatch"
  "Echoes.Runtime.Presentation.FeedbackHistoryModel"
  "Echoes.Runtime.Presentation.ResourceMonitor.PlayerScopedFacts"
  "Echoes.Runtime.UI.BattlefieldMenuAffordance"
  "Echoes.Runtime.UI.ControlsCaptureRoutes"
  "Echoes.Runtime.UI.ControlsPersistence"
  "Echoes.Runtime.UI.FeedbackHistoryShellRoutes"
  "Echoes.Runtime.UI.OnlineLocalMenuShellRoutes"
  "Echoes.Runtime.UI.ResourceMonitorShellRoutes"
  "Echoes.Runtime.UI.SelectionGuidanceCoverage"
)

num_expected="${#expected_tests[@]}"
# Scan bound comes from the REPORT, not from the expected array. Deriving it from
# num_expected made every added test invisible to the per-test scan below, so an
# expected test sitting past the old bound was reported absent (exit 6).
report_test_count="$(/usr/bin/plutil -extract tests raw "$report" 2>/dev/null)"
if [[ -z "$report_test_count" || "$report_test_count" != <-> ]]; then
  print -u2 "Could not read the automation report test count: $report"
  exit 4
fi
test_max_index=$(( report_test_count - 1 ))
typeset -A expected_inventory
for expected_test in "${expected_tests[@]}"; do
  if [[ -n "${expected_inventory[$expected_test]-}" ]]; then
    print -u2 "Expected Unreal automation inventory contains a duplicate: $expected_test"
    exit 4
  fi
  expected_inventory[$expected_test]=1
done

for expected_test in "${expected_tests[@]}"; do
  match_count=0
  for (( test_index=0; test_index<=test_max_index; test_index++ )); do
    if [[ "$(read_report_value tests.$test_index.fullTestPath)" == "$expected_test" ]]; then
      match_count=$(( match_count + 1 ))
      if [[ "$(read_report_value tests.$test_index.state)" != "Success" ||
            "$(read_report_value tests.$test_index.errors)" != "0" ||
            "$(read_report_value tests.$test_index.warnings)" != "0" ]]; then
        print -u2 "Unreal automation test was not clean: $expected_test"
        print -u2 "Inspect: $report"
        exit 5
      fi
    fi
  done
  if (( match_count == 0 )); then
    print -u2 "Expected Unreal automation test was absent: $expected_test"
    print -u2 "Inspect: $report"
    exit 6
  fi
  if (( match_count > 1 )); then
    print -u2 "Unreal automation inventory contains a duplicate: $expected_test"
    print -u2 "Inspect: $report"
    exit 4
  fi
done

# Every expected test must be present and clean, and nothing may be unclean — but a
# lane adding its own test must not break every other lane's gate. So: no exact-count
# equality against the shared array; succeeded may exceed it.
succeeded_count="$(read_report_value succeeded)"
if [[ "$(read_report_value succeededWithWarnings)" != "0" ||
      "$(read_report_value failed)" != "0" ||
      "$(read_report_value notRun)" != "0" ||
      "$(read_report_value inProcess)" != "0" ]]; then
  print -u2 "Unreal automation reported a non-clean result (failed/warnings/notRun/inProcess must all be 0)."
  print -u2 "Inspect: $report"
  exit 4
fi
if (( succeeded_count < num_expected )); then
  print -u2 "Unreal automation succeeded ${succeeded_count}, fewer than the ${num_expected} expected tests."
  print -u2 "Inspect: $report"
  exit 4
fi

print "Unreal automation passed: ${succeeded_count} Echoes tests (${num_expected} expected present), 0 warnings, 0 errors."
print "Save isolation boundary passed: exact deny clauses and synthetic protected-data denial passed; scoped storage was empty."
print "Evidence report: $report"
