#pragma once

#include "CoreMinimal.h"
#include "EchoesNoNeutralLedgerMissionModel.h"
#include "EchoesSimCore/Simulation.h"

enum class EEchoesFutureThatWonPhase : uint8
{
    Inactive,
    EstablishIndependentReadback,
    VerifyRecordedInputs,
    BindRecordedProtocol,
    HoldStabilityWindow,
    ObserveDistrictReadbacks,
    Complete,
    Failed
};

struct FEchoesFutureThatWonMissionFacts final
{
    bool bOperationActive = false;
    bool bLocalCoreIntact = false;
    bool bOruunIntact = false;
    bool bVerifierIntact = false;
    bool bFutureWellIntact = false;
    bool bPublicInterfacesIntact = false;
    bool bIndependentPublicReadbackEstablished = false;
    bool bFirstRecordedInputVerified = false;
    bool bSecondRecordedInputVerified = false;
    bool bRecordedProtocolBound = false;
    bool bConflictingProtocolBound = false;
    bool bStabilityWindowHeld = false;
    bool bFirstDistrictReadbackObserved = false;
    bool bSecondDistrictReadbackObserved = false;
    bool bReshapeWindowExpired = false;
    bool bSkirmishStillOngoing = true;
};

struct FEchoesFutureThatWonPlan final
{
    echoes::sim::FutureWellChoice FoundingDoctrine =
        echoes::sim::FutureWellChoice::Dormant;
    echoes::sim::FutureWellChoice RecordedProtocol =
        echoes::sim::FutureWellChoice::Dormant;
    EEchoesCityDistrict FirstContributingDistrict =
        EEchoesCityDistrict::LifeSupport;
    EEchoesCityDistrict SecondContributingDistrict =
        EEchoesCityDistrict::Transit;
    EEchoesCityDistrict DeferredDistrict =
        EEchoesCityDistrict::Archive;
    echoes::sim::Vec2 FirstDistrictInputSite;
    echoes::sim::Vec2 SecondDistrictInputSite;
    echoes::sim::Vec2 MeridianReadbackSite;
    echoes::sim::Vec2 KharuunReadbackSite;
    echoes::sim::Vec2 RestorationDemonstratorSite;
    echoes::sim::Vec2 FutureWellSite;
    uint64 StabilityWindowTicks = 300;
    uint8 StablePlanKey = 0;
    const TCHAR* RouteStableName = TEXT("unavailable");
    const TCHAR* RouteDisplayName = TEXT("UNAVAILABLE ROUTE");
    const TCHAR* ProtocolStableName = TEXT("unavailable");
    const TCHAR* ProtocolDisplayName = TEXT("UNAVAILABLE PROTOCOL");
};

/**
 * Pure Mission 12 reducer and exact eleven-record restoration projection.
 * The model proves a bounded local activation/readback contract only.
 */
struct ECHOESOFTHEBROKENSUN_API FEchoesFutureThatWonMissionModel final
{
    [[nodiscard]] static EEchoesFutureThatWonPhase DeterminePhase(
        const FEchoesFutureThatWonMissionFacts& Facts);
    [[nodiscard]] static const TCHAR* StableName(
        EEchoesFutureThatWonPhase Phase);
    [[nodiscard]] static bool TryPlanForLedger(
        echoes::sim::FutureWellChoice FoundingDoctrine,
        uint8 ReserveAuthorityFacts,
        echoes::sim::FutureWellChoice RecordedProtocol,
        FEchoesFutureThatWonPlan& OutPlan);
};
