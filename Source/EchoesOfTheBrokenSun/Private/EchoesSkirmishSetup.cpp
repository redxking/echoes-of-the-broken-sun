#include "EchoesSkirmishSetup.h"

#include "EchoesSimCore/SkirmishMapPresets.h"

#include <array>
#include <cstddef>

#include "Containers/Queue.h"

namespace
{
template <typename EnumType>
EnumType CycleEnum(EnumType Value, int32 Direction, int32 Count)
{
    if (Count <= 0)
    {
        return Value;
    }
    const int32 Current = static_cast<int32>(Value);
    int32 Next = (Current + Direction) % Count;
    if (Next < 0)
    {
        Next += Count;
    }
    return static_cast<EnumType>(Next);
}

bool IsPlayableFaction(echoes::sim::Faction Faction)
{
    return Faction == echoes::sim::Faction::MeridianCompact ||
        Faction == echoes::sim::Faction::KharuunAssemblies ||
        Faction == echoes::sim::Faction::HollowChoir;
}

// The release boundary authorises exactly five AI doctrines - Warden, Raider,
// Steward, Expansionist, Adaptive - and section 16.1 documents a purpose, an
// economy/scouting posture, a combat posture and a Well preference for each of
// those five and no others. AiPersonality::Balanced is enum value 0 and is the
// simulation's internal default parameter; it is not an authored doctrine. It
// used to be offered here and in the cycler, which made a sixth, undocumented
// doctrine selectable - accessible content with no documented player purpose,
// which AUTH-005 and VAL-003 make a release blocker. The selector now offers
// only the five authored doctrines.
constexpr echoes::sim::AiPersonality kFirstAuthoredDoctrine =
    echoes::sim::AiPersonality::Defensive;
constexpr int32 kAuthoredDoctrineCount = 5;

bool IsKnownAi(echoes::sim::AiPersonality Personality)
{
    return Personality == echoes::sim::AiPersonality::Defensive ||
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
    if (Setup.Seed == 0)
    {
        OutError = TEXT("[SKIRMISH_SEED_INVALID] Select a nonzero match seed.");
        return false;
    }
    if (!IsPlayableFaction(Setup.LocalFaction) ||
        !IsPlayableFaction(Setup.OpponentFaction))
    {
        OutError = TEXT("[SKIRMISH_FACTION_INVALID] Select two playable forces.");
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
    if (static_cast<uint8>(Setup.Difficulty) >
        static_cast<uint8>(EEchoesSkirmishDifficulty::Sovereign))
    {
        OutError = TEXT("[SKIRMISH_DIFFICULTY_INVALID] Select a supported difficulty tier.");
        return false;
    }
    if (Setup.VictoryCondition != EEchoesSkirmishVictoryCondition::Corefall)
    {
        OutError = TEXT("[SKIRMISH_VICTORY_INVALID] This offline match supports Corefall only. Select Corefall before deployment.");
        return false;
    }
    if (static_cast<uint8>(Setup.GameSpeed) >
        static_cast<uint8>(EEchoesSkirmishGameSpeed::Fast))
    {
        OutError = TEXT("[SKIRMISH_SPEED_INVALID] Select a supported game speed.");
        return false;
    }
    if (static_cast<uint8>(Setup.TeamSetup) >
        static_cast<uint8>(EEchoesSkirmishTeamSetup::FreeForAll))
    {
        OutError = TEXT("[SKIRMISH_TEAM_INVALID] Select a supported team setup.");
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
        // Balanced is not an authored doctrine and is unreachable from the
        // selector; it deliberately has no player-facing name.
        case echoes::sim::AiPersonality::Balanced: break;
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
        // No description, because there is no authored doctrine to describe.
        case echoes::sim::AiPersonality::Balanced: break;
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
            return TEXT("SCARCE // 250 MATTER + 18 DAWN");
        case EEchoesSkirmishResourceLevel::Standard:
            return TEXT("STANDARD // 400 MATTER + 30 DAWN");
        case EEchoesSkirmishResourceLevel::Abundant:
            return TEXT("ABUNDANT // 700 MATTER + 60 DAWN");
    }
    return TEXT("UNKNOWN RESOURCE LEVEL");
}

const TCHAR* FEchoesSkirmishSetupModel::ResourceDescription(
    EEchoesSkirmishResourceLevel Level)
{
    switch (Level)
    {
        case EEchoesSkirmishResourceLevel::Scarce:
            return TEXT("CONSTRAINED STOCKPILE: REQUIRES EARLY EXPANSION AND TIGHT MACRO DISCIPLINE");
        case EEchoesSkirmishResourceLevel::Standard:
            return TEXT("BALANCED BASELINE: STANDARD OPENING BUILD ORDERS AND ENGAGEMENT PACING");
        case EEchoesSkirmishResourceLevel::Abundant:
            return TEXT("GENEROUS RESERVE: FAST TECH SCALING AND IMMEDIATE MULTI-PRODUCTION");
    }
    return TEXT("UNKNOWN RESOURCE PROFILE");
}

echoes::sim::ResourcePool FEchoesSkirmishSetupModel::StartingResources(
    EEchoesSkirmishResourceLevel Level)
{
    switch (Level)
    {
        case EEchoesSkirmishResourceLevel::Scarce: return {250, 18};
        case EEchoesSkirmishResourceLevel::Standard: return {400, 30};
        case EEchoesSkirmishResourceLevel::Abundant: return {700, 60};
    }
    return {};
}

FEchoesAiDifficultyPolicy FEchoesAiDifficultyPolicy::For(EEchoesSkirmishDifficulty Difficulty)
{
    switch (Difficulty)
    {
    case EEchoesSkirmishDifficulty::Story: return {60, 200, 4};
    case EEchoesSkirmishDifficulty::Standard: return {30, 100, 7};
    case EEchoesSkirmishDifficulty::Veteran: return {18, 60, 10};
    case EEchoesSkirmishDifficulty::Sovereign: return {10, 40, 12};
    }
    return {};
}

const TCHAR* FEchoesSkirmishSetupModel::DifficultyDisplayName(EEchoesSkirmishDifficulty Difficulty)
{
    switch (Difficulty)
    {
    case EEchoesSkirmishDifficulty::Story: return TEXT("Story");
    case EEchoesSkirmishDifficulty::Standard: return TEXT("Standard");
    case EEchoesSkirmishDifficulty::Veteran: return TEXT("Veteran");
    case EEchoesSkirmishDifficulty::Sovereign: return TEXT("Sovereign");
    }
    return TEXT("Unknown difficulty");
}

const TCHAR* FEchoesSkirmishSetupModel::DifficultyDescription(EEchoesSkirmishDifficulty Difficulty)
{
    switch (Difficulty)
    {
    case EEchoesSkirmishDifficulty::Story: return TEXT("3.0s reaction; strategic review every 10s; at most 4 group commands/s. Equal combat and economy rules.");
    case EEchoesSkirmishDifficulty::Standard: return TEXT("1.5s reaction; strategic review every 5s; at most 7 group commands/s. Equal combat and economy rules.");
    case EEchoesSkirmishDifficulty::Veteran: return TEXT("0.9s reaction; strategic review every 3s; at most 10 group commands/s. Equal combat and economy rules.");
    case EEchoesSkirmishDifficulty::Sovereign: return TEXT("0.5s reaction; strategic review every 2s; at most 12 group commands/s. Equal combat and economy rules.");
    }
    return TEXT("Difficulty unavailable");
}

const TCHAR* FEchoesSkirmishSetupModel::AssistedDifficultyModifiers()
{
    // Legacy call sites retain the same ordinal; the selected Story contract applies.
    return DifficultyDescription(EEchoesSkirmishDifficulty::Story);
}

const TCHAR* FEchoesSkirmishSetupModel::VictoryConditionDisplayName(
    EEchoesSkirmishVictoryCondition Condition)
{
    switch (Condition)
    {
        case EEchoesSkirmishVictoryCondition::Corefall:
            return TEXT("COREFALL // DESTROY ENEMY COMMAND CORE");
        case EEchoesSkirmishVictoryCondition::WellControl:
            return TEXT("Unsupported legacy victory rule");
        case EEchoesSkirmishVictoryCondition::Conquest:
            return TEXT("Unsupported legacy victory rule");
    }
    return TEXT("UNKNOWN VICTORY CONDITION");
}

const TCHAR* FEchoesSkirmishSetupModel::VictoryConditionDescription(
    EEchoesSkirmishVictoryCondition Condition)
{
    switch (Condition)
    {
        case EEchoesSkirmishVictoryCondition::Corefall:
            return TEXT("MATCH ENDS WHEN EITHER SIDE'S COMMAND CORE IS DESTROYED (DEFAULT RTS CONTRACT)");
        case EEchoesSkirmishVictoryCondition::WellControl:
            return TEXT("Select Corefall to start an offline match.");
        case EEchoesSkirmishVictoryCondition::Conquest:
            return TEXT("Select Corefall to start an offline match.");
    }
    return TEXT("VICTORY CONDITION DATA UNAVAILABLE");
}

const TCHAR* FEchoesSkirmishSetupModel::GameSpeedDisplayName(
    EEchoesSkirmishGameSpeed Speed)
{
    switch (Speed)
    {
        case EEchoesSkirmishGameSpeed::Tactical:
            return TEXT("TACTICAL // 0.75X SPEED");
        case EEchoesSkirmishGameSpeed::Normal:
            return TEXT("NORMAL // 1.0X SPEED (20 HZ)");
        case EEchoesSkirmishGameSpeed::Fast:
            return TEXT("FAST // 1.5X SPEED");
    }
    return TEXT("UNKNOWN SPEED");
}

const TCHAR* FEchoesSkirmishSetupModel::GameSpeedDescription(
    EEchoesSkirmishGameSpeed Speed)
{
    switch (Speed)
    {
        case EEchoesSkirmishGameSpeed::Tactical:
            return TEXT("REDUCED PACE FOR DELIBERATE TACTICAL POSITIONING (26.7 MS TICK INTERVAL)");
        case EEchoesSkirmishGameSpeed::Normal:
            return TEXT("STANDARD REAL-TIME RTS TICK CADENCE (50.0 MS FIXED TICK INTERVAL)");
        case EEchoesSkirmishGameSpeed::Fast:
            return TEXT("ACCELERATED CADENCE FOR HIGH-SPEED STRATEGIC MACRO (33.3 MS TICK INTERVAL)");
    }
    return TEXT("SPEED DATA UNAVAILABLE");
}

float FEchoesSkirmishSetupModel::GameSpeedMultiplier(
    EEchoesSkirmishGameSpeed Speed)
{
    switch (Speed)
    {
        case EEchoesSkirmishGameSpeed::Tactical: return 0.75f;
        case EEchoesSkirmishGameSpeed::Normal: return 1.00f;
        case EEchoesSkirmishGameSpeed::Fast: return 1.50f;
    }
    return 1.0f;
}

const TCHAR* FEchoesSkirmishSetupModel::TeamSetupDisplayName(
    EEchoesSkirmishTeamSetup TeamSetup)
{
    switch (TeamSetup)
    {
        case EEchoesSkirmishTeamSetup::OneVsOne:
            return TEXT("1V1 // LOCAL VS OPPONENT");
        case EEchoesSkirmishTeamSetup::FreeForAll:
            return TEXT("FREE-FOR-ALL // INDEPENDENT FORCES");
    }
    return TEXT("UNKNOWN TEAM SETUP");
}

const TCHAR* FEchoesSkirmishSetupModel::TeamSetupDescription(
    EEchoesSkirmishTeamSetup TeamSetup)
{
    switch (TeamSetup)
    {
        case EEchoesSkirmishTeamSetup::OneVsOne:
            return TEXT("STANDARD TWO-PLAYER CONFLICT: TEAM 1 DEPLOYED AGAINST TEAM 2");
        case EEchoesSkirmishTeamSetup::FreeForAll:
            return TEXT("INDEPENDENT SURVIVAL: EVERY FORCE HOLDS HOSTILE STANDING WITH ALL OTHERS");
    }
    return TEXT("TEAM SETUP DATA UNAVAILABLE");
}

FEchoesSkirmishSetup FEchoesSkirmishSetupModel::WithNextFaction(
    const FEchoesSkirmishSetup& Setup,
    bool bLocal,
    int32 Direction)
{
    FEchoesSkirmishSetup Result = Setup;
    echoes::sim::Faction& Target =
        bLocal ? Result.LocalFaction : Result.OpponentFaction;
    // Mirror matchups are a required configuration, so the cycler steps once
    // and stops. It used to loop past any faction that matched the other side,
    // which made three of the nine required matchups unreachable in the UI.
    Target = CycleEnum(Target, Direction, 3);
    return Result;
}

FEchoesSkirmishSetup FEchoesSkirmishSetupModel::WithNextTeam(
    const FEchoesSkirmishSetup& Setup,
    int32 Direction)
{
    FEchoesSkirmishSetup Result = Setup;
    Result.TeamSetup = CycleEnum(Result.TeamSetup, Direction, 2);
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
    // Cycles the five authored doctrines only. AiPersonality::Balanced sits at
    // enum index 0, immediately below the authored window, so the cycler walks
    // [Defensive .. Adaptive] and can never land on it. A setup carrying an
    // unauthored value (an older save, a fixture) is folded onto the first
    // authored doctrine rather than being stepped further along.
    FEchoesSkirmishSetup Result = Setup;
    const int32 First = static_cast<int32>(kFirstAuthoredDoctrine);
    const int32 Current = static_cast<int32>(Result.AiPersonality);
    const int32 Index =
        (Current >= First && Current < First + kAuthoredDoctrineCount)
        ? Current - First
        : 0;
    int32 Next = (Index + Direction) % kAuthoredDoctrineCount;
    if (Next < 0)
    {
        Next += kAuthoredDoctrineCount;
    }
    Result.AiPersonality = static_cast<echoes::sim::AiPersonality>(First + Next);
    return Result;
}

FEchoesSkirmishSetup FEchoesSkirmishSetupModel::WithNextDifficulty(
    const FEchoesSkirmishSetup& Setup,
    int32 Direction)
{
    FEchoesSkirmishSetup Result = Setup;
    Result.Difficulty = CycleEnum(Result.Difficulty, Direction, 4);
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

FEchoesSkirmishSetup FEchoesSkirmishSetupModel::WithNextVictoryCondition(
    const FEchoesSkirmishSetup& Setup,
    int32 Direction)
{
    FEchoesSkirmishSetup Result = Setup;
    Result.VictoryCondition = EEchoesSkirmishVictoryCondition::Corefall;
    return Result;
}

FEchoesSkirmishSetup FEchoesSkirmishSetupModel::WithNextGameSpeed(
    const FEchoesSkirmishSetup& Setup,
    int32 Direction)
{
    FEchoesSkirmishSetup Result = Setup;
    Result.GameSpeed = CycleEnum(Result.GameSpeed, Direction, 3);
    return Result;
}

namespace
{
// One definition for all three battlefields now lives in
// EchoesSimCore/SkirmishMapPresets.h so the headless balance harness reads the
// same geometry this module does. The harness previously could not see it and
// measured a synthetic fixture instead.
[[nodiscard]] echoes::sim::SkirmishMapPreset ToCorePreset(
    EEchoesSkirmishMapPreset Preset)
{
    switch (Preset)
    {
        case EEchoesSkirmishMapPreset::GlassScar:
            return echoes::sim::SkirmishMapPreset::GlassScar;
        case EEchoesSkirmishMapPreset::CrownfallBasin:
            return echoes::sim::SkirmishMapPreset::CrownfallBasin;
        case EEchoesSkirmishMapPreset::SorynConfluence:
            return echoes::sim::SkirmishMapPreset::SorynConfluence;
    }
    return echoes::sim::SkirmishMapPreset::GlassScar;
}
}

bool FEchoesSkirmishSetupModel::IsBlockedTile(
    EEchoesSkirmishMapPreset Preset,
    int32 TileX,
    int32 TileY)
{
    return echoes::sim::IsSkirmishBlockedTile(
        ToCorePreset(Preset), TileX, TileY);
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
    const echoes::sim::SkirmishTile Tile =
        echoes::sim::SkirmishFutureWellTile(ToCorePreset(Preset));
    return {Tile.x, Tile.y};
}

namespace
{
template <std::size_t N>
[[nodiscard]] TArray<FIntPoint> ToTileArray(
    const std::array<echoes::sim::SkirmishTile, N>& Tiles)
{
    TArray<FIntPoint> Result;
    Result.Reserve(static_cast<int32>(N));
    for (const echoes::sim::SkirmishTile& Tile : Tiles)
    {
        Result.Add(FIntPoint(Tile.x, Tile.y));
    }
    return Result;
}
}

TArray<FIntPoint> FEchoesSkirmishSetupModel::LocalSpawnTiles(
    EEchoesSkirmishMapPreset Preset)
{
    return ToTileArray(
        echoes::sim::SkirmishLocalSpawnTiles(ToCorePreset(Preset)));
}

TArray<FIntPoint> FEchoesSkirmishSetupModel::OpponentSpawnTiles(
    EEchoesSkirmishMapPreset Preset)
{
    return ToTileArray(
        echoes::sim::SkirmishOpponentSpawnTiles(ToCorePreset(Preset)));
}

TArray<FIntPoint> FEchoesSkirmishSetupModel::ResourceNodeTiles(
    EEchoesSkirmishMapPreset Preset)
{
    return ToTileArray(
        echoes::sim::SkirmishResourceNodeTiles(ToCorePreset(Preset)));
}
