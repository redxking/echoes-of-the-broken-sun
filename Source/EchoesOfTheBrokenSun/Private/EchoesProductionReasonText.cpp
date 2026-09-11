// Copyright Echoes of the Broken Sun. All Rights Reserved.
// Author: Angelis Pseftis
#include "EchoesProductionReasonText.h"

#include "EchoesCommandDeckModel.h"
#include "EchoesContentSubsystem.h"

using echoes::sim::EntityType;
using echoes::sim::Faction;
using echoes::sim::ProductionStartBlockReason;
using echoes::sim::ResourcePool;

namespace
{
FString Grouped(int32 Value)
{
    return FText::AsNumber(Value).ToString();
}
}

FString FEchoesProductionReasonText::UnitName(
    Faction FactionValue,
    EntityType Type,
    const FEchoesContentCatalog* Catalog)
{
    if (Catalog != nullptr)
    {
        if (const FEchoesUnitContent* Unit = Catalog->FindUnit(FactionValue, Type))
        {
            return Unit->DisplayName.ToUpper();
        }
        if (const FEchoesBuildingContent* Building = Catalog->FindBuilding(FactionValue, Type))
        {
            return Building->DisplayName.ToUpper();
        }
    }
    if (FactionValue == Faction::MeridianCompact)
    {
        switch (Type)
        {
            case EntityType::Worker:
            case EntityType::Soldier:
            case EntityType::HeavyUnit:
            case EntityType::ScoutUnit:
            case EntityType::Barracks:
            case EntityType::Dropoff:
            case EntityType::UtilityStructure:
                return FString(FEchoesCommandDeckModel::GetM01RoleName(Type)).ToUpper();
            default:
                break;
        }
    }
    return FString();
}

FText FEchoesProductionReasonText::TileCost(const ResourcePool& Cost)
{
    return Cost.dawnshards > 0
        ? FText::FromString(FString::Printf(TEXT("%dM %dD"), Cost.material, Cost.dawnshards))
        : FText::FromString(FString::Printf(TEXT("%dM"), Cost.material));
}

FString FEchoesProductionReasonText::ProseCost(const ResourcePool& Cost)
{
    return FString::Printf(TEXT("%s Matter / %s Dawn"),
        *Grouped(Cost.material), *Grouped(Cost.dawnshards));
}

FString FEchoesProductionReasonText::FundingRefusal(
    ProductionStartBlockReason Reason,
    const FString& UnitName,
    const ResourcePool& Cost,
    const ResourcePool& Held,
    const echoes::sim::FutureWellRules& Well,
    uint32 TicksPerSecond)
{
    const bool bDawnShort = Reason == ProductionStartBlockReason::InsufficientDawn ||
        (Reason != ProductionStartBlockReason::InsufficientMatter &&
         Held.material >= Cost.material && Held.dawnshards < Cost.dawnshards);
    FString Source;
    if (bDawnShort)
    {
        const FString Interval = TicksPerSecond > 0
            ? FString::Printf(TEXT("%llu s"),
                static_cast<unsigned long long>(
                    (Well.preserveIntervalTicks + TicksPerSecond - 1) / TicksPerSecond))
            : FString::Printf(TEXT("%llu ticks"),
                static_cast<unsigned long long>(Well.preserveIntervalTicks));
        Source = FString::Printf(
            TEXT("Dawn comes from a Future Well: Harvest yields %s at once; Preserve yields %s every %s."),
            *Grouped(Well.harvestImmediateDawn), *Grouped(Well.preserveDawnPerInterval), *Interval);
    }
    else
    {
        Source = TEXT("Surveyors deliver Matter from deposits to a connected Power Link or Anchor.");
    }
    return FString::Printf(TEXT("[%s] %s costs %s; you hold %s. %s"),
        bDawnShort ? TEXT("INSUFFICIENT_DAWN") : TEXT("INSUFFICIENT_MATTER"),
        *UnitName, *ProseCost(Cost), *ProseCost(Held), *Source);
}

FText FEchoesProductionReasonText::Availability(ProductionStartBlockReason Reason)
{
    switch (Reason)
    {
        case ProductionStartBlockReason::None:
        case ProductionStartBlockReason::Busy:
            return FText::GetEmpty();
        case ProductionStartBlockReason::QueueFull:
            return FText::Format(
                NSLOCTEXT("EchoesProduction", "AvailabilityQueueFull", "Queue full: {0} waiting"),
                static_cast<int32>(echoes::sim::Entity::kMaxProductionQueue));
        case ProductionStartBlockReason::InsufficientMatter:
            return NSLOCTEXT("EchoesProduction", "AvailabilityMatter", "Needs more Matter");
        case ProductionStartBlockReason::InsufficientDawn:
            return NSLOCTEXT("EchoesProduction", "AvailabilityDawn", "Needs Dawn (Future Well)");
        case ProductionStartBlockReason::LogisticsCapacity:
            return NSLOCTEXT("EchoesProduction", "AvailabilityLogistics", "Logistics full: build a Power Link");
        case ProductionStartBlockReason::EntityCapacity:
            return NSLOCTEXT("EchoesProduction", "AvailabilityEntities", "Entity limit reached");
        case ProductionStartBlockReason::MobileEntityLimit:
            return FText::Format(
                NSLOCTEXT("EchoesProduction", "AvailabilityArmy", "Army limit {0}"),
                static_cast<int32>(echoes::sim::kMobileEntityLimit));
        case ProductionStartBlockReason::ProducerIncomplete:
            return NSLOCTEXT("EchoesProduction", "AvailabilityIncomplete", "Under construction");
        case ProductionStartBlockReason::Unpowered:
            return NSLOCTEXT("EchoesProduction", "AvailabilityUnpowered", "Unpowered: extend a Power Link");
        case ProductionStartBlockReason::InvalidProducer:
        case ProductionStartBlockReason::UnsupportedUnit:
            break;
    }
    return NSLOCTEXT("EchoesProduction", "AvailabilityUnavailable", "Unavailable");
}

bool FEchoesProductionReasonText::IsStructural(ProductionStartBlockReason Reason)
{
    return Reason == ProductionStartBlockReason::InvalidProducer ||
        Reason == ProductionStartBlockReason::ProducerIncomplete ||
        Reason == ProductionStartBlockReason::UnsupportedUnit;
}
