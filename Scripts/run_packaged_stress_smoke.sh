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
log="${2:-$project_root/BuildArtifacts/PackagedStressRuntimeSmoke.log}"

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
  -EchoesStress400 -stdout -FullStdOutLogOutput \
  -benchmark -fps=20 -benchmarkseconds=3 > "$log" 2>&1

for marker in ECHOES_ENV_READY ECHOES_WEATHER_READY ECHOES_GLASS_SCAR_READY \
  ECHOES_FOG_READY ECHOES_STRESS_ORDERS_READY ECHOES_STRESS_READY \
  ECHOES_BOOT_READY ECHOES_SIM_FIRST_TICK ECHOES_STRESS_COMBAT_ACTIVE; do
  if ! /usr/bin/grep -q "\\[$marker\\]" "$log"; then
    print -u2 "Packaged stress marker $marker was absent. Inspect: $log"
    exit 5
  fi
done

if ! /usr/bin/grep -q \
  '\[ECHOES_STRESS_ORDERS_READY\] attackMove=396 teams=4 executeTick=1' \
  "$log" ||
   ! /usr/bin/grep -q \
  '\[ECHOES_STRESS_READY\] units=400 teams=4 entities=401 visibleViews=401' \
  "$log" ||
   ! /usr/bin/grep -Eq \
  '\[ECHOES_STRESS_COMBAT_ACTIVE\] tick=[0-9]+ damaged=[1-9][0-9]* destroyed=[0-9]+ remaining=[0-9]+ visibleViews=401' \
  "$log"; then
  print -u2 "The packaged stress fixture did not match its accepted scale, order, or active-damage boundary. Inspect: $log"
  exit 6
fi

if /usr/bin/grep -Eq \
  '\[ECHOES_AI_COMMAND_REJECTED\]|\[ECHOES_BOOT_INCOMPLETE\]|\[ECHOES_SIM_VIEW_SYNC_FAILED\]|Fatal error:|Assertion failed:|GPU Crashed' \
  "$log"; then
  print -u2 "The packaged stress fixture reported a rejected warning or failure. Inspect: $log"
  exit 7
fi

print "Packaged stress runtime passed: four teams, 401 views, 396 attack-move orders, procedural atmosphere, and active combat initialized."
print "Evidence log: $log"
