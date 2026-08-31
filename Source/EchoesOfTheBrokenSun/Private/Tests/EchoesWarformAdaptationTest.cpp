#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "EchoesEntityView.h"
#include "EchoesSimCore/Simulation.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/World.h"
#include "Misc/ConfigCacheIni.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesWarformAdaptationTest,
    "Echoes.Runtime.Gameplay.WarformAdaptation",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesWarformAdaptationTest::RunTest(const FString& Parameters)
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
        AddError(TEXT("Could not create the warform adaptation test world."));
        return false;
    }

    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge =
        World != nullptr
            ? World->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (!TestNotNull(TEXT("Adaptation world owns the simulation subsystem"), Bridge) ||
        !TestTrue(TEXT("Adaptation scenario starts"),
                  Bridge != nullptr && Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    uint32 OpponentBasin = 0;
    uint32 OpponentWarform = 0;
    uint32 LocalLancer = 0;
    for (const echoes::sim::Entity& Entity : Bridge->GetSimulation()->Entities())
    {
        if (Entity.owner == UEchoesSimulationSubsystem::OpponentPlayerId &&
            Entity.faction == echoes::sim::Faction::KharuunAssemblies &&
            Entity.type == echoes::sim::EntityType::Barracks)
        {
            OpponentBasin = Entity.id;
        }
        else if (Entity.owner == UEchoesSimulationSubsystem::OpponentPlayerId &&
                 Entity.faction == echoes::sim::Faction::KharuunAssemblies &&
                 Entity.type == echoes::sim::EntityType::Soldier)
        {
            OpponentWarform = Entity.id;
        }
        else if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
                 Entity.type == echoes::sim::EntityType::Soldier)
        {
            LocalLancer = Entity.id;
        }
    }
    TestTrue(TEXT("The scenario contains an opposing Growth Basin"),
             OpponentBasin != 0);
    TestTrue(TEXT("The scenario contains an opposing combat warform"),
             OpponentWarform != 0);
    TestTrue(TEXT("The scenario retains a local Lancer"), LocalLancer != 0);

    const echoes::sim::Entity* Warform = Bridge->FindEntity(OpponentWarform);
    if (TestNotNull(TEXT("The opposing warform is authoritative"), Warform))
    {
        AEchoesEntityView* Presentation =
            World->SpawnActor<AEchoesEntityView>();
        if (TestNotNull(TEXT("Warform presentation proxy spawns"), Presentation))
        {
            echoes::sim::Entity MoltingState = *Warform;
            MoltingState.pendingWarformAdaptation =
                echoes::sim::WarformAdaptation::Carapace;
            MoltingState.moltSite = OpponentBasin;
            Presentation->ApplyAuthoritativeState(MoltingState, true);
            TestTrue(TEXT("A molt exposes a non-color public state field"),
                     Presentation->IsWarformStateVisible());
            TestTrue(TEXT("Presentation retains the pending Carapace form"),
                     Presentation->GetPendingWarformAdaptation() ==
                         echoes::sim::WarformAdaptation::Carapace);

            echoes::sim::Entity StrikerState = *Warform;
            StrikerState.warformAdaptation =
                echoes::sim::WarformAdaptation::Striker;
            Presentation->ApplyAuthoritativeState(StrikerState, true);
            TestTrue(TEXT("A completed form remains publicly distinct"),
                     Presentation->IsWarformStateVisible());
            TestTrue(TEXT("Presentation updates to completed Striker form"),
                     Presentation->GetWarformAdaptation() ==
                         echoes::sim::WarformAdaptation::Striker);
            Presentation->Destroy();
        }
    }

    FString Feedback;
    TestFalse(
        TEXT("A Meridian Lancer cannot use Kharuun adaptation"),
        Bridge->IssueWarformAdaptation(
            LocalLancer,
            OpponentBasin,
            echoes::sim::WarformAdaptation::Carapace,
            Feedback));
    TestTrue(TEXT("Invalid adaptation is reason-coded"),
             Feedback.StartsWith(TEXT("[WARFORM_REQUIRED]")));

    TArray<FString> InputMappings;
    GConfig->GetArray(
        TEXT("/Script/Engine.InputSettings"),
        TEXT("ActionMappings"),
        InputMappings,
        GInputIni);
    TestTrue(
        TEXT("Shift-left-bracket Carapace mapping is present"),
        InputMappings.ContainsByPredicate(
            [](const FString& Mapping)
            {
                return Mapping.Contains(TEXT("ActionName=\"AdaptWarformCarapace\"")) &&
                       Mapping.Contains(TEXT("bShift=True")) &&
                       Mapping.Contains(TEXT("Key=LeftBracket"));
            }));
    TestTrue(
        TEXT("Shift-right-bracket Striker mapping is present"),
        InputMappings.ContainsByPredicate(
            [](const FString& Mapping)
            {
                return Mapping.Contains(TEXT("ActionName=\"AdaptWarformStriker\"")) &&
                       Mapping.Contains(TEXT("bShift=True")) &&
                       Mapping.Contains(TEXT("Key=RightBracket"));
            }));

    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors() && !WorldWrapper.HasFailed();
}

#endif
