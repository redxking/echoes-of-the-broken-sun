#!/bin/zsh
set -euo pipefail

project_root="${0:A:h:h}"
ue_root="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
editor="$ue_root/Engine/Binaries/Mac/UnrealEditor-Cmd"
project="$project_root/EchoesOfTheBrokenSun.uproject"
generator="$project_root/Scripts/generate_audio_assets.py"
log="$project_root/Saved/Logs/AudioAssetGeneration.log"

if [[ ! -x "$editor" ]]; then
  print -u2 "Unreal command editor is not available at: $editor"
  exit 2
fi

mkdir -p "$project_root/Saved/Logs"

"$editor" "$project" \
  -unattended -nop4 -nosplash -nullrhi -NoSound \
  -ExecutePythonScript="$generator" \
  -abslog="$log"

if ! rg -q '\[ECHOES_AUDIO_READY\].*revision=presentation-audio-v1.*cues=4 sourceRate=48000 channels=1.*command2D=true destruction3D=true.*maxConcurrentBounded=true.*thirdPartySamples=false finalAudio=false' "$log"; then
  print -u2 "The presentation-audio asset audit did not pass."
  print -u2 "Inspect: $log"
  exit 3
fi

if rg -q 'LogPython: Error:|LogAudio: Error:|LogEditorAssetSubsystem: Error:|LogInterchangeEngine: Error:' "$log"; then
  print -u2 "The Unreal audio generator reported an error."
  print -u2 "Inspect: $log"
  exit 4
fi

print "Generated four original mono presentation-audio sources and imported SoundWave assets."
print "Evidence log: $log"
