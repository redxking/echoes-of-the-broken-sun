#!/bin/zsh
# Re-exec under zsh if invoked as `bash Scripts/acquire_build_slot.sh` or `sh ...`.
# POSIX-only syntax, and it must stay above every zsh-specific construct below.
# Without it, bash errors on the ${(f)...} expansion and the print builtin, prints
# nothing useful and STILL EXITS 0 -- a checker that fails OPEN and reports a busy
# slot as free. Agent lanes type `bash Scripts/...` by habit, so this is not academic.
# shellcheck shell=sh
if [ -z "${ZSH_VERSION:-}" ]; then
  exec /bin/zsh "$0" "$@"
fi

# Inspect whether this Mac's single Unreal build slot is free.
# This is a preflight inspection, not an atomic reservation; coordinate ownership separately.
#
# Author and owner: Angelis Pseftis
#
# Exit 0 = slot free. Exit 1 = slot held (holders printed to stderr).
#
# Why this exists: `pgrep -f UnrealEditor` is not a usable check and produced three
# false positives in one session.
#   1. It matches the CHECKING process itself -- the shell and grep running the check
#      carry the pattern on their own command lines. A stuck ugrep reported the slot
#      busy for seven minutes with nothing building.
#   2. It matches every clang++ whose response-file path contains /UnrealEditor/,
#      which is most of a compile (86 KB of output in one case).
#   2b. It matches OTHER sessions' shell wrappers: a `/bin/zsh -c ...` running a
#      command that merely mentions RunUAT or UnrealBuildTool looks like a holder.
#      A false "busy" is not harmless here -- it would stall every lane behind a
#      phantom -- so command-interpreter wrappers are excluded and only the real
#      tool processes count.
#   3. It MISSES the case that actually blocks packaging: UnrealBuildTool holds a
#      GLOBAL MUTEX, so a plain compile blocks a cook even with no editor running.
#      That collision surfaces as UAT "Error_SDKNotFound" (exit 10), which sends you
#      chasing an SDK problem that does not exist. The real line in the log is
#      "Result: Failed (ConflictingInstance)".
#
# Detects, deliberately and by distinct signal:
#   - the editor, by exact process name
#   - UnrealBuildTool, by its .dll invocation (the mutex holder)
#   - UAT / BuildCookRun, by AutomationTool.dll or RunUAT
#   - an in-flight compile, by a compiler working under Intermediate/Build
set -uo pipefail

self=$$
parent=$PPID
holders=()

add() {  # add <label> <pid> <detail>
  [[ "$2" == "$self" || "$2" == "$parent" ]] && return 0
  holders+=("$1 pid=$2 $3")
}

# Obtain both inventories explicitly. Process substitution hides the producer's exit
# status; a denied ps previously produced an empty inventory and reported "free".
process_snapshot="$(ps -Ao pid=,command=)"
inspection_result=$?
if (( inspection_result != 0 )) || [[ -z "${process_snapshot//[[:space:]]/}" ]]; then
  print -u2 "build slot unknown: process inspection failed or returned no processes; refusing launch."
  exit 2
fi
editor_pids="$(pgrep -x UnrealEditor)"
inspection_result=$?
# pgrep exit 1 means no matches; every other nonzero result is an inspection error.
if (( inspection_result > 1 )); then
  print -u2 "build slot unknown: editor inspection failed; refusing launch."
  exit 2
fi

# 1. Editor, exact name only.
for pid in ${(f)editor_pids}; do
  [[ -n "$pid" ]] || continue
  stats="$(ps -o etime=,%cpu= -p "$pid" 2>/dev/null | tr -s ' ')"
  add "UnrealEditor" "$pid" "elapsed/cpu:${stats}"
done

# 2/3/4. Command-line signals, excluding this checker and its shell.
while read -r pid cmd; do
  [[ -z "${pid:-}" ]] && continue
  # Skip search and inspection tools: their ARGUMENTS carry the tool names, so a
  # grep/ugrep/rg looking for "UnrealBuildTool" is not a build. Same false-positive
  # family as the shell wrappers above; this class has now bitten three times, and a
  # false BUSY would stall every lane behind a holder that never exits.
  tool_name="${${cmd%% *}##*/}"
  case "$tool_name" in
    grep|ugrep|rg|ag|ack|egrep|fgrep|find|ps|awk|sed|xargs|tail|head|cat|less|watch)
      continue ;;
  esac
  case "$cmd" in
    *UnrealBuildTool.dll*)  add "UnrealBuildTool(mutex holder)" "$pid" "" ;;
    *AutomationTool.dll*|*RunUAT*)  add "UAT/BuildCookRun" "$pid" "" ;;
    *Intermediate/Build*)
      case "$cmd" in
        *clang*|*ld*|*libtool*) add "compile" "$pid" "" ;;
      esac ;;
  esac
done < <(print -r -- "$process_snapshot" \
           | grep -v "acquire_build_slot" \
           | grep -vE "^ *[0-9]+ +/bin/(zsh|bash|sh) -c ")


if (( ${#holders} > 0 )); then
  print -u2 "BUILD SLOT HELD (${#holders} holder(s)):"
  for h in "${holders[@]}"; do print -u2 "  $h"; done
  print -u2 "Do not start a build, cook, editor or packaged run. Wait, or coordinate."
  exit 1
fi
print "build slot free"
exit 0
