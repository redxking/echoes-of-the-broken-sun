#include "EchoesSimCore/Simulation.h"

#include <algorithm>
#include <array>
#include <limits>
#include <stdexcept>
#include <tuple>
#include <utility>

namespace echoes::sim {
namespace {

constexpr std::uint64_t kFnvOffset = 14695981039346656037ULL;
constexpr std::uint64_t kFnvPrime = 1099511628211ULL;
constexpr std::uint32_t kMaximumMapTiles = 4U * 1024U * 1024U;
constexpr std::uint32_t kMaximumSerializedEntities = 64U * 1024U;
constexpr std::uint32_t kMaximumSerializedCommands =
    static_cast<std::uint32_t>(kMaximumCommandLogEntries);
constexpr std::size_t kMaximumCachedPathFields = 128;
constexpr std::int32_t kGuardLeashRaw = 6 * kFixedScale;
constexpr std::int32_t kGuardFollowRaw = 2 * kFixedScale;
constexpr std::int32_t kPatrolLeashRaw = 6 * kFixedScale;
constexpr std::uint32_t kMaximumTicksPerSecond = 1000;
constexpr Tick kMaximumSupportedTick = std::numeric_limits<Tick>::max() / 2;
constexpr std::int32_t kMaximumVisionTiles = 256;
constexpr std::int32_t kMaximumProductionTicks = 60 * 1000;
constexpr std::uint32_t kLegacySnapshotVersion = 20;
constexpr std::uint32_t kPriorSnapshotVersion = 21;
constexpr std::uint32_t kChoirSnapshotVersion = 22;
constexpr std::uint32_t kProtectedCommandCoreSnapshotVersion = 23;
constexpr std::size_t kLegacyFactionCount = 2;
constexpr std::size_t kLegacyResearchTypeCount = 5;
constexpr std::size_t kLegacySerializedEntityBytes = 202;
constexpr std::size_t kPriorSerializedEntityBytes = 210;
constexpr std::size_t kSerializedEntityBytes = 235;
constexpr std::size_t kSerializedCommandBytes = 38;
constexpr std::size_t kSerializedCommandResolutionReceiptBytes = 19;
constexpr std::size_t kSnapshotFixedBytesAfterConfig = 132;
constexpr std::int32_t kMaximumMapDimension =
    std::numeric_limits<std::int32_t>::max() / kFixedScale;
constexpr std::uint8_t kValidCommandCoreProtectionMask =
    static_cast<std::uint8_t>((1U << kMaximumPlayers) - 1U);

[[nodiscard]] bool HasChoirSnapshotSchema(std::uint32_t version) {
    return version >= kChoirSnapshotVersion;
}

[[nodiscard]] bool HasProtectedCommandCoreSnapshotSchema(
    std::uint32_t version) {
    return version >= kProtectedCommandCoreSnapshotVersion;
}

[[nodiscard]] std::int64_t Abs64(std::int64_t value) {
    return value < 0 ? -value : value;
}

[[nodiscard]] std::uint64_t Fnv1a(std::span<const std::uint8_t> bytes) {
    std::uint64_t hash = kFnvOffset;
    for (const std::uint8_t byte : bytes) {
        hash ^= byte;
        hash *= kFnvPrime;
    }
    return hash;
}

void SetError(std::string* destination, const std::string& message) {
    if (destination != nullptr) {
        *destination = message;
    }
}

[[nodiscard]] bool CommandLess(const Command& lhs, const Command& rhs) {
    return std::tie(lhs.executeTick,
                    lhs.player,
                    lhs.sequence,
                    lhs.type,
                    lhs.actor,
                    lhs.target,
                    lhs.position.x,
                    lhs.position.y,
                    lhs.buildType,
                    lhs.wellChoice,
                    lhs.warformAdaptation,
                    lhs.researchType) <
           std::tie(rhs.executeTick,
                    rhs.player,
                    rhs.sequence,
                    rhs.type,
                    rhs.actor,
                    rhs.target,
                    rhs.position.x,
                    rhs.position.y,
                    rhs.buildType,
                    rhs.wellChoice,
                    rhs.warformAdaptation,
                    rhs.researchType);
}

[[nodiscard]] bool HasSameCommandKey(const Command& lhs, const Command& rhs) {
    return lhs.player == rhs.player && lhs.sequence == rhs.sequence;
}

[[nodiscard]] bool IsValidFaction(Faction faction) {
    return faction == Faction::MeridianCompact ||
           faction == Faction::KharuunAssemblies ||
           faction == Faction::HollowChoir;
}

[[nodiscard]] bool IsValidEntityType(EntityType type) {
    return type >= EntityType::Worker && type <= EntityType::UtilityStructure;
}

constexpr std::array<EntityType, 8> kConfigurableEntityTypes{
    EntityType::Worker,
    EntityType::Soldier,
    EntityType::CommandCore,
    EntityType::Dropoff,
    EntityType::Barracks,
    EntityType::HeavyUnit,
    EntityType::ScoutUnit,
    EntityType::UtilityStructure,
};

[[nodiscard]] bool IsConfigurableEntityType(EntityType type) {
    return std::find(kConfigurableEntityTypes.begin(),
                     kConfigurableEntityTypes.end(), type) !=
           kConfigurableEntityTypes.end();
}

[[nodiscard]] bool IsBarracksUnitType(EntityType type) {
    return type == EntityType::Soldier || type == EntityType::HeavyUnit ||
           type == EntityType::ScoutUnit;
}

[[nodiscard]] bool IsValidTerrain(Terrain terrain) {
    return terrain >= Terrain::Open && terrain <= Terrain::Scarred;
}

[[nodiscard]] bool IsValidCommandType(CommandType type) {
    return type >= CommandType::Stop &&
           type <= CommandType::ReconcileToPossible;
}

[[nodiscard]] bool IsValidCommandResolutionOutcome(
    CommandResolutionOutcome outcome) {
    return outcome >= CommandResolutionOutcome::Applied &&
           outcome <= CommandResolutionOutcome::InvalidPosition;
}

[[nodiscard]] bool IsValidResearchType(ResearchType type) {
    return type >= ResearchType::None &&
           type <= ResearchType::ChoirSharedResolution;
}

[[nodiscard]] bool IsValidChoirIdentityState(ChoirIdentityState state) {
    return state >= ChoirIdentityState::NotChoir &&
           state <= ChoirIdentityState::DualResolvePossible;
}

[[nodiscard]] bool IsValidWellChoice(FutureWellChoice choice) {
    return choice >= FutureWellChoice::Dormant && choice <= FutureWellChoice::Reshape;
}

[[nodiscard]] bool IsValidWarformAdaptation(WarformAdaptation adaptation) {
    return adaptation >= WarformAdaptation::None &&
           adaptation <= WarformAdaptation::Striker;
}

[[nodiscard]] bool IsValidAiPersonality(AiPersonality personality) {
    return personality >= AiPersonality::Balanced &&
           personality <= AiPersonality::Adaptive;
}

// Standard Adaptive play keeps its combat force in an opening posture for five
// minutes. It still defends visible threats, builds, gathers, produces, and
// researches, but does not chase anonymous vibration contacts or roam the map.
constexpr Tick kAdaptiveOpeningPostureTicks = 6000;

[[nodiscard]] std::int32_t SaturatingAdd(std::int32_t lhs, std::int32_t rhs);
[[nodiscard]] bool ResourceCovers(const ResourcePool& available,
                                  const ResourcePool& cost);

[[nodiscard]] bool IsBuildingType(EntityType type) {
    return type == EntityType::CommandCore || type == EntityType::Dropoff ||
           type == EntityType::Barracks || type == EntityType::UtilityStructure;
}

[[nodiscard]] bool IsDropoffType(EntityType type) {
    return type == EntityType::CommandCore || type == EntityType::Dropoff;
}

[[nodiscard]] const EntityArchetypeRules& ArchetypeFor(
    const SimulationRules& rules,
    Faction faction,
    EntityType type) {
    return rules.archetypes[static_cast<std::size_t>(faction)]
                           [static_cast<std::size_t>(type)];
}

[[nodiscard]] std::int32_t FootprintHalfExtentFor(
    const SimulationRules& rules,
    Faction faction,
    EntityType type) {
    switch (type) {
        case EntityType::Worker:
        case EntityType::Soldier:
        case EntityType::HeavyUnit:
        case EntityType::ScoutUnit:
        case EntityType::CommandCore:
        case EntityType::Dropoff:
        case EntityType::Barracks:
        case EntityType::UtilityStructure:
            return ArchetypeFor(rules, faction, type).footprintHalfExtentRaw;
        case EntityType::FutureWell:
            return kFixedScale / 2;
        case EntityType::ResourceNode:
            return kFixedScale / 3;
    }
    return kFixedScale;
}

[[nodiscard]] ResourcePool BuildCostFor(const SimulationRules& rules,
                                        Faction faction,
                                        EntityType type) {
    switch (type) {
        case EntityType::CommandCore:
        case EntityType::Dropoff:
        case EntityType::Barracks:
        case EntityType::UtilityStructure:
            return ArchetypeFor(rules, faction, type).cost;
        default:
            return {};
    }
}

[[nodiscard]] ResourcePool ProductionCostFor(const SimulationRules& rules,
                                             Faction faction,
                                             EntityType type) {
    switch (type) {
        case EntityType::Worker:
        case EntityType::Soldier:
        case EntityType::HeavyUnit:
        case EntityType::ScoutUnit:
            return ArchetypeFor(rules, faction, type).cost;
        default:
            return {};
    }
}

[[nodiscard]] std::int32_t PopulationCostFor(const SimulationRules& rules,
                                             Faction faction,
                                             EntityType type) {
    switch (type) {
        case EntityType::Worker:
        case EntityType::Soldier:
        case EntityType::HeavyUnit:
        case EntityType::ScoutUnit:
            return ArchetypeFor(rules, faction, type).populationCost;
        default:
            return 0;
    }
}

[[nodiscard]] bool IsValidSimulationRules(const SimulationRules& rules) {
    if (rules.version != 1 && rules.version != 2) {
        return false;
    }
    const std::size_t supportedFactionCount =
        rules.version == 1 ? kLegacyFactionCount : kFactionCount;
    for (std::size_t faction = 0; faction < supportedFactionCount; ++faction) {
        for (const EntityType type : kConfigurableEntityTypes) {
            const EntityArchetypeRules& archetype =
                rules.archetypes[faction][static_cast<std::size_t>(type)];
            if (archetype.cost.material < 0 || archetype.cost.dawnshards < 0 ||
                archetype.maxHitPoints <= 0 ||
                archetype.movementPerTickRaw < 0 ||
                archetype.visionTiles < 0 ||
                archetype.visionTiles > kMaximumVisionTiles ||
                archetype.attackRangeRaw < 0 || archetype.attackDamage < 0 ||
                archetype.attackPeriodTicks > kMaximumSupportedTick ||
                archetype.workRate < 0 || archetype.cargoCapacity < 0 ||
                archetype.constructionRequired < 0 ||
                archetype.populationCost < 0 ||
                archetype.populationCapacity < 0 ||
                archetype.productionTicks < 0 ||
                archetype.productionTicks > kMaximumProductionTicks ||
                archetype.footprintHalfExtentRaw <= 0 ||
                archetype.footprintHalfExtentRaw > 16 * kFixedScale) {
                return false;
            }
        }
        const auto& worker =
            rules.archetypes[faction][static_cast<std::size_t>(EntityType::Worker)];
        const auto& soldier =
            rules.archetypes[faction][static_cast<std::size_t>(EntityType::Soldier)];
        if (worker.populationCost <= 0 || worker.productionTicks <= 0 ||
            worker.workRate <= 0 || worker.cargoCapacity <= 0 ||
            soldier.populationCost <= 0 || soldier.productionTicks <= 0) {
            return false;
        }
        for (const EntityType type : {EntityType::HeavyUnit,
                                     EntityType::ScoutUnit}) {
            const auto& unit =
                rules.archetypes[faction][static_cast<std::size_t>(type)];
            if (unit.populationCost <= 0 || unit.productionTicks <= 0 ||
                unit.attackDamage <= 0 || unit.attackPeriodTicks == 0) {
                return false;
            }
        }
        for (const EntityType type : {EntityType::CommandCore,
                                     EntityType::Dropoff,
                                     EntityType::Barracks,
                                     EntityType::UtilityStructure}) {
            if (rules.archetypes[faction][static_cast<std::size_t>(type)]
                    .constructionRequired <= 0) {
                return false;
            }
        }
    }
    const FutureWellRules& well = rules.futureWell;
    const BulwarkDeploymentRules& bulwark = rules.bulwarkDeployment;
    const RelaySupplyRules& relay = rules.relaySupply;
    const WaystoneMigrationRules& waystone = rules.waystoneMigration;
    const WarformAdaptationRules& adaptation = rules.warformAdaptation;
    const MineralCoverRules& mineralCover = rules.mineralCover;
    const VibrationDetectionRules& vibration = rules.vibrationDetection;
    const PoweredAegisRules& aegis = rules.poweredAegis;
    const EntityArchetypeRules& aegisArchetype =
        rules.archetypes[static_cast<std::size_t>(Faction::MeridianCompact)]
                        [static_cast<std::size_t>(EntityType::UtilityStructure)];
    bool researchValid = true;
    const std::size_t supportedResearchCount =
        rules.version == 1 ? kLegacyResearchTypeCount : rules.research.size();
    for (std::size_t index = 1; index < supportedResearchCount; ++index) {
        const ResearchRules& research = rules.research[index];
        const ResearchType type = static_cast<ResearchType>(index);
        researchValid = researchValid && IsValidFaction(research.faction) &&
            research.cost.material >= 0 && research.cost.dawnshards >= 0 &&
            research.researchTicks > 0 &&
            research.researchTicks <= kMaximumProductionTicks &&
            IsValidResearchType(research.prerequisite) &&
            research.prerequisite != type &&
            research.combatDamagePercent >= 100 &&
            research.combatDamagePercent <= 300 &&
            research.combatVisionPercent >= 100 &&
            research.combatVisionPercent <= 300;
        if (research.prerequisite != ResearchType::None) {
            const ResearchRules& prerequisite = rules.research[
                static_cast<std::size_t>(research.prerequisite)];
            researchValid = researchValid && prerequisite.faction == research.faction &&
                prerequisite.prerequisite == ResearchType::None;
        }
    }
    const ChoirIdentityRules& identity = rules.choirIdentity;
    const ChoirCoherenceRules& coherence = rules.choirCoherence;
    const bool choirRulesValid = rules.version == 1 ||
        (identity.durationTicks > 0 &&
         identity.durationTicks <= identity.cooldownTicks &&
         identity.cooldownTicks <= kMaximumSupportedTick &&
         identity.dawnCost > 0 && identity.dawnCost <= 100000 &&
         identity.manifestDamagePercent > 100 &&
         identity.manifestDamagePercent <= 300 &&
         identity.possibleMovementPercent > 100 &&
         identity.possibleMovementPercent <= 300 &&
         identity.possibleVisionPercent > 100 &&
         identity.possibleVisionPercent <= 300 &&
         coherence.upkeepIntervalTicks > 0 &&
         coherence.upkeepIntervalTicks <= kMaximumSupportedTick &&
         coherence.dawnCostPerStructure > 0 &&
         coherence.dawnCostPerStructure <= 100000);
    return researchValid && choirRulesValid &&
           well.harvestImmediateDawn >= 0 &&
           well.preserveDawnPerInterval >= 0 &&
           well.preserveIntervalTicks > 0 &&
           well.preserveIntervalTicks <= kMaximumSupportedTick &&
           well.preserveVisionTiles >= 0 &&
           well.preserveVisionTiles <= kMaximumVisionTiles &&
           well.reshapeDawnCost >= 0 && well.reshapeDurationMinimumTicks > 0 &&
           well.reshapeDurationMinimumTicks <= well.reshapeDurationMaximumTicks &&
           well.reshapeDurationMaximumTicks <=
               std::numeric_limits<std::uint32_t>::max() &&
           well.reshapeDurationMaximumTicks <= kMaximumSupportedTick &&
           bulwark.coverDepthRaw > 0 &&
           bulwark.coverDepthRaw <= 16 * kFixedScale &&
           bulwark.coverHalfWidthRaw > 0 &&
           bulwark.coverHalfWidthRaw <= 16 * kFixedScale &&
           bulwark.damageReductionPercent > 0 &&
           bulwark.damageReductionPercent < 100 &&
           bulwark.deployedMovementPercent > 0 &&
           bulwark.deployedMovementPercent < 100 &&
           relay.connectionRadiusRaw > 0 &&
           relay.connectionRadiusRaw <= 32 * kFixedScale &&
           relay.capacityBonus > 0 && relay.capacityBonus <= 1000 &&
           relay.durationTicks > 0 &&
           relay.durationTicks <= relay.cooldownTicks &&
           relay.cooldownTicks <= kMaximumSupportedTick &&
           waystone.movementPerTickRaw > 0 &&
           waystone.movementPerTickRaw <= kFixedScale &&
           rules.archetypes[static_cast<std::size_t>(Faction::KharuunAssemblies)]
                           [static_cast<std::size_t>(EntityType::Dropoff)]
                   .movementPerTickRaw == waystone.movementPerTickRaw &&
           waystone.uprootTicks > 0 &&
           waystone.uprootTicks <= kMaximumSupportedTick &&
           waystone.rootTicks > 0 &&
           waystone.rootTicks <= kMaximumSupportedTick &&
           waystone.mobileDamageTakenPercent > 100 &&
           waystone.mobileDamageTakenPercent <= 300 &&
           adaptation.siteRadiusRaw > 0 &&
           adaptation.siteRadiusRaw <= 32 * kFixedScale &&
           adaptation.moltTicks > 0 &&
           adaptation.moltTicks <= kMaximumSupportedTick &&
           adaptation.dawnCost > 0 && adaptation.dawnCost <= 100000 &&
           adaptation.moltDamageTakenPercent > 100 &&
           adaptation.moltDamageTakenPercent <= 300 &&
           adaptation.carapaceHealthPercent > 100 &&
           adaptation.carapaceHealthPercent <= 300 &&
           adaptation.carapaceMovementPercent > 0 &&
           adaptation.carapaceMovementPercent < 100 &&
           adaptation.strikerDamagePercent > 100 &&
           adaptation.strikerDamagePercent <= 300 &&
           adaptation.strikerCooldownPercent > 0 &&
           adaptation.strikerCooldownPercent < 100 &&
           mineralCover.castRangeRaw > 0 &&
           mineralCover.castRangeRaw <= 32 * kFixedScale &&
           mineralCover.durationTicks > 0 &&
           mineralCover.durationTicks <= mineralCover.cooldownTicks &&
           mineralCover.cooldownTicks <= kMaximumSupportedTick &&
           mineralCover.dawnCost > 0 && mineralCover.dawnCost <= 100000 &&
           mineralCover.maxHitPoints > 0 &&
           mineralCover.maxHitPoints <= 1000000 &&
           mineralCover.halfExtentRaw >= kFixedScale / 4 &&
           mineralCover.halfExtentRaw <= 2 * kFixedScale &&
           vibration.resonantRadiusRaw > 0 &&
           vibration.resonantRadiusRaw <= 64 * kFixedScale &&
           vibration.listeningSpineRadiusRaw > 0 &&
           vibration.listeningSpineRadiusRaw <= 64 * kFixedScale &&
           vibration.signatureLingerTicks > 0 &&
           vibration.signatureLingerTicks <= kMaximumSupportedTick &&
           vibration.contactResolutionRaw >= kFixedScale &&
           vibration.contactResolutionRaw <= 16 * kFixedScale &&
           aegis.connectionRadiusRaw > 0 &&
           aegis.connectionRadiusRaw <= 32 * kFixedScale &&
           aegisArchetype.attackRangeRaw > 0 &&
           aegisArchetype.attackRangeRaw <= 64 * kFixedScale &&
           aegisArchetype.attackDamage > 0 &&
           aegisArchetype.attackPeriodTicks > 0;
}

[[nodiscard]] std::uint64_t DistanceSquaredRawFor(Vec2 first, Vec2 second) {
    const std::int64_t deltaX =
        static_cast<std::int64_t>(first.x.Raw()) - second.x.Raw();
    const std::int64_t deltaY =
        static_cast<std::int64_t>(first.y.Raw()) - second.y.Raw();
    const std::uint64_t magnitudeX =
        deltaX < 0 ? std::uint64_t{0} - static_cast<std::uint64_t>(deltaX)
                   : static_cast<std::uint64_t>(deltaX);
    const std::uint64_t magnitudeY =
        deltaY < 0 ? std::uint64_t{0} - static_cast<std::uint64_t>(deltaY)
                   : static_cast<std::uint64_t>(deltaY);
    const std::uint64_t squaredX = magnitudeX * magnitudeX;
    const std::uint64_t squaredY = magnitudeY * magnitudeY;
    return squaredY > std::numeric_limits<std::uint64_t>::max() - squaredX
               ? std::numeric_limits<std::uint64_t>::max()
               : squaredX + squaredY;
}

[[nodiscard]] std::uint64_t StatelessAiValueFor(const PlayerView& view,
                                                EntityId entity,
                                                std::uint64_t salt) {
    std::uint64_t value =
        view.DecisionSeed() ^
        (view.CurrentTick() * 0x9e3779b97f4a7c15ULL) ^
        (static_cast<std::uint64_t>(view.Player().id) << 56U) ^
        (static_cast<std::uint64_t>(entity) << 17U) ^ salt;
    value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31U);
}

[[nodiscard]] bool ViewIsInsideMap(const PlayerView& view,
                                   Vec2 position,
                                   std::int32_t halfExtentRaw = 0) {
    const std::int64_t rawX = position.x.Raw();
    const std::int64_t rawY = position.y.Raw();
    return rawX - halfExtentRaw >= 0 && rawY - halfExtentRaw >= 0 &&
           rawX + halfExtentRaw <
               static_cast<std::int64_t>(view.Config().mapWidthTiles) *
                   kFixedScale &&
           rawY + halfExtentRaw <
               static_cast<std::int64_t>(view.Config().mapHeightTiles) *
                   kFixedScale;
}

[[nodiscard]] const Entity* FindViewEntity(const PlayerView& view,
                                           EntityId id) {
    const auto found = std::find_if(
        view.Entities().begin(), view.Entities().end(),
        [id](const Entity& entity) { return entity.id == id; });
    return found != view.Entities().end() && found->id == id ? &*found : nullptr;
}

[[nodiscard]] PlacementResult ValidateViewPlacement(
    const PlayerView& view,
    EntityType buildingType,
    Vec2 position) {
    if (!IsBuildingType(buildingType)) {
        return PlacementResult::InvalidBuildingType;
    }
    const std::int32_t halfExtent = FootprintHalfExtentFor(
        view.Config().rules, view.Player().faction, buildingType);
    if (!ViewIsInsideMap(view, position, halfExtent)) {
        return PlacementResult::OutsideMap;
    }
    const std::int32_t minimumTileX =
        (position.x.Raw() - halfExtent) / kFixedScale;
    const std::int32_t minimumTileY =
        (position.y.Raw() - halfExtent) / kFixedScale;
    const std::int32_t maximumTileX =
        (position.x.Raw() + halfExtent - 1) / kFixedScale;
    const std::int32_t maximumTileY =
        (position.y.Raw() + halfExtent - 1) / kFixedScale;
    for (std::int32_t tileY = minimumTileY; tileY <= maximumTileY; ++tileY) {
        for (std::int32_t tileX = minimumTileX; tileX <= maximumTileX; ++tileX) {
            const Vec2 tilePosition = Vec2::FromTiles(tileX, tileY);
            if (view.VisibilityAt(tilePosition) != Visibility::Visible ||
                view.TerrainAt(tileX, tileY) != Terrain::Open) {
                return PlacementResult::TerrainRestricted;
            }
        }
    }
    for (const Entity& entity : view.Entities()) {
        const std::int32_t combinedExtent =
            halfExtent + FootprintHalfExtentFor(
                             view.Config().rules, entity.faction, entity.type);
        if (Abs64(static_cast<std::int64_t>(position.x.Raw()) -
                  entity.position.x.Raw()) < combinedExtent &&
            Abs64(static_cast<std::int64_t>(position.y.Raw()) -
                  entity.position.y.Raw()) < combinedExtent) {
            return PlacementResult::Occupied;
        }
    }
    return PlacementResult::Valid;
}

[[nodiscard]] ProductionResult ValidateViewProduction(
    const PlayerView& view,
    EntityId producer,
    EntityType unitType) {
    const Entity* building = FindViewEntity(view, producer);
    if (building == nullptr || building->owner != view.Player().id ||
        building->hitPoints <= 0) {
        return ProductionResult::InvalidProducer;
    }
    if (!building->completed) {
        return ProductionResult::ProducerIncomplete;
    }
    if (building->productionRequired > 0) {
        return ProductionResult::ProducerBusy;
    }
    const bool supported =
        (building->type == EntityType::CommandCore &&
         unitType == EntityType::Worker) ||
        (building->type == EntityType::Barracks &&
         IsBarracksUnitType(unitType));
    if (!supported) {
        return ProductionResult::UnsupportedUnit;
    }
    if (!ResourceCovers(
            view.Player().resources,
            ProductionCostFor(
                view.Config().rules, view.Player().faction, unitType))) {
        return ProductionResult::InsufficientResources;
    }
    std::int32_t committedPopulation = view.PopulationUsed();
    for (const Entity& entity : view.Entities()) {
        if (entity.owner == view.Player().id && entity.productionRequired > 0) {
            committedPopulation = SaturatingAdd(
                committedPopulation,
                PopulationCostFor(
                    view.Config().rules, entity.faction, entity.productionType));
        }
    }
    if (SaturatingAdd(
            committedPopulation,
            PopulationCostFor(
                view.Config().rules, view.Player().faction, unitType)) >
        view.PopulationCapacity()) {
        return ProductionResult::CapacityReached;
    }
    return ProductionResult::Valid;
}

[[nodiscard]] EntityId FindViewOwnedDropoff(const PlayerView& view,
                                            Vec2 from) {
    EntityId nearest = 0;
    std::uint64_t nearestDistance = std::numeric_limits<std::uint64_t>::max();
    for (const Entity& entity : view.Entities()) {
        if (entity.owner != view.Player().id || !entity.completed ||
            !IsDropoffType(entity.type) ||
            (entity.faction == Faction::KharuunAssemblies &&
             entity.type == EntityType::Dropoff &&
             entity.waystoneMode != WaystoneMode::Rooted)) {
            continue;
        }
        const std::uint64_t distance =
            DistanceSquaredRawFor(from, entity.position);
        if (distance < nearestDistance ||
            (distance == nearestDistance && entity.id < nearest)) {
            nearest = entity.id;
            nearestDistance = distance;
        }
    }
    return nearest;
}

[[nodiscard]] std::int32_t SaturatingAdd(std::int32_t lhs, std::int32_t rhs) {
    const std::int64_t sum = static_cast<std::int64_t>(lhs) + rhs;
    return static_cast<std::int32_t>(std::clamp<std::int64_t>(
        sum, std::numeric_limits<std::int32_t>::min(),
        std::numeric_limits<std::int32_t>::max()));
}

class BinaryWriter final {
public:
    void Reserve(std::size_t byteCount) { bytes_.reserve(byteCount); }
    void U8(std::uint8_t value) { bytes_.push_back(value); }
    void U32(std::uint32_t value) {
        for (std::uint32_t shift = 0; shift < 32; shift += 8) {
            U8(static_cast<std::uint8_t>((value >> shift) & 0xffU));
        }
    }
    void U64(std::uint64_t value) {
        for (std::uint32_t shift = 0; shift < 64; shift += 8) {
            U8(static_cast<std::uint8_t>((value >> shift) & 0xffULL));
        }
    }
    void I32(std::int32_t value) { U32(static_cast<std::uint32_t>(value)); }
    void Bytes(std::span<const std::uint8_t> values) {
        bytes_.insert(bytes_.end(), values.begin(), values.end());
    }
    [[nodiscard]] const std::vector<std::uint8_t>& Data() const { return bytes_; }
    [[nodiscard]] std::vector<std::uint8_t> Take() { return std::move(bytes_); }

private:
    std::vector<std::uint8_t> bytes_{};
};

class HashWriter final {
public:
    void U8(std::uint8_t value) { Mix(0x0100000000000000ULL | value); }
    void U32(std::uint32_t value) { Mix(0x0400000000000000ULL | value); }
    void U64(std::uint64_t value) {
        Mix(0x0800000000000000ULL ^ value);
    }
    void I32(std::int32_t value) { U32(static_cast<std::uint32_t>(value)); }
    void Bytes(std::span<const std::uint8_t> values) {
        Mix(0x4200000000000000ULL ^ values.size());
        std::size_t index = 0;
        while (index < values.size()) {
            std::uint64_t packed = 0;
            const std::size_t count =
                std::min<std::size_t>(8, values.size() - index);
            for (std::size_t byte = 0; byte < count; ++byte) {
                packed |= static_cast<std::uint64_t>(values[index + byte])
                          << (byte * 8U);
            }
            Mix(packed ^ (static_cast<std::uint64_t>(count) << 56U));
            index += count;
        }
    }
    [[nodiscard]] std::uint64_t Value() const { return hash_; }

private:
    void Mix(std::uint64_t value) {
        hash_ ^= value + 0x9e3779b97f4a7c15ULL;
        hash_ *= 0xd6e8feb86659fd93ULL;
        hash_ = (hash_ << 27U) | (hash_ >> 37U);
    }

    std::uint64_t hash_ = 0x243f6a8885a308d3ULL;
};

class BinaryReader final {
public:
    explicit BinaryReader(std::span<const std::uint8_t> bytes) : bytes_(bytes) {}
    [[nodiscard]] bool U8(std::uint8_t& value) {
        if (position_ >= bytes_.size()) {
            return false;
        }
        value = bytes_[position_++];
        return true;
    }
    [[nodiscard]] bool U32(std::uint32_t& value) {
        value = 0;
        for (std::uint32_t shift = 0; shift < 32; shift += 8) {
            std::uint8_t byte = 0;
            if (!U8(byte)) {
                return false;
            }
            value |= static_cast<std::uint32_t>(byte) << shift;
        }
        return true;
    }
    [[nodiscard]] bool U64(std::uint64_t& value) {
        value = 0;
        for (std::uint32_t shift = 0; shift < 64; shift += 8) {
            std::uint8_t byte = 0;
            if (!U8(byte)) {
                return false;
            }
            value |= static_cast<std::uint64_t>(byte) << shift;
        }
        return true;
    }
    [[nodiscard]] bool I32(std::int32_t& value) {
        std::uint32_t encoded = 0;
        if (!U32(encoded)) {
            return false;
        }
        value = static_cast<std::int32_t>(encoded);
        return true;
    }
    [[nodiscard]] bool Bytes(std::span<std::uint8_t> destination) {
        if (destination.size() > bytes_.size() - position_) {
            return false;
        }
        std::copy_n(bytes_.begin() + static_cast<std::ptrdiff_t>(position_),
                    destination.size(), destination.begin());
        position_ += destination.size();
        return true;
    }
    [[nodiscard]] bool AtEnd() const { return position_ == bytes_.size(); }
    [[nodiscard]] std::size_t Remaining() const { return bytes_.size() - position_; }

private:
    std::span<const std::uint8_t> bytes_{};
    std::size_t position_ = 0;
};

template <typename Writer>
void WriteCommand(Writer& writer, const Command& command) {
    writer.U64(command.executeTick);
    writer.U8(command.player);
    writer.U64(command.sequence);
    writer.U8(static_cast<std::uint8_t>(command.type));
    writer.U32(command.actor);
    writer.U32(command.target);
    writer.I32(command.position.x.Raw());
    writer.I32(command.position.y.Raw());
    writer.U8(static_cast<std::uint8_t>(command.buildType));
    writer.U8(static_cast<std::uint8_t>(command.wellChoice));
    writer.U8(static_cast<std::uint8_t>(command.warformAdaptation));
    writer.U8(static_cast<std::uint8_t>(command.researchType));
}

[[nodiscard]] bool ReadCommand(BinaryReader& reader, Command& command) {
    std::uint8_t type = 0;
    std::uint8_t buildType = 0;
    std::uint8_t wellChoice = 0;
    std::uint8_t warformAdaptation = 0;
    std::uint8_t researchType = 0;
    std::int32_t rawX = 0;
    std::int32_t rawY = 0;
    if (!reader.U64(command.executeTick) || !reader.U8(command.player) ||
        !reader.U64(command.sequence) || !reader.U8(type) ||
        !reader.U32(command.actor) || !reader.U32(command.target) ||
        !reader.I32(rawX) || !reader.I32(rawY) || !reader.U8(buildType) ||
        !reader.U8(wellChoice) || !reader.U8(warformAdaptation) ||
        !reader.U8(researchType)) {
        return false;
    }
    if (type > static_cast<std::uint8_t>(CommandType::ReconcileToPossible) ||
        buildType > static_cast<std::uint8_t>(EntityType::UtilityStructure) ||
        wellChoice > static_cast<std::uint8_t>(FutureWellChoice::Reshape) ||
        warformAdaptation >
            static_cast<std::uint8_t>(WarformAdaptation::Striker) ||
        researchType >
            static_cast<std::uint8_t>(ResearchType::ChoirSharedResolution)) {
        return false;
    }
    command.type = static_cast<CommandType>(type);
    command.position = Vec2::FromRaw(rawX, rawY);
    command.buildType = static_cast<EntityType>(buildType);
    command.wellChoice = static_cast<FutureWellChoice>(wellChoice);
    command.warformAdaptation =
        static_cast<WarformAdaptation>(warformAdaptation);
    command.researchType = static_cast<ResearchType>(researchType);
    return true;
}

[[nodiscard]] bool ResourceCovers(const ResourcePool& available,
                                  const ResourcePool& cost) {
    return available.material >= cost.material &&
           available.dawnshards >= cost.dawnshards;
}

}  // namespace

SimulationRules DefaultSimulationRules() {
    SimulationRules rules{};
    const auto set = [&rules](Faction faction,
                              EntityType type,
                              EntityArchetypeRules value) {
        rules.archetypes[static_cast<std::size_t>(faction)]
                        [static_cast<std::size_t>(type)] = value;
    };

    set(Faction::MeridianCompact, EntityType::Worker,
        {{50, 0}, 80, 128, 5, kFixedScale, 4, 20, 10, 100, 0, 1, 0,
         60, kFixedScale / 8});
    set(Faction::MeridianCompact, EntityType::Soldier,
        {{85, 20}, 120, 112, 6, 4 * kFixedScale, 18, 12, 0, 0, 0, 2,
         0, 100, kFixedScale / 8});
    set(Faction::MeridianCompact, EntityType::CommandCore,
        {{420, 40}, 1000, 0, 8, 0, 0, 0, 0, 0, 400, 0, 12, 0,
         kFixedScale});
    set(Faction::MeridianCompact, EntityType::Dropoff,
        {{110, 0}, 500, 0, 5, 0, 0, 0, 0, 0, 100, 0, 6, 0,
         3 * kFixedScale / 4});
    set(Faction::MeridianCompact, EntityType::Barracks,
        {{170, 20}, 650, 0, 5, 0, 0, 0, 0, 0, 160, 0, 0, 0,
         kFixedScale});
    set(Faction::MeridianCompact, EntityType::HeavyUnit,
        {{130, 25}, 260, 117, 9, 3 * kFixedScale, 10, 24, 0, 0, 0, 3,
         0, 140, kFixedScale / 8});
    set(Faction::MeridianCompact, EntityType::ScoutUnit,
        {{70, 20}, 75, 256, 15, 4 * kFixedScale, 6, 24, 0, 0, 0, 1,
         0, 80, kFixedScale / 8});
    set(Faction::MeridianCompact, EntityType::UtilityStructure,
        {{130, 30}, 520, 0, 7, 9 * kFixedScale, 28, 20, 0, 0, 120, 0, 0, 0,
         kFixedScale});

    set(Faction::KharuunAssemblies, EntityType::Worker,
        {{50, 0}, 70, 160, 6, kFixedScale, 5, 16, 9, 90, 0, 1, 0,
         60, kFixedScale / 8});
    set(Faction::KharuunAssemblies, EntityType::Soldier,
        {{75, 30}, 105, 176, 7, Fixed::FromRatio(3, 2).Raw(), 25, 10,
         0, 0, 0, 2, 0, 100, kFixedScale / 8});
    set(Faction::KharuunAssemblies, EntityType::CommandCore,
        {{380, 60}, 850, 0, 8, 0, 0, 0, 0, 0, 400, 0, 12, 0,
         kFixedScale});
    set(Faction::KharuunAssemblies, EntityType::Dropoff,
        {{95, 0}, 420, Fixed::FromRatio(3, 50).Raw(), 5, 0, 0, 0, 0, 0, 100, 0, 5, 0,
         3 * kFixedScale / 4});
    set(Faction::KharuunAssemblies, EntityType::Barracks,
        {{150, 30}, 540, 0, 5, 0, 0, 0, 0, 0, 160, 0, 0, 0,
         kFixedScale});
    set(Faction::KharuunAssemblies, EntityType::HeavyUnit,
        {{120, 30}, 245, 138, 8, 2 * kFixedScale, 16, 28, 0, 0, 0, 3,
         0, 140, kFixedScale / 8});
    set(Faction::KharuunAssemblies, EntityType::ScoutUnit,
        {{80, 25}, 85, 240, 16, 3891, 8, 20, 0, 0, 0, 1,
         0, 80, kFixedScale / 8});
    set(Faction::KharuunAssemblies, EntityType::UtilityStructure,
        {{115, 25}, 440, 0, 9, 0, 0, 0, 0, 0, 120, 0, 0, 0,
         kFixedScale});

    set(Faction::HollowChoir, EntityType::Worker,
        {{55, 5}, 80, 194, 10, 0, 0, 0, 9, 12, 0, 1, 0,
         65, kFixedScale / 8});
    set(Faction::HollowChoir, EntityType::Soldier,
        {{80, 35}, 115, 179, 12, 5632, 16, 25, 0, 0, 0, 2, 0,
         100, kFixedScale / 8});
    set(Faction::HollowChoir, EntityType::CommandCore,
        {{0, 0}, 1250, 0, 9, 0, 0, 0, 0, 0, 400, 0, 12, 0,
         5 * kFixedScale / 2});
    set(Faction::HollowChoir, EntityType::Dropoff,
        {{85, 25}, 400, 0, 6, 0, 0, 0, 0, 0, 110, 0, 6, 0,
         kFixedScale});
    set(Faction::HollowChoir, EntityType::Barracks,
        {{175, 40}, 680, 0, 6, 0, 0, 0, 0, 0, 170, 0, 0, 0,
         2 * kFixedScale});
    set(Faction::HollowChoir, EntityType::HeavyUnit,
        {{140, 45}, 230, 133, 9, 4096, 15, 30, 0, 0, 0, 3, 0,
         150, kFixedScale / 8});
    set(Faction::HollowChoir, EntityType::ScoutUnit,
        {{75, 35}, 70, 266, 16, 4300, 7, 22, 0, 0, 0, 1, 0,
         85, kFixedScale / 8});
    set(Faction::HollowChoir, EntityType::UtilityStructure,
        {{120, 35}, 480, 0, 8, 0, 0, 0, 0, 0, 130, 0, 0, 0,
         kFixedScale});
    rules.research[static_cast<std::size_t>(
        ResearchType::MeridianPrismaticTargeting)] = {
            Faction::MeridianCompact, {120, 40}, 180, ResearchType::None,
            115, 100};
    rules.research[static_cast<std::size_t>(
        ResearchType::MeridianHorizonLattice)] = {
            Faction::MeridianCompact, {90, 55}, 220,
            ResearchType::MeridianPrismaticTargeting, 100, 120};
    rules.research[static_cast<std::size_t>(
        ResearchType::KharuunEchoCartography)] = {
            Faction::KharuunAssemblies, {100, 45}, 180, ResearchType::None,
            100, 120};
    rules.research[static_cast<std::size_t>(
        ResearchType::KharuunAncestralEdge)] = {
            Faction::KharuunAssemblies, {110, 50}, 220,
            ResearchType::KharuunEchoCartography, 115, 100};
    rules.research[static_cast<std::size_t>(
        ResearchType::ChoirHeldAlternatives)] = {
            Faction::HollowChoir, {105, 50}, 190, ResearchType::None,
            110, 110};
    rules.research[static_cast<std::size_t>(
        ResearchType::ChoirSharedResolution)] = {
            Faction::HollowChoir, {115, 60}, 230,
            ResearchType::ChoirHeldAlternatives, 100, 120};
    return rules;
}

std::uint32_t Simulation::DeterministicRng::NextU32() {
    state += 0x9e3779b97f4a7c15ULL;
    std::uint64_t value = state;
    value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;
    value ^= value >> 31U;
    return static_cast<std::uint32_t>(value >> 32U);
}

std::uint32_t Simulation::DeterministicRng::Uniform(
    std::uint32_t exclusiveUpperBound) {
    if (exclusiveUpperBound == 0) {
        return 0;
    }
    const std::uint32_t threshold =
        static_cast<std::uint32_t>(-exclusiveUpperBound) % exclusiveUpperBound;
    for (;;) {
        const std::uint32_t value = NextU32();
        if (value >= threshold) {
            return value % exclusiveUpperBound;
        }
    }
}

Simulation::Simulation(SimulationConfig config)
    : config_(config), rng_(config.randomSeed) {
    const std::int64_t tileCount =
        static_cast<std::int64_t>(config_.mapWidthTiles) * config_.mapHeightTiles;
    if (config_.mapWidthTiles <= 0 || config_.mapHeightTiles <= 0 ||
        config_.mapWidthTiles > kMaximumMapDimension ||
        config_.mapHeightTiles > kMaximumMapDimension ||
        config_.ticksPerSecond == 0 ||
        config_.ticksPerSecond > kMaximumTicksPerSecond || tileCount <= 0 ||
        tileCount > kMaximumMapTiles ||
        (config_.protectedCommandCorePlayerMask &
         static_cast<std::uint8_t>(~kValidCommandCoreProtectionMask)) != 0 ||
        !IsValidSimulationRules(config_.rules)) {
        throw std::invalid_argument("invalid deterministic simulation configuration");
    }
    terrain_.assign(static_cast<std::size_t>(tileCount), Terrain::Open);
    for (PlayerId player = 0; player < players_.size(); ++player) {
        players_[player].id = player;
        explored_[player].assign(static_cast<std::size_t>(tileCount), 0);
        visible_[player].assign(static_cast<std::size_t>(tileCount), 0);
    }
}

bool Simulation::AddPlayer(PlayerId player,
                           Faction faction,
                           ResourcePool startingResources) {
    if (player >= players_.size() || players_[player].active ||
        !IsValidFaction(faction) ||
        (faction == Faction::HollowChoir && config_.rules.version < 2) ||
        startingResources.material < 0 || startingResources.dawnshards < 0) {
        return false;
    }
    players_[player] = PlayerState{player, faction, startingResources, true};
    UpdateVisibility();
    return true;
}

const PlayerState* Simulation::FindPlayer(PlayerId player) const {
    return player < players_.size() && players_[player].active ? &players_[player]
                                                               : nullptr;
}

std::optional<std::uint64_t> Simulation::NextCommandSequence(
    PlayerId player) const {
    if (player >= players_.size() || !players_[player].active) {
        return std::nullopt;
    }
    std::uint64_t maximumSequence =
        hasExecutedSequence_[player] ? lastExecutedSequence_[player] : 0;
    for (const Command& command : pendingCommands_) {
        if (command.player == player) {
            maximumSequence = std::max(maximumSequence, command.sequence);
        }
    }
    if (maximumSequence == std::numeric_limits<std::uint64_t>::max()) {
        return std::nullopt;
    }
    return maximumSequence + 1;
}

std::optional<CommandResolutionReceipt>
Simulation::FindCommandResolutionReceipt(PlayerId player,
                                         std::uint64_t sequence) const {
    const auto found = std::find_if(
        commandResolutionReceipts_.begin(),
        commandResolutionReceipts_.end(),
        [player, sequence](const StoredCommandResolutionReceipt& stored) {
            return stored.receipt.player == player &&
                   stored.sequence == sequence;
        });
    return found == commandResolutionReceipts_.end()
               ? std::nullopt
               : std::optional<CommandResolutionReceipt>{found->receipt};
}

PlayerState* Simulation::MutablePlayer(PlayerId player) {
    return player < players_.size() && players_[player].active ? &players_[player]
                                                               : nullptr;
}

Entity Simulation::MakeEntity(PlayerId owner,
                              Faction faction,
                              EntityType type,
                              Vec2 position) const {
    Entity entity{};
    entity.owner = owner;
    entity.faction = faction;
    entity.type = type;
    entity.position = position;
    if (IsConfigurableEntityType(type)) {
        const EntityArchetypeRules& archetype =
            ArchetypeFor(config_.rules, faction, type);
        entity.maxHitPoints = archetype.maxHitPoints;
        entity.movementPerTickRaw = archetype.movementPerTickRaw;
        entity.visionTiles = archetype.visionTiles;
        entity.attackRangeRaw = archetype.attackRangeRaw;
        entity.attackDamage = archetype.attackDamage;
        entity.attackPeriodTicks = archetype.attackPeriodTicks;
        entity.workRate = archetype.workRate;
        entity.cargoCapacity = archetype.cargoCapacity;
        entity.constructionRequired = archetype.constructionRequired;
        entity.hitPoints = entity.maxHitPoints;
        if (faction == Faction::KharuunAssemblies &&
            type == EntityType::Dropoff) {
            entity.waystoneMode = WaystoneMode::Rooted;
        }
        const PlayerState* player = FindPlayer(owner);
        if (player != nullptr && IsBarracksUnitType(type)) {
            for (std::size_t index = 1; index < config_.rules.research.size();
                 ++index) {
                const ResearchType research = static_cast<ResearchType>(index);
                if (player->HasCompletedResearch(research)) {
                    ApplyResearchRule(entity, config_.rules.research[index]);
                }
            }
        }
        if (faction == Faction::HollowChoir && IsBarracksUnitType(type)) {
            entity.choirIdentityState = ChoirIdentityState::Manifest;
            RefreshChoirIdentityStats(entity);
        }
        return entity;
    }
    switch (type) {
        case EntityType::Worker:
        case EntityType::Soldier:
        case EntityType::CommandCore:
        case EntityType::Dropoff:
        case EntityType::Barracks:
        case EntityType::HeavyUnit:
        case EntityType::ScoutUnit:
        case EntityType::UtilityStructure:
            break;
        case EntityType::ResourceNode:
            entity.maxHitPoints = 1;
            break;
        case EntityType::FutureWell:
            entity.maxHitPoints = 100000;
            break;
    }
    entity.hitPoints = entity.maxHitPoints;
    return entity;
}

void Simulation::ApplyResearchRule(Entity& entity,
                                   const ResearchRules& rules) const {
    if (entity.faction != rules.faction || !IsBarracksUnitType(entity.type)) {
        return;
    }
    const auto ApplyPercent = [](std::int32_t value, std::int32_t percent) {
        const std::int64_t scaled =
            static_cast<std::int64_t>(value) * percent / 100;
        return static_cast<std::int32_t>(std::clamp<std::int64_t>(
            scaled, 0, std::numeric_limits<std::int32_t>::max()));
    };
    entity.attackDamage = ApplyPercent(
        entity.attackDamage, rules.combatDamagePercent);
    entity.visionTiles = std::min(
        kMaximumVisionTiles,
        ApplyPercent(entity.visionTiles, rules.combatVisionPercent));
}

bool Simulation::IsChoirIdentityUnit(const Entity& entity) const {
    return config_.rules.version >= 2 &&
           entity.faction == Faction::HollowChoir &&
           IsBarracksUnitType(entity.type);
}

bool Simulation::IsChoirCoherenceStructure(const Entity& entity) const {
    return config_.rules.version >= 2 && entity.owner != kNeutralPlayer &&
           entity.faction == Faction::HollowChoir &&
           (entity.type == EntityType::Dropoff ||
            entity.type == EntityType::Barracks ||
            entity.type == EntityType::UtilityStructure);
}

void Simulation::RefreshChoirIdentityStats(Entity& entity) const {
    if (!IsChoirIdentityUnit(entity)) {
        return;
    }
    const EntityArchetypeRules& archetype =
        ArchetypeFor(config_.rules, entity.faction, entity.type);
    entity.movementPerTickRaw = archetype.movementPerTickRaw;
    entity.visionTiles = archetype.visionTiles;
    entity.attackRangeRaw = archetype.attackRangeRaw;
    entity.attackDamage = archetype.attackDamage;
    entity.attackPeriodTicks = archetype.attackPeriodTicks;
    if (const PlayerState* player = FindPlayer(entity.owner); player != nullptr) {
        for (std::size_t index = 1; index < config_.rules.research.size(); ++index) {
            const ResearchType research = static_cast<ResearchType>(index);
            if (player->HasCompletedResearch(research)) {
                ApplyResearchRule(entity, config_.rules.research[index]);
            }
        }
    }
    const auto ApplyPercent = [](std::int32_t value, std::int32_t percent) {
        return static_cast<std::int32_t>(std::clamp<std::int64_t>(
            static_cast<std::int64_t>(value) * percent / 100,
            0,
            std::numeric_limits<std::int32_t>::max()));
    };
    const bool manifest =
        entity.choirIdentityState == ChoirIdentityState::Manifest ||
        entity.choirIdentityState == ChoirIdentityState::DualResolveManifest ||
        entity.choirIdentityState == ChoirIdentityState::DualResolvePossible;
    const bool possible =
        entity.choirIdentityState == ChoirIdentityState::Possible ||
        entity.choirIdentityState == ChoirIdentityState::DualResolveManifest ||
        entity.choirIdentityState == ChoirIdentityState::DualResolvePossible;
    if (manifest) {
        entity.attackDamage = ApplyPercent(
            entity.attackDamage,
            config_.rules.choirIdentity.manifestDamagePercent);
    }
    if (possible) {
        entity.movementPerTickRaw = std::max(
            1,
            ApplyPercent(
                entity.movementPerTickRaw,
                config_.rules.choirIdentity.possibleMovementPercent));
        entity.visionTiles = std::min(
            kMaximumVisionTiles,
            ApplyPercent(
                entity.visionTiles,
                config_.rules.choirIdentity.possibleVisionPercent));
    }
}

EntityId Simulation::SpawnEntity(PlayerId owner,
                                 Faction faction,
                                 EntityType type,
                                 Vec2 position) {
    if (!IsInsideMap(position) || owner == kNeutralPlayer ||
        FindPlayer(owner) == nullptr || players_[owner].faction != faction ||
        !IsValidFaction(faction) || !IsValidEntityType(type) ||
        type == EntityType::ResourceNode || type == EntityType::FutureWell) {
        return 0;
    }
    Entity entity = MakeEntity(owner, faction, type, position);
    if (IsChoirCoherenceStructure(entity)) {
        entity.choirCoherenceNextChargeTick = std::min(
            kMaximumSupportedTick,
            currentTick_ + config_.rules.choirCoherence.upkeepIntervalTicks);
    }
    if (!TryAllocateEntityId(entity.id)) {
        return 0;
    }
    entities_.push_back(entity);
    ResolveAegisPower();
    UpdateVisibility();
    return entity.id;
}

EntityId Simulation::SpawnPublicInterface(Faction faction, Vec2 position) {
    if (!IsInsideMap(position) || !IsValidFaction(faction)) {
        return 0;
    }
    Entity entity = MakeEntity(kNeutralPlayer, faction,
                               EntityType::UtilityStructure, position);
    // Public interfaces expose durable mission state but confer no command,
    // vision, or autonomous combat authority to either player.
    entity.attackDamage = 0;
    entity.attackRangeRaw = 0;
    entity.attackPeriodTicks = 0;
    entity.attackCooldownTicks = 0;
    entity.visionTiles = 0;
    entity.constructionProgress = 0;
    entity.constructionRequired = 0;
    if (!TryAllocateEntityId(entity.id)) {
        return 0;
    }
    entities_.push_back(entity);
    ResolveAegisPower();
    UpdateVisibility();
    return entity.id;
}

EntityId Simulation::SpawnResourceNode(Vec2 position, std::int32_t amount) {
    if (!IsInsideMap(position) || amount <= 0) {
        return 0;
    }
    Entity entity = MakeEntity(kNeutralPlayer, Faction::MeridianCompact,
                               EntityType::ResourceNode, position);
    if (!TryAllocateEntityId(entity.id)) {
        return 0;
    }
    entity.resourceRemaining = amount;
    entities_.push_back(entity);
    UpdateVisibility();
    return entity.id;
}

EntityId Simulation::SpawnFutureWell(Vec2 position) {
    if (!IsInsideMap(position)) {
        return 0;
    }
    Entity entity = MakeEntity(kNeutralPlayer, Faction::MeridianCompact,
                               EntityType::FutureWell, position);
    if (!TryAllocateEntityId(entity.id)) {
        return 0;
    }
    entities_.push_back(entity);
    UpdateVisibility();
    return entity.id;
}

const Entity* Simulation::FindEntity(EntityId id) const {
    const auto found = std::lower_bound(
        entities_.begin(), entities_.end(), id,
        [](const Entity& entity, EntityId key) { return entity.id < key; });
    return found != entities_.end() && found->id == id ? &*found : nullptr;
}

Entity* Simulation::MutableEntity(EntityId id) {
    const auto found = std::lower_bound(
        entities_.begin(), entities_.end(), id,
        [](const Entity& entity, EntityId key) { return entity.id < key; });
    return found != entities_.end() && found->id == id ? &*found : nullptr;
}

bool Simulation::TryAllocateEntityId(EntityId& id) {
    if (entities_.size() >= kMaximumSerializedEntities || nextEntityId_ == 0 ||
        nextEntityId_ == std::numeric_limits<EntityId>::max()) {
        id = 0;
        return false;
    }
    id = nextEntityId_++;
    return true;
}

bool Simulation::SetTerrainTile(std::int32_t tileX,
                                std::int32_t tileY,
                                Terrain terrain) {
    if (tileX < 0 || tileY < 0 || tileX >= config_.mapWidthTiles ||
        tileY >= config_.mapHeightTiles || !IsValidTerrain(terrain)) {
        return false;
    }
    const std::size_t tile =
        static_cast<std::size_t>(tileY * config_.mapWidthTiles + tileX);
    if (terrain_[tile] != terrain) {
        terrain_[tile] = terrain;
        pathFieldCache_.clear();
    }
    return true;
}

Terrain Simulation::TerrainAt(std::int32_t tileX, std::int32_t tileY) const {
    if (tileX < 0 || tileY < 0 || tileX >= config_.mapWidthTiles ||
        tileY >= config_.mapHeightTiles) {
        return Terrain::Blocked;
    }
    return terrain_[static_cast<std::size_t>(tileY * config_.mapWidthTiles + tileX)];
}

bool Simulation::IsInsideMap(Vec2 position, std::int32_t halfExtentRaw) const {
    const std::int64_t rawX = position.x.Raw();
    const std::int64_t rawY = position.y.Raw();
    return rawX - halfExtentRaw >= 0 && rawY - halfExtentRaw >= 0 &&
           rawX + halfExtentRaw <
               static_cast<std::int64_t>(config_.mapWidthTiles) * kFixedScale &&
           rawY + halfExtentRaw <
               static_cast<std::int64_t>(config_.mapHeightTiles) * kFixedScale;
}

bool Simulation::IsReshapedOpen(std::int32_t tileX, std::int32_t tileY) const {
    for (const Entity& entity : entities_) {
        if (entity.type != EntityType::FutureWell ||
            entity.wellChoice != FutureWellChoice::Reshape ||
            currentTick_ >= entity.reshapeUntilTick) {
            continue;
        }
        const std::int32_t wellX = entity.position.x.FloorToInt();
        const std::int32_t wellY = entity.position.y.FloorToInt();
        if (Abs64(static_cast<std::int64_t>(tileX) - wellX) <= 1 &&
            Abs64(static_cast<std::int64_t>(tileY) - wellY) <= 1) {
            return true;
        }
    }

    return false;
}

bool Simulation::IsPositionPassable(Vec2 position) const {
    if (!IsInsideMap(position)) {
        return false;
    }
    const std::int32_t tileX = position.x.FloorToInt();
    const std::int32_t tileY = position.y.FloorToInt();
    return TerrainAt(tileX, tileY) != Terrain::Blocked ||
           IsReshapedOpen(tileX, tileY);
}

bool Simulation::IsSpawnPositionAvailable(Faction faction,
                                           EntityType type,
                                           Vec2 position) const {
    if (!IsValidFaction(faction) || !IsValidEntityType(type) ||
        type == EntityType::ResourceNode || type == EntityType::FutureWell) {
        return false;
    }
    const std::int32_t halfExtent = FootprintHalfExtentRaw(faction, type);
    if (!IsInsideMap(position, halfExtent)) {
        return false;
    }
    const std::int32_t minimumTileX =
        (position.x.Raw() - halfExtent) / kFixedScale;
    const std::int32_t maximumTileX =
        (position.x.Raw() + halfExtent - 1) / kFixedScale;
    const std::int32_t minimumTileY =
        (position.y.Raw() - halfExtent) / kFixedScale;
    const std::int32_t maximumTileY =
        (position.y.Raw() + halfExtent - 1) / kFixedScale;
    for (std::int32_t tileY = minimumTileY; tileY <= maximumTileY; ++tileY) {
        for (std::int32_t tileX = minimumTileX; tileX <= maximumTileX; ++tileX) {
            if (TerrainAt(tileX, tileY) == Terrain::Blocked &&
                !IsReshapedOpen(tileX, tileY)) {
                return false;
            }
        }
    }
    for (const Entity& entity : entities_) {
        if (entity.hitPoints <= 0) {
            continue;
        }
        const std::int32_t combinedExtent =
            halfExtent + FootprintHalfExtentRaw(entity.faction, entity.type);
        if (Abs64(static_cast<std::int64_t>(position.x.Raw()) -
                  entity.position.x.Raw()) < combinedExtent &&
            Abs64(static_cast<std::int64_t>(position.y.Raw()) -
                  entity.position.y.Raw()) < combinedExtent) {
            return false;
        }
    }
    return true;
}

bool Simulation::IsBuilding(EntityType type) const {
    return IsBuildingType(type);
}

bool Simulation::IsDropoff(EntityType type) const {
    return IsDropoffType(type);
}

std::int32_t Simulation::FootprintHalfExtentRaw(Faction faction,
                                                EntityType type) const {
    return FootprintHalfExtentFor(config_.rules, faction, type);
}

ResourcePool Simulation::BuildCost(Faction faction, EntityType type) const {
    return BuildCostFor(config_.rules, faction, type);
}

ResourcePool Simulation::ProductionCost(Faction faction, EntityType type) const {
    return ProductionCostFor(config_.rules, faction, type);
}

std::int32_t Simulation::ProductionTicks(Faction faction,
                                         EntityType type) const {
    return type == EntityType::Worker || IsBarracksUnitType(type)
               ? ArchetypeFor(config_.rules, faction, type).productionTicks
               : 0;
}

std::int32_t Simulation::PopulationCost(Faction faction,
                                        EntityType type) const {
    return PopulationCostFor(config_.rules, faction, type);
}

std::int32_t Simulation::PopulationUsed(PlayerId player) const {
    if (FindPlayer(player) == nullptr) {
        return 0;
    }
    std::int32_t used = 0;
    for (const Entity& entity : entities_) {
        if (entity.owner == player && entity.hitPoints > 0 && entity.completed) {
            used = SaturatingAdd(
                used,
                PopulationCostFor(config_.rules, entity.faction, entity.type));
        }
    }
    return used;
}

std::int32_t Simulation::PopulationCapacity(PlayerId player) const {
    if (FindPlayer(player) == nullptr) {
        return 0;
    }
    std::int32_t capacity = 0;
    for (const Entity& entity : entities_) {
        if (entity.owner != player || entity.hitPoints <= 0 || !entity.completed) {
            continue;
        }
        if (entity.type == EntityType::CommandCore ||
            (entity.type == EntityType::Dropoff &&
             IsOperationalDropoff(entity))) {
            capacity = SaturatingAdd(
                capacity,
                ArchetypeFor(config_.rules, entity.faction, entity.type)
                    .populationCapacity);
        }
        if (entity.relaySupplyActive &&
            entity.faction == Faction::MeridianCompact &&
            entity.type == EntityType::ScoutUnit &&
            IsRelayConnected(entity)) {
            capacity = SaturatingAdd(
                capacity,
                config_.rules.relaySupply.capacityBonus);
        }
    }
    return capacity;
}

bool Simulation::IsOperationalDropoff(const Entity& entity) const {
    return entity.type == EntityType::CommandCore ||
           (entity.type == EntityType::Dropoff &&
            (entity.faction != Faction::KharuunAssemblies ||
             entity.waystoneMode == WaystoneMode::Rooted));
}

bool Simulation::CanRootWaystone(const Entity& waystone) const {
    if (waystone.faction != Faction::KharuunAssemblies ||
        waystone.type != EntityType::Dropoff || !waystone.completed ||
        waystone.hitPoints <= 0) {
        return false;
    }
    const std::int32_t halfExtent =
        FootprintHalfExtentRaw(waystone.faction, waystone.type);
    if (!IsInsideMap(waystone.position, halfExtent)) {
        return false;
    }
    const std::int32_t minimumTileX =
        (waystone.position.x.Raw() - halfExtent) / kFixedScale;
    const std::int32_t maximumTileX =
        (waystone.position.x.Raw() + halfExtent) / kFixedScale;
    const std::int32_t minimumTileY =
        (waystone.position.y.Raw() - halfExtent) / kFixedScale;
    const std::int32_t maximumTileY =
        (waystone.position.y.Raw() + halfExtent) / kFixedScale;
    for (std::int32_t tileY = minimumTileY; tileY <= maximumTileY; ++tileY) {
        for (std::int32_t tileX = minimumTileX; tileX <= maximumTileX; ++tileX) {
            if (TerrainAt(tileX, tileY) == Terrain::Blocked &&
                !IsReshapedOpen(tileX, tileY)) {
                return false;
            }
        }
    }
    for (const Entity& candidate : entities_) {
        if (candidate.id == waystone.id || candidate.hitPoints <= 0 ||
            !IsBuilding(candidate.type)) {
            continue;
        }
        const std::int32_t combinedExtent =
            halfExtent + FootprintHalfExtentRaw(candidate.faction, candidate.type);
        if (Abs64(static_cast<std::int64_t>(waystone.position.x.Raw()) -
                  candidate.position.x.Raw()) < combinedExtent &&
            Abs64(static_cast<std::int64_t>(waystone.position.y.Raw()) -
                  candidate.position.y.Raw()) < combinedExtent) {
            return false;
        }
    }
    return true;
}

WaystoneRootResult Simulation::ValidateWaystoneRoot(
    PlayerId player,
    EntityId actor) const {
    if (FindPlayer(player) == nullptr) {
        return WaystoneRootResult::InvalidPlayer;
    }
    const Entity* waystone = FindEntity(actor);
    if (waystone == nullptr || waystone->owner != player ||
        !waystone->completed || waystone->hitPoints <= 0 ||
        waystone->faction != Faction::KharuunAssemblies ||
        waystone->type != EntityType::Dropoff ||
        waystone->waystoneMode == WaystoneMode::NotWaystone) {
        return WaystoneRootResult::InvalidActor;
    }
    if (waystone->waystoneMode == WaystoneMode::Uprooting ||
        waystone->waystoneMode == WaystoneMode::Rooting) {
        return WaystoneRootResult::TransitionActive;
    }
    if (waystone->waystoneMode == WaystoneMode::Mobile &&
        !CanRootWaystone(*waystone)) {
        return WaystoneRootResult::RootingBlocked;
    }
    return WaystoneRootResult::Valid;
}

bool Simulation::IsWarform(const Entity& entity) const {
    return entity.faction == Faction::KharuunAssemblies &&
           (entity.type == EntityType::Soldier ||
            entity.type == EntityType::HeavyUnit ||
            entity.type == EntityType::ScoutUnit);
}

WarformAdaptationResult Simulation::ValidateWarformAdaptation(
    PlayerId player,
    EntityId actor,
    EntityId site,
    WarformAdaptation adaptation) const {
    const PlayerState* playerState = FindPlayer(player);
    if (playerState == nullptr) {
        return WarformAdaptationResult::InvalidPlayer;
    }
    const Entity* warform = FindEntity(actor);
    if (warform == nullptr || warform->owner != player || !warform->completed ||
        warform->hitPoints <= 0 || !IsWarform(*warform)) {
        return WarformAdaptationResult::InvalidActor;
    }
    if (adaptation != WarformAdaptation::Carapace &&
        adaptation != WarformAdaptation::Striker) {
        return WarformAdaptationResult::InvalidAdaptation;
    }
    if (warform->pendingWarformAdaptation != WarformAdaptation::None) {
        return WarformAdaptationResult::MoltActive;
    }
    if (warform->warformAdaptation == adaptation) {
        return WarformAdaptationResult::AlreadyAdapted;
    }
    const Entity* basin = FindEntity(site);
    if (basin == nullptr || basin->owner != player || !basin->completed ||
        basin->hitPoints <= 0 ||
        basin->faction != Faction::KharuunAssemblies ||
        basin->type != EntityType::Barracks) {
        return WarformAdaptationResult::InvalidSite;
    }
    const std::int64_t radius = config_.rules.warformAdaptation.siteRadiusRaw;
    if (DistanceSquaredRaw(warform->position, basin->position) >
        static_cast<std::uint64_t>(radius * radius)) {
        return WarformAdaptationResult::OutsideSiteRadius;
    }
    if (playerState->resources.dawnshards <
        config_.rules.warformAdaptation.dawnCost) {
        return WarformAdaptationResult::InsufficientDawn;
    }
    return WarformAdaptationResult::Valid;
}

void Simulation::ApplyWarformAdaptation(
    Entity& entity,
    WarformAdaptation adaptation) {
    if (!IsWarform(entity)) {
        return;
    }
    const Entity researchedBase = MakeEntity(
        entity.owner, entity.faction, entity.type, entity.position);
    const std::int32_t missingHitPoints =
        std::max(0, entity.maxHitPoints - entity.hitPoints);
    entity.maxHitPoints = researchedBase.maxHitPoints;
    entity.movementPerTickRaw = researchedBase.movementPerTickRaw;
    entity.visionTiles = researchedBase.visionTiles;
    entity.attackDamage = researchedBase.attackDamage;
    entity.attackPeriodTicks = researchedBase.attackPeriodTicks;
    if (adaptation == WarformAdaptation::Carapace) {
        entity.maxHitPoints = std::max(
            1,
            static_cast<std::int32_t>(
                static_cast<std::int64_t>(researchedBase.maxHitPoints) *
                config_.rules.warformAdaptation.carapaceHealthPercent / 100));
        entity.movementPerTickRaw = std::max(
            1,
            static_cast<std::int32_t>(
                static_cast<std::int64_t>(researchedBase.movementPerTickRaw) *
                config_.rules.warformAdaptation.carapaceMovementPercent / 100));
    } else if (adaptation == WarformAdaptation::Striker) {
        entity.attackDamage = std::max(
            1,
            static_cast<std::int32_t>(
                static_cast<std::int64_t>(researchedBase.attackDamage) *
                config_.rules.warformAdaptation.strikerDamagePercent / 100));
        entity.attackPeriodTicks = std::max<Tick>(
            1,
            researchedBase.attackPeriodTicks *
                static_cast<Tick>(
                    config_.rules.warformAdaptation.strikerCooldownPercent) /
                100);
    }
    entity.hitPoints = std::max(1, entity.maxHitPoints - missingHitPoints);
    entity.warformAdaptation = adaptation;
}

bool Simulation::IsCairnback(const Entity& entity) const {
    return entity.faction == Faction::KharuunAssemblies &&
           entity.type == EntityType::HeavyUnit &&
           !entity.temporaryMineralCover;
}

std::int32_t Simulation::VibrationDetectionRadiusRaw(
    const Entity& entity) const {
    if (!entity.completed || entity.hitPoints <= 0 ||
        entity.faction != Faction::KharuunAssemblies ||
        entity.temporaryMineralCover) {
        return 0;
    }
    if (entity.type == EntityType::ScoutUnit) {
        return config_.rules.vibrationDetection.resonantRadiusRaw;
    }
    if (entity.type == EntityType::UtilityStructure) {
        return config_.rules.vibrationDetection.listeningSpineRadiusRaw;
    }
    return 0;
}

bool Simulation::IsAegisPost(const Entity& entity) const {
    return entity.faction == Faction::MeridianCompact &&
           entity.type == EntityType::UtilityStructure &&
           !entity.temporaryMineralCover;
}

bool Simulation::IsAegisNetworkPowered(const Entity& aegis) const {
    if (!IsAegisPost(aegis) || !aegis.completed || aegis.hitPoints <= 0) {
        return false;
    }
    if (aegis.owner == kNeutralPlayer) {
        return true;
    }
    const std::int64_t radius = config_.rules.poweredAegis.connectionRadiusRaw;
    const std::uint64_t radiusSquared =
        static_cast<std::uint64_t>(radius * radius);
    std::vector<EntityId> poweredNodes{};
    poweredNodes.reserve(entities_.size());
    for (const Entity& entity : entities_) {
        if (entity.owner == aegis.owner && entity.completed &&
            entity.hitPoints > 0 &&
            entity.faction == Faction::MeridianCompact &&
            entity.type == EntityType::CommandCore) {
            poweredNodes.push_back(entity.id);
        }
    }
    bool added = true;
    while (added) {
        added = false;
        for (const Entity& link : entities_) {
            if (link.owner != aegis.owner || !link.completed ||
                link.hitPoints <= 0 ||
                link.faction != Faction::MeridianCompact ||
                link.type != EntityType::Dropoff ||
                std::find(poweredNodes.begin(), poweredNodes.end(), link.id) !=
                    poweredNodes.end()) {
                continue;
            }
            const bool connected = std::any_of(
                poweredNodes.begin(), poweredNodes.end(),
                [&](EntityId nodeId) {
                    const Entity* node = FindEntity(nodeId);
                    return node != nullptr &&
                           DistanceSquaredRaw(link.position, node->position) <=
                               radiusSquared;
                });
            if (connected) {
                poweredNodes.push_back(link.id);
                added = true;
            }
        }
    }
    return std::any_of(
        poweredNodes.begin(), poweredNodes.end(),
        [&](EntityId nodeId) {
            const Entity* node = FindEntity(nodeId);
            return node != nullptr &&
                   DistanceSquaredRaw(aegis.position, node->position) <=
                       radiusSquared;
        });
}

MineralCoverResult Simulation::ValidateMineralCover(
    PlayerId player,
    EntityId actor,
    Vec2 position) const {
    const PlayerState* playerState = FindPlayer(player);
    if (playerState == nullptr) {
        return MineralCoverResult::InvalidPlayer;
    }
    const Entity* cairnback = FindEntity(actor);
    if (cairnback == nullptr || cairnback->owner != player ||
        !cairnback->completed || cairnback->hitPoints <= 0 ||
        !IsCairnback(*cairnback)) {
        return MineralCoverResult::InvalidActor;
    }
    if (cairnback->pendingWarformAdaptation !=
        WarformAdaptation::None) {
        return MineralCoverResult::MoltActive;
    }
    if (currentTick_ < cairnback->mineralCoverCooldownUntilTick) {
        return MineralCoverResult::CooldownActive;
    }
    const MineralCoverRules& rules = config_.rules.mineralCover;
    if (!IsInsideMap(position, rules.halfExtentRaw)) {
        return MineralCoverResult::InvalidPosition;
    }
    const std::int64_t castRange = rules.castRangeRaw;
    if (DistanceSquaredRaw(cairnback->position, position) >
        static_cast<std::uint64_t>(castRange * castRange)) {
        return MineralCoverResult::OutsideCastRange;
    }
    const std::int32_t tileX = position.x.FloorToInt();
    const std::int32_t tileY = position.y.FloorToInt();
    if (TerrainAt(tileX, tileY) == Terrain::Blocked) {
        return MineralCoverResult::InvalidPosition;
    }
    for (const Entity& entity : entities_) {
        if (entity.hitPoints <= 0) {
            continue;
        }
        const std::int32_t halfExtent = entity.temporaryMineralCover
                                            ? rules.halfExtentRaw
                                            : FootprintHalfExtentRaw(
                                                  entity.faction,
                                                  entity.type);
        const std::int32_t combinedExtent = rules.halfExtentRaw + halfExtent;
        if (Abs64(static_cast<std::int64_t>(position.x.Raw()) -
                  entity.position.x.Raw()) < combinedExtent &&
            Abs64(static_cast<std::int64_t>(position.y.Raw()) -
                  entity.position.y.Raw()) < combinedExtent) {
            return MineralCoverResult::Occupied;
        }
    }
    if (playerState->resources.dawnshards < rules.dawnCost) {
        return MineralCoverResult::InsufficientDawn;
    }
    if (entities_.size() >= kMaximumSerializedEntities || nextEntityId_ == 0 ||
        nextEntityId_ == std::numeric_limits<EntityId>::max()) {
        return MineralCoverResult::EntityCapacityReached;
    }
    return MineralCoverResult::Valid;
}

ChoirReconciliationResult Simulation::ValidateChoirReconciliation(
    PlayerId player,
    EntityId actor,
    ChoirIdentityState stableState) const {
    const PlayerState* playerState = FindPlayer(player);
    if (playerState == nullptr) {
        return ChoirReconciliationResult::InvalidPlayer;
    }
    const Entity* entity = FindEntity(actor);
    if (entity == nullptr || entity->owner != player || !entity->completed ||
        entity->hitPoints <= 0 || !IsChoirIdentityUnit(*entity) ||
        (stableState != ChoirIdentityState::Manifest &&
         stableState != ChoirIdentityState::Possible)) {
        return ChoirReconciliationResult::InvalidActor;
    }
    if (entity->choirIdentityState == ChoirIdentityState::DualResolveManifest ||
        entity->choirIdentityState == ChoirIdentityState::DualResolvePossible) {
        return ChoirReconciliationResult::AlreadyResolving;
    }
    if (entity->choirIdentityState == stableState) {
        return ChoirReconciliationResult::AlreadyStable;
    }
    if (currentTick_ < entity->choirIdentityNextAvailableTick) {
        return ChoirReconciliationResult::CooldownActive;
    }
    if (playerState->resources.dawnshards <
        config_.rules.choirIdentity.dawnCost) {
        return ChoirReconciliationResult::InsufficientDawn;
    }
    return ChoirReconciliationResult::Valid;
}

EntityId Simulation::InterceptingMineralCover(
    const Entity& attacker,
    const Entity& target) const {
    if (target.temporaryMineralCover || attacker.owner == target.owner) {
        return 0;
    }
    const std::int64_t deltaX =
        static_cast<std::int64_t>(target.position.x.Raw()) -
        attacker.position.x.Raw();
    const std::int64_t deltaY =
        static_cast<std::int64_t>(target.position.y.Raw()) -
        attacker.position.y.Raw();
    const std::int64_t lengthSquared = deltaX * deltaX + deltaY * deltaY;
    if (lengthSquared <= 0) {
        return 0;
    }
    EntityId nearest = 0;
    std::int64_t nearestProgress = kFixedScale + 1;
    for (const Entity& cover : entities_) {
        if (!cover.temporaryMineralCover || cover.hitPoints <= 0 ||
            cover.owner != target.owner || cover.id == attacker.id ||
            cover.id == target.id || currentTick_ >= cover.mineralCoverUntilTick) {
            continue;
        }
        const std::int64_t coverX =
            static_cast<std::int64_t>(cover.position.x.Raw()) -
            attacker.position.x.Raw();
        const std::int64_t coverY =
            static_cast<std::int64_t>(cover.position.y.Raw()) -
            attacker.position.y.Raw();
        const std::int64_t dot = coverX * deltaX + coverY * deltaY;
        if (dot <= 0 || dot >= lengthSquared) {
            continue;
        }
        const std::int64_t progress = dot * kFixedScale / lengthSquared;
        const std::int64_t closestX =
            static_cast<std::int64_t>(attacker.position.x.Raw()) +
            deltaX * progress / kFixedScale;
        const std::int64_t closestY =
            static_cast<std::int64_t>(attacker.position.y.Raw()) +
            deltaY * progress / kFixedScale;
        if (Abs64(closestX - cover.position.x.Raw()) >
                config_.rules.mineralCover.halfExtentRaw ||
            Abs64(closestY - cover.position.y.Raw()) >
                config_.rules.mineralCover.halfExtentRaw) {
            continue;
        }
        if (progress < nearestProgress ||
            (progress == nearestProgress && (nearest == 0 || cover.id < nearest))) {
            nearest = cover.id;
            nearestProgress = progress;
        }
    }
    return nearest;
}

bool Simulation::IsRelayConnected(const Entity& relay) const {
    if (relay.owner == kNeutralPlayer || !relay.completed ||
        relay.hitPoints <= 0 ||
        relay.faction != Faction::MeridianCompact ||
        relay.type != EntityType::ScoutUnit) {
        return false;
    }
    const std::uint64_t radiusSquared =
        static_cast<std::uint64_t>(config_.rules.relaySupply.connectionRadiusRaw) *
        config_.rules.relaySupply.connectionRadiusRaw;
    return std::any_of(
        entities_.begin(),
        entities_.end(),
        [&](const Entity& candidate) {
            return candidate.owner == relay.owner && candidate.completed &&
                   candidate.hitPoints > 0 &&
                   (candidate.type == EntityType::CommandCore ||
                    candidate.type == EntityType::Dropoff) &&
                   DistanceSquaredRaw(relay.position, candidate.position) <=
                       radiusSquared;
        });
}

RelaySupplyResult Simulation::ValidateRelaySupply(
    PlayerId player,
    EntityId actor) const {
    if (FindPlayer(player) == nullptr) {
        return RelaySupplyResult::InvalidPlayer;
    }
    const Entity* relay = FindEntity(actor);
    if (relay == nullptr || relay->owner != player || !relay->completed ||
        relay->hitPoints <= 0 ||
        relay->faction != Faction::MeridianCompact ||
        relay->type != EntityType::ScoutUnit) {
        return RelaySupplyResult::InvalidActor;
    }
    if (relay->relaySupplyActive) {
        return RelaySupplyResult::AlreadyActive;
    }
    if (relay->relaySupplyCooldownUntilTick > currentTick_) {
        return RelaySupplyResult::CooldownActive;
    }
    return IsRelayConnected(*relay) ? RelaySupplyResult::Valid
                                    : RelaySupplyResult::Disconnected;
}

ProductionResult Simulation::ValidateProduction(PlayerId player,
                                                EntityId producer,
                                                EntityType unitType) const {
    const PlayerState* playerState = FindPlayer(player);
    if (playerState == nullptr) {
        return ProductionResult::InvalidPlayer;
    }
    const Entity* building = FindEntity(producer);
    if (building == nullptr || building->owner != player ||
        building->hitPoints <= 0) {
        return ProductionResult::InvalidProducer;
    }
    if (!building->completed) {
        return ProductionResult::ProducerIncomplete;
    }
    if (building->productionRequired > 0) {
        return ProductionResult::ProducerBusy;
    }
    if (playerState->activeResearch != ResearchType::None &&
        playerState->researchProducer == producer) {
        return ProductionResult::ProducerBusy;
    }
    const bool supported =
        (building->type == EntityType::CommandCore &&
         unitType == EntityType::Worker) ||
        (building->type == EntityType::Barracks &&
         IsBarracksUnitType(unitType));
    if (!supported) {
        return ProductionResult::UnsupportedUnit;
    }
    const ResourcePool cost = ProductionCost(playerState->faction, unitType);
    if (!ResourceCovers(playerState->resources, cost)) {
        return ProductionResult::InsufficientResources;
    }
    std::int32_t committedPopulation = PopulationUsed(player);
    for (const Entity& entity : entities_) {
        if (entity.owner == player && entity.productionRequired > 0) {
            committedPopulation = SaturatingAdd(
                committedPopulation,
                PopulationCost(entity.faction, entity.productionType));
        }
    }
    if (SaturatingAdd(
            committedPopulation, PopulationCost(playerState->faction, unitType)) >
        PopulationCapacity(player)) {
        return ProductionResult::CapacityReached;
    }
    if (entities_.size() >= kMaximumSerializedEntities || nextEntityId_ == 0 ||
        nextEntityId_ == std::numeric_limits<EntityId>::max()) {
        return ProductionResult::EntityCapacityReached;
    }
    return ProductionResult::Valid;
}

const ResearchRules* Simulation::ResearchDefinition(
    ResearchType researchType) const {
    if (researchType == ResearchType::None ||
        !IsValidResearchType(researchType)) {
        return nullptr;
    }
    return &config_.rules.research[static_cast<std::size_t>(researchType)];
}

ResearchResult Simulation::ValidateResearch(
    PlayerId player,
    EntityId producer,
    ResearchType researchType) const {
    const PlayerState* playerState = FindPlayer(player);
    if (playerState == nullptr) {
        return ResearchResult::InvalidPlayer;
    }
    const Entity* building = FindEntity(producer);
    if (building == nullptr || building->owner != player ||
        building->hitPoints <= 0 || building->type != EntityType::Barracks) {
        return ResearchResult::InvalidProducer;
    }
    if (!building->completed) {
        return ResearchResult::ProducerIncomplete;
    }
    if (building->productionRequired > 0 ||
        playerState->activeResearch != ResearchType::None) {
        return ResearchResult::ProducerBusy;
    }
    const ResearchRules* rules = ResearchDefinition(researchType);
    if (rules == nullptr || rules->researchTicks == 0) {
        return ResearchResult::InvalidTechnology;
    }
    if (rules->faction != playerState->faction) {
        return ResearchResult::WrongFaction;
    }
    if (playerState->HasCompletedResearch(researchType)) {
        return ResearchResult::AlreadyCompleted;
    }
    if (rules->prerequisite != ResearchType::None &&
        !playerState->HasCompletedResearch(rules->prerequisite)) {
        return ResearchResult::PrerequisiteMissing;
    }
    if (!ResourceCovers(playerState->resources, rules->cost)) {
        return ResearchResult::InsufficientResources;
    }
    return ResearchResult::Valid;
}

MatchOutcome Simulation::Outcome() const {
    const std::size_t activePlayerCount = std::count_if(
        players_.begin(), players_.end(),
        [](const PlayerState& player) { return player.active; });
    if (activePlayerCount < 2) {
        return MatchOutcome::Ongoing;
    }
    std::array<bool, kMaximumPlayers> hasCommandCore{};
    for (const Entity& entity : entities_) {
        if (entity.owner < hasCommandCore.size() &&
            players_[entity.owner].active && entity.hitPoints > 0 &&
            entity.type == EntityType::CommandCore) {
            hasCommandCore[entity.owner] = true;
        }
    }
    std::size_t survivingPlayerCount = 0;
    PlayerId survivor = kNeutralPlayer;
    for (PlayerId player = 0; player < players_.size(); ++player) {
        if (players_[player].active && hasCommandCore[player]) {
            ++survivingPlayerCount;
            survivor = player;
        }
    }
    if (survivingPlayerCount > 1) {
        return MatchOutcome::Ongoing;
    }
    if (survivingPlayerCount == 1) {
        constexpr std::array<MatchOutcome, kMaximumPlayers> outcomes{
            MatchOutcome::Player0Victory,
            MatchOutcome::Player1Victory,
            MatchOutcome::Player2Victory,
            MatchOutcome::Player3Victory,
        };
        return outcomes[survivor];
    }
    return MatchOutcome::Draw;
}

bool Simulation::ForfeitPlayer(PlayerId player) {
    if (player >= players_.size() || !players_[player].active ||
        Outcome() != MatchOutcome::Ongoing) {
        return false;
    }
    bool retiredCommandCore = false;
    for (Entity& entity : entities_) {
        if (entity.owner == player && entity.type == EntityType::CommandCore &&
            entity.hitPoints > 0) {
            entity.hitPoints = 0;
            entity.order = {};
            retiredCommandCore = true;
        }
    }
    if (!retiredCommandCore) {
        return false;
    }
    pendingCommands_.erase(
        std::remove_if(
            pendingCommands_.begin(), pendingCommands_.end(),
            [player](const Command& command) { return command.player == player; }),
        pendingCommands_.end());
    // Match the normal end-of-tick destruction contract immediately. A
    // forfeit pauses the authority before another tick, so retaining a
    // zero-health Core here would create a state the snapshot validator
    // correctly refuses to load.
    RemoveDestroyedEntities();
    ClearInvalidOrders();
    DisableReplayExport();
    return true;
}

PlacementResult Simulation::ValidatePlacement(PlayerId player,
                                               EntityType buildingType,
                                               Vec2 position,
                                               EntityId* blockingEntity) const {
    if (blockingEntity != nullptr) {
        *blockingEntity = 0;
    }
    if (FindPlayer(player) == nullptr) {
        return PlacementResult::InvalidPlayer;
    }
    if (!IsBuilding(buildingType)) {
        return PlacementResult::InvalidBuildingType;
    }
    const Faction faction = players_[player].faction;
    const std::int32_t halfExtent =
        FootprintHalfExtentRaw(faction, buildingType);
    if (!IsInsideMap(position, halfExtent)) {
        return PlacementResult::OutsideMap;
    }
    const std::int32_t minimumTileX = (position.x.Raw() - halfExtent) / kFixedScale;
    const std::int32_t minimumTileY = (position.y.Raw() - halfExtent) / kFixedScale;
    const std::int32_t maximumTileX =
        (position.x.Raw() + halfExtent - 1) / kFixedScale;
    const std::int32_t maximumTileY =
        (position.y.Raw() + halfExtent - 1) / kFixedScale;
    for (std::int32_t tileY = minimumTileY; tileY <= maximumTileY; ++tileY) {
        for (std::int32_t tileX = minimumTileX; tileX <= maximumTileX; ++tileX) {
            if (TerrainAt(tileX, tileY) != Terrain::Open) {
                return PlacementResult::TerrainRestricted;
            }
        }
    }
    for (const Entity& entity : entities_) {
        const std::int32_t combinedExtent =
            halfExtent + FootprintHalfExtentRaw(entity.faction, entity.type);
        if (Abs64(static_cast<std::int64_t>(position.x.Raw()) -
                  entity.position.x.Raw()) < combinedExtent &&
            Abs64(static_cast<std::int64_t>(position.y.Raw()) -
                  entity.position.y.Raw()) < combinedExtent) {
            if (blockingEntity != nullptr) {
                *blockingEntity = entity.id;
            }
            return PlacementResult::Occupied;
        }
    }
    return PlacementResult::Valid;
}

bool Simulation::QueueCommand(const Command& command, std::string* rejectionReason) {
    if (rejectionReason != nullptr) {
        rejectionReason->clear();
    }
    if (FindPlayer(command.player) == nullptr) {
        SetError(rejectionReason, "command player is not active");
        return false;
    }
    if (command.executeTick < currentTick_ ||
        command.executeTick > kMaximumSupportedTick) {
        SetError(rejectionReason, "command tick is outside the supported range");
        return false;
    }
    if (!IsValidCommandType(command.type) ||
        !IsValidEntityType(command.buildType) ||
        !IsValidWellChoice(command.wellChoice) ||
        !IsValidWarformAdaptation(command.warformAdaptation) ||
        !IsValidResearchType(command.researchType) ||
        command.actor == 0) {
        SetError(rejectionReason, "command encoding is invalid");
        return false;
    }
    if (hasExecutedSequence_[command.player] &&
        command.sequence <= lastExecutedSequence_[command.player]) {
        SetError(rejectionReason, "command sequence is not newer than executed input");
        return false;
    }
    for (const Command& prior : pendingCommands_) {
        if (prior.player != command.player) {
            continue;
        }
        if (HasSameCommandKey(prior, command)) {
            SetError(rejectionReason, "player and sequence must identify one command");
            return false;
        }
        if ((prior.executeTick < command.executeTick &&
             prior.sequence >= command.sequence) ||
            (prior.executeTick > command.executeTick &&
             prior.sequence <= command.sequence)) {
            SetError(rejectionReason,
                     "command sequence must increase across execution ticks");
            return false;
        }
    }
    if (pendingCommands_.size() >= kMaximumSerializedCommands ||
        commandLog_.size() >= kMaximumSerializedCommands) {
        SetError(rejectionReason, "command capacity is exhausted");
        return false;
    }
    if (replayInitialSnapshot_.empty()) {
        replayInitialSnapshot_ = SaveSnapshot();
    }
    pendingCommands_.push_back(command);
    commandLog_.push_back(command);
    return true;
}

std::uint64_t Simulation::DistanceSquaredRaw(Vec2 first, Vec2 second) const {
    return DistanceSquaredRawFor(first, second);
}

bool Simulation::InInteractionRange(const Entity& first,
                                    const Entity& second,
                                    std::int32_t extraRangeRaw) const {
    const std::int64_t range = static_cast<std::int64_t>(extraRangeRaw) +
                               FootprintHalfExtentRaw(first.faction, first.type) +
                               FootprintHalfExtentRaw(second.faction, second.type);
    return DistanceSquaredRaw(first.position, second.position) <=
           static_cast<std::uint64_t>(range * range);
}

std::optional<Vec2> Simulation::FindNextPathWaypoint(
    Vec2 from,
    Vec2 destination) const {
    const std::int32_t startX = from.x.FloorToInt();
    const std::int32_t startY = from.y.FloorToInt();
    const std::int32_t goalX = destination.x.FloorToInt();
    const std::int32_t goalY = destination.y.FloorToInt();
    if (startX == goalX && startY == goalY) {
        return destination;
    }
    const Vec2 goalTile = Vec2::FromTiles(goalX, goalY);
    if (!IsPositionPassable(goalTile)) {
        return std::nullopt;
    }

    const std::size_t width =
        static_cast<std::size_t>(config_.mapWidthTiles);
    const std::size_t tileCount =
        width * static_cast<std::size_t>(config_.mapHeightTiles);
    const auto tileIndex = [width](std::int32_t tileX,
                                   std::int32_t tileY) {
        return static_cast<std::size_t>(tileY) * width +
               static_cast<std::size_t>(tileX);
    };
    const std::size_t start = tileIndex(startX, startY);
    const std::size_t goal = tileIndex(goalX, goalY);
    constexpr std::size_t kUnvisited = std::numeric_limits<std::size_t>::max();

    auto cached = pathFieldCache_.find(goal);
    if (cached == pathFieldCache_.end()) {
        PathFieldCacheEntry field{};
        field.distanceToGoal.assign(tileCount, kUnvisited);
        field.lastUsedTick = currentTick_;
        std::vector<std::uint8_t> passable(tileCount, 0);
        for (std::size_t tile = 0; tile < tileCount; ++tile) {
            passable[tile] = terrain_[tile] != Terrain::Blocked ? 1 : 0;
        }
        for (const Entity& entity : entities_) {
            if (entity.type != EntityType::FutureWell ||
                entity.wellChoice != FutureWellChoice::Reshape ||
                currentTick_ >= entity.reshapeUntilTick) {
                continue;
            }
            const std::int32_t wellX = entity.position.x.FloorToInt();
            const std::int32_t wellY = entity.position.y.FloorToInt();
            for (std::int32_t offsetY = -1; offsetY <= 1; ++offsetY) {
                for (std::int32_t offsetX = -1; offsetX <= 1; ++offsetX) {
                    const std::int32_t tileX = wellX + offsetX;
                    const std::int32_t tileY = wellY + offsetY;
                    if (tileX >= 0 && tileY >= 0 &&
                        tileX < config_.mapWidthTiles &&
                        tileY < config_.mapHeightTiles) {
                        passable[tileIndex(tileX, tileY)] = 1;
                    }
                }
            }
        }
        std::vector<std::size_t> frontier{};
        frontier.reserve(std::min<std::size_t>(tileCount, 4096));
        field.distanceToGoal[goal] = 0;
        frontier.push_back(goal);

        // Reverse expansion produces the shortest distance for every reachable
        // tile sharing this destination. Waypoint selection below retains the
        // original forward-search N/E/S/W equal-cost preference.
        constexpr std::array<std::array<std::int32_t, 2>, 4> directions{{
            {{0, -1}},
            {{1, 0}},
            {{0, 1}},
            {{-1, 0}},
        }};
        std::size_t head = 0;
        while (head < frontier.size()) {
            const std::size_t current = frontier[head++];
            const std::int32_t currentX =
                static_cast<std::int32_t>(current % width);
            const std::int32_t currentY =
                static_cast<std::int32_t>(current / width);
            for (const auto& direction : directions) {
                const std::int32_t nextX = currentX + direction[0];
                const std::int32_t nextY = currentY + direction[1];
                if (nextX < 0 || nextY < 0 ||
                    nextX >= config_.mapWidthTiles ||
                    nextY >= config_.mapHeightTiles) {
                    continue;
                }
                const std::size_t next = tileIndex(nextX, nextY);
                if (field.distanceToGoal[next] != kUnvisited ||
                    passable[next] == 0) {
                    continue;
                }
                field.distanceToGoal[next] =
                    field.distanceToGoal[current] + 1;
                frontier.push_back(next);
            }
        }

        if (pathFieldCache_.size() >= kMaximumCachedPathFields) {
            const auto oldest = std::min_element(
                pathFieldCache_.begin(), pathFieldCache_.end(),
                [](const auto& lhs, const auto& rhs) {
                    return std::tie(lhs.second.lastUsedTick, lhs.first) <
                           std::tie(rhs.second.lastUsedTick, rhs.first);
                });
            pathFieldCache_.erase(oldest);
        }
        cached = pathFieldCache_.emplace(goal, std::move(field)).first;
    } else {
        cached->second.lastUsedTick = currentTick_;
    }
    const std::size_t startDistance =
        cached->second.distanceToGoal[start];
    if (startDistance == kUnvisited) {
        return std::nullopt;
    }

    constexpr std::array<std::array<std::int32_t, 2>, 4> directions{{
        {{0, -1}},
        {{1, 0}},
        {{0, 1}},
        {{-1, 0}},
    }};
    for (const auto& direction : directions) {
        const std::int32_t nextX = startX + direction[0];
        const std::int32_t nextY = startY + direction[1];
        if (nextX < 0 || nextY < 0 ||
            nextX >= config_.mapWidthTiles ||
            nextY >= config_.mapHeightTiles) {
            continue;
        }
        const std::size_t next = tileIndex(nextX, nextY);
        if (cached->second.distanceToGoal[next] != kUnvisited &&
            cached->second.distanceToGoal[next] + 1 == startDistance) {
            return Vec2::FromTiles(nextX, nextY);
        }
    }
    return std::nullopt;
}

bool Simulation::MoveTowards(Entity& entity, Vec2 destination) {
    if (entity.movementPerTickRaw <= 0) {
        return false;
    }
    if (entity.position == destination) {
        return true;
    }
    const std::optional<Vec2> waypoint =
        FindNextPathWaypoint(entity.position, destination);
    if (!waypoint.has_value()) {
        return false;
    }
    const Vec2 movementTarget = *waypoint;
    const std::int64_t deltaX =
        static_cast<std::int64_t>(movementTarget.x.Raw()) -
        entity.position.x.Raw();
    const std::int64_t deltaY =
        static_cast<std::int64_t>(movementTarget.y.Raw()) -
        entity.position.y.Raw();
    const std::int64_t distance = Abs64(deltaX) + Abs64(deltaY);
    if (distance == 0) {
        return entity.position == destination;
    }
    std::int32_t movementPerTick = entity.movementPerTickRaw;
    if (entity.deployed &&
        entity.faction == Faction::MeridianCompact &&
        entity.type == EntityType::HeavyUnit) {
        movementPerTick = std::max(
            1,
            static_cast<std::int32_t>(
                static_cast<std::int64_t>(movementPerTick) *
                config_.rules.bulwarkDeployment.deployedMovementPercent / 100));
    }
    const std::int64_t travel =
        std::min<std::int64_t>(movementPerTick, distance);
    std::int64_t stepX = travel * Abs64(deltaX) / distance;
    std::int64_t stepY = travel - stepX;
    stepX = deltaX < 0 ? -stepX : stepX;
    stepY = deltaY < 0 ? -stepY : stepY;
    const Vec2 candidate = Vec2::FromRaw(
        static_cast<std::int32_t>(entity.position.x.Raw() + stepX),
        static_cast<std::int32_t>(entity.position.y.Raw() + stepY));
    if (!IsPositionPassable(candidate)) {
        return false;
    }
    if (candidate != entity.position) {
        entity.position = candidate;
        entity.vibrationSignatureUntilTick = std::min(
            kMaximumSupportedTick,
            currentTick_ + config_.rules.vibrationDetection.signatureLingerTicks);
    }
    return entity.position == destination;
}

std::int32_t Simulation::DamageAfterDirectionalCover(
    const Entity& attacker,
    const Entity& target,
    std::int32_t damage) const {
    if (damage <= 0 || target.owner == kNeutralPlayer ||
        attacker.owner == target.owner) {
        return damage;
    }
    const BulwarkDeploymentRules& rules = config_.rules.bulwarkDeployment;
    for (const Entity& bulwark : entities_) {
        if (!bulwark.deployed || !bulwark.completed || bulwark.hitPoints <= 0 ||
            bulwark.owner != target.owner ||
            bulwark.faction != Faction::MeridianCompact ||
            bulwark.type != EntityType::HeavyUnit) {
            continue;
        }

        const std::int64_t attackerDeltaX =
            static_cast<std::int64_t>(attacker.position.x.Raw()) -
            bulwark.position.x.Raw();
        const std::int64_t attackerDeltaY =
            static_cast<std::int64_t>(attacker.position.y.Raw()) -
            bulwark.position.y.Raw();
        const std::int64_t targetDeltaX =
            static_cast<std::int64_t>(target.position.x.Raw()) -
            bulwark.position.x.Raw();
        const std::int64_t targetDeltaY =
            static_cast<std::int64_t>(target.position.y.Raw()) -
            bulwark.position.y.Raw();

        std::int64_t attackerForward = 0;
        std::int64_t targetBehind = 0;
        std::int64_t targetLateral = 0;
        if (bulwark.deploymentFacing.x.Raw() != 0) {
            const std::int32_t sign = bulwark.deploymentFacing.x.Raw() > 0 ? 1 : -1;
            attackerForward = attackerDeltaX * sign;
            targetBehind = -targetDeltaX * sign;
            targetLateral = Abs64(targetDeltaY);
        } else {
            const std::int32_t sign = bulwark.deploymentFacing.y.Raw() > 0 ? 1 : -1;
            attackerForward = attackerDeltaY * sign;
            targetBehind = -targetDeltaY * sign;
            targetLateral = Abs64(targetDeltaX);
        }
        if (attackerForward <= 0 || targetBehind < 0 ||
            targetBehind > rules.coverDepthRaw ||
            targetLateral > rules.coverHalfWidthRaw) {
            continue;
        }
        return std::max(
            1,
            static_cast<std::int32_t>(
                static_cast<std::int64_t>(damage) *
                (100 - rules.damageReductionPercent) / 100));
    }
    return damage;
}

EntityId Simulation::FindNearestOwnedDropoff(PlayerId player, Vec2 from) const {
    EntityId nearest = 0;
    std::uint64_t nearestDistance = std::numeric_limits<std::uint64_t>::max();
    for (const Entity& entity : entities_) {
        if (entity.owner != player || !entity.completed ||
            !IsOperationalDropoff(entity)) {
            continue;
        }
        const std::uint64_t distance = DistanceSquaredRaw(from, entity.position);
        if (distance < nearestDistance ||
            (distance == nearestDistance && entity.id < nearest)) {
            nearest = entity.id;
            nearestDistance = distance;
        }
    }
    return nearest;
}

bool Simulation::IsProtectedCommandCore(const Entity& entity) const {
    return entity.type == EntityType::CommandCore &&
           entity.owner < kMaximumPlayers &&
           (config_.protectedCommandCorePlayerMask &
            static_cast<std::uint8_t>(1U << entity.owner)) != 0;
}

EntityId Simulation::FindNearestVisibleEnemy(PlayerId player,
                                             Vec2 from,
                                             std::int32_t radiusRaw) const {
    EntityId nearest = 0;
    std::uint64_t nearestDistance = std::numeric_limits<std::uint64_t>::max();
    const std::uint64_t radiusSquared =
        static_cast<std::uint64_t>(radiusRaw) * radiusRaw;
    for (const Entity& entity : entities_) {
        if (entity.owner == kNeutralPlayer || entity.owner == player ||
            entity.hitPoints <= 0 || IsProtectedCommandCore(entity) ||
            !IsEntityVisibleTo(player, entity.id)) {
            continue;
        }
        const std::uint64_t distance = DistanceSquaredRaw(from, entity.position);
        if (distance > radiusSquared) {
            continue;
        }
        if (distance < nearestDistance ||
            (distance == nearestDistance &&
             (nearest == 0 || entity.id < nearest))) {
            nearest = entity.id;
            nearestDistance = distance;
        }
    }
    return nearest;
}

EntityId Simulation::FindNearestVisibleEnemyInRange(
    const Entity& attacker) const {
    EntityId nearest = 0;
    std::uint64_t nearestDistance = std::numeric_limits<std::uint64_t>::max();
    for (const Entity& entity : entities_) {
        if (entity.owner == kNeutralPlayer || entity.owner == attacker.owner ||
            entity.hitPoints <= 0 || IsProtectedCommandCore(entity) ||
            !IsEntityVisibleTo(attacker.owner, entity.id) ||
            !InInteractionRange(attacker, entity, attacker.attackRangeRaw)) {
            continue;
        }
        const std::uint64_t distance =
            DistanceSquaredRaw(attacker.position, entity.position);
        if (distance < nearestDistance ||
            (distance == nearestDistance &&
             (nearest == 0 || entity.id < nearest))) {
            nearest = entity.id;
            nearestDistance = distance;
        }
    }
    return nearest;
}

bool Simulation::IsInsidePatrolEnvelope(
    const Order& order,
    Vec2 position) const {
    const std::int64_t minimumX = std::min<std::int64_t>(
        order.anchor.x.Raw(), order.destination.x.Raw());
    const std::int64_t maximumX = std::max<std::int64_t>(
        order.anchor.x.Raw(), order.destination.x.Raw());
    const std::int64_t minimumY = std::min<std::int64_t>(
        order.anchor.y.Raw(), order.destination.y.Raw());
    const std::int64_t maximumY = std::max<std::int64_t>(
        order.anchor.y.Raw(), order.destination.y.Raw());
    return static_cast<std::int64_t>(position.x.Raw()) >=
               minimumX - kPatrolLeashRaw &&
           static_cast<std::int64_t>(position.x.Raw()) <=
               maximumX + kPatrolLeashRaw &&
           static_cast<std::int64_t>(position.y.Raw()) >=
               minimumY - kPatrolLeashRaw &&
           static_cast<std::int64_t>(position.y.Raw()) <=
               maximumY + kPatrolLeashRaw;
}

EntityId Simulation::FindNearestVisiblePatrolEnemy(
    const Entity& attacker) const {
    EntityId nearest = 0;
    std::uint64_t nearestDistance = std::numeric_limits<std::uint64_t>::max();
    const std::int32_t visionRaw = attacker.visionTiles * kFixedScale;
    const std::uint64_t visionSquared =
        static_cast<std::uint64_t>(visionRaw) * visionRaw;
    for (const Entity& entity : entities_) {
        if (entity.owner == kNeutralPlayer || entity.owner == attacker.owner ||
            entity.hitPoints <= 0 || IsProtectedCommandCore(entity) ||
            !IsEntityVisibleTo(attacker.owner, entity.id) ||
            !IsInsidePatrolEnvelope(attacker.order, entity.position)) {
            continue;
        }
        const std::uint64_t distance =
            DistanceSquaredRaw(attacker.position, entity.position);
        if (distance > visionSquared) {
            continue;
        }
        if (distance < nearestDistance ||
            (distance == nearestDistance &&
             (nearest == 0 || entity.id < nearest))) {
            nearest = entity.id;
            nearestDistance = distance;
        }
    }
    return nearest;
}

std::optional<Vec2> Simulation::FindProductionSpawnPosition(
    const Entity& producer) const {
    const std::int32_t centerX = producer.position.x.FloorToInt();
    const std::int32_t centerY = producer.position.y.FloorToInt();
    for (std::int32_t radius = 2; radius <= 8; ++radius) {
        for (std::int32_t offsetY = -radius; offsetY <= radius; ++offsetY) {
            for (std::int32_t offsetX = -radius; offsetX <= radius; ++offsetX) {
                if (Abs64(offsetX) != radius && Abs64(offsetY) != radius) {
                    continue;
                }
                const Vec2 candidate =
                    Vec2::FromTiles(centerX + offsetX, centerY + offsetY);
                if (!IsPositionPassable(candidate)) {
                    continue;
                }
                bool blockedByBuilding = false;
                for (const Entity& entity : entities_) {
                    if (entity.hitPoints <= 0 || !IsBuilding(entity.type)) {
                        continue;
                    }
                    const std::int32_t combinedExtent =
                        FootprintHalfExtentRaw(entity.faction, entity.type) +
                        kFixedScale / 8;
                    if (Abs64(static_cast<std::int64_t>(candidate.x.Raw()) -
                              entity.position.x.Raw()) < combinedExtent &&
                        Abs64(static_cast<std::int64_t>(candidate.y.Raw()) -
                              entity.position.y.Raw()) < combinedExtent) {
                        blockedByBuilding = true;
                        break;
                    }
                }
                if (!blockedByBuilding) {
                    return candidate;
                }
            }
        }
    }
    return std::nullopt;
}

void Simulation::ProcessCommandsForCurrentTick() {
    std::vector<Command> due{};
    std::vector<Command> remaining{};
    due.reserve(pendingCommands_.size());
    remaining.reserve(pendingCommands_.size());
    for (const Command& command : pendingCommands_) {
        (command.executeTick == currentTick_ ? due : remaining).push_back(command);
    }
    std::sort(due.begin(), due.end(), CommandLess);
    pendingCommands_ = std::move(remaining);
    for (const Command& command : due) {
        const CommandResolutionOutcome outcome = ApplyCommand(command);
        RecordCommandResolutionReceipt(command, outcome);
        hasExecutedSequence_[command.player] = true;
        lastExecutedSequence_[command.player] = command.sequence;
    }
}

void Simulation::RecordCommandResolutionReceipt(
    const Command& command,
    CommandResolutionOutcome outcome) {
    // QueueCommand and snapshot validation enforce unique, monotonically
    // executed player/sequence keys. Preserve that invariant here without an
    // O(receipt-count) duplicate scan on the authoritative due-command path.
    StoredCommandResolutionReceipt stored{};
    stored.sequence = command.sequence;
    stored.receipt.player = command.player;
    stored.receipt.commandType = command.type;
    stored.receipt.assignedExecutionTick = command.executeTick;
    stored.receipt.outcome = outcome;
    commandResolutionReceipts_.push_back(stored);
    if (commandResolutionReceipts_.size() >
        kMaximumCommandResolutionReceipts) {
        commandResolutionReceipts_.pop_front();
    }
}

void Simulation::PruneCommandResolutionReceipts() {
    while (!commandResolutionReceipts_.empty()) {
        const Tick assigned = commandResolutionReceipts_.front()
                                  .receipt.assignedExecutionTick;
        if (currentTick_ <= assigned ||
            currentTick_ - assigned <=
                kCommandResolutionReceiptRetentionTicks) {
            break;
        }
        commandResolutionReceipts_.pop_front();
    }
}

CommandResolutionOutcome Simulation::ApplyCommand(const Command& command) {
    Entity* actor = MutableEntity(command.actor);
    if (actor == nullptr || actor->owner != command.player || !actor->completed ||
        actor->hitPoints <= 0) {
        return CommandResolutionOutcome::NoEffect;
    }
    if (actor->pendingWarformAdaptation != WarformAdaptation::None &&
        command.type != CommandType::AdaptWarform) {
        return CommandResolutionOutcome::NoEffect;
    }
    CommandResolutionOutcome outcome = CommandResolutionOutcome::NoEffect;
    const auto apply = [&]() {
        switch (command.type) {
        case CommandType::Stop: {
            PlayerState* player = MutablePlayer(command.player);
            if (player != nullptr &&
                player->activeResearch != ResearchType::None &&
                player->researchProducer == actor->id) {
                player->lastInterruptedResearch = player->activeResearch;
                player->activeResearch = ResearchType::None;
                player->researchProducer = 0;
                player->researchProgress = 0;
                player->researchRequired = 0;
            }
            actor->order = {};
            outcome = CommandResolutionOutcome::Applied;
            return;
        }
        case CommandType::Move:
            if (actor->movementPerTickRaw > 0 && IsInsideMap(command.position) &&
                (actor->waystoneMode == WaystoneMode::NotWaystone ||
                 actor->waystoneMode == WaystoneMode::Mobile)) {
                actor->order.type = OrderType::Move;
                actor->order.target = 0;
                actor->order.destination = command.position;
                outcome = CommandResolutionOutcome::Applied;
            }
            return;
        case CommandType::Gather: {
            const Entity* target = FindEntity(command.target);
            if (actor->type == EntityType::Worker && target != nullptr &&
                target->type == EntityType::ResourceNode &&
                target->resourceRemaining > 0 &&
                IsEntityVisibleTo(command.player, target->id)) {
                actor->order.type = OrderType::Gather;
                actor->order.target = target->id;
                actor->order.destination = target->position;
                outcome = CommandResolutionOutcome::Applied;
            }
            return;
        }
        case CommandType::Deliver: {
            const Entity* target = FindEntity(command.target);
            if (actor->type == EntityType::Worker && target != nullptr &&
                target->owner == command.player && target->completed &&
                IsOperationalDropoff(*target)) {
                actor->order.type = OrderType::Deliver;
                actor->order.target = target->id;
                actor->order.destination = target->position;
                outcome = CommandResolutionOutcome::Applied;
            }
            return;
        }
        case CommandType::Build: {
            if (actor->type != EntityType::Worker ||
                actor->order.type == OrderType::Build ||
                ValidatePlacement(command.player, command.buildType,
                                  command.position) != PlacementResult::Valid) {
                return;
            }
            PlayerState* player = MutablePlayer(command.player);
            const ResourcePool cost = BuildCost(player->faction, command.buildType);
            if (!ResourceCovers(player->resources, cost)) {
                return;
            }
            EntityId siteId = 0;
            if (!TryAllocateEntityId(siteId)) {
                return;
            }
            player->resources.material -= cost.material;
            player->resources.dawnshards -= cost.dawnshards;
            Entity site = MakeEntity(command.player, player->faction,
                                     command.buildType, command.position);
            site.id = siteId;
            site.completed = false;
            site.hitPoints = std::max(1, site.maxHitPoints / 10);
            site.constructionProgress = 0;
            actor->order.type = OrderType::Build;
            actor->order.target = site.id;
            actor->order.destination = site.position;
            actor->order.buildType = command.buildType;
            // Set the order before push_back; vector growth may relocate the actor.
            entities_.push_back(site);
            outcome = CommandResolutionOutcome::Applied;
            return;
        }
        case CommandType::Attack: {
            const Entity* target = FindEntity(command.target);
            if (actor->attackDamage > 0 && target != nullptr &&
                target->owner != kNeutralPlayer && target->owner != command.player &&
                !IsProtectedCommandCore(*target) &&
                IsEntityVisibleTo(command.player, target->id)) {
                actor->order.type = OrderType::Attack;
                actor->order.target = target->id;
                actor->order.destination = target->position;
                outcome = CommandResolutionOutcome::Applied;
            }
            return;
        }
        case CommandType::FutureWell: {
            const Entity* target = FindEntity(command.target);
            if (actor->type == EntityType::Worker && target != nullptr &&
                target->type == EntityType::FutureWell &&
                target->wellChoice == FutureWellChoice::Dormant &&
                command.wellChoice != FutureWellChoice::Dormant &&
                IsEntityVisibleTo(command.player, target->id)) {
                actor->order.type = OrderType::FutureWell;
                actor->order.target = target->id;
                actor->order.destination = target->position;
                actor->order.wellChoice = command.wellChoice;
                outcome = CommandResolutionOutcome::Applied;
            }
            return;
        }
        case CommandType::Produce: {
            if (ValidateProduction(command.player, actor->id, command.buildType) !=
                ProductionResult::Valid) {
                return;
            }
            PlayerState* player = MutablePlayer(command.player);
            if (player == nullptr) {
                return;
            }
            const ResourcePool cost =
                ProductionCost(player->faction, command.buildType);
            player->resources.material -= cost.material;
            player->resources.dawnshards -= cost.dawnshards;
            actor->productionType = command.buildType;
            actor->productionProgress = 0;
            actor->productionRequired =
                ProductionTicks(player->faction, command.buildType);
            outcome = CommandResolutionOutcome::Applied;
            return;
        }
        case CommandType::Research: {
            if (ValidateResearch(command.player, actor->id,
                                 command.researchType) != ResearchResult::Valid) {
                return;
            }
            PlayerState* player = MutablePlayer(command.player);
            const ResearchRules* rules = ResearchDefinition(command.researchType);
            if (player == nullptr || rules == nullptr) {
                return;
            }
            player->resources.material -= rules->cost.material;
            player->resources.dawnshards -= rules->cost.dawnshards;
            player->activeResearch = command.researchType;
            player->researchProducer = actor->id;
            player->researchProgress = 0;
            player->researchRequired = static_cast<std::int32_t>(
                rules->researchTicks);
            player->lastInterruptedResearch = ResearchType::None;
            outcome = CommandResolutionOutcome::Applied;
            return;
        }
        case CommandType::ReconcileToManifest:
        case CommandType::ReconcileToPossible: {
            const ChoirIdentityState stableState =
                command.type == CommandType::ReconcileToManifest
                    ? ChoirIdentityState::Manifest
                    : ChoirIdentityState::Possible;
            if (ValidateChoirReconciliation(
                    command.player, actor->id, stableState) !=
                ChoirReconciliationResult::Valid) {
                return;
            }
            PlayerState* player = MutablePlayer(command.player);
            if (player == nullptr) {
                return;
            }
            player->resources.dawnshards -=
                config_.rules.choirIdentity.dawnCost;
            actor->choirIdentityState =
                stableState == ChoirIdentityState::Manifest
                    ? ChoirIdentityState::DualResolveManifest
                    : ChoirIdentityState::DualResolvePossible;
            actor->choirIdentityResolveAtTick = std::min(
                kMaximumSupportedTick,
                currentTick_ + config_.rules.choirIdentity.durationTicks);
            actor->choirIdentityNextAvailableTick = std::min(
                kMaximumSupportedTick,
                actor->choirIdentityResolveAtTick +
                    config_.rules.choirIdentity.cooldownTicks);
            RefreshChoirIdentityStats(*actor);
            outcome = CommandResolutionOutcome::Applied;
            return;
        }
        case CommandType::AttackMove:
            if (actor->attackDamage > 0 && actor->movementPerTickRaw > 0 &&
                IsPositionPassable(command.position)) {
                actor->order.type = OrderType::AttackMove;
                actor->order.target = 0;
                actor->order.destination = command.position;
                outcome = CommandResolutionOutcome::Applied;
            }
            return;
        case CommandType::Hold:
            if (actor->attackDamage > 0) {
                actor->order.type = OrderType::Hold;
                actor->order.target = 0;
                actor->order.destination = actor->position;
                outcome = CommandResolutionOutcome::Applied;
            }
            return;
        case CommandType::Guard: {
            const Entity* guarded = FindEntity(command.target);
            if (actor->attackDamage > 0 && guarded != nullptr &&
                guarded->owner == command.player && guarded->id != actor->id) {
                actor->order.type = OrderType::Guard;
                actor->order.target = guarded->id;
                actor->order.destination = guarded->position;
                outcome = CommandResolutionOutcome::Applied;
            }
            return;
        }
        case CommandType::Patrol:
            if (actor->attackDamage > 0 && actor->movementPerTickRaw > 0 &&
                IsPositionPassable(command.position) &&
                command.position != actor->position) {
                actor->order.type = OrderType::Patrol;
                actor->order.target = 0;
                actor->order.anchor = actor->position;
                actor->order.destination = command.position;
                outcome = CommandResolutionOutcome::Applied;
            }
            return;
        case CommandType::ToggleDeploy: {
            if (actor->faction != Faction::MeridianCompact ||
                actor->type != EntityType::HeavyUnit) {
                return;
            }
            if (actor->deployed) {
                actor->deployed = false;
                outcome = CommandResolutionOutcome::Applied;
                return;
            }
            const std::int64_t deltaX =
                static_cast<std::int64_t>(command.position.x.Raw()) -
                actor->position.x.Raw();
            const std::int64_t deltaY =
                static_cast<std::int64_t>(command.position.y.Raw()) -
                actor->position.y.Raw();
            if (deltaX == 0 && deltaY == 0) {
                return;
            }
            if (Abs64(deltaX) >= Abs64(deltaY)) {
                actor->deploymentFacing = Vec2::FromRaw(
                    deltaX >= 0 ? kFixedScale : -kFixedScale,
                    0);
            } else {
                actor->deploymentFacing = Vec2::FromRaw(
                    0,
                    deltaY >= 0 ? kFixedScale : -kFixedScale);
            }
            actor->deployed = true;
            outcome = CommandResolutionOutcome::Applied;
            return;
        }
        case CommandType::ActivateRelaySupply:
            if (ValidateRelaySupply(command.player, actor->id) !=
                RelaySupplyResult::Valid) {
                return;
            }
            actor->relaySupplyActive = true;
            actor->relaySupplyUntilTick = std::min(
                kMaximumSupportedTick,
                currentTick_ + config_.rules.relaySupply.durationTicks);
            actor->relaySupplyCooldownUntilTick = std::min(
                kMaximumSupportedTick,
                currentTick_ + config_.rules.relaySupply.cooldownTicks);
            outcome = CommandResolutionOutcome::Applied;
            return;
        case CommandType::ToggleWaystoneRoot:
            if (ValidateWaystoneRoot(command.player, actor->id) !=
                WaystoneRootResult::Valid) {
                return;
            }
            actor->order = {};
            if (actor->waystoneMode == WaystoneMode::Rooted) {
                actor->waystoneMode = WaystoneMode::Uprooting;
                actor->waystoneTransitionUntilTick = std::min(
                    kMaximumSupportedTick,
                    currentTick_ + config_.rules.waystoneMigration.uprootTicks);
            } else {
                actor->waystoneMode = WaystoneMode::Rooting;
                actor->waystoneTransitionUntilTick = std::min(
                    kMaximumSupportedTick,
                    currentTick_ + config_.rules.waystoneMigration.rootTicks);
            }
            outcome = CommandResolutionOutcome::Applied;
            return;
        case CommandType::AdaptWarform: {
            if (ValidateWarformAdaptation(
                    command.player,
                    actor->id,
                    command.target,
                    command.warformAdaptation) !=
                WarformAdaptationResult::Valid) {
                return;
            }
            PlayerState* player = MutablePlayer(command.player);
            if (player == nullptr) {
                return;
            }
            player->resources.dawnshards -=
                config_.rules.warformAdaptation.dawnCost;
            ApplyWarformAdaptation(*actor, WarformAdaptation::None);
            actor->order = {};
            actor->pendingWarformAdaptation = command.warformAdaptation;
            actor->moltSite = command.target;
            actor->moltUntilTick = std::min(
                kMaximumSupportedTick,
                currentTick_ + config_.rules.warformAdaptation.moltTicks);
            outcome = CommandResolutionOutcome::Applied;
            return;
        }
        case CommandType::RaiseMineralCover: {
            const MineralCoverResult validation = ValidateMineralCover(
                command.player, actor->id, command.position);
            if (validation == MineralCoverResult::InvalidPosition) {
                outcome = CommandResolutionOutcome::InvalidPosition;
                return;
            }
            if (validation != MineralCoverResult::Valid) {
                return;
            }
            PlayerState* player = MutablePlayer(command.player);
            if (player == nullptr) {
                return;
            }
            EntityId coverId = 0;
            if (!TryAllocateEntityId(coverId)) {
                return;
            }
            const MineralCoverRules& rules = config_.rules.mineralCover;
            const std::int32_t tileX = command.position.x.FloorToInt();
            const std::int32_t tileY = command.position.y.FloorToInt();
            Entity cover = MakeEntity(command.player, actor->faction,
                                      EntityType::UtilityStructure,
                                      command.position);
            cover.id = coverId;
            cover.hitPoints = rules.maxHitPoints;
            cover.maxHitPoints = rules.maxHitPoints;
            cover.movementPerTickRaw = 0;
            cover.visionTiles = 0;
            cover.attackRangeRaw = 0;
            cover.attackDamage = 0;
            cover.attackPeriodTicks = 0;
            cover.workRate = 0;
            cover.cargoCapacity = 0;
            cover.constructionRequired = 0;
            cover.temporaryMineralCover = true;
            cover.mineralCoverCreator = actor->id;
            cover.mineralCoverUntilTick = std::min(
                kMaximumSupportedTick,
                currentTick_ + rules.durationTicks);
            cover.mineralCoverUnderlyingTerrain = TerrainAt(tileX, tileY);
            player->resources.dawnshards -= rules.dawnCost;
            actor->order = {};
            actor->mineralCoverCooldownUntilTick = std::min(
                kMaximumSupportedTick,
                currentTick_ + rules.cooldownTicks);
            (void)SetTerrainTile(tileX, tileY, Terrain::Blocked);
            entities_.push_back(cover);
            outcome = CommandResolutionOutcome::Applied;
            return;
        }
        }
    };
    apply();
    return outcome;
}

void Simulation::ProcessGather(Entity& worker) {
    Entity* resource = MutableEntity(worker.order.target);
    if (resource == nullptr || resource->type != EntityType::ResourceNode ||
        resource->resourceRemaining <= 0) {
        worker.order = {};
        return;
    }
    if (!InInteractionRange(worker, *resource, kFixedScale / 2)) {
        (void)MoveTowards(worker, resource->position);
        return;
    }
    const std::int32_t capacity = worker.cargoCapacity - worker.cargo;
    const std::int32_t gathered =
        std::min({worker.workRate, capacity, resource->resourceRemaining});
    if (gathered > 0) {
        worker.cargo += gathered;
        resource->resourceRemaining -= gathered;
    }
    if (worker.cargo >= worker.cargoCapacity || resource->resourceRemaining <= 0) {
        worker.order = {};
    }
}

void Simulation::ProcessDeliver(Entity& worker) {
    Entity* dropoff = MutableEntity(worker.order.target);
    if (dropoff == nullptr || dropoff->owner != worker.owner || !dropoff->completed ||
        !IsOperationalDropoff(*dropoff)) {
        worker.order = {};
        return;
    }
    if (!InInteractionRange(worker, *dropoff, kFixedScale / 2)) {
        (void)MoveTowards(worker, dropoff->position);
        return;
    }
    PlayerState* player = MutablePlayer(worker.owner);
    if (player != nullptr && worker.cargo > 0) {
        player->resources.material =
            SaturatingAdd(player->resources.material, worker.cargo);
        worker.cargo = 0;
    }
    worker.order = {};
}

void Simulation::ProcessBuild(Entity& worker) {
    Entity* site = MutableEntity(worker.order.target);
    if (site == nullptr || site->owner != worker.owner || site->completed ||
        !IsBuilding(site->type)) {
        worker.order = {};
        return;
    }
    if (!InInteractionRange(worker, *site, kFixedScale / 2)) {
        (void)MoveTowards(worker, site->position);
        return;
    }
    site->constructionProgress = std::min(
        site->constructionRequired,
        SaturatingAdd(site->constructionProgress, worker.workRate));
    const std::int64_t scaledHealth =
        static_cast<std::int64_t>(site->maxHitPoints) * site->constructionProgress /
        std::max(1, site->constructionRequired);
    site->hitPoints =
        std::max(site->hitPoints, static_cast<std::int32_t>(scaledHealth));
    if (site->constructionProgress >= site->constructionRequired) {
        site->completed = true;
        site->hitPoints = site->maxHitPoints;
        if (IsChoirCoherenceStructure(*site)) {
            site->choirCoherenceNextChargeTick = std::min(
                kMaximumSupportedTick,
                currentTick_ +
                    config_.rules.choirCoherence.upkeepIntervalTicks);
        }
        worker.order = {};
    }
}

void Simulation::ProcessAttack(
    Entity& attacker,
    std::vector<PendingDamage>& pendingDamage) {
    Entity* target = MutableEntity(attacker.order.target);
    if (target == nullptr || target->owner == kNeutralPlayer ||
        target->owner == attacker.owner ||
        IsProtectedCommandCore(*target) ||
        !IsEntityVisibleTo(attacker.owner, target->id)) {
        attacker.order = {};
        return;
    }
    if (!InInteractionRange(attacker, *target, attacker.attackRangeRaw)) {
        (void)MoveTowards(attacker, target->position);
        return;
    }
    if (attacker.attackCooldownTicks == 0) {
        pendingDamage.push_back({target->id, attacker.id, attacker.attackDamage});
        attacker.attackCooldownTicks = attacker.attackPeriodTicks;
    }
}

void Simulation::ProcessAttackMove(
    Entity& attacker,
    std::vector<PendingDamage>& pendingDamage) {
    if (attacker.attackDamage <= 0 || attacker.movementPerTickRaw <= 0) {
        attacker.order = {};
        return;
    }

    Entity* target = attacker.order.target != 0
                         ? MutableEntity(attacker.order.target)
                         : nullptr;
    if (target == nullptr || target->owner == kNeutralPlayer ||
        target->owner == attacker.owner ||
        (target != nullptr && IsProtectedCommandCore(*target)) ||
        !IsEntityVisibleTo(attacker.owner, target->id)) {
        attacker.order.target = 0;
        target = nullptr;
    }
    if (target == nullptr) {
        attacker.order.target = FindNearestVisibleEnemy(
            attacker.owner,
            attacker.position,
            attacker.visionTiles * kFixedScale);
        target = attacker.order.target != 0
                     ? MutableEntity(attacker.order.target)
                     : nullptr;
    }
    if (target != nullptr) {
        if (!InInteractionRange(attacker, *target, attacker.attackRangeRaw)) {
            (void)MoveTowards(attacker, target->position);
            return;
        }
        if (attacker.attackCooldownTicks == 0) {
            pendingDamage.push_back({target->id, attacker.id, attacker.attackDamage});
            attacker.attackCooldownTicks = attacker.attackPeriodTicks;
        }
        return;
    }
    if (MoveTowards(attacker, attacker.order.destination)) {
        attacker.order = {};
    }
}

void Simulation::ProcessHold(
    Entity& attacker,
    std::vector<PendingDamage>& pendingDamage) {
    if (attacker.attackDamage <= 0) {
        attacker.order = {};
        return;
    }

    Entity* target = attacker.order.target != 0
                         ? MutableEntity(attacker.order.target)
                         : nullptr;
    if (target == nullptr || target->owner == kNeutralPlayer ||
        target->owner == attacker.owner ||
        (target != nullptr && IsProtectedCommandCore(*target)) ||
        !IsEntityVisibleTo(attacker.owner, target->id) ||
        !InInteractionRange(attacker, *target, attacker.attackRangeRaw)) {
        attacker.order.target = 0;
        target = nullptr;
    }
    if (target == nullptr) {
        attacker.order.target = FindNearestVisibleEnemyInRange(attacker);
        target = attacker.order.target != 0
                     ? MutableEntity(attacker.order.target)
                     : nullptr;
    }
    if (target != nullptr && attacker.attackCooldownTicks == 0) {
        pendingDamage.push_back({target->id, attacker.id, attacker.attackDamage});
        attacker.attackCooldownTicks = attacker.attackPeriodTicks;
    }
}

void Simulation::ProcessGuard(
    Entity& attacker,
    std::vector<PendingDamage>& pendingDamage) {
    Entity* guarded = MutableEntity(attacker.order.target);
    if (attacker.attackDamage <= 0 || guarded == nullptr ||
        guarded->owner != attacker.owner || guarded->id == attacker.id) {
        attacker.order = {};
        return;
    }
    attacker.order.destination = guarded->position;

    const std::uint64_t leashSquared =
        static_cast<std::uint64_t>(kGuardLeashRaw) * kGuardLeashRaw;
    if (DistanceSquaredRaw(attacker.position, guarded->position) >
        leashSquared) {
        (void)MoveTowards(attacker, guarded->position);
        return;
    }

    const EntityId enemyId = FindNearestVisibleEnemy(
        attacker.owner,
        guarded->position,
        kGuardLeashRaw);
    Entity* enemy = enemyId != 0 ? MutableEntity(enemyId) : nullptr;
    if (enemy != nullptr) {
        if (!InInteractionRange(attacker, *enemy, attacker.attackRangeRaw)) {
            (void)MoveTowards(attacker, enemy->position);
            return;
        }
        if (attacker.attackCooldownTicks == 0) {
            pendingDamage.push_back({enemy->id, attacker.id, attacker.attackDamage});
            attacker.attackCooldownTicks = attacker.attackPeriodTicks;
        }
        return;
    }

    const std::uint64_t followSquared =
        static_cast<std::uint64_t>(kGuardFollowRaw) * kGuardFollowRaw;
    if (DistanceSquaredRaw(attacker.position, guarded->position) >
        followSquared) {
        (void)MoveTowards(attacker, guarded->position);
    }
}

void Simulation::ProcessPatrol(
    Entity& attacker,
    std::vector<PendingDamage>& pendingDamage) {
    if (attacker.attackDamage <= 0 || attacker.movementPerTickRaw <= 0) {
        attacker.order = {};
        return;
    }

    Entity* target = attacker.order.target != 0
                         ? MutableEntity(attacker.order.target)
                         : nullptr;
    if (target == nullptr || target->owner == kNeutralPlayer ||
        target->owner == attacker.owner ||
        (target != nullptr && IsProtectedCommandCore(*target)) ||
        !IsEntityVisibleTo(attacker.owner, target->id) ||
        !IsInsidePatrolEnvelope(attacker.order, target->position)) {
        attacker.order.target = 0;
        target = nullptr;
    }
    if (target == nullptr) {
        attacker.order.target = FindNearestVisiblePatrolEnemy(attacker);
        target = attacker.order.target != 0
                     ? MutableEntity(attacker.order.target)
                     : nullptr;
    }
    if (target != nullptr) {
        if (!InInteractionRange(attacker, *target, attacker.attackRangeRaw)) {
            (void)MoveTowards(attacker, target->position);
            return;
        }
        if (attacker.attackCooldownTicks == 0) {
            pendingDamage.push_back({target->id, attacker.id, attacker.attackDamage});
            attacker.attackCooldownTicks = attacker.attackPeriodTicks;
        }
        return;
    }

    if (MoveTowards(attacker, attacker.order.destination)) {
        std::swap(attacker.order.anchor, attacker.order.destination);
    }
}

void Simulation::ProcessAegisDefense(
    Entity& aegis,
    std::vector<PendingDamage>& pendingDamage) {
    if (!aegis.aegisPowered || aegis.attackDamage <= 0 ||
        aegis.attackPeriodTicks == 0) {
        return;
    }
    const EntityId targetId = FindNearestVisibleEnemyInRange(aegis);
    if (targetId != 0 && aegis.attackCooldownTicks == 0) {
        pendingDamage.push_back({targetId, aegis.id, aegis.attackDamage});
        aegis.attackCooldownTicks = aegis.attackPeriodTicks;
    }
}

void Simulation::ProcessFutureWell(Entity& worker) {
    Entity* well = MutableEntity(worker.order.target);
    if (well == nullptr || well->type != EntityType::FutureWell ||
        well->wellChoice != FutureWellChoice::Dormant) {
        worker.order = {};
        return;
    }
    if (!InInteractionRange(worker, *well, kFixedScale / 2)) {
        (void)MoveTowards(worker, well->position);
        return;
    }
    PlayerState* player = MutablePlayer(worker.owner);
    if (player == nullptr) {
        worker.order = {};
        return;
    }
    switch (worker.order.wellChoice) {
        case FutureWellChoice::Harvest: {
            player->resources.dawnshards =
                SaturatingAdd(player->resources.dawnshards,
                              config_.rules.futureWell.harvestImmediateDawn);
            well->owner = worker.owner;
            well->faction = worker.faction;
            well->wellChoice = FutureWellChoice::Harvest;
            const std::int32_t centerX = well->position.x.FloorToInt();
            const std::int32_t centerY = well->position.y.FloorToInt();
            for (std::int32_t offsetY = -1; offsetY <= 1; ++offsetY) {
                for (std::int32_t offsetX = -1; offsetX <= 1; ++offsetX) {
                    const std::int32_t tileX = centerX + offsetX;
                    const std::int32_t tileY = centerY + offsetY;
                    if (TerrainAt(tileX, tileY) == Terrain::Open) {
                        (void)SetTerrainTile(tileX, tileY, Terrain::Scarred);
                    }
                }
            }
            break;
        }
        case FutureWellChoice::Preserve:
            well->owner = worker.owner;
            well->faction = worker.faction;
            well->wellChoice = FutureWellChoice::Preserve;
            break;
        case FutureWellChoice::Reshape:
            if (player->resources.dawnshards <
                config_.rules.futureWell.reshapeDawnCost) {
                worker.order = {};
                return;
            }
            player->resources.dawnshards -=
                config_.rules.futureWell.reshapeDawnCost;
            well->owner = worker.owner;
            well->faction = worker.faction;
            well->wellChoice = FutureWellChoice::Reshape;
            well->reshapeVariant = static_cast<std::uint8_t>(rng_.Uniform(4));
            {
                const Tick minimum =
                    config_.rules.futureWell.reshapeDurationMinimumTicks;
                const Tick maximum =
                    config_.rules.futureWell.reshapeDurationMaximumTicks;
                const Tick span = maximum - minimum + 1;
                well->reshapeUntilTick =
                    currentTick_ + minimum + rng_.Uniform(
                        static_cast<std::uint32_t>(span));
            }
            pathFieldCache_.clear();
            break;
        case FutureWellChoice::Dormant:
            worker.order = {};
            return;
    }
    // Commands execute at currentTick_ and Step advances immediately after
    // processing. Record the completed activation boundary, not the command's
    // pre-step tick, so a saved state always satisfies activation <= current.
    well->wellActivationTick = currentTick_ + 1;
    worker.order = {};
}

void Simulation::ProcessProduction() {
    struct CompletedUnit final {
        EntityId producer = 0;
        Entity unit{};
    };
    std::vector<CompletedUnit> completedUnits{};
    for (Entity& producer : entities_) {
        if (producer.hitPoints <= 0 || !producer.completed ||
            producer.productionRequired <= 0) {
            continue;
        }
        producer.productionProgress = std::min(
            producer.productionRequired,
            SaturatingAdd(producer.productionProgress, 1));
        if (producer.productionProgress < producer.productionRequired) {
            continue;
        }
        const std::optional<Vec2> spawnPosition =
            FindProductionSpawnPosition(producer);
        if (!spawnPosition.has_value() ||
            entities_.size() + completedUnits.size() >=
                kMaximumSerializedEntities) {
            continue;
        }
        EntityId unitId = 0;
        if (!TryAllocateEntityId(unitId)) {
            continue;
        }
        Entity unit = MakeEntity(
            producer.owner,
            producer.faction,
            producer.productionType,
            *spawnPosition);
        unit.id = unitId;
        completedUnits.push_back({producer.id, unit});
    }
    for (const CompletedUnit& completion : completedUnits) {
        Entity* producer = MutableEntity(completion.producer);
        if (producer == nullptr || producer->hitPoints <= 0) {
            continue;
        }
        producer->productionProgress = 0;
        producer->productionRequired = 0;
        entities_.push_back(completion.unit);
    }
}

void Simulation::ProcessResearch() {
    for (PlayerState& player : players_) {
        if (!player.active || player.activeResearch == ResearchType::None) {
            continue;
        }
        const Entity* producer = FindEntity(player.researchProducer);
        if (producer == nullptr || producer->owner != player.id ||
            producer->hitPoints <= 0 || !producer->completed ||
            producer->type != EntityType::Barracks) {
            player.lastInterruptedResearch = player.activeResearch;
            player.activeResearch = ResearchType::None;
            player.researchProducer = 0;
            player.researchProgress = 0;
            player.researchRequired = 0;
            continue;
        }
        player.researchProgress = std::min(
            player.researchRequired,
            SaturatingAdd(player.researchProgress, 1));
        if (player.researchProgress < player.researchRequired) {
            continue;
        }
        const ResearchType completed = player.activeResearch;
        const ResearchRules* rules = ResearchDefinition(completed);
        if (rules != nullptr) {
            player.completedResearchMask |=
                1U << static_cast<std::uint8_t>(completed);
            for (Entity& entity : entities_) {
                if (entity.owner != player.id || entity.hitPoints <= 0 ||
                    !IsBarracksUnitType(entity.type)) {
                    continue;
                }
                Entity refreshed = MakeEntity(
                    entity.owner, entity.faction, entity.type, entity.position);
                if (entity.warformAdaptation != WarformAdaptation::None) {
                    ApplyWarformAdaptation(
                        refreshed, entity.warformAdaptation);
                }
                entity.attackDamage = refreshed.attackDamage;
                entity.visionTiles = refreshed.visionTiles;
                if (IsChoirIdentityUnit(entity)) {
                    RefreshChoirIdentityStats(entity);
                }
            }
        }
        player.activeResearch = ResearchType::None;
        player.researchProducer = 0;
        player.researchProgress = 0;
        player.researchRequired = 0;
        player.lastInterruptedResearch = ResearchType::None;
    }
}

void Simulation::ProcessEntityOrders() {
    std::vector<PendingDamage> pendingDamage{};
    for (Entity& entity : entities_) {
        if (entity.hitPoints <= 0 || !entity.completed) {
            continue;
        }
        if (entity.attackCooldownTicks > 0) {
            --entity.attackCooldownTicks;
        }
        if (IsAegisPost(entity)) {
            entity.order = {};
            ProcessAegisDefense(entity, pendingDamage);
            continue;
        }
        switch (entity.order.type) {
            case OrderType::None:
                break;
            case OrderType::Move:
                if (MoveTowards(entity, entity.order.destination)) {
                    entity.order = {};
                }
                break;
            case OrderType::Gather:
                ProcessGather(entity);
                break;
            case OrderType::Deliver:
                ProcessDeliver(entity);
                break;
            case OrderType::Build:
                ProcessBuild(entity);
                break;
            case OrderType::Attack:
                ProcessAttack(entity, pendingDamage);
                break;
            case OrderType::FutureWell:
                ProcessFutureWell(entity);
                break;
            case OrderType::AttackMove:
                ProcessAttackMove(entity, pendingDamage);
                break;
            case OrderType::Hold:
                ProcessHold(entity, pendingDamage);
                break;
            case OrderType::Guard:
                ProcessGuard(entity, pendingDamage);
                break;
            case OrderType::Patrol:
                ProcessPatrol(entity, pendingDamage);
                break;
        }
    }
    for (PendingDamage& damage : pendingDamage) {
        const Entity* attacker = FindEntity(damage.source);
        const Entity* target = FindEntity(damage.target);
        if (attacker == nullptr || target == nullptr ||
            IsProtectedCommandCore(*target)) {
            continue;
        }
        const EntityId cover = InterceptingMineralCover(*attacker, *target);
        if (cover != 0) {
            damage.target = cover;
        }
    }
    std::sort(
        pendingDamage.begin(),
        pendingDamage.end(),
        [](const PendingDamage& lhs, const PendingDamage& rhs) {
            return std::tie(lhs.target, lhs.source, lhs.damage) <
                   std::tie(rhs.target, rhs.source, rhs.damage);
        });
    std::size_t index = 0;
    while (index < pendingDamage.size()) {
        const EntityId targetId = pendingDamage[index].target;
        std::int64_t totalDamage = 0;
        const Entity* target = FindEntity(targetId);
        while (index < pendingDamage.size() &&
               pendingDamage[index].target == targetId) {
            const Entity* attacker = FindEntity(pendingDamage[index].source);
            std::int32_t resolvedDamage =
                attacker != nullptr && target != nullptr
                    ? DamageAfterDirectionalCover(
                          *attacker,
                          *target,
                          pendingDamage[index].damage)
                    : pendingDamage[index].damage;
            if (target != nullptr &&
                target->faction == Faction::KharuunAssemblies &&
                target->type == EntityType::Dropoff &&
                target->waystoneMode != WaystoneMode::Rooted) {
                resolvedDamage = std::max(
                    1,
                    static_cast<std::int32_t>(
                        static_cast<std::int64_t>(resolvedDamage) *
                        config_.rules.waystoneMigration.mobileDamageTakenPercent /
                        100));
            }
            if (target != nullptr &&
                target->pendingWarformAdaptation !=
                    WarformAdaptation::None) {
                resolvedDamage = std::max(
                    1,
                    static_cast<std::int32_t>(
                        static_cast<std::int64_t>(resolvedDamage) *
                        config_.rules.warformAdaptation
                            .moltDamageTakenPercent /
                        100));
            }
            totalDamage += resolvedDamage;
            ++index;
        }
        if (Entity* mutableTarget = MutableEntity(targetId);
            mutableTarget != nullptr &&
            !IsProtectedCommandCore(*mutableTarget)) {
            mutableTarget->hitPoints -= static_cast<std::int32_t>(std::min<std::int64_t>(
                totalDamage, std::numeric_limits<std::int32_t>::max()));
        }
    }
}

void Simulation::ApplyPreserveIncome() {
    if ((currentTick_ + 1) %
            config_.rules.futureWell.preserveIntervalTicks !=
        0) {
        return;
    }
    for (const Entity& entity : entities_) {
        if (entity.type == EntityType::FutureWell &&
            entity.wellChoice == FutureWellChoice::Preserve) {
            if (PlayerState* player = MutablePlayer(entity.owner); player != nullptr) {
                player->resources.dawnshards =
                    SaturatingAdd(
                        player->resources.dawnshards,
                        config_.rules.futureWell.preserveDawnPerInterval);
            }
        }
    }
}

void Simulation::RemoveDestroyedEntities() {
    for (const Entity& entity : entities_) {
        if (!entity.temporaryMineralCover || entity.hitPoints > 0) {
            continue;
        }
        const std::int32_t tileX = entity.position.x.FloorToInt();
        const std::int32_t tileY = entity.position.y.FloorToInt();
        if (TerrainAt(tileX, tileY) == Terrain::Blocked) {
            (void)SetTerrainTile(
                tileX, tileY, entity.mineralCoverUnderlyingTerrain);
        }
    }
    std::erase_if(entities_, [](const Entity& entity) {
        return entity.hitPoints <= 0 ||
               (entity.type == EntityType::ResourceNode &&
                entity.resourceRemaining <= 0);
    });
}

void Simulation::ClearInvalidOrders() {
    for (Entity& entity : entities_) {
        switch (entity.order.type) {
            case OrderType::Gather:
            case OrderType::Deliver:
            case OrderType::Build:
            case OrderType::Attack:
            case OrderType::FutureWell:
                if (const Entity* target = FindEntity(entity.order.target);
                    target == nullptr ||
                    (entity.order.type == OrderType::Attack &&
                     IsProtectedCommandCore(*target))) {
                    entity.order = {};
                }
                break;
            case OrderType::AttackMove:
                if (entity.order.target != 0 &&
                    (FindEntity(entity.order.target) == nullptr ||
                     IsProtectedCommandCore(
                         *FindEntity(entity.order.target)))) {
                    entity.order.target = 0;
                }
                break;
            case OrderType::Hold:
                if (entity.order.target != 0 &&
                    (FindEntity(entity.order.target) == nullptr ||
                     IsProtectedCommandCore(
                         *FindEntity(entity.order.target)))) {
                    entity.order.target = 0;
                }
                break;
            case OrderType::Guard:
                if (FindEntity(entity.order.target) == nullptr) {
                    entity.order = {};
                }
                break;
            case OrderType::Patrol:
                if (entity.order.target != 0 &&
                    (FindEntity(entity.order.target) == nullptr ||
                     IsProtectedCommandCore(
                         *FindEntity(entity.order.target)))) {
                    entity.order.target = 0;
                }
                break;
            case OrderType::None:
            case OrderType::Move:
                break;
        }
    }
}

void Simulation::ResolveExpiredReshapes() {
    std::vector<Vec2> expiredCenters{};
    for (Entity& entity : entities_) {
        if (entity.type == EntityType::FutureWell &&
            entity.wellChoice == FutureWellChoice::Reshape &&
            entity.reshapeUntilTick != 0 &&
            currentTick_ >= entity.reshapeUntilTick) {
            expiredCenters.push_back(entity.position);
            entity.reshapeUntilTick = 0;
        }
    }
    if (expiredCenters.empty()) {
        return;
    }
    pathFieldCache_.clear();

    for (Entity& entity : entities_) {
        if (entity.hitPoints <= 0 || entity.movementPerTickRaw <= 0) {
            continue;
        }
        const std::int32_t tileX = entity.position.x.FloorToInt();
        const std::int32_t tileY = entity.position.y.FloorToInt();
        if (TerrainAt(tileX, tileY) != Terrain::Blocked) {
            continue;
        }
        const bool affected = std::any_of(
            expiredCenters.begin(), expiredCenters.end(), [&](Vec2 center) {
                return Abs64(static_cast<std::int64_t>(tileX) -
                             center.x.FloorToInt()) <= 1 &&
                       Abs64(static_cast<std::int64_t>(tileY) -
                             center.y.FloorToInt()) <= 1;
            });
        if (!affected) {
            continue;
        }

        bool foundFallback = false;
        std::uint64_t bestDistance = std::numeric_limits<std::uint64_t>::max();
        std::size_t bestTile = 0;
        for (std::int32_t candidateY = 0; candidateY < config_.mapHeightTiles;
             ++candidateY) {
            for (std::int32_t candidateX = 0;
                 candidateX < config_.mapWidthTiles; ++candidateX) {
                if (TerrainAt(candidateX, candidateY) == Terrain::Blocked) {
                    continue;
                }
                const std::int64_t deltaX =
                    static_cast<std::int64_t>(candidateX) - tileX;
                const std::int64_t deltaY =
                    static_cast<std::int64_t>(candidateY) - tileY;
                const std::uint64_t distance = static_cast<std::uint64_t>(
                    deltaX * deltaX + deltaY * deltaY);
                const std::size_t candidateTile = static_cast<std::size_t>(
                    candidateY * config_.mapWidthTiles + candidateX);
                if (!foundFallback || distance < bestDistance ||
                    (distance == bestDistance && candidateTile < bestTile)) {
                    foundFallback = true;
                    bestDistance = distance;
                    bestTile = candidateTile;
                }
            }
        }
        if (foundFallback) {
            const std::int32_t fallbackX = static_cast<std::int32_t>(
                bestTile % static_cast<std::size_t>(config_.mapWidthTiles));
            const std::int32_t fallbackY = static_cast<std::int32_t>(
                bestTile / static_cast<std::size_t>(config_.mapWidthTiles));
            entity.position = Vec2::FromTiles(fallbackX, fallbackY);
        } else {
            // Invalid all-blocked maps still resolve deterministically without
            // leaving an entity in an inescapable cell.
            (void)SetTerrainTile(tileX, tileY, Terrain::Open);
        }
        entity.order = {};
    }
}

void Simulation::ResolveExpiredRelaySupply() {
    for (Entity& entity : entities_) {
        if (entity.relaySupplyActive &&
            (currentTick_ >= entity.relaySupplyUntilTick ||
             !IsRelayConnected(entity))) {
            entity.relaySupplyActive = false;
            entity.relaySupplyUntilTick = 0;
        }
    }
}

void Simulation::ResolveWaystoneTransitions() {
    for (Entity& entity : entities_) {
        if (entity.waystoneMode == WaystoneMode::Uprooting &&
            currentTick_ >= entity.waystoneTransitionUntilTick) {
            entity.waystoneMode = WaystoneMode::Mobile;
            entity.waystoneTransitionUntilTick = 0;
        } else if (entity.waystoneMode == WaystoneMode::Rooting &&
                   currentTick_ >= entity.waystoneTransitionUntilTick) {
            if (CanRootWaystone(entity)) {
                entity.waystoneMode = WaystoneMode::Rooted;
                entity.waystoneTransitionUntilTick = 0;
                entity.order = {};
            } else {
                entity.waystoneMode = WaystoneMode::Mobile;
                entity.waystoneTransitionUntilTick = 0;
            }
        }
    }
}

void Simulation::ResolveWarformMolts() {
    for (Entity& entity : entities_) {
        if (entity.pendingWarformAdaptation == WarformAdaptation::None) {
            continue;
        }
        const Entity* site = FindEntity(entity.moltSite);
        const std::int64_t radius =
            config_.rules.warformAdaptation.siteRadiusRaw;
        const bool siteValid =
            site != nullptr && site->owner == entity.owner && site->completed &&
            site->hitPoints > 0 &&
            site->faction == Faction::KharuunAssemblies &&
            site->type == EntityType::Barracks &&
            DistanceSquaredRaw(entity.position, site->position) <=
                static_cast<std::uint64_t>(radius * radius);
        if (!siteValid) {
            entity.pendingWarformAdaptation = WarformAdaptation::None;
            entity.moltSite = 0;
            entity.moltUntilTick = 0;
            continue;
        }
        if (currentTick_ >= entity.moltUntilTick) {
            const WarformAdaptation completed =
                entity.pendingWarformAdaptation;
            ApplyWarformAdaptation(entity, completed);
            entity.pendingWarformAdaptation = WarformAdaptation::None;
            entity.moltSite = 0;
            entity.moltUntilTick = 0;
            entity.order = {};
        }
    }
}

void Simulation::ResolveMineralCovers() {
    for (Entity& entity : entities_) {
        if (entity.temporaryMineralCover && entity.hitPoints > 0 &&
            currentTick_ >= entity.mineralCoverUntilTick) {
            entity.hitPoints = 0;
            const std::int32_t tileX = entity.position.x.FloorToInt();
            const std::int32_t tileY = entity.position.y.FloorToInt();
            if (TerrainAt(tileX, tileY) == Terrain::Blocked) {
                (void)SetTerrainTile(
                    tileX, tileY, entity.mineralCoverUnderlyingTerrain);
            }
        }
    }
}

void Simulation::ResolveAegisPower() {
    for (Entity& entity : entities_) {
        entity.aegisPowered =
            IsAegisPost(entity) && IsAegisNetworkPowered(entity);
    }
}

void Simulation::ResolveChoirIdentities() {
    for (Entity& entity : entities_) {
        if (!IsChoirIdentityUnit(entity) ||
            (entity.choirIdentityState !=
                 ChoirIdentityState::DualResolveManifest &&
             entity.choirIdentityState !=
                 ChoirIdentityState::DualResolvePossible) ||
            currentTick_ < entity.choirIdentityResolveAtTick) {
            continue;
        }
        entity.choirIdentityState =
            entity.choirIdentityState ==
                    ChoirIdentityState::DualResolveManifest
                ? ChoirIdentityState::Manifest
                : ChoirIdentityState::Possible;
        entity.choirIdentityResolveAtTick = 0;
        RefreshChoirIdentityStats(entity);
    }
}

void Simulation::ResolveChoirCoherence() {
    for (Entity& entity : entities_) {
        if (!entity.completed || entity.hitPoints <= 0 ||
            !IsChoirCoherenceStructure(entity) ||
            entity.choirCoherenceNextChargeTick == 0 ||
            currentTick_ < entity.choirCoherenceNextChargeTick) {
            continue;
        }
        PlayerState* player = MutablePlayer(entity.owner);
        if (player == nullptr ||
            player->resources.dawnshards <
                config_.rules.choirCoherence.dawnCostPerStructure) {
            entity.hitPoints = 0;
            continue;
        }
        player->resources.dawnshards -=
            config_.rules.choirCoherence.dawnCostPerStructure;
        entity.choirCoherenceNextChargeTick = std::min(
            kMaximumSupportedTick,
            currentTick_ + config_.rules.choirCoherence.upkeepIntervalTicks);
    }
}

void Simulation::Step() {
    if (currentTick_ >= kMaximumSupportedTick) {
        return;
    }
    ResolveExpiredRelaySupply();
    ResolveWaystoneTransitions();
    ResolveWarformMolts();
    ResolveMineralCovers();
    ResolveChoirIdentities();
    ResolveChoirCoherence();
    ResolveAegisPower();
    UpdateVisibility();
    ProcessCommandsForCurrentTick();
    ProcessEntityOrders();
    ProcessProduction();
    ProcessResearch();
    ApplyPreserveIncome();
    RemoveDestroyedEntities();
    ClearInvalidOrders();
    ++currentTick_;
    PruneCommandResolutionReceipts();
    ResolveExpiredRelaySupply();
    ResolveWaystoneTransitions();
    ResolveWarformMolts();
    ResolveMineralCovers();
    ResolveChoirIdentities();
    RemoveDestroyedEntities();
    ResolveAegisPower();
    ResolveExpiredReshapes();
    UpdateVisibility();
}

void Simulation::Step(Tick tickCount) {
    for (Tick tick = 0;
         tick < tickCount && currentTick_ < kMaximumSupportedTick; ++tick) {
        Step();
    }
}

void Simulation::UpdateVisibility() {
    for (PlayerId player = 0; player < visible_.size(); ++player) {
        std::fill(visible_[player].begin(), visible_[player].end(), 0);
    }
    const auto markVisible = [&](PlayerId player, Vec2 position,
                                 std::int32_t radiusTiles) {
        if (player >= players_.size() || !players_[player].active) {
            return;
        }
        const std::int32_t centerX = position.x.FloorToInt();
        const std::int32_t centerY = position.y.FloorToInt();
        for (std::int32_t offsetY = -radiusTiles; offsetY <= radiusTiles;
             ++offsetY) {
            for (std::int32_t offsetX = -radiusTiles; offsetX <= radiusTiles;
                 ++offsetX) {
                const std::int64_t distanceSquared =
                    static_cast<std::int64_t>(offsetX) * offsetX +
                    static_cast<std::int64_t>(offsetY) * offsetY;
                const std::int64_t radiusSquared =
                    static_cast<std::int64_t>(radiusTiles) * radiusTiles;
                if (distanceSquared > radiusSquared) {
                    continue;
                }
                const std::int32_t tileX = centerX + offsetX;
                const std::int32_t tileY = centerY + offsetY;
                if (tileX < 0 || tileY < 0 || tileX >= config_.mapWidthTiles ||
                    tileY >= config_.mapHeightTiles) {
                    continue;
                }
                const std::size_t tile = static_cast<std::size_t>(
                    tileY * config_.mapWidthTiles + tileX);
                visible_[player][tile] = 1;
                explored_[player][tile] = 1;
            }
        }
    };
    for (const Entity& entity : entities_) {
        if (entity.owner < players_.size() && players_[entity.owner].active) {
            markVisible(entity.owner, entity.position, entity.visionTiles);
            if (entity.type == EntityType::FutureWell &&
                entity.wellChoice == FutureWellChoice::Preserve) {
                markVisible(entity.owner, entity.position,
                            config_.rules.futureWell.preserveVisionTiles);
            }
        }
    }
}

#if defined(ECHOES_SIMCORE_PROFILE)
void Simulation::ProfileRefreshVisibility() {
    UpdateVisibility();
}

bool Simulation::ProfilePathRequest(Vec2 from, Vec2 destination) const {
    return FindNextPathWaypoint(from, destination).has_value();
}
#endif

Visibility PlayerView::VisibilityAt(Vec2 position) const {
    if (!ViewIsInsideMap(*this, position)) {
        return Visibility::Unexplored;
    }
    const std::size_t tile = static_cast<std::size_t>(
        position.y.FloorToInt() * config_.mapWidthTiles +
        position.x.FloorToInt());
    return tile < tiles_.size() ? tiles_[tile].visibility
                                : Visibility::Unexplored;
}

Terrain PlayerView::TerrainAt(std::int32_t tileX, std::int32_t tileY) const {
    if (tileX < 0 || tileY < 0 || tileX >= config_.mapWidthTiles ||
        tileY >= config_.mapHeightTiles) {
        return Terrain::Blocked;
    }
    const std::size_t tile =
        static_cast<std::size_t>(tileY * config_.mapWidthTiles + tileX);
    return tile < tiles_.size() ? tiles_[tile].terrain : Terrain::Blocked;
}

bool PlayerView::IsPositionPassable(Vec2 position) const {
    if (!ViewIsInsideMap(*this, position)) {
        return false;
    }
    const std::size_t tile = static_cast<std::size_t>(
        position.y.FloorToInt() * config_.mapWidthTiles +
        position.x.FloorToInt());
    return tile < tiles_.size() && tiles_[tile].passable;
}

Visibility Simulation::VisibilityAt(PlayerId player, Vec2 position) const {
    if (player >= players_.size() || !players_[player].active ||
        !IsInsideMap(position)) {
        return Visibility::Unexplored;
    }
    const std::size_t tile = static_cast<std::size_t>(
        position.y.FloorToInt() * config_.mapWidthTiles + position.x.FloorToInt());
    if (visible_[player][tile] != 0) {
        return Visibility::Visible;
    }
    return explored_[player][tile] != 0 ? Visibility::Explored
                                        : Visibility::Unexplored;
}

bool Simulation::IsEntityVisibleTo(PlayerId player, EntityId entity) const {
    const Entity* target = FindEntity(entity);
    if (target == nullptr || FindPlayer(player) == nullptr) {
        return false;
    }
    return target->owner == player ||
           VisibilityAt(player, target->position) == Visibility::Visible;
}

std::optional<PlayerView> Simulation::CreatePlayerView(PlayerId player) const {
    const PlayerState* playerState = FindPlayer(player);
    if (playerState == nullptr) {
        return std::nullopt;
    }

    PlayerView view{};
    view.config_ = config_;
    view.config_.randomSeed = 0;
    view.currentTick_ = currentTick_;
    view.player_ = *playerState;
    view.decisionSeed_ = config_.randomSeed;
    view.populationUsed_ = PopulationUsed(player);
    view.populationCapacity_ = PopulationCapacity(player);
    const std::size_t tileCount =
        static_cast<std::size_t>(config_.mapWidthTiles) *
        static_cast<std::size_t>(config_.mapHeightTiles);
    view.tiles_.resize(tileCount);
    for (std::int32_t tileY = 0; tileY < config_.mapHeightTiles; ++tileY) {
        for (std::int32_t tileX = 0; tileX < config_.mapWidthTiles; ++tileX) {
            const Vec2 position = Vec2::FromTiles(tileX, tileY);
            const Visibility visibility = VisibilityAt(player, position);
            PlayerViewTile& tile = view.tiles_[static_cast<std::size_t>(
                tileY * config_.mapWidthTiles + tileX)];
            tile.visibility = visibility;
            if (visibility != Visibility::Unexplored) {
                tile.terrain = TerrainAt(tileX, tileY);
                if (visibility != Visibility::Visible &&
                    tile.terrain == Terrain::Blocked) {
                    for (const Entity& entity : entities_) {
                        if (entity.temporaryMineralCover &&
                            entity.hitPoints > 0 &&
                            entity.position.x.FloorToInt() == tileX &&
                            entity.position.y.FloorToInt() == tileY) {
                            tile.terrain =
                                entity.mineralCoverUnderlyingTerrain;
                            break;
                        }
                    }
                }
                tile.passable =
                    tile.terrain != Terrain::Blocked ||
                    (visibility == Visibility::Visible &&
                     IsPositionPassable(position));
            }
        }
    }
    view.entities_.reserve(entities_.size());
    for (const Entity& entity : entities_) {
        if (entity.owner == player || IsEntityVisibleTo(player, entity.id)) {
            Entity observed = entity;
            if (entity.owner != player) {
                observed.hitPoints = 1;
                observed.maxHitPoints = 1;
                observed.movementPerTickRaw = 0;
                observed.visionTiles = 0;
                observed.attackRangeRaw = 0;
                observed.attackDamage = 0;
                observed.attackPeriodTicks = 0;
                observed.attackCooldownTicks = 0;
                observed.workRate = 0;
                observed.cargo = 0;
                observed.cargoCapacity = 0;
                observed.resourceRemaining =
                    entity.type == EntityType::ResourceNode &&
                            entity.resourceRemaining > 0
                        ? 1
                        : 0;
                observed.constructionProgress = 0;
                observed.constructionRequired = 0;
                observed.order = {};
                observed.reshapeUntilTick = 0;
                observed.reshapeVariant = 0;
                observed.productionType = EntityType::Worker;
                observed.productionProgress = 0;
                observed.productionRequired = 0;
                observed.relaySupplyUntilTick = 0;
                observed.relaySupplyCooldownUntilTick = 0;
                observed.waystoneTransitionUntilTick = 0;
                observed.moltUntilTick = 0;
                observed.mineralCoverCooldownUntilTick = 0;
                observed.mineralCoverUntilTick = 0;
                observed.mineralCoverUnderlyingTerrain = Terrain::Open;
                observed.vibrationSignatureUntilTick = 0;
                if (observed.mineralCoverCreator != 0 &&
                    !IsEntityVisibleTo(player,
                                       observed.mineralCoverCreator)) {
                    observed.mineralCoverCreator = 0;
                }
                if (observed.moltSite != 0 &&
                    !IsEntityVisibleTo(player, observed.moltSite)) {
                    observed.moltSite = 0;
                }
            }
            view.entities_.push_back(observed);
        }
    }
    const std::int32_t resolution =
        config_.rules.vibrationDetection.contactResolutionRaw;
    for (const Entity& source : entities_) {
        if (source.owner == kNeutralPlayer || source.owner == player ||
            source.hitPoints <= 0 || source.movementPerTickRaw <= 0 ||
            source.vibrationSignatureUntilTick <= currentTick_ ||
            IsEntityVisibleTo(player, source.id)) {
            continue;
        }
        bool detected = false;
        for (const Entity& detector : entities_) {
            if (detector.owner != player) {
                continue;
            }
            const std::int32_t radius =
                VibrationDetectionRadiusRaw(detector);
            if (radius <= 0) {
                continue;
            }
            if (DistanceSquaredRaw(detector.position, source.position) <=
                static_cast<std::uint64_t>(
                    static_cast<std::int64_t>(radius) * radius)) {
                detected = true;
                break;
            }
        }
        if (!detected) {
            continue;
        }
        const auto Quantize = [&](std::int32_t raw, std::int32_t maximumRaw) {
            const std::int64_t cell = raw / resolution;
            const std::int64_t centered =
                cell * resolution + resolution / 2;
            return static_cast<std::int32_t>(std::clamp<std::int64_t>(
                centered, 0, maximumRaw - 1));
        };
        const Vec2 approximate = Vec2::FromRaw(
            Quantize(source.position.x.Raw(),
                     config_.mapWidthTiles * kFixedScale),
            Quantize(source.position.y.Raw(),
                     config_.mapHeightTiles * kFixedScale));
        const auto duplicate = std::find_if(
            view.vibrationSignatures_.begin(),
            view.vibrationSignatures_.end(),
            [&](const VibrationSignature& signature) {
                return signature.approximatePosition == approximate;
            });
        if (duplicate == view.vibrationSignatures_.end()) {
            view.vibrationSignatures_.push_back({approximate});
        }
    }
    std::sort(
        view.vibrationSignatures_.begin(),
        view.vibrationSignatures_.end(),
        [](const VibrationSignature& lhs, const VibrationSignature& rhs) {
            return std::tie(lhs.approximatePosition.x,
                            lhs.approximatePosition.y) <
                   std::tie(rhs.approximatePosition.x,
                            rhs.approximatePosition.y);
        });
    return view;
}

std::vector<Command> Simulation::GenerateAiCommands(PlayerId player,
                                                    AiPersonality personality) const {
    const std::optional<PlayerView> view = CreatePlayerView(player);
    return view.has_value() ? GenerateAiCommands(*view, personality)
                            : std::vector<Command>{};
}

std::vector<Command> Simulation::GenerateAiCommands(
    const PlayerView& view,
    AiPersonality personality) {
    std::vector<Command> commands{};
    if (!IsValidAiPersonality(personality)) {
        return commands;
    }

    const PlayerId player = view.Player().id;
    const PlayerState* playerState = &view.Player();
    const Tick currentTick_ = view.CurrentTick();
    const SimulationConfig& config_ = view.Config();
    const std::vector<Entity>& entities_ = view.Entities();
    const auto IsProtectedCommandCore = [&](const Entity& entity) {
        return entity.type == EntityType::CommandCore &&
               entity.owner < kMaximumPlayers &&
               (config_.protectedCommandCorePlayerMask &
                static_cast<std::uint8_t>(1U << entity.owner)) != 0;
    };
    const auto PopulationUsed = [&](PlayerId) { return view.PopulationUsed(); };
    const auto PopulationCapacity = [&](PlayerId) {
        return view.PopulationCapacity();
    };
    const auto PopulationCost = [&](EntityType type) {
        return PopulationCostFor(config_.rules, playerState->faction, type);
    };
    const auto BuildCost = [&](Faction faction, EntityType type) {
        return BuildCostFor(config_.rules, faction, type);
    };
    const auto ValidatePlacement = [&](PlayerId,
                                       EntityType type,
                                       Vec2 position) {
        return ValidateViewPlacement(view, type, position);
    };
    const auto ValidateProduction = [&](PlayerId,
                                        EntityId producer,
                                        EntityType type) {
        return ValidateViewProduction(view, producer, type);
    };
    const auto VisibilityAt = [&](PlayerId, Vec2 position) {
        return view.VisibilityAt(position);
    };
    const auto IsEntityVisibleTo = [&](PlayerId, EntityId id) {
        return FindViewEntity(view, id) != nullptr;
    };
    const auto FindNearestOwnedDropoff = [&](PlayerId, Vec2 from) {
        return FindViewOwnedDropoff(view, from);
    };
    const auto DistanceSquaredRaw = [&](Vec2 first, Vec2 second) {
        return DistanceSquaredRawFor(first, second);
    };
    const auto IsPositionPassable = [&](Vec2 position) {
        return view.IsPositionPassable(position);
    };
    const auto StatelessAiValue = [&](PlayerId, EntityId entity,
                                      std::uint64_t salt) {
        return StatelessAiValueFor(view, entity, salt);
    };

    const Entity* commandCore = nullptr;
    EntityId researchProducer = 0;
    std::int32_t barracksCount = 0;
    std::int32_t dropoffCount = 0;
    std::int32_t visibleHeavyThreats = 0;
    std::int32_t visibleMobileThreats = 0;
    std::int32_t committedPopulation = PopulationUsed(player);
    for (const Entity& entity : entities_) {
        if (entity.owner != player && entity.owner != kNeutralPlayer) {
            if (entity.type == EntityType::HeavyUnit ||
                IsBuildingType(entity.type)) {
                ++visibleHeavyThreats;
            } else if (entity.type == EntityType::Soldier ||
                       entity.type == EntityType::ScoutUnit) {
                ++visibleMobileThreats;
            }
        }
        if (entity.owner != player || entity.hitPoints <= 0) {
            continue;
        }
        if (entity.type == EntityType::CommandCore && entity.completed &&
            (commandCore == nullptr || entity.id < commandCore->id)) {
            commandCore = &entity;
        }
        if (entity.type == EntityType::Barracks) {
            ++barracksCount;
            if (entity.completed && entity.productionRequired == 0 &&
                (researchProducer == 0 || entity.id < researchProducer)) {
                researchProducer = entity.id;
            }
        } else if (entity.type == EntityType::Dropoff) {
            ++dropoffCount;
        }
        if (entity.productionRequired > 0) {
            committedPopulation = SaturatingAdd(
                committedPopulation,
                PopulationCost(entity.productionType));
        }
    }

    EntityId choirReconciliationActor = 0;
    ChoirIdentityState choirReconciliationTarget =
        ChoirIdentityState::NotChoir;
    if (config_.rules.version >= 2 &&
        playerState->faction == Faction::HollowChoir &&
        playerState->resources.dawnshards >=
            config_.rules.choirIdentity.dawnCost) {
        const ChoirIdentityState desired =
            visibleHeavyThreats + visibleMobileThreats > 0
                ? ChoirIdentityState::Manifest
                : ChoirIdentityState::Possible;
        for (const Entity& candidate : entities_) {
            if (candidate.owner != player || !candidate.completed ||
                candidate.hitPoints <= 0 ||
                candidate.faction != Faction::HollowChoir ||
                !IsBarracksUnitType(candidate.type) ||
                candidate.choirIdentityState == desired ||
                candidate.choirIdentityState ==
                    ChoirIdentityState::DualResolveManifest ||
                candidate.choirIdentityState ==
                    ChoirIdentityState::DualResolvePossible ||
                currentTick_ < candidate.choirIdentityNextAvailableTick) {
                continue;
            }
            if (choirReconciliationActor == 0 ||
                candidate.id < choirReconciliationActor) {
                choirReconciliationActor = candidate.id;
                choirReconciliationTarget = desired;
            }
        }
    }

    EntityType expansionType = EntityType::Worker;
    const std::int32_t capacityHeadroom =
        PopulationCapacity(player) - committedPopulation;
    std::int32_t expansionHeadroom = 2;
    switch (personality) {
        case AiPersonality::Economic:
            expansionHeadroom = 4;
            break;
        case AiPersonality::Expansionist:
            expansionHeadroom = 6;
            break;
        case AiPersonality::Adaptive:
            expansionHeadroom = 6;
            break;
        case AiPersonality::Balanced:
        case AiPersonality::Defensive:
            expansionHeadroom = 2;
            break;
        case AiPersonality::Raider:
            expansionHeadroom = 0;
            break;
    }
    if (barracksCount == 0) {
        expansionType = EntityType::Barracks;
    } else if (capacityHeadroom <= expansionHeadroom &&
               (dropoffCount == 0 ||
                (personality == AiPersonality::Adaptive &&
                 dropoffCount < 2 && capacityHeadroom <= 6))) {
        expansionType = EntityType::Dropoff;
    }

    EntityId expansionBuilder = 0;
    Vec2 expansionPosition{};
    if (expansionType != EntityType::Worker && commandCore != nullptr &&
        ResourceCovers(playerState->resources,
                       BuildCost(playerState->faction, expansionType))) {
        for (const Entity& candidate : entities_) {
            if (candidate.owner == player && candidate.completed &&
                candidate.hitPoints > 0 && candidate.type == EntityType::Worker &&
                candidate.order.type != OrderType::Build &&
                (expansionBuilder == 0 || candidate.id < expansionBuilder)) {
                expansionBuilder = candidate.id;
            }
        }
        if (expansionBuilder != 0) {
            const std::int32_t baseX = commandCore->position.x.FloorToInt();
            const std::int32_t baseY = commandCore->position.y.FloorToInt();
            bool foundPlacement = false;
            for (std::int32_t radius = 4; radius <= 10 && !foundPlacement;
                 ++radius) {
                for (std::int32_t offsetY = -radius;
                     offsetY <= radius && !foundPlacement;
                     ++offsetY) {
                    for (std::int32_t offsetX = -radius;
                         offsetX <= radius;
                         ++offsetX) {
                        if (Abs64(offsetX) != radius && Abs64(offsetY) != radius) {
                            continue;
                        }
                        const Vec2 candidate =
                            Vec2::FromTiles(baseX + offsetX, baseY + offsetY);
                        if (VisibilityAt(player, candidate) != Visibility::Visible ||
                            ValidatePlacement(player, expansionType, candidate) !=
                                PlacementResult::Valid) {
                            continue;
                        }
                        expansionPosition = candidate;
                        foundPlacement = true;
                        break;
                    }
                }
            }
            if (!foundPlacement) {
                expansionBuilder = 0;
            }
        }
    }

    std::int32_t retreatHealthPercent = 30;
    switch (personality) {
        case AiPersonality::Defensive:
            retreatHealthPercent = 50;
            break;
        case AiPersonality::Economic:
            retreatHealthPercent = 45;
            break;
        case AiPersonality::Adaptive:
            retreatHealthPercent = 35;
            break;
        case AiPersonality::Balanced:
            retreatHealthPercent = 30;
            break;
        case AiPersonality::Expansionist:
            retreatHealthPercent = 25;
            break;
        case AiPersonality::Raider:
            retreatHealthPercent = 20;
            break;
    }

    for (const Entity& actor : entities_) {
        if (actor.owner != player || !actor.completed) {
            continue;
        }
        Command command{};
        command.executeTick = currentTick_;
        command.player = player;
        command.sequence = (currentTick_ << 32U) | actor.id;
        command.actor = actor.id;
        if (actor.id == expansionBuilder) {
            command.type = CommandType::Build;
            command.buildType = expansionType;
            command.position = expansionPosition;
            commands.push_back(command);
            continue;
        }
        if (actor.pendingWarformAdaptation != WarformAdaptation::None) {
            continue;
        }
        if (actor.id == choirReconciliationActor) {
            command.type = choirReconciliationTarget ==
                                   ChoirIdentityState::Manifest
                               ? CommandType::ReconcileToManifest
                               : CommandType::ReconcileToPossible;
            commands.push_back(command);
            continue;
        }
        if (actor.type == EntityType::CommandCore ||
            actor.type == EntityType::Barracks) {
            if (actor.id == researchProducer &&
                playerState->activeResearch == ResearchType::None) {
                for (std::size_t index = 1;
                     index < config_.rules.research.size(); ++index) {
                    const ResearchType research =
                        static_cast<ResearchType>(index);
                    const ResearchRules& rules = config_.rules.research[index];
                    if (rules.faction != playerState->faction ||
                        playerState->HasCompletedResearch(research) ||
                        (rules.prerequisite != ResearchType::None &&
                         !playerState->HasCompletedResearch(rules.prerequisite)) ||
                        !ResourceCovers(playerState->resources, rules.cost)) {
                        continue;
                    }
                    command.type = CommandType::Research;
                    command.researchType = research;
                    commands.push_back(command);
                    break;
                }
                if (command.type == CommandType::Research) {
                    continue;
                }
            }
            command.type = CommandType::Produce;
            command.buildType = actor.type == EntityType::CommandCore
                                    ? EntityType::Worker
                                    : EntityType::Soldier;
            if (ValidateProduction(player, actor.id, command.buildType) ==
                ProductionResult::Valid) {
                commands.push_back(command);
            }
            continue;
        }
        if (actor.type != EntityType::Worker &&
            !IsBarracksUnitType(actor.type)) {
            continue;
        }
        if (actor.type == EntityType::Worker) {
            if (actor.cargo > 0) {
                const EntityId dropoff = FindNearestOwnedDropoff(player, actor.position);
                if (dropoff != 0) {
                    command.type = CommandType::Deliver;
                    command.target = dropoff;
                    commands.push_back(command);
                    continue;
                }
            }
            const Entity* nearestResource = nullptr;
            std::uint64_t nearestDistance = std::numeric_limits<std::uint64_t>::max();
            for (const Entity& candidate : entities_) {
                if (candidate.type != EntityType::ResourceNode ||
                    candidate.resourceRemaining <= 0 ||
                    !IsEntityVisibleTo(player, candidate.id)) {
                    continue;
                }
                const std::uint64_t distance =
                    DistanceSquaredRaw(actor.position, candidate.position);
                if (distance < nearestDistance ||
                    (distance == nearestDistance &&
                     (nearestResource == nullptr || candidate.id < nearestResource->id))) {
                    nearestResource = &candidate;
                    nearestDistance = distance;
                }
            }
            if (nearestResource != nullptr) {
                command.type = CommandType::Gather;
                command.target = nearestResource->id;
                commands.push_back(command);
                continue;
            }
            const Entity* nearestWell = nullptr;
            nearestDistance = std::numeric_limits<std::uint64_t>::max();
            for (const Entity& candidate : entities_) {
                if (candidate.type != EntityType::FutureWell ||
                    candidate.wellChoice != FutureWellChoice::Dormant ||
                    !IsEntityVisibleTo(player, candidate.id)) {
                    continue;
                }
                const std::uint64_t distance =
                    DistanceSquaredRaw(actor.position, candidate.position);
                if (distance < nearestDistance ||
                    (distance == nearestDistance &&
                     (nearestWell == nullptr || candidate.id < nearestWell->id))) {
                    nearestWell = &candidate;
                    nearestDistance = distance;
                }
            }
            if (nearestWell != nullptr) {
                command.type = CommandType::FutureWell;
                command.target = nearestWell->id;
                command.wellChoice = personality == AiPersonality::Economic
                                         ? FutureWellChoice::Preserve
                                     : personality == AiPersonality::Raider
                                         ? FutureWellChoice::Reshape
                                         : FutureWellChoice::Harvest;
                commands.push_back(command);
                continue;
            }
        } else {
            if ((personality == AiPersonality::Adaptive ||
                 personality == AiPersonality::Defensive) &&
                playerState->faction == Faction::KharuunAssemblies &&
                actor.type == EntityType::HeavyUnit &&
                !actor.temporaryMineralCover &&
                currentTick_ >= actor.mineralCoverCooldownUntilTick &&
                playerState->resources.dawnshards >=
                    config_.rules.mineralCover.dawnCost) {
                const Entity* nearestThreat = nullptr;
                std::uint64_t nearestThreatDistance =
                    std::numeric_limits<std::uint64_t>::max();
                for (const Entity& candidate : entities_) {
                    if (candidate.owner == kNeutralPlayer ||
                        candidate.owner == player || candidate.hitPoints <= 0 ||
                        IsProtectedCommandCore(candidate) ||
                        !IsEntityVisibleTo(player, candidate.id)) {
                        continue;
                    }
                    const std::uint64_t distance =
                        DistanceSquaredRaw(actor.position, candidate.position);
                    if (distance < nearestThreatDistance ||
                        (distance == nearestThreatDistance &&
                         (nearestThreat == nullptr ||
                          candidate.id < nearestThreat->id))) {
                        nearestThreat = &candidate;
                        nearestThreatDistance = distance;
                    }
                }
                const std::int64_t responseRadius =
                    config_.rules.mineralCover.castRangeRaw + 3 * kFixedScale;
                if (nearestThreat != nullptr &&
                    nearestThreatDistance <=
                        static_cast<std::uint64_t>(responseRadius * responseRadius)) {
                    const std::int64_t deltaX =
                        static_cast<std::int64_t>(nearestThreat->position.x.Raw()) -
                        actor.position.x.Raw();
                    const std::int64_t deltaY =
                        static_cast<std::int64_t>(nearestThreat->position.y.Raw()) -
                        actor.position.y.Raw();
                    const Vec2 coverPosition = Abs64(deltaX) >= Abs64(deltaY)
                                                   ? Vec2::FromRaw(
                                                         actor.position.x.Raw() +
                                                             (deltaX >= 0 ? kFixedScale
                                                                          : -kFixedScale),
                                                         actor.position.y.Raw())
                                                   : Vec2::FromRaw(
                                                         actor.position.x.Raw(),
                                                         actor.position.y.Raw() +
                                                             (deltaY >= 0 ? kFixedScale
                                                                          : -kFixedScale));
                    bool occupied = !IsPositionPassable(coverPosition) ||
                                    VisibilityAt(player, coverPosition) !=
                                        Visibility::Visible;
                    for (const Entity& candidate : entities_) {
                        const std::int32_t candidateExtent =
                            candidate.temporaryMineralCover
                                ? config_.rules.mineralCover.halfExtentRaw
                                : FootprintHalfExtentFor(
                                      config_.rules,
                                      candidate.faction,
                                      candidate.type);
                        const std::int32_t combinedExtent =
                            config_.rules.mineralCover.halfExtentRaw +
                            candidateExtent;
                        if (candidate.hitPoints > 0 &&
                            Abs64(static_cast<std::int64_t>(
                                      coverPosition.x.Raw()) -
                                  candidate.position.x.Raw()) < combinedExtent &&
                            Abs64(static_cast<std::int64_t>(
                                      coverPosition.y.Raw()) -
                                  candidate.position.y.Raw()) < combinedExtent) {
                            occupied = true;
                            break;
                        }
                    }
                    if (!occupied) {
                        command.type = CommandType::RaiseMineralCover;
                        command.position = coverPosition;
                        commands.push_back(command);
                        continue;
                    }
                }
            }
            if (personality == AiPersonality::Adaptive &&
                playerState->faction == Faction::KharuunAssemblies &&
                actor.pendingWarformAdaptation == WarformAdaptation::None &&
                visibleHeavyThreats + visibleMobileThreats > 0 &&
                playerState->resources.dawnshards >=
                    config_.rules.warformAdaptation.dawnCost) {
                const WarformAdaptation desired =
                    visibleHeavyThreats >= visibleMobileThreats
                        ? WarformAdaptation::Carapace
                        : WarformAdaptation::Striker;
                if (actor.warformAdaptation != desired) {
                    const Entity* nearestBasin = nullptr;
                    std::uint64_t nearestBasinDistance =
                        std::numeric_limits<std::uint64_t>::max();
                    const std::int64_t radius =
                        config_.rules.warformAdaptation.siteRadiusRaw;
                    const std::uint64_t radiusSquared =
                        static_cast<std::uint64_t>(radius * radius);
                    for (const Entity& candidate : entities_) {
                        if (candidate.owner != player || !candidate.completed ||
                            candidate.hitPoints <= 0 ||
                            candidate.faction != Faction::KharuunAssemblies ||
                            candidate.type != EntityType::Barracks) {
                            continue;
                        }
                        const std::uint64_t distance =
                            DistanceSquaredRaw(actor.position, candidate.position);
                        if (distance <= radiusSquared &&
                            (distance < nearestBasinDistance ||
                             (distance == nearestBasinDistance &&
                              (nearestBasin == nullptr ||
                               candidate.id < nearestBasin->id)))) {
                            nearestBasin = &candidate;
                            nearestBasinDistance = distance;
                        }
                    }
                    if (nearestBasin != nullptr) {
                        command.type = CommandType::AdaptWarform;
                        command.target = nearestBasin->id;
                        command.warformAdaptation = desired;
                        commands.push_back(command);
                        continue;
                    }
                }
            }
            const bool shouldRetreat =
                commandCore != nullptr && actor.maxHitPoints > 0 &&
                static_cast<std::int64_t>(actor.hitPoints) * 100 <=
                    static_cast<std::int64_t>(actor.maxHitPoints) *
                        retreatHealthPercent;
            if (shouldRetreat) {
                const std::uint64_t distanceToCore =
                    DistanceSquaredRaw(actor.position, commandCore->position);
                const std::uint64_t holdDistance =
                    static_cast<std::uint64_t>(3 * kFixedScale) *
                    (3 * kFixedScale);
                if (distanceToCore <= holdDistance) {
                    command.type = CommandType::Hold;
                } else {
                    constexpr std::array<std::pair<std::int32_t, std::int32_t>, 8>
                        rallyOffsets{{
                            {-3, 0}, {0, -3}, {3, 0}, {0, 3},
                            {-3, -3}, {3, -3}, {3, 3}, {-3, 3},
                        }};
                    const std::size_t firstOffset =
                        static_cast<std::size_t>(actor.id) % rallyOffsets.size();
                    command.type = CommandType::Move;
                    command.position = commandCore->position;
                    for (std::size_t offsetIndex = 0;
                         offsetIndex < rallyOffsets.size();
                         ++offsetIndex) {
                        const auto& offset = rallyOffsets[
                            (firstOffset + offsetIndex) % rallyOffsets.size()];
                        const Vec2 candidate = Vec2::FromTiles(
                            commandCore->position.x.FloorToInt() + offset.first,
                            commandCore->position.y.FloorToInt() + offset.second);
                        if (IsPositionPassable(candidate)) {
                            command.position = candidate;
                            break;
                        }
                    }
                }
                commands.push_back(command);
                continue;
            }
            const Entity* nearestEnemy = nullptr;
            std::uint64_t nearestDistance = std::numeric_limits<std::uint64_t>::max();
            for (const Entity& candidate : entities_) {
                if (candidate.owner == kNeutralPlayer || candidate.owner == player ||
                    candidate.hitPoints <= 0 ||
                    IsProtectedCommandCore(candidate) ||
                    !IsEntityVisibleTo(player, candidate.id)) {
                    continue;
                }
                const std::uint64_t distance =
                    DistanceSquaredRaw(actor.position, candidate.position);
                if (distance < nearestDistance ||
                    (distance == nearestDistance &&
                     (nearestEnemy == nullptr || candidate.id < nearestEnemy->id))) {
                    nearestEnemy = &candidate;
                    nearestDistance = distance;
                }
            }
            if (nearestEnemy != nullptr) {
                command.type = CommandType::Attack;
                command.target = nearestEnemy->id;
                commands.push_back(command);
                continue;
            }
            const VibrationSignature* nearestSignature = nullptr;
            std::uint64_t nearestSignatureDistance =
                std::numeric_limits<std::uint64_t>::max();
            for (const VibrationSignature& signature :
                 view.VibrationSignatures()) {
                const std::uint64_t distance = DistanceSquaredRaw(
                    actor.position, signature.approximatePosition);
                if (distance < nearestSignatureDistance ||
                    (distance == nearestSignatureDistance &&
                     (nearestSignature == nullptr ||
                      std::tie(signature.approximatePosition.x,
                               signature.approximatePosition.y) <
                          std::tie(nearestSignature->approximatePosition.x,
                                   nearestSignature->approximatePosition.y)))) {
                    nearestSignature = &signature;
                    nearestSignatureDistance = distance;
                }
            }
            const bool adaptiveOpeningPosture =
                personality == AiPersonality::Adaptive &&
                commandCore != nullptr &&
                currentTick_ < kAdaptiveOpeningPostureTicks;
            if (nearestSignature != nullptr && !adaptiveOpeningPosture) {
                command.type = CommandType::AttackMove;
                command.position = nearestSignature->approximatePosition;
                commands.push_back(command);
                continue;
            }
            if (adaptiveOpeningPosture) {
                const std::uint64_t distanceToCore =
                    DistanceSquaredRaw(actor.position, commandCore->position);
                const std::uint64_t holdDistance =
                    static_cast<std::uint64_t>(9 * kFixedScale) *
                    (9 * kFixedScale);
                command.type = distanceToCore > holdDistance
                                   ? CommandType::Move
                                   : CommandType::Hold;
                command.position = commandCore->position;
                commands.push_back(command);
                continue;
            }
            if (personality == AiPersonality::Defensive ||
                personality == AiPersonality::Economic) {
                const auto base = std::find_if(
                    entities_.begin(), entities_.end(), [&](const Entity& candidate) {
                        return candidate.owner == player && candidate.completed &&
                               candidate.type == EntityType::CommandCore;
                    });
                if (base != entities_.end() &&
                    DistanceSquaredRaw(actor.position, base->position) >
                        static_cast<std::uint64_t>(9 * kFixedScale * kFixedScale)) {
                    command.type = CommandType::Move;
                    command.position = base->position;
                    commands.push_back(command);
                    continue;
                }
            }
        }
        const std::uint64_t random = StatelessAiValue(player, actor.id, 0xa17eULL);
        const std::int32_t usableWidth = std::max(1, config_.mapWidthTiles - 2);
        const std::int32_t usableHeight = std::max(1, config_.mapHeightTiles - 2);
        command.type = CommandType::Move;
        command.position = Vec2::FromTiles(
            std::min(config_.mapWidthTiles - 1,
                     1 + static_cast<std::int32_t>(random % usableWidth)),
            std::min(config_.mapHeightTiles - 1,
                     1 + static_cast<std::int32_t>((random >> 32U) % usableHeight)));
        commands.push_back(command);
    }
    return commands;
}

template <typename Writer>
void Simulation::WriteSnapshotPayload(Writer& writer) const {
    writer.U8('E');
    writer.U8('B');
    writer.U8('S');
    writer.U8('N');
    writer.U32(kSnapshotVersion);
    writer.I32(config_.mapWidthTiles);
    writer.I32(config_.mapHeightTiles);
    writer.U32(config_.ticksPerSecond);
    writer.U64(config_.randomSeed);
    writer.U8(config_.protectedCommandCorePlayerMask);
    writer.U32(config_.rules.version);
    writer.Bytes(config_.rules.contentSha256);
    for (const auto& faction : config_.rules.archetypes) {
        for (const EntityArchetypeRules& archetype : faction) {
            writer.I32(archetype.cost.material);
            writer.I32(archetype.cost.dawnshards);
            writer.I32(archetype.maxHitPoints);
            writer.I32(archetype.movementPerTickRaw);
            writer.I32(archetype.visionTiles);
            writer.I32(archetype.attackRangeRaw);
            writer.I32(archetype.attackDamage);
            writer.U64(archetype.attackPeriodTicks);
            writer.I32(archetype.workRate);
            writer.I32(archetype.cargoCapacity);
            writer.I32(archetype.constructionRequired);
            writer.I32(archetype.populationCost);
            writer.I32(archetype.populationCapacity);
            writer.I32(archetype.productionTicks);
            writer.I32(archetype.footprintHalfExtentRaw);
        }
    }
    writer.I32(config_.rules.futureWell.harvestImmediateDawn);
    writer.I32(config_.rules.futureWell.preserveDawnPerInterval);
    writer.U64(config_.rules.futureWell.preserveIntervalTicks);
    writer.I32(config_.rules.futureWell.preserveVisionTiles);
    writer.I32(config_.rules.futureWell.reshapeDawnCost);
    writer.U64(config_.rules.futureWell.reshapeDurationMinimumTicks);
    writer.U64(config_.rules.futureWell.reshapeDurationMaximumTicks);
    writer.I32(config_.rules.bulwarkDeployment.coverDepthRaw);
    writer.I32(config_.rules.bulwarkDeployment.coverHalfWidthRaw);
    writer.I32(config_.rules.bulwarkDeployment.damageReductionPercent);
    writer.I32(config_.rules.bulwarkDeployment.deployedMovementPercent);
    writer.I32(config_.rules.relaySupply.connectionRadiusRaw);
    writer.I32(config_.rules.relaySupply.capacityBonus);
    writer.U64(config_.rules.relaySupply.durationTicks);
    writer.U64(config_.rules.relaySupply.cooldownTicks);
    writer.I32(config_.rules.waystoneMigration.movementPerTickRaw);
    writer.U64(config_.rules.waystoneMigration.uprootTicks);
    writer.U64(config_.rules.waystoneMigration.rootTicks);
    writer.I32(config_.rules.waystoneMigration.mobileDamageTakenPercent);
    writer.I32(config_.rules.warformAdaptation.siteRadiusRaw);
    writer.U64(config_.rules.warformAdaptation.moltTicks);
    writer.I32(config_.rules.warformAdaptation.dawnCost);
    writer.I32(config_.rules.warformAdaptation.moltDamageTakenPercent);
    writer.I32(config_.rules.warformAdaptation.carapaceHealthPercent);
    writer.I32(config_.rules.warformAdaptation.carapaceMovementPercent);
    writer.I32(config_.rules.warformAdaptation.strikerDamagePercent);
    writer.I32(config_.rules.warformAdaptation.strikerCooldownPercent);
    writer.I32(config_.rules.mineralCover.castRangeRaw);
    writer.U64(config_.rules.mineralCover.durationTicks);
    writer.U64(config_.rules.mineralCover.cooldownTicks);
    writer.I32(config_.rules.mineralCover.dawnCost);
    writer.I32(config_.rules.mineralCover.maxHitPoints);
    writer.I32(config_.rules.mineralCover.halfExtentRaw);
    writer.I32(config_.rules.vibrationDetection.resonantRadiusRaw);
    writer.I32(config_.rules.vibrationDetection.listeningSpineRadiusRaw);
    writer.U64(config_.rules.vibrationDetection.signatureLingerTicks);
    writer.I32(config_.rules.vibrationDetection.contactResolutionRaw);
    writer.I32(config_.rules.poweredAegis.connectionRadiusRaw);
    writer.U64(config_.rules.choirIdentity.durationTicks);
    writer.U64(config_.rules.choirIdentity.cooldownTicks);
    writer.I32(config_.rules.choirIdentity.dawnCost);
    writer.I32(config_.rules.choirIdentity.manifestDamagePercent);
    writer.I32(config_.rules.choirIdentity.possibleMovementPercent);
    writer.I32(config_.rules.choirIdentity.possibleVisionPercent);
    writer.U64(config_.rules.choirCoherence.upkeepIntervalTicks);
    writer.I32(config_.rules.choirCoherence.dawnCostPerStructure);
    for (const ResearchRules& research : config_.rules.research) {
        writer.U8(static_cast<std::uint8_t>(research.faction));
        writer.I32(research.cost.material);
        writer.I32(research.cost.dawnshards);
        writer.U64(research.researchTicks);
        writer.U8(static_cast<std::uint8_t>(research.prerequisite));
        writer.I32(research.combatDamagePercent);
        writer.I32(research.combatVisionPercent);
    }
    writer.U64(currentTick_);
    writer.U32(nextEntityId_);
    writer.U64(rng_.state);
    for (const PlayerState& player : players_) {
        writer.U8(player.active ? 1 : 0);
        writer.U8(player.id);
        writer.U8(static_cast<std::uint8_t>(player.faction));
        writer.I32(player.resources.material);
        writer.I32(player.resources.dawnshards);
        writer.U32(player.completedResearchMask);
        writer.U8(static_cast<std::uint8_t>(player.activeResearch));
        writer.U32(player.researchProducer);
        writer.I32(player.researchProgress);
        writer.I32(player.researchRequired);
        writer.U8(static_cast<std::uint8_t>(player.lastInterruptedResearch));
    }
    for (PlayerId player = 0; player < players_.size(); ++player) {
        writer.U8(hasExecutedSequence_[player] ? 1 : 0);
        writer.U64(lastExecutedSequence_[player]);
    }
    writer.U32(static_cast<std::uint32_t>(terrain_.size()));
    writer.Bytes(std::span<const std::uint8_t>(
        reinterpret_cast<const std::uint8_t*>(terrain_.data()),
        terrain_.size()));
    for (const auto& explored : explored_) {
        writer.U32(static_cast<std::uint32_t>(explored.size()));
        writer.Bytes(explored);
    }
    writer.U32(static_cast<std::uint32_t>(entities_.size()));
    for (const Entity& entity : entities_) {
        writer.U32(entity.id);
        writer.U8(entity.owner);
        writer.U8(static_cast<std::uint8_t>(entity.faction));
        writer.U8(static_cast<std::uint8_t>(entity.type));
        writer.I32(entity.position.x.Raw());
        writer.I32(entity.position.y.Raw());
        writer.I32(entity.hitPoints);
        writer.I32(entity.maxHitPoints);
        writer.I32(entity.movementPerTickRaw);
        writer.I32(entity.visionTiles);
        writer.I32(entity.attackRangeRaw);
        writer.I32(entity.attackDamage);
        writer.U64(entity.attackPeriodTicks);
        writer.U64(entity.attackCooldownTicks);
        writer.I32(entity.workRate);
        writer.I32(entity.cargo);
        writer.I32(entity.cargoCapacity);
        writer.I32(entity.resourceRemaining);
        writer.U8(entity.completed ? 1 : 0);
        writer.I32(entity.constructionProgress);
        writer.I32(entity.constructionRequired);
        writer.U8(static_cast<std::uint8_t>(entity.order.type));
        writer.U32(entity.order.target);
        writer.I32(entity.order.anchor.x.Raw());
        writer.I32(entity.order.anchor.y.Raw());
        writer.I32(entity.order.destination.x.Raw());
        writer.I32(entity.order.destination.y.Raw());
        writer.U8(static_cast<std::uint8_t>(entity.order.buildType));
        writer.U8(static_cast<std::uint8_t>(entity.order.wellChoice));
        writer.U8(static_cast<std::uint8_t>(entity.wellChoice));
        writer.U64(entity.wellActivationTick);
        writer.U64(entity.reshapeUntilTick);
        writer.U8(entity.reshapeVariant);
        writer.U8(static_cast<std::uint8_t>(entity.productionType));
        writer.I32(entity.productionProgress);
        writer.I32(entity.productionRequired);
        writer.U8(entity.deployed ? 1 : 0);
        writer.I32(entity.deploymentFacing.x.Raw());
        writer.I32(entity.deploymentFacing.y.Raw());
        writer.U8(entity.relaySupplyActive ? 1 : 0);
        writer.U64(entity.relaySupplyUntilTick);
        writer.U64(entity.relaySupplyCooldownUntilTick);
        writer.U8(static_cast<std::uint8_t>(entity.waystoneMode));
        writer.U64(entity.waystoneTransitionUntilTick);
        writer.U8(static_cast<std::uint8_t>(entity.warformAdaptation));
        writer.U8(static_cast<std::uint8_t>(entity.pendingWarformAdaptation));
        writer.U32(entity.moltSite);
        writer.U64(entity.moltUntilTick);
        writer.U64(entity.mineralCoverCooldownUntilTick);
        writer.U8(entity.temporaryMineralCover ? 1 : 0);
        writer.U32(entity.mineralCoverCreator);
        writer.U64(entity.mineralCoverUntilTick);
        writer.U8(static_cast<std::uint8_t>(
            entity.mineralCoverUnderlyingTerrain));
        writer.U64(entity.vibrationSignatureUntilTick);
        writer.U8(entity.aegisPowered ? 1 : 0);
        writer.U8(static_cast<std::uint8_t>(entity.choirIdentityState));
        writer.U64(entity.choirIdentityResolveAtTick);
        writer.U64(entity.choirIdentityNextAvailableTick);
        writer.U64(entity.choirCoherenceNextChargeTick);
    }
    std::vector<Command> pending = pendingCommands_;
    std::sort(pending.begin(), pending.end(), CommandLess);
    writer.U32(static_cast<std::uint32_t>(pending.size()));
    for (const Command& command : pending) {
        WriteCommand(writer, command);
    }
    std::vector<StoredCommandResolutionReceipt> receipts(
        commandResolutionReceipts_.begin(),
        commandResolutionReceipts_.end());
    std::sort(
        receipts.begin(), receipts.end(),
        [](const StoredCommandResolutionReceipt& lhs,
           const StoredCommandResolutionReceipt& rhs) {
            return std::tie(lhs.receipt.assignedExecutionTick,
                            lhs.receipt.player,
                            lhs.sequence) <
                   std::tie(rhs.receipt.assignedExecutionTick,
                            rhs.receipt.player,
                            rhs.sequence);
        });
    writer.U32(static_cast<std::uint32_t>(receipts.size()));
    for (const StoredCommandResolutionReceipt& stored : receipts) {
        writer.U8(stored.receipt.player);
        writer.U64(stored.sequence);
        writer.U8(static_cast<std::uint8_t>(stored.receipt.commandType));
        writer.U64(stored.receipt.assignedExecutionTick);
        writer.U8(static_cast<std::uint8_t>(stored.receipt.outcome));
    }
}

std::vector<std::uint8_t> Simulation::SaveSnapshot() const {
    BinaryWriter writer{};
    writer.Reserve(
        1536U + terrain_.size() * (1U + explored_.size()) +
        entities_.size() * kSerializedEntityBytes +
        pendingCommands_.size() * kSerializedCommandBytes + 4U +
        commandResolutionReceipts_.size() *
            kSerializedCommandResolutionReceiptBytes);
    WriteSnapshotPayload(writer);
    const std::uint64_t integrity = Fnv1a(writer.Data());
    writer.U64(integrity);
    return writer.Take();
}

std::uint64_t Simulation::StateChecksum() const {
    HashWriter writer{};
    WriteSnapshotPayload(writer);
    return writer.Value();
}

std::optional<Simulation> Simulation::LoadSnapshot(
    std::span<const std::uint8_t> bytes,
    std::string* error) {
    if (error != nullptr) {
        error->clear();
    }
    if (bytes.size() < 12) {
        SetError(error, "snapshot is truncated");
        return std::nullopt;
    }
    std::uint64_t expectedIntegrity = 0;
    for (std::uint32_t shift = 0; shift < 64; shift += 8) {
        expectedIntegrity |= static_cast<std::uint64_t>(
                                 bytes[bytes.size() - 8 + shift / 8])
                             << shift;
    }
    const std::span<const std::uint8_t> payload = bytes.first(bytes.size() - 8);
    if (Fnv1a(payload) != expectedIntegrity) {
        SetError(error, "snapshot integrity check failed");
        return std::nullopt;
    }
    BinaryReader reader(payload);
    std::array<std::uint8_t, 4> magic{};
    if (!reader.Bytes(magic) ||
        magic != std::array<std::uint8_t, 4>{'E', 'B', 'S', 'N'}) {
        SetError(error, "snapshot magic is invalid");
        return std::nullopt;
    }
    std::uint32_t version = 0;
    SimulationConfig config{};
    config.rules = SimulationRules{};
    if (!reader.U32(version)) {
        SetError(error, "snapshot header is truncated");
        return std::nullopt;
    }
    if (version != kSnapshotVersion &&
        version != kProtectedCommandCoreSnapshotVersion &&
        version != kChoirSnapshotVersion &&
        version != kPriorSnapshotVersion && version != kLegacySnapshotVersion) {
        SetError(error, "snapshot version is unsupported");
        return std::nullopt;
    }
    if (!reader.I32(config.mapWidthTiles) || !reader.I32(config.mapHeightTiles) ||
        !reader.U32(config.ticksPerSecond) || !reader.U64(config.randomSeed)) {
        SetError(error, "snapshot header is truncated");
        return std::nullopt;
    }
    if (HasProtectedCommandCoreSnapshotSchema(version)) {
        if (!reader.U8(config.protectedCommandCorePlayerMask)) {
            SetError(error, "snapshot protection mask is truncated");
            return std::nullopt;
        }
    } else {
        config.protectedCommandCorePlayerMask = 0;
    }
    if ((config.protectedCommandCorePlayerMask &
         static_cast<std::uint8_t>(~kValidCommandCoreProtectionMask)) != 0) {
        SetError(error, "snapshot protection mask is invalid");
        return std::nullopt;
    }
    if (!reader.U32(config.rules.version) ||
        !reader.Bytes(config.rules.contentSha256)) {
        SetError(error, "snapshot rules header is truncated");
        return std::nullopt;
    }
    const std::size_t serializedFactionCount =
        HasChoirSnapshotSchema(version) ? kFactionCount : kLegacyFactionCount;
    for (std::size_t factionIndex = 0;
         factionIndex < serializedFactionCount;
         ++factionIndex) {
        for (EntityArchetypeRules& archetype :
             config.rules.archetypes[factionIndex]) {
            if (!reader.I32(archetype.cost.material) ||
                !reader.I32(archetype.cost.dawnshards) ||
                !reader.I32(archetype.maxHitPoints) ||
                !reader.I32(archetype.movementPerTickRaw) ||
                !reader.I32(archetype.visionTiles) ||
                !reader.I32(archetype.attackRangeRaw) ||
                !reader.I32(archetype.attackDamage) ||
                !reader.U64(archetype.attackPeriodTicks) ||
                !reader.I32(archetype.workRate) ||
                !reader.I32(archetype.cargoCapacity) ||
                !reader.I32(archetype.constructionRequired) ||
                !reader.I32(archetype.populationCost) ||
                !reader.I32(archetype.populationCapacity) ||
                !reader.I32(archetype.productionTicks) ||
                !reader.I32(archetype.footprintHalfExtentRaw)) {
                SetError(error, "snapshot archetype rules are truncated");
                return std::nullopt;
            }
        }
    }
    if (!reader.I32(config.rules.futureWell.harvestImmediateDawn) ||
        !reader.I32(config.rules.futureWell.preserveDawnPerInterval) ||
        !reader.U64(config.rules.futureWell.preserveIntervalTicks) ||
        !reader.I32(config.rules.futureWell.preserveVisionTiles) ||
        !reader.I32(config.rules.futureWell.reshapeDawnCost) ||
        !reader.U64(config.rules.futureWell.reshapeDurationMinimumTicks) ||
        !reader.U64(config.rules.futureWell.reshapeDurationMaximumTicks) ||
        !reader.I32(config.rules.bulwarkDeployment.coverDepthRaw) ||
        !reader.I32(config.rules.bulwarkDeployment.coverHalfWidthRaw) ||
        !reader.I32(config.rules.bulwarkDeployment.damageReductionPercent) ||
        !reader.I32(config.rules.bulwarkDeployment.deployedMovementPercent) ||
        !reader.I32(config.rules.relaySupply.connectionRadiusRaw) ||
        !reader.I32(config.rules.relaySupply.capacityBonus) ||
        !reader.U64(config.rules.relaySupply.durationTicks) ||
        !reader.U64(config.rules.relaySupply.cooldownTicks) ||
        !reader.I32(config.rules.waystoneMigration.movementPerTickRaw) ||
        !reader.U64(config.rules.waystoneMigration.uprootTicks) ||
        !reader.U64(config.rules.waystoneMigration.rootTicks) ||
        !reader.I32(config.rules.waystoneMigration.mobileDamageTakenPercent) ||
        !reader.I32(config.rules.warformAdaptation.siteRadiusRaw) ||
        !reader.U64(config.rules.warformAdaptation.moltTicks) ||
        !reader.I32(config.rules.warformAdaptation.dawnCost) ||
        !reader.I32(config.rules.warformAdaptation.moltDamageTakenPercent) ||
        !reader.I32(config.rules.warformAdaptation.carapaceHealthPercent) ||
        !reader.I32(config.rules.warformAdaptation.carapaceMovementPercent) ||
        !reader.I32(config.rules.warformAdaptation.strikerDamagePercent) ||
        !reader.I32(config.rules.warformAdaptation.strikerCooldownPercent) ||
        !reader.I32(config.rules.mineralCover.castRangeRaw) ||
        !reader.U64(config.rules.mineralCover.durationTicks) ||
        !reader.U64(config.rules.mineralCover.cooldownTicks) ||
        !reader.I32(config.rules.mineralCover.dawnCost) ||
        !reader.I32(config.rules.mineralCover.maxHitPoints) ||
        !reader.I32(config.rules.mineralCover.halfExtentRaw) ||
        !reader.I32(config.rules.vibrationDetection.resonantRadiusRaw) ||
        !reader.I32(config.rules.vibrationDetection.listeningSpineRadiusRaw) ||
        !reader.U64(config.rules.vibrationDetection.signatureLingerTicks) ||
        !reader.I32(config.rules.vibrationDetection.contactResolutionRaw) ||
        !reader.I32(config.rules.poweredAegis.connectionRadiusRaw)) {
        SetError(error, "snapshot authored rules are truncated");
        return std::nullopt;
    }
    if (HasChoirSnapshotSchema(version) &&
        (!reader.U64(config.rules.choirIdentity.durationTicks) ||
         !reader.U64(config.rules.choirIdentity.cooldownTicks) ||
         !reader.I32(config.rules.choirIdentity.dawnCost) ||
         !reader.I32(config.rules.choirIdentity.manifestDamagePercent) ||
         !reader.I32(config.rules.choirIdentity.possibleMovementPercent) ||
         !reader.I32(config.rules.choirIdentity.possibleVisionPercent) ||
         !reader.U64(config.rules.choirCoherence.upkeepIntervalTicks) ||
         !reader.I32(config.rules.choirCoherence.dawnCostPerStructure))) {
        SetError(error, "snapshot Hollow Choir rules are truncated");
        return std::nullopt;
    }
    const std::size_t serializedResearchCount =
        HasChoirSnapshotSchema(version) ? kResearchTypeCount
                                        : kLegacyResearchTypeCount;
    for (std::size_t researchIndex = 0;
         researchIndex < serializedResearchCount;
         ++researchIndex) {
        ResearchRules& research = config.rules.research[researchIndex];
        std::uint8_t faction = 0;
        std::uint8_t prerequisite = 0;
        if (!reader.U8(faction) || !reader.I32(research.cost.material) ||
            !reader.I32(research.cost.dawnshards) ||
            !reader.U64(research.researchTicks) || !reader.U8(prerequisite) ||
            !reader.I32(research.combatDamagePercent) ||
            !reader.I32(research.combatVisionPercent) ||
            faction > static_cast<std::uint8_t>(
                HasChoirSnapshotSchema(version) ? Faction::HollowChoir
                                                : Faction::KharuunAssemblies) ||
            prerequisite > static_cast<std::uint8_t>(
                HasChoirSnapshotSchema(version)
                    ? ResearchType::ChoirSharedResolution
                    : ResearchType::KharuunAncestralEdge)) {
            SetError(error, "snapshot research rules are invalid");
            return std::nullopt;
        }
        research.faction = static_cast<Faction>(faction);
        research.prerequisite = static_cast<ResearchType>(prerequisite);
    }
    const std::int64_t tileCount =
        static_cast<std::int64_t>(config.mapWidthTiles) * config.mapHeightTiles;
    if (config.mapWidthTiles <= 0 || config.mapHeightTiles <= 0 ||
        config.mapWidthTiles > kMaximumMapDimension ||
        config.mapHeightTiles > kMaximumMapDimension ||
        config.ticksPerSecond == 0 ||
        config.ticksPerSecond > kMaximumTicksPerSecond || tileCount <= 0 ||
        tileCount > kMaximumMapTiles || !IsValidSimulationRules(config.rules)) {
        SetError(error, "snapshot map configuration is invalid");
        return std::nullopt;
    }
    const std::size_t minimumRemaining =
        kSnapshotFixedBytesAfterConfig +
        static_cast<std::size_t>(tileCount) * (1U + kMaximumPlayers);
    if (reader.Remaining() < minimumRemaining) {
        SetError(error, "snapshot payload is too short for its declared map");
        return std::nullopt;
    }

    Simulation simulation(config);
    if (!reader.U64(simulation.currentTick_) ||
        !reader.U32(simulation.nextEntityId_) ||
        !reader.U64(simulation.rng_.state) ||
        simulation.currentTick_ > kMaximumSupportedTick ||
        simulation.nextEntityId_ == 0) {
        SetError(error, "snapshot state header is truncated");
        return std::nullopt;
    }
    for (PlayerId index = 0; index < simulation.players_.size(); ++index) {
        std::uint8_t active = 0;
        std::uint8_t id = 0;
        std::uint8_t faction = 0;
        std::uint8_t activeResearch = 0;
        std::uint8_t lastInterruptedResearch = 0;
        PlayerState player{};
        if (!reader.U8(active) || !reader.U8(id) || !reader.U8(faction) ||
            !reader.I32(player.resources.material) ||
            !reader.I32(player.resources.dawnshards) ||
            !reader.U32(player.completedResearchMask) ||
            !reader.U8(activeResearch) || !reader.U32(player.researchProducer) ||
            !reader.I32(player.researchProgress) ||
            !reader.I32(player.researchRequired) ||
            !reader.U8(lastInterruptedResearch) || id != index || active > 1 ||
            faction > static_cast<std::uint8_t>(
                HasChoirSnapshotSchema(version) ? Faction::HollowChoir
                                                : Faction::KharuunAssemblies) ||
            activeResearch > static_cast<std::uint8_t>(
                HasChoirSnapshotSchema(version)
                    ? ResearchType::ChoirSharedResolution
                    : ResearchType::KharuunAncestralEdge) ||
            lastInterruptedResearch > static_cast<std::uint8_t>(
                HasChoirSnapshotSchema(version)
                    ? ResearchType::ChoirSharedResolution
                    : ResearchType::KharuunAncestralEdge) ||
            (player.completedResearchMask &
             ~(HasChoirSnapshotSchema(version) ? 0x7eU : 0x1eU)) != 0 ||
            (active != 0 &&
             faction == static_cast<std::uint8_t>(Faction::HollowChoir) &&
             config.rules.version < 2) ||
            player.resources.material < 0 || player.resources.dawnshards < 0 ||
            player.researchProgress < 0 || player.researchRequired < 0 ||
            player.researchRequired > kMaximumProductionTicks ||
            player.researchProgress > player.researchRequired ||
            ((activeResearch == 0) !=
             (player.researchProducer == 0 && player.researchProgress == 0 &&
              player.researchRequired == 0)) ||
            (activeResearch != 0 && lastInterruptedResearch != 0) ||
            (lastInterruptedResearch != 0 &&
             ((player.completedResearchMask &
               (1U << lastInterruptedResearch)) != 0 ||
              config.rules.research[lastInterruptedResearch].faction !=
                  static_cast<Faction>(faction)))) {
            SetError(error, "snapshot player state is invalid");
            return std::nullopt;
        }
        player.id = id;
        player.active = active != 0;
        player.faction = static_cast<Faction>(faction);
        player.activeResearch = static_cast<ResearchType>(activeResearch);
        player.lastInterruptedResearch =
            static_cast<ResearchType>(lastInterruptedResearch);
        simulation.players_[index] = player;
    }
    for (PlayerId player = 0; player < simulation.players_.size(); ++player) {
        std::uint8_t hasSequence = 0;
        if (!reader.U8(hasSequence) ||
            !reader.U64(simulation.lastExecutedSequence_[player]) ||
            hasSequence > 1 ||
            (hasSequence != 0 && !simulation.players_[player].active)) {
            SetError(error, "snapshot command sequence state is invalid");
            return std::nullopt;
        }
        simulation.hasExecutedSequence_[player] = hasSequence != 0;
    }
    std::uint32_t serializedTileCount = 0;
    if (!reader.U32(serializedTileCount) ||
        static_cast<std::int64_t>(serializedTileCount) != tileCount) {
        SetError(error, "snapshot terrain dimensions do not match the map");
        return std::nullopt;
    }
    for (Terrain& terrain : simulation.terrain_) {
        std::uint8_t encoded = 0;
        if (!reader.U8(encoded) ||
            encoded > static_cast<std::uint8_t>(Terrain::Scarred)) {
            SetError(error, "snapshot terrain contains an invalid value");
            return std::nullopt;
        }
        terrain = static_cast<Terrain>(encoded);
    }
    for (auto& explored : simulation.explored_) {
        std::uint32_t count = 0;
        if (!reader.U32(count) || count != serializedTileCount) {
            SetError(error, "snapshot fog dimensions do not match the map");
            return std::nullopt;
        }
        if (!reader.Bytes(explored) ||
            std::any_of(explored.begin(), explored.end(),
                        [](std::uint8_t value) { return value > 1; })) {
            SetError(error, "snapshot fog state is invalid");
            return std::nullopt;
        }
    }
    const std::size_t serializedEntityBytes =
        version == kLegacySnapshotVersion
            ? kLegacySerializedEntityBytes
            : version == kPriorSnapshotVersion
                  ? kPriorSerializedEntityBytes
                  : kSerializedEntityBytes;
    std::uint32_t entityCount = 0;
    if (!reader.U32(entityCount) || entityCount > kMaximumSerializedEntities ||
        static_cast<std::size_t>(entityCount) >
            reader.Remaining() / serializedEntityBytes) {
        SetError(error, "snapshot entity count is invalid");
        return std::nullopt;
    }
    simulation.entities_.clear();
    simulation.entities_.reserve(entityCount);
    EntityId priorId = 0;
    for (std::uint32_t index = 0; index < entityCount; ++index) {
        Entity entity{};
        std::uint8_t faction = 0;
        std::uint8_t type = 0;
        std::uint8_t completed = 0;
        std::uint8_t orderType = 0;
        std::uint8_t orderBuildType = 0;
        std::uint8_t orderWellChoice = 0;
        std::uint8_t wellChoice = 0;
        std::uint8_t productionType = 0;
        std::uint8_t deployed = 0;
        std::uint8_t relaySupplyActive = 0;
        std::uint8_t waystoneMode = 0;
        std::uint8_t warformAdaptation = 0;
        std::uint8_t pendingWarformAdaptation = 0;
        std::uint8_t temporaryMineralCover = 0;
        std::uint8_t mineralCoverUnderlyingTerrain = 0;
        std::uint8_t aegisPowered = 0;
        std::uint8_t choirIdentityState = 0;
        std::int32_t rawX = 0;
        std::int32_t rawY = 0;
        std::int32_t orderAnchorRawX = 0;
        std::int32_t orderAnchorRawY = 0;
        std::int32_t orderRawX = 0;
        std::int32_t orderRawY = 0;
        std::int32_t deploymentFacingRawX = 0;
        std::int32_t deploymentFacingRawY = 0;
        if (!reader.U32(entity.id) || !reader.U8(entity.owner) ||
            !reader.U8(faction) || !reader.U8(type) || !reader.I32(rawX) ||
            !reader.I32(rawY) || !reader.I32(entity.hitPoints) ||
            !reader.I32(entity.maxHitPoints) ||
            !reader.I32(entity.movementPerTickRaw) ||
            !reader.I32(entity.visionTiles) ||
            !reader.I32(entity.attackRangeRaw) ||
            !reader.I32(entity.attackDamage) ||
            !reader.U64(entity.attackPeriodTicks) ||
            !reader.U64(entity.attackCooldownTicks) ||
            !reader.I32(entity.workRate) || !reader.I32(entity.cargo) ||
            !reader.I32(entity.cargoCapacity) ||
            !reader.I32(entity.resourceRemaining) || !reader.U8(completed) ||
            !reader.I32(entity.constructionProgress) ||
            !reader.I32(entity.constructionRequired) || !reader.U8(orderType) ||
            !reader.U32(entity.order.target) || !reader.I32(orderAnchorRawX) ||
            !reader.I32(orderAnchorRawY) || !reader.I32(orderRawX) ||
            !reader.I32(orderRawY) || !reader.U8(orderBuildType) ||
            !reader.U8(orderWellChoice) || !reader.U8(wellChoice) ||
            (version != kLegacySnapshotVersion &&
             !reader.U64(entity.wellActivationTick)) ||
            !reader.U64(entity.reshapeUntilTick) ||
            !reader.U8(entity.reshapeVariant) || !reader.U8(productionType) ||
            !reader.I32(entity.productionProgress) ||
            !reader.I32(entity.productionRequired) || !reader.U8(deployed) ||
            !reader.I32(deploymentFacingRawX) ||
            !reader.I32(deploymentFacingRawY) ||
            !reader.U8(relaySupplyActive) ||
            !reader.U64(entity.relaySupplyUntilTick) ||
            !reader.U64(entity.relaySupplyCooldownUntilTick) ||
            !reader.U8(waystoneMode) ||
            !reader.U64(entity.waystoneTransitionUntilTick) ||
            !reader.U8(warformAdaptation) ||
            !reader.U8(pendingWarformAdaptation) ||
            !reader.U32(entity.moltSite) ||
            !reader.U64(entity.moltUntilTick) ||
            !reader.U64(entity.mineralCoverCooldownUntilTick) ||
            !reader.U8(temporaryMineralCover) ||
            !reader.U32(entity.mineralCoverCreator) ||
            !reader.U64(entity.mineralCoverUntilTick) ||
            !reader.U8(mineralCoverUnderlyingTerrain) ||
            !reader.U64(entity.vibrationSignatureUntilTick) ||
            !reader.U8(aegisPowered) ||
            (HasChoirSnapshotSchema(version) &&
             (!reader.U8(choirIdentityState) ||
              !reader.U64(entity.choirIdentityResolveAtTick) ||
              !reader.U64(entity.choirIdentityNextAvailableTick) ||
              !reader.U64(entity.choirCoherenceNextChargeTick)))) {
            SetError(error, "snapshot entity data is truncated");
            return std::nullopt;
        }
        if (version == kLegacySnapshotVersion &&
            wellChoice !=
                static_cast<std::uint8_t>(FutureWellChoice::Dormant)) {
            // Version 20 did not retain the exact activation boundary. Its
            // current tick is a conservative migration point; Mission 12
            // checkpoints require a native version-21 payload and never use
            // this derived value as continuity evidence.
            entity.wellActivationTick = simulation.currentTick_;
        }
        const bool neutralPublicInterface =
            entity.owner == kNeutralPlayer &&
            type == static_cast<std::uint8_t>(EntityType::UtilityStructure) &&
            completed == 1 &&
            orderType == static_cast<std::uint8_t>(OrderType::None) &&
            entity.movementPerTickRaw == 0 && entity.visionTiles == 0 &&
            entity.attackRangeRaw == 0 && entity.attackDamage == 0 &&
            entity.attackPeriodTicks == 0 && entity.attackCooldownTicks == 0 &&
            entity.workRate == 0 && entity.cargo == 0 &&
            entity.cargoCapacity == 0 && entity.resourceRemaining == 0 &&
            entity.constructionProgress == 0 &&
            entity.constructionRequired == 0 &&
            entity.productionProgress == 0 &&
            entity.productionRequired == 0;
        const bool neutralEntityTypeValid =
            entity.owner != kNeutralPlayer ||
            type == static_cast<std::uint8_t>(EntityType::ResourceNode) ||
            type == static_cast<std::uint8_t>(EntityType::FutureWell) ||
            neutralPublicInterface;
        const bool choirIdentityUnit =
            faction == static_cast<std::uint8_t>(Faction::HollowChoir) &&
            (type == static_cast<std::uint8_t>(EntityType::Soldier) ||
             type == static_cast<std::uint8_t>(EntityType::HeavyUnit) ||
             type == static_cast<std::uint8_t>(EntityType::ScoutUnit));
        const bool choirIdentityResolving =
            choirIdentityState == static_cast<std::uint8_t>(
                                      ChoirIdentityState::DualResolveManifest) ||
            choirIdentityState == static_cast<std::uint8_t>(
                                      ChoirIdentityState::DualResolvePossible);
        const bool choirCoherenceStructure =
            entity.owner != kNeutralPlayer &&
            faction == static_cast<std::uint8_t>(Faction::HollowChoir) &&
            (type == static_cast<std::uint8_t>(EntityType::Dropoff) ||
             type == static_cast<std::uint8_t>(EntityType::Barracks) ||
             type == static_cast<std::uint8_t>(EntityType::UtilityStructure));
        if (entity.id == 0 || entity.id <= priorId ||
            faction > static_cast<std::uint8_t>(
                HasChoirSnapshotSchema(version) ? Faction::HollowChoir
                                                : Faction::KharuunAssemblies) ||
            type > static_cast<std::uint8_t>(EntityType::UtilityStructure) ||
            completed > 1 ||
            orderType > static_cast<std::uint8_t>(OrderType::Patrol) ||
            orderBuildType >
                static_cast<std::uint8_t>(EntityType::UtilityStructure) ||
            orderWellChoice > static_cast<std::uint8_t>(FutureWellChoice::Reshape) ||
            wellChoice > static_cast<std::uint8_t>(FutureWellChoice::Reshape) ||
            entity.reshapeVariant > 3 ||
            productionType >
                static_cast<std::uint8_t>(EntityType::UtilityStructure) ||
            deployed > 1 ||
            relaySupplyActive > 1 ||
            waystoneMode > static_cast<std::uint8_t>(WaystoneMode::Rooting) ||
            warformAdaptation >
                static_cast<std::uint8_t>(WarformAdaptation::Striker) ||
            pendingWarformAdaptation >
                static_cast<std::uint8_t>(WarformAdaptation::Striker) ||
            temporaryMineralCover > 1 ||
            aegisPowered > 1 ||
            !IsValidChoirIdentityState(
                static_cast<ChoirIdentityState>(choirIdentityState)) ||
            mineralCoverUnderlyingTerrain >
                static_cast<std::uint8_t>(Terrain::Scarred) ||
            (entity.owner != kNeutralPlayer &&
             (entity.owner >= simulation.players_.size() ||
              !simulation.players_[entity.owner].active)) ||
            !neutralEntityTypeValid ||
            entity.maxHitPoints <= 0 || entity.hitPoints <= 0 ||
            entity.hitPoints > entity.maxHitPoints ||
            entity.movementPerTickRaw < 0 || entity.visionTiles < 0 ||
            entity.visionTiles > kMaximumVisionTiles ||
            entity.attackRangeRaw < 0 || entity.attackDamage < 0 ||
            entity.attackPeriodTicks > kMaximumSupportedTick ||
            entity.attackCooldownTicks > entity.attackPeriodTicks ||
            entity.workRate < 0 || entity.cargo < 0 ||
            entity.cargoCapacity < entity.cargo || entity.resourceRemaining < 0 ||
            entity.constructionProgress < 0 || entity.constructionRequired < 0 ||
            entity.constructionProgress > entity.constructionRequired ||
            entity.productionProgress < 0 || entity.productionRequired < 0 ||
            entity.productionRequired > kMaximumProductionTicks ||
            entity.productionProgress > entity.productionRequired ||
            (entity.productionRequired == 0 &&
             entity.productionProgress != 0) ||
            (entity.productionRequired > 0 &&
             !((type == static_cast<std::uint8_t>(EntityType::CommandCore) &&
               productionType == static_cast<std::uint8_t>(EntityType::Worker)) ||
               (type == static_cast<std::uint8_t>(EntityType::Barracks) &&
                (productionType == static_cast<std::uint8_t>(EntityType::Soldier) ||
                 productionType == static_cast<std::uint8_t>(EntityType::HeavyUnit) ||
                 productionType == static_cast<std::uint8_t>(EntityType::ScoutUnit))))) ||
            entity.wellActivationTick > kMaximumSupportedTick ||
            (wellChoice ==
                 static_cast<std::uint8_t>(FutureWellChoice::Dormant) &&
             entity.wellActivationTick != 0) ||
            (wellChoice !=
                 static_cast<std::uint8_t>(FutureWellChoice::Dormant) &&
             (type != static_cast<std::uint8_t>(EntityType::FutureWell) ||
              entity.wellActivationTick == 0 ||
              entity.wellActivationTick > simulation.currentTick_)) ||
            entity.reshapeUntilTick > kMaximumSupportedTick ||
            (wellChoice != static_cast<std::uint8_t>(FutureWellChoice::Reshape) &&
             entity.reshapeUntilTick != 0) ||
            (wellChoice == static_cast<std::uint8_t>(FutureWellChoice::Reshape) &&
             entity.reshapeUntilTick != 0 &&
             entity.reshapeUntilTick <= simulation.currentTick_) ||
            (deployed != 0 &&
             (faction != static_cast<std::uint8_t>(Faction::MeridianCompact) ||
              type != static_cast<std::uint8_t>(EntityType::HeavyUnit))) ||
            !((deploymentFacingRawX == kFixedScale && deploymentFacingRawY == 0) ||
              (deploymentFacingRawX == -kFixedScale && deploymentFacingRawY == 0) ||
              (deploymentFacingRawX == 0 && deploymentFacingRawY == kFixedScale) ||
              (deploymentFacingRawX == 0 && deploymentFacingRawY == -kFixedScale)) ||
            (relaySupplyActive != 0 &&
             (faction != static_cast<std::uint8_t>(Faction::MeridianCompact) ||
              type != static_cast<std::uint8_t>(EntityType::ScoutUnit) ||
              entity.relaySupplyUntilTick <= simulation.currentTick_)) ||
            (relaySupplyActive == 0 && entity.relaySupplyUntilTick != 0) ||
            entity.relaySupplyUntilTick > kMaximumSupportedTick ||
            entity.relaySupplyCooldownUntilTick > kMaximumSupportedTick ||
            entity.relaySupplyCooldownUntilTick < entity.relaySupplyUntilTick ||
            (waystoneMode != static_cast<std::uint8_t>(WaystoneMode::NotWaystone) &&
             (faction != static_cast<std::uint8_t>(Faction::KharuunAssemblies) ||
              type != static_cast<std::uint8_t>(EntityType::Dropoff))) ||
            (faction == static_cast<std::uint8_t>(Faction::KharuunAssemblies) &&
             type == static_cast<std::uint8_t>(EntityType::Dropoff) &&
             waystoneMode == static_cast<std::uint8_t>(WaystoneMode::NotWaystone)) ||
            ((waystoneMode == static_cast<std::uint8_t>(WaystoneMode::Uprooting) ||
              waystoneMode == static_cast<std::uint8_t>(WaystoneMode::Rooting)) &&
             (entity.waystoneTransitionUntilTick <= simulation.currentTick_ ||
              entity.waystoneTransitionUntilTick > kMaximumSupportedTick)) ||
            ((waystoneMode == static_cast<std::uint8_t>(WaystoneMode::NotWaystone) ||
              waystoneMode == static_cast<std::uint8_t>(WaystoneMode::Rooted) ||
              waystoneMode == static_cast<std::uint8_t>(WaystoneMode::Mobile)) &&
             entity.waystoneTransitionUntilTick != 0) ||
            ((warformAdaptation !=
                  static_cast<std::uint8_t>(WarformAdaptation::None) ||
              pendingWarformAdaptation !=
                  static_cast<std::uint8_t>(WarformAdaptation::None)) &&
             (faction !=
                  static_cast<std::uint8_t>(Faction::KharuunAssemblies) ||
              (type != static_cast<std::uint8_t>(EntityType::Soldier) &&
               type != static_cast<std::uint8_t>(EntityType::HeavyUnit) &&
               type != static_cast<std::uint8_t>(EntityType::ScoutUnit)))) ||
            (pendingWarformAdaptation ==
                 static_cast<std::uint8_t>(WarformAdaptation::None) &&
             (entity.moltSite != 0 || entity.moltUntilTick != 0)) ||
            (pendingWarformAdaptation !=
                 static_cast<std::uint8_t>(WarformAdaptation::None) &&
             (warformAdaptation !=
                  static_cast<std::uint8_t>(WarformAdaptation::None) ||
              entity.moltSite == 0 ||
              entity.moltUntilTick <= simulation.currentTick_ ||
              entity.moltUntilTick > kMaximumSupportedTick)) ||
            entity.mineralCoverCooldownUntilTick > kMaximumSupportedTick ||
            entity.vibrationSignatureUntilTick > kMaximumSupportedTick ||
            (entity.vibrationSignatureUntilTick != 0 &&
             (entity.owner == kNeutralPlayer ||
              entity.movementPerTickRaw <= 0)) ||
            (entity.vibrationSignatureUntilTick > simulation.currentTick_ &&
             entity.vibrationSignatureUntilTick - simulation.currentTick_ >
                 simulation.config_.rules.vibrationDetection
                     .signatureLingerTicks) ||
            (entity.mineralCoverCooldownUntilTick != 0 &&
             (faction !=
                  static_cast<std::uint8_t>(Faction::KharuunAssemblies) ||
              type != static_cast<std::uint8_t>(EntityType::HeavyUnit))) ||
            (aegisPowered != 0 &&
             (faction !=
                  static_cast<std::uint8_t>(Faction::MeridianCompact) ||
              type !=
                  static_cast<std::uint8_t>(EntityType::UtilityStructure) ||
                  completed == 0)) ||
            (choirIdentityUnit !=
             (choirIdentityState != static_cast<std::uint8_t>(
                                          ChoirIdentityState::NotChoir))) ||
            (choirIdentityUnit && simulation.config_.rules.version < 2) ||
            (choirIdentityResolving &&
             (entity.choirIdentityResolveAtTick <= simulation.currentTick_ ||
              entity.choirIdentityResolveAtTick > kMaximumSupportedTick ||
              entity.choirIdentityNextAvailableTick <
                  entity.choirIdentityResolveAtTick ||
              entity.choirIdentityNextAvailableTick > kMaximumSupportedTick)) ||
            (!choirIdentityResolving &&
             entity.choirIdentityResolveAtTick != 0) ||
            entity.choirIdentityNextAvailableTick > kMaximumSupportedTick ||
            (!choirIdentityUnit &&
             entity.choirIdentityNextAvailableTick != 0) ||
            entity.choirCoherenceNextChargeTick > kMaximumSupportedTick ||
            (choirCoherenceStructure && completed != 0 &&
             (entity.choirCoherenceNextChargeTick == 0 ||
              entity.choirCoherenceNextChargeTick < simulation.currentTick_)) ||
            ((!choirCoherenceStructure || completed == 0) &&
             entity.choirCoherenceNextChargeTick != 0) ||
            (temporaryMineralCover == 0 &&
             (entity.mineralCoverCreator != 0 ||
              entity.mineralCoverUntilTick != 0 ||
              mineralCoverUnderlyingTerrain !=
                  static_cast<std::uint8_t>(Terrain::Open))) ||
            (temporaryMineralCover != 0 &&
             (faction !=
                  static_cast<std::uint8_t>(Faction::KharuunAssemblies) ||
              type !=
                  static_cast<std::uint8_t>(EntityType::UtilityStructure) ||
              completed == 0 || entity.mineralCoverCreator == 0 ||
              entity.mineralCoverCreator >= entity.id ||
              entity.mineralCoverUntilTick <= simulation.currentTick_ ||
              entity.mineralCoverUntilTick > kMaximumSupportedTick ||
              mineralCoverUnderlyingTerrain ==
                  static_cast<std::uint8_t>(Terrain::Blocked) ||
              entity.maxHitPoints !=
                  simulation.config_.rules.mineralCover.maxHitPoints ||
              entity.movementPerTickRaw != 0 || entity.visionTiles != 0 ||
              entity.attackRangeRaw != 0 || entity.attackDamage != 0 ||
              entity.attackPeriodTicks != 0 || entity.workRate != 0 ||
              entity.cargoCapacity != 0 ||
              entity.constructionRequired != 0))) {
            SetError(error, "snapshot entity state is invalid");
            return std::nullopt;
        }
        entity.faction = static_cast<Faction>(faction);
        entity.type = static_cast<EntityType>(type);
        entity.position = Vec2::FromRaw(rawX, rawY);
        entity.completed = completed != 0;
        entity.order.type = static_cast<OrderType>(orderType);
        entity.order.anchor =
            Vec2::FromRaw(orderAnchorRawX, orderAnchorRawY);
        entity.order.destination = Vec2::FromRaw(orderRawX, orderRawY);
        entity.order.buildType = static_cast<EntityType>(orderBuildType);
        entity.order.wellChoice = static_cast<FutureWellChoice>(orderWellChoice);
        entity.wellChoice = static_cast<FutureWellChoice>(wellChoice);
        entity.productionType = static_cast<EntityType>(productionType);
        entity.deployed = deployed != 0;
        entity.deploymentFacing =
            Vec2::FromRaw(deploymentFacingRawX, deploymentFacingRawY);
        entity.relaySupplyActive = relaySupplyActive != 0;
        entity.waystoneMode = static_cast<WaystoneMode>(waystoneMode);
        entity.warformAdaptation =
            static_cast<WarformAdaptation>(warformAdaptation);
        entity.pendingWarformAdaptation =
            static_cast<WarformAdaptation>(pendingWarformAdaptation);
        entity.temporaryMineralCover = temporaryMineralCover != 0;
        entity.mineralCoverUnderlyingTerrain =
            static_cast<Terrain>(mineralCoverUnderlyingTerrain);
        entity.aegisPowered = aegisPowered != 0;
        entity.choirIdentityState =
            static_cast<ChoirIdentityState>(choirIdentityState);
        if (!simulation.IsInsideMap(entity.position)) {
            SetError(error, "snapshot entity is outside the map");
            return std::nullopt;
        }
        if (entity.temporaryMineralCover &&
            simulation.TerrainAt(entity.position.x.FloorToInt(),
                                 entity.position.y.FloorToInt()) !=
                Terrain::Blocked) {
            SetError(error, "snapshot mineral cover terrain is invalid");
            return std::nullopt;
        }
        if (entity.order.type == OrderType::Patrol &&
            (!simulation.IsPositionPassable(entity.order.anchor) ||
             !simulation.IsPositionPassable(entity.order.destination))) {
            SetError(error, "snapshot patrol route is invalid");
            return std::nullopt;
        }
        simulation.entities_.push_back(entity);
        priorId = entity.id;
    }
    if (simulation.nextEntityId_ == 0 ||
        (!simulation.entities_.empty() &&
         simulation.nextEntityId_ <= simulation.entities_.back().id)) {
        SetError(error, "snapshot next entity identifier is invalid");
        return std::nullopt;
    }
    for (const PlayerState& player : simulation.players_) {
        if (!player.active || player.activeResearch == ResearchType::None) {
            continue;
        }
        const ResearchRules* rules =
            simulation.ResearchDefinition(player.activeResearch);
        const Entity* producer = simulation.FindEntity(player.researchProducer);
        if (rules == nullptr || rules->faction != player.faction ||
            player.HasCompletedResearch(player.activeResearch) ||
            player.researchRequired !=
                static_cast<std::int32_t>(rules->researchTicks) ||
            player.researchProgress >= player.researchRequired ||
            (rules->prerequisite != ResearchType::None &&
             !player.HasCompletedResearch(rules->prerequisite)) ||
            producer == nullptr || producer->owner != player.id ||
            producer->hitPoints <= 0 || !producer->completed ||
            producer->type != EntityType::Barracks ||
            producer->productionRequired != 0) {
            SetError(error, "snapshot research state is invalid");
            return std::nullopt;
        }
    }
    for (const Entity& entity : simulation.entities_) {
        if (entity.aegisPowered !=
            simulation.IsAegisNetworkPowered(entity)) {
            SetError(error, "snapshot Aegis power state is invalid");
            return std::nullopt;
        }
    }
    std::map<std::pair<std::int32_t, std::int32_t>, EntityId> mineralCoverTiles;
    for (const Entity& entity : simulation.entities_) {
        if (!entity.temporaryMineralCover) {
            continue;
        }
        const auto tile = std::pair{
            entity.position.x.FloorToInt(),
            entity.position.y.FloorToInt()};
        if (!mineralCoverTiles.emplace(tile, entity.id).second) {
            SetError(error, "snapshot mineral covers overlap");
            return std::nullopt;
        }
        if (const Entity* creator =
                simulation.FindEntity(entity.mineralCoverCreator);
            creator != nullptr && creator->owner != entity.owner) {
            SetError(error, "snapshot mineral cover creator is invalid");
            return std::nullopt;
        }
    }
    for (const Entity& entity : simulation.entities_) {
        if (!simulation.IsWarform(entity)) {
            continue;
        }
        Entity expected = simulation.MakeEntity(
            entity.owner, entity.faction, entity.type, entity.position);
        simulation.ApplyWarformAdaptation(
            expected, entity.warformAdaptation);
        if (entity.maxHitPoints != expected.maxHitPoints ||
            entity.movementPerTickRaw != expected.movementPerTickRaw ||
            entity.attackDamage != expected.attackDamage ||
            entity.attackPeriodTicks != expected.attackPeriodTicks) {
            SetError(error, "snapshot warform statistics are invalid");
            return std::nullopt;
        }
        if (entity.pendingWarformAdaptation != WarformAdaptation::None) {
            const Entity* site = simulation.FindEntity(entity.moltSite);
            const std::int64_t radius =
                simulation.config_.rules.warformAdaptation.siteRadiusRaw;
            if (site == nullptr || site->owner != entity.owner ||
                !site->completed || site->hitPoints <= 0 ||
                site->faction != Faction::KharuunAssemblies ||
                site->type != EntityType::Barracks ||
                simulation.DistanceSquaredRaw(entity.position, site->position) >
                    static_cast<std::uint64_t>(radius * radius)) {
                SetError(error, "snapshot warform molt site is invalid");
                return std::nullopt;
            }
        }
    }
    for (const Entity& entity : simulation.entities_) {
        if (!simulation.IsChoirIdentityUnit(entity)) {
            continue;
        }
        Entity expected = entity;
        simulation.RefreshChoirIdentityStats(expected);
        if (entity.movementPerTickRaw != expected.movementPerTickRaw ||
            entity.visionTiles != expected.visionTiles ||
            entity.attackRangeRaw != expected.attackRangeRaw ||
            entity.attackDamage != expected.attackDamage ||
            entity.attackPeriodTicks != expected.attackPeriodTicks) {
            SetError(error, "snapshot Hollow Choir identity statistics are invalid");
            return std::nullopt;
        }
    }
    std::uint32_t commandCount = 0;
    if (!reader.U32(commandCount) || commandCount > kMaximumSerializedCommands ||
        static_cast<std::size_t>(commandCount) >
            reader.Remaining() / kSerializedCommandBytes) {
        SetError(error, "snapshot command count is invalid");
        return std::nullopt;
    }
    simulation.pendingCommands_.resize(commandCount);
    for (Command& command : simulation.pendingCommands_) {
        if (!ReadCommand(reader, command) ||
            command.player >= simulation.players_.size() ||
            !simulation.players_[command.player].active ||
            command.executeTick < simulation.currentTick_ ||
            command.executeTick > kMaximumSupportedTick || command.actor == 0 ||
            (simulation.config_.rules.version < 2 &&
             (command.type > CommandType::Research ||
              command.researchType >
                  ResearchType::KharuunAncestralEdge))) {
            SetError(error, "snapshot pending command is invalid");
            return std::nullopt;
        }
    }
    std::sort(simulation.pendingCommands_.begin(),
              simulation.pendingCommands_.end(), CommandLess);
    std::array<bool, kMaximumPlayers> sawPendingSequence{};
    std::array<Tick, kMaximumPlayers> lastPendingTick{};
    std::array<std::uint64_t, kMaximumPlayers> lastPendingSequence{};
    for (const Command& command : simulation.pendingCommands_) {
        const PlayerId player = command.player;
        if ((simulation.hasExecutedSequence_[player] &&
             command.sequence <= simulation.lastExecutedSequence_[player]) ||
            (sawPendingSequence[player] &&
             command.sequence <= lastPendingSequence[player])) {
            SetError(error, "snapshot command sequences are not monotonic");
            return std::nullopt;
        }
        if (sawPendingSequence[player] &&
            command.executeTick < lastPendingTick[player]) {
            SetError(error, "snapshot command ticks are not canonical");
            return std::nullopt;
        }
        sawPendingSequence[player] = true;
        lastPendingTick[player] = command.executeTick;
        lastPendingSequence[player] = command.sequence;
    }
    if (version == kSnapshotVersion) {
        std::uint32_t receiptCount = 0;
        if (!reader.U32(receiptCount) ||
            receiptCount > kMaximumCommandResolutionReceipts ||
            static_cast<std::size_t>(receiptCount) >
                reader.Remaining() /
                    kSerializedCommandResolutionReceiptBytes) {
            SetError(error, "snapshot command resolution receipt count is invalid");
            return std::nullopt;
        }
        simulation.commandResolutionReceipts_.clear();
        std::array<bool, kMaximumPlayers> sawReceiptSequence{};
        std::array<std::uint64_t, kMaximumPlayers> lastReceiptSequence{};
        bool sawCanonicalReceipt = false;
        Tick lastReceiptTick = 0;
        PlayerId lastReceiptPlayer = 0;
        std::uint64_t lastCanonicalReceiptSequence = 0;
        for (std::uint32_t index = 0; index < receiptCount; ++index) {
            StoredCommandResolutionReceipt stored{};
            std::uint8_t commandType = 0;
            std::uint8_t outcome = 0;
            if (!reader.U8(stored.receipt.player) ||
                !reader.U64(stored.sequence) ||
                !reader.U8(commandType) ||
                !reader.U64(stored.receipt.assignedExecutionTick) ||
                !reader.U8(outcome)) {
                SetError(error,
                         "snapshot command resolution receipt is truncated");
                return std::nullopt;
            }
            stored.receipt.commandType =
                static_cast<CommandType>(commandType);
            stored.receipt.outcome =
                static_cast<CommandResolutionOutcome>(outcome);
            const PlayerId player = stored.receipt.player;
            const Tick assignedTick =
                stored.receipt.assignedExecutionTick;
            const bool canonical =
                !sawCanonicalReceipt ||
                std::tie(lastReceiptTick,
                         lastReceiptPlayer,
                         lastCanonicalReceiptSequence) <
                    std::tie(assignedTick, player, stored.sequence);
            if (player >= simulation.players_.size() ||
                !simulation.players_[player].active ||
                !IsValidCommandType(stored.receipt.commandType) ||
                !IsValidCommandResolutionOutcome(stored.receipt.outcome) ||
                assignedTick >= simulation.currentTick_ ||
                (simulation.currentTick_ > assignedTick &&
                 simulation.currentTick_ - assignedTick >
                     kCommandResolutionReceiptRetentionTicks) ||
                !simulation.hasExecutedSequence_[player] ||
                stored.sequence >
                    simulation.lastExecutedSequence_[player] ||
                (sawReceiptSequence[player] &&
                 stored.sequence <= lastReceiptSequence[player]) ||
                !canonical ||
                (stored.receipt.outcome ==
                     CommandResolutionOutcome::InvalidPosition &&
                 stored.receipt.commandType !=
                     CommandType::RaiseMineralCover) ||
                (simulation.config_.rules.version < 2 &&
                 stored.receipt.commandType > CommandType::Research)) {
                SetError(error,
                         "snapshot command resolution receipt is invalid");
                return std::nullopt;
            }
            sawReceiptSequence[player] = true;
            lastReceiptSequence[player] = stored.sequence;
            sawCanonicalReceipt = true;
            lastReceiptTick = assignedTick;
            lastReceiptPlayer = player;
            lastCanonicalReceiptSequence = stored.sequence;
            simulation.commandResolutionReceipts_.push_back(stored);
        }
    } else {
        // Schemas 20 through 23 predate authoritative resolution receipts.
        // Their empty ledger means unavailable evidence, never success.
        simulation.commandResolutionReceipts_.clear();
    }
    if (!reader.AtEnd()) {
        SetError(error, "snapshot contains trailing payload data");
        return std::nullopt;
    }
    simulation.commandLog_.clear();
    simulation.replayInitialSnapshot_.clear();
    simulation.UpdateVisibility();
    return simulation;
}

void Simulation::CaptureReplayBaseline() {
    replayInitialSnapshot_ = SaveSnapshot();
    commandLog_.clear();
}

void Simulation::DisableReplayExport() {
    replayExportEnabled_ = false;
}

ReplayRecord Simulation::ExportReplay(std::string* error) const {
    if (error != nullptr) {
        error->clear();
    }
    if (!replayExportEnabled_) {
        SetError(error, "replay export is disabled");
        ReplayRecord rejected{};
        rejected.version = 0;
        return rejected;
    }
    ReplayRecord replay{};
    replay.initialSnapshot = replayInitialSnapshot_.empty() ? SaveSnapshot()
                                                            : replayInitialSnapshot_;
    replay.commands = commandLog_;
    std::sort(replay.commands.begin(), replay.commands.end(), CommandLess);
    replay.finalTick = currentTick_;
    replay.finalChecksum = StateChecksum();
    return replay;
}

std::optional<Simulation> Simulation::ReplayToEnd(const ReplayRecord& replay,
                                                  std::string* error) {
    if (error != nullptr) {
        error->clear();
    }
    if (replay.version != kReplayVersion) {
        SetError(error, "replay version is unsupported");
        return std::nullopt;
    }
    if (replay.commands.size() > kMaximumSerializedCommands ||
        replay.finalTick > kMaximumSupportedTick) {
        SetError(error, "replay bounds are invalid");
        return std::nullopt;
    }
    std::optional<Simulation> simulation = LoadSnapshot(replay.initialSnapshot, error);
    if (!simulation.has_value()) {
        return std::nullopt;
    }
    if (replay.finalTick < simulation->CurrentTick()) {
        SetError(error, "replay final tick precedes its baseline");
        return std::nullopt;
    }
    for (const Command& command : replay.commands) {
        std::string rejection;
        if (!simulation->QueueCommand(command, &rejection)) {
            SetError(error, "replay command rejected: " + rejection);
            return std::nullopt;
        }
    }
    simulation->Step(replay.finalTick - simulation->CurrentTick());
    if (simulation->StateChecksum() != replay.finalChecksum) {
        SetError(error, "replay final checksum does not match");
        return std::nullopt;
    }
    return simulation;
}

}  // namespace echoes::sim
