#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesEntityView.h"
#include "EchoesSimCore/Simulation.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/World.h"
#include "Misc/ConfigCacheIni.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesRelaySupplyTest,
    "Echoes.Runtime.Gameplay.RelaySupply",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesRelaySupplyTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the Relay supply test world."));
        return false;
    }

    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge =
        World != nullptr
            ? World->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (!TestNotNull(TEXT("Relay world owns the simulation subsystem"), Bridge) ||
        !TestTrue(TEXT("Relay scenario starts"),
                  Bridge != nullptr && Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    uint32 LocalRelay = 0;
    uint32 LocalLancer = 0;
    for (const echoes::sim::Entity& Entity : Bridge->GetSimulation()->Entities())
    {
        if (Entity.owner != UEchoesSimulationSubsystem::LocalPlayerId)
        {
            continue;
        }
        if (LocalRelay == 0 &&
            Entity.faction == echoes::sim::Faction::MeridianCompact &&
            Entity.type == echoes::sim::EntityType::ScoutUnit)
        {
            LocalRelay = Entity.id;
        }
        else if (LocalLancer == 0 &&
                 Entity.type == echoes::sim::EntityType::Soldier)
        {
            LocalLancer = Entity.id;
        }
    }
    if (!TestTrue(TEXT("The scenario contains a local Relay Skiff"), LocalRelay != 0) ||
        !TestTrue(TEXT("The scenario contains a non-Relay unit"), LocalLancer != 0))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const int32 CapacityBefore = Bridge->GetSimulation()->PopulationCapacity(
        UEchoesSimulationSubsystem::LocalPlayerId);
    FString Feedback;
    TestFalse(
        TEXT("A Lancer cannot activate Relay supply"),
        Bridge->IssueCommand(
            echoes::sim::CommandType::ActivateRelaySupply,
            LocalLancer,
            0,
            Bridge->SimToWorld(Bridge->FindEntity(LocalLancer)->position),
            echoes::sim::FutureWellChoice::Harvest,
            Feedback));
    TestTrue(TEXT("Invalid Relay activation is reason-coded"),
             Feedback.StartsWith(TEXT("[RELAY_REQUIRED]")));

    Feedback.Reset();
    TestTrue(
        TEXT("A connected Relay accepts supply activation"),
        Bridge->IssueCommand(
            echoes::sim::CommandType::ActivateRelaySupply,
            LocalRelay,
            0,
            Bridge->SimToWorld(Bridge->FindEntity(LocalRelay)->position),
            echoes::sim::FutureWellChoice::Harvest,
            Feedback));
    Bridge->Tick(0.05f);
    Bridge->Tick(0.05f);

    const echoes::sim::Entity* ActiveRelay = Bridge->FindEntity(LocalRelay);
    TestNotNull(TEXT("Active Relay remains alive"), ActiveRelay);
    if (ActiveRelay != nullptr)
    {
        TestTrue(TEXT("Unreal adapter applies public Relay state"),
                 ActiveRelay->relaySupplyActive);
        TestTrue(TEXT("Relay activation increases logistics capacity"),
                 Bridge->GetSimulation()->PopulationCapacity(
                     UEchoesSimulationSubsystem::LocalPlayerId) ==
                     CapacityBefore +
                         Bridge->GetSimulation()->Config().rules.relaySupply.capacityBonus);
    }
    AEchoesEntityView* RelayView = Bridge->FindEntityView(LocalRelay);
    if (TestNotNull(TEXT("Relay presentation proxy remains available"), RelayView))
    {
        TestTrue(TEXT("Active Relay exposes its public supply field"),
                 RelayView->IsRelaySupplyFieldVisible());
    }

    Feedback.Reset();
    TestFalse(
        TEXT("An already-active Relay rejects duplicate activation"),
        Bridge->IssueCommand(
            echoes::sim::CommandType::ActivateRelaySupply,
            LocalRelay,
            0,
            Bridge->SimToWorld(Bridge->FindEntity(LocalRelay)->position),
            echoes::sim::FutureWellChoice::Harvest,
            Feedback));
    TestTrue(TEXT("Duplicate activation has a stable reason"),
             Feedback.StartsWith(TEXT("[RELAY_ALREADY_ACTIVE]")));

    TArray<FString> InputMappings;
    GConfig->GetArray(
        TEXT("/Script/Engine.InputSettings"),
        TEXT("ActionMappings"),
        InputMappings,
        GInputIni);
    TestTrue(
        TEXT("Equals Relay-supply mapping is present"),
        InputMappings.ContainsByPredicate(
            [](const FString& Mapping)
            {
                return Mapping.Contains(TEXT("ActionName=\"ActivateRelaySupply\"")) &&
                       Mapping.Contains(TEXT("Key=Equals"));
            }));

    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors() && !WorldWrapper.HasFailed();
}

#endif
