#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesEntityView.h"
#include "EchoesSimCore/Simulation.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/World.h"
#include "Misc/ConfigCacheIni.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesWaystoneMigrationTest,
    "Echoes.Runtime.Gameplay.WaystoneMigration",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesWaystoneMigrationTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the Waystone migration test world."));
        return false;
    }

    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge =
        World != nullptr
            ? World->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (!TestNotNull(TEXT("Waystone world owns the simulation subsystem"), Bridge) ||
        !TestTrue(TEXT("Waystone scenario starts"),
                  Bridge != nullptr && Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    uint32 OpponentWaystone = 0;
    uint32 LocalPowerLink = 0;
    for (const echoes::sim::Entity& Entity : Bridge->GetSimulation()->Entities())
    {
        if (Entity.owner == UEchoesSimulationSubsystem::OpponentPlayerId &&
            Entity.faction == echoes::sim::Faction::KharuunAssemblies &&
            Entity.type == echoes::sim::EntityType::Dropoff)
        {
            OpponentWaystone = Entity.id;
        }
        else if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
                 Entity.type == echoes::sim::EntityType::Dropoff)
        {
            LocalPowerLink = Entity.id;
        }
    }
    TestTrue(TEXT("The complete scenario contains an opposing Waystone"),
             OpponentWaystone != 0);
    TestTrue(TEXT("The scenario retains the local Power Link"),
             LocalPowerLink != 0);

    const echoes::sim::Entity* Waystone = Bridge->FindEntity(OpponentWaystone);
    if (TestNotNull(TEXT("The Waystone is authoritative"), Waystone))
    {
        TestTrue(TEXT("A completed Waystone begins rooted"),
                 Waystone->waystoneMode == echoes::sim::WaystoneMode::Rooted);
        AEchoesEntityView* Presentation =
            World->SpawnActor<AEchoesEntityView>();
        if (TestNotNull(TEXT("Waystone presentation proxy spawns"), Presentation))
        {
            Presentation->ApplyAuthoritativeState(*Waystone, true);
            TestTrue(TEXT("Rooted Waystone exposes a public state field"),
                     Presentation->IsWaystoneStateVisible());
            TestTrue(TEXT("Presentation retains rooted public state"),
                     Presentation->GetWaystoneMode() ==
                         echoes::sim::WaystoneMode::Rooted);

            echoes::sim::Entity MobileState = *Waystone;
            MobileState.waystoneMode = echoes::sim::WaystoneMode::Mobile;
            Presentation->ApplyAuthoritativeState(MobileState, true);
            TestTrue(TEXT("Mobile Waystone state remains publicly visible"),
                     Presentation->IsWaystoneStateVisible());
            TestTrue(TEXT("Presentation updates to mobile public state"),
                     Presentation->GetWaystoneMode() ==
                         echoes::sim::WaystoneMode::Mobile);
            Presentation->Destroy();
        }
    }

    FString Feedback;
    TestFalse(
        TEXT("A Meridian Power Link cannot use Waystone migration"),
        Bridge->IssueCommand(
            echoes::sim::CommandType::ToggleWaystoneRoot,
            LocalPowerLink,
            0,
            Bridge->SimToWorld(Bridge->FindEntity(LocalPowerLink)->position),
            echoes::sim::FutureWellChoice::Dormant,
            Feedback));
    TestTrue(TEXT("Invalid migration is reason-coded"),
             Feedback.StartsWith(TEXT("[WAYSTONE_REQUIRED]")));

    TArray<FString> InputMappings;
    GConfig->GetArray(
        TEXT("/Script/Engine.InputSettings"),
        TEXT("ActionMappings"),
        InputMappings,
        GInputIni);
    TestTrue(
        TEXT("Hyphen Waystone migration mapping is present"),
        InputMappings.ContainsByPredicate(
            [](const FString& Mapping)
            {
                return Mapping.Contains(TEXT("ActionName=\"ToggleWaystoneRoot\"")) &&
                       Mapping.Contains(TEXT("Key=Hyphen"));
            }));

    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors() && !WorldWrapper.HasFailed();
}

#endif
