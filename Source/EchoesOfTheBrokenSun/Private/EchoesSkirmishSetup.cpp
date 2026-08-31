#include "EchoesSkirmishSetup.h"

#include "Containers/Queue.h"

namespace
{
template <typename EnumType>
EnumType CycleEnum(EnumType Value, int32 Direction, int32 Count)
{
    const int32 Current = static_cast<int32>(Value);
    const int32 Step = Direction < 0 ? -1 : 1;
    return static_cast<EnumType>((Current + Step + Count) % Count);
}

bool IsPlayableFaction(echoes::sim::Faction Faction)
{
    return Faction == echoes::sim::Faction::MeridianCompact ||
        Faction == echoes::sim::Faction::KharuunAssemblies ||
        Faction == echoes::sim::Faction::HollowChoir;
}

bool IsKnownAi(echoes::sim::AiPersonality Personality)
{
    return Personality == echoes::sim::AiPersonality::Balanced ||
        Personality == echoes::sim::AiPersonality::Defensive ||
        Personality == echoes::sim::AiPersonality::Raider ||
        Personality == echoes::sim::AiPersonality::Economic ||
        Personality == echoes::sim::AiPersonality::Expansionist ||
        Personality == echoes::sim::AiPersonality::Adaptive;
}

bool IsInside(const FIntPoint& Tile)
{
    return Tile.X >= 0 &&
        Tile.X < FEchoesSkirmishSetupModel::MapWidthTiles &&
        Tile.Y >= 0 &&
        Tile.Y < FEchoesSkirmishSetupModel::MapHeightTiles;
}

bool IsTypedSpawnFootprintOpen(
    EEchoesSkirmishMapPreset Preset,
    echoes::sim::Faction Faction,
    echoes::sim::EntityType Type,
    const FIntPoint& Tile)
{
    const echoes::sim::SimulationRules Rules =
        echoes::sim::DefaultSimulationRules();
    const int32 HalfExtent = Rules.archetypes
        [static_cast<size_t>(Faction)]
        [static_cast<size_t>(Type)]
            .footprintHalfExtentRaw;
    const int32 CenterX = Tile.X * echoes::sim::kFixedScale;
    const int32 CenterY = Tile.Y * echoes::sim::kFixedScale;
    const int32 MinimumTileX =
        (CenterX - HalfExtent) / echoes::sim::kFixedScale;
    const int32 MaximumTileX =
        (CenterX + HalfExtent - 1) / echoes::sim::kFixedScale;
    const int32 MinimumTileY =
        (CenterY - HalfExtent) / echoes::sim::kFixedScale;
    const int32 MaximumTileY =
        (CenterY + HalfExtent - 1) / echoes::sim::kFixedScale;
    for (int32 TileY = MinimumTileY; TileY <= MaximumTileY; ++TileY)
    {
        for (int32 TileX = MinimumTileX; TileX <= MaximumTileX; ++TileX)
        {
            if (!IsInside({TileX, TileY}) ||
                FEchoesSkirmishSetupModel::IsBlockedTile(
                    Preset, TileX, TileY))
            {
                return false;
            }
        }
    }
    return true;
}

bool HasOpenPath(
    EEchoesSkirmishMapPreset Preset,
    const FIntPoint& Start,
    const FIntPoint& Goal)
{
    if (!IsInside(Start) || !IsInside(Goal) ||
        FEchoesSkirmishSetupModel::IsBlockedTile(Preset, Start.X, Start.Y) ||
        FEchoesSkirmishSetupModel::IsBlockedTile(Preset, Goal.X, Goal.Y))
    {
        return false;
    }

    TArray<bool> Visited;
    Visited.Init(
        false,
        FEchoesSkirmishSetupModel::MapWidthTiles *
            FEchoesSkirmishSetupModel::MapHeightTiles);
    TQueue<FIntPoint> Frontier;
    Frontier.Enqueue(Start);
    Visited[Start.Y * FEchoesSkirmishSetupModel::MapWidthTiles + Start.X] =
        true;
    const FIntPoint Directions[] = {
        {1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    FIntPoint Current;
    while (Frontier.Dequeue(Current))
    {
        if (Current == Goal)
        {
            return true;
        }
        for (const FIntPoint& Direction : Directions)
        {
            const FIntPoint Next = Current + Direction;
            if (!IsInside(Next) ||
                FEchoesSkirmishSetupModel::IsBlockedTile(
                    Preset, Next.X, Next.Y))
            {
                continue;
            }
            const int32 Index =
                Next.Y * FEchoesSkirmishSetupModel::MapWidthTiles + Next.X;
            if (!Visited[Index])
            {
                Visited[Index] = true;
                Frontier.Enqueue(Next);
            }
        }
    }
    return false;
}
}

FEchoesSkirmishSetup FEchoesSkirmishSetupModel::DefaultSetup()
{
    return FEchoesSkirmishSetup{};
}

FEchoesSkirmishSetup FEchoesSkirmishSetupModel::CanonicalOnlineSetup()
{
    FEchoesSkirmishSetup Setup;
    Setup.LocalFaction = echoes::sim::Faction::MeridianCompact;
    Setup.OpponentFaction = echoes::sim::Faction::KharuunAssemblies;
    Setup.MapPreset = EEchoesSkirmishMapPreset::GlassScar;
    Setup.AiPersonality = echoes::sim::AiPersonality::Adaptive;
    Setup.ResourceLevel = EEchoesSkirmishResourceLevel::Standard;
    return Setup;
}

bool FEchoesSkirmishSetupModel::IsCanonicalOnlineSetup(
    const FEchoesSkirmishSetup& Setup)
{
    return Setup == CanonicalOnlineSetup();
}

bool FEchoesSkirmishSetupModel::Validate(
    const FEchoesSkirmishSetup& Setup,
    FString& OutError)
{
    OutError.Reset();
    if (!IsPlayableFaction(Setup.LocalFaction) ||
        !IsPlayableFaction(Setup.OpponentFaction))
    {
        OutError = TEXT("[SKIRMISH_FACTION_INVALID] Select two playable forces.");
        return false;
    }
    if (Setup.LocalFaction == Setup.OpponentFaction)
    {
        OutError = TEXT("[SKIRMISH_MATCHUP_INVALID] Local and opposing forces must be different.");
        return false;
    }
    if (static_cast<uint8>(Setup.MapPreset) >
        static_cast<uint8>(EEchoesSkirmishMapPreset::SorynConfluence))
    {
        OutError = TEXT("[SKIRMISH_MAP_INVALID] Select an authored battlefield.");
        return false;
    }
    if (!IsKnownAi(Setup.AiPersonality))
    {
        OutError = TEXT("[SKIRMISH_AI_INVALID] Select a supported AI profile.");
        return false;
    }
    if (static_cast<uint8>(Setup.ResourceLevel) >
        static_cast<uint8>(EEchoesSkirmishResourceLevel::Abundant))
    {
        OutError = TEXT("[SKIRMISH_RESOURCES_INVALID] Select a supported resource level.");
        return false;
    }

    const TArray<FIntPoint> LocalSpawns = LocalSpawnTiles(Setup.MapPreset);
    const TArray<FIntPoint> OpponentSpawns =
        OpponentSpawnTiles(Setup.MapPreset);
    const TArray<FIntPoint> Resources = ResourceNodeTiles(Setup.MapPreset);
    const FIntPoint Well = FutureWellTile(Setup.MapPreset);
    if (LocalSpawns.Num() != 12 || OpponentSpawns.Num() != 11 ||
        Resources.Num() < 6 || !IsInside(Well))
    {
        OutError = TEXT("[SKIRMISH_MAP_CONTRACT_INVALID] Battlefield deployment data is incomplete.");
        return false;
    }

    static constexpr echoes::sim::EntityType LocalTypes[] = {
        echoes::sim::EntityType::CommandCore,
        echoes::sim::EntityType::Barracks,
        echoes::sim::EntityType::Dropoff,
        echoes::sim::EntityType::Worker,
        echoes::sim::EntityType::Worker,
        echoes::sim::EntityType::Worker,
        echoes::sim::EntityType::Soldier,
        echoes::sim::EntityType::Soldier,
        echoes::sim::EntityType::Soldier,
        echoes::sim::EntityType::HeavyUnit,
        echoes::sim::EntityType::ScoutUnit,
        echoes::sim::EntityType::UtilityStructure};
    static constexpr echoes::sim::EntityType OpponentTypes[] = {
        echoes::sim::EntityType::CommandCore,
        echoes::sim::EntityType::Barracks,
        echoes::sim::EntityType::Dropoff,
        echoes::sim::EntityType::Worker,
        echoes::sim::EntityType::Worker,
        echoes::sim::EntityType::Worker,
        echoes::sim::EntityType::Soldier,
        echoes::sim::EntityType::Soldier,
        echoes::sim::EntityType::HeavyUnit,
        echoes::sim::EntityType::ScoutUnit,
        echoes::sim::EntityType::UtilityStructure};
    for (int32 Index = 0; Index < LocalSpawns.Num(); ++Index)
    {
        if (!IsTypedSpawnFootprintOpen(
                Setup.MapPreset,
                Setup.LocalFaction,
                LocalTypes[Index],
                LocalSpawns[Index]))
        {
            OutError = FString::Printf(
                TEXT("[SKIRMISH_MAP_FOOTPRINT_INVALID] Local deployment footprint %d intersects blocked terrain or the battlefield edge."),
                Index + 1);
            return false;
        }
    }
    for (int32 Index = 0; Index < OpponentSpawns.Num(); ++Index)
    {
        if (!IsTypedSpawnFootprintOpen(
                Setup.MapPreset,
                Setup.OpponentFaction,
                OpponentTypes[Index],
                OpponentSpawns[Index]))
        {
            OutError = FString::Printf(
                TEXT("[SKIRMISH_MAP_FOOTPRINT_INVALID] Opposing deployment footprint %d intersects blocked terrain or the battlefield edge."),
                Index + 1);
            return false;
        }
    }

    TSet<FIntPoint> Occupied;
    const auto ValidateTiles = [&Occupied, &OutError, &Setup](
        const TArray<FIntPoint>& Tiles,
        const TCHAR* Label)
    {
        for (const FIntPoint& Tile : Tiles)
        {
            if (!IsInside(Tile) || IsBlockedTile(Setup.MapPreset, Tile.X, Tile.Y) ||
                Occupied.Contains(Tile))
            {
                OutError = FString::Printf(
                    TEXT("[SKIRMISH_MAP_CONTRACT_INVALID] %s tile %d,%d is blocked, duplicated, or outside the battlefield."),
                    Label,
                    Tile.X,
                    Tile.Y);
                return false;
            }
            Occupied.Add(Tile);
        }
        return true;
    };
    if (!ValidateTiles(LocalSpawns, TEXT("local deployment")) ||
        !ValidateTiles(OpponentSpawns, TEXT("opposing deployment")) ||
        !ValidateTiles(Resources, TEXT("Matter deposit")) ||
        IsBlockedTile(Setup.MapPreset, Well.X, Well.Y) ||
        Occupied.Contains(Well))
    {
        if (OutError.IsEmpty())
        {
            OutError = TEXT("[SKIRMISH_MAP_CONTRACT_INVALID] The Future Well site is not deployable.");
        }
        return false;
    }
    if (!HasOpenPath(Setup.MapPreset, LocalSpawns[0], Well) ||
        !HasOpenPath(Setup.MapPreset, OpponentSpawns[0], Well) ||
        !HasOpenPath(Setup.MapPreset, LocalSpawns[0], OpponentSpawns[0]))
    {
        OutError = TEXT("[SKIRMISH_MAP_ROUTE_INVALID] Both forces and the Future Well must share a playable route.");
        return false;
    }
    return true;
}

const TCHAR* FEchoesSkirmishSetupModel::FactionDisplayName(
    echoes::sim::Faction Faction)
{
    switch (Faction)
    {
        case echoes::sim::Faction::MeridianCompact:
            return TEXT("MERIDIAN COMPACT");
        case echoes::sim::Faction::KharuunAssemblies:
            return TEXT("KHARUUN ASSEMBLIES");
        case echoes::sim::Faction::HollowChoir:
            return TEXT("HOLLOW CHOIR");
    }
    return TEXT("UNKNOWN FORCE");
}

const TCHAR* FEchoesSkirmishSetupModel::MapDisplayName(
    EEchoesSkirmishMapPreset Preset)
{
    switch (Preset)
    {
        case EEchoesSkirmishMapPreset::GlassScar: return TEXT("GLASS SCAR");
        case EEchoesSkirmishMapPreset::CrownfallBasin:
            return TEXT("CROWNFALL BASIN");
        case EEchoesSkirmishMapPreset::SorynConfluence:
            return TEXT("SORYN CONFLUENCE");
    }
    return TEXT("UNKNOWN BATTLEFIELD");
}

const TCHAR* FEchoesSkirmishSetupModel::MapDescription(
    EEchoesSkirmishMapPreset Preset)
{
    switch (Preset)
    {
        case EEchoesSkirmishMapPreset::GlassScar:
            return TEXT("THREE CROSSINGS // CENTRAL WELL // SOUTHWEST–NORTHEAST DEPLOYMENT");
        case EEchoesSkirmishMapPreset::CrownfallBasin:
            return TEXT("TWIN RIDGES // OFFSET WELL // NORTHWEST–SOUTHEAST DEPLOYMENT");
        case EEchoesSkirmishMapPreset::SorynConfluence:
            return TEXT("FRACTURED RING // FOUR GATES // WEST–EAST DEPLOYMENT");
    }
    return TEXT("BATTLEFIELD DATA UNAVAILABLE");
}

const TCHAR* FEchoesSkirmishSetupModel::AiDisplayName(
    echoes::sim::AiPersonality Personality)
{
    switch (Personality)
    {
        case echoes::sim::AiPersonality::Balanced: return TEXT("BALANCED");
        case echoes::sim::AiPersonality::Defensive: return TEXT("WARDEN");
        case echoes::sim::AiPersonality::Raider: return TEXT("RAIDER");
        case echoes::sim::AiPersonality::Economic: return TEXT("STEWARD");
        case echoes::sim::AiPersonality::Expansionist: return TEXT("EXPANSIONIST");
        case echoes::sim::AiPersonality::Adaptive: return TEXT("ADAPTIVE");
    }
    return TEXT("UNKNOWN");
}

const TCHAR* FEchoesSkirmishSetupModel::AiDescription(
    echoes::sim::AiPersonality Personality)
{
    switch (Personality)
    {
        case echoes::sim::AiPersonality::Balanced:
            return TEXT("MIXED FORCE DEVELOPMENT AND PRESSURE");
        case echoes::sim::AiPersonality::Defensive:
            return TEXT("FORTIFIES, HOLDS, AND COUNTERS NEAR ITS CORE");
        case echoes::sim::AiPersonality::Raider:
            return TEXT("EARLY MOBILE PRESSURE AGAINST EXPOSED TARGETS");
        case echoes::sim::AiPersonality::Economic:
            return TEXT("WORKERS, MATTER FLOW, AND DELAYED MASS");
        case echoes::sim::AiPersonality::Expansionist:
            return TEXT("FORWARD INFRASTRUCTURE AND MAP CONTROL");
        case echoes::sim::AiPersonality::Adaptive:
            return TEXT("RESPONDS TO OBSERVED COMPOSITION AND DAMAGE");
    }
    return TEXT("AI PROFILE DATA UNAVAILABLE");
}

const TCHAR* FEchoesSkirmishSetupModel::ResourceDisplayName(
    EEchoesSkirmishResourceLevel Level)
{
    switch (Level)
    {
        case EEchoesSkirmishResourceLevel::Scarce:
            return TEXT("SCARCE // 320 MATTER + 18 DAWN");
        case EEchoesSkirmishResourceLevel::Standard:
            return TEXT("STANDARD // 500 MATTER + 30 DAWN");
        case EEchoesSkirmishResourceLevel::Abundant:
            return TEXT("ABUNDANT // 800 MATTER + 60 DAWN");
    }
    return TEXT("UNKNOWN RESOURCE LEVEL");
}

echoes::sim::ResourcePool FEchoesSkirmishSetupModel::StartingResources(
    EEchoesSkirmishResourceLevel Level)
{
    switch (Level)
    {
        case EEchoesSkirmishResourceLevel::Scarce: return {320, 18};
        case EEchoesSkirmishResourceLevel::Standard: return {500, 30};
        case EEchoesSkirmishResourceLevel::Abundant: return {800, 60};
    }
    return {};
}

FEchoesSkirmishSetup FEchoesSkirmishSetupModel::WithNextFaction(
    const FEchoesSkirmishSetup& Setup,
    bool bLocal,
    int32 Direction)
{
    FEchoesSkirmishSetup Result = Setup;
    echoes::sim::Faction& Target =
        bLocal ? Result.LocalFaction : Result.OpponentFaction;
    for (int32 Attempt = 0; Attempt < 3; ++Attempt)
    {
        Target = CycleEnum(Target, Direction, 3);
        if (Result.LocalFaction != Result.OpponentFaction)
        {
            break;
        }
    }
    return Result;
}

FEchoesSkirmishSetup FEchoesSkirmishSetupModel::WithNextMap(
    const FEchoesSkirmishSetup& Setup,
    int32 Direction)
{
    FEchoesSkirmishSetup Result = Setup;
    Result.MapPreset = CycleEnum(Result.MapPreset, Direction, 3);
    return Result;
}

FEchoesSkirmishSetup FEchoesSkirmishSetupModel::WithNextAi(
    const FEchoesSkirmishSetup& Setup,
    int32 Direction)
{
    FEchoesSkirmishSetup Result = Setup;
    Result.AiPersonality = CycleEnum(Result.AiPersonality, Direction, 6);
    return Result;
}

FEchoesSkirmishSetup FEchoesSkirmishSetupModel::WithNextResources(
    const FEchoesSkirmishSetup& Setup,
    int32 Direction)
{
    FEchoesSkirmishSetup Result = Setup;
    Result.ResourceLevel = CycleEnum(Result.ResourceLevel, Direction, 3);
    return Result;
}

bool FEchoesSkirmishSetupModel::IsBlockedTile(
    EEchoesSkirmishMapPreset Preset,
    int32 TileX,
    int32 TileY)
{
    switch (Preset)
    {
        case EEchoesSkirmishMapPreset::GlassScar:
        {
            const bool bInScar = TileY >= 30 && TileY <= 34 &&
                TileX >= 8 && TileX <= 55;
            const bool bCrossing = (TileX >= 12 && TileX <= 15) ||
                (TileX >= 29 && TileX <= 35) ||
                (TileX >= 48 && TileX <= 51);
            return bInScar && !bCrossing;
        }
        case EEchoesSkirmishMapPreset::CrownfallBasin:
        {
            const bool bRidge =
                ((TileX >= 27 && TileX <= 29) ||
                 (TileX >= 35 && TileX <= 37)) &&
                TileY >= 6 && TileY <= 57;
            const bool bGate = (TileY >= 13 && TileY <= 17) ||
                (TileY >= 30 && TileY <= 34) ||
                (TileY >= 46 && TileY <= 50);
            const bool bNorthShelf = TileY >= 39 && TileY <= 41 &&
                TileX >= 10 && TileX <= 22 &&
                !(TileX >= 15 && TileX <= 17);
            const bool bSouthShelf = TileY >= 22 && TileY <= 24 &&
                TileX >= 42 && TileX <= 54 &&
                !(TileX >= 47 && TileX <= 49);
            return (bRidge && !bGate) || bNorthShelf || bSouthShelf;
        }
        case EEchoesSkirmishMapPreset::SorynConfluence:
        {
            const bool bOuterHorizontal =
                (TileY == 19 || TileY == 20 || TileY == 43 || TileY == 44) &&
                TileX >= 20 && TileX <= 43;
            const bool bOuterVertical =
                (TileX == 20 || TileX == 21 || TileX == 42 || TileX == 43) &&
                TileY >= 19 && TileY <= 44;
            const bool bNorthSouthGate = TileX >= 30 && TileX <= 33;
            const bool bWestEastGate = TileY >= 30 && TileY <= 33;
            const bool bRing =
                (bOuterHorizontal && !bNorthSouthGate) ||
                (bOuterVertical && !bWestEastGate);
            const bool bWestShard = TileX >= 9 && TileX <= 16 &&
                TileY >= 25 && TileY <= 27;
            const bool bEastShard = TileX >= 47 && TileX <= 54 &&
                TileY >= 36 && TileY <= 38;
            return bRing || bWestShard || bEastShard;
        }
    }
    return true;
}

int32 FEchoesSkirmishSetupModel::ExpectedBlockedTileCount(
    EEchoesSkirmishMapPreset Preset)
{
    int32 Count = 0;
    for (int32 Y = 0; Y < MapHeightTiles; ++Y)
    {
        for (int32 X = 0; X < MapWidthTiles; ++X)
        {
            Count += IsBlockedTile(Preset, X, Y) ? 1 : 0;
        }
    }
    return Count;
}

FIntPoint FEchoesSkirmishSetupModel::FutureWellTile(
    EEchoesSkirmishMapPreset Preset)
{
    switch (Preset)
    {
        case EEchoesSkirmishMapPreset::GlassScar: return {32, 32};
        case EEchoesSkirmishMapPreset::CrownfallBasin: return {32, 39};
        case EEchoesSkirmishMapPreset::SorynConfluence: return {32, 32};
    }
    return {-1, -1};
}

TArray<FIntPoint> FEchoesSkirmishSetupModel::LocalSpawnTiles(
    EEchoesSkirmishMapPreset Preset)
{
    switch (Preset)
    {
        case EEchoesSkirmishMapPreset::GlassScar:
            return {{10, 10}, {14, 10}, {6, 17}, {8, 13}, {11, 14},
                    {14, 12}, {8, 8}, {12, 7}, {16, 10}, {7, 6},
                    {15, 6}, {6, 11}};
        case EEchoesSkirmishMapPreset::CrownfallBasin:
            return {{10, 52}, {14, 52}, {6, 45}, {8, 49}, {11, 48},
                    {14, 50}, {8, 54}, {12, 57}, {16, 54}, {7, 58},
                    {15, 58}, {6, 53}};
        case EEchoesSkirmishMapPreset::SorynConfluence:
            return {{8, 32}, {8, 21}, {13, 38}, {11, 30}, {12, 34},
                    {15, 32}, {6, 29}, {6, 35}, {12, 40}, {5, 38},
                    {15, 42}, {13, 24}};
    }
    return {};
}

TArray<FIntPoint> FEchoesSkirmishSetupModel::OpponentSpawnTiles(
    EEchoesSkirmishMapPreset Preset)
{
    switch (Preset)
    {
        case EEchoesSkirmishMapPreset::GlassScar:
            return {{54, 54}, {50, 54}, {58, 48}, {51, 53}, {54, 50},
                    {57, 52}, {50, 57}, {54, 58}, {57, 58},
                    {49, 58}, {58, 53}};
        case EEchoesSkirmishMapPreset::CrownfallBasin:
            return {{54, 12}, {50, 12}, {58, 19}, {51, 13}, {54, 16},
                    {57, 14}, {50, 9}, {54, 6}, {57, 6},
                    {49, 6}, {58, 11}};
        case EEchoesSkirmishMapPreset::SorynConfluence:
            return {{56, 32}, {56, 43}, {51, 26}, {53, 34}, {52, 30},
                    {49, 32}, {58, 35}, {58, 29}, {59, 26},
                    {49, 22}, {51, 40}};
    }
    return {};
}

TArray<FIntPoint> FEchoesSkirmishSetupModel::ResourceNodeTiles(
    EEchoesSkirmishMapPreset Preset)
{
    switch (Preset)
    {
        case EEchoesSkirmishMapPreset::GlassScar:
            return {{16, 16}, {21, 13}, {25, 28}, {33, 22},
                    {31, 43}, {43, 36}, {47, 50}, {52, 45}};
        case EEchoesSkirmishMapPreset::CrownfallBasin:
            return {{15, 46}, {20, 53}, {20, 34}, {32, 27},
                    {34, 48}, {44, 30}, {48, 17}, {56, 22}};
        case EEchoesSkirmishMapPreset::SorynConfluence:
            return {{15, 18}, {17, 47}, {26, 27}, {27, 37},
                    {37, 27}, {38, 37}, {47, 17}, {49, 46}};
    }
    return {};
}
