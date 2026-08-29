#pragma once

#include "CoreMinimal.h"
#include "EchoesSimCore/Simulation.h"

enum class EEchoesUnburiedRoadPhase : uint8
{
    Inactive,
    EstablishRoadhead,
    RaiseListeningSpine,
    RecoverMemoryShard,
    Complete,
    Failed
};

struct FEchoesUnburiedRoadMissionFacts final
{
    bool bOperationActive = false;
    bool bLocalCoreIntact = false;
    bool bMemoryBearerIntact = false;
    bool bWaystoneIntact = false;
    bool bWaystoneRootedAtRoadhead = false;
    bool bListeningSpineComplete = false;
    bool bMemoryBearerAtShard = false;
    bool bSkirmishStillOngoing = true;
};

struct FEchoesUnburiedRoadRoute final
{
    echoes::sim::FutureWellChoice PriorChoice =
        echoes::sim::FutureWellChoice::Dormant;
    echoes::sim::Vec2 Roadhead;
    echoes::sim::Vec2 ListeningSpineSite;
    echoes::sim::Vec2 MemoryShardSite;
    const TCHAR* StableName = TEXT("unavailable");
    const TCHAR* DisplayName = TEXT("UNAVAILABLE ROAD");
};

/** Pure mission-04 reducer and inherited three-record route mapping. */
struct ECHOESOFTHEBROKENSUN_API FEchoesUnburiedRoadMissionModel final
{
    [[nodiscard]] static EEchoesUnburiedRoadPhase DeterminePhase(
        const FEchoesUnburiedRoadMissionFacts& Facts);
    [[nodiscard]] static const TCHAR* StableName(
        EEchoesUnburiedRoadPhase Phase);
    [[nodiscard]] static FEchoesUnburiedRoadRoute RouteForChoice(
        echoes::sim::FutureWellChoice Choice);
};
