// Shipping skirmish map geometry, in plain C++ so every consumer reads ONE
// definition.
//
// Author and owner: Angelis Pseftis
//
// This was previously only inside FEchoesSkirmishSetupModel in the Unreal
// module, which made it unreachable from the headless balance harness. The
// harness therefore measured a synthetic fixture instead and reported that
// gap itself ("synthetic single-map fixture is not the three shipping maps"),
// so every balance number it produced described a map the game does not ship.
// Transcribing the geometry into the harness would have created a second copy
// free to drift; extracting it here does not.
//
// SPEC-SKM-011..013 fix the three battlefields at 64x64 tiles on a 100 cm
// simulation scale. The MAP-001 fairness derivations that positioned the Wells
// and deposits are preserved with the data they justify, because a measured
// placement whose reasoning is separated from it becomes a magic number at the
// first edit.
//
// This is geometry authored as code. Glass Scar additionally has a registered
// pack under Content/World/Source/GlassScar; Crownfall Basin and Soryn
// Confluence have none, so for those two this header IS the source rather than
// a generated output. That gap belongs to the world lane and is recorded
// rather than deepened here.

#pragma once

#include <array>
#include <cstdint>

#include "EchoesSimCore/Simulation.h"

namespace echoes::sim {

enum class SkirmishMapPreset : std::uint8_t {
    GlassScar = 0,
    CrownfallBasin = 1,
    SorynConfluence = 2,
};

inline constexpr std::array<SkirmishMapPreset, 3> kSkirmishMapPresets = {
    SkirmishMapPreset::GlassScar,
    SkirmishMapPreset::CrownfallBasin,
    SkirmishMapPreset::SorynConfluence,
};

inline constexpr std::int32_t kSkirmishMapWidthTiles = 64;
inline constexpr std::int32_t kSkirmishMapHeightTiles = 64;

struct SkirmishTile final {
    std::int32_t x = 0;
    std::int32_t y = 0;

    friend constexpr bool operator==(const SkirmishTile&,
                                     const SkirmishTile&) = default;
};

[[nodiscard]] constexpr const char* SkirmishMapPresetId(
    SkirmishMapPreset preset) {
    switch (preset) {
        case SkirmishMapPreset::GlassScar: return "GlassScar";
        case SkirmishMapPreset::CrownfallBasin: return "CrownfallBasin";
        case SkirmishMapPreset::SorynConfluence: return "SorynConfluence";
    }
    return "Unknown";
}

/** Blocked ground for one shipping battlefield. Out-of-range tiles report
 * blocked, which keeps callers that walk beyond the grid from reading open
 * ground off the edge of the map. */
[[nodiscard]] constexpr bool IsSkirmishBlockedTile(SkirmishMapPreset preset,
                                                   std::int32_t tileX,
                                                   std::int32_t tileY) {
    if (tileX < 0 || tileY < 0 || tileX >= kSkirmishMapWidthTiles ||
        tileY >= kSkirmishMapHeightTiles) {
        return true;
    }
    switch (preset) {
        case SkirmishMapPreset::GlassScar: {
            const bool inScar =
                tileY >= 30 && tileY <= 34 && tileX >= 8 && tileX <= 55;
            const bool crossing = (tileX >= 12 && tileX <= 15) ||
                                  (tileX >= 29 && tileX <= 35) ||
                                  (tileX >= 48 && tileX <= 51);
            return inScar && !crossing;
        }
        case SkirmishMapPreset::CrownfallBasin: {
            const bool ridge =
                ((tileX >= 27 && tileX <= 29) || (tileX >= 35 && tileX <= 37)) &&
                tileY >= 6 && tileY <= 57;
            const bool gate = (tileY >= 13 && tileY <= 17) ||
                              (tileY >= 30 && tileY <= 34) ||
                              (tileY >= 46 && tileY <= 50);
            const bool northShelf = tileY >= 39 && tileY <= 41 && tileX >= 10 &&
                                    tileX <= 22 && !(tileX >= 15 && tileX <= 17);
            const bool southShelf = tileY >= 22 && tileY <= 24 && tileX >= 42 &&
                                    tileX <= 54 && !(tileX >= 47 && tileX <= 49);
            return (ridge && !gate) || northShelf || southShelf;
        }
        case SkirmishMapPreset::SorynConfluence: {
            const bool outerHorizontal =
                (tileY == 19 || tileY == 20 || tileY == 43 || tileY == 44) &&
                tileX >= 20 && tileX <= 43;
            const bool outerVertical =
                (tileX == 20 || tileX == 21 || tileX == 42 || tileX == 43) &&
                tileY >= 19 && tileY <= 44;
            const bool northSouthGate = tileX >= 30 && tileX <= 33;
            const bool westEastGate = tileY >= 30 && tileY <= 33;
            const bool ring = (outerHorizontal && !northSouthGate) ||
                              (outerVertical && !westEastGate);
            const bool westShard =
                tileX >= 9 && tileX <= 16 && tileY >= 25 && tileY <= 27;
            const bool eastShard =
                tileX >= 47 && tileX <= 54 && tileY >= 36 && tileY <= 38;
            return ring || westShard || eastShard;
        }
    }
    return true;
}

[[nodiscard]] constexpr SkirmishTile SkirmishFutureWellTile(
    SkirmishMapPreset preset) {
    switch (preset) {
        case SkirmishMapPreset::GlassScar: return {32, 32};
        // MAP-001 fairness correction. The Well sat at 32,39: 35 tiles from the
        // northwest start and 49 from the southeast one, 28.6% apart against a
        // 5% ceiling. Because the two starts lie on a northwest-southeast
        // diagonal, the equidistant locus on this battlefield runs along the
        // opposing diagonal, and no due-north tile is reachable equally by both
        // forces. 34,34 is the northernmost equidistant tile inside the central
        // corridor between the twin ridges (x 30-34) and inside the middle gate
        // band (y 30-34), so it stays neutral ground, stays north of centre,
        // and measures 42 tiles from either Command Core.
        case SkirmishMapPreset::CrownfallBasin: return {34, 34};
        case SkirmishMapPreset::SorynConfluence: return {32, 32};
    }
    return {-1, -1};
}

// Index 0 is the Command Core and index 2 is the starting Dropoff; MAP-001's
// "resource travel time" is worker haul time, which is measured to whichever of
// those two is nearer, so the Dropoff placement is part of the fairness
// contract and not free decoration. Every Dropoff below is measured, not eyed.
[[nodiscard]] constexpr std::array<SkirmishTile, 12> SkirmishLocalSpawnTiles(
    SkirmishMapPreset preset) {
    switch (preset) {
        case SkirmishMapPreset::GlassScar:
            // Dropoff 6,17 -> 6,14. The Barracks at 14,10 is 4x4
            // (SPEC-STR-003), so 14,12 and 16,10 are inside its footprint and
            // 8,8 is inside the 5x5 Core; the worker, Soldier and route scout
            // that used those tiles now stand one tile clear of them.
            return {{{10, 10}, {14, 10}, {6, 14}, {8, 13}, {11, 14},
                     {14, 13}, {6, 8}, {12, 7}, {17, 10}, {7, 6},
                     {15, 6}, {6, 11}}};
        case SkirmishMapPreset::CrownfallBasin:
            // Dropoff 6,45 -> 3,47.
            return {{{10, 52}, {14, 52}, {3, 47}, {8, 49}, {11, 48},
                     {14, 49}, {8, 55}, {12, 57}, {17, 54}, {7, 58},
                     {15, 58}, {6, 53}}};
        case SkirmishMapPreset::SorynConfluence:
            // Dropoff 13,38 -> 10,38.
            return {{{8, 32}, {8, 21}, {10, 38}, {11, 30}, {12, 34},
                     {15, 32}, {6, 29}, {6, 35}, {12, 40}, {5, 38},
                     {15, 42}, {13, 24}}};
    }
    return {};
}

[[nodiscard]] constexpr std::array<SkirmishTile, 11> SkirmishOpponentSpawnTiles(
    SkirmishMapPreset preset) {
    switch (preset) {
        case SkirmishMapPreset::GlassScar:
            // Dropoff 58,48 -> 58,50. Workers stand clear of the 4x4
            // Barracks at 50,54 and the 2x2 Aegis Post at 58,53
            // (SPEC-STR-003/004 footprints, SPEC-MOV-006 standing room).
            return {{{54, 54}, {50, 54}, {58, 50}, {51, 51}, {54, 50},
                     {56, 51}, {50, 57}, {54, 58}, {57, 58},
                     {49, 58}, {58, 53}}};
        case SkirmishMapPreset::CrownfallBasin:
            // Dropoff 58,19 -> 61,17.
            return {{{54, 12}, {50, 12}, {61, 17}, {49, 15}, {54, 16},
                     {57, 14}, {50, 9}, {54, 6}, {57, 6},
                     {49, 6}, {58, 11}}};
        case SkirmishMapPreset::SorynConfluence:
            // Dropoff 51,26 -> 54,26.
            return {{{56, 32}, {56, 43}, {54, 26}, {53, 34}, {52, 30},
                     {49, 32}, {58, 35}, {58, 29}, {59, 26},
                     {49, 22}, {51, 40}}};
    }
    return {};
}

// MAP-001 fairness correction, derived by breadth-first search over the
// shipping terrain rather than by eye. Each deposit is placed so the eight
// tiles form four swap-matched pairs: for every deposit A that is n tiles from
// one force and m from the other, its partner B is m from the first and n from
// the second. That makes the two forces' sorted distance ladders identical -
// 0% apart at every rank, not merely inside MAP-001's 5% ceiling - without
// requiring the terrain itself to be symmetric, which on these three
// battlefields it is not.
//
// Glass Scar moved four deposits by 1, 3, 4 and 1 tiles; Crownfall Basin moved
// two, one by 1 tile and one by 9; Soryn Confluence needed no deposit moved.
// Measured worst-case disparity after the change, on all three maps and from
// both the Command Core and the worker haul anchor: 0.0%.
[[nodiscard]] constexpr std::array<SkirmishTile, 8> SkirmishResourceNodeTiles(
    SkirmishMapPreset preset) {
    switch (preset) {
        case SkirmishMapPreset::GlassScar:
            // 47,50 -> 46,50   52,45 -> 49,45
            // 43,36 -> 39,36   31,43 -> 30,43
            return {{{16, 16}, {21, 13}, {25, 28}, {33, 22},
                     {30, 43}, {39, 36}, {46, 50}, {49, 45}}};
        case SkirmishMapPreset::CrownfallBasin:
            // 32,27 -> 32,18 (kept in the central corridor between the ridges)
            // 56,22 -> 55,22
            return {{{15, 46}, {20, 53}, {20, 34}, {32, 18},
                     {34, 48}, {44, 30}, {48, 17}, {55, 22}}};
        case SkirmishMapPreset::SorynConfluence:
            return {{{15, 18}, {17, 47}, {26, 27}, {27, 37},
                     {37, 27}, {38, 37}, {47, 17}, {49, 46}}};
    }
    return {};
}

/** The shipping skirmish force, in spawn-tile order. Shared so the headless
 * harness fields the same army the game does: the tile lists and these type
 * lists are positional, and a mismatch between them is silent - the wrong unit
 * simply appears on the wrong tile. Local has twelve entries and the opponent
 * eleven, matching SkirmishLocalSpawnTiles and SkirmishOpponentSpawnTiles. */
inline constexpr std::array<EntityType, 12> kSkirmishLocalForce = {
    EntityType::CommandCore,
    EntityType::Barracks,
    EntityType::Dropoff,
    EntityType::Worker,
    EntityType::Worker,
    EntityType::Worker,
    EntityType::Soldier,
    EntityType::Soldier,
    EntityType::Soldier,
    EntityType::HeavyUnit,
    EntityType::ScoutUnit,
    EntityType::UtilityStructure,
};

inline constexpr std::array<EntityType, 11> kSkirmishOpponentForce = {
    EntityType::CommandCore,
    EntityType::Barracks,
    EntityType::Dropoff,
    EntityType::Worker,
    EntityType::Worker,
    EntityType::Worker,
    EntityType::Soldier,
    EntityType::Soldier,
    EntityType::HeavyUnit,
    EntityType::ScoutUnit,
    EntityType::UtilityStructure,
};

/** SPEC-SKM-006 starting pools. Standard is the balance-matrix condition. */
inline constexpr std::int32_t kSkirmishStandardMaterial = 400;
inline constexpr std::int32_t kSkirmishStandardDawn = 30;

/** Matter per authored deposit in a skirmish, as the runtime spawns them. */
inline constexpr std::int32_t kSkirmishDepositAmount = 1500;

}  // namespace echoes::sim
