#pragma once

#if defined(__has_include)
#if __has_include("HAL/Platform.h")
#include "HAL/Platform.h"
#endif
#endif

#include <array>
#include <cstddef>
#include <compare>
#include <cstdint>
#include <deque>
#include <limits>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <vector>

#ifndef ECHOESSIMCORE_API
#define ECHOESSIMCORE_API
#endif

namespace echoes::sim {

using Tick = std::uint64_t;
using EntityId = std::uint32_t;
using PlayerId = std::uint8_t;

inline constexpr PlayerId kNeutralPlayer = 0xff;
inline constexpr std::size_t kMaximumPlayers = 4;
inline constexpr std::int32_t kFixedScale = 1024;
inline constexpr std::size_t kMaximumCommandLogEntries = 256U * 1024U;
inline constexpr std::size_t kMaximumCommandResolutionReceipts = 4096;
inline constexpr Tick kCommandResolutionReceiptRetentionTicks = 1200;
inline constexpr std::uint32_t kSnapshotVersion = 24;
inline constexpr std::uint32_t kReplayVersion = 24;

// Signed Q22.10 fixed-point value. Simulation state never depends on floating point.
class Fixed final {
public:
    constexpr Fixed() = default;

    [[nodiscard]] static constexpr Fixed FromRaw(std::int32_t raw) {
        return Fixed(raw);
    }

    [[nodiscard]] static constexpr Fixed FromInt(std::int32_t value) {
        return FromWide(static_cast<std::int64_t>(value) * kFixedScale);
    }

    [[nodiscard]] static constexpr Fixed FromRatio(std::int32_t numerator,
                                                    std::int32_t denominator) {
        return denominator == 0
                   ? Fixed()
                   : FromWide((static_cast<std::int64_t>(numerator) * kFixedScale) /
                              denominator);
    }

    [[nodiscard]] constexpr std::int32_t Raw() const { return raw_; }
    [[nodiscard]] constexpr std::int32_t FloorToInt() const {
        return raw_ >= 0 ? raw_ / kFixedScale
                         : -static_cast<std::int32_t>(
                               (-static_cast<std::int64_t>(raw_) + kFixedScale - 1) /
                               kFixedScale);
    }

    friend constexpr bool operator==(Fixed, Fixed) = default;
    friend constexpr auto operator<=>(Fixed, Fixed) = default;
    friend constexpr Fixed operator+(Fixed lhs, Fixed rhs) {
        return FromWide(static_cast<std::int64_t>(lhs.raw_) + rhs.raw_);
    }
    friend constexpr Fixed operator-(Fixed lhs, Fixed rhs) {
        return FromWide(static_cast<std::int64_t>(lhs.raw_) - rhs.raw_);
    }

private:
    [[nodiscard]] static constexpr Fixed FromWide(std::int64_t raw) {
        return Fixed(static_cast<std::int32_t>(
            raw > std::numeric_limits<std::int32_t>::max()
                ? std::numeric_limits<std::int32_t>::max()
                : raw < std::numeric_limits<std::int32_t>::min()
                      ? std::numeric_limits<std::int32_t>::min()
                      : raw));
    }

    explicit constexpr Fixed(std::int32_t raw) : raw_(raw) {}
    std::int32_t raw_ = 0;
};

struct Vec2 final {
    Fixed x{};
    Fixed y{};

    [[nodiscard]] static constexpr Vec2 FromRaw(std::int32_t rawX,
                                                std::int32_t rawY) {
        return {Fixed::FromRaw(rawX), Fixed::FromRaw(rawY)};
    }

    [[nodiscard]] static constexpr Vec2 FromTiles(std::int32_t tileX,
                                                  std::int32_t tileY) {
        return {Fixed::FromInt(tileX), Fixed::FromInt(tileY)};
    }

    friend constexpr bool operator==(const Vec2&, const Vec2&) = default;
};

enum class Faction : std::uint8_t {
    MeridianCompact = 0,
    KharuunAssemblies = 1,
    HollowChoir = 2,
};

enum class ResearchType : std::uint8_t {
    None = 0,
    MeridianPrismaticTargeting = 1,
    MeridianHorizonLattice = 2,
    KharuunEchoCartography = 3,
    KharuunAncestralEdge = 4,
    ChoirHeldAlternatives = 5,
    ChoirSharedResolution = 6,
};

inline constexpr std::size_t kResearchTypeCount = 7;

enum class EntityType : std::uint8_t {
    Worker = 0,
    Soldier = 1,
    CommandCore = 2,
    Dropoff = 3,
    Barracks = 4,
    ResourceNode = 5,
    FutureWell = 6,
    HeavyUnit = 7,
    ScoutUnit = 8,
    UtilityStructure = 9,
};

enum class Terrain : std::uint8_t {
    Open = 0,
    Blocked = 1,
    Scarred = 2,
};

enum class Visibility : std::uint8_t {
    Unexplored = 0,
    Explored = 1,
    Visible = 2,
};

enum class FutureWellChoice : std::uint8_t {
    Dormant = 0,
    Harvest = 1,
    Preserve = 2,
    Reshape = 3,
};

enum class OrderType : std::uint8_t {
    None = 0,
    Move = 1,
    Gather = 2,
    Deliver = 3,
    Build = 4,
    Attack = 5,
    FutureWell = 6,
    AttackMove = 7,
    Hold = 8,
    Guard = 9,
    Patrol = 10,
};

enum class CommandType : std::uint8_t {
    Stop = 0,
    Move = 1,
    Gather = 2,
    Deliver = 3,
    Build = 4,
    Attack = 5,
    FutureWell = 6,
    Produce = 7,
    AttackMove = 8,
    Hold = 9,
    Guard = 10,
    Patrol = 11,
    ToggleDeploy = 12,
    ActivateRelaySupply = 13,
    ToggleWaystoneRoot = 14,
    AdaptWarform = 15,
    RaiseMineralCover = 16,
    Research = 17,
    ReconcileToManifest = 18,
    ReconcileToPossible = 19,
};

/** Coarse authoritative result of resolving a structurally admitted command. */
enum class CommandResolutionOutcome : std::uint8_t {
    Applied = 0,
    NoEffect = 1,
    InvalidPosition = 2,
};

enum class ChoirIdentityState : std::uint8_t {
    NotChoir = 0,
    Manifest = 1,
    Possible = 2,
    DualResolveManifest = 3,
    DualResolvePossible = 4,
};

enum class WaystoneMode : std::uint8_t {
    NotWaystone = 0,
    Rooted = 1,
    Uprooting = 2,
    Mobile = 3,
    Rooting = 4,
};

enum class WarformAdaptation : std::uint8_t {
    None = 0,
    Carapace = 1,
    Striker = 2,
};

enum class PlacementResult : std::uint8_t {
    Valid = 0,
    InvalidPlayer = 1,
    InvalidBuildingType = 2,
    OutsideMap = 3,
    TerrainRestricted = 4,
    Occupied = 5,
};

enum class ProductionResult : std::uint8_t {
    Valid = 0,
    InvalidPlayer = 1,
    InvalidProducer = 2,
    ProducerIncomplete = 3,
    ProducerBusy = 4,
    UnsupportedUnit = 5,
    InsufficientResources = 6,
    CapacityReached = 7,
    EntityCapacityReached = 8,
};

enum class ResearchResult : std::uint8_t {
    Valid = 0,
    InvalidPlayer = 1,
    InvalidProducer = 2,
    ProducerIncomplete = 3,
    ProducerBusy = 4,
    InvalidTechnology = 5,
    WrongFaction = 6,
    AlreadyCompleted = 7,
    PrerequisiteMissing = 8,
    InsufficientResources = 9,
};

enum class RelaySupplyResult : std::uint8_t {
    Valid = 0,
    InvalidPlayer = 1,
    InvalidActor = 2,
    AlreadyActive = 3,
    CooldownActive = 4,
    Disconnected = 5,
};

enum class WaystoneRootResult : std::uint8_t {
    Valid = 0,
    InvalidPlayer = 1,
    InvalidActor = 2,
    TransitionActive = 3,
    RootingBlocked = 4,
};

enum class WarformAdaptationResult : std::uint8_t {
    Valid = 0,
    InvalidPlayer = 1,
    InvalidActor = 2,
    InvalidAdaptation = 3,
    AlreadyAdapted = 4,
    MoltActive = 5,
    InvalidSite = 6,
    OutsideSiteRadius = 7,
    InsufficientDawn = 8,
};

enum class MineralCoverResult : std::uint8_t {
    Valid = 0,
    InvalidPlayer = 1,
    InvalidActor = 2,
    MoltActive = 3,
    CooldownActive = 4,
    OutsideCastRange = 5,
    InvalidPosition = 6,
    Occupied = 7,
    InsufficientDawn = 8,
    EntityCapacityReached = 9,
};

enum class ChoirReconciliationResult : std::uint8_t {
    Valid = 0,
    InvalidPlayer = 1,
    InvalidActor = 2,
    AlreadyResolving = 3,
    AlreadyStable = 4,
    CooldownActive = 5,
    InsufficientDawn = 6,
};

enum class MatchOutcome : std::uint8_t {
    Ongoing = 0,
    Player0Victory = 1,
    Player1Victory = 2,
    Draw = 3,
    Player2Victory = 4,
    Player3Victory = 5,
};

enum class AiPersonality : std::uint8_t {
    Balanced = 0,
    Defensive = 1,
    Raider = 2,
    Economic = 3,
    Expansionist = 4,
    Adaptive = 5,
};

struct ResourcePool final {
    std::int32_t material = 0;
    std::int32_t dawnshards = 0;

    friend bool operator==(const ResourcePool&, const ResourcePool&) = default;
};

inline constexpr std::size_t kFactionCount = 3;
inline constexpr std::size_t kConfigurableEntityTypeCount = 10;

/** Deterministic values for one foundational simulation archetype. */
struct EntityArchetypeRules final {
    ResourcePool cost{};
    std::int32_t maxHitPoints = 1;
    std::int32_t movementPerTickRaw = 0;
    std::int32_t visionTiles = 0;
    std::int32_t attackRangeRaw = 0;
    std::int32_t attackDamage = 0;
    Tick attackPeriodTicks = 0;
    std::int32_t workRate = 0;
    std::int32_t cargoCapacity = 0;
    std::int32_t constructionRequired = 0;
    std::int32_t populationCost = 0;
    std::int32_t populationCapacity = 0;
    std::int32_t productionTicks = 0;
    std::int32_t footprintHalfExtentRaw = kFixedScale / 8;

    friend bool operator==(const EntityArchetypeRules&,
                           const EntityArchetypeRules&) = default;
};

/** Deterministic Future Well economy and active-duration rules. */
struct FutureWellRules final {
    std::int32_t harvestImmediateDawn = 300;
    std::int32_t preserveDawnPerInterval = 3;
    Tick preserveIntervalTicks = 10;
    std::int32_t preserveVisionTiles = 8;
    std::int32_t reshapeDawnCost = 100;
    Tick reshapeDurationMinimumTicks = 40;
    Tick reshapeDurationMaximumTicks = 60;

    friend bool operator==(const FutureWellRules&,
                           const FutureWellRules&) = default;
};

/** Authored Meridian Bulwark directional-cover behavior. */
struct BulwarkDeploymentRules final {
    std::int32_t coverDepthRaw = 3 * kFixedScale;
    std::int32_t coverHalfWidthRaw = 2 * kFixedScale;
    std::int32_t damageReductionPercent = 40;
    std::int32_t deployedMovementPercent = 35;

    friend bool operator==(const BulwarkDeploymentRules&,
                           const BulwarkDeploymentRules&) = default;
};

/** Authored Meridian Relay Skiff temporary-logistics behavior. */
struct RelaySupplyRules final {
    std::int32_t connectionRadiusRaw = 7 * kFixedScale;
    std::int32_t capacityBonus = 4;
    Tick durationTicks = 400;
    Tick cooldownTicks = 800;

    friend bool operator==(const RelaySupplyRules&,
                           const RelaySupplyRules&) = default;
};

/** Authored Kharuun Waystone migration and rooting behavior. */
struct WaystoneMigrationRules final {
    std::int32_t movementPerTickRaw = Fixed::FromRatio(3, 50).Raw();
    Tick uprootTicks = 40;
    Tick rootTicks = 60;
    std::int32_t mobileDamageTakenPercent = 125;

    friend bool operator==(const WaystoneMigrationRules&,
                           const WaystoneMigrationRules&) = default;
};

/** Authored Kharuun warform adaptation behavior rooted at a Growth Basin. */
struct WarformAdaptationRules final {
    std::int32_t siteRadiusRaw = 6 * kFixedScale;
    Tick moltTicks = 80;
    std::int32_t dawnCost = 25;
    std::int32_t moltDamageTakenPercent = 150;
    std::int32_t carapaceHealthPercent = 135;
    std::int32_t carapaceMovementPercent = 80;
    std::int32_t strikerDamagePercent = 125;
    std::int32_t strikerCooldownPercent = 85;

    friend bool operator==(const WarformAdaptationRules&,
                           const WarformAdaptationRules&) = default;
};

/** Authored Kharuun Cairnback destructible temporary-cover behavior. */
struct MineralCoverRules final {
    std::int32_t castRangeRaw = 4 * kFixedScale + kFixedScale / 2;
    Tick durationTicks = 300;
    Tick cooldownTicks = 600;
    std::int32_t dawnCost = 15;
    std::int32_t maxHitPoints = 180;
    std::int32_t halfExtentRaw = 3 * kFixedScale / 4;

    friend bool operator==(const MineralCoverRules&,
                           const MineralCoverRules&) = default;
};

/** Authored Kharuun Resonant and Listening Spine movement-signature behavior. */
struct VibrationDetectionRules final {
    std::int32_t resonantRadiusRaw = 22 * kFixedScale;
    std::int32_t listeningSpineRadiusRaw = 26 * kFixedScale;
    Tick signatureLingerTicks = 40;
    std::int32_t contactResolutionRaw = 2 * kFixedScale;

    friend bool operator==(const VibrationDetectionRules&,
                           const VibrationDetectionRules&) = default;
};

/** Authored Meridian Aegis power-network connection behavior. */
struct PoweredAegisRules final {
    std::int32_t connectionRadiusRaw = 8 * kFixedScale;

    friend bool operator==(const PoweredAegisRules&,
                           const PoweredAegisRules&) = default;
};

/** Authored Hollow Choir incompatible-capability reconciliation behavior. */
struct ChoirIdentityRules final {
    Tick durationTicks = 160;
    Tick cooldownTicks = 400;
    std::int32_t dawnCost = 20;
    std::int32_t manifestDamagePercent = 130;
    std::int32_t possibleMovementPercent = 130;
    std::int32_t possibleVisionPercent = 125;

    friend bool operator==(const ChoirIdentityRules&,
                           const ChoirIdentityRules&) = default;
};

/** Authored Hollow Choir structure persistence while coherence remains funded. */
struct ChoirCoherenceRules final {
    Tick upkeepIntervalTicks = 600;
    std::int32_t dawnCostPerStructure = 5;

    friend bool operator==(const ChoirCoherenceRules&,
                           const ChoirCoherenceRules&) = default;
};

/** Authored deterministic research definition. Index matches ResearchType. */
struct ResearchRules final {
    Faction faction = Faction::MeridianCompact;
    ResourcePool cost{};
    Tick researchTicks = 0;
    ResearchType prerequisite = ResearchType::None;
    std::int32_t combatDamagePercent = 100;
    std::int32_t combatVisionPercent = 100;

    friend bool operator==(const ResearchRules&, const ResearchRules&) = default;
};

/** Versioned authoritative rules copied into saves, replays, and checksums. */
struct SimulationRules final {
    std::uint32_t version = 2;
    std::array<std::uint8_t, 32> contentSha256{};
    std::array<std::array<EntityArchetypeRules,
                          kConfigurableEntityTypeCount>,
               kFactionCount>
        archetypes{};
    FutureWellRules futureWell{};
    BulwarkDeploymentRules bulwarkDeployment{};
    RelaySupplyRules relaySupply{};
    WaystoneMigrationRules waystoneMigration{};
    WarformAdaptationRules warformAdaptation{};
    MineralCoverRules mineralCover{};
    VibrationDetectionRules vibrationDetection{};
    PoweredAegisRules poweredAegis{};
    ChoirIdentityRules choirIdentity{};
    ChoirCoherenceRules choirCoherence{};
    std::array<ResearchRules, kResearchTypeCount> research{};

    friend bool operator==(const SimulationRules&,
                           const SimulationRules&) = default;
};

[[nodiscard]] ECHOESSIMCORE_API SimulationRules DefaultSimulationRules();

struct PlayerState final {
    PlayerId id = 0;
    Faction faction = Faction::MeridianCompact;
    ResourcePool resources{};
    bool active = false;
    std::uint32_t completedResearchMask = 0;
    ResearchType activeResearch = ResearchType::None;
    EntityId researchProducer = 0;
    std::int32_t researchProgress = 0;
    std::int32_t researchRequired = 0;
    ResearchType lastInterruptedResearch = ResearchType::None;

    [[nodiscard]] bool HasCompletedResearch(ResearchType type) const {
        const std::uint8_t value = static_cast<std::uint8_t>(type);
        return value > 0 && value < 32 &&
               (completedResearchMask & (1U << value)) != 0;
    }
};

struct Order final {
    OrderType type = OrderType::None;
    EntityId target = 0;
    Vec2 anchor{};
    Vec2 destination{};
    EntityType buildType = EntityType::Barracks;
    FutureWellChoice wellChoice = FutureWellChoice::Dormant;

    friend bool operator==(const Order&, const Order&) = default;
};

struct Entity final {
    EntityId id = 0;
    PlayerId owner = kNeutralPlayer;
    Faction faction = Faction::MeridianCompact;
    EntityType type = EntityType::Worker;
    Vec2 position{};
    std::int32_t hitPoints = 1;
    std::int32_t maxHitPoints = 1;
    std::int32_t movementPerTickRaw = 0;
    std::int32_t visionTiles = 0;
    std::int32_t attackRangeRaw = 0;
    std::int32_t attackDamage = 0;
    Tick attackPeriodTicks = 0;
    Tick attackCooldownTicks = 0;
    std::int32_t workRate = 0;
    std::int32_t cargo = 0;
    std::int32_t cargoCapacity = 0;
    std::int32_t resourceRemaining = 0;
    bool completed = true;
    std::int32_t constructionProgress = 0;
    std::int32_t constructionRequired = 0;
    Order order{};
    FutureWellChoice wellChoice = FutureWellChoice::Dormant;
    // The first completed protocol tick. This is authoritative, monotonic,
    // and lets downstream scenarios measure an intact activation interval
    // without relying on presentation or subsystem-only timers.
    Tick wellActivationTick = 0;
    Tick reshapeUntilTick = 0;
    std::uint8_t reshapeVariant = 0;
    EntityType productionType = EntityType::Worker;
    std::int32_t productionProgress = 0;
    std::int32_t productionRequired = 0;
    bool deployed = false;
    Vec2 deploymentFacing = Vec2::FromRaw(kFixedScale, 0);
    bool relaySupplyActive = false;
    Tick relaySupplyUntilTick = 0;
    Tick relaySupplyCooldownUntilTick = 0;
    WaystoneMode waystoneMode = WaystoneMode::NotWaystone;
    Tick waystoneTransitionUntilTick = 0;
    WarformAdaptation warformAdaptation = WarformAdaptation::None;
    WarformAdaptation pendingWarformAdaptation = WarformAdaptation::None;
    EntityId moltSite = 0;
    Tick moltUntilTick = 0;
    Tick mineralCoverCooldownUntilTick = 0;
    bool temporaryMineralCover = false;
    EntityId mineralCoverCreator = 0;
    Tick mineralCoverUntilTick = 0;
    Terrain mineralCoverUnderlyingTerrain = Terrain::Open;
    Tick vibrationSignatureUntilTick = 0;
    bool aegisPowered = false;
    ChoirIdentityState choirIdentityState = ChoirIdentityState::NotChoir;
    Tick choirIdentityResolveAtTick = 0;
    Tick choirIdentityNextAvailableTick = 0;
    Tick choirCoherenceNextChargeTick = 0;

    friend bool operator==(const Entity&, const Entity&) = default;
};

struct Command final {
    Tick executeTick = 0;
    PlayerId player = 0;
    std::uint64_t sequence = 0;
    CommandType type = CommandType::Stop;
    EntityId actor = 0;
    EntityId target = 0;
    Vec2 position{};
    EntityType buildType = EntityType::Barracks;
    FutureWellChoice wellChoice = FutureWellChoice::Dormant;
    WarformAdaptation warformAdaptation = WarformAdaptation::None;
    ResearchType researchType = ResearchType::None;

    friend bool operator==(const Command&, const Command&) = default;
};

/**
 * Presentation-safe evidence that a command reached deterministic resolution.
 * Applied does not mean that a long-lived order has completed. NoEffect
 * deliberately withholds semantic failure details, and InvalidPosition is the
 * one explicit boundary required for mineral-cover coordinate rejection.
 */
struct CommandResolutionReceipt final {
    PlayerId player = 0;
    CommandType commandType = CommandType::Stop;
    Tick assignedExecutionTick = 0;
    CommandResolutionOutcome outcome = CommandResolutionOutcome::NoEffect;

    friend bool operator==(const CommandResolutionReceipt&,
                           const CommandResolutionReceipt&) = default;
};

struct SimulationConfig final {
    std::int32_t mapWidthTiles = 64;
    std::int32_t mapHeightTiles = 64;
    std::uint32_t ticksPerSecond = 20;
    std::uint64_t randomSeed = 1;
    SimulationRules rules = DefaultSimulationRules();
    // Bit N makes player N's Command Core ineligible as an enemy target. The
    // default remains zero so authored matches retain their normal outcome
    // contract; deterministic endurance fixtures opt in explicitly.
    std::uint8_t protectedCommandCorePlayerMask = 0;

    friend bool operator==(const SimulationConfig&,
                           const SimulationConfig&) = default;
};

struct PlayerViewTile final {
    Visibility visibility = Visibility::Unexplored;
    Terrain terrain = Terrain::Blocked;
    bool passable = false;

    friend bool operator==(const PlayerViewTile&, const PlayerViewTile&) = default;
};

/** Approximate anonymous contact produced by Kharuun vibration detectors. */
struct VibrationSignature final {
    Vec2 approximatePosition{};

    friend bool operator==(const VibrationSignature&,
                           const VibrationSignature&) = default;
};

// Materialized, visibility-scoped input for command producers. Construction is
// restricted to Simulation so AI logic cannot acquire an authoritative-world
// reference or fabricate information that the player has not observed.
class ECHOESSIMCORE_API PlayerView final {
public:
    [[nodiscard]] const SimulationConfig& Config() const { return config_; }
    [[nodiscard]] Tick CurrentTick() const { return currentTick_; }
    [[nodiscard]] const PlayerState& Player() const { return player_; }
    [[nodiscard]] std::uint64_t DecisionSeed() const { return decisionSeed_; }
    [[nodiscard]] std::int32_t PopulationUsed() const { return populationUsed_; }
    [[nodiscard]] std::int32_t PopulationCapacity() const {
        return populationCapacity_;
    }
    [[nodiscard]] const std::vector<Entity>& Entities() const { return entities_; }
    [[nodiscard]] const std::vector<VibrationSignature>& VibrationSignatures() const {
        return vibrationSignatures_;
    }
    [[nodiscard]] Visibility VisibilityAt(Vec2 position) const;
    [[nodiscard]] Terrain TerrainAt(std::int32_t tileX,
                                    std::int32_t tileY) const;
    [[nodiscard]] bool IsPositionPassable(Vec2 position) const;

private:
    friend class Simulation;
    PlayerView() = default;

    SimulationConfig config_{};
    Tick currentTick_ = 0;
    PlayerState player_{};
    std::uint64_t decisionSeed_ = 0;
    std::int32_t populationUsed_ = 0;
    std::int32_t populationCapacity_ = 0;
    std::vector<PlayerViewTile> tiles_{};
    std::vector<Entity> entities_{};
    std::vector<VibrationSignature> vibrationSignatures_{};
};

struct ReplayRecord final {
    std::uint32_t version = kReplayVersion;
    std::vector<std::uint8_t> initialSnapshot{};
    // Receipts are never an independent replay input. Commands after the
    // baseline regenerate them through normal deterministic resolution.
    std::vector<Command> commands{};
    Tick finalTick = 0;
    std::uint64_t finalChecksum = 0;
};

class ECHOESSIMCORE_API Simulation final {
public:
    explicit Simulation(SimulationConfig config = {});

    [[nodiscard]] const SimulationConfig& Config() const { return config_; }
    [[nodiscard]] Tick CurrentTick() const { return currentTick_; }
    [[nodiscard]] const std::vector<Entity>& Entities() const { return entities_; }
    [[nodiscard]] const std::vector<Command>& CommandLog() const {
        return commandLog_;
    }
    [[nodiscard]] std::optional<std::uint64_t> NextCommandSequence(
        PlayerId player) const;
    /**
     * Observational lookup only. Absence covers missing, pending, expired,
     * legacy-loaded, and otherwise unavailable evidence; it never authorizes
     * retry, replay, or an assumption of success.
     */
    [[nodiscard]] std::optional<CommandResolutionReceipt>
    FindCommandResolutionReceipt(PlayerId player,
                                 std::uint64_t sequence) const;

    bool AddPlayer(PlayerId player, Faction faction, ResourcePool startingResources);
    [[nodiscard]] const PlayerState* FindPlayer(PlayerId player) const;

    EntityId SpawnEntity(PlayerId owner,
                         Faction faction,
                         EntityType type,
                         Vec2 position);
    EntityId SpawnPublicInterface(Faction faction, Vec2 position);
    EntityId SpawnResourceNode(Vec2 position, std::int32_t amount);
    EntityId SpawnFutureWell(Vec2 position);
    [[nodiscard]] const Entity* FindEntity(EntityId id) const;

    bool SetTerrainTile(std::int32_t tileX, std::int32_t tileY, Terrain terrain);
    [[nodiscard]] Terrain TerrainAt(std::int32_t tileX,
                                    std::int32_t tileY) const;
    [[nodiscard]] bool IsPositionPassable(Vec2 position) const;
    /** Terrain- and footprint-aware admission check for controlled spawning. */
    [[nodiscard]] bool IsSpawnPositionAvailable(Faction faction,
                                                EntityType type,
                                                Vec2 position) const;
    [[nodiscard]] PlacementResult ValidatePlacement(PlayerId player,
                                                    EntityType buildingType,
                                                    Vec2 position,
                                                    EntityId* blockingEntity = nullptr) const;
    [[nodiscard]] ProductionResult ValidateProduction(
        PlayerId player,
        EntityId producer,
        EntityType unitType) const;
    [[nodiscard]] ResearchResult ValidateResearch(
        PlayerId player,
        EntityId producer,
        ResearchType researchType) const;
    [[nodiscard]] const ResearchRules* ResearchDefinition(
        ResearchType researchType) const;
    [[nodiscard]] RelaySupplyResult ValidateRelaySupply(
        PlayerId player,
        EntityId actor) const;
    [[nodiscard]] WaystoneRootResult ValidateWaystoneRoot(
        PlayerId player,
        EntityId actor) const;
    [[nodiscard]] WarformAdaptationResult ValidateWarformAdaptation(
        PlayerId player,
        EntityId actor,
        EntityId site,
        WarformAdaptation adaptation) const;
    [[nodiscard]] MineralCoverResult ValidateMineralCover(
        PlayerId player,
        EntityId actor,
        Vec2 position) const;
    [[nodiscard]] ChoirReconciliationResult ValidateChoirReconciliation(
        PlayerId player,
        EntityId actor,
        ChoirIdentityState stableState) const;
    [[nodiscard]] ResourcePool BuildCost(Faction faction, EntityType type) const;
    [[nodiscard]] ResourcePool ProductionCost(Faction faction,
                                               EntityType type) const;
    [[nodiscard]] std::int32_t PopulationUsed(PlayerId player) const;
    [[nodiscard]] std::int32_t PopulationCapacity(PlayerId player) const;
    [[nodiscard]] MatchOutcome Outcome() const;
    /** Records a deterministic player forfeit by retiring that player's live Command Core. */
    bool ForfeitPlayer(PlayerId player);

    bool QueueCommand(const Command& command, std::string* rejectionReason = nullptr);
    void Step();
    void Step(Tick tickCount);

    [[nodiscard]] Visibility VisibilityAt(PlayerId player, Vec2 position) const;
    [[nodiscard]] bool IsEntityVisibleTo(PlayerId player, EntityId entity) const;

    // The only supported bridge from authoritative state to an AI/controller.
    [[nodiscard]] std::optional<PlayerView> CreatePlayerView(PlayerId player) const;

    // Compatibility entry point; immediately materializes a PlayerView.
    [[nodiscard]] std::vector<Command> GenerateAiCommands(
        PlayerId player,
        AiPersonality personality = AiPersonality::Balanced) const;
    // Pure command producer: has no Simulation reference or hidden-world access.
    [[nodiscard]] static std::vector<Command> GenerateAiCommands(
        const PlayerView& view,
        AiPersonality personality = AiPersonality::Balanced);

    [[nodiscard]] std::uint64_t StateChecksum() const;
    [[nodiscard]] std::vector<std::uint8_t> SaveSnapshot() const;
    [[nodiscard]] static std::optional<Simulation> LoadSnapshot(
        std::span<const std::uint8_t> bytes,
        std::string* error = nullptr);

    // Capture after deterministic map/scenario setup and before player commands.
    void CaptureReplayBaseline();
    // One-way runtime gate for fixtures whose out-of-band state transitions
    // cannot be represented by the deterministic replay command stream.
    void DisableReplayExport();
    [[nodiscard]] ReplayRecord ExportReplay(std::string* error = nullptr) const;
    [[nodiscard]] static std::optional<Simulation> ReplayToEnd(
        const ReplayRecord& replay,
        std::string* error = nullptr);

#if defined(ECHOES_SIMCORE_PROFILE)
    // Compiled only into the native profiler so subsystem costs can be isolated.
    void ProfileRefreshVisibility();
    [[nodiscard]] bool ProfilePathRequest(Vec2 from, Vec2 destination) const;
#endif

private:
    template <typename Writer>
    void WriteSnapshotPayload(Writer& writer) const;

    struct PathFieldCacheEntry final {
        std::vector<std::size_t> distanceToGoal{};
        Tick lastUsedTick = 0;
    };

    struct DeterministicRng final {
        explicit DeterministicRng(std::uint64_t seed = 1) : state(seed) {}
        [[nodiscard]] std::uint32_t NextU32();
        [[nodiscard]] std::uint32_t Uniform(std::uint32_t exclusiveUpperBound);
        std::uint64_t state = 1;
    };

    struct PendingDamage final {
        EntityId target = 0;
        EntityId source = 0;
        std::int32_t damage = 0;
    };

    struct StoredCommandResolutionReceipt final {
        std::uint64_t sequence = 0;
        CommandResolutionReceipt receipt{};
    };

    [[nodiscard]] bool IsInsideMap(Vec2 position,
                                   std::int32_t halfExtentRaw = 0) const;
    [[nodiscard]] PlayerState* MutablePlayer(PlayerId player);
    [[nodiscard]] Entity* MutableEntity(EntityId id);
    [[nodiscard]] bool IsBuilding(EntityType type) const;
    [[nodiscard]] bool IsDropoff(EntityType type) const;
    [[nodiscard]] std::int32_t FootprintHalfExtentRaw(Faction faction,
                                                      EntityType type) const;
    [[nodiscard]] Entity MakeEntity(PlayerId owner,
                                    Faction faction,
                                    EntityType type,
                                    Vec2 position) const;
    [[nodiscard]] std::int32_t ProductionTicks(Faction faction,
                                               EntityType type) const;
    [[nodiscard]] std::int32_t PopulationCost(Faction faction,
                                              EntityType type) const;
    [[nodiscard]] std::optional<Vec2> FindProductionSpawnPosition(
        const Entity& producer) const;
    [[nodiscard]] bool IsReshapedOpen(std::int32_t tileX,
                                      std::int32_t tileY) const;
    [[nodiscard]] bool InInteractionRange(const Entity& first,
                                          const Entity& second,
                                          std::int32_t extraRangeRaw) const;
    [[nodiscard]] std::optional<Vec2> FindNextPathWaypoint(
        Vec2 from,
        Vec2 destination) const;
    [[nodiscard]] bool MoveTowards(Entity& entity, Vec2 destination);
    [[nodiscard]] bool IsRelayConnected(const Entity& relay) const;
    [[nodiscard]] bool CanRootWaystone(const Entity& waystone) const;
    [[nodiscard]] bool IsOperationalDropoff(const Entity& entity) const;
    [[nodiscard]] bool IsWarform(const Entity& entity) const;
    [[nodiscard]] bool IsCairnback(const Entity& entity) const;
    [[nodiscard]] std::int32_t VibrationDetectionRadiusRaw(
        const Entity& entity) const;
    [[nodiscard]] bool IsAegisPost(const Entity& entity) const;
    [[nodiscard]] bool IsAegisNetworkPowered(const Entity& aegis) const;
    [[nodiscard]] bool IsProtectedCommandCore(const Entity& entity) const;
    [[nodiscard]] bool IsChoirIdentityUnit(const Entity& entity) const;
    [[nodiscard]] bool IsChoirCoherenceStructure(const Entity& entity) const;
    [[nodiscard]] EntityId InterceptingMineralCover(
        const Entity& attacker,
        const Entity& target) const;
    void ApplyWarformAdaptation(Entity& entity,
                                WarformAdaptation adaptation);
    void ApplyResearchRule(Entity& entity, const ResearchRules& rules) const;
    void RefreshChoirIdentityStats(Entity& entity) const;
    [[nodiscard]] std::int32_t DamageAfterDirectionalCover(
        const Entity& attacker,
        const Entity& target,
        std::int32_t damage) const;
    [[nodiscard]] EntityId FindNearestOwnedDropoff(PlayerId player,
                                                   Vec2 from) const;
    [[nodiscard]] EntityId FindNearestVisibleEnemy(PlayerId player,
                                                   Vec2 from,
                                                   std::int32_t radiusRaw) const;
    [[nodiscard]] EntityId FindNearestVisibleEnemyInRange(
        const Entity& attacker) const;
    [[nodiscard]] EntityId FindNearestVisiblePatrolEnemy(
        const Entity& attacker) const;
    [[nodiscard]] bool IsInsidePatrolEnvelope(
        const Order& order,
        Vec2 position) const;
    [[nodiscard]] std::uint64_t DistanceSquaredRaw(Vec2 first, Vec2 second) const;
    [[nodiscard]] bool TryAllocateEntityId(EntityId& id);

    void UpdateVisibility();
    void ResolveExpiredReshapes();
    void ResolveExpiredRelaySupply();
    void ResolveWaystoneTransitions();
    void ResolveWarformMolts();
    void ResolveMineralCovers();
    void ResolveAegisPower();
    void ResolveChoirIdentities();
    void ResolveChoirCoherence();
    void ProcessCommandsForCurrentTick();
    [[nodiscard]] CommandResolutionOutcome ApplyCommand(
        const Command& command);
    void RecordCommandResolutionReceipt(
        const Command& command,
        CommandResolutionOutcome outcome);
    void PruneCommandResolutionReceipts();
    void ProcessEntityOrders();
    void ProcessGather(Entity& worker);
    void ProcessDeliver(Entity& worker);
    void ProcessBuild(Entity& worker);
    void ProcessAttack(Entity& attacker,
                       std::vector<PendingDamage>& pendingDamage);
    void ProcessAttackMove(
        Entity& attacker,
        std::vector<PendingDamage>& pendingDamage);
    void ProcessHold(
        Entity& attacker,
        std::vector<PendingDamage>& pendingDamage);
    void ProcessGuard(
        Entity& attacker,
        std::vector<PendingDamage>& pendingDamage);
    void ProcessPatrol(
        Entity& attacker,
        std::vector<PendingDamage>& pendingDamage);
    void ProcessAegisDefense(
        Entity& aegis,
        std::vector<PendingDamage>& pendingDamage);
    void ProcessFutureWell(Entity& worker);
    void ProcessProduction();
    void ProcessResearch();
    void ApplyPreserveIncome();
    void RemoveDestroyedEntities();
    void ClearInvalidOrders();

    SimulationConfig config_{};
    Tick currentTick_ = 0;
    EntityId nextEntityId_ = 1;
    DeterministicRng rng_{};
    std::array<PlayerState, kMaximumPlayers> players_{};
    std::vector<Terrain> terrain_{};
    std::array<std::vector<std::uint8_t>, kMaximumPlayers> explored_{};
    std::array<std::vector<std::uint8_t>, kMaximumPlayers> visible_{};
    std::vector<Entity> entities_{};
    std::vector<Command> pendingCommands_{};
    std::vector<Command> commandLog_{};
    std::deque<StoredCommandResolutionReceipt> commandResolutionReceipts_{};
    std::vector<std::uint8_t> replayInitialSnapshot_{};
    bool replayExportEnabled_ = true;
    std::array<std::uint64_t, kMaximumPlayers> lastExecutedSequence_{};
    std::array<bool, kMaximumPlayers> hasExecutedSequence_{};
    mutable std::map<std::size_t, PathFieldCacheEntry> pathFieldCache_{};
};

}  // namespace echoes::sim
