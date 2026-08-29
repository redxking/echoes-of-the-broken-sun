#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesEntityView.h"
#include "EchoesSimCore/Simulation.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/World.h"
#include "Misc/ConfigCacheIni.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesBulwarkDeploymentTest,
    "Echoes.Runtime.Gameplay.BulwarkDeployment",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesBulwarkDeploymentTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the Bulwark deployment test world."));
        return false;
    }

    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge =
        World != nullptr
            ? World->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (!TestNotNull(TEXT("Bulwark world owns the simulation subsystem"), Bridge) ||
        !TestTrue(TEXT("Bulwark scenario starts"),
                  Bridge != nullptr && Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    uint32 LocalBulwark = 0;
    uint32 LocalLancer = 0;
    for (const echoes::sim::Entity& Entity : Bridge->GetSimulation()->Entities())
    {
        if (Entity.owner != UEchoesSimulationSubsystem::LocalPlayerId)
        {
            continue;
        }
        if (LocalBulwark == 0 &&
            Entity.faction == echoes::sim::Faction::MeridianCompact &&
            Entity.type == echoes::sim::EntityType::HeavyUnit)
        {
            LocalBulwark = Entity.id;
        }
        else if (LocalLancer == 0 &&
                 Entity.type == echoes::sim::EntityType::Soldier)
        {
            LocalLancer = Entity.id;
        }
    }
    if (!TestTrue(TEXT("The scenario contains a local Bulwark"), LocalBulwark != 0) ||
        !TestTrue(TEXT("The scenario contains a non-Bulwark combat unit"), LocalLancer != 0))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    FString Feedback;
    const echoes::sim::Entity* Lancer = Bridge->FindEntity(LocalLancer);
    TestFalse(
        TEXT("A Lancer cannot use Bulwark deployment"),
        Bridge->IssueCommand(
            echoes::sim::CommandType::ToggleDeploy,
            LocalLancer,
            0,
            Bridge->SimToWorld(Lancer->position) + FVector(200.0f, 0.0f, 0.0f),
            echoes::sim::FutureWellChoice::Harvest,
            Feedback));
    TestTrue(TEXT("Invalid deployment is reason-coded"),
             Feedback.StartsWith(TEXT("[BULWARK_REQUIRED]")));

    const echoes::sim::Entity* Bulwark = Bridge->FindEntity(LocalBulwark);
    const FVector FacingPoint =
        Bridge->SimToWorld(Bulwark->position) + FVector(300.0f, 0.0f, 0.0f);
    Feedback.Reset();
    TestTrue(
        TEXT("A Meridian Bulwark accepts cursor-facing deployment"),
        Bridge->IssueCommand(
            echoes::sim::CommandType::ToggleDeploy,
            LocalBulwark,
            0,
            FacingPoint,
            echoes::sim::FutureWellChoice::Harvest,
            Feedback));
    Bridge->Tick(0.05f);
    Bridge->Tick(0.05f);
    Bulwark = Bridge->FindEntity(LocalBulwark);
    TestNotNull(TEXT("Deployed Bulwark remains alive"), Bulwark);
    if (Bulwark != nullptr)
    {
        TestTrue(TEXT("Unreal adapter applies deployment state"), Bulwark->deployed);
        TestTrue(TEXT("Unreal adapter preserves east-facing cover"),
                 Bulwark->deploymentFacing == echoes::sim::Vec2::FromRaw(
                                                   echoes::sim::kFixedScale,
                                                   0));

        AEchoesEntityView* View = World->SpawnActor<AEchoesEntityView>();
        if (TestNotNull(TEXT("Deployment presentation actor spawns"), View))
        {
            View->ApplyAuthoritativeState(*Bulwark, true);
            TestTrue(TEXT("Deployed Bulwark exposes its public cover barrier"),
                     View->IsDeploymentCoverVisible());
            View->Destroy();
        }
    }

    Feedback.Reset();
    TestTrue(
        TEXT("A deployed Bulwark accepts pack-up"),
        Bridge->IssueCommand(
            echoes::sim::CommandType::ToggleDeploy,
            LocalBulwark,
            0,
            FacingPoint,
            echoes::sim::FutureWellChoice::Harvest,
            Feedback));
    Bridge->Tick(0.05f);
    Bridge->Tick(0.05f);
    TestFalse(TEXT("Pack-up clears public deployment state"),
              Bridge->FindEntity(LocalBulwark)->deployed);

    TArray<FString> InputMappings;
    GConfig->GetArray(
        TEXT("/Script/Engine.InputSettings"),
        TEXT("ActionMappings"),
        InputMappings,
        GInputIni);
    TestTrue(
        TEXT("Backslash deployment mapping is present"),
        InputMappings.ContainsByPredicate(
            [](const FString& Mapping)
            {
                return Mapping.Contains(TEXT("ActionName=\"ToggleBulwarkDeployment\"")) &&
                       Mapping.Contains(TEXT("Key=Backslash"));
            }));

    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors() && !WorldWrapper.HasFailed();
}

#endif
