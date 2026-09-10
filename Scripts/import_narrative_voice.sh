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
  # Match the executable, not a substring: UnrealEditorServices and monitor
  # shells both carry "UnrealEditor" in their command lines and are not
  # editors. An interactive editor is the UnrealEditor binary without -game;
  # a -game or -benchmark run stages no deletions, so it is contention only
  # and is handled by the build-slot check below.
  interactive=$(ps -Ao comm=,args= \
    | awk '$1 ~ /\/UnrealEditor$/ && $0 !~ / -game( |$)/' | wc -l | tr -d ' ')
  if [[ "$interactive" != "0" ]]; then
    print -u2 "An interactive Unreal editor is open; it stages asset deletions"
    print -u2 "and the import would fail silently as marked-for-delete (SCC trap)."
    print -u2 "Close it and re-run."
    exit 3
  fi
  # Block rather than bounce: name the holder, say whether it is working, and
  # fire the moment it clears. A holder sitting at 0.0% CPU with no bounded
  # duration argument is called out, because a hung holder and a busy one look
  # identical from a single sample.
  if ! "$project_root/Scripts/acquire_build_slot.sh" >/dev/null 2>&1; then
    for pid in ${(f)"$("$project_root/Scripts/acquire_build_slot.sh" 2>&1 \
        | grep -oE 'pid=[0-9]+' | cut -d= -f2)"}; do
      args=$(ps -o args= -p "$pid" 2>/dev/null | cut -c1-120)
      [[ -z "$args" ]] && { print "attempt $attempt: holder pid=$pid already gone (stale report)"; continue; }
      cpu1=$(ps -o %cpu= -p "$pid" 2>/dev/null | tr -d ' ')
      sleep 3
      cpu2=$(ps -o %cpu= -p "$pid" 2>/dev/null | tr -d ' ')
      el=$(ps -o etime= -p "$pid" 2>/dev/null | tr -d ' ')
      bounded="unbounded"
      [[ "$args" == *-benchmarkseconds=* || "$args" == *-ExecutePythonScript=* \
         || "$args" == *BuildCookRun* ]] && bounded="bounded"
      # Only call a holder hung once it has had time to be one. A compile
      # step legitimately idles for a few seconds while it waits on I/O, and
      # flagging that is noise that trains the reader to ignore the flag.
      idle=""
      mins=${${el%:*}%%:*}
      [[ "$el" == *:*:* ]] && mins=99
      if [[ "$cpu1" == "0.0" && "$cpu2" == "0.0" && "$bounded" == "unbounded" \
            && "$mins" -ge 2 ]]; then
        idle="  <-- 0.0%% CPU for two samples, up $el, no bounded-duration argument; possibly hung"
      fi
      print "attempt $attempt/$attempts: waiting on pid=$pid cpu=${cpu1}/${cpu2}% up=$el $bounded$idle"
      print "    $args"
    done
    sleep 20
    continue
  fi

  purge_log="$evidence/purge-attempt-$attempt.log"
  purge_out="$evidence/purge-launcher-attempt-$attempt.log"
  # Separate session: delete-then-reimport in one session strands the package name.
  set +e
  "$editor" "$project" \
    -unattended -nop4 -nosplash -nullrhi -NoSound -SCCProvider=None \
    -ExecutePythonScript="$project_root/Scripts/purge_stale_voice_assets.py" \
    -abslog="$purge_log" > "$purge_out" 2>&1
  set -e
  if grep -q "conflicting instance of Global" "$purge_out" 2>/dev/null; then
    print "attempt $attempt/$attempts: UBT mutex conflict during purge, retrying in 60s"
    sleep 60
    continue
  fi
  if ! grep -q "ECHOES_VOICE_PURGE_READY" "$purge_log" 2>/dev/null; then
    print -u2 "attempt $attempt: stale-voice purge did not complete."
    print -u2 "  purge log: $purge_log"
    exit 6
  fi
  grep -o "\[ECHOES_VOICE_PURGE_READY\].*" "$purge_log" | tail -1

  log="$evidence/import-attempt-$attempt.log"
  out="$evidence/launcher-attempt-$attempt.log"
  set +e
  "$editor" "$project" \
    -unattended -nop4 -nosplash -nullrhi -NoSound -SCCProvider=None \
    -ExecutePythonScript="$script" -abslog="$log" > "$out" 2>&1
  editor_status=$?
  set -e

  if grep -q "conflicting instance of Global" "$out" 2>/dev/null; then
    print "attempt $attempt/$attempts: UBT mutex conflict, retrying in 60s"
    sleep 60
    continue
  fi
  if ! grep -q "ECHOES_NARRATIVE_VOICE_READY" "$log" 2>/dev/null; then
    print -u2 "attempt $attempt: editor exited (status $editor_status) without the import marker."
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
