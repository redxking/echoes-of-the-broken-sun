#!/bin/zsh
set -euo pipefail

project_root="${0:A:h:h}"
ue_root="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
builder="$ue_root/Engine/Build/BatchFiles/Mac/Build.sh"
project="$project_root/EchoesOfTheBrokenSun.uproject"
max_parallel_actions="${ECHOES_MAX_PARALLEL_ACTIONS:-4}"

if [[ ! -x "$builder" ]]; then
  print -u2 "Unreal build script not found at: $builder"
  print -u2 "Complete UE 5.8 installation or set UE_ROOT."
  exit 2
fi

if [[ "$max_parallel_actions" != <-> || "$max_parallel_actions" -lt 1 ]]; then
  print -u2 "ECHOES_MAX_PARALLEL_ACTIONS must be a positive integer."
  exit 3
fi

"$project_root/Scripts/test_content.sh"

# Capture the build so a unity-batch collision can be named. Unreal compiles several
# test .cpp files into one Module.EchoesOfTheBrokenSun.N.cpp translation unit, so two
# files that each open a bare `namespace {` merge into ONE anonymous namespace. Every
# file then compiles alone but fails together, and the compiler reports "redefinition
# of <helper>" or "declaration shadows a variable in namespace '(anonymous)'" -- naming
# a symbol when the cause is two files sharing a helper name under unity batching.
# That message points away from the problem, which is how it costs twenty minutes.
build_log="$(/usr/bin/mktemp -t echoes-build-editor)"
cleanup_build_log() { [[ -n "${build_log:-}" && -f "$build_log" ]] && /bin/rm -f "$build_log"; }
trap cleanup_build_log EXIT INT TERM

set +e
"$builder" EchoesOfTheBrokenSunEditor Mac Development "$project" \
  -waitmutex -NoHotReloadFromIDE \
  -MaxParallelActions="$max_parallel_actions" 2>&1 | /usr/bin/tee "$build_log"
build_status=${pipestatus[1]}
set -e

if (( build_status != 0 )) &&
   /usr/bin/grep -qE "Module\.EchoesOfTheBrokenSun\.[0-9]+\.cpp" "$build_log" &&
   /usr/bin/grep -qE "redefinition of|declaration shadows a variable in namespace" "$build_log"; then
  print -u2 ""
  print -u2 "=== unity-build namespace collision ==="
  print -u2 "This is NOT a redefinition in any one file. Unreal batches several test .cpp"
  print -u2 "files into a single Module.EchoesOfTheBrokenSun.N.cpp translation unit, so two"
  print -u2 "files that each open a bare 'namespace {' share one anonymous namespace and"
  print -u2 "their identically named helpers collide. Each file compiles fine on its own."
  print -u2 ""
  print -u2 "Fix: give each file a unique NAMED namespace, e.g."
  print -u2 "  namespace EchoesTutorialProgressPersistenceTest { ... }"
  print -u2 "rather than a bare 'namespace { ... }'."
  print -u2 ""
  print -u2 "Colliding symbols and files reported by the compiler:"
  /usr/bin/grep -E "error: (redefinition of|declaration shadows)" "$build_log" \
    | /usr/bin/sed 's/^/  /' >&2
  print -u2 "======================================="
fi

exit $build_status
