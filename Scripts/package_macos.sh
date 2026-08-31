#!/bin/zsh
set -euo pipefail

project_root="${0:A:h:h}"
if ! /usr/bin/env -0 >/dev/null; then
  print -u2 "Packaging could not enumerate the inherited process environment safely."
  exit 2
fi
typeset -a inherited_git_environment_names=()
while IFS= read -r -d $'\0' environment_record; do
  environment_name="${environment_record%%=*}"
  if [[ "$environment_name" == GIT_* ]]; then
    inherited_git_environment_names+=("$environment_name")
  fi
done < <(/usr/bin/env -0)
if (( ${#inherited_git_environment_names} > 0 )); then
  inherited_git_environment_names=("${(@on)inherited_git_environment_names}")
  print -u2 "Packaging refuses inherited Git or Git LFS environment variables before tool use: ${(j:, :)inherited_git_environment_names}"
  exit 2
fi
export GIT_ATTR_NOSYSTEM=1
export GIT_CONFIG_GLOBAL=/dev/null
export GIT_CONFIG_NOSYSTEM=1
export GIT_CONFIG_SYSTEM=/dev/null
export GIT_NO_REPLACE_OBJECTS=1
export GIT_TERMINAL_PROMPT=0
git_environment_policy="reject-all-inherited-GIT-prefix;controlled=GIT_ATTR_NOSYSTEM=1,GIT_CONFIG_GLOBAL=/dev/null,GIT_CONFIG_NOSYSTEM=1,GIT_CONFIG_SYSTEM=/dev/null,GIT_NO_REPLACE_OBJECTS=1,GIT_TERMINAL_PROMPT=0"
approved_git_path="/opt/homebrew/bin/git"
approved_git_version="git version 2.55.0"
approved_git_resolved_path="/opt/homebrew/Cellar/git/2.55.0/bin/git"
approved_git_sha256="9048038886ac36210fbb616b49b0707465f63683cb04e33a2013baf95f746938"
approved_git_lfs_path="/opt/homebrew/bin/git-lfs"
approved_git_lfs_version="git-lfs/3.7.1 (GitHub; darwin arm64; go 1.25.3)"
approved_git_lfs_resolved_path="/opt/homebrew/Cellar/git-lfs/3.7.1/bin/git-lfs"
approved_git_lfs_sha256="8a62ba6b8bc9ab15cae4b2704c434568b2d8bd4bda9468a0d48fb70131191501"
approved_origin_url="https://github.com/redxking/echoes-of-the-broken-sun.git"
git_path="$(command -v git)"
git_lfs_path="$(command -v git-lfs)"
if [[ "$git_path" != "$approved_git_path" || ! -x "$git_path" ||
      "$git_lfs_path" != "$approved_git_lfs_path" || ! -x "$git_lfs_path" ]]; then
  print -u2 "Packaging requires the approved Git and Git LFS entry points at /opt/homebrew/bin."
  exit 2
fi
git_resolved_path="${git_path:A}"
git_lfs_resolved_path="${git_lfs_path:A}"
git_sha256="$(/usr/bin/shasum -a 256 "$git_path" | /usr/bin/awk '{print $1}')"
git_lfs_sha256="$(/usr/bin/shasum -a 256 "$git_lfs_path" | /usr/bin/awk '{print $1}')"
if [[ "$git_resolved_path" != "$approved_git_resolved_path" ||
      "$git_sha256" != "$approved_git_sha256" ||
      "$git_lfs_resolved_path" != "$approved_git_lfs_resolved_path" ||
      "$git_lfs_sha256" != "$approved_git_lfs_sha256" ]]; then
  print -u2 "The approved Git or Git LFS executable identity changed; review and repin before packaging."
  exit 2
fi
git_version="$($git_resolved_path --version)"
git_lfs_version="$($git_lfs_resolved_path version)"
if [[ "$git_version" != "$approved_git_version" ||
      "$git_lfs_version" != "$approved_git_lfs_version" ]]; then
  print -u2 "Packaging requires the reviewed Git 2.55.0 and Git LFS 3.7.1 tool identities."
  exit 2
fi
git() {
  "$git_resolved_path" "$@"
}
git_lfs() {
  (cd "$project_root" && PATH="${git_resolved_path:h}:$PATH" "$git_lfs_resolved_path" "$@")
}
source_index_concealment_records() {
  {
    git -C "$project_root" ls-files -v |
      /usr/bin/awk 'substr($0, 1, 2) != "H " { print "ls-files-v " $0 }'
    git -C "$project_root" ls-files -f |
      /usr/bin/awk 'substr($0, 1, 2) != "H " { print "ls-files-f " $0 }'
  }
}
git_lfs_restrictive_config_records() {
  git_lfs env 2>/dev/null |
    /usr/bin/awk '/^FetchInclude=|^FetchExclude=/ { print }'
}
export PATH="${git_resolved_path:h}:${git_lfs_resolved_path:h}:$PATH"
if ! source_top_level="$(git -C "$project_root" rev-parse --path-format=absolute --show-toplevel 2>/dev/null)"; then
  print -u2 "Packaging could not resolve the canonical Git checkout top level."
  exit 10
fi
source_top_level="${source_top_level:A}"
if [[ "$source_top_level" != "$project_root" ]]; then
  print -u2 "Packaging refuses a Git checkout redirected away from the intended source root."
  print -u2 "Intended source root: $project_root; observed Git top level: $source_top_level"
  exit 10
fi
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
if [[ "$source_branch" != detached ]]; then
  print -u2 "Packaging requires a detached dedicated linked worktree at pushed main."
  exit 10
fi
origin_commit="$(git -C "$project_root" rev-parse --verify origin/main 2>/dev/null || print unknown)"
origin_fetch_url="$(git -C "$project_root" remote get-url origin 2>/dev/null || print unknown)"
origin_push_url="$(git -C "$project_root" remote get-url --push origin 2>/dev/null || print unknown)"
if [[ "$origin_fetch_url" != "$approved_origin_url" ||
      "$origin_push_url" != "$approved_origin_url" ]]; then
  print -u2 "Packaging requires the reviewed canonical GitHub origin for both fetch and push."
  exit 10
fi
source_status_sha256="$(git -C "$project_root" status --porcelain=v2 -z 2>/dev/null | /usr/bin/shasum -a 256 | /usr/bin/awk '{print $1}')"
git_lfs_status_sha256="$(git_lfs status --porcelain 2>/dev/null | /usr/bin/shasum -a 256 | /usr/bin/awk '{print $1}')"
max_parallel_actions="${ECHOES_MAX_PARALLEL_ACTIONS:-4}"
git_common_dir="$(git -C "$project_root" rev-parse --path-format=absolute --git-common-dir 2>/dev/null || print "$project_root/.git")"
git_common_dir="${git_common_dir:A}"
git_dir="$(git -C "$project_root" rev-parse --path-format=absolute --git-dir 2>/dev/null || print "$project_root/.git")"
git_dir="${git_dir:A}"
repository_checkout="${git_common_dir:h}"
artifact_root_default="${repository_checkout:h}/BuildArtifacts"
artifact_root="${ECHOES_BUILD_ARTIFACT_ROOT:-$artifact_root_default}"
if [[ "$artifact_root" != /* ]]; then
  artifact_root="${project_root:h}/$artifact_root"
fi
artifact_root="${artifact_root:A}"
source_short="${source_commit[1,8]}"
archive_dir="${1:-$artifact_root/Packages/Mac-Development-$timestamp-$source_short}"

if [[ "$git_dir" == "$git_common_dir" ]]; then
  print -u2 "Packaging requires a dedicated linked Git worktree, not the primary checkout."
  exit 10
fi
if ! source_index_concealment="$(source_index_concealment_records)"; then
  print -u2 "Packaging could not inspect Git index concealment flags."
  exit 10
fi
if [[ -n "$source_index_concealment" ]]; then
  print -u2 "Packaging refuses assume-unchanged, skip-worktree, or other nonstandard Git index states."
  print -u2 "$source_index_concealment"
  exit 10
fi
if [[ -n "${GIT_LFS_SKIP_SMUDGE:-}" ]]; then
  print -u2 "Packaging refuses GIT_LFS_SKIP_SMUDGE; every tracked LFS object must be hydrated."
  exit 10
fi
if ! git_lfs_restrictive_config="$(git_lfs_restrictive_config_records)"; then
  print -u2 "Packaging could not inspect Git LFS fetch restrictions."
  exit 10
fi
if [[ -n "$git_lfs_restrictive_config" ]]; then
  print -u2 "Packaging refuses lfs.fetchinclude or lfs.fetchexclude restrictions."
  print -u2 "$git_lfs_restrictive_config"
  exit 10
fi
if ! git_lfs_inventory="$(git_lfs ls-files -l 2>/dev/null)"; then
  print -u2 "Packaging could not inspect the Git LFS working-tree inventory."
  exit 10
fi
git_lfs_tracked_file_count="$(/usr/bin/printf '%s\n' "$git_lfs_inventory" | /usr/bin/awk 'NF { count++ } END { print count + 0 }')"
git_lfs_hydration_failures="$(
  /usr/bin/printf '%s\n' "$git_lfs_inventory" |
    /usr/bin/awk 'NF && (NF < 3 || length($1) != 64 || $1 !~ /^[0-9a-f]+$/ || $2 != "*") { print }'
)"
if (( git_lfs_tracked_file_count < 1 )) || [[ -n "$git_lfs_hydration_failures" ]]; then
  print -u2 "Packaging requires every Git LFS tracked working-tree file to contain its full object, not a pointer stub."
  print -u2 "$git_lfs_hydration_failures"
  exit 10
fi
ignored_checkout_state="$(git -C "$project_root" ls-files --others --ignored --exclude-standard)"
if [[ -n "$ignored_checkout_state" ]]; then
  print -u2 "Packaging requires a fresh linked worktree with no repository-local ignored or derived state."
  print -u2 "$ignored_checkout_state"
  exit 10
fi
export PYTHONDONTWRITEBYTECODE=1

read_remote_main() {
  local remote_record
  remote_record="$(git -C "$project_root" ls-remote --exit-code "$approved_origin_url" refs/heads/main 2>/dev/null)" || return 1
  print "${remote_record%%[[:space:]]*}"
}

remote_commit="$(read_remote_main || print unknown)"

verify_clean_pushed_source() {
  local phase="$1"
  local observed_top_level observed_commit observed_tree observed_origin observed_remote
  local observed_status observed_status_sha256 observed_lfs_status observed_lfs_status_sha256
  local observed_index_concealment observed_origin_fetch_url observed_origin_push_url
  local observed_lfs_inventory observed_lfs_restrictive_config
  observed_top_level="$(git -C "$project_root" rev-parse --path-format=absolute --show-toplevel 2>/dev/null || print unknown)"
  if [[ "$observed_top_level" != unknown ]]; then
    observed_top_level="${observed_top_level:A}"
  fi
  observed_commit="$(git -C "$project_root" rev-parse --verify HEAD 2>/dev/null || print unknown)"
  observed_tree="$(git -C "$project_root" rev-parse --verify 'HEAD^{tree}' 2>/dev/null || print unknown)"
  observed_origin="$(git -C "$project_root" rev-parse --verify origin/main 2>/dev/null || print unknown)"
  observed_origin_fetch_url="$(git -C "$project_root" remote get-url origin 2>/dev/null || print unknown)"
  observed_origin_push_url="$(git -C "$project_root" remote get-url --push origin 2>/dev/null || print unknown)"
  observed_remote="$(read_remote_main || print unknown)"
  observed_status="$(git -C "$project_root" status --porcelain --untracked-files=normal 2>/dev/null || print status-unavailable)"
  observed_status_sha256="$(git -C "$project_root" status --porcelain=v2 -z 2>/dev/null | /usr/bin/shasum -a 256 | /usr/bin/awk '{print $1}')"
  observed_lfs_status="$(git_lfs status --porcelain 2>/dev/null || print lfs-status-unavailable)"
  observed_lfs_status_sha256="$(git_lfs status --porcelain 2>/dev/null | /usr/bin/shasum -a 256 | /usr/bin/awk '{print $1}')"
  observed_index_concealment="$(source_index_concealment_records 2>/dev/null || print index-state-unavailable)"
  observed_lfs_restrictive_config="$(git_lfs_restrictive_config_records 2>/dev/null || print lfs-config-unavailable)"
  observed_lfs_inventory="$(git_lfs ls-files -l 2>/dev/null || print lfs-inventory-unavailable)"
  if [[ "$observed_top_level" != "$project_root" ||
        "$source_commit" == unknown || "$source_tree_hash" == unknown ||
        "$origin_commit" == unknown ||
        "$remote_commit" == unknown ||
        "$observed_commit" != "$source_commit" ||
        "$observed_tree" != "$source_tree_hash" ||
        "$observed_origin" != "$source_commit" ||
        "$observed_origin_fetch_url" != "$approved_origin_url" ||
        "$observed_origin_push_url" != "$approved_origin_url" ||
        "$observed_remote" != "$source_commit" ||
        -n "$observed_status" ||
        -n "$observed_lfs_status" ||
        -n "$observed_index_concealment" ||
        -n "$observed_lfs_restrictive_config" ||
        "$observed_lfs_inventory" != "$git_lfs_inventory" ||
        "$observed_status_sha256" != "$source_status_sha256" ||
        "$observed_lfs_status_sha256" != "$git_lfs_status_sha256" ]]; then
    print -u2 "Packaging requires an unchanged clean checkout at pushed origin/main ($phase)."
    print -u2 "Intended source root: $project_root; observed Git top level: $observed_top_level."
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
ignored_state_pending="$archive_dir.repo-local-ignored-before"
index_concealment_pending="$archive_dir.source-index-concealment"
lfs_restrictive_config_pending="$archive_dir.git-lfs-restrictive-config"
if [[ -e "$archive_dir" || -e "$preflight_log_pending" ||
      -e "$build_log_pending" || -e "$ignored_state_pending" ||
      -e "$index_concealment_pending" ||
      -e "$lfs_restrictive_config_pending" ]]; then
  print -u2 "Refusing to mix a new package with an existing archive: $archive_dir"
  print -u2 "Choose a new archive path. Existing artifacts and failed-run logs were left untouched."
  exit 3
fi

mkdir -p "${archive_dir:h}"
: > "$ignored_state_pending"
: > "$index_concealment_pending"
: > "$lfs_restrictive_config_pending"
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

if ! git_lfs_fsck_output="$(git_lfs fsck --objects --pointers 2>&1)"; then
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
  print "git_lfs_tracked_file_count=$git_lfs_tracked_file_count"
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
{
  print "echoes_content_preflight_outcome=passed"
  print "echoes_environment_preflight_outcome=passed"
  print "echoes_package_preflight_outcome=passed"
} >> "$preflight_log_pending"

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
print "echoes_build_cook_run_outcome=passed" >> "$build_log_pending"

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
print "echoes_normal_startup_smoke_outcome=passed" >> "$smoke_log"
stress_smoke_log="$archive_dir/EchoesOfTheBrokenSun.legacy-stress-startup-smoke.log"
stress_smoke_state="$archive_dir/SmokeRuntimeState/LegacyStress"
"$project_root/Scripts/run_packaged_stress_smoke.sh" "$app" "$stress_smoke_log" "$stress_smoke_state"
print "echoes_legacy_stress_startup_smoke_outcome=passed" >> "$stress_smoke_log"
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
repo_local_ignored_state_evidence="$archive_dir/EchoesOfTheBrokenSun.repo-local-ignored-before.txt"
source_index_concealment_evidence="$archive_dir/EchoesOfTheBrokenSun.source-index-concealment.txt"
git_lfs_status_evidence="$archive_dir/EchoesOfTheBrokenSun.git-lfs-status.porcelain"
git_lfs_fsck_evidence="$archive_dir/EchoesOfTheBrokenSun.git-lfs-fsck.txt"
git_lfs_restrictive_config_evidence="$archive_dir/EchoesOfTheBrokenSun.git-lfs-restrictive-config.txt"
git_lfs_hydration_evidence="$archive_dir/EchoesOfTheBrokenSun.git-lfs-hydration.txt"
toolchain_evidence="$archive_dir/EchoesOfTheBrokenSun.toolchain.txt"
generated_content_pack_copy="$archive_dir/EchoesContentPack.cooked-input.json"
generated_content_pack_digest_copy="$archive_dir/EchoesContentPack.cooked-input.json.sha256"
packager_copy="$archive_dir/package_macos.used.sh"
package_verifier_copy="$archive_dir/verify_packaged_app.used.py"

git -C "$project_root" status --porcelain=v2 -z > "$source_status_evidence"
/bin/mv "$ignored_state_pending" "$repo_local_ignored_state_evidence"
/bin/mv "$index_concealment_pending" "$source_index_concealment_evidence"
/bin/mv "$lfs_restrictive_config_pending" "$git_lfs_restrictive_config_evidence"
git_lfs status --porcelain > "$git_lfs_status_evidence"
{
  print -r -- "$git_lfs_fsck_output"
  print "echoes_git_lfs_fsck_outcome=passed"
} > "$git_lfs_fsck_evidence"
{
  print "echoes_git_lfs_hydration_outcome=passed"
  print "echoes_git_lfs_tracked_file_count=$git_lfs_tracked_file_count"
  print -r -- "$git_lfs_inventory"
} > "$git_lfs_hydration_evidence"
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
  print "git_resolved_path=$git_resolved_path"
  print "git_version=$git_version"
  print "git_sha256=$git_sha256"
  print "git_lfs_path=$git_lfs_path"
  print "git_lfs_resolved_path=$git_lfs_resolved_path"
  print "git_lfs_version=$git_lfs_version"
  print "git_lfs_sha256=$git_lfs_sha256"
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
repo_local_ignored_state_evidence_sha256="$(/usr/bin/shasum -a 256 "$repo_local_ignored_state_evidence" | /usr/bin/awk '{print $1}')"
source_index_concealment_evidence_sha256="$(/usr/bin/shasum -a 256 "$source_index_concealment_evidence" | /usr/bin/awk '{print $1}')"
git_lfs_status_evidence_sha256="$(/usr/bin/shasum -a 256 "$git_lfs_status_evidence" | /usr/bin/awk '{print $1}')"
git_lfs_fsck_evidence_sha256="$(/usr/bin/shasum -a 256 "$git_lfs_fsck_evidence" | /usr/bin/awk '{print $1}')"
git_lfs_restrictive_config_evidence_sha256="$(/usr/bin/shasum -a 256 "$git_lfs_restrictive_config_evidence" | /usr/bin/awk '{print $1}')"
git_lfs_hydration_evidence_sha256="$(/usr/bin/shasum -a 256 "$git_lfs_hydration_evidence" | /usr/bin/awk '{print $1}')"
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
  print "source_checkout_kind=dedicated-linked-worktree-detached-at-main"
  print "source_top_level_binding=canonical-exact"
  print "git_environment_policy=$git_environment_policy"
  print "origin_main=$origin_commit"
  print "remote_main=$remote_commit"
  print "source_tree=clean"
  print "source_binding=clean-pushed-main"
  print "source_upstream_ref=origin/main"
  print "source_origin_fetch_url=$origin_fetch_url"
  print "source_origin_push_url=$origin_push_url"
  print "source_remote_authority=github.com/redxking/echoes-of-the-broken-sun"
  print "source_status_sha256=$source_status_evidence_sha256"
  print "source_status_evidence=${source_status_evidence:t}"
  print "repo_local_derived_state_before=clean"
  print "repo_local_derived_state_scope=git-ignored-paths-within-source-checkout"
  print "repo_local_ignored_state_evidence=${repo_local_ignored_state_evidence:t}"
  print "repo_local_ignored_state_sha256=$repo_local_ignored_state_evidence_sha256"
  print "source_index_concealment=absent"
  print "source_index_concealment_scope=git-ls-files-v-and-f-non-H-records"
  print "source_index_concealment_evidence=${source_index_concealment_evidence:t}"
  print "source_index_concealment_sha256=$source_index_concealment_evidence_sha256"
  print "git_lfs_version=$git_lfs_version"
  print "git_lfs_path=$git_lfs_path"
  print "git_lfs_resolved_path=$git_lfs_resolved_path"
  print "git_lfs_sha256=$git_lfs_sha256"
  print "git_lfs_status=clean"
  print "git_lfs_status_sha256=$git_lfs_status_evidence_sha256"
  print "git_lfs_status_evidence=${git_lfs_status_evidence:t}"
  print "git_lfs_fsck=passed"
  print "git_lfs_fsck_outcome=passed"
  print "git_lfs_fsck_sha256=$git_lfs_fsck_evidence_sha256"
  print "git_lfs_fsck_evidence=${git_lfs_fsck_evidence:t}"
  print "git_lfs_restrictive_fetch_config=absent"
  print "git_lfs_restrictive_fetch_config_scope=effective-git-lfs-FetchInclude-and-FetchExclude"
  print "git_lfs_restrictive_config_evidence=${git_lfs_restrictive_config_evidence:t}"
  print "git_lfs_restrictive_config_sha256=$git_lfs_restrictive_config_evidence_sha256"
  print "git_lfs_hydration=complete"
  print "git_lfs_hydration_scope=all-git-lfs-tracked-working-tree-files"
  print "git_lfs_hydration_outcome=passed"
  print "git_lfs_tracked_file_count=$git_lfs_tracked_file_count"
  print "git_lfs_hydration_evidence=${git_lfs_hydration_evidence:t}"
  print "git_lfs_hydration_sha256=$git_lfs_hydration_evidence_sha256"
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
  print "content_preflight_outcome=passed"
  print "environment_preflight_outcome=passed"
  print "package_preflight_outcome=passed"
  print "build_log=${build_log:t}"
  print "build_log_sha256=$build_log_sha256"
  print "build_cook_run_outcome=passed"
  print "ignored_cook_inputs=Content/Data/Generated/EchoesContentPack.json;Content/Data/Generated/EchoesContentPack.json.sha256"
  print "generated_content_pack_source=Content/Data/Generated/EchoesContentPack.json"
  print "generated_content_pack_digest_source=Content/Data/Generated/EchoesContentPack.json.sha256"
  print "generated_content_pack=${generated_content_pack_copy:t}"
  print "generated_content_pack_sha256=$generated_content_pack_copy_sha256"
  print "generated_content_pack_digest=${generated_content_pack_digest_copy:t}"
  print "generated_content_pack_digest_sha256=$generated_content_pack_digest_copy_sha256"
  print "normal_startup_smoke=${smoke_log:t}"
  print "normal_startup_smoke_sha256=$smoke_log_sha256"
  print "normal_startup_smoke_outcome=passed"
  print "legacy_stress_startup_smoke=${stress_smoke_log:t}"
  print "legacy_stress_startup_smoke_sha256=$stress_smoke_log_sha256"
  print "legacy_stress_startup_smoke_outcome=passed"
  print "unreal_engine=$engine_version"
  print "unreal_root=$ue_root"
  print "unreal_changelist=$engine_changelist"
  print "unreal_branch=$engine_branch"
  print "unreal_promoted=$engine_promoted"
  print "engine_build_file_sha256=$engine_build_file_sha256"
  print "uat_sha256=$uat_sha256"
  print "uat_driver_sha256=$uat_driver_sha256"
  print "git_path=$git_path"
  print "git_resolved_path=$git_resolved_path"
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
  print "build_context_record=package-tool-observed"
  print "manifest_authority=self-consistency-record-not-independent-attestation"
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
  --require-live-build-context --json-output "$provenance" >/dev/null
provenance_hash="$(/usr/bin/shasum -a 256 "$provenance" | /usr/bin/awk '{print $1}')"
print "$provenance_hash  ${provenance:t}" > "$provenance_digest"
(cd "$archive_dir" && /usr/bin/shasum -a 256 -c "${manifest_digest:t}" >/dev/null)
(cd "$archive_dir" && /usr/bin/shasum -a 256 -c "${provenance_digest:t}" >/dev/null)

print "Fresh-linked-worktree Mac Development package passed structural, semantic-evidence, live-context, signature, startup, and exact-manifest checks."
print "Application: $app"
print "Content manifest: $manifest"
print "Manifest SHA-256: $manifest_hash"
print "Machine-readable provenance: $provenance"
print "Provenance SHA-256: $provenance_hash"
print "Distribution boundary: ad-hoc signed, not notarized by this tool, no installer, and not release-qualified."
