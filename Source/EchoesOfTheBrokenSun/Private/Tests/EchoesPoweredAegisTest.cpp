#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesEntityView.h"
#include "EchoesSimCore/Simulation.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesPoweredAegisTest,
    "Echoes.Runtime.Gameplay.PoweredAegis",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesPoweredAegisTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the powered-Aegis test world."));
        return false;
    }

    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge =
        World != nullptr
            ? World->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (!TestNotNull(TEXT("Aegis world owns the simulation subsystem"), Bridge) ||
        !TestTrue(TEXT("Aegis scenario starts"),
                  Bridge != nullptr && Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const echoes::sim::Simulation* Simulation = Bridge->GetSimulation();
    TestNotNull(TEXT("The deterministic simulation is available"), Simulation);
    if (Simulation == nullptr)
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    TestEqual(
        TEXT("The canonical Aegis connection radius is authored"),
        Simulation->Config().rules.poweredAegis.connectionRadiusRaw,
        8 * echoes::sim::kFixedScale);
    const echoes::sim::EntityArchetypeRules& AegisRules =
        Simulation->Config().rules.archetypes[
            static_cast<std::size_t>(echoes::sim::Faction::MeridianCompact)]
            [static_cast<std::size_t>(
                echoes::sim::EntityType::UtilityStructure)];
    TestEqual(TEXT("The canonical Aegis range is authored"),
              AegisRules.attackRangeRaw, 9 * echoes::sim::kFixedScale);
    TestEqual(TEXT("The canonical Aegis damage is authored"),
              AegisRules.attackDamage, 28);
    TestEqual(TEXT("The canonical Aegis cadence is authored"),
              AegisRules.attackPeriodTicks,
              static_cast<echoes::sim::Tick>(20));

    const echoes::sim::Entity* ScenarioAegis = nullptr;
    for (const echoes::sim::Entity& Entity : Simulation->Entities())
    {
        if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            Entity.faction == echoes::sim::Faction::MeridianCompact &&
            Entity.type == echoes::sim::EntityType::UtilityStructure)
        {
            ScenarioAegis = &Entity;
            break;
        }
    }
    if (TestNotNull(TEXT("The actual scenario contains an Aegis Post"),
                    ScenarioAegis))
    {
        TestTrue(TEXT("The scenario Aegis is connected to its Anchor network"),
                 ScenarioAegis->aegisPowered);
        AEchoesEntityView* View = World->SpawnActor<AEchoesEntityView>();
        if (TestNotNull(TEXT("The Aegis presentation actor spawns"), View))
        {
            View->ApplyAuthoritativeState(*ScenarioAegis, true);
            TestTrue(
                TEXT("Powered Aegis state has a non-color geometric field"),
                View->IsAegisPowerFieldVisible());
            echoes::sim::Entity Unpowered = *ScenarioAegis;
            Unpowered.aegisPowered = false;
            View->ApplyAuthoritativeState(Unpowered, true);
            TestFalse(TEXT("Power loss removes the powered geometry"),
                      View->IsAegisPowerFieldVisible());
            View->Destroy();
        }
    }

    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors() && !WorldWrapper.HasFailed();
}

#endif
