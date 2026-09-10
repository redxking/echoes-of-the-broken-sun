#!/bin/zsh
# Prove the batched module still compiles BEFORE committing to main.
#
# Author and owner: Angelis Pseftis
#
# Why this exists: a commit landed a `main` that did not compile, and four more
# commits landed on the broken tree before anyone could fix it. Both colliding
# definitions arrived in the SAME commit, so no cross-lane coordination could have
# caught it -- and a lane that compiles only its own file cannot see this class of
# error at all, because each file compiles fine alone. Unreal batches several test
# .cpp files into one Module.EchoesOfTheBrokenSun.N.cpp translation unit; file-scope
# helpers and constants in a bare `namespace {` merge and collide there.
#
# UBT fails on this in seconds. The alternative cost several lane-hours.
#
# Usage:   Scripts/precommit_build_check.sh
# Exit 0 = the batched module compiles, safe to commit.
# Exit 1 = it does not compile. Do not commit; fix or report first.
# Exit 2 = could not run the check (no build slot); nothing was proved either way.
set -uo pipefail

project_root="${0:A:h:h}"
cd "$project_root" || exit 2

wait_seconds="${ECHOES_PRECOMMIT_SLOT_WAIT:-300}"
waited=0
while ! "$project_root/Scripts/acquire_build_slot.sh" >/dev/null 2>&1; do
  if (( waited >= wait_seconds )); then
    print -u2 "precommit-build-check: build slot busy for ${wait_seconds}s; check NOT run."
    "$project_root/Scripts/acquire_build_slot.sh" 2>&1 | /usr/bin/sed 's/^/  /' >&2
    print -u2 "Nothing was proved. Re-run before committing, or coordinate for the slot."
    exit 2
  fi
  sleep 10
  (( waited += 10 ))
done

print "precommit-build-check: compiling the batched editor module..."
if "$project_root/Scripts/build_editor.sh"; then
  print "precommit-build-check: PASS - the batched module compiles; safe to commit."
  exit 0
fi

print -u2 ""
print -u2 "precommit-build-check: FAIL - do NOT commit this tree."
print -u2 "main would not compile, and every lane that pulls it is blocked until it is fixed."
print -u2 "If the failure names a redefinition or a shadowed variable inside a"
print -u2 "Module.EchoesOfTheBrokenSun.N.cpp unit, build_editor.sh has already explained it"
print -u2 "above: give each test file a unique NAMED namespace rather than a bare"
print -u2 "'namespace { }', including file-scope constants, which are the usual shadow root."
exit 1
