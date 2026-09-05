// Author: Angelis Pseftis
#pragma once
#include "EchoesSimCore/Simulation.h"
#include <cstdint>
#include <string_view>

namespace echoes::world
{
struct CampaignTerrainResult final
{
    bool ok = false;
    std::int32_t blocked_cells = 0;
    const char* map_id = "";
    const char* source_sha256 = "";
    const char* detail = "";
    const char* terrain_identity_sha256 = "";
};
// Mission ordinal is the stable persisted M01..M15 ID, never an operation enum cast.
// Applies only before entities/ticks exist. Invalid identity/data refuses before writes.
[[nodiscard]] CampaignTerrainResult ApplyCampaignTerrain(
    echoes::sim::Simulation& simulation, std::uint8_t mission_ordinal,
    echoes::sim::FutureWellChoice doctrine);
[[nodiscard]] CampaignTerrainResult CheckCampaignTerrain(
    std::uint8_t mission_ordinal, echoes::sim::FutureWellChoice doctrine);
[[nodiscard]] bool IsCampaignTerrainPassable(std::uint8_t mission_ordinal,
    echoes::sim::FutureWellChoice doctrine, std::int32_t x, std::int32_t y);
// Returns one source-defined semantic site; leaves outputs unchanged on refusal.
[[nodiscard]] bool FindCampaignMapAnchor(std::uint8_t mission_ordinal,
    std::string_view id, std::int32_t& x, std::int32_t& y);
}
