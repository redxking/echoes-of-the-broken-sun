#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesEntityView.h"
#include "EchoesSimCore/Simulation.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesTerrainView.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Tests/AutomationCommon.h"

namespace
{
struct FPreservedFile final
{
    explicit FPreservedFile(FString InPath)
        : Path(MoveTemp(InPath))
    {
        bExisted = IFileManager::Get().FileExists(*Path);
        if (bExisted)
        {
            FFileHelper::LoadFileToArray(Contents, *Path);
        }
    }

    ~FPreservedFile()
    {
        IFileManager::Get().Delete(*Path, false, true, true);
        if (bExisted)
        {
            FFileHelper::SaveArrayToFile(Contents, *Path);
        }
    }

    FString Path;
    TArray<uint8> Contents;
    bool bExisted = false;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesQuickSaveLoadTest,
    "Echoes.Runtime.Persistence.QuickSaveLoad",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesQuickSaveLoadTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    const FString SavePath = UEchoesSimulationSubsystem::GetQuickSavePath();
    FPreservedFile PreservedPrimary(SavePath);
    FPreservedFile PreservedBackup(SavePath + TEXT(".bak"));
    FPreservedFile PreservedTemporary(SavePath + TEXT(".tmp"));
    IFileManager::Get().Delete(*SavePath, false, true, true);
    IFileManager::Get().Delete(*(SavePath + TEXT(".bak")), false, true, true);
    IFileManager::Get().Delete(*(SavePath + TEXT(".tmp")), false, true, true);

    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the quick-save test world."));
        return false;
    }

    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge =
        World != nullptr
            ? World->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (!TestNotNull(TEXT("Quick-save world owns the simulation subsystem"), Bridge) ||
        !TestTrue(TEXT("Quick-save scenario starts"),
                  Bridge != nullptr && Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    echoes::sim::EntityId ScoutId = 0;
    for (const echoes::sim::Entity& Entity : Bridge->GetSimulation()->Entities())
    {
        if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            Entity.type == echoes::sim::EntityType::Soldier &&
            Entity.position == echoes::sim::Vec2::FromTiles(16, 10))
        {
            ScoutId = Entity.id;
            break;
        }
    }
    if (!TestTrue(TEXT("Quick-save route scout exists"), ScoutId != 0))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const auto MoveScout = [this, Bridge, ScoutId](
                               const echoes::sim::Vec2& Destination)
    {
        FString Feedback;
        if (!Bridge->IssueCommand(
                echoes::sim::CommandType::Move,
                ScoutId,
                0,
                Bridge->SimToWorld(Destination),
                echoes::sim::FutureWellChoice::Dormant,
                Feedback))
        {
            AddError(FString::Printf(TEXT("Move before save failed: %s"), *Feedback));
            return false;
        }
        for (int32 TickIndex = 0; TickIndex < 400; ++TickIndex)
        {
            Bridge->Tick(0.05f);
            const echoes::sim::Entity* Scout = Bridge->FindEntity(ScoutId);
            if (Scout != nullptr && Scout->position == Destination)
            {
                return true;
            }
        }
        AddError(TEXT("Scout did not reach the checkpoint destination."));
        return false;
    };

    FString Feedback;
    if (!TestTrue(TEXT("Scout reaches first checkpoint"),
                  MoveScout(echoes::sim::Vec2::FromTiles(18, 18))) ||
        !TestTrue(TEXT("First checkpoint commits"),
                  Bridge->QuickSaveScenario(Feedback)))
    {
        AddInfo(FString::Printf(TEXT("First save feedback: %s"), *Feedback));
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    const uint64 FirstTick = Bridge->GetSimulation()->CurrentTick();
    const uint64 FirstChecksum = Bridge->GetSimulation()->StateChecksum();

    if (!TestTrue(TEXT("Scout reaches second checkpoint"),
                  MoveScout(echoes::sim::Vec2::FromTiles(22, 22))) ||
        !TestTrue(TEXT("Second checkpoint commits"),
                  Bridge->QuickSaveScenario(Feedback)))
    {
        AddInfo(FString::Printf(TEXT("Second save feedback: %s"), *Feedback));
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    const uint64 SecondTick = Bridge->GetSimulation()->CurrentTick();
    const uint64 SecondChecksum = Bridge->GetSimulation()->StateChecksum();
    TestTrue(TEXT("Second checkpoint advances deterministic time"),
             SecondTick > FirstTick);
    TestTrue(TEXT("Second checkpoint changes deterministic state"),
             SecondChecksum != FirstChecksum);
    TestTrue(TEXT("Second save retains a prior-generation backup"),
             IFileManager::Get().FileExists(*(SavePath + TEXT(".bak"))));

    if (!TestTrue(TEXT("Scout moves beyond the saved state"),
                  MoveScout(echoes::sim::Vec2::FromTiles(26, 26))))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    TestTrue(TEXT("Live state differs before primary restore"),
             Bridge->GetSimulation()->StateChecksum() != SecondChecksum);

    if (TestTrue(TEXT("Primary checkpoint restores"),
                 Bridge->QuickLoadScenario(Feedback)))
    {
        TestEqual(TEXT("Primary restore returns to exact tick"),
                  Bridge->GetSimulation()->CurrentTick(),
                  SecondTick);
        TestEqual(TEXT("Primary restore returns to exact checksum"),
                  Bridge->GetSimulation()->StateChecksum(),
                  SecondChecksum);
        TestNotNull(TEXT("Primary restore reconstructs the scout view"),
                    Bridge->FindEntityView(ScoutId));
        AEchoesTerrainView* TerrainView = Bridge->GetTerrainView();
        if (TestNotNull(TEXT("Primary restore reconstructs terrain"), TerrainView))
        {
            TestEqual(TEXT("Restored terrain retains the Glass Scar"),
                      TerrainView->GetBlockedTileCount(),
                      165);
        }
    }
    else
    {
        AddInfo(FString::Printf(TEXT("Primary load feedback: %s"), *Feedback));
    }

    TestTrue(TEXT("Corrupt primary is written for fallback exercise"),
             FFileHelper::SaveStringToFile(TEXT("corrupt"), *SavePath));
    if (TestTrue(TEXT("Prior-generation backup restores"),
                 Bridge->QuickLoadScenario(Feedback)))
    {
        TestTrue(TEXT("Fallback is identified to the player"),
                 Feedback.Contains(TEXT("prior-generation backup")));
        TestEqual(TEXT("Backup restore returns to first checkpoint tick"),
                  Bridge->GetSimulation()->CurrentTick(),
                  FirstTick);
        TestEqual(TEXT("Backup restore returns to first checkpoint checksum"),
                  Bridge->GetSimulation()->StateChecksum(),
                  FirstChecksum);
        TestNotNull(TEXT("Backup restore reconstructs the scout view"),
                    Bridge->FindEntityView(ScoutId));

        FString PostLoadFeedback;
        TestTrue(
            TEXT("A restored match accepts a fresh local command sequence"),
            Bridge->IssueCommand(
                echoes::sim::CommandType::Stop,
                ScoutId,
                0,
                Bridge->SimToWorld(Bridge->FindEntity(ScoutId)->position),
                echoes::sim::FutureWellChoice::Dormant,
                PostLoadFeedback));
    }
    else
    {
        AddInfo(FString::Printf(TEXT("Backup load feedback: %s"), *Feedback));
    }

    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors() && !WorldWrapper.HasFailed();
}

#endif
