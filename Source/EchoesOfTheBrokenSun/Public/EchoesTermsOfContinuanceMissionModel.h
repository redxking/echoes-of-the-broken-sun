#pragma once

#include "CoreMinimal.h"
#include "EchoesSimCore/Simulation.h"

enum class EEchoesTermsOfContinuancePhase : uint8
{
    Inactive,
    SynchronizeNetworks,
    HoldContinuanceWindow,
    ExtractWitnesses,
    Complete,
    Failed
};

struct FEchoesTermsOfContinuanceMissionFacts final
{
    bool bOperationActive = false;
    bool bLocalCoreIntact = false;
    bool bMeridianRelayIntact = false;
    bool bKharuunSpineIntact = false;
    bool bMeridianWitnessIntact = false;
    bool bKharuunWitnessIntact = false;
    bool bMeridianRelaySynchronized = false;
    bool bKharuunSpineSynchronized = false;
    bool bContinuanceWindowHeld = false;
    bool bMeridianWitnessExtracted = false;
    bool bKharuunWitnessExtracted = false;
    bool bContinuanceWindowCompromised = false;
    bool bWitnessExtractionStartedEarly = false;
    bool bSkirmishStillOngoing = true;
};

struct FEchoesTermsOfContinuancePlan final
{
    echoes::sim::FutureWellChoice PriorChoice =
        echoes::sim::FutureWellChoice::Dormant;
    echoes::sim::Vec2 MeridianRelaySite;
    echoes::sim::Vec2 KharuunSpineSite;
    echoes::sim::Vec2 WitnessExtractionSite;
    uint64 ContinuanceWindowStartTick = 300;
    uint64 ContinuanceWindowEndTick = 900;
    const TCHAR* StableName = TEXT("unavailable");
    const TCHAR* DisplayName = TEXT("UNAVAILABLE ACCORD");
};

/** Pure mission-05 reducer and inherited four-record accord geometry. */
struct ECHOESOFTHEBROKENSUN_API FEchoesTermsOfContinuanceMissionModel final
{
    [[nodiscard]] static EEchoesTermsOfContinuancePhase DeterminePhase(
        const FEchoesTermsOfContinuanceMissionFacts& Facts);
    [[nodiscard]] static const TCHAR* StableName(
        EEchoesTermsOfContinuancePhase Phase);
    [[nodiscard]] static FEchoesTermsOfContinuancePlan PlanForChoice(
        echoes::sim::FutureWellChoice Choice);
};
