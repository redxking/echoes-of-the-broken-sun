#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/StaticMeshComponent.h"
#include "EchoesEntityView.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesFutureWellCollapseTest,
    "Echoes.Runtime.Presentation.FutureWellHarvestCollapse",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesFutureWellCollapseTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the Future Well collapse test world."));
        return false;
    }
    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge = World != nullptr
        ? World->GetSubsystem<UEchoesSimulationSubsystem>()
        : nullptr;
    if (Bridge == nullptr || !Bridge->StartPrototypeScenario())
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not start the Future Well collapse test scenario."));
        return false;
    }

    AEchoesEntityView* View = World->SpawnActor<AEchoesEntityView>();
    if (!TestNotNull(TEXT("Future Well view spawns"), View))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    echoes::sim::Entity State{};
    State.id = 991001;
    State.type = echoes::sim::EntityType::FutureWell;
    State.position = echoes::sim::Vec2::FromTiles(32, 32);
    State.hitPoints = State.maxHitPoints = 1;
    State.completed = true;
    State.wellChoice = echoes::sim::FutureWellChoice::Harvest;
    View->ActivateForEntity(State, true);

    UStaticMeshComponent* Orbit = nullptr;
    UStaticMeshComponent* Rim = nullptr;
    UStaticMeshComponent* Cavity = nullptr;
    TArray<UStaticMeshComponent*> Components;
    View->GetComponents(Components);
    for (UStaticMeshComponent* Component : Components)
    {
        if (Component->GetFName() == TEXT("FutureWellOrbitOuter")) Orbit = Component;
        if (Component->GetFName() == TEXT("FutureWellCollapseRim")) Rim = Component;
        if (Component->GetFName() == TEXT("FutureWellCollapseCavity")) Cavity = Component;
    }
    if (!TestNotNull(TEXT("Harvest remnant rim exists"), Rim) ||
        !TestNotNull(TEXT("Harvest remnant cavity exists"), Cavity) ||
        !TestNotNull(TEXT("Intact orbit exists to be hidden"), Orbit))
    {
        View->Destroy();
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    TestTrue(TEXT("Harvest is a terminal collapsed visual state"),
             View->IsFutureWellTerminallyCollapsed() &&
                 View->IsFutureWellCollapsedRemnantVisible());
    TestFalse(TEXT("Harvest does not leave the intact Well presentation visible"),
              View->IsFutureWellPresentationVisible());
    TestFalse(TEXT("Harvest remnant cannot be picked through intact Well geometry"),
              View->IsFutureWellPresentationEntityPickable());
    TestFalse(TEXT("Harvest remnant disables the footprint pick proxy"),
              View->IsEntityPickProxyEnabled());
    TestFalse(TEXT("Harvest remnant has no health or owner marker"),
              View->IsHealthBarVisible() || View->IsOwnerMarkerVisible());
    View->SetSelected(true);
    TestFalse(TEXT("Harvest remnant rejects selection"), View->IsSelected());
    const FQuat TerminalOrbit = Orbit->GetRelativeRotation().Quaternion();
    View->Tick(0.5f);
    TestTrue(TEXT("Harvest remnant leaves no orbit motion running"),
             TerminalOrbit.Equals(Orbit->GetRelativeRotation().Quaternion()));
    TestTrue(TEXT("Harvest collapse geometry is visible but non-colliding"),
             Rim->IsVisible() && Cavity->IsVisible() &&
                 Rim->GetCollisionEnabled() == ECollisionEnabled::NoCollision &&
                 Cavity->GetCollisionEnabled() == ECollisionEnabled::NoCollision);

    // A pending Harvest remains an active, publicly telegraphed Well. The
    // permanent remnant appears only after the authoritative timer completes.
    State.wellChoice = echoes::sim::FutureWellChoice::Dormant;
    State.wellPendingChoice = echoes::sim::FutureWellChoice::Harvest;
    State.wellProtocolTicks = 180;
    View->ApplyAuthoritativeState(State, true);
    TestFalse(TEXT("Pending Harvest has not collapsed the Well"),
              View->IsFutureWellTerminallyCollapsed());
    TestTrue(TEXT("Pending Harvest uses the active amber protocol visual"),
             View->GetFutureWellVisualChoice() == echoes::sim::FutureWellChoice::Harvest &&
                 View->IsFutureWellPresentationVisible());
    TestTrue(TEXT("Pending Harvest remains pickable for interruption"),
             View->IsFutureWellPresentationEntityPickable() &&
                 View->IsEntityPickProxyEnabled());

    // Pooling must remove the terminal state. A retained actor can later be
    // bound to an ordinary Well without inheriting hidden collision or debris.
    View->PrepareForPool();
    State.wellPendingChoice = echoes::sim::FutureWellChoice::Dormant;
    State.wellProtocolTicks = 0;
    View->ActivateForEntity(State, true);
    TestFalse(TEXT("Pooled recovery clears the terminal-collapse state"),
              View->IsFutureWellTerminallyCollapsed() ||
                  View->IsFutureWellCollapsedRemnantVisible());
    TestTrue(TEXT("Pooled recovery restores an intact, pickable Well"),
             View->IsFutureWellPresentationVisible() &&
                 View->IsFutureWellPresentationEntityPickable() &&
                 View->IsEntityPickProxyEnabled());

    // Feed the view states emitted by the real core. The prior hand-built
    // pending-Harvest fixture did not match schema 27's Harvest countdown.
    for (const auto Choice : {echoes::sim::FutureWellChoice::Harvest,
                              echoes::sim::FutureWellChoice::Reshape})
    {
        echoes::sim::Simulation Sim({20, 20, 20, 0x57454c4c});
        Sim.AddPlayer(0, echoes::sim::Faction::MeridianCompact, {0, 120});
        const auto Worker = Sim.SpawnEntity(0, echoes::sim::Faction::MeridianCompact,
            echoes::sim::EntityType::Worker, echoes::sim::Vec2::FromTiles(5, 6));
        const auto Well = Sim.SpawnFutureWell(echoes::sim::Vec2::FromTiles(6, 6));
        echoes::sim::Command Command{};
        Command.player = 0; Command.sequence = 1;
        Command.type = echoes::sim::CommandType::FutureWell;
        Command.actor = Worker; Command.target = Well; Command.wellChoice = Choice;
        TestTrue(TEXT("Real Well command queues"), Sim.QueueCommand(Command));
        Sim.Step(300);
        View->ApplyAuthoritativeState(*Sim.FindEntity(Well), true);
        TestEqual(TEXT("Committed warning begins at 180 ticks"),
            Sim.FindEntity(Well)->wellProtocolTicks, echoes::sim::Tick{180});
        TestFalse(TEXT("Warning never displays a collapsed Well"),
            View->IsFutureWellTerminallyCollapsed());
        TestTrue(TEXT("Warning shows its actual protocol identity"),
            View->GetFutureWellVisualChoice() == Choice &&
            View->IsFutureWellPresentationVisible());
        TestEqual(TEXT("Reshape field marks only its warning phase"),
            View->IsReshapeTelegraphActive(), Choice == echoes::sim::FutureWellChoice::Reshape);
        Sim.Step(179);
        View->ApplyAuthoritativeState(*Sim.FindEntity(Well), true);
        TestFalse(TEXT("Final warning tick still has an intact Well"),
            View->IsFutureWellTerminallyCollapsed());
        Sim.Step();
        View->ApplyAuthoritativeState(*Sim.FindEntity(Well), true);
        TestEqual(TEXT("Only completed Harvest collapses"),
            View->IsFutureWellTerminallyCollapsed(), Choice == echoes::sim::FutureWellChoice::Harvest);
        TestFalse(TEXT("Protocol completion removes the warning field"),
            View->IsReshapeTelegraphActive());
    }

    View->Destroy();
    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return !WorldWrapper.HasFailed();
}

#endif
