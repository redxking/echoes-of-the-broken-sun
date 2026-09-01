// Coverage-guided fuzz targets for the engine-independent network wire
// decoders. One translation unit, three compile modes:
//
//   (default)                     libFuzzer entry point. Requires a compiler
//                                 whose runtime provides -fsanitize=fuzzer.
//   -DECHOES_FUZZ_WRITE_SEEDS     Plain binary: main(outDir) writes one valid
//                                 canonical seed packet per target.
//   -DECHOES_FUZZ_REGRESSION_MAIN Plain binary: main(file...) replays corpus
//                                 files through the selected target once, for
//                                 ASan/UBSan regression replay on toolchains
//                                 without the libFuzzer runtime.
//
// The active target is selected at process start from ECHOES_FUZZ_TARGET:
//   1=hello 2=command 3=batch 4=keyframe 5=delta 6=apply
//
// Every target enforces two properties beyond "no crash/UB":
//   - a packet the decoder accepts must re-encode to the identical bytes
//     (canonical-form round trip), and
//   - an accepted keyframe+delta pair must apply without violating the
//     delta's declared snapshot lineage.
// A violated property aborts, which the fuzzer records as a crash artifact.

#include "EchoesSimCore/NetworkProtocol.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <span>
#include <string>
#include <vector>

#if defined(ECHOES_FUZZ_WRITE_SEEDS) || defined(ECHOES_FUZZ_REGRESSION_MAIN)
#include <fstream>
#endif

namespace {

using namespace echoes::sim;
using namespace echoes::sim::net;

void Require(bool condition, const char* property) {
    if (!condition) {
        std::fprintf(
            stderr, "ECHOES_FUZZ_PROPERTY_VIOLATED: %s\n", property);
        std::abort();
    }
}

void FuzzCompatibilityHello(std::span<const std::uint8_t> bytes) {
    CompatibilityManifest manifest{};
    if (DecodeCompatibilityHello(bytes, manifest) != DecodeStatus::Ok) {
        return;
    }
    const std::vector<std::uint8_t> reencoded =
        EncodeCompatibilityHello(manifest);
    Require(reencoded.size() == bytes.size() &&
                std::memcmp(reencoded.data(), bytes.data(), bytes.size()) == 0,
            "accepted CompatibilityHello re-encodes byte-identically");
}

void FuzzCommandRequest(std::span<const std::uint8_t> bytes) {
    CommandRequest request{};
    if (DecodeCommandRequest(bytes, request) != DecodeStatus::Ok) {
        return;
    }
    const std::vector<std::uint8_t> reencoded = EncodeCommandRequest(request);
    Require(reencoded.size() == bytes.size() &&
                std::memcmp(reencoded.data(), bytes.data(), bytes.size()) == 0,
            "accepted CommandRequest re-encodes byte-identically");
}

void FuzzCommandBatchRequest(std::span<const std::uint8_t> bytes) {
    CommandBatchRequest batch{};
    if (DecodeCommandBatchRequest(bytes, batch) != DecodeStatus::Ok) {
        return;
    }
    const std::vector<std::uint8_t> reencoded =
        EncodeCommandBatchRequest(batch);
    Require(reencoded.size() == bytes.size() &&
                std::memcmp(reencoded.data(), bytes.data(), bytes.size()) == 0,
            "accepted CommandBatchRequest re-encodes byte-identically");
}

void FuzzScopedViewKeyframe(std::span<const std::uint8_t> bytes) {
    ScopedViewKeyframe keyframe{};
    if (DecodeScopedViewKeyframe(bytes, keyframe) != DecodeStatus::Ok) {
        return;
    }
    const std::vector<std::uint8_t> reencoded =
        EncodeScopedViewKeyframe(keyframe);
    Require(reencoded.size() == bytes.size() &&
                std::memcmp(reencoded.data(), bytes.data(), bytes.size()) == 0,
            "accepted ScopedViewKeyframe re-encodes byte-identically");
}

void FuzzScopedViewDelta(std::span<const std::uint8_t> bytes) {
    ScopedViewDelta delta{};
    if (DecodeScopedViewDelta(bytes, delta) != DecodeStatus::Ok) {
        return;
    }
    const std::vector<std::uint8_t> reencoded = EncodeScopedViewDelta(delta);
    Require(reencoded.size() == bytes.size() &&
                std::memcmp(reencoded.data(), bytes.data(), bytes.size()) == 0,
            "accepted ScopedViewDelta re-encodes byte-identically");
}

// Input layout: 4-byte little-endian keyframe length, keyframe bytes, then
// delta bytes. Both sections must independently decode before apply runs.
void FuzzApplyScopedViewDelta(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < 4) {
        return;
    }
    const std::size_t keyframeLength =
        static_cast<std::size_t>(bytes[0]) |
        static_cast<std::size_t>(bytes[1]) << 8 |
        static_cast<std::size_t>(bytes[2]) << 16 |
        static_cast<std::size_t>(bytes[3]) << 24;
    if (keyframeLength > bytes.size() - 4) {
        return;
    }
    const std::span<const std::uint8_t> keyframeBytes =
        bytes.subspan(4, keyframeLength);
    const std::span<const std::uint8_t> deltaBytes =
        bytes.subspan(4 + keyframeLength);
    ScopedViewKeyframe base{};
    ScopedViewDelta delta{};
    if (DecodeScopedViewKeyframe(keyframeBytes, base) != DecodeStatus::Ok ||
        DecodeScopedViewDelta(deltaBytes, delta) != DecodeStatus::Ok) {
        return;
    }
    ScopedViewKeyframe current{};
    std::string error;
    if (!ApplyScopedViewDelta(base, delta, current, &error)) {
        return;
    }
    Require(current.snapshotId == delta.snapshotId &&
                current.simulationTick == delta.simulationTick,
            "applied delta preserves declared snapshot lineage");
}

using FuzzTargetFunction = void (*)(std::span<const std::uint8_t>);

struct FuzzTarget final {
    const char* name;
    FuzzTargetFunction function;
};

[[maybe_unused]] constexpr FuzzTarget kFuzzTargets[] = {
    {"hello", &FuzzCompatibilityHello},
    {"command", &FuzzCommandRequest},
    {"batch", &FuzzCommandBatchRequest},
    {"keyframe", &FuzzScopedViewKeyframe},
    {"delta", &FuzzScopedViewDelta},
    {"apply", &FuzzApplyScopedViewDelta},
};
[[maybe_unused]] constexpr std::size_t kFuzzTargetCount =
    sizeof(kFuzzTargets) / sizeof(kFuzzTargets[0]);

#if !defined(ECHOES_FUZZ_WRITE_SEEDS)
const FuzzTarget& SelectedFuzzTarget() {
    static const FuzzTarget& target = [chosen = std::getenv(
                                           "ECHOES_FUZZ_TARGET")]()
        -> const FuzzTarget& {
        const long index = chosen != nullptr ? std::strtol(chosen, nullptr, 10)
                                             : 0;
        if (index < 1 || static_cast<std::size_t>(index) > kFuzzTargetCount) {
            std::fprintf(
                stderr,
                "ECHOES_FUZZ_TARGET must be 1..%zu "
                "(1=hello 2=command 3=batch 4=keyframe 5=delta 6=apply).\n",
                kFuzzTargetCount);
            std::exit(2);
        }
        return kFuzzTargets[index - 1];
    }();
    return target;
}
#endif

#if defined(ECHOES_FUZZ_WRITE_SEEDS) || defined(ECHOES_FUZZ_REGRESSION_MAIN)
[[nodiscard]] ScopedViewKeyframe CanonicalMinimalKeyframe() {
    ScopedViewKeyframe keyframe{};
    keyframe.snapshotId = 1;
    keyframe.mapWidthTiles = 1;
    keyframe.mapHeightTiles = 1;
    keyframe.tiles.push_back(ScopedTileState{});
    const std::vector<std::uint8_t> encoded =
        EncodeScopedViewKeyframe(keyframe);
    ScopedViewKeyframe canonical{};
    if (encoded.empty() ||
        DecodeScopedViewKeyframe(encoded, canonical) != DecodeStatus::Ok) {
        std::fprintf(stderr, "Minimal keyframe seed failed to round-trip.\n");
        std::exit(1);
    }
    return canonical;
}
#endif

#if defined(ECHOES_FUZZ_WRITE_SEEDS)
[[nodiscard]] ScopedViewKeyframe CanonicalizeKeyframe(
    const ScopedViewKeyframe& keyframe) {
    const std::vector<std::uint8_t> encoded =
        EncodeScopedViewKeyframe(keyframe);
    ScopedViewKeyframe canonical{};
    if (encoded.empty() ||
        DecodeScopedViewKeyframe(encoded, canonical) != DecodeStatus::Ok) {
        std::fprintf(stderr, "Keyframe seed failed to canonicalize.\n");
        std::exit(1);
    }
    return canonical;
}

bool WriteSeedFile(const std::string& path,
                   std::span<const std::uint8_t> bytes) {
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    stream.write(reinterpret_cast<const char*>(bytes.data()),
                 static_cast<std::streamsize>(bytes.size()));
    return static_cast<bool>(stream);
}
#endif

}  // namespace

#if defined(ECHOES_FUZZ_WRITE_SEEDS)

int main(int argc, char** argv) {
    if (argc != 2) {
        std::fprintf(stderr, "Usage: %s <output-directory>\n", argv[0]);
        return 2;
    }
    const std::string outDir(argv[1]);

    const std::vector<std::uint8_t> helloBytes =
        EncodeCompatibilityHello(CompatibilityManifest{});

    CommandRequest command{};
    command.sequence = 1;
    command.executeTick = 10;
    command.actor = 1;
    const std::vector<std::uint8_t> commandBytes =
        EncodeCommandRequest(command);

    CommandBatchRequest batch{};
    batch.clientBatchId = 1;
    CommandIntent intent{};
    intent.actor = 1;
    batch.intents.push_back(intent);
    const std::vector<std::uint8_t> batchBytes =
        EncodeCommandBatchRequest(batch);

    const ScopedViewKeyframe base = CanonicalMinimalKeyframe();
    const std::vector<std::uint8_t> keyframeBytes =
        EncodeScopedViewKeyframe(base);

    ScopedViewKeyframe next = base;
    next.snapshotId = base.snapshotId + 1;
    const ScopedViewKeyframe canonicalNext = CanonicalizeKeyframe(next);
    ScopedViewDelta delta{};
    std::string error;
    if (!BuildScopedViewDelta(base, canonicalNext, delta, &error)) {
        std::fprintf(
            stderr, "Delta seed construction failed: %s\n", error.c_str());
        return 1;
    }
    const std::vector<std::uint8_t> deltaBytes = EncodeScopedViewDelta(delta);

    std::vector<std::uint8_t> applyBytes;
    applyBytes.push_back(
        static_cast<std::uint8_t>(keyframeBytes.size() & 0xffU));
    applyBytes.push_back(
        static_cast<std::uint8_t>((keyframeBytes.size() >> 8) & 0xffU));
    applyBytes.push_back(
        static_cast<std::uint8_t>((keyframeBytes.size() >> 16) & 0xffU));
    applyBytes.push_back(
        static_cast<std::uint8_t>((keyframeBytes.size() >> 24) & 0xffU));
    applyBytes.insert(
        applyBytes.end(), keyframeBytes.begin(), keyframeBytes.end());
    applyBytes.insert(applyBytes.end(), deltaBytes.begin(), deltaBytes.end());

    if (helloBytes.empty() || commandBytes.empty() || batchBytes.empty() ||
        keyframeBytes.empty() || deltaBytes.empty() ||
        !WriteSeedFile(outDir + "/hello.bin", helloBytes) ||
        !WriteSeedFile(outDir + "/command.bin", commandBytes) ||
        !WriteSeedFile(outDir + "/batch.bin", batchBytes) ||
        !WriteSeedFile(outDir + "/keyframe.bin", keyframeBytes) ||
        !WriteSeedFile(outDir + "/delta.bin", deltaBytes) ||
        !WriteSeedFile(outDir + "/apply.bin", applyBytes)) {
        std::fprintf(stderr, "Seed generation failed under %s\n",
                     outDir.c_str());
        return 1;
    }
    std::printf("Wrote 6 canonical seed packets under %s\n", outDir.c_str());
    return 0;
}

#else  // fuzzer or regression mode

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data,
                                      std::size_t size) {
    SelectedFuzzTarget().function(
        std::span<const std::uint8_t>(data, size));
    return 0;
}

#if defined(ECHOES_FUZZ_REGRESSION_MAIN)
int main(int argc, char** argv) {
    const FuzzTarget& target = SelectedFuzzTarget();
    // Self-check: the seed round-trip properties must hold on this build
    // before any corpus replay is meaningful.
    const ScopedViewKeyframe base = CanonicalMinimalKeyframe();
    const std::vector<std::uint8_t> keyframeBytes =
        EncodeScopedViewKeyframe(base);
    FuzzScopedViewKeyframe(keyframeBytes);
    int replayed = 0;
    for (int index = 1; index < argc; ++index) {
        std::ifstream stream(argv[index], std::ios::binary);
        if (!stream) {
            std::fprintf(stderr, "Cannot read %s\n", argv[index]);
            return 2;
        }
        const std::vector<std::uint8_t> bytes(
            (std::istreambuf_iterator<char>(stream)),
            std::istreambuf_iterator<char>());
        target.function(std::span<const std::uint8_t>(bytes));
        ++replayed;
    }
    std::printf(
        "Replayed %d corpus file(s) through target '%s' cleanly.\n",
        replayed,
        target.name);
    return 0;
}
#endif

#endif  // ECHOES_FUZZ_WRITE_SEEDS
