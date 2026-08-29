#pragma once

#include "CoreMinimal.h"
#include "EchoesSimCore/Simulation.h"

enum class EEchoesShapeOfSilencePhase : uint8
{
    Inactive,
    RootWaystone,
    RaiseListeningSpine,
    PositionMemoryWitnesses,
    ReachConfluence,
    Complete,
    Failed
};

struct FEchoesShapeOfSilenceMissionFacts final
{
    bool bOperationActive = false;
    bool bLocalCoreIntact = false;
    bool bWaystoneIntact = false;
    bool bOruunIntact = false;
    bool bFirstWitnessIntact = false;
    bool bSecondWitnessIntact = false;
    bool bWaystoneRootedAtAnchor = false;
    bool bListeningSpineRaised = false;
    bool bFirstWitnessPositioned = false;
    bool bSecondWitnessPositioned = false;
    bool bOruunAtConfluence = false;
    bool bSkirmishStillOngoing = true;
};

struct FEchoesShapeOfSilencePlan final
{
    echoes::sim::FutureWellChoice PriorChoice =
        echoes::sim::FutureWellChoice::Dormant;
    echoes::sim::Vec2 WaystoneAnchor;
    echoes::sim::Vec2 ListeningSpineSite;
    echoes::sim::Vec2 FirstWitnessSite;
    echoes::sim::Vec2 SecondWitnessSite;
    echoes::sim::Vec2 ConfluenceSite;
    const TCHAR* StableName = TEXT("unavailable");
    const TCHAR* DisplayName = TEXT("UNAVAILABLE MEMORY HOLLOW");
};

/** Pure mission-07 reducer and inherited six-record listening geometry. */
struct ECHOESOFTHEBROKENSUN_API FEchoesShapeOfSilenceMissionModel final
{
    [[nodiscard]] static EEchoesShapeOfSilencePhase DeterminePhase(
        const FEchoesShapeOfSilenceMissionFacts& Facts);
    [[nodiscard]] static const TCHAR* StableName(
        EEchoesShapeOfSilencePhase Phase);
    [[nodiscard]] static FEchoesShapeOfSilencePlan PlanForChoice(
        echoes::sim::FutureWellChoice Choice);
};
