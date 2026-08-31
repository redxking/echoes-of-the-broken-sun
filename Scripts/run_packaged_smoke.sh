#!/bin/zsh
set -euo pipefail

project_root="${0:A:h:h}"

if (( $# < 1 || $# > 3 )); then
  print -u2 "Usage: $0 /path/to/EchoesOfTheBrokenSun.app [evidence-log] [isolated-runtime-state]"
  exit 2
fi

app="${1:A}"
binary="$app/Contents/MacOS/EchoesOfTheBrokenSun"
pak_dir="$app/Contents/UE/EchoesOfTheBrokenSun/Content/Paks"
log="${2:-$project_root/BuildArtifacts/PackagedRuntimeSmoke.log}"
runtime_state="${3:-${log:r}.runtime-state}"

if [[ "$log" != /* ]]; then
  log="$project_root/$log"
fi
if [[ "$runtime_state" != /* ]]; then
  runtime_state="$project_root/$runtime_state"
fi
runtime_state="${runtime_state:A}"

if [[ ! -x "$binary" || ! -d "$pak_dir" ]]; then
  print -u2 "The supplied application is not a self-contained Echoes package: $app"
  exit 3
fi

if ! /usr/bin/codesign --verify --deep --strict "$app"; then
  print -u2 "The package signature seal is invalid: $app"
  exit 4
fi
if [[ -e "$runtime_state" ]]; then
  print -u2 "Refusing to reuse packaged-smoke runtime state: $runtime_state"
  exit 4
fi

mkdir -p "${log:h}"
mkdir -p "$runtime_state/save-games" "$runtime_state/user-dir"
: > "$log"

"$binary" /Engine/Maps/Entry \
  -game -unattended -nop4 -nosplash -nullrhi -nosound \
  "-EchoesSaveGameDirectory=$runtime_state/save-games" \
  "-UserDir=$runtime_state/user-dir" \
  -stdout -FullStdOutLogOutput \
  -benchmark -fps=20 -benchmarkseconds=3 > "$log" 2>&1
print "[ECHOES_SMOKE_STORAGE_ISOLATED] saveGameDirectory=$runtime_state/save-games userDir=$runtime_state/user-dir" >> "$log"

for marker in ECHOES_AUDIO_READY ECHOES_ENV_READY ECHOES_WEATHER_READY ECHOES_SIM_READY ECHOES_GLASS_SCAR_READY ECHOES_FOG_READY ECHOES_BOOT_READY ECHOES_SIM_FIRST_TICK; do
  if ! /usr/bin/grep -q "\\[$marker\\]" "$log"; then
    print -u2 "Packaged runtime marker $marker was absent. Inspect: $log"
    exit 5
  fi
done

if ! /usr/bin/grep -Eq '\[ECHOES_SIM_READY\].*32 entities, 14 visible views, 20 Hz' "$log"; then
  print -u2 "The packaged scenario did not report the accepted initial state. Inspect: $log"
  exit 6
fi

if ! /usr/bin/grep -q '\[ECHOES_GLASS_SCAR_READY\] blocked=165 crossings=3 centralWell=(32,32)' "$log"; then
  print -u2 "The packaged Glass Scar terrain did not report the accepted layout. Inspect: $log"
  exit 7
fi

if ! /usr/bin/grep -Eq '\[ECHOES_FOG_READY\] tiles=4096 visible=[1-9][0-9]* explored=[0-9]+ unexplored=[1-9][0-9]*' "$log"; then
  print -u2 "The packaged fog/shroud surface did not report the accepted initial state. Inspect: $log"
  exit 8
fi

if /usr/bin/grep -Eq 'EnhancedInput user settings|\[ECHOES_BOOT_INCOMPLETE\]|\[ECHOES_BOOT_NO_SUBSYSTEM\]|\[ECHOES_SIM_VIEW_SYNC_FAILED\]|\[ECHOES_FOG_INIT_FAILED\]|\[ECHOES_TERRAIN_VIEW_INIT_FAILED\]|Fatal error:|Assertion failed:' "$log"; then
  print -u2 "Packaged runtime reported a rejected warning or failure. Inspect: $log"
  exit 9
fi

print "Packaged runtime passed: cooked presentation audio, content, reduced-motion-aware Glass Scar atmosphere, terrain, fog/shroud, simulation bootstrap, and first fixed tick initialized."
print "Evidence log: $log"
print "Isolated runtime state: $runtime_state"
