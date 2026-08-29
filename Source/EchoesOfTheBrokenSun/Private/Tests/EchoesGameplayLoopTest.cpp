#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesSimCore/Simulation.h"
#include "EchoesPlayerController.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesGameplayLoopTest,
    "Echoes.Runtime.Gameplay.ProductionPauseRestart",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesGameplayLoopTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the temporary gameplay world."));
        return false;
    }

    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge =
        World != nullptr
            ? World->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (!TestNotNull(TEXT("Gameplay world owns the simulation subsystem"), Bridge) ||
        !TestTrue(TEXT("Gameplay scenario starts"),
                  Bridge != nullptr && Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const echoes::sim::Simulation* Initial = Bridge->GetSimulation();
    if (!TestNotNull(TEXT("Gameplay simulation is available"), Initial))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    echoes::sim::EntityId LocalCore = 0;
    echoes::sim::EntityId LocalBarracks = 0;
    echoes::sim::EntityId FutureWell = 0;
    echoes::sim::EntityId HostileCore = 0;
    int32 InitialWorkers = 0;
    int32 InitialSoldiers = 0;
    for (const echoes::sim::Entity& Entity : Initial->Entities())
    {
        if (Entity.type == echoes::sim::EntityType::FutureWell)
        {
            FutureWell = Entity.id;
        }
        else if (Entity.owner != echoes::sim::kNeutralPlayer &&
                 Entity.owner != UEchoesSimulationSubsystem::LocalPlayerId &&
                 Entity.type == echoes::sim::EntityType::CommandCore)
        {
            HostileCore = Entity.id;
        }
        if (Entity.owner != UEchoesSimulationSubsystem::LocalPlayerId)
        {
            continue;
        }
        if (Entity.type == echoes::sim::EntityType::CommandCore)
        {
            LocalCore = Entity.id;
        }
        else if (Entity.type == echoes::sim::EntityType::Barracks)
        {
            LocalBarracks = Entity.id;
        }
        else if (Entity.type == echoes::sim::EntityType::Worker)
        {
            ++InitialWorkers;
        }
        else if (Entity.type == echoes::sim::EntityType::Soldier)
        {
            ++InitialSoldiers;
        }
    }
    TestTrue(TEXT("Local Command Core is present"), LocalCore != 0);
    TestTrue(TEXT("Local Barracks is present"), LocalBarracks != 0);
    TestTrue(TEXT("Future Well is present"), FutureWell != 0);
    TestTrue(TEXT("Hostile Command Core is present"), HostileCore != 0);
    TestEqual(TEXT("Initial local logistics are committed"),
              Initial->PopulationUsed(UEchoesSimulationSubsystem::LocalPlayerId),
              9);
    TestEqual(TEXT("Initial local logistics capacity is available"),
              Initial->PopulationCapacity(UEchoesSimulationSubsystem::LocalPlayerId),
              12);

    const FEchoesObjectiveSnapshot Objectives =
        Bridge->GetLocalObjectiveSnapshot();
    TestTrue(TEXT("Objective snapshot identifies a ready scenario"),
             Objectives.bScenarioReady);
    TestTrue(TEXT("Objective snapshot reports the owned Command Core"),
             Objectives.bLocalCoreIntact);
    TestTrue(TEXT("Owned Command Core integrity is available to its player"),
             Objectives.LocalCoreHitPoints > 0 &&
                 Objectives.LocalCoreHitPoints == Objectives.LocalCoreMaxHitPoints);
    TestEqual(
        TEXT("Future Well objective follows current local visibility"),
        Objectives.bFutureWellVisible,
        Initial->IsEntityVisibleTo(
            UEchoesSimulationSubsystem::LocalPlayerId,
            FutureWell));
    TestEqual(
        TEXT("Hostile Command Core objective does not bypass visibility"),
        Objectives.bHostileCoreVisible,
        Initial->IsEntityVisibleTo(
            UEchoesSimulationSubsystem::LocalPlayerId,
            HostileCore));
    TestTrue(TEXT("Objective snapshot starts with an ongoing result"),
             Objectives.Outcome == echoes::sim::MatchOutcome::Ongoing);

    const echoes::sim::Tick PausedTick = Initial->CurrentTick();
    AEchoesPlayerController* BriefingController =
        World->SpawnActor<AEchoesPlayerController>();
    if (TestNotNull(TEXT("Mission briefing controller can be created"), BriefingController))
    {
        BriefingController->PresentTitleScreen();
        TestTrue(TEXT("Title screen is visible at interactive launch"),
                 BriefingController->IsTitleScreenVisible());
        TestTrue(TEXT("Title screen pauses the deterministic scenario"),
                 Bridge->IsScenarioPaused());
        Bridge->Tick(0.5f);
        TestEqual(TEXT("Title screen prevents deterministic simulation advancement"),
                  Bridge->GetSimulation()->CurrentTick(),
                  PausedTick);
        BriefingController->ConfirmPrimaryAction();
        TestFalse(TEXT("Title confirmation dismisses the title screen"),
                  BriefingController->IsTitleScreenVisible());
        TestTrue(TEXT("Mission briefing is visible before deployment"),
                 BriefingController->IsMissionBriefingVisible());
        TestTrue(TEXT("Mission briefing pauses the deterministic scenario"),
                 Bridge->IsScenarioPaused());
        Bridge->Tick(0.5f);
        TestEqual(TEXT("Briefing prevents deterministic simulation advancement"),
                  Bridge->GetSimulation()->CurrentTick(),
                  PausedTick);
        BriefingController->ConfirmPrimaryAction();
        TestFalse(TEXT("Deployment dismisses the mission briefing"),
                  BriefingController->IsMissionBriefingVisible());
        TestFalse(TEXT("Deployment resumes the deterministic scenario"),
                  Bridge->IsScenarioPaused());
        const echoes::sim::Tick MenuPausedTick =
            Bridge->GetSimulation()->CurrentTick();
        BriefingController->TogglePauseMenu();
        TestTrue(TEXT("Field menu becomes visible"),
                 BriefingController->IsPauseMenuVisible());
        TestTrue(TEXT("Field menu pauses the deterministic scenario"),
                 Bridge->IsScenarioPaused());
        TestTrue(TEXT("Field menu is a modal input boundary"),
                 BriefingController->IsModalOverlayVisible());
        Bridge->Tick(0.5f);
        TestEqual(TEXT("Field menu prevents simulation advancement"),
                  Bridge->GetSimulation()->CurrentTick(),
                  MenuPausedTick);
        BriefingController->ConfirmPrimaryAction();
        TestFalse(TEXT("Enter dismisses the field menu"),
                  BriefingController->IsPauseMenuVisible());
        TestFalse(TEXT("Enter resumes the field operation"),
                  Bridge->IsScenarioPaused());
        BriefingController->Destroy();
    }

    Bridge->SetScenarioPaused(true);
    Bridge->Tick(0.5f);
    TestEqual(TEXT("Pause prevents deterministic simulation advancement"),
              Bridge->GetSimulation()->CurrentTick(),
              PausedTick);
    Bridge->SetScenarioPaused(false);

    FString Feedback;
    TestTrue(
        TEXT("Command Core accepts worker production"),
        Bridge->IssueProductionCommand(
            LocalCore,
            echoes::sim::EntityType::Worker,
            Feedback));
    if (!Feedback.IsEmpty())
    {
        AddInfo(FString::Printf(TEXT("Worker production feedback: %s"), *Feedback));
    }
    Feedback.Reset();
    TestTrue(
        TEXT("Barracks accepts soldier production"),
        Bridge->IssueProductionCommand(
            LocalBarracks,
            echoes::sim::EntityType::Soldier,
            Feedback));
    if (!Feedback.IsEmpty())
    {
        AddInfo(FString::Printf(TEXT("Soldier production feedback: %s"), *Feedback));
    }

    for (int32 TickIndex = 0; TickIndex < 112; ++TickIndex)
    {
        Bridge->Tick(0.05f);
    }
    const echoes::sim::Simulation* Produced = Bridge->GetSimulation();
    int32 ProducedWorkers = 0;
    int32 ProducedSoldiers = 0;
    for (const echoes::sim::Entity& Entity : Produced->Entities())
    {
        if (Entity.owner != UEchoesSimulationSubsystem::LocalPlayerId)
        {
            continue;
        }
        ProducedWorkers += Entity.type == echoes::sim::EntityType::Worker ? 1 : 0;
        ProducedSoldiers += Entity.type == echoes::sim::EntityType::Soldier ? 1 : 0;
    }
    TestEqual(TEXT("Worker production creates exactly one local worker"),
              ProducedWorkers,
              InitialWorkers + 1);
    TestEqual(TEXT("Soldier production creates exactly one local soldier"),
              ProducedSoldiers,
              InitialSoldiers + 1);
    TestEqual(TEXT("Production fills the initial logistics capacity"),
              Produced->PopulationUsed(UEchoesSimulationSubsystem::LocalPlayerId),
              12);

    TestTrue(TEXT("Restart rebuilds the deterministic scenario"),
             Bridge->RestartPrototypeScenario());
    const echoes::sim::Simulation* Restarted = Bridge->GetSimulation();
    TestNotNull(TEXT("Restarted simulation is available"), Restarted);
    if (Restarted != nullptr)
    {
        TestEqual(TEXT("Restart returns to tick zero"), Restarted->CurrentTick(),
                  static_cast<echoes::sim::Tick>(0));
        TestEqual(TEXT("Restart restores initial entity count"),
                  static_cast<int32>(Restarted->Entities().size()),
                  25);
        TestTrue(TEXT("Restart restores an ongoing match"),
                 Restarted->Outcome() == echoes::sim::MatchOutcome::Ongoing);
        const echoes::sim::PlayerState* Player = Restarted->FindPlayer(
            UEchoesSimulationSubsystem::LocalPlayerId);
        TestNotNull(TEXT("Restart restores the local player"), Player);
        if (Player != nullptr)
        {
            TestEqual(TEXT("Restart restores local Matter"),
                      Player->resources.material,
                      500);
            TestEqual(TEXT("Restart restores local Dawnshards"),
                      Player->resources.dawnshards,
                      30);
        }
    }

    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors() && !WorldWrapper.HasFailed();
}

#endif
