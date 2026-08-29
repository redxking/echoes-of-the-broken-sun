#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesSimCore/Simulation.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesGuardTest,
    "Echoes.Runtime.Gameplay.Guard",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesGuardTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the guard test world."));
        return false;
    }
    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge =
        World != nullptr ? World->GetSubsystem<UEchoesSimulationSubsystem>() : nullptr;
    if (!TestNotNull(TEXT("Guard world owns the simulation subsystem"), Bridge) ||
        !TestTrue(TEXT("Guard scenario starts"),
                  Bridge != nullptr && Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    uint32 Defender = 0;
    uint32 ProtectedWorker = 0;
    uint32 Hostile = 0;
    for (const echoes::sim::Entity& Entity : Bridge->GetSimulation()->Entities())
    {
        if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            Defender == 0 && Entity.type == echoes::sim::EntityType::Soldier)
        {
            Defender = Entity.id;
        }
        else if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
                 ProtectedWorker == 0 &&
                 Entity.type == echoes::sim::EntityType::Worker)
        {
            ProtectedWorker = Entity.id;
        }
        else if (Entity.owner == UEchoesSimulationSubsystem::OpponentPlayerId &&
                 Hostile == 0)
        {
            Hostile = Entity.id;
        }
    }
    if (!TestTrue(TEXT("Guard candidates exist"),
                  Defender != 0 && ProtectedWorker != 0 && Hostile != 0))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    FString Feedback;
    TestFalse(TEXT("Hostile guard target is rejected"),
              Bridge->IssueCommand(
                  echoes::sim::CommandType::Guard, Defender, Hostile,
                  Bridge->SimToWorld(Bridge->FindEntity(Hostile)->position),
                  echoes::sim::FutureWellChoice::Harvest, Feedback));
    TestTrue(TEXT("Hostile rejection is reason-coded"),
             Feedback.StartsWith(TEXT("[GUARD_TARGET_INVALID]")));

    Feedback.Reset();
    TestTrue(TEXT("Owned target accepts guard assignment"),
             Bridge->IssueCommand(
                 echoes::sim::CommandType::Guard, Defender, ProtectedWorker,
                 Bridge->SimToWorld(Bridge->FindEntity(ProtectedWorker)->position),
                 echoes::sim::FutureWellChoice::Harvest, Feedback));
    Bridge->Tick(0.05f);
    Bridge->Tick(0.05f);
    const echoes::sim::Entity* Guarding = Bridge->FindEntity(Defender);
    TestNotNull(TEXT("Guarding defender remains alive"), Guarding);
    if (Guarding != nullptr)
    {
        TestTrue(TEXT("Unreal adapter applies guard"),
                 Guarding->order.type == echoes::sim::OrderType::Guard);
        TestEqual(TEXT("Guard retains the protected entity"),
                  Guarding->order.target, ProtectedWorker);
    }

    Feedback.Reset();
    TestTrue(TEXT("Stop interrupts guard"),
             Bridge->IssueCommand(
                 echoes::sim::CommandType::Stop, Defender, 0,
                 Bridge->SimToWorld(Bridge->FindEntity(Defender)->position),
                 echoes::sim::FutureWellChoice::Harvest, Feedback));
    Bridge->Tick(0.05f);
    Bridge->Tick(0.05f);
    TestTrue(TEXT("Stop clears guard"),
             Bridge->FindEntity(Defender)->order.type ==
                 echoes::sim::OrderType::None);

    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors() && !WorldWrapper.HasFailed();
}

#endif
