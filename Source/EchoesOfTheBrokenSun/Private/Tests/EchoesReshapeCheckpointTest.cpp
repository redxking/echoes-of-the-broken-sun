// Author and owner: Angelis Pseftis
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "EchoesTestSaveEnvironment.h"
#include "EchoesSnapshotMigrationTestHelpers.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesSkirmishSetup.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

bool EchoesValidateSkirmishSnapshotBindingForTesting(
    const echoes::sim::Simulation&, const FEchoesSkirmishSetup&, FString&);

namespace {
std::vector<uint8> ToLegacy32(const echoes::sim::Simulation& Sim) {
    auto Bytes = Sim.SaveSnapshot();
    const auto Tail = 4 + 24 * Sim.ReopenedReshapeTerrain().size();
    Bytes.erase(Bytes.end() - 8 - Tail, Bytes.end() - 8);
    Bytes[4] = 32; Bytes[5] = 0; Bytes[6] = 0; Bytes[7] = 0;
    uint64 Integrity = 14695981039346656037ULL;
    for (size_t Index = 0; Index < Bytes.size() - 8; ++Index) {
        Integrity ^= Bytes[Index]; Integrity *= 1099511628211ULL;
    }
    for (size_t Index = 0; Index < 8; ++Index)
        Bytes[Bytes.size() - 8 + Index] = Integrity >> (Index * 8);
    return Bytes;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEchoesReshapeCheckpointTest,
    "Echoes.Runtime.Persistence.ReshapeCheckpoint",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesReshapeCheckpointTest::RunTest(const FString& Parameters) {
    (void)Parameters;
    using namespace echoes::sim;
    FEchoesScopedTestSaveEnvironment SaveEnvironment(*this);
    if (!SaveEnvironment.IsReady()) return false;
    FTestWorldWrapper World;
    if (!World.CreateTestWorld(EWorldType::Game)) return false;
    auto* Bridge = World.GetTestWorld()->GetSubsystem<UEchoesSimulationSubsystem>();
    if (!TestNotNull(TEXT("Bridge"), Bridge)) return false;
    FString Feedback;
    FEchoesSkirmishSetup Setup;
    Setup.ResourceLevel = EEchoesSkirmishResourceLevel::Abundant;
    if (!TestTrue(TEXT("Start"), Bridge->StartPrototypeScenario()) ||
        !TestTrue(TEXT("Setup"), Bridge->ApplySkirmishSetup(Setup, Feedback))) return false;
    // Controlled fixture uses the shipping Glass Scar grid without changing
    // one tile. This extra Well sits in its five-tile-thick authored scar.
    auto* Sim = const_cast<Simulation*>(Bridge->GetSimulation());
    // Abundant currently provides 60 Dawn; the pinned Reshape cost is 120.
    // Fund this isolated fixture explicitly rather than changing the setup.
    const_cast<PlayerState*>(Sim->FindPlayer(0))->resources.dawnshards = 200;
    const auto Well = Sim->SpawnFutureWell(Vec2::FromTiles(20, 32));
    const auto Worker = Sim->SpawnEntity(0, Setup.LocalFaction, EntityType::Worker,
        Vec2::FromTiles(20, 29));
    Command Reshape{};
    Reshape.player = 0; Reshape.sequence = 1; Reshape.actor = Worker;
    Reshape.type = CommandType::FutureWell; Reshape.target = Well;
    Reshape.wellChoice = FutureWellChoice::Reshape;
    if (!TestTrue(TEXT("Reshape command"), Sim->QueueCommand(Reshape))) return false;
    Sim->Step(480);
    const auto Expiry = Sim->FindEntity(Well)->reshapeUntilTick;
    if (!TestTrue(TEXT("Active manifestation"), Expiry > Sim->CurrentTick())) return false;
    const auto Soldier = Sim->SpawnEntity(0, Setup.LocalFaction, EntityType::Soldier,
        Vec2::FromTiles(20, 32));
    Sim->CaptureReplayBaseline();
    const auto BeforeExpiry = Sim->SaveSnapshot();
    if (!TestTrue(TEXT("Active checkpoint saves"), Bridge->QuickSaveScenario(Feedback))) return false;
    const auto ActiveSavedTick = Sim->CurrentTick();
    Sim->Step(3);
    TestTrue(TEXT("Active live state advances after saving"), Sim->CurrentTick() > ActiveSavedTick);
    if (!TestTrue(TEXT("Active checkpoint loads"), Bridge->QuickLoadScenario(Feedback))) return false;
    Sim = const_cast<Simulation*>(Bridge->GetSimulation());
    TestTrue(TEXT("Active checkpoint exact snapshot"), Sim->SaveSnapshot() == BeforeExpiry);
    Sim->Step(Expiry - Sim->CurrentTick());
    if (!TestTrue(TEXT("Emergency fallback opens authored blocked cell"),
        Sim->TerrainAt(20, 32) == Terrain::Open)) return false;
    const uint64 Tick = Sim->CurrentTick(), Checksum = Sim->StateChecksum();
    const auto Position = Sim->FindEntity(Soldier)->position;
    const auto Snapshot = Sim->SaveSnapshot();
    auto LossyProjection = Snapshot;
    TestFalse(TEXT("Older format projection refuses populated terrain history"),
        EchoesSnapshotMigrationTestHelpers::ProjectSnapshotBufferToV31(LossyProjection));
    TestTrue(TEXT("Refused projection preserves original bytes"), LossyProjection == Snapshot);
    if (!TestTrue(TEXT("Expired checkpoint saves"), Bridge->QuickSaveScenario(Feedback))) return false;
    Sim->Step(3);
    TestTrue(TEXT("Expired live state advances after saving"), Sim->CurrentTick() > Tick);
    if (!TestTrue(TEXT("Expired checkpoint loads"), Bridge->QuickLoadScenario(Feedback)))
    {
        AddError(Feedback);
        return false;
    }
    Sim = const_cast<Simulation*>(Bridge->GetSimulation());
    TestEqual(TEXT("Exact saved tick"), Sim->CurrentTick(), Tick);
    TestEqual(TEXT("Exact saved checksum"), Sim->StateChecksum(), Checksum);
    TestTrue(TEXT("Exact saved position"), Sim->FindEntity(Soldier)->position == Position);
    TestTrue(TEXT("Exact saved payload"), Sim->SaveSnapshot() == Snapshot);
    FString BindingError;
    TestTrue(TEXT("Recorded opening passes the production map binder"),
        EchoesValidateSkirmishSnapshotBindingForTesting(*Sim, Setup, BindingError));
    Simulation Nearby = *Sim;
    Nearby.SetTerrainTile(20, 31, Terrain::Open);
    TestFalse(TEXT("Nearby unrecorded missing blocked tile refuses"),
        EchoesValidateSkirmishSnapshotBindingForTesting(Nearby, Setup, BindingError));
    TestTrue(TEXT("Refusal identifies map mismatch"), BindingError.Contains(TEXT("LOAD_SKIRMISH_MAP_MISMATCH")));
    Simulation ExtraBlocked = *Sim;
    ExtraBlocked.SetTerrainTile(2, 2, Terrain::Blocked);
    TestFalse(TEXT("Unexplained extra blocked tile refuses"),
        EchoesValidateSkirmishSnapshotBindingForTesting(ExtraBlocked, Setup, BindingError));
    FEchoesSkirmishSetup WrongMap = Setup;
    WrongMap.MapPreset = EEchoesSkirmishMapPreset::CrownfallBasin;
    TestFalse(TEXT("Wrong authored preset still refuses"),
        EchoesValidateSkirmishSnapshotBindingForTesting(*Sim, WrongMap, BindingError));
    FEchoesSkirmishSetup WrongFaction = Setup;
    WrongFaction.LocalFaction = Faction::KharuunAssemblies;
    TestFalse(TEXT("Wrong faction still refuses"),
        EchoesValidateSkirmishSnapshotBindingForTesting(*Sim, WrongFaction, BindingError));

    // The old format genuinely lacked this field. Its unbound opening is
    // refused; an exact old replay prefix can prove the omitted delta.
    std::string Error;
    auto OldExpired = Simulation::LoadSnapshot(ToLegacy32(*Sim), &Error);
    if (!TestTrue(TEXT("Older checkpoint decodes"), OldExpired.has_value())) return false;
    TestFalse(TEXT("Unbound older reopened checkpoint refuses map binding"),
        EchoesValidateSkirmishSnapshotBindingForTesting(*OldExpired, Setup, BindingError));
    auto OldPrefix = Sim->ExportReplay();
    auto Baseline = Simulation::LoadSnapshot(OldPrefix.initialSnapshot, &Error);
    if (!TestTrue(TEXT("Replay baseline decodes"), Baseline.has_value())) return false;
    OldPrefix.initialSnapshot = ToLegacy32(*Baseline);
    OldPrefix.version = kFutureWellCaptureGeometryReplayVersion;
    auto OldPlayback = Simulation::BeginReplaySimulation(OldPrefix, &Error);
    if (!TestTrue(TEXT("Older recording opens"), OldPlayback.has_value())) return false;
    if (!TestTrue(TEXT("Controlled prefix has no post-baseline commands"), OldPrefix.commands.empty())) return false;
    OldPlayback->Step(Tick - OldPlayback->CurrentTick());
    OldPrefix.finalChecksum = OldPlayback->ReplayStateChecksum();
    if (!TestTrue(TEXT("Verified old prefix hydrates omitted history"),
        OldExpired->ContinueReplayRecording(OldPrefix, &Error))) return false;
    TestEqual(TEXT("Old recovery preserves exact current state"), OldExpired->StateChecksum(), Checksum);
    TestTrue(TEXT("Replay-proven older delta passes production map binding"),
        EchoesValidateSkirmishSnapshotBindingForTesting(*OldExpired, Setup, BindingError));
    AddInfo(FString::Printf(TEXT("RESHAPE_CHECKPOINT tick=%llu checksum=%llu"), Tick, Checksum));
    Bridge->StopPrototypeScenario();
    World.ForwardErrorMessages(this);
    return !HasAnyErrors() && !World.HasFailed();
}
#endif
