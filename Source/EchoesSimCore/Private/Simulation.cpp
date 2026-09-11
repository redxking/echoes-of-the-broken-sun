#include "EchoesSimCore/Simulation.h"

#include <algorithm>
#include <cmath>
#include <array>
#include <limits>
#include <set>
#include <stdexcept>
#include <tuple>
#include <utility>

namespace echoes::sim {
namespace {

constexpr std::uint64_t kFnvOffset = 14695981039346656037ULL;
constexpr std::uint64_t kFnvPrime = 1099511628211ULL;
constexpr std::uint32_t kMaximumMapTiles = 4U * 1024U * 1024U;
constexpr std::uint32_t kMaximumSerializedEntities = 64U * 1024U;
constexpr std::uint32_t kMaximumSerializedProjectiles =
    kMaximumSerializedEntities;
constexpr std::size_t kSerializedProjectileBytes = 41;
// A ballistic record advances no farther than one map tile per simulation
// tick. The current weapon implementation uses 60 cm/tick (614 raw units).
constexpr std::int32_t kMaximumBallisticProjectileSpeedRaw = kFixedScale;
constexpr std::uint32_t kMaximumSerializedCommands =
    static_cast<std::uint32_t>(kMaximumCommandLogEntries);
constexpr std::size_t kMaximumCachedPathFields = 128;
// SPEC-BUD-006 authored Logistics ceiling for one player.
// REL-ECO-011 (TBR-STR-003, 2026-09-11): the Logistics ceiling is 120 under
// current rules; recordings older than schema 33 keep the 200 they were made
// with. Above kCommittedBandThreshold committed Logistics every further two
// points of fielded population cost one more (a line unit costs 3, not 2), so
// mass is taxed by the rules and not only by the map.
constexpr std::int32_t kMaximumPopulationCapacity = 120;
constexpr std::int32_t kLegacyMaximumPopulationCapacity = 200;
constexpr std::int32_t kCommittedBandThreshold = 80;
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
constexpr std::uint32_t kCommandResolutionReceiptSnapshotVersion = 24;
constexpr std::uint32_t kMemorySnapshotVersion = 25;
constexpr std::uint32_t kWorkStateSnapshotVersion = 26;
constexpr std::uint32_t kFutureWellLifecycleSnapshotVersion = 27;
constexpr std::uint32_t kHostilitySnapshotVersion = 28;
constexpr std::uint32_t kProductionPipelineSnapshotVersion = 29;
constexpr std::uint32_t kLinkMechanicsSnapshotVersion = 30;
constexpr std::uint32_t kBulwarkCommitmentSnapshotVersion = 31;
constexpr std::size_t kSerializedRememberedObjectBytes = 24;
constexpr std::size_t kLegacyFactionCount = 2;
constexpr std::size_t kLegacyResearchTypeCount = 5;
constexpr std::size_t kLegacySerializedEntityBytes = 202;
constexpr std::size_t kPriorSerializedEntityBytes = 210;
constexpr std::size_t kSerializedEntityBytes = 235;
constexpr std::size_t kLegacySerializedCommandBytes = 38;
constexpr std::size_t kSerializedCommandBytes = 39;
constexpr std::size_t kSerializedCommandResolutionReceiptBytes = 19;
constexpr std::size_t kSerializedFutureWellLifecycleBytes = 16;
constexpr std::size_t kSnapshotFixedBytesAfterConfig = 132;
constexpr std::int32_t kFutureWellCaptureRadiusRaw = 21 * kFixedScale / 5;
constexpr std::int32_t kFutureWellScarRadiusRaw = 6 * kFixedScale;
constexpr std::uint16_t kFutureWellCaptureRequiredTicks = 300;
constexpr Tick kHarvestTelegraphTicks = 180;
constexpr Tick kReshapeTelegraphTicks = 180;
constexpr std::int32_t kMaximumMapDimension =
    std::numeric_limits<std::int32_t>::max() / kFixedScale;
constexpr std::uint8_t kValidCommandCoreProtectionMask =
    static_cast<std::uint8_t>((1U << kMaximumPlayers) - 1U);

// Every replay schema from the legacy cutoff to the current one has explicit
// semantics (the legacy*ReplaySemantics_ flags), so all of them load. A list
// of named versions used to live here and silently dropped schemas 30 to 32
// as each bump appended only the newest constant; retained recordings and
// campaign checkpoints at those versions were then refused as unsupported.
[[nodiscard]] bool IsSupportedReplayVersion(std::uint32_t version) {
    return version >= kLegacyReplayVersion && version <= kReplayVersion;
}

[[nodiscard]] bool HasChoirSnapshotSchema(std::uint32_t version) {
    return version >= kChoirSnapshotVersion;
}

[[nodiscard]] bool HasProtectedCommandCoreSnapshotSchema(
    std::uint32_t version) {
    return version >= kProtectedCommandCoreSnapshotVersion;
}

[[nodiscard]] bool HasCommandResolutionReceiptSnapshotSchema(
    std::uint32_t version) {
    return version >= kCommandResolutionReceiptSnapshotVersion;
}

[[nodiscard]] bool HasMemorySnapshotSchema(std::uint32_t version) {
    return version >= kMemorySnapshotVersion;
}

[[nodiscard]] bool HasWorkStateSnapshotSchema(std::uint32_t version) {
    return version >= kWorkStateSnapshotVersion;
}

[[nodiscard]] bool HasFutureWellLifecycleSnapshotSchema(
    std::uint32_t version) {
    return version >= kFutureWellLifecycleSnapshotVersion;
}

[[nodiscard]] bool HasProductionPipelineSnapshotSchema(
    std::uint32_t version) {
    return version >= kProductionPipelineSnapshotVersion;
}

[[nodiscard]] bool HasLinkMechanicsSnapshotSchema(std::uint32_t version) {
    return version >= kLinkMechanicsSnapshotVersion;
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

[[nodiscard]] std::optional<std::uint64_t> Fnv1a(
    std::span<const std::uint8_t> bytes,
    const ReplayCancellationCheck& shouldCancel) {
    constexpr std::size_t kCancellationChunkBytes = 1024U * 1024U;
    std::uint64_t hash = kFnvOffset;
    for (std::size_t index = 0; index < bytes.size(); ++index) {
        if ((index % kCancellationChunkBytes) == 0U && shouldCancel &&
            shouldCancel()) {
            return std::nullopt;
        }
        hash ^= bytes[index];
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

// REL-AI-011/012/031: how far each doctrine grows its economy and industry.
// The opponent used to be capped structurally rather than by doctrine -- one
// Barracks for the whole match, eight workers, and supply only if it happened
// to start without a Dropoff -- so every personality ended a match with the
// same three units.
struct AiMacroTargets final {
    std::int32_t workerTarget;
    std::int32_t producerCap;
    std::int32_t dropoffCap;
    std::int32_t utilityCap;
};

// REL-AI-009/010/012/013 and REL-FAC-019: what each doctrine puts in the field.
// The generator could previously emit only Soldiers, so the roster's entire
// soft-counter design was inert against the only opponent in the game.
struct AiComposition final {
    std::int32_t soldierWeight;
    std::int32_t heavyWeight;
    std::int32_t scoutWeight;
};

[[nodiscard]] AiComposition CompositionFor(AiPersonality personality) {
    switch (personality) {
        // Warden holds a perimeter with heavy line units (REL-AI-009).
        case AiPersonality::Defensive:
            return {40, 45, 15};
        // Raider fields fast mobile units and no slow screens (REL-AI-010).
        case AiPersonality::Raider:
            return {55, 0, 45};
        case AiPersonality::Economic:
            return {55, 30, 15};
        // Expansionist needs eyes on the territory it claims (REL-AI-012).
        case AiPersonality::Expansionist:
            return {45, 25, 30};
        case AiPersonality::Adaptive:
            return {45, 35, 20};
        case AiPersonality::Balanced:
            return {50, 30, 20};
    }
    return {50, 30, 20};
}

// REL-AI-013: Adaptive weights its army against what it has actually observed --
// screens against heavy lines, sensors against fast movement.
[[nodiscard]] AiComposition AdjustedCompositionFor(
    AiPersonality personality,
    std::int32_t visibleHeavyThreats,
    std::int32_t visibleMobileThreats) {
    AiComposition mix = CompositionFor(personality);
    if (personality != AiPersonality::Adaptive) {
        return mix;
    }
    if (visibleHeavyThreats > visibleMobileThreats) {
        mix.soldierWeight += 20;
    } else if (visibleMobileThreats > visibleHeavyThreats) {
        mix.scoutWeight += 20;
    }
    return mix;
}

[[nodiscard]] AiMacroTargets MacroTargetsFor(AiPersonality personality) {
    switch (personality) {
        // The Steward maximises worker growth (REL-AI-011).
        case AiPersonality::Economic:
            return {26, 4, 6, 1};
        case AiPersonality::Expansionist:
            return {20, 5, 6, 1};
        case AiPersonality::Adaptive:
            return {18, 4, 5, 1};
        case AiPersonality::Balanced:
            return {18, 4, 5, 1};
        // The Warden fortifies early and holds (REL-AI-009).
        case AiPersonality::Defensive:
            return {16, 3, 4, 3};
        // The Raider spends on pressure, not on a bigger base.
        case AiPersonality::Raider:
            return {12, 5, 4, 0};
    }
    return {16, 3, 4, 1};
}

[[nodiscard]] bool IsBarracksUnitType(EntityType type) {
    return type == EntityType::Soldier || type == EntityType::HeavyUnit ||
           type == EntityType::ScoutUnit;
}

// SPEC-RES-008: the entity classes that spend a player's 30-slot mobile
// allowance. Listed explicitly rather than as !IsBuildingType so neutral
// deposits and Future Wells can never be miscounted, and so a new mobile
// class must be classified here before it can be produced.
[[nodiscard]] bool IsMobileEntityType(EntityType type) {
    return type == EntityType::Worker || IsBarracksUnitType(type);
}

[[nodiscard]] std::int32_t CountMobileEntities(
    const std::vector<Entity>& entities, PlayerId player) {
    std::int32_t count = 0;
    for (const Entity& entity : entities) {
        if (entity.owner == player && entity.hitPoints > 0 &&
            IsMobileEntityType(entity.type)) {
            ++count;
        }
    }
    return count;
}

[[nodiscard]] std::int32_t CountMobileEntityReservations(
    const std::vector<Entity>& entities, PlayerId player) {
    std::int32_t reserved = 0;
    for (const Entity& entity : entities) {
        if (entity.owner == player && entity.hitPoints > 0 &&
            entity.productionRequired > 0 &&
            IsMobileEntityType(entity.productionType)) {
            ++reserved;
        }
    }
    return reserved;
}

[[nodiscard]] bool IsValidTerrain(Terrain terrain) {
    return terrain >= Terrain::Open && terrain <= Terrain::Scarred;
}

[[nodiscard]] bool IsValidCommandType(CommandType type) {
    return type >= CommandType::Stop &&
           type <= CommandType::CancelConstruction;
}

[[nodiscard]] bool IsValidCommandResolutionOutcome(
    CommandResolutionOutcome outcome) {
    return outcome >= CommandResolutionOutcome::Applied &&
           outcome <= CommandResolutionOutcome::DestinationOccupied;
}

/** True for the MOV-002 movement rejection vocabulary. */
[[nodiscard]] bool IsMovementRejectionOutcome(
    CommandResolutionOutcome outcome) {
    return outcome == CommandResolutionOutcome::NoPath ||
           outcome == CommandResolutionOutcome::RouteBlocked ||
           outcome == CommandResolutionOutcome::DestinationOccupied;
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
// SPEC-DOC-005 "moderate opening": Standard Adaptive holds its Command Core
// opening posture for 300 s (ledger SIM-033) before its barracks units march.
// The 1200-tick value introduced with the march behaviour had no requirement
// authority and sent raids into Missions 10 and 15 mid-contract.
constexpr Tick kAdaptiveOpeningPostureTicks = 6000;

// Section 7 terrain table: Scarred is 85% speed. Open is 100%; Blocked and
// Water/void are impassable and never reach a movement step. This is an
// authored terrain constant, not per-faction tuning, so it lives here rather
// than in SimulationRules, which is serialized field-by-field into every
// snapshot and replay.
constexpr std::int32_t kScarredMovementPercent = 85;

[[nodiscard]] std::int32_t SaturatingAdd(std::int32_t lhs, std::int32_t rhs);
[[nodiscard]] bool ResourceCovers(const ResourcePool& available,
                                  const ResourcePool& cost);

[[nodiscard]] bool IsBuildingType(EntityType type) {
    return type == EntityType::CommandCore || type == EntityType::Dropoff ||
           type == EntityType::Barracks || type == EntityType::UtilityStructure;
}

// BLD-009: players may build multiple production, supply, utility, and
// drop-off structures, but never an additional Command Core. IsBuildingType
// stays the structure *classifier* (threat scoring, dropoff rules, snapshot
// validation); only placement consults this narrower predicate. A Core still
// reaches the field through authored spawns and mission scripting.
[[nodiscard]] bool IsConstructableBuildingType(EntityType type) {
    return IsBuildingType(type) && type != EntityType::CommandCore;
}

[[nodiscard]] bool IsDropoffType(EntityType type) {
    return type == EntityType::CommandCore || type == EntityType::Dropoff;
}

// FOG information state "Explored": last observed permanent objects. Units are
// deliberately excluded — the spec keeps unit sightings in the separate,
// optional "Last known" state, which this core does not grant.
[[nodiscard]] bool IsRememberablePermanentObject(const Entity& entity) {
    if (entity.hitPoints <= 0 || entity.temporaryMineralCover) {
        return false;
    }
    // An uprooted Waystone is walking, not standing: while it is mobile it is
    // a unit and leaves no permanent-object memory. The site it was last seen
    // rooted at stays remembered until the player looks at it again.
    if (entity.waystoneMode == WaystoneMode::Uprooting ||
        entity.waystoneMode == WaystoneMode::Mobile) {
        return false;
    }
    switch (entity.type) {
        case EntityType::CommandCore:
        case EntityType::Dropoff:
        case EntityType::Barracks:
        case EntityType::UtilityStructure:
        case EntityType::ResourceNode:
        case EntityType::FutureWell:
            return true;
        case EntityType::Worker:
        case EntityType::Soldier:
        case EntityType::HeavyUnit:
        case EntityType::ScoutUnit:
            break;
    }
    return false;
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
    if (!IsConstructableBuildingType(buildingType)) {
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

// REL-FAC-002.PROD: whether an owned operational network node reaches a
// position, judged from the scoped view. The opponent sites its Foundry where
// it will produce; the view carries the same networkOperational flags the
// simulation resolved, never a private answer.
[[nodiscard]] bool IsViewPositionInMeridianNetwork(const PlayerView& view,
                                                   Vec2 position) {
    const std::int64_t radius =
        view.Config().rules.poweredAegis.connectionRadiusRaw;
    const std::uint64_t radiusSquared =
        static_cast<std::uint64_t>(radius * radius);
    for (const Entity& node : view.Entities()) {
        if (node.owner == view.Player().id && node.completed &&
            node.hitPoints > 0 && node.faction == Faction::MeridianCompact &&
            node.networkOperational &&
            DistanceSquaredRawFor(position, node.position) <= radiusSquared) {
            return true;
        }
    }
    return false;
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
    if (view.ProductionRequiresNetworkPower() &&
        building->faction == Faction::MeridianCompact &&
        building->type == EntityType::Barracks &&
        !building->networkOperational) {
        return ProductionResult::ProducerUnpowered;
    }
    if (view.Player().activeResearch != ResearchType::None &&
        view.Player().researchProducer == producer) {
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
    if (building->productionRequired > 0 ||
        !building->productionQueue.empty()) {
        return building->productionQueue.size() < Entity::kMaxProductionQueue
                   ? ProductionResult::Valid
                   : ProductionResult::QueueFull;
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
                entity.productionLogisticsCost);
        }
    }
    if (SaturatingAdd(
            committedPopulation,
            PopulationCostFor(
                view.Config().rules, view.Player().faction, unitType)) >
        view.PopulationCapacity()) {
        return ProductionResult::CapacityReached;
    }

    // SPEC-RES-008: the opponent plans against the same limit the player is
    // held to, from its own scoped view.
    if (IsMobileEntityType(unitType) &&
        view.MobileEntityCount() + view.MobileEntityReservations() >=
            kMobileEntityLimit) {
        return ProductionResult::MobileEntityLimitReached;
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
             entity.waystoneMode != WaystoneMode::Rooted) ||
            (entity.faction == Faction::MeridianCompact &&
             entity.type == EntityType::Dropoff &&
             !entity.networkOperational)) {
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
    void U16(std::uint16_t value) {
        U8(static_cast<std::uint8_t>(value & 0xffU));
        U8(static_cast<std::uint8_t>((value >> 8U) & 0xffU));
    }
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
    void U16(std::uint16_t value) { Mix(0x0200000000000000ULL | value); }
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
    [[nodiscard]] bool U16(std::uint16_t& value) {
        std::uint8_t low = 0;
        std::uint8_t high = 0;
        if (!U8(low) || !U8(high)) {
            return false;
        }
        value = static_cast<std::uint16_t>(low) |
                (static_cast<std::uint16_t>(high) << 8U);
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
void WriteCommand(Writer& writer, const Command& command,
                  std::uint32_t snapshotVersion) {
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
    if (HasProductionPipelineSnapshotSchema(snapshotVersion)) {
        writer.U8(command.queue ? 1 : 0);
    }
}

[[nodiscard]] bool ReadCommand(BinaryReader& reader, Command& command,
                               std::uint32_t snapshotVersion) {
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
    std::uint8_t queued = 0;
    if (HasProductionPipelineSnapshotSchema(snapshotVersion) &&
        (!reader.U8(queued) || queued > 1)) {
        return false;
    }
    const CommandType maximumCommand =
        HasLinkMechanicsSnapshotSchema(snapshotVersion)
            ? CommandType::CancelConstruction
            : HasProductionPipelineSnapshotSchema(snapshotVersion)
                  ? CommandType::SetRallyRoute
                  : CommandType::ReconcileToPossible;
    if (type > static_cast<std::uint8_t>(maximumCommand) ||
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
    command.queue = queued != 0;
    return true;
}

[[nodiscard]] bool ResourceCovers(const ResourcePool& available,
                                  const ResourcePool& cost) {
    return available.material >= cost.material &&
           available.dawnshards >= cost.dawnshards;
}

}  // namespace

const char* CommandRejectionReasonCode(CommandResolutionOutcome outcome) {
    switch (outcome) {
    case CommandResolutionOutcome::NoPath:
        return "NO PATH";
    case CommandResolutionOutcome::RouteBlocked:
        return "ROUTE BLOCKED";
    case CommandResolutionOutcome::DestinationOccupied:
        return "DESTINATION OCCUPIED";
    case CommandResolutionOutcome::InvalidPosition:
        return "INVALID POSITION";
    case CommandResolutionOutcome::Applied:
    case CommandResolutionOutcome::NoEffect:
        break;
    }
    return "";
}

const char* CommandRejectionRecovery(CommandResolutionOutcome outcome) {
    switch (outcome) {
    case CommandResolutionOutcome::NoPath:
        return "Your map shows no route to that tile. Scout a connecting "
               "route or order the move to ground you can already reach.";
    case CommandResolutionOutcome::RouteBlocked:
        return "This unit is walled in. Clear or destroy the obstruction "
               "beside it, then order the move again.";
    case CommandResolutionOutcome::DestinationOccupied:
        return "That tile is not open ground. Order the move to a clear tile "
               "next to it.";
    case CommandResolutionOutcome::InvalidPosition:
        return "That position is outside the playable map. Pick a tile inside "
               "the battlefield.";
    case CommandResolutionOutcome::Applied:
    case CommandResolutionOutcome::NoEffect:
        break;
    }
    return "";
}

SimulationRules DefaultSimulationRules() {
    SimulationRules rules{};
    const auto set = [&rules](Faction faction,
                              EntityType type,
                              EntityArchetypeRules value) {
        rules.archetypes[static_cast<std::size_t>(faction)]
                        [static_cast<std::size_t>(type)] = value;
    };

    // Surveyor: "Work rate 10; cargo 10; no attack" and, again, "No attack,
    // low health, and high strategic value." Attack range/damage/cadence are
    // zero for both worker archetypes below.
    set(Faction::MeridianCompact, EntityType::Worker,
        {{50, 0}, 80, 128, 5, 0, 0, 0, 10, 100, 0, 1, 0,
         60, kFixedScale / 8});
    set(Faction::MeridianCompact, EntityType::Soldier,
        {{85, 20}, 120, 112, 6, 4 * kFixedScale, 18, 12, 0, 0, 0, 2,
         0, 100, kFixedScale / 8});
    // Anchor footprint is 5x5 tiles (Content/Data/Source/buildings.json
    // mc_anchor, and the Concordance entry below already encodes it).
    // kFixedScale is one tile, so a 5x5 half-extent is 5 * kFixedScale / 2.
    set(Faction::MeridianCompact, EntityType::CommandCore,
        {{420, 40}, 1000, 0, 8, 0, 0, 0, 0, 0, 400, 0, 12, 0,
         5 * kFixedScale / 2});
    set(Faction::MeridianCompact, EntityType::Dropoff,
        {{110, 0}, 500, 0, 5, 0, 0, 0, 0, 0, 100, 0, 6, 0,
         3 * kFixedScale / 4});
    set(Faction::MeridianCompact, EntityType::Barracks,
        {{170, 20}, 650, 0, 5, 0, 0, 0, 0, 0, 160, 0, 0, 0,
         2 * kFixedScale});
    set(Faction::MeridianCompact, EntityType::HeavyUnit,
        {{130, 25}, 260, 117, 9, 3 * kFixedScale, 10, 24, 0, 0, 0, 3,
         0, 140, kFixedScale / 8});
    set(Faction::MeridianCompact, EntityType::ScoutUnit,
        {{70, 20}, 75, 256, 15, 4 * kFixedScale, 6, 24, 0, 0, 0, 1,
         0, 80, kFixedScale / 8});
    set(Faction::MeridianCompact, EntityType::UtilityStructure,
        {{130, 30}, 520, 0, 7, 9 * kFixedScale, 28, 20, 0, 0, 120, 0, 0, 0,
         kFixedScale});

    // Tender: "Work rate 9; cargo 10; no attack" and, again, "No attack.
    // Stabilization is slow, visible, and too expensive [...]".
    set(Faction::KharuunAssemblies, EntityType::Worker,
        {{50, 0}, 70, 160, 6, 0, 0, 0, 9, 90, 0, 1, 0,
         60, kFixedScale / 8});
    set(Faction::KharuunAssemblies, EntityType::Soldier,
        {{75, 30}, 105, 176, 7, Fixed::FromRatio(3, 2).Raw(), 25, 10,
         0, 0, 0, 2, 0, 100, kFixedScale / 8});
    // Memory Hearth footprint is 5x5 tiles (buildings.json ka_memory_hearth).
    set(Faction::KharuunAssemblies, EntityType::CommandCore,
        {{380, 60}, 850, 0, 8, 0, 0, 0, 0, 0, 400, 0, 12, 0,
         5 * kFixedScale / 2});
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
        !config_.HasValidHostilityMasks() ||
        !IsValidSimulationRules(config_.rules)) {
        throw std::invalid_argument("invalid deterministic simulation configuration");
    }
    terrain_.assign(static_cast<std::size_t>(tileCount), Terrain::Open);
    for (PlayerId player = 0; player < players_.size(); ++player) {
        players_[player].id = player;
        explored_[player].assign(static_cast<std::size_t>(tileCount), 0);
        visible_[player].assign(static_cast<std::size_t>(tileCount), 0);
        // Unseen ground remembers nothing usable. Blocked is the safe
        // default: an unexplored tile must never read as known-open.
        rememberedTerrain_[player].assign(static_cast<std::size_t>(tileCount),
                                          Terrain::Blocked);
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
    // 12.5: "Manifest grants 130% damage. Possible grants 130% movement and
    // 125% vision." The 160-tick public transition is neither identity, so it
    // grants neither bonus. Holding both while DualResolve* made the declared
    // liability window the unit's strongest state; the unit now pays base
    // stats for the whole publicly visible transition.
    const bool manifest =
        entity.choirIdentityState == ChoirIdentityState::Manifest;
    const bool possible =
        entity.choirIdentityState == ChoirIdentityState::Possible;
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
                                 Vec2 position,
                                 std::optional<std::int32_t> initialHitPoints) {
    if (!IsInsideMap(position) || owner == kNeutralPlayer ||
        FindPlayer(owner) == nullptr || players_[owner].faction != faction ||
        !IsValidFaction(faction) || !IsValidEntityType(type) ||
        type == EntityType::ResourceNode || type == EntityType::FutureWell) {
        return 0;
    }
    Entity entity = MakeEntity(owner, faction, type, position);
    if (initialHitPoints.has_value()) {
        if (*initialHitPoints <= 0 || *initialHitPoints > entity.maxHitPoints) {
            return 0;
        }
        entity.hitPoints = *initialHitPoints;
    }
    if (IsChoirCoherenceStructure(entity)) {
        entity.choirCoherenceNextChargeTick = std::min(
            kMaximumSupportedTick,
            currentTick_ + config_.rules.choirCoherence.upkeepIntervalTicks);
    }
    if (!TryAllocateEntityId(entity.id)) {
        return 0;
    }
    entities_.push_back(entity);
    MarkStructureOccupancyDirty();
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
    MarkStructureOccupancyDirty();
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
    MarkStructureOccupancyDirty();
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
    MarkStructureOccupancyDirty();
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
    // Terrain only, deliberately. A completed structure occupies ground for
    // the purpose of MOVING THROUGH it -- see IsPositionPassableFor -- but not
    // for the purpose of naming a destination. Folding occupancy in here
    // refused every authored order aimed at a building or the ground it clips,
    // because the order gates ask this question, and a right-click on a
    // building has always meant "walk up to it", never "that is illegal".
    return TerrainAt(tileX, tileY) != Terrain::Blocked ||
           IsReshapedOpen(tileX, tileY);
}

bool Simulation::IsGroundOpen(Vec2 position) const {
    if (!IsInsideMap(position)) {
        return false;
    }
    const std::int32_t tileX = position.x.FloorToInt();
    const std::int32_t tileY = position.y.FloorToInt();
    return TerrainAt(tileX, tileY) != Terrain::Blocked ||
           IsReshapedOpen(tileX, tileY);
}

void Simulation::MarkStructureOccupancyDirty() {
    structureOccupancyDirty_ = true;
    // The cached path field bakes structure passability, so it retires with it.
    pathFieldCache_.clear();
}

void Simulation::EnsureStructureOccupancy() const {
    const std::size_t tileCount =
        static_cast<std::size_t>(config_.mapWidthTiles) *
        static_cast<std::size_t>(config_.mapHeightTiles);
    if (!structureOccupancyDirty_ && structureOccupancy_.size() == tileCount) {
        return;
    }
    structureOccupancy_.assign(tileCount, 0);
    for (const Entity& entity : entities_) {
        if (entity.hitPoints <= 0 || !entity.completed ||
            !IsBuildingType(entity.type)) {
            continue;
        }
        // A Kharuun Waystone under way is a travelling body, not standing
        // ground: it occupies tiles through the mobile grid instead, so it
        // never blocks itself out of its own migration.
        if (entity.waystoneMode == WaystoneMode::Mobile) {
            continue;
        }
        const std::int32_t halfExtent =
            FootprintHalfExtentRaw(entity.faction, entity.type);
        const std::int32_t minimumTileX = std::max(
            0, (entity.position.x.Raw() - halfExtent) / kFixedScale);
        const std::int32_t maximumTileX = std::min(
            config_.mapWidthTiles - 1,
            (entity.position.x.Raw() + halfExtent - 1) / kFixedScale);
        const std::int32_t minimumTileY = std::max(
            0, (entity.position.y.Raw() - halfExtent) / kFixedScale);
        const std::int32_t maximumTileY = std::min(
            config_.mapHeightTiles - 1,
            (entity.position.y.Raw() + halfExtent - 1) / kFixedScale);
        for (std::int32_t tileY = minimumTileY; tileY <= maximumTileY; ++tileY) {
            for (std::int32_t tileX = minimumTileX; tileX <= maximumTileX;
                 ++tileX) {
                structureOccupancy_[static_cast<std::size_t>(
                    tileY * config_.mapWidthTiles + tileX)] = 1;
            }
        }
    }
    structureOccupancyDirty_ = false;
}

bool Simulation::IsStructureBlockedAt(Vec2 position) const {
    // The tile grid is a broad phase: it marks every tile a footprint touches,
    // so a free tile is conclusive. A touched tile is then measured exactly,
    // because rounding a 2.5-tile footprint up to whole tiles would push a
    // hauler or attacker out of the interaction range it must reach.
    if (!IsStructureOccupiedTile(position.x.FloorToInt(),
                                 position.y.FloorToInt())) {
        return false;
    }
    for (const Entity& entity : entities_) {
        if (entity.hitPoints <= 0 || !entity.completed ||
            !IsBuildingType(entity.type) ||
            entity.waystoneMode == WaystoneMode::Mobile) {
            continue;
        }
        const std::int32_t halfExtent =
            FootprintHalfExtentRaw(entity.faction, entity.type);
        if (Abs64(static_cast<std::int64_t>(position.x.Raw()) -
                  entity.position.x.Raw()) < halfExtent &&
            Abs64(static_cast<std::int64_t>(position.y.Raw()) -
                  entity.position.y.Raw()) < halfExtent) {
            return true;
        }
    }
    return false;
}

bool Simulation::IsStructureOccupiedTile(std::int32_t tileX,
                                         std::int32_t tileY) const {
    if (tileX < 0 || tileY < 0 || tileX >= config_.mapWidthTiles ||
        tileY >= config_.mapHeightTiles) {
        return false;
    }
    EnsureStructureOccupancy();
    return structureOccupancy_[static_cast<std::size_t>(
               tileY * config_.mapWidthTiles + tileX)] != 0;
}

void Simulation::RebuildMobileOccupancy() {
    const std::size_t tileCount =
        static_cast<std::size_t>(config_.mapWidthTiles) *
        static_cast<std::size_t>(config_.mapHeightTiles);
    mobileOccupancy_.assign(tileCount, 0);
    for (const Entity& entity : entities_) {
        if (entity.hitPoints <= 0 || !entity.completed ||
            entity.movementPerTickRaw <= 0 ||
            static_cast<std::size_t>(entity.owner) >= kMaximumPlayers) {
            continue;
        }
        const std::uint8_t seat =
            static_cast<std::uint8_t>(1U << entity.owner);
        const std::int32_t halfExtent =
            FootprintHalfExtentRaw(entity.faction, entity.type);
        const std::int32_t minimumTileX = std::max(
            0, (entity.position.x.Raw() - halfExtent) / kFixedScale);
        const std::int32_t maximumTileX = std::min(
            config_.mapWidthTiles - 1,
            (entity.position.x.Raw() + halfExtent - 1) / kFixedScale);
        const std::int32_t minimumTileY = std::max(
            0, (entity.position.y.Raw() - halfExtent) / kFixedScale);
        const std::int32_t maximumTileY = std::min(
            config_.mapHeightTiles - 1,
            (entity.position.y.Raw() + halfExtent - 1) / kFixedScale);
        for (std::int32_t tileY = minimumTileY; tileY <= maximumTileY; ++tileY) {
            for (std::int32_t tileX = minimumTileX; tileX <= maximumTileX;
                 ++tileX) {
                mobileOccupancy_[static_cast<std::size_t>(
                    tileY * config_.mapWidthTiles + tileX)] |= seat;
            }
        }
    }
}

std::uint8_t Simulation::MobileOwnerMaskAt(std::int32_t tileX,
                                           std::int32_t tileY) const {
    if (tileX < 0 || tileY < 0 || tileX >= config_.mapWidthTiles ||
        tileY >= config_.mapHeightTiles) {
        return 0;
    }
    const std::size_t tile =
        static_cast<std::size_t>(tileY * config_.mapWidthTiles + tileX);
    if (tile >= mobileOccupancy_.size()) {
        return 0;
    }
    return mobileOccupancy_[tile];
}

bool Simulation::IsPositionPassableFor(PlayerId mover, Vec2 position) const {
    if (!IsPositionPassable(position)) {
        return false;
    }
    if (legacyOpenGroundReplaySemantics_) {
        return true;
    }
    // SPEC-MOV-006: a completed structure occupies its ground absolutely for
    // anything trying to stand on it.
    if (IsStructureBlockedAt(position)) {
        return false;
    }
    const std::uint8_t occupants = MobileOwnerMaskAt(position.x.FloorToInt(),
                                                     position.y.FloorToInt());
    if (occupants == 0) {
        return true;
    }
    if (static_cast<std::size_t>(mover) >= kMaximumPlayers) {
        // A seatless query asks about static ground only: terrain and
        // structures. Bodies belong to whoever is moving.
        return true;
    }
    const std::uint8_t own = static_cast<std::uint8_t>(1U << mover);
    // Only a foreign seat's unit blocks; an allied body is pushed past by
    // ApplySoftSeparation so a friendly column never imprisons itself.
    return (occupants & static_cast<std::uint8_t>(~own)) == 0;
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
        if (entity.hitPoints <= 0 || IsCollapsedFutureWell(entity)) {
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

std::int32_t Simulation::MobileEntityCount(PlayerId player) const {
    if (FindPlayer(player) == nullptr) {
        return 0;
    }
    return CountMobileEntities(entities_, player);
}

std::int32_t Simulation::MobileEntityReservations(PlayerId player) const {
    if (FindPlayer(player) == nullptr) {
        return 0;
    }
    return CountMobileEntityReservations(entities_, player);
}

std::int32_t Simulation::BasePopulationUsed(PlayerId player) const {
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

std::int32_t Simulation::CommittedBandSurcharge(PlayerId player) const {
    if (legacyFiringLaneReplaySemantics_) {
        return 0;
    }
    const std::int32_t base = BasePopulationUsed(player);
    return std::max(0, base - kCommittedBandThreshold) / 2;
}

std::int32_t Simulation::PopulationUsed(PlayerId player) const {
    // REL-ECO-011.BAND: the fielded army above the committed threshold costs
    // more to hold. Reservations in production keep their admitted cost; the
    // surcharge lands when the unit stands on the field.
    return SaturatingAdd(BasePopulationUsed(player),
                         CommittedBandSurcharge(player));
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
    // SPEC-BUD-006: the authored 200 Logistics ceiling. Supply structures are
    // additive and unbounded, so without this a player who spends on nothing but
    // depots raises the army ceiling past the load the 400-unit performance
    // budget is qualified against.
    return std::min(capacity, legacyFiringLaneReplaySemantics_
                                  ? kLegacyMaximumPopulationCapacity
                                  : kMaximumPopulationCapacity);
}

bool Simulation::IsOperationalDropoff(const Entity& entity) const {
    return entity.type == EntityType::CommandCore ||
           (entity.type == EntityType::Dropoff &&
            (entity.faction == Faction::KharuunAssemblies
                 ? entity.waystoneMode == WaystoneMode::Rooted
                 : entity.faction != Faction::MeridianCompact ||
                       legacyLinkReplaySemantics_ ||
                       entity.networkOperational));
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

bool Simulation::IsProducerPowered(const Entity& producer) const {
    // REL-FAC-002.PROD (owner ruling 2026-09-11): only a Meridian Foundry is
    // gated, and only under current rules; recordings older than schema 32
    // and legacy Link semantics keep the ungated production they were made
    // with so retained replays still reproduce.
    if (legacyLinkReplaySemantics_ || legacyPoweredProductionReplaySemantics_ ||
        producer.faction != Faction::MeridianCompact ||
        producer.type != EntityType::Barracks) {
        return true;
    }
    return producer.networkOperational;
}

bool Simulation::IsPositionInMeridianNetwork(PlayerId player,
                                             Vec2 position) const {
    if (legacyLinkReplaySemantics_) {
        return true;
    }
    const std::int64_t radius = config_.rules.poweredAegis.connectionRadiusRaw;
    const std::uint64_t radiusSquared =
        static_cast<std::uint64_t>(radius * radius);
    return std::any_of(entities_.begin(), entities_.end(),
                       [&](const Entity& node) {
        return node.owner == player && node.completed && node.hitPoints > 0 &&
               node.faction == Faction::MeridianCompact &&
               node.networkOperational &&
               (node.type == EntityType::CommandCore ||
                node.type == EntityType::Dropoff ||
                node.type == EntityType::Barracks) &&
               DistanceSquaredRaw(position, node.position) <= radiusSquared;
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
                    (candidate.type == EntityType::Dropoff &&
                     IsOperationalDropoff(candidate))) &&
                   DistanceSquaredRaw(relay.position, candidate.position) <=
                       radiusSquared;
        });
}

RepairResult Simulation::ValidateRepair(
    PlayerId player, EntityId workerId, EntityId targetId) const {
    const Entity* worker = FindEntity(workerId);
    const Entity* target = FindEntity(targetId);
    if (FindPlayer(player) == nullptr || worker == nullptr ||
        worker->owner != player || !worker->completed || worker->hitPoints <= 0 ||
        worker->type != EntityType::Worker) {
        return RepairResult::InvalidWorker;
    }
    if (target == nullptr || target->hitPoints <= 0 || targetId == workerId ||
        target->owner == kNeutralPlayer || config_.IsHostile(player, target->owner) ||
        (!target->completed && !IsBuilding(target->type)) ||
        (worker->faction != Faction::MeridianCompact &&
         (!target->completed || !IsBuilding(target->type)))) {
        return RepairResult::InvalidTarget;
    }
    if (!IsEntityVisibleTo(player, targetId)) return RepairResult::TargetNotVisible;
    if (target->hitPoints >= target->maxHitPoints) return RepairResult::Undamaged;
    if (worker->faction == Faction::MeridianCompact &&
        !IsPositionInMeridianNetwork(player, worker->position)) {
        return RepairResult::Disconnected;
    }
    return RepairResult::Valid;
}

ConstructionAssistResult Simulation::ValidateConstructionAssist(
    PlayerId player, EntityId workerId, EntityId siteId) const {
    const Entity* worker = FindEntity(workerId);
    const Entity* site = FindEntity(siteId);
    if (FindPlayer(player) == nullptr || worker == nullptr || worker->owner != player ||
        !worker->completed || worker->hitPoints <= 0 || worker->type != EntityType::Worker) {
        return ConstructionAssistResult::InvalidWorker;
    }
    if (worker->order.type == OrderType::Build) return ConstructionAssistResult::WorkerBusy;
    if (site == nullptr || site->owner != player || site->hitPoints <= 0 ||
        !IsBuilding(site->type)) return ConstructionAssistResult::InvalidSite;
    if (site->completed) return ConstructionAssistResult::SiteComplete;
    return ConstructionAssistResult::Valid;
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
    if (!IsProducerPowered(*building)) {
        return ProductionResult::ProducerUnpowered;
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
    if (building->productionRequired > 0 ||
        !building->productionQueue.empty()) {
        if (legacyProductionReplaySemantics_) {
            return ProductionResult::ProducerBusy;
        }
        return building->productionQueue.size() < Entity::kMaxProductionQueue
                   ? ProductionResult::Valid
                   : ProductionResult::QueueFull;
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
                entity.productionLogisticsCost);
        }
    }
    if (SaturatingAdd(
            committedPopulation, PopulationCost(playerState->faction, unitType)) >
        PopulationCapacity(player)) {
        return ProductionResult::CapacityReached;
    }

    // SPEC-RES-008: fielded plus reserved mobile entities. Reported apart
    // from Logistics so "army limit reached" never reads as "build a drop-off".
    if (IsMobileEntityType(unitType) &&
        MobileEntityCount(player) + MobileEntityReservations(player) >=
            kMobileEntityLimit) {
        return ProductionResult::MobileEntityLimitReached;
    }
    if (entities_.size() >= kMaximumSerializedEntities || nextEntityId_ == 0 ||
        nextEntityId_ == std::numeric_limits<EntityId>::max() ||
        nextProductionItemId_ == 0 ||
        nextProductionItemId_ ==
            std::numeric_limits<ProductionItemId>::max()) {
        return ProductionResult::EntityCapacityReached;
    }
    return ProductionResult::Valid;
}

ProductionStartBlockReason Simulation::ProductionStartBlockReasonFor(
    PlayerId player,
    EntityId producer,
    EntityType unitType) const {
    const PlayerState* playerState = FindPlayer(player);
    const Entity* building = FindEntity(producer);
    if (playerState == nullptr || building == nullptr ||
        building->owner != player || building->hitPoints <= 0) {
        return ProductionStartBlockReason::InvalidProducer;
    }
    if (!building->completed) {
        return ProductionStartBlockReason::ProducerIncomplete;
    }
    const bool supported =
        (building->type == EntityType::CommandCore &&
         unitType == EntityType::Worker) ||
        (building->type == EntityType::Barracks &&
         IsBarracksUnitType(unitType));
    if (!supported) {
        return ProductionStartBlockReason::UnsupportedUnit;
    }
    if (!IsProducerPowered(*building)) {
        return ProductionStartBlockReason::Unpowered;
    }
    if (playerState->activeResearch != ResearchType::None &&
        playerState->researchProducer == producer) {
        return ProductionStartBlockReason::Busy;
    }
    if (building->productionRequired > 0 ||
        !building->productionQueue.empty()) {
        return building->productionQueue.size() >= Entity::kMaxProductionQueue
                   ? ProductionStartBlockReason::QueueFull
                   : ProductionStartBlockReason::Busy;
    }
    const ResourcePool cost = ProductionCost(playerState->faction, unitType);
    if (playerState->resources.material < cost.material) {
        return ProductionStartBlockReason::InsufficientMatter;
    }
    if (playerState->resources.dawnshards < cost.dawnshards) {
        return ProductionStartBlockReason::InsufficientDawn;
    }
    std::int32_t committedPopulation = PopulationUsed(player);
    for (const Entity& entity : entities_) {
        if (entity.owner == player && entity.productionRequired > 0) {
            committedPopulation = SaturatingAdd(
                committedPopulation, entity.productionLogisticsCost);
        }
    }
    if (SaturatingAdd(
            committedPopulation,
            PopulationCost(playerState->faction, unitType)) >
        PopulationCapacity(player)) {
        return ProductionStartBlockReason::LogisticsCapacity;
    }

    if (IsMobileEntityType(unitType) &&
        MobileEntityCount(player) + MobileEntityReservations(player) >=
            kMobileEntityLimit) {
        return ProductionStartBlockReason::MobileEntityLimit;
    }
    if (entities_.size() >= kMaximumSerializedEntities || nextEntityId_ == 0 ||
        nextEntityId_ == std::numeric_limits<EntityId>::max() ||
        nextProductionItemId_ == 0 ||
        nextProductionItemId_ ==
            std::numeric_limits<ProductionItemId>::max()) {
        return ProductionStartBlockReason::EntityCapacity;
    }
    return ProductionStartBlockReason::None;
}

std::optional<ProducerQueueState> Simulation::ProducerQueueStateFor(
    PlayerId player,
    EntityId producer) const {
    const Entity* building = FindEntity(producer);
    if (FindPlayer(player) == nullptr || building == nullptr ||
        building->owner != player || building->hitPoints <= 0 ||
        !building->completed ||
        (building->type != EntityType::CommandCore &&
         building->type != EntityType::Barracks)) {
        return std::nullopt;
    }
    ProducerQueueState state{};
    state.producer = producer;
    state.active = building->productionRequired > 0;
    if (state.active) {
        state.activeItem.itemId = building->activeProductionItemId;
        state.activeItem.unitType = building->productionType;
        state.activeItem.configuredCost = building->productionInvestedCost;
        state.activeItem.requiredTicks = building->productionRequired;
        state.activeItem.logisticsCost = building->productionLogisticsCost;
        state.activeItem.investedCost = building->productionInvestedCost;
        state.activeProgress = building->productionProgress;
    }
    state.spawnBlockedTicks = building->productionSpawnBlockedTicks;
    state.pausedForSpawn = building->productionPausedForSpawn;
    state.spawnBlockedAlert = building->productionSpawnBlockedAlert;
    state.rallyAlert = building->rallyRouteAlert;
    state.unpowered = !IsProducerPowered(*building);
    state.waiting = building->productionQueue;
    state.rallyRoute = building->rallyRoute;
    return state;
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
        // OUT-001/OUT-002: only a *surviving* Core keeps a player alive. An
        // incomplete construction site is not a Core yet (BLD-004 grants it no
        // structure function), so it may not postpone Corefall.
        if (entity.owner < hasCommandCore.size() &&
            players_[entity.owner].active && entity.hitPoints > 0 &&
            entity.completed && entity.type == EntityType::CommandCore) {
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
    replayForfeitingPlayer_ = player;
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
    if (!IsConstructableBuildingType(buildingType)) {
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
        if (IsCollapsedFutureWell(entity)) {
            continue;
        }
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

bool Simulation::IsExecutableCommandTick(Tick tick) {
    return tick < kMaximumSupportedTick;
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
        (legacyProductionReplaySemantics_ &&
         command.type > CommandType::ReconcileToPossible) ||
        (legacyLinkReplaySemantics_ &&
         command.type > CommandType::SetRallyRoute) ||
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

bool Simulation::PrepareReplayCommandSchedule(
    const ReplayRecord& replay,
    ReplayCommandSchedule& schedule,
    std::string* rejectionReason,
    const ReplayCancellationCheck& shouldCancel) {
    schedule = {};
    if (rejectionReason != nullptr) {
        rejectionReason->clear();
    }
    if (replay.commands.size() >
            kMaximumSerializedCommands - pendingCommands_.size() ||
        replay.commands.size() >
            kMaximumSerializedCommands - commandLog_.size()) {
        SetError(rejectionReason, "command capacity is exhausted");
        return false;
    }

    schedule.baselinePending = pendingCommands_;
    schedule.recorded = replay.commands;
    std::sort(schedule.baselinePending.begin(),
              schedule.baselinePending.end(), CommandLess);
    std::sort(schedule.recorded.begin(), schedule.recorded.end(), CommandLess);
    if (shouldCancel && shouldCancel()) {
        SetError(rejectionReason, "replay validation cancelled");
        schedule = {};
        return false;
    }

    std::array<std::vector<const Command*>, kMaximumPlayers> byPlayer{};
    std::size_t commandIndex = 0;
    const auto Collect = [&](const std::vector<Command>& commands,
                             bool validateEncoding) {
        for (const Command& command : commands) {
            if ((commandIndex++ & 0xffU) == 0U &&
                shouldCancel && shouldCancel()) {
                SetError(rejectionReason, "replay validation cancelled");
                return false;
            }
            if (command.player >= kMaximumPlayers ||
                FindPlayer(command.player) == nullptr) {
                SetError(rejectionReason, "command player is not active");
                return false;
            }
            if (command.executeTick < currentTick_ ||
                command.executeTick > kMaximumSupportedTick) {
                SetError(rejectionReason,
                         "command tick is outside the supported range");
                return false;
            }
            if (validateEncoding &&
                (!IsValidCommandType(command.type) ||
                 (replay.version < kProductionReplayVersion &&
                  command.type > CommandType::ReconcileToPossible) ||
                 (replay.version < kLinkMechanicsReplayVersion &&
                  command.type > CommandType::SetRallyRoute) ||
                 !IsValidEntityType(command.buildType) ||
                 !IsValidWellChoice(command.wellChoice) ||
                 !IsValidWarformAdaptation(command.warformAdaptation) ||
                 !IsValidResearchType(command.researchType) ||
                 command.actor == 0)) {
                SetError(rejectionReason, "command encoding is invalid");
                return false;
            }
            byPlayer[command.player].push_back(&command);
        }
        return true;
    };
    // Snapshot loading has already validated baseline-pending encodings. They
    // still participate in the sequence frontier shared with recorded input.
    if (!Collect(schedule.baselinePending, false) ||
        !Collect(schedule.recorded, true)) {
        schedule = {};
        return false;
    }

    for (PlayerId player = 0; player < kMaximumPlayers; ++player) {
        std::vector<const Command*>& commands = byPlayer[player];
        std::sort(
            commands.begin(), commands.end(),
            [](const Command* lhs, const Command* rhs) {
                return CommandLess(*lhs, *rhs);
            });
        Tick priorTick = 0;
        std::uint64_t priorTickMaximumSequence = 0;
        bool hasPriorTick = false;
        std::uint64_t previousSequenceAtTick = 0;
        bool hasSequenceAtTick = false;
        for (std::size_t index = 0; index < commands.size(); ++index) {
            if ((index & 0xffU) == 0U &&
                shouldCancel && shouldCancel()) {
                SetError(rejectionReason, "replay validation cancelled");
                schedule = {};
                return false;
            }
            const Command& command = *commands[index];
            if (hasExecutedSequence_[player] &&
                command.sequence <= lastExecutedSequence_[player]) {
                SetError(rejectionReason,
                         "command sequence is not newer than executed input");
                schedule = {};
                return false;
            }
            if (!hasPriorTick || command.executeTick != priorTick) {
                if (hasPriorTick &&
                    command.sequence <= priorTickMaximumSequence) {
                    SetError(rejectionReason,
                             "command sequence must increase across execution ticks");
                    schedule = {};
                    return false;
                }
                priorTick = command.executeTick;
                priorTickMaximumSequence = command.sequence;
                hasPriorTick = true;
                previousSequenceAtTick = command.sequence;
                hasSequenceAtTick = true;
                continue;
            }
            if (hasSequenceAtTick && command.sequence == previousSequenceAtTick) {
                SetError(rejectionReason,
                         "player and sequence must identify one command");
                schedule = {};
                return false;
            }
            previousSequenceAtTick = command.sequence;
            priorTickMaximumSequence = command.sequence;
        }
    }

    if (shouldCancel && shouldCancel()) {
        SetError(rejectionReason, "replay validation cancelled");
        schedule = {};
        return false;
    }
    pendingCommands_.clear();
    commandLog_.clear();
    replayInitialSnapshot_ = replay.initialSnapshot;
    replayForfeitingPlayer_ = kNeutralPlayer;
    return true;
}

void Simulation::AdmitPreparedReplayCommand(const Command& command,
                                            bool recorded) {
    pendingCommands_.push_back(command);
    if (recorded) {
        commandLog_.push_back(command);
    }
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

bool Simulation::InStructureReach(const Entity& worker,
                                  const Entity& structure,
                                  std::int32_t extraRangeRaw) const {
    // Reproduced 2026-09-11 (M01 live Gather regression): the Core is 5x5, so
    // its corner sits 2.5*sqrt(2) = 3.54 tiles from the centre while the
    // circular reach was 0.5 + 2.5 + 0.125 = 3.13 tiles. A Surveyor returning
    // from a deposit on the diagonal stopped at the corner, MoveTowards could
    // not enter the footprint, and the load was never credited. Measuring to
    // the footprint box makes every side and corner reachable at the same
    // clearance and leaves small footprints (units, deposits) unchanged.
    const std::int64_t half =
        FootprintHalfExtentRaw(structure.faction, structure.type);
    const std::int64_t reach =
        static_cast<std::int64_t>(extraRangeRaw) +
        FootprintHalfExtentRaw(worker.faction, worker.type);
    const std::int64_t offsetX = std::max<std::int64_t>(
        0,
        Abs64(static_cast<std::int64_t>(worker.position.x.Raw()) -
              structure.position.x.Raw()) - half);
    const std::int64_t offsetY = std::max<std::int64_t>(
        0,
        Abs64(static_cast<std::int64_t>(worker.position.y.Raw()) -
              structure.position.y.Raw()) - half);
    return offsetX * offsetX + offsetY * offsetY <= reach * reach;
}

bool Simulation::IsTileKnownPassableTo(PlayerId player,
                                       std::int32_t tileX,
                                       std::int32_t tileY) const {
    if (tileX < 0 || tileY < 0 || tileX >= config_.mapWidthTiles ||
        tileY >= config_.mapHeightTiles) {
        return false;
    }
    const Vec2 position = Vec2::FromTiles(tileX, tileY);
    const Visibility visibility = VisibilityAt(player, position);
    if (visibility == Visibility::Unexplored) {
        // MOV-002 pathing works from known passability. Ground this player has
        // never observed is assumed open, so an order they cannot yet disprove
        // stays admissible and no receipt reveals what is really there.
        return true;
    }
    if (visibility == Visibility::Visible) {
        // Judged at the same tile granularity the route grid uses, so a
        // movement receipt agrees with the route the unit would actually be
        // given: a tile a structure sits on is not a tile to route through.
        // Naming such a tile as a DESTINATION is a separate question, answered
        // by IsTileKnownGroundOpenTo below.
        return IsGroundOpen(position) &&
               (legacyOpenGroundReplaySemantics_ ||
                !IsStructureOccupiedTile(tileX, tileY));
    }
    // Movement admission consumes the same last-observed ground as the
    // player view. Live terrain changes behind fog cannot alter a receipt.
    return rememberedTerrain_[player][static_cast<std::size_t>(
        tileY * config_.mapWidthTiles + tileX)] != Terrain::Blocked;
}

bool Simulation::IsTileKnownGroundOpenTo(PlayerId player,
                                         std::int32_t tileX,
                                         std::int32_t tileY) const {
    // Destination legality asks only whether the ground is open, never whether
    // a building stands on it. Ordering a unit to a building means "walk up to
    // it"; the unit halts at the footprint edge because the step gate stops it,
    // and refusing the order outright instead broke every authored campaign
    // route aimed at a site that a structure clips.
    if (tileX < 0 || tileY < 0 || tileX >= config_.mapWidthTiles ||
        tileY >= config_.mapHeightTiles) {
        return false;
    }
    const Vec2 position = Vec2::FromTiles(tileX, tileY);
    const Visibility visibility = VisibilityAt(player, position);
    if (visibility == Visibility::Unexplored) {
        return true;
    }
    if (visibility == Visibility::Visible) {
        return IsGroundOpen(position);
    }
    return rememberedTerrain_[player][static_cast<std::size_t>(
        tileY * config_.mapWidthTiles + tileX)] != Terrain::Blocked;
}

bool Simulation::IsTileReachableInPlayerKnowledge(
    PlayerId player,
    std::int32_t startTileX,
    std::int32_t startTileY,
    std::int32_t goalTileX,
    std::int32_t goalTileY) const {
    if (startTileX == goalTileX && startTileY == goalTileY) {
        return true;
    }
    const std::size_t width = static_cast<std::size_t>(config_.mapWidthTiles);
    const std::size_t tileCount =
        width * static_cast<std::size_t>(config_.mapHeightTiles);
    const auto tileIndex = [width](std::int32_t tileX, std::int32_t tileY) {
        return static_cast<std::size_t>(tileY) * width +
               static_cast<std::size_t>(tileX);
    };
    const std::size_t goal = tileIndex(goalTileX, goalTileY);

    // A building is a destination even though it is never a route. Its centre
    // tile can sit several tiles inside its own footprint, so no route tile is
    // ever orthogonally adjacent to it and a plain flood declares it
    // unreachable -- which refused every authored order aimed at a structure.
    // Reaching the ground at the footprint's edge is reaching the building.
    std::int32_t haloMinX = goalTileX;
    std::int32_t haloMaxX = goalTileX;
    std::int32_t haloMinY = goalTileY;
    std::int32_t haloMaxY = goalTileY;
    bool goalCarriesStructure = false;
    if (!legacyOpenGroundReplaySemantics_ &&
        IsStructureOccupiedTile(goalTileX, goalTileY)) {
        for (const Entity& occupant : entities_) {
            if (occupant.hitPoints <= 0 || !occupant.completed ||
                !IsBuildingType(occupant.type) ||
                occupant.waystoneMode == WaystoneMode::Mobile) {
                continue;
            }
            const std::int32_t extent =
                FootprintHalfExtentRaw(occupant.faction, occupant.type);
            const std::int32_t minX =
                (occupant.position.x.Raw() - extent) / kFixedScale;
            const std::int32_t maxX =
                (occupant.position.x.Raw() + extent - 1) / kFixedScale;
            const std::int32_t minY =
                (occupant.position.y.Raw() - extent) / kFixedScale;
            const std::int32_t maxY =
                (occupant.position.y.Raw() + extent - 1) / kFixedScale;
            if (goalTileX < minX || goalTileX > maxX || goalTileY < minY ||
                goalTileY > maxY) {
                continue;
            }
            haloMinX = minX - 1;
            haloMaxX = maxX + 1;
            haloMinY = minY - 1;
            haloMaxY = maxY + 1;
            goalCarriesStructure = true;
            break;
        }
    }
    const auto ReachesGoal = [&](std::size_t tile, std::int32_t tileX,
                                 std::int32_t tileY) {
        if (tile == goal) {
            return true;
        }
        return goalCarriesStructure && tileX >= haloMinX && tileX <= haloMaxX &&
               tileY >= haloMinY && tileY <= haloMaxY;
    };
    if (ReachesGoal(tileIndex(startTileX, startTileY), startTileX,
                    startTileY)) {
        return true;
    }

    // Deterministic breadth-first flood in the same N/E/S/W order the
    // authoritative path field expands, over player-known passability only.
    std::vector<std::uint8_t> reached(tileCount, 0);
    std::vector<std::size_t> frontier{};
    frontier.reserve(std::min<std::size_t>(tileCount, 4096));
    reached[tileIndex(startTileX, startTileY)] = 1;
    frontier.push_back(tileIndex(startTileX, startTileY));
    constexpr std::array<std::array<std::int32_t, 2>, 4> directions{{
        {{0, -1}},
        {{1, 0}},
        {{0, 1}},
        {{-1, 0}},
    }};
    std::size_t head = 0;
    while (head < frontier.size()) {
        const std::size_t current = frontier[head++];
        const std::int32_t currentX = static_cast<std::int32_t>(current % width);
        const std::int32_t currentY = static_cast<std::int32_t>(current / width);
        for (const auto& direction : directions) {
            const std::int32_t nextX = currentX + direction[0];
            const std::int32_t nextY = currentY + direction[1];
            if (nextX < 0 || nextY < 0 || nextX >= config_.mapWidthTiles ||
                nextY >= config_.mapHeightTiles) {
                continue;
            }
            const std::size_t next = tileIndex(nextX, nextY);
            if (reached[next] != 0) {
                continue;
            }
            if (!IsTileKnownPassableTo(player, nextX, nextY)) {
                continue;
            }
            if (ReachesGoal(next, nextX, nextY)) {
                return true;
            }
            reached[next] = 1;
            frontier.push_back(next);
        }
    }
    return false;
}

CommandResolutionOutcome Simulation::ValidateMoveOrder(PlayerId player,
                                                       EntityId actorId,
                                                       Vec2 destination) const {
    const Entity* actor = FindEntity(actorId);
    // These refusals predate reason codes and stay NoEffect so existing
    // receipts are unchanged; only genuinely unreachable ground gains a code.
    if (actor == nullptr || actor->owner != player || !actor->completed ||
        actor->hitPoints <= 0 || actor->movementPerTickRaw <= 0) {
        return CommandResolutionOutcome::NoEffect;
    }
    if (actor->waystoneMode != WaystoneMode::NotWaystone &&
        actor->waystoneMode != WaystoneMode::Mobile) {
        return CommandResolutionOutcome::NoEffect;
    }
    if (!IsInsideMap(destination)) {
        return CommandResolutionOutcome::NoEffect;
    }

    const std::int32_t startX = actor->position.x.FloorToInt();
    const std::int32_t startY = actor->position.y.FloorToInt();
    const std::int32_t goalX = destination.x.FloorToInt();
    const std::int32_t goalY = destination.y.FloorToInt();
    if (startX == goalX && startY == goalY) {
        return CommandResolutionOutcome::Applied;
    }
    if (!IsTileKnownGroundOpenTo(player, goalX, goalY)) {
        return CommandResolutionOutcome::DestinationOccupied;
    }
    // A unit whose own tile has no known-open neighbour cannot start any route.
    // Checked before connectivity so an enclosed unit names the obstruction
    // beside it instead of reporting a map-wide absence of routes.
    bool hasOpenNeighbour = false;
    constexpr std::array<std::array<std::int32_t, 2>, 4> directions{{
        {{0, -1}},
        {{1, 0}},
        {{0, 1}},
        {{-1, 0}},
    }};
    for (const auto& direction : directions) {
        if (IsTileKnownPassableTo(player, startX + direction[0],
                                  startY + direction[1])) {
            hasOpenNeighbour = true;
            break;
        }
    }
    std::int32_t routeStartX = startX;
    std::int32_t routeStartY = startY;
    if (!hasOpenNeighbour) {
        // Schema 31 (SPEC-MOV-006/008): the unit may stand on open ground the
        // tile grid masks on every side, such as the half-tile gap between a
        // Core and a supply node. Judge the route from the nearest tile it can
        // reach across that ground; only truly enclosed ground is refused.
        std::optional<std::pair<std::int32_t, std::int32_t>> escape{};
        if (!legacyMaskedCorridorReplaySemantics_) {
            escape = FindKnownMaskedGroundEscape(player, startX, startY);
        }
        if (!escape.has_value()) {
            return CommandResolutionOutcome::RouteBlocked;
        }
        routeStartX = escape->first;
        routeStartY = escape->second;
        if (routeStartX == goalX && routeStartY == goalY) {
            return CommandResolutionOutcome::Applied;
        }
    }
    if (!IsTileReachableInPlayerKnowledge(player, routeStartX, routeStartY,
                                          goalX, goalY)) {
        return CommandResolutionOutcome::NoPath;
    }
    return CommandResolutionOutcome::Applied;
}

std::optional<std::pair<std::int32_t, std::int32_t>>
Simulation::FindKnownMaskedGroundEscape(PlayerId player,
                                        std::int32_t startTileX,
                                        std::int32_t startTileY) const {
    // Player knowledge only (FOG-001): a tile the player currently sees is
    // measured at its centre against terrain and the exact boxes of the
    // structures standing in it, which that player can see; any other tile
    // keeps the tile-level knowledge rule.
    constexpr std::int32_t kEscapeRadiusTiles = 8;
    const std::int32_t minX = std::max(0, startTileX - kEscapeRadiusTiles);
    const std::int32_t maxX = std::min(config_.mapWidthTiles - 1, startTileX + kEscapeRadiusTiles);
    const std::int32_t minY = std::max(0, startTileY - kEscapeRadiusTiles);
    const std::int32_t maxY = std::min(config_.mapHeightTiles - 1, startTileY + kEscapeRadiusTiles);
    const std::size_t spanX = static_cast<std::size_t>(maxX - minX + 1);
    const std::size_t spanY = static_cast<std::size_t>(maxY - minY + 1);
    std::vector<std::uint8_t> seen(spanX * spanY, 0);
    const auto localIndex = [&](std::int32_t tileX, std::int32_t tileY) {
        return static_cast<std::size_t>(tileY - minY) * spanX +
               static_cast<std::size_t>(tileX - minX);
    };
    const auto centreKnownOpen = [&](std::int32_t tileX, std::int32_t tileY) {
        const Vec2 corner = Vec2::FromTiles(tileX, tileY);
        if (VisibilityAt(player, corner) != Visibility::Visible) {
            return IsTileKnownPassableTo(player, tileX, tileY);
        }
        const Vec2 centre = Vec2::FromRaw(tileX * kFixedScale + kFixedScale / 2,
                                          tileY * kFixedScale + kFixedScale / 2);
        return IsGroundOpen(corner) && !IsStructureBlockedAt(centre);
    };
    constexpr std::array<std::array<std::int32_t, 2>, 4> directions{{
        {{0, -1}}, {{1, 0}}, {{0, 1}}, {{-1, 0}},
    }};
    std::vector<std::pair<std::int32_t, std::int32_t>> queue{};
    queue.reserve(spanX * spanY);
    queue.emplace_back(startTileX, startTileY);
    seen[localIndex(startTileX, startTileY)] = 1;
    std::size_t head = 0;
    while (head < queue.size()) {
        const auto [currentX, currentY] = queue[head++];
        for (const auto& direction : directions) {
            const std::int32_t nextX = currentX + direction[0];
            const std::int32_t nextY = currentY + direction[1];
            if (nextX < minX || nextY < minY || nextX > maxX || nextY > maxY) {
                continue;
            }
            const std::size_t local = localIndex(nextX, nextY);
            if (seen[local] != 0) {
                continue;
            }
            seen[local] = 1;
            if (IsTileKnownPassableTo(player, nextX, nextY)) {
                return std::make_pair(nextX, nextY);
            }
            if (centreKnownOpen(nextX, nextY)) {
                queue.emplace_back(nextX, nextY);
            }
        }
    }
    return std::nullopt;
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
    // Terrain still refuses outright; structure occupancy does not, because an
    // approach to a building is an ordinary order (gather, deliver, build,
    // repair). The final step is gated by passability, so the mover halts at
    // the footprint edge instead of entering it.
    if (TerrainAt(goalX, goalY) == Terrain::Blocked &&
        !IsReshapedOpen(goalX, goalY)) {
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
        // SPEC-MOV-006: route around completed structures instead of through
        // them. The goal tile itself stays seedable even when a structure
        // stands on it, so a worker ordered to a Dropoff still walks up to it
        // and stops at its edge rather than refusing to approach at all.
        if (!legacyOpenGroundReplaySemantics_) {
            // The route grid is deliberately pessimistic: any tile a
            // footprint touches is off-route, so a path never threads a sliver
            // of open ground a unit cannot physically enter from the side it
            // approaches. Standing room is judged exactly, by
            // IsStructureBlockedAt, so a unit can still close right up to a
            // wall when its order requires it.
            EnsureStructureOccupancy();
            for (std::size_t tile = 0; tile < tileCount; ++tile) {
                if (structureOccupancy_[tile] != 0) {
                    passable[tile] = 0;
                }
            }
        }
        // Re-open the whole footprint standing on the goal, not just the goal
        // cell. A Command Core covers several tiles, so seeding one isolated
        // cell would leave the field unreachable and an attacker, builder or
        // hauler would refuse to approach the building at all.
        passable[goal] = 1;
        if (structureOccupancy_[goal] != 0) {
            for (const Entity& occupant : entities_) {
                if (occupant.hitPoints <= 0 || !occupant.completed ||
                    !IsBuildingType(occupant.type) ||
                    occupant.waystoneMode == WaystoneMode::Mobile) {
                    continue;
                }
                const std::int32_t occupantExtent =
                    FootprintHalfExtentRaw(occupant.faction, occupant.type);
                const std::int32_t occupantMinX = std::max(
                    0, (occupant.position.x.Raw() - occupantExtent) / kFixedScale);
                const std::int32_t occupantMaxX = std::min(
                    config_.mapWidthTiles - 1,
                    (occupant.position.x.Raw() + occupantExtent - 1) / kFixedScale);
                const std::int32_t occupantMinY = std::max(
                    0, (occupant.position.y.Raw() - occupantExtent) / kFixedScale);
                const std::int32_t occupantMaxY = std::min(
                    config_.mapHeightTiles - 1,
                    (occupant.position.y.Raw() + occupantExtent - 1) / kFixedScale);
                if (goalX < occupantMinX || goalX > occupantMaxX ||
                    goalY < occupantMinY || goalY > occupantMaxY) {
                    continue;
                }
                for (std::int32_t tileY = occupantMinY; tileY <= occupantMaxY;
                     ++tileY) {
                    for (std::int32_t tileX = occupantMinX;
                         tileX <= occupantMaxX; ++tileX) {
                        passable[tileIndex(tileX, tileY)] = 1;
                    }
                }
            }
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
    std::size_t startDistance = cached->second.distanceToGoal[start];
    if (startDistance == kUnvisited) {
        // The mover is standing on solid ground of its own — a unit that has
        // just emerged inside its producer's footprint, or one a finished
        // building closed over. Leave by the cheapest open tile beside it
        // instead of reporting no route at all (SPEC-MOV-008).
        constexpr std::array<std::array<std::int32_t, 2>, 4> escapes{{
            {{0, -1}}, {{1, 0}}, {{0, 1}}, {{-1, 0}},
        }};
        std::optional<Vec2> best{};
        std::size_t bestDistance = kUnvisited;
        for (const auto& escape : escapes) {
            const std::int32_t nextX = startX + escape[0];
            const std::int32_t nextY = startY + escape[1];
            if (nextX < 0 || nextY < 0 || nextX >= config_.mapWidthTiles ||
                nextY >= config_.mapHeightTiles) {
                continue;
            }
            const std::size_t neighbour =
                cached->second.distanceToGoal[tileIndex(nextX, nextY)];
            if (neighbour != kUnvisited &&
                (bestDistance == kUnvisited || neighbour < bestDistance)) {
                bestDistance = neighbour;
                best = Vec2::FromTiles(nextX, nextY);
            }
        }
        if (best.has_value() || legacyMaskedCorridorReplaySemantics_) {
            return best;
        }
        // Schema 31 (SPEC-MOV-006/008): no neighbour is field-reachable either.
        // The mover stands on open ground the field cannot see, such as the
        // half-tile corridor between a Core and a Foundry placed one tile
        // short of touching it: the step gate measures footprints exactly, so
        // a hauler can stand there, while the field masks every tile either
        // footprint touches, so the whole row is off-route on both sides.
        // Walk the masked ground tile by tile, through tile centres that are
        // exactly passable, to the nearest field-reachable tile, and take the
        // first step of that walk. Bounded, 4-connected, fixed N/E/S/W order;
        // a pure function of the state, so replay and network stay in step.
        constexpr std::int32_t kEscapeRadiusTiles = 8;
        const std::int32_t minX = std::max(0, startX - kEscapeRadiusTiles);
        const std::int32_t maxX = std::min(config_.mapWidthTiles - 1, startX + kEscapeRadiusTiles);
        const std::int32_t minY = std::max(0, startY - kEscapeRadiusTiles);
        const std::int32_t maxY = std::min(config_.mapHeightTiles - 1, startY + kEscapeRadiusTiles);
        const std::size_t spanX = static_cast<std::size_t>(maxX - minX + 1);
        const std::size_t spanY = static_cast<std::size_t>(maxY - minY + 1);
        std::vector<std::int32_t> parent(spanX * spanY, -1);
        const auto localIndex = [&](std::int32_t tileX, std::int32_t tileY) {
            return static_cast<std::size_t>(tileY - minY) * spanX +
                   static_cast<std::size_t>(tileX - minX);
        };
        const auto centreOpen = [&](std::int32_t tileX, std::int32_t tileY) {
            if ((TerrainAt(tileX, tileY) == Terrain::Blocked &&
                 !IsReshapedOpen(tileX, tileY))) {
                return false;
            }
            return !IsStructureBlockedAt(Vec2::FromRaw(
                tileX * kFixedScale + kFixedScale / 2,
                tileY * kFixedScale + kFixedScale / 2));
        };
        std::vector<std::pair<std::int32_t, std::int32_t>> queue{};
        queue.reserve(spanX * spanY);
        queue.emplace_back(startX, startY);
        parent[localIndex(startX, startY)] = static_cast<std::int32_t>(localIndex(startX, startY));
        std::size_t readHead = 0;
        while (readHead < queue.size()) {
            const auto [currentX, currentY] = queue[readHead++];
            for (const auto& escape : escapes) {
                const std::int32_t nextX = currentX + escape[0];
                const std::int32_t nextY = currentY + escape[1];
                if (nextX < minX || nextY < minY || nextX > maxX || nextY > maxY) {
                    continue;
                }
                const std::size_t local = localIndex(nextX, nextY);
                if (parent[local] != -1) {
                    continue;
                }
                if (cached->second.distanceToGoal[tileIndex(nextX, nextY)] != kUnvisited) {
                    // Field-reachable: retrace to the first step out of the start.
                    std::int32_t stepX = nextX;
                    std::int32_t stepY = nextY;
                    std::int32_t backX = currentX;
                    std::int32_t backY = currentY;
                    while (!(backX == startX && backY == startY)) {
                        stepX = backX;
                        stepY = backY;
                        const std::int32_t previous = parent[localIndex(backX, backY)];
                        backX = minX + static_cast<std::int32_t>(previous % static_cast<std::int32_t>(spanX));
                        backY = minY + static_cast<std::int32_t>(previous / static_cast<std::int32_t>(spanX));
                    }
                    return Vec2::FromTiles(stepX, stepY);
                }
                if (!centreOpen(nextX, nextY)) {
                    continue;
                }
                parent[local] = static_cast<std::int32_t>(localIndex(currentX, currentY));
                queue.emplace_back(nextX, nextY);
            }
        }
        return best;
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

bool Simulation::HasLineOfSight(Vec2 start, Vec2 end, std::int32_t halfExtent) const {
    const std::int64_t deltaX = static_cast<std::int64_t>(end.x.Raw()) - start.x.Raw();
    const std::int64_t deltaY = static_cast<std::int64_t>(end.y.Raw()) - start.y.Raw();
    const std::int64_t distSq = deltaX * deltaX + deltaY * deltaY;
    if (distSq == 0) {
        return true;
    }
    const std::int64_t dist = IntegerSqrt64(distSq);
    constexpr std::int64_t kSampleStep = 256;
    const std::int64_t numSteps = std::max<std::int64_t>(1, (dist + kSampleStep - 1) / kSampleStep);
    for (std::int64_t step = 0; step <= numSteps; ++step) {
        const std::int64_t curX = start.x.Raw() + (deltaX * step) / numSteps;
        const std::int64_t curY = start.y.Raw() + (deltaY * step) / numSteps;
        const Vec2 center = Vec2::FromRaw(static_cast<std::int32_t>(curX),
                                          static_cast<std::int32_t>(curY));
        if (!IsGroundOpen(center)) {
            return false;
        }
        if (halfExtent > 0) {
            const std::int32_t checkExtent = std::min(halfExtent, kFixedScale / 8);
            if (!IsGroundOpen(Vec2::FromRaw(static_cast<std::int32_t>(curX - checkExtent), static_cast<std::int32_t>(curY))) ||
                !IsGroundOpen(Vec2::FromRaw(static_cast<std::int32_t>(curX + checkExtent), static_cast<std::int32_t>(curY))) ||
                !IsGroundOpen(Vec2::FromRaw(static_cast<std::int32_t>(curX), static_cast<std::int32_t>(curY - checkExtent))) ||
                !IsGroundOpen(Vec2::FromRaw(static_cast<std::int32_t>(curX), static_cast<std::int32_t>(curY + checkExtent)))) {
                return false;
            }
        }
    }
    return true;
}

bool Simulation::HasTraversableLineOfSight(PlayerId mover,
                                           Vec2 start,
                                           Vec2 end,
                                           std::int32_t halfExtent) const {
    const std::int64_t deltaX = static_cast<std::int64_t>(end.x.Raw()) - start.x.Raw();
    const std::int64_t deltaY = static_cast<std::int64_t>(end.y.Raw()) - start.y.Raw();
    const std::int64_t distSq = deltaX * deltaX + deltaY * deltaY;
    if (distSq == 0) {
        return true;
    }
    const std::int64_t dist = IntegerSqrt64(distSq);
    constexpr std::int64_t kSampleStep = 256;
    const std::int64_t numSteps = std::max<std::int64_t>(1, (dist + kSampleStep - 1) / kSampleStep);
    for (std::int64_t step = 0; step <= numSteps; ++step) {
        const std::int64_t curX = start.x.Raw() + (deltaX * step) / numSteps;
        const std::int64_t curY = start.y.Raw() + (deltaY * step) / numSteps;
        const Vec2 center = Vec2::FromRaw(static_cast<std::int32_t>(curX),
                                          static_cast<std::int32_t>(curY));
        if (!IsPositionPassableFor(mover, center)) {
            return false;
        }
        if (halfExtent > 0) {
            const std::int32_t checkExtent = std::min(halfExtent, kFixedScale / 8);
            if (!IsPositionPassableFor(mover, Vec2::FromRaw(static_cast<std::int32_t>(curX - checkExtent), static_cast<std::int32_t>(curY))) ||
                !IsPositionPassableFor(mover, Vec2::FromRaw(static_cast<std::int32_t>(curX + checkExtent), static_cast<std::int32_t>(curY))) ||
                !IsPositionPassableFor(mover, Vec2::FromRaw(static_cast<std::int32_t>(curX), static_cast<std::int32_t>(curY - checkExtent))) ||
                !IsPositionPassableFor(mover, Vec2::FromRaw(static_cast<std::int32_t>(curX), static_cast<std::int32_t>(curY + checkExtent)))) {
                return false;
            }
        }
    }
    return true;
}

Vec2 Simulation::FindStringPulledTarget(Vec2 start, Vec2 destination, std::int32_t halfExtent) const {
    if (HasTraversableLineOfSight(kNeutralPlayer, start, destination,
                                  halfExtent)) {
        return destination;
    }
    const std::int32_t startX = start.x.FloorToInt();
    const std::int32_t startY = start.y.FloorToInt();
    const std::int32_t goalX = destination.x.FloorToInt();
    const std::int32_t goalY = destination.y.FloorToInt();
    const std::size_t width = config_.mapWidthTiles;
    const auto tileIndex = [width](std::int32_t x, std::int32_t y) {
        return static_cast<std::size_t>(y) * width + static_cast<std::size_t>(x);
    };
    const std::size_t goal = tileIndex(goalX, goalY);
    auto cached = pathFieldCache_.find(goal);
    if (cached == pathFieldCache_.end()) {
        const std::optional<Vec2> fallback = FindNextPathWaypoint(start, destination);
        return fallback.value_or(destination);
    }
    constexpr std::size_t kUnvisited = std::numeric_limits<std::size_t>::max();
    const std::size_t startIdx = tileIndex(startX, startY);
    if (startIdx >= cached->second.distanceToGoal.size()) {
        return destination;
    }
    std::size_t currentDist = cached->second.distanceToGoal[startIdx];
    if (currentDist == kUnvisited || currentDist == 0) {
        return destination;
    }

    constexpr std::array<std::array<std::int32_t, 2>, 4> directions{{
        {{0, -1}},
        {{1, 0}},
        {{0, 1}},
        {{-1, 0}},
    }};

    std::vector<Vec2> waypoints;
    waypoints.reserve(8);
    std::int32_t curX = startX;
    std::int32_t curY = startY;

    for (std::size_t step = 0; step < 8 && currentDist > 0; ++step) {
        bool found = false;
        for (const auto& dir : directions) {
            const std::int32_t nextX = curX + dir[0];
            const std::int32_t nextY = curY + dir[1];
            if (nextX < 0 || nextY < 0 ||
                nextX >= config_.mapWidthTiles ||
                nextY >= config_.mapHeightTiles) {
                continue;
            }
            const std::size_t nextIdx = tileIndex(nextX, nextY);
            if (cached->second.distanceToGoal[nextIdx] != kUnvisited &&
                cached->second.distanceToGoal[nextIdx] + 1 == currentDist) {
                curX = nextX;
                curY = nextY;
                currentDist = cached->second.distanceToGoal[nextIdx];
                waypoints.push_back(Vec2::FromRaw(
                    curX * kFixedScale + kFixedScale / 2,
                    curY * kFixedScale + kFixedScale / 2));
                found = true;
                break;
            }
        }
        if (!found) {
            break;
        }
    }

    for (auto it = waypoints.rbegin(); it != waypoints.rend(); ++it) {
        if (HasLineOfSight(start, *it, halfExtent)) {
            return *it;
        }
    }
    if (!waypoints.empty()) {
        return waypoints.front();
    }
    const std::optional<Vec2> fallback = FindNextPathWaypoint(start, destination);
    return fallback.value_or(destination);
}

bool Simulation::MoveTowards(Entity& entity, Vec2 destination) {
    if (entity.movementPerTickRaw <= 0) {
        return false;
    }
    if (entity.position == destination) {
        return true;
    }

    const std::int32_t halfExtent = FootprintHalfExtentRaw(entity.faction, entity.type);

    // SPEC-MOV-006/008: a foreign seat's body and every completed structure
    // block this step. If the unit is already standing in blocked ground — a
    // structure finished on top of it, or a snapshot placed it there — it
    // leaves on terrain alone rather than being imprisoned for good.
    const bool trapped = !IsPositionPassableFor(entity.owner, entity.position);
    const auto stepAllowed = [this, &entity, trapped](Vec2 candidate) {
        if (!trapped) {
            return IsPositionPassableFor(entity.owner, candidate);
        }
        if (!IsInsideMap(candidate)) {
            return false;
        }
        const std::int32_t tileX = candidate.x.FloorToInt();
        const std::int32_t tileY = candidate.y.FloorToInt();
        return TerrainAt(tileX, tileY) != Terrain::Blocked ||
               IsReshapedOpen(tileX, tileY);
    };

    Vec2 movementTarget = destination;
    // A trapped unit walks straight out on terrain alone. Consulting the path
    // field first would be futile: its own cell is masked blocked, so the
    // field never reaches it and it would stand still for ever.
    if (!trapped &&
        !HasTraversableLineOfSight(entity.owner, entity.position, destination,
                                   halfExtent)) {
        const std::optional<Vec2> waypoint = FindNextPathWaypoint(entity.position, destination);
        if (!waypoint.has_value()) {
            return false;
        }
        // SPEC-MOV-006.FAIL: while an obstacle intercepts the direct line the
        // unit follows the grid field to the next waypoint centre. String
        // pulling resumes the moment the line is clear again. Re-deriving a
        // pulled target every tick against an obstacle face made the unit
        // argue with the field and oscillate on a tile boundary instead of
        // rounding the corner.
        const std::int32_t waypointTileX = waypoint->x.FloorToInt();
        const std::int32_t waypointTileY = waypoint->y.FloorToInt();
        movementTarget =
            (waypointTileX == destination.x.FloorToInt() &&
             waypointTileY == destination.y.FloorToInt())
                ? destination
                : Vec2::FromRaw(waypointTileX * kFixedScale + kFixedScale / 2,
                                waypointTileY * kFixedScale + kFixedScale / 2);
    }

    const std::int64_t deltaX =
        static_cast<std::int64_t>(movementTarget.x.Raw()) -
        entity.position.x.Raw();
    const std::int64_t deltaY =
        static_cast<std::int64_t>(movementTarget.y.Raw()) -
        entity.position.y.Raw();
    const std::int64_t distSq = deltaX * deltaX + deltaY * deltaY;
    if (distSq == 0) {
        return entity.position == destination;
    }
    const std::int64_t distance = IntegerSqrt64(distSq);
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
    // Section 7 terrain table: Scarred costs 85% speed.
    if (TerrainAt(entity.position.x.FloorToInt(),
                  entity.position.y.FloorToInt()) == Terrain::Scarred) {
        movementPerTick = std::max(
            1,
            static_cast<std::int32_t>(
                static_cast<std::int64_t>(movementPerTick) *
                kScarredMovementPercent / 100));
    }

    // Arrival damping (SPEC-MOV-012)
    if (distance <= movementPerTick) {
        if (stepAllowed(movementTarget)) {
            entity.position = movementTarget;
            entity.vibrationSignatureUntilTick = std::min(
                kMaximumSupportedTick,
                currentTick_ + config_.rules.vibrationDetection.signatureLingerTicks);
        }
        return entity.position == destination;
    }

    const std::int64_t travel = movementPerTick;
    const std::int64_t stepX = travel * deltaX / distance;
    const std::int64_t stepY = travel * deltaY / distance;

    const Vec2 candidate = Vec2::FromRaw(
        static_cast<std::int32_t>(entity.position.x.Raw() + stepX),
        static_cast<std::int32_t>(entity.position.y.Raw() + stepY));
    if (!stepAllowed(candidate)) {
        // SPEC-MOV-006.FAIL: the direct step clipped an obstacle corner the
        // sampled line-of-sight test did not see. Route first and slide only
        // as a last resort. Sliding first parks the unit on a tile boundary
        // against the obstacle face, where the waypoint it derives from its
        // own tile flips every tick and it oscillates instead of going round.
        const std::optional<Vec2> gridWaypoint =
            FindNextPathWaypoint(entity.position, destination);
        if (gridWaypoint.has_value()) {
            const std::int32_t waypointTileX = gridWaypoint->x.FloorToInt();
            const std::int32_t waypointTileY = gridWaypoint->y.FloorToInt();
            const Vec2 gridTarget =
                (waypointTileX == destination.x.FloorToInt() &&
                 waypointTileY == destination.y.FloorToInt())
                    ? destination
                    : Vec2::FromRaw(
                          waypointTileX * kFixedScale + kFixedScale / 2,
                          waypointTileY * kFixedScale + kFixedScale / 2);
            const std::int64_t gridDeltaX =
                static_cast<std::int64_t>(gridTarget.x.Raw()) -
                entity.position.x.Raw();
            const std::int64_t gridDeltaY =
                static_cast<std::int64_t>(gridTarget.y.Raw()) -
                entity.position.y.Raw();
            const std::int64_t gridDistance =
                IntegerSqrt64(gridDeltaX * gridDeltaX + gridDeltaY * gridDeltaY);
            if (gridDistance > 0) {
                const std::int64_t gridTravel =
                    std::min<std::int64_t>(travel, gridDistance);
                const Vec2 gridCandidate = Vec2::FromRaw(
                    static_cast<std::int32_t>(
                        entity.position.x.Raw() + gridTravel * gridDeltaX / gridDistance),
                    static_cast<std::int32_t>(
                        entity.position.y.Raw() + gridTravel * gridDeltaY / gridDistance));
                if (gridCandidate != entity.position &&
                    stepAllowed(gridCandidate)) {
                    entity.position = gridCandidate;
                    entity.vibrationSignatureUntilTick = std::min(
                        kMaximumSupportedTick,
                        currentTick_ + config_.rules.vibrationDetection.signatureLingerTicks);
                    return false;
                }
            }
        }
        // The route produced nothing this tick: slide along the obstacle face
        // so a unit pressed into a corner still makes progress.
        const Vec2 candidateX =
            Vec2::FromRaw(candidate.x.Raw(), entity.position.y.Raw());
        const Vec2 candidateY =
            Vec2::FromRaw(entity.position.x.Raw(), candidate.y.Raw());
        if (stepX != 0 && stepAllowed(candidateX)) {
            entity.position = candidateX;
            return false;
        }
        if (stepY != 0 && stepAllowed(candidateY)) {
            entity.position = candidateY;
            return false;
        }
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

bool Simulation::IsSeparationCandidate(const Entity& entity) const {
    if (entity.hitPoints <= 0 || !entity.completed ||
        entity.movementPerTickRaw <= 0) {
        return false;
    }
    if (IsBuilding(entity.type)) {
        // A Kharuun Waystone takes part only while it travels. Rooted and
        // transitioning Waystones are stationary supply structures that must
        // hold their site; SPEC-MOV-008 governs mobile units.
        return entity.faction == Faction::KharuunAssemblies &&
               entity.type == EntityType::Dropoff &&
               entity.waystoneMode == WaystoneMode::Mobile;
    }
    return entity.type == EntityType::Worker ||
           entity.type == EntityType::Soldier ||
           entity.type == EntityType::HeavyUnit ||
           entity.type == EntityType::ScoutUnit;
}

bool Simulation::ShouldPackAtDestination(const Entity& entity) const {
    // Packing is a group-of-units behaviour; a travelling structure such as a
    // mobile Waystone must reach its exact ordered site so it can root there.
    if (entity.order.type != OrderType::Move ||
        entity.position == entity.order.destination ||
        IsBuilding(entity.type) ||
        !IsSeparationCandidate(entity)) {
        return false;
    }
    const std::int64_t halfExtent =
        FootprintHalfExtentRaw(entity.faction, entity.type);
    const std::int64_t reach = entity.movementPerTickRaw;
    for (const Entity& other : entities_) {
        if (other.id == entity.id || other.owner != entity.owner ||
            other.order.type != OrderType::None ||
            other.order.destination != entity.order.destination ||
            !IsSeparationCandidate(other)) {
            continue;
        }
        // The resting neighbour shares this destination: stop before the
        // next step would overlap it (SPEC-MOV-011 arrival area,
        // SPEC-MOV-012 clean halt without neighbour pushing).
        const std::int64_t contact =
            halfExtent +
            FootprintHalfExtentRaw(other.faction, other.type) + reach;
        const std::int64_t deltaX =
            static_cast<std::int64_t>(other.position.x.Raw()) -
            entity.position.x.Raw();
        const std::int64_t deltaY =
            static_cast<std::int64_t>(other.position.y.Raw()) -
            entity.position.y.Raw();
        if (Abs64(deltaX) >= contact || Abs64(deltaY) >= contact) {
            continue;
        }
        if (deltaX * deltaX + deltaY * deltaY < contact * contact) {
            return true;
        }
    }
    return false;
}

void Simulation::ApplySoftSeparation(
    const std::vector<Vec2>& positionsBeforeOrders) {
    struct MobileCandidate {
        EntityId id;
        std::size_t index;
        std::int32_t x;
        std::int32_t y;
        std::int32_t halfExtent;
        std::int32_t movementPerTick;
        PlayerId owner;
        bool moving;
    };

    std::vector<MobileCandidate> mobile;
    mobile.reserve(entities_.size());
    for (std::size_t i = 0; i < entities_.size(); ++i) {
        const Entity& e = entities_[i];
        if (!IsSeparationCandidate(e)) {
            continue;
        }
        const bool moving =
            i < positionsBeforeOrders.size() &&
            positionsBeforeOrders[i] != e.position;
        mobile.push_back({
            e.id,
            i,
            e.position.x.Raw(),
            e.position.y.Raw(),
            FootprintHalfExtentRaw(e.faction, e.type),
            e.movementPerTickRaw,
            e.owner,
            moving
        });
    }

    if (mobile.size() < 2) {
        return;
    }

    std::sort(mobile.begin(), mobile.end(), [](const auto& a, const auto& b) {
        return a.id < b.id;
    });

    for (std::size_t i = 0; i < mobile.size(); ++i) {
        for (std::size_t j = i + 1; j < mobile.size(); ++j) {
            if (mobile[i].owner != mobile[j].owner) {
                continue;
            }
            const Entity& first = entities_[mobile[i].index];
            const Entity& second = entities_[mobile[j].index];
            if (first.type == EntityType::Worker && second.type == EntityType::Worker &&
                first.assignedResourceNode != 0 &&
                first.assignedResourceNode == second.assignedResourceNode) {
                continue;
            }
            const std::int32_t minClearance = mobile[i].halfExtent + mobile[j].halfExtent;
            const std::int64_t deltaX = static_cast<std::int64_t>(mobile[j].x) - mobile[i].x;
            const std::int64_t deltaY = static_cast<std::int64_t>(mobile[j].y) - mobile[i].y;
            if (Abs64(deltaX) >= minClearance || Abs64(deltaY) >= minClearance) {
                continue;
            }
            const std::int64_t distSq = deltaX * deltaX + deltaY * deltaY;
            const std::int64_t minClearanceSq = static_cast<std::int64_t>(minClearance) * minClearance;
            if (distSq >= minClearanceSq) {
                continue;
            }

            const std::int64_t dist = IntegerSqrt64(distSq);
            const std::int64_t overlap = minClearance - dist;
            if (overlap <= 0) {
                continue;
            }

            const std::int64_t maxNudge = std::min<std::int64_t>({
                static_cast<std::int64_t>(mobile[i].movementPerTick / 2),
                static_cast<std::int64_t>(mobile[j].movementPerTick / 2),
                static_cast<std::int64_t>(32)
            });
            const std::int64_t nudge = std::max<std::int64_t>(1, std::min<std::int64_t>(overlap / 2, maxNudge));

            std::int64_t pushX = 0;
            std::int64_t pushY = 0;
            if (dist > 0) {
                pushX = (nudge * deltaX) / dist;
                pushY = (nudge * deltaY) / dist;
                if (pushX == 0 && deltaX != 0) {
                    pushX = deltaX > 0 ? 1 : -1;
                }
                if (pushY == 0 && deltaY != 0) {
                    pushY = deltaY > 0 ? 1 : -1;
                }
            } else {
                pushX = ((mobile[i].id + mobile[j].id) % 2 == 0) ? nudge : -nudge;
                pushY = ((mobile[i].id + mobile[j].id) % 4 < 2) ? nudge : -nudge;
            }

            Entity& entityA = entities_[mobile[i].index];
            Entity& entityB = entities_[mobile[j].index];

            // SPEC-MOV-008/012 yield policy. A unit that moved this tick
            // deflects around a resting neighbour instead of shoving it,
            // unless the mover's ordered destination is the spot the resting
            // unit occupies: a parked blocker is transient and yields so the
            // order can complete. Two resting units that still overlap share
            // the correction, except that a unit standing exactly on its own
            // ordered destination holds its ground.
            const auto wantsSpotOf = [minClearance](const Entity& mover,
                                                    const Entity& resting) {
                const std::int64_t wantX =
                    static_cast<std::int64_t>(mover.order.destination.x.Raw()) -
                    resting.position.x.Raw();
                const std::int64_t wantY =
                    static_cast<std::int64_t>(mover.order.destination.y.Raw()) -
                    resting.position.y.Raw();
                return Abs64(wantX) < minClearance && Abs64(wantY) < minClearance &&
                       wantX * wantX + wantY * wantY <
                           static_cast<std::int64_t>(minClearance) * minClearance;
            };
            const auto holdsGround = [](const Entity& resting) {
                return resting.position == resting.order.destination &&
                       resting.position != Vec2{};
            };
            bool pushA = true;
            bool pushB = true;
            if (mobile[i].halfExtent != mobile[j].halfExtent) {
                // Footprint mass: the smaller unit yields to the larger one
                // whether or not either is moving, so a travelling Waystone
                // reaches its exact rooting site through resting workers
                // instead of being deflected off it.
                pushA = mobile[i].halfExtent < mobile[j].halfExtent;
                pushB = !pushA;
            } else if (mobile[i].moving && !mobile[j].moving) {
                pushB = wantsSpotOf(entityA, entityB);
                pushA = !pushB;
            } else if (mobile[j].moving && !mobile[i].moving) {
                pushA = wantsSpotOf(entityB, entityA);
                pushB = !pushA;
            } else if (!mobile[i].moving && !mobile[j].moving) {
                const bool aHolds = holdsGround(entityA);
                const bool bHolds = holdsGround(entityB);
                if (aHolds != bHolds) {
                    pushA = !aHolds;
                    pushB = !bHolds;
                }
            }

            if (pushA) {
                const Vec2 candA = Vec2::FromRaw(
                    static_cast<std::int32_t>(entityA.position.x.Raw() - pushX),
                    static_cast<std::int32_t>(entityA.position.y.Raw() - pushY));
                if (IsPositionPassable(candA)) {
                    entityA.position = candA;
                    mobile[i].x = candA.x.Raw();
                    mobile[i].y = candA.y.Raw();
                }
            }
            if (pushB) {
                const Vec2 candB = Vec2::FromRaw(
                    static_cast<std::int32_t>(entityB.position.x.Raw() + pushX),
                    static_cast<std::int32_t>(entityB.position.y.Raw() + pushY));
                if (IsPositionPassable(candB)) {
                    entityB.position = candB;
                    mobile[j].x = candB.x.Raw();
                    mobile[j].y = candB.y.Raw();
                }
            }
        }
    }
}

EntityId Simulation::FindSmartCastCaster(
    PlayerId player,
    CommandType commandType,
    Vec2 targetPosition,
    EntityId targetEntity,
    const std::vector<EntityId>& candidates) const {
    EntityId bestId = 0;
    std::uint64_t bestDist = std::numeric_limits<std::uint64_t>::max();

    Vec2 focalPoint = targetPosition;
    if (focalPoint == Vec2{} && targetEntity != 0) {
        const Entity* tgt = FindEntity(targetEntity);
        if (tgt != nullptr) {
            focalPoint = tgt->position;
        }
    }

    const auto checkCandidate = [&](const Entity& entity) {
        if (entity.owner != player || !entity.completed || entity.hitPoints <= 0) {
            return;
        }
        switch (commandType) {
            case CommandType::RaiseMineralCover: {
                if (entity.faction != Faction::KharuunAssemblies ||
                    entity.type != EntityType::HeavyUnit ||
                    entity.mineralCoverCooldownUntilTick > currentTick_) {
                    return;
                }
                const PlayerState* pState = FindPlayer(player);
                if (pState == nullptr ||
                    pState->resources.dawnshards < config_.rules.mineralCover.dawnCost) {
                    return;
                }
                break;
            }
            case CommandType::ToggleDeploy: {
                if (entity.faction != Faction::MeridianCompact ||
                    entity.type != EntityType::HeavyUnit ||
                    entity.deploymentPhase != BulwarkDeploymentPhase::None) {
                    return;
                }
                break;
            }
            case CommandType::ActivateRelaySupply: {
                // Choose only a caster the authoritative command can admit,
                // including connection and cooldown, rather than a lookalike
                // infrastructure type or an unreachable nearer Skiff.
                if (ValidateRelaySupply(player, entity.id) != RelaySupplyResult::Valid) {
                    return;
                }
                break;
            }
            default:
                break;
        }
        const std::uint64_t dist = DistanceSquaredRaw(entity.position, focalPoint);
        if (dist < bestDist || (dist == bestDist && (bestId == 0 || entity.id < bestId))) {
            bestDist = dist;
            bestId = entity.id;
        }
    };

    if (!candidates.empty()) {
        for (EntityId id : candidates) {
            const Entity* ent = FindEntity(id);
            if (ent != nullptr) {
                checkCandidate(*ent);
            }
        }
    } else {
        for (const Entity& ent : entities_) {
            checkCandidate(ent);
        }
    }
    return bestId;
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
        if (!legacyBulwarkReplaySemantics_) {
            const auto lateral = static_cast<std::uint64_t>(Abs64(
                bulwark.deploymentFacing.x.Raw() != 0 ? attackerDeltaY : attackerDeltaX));
            const auto forward = static_cast<std::uint64_t>(attackerForward);
            // Half-angle 60 degrees: lateral^2 <= 3 * forward^2.
            // Divide instead of tripling to avoid overflow even at int32 extrema.
            const auto lateralSquared = lateral * lateral;
            const auto minimumForwardSquared = lateralSquared / 3U +
                (lateralSquared % 3U != 0 ? 1U : 0U);
            if (forward * forward < minimumForwardSquared) continue;
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
    std::vector<const Entity*> candidates;
    for (const Entity& entity : entities_) {
        if (entity.owner == player && entity.hitPoints > 0 && entity.completed &&
            IsOperationalDropoff(entity)) {
            candidates.push_back(&entity);
        }
    }
    if (candidates.empty() || !IsInsideMap(from)) {
        return 0;
    }
    const std::size_t width = static_cast<std::size_t>(config_.mapWidthTiles);
    const auto indexOf = [width](Vec2 position) {
        return static_cast<std::size_t>(position.y.FloorToInt()) * width +
               static_cast<std::size_t>(position.x.FloorToInt());
    };
    constexpr std::size_t unreachable = std::numeric_limits<std::size_t>::max();
    std::vector<std::size_t> distance(terrain_.size(), unreachable);
    std::vector<std::size_t> frontier{indexOf(from)};
    distance[frontier.front()] = 0;
    constexpr std::array<std::array<std::int32_t, 2>, 4> directions{{
        {{0, -1}}, {{1, 0}}, {{0, 1}}, {{-1, 0}},
    }};
    for (std::size_t head = 0; head < frontier.size(); ++head) {
        const auto current = frontier[head];
        const auto x = static_cast<std::int32_t>(current % width);
        const auto y = static_cast<std::int32_t>(current / width);
        for (const auto& direction : directions) {
            const auto nx = x + direction[0], ny = y + direction[1];
            if (nx < 0 || ny < 0 || nx >= config_.mapWidthTiles ||
                ny >= config_.mapHeightTiles) {
                continue;
            }
            const auto next = static_cast<std::size_t>(ny) * width + static_cast<std::size_t>(nx);
            if (distance[next] != unreachable) {
                continue;
            }
            if (IsTileKnownPassableTo(player, nx, ny)) {
                distance[next] = distance[current] + 1;
                frontier.push_back(next);
                continue;
            }
            // A depot stands on solid ground of its own. It is still a
            // destination, so it takes a cost from the open tile beside it,
            // but the flood never continues through the building.
            if (IsStructureOccupiedTile(nx, ny)) {
                distance[next] = distance[current] + 1;
            }
        }
    }
    const Entity* nearest = nullptr;
    std::size_t bestCost = unreachable;
    std::uint64_t bestStraightDistance = std::numeric_limits<std::uint64_t>::max();
    for (const Entity* depot : candidates) {
        // A depot occupies several tiles and its centre one is solid, so the
        // flood above stops at the rim. Cost it by the cheapest tile of its
        // own footprint rather than by a centre no hauler can stand on.
        const std::int32_t depotExtent =
            FootprintHalfExtentRaw(depot->faction, depot->type);
        const std::int32_t depotMinX = std::max(
            0, (depot->position.x.Raw() - depotExtent) / kFixedScale);
        const std::int32_t depotMaxX = std::min(
            config_.mapWidthTiles - 1,
            (depot->position.x.Raw() + depotExtent - 1) / kFixedScale);
        const std::int32_t depotMinY = std::max(
            0, (depot->position.y.Raw() - depotExtent) / kFixedScale);
        const std::int32_t depotMaxY = std::min(
            config_.mapHeightTiles - 1,
            (depot->position.y.Raw() + depotExtent - 1) / kFixedScale);
        std::size_t cost = distance[indexOf(depot->position)];
        for (std::int32_t tileY = depotMinY; tileY <= depotMaxY; ++tileY) {
            for (std::int32_t tileX = depotMinX; tileX <= depotMaxX; ++tileX) {
                const std::size_t candidateCost =
                    distance[static_cast<std::size_t>(tileY) * width +
                             static_cast<std::size_t>(tileX)];
                if (candidateCost < cost) {
                    cost = candidateCost;
                }
            }
        }
        const auto straight = DistanceSquaredRaw(from, depot->position);
        if (cost != unreachable &&
            (nearest == nullptr || std::tie(cost, straight, depot->id) <
             std::tie(bestCost, bestStraightDistance, nearest->id))) {
            nearest = depot;
            bestCost = cost;
            bestStraightDistance = straight;
        }
    }
    return nearest != nullptr ? nearest->id : 0;
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
        if (!config_.IsHostile(player, entity.owner) ||
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
        if (!config_.IsHostile(attacker.owner, entity.owner) ||
            entity.hitPoints <= 0 || IsProtectedCommandCore(entity) ||
            !IsEntityVisibleTo(attacker.owner, entity.id) ||
            !InInteractionRange(attacker, entity, attacker.attackRangeRaw) ||
            !HasLineOfFire(attacker, entity)) {
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
        if (!config_.IsHostile(attacker.owner, entity.owner) ||
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
    const auto IsAvailable = [this, &producer](Vec2 candidate) {
        if (!legacyProductionReplaySemantics_) {
            return IsSpawnPositionAvailable(
                producer.faction, producer.productionType, candidate);
        }
        // Replay versions through 25 admitted completed units against terrain
        // and building footprints only. Retain that exact search contract for
        // authenticated legacy playback; current production continues to use
        // full unit-footprint admission.
        if (!IsPositionPassable(candidate)) {
            return false;
        }
        for (const Entity& entity : entities_) {
            if (entity.hitPoints <= 0 ||
                (replayChecksumSnapshotVersion_ >=
                     kFutureWellLifecycleSnapshotVersion &&
                 IsCollapsedFutureWell(entity)) ||
                !IsBuilding(entity.type)) {
                continue;
            }
            const std::int32_t combinedExtent =
                FootprintHalfExtentRaw(entity.faction, entity.type) +
                kFixedScale / 8;
            if (Abs64(static_cast<std::int64_t>(candidate.x.Raw()) -
                      entity.position.x.Raw()) < combinedExtent &&
                Abs64(static_cast<std::int64_t>(candidate.y.Raw()) -
                      entity.position.y.Raw()) < combinedExtent) {
                return false;
            }
        }
        return true;
    };
    if (legacyProductionReplaySemantics_ &&
        replayChecksumSnapshotVersion_ < kMemorySnapshotVersion) {
        // Schema 24 searched each ring from its negative corner. Schema 25
        // changed the ordering to search away from the map centre.
        for (std::int32_t radius = 2; radius <= 8; ++radius) {
            for (std::int32_t offsetY = -radius; offsetY <= radius;
                 ++offsetY) {
                for (std::int32_t offsetX = -radius; offsetX <= radius;
                     ++offsetX) {
                    if (Abs64(offsetX) != radius &&
                        Abs64(offsetY) != radius) {
                        continue;
                    }
                    const Vec2 candidate = Vec2::FromTiles(
                        centerX + offsetX, centerY + offsetY);
                    if (IsAvailable(candidate)) {
                        return candidate;
                    }
                }
            }
        }
        return std::nullopt;
    }
    const std::int32_t mapCenterX = config_.mapWidthTiles / 2;
    const std::int32_t mapCenterY = config_.mapHeightTiles / 2;
    const std::int32_t signX = centerX < mapCenterX ? -1 : 1;
    const std::int32_t signY = centerY < mapCenterY ? -1 : 1;
    for (std::int32_t radius = 2; radius <= 8; ++radius) {
        for (std::int32_t stepY = 0; stepY <= 2 * radius; ++stepY) {
            const std::int32_t offsetY = signY * (radius - stepY);
            for (std::int32_t stepX = 0; stepX <= 2 * radius; ++stepX) {
                const std::int32_t offsetX = signX * (radius - stepX);
                if (Abs64(offsetX) != radius && Abs64(offsetY) != radius) {
                    continue;
                }
                const Vec2 candidate =
                    Vec2::FromTiles(centerX + offsetX, centerY + offsetY);
                if (IsAvailable(candidate)) {
                    return candidate;
                }
            }
        }
    }
    return std::nullopt;
}

bool Simulation::TryActivateNextProduction(Entity& producer) {
    if (producer.productionRequired > 0 || producer.productionQueue.empty() ||
        producer.hitPoints <= 0 || !producer.completed ||
        !IsProducerPowered(producer)) {
        return false;
    }
    PlayerState* player = MutablePlayer(producer.owner);
    if (player == nullptr ||
        (player->activeResearch != ResearchType::None &&
         player->researchProducer == producer.id)) {
        return false;
    }
    const ProductionQueueItem& item = producer.productionQueue.front();
    if (!ResourceCovers(player->resources, item.configuredCost) ||
        entities_.size() >= kMaximumSerializedEntities) {
        return false;
    }
    std::int32_t committedPopulation = PopulationUsed(producer.owner);
    for (const Entity& entity : entities_) {
        if (entity.owner == producer.owner && entity.productionRequired > 0) {
            committedPopulation = SaturatingAdd(
                committedPopulation, entity.productionLogisticsCost);
        }
    }
    if (SaturatingAdd(committedPopulation, item.logisticsCost) >
        PopulationCapacity(producer.owner)) {
        return false;
    }
    // SPEC-RES-008: a waiting mobile unit stays waiting while fielded plus
    // reserved entities already reach the limit; a death, cancellation or
    // lost producer frees the slot and the next tick starts it.
    if (IsMobileEntityType(item.unitType) &&
        MobileEntityCount(producer.owner) +
                MobileEntityReservations(producer.owner) >=
            kMobileEntityLimit) {
        return false;
    }
    player->resources.material -= item.configuredCost.material;
    player->resources.dawnshards -= item.configuredCost.dawnshards;
    producer.activeProductionItemId = item.itemId;
    producer.productionType = item.unitType;
    producer.productionProgress = 0;
    producer.productionRequired = item.requiredTicks;
    producer.productionInvestedCost = item.configuredCost;
    producer.productionLogisticsCost = item.logisticsCost;
    producer.productionSpawnBlockedTicks = 0;
    producer.productionPausedForSpawn = false;
    producer.productionSpawnBlockedAlert = false;
    if (producer.owner < productionTransitionReceipts_.size()) {
        productionTransitionReceipts_[producer.owner].push_back({
            currentTick_, producer.id, item.itemId, 0, item.unitType,
            ProductionTransition::Activated,
            ProductionStartBlockReason::None, item.configuredCost, {},
            item.logisticsCost, 0});
    }
    producer.productionQueue.erase(producer.productionQueue.begin());
    return true;
}

void Simulation::ClearActiveProduction(Entity& producer) {
    producer.productionType = EntityType::Worker;
    producer.activeProductionItemId = 0;
    producer.productionProgress = 0;
    producer.productionRequired = 0;
    producer.productionInvestedCost = {};
    producer.productionLogisticsCost = 0;
    producer.productionSpawnBlockedTicks = 0;
    producer.productionPausedForSpawn = false;
    producer.productionSpawnBlockedAlert = false;
}

void Simulation::ApplyRallyRoute(Entity& unit, Entity& producer) {
    unit.order = {};
    unit.orderQueue.clear();
    if (producer.rallyRoute.empty()) {
        producer.rallyRouteAlert = false;
        return;
    }
    std::vector<Order> admitted{};
    admitted.reserve(producer.rallyRoute.size());
    for (Order order : producer.rallyRoute) {
        const Vec2 routeStart = admitted.empty()
                                    ? unit.position
                                    : admitted.back().destination;
        order.anchor = routeStart;
        if (order.type == OrderType::Move) {
            if (!IsPositionPassable(order.destination)) {
                producer.rallyRouteAlert = true;
                return;
            }
        } else if (order.type == OrderType::Guard) {
            const Entity* target = FindEntity(order.target);
            if (unit.attackDamage <= 0 || target == nullptr ||
                target->hitPoints <= 0 || target->id == unit.id ||
                target->owner == kNeutralPlayer ||
                config_.IsHostile(unit.owner, target->owner)) {
                producer.rallyRouteAlert = true;
                return;
            }
            order.destination = target->position;
        } else if (order.type == OrderType::Gather) {
            const Entity* target = FindEntity(order.target);
            if (unit.type != EntityType::Worker || target == nullptr ||
                target->type != EntityType::ResourceNode ||
                target->resourceRemaining <= 0) {
                producer.rallyRouteAlert = true;
                return;
            }
            order.destination = target->position;
        } else {
            producer.rallyRouteAlert = true;
            return;
        }
        if (order.destination != routeStart &&
            !FindNextPathWaypoint(routeStart, order.destination).has_value()) {
            producer.rallyRouteAlert = true;
            return;
        }
        admitted.push_back(order);
    }
    unit.order = admitted.front();
    unit.orderQueue.assign(admitted.begin() + 1, admitted.end());
    if (unit.order.type == OrderType::Gather) {
        BeginGather(unit, unit.order.target);
    }
    producer.rallyRouteAlert = false;
}

void Simulation::ProcessCommandsForCurrentTick(
    std::map<std::pair<PlayerId, std::uint64_t>,
             CommandResolutionOutcome>* resolutionSink) {
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
        RecordCommandResolutionReceipt(command, outcome, resolutionSink);
        hasExecutedSequence_[command.player] = true;
        lastExecutedSequence_[command.player] = command.sequence;
    }
}

void Simulation::RecordCommandResolutionReceipt(
    const Command& command,
    CommandResolutionOutcome outcome,
    std::map<std::pair<PlayerId, std::uint64_t>,
             CommandResolutionOutcome>* resolutionSink) {
    // QueueCommand and snapshot validation enforce unique, monotonically
    // executed player/sequence keys. Preserve that invariant here without an
    // O(receipt-count) duplicate scan on the authoritative due-command path.
    StoredCommandResolutionReceipt stored{};
    stored.sequence = command.sequence;
    stored.receipt.player = command.player;
    stored.receipt.commandType = command.type;
    stored.receipt.assignedExecutionTick = command.executeTick;
    stored.receipt.outcome = outcome;
    if (resolutionSink != nullptr) {
        resolutionSink->emplace(
            std::make_pair(command.player, command.sequence), outcome);
    }
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
    if (actor == nullptr || actor->owner != command.player ||
        (!actor->completed &&
         command.type != CommandType::CancelConstruction) ||
        actor->hitPoints <= 0) {
        return CommandResolutionOutcome::NoEffect;
    }
    if (actor->pendingWarformAdaptation != WarformAdaptation::None &&
        command.type != CommandType::AdaptWarform) {
        return CommandResolutionOutcome::NoEffect;
    }
    const Order previousOrder = actor->order;
    const EntityId previousNode = actor->assignedResourceNode;
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
            actor->orderQueue.clear();
            outcome = CommandResolutionOutcome::Applied;
            return;
        }
        case CommandType::Move: {
            // MOV-002 / SIM-003: an order the ordering player's own map proves
            // impossible is refused with a stable reason code instead of being
            // receipted as Applied and then failing silently in MoveTowards
            // for the rest of the match.
            const CommandResolutionOutcome admission =
                ValidateMoveOrder(command.player, actor->id, command.position);
            if (admission != CommandResolutionOutcome::Applied) {
                outcome = admission;
                return;
            }
            if (command.queue && actor->order.type != OrderType::None) {
                if (actor->orderQueue.size() < Entity::kMaxQueuedOrders) {
                    Order queued{};
                    queued.type = OrderType::Move;
                    queued.anchor = actor->orderQueue.empty()
                                        ? actor->order.destination
                                        : actor->orderQueue.back().destination;
                    queued.destination = command.position;
                    actor->orderQueue.push_back(queued);
                }
                outcome = CommandResolutionOutcome::Applied;
                return;
            }
            actor->order.type = OrderType::Move;
            actor->order.target = 0;
            actor->order.anchor = actor->position;
            actor->order.destination = command.position;
            outcome = CommandResolutionOutcome::Applied;
            return;
        }
        case CommandType::Gather: {
            const Entity* target = FindEntity(command.target);
            if (actor->type == EntityType::Worker && target != nullptr &&
                target->type == EntityType::ResourceNode &&
                target->resourceRemaining > 0 &&
                IsEntityVisibleTo(command.player, target->id)) {
                if (command.queue && actor->order.type != OrderType::None) {
                    if (actor->orderQueue.size() < Entity::kMaxQueuedOrders) {
                        Order queued{};
                        queued.type = OrderType::Gather;
                        queued.target = target->id;
                        queued.anchor = actor->position;
                        queued.destination = target->position;
                        actor->orderQueue.push_back(queued);
                    }
                    outcome = CommandResolutionOutcome::Applied;
                    return;
                }
                actor->order.type = OrderType::Gather;
                actor->order.target = target->id;
                actor->order.anchor = actor->position;
                actor->order.destination = target->position;
                actor->assignedResourceNode = target->id;
                actor->harvestTicks = 0;
                outcome = CommandResolutionOutcome::Applied;
            }
            return;
        }
        case CommandType::Deliver: {
            const Entity* target = FindEntity(command.target);
            if (actor->type == EntityType::Worker && target != nullptr &&
                target->owner == command.player && target->completed &&
                IsOperationalDropoff(*target)) {
                if (command.queue && actor->order.type != OrderType::None) {
                    if (actor->orderQueue.size() < Entity::kMaxQueuedOrders) {
                        Order queued{};
                        queued.type = OrderType::Deliver;
                        queued.target = target->id;
                        queued.anchor = actor->position;
                        queued.destination = target->position;
                        actor->orderQueue.push_back(queued);
                    }
                    outcome = CommandResolutionOutcome::Applied;
                    return;
                }
                actor->order.type = OrderType::Deliver;
                actor->order.target = target->id;
                actor->order.anchor = actor->position;
                actor->order.destination = target->position;
                outcome = CommandResolutionOutcome::Applied;
            }
            return;
        }
        case CommandType::Build: {
            if (actor->type != EntityType::Worker ||
                actor->order.type == OrderType::Build) {
                return;
            }
            // A targeted Build is assist-only. Invalid or cancelled sites must
            // never fall through to placement, payment, or site resurrection.
            // Replay <=28 retains its historical assist behavior below.
            if (command.target != 0 && !legacyConstructionAssistReplaySemantics_) {
                if (ValidateConstructionAssist(command.player, command.actor,
                        command.target) != ConstructionAssistResult::Valid) {
                    return;
                }
                const Entity* siteTarget = FindEntity(command.target);
                actor->order.type = OrderType::Build;
                actor->order.target = siteTarget->id;
                actor->order.anchor = actor->position;
                actor->order.destination = siteTarget->position;
                actor->order.buildType = siteTarget->type;
                outcome = CommandResolutionOutcome::Applied;
                return;
            }
            // Multi-builder assist (REL-BLD-004): historical replay path.
            if (command.target != 0) {
                const Entity* siteTarget = FindEntity(command.target);
                if (siteTarget != nullptr && siteTarget->owner == command.player &&
                    !siteTarget->completed && IsBuilding(siteTarget->type)) {
                    actor->order.type = OrderType::Build;
                    actor->order.target = siteTarget->id;
                    actor->order.anchor = actor->position;
                    actor->order.destination = siteTarget->position;
                    actor->order.buildType = siteTarget->type;
                    outcome = CommandResolutionOutcome::Applied;
                    return;
                }
            }
            if (ValidatePlacement(command.player, command.buildType,
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
            MarkStructureOccupancyDirty();
            site.hitPoints = std::max(1, site.maxHitPoints / 10);
            site.constructionProgress = 0;
            site.constructionInvestedCost = cost;
            actor->order.type = OrderType::Build;
            actor->order.target = site.id;
            actor->order.anchor = actor->position;
            actor->order.destination = site.position;
            actor->order.buildType = command.buildType;
            // Set the order before push_back; vector growth may relocate the actor.
            const EntityId workerId = actor->id;
            entities_.push_back(site);
            MarkStructureOccupancyDirty();
            constructionReceipts_[command.player].push_back({
                currentTick_, workerId, site.id,
                ConstructionTransition::Created, 0, cost, {},
                command.sequence});
            outcome = CommandResolutionOutcome::Applied;
            return;
        }
        case CommandType::Repair: {
            const Entity* target = FindEntity(command.target);
            if (ValidateRepair(command.player, command.actor, command.target) !=
                RepairResult::Valid) {
                return;
            }
            if (command.queue && actor->order.type != OrderType::None) {
                if (actor->orderQueue.size() < Entity::kMaxQueuedOrders) {
                    Order queued{};
                    queued.type = OrderType::Repair;
                    queued.target = target->id;
                    queued.anchor = actor->position;
                    queued.destination = target->position;
                    actor->orderQueue.push_back(queued);
                }
                outcome = CommandResolutionOutcome::Applied;
                return;
            }
            actor->order.type = OrderType::Repair;
            actor->order.target = target->id;
            actor->order.anchor = actor->position;
            actor->order.destination = target->position;
            outcome = CommandResolutionOutcome::Applied;
            return;
        }
        case CommandType::CancelConstruction: {
            if (actor->completed || !IsBuilding(actor->type)) {
                return;
            }
            PlayerState* player = MutablePlayer(command.player);
            if (player == nullptr) {
                return;
            }
            const std::int32_t refundPercent =
                static_cast<std::int64_t>(actor->constructionProgress) * 2 <
                        std::max(1, actor->constructionRequired)
                    ? 75
                    : 50;
            const auto Refund = [refundPercent](std::int32_t invested) {
                return static_cast<std::int32_t>(
                    static_cast<std::int64_t>(invested) * refundPercent / 100);
            };
            const ResourcePool refund{
                Refund(actor->constructionInvestedCost.material),
                Refund(actor->constructionInvestedCost.dawnshards)};
            player->resources.material = SaturatingAdd(
                player->resources.material, refund.material);
            player->resources.dawnshards = SaturatingAdd(
                player->resources.dawnshards, refund.dawnshards);
            const EntityId cancelledId = actor->id;
            constructionReceipts_[command.player].push_back({
                currentTick_, 0, cancelledId,
                ConstructionTransition::Cancelled, 0, {}, refund,
                command.sequence});
            for (Entity& entity : entities_) {
                if ((entity.order.type == OrderType::Build ||
                     entity.order.type == OrderType::Repair) &&
                    entity.order.target == cancelledId) {
                    entity.order = {};
                }
                std::erase_if(entity.orderQueue, [cancelledId](const Order& order) {
                    return (order.type == OrderType::Build ||
                            order.type == OrderType::Repair) &&
                           order.target == cancelledId;
                });
            }
            actor = MutableEntity(cancelledId);
            if (actor != nullptr) {
                actor->hitPoints = 0;
            }
            outcome = CommandResolutionOutcome::Applied;
            return;
        }
        case CommandType::Attack: {
            const Entity* target = FindEntity(command.target);
            if (actor->attackDamage > 0 && target != nullptr &&
                config_.IsHostile(command.player, target->owner) &&
                !IsProtectedCommandCore(*target) &&
                IsEntityVisibleTo(command.player, target->id)) {
                if (command.queue && actor->order.type != OrderType::None) {
                    if (actor->orderQueue.size() < Entity::kMaxQueuedOrders) {
                        Order queued{};
                        queued.type = OrderType::Attack;
                        queued.target = target->id;
                        queued.anchor = actor->position;
                        queued.destination = target->position;
                        actor->orderQueue.push_back(queued);
                    }
                    outcome = CommandResolutionOutcome::Applied;
                    return;
                }
                actor->order.type = OrderType::Attack;
                actor->order.target = target->id;
                actor->order.anchor = actor->position;
                actor->order.destination = target->position;
                outcome = CommandResolutionOutcome::Applied;
            }
            return;
        }
        case CommandType::FutureWell: {
            const Entity* target = FindEntity(command.target);
            const PlayerState* player = FindPlayer(command.player);
            if (actor->type == EntityType::Worker && target != nullptr &&
                IsOperationalFutureWell(*target) &&
                target->wellProtocolTicks == 0 &&
                (command.wellChoice != FutureWellChoice::Reshape ||
                 (player != nullptr && player->resources.dawnshards >=
                    config_.rules.futureWell.reshapeDawnCost)) &&
                (target->wellChoice == FutureWellChoice::Dormant ||
                 (target->wellChoice == FutureWellChoice::Preserve &&
                  config_.IsHostile(command.player, target->owner))) &&
                command.wellChoice != FutureWellChoice::Dormant &&
                IsEntityVisibleTo(command.player, target->id)) {
                if (command.queue && actor->order.type != OrderType::None) {
                    if (actor->orderQueue.size() < Entity::kMaxQueuedOrders) {
                        Order queued{};
                        queued.type = OrderType::FutureWell;
                        queued.target = target->id;
                        queued.anchor = actor->position;
                        queued.destination = target->position;
                        queued.wellChoice = command.wellChoice;
                        actor->orderQueue.push_back(queued);
                    }
                    outcome = CommandResolutionOutcome::Applied;
                    return;
                }
                actor->order.type = OrderType::FutureWell;
                actor->order.target = target->id;
                actor->order.anchor = actor->position;
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
            ProductionQueueItem item{};
            if (nextProductionItemId_ == 0 ||
                nextProductionItemId_ ==
                    std::numeric_limits<ProductionItemId>::max()) {
                return;
            }
            item.itemId = nextProductionItemId_;
            item.unitType = command.buildType;
            item.configuredCost =
                ProductionCost(player->faction, command.buildType);
            item.requiredTicks =
                ProductionTicks(player->faction, command.buildType);
            item.logisticsCost =
                PopulationCost(player->faction, command.buildType);
            if (actor->productionRequired > 0 ||
                !actor->productionQueue.empty()) {
                actor->productionQueue.push_back(item);
                ++nextProductionItemId_;
                productionTransitionReceipts_[command.player].push_back({
                    currentTick_, actor->id, item.itemId, 0, item.unitType,
                    ProductionTransition::Queued,
                    ProductionStartBlockReason::Busy, {}, {}, 0,
                    command.sequence});
                if (actor->productionRequired <= 0) {
                    (void)TryActivateNextProduction(*actor);
                }
            } else {
                actor->productionQueue.insert(
                    actor->productionQueue.begin(), item);
                if (!TryActivateNextProduction(*actor)) {
                    actor->productionQueue.erase(actor->productionQueue.begin());
                    return;
                }
                ++nextProductionItemId_;
                if (!productionTransitionReceipts_[command.player].empty()) {
                    productionTransitionReceipts_[command.player].back()
                        .commandSequence = command.sequence;
                }
            }
            outcome = CommandResolutionOutcome::Applied;
            return;
        }
        case CommandType::CancelProduction: {
            PlayerState* player = MutablePlayer(command.player);
            if (player == nullptr || !actor->completed ||
                (actor->type != EntityType::CommandCore &&
                 actor->type != EntityType::Barracks)) {
                return;
            }
            const std::uint32_t slot = command.target;
            const ProductionItemId expectedItemId =
                static_cast<ProductionItemId>(
                    static_cast<std::uint32_t>(command.position.x.Raw())) |
                (static_cast<ProductionItemId>(
                     static_cast<std::uint32_t>(command.position.y.Raw()))
                 << 32U);
            if (slot == 0) {
                if (actor->productionRequired <= 0) {
                    return;
                }
                if (!legacyLinkReplaySemantics_ &&
                    expectedItemId != actor->activeProductionItemId) {
                    return;
                }
                const std::int32_t refundPercent =
                    static_cast<std::int64_t>(actor->productionProgress) * 2 <
                            actor->productionRequired
                        ? 75
                        : 50;
                const auto Refund = [refundPercent](std::int32_t invested) {
                    return static_cast<std::int32_t>(
                        static_cast<std::int64_t>(invested) * refundPercent /
                        100);
                };
                player->resources.material = SaturatingAdd(
                    player->resources.material,
                    Refund(actor->productionInvestedCost.material));
                player->resources.dawnshards = SaturatingAdd(
                    player->resources.dawnshards,
                    Refund(actor->productionInvestedCost.dawnshards));
                const ProductionItemId cancelledId =
                    actor->activeProductionItemId;
                const EntityType cancelledType = actor->productionType;
                const ResourcePool refunded{
                    Refund(actor->productionInvestedCost.material),
                    Refund(actor->productionInvestedCost.dawnshards)};
                const std::int32_t releasedLogistics =
                    actor->productionLogisticsCost;
                ClearActiveProduction(*actor);
                productionTransitionReceipts_[command.player].push_back({
                    currentTick_, actor->id, cancelledId, 0, cancelledType,
                    ProductionTransition::Cancelled,
                    ProductionStartBlockReason::None, {}, refunded,
                    -releasedLogistics, command.sequence});
                (void)TryActivateNextProduction(*actor);
                outcome = CommandResolutionOutcome::Applied;
                return;
            }
            if (slot > actor->productionQueue.size()) {
                return;
            }
            const std::size_t waitingIndex =
                static_cast<std::size_t>(slot - 1);
            const ProductionQueueItem cancelled =
                actor->productionQueue[waitingIndex];
            if (!legacyLinkReplaySemantics_ &&
                expectedItemId != cancelled.itemId) {
                return;
            }
            player->resources.material = SaturatingAdd(
                player->resources.material, cancelled.investedCost.material);
            player->resources.dawnshards = SaturatingAdd(
                player->resources.dawnshards,
                cancelled.investedCost.dawnshards);
            actor->productionQueue.erase(
                actor->productionQueue.begin() +
                static_cast<std::ptrdiff_t>(waitingIndex));
            productionTransitionReceipts_[command.player].push_back({
                currentTick_, actor->id, cancelled.itemId, 0,
                cancelled.unitType, ProductionTransition::Cancelled,
                ProductionStartBlockReason::None, {}, cancelled.investedCost,
                0, command.sequence});
            outcome = CommandResolutionOutcome::Applied;
            return;
        }
        case CommandType::ReorderProduction: {
            if (!actor->completed ||
                (actor->type != EntityType::CommandCore &&
                 actor->type != EntityType::Barracks) ||
                (command.target & 0xffff0000U) != 0) {
                return;
            }
            const std::uint32_t fromSlot = command.target & 0xffU;
            const std::uint32_t toSlot = (command.target >> 8U) & 0xffU;
            if (fromSlot == 0 || toSlot == 0 ||
                fromSlot > actor->productionQueue.size() ||
                toSlot > actor->productionQueue.size()) {
                return;
            }
            if (fromSlot != toSlot) {
                ProductionQueueItem moved = actor->productionQueue[
                    static_cast<std::size_t>(fromSlot - 1)];
                actor->productionQueue.erase(
                    actor->productionQueue.begin() +
                    static_cast<std::ptrdiff_t>(fromSlot - 1));
                actor->productionQueue.insert(
                    actor->productionQueue.begin() +
                        static_cast<std::ptrdiff_t>(toSlot - 1),
                    moved);
            }
            outcome = CommandResolutionOutcome::Applied;
            return;
        }
        case CommandType::SetRallyRoute: {
            if (!actor->completed ||
                (actor->type != EntityType::CommandCore &&
                 actor->type != EntityType::Barracks)) {
                return;
            }
            Order rally{};
            rally.anchor = actor->position;
            rally.destination = command.position;
            if (command.target == 0) {
                if (!IsInsideMap(command.position) ||
                    !IsPositionPassable(command.position)) {
                    return;
                }
                rally.type = OrderType::Move;
            } else {
                const Entity* target = FindEntity(command.target);
                if (target == nullptr || target->hitPoints <= 0 ||
                    !IsEntityVisibleTo(command.player, target->id)) {
                    return;
                }
                rally.target = target->id;
                rally.destination = target->position;
                if (target->type == EntityType::ResourceNode &&
                    target->resourceRemaining > 0) {
                    rally.type = OrderType::Gather;
                } else if (target->owner != kNeutralPlayer &&
                           !config_.IsHostile(command.player, target->owner)) {
                    rally.type = OrderType::Guard;
                } else {
                    return;
                }
            }
            if (!command.queue) {
                actor->rallyRoute.clear();
            }
            if (actor->rallyRoute.size() >= Entity::kMaxRallyOrders) {
                return;
            }
            actor->rallyRoute.push_back(rally);
            actor->rallyRouteAlert = false;
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
                if (command.queue && actor->order.type != OrderType::None) {
                    if (actor->orderQueue.size() < Entity::kMaxQueuedOrders) {
                        Order queued{};
                        queued.type = OrderType::AttackMove;
                        queued.anchor = actor->position;
                        queued.destination = command.position;
                        actor->orderQueue.push_back(queued);
                    }
                    outcome = CommandResolutionOutcome::Applied;
                    return;
                }
                actor->order.type = OrderType::AttackMove;
                actor->order.target = 0;
                actor->order.anchor = actor->position;
                actor->order.destination = command.position;
                outcome = CommandResolutionOutcome::Applied;
            }
            return;
        case CommandType::Hold:
            if (actor->attackDamage > 0) {
                if (command.queue && actor->order.type != OrderType::None) {
                    if (actor->orderQueue.size() < Entity::kMaxQueuedOrders) {
                        Order queued{};
                        queued.type = OrderType::Hold;
                        queued.target = 0;
                        queued.anchor = actor->position;
                        queued.destination = actor->position;
                        actor->orderQueue.push_back(queued);
                    }
                    outcome = CommandResolutionOutcome::Applied;
                    return;
                }
                actor->order.type = OrderType::Hold;
                actor->order.target = 0;
                actor->order.anchor = actor->position;
                actor->order.destination = actor->position;
                outcome = CommandResolutionOutcome::Applied;
            }
            return;
        case CommandType::Guard: {
            const Entity* guarded = FindEntity(command.target);
            if (actor->attackDamage > 0 && guarded != nullptr &&
                guarded->owner != kNeutralPlayer &&
                !config_.IsHostile(command.player, guarded->owner) &&
                guarded->id != actor->id) {
                if (command.queue && actor->order.type != OrderType::None) {
                    if (actor->orderQueue.size() < Entity::kMaxQueuedOrders) {
                        Order queued{};
                        queued.type = OrderType::Guard;
                        queued.target = guarded->id;
                        queued.anchor = actor->position;
                        queued.destination = guarded->position;
                        actor->orderQueue.push_back(queued);
                    }
                    outcome = CommandResolutionOutcome::Applied;
                    return;
                }
                actor->order.type = OrderType::Guard;
                actor->order.target = guarded->id;
                actor->order.anchor = actor->position;
                actor->order.destination = guarded->position;
                outcome = CommandResolutionOutcome::Applied;
            }
            return;
        }
        case CommandType::Patrol:
            if (actor->attackDamage > 0 && actor->movementPerTickRaw > 0 &&
                IsPositionPassable(command.position) &&
                command.position != actor->position) {
                if (command.queue && actor->order.type != OrderType::None) {
                    if (actor->orderQueue.size() < Entity::kMaxQueuedOrders) {
                        Order queued{};
                        queued.type = OrderType::Patrol;
                        queued.target = 0;
                        queued.anchor = actor->position;
                        queued.destination = command.position;
                        actor->orderQueue.push_back(queued);
                    }
                    outcome = CommandResolutionOutcome::Applied;
                    return;
                }
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
            if (actor->deploymentPhase != BulwarkDeploymentPhase::None) {
                return;
            }
            if (!legacyBulwarkReplaySemantics_ && currentTick_ > kMaximumSupportedTick -
                    (actor->deployed ? kBulwarkPackTicks : kBulwarkDeployTicks)) return;
            if (actor->deployed) {
                if (legacyBulwarkReplaySemantics_) {
                    actor->deployed = false;
                } else {
                    actor->deploymentPhase = BulwarkDeploymentPhase::Packing;
                    actor->deploymentTransitionUntilTick = currentTick_ + kBulwarkPackTicks;
                }
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
            if (legacyBulwarkReplaySemantics_) {
                actor->deployed = true;
            } else {
                actor->deploymentPhase = BulwarkDeploymentPhase::Deploying;
                actor->deploymentTransitionUntilTick = currentTick_ + kBulwarkDeployTicks;
            }
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
            MarkStructureOccupancyDirty();
            outcome = CommandResolutionOutcome::Applied;
            return;
        }
        }
    };
    apply();
    // Some commands append entities and invalidate actor pointers.
    actor = MutableEntity(command.actor);
    if (outcome == CommandResolutionOutcome::Applied && actor != nullptr &&
        actor->type == EntityType::Worker &&
        !(command.queue && previousOrder.type != OrderType::None) &&
        (actor->order != previousOrder || command.type == CommandType::Stop ||
         command.type == CommandType::Gather || command.type == CommandType::Deliver)) {
        actor->orderQueue.clear();
        if (actor->order.type == OrderType::Gather) {
            BeginGather(*actor, actor->order.target);
        } else {
            ClearHarvestState(*actor);
            if (actor->order.type == OrderType::Deliver) {
                actor->assignedResourceNode = previousNode;
                actor->harvestState = HarvestState::ReturningHome;
            }
        }
    }
    return outcome;
}

void Simulation::ClearHarvestState(Entity& worker) {
    worker.harvestState = HarvestState::Idle;
    worker.harvestQueueTicket = 0;
    worker.harvestSlotHeld = false;
    worker.harvestTicks = 0;
    worker.assignedResourceNode = 0;
}

void Simulation::BeginGather(Entity& worker, EntityId node) {
    ClearHarvestState(worker);
    worker.assignedResourceNode = node;
    worker.harvestState = HarvestState::MovingToResource;
    worker.order.type = OrderType::Gather;
    worker.order.target = node;
    if (const Entity* resource = FindEntity(node)) {
        worker.order.destination = resource->position;
    }
}

void Simulation::ReconcileHarvestReservations() {
    // Owner harvesting directive: one non-preemptive extraction position. Queue admission is based on
    // arrival, with entity ID resolving arrivals on the same simulation tick.
    std::map<EntityId, std::vector<Entity*>> queues;
    for (Entity& worker : entities_) {
        if (worker.type != EntityType::Worker) {
            continue;
        }
        if (worker.hitPoints <= 0 || !worker.completed ||
            (worker.order.type != OrderType::Gather &&
             worker.order.type != OrderType::Deliver)) {
            ClearHarvestState(worker);
            continue;
        }
        if (worker.order.type == OrderType::Deliver) {
            worker.harvestSlotHeld = false;
            worker.harvestQueueTicket = 0;
            continue;
        }
        if (worker.assignedResourceNode != worker.order.target ||
            worker.harvestState == HarvestState::Idle) {
            BeginGather(worker, worker.order.target);
        }
        const Entity* resource = FindEntity(worker.order.target);
        if (resource == nullptr || resource->hitPoints <= 0 ||
            resource->type != EntityType::ResourceNode ||
            resource->resourceRemaining <= 0 ||
            (worker.harvestQueueTicket == 0 &&
             (!InInteractionRange(worker, *resource, kFixedScale / 2) ||
              !HasLineOfSight(worker.position, resource->position)))) {
            worker.harvestSlotHeld = false;
            worker.harvestQueueTicket = 0;
            worker.harvestTicks = 0;
            worker.harvestState = HarvestState::MovingToResource;
            continue;
        }
        worker.harvestState = HarvestState::Harvesting;
        if (worker.harvestQueueTicket == 0) {
            worker.harvestQueueTicket = currentTick_ + 1;
        }
        queues[resource->id].push_back(&worker);
    }
    for (auto& [node, workers] : queues) {
        (void)node;
        std::sort(workers.begin(), workers.end(), [](const Entity* a, const Entity* b) {
            return std::tie(a->harvestQueueTicket, a->id) <
                   std::tie(b->harvestQueueTicket, b->id);
        });
        std::size_t occupied = std::count_if(workers.begin(), workers.end(),
            [](const Entity* worker) { return worker->harvestSlotHeld; });
        std::size_t waitingIndex = 0;
        const Entity* resource = FindEntity(node);
        constexpr std::array<std::array<std::int32_t, 2>, 8> directions{{
            {{-1, 0}}, {{-1, -1}}, {{0, -1}}, {{1, -1}},
            {{1, 0}}, {{1, 1}}, {{0, 1}}, {{-1, 1}},
        }};
        for (Entity* worker : workers) {
            if (!worker->harvestSlotHeld && occupied < 1) {
                worker->harvestSlotHeld = true;
                ++occupied;
            }
            worker->order.destination = resource->position;
            if (!worker->harvestSlotHeld) {
                // Deterministic parking rings beside the deposit. Navigation
                // moves to the anchor; no teleport or renderer-only queue.
                const std::size_t slot = waitingIndex++;
                const std::int32_t radius =
                    FootprintHalfExtentRaw(resource->faction, resource->type) +
                    (2 + static_cast<std::int32_t>(slot / directions.size())) * kFixedScale;
                for (std::size_t attempt = 0; attempt < directions.size(); ++attempt) {
                    const auto& direction = directions[(slot + attempt) % directions.size()];
                    const Vec2 point = Vec2::FromRaw(
                        resource->position.x.Raw() + direction[0] * radius,
                        resource->position.y.Raw() + direction[1] * radius);
                    if (HasLineOfSight(point, point,
                        FootprintHalfExtentRaw(worker->faction, worker->type))) {
                        worker->order.destination = point;
                        break;
                    }
                }
            }
        }
    }
}

void Simulation::ReturnHarvestCargo(Entity& worker) {
    worker.harvestSlotHeld = false;
    worker.harvestQueueTicket = 0;
    worker.harvestTicks = 0;
    if (worker.cargo <= 0) {
        ClearHarvestState(worker);
        worker.order = {};
        return;
    }
    worker.harvestState = HarvestState::ReturningHome;
    worker.order.type = OrderType::Deliver;
    worker.order.target = FindNearestOwnedDropoff(worker.owner, worker.position);
    if (const Entity* depot = FindEntity(worker.order.target)) {
        worker.order.destination = depot->position;
    }
    // No operational depot: retain cargo and the return order. A subsequent
    // tick can route it when a depot is restored; no resources are fabricated.
}

void Simulation::ProcessGather(Entity& worker) {
    Entity* resource = MutableEntity(worker.order.target);
    if (resource == nullptr || resource->hitPoints <= 0 ||
        resource->type != EntityType::ResourceNode || resource->resourceRemaining <= 0) {
        ReturnHarvestCargo(worker);
        return;
    }
    if (worker.cargoCapacity <= 0 || worker.workRate <= 0) {
        ClearHarvestState(worker);
        worker.order = {};
        return;
    }
    if (worker.cargo >= worker.cargoCapacity) {
        ReturnHarvestCargo(worker);
        return;
    }
    if (worker.harvestQueueTicket != 0 && !worker.harvestSlotHeld) {
        worker.harvestState = HarvestState::Harvesting;
        (void)MoveTowards(worker, worker.order.destination);
        return;
    }
    if (!InInteractionRange(worker, *resource, kFixedScale / 2) ||
        !HasLineOfSight(worker.position, resource->position)) {
        // A promoted waiter keeps ownership while approaching the contact.
        worker.harvestState = worker.harvestQueueTicket != 0
            ? HarvestState::Harvesting : HarvestState::MovingToResource;
        (void)MoveTowards(worker, resource->position);
        return;
    }
    worker.harvestState = HarvestState::Harvesting;
    if (!worker.harvestSlotHeld) {
        return;
    }
    ++worker.harvestTicks;
    // Completion-time extraction. Retain existing capacities and derive the
    // duration from their existing rates; the calibrated 10-load takes20 ticks.
    const Tick duration = worker.cargoCapacity == 10 ? 20 :
        static_cast<Tick>((static_cast<std::int64_t>(worker.cargoCapacity) +
                          worker.workRate - 1) / worker.workRate);
    if (worker.harvestTicks < duration) {
        return;
    }
    const std::int32_t gathered = std::min(
        worker.cargoCapacity - worker.cargo, resource->resourceRemaining);
    worker.cargo += gathered;
    resource->resourceRemaining -= gathered;
    ReturnHarvestCargo(worker);
}

void Simulation::ProcessDeliver(Entity& worker) {
    worker.harvestState = HarvestState::ReturningHome;
    Entity* dropoff = MutableEntity(worker.order.target);
    if (dropoff == nullptr || dropoff->hitPoints <= 0 ||
        dropoff->owner != worker.owner || !dropoff->completed ||
        !IsOperationalDropoff(*dropoff)) {
        ReturnHarvestCargo(worker);
        return;
    }
    if (!InStructureReach(worker, *dropoff, kFixedScale / 2) ||
        !HasLineOfSight(worker.position, dropoff->position)) {
        const Vec2 before = worker.position;
        (void)MoveTowards(worker, dropoff->position);
        if (worker.position == before) {
            ReturnHarvestCargo(worker);
            return;
        }
        if (InStructureReach(worker, *dropoff, kFixedScale / 2) &&
            HasLineOfSight(worker.position, dropoff->position)) {
            worker.harvestState = HarvestState::Delivering;
        }
        return;
    }
    worker.harvestState = HarvestState::Delivering;
    PlayerState* player = MutablePlayer(worker.owner);
    if (player != nullptr && worker.cargo > 0) {
        const std::int32_t materialBefore = player->resources.material;
        player->resources.material = SaturatingAdd(materialBefore, worker.cargo);
        const std::int32_t credited =
            player->resources.material - materialBefore;
        if (credited > 0 && worker.owner < materialDeliveryReceipts_.size()) {
            materialDeliveryReceipts_[worker.owner].push_back(
                MaterialDeliveryReceipt{currentTick_, worker.id, credited});
        }
        worker.cargo = 0;
    }
    const Entity* node = FindEntity(worker.assignedResourceNode);
    if (node != nullptr && node->hitPoints > 0 &&
        node->type == EntityType::ResourceNode && node->resourceRemaining > 0) {
        BeginGather(worker, node->id);
    } else {
        // SPEC-RES-006: finish the last load, then idle. Never discover or
        // silently assign a different deposit after exhaustion.
        ClearHarvestState(worker);
        worker.order = {};
    }
}

void Simulation::ProcessBuild(Entity& worker) {
    Entity* site = MutableEntity(worker.order.target);
    if (site == nullptr || site->owner != worker.owner || site->completed ||
        (!legacyConstructionAssistReplaySemantics_ && site->hitPoints <= 0) ||
        !IsBuilding(site->type)) {
        worker.order = {};
        return;
    }
    if (!InStructureReach(worker, *site, kFixedScale / 2)) {
        (void)MoveTowards(worker, site->position);
        return;
    }

    const std::int32_t progressBefore = site->constructionProgress;
    if (worker.cargoCapacity > 12) {
        // Legacy unit fixture (e.g. test 2): uses workRate directly
        site->constructionProgress = std::min(
            site->constructionRequired,
            SaturatingAdd(site->constructionProgress, worker.workRate));
    } else {
        // REL-BLD-004: Multi-builder assist diminishing returns:
        // 1st builder = 100%, 2nd = +60%, 3rd = +40%, 4th+ = +0% (cap 200% / 2.0x)
        std::int32_t builderRank = 0;
        for (const Entity& other : entities_) {
            if (other.id != worker.id && other.owner == worker.owner &&
                other.type == EntityType::Worker && other.order.type == OrderType::Build &&
                other.order.target == site->id &&
                InStructureReach(other, *site, kFixedScale / 2)) {
                if (other.id < worker.id) {
                    builderRank++;
                }
            }
        }

        std::int32_t subProgressRate = 0;
        if (builderRank == 0) {
            subProgressRate = 100; // 100% speed = 1 progress unit / tick
        } else if (builderRank == 1) {
            subProgressRate = 60;  // +60% speed
        } else if (builderRank == 2) {
            subProgressRate = 40;  // +40% speed
        } else {
            subProgressRate = 0;   // 4th+ builder = +0% (capped at 2.0x)
        }

        site->constructionSubProgress += subProgressRate;
        const std::int32_t progressAdvance = site->constructionSubProgress / 100;
        site->constructionSubProgress %= 100;

        if (progressAdvance > 0) {
            site->constructionProgress = std::min(
                site->constructionRequired,
                SaturatingAdd(site->constructionProgress, progressAdvance));
        }
    }

    const std::int64_t scaledHealth =
        static_cast<std::int64_t>(site->maxHitPoints) * site->constructionProgress /
        std::max(1, site->constructionRequired);
    site->hitPoints =
        std::max(site->hitPoints, static_cast<std::int32_t>(scaledHealth));
    const std::int32_t progressDelta =
        site->constructionProgress - progressBefore;
    if (progressDelta > 0 && worker.owner < constructionReceipts_.size()) {
        constructionReceipts_[worker.owner].push_back({
            currentTick_, worker.id, site->id,
            ConstructionTransition::Progressed, progressDelta, {}, {}, 0});
    }
    if (site->constructionProgress >= site->constructionRequired) {
        site->completed = true;
        MarkStructureOccupancyDirty();
        site->hitPoints = site->maxHitPoints;
        if (IsChoirCoherenceStructure(*site)) {
            site->choirCoherenceNextChargeTick = std::min(
                kMaximumSupportedTick,
                currentTick_ +
                    config_.rules.choirCoherence.upkeepIntervalTicks);
        }
        if (worker.owner < constructionReceipts_.size()) {
            constructionReceipts_[worker.owner].push_back({
                currentTick_, worker.id, site->id,
                ConstructionTransition::Completed, 0, {}, {}, 0});
        }
        worker.order = {};
    }
}

void Simulation::ProcessRepair(Entity& worker) {
    Entity* target = MutableEntity(worker.order.target);
    if (target == nullptr || worker.type != EntityType::Worker ||
        target->hitPoints <= 0 || target->owner == kNeutralPlayer ||
        config_.IsHostile(worker.owner, target->owner) ||
        (!target->completed && !IsBuilding(target->type)) ||
        (worker.faction != Faction::MeridianCompact &&
         (!target->completed || !IsBuilding(target->type)))) {
        worker.order = {};
        return;
    }
    if (worker.faction == Faction::MeridianCompact &&
        !IsPositionInMeridianNetwork(worker.owner, worker.position)) {
        worker.order = {};
        return;
    }
    const std::int32_t interactionRange =
        worker.faction == Faction::MeridianCompact
            ? 2 * kFixedScale
            : kFixedScale / 2;
    if (!InStructureReach(worker, *target, interactionRange)) {
        (void)MoveTowards(worker, target->position);
        return;
    }
    if (currentTick_ < worker.repairInterruptedUntilTick) {
        return;
    }
    std::int32_t ceiling = target->maxHitPoints;
    if (!target->completed) {
        const std::int32_t progressCeiling = static_cast<std::int32_t>(
            static_cast<std::int64_t>(target->maxHitPoints) *
            target->constructionProgress /
            std::max(1, target->constructionRequired));
        ceiling = std::max(1, std::max(target->maxHitPoints / 10,
                                      progressCeiling));
    }
    if (target->hitPoints >= ceiling) {
        worker.order = {};
        return;
    }
    std::int32_t ratePerSecond = 20;
    std::int32_t hitPointsPerMatter = 4;
    if (worker.faction == Faction::MeridianCompact) {
        hitPointsPerMatter = 10;
        std::int32_t rank = 0;
        for (const Entity& other : entities_) {
            if (other.id < worker.id && other.owner == worker.owner &&
                other.type == EntityType::Worker &&
                other.faction == Faction::MeridianCompact &&
                other.order.type == OrderType::Repair &&
                other.order.target == target->id &&
                other.hitPoints > 0 && other.completed &&
                currentTick_ >= other.repairInterruptedUntilTick &&
                InStructureReach(other, *target, 2 * kFixedScale)) {
                ++rank;
            }
        }
        ratePerSecond = rank == 0 ? 10 : rank == 1 ? 6 : rank == 2 ? 4 : 0;
    }
    if (ratePerSecond <= 0) {
        return;
    }
    worker.repairRateRemainder = SaturatingAdd(
        worker.repairRateRemainder, ratePerSecond);
    std::int32_t availableHitPoints = static_cast<std::int32_t>(
        worker.repairRateRemainder /
        static_cast<std::int32_t>(config_.ticksPerSecond));
    worker.repairRateRemainder %=
        static_cast<std::int32_t>(config_.ticksPerSecond);
    if (availableHitPoints <= 0) {
        return;
    }
    PlayerState* player = MutablePlayer(worker.owner);
    if (player == nullptr) {
        worker.order = {};
        return;
    }
    std::int32_t restored = 0;
    std::int32_t spent = 0;
    while (availableHitPoints-- > 0 && target->hitPoints < ceiling) {
        if (worker.repairPaidHitPointCredit <= 0) {
            if (player->resources.material <= 0) {
                worker.order = {};
                break;
            }
            --player->resources.material;
            ++spent;
            worker.repairPaidHitPointCredit = hitPointsPerMatter;
        }
        ++target->hitPoints;
        --worker.repairPaidHitPointCredit;
        ++restored;
    }
    if (restored > 0 && worker.owner < repairReceipts_.size()) {
        repairReceipts_[worker.owner].push_back(
            {currentTick_, worker.id, target->id, restored, spent});
    }
    if (target->hitPoints >= ceiling) {
        worker.order = {};
    }
}

namespace {
// SPEC-CMB-013: a body occludes with at least this radius. Pathing footprints
// are 12.5 cm so units can pass in corridors; a soldier's torso is not, and a
// lane rule measured against the footprint fired through nearly everyone.
constexpr std::int64_t kFiringLaneBodyRadiusRaw = 3 * kFixedScale / 5; // 60 cm

// SPEC-CMB-013 (TBR-STR-001): allied mobile bodies occlude the shot the way
// Mineral Cover does. Only the segment strictly between the two footprints
// counts, so a shoulder-to-shoulder neighbour never blocks and the target
// itself is never its own obstruction. Shared by the authoritative simulation
// and the player view so presentation reports exactly what the rules apply.
EntityId FriendlyBodyBlockingLaneIn(const std::vector<Entity>& entities,
                                    const SimulationConfig& config,
                                    bool lanesEnforced,
                                    const Entity& attacker,
                                    const Entity& target) {
    if (!lanesEnforced || attacker.attackDamage <= 0) {
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
    const std::int64_t length = IntegerSqrt64(lengthSquared);
    const std::int64_t muzzleClearance =
        FootprintHalfExtentFor(config.rules, attacker.faction, attacker.type);
    const std::int64_t targetClearance =
        FootprintHalfExtentFor(config.rules, target.faction, target.type);
    EntityId nearest = 0;
    std::int64_t nearestDot = std::numeric_limits<std::int64_t>::max();
    for (const Entity& body : entities) {
        if (body.id == attacker.id || body.id == target.id ||
            body.hitPoints <= 0 || !body.completed ||
            body.temporaryMineralCover ||
            config.IsHostile(attacker.owner, body.owner)) {
            continue;
        }
        if (body.type != EntityType::Worker && body.type != EntityType::Soldier &&
            body.type != EntityType::HeavyUnit &&
            body.type != EntityType::ScoutUnit) {
            continue;
        }
        if (body.deployed) {
            continue; // a deployed shield is low; allies fire over it
        }
        const std::int64_t bodyRadius = std::max<std::int64_t>(
            kFiringLaneBodyRadiusRaw,
            FootprintHalfExtentFor(config.rules, body.faction, body.type));
        const std::int64_t bodyX =
            static_cast<std::int64_t>(body.position.x.Raw()) -
            attacker.position.x.Raw();
        const std::int64_t bodyY =
            static_cast<std::int64_t>(body.position.y.Raw()) -
            attacker.position.y.Raw();
        const std::int64_t dot = bodyX * deltaX + bodyY * deltaY;
        const std::int64_t along = dot / length;
        if (along <= muzzleClearance + bodyRadius ||
            along >= length - targetClearance) {
            continue;
        }
        const std::int64_t cross = bodyX * deltaY - bodyY * deltaX;
        const std::int64_t perpendicular = Abs64(cross) / length;
        if (perpendicular >= bodyRadius) {
            continue;
        }
        if (dot < nearestDot) {
            nearestDot = dot;
            nearest = body.id;
        }
    }
    return nearest;
}
}  // namespace

EntityId Simulation::FriendlyBodyBlockingLane(const Entity& attacker,
                                              const Entity& target) const {
    return FriendlyBodyBlockingLaneIn(entities_, config_,
                                      !legacyFiringLaneReplaySemantics_,
                                      attacker, target);
}

EntityId PlayerView::FriendlyBodyBlockingLane(const Entity& attacker,
                                              const Entity& target) const {
    return FriendlyBodyBlockingLaneIn(entities_, config_, firingLanesEnforced_,
                                      attacker, target);
}

bool Simulation::HasLineOfFire(const Entity& attacker, const Entity& target) const {
    if (FriendlyBodyBlockingLane(attacker, target) != 0) {
        return false;
    }
    if (!HasLineOfSight(attacker.position, target.position)) {
        // Destructible mineral cover may receive the shot. A permanent wall
        // before that cover still blocks it; do not spend a cooldown on it.
        const Entity* cover = target.temporaryMineralCover ? &target :
            FindEntity(InterceptingMineralCover(attacker, target));
        if (cover == nullptr) {
            return false;
        }
        const std::int64_t dx = static_cast<std::int64_t>(cover->position.x.Raw()) - attacker.position.x.Raw();
        const std::int64_t dy = static_cast<std::int64_t>(cover->position.y.Raw()) - attacker.position.y.Raw();
        const std::int64_t steps = std::max<std::int64_t>(1, std::max(Abs64(dx), Abs64(dy)) / (kFixedScale / 4) + 1);
        for (std::int64_t step = 0; step < steps; ++step) {
            const Vec2 point = Vec2::FromRaw(
                static_cast<std::int32_t>(attacker.position.x.Raw() + dx * step / steps),
                static_cast<std::int32_t>(attacker.position.y.Raw() + dy * step / steps));
            if (point.x.FloorToInt() == cover->position.x.FloorToInt() &&
                point.y.FloorToInt() == cover->position.y.FloorToInt()) {
                break;
            }
            if (!IsPositionPassable(point)) {
                return false;
            }
        }
    }
    return true;
}

void Simulation::TryFireAt(Entity& attacker, const Entity& target,
                           std::vector<PendingDamage>& pendingDamage) {
    if (attacker.attackCooldownTicks != 0 || target.hitPoints <= 0 ||
        !config_.IsHostile(attacker.owner, target.owner) ||
        !HasLineOfFire(attacker, target)) {
        return;
    }
    if (config_.enableBallisticProjectiles) {
        SpawnBallisticProjectile(attacker, target, attacker.attackDamage);
    } else {
        pendingDamage.push_back({target.id, attacker.id, attacker.attackDamage});
    }
    attacker.attackCooldownTicks = attacker.attackPeriodTicks;
}

void Simulation::ProcessAttack(
    Entity& attacker,
    std::vector<PendingDamage>& pendingDamage) {
    Entity* target = MutableEntity(attacker.order.target);
    if (target == nullptr || !config_.IsHostile(attacker.owner, target->owner) ||
        IsProtectedCommandCore(*target) ||
        !IsEntityVisibleTo(attacker.owner, target->id)) {
        attacker.order = {};
        return;
    }
    if (!InInteractionRange(attacker, *target, attacker.attackRangeRaw) ||
        !HasLineOfFire(attacker, *target)) {
        // SPEC-CMD-015: Focus-Fire Target Preservation on Range Loss with bounded 400 cm chase radius
        if (attacker.order.anchor == Vec2{}) {
            attacker.order.anchor = attacker.position;
        }
        const std::int64_t deltaX =
            static_cast<std::int64_t>(attacker.position.x.Raw()) -
            attacker.order.anchor.x.Raw();
        const std::int64_t deltaY =
            static_cast<std::int64_t>(attacker.position.y.Raw()) -
            attacker.order.anchor.y.Raw();
        constexpr std::int64_t kMaxChaseDistanceRaw = 4 * kFixedScale; // 400 cm
        if (deltaX * deltaX + deltaY * deltaY >
            kMaxChaseDistanceRaw * kMaxChaseDistanceRaw) {
            attacker.order = {};
            const EntityId localEnemy = FindNearestVisibleEnemyInRange(attacker);
            if (localEnemy != 0) {
                attacker.order.type = OrderType::Attack;
                attacker.order.target = localEnemy;
                attacker.order.anchor = attacker.position;
            }
            return;
        }
        (void)MoveTowards(attacker, target->position);
        return;
    }
    attacker.order.anchor = attacker.position;
    TryFireAt(attacker, *target, pendingDamage);
}

void Simulation::ProcessAttackMove(
    Entity& attacker,
    std::vector<PendingDamage>& pendingDamage) {
    if (attacker.attackDamage <= 0 || attacker.movementPerTickRaw <= 0) {
        attacker.order = {};
        return;
    }

    const auto IsArmedOrMobile = [this](const Entity& e) {
        return e.attackDamage > 0 || (!IsBuilding(e.type) && e.movementPerTickRaw > 0);
    };

    Entity* target = attacker.order.target != 0
                         ? MutableEntity(attacker.order.target)
                         : nullptr;
    if (target == nullptr || !config_.IsHostile(attacker.owner, target->owner) ||
        (target != nullptr && IsProtectedCommandCore(*target)) ||
        !IsEntityVisibleTo(attacker.owner, target->id)) {
        attacker.order.target = 0;
        target = nullptr;
    }

    // SPEC-CMD-014: Attack-Move Intelligent Threat Filtering
    // Prioritize armed combatants and mobile threats over passive non-threatening buildings
    const std::uint64_t visionDistSquared =
        static_cast<std::uint64_t>(attacker.visionTiles * kFixedScale) *
        (attacker.visionTiles * kFixedScale);

    if (target == nullptr || !IsArmedOrMobile(*target)) {
        EntityId priorityThreat = 0;
        std::uint64_t nearestThreatDist = std::numeric_limits<std::uint64_t>::max();
        for (const Entity& enemy : entities_) {
            if (!config_.IsHostile(attacker.owner, enemy.owner) ||
                enemy.hitPoints <= 0 || IsProtectedCommandCore(enemy) ||
                !IsEntityVisibleTo(attacker.owner, enemy.id) ||
                !IsArmedOrMobile(enemy)) {
                continue;
            }
            const std::uint64_t dist = DistanceSquaredRaw(attacker.position, enemy.position);
            if (dist <= visionDistSquared &&
                (dist < nearestThreatDist || (dist == nearestThreatDist && (priorityThreat == 0 || enemy.id < priorityThreat)))) {
                nearestThreatDist = dist;
                priorityThreat = enemy.id;
            }
        }
        if (priorityThreat != 0) {
            attacker.order.target = priorityThreat;
            target = MutableEntity(priorityThreat);
        }
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
        if (!InInteractionRange(attacker, *target, attacker.attackRangeRaw) ||
            !HasLineOfFire(attacker, *target)) {
            (void)MoveTowards(attacker, target->position);
            return;
        }
        TryFireAt(attacker, *target, pendingDamage);
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
    if (target == nullptr || !config_.IsHostile(attacker.owner, target->owner) ||
        (target != nullptr && IsProtectedCommandCore(*target)) ||
        !IsEntityVisibleTo(attacker.owner, target->id) ||
        !InInteractionRange(attacker, *target, attacker.attackRangeRaw) ||
        !HasLineOfFire(attacker, *target)) {
        attacker.order.target = 0;
        target = nullptr;
    }
    if (target == nullptr) {
        attacker.order.target = FindNearestVisibleEnemyInRange(attacker);
        target = attacker.order.target != 0
                     ? MutableEntity(attacker.order.target)
                     : nullptr;
    }
    if (target != nullptr) {
        TryFireAt(attacker, *target, pendingDamage);
    }
}

void Simulation::ProcessGuard(
    Entity& attacker,
    std::vector<PendingDamage>& pendingDamage) {
    Entity* guarded = MutableEntity(attacker.order.target);
    if (attacker.attackDamage <= 0 || guarded == nullptr ||
        guarded->owner == kNeutralPlayer ||
        config_.IsHostile(attacker.owner, guarded->owner) ||
        guarded->id == attacker.id) {
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
        if (!InInteractionRange(attacker, *enemy, attacker.attackRangeRaw) ||
            !HasLineOfFire(attacker, *enemy)) {
            (void)MoveTowards(attacker, enemy->position);
            return;
        }
        TryFireAt(attacker, *enemy, pendingDamage);
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
    if (target == nullptr || !config_.IsHostile(attacker.owner, target->owner) ||
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
        if (!InInteractionRange(attacker, *target, attacker.attackRangeRaw) ||
            !HasLineOfFire(attacker, *target)) {
            (void)MoveTowards(attacker, target->position);
            return;
        }
        TryFireAt(attacker, *target, pendingDamage);
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
    if (const Entity* target = FindEntity(targetId)) {
        TryFireAt(aegis, *target, pendingDamage);
    }
}

void Simulation::ProcessFutureWell(Entity& worker) {
    Entity* well = MutableEntity(worker.order.target);
    if (well == nullptr || !IsOperationalFutureWell(*well) ||
        well->wellProtocolTicks > 0 ||
        (well->wellChoice != FutureWellChoice::Dormant &&
         (well->wellChoice != FutureWellChoice::Preserve ||
          !config_.IsHostile(worker.owner, well->owner)))) {
        worker.order = {};
        return;
    }
    if (!IsFutureWellZoneMember(*well, worker)) {
        (void)MoveTowards(worker, well->position);
        return;
    }
}

bool Simulation::IsCollapsedFutureWell(const Entity& entity) const {
    return entity.type == EntityType::FutureWell &&
           entity.wellChoice == FutureWellChoice::Harvest &&
           entity.wellProtocolTicks == 0;
}

bool Simulation::IsOperationalFutureWell(const Entity& entity) const {
    return entity.type == EntityType::FutureWell && entity.hitPoints > 0 &&
           !IsCollapsedFutureWell(entity);
}

bool Simulation::IsFutureWellZoneMember(const Entity& well,
                                         const Entity& entity) const {
    if (well.type != EntityType::FutureWell || entity.hitPoints <= 0 ||
        entity.owner == kNeutralPlayer || !IsInsideMap(entity.position)) {
        return false;
    }
    const std::int64_t radius = kFutureWellCaptureRadiusRaw;
    return DistanceSquaredRaw(well.position, entity.position) <=
           static_cast<std::uint64_t>(radius * radius);
}

bool Simulation::IsFutureWellContested(const Entity& well) const {
    if (!IsOperationalFutureWell(well) || well.owner == kNeutralPlayer) {
        return false;
    }
    for (const Entity& entity : entities_) {
        if (entity.id != well.id && config_.IsHostile(well.owner, entity.owner) &&
            IsFutureWellZoneMember(well, entity)) {
            return true;
        }
    }
    return false;
}

std::vector<FutureWellTelegraph> Simulation::PublicFutureWellTelegraphs() const {
    std::vector<FutureWellTelegraph> telegraphs{};
    for (const Entity& entity : entities_) {
        if (entity.type == EntityType::FutureWell &&
            (entity.wellChoice == FutureWellChoice::Harvest ||
             entity.wellPendingChoice == FutureWellChoice::Reshape) &&
            entity.wellProtocolTicks > 0) {
            telegraphs.push_back(
                {entity.id, entity.position, entity.wellProtocolTicks,
                 entity.wellPendingChoice == FutureWellChoice::Reshape
                     ? FutureWellChoice::Reshape : FutureWellChoice::Harvest});
        }
    }
    return telegraphs;
}

void Simulation::CompleteFutureWellCapture(Entity& well) {
    const PlayerState* player = FindPlayer(well.wellCapturePlayer);
    if (player == nullptr || well.wellPendingChoice == FutureWellChoice::Dormant ||
        (well.wellPendingChoice == FutureWellChoice::Reshape &&
         player->resources.dawnshards < config_.rules.futureWell.reshapeDawnCost)) {
        // Affordability may change during capture. Leave existing ownership
        // and Preserve benefits intact; an unfunded attempt never commits.
        const PlayerId claimant = well.wellCapturePlayer;
        for (Entity& entity : entities_) {
            if (entity.owner == claimant && entity.order.type == OrderType::FutureWell &&
                entity.order.target == well.id) {
                entity.order = {};
            }
        }
        well.wellCapturePlayer = kNeutralPlayer;
        well.wellCaptureProgress = 0;
        well.wellPendingChoice = FutureWellChoice::Dormant;
        return;
    }
    well.owner = player->id;
    well.faction = player->faction;
    well.wellChoice = well.wellPendingChoice;
    well.wellActivationTick = currentTick_ + 1;
    well.wellCapturePlayer = kNeutralPlayer;
    well.wellCaptureProgress = 0;
    well.wellPendingChoice = FutureWellChoice::Dormant;
    well.wellProtocolTicks = 0;

    if (well.wellChoice == FutureWellChoice::Harvest) {
        well.wellProtocolTicks = kHarvestTelegraphTicks;
    } else if (well.wellChoice == FutureWellChoice::Reshape) {
        PlayerState* mutablePlayer = MutablePlayer(well.owner);
        mutablePlayer->resources.dawnshards -=
            config_.rules.futureWell.reshapeDawnCost;
        // Keep the committed warning distinct from the manifested protocol.
        // Mission reducers must not see an activated or expired route yet.
        well.wellChoice = FutureWellChoice::Dormant;
        well.wellPendingChoice = FutureWellChoice::Reshape;
        well.wellActivationTick = 0;
        well.wellProtocolTicks = kReshapeTelegraphTicks;
    }
    for (Entity& entity : entities_) {
        if (entity.order.type == OrderType::FutureWell &&
            entity.order.target == well.id) {
            entity.order = {};
        }
    }
}

void Simulation::CollapseFutureWell(Entity& well) {
    if (PlayerState* player = MutablePlayer(well.owner); player != nullptr) {
        player->resources.dawnshards = SaturatingAdd(
            player->resources.dawnshards,
            config_.rules.futureWell.harvestImmediateDawn);
    }
    const std::int32_t centerX = well.position.x.FloorToInt();
    const std::int32_t centerY = well.position.y.FloorToInt();
    for (std::int32_t tileY = centerY - 6; tileY <= centerY + 6; ++tileY) {
        for (std::int32_t tileX = centerX - 6; tileX <= centerX + 6; ++tileX) {
            const Vec2 tileCenter = Vec2::FromRaw(
                tileX * kFixedScale + kFixedScale / 2,
                tileY * kFixedScale + kFixedScale / 2);
            if (DistanceSquaredRaw(well.position, tileCenter) <=
                    static_cast<std::uint64_t>(kFutureWellScarRadiusRaw) *
                        kFutureWellScarRadiusRaw &&
                TerrainAt(tileX, tileY) == Terrain::Open) {
                (void)SetTerrainTile(tileX, tileY, Terrain::Scarred);
            }
        }
    }
    well.wellProtocolTicks = 0;
    well.wellCapturePlayer = kNeutralPlayer;
    well.wellCaptureProgress = 0;
    well.wellPendingChoice = FutureWellChoice::Dormant;
}

void Simulation::ProcessFutureWellLifecycles() {
    for (Entity& well : entities_) {
        if (well.type != EntityType::FutureWell || IsCollapsedFutureWell(well)) {
            continue;
        }
        if (well.wellPendingChoice == FutureWellChoice::Reshape &&
            well.wellProtocolTicks > 0) {
            // Capture is contestable until commitment. Reshape has no authored
            // post-commit cancellation/refund rule; the paid public warning
            // completes independently of worker orders or hostile presence.
            if (--well.wellProtocolTicks == 0) {
                well.wellChoice = FutureWellChoice::Reshape;
                well.wellPendingChoice = FutureWellChoice::Dormant;
                well.wellActivationTick = currentTick_ + 1;
                well.reshapeVariant = static_cast<std::uint8_t>(rng_.Uniform(4));
                const Tick minimum = config_.rules.futureWell.reshapeDurationMinimumTicks;
                const Tick maximum = config_.rules.futureWell.reshapeDurationMaximumTicks;
                well.reshapeUntilTick = well.wellActivationTick + minimum +
                    rng_.Uniform(static_cast<std::uint32_t>(maximum - minimum + 1));
                pathFieldCache_.clear();
            }
            continue;
        }
        if (well.wellChoice == FutureWellChoice::Harvest) {
            if (IsFutureWellContested(well)) {
                // A hostile breach interrupts the public telegraph before the
                // payout and leaves a neutral Well for a new capture attempt.
                well.owner = kNeutralPlayer;
                well.faction = Faction::MeridianCompact;
                well.wellChoice = FutureWellChoice::Dormant;
                well.wellActivationTick = 0;
                well.wellProtocolTicks = 0;
                continue;
            }
            if (well.wellProtocolTicks > 0 && --well.wellProtocolTicks == 0) {
                CollapseFutureWell(well);
            }
            continue;
        }
        if (well.wellChoice == FutureWellChoice::Reshape) {
            continue;
        }

        std::array<bool, kMaximumPlayers> capturePlayers{};
        std::array<FutureWellChoice, kMaximumPlayers> captureChoices{};
        std::array<bool, kMaximumPlayers> hostilePresence{};
        for (const Entity& entity : entities_) {
            if (entity.id == well.id || !IsFutureWellZoneMember(well, entity)) {
                continue;
            }
            for (PlayerId player = 0; player < players_.size(); ++player) {
                if (config_.IsHostile(player, entity.owner)) {
                    hostilePresence[player] = true;
                }
            }
            if (entity.type == EntityType::Worker &&
                entity.order.type == OrderType::FutureWell &&
                entity.order.target == well.id && entity.owner < players_.size() &&
                players_[entity.owner].active &&
                entity.order.wellChoice != FutureWellChoice::Dormant &&
                (well.wellChoice == FutureWellChoice::Dormant ||
                 config_.IsHostile(entity.owner, well.owner))) {
                capturePlayers[entity.owner] = true;
                if (captureChoices[entity.owner] == FutureWellChoice::Dormant) {
                    captureChoices[entity.owner] = entity.order.wellChoice;
                }
            }
        }

        PlayerId contender = well.wellCapturePlayer;
        if (contender == kNeutralPlayer || contender >= players_.size() ||
            !capturePlayers[contender]) {
            contender = kNeutralPlayer;
            for (PlayerId player = 0; player < players_.size(); ++player) {
                if (capturePlayers[player]) {
                    contender = player;
                    break;
                }
            }
            if (contender != kNeutralPlayer &&
                contender != well.wellCapturePlayer) {
                well.wellCapturePlayer = contender;
                well.wellCaptureProgress = 0;
                well.wellPendingChoice = captureChoices[contender];
            }
        }
        if (contender == kNeutralPlayer) {
            // Retain the claimant while its abandoned meter decays. A new
            // contender starts its own capture rather than inheriting progress.
            if (well.wellCaptureProgress > 0) {
                --well.wellCaptureProgress;
            }
            if (well.wellCaptureProgress == 0) {
                well.wellCapturePlayer = kNeutralPlayer;
                well.wellPendingChoice = FutureWellChoice::Dormant;
            }
            continue;
        }
        if (hostilePresence[contender]) {
            continue;
        }
        well.wellPendingChoice = captureChoices[contender];
        if (well.wellCaptureProgress < kFutureWellCaptureRequiredTicks) {
            ++well.wellCaptureProgress;
        }
        if (well.wellCaptureProgress == kFutureWellCaptureRequiredTicks) {
            CompleteFutureWellCapture(well);
        }
    }
}

void Simulation::ProcessProduction() {
    std::vector<EntityId> producerIds{};
    producerIds.reserve(entities_.size());
    for (const Entity& entity : entities_) {
        if (entity.hitPoints > 0 && entity.completed &&
            (entity.type == EntityType::CommandCore ||
             entity.type == EntityType::Barracks)) {
            producerIds.push_back(entity.id);
        }
    }
    for (EntityId producerId : producerIds) {
        Entity* producer = MutableEntity(producerId);
        if (producer == nullptr) {
            continue;
        }
        if (producer->productionRequired <= 0) {
            (void)TryActivateNextProduction(*producer);
        }
        if (producer->productionRequired <= 0) {
            continue;
        }
        // REL-FAC-002.PROD: an unpowered Foundry holds its progress and
        // keeps the item; reconnecting resumes it where it stopped.
        if (!producer->productionPausedForSpawn &&
            IsProducerPowered(*producer)) {
            producer->productionProgress = std::min(
                producer->productionRequired,
                SaturatingAdd(producer->productionProgress, 1));
        }
        if (producer->productionProgress < producer->productionRequired) {
            continue;
        }
        const std::optional<Vec2> spawnPosition =
            FindProductionSpawnPosition(*producer);
        if (!spawnPosition.has_value() ||
            entities_.size() >= kMaximumSerializedEntities) {
            const bool wasAlerted = producer->productionSpawnBlockedAlert;
            producer->productionSpawnBlockedTicks = std::min<Tick>(
                100, producer->productionSpawnBlockedTicks + 1);
            if (producer->productionSpawnBlockedTicks >= 100) {
                producer->productionPausedForSpawn = true;
                producer->productionSpawnBlockedAlert = true;
            }
            if (!wasAlerted && producer->productionSpawnBlockedAlert) {
                productionTransitionReceipts_[producer->owner].push_back({
                    currentTick_, producer->id,
                    producer->activeProductionItemId, 0,
                    producer->productionType,
                    ProductionTransition::SpawnBlocked,
                    ProductionStartBlockReason::None, {}, {}, 0, 0});
            }
            continue;
        }
        EntityId unitId = 0;
        if (!TryAllocateEntityId(unitId)) {
            const bool wasAlerted = producer->productionSpawnBlockedAlert;
            producer->productionSpawnBlockedTicks = std::min<Tick>(
                100, producer->productionSpawnBlockedTicks + 1);
            if (producer->productionSpawnBlockedTicks >= 100) {
                producer->productionPausedForSpawn = true;
                producer->productionSpawnBlockedAlert = true;
            }
            if (!wasAlerted && producer->productionSpawnBlockedAlert) {
                productionTransitionReceipts_[producer->owner].push_back({
                    currentTick_, producer->id,
                    producer->activeProductionItemId, 0,
                    producer->productionType,
                    ProductionTransition::SpawnBlocked,
                    ProductionStartBlockReason::None, {}, {}, 0, 0});
            }
            continue;
        }
        const ProductionItemId completedItemId =
            producer->activeProductionItemId;
        const EntityType completedType = producer->productionType;
        const bool resumedFromBlock =
            producer->productionSpawnBlockedTicks > 0;
        Entity unit = MakeEntity(
            producer->owner,
            producer->faction,
            producer->productionType,
            *spawnPosition);
        unit.id = unitId;
        ClearActiveProduction(*producer);
        entities_.push_back(unit);
        MarkStructureOccupancyDirty();
        producer = MutableEntity(producerId);
        Entity* spawned = MutableEntity(unitId);
        if (producer != nullptr && spawned != nullptr) {
            ApplyRallyRoute(*spawned, *producer);
            if (resumedFromBlock) {
                productionTransitionReceipts_[producer->owner].push_back({
                    currentTick_, producer->id, completedItemId, unitId,
                    completedType, ProductionTransition::SpawnResumed,
                    ProductionStartBlockReason::None, {}, {}, 0, 0});
            }
            productionTransitionReceipts_[producer->owner].push_back({
                currentTick_, producer->id, completedItemId, unitId,
                completedType, ProductionTransition::Completed,
                ProductionStartBlockReason::None, {}, {},
                0, 0});
            (void)TryActivateNextProduction(*producer);
        }
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
    ReconcileHarvestReservations();
    // One occupancy snapshot per tick, taken before any unit moves, so the
    // order in which entities are processed cannot change who blocks whom.
    RebuildMobileOccupancy();
    std::vector<PendingDamage> pendingDamage{};
    std::vector<Vec2> positionsBeforeOrders{};
    positionsBeforeOrders.reserve(entities_.size());
    for (const Entity& entity : entities_) {
        positionsBeforeOrders.push_back(entity.position);
    }
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
                if (ShouldPackAtDestination(entity) ||
                    MoveTowards(entity, entity.order.destination)) {
                    if (legacyProductionReplaySemantics_ &&
                        replayChecksumSnapshotVersion_ <
                            kMemorySnapshotVersion) {
                        entity.order = {};
                    } else {
                        entity.order.type = OrderType::None;
                    }
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
            case OrderType::Repair:
                ProcessRepair(entity);
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
        if (entity.order.type == OrderType::None && !entity.orderQueue.empty()) {
            entity.order = entity.orderQueue.front();
            entity.order.anchor = entity.position;
            entity.orderQueue.erase(entity.orderQueue.begin());
            if (entity.order.type == OrderType::Gather) {
                BeginGather(entity, entity.order.target);
            } else {
                ClearHarvestState(entity);
                if (entity.order.type == OrderType::Deliver) {
                    entity.harvestState = HarvestState::ReturningHome;
                }
            }
        }
    }
    ApplySoftSeparation(positionsBeforeOrders);
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
            if (attacker == nullptr || target == nullptr ||
                !config_.IsHostile(attacker->owner, target->owner)) {
                ++index;
                continue;
            }
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
            const std::int32_t appliedDamage =
                static_cast<std::int32_t>(std::min<std::int64_t>(
                    totalDamage, std::numeric_limits<std::int32_t>::max()));
            mutableTarget->hitPoints -= appliedDamage;
            if (appliedDamage > 0 &&
                mutableTarget->order.type == OrderType::Repair) {
                mutableTarget->repairInterruptedUntilTick = std::min(
                    kMaximumSupportedTick, currentTick_ + 20);
            }
        }
    }
}

void Simulation::ApplyPreserveIncome() {
    for (const Entity& entity : entities_) {
        if (entity.type == EntityType::FutureWell &&
            entity.wellChoice == FutureWellChoice::Preserve &&
            !IsFutureWellContested(entity) &&
            entity.wellActivationTick != 0 && currentTick_ + 1 > entity.wellActivationTick &&
            (currentTick_ + 1 - entity.wellActivationTick) %
                    config_.rules.futureWell.preserveIntervalTicks == 0) {
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
        // Exhausted deposits remain as non-interactable terrain landmarks.
        return entity.hitPoints <= 0;
    });
    MarkStructureOccupancyDirty();
}

void Simulation::ClearInvalidOrders() {
    for (Entity& entity : entities_) {
        switch (entity.order.type) {
            case OrderType::Gather:
                if (const Entity* node = FindEntity(entity.order.target);
                    node == nullptr || node->resourceRemaining <= 0) {
                    ReturnHarvestCargo(entity);
                }
                break;
            case OrderType::Deliver:
                // Delivery owns missing-depot recovery and retained cargo.
                break;
            case OrderType::Build:
            case OrderType::Repair:
            case OrderType::Attack:
            case OrderType::FutureWell:
                if (const Entity* target = FindEntity(entity.order.target);
                    target == nullptr ||
                    (entity.order.type == OrderType::FutureWell &&
                     (!IsOperationalFutureWell(*target) ||
                      (target->wellChoice != FutureWellChoice::Dormant &&
                       (target->wellChoice != FutureWellChoice::Preserve ||
                        !config_.IsHostile(entity.owner, target->owner))))) ||
                    (entity.order.type == OrderType::Repair &&
                     (target->hitPoints <= 0 ||
                      config_.IsHostile(entity.owner, target->owner))) ||
                    (entity.order.type == OrderType::Attack &&
                     (!config_.IsHostile(entity.owner, target->owner) ||
                      IsProtectedCommandCore(*target)))) {
                    entity.order = {};
                }
                break;
            case OrderType::AttackMove:
                if (entity.order.target != 0 &&
                    (FindEntity(entity.order.target) == nullptr ||
                     !config_.IsHostile(entity.owner, FindEntity(entity.order.target)->owner) ||
                     IsProtectedCommandCore(
                         *FindEntity(entity.order.target)))) {
                    entity.order.target = 0;
                }
                break;
            case OrderType::Hold:
                if (entity.order.target != 0 &&
                    (FindEntity(entity.order.target) == nullptr ||
                     !config_.IsHostile(entity.owner, FindEntity(entity.order.target)->owner) ||
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
                     !config_.IsHostile(entity.owner, FindEntity(entity.order.target)->owner) ||
                     IsProtectedCommandCore(
                         *FindEntity(entity.order.target)))) {
                    entity.order.target = 0;
                }
                break;
            case OrderType::None:
            case OrderType::Move:
                break;
        }
        if (entity.order.type == OrderType::None && !entity.orderQueue.empty()) {
            entity.order = entity.orderQueue.front();
            entity.order.anchor = entity.position;
            entity.orderQueue.erase(entity.orderQueue.begin());
            if (entity.order.type == OrderType::Gather) {
                BeginGather(entity, entity.order.target);
            } else {
                ClearHarvestState(entity);
                if (entity.order.type == OrderType::Deliver) {
                    entity.harvestState = HarvestState::ReturningHome;
                }
            }
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

        // MOV-004: a unit whose ground closes under it stops at the last safe
        // position it can still reach. The Reshape footprint is the 3x3 around
        // the well, so the nearest safe ground is at most two tiles away; a
        // whole-map scan is not a stop, it is a teleport, and the spec grants
        // no displacement of unbounded distance.
        bool foundFallback = false;
        std::uint64_t bestDistance = std::numeric_limits<std::uint64_t>::max();
        std::size_t bestTile = 0;
        constexpr std::int32_t kReshapeEvictionRadiusTiles = 2;
        for (std::int32_t candidateY = tileY - kReshapeEvictionRadiusTiles;
             candidateY <= tileY + kReshapeEvictionRadiusTiles; ++candidateY) {
            for (std::int32_t candidateX = tileX - kReshapeEvictionRadiusTiles;
                 candidateX <= tileX + kReshapeEvictionRadiusTiles;
                 ++candidateX) {
                if (candidateX < 0 || candidateY < 0 ||
                    candidateX >= config_.mapWidthTiles ||
                    candidateY >= config_.mapHeightTiles ||
                    TerrainAt(candidateX, candidateY) == Terrain::Blocked) {
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
            // Enclosed pockets and invalid all-blocked maps still resolve
            // deterministically without leaving an entity in an inescapable
            // cell: the unit stays exactly where it is and its ground reopens.
            (void)SetTerrainTile(tileX, tileY, Terrain::Open);
        }
        // MOV-004 preserves the order rather than cancelling it. The route is
        // recalculated on the next tick; while no route exists the unit holds
        // its last safe position with the order still standing.
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

void Simulation::ResolveBulwarkTransitions() {
    for (Entity& entity : entities_) {
        if (entity.deploymentPhase != BulwarkDeploymentPhase::None &&
            currentTick_ >= entity.deploymentTransitionUntilTick) {
            entity.deployed = entity.deploymentPhase == BulwarkDeploymentPhase::Deploying;
            entity.deploymentPhase = BulwarkDeploymentPhase::None;
            entity.deploymentTransitionUntilTick = 0;
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
    if (!legacyLinkReplaySemantics_) {
        for (Entity& entity : entities_) {
            entity.networkOperational = false;
            if (entity.owner != kNeutralPlayer && entity.completed &&
                entity.hitPoints > 0 &&
                entity.faction == Faction::MeridianCompact &&
                entity.type == EntityType::CommandCore) {
                entity.networkOperational = true;
            }
        }
        const std::int64_t radius =
            config_.rules.poweredAegis.connectionRadiusRaw;
        const std::uint64_t radiusSquared =
            static_cast<std::uint64_t>(radius * radius);
        bool added = true;
        while (added) {
            added = false;
            for (Entity& candidate : entities_) {
                if (candidate.networkOperational ||
                    candidate.owner == kNeutralPlayer ||
                    !candidate.completed || candidate.hitPoints <= 0 ||
                    candidate.faction != Faction::MeridianCompact ||
                    (candidate.type != EntityType::Dropoff &&
                     candidate.type != EntityType::Barracks)) {
                    continue;
                }
                const bool connected = std::any_of(
                    entities_.begin(), entities_.end(),
                    [&](const Entity& node) {
                        return node.owner == candidate.owner &&
                               node.networkOperational &&
                               DistanceSquaredRaw(candidate.position,
                                                  node.position) <=
                                   radiusSquared;
                    });
                if (connected) {
                    candidate.networkOperational = true;
                    added = true;
                }
            }
        }
    }
    for (Entity& entity : entities_) {
        if (legacyLinkReplaySemantics_) {
            entity.networkOperational = false;
            entity.aegisPowered =
                IsAegisPost(entity) && IsAegisNetworkPowered(entity);
            continue;
        }
        if (IsAegisPost(entity)) {
            entity.aegisPowered =
                entity.completed && entity.hitPoints > 0 &&
                (entity.owner == kNeutralPlayer ||
                 IsPositionInMeridianNetwork(entity.owner, entity.position));
        } else {
            entity.aegisPowered = false;
        }
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

        // REL-FAC-013 & REL-FAC-013.AUTH: Phase Anchors project a 700 cm stabilization field
        // reducing coherence charge by 1 Dawn (5 -> 4). Overlapping fields receive max reduction to 3 Dawn (floor).
        std::int32_t anchorCount = 0;
        constexpr std::uint64_t kPhaseAnchorRadiusRaw = 7 * kFixedScale; // 700 cm
        constexpr std::uint64_t kPhaseAnchorRadiusSquared =
            kPhaseAnchorRadiusRaw * kPhaseAnchorRadiusRaw;
        for (const Entity& anchor : entities_) {
            if (anchor.owner == entity.owner && anchor.completed && anchor.hitPoints > 0 &&
                anchor.faction == Faction::HollowChoir &&
                anchor.type == EntityType::UtilityStructure) {
                if (DistanceSquaredRaw(entity.position, anchor.position) <=
                    kPhaseAnchorRadiusSquared) {
                    anchorCount++;
                }
            }
        }
        const std::int32_t reduction = std::min(2, anchorCount);
        const std::int32_t requiredDawn = std::max(
            anchorCount > 0 ? 3 : 0,
            config_.rules.choirCoherence.dawnCostPerStructure - reduction);

        if (player == nullptr || player->resources.dawnshards < requiredDawn) {
            entity.hitPoints = 0;
            continue;
        }
        player->resources.dawnshards -= requiredDawn;
        entity.choirCoherenceNextChargeTick = std::min(
            kMaximumSupportedTick,
            currentTick_ + config_.rules.choirCoherence.upkeepIntervalTicks);
    }
}

void Simulation::ApplyResolvedDamage(
    Entity& target,
    std::int32_t damage,
    const Entity* attacker) {
    if (IsProtectedCommandCore(target) ||
        (attacker != nullptr && !config_.IsHostile(attacker->owner, target.owner))) {
        return;
    }
    std::int32_t resolvedDamage =
        attacker != nullptr
            ? DamageAfterDirectionalCover(*attacker, target, damage)
            : damage;
    if (target.faction == Faction::KharuunAssemblies &&
        target.type == EntityType::Dropoff &&
        target.waystoneMode != WaystoneMode::Rooted) {
        resolvedDamage = std::max(
            1,
            static_cast<std::int32_t>(
                static_cast<std::int64_t>(resolvedDamage) *
                config_.rules.waystoneMigration.mobileDamageTakenPercent /
                100));
    }
    if (target.pendingWarformAdaptation != WarformAdaptation::None) {
        resolvedDamage = std::max(
            1,
            static_cast<std::int32_t>(
                static_cast<std::int64_t>(resolvedDamage) *
                config_.rules.warformAdaptation.moltDamageTakenPercent /
                100));
    }
    target.hitPoints -= resolvedDamage;
    if (resolvedDamage > 0 && target.order.type == OrderType::Repair) {
        target.repairInterruptedUntilTick = std::min(
            kMaximumSupportedTick, currentTick_ + 20);
    }
}

void Simulation::SpawnBallisticProjectile(
    const Entity& attacker,
    const Entity& target,
    std::int32_t damage) {
    Projectile proj{};
    proj.id = nextProjectileId_++;
    proj.owner = attacker.owner;
    proj.source = attacker.id;
    proj.target = target.id;
    proj.position = attacker.position;
    proj.destination = target.position;
    proj.damage = damage;
    proj.speedRaw = (60 * kFixedScale) / 100; // 1200 cm/s / 20 ticks = 60 cm/tick (614 raw units)
    const std::int64_t dx = static_cast<std::int64_t>(target.position.x.Raw()) - attacker.position.x.Raw();
    const std::int64_t dy = static_cast<std::int64_t>(target.position.y.Raw()) - attacker.position.y.Raw();
    proj.travelDistanceRemainingRaw = static_cast<std::int32_t>(IntegerSqrt64(dx * dx + dy * dy));
    projectiles_.push_back(proj);
}

void Simulation::UpdateProjectiles() {
    std::vector<Projectile> activeProjectiles;
    activeProjectiles.reserve(projectiles_.size());
    for (Projectile& projectile : projectiles_) {
        Entity* target = MutableEntity(projectile.target);
        const Entity* attacker = FindEntity(projectile.source);
        if (target == nullptr || target->hitPoints <= 0 ||
            !config_.IsHostile(projectile.owner, target->owner) || IsProtectedCommandCore(*target)) {
            continue;
        }
        // Tracking is authoritative: a moving target cannot be damaged at its
        // old position. Re-evaluate the current flight segment every tick.
        projectile.destination = target->position;
        const std::int64_t dx = static_cast<std::int64_t>(target->position.x.Raw()) - projectile.position.x.Raw();
        const std::int64_t dy = static_cast<std::int64_t>(target->position.y.Raw()) - projectile.position.y.Raw();
        const std::int64_t distance = IntegerSqrt64(dx * dx + dy * dy);
        const bool arriving = distance <= projectile.speedRaw;
        const Vec2 next = arriving ? target->position : Vec2::FromRaw(
            static_cast<std::int32_t>(projectile.position.x.Raw() + dx * projectile.speedRaw / distance),
            static_cast<std::int32_t>(projectile.position.y.Raw() + dy * projectile.speedRaw / distance));
        const std::int64_t segmentX = static_cast<std::int64_t>(next.x.Raw()) - projectile.position.x.Raw();
        const std::int64_t segmentY = static_cast<std::int64_t>(next.y.Raw()) - projectile.position.y.Raw();
        const std::int64_t steps = std::max<std::int64_t>(1,
            std::max(Abs64(segmentX), Abs64(segmentY)) / (kFixedScale / 4) + 1);
        bool consumed = false;
        for (std::int64_t step = 0; step <= steps; ++step) {
            const Vec2 point = Vec2::FromRaw(
                static_cast<std::int32_t>(projectile.position.x.Raw() + segmentX * step / steps),
                static_cast<std::int32_t>(projectile.position.y.Raw() + segmentY * step / steps));
            if (IsPositionPassable(point)) {
                continue;
            }
            // Resolve the first encountered blocker. Destructible cover takes
            // the hit before the generic blocked-terrain rejection consumes it.
            for (Entity& cover : entities_) {
                if (cover.temporaryMineralCover && cover.hitPoints > 0 &&
                    config_.IsHostile(projectile.owner, cover.owner) &&
                    cover.position.x.FloorToInt() == point.x.FloorToInt() &&
                    cover.position.y.FloorToInt() == point.y.FloorToInt()) {
                    ApplyResolvedDamage(cover, projectile.damage, attacker);
                    break;
                }
            }
            consumed = true;
            break;
        }
        if (consumed) {
            continue;
        }
        if (arriving) {
            ApplyResolvedDamage(*target, projectile.damage, attacker);
            continue;
        }
        projectile.position = next;
        const std::int64_t remainingX = static_cast<std::int64_t>(target->position.x.Raw()) - next.x.Raw();
        const std::int64_t remainingY = static_cast<std::int64_t>(target->position.y.Raw()) - next.y.Raw();
        projectile.travelDistanceRemainingRaw = static_cast<std::int32_t>(
            IntegerSqrt64(remainingX * remainingX + remainingY * remainingY));
        activeProjectiles.push_back(projectile);
    }
    projectiles_ = std::move(activeProjectiles);
}

void Simulation::Step() {
    Step(nullptr);
}

void Simulation::Step(
    std::map<std::pair<PlayerId, std::uint64_t>,
             CommandResolutionOutcome>* resolutionSink) {
    if (currentTick_ >= kMaximumSupportedTick) {
        return;
    }
    for (std::vector<MaterialDeliveryReceipt>& receipts :
         materialDeliveryReceipts_) {
        receipts.clear();
    }
    for (auto& receipts : repairReceipts_) receipts.clear();
    for (auto& receipts : constructionReceipts_) receipts.clear();
    for (auto& receipts : productionTransitionReceipts_) receipts.clear();
    ResolveExpiredRelaySupply();
    ResolveWaystoneTransitions();
    ResolveBulwarkTransitions();
    ResolveWarformMolts();
    ResolveMineralCovers();
    ResolveChoirIdentities();
    ResolveChoirCoherence();
    ResolveAegisPower();
    UpdateVisibility();
    ProcessCommandsForCurrentTick(resolutionSink);
    ProcessEntityOrders();
    ProcessFutureWellLifecycles();
    UpdateProjectiles();
    ProcessProduction();
    ProcessResearch();
    ApplyPreserveIncome();
    RemoveDestroyedEntities();
    ClearInvalidOrders();
    ++currentTick_;
    PruneCommandResolutionReceipts();
    ResolveExpiredRelaySupply();
    ResolveWaystoneTransitions();
    ResolveBulwarkTransitions();
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

bool Simulation::UpdateVisibility(
    const ReplayCancellationCheck& shouldCancel) {
    std::size_t cancellationWork = 0;
    const auto cancelled = [&]() {
        ++cancellationWork;
        return (cancellationWork & 0xffU) == 0U && shouldCancel &&
            shouldCancel();
    };
    for (PlayerId player = 0; player < visible_.size(); ++player) {
        if (shouldCancel && shouldCancel()) {
            return false;
        }
        std::fill(visible_[player].begin(), visible_[player].end(), 0);
    }
    // FOG information state: Explored is "remembered terrain ... no live unit
    // or temporary terrain state". Cairnback mineral cover is exactly such a
    // temporary state, so a covered tile is remembered as the permanent ground
    // it will revert to rather than as Blocked. Built once per pass in entity
    // id order; first cover on a tile wins, which is deterministic because
    // entities_ is kept sorted by id.
    std::map<std::size_t, Terrain> temporaryCoverGround{};
    for (const Entity& entity : entities_) {
        if (cancelled()) {
            return false;
        }
        if (!entity.temporaryMineralCover || entity.hitPoints <= 0) {
            continue;
        }
        const std::int32_t coverX = entity.position.x.FloorToInt();
        const std::int32_t coverY = entity.position.y.FloorToInt();
        if (coverX < 0 || coverY < 0 || coverX >= config_.mapWidthTiles ||
            coverY >= config_.mapHeightTiles) {
            continue;
        }
        temporaryCoverGround.try_emplace(
            static_cast<std::size_t>(coverY * config_.mapWidthTiles + coverX),
            entity.mineralCoverUnderlyingTerrain);
    }
    const auto permanentTerrainAt = [&](std::size_t tile) {
        const auto covered = temporaryCoverGround.find(tile);
        return covered != temporaryCoverGround.end() ? covered->second
                                                     : terrain_[tile];
    };
    const auto markVisible = [&](PlayerId player, Vec2 position,
                                 std::int32_t radiusTiles) {
        if (player >= players_.size() || !players_[player].active) {
            return true;
        }
        const std::int32_t centerX = position.x.FloorToInt();
        const std::int32_t centerY = position.y.FloorToInt();
        for (std::int32_t offsetY = -radiusTiles; offsetY <= radiusTiles;
            ++offsetY) {
            for (std::int32_t offsetX = -radiusTiles; offsetX <= radiusTiles;
                 ++offsetX) {
                if (cancelled()) {
                    return false;
                }
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
                // Terrain memory snapshots at the moment of sight. Once
                // vision lapses this value is frozen until the tile is seen
                // again, so a change made out of sight cannot repaint it.
                rememberedTerrain_[player][tile] = permanentTerrainAt(tile);
            }
        }
        return true;
    };
    for (const Entity& entity : entities_) {
        if (cancelled()) {
            return false;
        }
        if (entity.owner < players_.size() && players_[entity.owner].active) {
            if (!markVisible(
                    entity.owner, entity.position, entity.visionTiles)) {
                return false;
            }
            if (entity.type == EntityType::FutureWell &&
                entity.wellChoice == FutureWellChoice::Preserve &&
                !IsFutureWellContested(entity)) {
                if (!markVisible(
                        entity.owner,
                        entity.position,
                        config_.rules.futureWell.preserveVisionTiles)) {
                    return false;
                }
            }
        }
    }
    return UpdateRememberedObjects(shouldCancel);
}

bool Simulation::UpdateRememberedObjects(
    const ReplayCancellationCheck& shouldCancel) {
    std::size_t cancellationWork = 0;
    const auto cancelled = [&]() {
        ++cancellationWork;
        return (cancellationWork & 0xffU) == 0U && shouldCancel &&
            shouldCancel();
    };
    const auto tileIndex = [&](Vec2 position) {
        const std::int32_t tileX = std::clamp(
            position.x.FloorToInt(), 0, config_.mapWidthTiles - 1);
        const std::int32_t tileY = std::clamp(
            position.y.FloorToInt(), 0, config_.mapHeightTiles - 1);
        return static_cast<std::size_t>(tileY * config_.mapWidthTiles + tileX);
    };
    for (PlayerId player = 0; player < players_.size(); ++player) {
        if (shouldCancel && shouldCancel()) {
            return false;
        }
        std::vector<RememberedObject>& memory = rememberedObjects_[player];
        if (!players_[player].active) {
            memory.clear();
            continue;
        }
        // A memory is only ever corrected by looking. Standing on the
        // remembered tile and finding the object gone — destroyed, depleted,
        // or uprooted and walked away — clears it. Losing vision never does.
        bool eraseCancelled = false;
        std::erase_if(memory, [&](const RememberedObject& remembered) {
            if (cancelled()) {
                eraseCancelled = true;
                return false;
            }
            const std::size_t tile = tileIndex(remembered.position);
            if (visible_[player][tile] == 0) {
                return false;
            }
            const Entity* live = FindEntity(remembered.id);
            return live == nullptr || !IsRememberablePermanentObject(*live) ||
                   tileIndex(live->position) != tile;
        });
        if (eraseCancelled) {
            return false;
        }
        for (const Entity& entity : entities_) {
            if (cancelled()) {
                return false;
            }
            // A player's own objects are always live in their view; they need
            // no memory and must not be duplicated into one.
            if (entity.owner == player ||
                !IsRememberablePermanentObject(entity) ||
                visible_[player][tileIndex(entity.position)] == 0) {
                continue;
            }
            const RememberedObject observed{entity.id,        entity.owner,
                                            entity.faction,   entity.type,
                                            entity.wellChoice, entity.position,
                                            currentTick_};
            const auto slot = std::lower_bound(
                memory.begin(), memory.end(), entity.id,
                [](const RememberedObject& candidate, EntityId id) {
                    return candidate.id < id;
                });
            if (slot != memory.end() && slot->id == entity.id) {
                *slot = observed;
                continue;
            }
            if (memory.size() >= kMaximumRememberedObjects) {
                // Bounded ledger: the oldest observation fades first, with a
                // stable tie-break by lowest entity id.
                const auto oldest = std::min_element(
                    memory.begin(), memory.end(),
                    [](const RememberedObject& lhs,
                       const RememberedObject& rhs) {
                        return std::tie(lhs.observedTick, lhs.id) <
                               std::tie(rhs.observedTick, rhs.id);
                    });
                if (oldest == memory.end() ||
                    std::tie(oldest->observedTick, oldest->id) >=
                        std::tie(observed.observedTick, observed.id)) {
                    continue;
                }
                memory.erase(oldest);
                const auto reslot = std::lower_bound(
                    memory.begin(), memory.end(), entity.id,
                    [](const RememberedObject& candidate, EntityId id) {
                        return candidate.id < id;
                    });
                memory.insert(reslot, observed);
                continue;
            }
            memory.insert(slot, observed);
        }
    }
    return true;
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

ResourcePool PlayerView::ProductionCost(EntityType unitType) const {
    return ProductionCostFor(config_.rules, player_.faction, unitType);
}

ResourcePool PlayerView::BuildCost(EntityType structureType) const {
    return BuildCostFor(config_.rules, player_.faction, structureType);
}

ProductionStartBlockReason PlayerView::ProductionStartBlockReasonFor(
    EntityId producer,
    EntityType unitType) const {
    // Mirrors Simulation::ProductionStartBlockReasonFor over the scoped copy,
    // in the same order, so the deck and the refusal name the same reason.
    const Entity* building = FindViewEntity(*this, producer);
    if (building == nullptr || building->owner != player_.id ||
        building->hitPoints <= 0) {
        return ProductionStartBlockReason::InvalidProducer;
    }
    if (!building->completed) {
        return ProductionStartBlockReason::ProducerIncomplete;
    }
    const bool supported =
        (building->type == EntityType::CommandCore &&
         unitType == EntityType::Worker) ||
        (building->type == EntityType::Barracks &&
         IsBarracksUnitType(unitType));
    if (!supported) {
        return ProductionStartBlockReason::UnsupportedUnit;
    }
    if (productionRequiresNetworkPower_ &&
        building->faction == Faction::MeridianCompact &&
        building->type == EntityType::Barracks &&
        !building->networkOperational) {
        return ProductionStartBlockReason::Unpowered;
    }
    if (player_.activeResearch != ResearchType::None &&
        player_.researchProducer == producer) {
        return ProductionStartBlockReason::Busy;
    }
    if (building->productionRequired > 0 ||
        !building->productionQueue.empty()) {
        return building->productionQueue.size() >= Entity::kMaxProductionQueue
                   ? ProductionStartBlockReason::QueueFull
                   : ProductionStartBlockReason::Busy;
    }
    const ResourcePool cost = ProductionCost(unitType);
    if (player_.resources.material < cost.material) {
        return ProductionStartBlockReason::InsufficientMatter;
    }
    if (player_.resources.dawnshards < cost.dawnshards) {
        return ProductionStartBlockReason::InsufficientDawn;
    }
    std::int32_t committedPopulation = populationUsed_;
    for (const Entity& entity : entities_) {
        if (entity.owner == player_.id && entity.productionRequired > 0) {
            committedPopulation = SaturatingAdd(
                committedPopulation, entity.productionLogisticsCost);
        }
    }
    if (SaturatingAdd(
            committedPopulation,
            PopulationCostFor(config_.rules, player_.faction, unitType)) >
        populationCapacity_) {
        return ProductionStartBlockReason::LogisticsCapacity;
    }
    if (IsMobileEntityType(unitType) &&
        mobileEntityCount_ + mobileEntityReservations_ >= kMobileEntityLimit) {
        return ProductionStartBlockReason::MobileEntityLimit;
    }
    return ProductionStartBlockReason::None;
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
    view.usesBulwarkCommitmentRules_ = !legacyBulwarkReplaySemantics_;
    view.productionRequiresNetworkPower_ =
        !legacyLinkReplaySemantics_ && !legacyPoweredProductionReplaySemantics_;
    view.firingLanesEnforced_ = !legacyFiringLaneReplaySemantics_;
    view.player_ = *playerState;
    view.decisionSeed_ = config_.randomSeed;
    view.populationUsed_ = PopulationUsed(player);
    view.committedBandSurcharge_ = CommittedBandSurcharge(player);
    view.populationCapacity_ = PopulationCapacity(player);
    view.mobileEntityCount_ = MobileEntityCount(player);
    view.mobileEntityReservations_ = MobileEntityReservations(player);
    view.materialDeliveries_ = materialDeliveryReceipts_[player];
    view.repairReceipts_ = repairReceipts_[player];
    view.constructionReceipts_ = constructionReceipts_[player];
    view.productionTransitions_ = productionTransitionReceipts_[player];
    view.publicFutureWellTelegraphs_ = PublicFutureWellTelegraphs();
    for (const Entity& entity : entities_) {
        if (entity.owner == player && entity.completed && entity.hitPoints > 0 &&
            entity.faction == Faction::MeridianCompact &&
            entity.type == EntityType::ScoutUnit && IsRelayConnected(entity)) {
            view.connectedRelayUnits_.push_back(entity.id);
        }
        if (const std::optional<ProducerQueueState> producerState =
                ProducerQueueStateFor(player, entity.id);
            producerState.has_value()) {
            view.producerQueues_.push_back(*producerState);
        }
    }
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
                // Visible reports the live authorized tile. Explored reports
                // the snapshot taken the last time this player saw it, so a
                // change made out of sight — an enemy Harvest scarring the
                // ground, a mineral cover raised and expired — cannot repaint
                // the map through fog. UpdateVisibility already stored the
                // permanent ground under any temporary cover, so no live
                // entity scan is needed here.
                tile.terrain =
                    visibility == Visibility::Visible
                        ? TerrainAt(tileX, tileY)
                        : rememberedTerrain_[player][static_cast<std::size_t>(
                              tileY * config_.mapWidthTiles + tileX)];
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
                // The visible deposit's stock is public economy information.
                // Hidden deposits never enter this visible-entity projection.
                observed.resourceRemaining =
                    entity.type == EntityType::ResourceNode
                        ? entity.resourceRemaining
                        : 0;
                observed.harvestState = HarvestState::Idle;
                observed.harvestSlotHeld = false;
                observed.harvestTicks = 0;
                observed.harvestQueueTicket = 0;
                observed.assignedResourceNode = 0;
                observed.orderQueue.clear();
                observed.constructionSubProgress = 0;
                observed.constructionProgress = 0;
                observed.constructionRequired = 0;
                observed.order = {};
                observed.reshapeUntilTick = 0;
                observed.reshapeVariant = 0;
                observed.wellCapturePlayer = kNeutralPlayer;
                observed.wellCaptureProgress = 0;
                observed.wellPendingChoice = FutureWellChoice::Dormant;
                observed.wellProtocolTicks = 0;
                observed.productionType = EntityType::Worker;
                observed.productionProgress = 0;
                observed.productionRequired = 0;
                observed.productionInvestedCost = {};
                observed.productionLogisticsCost = 0;
                observed.productionSpawnBlockedTicks = 0;
                observed.productionPausedForSpawn = false;
                observed.productionSpawnBlockedAlert = false;
                observed.rallyRouteAlert = false;
                observed.productionQueue.clear();
                observed.rallyRoute.clear();
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
    // Permanent objects the player saw and no longer sees. An object that is
    // visible right now is already an authoritative entity in the view, so it
    // is not also published as a memory.
    for (const RememberedObject& remembered : rememberedObjects_[player]) {
        if (IsEntityVisibleTo(player, remembered.id)) {
            continue;
        }
        view.rememberedObjects_.push_back(remembered);
    }
    const std::int32_t resolution =
        config_.rules.vibrationDetection.contactResolutionRaw;
    for (const Entity& source : entities_) {
        if (!config_.IsHostile(player, source.owner) ||
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
    std::set<EntityId> wellsAssignedThisBatch;
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
    std::int32_t workerCount = 0;
    std::int32_t soldierCount = 0;
    std::int32_t heavyCount = 0;
    std::int32_t scoutCount = 0;
    std::int32_t utilityCount = 0;
    std::int32_t visibleHeavyThreats = 0;
    std::int32_t visibleMobileThreats = 0;
    std::int32_t committedPopulation = PopulationUsed(player);
    for (const Entity& entity : entities_) {
        // SPEC-AI-001: these counts steer warform adaptation, mineral cover,
        // Choir identity and the Adaptive army composition, and they were named
        // "visible" while counting every hostile entity on the map, alive or
        // dead, seen or unseen. The opponent was reading hidden units to decide
        // what to build. It now sees what a player in its seat would see.
        if (config_.IsHostile(player, entity.owner) && entity.hitPoints > 0 &&
            IsEntityVisibleTo(player, entity.id)) {
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
        if (entity.type == EntityType::Worker) {
            ++workerCount;
        } else if (entity.type == EntityType::Soldier) {
            ++soldierCount;
        } else if (entity.type == EntityType::HeavyUnit) {
            ++heavyCount;
        } else if (entity.type == EntityType::ScoutUnit) {
            ++scoutCount;
        } else if (entity.type == EntityType::UtilityStructure) {
            ++utilityCount;
        }
        if (entity.type == EntityType::CommandCore && entity.completed &&
            (commandCore == nullptr || entity.id < commandCore->id)) {
            commandCore = &entity;
        }
        if (entity.type == EntityType::Barracks) {
            if (entity.completed) {
                ++barracksCount;
                if (entity.productionRequired == 0 &&
                    (researchProducer == 0 || entity.id < researchProducer)) {
                    researchProducer = entity.id;
                }
            }
        } else if (entity.type == EntityType::Dropoff) {
            if (entity.completed) {
                ++dropoffCount;
            }
        }
        if (entity.productionRequired > 0) {
            committedPopulation = SaturatingAdd(
                committedPopulation,
                entity.productionLogisticsCost);
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
    const AiMacroTargets macroTargets = MacroTargetsFor(personality);
    if (barracksCount == 0) {
        expansionType = EntityType::Barracks;
    } else if (capacityHeadroom <= expansionHeadroom &&
               dropoffCount < macroTargets.dropoffCap) {
        // Raise supply whenever the cap is the thing in the way. The old
        // condition required dropoffCount == 0, and a skirmish opponent starts
        // holding one, so capacity was pinned for the whole match.
        expansionType = EntityType::Dropoff;
    } else if (personality == AiPersonality::Defensive && utilityCount < 1) {
        // REL-AI-009: the Warden puts up ONE post early, then fields its heavy
        // line, then fortifies further behind its industry. Taking its whole
        // fortification ceiling up front spent the Dawn the heavy line needs
        // and measured zero heavy units across fifty seeds.
        expansionType = EntityType::UtilityStructure;
    } else if (barracksCount < macroTargets.producerCap &&
               capacityHeadroom > expansionHeadroom) {
        // Industry scales while there is room to use it.
        expansionType = EntityType::Barracks;
    } else if (utilityCount < macroTargets.utilityCap &&
               (heavyCount >= 1 ||
                CompositionFor(personality).heavyWeight == 0)) {
        // Fortification waits until the doctrine's heavy line exists, so posts
        // cannot consume the Dawn those units need. It sits behind industry
        // deliberately: a post costs 30 Dawn, the most of any building, and
        // choosing an unaffordable expansion blocks the whole expansion path
        // for that window rather than falling back to a cheaper one.
        expansionType = EntityType::UtilityStructure;
    }

    EntityId expansionBuilder = 0;
    Vec2 expansionPosition{};
    if (expansionType != EntityType::Worker && commandCore != nullptr &&
        ResourceCovers(playerState->resources,
                       BuildCost(playerState->faction, expansionType))) {
        for (const Entity& candidate : entities_) {
            // A worker holding a Well capture is not a builder candidate
            // either: the capture needs continuous presence, and the
            // lowest-id worker was pulled off the Well to found the next
            // site (SPEC-AI-002/004).
            if (candidate.owner == player && candidate.completed &&
                candidate.hitPoints > 0 && candidate.type == EntityType::Worker &&
                candidate.order.type != OrderType::Build &&
                candidate.order.type != OrderType::FutureWell &&
                (expansionBuilder == 0 || candidate.id < expansionBuilder)) {
                expansionBuilder = candidate.id;
            }
        }
        if (expansionBuilder != 0) {
            const std::int32_t baseX = commandCore->position.x.FloorToInt();
            const std::int32_t baseY = commandCore->position.y.FloorToInt();
            const std::int32_t mapCenterX = config_.mapWidthTiles / 2;
            const std::int32_t mapCenterY = config_.mapHeightTiles / 2;
            const std::int32_t signX = baseX < mapCenterX ? -1 : 1;
            const std::int32_t signY = baseY < mapCenterY ? -1 : 1;
            bool foundPlacement = false;
            for (std::int32_t radius = 4; radius <= 10 && !foundPlacement;
                 ++radius) {
                for (std::int32_t stepY = 0;
                     stepY <= 2 * radius && !foundPlacement;
                     ++stepY) {
                    const std::int32_t offsetY = signY * (radius - stepY);
                    for (std::int32_t stepX = 0;
                         stepX <= 2 * radius;
                         ++stepX) {
                        const std::int32_t offsetX = signX * (radius - stepX);
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
                        // REL-FAC-002.PROD: a Meridian Foundry outside the
                        // network would never produce; site it in reach.
                        if (expansionType == EntityType::Barracks &&
                            playerState->faction == Faction::MeridianCompact &&
                            view.ProductionRequiresNetworkPower() &&
                            !IsViewPositionInMeridianNetwork(view, candidate)) {
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

    // REL-AI-031 and SPEC-AIST-002: send one unit to the edge of what this seat
    // knows. The only non-combat movement the opponent had was a seeded random
    // tile, and with no reconnaissance its march target collapsed to the mirror
    // of its own Core -- a blind rush across a map it had never looked at. That
    // mattered more once the threat census stopped counting unseen units: an
    // opponent that plans only from what it can see has to go and see something.
    //
    // Everything below reads the player view, never the world: an unexplored
    // tile is unexplored because THIS seat has not looked at it.
    EntityId scoutActor = 0;
    Vec2 scoutTarget{};
    bool haveScoutTarget = false;
    {
        const Entity* preferred = nullptr;
        const Entity* idleSoldier = nullptr;
        for (const Entity& candidate : view.Entities()) {
            if (candidate.owner != player || !candidate.completed ||
                candidate.hitPoints <= 0 || candidate.movementPerTickRaw <= 0) {
                continue;
            }
            if (candidate.type == EntityType::ScoutUnit) {
                if (preferred == nullptr || candidate.id < preferred->id) {
                    preferred = &candidate;
                }
            } else if (candidate.type == EntityType::Soldier &&
                       candidate.order.type == OrderType::None) {
                // Only an idle Soldier stands in for a missing scout, so
                // reconnaissance never pulls a unit out of a fight.
                if (idleSoldier == nullptr || candidate.id < idleSoldier->id) {
                    idleSoldier = &candidate;
                }
            }
        }
        const Entity* chosen = preferred != nullptr ? preferred : idleSoldier;
        if (chosen != nullptr) {
            scoutActor = chosen->id;
            // The destination is a tile this seat already knows to be standable
            // and which touches ground it has never seen. Walking to the near
            // side of the unknown reveals it; walking into it may not even be
            // possible, because unexplored ground can be solid.
            std::uint64_t bestDistance =
                std::numeric_limits<std::uint64_t>::max();
            constexpr std::array<std::array<std::int32_t, 2>, 4> directions{{
                {{0, -1}}, {{1, 0}}, {{0, 1}}, {{-1, 0}},
            }};
            for (std::int32_t tileY = 0; tileY < config_.mapHeightTiles; ++tileY) {
                for (std::int32_t tileX = 0; tileX < config_.mapWidthTiles;
                     ++tileX) {
                    const Vec2 tile = Vec2::FromTiles(tileX, tileY);
                    if (view.VisibilityAt(tile) == Visibility::Unexplored ||
                        view.TerrainAt(tileX, tileY) == Terrain::Blocked) {
                        continue;
                    }
                    bool touchesUnknown = false;
                    for (const auto& direction : directions) {
                        const std::int32_t nextX = tileX + direction[0];
                        const std::int32_t nextY = tileY + direction[1];
                        if (nextX < 0 || nextY < 0 ||
                            nextX >= config_.mapWidthTiles ||
                            nextY >= config_.mapHeightTiles) {
                            continue;
                        }
                        if (view.VisibilityAt(Vec2::FromTiles(nextX, nextY)) ==
                            Visibility::Unexplored) {
                            touchesUnknown = true;
                            break;
                        }
                    }
                    if (!touchesUnknown) {
                        continue;
                    }
                    const std::uint64_t distance =
                        DistanceSquaredRaw(chosen->position, tile);
                    if (distance < bestDistance) {
                        bestDistance = distance;
                        scoutTarget = tile;
                        haveScoutTarget = true;
                    }
                }
            }
        }
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
            // Keep one unit on the way behind the active one. Refusing to
            // queue at all left every producer idle for a tick between units,
            // which is most of why the opponent never fielded an army.
            if (actor.productionQueue.size() >= 2) {
                continue;
            }
            command.type = CommandType::Produce;
            if (actor.type == EntityType::CommandCore) {
                if (workerCount >= macroTargets.workerTarget) {
                    continue;
                }
                command.buildType = EntityType::Worker;
                if (ValidateProduction(player, actor.id, command.buildType) ==
                    ProductionResult::Valid) {
                    commands.push_back(command);
                }
                continue;
            }
            // Fill the doctrine's shape rather than producing one unit type
            // for ever. Candidates are tried in order of how far each sits
            // below its authored share, so an unaffordable heavy does not stall
            // a producer that could raise a scout this window.
            const AiComposition mix = AdjustedCompositionFor(
                personality, visibleHeavyThreats, visibleMobileThreats);
            struct CompositionCandidate final {
                EntityType type;
                std::int32_t owned;
                std::int32_t weight;
                bool taken;
            };
            std::array<CompositionCandidate, 3> candidates{{
                {EntityType::Soldier, soldierCount, mix.soldierWeight, false},
                {EntityType::HeavyUnit, heavyCount, mix.heavyWeight, false},
                {EntityType::ScoutUnit, scoutCount, mix.scoutWeight, false},
            }};
            bool produced = false;
            for (std::size_t attempt = 0;
                 attempt < candidates.size() && !produced; ++attempt) {
                CompositionCandidate* choice = nullptr;
                for (CompositionCandidate& candidate : candidates) {
                    if (candidate.taken || candidate.weight <= 0) {
                        continue;
                    }
                    // owned/weight compared by cross-multiplication: exact in
                    // integers and independent of iteration order beyond the
                    // stable enum ordering used to break ties.
                    if (choice == nullptr ||
                        static_cast<std::int64_t>(candidate.owned) *
                                choice->weight <
                            static_cast<std::int64_t>(choice->owned) *
                                candidate.weight) {
                        choice = &candidate;
                    }
                }
                if (choice == nullptr) {
                    break;
                }
                choice->taken = true;
                command.buildType = choice->type;
                if (ValidateProduction(player, actor.id, command.buildType) ==
                    ProductionResult::Valid) {
                    commands.push_back(command);
                    produced = true;
                }
            }
            continue;
        }
        if (actor.type != EntityType::Worker &&
            !IsBarracksUnitType(actor.type)) {
            continue;
        }
        if (actor.type == EntityType::Worker) {
            if (actor.order.type == OrderType::Build) {
                const Entity* targetSite = FindViewEntity(view, actor.order.target);
                if (targetSite != nullptr && !targetSite->completed && targetSite->hitPoints > 0) {
                    continue;
                }
            }
            if (actor.order.type == OrderType::FutureWell) {
                // Leave a worker that is already capturing a Well alone. Capture
                // needs kFutureWellCaptureRequiredTicks of continuous zone
                // presence, and the Gather/Deliver branches below replaced the
                // order about 30 ticks after it took effect, so no AI seat ever
                // captured a Well or earned Dawn (SPEC-AI-002/004). The core
                // clears the order itself when the Well stops being capturable.
                continue;
            }
            const Entity* incompleteSite = nullptr;
            for (const Entity& candidate : entities_) {
                if (candidate.owner == player && !candidate.completed &&
                    candidate.hitPoints > 0 && IsBuildingType(candidate.type)) {
                    incompleteSite = &candidate;
                    break;
                }
            }
            if (incompleteSite != nullptr && actor.order.type != OrderType::Build) {
                bool alreadyBeingBuilt = false;
                for (const Entity& other : entities_) {
                    if (other.owner == player && other.type == EntityType::Worker &&
                        other.order.type == OrderType::Build &&
                        other.order.target == incompleteSite->id) {
                        alreadyBeingBuilt = true;
                        break;
                    }
                }
                if (!alreadyBeingBuilt) {
                    command.type = CommandType::Build;
                    command.target = incompleteSite->id;
                    command.buildType = incompleteSite->type;
                    command.position = incompleteSite->position;
                    commands.push_back(command);
                    continue;
                }
            }
            if (actor.cargo > 0) {
                const EntityId dropoff = FindNearestOwnedDropoff(player, actor.position);
                if (dropoff != 0) {
                    if (actor.order.type == OrderType::Deliver &&
                        actor.order.target == dropoff) {
                        continue;
                    }
                    command.type = CommandType::Deliver;
                    command.target = dropoff;
                    commands.push_back(command);
                    continue;
                }
            }
            const Entity* nearestWell = nullptr;
            std::uint64_t nearestWellDistance = std::numeric_limits<std::uint64_t>::max();
            for (const Entity& candidate : entities_) {
                if (candidate.type != EntityType::FutureWell ||
                    candidate.wellChoice != FutureWellChoice::Dormant ||
                    std::any_of(view.PublicFutureWellTelegraphs().begin(),
                        view.PublicFutureWellTelegraphs().end(),
                        [&](const FutureWellTelegraph& event) {
                            return event.wellId == candidate.id;
                        }) ||
                    !IsEntityVisibleTo(player, candidate.id)) {
                    continue;
                }
                // Skip wells already assigned to a worker in this batch.
                if (wellsAssignedThisBatch.count(candidate.id)) {
                    continue;
                }
                const std::uint64_t distance =
                    DistanceSquaredRaw(actor.position, candidate.position);
                if (distance < nearestWellDistance ||
                    (distance == nearestWellDistance &&
                     (nearestWell == nullptr || candidate.id < nearestWell->id))) {
                    nearestWell = &candidate;
                    nearestWellDistance = distance;
                }
            }
            bool wellAlreadyTargeted = false;
            if (nearestWell != nullptr) {
                for (const Entity& other : entities_) {
                    if (other.owner == player &&
                        other.order.type == OrderType::FutureWell &&
                        other.order.target == nearestWell->id) {
                        wellAlreadyTargeted = true;
                        break;
                    }
                }
            }
            if (nearestWell != nullptr && !wellAlreadyTargeted) {
                command.type = CommandType::FutureWell;
                command.target = nearestWell->id;
                // Choir MUST Preserve wells for sustainable dawnshard income
                // (coherence upkeep requires ongoing dawnshards; Harvest is
                // a one-time lump sum that runs out, collapsing all structures).
                // REL-AI-009 names Preserve as the Warden's protocol, "for
                // sustained advantage". It was taking Harvest: one 500-Dawn
                // lump for the whole match, after which its Dawn sat at zero
                // and it could not afford a single further combat unit, so its
                // authored heavy line never reached the field.
                command.wellChoice = (personality == AiPersonality::Economic ||
                                      personality == AiPersonality::Adaptive ||
                                      personality == AiPersonality::Defensive ||
                                      playerState->faction == Faction::HollowChoir)
                                         ? FutureWellChoice::Preserve
                                     : personality == AiPersonality::Raider
                                         ? FutureWellChoice::Reshape
                                         : FutureWellChoice::Harvest;
                wellsAssignedThisBatch.insert(nearestWell->id);
                commands.push_back(command);
                continue;
            }
            // Leave a worker that is already working a live node alone.
            // Re-ordering it calls BeginGather, which clears the harvest
            // state, and the extraction timer restarts from zero. At the
            // planning cadence that reset always landed before an
            // extraction could finish, so the opponent's Matter income was
            // exactly zero for an entire match and every combat unit it
            // ever fielded came out of its opening resources (REL-AI-011).
            // A worker queued beside a crowded deposit (waiting for its one
            // extraction slot, carrying nothing) may be re-sent to a clearly
            // better one; nothing in progress is lost. Any other gatherer is
            // left alone.
            const Entity* waitingOn = nullptr;
            if (actor.order.type == OrderType::Gather) {
                const Entity* liveTarget = nullptr;
                for (const Entity& candidate : entities_) {
                    if (candidate.id == actor.order.target &&
                        candidate.type == EntityType::ResourceNode &&
                        candidate.resourceRemaining > 0) {
                        liveTarget = &candidate;
                        break;
                    }
                }
                if (liveTarget != nullptr) {
                    const bool waiting =
                        actor.harvestState == HarvestState::Harvesting &&
                        !actor.harvestSlotHeld && actor.cargo == 0;
                    if (!waiting) {
                        continue;
                    }
                    waitingOn = liveTarget;
                }
            }
            // SPEC-DOC-005 / SIM-033: during the Adaptive opening posture the
            // seat holds what it has; no worker walks to a remembered deposit
            // or prospects the frontier until the posture ends. Without this
            // the opponent's prospector found and captured Mission 11's Well
            // and Oruun died before tick 665 (strategy-validation lane,
            // 2026-09-11).
            const bool holdingOpeningPosture =
                personality == AiPersonality::Adaptive && commandCore != nullptr &&
                currentTick_ < kAdaptiveOpeningPostureTicks;
            // A worker already walking, to a remembered deposit or to the
            // frontier (below), is left to arrive; the pass after it arrives
            // sees what it found and gathers or prospects again. Re-ordering
            // a mover every planning pass sent the prospector back to the
            // queue it had just left (probe, 2026-09-11).
            if (actor.order.type == OrderType::Move) {
                continue;
            }
            // REL-AI-031 / REL-FAC-016 / SPEC-RES-003: a deposit serves one
            // extractor at a time, so the nearest node alone is a queue, not
            // an economy. Ten workers on one home deposit realized one
            // worker's income and the opponent starved (balance matrix,
            // 2026-09-11). Choose the least-loaded known deposit, with
            // distance as the price of spreading: each worker already on a
            // node costs six tiles of route, so a home deposit fills to a
            // handful before a farther one is worth the walk. Ties break by
            // id; the count includes assignments made earlier in this pass.
            const Entity* chosenResource = nullptr;
            std::uint64_t chosenScore = std::numeric_limits<std::uint64_t>::max();
            for (const Entity& candidate : entities_) {
                if (candidate.type != EntityType::ResourceNode ||
                    candidate.resourceRemaining <= 0 ||
                    !IsEntityVisibleTo(player, candidate.id)) {
                    continue;
                }
                std::uint64_t load = 0;
                for (const Entity& other : entities_) {
                    if (other.owner == player && other.id != actor.id &&
                        other.type == EntityType::Worker && other.hitPoints > 0 &&
                        ((other.order.type == OrderType::Gather &&
                          other.order.target == candidate.id) ||
                         other.assignedResourceNode == candidate.id)) {
                        ++load;
                    }
                }
                for (const Command& earlier : commands) {
                    if (earlier.type == CommandType::Gather &&
                        earlier.target == candidate.id) {
                        ++load;
                    }
                }
                const std::uint64_t distanceTiles =
                    static_cast<std::uint64_t>(std::sqrt(static_cast<double>(
                        DistanceSquaredRaw(actor.position, candidate.position)))) /
                    static_cast<std::uint64_t>(kFixedScale);
                const std::uint64_t score = load * 6 + distanceTiles;
                if (score < chosenScore ||
                    (score == chosenScore &&
                     (chosenResource == nullptr || candidate.id < chosenResource->id))) {
                    chosenResource = &candidate;
                    chosenScore = score;
                }
            }
            // REL-AI-031 "expand to known resources": a deposit this seat has
            // seen but cannot see now is still worth the walk once the visible
            // ones are crowded. A Gather cannot target fog (exactly as for the
            // player), so the worker is sent to the remembered position and
            // gathers on the pass after it arrives in sight. Four tiles of
            // route stand in for the uncertainty of an unseen stock.
            const RememberedObject* chosenMemory = nullptr;
            for (const RememberedObject& memory : view.RememberedObjects()) {
                if (memory.type != EntityType::ResourceNode || holdingOpeningPosture) {
                    continue;
                }
                bool visibleNow = false;
                for (const Entity& candidate : entities_) {
                    if (candidate.id == memory.id) { visibleNow = true; break; }
                }
                if (visibleNow) {
                    continue;
                }
                std::uint64_t load = 0;
                for (const Entity& other : entities_) {
                    if (other.owner == player && other.id != actor.id &&
                        other.type == EntityType::Worker && other.hitPoints > 0 &&
                        other.order.type == OrderType::Move &&
                        DistanceSquaredRaw(other.order.destination, memory.position) <=
                            static_cast<std::uint64_t>(kFixedScale) * kFixedScale) {
                        ++load;
                    }
                }
                for (const Command& earlier : commands) {
                    if (earlier.type == CommandType::Move &&
                        DistanceSquaredRaw(earlier.position, memory.position) <=
                            static_cast<std::uint64_t>(kFixedScale) * kFixedScale) {
                        ++load;
                    }
                }
                const std::uint64_t distanceTiles =
                    static_cast<std::uint64_t>(std::sqrt(static_cast<double>(
                        DistanceSquaredRaw(actor.position, memory.position)))) /
                    static_cast<std::uint64_t>(kFixedScale);
                const std::uint64_t score = load * 6 + distanceTiles + 4;
                if (score < chosenScore ||
                    (score == chosenScore &&
                     (chosenMemory == nullptr || memory.id < chosenMemory->id))) {
                    chosenMemory = &memory;
                    chosenResource = nullptr;
                    chosenScore = score;
                }
            }
            // REL-AI-031 "expand to known resources" begins with finding
            // them. A worker with nothing better to do than queue prospects:
            // it walks to the known-passable frontier tile nearest the map
            // centre, as the scout does, and what it sees on arrival feeds
            // the next pass. Only from this seat's own map knowledge; one
            // prospector at a time.
            const auto Prospect = [&]() -> bool {
                    if (holdingOpeningPosture) {
                        return false;
                    }
                    // REL-AI-031 "expand to known resources" begins with
                    // finding them. When the only known deposit is crowded
                    // (two or more others already on it) and nothing better
                    // is known, one waiting worker prospects: it walks to the
                    // known-passable frontier tile nearest the map centre, as
                    // the scout does, and what it sees on arrival feeds the
                    // next pass. Only from this seat's own map knowledge; one
                    // prospector at a time.
                    bool prospecting = false;
                    for (const Entity& other : entities_) {
                        if (other.owner != player || other.id == actor.id ||
                            other.type != EntityType::Worker || other.hitPoints <= 0 ||
                            other.order.type != OrderType::Move) {
                            continue;
                        }
                        bool towardDeposit = false;
                        for (const RememberedObject& memory : view.RememberedObjects()) {
                            if (memory.type == EntityType::ResourceNode &&
                                DistanceSquaredRaw(other.order.destination, memory.position) <=
                                    static_cast<std::uint64_t>(kFixedScale) * kFixedScale) {
                                towardDeposit = true;
                                break;
                            }
                        }
                        if (!towardDeposit) { prospecting = true; break; }
                    }
                    for (const Command& earlier : commands) {
                        if (earlier.type == CommandType::Move && earlier.target == 0) {
                            prospecting = true;
                            break;
                        }
                    }
                    if (prospecting) {
                        return false;
                    }
                    // Prospecting rings outward from the seat's own Anchor and
                    // stays within a bounded radius of it: a human scouts the
                    // ground near home first and does not send a Surveyor
                    // across the map into the other side's corridor. The
                    // radius covers the shipping maps' base-side deposits
                    // (about eleven tiles out); farther ground is the scout's
                    // to reveal, after which a remembered deposit is walked to.
                    if (commandCore == nullptr) {
                        return false;
                    }
                    const Vec2 centre = commandCore->position;
                    constexpr std::uint64_t kProspectRadiusRaw =
                        static_cast<std::uint64_t>(16) * kFixedScale;
                    std::uint64_t bestDistance = std::numeric_limits<std::uint64_t>::max();
                    Vec2 frontier{};
                    bool haveFrontier = false;
                    std::uint64_t farDistance = std::numeric_limits<std::uint64_t>::max();
                    Vec2 farFrontier{};
                    bool haveFarFrontier = false;
                    constexpr std::array<std::array<std::int32_t, 2>, 4> steps{{
                        {{0, -1}}, {{1, 0}}, {{0, 1}}, {{-1, 0}},
                    }};
                    for (std::int32_t tileY = 0; tileY < config_.mapHeightTiles; ++tileY) {
                        for (std::int32_t tileX = 0; tileX < config_.mapWidthTiles; ++tileX) {
                            const Vec2 tile = Vec2::FromTiles(tileX, tileY);
                            if (view.VisibilityAt(tile) == Visibility::Unexplored ||
                                view.TerrainAt(tileX, tileY) == Terrain::Blocked) {
                                continue;
                            }
                            bool touchesUnknown = false;
                            for (const auto& step : steps) {
                                const std::int32_t nextX = tileX + step[0];
                                const std::int32_t nextY = tileY + step[1];
                                if (nextX < 0 || nextY < 0 ||
                                    nextX >= config_.mapWidthTiles ||
                                    nextY >= config_.mapHeightTiles) {
                                    continue;
                                }
                                if (view.VisibilityAt(Vec2::FromTiles(nextX, nextY)) ==
                                    Visibility::Unexplored) {
                                    touchesUnknown = true;
                                    break;
                                }
                            }
                            if (!touchesUnknown) {
                                continue;
                            }
                            const std::uint64_t distance = DistanceSquaredRaw(centre, tile);
                            if (distance <= kProspectRadiusRaw * kProspectRadiusRaw) {
                                if (distance < bestDistance) {
                                    bestDistance = distance;
                                    frontier = tile;
                                    haveFrontier = true;
                                }
                            } else if (distance < farDistance) {
                                farDistance = distance;
                                farFrontier = tile;
                                haveFarFrontier = true;
                            }
                        }
                    }
                    // Once the ground within the radius is fully explored and
                    // still holds only the crowded deposit, look farther: the
                    // nearest frontier to home anywhere. Without this a seat
                    // whose home ring was all explored never prospected again,
                    // queued ten workers on one extraction slot and issued no
                    // further orders for the rest of the match (seat 0 on the
                    // harness map, 34 commands in 8,000 ticks, 2026-09-11).
                    if (!haveFrontier && haveFarFrontier) {
                        frontier = farFrontier;
                        haveFrontier = true;
                    }
                    if (!haveFrontier) {
                        return false;
                    }
                    command.type = CommandType::Move;
                    command.target = 0;
                    command.position = frontier;
                    commands.push_back(command);
                    return true;
            };
            if (waitingOn != nullptr) {
                // Keep the queue unless the alternative saves a clear margin:
                // one worker's worth of load (six tiles) beyond the current
                // node's own score, so a crowd does not shuffle every pass.
                std::uint64_t currentLoad = 0;
                for (const Entity& other : entities_) {
                    if (other.owner == player && other.id != actor.id &&
                        other.type == EntityType::Worker && other.hitPoints > 0 &&
                        ((other.order.type == OrderType::Gather &&
                          other.order.target == waitingOn->id) ||
                         other.assignedResourceNode == waitingOn->id)) {
                        ++currentLoad;
                    }
                }
                const std::uint64_t currentDistanceTiles =
                    static_cast<std::uint64_t>(std::sqrt(static_cast<double>(
                        DistanceSquaredRaw(actor.position, waitingOn->position)))) /
                    static_cast<std::uint64_t>(kFixedScale);
                const std::uint64_t currentScore = currentLoad * 6 + currentDistanceTiles;
                const bool noAlternative =
                    chosenResource == waitingOn ||
                    (chosenResource == nullptr && chosenMemory == nullptr) ||
                    chosenScore + 6 >= currentScore;
                if (noAlternative) {
                    if (currentLoad >= 2) {
                        (void)Prospect();
                    }
                    continue;
                }
            }
            if (chosenResource != nullptr) {
                // An idle worker (one that just returned from prospecting)
                // keeps prospecting rather than rejoining a crowded queue.
                std::uint64_t chosenLoad = 0;
                for (const Entity& other : entities_) {
                    if (other.owner == player && other.id != actor.id &&
                        other.type == EntityType::Worker && other.hitPoints > 0 &&
                        ((other.order.type == OrderType::Gather &&
                          other.order.target == chosenResource->id) ||
                         other.assignedResourceNode == chosenResource->id)) {
                        ++chosenLoad;
                    }
                }
                if (chosenLoad >= 2 && chosenMemory == nullptr &&
                    actor.order.type == OrderType::None && Prospect()) {
                    continue;
                }
                command.type = CommandType::Gather;
                command.target = chosenResource->id;
                commands.push_back(command);
                continue;
            }
            if (chosenMemory != nullptr) {
                command.type = CommandType::Move;
                command.target = 0;
                command.position = chosenMemory->position;
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
                    if (!config_.IsHostile(player, candidate.owner) || candidate.hitPoints <= 0 ||
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
            // SPEC-DOC-005 / SIM-033: during the Adaptive opening posture the
            // seat defends what it holds and does not roam. This scan used to
            // accept any visible hostile at any distance and ran before the
            // posture check below, so a gatherer's sight of a foundation
            // twenty tiles away marched the whole force out mid-posture
            // (Mission 04 Reshape, 2026-09-11). While the posture holds, a
            // hostile is a threat only inside the posture's own nine-tile
            // radius around the Core or beside an owned structure.
            const bool adaptiveOpeningPosture =
                personality == AiPersonality::Adaptive &&
                commandCore != nullptr &&
                currentTick_ < kAdaptiveOpeningPostureTicks;
            const std::uint64_t holdDistance =
                static_cast<std::uint64_t>(9 * kFixedScale) * (9 * kFixedScale);
            const auto ThreatensHeldGround = [&](const Entity& hostile) {
                if (!adaptiveOpeningPosture) {
                    return true;
                }
                if (DistanceSquaredRaw(hostile.position, commandCore->position) <=
                    holdDistance) {
                    return true;
                }
                constexpr std::uint64_t kStructureGuardRaw = 3 * kFixedScale;
                for (const Entity& held : entities_) {
                    if (held.owner == player && held.hitPoints > 0 &&
                        IsBuildingType(held.type) &&
                        DistanceSquaredRaw(hostile.position, held.position) <=
                            kStructureGuardRaw * kStructureGuardRaw) {
                        return true;
                    }
                }
                return false;
            };
            const Entity* nearestEnemy = nullptr;
            std::uint64_t nearestDistance = std::numeric_limits<std::uint64_t>::max();
            for (const Entity& candidate : entities_) {
                if (!config_.IsHostile(player, candidate.owner) ||
                    candidate.hitPoints <= 0 ||
                    IsProtectedCommandCore(candidate) ||
                    !IsEntityVisibleTo(player, candidate.id) ||
                    !ThreatensHeldGround(candidate)) {
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
            if (nearestSignature != nullptr && !adaptiveOpeningPosture) {
                command.type = CommandType::AttackMove;
                command.position = nearestSignature->approximatePosition;
                commands.push_back(command);
                continue;
            }
            if (adaptiveOpeningPosture) {
                const std::uint64_t distanceToCore =
                    DistanceSquaredRaw(actor.position, commandCore->position);
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
        if (actor.order.type != OrderType::None &&
            actor.order.type != OrderType::Hold) {
            continue;
        }
        if (actor.id == scoutActor && haveScoutTarget) {
            // Reconnaissance outranks the blind march: a unit that has somewhere
            // unseen to look goes and looks instead of walking at the mirror of
            // its own base.
            command.type = CommandType::Move;
            command.position = scoutTarget;
            commands.push_back(command);
            continue;
        }
        if (IsBarracksUnitType(actor.type)) {
            Vec2 marchTarget{};
            // A structure this seat can see right now is the best march target
            // it has. The march consulted remembered objects only, and a
            // structure stays out of memory for as long as it stays in sight,
            // so an opponent that had just scouted the enemy base still walked
            // at the mirror of its own Core instead of at what it had found.
            for (const Entity& seen : view.Entities()) {
                if (config_.IsHostile(player, seen.owner) &&
                    seen.hitPoints > 0 &&
                    (seen.type == EntityType::CommandCore ||
                     seen.type == EntityType::Barracks ||
                     seen.type == EntityType::Dropoff)) {
                    marchTarget = seen.position;
                    break;
                }
            }
            for (const RememberedObject& obj : view.RememberedObjects()) {
                if (marchTarget != Vec2{}) {
                    break;
                }
                if (config_.IsHostile(player, obj.owner) &&
                    (obj.type == EntityType::CommandCore ||
                     obj.type == EntityType::Barracks ||
                     obj.type == EntityType::Dropoff)) {
                    marchTarget = obj.position;
                    break;
                }
            }
            if (marchTarget == Vec2{} && commandCore != nullptr) {
                marchTarget = Vec2::FromTiles(
                    std::max(1, config_.mapWidthTiles - commandCore->position.x.FloorToInt()),
                    std::max(1, config_.mapHeightTiles - commandCore->position.y.FloorToInt()));
            }
            if (marchTarget != Vec2{}) {
                command.type = CommandType::AttackMove;
                command.position = marchTarget;
                commands.push_back(command);
                continue;
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
void Simulation::WriteSnapshotPayload(Writer& writer, std::uint32_t version) const {
    writer.U8('E');
    writer.U8('B');
    writer.U8('S');
    writer.U8('N');
    writer.U32(version);
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
    if (HasMemorySnapshotSchema(version)) {
        // Schema 25: per-player remembered terrain and remembered permanent
        // objects. Both are authoritative per-player state; a save that
        // dropped them would repaint the loading player's map from live truth.
        for (const auto& remembered : rememberedTerrain_) {
            writer.U32(static_cast<std::uint32_t>(remembered.size()));
            writer.Bytes(std::span<const std::uint8_t>(
                reinterpret_cast<const std::uint8_t*>(remembered.data()),
                remembered.size()));
        }
        for (const auto& memory : rememberedObjects_) {
            writer.U32(static_cast<std::uint32_t>(memory.size()));
            for (const RememberedObject& remembered : memory) {
                writer.U32(remembered.id);
                writer.U8(remembered.owner);
                writer.U8(static_cast<std::uint8_t>(remembered.faction));
                writer.U8(static_cast<std::uint8_t>(remembered.type));
                writer.U8(static_cast<std::uint8_t>(remembered.wellChoice));
                writer.I32(remembered.position.x.Raw());
                writer.I32(remembered.position.y.Raw());
                writer.U64(remembered.observedTick);
            }
        }
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
        WriteCommand(writer, command, version);
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
    if (HasWorkStateSnapshotSchema(version)) {
        // Schema 26 appends transient-but-authoritative work/order state. Keep
        // earlier entity records intact so migrations retain their layout.
        writer.U32(static_cast<std::uint32_t>(entities_.size()));
        for (const Entity& entity : entities_) {
            writer.U32(entity.id);
            writer.U8(static_cast<std::uint8_t>(entity.harvestState));
            writer.U8(entity.harvestSlotHeld ? 1 : 0);
            writer.U64(entity.harvestTicks);
            writer.U32(entity.assignedResourceNode);
            writer.U64(entity.harvestQueueTicket);
            writer.I32(entity.constructionSubProgress);
            writer.U8(static_cast<std::uint8_t>(entity.orderQueue.size()));
            for (const Order& order : entity.orderQueue) {
                writer.U8(static_cast<std::uint8_t>(order.type));
                writer.U32(order.target);
                writer.I32(order.anchor.x.Raw());
                writer.I32(order.anchor.y.Raw());
                writer.I32(order.destination.x.Raw());
                writer.I32(order.destination.y.Raw());
                writer.U8(static_cast<std::uint8_t>(order.buildType));
                writer.U8(static_cast<std::uint8_t>(order.wellChoice));
            }
        }
        // Schema 26 also retains in-flight ballistic state. These records are
        // ordered by monotonic IDs so load cannot change impact order.
        writer.U8(config_.enableBallisticProjectiles ? 1 : 0);
        writer.U32(nextProjectileId_);
        writer.U32(static_cast<std::uint32_t>(projectiles_.size()));
        for (const Projectile& projectile : projectiles_) {
            writer.U32(projectile.id);
            writer.U8(projectile.owner);
            writer.U32(projectile.source);
            writer.U32(projectile.target);
            writer.I32(projectile.position.x.Raw());
            writer.I32(projectile.position.y.Raw());
            writer.I32(projectile.destination.x.Raw());
            writer.I32(projectile.destination.y.Raw());
            writer.I32(projectile.damage);
            writer.I32(projectile.speedRaw);
            writer.I32(projectile.travelDistanceRemainingRaw);
        }
    }
    if (HasFutureWellLifecycleSnapshotSchema(version)) {
        // Schema 27 appends Well lifecycle state after schema-26 records.
        writer.U32(static_cast<std::uint32_t>(entities_.size()));
        for (const Entity& entity : entities_) {
            writer.U32(entity.id);
            writer.U8(entity.wellCapturePlayer);
            writer.U16(entity.wellCaptureProgress);
            writer.U8(static_cast<std::uint8_t>(entity.wellPendingChoice));
            writer.U64(entity.wellProtocolTicks);
        }
    }
    // Schema 28 preserves explicit hostility without changing earlier blocks.
    if (version >= kHostilitySnapshotVersion) {
        for (std::uint8_t mask : config_.hostilityMasks) writer.U8(mask);
    }
    // Schema 29 appends queue-aware manufacturing and rally authority. The
    // legacy active-slot fields above remain intact for schemas 20-28.
    if (HasProductionPipelineSnapshotSchema(version)) {
        writer.U32(static_cast<std::uint32_t>(entities_.size()));
        for (const Entity& entity : entities_) {
            writer.U32(entity.id);
            writer.I32(entity.productionInvestedCost.material);
            writer.I32(entity.productionInvestedCost.dawnshards);
            writer.I32(entity.productionLogisticsCost);
            writer.U64(entity.productionSpawnBlockedTicks);
            writer.U8(entity.productionPausedForSpawn ? 1 : 0);
            writer.U8(entity.productionSpawnBlockedAlert ? 1 : 0);
            writer.U8(entity.rallyRouteAlert ? 1 : 0);
            writer.U8(static_cast<std::uint8_t>(entity.productionQueue.size()));
            for (const ProductionQueueItem& item : entity.productionQueue) {
                writer.U8(static_cast<std::uint8_t>(item.unitType));
                writer.I32(item.configuredCost.material);
                writer.I32(item.configuredCost.dawnshards);
                writer.I32(item.requiredTicks);
                writer.I32(item.logisticsCost);
                writer.I32(item.investedCost.material);
                writer.I32(item.investedCost.dawnshards);
            }
            writer.U8(static_cast<std::uint8_t>(entity.rallyRoute.size()));
            for (const Order& order : entity.rallyRoute) {
                writer.U8(static_cast<std::uint8_t>(order.type));
                writer.U32(order.target);
                writer.I32(order.anchor.x.Raw());
                writer.I32(order.anchor.y.Raw());
                writer.I32(order.destination.x.Raw());
                writer.I32(order.destination.y.Raw());
                writer.U8(static_cast<std::uint8_t>(order.buildType));
                writer.U8(static_cast<std::uint8_t>(order.wellChoice));
            }
        }
    }
    if (HasLinkMechanicsSnapshotSchema(version)) {
        writer.U64(nextProductionItemId_);
        writer.U32(static_cast<std::uint32_t>(entities_.size()));
        for (const Entity& entity : entities_) {
            writer.U32(entity.id);
            writer.U64(entity.activeProductionItemId);
            writer.U8(static_cast<std::uint8_t>(entity.productionQueue.size()));
            for (const ProductionQueueItem& item : entity.productionQueue) {
                writer.U64(item.itemId);
            }
            writer.I32(entity.constructionInvestedCost.material);
            writer.I32(entity.constructionInvestedCost.dawnshards);
            writer.U64(entity.repairInterruptedUntilTick);
            writer.I32(entity.repairRateRemainder);
            writer.I32(entity.repairPaidHitPointCredit);
            writer.U8(entity.networkOperational ? 1 : 0);
        }
    }
    if (version >= kBulwarkCommitmentSnapshotVersion) {
        writer.U32(static_cast<std::uint32_t>(entities_.size()));
        for (const Entity& entity : entities_) {
            writer.U32(entity.id);
            writer.U8(static_cast<std::uint8_t>(entity.deploymentPhase));
            writer.U64(entity.deploymentTransitionUntilTick);
        }
    }
}

std::vector<std::uint8_t> Simulation::SaveSnapshot(
    std::uint64_t* snapshotStateChecksum) const {
    if (legacyLinkReplaySemantics_) {
        // Schema 30 stores current network derivatives. Keep the live historical
        // execution and its replay checksum unchanged; migrate only the save copy.
        Simulation checkpoint = *this;
        checkpoint.legacyLinkReplaySemantics_ = false;
        checkpoint.ResolveAegisPower();
        return checkpoint.SaveSnapshot(snapshotStateChecksum);
    }
    if (snapshotStateChecksum != nullptr) {
        *snapshotStateChecksum = StateChecksum();
    }
    BinaryWriter writer{};
    std::size_t rememberedObjectCount = 0;
    for (const auto& memory : rememberedObjects_) {
        rememberedObjectCount += memory.size();
    }
    writer.Reserve(
        1536U +
        terrain_.size() *
            (1U + explored_.size() + rememberedTerrain_.size()) +
        rememberedObjectCount * kSerializedRememberedObjectBytes +
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
    std::string* error,
    PlayerHostilityMasks legacyHostilityMasks,
    const ReplayCancellationCheck& shouldCancel) {
    if (error != nullptr) {
        error->clear();
    }
    const auto IsCancelled = [&]() {
        if (shouldCancel && shouldCancel()) {
            SetError(error, "snapshot load cancelled");
            return true;
        }
        return false;
    };
    std::size_t cancellationWork = 0;
    const auto IsCancelledAtInterval = [&]() {
        ++cancellationWork;
        return (cancellationWork & 0xffU) == 0U && IsCancelled();
    };
    if (IsCancelled()) {
        return std::nullopt;
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
    const std::optional<std::uint64_t> actualIntegrity =
        Fnv1a(payload, shouldCancel);
    if (!actualIntegrity.has_value()) {
        SetError(error, "snapshot load cancelled");
        return std::nullopt;
    }
    if (*actualIntegrity != expectedIntegrity) {
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
        version != kLinkMechanicsSnapshotVersion &&
        version != kProductionPipelineSnapshotVersion &&
        version != kHostilitySnapshotVersion &&
        version != kFutureWellLifecycleSnapshotVersion &&
        version != kWorkStateSnapshotVersion &&
        version != kMemorySnapshotVersion &&
        version != kCommandResolutionReceiptSnapshotVersion &&
        version != kProtectedCommandCoreSnapshotVersion &&
        version != kChoirSnapshotVersion &&
        version != kPriorSnapshotVersion && version != kLegacySnapshotVersion) {
        SetError(error, "snapshot version is unsupported");
        return std::nullopt;
    }
    if (version < kHostilitySnapshotVersion) {
        config.hostilityMasks = legacyHostilityMasks;
        if (!config.HasValidHostilityMasks()) {
            SetError(error, "legacy snapshot hostility migration is invalid");
            return std::nullopt;
        }
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
        static_cast<std::size_t>(tileCount) *
            (1U + kMaximumPlayers +
             (HasMemorySnapshotSchema(version) ? kMaximumPlayers : 0U));
    if (reader.Remaining() < minimumRemaining) {
        SetError(error, "snapshot payload is too short for its declared map");
        return std::nullopt;
    }

    if (IsCancelled()) {
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
        if (IsCancelledAtInterval()) {
            return std::nullopt;
        }
        std::uint8_t encoded = 0;
        if (!reader.U8(encoded) ||
            encoded > static_cast<std::uint8_t>(Terrain::Scarred)) {
            SetError(error, "snapshot terrain contains an invalid value");
            return std::nullopt;
        }
        terrain = static_cast<Terrain>(encoded);
    }
    for (auto& explored : simulation.explored_) {
        if (IsCancelled()) {
            return std::nullopt;
        }
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
    if (HasMemorySnapshotSchema(version)) {
        for (auto& remembered : simulation.rememberedTerrain_) {
            if (IsCancelled()) {
                return std::nullopt;
            }
            std::uint32_t count = 0;
            if (!reader.U32(count) || count != serializedTileCount) {
                SetError(error,
                         "snapshot terrain memory dimensions do not match the map");
                return std::nullopt;
            }
            for (Terrain& terrain : remembered) {
                if (IsCancelledAtInterval()) {
                    return std::nullopt;
                }
                std::uint8_t encoded = 0;
                if (!reader.U8(encoded) ||
                    encoded > static_cast<std::uint8_t>(Terrain::Scarred)) {
                    SetError(error,
                             "snapshot terrain memory contains an invalid value");
                    return std::nullopt;
                }
                terrain = static_cast<Terrain>(encoded);
            }
        }
        for (PlayerId index = 0;
             index < simulation.rememberedObjects_.size(); ++index) {
            std::vector<RememberedObject>& memory =
                simulation.rememberedObjects_[index];
            std::uint32_t count = 0;
            if (!reader.U32(count) || count > kMaximumRememberedObjects ||
                static_cast<std::size_t>(count) >
                    reader.Remaining() / kSerializedRememberedObjectBytes) {
                SetError(error, "snapshot object memory count is invalid");
                return std::nullopt;
            }
            memory.clear();
            if (IsCancelled()) {
                return std::nullopt;
            }
            memory.reserve(count);
            EntityId priorRememberedId = 0;
            for (std::uint32_t entry = 0; entry < count; ++entry) {
                if (IsCancelledAtInterval()) {
                    return std::nullopt;
                }
                RememberedObject remembered{};
                std::uint8_t faction = 0;
                std::uint8_t type = 0;
                std::uint8_t wellChoice = 0;
                std::int32_t rawX = 0;
                std::int32_t rawY = 0;
                if (!reader.U32(remembered.id) || !reader.U8(remembered.owner) ||
                    !reader.U8(faction) || !reader.U8(type) ||
                    !reader.U8(wellChoice) || !reader.I32(rawX) ||
                    !reader.I32(rawY) || !reader.U64(remembered.observedTick)) {
                    SetError(error, "snapshot object memory is truncated");
                    return std::nullopt;
                }
                remembered.faction = static_cast<Faction>(faction);
                remembered.type = static_cast<EntityType>(type);
                remembered.wellChoice =
                    static_cast<FutureWellChoice>(wellChoice);
                remembered.position = Vec2::FromRaw(rawX, rawY);
                // Memory is player-scoped authority, so it is validated as
                // strictly as any other loaded state: ascending unique ids,
                // in-map positions, real classes, and an observation that
                // cannot come from the future.
                if (remembered.id == 0 || remembered.id <= priorRememberedId ||
                    remembered.id >= simulation.nextEntityId_ ||
                    (remembered.owner != kNeutralPlayer &&
                     remembered.owner >= simulation.players_.size()) ||
                    remembered.owner == index ||
                    !IsValidFaction(remembered.faction) ||
                    !IsValidEntityType(remembered.type) ||
                    wellChoice >
                        static_cast<std::uint8_t>(FutureWellChoice::Reshape) ||
                    !simulation.IsInsideMap(remembered.position) ||
                    remembered.observedTick > simulation.currentTick_) {
                    SetError(error, "snapshot object memory is invalid");
                    return std::nullopt;
                }
                priorRememberedId = remembered.id;
                memory.push_back(remembered);
            }
        }
    } else {
        // Schemas 20 through 24 recorded no memory. Their explored ground is
        // reconstructed from the live map — the same information those saves
        // already served through the view — and object memory starts empty
        // rather than inventing sightings the player never had.
        for (PlayerId index = 0;
             index < simulation.rememberedTerrain_.size(); ++index) {
            for (std::size_t tile = 0;
                 tile < simulation.rememberedTerrain_[index].size(); ++tile) {
                if (IsCancelledAtInterval()) {
                    return std::nullopt;
                }
                simulation.rememberedTerrain_[index][tile] =
                    simulation.explored_[index][tile] != 0
                        ? simulation.terrain_[tile]
                        : Terrain::Blocked;
            }
            simulation.rememberedObjects_[index].clear();
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
    if (IsCancelled()) {
        return std::nullopt;
    }
    simulation.entities_.reserve(entityCount);
    EntityId priorId = 0;
    for (std::uint32_t index = 0; index < entityCount; ++index) {
        if (IsCancelledAtInterval()) {
            return std::nullopt;
        }
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
            orderType > static_cast<std::uint8_t>(
                HasLinkMechanicsSnapshotSchema(version)
                    ? OrderType::Repair
                    : OrderType::Patrol) ||
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
        simulation.MarkStructureOccupancyDirty();
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
        simulation.MarkStructureOccupancyDirty();
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
        if (IsCancelledAtInterval()) {
            return std::nullopt;
        }
        if (!HasLinkMechanicsSnapshotSchema(version) &&
            entity.aegisPowered !=
            simulation.IsAegisNetworkPowered(entity)) {
            SetError(error, "snapshot Aegis power state is invalid");
            return std::nullopt;
        }
    }
    std::map<std::pair<std::int32_t, std::int32_t>, EntityId> mineralCoverTiles;
    for (const Entity& entity : simulation.entities_) {
        if (IsCancelledAtInterval()) {
            return std::nullopt;
        }
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
        if (IsCancelledAtInterval()) {
            return std::nullopt;
        }
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
        if (IsCancelledAtInterval()) {
            return std::nullopt;
        }
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
    const std::size_t serializedCommandBytes =
        HasProductionPipelineSnapshotSchema(version)
            ? kSerializedCommandBytes
            : kLegacySerializedCommandBytes;
    if (!reader.U32(commandCount) || commandCount > kMaximumSerializedCommands ||
        static_cast<std::size_t>(commandCount) >
            reader.Remaining() / serializedCommandBytes) {
        SetError(error, "snapshot command count is invalid");
        return std::nullopt;
    }
    if (IsCancelled()) {
        return std::nullopt;
    }
    simulation.pendingCommands_.resize(commandCount);
    for (Command& command : simulation.pendingCommands_) {
        if (IsCancelledAtInterval()) {
            return std::nullopt;
        }
        if (!ReadCommand(reader, command, version) ||
            command.player >= simulation.players_.size() ||
            !simulation.players_[command.player].active ||
            command.executeTick < simulation.currentTick_ ||
            command.executeTick > kMaximumSupportedTick || command.actor == 0 ||
            (simulation.config_.rules.version < 2 &&
             ((command.type == CommandType::ReconcileToManifest ||
               command.type == CommandType::ReconcileToPossible) ||
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
        if (IsCancelledAtInterval()) {
            return std::nullopt;
        }
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
    if (HasCommandResolutionReceiptSnapshotSchema(version)) {
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
            if (IsCancelledAtInterval()) {
                return std::nullopt;
            }
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
                (!HasLinkMechanicsSnapshotSchema(version) &&
                 stored.receipt.commandType > CommandType::SetRallyRoute) ||
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
                (IsMovementRejectionOutcome(stored.receipt.outcome) &&
                 stored.receipt.commandType != CommandType::Move) ||
                (simulation.config_.rules.version < 2 &&
                 (stored.receipt.commandType ==
                      CommandType::ReconcileToManifest ||
                  stored.receipt.commandType ==
                      CommandType::ReconcileToPossible))) {
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
    if (HasWorkStateSnapshotSchema(version)) {
        std::uint32_t workCount = 0;
        if (!reader.U32(workCount) || workCount != simulation.entities_.size()) {
            SetError(error, "snapshot work-state count is invalid");
            return std::nullopt;
        }
        std::map<EntityId, std::size_t> heldSlots;
        for (Entity& entity : simulation.entities_) {
            if (IsCancelledAtInterval()) {
                return std::nullopt;
            }
            EntityId id = 0;
            std::uint8_t state = 0, held = 0, queueCount = 0;
            if (!reader.U32(id) || !reader.U8(state) || !reader.U8(held) ||
                !reader.U64(entity.harvestTicks) || !reader.U32(entity.assignedResourceNode) ||
                !reader.U64(entity.harvestQueueTicket) ||
                !reader.I32(entity.constructionSubProgress) || !reader.U8(queueCount)) {
                SetError(error, "snapshot work state is truncated");
                return std::nullopt;
            }
            entity.harvestState = static_cast<HarvestState>(state);
            entity.harvestSlotHeld = held != 0;
            const bool gathering = entity.order.type == OrderType::Gather;
            const bool delivering = entity.order.type == OrderType::Deliver;
            if (id != entity.id || state > static_cast<std::uint8_t>(HarvestState::Delivering) ||
                held > 1 || entity.harvestTicks > simulation.currentTick_ ||
                entity.harvestQueueTicket > simulation.currentTick_ + 1 ||
                entity.constructionSubProgress < 0 || entity.constructionSubProgress >= 100 ||
                queueCount > Entity::kMaxQueuedOrders ||
                (state != 0 && (entity.type != EntityType::Worker || (!gathering && !delivering))) ||
                (gathering && entity.assignedResourceNode != entity.order.target) ||
                (entity.harvestQueueTicket != 0 &&
                 (!gathering || entity.harvestState != HarvestState::Harvesting)) ||
                (held != 0 && (entity.harvestQueueTicket == 0 ||
                              ++heldSlots[entity.assignedResourceNode] > 1)) ||
                (state == 0 && (held != 0 || entity.harvestTicks != 0 ||
                              entity.harvestQueueTicket != 0 || entity.assignedResourceNode != 0))) {
                SetError(error, "snapshot work state is invalid");
                return std::nullopt;
            }
            for (std::uint8_t i = 0; i < queueCount; ++i) {
                Order order{};
                std::uint8_t type = 0, build = 0, well = 0;
                std::int32_t ax = 0, ay = 0, dx = 0, dy = 0;
                if (!reader.U8(type) || !reader.U32(order.target) ||
                    !reader.I32(ax) || !reader.I32(ay) || !reader.I32(dx) || !reader.I32(dy) ||
                    !reader.U8(build) || !reader.U8(well)) {
                    SetError(error, "snapshot queued order is truncated");
                    return std::nullopt;
                }
                order.type = static_cast<OrderType>(type);
                order.buildType = static_cast<EntityType>(build);
                order.wellChoice = static_cast<FutureWellChoice>(well);
                order.anchor = Vec2::FromRaw(ax, ay);
                order.destination = Vec2::FromRaw(dx, dy);
                if (type == 0 || type > static_cast<std::uint8_t>(
                        HasLinkMechanicsSnapshotSchema(version)
                            ? OrderType::Repair
                            : OrderType::Patrol) ||
                    !IsValidEntityType(order.buildType) ||
                    well > static_cast<std::uint8_t>(FutureWellChoice::Reshape) ||
                    !simulation.IsInsideMap(order.anchor) || !simulation.IsInsideMap(order.destination)) {
                    SetError(error, "snapshot queued order is invalid");
                    return std::nullopt;
                }
                entity.orderQueue.push_back(order);
            }
        }

        std::uint8_t ballisticProjectiles = 0;
        std::uint32_t projectileCount = 0;
        if (!reader.U8(ballisticProjectiles) ||
            !reader.U32(simulation.nextProjectileId_) ||
            !reader.U32(projectileCount) || ballisticProjectiles > 1 ||
            simulation.nextProjectileId_ == 0 ||
            (ballisticProjectiles == 0 && projectileCount != 0) ||
            projectileCount > kMaximumSerializedProjectiles ||
            static_cast<std::size_t>(projectileCount) >
                reader.Remaining() / kSerializedProjectileBytes) {
            SetError(error, "snapshot projectile header is invalid");
            return std::nullopt;
        }
        simulation.config_.enableBallisticProjectiles =
            ballisticProjectiles != 0;
        simulation.projectiles_.clear();
        if (IsCancelled()) {
            return std::nullopt;
        }
        simulation.projectiles_.reserve(projectileCount);
        EntityId priorProjectileId = 0;
        for (std::uint32_t index = 0; index < projectileCount; ++index) {
            if (IsCancelledAtInterval()) {
                return std::nullopt;
            }
            Projectile projectile{};
            std::int32_t positionX = 0;
            std::int32_t positionY = 0;
            std::int32_t destinationX = 0;
            std::int32_t destinationY = 0;
            if (!reader.U32(projectile.id) || !reader.U8(projectile.owner) ||
                !reader.U32(projectile.source) || !reader.U32(projectile.target) ||
                !reader.I32(positionX) || !reader.I32(positionY) ||
                !reader.I32(destinationX) || !reader.I32(destinationY) ||
                !reader.I32(projectile.damage) || !reader.I32(projectile.speedRaw) ||
                !reader.I32(projectile.travelDistanceRemainingRaw)) {
                SetError(error, "snapshot projectile data is truncated");
                return std::nullopt;
            }
            projectile.position = Vec2::FromRaw(positionX, positionY);
            projectile.destination = Vec2::FromRaw(destinationX, destinationY);
            std::int64_t maximumTravelRaw =
                std::numeric_limits<std::int32_t>::max();
            if (simulation.config_.mapWidthTiles <=
                    std::numeric_limits<std::int32_t>::max() / kFixedScale &&
                simulation.config_.mapHeightTiles <=
                    std::numeric_limits<std::int32_t>::max() / kFixedScale) {
                const std::int64_t mapWidthRaw =
                    static_cast<std::int64_t>(simulation.config_.mapWidthTiles) *
                    kFixedScale;
                const std::int64_t mapHeightRaw =
                    static_cast<std::int64_t>(simulation.config_.mapHeightTiles) *
                    kFixedScale;
                maximumTravelRaw = IntegerSqrt64(
                    mapWidthRaw * mapWidthRaw + mapHeightRaw * mapHeightRaw);
            }
            if (projectile.id == 0 || projectile.id <= priorProjectileId ||
                projectile.id >= simulation.nextProjectileId_ ||
                projectile.owner == kNeutralPlayer ||
                projectile.owner >= simulation.players_.size() ||
                !simulation.players_[projectile.owner].active ||
                projectile.source == 0 || projectile.target == 0 ||
                projectile.source >= simulation.nextEntityId_ ||
                projectile.target >= simulation.nextEntityId_ ||
                !simulation.IsInsideMap(projectile.position) ||
                !simulation.IsInsideMap(projectile.destination) ||
                projectile.damage <= 0 || projectile.speedRaw <= 0 ||
                projectile.speedRaw > kMaximumBallisticProjectileSpeedRaw ||
                projectile.travelDistanceRemainingRaw < 0 ||
                static_cast<std::int64_t>(
                    projectile.travelDistanceRemainingRaw) > maximumTravelRaw) {
                SetError(error, "snapshot projectile state is invalid");
                return std::nullopt;
            }
            priorProjectileId = projectile.id;
            simulation.projectiles_.push_back(projectile);
        }
        if (HasFutureWellLifecycleSnapshotSchema(version)) {
            std::uint32_t lifecycleCount = 0;
            if (!reader.U32(lifecycleCount) ||
                lifecycleCount != simulation.entities_.size() ||
                static_cast<std::size_t>(lifecycleCount) >
                    reader.Remaining() / kSerializedFutureWellLifecycleBytes) {
                SetError(error, "snapshot Future Well lifecycle count is invalid");
                return std::nullopt;
            }
            for (Entity& entity : simulation.entities_) {
                if (IsCancelledAtInterval()) {
                    return std::nullopt;
                }
                EntityId id = 0;
                std::uint8_t capturePlayer = kNeutralPlayer;
                std::uint8_t pendingChoice = 0;
                std::uint16_t progress = 0;
                Tick protocolTicks = 0;
                if (!reader.U32(id) || !reader.U8(capturePlayer) ||
                    !reader.U16(progress) || !reader.U8(pendingChoice) ||
                    !reader.U64(protocolTicks)) {
                    SetError(error, "snapshot Future Well lifecycle is truncated");
                    return std::nullopt;
                }
                const bool capturePlayerValid =
                    capturePlayer == kNeutralPlayer ||
                    (capturePlayer < simulation.players_.size() &&
                     simulation.players_[capturePlayer].active);
                const bool futureWell = entity.type == EntityType::FutureWell;
                const bool inactiveCapture = capturePlayer == kNeutralPlayer &&
                    progress == 0 &&
                    pendingChoice == static_cast<std::uint8_t>(FutureWellChoice::Dormant);
                const bool inactiveLifecycle = inactiveCapture && protocolTicks == 0;
                // A newly assigned capture may still have zero progress when
                // hostile presence prevents its first uncontested tick.
                const bool activeCaptureValid =
                    capturePlayer != kNeutralPlayer &&
                    pendingChoice > static_cast<std::uint8_t>(FutureWellChoice::Dormant) &&
                    pendingChoice <= static_cast<std::uint8_t>(FutureWellChoice::Reshape) &&
                    progress < kFutureWellCaptureRequiredTicks &&
                    (entity.wellChoice == FutureWellChoice::Dormant ||
                     (entity.wellChoice == FutureWellChoice::Preserve &&
                      entity.owner != capturePlayer)) && protocolTicks == 0;
                const bool harvestStateValid =
                    entity.wellChoice != FutureWellChoice::Harvest ||
                    (entity.owner != kNeutralPlayer &&
                     (protocolTicks == 0 || protocolTicks <= kHarvestTelegraphTicks));
                const bool nonHarvestProtocolValid =
                    entity.wellChoice == FutureWellChoice::Harvest ||
                    (entity.wellChoice == FutureWellChoice::Dormant &&
                     pendingChoice == static_cast<std::uint8_t>(FutureWellChoice::Reshape)) ||
                    protocolTicks == 0;
                // Committing Harvest clears capture state while retaining its
                // public countdown. Save/load must preserve that valid phase.
                const bool activeHarvestProtocolValid = inactiveCapture &&
                    entity.wellChoice == FutureWellChoice::Harvest &&
                    protocolTicks > 0 && protocolTicks <= kHarvestTelegraphTicks;
                const bool activeReshapeProtocolValid =
                    entity.wellChoice == FutureWellChoice::Dormant &&
                    entity.owner != kNeutralPlayer &&
                    entity.wellActivationTick == 0 && entity.reshapeUntilTick == 0 &&
                    entity.reshapeVariant == 0 &&
                    capturePlayer == kNeutralPlayer && progress == 0 &&
                    pendingChoice == static_cast<std::uint8_t>(FutureWellChoice::Reshape) &&
                    protocolTicks > 0 && protocolTicks <= kReshapeTelegraphTicks;
                if (id != entity.id || !capturePlayerValid ||
                    pendingChoice > static_cast<std::uint8_t>(FutureWellChoice::Reshape) ||
                    (!futureWell && !inactiveLifecycle) ||
                    (futureWell && !(inactiveLifecycle || activeCaptureValid ||
                                     activeHarvestProtocolValid || activeReshapeProtocolValid)) ||
                    !harvestStateValid || !nonHarvestProtocolValid) {
                    SetError(error, "snapshot Future Well lifecycle is invalid");
                    return std::nullopt;
                }
                entity.wellCapturePlayer = capturePlayer;
                entity.wellCaptureProgress = progress;
                entity.wellPendingChoice =
                    static_cast<FutureWellChoice>(pendingChoice);
                entity.wellProtocolTicks = protocolTicks;
            }
        }
    } else {
        // Schemas 20-25 omitted progress, assignment on return, and queued
        // orders. Restart a known Gather; deliver legacy cargo then idle when
        // its source cannot be recovered. Never guess an original deposit.
        for (Entity& entity : simulation.entities_) {
            if (IsCancelledAtInterval()) {
                return std::nullopt;
            }
            if (entity.type == EntityType::Worker && entity.order.type == OrderType::Gather) {
                simulation.BeginGather(entity, entity.order.target);
            } else if (entity.type == EntityType::Worker && entity.order.type == OrderType::Deliver) {
                entity.harvestState = HarvestState::ReturningHome;
            }
        }
    }
    if (version >= 28) {
        for (std::uint8_t& mask : simulation.config_.hostilityMasks) {
            if (!reader.U8(mask)) {
                SetError(error, "snapshot hostility masks are truncated");
                return std::nullopt;
            }
        }
        if (!simulation.config_.HasValidHostilityMasks()) {
            SetError(error, "snapshot hostility masks are invalid");
            return std::nullopt;
        }
    }
    if (HasProductionPipelineSnapshotSchema(version)) {
        std::uint32_t producerStateCount = 0;
        if (!reader.U32(producerStateCount) ||
            producerStateCount != simulation.entities_.size()) {
            SetError(error, "snapshot production state count is invalid");
            return std::nullopt;
        }
        for (Entity& entity : simulation.entities_) {
            if (IsCancelledAtInterval()) {
                return std::nullopt;
            }
            std::uint32_t id = 0;
            std::uint8_t paused = 0;
            std::uint8_t spawnAlert = 0;
            std::uint8_t rallyAlert = 0;
            std::uint8_t waitingCount = 0;
            if (!reader.U32(id) ||
                !reader.I32(entity.productionInvestedCost.material) ||
                !reader.I32(entity.productionInvestedCost.dawnshards) ||
                !reader.I32(entity.productionLogisticsCost) ||
                !reader.U64(entity.productionSpawnBlockedTicks) ||
                !reader.U8(paused) || !reader.U8(spawnAlert) ||
                !reader.U8(rallyAlert) || !reader.U8(waitingCount) ||
                id != entity.id || paused > 1 || spawnAlert > 1 ||
                rallyAlert > 1 ||
                waitingCount > Entity::kMaxProductionQueue) {
                SetError(error, "snapshot production state is invalid");
                return std::nullopt;
            }
            entity.productionPausedForSpawn = paused != 0;
            entity.productionSpawnBlockedAlert = spawnAlert != 0;
            entity.rallyRouteAlert = rallyAlert != 0;
            entity.productionQueue.clear();
            entity.productionQueue.reserve(waitingCount);
            for (std::uint8_t index = 0; index < waitingCount; ++index) {
                ProductionQueueItem item{};
                std::uint8_t unitType = 0;
                if (!reader.U8(unitType) ||
                    !reader.I32(item.configuredCost.material) ||
                    !reader.I32(item.configuredCost.dawnshards) ||
                    !reader.I32(item.requiredTicks) ||
                    !reader.I32(item.logisticsCost) ||
                    !reader.I32(item.investedCost.material) ||
                    !reader.I32(item.investedCost.dawnshards) ||
                    unitType > static_cast<std::uint8_t>(EntityType::UtilityStructure)) {
                    SetError(error, "snapshot production queue is truncated or invalid");
                    return std::nullopt;
                }
                item.unitType = static_cast<EntityType>(unitType);
                entity.productionQueue.push_back(item);
            }
            std::uint8_t rallyCount = 0;
            if (!reader.U8(rallyCount) ||
                rallyCount > Entity::kMaxRallyOrders) {
                SetError(error, "snapshot rally route count is invalid");
                return std::nullopt;
            }
            entity.rallyRoute.clear();
            entity.rallyRoute.reserve(rallyCount);
            for (std::uint8_t index = 0; index < rallyCount; ++index) {
                Order order{};
                std::uint8_t orderType = 0;
                std::int32_t anchorX = 0;
                std::int32_t anchorY = 0;
                std::int32_t destinationX = 0;
                std::int32_t destinationY = 0;
                std::uint8_t buildType = 0;
                std::uint8_t wellChoice = 0;
                if (!reader.U8(orderType) || !reader.U32(order.target) ||
                    !reader.I32(anchorX) || !reader.I32(anchorY) ||
                    !reader.I32(destinationX) || !reader.I32(destinationY) ||
                    !reader.U8(buildType) || !reader.U8(wellChoice) ||
                    orderType > static_cast<std::uint8_t>(OrderType::Patrol) ||
                    buildType > static_cast<std::uint8_t>(EntityType::UtilityStructure) ||
                    wellChoice > static_cast<std::uint8_t>(FutureWellChoice::Reshape)) {
                    SetError(error, "snapshot rally route is truncated or invalid");
                    return std::nullopt;
                }
                order.type = static_cast<OrderType>(orderType);
                order.anchor = Vec2::FromRaw(anchorX, anchorY);
                order.destination = Vec2::FromRaw(destinationX, destinationY);
                order.buildType = static_cast<EntityType>(buildType);
                order.wellChoice = static_cast<FutureWellChoice>(wellChoice);
                entity.rallyRoute.push_back(order);
            }

            const bool producer = entity.type == EntityType::CommandCore ||
                entity.type == EntityType::Barracks;
            const auto Supports = [&entity](EntityType type) {
                return (entity.type == EntityType::CommandCore &&
                        type == EntityType::Worker) ||
                    (entity.type == EntityType::Barracks &&
                     IsBarracksUnitType(type));
            };
            const bool active = entity.productionRequired > 0;
            bool queueValid = true;
            for (const ProductionQueueItem& item : entity.productionQueue) {
                queueValid = queueValid && Supports(item.unitType) &&
                    item.configuredCost.material >= 0 &&
                    item.configuredCost.dawnshards >= 0 &&
                    item.investedCost == ResourcePool{} &&
                    item.requiredTicks > 0 &&
                    item.requiredTicks <= kMaximumProductionTicks &&
                    item.logisticsCost > 0;
            }
            bool rallyValid = true;
            for (const Order& order : entity.rallyRoute) {
                rallyValid = rallyValid &&
                    (order.type == OrderType::Move ||
                     order.type == OrderType::Guard ||
                     order.type == OrderType::Gather) &&
                    simulation.IsInsideMap(order.destination);
            }
            const bool activeStateValid = active
                ? Supports(entity.productionType) &&
                    entity.productionInvestedCost.material >= 0 &&
                    entity.productionInvestedCost.dawnshards >= 0 &&
                    entity.productionLogisticsCost > 0 &&
                    entity.productionSpawnBlockedTicks <= 100 &&
                    (!entity.productionPausedForSpawn ||
                     (entity.productionSpawnBlockedTicks == 100 &&
                      entity.productionSpawnBlockedAlert)) &&
                    (!entity.productionSpawnBlockedAlert ||
                     entity.productionSpawnBlockedTicks == 100)
                : entity.productionInvestedCost.material == 0 &&
                    entity.productionInvestedCost.dawnshards == 0 &&
                    entity.productionLogisticsCost == 0 &&
                    entity.productionSpawnBlockedTicks == 0 &&
                    !entity.productionPausedForSpawn &&
                    !entity.productionSpawnBlockedAlert;
            if (!producer || !entity.completed || entity.hitPoints <= 0) {
                if (active || !entity.productionQueue.empty() ||
                    !entity.rallyRoute.empty() || !activeStateValid ||
                    entity.rallyRouteAlert) {
                    SetError(error, "snapshot non-producer carries production state");
                    return std::nullopt;
                }
            } else if (!activeStateValid || !queueValid || !rallyValid) {
                SetError(error, "snapshot producer state is invalid");
                return std::nullopt;
            }
        }
    } else {
        // Schemas 20-28 charged production at command admission and retained
        // only the active slot. Reconstruct the exact investment/logistics
        // needed to finish or cancel it under the compatibility path.
        for (Entity& entity : simulation.entities_) {
            if (entity.productionRequired <= 0) {
                continue;
            }
            entity.productionInvestedCost = simulation.ProductionCost(
                entity.faction, entity.productionType);
            entity.productionLogisticsCost = simulation.PopulationCost(
                entity.faction, entity.productionType);
        }
    }
    if (HasLinkMechanicsSnapshotSchema(version)) {
        std::uint32_t linkStateCount = 0;
        if (!reader.U64(simulation.nextProductionItemId_) ||
            !reader.U32(linkStateCount) ||
            simulation.nextProductionItemId_ == 0 ||
            linkStateCount != simulation.entities_.size()) {
            SetError(error, "snapshot Link mechanics header is invalid");
            return std::nullopt;
        }
        std::set<ProductionItemId> itemIds;
        std::vector<bool> storedNetworkState{};
        std::vector<bool> storedAegisState{};
        storedNetworkState.reserve(simulation.entities_.size());
        storedAegisState.reserve(simulation.entities_.size());
        for (Entity& entity : simulation.entities_) {
            EntityId id = 0;
            std::uint8_t waitingCount = 0;
            std::uint8_t networkOperational = 0;
            if (!reader.U32(id) ||
                !reader.U64(entity.activeProductionItemId) ||
                !reader.U8(waitingCount) || id != entity.id ||
                waitingCount != entity.productionQueue.size()) {
                SetError(error, "snapshot Link mechanics state is invalid");
                return std::nullopt;
            }
            for (ProductionQueueItem& item : entity.productionQueue) {
                if (!reader.U64(item.itemId) || item.itemId == 0 ||
                    item.itemId >= simulation.nextProductionItemId_ ||
                    !itemIds.insert(item.itemId).second) {
                    SetError(error,
                             "snapshot production item identity is invalid");
                    return std::nullopt;
                }
            }
            if (!reader.I32(entity.constructionInvestedCost.material) ||
                !reader.I32(entity.constructionInvestedCost.dawnshards) ||
                !reader.U64(entity.repairInterruptedUntilTick) ||
                !reader.I32(entity.repairRateRemainder) ||
                !reader.I32(entity.repairPaidHitPointCredit) ||
                !reader.U8(networkOperational) || networkOperational > 1 ||
                entity.constructionInvestedCost.material < 0 ||
                entity.constructionInvestedCost.dawnshards < 0 ||
                entity.repairInterruptedUntilTick > kMaximumSupportedTick ||
                entity.repairRateRemainder < 0 ||
                entity.repairRateRemainder >=
                    static_cast<std::int32_t>(simulation.config_.ticksPerSecond) ||
                entity.repairPaidHitPointCredit < 0 ||
                entity.repairPaidHitPointCredit > 10 ||
                ((entity.productionRequired > 0) !=
                 (entity.activeProductionItemId != 0)) ||
                (entity.activeProductionItemId != 0 &&
                 (entity.activeProductionItemId >=
                      simulation.nextProductionItemId_ ||
                  !itemIds.insert(entity.activeProductionItemId).second))) {
                SetError(error, "snapshot Link mechanics state is invalid");
                return std::nullopt;
            }
            entity.networkOperational = networkOperational != 0;
            storedNetworkState.push_back(entity.networkOperational);
            storedAegisState.push_back(entity.aegisPowered);
        }
        simulation.legacyLinkReplaySemantics_ = false;
        simulation.ResolveAegisPower();
        for (std::size_t index = 0; index < simulation.entities_.size(); ++index) {
            if (simulation.entities_[index].aegisPowered != storedAegisState[index]) {
                SetError(error, "snapshot Aegis power state is invalid");
                return std::nullopt;
            }
            if (simulation.entities_[index].networkOperational !=
                storedNetworkState[index]) {
                SetError(error,
                         "snapshot Meridian network state is invalid");
                return std::nullopt;
            }
        }
    } else {
        simulation.nextProductionItemId_ = 1;
        for (Entity& entity : simulation.entities_) {
            if (entity.productionRequired > 0) {
                entity.activeProductionItemId =
                    simulation.nextProductionItemId_++;
            }
            for (ProductionQueueItem& item : entity.productionQueue) {
                item.itemId = simulation.nextProductionItemId_++;
            }
            entity.constructionInvestedCost = {};
            entity.repairInterruptedUntilTick = 0;
            entity.repairRateRemainder = 0;
            entity.repairPaidHitPointCredit = 0;
            entity.networkOperational = false;
        }
        // A loaded save migrates immediately to current network semantics so
        // its next schema-30 save is self-consistent. BeginReplaySimulation
        // restores the historical replay cutoff after baseline loading.
        simulation.legacyLinkReplaySemantics_ = false;
        simulation.ResolveAegisPower();
    }
    if (version >= kBulwarkCommitmentSnapshotVersion) {
        std::uint32_t count = 0;
        if (!reader.U32(count) || count != simulation.entities_.size()) {
            SetError(error, "snapshot Bulwark transition count is invalid");
            return std::nullopt;
        }
        for (Entity& entity : simulation.entities_) {
            EntityId id = 0;
            std::uint8_t phase = 0;
            Tick until = 0;
            if (!reader.U32(id) || !reader.U8(phase) || !reader.U64(until) ||
                id != entity.id || phase > static_cast<std::uint8_t>(BulwarkDeploymentPhase::Packing)) {
                SetError(error, "snapshot Bulwark transition record is invalid");
                return std::nullopt;
            }
            const auto decoded = static_cast<BulwarkDeploymentPhase>(phase);
            const Tick duration = decoded == BulwarkDeploymentPhase::Deploying
                ? kBulwarkDeployTicks : kBulwarkPackTicks;
            if ((decoded == BulwarkDeploymentPhase::None && until != 0) ||
                (decoded != BulwarkDeploymentPhase::None &&
                 (entity.faction != Faction::MeridianCompact ||
                  entity.type != EntityType::HeavyUnit || !entity.completed ||
                  until <= simulation.currentTick_ ||
                  until - simulation.currentTick_ > duration ||
                  until > kMaximumSupportedTick ||
                  entity.deployed != (decoded == BulwarkDeploymentPhase::Packing)))) {
                SetError(error, "snapshot Bulwark transition state is inconsistent");
                return std::nullopt;
            }
            entity.deploymentPhase = decoded;
            entity.deploymentTransitionUntilTick = until;
        }
    }
    if (!reader.AtEnd()) {
        SetError(error, "snapshot contains trailing payload data");
        return std::nullopt;
    }
    if (version < 28 && legacyHostilityMasks != kDefaultHostilityMasks) {
        simulation.ClearInvalidOrders();
        std::erase_if(simulation.projectiles_, [&simulation](const Projectile& projectile) {
            const Entity* target = simulation.FindEntity(projectile.target);
            return target != nullptr &&
                !simulation.config_.IsHostile(projectile.owner, target->owner);
        });
    }
    if (IsCancelled()) {
        return std::nullopt;
    }
    simulation.commandLog_.clear();
    simulation.replayInitialSnapshot_.clear();
    if (!simulation.UpdateVisibility(shouldCancel)) {
        SetError(error, "snapshot load cancelled");
        return std::nullopt;
    }
    return simulation;
}

void Simulation::CaptureReplayBaseline() {
    if (legacyLinkReplaySemantics_) {
        // Starting a new current-version recording migrates the live state as
        // well as its baseline; otherwise a zero-tick replay already diverges.
        legacyLinkReplaySemantics_ = false;
        ResolveAegisPower();
    }
    replayInitialSnapshot_ = SaveSnapshot();
    commandLog_.clear();
    replayVersion_ = kReplayVersion;
    replayChecksumSnapshotVersion_ = kSnapshotVersion;
    legacyProductionReplaySemantics_ = false;
    legacyLinkReplaySemantics_ = false;
    legacyBulwarkReplaySemantics_ = false;
    legacyConstructionAssistReplaySemantics_ = false;
    legacyOpenGroundReplaySemantics_ = false;
    legacyMaskedCorridorReplaySemantics_ = false;
    legacyPoweredProductionReplaySemantics_ = false;
    legacyFiringLaneReplaySemantics_ = false;
    replayForfeitingPlayer_ = kNeutralPlayer;
}

void Simulation::DisableReplayExport() {
    replayExportEnabled_ = false;
}

bool Simulation::ContinueReplayRecording(const ReplayRecord& prefix,
                                         std::string* error) {
    if (error != nullptr) {
        error->clear();
    }
    if (!replayExportEnabled_) {
        SetError(error, "replay export is disabled");
        return false;
    }
    if (prefix.finalTick != currentTick_) {
        SetError(error, "replay prefix tick does not match restored state");
        return false;
    }
    std::string replayError;
    std::optional<Simulation> replayed = ReplayToEnd(prefix, &replayError);
    if (!replayed.has_value()) {
        SetError(error, "replay prefix is invalid: " + replayError);
        return false;
    }
    // Snapshot loading evaluates derived network state under current rules.
    // Compare a staged restored candidate under the prefix's historical rules
    // so a supported replay-bound checkpoint is normalized consistently.
    Simulation restored = *this;
    restored.legacyProductionReplaySemantics_ =
        prefix.version < kProductionReplayVersion;
    restored.legacyLinkReplaySemantics_ =
        prefix.version < kLinkMechanicsReplayVersion;
    restored.legacyBulwarkReplaySemantics_ =
        prefix.version < kBulwarkCommitmentReplayVersion;
    restored.legacyConstructionAssistReplaySemantics_ =
        prefix.version < kMaintenanceReplayVersion;
    restored.legacyOpenGroundReplaySemantics_ =
        prefix.version < kGroundOccupancyReplayVersion;
    restored.legacyMaskedCorridorReplaySemantics_ =
        prefix.version < kMaskedCorridorReplayVersion;
    restored.legacyPoweredProductionReplaySemantics_ =
        prefix.version < kPoweredProductionReplayVersion;
    restored.legacyFiringLaneReplaySemantics_ =
        prefix.version < kFiringLaneReplayVersion;
    restored.ResolveAegisPower();
    if (replayed->StateChecksum() != restored.StateChecksum()) {
        SetError(error, "replay prefix state does not match restored state");
        return false;
    }
    *this = std::move(restored);
    replayInitialSnapshot_ = prefix.initialSnapshot;
    commandLog_ = prefix.commands;
    replayVersion_ = prefix.version;
    replayChecksumSnapshotVersion_ =
        replayed->replayChecksumSnapshotVersion_;
    replayForfeitingPlayer_ = prefix.forfeitingPlayer;
    return true;
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
    replay.version = replayVersion_;
    replay.initialSnapshot = replayInitialSnapshot_.empty() ? SaveSnapshot()
                                                            : replayInitialSnapshot_;
    replay.commands = commandLog_;
    std::sort(replay.commands.begin(), replay.commands.end(), CommandLess);
    replay.finalTick = currentTick_;
    replay.finalChecksum = ReplayStateChecksum();
    replay.forfeitingPlayer = replayForfeitingPlayer_;
    return replay;
}

std::optional<Simulation> Simulation::BeginReplaySimulation(
    const ReplayRecord& replay,
    std::string* error,
    const ReplayCancellationCheck& shouldCancel) {
    if (error != nullptr) {
        error->clear();
    }
    if (!IsSupportedReplayVersion(replay.version)) {
        SetError(error, "replay version is unsupported");
        return std::nullopt;
    }
    std::optional<Simulation> simulation = LoadSnapshot(
        replay.initialSnapshot, error, kDefaultHostilityMasks, shouldCancel);
    if (!simulation.has_value()) {
        return std::nullopt;
    }
    const std::uint32_t baselineVersion =
        static_cast<std::uint32_t>(replay.initialSnapshot[4]) |
        (static_cast<std::uint32_t>(replay.initialSnapshot[5]) << 8U) |
        (static_cast<std::uint32_t>(replay.initialSnapshot[6]) << 16U) |
        (static_cast<std::uint32_t>(replay.initialSnapshot[7]) << 24U);
    simulation->replayInitialSnapshot_ = replay.initialSnapshot;
    simulation->replayVersion_ = replay.version;
    simulation->replayChecksumSnapshotVersion_ =
        baselineVersion >= kCommandResolutionReceiptSnapshotVersion
            ? baselineVersion
            : kSnapshotVersion;
    simulation->legacyProductionReplaySemantics_ =
        replay.version < kProductionReplayVersion;
    simulation->legacyLinkReplaySemantics_ =
        replay.version < kLinkMechanicsReplayVersion;
    simulation->legacyBulwarkReplaySemantics_ =
        replay.version < kBulwarkCommitmentReplayVersion;
    simulation->legacyConstructionAssistReplaySemantics_ =
        replay.version < kMaintenanceReplayVersion;
    simulation->legacyOpenGroundReplaySemantics_ =
        replay.version < kGroundOccupancyReplayVersion;
    simulation->legacyMaskedCorridorReplaySemantics_ =
        replay.version < kMaskedCorridorReplayVersion;
    simulation->legacyPoweredProductionReplaySemantics_ =
        replay.version < kPoweredProductionReplayVersion;
    simulation->legacyFiringLaneReplaySemantics_ =
        replay.version < kFiringLaneReplayVersion;
    // Loading a save applies current network rules. Playback must restore the
    // original rules before its first checksum, including zero-tick records.
    simulation->ResolveAegisPower();
    return simulation;
}

std::uint64_t Simulation::ReplayStateChecksum() const {
    HashWriter writer{};
    WriteSnapshotPayload(writer, replayChecksumSnapshotVersion_);
    return writer.Value();
}

std::optional<Simulation> Simulation::ReplayToEnd(const ReplayRecord& replay,
                                                  std::string* error,
                                                  const ReplayCancellationCheck& shouldCancel) {
    if (error != nullptr) {
        error->clear();
    }
    if (!IsSupportedReplayVersion(replay.version)) {
        SetError(error, "replay version is unsupported");
        return std::nullopt;
    }
    if ((replay.version == kLegacyReplayVersion &&
         replay.forfeitingPlayer != kNeutralPlayer) ||
        (replay.forfeitingPlayer != kNeutralPlayer &&
         replay.forfeitingPlayer >= kMaximumPlayers)) {
        SetError(error, "replay forfeit marker is invalid");
        return std::nullopt;
    }
    if (replay.commands.size() > kMaximumSerializedCommands ||
        replay.finalTick > kMaximumSupportedTick) {
        SetError(error, "replay bounds are invalid");
        return std::nullopt;
    }
    if (shouldCancel && shouldCancel()) {
        SetError(error, "replay validation cancelled");
        return std::nullopt;
    }
    std::optional<Simulation> simulation = BeginReplaySimulation(
        replay, error, shouldCancel);
    if (!simulation.has_value()) {
        if (error != nullptr && *error == "snapshot load cancelled") {
            SetError(error, "replay validation cancelled");
        }
        return std::nullopt;
    }
    if (replay.finalTick < simulation->CurrentTick()) {
        SetError(error, "replay final tick precedes its baseline");
        return std::nullopt;
    }
    ReplayCommandSchedule schedule;
    std::string rejection;
    if (!simulation->PrepareReplayCommandSchedule(
            replay, schedule, &rejection, shouldCancel)) {
        SetError(error, rejection == "replay validation cancelled"
            ? rejection
            : "replay command rejected: " + rejection);
        return std::nullopt;
    }
    std::size_t baselineCommand = 0;
    std::size_t recordedCommand = 0;
    std::size_t admittedCommandCount = 0;
    Tick ticksUntilCancellationCheck = 0;
    while (simulation->CurrentTick() < replay.finalTick) {
        if (ticksUntilCancellationCheck == 0) {
            if (shouldCancel && shouldCancel()) {
                SetError(error, "replay validation cancelled");
                return std::nullopt;
            }
            ticksUntilCancellationCheck = 256;
        }
        --ticksUntilCancellationCheck;
        const Tick executingTick = simulation->CurrentTick();
        while (baselineCommand < schedule.baselinePending.size() &&
               schedule.baselinePending[baselineCommand].executeTick ==
                   executingTick) {
            if ((admittedCommandCount++ & 0xffU) == 0U &&
                shouldCancel && shouldCancel()) {
                SetError(error, "replay validation cancelled");
                return std::nullopt;
            }
            simulation->AdmitPreparedReplayCommand(
                schedule.baselinePending[baselineCommand++], false);
        }
        while (recordedCommand < schedule.recorded.size() &&
               schedule.recorded[recordedCommand].executeTick == executingTick) {
            if ((admittedCommandCount++ & 0xffU) == 0U &&
                shouldCancel && shouldCancel()) {
                SetError(error, "replay validation cancelled");
                return std::nullopt;
            }
            simulation->AdmitPreparedReplayCommand(
                schedule.recorded[recordedCommand++], true);
        }
        simulation->Step();
    }
    if (shouldCancel && shouldCancel()) {
        SetError(error, "replay validation cancelled");
        return std::nullopt;
    }
    std::size_t futureCommandCount = 0;
    while (baselineCommand < schedule.baselinePending.size()) {
        if ((futureCommandCount++ & 0xffU) == 0U &&
            shouldCancel && shouldCancel()) {
            SetError(error, "replay validation cancelled");
            return std::nullopt;
        }
        simulation->AdmitPreparedReplayCommand(
            schedule.baselinePending[baselineCommand++], false);
    }
    while (recordedCommand < schedule.recorded.size()) {
        if ((futureCommandCount++ & 0xffU) == 0U &&
            shouldCancel && shouldCancel()) {
            SetError(error, "replay validation cancelled");
            return std::nullopt;
        }
        simulation->AdmitPreparedReplayCommand(
            schedule.recorded[recordedCommand++], true);
    }
    if (shouldCancel && shouldCancel()) {
        SetError(error, "replay validation cancelled");
        return std::nullopt;
    }
    if (replay.forfeitingPlayer != kNeutralPlayer &&
        !simulation->ForfeitPlayer(replay.forfeitingPlayer)) {
        SetError(error, "replay forfeit marker could not be applied");
        return std::nullopt;
    }
    if (simulation->ReplayStateChecksum() != replay.finalChecksum) {
        SetError(error, "replay final checksum does not match");
        return std::nullopt;
    }
    if (shouldCancel && shouldCancel()) {
        SetError(error, "replay validation cancelled");
        return std::nullopt;
    }
    return simulation;
}

std::optional<MatchReport> Simulation::BuildMatchReport(
    const ReplayRecord& replay,
    std::string* error,
    const ReplayCancellationCheck& shouldCancel) {
    if (error != nullptr) {
        error->clear();
    }
    if (!IsSupportedReplayVersion(replay.version)) {
        SetError(error, "replay version is unsupported");
        return std::nullopt;
    }
    if ((replay.version == kLegacyReplayVersion &&
         replay.forfeitingPlayer != kNeutralPlayer) ||
        (replay.forfeitingPlayer != kNeutralPlayer &&
         replay.forfeitingPlayer >= kMaximumPlayers)) {
        SetError(error, "replay forfeit marker is invalid");
        return std::nullopt;
    }
    if (replay.commands.size() > kMaximumSerializedCommands ||
        replay.finalTick > kMaximumSupportedTick) {
        SetError(error, "replay bounds are invalid");
        return std::nullopt;
    }
    if (shouldCancel && shouldCancel()) {
        SetError(error, "replay validation cancelled");
        return std::nullopt;
    }
    std::optional<Simulation> simulation = BeginReplaySimulation(
        replay, error, shouldCancel);
    if (!simulation.has_value()) {
        if (error != nullptr && *error == "snapshot load cancelled") {
            SetError(error, "replay validation cancelled");
        }
        return std::nullopt;
    }
    if (replay.finalTick < simulation->CurrentTick()) {
        SetError(error, "replay final tick precedes its baseline");
        return std::nullopt;
    }

    MatchReport report{};
    report.baselineTick = simulation->CurrentTick();
    report.finalTick = replay.finalTick;
    report.durationTicks = replay.finalTick - report.baselineTick;
    report.finalChecksum = replay.finalChecksum;
    for (PlayerId player = 0; player < kMaximumPlayers; ++player) {
        if (const PlayerState* state = simulation->FindPlayer(player);
            state != nullptr) {
            report.players[player].active = true;
            report.players[player].faction = state->faction;
        }
    }

    ReplayCommandSchedule schedule;
    std::string rejection;
    if (!simulation->PrepareReplayCommandSchedule(
            replay, schedule, &rejection, shouldCancel)) {
        SetError(error, rejection == "replay validation cancelled"
            ? rejection
            : "replay command rejected: " + rejection);
        return std::nullopt;
    }
    const std::vector<Command>& commands = schedule.recorded;
    report.commands.reserve(commands.size());
    std::size_t commandIndex = 0;
    for (const Command& command : commands) {
        if ((commandIndex++ & 0xffU) == 0U &&
            shouldCancel && shouldCancel()) {
            SetError(error, "replay validation cancelled");
            return std::nullopt;
        }
        MatchCommandRecord record{};
        record.tick = command.executeTick;
        record.player = command.player;
        record.sequence = command.sequence;
        record.type = command.type;
        report.commands.push_back(record);
    }

    const auto IsUnit = [](EntityType type) {
        return type == EntityType::Worker || type == EntityType::Soldier ||
            type == EntityType::HeavyUnit || type == EntityType::ScoutUnit;
    };
    const auto IsArmedUnit = [&](const Entity& entity) {
        return IsUnit(entity.type) && entity.attackDamage > 0 &&
            entity.hitPoints > 0;
    };
    const auto MakeEntityMap = [](const std::vector<Entity>& entities) {
        std::map<EntityId, Entity> result;
        for (const Entity& entity : entities) {
            result.emplace(entity.id, entity);
        }
        return result;
    };
    const auto AppendTimelineSample = [&report](Tick tick) {
        MatchTimelineSample sample{};
        sample.tick = tick;
        for (PlayerId player = 0; player < kMaximumPlayers; ++player) {
            const MatchPlayerStatistics& stats = report.players[player];
            sample.unitsTrained[player] = stats.unitsTrained;
            sample.unitsLost[player] = stats.unitsLost;
            sample.materialCollected[player] = stats.materialCollected;
            sample.materialDelivered[player] = stats.materialDelivered;
            sample.admittedCommands[player] = stats.admittedCommands;
        }
        report.timelineSamples.push_back(sample);
    };

    AppendTimelineSample(report.baselineTick);
    std::array<std::uint32_t, kMaximumPlayers> intervalCommands{};
    Tick intervalStart = report.baselineTick;
    std::size_t nextCommand = 0;
    std::size_t nextBaselineCommand = 0;
    std::size_t admittedCommandCount = 0;
    bool firstCombatContactRecorded = false;
    bool majorClashActive = false;

    Tick ticksUntilCancellationCheck = 0;
    while (simulation->CurrentTick() < replay.finalTick) {
        if (ticksUntilCancellationCheck == 0) {
            if (shouldCancel && shouldCancel()) {
                SetError(error, "replay validation cancelled");
                return std::nullopt;
            }
            ticksUntilCancellationCheck = 64;
        }
        --ticksUntilCancellationCheck;
        const Tick executingTick = simulation->CurrentTick();
        while (nextBaselineCommand < schedule.baselinePending.size() &&
               schedule.baselinePending[nextBaselineCommand].executeTick ==
                   executingTick) {
            if ((admittedCommandCount++ & 0xffU) == 0U &&
                shouldCancel && shouldCancel()) {
                SetError(error, "replay validation cancelled");
                return std::nullopt;
            }
            simulation->AdmitPreparedReplayCommand(
                schedule.baselinePending[nextBaselineCommand++], false);
        }
        const std::size_t firstRecordedCommandThisTick = nextCommand;
        while (nextCommand < commands.size() &&
               commands[nextCommand].executeTick == executingTick) {
            if ((admittedCommandCount++ & 0xffU) == 0U &&
                shouldCancel && shouldCancel()) {
                SetError(error, "replay validation cancelled");
                return std::nullopt;
            }
            simulation->AdmitPreparedReplayCommand(commands[nextCommand], true);
            ++nextCommand;
        }
        const std::map<EntityId, Entity> before =
            MakeEntityMap(simulation->Entities());
        const std::vector<Projectile> projectilesBefore =
            simulation->Projectiles();
        std::map<std::pair<PlayerId, std::uint64_t>,
                 CommandResolutionOutcome> tickCommandResolutions;
        simulation->Step(&tickCommandResolutions);
        const std::map<EntityId, Entity> after =
            MakeEntityMap(simulation->Entities());
        std::vector<EntityId> appliedAttackTargets;

        std::size_t resolvedCommand = firstRecordedCommandThisTick;
        while (resolvedCommand < nextCommand) {
            const Command& command = commands[resolvedCommand];
            const auto receipt = tickCommandResolutions.find(
                std::make_pair(command.player, command.sequence));
            if (receipt == tickCommandResolutions.end()) {
                SetError(error, "replay command has no resolution receipt");
                return std::nullopt;
            }
            MatchCommandRecord& record = report.commands[resolvedCommand];
            record.resolved = true;
            record.outcome = receipt->second;
            ++report.players[command.player].admittedCommands;
            ++intervalCommands[command.player];
            if (command.type == CommandType::Attack &&
                receipt->second == CommandResolutionOutcome::Applied) {
                appliedAttackTargets.push_back(command.target);
            }
            if (command.type == CommandType::FutureWell &&
                receipt->second == CommandResolutionOutcome::Applied) {
                report.wellDecisions.push_back({
                    executingTick,
                    command.player,
                    command.target,
                    command.wellChoice});
                report.events.push_back({
                    executingTick,
                    ReplayTimelineEventType::FutureWellProtocol,
                    command.player,
                    command.target});
            }
            ++resolvedCommand;
        }

        bool hostileDamageThisTick = false;
        for (const auto& [id, previous] : before) {
            const auto currentIt = after.find(id);
            const bool lost = currentIt == after.end();
            if (lost && IsUnit(previous.type) &&
                previous.owner < kMaximumPlayers) {
                ++report.players[previous.owner].unitsLost;
            }
            if (lost && previous.type == EntityType::CommandCore &&
                previous.owner < kMaximumPlayers) {
                report.events.push_back({
                    executingTick,
                    ReplayTimelineEventType::CommandCoreLoss,
                    previous.owner,
                    previous.id});
            }
            if (!IsUnit(previous.type) || previous.owner >= kMaximumPlayers ||
                currentIt == after.end()) {
                continue;
            }
            const Entity& current = currentIt->second;
            if (current.type == EntityType::Worker) {
                if (current.cargo > previous.cargo) {
                    report.players[current.owner].materialCollected +=
                        static_cast<std::uint64_t>(current.cargo - previous.cargo);
                } else if (current.cargo < previous.cargo) {
                    // Cargo can leave a surviving worker only through the
                    // authoritative depot-delivery path. Death/removal is
                    // deliberately excluded so lost cargo is not delivered.
                    report.players[current.owner].materialDelivered +=
                        static_cast<std::uint64_t>(previous.cargo - current.cargo);
                }
            }
        }
        for (const auto& [id, current] : after) {
            if (!before.contains(id) && IsUnit(current.type) &&
                current.owner < kMaximumPlayers) {
                ++report.players[current.owner].unitsTrained;
            }
        }

        for (const auto& [id, previous] : before) {
            const auto currentIt = after.find(id);
            if (currentIt != after.end() &&
                currentIt->second.hitPoints >= previous.hitPoints) {
                continue;
            }
            const auto HasHostileAttacker = [&](const Entity& attacker) {
                return attacker.attackDamage > 0 &&
                    attacker.order.target == previous.id &&
                    simulation->Config().IsHostile(
                        attacker.owner, previous.owner);
            };
            hostileDamageThisTick = std::any_of(
                simulation->Entities().begin(), simulation->Entities().end(),
                HasHostileAttacker);
            if (!hostileDamageThisTick) {
                hostileDamageThisTick = std::find(
                    appliedAttackTargets.begin(), appliedAttackTargets.end(),
                    previous.id) != appliedAttackTargets.end();
            }
            if (!hostileDamageThisTick) {
                hostileDamageThisTick = std::any_of(
                    before.begin(), before.end(),
                    [&](const auto& entry) {
                        return HasHostileAttacker(entry.second);
                    });
            }
            if (!hostileDamageThisTick) {
                hostileDamageThisTick = std::any_of(
                    projectilesBefore.begin(), projectilesBefore.end(),
                    [&](const Projectile& projectile) {
                        return projectile.target == previous.id &&
                            simulation->Config().IsHostile(
                                projectile.owner, previous.owner);
                    });
            }
            if (hostileDamageThisTick) {
                if (!firstCombatContactRecorded) {
                    report.events.push_back({
                        executingTick,
                        ReplayTimelineEventType::FirstCombatContact,
                        previous.owner,
                        previous.id});
                    firstCombatContactRecorded = true;
                }
                break;
            }
        }

        std::array<std::uint32_t, kMaximumPlayers> engagedArmy{};
        for (const auto& [id, entity] : after) {
            if (!IsArmedUnit(entity) || entity.owner >= kMaximumPlayers ||
                entity.order.target == 0) {
                continue;
            }
            const auto targetIt = after.find(entity.order.target);
            if (targetIt != after.end() && IsArmedUnit(targetIt->second) &&
                simulation->Config().IsHostile(
                    entity.owner, targetIt->second.owner)) {
                ++engagedArmy[entity.owner];
            }
        }
        bool majorClashNow = false;
        for (PlayerId first = 0; first < kMaximumPlayers; ++first) {
            if (engagedArmy[first] < 2) {
                continue;
            }
            for (PlayerId second = static_cast<PlayerId>(first + 1);
                 second < kMaximumPlayers; ++second) {
                if (engagedArmy[second] >= 2 &&
                    simulation->Config().IsHostile(first, second)) {
                    majorClashNow = true;
                }
            }
        }
        if (majorClashNow && !majorClashActive) {
            report.events.push_back({
                executingTick,
                ReplayTimelineEventType::MajorArmyClash,
                kNeutralPlayer,
                0});
        }
        majorClashActive = majorClashNow;

        const Tick sampleTick = simulation->CurrentTick();
        if (sampleTick - intervalStart == kMatchReportApmIntervalTicks ||
            sampleTick == replay.finalTick) {
            const Tick intervalTicks = sampleTick - intervalStart;
            if (intervalTicks > 0) {
                for (PlayerId player = 0; player < kMaximumPlayers; ++player) {
                    if (!report.players[player].active) {
                        continue;
                    }
                    const std::uint64_t scaled =
                        static_cast<std::uint64_t>(intervalCommands[player]) *
                        kMatchReportApmIntervalTicks * 100U;
                    MatchApmSample sample{};
                    sample.startTick = intervalStart;
                    sample.endTick = sampleTick;
                    sample.player = player;
                    sample.commandCount = intervalCommands[player];
                    sample.actionsPerMinuteX100 =
                        scaled / intervalTicks;
                    report.apmSamples.push_back(sample);
                }
            }
            AppendTimelineSample(sampleTick);
            intervalStart = sampleTick;
            intervalCommands.fill(0);
        }
    }

    if (shouldCancel && shouldCancel()) {
        SetError(error, "replay validation cancelled");
        return std::nullopt;
    }
    std::size_t futureCommandCount = 0;
    while (nextBaselineCommand < schedule.baselinePending.size()) {
        if ((futureCommandCount++ & 0xffU) == 0U &&
            shouldCancel && shouldCancel()) {
            SetError(error, "replay validation cancelled");
            return std::nullopt;
        }
        simulation->AdmitPreparedReplayCommand(
            schedule.baselinePending[nextBaselineCommand++], false);
    }
    while (nextCommand < commands.size()) {
        if ((futureCommandCount++ & 0xffU) == 0U &&
            shouldCancel && shouldCancel()) {
            SetError(error, "replay validation cancelled");
            return std::nullopt;
        }
        simulation->AdmitPreparedReplayCommand(commands[nextCommand++], true);
    }
    if (shouldCancel && shouldCancel()) {
        SetError(error, "replay validation cancelled");
        return std::nullopt;
    }
    if (replay.forfeitingPlayer != kNeutralPlayer) {
        if (!simulation->ForfeitPlayer(replay.forfeitingPlayer)) {
            SetError(error, "replay forfeit marker could not be applied");
            return std::nullopt;
        }
        report.forfeitingPlayer = replay.forfeitingPlayer;
        report.outcomeCause = MatchOutcomeCause::PlayerForfeit;
        // No timeline mark. ForfeitPlayer retires the conceding seat's Core to
        // end the match deterministically, but nobody destroyed it, and the
        // timeline was marking a Command Core loss that never happened. The
        // four marks in REL-QOL-014 are event types, not a guarantee that all
        // four occur in every match: a conceded match simply has no Core loss
        // to bookmark. The cause is already carried by outcomeCause.
    }

    // Commands scheduled after the recording stopped remain admitted replay
    // inputs but have no resolution. Preserve them as resolved=false and keep
    // them out of APM and Well-decision claims.
    if (simulation->ReplayStateChecksum() != replay.finalChecksum) {
        SetError(error, "replay final checksum does not match");
        return std::nullopt;
    }
    report.outcome = simulation->Outcome();
    if (report.outcomeCause == MatchOutcomeCause::None &&
        report.outcome != MatchOutcome::Ongoing) {
        report.outcomeCause = MatchOutcomeCause::CommandCoreLoss;
    }
    if (shouldCancel && shouldCancel()) {
        SetError(error, "replay validation cancelled");
        return std::nullopt;
    }
    return report;
}

}  // namespace echoes::sim
