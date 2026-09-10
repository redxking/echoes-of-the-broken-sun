#!/bin/zsh
# Package the pushed tip of main in a throwaway worktree that cannot outlive the run.
#
# Author and owner: Angelis Pseftis
#
# The packaging procedure requires a detached clean worktree at pushed main, while the
# project forbids leaving worktrees behind -- ~14,000 lines of real work were once
# stranded in exactly these directories. The removal therefore runs from an EXIT trap,
# so an abort, a failed gate or an interrupt still cleans up. Removal is verified and
# the verification is written next to the run's evidence.
#
# It packages `origin/main`, never local HEAD: with several lanes committing, local HEAD
# routinely runs ahead of the pushed tip, and the packager requires
# HEAD == origin/main == live remote. Three packaging attempts died on exactly that.
set -uo pipefail

project_root="${0:A:h:h}"
cd "$project_root" || exit 1

worktree=""
cleanup() {
  [[ -z "$worktree" ]] && return 0
  git worktree remove --force "$worktree" >/dev/null 2>&1
  if git worktree list | /usr/bin/grep -q "$(basename "$worktree")" || [[ -e "$worktree" ]]; then
    print -u2 "WARNING: throwaway packaging worktree was not removed: $worktree"
    return 1
  fi
  print "throwaway worktree removed and verified absent: $worktree"
}
trap cleanup EXIT INT TERM

"$project_root/Scripts/acquire_build_slot.sh" || { print -u2 "build slot held; not starting."; exit 1; }

git fetch origin main -q 2>/dev/null
pushed="$(git rev-parse origin/main)"
worktree="$project_root/../Worktrees/pkg-throwaway-${pushed[1,7]}"
[[ -e "$worktree" ]] && git worktree remove --force "$worktree" >/dev/null 2>&1
git worktree add --detach "$worktree" "$pushed" >/dev/null 2>&1 || { print -u2 "worktree add failed"; exit 1; }

evidence="$project_root/BuildArtifacts/Evidence/wi1-package-$(date -u +%Y%m%dT%H%M%SZ)"
mkdir -p "$evidence"
{
  print "gate=package-from-pushed-main"
  print "started_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
  print "source_commit=$pushed"
  print "worktree=$worktree (throwaway; removed by EXIT trap in this run)"
  print "clean=$(git -C "$worktree" status --porcelain=v2 --untracked-files=no | wc -l | tr -d ' ') derived=$(git -C "$worktree" ls-files --others --ignored --exclude-standard | wc -l | tr -d ' ')"
} > "$evidence/run.log"

typeset -a unset_flags
for name in $(env | /usr/bin/sed -n 's/^\(GIT_[A-Za-z0-9_]*\)=.*/\1/p'); do unset_flags+=(-u "$name"); done
( cd "$worktree" && env "${unset_flags[@]}" ECHOES_BUILD_ARTIFACT_ROOT="$project_root/BuildArtifacts" \
    /bin/zsh "$worktree/Scripts/package_macos.sh" ) >> "$evidence/run.log" 2>&1
status=$?
print "exit_code=$status" >> "$evidence/run.log"
print "completed_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)" >> "$evidence/run.log"

# D1 requires the removal verification to be recorded. The package's own provenance JSON
# is digest-sealed by its sidecar, so appending to it would invalidate verification;
# the record therefore lives beside the run evidence instead.
cleanup | /usr/bin/tee -a "$evidence/run.log"
trap - EXIT INT TERM
print "evidence=$evidence"
exit $status
