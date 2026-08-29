#!/bin/zsh
set -euo pipefail

project_root="${0:A:h:h}"

if (( $# < 1 || $# > 2 )); then
  print -u2 "Usage: $0 /path/to/EchoesOfTheBrokenSun.app [evidence-log]"
  exit 2
fi

app="${1:A}"
binary="$app/Contents/MacOS/EchoesOfTheBrokenSun"
pak_dir="$app/Contents/UE/EchoesOfTheBrokenSun/Content/Paks"
log="${2:-$project_root/BuildArtifacts/PackagedRuntimeSmoke.log}"

if [[ "$log" != /* ]]; then
  log="$project_root/$log"
fi

if [[ ! -x "$binary" || ! -d "$pak_dir" ]]; then
  print -u2 "The supplied application is not a self-contained Echoes package: $app"
  exit 3
fi

if ! /usr/bin/codesign --verify --deep --strict "$app"; then
  print -u2 "The package signature seal is invalid: $app"
  exit 4
fi

mkdir -p "${log:h}"
: > "$log"

"$binary" /Engine/Maps/Entry \
  -game -unattended -nop4 -nosplash -nullrhi -nosound \
  -stdout -FullStdOutLogOutput \
  -benchmark -fps=20 -benchmarkseconds=3 > "$log" 2>&1

for marker in ECHOES_ENV_READY ECHOES_WEATHER_READY ECHOES_SIM_READY ECHOES_GLASS_SCAR_READY ECHOES_FOG_READY ECHOES_BOOT_READY ECHOES_SIM_FIRST_TICK; do
  if ! /usr/bin/grep -q "\\[$marker\\]" "$log"; then
    print -u2 "Packaged runtime marker $marker was absent. Inspect: $log"
    exit 5
  fi
done

if ! /usr/bin/grep -Eq '\[ECHOES_SIM_READY\].*25 entities, 10 visible views, 20 Hz' "$log"; then
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

print "Packaged runtime passed: cooked content, reduced-motion-aware Glass Scar atmosphere, terrain, fog/shroud, simulation bootstrap, and first fixed tick initialized."
print "Evidence log: $log"
