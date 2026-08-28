#!/bin/zsh
set -u

project_root="${0:A:h:h}"
ue_root="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
minimum_free_gib=40
preferred_free_gib=60
failures=0

pass() { print "PASS  $1"; }
warn() { print "WARN  $1"; }
fail() { print "FAIL  $1"; failures=$((failures + 1)); }

print "Echoes of the Broken Sun — development environment check"
print "Project: $project_root"

arch_name="$(uname -m)"
if [[ "$arch_name" == "arm64" ]]; then
  pass "Native Apple Silicon host"
else
  fail "Expected arm64 host; found $arch_name"
fi

macos_version="$(sw_vers -productVersion)"
pass "macOS $macos_version detected"

free_kib="$(df -k /System/Volumes/Data 2>/dev/null | awk 'NR==2 {print $4}')"
if [[ -z "$free_kib" ]]; then
  free_kib="$(df -k / | awk 'NR==2 {print $4}')"
fi
free_gib=$((free_kib / 1024 / 1024))
if (( free_gib >= preferred_free_gib )); then
  pass "$free_gib GiB free (preferred working headroom: $preferred_free_gib GiB)"
elif (( free_gib >= minimum_free_gib )); then
  warn "$free_gib GiB free; builds may proceed, but restore at least $preferred_free_gib GiB before large imports or release packaging"
else
  fail "$free_gib GiB free; at least $minimum_free_gib GiB is required for this prototype build"
fi

if command -v git >/dev/null 2>&1; then
  pass "$(git --version)"
else
  fail "Git is not available"
fi

if command -v git-lfs >/dev/null 2>&1; then
  pass "$(git lfs version)"
else
  fail "Git LFS is not available"
fi

developer_dir="${DEVELOPER_DIR:-$(xcode-select -p 2>/dev/null)}"
if [[ "$developer_dir" == *"CommandLineTools"* || -z "$developer_dir" ]]; then
  fail "Full Xcode is not selected (developer directory: ${developer_dir:-unset})"
else
  xcode_version="$(DEVELOPER_DIR="$developer_dir" xcodebuild -version 2>/dev/null | paste -sd ' ' -)"
  pass "Full Xcode selected: $xcode_version"
fi

if [[ -n "$developer_dir" ]] && DEVELOPER_DIR="$developer_dir" xcrun metal -v >/dev/null 2>&1; then
  pass "Metal Toolchain is executable"
else
  fail "Metal Toolchain is missing or unavailable in the selected Xcode"
fi

if [[ -x "$ue_root/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor" ]]; then
  pass "Unreal Editor present at $ue_root"
else
  fail "Unreal Editor is incomplete or absent at $ue_root"
fi

if [[ -f "$project_root/EchoesOfTheBrokenSun.uproject" ]]; then
  pass "Project descriptor present"
else
  fail "Project descriptor missing"
fi

if (( failures > 0 )); then
  print "\nEnvironment check failed with $failures blocking condition(s)."
  exit 1
fi

print "\nEnvironment check passed."
