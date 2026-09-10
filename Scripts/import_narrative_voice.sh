#!/bin/zsh
# Import the prepared narrative voice WAVs as SoundWave assets, resiliently.
#
# Two failure modes made the bare commandlet unsafe to run by hand:
#
#   1. UBT holds a global mutex. If another lane is compiling, the editor hits
#      "A conflicting instance of Global\UnrealBuildTool_Mutex_..." during
#      startup SDK validation and exits BEFORE executing the Python script.
#      Nothing appears in the editor log - no error, no marker - so the run
#      looks clean and imports nothing. This retries past that.
#   2. The recorded SCC trap: an open editor stages asset deletions and the
#      import then fails silently as marked-for-delete. This refuses to start
#      while an editor binary is running, and passes -SCCProvider=None.
#
# It fails loudly rather than silently: the completion marker must appear AND
# the .uasset hashes must actually change (or every line already be at the
# recorded revision), or the script exits nonzero.
set -euo pipefail

project_root="${0:A:h:h}"
ue_root="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
editor="$ue_root/Engine/Binaries/Mac/UnrealEditor-Cmd"
project="$project_root/EchoesOfTheBrokenSun.uproject"
script="$project_root/Scripts/import_narrative_voice.py"
attempts="${ECHOES_IMPORT_ATTEMPTS:-6}"

evidence="${1:-$project_root/BuildArtifacts/Evidence/voice-import-$(date -u +%Y%m%dT%H%M%SZ)}"
mkdir -p "$evidence"

[[ -x "$editor" ]] || { print -u2 "editor not found: $editor"; exit 2; }

before="$evidence/uasset-before.sha256"
after="$evidence/uasset-after.sha256"
shasum -a 256 "$project_root"/Content/Audio/Voice/*.uasset 2>/dev/null | sort > "$before" || : > "$before"

for attempt in {1..$attempts}; do
  # An editor binary staging deletions is the trap; never race it.
  if ps -Ao comm | grep -q "UnrealEditor$"; then
    print -u2 "An Unreal editor is running; close it before importing (SCC trap)."
    exit 3
  fi
  # Courtesy check. UBT is the real contender, not just the editor.
  if ! "$project_root/Scripts/acquire_build_slot.sh" >/dev/null 2>&1; then
    print "attempt $attempt/$attempts: build slot busy, waiting 60s"
    sleep 60
    continue
  fi

  log="$evidence/import-attempt-$attempt.log"
  out="$evidence/launcher-attempt-$attempt.log"
  set +e
  "$editor" "$project" \
    -unattended -nop4 -nosplash -nullrhi -NoSound -SCCProvider=None \
    -ExecutePythonScript="$script" -abslog="$log" > "$out" 2>&1
  status=$?
  set -e

  if grep -q "conflicting instance of Global" "$out" 2>/dev/null; then
    print "attempt $attempt/$attempts: UBT mutex conflict, retrying in 60s"
    sleep 60
    continue
  fi
  if ! grep -q "ECHOES_NARRATIVE_VOICE_READY" "$log" 2>/dev/null; then
    print -u2 "attempt $attempt: editor exited (status $status) without the import marker."
    print -u2 "  editor log:   $log"
    print -u2 "  launcher log: $out"
    exit 4
  fi

  shasum -a 256 "$project_root"/Content/Audio/Voice/*.uasset | sort > "$after"
  changed=$(comm -13 "$before" "$after" | wc -l | tr -d ' ')
  total=$(wc -l < "$after" | tr -d ' ')
  print "[ECHOES_VOICE_IMPORT] attempt=$attempt assets=$total changed=$changed"
  grep -o "\[ECHOES_NARRATIVE_VOICE_READY\].*" "$log" | tail -1
  print "evidence: $evidence"
  exit 0
done

print -u2 "build slot never came free across $attempts attempts; nothing imported"
exit 5
