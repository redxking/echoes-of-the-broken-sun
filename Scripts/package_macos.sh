#!/bin/zsh
set -euo pipefail

project_root="${0:A:h:h}"
git_path="$(command -v git)"
if [[ -z "$git_path" || ! -x "$git_path" ]]; then
  print -u2 "Git is unavailable."
  exit 2
fi
git_version="$($git_path --version)"
git_sha256="$(/usr/bin/shasum -a 256 "$git_path" | /usr/bin/awk '{print $1}')"
git() {
  "$git_path" "$@"
}
approved_ue_root="/Users/Shared/Epic Games/UE_5.8"
approved_ue_root="${approved_ue_root:A}"
approved_developer_dir="/Applications/Xcode.app/Contents/Developer"
approved_developer_dir="${approved_developer_dir:A}"
developer_dir="${DEVELOPER_DIR:-$(/usr/bin/xcode-select -p)}"
developer_dir="${developer_dir:A}"
if [[ ! -d "$developer_dir" || "$developer_dir" != "$approved_developer_dir" ]]; then
  print -u2 "Packaging is authorized only with the verified Xcode installation at: $approved_developer_dir"
  exit 2
fi
export DEVELOPER_DIR="$approved_developer_dir"
ue_root="${UE_ROOT:-$approved_ue_root}"
ue_root="${ue_root:A}"
uat="$ue_root/Engine/Build/BatchFiles/RunUAT.command"
uat_driver="$ue_root/Engine/Build/BatchFiles/RunUAT.sh"
engine_build_file="$ue_root/Engine/Build/Build.version"
project="$project_root/EchoesOfTheBrokenSun.uproject"
expected_version="$(/usr/bin/awk -F= '$1 == "ProjectVersion" { print $2; exit }' "$project_root/Config/DefaultGame.ini")"
timestamp="$(date -u +%Y%m%dT%H%M%SZ)"
source_commit="$(git -C "$project_root" rev-parse --verify HEAD 2>/dev/null || print unknown)"
source_tree_hash="$(git -C "$project_root" rev-parse --verify 'HEAD^{tree}' 2>/dev/null || print unknown)"
source_branch="$(git -C "$project_root" symbolic-ref --quiet --short HEAD 2>/dev/null || print detached)"
origin_commit="$(git -C "$project_root" rev-parse --verify origin/main 2>/dev/null || print unknown)"
source_status_sha256="$(git -C "$project_root" status --porcelain=v2 -z 2>/dev/null | /usr/bin/shasum -a 256 | /usr/bin/awk '{print $1}')"
git_lfs_version="$(git -C "$project_root" lfs version 2>/dev/null || print unavailable)"
git_lfs_status_sha256="$(git -C "$project_root" lfs status --porcelain 2>/dev/null | /usr/bin/shasum -a 256 | /usr/bin/awk '{print $1}')"
max_parallel_actions="${ECHOES_MAX_PARALLEL_ACTIONS:-4}"
git_common_dir="$(git -C "$project_root" rev-parse --path-format=absolute --git-common-dir 2>/dev/null || print "$project_root/.git")"
git_common_dir="${git_common_dir:A}"
repository_checkout="${git_common_dir:h}"
artifact_root_default="${repository_checkout:h}/BuildArtifacts"
artifact_root="${ECHOES_BUILD_ARTIFACT_ROOT:-$artifact_root_default}"
if [[ "$artifact_root" != /* ]]; then
  artifact_root="${project_root:h}/$artifact_root"
fi
artifact_root="${artifact_root:A}"
source_short="${source_commit[1,8]}"
archive_dir="${1:-$artifact_root/Packages/Mac-Development-$timestamp-$source_short}"

read_remote_main() {
  local remote_record
  remote_record="$(GIT_TERMINAL_PROMPT=0 git -C "$project_root" ls-remote --exit-code origin refs/heads/main 2>/dev/null)" || return 1
  print "${remote_record%%[[:space:]]*}"
}

remote_commit="$(read_remote_main || print unknown)"

verify_clean_pushed_source() {
  local phase="$1"
  local observed_commit observed_tree observed_origin observed_remote
  local observed_status observed_status_sha256 observed_lfs_status observed_lfs_status_sha256
  observed_commit="$(git -C "$project_root" rev-parse --verify HEAD 2>/dev/null || print unknown)"
  observed_tree="$(git -C "$project_root" rev-parse --verify 'HEAD^{tree}' 2>/dev/null || print unknown)"
  observed_origin="$(git -C "$project_root" rev-parse --verify origin/main 2>/dev/null || print unknown)"
  observed_remote="$(read_remote_main || print unknown)"
  observed_status="$(git -C "$project_root" status --porcelain --untracked-files=normal 2>/dev/null || print status-unavailable)"
  observed_status_sha256="$(git -C "$project_root" status --porcelain=v2 -z 2>/dev/null | /usr/bin/shasum -a 256 | /usr/bin/awk '{print $1}')"
  observed_lfs_status="$(git -C "$project_root" lfs status --porcelain 2>/dev/null || print lfs-status-unavailable)"
  observed_lfs_status_sha256="$(git -C "$project_root" lfs status --porcelain 2>/dev/null | /usr/bin/shasum -a 256 | /usr/bin/awk '{print $1}')"
  if [[ "$source_commit" == unknown || "$source_tree_hash" == unknown ||
        "$origin_commit" == unknown ||
        "$remote_commit" == unknown ||
        "$observed_commit" != "$source_commit" ||
        "$observed_tree" != "$source_tree_hash" ||
        "$observed_origin" != "$source_commit" ||
        "$observed_remote" != "$source_commit" ||
        -n "$observed_status" ||
        -n "$observed_lfs_status" ||
        "$observed_status_sha256" != "$source_status_sha256" ||
        "$observed_lfs_status_sha256" != "$git_lfs_status_sha256" ]]; then
    print -u2 "Packaging requires an unchanged clean checkout at pushed origin/main ($phase)."
    print -u2 "Expected source commit/tree: $source_commit/$source_tree_hash; observed HEAD/tree: $observed_commit/$observed_tree."
    print -u2 "Observed origin/main: $observed_origin; live remote main: $observed_remote; Git LFS status must also be clean."
    exit 9
  fi
}

if [[ "$archive_dir" != /* ]]; then
  archive_dir="$artifact_root/$archive_dir"
fi
archive_dir="${archive_dir:A}"

if [[ "$archive_dir" == "$project_root" || "$archive_dir" == "$project_root"/* ]]; then
  print -u2 "Package archives must be written outside the working checkout: $archive_dir"
  exit 3
fi

if [[ "$ue_root" != "$approved_ue_root" ]]; then
  print -u2 "Packaging is authorized only with the verified UE 5.8.2 installation at: $approved_ue_root"
  exit 2
fi

if [[ ! -x "$uat" || ! -x "$uat_driver" || ! -f "$engine_build_file" ]]; then
  print -u2 "The approved Unreal Automation Tool or build identity is unavailable under: $ue_root"
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

engine_version="$(/usr/bin/plutil -extract MajorVersion raw "$engine_build_file").$(/usr/bin/plutil -extract MinorVersion raw "$engine_build_file").$(/usr/bin/plutil -extract PatchVersion raw "$engine_build_file")"
engine_changelist="$(/usr/bin/plutil -extract Changelist raw "$engine_build_file")"
engine_branch="$(/usr/bin/plutil -extract BranchName raw "$engine_build_file")"
engine_promoted="$(/usr/bin/plutil -extract IsPromotedBuild raw "$engine_build_file")"
if [[ "$engine_version" != 5.8.2 || "$engine_promoted" != 1 ]]; then
  print -u2 "Packaging requires the approved promoted Unreal Engine 5.8.2 toolchain; observed $engine_version (promoted=$engine_promoted)."
  exit 2
fi
engine_build_file_sha256="$(/usr/bin/shasum -a 256 "$engine_build_file" | /usr/bin/awk '{print $1}')"
uat_sha256="$(/usr/bin/shasum -a 256 "$uat" | /usr/bin/awk '{print $1}')"
uat_driver_sha256="$(/usr/bin/shasum -a 256 "$uat_driver" | /usr/bin/awk '{print $1}')"
xcode_version="$(/usr/bin/xcodebuild -version | /usr/bin/paste -sd ';' -)"
if ! /usr/bin/xcodebuild -checkFirstLaunchStatus >/dev/null; then
  print -u2 "The selected Xcode installation has not completed first-launch setup."
  exit 2
fi
sdk_path="$(/usr/bin/xcrun --sdk macosx --show-sdk-path)"
sdk_version="$(/usr/bin/xcrun --sdk macosx --show-sdk-version)"
clang_path="$(/usr/bin/xcrun --find clang)"
clang_version="$(/usr/bin/xcrun clang --version | /usr/bin/sed -n '1p')"
clang_sha256="$(/usr/bin/shasum -a 256 "$clang_path" | /usr/bin/awk '{print $1}')"
metal_path="$(/usr/bin/xcrun --find metal)"
metal_version="$(/usr/bin/xcrun metal --version 2>&1 | /usr/bin/sed -n '1p')"
metal_sha256="$(/usr/bin/shasum -a 256 "$metal_path" | /usr/bin/awk '{print $1}')"
host_os_version="$(/usr/bin/sw_vers -productVersion)"
host_os_build="$(/usr/bin/sw_vers -buildVersion)"
host_arch="$(/usr/bin/uname -m)"
host_model="$(/usr/sbin/sysctl -n hw.model)"
if [[ "$host_arch" != arm64 ]]; then
  print -u2 "The Mac package target requires an arm64 build host; observed $host_arch."
  exit 2
fi

verify_clean_pushed_source "before build"

preflight_log_pending="$archive_dir.package-preflight.log"
build_log_pending="$archive_dir.BuildCookRun.log"
if [[ -e "$archive_dir" || -e "$preflight_log_pending" || -e "$build_log_pending" ]]; then
  print -u2 "Refusing to mix a new package with an existing archive: $archive_dir"
  print -u2 "Choose a new archive path. Existing artifacts and failed-run logs were left untouched."
  exit 3
fi

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

if ! git_lfs_fsck_output="$(git -C "$project_root" lfs fsck 2>&1)"; then
  print -u2 "$git_lfs_fsck_output"
  print -u2 "Git LFS fsck failed; packaging was not started."
  exit 9
fi

{
  print "source_commit=$source_commit"
  print "source_tree_hash=$source_tree_hash"
  print "source_branch=$source_branch"
  print "source_status_sha256=$source_status_sha256"
  print "git_lfs_version=$git_lfs_version"
  print "git_lfs_status_sha256=$git_lfs_status_sha256"
  print "unreal_engine=$engine_version"
  print "unreal_changelist=$engine_changelist"
  print "xcode=$xcode_version"
  print "macos_sdk_version=$sdk_version"
  print "clang_version=$clang_version"
  print "metal_version=$metal_version"
  print -r -- "$git_lfs_fsck_output"
  "$project_root/Scripts/test_content.sh"
  "$project_root/Scripts/check_environment.sh"
} 2>&1 | /usr/bin/tee "$preflight_log_pending"

verify_clean_pushed_source "after preflight"
generated_content_pack="$project_root/Content/Data/Generated/EchoesContentPack.json"
generated_content_pack_digest="$generated_content_pack.sha256"
ignored_cook_inputs="$(git -C "$project_root" ls-files --others --ignored --exclude-standard -- Content/Data/Generated)"
expected_ignored_cook_inputs=$'Content/Data/Generated/EchoesContentPack.json\nContent/Data/Generated/EchoesContentPack.json.sha256'
if [[ ! -f "$generated_content_pack" || ! -f "$generated_content_pack_digest" ||
      "$ignored_cook_inputs" != "$expected_ignored_cook_inputs" ]]; then
  print -u2 "The content preflight must produce exactly EchoesContentPack.json and its SHA-256 sidecar as ignored cook inputs."
  exit 9
fi
generated_content_pack_sha256="$(/usr/bin/shasum -a 256 "$generated_content_pack" | /usr/bin/awk '{print $1}')"
generated_content_pack_declared_sha256="$(/usr/bin/tr -d '\r\n' < "$generated_content_pack_digest")"
if [[ "$generated_content_pack_declared_sha256" != "$generated_content_pack_sha256" ]]; then
  print -u2 "The generated content SHA-256 sidecar does not match EchoesContentPack.json."
  exit 9
fi

build_command=(
  "$uat" BuildCookRun
  "-project=$project"
  -noP4
  -platform=Mac
  -target=EchoesOfTheBrokenSun
  -clientconfig=Development
  "-ubtargs=-MaxParallelActions=$max_parallel_actions"
  -build -cook -stage -pak -package -archive
  "-archivedirectory=$archive_dir"
  -utf8output
)
build_command_argv="$(
  /usr/bin/printf '%s\0' "${build_command[@]}" | /usr/bin/python3 -c \
    'import json, sys; print(json.dumps([part.decode("utf-8") for part in sys.stdin.buffer.read().split(b"\0") if part], separators=(",", ":")))'
)"
"${build_command[@]}" 2>&1 | /usr/bin/tee "$build_log_pending"

verify_clean_pushed_source "after build"
generated_content_pack_after_build_sha256="$(/usr/bin/shasum -a 256 "$generated_content_pack" | /usr/bin/awk '{print $1}')"
generated_content_pack_after_build_declared_sha256="$(/usr/bin/tr -d '\r\n' < "$generated_content_pack_digest")"
if [[ "$generated_content_pack_after_build_sha256" != "$generated_content_pack_sha256" ||
      "$generated_content_pack_after_build_declared_sha256" != "$generated_content_pack_sha256" ]]; then
  print -u2 "The generated cooked content input changed during BuildCookRun."
  exit 9
fi

app="$archive_dir/EchoesOfTheBrokenSun.app"
binary="$app/Contents/MacOS/EchoesOfTheBrokenSun"
pak_dir="$app/Contents/UE/EchoesOfTheBrokenSun/Content/Paks"
plist="$app/Contents/Info.plist"
entitlements="$project_root/Build/Mac/Resources/Sandbox.Server.entitlements"

if [[ ! -x "$binary" || ! -d "$pak_dir" ]]; then
  print -u2 "Unreal reported success but did not produce a self-contained application: $app"
  exit 5
fi

preflight_log="$archive_dir/EchoesOfTheBrokenSun.package-preflight.log"
build_log="$archive_dir/EchoesOfTheBrokenSun.BuildCookRun.log"
/bin/mv "$preflight_log_pending" "$preflight_log"
/bin/mv "$build_log_pending" "$build_log"

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
signature_evidence="$archive_dir/EchoesOfTheBrokenSun.signature-assessment.txt"
{
  /usr/bin/codesign --verify --deep --strict --verbose=4 "$app"
  /usr/bin/codesign --display --verbose=4 "$app"
} > "$signature_evidence" 2>&1
signature_class="$(/usr/bin/awk -F= '$1 == "Signature" { print $2; exit }' "$signature_evidence")"
signature_team_identifier="$(/usr/bin/awk -F= '$1 == "TeamIdentifier" { print $2; exit }' "$signature_evidence")"
if [[ "$signature_class" != adhoc || "$signature_team_identifier" != "not set" ]]; then
  print -u2 "The local Development package did not retain the required ad-hoc/no-Team-ID signature boundary."
  exit 8
fi
signature_team_identifier=none
{
  print "echoes_signature_class=$signature_class"
  print "echoes_signature_team_identifier=$signature_team_identifier"
  print "echoes_signature_verification=passed"
} >> "$signature_evidence"

gatekeeper_evidence="$archive_dir/EchoesOfTheBrokenSun.gatekeeper-assessment.txt"
/usr/sbin/spctl --status > "$gatekeeper_evidence" 2>&1 || true
print "assessment_target=${app:t}" >> "$gatekeeper_evidence"
if /usr/sbin/spctl --assess --type execute --verbose=4 "$app" >> "$gatekeeper_evidence" 2>&1; then
  gatekeeper_exit=0
else
  gatekeeper_exit=$?
fi
if /usr/bin/grep -qi 'assessments disabled' "$gatekeeper_evidence"; then
  gatekeeper_policy=disabled
elif /usr/bin/grep -qi 'assessments enabled' "$gatekeeper_evidence"; then
  gatekeeper_policy=enabled
else
  gatekeeper_policy=unknown
fi
if [[ "$gatekeeper_policy" == disabled ]]; then
  gatekeeper_assessment=not-enforced
elif [[ "$gatekeeper_policy" == enabled ]] && (( gatekeeper_exit == 0 )); then
  gatekeeper_assessment=accepted
elif /usr/bin/grep -Eqi 'rejected|not accepted|no usable signature' "$gatekeeper_evidence"; then
  gatekeeper_assessment=rejected
else
  gatekeeper_assessment=assessment-error
fi
{
  print "echoes_gatekeeper_policy=$gatekeeper_policy"
  print "echoes_gatekeeper_assessment=$gatekeeper_assessment"
  print "echoes_gatekeeper_exit_code=$gatekeeper_exit"
} >> "$gatekeeper_evidence"

stapler_evidence="$archive_dir/EchoesOfTheBrokenSun.stapler-validation.txt"
if /usr/bin/xcrun stapler validate "$app" > "$stapler_evidence" 2>&1; then
  stapler_exit=0
else
  stapler_exit=$?
fi
if (( stapler_exit == 0 )); then
  stapling_status=validated
elif /usr/bin/grep -Eqi 'does not have a ticket|could not validate|not stapled' "$stapler_evidence"; then
  stapling_status=not-stapled
else
  stapling_status=validation-error
fi
{
  print "echoes_stapling_status=$stapling_status"
  print "echoes_stapler_exit_code=$stapler_exit"
} >> "$stapler_evidence"

smoke_log="$archive_dir/EchoesOfTheBrokenSun.normal-startup-smoke.log"
smoke_state="$archive_dir/SmokeRuntimeState/Normal"
"$project_root/Scripts/run_packaged_smoke.sh" "$app" "$smoke_log" "$smoke_state"
stress_smoke_log="$archive_dir/EchoesOfTheBrokenSun.legacy-stress-startup-smoke.log"
stress_smoke_state="$archive_dir/SmokeRuntimeState/LegacyStress"
"$project_root/Scripts/run_packaged_stress_smoke.sh" "$app" "$stress_smoke_log" "$stress_smoke_state"
smoke_log_sha256="$(/usr/bin/shasum -a 256 "$smoke_log" | /usr/bin/awk '{print $1}')"
stress_smoke_log_sha256="$(/usr/bin/shasum -a 256 "$stress_smoke_log" | /usr/bin/awk '{print $1}')"

verify_clean_pushed_source "before manifest"
generated_content_pack_before_manifest_sha256="$(/usr/bin/shasum -a 256 "$generated_content_pack" | /usr/bin/awk '{print $1}')"
generated_content_pack_before_manifest_declared_sha256="$(/usr/bin/tr -d '\r\n' < "$generated_content_pack_digest")"
if [[ "$generated_content_pack_before_manifest_sha256" != "$generated_content_pack_sha256" ||
      "$generated_content_pack_before_manifest_declared_sha256" != "$generated_content_pack_sha256" ]]; then
  print -u2 "The generated cooked content input changed before provenance publication."
  exit 9
fi
manifest="$archive_dir/EchoesOfTheBrokenSun.manifest.txt"
manifest_digest="$archive_dir/EchoesOfTheBrokenSun.manifest.sha256"
source_status_evidence="$archive_dir/EchoesOfTheBrokenSun.source-status.porcelain-v2-z"
git_lfs_status_evidence="$archive_dir/EchoesOfTheBrokenSun.git-lfs-status.porcelain"
git_lfs_fsck_evidence="$archive_dir/EchoesOfTheBrokenSun.git-lfs-fsck.txt"
toolchain_evidence="$archive_dir/EchoesOfTheBrokenSun.toolchain.txt"
generated_content_pack_copy="$archive_dir/EchoesContentPack.cooked-input.json"
generated_content_pack_digest_copy="$archive_dir/EchoesContentPack.cooked-input.json.sha256"
packager_copy="$archive_dir/package_macos.used.sh"
package_verifier_copy="$archive_dir/verify_packaged_app.used.py"

git -C "$project_root" status --porcelain=v2 -z > "$source_status_evidence"
git -C "$project_root" lfs status --porcelain > "$git_lfs_status_evidence"
print -r -- "$git_lfs_fsck_output" > "$git_lfs_fsck_evidence"
{
  print "unreal_root=$ue_root"
  print "unreal_engine=$engine_version"
  print "unreal_changelist=$engine_changelist"
  print "unreal_branch=$engine_branch"
  print "unreal_promoted=$engine_promoted"
  print "engine_build_file_sha256=$engine_build_file_sha256"
  print "uat_sha256=$uat_sha256"
  print "uat_driver_sha256=$uat_driver_sha256"
  print "git_path=$git_path"
  print "git_version=$git_version"
  print "git_sha256=$git_sha256"
  print "developer_dir=$developer_dir"
  print "xcode=$xcode_version"
  print "macos_sdk_version=$sdk_version"
  print "macos_sdk_path=$sdk_path"
  print "clang_path=$clang_path"
  print "clang_version=$clang_version"
  print "clang_sha256=$clang_sha256"
  print "metal_path=$metal_path"
  print "metal_version=$metal_version"
  print "metal_sha256=$metal_sha256"
} > "$toolchain_evidence"
/bin/cp "$generated_content_pack" "$generated_content_pack_copy"
/bin/cp "$generated_content_pack_digest" "$generated_content_pack_digest_copy"
/bin/cp "$project_root/Scripts/package_macos.sh" "$packager_copy"
/bin/cp "$project_root/Scripts/verify_packaged_app.py" "$package_verifier_copy"

preflight_log_sha256="$(/usr/bin/shasum -a 256 "$preflight_log" | /usr/bin/awk '{print $1}')"
build_log_sha256="$(/usr/bin/shasum -a 256 "$build_log" | /usr/bin/awk '{print $1}')"
source_status_evidence_sha256="$(/usr/bin/shasum -a 256 "$source_status_evidence" | /usr/bin/awk '{print $1}')"
git_lfs_status_evidence_sha256="$(/usr/bin/shasum -a 256 "$git_lfs_status_evidence" | /usr/bin/awk '{print $1}')"
git_lfs_fsck_evidence_sha256="$(/usr/bin/shasum -a 256 "$git_lfs_fsck_evidence" | /usr/bin/awk '{print $1}')"
toolchain_evidence_sha256="$(/usr/bin/shasum -a 256 "$toolchain_evidence" | /usr/bin/awk '{print $1}')"
generated_content_pack_copy_sha256="$(/usr/bin/shasum -a 256 "$generated_content_pack_copy" | /usr/bin/awk '{print $1}')"
generated_content_pack_digest_copy_sha256="$(/usr/bin/shasum -a 256 "$generated_content_pack_digest_copy" | /usr/bin/awk '{print $1}')"
if [[ "$generated_content_pack_copy_sha256" != "$generated_content_pack_sha256" ]]; then
  print -u2 "The retained generated cooked content input differs from the preflight input."
  exit 9
fi
signature_evidence_sha256="$(/usr/bin/shasum -a 256 "$signature_evidence" | /usr/bin/awk '{print $1}')"
gatekeeper_evidence_sha256="$(/usr/bin/shasum -a 256 "$gatekeeper_evidence" | /usr/bin/awk '{print $1}')"
stapler_evidence_sha256="$(/usr/bin/shasum -a 256 "$stapler_evidence" | /usr/bin/awk '{print $1}')"
packager_sha256="$(/usr/bin/shasum -a 256 "$packager_copy" | /usr/bin/awk '{print $1}')"
package_verifier_sha256="$(/usr/bin/shasum -a 256 "$package_verifier_copy" | /usr/bin/awk '{print $1}')"
project_file_sha256="$(/usr/bin/shasum -a 256 "$project" | /usr/bin/awk '{print $1}')"
application_executable_sha256="$(/usr/bin/shasum -a 256 "$binary" | /usr/bin/awk '{print $1}')"

{
  print "manifest_schema=2"
  print "artifact=EchoesOfTheBrokenSun.app"
  print "created_utc=$timestamp"
  print "source_commit=$source_commit"
  print "source_tree_hash=$source_tree_hash"
  print "source_branch=$source_branch"
  print "source_checkout_path=$project_root"
  print "origin_main=$origin_commit"
  print "remote_main=$remote_commit"
  print "source_tree=clean"
  print "source_binding=clean-pushed-main"
  print "source_upstream_ref=origin/main"
  print "source_status_sha256=$source_status_evidence_sha256"
  print "source_status_evidence=${source_status_evidence:t}"
  print "git_lfs_version=$git_lfs_version"
  print "git_lfs_status=clean"
  print "git_lfs_status_sha256=$git_lfs_status_evidence_sha256"
  print "git_lfs_status_evidence=${git_lfs_status_evidence:t}"
  print "git_lfs_fsck=passed"
  print "git_lfs_fsck_sha256=$git_lfs_fsck_evidence_sha256"
  print "git_lfs_fsck_evidence=${git_lfs_fsck_evidence:t}"
  print "configuration=Development"
  print "platform=Mac-arm64"
  print "architecture=$host_arch"
  print "archive_path=$archive_dir"
  print "archive_outside_checkout=true"
  print "archive_free_gib_before=$archive_free_gib"
  print "internal_free_gib_before=$internal_free_gib"
  print "build_command_argv=$build_command_argv"
  print "package_preflight_log=${preflight_log:t}"
  print "package_preflight_log_sha256=$preflight_log_sha256"
  print "build_log=${build_log:t}"
  print "build_log_sha256=$build_log_sha256"
  print "ignored_cook_inputs=Content/Data/Generated/EchoesContentPack.json;Content/Data/Generated/EchoesContentPack.json.sha256"
  print "generated_content_pack_source=Content/Data/Generated/EchoesContentPack.json"
  print "generated_content_pack_digest_source=Content/Data/Generated/EchoesContentPack.json.sha256"
  print "generated_content_pack=${generated_content_pack_copy:t}"
  print "generated_content_pack_sha256=$generated_content_pack_copy_sha256"
  print "generated_content_pack_digest=${generated_content_pack_digest_copy:t}"
  print "generated_content_pack_digest_sha256=$generated_content_pack_digest_copy_sha256"
  print "normal_startup_smoke=${smoke_log:t}"
  print "normal_startup_smoke_sha256=$smoke_log_sha256"
  print "legacy_stress_startup_smoke=${stress_smoke_log:t}"
  print "legacy_stress_startup_smoke_sha256=$stress_smoke_log_sha256"
  print "unreal_engine=$engine_version"
  print "unreal_root=$ue_root"
  print "unreal_changelist=$engine_changelist"
  print "unreal_branch=$engine_branch"
  print "unreal_promoted=$engine_promoted"
  print "engine_build_file_sha256=$engine_build_file_sha256"
  print "uat_sha256=$uat_sha256"
  print "uat_driver_sha256=$uat_driver_sha256"
  print "git_path=$git_path"
  print "git_version=$git_version"
  print "git_sha256=$git_sha256"
  print "toolchain_evidence=${toolchain_evidence:t}"
  print "toolchain_evidence_sha256=$toolchain_evidence_sha256"
  print "developer_dir=$developer_dir"
  print "xcode=$xcode_version"
  print "macos_sdk_version=$sdk_version"
  print "macos_sdk_path=$sdk_path"
  print "clang_path=$clang_path"
  print "clang_version=$clang_version"
  print "clang_sha256=$clang_sha256"
  print "metal_path=$metal_path"
  print "metal_version=$metal_version"
  print "metal_sha256=$metal_sha256"
  print "host_os_version=$host_os_version"
  print "host_os_build=$host_os_build"
  print "host_arch=$host_arch"
  print "host_model=$host_model"
  print "project_file_sha256=$project_file_sha256"
  print "packager_copy=${packager_copy:t}"
  print "packager_sha256=$packager_sha256"
  print "package_verifier_copy=${package_verifier_copy:t}"
  print "package_verifier_sha256=$package_verifier_sha256"
  print "bundle_identifier=$(/usr/libexec/PlistBuddy -c 'Print :CFBundleIdentifier' "$plist")"
  print "bundle_short_version=$(/usr/libexec/PlistBuddy -c 'Print :CFBundleShortVersionString' "$plist")"
  print "bundle_build_version=$(/usr/libexec/PlistBuddy -c 'Print :CFBundleVersion' "$plist")"
  print "application_executable_sha256=$application_executable_sha256"
  print "signature_class=$signature_class"
  print "signature_team_identifier=$signature_team_identifier"
  print "signature_verification=passed"
  print "signature_evidence=${signature_evidence:t}"
  print "signature_evidence_sha256=$signature_evidence_sha256"
  print "developer_id_signing=not-performed"
  print "gatekeeper_policy=$gatekeeper_policy"
  print "gatekeeper_assessment=$gatekeeper_assessment"
  print "gatekeeper_exit_code=$gatekeeper_exit"
  print "gatekeeper_evidence=${gatekeeper_evidence:t}"
  print "gatekeeper_evidence_sha256=$gatekeeper_evidence_sha256"
  print "notarization_status=not-submitted-by-package-tool"
  print "stapling_status=$stapling_status"
  print "stapler_exit_code=$stapler_exit"
  print "stapler_evidence=${stapler_evidence:t}"
  print "stapler_evidence_sha256=$stapler_evidence_sha256"
  print "installer_status=not-produced"
  print "release_qualification=not-release-qualified"
  print "claim_boundary=local-development-package-only"
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
provenance="$archive_dir/EchoesOfTheBrokenSun.provenance.json"
provenance_digest="$archive_dir/EchoesOfTheBrokenSun.provenance.sha256"
/usr/bin/python3 "$project_root/Scripts/verify_packaged_app.py" \
  --app "$app" --manifest "$manifest" --manifest-digest "$manifest_digest" \
  --json-output "$provenance" >/dev/null
provenance_hash="$(/usr/bin/shasum -a 256 "$provenance" | /usr/bin/awk '{print $1}')"
print "$provenance_hash  ${provenance:t}" > "$provenance_digest"
(cd "$archive_dir" && /usr/bin/shasum -a 256 -c "${manifest_digest:t}" >/dev/null)
(cd "$archive_dir" && /usr/bin/shasum -a 256 -c "${provenance_digest:t}" >/dev/null)

print "Clean-source incremental Mac Development package passed structural, signature, startup, and exact-manifest checks."
print "Application: $app"
print "Content manifest: $manifest"
print "Manifest SHA-256: $manifest_hash"
print "Machine-readable provenance: $provenance"
print "Provenance SHA-256: $provenance_hash"
print "Distribution boundary: ad-hoc signed, not notarized by this tool, no installer, and not release-qualified."
