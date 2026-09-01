#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "EchoesSimCore/Simulation.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesFailureReasonTest,
    "Echoes.Runtime.Campaign.FailureReasons",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesFailureReasonTest::RunTest(const FString& Parameters)
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
        AddError(TEXT("Could not create the failure-reason world."));
        return false;
    }

    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge =
        World != nullptr ? World->GetSubsystem<UEchoesSimulationSubsystem>()
                         : nullptr;
    FString Feedback;
    if (!TestNotNull(TEXT("World owns the simulation subsystem"), Bridge) ||
        !TestTrue(TEXT("Skirmish (no campaign contract) derives generic"),
                  Bridge != nullptr &&
                      Bridge->GetMissionFailureReasonCode() ==
                          TEXT("generic")) ||
        !TestTrue(TEXT("The prologue operation selects with an empty ledger"),
                  Bridge->SelectOperationMode(
                      EEchoesOperationMode::CampaignPrologue, Feedback)) ||
        !TestTrue(TEXT("The prologue scenario starts"),
                  Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    echoes::sim::Simulation* Simulation =
        const_cast<echoes::sim::Simulation*>(Bridge->GetSimulation());
    if (!TestNotNull(TEXT("The test owns a mutable simulation fixture"),
                     Simulation))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    // Locate the authoritative cast of the prologue's failure causes.
    echoes::sim::EntityId LocalCoreId = 0;
    echoes::sim::EntityId FutureWellId = 0;
    for (const echoes::sim::Entity& Entity : Simulation->Entities())
    {
        if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            Entity.type == echoes::sim::EntityType::CommandCore)
        {
            LocalCoreId = Entity.id;
        }
        if (Entity.type == echoes::sim::EntityType::FutureWell)
        {
            FutureWellId = Entity.id;
        }
    }
    const echoes::sim::EntityId CarrierId = Bridge->GetArchiveCarrierId();
    if (!TestTrue(TEXT("Core, carrier, and Well are all bound"),
                  LocalCoreId != 0 && CarrierId != 0 && FutureWellId != 0))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    TestEqual(TEXT("An intact ongoing prologue derives generic"),
              Bridge->GetMissionFailureReasonCode(), FString(TEXT("generic")));

    // Each cause is asserted in isolation against live authoritative state,
    // then restored, so causes never contaminate one another.
    echoes::sim::Entity* Core = Simulation->MutableEntityForTesting(LocalCoreId);
    echoes::sim::Entity* Carrier = Simulation->MutableEntityForTesting(CarrierId);
    echoes::sim::Entity* Well = Simulation->MutableEntityForTesting(FutureWellId);
    if (!TestTrue(TEXT("Mutable cast entities resolve"),
                  Core != nullptr && Carrier != nullptr && Well != nullptr))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const int32 CoreHitPoints = Core->hitPoints;
    Core->hitPoints = 0;
    TestEqual(TEXT("A destroyed local core derives local_core_lost"),
              Bridge->GetMissionFailureReasonCode(),
              FString(TEXT("local_core_lost")));

    const int32 CarrierHitPoints = Carrier->hitPoints;
    Carrier->hitPoints = 0;
    TestEqual(
        TEXT("Core loss outranks carrier loss in the authored priority"),
        Bridge->GetMissionFailureReasonCode(),
        FString(TEXT("local_core_lost")));

    Core->hitPoints = CoreHitPoints;
    TestEqual(TEXT("A destroyed archive carrier derives archive_carrier_lost"),
              Bridge->GetMissionFailureReasonCode(),
              FString(TEXT("archive_carrier_lost")));
    Carrier->hitPoints = CarrierHitPoints;

    const echoes::sim::PlayerId WellOwner = Well->owner;
    const echoes::sim::FutureWellChoice WellChoice = Well->wellChoice;
    Well->owner = UEchoesSimulationSubsystem::OpponentPlayerId;
    Well->wellChoice = echoes::sim::FutureWellChoice::Harvest;
    TestEqual(TEXT("An opposing Well protocol derives future_well_lost"),
              Bridge->GetMissionFailureReasonCode(),
              FString(TEXT("future_well_lost")));
    Well->owner = WellOwner;
    Well->wellChoice = WellChoice;

    TestEqual(TEXT("Restored state derives generic again"),
              Bridge->GetMissionFailureReasonCode(), FString(TEXT("generic")));

    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
