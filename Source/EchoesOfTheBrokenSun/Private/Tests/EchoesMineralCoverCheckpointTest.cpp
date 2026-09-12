#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "EchoesTestSaveEnvironment.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesSkirmishSetup.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesMineralCoverCheckpointTest,
    "Echoes.Runtime.Persistence.MineralCoverCheckpoint",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesMineralCoverCheckpointTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    using namespace echoes::sim;
    FEchoesScopedTestSaveEnvironment SaveEnvironment(*this);
    if (!SaveEnvironment.IsReady()) return false;
    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game)) return false;
    auto* Bridge = WorldWrapper.GetTestWorld()->GetSubsystem<UEchoesSimulationSubsystem>();
    if (!TestNotNull(TEXT("Checkpoint subsystem exists"), Bridge)) return false;
    FString Feedback;
    FEchoesSkirmishSetup Setup;
    Setup.LocalFaction = Faction::KharuunAssemblies;
    Setup.OpponentFaction = Faction::MeridianCompact;
    Setup.ResourceLevel = EEchoesSkirmishResourceLevel::Abundant;
    if (!TestTrue(TEXT("Skirmish starts"), Bridge->StartPrototypeScenario()) ||
        !TestTrue(TEXT("Kharuun setup applies"), Bridge->ApplySkirmishSetup(Setup, Feedback)))
        return false;

    EntityId Cairnback = 0;
    Vec2 Site{};
    bool FoundSite = false;
    for (const Entity& EntityState : Bridge->GetSimulation()->Entities())
    {
        if (EntityState.owner != UEchoesSimulationSubsystem::LocalPlayerId ||
            EntityState.type != EntityType::HeavyUnit) continue;
        Cairnback = EntityState.id;
        for (int32 Y = 0; Y < FEchoesSkirmishSetupModel::MapHeightTiles && !FoundSite; ++Y)
            for (int32 X = 0; X < FEchoesSkirmishSetupModel::MapWidthTiles && !FoundSite; ++X)
                if (Bridge->GetSimulation()->ValidateMineralCover(
                        UEchoesSimulationSubsystem::LocalPlayerId, Cairnback,
                        Vec2::FromTiles(X, Y)) == MineralCoverResult::Valid)
                {
                    Site = Vec2::FromTiles(X, Y);
                    FoundSite = true;
                }
        break;
    }
    if (!TestTrue(TEXT("Authored Cairnback has a valid cover site"), FoundSite)) return false;
    if (!TestTrue(TEXT("Cover command enters the gameplay bridge"), Bridge->IssueCommand(
            CommandType::RaiseMineralCover, Cairnback, 0, Bridge->SimToWorld(Site),
            FutureWellChoice::Dormant, Feedback))) return false;
    Bridge->SetScenarioPaused(false);
    EntityId Cover = 0;
    for (int32 Step = 0; Step < 20 && Cover == 0; ++Step)
    {
        Bridge->Tick(0.05f);
        for (const Entity& EntityState : Bridge->GetSimulation()->Entities())
            if (EntityState.temporaryMineralCover && EntityState.position == Site &&
                EntityState.hitPoints > 0) Cover = EntityState.id;
    }
    if (!TestTrue(TEXT("Cover is live before checkpoint capture"), Cover != 0)) return false;
    const uint64 SavedTick = Bridge->GetSimulation()->CurrentTick();
    const uint64 SavedChecksum = Bridge->GetSimulation()->StateChecksum();
    if (!TestTrue(TEXT("Live-cover checkpoint saves"), Bridge->QuickSaveScenario(Feedback))) return false;
    Bridge->Tick(0.15f);
    TestTrue(TEXT("Simulation advances beyond the checkpoint"),
        Bridge->GetSimulation()->CurrentTick() > SavedTick);
    if (!TestTrue(*FString::Printf(TEXT("Live-cover checkpoint loads: %s"), *Feedback),
            Bridge->QuickLoadScenario(Feedback))) return false;
    TestEqual(TEXT("Saved tick restored exactly"), Bridge->GetSimulation()->CurrentTick(), SavedTick);
    TestEqual(TEXT("Complete authoritative state restored exactly"),
        Bridge->GetSimulation()->StateChecksum(), SavedChecksum);
    const Entity* RestoredCover = Bridge->FindEntity(Cover);
    TestTrue(TEXT("Live cover survives load at the exact tile"), RestoredCover &&
        RestoredCover->temporaryMineralCover && RestoredCover->position == Site &&
        RestoredCover->hitPoints > 0);
    TestTrue(TEXT("Cover tile remains blocked"), Bridge->GetSimulation()->TerrainAt(
        Site.x.FloorToInt(), Site.y.FloorToInt()) == Terrain::Blocked);
    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors() && !WorldWrapper.HasFailed();
}
#endif
