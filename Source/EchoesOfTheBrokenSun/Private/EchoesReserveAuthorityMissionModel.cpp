#include "EchoesReserveAuthorityMissionModel.h"

int32 FEchoesReserveAuthorityMissionModel::PoweredDistrictCount(
    const FEchoesReserveAuthorityMissionFacts& Facts)
{
    return (Facts.bLifeSupportPowered ? 1 : 0) +
        (Facts.bTransitPowered ? 1 : 0) +
        (Facts.bArchivePowered ? 1 : 0);
}

EEchoesCityDistrict FEchoesReserveAuthorityMissionModel::DeferredDistrict(
    const FEchoesReserveAuthorityMissionFacts& Facts)
{
    if (PoweredDistrictCount(Facts) != 2)
    {
        return EEchoesCityDistrict::LifeSupport;
    }
    if (!Facts.bLifeSupportPowered)
    {
        return EEchoesCityDistrict::LifeSupport;
    }
    if (!Facts.bTransitPowered)
    {
        return EEchoesCityDistrict::Transit;
    }
    return EEchoesCityDistrict::Archive;
}

EEchoesReserveAuthorityPhase
FEchoesReserveAuthorityMissionModel::DeterminePhase(
    const FEchoesReserveAuthorityMissionFacts& Facts)
{
    if (!Facts.bOperationActive)
    {
        return EEchoesReserveAuthorityPhase::Inactive;
    }
    if (!Facts.bLocalCoreIntact || !Facts.bMaraIntact ||
        !Facts.bLifeSupportIntact || !Facts.bTransitIntact ||
        !Facts.bArchiveIntact || !Facts.bSkirmishStillOngoing)
    {
        return EEchoesReserveAuthorityPhase::Failed;
    }

    const int32 PoweredCount = PoweredDistrictCount(Facts);
    if (PoweredCount > 2)
    {
        return EEchoesReserveAuthorityPhase::Failed;
    }
    if (!Facts.bAuthoritySecured)
    {
        return EEchoesReserveAuthorityPhase::SecureAuthority;
    }
    if (PoweredCount == 0)
    {
        return EEchoesReserveAuthorityPhase::AllocateFirstDistrict;
    }
    if (PoweredCount == 1)
    {
        return EEchoesReserveAuthorityPhase::AllocateSecondDistrict;
    }
    if (!Facts.bMaraAtDeferredDistrict)
    {
        return EEchoesReserveAuthorityPhase::ReachDeferredDistrict;
    }
    return EEchoesReserveAuthorityPhase::Complete;
}

const TCHAR* FEchoesReserveAuthorityMissionModel::StableName(
    EEchoesReserveAuthorityPhase Phase)
{
    switch (Phase)
    {
        case EEchoesReserveAuthorityPhase::Inactive: return TEXT("inactive");
        case EEchoesReserveAuthorityPhase::SecureAuthority:
            return TEXT("secure_authority");
        case EEchoesReserveAuthorityPhase::AllocateFirstDistrict:
            return TEXT("allocate_first_district");
        case EEchoesReserveAuthorityPhase::AllocateSecondDistrict:
            return TEXT("allocate_second_district");
        case EEchoesReserveAuthorityPhase::ReachDeferredDistrict:
            return TEXT("reach_deferred_district");
        case EEchoesReserveAuthorityPhase::Complete: return TEXT("complete");
        case EEchoesReserveAuthorityPhase::Failed: return TEXT("failed");
    }
    return TEXT("unknown");
}

FEchoesReserveAuthorityPlan
FEchoesReserveAuthorityMissionModel::PlanForChoice(
    echoes::sim::FutureWellChoice Choice)
{
    using echoes::sim::FutureWellChoice;
    switch (Choice)
    {
        case FutureWellChoice::Harvest:
            return {
                Choice,
                EEchoesCityDistrict::LifeSupport,
                echoes::sim::Vec2::FromTiles(15, 16),
                TEXT("emergency_rationing"),
                TEXT("EMERGENCY RATIONING")};
        case FutureWellChoice::Preserve:
            return {
                Choice,
                EEchoesCityDistrict::Archive,
                echoes::sim::Vec2::FromTiles(15, 15),
                TEXT("continuity_reserve"),
                TEXT("CONTINUITY RESERVE")};
        case FutureWellChoice::Reshape:
            return {
                Choice,
                EEchoesCityDistrict::Transit,
                echoes::sim::Vec2::FromTiles(16, 15),
                TEXT("transit_weave_reserve"),
                TEXT("TRANSIT-WEAVE RESERVE")};
        default:
            return {};
    }
}

echoes::sim::Vec2 FEchoesReserveAuthorityMissionModel::RelaySiteForDistrict(
    EEchoesCityDistrict District)
{
    using echoes::sim::Vec2;
    switch (District)
    {
        case EEchoesCityDistrict::LifeSupport:
            return Vec2::FromTiles(17, 12);
        case EEchoesCityDistrict::Transit:
            return Vec2::FromTiles(12, 17);
        case EEchoesCityDistrict::Archive:
            return Vec2::FromTiles(16, 14);
    }
    return {};
}
