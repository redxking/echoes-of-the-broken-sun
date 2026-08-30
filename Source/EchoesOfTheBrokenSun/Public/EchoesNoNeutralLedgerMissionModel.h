#pragma once

#include "CoreMinimal.h"
#include "EchoesCityReserveMissionModel.h"
#include "EchoesSimCore/Simulation.h"

enum class EEchoesNoNeutralLedgerPhase : uint8
{
    Inactive,
    SecureInheritedRoute,
    IntegrateDistrictContributions,
    AttestEvidenceChannels,
    ApplyRecordedProtocol,
    RallyCoalition,
    Complete,
    Failed
};

struct FEchoesNoNeutralLedgerMissionFacts final
{
    bool bOperationActive = false;
    bool bLocalCoreIntact = false;
    bool bOruunIntact = false;
    bool bWaystoneIntact = false;
    bool bLedgerWitnessIntact = false;
    bool bFutureWellIntact = false;
    bool bPublicInterfacesIntact = false;
    bool bInheritedRouteSecured = false;
    bool bFirstDistrictIntegrated = false;
    bool bSecondDistrictIntegrated = false;
    bool bBothEvidenceChannelsAttested = false;
    bool bRecordedProtocolApplied = false;
    bool bConflictingProtocolApplied = false;
    bool bCoalitionRallied = false;
    bool bReshapeWindowExpired = false;
    bool bSkirmishStillOngoing = true;
};

struct FEchoesNoNeutralLedgerPlan final
{
    echoes::sim::FutureWellChoice FoundingDoctrine =
        echoes::sim::FutureWellChoice::Dormant;
    echoes::sim::FutureWellChoice LumeProtocol =
        echoes::sim::FutureWellChoice::Dormant;
    EEchoesCityDistrict FirstContributingDistrict =
        EEchoesCityDistrict::LifeSupport;
    EEchoesCityDistrict SecondContributingDistrict =
        EEchoesCityDistrict::Transit;
    EEchoesCityDistrict DeferredDistrict =
        EEchoesCityDistrict::Archive;
    echoes::sim::Vec2 RouteSite;
    echoes::sim::Vec2 FirstDistrictSite;
    echoes::sim::Vec2 SecondDistrictSite;
    echoes::sim::Vec2 MeridianEvidenceSite;
    echoes::sim::Vec2 KharuunEvidenceSite;
    echoes::sim::Vec2 FutureWellSite;
    echoes::sim::Vec2 RallySite;
    uint8 StablePlanKey = 0;
    const TCHAR* RouteStableName = TEXT("unavailable");
    const TCHAR* RouteDisplayName = TEXT("UNAVAILABLE ROUTE");
    const TCHAR* ProtocolStableName = TEXT("unavailable");
    const TCHAR* ProtocolDisplayName = TEXT("UNAVAILABLE PROTOCOL");
};

/** Pure mission-11 reducer and ten-record coalition projection. */
struct ECHOESOFTHEBROKENSUN_API FEchoesNoNeutralLedgerMissionModel final
{
    [[nodiscard]] static EEchoesNoNeutralLedgerPhase DeterminePhase(
        const FEchoesNoNeutralLedgerMissionFacts& Facts);
    [[nodiscard]] static const TCHAR* StableName(
        EEchoesNoNeutralLedgerPhase Phase);
    [[nodiscard]] static bool TryPlanForLedger(
        echoes::sim::FutureWellChoice FoundingDoctrine,
        uint8 ReserveAuthorityFacts,
        echoes::sim::FutureWellChoice LumeProtocol,
        FEchoesNoNeutralLedgerPlan& OutPlan);
    [[nodiscard]] static echoes::sim::Vec2 DistrictContributionSite(
        EEchoesCityDistrict District);
    [[nodiscard]] static echoes::sim::Vec2 RallySiteForProtocol(
        echoes::sim::FutureWellChoice Protocol);
};
