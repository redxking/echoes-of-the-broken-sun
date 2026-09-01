// Compiled Glass Scar map binding: consumes the generated constant tables in
// EchoesGlassScarCompiledMapPack.h against the public EchoesSimCore API only.
// This module does not change runtime authority: ConfigureGlassScar in the
// simulation subsystem remains the live terrain author, and parity between the
// two is proven test-side. Engine-independent by design (no Unreal includes)
// so the binding can also be exercised by native tooling.

#pragma once

#include "EchoesSimCore/Simulation.h"

#include <cstdint>

namespace echoes::world
{

struct CompiledMapCheckResult final
{
    bool ok = false;
    // Static string describing the first failed invariant; "ok" when ok.
    const char* detail = "";
};

// Re-derives the compiled contract's structural invariants from the generated
// tables: cell census, mask/cost coupling, region cell counts, height and
// region ordinal validity, portal edge-pair census, objective placement, and
// camera bounds. Fails closed on the first violation.
[[nodiscard]] CompiledMapCheckResult VerifyCompiledGlassScarTables();

// Applies the compiled base Glass Scar terrain to the given simulation by
// setting every ground-impassable cell to Terrain::Blocked. Returns the number
// of tiles set, which must equal kExpectedBlockedCellCount on a fresh
// 64x64 simulation, or -1 when the simulation grid does not match the compiled
// grid or a tile write is rejected. Mission overlays are intentionally not
// applied; they remain runtime code owned elsewhere.
[[nodiscard]] std::int32_t ApplyCompiledGlassScar(echoes::sim::Simulation& Simulation);

// True when the compiled movement mask marks the tile ground-passable.
// Out-of-range coordinates are impassable.
[[nodiscard]] bool IsCompiledGroundPassable(std::int32_t TileX, std::int32_t TileY);

struct CompiledTile final
{
    std::int32_t X = 0;
    std::int32_t Y = 0;
};

[[nodiscard]] CompiledTile FutureWellPrimaryTile();
[[nodiscard]] std::int32_t FutureWellFallbackCount();
// Ordinal-indexed fallback tile; out-of-range ordinals return {-1, -1}.
[[nodiscard]] CompiledTile FutureWellFallbackTile(std::int32_t Ordinal);

struct CompiledCameraBounds final
{
    std::int32_t MinX = 0;
    std::int32_t MinY = 0;
    std::int32_t MaxXExclusive = 0;
    std::int32_t MaxYExclusive = 0;
};

[[nodiscard]] CompiledCameraBounds CameraTileBounds();

// Provenance digests pinned by the generated header.
[[nodiscard]] const char* CompiledPackSha256();
[[nodiscard]] const char* AuthoringCanonicalSha256();
[[nodiscard]] const char* BaseDescriptorCanonicalSha256();

}  // namespace echoes::world
