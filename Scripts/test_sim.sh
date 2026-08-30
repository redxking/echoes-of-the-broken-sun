#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
project_dir="$(cd "${script_dir}/.." && pwd)"
build_dir="$(mktemp -d "${TMPDIR:-/tmp}/echoes-sim-tests.XXXXXX")"
trap 'rm -rf "${build_dir}"' EXIT

cxx="${CXX:-clang++}"
common_flags=(
    -std=c++20
    -Wall
    -Wextra
    -Wpedantic
    -Werror
    -I "${project_dir}/Source/EchoesSimCore/Public"
)
sources=(
    "${project_dir}/Source/EchoesSimCore/Private/Simulation.cpp"
    "${project_dir}/Source/EchoesSimCore/Private/NetworkProtocol.cpp"
    "${project_dir}/Tests/Native/SimCoreTests.cpp"
)

run_configuration() {
    local name="$1"
    shift
    local executable="${build_dir}/echoes_sim_tests_${name}"

    echo "== ${name} =="
    "${cxx}" "${common_flags[@]}" "$@" "${sources[@]}" -o "${executable}"
    "${executable}"
}

run_configuration optimized -O2
run_configuration debug -O0 -g

echo "== address-undefined-sanitizers =="
sanitized_executable="${build_dir}/echoes_sim_tests_sanitized"
"${cxx}" "${common_flags[@]}" \
    -O1 \
    -g \
    -fno-omit-frame-pointer \
    -fsanitize=address,undefined \
    "${sources[@]}" \
    -o "${sanitized_executable}"
# LeakSanitizer is unavailable in Apple's macOS AddressSanitizer runtime.
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 "${sanitized_executable}"
