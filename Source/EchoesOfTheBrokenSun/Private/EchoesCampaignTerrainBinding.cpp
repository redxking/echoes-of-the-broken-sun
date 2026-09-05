// Author: Angelis Pseftis
#include "EchoesCampaignTerrainBinding.h"
#include "../../../Content/World/Generated/Campaign/EchoesCampaignMapPack.h"
#include <array>

namespace echoes::world
{
bool FindCampaignMapAnchor(std::uint8_t ordinal, std::string_view id, std::int32_t& x, std::int32_t& y)
{
    const campaign_map_pack::MissionAnchor* Match = nullptr;
    for (const auto& Anchor : campaign_map_pack::kMissionAnchors)
        if (Anchor.mission_ordinal == ordinal && Anchor.id == id)
        {
            if (Match) return false;
            Match = &Anchor;
        }
    if (!Match || Match->x < 0 || Match->y < 0 || Match->x >= 64 || Match->y >= 64) return false;
    x = Match->x;
    y = Match->y;
    return true;
}
namespace
{
namespace campaign_terrain_pack = campaign_map_pack;
const campaign_terrain_pack::TerrainVariant* FindCampaignTerrainVariant(std::uint8_t ordinal, echoes::sim::FutureWellChoice doctrine)
{
    if (ordinal < 1 || ordinal > campaign_terrain_pack::kMissionIdentities.size()) return nullptr;
    std::uint8_t mask = 0;
    switch (doctrine)
    {
    case echoes::sim::FutureWellChoice::Harvest: mask = campaign_terrain_pack::kHarvestMask; break;
    case echoes::sim::FutureWellChoice::Preserve: mask = campaign_terrain_pack::kPreserveMask; break;
    case echoes::sim::FutureWellChoice::Reshape: mask = campaign_terrain_pack::kReshapeMask; break;
    default: return nullptr;
    }
    const campaign_terrain_pack::TerrainVariant* found = nullptr;
    for (const auto& variant : campaign_terrain_pack::kTerrainVariants)
        if (variant.mission_index == ordinal - 1 && variant.doctrine_mask == mask)
        {
            if (found) return nullptr;
            found = &variant;
        }
    return found;
}
}
CampaignTerrainResult CheckCampaignTerrain(std::uint8_t ordinal, echoes::sim::FutureWellChoice doctrine)
{
    const auto* variant = FindCampaignTerrainVariant(ordinal, doctrine);
    if (!variant || !variant->blocked_cells) return {false, 0, "", "", "missing or ambiguous mission/doctrine binding"};
    const auto& identity = campaign_terrain_pack::kMissionIdentities[ordinal - 1];
    if (identity.map_id.empty() || identity.source_sha256.size() != 64 ||
        identity.terrain_identity_sha256.size() != 64 || identity.doctrine_variant_mask != 7)
        return {false, 0, "", "", "invalid compiled identity"};
    std::int32_t blocked = 0, start = -1;
    for (std::int32_t i = 0; i < campaign_terrain_pack::kCellCount; ++i)
    {
        const auto value = (*variant->blocked_cells)[i];
        if (value > 1) return {false, 0, "", "", "invalid terrain cell"};
        blocked += value;
        if (!value && start < 0) start = i;
    }
    if (blocked != variant->expected_blocked_cell_count || start < 0)
        return {false, 0, "", "", "compiled terrain census mismatch"};
    std::array<bool, campaign_terrain_pack::kCellCount> seen{};
    std::array<std::int32_t, campaign_terrain_pack::kCellCount> queue{};
    std::int32_t head = 0, tail = 1;
    queue[0] = start; seen[start] = true;
    while (head < tail)
    {
        const auto cell = queue[head++];
        const auto x = cell % campaign_terrain_pack::kGridWidthTiles, y = cell / campaign_terrain_pack::kGridWidthTiles;
        const std::int32_t neighbours[] = {x > 0 ? cell-1 : -1,
            x+1 < campaign_terrain_pack::kGridWidthTiles ? cell+1 : -1,
            y > 0 ? cell-campaign_terrain_pack::kGridWidthTiles : -1,
            y+1 < campaign_terrain_pack::kGridHeightTiles ? cell+campaign_terrain_pack::kGridWidthTiles : -1};
        for (const auto next : neighbours)
            if (next >= 0 && !seen[next] && !(*variant->blocked_cells)[next])
            { seen[next] = true; queue[tail++] = next; }
    }
    if (tail != campaign_terrain_pack::kCellCount - blocked) return {false, 0, "", "", "disconnected campaign terrain"};
    return {true, blocked, identity.map_id.data(), identity.source_sha256.data(), "ok", identity.terrain_identity_sha256.data()};
}
CampaignTerrainResult ApplyCampaignTerrain(echoes::sim::Simulation& simulation,
    std::uint8_t ordinal, echoes::sim::FutureWellChoice doctrine)
{
    auto result = CheckCampaignTerrain(ordinal, doctrine);
    if (!result.ok) return result;
    if (simulation.Config().mapWidthTiles != campaign_terrain_pack::kGridWidthTiles ||
        simulation.Config().mapHeightTiles != campaign_terrain_pack::kGridHeightTiles ||
        simulation.CurrentTick() != 0 || !simulation.Entities().empty())
        return {false, 0, "", "", "campaign terrain requires a fresh matching grid"};
    const auto* variant = FindCampaignTerrainVariant(ordinal, doctrine);
    for (std::int32_t i = 0; i < campaign_terrain_pack::kCellCount; ++i)
        if (!simulation.SetTerrainTile(i % campaign_terrain_pack::kGridWidthTiles, i / campaign_terrain_pack::kGridWidthTiles,
            (*variant->blocked_cells)[i] ? echoes::sim::Terrain::Blocked : echoes::sim::Terrain::Open))
            return {false, 0, "", "", "campaign terrain write refused"};
    return result;
}
bool IsCampaignTerrainPassable(std::uint8_t ordinal, echoes::sim::FutureWellChoice doctrine,
    std::int32_t x, std::int32_t y)
{
    const auto* variant = FindCampaignTerrainVariant(ordinal, doctrine);
    return variant && variant->blocked_cells && x >= 0 && y >= 0 &&
        x < campaign_terrain_pack::kGridWidthTiles && y < campaign_terrain_pack::kGridHeightTiles &&
        (*variant->blocked_cells)[y * campaign_terrain_pack::kGridWidthTiles + x] == 0;
}
}
