#!/bin/zsh
set -euo pipefail

project_root="${0:A:h:h}"
ue_root="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
editor="$ue_root/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
project="$project_root/EchoesOfTheBrokenSun.uproject"
log="${1:-$project_root/BuildArtifacts/StressRuntimeSmoke.log}"

if [[ "$log" != /* ]]; then
  log="$project_root/$log"
fi
if [[ ! -x "$editor" ]]; then
  print -u2 "Unreal Editor is not available at: $editor"
  exit 2
fi

mkdir -p "${log:h}"
: > "$log"

"$editor" "$project" /Engine/Maps/Entry \
  -game -unattended -nop4 -nosplash -nullrhi -nosound \
  -EchoesStress400 \
  -benchmark -fps=20 -benchmarkseconds=3 -AbsLog="$log"

for marker in ECHOES_CONTENT_READY ECHOES_ENV_READY ECHOES_WEATHER_READY ECHOES_GLASS_SCAR_READY ECHOES_FOG_READY \
  ECHOES_SIM_READY ECHOES_STRESS_ORDERS_READY ECHOES_STRESS_READY \
  ECHOES_BOOT_READY ECHOES_SIM_FIRST_TICK ECHOES_STRESS_COMBAT_ACTIVE; do
  if ! /usr/bin/grep -q "\\[$marker\\]" "$log"; then
    print -u2 "Stress runtime marker $marker was absent. Inspect: $log"
    exit 3
  fi
done

if ! /usr/bin/grep -q \
  '\[ECHOES_CONTENT_READY\] packVersion=1 schema=1 factions=3 playable=2 units=8 buildings=8 sha256=a5ac74b23b572f2db4baf1236738a7f09b682d0de91580336af1f5a57dbb5586 source=canonical' \
  "$log"; then
  print -u2 "Stress runtime did not load the reviewed canonical content digest. Inspect: $log"
  exit 4
fi

if ! /usr/bin/grep -q \
  '\[ECHOES_SIM_RULES_READY\] version=1 sha256=a5ac74b23b572f2db4baf1236738a7f09b682d0de91580336af1f5a57dbb5586 rosterArchetypes=16 catalogUnits=8 catalogBuildings=8 futureWell=authored bulwarkDeployment=authored' \
  "$log"; then
  print -u2 "Stress runtime did not install the authored deterministic rules. Inspect: $log"
  exit 4
fi

if ! /usr/bin/grep -q \
  '\[ECHOES_STRESS_READY\] units=400 teams=4 entities=401 visibleViews=401' \
  "$log"; then
  print -u2 "Stress runtime did not expose the accepted 400-unit/four-team view boundary. Inspect: $log"
  exit 5
fi

if ! /usr/bin/grep -q \
  '\[ECHOES_STRESS_ORDERS_READY\] attackMove=396 teams=4 executeTick=1' \
  "$log"; then
  print -u2 "Stress runtime did not queue the accepted four-team broad-order fixture. Inspect: $log"
  exit 6
fi

if /usr/bin/grep -Eq \
  '\[ECHOES_BOOT_INCOMPLETE\]|\[ECHOES_BOOT_NO_SUBSYSTEM\]|\[ECHOES_CONTENT_FAILED\]|\[ECHOES_SIM_CONTENT_REJECTED\]|\[ECHOES_SIM_VIEW_SYNC_FAILED\]|\[ECHOES_FOG_INIT_FAILED\]|\[ECHOES_TERRAIN_VIEW_INIT_FAILED\]|Fatal error:|Assertion failed:' \
  "$log"; then
  print -u2 "Stress runtime reported a boot or fatal failure. Inspect: $log"
  exit 7
fi

print "Stress runtime passed: 400 units across four teams produced 401 visibility-scoped entity views, queued 396 deterministic attack-move orders, and entered active combat."
print "Evidence log: $log"
