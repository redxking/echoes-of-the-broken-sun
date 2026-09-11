// Copyright Echoes of the Broken Sun. All Rights Reserved.
// Author: Angelis Pseftis
#pragma once

#include "CoreMinimal.h"
#include "EchoesSimCore/Simulation.h"

struct FEchoesContentCatalog;

/**
 * One wording for "why this unit cannot start", shared by the status line,
 * the command deck and the production panel. The owner's D2 play test
 * (2026-09-11) read "[INSUFFICIENT_RESOURCES] The selected unit cannot be
 * funded." with 1,580 Matter in hand and no way to learn that Dawn was the
 * missing resource or where Dawn comes from; every refusal now names the
 * unit, its price, the holding, the short resource and that resource's source.
 */
struct ECHOESOFTHEBROKENSUN_API FEchoesProductionReasonText final
{
    /**
     * Upper-case roster name: the catalog's display name when it has one;
     * for the Meridian roster without a catalog, the M01 role name; otherwise
     * empty, so a caller keeps its own label rather than showing a wrong one.
     */
    [[nodiscard]] static FString UnitName(
        echoes::sim::Faction Faction,
        echoes::sim::EntityType Type,
        const FEchoesContentCatalog* Catalog);

    /** "85M 20D" (Dawn omitted when zero): the price line of a command tile. */
    [[nodiscard]] static FText TileCost(const echoes::sim::ResourcePool& Cost);

    /** "85 Matter / 20 Dawn" with thousands separators, for prose. */
    [[nodiscard]] static FString ProseCost(const echoes::sim::ResourcePool& Cost);

    /**
     * Status-line refusal for a funding shortfall. Reason selects the short
     * resource when it is InsufficientMatter or InsufficientDawn; any other
     * value is judged from Cost against Held, Matter first. The Dawn source
     * quotes the configured Future Well yields rather than fixed numbers.
     */
    [[nodiscard]] static FString FundingRefusal(
        echoes::sim::ProductionStartBlockReason Reason,
        const FString& UnitName,
        const echoes::sim::ResourcePool& Cost,
        const echoes::sim::ResourcePool& Held,
        const echoes::sim::FutureWellRules& Well,
        uint32 TicksPerSecond);

    /**
     * Short availability line for a command tile. Empty for None and Busy: a
     * busy producer with queue room still accepts the order.
     */
    [[nodiscard]] static FText Availability(
        echoes::sim::ProductionStartBlockReason Reason);

    /**
     * Reasons that make a tile inert instead of explaining a refusal: there is
     * no producer for this unit, or it is not finished. Resource, logistics
     * and army-limit shortfalls keep the tile live so pressing it explains.
     */
    [[nodiscard]] static bool IsStructural(
        echoes::sim::ProductionStartBlockReason Reason);
};
