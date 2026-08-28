#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
project_dir="$(cd "${script_dir}/.." && pwd)"
build_dir="$(mktemp -d "${TMPDIR:-/tmp}/echoes-sim-tests.XXXXXX")"
trap 'rm -rf "${build_dir}"' EXIT

cxx="${CXX:-clang++}"
"${cxx}" \
    -std=c++20 \
    -O2 \
    -Wall \
    -Wextra \
    -Wpedantic \
    -Werror \
    -I "${project_dir}/Source/EchoesSimCore/Public" \
    "${project_dir}/Source/EchoesSimCore/Private/Simulation.cpp" \
    "${project_dir}/Tests/Native/SimCoreTests.cpp" \
    -o "${build_dir}/echoes_sim_tests"

"${build_dir}/echoes_sim_tests"
