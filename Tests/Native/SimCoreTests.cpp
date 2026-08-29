#include "EchoesSimCore/Simulation.h"

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

std::size_t SnapshotEntityCountOffset(std::size_t mapTileCount) {
    // Snapshot v12 header/rules/player/sequence fields plus terrain and four fog grids.
    return 1520 + 5 * mapTileCount;
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
    WriteU64(excessiveTick, 1400, std::numeric_limits<std::uint64_t>::max());
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
    WriteU32(exhaustedIds, 1408, std::numeric_limits<std::uint32_t>::max());
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
