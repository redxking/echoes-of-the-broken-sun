#pragma once

#include "CoreMinimal.h"
#include "EchoesSimCore/Simulation.h"

enum class EEchoesShapeBesideUsPhase : uint8
{
    Inactive,
    ReachFirstEcho,
    RaiseEchoRelay,
    TraversePairedStates,
    ReachConvergence,
    Complete,
    Failed
};

struct FEchoesShapeBesideUsMissionFacts final
{
    bool bOperationActive = false;
    bool bLocalCoreIntact = false;
    bool bTalarIntact = false;
    bool bFirstStateWitnessIntact = false;
    bool bSecondStateWitnessIntact = false;
    bool bFirstEchoObserved = false;
    bool bEchoRelayRaised = false;
    bool bFirstStateTraversed = false;
    bool bSecondStateTraversed = false;
    bool bTalarAtConvergence = false;
    bool bSkirmishStillOngoing = true;
};

struct FEchoesShapeBesideUsPlan final
{
    echoes::sim::FutureWellChoice PriorChoice =
        echoes::sim::FutureWellChoice::Dormant;
    echoes::sim::Vec2 FirstEchoSite;
    echoes::sim::Vec2 EchoRelaySite;
    echoes::sim::Vec2 FirstStateSite;
    echoes::sim::Vec2 SecondStateSite;
    echoes::sim::Vec2 ConvergenceSite;
    const TCHAR* StableName = TEXT("unavailable");
    const TCHAR* DisplayName = TEXT("UNAVAILABLE OVERLAP");
};

/** Pure mission-08 reducer and inherited seven-record overlap geometry. */
struct ECHOESOFTHEBROKENSUN_API FEchoesShapeBesideUsMissionModel final
{
    [[nodiscard]] static EEchoesShapeBesideUsPhase DeterminePhase(
        const FEchoesShapeBesideUsMissionFacts& Facts);
    [[nodiscard]] static const TCHAR* StableName(
        EEchoesShapeBesideUsPhase Phase);
    [[nodiscard]] static FEchoesShapeBesideUsPlan PlanForChoice(
        echoes::sim::FutureWellChoice Choice);
};
