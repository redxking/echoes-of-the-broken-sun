#pragma once

#include "CoreMinimal.h"
#include "EchoesCityReserveMissionModel.h"
#include "EchoesSimCore/Simulation.h"

enum class EEchoesReserveAuthorityPhase : uint8
{
    Inactive,
    SecureAuthority,
    AllocateFirstDistrict,
    AllocateSecondDistrict,
    ReachDeferredDistrict,
    Complete,
    Failed
};

struct FEchoesReserveAuthorityMissionFacts final
{
    bool bOperationActive = false;
    bool bLocalCoreIntact = false;
    bool bMaraIntact = false;
    bool bLifeSupportIntact = false;
    bool bTransitIntact = false;
    bool bArchiveIntact = false;
    bool bAuthoritySecured = false;
    bool bLifeSupportPowered = false;
    bool bTransitPowered = false;
    bool bArchivePowered = false;
    bool bMaraAtDeferredDistrict = false;
    bool bSkirmishStillOngoing = true;
};

struct FEchoesReserveAuthorityPlan final
{
    echoes::sim::FutureWellChoice PriorChoice =
        echoes::sim::FutureWellChoice::Dormant;
    EEchoesCityDistrict RecommendedFirstDistrict =
        EEchoesCityDistrict::LifeSupport;
    echoes::sim::Vec2 AuthoritySite;
    const TCHAR* StableName = TEXT("unavailable");
    const TCHAR* DisplayName = TEXT("UNAVAILABLE RESERVE DOCTRINE");
};

/** Pure mission-09 reducer and inherited eight-record reserve doctrine. */
struct ECHOESOFTHEBROKENSUN_API FEchoesReserveAuthorityMissionModel final
{
    [[nodiscard]] static EEchoesReserveAuthorityPhase DeterminePhase(
        const FEchoesReserveAuthorityMissionFacts& Facts);
    [[nodiscard]] static const TCHAR* StableName(
        EEchoesReserveAuthorityPhase Phase);
    [[nodiscard]] static FEchoesReserveAuthorityPlan PlanForChoice(
        echoes::sim::FutureWellChoice Choice);
    [[nodiscard]] static echoes::sim::Vec2 RelaySiteForDistrict(
        EEchoesCityDistrict District);
    [[nodiscard]] static int32 PoweredDistrictCount(
        const FEchoesReserveAuthorityMissionFacts& Facts);
    [[nodiscard]] static EEchoesCityDistrict DeferredDistrict(
        const FEchoesReserveAuthorityMissionFacts& Facts);
};
