// Author: Angelis Pseftis
// Compile only against the hash-verified archived schema-29 writer.
#include "EchoesSimCore/Simulation.h"
#include <fstream>
#include <iostream>
using namespace echoes::sim;
int main(int argc, char** argv) {
    if (argc != 2 || kSnapshotVersion != 29 || kReplayVersion != 26) return 2;
    SimulationConfig config{32, 32, 20, 0x2926};
    config.rules.poweredAegis.connectionRadiusRaw = 8 * kFixedScale;
    Simulation sim(config);
    if (!sim.AddPlayer(0, Faction::MeridianCompact, {1000, 100})) return 3;
    const auto core = sim.SpawnEntity(0, Faction::MeridianCompact,
        EntityType::CommandCore, Vec2::FromTiles(4, 4));
    const auto foundry = sim.SpawnEntity(0, Faction::MeridianCompact,
        EntityType::Barracks, Vec2::FromTiles(10, 4));
    const auto aegis = sim.SpawnEntity(0, Faction::MeridianCompact,
        EntityType::UtilityStructure, Vec2::FromTiles(16, 4));
    if (!core || !foundry || !aegis || sim.FindEntity(aegis)->aegisPowered) return 4;
    sim.CaptureReplayBaseline();
    std::string error;
    const auto replay = sim.ExportReplay(&error);
    const auto verified = Simulation::ReplayToEnd(replay, &error);
    if (!verified || !error.empty() || verified->ReplayStateChecksum() != replay.finalChecksum) {
        std::cerr << error; return 5;
    }
    std::ofstream output(argv[1], std::ios::binary);
    output.write(reinterpret_cast<const char*>(replay.initialSnapshot.data()),
        replay.initialSnapshot.size());
    if (!output) return 6;
    std::cout << "{\"snapshot_version\":29,\"replay_version\":26,\"final_tick\":0,\"final_checksum\":"
        << replay.finalChecksum << ",\"aegis_id\":" << aegis << ",\"self_verified\":true}\n";
    for (std::uint64_t sequence = 1; sequence <= 2; ++sequence) {
        Command command{};
        command.player = 0; command.sequence = sequence;
        command.type = CommandType::Produce; command.actor = foundry;
        command.buildType = EntityType::Soldier;
        if (!sim.QueueCommand(command, &error)) return 7;
    }
    sim.Step();
    const auto queue = sim.ProducerQueueStateFor(0, foundry);
    if (!queue || !queue->active || queue->waiting.size() != 1) return 8;
    const auto production = sim.SaveSnapshot();
    const auto loaded = Simulation::LoadSnapshot(production, &error);
    if (!loaded || loaded->SaveSnapshot() != production) return 9;
    std::ofstream productionOutput(std::string(argv[1]) + ".production", std::ios::binary);
    productionOutput.write(reinterpret_cast<const char*>(production.data()), production.size());
    if (!productionOutput) return 10;
}
