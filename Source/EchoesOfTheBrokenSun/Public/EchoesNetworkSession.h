#pragma once

#include "CoreMinimal.h"
#include "EchoesSimCore/NetworkProtocol.h"

#include <optional>

namespace echoes::network {

/** Exact compatibility identity for the first Glass Scar listen-server slice. */
[[nodiscard]] ECHOESOFTHEBROKENSUN_API sim::net::CompatibilityManifest
BuildCompatibilityManifest(const sim::Simulation* simulation = nullptr);

/** Fixed-window per-connection command budget for the current transport slice. */
class ECHOESOFTHEBROKENSUN_API CommandRateLimiter final
{
public:
    // Retained name for the accepted 0.80 request-window contract.
    static constexpr std::uint32_t MaximumCommandsPerWindow = 8;
    static constexpr std::uint32_t MaximumIntentsPerWindow = 1024;
    static constexpr double WindowSeconds = 1.0;

    [[nodiscard]] bool TryConsume(
        double nowSeconds,
        std::uint32_t intentCount = 1);
    [[nodiscard]] std::uint32_t CurrentCount() const { return requestCount_; }
    [[nodiscard]] std::uint32_t CurrentIntentCount() const
    {
        return intentCount_;
    }

private:
    double windowStartSeconds_ = -1.0;
    std::uint32_t requestCount_ = 0;
    std::uint32_t intentCount_ = 0;
};

enum class ScopedViewAcceptance : std::uint8_t
{
    AcceptedFirst = 0,
    AcceptedNext,
    AcceptedRecovery,
    AcceptedDelta,
    InvalidSnapshot,
    PlayerChanged,
    StaleOrDuplicate,
    BaseMissing,
    DeltaRejected,
};

/** Client-owned, visibility-scoped authoritative state with monotonic lineage. */
class ECHOESOFTHEBROKENSUN_API ScopedViewState final
{
public:
    [[nodiscard]] ScopedViewAcceptance Accept(
        const sim::net::ScopedViewKeyframe& keyframe);
    [[nodiscard]] ScopedViewAcceptance AcceptDelta(
        const sim::net::ScopedViewDelta& delta,
        std::string* error = nullptr);
    [[nodiscard]] const std::optional<sim::net::ScopedViewKeyframe>& Current()
        const
    {
        return current_;
    }
    [[nodiscard]] std::uint64_t AcceptedCount() const
    {
        return acceptedCount_;
    }

private:
    std::optional<sim::net::ScopedViewKeyframe> current_{};
    std::uint64_t acceptedCount_ = 0;
};

[[nodiscard]] ECHOESOFTHEBROKENSUN_API const char* StableId(
    ScopedViewAcceptance acceptance);

[[nodiscard]] inline std::span<const std::uint8_t> AsByteSpan(
    const TArray<uint8>& bytes) {
    return {bytes.GetData(), static_cast<std::size_t>(bytes.Num())};
}

[[nodiscard]] inline TArray<uint8> ToByteArray(
    const std::vector<std::uint8_t>& bytes) {
    TArray<uint8> converted;
    if (!bytes.empty()) {
        converted.Append(bytes.data(), static_cast<int32>(bytes.size()));
    }
    return converted;
}

}  // namespace echoes::network
