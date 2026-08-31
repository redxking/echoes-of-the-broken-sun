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
    FEchoesMineralCoverTest,
    "Echoes.Runtime.Gameplay.MineralCover",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesMineralCoverTest::RunTest(const FString& Parameters)
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
        AddError(TEXT("Could not create the mineral-cover test world."));
        return false;
    }

    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge =
        World != nullptr
            ? World->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (!TestNotNull(TEXT("Mineral-cover world owns the simulation subsystem"), Bridge) ||
        !TestTrue(TEXT("Mineral-cover scenario starts"),
                  Bridge != nullptr && Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    uint32 OpponentCairnback = 0;
    uint32 LocalBulwark = 0;
    for (const echoes::sim::Entity& Entity : Bridge->GetSimulation()->Entities())
    {
        if (Entity.owner == UEchoesSimulationSubsystem::OpponentPlayerId &&
            Entity.faction == echoes::sim::Faction::KharuunAssemblies &&
            Entity.type == echoes::sim::EntityType::HeavyUnit)
        {
            OpponentCairnback = Entity.id;
        }
        else if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
                 Entity.faction == echoes::sim::Faction::MeridianCompact &&
                 Entity.type == echoes::sim::EntityType::HeavyUnit)
        {
            LocalBulwark = Entity.id;
        }
    }
    TestTrue(TEXT("The scenario contains an opposing Cairnback"),
             OpponentCairnback != 0);
    TestTrue(TEXT("The scenario retains a local Bulwark"), LocalBulwark != 0);

    const echoes::sim::Entity* Cairnback =
        Bridge->FindEntity(OpponentCairnback);
    if (TestNotNull(TEXT("The opposing Cairnback is authoritative"), Cairnback))
    {
        AEchoesEntityView* Presentation =
            World->SpawnActor<AEchoesEntityView>();
        if (TestNotNull(TEXT("Mineral-cover presentation proxy spawns"), Presentation))
        {
            echoes::sim::Entity CoverState = *Cairnback;
            CoverState.type = echoes::sim::EntityType::UtilityStructure;
            CoverState.temporaryMineralCover = true;
            CoverState.mineralCoverCreator = OpponentCairnback;
            CoverState.mineralCoverUntilTick = 300;
            CoverState.hitPoints = 180;
            CoverState.maxHitPoints = 180;
            Presentation->ApplyAuthoritativeState(CoverState, true);
            TestTrue(TEXT("Temporary cover uses a distinct public barrier proxy"),
                     Presentation->IsTemporaryMineralCover());
            TestEqual(TEXT("Temporary cover has an explicit display name"),
                      Presentation->GetDisplayName(),
                      FString(TEXT("Mineral Cover")));
            Presentation->Destroy();
        }
    }

    FString Feedback;
    const echoes::sim::Entity* Bulwark = Bridge->FindEntity(LocalBulwark);
    if (!TestNotNull(TEXT("The local Bulwark is authoritative"), Bulwark))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    TestFalse(
        TEXT("A Meridian Bulwark cannot raise Kharuun mineral cover"),
        Bridge->IssueMineralCover(
            LocalBulwark,
            Bridge->SimToWorld(Bulwark->position) + FVector(100.0f, 0.0f, 0.0f),
            Feedback));
    TestTrue(TEXT("Invalid mineral cover is reason-coded"),
             Feedback.StartsWith(TEXT("[CAIRNBACK_REQUIRED]")));

    TArray<FString> InputMappings;
    GConfig->GetArray(
        TEXT("/Script/Engine.InputSettings"),
        TEXT("ActionMappings"),
        InputMappings,
        GInputIni);
    TestTrue(
        TEXT("Shift-semicolon mineral-cover mapping is present"),
        InputMappings.ContainsByPredicate(
            [](const FString& Mapping)
            {
                return Mapping.Contains(TEXT("ActionName=\"RaiseMineralCover\"")) &&
                       Mapping.Contains(TEXT("bShift=True")) &&
                       Mapping.Contains(TEXT("Key=Semicolon"));
            }));

    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors() && !WorldWrapper.HasFailed();
}

#endif
