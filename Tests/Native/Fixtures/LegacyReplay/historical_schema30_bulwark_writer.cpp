// Author: Angelis Pseftis
// Run against the archived pre-repair candidate28 simulation, never a projection
// of a newer writer. The receipt binds its dirty source hashes and baseline.
#include "EchoesSimCore/Simulation.h"
#include <fstream>
#include <iostream>
using namespace echoes::sim;

int main(int argc, char** argv) {
    if (argc != 2 || kSnapshotVersion != 30 || kReplayVersion != 27) return 2;
    Simulation simulation({24, 24, 20, 0x42554c5741524bULL});
    if (!simulation.AddPlayer(0, Faction::MeridianCompact, {0, 0}) ||
        !simulation.AddPlayer(1, Faction::KharuunAssemblies, {0, 0})) return 3;
    const auto bulwark = simulation.SpawnEntity(0, Faction::MeridianCompact,
        EntityType::HeavyUnit, Vec2::FromTiles(10, 10));
    const auto lancer = simulation.SpawnEntity(0, Faction::MeridianCompact,
        EntityType::Soldier, Vec2::FromRaw(9 * kFixedScale + kFixedScale / 2,
                                        10 * kFixedScale));
    const auto enemy = simulation.SpawnEntity(1, Faction::KharuunAssemblies,
        EntityType::Soldier, Vec2::FromTiles(11, 10));
    if (!bulwark || !lancer || !enemy) return 4;
    simulation.CaptureReplayBaseline();
    Command deploy{};
    deploy.player = 0; deploy.sequence = 1;
    deploy.type = CommandType::ToggleDeploy; deploy.actor = bulwark;
    deploy.position = Vec2::FromTiles(12, 10);
    Command attack{};
    attack.player = 1; attack.sequence = 1;
    attack.type = CommandType::Attack; attack.actor = enemy; attack.target = lancer;
    if (!simulation.QueueCommand(deploy) || !simulation.QueueCommand(attack)) return 5;
    simulation.Step();
    if (!simulation.FindEntity(bulwark) || !simulation.FindEntity(bulwark)->deployed)
        return 6;
    const auto deployedChecksum = simulation.ReplayStateChecksum();
    const auto coveredHealth = simulation.FindEntity(lancer)->hitPoints;
    Command pack = deploy;
    pack.executeTick = 1; pack.sequence = 2;
    if (!simulation.QueueCommand(pack)) return 7;
    simulation.Step();
    if (simulation.FindEntity(bulwark)->deployed) return 8;
    std::string error;
    const auto replay = simulation.ExportReplay(&error);
    const auto verified = Simulation::ReplayToEnd(replay, &error);
    if (!verified || !error.empty() ||
        verified->ReplayStateChecksum() != replay.finalChecksum) return 9;
    std::ofstream output(argv[1], std::ios::binary);
    output.write(reinterpret_cast<const char*>(replay.initialSnapshot.data()),
                 replay.initialSnapshot.size());
    if (!output) return 10;
    std::cout << "{\"snapshot_version\":30,\"replay_version\":27,"
        "\"final_tick\":2,\"final_checksum\":" << replay.finalChecksum
        << ",\"deployed_tick1_checksum\":" << deployedChecksum
        << ",\"covered_lancer_health\":" << coveredHealth
        << ",\"self_verified\":true}\n";
}
