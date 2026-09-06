// GENERATED FILE - edit the registered Training staging source instead.
// Author and owner: Angelis Pseftis
#pragma once
#include <cstdint>
#include <string_view>
namespace echoes::world::training_staging {
struct Tile final { std::int32_t x; std::int32_t y; };
inline constexpr std::string_view kSourceSha256 = "a995100db4098e98a12263adb322c699addbcfdb4147f460f6e3966f0a231670";
inline constexpr std::string_view kOperation = "training-readiness";
inline constexpr Tile kLinkBuildSite{6, 14};
inline constexpr Tile kLinkRejectedSite{19, 10};
inline constexpr Tile kLinkRepairTargetSite{6, 17};
inline constexpr Tile kFoundryProducerSite{14, 10};
inline constexpr Tile kFoundryRallySite{14, 18};
inline constexpr std::int32_t kLinkRepairInitialHp = 440;
inline constexpr std::int32_t kLinkRepairMaxHp = 450;
inline constexpr std::int32_t kLinkCompletionRadiusSimCm = 200;
}
