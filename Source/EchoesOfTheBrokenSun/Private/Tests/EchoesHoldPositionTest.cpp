#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "EchoesSimCore/Simulation.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesHoldPositionTest,
    "Echoes.Runtime.Gameplay.HoldPosition",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesHoldPositionTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    FEchoesScopedTestSaveEnvironment TestSaveEnvironment(*this);
    if (!TestSaveEnvironment.IsReady())
    {
        return false;
    }

    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the hold-position test world."));
        return false;
    }

    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge =
        World != nullptr
            ? World->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (!TestNotNull(TEXT("Hold-position world owns the simulation subsystem"), Bridge) ||
        !TestTrue(TEXT("Hold-position scenario starts"),
                  Bridge != nullptr && Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    uint32 LocalSoldier = 0;
    uint32 LocalCore = 0;
    for (const echoes::sim::Entity& Entity : Bridge->GetSimulation()->Entities())
    {
        if (Entity.owner != UEchoesSimulationSubsystem::LocalPlayerId)
        {
            continue;
        }
        if (LocalSoldier == 0 &&
            Entity.type == echoes::sim::EntityType::Soldier)
        {
            LocalSoldier = Entity.id;
        }
        else if (LocalCore == 0 &&
                 Entity.type == echoes::sim::EntityType::CommandCore)
        {
            LocalCore = Entity.id;
        }
    }
    if (!TestTrue(TEXT("A local hold-position defender exists"), LocalSoldier != 0) ||
        !TestTrue(TEXT("A noncombat local structure exists"), LocalCore != 0))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const echoes::sim::Vec2 Anchor =
        Bridge->FindEntity(LocalSoldier)->position;
    FString Feedback;
    TestFalse(
        TEXT("A noncombat structure rejects hold position"),
        Bridge->IssueCommand(
            echoes::sim::CommandType::Hold,
            LocalCore,
            0,
            Bridge->SimToWorld(Bridge->FindEntity(LocalCore)->position),
            echoes::sim::FutureWellChoice::Harvest,
            Feedback));
    TestTrue(TEXT("Structure rejection is reason-coded"),
             Feedback.StartsWith(TEXT("[HOLD_REQUIRES_DEFENDER]")));

    Feedback.Reset();
    TestTrue(
        TEXT("An attack-capable local unit accepts hold position"),
        Bridge->IssueCommand(
            echoes::sim::CommandType::Hold,
            LocalSoldier,
            0,
            Bridge->SimToWorld(Anchor),
            echoes::sim::FutureWellChoice::Harvest,
            Feedback));
    Bridge->Tick(0.05f);
    Bridge->Tick(0.05f);
    const echoes::sim::Entity* Held = Bridge->FindEntity(LocalSoldier);
    TestNotNull(TEXT("Held defender remains alive"), Held);
    if (Held != nullptr)
    {
        TestTrue(TEXT("Unreal adapter applies the hold order"),
                 Held->order.type == echoes::sim::OrderType::Hold);
        TestTrue(TEXT("Hold records the authoritative anchor"),
                 Held->order.destination == Anchor);
        TestTrue(TEXT("Hold does not displace the defender"),
                 Held->position == Anchor);
    }

    Feedback.Reset();
    TestTrue(
        TEXT("Stop interrupts hold position"),
        Bridge->IssueCommand(
            echoes::sim::CommandType::Stop,
            LocalSoldier,
            0,
            Bridge->SimToWorld(Anchor),
            echoes::sim::FutureWellChoice::Harvest,
            Feedback));
    Bridge->Tick(0.05f);
    Bridge->Tick(0.05f);
    const echoes::sim::Entity* Stopped = Bridge->FindEntity(LocalSoldier);
    TestNotNull(TEXT("Stopped defender remains alive"), Stopped);
    if (Stopped != nullptr)
    {
        TestTrue(TEXT("Stop clears the hold order"),
                 Stopped->order.type == echoes::sim::OrderType::None);
        TestTrue(TEXT("Stop preserves the hold anchor"),
                 Stopped->position == Anchor);
    }

    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors() && !WorldWrapper.HasFailed();
}

#endif
