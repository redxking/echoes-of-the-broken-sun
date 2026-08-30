#pragma once

#include "CoreMinimal.h"
#include "EchoesCampaignProgress.h"
#include "EchoesSeveralVoicesOneCommandMissionModel.h"
#include "EchoesSimCore/Simulation.h"

enum class EEchoesBrokenSunPhase : uint8
{
    Inactive,
    SecureCrownfallApproach,
    AssembleAccord,
    ChooseFinalResolution,
    RaiseResolutionConduit,
    HoldFinalResolution,
    Complete,
    Failed
};

struct FEchoesBrokenSunMissionFacts final
{
    bool bOperationActive = false;
    bool bLocalCoreIntact = false;
    bool bMaraIntact = false;
    bool bOruunIntact = false;
    bool bTalarIntact = false;
    bool bNemeIntact = false;
    bool bCommandForceIntact = false;
    bool bApproachAnchorComplete = false;
    bool bMeridianAccordEstablished = false;
    bool bKharuunAccordEstablished = false;
    bool bChoirAccordEstablished = false;
    EEchoesFinalResolution SelectedResolution =
        EEchoesFinalResolution::None;
    bool bSelectedResolutionEligible = false;
    bool bResolutionConduitComplete = false;
    bool bResolutionWindowHeld = false;
    bool bResolutionContractFailed = false;
    bool bSkirmishStillOngoing = true;
};

struct FEchoesBrokenSunPlan final
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
    echoes::sim::Vec2 MaraAccordSite;
    echoes::sim::Vec2 OruunAccordSite;
    echoes::sim::Vec2 NemeAccordSite;
    echoes::sim::Vec2 TalarPublicRecordSite;
    echoes::sim::Vec2 CrownfallApproachSite;
    echoes::sim::Vec2 FinalConvergenceSite;
    uint64 ResolutionHoldTicks = 0;
    uint8 AvailableFinalResolutions = 0;
    uint8 StablePlanKey = 0;
    const TCHAR* RouteStableName = TEXT("unavailable");
    const TCHAR* RouteDisplayName = TEXT("UNAVAILABLE ROUTE");
    const TCHAR* ProtocolStableName = TEXT("unavailable");
    const TCHAR* ProtocolDisplayName = TEXT("UNAVAILABLE PROTOCOL");
};

/**
 * Pure Mission 15 reducer and exact fourteen-record final projection. A named
 * resolution is only an intent until the approach, accord, conduit, and fixed
 * authoritative hold all succeed.
 */
struct ECHOESOFTHEBROKENSUN_API FEchoesBrokenSunMissionModel final
{
    [[nodiscard]] static EEchoesBrokenSunPhase DeterminePhase(
        const FEchoesBrokenSunMissionFacts& Facts);
    [[nodiscard]] static const TCHAR* StableName(
        EEchoesBrokenSunPhase Phase);
    [[nodiscard]] static bool TryPlanForLedger(
        echoes::sim::FutureWellChoice FoundingDoctrine,
        uint8 ReserveAuthorityFacts,
        echoes::sim::FutureWellChoice RecordedProtocol,
        FEchoesBrokenSunPlan& OutPlan);
    [[nodiscard]] static uint8 ResolutionMask(
        EEchoesFinalResolution Resolution);
    [[nodiscard]] static bool IsResolutionAvailable(
        const FEchoesBrokenSunPlan& Plan,
        EEchoesFinalResolution Resolution);
    [[nodiscard]] static echoes::sim::Vec2 ResolutionConvergenceSite(
        const FEchoesBrokenSunPlan& Plan,
        EEchoesFinalResolution Resolution);
    [[nodiscard]] static uint64 ResolutionHoldTicks(
        const FEchoesBrokenSunPlan& Plan,
        EEchoesFinalResolution Resolution);
    [[nodiscard]] static const TCHAR* ResolutionStableName(
        EEchoesFinalResolution Resolution);
    [[nodiscard]] static const TCHAR* ResolutionDisplayName(
        EEchoesFinalResolution Resolution);
    [[nodiscard]] static const TCHAR* ResolutionCostSummary(
        EEchoesFinalResolution Resolution);
};
