#!/bin/zsh
set -euo pipefail

project_root="${0:A:h:h}"
ue_root="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
editor="$ue_root/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
project="$project_root/EchoesOfTheBrokenSun.uproject"
report_dir="$project_root/BuildArtifacts/Automation"
report="$report_dir/index.json"
player_save_dir="$project_root/Saved/SaveGames"
temporary_base="${TMPDIR:-/tmp}"
suite_storage_root=""
snapshot_root=""

cleanup_temporary_roots() {
  if [[ -n "$suite_storage_root" &&
        -d "$suite_storage_root" &&
        "${suite_storage_root:t}" == EchoesAutomationSuite.* ]]; then
    rm -rf "$suite_storage_root"
  fi
  if [[ -n "$snapshot_root" &&
        -d "$snapshot_root" &&
        "${snapshot_root:t}" == EchoesSaveSnapshot.* ]]; then
    rm -rf "$snapshot_root"
  fi
}
trap cleanup_temporary_roots EXIT INT TERM

suite_storage_root="$(mktemp -d "$temporary_base/EchoesAutomationSuite.XXXXXX")"
snapshot_root="$(mktemp -d "$temporary_base/EchoesSaveSnapshot.XXXXXX")"
before_manifest="$snapshot_root/before.manifest"
after_manifest="$snapshot_root/after.manifest"

snapshot_player_saves() {
  local destination="$1"
  /usr/bin/python3 - "$player_save_dir" "$destination" <<'PY'
import hashlib
import json
import os
import stat
import sys

root, destination = sys.argv[1:]


def signature(info):
    return {
        "device": info.st_dev,
        "inode": info.st_ino,
        "mode": info.st_mode,
        "size": info.st_size,
        "mtime_ns": info.st_mtime_ns,
    }


entries = []
if not os.path.lexists(root):
    entries.append({"path": "", "type": "root-absent"})
else:
    root_before = os.lstat(root)
    if not stat.S_ISDIR(root_before.st_mode):
        raise RuntimeError("Saved/SaveGames exists but is not a directory")

    def walk(directory, relative_directory):
        directory_before = os.lstat(directory)
        entries.append({
            "path": relative_directory,
            "type": "directory",
            **signature(directory_before),
        })
        children = sorted(
            list(os.scandir(directory)),
            key=lambda child: os.fsencode(child.name),
        )
        for child in children:
            relative_path = os.path.join(relative_directory, child.name)
            child_info = child.stat(follow_symlinks=False)
            record = {"path": relative_path, **signature(child_info)}
            if stat.S_ISLNK(child_info.st_mode):
                record.update(type="symlink", target=os.readlink(child.path))
                entries.append(record)
            elif stat.S_ISDIR(child_info.st_mode):
                walk(child.path, relative_path)
            elif stat.S_ISREG(child_info.st_mode):
                digest = hashlib.sha256()
                with open(child.path, "rb") as source:
                    for block in iter(lambda: source.read(1024 * 1024), b""):
                        digest.update(block)
                child_after = os.lstat(child.path)
                if signature(child_info) != signature(child_after):
                    raise RuntimeError(
                        f"Save file changed while it was being hashed: {relative_path!r}"
                    )
                record.update(type="file", sha256=digest.hexdigest())
                entries.append(record)
            else:
                record.update(type="other")
                entries.append(record)
        directory_after = os.lstat(directory)
        if signature(directory_before) != signature(directory_after):
            raise RuntimeError(
                f"Save directory changed while it was being inventoried: {relative_directory!r}"
            )

    walk(root, "")
    root_after = os.lstat(root)
    if signature(root_before) != signature(root_after):
        raise RuntimeError("Saved/SaveGames changed during snapshot collection")

with open(destination, "w", encoding="utf-8", newline="\n") as output:
    json.dump(entries, output, ensure_ascii=True, sort_keys=True, separators=(",", ":"))
    output.write("\n")
PY
}

if [[ ! -x "$editor" ]]; then
  print -u2 "Unreal Editor is not available at: $editor"
  exit 2
fi

mkdir -p "$project_root/BuildArtifacts"
rm -rf "$report_dir"
if ! snapshot_player_saves "$before_manifest"; then
  print -u2 "[ECHOES_PLAYER_SAVE_SNAPSHOT_FAILED] Could not capture the pre-suite SaveGames manifest."
  exit 9
fi

set +e
"$editor" "$project" \
  -unattended -nop4 -nosplash -nullrhi \
  -EchoesSaveGameDirectory="$suite_storage_root" \
  -ExecCmds="Automation RunTests Echoes; Quit" \
  -TestExit="Automation Test Queue Empty" \
  -ReportExportPath="$report_dir"
editor_status=$?
set -e

if ! snapshot_player_saves "$after_manifest"; then
  print -u2 "[ECHOES_PLAYER_SAVE_SNAPSHOT_FAILED] Could not capture the post-suite SaveGames manifest."
  exit 9
fi
mkdir -p "$report_dir/SaveIsolation"
cp "$before_manifest" "$report_dir/SaveIsolation/player-save-before.manifest"
cp "$after_manifest" "$report_dir/SaveIsolation/player-save-after.manifest"

if ! cmp -s "$before_manifest" "$after_manifest"; then
  diff -u "$before_manifest" "$after_manifest" \
    > "$report_dir/SaveIsolation/player-save-diff.txt" || true
  print -u2 "[ECHOES_PLAYER_SAVE_GUARD_FAILED] Automation changed the real Saved/SaveGames tree."
  print -u2 "No player files were deleted or restored; inspect the retained manifests and diff."
  exit 7
fi

if [[ -n "$(find "$suite_storage_root" -mindepth 1 -print -quit)" ]]; then
  find "$suite_storage_root" -mindepth 1 -print | LC_ALL=C sort \
    > "$report_dir/SaveIsolation/scoped-storage-leftovers.txt"
  print -u2 "[ECHOES_TEST_STORAGE_CLEANUP_FAILED] GUID-scoped automation storage was not empty after the suite."
  exit 8
fi

print -r -- "realSaveGamesUnchanged=true" \
  > "$report_dir/SaveIsolation/result.txt"
print -r -- "scopedStorageEmpty=true" \
  >> "$report_dir/SaveIsolation/result.txt"

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

if [[ "$(read_report_value succeeded)" != "59" ||
      "$(read_report_value succeededWithWarnings)" != "0" ||
      "$(read_report_value failed)" != "0" ||
      "$(read_report_value notRun)" != "0" ||
      "$(read_report_value inProcess)" != "0" ]]; then
  print -u2 "Unreal automation totals did not match the expected 59/59 clean result."
  print -u2 "Inspect: $report"
  exit 4
fi

expected_tests=(
  "Echoes.Runtime.Accessibility.GameUserSettings"
  "Echoes.Runtime.Audio.MixArchitecture"
  "Echoes.Runtime.Audio.MusicAmbience"
  "Echoes.Runtime.Audio.InterfaceCues"
  "Echoes.Runtime.AI.SkirmishDeterminismSmoke"
  "Echoes.Runtime.Bootstrap.ClassesAndCore"
  "Echoes.Runtime.Campaign.WhatTheLedgerKeeps"
  "Echoes.Runtime.Campaign.Journey"
  "Echoes.Runtime.Campaign.FreshJourney"
  "Echoes.Runtime.Campaign.SevenAccountsOfRain"
  "Echoes.Runtime.Campaign.ACityOnReserve"
  "Echoes.Runtime.Campaign.TheUnburiedRoad"
  "Echoes.Runtime.Campaign.TermsOfContinuance"
  "Echoes.Runtime.Campaign.NamesWithoutBirths"
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
  "Echoes.Runtime.Controls.ControlGroups"
  "Echoes.Runtime.Controls.SharedKeyDispatch"
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
  "Echoes.Runtime.Gameplay.SkirmishSetup"
  "Echoes.Runtime.Gameplay.FactionResearch"
  "Echoes.Runtime.Gameplay.HoldPosition"
  "Echoes.Runtime.Gameplay.Guard"
  "Echoes.Runtime.Gameplay.Patrol"
  "Echoes.Runtime.Gameplay.ProductionPauseRestart"
  "Echoes.Runtime.Map.GlassScar"
  "Echoes.Runtime.Map.PresentationProfiles"
  "Echoes.Runtime.Network.ProtocolAdmission"
  "Echoes.Runtime.Network.OnlineFrontDoor"
  "Echoes.Runtime.Performance.FourTeamScale"
  "Echoes.Runtime.Performance.SustainedFourTeamScale"
  "Echoes.Runtime.Presentation.CommandMarkers"
  "Echoes.Runtime.Presentation.DestructionVFX"
  "Echoes.Runtime.Presentation.AudioConfirmation"
  "Echoes.Runtime.Presentation.FormationLayout"
  "Echoes.Runtime.Presentation.Pooling"
  "Echoes.Runtime.Presentation.CommandDeckModel"
  "Echoes.Runtime.Presentation.ContactIndicatorLayout"
  "Echoes.Runtime.Persistence.QuickSaveLoad"
  "Echoes.Runtime.Persistence.CampaignProgress"
  "Echoes.Runtime.Presentation.VisualTheme"
  "Echoes.Runtime.Visibility.ActorLifecycle"
)

for expected_test in "${expected_tests[@]}"; do
  matched=false
  for test_index in {0..58}; do
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

print "Unreal automation passed: 59/59 Echoes tests, 0 warnings, 0 errors."
print "Player SaveGames guard passed: sampled tree unchanged; scoped storage empty."
print "Evidence report: $report"
