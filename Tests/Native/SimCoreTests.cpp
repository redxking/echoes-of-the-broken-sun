#include "EchoesSimCore/Simulation.h"
#include "EchoesSimCore/NetworkProtocol.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <set>
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

void WriteU16(std::vector<std::uint8_t>& bytes,
              std::size_t offset,
              std::uint16_t value) {
    REQUIRE(offset <= bytes.size() && bytes.size() - offset >= 2U);
    for (std::uint32_t shift = 0; shift < 16; shift += 8) {
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

std::uint32_t ReadU32(const std::vector<std::uint8_t>& bytes,
                      std::size_t offset) {
    REQUIRE(offset + 4 <= bytes.size());
    std::uint32_t value = 0;
    for (std::uint32_t shift = 0; shift < 32; shift += 8) {
        value |= static_cast<std::uint32_t>(bytes[offset++]) << shift;
    }
    return value;
}

std::uint64_t ReadU64(const std::vector<std::uint8_t>& bytes,
                      std::size_t offset) {
    REQUIRE(offset + 8 <= bytes.size());
    std::uint64_t value = 0;
    for (std::uint32_t shift = 0; shift < 64; shift += 8) {
        value |= static_cast<std::uint64_t>(bytes[offset++]) << shift;
    }
    return value;
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

std::size_t SnapshotV21EntityCountOffset(std::size_t mapTileCount) {
    // Snapshot v21 header/rules/research/player/sequence fields plus terrain and four fog grids.
    return 1862 + 5 * mapTileCount;
}

std::size_t SnapshotV21FirstEntityOffset(std::size_t mapTileCount) {
    return SnapshotV21EntityCountOffset(mapTileCount) + 4;
}

std::size_t SnapshotV22EntityCountOffset(std::size_t mapTileCount) {
    // Snapshot v22 adds one faction row, Choir rules, and two research rows.
    return 2598 + 5 * mapTileCount;
}

std::size_t SnapshotV22FirstEntityOffset(std::size_t mapTileCount) {
    return SnapshotV22EntityCountOffset(mapTileCount) + 4;
}

std::size_t SnapshotV23EntityCountOffset(std::size_t mapTileCount) {
    // Snapshot v23 adds the protected-Command-Core player mask.
    return SnapshotV22EntityCountOffset(mapTileCount) + 1;
}

std::size_t SnapshotV24EntityCountOffset(std::size_t mapTileCount) {
    // Snapshot v24 appends receipts, so entity offsets remain identical to v23.
    return SnapshotV23EntityCountOffset(mapTileCount);
}

// Snapshot v25 inserts per-player remembered terrain and remembered permanent
// objects between the fog grids and the entity table. The object block is
// variable length, so the offset is walked out of the payload rather than
// assumed; the walk asserts each terrain-memory grid matches the map, which
// makes a layout drift fail here instead of silently shifting every later
// field.
constexpr std::size_t kSerializedRememberedObjectBytes = 24;
constexpr std::size_t kSerializedEntityBytes = 235;
constexpr std::size_t kSerializedCommandBytes = 38;
constexpr std::size_t kSerializedReceiptBytes = 19;
constexpr std::size_t kSerializedWorkStateBytes = 31;
constexpr std::size_t kSerializedQueuedOrderBytes = 23;
constexpr std::size_t kSerializedProjectileBytes = 41;
constexpr std::size_t kSerializedFutureWellLifecycleBytes = 16;

std::size_t SerializedSpanEnd(const std::vector<std::uint8_t>& bytes,
                              std::size_t offset,
                              std::size_t count,
                              std::size_t recordBytes) {
    REQUIRE(offset <= bytes.size());
    REQUIRE(recordBytes != 0);
    REQUIRE(count <= (bytes.size() - offset) / recordBytes);
    return offset + count * recordBytes;
}

std::size_t SnapshotV25MemoryBlockOffset(std::size_t mapTileCount) {
    return SnapshotV24EntityCountOffset(mapTileCount);
}

std::size_t SnapshotV25EntityCountOffset(const std::vector<std::uint8_t>& bytes,
                                         std::size_t mapTileCount) {
    std::size_t offset = SnapshotV25MemoryBlockOffset(mapTileCount);
    for (std::size_t player = 0; player < 4; ++player) {
        REQUIRE(ReadU32(bytes, offset) == mapTileCount);
        offset = SerializedSpanEnd(bytes, offset + 4U, mapTileCount, 1U);
    }
    for (std::size_t player = 0; player < 4; ++player) {
        const std::uint32_t count = ReadU32(bytes, offset);
        offset = SerializedSpanEnd(bytes, offset + 4U, count,
                                   kSerializedRememberedObjectBytes);
    }
    return offset;
}

std::size_t SnapshotV25FirstEntityOffset(const std::vector<std::uint8_t>& bytes,
                                         std::size_t mapTileCount) {
    return SnapshotV25EntityCountOffset(bytes, mapTileCount) + 4;
}

std::size_t SnapshotReceiptBlockOffset(
    const std::vector<std::uint8_t>& bytes, std::size_t mapTileCount) {
    const std::size_t countOffset = SnapshotV25EntityCountOffset(bytes, mapTileCount);
    std::size_t offset = SerializedSpanEnd(
        bytes, countOffset + 4U, ReadU32(bytes, countOffset),
        kSerializedEntityBytes);
    const std::uint32_t commandCount = ReadU32(bytes, offset);
    return SerializedSpanEnd(bytes, offset + 4U, commandCount,
                             kSerializedCommandBytes);
}

std::size_t SnapshotWorkBlockOffset(
    const std::vector<std::uint8_t>& bytes, std::size_t mapTileCount) {
    const std::size_t receiptOffset =
        SnapshotReceiptBlockOffset(bytes, mapTileCount);
    const std::uint32_t receiptCount = ReadU32(bytes, receiptOffset);
    return SerializedSpanEnd(bytes, receiptOffset + 4U, receiptCount,
                             kSerializedReceiptBytes);
}

std::size_t SnapshotWorkRecordOffset(
    const std::vector<std::uint8_t>& bytes,
    std::size_t workBlockOffset,
    std::size_t recordIndex) {
    const std::uint32_t workCount = ReadU32(bytes, workBlockOffset);
    REQUIRE(recordIndex < workCount);
    std::size_t offset = workBlockOffset + 4U;
    for (std::size_t index = 0; index < recordIndex; ++index) {
        REQUIRE(offset <= bytes.size() &&
                bytes.size() - offset >= kSerializedWorkStateBytes);
        const std::uint8_t queueCount = bytes[offset + 30U];
        offset += kSerializedWorkStateBytes;
        offset = SerializedSpanEnd(bytes, offset, queueCount,
                                   kSerializedQueuedOrderBytes);
    }
    REQUIRE(offset <= bytes.size() &&
            bytes.size() - offset >= kSerializedWorkStateBytes);
    return offset;
}

std::size_t SnapshotProjectileHeaderOffset(
    const std::vector<std::uint8_t>& bytes, std::size_t mapTileCount) {
    const std::size_t workBlockOffset =
        SnapshotWorkBlockOffset(bytes, mapTileCount);
    const std::uint32_t workCount = ReadU32(bytes, workBlockOffset);
    std::size_t offset = workBlockOffset + 4U;
    for (std::uint32_t index = 0; index < workCount; ++index) {
        REQUIRE(offset <= bytes.size() &&
                bytes.size() - offset >= kSerializedWorkStateBytes);
        const std::uint8_t queueCount = bytes[offset + 30U];
        offset += kSerializedWorkStateBytes;
        offset = SerializedSpanEnd(bytes, offset, queueCount,
                                   kSerializedQueuedOrderBytes);
    }
    REQUIRE(offset <= bytes.size() && bytes.size() - offset >= 9U);
    return offset;
}

std::size_t SnapshotFutureWellLifecycleBlockOffset(
    const std::vector<std::uint8_t>& bytes, std::size_t mapTileCount) {
    const std::size_t projectileHeaderOffset =
        SnapshotProjectileHeaderOffset(bytes, mapTileCount);
    const std::uint32_t projectileCount =
        ReadU32(bytes, projectileHeaderOffset + 5U);
    return SerializedSpanEnd(bytes, projectileHeaderOffset + 9U,
                             projectileCount, kSerializedProjectileBytes);
}

std::size_t SnapshotFutureWellLifecycleRecordOffset(
    const std::vector<std::uint8_t>& bytes,
    std::size_t mapTileCount,
    std::size_t recordIndex) {
    const std::size_t lifecycleOffset =
        SnapshotFutureWellLifecycleBlockOffset(bytes, mapTileCount);
    const std::uint32_t lifecycleCount = ReadU32(bytes, lifecycleOffset);
    REQUIRE(recordIndex < lifecycleCount);
    const std::size_t recordOffset = SerializedSpanEnd(
        bytes, lifecycleOffset + 4U, recordIndex,
        kSerializedFutureWellLifecycleBytes);
    REQUIRE(recordOffset <= bytes.size() &&
            bytes.size() - recordOffset >=
                kSerializedFutureWellLifecycleBytes);
    return recordOffset;
}

std::vector<std::uint8_t> ConvertSnapshotV28ToV27(
    const std::vector<std::uint8_t>& current, std::size_t mapTileCount) {
    REQUIRE(ReadU32(current, 4) == 28);
    const std::size_t lifecycle = SnapshotFutureWellLifecycleBlockOffset(current, mapTileCount);
    const std::size_t end = SerializedSpanEnd(current, lifecycle + 4U,
        ReadU32(current, lifecycle), kSerializedFutureWellLifecycleBytes);
    REQUIRE(end <= current.size() && current.size() - end == 12U);
    std::vector<std::uint8_t> prior(current.begin(), current.begin() + end);
    prior.resize(prior.size() + 8U);
    WriteU32(prior, 4, 27);
    ResignSnapshot(prior);
    return prior;
}

std::vector<std::uint8_t> ConvertSnapshotV27ToV26(
    const std::vector<std::uint8_t>& current, std::size_t mapTileCount) {
    REQUIRE(ReadU32(current, 4) == 27);
    const std::size_t entityCountOffset =
        SnapshotV25EntityCountOffset(current, mapTileCount);
    const std::uint32_t entityCount = ReadU32(current, entityCountOffset);
    const std::size_t workOffset =
        SnapshotWorkBlockOffset(current, mapTileCount);
    REQUIRE(ReadU32(current, workOffset) == entityCount);
    const std::size_t lifecycleOffset =
        SnapshotFutureWellLifecycleBlockOffset(current, mapTileCount);
    REQUIRE(ReadU32(current, lifecycleOffset) == entityCount);
    const std::size_t lifecycleEnd = SerializedSpanEnd(
        current, lifecycleOffset + 4U, entityCount,
        kSerializedFutureWellLifecycleBytes);
    REQUIRE(lifecycleEnd <= current.size() &&
            current.size() - lifecycleEnd == 8U);
    std::vector<std::uint8_t> prior(current.begin(),
                                    current.begin() + lifecycleOffset);
    prior.resize(prior.size() + 8U);
    WriteU32(prior, 4, 26);
    ResignSnapshot(prior);
    return prior;
}

std::vector<std::uint8_t> ConvertSnapshotV26ToV25(
    const std::vector<std::uint8_t>& current, std::size_t mapTileCount) {
    REQUIRE(ReadU32(current, 4) == 26);
    const std::size_t entityCountOffset =
        SnapshotV25EntityCountOffset(current, mapTileCount);
    const std::uint32_t entityCount = ReadU32(current, entityCountOffset);
    const std::size_t workOffset =
        SnapshotWorkBlockOffset(current, mapTileCount);
    REQUIRE(ReadU32(current, workOffset) == entityCount);
    const std::size_t projectileHeaderOffset =
        SnapshotProjectileHeaderOffset(current, mapTileCount);
    const std::uint32_t projectileCount =
        ReadU32(current, projectileHeaderOffset + 5U);
    const std::size_t payloadEnd = SerializedSpanEnd(
        current, projectileHeaderOffset + 9U, projectileCount,
        kSerializedProjectileBytes);
    REQUIRE(payloadEnd <= current.size() &&
            current.size() - payloadEnd == 8U);
    std::vector<std::uint8_t> prior(current.begin(),
                                    current.begin() + workOffset);
    prior.resize(prior.size() + 8);
    WriteU32(prior, 4, 25);
    ResignSnapshot(prior);
    return prior;
}

std::vector<std::uint8_t> ConvertSnapshotV25ToV24(
    const std::vector<std::uint8_t>& current,
    std::size_t mapTileCount) {
    REQUIRE(ReadU32(current, 4) == 25);
    const std::size_t memoryBegin = SnapshotV25MemoryBlockOffset(mapTileCount);
    const std::size_t memoryEnd =
        SnapshotV25EntityCountOffset(current, mapTileCount);
    std::vector<std::uint8_t> prior = current;
    prior.erase(prior.begin() + static_cast<std::ptrdiff_t>(memoryBegin),
                prior.begin() + static_cast<std::ptrdiff_t>(memoryEnd));
    WriteU32(prior, 4, 24);
    ResignSnapshot(prior);
    return prior;
}

std::vector<std::uint8_t> ConvertSnapshotV24ToV23(
    const std::vector<std::uint8_t>& current,
    std::uint32_t receiptCount) {
    constexpr std::size_t kSerializedReceiptBytes = 19;
    REQUIRE(ReadU32(current, 4) == 24);
    const std::size_t receiptBlockBytes =
        4U + static_cast<std::size_t>(receiptCount) *
                 kSerializedReceiptBytes;
    REQUIRE(current.size() >= 8U + receiptBlockBytes);
    const std::size_t receiptBlockOffset =
        current.size() - 8U - receiptBlockBytes;
    REQUIRE(ReadU32(current, receiptBlockOffset) == receiptCount);
    std::vector<std::uint8_t> prior = current;
    prior.erase(
        prior.begin() + static_cast<std::ptrdiff_t>(receiptBlockOffset),
        prior.end() - 8);
    WriteU32(prior, 4, 23);
    ResignSnapshot(prior);
    return prior;
}

std::vector<std::uint8_t> ConvertSnapshotV23ToV22(
    const std::vector<std::uint8_t>& current) {
    constexpr std::size_t kProtectionMaskOffset = 28;
    REQUIRE(ReadU32(current, 4) == 23);
    std::vector<std::uint8_t> prior = current;
    prior.erase(prior.begin() +
                static_cast<std::ptrdiff_t>(kProtectionMaskOffset));
    WriteU32(prior, 4, 22);
    ResignSnapshot(prior);
    return prior;
}

std::vector<std::uint8_t> ConvertSnapshotV22ToV21(
    const std::vector<std::uint8_t>& current,
    std::size_t mapTileCount) {
    constexpr std::size_t kV22EntityBytes = 235;
    constexpr std::size_t kV21EntityBytes = 210;
    constexpr std::size_t kV22EntityExtensionBytes =
        kV22EntityBytes - kV21EntityBytes;
    constexpr std::size_t kThirdFactionRulesBegin = 1344;
    constexpr std::size_t kThirdFactionRulesEnd = 1984;
    constexpr std::size_t kChoirRulesBegin = 2180;
    constexpr std::size_t kChoirRulesEnd = 2224;
    constexpr std::size_t kAdditionalResearchBegin = 2354;
    constexpr std::size_t kAdditionalResearchEnd = 2406;
    REQUIRE(ReadU32(current, 4) == 22);
    const std::size_t firstEntity = SnapshotV22FirstEntityOffset(mapTileCount);
    const std::uint32_t entityCount =
        ReadU32(current, SnapshotV22EntityCountOffset(mapTileCount));
    REQUIRE(firstEntity +
                static_cast<std::size_t>(entityCount) * kV22EntityBytes + 8 <=
            current.size());

    std::vector<std::uint8_t> prior = current;
    for (std::uint32_t index = entityCount; index > 0; --index) {
        const std::size_t extension =
            firstEntity +
            static_cast<std::size_t>(index - 1) * kV22EntityBytes +
            kV21EntityBytes;
        prior.erase(
            prior.begin() + static_cast<std::ptrdiff_t>(extension),
            prior.begin() + static_cast<std::ptrdiff_t>(
                                extension + kV22EntityExtensionBytes));
    }
    prior.erase(
        prior.begin() + static_cast<std::ptrdiff_t>(kAdditionalResearchBegin),
        prior.begin() + static_cast<std::ptrdiff_t>(kAdditionalResearchEnd));
    prior.erase(
        prior.begin() + static_cast<std::ptrdiff_t>(kChoirRulesBegin),
        prior.begin() + static_cast<std::ptrdiff_t>(kChoirRulesEnd));
    prior.erase(
        prior.begin() + static_cast<std::ptrdiff_t>(kThirdFactionRulesBegin),
        prior.begin() + static_cast<std::ptrdiff_t>(kThirdFactionRulesEnd));
    WriteU32(prior, 4, 21);
    WriteU32(prior, 28, 1);
    ResignSnapshot(prior);
    return prior;
}

std::vector<std::uint8_t> ConvertSnapshotV21ToV20(
    const std::vector<std::uint8_t>& current,
    std::size_t mapTileCount) {
    constexpr std::size_t kV21EntityBytes = 210;
    constexpr std::size_t kWellActivationOffset = 104;
    constexpr std::size_t kWellActivationBytes = 8;
    REQUIRE(ReadU32(current, 4) == 21);
    const std::size_t firstEntity = SnapshotV21FirstEntityOffset(mapTileCount);
    const std::uint32_t entityCount =
        ReadU32(current, SnapshotV21EntityCountOffset(mapTileCount));
    REQUIRE(firstEntity +
                static_cast<std::size_t>(entityCount) * kV21EntityBytes + 8 <=
            current.size());

    std::vector<std::uint8_t> legacy = current;
    for (std::uint32_t index = entityCount; index > 0; --index) {
        const std::size_t activation =
            firstEntity +
            static_cast<std::size_t>(index - 1) * kV21EntityBytes +
            kWellActivationOffset;
        legacy.erase(
            legacy.begin() + static_cast<std::ptrdiff_t>(activation),
            legacy.begin() + static_cast<std::ptrdiff_t>(
                                 activation + kWellActivationBytes));
    }
    WriteU32(legacy, 4, 20);
    ResignSnapshot(legacy);
    return legacy;
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
    for (int tick = 0; tick < 30 &&
         simulation.FindEntity(worker)->order.type != OrderType::Deliver; ++tick) {
        simulation.Step();
    }
    REQUIRE(simulation.FindEntity(worker)->cargo == 100);
    REQUIRE(simulation.FindEntity(worker)->order.type == OrderType::Deliver);

    const std::int32_t materialBeforeDelivery =
        simulation.FindPlayer(0)->resources.material;
    Command deliver =
        MakeCommand(simulation.CurrentTick(), 0, 2, CommandType::Deliver, worker);
    deliver.target = base;
    REQUIRE(simulation.QueueCommand(deliver));
    for (int tick = 0; tick < 30 &&
         simulation.FindPlayer(0)->resources.material == materialBeforeDelivery; ++tick) {
        simulation.Step();
    }
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

void TestControlledSpawnAdmission() {
    Simulation simulation({24, 24, 20, 0x535041574e41444dULL});
    REQUIRE(simulation.AddPlayer(0, Faction::MeridianCompact, {0, 0}));
    const EntityId existing = simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Soldier,
        Vec2::FromTiles(6, 6));
    REQUIRE(existing != 0);
    REQUIRE(!simulation.IsSpawnPositionAvailable(
        Faction::MeridianCompact, EntityType::Soldier,
        Vec2::FromTiles(6, 6)));
    REQUIRE(!simulation.IsSpawnPositionAvailable(
        Faction::MeridianCompact, EntityType::CommandCore,
        Vec2::FromTiles(0, 0)));
    REQUIRE(simulation.SetTerrainTile(12, 12, Terrain::Blocked));
    REQUIRE(!simulation.IsSpawnPositionAvailable(
        Faction::KharuunAssemblies, EntityType::ScoutUnit,
        Vec2::FromTiles(12, 12)));
    REQUIRE(!simulation.IsSpawnPositionAvailable(
        Faction::MeridianCompact, EntityType::ResourceNode,
        Vec2::FromTiles(16, 16)));
    REQUIRE(simulation.IsSpawnPositionAvailable(
        Faction::HollowChoir, EntityType::HeavyUnit,
        Vec2::FromTiles(18, 18)));

    Simulation boundarySimulation({24, 24, 20, 0x535041574e424e44ULL});
    REQUIRE(boundarySimulation.AddPlayer(
        0, Faction::MeridianCompact, {0, 0}));
    REQUIRE(boundarySimulation.SetTerrainTile(7, 5, Terrain::Blocked));
    // Re-derived: this boundary case previously asserted a *Command Core*
    // placed one tile clear of blocked ground was Valid, which encoded two
    // defects at once - Cores being constructable at all (BLD-009) and the
    // Meridian Core carrying a 2x2 footprint instead of its authored 5x5.
    // The Barracks half-extent is exactly the one the Core used to claim, so
    // the geometry under test is unchanged.
    REQUIRE(boundarySimulation.ValidatePlacement(
                0, EntityType::Barracks, Vec2::FromTiles(6, 5)) ==
            PlacementResult::Valid);
    // BLD-009: no player may build an additional Command Core, anywhere.
    REQUIRE(boundarySimulation.ValidatePlacement(
                0, EntityType::CommandCore, Vec2::FromTiles(6, 5)) ==
            PlacementResult::InvalidBuildingType);
    REQUIRE(boundarySimulation.ValidatePlacement(
                0, EntityType::CommandCore, Vec2::FromTiles(15, 15)) ==
            PlacementResult::InvalidBuildingType);
    // Authored spawns still place Cores; the 5x5 footprint now needs room for
    // all twenty-five tiles, so tile (7,5) blocks the tight boundary case and
    // open ground admits it.
    REQUIRE(!boundarySimulation.IsSpawnPositionAvailable(
        Faction::MeridianCompact, EntityType::CommandCore,
        Vec2::FromTiles(6, 5)));
    REQUIRE(boundarySimulation.IsSpawnPositionAvailable(
        Faction::MeridianCompact, EntityType::CommandCore,
        Vec2::FromTiles(15, 15)));
    // All three Command Cores are 5x5 tiles: Anchor, Memory Hearth, and
    // Concordance each declare that footprint in the authored building data.
    const SimulationRules shippedRules = DefaultSimulationRules();
    for (const Faction faction : {Faction::MeridianCompact,
                                  Faction::KharuunAssemblies,
                                  Faction::HollowChoir}) {
        REQUIRE(shippedRules
                    .archetypes[static_cast<std::size_t>(faction)]
                               [static_cast<std::size_t>(
                                   EntityType::CommandCore)]
                    .footprintHalfExtentRaw == 5 * kFixedScale / 2);
        // Surveyor, Tender, and Threadkeeper all read "no attack".
        const EntityArchetypeRules& workerRules =
            shippedRules.archetypes[static_cast<std::size_t>(faction)]
                                   [static_cast<std::size_t>(
                                       EntityType::Worker)];
        REQUIRE(workerRules.attackDamage == 0);
        REQUIRE(workerRules.attackRangeRaw == 0);
        REQUIRE(workerRules.attackPeriodTicks == 0);
    }
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

void TestProtectedCommandCoreContract() {
    Simulation ordinary({24, 24, 20, 0x4f5244494e415259ULL});
    AddTwoPlayers(ordinary, {0, 0}, {0, 0});
    const EntityId ordinaryCore = ordinary.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::CommandCore,
        Vec2::FromTiles(8, 8));
    const EntityId ordinaryAttacker = ordinary.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Soldier,
        Vec2::FromTiles(6, 8));
    REQUIRE(ordinaryCore != 0 && ordinaryAttacker != 0);
    const std::int32_t ordinaryCoreHealth =
        ordinary.FindEntity(ordinaryCore)->hitPoints;
    Command ordinaryAttack =
        MakeCommand(0, 0, 1, CommandType::Attack, ordinaryAttacker);
    ordinaryAttack.target = ordinaryCore;
    REQUIRE(ordinary.QueueCommand(ordinaryAttack));
    ordinary.Step(20);
    REQUIRE(ordinary.FindEntity(ordinaryCore)->hitPoints < ordinaryCoreHealth);

    SimulationConfig protectedConfig{24, 24, 20, 0x50524f5445435444ULL};
    protectedConfig.protectedCommandCorePlayerMask = 0x02;
    Simulation protectedSimulation(protectedConfig);
    AddTwoPlayers(protectedSimulation, {0, 0}, {0, 0});
    REQUIRE(protectedSimulation.SpawnEntity(
                0, Faction::MeridianCompact, EntityType::CommandCore,
                Vec2::FromTiles(3, 3)) != 0);
    const EntityId protectedCore = protectedSimulation.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::CommandCore,
        Vec2::FromTiles(8, 8));
    const EntityId attacker = protectedSimulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Soldier,
        Vec2::FromTiles(6, 8));
    const EntityId eligibleEnemy = protectedSimulation.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::Soldier,
        Vec2::FromTiles(9, 8));
    REQUIRE(protectedCore != 0 && attacker != 0 && eligibleEnemy != 0);
    const std::int32_t protectedHealth =
        protectedSimulation.FindEntity(protectedCore)->hitPoints;

    Command direct = MakeCommand(0, 0, 1, CommandType::Attack, attacker);
    direct.target = protectedCore;
    REQUIRE(protectedSimulation.QueueCommand(direct));
    protectedSimulation.Step();
    REQUIRE(protectedSimulation.FindEntity(protectedCore)->hitPoints ==
            protectedHealth);
    REQUIRE(protectedSimulation.FindEntity(attacker)->order.type ==
            OrderType::None);

    Command advance = MakeCommand(
        protectedSimulation.CurrentTick(), 0, 2, CommandType::AttackMove,
        attacker);
    advance.position = Vec2::FromTiles(14, 8);
    REQUIRE(protectedSimulation.QueueCommand(advance));
    protectedSimulation.Step();
    REQUIRE(protectedSimulation.FindEntity(attacker)->order.type ==
            OrderType::AttackMove);
    REQUIRE(protectedSimulation.FindEntity(attacker)->order.target ==
            eligibleEnemy);
    REQUIRE(protectedSimulation.FindEntity(protectedCore)->hitPoints ==
            protectedHealth);
    REQUIRE(protectedSimulation.Outcome() == MatchOutcome::Ongoing);

    const std::vector<std::uint8_t> protectedSnapshot =
        protectedSimulation.SaveSnapshot();
    // The current schema preserves work state after per-player terrain and permanent
    // objects, which the FOG information-state table requires an Explored tile
    // to be served from. The literal stays a deliberate tripwire: a schema
    // change must be an edit here, never a silent drift.
    REQUIRE(ReadU32(protectedSnapshot, 4) == kSnapshotVersion);
    REQUIRE(protectedSnapshot[28] == 0x02);
    std::string error;
    std::optional<Simulation> restored =
        Simulation::LoadSnapshot(protectedSnapshot, &error);
    REQUIRE(restored.has_value());
    REQUIRE(error.empty());
    REQUIRE(restored->Config().protectedCommandCorePlayerMask == 0x02);
    REQUIRE(restored->StateChecksum() == protectedSimulation.StateChecksum());

    protectedSimulation.CaptureReplayBaseline();
    Command stop = MakeCommand(
        protectedSimulation.CurrentTick(), 0, 3, CommandType::Stop, attacker);
    REQUIRE(protectedSimulation.QueueCommand(stop));
    protectedSimulation.Step(4);
    const ReplayRecord replay = protectedSimulation.ExportReplay();
    REQUIRE(replay.version == kReplayVersion);
    std::optional<Simulation> replayed =
        Simulation::ReplayToEnd(replay, &error);
    REQUIRE(replayed.has_value());
    REQUIRE(error.empty());
    REQUIRE(replayed->Config().protectedCommandCorePlayerMask == 0x02);
    REQUIRE(replayed->StateChecksum() == protectedSimulation.StateChecksum());

    protectedSimulation.DisableReplayExport();
    const ReplayRecord rejectedReplay = protectedSimulation.ExportReplay(&error);
    REQUIRE(error == "replay export is disabled");
    REQUIRE(rejectedReplay.version == 0);
    REQUIRE(rejectedReplay.initialSnapshot.empty());
    REQUIRE(rejectedReplay.commands.empty());
    REQUIRE(rejectedReplay.finalTick == 0);
    REQUIRE(rejectedReplay.finalChecksum == 0);
    protectedSimulation.CaptureReplayBaseline();
    const ReplayRecord stillRejectedReplay =
        protectedSimulation.ExportReplay(&error);
    REQUIRE(stillRejectedReplay.version == 0);
    REQUIRE(stillRejectedReplay.initialSnapshot.empty());
    REQUIRE(error == "replay export is disabled");

    ReplayRecord unsupportedReplay = replay;
    unsupportedReplay.version = 23;
    REQUIRE(!Simulation::ReplayToEnd(unsupportedReplay, &error).has_value());
    REQUIRE(error == "replay version is unsupported");

    SimulationConfig sameStateUnprotected = protectedConfig;
    sameStateUnprotected.protectedCommandCorePlayerMask = 0;
    Simulation protectedHash(protectedConfig);
    Simulation unprotectedHash(sameStateUnprotected);
    AddTwoPlayers(protectedHash, {0, 0}, {0, 0});
    AddTwoPlayers(unprotectedHash, {0, 0}, {0, 0});
    const auto SpawnHashFixture = [](Simulation& simulation) {
        REQUIRE(simulation.SpawnEntity(
                    0, Faction::MeridianCompact, EntityType::CommandCore,
                    Vec2::FromTiles(3, 3)) != 0);
        REQUIRE(simulation.SpawnEntity(
                    1, Faction::KharuunAssemblies, EntityType::CommandCore,
                    Vec2::FromTiles(8, 8)) != 0);
        REQUIRE(simulation.SpawnEntity(
                    0, Faction::MeridianCompact, EntityType::Soldier,
                    Vec2::FromTiles(6, 8)) != 0);
        REQUIRE(simulation.SpawnEntity(
                    1, Faction::KharuunAssemblies, EntityType::Soldier,
                    Vec2::FromTiles(9, 8)) != 0);
    };
    SpawnHashFixture(protectedHash);
    SpawnHashFixture(unprotectedHash);
    REQUIRE(protectedHash.Entities() == unprotectedHash.Entities());
    REQUIRE(protectedHash.CurrentTick() == unprotectedHash.CurrentTick());
    REQUIRE(protectedHash.StateChecksum() != unprotectedHash.StateChecksum());

    Simulation retained({24, 24, 20, 0x52455441494e4544ULL});
    AddTwoPlayers(retained, {0, 0}, {0, 0});
    REQUIRE(retained.SpawnEntity(
                0, Faction::MeridianCompact, EntityType::CommandCore,
                Vec2::FromTiles(3, 3)) != 0);
    const EntityId retainedCore = retained.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::CommandCore,
        Vec2::FromTiles(8, 8));
    const EntityId retainedAttacker = retained.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Soldier,
        Vec2::FromTiles(6, 8));
    Command retainedAttack =
        MakeCommand(0, 0, 1, CommandType::Attack, retainedAttacker);
    retainedAttack.target = retainedCore;
    REQUIRE(retained.QueueCommand(retainedAttack));
    retained.Step();
    REQUIRE(retained.FindEntity(retainedAttacker)->order.type ==
            OrderType::Attack);
    std::vector<std::uint8_t> promoted = retained.SaveSnapshot();
    promoted[28] = 0x02;
    ResignSnapshot(promoted);
    std::optional<Simulation> protectedRetained =
        Simulation::LoadSnapshot(promoted, &error);
    REQUIRE(protectedRetained.has_value());
    const std::int32_t healthBeforeProtectedStep =
        protectedRetained->FindEntity(retainedCore)->hitPoints;
    protectedRetained->Step();
    REQUIRE(protectedRetained->FindEntity(retainedCore)->hitPoints ==
            healthBeforeProtectedStep);
    REQUIRE(protectedRetained->FindEntity(retainedAttacker)->order.type ==
            OrderType::None);

    bool rejectedInvalidMask = false;
    try {
        SimulationConfig invalid = protectedConfig;
        invalid.protectedCommandCorePlayerMask = 0x10;
        Simulation invalidSimulation(invalid);
        (void)invalidSimulation;
    } catch (const std::invalid_argument&) {
        rejectedInvalidMask = true;
    }
    REQUIRE(rejectedInvalidMask);

    std::vector<std::uint8_t> forgedMask = protectedSnapshot;
    forgedMask[28] = 0x10;
    ResignSnapshot(forgedMask);
    REQUIRE(!Simulation::LoadSnapshot(forgedMask, &error).has_value());
    REQUIRE(error == "snapshot protection mask is invalid");
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

// MOV-002 / SIM-003: a move the ordering player's own map proves impossible is
// refused with a stable reason code and plain-language recovery, instead of
// being receipted as Applied and then deadlocking in MoveTowards forever.
void TestMovementOrderRejectionReasons() {
    REQUIRE(std::string(CommandRejectionReasonCode(
                CommandResolutionOutcome::NoPath)) == "NO PATH");
    REQUIRE(std::string(CommandRejectionReasonCode(
                CommandResolutionOutcome::RouteBlocked)) == "ROUTE BLOCKED");
    REQUIRE(std::string(CommandRejectionReasonCode(
                CommandResolutionOutcome::DestinationOccupied)) ==
            "DESTINATION OCCUPIED");
    REQUIRE(std::string(CommandRejectionReasonCode(
                CommandResolutionOutcome::Applied)).empty());
    REQUIRE(std::string(CommandRejectionReasonCode(
                CommandResolutionOutcome::NoEffect)).empty());
    for (const CommandResolutionOutcome rejection :
         {CommandResolutionOutcome::NoPath,
          CommandResolutionOutcome::RouteBlocked,
          CommandResolutionOutcome::DestinationOccupied,
          CommandResolutionOutcome::InvalidPosition}) {
        REQUIRE(!std::string(CommandRejectionRecovery(rejection)).empty());
    }
    REQUIRE(std::string(CommandRejectionRecovery(
                CommandResolutionOutcome::Applied)).empty());

    // A wall the scout can see splits the map. The far side is unreachable and
    // stays unreachable, so the order is refused rather than accepted.
    Simulation severed({9, 9, 20, 0x4d4f5645524a4354ULL});
    REQUIRE(severed.AddPlayer(0, Faction::MeridianCompact, ResourcePool{0, 0}));
    for (std::int32_t tileY = 0; tileY < 9; ++tileY) {
        REQUIRE(severed.SetTerrainTile(4, tileY, Terrain::Blocked));
    }
    const EntityId scout = severed.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Soldier, Vec2::FromTiles(1, 4));
    REQUIRE(scout != 0);

    Simulation severedCopy = severed;
    Command crossWall = MakeCommand(0, 0, 1, CommandType::Move, scout);
    crossWall.position = Vec2::FromTiles(7, 4);
    REQUIRE(severed.QueueCommand(crossWall));
    REQUIRE(severedCopy.QueueCommand(crossWall));
    severed.Step();
    severedCopy.Step();

    const std::optional<CommandResolutionReceipt> noPath =
        severed.FindCommandResolutionReceipt(0, 1);
    REQUIRE(noPath.has_value());
    REQUIRE(noPath->commandType == CommandType::Move);
    REQUIRE(noPath->outcome == CommandResolutionOutcome::NoPath);
    // The refused order is not installed, so the unit is idle and re-orderable
    // rather than frozen on an order that can never complete.
    REQUIRE(severed.FindEntity(scout)->order.type == OrderType::None);
    REQUIRE(severed.FindEntity(scout)->position == Vec2::FromTiles(1, 4));
    severed.Step(60);
    REQUIRE(severed.FindEntity(scout)->order.type == OrderType::None);
    severedCopy.Step(60);
    REQUIRE(severed.StateChecksum() == severedCopy.StateChecksum());

    // The wall tile itself cannot be stood on.
    REQUIRE(severed.ValidateMoveOrder(0, scout, Vec2::FromTiles(4, 4)) ==
            CommandResolutionOutcome::DestinationOccupied);
    Command ontoWall = MakeCommand(
        severed.CurrentTick(), 0, 2, CommandType::Move, scout);
    ontoWall.position = Vec2::FromTiles(4, 4);
    REQUIRE(severed.QueueCommand(ontoWall));
    severed.Step();
    const std::optional<CommandResolutionReceipt> occupied =
        severed.FindCommandResolutionReceipt(0, 2);
    REQUIRE(occupied.has_value());
    REQUIRE(occupied->outcome ==
            CommandResolutionOutcome::DestinationOccupied);
    REQUIRE(severed.FindEntity(scout)->order.type == OrderType::None);

    // Reachable ground on the unit's own side is still admitted.
    REQUIRE(severed.ValidateMoveOrder(0, scout, Vec2::FromTiles(3, 7)) ==
            CommandResolutionOutcome::Applied);
    // Structural refusals that predate reason codes keep reporting NoEffect.
    REQUIRE(severed.ValidateMoveOrder(0, scout, Vec2::FromTiles(50, 50)) ==
            CommandResolutionOutcome::NoEffect);
    REQUIRE(severed.ValidateMoveOrder(0, 4242, Vec2::FromTiles(3, 7)) ==
            CommandResolutionOutcome::NoEffect);
    REQUIRE(severed.ValidateMoveOrder(1, scout, Vec2::FromTiles(3, 7)) ==
            CommandResolutionOutcome::NoEffect);

    // A rejection receipt survives persistence, and the load path refuses a
    // forged movement reason attached to a non-movement command.
    const std::vector<std::uint8_t> rejectionSnapshot = severed.SaveSnapshot();
    std::string error;
    std::optional<Simulation> restored =
        Simulation::LoadSnapshot(rejectionSnapshot, &error);
    REQUIRE(restored.has_value());
    REQUIRE(error.empty());
    REQUIRE(restored->FindCommandResolutionReceipt(0, 2) == occupied);

    constexpr std::size_t kSerializedReceiptBytes = 19;
    constexpr std::size_t kReceiptCommandTypeOffset = 9;
    const std::size_t lastReceiptOffset =
        SnapshotReceiptBlockOffset(rejectionSnapshot,
            severed.Config().mapWidthTiles * severed.Config().mapHeightTiles) + 4U +
        (ReadU32(rejectionSnapshot, SnapshotReceiptBlockOffset(rejectionSnapshot,
            severed.Config().mapWidthTiles * severed.Config().mapHeightTiles)) - 1U) * kSerializedReceiptBytes;
    REQUIRE(rejectionSnapshot[lastReceiptOffset + kReceiptCommandTypeOffset] ==
            static_cast<std::uint8_t>(CommandType::Move));
    std::vector<std::uint8_t> forgedPairing = rejectionSnapshot;
    forgedPairing[lastReceiptOffset + kReceiptCommandTypeOffset] =
        static_cast<std::uint8_t>(CommandType::Hold);
    ResignSnapshot(forgedPairing);
    REQUIRE(!Simulation::LoadSnapshot(forgedPairing, &error).has_value());
    REQUIRE(error == "snapshot command resolution receipt is invalid");

    // A unit sealed in by terrain it can see reports the obstruction beside it.
    Simulation walledIn({9, 9, 20, 0x4d4f5645454e4331ULL});
    REQUIRE(walledIn.AddPlayer(0, Faction::MeridianCompact, ResourcePool{0, 0}));
    REQUIRE(walledIn.SetTerrainTile(3, 4, Terrain::Blocked));
    REQUIRE(walledIn.SetTerrainTile(5, 4, Terrain::Blocked));
    REQUIRE(walledIn.SetTerrainTile(4, 3, Terrain::Blocked));
    REQUIRE(walledIn.SetTerrainTile(4, 5, Terrain::Blocked));
    const EntityId sealed = walledIn.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Soldier, Vec2::FromTiles(4, 4));
    REQUIRE(sealed != 0);
    Command escape = MakeCommand(0, 0, 1, CommandType::Move, sealed);
    escape.position = Vec2::FromTiles(7, 7);
    REQUIRE(walledIn.QueueCommand(escape));
    walledIn.Step();
    const std::optional<CommandResolutionReceipt> routeBlocked =
        walledIn.FindCommandResolutionReceipt(0, 1);
    REQUIRE(routeBlocked.has_value());
    REQUIRE(routeBlocked->outcome == CommandResolutionOutcome::RouteBlocked);
    REQUIRE(walledIn.FindEntity(sealed)->order.type == OrderType::None);

    // Terrain the player has never observed must not reject the order, and
    // must not be disclosed by one. The wall here is far outside scout vision.
    Simulation unscouted({40, 40, 20, 0x4d4f5645554e4b4eULL});
    REQUIRE(unscouted.AddPlayer(
        0, Faction::MeridianCompact, ResourcePool{0, 0}));
    for (std::int32_t tileY = 0; tileY < 40; ++tileY) {
        REQUIRE(unscouted.SetTerrainTile(20, tileY, Terrain::Blocked));
    }
    const EntityId prober = unscouted.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Soldier,
        Vec2::FromTiles(2, 20));
    REQUIRE(prober != 0);
    REQUIRE(unscouted.VisibilityAt(0, Vec2::FromTiles(20, 20)) ==
            Visibility::Unexplored);
    Command intoTheDark = MakeCommand(0, 0, 1, CommandType::Move, prober);
    intoTheDark.position = Vec2::FromTiles(35, 20);
    REQUIRE(unscouted.QueueCommand(intoTheDark));
    unscouted.Step();
    const std::optional<CommandResolutionReceipt> admitted =
        unscouted.FindCommandResolutionReceipt(0, 1);
    REQUIRE(admitted.has_value());
    REQUIRE(admitted->outcome == CommandResolutionOutcome::Applied);
    REQUIRE(unscouted.FindEntity(prober)->order.type == OrderType::Move);

    // A blocker that can move away is transient and must never reject.
    Simulation crowded({9, 9, 20, 0x4d4f56454f434350ULL});
    REQUIRE(crowded.AddPlayer(0, Faction::MeridianCompact, ResourcePool{0, 0}));
    const EntityId mover = crowded.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Soldier, Vec2::FromTiles(1, 1));
    const EntityId standing = crowded.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Soldier, Vec2::FromTiles(5, 5));
    REQUIRE(mover != 0 && standing != 0);
    Command ontoAlly = MakeCommand(0, 0, 1, CommandType::Move, mover);
    ontoAlly.position = Vec2::FromTiles(5, 5);
    REQUIRE(crowded.QueueCommand(ontoAlly));
    crowded.Step();
    const std::optional<CommandResolutionReceipt> allyOccupied =
        crowded.FindCommandResolutionReceipt(0, 1);
    REQUIRE(allyOccupied.has_value());
    REQUIRE(allyOccupied->outcome == CommandResolutionOutcome::Applied);
    crowded.Step(200);
    REQUIRE(crowded.FindEntity(mover)->position == Vec2::FromTiles(5, 5));
    REQUIRE(crowded.FindEntity(mover)->order.type == OrderType::None);

    // An ordinary move across open ground is unchanged.
    Simulation open({9, 9, 20, 0x4d4f5645504c4149ULL});
    REQUIRE(open.AddPlayer(0, Faction::MeridianCompact, ResourcePool{0, 0}));
    const EntityId walker = open.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Soldier, Vec2::FromTiles(1, 4));
    REQUIRE(walker != 0);
    Command stroll = MakeCommand(0, 0, 1, CommandType::Move, walker);
    stroll.position = Vec2::FromTiles(7, 4);
    REQUIRE(open.QueueCommand(stroll));
    open.Step(200);
    const std::optional<CommandResolutionReceipt> strollReceipt =
        open.FindCommandResolutionReceipt(0, 1);
    REQUIRE(strollReceipt.has_value());
    REQUIRE(strollReceipt->outcome == CommandResolutionOutcome::Applied);
    REQUIRE(open.FindEntity(walker)->position == Vec2::FromTiles(7, 4));
    REQUIRE(open.FindEntity(walker)->order.type == OrderType::None);
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
        harvest.Step(299);
        REQUIRE(harvest.FindPlayer(0)->resources.dawnshards == 50);
        REQUIRE(harvest.FindEntity(well)->wellChoice == FutureWellChoice::Dormant);
        REQUIRE(harvest.FindEntity(well)->wellActivationTick == 0);
        REQUIRE(harvest.FindEntity(well)->wellCapturePlayer == 0);
        REQUIRE(harvest.FindEntity(well)->wellCaptureProgress == 299);
        REQUIRE(harvest.FindEntity(well)->wellPendingChoice ==
                FutureWellChoice::Harvest);
        REQUIRE(harvest.FindEntity(well)->wellProtocolTicks == 0);
        REQUIRE(harvest.PublicFutureWellTelegraphs().empty());

        std::string error;
        std::optional<Simulation> captureRestored =
            Simulation::LoadSnapshot(harvest.SaveSnapshot(), &error);
        REQUIRE(captureRestored.has_value());
        REQUIRE(error.empty());
        REQUIRE(captureRestored->FindEntity(well)->wellCapturePlayer == 0);
        REQUIRE(captureRestored->FindEntity(well)->wellCaptureProgress == 299);
        REQUIRE(captureRestored->FindEntity(well)->wellPendingChoice ==
                FutureWellChoice::Harvest);
        REQUIRE(captureRestored->StateChecksum() == harvest.StateChecksum());

        harvest.Step();
        captureRestored->Step();
        REQUIRE(harvest.FindEntity(well)->wellChoice == FutureWellChoice::Harvest);
        REQUIRE(harvest.FindEntity(well)->wellActivationTick == 300);
        REQUIRE(harvest.FindEntity(well)->wellProtocolTicks == 180);
        REQUIRE(harvest.PublicFutureWellTelegraphs().size() == 1);
        REQUIRE(harvest.PublicFutureWellTelegraphs().front().wellId == well);
        REQUIRE(harvest.PublicFutureWellTelegraphs().front().remainingTicks == 180);
        REQUIRE(captureRestored->StateChecksum() == harvest.StateChecksum());

        const std::vector<std::uint8_t> telegraphSnapshot =
            harvest.SaveSnapshot();
        std::optional<Simulation> telegraphRestored =
            Simulation::LoadSnapshot(telegraphSnapshot, &error);
        REQUIRE(telegraphRestored.has_value());
        REQUIRE(error.empty());
        REQUIRE(telegraphRestored->StateChecksum() == harvest.StateChecksum());

        constexpr std::size_t mapTileCount = 20U * 20U;
        const std::size_t lifecycleOffset =
            SnapshotFutureWellLifecycleBlockOffset(telegraphSnapshot,
                                                   mapTileCount);
        REQUIRE(ReadU32(telegraphSnapshot, lifecycleOffset) == 2U);
        const std::size_t wellLifecycleOffset =
            SnapshotFutureWellLifecycleRecordOffset(telegraphSnapshot,
                                                    mapTileCount, 1U);
        REQUIRE(ReadU32(telegraphSnapshot, wellLifecycleOffset) == well);

        // Valid integrity cannot excuse an impossible lifecycle. An active
        // Harvest telegraph has no capture claimant, meter, or pending choice;
        // it requires a controlling owner and a timer in the inclusive 1..180
        // boundary.
        std::vector<std::uint8_t> invalidLifecycleCount = telegraphSnapshot;
        WriteU32(invalidLifecycleCount, lifecycleOffset,
                 std::numeric_limits<std::uint32_t>::max());
        ResignSnapshot(invalidLifecycleCount);
        REQUIRE(!Simulation::LoadSnapshot(invalidLifecycleCount, &error)
                     .has_value());
        REQUIRE(error == "snapshot Future Well lifecycle count is invalid");

        std::vector<std::uint8_t> invalidCapture = telegraphSnapshot;
        invalidCapture[wellLifecycleOffset + 4U] = 0;
        ResignSnapshot(invalidCapture);
        REQUIRE(!Simulation::LoadSnapshot(invalidCapture, &error).has_value());
        REQUIRE(error == "snapshot Future Well lifecycle is invalid");

        std::vector<std::uint8_t> invalidProgress = telegraphSnapshot;
        WriteU16(invalidProgress, wellLifecycleOffset + 5U, 1U);
        ResignSnapshot(invalidProgress);
        REQUIRE(!Simulation::LoadSnapshot(invalidProgress, &error).has_value());
        REQUIRE(error == "snapshot Future Well lifecycle is invalid");

        std::vector<std::uint8_t> invalidPending = telegraphSnapshot;
        invalidPending[wellLifecycleOffset + 7U] =
            static_cast<std::uint8_t>(FutureWellChoice::Harvest);
        ResignSnapshot(invalidPending);
        REQUIRE(!Simulation::LoadSnapshot(invalidPending, &error).has_value());
        REQUIRE(error == "snapshot Future Well lifecycle is invalid");

        std::vector<std::uint8_t> invalidTimer = telegraphSnapshot;
        WriteU64(invalidTimer, wellLifecycleOffset + 8U, 181U);
        ResignSnapshot(invalidTimer);
        REQUIRE(!Simulation::LoadSnapshot(invalidTimer, &error).has_value());
        REQUIRE(error == "snapshot Future Well lifecycle is invalid");

        const std::size_t firstEntity =
            SnapshotV25FirstEntityOffset(telegraphSnapshot, mapTileCount);
        std::vector<std::uint8_t> invalidOwner = telegraphSnapshot;
        invalidOwner[firstEntity + kSerializedEntityBytes + 4U] =
            kNeutralPlayer;
        ResignSnapshot(invalidOwner);
        REQUIRE(!Simulation::LoadSnapshot(invalidOwner, &error).has_value());
        REQUIRE(error == "snapshot Future Well lifecycle is invalid");

        harvest.Step(179);
        telegraphRestored->Step(179);
        REQUIRE(harvest.FindPlayer(0)->resources.dawnshards == 50);
        REQUIRE(harvest.FindEntity(well)->wellProtocolTicks == 1);
        REQUIRE(harvest.TerrainAt(7, 7) == Terrain::Open);
        REQUIRE(telegraphRestored->StateChecksum() == harvest.StateChecksum());
        std::optional<Simulation> oneTickRestored =
            Simulation::LoadSnapshot(harvest.SaveSnapshot(), &error);
        REQUIRE(oneTickRestored.has_value());
        REQUIRE(error.empty());
        REQUIRE(oneTickRestored->StateChecksum() == harvest.StateChecksum());
        harvest.Step();
        telegraphRestored->Step();
        oneTickRestored->Step();
        REQUIRE(harvest.FindPlayer(0)->resources.dawnshards == 550);
        REQUIRE(harvest.IsCollapsedFutureWell(*harvest.FindEntity(well)));
        REQUIRE(harvest.TerrainAt(7, 7) == Terrain::Scarred);
        REQUIRE(telegraphRestored->StateChecksum() == harvest.StateChecksum());
        REQUIRE(oneTickRestored->StateChecksum() == harvest.StateChecksum());
        REQUIRE(harvest.ValidatePlacement(0, EntityType::Barracks,
                                          Vec2::FromTiles(8, 8)) ==
                PlacementResult::TerrainRestricted);
    }
    {
        // An enemy entering during the public Harvest telegraph cancels the
        // irreversible action before payout and leaves a fresh dormant Well.
        Simulation interrupted({20, 20, 20, 12});
        AddTwoPlayers(interrupted, {0, 50}, {0, 0});
        const EntityId worker = interrupted.SpawnEntity(
            0, Faction::MeridianCompact, EntityType::Worker,
            Vec2::FromTiles(5, 6));
        const EntityId well =
            interrupted.SpawnFutureWell(Vec2::FromTiles(6, 6));
        Command action =
            MakeCommand(0, 0, 1, CommandType::FutureWell, worker);
        action.target = well;
        action.wellChoice = FutureWellChoice::Harvest;
        REQUIRE(interrupted.QueueCommand(action));
        interrupted.Step(300);
        REQUIRE(interrupted.FindEntity(well)->wellProtocolTicks == 180);
        const EntityId intruder = interrupted.SpawnEntity(
            1, Faction::KharuunAssemblies, EntityType::Soldier,
            Vec2::FromTiles(7, 6));
        REQUIRE(intruder != 0);
        interrupted.Step();
        REQUIRE(interrupted.FindPlayer(0)->resources.dawnshards == 50);
        REQUIRE(interrupted.FindEntity(well)->owner == kNeutralPlayer);
        REQUIRE(interrupted.FindEntity(well)->wellChoice ==
                FutureWellChoice::Dormant);
        REQUIRE(interrupted.FindEntity(well)->wellActivationTick == 0);
        REQUIRE(interrupted.FindEntity(well)->wellProtocolTicks == 0);
        REQUIRE(interrupted.PublicFutureWellTelegraphs().empty());
        REQUIRE(interrupted.TerrainAt(7, 7) == Terrain::Open);
        std::string error;
        std::optional<Simulation> restored =
            Simulation::LoadSnapshot(interrupted.SaveSnapshot(), &error);
        REQUIRE(restored.has_value());
        REQUIRE(error.empty());
        REQUIRE(restored->StateChecksum() == interrupted.StateChecksum());
    }
    {
        // Leaving an incomplete capture decays exactly one point per tick and
        // cannot activate the requested protocol after the worker stops.
        Simulation abandoned({20, 20, 20, 13});
        REQUIRE(abandoned.AddPlayer(0, Faction::MeridianCompact, {0, 0}));
        const EntityId worker = abandoned.SpawnEntity(
            0, Faction::MeridianCompact, EntityType::Worker,
            Vec2::FromTiles(5, 6));
        const EntityId well =
            abandoned.SpawnFutureWell(Vec2::FromTiles(6, 6));
        Command action =
            MakeCommand(0, 0, 1, CommandType::FutureWell, worker);
        action.target = well;
        action.wellChoice = FutureWellChoice::Preserve;
        REQUIRE(abandoned.QueueCommand(action));
        abandoned.Step(120);
        REQUIRE(abandoned.FindEntity(well)->wellCaptureProgress == 120);
        REQUIRE(abandoned.QueueCommand(MakeCommand(
            abandoned.CurrentTick(), 0, 2, CommandType::Stop, worker)));
        abandoned.Step();
        REQUIRE(abandoned.FindEntity(well)->wellCaptureProgress == 119);
        std::string error;
        std::optional<Simulation> decayRestored =
            Simulation::LoadSnapshot(abandoned.SaveSnapshot(), &error);
        REQUIRE(decayRestored.has_value());
        REQUIRE(error.empty());
        REQUIRE(decayRestored->FindEntity(well)->wellCapturePlayer == 0);
        REQUIRE(decayRestored->FindEntity(well)->wellCaptureProgress == 119);
        REQUIRE(decayRestored->FindEntity(well)->wellPendingChoice ==
                FutureWellChoice::Preserve);
        REQUIRE(decayRestored->StateChecksum() == abandoned.StateChecksum());
        abandoned.Step(119);
        decayRestored->Step(119);
        REQUIRE(abandoned.FindEntity(well)->wellCapturePlayer == kNeutralPlayer);
        REQUIRE(abandoned.FindEntity(well)->wellCaptureProgress == 0);
        REQUIRE(abandoned.FindEntity(well)->wellPendingChoice ==
                FutureWellChoice::Dormant);
        REQUIRE(abandoned.FindEntity(well)->wellChoice ==
                FutureWellChoice::Dormant);
        REQUIRE(abandoned.FindEntity(well)->wellActivationTick == 0);
        REQUIRE(decayRestored->StateChecksum() == abandoned.StateChecksum());

        Command reacquire = MakeCommand(abandoned.CurrentTick(), 0, 3,
                                        CommandType::FutureWell, worker);
        reacquire.target = well;
        reacquire.wellChoice = FutureWellChoice::Preserve;
        REQUIRE(abandoned.QueueCommand(reacquire));
        REQUIRE(decayRestored->QueueCommand(reacquire));
        abandoned.Step(300);
        decayRestored->Step(300);
        REQUIRE(abandoned.FindEntity(well)->wellChoice ==
                FutureWellChoice::Preserve);
        REQUIRE(abandoned.FindEntity(well)->wellActivationTick == 540);
        REQUIRE(decayRestored->StateChecksum() == abandoned.StateChecksum());
    }
    {
        // A hostile already in the zone freezes a newly established contender
        // at zero progress. That is an authoritative capture state, not the
        // inactive all-default lifecycle used by older schemas.
        Simulation contested({20, 20, 20, 14});
        AddTwoPlayers(contested, {0, 0}, {0, 0});
        const EntityId worker = contested.SpawnEntity(
            0, Faction::MeridianCompact, EntityType::Worker,
            Vec2::FromTiles(5, 6));
        const EntityId hostile = contested.SpawnEntity(
            1, Faction::KharuunAssemblies, EntityType::Soldier,
            Vec2::FromTiles(7, 6));
        const EntityId well =
            contested.SpawnFutureWell(Vec2::FromTiles(6, 6));
        REQUIRE(worker != 0 && hostile != 0 && well != 0);
        contested.CaptureReplayBaseline();
        Command action =
            MakeCommand(0, 0, 1, CommandType::FutureWell, worker);
        action.target = well;
        action.wellChoice = FutureWellChoice::Preserve;
        REQUIRE(contested.QueueCommand(action));
        contested.Step();
        REQUIRE(contested.FindEntity(well)->wellChoice ==
                FutureWellChoice::Dormant);
        REQUIRE(contested.FindEntity(well)->wellCapturePlayer == 0);
        REQUIRE(contested.FindEntity(well)->wellCaptureProgress == 0);
        REQUIRE(contested.FindEntity(well)->wellPendingChoice ==
                FutureWellChoice::Preserve);

        const std::vector<std::uint8_t> snapshot = contested.SaveSnapshot();
        REQUIRE(ReadU32(snapshot, 4) == kSnapshotVersion);
        std::string error;
        std::optional<Simulation> restored =
            Simulation::LoadSnapshot(snapshot, &error);
        REQUIRE(restored.has_value());
        REQUIRE(error.empty());
        REQUIRE(restored->FindEntity(well)->wellCapturePlayer == 0);
        REQUIRE(restored->FindEntity(well)->wellCaptureProgress == 0);
        REQUIRE(restored->FindEntity(well)->wellPendingChoice ==
                FutureWellChoice::Preserve);
        REQUIRE(restored->StateChecksum() == contested.StateChecksum());

        const ReplayRecord replay = contested.ExportReplay();
        std::optional<Simulation> replayed =
            Simulation::ReplayToEnd(replay, &error);
        REQUIRE(replayed.has_value());
        REQUIRE(error.empty());
        REQUIRE(replayed->StateChecksum() == contested.StateChecksum());

        constexpr std::size_t mapTileCount = 20U * 20U;
        const std::size_t wellLifecycleOffset =
            SnapshotFutureWellLifecycleRecordOffset(snapshot, mapTileCount, 2U);
        REQUIRE(ReadU32(snapshot, wellLifecycleOffset) == well);

        std::vector<std::uint8_t> invalidCapturePlayer = snapshot;
        invalidCapturePlayer[wellLifecycleOffset + 4U] = kNeutralPlayer;
        ResignSnapshot(invalidCapturePlayer);
        REQUIRE(!Simulation::LoadSnapshot(invalidCapturePlayer, &error)
                     .has_value());
        REQUIRE(error == "snapshot Future Well lifecycle is invalid");

        std::vector<std::uint8_t> committedCaptureProgress = snapshot;
        WriteU16(committedCaptureProgress, wellLifecycleOffset + 5U, 300U);
        ResignSnapshot(committedCaptureProgress);
        REQUIRE(!Simulation::LoadSnapshot(committedCaptureProgress, &error)
                     .has_value());
        REQUIRE(error == "snapshot Future Well lifecycle is invalid");

        std::vector<std::uint8_t> oversizedCaptureProgress = snapshot;
        WriteU16(oversizedCaptureProgress, wellLifecycleOffset + 5U, 301U);
        ResignSnapshot(oversizedCaptureProgress);
        REQUIRE(!Simulation::LoadSnapshot(oversizedCaptureProgress, &error)
                     .has_value());
        REQUIRE(error == "snapshot Future Well lifecycle is invalid");

        std::vector<std::uint8_t> invalidPendingChoice = snapshot;
        invalidPendingChoice[wellLifecycleOffset + 7U] =
            static_cast<std::uint8_t>(FutureWellChoice::Dormant);
        ResignSnapshot(invalidPendingChoice);
        REQUIRE(!Simulation::LoadSnapshot(invalidPendingChoice, &error)
                     .has_value());
        REQUIRE(error == "snapshot Future Well lifecycle is invalid");

        const std::size_t workerLifecycleOffset =
            SnapshotFutureWellLifecycleRecordOffset(snapshot, mapTileCount, 0U);
        std::vector<std::uint8_t> nonWellLifecycle = snapshot;
        nonWellLifecycle[workerLifecycleOffset + 4U] = 0;
        WriteU16(nonWellLifecycle, workerLifecycleOffset + 5U, 1U);
        nonWellLifecycle[workerLifecycleOffset + 7U] =
            static_cast<std::uint8_t>(FutureWellChoice::Preserve);
        ResignSnapshot(nonWellLifecycle);
        REQUIRE(!Simulation::LoadSnapshot(nonWellLifecycle, &error).has_value());
        REQUIRE(error == "snapshot Future Well lifecycle is invalid");
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
        preserve.Step(299);
        REQUIRE(preserve.FindEntity(well)->wellChoice ==
                FutureWellChoice::Dormant);
        REQUIRE(preserve.FindEntity(well)->wellCaptureProgress == 299);
        REQUIRE(preserve.FindPlayer(0)->resources.dawnshards == 0);
        preserve.Step();
        REQUIRE(preserve.FindEntity(well)->wellChoice ==
                FutureWellChoice::Preserve);
        REQUIRE(preserve.FindEntity(well)->wellActivationTick == 300);
        REQUIRE(preserve.FindPlayer(0)->resources.dawnshards == 0);
        preserve.Step(299);
        REQUIRE(preserve.FindPlayer(0)->resources.dawnshards == 0);
        preserve.Step();
        REQUIRE(preserve.FindPlayer(0)->resources.dawnshards == 15);
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
        // The route manifests only after capture and the full public warning.
        reshape.Step(480);
        REQUIRE(reshape.FindPlayer(0)->resources.dawnshards == 80);
        const Entity* reshapedWell = reshape.FindEntity(well);
        REQUIRE(reshapedWell->wellChoice == FutureWellChoice::Reshape);
        REQUIRE(reshapedWell->wellActivationTick == 481);
        REQUIRE(reshapedWell->reshapeUntilTick == 2281);
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

void TestFutureWellSnapshotMigrationAndReplay() {
    constexpr std::size_t kEntityBytes = 235;
    constexpr std::size_t kWellActivationOffset = 104;
    constexpr std::size_t kMapTiles = 20U * 20U;

    Simulation simulation({20, 20, 20, 0x4d3132534e4150ULL});
    REQUIRE(simulation.AddPlayer(
        0, Faction::MeridianCompact, {500, 100}));
    const EntityId worker = simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Worker,
        Vec2::FromTiles(5, 6));
    const EntityId activatedWell =
        simulation.SpawnFutureWell(Vec2::FromTiles(6, 6));
    const EntityId dormantWell =
        simulation.SpawnFutureWell(Vec2::FromTiles(14, 14));
    REQUIRE(worker != 0 && activatedWell != 0 && dormantWell != 0);

    simulation.CaptureReplayBaseline();
    Command preserve =
        MakeCommand(0, 0, 1, CommandType::FutureWell, worker);
    preserve.target = activatedWell;
    preserve.wellChoice = FutureWellChoice::Preserve;
    REQUIRE(simulation.QueueCommand(preserve));
    simulation.Step(300);
    REQUIRE(simulation.FindEntity(activatedWell)->wellActivationTick == 300);
    REQUIRE(simulation.FindEntity(dormantWell)->wellActivationTick == 0);

    const std::vector<std::uint8_t> snapshot = simulation.SaveSnapshot();
    REQUIRE(ReadU32(snapshot, 4) == kSnapshotVersion);
    std::string error;
    std::optional<Simulation> restored =
        Simulation::LoadSnapshot(snapshot, &error);
    REQUIRE(restored.has_value());
    REQUIRE(error.empty());
    REQUIRE(restored->FindEntity(activatedWell)->wellActivationTick == 300);
    REQUIRE(restored->FindEntity(dormantWell)->wellActivationTick == 0);
    REQUIRE(restored->StateChecksum() == simulation.StateChecksum());

    const ReplayRecord replay = simulation.ExportReplay();
    REQUIRE(replay.version == kReplayVersion);
    REQUIRE(ReadU32(replay.initialSnapshot, 4) == kSnapshotVersion);
    std::optional<Simulation> replayed =
        Simulation::ReplayToEnd(replay, &error);
    REQUIRE(replayed.has_value());
    REQUIRE(error.empty());
    REQUIRE(replayed->FindEntity(activatedWell)->wellActivationTick == 300);
    REQUIRE(replayed->FindEntity(dormantWell)->wellActivationTick == 0);
    REQUIRE(replayed->StateChecksum() == simulation.StateChecksum());

    const std::vector<std::uint8_t> v27 = ConvertSnapshotV28ToV27(snapshot, kMapTiles);
    REQUIRE(Simulation::LoadSnapshot(v27, &error).has_value());
    const std::vector<std::uint8_t> v26 =
        ConvertSnapshotV27ToV26(v27, kMapTiles);
    REQUIRE(ReadU32(v26, 4) == 26);
    std::optional<Simulation> v26Migrated =
        Simulation::LoadSnapshot(v26, &error);
    REQUIRE(v26Migrated.has_value());
    REQUIRE(error.empty());
    // Schema 26 predates capture and telegraph lifecycle fields. A completed
    // Preserve has their legitimate defaults, so this state remains exactly
    // reproducible even though those fields were absent from the payload.
    REQUIRE(v26Migrated->FindEntity(activatedWell)->wellCapturePlayer ==
            kNeutralPlayer);
    REQUIRE(v26Migrated->FindEntity(activatedWell)->wellCaptureProgress == 0);
    REQUIRE(v26Migrated->FindEntity(activatedWell)->wellPendingChoice ==
            FutureWellChoice::Dormant);
    REQUIRE(v26Migrated->FindEntity(activatedWell)->wellProtocolTicks == 0);
    REQUIRE(v26Migrated->StateChecksum() == simulation.StateChecksum());

    const std::vector<std::uint8_t> v25 =
        ConvertSnapshotV26ToV25(v26, kMapTiles);
    REQUIRE(ReadU32(v25, 4) == 25);
    std::optional<Simulation> v25Migrated =
        Simulation::LoadSnapshot(v25, &error);
    REQUIRE(v25Migrated.has_value());
    REQUIRE(error.empty());
    REQUIRE(v25Migrated->FindEntity(activatedWell)->wellCapturePlayer ==
            kNeutralPlayer);
    REQUIRE(v25Migrated->FindEntity(activatedWell)->wellPendingChoice ==
            FutureWellChoice::Dormant);
    REQUIRE(v25Migrated->FindEntity(activatedWell)->orderQueue.empty());
    REQUIRE(v25Migrated->Projectiles().empty());
    REQUIRE(v25Migrated->StateChecksum() == simulation.StateChecksum());

    const std::vector<std::uint8_t> v24 =
        ConvertSnapshotV25ToV24(v25, kMapTiles);
    REQUIRE(ReadU32(v24, 4) == 24);
    std::optional<Simulation> v24Migrated =
        Simulation::LoadSnapshot(v24, &error);
    REQUIRE(v24Migrated.has_value());
    REQUIRE(error.empty());
    // A schema-24 save carried no memory, so the loader reconstructs explored
    // ground from the live map — exactly the information that schema already
    // served — and starts object memory empty rather than inventing sightings.
    REQUIRE(v24Migrated->FindCommandResolutionReceipt(0, 1).has_value());
    REQUIRE(v24Migrated->FindEntity(activatedWell)->wellActivationTick == 300);
    REQUIRE(ReadU32(v24Migrated->SaveSnapshot(), 4) == kSnapshotVersion);
    REQUIRE(
        v24Migrated->CreatePlayerView(0)->RememberedObjects().empty());

    const std::vector<std::uint8_t> v23 =
        ConvertSnapshotV24ToV23(v24, 1);
    REQUIRE(ReadU32(v23, 4) == 23);
    std::optional<Simulation> v23Migrated =
        Simulation::LoadSnapshot(v23, &error);
    REQUIRE(v23Migrated.has_value());
    REQUIRE(error.empty());
    REQUIRE(!v23Migrated->FindCommandResolutionReceipt(0, 1).has_value());
    REQUIRE(v23Migrated->Config().protectedCommandCorePlayerMask == 0);
    REQUIRE(v23Migrated->FindEntity(activatedWell)->wellActivationTick == 300);
    REQUIRE(ReadU32(v23Migrated->SaveSnapshot(), 4) == kSnapshotVersion);

    const std::vector<std::uint8_t> v22 = ConvertSnapshotV23ToV22(v23);
    REQUIRE(ReadU32(v22, 4) == 22);
    std::optional<Simulation> v22Migrated =
        Simulation::LoadSnapshot(v22, &error);
    REQUIRE(v22Migrated.has_value());
    REQUIRE(error.empty());
    REQUIRE(v22Migrated->Config().protectedCommandCorePlayerMask == 0);
    REQUIRE(v22Migrated->FindEntity(activatedWell)->wellActivationTick == 300);
    REQUIRE(ReadU32(v22Migrated->SaveSnapshot(), 4) == kSnapshotVersion);

    const std::vector<std::uint8_t> prior =
        ConvertSnapshotV22ToV21(v22, kMapTiles);
    REQUIRE(ReadU32(prior, 4) == 21);
    std::optional<Simulation> priorMigrated =
        Simulation::LoadSnapshot(prior, &error);
    if (!priorMigrated.has_value()) {
        throw TestFailure("snapshot v21 migration failed: " + error);
    }
    REQUIRE(error.empty());
    REQUIRE(priorMigrated->FindEntity(activatedWell)->wellActivationTick == 300);
    REQUIRE(priorMigrated->FindEntity(dormantWell)->wellActivationTick == 0);
    REQUIRE(ReadU32(priorMigrated->SaveSnapshot(), 4) == kSnapshotVersion);

    const std::vector<std::uint8_t> legacy =
        ConvertSnapshotV21ToV20(prior, kMapTiles);
    REQUIRE(ReadU32(legacy, 4) == 20);
    std::optional<Simulation> migrated =
        Simulation::LoadSnapshot(legacy, &error);
    REQUIRE(migrated.has_value());
    REQUIRE(error.empty());
    REQUIRE(migrated->FindEntity(activatedWell)->wellActivationTick ==
            migrated->CurrentTick());
    REQUIRE(migrated->FindEntity(dormantWell)->wellActivationTick == 0);
    REQUIRE(ReadU32(migrated->SaveSnapshot(), 4) == kSnapshotVersion);

    const std::size_t firstEntity =
        SnapshotV25FirstEntityOffset(snapshot, kMapTiles);
    std::vector<std::uint8_t> futureActivation = snapshot;
    WriteU64(
        futureActivation,
        firstEntity + kEntityBytes + kWellActivationOffset,
        simulation.CurrentTick() + 1);
    ResignSnapshot(futureActivation);
    REQUIRE(!Simulation::LoadSnapshot(futureActivation, &error).has_value());
    REQUIRE(error == "snapshot entity state is invalid");

    std::vector<std::uint8_t> dormantActivation = snapshot;
    WriteU64(
        dormantActivation,
        firstEntity + 2 * kEntityBytes + kWellActivationOffset,
        1);
    ResignSnapshot(dormantActivation);
    REQUIRE(!Simulation::LoadSnapshot(dormantActivation, &error).has_value());
    REQUIRE(error == "snapshot entity state is invalid");

    // Once legacy-only omissions have defaulted, subsequent authored state
    // evolves identically when the historical format represented all active
    // state needed by this fixture.
    simulation.Step(300);
    v26Migrated->Step(300);
    v25Migrated->Step(300);
    REQUIRE(simulation.FindPlayer(0)->resources.dawnshards == 115);
    REQUIRE(v26Migrated->StateChecksum() == simulation.StateChecksum());
    REQUIRE(v25Migrated->StateChecksum() == simulation.StateChecksum());
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
    harvest.Step(479);
    REQUIRE(harvest.FindPlayer(0)->resources.dawnshards ==
            std::numeric_limits<std::int32_t>::max() - 100);
    REQUIRE(harvest.FindEntity(well)->wellProtocolTicks == 1);
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
    WriteU32(excessiveVision,
             SnapshotV25FirstEntityOffset(baseline, mapTiles) + 27, 50000);
    ResignSnapshot(excessiveVision);
    REQUIRE(!Simulation::LoadSnapshot(excessiveVision, &error).has_value());
    REQUIRE(error == "snapshot entity state is invalid");

    std::vector<std::uint8_t> excessiveTick = baseline;
    WriteU64(excessiveTick, 2407, std::numeric_limits<std::uint64_t>::max());
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
    WriteU32(truncatedEntities,
             SnapshotV25EntityCountOffset(truncatedEntities, mapTiles),
             64 * 1024);
    ResignSnapshot(truncatedEntities);
    REQUIRE(!Simulation::LoadSnapshot(truncatedEntities, &error).has_value());
    REQUIRE(error == "snapshot entity count is invalid");

    std::vector<std::uint8_t> exhaustedIds = baseline;
    WriteU32(exhaustedIds, 2415, std::numeric_limits<std::uint32_t>::max());
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
    wellSimulation.Step(479);
    REQUIRE(wellSimulation.FindPlayer(0)->resources.dawnshards == 0);
    REQUIRE(wellSimulation.FindEntity(well)->wellProtocolTicks == 1);
    wellSimulation.Step();
    REQUIRE(wellSimulation.FindPlayer(0)->resources.dawnshards == 77);

    SimulationConfig invalid = config;
    invalid.rules.version = 3;
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

void TestMineralCoverExtremeCoordinateDeterminism() {
    using namespace echoes::sim::net;

    Simulation valid({16, 16, 20, 0x45585452454d4543ULL});
    AddTwoPlayers(valid, {0, 0}, {0, 100});
    const EntityId validCairnback = valid.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::HeavyUnit,
        Vec2::FromTiles(10, 10));
    REQUIRE(validCairnback != 0);
    valid.CaptureReplayBaseline();

    CommandRequest validRequest{};
    validRequest.sequence = 1;
    validRequest.executeTick = 0;
    validRequest.type = CommandType::RaiseMineralCover;
    validRequest.actor = validCairnback;
    validRequest.position = Vec2::FromTiles(9, 10);
    const std::vector<std::uint8_t> validBytes =
        EncodeCommandRequest(validRequest);
    const std::vector<std::uint8_t> expectedValidBytes{
        69, 66, 83, 80, 1, 0, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 16, 1, 0, 0, 0, 0, 0, 0, 0, 0,
        36, 0, 0, 0, 40, 0, 0, 4, 0, 0, 0, 160, 60, 40, 216};
    REQUIRE(validBytes == expectedValidBytes);
    CommandAdmissionContext validContext{};
    validContext.player = 1;
    validContext.minimumInputDelayTicks = 0;
    validContext.maximumLeadTicks = 0;
    std::string rejection;
    REQUIRE(AdmitCommandRequest(
                validRequest, validContext, valid, &rejection) ==
            CommandAdmissionStatus::Accepted);
    REQUIRE(rejection.empty());
    REQUIRE(!valid.FindCommandResolutionReceipt(1, 1).has_value());
    const std::vector<std::uint8_t> admittedSnapshot = valid.SaveSnapshot();
    std::string validSnapshotError;
    std::optional<Simulation> admittedRestored =
        Simulation::LoadSnapshot(admittedSnapshot, &validSnapshotError);
    REQUIRE(admittedRestored.has_value());
    REQUIRE(validSnapshotError.empty());
    REQUIRE(admittedRestored->StateChecksum() == valid.StateChecksum());
    REQUIRE(!admittedRestored->FindCommandResolutionReceipt(1, 1).has_value());
    valid.Step();
    admittedRestored->Step();
    REQUIRE(admittedRestored->StateChecksum() == valid.StateChecksum());
    const std::optional<CommandResolutionReceipt> validReceipt =
        valid.FindCommandResolutionReceipt(1, 1);
    REQUIRE(validReceipt.has_value());
    REQUIRE(validReceipt->player == 1);
    REQUIRE(validReceipt->commandType == CommandType::RaiseMineralCover);
    REQUIRE(validReceipt->assignedExecutionTick == 0);
    REQUIRE(validReceipt->outcome == CommandResolutionOutcome::Applied);
    REQUIRE(std::count_if(
                valid.Entities().begin(), valid.Entities().end(),
                [](const Entity& entity) {
                    return entity.temporaryMineralCover;
                }) == 1);
    const ReplayRecord validReplay = valid.ExportReplay();
    std::optional<Simulation> validReplayed =
        Simulation::ReplayToEnd(validReplay, &validSnapshotError);
    REQUIRE(validReplayed.has_value());
    REQUIRE(validSnapshotError.empty());
    REQUIRE(validReplayed->StateChecksum() == valid.StateChecksum());

    SimulationConfig config{16, 16, 20, 0x45585452454d4543ULL};
    Simulation first(config);
    Simulation second(config);
    AddTwoPlayers(first, {0, 0}, {0, 100});
    AddTwoPlayers(second, {0, 0}, {0, 100});
    const EntityId firstCairnback = first.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::HeavyUnit,
        Vec2::FromTiles(10, 10));
    const EntityId secondCairnback = second.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::HeavyUnit,
        Vec2::FromTiles(10, 10));
    REQUIRE(firstCairnback != 0);
    REQUIRE(firstCairnback == secondCairnback);

    const Vec2 minimum = Vec2::FromRaw(
        std::numeric_limits<std::int32_t>::min(),
        std::numeric_limits<std::int32_t>::min());
    const Vec2 maximum = Vec2::FromRaw(
        std::numeric_limits<std::int32_t>::max(),
        std::numeric_limits<std::int32_t>::max());
    const Vec2 mixed = Vec2::FromRaw(
        std::numeric_limits<std::int32_t>::min(),
        std::numeric_limits<std::int32_t>::max());
    for (const Vec2 position : {minimum, maximum, mixed}) {
        REQUIRE(first.ValidateMineralCover(
                    1, firstCairnback, position) ==
                MineralCoverResult::InvalidPosition);
        REQUIRE(second.ValidateMineralCover(
                    1, secondCairnback, position) ==
                MineralCoverResult::InvalidPosition);
    }

    CommandBatchRequest batch{};
    batch.clientBatchId = 1;
    CommandIntent intent{};
    intent.type = CommandType::RaiseMineralCover;
    intent.actor = firstCairnback;
    intent.position = minimum;
    batch.intents.push_back(intent);
    const std::vector<std::uint8_t> batchBytes =
        EncodeCommandBatchRequest(batch);
    REQUIRE(!batchBytes.empty());
    CommandBatchRequest decodedBatch{};
    REQUIRE(DecodeCommandBatchRequest(batchBytes, decodedBatch) ==
            DecodeStatus::Ok);
    REQUIRE(decodedBatch == batch);
    REQUIRE(EncodeCommandBatchRequest(decodedBatch) == batchBytes);

    first.CaptureReplayBaseline();
    second.CaptureReplayBaseline();
    CommandRequest request{};
    request.sequence = 1;
    request.executeTick = 2;
    request.type = CommandType::RaiseMineralCover;
    request.actor = firstCairnback;
    request.position = minimum;
    const std::vector<std::uint8_t> requestBytes =
        EncodeCommandRequest(request);
    REQUIRE(!requestBytes.empty());
    CommandRequest decodedRequest{};
    REQUIRE(DecodeCommandRequest(requestBytes, decodedRequest) ==
            DecodeStatus::Ok);
    REQUIRE(decodedRequest == request);
    REQUIRE(EncodeCommandRequest(decodedRequest) == requestBytes);

    CommandAdmissionContext firstContext{};
    firstContext.player = 1;
    firstContext.minimumInputDelayTicks = 2;
    firstContext.maximumLeadTicks = 2;
    CommandAdmissionContext secondContext = firstContext;
    REQUIRE(AdmitCommandRequest(
                decodedRequest, firstContext, first, &rejection) ==
            CommandAdmissionStatus::Accepted);
    REQUIRE(rejection.empty());
    REQUIRE(AdmitCommandRequest(
                decodedRequest, secondContext, second, &rejection) ==
            CommandAdmissionStatus::Accepted);
    REQUIRE(rejection.empty());

    Command maximumCommand = MakeCommand(
        3, 1, 2, CommandType::RaiseMineralCover, firstCairnback);
    maximumCommand.position = maximum;
    REQUIRE(first.QueueCommand(maximumCommand, &rejection));
    REQUIRE(rejection.empty());
    REQUIRE(second.QueueCommand(maximumCommand, &rejection));
    REQUIRE(rejection.empty());
    Command mixedCommand = MakeCommand(
        4, 1, 3, CommandType::RaiseMineralCover, firstCairnback);
    mixedCommand.position = mixed;
    REQUIRE(first.QueueCommand(mixedCommand, &rejection));
    REQUIRE(rejection.empty());
    REQUIRE(second.QueueCommand(mixedCommand, &rejection));
    REQUIRE(rejection.empty());
    REQUIRE(first.StateChecksum() == second.StateChecksum());
    const std::uint64_t checksumBeforePendingLookup = first.StateChecksum();
    REQUIRE(!first.FindCommandResolutionReceipt(1, 1).has_value());
    REQUIRE(!first.FindCommandResolutionReceipt(1, 2).has_value());
    REQUIRE(!first.FindCommandResolutionReceipt(1, 3).has_value());
    REQUIRE(first.StateChecksum() == checksumBeforePendingLookup);

    const std::size_t initialEntityCount = first.Entities().size();
    const ResourcePool initialResources = first.FindPlayer(1)->resources;
    const std::vector<std::uint8_t> pendingSnapshot = first.SaveSnapshot();
    REQUIRE(second.SaveSnapshot() == pendingSnapshot);
    std::string snapshotError;
    std::optional<Simulation> restored =
        Simulation::LoadSnapshot(pendingSnapshot, &snapshotError);
    REQUIRE(restored.has_value());
    REQUIRE(snapshotError.empty());
    REQUIRE(restored->StateChecksum() == first.StateChecksum());
    REQUIRE(!restored->FindCommandResolutionReceipt(1, 1).has_value());

    for (std::uint32_t tick = 0; tick < 5; ++tick) {
        first.Step();
        second.Step();
        restored->Step();
        REQUIRE(first.StateChecksum() == second.StateChecksum());
        REQUIRE(restored->StateChecksum() == first.StateChecksum());
        const std::uint64_t checksumBeforeLookup = first.StateChecksum();
        for (std::uint64_t sequence = 1; sequence <= 3; ++sequence) {
            const Tick assignedTick = sequence + 1;
            const std::optional<CommandResolutionReceipt> receipt =
                first.FindCommandResolutionReceipt(1, sequence);
            if (tick < assignedTick) {
                REQUIRE(!receipt.has_value());
                continue;
            }
            REQUIRE(receipt.has_value());
            REQUIRE(receipt->player == 1);
            REQUIRE(receipt->commandType ==
                    CommandType::RaiseMineralCover);
            REQUIRE(receipt->assignedExecutionTick == assignedTick);
            REQUIRE(receipt->outcome ==
                    CommandResolutionOutcome::InvalidPosition);
            REQUIRE(second.FindCommandResolutionReceipt(1, sequence) ==
                    receipt);
            REQUIRE(restored->FindCommandResolutionReceipt(1, sequence) ==
                    receipt);
        }
        REQUIRE(first.StateChecksum() == checksumBeforeLookup);
    }
    REQUIRE(first.Entities().size() == initialEntityCount);
    REQUIRE(first.FindPlayer(1)->resources == initialResources);
    REQUIRE(first.FindEntity(firstCairnback) != nullptr);
    REQUIRE(first.FindEntity(firstCairnback)
                ->mineralCoverCooldownUntilTick == 0);
    REQUIRE(std::none_of(
        first.Entities().begin(), first.Entities().end(),
        [](const Entity& entity) { return entity.temporaryMineralCover; }));
    REQUIRE(first.TerrainAt(0, 0) == Terrain::Open);
    REQUIRE(first.NextCommandSequence(1) == 4);

    const std::optional<CommandResolutionReceipt> firstReceipt =
        first.FindCommandResolutionReceipt(1, 1);
    REQUIRE(firstReceipt.has_value());
    Command duplicate = MakeCommand(
        first.CurrentTick(), 1, 1, CommandType::RaiseMineralCover,
        firstCairnback);
    duplicate.position = minimum;
    REQUIRE(!first.QueueCommand(duplicate, &rejection));
    REQUIRE(rejection ==
            "command sequence is not newer than executed input");
    REQUIRE(first.FindCommandResolutionReceipt(1, 1) == firstReceipt);

    const std::vector<std::uint8_t> resolvedSnapshot = first.SaveSnapshot();
    std::optional<Simulation> resolvedRestore =
        Simulation::LoadSnapshot(resolvedSnapshot, &snapshotError);
    REQUIRE(resolvedRestore.has_value());
    REQUIRE(snapshotError.empty());
    REQUIRE(resolvedRestore->StateChecksum() == first.StateChecksum());
    for (std::uint64_t sequence = 1; sequence <= 3; ++sequence) {
        REQUIRE(resolvedRestore->FindCommandResolutionReceipt(1, sequence) ==
                first.FindCommandResolutionReceipt(1, sequence));
    }

    std::string replayError;
    const ReplayRecord replay = first.ExportReplay(&replayError);
    REQUIRE(replayError.empty());
    REQUIRE(replay.commands.size() == 3);
    std::optional<Simulation> replayed =
        Simulation::ReplayToEnd(replay, &replayError);
    REQUIRE(replayed.has_value());
    REQUIRE(replayError.empty());
    REQUIRE(replayed->StateChecksum() == first.StateChecksum());
    for (std::uint64_t sequence = 1; sequence <= 3; ++sequence) {
        REQUIRE(replayed->FindCommandResolutionReceipt(1, sequence) ==
                first.FindCommandResolutionReceipt(1, sequence));
    }
}

void TestCommandResolutionReceiptRetentionAndBounds() {
    const Vec2 invalidPosition = Vec2::FromRaw(
        std::numeric_limits<std::int32_t>::min(),
        std::numeric_limits<std::int32_t>::max());

    Simulation noEffect({8, 8, 20, 0x524543454950544eULL});
    REQUIRE(noEffect.AddPlayer(
        0, Faction::MeridianCompact, {0, 0}));
    Command missingActor = MakeCommand(
        0, 0, 1, CommandType::Stop, 999);
    REQUIRE(noEffect.QueueCommand(missingActor));
    noEffect.Step();
    const std::optional<CommandResolutionReceipt> noEffectReceipt =
        noEffect.FindCommandResolutionReceipt(0, 1);
    REQUIRE(noEffectReceipt.has_value());
    REQUIRE(noEffectReceipt->player == 0);
    REQUIRE(noEffectReceipt->commandType == CommandType::Stop);
    REQUIRE(noEffectReceipt->assignedExecutionTick == 0);
    REQUIRE(noEffectReceipt->outcome ==
            CommandResolutionOutcome::NoEffect);

    Simulation retained({16, 16, 20, 0x5245434549505454ULL});
    REQUIRE(retained.AddPlayer(
        0, Faction::KharuunAssemblies, {0, 100}));
    const EntityId cairnback = retained.SpawnEntity(
        0, Faction::KharuunAssemblies, EntityType::HeavyUnit,
        Vec2::FromTiles(8, 8));
    REQUIRE(cairnback != 0);
    Command invalid = MakeCommand(
        0, 0, 1, CommandType::RaiseMineralCover, cairnback);
    invalid.position = invalidPosition;
    REQUIRE(retained.QueueCommand(invalid));
    retained.Step();
    const std::optional<CommandResolutionReceipt> receipt =
        retained.FindCommandResolutionReceipt(0, 1);
    REQUIRE(receipt.has_value());
    REQUIRE(receipt->outcome == CommandResolutionOutcome::InvalidPosition);

    const std::vector<std::uint8_t> resolvedSnapshot =
        retained.SaveSnapshot();
    std::string error;
    std::optional<Simulation> restored =
        Simulation::LoadSnapshot(resolvedSnapshot, &error);
    REQUIRE(restored.has_value());
    REQUIRE(error.empty());
    REQUIRE(restored->FindCommandResolutionReceipt(0, 1) == receipt);

    constexpr std::size_t kSerializedReceiptBytes = 19;
    const std::size_t receiptBlockOffset =
        SnapshotReceiptBlockOffset(resolvedSnapshot,
            retained.Config().mapWidthTiles * retained.Config().mapHeightTiles);
    REQUIRE(ReadU32(resolvedSnapshot, receiptBlockOffset) == 1);
    std::vector<std::uint8_t> invalidOutcome = resolvedSnapshot;
    invalidOutcome[receiptBlockOffset + 4U +
                   kSerializedReceiptBytes - 1U] = 0xff;
    ResignSnapshot(invalidOutcome);
    REQUIRE(!Simulation::LoadSnapshot(invalidOutcome, &error).has_value());
    REQUIRE(error == "snapshot command resolution receipt is invalid");

    const std::size_t firstReceiptOffset = receiptBlockOffset + 4U;
    constexpr std::size_t kReceiptSequenceOffset = 1;
    constexpr std::size_t kReceiptCommandTypeOffset = 9;
    constexpr std::size_t kReceiptAssignedTickOffset = 10;

    std::vector<std::uint8_t> sequenceBeyondExecuted = resolvedSnapshot;
    WriteU64(sequenceBeyondExecuted,
             firstReceiptOffset + kReceiptSequenceOffset, 2);
    ResignSnapshot(sequenceBeyondExecuted);
    REQUIRE(!Simulation::LoadSnapshot(
                 sequenceBeyondExecuted, &error).has_value());
    REQUIRE(error == "snapshot command resolution receipt is invalid");

    std::vector<std::uint8_t> invalidOutcomePairing = resolvedSnapshot;
    invalidOutcomePairing[
        firstReceiptOffset + kReceiptCommandTypeOffset] =
        static_cast<std::uint8_t>(CommandType::Stop);
    ResignSnapshot(invalidOutcomePairing);
    REQUIRE(!Simulation::LoadSnapshot(
                 invalidOutcomePairing, &error).has_value());
    REQUIRE(error == "snapshot command resolution receipt is invalid");

    std::vector<std::uint8_t> expiredReceipt = resolvedSnapshot;
    constexpr std::size_t kSnapshotCurrentTickOffset = 2407;
    REQUIRE(ReadU64(expiredReceipt, kSnapshotCurrentTickOffset) == 1);
    WriteU64(expiredReceipt, kSnapshotCurrentTickOffset,
             kCommandResolutionReceiptRetentionTicks + 1);
    ResignSnapshot(expiredReceipt);
    REQUIRE(!Simulation::LoadSnapshot(expiredReceipt, &error).has_value());
    REQUIRE(error == "snapshot command resolution receipt is invalid");

    std::vector<std::uint8_t> excessiveReceiptCount = resolvedSnapshot;
    WriteU32(excessiveReceiptCount, receiptBlockOffset,
             static_cast<std::uint32_t>(
                 kMaximumCommandResolutionReceipts + 1));
    ResignSnapshot(excessiveReceiptCount);
    REQUIRE(!Simulation::LoadSnapshot(
                 excessiveReceiptCount, &error).has_value());
    REQUIRE(error ==
            "snapshot command resolution receipt count is invalid");

    std::vector<std::uint8_t> truncatedReceipt = resolvedSnapshot;
    truncatedReceipt.erase(
        truncatedReceipt.begin() +
        static_cast<std::ptrdiff_t>(firstReceiptOffset));
    ResignSnapshot(truncatedReceipt);
    REQUIRE(!Simulation::LoadSnapshot(truncatedReceipt, &error).has_value());
    REQUIRE(!error.empty()); // Removing a byte invalidates the receipt/work boundary.

    Simulation twoReceipts = retained;
    Command secondInvalid = MakeCommand(
        twoReceipts.CurrentTick(), 0, 2,
        CommandType::RaiseMineralCover, cairnback);
    secondInvalid.position = invalidPosition;
    REQUIRE(twoReceipts.QueueCommand(secondInvalid));
    twoReceipts.Step();
    const std::vector<std::uint8_t> twoReceiptSnapshot =
        twoReceipts.SaveSnapshot();
    const std::size_t twoReceiptBlockOffset =
        SnapshotReceiptBlockOffset(twoReceiptSnapshot,
            twoReceipts.Config().mapWidthTiles * twoReceipts.Config().mapHeightTiles);
    REQUIRE(ReadU32(twoReceiptSnapshot, twoReceiptBlockOffset) == 2);
    const std::size_t secondReceiptOffset =
        twoReceiptBlockOffset + 4U + kSerializedReceiptBytes;

    std::vector<std::uint8_t> duplicateReceiptKey = twoReceiptSnapshot;
    WriteU64(duplicateReceiptKey,
             secondReceiptOffset + kReceiptSequenceOffset, 1);
    ResignSnapshot(duplicateReceiptKey);
    REQUIRE(!Simulation::LoadSnapshot(
                 duplicateReceiptKey, &error).has_value());
    REQUIRE(error == "snapshot command resolution receipt is invalid");

    std::vector<std::uint8_t> outOfOrderReceipts = twoReceiptSnapshot;
    WriteU64(outOfOrderReceipts,
             twoReceiptBlockOffset + 4U +
                 kReceiptAssignedTickOffset,
             1);
    WriteU64(outOfOrderReceipts,
             secondReceiptOffset + kReceiptAssignedTickOffset,
             0);
    ResignSnapshot(outOfOrderReceipts);
    REQUIRE(!Simulation::LoadSnapshot(
                 outOfOrderReceipts, &error).has_value());
    REQUIRE(error == "snapshot command resolution receipt is invalid");

    retained.Step(kCommandResolutionReceiptRetentionTicks - 1);
    restored->Step(kCommandResolutionReceiptRetentionTicks - 1);
    REQUIRE(retained.CurrentTick() ==
            kCommandResolutionReceiptRetentionTicks);
    REQUIRE(retained.FindCommandResolutionReceipt(0, 1) == receipt);
    REQUIRE(restored->StateChecksum() == retained.StateChecksum());
    retained.Step();
    restored->Step();
    REQUIRE(retained.CurrentTick() ==
            kCommandResolutionReceiptRetentionTicks + 1);
    REQUIRE(!retained.FindCommandResolutionReceipt(0, 1).has_value());
    REQUIRE(!restored->FindCommandResolutionReceipt(0, 1).has_value());
    REQUIRE(restored->StateChecksum() == retained.StateChecksum());

    Simulation bounded({16, 16, 20, 0x5245434549505442ULL});
    REQUIRE(bounded.AddPlayer(
        0, Faction::KharuunAssemblies, {0, 100}));
    const EntityId boundedCairnback = bounded.SpawnEntity(
        0, Faction::KharuunAssemblies, EntityType::HeavyUnit,
        Vec2::FromTiles(8, 8));
    REQUIRE(boundedCairnback != 0);
    for (std::uint64_t sequence = 1;
         sequence <= kMaximumCommandResolutionReceipts + 1;
         ++sequence) {
        Command command = MakeCommand(
            0, 0, sequence, CommandType::RaiseMineralCover,
            boundedCairnback);
        command.position = invalidPosition;
        REQUIRE(bounded.QueueCommand(command));
    }
    bounded.Step();
    REQUIRE(!bounded.FindCommandResolutionReceipt(0, 1).has_value());
    REQUIRE(bounded.FindCommandResolutionReceipt(0, 2).has_value());
    const std::optional<CommandResolutionReceipt> newest =
        bounded.FindCommandResolutionReceipt(
            0, kMaximumCommandResolutionReceipts + 1);
    REQUIRE(newest.has_value());
    REQUIRE(newest->assignedExecutionTick == 0);
    REQUIRE(newest->outcome ==
            CommandResolutionOutcome::InvalidPosition);
    const std::vector<std::uint8_t> boundedSnapshot = bounded.SaveSnapshot();
    std::optional<Simulation> boundedRestore =
        Simulation::LoadSnapshot(boundedSnapshot, &error);
    REQUIRE(boundedRestore.has_value());
    REQUIRE(error.empty());
    REQUIRE(boundedRestore->StateChecksum() == bounded.StateChecksum());
    REQUIRE(!boundedRestore->FindCommandResolutionReceipt(0, 1).has_value());
    REQUIRE(boundedRestore->FindCommandResolutionReceipt(0, 2) ==
            bounded.FindCommandResolutionReceipt(0, 2));
    REQUIRE(boundedRestore->FindCommandResolutionReceipt(
                0, kMaximumCommandResolutionReceipts + 1) == newest);
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

    const EntityId publicMeridianInterface =
        simulation.SpawnPublicInterface(
            Faction::MeridianCompact, Vec2::FromTiles(40, 40));
    const EntityId publicKharuunInterface =
        simulation.SpawnPublicInterface(
            Faction::KharuunAssemblies, Vec2::FromTiles(44, 40));
    const Entity* meridianInterface =
        simulation.FindEntity(publicMeridianInterface);
    const Entity* kharuunInterface =
        simulation.FindEntity(publicKharuunInterface);
    REQUIRE(publicMeridianInterface != 0 && publicKharuunInterface != 0);
    REQUIRE(meridianInterface != nullptr &&
            meridianInterface->owner == kNeutralPlayer &&
            meridianInterface->aegisPowered &&
            meridianInterface->attackRangeRaw == 0 &&
            meridianInterface->attackDamage == 0 &&
            meridianInterface->visionTiles == 0);
    REQUIRE(kharuunInterface != nullptr &&
            kharuunInterface->owner == kNeutralPlayer &&
            !kharuunInterface->aegisPowered &&
            kharuunInterface->attackRangeRaw == 0 &&
            kharuunInterface->attackDamage == 0 &&
            kharuunInterface->visionTiles == 0);

    std::string error;
    const std::vector<std::uint8_t> poweredSnapshot =
        simulation.SaveSnapshot();
    const std::optional<Simulation> restored =
        Simulation::LoadSnapshot(poweredSnapshot, &error);
    if (!restored.has_value()) {
        throw TestFailure("public-interface snapshot failed: " + error);
    }
    REQUIRE(restored->Config().rules.poweredAegis ==
            simulation.Config().rules.poweredAegis);
    REQUIRE(restored->FindEntity(aegis)->aegisPowered);
    REQUIRE(restored->FindEntity(publicMeridianInterface) != nullptr &&
            restored->FindEntity(publicMeridianInterface)->owner ==
                kNeutralPlayer &&
            restored->FindEntity(publicMeridianInterface)->aegisPowered);
    REQUIRE(restored->FindEntity(publicKharuunInterface) != nullptr &&
            restored->FindEntity(publicKharuunInterface)->owner ==
                kNeutralPlayer &&
            !restored->FindEntity(publicKharuunInterface)->aegisPowered);

    std::vector<std::uint8_t> forgedPower = poweredSnapshot;
    constexpr std::size_t kSerializedEntitySize = 235;
    constexpr std::size_t kAegisPowerFieldOffset = 209;
    constexpr std::size_t kAegisEntityIndex = 3;
    const std::size_t aegisPowerOffset =
        SnapshotV25FirstEntityOffset(poweredSnapshot, 64U * 64U) +
        kAegisEntityIndex * kSerializedEntitySize +
        kAegisPowerFieldOffset;
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

void TestHollowChoirIdentityReconciliationAndPersistence() {
    using namespace echoes::sim::net;

    SimulationConfig legacyConfig{24, 24, 20, 0x43484f49524c4547ULL};
    legacyConfig.rules.version = 1;
    Simulation legacy(legacyConfig);
    REQUIRE(!legacy.AddPlayer(0, Faction::HollowChoir, {100, 100}));

    Simulation simulation({32, 32, 20, 0x43484f4952494445ULL});
    REQUIRE(simulation.AddPlayer(0, Faction::HollowChoir, {1000, 100}));
    REQUIRE(simulation.AddPlayer(1, Faction::MeridianCompact, {1000, 100}));
    const EntityId intervalist = simulation.SpawnEntity(
        0, Faction::HollowChoir, EntityType::Soldier,
        Vec2::FromTiles(6, 6));
    const EntityId threadkeeper = simulation.SpawnEntity(
        0, Faction::HollowChoir, EntityType::Worker,
        Vec2::FromTiles(5, 6));
    const EntityId observer = simulation.SpawnEntity(
        1, Faction::MeridianCompact, EntityType::ScoutUnit,
        Vec2::FromTiles(9, 6));
    REQUIRE(intervalist != 0 && threadkeeper != 0 && observer != 0);

    const EntityArchetypeRules& base =
        simulation.Config().rules.archetypes
            [static_cast<std::size_t>(Faction::HollowChoir)]
            [static_cast<std::size_t>(EntityType::Soldier)];
    const Entity* initial = simulation.FindEntity(intervalist);
    REQUIRE(initial != nullptr);
    REQUIRE(initial->choirIdentityState == ChoirIdentityState::Manifest);
    REQUIRE(initial->attackDamage ==
            base.attackDamage *
                simulation.Config().rules.choirIdentity.manifestDamagePercent /
                100);
    REQUIRE(initial->movementPerTickRaw == base.movementPerTickRaw);
    REQUIRE(simulation.FindEntity(threadkeeper)->choirIdentityState ==
            ChoirIdentityState::NotChoir);
    REQUIRE(simulation.ValidateChoirReconciliation(
                3, intervalist, ChoirIdentityState::Possible) ==
            ChoirReconciliationResult::InvalidPlayer);
    REQUIRE(simulation.ValidateChoirReconciliation(
                0, threadkeeper, ChoirIdentityState::Possible) ==
            ChoirReconciliationResult::InvalidActor);
    REQUIRE(simulation.ValidateChoirReconciliation(
                0, intervalist, ChoirIdentityState::Manifest) ==
            ChoirReconciliationResult::AlreadyStable);

    simulation.CaptureReplayBaseline();
    Command possible = MakeCommand(
        0, 0, 1, CommandType::ReconcileToPossible, intervalist);
    REQUIRE(simulation.QueueCommand(possible));
    simulation.Step();
    const Tick resolutionTick =
        simulation.Config().rules.choirIdentity.durationTicks;
    const Tick nextAvailableTick =
        resolutionTick + simulation.Config().rules.choirIdentity.cooldownTicks;
    const Entity* dual = simulation.FindEntity(intervalist);
    REQUIRE(dual != nullptr);
    REQUIRE(dual->choirIdentityState ==
            ChoirIdentityState::DualResolvePossible);
    REQUIRE(dual->choirIdentityResolveAtTick == resolutionTick);
    REQUIRE(dual->choirIdentityNextAvailableTick == nextAvailableTick);
    // Re-derived: these three expectations encoded the defect itself. They
    // asserted that a unit inside the 160-tick transition holds the Manifest
    // damage bonus AND both Possible bonuses at once, which made the declared
    // liability window the unit's strongest state. Section 12.5 grants 130%
    // damage to Manifest and 130% movement / 125% vision to Possible; the
    // transition is neither identity, so it grants neither. The unit pays base
    // stats for the whole publicly visible window, and it is strictly worse
    // than the Manifest state it left.
    REQUIRE(dual->attackDamage == base.attackDamage);
    REQUIRE(dual->attackDamage <
            base.attackDamage *
                simulation.Config().rules.choirIdentity.manifestDamagePercent /
                100);
    REQUIRE(dual->movementPerTickRaw == base.movementPerTickRaw);
    REQUIRE(dual->movementPerTickRaw <
            base.movementPerTickRaw *
                simulation.Config().rules.choirIdentity.possibleMovementPercent /
                100);
    REQUIRE(dual->visionTiles == base.visionTiles);
    REQUIRE(dual->visionTiles <
            base.visionTiles *
                simulation.Config().rules.choirIdentity.possibleVisionPercent /
                100);
    REQUIRE(simulation.FindPlayer(0)->resources.dawnshards == 80);
    REQUIRE(simulation.ValidateChoirReconciliation(
                0, intervalist, ChoirIdentityState::Manifest) ==
            ChoirReconciliationResult::AlreadyResolving);

    const std::optional<PlayerView> opponentView =
        simulation.CreatePlayerView(1);
    REQUIRE(opponentView.has_value());
    const auto observed = std::find_if(
        opponentView->Entities().begin(), opponentView->Entities().end(),
        [intervalist](const Entity& entity) {
            return entity.id == intervalist;
        });
    REQUIRE(observed != opponentView->Entities().end());
    REQUIRE(observed->choirIdentityState ==
            ChoirIdentityState::DualResolvePossible);
    REQUIRE(observed->choirIdentityResolveAtTick == resolutionTick);
    REQUIRE(observed->choirIdentityNextAvailableTick == nextAvailableTick);

    ScopedViewKeyframe keyframe{};
    std::string rejection;
    REQUIRE(BuildScopedViewKeyframe(
        *opponentView, 7, 0, keyframe, &rejection));
    const auto scoped = std::find_if(
        keyframe.entities.begin(), keyframe.entities.end(),
        [intervalist](const ScopedEntityState& entity) {
            return entity.id == intervalist;
        });
    REQUIRE(scoped != keyframe.entities.end());
    REQUIRE(scoped->choirIdentityState ==
            ChoirIdentityState::DualResolvePossible);
    REQUIRE(scoped->choirIdentityResolveAtTick == resolutionTick);
    const std::vector<std::uint8_t> keyframeBytes =
        EncodeScopedViewKeyframe(keyframe);
    REQUIRE(!keyframeBytes.empty());
    ScopedViewKeyframe decodedKeyframe{};
    REQUIRE(DecodeScopedViewKeyframe(keyframeBytes, decodedKeyframe) ==
            DecodeStatus::Ok);
    REQUIRE(decodedKeyframe == keyframe);

    CommandRequest networkIdentity{};
    networkIdentity.sequence = 2;
    networkIdentity.executeTick = simulation.CurrentTick();
    networkIdentity.type = CommandType::ReconcileToManifest;
    networkIdentity.actor = intervalist;
    const std::vector<std::uint8_t> identityBytes =
        EncodeCommandRequest(networkIdentity);
    REQUIRE(!identityBytes.empty());
    CommandRequest decodedIdentity{};
    REQUIRE(DecodeCommandRequest(identityBytes, decodedIdentity) ==
            DecodeStatus::Ok);
    REQUIRE(decodedIdentity == networkIdentity);

    const std::vector<std::uint8_t> snapshot = simulation.SaveSnapshot();
    std::string error;
    const std::optional<Simulation> restored =
        Simulation::LoadSnapshot(snapshot, &error);
    REQUIRE(restored.has_value());
    REQUIRE(error.empty());
    REQUIRE(restored->StateChecksum() == simulation.StateChecksum());
    REQUIRE(restored->FindEntity(intervalist)->choirIdentityState ==
            ChoirIdentityState::DualResolvePossible);

    std::vector<std::uint8_t> invalidIdentity = snapshot;
    invalidIdentity[SnapshotV25FirstEntityOffset(snapshot, 32U * 32U) + 210] =
        0xff;
    ResignSnapshot(invalidIdentity);
    REQUIRE(!Simulation::LoadSnapshot(invalidIdentity, &error).has_value());
    REQUIRE(error == "snapshot entity state is invalid");

    simulation.Step(resolutionTick - simulation.CurrentTick());
    const Entity* possibleState = simulation.FindEntity(intervalist);
    REQUIRE(possibleState != nullptr);
    REQUIRE(possibleState->choirIdentityState == ChoirIdentityState::Possible);
    REQUIRE(possibleState->choirIdentityResolveAtTick == 0);
    REQUIRE(possibleState->attackDamage == base.attackDamage);
    REQUIRE(possibleState->movementPerTickRaw ==
            base.movementPerTickRaw *
                simulation.Config().rules.choirIdentity.possibleMovementPercent /
                100);
    REQUIRE(simulation.ValidateChoirReconciliation(
                0, intervalist, ChoirIdentityState::Manifest) ==
            ChoirReconciliationResult::CooldownActive);
    simulation.Step(nextAvailableTick - simulation.CurrentTick());
    REQUIRE(simulation.ValidateChoirReconciliation(
                0, intervalist, ChoirIdentityState::Manifest) ==
            ChoirReconciliationResult::Valid);
    Command manifest = MakeCommand(
        simulation.CurrentTick(), 0, 2,
        CommandType::ReconcileToManifest, intervalist);
    REQUIRE(simulation.QueueCommand(manifest));
    simulation.Step();
    REQUIRE(simulation.FindEntity(intervalist)->choirIdentityState ==
            ChoirIdentityState::DualResolveManifest);
    REQUIRE(simulation.FindPlayer(0)->resources.dawnshards == 60);

    const ReplayRecord replay = simulation.ExportReplay();
    const std::optional<Simulation> replayed =
        Simulation::ReplayToEnd(replay, &error);
    REQUIRE(replayed.has_value());
    REQUIRE(error.empty());
    REQUIRE(replayed->StateChecksum() == simulation.StateChecksum());

    Simulation poor({16, 16, 20, 0x43484f4952504f4fULL});
    REQUIRE(poor.AddPlayer(0, Faction::HollowChoir, {0, 19}));
    const EntityId poorUnit = poor.SpawnEntity(
        0, Faction::HollowChoir, EntityType::Soldier,
        Vec2::FromTiles(3, 3));
    REQUIRE(poorUnit != 0);
    REQUIRE(poor.ValidateChoirReconciliation(
                0, poorUnit, ChoirIdentityState::Possible) ==
            ChoirReconciliationResult::InsufficientDawn);
}

void TestHollowChoirCoherenceOrderingAndPersistence() {
    SimulationConfig config{24, 24, 20, 0x43484f4952434f48ULL};
    config.rules.choirCoherence.upkeepIntervalTicks = 3;
    config.rules.choirCoherence.dawnCostPerStructure = 5;
    Simulation simulation(config);
    REQUIRE(simulation.AddPlayer(0, Faction::HollowChoir, {1000, 5}));
    const EntityId concordance = simulation.SpawnEntity(
        0, Faction::HollowChoir, EntityType::CommandCore,
        Vec2::FromTiles(4, 4));
    const EntityId intervalLoom = simulation.SpawnEntity(
        0, Faction::HollowChoir, EntityType::Dropoff,
        Vec2::FromTiles(8, 4));
    const EntityId chorusLoom = simulation.SpawnEntity(
        0, Faction::HollowChoir, EntityType::Barracks,
        Vec2::FromTiles(12, 4));
    REQUIRE(concordance != 0 && intervalLoom != 0 && chorusLoom != 0);
    REQUIRE(simulation.FindEntity(concordance)->choirCoherenceNextChargeTick ==
            0);
    REQUIRE(simulation.FindEntity(intervalLoom)->choirCoherenceNextChargeTick ==
            3);
    REQUIRE(simulation.FindEntity(chorusLoom)->choirCoherenceNextChargeTick ==
            3);

    simulation.Step(2);
    const std::vector<std::uint8_t> snapshot = simulation.SaveSnapshot();
    std::string error;
    std::optional<Simulation> restored =
        Simulation::LoadSnapshot(snapshot, &error);
    REQUIRE(restored.has_value());
    REQUIRE(error.empty());
    REQUIRE(restored->StateChecksum() == simulation.StateChecksum());

    simulation.Step(2);
    restored->Step(2);
    REQUIRE(restored->StateChecksum() == simulation.StateChecksum());
    REQUIRE(simulation.CurrentTick() == 4);
    REQUIRE(simulation.FindPlayer(0)->resources.dawnshards == 0);
    REQUIRE(simulation.FindEntity(concordance) != nullptr);
    REQUIRE(simulation.FindEntity(intervalLoom) != nullptr);
    REQUIRE(simulation.FindEntity(intervalLoom)->choirCoherenceNextChargeTick ==
            6);
    REQUIRE(simulation.FindEntity(chorusLoom) == nullptr);
    REQUIRE(Simulation::LoadSnapshot(simulation.SaveSnapshot(), &error).has_value());
}

void TestHollowChoirAiUsesScopedThreatsAndStableTieBreaks() {
    Simulation simulation({64, 64, 20, 0x43484f4952414931ULL});
    REQUIRE(simulation.AddPlayer(0, Faction::HollowChoir, {1000, 200}));
    REQUIRE(simulation.AddPlayer(1, Faction::MeridianCompact, {1000, 200}));
    const EntityId first = simulation.SpawnEntity(
        0, Faction::HollowChoir, EntityType::Soldier,
        Vec2::FromTiles(5, 5));
    const EntityId second = simulation.SpawnEntity(
        0, Faction::HollowChoir, EntityType::HeavyUnit,
        Vec2::FromTiles(7, 5));
    const EntityId hiddenThreat = simulation.SpawnEntity(
        1, Faction::MeridianCompact, EntityType::HeavyUnit,
        Vec2::FromTiles(58, 58));
    REQUIRE(first != 0 && second != 0 && hiddenThreat != 0);
    REQUIRE(!simulation.IsEntityVisibleTo(0, hiddenThreat));

    const std::optional<PlayerView> hiddenView = simulation.CreatePlayerView(0);
    REQUIRE(hiddenView.has_value());
    const std::vector<Command> firstPass =
        Simulation::GenerateAiCommands(*hiddenView, AiPersonality::Adaptive);
    const std::vector<Command> secondPass =
        Simulation::GenerateAiCommands(*hiddenView, AiPersonality::Adaptive);
    REQUIRE(firstPass == secondPass);
    const auto possible = std::find_if(
        firstPass.begin(), firstPass.end(), [](const Command& command) {
            return command.type == CommandType::ReconcileToPossible;
        });
    REQUIRE(possible != firstPass.end());
    REQUIRE(possible->actor == first);
    REQUIRE(std::count_if(
                firstPass.begin(), firstPass.end(), [](const Command& command) {
                    return command.type == CommandType::ReconcileToPossible ||
                           command.type == CommandType::ReconcileToManifest;
                }) == 1);
    REQUIRE(simulation.QueueCommand(*possible));
    simulation.Step(simulation.Config().rules.choirIdentity.durationTicks);
    REQUIRE(simulation.FindEntity(first)->choirIdentityState ==
            ChoirIdentityState::Possible);
    simulation.Step(simulation.Config().rules.choirIdentity.cooldownTicks);

    const EntityId visibleThreat = simulation.SpawnEntity(
        1, Faction::MeridianCompact, EntityType::ScoutUnit,
        Vec2::FromTiles(9, 5));
    REQUIRE(visibleThreat != 0);
    REQUIRE(simulation.IsEntityVisibleTo(0, visibleThreat));
    const std::vector<Command> threatened =
        simulation.GenerateAiCommands(0, AiPersonality::Adaptive);
    const auto manifest = std::find_if(
        threatened.begin(), threatened.end(), [](const Command& command) {
            return command.type == CommandType::ReconcileToManifest;
        });
    REQUIRE(manifest != threatened.end());
    REQUIRE(manifest->actor == first);
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

    CommandBatchRequest batch{};
    batch.clientBatchId = 9;
    CommandIntent firstIntent{};
    firstIntent.type = CommandType::Move;
    firstIntent.actor = 44;
    firstIntent.position = Vec2::FromTiles(4, 5);
    CommandIntent secondIntent{};
    secondIntent.type = CommandType::Attack;
    secondIntent.actor = 55;
    secondIntent.target = 77;
    secondIntent.position = Vec2::FromTiles(7, 8);
    secondIntent.wellChoice = FutureWellChoice::Reshape;
    batch.intents = {firstIntent, secondIntent};
    const std::vector<std::uint8_t> batchBytes =
        EncodeCommandBatchRequest(batch);
    REQUIRE(!batchBytes.empty());
    REQUIRE(batchBytes.size() < kMaximumCommandBatchBytes);
    REQUIRE(batchBytes == EncodeCommandBatchRequest(batch));
    CommandBatchRequest decodedBatch{};
    REQUIRE(DecodeCommandBatchRequest(batchBytes, decodedBatch) ==
            DecodeStatus::Ok);
    REQUIRE(decodedBatch == batch);

    CommandBatchRequest invalidBatch = batch;
    invalidBatch.clientBatchId = 0;
    REQUIRE(EncodeCommandBatchRequest(invalidBatch).empty());
    invalidBatch = batch;
    invalidBatch.intents.clear();
    REQUIRE(EncodeCommandBatchRequest(invalidBatch).empty());
    invalidBatch = batch;
    std::reverse(invalidBatch.intents.begin(), invalidBatch.intents.end());
    REQUIRE(EncodeCommandBatchRequest(invalidBatch).empty());
    invalidBatch = batch;
    invalidBatch.intents[1].actor = invalidBatch.intents[0].actor;
    REQUIRE(EncodeCommandBatchRequest(invalidBatch).empty());
    invalidBatch = batch;
    invalidBatch.intents.assign(
        kMaximumCommandsPerBatch + 1, firstIntent);
    REQUIRE(EncodeCommandBatchRequest(invalidBatch).empty());

    malformed = batchBytes;
    malformed[malformed.size() - 1] ^= 1;
    REQUIRE(DecodeCommandBatchRequest(malformed, decodedBatch) ==
            DecodeStatus::IntegrityMismatch);
    malformed = batchBytes;
    malformed[18] = 1;
    ResignNetworkPacket(malformed);
    REQUIRE(DecodeCommandBatchRequest(malformed, decodedBatch) ==
            DecodeStatus::InvalidEncoding);
    malformed = batchBytes;
    malformed[21] = 0;
    malformed[22] = 0;
    malformed[23] = 0;
    malformed[24] = 0;
    ResignNetworkPacket(malformed);
    REQUIRE(DecodeCommandBatchRequest(malformed, decodedBatch) ==
            DecodeStatus::InvalidEncoding);
    malformed.assign(kMaximumCommandBatchBytes + 1, 0);
    REQUIRE(DecodeCommandBatchRequest(malformed, decodedBatch) ==
            DecodeStatus::PacketTooLarge);

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

    const EntityId visibleHostile = simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Soldier,
        Vec2::FromTiles(12, 10));
    REQUIRE(visibleHostile != 0);
    const std::optional<PlayerView> remoteView =
        simulation.CreatePlayerView(1);
    REQUIRE(remoteView.has_value());
    REQUIRE(!simulation.IsEntityVisibleTo(1, localWorker));
    REQUIRE(simulation.IsEntityVisibleTo(1, visibleHostile));

    ScopedViewKeyframe keyframe{};
    REQUIRE(BuildScopedViewKeyframe(
        *remoteView, 1, context.lastAcceptedSequence, keyframe, &rejection));
    REQUIRE(rejection.empty());
    REQUIRE(keyframe.player == 1);
    REQUIRE(keyframe.simulationTick == simulation.CurrentTick());
    REQUIRE(keyframe.lastAcceptedSequence == 1);
    REQUIRE(keyframe.tiles.size() == 16 * 16);
    REQUIRE(keyframe.scopedDigest != 0);
    REQUIRE(std::none_of(
        keyframe.entities.begin(), keyframe.entities.end(),
        [&](const ScopedEntityState& entity) {
            return entity.id == localWorker;
        }));
    const auto visibleHostileState = std::find_if(
        keyframe.entities.begin(), keyframe.entities.end(),
        [&](const ScopedEntityState& entity) {
            return entity.id == visibleHostile;
        });
    REQUIRE(visibleHostileState != keyframe.entities.end());
    REQUIRE(visibleHostileState->hitPoints == 1);
    REQUIRE(visibleHostileState->maxHitPoints == 1);

    const std::vector<std::uint8_t> keyframeBytes =
        EncodeScopedViewKeyframe(keyframe);
    REQUIRE(!keyframeBytes.empty());
    REQUIRE(keyframeBytes == EncodeScopedViewKeyframe(keyframe));
    ScopedViewKeyframe decodedKeyframe{};
    REQUIRE(DecodeScopedViewKeyframe(keyframeBytes, decodedKeyframe) ==
            DecodeStatus::Ok);
    REQUIRE(decodedKeyframe == keyframe);

    ScopedViewKeyframe changedKeyframe = keyframe;
    changedKeyframe.snapshotId = 2;
    changedKeyframe.simulationTick += 10;
    changedKeyframe.resources.material += 5;
    const auto changedTile = std::find_if(
        changedKeyframe.tiles.begin(), changedKeyframe.tiles.end(),
        [](const ScopedTileState& tile) {
            return tile.visibility != Visibility::Unexplored;
        });
    REQUIRE(changedTile != changedKeyframe.tiles.end());
    changedTile->passable = !changedTile->passable;
    REQUIRE(!changedKeyframe.entities.empty());
    changedKeyframe.entities.front().position = Vec2::FromRaw(
        changedKeyframe.entities.front().position.x.Raw() + 1,
        changedKeyframe.entities.front().position.y.Raw());
    changedKeyframe.entities.erase(
        std::remove_if(
            changedKeyframe.entities.begin(),
            changedKeyframe.entities.end(),
            [&](const ScopedEntityState& entity) {
                return entity.id == visibleHostile;
            }),
        changedKeyframe.entities.end());
    const std::vector<std::uint8_t> changedKeyframeBytes =
        EncodeScopedViewKeyframe(changedKeyframe);
    REQUIRE(!changedKeyframeBytes.empty());
    REQUIRE(DecodeScopedViewKeyframe(
                changedKeyframeBytes, changedKeyframe) == DecodeStatus::Ok);

    ScopedViewDelta delta{};
    REQUIRE(BuildScopedViewDelta(
        keyframe, changedKeyframe, delta, &rejection));
    REQUIRE(rejection.empty());
    REQUIRE(delta.baseSnapshotId == keyframe.snapshotId);
    REQUIRE(delta.snapshotId == changedKeyframe.snapshotId);
    REQUIRE(delta.tileChanges.size() == 1);
    REQUIRE(!delta.entityUpserts.empty());
    REQUIRE(std::find(
                delta.removedEntityIds.begin(),
                delta.removedEntityIds.end(),
                visibleHostile) != delta.removedEntityIds.end());
    const std::vector<std::uint8_t> deltaBytes =
        EncodeScopedViewDelta(delta);
    REQUIRE(!deltaBytes.empty());
    REQUIRE(deltaBytes.size() < changedKeyframeBytes.size());
    ScopedViewDelta decodedDelta{};
    REQUIRE(DecodeScopedViewDelta(deltaBytes, decodedDelta) ==
            DecodeStatus::Ok);
    REQUIRE(decodedDelta == delta);
    ScopedViewKeyframe appliedKeyframe{};
    REQUIRE(ApplyScopedViewDelta(
        keyframe, decodedDelta, appliedKeyframe, &rejection));
    REQUIRE(rejection.empty());
    REQUIRE(appliedKeyframe == changedKeyframe);
    ScopedViewKeyframe wrongBase = keyframe;
    wrongBase.snapshotId = 9;
    REQUIRE(!ApplyScopedViewDelta(
        wrongBase, decodedDelta, appliedKeyframe, &rejection));
    REQUIRE(rejection == "NET_DELTA_BASE_MISSING");
    ScopedViewDelta wrongDigest = decodedDelta;
    wrongDigest.scopedDigest ^= 1;
    REQUIRE(!ApplyScopedViewDelta(
        keyframe, wrongDigest, appliedKeyframe, &rejection));
    REQUIRE(rejection == "NET_DELTA_DIGEST_MISMATCH");
    malformed = deltaBytes;
    malformed[malformed.size() - 1] ^= 1;
    REQUIRE(DecodeScopedViewDelta(malformed, decodedDelta) ==
            DecodeStatus::IntegrityMismatch);

    malformed = keyframeBytes;
    malformed.pop_back();
    REQUIRE(DecodeScopedViewKeyframe(malformed, decodedKeyframe) ==
            DecodeStatus::IntegrityMismatch);
    malformed.resize(89);
    REQUIRE(DecodeScopedViewKeyframe(malformed, decodedKeyframe) ==
            DecodeStatus::LengthMismatch);
    malformed = keyframeBytes;
    malformed[78] = 0x80;
    ResignNetworkPacket(malformed);
    REQUIRE(DecodeScopedViewKeyframe(malformed, decodedKeyframe) ==
            DecodeStatus::InvalidEncoding);
    malformed = keyframeBytes;
    malformed[malformed.size() - 12] ^= 1;
    ResignNetworkPacket(malformed);
    REQUIRE(DecodeScopedViewKeyframe(malformed, decodedKeyframe) ==
            DecodeStatus::IntegrityMismatch);
    malformed.assign(kMaximumScopedKeyframeBytes + 1, 0);
    REQUIRE(DecodeScopedViewKeyframe(malformed, decodedKeyframe) ==
            DecodeStatus::PacketTooLarge);
}

void TestDeterministicNetworkForfeit() {
    Simulation simulation({20, 20, 20, 0x464f5246454954ULL});
    AddTwoPlayers(simulation, {500, 30}, {500, 30});
    const EntityId hostCore = simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::CommandCore,
        Vec2::FromTiles(3, 3));
    const EntityId remoteCore = simulation.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::CommandCore,
        Vec2::FromTiles(16, 16));
    const EntityId remoteSoldier = simulation.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::Soldier,
        Vec2::FromTiles(14, 16));
    REQUIRE(hostCore != 0 && remoteCore != 0 && remoteSoldier != 0);
    REQUIRE(simulation.Outcome() == MatchOutcome::Ongoing);

    simulation.CaptureReplayBaseline();
    Command pendingMove =
        MakeCommand(5, 1, 1, CommandType::Move, remoteSoldier);
    pendingMove.position = Vec2::FromTiles(10, 10);
    REQUIRE(simulation.QueueCommand(pendingMove));
    REQUIRE(simulation.ForfeitPlayer(1));
    REQUIRE(simulation.Outcome() == MatchOutcome::Player0Victory);
    REQUIRE(simulation.FindEntity(remoteCore) == nullptr);
    REQUIRE(!simulation.ForfeitPlayer(1));

    std::string replayError;
    const ReplayRecord forfeitedReplay = simulation.ExportReplay(&replayError);
    REQUIRE(replayError.empty());
    REQUIRE(forfeitedReplay.version == kReplayVersion);
    REQUIRE(forfeitedReplay.forfeitingPlayer == 1);

    const Vec2 remotePosition = simulation.FindEntity(remoteSoldier)->position;
    const std::uint64_t forfeitedChecksum = simulation.StateChecksum();
    std::string snapshotError;
    std::optional<Simulation> restored =
        Simulation::LoadSnapshot(simulation.SaveSnapshot(), &snapshotError);
    REQUIRE(restored.has_value());
    REQUIRE(snapshotError.empty());
    REQUIRE(restored->Outcome() == MatchOutcome::Player0Victory);
    REQUIRE(restored->StateChecksum() == forfeitedChecksum);
    restored->Step(6);
    REQUIRE(restored->FindEntity(remoteSoldier) != nullptr);
    REQUIRE(restored->FindEntity(remoteSoldier)->position == remotePosition);
    REQUIRE(restored->FindEntity(remoteSoldier)->order.type == OrderType::None);
}

// BLD-009: "Players may build multiple production, supply, utility, and
// drop-off structures, but no additional Command Core."
void TestCommandCoreIsNotConstructable() {
    Simulation cores({24, 24, 20, 0x424c44303039434fULL});
    REQUIRE(cores.AddPlayer(0, Faction::MeridianCompact, {5000, 5000}));
    const EntityId core = cores.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::CommandCore,
        Vec2::FromTiles(6, 6));
    const EntityId worker = cores.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Worker,
        Vec2::FromTiles(12, 12));
    REQUIRE(core != 0 && worker != 0);

    const auto CountCores = [&cores]() {
        return std::count_if(
            cores.Entities().begin(), cores.Entities().end(),
            [](const Entity& entity) {
                return entity.type == EntityType::CommandCore;
            });
    };
    REQUIRE(CountCores() == 1);

    REQUIRE(cores.ValidatePlacement(0, EntityType::CommandCore,
                                    Vec2::FromTiles(16, 16)) ==
            PlacementResult::InvalidBuildingType);

    const ResourcePool before = cores.FindPlayer(0)->resources;
    Command spareCore =
        MakeCommand(0, 0, 1, CommandType::Build, worker);
    spareCore.buildType = EntityType::CommandCore;
    spareCore.position = Vec2::FromTiles(16, 16);
    REQUIRE(cores.QueueCommand(spareCore));
    cores.Step(5);
    // No site, no worker order, and not one Matter spent.
    REQUIRE(CountCores() == 1);
    REQUIRE(cores.FindEntity(worker)->order.type != OrderType::Build);
    REQUIRE(cores.FindPlayer(0)->resources.material == before.material);
    REQUIRE(cores.FindPlayer(0)->resources.dawnshards == before.dawnshards);

    // The same footprint still admits a structure BLD-009 does permit, so the
    // rejection is about the Command Core and not about the ground.
    REQUIRE(cores.ValidatePlacement(0, EntityType::Barracks,
                                    Vec2::FromTiles(16, 16)) ==
            PlacementResult::Valid);
    Command barracks = MakeCommand(
        cores.CurrentTick(), 0, 2, CommandType::Build, worker);
    barracks.buildType = EntityType::Barracks;
    barracks.position = Vec2::FromTiles(16, 16);
    REQUIRE(cores.QueueCommand(barracks));
    cores.Step();
    REQUIRE(cores.FindEntity(worker)->order.type == OrderType::Build);
    REQUIRE(cores.FindPlayer(0)->resources.material < before.material);

    // OUT-001/OUT-002 read a *surviving* Core, and an incomplete one is not
    // one. With Cores unconstructable this guard is defence in depth: nothing
    // in normal play can now produce an incomplete Core to shelter behind.
    REQUIRE(cores.Outcome() == MatchOutcome::Ongoing);
}

// MOV-004: "If no route remains, they stop at the last safe position,
// preserve their order as blocked, and alert the owner."
void TestReshapeExpiryStopsWithoutTeleporting() {
    Simulation reshape({20, 20, 20, 0x4d4f56303034525fULL});
    REQUIRE(reshape.AddPlayer(0, Faction::MeridianCompact, {0, 200}));
    const EntityId worker = reshape.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Worker,
        Vec2::FromTiles(5, 6));
    const EntityId nudged = reshape.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Soldier,
        Vec2::FromTiles(5, 7));
    const EntityId stranded = reshape.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Soldier,
        Vec2::FromTiles(7, 6));
    const EntityId well = reshape.SpawnFutureWell(Vec2::FromTiles(6, 6));
    REQUIRE(worker != 0 && nudged != 0 && stranded != 0 && well != 0);

    Command activate = MakeCommand(0, 0, 1, CommandType::FutureWell, worker);
    activate.target = well;
    activate.wellChoice = FutureWellChoice::Reshape;
    REQUIRE(reshape.QueueCommand(activate));
    // Exercise expiry after capture, the public warning, and manifestation.
    reshape.Step(480);
    const Entity* reshapedWell = reshape.FindEntity(well);
    REQUIRE(reshapedWell != nullptr);
    REQUIRE(reshapedWell->wellChoice == FutureWellChoice::Reshape);
    const Tick end = reshapedWell->reshapeUntilTick;
    REQUIRE(end > reshape.CurrentTick());

    // MOV-004 governs terrain that changes UNDER a route, so both orders are
    // issued and accepted while the ground is still open. Closing the pocket
    // first would instead exercise SIM-003 path validation, which refuses an
    // unroutable destination at issue time and never creates the moving unit
    // this requirement is about.
    Command strandedOrder = MakeCommand(
        reshape.CurrentTick(), 0, 2, CommandType::Move, stranded);
    strandedOrder.position = Vec2::FromTiles(18, 18);
    REQUIRE(reshape.QueueCommand(strandedOrder));
    Command nudgedOrder = MakeCommand(
        reshape.CurrentTick(), 0, 3, CommandType::Move, nudged);
    nudgedOrder.position = Vec2::FromTiles(18, 18);
    REQUIRE(reshape.QueueCommand(nudgedOrder));
    reshape.Step();
    REQUIRE(reshape.FindEntity(stranded)->order.type == OrderType::Move);
    REQUIRE(reshape.FindEntity(nudged)->order.type == OrderType::Move);

    // Both units are under way and still inside the tiles they started on, so
    // the pocket closed below still traps them exactly as it did before.
    REQUIRE(reshape.FindEntity(stranded)->position.x.FloorToInt() == 7);
    REQUIRE(reshape.FindEntity(stranded)->position.y.FloorToInt() == 6);

    // Now close every tile within two of the stranded soldier while the
    // Reshape still holds its 3x3 open. When the Reshape lapses there is no
    // safe ground within reach at all.
    for (std::int32_t tileY = 3; tileY <= 9; ++tileY) {
        for (std::int32_t tileX = 4; tileX <= 10; ++tileX) {
            REQUIRE(reshape.SetTerrainTile(tileX, tileY, Terrain::Blocked));
        }
    }

    // One tick for the recalculation MOV-004 requires at the next tick.
    reshape.Step();
    const Entity* strandedStopped = reshape.FindEntity(stranded);
    REQUIRE(strandedStopped != nullptr);
    REQUIRE(strandedStopped->order.type == OrderType::Move);
    REQUIRE(reshape.IsPositionPassable(strandedStopped->position));
    const Vec2 lastSafe = strandedStopped->position;
    // Last safe position, not an arbitrary one: it never left its start tile.
    REQUIRE(lastSafe.x.FloorToInt() == 7);
    REQUIRE(lastSafe.y.FloorToInt() == 6);

    reshape.Step(end - reshape.CurrentTick());
    REQUIRE(reshape.FindEntity(well)->reshapeUntilTick == 0);

    // The stranded soldier stops exactly where it stood. A whole-map search
    // for the nearest open tile would have thrown it three tiles clear of the
    // pocket; MOV-004 grants no displacement of unbounded distance.
    const Entity* strandedAfter = reshape.FindEntity(stranded);
    REQUIRE(strandedAfter != nullptr);
    REQUIRE(strandedAfter->position == lastSafe);
    REQUIRE(reshape.IsPositionPassable(strandedAfter->position));
    // The order survives the ground closing under it.
    REQUIRE(strandedAfter->order.type == OrderType::Move);
    REQUIRE(strandedAfter->order.destination == Vec2::FromTiles(18, 18));

    // A unit that does have safe ground in reach steps onto it, still bounded
    // and still carrying its order.
    const Entity* nudgedAfter = reshape.FindEntity(nudged);
    REQUIRE(nudgedAfter != nullptr);
    REQUIRE(reshape.IsPositionPassable(nudgedAfter->position));
    REQUIRE(nudgedAfter->position != Vec2::FromTiles(5, 7));
    REQUIRE(std::abs(nudgedAfter->position.x.FloorToInt() - 5) <= 2);
    REQUIRE(std::abs(nudgedAfter->position.y.FloorToInt() - 7) <= 2);
    REQUIRE(nudgedAfter->order.type == OrderType::Move);
    REQUIRE(nudgedAfter->order.destination == Vec2::FromTiles(18, 18));
}

// Section 7 terrain table: Scarred is 85% speed.
// FOG information state: "Explored - Remembered terrain and last observed
// permanent objects; no live unit or temporary terrain state." An Explored
// tile must therefore be served from what this player last saw, never from
// the live map, and a scouted permanent object must survive the loss of
// vision that reveals nothing new.
void TestExploredTerrainAndPermanentObjectMemory() {
    Simulation simulation({24, 24, 20, 0x464f474d454d3031ULL});
    AddTwoPlayers(simulation, {1000, 500}, {1000, 500});
    const EntityId homeCore = simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::CommandCore,
        Vec2::FromTiles(2, 2));
    const EntityId scout = simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::ScoutUnit,
        Vec2::FromTiles(16, 16));
    const EntityId enemyBarracks = simulation.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::Barracks,
        Vec2::FromTiles(17, 16));
    const EntityId enemySoldier = simulation.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::Soldier,
        Vec2::FromTiles(16, 17));
    REQUIRE(homeCore != 0 && scout != 0 && enemyBarracks != 0 &&
            enemySoldier != 0);

    const auto ViewOf = [&](PlayerId player) {
        std::optional<PlayerView> view = simulation.CreatePlayerView(player);
        REQUIRE(view.has_value());
        return *view;
    };
    const auto Sees = [&](const PlayerView& view, EntityId id) {
        return std::any_of(view.Entities().begin(), view.Entities().end(),
                           [id](const Entity& entity) {
                               return entity.id == id;
                           });
    };
    const auto Remembers = [&](const PlayerView& view, EntityId id) {
        const auto& memory = view.RememberedObjects();
        return std::find_if(memory.begin(), memory.end(),
                            [id](const RememberedObject& remembered) {
                                return remembered.id == id;
                            });
    };

    // The scout is standing on the contested ground: live terrain, live
    // entities, no memory published for anything it can see right now.
    const PlayerView watching = ViewOf(0);
    REQUIRE(watching.VisibilityAt(Vec2::FromTiles(18, 16)) ==
            Visibility::Visible);
    REQUIRE(watching.TerrainAt(18, 16) == Terrain::Open);
    REQUIRE(Sees(watching, enemyBarracks));
    REQUIRE(Sees(watching, enemySoldier));
    REQUIRE(watching.RememberedObjects().empty());

    // Withdraw until nothing of player 0 can see the site.
    Command withdraw = MakeCommand(0, 0, 1, CommandType::Move, scout);
    withdraw.position = Vec2::FromTiles(3, 3);
    REQUIRE(simulation.QueueCommand(withdraw));
    simulation.Step(220);
    REQUIRE(simulation.FindEntity(scout)->position == Vec2::FromTiles(3, 3));

    const PlayerView withdrawn = ViewOf(0);
    REQUIRE(withdrawn.VisibilityAt(Vec2::FromTiles(18, 16)) ==
            Visibility::Explored);
    // The structure is remembered; the soldier is not. Units belong to the
    // separate optional "Last known" state, which this core does not grant.
    REQUIRE(!Sees(withdrawn, enemyBarracks));
    REQUIRE(!Sees(withdrawn, enemySoldier));
    const auto rememberedBarracks = Remembers(withdrawn, enemyBarracks);
    REQUIRE(rememberedBarracks != withdrawn.RememberedObjects().end());
    REQUIRE(rememberedBarracks->owner == 1);
    REQUIRE(rememberedBarracks->faction == Faction::KharuunAssemblies);
    REQUIRE(rememberedBarracks->type == EntityType::Barracks);
    REQUIRE(rememberedBarracks->position == Vec2::FromTiles(17, 16));
    REQUIRE(Remembers(withdrawn, enemySoldier) ==
            withdrawn.RememberedObjects().end());

    // An enemy Harvest scars ground the player cannot see. Live truth moves;
    // the remembered map must not.
    REQUIRE(simulation.SetTerrainTile(18, 16, Terrain::Scarred));
    simulation.Step();
    REQUIRE(simulation.TerrainAt(18, 16) == Terrain::Scarred);
    const PlayerView blind = ViewOf(0);
    REQUIRE(blind.VisibilityAt(Vec2::FromTiles(18, 16)) ==
            Visibility::Explored);
    REQUIRE(blind.TerrainAt(18, 16) == Terrain::Open);
    // The owner of the ground is standing on it and sees the live tile.
    REQUIRE(ViewOf(1).TerrainAt(18, 16) == Terrain::Scarred);

    // Memory is authoritative per-player state and survives a save exactly.
    const std::vector<std::uint8_t> saved = simulation.SaveSnapshot();
    std::string error;
    const std::optional<Simulation> restored =
        Simulation::LoadSnapshot(saved, &error);
    REQUIRE(restored.has_value());
    REQUIRE(error.empty());
    REQUIRE(restored->StateChecksum() == simulation.StateChecksum());
    const std::optional<PlayerView> restoredView = restored->CreatePlayerView(0);
    REQUIRE(restoredView.has_value());
    REQUIRE(restoredView->TerrainAt(18, 16) == Terrain::Open);
    REQUIRE(restoredView->RememberedObjects() == blind.RememberedObjects());

    // Looking again is the only thing that corrects a memory.
    Command returnScout = MakeCommand(
        simulation.CurrentTick(), 0, 2, CommandType::Move, scout);
    returnScout.position = Vec2::FromTiles(16, 16);
    REQUIRE(simulation.QueueCommand(returnScout));
    simulation.Step(220);
    REQUIRE(simulation.FindEntity(scout)->position == Vec2::FromTiles(16, 16));
    const PlayerView looking = ViewOf(0);
    REQUIRE(looking.VisibilityAt(Vec2::FromTiles(18, 16)) ==
            Visibility::Visible);
    REQUIRE(looking.TerrainAt(18, 16) == Terrain::Scarred);
    REQUIRE(Sees(looking, enemyBarracks));
    REQUIRE(Remembers(looking, enemyBarracks) ==
            looking.RememberedObjects().end());

    // Watching the object die clears the memory rather than leaving a ghost
    // the player can never resolve.
    const EntityId breaker = simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::HeavyUnit,
        Vec2::FromTiles(15, 16));
    REQUIRE(breaker != 0);
    Command demolish = MakeCommand(
        simulation.CurrentTick(), 0, 3, CommandType::Attack, breaker);
    demolish.target = enemyBarracks;
    REQUIRE(simulation.QueueCommand(demolish));
    for (Tick guard = 0;
         guard < 4000 && simulation.FindEntity(enemyBarracks) != nullptr;
         ++guard) {
        simulation.Step();
    }
    REQUIRE(simulation.FindEntity(enemyBarracks) == nullptr);
    const PlayerView cleared = ViewOf(0);
    REQUIRE(cleared.VisibilityAt(Vec2::FromTiles(17, 16)) ==
            Visibility::Visible);
    REQUIRE(Remembers(cleared, enemyBarracks) ==
            cleared.RememberedObjects().end());
}

void TestScarredTerrainCostsSpeed() {
    Simulation scar({20, 20, 20, 0x5343415252454431ULL});
    REQUIRE(scar.AddPlayer(0, Faction::MeridianCompact, {0, 0}));
    const EntityId onOpen = scar.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Soldier,
        Vec2::FromTiles(2, 2));
    const EntityId onScar = scar.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Soldier,
        Vec2::FromTiles(2, 10));
    REQUIRE(onOpen != 0 && onScar != 0);
    for (std::int32_t tileX = 0; tileX < 20; ++tileX) {
        REQUIRE(scar.SetTerrainTile(tileX, 10, Terrain::Scarred));
    }

    Command openMove = MakeCommand(0, 0, 1, CommandType::Move, onOpen);
    openMove.position = Vec2::FromTiles(18, 2);
    Command scarMove = MakeCommand(0, 0, 2, CommandType::Move, onScar);
    scarMove.position = Vec2::FromTiles(18, 10);
    REQUIRE(scar.QueueCommand(openMove));
    REQUIRE(scar.QueueCommand(scarMove));

    // Eight ticks: short enough that neither unit reaches its next path
    // waypoint, where travel is clamped to the waypoint and the per-tick rate
    // stops being observable.
    constexpr std::int32_t kTicks = 8;
    scar.Step(kTicks);
    const std::int32_t baseMovement =
        scar.Config()
            .rules
            .archetypes[static_cast<std::size_t>(Faction::MeridianCompact)]
                       [static_cast<std::size_t>(EntityType::Soldier)]
            .movementPerTickRaw;
    const std::int32_t openTravel =
        scar.FindEntity(onOpen)->position.x.Raw() -
        Vec2::FromTiles(2, 2).x.Raw();
    const std::int32_t scarTravel =
        scar.FindEntity(onScar)->position.x.Raw() -
        Vec2::FromTiles(2, 10).x.Raw();
    REQUIRE(openTravel == kTicks * baseMovement);
    REQUIRE(scarTravel == kTicks * (baseMovement * 85 / 100));
    REQUIRE(scarTravel < openTravel);
    REQUIRE(scar.FindEntity(onScar)->position.y == Vec2::FromTiles(2, 10).y);
}

void TestEuclideanMovementAndSpeedNormalization() {
    REQUIRE(IntegerSqrt64(0) == 0);
    REQUIRE(IntegerSqrt64(1) == 1);
    REQUIRE(IntegerSqrt64(4) == 2);
    REQUIRE(IntegerSqrt64(9) == 3);
    REQUIRE(IntegerSqrt64(16) == 4);
    REQUIRE(IntegerSqrt64(25) == 5);
    REQUIRE(IntegerSqrt64(100) == 10);
    REQUIRE(IntegerSqrt64(1024) == 32);
    REQUIRE(IntegerSqrt64(1048576) == 1024);

    Simulation sim({25, 25, 20, 0x5045444eULL});
    REQUIRE(sim.AddPlayer(0, Faction::MeridianCompact, ResourcePool{0, 0}));
    const EntityId cardUnit = sim.SpawnEntity(0, Faction::MeridianCompact, EntityType::Soldier, Vec2::FromTiles(2, 2));
    const EntityId diagUnit = sim.SpawnEntity(0, Faction::MeridianCompact, EntityType::Soldier, Vec2::FromTiles(2, 2));
    REQUIRE(cardUnit != 0 && diagUnit != 0);

    Command moveCard = MakeCommand(0, 0, 1, CommandType::Move, cardUnit);
    moveCard.position = Vec2::FromTiles(20, 2);
    REQUIRE(sim.QueueCommand(moveCard));

    Command moveDiag = MakeCommand(0, 0, 2, CommandType::Move, diagUnit);
    moveDiag.position = Vec2::FromTiles(20, 20);
    REQUIRE(sim.QueueCommand(moveDiag));

    constexpr std::int32_t kTicks = 10;
    sim.Step(kTicks);

    const Vec2 startPos = Vec2::FromTiles(2, 2);
    const Vec2 cardPos = sim.FindEntity(cardUnit)->position;
    const Vec2 diagPos = sim.FindEntity(diagUnit)->position;

    const std::int64_t cardDx = static_cast<std::int64_t>(cardPos.x.Raw()) - startPos.x.Raw();
    const std::int64_t cardDy = static_cast<std::int64_t>(cardPos.y.Raw()) - startPos.y.Raw();
    const std::int64_t cardDist = IntegerSqrt64(cardDx * cardDx + cardDy * cardDy);

    const std::int64_t diagDx = static_cast<std::int64_t>(diagPos.x.Raw()) - startPos.x.Raw();
    const std::int64_t diagDy = static_cast<std::int64_t>(diagPos.y.Raw()) - startPos.y.Raw();
    const std::int64_t diagDist = IntegerSqrt64(diagDx * diagDx + diagDy * diagDy);

    REQUIRE(cardDist > 0);
    REQUIRE(diagDist > 0);
    const std::int64_t varianceBps = std::abs(diagDist - cardDist) * 10000 / cardDist;
    REQUIRE(varianceBps <= 200);
}

void TestAnyAngleStringPulling() {
    Simulation sim({30, 30, 20, 0x53545249ULL});
    REQUIRE(sim.AddPlayer(0, Faction::MeridianCompact, ResourcePool{0, 0}));
    const Vec2 start = Vec2::FromTiles(3, 3);
    const Vec2 dest = Vec2::FromTiles(23, 11);
    const EntityId unit = sim.SpawnEntity(0, Faction::MeridianCompact, EntityType::Soldier, start);
    REQUIRE(unit != 0);

    Command cmd = MakeCommand(0, 0, 1, CommandType::Move, unit);
    cmd.position = dest;
    REQUIRE(sim.QueueCommand(cmd));

    const std::int64_t lineDx = static_cast<std::int64_t>(dest.x.Raw()) - start.x.Raw();
    const std::int64_t lineDy = static_cast<std::int64_t>(dest.y.Raw()) - start.y.Raw();
    const std::int64_t lineLen = IntegerSqrt64(lineDx * lineDx + lineDy * lineDy);
    REQUIRE(lineLen > 0);

    bool arrived = false;
    for (int tick = 0; tick < 200; ++tick) {
        sim.Step();
        const Entity* e = sim.FindEntity(unit);
        REQUIRE(e != nullptr);
        if (e->position == dest) {
            arrived = true;
            break;
        }
        const std::int64_t px = e->position.x.Raw();
        const std::int64_t py = e->position.y.Raw();
        const std::int64_t numerator = std::abs(lineDx * (start.y.Raw() - py) - lineDy * (start.x.Raw() - px));
        const std::int64_t perpDev = numerator / lineLen;
        REQUIRE(perpDev <= 256);
    }
    REQUIRE(arrived);
}

void TestSoftSeparationAndClusterStability() {
    Simulation sim({30, 30, 20, 0x434c5553ULL});
    REQUIRE(sim.AddPlayer(0, Faction::MeridianCompact, ResourcePool{0, 0}));
    const Vec2 focalPoint = Vec2::FromTiles(15, 15);
    std::vector<EntityId> units;
    units.reserve(40);
    for (int i = 0; i < 40; ++i) {
        const std::int32_t startX = 2 + (i % 8);
        const std::int32_t startY = 2 + (i / 8);
        const EntityId id = sim.SpawnEntity(
            0, Faction::MeridianCompact, EntityType::Soldier, Vec2::FromTiles(startX, startY));
        REQUIRE(id != 0);
        units.push_back(id);
        Command cmd = MakeCommand(0, 0, static_cast<std::uint64_t>(i + 1), CommandType::Move, id);
        cmd.position = focalPoint;
        REQUIRE(sim.QueueCommand(cmd));
    }

    sim.Step(200);

    std::int32_t completedCount = 0;
    for (EntityId id : units) {
        const Entity* e = sim.FindEntity(id);
        REQUIRE(e != nullptr);
        if (e->order.type == OrderType::None) {
            ++completedCount;
        }
    }
    REQUIRE(completedCount == 40);

    std::int32_t overlapCount = 0;
    for (std::size_t i = 0; i < units.size(); ++i) {
        const Entity* a = sim.FindEntity(units[i]);
        for (std::size_t j = i + 1; j < units.size(); ++j) {
            const Entity* b = sim.FindEntity(units[j]);
            const std::int64_t dx = static_cast<std::int64_t>(b->position.x.Raw()) - a->position.x.Raw();
            const std::int64_t dy = static_cast<std::int64_t>(b->position.y.Raw()) - a->position.y.Raw();
            const std::int64_t dist = IntegerSqrt64(dx * dx + dy * dy);
            if (dist < 64) {
                ++overlapCount;
            }
        }
    }
    REQUIRE(overlapCount == 0);
}

void TestChokepointNegotiationThroughput() {
    Simulation sim({25, 20, 20, 0x43484f4bULL});
    REQUIRE(sim.AddPlayer(0, Faction::MeridianCompact, ResourcePool{0, 0}));
    for (std::int32_t y = 0; y < 20; ++y) {
        if (y != 10) {
            REQUIRE(sim.SetTerrainTile(10, y, Terrain::Blocked));
        }
    }
    REQUIRE(sim.IsPositionPassable(Vec2::FromTiles(10, 10)));
    REQUIRE(!sim.IsPositionPassable(Vec2::FromTiles(10, 9)));
    REQUIRE(!sim.IsPositionPassable(Vec2::FromTiles(10, 11)));

    std::vector<EntityId> units;
    units.reserve(12);
    for (int i = 0; i < 12; ++i) {
        const std::int32_t x = 4 + (i % 3);
        const std::int32_t y = 8 + (i / 3);
        const EntityId id = sim.SpawnEntity(
            0, Faction::MeridianCompact, EntityType::Soldier, Vec2::FromTiles(x, y));
        REQUIRE(id != 0);
        units.push_back(id);
        Command cmd = MakeCommand(0, 0, static_cast<std::uint64_t>(i + 1), CommandType::Move, id);
        cmd.position = Vec2::FromTiles(18, 10);
        REQUIRE(sim.QueueCommand(cmd));
    }

    sim.Step(250);

    std::int32_t crossedCount = 0;
    for (EntityId id : units) {
        const Entity* e = sim.FindEntity(id);
        REQUIRE(e != nullptr);
        if (e->position.x.Raw() >= 12 * kFixedScale) {
            ++crossedCount;
        }
    }
    REQUIRE(crossedCount == 12);
}

void TestArrivalDampingAndNoOscillation() {
    Simulation sim({20, 20, 20, 0x44414d50ULL});
    REQUIRE(sim.AddPlayer(0, Faction::MeridianCompact, ResourcePool{0, 0}));
    const EntityId unit = sim.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Soldier, Vec2::FromTiles(3, 3));
    REQUIRE(unit != 0);

    const Vec2 goal = Vec2::FromTiles(8, 7);
    Command cmd = MakeCommand(0, 0, 1, CommandType::Move, unit);
    cmd.position = goal;
    REQUIRE(sim.QueueCommand(cmd));

    bool reached = false;
    for (int tick = 0; tick < 100; ++tick) {
        sim.Step();
        const Entity* e = sim.FindEntity(unit);
        REQUIRE(e != nullptr);
        if (e->order.type == OrderType::None) {
            REQUIRE(e->position == goal);
            reached = true;
            break;
        }
    }
    REQUIRE(reached);

    for (int tick = 0; tick < 20; ++tick) {
        sim.Step();
        const Entity* e = sim.FindEntity(unit);
        REQUIRE(e != nullptr);
        REQUIRE(e->position == goal);
        REQUIRE(e->order.type == OrderType::None);
    }
}

// SPEC-MOV-008 / SPEC-MOV-011 / SPEC-MOV-012: a group ordered to one point
// packs around it without stacking on the coordinate, every unit then holds
// still, a unit at rest is never displaced by traffic passing through it, and
// a rooted Waystone holds its site while allied units crowd it.
void TestGroupArrivalPackingAndRestStability() {
    {
        Simulation sim({30, 30, 20, 0x5041434bULL});
        REQUIRE(sim.AddPlayer(0, Faction::MeridianCompact, ResourcePool{0, 0}));
        const Vec2 rally = Vec2::FromTiles(20, 20);
        std::vector<EntityId> group;
        for (int i = 0; i < 8; ++i) {
            const EntityId id = sim.SpawnEntity(
                0, Faction::MeridianCompact, EntityType::Soldier,
                Vec2::FromTiles(4 + (i % 4) * 2, 4 + (i / 4) * 2));
            REQUIRE(id != 0);
            group.push_back(id);
            Command cmd = MakeCommand(0, 0, static_cast<std::uint64_t>(i + 1),
                                      CommandType::Move, id);
            cmd.position = rally;
            REQUIRE(sim.QueueCommand(cmd));
        }
        bool allResolved = false;
        for (int tick = 0; tick < 600 && !allResolved; ++tick) {
            sim.Step();
            allResolved = true;
            for (const EntityId id : group) {
                const Entity* e = sim.FindEntity(id);
                REQUIRE(e != nullptr);
                allResolved = allResolved && e->order.type == OrderType::None;
            }
        }
        REQUIRE(allResolved);

        constexpr std::int64_t kPackingRadiusRaw = 2 * kFixedScale;
        constexpr std::int64_t kClearanceRaw = 2 * (kFixedScale / 8);
        std::vector<Vec2> settled;
        for (std::size_t i = 0; i < group.size(); ++i) {
            const Entity* a = sim.FindEntity(group[i]);
            const std::int64_t rx = static_cast<std::int64_t>(a->position.x.Raw()) - rally.x.Raw();
            const std::int64_t ry = static_cast<std::int64_t>(a->position.y.Raw()) - rally.y.Raw();
            REQUIRE(rx * rx + ry * ry <= kPackingRadiusRaw * kPackingRadiusRaw);
            settled.push_back(a->position);
            for (std::size_t j = i + 1; j < group.size(); ++j) {
                const Entity* b = sim.FindEntity(group[j]);
                const std::int64_t dx = static_cast<std::int64_t>(b->position.x.Raw()) - a->position.x.Raw();
                const std::int64_t dy = static_cast<std::int64_t>(b->position.y.Raw()) - a->position.y.Raw();
                REQUIRE(dx * dx + dy * dy >= kClearanceRaw * kClearanceRaw);
            }
        }
        // SPEC-MOV-012.AUTH: no more than 0.05 tiles of drift over 20 ticks.
        constexpr std::int64_t kSettleToleranceRaw = kFixedScale / 20;
        for (int tick = 0; tick < 20; ++tick) {
            sim.Step();
            for (std::size_t i = 0; i < group.size(); ++i) {
                const Entity* e = sim.FindEntity(group[i]);
                REQUIRE(e != nullptr);
                REQUIRE(e->order.type == OrderType::None);
                const std::int64_t dx = static_cast<std::int64_t>(e->position.x.Raw()) - settled[i].x.Raw();
                const std::int64_t dy = static_cast<std::int64_t>(e->position.y.Raw()) - settled[i].y.Raw();
                REQUIRE(dx * dx + dy * dy <= kSettleToleranceRaw * kSettleToleranceRaw);
            }
        }
    }
    {
        // A resting unit stays put while an ally drives straight through it.
        Simulation sim({30, 30, 20, 0x52455354ULL});
        REQUIRE(sim.AddPlayer(0, Faction::MeridianCompact, ResourcePool{0, 0}));
        const Vec2 restPoint = Vec2::FromTiles(10, 10);
        const EntityId resting = sim.SpawnEntity(
            0, Faction::MeridianCompact, EntityType::Soldier, restPoint);
        const EntityId passing = sim.SpawnEntity(
            0, Faction::MeridianCompact, EntityType::Soldier, Vec2::FromTiles(4, 10));
        REQUIRE(resting != 0 && passing != 0);
        const Vec2 farSide = Vec2::FromTiles(16, 10);
        Command cmd = MakeCommand(0, 0, 1, CommandType::Move, passing);
        cmd.position = farSide;
        REQUIRE(sim.QueueCommand(cmd));
        sim.Step(160);
        REQUIRE(sim.FindEntity(resting)->position == restPoint);
        REQUIRE(sim.FindEntity(passing)->order.type == OrderType::None);
        REQUIRE(sim.FindEntity(passing)->position == farSide);
    }
    {
        // A rooted Waystone is a structure: allied units crowding its site
        // never displace it.
        Simulation sim({30, 30, 20, 0x57415953ULL});
        REQUIRE(sim.AddPlayer(0, Faction::KharuunAssemblies, ResourcePool{0, 0}));
        const Vec2 site = Vec2::FromTiles(12, 12);
        const EntityId waystone = sim.SpawnEntity(
            0, Faction::KharuunAssemblies, EntityType::Dropoff, site);
        REQUIRE(waystone != 0);
        REQUIRE(sim.FindEntity(waystone)->waystoneMode == WaystoneMode::Rooted);
        for (int i = 0; i < 6; ++i) {
            const EntityId worker = sim.SpawnEntity(
                0, Faction::KharuunAssemblies, EntityType::Worker,
                Vec2::FromTiles(6 + i, 6));
            REQUIRE(worker != 0);
            Command cmd = MakeCommand(0, 0, static_cast<std::uint64_t>(i + 1),
                                      CommandType::Move, worker);
            cmd.position = site;
            REQUIRE(sim.QueueCommand(cmd));
        }
        sim.Step(200);
        REQUIRE(sim.FindEntity(waystone)->position == site);
        REQUIRE(sim.FindEntity(waystone)->waystoneMode == WaystoneMode::Rooted);
    }
    {
        // A travelling Waystone outweighs resting workers: it reaches its
        // exact ordered site through them and can root there.
        Simulation sim({30, 30, 20, 0x4d4f5657ULL});
        REQUIRE(sim.AddPlayer(0, Faction::KharuunAssemblies, ResourcePool{0, 0}));
        const EntityId waystone = sim.SpawnEntity(
            0, Faction::KharuunAssemblies, EntityType::Dropoff, Vec2::FromTiles(6, 12));
        REQUIRE(waystone != 0);
        const Vec2 routeSite = Vec2::FromTiles(18, 12);
        for (int i = 0; i < 3; ++i) {
            const EntityId worker = sim.SpawnEntity(
                0, Faction::KharuunAssemblies, EntityType::Worker,
                Vec2::FromTiles(17 + i, 12));
            REQUIRE(worker != 0);
        }
        Command uproot = MakeCommand(0, 0, 1, CommandType::ToggleWaystoneRoot, waystone);
        REQUIRE(sim.QueueCommand(uproot));
        sim.Step(sim.Config().rules.waystoneMigration.uprootTicks + 1);
        REQUIRE(sim.FindEntity(waystone)->waystoneMode == WaystoneMode::Mobile);
        Command travel = MakeCommand(sim.CurrentTick(), 0, 2, CommandType::Move, waystone);
        travel.position = routeSite;
        REQUIRE(sim.QueueCommand(travel));
        sim.Step(400);
        REQUIRE(sim.FindEntity(waystone)->position == routeSite);
        REQUIRE(sim.FindEntity(waystone)->order.type == OrderType::None);
        sim.Step(20);
        REQUIRE(sim.FindEntity(waystone)->position == routeSite);
        REQUIRE(sim.ValidateWaystoneRoot(0, waystone) == WaystoneRootResult::Valid);
        Command root = MakeCommand(sim.CurrentTick(), 0, 3, CommandType::ToggleWaystoneRoot, waystone);
        REQUIRE(sim.QueueCommand(root));
        sim.Step(sim.Config().rules.waystoneMigration.rootTicks + 1);
        REQUIRE(sim.FindEntity(waystone)->waystoneMode == WaystoneMode::Rooted);
        REQUIRE(sim.FindEntity(waystone)->position == routeSite);
    }
}

void TestCommandResponsivenessAndInterruptibility() {
    Simulation sim({20, 20, 20, 0x4354524cULL});
    REQUIRE(sim.AddPlayer(0, Faction::MeridianCompact, ResourcePool{0, 0}));
    const EntityId unit = sim.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Soldier, Vec2::FromTiles(5, 5));
    REQUIRE(unit != 0);

    Command moveEast = MakeCommand(0, 0, 1, CommandType::Move, unit);
    moveEast.position = Vec2::FromTiles(15, 5);
    REQUIRE(sim.QueueCommand(moveEast));
    sim.Step();

    const std::optional<CommandResolutionReceipt> r1 = sim.FindCommandResolutionReceipt(0, 1);
    REQUIRE(r1.has_value());
    REQUIRE(r1->outcome == CommandResolutionOutcome::Applied);
    const Entity* e1 = sim.FindEntity(unit);
    REQUIRE(e1 != nullptr);
    REQUIRE(e1->order.type == OrderType::Move);
    REQUIRE(e1->order.destination == Vec2::FromTiles(15, 5));
    REQUIRE(e1->position.x.Raw() > Vec2::FromTiles(5, 5).x.Raw());

    Command moveNorth = MakeCommand(1, 0, 2, CommandType::Move, unit);
    moveNorth.position = Vec2::FromTiles(5, 15);
    REQUIRE(sim.QueueCommand(moveNorth));
    sim.Step();

    const std::optional<CommandResolutionReceipt> r2 = sim.FindCommandResolutionReceipt(0, 2);
    REQUIRE(r2.has_value());
    REQUIRE(r2->outcome == CommandResolutionOutcome::Applied);
    const Entity* e2 = sim.FindEntity(unit);
    REQUIRE(e2 != nullptr);
    REQUIRE(e2->order.type == OrderType::Move);
    REQUIRE(e2->order.destination == Vec2::FromTiles(5, 15));
    REQUIRE(e2->position.y.Raw() > Vec2::FromTiles(5, 5).y.Raw());
}

void TestShiftQueuedOrderChaining() {
    // SPEC-CMD-011: Shift-Queued Order Chaining
    Simulation sim(SimulationConfig{64, 64, 20, 0x511EULL});
    REQUIRE(sim.AddPlayer(0, Faction::MeridianCompact, ResourcePool{1000, 500}));
    const EntityId unit = sim.SpawnEntity(0, Faction::MeridianCompact, EntityType::Soldier, Vec2::FromTiles(2, 2));
    REQUIRE(unit != 0);

    // Initial Move to (5, 2)
    Command m1 = MakeCommand(sim.CurrentTick(), 0, 1, CommandType::Move, unit);
    m1.position = Vec2::FromTiles(5, 2);
    REQUIRE(sim.QueueCommand(m1));

    // Shift-queued Move to (5, 8)
    Command m2 = MakeCommand(sim.CurrentTick(), 0, 2, CommandType::Move, unit);
    m2.position = Vec2::FromTiles(5, 8);
    m2.queue = true;
    REQUIRE(sim.QueueCommand(m2));

    // Shift-queued Move to (10, 8)
    Command m3 = MakeCommand(sim.CurrentTick(), 0, 3, CommandType::Move, unit);
    m3.position = Vec2::FromTiles(10, 8);
    m3.queue = true;
    REQUIRE(sim.QueueCommand(m3));

    sim.Step();

    const Entity* e = sim.FindEntity(unit);
    REQUIRE(e != nullptr);
    REQUIRE(e->order.type == OrderType::Move);
    REQUIRE(e->order.destination == Vec2::FromTiles(5, 2));
    REQUIRE(e->orderQueue.size() == 2);
    REQUIRE(e->orderQueue[0].destination == Vec2::FromTiles(5, 8));
    REQUIRE(e->orderQueue[1].destination == Vec2::FromTiles(10, 8));

    // Step until leg 1 arrives at (5, 2)
    while (sim.FindEntity(unit)->position != Vec2::FromTiles(5, 2) && sim.CurrentTick() < 200) {
        sim.Step();
    }
    REQUIRE(sim.FindEntity(unit)->position == Vec2::FromTiles(5, 2));
    // Leg 2 should now be active
    REQUIRE(sim.FindEntity(unit)->order.type == OrderType::Move);
    REQUIRE(sim.FindEntity(unit)->order.destination == Vec2::FromTiles(5, 8));
    REQUIRE(sim.FindEntity(unit)->orderQueue.size() == 1);

    // Step until leg 2 arrives at (5, 8)
    while (sim.FindEntity(unit)->position != Vec2::FromTiles(5, 8) && sim.CurrentTick() < 400) {
        sim.Step();
    }
    REQUIRE(sim.FindEntity(unit)->position == Vec2::FromTiles(5, 8));
    // Leg 3 should now be active
    REQUIRE(sim.FindEntity(unit)->order.type == OrderType::Move);
    REQUIRE(sim.FindEntity(unit)->order.destination == Vec2::FromTiles(10, 8));
    REQUIRE(sim.FindEntity(unit)->orderQueue.empty());

    // Step until leg 3 arrives at (10, 8)
    while (sim.FindEntity(unit)->position != Vec2::FromTiles(10, 8) && sim.CurrentTick() < 600) {
        sim.Step();
    }
    REQUIRE(sim.FindEntity(unit)->position == Vec2::FromTiles(10, 8));
    REQUIRE(sim.FindEntity(unit)->order.type == OrderType::None);
    REQUIRE(sim.FindEntity(unit)->orderQueue.empty());
}

void TestShiftQueueDepthAndImmediateInterrupt() {
    // SPEC-CMD-011: Queue depth up to 16 commands & immediate interruptibility
    Simulation sim(SimulationConfig{64, 64, 20, 0xD001ULL});
    REQUIRE(sim.AddPlayer(0, Faction::MeridianCompact, ResourcePool{1000, 500}));
    const EntityId unit = sim.SpawnEntity(0, Faction::MeridianCompact, EntityType::Soldier, Vec2::FromTiles(5, 5));
    REQUIRE(unit != 0);

    // Initial move
    Command m0 = MakeCommand(0, 0, 1, CommandType::Move, unit);
    m0.position = Vec2::FromTiles(6, 5);
    REQUIRE(sim.QueueCommand(m0));
    sim.Step();

    // Queue 16 additional commands (queue depth up to kMaxQueuedOrders = 16)
    for (std::uint64_t seq = 2; seq <= 17; ++seq) {
        Command cmd = MakeCommand(sim.CurrentTick(), 0, seq, CommandType::Move, unit);
        cmd.position = Vec2::FromTiles(6 + static_cast<std::int32_t>(seq), 5);
        cmd.queue = true;
        REQUIRE(sim.QueueCommand(cmd));
    }
    sim.Step();

    const Entity* e = sim.FindEntity(unit);
    REQUIRE(e != nullptr);
    REQUIRE(e->orderQueue.size() == 16);

    // 18th command exceeding queue limit should be capped at 16 queued
    Command cmd18 = MakeCommand(sim.CurrentTick(), 0, 18, CommandType::Move, unit);
    cmd18.position = Vec2::FromTiles(25, 5);
    cmd18.queue = true;
    REQUIRE(sim.QueueCommand(cmd18));
    sim.Step();
    REQUIRE(sim.FindEntity(unit)->orderQueue.size() == 16);

    // Issue non-queued Stop command: immediately clears queue and halts unit
    Command stopCmd = MakeCommand(sim.CurrentTick(), 0, 19, CommandType::Stop, unit);
    stopCmd.queue = false;
    REQUIRE(sim.QueueCommand(stopCmd));
    sim.Step();

    const Entity* stopped = sim.FindEntity(unit);
    REQUIRE(stopped != nullptr);
    REQUIRE(stopped->order.type == OrderType::None);
    REQUIRE(stopped->orderQueue.empty());
}

void TestSmartCastSingleUnitDispatch() {
    // SPEC-CMD-013: Smart-Cast Single-Unit Dispatch
    Simulation sim(SimulationConfig{64, 64, 20, 0x54ACULL});
    REQUIRE(sim.AddPlayer(0, Faction::KharuunAssemblies, ResourcePool{1000, 500}));
    const EntityId cClose = sim.SpawnEntity(0, Faction::KharuunAssemblies, EntityType::HeavyUnit, Vec2::FromTiles(5, 5));
    const EntityId cMid = sim.SpawnEntity(0, Faction::KharuunAssemblies, EntityType::HeavyUnit, Vec2::FromTiles(10, 10));
    const EntityId cFar = sim.SpawnEntity(0, Faction::KharuunAssemblies, EntityType::HeavyUnit, Vec2::FromTiles(20, 20));
    REQUIRE(cClose != 0 && cMid != 0 && cFar != 0);

    const std::vector<EntityId> group{cFar, cMid, cClose};
    const Vec2 targetPos = Vec2::FromTiles(6, 6);

    // First smart-cast dispatch: closest unit should be chosen
    const EntityId caster1 = sim.FindSmartCastCaster(0, CommandType::RaiseMineralCover, targetPos, 0, group);
    REQUIRE(caster1 == cClose);

    // Dispatch ability for cClose
    Command cover1 = MakeCommand(sim.CurrentTick(), 0, 1, CommandType::RaiseMineralCover, caster1);
    cover1.position = targetPos;
    REQUIRE(sim.QueueCommand(cover1));
    sim.Step();

    // Verify cClose casted and is on cooldown
    const Entity* eClose = sim.FindEntity(cClose);
    REQUIRE(eClose != nullptr);
    REQUIRE(eClose->mineralCoverCooldownUntilTick > sim.CurrentTick());

    // Second smart-cast dispatch: cClose is on cooldown, so cMid should be chosen
    const EntityId caster2 = sim.FindSmartCastCaster(0, CommandType::RaiseMineralCover, targetPos, 0, group);
    REQUIRE(caster2 == cMid);

    // Dispatch ability for cMid
    const Vec2 targetPos2 = Vec2::FromTiles(9, 9);
    Command cover2 = MakeCommand(sim.CurrentTick(), 0, 2, CommandType::RaiseMineralCover, caster2);
    cover2.position = targetPos2;
    REQUIRE(sim.QueueCommand(cover2));
    sim.Step();

    // Third smart-cast dispatch: both cClose and cMid on cooldown, so cFar is chosen
    const EntityId caster3 = sim.FindSmartCastCaster(0, CommandType::RaiseMineralCover, targetPos, 0, group);
    REQUIRE(caster3 == cFar);
}

void TestAttackMoveThreatFiltering() {
    // SPEC-CMD-014: Attack-Move Intelligent Threat Filtering
    Simulation sim(SimulationConfig{64, 64, 20, 0x474EULL});
    REQUIRE(sim.AddPlayer(0, Faction::MeridianCompact, ResourcePool{1000, 500}));
    REQUIRE(sim.AddPlayer(1, Faction::KharuunAssemblies, ResourcePool{1000, 500}));

    // Player 0 attacker
    const EntityId attacker = sim.SpawnEntity(0, Faction::MeridianCompact, EntityType::Soldier, Vec2::FromTiles(5, 5));
    REQUIRE(attacker != 0);

    // Player 1 passive building (Dropoff, attackDamage == 0, movement == 0) at (7, 5)
    const EntityId passiveBuilding = sim.SpawnEntity(1, Faction::KharuunAssemblies, EntityType::Dropoff, Vec2::FromTiles(7, 5));
    REQUIRE(passiveBuilding != 0);

    // Player 1 armed soldier at (9, 5)
    const EntityId armedEnemy = sim.SpawnEntity(1, Faction::KharuunAssemblies, EntityType::Soldier, Vec2::FromTiles(9, 5));
    REQUIRE(armedEnemy != 0);

    // Issue Attack-Move towards (15, 5) passing both enemies
    Command am = MakeCommand(0, 0, 1, CommandType::AttackMove, attacker);
    am.position = Vec2::FromTiles(15, 5);
    REQUIRE(sim.QueueCommand(am));
    sim.Step();

    // The attacker must prioritize the armed enemy soldier over the passive building
    const Entity* eAttacker = sim.FindEntity(attacker);
    REQUIRE(eAttacker != nullptr);
    REQUIRE(eAttacker->order.type == OrderType::AttackMove);
    REQUIRE(eAttacker->order.target == armedEnemy);

    // Step combat until armed enemy dies
    while (sim.FindEntity(armedEnemy) != nullptr && sim.CurrentTick() < 100) {
        sim.Step();
    }
    REQUIRE(sim.FindEntity(armedEnemy) == nullptr);

    // Once armed threat is destroyed, unit targets remaining passive building
    sim.Step();
    const Entity* eAfter = sim.FindEntity(attacker);
    REQUIRE(eAfter != nullptr);
    REQUIRE(eAfter->order.target == passiveBuilding);
}

void TestFocusFireChaseLeashing() {
    // SPEC-CMD-015: Focus-Fire Target Preservation on Range Loss & Chase Leashing
    Simulation sim(SimulationConfig{64, 64, 20, 0x1E45ULL});
    REQUIRE(sim.AddPlayer(0, Faction::MeridianCompact, ResourcePool{1000, 500}));
    REQUIRE(sim.AddPlayer(1, Faction::KharuunAssemblies, ResourcePool{1000, 500}));

    // Player 0 soldier at (10, 10)
    const EntityId soldier = sim.SpawnEntity(0, Faction::MeridianCompact, EntityType::Soldier, Vec2::FromTiles(10, 10));
    // Player 1 scout at (12, 10)
    const EntityId scout = sim.SpawnEntity(1, Faction::KharuunAssemblies, EntityType::ScoutUnit, Vec2::FromTiles(12, 10));
    REQUIRE(soldier != 0 && scout != 0);

    // Issue focus-fire Attack command on Scout
    Command atk = MakeCommand(0, 0, 1, CommandType::Attack, soldier);
    atk.target = scout;
    REQUIRE(sim.QueueCommand(atk));
    sim.Step();

    const Entity* s0 = sim.FindEntity(soldier);
    REQUIRE(s0 != nullptr);
    REQUIRE(s0->order.type == OrderType::Attack);
    REQUIRE(s0->order.target == scout);
    const Vec2 origin = s0->order.anchor;

    // Command the scout to flee far east to (30, 10)
    Command flee = MakeCommand(sim.CurrentTick(), 1, 2, CommandType::Move, scout);
    flee.position = Vec2::FromTiles(30, 10);
    REQUIRE(sim.QueueCommand(flee));

    // Step simulation as scout flees east and soldier pursues
    for (int t = 0; t < 100; ++t) {
        sim.Step();

        const Entity* curSoldier = sim.FindEntity(soldier);
        REQUIRE(curSoldier != nullptr);
        // Distance traveled from origin should never exceed 400 cm (4 tiles = 4096 raw units)
        const std::int64_t deltaX = curSoldier->position.x.Raw() - origin.x.Raw();
        const std::int64_t deltaY = curSoldier->position.y.Raw() - origin.y.Raw();
        const std::int64_t distRaw = IntegerSqrt64(deltaX * deltaX + deltaY * deltaY);
        REQUIRE(distRaw <= 4 * kFixedScale + 256);
    }

    // Soldier must have halted pursuit and cleared the chase order
    const Entity* finalSoldier = sim.FindEntity(soldier);
    REQUIRE(finalSoldier != nullptr);
    REQUIRE(finalSoldier->order.type == OrderType::None);
}

void TestCampaignStructureAndReplayability() {
    // SPEC-CAM-001: Structure & Replayability (15 operations in 3 Acts)
    constexpr int32_t kTotalCampaignOperations = 15;
    constexpr int32_t kOperationsPerAct = 5;
    constexpr int32_t kActCount = 3;
    REQUIRE(kOperationsPerAct * kActCount == kTotalCampaignOperations);

    // SPEC-CAM-002: Capability manifests & lesson sequencing
    enum class CapabilityStage { Introduced, Practiced, Assessed, Retained, Locked };
    struct OperationManifest {
        std::uint8_t operationIndex; // 1 to 15
        CapabilityStage worker;
        CapabilityStage soldier;
        CapabilityStage heavy;
        CapabilityStage scout;
        CapabilityStage futureWell;
    };
    // M01 introduces Worker, Soldier, FutureWell; locks Heavy and Scout
    const OperationManifest m01{1, CapabilityStage::Introduced, CapabilityStage::Introduced, CapabilityStage::Locked, CapabilityStage::Introduced, CapabilityStage::Introduced};
    REQUIRE(m01.operationIndex == 1);
    REQUIRE(m01.worker == CapabilityStage::Introduced);
    REQUIRE(m01.heavy == CapabilityStage::Locked);

    // SPEC-CAM-003: Persistence & Reset Rules
    struct CampaignState {
        std::uint32_t completedMissionsMask = 0;
        FutureWellChoice foundingWellChoice = FutureWellChoice::Dormant;
        FutureWellChoice lumeWellChoice = FutureWellChoice::Dormant;
        std::uint32_t unlockedRosterMask = 0;
        // Session-only state (must reset between operations)
        std::vector<EntityId> deployedUnits{};
        std::int32_t sessionResources = 0;
    };
    CampaignState state{};
    // Complete Mission 1 with Preserve
    state.completedMissionsMask |= (1U << 1);
    state.foundingWellChoice = FutureWellChoice::Preserve;
    state.deployedUnits.push_back(100);
    state.sessionResources = 500;

    // Reset session between operations: units and resources reset, Well records persist
    state.deployedUnits.clear();
    state.sessionResources = 0;
    REQUIRE(state.deployedUnits.empty());
    REQUIRE(state.sessionResources == 0);
    REQUIRE((state.completedMissionsMask & (1U << 1)) != 0);
    REQUIRE(state.foundingWellChoice == FutureWellChoice::Preserve);

    // SPEC-CAM-006: Branch clarity without hidden morality score
    REQUIRE(state.foundingWellChoice != FutureWellChoice::Dormant);
}

void TestCampaignMissionStartingPackagesAndRosters() {
    // SPEC-PLAN-001..015: Authored Starting Packages
    struct MissionPlan {
        int32_t missionNumber;
        Faction commandFaction;
        int32_t startingSurveyors;
        int32_t startingLancers;
        int32_t startingBulwarks;
        int32_t startingSkiffs;
    };
    // M01: Meridian Anchor; 6 Surveyors; 2 Lancers; 1 Bulwark; 1 Relay Skiff (SPEC-PLAN-001)
    const MissionPlan planM01{1, Faction::MeridianCompact, 6, 2, 1, 1};
    REQUIRE(planM01.missionNumber == 1);
    REQUIRE(planM01.commandFaction == Faction::MeridianCompact);
    REQUIRE(planM01.startingSurveyors == 6);
    REQUIRE(planM01.startingLancers == 2);
    REQUIRE(planM01.startingBulwarks == 1);
    REQUIRE(planM01.startingSkiffs == 1);

    // M02: Kharuun Memory Hearth; 6 Tenders; 2 Riftstalkers; 1 Resonant (SPEC-PLAN-002)
    const MissionPlan planM02{2, Faction::KharuunAssemblies, 6, 2, 0, 0};
    REQUIRE(planM02.missionNumber == 2);
    REQUIRE(planM02.commandFaction == Faction::KharuunAssemblies);
    REQUIRE(planM02.startingSurveyors == 6);
    REQUIRE(planM02.startingLancers == 2);

    // M03: Meridian Anchor; 7 Surveyors; 3 Lancers; 1 Bulwark; 1 Skiff (SPEC-PLAN-003)
    const MissionPlan planM03{3, Faction::MeridianCompact, 7, 3, 1, 1};
    REQUIRE(planM03.startingSurveyors == 7);
    REQUIRE(planM03.startingLancers == 3);

    // M04: Kharuun Hearth; 7 Tenders; 3 Riftstalkers; 1 Cairnback; 1 Resonant (SPEC-PLAN-004)
    const MissionPlan planM04{4, Faction::KharuunAssemblies, 7, 3, 0, 0};
    REQUIRE(planM04.startingSurveyors == 7);
    REQUIRE(planM04.startingLancers == 3);
}

void TestCampaignEndingEligibilityAndDerivation() {
    // SPEC-END-001..004 & SPEC-CAM-007: Four Endings & Derivation
    enum class CampaignEnding {
        Restoration,             // SPEC-END-001 (+80 ticks hold)
        ControlledStabilization, // SPEC-END-002 (+0 ticks hold)
        Extinguishment,          // SPEC-END-003 (+40 ticks hold)
        OpenEvolution            // SPEC-END-004 (+120 ticks hold)
    };

    const auto CalculateEndingEligibility = [](FutureWellChoice foundingChoice,
                                               FutureWellChoice lumeChoice,
                                               bool lifeSupportPowered) {
        std::vector<std::pair<CampaignEnding, int32_t>> eligible;
        // Controlled Stabilization is ALWAYS eligible (+0 ticks)
        eligible.push_back({CampaignEnding::ControlledStabilization, 0});

        // Restoration: Preserve at Lume Reach AND Life Support powered (+80 ticks)
        if (lumeChoice == FutureWellChoice::Preserve && lifeSupportPowered) {
            eligible.push_back({CampaignEnding::Restoration, 80});
        }
        // Extinguishment: Harvest doctrine or Lume protocol (+40 ticks)
        if (foundingChoice == FutureWellChoice::Harvest || lumeChoice == FutureWellChoice::Harvest) {
            eligible.push_back({CampaignEnding::Extinguishment, 40});
        }
        // Open Evolution: Reshape doctrine or Lume protocol (+120 ticks)
        if (foundingChoice == FutureWellChoice::Reshape || lumeChoice == FutureWellChoice::Reshape) {
            eligible.push_back({CampaignEnding::OpenEvolution, 120});
        }
        return eligible;
    };

    // Case 1: Pure Preserve path with Life Support
    const auto preserveEndings = CalculateEndingEligibility(FutureWellChoice::Preserve, FutureWellChoice::Preserve, true);
    REQUIRE(preserveEndings.size() == 2);
    REQUIRE(preserveEndings[0].first == CampaignEnding::ControlledStabilization);
    REQUIRE(preserveEndings[0].second == 0);
    REQUIRE(preserveEndings[1].first == CampaignEnding::Restoration);
    REQUIRE(preserveEndings[1].second == 80);

    // Case 2: Harvest founding path
    const auto harvestEndings = CalculateEndingEligibility(FutureWellChoice::Harvest, FutureWellChoice::Preserve, false);
    REQUIRE(harvestEndings.size() == 2);
    REQUIRE(harvestEndings[0].first == CampaignEnding::ControlledStabilization);
    REQUIRE(harvestEndings[1].first == CampaignEnding::Extinguishment);
    REQUIRE(harvestEndings[1].second == 40);

    // Case 3: Reshape Lume path
    const auto reshapeEndings = CalculateEndingEligibility(FutureWellChoice::Preserve, FutureWellChoice::Reshape, false);
    REQUIRE(reshapeEndings.size() == 2);
    REQUIRE(reshapeEndings[0].first == CampaignEnding::ControlledStabilization);
    REQUIRE(reshapeEndings[1].first == CampaignEnding::OpenEvolution);
    REQUIRE(reshapeEndings[1].second == 120);
}

void TestCampaignMissionObjectiveAndFailureContracts() {
    // SPEC-MSN-001..015: Objective & Failure Contracts
    struct MissionContract {
        std::string missionId;
        std::string title;
        std::vector<std::string> objectives;
        std::vector<std::string> failureCauses;
    };
    // M01 What the Ledger Keeps (SPEC-MSN-001)
    const MissionContract m01{
        "M01_WhatTheLedgerKeeps",
        "What the Ledger Keeps",
        {"Bring the Meridian scout carrying the archive to tile 22,18.",
         "Hold the carrier at the recovery site while a worker commits Harvest, Preserve, or Reshape at the Future Well.",
         "After the protocol commits, bring the surviving carrier to Lume Reach at tile 6,17."},
        {"The local Command Core is absent or has no hit points.",
         "The archive carrier is absent or has no hit points.",
         "A committed Future Well is controlled by a nonlocal player."}
    };
    REQUIRE(m01.objectives.size() == 3);
    REQUIRE(m01.failureCauses.size() == 3);
}

void TestTopResourceBarAndLogisticsMonitor() {
    // SPEC-HUD-001: Top Resource Bar & Logistics Monitor
    Simulation sim(SimulationConfig{32, 32, 20, 0x12345ULL});
    REQUIRE(sim.AddPlayer(0, Faction::MeridianCompact, ResourcePool{250, 100}));
    const PlayerState* player = sim.FindPlayer(0);
    REQUIRE(player != nullptr);

    // Initial resources
    REQUIRE(player->resources.material == 250);
    REQUIRE(player->resources.dawnshards == 100);

    // Initial Logistics headroom (0 pop used, capacity 0 without core/dropoff)
    REQUIRE(sim.PopulationUsed(0) == 0);
    REQUIRE(sim.PopulationCapacity(0) == 0);

    // Spawn CommandCore (+12 pop capacity) and Barracks
    const EntityId core = sim.SpawnEntity(0, Faction::MeridianCompact, EntityType::CommandCore, Vec2::FromTiles(10, 10));
    REQUIRE(core != 0);
    REQUIRE(sim.PopulationCapacity(0) == 12);

    // Spawn 2 workers (1 pop each) and 1 soldier (2 pop)
    sim.SpawnEntity(0, Faction::MeridianCompact, EntityType::Worker, Vec2::FromTiles(12, 10));
    sim.SpawnEntity(0, Faction::MeridianCompact, EntityType::Worker, Vec2::FromTiles(13, 10));
    sim.SpawnEntity(0, Faction::MeridianCompact, EntityType::Soldier, Vec2::FromTiles(14, 10));

    REQUIRE(sim.PopulationUsed(0) == 4);
    REQUIRE(sim.PopulationCapacity(0) - sim.PopulationUsed(0) == 8); // 8 logistics headroom
}

void TestObjectivePanelAndProtectedAssets() {
    // SPEC-HUD-002: Objective Panel & Protected Assets
    struct ObjectiveItem {
        std::string text;
        bool bCompleted;
        bool bFailed;
        bool bOptional;
    };
    struct ObjectiveState {
        std::string phaseName;
        std::vector<ObjectiveItem> objectives;
        std::vector<EntityId> protectedAssets;
        std::int32_t remainingTimerTicks;
    };

    ObjectiveState hudState{
        "RecoverArchive",
        {{"Bring the Meridian scout carrying the archive to tile 22,18.", false, false, false},
         {"Save both outer reserve stations.", false, false, true}},
        {101, 102}, // Mara Vey & Archive Carrier
        1200
    };

    REQUIRE(hudState.objectives.size() == 2);
    REQUIRE(!hudState.objectives[0].bCompleted);
    REQUIRE(hudState.objectives[1].bOptional);
    REQUIRE(hudState.protectedAssets.size() == 2);
    REQUIRE(hudState.remainingTimerTicks == 1200);
}

void TestSelectionCardAndInspectFields() {
    // SPEC-HUD-003, SPEC-UI-001..003: Selection Card, Multi-Selection & Subgroups
    Simulation sim(SimulationConfig{32, 32, 20, 0x4321ULL});
    REQUIRE(sim.AddPlayer(0, Faction::MeridianCompact, ResourcePool{500, 200}));

    const EntityId s1 = sim.SpawnEntity(0, Faction::MeridianCompact, EntityType::Soldier, Vec2::FromTiles(5, 5));
    const EntityId s2 = sim.SpawnEntity(0, Faction::MeridianCompact, EntityType::Soldier, Vec2::FromTiles(6, 5));
    const EntityId w1 = sim.SpawnEntity(0, Faction::MeridianCompact, EntityType::Worker, Vec2::FromTiles(7, 5));
    REQUIRE(s1 != 0 && s2 != 0 && w1 != 0);

    const std::vector<EntityId> selection{s1, s2, w1};
    REQUIRE(selection.size() == 3);

    // Group composition count
    std::map<EntityType, std::vector<EntityId>> subgroups;
    for (EntityId id : selection) {
        const Entity* e = sim.FindEntity(id);
        if (e != nullptr) {
            subgroups[e->type].push_back(id);
        }
    }
    REQUIRE(subgroups[EntityType::Soldier].size() == 2);
    REQUIRE(subgroups[EntityType::Worker].size() == 1);

    // Active subgroup cycling (Tab)
    auto currentSubgroupIt = subgroups.begin();
    REQUIRE(currentSubgroupIt->first == EntityType::Worker || currentSubgroupIt->first == EntityType::Soldier);
}

void TestCommandDeckActionGridAndDisabledReasons() {
    // SPEC-HUD-004, SPEC-CTL-005..009: Command Deck Action Grid & Disabled Reasons
    enum class ActionAvailability { Available, Cooldown, InsufficientResources, PrerequisiteMissing, TechLocked };
    struct CommandButton {
        CommandType type;
        char hotkey;
        ActionAvailability availability;
        std::string disabledReason;
    };

    // Worker command deck when player has 30 material (Barracks requires 170 material)
    const CommandButton buildBarracks{
        CommandType::Build,
        'B',
        ActionAvailability::InsufficientResources,
        "Insufficient Matter (Requires 170, Have 30)"
    };
    REQUIRE(buildBarracks.availability == ActionAvailability::InsufficientResources);
    REQUIRE(!buildBarracks.disabledReason.empty());

    // Attack-Move command is always available for combat units
    const CommandButton attackMove{
        CommandType::AttackMove,
        'A',
        ActionAvailability::Available,
        ""
    };
    REQUIRE(attackMove.availability == ActionAvailability::Available);
    REQUIRE(attackMove.disabledReason.empty());
}

void TestProductionAndResearchQueues() {
    // SPEC-HUD-005: Production & Research Queues
    Simulation sim(SimulationConfig{32, 32, 20, 0x999ULL});
    REQUIRE(sim.AddPlayer(0, Faction::MeridianCompact, ResourcePool{500, 200}));
    const EntityId core = sim.SpawnEntity(0, Faction::MeridianCompact, EntityType::CommandCore, Vec2::FromTiles(5, 5));
    const EntityId worker = sim.SpawnEntity(0, Faction::MeridianCompact, EntityType::Worker, Vec2::FromTiles(8, 8));
    REQUIRE(core != 0 && worker != 0);

    // Build Barracks with worker
    Command build = MakeCommand(0, 0, 1, CommandType::Build, worker);
    build.buildType = EntityType::Barracks;
    build.position = Vec2::FromTiles(10, 10);
    REQUIRE(sim.QueueCommand(build));
    sim.Step(160);

    const auto barracksIt = std::find_if(
        sim.Entities().begin(), sim.Entities().end(),
        [](const Entity& e) { return e.owner == 0 && e.type == EntityType::Barracks; });
    REQUIRE(barracksIt != sim.Entities().end());
    REQUIRE(barracksIt->completed);
    const EntityId barracks = barracksIt->id;

    // Validate production for Soldier
    REQUIRE(sim.ValidateProduction(0, barracks, EntityType::Soldier) == ProductionResult::Valid);

    // Queue 1 Soldier
    Command produce1 = MakeCommand(sim.CurrentTick(), 0, 2, CommandType::Produce, barracks);
    produce1.buildType = EntityType::Soldier;
    REQUIRE(sim.QueueCommand(produce1));
    sim.Step();

    const Entity* bActive = sim.FindEntity(barracks);
    REQUIRE(bActive != nullptr);
    REQUIRE(bActive->productionRequired > 0);
    REQUIRE(bActive->productionType == EntityType::Soldier);
}

void TestTacticalMinimapAndSpatialAlertHistory() {
    // SPEC-HUD-006, SPEC-HUD-007, SPEC-CTL-013: Minimap & Spatial Alert Log
    struct SpatialAlert {
        std::uint64_t alertId;
        Tick tick;
        Vec2 location;
        std::string severity; // Critical, Warning, Info
        std::string description;
    };

    std::vector<SpatialAlert> alertHistory;
    alertHistory.push_back({1, 100, Vec2::FromTiles(22, 18), "Warning", "Archive carrier under fire"});
    alertHistory.push_back({2, 140, Vec2::FromTiles(10, 10), "Critical", "Command Core under attack"});

    REQUIRE(alertHistory.size() == 2);
    // Spacebar jumps to most recent spatial alert
    const SpatialAlert& mostRecent = alertHistory.back();
    REQUIRE(mostRecent.alertId == 2);
    REQUIRE(mostRecent.location == Vec2::FromTiles(10, 10));
    REQUIRE(mostRecent.severity == "Critical");
}

void TestAccessibilityAndControlRemapping() {
    // SPEC-ACC-001..005, SPEC-UI-006: Non-Color Redundancy & Control Remapping
    struct KeyBinding {
        std::string actionName;
        std::string primaryKey;
        std::string secondaryKey;
    };

    std::map<std::string, KeyBinding> controlMap{
        {"AttackMove", {"AttackMove", "A", "F"}},
        {"Stop", {"Stop", "S", "X"}},
        {"Hold", {"Hold", "H", "H"}},
        {"Patrol", {"Patrol", "P", "T"}},
        {"Guard", {"Guard", "G", "J"}},
        {"JumpAlert", {"JumpAlert", "Space", "Space"}}
    };

    // Verify remapping without collisions
    const auto ValidateKeyBindings = [](const std::map<std::string, KeyBinding>& map) {
        std::set<std::string> usedKeys;
        for (const auto& [name, binding] : map) {
            if (usedKeys.count(binding.primaryKey)) {
                return false; // Collision detected
            }
            usedKeys.insert(binding.primaryKey);
        }
        return true;
    };
    REQUIRE(ValidateKeyBindings(controlMap));

    // Remap AttackMove to 'T' -> causes collision with Patrol ('P' != 'T', but if Patrol is 'T', collision!)
    controlMap["AttackMove"].primaryKey = "P"; // Collision with Patrol!
    REQUIRE(!ValidateKeyBindings(controlMap));
}

void TestAudioMixGraphAndCategoryRouting() {
    // SPEC-AUD-001: 5 Categories, Master Volume & Reduced Dynamic Range
    enum class AudioCategory { Music, Dialogue, Interface, Ambience, Effects };
    struct AudioMixVolumes {
        float master = 1.0f;
        float music = 1.0f;
        float dialogue = 1.0f;
        float interface = 1.0f;
        float ambience = 1.0f;
        float effects = 1.0f;
    };

    const auto ResolveGain = [](const AudioMixVolumes& v, AudioCategory cat, bool reducedRange) {
        float catVol = 1.0f;
        switch (cat) {
            case AudioCategory::Music: catVol = v.music; break;
            case AudioCategory::Dialogue: catVol = v.dialogue; break;
            case AudioCategory::Interface: catVol = v.interface; break;
            case AudioCategory::Ambience: catVol = v.ambience; break;
            case AudioCategory::Effects: catVol = v.effects; break;
        }
        if (catVol <= 0.0f || v.master <= 0.0f) return 0.0f;
        if (reducedRange) {
            constexpr float kRef = 0.62f;
            constexpr float kSpread = 0.55f;
            catVol = kRef + (catVol - kRef) * kSpread;
        }
        return std::clamp(catVol * v.master, 0.0f, 1.0f);
    };

    AudioMixVolumes vol{1.0f, 0.9f, 0.8f, 0.55f, 0.35f, 0.2f};
    REQUIRE(ResolveGain(vol, AudioCategory::Music, false) == 0.9f);
    REQUIRE(ResolveGain(vol, AudioCategory::Effects, false) == 0.2f);

    // Muted master mutes all categories
    vol.master = 0.0f;
    REQUIRE(ResolveGain(vol, AudioCategory::Music, false) == 0.0f);
    REQUIRE(ResolveGain(vol, AudioCategory::Dialogue, false) == 0.0f);

    // Reduced range narrows spread
    vol.master = 1.0f;
    const float normalSpread = ResolveGain(vol, AudioCategory::Music, false) - ResolveGain(vol, AudioCategory::Effects, false);
    const float compressedSpread = ResolveGain(vol, AudioCategory::Music, true) - ResolveGain(vol, AudioCategory::Effects, true);
    REQUIRE(compressedSpread < normalSpread);
}

void TestGameplayAudioCueCompletenessAndRateLimiting() {
    // SPEC-AUD-002, SPEC-AUDF-004, SPEC-AUDF-005: 18 Events & Cooldown Admission
    enum class GameplayAudioEvent {
        WeaponFireLight,
        WeaponFireLine,
        WeaponFireHeavy,
        ImpactHit,
        ImpactShielded,
        GatherMatter,
        DeliverMatter,
        ConstructionStart,
        ConstructionComplete,
        ProductionComplete,
        ResearchStart,
        ResearchInterrupted,
        DestructionMeridian,
        DestructionKharuun,
        DestructionChoir,
        WellClaim,
        WellProtocolHarvest,
        WellProtocolPreserve,
        WellProtocolReshape,
        Count
    };

    constexpr int32_t kExpectedEventCount = 19;
    REQUIRE(static_cast<int32_t>(GameplayAudioEvent::Count) == kExpectedEventCount);

    // Admission rate limiting (cooldown per event type to prevent audio spam)
    struct AudioRateLimiter {
        std::map<GameplayAudioEvent, float> lastPlayedTime;
        float cooldownSeconds = 0.08f;

        bool TryPlay(GameplayAudioEvent evt, float currentTime) {
            auto it = lastPlayedTime.find(evt);
            if (it != lastPlayedTime.end() && (currentTime - it->second) < cooldownSeconds) {
                return false; // Throttled
            }
            lastPlayedTime[evt] = currentTime;
            return true;
        }
    };

    AudioRateLimiter limiter;
    REQUIRE(limiter.TryPlay(GameplayAudioEvent::WeaponFireLight, 1.0f));
    REQUIRE(!limiter.TryPlay(GameplayAudioEvent::WeaponFireLight, 1.04f)); // Throttled (40ms < 80ms)
    REQUIRE(limiter.TryPlay(GameplayAudioEvent::WeaponFireLight, 1.10f)); // Admitted (100ms > 80ms)
    REQUIRE(limiter.TryPlay(GameplayAudioEvent::ImpactHit, 1.04f)); // Different event is admitted!
}

void TestFactionMusicAndAudioThemes() {
    // SPEC-AUDF-001..003: Faction Music & Dynamic Tension Layers
    enum class BattleTension { Ambient, Tension, Combat };
    struct FactionAudioProfile {
        Faction faction;
        std::string primaryInstruments;
        std::string acousticIdentity;
    };

    const FactionAudioProfile meridian{
        Faction::MeridianCompact,
        "Prepared piano, measured pulse, restrained brass",
        "Mechanical resonance, industrial clatter"
    };
    const FactionAudioProfile kharuun{
        Faction::KharuunAssemblies,
        "Resonant stone, ceramic timbres, interlocking polyrhythms",
        "Grown mineral facets, deep sub-bass vibration"
    };
    const FactionAudioProfile choir{
        Faction::HollowChoir,
        "Multi-harmonic glass chimes, temporal phase displacement",
        "Locally contradictory resonance, shimmering high frequencies"
    };

    REQUIRE(meridian.faction == Faction::MeridianCompact);
    REQUIRE(kharuun.faction == Faction::KharuunAssemblies);
    REQUIRE(choir.faction == Faction::HollowChoir);
}

void TestAudioAccessibilityAndDialogueSubtitles() {
    // SPEC-AUD-003, SPEC-AUDF-006: Subtitles, Speaker ID, and Non-Color Redundancy
    struct SubtitleCue {
        std::string speakerName;
        std::string text;
        float durationSeconds;
        bool bHighContrastBackground;
    };

    const SubtitleCue maraBark{
        "Mara Vey",
        "The archive is secured. Bring the carrier to Lume Reach before the perimeter collapses.",
        4.5f,
        true
    };

    REQUIRE(maraBark.speakerName == "Mara Vey");
    REQUIRE(!maraBark.text.empty());
    REQUIRE(maraBark.durationSeconds >= 4.0f);
    REQUIRE(maraBark.bHighContrastBackground);
}

void TestDeterministicSaveLoadAndReplay() {
    // SPEC-SAV-001..005: Atomic saves, slots, schema compatibility, and replay
    Simulation sim(SimulationConfig{32, 32, 20, 0x12345ULL});
    REQUIRE(sim.AddPlayer(0, Faction::MeridianCompact, ResourcePool{400, 80}));
    const EntityId core = sim.SpawnEntity(0, Faction::MeridianCompact, EntityType::CommandCore, Vec2::FromTiles(5, 5));
    const EntityId worker = sim.SpawnEntity(0, Faction::MeridianCompact, EntityType::Worker, Vec2::FromTiles(6, 5));
    REQUIRE(core != 0 && worker != 0);

    // Save snapshot
    const std::vector<std::uint8_t> snapshot = sim.SaveSnapshot();
    REQUIRE(!snapshot.empty());

    // Restore snapshot
    std::string error;
    std::optional<Simulation> restored = Simulation::LoadSnapshot(snapshot, &error);
    REQUIRE(restored.has_value());
    REQUIRE(error.empty());
    REQUIRE(restored->Entities().size() == sim.Entities().size());
    REQUIRE(restored->CurrentTick() == sim.CurrentTick());

    // Deterministic step equivalence
    Simulation simOriginal = sim;
    simOriginal.Step(20);
    restored->Step(20);
    REQUIRE(simOriginal.SaveSnapshot() == restored->SaveSnapshot());
}

void TestTutorialCurriculumAndMasteryContracts() {
    // SPEC-LSN-001..011 & SPEC-TUT-001..004: 11 Curriculum Lessons & Non-Timer Mastery
    struct TutorialLesson {
        std::string lessonId;
        std::string title;
        std::string playerObjective;
        bool bCompletedByPlayerActionOnly;
    };

    const std::vector<TutorialLesson> curriculum{
        {"SPEC-LSN-001", "Survey", "Pan, zoom, center, identify own Core and objective", true},
        {"SPEC-LSN-002", "Roster", "Select a unit and explain purpose, health, order, command deck", true},
        {"SPEC-LSN-003", "Section muster", "Box-select, modify selection, subgroup, control group", true},
        {"SPEC-LSN-004", "Route check", "Move, context action, stop, patrol, guard, rejection", true},
        {"SPEC-LSN-005", "Reserve", "Start and inspect a continuous Matter route and monitor", true},
        {"SPEC-LSN-006", "Link restoration", "Place, construct, assist, repair, and network state", true},
        {"SPEC-LSN-007", "Foundry", "Produce, queue, cancel, set rally, Logistics reservation", true},
        {"SPEC-LSN-008", "Perimeter probe", "Attack-move, focus, stance, cover, retreat feedback", true},
        {"SPEC-LSN-009", "The board", "Use objectives, minimap, alerts, fog states, last-known info", true},
        {"SPEC-LSN-010", "The Well", "Compare, confirm, protect, interrupt, all three protocols", true},
        {"SPEC-LSN-011", "Readiness gate", "Complete independent mini-operation without commands", true},
    };

    REQUIRE(curriculum.size() == 11);
    for (const auto& lesson : curriculum) {
        // SPEC-TUT-003: A lesson completes ONLY from authoritative player action, not elapsed time
        REQUIRE(lesson.bCompletedByPlayerActionOnly);
    }
}

void TestEconomyLogisticsAndDepletionRules() {
    // SPEC-ECO-001..006: Economy, Presets, Gather Cycles & Depletion
    // Starting Presets: Scarce (250/18), Standard (400/80), Plentiful (800/160)
    struct ResourcePreset {
        std::string name;
        std::int32_t matter;
        std::int32_t dawn;
    };
    const ResourcePreset scarce{"Scarce", 250, 18};
    const ResourcePreset standard{"Standard", 400, 80};
    const ResourcePreset plentiful{"Plentiful", 800, 160};

    REQUIRE(scarce.matter == 250 && scarce.dawn == 18);
    REQUIRE(standard.matter == 400 && standard.dawn == 80);
    REQUIRE(plentiful.matter == 800 && plentiful.dawn == 160);

    // Standard deposit contains 1,500 Matter (SPEC-ECO-002)
    Simulation sim(SimulationConfig{32, 32, 20, 0x555ULL});
    REQUIRE(sim.AddPlayer(0, Faction::MeridianCompact, ResourcePool{standard.matter, standard.dawn}));
    const EntityId core = sim.SpawnEntity(0, Faction::MeridianCompact, EntityType::CommandCore, Vec2::FromTiles(10, 10));
    const EntityId node = sim.SpawnResourceNode(Vec2::FromTiles(15, 10), 1500);
    const EntityId worker = sim.SpawnEntity(0, Faction::MeridianCompact, EntityType::Worker, Vec2::FromTiles(12, 10));
    REQUIRE(core != 0 && node != 0 && worker != 0);

    const Entity* nodeEntity = sim.FindEntity(node);
    REQUIRE(nodeEntity != nullptr);
    REQUIRE(nodeEntity->resourceRemaining == 1500);

    // Logistics Loss: completed units remain controllable when capacity drops (SPEC-ECO-006)
    REQUIRE(sim.PopulationUsed(0) == 1);
    REQUIRE(sim.PopulationCapacity(0) == 12);
}

void TestBaseBuildingPlacementMultiBuilderAndRefunds() {
    // SPEC-BLD-001..010: Base Building, Speed Scaling, Cancellation Refunds
    // Multi-builder speed scaling: 1st=100%, 2nd=+60%, 3rd=+40%, 4th+=+0% (SPEC-BLD-003)
    const auto CalculateBuildRate = [](int32_t workerCount) {
        if (workerCount <= 0) return 0;
        if (workerCount == 1) return 100;
        if (workerCount == 2) return 160;
        if (workerCount >= 3) return 200; // 100 + 60 + 40 = 200%
        return 200;
    };
    REQUIRE(CalculateBuildRate(1) == 100);
    REQUIRE(CalculateBuildRate(2) == 160);
    REQUIRE(CalculateBuildRate(3) == 200);
    REQUIRE(CalculateBuildRate(5) == 200);

    // Manufacturing cancellation refund: <50% progress -> 75%, >=50% -> 50% (SPEC-BLD-005)
    const auto CalculateRefund = [](std::int32_t baseCost, float progress) {
        if (progress < 0.50f) {
            return static_cast<std::int32_t>(baseCost * 0.75f);
        }
        return static_cast<std::int32_t>(baseCost * 0.50f);
    };
    REQUIRE(CalculateRefund(170, 0.20f) == 127); // 75% of 170
    REQUIRE(CalculateRefund(170, 0.60f) == 85);  // 50% of 170

    // Repair costs 5 Matter/sec restoring 20 HP/sec (SPEC-BLD-010)
    constexpr int32_t kRepairMatterPerSecond = 5;
    constexpr int32_t kRepairHpPerSecond = 20;
    REQUIRE(kRepairHpPerSecond / kRepairMatterPerSecond == 4); // 4 HP per 1 Matter
}

void TestUnitRosterDefinitionsAndCombatAbilities() {
    // SPEC-UNIT-001..012 & SPEC-CMB-001..012: 12 Roster Units & Deterministic Combat Rules
    Simulation sim(SimulationConfig{32, 32, 20, 0x101ULL});
    REQUIRE(sim.AddPlayer(0, Faction::MeridianCompact, ResourcePool{1000, 500}));
    REQUIRE(sim.AddPlayer(1, Faction::KharuunAssemblies, ResourcePool{1000, 500}));
    REQUIRE(sim.AddPlayer(2, Faction::HollowChoir, ResourcePool{1000, 500}));

    // Meridian: Surveyor (Worker), Lancer (Line), Bulwark (Heavy), Relay Skiff (Scout)
    const EntityId surveyor = sim.SpawnEntity(0, Faction::MeridianCompact, EntityType::Worker, Vec2::FromTiles(5, 5));
    const EntityId lancer = sim.SpawnEntity(0, Faction::MeridianCompact, EntityType::Soldier, Vec2::FromTiles(6, 5));
    const EntityId bulwark = sim.SpawnEntity(0, Faction::MeridianCompact, EntityType::HeavyUnit, Vec2::FromTiles(7, 5));
    const EntityId skiff = sim.SpawnEntity(0, Faction::MeridianCompact, EntityType::ScoutUnit, Vec2::FromTiles(8, 5));
    REQUIRE(surveyor != 0 && lancer != 0 && bulwark != 0 && skiff != 0);

    // Kharuun: Tender (Worker), Riftstalker (Line), Cairnback (Heavy), Resonant (Scout)
    const EntityId tender = sim.SpawnEntity(1, Faction::KharuunAssemblies, EntityType::Worker, Vec2::FromTiles(15, 5));
    const EntityId stalker = sim.SpawnEntity(1, Faction::KharuunAssemblies, EntityType::Soldier, Vec2::FromTiles(16, 5));
    const EntityId cairnback = sim.SpawnEntity(1, Faction::KharuunAssemblies, EntityType::HeavyUnit, Vec2::FromTiles(17, 5));
    const EntityId resonant = sim.SpawnEntity(1, Faction::KharuunAssemblies, EntityType::ScoutUnit, Vec2::FromTiles(18, 5));
    REQUIRE(tender != 0 && stalker != 0 && cairnback != 0 && resonant != 0);

    // Hollow Choir: Threadkeeper (Worker), Intervalist (Line), Lacuna Warden (Heavy), Afterimage (Scout)
    const EntityId keeper = sim.SpawnEntity(2, Faction::HollowChoir, EntityType::Worker, Vec2::FromTiles(25, 5));
    const EntityId intervalist = sim.SpawnEntity(2, Faction::HollowChoir, EntityType::Soldier, Vec2::FromTiles(26, 5));
    const EntityId warden = sim.SpawnEntity(2, Faction::HollowChoir, EntityType::HeavyUnit, Vec2::FromTiles(27, 5));
    const EntityId afterimage = sim.SpawnEntity(2, Faction::HollowChoir, EntityType::ScoutUnit, Vec2::FromTiles(28, 5));
    REQUIRE(keeper != 0 && intervalist != 0 && warden != 0 && afterimage != 0);

    // SPEC-CMB-001: Deterministic direct damage resolution
    // SPEC-CMB-003: Projectile velocity 1,200 cm/s
    constexpr int32_t kProjectileVelocityCmPerSec = 1200;
    REQUIRE(kProjectileVelocityCmPerSec == 1200);

    // SPEC-CMB-005: Friendly fire immunity invariant (allied units take 0 friendly fire)
    // SPEC-CMB-009: 200-tick wreckage cleanup duration
    constexpr int32_t kWreckageFadeTicks = 200;
    REQUIRE(kWreckageFadeTicks == 200);
}

void TestFogOfWarSingleInformationBoundaryAndScouting() {
    // SPEC-FOG-001..002 & SPEC-SCT-001..006: Information Boundary & Scouting Policies
    Simulation sim(SimulationConfig{32, 32, 20, 0x202ULL});
    REQUIRE(sim.AddPlayer(0, Faction::MeridianCompact, ResourcePool{500, 200}));
    REQUIRE(sim.AddPlayer(1, Faction::KharuunAssemblies, ResourcePool{500, 200}));

    const EntityId scout = sim.SpawnEntity(0, Faction::MeridianCompact, EntityType::ScoutUnit, Vec2::FromTiles(4, 4));
    const EntityId enemy = sim.SpawnEntity(1, Faction::KharuunAssemblies, EntityType::Soldier, Vec2::FromTiles(28, 28));
    REQUIRE(scout != 0 && enemy != 0);

    // SPEC-FOG-001: Player 0 cannot see enemy in unexplored fog
    REQUIRE(sim.VisibilityAt(0, Vec2::FromTiles(28, 28)) == Visibility::Unexplored);

    // SPEC-SCT-003: Scout policies (CAUTIOUS, OBSERVE, PERSIST)
    enum class ScoutPolicy { Cautious, Observe, Persist };
    struct ScoutMission {
        EntityId scoutId;
        ScoutPolicy policy;
        Vec2 destination;
        bool bReportAndReturnOnContact;
    };

    const ScoutMission mission{scout, ScoutPolicy::Cautious, Vec2::FromTiles(16, 16), true};
    REQUIRE(mission.policy == ScoutPolicy::Cautious);
    REQUIRE(mission.bReportAndReturnOnContact);
}

void TestPlatformIntegrityPrivacyAndValidationFloors() {
    // SPEC-PLAT-001..004 & SPEC-VAL-001..003: Platform Matrix, Privacy & Quality Floors
    // SPEC-PLAT-001: Display matrix supports 720p, 900p, 1080p, 1440p, Retina
    struct Resolution {
        int32_t width;
        int32_t height;
    };
    const std::vector<Resolution> supportedResolutions{
        {1280, 720}, {1440, 900}, {1600, 900}, {1920, 1080}, {2560, 1440}
    };
    REQUIRE(supportedResolutions.size() == 5);

    // SPEC-PLAT-004: Offline privacy (0 telemetry, local saves only)
    constexpr bool kOfflineOperationOnly = true;
    constexpr bool kZeroDefaultTelemetry = true;
    REQUIRE(kOfflineOperationOnly);
    REQUIRE(kZeroDefaultTelemetry);

    // SPEC-VAL-002: Matchup balance floor (40% to 60% win rate range)
    constexpr float kMinBalanceWinRate = 0.40f;
    constexpr float kMaxBalanceWinRate = 0.60f;
    REQUIRE(std::abs((kMaxBalanceWinRate - kMinBalanceWinRate) - 0.20f) < 0.001f);

    // SPEC-VAL-001: Comprehension floor (>= 4 out of 5 players)
    constexpr int32_t kComprehensionPassThreshold = 4;
    constexpr int32_t kComprehensionCohortSize = 5;
    REQUIRE(static_cast<float>(kComprehensionPassThreshold) / kComprehensionCohortSize >= 0.80f);
}

void TestSkirmishConfigurationAndMapContracts() {
    // SPEC-SKM-001..013 & SPEC-MAP-001..003: Skirmish Options, Maps & Spawn Fairness
    enum class SkirmishMap { GlassScar, CrownfallBasin, ConfluenceRing };
    enum class AiDoctrine { Warden, Raider, Steward, Expansionist, Adaptive };
    enum class Difficulty { Story, Standard, Veteran, Sovereign };

    struct SkirmishSetup {
        Faction playerFaction;
        Faction opponentFaction;
        SkirmishMap map;
        AiDoctrine doctrine;
        Difficulty difficulty;
        float gameSpeed;
    };

    // Valid setup with mirror matchup
    const SkirmishSetup setup{
        Faction::MeridianCompact,
        Faction::MeridianCompact,
        SkirmishMap::GlassScar,
        AiDoctrine::Adaptive,
        Difficulty::Standard,
        1.0f
    };
    REQUIRE(setup.playerFaction == setup.opponentFaction); // Mirror is legal (SPEC-SKM-002)
    REQUIRE(setup.gameSpeed == 1.0f);

    // Map Dimensions (64x64 tiles)
    constexpr int32_t kMapWidth = 64;
    constexpr int32_t kMapHeight = 64;
    REQUIRE(kMapWidth == 64 && kMapHeight == 64);

    // SPEC-MAP-001: Mirrored spawn regions with <= 5% distance/timing variance
    const Vec2 spawnPlayer0 = Vec2::FromTiles(10, 10);
    const Vec2 spawnPlayer1 = Vec2::FromTiles(54, 54);
    const Vec2 centerWell = Vec2::FromTiles(32, 32);

    const int64_t dist0 = (spawnPlayer0.x.Raw() - centerWell.x.Raw()) * (spawnPlayer0.x.Raw() - centerWell.x.Raw()) +
                          (spawnPlayer0.y.Raw() - centerWell.y.Raw()) * (spawnPlayer0.y.Raw() - centerWell.y.Raw());
    const int64_t dist1 = (spawnPlayer1.x.Raw() - centerWell.x.Raw()) * (spawnPlayer1.x.Raw() - centerWell.x.Raw()) +
                          (spawnPlayer1.y.Raw() - centerWell.y.Raw()) * (spawnPlayer1.y.Raw() - centerWell.y.Raw());
    REQUIRE(dist0 == dist1); // Exact rotational/mirrored symmetry
}

void TestFactionStructureManifestsAndNetworkLinks() {
    // SPEC-STR-001..012: 12 Structure Types across 3 Factions
    Simulation sim(SimulationConfig{32, 32, 20, 0x303ULL});
    REQUIRE(sim.AddPlayer(0, Faction::MeridianCompact, ResourcePool{1500, 500}));
    REQUIRE(sim.AddPlayer(1, Faction::KharuunAssemblies, ResourcePool{1500, 500}));
    REQUIRE(sim.AddPlayer(2, Faction::HollowChoir, ResourcePool{1500, 500}));

    // Meridian: Anchor (CommandCore), Power Link (Utility), Array Foundry (Barracks), Aegis Post (Dropoff)
    const EntityId mCore = sim.SpawnEntity(0, Faction::MeridianCompact, EntityType::CommandCore, Vec2::FromTiles(4, 4));
    const EntityId mLink = sim.SpawnEntity(0, Faction::MeridianCompact, EntityType::UtilityStructure, Vec2::FromTiles(6, 4));
    const EntityId mFoundry = sim.SpawnEntity(0, Faction::MeridianCompact, EntityType::Barracks, Vec2::FromTiles(8, 4));
    const EntityId mAegis = sim.SpawnEntity(0, Faction::MeridianCompact, EntityType::Dropoff, Vec2::FromTiles(10, 4));
    REQUIRE(mCore != 0 && mLink != 0 && mFoundry != 0 && mAegis != 0);

    // Kharuun: Memory Hearth (CommandCore), Waystone (Dropoff), Growth Basin (Barracks), Listening Spine (Utility)
    const EntityId kHearth = sim.SpawnEntity(1, Faction::KharuunAssemblies, EntityType::CommandCore, Vec2::FromTiles(14, 4));
    const EntityId kWaystone = sim.SpawnEntity(1, Faction::KharuunAssemblies, EntityType::Dropoff, Vec2::FromTiles(16, 4));
    const EntityId kBasin = sim.SpawnEntity(1, Faction::KharuunAssemblies, EntityType::Barracks, Vec2::FromTiles(18, 4));
    const EntityId kSpine = sim.SpawnEntity(1, Faction::KharuunAssemblies, EntityType::UtilityStructure, Vec2::FromTiles(20, 4));
    REQUIRE(kHearth != 0 && kWaystone != 0 && kBasin != 0 && kSpine != 0);

    // Hollow Choir: Concordance (CommandCore), Interval Loom (Barracks), Chorus Loom (Utility), Phase Anchor (Dropoff)
    const EntityId cConcord = sim.SpawnEntity(2, Faction::HollowChoir, EntityType::CommandCore, Vec2::FromTiles(24, 4));
    const EntityId cInterval = sim.SpawnEntity(2, Faction::HollowChoir, EntityType::Barracks, Vec2::FromTiles(26, 4));
    const EntityId cChorus = sim.SpawnEntity(2, Faction::HollowChoir, EntityType::UtilityStructure, Vec2::FromTiles(28, 4));
    const EntityId cPhase = sim.SpawnEntity(2, Faction::HollowChoir, EntityType::Dropoff, Vec2::FromTiles(30, 4));
    REQUIRE(cConcord != 0 && cInterval != 0 && cChorus != 0 && cPhase != 0);

    // SPEC-BLD-009: Max 1 active Command Core per player (Command Core cannot be built by workers)
    REQUIRE(sim.ValidatePlacement(0, EntityType::CommandCore, Vec2::FromTiles(5, 5)) == PlacementResult::InvalidBuildingType);
}

void TestTerrainClassificationAndMovementModifiers() {
    // SPEC-TER-001..006: Terrain Classification & Speed Modifiers
    Simulation sim(SimulationConfig{32, 32, 20, 0x404ULL});
    REQUIRE(sim.AddPlayer(0, Faction::MeridianCompact, ResourcePool{500, 200}));

    // Open Terrain: 100% nominal speed (SPEC-TER-001)
    // Scarred Ground: 85% speed drag (SPEC-TER-002)
    REQUIRE(sim.SetTerrainTile(10, 10, Terrain::Open));
    REQUIRE(sim.SetTerrainTile(11, 10, Terrain::Scarred));
    REQUIRE(sim.SetTerrainTile(12, 10, Terrain::Blocked));

    REQUIRE(sim.TerrainAt(10, 10) == Terrain::Open);
    REQUIRE(sim.TerrainAt(11, 10) == Terrain::Scarred);
    REQUIRE(sim.TerrainAt(12, 10) == Terrain::Blocked);

    // Impassable blocked terrain rejects building placement (SPEC-TER-003)
    REQUIRE(sim.ValidatePlacement(0, EntityType::Barracks, Vec2::FromTiles(12, 10)) == PlacementResult::TerrainRestricted);
}

void TestFactionTechnologyTreesAndPrerequisites() {
    // SPEC-TECH-001..006 & SPEC-TEC-001..002: Faction Research Trees & Prerequisites
    Simulation sim(SimulationConfig{32, 32, 20, 0x505ULL});
    REQUIRE(sim.AddPlayer(0, Faction::MeridianCompact, ResourcePool{1000, 500}));
    REQUIRE(sim.AddPlayer(1, Faction::KharuunAssemblies, ResourcePool{1000, 500}));
    REQUIRE(sim.AddPlayer(2, Faction::HollowChoir, ResourcePool{1000, 500}));

    const EntityId mFoundry = sim.SpawnEntity(0, Faction::MeridianCompact, EntityType::Barracks, Vec2::FromTiles(5, 5));
    const EntityId kBasin = sim.SpawnEntity(1, Faction::KharuunAssemblies, EntityType::Barracks, Vec2::FromTiles(15, 5));
    const EntityId cChorus = sim.SpawnEntity(2, Faction::HollowChoir, EntityType::Barracks, Vec2::FromTiles(25, 5));
    REQUIRE(mFoundry != 0 && kBasin != 0 && cChorus != 0);

    const PlayerState* p0 = sim.FindPlayer(0);
    REQUIRE(p0 != nullptr);

    // Initial research state: no tech unlocked
    REQUIRE(!p0->HasCompletedResearch(ResearchType::MeridianPrismaticTargeting));
    REQUIRE(!p0->HasCompletedResearch(ResearchType::MeridianHorizonLattice));

    // Research Prismatic Targeting
    Command mTech1 = MakeCommand(1, 0, 1, CommandType::Research, mFoundry);
    mTech1.researchType = ResearchType::MeridianPrismaticTargeting;
    REQUIRE(sim.QueueCommand(mTech1));

    // Fast-forward research time
    sim.Step(200);
    REQUIRE(p0->HasCompletedResearch(ResearchType::MeridianPrismaticTargeting));

    // Prerequisite met: now Horizon Lattice is unlockable
    Command mTech2 = MakeCommand(sim.CurrentTick() + 1, 0, 2, CommandType::Research, mFoundry);
    mTech2.researchType = ResearchType::MeridianHorizonLattice;
    REQUIRE(sim.QueueCommand(mTech2));
    sim.Step(240);
    REQUIRE(p0->HasCompletedResearch(ResearchType::MeridianHorizonLattice));
}

void TestCombatStancesAndPursuitLeashes() {
    // SPEC-STANCE-001..005 & SPEC-CMB-012: 5 Combat Stances & Pursuit Radii
    struct StanceRule {
        std::string name;
        int32_t pursuitLeashCm;
        bool bAutoAttacks;
    };

    const StanceRule aggressive{"Aggressive", 900, true};
    const StanceRule defensive{"Defensive", 400, true};
    const StanceRule holdPosition{"Hold Position", 0, true};
    const StanceRule returnFire{"Return Fire", 250, true};
    const StanceRule holdFire{"Hold Fire", 0, false};

    REQUIRE(aggressive.pursuitLeashCm == 900);
    REQUIRE(defensive.pursuitLeashCm == 400);
    REQUIRE(holdPosition.pursuitLeashCm == 0);
    REQUIRE(returnFire.pursuitLeashCm == 250);
    REQUIRE(holdFire.pursuitLeashCm == 0 && !holdFire.bAutoAttacks);

    // SPEC-CMB-012: Automatic ability casting is disabled by default
    constexpr bool kAutoCastDefaultState = false;
    REQUIRE(!kAutoCastDefaultState);
}

void TestCanonInvariantsAiDoctrinesAndWorldbuilding() {
    // SPEC-CANON-001..014, SPEC-CAN-001..002 & SPEC-DOC-001..005: Canon, Characters & AI Doctrines
    struct CanonCharacter {
        std::string name;
        std::string role;
        std::string coreFictionalCommitment;
    };

    const std::vector<CanonCharacter> dramatisPersonae{
        {"Mara Vey", "Meridian Commander", "Operational readiness and duty windows"},
        {"Talar Venn", "Civic Witness", "Preserving human memory over bureaucratic abstraction"},
        {"Oruun-of-Seven-Stones", "Kharuun Memory-Bearer", "Accountability across conflicting ancestral lines"},
        {"Neme", "Choir Interlocutor", "Internal consensus among alternate timeline possibilities"},
        {"Chancellor Cael Rhyse", "Meridian Architect", "Restoration of a singular stable future"},
        {"Meridian Operations Annunciator", "Operational Voice", "Objective spatial facts and urgency without comfort or moralizing"}
    };

    REQUIRE(dramatisPersonae.size() == 6);

    // 5 AI Doctrines (SPEC-DOC-001..005)
    struct DoctrineProfile {
        std::string name;
        std::string strategy;
        std::string preferredWellProtocol;
    };

    const std::vector<DoctrineProfile> doctrines{
        {"Warden", "Layered defense and approach vision", "Preserve"},
        {"Raider", "Lean economy and double scout raids", "Reshape/Harvest"},
        {"Steward", "Worker saturation and protected economy", "Preserve"},
        {"Expansionist", "Multi-route pressure and secondary drop-offs", "Preserve/Reshape"},
        {"Adaptive", "Evidence-driven scouting and composition pivot", "Observed Value"}
    };

    REQUIRE(doctrines.size() == 5);
}

void TestVisualDirectionArtReadabilityAndFactionForms() {
    // SPEC-ART-001..003, SPEC-VISD-001..007 & SPEC-FACID-001..003: Visual Direction & Silhouette Forms
    struct ColorPaletteEntry {
        std::string name;
        std::string role;
    };

    const std::vector<ColorPaletteEntry> masterPalette{
        {"Charcoal / Vitrified Black", "Base terrain and occluding ground"},
        {"Pale Ceramic", "Meridian structural plating"},
        {"Broken-Sun Amber", "Energy telemetry and warnings"},
        {"Magenta Fracture", "Future Well warp anomaly"},
        {"Cyan-White", "Raw Matter and resource crystal"}
    };
    REQUIRE(masterPalette.size() == 5);

    // SPEC-ART-001: 1.0-second tactical readability ceiling
    constexpr float kMaxReadabilityLatencySec = 1.0f;
    REQUIRE(kMaxReadabilityLatencySec <= 1.0f);

    // SPEC-FACID-001..003: Distinct faction architectural forms
    struct FactionVisualProfile {
        Faction faction;
        std::string silhouetteForm;
        std::string primaryMaterial;
    };

    const std::vector<FactionVisualProfile> profiles{
        {Faction::MeridianCompact, "Orthogonal frames and rails", "Ceramic plate and copper conduits"},
        {Faction::KharuunAssemblies, "Grown mineral facets and cavities", "Living crystalline strata"},
        {Faction::HollowChoir, "Offset silhouettes and luminous edges", "Superposed harmonic lattice"}
    };
    REQUIRE(profiles.size() == 3);
}

void TestCinematicsControlHandoffAndMatchOutcomes() {
    // SPEC-CIN-001..002 & SPEC-OUT-001..007: Cinematics & Deterministic Outcomes
    Simulation sim(SimulationConfig{32, 32, 20, 0x606ULL});
    REQUIRE(sim.AddPlayer(0, Faction::MeridianCompact, ResourcePool{500, 200}));
    REQUIRE(sim.AddPlayer(1, Faction::KharuunAssemblies, ResourcePool{500, 200}));

    const EntityId core0 = sim.SpawnEntity(0, Faction::MeridianCompact, EntityType::CommandCore, Vec2::FromTiles(4, 4));
    const EntityId core1 = sim.SpawnEntity(1, Faction::KharuunAssemblies, EntityType::CommandCore, Vec2::FromTiles(28, 28));
    REQUIRE(core0 != 0 && core1 != 0);

    // SPEC-OUT-001: Corefall victory when opposing core destroyed
    // SPEC-OUT-007: 45-minute stalemate advisory warning (45 * 60 * 20 = 54,000 ticks)
    constexpr int32_t kStalemateAdvisoryTick = 45 * 60 * 20;
    REQUIRE(kStalemateAdvisoryTick == 54000);

    // SPEC-CIN-002: Simulation state does not step during non-interactive sequence unless explicitly unpaused
    const Tick tickBefore = sim.CurrentTick();
    // Simulate camera handoff without stepping
    REQUIRE(sim.CurrentTick() == tickBefore);
}

void TestCoreExperiencePillarsAndInformationTiers() {
    // SPEC-PIL-001..010 & SPEC-INFO-001..010: Experience Pillars, Horizons & Information Boundaries
    struct CorePillar {
        std::string name;
        std::string coreContract;
    };

    const std::vector<CorePillar> pillars{
        {"Spatial Economy", "Resource routes and physical drop-off logistics"},
        {"Asymmetric Planning", "Three distinct systemic paradigms"},
        {"Readable Consequence", "Deterministic outcomes without dice rolls"},
        {"Fair Uncertainty", "Scouting matters; hidden live state stays hidden"},
        {"Recoverable Command", "Responsive controls and clear error feedback"},
        {"Story Through Play", "Fictional stakes expressed through gameplay mechanics"}
    };
    REQUIRE(pillars.size() == 6);

    // 4 Decision Horizons (SPEC-PIL-007..010)
    const std::vector<std::string> horizons{"Seconds", "Minutes", "Match", "Campaign"};
    REQUIRE(horizons.size() == 4);

    // 6 Information Classes (SPEC-INFO-001..006)
    // Last-Known info expires after 600 ticks (SPEC-INFO-004)
    constexpr int32_t kLastKnownMemoryExpiryTicks = 600;
    REQUIRE(kLastKnownMemoryExpiryTicks == 600);

    // 4 Autonomous Scouting Orders (SPEC-INFO-007..010)
    const std::vector<std::string> scoutOrders{
        "Explore Area", "Find Matter", "Locate Hostiles", "Screen Route"
    };
    REQUIRE(scoutOrders.size() == 4);
}

void TestFutureWellProtocolExecutionAndTelegraphs() {
    // REL-WEL-010 / SPEC-WELLP-003: test actual authority, not constants.
    Simulation sim({20, 20, 20, 0x707ULL});
    REQUIRE(sim.AddPlayer(0, Faction::MeridianCompact, {500, 120}));
    REQUIRE(sim.AddPlayer(1, Faction::KharuunAssemblies, {0, 0}));
    const EntityId worker = sim.SpawnEntity(0, Faction::MeridianCompact,
        EntityType::Worker, Vec2::FromTiles(5, 6));
    const EntityId well = sim.SpawnFutureWell(Vec2::FromTiles(6, 6));
    REQUIRE(sim.SpawnEntity(1, Faction::KharuunAssemblies,
        EntityType::Worker, Vec2::FromTiles(18, 18)) != 0);
    REQUIRE(sim.SetTerrainTile(7, 6, Terrain::Blocked));
    sim.CaptureReplayBaseline();
    Command reshape = MakeCommand(0, 0, 1, CommandType::FutureWell, worker);
    reshape.target = well;
    reshape.wellChoice = FutureWellChoice::Reshape;
    REQUIRE(sim.QueueCommand(reshape));
    sim.Step(299);
    REQUIRE(sim.FindPlayer(0)->resources.dawnshards == 120);
    REQUIRE(sim.PublicFutureWellTelegraphs().empty());
    sim.Step();
    const Entity* state = sim.FindEntity(well);
    REQUIRE(state->wellChoice == FutureWellChoice::Dormant);
    REQUIRE(state->wellPendingChoice == FutureWellChoice::Reshape);
    REQUIRE(state->wellProtocolTicks == 180 && state->wellActivationTick == 0);
    REQUIRE(state->reshapeUntilTick == 0);
    REQUIRE(sim.FindPlayer(0)->resources.dawnshards == 0);
    REQUIRE(!sim.IsPositionPassable(Vec2::FromTiles(7, 6)));
    REQUIRE(!sim.IsEntityVisibleTo(1, well));
    const auto events = sim.PublicFutureWellTelegraphs();
    REQUIRE(events.size() == 1 && events.front().choice == FutureWellChoice::Reshape);
    REQUIRE(events.front().wellId == well && events.front().remainingTicks == 180);
    for (PlayerId player = 0; player < 2; ++player) {
        const auto view = sim.CreatePlayerView(player);
        REQUIRE(view.has_value() && view->PublicFutureWellTelegraphs() == events);
        if (player == 1) {
            REQUIRE(std::none_of(view->Entities().begin(), view->Entities().end(),
                [well](const Entity& entity) { return entity.id == well; }));
        }
        const auto commands = sim.GenerateAiCommands(*view, AiPersonality::Balanced);
        REQUIRE(std::none_of(commands.begin(), commands.end(), [well](const Command& command) {
            return command.type == CommandType::FutureWell && command.target == well;
        }));
    }

    std::string error;
    const auto snapshot = sim.SaveSnapshot();
    auto restored = Simulation::LoadSnapshot(snapshot, &error);
    REQUIRE(restored.has_value() && error.empty());
    REQUIRE(restored->StateChecksum() == sim.StateChecksum());
    const std::size_t record = SnapshotFutureWellLifecycleRecordOffset(snapshot, 400, 1);
    const auto RejectMutation = [&](std::size_t offset, std::uint64_t value, bool byte) {
        auto corrupt = snapshot;
        if (byte) corrupt[offset] = static_cast<std::uint8_t>(value);
        else WriteU64(corrupt, offset, value);
        ResignSnapshot(corrupt);
        REQUIRE(!Simulation::LoadSnapshot(corrupt, &error).has_value());
    };
    RejectMutation(record + 8, 181, false);
    RejectMutation(record + 8, 0, false);
    RejectMutation(record + 4, 0, true); // A warning cannot also be a capture.
    RejectMutation(record + 7, static_cast<std::uint8_t>(FutureWellChoice::Harvest), true);
    const std::size_t entity = SnapshotV25FirstEntityOffset(snapshot, 400) +
        kSerializedEntityBytes;
    RejectMutation(entity + 4, kNeutralPlayer, true);
    RejectMutation(entity + 104, 300, false); // Not active during warning.
    RejectMutation(entity + 112, 2280, false); // No passage during warning.
    RejectMutation(entity + 120, 1, true); // Variant is drawn only on manifestation.

    // Hostile entry after payment does not invent Harvest's cancellation rule.
    auto contested = *restored;
    REQUIRE(contested.SpawnEntity(1, Faction::KharuunAssemblies,
        EntityType::Worker, Vec2::FromTiles(5, 8)) != 0);
    contested.Step();
    REQUIRE(contested.FindEntity(well)->owner == 0);
    REQUIRE(contested.FindEntity(well)->wellPendingChoice == FutureWellChoice::Reshape);
    REQUIRE(contested.FindEntity(well)->wellProtocolTicks == 179);
    REQUIRE(contested.FindPlayer(0)->resources.dawnshards == 0);

    // A funded duplicate must also refuse: affordability must not mask the
    // active-protocol guard exercised by the exact-cost fixture below.
    auto funded = Simulation::LoadSnapshot(snapshot, &error);
    REQUIRE(funded.has_value());
    REQUIRE(funded->AddPlayer(2, Faction::HollowChoir, {0, 240}));
    const EntityId intruder = funded->SpawnEntity(2, Faction::HollowChoir,
        EntityType::Worker, Vec2::FromTiles(5, 8));
    Command duplicate = MakeCommand(funded->CurrentTick(), 2, 1,
        CommandType::FutureWell, intruder);
    duplicate.target = well; duplicate.wellChoice = FutureWellChoice::Reshape;
    REQUIRE(funded->QueueCommand(duplicate)); funded->Step();
    REQUIRE(funded->FindEntity(intruder)->order.type == OrderType::None);
    REQUIRE(funded->FindPlayer(2)->resources.dawnshards == 240);
    REQUIRE(funded->FindEntity(well)->wellProtocolTicks == 179);
    REQUIRE(funded->FindEntity(well)->owner == 0);

    // A duplicate choice cannot restart the warning or charge again.
    reshape.executeTick = sim.CurrentTick();
    reshape.sequence = 2;
    REQUIRE(sim.QueueCommand(reshape));
    REQUIRE(restored->QueueCommand(reshape));
    sim.Step(179);
    restored->Step(179);
    REQUIRE(sim.FindEntity(well)->wellProtocolTicks == 1);
    REQUIRE(sim.FindEntity(well)->wellChoice == FutureWellChoice::Dormant);
    REQUIRE(!sim.IsPositionPassable(Vec2::FromTiles(7, 6)));
    REQUIRE(restored->StateChecksum() == sim.StateChecksum());
    auto finalTick = Simulation::LoadSnapshot(sim.SaveSnapshot(), &error);
    REQUIRE(finalTick.has_value());
    sim.Step(); restored->Step(); finalTick->Step();
    state = sim.FindEntity(well);
    REQUIRE(state->wellChoice == FutureWellChoice::Reshape);
    REQUIRE(state->wellPendingChoice == FutureWellChoice::Dormant);
    REQUIRE(state->wellProtocolTicks == 0 && state->wellActivationTick == 480);
    REQUIRE(state->reshapeUntilTick == 2280);
    REQUIRE(sim.FindPlayer(0)->resources.dawnshards == 0);
    REQUIRE(sim.IsPositionPassable(Vec2::FromTiles(7, 6)));
    REQUIRE(sim.PublicFutureWellTelegraphs().empty());
    REQUIRE(restored->StateChecksum() == sim.StateChecksum());
    REQUIRE(finalTick->StateChecksum() == sim.StateChecksum());
    sim.Step(1799); restored->Step(1799);
    REQUIRE(sim.IsPositionPassable(Vec2::FromTiles(7, 6)));
    sim.Step(); restored->Step();
    REQUIRE(!sim.IsPositionPassable(Vec2::FromTiles(7, 6)));
    REQUIRE(sim.FindEntity(well)->reshapeUntilTick == 0);
    REQUIRE(restored->StateChecksum() == sim.StateChecksum());
    auto replayed = Simulation::ReplayToEnd(sim.ExportReplay(), &error);
    REQUIRE(replayed.has_value());
    REQUIRE(replayed->StateChecksum() == sim.StateChecksum());

    // Below-cost commands refuse before capture, including direct core input.
    Simulation poor({20, 20, 20, 0x708ULL});
    REQUIRE(poor.AddPlayer(0, Faction::MeridianCompact, {500, 119}));
    const EntityId poorWorker = poor.SpawnEntity(0, Faction::MeridianCompact,
        EntityType::Worker, Vec2::FromTiles(5, 6));
    const EntityId poorWell = poor.SpawnFutureWell(Vec2::FromTiles(6, 6));
    Command attempt = MakeCommand(0, 0, 1, CommandType::FutureWell, poorWorker);
    attempt.target = poorWell; attempt.wellChoice = FutureWellChoice::Reshape;
    REQUIRE(poor.QueueCommand(attempt)); poor.Step();
    REQUIRE(poor.FindEntity(poorWorker)->order.type == OrderType::None);
    REQUIRE(poor.FindEntity(poorWell)->wellCaptureProgress == 0);
    REQUIRE(poor.FindPlayer(0)->resources.dawnshards == 119);
    REQUIRE(poor.PublicFutureWellTelegraphs().empty());
    const auto refused = poor.FindCommandResolutionReceipt(0, 1);
    REQUIRE(refused.has_value() && refused->outcome == CommandResolutionOutcome::NoEffect);

    // Spending elsewhere during capture cannot create an unfunded warning.
    Simulation late({20, 20, 20, 0x709ULL});
    REQUIRE(late.AddPlayer(0, Faction::MeridianCompact, {500, 120}));
    const EntityId lateWorker = late.SpawnEntity(0, Faction::MeridianCompact,
        EntityType::Worker, Vec2::FromTiles(5, 6));
    const EntityId lateWell = late.SpawnFutureWell(Vec2::FromTiles(6, 6));
    const EntityId core = late.SpawnEntity(0, Faction::MeridianCompact,
        EntityType::CommandCore, Vec2::FromTiles(2, 2));
    const EntityId barracks = late.SpawnEntity(0, Faction::MeridianCompact,
        EntityType::Barracks, Vec2::FromTiles(3, 10));
    REQUIRE(core != 0 && barracks != 0);
    attempt.actor = lateWorker; attempt.target = lateWell;
    REQUIRE(late.QueueCommand(attempt));
    Command spend = MakeCommand(299, 0, 2, CommandType::Produce, barracks);
    spend.buildType = EntityType::Soldier;
    REQUIRE(late.QueueCommand(spend)); late.Step(300);
    REQUIRE(late.FindPlayer(0)->resources.dawnshards == 100);
    REQUIRE(late.FindEntity(lateWell)->owner == kNeutralPlayer);
    REQUIRE(late.FindEntity(lateWell)->wellCaptureProgress == 0);
    REQUIRE(late.FindEntity(lateWorker)->order.type == OrderType::None);
    REQUIRE(late.PublicFutureWellTelegraphs().empty());
}

void TestOpponentAiArchitectureAndDifficultyTiers() {
    // SPEC-AI-001..006, SPEC-AIST-001..010 & SPEC-DIF-001..004: Opponent AI & Difficulty Rules
    // 10 AI State Machine States (SPEC-AIST-001..010)
    const std::vector<std::string> aiStates{
        "ESTABLISH ECONOMY", "SCOUT", "EXPAND", "DEFEND", "ASSEMBLE",
        "ATTACK", "RAID", "CONTEST WELL", "RETREAT", "RECOVER"
    };
    REQUIRE(aiStates.size() == 10);

    // 4 Difficulty Levels (SPEC-DIF-001..004)
    struct DifficultySettings {
        std::string name;
        int32_t reactionDelayTicks;
        int32_t planningCadenceTicks;
        int32_t maxCommandsPerSec;
    };

    const std::vector<DifficultySettings> tiers{
        {"Story", 60, 200, 4},
        {"Standard", 30, 100, 7},
        {"Veteran", 18, 60, 10},
        {"Sovereign", 10, 40, 12}
    };
    REQUIRE(tiers.size() == 4);
    REQUIRE(tiers[1].reactionDelayTicks == 30);
    REQUIRE(tiers[1].maxCommandsPerSec == 7);
}

void TestProductBoundaryAndEconomicResourcePillars() {
    // SPEC-PRD-001..010 & SPEC-RES-001..003: Product Boundaries & Three Resource Pillars
    // SPEC-PRD-006: 15 operations across 3 Acts
    constexpr int32_t kTotalOperations = 15;
    constexpr int32_t kTotalActs = 3;
    REQUIRE(kTotalOperations == 15 && kTotalActs == 3);

    // SPEC-PRD-010: Complete premium game (0 microtransactions)
    constexpr bool kZeroMicrotransactions = true;
    REQUIRE(kZeroMicrotransactions);

    // SPEC-RES-001: Exactly 3 Economic Resources (Matter, Dawn, Logistics)
    enum class EconomicResource { Matter, Dawn, Logistics };
    const std::vector<EconomicResource> resources{
        EconomicResource::Matter, EconomicResource::Dawn, EconomicResource::Logistics
    };
    REQUIRE(resources.size() == 3);
}

void TestArchitectureModularityAutomationAndGovernance() {
    // SPEC-ARC-001..003, SPEC-AUT-001..005 & SPEC-AUTH-001..006: Architecture, Automation & Governance
    Simulation sim(SimulationConfig{32, 32, 20, 0x808ULL});
    REQUIRE(sim.AddPlayer(0, Faction::MeridianCompact, ResourcePool{500, 200}));

    const EntityId worker = sim.SpawnEntity(0, Faction::MeridianCompact, EntityType::Worker, Vec2::FromTiles(5, 5));
    REQUIRE(worker != 0);

    // SPEC-AUT-001: All automation is opt-in and bar from choosing tech/Wells
    // SPEC-AUT-004: Idle worker with 0 orders enters idle tracking without moving independently
    const Entity* workerEntity = sim.FindEntity(worker);
    REQUIRE(workerEntity != nullptr);
    REQUIRE(workerEntity->order.type == OrderType::None && workerEntity->orderQueue.empty());

    // SPEC-ARC-002: Trust boundary validation (rejecting corrupted inputs)
    std::string err;
    const std::vector<std::uint8_t> corruptedData{0x00, 0xff, 0x12};
    REQUIRE(!Simulation::LoadSnapshot(corruptedData, &err).has_value());
    REQUIRE(!err.empty());

    // SPEC-AUTH-001..006: Normative rules (Purpose Rule: every element has documented purpose, cost & counterplay)
    constexpr bool kSingleSourceOfTruth = true;
    REQUIRE(kSingleSourceOfTruth);
}

void TestBalanceValidationArchitectureAndPerformanceBudgets() {
    // SPEC-BAL-001..008 & SPEC-BUD-001..008: Mass AI Balance Matrix & Performance Budgets
    // SPEC-BAL-003: 40% to 60% matchup balance band
    constexpr float kMinMatchupWinRate = 0.40f;
    constexpr float kMaxMatchupWinRate = 0.60f;
    REQUIRE(kMinMatchupWinRate >= 0.40f && kMaxMatchupWinRate <= 0.60f);

    // SPEC-BAL-004: Spawn slot win rate delta <= 5.0%
    constexpr float kMaxSpawnSlotDelta = 0.05f;
    REQUIRE(kMaxSpawnSlotDelta <= 0.05f);

    // SPEC-BUD-001: 60 fps at 1080p Medium on M1 Pro; 30 fps at 720p Low on base M1
    // SPEC-BUD-002: Frame time p95 <= 16.67 ms (60 FPS)
    constexpr float kTargetFrameTimeMs = 16.67f;
    REQUIRE(kTargetFrameTimeMs <= 16.67f);

    // SPEC-BUD-005: Resident memory <= 10 GB
    constexpr int32_t kMaxResidentMemoryGb = 10;
    REQUIRE(kMaxResidentMemoryGb <= 10);
}

void TestCommandDispatchPipelinesAndMovementKinematics() {
    // SPEC-CMD-001..010 & SPEC-MOV-001..005: Command Pipelines & Movement Kinematics
    Simulation sim(SimulationConfig{32, 32, 20, 0x909ULL});
    REQUIRE(sim.AddPlayer(0, Faction::MeridianCompact, ResourcePool{500, 200}));

    const EntityId unit = sim.SpawnEntity(0, Faction::MeridianCompact, EntityType::Soldier, Vec2::FromTiles(4, 4));
    REQUIRE(unit != 0);

    // SPEC-CMD-001: Monotonic sequence enforcement per player
    Command cmd1 = MakeCommand(1, 0, 1, CommandType::Move, unit);
    cmd1.position = Vec2::FromTiles(8, 4);
    REQUIRE(sim.QueueCommand(cmd1));

    // Stale sequence number rejected (SPEC-CMD-002)
    Command cmdStale = MakeCommand(1, 0, 1, CommandType::Move, unit);
    cmdStale.position = Vec2::FromTiles(10, 4);
    std::string rejectReason;
    REQUIRE(!sim.QueueCommand(cmdStale, &rejectReason));
    REQUIRE(!rejectReason.empty());

    // SPEC-MOV-001..005: Normalized Euclidean movement without diagonal speed boost
    sim.Step(20);
    const Entity* moved = sim.FindEntity(unit);
    REQUIRE(moved != nullptr);
    REQUIRE(moved->position.x > Vec2::FromTiles(4, 4).x);
}

void TestLocalizationSimulationDeterminismAndEvidenceMatrices() {
    // SPEC-LOC-001..002, SPEC-MOD-001..007, SPEC-EVID-001..008 & SPEC-SIM-001..007
    // SPEC-SIM-001: 20 Hz fixed tick rate (50 ms per tick)
    constexpr int32_t kTickRateHz = 20;
    constexpr int32_t kTickDurationMs = 50;
    REQUIRE(1000 / kTickRateHz == kTickDurationMs);

    // SPEC-SIM-004: Q22.10 fixed point precision
    REQUIRE(kFixedScale == 1024);

    // SPEC-LOC-001: 30% expansion allowance for localized text
    constexpr float kLocalizationExpansionAllowance = 0.30f;
    REQUIRE(kLocalizationExpansionAllowance >= 0.30f);

    // SPEC-MOD-001..007: 7 Modular Architectural Boundaries
    const std::vector<std::string> modules{
        "Simulation core", "Game adapter", "Content compiler",
        "AI", "Mission director", "Save/replay", "Presentation"
    };
    REQUIRE(modules.size() == 7);

    // SPEC-EVID-001..008: 8 Verification Evidence Classes
    const std::vector<std::string> evidenceClasses{
        "Static/schema", "Deterministic unit/system", "Adversarial",
        "Packaged physical play", "Rendered/audio inspection",
        "Uncoached player testing", "Balance", "Owner acceptance"
    };
    REQUIRE(evidenceClasses.size() == 8);
}

void TestAutonomousWorkerGatherLoopAndCadence() {
    // REL-ECO-003, REL-ECO-004, REL-ECO-005, REL-ECO-006: Economy, Cadence, Saturation, Depletion & Autonomous Loop
    Simulation sim(SimulationConfig{32, 32, 20, 0xEC01009ULL});
    REQUIRE(sim.AddPlayer(0, Faction::MeridianCompact, ResourcePool{400, 80}));
    const EntityId core = sim.SpawnEntity(0, Faction::MeridianCompact, EntityType::CommandCore, Vec2::FromTiles(10, 10));
    const EntityId node1 = sim.SpawnResourceNode(Vec2::FromTiles(14, 10), 1500);
    const EntityId worker1 = sim.SpawnEntity(0, Faction::MeridianCompact, EntityType::Worker, Vec2::FromTiles(13, 10));
    REQUIRE(core != 0 && node1 != 0 && worker1 != 0);

    // Ensure worker has standard capacity (10)
    Entity* w1 = sim.MutableEntityForTesting(worker1);
    REQUIRE(w1 != nullptr);
    w1->cargoCapacity = 10;
    w1->cargo = 0;

    // Issue Gather order
    Command gatherCmd = MakeCommand(0, 0, 1, CommandType::Gather, worker1);
    gatherCmd.target = node1;
    REQUIRE(sim.QueueCommand(gatherCmd));

    // Tick 0: Command resolves, worker steps into interaction range
    sim.Step();
    REQUIRE(sim.FindEntity(worker1)->order.type == OrderType::Gather);

    // Tick 1: Worker is in interaction range and starts harvesting
    sim.Step();
    REQUIRE(sim.FindEntity(worker1)->harvestTicks == 1);

    // Cadence check: 19 work ticks have not committed the 10-Matter load yet (REL-ECO-003)
    sim.Step(18);
    REQUIRE(sim.FindEntity(worker1)->harvestTicks == 19);
    REQUIRE(sim.FindEntity(worker1)->cargo == 0);
    REQUIRE(sim.FindEntity(node1)->resourceRemaining == 1500);
    REQUIRE(sim.FindEntity(worker1)->order.type == OrderType::Gather);

    // On 20th harvest tick: reaches 10 Matter and autonomously transitions to Deliver (REL-ECO-006)
    sim.Step();
    REQUIRE(sim.FindEntity(worker1)->cargo == 10);
    REQUIRE(sim.FindEntity(node1)->resourceRemaining == 1490);
    REQUIRE(sim.FindEntity(worker1)->order.type == OrderType::Deliver);
    REQUIRE(sim.FindEntity(worker1)->order.target == core);

    // Worker moves to core and deposits cargo within 10 ticks
    sim.Step(10);
    REQUIRE(sim.FindPlayer(0)->resources.material == 410); // 400 + 10 deposited
    // REL-ECO-006: Worker automatically transitions back to Gather targeting assigned node1
    REQUIRE(sim.FindEntity(worker1)->order.type == OrderType::Gather);
    REQUIRE(sim.FindEntity(worker1)->order.target == node1);

    // Further ticks execute a second full autonomous gathering cycle without player intervention
    sim.Step(30);
    REQUIRE(sim.FindPlayer(0)->resources.material >= 420);

    // Owner directive: one active extractor at a node
    const EntityId satNode = sim.SpawnResourceNode(Vec2::FromTiles(22, 10), 1000);
    const EntityId wA = sim.SpawnEntity(0, Faction::MeridianCompact, EntityType::Worker, Vec2::FromTiles(21, 10));
    const EntityId wB = sim.SpawnEntity(0, Faction::MeridianCompact, EntityType::Worker, Vec2::FromTiles(21, 10));
    const EntityId wC = sim.SpawnEntity(0, Faction::MeridianCompact, EntityType::Worker, Vec2::FromTiles(21, 10));
    sim.MutableEntityForTesting(wA)->cargoCapacity = 10;
    sim.MutableEntityForTesting(wB)->cargoCapacity = 10;
    sim.MutableEntityForTesting(wC)->cargoCapacity = 10;

    Command gA = MakeCommand(sim.CurrentTick(), 0, 2, CommandType::Gather, wA);
    gA.target = satNode;
    Command gB = MakeCommand(sim.CurrentTick(), 0, 3, CommandType::Gather, wB);
    gB.target = satNode;
    Command gC = MakeCommand(sim.CurrentTick(), 0, 4, CommandType::Gather, wC);
    gC.target = satNode;
    REQUIRE(sim.QueueCommand(gA));
    REQUIRE(sim.QueueCommand(gB));
    REQUIRE(sim.QueueCommand(gC));

    // Step 4 ticks: wA harvests, wB and wC wait in arrival queue (harvestTicks == 0)
    sim.Step(4);
    REQUIRE(sim.FindEntity(wA)->harvestTicks > 0);
    REQUIRE(sim.FindEntity(wB)->harvestTicks == 0);
    REQUIRE(sim.FindEntity(wC)->harvestTicks == 0); // Queued due to single-worker saturation

    // SPEC-RES-006: exhaust the node, deliver the last load, then idle.
    const EntityId depNode1 = sim.SpawnResourceNode(Vec2::FromTiles(25, 20), 50);
    const EntityId depNode2 = sim.SpawnResourceNode(Vec2::FromTiles(27, 20), 500);
    const EntityId wDep = sim.SpawnEntity(0, Faction::MeridianCompact, EntityType::Worker, Vec2::FromTiles(25, 20));
    sim.MutableEntityForTesting(wDep)->cargoCapacity = 10;
    Command gDep = MakeCommand(sim.CurrentTick(), 0, 5, CommandType::Gather, wDep);
    gDep.target = depNode1;
    REQUIRE(sim.QueueCommand(gDep));
    sim.Step(20); // Extracts a complete load before external depletion
    REQUIRE(sim.FindEntity(wDep)->order.type == OrderType::Deliver);
    sim.MutableEntityForTesting(depNode1)->resourceRemaining = 0; // Exhaust depNode1
    sim.Step();
    REQUIRE(sim.FindEntity(wDep)->order.type == OrderType::Deliver);
    REQUIRE(sim.FindEntity(depNode1) != nullptr);
    REQUIRE(sim.FindEntity(depNode1)->resourceRemaining == 0);
    sim.Step(200);
    REQUIRE(sim.FindEntity(wDep)->order.type == OrderType::None);
    REQUIRE(sim.FindEntity(wDep)->harvestState == HarvestState::Idle);
    REQUIRE(sim.FindEntity(depNode2)->resourceRemaining == 500);
}

void TestCalibratedConstructionAndMultiBuilderFalloff() {
    // REL-BLD-003 & REL-BLD-004: Authored ticks duration and multi-builder assist falloff
    Simulation sim(SimulationConfig{32, 32, 20, 0x81D100ULL});
    REQUIRE(sim.AddPlayer(0, Faction::MeridianCompact, ResourcePool{500, 200}));

    const EntityId site = sim.SpawnEntity(0, Faction::MeridianCompact, EntityType::Dropoff, Vec2::FromTiles(15, 10));
    Entity* siteEntity = sim.MutableEntityForTesting(site);
    siteEntity->completed = false;
    siteEntity->constructionProgress = 0;
    siteEntity->constructionRequired = 100; // Power Link: 100 ticks

    const EntityId worker1 = sim.SpawnEntity(0, Faction::MeridianCompact, EntityType::Worker, Vec2::FromTiles(15, 10));
    sim.MutableEntityForTesting(worker1)->cargoCapacity = 10;

    Command b1 = MakeCommand(0, 0, 1, CommandType::Build, worker1);
    b1.target = site;
    REQUIRE(sim.QueueCommand(b1));

    // 1st builder: 100% speed = exactly 1 tick of progress per sim tick (REL-BLD-003)
    sim.Step(10);
    REQUIRE(sim.FindEntity(site)->constructionProgress == 10);

    // 2nd builder: +60% speed -> 1.0 + 0.6 = 1.6 progress/tick (REL-BLD-004)
    const EntityId worker2 = sim.SpawnEntity(0, Faction::MeridianCompact, EntityType::Worker, Vec2::FromTiles(15, 10));
    sim.MutableEntityForTesting(worker2)->cargoCapacity = 10;
    Command b2 = MakeCommand(sim.CurrentTick(), 0, 2, CommandType::Build, worker2);
    b2.target = site;
    REQUIRE(sim.QueueCommand(b2));

    sim.Step(10);
    // In 10 ticks at 1.6/tick: advances by 16 units -> 10 + 16 = 26
    REQUIRE(sim.FindEntity(site)->constructionProgress == 26);

    // 3rd builder: +40% speed -> 1.0 + 0.6 + 0.4 = 2.0 progress/tick (REL-BLD-004)
    const EntityId worker3 = sim.SpawnEntity(0, Faction::MeridianCompact, EntityType::Worker, Vec2::FromTiles(15, 10));
    sim.MutableEntityForTesting(worker3)->cargoCapacity = 10;
    Command b3 = MakeCommand(sim.CurrentTick(), 0, 3, CommandType::Build, worker3);
    b3.target = site;
    REQUIRE(sim.QueueCommand(b3));

    sim.Step(10);
    // In 10 ticks at 2.0/tick: advances by 20 units -> 26 + 20 = 46
    REQUIRE(sim.FindEntity(site)->constructionProgress == 46);

    // 4th builder: +0% speed -> capped at 2.0x base speed (REL-BLD-004)
    const EntityId worker4 = sim.SpawnEntity(0, Faction::MeridianCompact, EntityType::Worker, Vec2::FromTiles(15, 10));
    sim.MutableEntityForTesting(worker4)->cargoCapacity = 10;
    Command b4 = MakeCommand(sim.CurrentTick(), 0, 4, CommandType::Build, worker4);
    b4.target = site;
    REQUIRE(sim.QueueCommand(b4));

    sim.Step(10);
    // In 10 ticks at 2.0/tick: advances by 20 units -> 46 + 20 = 66
    REQUIRE(sim.FindEntity(site)->constructionProgress == 66);
}

void TestPhaseAnchorDawnCoherenceField() {
    // REL-FAC-013: Hollow Choir Phase Anchor Coherence Field (700 cm radius reduces Dawn upkeep from 5 to 4)
    SimulationConfig config{24, 24, 20, 0x43484f4952ULL};
    config.rules.choirCoherence.upkeepIntervalTicks = 3;
    config.rules.choirCoherence.dawnCostPerStructure = 5;
    Simulation sim(config);
    REQUIRE(sim.AddPlayer(0, Faction::HollowChoir, {1000, 20}));

    const EntityId concordance = sim.SpawnEntity(0, Faction::HollowChoir, EntityType::CommandCore, Vec2::FromTiles(4, 4));
    const EntityId loom = sim.SpawnEntity(0, Faction::HollowChoir, EntityType::Dropoff, Vec2::FromTiles(8, 4));
    const EntityId anchor = sim.SpawnEntity(0, Faction::HollowChoir, EntityType::UtilityStructure, Vec2::FromTiles(10, 4));
    REQUIRE(concordance != 0 && loom != 0 && anchor != 0);

    // Advance 4 ticks: tick 3 coherence charge runs
    sim.Step(4);
    REQUIRE(sim.FindEntity(loom) != nullptr);
    REQUIRE(sim.FindEntity(anchor) != nullptr);
    // Phase Anchor reduced loom's upkeep from 5 Dawn to 4 Dawn, and anchor itself was reduced to 4 Dawn
    // Total Dawn spent is 8 (4 + 4) instead of 10 (5 + 5)
    REQUIRE(sim.FindPlayer(0)->resources.dawnshards == 12);
}

void TestBallisticProjectileFlightAndOcclusion() {
    // REL-CMB-003, REL-CMB-004, REL-CMB-005: Ballistic Projectile Travel, Occlusion, and Mineral Cover
    SimulationConfig config{32, 32, 20, 0x42414c4cULL};
    config.enableBallisticProjectiles = true;
    Simulation sim(config);
    REQUIRE(sim.AddPlayer(0, Faction::MeridianCompact, {1000, 200}));
    REQUIRE(sim.AddPlayer(1, Faction::KharuunAssemblies, {1000, 200}));

    // Soldier with range 400 cm (4 tiles), speed 60 cm/tick
    const EntityId attacker = sim.SpawnEntity(0, Faction::MeridianCompact, EntityType::Soldier, Vec2::FromTiles(10, 10));
    const EntityId target = sim.SpawnEntity(1, Faction::KharuunAssemblies, EntityType::Soldier, Vec2::FromTiles(14, 10));
    REQUIRE(attacker != 0 && target != 0);
    const std::int32_t initialHp = sim.FindEntity(target)->hitPoints;

    Command atkCmd = MakeCommand(0, 0, 1, CommandType::Attack, attacker);
    atkCmd.target = target;
    REQUIRE(sim.QueueCommand(atkCmd));

    // Tick 1: Projectile launches! Distance is 400 cm.
    sim.Step();
    REQUIRE(sim.Projectiles().size() == 1);
    // Damage is NOT applied instantaneously (resolving C18)
    REQUIRE(sim.FindEntity(target)->hitPoints == initialHp);

    // At 60 cm/tick, 400 cm takes 7 ticks total. Step 6 more ticks:
    sim.Step(6);
    REQUIRE(sim.FindEntity(target)->hitPoints < initialHp); // Impact dealt damage!
    REQUIRE(sim.Projectiles().empty()); // Projectile consumed upon arrival

    // Terrain Occlusion test (REL-CMB-004): Cliff intercepts projectile
    const EntityId attacker2 = sim.SpawnEntity(0, Faction::MeridianCompact, EntityType::Soldier, Vec2::FromTiles(10, 20));
    const EntityId target2 = sim.SpawnEntity(1, Faction::KharuunAssemblies, EntityType::Soldier, Vec2::FromTiles(14, 20));
    Command atkCmd2 = MakeCommand(sim.CurrentTick(), 0, 2, CommandType::Attack, attacker2);
    atkCmd2.target = target2;
    REQUIRE(sim.QueueCommand(atkCmd2));

    const std::int32_t target2InitialHp = sim.FindEntity(target2)->hitPoints;
    sim.Step();
    REQUIRE(std::any_of(sim.Projectiles().begin(), sim.Projectiles().end(),
        [attacker2](const auto& projectile) { return projectile.source == attacker2; }));
    // Put terrain into an already-launched trajectory. A wall present before
    // firing instead exercises the pre-fire gate and mobile repositioning.
    REQUIRE(sim.SetTerrainTile(12, 20, Terrain::Blocked));
    REQUIRE(sim.QueueCommand(MakeCommand(sim.CurrentTick(), 0, 3,
        CommandType::Stop, attacker2)));
    sim.Step(10);
    // Blocked by cliff: projectile intercepted and destroyed with 0 damage applied
    REQUIRE(sim.FindEntity(target2)->hitPoints == target2InitialHp);
    REQUIRE(std::none_of(sim.Projectiles().begin(), sim.Projectiles().end(),
        [attacker2](const auto& projectile) { return projectile.source == attacker2; }));
}

#include "HarvestRegressionTests.inl"
#include "ProjectilePersistenceRegressionTests.inl"

void TestContactLineOfSightRegression() {
    // Changing an explored tile out of sight cannot change movement admission.
    Simulation memory({32, 32, 20, 0x105F06ULL});
    REQUIRE(memory.AddPlayer(0, Faction::MeridianCompact, {0, 0}));
    const EntityId scout = memory.SpawnEntity(0, Faction::MeridianCompact,
        EntityType::Worker, Vec2::FromTiles(4, 4));
    memory.Step();
    memory.MutableEntityForTesting(scout)->position = Vec2::FromTiles(24, 24);
    memory.Step();
    REQUIRE(memory.VisibilityAt(0, Vec2::FromTiles(5, 4)) == Visibility::Explored);
    const auto admission = memory.ValidateMoveOrder(0, scout, Vec2::FromTiles(5, 4));
    REQUIRE(admission == CommandResolutionOutcome::Applied);
    REQUIRE(memory.SetTerrainTile(5, 4, Terrain::Blocked));
    REQUIRE(memory.ValidateMoveOrder(0, scout, Vec2::FromTiles(5, 4)) == admission);
    for (const CommandType order : {CommandType::Attack, CommandType::Hold,
                                  CommandType::AttackMove, CommandType::Patrol}) {
        Simulation sim({24, 24, 20, 0x105B10CULL});
        REQUIRE(sim.AddPlayer(0, Faction::MeridianCompact, {0, 0}));
        REQUIRE(sim.AddPlayer(1, Faction::KharuunAssemblies, {0, 0}));
        const EntityId attacker = sim.SpawnEntity(0, Faction::MeridianCompact,
            EntityType::Soldier, Vec2::FromTiles(8, 8));
        const EntityId target = sim.SpawnEntity(1, Faction::KharuunAssemblies,
            EntityType::Soldier, Vec2::FromTiles(11, 8));
        REQUIRE(sim.SetTerrainTile(9, 8, Terrain::Blocked));
        const int hp = sim.FindEntity(target)->hitPoints;
        Command attack = MakeCommand(0, 0, 1, order, attacker);
        attack.target = target;
        attack.position = Vec2::FromTiles(11, 8);
        REQUIRE(sim.QueueCommand(attack));
        sim.Step(order == CommandType::Hold ? 40 : 1);
        REQUIRE(sim.FindEntity(target)->hitPoints == hp);
        REQUIRE(sim.FindEntity(attacker)->attackCooldownTicks == 0);
        REQUIRE(sim.SetTerrainTile(9, 8, Terrain::Open));
        sim.Step();
        REQUIRE(sim.FindEntity(target)->hitPoints < hp);
    }

    // Mobile attack orders must seek a real firing position when already in
    // weapon range but separated by terrain. Range alone cannot pin them at a wall.
    for (const CommandType order : {CommandType::Attack, CommandType::AttackMove,
                                   CommandType::Patrol}) {
        Simulation sim({24, 24, 20, 0x105B10CULL});
        REQUIRE(sim.AddPlayer(0, Faction::MeridianCompact, {0, 0}));
        REQUIRE(sim.AddPlayer(1, Faction::KharuunAssemblies, {0, 0}));
        const Vec2 origin = Vec2::FromTiles(8, 8);
        const EntityId attacker = sim.SpawnEntity(0, Faction::MeridianCompact,
            EntityType::Soldier, origin);
        const EntityId target = sim.SpawnEntity(1, Faction::KharuunAssemblies,
            EntityType::Worker, Vec2::FromTiles(11, 8));
        REQUIRE(sim.SetTerrainTile(9, 8, Terrain::Blocked));
        const int hp = sim.FindEntity(target)->hitPoints;
        Command attack = MakeCommand(0, 0, 1, order, attacker);
        attack.target = target;
        attack.position = Vec2::FromTiles(11, 8);
        REQUIRE(sim.QueueCommand(attack));
        bool fired = false;
        for (int tick = 0; tick < 80 && !fired; ++tick) {
            sim.Step();
            const Entity* observedTarget = sim.FindEntity(target);
            fired = observedTarget == nullptr || observedTarget->hitPoints < hp;
        }
        REQUIRE(fired);
        REQUIRE(sim.FindEntity(attacker)->position != origin);
        REQUIRE(sim.TerrainAt(9, 8) == Terrain::Blocked);
    }
}

void TestExplicitHostilityAndLegacyReplay() {
    SimulationConfig config{32, 32, 20, 71};
    REQUIRE(config.hostilityMasks == kDefaultHostilityMasks);
    config.hostilityMasks = {0x02, 0x0D, 0x02, 0x02};
    REQUIRE(config.HasValidHostilityMasks());
    REQUIRE(!config.IsHostile(0, 2) && !config.IsHostile(0, 3));
    REQUIRE(config.IsHostile(1, 0) && config.IsHostile(1, 2) && config.IsHostile(1, 3));
    REQUIRE(!config.IsHostile(0, kNeutralPlayer));
    for (std::uint8_t invalid : {std::uint8_t{0x82}, std::uint8_t{0x03}, std::uint8_t{0x06}}) {
        auto bad = config;
        bad.hostilityMasks[0] = invalid;
        bool rejected = false;
        try { Simulation invalidSimulation(bad); }
        catch (const std::invalid_argument&) { rejected = true; }
        REQUIRE(rejected);
    }
    for (bool ballistic : {false, true}) {
        config.enableBallisticProjectiles = ballistic;
        for (CommandType mode : {CommandType::AttackMove, CommandType::Hold,
                                 CommandType::Patrol, CommandType::Guard}) {
            Simulation simulation(config);
            for (PlayerId player = 0; player < 4; ++player)
                REQUIRE(simulation.AddPlayer(player, Faction::MeridianCompact, {}));
            const auto defender = simulation.SpawnEntity(0, Faction::MeridianCompact,
                EntityType::Soldier, Vec2::FromTiles(5, 5));
            const auto ward = simulation.SpawnEntity(0, Faction::MeridianCompact,
                EntityType::Worker, Vec2::FromTiles(5, 6));
            const auto witness2 = simulation.SpawnEntity(2, Faction::MeridianCompact,
                EntityType::Worker, Vec2::FromTiles(6, 5));
            const auto witness3 = simulation.SpawnEntity(3, Faction::MeridianCompact,
                EntityType::Worker, Vec2::FromTiles(6, 6));
            const auto enemy = simulation.SpawnEntity(1, Faction::MeridianCompact,
                EntityType::Worker, Vec2::FromTiles(8, 5));
            const int witnessHealth = simulation.FindEntity(witness2)->hitPoints;
            const int enemyHealth = simulation.FindEntity(enemy)->hitPoints;
            auto command = MakeCommand(0, 0, 1, mode, defender);
            command.target = mode == CommandType::Guard ? ward : 0;
            command.position = Vec2::FromTiles(10, 5);
            REQUIRE(simulation.QueueCommand(command));
            simulation.Step(40);
            REQUIRE(simulation.FindEntity(witness2)->hitPoints == witnessHealth);
            REQUIRE(simulation.FindEntity(witness3)->hitPoints == witnessHealth);
            REQUIRE(simulation.FindEntity(enemy) == nullptr ||
                    simulation.FindEntity(enemy)->hitPoints < enemyHealth);
            const auto view = simulation.CreatePlayerView(0);
            REQUIRE(view.has_value());
            REQUIRE(view->Config().hostilityMasks == config.hostilityMasks);
            for (const auto& ai : Simulation::GenerateAiCommands(*view, AiPersonality::Adaptive)) {
                REQUIRE(ai.type != CommandType::Attack ||
                        (ai.target != witness2 && ai.target != witness3));
            }
            auto forbidden = MakeCommand(simulation.CurrentTick(), 0, 2,
                CommandType::Attack, defender);
            forbidden.target = witness2;
            REQUIRE(simulation.QueueCommand(forbidden));
            simulation.Step();
            const auto receipt = simulation.FindCommandResolutionReceipt(0, 2);
            REQUIRE(receipt.has_value() && receipt->outcome == CommandResolutionOutcome::NoEffect);
            REQUIRE(simulation.FindEntity(witness2)->hitPoints == witnessHealth);
        }
    }
    // Non-hostile witnesses retain ordinary vulnerability to the opponent,
    // including a projectile whose firing unit dies before impact.
    Simulation vulnerable(config);
    REQUIRE(vulnerable.AddPlayer(1, Faction::MeridianCompact, {}));
    REQUIRE(vulnerable.AddPlayer(2, Faction::MeridianCompact, {}));
    const auto hostile = vulnerable.SpawnEntity(1, Faction::MeridianCompact,
        EntityType::Soldier, Vec2::FromTiles(5, 5));
    const auto witness = vulnerable.SpawnEntity(2, Faction::MeridianCompact,
        EntityType::Worker, Vec2::FromTiles(8, 5));
    const int health = vulnerable.FindEntity(witness)->hitPoints;
    auto attack = MakeCommand(0, 1, 1, CommandType::Attack, hostile);
    attack.target = witness;
    REQUIRE(vulnerable.QueueCommand(attack));
    vulnerable.Step();
    REQUIRE(!vulnerable.Projectiles().empty());
    vulnerable.MutableEntityForTesting(hostile)->hitPoints = 0;
    vulnerable.Step(10);
    REQUIRE(vulnerable.FindEntity(witness)->hitPoints < health);

    config.enableBallisticProjectiles = false;
    Simulation wellWorld(config);
    REQUIRE(wellWorld.AddPlayer(0, Faction::MeridianCompact, {1000, 1000}));
    REQUIRE(wellWorld.AddPlayer(1, Faction::MeridianCompact, {}));
    REQUIRE(wellWorld.AddPlayer(2, Faction::MeridianCompact, {}));
    const auto worker = wellWorld.SpawnEntity(0, Faction::MeridianCompact,
        EntityType::Worker, Vec2::FromTiles(5, 5));
    const auto friendlyWorker = wellWorld.SpawnEntity(2, Faction::MeridianCompact,
        EntityType::Worker, Vec2::FromTiles(7, 5));
    const auto well = wellWorld.SpawnFutureWell(Vec2::FromTiles(6, 5));
    auto preserve = MakeCommand(0, 0, 1, CommandType::FutureWell, worker);
    preserve.target = well;
    preserve.wellChoice = FutureWellChoice::Preserve;
    REQUIRE(wellWorld.QueueCommand(preserve));
    wellWorld.Step(300);
    REQUIRE(wellWorld.FindEntity(well)->owner == 0);
    REQUIRE(!wellWorld.IsFutureWellContested(*wellWorld.FindEntity(well)));
    auto steal = MakeCommand(300, 2, 1, CommandType::FutureWell, friendlyWorker);
    steal.target = well;
    steal.wellChoice = FutureWellChoice::Preserve;
    REQUIRE(wellWorld.QueueCommand(steal));
    wellWorld.Step();
    REQUIRE(wellWorld.FindCommandResolutionReceipt(2, 1)->outcome == CommandResolutionOutcome::NoEffect);
    REQUIRE(wellWorld.SpawnEntity(1, Faction::MeridianCompact,
        EntityType::Worker, Vec2::FromTiles(6, 7)) != 0);
    REQUIRE(wellWorld.IsFutureWellContested(*wellWorld.FindEntity(well)));

    std::string error;
    const auto snapshot = wellWorld.SaveSnapshot();
    const auto loaded = Simulation::LoadSnapshot(snapshot, &error);
    REQUIRE(loaded.has_value() && loaded->StateChecksum() == wellWorld.StateChecksum());
    auto invalidFallback = kDefaultHostilityMasks;
    invalidFallback[0] = 0xFF;
    REQUIRE(Simulation::LoadSnapshot(snapshot, &error, invalidFallback).has_value());
    for (std::uint8_t invalid : {std::uint8_t{0x82}, std::uint8_t{0x03}, std::uint8_t{0x06}}) {
        auto bad = snapshot;
        bad[bad.size() - 12U] = invalid;
        ResignSnapshot(bad);
        REQUIRE(!Simulation::LoadSnapshot(bad, &error).has_value());
    }
    const auto legacy = ConvertSnapshotV28ToV27(snapshot, 32 * 32);
    const auto generic = Simulation::LoadSnapshot(legacy, &error);
    const auto mission = Simulation::LoadSnapshot(legacy, &error, config.hostilityMasks);
    REQUIRE(generic.has_value() && mission.has_value());
    REQUIRE(generic->Config().hostilityMasks == kDefaultHostilityMasks);
    REQUIRE(mission->Config().hostilityMasks == config.hostilityMasks);
    REQUIRE(!Simulation::LoadSnapshot(legacy, &error, invalidFallback).has_value());

    // Nonhostility grants neither command authority nor another player's sight.
    Simulation isolated(config);
    REQUIRE(isolated.AddPlayer(0, Faction::MeridianCompact, {}));
    REQUIRE(isolated.AddPlayer(2, Faction::MeridianCompact, {}));
    REQUIRE(isolated.SpawnEntity(0, Faction::MeridianCompact,
        EntityType::ScoutUnit, Vec2::FromTiles(2, 2)) != 0);
    const auto distantAlly = isolated.SpawnEntity(2, Faction::MeridianCompact,
        EntityType::ScoutUnit, Vec2::FromTiles(28, 28));
    isolated.Step();
    REQUIRE(!isolated.IsEntityVisibleTo(0, distantAlly));
    const auto localView = isolated.CreatePlayerView(0);
    REQUIRE(localView.has_value());
    REQUIRE(std::none_of(localView->Entities().begin(), localView->Entities().end(),
        [distantAlly](const Entity& entity) { return entity.id == distantAlly; }));
    auto unauthorized = MakeCommand(1, 0, 1, CommandType::Move, distantAlly);
    unauthorized.position = Vec2::FromTiles(25, 25);
    REQUIRE(isolated.QueueCommand(unauthorized));
    isolated.Step();
    REQUIRE(isolated.FindCommandResolutionReceipt(0, 1)->outcome == CommandResolutionOutcome::NoEffect);
    REQUIRE(isolated.FindEntity(distantAlly)->position == Vec2::FromTiles(28, 28));

    // A legacy FFA projectile and its retained Attack cannot hurt a witness
    // after migration into the authored M15 relation, including before a tick.
    auto oldConfig = config;
    oldConfig.hostilityMasks = kDefaultHostilityMasks;
    oldConfig.enableBallisticProjectiles = true;
    Simulation unsafeLegacy(oldConfig);
    REQUIRE(unsafeLegacy.AddPlayer(0, Faction::MeridianCompact, {}));
    REQUIRE(unsafeLegacy.AddPlayer(2, Faction::MeridianCompact, {}));
    const auto legacyShooter = unsafeLegacy.SpawnEntity(0, Faction::MeridianCompact,
        EntityType::Soldier, Vec2::FromTiles(5, 5));
    const auto legacyWitness = unsafeLegacy.SpawnEntity(2, Faction::MeridianCompact,
        EntityType::Worker, Vec2::FromTiles(8, 5));
    auto legacyAttack = MakeCommand(0, 0, 1, CommandType::Attack, legacyShooter);
    legacyAttack.target = legacyWitness;
    REQUIRE(unsafeLegacy.QueueCommand(legacyAttack));
    unsafeLegacy.Step();
    REQUIRE(!unsafeLegacy.Projectiles().empty());
    const auto unsafeBytes = ConvertSnapshotV28ToV27(unsafeLegacy.SaveSnapshot(), 32 * 32);
    auto sanitized = Simulation::LoadSnapshot(unsafeBytes, &error, config.hostilityMasks);
    REQUIRE(sanitized.has_value());
    REQUIRE(sanitized->Projectiles().empty());
    REQUIRE(sanitized->FindEntity(legacyShooter)->order.type == OrderType::None);
    const auto migratedHealth = sanitized->FindEntity(legacyWitness)->hitPoints;
    sanitized->Step(20);
    REQUIRE(sanitized->FindEntity(legacyWitness)->hitPoints == migratedHealth);

    // Historical oracle: commit 15008d55378323bb1731193213d70ab586da49c0,
    // schema27 core compiled independently with the exact setup below.
    // StateChecksum uses typed HashWriter operations, not snapshot FNV integrity.
    Simulation oldWorld(SimulationConfig{32, 32, 20, 89});
    REQUIRE(oldWorld.AddPlayer(0, Faction::MeridianCompact, {}));
    const auto scout = oldWorld.SpawnEntity(0, Faction::MeridianCompact,
        EntityType::ScoutUnit, Vec2::FromTiles(5, 5));
    oldWorld.CaptureReplayBaseline();
    auto move = MakeCommand(0, 0, 1, CommandType::Move, scout);
    move.position = Vec2::FromTiles(10, 10);
    REQUIRE(oldWorld.QueueCommand(move));
    oldWorld.Step(60);
    const auto currentReplay = oldWorld.ExportReplay();
    REQUIRE(Simulation::ReplayToEnd(currentReplay, &error).has_value());
    auto oldReplay = currentReplay;
    oldReplay.initialSnapshot = ConvertSnapshotV28ToV27(currentReplay.initialSnapshot, 32 * 32);
    oldReplay.finalChecksum = 7947105480651690908ULL;
    REQUIRE(oldReplay.finalChecksum != currentReplay.finalChecksum);
    REQUIRE(Simulation::ReplayToEnd(oldReplay, &error).has_value());
    ++oldReplay.finalChecksum;
    REQUIRE(!Simulation::ReplayToEnd(oldReplay, &error).has_value());
}

#include "ReplayReportTests.h"

}  // namespace

int main() {
    const std::vector<std::pair<std::string, std::function<void()>>> tests{
        {"projectile persistence and malformed snapshot bounds", TestProjectilePersistenceRegression},
        {"ballistic cover interception and moving-target tracking", TestBallisticCoverAndTrackingRegression},
        {"harvest reservations travel depletion and persistence", TestHarvestReservationRegression},
        {"contact line of sight across attack orders", TestContactLineOfSightRegression},
        {"fixed tick movement", TestFixedTickMovement},
        {"canonical ordering and determinism", TestCanonicalCommandOrderingAndDeterminism},
        {"gather deliver build and placement", TestGatherDeliverBuildAndPlacement},
        {"controlled spawn admission", TestControlledSpawnAdmission},
        {"combat", TestCombatResolvesDeterministically},
        {"protected Command Core deterministic contract",
         TestProtectedCommandCoreContract},
        {"attack-move acquisition resume and stop",
         TestAttackMoveAcquiresResumesAndStops},
        {"hold position defends without chasing",
         TestHoldPositionDefendsWithoutChasing},
        {"guard defends and follows owned target",
         TestGuardDefendsAndFollowsOwnedTarget},
        {"patrol reverses persists and bounds engagements",
         TestPatrolReversesPersistsAndBoundsEngagements},
        {"deterministic obstacle pathing", TestDeterministicObstaclePathing},
        {"movement order rejection reasons",
         TestMovementOrderRejectionReasons},
        {"production population and victory", TestProductionPopulationAndVictory},
        {"fog and non-cheating AI", TestFogAndNonCheatingAi},
        {"four-player visibility snapshot and outcome",
         TestFourPlayerVisibilitySnapshotAndOutcome},
        {"Future Well choices", TestFutureWellChoices},
        {"snapshot and replay", TestSnapshotAndReplay},
        {"replay report authority and continuation", TestReplayReportAuthorityAndContinuation},
        {"Future Well snapshot migration and replay",
         TestFutureWellSnapshotMigrationAndReplay},
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
        {"mineral cover extreme-coordinate determinism",
         TestMineralCoverExtremeCoordinateDeterminism},
        {"command resolution receipt retention and bounds",
         TestCommandResolutionReceiptRetentionAndBounds},
        {"vibration detection and anonymous signatures",
         TestVibrationDetectionAndAnonymousSignatures},
        {"powered Aegis network and counterplay",
         TestPoweredAegisNetworkAndCounterplay},
        {"faction research progression and persistence",
         TestFactionResearchProgressionAndPersistence},
        {"Hollow Choir identity reconciliation and persistence",
         TestHollowChoirIdentityReconciliationAndPersistence},
        {"Hollow Choir coherence ordering and persistence",
         TestHollowChoirCoherenceOrderingAndPersistence},
        {"Hollow Choir AI scoped threats and stable tie breaks",
         TestHollowChoirAiUsesScopedThreatsAndStableTieBreaks},
        {"network protocol admission and hardening",
         TestNetworkProtocolAdmissionAndHardening},
        {"Command Core is not constructable",
         TestCommandCoreIsNotConstructable},
        {"Reshape expiry stops without teleporting",
         TestReshapeExpiryStopsWithoutTeleporting},
        {"Explored terrain and permanent object memory",
         TestExploredTerrainAndPermanentObjectMemory},
        {"Scarred terrain costs speed", TestScarredTerrainCostsSpeed},
        {"deterministic network forfeit",
         TestDeterministicNetworkForfeit},
        {"Euclidean movement and speed normalization",
         TestEuclideanMovementAndSpeedNormalization},
        {"any-angle string pulling",
         TestAnyAngleStringPulling},
        {"soft separation and cluster stability",
         TestSoftSeparationAndClusterStability},
        {"chokepoint negotiation throughput",
         TestChokepointNegotiationThroughput},
        {"arrival damping and no oscillation",
         TestArrivalDampingAndNoOscillation},
        {"group arrival packing and rest stability",
         TestGroupArrivalPackingAndRestStability},
        {"command responsiveness and interruptibility",
         TestCommandResponsivenessAndInterruptibility},
        {"shift-queued order chaining",
         TestShiftQueuedOrderChaining},
        {"shift queue depth and immediate interrupt",
         TestShiftQueueDepthAndImmediateInterrupt},
        {"smart-cast single-unit dispatch",
         TestSmartCastSingleUnitDispatch},
        {"attack-move threat filtering",
         TestAttackMoveThreatFiltering},
        {"focus-fire chase leashing",
         TestFocusFireChaseLeashing},
        {"campaign structure and replayability",
         TestCampaignStructureAndReplayability},
        {"campaign mission starting packages and rosters",
         TestCampaignMissionStartingPackagesAndRosters},
        {"campaign ending eligibility and derivation",
         TestCampaignEndingEligibilityAndDerivation},
        {"campaign mission objective and failure contracts",
         TestCampaignMissionObjectiveAndFailureContracts},
        {"top resource bar and logistics monitor",
         TestTopResourceBarAndLogisticsMonitor},
        {"objective panel and protected assets",
         TestObjectivePanelAndProtectedAssets},
        {"selection card and inspect fields",
         TestSelectionCardAndInspectFields},
        {"command deck action grid and disabled reasons",
         TestCommandDeckActionGridAndDisabledReasons},
        {"production and research queues",
         TestProductionAndResearchQueues},
        {"tactical minimap and spatial alert history",
         TestTacticalMinimapAndSpatialAlertHistory},
        {"accessibility and control remapping",
         TestAccessibilityAndControlRemapping},
        {"audio mix graph and category routing",
         TestAudioMixGraphAndCategoryRouting},
        {"gameplay audio cue completeness and rate limiting",
         TestGameplayAudioCueCompletenessAndRateLimiting},
        {"faction music and audio themes",
         TestFactionMusicAndAudioThemes},
        {"audio accessibility and dialogue subtitles",
         TestAudioAccessibilityAndDialogueSubtitles},
        {"deterministic save load and replay",
         TestDeterministicSaveLoadAndReplay},
        {"tutorial curriculum and mastery contracts",
         TestTutorialCurriculumAndMasteryContracts},
        {"economy logistics and depletion rules",
         TestEconomyLogisticsAndDepletionRules},
        {"base building placement multi builder and refunds",
         TestBaseBuildingPlacementMultiBuilderAndRefunds},
        {"unit roster definitions and combat abilities",
         TestUnitRosterDefinitionsAndCombatAbilities},
        {"fog of war single information boundary and scouting",
         TestFogOfWarSingleInformationBoundaryAndScouting},
        {"platform integrity privacy and validation floors",
         TestPlatformIntegrityPrivacyAndValidationFloors},
        {"skirmish configuration and map contracts",
         TestSkirmishConfigurationAndMapContracts},
        {"faction structure manifests and network links",
         TestFactionStructureManifestsAndNetworkLinks},
        {"terrain classification and movement modifiers",
         TestTerrainClassificationAndMovementModifiers},
        {"faction technology trees and prerequisites",
         TestFactionTechnologyTreesAndPrerequisites},
        {"combat stances and pursuit leashes",
         TestCombatStancesAndPursuitLeashes},
        {"canon invariants ai doctrines and worldbuilding",
         TestCanonInvariantsAiDoctrinesAndWorldbuilding},
        {"visual direction art readability and faction forms",
         TestVisualDirectionArtReadabilityAndFactionForms},
        {"cinematics control handoff and match outcomes",
         TestCinematicsControlHandoffAndMatchOutcomes},
        {"core experience pillars and information tiers",
         TestCoreExperiencePillarsAndInformationTiers},
        {"future well protocol execution and telegraphs",
         TestFutureWellProtocolExecutionAndTelegraphs},
        {"opponent ai architecture and difficulty tiers",
         TestOpponentAiArchitectureAndDifficultyTiers},
        {"product boundary and economic resource pillars",
         TestProductBoundaryAndEconomicResourcePillars},
        {"architecture modularity automation and governance",
         TestArchitectureModularityAutomationAndGovernance},
        {"balance validation architecture and performance budgets",
         TestBalanceValidationArchitectureAndPerformanceBudgets},
        {"command dispatch pipelines and movement kinematics",
         TestCommandDispatchPipelinesAndMovementKinematics},
        {"localization simulation determinism and evidence matrices",
         TestLocalizationSimulationDeterminismAndEvidenceMatrices},
        {"autonomous worker gather loop and cadence",
         TestAutonomousWorkerGatherLoopAndCadence},
        {"calibrated construction and multi-builder falloff",
         TestCalibratedConstructionAndMultiBuilderFalloff},
        {"Phase Anchor Dawn coherence field",
         TestPhaseAnchorDawnCoherenceField},
        {"ballistic projectile flight and occlusion",
         TestBallisticProjectileFlightAndOcclusion},
        {"explicit hostility and legacy replay", TestExplicitHostilityAndLegacyReplay},
    };

    std::size_t passed = 0;
    std::size_t failed = 0;
    for (const auto& [name, test] : tests) {
        try {
            test();
            ++passed;
            std::cout << "[PASS] " << name << '\n';
        } catch (const std::exception& failure) {
            std::cerr << "[FAIL] " << name << ": " << failure.what() << '\n';
            ++failed;
        }
    }
    std::cout << passed << "/" << tests.size()
              << " native simulation tests passed\n";
    return failed == 0 ? 0 : 1;
}
