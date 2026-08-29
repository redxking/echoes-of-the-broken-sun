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
constexpr std::uint32_t kMaximumSerializedCommands = 256U * 1024U;
constexpr std::int32_t kGuardLeashRaw = 6 * kFixedScale;
constexpr std::int32_t kGuardFollowRaw = 2 * kFixedScale;
constexpr std::uint32_t kMaximumTicksPerSecond = 1000;
constexpr Tick kMaximumSupportedTick = std::numeric_limits<Tick>::max() / 2;
constexpr std::int32_t kMaximumVisionTiles = 256;
constexpr std::int32_t kMaximumProductionTicks = 60 * 1000;
constexpr std::size_t kSerializedEntityBytes = 114;
constexpr std::size_t kSerializedCommandBytes = 36;
constexpr std::size_t kSnapshotFixedBytesAfterConfig = 80;
constexpr std::int32_t kMaximumMapDimension =
    std::numeric_limits<std::int32_t>::max() / kFixedScale;

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
                    lhs.wellChoice) <
           std::tie(rhs.executeTick,
                    rhs.player,
                    rhs.sequence,
                    rhs.type,
                    rhs.actor,
                    rhs.target,
                    rhs.position.x,
                    rhs.position.y,
                    rhs.buildType,
                    rhs.wellChoice);
}

[[nodiscard]] bool HasSameCommandKey(const Command& lhs, const Command& rhs) {
    return lhs.player == rhs.player && lhs.sequence == rhs.sequence;
}

[[nodiscard]] bool IsValidFaction(Faction faction) {
    return faction == Faction::MeridianCompact ||
           faction == Faction::KharuunAssemblies;
}

[[nodiscard]] bool IsValidEntityType(EntityType type) {
    return type >= EntityType::Worker && type <= EntityType::FutureWell;
}

[[nodiscard]] bool IsValidTerrain(Terrain terrain) {
    return terrain >= Terrain::Open && terrain <= Terrain::Scarred;
}

[[nodiscard]] bool IsValidCommandType(CommandType type) {
    return type >= CommandType::Stop && type <= CommandType::Guard;
}

[[nodiscard]] bool IsValidWellChoice(FutureWellChoice choice) {
    return choice >= FutureWellChoice::Dormant && choice <= FutureWellChoice::Reshape;
}

[[nodiscard]] bool IsValidAiPersonality(AiPersonality personality) {
    return personality >= AiPersonality::Balanced &&
           personality <= AiPersonality::Economic;
}

[[nodiscard]] std::int32_t SaturatingAdd(std::int32_t lhs, std::int32_t rhs) {
    const std::int64_t sum = static_cast<std::int64_t>(lhs) + rhs;
    return static_cast<std::int32_t>(std::clamp<std::int64_t>(
        sum, std::numeric_limits<std::int32_t>::min(),
        std::numeric_limits<std::int32_t>::max()));
}

class BinaryWriter final {
public:
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

void WriteCommand(BinaryWriter& writer, const Command& command) {
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
}

[[nodiscard]] bool ReadCommand(BinaryReader& reader, Command& command) {
    std::uint8_t type = 0;
    std::uint8_t buildType = 0;
    std::uint8_t wellChoice = 0;
    std::int32_t rawX = 0;
    std::int32_t rawY = 0;
    if (!reader.U64(command.executeTick) || !reader.U8(command.player) ||
        !reader.U64(command.sequence) || !reader.U8(type) ||
        !reader.U32(command.actor) || !reader.U32(command.target) ||
        !reader.I32(rawX) || !reader.I32(rawY) || !reader.U8(buildType) ||
        !reader.U8(wellChoice)) {
        return false;
    }
    if (type > static_cast<std::uint8_t>(CommandType::Guard) ||
        buildType > static_cast<std::uint8_t>(EntityType::FutureWell) ||
        wellChoice > static_cast<std::uint8_t>(FutureWellChoice::Reshape)) {
        return false;
    }
    command.type = static_cast<CommandType>(type);
    command.position = Vec2::FromRaw(rawX, rawY);
    command.buildType = static_cast<EntityType>(buildType);
    command.wellChoice = static_cast<FutureWellChoice>(wellChoice);
    return true;
}

[[nodiscard]] bool ResourceCovers(const ResourcePool& available,
                                  const ResourcePool& cost) {
    return available.material >= cost.material &&
           available.dawnshards >= cost.dawnshards;
}

}  // namespace

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
        tileCount > kMaximumMapTiles) {
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
    switch (type) {
        case EntityType::Worker:
            entity.maxHitPoints = faction == Faction::MeridianCompact ? 80 : 70;
            entity.movementPerTickRaw =
                faction == Faction::MeridianCompact ? 128 : 160;
            entity.visionTiles = faction == Faction::MeridianCompact ? 5 : 6;
            entity.attackRangeRaw = kFixedScale;
            entity.attackDamage = faction == Faction::MeridianCompact ? 4 : 5;
            entity.attackPeriodTicks =
                faction == Faction::MeridianCompact ? 20 : 16;
            entity.workRate = faction == Faction::MeridianCompact ? 10 : 9;
            entity.cargoCapacity = faction == Faction::MeridianCompact ? 100 : 90;
            break;
        case EntityType::Soldier:
            entity.maxHitPoints = faction == Faction::MeridianCompact ? 120 : 105;
            entity.movementPerTickRaw =
                faction == Faction::MeridianCompact ? 112 : 176;
            entity.visionTiles = faction == Faction::MeridianCompact ? 6 : 7;
            entity.attackRangeRaw = faction == Faction::MeridianCompact
                                        ? 4 * kFixedScale
                                        : Fixed::FromRatio(3, 2).Raw();
            entity.attackDamage = faction == Faction::MeridianCompact ? 18 : 25;
            entity.attackPeriodTicks =
                faction == Faction::MeridianCompact ? 12 : 10;
            break;
        case EntityType::CommandCore:
            entity.maxHitPoints =
                faction == Faction::MeridianCompact ? 1000 : 850;
            entity.visionTiles = 8;
            entity.constructionRequired = 400;
            break;
        case EntityType::Dropoff:
            entity.maxHitPoints = faction == Faction::MeridianCompact ? 500 : 420;
            entity.visionTiles = 5;
            entity.constructionRequired = 100;
            break;
        case EntityType::Barracks:
            entity.maxHitPoints = faction == Faction::MeridianCompact ? 650 : 540;
            entity.visionTiles = 5;
            entity.constructionRequired = 160;
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
    if (!TryAllocateEntityId(entity.id)) {
        return 0;
    }
    entities_.push_back(entity);
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
    terrain_[static_cast<std::size_t>(tileY * config_.mapWidthTiles + tileX)] = terrain;
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

bool Simulation::IsBuilding(EntityType type) const {
    return type == EntityType::CommandCore || type == EntityType::Dropoff ||
           type == EntityType::Barracks;
}

bool Simulation::IsDropoff(EntityType type) const {
    return type == EntityType::CommandCore || type == EntityType::Dropoff;
}

std::int32_t Simulation::FootprintHalfExtentRaw(EntityType type) const {
    switch (type) {
        case EntityType::CommandCore:
        case EntityType::Barracks:
            return kFixedScale;
        case EntityType::Dropoff:
            return 3 * kFixedScale / 4;
        case EntityType::FutureWell:
            return kFixedScale / 2;
        case EntityType::ResourceNode:
            return kFixedScale / 3;
        case EntityType::Worker:
        case EntityType::Soldier:
            return kFixedScale / 8;
    }
    return kFixedScale;
}

ResourcePool Simulation::BuildCost(Faction faction, EntityType type) const {
    const bool meridian = faction == Faction::MeridianCompact;
    switch (type) {
        case EntityType::CommandCore:
            return {meridian ? 420 : 380, meridian ? 40 : 60};
        case EntityType::Dropoff:
            return {meridian ? 110 : 95, 0};
        case EntityType::Barracks:
            return {meridian ? 170 : 150, meridian ? 20 : 30};
        default:
            return {};
    }
}

ResourcePool Simulation::ProductionCost(Faction faction, EntityType type) const {
    const bool meridian = faction == Faction::MeridianCompact;
    switch (type) {
        case EntityType::Worker:
            return {50, 0};
        case EntityType::Soldier:
            return {meridian ? 85 : 75, meridian ? 20 : 30};
        default:
            return {};
    }
}

std::int32_t Simulation::ProductionTicks(EntityType type) const {
    switch (type) {
        case EntityType::Worker:
            return static_cast<std::int32_t>(config_.ticksPerSecond * 3U);
        case EntityType::Soldier:
            return static_cast<std::int32_t>(config_.ticksPerSecond * 5U);
        default:
            return 0;
    }
}

std::int32_t Simulation::PopulationCost(EntityType type) const {
    switch (type) {
        case EntityType::Worker:
            return 1;
        case EntityType::Soldier:
            return 2;
        default:
            return 0;
    }
}

std::int32_t Simulation::PopulationUsed(PlayerId player) const {
    if (FindPlayer(player) == nullptr) {
        return 0;
    }
    std::int32_t used = 0;
    for (const Entity& entity : entities_) {
        if (entity.owner == player && entity.hitPoints > 0 && entity.completed) {
            used = SaturatingAdd(used, PopulationCost(entity.type));
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
        if (entity.type == EntityType::CommandCore) {
            capacity = SaturatingAdd(capacity, 12);
        } else if (entity.type == EntityType::Dropoff) {
            capacity = SaturatingAdd(
                capacity,
                entity.faction == Faction::MeridianCompact ? 6 : 5);
        }
    }
    return capacity;
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
    const bool supported =
        (building->type == EntityType::CommandCore &&
         unitType == EntityType::Worker) ||
        (building->type == EntityType::Barracks &&
         unitType == EntityType::Soldier);
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
                PopulationCost(entity.productionType));
        }
    }
    if (SaturatingAdd(committedPopulation, PopulationCost(unitType)) >
        PopulationCapacity(player)) {
        return ProductionResult::CapacityReached;
    }
    if (entities_.size() >= kMaximumSerializedEntities || nextEntityId_ == 0 ||
        nextEntityId_ == std::numeric_limits<EntityId>::max()) {
        return ProductionResult::EntityCapacityReached;
    }
    return ProductionResult::Valid;
}

MatchOutcome Simulation::Outcome() const {
    if (!players_[0].active || !players_[1].active) {
        return MatchOutcome::Ongoing;
    }
    std::array<bool, 2> hasCommandCore{};
    for (const Entity& entity : entities_) {
        if (entity.owner < hasCommandCore.size() && entity.hitPoints > 0 &&
            entity.type == EntityType::CommandCore) {
            hasCommandCore[entity.owner] = true;
        }
    }
    if (hasCommandCore[0] && hasCommandCore[1]) {
        return MatchOutcome::Ongoing;
    }
    if (hasCommandCore[0]) {
        return MatchOutcome::Player0Victory;
    }
    if (hasCommandCore[1]) {
        return MatchOutcome::Player1Victory;
    }
    return MatchOutcome::Draw;
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
    const std::int32_t halfExtent = FootprintHalfExtentRaw(buildingType);
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
            halfExtent + FootprintHalfExtentRaw(entity.type);
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
        !IsValidWellChoice(command.wellChoice) || command.actor == 0) {
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
    const std::int64_t deltaX =
        static_cast<std::int64_t>(first.x.Raw()) - second.x.Raw();
    const std::int64_t deltaY =
        static_cast<std::int64_t>(first.y.Raw()) - second.y.Raw();
    return static_cast<std::uint64_t>(deltaX * deltaX + deltaY * deltaY);
}

bool Simulation::InInteractionRange(const Entity& first,
                                    const Entity& second,
                                    std::int32_t extraRangeRaw) const {
    const std::int64_t range = static_cast<std::int64_t>(extraRangeRaw) +
                               FootprintHalfExtentRaw(first.type) +
                               FootprintHalfExtentRaw(second.type);
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
    std::vector<std::size_t> parent(tileCount, kUnvisited);
    std::vector<std::size_t> frontier{};
    frontier.reserve(std::min<std::size_t>(tileCount, 4096));
    parent[start] = start;
    frontier.push_back(start);

    // Fixed direction order is part of deterministic equal-cost path selection.
    constexpr std::array<std::array<std::int32_t, 2>, 4> directions{{
        {{0, -1}},
        {{1, 0}},
        {{0, 1}},
        {{-1, 0}},
    }};
    std::size_t head = 0;
    while (head < frontier.size() && parent[goal] == kUnvisited) {
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
            if (parent[next] != kUnvisited ||
                !IsPositionPassable(Vec2::FromTiles(nextX, nextY))) {
                continue;
            }
            parent[next] = current;
            frontier.push_back(next);
        }
    }
    if (parent[goal] == kUnvisited) {
        return std::nullopt;
    }

    std::size_t next = goal;
    while (parent[next] != start) {
        next = parent[next];
    }
    const std::int32_t nextX = static_cast<std::int32_t>(next % width);
    const std::int32_t nextY = static_cast<std::int32_t>(next / width);
    return Vec2::FromTiles(nextX, nextY);
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
    const std::int64_t travel =
        std::min<std::int64_t>(entity.movementPerTickRaw, distance);
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
    entity.position = candidate;
    return entity.position == destination;
}

EntityId Simulation::FindNearestOwnedDropoff(PlayerId player, Vec2 from) const {
    EntityId nearest = 0;
    std::uint64_t nearestDistance = std::numeric_limits<std::uint64_t>::max();
    for (const Entity& entity : entities_) {
        if (entity.owner != player || !entity.completed || !IsDropoff(entity.type)) {
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

EntityId Simulation::FindNearestVisibleEnemy(PlayerId player,
                                             Vec2 from,
                                             std::int32_t radiusRaw) const {
    EntityId nearest = 0;
    std::uint64_t nearestDistance = std::numeric_limits<std::uint64_t>::max();
    const std::uint64_t radiusSquared =
        static_cast<std::uint64_t>(radiusRaw) * radiusRaw;
    for (const Entity& entity : entities_) {
        if (entity.owner == kNeutralPlayer || entity.owner == player ||
            entity.hitPoints <= 0 || !IsEntityVisibleTo(player, entity.id)) {
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
            entity.hitPoints <= 0 ||
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
                        FootprintHalfExtentRaw(entity.type) + kFixedScale / 8;
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
        ApplyCommand(command);
        hasExecutedSequence_[command.player] = true;
        lastExecutedSequence_[command.player] = command.sequence;
    }
}

void Simulation::ApplyCommand(const Command& command) {
    Entity* actor = MutableEntity(command.actor);
    if (actor == nullptr || actor->owner != command.player || !actor->completed ||
        actor->hitPoints <= 0) {
        return;
    }
    switch (command.type) {
        case CommandType::Stop:
            actor->order = {};
            return;
        case CommandType::Move:
            if (actor->movementPerTickRaw > 0 && IsInsideMap(command.position)) {
                actor->order.type = OrderType::Move;
                actor->order.target = 0;
                actor->order.destination = command.position;
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
            }
            return;
        }
        case CommandType::Deliver: {
            const Entity* target = FindEntity(command.target);
            if (actor->type == EntityType::Worker && target != nullptr &&
                target->owner == command.player && target->completed &&
                IsDropoff(target->type)) {
                actor->order.type = OrderType::Deliver;
                actor->order.target = target->id;
                actor->order.destination = target->position;
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
            return;
        }
        case CommandType::Attack: {
            const Entity* target = FindEntity(command.target);
            if (actor->attackDamage > 0 && target != nullptr &&
                target->owner != kNeutralPlayer && target->owner != command.player &&
                IsEntityVisibleTo(command.player, target->id)) {
                actor->order.type = OrderType::Attack;
                actor->order.target = target->id;
                actor->order.destination = target->position;
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
            actor->productionRequired = ProductionTicks(command.buildType);
            return;
        }
        case CommandType::AttackMove:
            if (actor->attackDamage > 0 && actor->movementPerTickRaw > 0 &&
                IsPositionPassable(command.position)) {
                actor->order.type = OrderType::AttackMove;
                actor->order.target = 0;
                actor->order.destination = command.position;
            }
            return;
        case CommandType::Hold:
            if (actor->attackDamage > 0) {
                actor->order.type = OrderType::Hold;
                actor->order.target = 0;
                actor->order.destination = actor->position;
            }
            return;
        case CommandType::Guard: {
            const Entity* guarded = FindEntity(command.target);
            if (actor->attackDamage > 0 && guarded != nullptr &&
                guarded->owner == command.player && guarded->id != actor->id) {
                actor->order.type = OrderType::Guard;
                actor->order.target = guarded->id;
                actor->order.destination = guarded->position;
            }
            return;
        }
    }
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
        !IsDropoff(dropoff->type)) {
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
        worker.order = {};
    }
}

void Simulation::ProcessAttack(
    Entity& attacker,
    std::vector<std::pair<EntityId, std::int32_t>>& pendingDamage) {
    Entity* target = MutableEntity(attacker.order.target);
    if (target == nullptr || target->owner == kNeutralPlayer ||
        target->owner == attacker.owner ||
        !IsEntityVisibleTo(attacker.owner, target->id)) {
        attacker.order = {};
        return;
    }
    if (!InInteractionRange(attacker, *target, attacker.attackRangeRaw)) {
        (void)MoveTowards(attacker, target->position);
        return;
    }
    if (attacker.attackCooldownTicks == 0) {
        pendingDamage.emplace_back(target->id, attacker.attackDamage);
        attacker.attackCooldownTicks = attacker.attackPeriodTicks;
    }
}

void Simulation::ProcessAttackMove(
    Entity& attacker,
    std::vector<std::pair<EntityId, std::int32_t>>& pendingDamage) {
    if (attacker.attackDamage <= 0 || attacker.movementPerTickRaw <= 0) {
        attacker.order = {};
        return;
    }

    Entity* target = attacker.order.target != 0
                         ? MutableEntity(attacker.order.target)
                         : nullptr;
    if (target == nullptr || target->owner == kNeutralPlayer ||
        target->owner == attacker.owner ||
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
            pendingDamage.emplace_back(target->id, attacker.attackDamage);
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
    std::vector<std::pair<EntityId, std::int32_t>>& pendingDamage) {
    if (attacker.attackDamage <= 0) {
        attacker.order = {};
        return;
    }

    Entity* target = attacker.order.target != 0
                         ? MutableEntity(attacker.order.target)
                         : nullptr;
    if (target == nullptr || target->owner == kNeutralPlayer ||
        target->owner == attacker.owner ||
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
        pendingDamage.emplace_back(target->id, attacker.attackDamage);
        attacker.attackCooldownTicks = attacker.attackPeriodTicks;
    }
}

void Simulation::ProcessGuard(
    Entity& attacker,
    std::vector<std::pair<EntityId, std::int32_t>>& pendingDamage) {
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
            pendingDamage.emplace_back(enemy->id, attacker.attackDamage);
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
                SaturatingAdd(player->resources.dawnshards, 300);
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
            if (player->resources.dawnshards < 100) {
                worker.order = {};
                return;
            }
            player->resources.dawnshards -= 100;
            well->owner = worker.owner;
            well->faction = worker.faction;
            well->wellChoice = FutureWellChoice::Reshape;
            well->reshapeVariant = static_cast<std::uint8_t>(rng_.Uniform(4));
            well->reshapeUntilTick = currentTick_ + 40 + rng_.Uniform(21);
            break;
        case FutureWellChoice::Dormant:
            worker.order = {};
            return;
    }
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

void Simulation::ProcessEntityOrders() {
    std::vector<std::pair<EntityId, std::int32_t>> pendingDamage{};
    for (Entity& entity : entities_) {
        if (entity.hitPoints <= 0 || !entity.completed) {
            continue;
        }
        if (entity.attackCooldownTicks > 0) {
            --entity.attackCooldownTicks;
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
        }
    }
    std::sort(pendingDamage.begin(), pendingDamage.end());
    std::size_t index = 0;
    while (index < pendingDamage.size()) {
        const EntityId targetId = pendingDamage[index].first;
        std::int64_t totalDamage = 0;
        while (index < pendingDamage.size() && pendingDamage[index].first == targetId) {
            totalDamage += pendingDamage[index].second;
            ++index;
        }
        if (Entity* target = MutableEntity(targetId); target != nullptr) {
            target->hitPoints -= static_cast<std::int32_t>(std::min<std::int64_t>(
                totalDamage, std::numeric_limits<std::int32_t>::max()));
        }
    }
}

void Simulation::ApplyPreserveIncome() {
    if ((currentTick_ + 1) % 10 != 0) {
        return;
    }
    for (const Entity& entity : entities_) {
        if (entity.type == EntityType::FutureWell &&
            entity.wellChoice == FutureWellChoice::Preserve) {
            if (PlayerState* player = MutablePlayer(entity.owner); player != nullptr) {
                player->resources.dawnshards =
                    SaturatingAdd(player->resources.dawnshards, 3);
            }
        }
    }
}

void Simulation::RemoveDestroyedEntities() {
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
                if (FindEntity(entity.order.target) == nullptr) {
                    entity.order = {};
                }
                break;
            case OrderType::AttackMove:
                if (entity.order.target != 0 &&
                    FindEntity(entity.order.target) == nullptr) {
                    entity.order.target = 0;
                }
                break;
            case OrderType::Hold:
                if (entity.order.target != 0 &&
                    FindEntity(entity.order.target) == nullptr) {
                    entity.order.target = 0;
                }
                break;
            case OrderType::Guard:
                if (FindEntity(entity.order.target) == nullptr) {
                    entity.order = {};
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

void Simulation::Step() {
    if (currentTick_ >= kMaximumSupportedTick) {
        return;
    }
    UpdateVisibility();
    ProcessCommandsForCurrentTick();
    ProcessEntityOrders();
    ProcessProduction();
    ApplyPreserveIncome();
    RemoveDestroyedEntities();
    ClearInvalidOrders();
    ++currentTick_;
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
                markVisible(entity.owner, entity.position, 8);
            }
        }
    }
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

std::uint64_t Simulation::StatelessAiValue(PlayerId player,
                                           EntityId entity,
                                           std::uint64_t salt) const {
    std::uint64_t value = config_.randomSeed ^
                          (currentTick_ * 0x9e3779b97f4a7c15ULL) ^
                          (static_cast<std::uint64_t>(player) << 56U) ^
                          (static_cast<std::uint64_t>(entity) << 17U) ^ salt;
    value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31U);
}

std::vector<Command> Simulation::GenerateAiCommands(PlayerId player,
                                                    AiPersonality personality) const {
    std::vector<Command> commands{};
    if (FindPlayer(player) == nullptr || !IsValidAiPersonality(personality)) {
        return commands;
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
        if (actor.type == EntityType::CommandCore ||
            actor.type == EntityType::Barracks) {
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
            actor.type != EntityType::Soldier) {
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
            const Entity* nearestEnemy = nullptr;
            std::uint64_t nearestDistance = std::numeric_limits<std::uint64_t>::max();
            for (const Entity& candidate : entities_) {
                if (candidate.owner == kNeutralPlayer || candidate.owner == player ||
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

std::vector<std::uint8_t> Simulation::SaveSnapshot() const {
    BinaryWriter writer{};
    writer.U8('E');
    writer.U8('B');
    writer.U8('S');
    writer.U8('N');
    writer.U32(kSnapshotVersion);
    writer.I32(config_.mapWidthTiles);
    writer.I32(config_.mapHeightTiles);
    writer.U32(config_.ticksPerSecond);
    writer.U64(config_.randomSeed);
    writer.U64(currentTick_);
    writer.U32(nextEntityId_);
    writer.U64(rng_.state);
    for (const PlayerState& player : players_) {
        writer.U8(player.active ? 1 : 0);
        writer.U8(player.id);
        writer.U8(static_cast<std::uint8_t>(player.faction));
        writer.I32(player.resources.material);
        writer.I32(player.resources.dawnshards);
    }
    for (PlayerId player = 0; player < players_.size(); ++player) {
        writer.U8(hasExecutedSequence_[player] ? 1 : 0);
        writer.U64(lastExecutedSequence_[player]);
    }
    writer.U32(static_cast<std::uint32_t>(terrain_.size()));
    for (const Terrain terrain : terrain_) {
        writer.U8(static_cast<std::uint8_t>(terrain));
    }
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
        writer.I32(entity.order.destination.x.Raw());
        writer.I32(entity.order.destination.y.Raw());
        writer.U8(static_cast<std::uint8_t>(entity.order.buildType));
        writer.U8(static_cast<std::uint8_t>(entity.order.wellChoice));
        writer.U8(static_cast<std::uint8_t>(entity.wellChoice));
        writer.U64(entity.reshapeUntilTick);
        writer.U8(entity.reshapeVariant);
        writer.U8(static_cast<std::uint8_t>(entity.productionType));
        writer.I32(entity.productionProgress);
        writer.I32(entity.productionRequired);
    }
    std::vector<Command> pending = pendingCommands_;
    std::sort(pending.begin(), pending.end(), CommandLess);
    writer.U32(static_cast<std::uint32_t>(pending.size()));
    for (const Command& command : pending) {
        WriteCommand(writer, command);
    }
    const std::uint64_t integrity = Fnv1a(writer.Data());
    writer.U64(integrity);
    return writer.Take();
}

std::uint64_t Simulation::StateChecksum() const {
    const std::vector<std::uint8_t> snapshot = SaveSnapshot();
    return Fnv1a(snapshot);
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
    if (!reader.U32(version)) {
        SetError(error, "snapshot header is truncated");
        return std::nullopt;
    }
    if (version != kSnapshotVersion) {
        SetError(error, "snapshot version is unsupported");
        return std::nullopt;
    }
    if (!reader.I32(config.mapWidthTiles) || !reader.I32(config.mapHeightTiles) ||
        !reader.U32(config.ticksPerSecond) || !reader.U64(config.randomSeed)) {
        SetError(error, "snapshot header is truncated");
        return std::nullopt;
    }
    const std::int64_t tileCount =
        static_cast<std::int64_t>(config.mapWidthTiles) * config.mapHeightTiles;
    if (config.mapWidthTiles <= 0 || config.mapHeightTiles <= 0 ||
        config.mapWidthTiles > kMaximumMapDimension ||
        config.mapHeightTiles > kMaximumMapDimension ||
        config.ticksPerSecond == 0 ||
        config.ticksPerSecond > kMaximumTicksPerSecond || tileCount <= 0 ||
        tileCount > kMaximumMapTiles) {
        SetError(error, "snapshot map configuration is invalid");
        return std::nullopt;
    }
    const std::size_t minimumRemaining =
        kSnapshotFixedBytesAfterConfig +
        static_cast<std::size_t>(tileCount) * 3U;
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
        PlayerState player{};
        if (!reader.U8(active) || !reader.U8(id) || !reader.U8(faction) ||
            !reader.I32(player.resources.material) ||
            !reader.I32(player.resources.dawnshards) || id != index || active > 1 ||
            faction > static_cast<std::uint8_t>(Faction::KharuunAssemblies) ||
            player.resources.material < 0 || player.resources.dawnshards < 0) {
            SetError(error, "snapshot player state is invalid");
            return std::nullopt;
        }
        player.id = id;
        player.active = active != 0;
        player.faction = static_cast<Faction>(faction);
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
    std::uint32_t entityCount = 0;
    if (!reader.U32(entityCount) || entityCount > kMaximumSerializedEntities ||
        static_cast<std::size_t>(entityCount) >
            reader.Remaining() / kSerializedEntityBytes) {
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
        std::int32_t rawX = 0;
        std::int32_t rawY = 0;
        std::int32_t orderRawX = 0;
        std::int32_t orderRawY = 0;
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
            !reader.U32(entity.order.target) || !reader.I32(orderRawX) ||
            !reader.I32(orderRawY) || !reader.U8(orderBuildType) ||
            !reader.U8(orderWellChoice) || !reader.U8(wellChoice) ||
            !reader.U64(entity.reshapeUntilTick) ||
            !reader.U8(entity.reshapeVariant) || !reader.U8(productionType) ||
            !reader.I32(entity.productionProgress) ||
            !reader.I32(entity.productionRequired)) {
            SetError(error, "snapshot entity data is truncated");
            return std::nullopt;
        }
        if (entity.id == 0 || entity.id <= priorId ||
            faction > static_cast<std::uint8_t>(Faction::KharuunAssemblies) ||
            type > static_cast<std::uint8_t>(EntityType::FutureWell) ||
            completed > 1 ||
            orderType > static_cast<std::uint8_t>(OrderType::Guard) ||
            orderBuildType > static_cast<std::uint8_t>(EntityType::FutureWell) ||
            orderWellChoice > static_cast<std::uint8_t>(FutureWellChoice::Reshape) ||
            wellChoice > static_cast<std::uint8_t>(FutureWellChoice::Reshape) ||
            entity.reshapeVariant > 3 ||
            productionType > static_cast<std::uint8_t>(EntityType::FutureWell) ||
            (entity.owner != kNeutralPlayer &&
             (entity.owner >= simulation.players_.size() ||
              !simulation.players_[entity.owner].active)) ||
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
                productionType == static_cast<std::uint8_t>(EntityType::Soldier)))) ||
            entity.reshapeUntilTick > kMaximumSupportedTick ||
            (wellChoice != static_cast<std::uint8_t>(FutureWellChoice::Reshape) &&
             entity.reshapeUntilTick != 0) ||
            (wellChoice == static_cast<std::uint8_t>(FutureWellChoice::Reshape) &&
             entity.reshapeUntilTick != 0 &&
             entity.reshapeUntilTick <= simulation.currentTick_)) {
            SetError(error, "snapshot entity state is invalid");
            return std::nullopt;
        }
        entity.faction = static_cast<Faction>(faction);
        entity.type = static_cast<EntityType>(type);
        entity.position = Vec2::FromRaw(rawX, rawY);
        entity.completed = completed != 0;
        entity.order.type = static_cast<OrderType>(orderType);
        entity.order.destination = Vec2::FromRaw(orderRawX, orderRawY);
        entity.order.buildType = static_cast<EntityType>(orderBuildType);
        entity.order.wellChoice = static_cast<FutureWellChoice>(orderWellChoice);
        entity.wellChoice = static_cast<FutureWellChoice>(wellChoice);
        entity.productionType = static_cast<EntityType>(productionType);
        if (!simulation.IsInsideMap(entity.position)) {
            SetError(error, "snapshot entity is outside the map");
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
            command.executeTick > kMaximumSupportedTick || command.actor == 0) {
            SetError(error, "snapshot pending command is invalid");
            return std::nullopt;
        }
    }
    std::sort(simulation.pendingCommands_.begin(),
              simulation.pendingCommands_.end(), CommandLess);
    std::array<bool, 2> sawPendingSequence{};
    std::array<Tick, 2> lastPendingTick{};
    std::array<std::uint64_t, 2> lastPendingSequence{};
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

ReplayRecord Simulation::ExportReplay() const {
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
