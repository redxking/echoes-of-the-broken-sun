#include "EchoesSimCore/Simulation.h"
#include "EchoesSimCore/NetworkProtocol.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

using namespace echoes::sim;

class TestFailure final : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

#define REQUIRE(condition)                                                        \
    do {                                                                          \
        if (!(condition)) {                                                       \
            throw TestFailure(std::string(__FILE__) + ":" +                     \
                              std::to_string(__LINE__) + ": " #condition);       \
        }                                                                         \
    } while (false)

Command MakeCommand(Tick tick,
                    PlayerId player,
                    std::uint64_t sequence,
                    CommandType type,
                    EntityId actor) {
    Command command{};
    command.executeTick = tick;
    command.player = player;
    command.sequence = sequence;
    command.type = type;
    command.actor = actor;
    return command;
}

std::uint64_t SnapshotIntegrity(const std::vector<std::uint8_t>& bytes,
                                std::size_t length) {
    std::uint64_t hash = 14695981039346656037ULL;
    for (std::size_t index = 0; index < length; ++index) {
        hash ^= bytes[index];
        hash *= 1099511628211ULL;
    }
    return hash;
}

void WriteU32(std::vector<std::uint8_t>& bytes,
              std::size_t offset,
              std::uint32_t value) {
    for (std::uint32_t shift = 0; shift < 32; shift += 8) {
        bytes[offset++] = static_cast<std::uint8_t>(value >> shift);
    }
}

void WriteU64(std::vector<std::uint8_t>& bytes,
              std::size_t offset,
              std::uint64_t value) {
    for (std::uint32_t shift = 0; shift < 64; shift += 8) {
        bytes[offset++] = static_cast<std::uint8_t>(value >> shift);
    }
}

void ResignSnapshot(std::vector<std::uint8_t>& bytes) {
    REQUIRE(bytes.size() >= 8);
    WriteU64(bytes, bytes.size() - 8,
             SnapshotIntegrity(bytes, bytes.size() - 8));
}

std::uint32_t NetworkCrc32(const std::vector<std::uint8_t>& bytes,
                           std::size_t length) {
    std::uint32_t crc = 0xffffffffU;
    for (std::size_t index = 0; index < length; ++index) {
        crc ^= bytes[index];
        for (std::uint8_t bit = 0; bit < 8; ++bit) {
            const std::uint32_t mask =
                0U - static_cast<std::uint32_t>(crc & 1U);
            crc = (crc >> 1) ^ (0xedb88320U & mask);
        }
    }
    return ~crc;
}

void ResignNetworkPacket(std::vector<std::uint8_t>& bytes) {
    REQUIRE(bytes.size() >= 4);
    WriteU32(bytes, bytes.size() - 4,
             NetworkCrc32(bytes, bytes.size() - 4));
}

std::size_t SnapshotEntityCountOffset(std::size_t mapTileCount) {
    // Snapshot v20 header/rules/research/player/sequence fields plus terrain and four fog grids.
    return 1862 + 5 * mapTileCount;
}

std::size_t SnapshotFirstEntityOffset(std::size_t mapTileCount) {
    return SnapshotEntityCountOffset(mapTileCount) + 4;
}

void AddTwoPlayers(Simulation& simulation,
                   ResourcePool playerZero = {1000, 500},
                   ResourcePool playerOne = {1000, 500}) {
    REQUIRE(simulation.AddPlayer(0, Faction::MeridianCompact, playerZero));
    REQUIRE(simulation.AddPlayer(1, Faction::KharuunAssemblies, playerOne));
}

void TestFixedTickMovement() {
    Simulation simulation({16, 16, 20, 17});
    REQUIRE(simulation.AddPlayer(0, Faction::MeridianCompact, {0, 0}));
    const EntityId worker = simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Worker, Vec2::FromTiles(2, 2));
    REQUIRE(worker != 0);

    Command move = MakeCommand(0, 0, 1, CommandType::Move, worker);
    move.position = Vec2::FromTiles(4, 2);
    REQUIRE(simulation.QueueCommand(move));
    simulation.Step(16);

    const Entity* moved = simulation.FindEntity(worker);
    REQUIRE(moved != nullptr);
    REQUIRE(moved->position == Vec2::FromTiles(4, 2));
    REQUIRE(moved->order.type == OrderType::None);
    REQUIRE(simulation.CurrentTick() == 16);
    REQUIRE(Fixed::FromRatio(3, 2).Raw() == 1536);
}

struct DeterminismScenario final {
    Simulation simulation;
    EntityId worker = 0;
    EntityId well = 0;
    EntityId meridianSoldier = 0;
    EntityId kharuunSoldier = 0;
};

DeterminismScenario MakeDeterminismScenario() {
    DeterminismScenario scenario{Simulation({24, 24, 20, 0x12345678ULL})};
    AddTwoPlayers(scenario.simulation);
    scenario.worker = scenario.simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Worker, Vec2::FromTiles(3, 4));
    scenario.well = scenario.simulation.SpawnFutureWell(Vec2::FromTiles(4, 4));
    scenario.meridianSoldier = scenario.simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Soldier, Vec2::FromTiles(6, 6));
    scenario.kharuunSoldier = scenario.simulation.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::Soldier, Vec2::FromTiles(9, 6));
    REQUIRE(scenario.worker != 0 && scenario.well != 0 &&
            scenario.meridianSoldier != 0 && scenario.kharuunSoldier != 0);
    return scenario;
}

std::vector<Command> DeterminismCommands(const DeterminismScenario& scenario) {
    Command reshape = MakeCommand(0, 0, 20, CommandType::FutureWell,
                                  scenario.worker);
    reshape.target = scenario.well;
    reshape.wellChoice = FutureWellChoice::Reshape;

    Command meridianAttack = MakeCommand(0, 0, 40, CommandType::Attack,
                                         scenario.meridianSoldier);
    meridianAttack.target = scenario.kharuunSoldier;

    Command kharuunAttack = MakeCommand(0, 1, 10, CommandType::Attack,
                                        scenario.kharuunSoldier);
    kharuunAttack.target = scenario.meridianSoldier;

    Command lateMove = MakeCommand(55, 0, 60, CommandType::Move,
                                   scenario.worker);
    lateMove.position = Vec2::FromTiles(12, 12);
    return {reshape, meridianAttack, kharuunAttack, lateMove};
}

void TestCanonicalCommandOrderingAndDeterminism() {
    DeterminismScenario forward = MakeDeterminismScenario();
    DeterminismScenario reverse = MakeDeterminismScenario();
    const std::vector<Command> commands = DeterminismCommands(forward);
    for (const Command& command : commands) {
        REQUIRE(forward.simulation.QueueCommand(command));
    }
    for (auto command = commands.rbegin(); command != commands.rend(); ++command) {
        REQUIRE(reverse.simulation.QueueCommand(*command));
    }

    Command duplicate = commands.front();
    duplicate.type = CommandType::Stop;
    REQUIRE(!forward.simulation.QueueCommand(duplicate));

    for (int tick = 0; tick < 100; ++tick) {
        forward.simulation.Step();
        reverse.simulation.Step();
        REQUIRE(forward.simulation.StateChecksum() ==
                reverse.simulation.StateChecksum());
    }
    REQUIRE(forward.simulation.FindEntity(forward.well)->reshapeVariant ==
            reverse.simulation.FindEntity(reverse.well)->reshapeVariant);
}

void TestGatherDeliverBuildAndPlacement() {
    Simulation simulation({24, 24, 20, 99});
    REQUIRE(simulation.AddPlayer(0, Faction::MeridianCompact, {500, 100}));
    const EntityId base = simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::CommandCore,
        Vec2::FromTiles(2, 2));
    const EntityId worker = simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Worker, Vec2::FromTiles(4, 2));
    const EntityId resource =
        simulation.SpawnResourceNode(Vec2::FromTiles(5, 2), 500);
    REQUIRE(base != 0 && worker != 0 && resource != 0);

    REQUIRE(simulation.ValidatePlacement(0, EntityType::Barracks,
                                         Vec2::FromTiles(0, 0)) ==
            PlacementResult::OutsideMap);
    EntityId blocker = 0;
    REQUIRE(simulation.ValidatePlacement(0, EntityType::Barracks,
                                         Vec2::FromTiles(2, 2), &blocker) ==
            PlacementResult::Occupied);
    REQUIRE(blocker == base);
    REQUIRE(simulation.SetTerrainTile(12, 12, Terrain::Blocked));
    REQUIRE(simulation.ValidatePlacement(0, EntityType::Barracks,
                                         Vec2::FromTiles(12, 12)) ==
            PlacementResult::TerrainRestricted);

    Command gather = MakeCommand(0, 0, 1, CommandType::Gather, worker);
    gather.target = resource;
    REQUIRE(simulation.QueueCommand(gather));
    simulation.Step(20);
    REQUIRE(simulation.FindEntity(worker)->cargo == 100);

    const std::int32_t materialBeforeDelivery =
        simulation.FindPlayer(0)->resources.material;
    Command deliver =
        MakeCommand(simulation.CurrentTick(), 0, 2, CommandType::Deliver, worker);
    deliver.target = base;
    REQUIRE(simulation.QueueCommand(deliver));
    simulation.Step(30);
    REQUIRE(simulation.FindEntity(worker)->cargo == 0);
    REQUIRE(simulation.FindPlayer(0)->resources.material ==
            materialBeforeDelivery + 100);

    Command build =
        MakeCommand(simulation.CurrentTick(), 0, 3, CommandType::Build, worker);
    build.buildType = EntityType::Barracks;
    build.position = Vec2::FromTiles(8, 8);
    REQUIRE(simulation.QueueCommand(build));
    simulation.Step(140);

    const auto barracks = std::find_if(
        simulation.Entities().begin(), simulation.Entities().end(),
        [](const Entity& entity) {
            return entity.owner == 0 && entity.type == EntityType::Barracks;
        });
    REQUIRE(barracks != simulation.Entities().end());
    REQUIRE(barracks->completed);
    REQUIRE(barracks->hitPoints == barracks->maxHitPoints);
    REQUIRE(simulation.FindPlayer(0)->resources.material ==
            materialBeforeDelivery + 100 - 170);
    REQUIRE(simulation.FindPlayer(0)->resources.dawnshards == 80);
}

void TestCombatResolvesDeterministically() {
    Simulation simulation({20, 20, 20, 7});
    AddTwoPlayers(simulation, {0, 0}, {0, 0});
    const EntityId meridian = simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Soldier, Vec2::FromTiles(4, 4));
    const EntityId kharuun = simulation.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::Soldier,
        Vec2::FromTiles(7, 4));
    Command first = MakeCommand(0, 0, 1, CommandType::Attack, meridian);
    first.target = kharuun;
    Command second = MakeCommand(0, 1, 1, CommandType::Attack, kharuun);
    second.target = meridian;
    REQUIRE(simulation.QueueCommand(first));
    REQUIRE(simulation.QueueCommand(second));
    simulation.Step(100);

    REQUIRE(simulation.FindEntity(meridian) == nullptr);
    REQUIRE(simulation.FindEntity(kharuun) != nullptr);
    REQUIRE(simulation.FindEntity(kharuun)->hitPoints > 0);
    REQUIRE(simulation.FindEntity(kharuun)->hitPoints <
            simulation.FindEntity(kharuun)->maxHitPoints);
}

void TestAttackMoveAcquiresResumesAndStops() {
    Simulation simulation({24, 24, 20, 0x41545441434b4d56ULL});
    AddTwoPlayers(simulation, {0, 0}, {0, 0});
    const EntityId attacker = simulation.SpawnEntity(
        0,
        Faction::MeridianCompact,
        EntityType::Soldier,
        Vec2::FromTiles(2, 2));
    const EntityId hiddenDefender = simulation.SpawnEntity(
        1,
        Faction::KharuunAssemblies,
        EntityType::Soldier,
        Vec2::FromTiles(12, 2));
    REQUIRE(attacker != 0 && hiddenDefender != 0);
    REQUIRE(!simulation.IsEntityVisibleTo(0, hiddenDefender));

    Command advance =
        MakeCommand(0, 0, 1, CommandType::AttackMove, attacker);
    advance.position = Vec2::FromTiles(18, 2);
    REQUIRE(simulation.QueueCommand(advance));
    simulation.Step(320);

    const Entity* advanced = simulation.FindEntity(attacker);
    REQUIRE(advanced != nullptr);
    REQUIRE(simulation.FindEntity(hiddenDefender) == nullptr);
    REQUIRE(advanced->position == Vec2::FromTiles(18, 2));
    REQUIRE(advanced->order.type == OrderType::None);

    Command secondAdvance = MakeCommand(
        simulation.CurrentTick(), 0, 2, CommandType::AttackMove, attacker);
    secondAdvance.position = Vec2::FromTiles(18, 18);
    REQUIRE(simulation.QueueCommand(secondAdvance));
    simulation.Step(4);
    const Vec2 stoppedPosition = simulation.FindEntity(attacker)->position;
    REQUIRE(stoppedPosition != Vec2::FromTiles(18, 18));

    Command stop =
        MakeCommand(simulation.CurrentTick(), 0, 3, CommandType::Stop, attacker);
    REQUIRE(simulation.QueueCommand(stop));
    simulation.Step();
    REQUIRE(simulation.FindEntity(attacker)->order.type == OrderType::None);
    REQUIRE(simulation.FindEntity(attacker)->position == stoppedPosition);
    simulation.Step(20);
    REQUIRE(simulation.FindEntity(attacker)->position == stoppedPosition);
}

void TestHoldPositionDefendsWithoutChasing() {
    Simulation simulation({24, 24, 20, 0x484f4c44504f534eULL});
    AddTwoPlayers(simulation, {0, 0}, {0, 0});
    const EntityId defender = simulation.SpawnEntity(
        0,
        Faction::MeridianCompact,
        EntityType::Soldier,
        Vec2::FromTiles(4, 4));
    const EntityId retreatingEnemy = simulation.SpawnEntity(
        1,
        Faction::KharuunAssemblies,
        EntityType::Soldier,
        Vec2::FromTiles(7, 4));
    REQUIRE(defender != 0 && retreatingEnemy != 0);
    const Vec2 anchor = simulation.FindEntity(defender)->position;

    Command hold = MakeCommand(0, 0, 1, CommandType::Hold, defender);
    Command retreat = MakeCommand(0, 1, 1, CommandType::Move, retreatingEnemy);
    retreat.position = Vec2::FromTiles(18, 4);
    REQUIRE(simulation.QueueCommand(hold));
    REQUIRE(simulation.QueueCommand(retreat));
    simulation.Step(60);

    const Entity* held = simulation.FindEntity(defender);
    const Entity* retreated = simulation.FindEntity(retreatingEnemy);
    REQUIRE(held != nullptr && retreated != nullptr);
    REQUIRE(held->position == anchor);
    REQUIRE(held->order.type == OrderType::Hold);
    REQUIRE(held->order.target == 0);
    REQUIRE(retreated->hitPoints < retreated->maxHitPoints);
    REQUIRE(retreated->position != Vec2::FromTiles(7, 4));

    const EntityId closeEnemy = simulation.SpawnEntity(
        1,
        Faction::KharuunAssemblies,
        EntityType::Soldier,
        Vec2::FromTiles(6, 4));
    REQUIRE(closeEnemy != 0);
    simulation.Step(120);
    REQUIRE(simulation.FindEntity(closeEnemy) == nullptr);
    held = simulation.FindEntity(defender);
    REQUIRE(held != nullptr);
    REQUIRE(held->position == anchor);
    REQUIRE(held->order.type == OrderType::Hold);

    const std::vector<std::uint8_t> snapshot = simulation.SaveSnapshot();
    std::string error;
    std::optional<Simulation> restored =
        Simulation::LoadSnapshot(snapshot, &error);
    REQUIRE(restored.has_value());
    REQUIRE(error.empty());
    REQUIRE(restored->StateChecksum() == simulation.StateChecksum());
    REQUIRE(restored->FindEntity(defender)->order.type == OrderType::Hold);
    REQUIRE(restored->FindEntity(defender)->position == anchor);

    Command stop = MakeCommand(
        simulation.CurrentTick(), 0, 2, CommandType::Stop, defender);
    REQUIRE(simulation.QueueCommand(stop));
    simulation.Step();
    REQUIRE(simulation.FindEntity(defender)->order.type == OrderType::None);
    REQUIRE(simulation.FindEntity(defender)->position == anchor);
}

void TestGuardDefendsAndFollowsOwnedTarget() {
    Simulation simulation({30, 20, 20, 0x47554152444f5244ULL});
    AddTwoPlayers(simulation, {0, 0}, {0, 0});
    const EntityId guard = simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Soldier,
        Vec2::FromTiles(4, 4));
    const EntityId protectedWorker = simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Worker,
        Vec2::FromTiles(6, 4));
    const EntityId nearbyEnemy = simulation.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::Soldier,
        Vec2::FromTiles(10, 4));
    REQUIRE(guard != 0 && protectedWorker != 0 && nearbyEnemy != 0);

    Command guardOrder = MakeCommand(0, 0, 1, CommandType::Guard, guard);
    guardOrder.target = protectedWorker;
    REQUIRE(simulation.QueueCommand(guardOrder));
    simulation.Step(180);
    REQUIRE(simulation.FindEntity(nearbyEnemy) == nullptr);
    const Entity* guardState = simulation.FindEntity(guard);
    const Entity* workerState = simulation.FindEntity(protectedWorker);
    REQUIRE(guardState != nullptr && workerState != nullptr);
    REQUIRE(guardState->order.type == OrderType::Guard);
    REQUIRE(guardState->order.target == protectedWorker);
    REQUIRE(guardState->position.x.Raw() >=
            workerState->position.x.Raw() - 2 * kFixedScale);
    REQUIRE(guardState->position.x.Raw() <=
            workerState->position.x.Raw() + 2 * kFixedScale);

    Command moveWorker = MakeCommand(
        simulation.CurrentTick(), 0, 2, CommandType::Move, protectedWorker);
    moveWorker.position = Vec2::FromTiles(18, 4);
    REQUIRE(simulation.QueueCommand(moveWorker));
    simulation.Step(240);
    guardState = simulation.FindEntity(guard);
    workerState = simulation.FindEntity(protectedWorker);
    REQUIRE(guardState != nullptr && workerState != nullptr);
    REQUIRE(workerState->position == Vec2::FromTiles(18, 4));
    REQUIRE(guardState->order.type == OrderType::Guard);
    REQUIRE(guardState->order.destination == workerState->position);
    REQUIRE(guardState->position.x.Raw() >=
            workerState->position.x.Raw() - 2 * kFixedScale);
    REQUIRE(guardState->position.x.Raw() <=
            workerState->position.x.Raw() + 2 * kFixedScale);

    std::string error;
    std::optional<Simulation> restored =
        Simulation::LoadSnapshot(simulation.SaveSnapshot(), &error);
    REQUIRE(restored.has_value());
    REQUIRE(error.empty());
    REQUIRE(restored->FindEntity(guard)->order.type == OrderType::Guard);
    REQUIRE(restored->FindEntity(guard)->order.target == protectedWorker);

    Command stop = MakeCommand(
        simulation.CurrentTick(), 0, 3, CommandType::Stop, guard);
    REQUIRE(simulation.QueueCommand(stop));
    simulation.Step();
    REQUIRE(simulation.FindEntity(guard)->order.type == OrderType::None);
}

void TestPatrolReversesPersistsAndBoundsEngagements() {
    Simulation simulation({36, 20, 20, 0x504154524f4c4f52ULL});
    AddTwoPlayers(simulation, {0, 0}, {0, 0});
    const Vec2 origin = Vec2::FromTiles(4, 4);
    const Vec2 endpoint = Vec2::FromTiles(10, 4);
    const EntityId patrol = simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Soldier, origin);
    const EntityId routeEnemy = simulation.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::Soldier,
        Vec2::FromTiles(9, 4));
    const EntityId distantEnemy = simulation.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::Soldier,
        Vec2::FromTiles(25, 4));
    REQUIRE(patrol != 0 && routeEnemy != 0 && distantEnemy != 0);

    Command patrolOrder = MakeCommand(0, 0, 1, CommandType::Patrol, patrol);
    patrolOrder.position = endpoint;
    REQUIRE(simulation.QueueCommand(patrolOrder));
    simulation.Step();
    const Entity* patrolling = simulation.FindEntity(patrol);
    REQUIRE(patrolling != nullptr);
    REQUIRE(patrolling->order.type == OrderType::Patrol);
    REQUIRE(patrolling->order.anchor == origin);
    REQUIRE(patrolling->order.destination == endpoint);

    std::string error;
    std::optional<Simulation> restored =
        Simulation::LoadSnapshot(simulation.SaveSnapshot(), &error);
    REQUIRE(restored.has_value());
    REQUIRE(error.empty());
    REQUIRE(restored->StateChecksum() == simulation.StateChecksum());
    const Entity* restoredPatrol = restored->FindEntity(patrol);
    REQUIRE(restoredPatrol != nullptr);
    REQUIRE(restoredPatrol->order.type == OrderType::Patrol);
    REQUIRE(restoredPatrol->order.anchor == origin);
    REQUIRE(restoredPatrol->order.destination == endpoint);

    bool sawReturnLeg = false;
    bool completedRoundTrip = false;
    for (int tick = 0; tick < 600 && !completedRoundTrip; ++tick) {
        simulation.Step();
        patrolling = simulation.FindEntity(patrol);
        REQUIRE(patrolling != nullptr);
        REQUIRE(patrolling->order.type == OrderType::Patrol);
        if (patrolling->order.anchor == endpoint &&
            patrolling->order.destination == origin) {
            sawReturnLeg = true;
        }
        if (sawReturnLeg && patrolling->order.anchor == origin &&
            patrolling->order.destination == endpoint) {
            completedRoundTrip = true;
        }
    }
    REQUIRE(sawReturnLeg);
    REQUIRE(completedRoundTrip);
    REQUIRE(simulation.FindEntity(routeEnemy) == nullptr);
    const Entity* untouched = simulation.FindEntity(distantEnemy);
    REQUIRE(untouched != nullptr);
    REQUIRE(untouched->hitPoints == untouched->maxHitPoints);

    const Vec2 stoppedPosition = simulation.FindEntity(patrol)->position;
    Command stop = MakeCommand(
        simulation.CurrentTick(), 0, 2, CommandType::Stop, patrol);
    REQUIRE(simulation.QueueCommand(stop));
    simulation.Step();
    REQUIRE(simulation.FindEntity(patrol)->order.type == OrderType::None);
    REQUIRE(simulation.FindEntity(patrol)->position == stoppedPosition);
    simulation.Step(40);
    REQUIRE(simulation.FindEntity(patrol)->position == stoppedPosition);
}

void TestDeterministicObstaclePathing() {
    Simulation first({12, 12, 20, 0x50415448});
    REQUIRE(first.AddPlayer(
        0, Faction::MeridianCompact, ResourcePool{0, 0}));
    for (std::int32_t tileY = 0; tileY < 11; ++tileY) {
        REQUIRE(first.SetTerrainTile(5, tileY, Terrain::Blocked));
    }
    const EntityId scout = first.SpawnEntity(
        0,
        Faction::MeridianCompact,
        EntityType::Soldier,
        Vec2::FromTiles(2, 2));
    REQUIRE(scout != 0);
    Command move = MakeCommand(0, 0, 1, CommandType::Move, scout);
    move.position = Vec2::FromTiles(8, 2);
    REQUIRE(first.QueueCommand(move));

    Simulation second = first;
    for (std::int32_t tick = 0; tick < 240; ++tick) {
        first.Step();
        second.Step();
        const Entity* moving = first.FindEntity(scout);
        REQUIRE(moving != nullptr);
        REQUIRE(first.TerrainAt(
                    moving->position.x.FloorToInt(),
                    moving->position.y.FloorToInt()) != Terrain::Blocked);
    }
    REQUIRE(first.FindEntity(scout)->position == Vec2::FromTiles(8, 2));
    REQUIRE(first.FindEntity(scout)->order.type == OrderType::None);
    REQUIRE(first.StateChecksum() == second.StateChecksum());

    Simulation invalidation({14, 10, 20, 0x4341434845494e56ULL});
    REQUIRE(invalidation.AddPlayer(
        0, Faction::MeridianCompact, ResourcePool{0, 0}));
    const EntityId rerouted = invalidation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Soldier,
        Vec2::FromTiles(2, 4));
    REQUIRE(rerouted != 0);
    Command cachedMove =
        MakeCommand(0, 0, 1, CommandType::Move, rerouted);
    cachedMove.position = Vec2::FromTiles(11, 4);
    REQUIRE(invalidation.QueueCommand(cachedMove));
    invalidation.Step(8);
    REQUIRE(invalidation.SetTerrainTile(5, 4, Terrain::Blocked));
    for (std::int32_t tick = 0; tick < 180; ++tick) {
        invalidation.Step();
        const Entity* moving = invalidation.FindEntity(rerouted);
        REQUIRE(moving != nullptr);
        REQUIRE(invalidation.TerrainAt(
                    moving->position.x.FloorToInt(),
                    moving->position.y.FloorToInt()) != Terrain::Blocked);
    }
    REQUIRE(invalidation.FindEntity(rerouted)->position ==
            Vec2::FromTiles(11, 4));
    REQUIRE(invalidation.FindEntity(rerouted)->order.type == OrderType::None);
}

void TestProductionPopulationAndVictory() {
    Simulation production({32, 32, 20, 0x51});
    AddTwoPlayers(production, {1000, 200}, {1000, 200});
    const EntityId localCore = production.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::CommandCore,
        Vec2::FromTiles(5, 5));
    const EntityId localBarracks = production.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Barracks,
        Vec2::FromTiles(10, 5));
    const EntityId enemyCore = production.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::CommandCore,
        Vec2::FromTiles(26, 26));
    REQUIRE(localCore != 0 && localBarracks != 0 && enemyCore != 0);
    REQUIRE(production.Outcome() == MatchOutcome::Ongoing);
    REQUIRE(production.PopulationUsed(0) == 0);
    REQUIRE(production.PopulationCapacity(0) == 12);
    REQUIRE(production.ValidateProduction(
                0, localCore, EntityType::Worker) == ProductionResult::Valid);
    REQUIRE(production.ValidateProduction(
                0, localCore, EntityType::Soldier) ==
            ProductionResult::UnsupportedUnit);

    Command workerOrder =
        MakeCommand(0, 0, 1, CommandType::Produce, localCore);
    workerOrder.buildType = EntityType::Worker;
    Command soldierOrder =
        MakeCommand(0, 0, 2, CommandType::Produce, localBarracks);
    soldierOrder.buildType = EntityType::Soldier;
    REQUIRE(production.QueueCommand(workerOrder));
    REQUIRE(production.QueueCommand(soldierOrder));
    production.Step(10);
    REQUIRE(production.FindEntity(localCore)->productionProgress == 10);
    REQUIRE(production.FindEntity(localBarracks)->productionProgress == 10);

    std::string error;
    std::optional<Simulation> restored =
        Simulation::LoadSnapshot(production.SaveSnapshot(), &error);
    REQUIRE(restored.has_value());
    REQUIRE(error.empty());
    production.Step(100);
    restored->Step(100);
    REQUIRE(restored->StateChecksum() == production.StateChecksum());
    REQUIRE(production.FindPlayer(0)->resources.material == 865);
    REQUIRE(production.FindPlayer(0)->resources.dawnshards == 180);
    REQUIRE(production.PopulationUsed(0) == 3);
    REQUIRE(std::count_if(
                production.Entities().begin(), production.Entities().end(),
                [](const Entity& entity) {
                    return entity.owner == 0 && entity.type == EntityType::Worker;
                }) == 1);
    REQUIRE(std::count_if(
                production.Entities().begin(), production.Entities().end(),
                [](const Entity& entity) {
                    return entity.owner == 0 && entity.type == EntityType::Soldier;
                }) == 1);

    Simulation capacity({32, 32, 20, 0x52});
    REQUIRE(capacity.AddPlayer(0, Faction::MeridianCompact, {1000, 200}));
    const EntityId capacityCore = capacity.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::CommandCore,
        Vec2::FromTiles(5, 5));
    REQUIRE(capacityCore != 0);
    for (std::int32_t index = 0; index < 12; ++index) {
        REQUIRE(capacity.SpawnEntity(
                    0, Faction::MeridianCompact, EntityType::Worker,
                    Vec2::FromTiles(10 + index % 6, 10 + index / 6)) != 0);
    }
    REQUIRE(capacity.PopulationUsed(0) == 12);
    REQUIRE(capacity.ValidateProduction(
                0, capacityCore, EntityType::Worker) ==
            ProductionResult::CapacityReached);

    Simulation victory({24, 24, 20, 0x53});
    AddTwoPlayers(victory, {0, 0}, {0, 0});
    REQUIRE(victory.SpawnEntity(
                0, Faction::MeridianCompact, EntityType::CommandCore,
                Vec2::FromTiles(4, 4)) != 0);
    const EntityId targetCore = victory.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::CommandCore,
        Vec2::FromTiles(10, 4));
    REQUIRE(targetCore != 0);
    for (std::uint64_t index = 0; index < 10; ++index) {
        const EntityId soldier = victory.SpawnEntity(
            0, Faction::MeridianCompact, EntityType::Soldier,
            Vec2::FromTiles(7, 4));
        REQUIRE(soldier != 0);
        Command attack =
            MakeCommand(0, 0, index + 1, CommandType::Attack, soldier);
        attack.target = targetCore;
        REQUIRE(victory.QueueCommand(attack));
    }
    victory.Step(80);
    REQUIRE(victory.FindEntity(targetCore) == nullptr);
    REQUIRE(victory.Outcome() == MatchOutcome::Player0Victory);
}

void TestFogAndNonCheatingAi() {
    static_assert(!std::is_default_constructible_v<PlayerView>);
    Simulation simulation({32, 32, 20, 5});
    AddTwoPlayers(simulation, {0, 0}, {0, 0});
    const EntityId worker = simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Worker, Vec2::FromTiles(3, 2));
    const EntityId soldier = simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Soldier, Vec2::FromTiles(3, 3));
    const EntityId resource =
        simulation.SpawnResourceNode(Vec2::FromTiles(4, 2), 100);
    const EntityId visibleEnemy = simulation.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::Soldier,
        Vec2::FromTiles(8, 3));
    const EntityId hiddenEnemy = simulation.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::Soldier,
        Vec2::FromTiles(25, 25));
    REQUIRE(worker != 0 && soldier != 0 && resource != 0 && visibleEnemy != 0 &&
            hiddenEnemy != 0);
    REQUIRE(simulation.VisibilityAt(0, Vec2::FromTiles(3, 3)) ==
            Visibility::Visible);
    REQUIRE(simulation.VisibilityAt(0, Vec2::FromTiles(25, 25)) ==
            Visibility::Unexplored);
    REQUIRE(!simulation.IsEntityVisibleTo(0, hiddenEnemy));
    REQUIRE(!simulation.CreatePlayerView(3).has_value());

    const std::optional<PlayerView> playerView = simulation.CreatePlayerView(0);
    REQUIRE(playerView.has_value());
    REQUIRE(playerView->Player().id == 0);
    REQUIRE(playerView->Config().randomSeed == 0);
    REQUIRE(playerView->DecisionSeed() == 5);
    REQUIRE(playerView->VisibilityAt(Vec2::FromTiles(25, 25)) ==
            Visibility::Unexplored);
    REQUIRE(playerView->TerrainAt(25, 25) == Terrain::Blocked);
    REQUIRE(!playerView->IsPositionPassable(Vec2::FromTiles(25, 25)));
    const auto observedEnemy = std::find_if(
        playerView->Entities().begin(), playerView->Entities().end(),
        [visibleEnemy](const Entity& entity) {
            return entity.id == visibleEnemy;
        });
    REQUIRE(observedEnemy != playerView->Entities().end());
    REQUIRE(observedEnemy->attackDamage == 0);
    REQUIRE(observedEnemy->attackCooldownTicks == 0);
    REQUIRE(observedEnemy->order.type == OrderType::None);
    const auto observedOwnSoldier = std::find_if(
        playerView->Entities().begin(), playerView->Entities().end(),
        [soldier](const Entity& entity) { return entity.id == soldier; });
    REQUIRE(observedOwnSoldier != playerView->Entities().end());
    REQUIRE(observedOwnSoldier->attackDamage > 0);
    REQUIRE(std::none_of(
        playerView->Entities().begin(), playerView->Entities().end(),
        [hiddenEnemy](const Entity& entity) { return entity.id == hiddenEnemy; }));

    const std::vector<Command> ai =
        simulation.GenerateAiCommands(0, AiPersonality::Balanced);
    REQUIRE(ai == Simulation::GenerateAiCommands(
                      *playerView, AiPersonality::Balanced));
    const auto workerDecision = std::find_if(
        ai.begin(), ai.end(),
        [worker](const Command& command) { return command.actor == worker; });
    const auto soldierDecision = std::find_if(
        ai.begin(), ai.end(),
        [soldier](const Command& command) { return command.actor == soldier; });
    REQUIRE(workerDecision != ai.end());
    REQUIRE(workerDecision->type == CommandType::Gather);
    REQUIRE(workerDecision->target == resource);
    REQUIRE(soldierDecision != ai.end());
    REQUIRE(soldierDecision->type == CommandType::Attack);
    REQUIRE(soldierDecision->target == visibleEnemy);
    REQUIRE(std::none_of(ai.begin(), ai.end(), [hiddenEnemy](const Command& command) {
        return command.target == hiddenEnemy;
    }));

    Simulation expansion({32, 32, 20, 0x455850414e444149ULL});
    REQUIRE(expansion.AddPlayer(
        0, Faction::KharuunAssemblies, ResourcePool{500, 60}));
    const EntityId expansionCore = expansion.SpawnEntity(
        0, Faction::KharuunAssemblies, EntityType::CommandCore,
        Vec2::FromTiles(5, 5));
    const EntityId expansionWorker = expansion.SpawnEntity(
        0, Faction::KharuunAssemblies, EntityType::Worker,
        Vec2::FromTiles(7, 5));
    REQUIRE(expansionCore != 0 && expansionWorker != 0);
    const std::vector<Command> expansionCommands =
        expansion.GenerateAiCommands(0, AiPersonality::Expansionist);
    const auto expansionDecision = std::find_if(
        expansionCommands.begin(), expansionCommands.end(),
        [expansionWorker](const Command& command) {
            return command.actor == expansionWorker;
        });
    REQUIRE(expansionDecision != expansionCommands.end());
    REQUIRE(expansionDecision->type == CommandType::Build);
    REQUIRE(expansionDecision->buildType == EntityType::Barracks);
    REQUIRE(expansion.VisibilityAt(0, expansionDecision->position) ==
            Visibility::Visible);
    REQUIRE(expansion.ValidatePlacement(
                0, expansionDecision->buildType, expansionDecision->position) ==
            PlacementResult::Valid);
    REQUIRE(expansion.QueueCommand(*expansionDecision));
    expansion.Step();
    REQUIRE(std::any_of(
        expansion.Entities().begin(), expansion.Entities().end(),
        [](const Entity& entity) {
            return entity.owner == 0 && entity.type == EntityType::Barracks &&
                   !entity.completed;
        }));

    Simulation retreat({32, 32, 20, 0x5245545245415441ULL});
    AddTwoPlayers(retreat, {0, 0}, {0, 0});
    REQUIRE(retreat.SpawnEntity(
                0, Faction::MeridianCompact, EntityType::CommandCore,
                Vec2::FromTiles(3, 3)) != 0);
    const EntityId retreatCore = retreat.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::CommandCore,
        Vec2::FromTiles(26, 26));
    const EntityId retreatSoldier = retreat.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::Soldier,
        Vec2::FromTiles(18, 18));
    const EntityId pursuingSoldier = retreat.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Soldier,
        Vec2::FromTiles(19, 18));
    REQUIRE(retreatCore != 0 && retreatSoldier != 0 && pursuingSoldier != 0);
    Command damage =
        MakeCommand(0, 0, 1, CommandType::Attack, pursuingSoldier);
    damage.target = retreatSoldier;
    REQUIRE(retreat.QueueCommand(damage));
    retreat.Step(49);
    const Entity* damagedSoldier = retreat.FindEntity(retreatSoldier);
    REQUIRE(damagedSoldier != nullptr);
    REQUIRE(damagedSoldier->hitPoints > 0);
    REQUIRE(damagedSoldier->hitPoints * 100 <= damagedSoldier->maxHitPoints * 35);
    const std::vector<Command> retreatCommands =
        retreat.GenerateAiCommands(1, AiPersonality::Adaptive);
    const auto retreatDecision = std::find_if(
        retreatCommands.begin(), retreatCommands.end(),
        [retreatSoldier](const Command& command) {
            return command.actor == retreatSoldier;
        });
    REQUIRE(retreatDecision != retreatCommands.end());
    REQUIRE(retreatDecision->type == CommandType::Move);
    REQUIRE(retreatDecision->target == 0);
    REQUIRE(retreatDecision->position != damagedSoldier->position);
    REQUIRE(std::none_of(
        retreatCommands.begin(), retreatCommands.end(),
        [retreatSoldier, pursuingSoldier](const Command& command) {
            return command.actor == retreatSoldier &&
                   command.target == pursuingSoldier;
        }));
    REQUIRE(retreat.GenerateAiCommands(
                1, static_cast<AiPersonality>(255)).empty());

    SimulationConfig openingConfig{64, 64, 20, 0x4f50454e494e4741ULL};
    openingConfig.rules.vibrationDetection.signatureLingerTicks = 10000;
    Simulation opening(openingConfig);
    AddTwoPlayers(opening, {0, 0}, {0, 0});
    const EntityId openingCore = opening.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::CommandCore,
        Vec2::FromTiles(55, 55));
    const EntityId openingResonant = opening.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::ScoutUnit,
        Vec2::FromTiles(50, 55));
    const EntityId openingMover = opening.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Soldier,
        Vec2::FromTiles(33, 55));
    REQUIRE(openingCore != 0 && openingResonant != 0 && openingMover != 0);
    Command openingMove =
        MakeCommand(0, 0, 1, CommandType::Move, openingMover);
    openingMove.position = Vec2::FromTiles(34, 55);
    REQUIRE(opening.QueueCommand(openingMove));
    opening.Step();
    const std::optional<PlayerView> openingView = opening.CreatePlayerView(1);
    REQUIRE(openingView.has_value());
    REQUIRE(!openingView->VibrationSignatures().empty());
    const std::vector<Command> openingCommands =
        Simulation::GenerateAiCommands(*openingView, AiPersonality::Adaptive);
    const auto openingDecision = std::find_if(
        openingCommands.begin(), openingCommands.end(),
        [openingResonant](const Command& command) {
            return command.actor == openingResonant;
        });
    REQUIRE(openingDecision != openingCommands.end());
    REQUIRE(openingDecision->type == CommandType::Hold);
    REQUIRE(std::none_of(
        openingCommands.begin(), openingCommands.end(),
        [openingResonant](const Command& command) {
            return command.actor == openingResonant &&
                   command.type == CommandType::AttackMove;
        }));

    Simulation exploredSimulation({32, 32, 20, 5});
    REQUIRE(exploredSimulation.AddPlayer(0, Faction::MeridianCompact, {0, 0}));
    const EntityId scout = exploredSimulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Worker, Vec2::FromTiles(3, 3));
    Command move = MakeCommand(0, 0, 1, CommandType::Move, scout);
    move.position = Vec2::FromTiles(20, 20);
    REQUIRE(exploredSimulation.QueueCommand(move));
    exploredSimulation.Step(300);
    REQUIRE(exploredSimulation.VisibilityAt(0, Vec2::FromTiles(3, 3)) ==
            Visibility::Explored);
}

void TestFourPlayerVisibilitySnapshotAndOutcome() {
    Simulation simulation({64, 64, 20, 0x464f5552504c4159ULL});
    REQUIRE(simulation.AddPlayer(0, Faction::MeridianCompact, {100, 10}));
    REQUIRE(simulation.AddPlayer(1, Faction::KharuunAssemblies, {200, 20}));
    REQUIRE(simulation.AddPlayer(2, Faction::KharuunAssemblies, {300, 30}));
    REQUIRE(simulation.AddPlayer(3, Faction::MeridianCompact, {400, 40}));
    REQUIRE(!simulation.AddPlayer(4, Faction::MeridianCompact, {0, 0}));

    constexpr std::array<Vec2, kMaximumPlayers> positions{
        Vec2::FromTiles(4, 4),
        Vec2::FromTiles(59, 59),
        Vec2::FromTiles(59, 4),
        Vec2::FromTiles(4, 59),
    };
    constexpr std::array<Faction, kMaximumPlayers> factions{
        Faction::MeridianCompact,
        Faction::KharuunAssemblies,
        Faction::KharuunAssemblies,
        Faction::MeridianCompact,
    };
    std::array<EntityId, kMaximumPlayers> scouts{};
    for (PlayerId player = 0; player < kMaximumPlayers; ++player) {
        scouts[player] = simulation.SpawnEntity(
            player, factions[player], EntityType::Soldier, positions[player]);
        REQUIRE(scouts[player] != 0);
    }
    for (PlayerId viewer = 0; viewer < kMaximumPlayers; ++viewer) {
        REQUIRE(simulation.VisibilityAt(viewer, positions[viewer]) ==
                Visibility::Visible);
        REQUIRE(simulation.IsEntityVisibleTo(viewer, scouts[viewer]));
        for (PlayerId target = 0; target < kMaximumPlayers; ++target) {
            if (target == viewer) {
                continue;
            }
            REQUIRE(simulation.VisibilityAt(viewer, positions[target]) ==
                    Visibility::Unexplored);
            REQUIRE(!simulation.IsEntityVisibleTo(viewer, scouts[target]));
        }
    }

    Command move = MakeCommand(0, 2, 1, CommandType::Move, scouts[2]);
    move.position = Vec2::FromTiles(50, 4);
    REQUIRE(simulation.QueueCommand(move));
    simulation.Step(8);
    std::string error;
    const std::vector<std::uint8_t> snapshot = simulation.SaveSnapshot();
    std::optional<Simulation> restored =
        Simulation::LoadSnapshot(snapshot, &error);
    REQUIRE(restored.has_value());
    REQUIRE(error.empty());
    REQUIRE(restored->StateChecksum() == simulation.StateChecksum());
    for (PlayerId player = 0; player < kMaximumPlayers; ++player) {
        REQUIRE(restored->FindPlayer(player) != nullptr);
        REQUIRE(restored->FindPlayer(player)->resources ==
                simulation.FindPlayer(player)->resources);
        REQUIRE(restored->VisibilityAt(player, positions[player]) ==
                simulation.VisibilityAt(player, positions[player]));
    }

    Simulation playerTwoWins({24, 24, 20, 0x503257494eULL});
    for (PlayerId player = 0; player < kMaximumPlayers; ++player) {
        REQUIRE(playerTwoWins.AddPlayer(player, factions[player], {0, 0}));
    }
    REQUIRE(playerTwoWins.SpawnEntity(
                2, factions[2], EntityType::CommandCore,
                Vec2::FromTiles(12, 12)) != 0);
    REQUIRE(playerTwoWins.Outcome() == MatchOutcome::Player2Victory);

    Simulation playerThreeWins({24, 24, 20, 0x503357494eULL});
    for (PlayerId player = 0; player < kMaximumPlayers; ++player) {
        REQUIRE(playerThreeWins.AddPlayer(player, factions[player], {0, 0}));
    }
    REQUIRE(playerThreeWins.SpawnEntity(
                3, factions[3], EntityType::CommandCore,
                Vec2::FromTiles(12, 12)) != 0);
    REQUIRE(playerThreeWins.Outcome() == MatchOutcome::Player3Victory);
}

void TestFutureWellChoices() {
    {
        Simulation harvest({20, 20, 20, 11});
        REQUIRE(harvest.AddPlayer(0, Faction::MeridianCompact, {500, 50}));
        const EntityId worker = harvest.SpawnEntity(
            0, Faction::MeridianCompact, EntityType::Worker,
            Vec2::FromTiles(5, 6));
        const EntityId well = harvest.SpawnFutureWell(Vec2::FromTiles(6, 6));
        Command action = MakeCommand(0, 0, 1, CommandType::FutureWell, worker);
        action.target = well;
        action.wellChoice = FutureWellChoice::Harvest;
        REQUIRE(harvest.QueueCommand(action));
        harvest.Step();
        REQUIRE(harvest.FindPlayer(0)->resources.dawnshards == 350);
        REQUIRE(harvest.FindEntity(well)->wellChoice == FutureWellChoice::Harvest);
        REQUIRE(harvest.TerrainAt(7, 7) == Terrain::Scarred);
        REQUIRE(harvest.ValidatePlacement(0, EntityType::Barracks,
                                          Vec2::FromTiles(8, 8)) ==
                PlacementResult::TerrainRestricted);
    }
    {
        Simulation preserve({24, 24, 20, 11});
        REQUIRE(preserve.AddPlayer(0, Faction::MeridianCompact, {0, 0}));
        const EntityId worker = preserve.SpawnEntity(
            0, Faction::MeridianCompact, EntityType::Worker,
            Vec2::FromTiles(9, 10));
        const EntityId well = preserve.SpawnFutureWell(Vec2::FromTiles(10, 10));
        Command action = MakeCommand(0, 0, 1, CommandType::FutureWell, worker);
        action.target = well;
        action.wellChoice = FutureWellChoice::Preserve;
        REQUIRE(preserve.QueueCommand(action));
        preserve.Step(20);
        REQUIRE(preserve.FindPlayer(0)->resources.dawnshards == 6);
        REQUIRE(preserve.FindEntity(well)->wellChoice == FutureWellChoice::Preserve);
        REQUIRE(preserve.VisibilityAt(0, Vec2::FromTiles(18, 10)) ==
                Visibility::Visible);
    }
    {
        Simulation reshape({20, 20, 20, 0x55});
        REQUIRE(reshape.AddPlayer(0, Faction::MeridianCompact, {0, 200}));
        const EntityId worker = reshape.SpawnEntity(
            0, Faction::MeridianCompact, EntityType::Worker,
            Vec2::FromTiles(5, 6));
        const EntityId pathfinder = reshape.SpawnEntity(
            0, Faction::MeridianCompact, EntityType::Soldier,
            Vec2::FromTiles(4, 6));
        const EntityId expiryProbe = reshape.SpawnEntity(
            0, Faction::MeridianCompact, EntityType::Soldier,
            Vec2::FromTiles(3, 7));
        const EntityId well = reshape.SpawnFutureWell(Vec2::FromTiles(6, 6));
        for (std::int32_t tileY = 0; tileY < 20; ++tileY) {
            REQUIRE(reshape.SetTerrainTile(7, tileY, Terrain::Blocked));
        }
        REQUIRE(!reshape.IsPositionPassable(Vec2::FromTiles(7, 6)));
        Command initiallyBlocked = MakeCommand(
            0, 0, 1, CommandType::Move, pathfinder);
        initiallyBlocked.position = Vec2::FromTiles(9, 6);
        REQUIRE(reshape.QueueCommand(initiallyBlocked));
        reshape.Step();
        REQUIRE(reshape.FindEntity(pathfinder)->position ==
                Vec2::FromTiles(4, 6));

        Command action = MakeCommand(1, 0, 2, CommandType::FutureWell, worker);
        action.target = well;
        action.wellChoice = FutureWellChoice::Reshape;
        REQUIRE(reshape.QueueCommand(action));
        reshape.Step();
        REQUIRE(reshape.FindPlayer(0)->resources.dawnshards == 100);
        const Entity* reshapedWell = reshape.FindEntity(well);
        REQUIRE(reshapedWell->wellChoice == FutureWellChoice::Reshape);
        REQUIRE(reshapedWell->reshapeUntilTick >= 40 &&
                reshapedWell->reshapeUntilTick <= 60);
        REQUIRE(reshape.IsPositionPassable(Vec2::FromTiles(7, 6)));
        Command enter = MakeCommand(reshape.CurrentTick(), 0, 3,
                                    CommandType::Move, worker);
        enter.position = Vec2::FromTiles(7, 6);
        REQUIRE(reshape.QueueCommand(enter));
        reshape.Step(20);
        REQUIRE(reshape.FindEntity(worker)->position == Vec2::FromTiles(7, 6));
        REQUIRE(reshape.FindEntity(pathfinder)->position.x.Raw() >
                Vec2::FromTiles(4, 6).x.Raw());
        const Tick end = reshapedWell->reshapeUntilTick;
        reshape.Step(end - reshape.CurrentTick());
        REQUIRE(!reshape.IsPositionPassable(Vec2::FromTiles(7, 6)));
        REQUIRE(reshape.FindEntity(worker)->position != Vec2::FromTiles(7, 6));
        REQUIRE(reshape.IsPositionPassable(reshape.FindEntity(worker)->position));

        Command afterExpiry = MakeCommand(
            reshape.CurrentTick(), 0, 4, CommandType::Move, expiryProbe);
        afterExpiry.position = Vec2::FromTiles(9, 6);
        REQUIRE(reshape.QueueCommand(afterExpiry));
        reshape.Step(80);
        REQUIRE(reshape.FindEntity(expiryProbe)->position ==
                Vec2::FromTiles(3, 7));
    }
}

void TestSnapshotAndReplay() {
    DeterminismScenario scenario = MakeDeterminismScenario();
    scenario.simulation.CaptureReplayBaseline();
    const std::vector<Command> commands = DeterminismCommands(scenario);
    for (const Command& command : commands) {
        REQUIRE(scenario.simulation.QueueCommand(command));
    }
    scenario.simulation.Step(25);

    const std::vector<std::uint8_t> snapshot = scenario.simulation.SaveSnapshot();
    std::string error;
    std::optional<Simulation> loaded = Simulation::LoadSnapshot(snapshot, &error);
    REQUIRE(loaded.has_value());
    REQUIRE(error.empty());
    REQUIRE(loaded->StateChecksum() == scenario.simulation.StateChecksum());
    for (int tick = 0; tick < 50; ++tick) {
        loaded->Step();
        scenario.simulation.Step();
        REQUIRE(loaded->StateChecksum() == scenario.simulation.StateChecksum());
    }

    std::vector<std::uint8_t> corrupted = snapshot;
    corrupted[20] ^= 0x40;
    REQUIRE(!Simulation::LoadSnapshot(corrupted, &error).has_value());
    REQUIRE(error == "snapshot integrity check failed");

    const ReplayRecord replay = scenario.simulation.ExportReplay();
    REQUIRE(replay.commands.size() == commands.size());
    std::optional<Simulation> reproduced = Simulation::ReplayToEnd(replay, &error);
    REQUIRE(reproduced.has_value());
    REQUIRE(reproduced->CurrentTick() == scenario.simulation.CurrentTick());
    REQUIRE(reproduced->StateChecksum() == scenario.simulation.StateChecksum());
}

void TestNumericAndPublicInputHardening() {
    REQUIRE(Fixed::FromInt(std::numeric_limits<std::int32_t>::max()).Raw() ==
            std::numeric_limits<std::int32_t>::max());
    REQUIRE(Fixed::FromInt(std::numeric_limits<std::int32_t>::min()).Raw() ==
            std::numeric_limits<std::int32_t>::min());
    REQUIRE((Fixed::FromRaw(std::numeric_limits<std::int32_t>::max()) +
             Fixed::FromRaw(1))
                .Raw() == std::numeric_limits<std::int32_t>::max());
    REQUIRE((Fixed::FromRaw(std::numeric_limits<std::int32_t>::min()) -
             Fixed::FromRaw(1))
                .Raw() == std::numeric_limits<std::int32_t>::min());
    REQUIRE(Fixed::FromRaw(std::numeric_limits<std::int32_t>::min())
                .FloorToInt() ==
            std::numeric_limits<std::int32_t>::min() / kFixedScale);

    Simulation harvest({8, 8, 20, 1});
    REQUIRE(harvest.AddPlayer(
        0, Faction::MeridianCompact,
        {0, std::numeric_limits<std::int32_t>::max() - 100}));
    const EntityId worker = harvest.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Worker, Vec2::FromTiles(2, 2));
    const EntityId well = harvest.SpawnFutureWell(Vec2::FromTiles(3, 2));
    Command action = MakeCommand(0, 0, 1, CommandType::FutureWell, worker);
    action.target = well;
    action.wellChoice = FutureWellChoice::Harvest;
    REQUIRE(harvest.QueueCommand(action));
    harvest.Step();
    REQUIRE(harvest.FindPlayer(0)->resources.dawnshards ==
            std::numeric_limits<std::int32_t>::max());

    Simulation validation({8, 8, 20, 1});
    REQUIRE(!validation.AddPlayer(0, static_cast<Faction>(255), {0, 0}));
    REQUIRE(validation.AddPlayer(0, Faction::MeridianCompact, {0, 0}));
    REQUIRE(!validation.SetTerrainTile(1, 1, static_cast<Terrain>(255)));
    REQUIRE(validation.SpawnEntity(0, Faction::MeridianCompact,
                                      static_cast<EntityType>(255),
                                      Vec2::FromTiles(2, 2)) == 0);
    const EntityId validWorker = validation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Worker, Vec2::FromTiles(2, 2));
    Command malformed = MakeCommand(0, 0, 1, static_cast<CommandType>(255),
                                    validWorker);
    REQUIRE(!validation.QueueCommand(malformed));
    malformed.type = CommandType::Stop;
    malformed.buildType = static_cast<EntityType>(255);
    REQUIRE(!validation.QueueCommand(malformed));
    REQUIRE(validation.GenerateAiCommands(
                           0, static_cast<AiPersonality>(255))
                .empty());
}

void TestSequenceAndBuildHardening() {
    Simulation sequences({12, 12, 20, 1});
    REQUIRE(!sequences.NextCommandSequence(0).has_value());
    REQUIRE(sequences.AddPlayer(0, Faction::MeridianCompact, {0, 0}));
    REQUIRE(sequences.NextCommandSequence(0) == 1);
    REQUIRE(!sequences.NextCommandSequence(1).has_value());
    const EntityId worker = sequences.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Worker, Vec2::FromTiles(2, 2));
    Command first = MakeCommand(0, 0, 10, CommandType::Stop, worker);
    Command duplicateAcrossTick =
        MakeCommand(1, 0, 10, CommandType::Stop, worker);
    Command regressed = MakeCommand(1, 0, 9, CommandType::Stop, worker);
    Command next = MakeCommand(1, 0, 11, CommandType::Stop, worker);
    REQUIRE(sequences.QueueCommand(first));
    REQUIRE(sequences.NextCommandSequence(0) == 11);
    REQUIRE(!sequences.QueueCommand(duplicateAcrossTick));
    REQUIRE(!sequences.QueueCommand(regressed));
    REQUIRE(sequences.QueueCommand(next));
    REQUIRE(sequences.NextCommandSequence(0) == 12);
    sequences.Step();
    REQUIRE(sequences.NextCommandSequence(0) == 12);

    std::string error;
    std::optional<Simulation> loaded =
        Simulation::LoadSnapshot(sequences.SaveSnapshot(), &error);
    REQUIRE(loaded.has_value());
    REQUIRE(loaded->NextCommandSequence(0) == 12);
    Command stale = MakeCommand(loaded->CurrentTick(), 0, 10,
                                CommandType::Stop, worker);
    Command newer = MakeCommand(loaded->CurrentTick(), 0, 12,
                                CommandType::Stop, worker);
    REQUIRE(!loaded->QueueCommand(stale));
    REQUIRE(loaded->QueueCommand(newer));
    REQUIRE(loaded->NextCommandSequence(0) == 13);

    Simulation build({24, 24, 20, 1});
    REQUIRE(build.AddPlayer(0, Faction::MeridianCompact, {1000, 500}));
    const EntityId builder = build.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Worker, Vec2::FromTiles(3, 3));
    Command firstBuild = MakeCommand(0, 0, 1, CommandType::Build, builder);
    firstBuild.buildType = EntityType::Barracks;
    firstBuild.position = Vec2::FromTiles(8, 8);
    Command secondBuild = MakeCommand(0, 0, 2, CommandType::Build, builder);
    secondBuild.buildType = EntityType::Barracks;
    secondBuild.position = Vec2::FromTiles(14, 14);
    REQUIRE(build.QueueCommand(firstBuild));
    REQUIRE(build.QueueCommand(secondBuild));
    build.Step();
    REQUIRE(std::count_if(build.Entities().begin(), build.Entities().end(),
                          [](const Entity& entity) {
                              return entity.type == EntityType::Barracks;
                          }) == 1);
    REQUIRE(build.FindPlayer(0)->resources.material == 830);
    REQUIRE(build.FindPlayer(0)->resources.dawnshards == 480);
}

void TestSnapshotAdversarialBoundsAndIdExhaustion() {
    constexpr std::size_t mapTiles = 8 * 8;
    Simulation source({8, 8, 20, 1});
    REQUIRE(source.AddPlayer(0, Faction::MeridianCompact, {500, 100}));
    const EntityId worker = source.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Worker, Vec2::FromTiles(2, 2));
    const std::vector<std::uint8_t> baseline = source.SaveSnapshot();
    std::string error;

    std::vector<std::uint8_t> excessiveVision = baseline;
    WriteU32(excessiveVision, SnapshotFirstEntityOffset(mapTiles) + 27, 50000);
    ResignSnapshot(excessiveVision);
    REQUIRE(!Simulation::LoadSnapshot(excessiveVision, &error).has_value());
    REQUIRE(error == "snapshot entity state is invalid");

    std::vector<std::uint8_t> excessiveTick = baseline;
    WriteU64(excessiveTick, 1670, std::numeric_limits<std::uint64_t>::max());
    ResignSnapshot(excessiveTick);
    REQUIRE(!Simulation::LoadSnapshot(excessiveTick, &error).has_value());

    std::vector<std::uint8_t> oversizedMap = baseline;
    WriteU32(oversizedMap, 8, 2048);
    WriteU32(oversizedMap, 12, 2048);
    ResignSnapshot(oversizedMap);
    REQUIRE(!Simulation::LoadSnapshot(oversizedMap, &error).has_value());
    REQUIRE(error == "snapshot payload is too short for its declared map");

    Simulation empty({8, 8, 20, 1});
    REQUIRE(empty.AddPlayer(0, Faction::MeridianCompact, {0, 0}));
    std::vector<std::uint8_t> truncatedEntities = empty.SaveSnapshot();
    WriteU32(truncatedEntities, SnapshotEntityCountOffset(mapTiles), 64 * 1024);
    ResignSnapshot(truncatedEntities);
    REQUIRE(!Simulation::LoadSnapshot(truncatedEntities, &error).has_value());
    REQUIRE(error == "snapshot entity count is invalid");

    std::vector<std::uint8_t> exhaustedIds = baseline;
    WriteU32(exhaustedIds, 1678, std::numeric_limits<std::uint32_t>::max());
    ResignSnapshot(exhaustedIds);
    std::optional<Simulation> exhausted =
        Simulation::LoadSnapshot(exhaustedIds, &error);
    REQUIRE(exhausted.has_value());
    const std::size_t entityCount = exhausted->Entities().size();
    REQUIRE(exhausted->SpawnEntity(0, Faction::MeridianCompact,
                                     EntityType::Worker,
                                     Vec2::FromTiles(3, 2)) == 0);
    REQUIRE(exhausted->Entities().size() == entityCount);
    const std::int32_t material = exhausted->FindPlayer(0)->resources.material;
    Command build = MakeCommand(0, 0, 1, CommandType::Build, worker);
    build.position = Vec2::FromTiles(5, 5);
    build.buildType = EntityType::Barracks;
    REQUIRE(exhausted->QueueCommand(build));
    exhausted->Step();
    REQUIRE(exhausted->Entities().size() == entityCount);
    REQUIRE(exhausted->FindPlayer(0)->resources.material == material);
    REQUIRE(Simulation::LoadSnapshot(exhausted->SaveSnapshot(), &error).has_value());
}

void TestAuthoredRulesDriveSimulationAndPersist() {
    SimulationConfig config{20, 20, 20, 0x415554484f524544ULL};
    config.rules.contentSha256[0] = 0x46;
    EntityArchetypeRules& workerRules =
        config.rules.archetypes[0][static_cast<std::size_t>(EntityType::Worker)];
    workerRules.cost = {17, 3};
    workerRules.maxHitPoints = 137;
    workerRules.movementPerTickRaw = 205;
    workerRules.visionTiles = 9;
    workerRules.workRate = 7;
    workerRules.cargoCapacity = 13;
    workerRules.populationCost = 2;
    workerRules.productionTicks = 7;
    config.rules.futureWell.harvestImmediateDawn = 77;
    config.rules.futureWell.preserveDawnPerInterval = 5;
    config.rules.futureWell.preserveIntervalTicks = 4;
    config.rules.futureWell.preserveVisionTiles = 11;
    config.rules.futureWell.reshapeDawnCost = 31;
    config.rules.futureWell.reshapeDurationMinimumTicks = 12;
    config.rules.futureWell.reshapeDurationMaximumTicks = 12;

    Simulation simulation(config);
    REQUIRE(simulation.AddPlayer(0, Faction::MeridianCompact, {100, 40}));
    const EntityId core = simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::CommandCore,
        Vec2::FromTiles(5, 5));
    const EntityId worker = simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Worker,
        Vec2::FromTiles(8, 8));
    REQUIRE(core != 0 && worker != 0);
    const Entity* workerState = simulation.FindEntity(worker);
    REQUIRE(workerState != nullptr);
    REQUIRE(workerState->maxHitPoints == 137);
    REQUIRE(workerState->movementPerTickRaw == 205);
    REQUIRE(workerState->visionTiles == 9);
    REQUIRE(workerState->workRate == 7);
    REQUIRE(workerState->cargoCapacity == 13);
    REQUIRE(simulation.PopulationUsed(0) == 2);

    Command production = MakeCommand(0, 0, 1, CommandType::Produce, core);
    production.buildType = EntityType::Worker;
    REQUIRE(simulation.QueueCommand(production));
    simulation.Step();
    REQUIRE(simulation.FindPlayer(0)->resources.material == 83);
    REQUIRE(simulation.FindPlayer(0)->resources.dawnshards == 37);
    REQUIRE(simulation.FindEntity(core)->productionRequired == 7);

    const std::vector<std::uint8_t> snapshot = simulation.SaveSnapshot();
    std::string error;
    std::optional<Simulation> restored =
        Simulation::LoadSnapshot(snapshot, &error);
    REQUIRE(restored.has_value());
    REQUIRE(error.empty());
    REQUIRE(restored->Config() == config);
    REQUIRE(restored->StateChecksum() == simulation.StateChecksum());

    Simulation wellSimulation(config);
    REQUIRE(wellSimulation.AddPlayer(0, Faction::MeridianCompact, {0, 0}));
    const EntityId wellWorker = wellSimulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Worker,
        Vec2::FromTiles(5, 6));
    const EntityId well =
        wellSimulation.SpawnFutureWell(Vec2::FromTiles(6, 6));
    Command harvest =
        MakeCommand(0, 0, 1, CommandType::FutureWell, wellWorker);
    harvest.target = well;
    harvest.wellChoice = FutureWellChoice::Harvest;
    REQUIRE(wellSimulation.QueueCommand(harvest));
    wellSimulation.Step();
    REQUIRE(wellSimulation.FindPlayer(0)->resources.dawnshards == 77);

    SimulationConfig invalid = config;
    invalid.rules.version = 2;
    bool rejected = false;
    try {
        Simulation invalidSimulation(invalid);
        (void)invalidSimulation;
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    REQUIRE(rejected);
}

void TestCompleteRosterEntityTypesAndProduction() {
    Simulation simulation({40, 40, 20, 0x524f535445523136ULL});
    AddTwoPlayers(simulation, {1000, 500}, {1000, 500});
    const EntityId core = simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::CommandCore,
        Vec2::FromTiles(5, 5));
    const EntityId barracks = simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Barracks,
        Vec2::FromTiles(10, 5));
    const EntityId worker = simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Worker,
        Vec2::FromTiles(8, 10));
    const EntityId heavy = simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::HeavyUnit,
        Vec2::FromTiles(12, 10));
    const EntityId scout = simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::ScoutUnit,
        Vec2::FromTiles(14, 10));
    const EntityId enemyUtility = simulation.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::UtilityStructure,
        Vec2::FromTiles(32, 32));
    REQUIRE(core != 0 && barracks != 0 && worker != 0 && heavy != 0 &&
            scout != 0 && enemyUtility != 0);
    REQUIRE(simulation.FindEntity(heavy)->attackDamage == 10);
    REQUIRE(simulation.FindEntity(scout)->visionTiles == 15);
    REQUIRE(simulation.FindEntity(enemyUtility)->visionTiles == 9);
    REQUIRE(simulation.PopulationUsed(0) == 5);
    REQUIRE(simulation.ValidateProduction(
                0, barracks, EntityType::HeavyUnit) == ProductionResult::Valid);
    REQUIRE(simulation.ValidateProduction(
                0, barracks, EntityType::ScoutUnit) == ProductionResult::Valid);

    Command produce = MakeCommand(0, 0, 1, CommandType::Produce, barracks);
    produce.buildType = EntityType::HeavyUnit;
    REQUIRE(simulation.QueueCommand(produce));
    simulation.Step(140);
    REQUIRE(std::count_if(
                simulation.Entities().begin(), simulation.Entities().end(),
                [](const Entity& entity) {
                    return entity.owner == 0 &&
                           entity.type == EntityType::HeavyUnit;
                }) == 2);

    Command build = MakeCommand(
        simulation.CurrentTick(), 0, 2, CommandType::Build, worker);
    build.position = Vec2::FromTiles(20, 20);
    build.buildType = EntityType::UtilityStructure;
    const std::int32_t materialBefore =
        simulation.FindPlayer(0)->resources.material;
    const std::int32_t dawnBefore =
        simulation.FindPlayer(0)->resources.dawnshards;
    REQUIRE(simulation.QueueCommand(build));
    simulation.Step();
    REQUIRE(simulation.FindPlayer(0)->resources.material == materialBefore - 130);
    REQUIRE(simulation.FindPlayer(0)->resources.dawnshards == dawnBefore - 30);
    REQUIRE(std::any_of(
        simulation.Entities().begin(), simulation.Entities().end(),
        [](const Entity& entity) {
            return entity.owner == 0 &&
                   entity.type == EntityType::UtilityStructure &&
                   !entity.completed;
        }));

    std::string error;
    std::optional<Simulation> restored =
        Simulation::LoadSnapshot(simulation.SaveSnapshot(), &error);
    REQUIRE(restored.has_value());
    REQUIRE(error.empty());
    REQUIRE(restored->StateChecksum() == simulation.StateChecksum());
}

void TestBulwarkDirectionalCoverDeployment() {
    Simulation simulation({24, 24, 20, 0x42554c5741524bULL});
    AddTwoPlayers(simulation, {0, 0}, {0, 0});
    const EntityId bulwark = simulation.SpawnEntity(
        0,
        Faction::MeridianCompact,
        EntityType::HeavyUnit,
        Vec2::FromTiles(10, 10));
    const EntityId protectedLancer = simulation.SpawnEntity(
        0,
        Faction::MeridianCompact,
        EntityType::Soldier,
        Vec2::FromRaw(9 * kFixedScale + kFixedScale / 2,
                      10 * kFixedScale));
    const EntityId attacker = simulation.SpawnEntity(
        1,
        Faction::KharuunAssemblies,
        EntityType::Soldier,
        Vec2::FromTiles(11, 10));
    REQUIRE(bulwark != 0 && protectedLancer != 0 && attacker != 0);
    simulation.CaptureReplayBaseline();

    Command deploy =
        MakeCommand(0, 0, 1, CommandType::ToggleDeploy, bulwark);
    deploy.position = Vec2::FromTiles(12, 10);
    Command attack =
        MakeCommand(0, 1, 1, CommandType::Attack, attacker);
    attack.target = protectedLancer;
    REQUIRE(simulation.QueueCommand(deploy));
    REQUIRE(simulation.QueueCommand(attack));
    const std::int32_t healthBefore =
        simulation.FindEntity(protectedLancer)->hitPoints;
    simulation.Step();

    const Entity* deployed = simulation.FindEntity(bulwark);
    REQUIRE(deployed != nullptr && deployed->deployed);
    REQUIRE(deployed->deploymentFacing == Vec2::FromRaw(kFixedScale, 0));
    const std::optional<PlayerView> opponentView = simulation.CreatePlayerView(1);
    REQUIRE(opponentView.has_value());
    const auto observedBulwark = std::find_if(
        opponentView->Entities().begin(),
        opponentView->Entities().end(),
        [bulwark](const Entity& entity) { return entity.id == bulwark; });
    REQUIRE(observedBulwark != opponentView->Entities().end());
    REQUIRE(observedBulwark->deployed);
    REQUIRE(observedBulwark->deploymentFacing == Vec2::FromRaw(kFixedScale, 0));
    REQUIRE(observedBulwark->hitPoints == 1);
    const std::int32_t attackDamage =
        simulation.Config()
            .rules.archetypes[static_cast<std::size_t>(Faction::KharuunAssemblies)]
                             [static_cast<std::size_t>(EntityType::Soldier)]
            .attackDamage;
    const std::int32_t reducedDamage = std::max(
        1,
        attackDamage *
            (100 - simulation.Config().rules.bulwarkDeployment.damageReductionPercent) /
            100);
    REQUIRE(simulation.FindEntity(protectedLancer)->hitPoints ==
            healthBefore - reducedDamage);

    std::string error;
    std::optional<Simulation> restored =
        Simulation::LoadSnapshot(simulation.SaveSnapshot(), &error);
    REQUIRE(restored.has_value());
    REQUIRE(restored->FindEntity(bulwark)->deployed);
    REQUIRE(restored->FindEntity(bulwark)->deploymentFacing ==
            Vec2::FromRaw(kFixedScale, 0));
    REQUIRE(restored->Config().rules.bulwarkDeployment ==
            simulation.Config().rules.bulwarkDeployment);

    const std::int32_t deployedStartX = deployed->position.x.Raw();
    const std::int32_t baseMovement = deployed->movementPerTickRaw;
    Command slowMove =
        MakeCommand(simulation.CurrentTick(), 0, 2, CommandType::Move, bulwark);
    slowMove.position = Vec2::FromTiles(14, 10);
    REQUIRE(simulation.QueueCommand(slowMove));
    simulation.Step();
    const std::int32_t deployedTravel =
        simulation.FindEntity(bulwark)->position.x.Raw() - deployedStartX;
    REQUIRE(deployedTravel == std::max(
        1,
        baseMovement *
            simulation.Config().rules.bulwarkDeployment.deployedMovementPercent /
            100));

    Command undeploy = MakeCommand(
        simulation.CurrentTick(), 0, 3, CommandType::ToggleDeploy, bulwark);
    REQUIRE(simulation.QueueCommand(undeploy));
    const std::int32_t undeployedStartX =
        simulation.FindEntity(bulwark)->position.x.Raw();
    simulation.Step();
    REQUIRE(!simulation.FindEntity(bulwark)->deployed);
    REQUIRE(simulation.FindEntity(bulwark)->position.x.Raw() - undeployedStartX ==
            baseMovement);

    const ReplayRecord replay = simulation.ExportReplay();
    std::optional<Simulation> replayed = Simulation::ReplayToEnd(replay, &error);
    REQUIRE(replayed.has_value());
    REQUIRE(replayed->StateChecksum() == simulation.StateChecksum());

    Simulation flank({24, 24, 20, 7});
    AddTwoPlayers(flank, {0, 0}, {0, 0});
    const EntityId flankBulwark = flank.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::HeavyUnit,
        Vec2::FromTiles(10, 10));
    const EntityId flankLancer = flank.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Soldier,
        Vec2::FromRaw(9 * kFixedScale + kFixedScale / 2,
                      10 * kFixedScale));
    const EntityId flankAttacker = flank.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::Soldier,
        Vec2::FromRaw(9 * kFixedScale + kFixedScale / 2,
                      11 * kFixedScale));
    Command faceEast =
        MakeCommand(0, 0, 1, CommandType::ToggleDeploy, flankBulwark);
    faceEast.position = Vec2::FromTiles(12, 10);
    Command flankAttack =
        MakeCommand(0, 1, 1, CommandType::Attack, flankAttacker);
    flankAttack.target = flankLancer;
    REQUIRE(flank.QueueCommand(faceEast));
    REQUIRE(flank.QueueCommand(flankAttack));
    const std::int32_t flankHealth = flank.FindEntity(flankLancer)->hitPoints;
    flank.Step();
    REQUIRE(flank.FindEntity(flankLancer)->hitPoints ==
            flankHealth - attackDamage);

    const EntityId cairnback = flank.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::HeavyUnit,
        Vec2::FromTiles(15, 15));
    Command invalidFactionDeploy = MakeCommand(
        flank.CurrentTick(), 1, 2, CommandType::ToggleDeploy, cairnback);
    invalidFactionDeploy.position = Vec2::FromTiles(16, 15);
    REQUIRE(flank.QueueCommand(invalidFactionDeploy));
    flank.Step();
    REQUIRE(!flank.FindEntity(cairnback)->deployed);
}

void TestRelaySupplyExtensionLifecycle() {
    Simulation simulation({24, 24, 20, 0x52454c4159535550ULL});
    AddTwoPlayers(simulation, {1000, 500}, {0, 0});
    const EntityId core = simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::CommandCore,
        Vec2::FromTiles(4, 4));
    const EntityId foundry = simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Barracks,
        Vec2::FromTiles(5, 4));
    const EntityId relay = simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::ScoutUnit,
        Vec2::FromTiles(6, 4));
    const EntityId disconnectedRelay = simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::ScoutUnit,
        Vec2::FromTiles(20, 20));
    for (std::int32_t index = 0; index < 5; ++index) {
        REQUIRE(simulation.SpawnEntity(
                    0, Faction::MeridianCompact, EntityType::Soldier,
                    Vec2::FromTiles(4 + index, 6)) != 0);
    }
    const EntityId observer = simulation.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::ScoutUnit,
        Vec2::FromTiles(8, 4));
    REQUIRE(core != 0 && foundry != 0 && relay != 0 &&
            disconnectedRelay != 0 && observer != 0);
    REQUIRE(simulation.PopulationUsed(0) == 12);
    REQUIRE(simulation.PopulationCapacity(0) == 12);
    REQUIRE(simulation.ValidateProduction(0, foundry, EntityType::HeavyUnit) ==
            ProductionResult::CapacityReached);
    REQUIRE(simulation.ValidateRelaySupply(0, disconnectedRelay) ==
            RelaySupplyResult::Disconnected);

    simulation.CaptureReplayBaseline();
    Command activate =
        MakeCommand(0, 0, 1, CommandType::ActivateRelaySupply, relay);
    REQUIRE(simulation.QueueCommand(activate));
    simulation.Step();
    const Entity* activeRelay = simulation.FindEntity(relay);
    REQUIRE(activeRelay != nullptr && activeRelay->relaySupplyActive);
    REQUIRE(activeRelay->relaySupplyUntilTick ==
            simulation.Config().rules.relaySupply.durationTicks);
    REQUIRE(activeRelay->relaySupplyCooldownUntilTick ==
            simulation.Config().rules.relaySupply.cooldownTicks);
    REQUIRE(simulation.PopulationCapacity(0) == 16);
    REQUIRE(simulation.ValidateProduction(0, foundry, EntityType::HeavyUnit) ==
            ProductionResult::Valid);
    REQUIRE(simulation.ValidateRelaySupply(0, relay) ==
            RelaySupplyResult::AlreadyActive);

    const std::optional<PlayerView> opponentView = simulation.CreatePlayerView(1);
    REQUIRE(opponentView.has_value());
    const auto observedRelay = std::find_if(
        opponentView->Entities().begin(),
        opponentView->Entities().end(),
        [relay](const Entity& entity) { return entity.id == relay; });
    REQUIRE(observedRelay != opponentView->Entities().end());
    REQUIRE(observedRelay->relaySupplyActive);
    REQUIRE(observedRelay->relaySupplyUntilTick == 0);
    REQUIRE(observedRelay->relaySupplyCooldownUntilTick == 0);

    std::string error;
    std::optional<Simulation> restored =
        Simulation::LoadSnapshot(simulation.SaveSnapshot(), &error);
    REQUIRE(restored.has_value());
    REQUIRE(restored->FindEntity(relay)->relaySupplyActive);
    REQUIRE(restored->Config().rules.relaySupply ==
            simulation.Config().rules.relaySupply);

    simulation.Step(simulation.Config().rules.relaySupply.durationTicks - 1);
    REQUIRE(simulation.CurrentTick() ==
            simulation.Config().rules.relaySupply.durationTicks);
    REQUIRE(!simulation.FindEntity(relay)->relaySupplyActive);
    REQUIRE(simulation.FindEntity(relay)->relaySupplyUntilTick == 0);
    REQUIRE(simulation.PopulationCapacity(0) == 12);
    REQUIRE(simulation.ValidateRelaySupply(0, relay) ==
            RelaySupplyResult::CooldownActive);

    simulation.Step(
        simulation.Config().rules.relaySupply.cooldownTicks -
        simulation.CurrentTick());
    REQUIRE(simulation.ValidateRelaySupply(0, relay) ==
            RelaySupplyResult::Valid);

    const ReplayRecord replay = simulation.ExportReplay();
    std::optional<Simulation> replayed = Simulation::ReplayToEnd(replay, &error);
    REQUIRE(replayed.has_value());
    REQUIRE(replayed->StateChecksum() == simulation.StateChecksum());

    REQUIRE(simulation.ValidateRelaySupply(1, observer) ==
            RelaySupplyResult::InvalidActor);

    SimulationConfig severedConfig{16, 16, 20, 9};
    severedConfig.rules
        .archetypes[static_cast<std::size_t>(Faction::KharuunAssemblies)]
                   [static_cast<std::size_t>(EntityType::Soldier)]
        .attackDamage = 2000;
    Simulation severed(severedConfig);
    AddTwoPlayers(severed, {0, 0}, {0, 0});
    const EntityId severedCore = severed.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::CommandCore,
        Vec2::FromTiles(4, 4));
    const EntityId severedRelay = severed.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::ScoutUnit,
        Vec2::FromTiles(6, 4));
    const EntityId coreAttacker = severed.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::Soldier,
        Vec2::FromTiles(5, 4));
    REQUIRE(severedCore != 0 && severedRelay != 0 && coreAttacker != 0);
    Command severedActivate = MakeCommand(
        0, 0, 1, CommandType::ActivateRelaySupply, severedRelay);
    Command severingAttack = MakeCommand(
        0, 1, 1, CommandType::Attack, coreAttacker);
    severingAttack.target = severedCore;
    REQUIRE(severed.QueueCommand(severedActivate));
    REQUIRE(severed.QueueCommand(severingAttack));
    severed.Step();
    REQUIRE(severed.FindEntity(severedCore) == nullptr);
    REQUIRE(!severed.FindEntity(severedRelay)->relaySupplyActive);
    REQUIRE(severed.FindEntity(severedRelay)->relaySupplyCooldownUntilTick >
            severed.CurrentTick());
    REQUIRE(severed.PopulationCapacity(0) == 0);
}

void TestWaystoneMigrationAndRooting() {
    Simulation simulation({24, 24, 20, 0x57415953544f4e45ULL});
    AddTwoPlayers(simulation, {0, 0}, {1000, 500});
    const EntityId memoryHearth = simulation.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::CommandCore,
        Vec2::FromTiles(3, 3));
    const EntityId waystone = simulation.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::Dropoff,
        Vec2::FromTiles(10, 10));
    const EntityId attacker = simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Soldier,
        Vec2::FromTiles(11, 10));
    REQUIRE(memoryHearth != 0 && waystone != 0 && attacker != 0);
    REQUIRE(simulation.FindEntity(waystone)->waystoneMode ==
            WaystoneMode::Rooted);
    REQUIRE(simulation.PopulationCapacity(1) == 17);
    REQUIRE(simulation.ValidateWaystoneRoot(1, waystone) ==
            WaystoneRootResult::Valid);
    REQUIRE(simulation.ValidateWaystoneRoot(0, attacker) ==
            WaystoneRootResult::InvalidActor);
    simulation.CaptureReplayBaseline();

    Command uproot = MakeCommand(
        0, 1, 1, CommandType::ToggleWaystoneRoot, waystone);
    REQUIRE(simulation.QueueCommand(uproot));
    simulation.Step();
    REQUIRE(simulation.FindEntity(waystone)->waystoneMode ==
            WaystoneMode::Uprooting);
    REQUIRE(simulation.FindEntity(waystone)->waystoneTransitionUntilTick ==
            simulation.Config().rules.waystoneMigration.uprootTicks);
    REQUIRE(simulation.PopulationCapacity(1) == 12);
    REQUIRE(simulation.ValidateWaystoneRoot(1, waystone) ==
            WaystoneRootResult::TransitionActive);

    Command prematureMove = MakeCommand(
        simulation.CurrentTick(), 1, 2, CommandType::Move, waystone);
    prematureMove.position = Vec2::FromTiles(14, 10);
    REQUIRE(simulation.QueueCommand(prematureMove));
    const Vec2 rootedPosition = simulation.FindEntity(waystone)->position;
    simulation.Step();
    REQUIRE(simulation.FindEntity(waystone)->position == rootedPosition);
    REQUIRE(simulation.FindEntity(waystone)->order.type == OrderType::None);

    simulation.Step(
        simulation.Config().rules.waystoneMigration.uprootTicks -
        simulation.CurrentTick());
    REQUIRE(simulation.FindEntity(waystone)->waystoneMode ==
            WaystoneMode::Mobile);
    REQUIRE(simulation.FindEntity(waystone)->waystoneTransitionUntilTick == 0);

    const std::int32_t mobileHealth = simulation.FindEntity(waystone)->hitPoints;
    Command migrate = MakeCommand(
        simulation.CurrentTick(), 1, 3, CommandType::Move, waystone);
    migrate.position = Vec2::FromTiles(14, 10);
    Command exposeAttack = MakeCommand(
        simulation.CurrentTick(), 0, 1, CommandType::Attack, attacker);
    exposeAttack.target = waystone;
    REQUIRE(simulation.QueueCommand(migrate));
    REQUIRE(simulation.QueueCommand(exposeAttack));
    const std::int32_t mobileStartX =
        simulation.FindEntity(waystone)->position.x.Raw();
    simulation.Step();
    REQUIRE(simulation.FindEntity(waystone)->position.x.Raw() - mobileStartX ==
            simulation.Config().rules.waystoneMigration.movementPerTickRaw);
    const std::int32_t baseDamage =
        simulation.Config()
            .rules.archetypes[static_cast<std::size_t>(Faction::MeridianCompact)]
                             [static_cast<std::size_t>(EntityType::Soldier)]
            .attackDamage;
    REQUIRE(simulation.FindEntity(waystone)->hitPoints ==
            mobileHealth - baseDamage *
                simulation.Config().rules.waystoneMigration
                    .mobileDamageTakenPercent /
                100);

    const Entity* mobileWaystone = simulation.FindEntity(waystone);
    REQUIRE(simulation.SetTerrainTile(
        mobileWaystone->position.x.FloorToInt(),
        mobileWaystone->position.y.FloorToInt(),
        Terrain::Blocked));
    REQUIRE(simulation.ValidateWaystoneRoot(1, waystone) ==
            WaystoneRootResult::RootingBlocked);
    REQUIRE(simulation.SetTerrainTile(
        mobileWaystone->position.x.FloorToInt(),
        mobileWaystone->position.y.FloorToInt(),
        Terrain::Open));

    Command stopAttack = MakeCommand(
        simulation.CurrentTick(), 0, 2, CommandType::Stop, attacker);
    Command root = MakeCommand(
        simulation.CurrentTick(), 1, 4, CommandType::ToggleWaystoneRoot,
        waystone);
    REQUIRE(simulation.QueueCommand(stopAttack));
    REQUIRE(simulation.QueueCommand(root));
    simulation.Step();
    REQUIRE(simulation.FindEntity(waystone)->waystoneMode ==
            WaystoneMode::Rooting);
    REQUIRE(simulation.PopulationCapacity(1) == 12);

    const std::optional<PlayerView> opponentView = simulation.CreatePlayerView(0);
    REQUIRE(opponentView.has_value());
    const auto observedWaystone = std::find_if(
        opponentView->Entities().begin(), opponentView->Entities().end(),
        [waystone](const Entity& entity) { return entity.id == waystone; });
    REQUIRE(observedWaystone != opponentView->Entities().end());
    REQUIRE(observedWaystone->waystoneMode == WaystoneMode::Rooting);
    REQUIRE(observedWaystone->waystoneTransitionUntilTick == 0);
    REQUIRE(observedWaystone->hitPoints == 1);

    std::string error;
    std::optional<Simulation> restored =
        Simulation::LoadSnapshot(simulation.SaveSnapshot(), &error);
    REQUIRE(restored.has_value());
    REQUIRE(restored->FindEntity(waystone)->waystoneMode ==
            WaystoneMode::Rooting);
    REQUIRE(restored->Config().rules.waystoneMigration ==
            simulation.Config().rules.waystoneMigration);

    const Tick ticksUntilRooted =
        simulation.FindEntity(waystone)->waystoneTransitionUntilTick -
        simulation.CurrentTick();
    simulation.Step(ticksUntilRooted);
    REQUIRE(simulation.FindEntity(waystone)->waystoneMode ==
            WaystoneMode::Rooted);
    REQUIRE(simulation.FindEntity(waystone)->waystoneTransitionUntilTick == 0);
    REQUIRE(simulation.FindEntity(waystone)->order.type == OrderType::None);
    REQUIRE(simulation.PopulationCapacity(1) == 17);

    const ReplayRecord replay = simulation.ExportReplay();
    std::optional<Simulation> replayed = Simulation::ReplayToEnd(replay, &error);
    REQUIRE(replayed.has_value());
    REQUIRE(replayed->StateChecksum() == simulation.StateChecksum());
}

void TestWarformAdaptationAndMoltCounterplay() {
    Simulation unfunded({16, 16, 20, 0x554e46554e444544ULL});
    AddTwoPlayers(unfunded, {0, 0}, {0, 24});
    const EntityId unfundedBasin = unfunded.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::Barracks,
        Vec2::FromTiles(8, 8));
    const EntityId unfundedWarform = unfunded.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::Soldier,
        Vec2::FromTiles(9, 8));
    REQUIRE(unfundedBasin != 0 && unfundedWarform != 0);
    REQUIRE(unfunded.ValidateWarformAdaptation(
                1, unfundedWarform, unfundedBasin,
                WarformAdaptation::Carapace) ==
            WarformAdaptationResult::InsufficientDawn);

    Simulation simulation({24, 24, 20, 0x4d4f4c5453495445ULL});
    AddTwoPlayers(simulation, {0, 0}, {1000, 500});
    const EntityId basin = simulation.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::Barracks,
        Vec2::FromTiles(10, 10));
    const EntityId warform = simulation.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::Soldier,
        Vec2::FromTiles(11, 10));
    const EntityId distantWarform = simulation.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::HeavyUnit,
        Vec2::FromTiles(20, 20));
    const EntityId attacker = simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Soldier,
        Vec2::FromTiles(12, 10));
    REQUIRE(basin != 0 && warform != 0 && distantWarform != 0 &&
            attacker != 0);
    REQUIRE(simulation.ValidateWarformAdaptation(
                1, warform, basin, WarformAdaptation::Carapace) ==
            WarformAdaptationResult::Valid);
    REQUIRE(simulation.ValidateWarformAdaptation(
                3, warform, basin, WarformAdaptation::Carapace) ==
            WarformAdaptationResult::InvalidPlayer);
    REQUIRE(simulation.ValidateWarformAdaptation(
                0, attacker, basin, WarformAdaptation::Carapace) ==
            WarformAdaptationResult::InvalidActor);
    REQUIRE(simulation.ValidateWarformAdaptation(
                1, warform, basin, WarformAdaptation::None) ==
            WarformAdaptationResult::InvalidAdaptation);
    REQUIRE(simulation.ValidateWarformAdaptation(
                1, warform, warform, WarformAdaptation::Carapace) ==
            WarformAdaptationResult::InvalidSite);
    REQUIRE(simulation.ValidateWarformAdaptation(
                1, distantWarform, basin, WarformAdaptation::Carapace) ==
            WarformAdaptationResult::OutsideSiteRadius);
    const std::optional<PlayerView> adaptiveView = simulation.CreatePlayerView(1);
    REQUIRE(adaptiveView.has_value());
    const std::vector<Command> adaptiveCommands =
        Simulation::GenerateAiCommands(*adaptiveView, AiPersonality::Adaptive);
    REQUIRE(std::any_of(
        adaptiveCommands.begin(), adaptiveCommands.end(),
        [warform, basin](const Command& command) {
            return command.type == CommandType::AdaptWarform &&
                   command.actor == warform && command.target == basin &&
                   command.warformAdaptation == WarformAdaptation::Striker;
        }));

    const Entity* baseline = simulation.FindEntity(warform);
    REQUIRE(baseline != nullptr);
    const std::int32_t baseHealth = baseline->maxHitPoints;
    const std::int32_t baseMovement = baseline->movementPerTickRaw;
    const std::int32_t baseDamage = baseline->attackDamage;
    const Tick baseCooldown = baseline->attackPeriodTicks;
    const std::int32_t startingDawn =
        simulation.FindPlayer(1)->resources.dawnshards;
    simulation.CaptureReplayBaseline();

    Command adapt = MakeCommand(
        0, 1, 1, CommandType::AdaptWarform, warform);
    adapt.target = basin;
    adapt.warformAdaptation = WarformAdaptation::Carapace;
    Command attack = MakeCommand(0, 0, 1, CommandType::Attack, attacker);
    attack.target = warform;
    REQUIRE(simulation.QueueCommand(adapt));
    REQUIRE(simulation.QueueCommand(attack));
    simulation.Step();

    const Entity* molting = simulation.FindEntity(warform);
    REQUIRE(molting != nullptr);
    REQUIRE(molting->warformAdaptation == WarformAdaptation::None);
    REQUIRE(molting->pendingWarformAdaptation ==
            WarformAdaptation::Carapace);
    REQUIRE(molting->moltSite == basin);
    REQUIRE(molting->moltUntilTick ==
            simulation.Config().rules.warformAdaptation.moltTicks);
    REQUIRE(molting->order.type == OrderType::None);
    REQUIRE(simulation.FindPlayer(1)->resources.dawnshards ==
            startingDawn - simulation.Config().rules.warformAdaptation.dawnCost);
    const std::int32_t enemyDamage =
        simulation.FindEntity(attacker)->attackDamage;
    REQUIRE(molting->hitPoints ==
            baseHealth - enemyDamage *
                simulation.Config().rules.warformAdaptation
                    .moltDamageTakenPercent /
                100);
    REQUIRE(simulation.ValidateWarformAdaptation(
                1, warform, basin, WarformAdaptation::Striker) ==
            WarformAdaptationResult::MoltActive);

    const std::optional<PlayerView> opponentView = simulation.CreatePlayerView(0);
    REQUIRE(opponentView.has_value());
    const auto observedWarform = std::find_if(
        opponentView->Entities().begin(), opponentView->Entities().end(),
        [warform](const Entity& entity) { return entity.id == warform; });
    REQUIRE(observedWarform != opponentView->Entities().end());
    REQUIRE(observedWarform->pendingWarformAdaptation ==
            WarformAdaptation::Carapace);
    REQUIRE(observedWarform->moltSite == basin);
    REQUIRE(observedWarform->moltUntilTick == 0);
    REQUIRE(observedWarform->hitPoints == 1);

    Command blockedMove = MakeCommand(
        simulation.CurrentTick(), 1, 2, CommandType::Move, warform);
    blockedMove.position = Vec2::FromTiles(15, 10);
    Command stopAttack = MakeCommand(
        simulation.CurrentTick(), 0, 2, CommandType::Stop, attacker);
    REQUIRE(simulation.QueueCommand(blockedMove));
    REQUIRE(simulation.QueueCommand(stopAttack));
    const Vec2 moltPosition = molting->position;
    simulation.Step();
    REQUIRE(simulation.FindEntity(warform)->position == moltPosition);
    REQUIRE(simulation.FindEntity(warform)->order.type == OrderType::None);

    std::string error;
    std::optional<Simulation> restored =
        Simulation::LoadSnapshot(simulation.SaveSnapshot(), &error);
    REQUIRE(restored.has_value());
    REQUIRE(restored->FindEntity(warform)->pendingWarformAdaptation ==
            WarformAdaptation::Carapace);
    REQUIRE(restored->Config().rules.warformAdaptation ==
            simulation.Config().rules.warformAdaptation);

    simulation.Step(
        simulation.FindEntity(warform)->moltUntilTick -
        simulation.CurrentTick());
    const Entity* carapace = simulation.FindEntity(warform);
    REQUIRE(carapace != nullptr);
    REQUIRE(carapace->warformAdaptation == WarformAdaptation::Carapace);
    REQUIRE(carapace->pendingWarformAdaptation == WarformAdaptation::None);
    REQUIRE(carapace->moltSite == 0 && carapace->moltUntilTick == 0);
    REQUIRE(carapace->maxHitPoints ==
            baseHealth *
                simulation.Config().rules.warformAdaptation
                    .carapaceHealthPercent /
                100);
    REQUIRE(carapace->movementPerTickRaw ==
            baseMovement *
                simulation.Config().rules.warformAdaptation
                    .carapaceMovementPercent /
                100);
    REQUIRE(simulation.ValidateWarformAdaptation(
                1, warform, basin, WarformAdaptation::Carapace) ==
            WarformAdaptationResult::AlreadyAdapted);

    Command striker = MakeCommand(
        simulation.CurrentTick(), 1, 3, CommandType::AdaptWarform, warform);
    striker.target = basin;
    striker.warformAdaptation = WarformAdaptation::Striker;
    REQUIRE(simulation.QueueCommand(striker));
    simulation.Step();
    REQUIRE(simulation.FindEntity(warform)->maxHitPoints == baseHealth);
    REQUIRE(simulation.FindEntity(warform)->warformAdaptation ==
            WarformAdaptation::None);
    simulation.Step(
        simulation.FindEntity(warform)->moltUntilTick -
        simulation.CurrentTick());
    const Entity* strikerForm = simulation.FindEntity(warform);
    REQUIRE(strikerForm != nullptr);
    REQUIRE(strikerForm->warformAdaptation == WarformAdaptation::Striker);
    REQUIRE(strikerForm->maxHitPoints == baseHealth);
    REQUIRE(strikerForm->movementPerTickRaw == baseMovement);
    REQUIRE(strikerForm->attackDamage ==
            baseDamage *
                simulation.Config().rules.warformAdaptation
                    .strikerDamagePercent /
                100);
    REQUIRE(strikerForm->attackPeriodTicks ==
            baseCooldown *
                simulation.Config().rules.warformAdaptation
                    .strikerCooldownPercent /
                100);

    const ReplayRecord replay = simulation.ExportReplay();
    std::optional<Simulation> replayed = Simulation::ReplayToEnd(replay, &error);
    REQUIRE(replayed.has_value());
    REQUIRE(replayed->StateChecksum() == simulation.StateChecksum());

    SimulationConfig cancelConfig{20, 20, 20, 0x43414e43454cULL};
    cancelConfig.rules
        .archetypes[static_cast<std::size_t>(Faction::MeridianCompact)]
                   [static_cast<std::size_t>(EntityType::Soldier)]
        .attackDamage = 2000;
    Simulation cancelled(cancelConfig);
    AddTwoPlayers(cancelled, {0, 0}, {0, 100});
    const EntityId cancelBasin = cancelled.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::Barracks,
        Vec2::FromTiles(10, 10));
    const EntityId cancelWarform = cancelled.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::HeavyUnit,
        Vec2::FromTiles(9, 10));
    const EntityId siteAttacker = cancelled.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Soldier,
        Vec2::FromTiles(11, 10));
    REQUIRE(cancelBasin != 0 && cancelWarform != 0 && siteAttacker != 0);
    Command cancelMolt = MakeCommand(
        0, 1, 1, CommandType::AdaptWarform, cancelWarform);
    cancelMolt.target = cancelBasin;
    cancelMolt.warformAdaptation = WarformAdaptation::Carapace;
    Command destroySite = MakeCommand(
        0, 0, 1, CommandType::Attack, siteAttacker);
    destroySite.target = cancelBasin;
    REQUIRE(cancelled.QueueCommand(cancelMolt));
    REQUIRE(cancelled.QueueCommand(destroySite));
    cancelled.Step();
    REQUIRE(cancelled.FindEntity(cancelBasin) == nullptr);
    REQUIRE(cancelled.FindEntity(cancelWarform)->warformAdaptation ==
            WarformAdaptation::None);
    REQUIRE(cancelled.FindEntity(cancelWarform)->pendingWarformAdaptation ==
            WarformAdaptation::None);
    REQUIRE(cancelled.FindEntity(cancelWarform)->moltSite == 0);
    REQUIRE(cancelled.FindEntity(cancelWarform)->moltUntilTick == 0);
}

void TestCairnbackTemporaryMineralCover() {
    Simulation unfunded({16, 16, 20, 0x434f5645524e4fULL});
    AddTwoPlayers(unfunded, {0, 0}, {0, 14});
    const EntityId unfundedCairnback = unfunded.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::HeavyUnit,
        Vec2::FromTiles(6, 6));
    REQUIRE(unfundedCairnback != 0);
    REQUIRE(unfunded.ValidateMineralCover(
                1, unfundedCairnback, Vec2::FromTiles(7, 6)) ==
            MineralCoverResult::InsufficientDawn);

    Simulation molting({16, 16, 20, 0x434f5645524d4fULL});
    AddTwoPlayers(molting, {0, 0}, {0, 100});
    const EntityId moltBasin = molting.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::Barracks,
        Vec2::FromTiles(6, 6));
    const EntityId moltingCairnback = molting.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::HeavyUnit,
        Vec2::FromTiles(7, 6));
    REQUIRE(moltBasin != 0 && moltingCairnback != 0);
    Command beginMolt = MakeCommand(
        0, 1, 1, CommandType::AdaptWarform, moltingCairnback);
    beginMolt.target = moltBasin;
    beginMolt.warformAdaptation = WarformAdaptation::Carapace;
    REQUIRE(molting.QueueCommand(beginMolt));
    molting.Step();
    REQUIRE(molting.ValidateMineralCover(
                1, moltingCairnback, Vec2::FromTiles(8, 6)) ==
            MineralCoverResult::MoltActive);

    SimulationConfig config{24, 24, 20, 0x4d494e4552414cULL};
    config.rules.mineralCover.durationTicks = 6;
    config.rules.mineralCover.cooldownTicks = 12;
    config.rules.mineralCover.maxHitPoints = 30;
    Simulation simulation(config);
    AddTwoPlayers(simulation, {0, 0}, {0, 100});
    REQUIRE(simulation.SetTerrainTile(9, 10, Terrain::Scarred));
    const EntityId cairnback = simulation.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::HeavyUnit,
        Vec2::FromTiles(10, 10));
    const EntityId expiryCairnback = simulation.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::HeavyUnit,
        Vec2::FromTiles(10, 14));
    const EntityId protectedWarform = simulation.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::Soldier,
        Vec2::FromTiles(8, 10));
    const EntityId attacker = simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Soldier,
        Vec2::FromTiles(12, 10));
    const EntityId counterplayAttacker = simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Soldier,
        Vec2::FromTiles(13, 10));
    const EntityId invalidActor = simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::HeavyUnit,
        Vec2::FromTiles(18, 18));
    REQUIRE(cairnback != 0 && expiryCairnback != 0 &&
            protectedWarform != 0 && attacker != 0 &&
            counterplayAttacker != 0 && invalidActor != 0);
    REQUIRE(simulation.ValidateMineralCover(
                3, cairnback, Vec2::FromTiles(9, 10)) ==
            MineralCoverResult::InvalidPlayer);
    REQUIRE(simulation.ValidateMineralCover(
                0, invalidActor, Vec2::FromTiles(17, 18)) ==
            MineralCoverResult::InvalidActor);
    REQUIRE(simulation.ValidateMineralCover(
                1, cairnback, Vec2::FromTiles(20, 20)) ==
            MineralCoverResult::OutsideCastRange);
    REQUIRE(simulation.ValidateMineralCover(
                1, cairnback, cairnback == 0
                                  ? Vec2{}
                                  : simulation.FindEntity(cairnback)->position) ==
            MineralCoverResult::Occupied);
    REQUIRE(simulation.ValidateMineralCover(
                1, cairnback, Vec2::FromTiles(9, 10)) ==
            MineralCoverResult::Valid);
    const std::int32_t startingDawn =
        simulation.FindPlayer(1)->resources.dawnshards;
    const std::int32_t protectedHealth =
        simulation.FindEntity(protectedWarform)->hitPoints;
    simulation.CaptureReplayBaseline();

    Command raise = MakeCommand(
        0, 1, 1, CommandType::RaiseMineralCover, cairnback);
    raise.position = Vec2::FromTiles(9, 10);
    Command attack = MakeCommand(
        0, 0, 1, CommandType::Attack, attacker);
    attack.target = protectedWarform;
    REQUIRE(simulation.QueueCommand(raise));
    REQUIRE(simulation.QueueCommand(attack));
    simulation.Step();

    const auto coverIt = std::find_if(
        simulation.Entities().begin(), simulation.Entities().end(),
        [](const Entity& entity) { return entity.temporaryMineralCover; });
    REQUIRE(coverIt != simulation.Entities().end());
    const EntityId cover = coverIt->id;
    REQUIRE(coverIt->owner == 1);
    REQUIRE(coverIt->mineralCoverCreator == cairnback);
    REQUIRE(coverIt->maxHitPoints == 30);
    REQUIRE(coverIt->hitPoints ==
            30 - simulation.FindEntity(attacker)->attackDamage);
    REQUIRE(coverIt->mineralCoverUnderlyingTerrain == Terrain::Scarred);
    REQUIRE(coverIt->mineralCoverUntilTick == 6);
    REQUIRE(simulation.TerrainAt(9, 10) == Terrain::Blocked);
    REQUIRE(!simulation.IsPositionPassable(Vec2::FromTiles(9, 10)));
    REQUIRE(simulation.FindEntity(protectedWarform)->hitPoints ==
            protectedHealth);
    REQUIRE(simulation.FindPlayer(1)->resources.dawnshards ==
            startingDawn - simulation.Config().rules.mineralCover.dawnCost);
    REQUIRE(simulation.ValidateMineralCover(
                1, cairnback, Vec2::FromTiles(11, 10)) ==
            MineralCoverResult::CooldownActive);

    const std::optional<PlayerView> opponentView =
        simulation.CreatePlayerView(0);
    REQUIRE(opponentView.has_value());
    const auto observedCover = std::find_if(
        opponentView->Entities().begin(), opponentView->Entities().end(),
        [cover](const Entity& entity) { return entity.id == cover; });
    REQUIRE(observedCover != opponentView->Entities().end());
    REQUIRE(observedCover->temporaryMineralCover);
    REQUIRE(observedCover->hitPoints == 1);
    REQUIRE(observedCover->mineralCoverUntilTick == 0);
    REQUIRE(observedCover->mineralCoverUnderlyingTerrain == Terrain::Open);

    std::string error;
    std::optional<Simulation> restored =
        Simulation::LoadSnapshot(simulation.SaveSnapshot(), &error);
    REQUIRE(restored.has_value());
    REQUIRE(restored->FindEntity(cover) != nullptr);
    REQUIRE(restored->FindEntity(cover)->temporaryMineralCover);
    REQUIRE(restored->Config().rules.mineralCover ==
            simulation.Config().rules.mineralCover);
    REQUIRE(restored->TerrainAt(9, 10) == Terrain::Blocked);

    Command destroyCover = MakeCommand(
        simulation.CurrentTick(), 0, 2, CommandType::Attack,
        counterplayAttacker);
    destroyCover.target = cover;
    REQUIRE(simulation.QueueCommand(destroyCover));
    simulation.Step();
    REQUIRE(simulation.FindEntity(cover) == nullptr);
    REQUIRE(simulation.TerrainAt(9, 10) == Terrain::Scarred);
    REQUIRE(simulation.IsPositionPassable(Vec2::FromTiles(9, 10)));
    REQUIRE(simulation.FindEntity(protectedWarform)->hitPoints ==
            protectedHealth);

    Command expiringCover = MakeCommand(
        simulation.CurrentTick(), 1, 2, CommandType::RaiseMineralCover,
        expiryCairnback);
    expiringCover.position = Vec2::FromTiles(9, 14);
    REQUIRE(simulation.QueueCommand(expiringCover));
    simulation.Step();
    const auto expiringIt = std::find_if(
        simulation.Entities().begin(), simulation.Entities().end(),
        [expiryCairnback](const Entity& entity) {
            return entity.temporaryMineralCover &&
                   entity.mineralCoverCreator == expiryCairnback;
        });
    REQUIRE(expiringIt != simulation.Entities().end());
    const EntityId expiring = expiringIt->id;
    const Tick expiryTick = expiringIt->mineralCoverUntilTick;
    REQUIRE(simulation.TerrainAt(9, 14) == Terrain::Blocked);
    simulation.Step(expiryTick - simulation.CurrentTick());
    REQUIRE(simulation.FindEntity(expiring) == nullptr);
    REQUIRE(simulation.TerrainAt(9, 14) == Terrain::Open);

    const ReplayRecord replay = simulation.ExportReplay();
    std::optional<Simulation> replayed =
        Simulation::ReplayToEnd(replay, &error);
    REQUIRE(replayed.has_value());
    REQUIRE(replayed->StateChecksum() == simulation.StateChecksum());

    Simulation ai({20, 20, 20, 0x434f5645524149ULL});
    REQUIRE(ai.AddPlayer(0, Faction::KharuunAssemblies, {0, 100}));
    REQUIRE(ai.AddPlayer(1, Faction::MeridianCompact, {0, 0}));
    const EntityId aiCairnback = ai.SpawnEntity(
        0, Faction::KharuunAssemblies, EntityType::HeavyUnit,
        Vec2::FromTiles(6, 6));
    const EntityId visibleEnemy = ai.SpawnEntity(
        1, Faction::MeridianCompact, EntityType::Soldier,
        Vec2::FromTiles(10, 6));
    REQUIRE(aiCairnback != 0 && visibleEnemy != 0);
    const std::optional<PlayerView> aiView = ai.CreatePlayerView(0);
    REQUIRE(aiView.has_value());
    const std::vector<Command> aiCommands =
        Simulation::GenerateAiCommands(*aiView, AiPersonality::Adaptive);
    REQUIRE(std::any_of(
        aiCommands.begin(), aiCommands.end(),
        [aiCairnback](const Command& command) {
            return command.type == CommandType::RaiseMineralCover &&
                   command.actor == aiCairnback &&
                   command.position == Vec2::FromTiles(7, 6);
        }));
}

void TestVibrationDetectionAndAnonymousSignatures() {
    SimulationConfig config{64, 64, 20, 0x56494252415445ULL};
    config.rules.vibrationDetection.signatureLingerTicks = 4;
    Simulation simulation(config);
    REQUIRE(simulation.AddPlayer(
        0, Faction::MeridianCompact, {0, 0}));
    REQUIRE(simulation.AddPlayer(
        1, Faction::KharuunAssemblies, {0, 0}));
    const EntityId listeningSpine = simulation.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::UtilityStructure,
        Vec2::FromTiles(20, 20));
    const EntityId resonant = simulation.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::ScoutUnit,
        Vec2::FromTiles(20, 22));
    const EntityId movingLancer = simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Soldier,
        Vec2::FromTiles(38, 20));
    REQUIRE(listeningSpine != 0 && resonant != 0 && movingLancer != 0);
    simulation.CaptureReplayBaseline();
    Command move = MakeCommand(
        0, 0, 1, CommandType::Move, movingLancer);
    move.position = Vec2::FromTiles(37, 20);
    REQUIRE(simulation.QueueCommand(move));
    simulation.Step();

    const std::optional<PlayerView> detected =
        simulation.CreatePlayerView(1);
    REQUIRE(detected.has_value());
    REQUIRE(std::none_of(
        detected->Entities().begin(), detected->Entities().end(),
        [movingLancer](const Entity& entity) {
            return entity.id == movingLancer;
        }));
    REQUIRE(detected->VibrationSignatures().size() == 1);
    REQUIRE(detected->VibrationSignatures()[0].approximatePosition ==
            Vec2::FromTiles(37, 21));
    REQUIRE(simulation.CreatePlayerView(0)->VibrationSignatures().empty());

    const std::vector<Command> commands =
        Simulation::GenerateAiCommands(*detected, AiPersonality::Adaptive);
    REQUIRE(std::any_of(
        commands.begin(), commands.end(),
        [resonant](const Command& command) {
            return command.actor == resonant &&
                   command.type == CommandType::AttackMove &&
                   command.position == Vec2::FromTiles(37, 21);
        }));

    std::string error;
    const std::optional<Simulation> restored =
        Simulation::LoadSnapshot(simulation.SaveSnapshot(), &error);
    REQUIRE(restored.has_value());
    REQUIRE(restored->Config().rules.vibrationDetection ==
            simulation.Config().rules.vibrationDetection);
    REQUIRE(restored->FindEntity(movingLancer)->vibrationSignatureUntilTick ==
            simulation.FindEntity(movingLancer)->vibrationSignatureUntilTick);
    REQUIRE(restored->CreatePlayerView(1)->VibrationSignatures() ==
            detected->VibrationSignatures());
    const ReplayRecord replay = simulation.ExportReplay();
    const std::optional<Simulation> replayed =
        Simulation::ReplayToEnd(replay, &error);
    REQUIRE(replayed.has_value());
    REQUIRE(replayed->StateChecksum() == simulation.StateChecksum());

    Command stop = MakeCommand(
        simulation.CurrentTick(), 0, 2, CommandType::Stop, movingLancer);
    REQUIRE(simulation.QueueCommand(stop));
    simulation.Step(3);
    REQUIRE(simulation.CurrentTick() == 4);
    REQUIRE(simulation.CreatePlayerView(1)->VibrationSignatures().empty());

    Simulation visible(config);
    REQUIRE(visible.AddPlayer(0, Faction::MeridianCompact, {0, 0}));
    REQUIRE(visible.AddPlayer(1, Faction::KharuunAssemblies, {0, 0}));
    const EntityId visibleResonant = visible.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::ScoutUnit,
        Vec2::FromTiles(20, 20));
    const EntityId visibleLancer = visible.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Soldier,
        Vec2::FromTiles(23, 20));
    REQUIRE(visibleResonant != 0 && visibleLancer != 0);
    Command visibleMove = MakeCommand(
        0, 0, 1, CommandType::Move, visibleLancer);
    visibleMove.position = Vec2::FromTiles(24, 20);
    REQUIRE(visible.QueueCommand(visibleMove));
    visible.Step();
    const std::optional<PlayerView> visibleView = visible.CreatePlayerView(1);
    REQUIRE(visibleView.has_value());
    REQUIRE(std::any_of(
        visibleView->Entities().begin(), visibleView->Entities().end(),
        [visibleLancer](const Entity& entity) {
            return entity.id == visibleLancer;
        }));
    REQUIRE(visibleView->VibrationSignatures().empty());

    Simulation uncovered(config);
    REQUIRE(uncovered.AddPlayer(0, Faction::MeridianCompact, {0, 0}));
    REQUIRE(uncovered.AddPlayer(1, Faction::KharuunAssemblies, {0, 0}));
    const EntityId uncoveredLancer = uncovered.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Soldier,
        Vec2::FromTiles(38, 20));
    REQUIRE(uncoveredLancer != 0);
    Command uncoveredMove = MakeCommand(
        0, 0, 1, CommandType::Move, uncoveredLancer);
    uncoveredMove.position = Vec2::FromTiles(37, 20);
    REQUIRE(uncovered.QueueCommand(uncoveredMove));
    uncovered.Step();
    REQUIRE(uncovered.CreatePlayerView(1)->VibrationSignatures().empty());
}

void TestPoweredAegisNetworkAndCounterplay() {
    SimulationConfig config{64, 64, 20, 0x4145474953504f57ULL};
    config.rules.poweredAegis.connectionRadiusRaw = 8 * kFixedScale;
    auto& aegisRules =
        config.rules.archetypes[static_cast<std::size_t>(
            Faction::MeridianCompact)]
                               [static_cast<std::size_t>(
                                   EntityType::UtilityStructure)];
    aegisRules.attackRangeRaw = 9 * kFixedScale;
    aegisRules.attackDamage = 28;
    aegisRules.attackPeriodTicks = 1;
    auto& breakerRules =
        config.rules.archetypes[static_cast<std::size_t>(
            Faction::KharuunAssemblies)]
                               [static_cast<std::size_t>(
                                   EntityType::HeavyUnit)];
    breakerRules.attackRangeRaw = 5 * kFixedScale;
    breakerRules.attackDamage = 1000;
    breakerRules.attackPeriodTicks = 1;

    Simulation simulation(config);
    AddTwoPlayers(simulation, {0, 0}, {0, 0});
    const EntityId anchor = simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::CommandCore,
        Vec2::FromTiles(2, 2));
    const EntityId firstLink = simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Dropoff,
        Vec2::FromTiles(9, 2));
    const EntityId bridgeLink = simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Dropoff,
        Vec2::FromTiles(16, 2));
    const EntityId aegis = simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::UtilityStructure,
        Vec2::FromTiles(23, 2));
    const EntityId target = simulation.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::Soldier,
        Vec2::FromTiles(28, 2));
    const EntityId breaker = simulation.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::HeavyUnit,
        Vec2::FromTiles(16, 4));
    REQUIRE(anchor != 0 && firstLink != 0 && bridgeLink != 0 &&
            aegis != 0 && target != 0 && breaker != 0);
    REQUIRE(simulation.FindEntity(aegis)->aegisPowered);
    REQUIRE(simulation.FindEntity(aegis)->attackRangeRaw == 9 * kFixedScale);
    REQUIRE(simulation.FindEntity(aegis)->attackDamage == 28);

    const std::optional<PlayerView> opponentView =
        simulation.CreatePlayerView(1);
    REQUIRE(opponentView.has_value());
    const auto observedAegis = std::find_if(
        opponentView->Entities().begin(), opponentView->Entities().end(),
        [aegis](const Entity& entity) { return entity.id == aegis; });
    REQUIRE(observedAegis != opponentView->Entities().end());
    REQUIRE(observedAegis->aegisPowered);
    REQUIRE(observedAegis->attackDamage == 0);
    REQUIRE(observedAegis->attackRangeRaw == 0);

    std::string error;
    const std::vector<std::uint8_t> poweredSnapshot =
        simulation.SaveSnapshot();
    const std::optional<Simulation> restored =
        Simulation::LoadSnapshot(poweredSnapshot, &error);
    REQUIRE(restored.has_value());
    REQUIRE(restored->Config().rules.poweredAegis ==
            simulation.Config().rules.poweredAegis);
    REQUIRE(restored->FindEntity(aegis)->aegisPowered);

    std::vector<std::uint8_t> forgedPower = poweredSnapshot;
    constexpr std::size_t kSerializedEntitySize = 202;
    constexpr std::size_t kAegisEntityIndex = 3;
    const std::size_t aegisPowerOffset =
        SnapshotFirstEntityOffset(64U * 64U) +
        kAegisEntityIndex * kSerializedEntitySize +
        (kSerializedEntitySize - 1);
    REQUIRE(forgedPower[aegisPowerOffset] == 1);
    forgedPower[aegisPowerOffset] = 0;
    ResignSnapshot(forgedPower);
    REQUIRE(!Simulation::LoadSnapshot(forgedPower, &error).has_value());
    REQUIRE(error == "snapshot Aegis power state is invalid");

    simulation.CaptureReplayBaseline();
    const std::int32_t targetHealth = simulation.FindEntity(target)->hitPoints;
    Command sever = MakeCommand(
        0, 1, 1, CommandType::Attack, breaker);
    sever.target = bridgeLink;
    REQUIRE(simulation.QueueCommand(sever));
    simulation.Step();
    REQUIRE(simulation.FindEntity(bridgeLink) == nullptr);
    REQUIRE(!simulation.FindEntity(aegis)->aegisPowered);
    REQUIRE(simulation.FindEntity(target)->hitPoints == targetHealth - 28);
    const std::int32_t healthAfterPowerLoss =
        simulation.FindEntity(target)->hitPoints;
    simulation.Step(3);
    REQUIRE(simulation.FindEntity(target)->hitPoints == healthAfterPowerLoss);

    const ReplayRecord replay = simulation.ExportReplay();
    const std::optional<Simulation> replayed =
        Simulation::ReplayToEnd(replay, &error);
    REQUIRE(replayed.has_value());
    REQUIRE(replayed->StateChecksum() == simulation.StateChecksum());

    Simulation isolated(config);
    REQUIRE(isolated.AddPlayer(
        0, Faction::MeridianCompact, {0, 0}));
    const EntityId isolatedAegis = isolated.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::UtilityStructure,
        Vec2::FromTiles(20, 20));
    REQUIRE(isolatedAegis != 0);
    REQUIRE(!isolated.FindEntity(isolatedAegis)->aegisPowered);
}

void TestFactionResearchProgressionAndPersistence() {
    Simulation simulation({32, 32, 20, 91});
    AddTwoPlayers(simulation, {1000, 500}, {1000, 500});
    const EntityId meridianFoundry = simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Barracks,
        Vec2::FromTiles(5, 5));
    const EntityId kharuunBasin = simulation.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::Barracks,
        Vec2::FromTiles(24, 24));
    const EntityId lancer = simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Soldier,
        Vec2::FromTiles(6, 5));
    const EntityId riftstalker = simulation.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::Soldier,
        Vec2::FromTiles(23, 24));
    REQUIRE(meridianFoundry != 0 && kharuunBasin != 0 &&
            lancer != 0 && riftstalker != 0);
    REQUIRE(simulation.ValidateResearch(
                0, meridianFoundry,
                ResearchType::MeridianHorizonLattice) ==
            ResearchResult::PrerequisiteMissing);
    REQUIRE(simulation.ValidateResearch(
                0, meridianFoundry,
                ResearchType::KharuunEchoCartography) ==
            ResearchResult::WrongFaction);

    simulation.CaptureReplayBaseline();
    Command targeting = MakeCommand(
        0, 0, 1, CommandType::Research, meridianFoundry);
    targeting.researchType = ResearchType::MeridianPrismaticTargeting;
    REQUIRE(simulation.QueueCommand(targeting));
    simulation.Step();
    const PlayerState* meridian = simulation.FindPlayer(0);
    REQUIRE(meridian != nullptr);
    REQUIRE(meridian->activeResearch ==
            ResearchType::MeridianPrismaticTargeting);
    REQUIRE(meridian->researchProgress == 1);
    REQUIRE(meridian->researchRequired == 180);
    REQUIRE(meridian->resources.material == 880);
    REQUIRE(meridian->resources.dawnshards == 460);
    REQUIRE(simulation.ValidateProduction(
                0, meridianFoundry, EntityType::Soldier) ==
            ProductionResult::ProducerBusy);
    const std::optional<PlayerView> researchingView =
        simulation.CreatePlayerView(0);
    REQUIRE(researchingView.has_value());
    REQUIRE(researchingView->Player().activeResearch ==
            ResearchType::MeridianPrismaticTargeting);

    std::string error;
    const std::vector<std::uint8_t> activeSnapshot = simulation.SaveSnapshot();
    const std::optional<Simulation> activeLoaded =
        Simulation::LoadSnapshot(activeSnapshot, &error);
    REQUIRE(activeLoaded.has_value());
    REQUIRE(activeLoaded->StateChecksum() == simulation.StateChecksum());

    simulation.Step(179);
    meridian = simulation.FindPlayer(0);
    REQUIRE(meridian->activeResearch == ResearchType::None);
    REQUIRE(meridian->HasCompletedResearch(
        ResearchType::MeridianPrismaticTargeting));
    REQUIRE(simulation.FindEntity(lancer)->attackDamage == 20);
    const EntityId upgradedLancer = simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Soldier,
        Vec2::FromTiles(7, 5));
    REQUIRE(simulation.FindEntity(upgradedLancer)->attackDamage == 20);
    REQUIRE(simulation.ValidateResearch(
                0, meridianFoundry,
                ResearchType::MeridianPrismaticTargeting) ==
            ResearchResult::AlreadyCompleted);
    simulation.CaptureReplayBaseline();

    Command lattice = MakeCommand(
        simulation.CurrentTick(), 0, 2, CommandType::Research,
        meridianFoundry);
    lattice.researchType = ResearchType::MeridianHorizonLattice;
    REQUIRE(simulation.QueueCommand(lattice));
    simulation.Step(220);
    REQUIRE(simulation.FindPlayer(0)->HasCompletedResearch(
        ResearchType::MeridianHorizonLattice));
    REQUIRE(simulation.FindEntity(lancer)->visionTiles == 7);

    Command cartography = MakeCommand(
        simulation.CurrentTick(), 1, 1, CommandType::Research, kharuunBasin);
    cartography.researchType = ResearchType::KharuunEchoCartography;
    REQUIRE(simulation.QueueCommand(cartography));
    simulation.Step(180);
    REQUIRE(simulation.FindPlayer(1)->HasCompletedResearch(
        ResearchType::KharuunEchoCartography));
    REQUIRE(simulation.FindEntity(riftstalker)->visionTiles == 8);

    Command edge = MakeCommand(
        simulation.CurrentTick(), 1, 2, CommandType::Research, kharuunBasin);
    edge.researchType = ResearchType::KharuunAncestralEdge;
    REQUIRE(simulation.QueueCommand(edge));
    simulation.Step(220);
    REQUIRE(simulation.FindPlayer(1)->HasCompletedResearch(
        ResearchType::KharuunAncestralEdge));
    REQUIRE(simulation.FindEntity(riftstalker)->attackDamage == 28);

    Command striker = MakeCommand(
        simulation.CurrentTick(), 1, 3, CommandType::AdaptWarform,
        riftstalker);
    striker.target = kharuunBasin;
    striker.warformAdaptation = WarformAdaptation::Striker;
    REQUIRE(simulation.QueueCommand(striker));
    simulation.Step(80);
    REQUIRE(simulation.FindEntity(riftstalker)->warformAdaptation ==
            WarformAdaptation::Striker);
    REQUIRE(simulation.FindEntity(riftstalker)->attackDamage == 35);

    const ReplayRecord replay = simulation.ExportReplay();
    const std::optional<Simulation> replayed =
        Simulation::ReplayToEnd(replay, &error);
    REQUIRE(replayed.has_value());
    REQUIRE(replayed->StateChecksum() == simulation.StateChecksum());

    Simulation cancelled({20, 20, 20, 93});
    AddTwoPlayers(cancelled, {1000, 500}, {0, 0});
    const EntityId cancelledFoundry = cancelled.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Barracks,
        Vec2::FromTiles(6, 6));
    REQUIRE(cancelledFoundry != 0);
    cancelled.CaptureReplayBaseline();
    Command cancellableResearch = MakeCommand(
        0, 0, 1, CommandType::Research, cancelledFoundry);
    cancellableResearch.researchType =
        ResearchType::MeridianPrismaticTargeting;
    REQUIRE(cancelled.QueueCommand(cancellableResearch));
    cancelled.Step();
    const PlayerState* cancelledPlayer = cancelled.FindPlayer(0);
    REQUIRE(cancelledPlayer != nullptr);
    REQUIRE(cancelledPlayer->activeResearch ==
            ResearchType::MeridianPrismaticTargeting);
    const ResourcePool committedResearchResources =
        cancelledPlayer->resources;
    Command cancelResearch = MakeCommand(
        cancelled.CurrentTick(), 0, 2, CommandType::Stop,
        cancelledFoundry);
    REQUIRE(cancelled.QueueCommand(cancelResearch));
    cancelled.Step();
    cancelledPlayer = cancelled.FindPlayer(0);
    REQUIRE(cancelledPlayer->activeResearch == ResearchType::None);
    REQUIRE(cancelledPlayer->researchProducer == 0);
    REQUIRE(cancelledPlayer->researchProgress == 0);
    REQUIRE(cancelledPlayer->researchRequired == 0);
    REQUIRE(cancelledPlayer->lastInterruptedResearch ==
            ResearchType::MeridianPrismaticTargeting);
    REQUIRE(cancelledPlayer->resources == committedResearchResources);
    REQUIRE(!cancelledPlayer->HasCompletedResearch(
        ResearchType::MeridianPrismaticTargeting));
    const std::optional<Simulation> replayedCancellation =
        Simulation::ReplayToEnd(cancelled.ExportReplay(), &error);
    REQUIRE(replayedCancellation.has_value());
    REQUIRE(replayedCancellation->StateChecksum() ==
            cancelled.StateChecksum());

    SimulationConfig interruptedConfig{20, 20, 20, 92};
    interruptedConfig.rules
        .archetypes[static_cast<std::size_t>(Faction::MeridianCompact)]
                   [static_cast<std::size_t>(EntityType::Soldier)]
        .attackDamage = 2000;
    Simulation interrupted(interruptedConfig);
    AddTwoPlayers(interrupted, {0, 0}, {1000, 500});
    const EntityId interruptedBasin = interrupted.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::Barracks,
        Vec2::FromTiles(10, 10));
    const EntityId researchBreaker = interrupted.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Soldier,
        Vec2::FromTiles(11, 10));
    REQUIRE(interruptedBasin != 0 && researchBreaker != 0);
    Command interruptedResearch = MakeCommand(
        0, 1, 1, CommandType::Research, interruptedBasin);
    interruptedResearch.researchType = ResearchType::KharuunEchoCartography;
    Command destroyResearchSite = MakeCommand(
        0, 0, 1, CommandType::Attack, researchBreaker);
    destroyResearchSite.target = interruptedBasin;
    REQUIRE(interrupted.QueueCommand(interruptedResearch));
    REQUIRE(interrupted.QueueCommand(destroyResearchSite));
    interrupted.Step();
    const PlayerState* interruptedPlayer = interrupted.FindPlayer(1);
    REQUIRE(interrupted.FindEntity(interruptedBasin) == nullptr);
    REQUIRE(interruptedPlayer != nullptr);
    REQUIRE(interruptedPlayer->activeResearch == ResearchType::None);
    REQUIRE(interruptedPlayer->researchProducer == 0);
    REQUIRE(interruptedPlayer->researchProgress == 0);
    REQUIRE(interruptedPlayer->researchRequired == 0);
    REQUIRE(interruptedPlayer->lastInterruptedResearch ==
            ResearchType::KharuunEchoCartography);
    REQUIRE(interruptedPlayer->resources.material == 900);
    REQUIRE(interruptedPlayer->resources.dawnshards == 455);
    REQUIRE(!interruptedPlayer->HasCompletedResearch(
        ResearchType::KharuunEchoCartography));
    const std::optional<PlayerView> interruptedView =
        interrupted.CreatePlayerView(1);
    REQUIRE(interruptedView.has_value());
    REQUIRE(interruptedView->Player().lastInterruptedResearch ==
            ResearchType::KharuunEchoCartography);
    const std::vector<std::uint8_t> interruptedSnapshot =
        interrupted.SaveSnapshot();
    const std::optional<Simulation> interruptedLoaded =
        Simulation::LoadSnapshot(interruptedSnapshot, &error);
    REQUIRE(interruptedLoaded.has_value());
    REQUIRE(interruptedLoaded->StateChecksum() ==
            interrupted.StateChecksum());

    const EntityId replacementBasin = interrupted.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::Barracks,
        Vec2::FromTiles(14, 14));
    REQUIRE(replacementBasin != 0);
    Command replacementResearch = MakeCommand(
        interrupted.CurrentTick(), 1, 2, CommandType::Research,
        replacementBasin);
    replacementResearch.researchType = ResearchType::KharuunEchoCartography;
    REQUIRE(interrupted.QueueCommand(replacementResearch));
    interrupted.Step();
    REQUIRE(interrupted.FindPlayer(1)->lastInterruptedResearch ==
            ResearchType::None);
}

void TestNetworkProtocolAdmissionAndHardening() {
    using namespace echoes::sim::net;

    CompatibilityManifest authority{};
    authority.simulationRulesVersion = DefaultSimulationRules().version;
    authority.serializationFeatureFlags = 0x0102030405060708ULL;
    for (std::size_t index = 0; index < kDigestBytes; ++index) {
        authority.buildIdSha256[index] = static_cast<std::uint8_t>(index);
        authority.rulesPackSha256[index] =
            static_cast<std::uint8_t>(index + 32);
        authority.mapPackSha256[index] =
            static_cast<std::uint8_t>(index + 64);
        authority.matchSettingsSha256[index] =
            static_cast<std::uint8_t>(index + 96);
    }

    const std::vector<std::uint8_t> hello =
        EncodeCompatibilityHello(authority);
    REQUIRE(hello.size() < kMaximumPacketBytes);
    REQUIRE(hello == EncodeCompatibilityHello(authority));
    CompatibilityManifest decodedManifest{};
    REQUIRE(DecodeCompatibilityHello(hello, decodedManifest) ==
            DecodeStatus::Ok);
    REQUIRE(decodedManifest == authority);
    REQUIRE(CheckCompatibility(authority, decodedManifest) ==
            CompatibilityStatus::Accepted);
    REQUIRE(StableId(CompatibilityStatus::Accepted) == "NET_COMPATIBLE");

    CompatibilityManifest mismatch = authority;
    mismatch.protocolVersion += 1;
    REQUIRE(CheckCompatibility(authority, mismatch) ==
            CompatibilityStatus::ProtocolMismatch);
    mismatch = authority;
    mismatch.snapshotVersion += 1;
    REQUIRE(CheckCompatibility(authority, mismatch) ==
            CompatibilityStatus::SnapshotSchemaMismatch);
    mismatch = authority;
    mismatch.simulationRulesVersion += 1;
    REQUIRE(CheckCompatibility(authority, mismatch) ==
            CompatibilityStatus::SimulationRulesMismatch);
    mismatch = authority;
    mismatch.playerViewSchemaVersion += 1;
    REQUIRE(CheckCompatibility(authority, mismatch) ==
            CompatibilityStatus::PlayerViewSchemaMismatch);
    mismatch = authority;
    mismatch.buildIdSha256[0] ^= 1;
    REQUIRE(CheckCompatibility(authority, mismatch) ==
            CompatibilityStatus::BuildMismatch);
    mismatch = authority;
    mismatch.rulesPackSha256[0] ^= 1;
    REQUIRE(CheckCompatibility(authority, mismatch) ==
            CompatibilityStatus::RulesPackMismatch);
    mismatch = authority;
    mismatch.mapPackSha256[0] ^= 1;
    REQUIRE(CheckCompatibility(authority, mismatch) ==
            CompatibilityStatus::MapPackMismatch);
    mismatch = authority;
    mismatch.serializationFeatureFlags ^= 1;
    REQUIRE(CheckCompatibility(authority, mismatch) ==
            CompatibilityStatus::SerializationFeaturesMismatch);
    mismatch = authority;
    mismatch.matchSettingsSha256[0] ^= 1;
    REQUIRE(CheckCompatibility(authority, mismatch) ==
            CompatibilityStatus::MatchSettingsMismatch);

    std::vector<std::uint8_t> malformed = hello;
    malformed.pop_back();
    REQUIRE(DecodeCompatibilityHello(malformed, decodedManifest) ==
            DecodeStatus::LengthMismatch);
    malformed = hello;
    malformed.push_back(0);
    REQUIRE(DecodeCompatibilityHello(malformed, decodedManifest) ==
            DecodeStatus::LengthMismatch);
    malformed = hello;
    malformed[12] ^= 1;
    REQUIRE(DecodeCompatibilityHello(malformed, decodedManifest) ==
            DecodeStatus::IntegrityMismatch);
    malformed = hello;
    malformed[0] ^= 1;
    ResignNetworkPacket(malformed);
    REQUIRE(DecodeCompatibilityHello(malformed, decodedManifest) ==
            DecodeStatus::BadMagic);
    malformed = hello;
    malformed[4] += 1;
    ResignNetworkPacket(malformed);
    REQUIRE(DecodeCompatibilityHello(malformed, decodedManifest) ==
            DecodeStatus::UnsupportedEnvelopeVersion);
    malformed = hello;
    malformed[6] = static_cast<std::uint8_t>(PacketKind::CommandRequest);
    ResignNetworkPacket(malformed);
    REQUIRE(DecodeCompatibilityHello(malformed, decodedManifest) ==
            DecodeStatus::WrongPacketKind);
    malformed = hello;
    malformed[7] = 1;
    ResignNetworkPacket(malformed);
    REQUIRE(DecodeCompatibilityHello(malformed, decodedManifest) ==
            DecodeStatus::ReservedFieldNonzero);
    malformed.assign(kMaximumPacketBytes + 1, 0);
    REQUIRE(DecodeCompatibilityHello(malformed, decodedManifest) ==
            DecodeStatus::PacketTooLarge);

    CommandRequest request{};
    request.sequence = 7;
    request.executeTick = 12;
    request.type = CommandType::Build;
    request.actor = 44;
    request.target = 55;
    request.position = Vec2::FromRaw(-123456, 987654);
    request.buildType = EntityType::UtilityStructure;
    request.wellChoice = FutureWellChoice::Preserve;
    request.warformAdaptation = WarformAdaptation::Carapace;
    request.researchType = ResearchType::MeridianHorizonLattice;
    const std::vector<std::uint8_t> commandBytes =
        EncodeCommandRequest(request);
    REQUIRE(commandBytes.size() < kMaximumPacketBytes);
    REQUIRE(commandBytes == EncodeCommandRequest(request));
    CommandRequest decodedRequest{};
    REQUIRE(DecodeCommandRequest(commandBytes, decodedRequest) ==
            DecodeStatus::Ok);
    REQUIRE(decodedRequest == request);

    malformed = commandBytes;
    malformed[24] = 0xff;
    ResignNetworkPacket(malformed);
    REQUIRE(DecodeCommandRequest(malformed, decodedRequest) ==
            DecodeStatus::InvalidEncoding);
    malformed = commandBytes;
    malformed[25] = 0;
    malformed[26] = 0;
    malformed[27] = 0;
    malformed[28] = 0;
    ResignNetworkPacket(malformed);
    REQUIRE(DecodeCommandRequest(malformed, decodedRequest) ==
            DecodeStatus::InvalidEncoding);
    malformed = commandBytes;
    malformed[malformed.size() - 1] ^= 1;
    REQUIRE(DecodeCommandRequest(malformed, decodedRequest) ==
            DecodeStatus::IntegrityMismatch);

    Simulation simulation({16, 16, 20, 31});
    AddTwoPlayers(simulation);
    const EntityId remoteWorker = simulation.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::Worker,
        Vec2::FromTiles(10, 10));
    REQUIRE(remoteWorker != 0);
    const EntityId localWorker = simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Worker,
        Vec2::FromTiles(2, 2));
    REQUIRE(localWorker != 0);

    CommandAdmissionContext context{};
    context.player = 1;
    context.minimumInputDelayTicks = 2;
    context.maximumLeadTicks = 8;
    CommandRequest remoteMove{};
    remoteMove.sequence = 1;
    remoteMove.executeTick = 2;
    remoteMove.type = CommandType::Move;
    remoteMove.actor = remoteWorker;
    remoteMove.position = Vec2::FromTiles(11, 10);
    std::string rejection;
    REQUIRE(AdmitCommandRequest(remoteMove, context, simulation, &rejection) ==
            CommandAdmissionStatus::Accepted);
    REQUIRE(rejection.empty());
    REQUIRE(context.hasAcceptedSequence);
    REQUIRE(context.lastAcceptedSequence == 1);
    REQUIRE(simulation.CommandLog().back().player == 1);
    REQUIRE(simulation.CommandLog().back().sequence == 1);
    REQUIRE(StableId(CommandAdmissionStatus::Accepted) ==
            "NET_CMD_ACCEPTED");

    REQUIRE(AdmitCommandRequest(remoteMove, context, simulation, &rejection) ==
            CommandAdmissionStatus::SequenceUnexpected);
    CommandRequest tooEarly = remoteMove;
    tooEarly.sequence = 2;
    tooEarly.executeTick = 1;
    REQUIRE(AdmitCommandRequest(tooEarly, context, simulation, &rejection) ==
            CommandAdmissionStatus::TickTooEarly);
    CommandRequest tooLate = tooEarly;
    tooLate.executeTick = 9;
    REQUIRE(AdmitCommandRequest(tooLate, context, simulation, &rejection) ==
            CommandAdmissionStatus::TickTooLate);

    CommandAdmissionContext invalidSeat = context;
    invalidSeat.player = 3;
    REQUIRE(AdmitCommandRequest(tooEarly, invalidSeat, simulation, &rejection) ==
            CommandAdmissionStatus::InvalidSeat);
    CommandRequest foreignActor = remoteMove;
    foreignActor.sequence = 2;
    foreignActor.executeTick = 3;
    foreignActor.actor = localWorker;
    REQUIRE(AdmitCommandRequest(foreignActor, context, simulation,
                                &rejection) ==
            CommandAdmissionStatus::ActorNotOwned);
    REQUIRE(context.lastAcceptedSequence == 1);
    CommandAdmissionContext invalidRange = context;
    invalidRange.minimumInputDelayTicks = 9;
    invalidRange.maximumLeadTicks = 8;
    REQUIRE(AdmitCommandRequest(tooEarly, invalidRange, simulation,
                                &rejection) ==
            CommandAdmissionStatus::TickRangeInvalid);
    simulation.Step();
    invalidRange.minimumInputDelayTicks = 0;
    invalidRange.maximumLeadTicks = std::numeric_limits<Tick>::max();
    REQUIRE(AdmitCommandRequest(tooEarly, invalidRange, simulation,
                                &rejection) ==
            CommandAdmissionStatus::TickRangeInvalid);

    CommandRequest simulationRejected = remoteMove;
    simulationRejected.sequence = 2;
    simulationRejected.executeTick = 3;
    simulationRejected.type = static_cast<CommandType>(0xff);
    const std::size_t logSizeBeforeRejection = simulation.CommandLog().size();
    REQUIRE(AdmitCommandRequest(simulationRejected, context, simulation,
                                &rejection) ==
            CommandAdmissionStatus::CommandRejected);
    REQUIRE(!rejection.empty());
    REQUIRE(context.lastAcceptedSequence == 1);
    REQUIRE(simulation.CommandLog().size() == logSizeBeforeRejection);

    simulation.Step(20);
    REQUIRE(simulation.FindEntity(remoteWorker)->position ==
            Vec2::FromTiles(11, 10));
}

}  // namespace

int main() {
    const std::vector<std::pair<std::string, std::function<void()>>> tests{
        {"fixed tick movement", TestFixedTickMovement},
        {"canonical ordering and determinism", TestCanonicalCommandOrderingAndDeterminism},
        {"gather deliver build and placement", TestGatherDeliverBuildAndPlacement},
        {"combat", TestCombatResolvesDeterministically},
        {"attack-move acquisition resume and stop",
         TestAttackMoveAcquiresResumesAndStops},
        {"hold position defends without chasing",
         TestHoldPositionDefendsWithoutChasing},
        {"guard defends and follows owned target",
         TestGuardDefendsAndFollowsOwnedTarget},
        {"patrol reverses persists and bounds engagements",
         TestPatrolReversesPersistsAndBoundsEngagements},
        {"deterministic obstacle pathing", TestDeterministicObstaclePathing},
        {"production population and victory", TestProductionPopulationAndVictory},
        {"fog and non-cheating AI", TestFogAndNonCheatingAi},
        {"four-player visibility snapshot and outcome",
         TestFourPlayerVisibilitySnapshotAndOutcome},
        {"Future Well choices", TestFutureWellChoices},
        {"snapshot and replay", TestSnapshotAndReplay},
        {"numeric and public input hardening", TestNumericAndPublicInputHardening},
        {"sequence and build hardening", TestSequenceAndBuildHardening},
        {"snapshot adversarial bounds and id exhaustion",
         TestSnapshotAdversarialBoundsAndIdExhaustion},
        {"authored rules drive simulation and persist",
         TestAuthoredRulesDriveSimulationAndPersist},
        {"complete roster entity types and production",
         TestCompleteRosterEntityTypesAndProduction},
        {"Bulwark directional cover deployment",
         TestBulwarkDirectionalCoverDeployment},
        {"Relay supply extension lifecycle",
         TestRelaySupplyExtensionLifecycle},
        {"Waystone migration and rooting",
         TestWaystoneMigrationAndRooting},
        {"Warform adaptation and molt counterplay",
         TestWarformAdaptationAndMoltCounterplay},
        {"Cairnback temporary mineral cover",
         TestCairnbackTemporaryMineralCover},
        {"vibration detection and anonymous signatures",
         TestVibrationDetectionAndAnonymousSignatures},
        {"powered Aegis network and counterplay",
         TestPoweredAegisNetworkAndCounterplay},
        {"faction research progression and persistence",
         TestFactionResearchProgressionAndPersistence},
        {"network protocol admission and hardening",
         TestNetworkProtocolAdmissionAndHardening},
    };

    std::size_t passed = 0;
    for (const auto& [name, test] : tests) {
        try {
            test();
            ++passed;
            std::cout << "[PASS] " << name << '\n';
        } catch (const std::exception& failure) {
            std::cerr << "[FAIL] " << name << ": " << failure.what() << '\n';
            return 1;
        }
    }
    std::cout << passed << "/" << tests.size()
              << " native simulation tests passed\n";
    return 0;
}
