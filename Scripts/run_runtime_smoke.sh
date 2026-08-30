#!/bin/zsh
set -euo pipefail

project_root="${0:A:h:h}"
ue_root="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
editor="$ue_root/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
project="$project_root/EchoesOfTheBrokenSun.uproject"
local_faction="${ECHOES_LOCAL_FACTION:-Meridian}"
case "$local_faction" in
  Meridian|MeridianCompact)
    faction_args=()
    expected_faction_marker='\[ECHOES_FACTION_SCENARIO_READY\] local=MeridianCompact opposition=KharuunAssemblies selectable=true'
    ;;
  Kharuun|KharuunAssemblies)
    faction_args=(-EchoesFaction=Kharuun)
    expected_faction_marker='\[ECHOES_FACTION_SCENARIO_READY\] local=KharuunAssemblies opposition=MeridianCompact selectable=true'
    ;;
  *)
    print -u2 "Unsupported ECHOES_LOCAL_FACTION: $local_faction"
    exit 2
    ;;
esac
log="${ECHOES_RUNTIME_LOG:-$project_root/BuildArtifacts/RuntimeSmoke.log}"
if [[ "$log" != /* ]]; then
  log="$project_root/$log"
fi

if [[ ! -x "$editor" ]]; then
  print -u2 "Unreal Editor is not available at: $editor"
  exit 2
fi

mkdir -p "$project_root/BuildArtifacts"
: > "$log"

"$editor" "$project" /Engine/Maps/Entry \
  -game -unattended -nop4 -nosplash -nullrhi -nosound \
  "${faction_args[@]}" \
  -benchmark -fps=20 -benchmarkseconds=3 -AbsLog="$log"

if ! /usr/bin/grep -q '\[ECHOES_ENV_READY\]' "$log" ||
   ! /usr/bin/grep -q '\[ECHOES_CONTENT_READY\] packVersion=1 schema=2 factions=3 playable=3 units=12 buildings=12 technologies=6 sha256=0460f5e2fc180238fc71364af138cce3fe1943ef2942af19a66eb2cc1de356e1 source=canonical' "$log" ||
   ! /usr/bin/grep -q '\[ECHOES_SIM_RULES_READY\] version=2 sha256=0460f5e2fc180238fc71364af138cce3fe1943ef2942af19a66eb2cc1de356e1 rosterArchetypes=24 catalogUnits=12 catalogBuildings=12 technologies=6 research=authored futureWell=authored bulwarkDeployment=authored relaySupply=authored waystoneMigration=authored warformAdaptation=authored mineralCover=authored vibrationDetection=authored poweredAegis=authored choirIdentity=authored choirCoherence=authored' "$log" ||
   ! /usr/bin/grep -q '\[ECHOES_WEATHER_READY\] glassScarDrift=active reducedMotionAware=true finalArt=false' "$log" ||
   ! /usr/bin/grep -q '\[ECHOES_AUDIO_READY\] revision=presentation-audio-v1 cues=3 authored=true command2D=true destruction3D=true commandCooldownMs=80 destructionCooldownMs=140 runtimeAuthority=presentation thirdPartySamples=false finalAudio=false' "$log" ||
   ! /usr/bin/grep -q '\[ECHOES_SIM_READY\]' "$log" ||
   ! /usr/bin/grep -q "$expected_faction_marker" "$log" ||
   ! /usr/bin/grep -q '\[ECHOES_POWERED_AEGIS_READY\] powered=1 publicState=true networkCounterplay=true' "$log" ||
   ! /usr/bin/grep -q '\[ECHOES_GLASS_SCAR_READY\] blocked=165 crossings=3 centralWell=(32,32)' "$log" ||
   ! /usr/bin/grep -Eq '\[ECHOES_FOG_READY\] tiles=4096 visible=[1-9][0-9]* explored=[0-9]+ unexplored=[1-9][0-9]*' "$log" ||
   ! /usr/bin/grep -Eq '\[ECHOES_AI_PLAYER_VIEW\] player=[1-9][0-9]* owned=[1-9][0-9]* observed=[1-9][0-9]* hiddenEntitiesExcluded=true opponentInternalsRedacted=true authoritativeWorldHandle=false' "$log" ||
   ! /usr/bin/grep -Eq '\[ECHOES_AI_EXPANSION\] personality=adaptive actor=[1-9][0-9]* buildType=[1-9][0-9]* tile=\(-?[0-9]+,-?[0-9]+\) visibilityBounded=true' "$log" ||
   ! /usr/bin/grep -q '\[ECHOES_BOOT_READY\]' "$log" ||
   ! /usr/bin/grep -q '\[ECHOES_SIM_FIRST_TICK\]' "$log"; then
  print -u2 "Runtime smoke markers were incomplete. Inspect: $log"
  exit 3
fi

if /usr/bin/grep -Eq '\[ECHOES_BOOT_INCOMPLETE\]|\[ECHOES_BOOT_NO_SUBSYSTEM\]|\[ECHOES_CONTENT_FAILED\]|\[ECHOES_SIM_CONTENT_REJECTED\]|\[ECHOES_SIM_VIEW_SYNC_FAILED\]|\[ECHOES_AI_PLAYER_VIEW_FAILED\]|\[ECHOES_FOG_INIT_FAILED\]|\[ECHOES_TERRAIN_VIEW_INIT_FAILED\]|Fatal error:|Assertion failed:' "$log"; then
  print -u2 "Runtime smoke reported a boot or fatal failure. Inspect: $log"
  exit 4
fi

print "Runtime bootstrap passed for $local_faction: authored presentation-audio assets, environment, reduced-motion-aware Glass Scar atmosphere, terrain, 4,096-tile fog/shroud, adaptive visible-terrain AI expansion, 20 Hz simulation, and first fixed tick initialized."
print "Evidence log: $log"
