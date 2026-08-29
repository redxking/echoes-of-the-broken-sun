#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
project_dir="$(cd "${script_dir}/.." && pwd)"
build_dir="$(mktemp -d "${TMPDIR:-/tmp}/echoes-sim-profile.XXXXXX")"
trap 'rm -rf "${build_dir}"' EXIT

cxx="${CXX:-clang++}"
executable="${build_dir}/echoes_sim_profile"
artifact_dir="${project_dir}/BuildArtifacts/Performance"
artifact="${artifact_dir}/native_sim_profile.json"

"${cxx}" \
  -std=c++20 \
  -O3 \
  -DNDEBUG \
  -DECHOES_SIMCORE_PROFILE=1 \
  -Wall \
  -Wextra \
  -Wpedantic \
  -Werror \
  -I "${project_dir}/Source/EchoesSimCore/Public" \
  "${project_dir}/Source/EchoesSimCore/Private/Simulation.cpp" \
  "${project_dir}/Tests/Native/SimCoreProfile.cpp" \
  -o "${executable}"

mkdir -p "${artifact_dir}"
"${executable}" | tee "${artifact}"
echo "Performance evidence: ${artifact}"
