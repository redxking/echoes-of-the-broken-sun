#pragma once

#include "CoreMinimal.h"
#include "EchoesCityReserveMissionModel.h"
#include "EchoesSimCore/Simulation.h"

enum class EEchoesChoirAtLumeReachPhase : uint8
{
    Inactive,
    EstablishContact,
    ResolveDeferredLiability,
    RaiseFirstAnchor,
    RaiseSecondAnchor,
    CommitFutureWell,
    ResolveFutureWell,
    Complete,
    Failed
};

struct FEchoesChoirAtLumeReachMissionFacts final
{
    bool bOperationActive = false;
    bool bLocalCoreIntact = false;
    bool bOruunIntact = false;
    bool bWaystoneIntact = false;
    bool bFutureWellIntact = false;
    bool bContactEstablished = false;
    bool bDeferredLiabilityResolved = false;
    bool bFirstAnchorRaised = false;
    bool bSecondAnchorRaised = false;
    bool bFutureWellProtocolChosen = false;
    bool bBranchResolutionCompleted = false;
    bool bReshapeWindowExpired = false;
    bool bSkirmishStillOngoing = true;
};

struct FEchoesChoirAtLumeReachPlan final
{
    echoes::sim::FutureWellChoice PriorChoice =
        echoes::sim::FutureWellChoice::Dormant;
    EEchoesCityDistrict DeferredDistrict =
        EEchoesCityDistrict::LifeSupport;
    echoes::sim::Vec2 ContactSite;
    echoes::sim::Vec2 LiabilitySite;
    echoes::sim::Vec2 FirstAnchorSite;
    echoes::sim::Vec2 SecondAnchorSite;
    echoes::sim::Vec2 FutureWellSite;
    const TCHAR* StableName = TEXT("unavailable");
    const TCHAR* DisplayName = TEXT("UNAVAILABLE LUME APPROACH");
};

/** Pure mission-10 reducer and inherited nine-record Lume Reach geometry. */
struct ECHOESOFTHEBROKENSUN_API FEchoesChoirAtLumeReachMissionModel final
{
    [[nodiscard]] static EEchoesChoirAtLumeReachPhase DeterminePhase(
        const FEchoesChoirAtLumeReachMissionFacts& Facts);
    [[nodiscard]] static const TCHAR* StableName(
        EEchoesChoirAtLumeReachPhase Phase);
    [[nodiscard]] static FEchoesChoirAtLumeReachPlan PlanForChoice(
        echoes::sim::FutureWellChoice PriorChoice,
        EEchoesCityDistrict DeferredDistrict);
    [[nodiscard]] static echoes::sim::Vec2 LiabilitySiteForDistrict(
        EEchoesCityDistrict District);
    [[nodiscard]] static echoes::sim::Vec2 ResolutionSiteForChoice(
        echoes::sim::FutureWellChoice Choice);
};
