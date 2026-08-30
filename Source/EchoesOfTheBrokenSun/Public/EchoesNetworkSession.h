#pragma once

#include "CoreMinimal.h"
#include "EchoesSimCore/NetworkProtocol.h"

namespace echoes::network {

/** Exact compatibility identity for the first Glass Scar listen-server slice. */
[[nodiscard]] ECHOESOFTHEBROKENSUN_API sim::net::CompatibilityManifest
BuildCompatibilityManifest(const sim::Simulation* simulation = nullptr);

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
