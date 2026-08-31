#!/bin/zsh
set -euo pipefail

project_root="${0:A:h:h}"
ue_root="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
uat="$ue_root/Engine/Build/BatchFiles/RunUAT.command"
project="$project_root/EchoesOfTheBrokenSun.uproject"
expected_version="$(/usr/bin/awk -F= '$1 == "ProjectVersion" { print $2; exit }' "$project_root/Config/DefaultGame.ini")"
timestamp="$(date -u +%Y%m%dT%H%M%SZ)"
archive_dir="${1:-$project_root/BuildArtifacts/Packages/Mac-Development-$timestamp}"
source_commit="$(git -C "$project_root" rev-parse --verify HEAD 2>/dev/null || print unknown)"
origin_commit="$(git -C "$project_root" rev-parse --verify origin/main 2>/dev/null || print unknown)"
max_parallel_actions="${ECHOES_MAX_PARALLEL_ACTIONS:-4}"

read_remote_main() {
  local remote_record
  remote_record="$(GIT_TERMINAL_PROMPT=0 git -C "$project_root" ls-remote --exit-code origin refs/heads/main 2>/dev/null)" || return 1
  print "${remote_record%%[[:space:]]*}"
}

remote_commit="$(read_remote_main || print unknown)"

verify_clean_pushed_source() {
  local phase="$1"
  local observed_commit observed_origin observed_remote observed_status
  observed_commit="$(git -C "$project_root" rev-parse --verify HEAD 2>/dev/null || print unknown)"
  observed_origin="$(git -C "$project_root" rev-parse --verify origin/main 2>/dev/null || print unknown)"
  observed_remote="$(read_remote_main || print unknown)"
  observed_status="$(git -C "$project_root" status --porcelain --untracked-files=normal 2>/dev/null || print status-unavailable)"
  if [[ "$source_commit" == unknown || "$origin_commit" == unknown ||
        "$remote_commit" == unknown ||
        "$observed_commit" != "$source_commit" ||
        "$observed_origin" != "$source_commit" ||
        "$observed_remote" != "$source_commit" ||
        -n "$observed_status" ]]; then
    print -u2 "Packaging requires an unchanged clean checkout at pushed origin/main ($phase)."
    print -u2 "Expected source commit: $source_commit; observed HEAD: $observed_commit; observed origin/main: $observed_origin; live remote main: $observed_remote."
    exit 9
  fi
}

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

verify_clean_pushed_source "before build"

if [[ -e "$archive_dir" ]]; then
  print -u2 "Refusing to mix a new package with an existing archive: $archive_dir"
  print -u2 "Choose a new archive path. Existing artifacts were left untouched."
  exit 3
fi

"$project_root/Scripts/test_content.sh"
"$project_root/Scripts/check_environment.sh"

mkdir -p "${archive_dir:h}"
archive_free_kib="$(df -k "${archive_dir:h}" | awk 'NR==2 {print $4}')"
archive_free_gib=$((archive_free_kib / 1024 / 1024))
if (( archive_free_gib < 60 )); then
  print -u2 "Packaging requires 60 GiB of headroom on the archive destination filesystem; only $archive_free_gib GiB is available at ${archive_dir:h}."
  exit 4
fi
internal_free_kib="$(df -k /System/Volumes/Data 2>/dev/null | awk 'NR==2 {print $4}')"
if [[ -z "$internal_free_kib" ]]; then
  internal_free_kib="$(df -k / | awk 'NR==2 {print $4}')"
fi
internal_free_gib=$((internal_free_kib / 1024 / 1024))
if (( internal_free_gib < 60 )); then
  print -u2 "Packaging requires 60 GiB of internal headroom for Unreal, temporary, and cache activity; only $internal_free_gib GiB is available."
  exit 4
fi

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

verify_clean_pushed_source "after build"

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

smoke_log="$archive_dir/EchoesOfTheBrokenSun.normal-startup-smoke.log"
smoke_state="$archive_dir/SmokeRuntimeState/Normal"
"$project_root/Scripts/run_packaged_smoke.sh" "$app" "$smoke_log" "$smoke_state"
stress_smoke_log="$archive_dir/EchoesOfTheBrokenSun.legacy-stress-startup-smoke.log"
stress_smoke_state="$archive_dir/SmokeRuntimeState/LegacyStress"
"$project_root/Scripts/run_packaged_stress_smoke.sh" "$app" "$stress_smoke_log" "$stress_smoke_state"
smoke_log_sha256="$(/usr/bin/shasum -a 256 "$smoke_log" | /usr/bin/awk '{print $1}')"
stress_smoke_log_sha256="$(/usr/bin/shasum -a 256 "$stress_smoke_log" | /usr/bin/awk '{print $1}')"

verify_clean_pushed_source "before manifest"

engine_build_file="$ue_root/Engine/Build/Build.version"
engine_version="$(/usr/bin/plutil -extract MajorVersion raw "$engine_build_file").$(/usr/bin/plutil -extract MinorVersion raw "$engine_build_file").$(/usr/bin/plutil -extract PatchVersion raw "$engine_build_file")"
xcode_version="$(xcodebuild -version | paste -sd ';' -)"
manifest="$archive_dir/EchoesOfTheBrokenSun.manifest.txt"
manifest_digest="$archive_dir/EchoesOfTheBrokenSun.manifest.sha256"

{
  print "artifact=EchoesOfTheBrokenSun.app"
  print "created_utc=$timestamp"
  print "source_commit=$source_commit"
  print "origin_main=$origin_commit"
  print "remote_main=$remote_commit"
  print "source_tree=clean"
  print "source_binding=clean-pushed-main"
  print "configuration=Development"
  print "platform=Mac-arm64"
  print "normal_startup_smoke=${smoke_log:t}"
  print "normal_startup_smoke_sha256=$smoke_log_sha256"
  print "legacy_stress_startup_smoke=${stress_smoke_log:t}"
  print "legacy_stress_startup_smoke_sha256=$stress_smoke_log_sha256"
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

verify_clean_pushed_source "after manifest"
/usr/bin/codesign --verify --deep --strict "$app"
/usr/bin/python3 "$project_root/Scripts/verify_packaged_app.py" \
  --app "$app" --manifest "$manifest" --manifest-digest "$manifest_digest" \
  >/dev/null
(cd "$archive_dir" && /usr/bin/shasum -a 256 -c "${manifest_digest:t}" >/dev/null)

print "Clean-source incremental Mac Development package passed structural, signature, startup, and exact-manifest checks."
print "Application: $app"
print "Content manifest: $manifest"
print "Manifest SHA-256: $manifest_hash"
