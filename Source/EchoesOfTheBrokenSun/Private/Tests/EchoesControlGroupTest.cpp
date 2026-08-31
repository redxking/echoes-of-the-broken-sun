#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "EchoesPlayerController.h"
#include "EchoesSimCore/Simulation.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesControlGroupTest,
    "Echoes.Runtime.Controls.ControlGroups",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesControlGroupTest::RunTest(const FString& Parameters)
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
        AddError(TEXT("Could not create the control-group test world."));
        return false;
    }

    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge =
        World != nullptr
            ? World->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (!TestNotNull(TEXT("Control-group world owns the simulation subsystem"), Bridge) ||
        !TestTrue(TEXT("Control-group scenario starts"),
                  Bridge != nullptr && Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    TArray<uint32> LocalIds;
    uint32 HostileId = 0;
    for (const echoes::sim::Entity& Entity : Bridge->GetSimulation()->Entities())
    {
        if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            (Entity.type == echoes::sim::EntityType::Worker ||
             Entity.type == echoes::sim::EntityType::Soldier) &&
            LocalIds.Num() < 2)
        {
            LocalIds.Add(Entity.id);
        }
        else if (Entity.owner == UEchoesSimulationSubsystem::OpponentPlayerId &&
                 HostileId == 0)
        {
            HostileId = Entity.id;
        }
    }
    if (!TestEqual(TEXT("Two local control-group candidates exist"), LocalIds.Num(), 2) ||
        !TestTrue(TEXT("A hostile validation candidate exists"), HostileId != 0))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    AEchoesPlayerController* Controller =
        World->SpawnActor<AEchoesPlayerController>();
    if (!TestNotNull(TEXT("Control-group controller spawns"), Controller))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    TArray<uint32> WithDuplicate = LocalIds;
    WithDuplicate.Add(LocalIds[0]);
    FString Feedback;
    TestTrue(TEXT("A valid group assignment succeeds"),
             Controller->SetControlGroup(0, WithDuplicate, Feedback));
    const TArray<uint32> GroupOne = Controller->GetValidControlGroup(0);
    TestEqual(TEXT("A control group removes duplicate IDs"), GroupOne.Num(), 2);
    TestTrue(TEXT("Control group retains the first local entity"),
             GroupOne.Contains(LocalIds[0]));
    TestTrue(TEXT("Control group retains the second local entity"),
             GroupOne.Contains(LocalIds[1]));

    TestFalse(TEXT("An out-of-range group is rejected"),
              Controller->SetControlGroup(10, LocalIds, Feedback));
    TestTrue(TEXT("Invalid-group feedback is reason-coded"),
             Feedback.StartsWith(TEXT("[GROUP_INDEX_INVALID]")));

    TestFalse(TEXT("A hostile-only group is rejected"),
              Controller->SetControlGroup(1, {HostileId}, Feedback));
    TestTrue(TEXT("Hostile-only feedback is reason-coded"),
             Feedback.StartsWith(TEXT("[GROUP_NO_VALID_ENTITIES]")));

    TestTrue(TEXT("An empty assignment clears its group"),
             Controller->SetControlGroup(0, {}, Feedback));
    TestTrue(TEXT("A cleared group resolves empty"),
             Controller->GetValidControlGroup(0).IsEmpty());

    Controller->Destroy();
    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors() && !WorldWrapper.HasFailed();
}

#endif
