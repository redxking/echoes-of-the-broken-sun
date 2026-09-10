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
  # Refuse only for the actual hazard: an INTERACTIVE editor session, which
  # stages asset deletions so the import fails silently as marked-for-delete.
  # A -game/-benchmark run, a -nullrhi automation run, or a commandlet stages
  # nothing and touches no assets, so it is CPU contention only.
  #
  # Two independent protections cover the trap regardless: this run passes
  # -SCCProvider=None, and success requires BOTH the completion marker and
  # changed .uasset hashes - which a marked-for-delete import cannot produce.
  # Waiting for an idle machine across eight working lanes is waiting for
  # something that does not happen.
  interactive=$(ps -Ao comm=,args= \
    | awk '$1 ~ /\/UnrealEditor$/ && $0 !~ /-game|-benchmark|-nullrhi|-run=|-ExecutePythonScript/' \
    | wc -l | tr -d ' ')
  if [[ "$interactive" != "0" ]]; then
    print -u2 "An interactive Unreal editor is open; it stages asset deletions"
    print -u2 "and the import would fail silently as marked-for-delete (SCC trap)."
    print -u2 "Close it and re-run."
    exit 3
  fi

  # Wait only for a REAL compile. Match the executable, never a command-line
  # substring: other lanes' monitoring shells carry "UnrealBuildTool.dll" and
  # "clang -cc1" in their own command lines, and blocking on those means
  # blocking on a phantom. This gate spent ten minutes waiting on a /bin/zsh
  # at 0.0%% CPU before that was caught - the third time tonight substring
  # matching produced a false holder.
  compiling=$(ps -Ao comm=,args= \
    | awk '$1 ~ /(dotnet|clang|clang\+\+|cc1)$/ && /UnrealBuildTool\.dll|-cc1/' \
    | wc -l | tr -d ' ')
  if [[ "$compiling" != "0" ]]; then
    print "attempt $attempt/$attempts: $compiling real compiler process(es) running; the"
    print "    UBT mutex would abort this run during startup. Waiting 20s."
    sleep 20
    continue
  fi
  # A concurrent build replaces the game module binary. If it is missing or
  # still being written, the editor loads EchoesSimCore, reports "The game
  # module 'EchoesOfTheBrokenSun' could not be found" and exits before running
  # the script - another clean-looking exit that does no work.
  module="$project_root/Binaries/Mac/libUnrealEditor-EchoesOfTheBrokenSun.dylib"
  if [[ ! -f "$module" ]]; then
    print "attempt $attempt/$attempts: game module binary absent (a build is mid-flight). Waiting 20s."
    sleep 20
    continue
  fi
  size1=$(stat -f%z "$module" 2>/dev/null || echo 0)
  sleep 2
  size2=$(stat -f%z "$module" 2>/dev/null || echo 0)
  if [[ "$size1" != "$size2" || "$size1" == "0" ]]; then
    print "attempt $attempt/$attempts: game module binary still being written. Waiting 20s."
    sleep 20
    continue
  fi

  others=$(ps -Ao comm= | grep -cE "/UnrealEditor$" || true)
  [[ "$others" != "0" ]] && print "attempt $attempt: proceeding alongside $others non-interactive editor process(es)"

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
  if grep -q "game module 'EchoesOfTheBrokenSun' could not be found" "$purge_log" 2>/dev/null; then
    print "attempt $attempt/$attempts: game module was rebuilt under us; retrying in 30s"
    sleep 30
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
