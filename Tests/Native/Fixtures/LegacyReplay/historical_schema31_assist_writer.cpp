// Author: Angelis Pseftis
// Compile against the captured candidate36 Simulation.cpp and Simulation.h.
// The receipt must bind those actual source hashes; never relabel a new writer.
#include "EchoesSimCore/Simulation.h"
#include <fstream>
#include <iostream>
using namespace echoes::sim;

int main(int argc, char** argv) {
    if (argc != 2 || kSnapshotVersion != 31 || kReplayVersion != 28) return 2;
    Simulation sim({32, 32, 20, 0x43414e43454c4153ULL});
    if (!sim.AddPlayer(0, Faction::MeridianCompact, {2000, 500}) ||
        !sim.AddPlayer(1, Faction::KharuunAssemblies, {2000, 500})) return 3;
    const auto core = sim.SpawnEntity(0, Faction::MeridianCompact,
        EntityType::CommandCore, Vec2::FromTiles(4, 4));
    const auto builder = sim.SpawnEntity(0, Faction::MeridianCompact,
        EntityType::Worker, Vec2::FromTiles(7, 8));
    const auto assistant = sim.SpawnEntity(0, Faction::MeridianCompact,
        EntityType::Worker, Vec2::FromTiles(8, 7));
    const auto staleAssistant = sim.SpawnEntity(0, Faction::MeridianCompact,
        EntityType::Worker, Vec2::FromTiles(7, 12));
    if (!core || !builder || !assistant || !staleAssistant) return 4;
    sim.CaptureReplayBaseline();
    Command build{};
    build.player = 0; build.sequence = 1; build.type = CommandType::Build;
    build.actor = builder; build.buildType = EntityType::Dropoff;
    build.position = Vec2::FromTiles(8, 8);
    if (!sim.QueueCommand(build)) return 5;
    sim.Step();
    const auto view = sim.CreatePlayerView(0);
    if (!view || view->ConstructionReceipts().empty()) return 6;
    const auto site = view->ConstructionReceipts().front().structure;
    const auto buildChecksum = sim.ReplayStateChecksum();
    Command cancel{};
    cancel.executeTick = 1; cancel.player = 0; cancel.sequence = 2;
    cancel.type = CommandType::CancelConstruction; cancel.actor = site;
    Command assist = build;
    assist.executeTick = 1; assist.sequence = 3; assist.actor = assistant;
    assist.target = site;
    if (!sim.QueueCommand(cancel) || !sim.QueueCommand(assist)) return 7;
    sim.Step();
    const auto* revived = sim.FindEntity(site);
    const auto* helper = sim.FindEntity(assistant);
    if (!revived || revived->hitPoints <= 0 || !helper ||
        helper->order.type != OrderType::Build) return 8;
    const auto resurrectionChecksum = sim.ReplayStateChecksum();
    const auto revivedHealth = revived->hitPoints;
    const auto refunded = sim.FindPlayer(0)->resources;
    Command stale = build;
    stale.executeTick = 2; stale.sequence = 4; stale.actor = staleAssistant;
    stale.target = site + 100; stale.position = Vec2::FromTiles(8, 12);
    if (!sim.QueueCommand(stale)) return 11;
    sim.Step();
    const auto finalView = sim.CreatePlayerView(0);
    if (!finalView || finalView->ConstructionReceipts().empty()) return 12;
    EntityId staleCreated = 0;
    for (const auto& receipt : finalView->ConstructionReceipts()) {
        if (receipt.transition == ConstructionTransition::Created &&
            receipt.commandSequence == stale.sequence) staleCreated = receipt.structure;
    }
    if (!staleCreated || sim.FindPlayer(0)->resources.material >= refunded.material)
        return 13;
    std::string error;
    const auto replay = sim.ExportReplay(&error);
    const auto verified = Simulation::ReplayToEnd(replay, &error);
    if (!verified || !error.empty() ||
        verified->ReplayStateChecksum() != replay.finalChecksum) return 9;
    auto continued = Simulation::LoadSnapshot(sim.SaveSnapshot(), &error);
    if (!continued || !continued->ContinueReplayRecording(replay, &error)) return 14;
    cancel.executeTick = continued->CurrentTick(); cancel.sequence = 5;
    assist.executeTick = continued->CurrentTick(); assist.sequence = 6;
    if (!continued->QueueCommand(cancel) || !continued->QueueCommand(assist)) return 15;
    continued->Step();
    if (!continued->FindEntity(site) || continued->FindEntity(site)->hitPoints <= 0 ||
        continued->ExportReplay().version != 28) return 16;
    const auto continuation = continued->ExportReplay(&error);
    if (!Simulation::ReplayToEnd(continuation, &error)) return 17;
    std::ofstream output(argv[1], std::ios::binary);
    output.write(reinterpret_cast<const char*>(replay.initialSnapshot.data()),
        static_cast<std::streamsize>(replay.initialSnapshot.size()));
    if (!output) return 10;
    std::cout << "{\"snapshot_version\":31,\"replay_version\":28,"
        "\"final_tick\":3,\"final_checksum\":" << replay.finalChecksum
        << ",\"build_tick1_checksum\":" << buildChecksum
        << ",\"revived_site\":" << site
        << ",\"revived_health\":" << revivedHealth
        << ",\"resurrection_tick2_checksum\":" << resurrectionChecksum
        << ",\"stale_created_site\":" << staleCreated
        << ",\"refunded_material\":" << refunded.material
        << ",\"refunded_dawnshards\":" << refunded.dawnshards
        << ",\"final_material\":" << sim.FindPlayer(0)->resources.material
        << ",\"final_dawnshards\":" << sim.FindPlayer(0)->resources.dawnshards
        << ",\"continued_tick4_checksum\":" << continuation.finalChecksum
        << ",\"self_verified\":true}\n";
}
