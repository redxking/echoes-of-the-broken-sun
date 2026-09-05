#!/bin/zsh
# Author and owner: Angelis Pseftis
# Interactive Echoes Editor only. Do not use for cook, package, or commandlets.
set -euo pipefail

project_root="${0:A:h:h}"
ue_root="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
editor="$ue_root/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
project="$project_root/EchoesOfTheBrokenSun.uproject"
cache_root="${project_root:h}/LocalCache"
log_path="${ECHOES_EDITOR_LOG:-$project_root/Saved/Logs/Editor-MCP-$(date -u +%Y%m%dT%H%M%SZ).log}"

if [[ ! -f "$project" || ! -x "$editor" ]]; then
  print -u2 "Project or UE 5.8 Editor unavailable. Connect the archive drive and check UE_ROOT."
  exit 2
fi
for module_file in UnrealEditor.modules libUnrealEditor-EchoesOfTheBrokenSun.dylib libUnrealEditor-EchoesSimCore.dylib; do
  if [[ ! -f "$project_root/Binaries/Mac/$module_file" ]]; then
    print -u2 "Missing $module_file. Build the editor first with Scripts/build_editor.sh."
    exit 2
  fi
done
if pgrep -x UnrealEditor >/dev/null || lsof -nP -iTCP:8000 -sTCP:LISTEN >/dev/null 2>&1; then
  print -u2 "An Editor or port 8000 listener is already running. Inspect that session before launching another."
  exit 3
fi

mkdir -p "$cache_root" "${log_path:h}"
print "Opening external project: $project"
print "Derived data cache: $cache_root"
print "Editor log: $log_path"
exec "$editor" "$project" \
  -ModelContextProtocolStartServer -ModelContextProtocolPort=8000 \
  "-LocalDataCachePath=$cache_root" "-abslog=$log_path"
