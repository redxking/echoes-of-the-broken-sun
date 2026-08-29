#pragma once

#include "CoreMinimal.h"
#include "EchoesSimCore/Simulation.h"

enum class EEchoesCityReservePhase : uint8
{
    Inactive,
    StabilizePriority,
    StabilizeSecondary,
    StabilizeFinal,
    Complete,
    Failed
};

enum class EEchoesCityDistrict : uint8
{
    LifeSupport,
    Transit,
    Archive
};

struct FEchoesCityReserveMissionFacts final
{
    bool bOperationActive = false;
    bool bLocalCoreIntact = false;
    bool bLifeSupportIntact = false;
    bool bTransitIntact = false;
    bool bArchiveIntact = false;
    bool bLifeSupportPowered = false;
    bool bTransitPowered = false;
    bool bArchivePowered = false;
    bool bSkirmishStillOngoing = true;
};

struct FEchoesCityReserveGrid final
{
    echoes::sim::FutureWellChoice PriorChoice =
        echoes::sim::FutureWellChoice::Dormant;
    EEchoesCityDistrict Priority = EEchoesCityDistrict::LifeSupport;
    EEchoesCityDistrict Secondary = EEchoesCityDistrict::Transit;
    EEchoesCityDistrict Final = EEchoesCityDistrict::Archive;
    const TCHAR* StableName = TEXT("unavailable");
    const TCHAR* DisplayName = TEXT("UNAVAILABLE RESERVE PLAN");
};

/** Pure mission-03 reducer and inherited Well-choice priority mapping. */
struct ECHOESOFTHEBROKENSUN_API FEchoesCityReserveMissionModel final
{
    [[nodiscard]] static EEchoesCityReservePhase DeterminePhase(
        const FEchoesCityReserveMissionFacts& Facts,
        const FEchoesCityReserveGrid& Grid);
    [[nodiscard]] static const TCHAR* StableName(
        EEchoesCityReservePhase Phase);
    [[nodiscard]] static FEchoesCityReserveGrid GridForChoice(
        echoes::sim::FutureWellChoice Choice);
    [[nodiscard]] static echoes::sim::Vec2 SiteForDistrict(
        EEchoesCityDistrict District);
    [[nodiscard]] static const TCHAR* DistrictDisplayName(
        EEchoesCityDistrict District);
};
