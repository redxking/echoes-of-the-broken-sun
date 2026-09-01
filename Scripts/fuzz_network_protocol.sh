#!/usr/bin/env bash
# Bounded coverage-guided fuzzing of the native network wire decoders.
#
# Scheduling contract (coordinator ruling, ACTIVE_LANES.md): building this
# harness and generating seeds is LIGHT work; the bounded fuzz RUN is
# heavy-adjacent and executes only on the coordinator's explicit go signal,
# never while WorkstreamControl/HEAVY_RUN_LOCK.md is ACTIVE. The run phase
# therefore refuses to start unless ECHOES_FUZZ_GO=1 is exported.
#
# Toolchain: -fsanitize=fuzzer needs a compiler that ships the libFuzzer
# runtime. Apple's Xcode clang may not; Homebrew LLVM does. Override with
# ECHOES_FUZZ_CXX. Without a fuzzer-capable compiler this script still
# builds the seed generator and an ASan/UBSan regression-replay binary and
# replays the seed corpus once, then exits nonzero for the missing runtime.
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
project_dir="$(cd "${script_dir}/.." && pwd)"
build_dir="$(mktemp -d "${TMPDIR:-/tmp}/echoes-net-fuzz.XXXXXX")"
trap 'rm -rf "${build_dir}"' EXIT

seconds_per_target="${ECHOES_FUZZ_SECONDS:-120}"
evidence_root="${ECHOES_FUZZ_EVIDENCE_ROOT:-}"
go_signal="${ECHOES_FUZZ_GO:-0}"

if ! [[ "${seconds_per_target}" =~ ^[0-9]+$ ]] ||
    [ "${seconds_per_target}" -lt 1 ] ||
    [ "${seconds_per_target}" -gt 3600 ]; then
    echo "ECHOES_FUZZ_SECONDS must be an integer from 1 through 3600." >&2
    exit 2
fi

cxx="${ECHOES_FUZZ_CXX:-}"
if [ -z "${cxx}" ]; then
    if [ -x /opt/homebrew/opt/llvm/bin/clang++ ]; then
        cxx=/opt/homebrew/opt/llvm/bin/clang++
    else
        cxx=clang++
    fi
fi

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
    "${project_dir}/Tests/Native/NetworkProtocolFuzzers.cpp"
)
target_names=(hello command batch keyframe delta apply)

echo "== build: seed generator =="
"${cxx}" "${common_flags[@]}" -O1 -DECHOES_FUZZ_WRITE_SEEDS \
    "${sources[@]}" -o "${build_dir}/seedgen"

echo "== build: ASan/UBSan regression replayer =="
"${cxx}" "${common_flags[@]}" -O1 -g -fno-omit-frame-pointer \
    -fsanitize=address,undefined -DECHOES_FUZZ_REGRESSION_MAIN \
    "${sources[@]}" -o "${build_dir}/replayer"

echo "== seeds =="
seed_dir="${build_dir}/seeds"
mkdir -p "${seed_dir}"
"${build_dir}/seedgen" "${seed_dir}"

echo "== seed replay through every target (sanitized) =="
for index in "${!target_names[@]}"; do
    name="${target_names[${index}]}"
    ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 \
        ECHOES_FUZZ_TARGET=$((index + 1)) \
        "${build_dir}/replayer" "${seed_dir}/${name}.bin"
done

echo "== build: libFuzzer targets =="
fuzzer_available=true
if ! "${cxx}" "${common_flags[@]}" -O1 -g -fno-omit-frame-pointer \
    -fsanitize=fuzzer,address,undefined \
    "${sources[@]}" -o "${build_dir}/fuzzer" 2>"${build_dir}/fuzzer-build.log"
then
    fuzzer_available=false
    echo "libFuzzer runtime is unavailable with ${cxx}:" >&2
    tail -n 5 "${build_dir}/fuzzer-build.log" >&2
    echo "Install a fuzzer-capable toolchain (e.g. Homebrew LLVM) or set" >&2
    echo "ECHOES_FUZZ_CXX; seed build and sanitized replay passed above." >&2
fi

if [ "${go_signal}" != "1" ]; then
    echo "Run phase NOT started: export ECHOES_FUZZ_GO=1 only after the" >&2
    echo "coordinator's explicit go signal while HEAVY_RUN_LOCK.md is FREE." >&2
    if [ "${fuzzer_available}" = true ]; then
        exit 0
    fi
    exit 3
fi
if [ "${fuzzer_available}" != true ]; then
    echo "ECHOES_FUZZ_GO=1 was set but no fuzzer runtime links." >&2
    exit 3
fi

if [ -z "${evidence_root}" ]; then
    head_sha="$(git -C "${project_dir}" rev-parse --short=7 HEAD)"
    stamp="$(date -u +%Y%m%dT%H%M%SZ)"
    evidence_root="${project_dir}/../WorkstreamControl/evidence/network-fuzz-${head_sha}-${stamp}"
fi
mkdir -p "${evidence_root}"

{
    echo "utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "head=$(git -C "${project_dir}" rev-parse HEAD)"
    echo "compiler=$(${cxx} --version | head -n 1)"
    echo "seconds_per_target=${seconds_per_target}"
} > "${evidence_root}/run-manifest.txt"

overall=0
for index in "${!target_names[@]}"; do
    name="${target_names[${index}]}"
    corpus="${evidence_root}/corpus-${name}"
    mkdir -p "${corpus}"
    cp "${seed_dir}/${name}.bin" "${corpus}/"
    echo "== fuzz: ${name} (${seconds_per_target}s) =="
    if ! ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 \
        ECHOES_FUZZ_TARGET=$((index + 1)) \
        "${build_dir}/fuzzer" \
        -max_total_time="${seconds_per_target}" \
        -print_final_stats=1 \
        -artifact_prefix="${evidence_root}/artifact-${name}-" \
        "${corpus}" \
        > "${evidence_root}/fuzz-${name}.log" 2>&1
    then
        overall=1
        echo "TARGET ${name} FAILED; see ${evidence_root}/fuzz-${name}.log" >&2
    fi
    grep -E "stat::number_of_executed_units|cov:" \
        "${evidence_root}/fuzz-${name}.log" | tail -n 3 || true
done

(cd "${evidence_root}" && find . -type f -exec shasum -a 256 {} \; \
    | sort -k 2 > evidence-sha256.txt.tmp &&
    mv evidence-sha256.txt.tmp evidence-sha256.txt)

if [ "${overall}" -ne 0 ]; then
    echo "Fuzzing found at least one crash or property violation." >&2
    exit 1
fi
echo "All ${#target_names[@]} targets completed ${seconds_per_target}s each with zero crashes."
echo "Evidence: ${evidence_root}"
