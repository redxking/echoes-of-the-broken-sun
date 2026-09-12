// Author and owner: Angelis Pseftis
// Standalone regression; deliberately separate from the shared dirty harness.
#include "EchoesSimCore/Simulation.h"
#include "EchoesSimCore/SkirmishMapPresets.h"
#include <iostream>
#include <stdexcept>
#include <vector>

using namespace echoes::sim;
static void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
static void Put(std::vector<std::uint8_t>& bytes, std::size_t offset,
                std::uint64_t value, std::size_t count) {
    for (std::size_t n = 0; n < count; ++n) bytes[offset + n] = value >> (8 * n);
}
static void Resign(std::vector<std::uint8_t>& bytes) {
    std::uint64_t hash = 14695981039346656037ULL;
    for (std::size_t n = 0; n < bytes.size() - 8; ++n) {
        hash ^= bytes[n]; hash *= 1099511628211ULL;
    }
    Put(bytes, bytes.size() - 8, hash, 8);
}
static std::vector<std::uint8_t> Legacy32(const Simulation& sim) {
    auto bytes = sim.SaveSnapshot();
    const auto tail = 4 + 24 * sim.ReopenedReshapeTerrain().size();
    bytes.erase(bytes.end() - 8 - tail, bytes.end() - 8);
    Put(bytes, 4, 32, 4); Resign(bytes); return bytes;
}

void RunReshapeCheckpointRegression() {
        Simulation sim({64, 64, 20, 0x52455348415045ULL});
        Require(sim.AddPlayer(0, Faction::MeridianCompact, {0, 200}), "player");
        for (int y = 0; y < 64; ++y)
            for (int x = 0; x < 64; ++x)
                Require(sim.SetTerrainTile(x, y, IsSkirmishBlockedTile(
                    SkirmishMapPreset::GlassScar, x, y) ? Terrain::Blocked : Terrain::Open), "terrain");
        const auto well = sim.SpawnFutureWell(Vec2::FromTiles(20, 32));
        const auto worker = sim.SpawnEntity(0, Faction::MeridianCompact,
            EntityType::Worker, Vec2::FromTiles(20, 29));
        Command reshape{};
        reshape.player = 0; reshape.sequence = 1; reshape.type = CommandType::FutureWell;
        reshape.actor = worker; reshape.target = well; reshape.wellChoice = FutureWellChoice::Reshape;
        Require(sim.QueueCommand(reshape), "reshape command");
        sim.Step(480);
        Require(sim.FindEntity(well)->reshapeUntilTick > sim.CurrentTick(), "active reshape");
        const auto soldier = sim.SpawnEntity(0, Faction::MeridianCompact,
            EntityType::Soldier, Vec2::FromTiles(20, 32));
        Require(soldier != 0, "soldier");
        sim.CaptureReplayBaseline();
        const auto activeLegacy = Legacy32(sim);
        const auto expiry = sim.FindEntity(well)->reshapeUntilTick;
        sim.Step(expiry - sim.CurrentTick());
        Require(sim.TerrainAt(20, 32) == Terrain::Open, "expiry must reopen authored blocked tile");
        const auto saved = sim.SaveSnapshot();
        std::string error;
        auto restored = Simulation::LoadSnapshot(saved, &error);
        Require(restored.has_value(), error.c_str());
        Require(restored->CurrentTick() == sim.CurrentTick(), "exact tick");
        Require(restored->StateChecksum() == sim.StateChecksum(), "exact checksum");
        Require(restored->FindEntity(soldier)->position == sim.FindEntity(soldier)->position, "exact position");
        auto replayed = Simulation::ReplayToEnd(sim.ExportReplay(), &error);
        Require(replayed.has_value(), error.c_str());
        Require(replayed->StateChecksum() == sim.StateChecksum(), "expiry replay checksum");
        const auto history = sim.ReopenedReshapeTerrain();
        Require(history.size() == 1 && history[0].tileIndex == 32 * 64 + 20 &&
            history[0].wellId == well && history[0].expiryTick == expiry, "exact expiry history");
        Require(restored->CheckpointBindingTerrainAt(20, 32) == Terrain::Blocked,
            "authored blocked ground reconstructed");
        Require(restored->ReopenedReshapeTerrain()[0] == history[0], "history serialized exactly");

        // A nearby missing blocked tile is not justified by being in the
        // same expired Well footprint. Explicit edits invalidate old history.
        Simulation unrelated = *restored;
        Require(unrelated.SetTerrainTile(20, 31, Terrain::Open), "unrelated edit");
        Require(unrelated.CheckpointBindingTerrainAt(20, 31) == Terrain::Open,
            "unrecorded nearby opening has no binding exemption");
        Require(unrelated.SetTerrainTile(20, 32, Terrain::Open), "same-value edit");
        Require(unrelated.ReopenedReshapeTerrain().empty(), "explicit edit clears history");
        Require(unrelated.CheckpointBindingTerrainAt(INT32_MAX, INT32_MAX) == Terrain::Blocked,
            "binding accessor rejects extreme out-of-map coordinates safely");

        const auto ledger = saved.size() - 8 - 24;
        auto RejectMutation = [&](std::size_t offset, std::uint64_t value,
                                  std::size_t count, const char* message) {
            auto damaged = saved; Put(damaged, offset, value, count); Resign(damaged);
            Require(!Simulation::LoadSnapshot(damaged, &error).has_value(), message);
            Require(error.find("Reshape terrain history") != std::string::npos,
                "malformed history fails semantic validation");
        };
        RejectMutation(ledger - 4, 4097, 4, "oversized history count");
        RejectMutation(ledger, 4096, 4, "out-of-map tile");
        RejectMutation(ledger + 4, worker, 4, "live non-Well source");
        RejectMutation(ledger + 8, 0xffffffffU, 4, "negative source center");
        RejectMutation(ledger + 8, 25, 4, "tile outside source footprint");
        RejectMutation(ledger + 16, expiry + 1, 8, "future expiry tick");
        RejectMutation(ledger + 16, 0, 8, "zero expiry tick");
        auto duplicate = saved;
        duplicate.insert(duplicate.end() - 8, saved.begin() + ledger, saved.end() - 8);
        Put(duplicate, ledger - 4, 2, 4); Resign(duplicate);
        Require(!Simulation::LoadSnapshot(duplicate, &error), "duplicate history tile");

        // A projected older-layout fixture still hashes schema-32 bytes,
        // while resimulation can prove omitted historical terrain metadata.
        ReplayRecord legacy{};
        legacy.version = kFutureWellCaptureGeometryReplayVersion;
        legacy.initialSnapshot = activeLegacy;
        legacy.finalTick = expiry;
        auto legacyRun = Simulation::BeginReplaySimulation(legacy, &error);
        Require(legacyRun.has_value(), error.c_str());
        legacyRun->Step(expiry - legacyRun->CurrentTick());
        legacy.finalChecksum = legacyRun->ReplayStateChecksum();
        auto legacyReplay = Simulation::ReplayToEnd(legacy, &error);
        Require(legacyReplay.has_value(), error.c_str());
        auto oldExpired = Simulation::LoadSnapshot(Legacy32(*legacyReplay), &error);
        Require(oldExpired.has_value(), error.c_str());
        Require(oldExpired->CheckpointBindingTerrainAt(20, 32) == Terrain::Open,
            "unbound legacy opening does not invent history");
        Require(oldExpired->ContinueReplayRecording(legacy, &error), error.c_str());
        Require(oldExpired->StateChecksum() == legacyReplay->StateChecksum(),
            "verified old prefix reconstructs omitted history exactly");
        Require(oldExpired->ExportReplay().version == legacy.version &&
            oldExpired->ExportReplay().initialSnapshot == legacy.initialSnapshot &&
            oldExpired->ExportReplay().finalChecksum == legacy.finalChecksum,
            "legacy recording identity and checksum preserved");
        auto mismatchedHistory = saved;
        Put(mismatchedHistory, ledger + 16, expiry - 1, 8); Resign(mismatchedHistory);
        auto mismatched = Simulation::LoadSnapshot(mismatchedHistory, &error);
        Require(mismatched.has_value(), "structurally valid alternate history");
        Require(!mismatched->ContinueReplayRecording(legacy, &error),
            "nonempty history must match verified prefix exactly");
        Simulation cleared = *oldExpired;
        Require(cleared.SetTerrainTile(20, 32, Terrain::Open), "clear migrated history");
        Require(!cleared.ContinueReplayRecording(legacy, &error),
            "same-object explicit edit cannot resurrect hydrated history");
        auto clearedSave = Simulation::LoadSnapshot(cleared.SaveSnapshot(), &error);
        Require(clearedSave.has_value(), error.c_str());
        Require(!clearedSave->ContinueReplayRecording(legacy, &error),
            "native33 deliberately absent history cannot be hydrated from legacy prefix");
        auto oldActive = Simulation::LoadSnapshot(activeLegacy, &error);
        Require(oldActive.has_value(), error.c_str());
        auto activePrefix = legacy;
        activePrefix.finalTick = oldActive->CurrentTick();
        auto activePlayback = Simulation::BeginReplaySimulation(activePrefix, &error);
        Require(activePlayback.has_value(), error.c_str());
        activePrefix.finalChecksum = activePlayback->ReplayStateChecksum();
        Require(oldActive->ContinueReplayRecording(activePrefix, &error), error.c_str());
        oldActive->Step(expiry - oldActive->CurrentTick());
        auto oldContinuedSave = Simulation::LoadSnapshot(oldActive->SaveSnapshot(), &error);
        Require(oldContinuedSave.has_value(), error.c_str());
        Require(oldContinuedSave->ContinueReplayRecording(oldActive->ExportReplay(), &error), error.c_str());
        Require(oldContinuedSave->StateChecksum() == oldActive->StateChecksum(),
            "legacy pre-expiry session saves new expiry metadata and retains history");
        // Cover may temporarily block a reopened tile without replacing the
        // permanent expiry history, and may later return it to open ground.
        auto covered = Simulation::LoadSnapshot(activeLegacy, &error);
        Require(covered.has_value(), error.c_str());
        const_cast<Entity*>(covered->FindEntity(soldier))->position = Vec2::FromTiles(21, 32);
        covered->CaptureReplayBaseline();
        covered->Step(expiry - covered->CurrentTick());
        Require(covered->ReopenedReshapeTerrain().size() == 1, "adjacent expiry history");
        const auto coverHistory = covered->ReopenedReshapeTerrain()[0];
        const_cast<Entity*>(covered->FindEntity(soldier))->position = Vec2::FromTiles(21, 29);
        Require(covered->AddPlayer(1, Faction::KharuunAssemblies, {0, 300}), "cover player");
        const auto heavy = covered->SpawnEntity(1, Faction::KharuunAssemblies,
            EntityType::HeavyUnit, Vec2::FromTiles(23, 29));
        const auto rejectedSite = covered->ValidateMineralCover(1, heavy, Vec2::FromTiles(21, 32));
        Require(rejectedSite == MineralCoverResult::Occupied,
            "integer cover anchor overlaps the Well's footprint");
        // The cover half extent is 0.75 tiles and the Well's is 0.5;
        // the integer anchor only one tile away overlaps. The centre of the
        // same reopened tile is 1.5 tiles away and clears that footprint.
        const Vec2 coverSite = Vec2::FromRaw(21 * kFixedScale + kFixedScale / 2,
            32 * kFixedScale + kFixedScale / 2);
        const auto coverResult = covered->ValidateMineralCover(1, heavy, coverSite);
        if (coverResult != MineralCoverResult::Valid)
            throw std::runtime_error("reopened tile cover validation result=" +
                std::to_string(static_cast<unsigned>(coverResult)));
        std::cout << "cover_validation integer=" << static_cast<unsigned>(rejectedSite)
            << " centre=" << static_cast<unsigned>(coverResult) << '\n';
        covered->CaptureReplayBaseline();
        Command cover{};
        cover.player = 1; cover.sequence = 1; cover.actor = heavy;
        cover.executeTick = covered->CurrentTick(); cover.type = CommandType::RaiseMineralCover;
        cover.position = coverSite;
        Require(covered->QueueCommand(cover), "cover command");
        covered->Step();
        Require(covered->TerrainAt(21, 32) == Terrain::Blocked, "live cover blocks reopened tile");
        Require(covered->ReopenedReshapeTerrain()[0] == coverHistory,
            "cover creation preserves original expiry history");
        // Keep this lifecycle fixture out of combat after invocation.
        const_cast<Entity*>(covered->FindEntity(soldier))->position = Vec2::FromTiles(10, 10);
        const_cast<Entity*>(covered->FindEntity(heavy))->position = Vec2::FromTiles(55, 55);
        covered->Step();
        auto coverSave = Simulation::LoadSnapshot(covered->SaveSnapshot(), &error);
        Require(coverSave.has_value(), error.c_str());
        Require(coverSave->StateChecksum() == covered->StateChecksum(), "live cover/history snapshot exact");
        Require(coverSave->CheckpointBindingTerrainAt(21, 32) == Terrain::Blocked,
            "live cover unwraps to recorded original blocked terrain");
        coverSave->Step(coverSave->Config().rules.mineralCover.durationTicks);
        Require(coverSave->TerrainAt(21, 32) == Terrain::Open, "cover expiry restores reopened ground");
        Require(coverSave->ReopenedReshapeTerrain()[0] == coverHistory &&
            coverSave->CheckpointBindingTerrainAt(21, 32) == Terrain::Blocked,
            "cover removal preserves authored terrain binding history");
        Require(Simulation::LoadSnapshot(coverSave->SaveSnapshot(), &error).has_value(), error.c_str());

        // The rescue branch itself must inspect permanent ground underneath
        // cover: temporary blocking of open ground creates no new history.
        auto openRescue = Simulation::LoadSnapshot(activeLegacy, &error);
        Require(openRescue.has_value(), error.c_str());
        Require(openRescue->SetTerrainTile(21, 32, Terrain::Open), "controlled underlying open tile");
        const_cast<Entity*>(openRescue->FindEntity(soldier))->position = Vec2::FromTiles(10, 10);
        openRescue->Step(expiry - 2 - openRescue->CurrentTick());
        Require(openRescue->AddPlayer(1, Faction::KharuunAssemblies, {0, 300}), "rescue cover player");
        const auto rescueHeavy = openRescue->SpawnEntity(1, Faction::KharuunAssemblies,
            EntityType::HeavyUnit, Vec2::FromTiles(23, 29));
        cover.actor = rescueHeavy; cover.executeTick = openRescue->CurrentTick();
        Require(openRescue->QueueCommand(cover), "rescue cover command");
        openRescue->Step();
        Require(openRescue->TerrainAt(21, 32) == Terrain::Blocked, "cover over permanent open ground");
        Require(openRescue->SpawnEntity(0, Faction::MeridianCompact,
            EntityType::Worker, Vec2::FromTiles(21, 32)) != 0, "unit enters covered active passage");
        const_cast<Entity*>(openRescue->FindEntity(rescueHeavy))->position = Vec2::FromTiles(55, 55);
        openRescue->Step();
        Require(openRescue->TerrainAt(21, 32) == Terrain::Open &&
            openRescue->ReopenedReshapeTerrain().empty(),
            "rescue of temporary blocker never claims original blocked ground");

        // A second Reshape expiry under cover on previously reopened ground
        // keeps the original source and tick rather than replacing history.
        auto repeatedRescue = Simulation::LoadSnapshot(covered->SaveSnapshot(), &error);
        Require(repeatedRescue.has_value(), error.c_str());
        const auto secondWell = repeatedRescue->SpawnFutureWell(Vec2::FromTiles(22, 32));
        auto* secondState = const_cast<Entity*>(repeatedRescue->FindEntity(secondWell));
        secondState->owner = 0; secondState->wellChoice = FutureWellChoice::Reshape;
        secondState->wellActivationTick = repeatedRescue->CurrentTick();
        secondState->reshapeUntilTick = repeatedRescue->CurrentTick() + 1;
        Require(repeatedRescue->SpawnEntity(0, Faction::MeridianCompact,
            EntityType::Worker, Vec2::FromTiles(21, 32)) != 0, "covered previously reopened passage occupied");
        repeatedRescue->Step();
        Require(repeatedRescue->TerrainAt(21, 32) == Terrain::Open &&
            repeatedRescue->ReopenedReshapeTerrain().size() == 1 &&
            repeatedRescue->ReopenedReshapeTerrain()[0] == coverHistory,
            "repeat rescue preserves exact original source and expiry tick");
        // Existing cover state expects blocked terrain while it lives; after
        // its timer expires the retained original history remains loadable.
        repeatedRescue->Step(repeatedRescue->Config().rules.mineralCover.durationTicks);
        Require(repeatedRescue->ReopenedReshapeTerrain()[0] == coverHistory &&
            Simulation::LoadSnapshot(repeatedRescue->SaveSnapshot(), &error).has_value(),
            "repeat rescue/cover expiry retains serializable original history");
        std::cout << "PASS ordinary cover creation/save/expiry over reopened ground\n";
        std::cout << "PASS semantic history negatives and legacy-prefix recovery\n";
        std::cout << "expiry=" << expiry << " tick=" << sim.CurrentTick()
                  << " checksum=" << sim.StateChecksum() << " authored_blocked=1 restored_open=1\n";
        // Engine integration regression exercises the real setup validator.
        std::cout << "PASS native expiry serialization and replay\n";
}

#ifndef ECHOES_RESHAPE_CHECKPOINT_EMBEDDED
int main() {
    try {
        RunReshapeCheckpointRegression();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL " << error.what() << '\n'; return 1;
    }
}
#endif
