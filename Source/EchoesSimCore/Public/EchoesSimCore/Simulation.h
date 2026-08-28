#pragma once

#if defined(__has_include)
#if __has_include("HAL/Platform.h")
#include "HAL/Platform.h"
#endif
#endif

#include <array>
#include <compare>
#include <cstdint>
#include <limits>
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
inline constexpr std::int32_t kFixedScale = 1024;
inline constexpr std::uint32_t kSnapshotVersion = 3;
inline constexpr std::uint32_t kReplayVersion = 3;

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
};

enum class EntityType : std::uint8_t {
    Worker = 0,
    Soldier = 1,
    CommandCore = 2,
    Dropoff = 3,
    Barracks = 4,
    ResourceNode = 5,
    FutureWell = 6,
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

enum class MatchOutcome : std::uint8_t {
    Ongoing = 0,
    Player0Victory = 1,
    Player1Victory = 2,
    Draw = 3,
};

enum class AiPersonality : std::uint8_t {
    Balanced = 0,
    Defensive = 1,
    Raider = 2,
    Economic = 3,
};

struct ResourcePool final {
    std::int32_t material = 0;
    std::int32_t dawnshards = 0;

    friend bool operator==(const ResourcePool&, const ResourcePool&) = default;
};

struct PlayerState final {
    PlayerId id = 0;
    Faction faction = Faction::MeridianCompact;
    ResourcePool resources{};
    bool active = false;
};

struct Order final {
    OrderType type = OrderType::None;
    EntityId target = 0;
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
    Tick reshapeUntilTick = 0;
    std::uint8_t reshapeVariant = 0;
    EntityType productionType = EntityType::Worker;
    std::int32_t productionProgress = 0;
    std::int32_t productionRequired = 0;

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

    friend bool operator==(const Command&, const Command&) = default;
};

struct SimulationConfig final {
    std::int32_t mapWidthTiles = 64;
    std::int32_t mapHeightTiles = 64;
    std::uint32_t ticksPerSecond = 20;
    std::uint64_t randomSeed = 1;
};

struct ReplayRecord final {
    std::uint32_t version = kReplayVersion;
    std::vector<std::uint8_t> initialSnapshot{};
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

    bool AddPlayer(PlayerId player, Faction faction, ResourcePool startingResources);
    [[nodiscard]] const PlayerState* FindPlayer(PlayerId player) const;

    EntityId SpawnEntity(PlayerId owner,
                         Faction faction,
                         EntityType type,
                         Vec2 position);
    EntityId SpawnResourceNode(Vec2 position, std::int32_t amount);
    EntityId SpawnFutureWell(Vec2 position);
    [[nodiscard]] const Entity* FindEntity(EntityId id) const;

    bool SetTerrainTile(std::int32_t tileX, std::int32_t tileY, Terrain terrain);
    [[nodiscard]] Terrain TerrainAt(std::int32_t tileX,
                                    std::int32_t tileY) const;
    [[nodiscard]] bool IsPositionPassable(Vec2 position) const;
    [[nodiscard]] PlacementResult ValidatePlacement(PlayerId player,
                                                    EntityType buildingType,
                                                    Vec2 position,
                                                    EntityId* blockingEntity = nullptr) const;
    [[nodiscard]] ProductionResult ValidateProduction(
        PlayerId player,
        EntityId producer,
        EntityType unitType) const;
    [[nodiscard]] ResourcePool BuildCost(Faction faction, EntityType type) const;
    [[nodiscard]] ResourcePool ProductionCost(Faction faction,
                                               EntityType type) const;
    [[nodiscard]] std::int32_t PopulationUsed(PlayerId player) const;
    [[nodiscard]] std::int32_t PopulationCapacity(PlayerId player) const;
    [[nodiscard]] MatchOutcome Outcome() const;

    bool QueueCommand(const Command& command, std::string* rejectionReason = nullptr);
    void Step();
    void Step(Tick tickCount);

    [[nodiscard]] Visibility VisibilityAt(PlayerId player, Vec2 position) const;
    [[nodiscard]] bool IsEntityVisibleTo(PlayerId player, EntityId entity) const;

    // AI only examines the same visibility state exposed to a human player.
    [[nodiscard]] std::vector<Command> GenerateAiCommands(
        PlayerId player,
        AiPersonality personality = AiPersonality::Balanced) const;

    [[nodiscard]] std::uint64_t StateChecksum() const;
    [[nodiscard]] std::vector<std::uint8_t> SaveSnapshot() const;
    [[nodiscard]] static std::optional<Simulation> LoadSnapshot(
        std::span<const std::uint8_t> bytes,
        std::string* error = nullptr);

    // Capture after deterministic map/scenario setup and before player commands.
    void CaptureReplayBaseline();
    [[nodiscard]] ReplayRecord ExportReplay() const;
    [[nodiscard]] static std::optional<Simulation> ReplayToEnd(
        const ReplayRecord& replay,
        std::string* error = nullptr);

private:
    struct DeterministicRng final {
        explicit DeterministicRng(std::uint64_t seed = 1) : state(seed) {}
        [[nodiscard]] std::uint32_t NextU32();
        [[nodiscard]] std::uint32_t Uniform(std::uint32_t exclusiveUpperBound);
        std::uint64_t state = 1;
    };

    [[nodiscard]] bool IsInsideMap(Vec2 position,
                                   std::int32_t halfExtentRaw = 0) const;
    [[nodiscard]] PlayerState* MutablePlayer(PlayerId player);
    [[nodiscard]] Entity* MutableEntity(EntityId id);
    [[nodiscard]] bool IsBuilding(EntityType type) const;
    [[nodiscard]] bool IsDropoff(EntityType type) const;
    [[nodiscard]] std::int32_t FootprintHalfExtentRaw(EntityType type) const;
    [[nodiscard]] Entity MakeEntity(PlayerId owner,
                                    Faction faction,
                                    EntityType type,
                                    Vec2 position) const;
    [[nodiscard]] std::int32_t ProductionTicks(EntityType type) const;
    [[nodiscard]] std::int32_t PopulationCost(EntityType type) const;
    [[nodiscard]] std::optional<Vec2> FindProductionSpawnPosition(
        const Entity& producer) const;
    [[nodiscard]] bool IsReshapedOpen(std::int32_t tileX,
                                      std::int32_t tileY) const;
    [[nodiscard]] bool InInteractionRange(const Entity& first,
                                          const Entity& second,
                                          std::int32_t extraRangeRaw) const;
    [[nodiscard]] bool MoveTowards(Entity& entity, Vec2 destination);
    [[nodiscard]] EntityId FindNearestOwnedDropoff(PlayerId player,
                                                   Vec2 from) const;
    [[nodiscard]] std::uint64_t DistanceSquaredRaw(Vec2 first, Vec2 second) const;
    [[nodiscard]] bool TryAllocateEntityId(EntityId& id);

    void UpdateVisibility();
    void ResolveExpiredReshapes();
    void ProcessCommandsForCurrentTick();
    void ApplyCommand(const Command& command);
    void ProcessEntityOrders();
    void ProcessGather(Entity& worker);
    void ProcessDeliver(Entity& worker);
    void ProcessBuild(Entity& worker);
    void ProcessAttack(Entity& attacker,
                       std::vector<std::pair<EntityId, std::int32_t>>& pendingDamage);
    void ProcessFutureWell(Entity& worker);
    void ProcessProduction();
    void ApplyPreserveIncome();
    void RemoveDestroyedEntities();
    void ClearInvalidOrders();

    [[nodiscard]] std::uint64_t StatelessAiValue(PlayerId player,
                                                 EntityId entity,
                                                 std::uint64_t salt) const;

    SimulationConfig config_{};
    Tick currentTick_ = 0;
    EntityId nextEntityId_ = 1;
    DeterministicRng rng_{};
    std::array<PlayerState, 2> players_{};
    std::vector<Terrain> terrain_{};
    std::array<std::vector<std::uint8_t>, 2> explored_{};
    std::array<std::vector<std::uint8_t>, 2> visible_{};
    std::vector<Entity> entities_{};
    std::vector<Command> pendingCommands_{};
    std::vector<Command> commandLog_{};
    std::vector<std::uint8_t> replayInitialSnapshot_{};
    std::array<std::uint64_t, 2> lastExecutedSequence_{};
    std::array<bool, 2> hasExecutedSequence_{};
};

}  // namespace echoes::sim
