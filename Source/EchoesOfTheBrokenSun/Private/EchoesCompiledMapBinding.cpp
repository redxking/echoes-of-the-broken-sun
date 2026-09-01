#include "EchoesCompiledMapBinding.h"

#include "EchoesGlassScarCompiledMapPack.h"

namespace echoes::world
{

namespace
{
namespace pack = echoes::world::glass_scar_pack;

[[nodiscard]] bool IsGroundPassableIndex(std::int32_t CellIndex)
{
    return (pack::kMovementMask[CellIndex] & pack::kGroundMovementMask) != 0;
}

[[nodiscard]] CompiledTile TileFromIndex(std::int32_t CellIndex)
{
    return CompiledTile{
        CellIndex % pack::kGridWidthTiles,
        CellIndex / pack::kGridWidthTiles};
}
}  // namespace

CompiledMapCheckResult VerifyCompiledGlassScarTables()
{
    std::int32_t BlockedCells = 0;
    std::int32_t RegionCellCounts[8] = {};
    for (std::int32_t Index = 0; Index < pack::kCellCount; ++Index)
    {
        const bool bGround = IsGroundPassableIndex(Index);
        const std::uint8_t Cost = pack::kBaseMoveCost[Index];
        const std::uint8_t Height = pack::kHeightBandOrdinal[Index];
        const std::uint8_t Region = pack::kRegionOrdinal[Index];
        if (!bGround)
        {
            ++BlockedCells;
            if (Cost != pack::kBlockedBaseMoveCost)
            {
                return {false, "blocked cell carries a nonzero base move cost"};
            }
            if (Region != 0)
            {
                return {false, "blocked cell carries a stable region ordinal"};
            }
        }
        else
        {
            if (Cost != pack::kPassableBaseMoveCost)
            {
                return {false, "passable cell diverges from the contract entry cost"};
            }
            if (Region == 0 || Region > 7)
            {
                return {false, "passable cell has no valid stable region ordinal"};
            }
        }
        if (Height >= 2)
        {
            return {false, "cell height band ordinal is out of range"};
        }
        ++RegionCellCounts[Region];
    }
    if (BlockedCells != pack::kExpectedBlockedCellCount)
    {
        return {false, "blocked cell census diverges from the contract"};
    }
    if (pack::kCellCount - BlockedCells != pack::kExpectedPassableCellCount)
    {
        return {false, "passable cell census diverges from the contract"};
    }
    for (const pack::FCompiledRegion& Region : pack::kRegions)
    {
        if (Region.Ordinal == 0 || Region.Ordinal > 7 ||
            RegionCellCounts[Region.Ordinal] != Region.CellCount)
        {
            return {false, "stable region cell count diverges from its record"};
        }
    }
    std::int32_t EdgePairTotal = 0;
    for (const pack::FCompiledPortal& Portal : pack::kPortals)
    {
        if (Portal.EdgePairCount <= 0)
        {
            return {false, "portal records no crossing edge pairs"};
        }
        EdgePairTotal += Portal.EdgePairCount;
    }
    if (EdgePairTotal != pack::kPortalEdgePairCount)
    {
        return {false, "portal edge-pair census diverges from the flattened table"};
    }
    for (const pack::FCompiledPortalEdgePair& Pair : pack::kPortalEdgePairs)
    {
        if (Pair.FirstCellIndex < 0 || Pair.FirstCellIndex >= pack::kCellCount ||
            Pair.SecondCellIndex < 0 || Pair.SecondCellIndex >= pack::kCellCount)
        {
            return {false, "portal edge pair references an out-of-range cell"};
        }
        if (!IsGroundPassableIndex(Pair.FirstCellIndex) ||
            !IsGroundPassableIndex(Pair.SecondCellIndex))
        {
            return {false, "portal edge pair references a blocked cell"};
        }
    }
    if (pack::kFutureWellPrimaryCellIndex < 0 ||
        pack::kFutureWellPrimaryCellIndex >= pack::kCellCount ||
        !IsGroundPassableIndex(pack::kFutureWellPrimaryCellIndex))
    {
        return {false, "future-well primary cell is blocked or out of range"};
    }
    for (const std::int32_t Fallback : pack::kFutureWellFallbackCellIndices)
    {
        if (Fallback < 0 || Fallback >= pack::kCellCount ||
            !IsGroundPassableIndex(Fallback))
        {
            return {false, "future-well fallback cell is blocked or out of range"};
        }
    }
    if (pack::kCameraMinX != 0 || pack::kCameraMinY != 0 ||
        pack::kCameraMaxXExclusive != pack::kGridWidthTiles ||
        pack::kCameraMaxYExclusive != pack::kGridHeightTiles)
    {
        return {false, "camera bounds diverge from the half-open grid contract"};
    }
    return {true, "ok"};
}

std::int32_t ApplyCompiledGlassScar(echoes::sim::Simulation& Simulation)
{
    const echoes::sim::SimulationConfig& Config = Simulation.Config();
    if (Config.mapWidthTiles != pack::kGridWidthTiles ||
        Config.mapHeightTiles != pack::kGridHeightTiles)
    {
        return -1;
    }
    std::int32_t BlockedTiles = 0;
    for (std::int32_t Index = 0; Index < pack::kCellCount; ++Index)
    {
        if (IsGroundPassableIndex(Index))
        {
            continue;
        }
        const CompiledTile Tile = TileFromIndex(Index);
        if (!Simulation.SetTerrainTile(
                Tile.X, Tile.Y, echoes::sim::Terrain::Blocked))
        {
            return -1;
        }
        ++BlockedTiles;
    }
    return BlockedTiles;
}

bool IsCompiledGroundPassable(std::int32_t TileX, std::int32_t TileY)
{
    if (TileX < 0 || TileX >= pack::kGridWidthTiles || TileY < 0 ||
        TileY >= pack::kGridHeightTiles)
    {
        return false;
    }
    return IsGroundPassableIndex(TileY * pack::kGridWidthTiles + TileX);
}

CompiledTile FutureWellPrimaryTile()
{
    return TileFromIndex(pack::kFutureWellPrimaryCellIndex);
}

std::int32_t FutureWellFallbackCount()
{
    return static_cast<std::int32_t>(
        sizeof(pack::kFutureWellFallbackCellIndices) /
        sizeof(pack::kFutureWellFallbackCellIndices[0]));
}

CompiledTile FutureWellFallbackTile(std::int32_t Ordinal)
{
    if (Ordinal < 0 || Ordinal >= FutureWellFallbackCount())
    {
        return CompiledTile{-1, -1};
    }
    return TileFromIndex(pack::kFutureWellFallbackCellIndices[Ordinal]);
}

CompiledCameraBounds CameraTileBounds()
{
    return CompiledCameraBounds{
        pack::kCameraMinX,
        pack::kCameraMinY,
        pack::kCameraMaxXExclusive,
        pack::kCameraMaxYExclusive};
}

const char* CompiledPackSha256()
{
    return pack::kCompiledPackSha256;
}

const char* AuthoringCanonicalSha256()
{
    return pack::kAuthoringCanonicalSha256;
}

const char* BaseDescriptorCanonicalSha256()
{
    return pack::kBaseDescriptorCanonicalSha256;
}

}  // namespace echoes::world
