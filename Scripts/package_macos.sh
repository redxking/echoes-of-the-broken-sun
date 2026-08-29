#!/bin/zsh
set -euo pipefail

project_root="${0:A:h:h}"
ue_root="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
uat="$ue_root/Engine/Build/BatchFiles/RunUAT.command"
project="$project_root/EchoesOfTheBrokenSun.uproject"
expected_version="$(/usr/bin/awk -F= '$1 == "ProjectVersion" { print $2; exit }' "$project_root/Config/DefaultGame.ini")"
timestamp="$(date -u +%Y%m%dT%H%M%SZ)"
archive_dir="${1:-$project_root/BuildArtifacts/Packages/Mac-Development-$timestamp}"
source_commit="$(git -C "$project_root" rev-parse HEAD 2>/dev/null || print unknown)"
source_state=clean
max_parallel_actions="${ECHOES_MAX_PARALLEL_ACTIONS:-4}"
if [[ -n "$(git -C "$project_root" status --porcelain --untracked-files=normal 2>/dev/null)" ]]; then
  source_state=dirty
fi

if [[ "$archive_dir" != /* ]]; then
  archive_dir="$project_root/$archive_dir"
fi
archive_dir="${archive_dir:A}"

if [[ ! -x "$uat" ]]; then
  print -u2 "Unreal Automation Tool is not available at: $uat"
  exit 2
fi

if [[ -z "$expected_version" ]]; then
  print -u2 "ProjectVersion is missing from Config/DefaultGame.ini."
  exit 2
fi

if [[ "$max_parallel_actions" != <-> || "$max_parallel_actions" -lt 1 ]]; then
  print -u2 "ECHOES_MAX_PARALLEL_ACTIONS must be a positive integer."
  exit 2
fi

if [[ -e "$archive_dir" ]]; then
  print -u2 "Refusing to mix a new package with an existing archive: $archive_dir"
  print -u2 "Choose a new archive path. Existing artifacts were left untouched."
  exit 3
fi

"$project_root/Scripts/check_environment.sh"

free_kib="$(df -k /System/Volumes/Data 2>/dev/null | awk 'NR==2 {print $4}')"
if [[ -z "$free_kib" ]]; then
  free_kib="$(df -k / | awk 'NR==2 {print $4}')"
fi
free_gib=$((free_kib / 1024 / 1024))
if (( free_gib < 60 )); then
  print -u2 "Packaging requires the project's preferred 60 GiB free-space headroom; only $free_gib GiB is available."
  exit 4
fi

mkdir -p "${archive_dir:h}"

"$uat" BuildCookRun \
  -project="$project" \
  -noP4 \
  -platform=Mac \
  -target=EchoesOfTheBrokenSun \
  -clientconfig=Development \
  -ubtargs="-MaxParallelActions=$max_parallel_actions" \
  -build -cook -stage -pak -package -archive \
  -archivedirectory="$archive_dir" \
  -utf8output

app="$archive_dir/EchoesOfTheBrokenSun.app"
binary="$app/Contents/MacOS/EchoesOfTheBrokenSun"
pak_dir="$app/Contents/UE/EchoesOfTheBrokenSun/Content/Paks"
plist="$app/Contents/Info.plist"
entitlements="$project_root/Build/Mac/Resources/Sandbox.Server.entitlements"

if [[ ! -x "$binary" || ! -d "$pak_dir" ]]; then
  print -u2 "Unreal reported success but did not produce a self-contained application: $app"
  exit 5
fi

pak_count="$(find "$pak_dir" -maxdepth 1 -type f | wc -l | tr -d ' ')"
if (( pak_count < 5 )); then
  print -u2 "The package contains only $pak_count cooked container files; expected at least 5."
  exit 6
fi

if [[ "$(/usr/libexec/PlistBuddy -c 'Print :CFBundleIdentifier' "$plist")" != "com.angelispseftis.echoesofthebrokensun" ||
      "$(/usr/libexec/PlistBuddy -c 'Print :CFBundleShortVersionString' "$plist")" != "$expected_version" ]]; then
  print -u2 "The package identity or version does not match the configured $expected_version development baseline."
  exit 7
fi

if ! /usr/bin/file "$binary" | /usr/bin/grep -q 'Mach-O 64-bit executable arm64'; then
  print -u2 "The packaged executable is not the required native arm64 build."
  exit 8
fi

# Unreal's Mac packaging flow can update cooked containers after its Xcode signing
# phase. Seal the completed local development archive after all content is final.
# This remains an ad-hoc signature; it is not Developer ID signing or notarization.
/usr/bin/codesign --force --deep --sign - \
  --entitlements "$entitlements" --timestamp=none "$app"
/usr/bin/codesign --verify --deep --strict --verbose=2 "$app"

smoke_log="$project_root/BuildArtifacts/PackagedRuntimeSmoke-$timestamp.log"
"$project_root/Scripts/run_packaged_smoke.sh" "$app" "$smoke_log"

engine_build_file="$ue_root/Engine/Build/Build.version"
engine_version="$(/usr/bin/plutil -extract MajorVersion raw "$engine_build_file").$(/usr/bin/plutil -extract MinorVersion raw "$engine_build_file").$(/usr/bin/plutil -extract PatchVersion raw "$engine_build_file")"
xcode_version="$(xcodebuild -version | paste -sd ';' -)"
manifest="$archive_dir/EchoesOfTheBrokenSun.manifest.txt"
manifest_digest="$archive_dir/EchoesOfTheBrokenSun.manifest.sha256"

{
  print "artifact=EchoesOfTheBrokenSun.app"
  print "created_utc=$timestamp"
  print "source_commit=$source_commit"
  print "source_tree=$source_state"
  print "configuration=Development"
  print "platform=Mac-arm64"
  print "unreal_engine=$engine_version"
  print "xcode=$xcode_version"
  print "bundle_identifier=$(/usr/libexec/PlistBuddy -c 'Print :CFBundleIdentifier' "$plist")"
  print "bundle_short_version=$(/usr/libexec/PlistBuddy -c 'Print :CFBundleShortVersionString' "$plist")"
  print ""
  print "sha256  relative_path"
  LC_ALL=C /usr/bin/find "$app/Contents" -type f -print |
    LC_ALL=C /usr/bin/sort |
    while IFS= read -r packaged_file; do
      relative_path="${packaged_file#$app/}"
      file_hash="$(/usr/bin/shasum -a 256 "$packaged_file" | awk '{print $1}')"
      print "$file_hash  $relative_path"
    done
  LC_ALL=C /usr/bin/find "$app/Contents" -type l -print |
    LC_ALL=C /usr/bin/sort |
    while IFS= read -r packaged_link; do
      relative_path="${packaged_link#$app/}"
      print "SYMLINK  $relative_path -> $(readlink "$packaged_link")"
    done
} > "$manifest"

manifest_hash="$(/usr/bin/shasum -a 256 "$manifest" | awk '{print $1}')"
print "$manifest_hash  ${manifest:t}" > "$manifest_digest"

print "Mac Development package passed structural, signature, and startup checks."
print "Application: $app"
print "Content manifest: $manifest"
print "Manifest SHA-256: $manifest_hash"
