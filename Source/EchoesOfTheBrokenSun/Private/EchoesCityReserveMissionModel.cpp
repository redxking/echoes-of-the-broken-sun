#include "EchoesCityReserveMissionModel.h"

namespace
{
bool IsIntact(
    const FEchoesCityReserveMissionFacts& Facts,
    EEchoesCityDistrict District)
{
    switch (District)
    {
        case EEchoesCityDistrict::LifeSupport:
            return Facts.bLifeSupportIntact;
        case EEchoesCityDistrict::Transit:
            return Facts.bTransitIntact;
        case EEchoesCityDistrict::Archive:
            return Facts.bArchiveIntact;
    }
    return false;
}

bool IsPowered(
    const FEchoesCityReserveMissionFacts& Facts,
    EEchoesCityDistrict District)
{
    switch (District)
    {
        case EEchoesCityDistrict::LifeSupport:
            return Facts.bLifeSupportPowered;
        case EEchoesCityDistrict::Transit:
            return Facts.bTransitPowered;
        case EEchoesCityDistrict::Archive:
            return Facts.bArchivePowered;
    }
    return false;
}
}

EEchoesCityReservePhase FEchoesCityReserveMissionModel::DeterminePhase(
    const FEchoesCityReserveMissionFacts& Facts,
    const FEchoesCityReserveGrid& Grid)
{
    if (!Facts.bOperationActive)
    {
        return EEchoesCityReservePhase::Inactive;
    }
    if (!Facts.bLocalCoreIntact || !Facts.bSkirmishStillOngoing ||
        !IsIntact(Facts, EEchoesCityDistrict::LifeSupport) ||
        !IsIntact(Facts, EEchoesCityDistrict::Transit) ||
        !IsIntact(Facts, EEchoesCityDistrict::Archive))
    {
        return EEchoesCityReservePhase::Failed;
    }
    if (!IsPowered(Facts, Grid.Priority))
    {
        return EEchoesCityReservePhase::StabilizePriority;
    }
    if (!IsPowered(Facts, Grid.Secondary))
    {
        return EEchoesCityReservePhase::StabilizeSecondary;
    }
    if (!IsPowered(Facts, Grid.Final))
    {
        return EEchoesCityReservePhase::StabilizeFinal;
    }
    return EEchoesCityReservePhase::Complete;
}

const TCHAR* FEchoesCityReserveMissionModel::StableName(
    EEchoesCityReservePhase Phase)
{
    switch (Phase)
    {
        case EEchoesCityReservePhase::Inactive: return TEXT("inactive");
        case EEchoesCityReservePhase::StabilizePriority:
            return TEXT("stabilize_priority");
        case EEchoesCityReservePhase::StabilizeSecondary:
            return TEXT("stabilize_secondary");
        case EEchoesCityReservePhase::StabilizeFinal:
            return TEXT("stabilize_final");
        case EEchoesCityReservePhase::Complete: return TEXT("complete");
        case EEchoesCityReservePhase::Failed: return TEXT("failed");
    }
    return TEXT("unknown");
}

FEchoesCityReserveGrid FEchoesCityReserveMissionModel::GridForChoice(
    echoes::sim::FutureWellChoice Choice)
{
    using echoes::sim::FutureWellChoice;
    switch (Choice)
    {
        case FutureWellChoice::Harvest:
            return {
                Choice,
                EEchoesCityDistrict::LifeSupport,
                EEchoesCityDistrict::Transit,
                EEchoesCityDistrict::Archive,
                TEXT("emergency_load_shed"),
                TEXT("EMERGENCY LOAD-SHED PLAN")};
        case FutureWellChoice::Preserve:
            return {
                Choice,
                EEchoesCityDistrict::Archive,
                EEchoesCityDistrict::LifeSupport,
                EEchoesCityDistrict::Transit,
                TEXT("continuity_reserve"),
                TEXT("CONTINUITY RESERVE PLAN")};
        case FutureWellChoice::Reshape:
            return {
                Choice,
                EEchoesCityDistrict::Transit,
                EEchoesCityDistrict::LifeSupport,
                EEchoesCityDistrict::Archive,
                TEXT("manifested_transit_weave"),
                TEXT("MANIFESTED TRANSIT-WEAVE PLAN")};
        default:
            return {};
    }
}

echoes::sim::Vec2 FEchoesCityReserveMissionModel::SiteForDistrict(
    EEchoesCityDistrict District)
{
    using echoes::sim::Vec2;
    switch (District)
    {
        case EEchoesCityDistrict::LifeSupport:
            return Vec2::FromTiles(24, 10);
        case EEchoesCityDistrict::Transit:
            return Vec2::FromTiles(10, 24);
        case EEchoesCityDistrict::Archive:
            return Vec2::FromTiles(20, 20);
    }
    return {};
}

const TCHAR* FEchoesCityReserveMissionModel::DistrictDisplayName(
    EEchoesCityDistrict District)
{
    switch (District)
    {
        case EEchoesCityDistrict::LifeSupport:
            return TEXT("LIFE SUPPORT");
        case EEchoesCityDistrict::Transit:
            return TEXT("TRANSIT");
        case EEchoesCityDistrict::Archive:
            return TEXT("ARCHIVE CONTINUITY");
    }
    return TEXT("UNKNOWN DISTRICT");
}
