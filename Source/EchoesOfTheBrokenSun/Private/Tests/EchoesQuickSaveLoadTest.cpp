#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesEntityView.h"
#include "EchoesSimCore/Simulation.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesTerrainView.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
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
    const FString PrologueSavePath = FPaths::Combine(
        FPaths::GetPath(SavePath),
        TEXT("EchoesQuickSaveWhatTheLedgerKeeps.bin"));
    FPreservedFile PreservedPrimary(SavePath);
    FPreservedFile PreservedBackup(SavePath + TEXT(".bak"));
    FPreservedFile PreservedBackupTemporary(
        SavePath + TEXT(".bak.tmp"));
    FPreservedFile PreservedTemporary(SavePath + TEXT(".tmp"));
    FPreservedFile PreservedProloguePrimary(PrologueSavePath);
    FPreservedFile PreservedPrologueBackup(
        PrologueSavePath + TEXT(".bak"));
    FPreservedFile PreservedPrologueBackupTemporary(
        PrologueSavePath + TEXT(".bak.tmp"));
    FPreservedFile PreservedPrologueTemporary(
        PrologueSavePath + TEXT(".tmp"));
    IFileManager::Get().Delete(*SavePath, false, true, true);
    IFileManager::Get().Delete(*(SavePath + TEXT(".bak")), false, true, true);
    IFileManager::Get().Delete(
        *(SavePath + TEXT(".bak.tmp")), false, true, true);
    IFileManager::Get().Delete(*(SavePath + TEXT(".tmp")), false, true, true);
    IFileManager::Get().Delete(*PrologueSavePath, false, true, true);
    IFileManager::Get().Delete(
        *(PrologueSavePath + TEXT(".bak")), false, true, true);
    IFileManager::Get().Delete(
        *(PrologueSavePath + TEXT(".bak.tmp")), false, true, true);
    IFileManager::Get().Delete(
        *(PrologueSavePath + TEXT(".tmp")), false, true, true);

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
    const auto LegacySnapshot =
        Bridge->GetSimulation()->SaveSnapshot();
    TArray<uint8> LegacySnapshotBytes;
    LegacySnapshotBytes.Append(
        LegacySnapshot.data(),
        static_cast<int32>(LegacySnapshot.size()));
    TestTrue(
        TEXT("A pre-container raw checkpoint fixture can be written"),
        !LegacySnapshotBytes.IsEmpty() &&
            FFileHelper::SaveArrayToFile(LegacySnapshotBytes, *SavePath));
    TestTrue(
        TEXT("The pre-container raw checkpoint remains load-compatible"),
        Bridge->QuickLoadScenario(Feedback));
    TestTrue(
        TEXT("Legacy loading does not misidentify the primary as a recovery file"),
        !Feedback.Contains(TEXT("prior-generation")) &&
            !Feedback.Contains(TEXT("staged")));

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
    TArray<uint8> MigratedLegacyBackupBytes;
    TestTrue(
        TEXT("The first context-bound save preserves the load-compatible legacy primary"),
        FFileHelper::LoadFileToArray(
            MigratedLegacyBackupBytes,
            *(SavePath + TEXT(".bak"))) &&
            MigratedLegacyBackupBytes == LegacySnapshotBytes);
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

    TArray<uint8> ValidBackupBytes;
    TestTrue(TEXT("The valid recovery generation can be retained exactly"),
             FFileHelper::LoadFileToArray(
                 ValidBackupBytes,
                 *(SavePath + TEXT(".bak"))) &&
                 !ValidBackupBytes.IsEmpty());

    Bridge->FailNextQuickSaveBackupRotationForTesting();
    FString RotationFeedback;
    TestTrue(
        TEXT("A forced backup-rotation fault still commits the new primary"),
        Bridge->QuickSaveScenario(RotationFeedback));
    TestTrue(
        TEXT("The rotation fault is disclosed without overstating backup state"),
        RotationFeedback.Contains(TEXT("backup rotation was deferred")) &&
            RotationFeedback.Contains(TEXT("remains staged for recovery")));
    TestTrue(
        TEXT("The prior validated primary remains as a staged recovery file"),
        IFileManager::Get().FileExists(*(SavePath + TEXT(".bak.tmp"))));
    TArray<uint8> BackupAfterRotationFault;
    TestTrue(
        TEXT("A failed atomic rotation leaves the existing backup untouched"),
        FFileHelper::LoadFileToArray(
            BackupAfterRotationFault,
            *(SavePath + TEXT(".bak"))) &&
            BackupAfterRotationFault == ValidBackupBytes);
    TestTrue(
        TEXT("The primary can be corrupted to exercise staged recovery"),
        FFileHelper::SaveStringToFile(
            TEXT("corrupt primary before staged recovery"),
            *SavePath));
    if (TestTrue(
            TEXT("The newer staged generation is preferred over the older valid backup"),
            Bridge->QuickLoadScenario(RotationFeedback)))
    {
        TestTrue(
            TEXT("The staged source is identified to the player"),
            RotationFeedback.Contains(
                TEXT("staged prior-generation recovery")));
        TestEqual(
            TEXT("Staged recovery returns to the exact second checkpoint tick"),
            Bridge->GetSimulation()->CurrentTick(),
            SecondTick);
        TestEqual(
            TEXT("Staged recovery returns to the exact second checkpoint checksum"),
            Bridge->GetSimulation()->StateChecksum(),
            SecondChecksum);
    }
    TArray<uint8> BackupAfterStagedRecovery;
    TestTrue(
        TEXT("Staged recovery leaves the older valid backup untouched"),
        FFileHelper::LoadFileToArray(
            BackupAfterStagedRecovery,
            *(SavePath + TEXT(".bak"))) &&
            BackupAfterStagedRecovery == ValidBackupBytes);
    TestTrue(
        TEXT("The staged-recovery fixture can be retired before backup-only coverage"),
        IFileManager::Get().Delete(
            *(SavePath + TEXT(".bak.tmp")),
            false,
            true,
            true));
    TestTrue(
        TEXT("A fresh primary commits after staged recovery"),
        Bridge->QuickSaveScenario(RotationFeedback));
    TestTrue(
        TEXT("The restored validated backup remains explicitly preserved"),
        RotationFeedback.Contains(
            TEXT("validated recovery checkpoint was preserved")));

    FString FactionFeedback;
    TestTrue(
        TEXT("The shared skirmish slot can build a wrong-faction primary fixture"),
        Bridge->SelectLocalFaction(
            echoes::sim::Faction::KharuunAssemblies,
            FactionFeedback) &&
            Bridge->QuickSaveScenario(FactionFeedback));
    TArray<uint8> KharuunPrimaryBytes;
    TestTrue(
        TEXT("The wrong-faction fixture is a structurally valid checkpoint"),
        FFileHelper::LoadFileToArray(KharuunPrimaryBytes, *SavePath) &&
            !KharuunPrimaryBytes.IsEmpty());
    TestTrue(
        TEXT("The wrong-faction write does not call an incompatible backup validated"),
        FactionFeedback.Contains(
            TEXT("only validated generation")) &&
            !FactionFeedback.Contains(
                TEXT("validated recovery checkpoint was preserved")));
    TestTrue(
        TEXT("The active operation can return to Meridian authority"),
        Bridge->SelectLocalFaction(
            echoes::sim::Faction::MeridianCompact,
            FactionFeedback));
    TestTrue(
        TEXT("Saving over a structurally valid wrong-faction primary succeeds"),
        Bridge->QuickSaveScenario(FactionFeedback));
    TestTrue(
        TEXT("The incompatible primary is rejected without rotating the backup"),
        FactionFeedback.Contains(
            TEXT("validated recovery checkpoint was preserved")));
    TArray<uint8> ContextPreservedBackupBytes;
    TestTrue(
        TEXT("Context rejection retains the last load-compatible checkpoint"),
        FFileHelper::LoadFileToArray(
            ContextPreservedBackupBytes,
            *(SavePath + TEXT(".bak"))) &&
            ContextPreservedBackupBytes == ValidBackupBytes);

    FString OperationFeedback;
    TestTrue(
        TEXT("The campaign prologue can build a different-operation fixture"),
        Bridge->SelectOperationMode(
            EEchoesOperationMode::CampaignPrologue,
            OperationFeedback) &&
            Bridge->QuickSaveScenario(OperationFeedback));
    TArray<uint8> ProloguePrimaryBytes;
    TestTrue(
        TEXT("The different-operation fixture is a valid context-bound checkpoint"),
        FFileHelper::LoadFileToArray(
            ProloguePrimaryBytes,
            *PrologueSavePath) &&
            !ProloguePrimaryBytes.IsEmpty());
    TestTrue(
        TEXT("The different-operation fixture can be placed in the shared slot"),
        FFileHelper::SaveArrayToFile(ProloguePrimaryBytes, *SavePath));
    TestTrue(
        TEXT("The active operation can return to the Glass Scar skirmish"),
        Bridge->SelectOperationMode(
            EEchoesOperationMode::Skirmish,
            OperationFeedback));
    TestTrue(
        TEXT("Saving over a same-faction wrong-operation primary succeeds"),
        Bridge->QuickSaveScenario(OperationFeedback));
    TestTrue(
        TEXT("Wrong-operation rejection preserves the validated recovery"),
        OperationFeedback.Contains(
            TEXT("validated recovery checkpoint was preserved")));
    TArray<uint8> OperationPreservedBackupBytes;
    TestTrue(
        TEXT("Wrong-operation rejection cannot rotate the recovery generation"),
        FFileHelper::LoadFileToArray(
            OperationPreservedBackupBytes,
            *(SavePath + TEXT(".bak"))) &&
            OperationPreservedBackupBytes == ValidBackupBytes);

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
        Bridge->Tick(0.05f);

        const uint64 RecoverySaveTick =
            Bridge->GetSimulation()->CurrentTick();
        const uint64 RecoverySaveChecksum =
            Bridge->GetSimulation()->StateChecksum();
        TestTrue(
            TEXT("A new checkpoint commits while the current primary is corrupt"),
            Bridge->QuickSaveScenario(PostLoadFeedback));
        TestTrue(
            TEXT("Saving over a corrupt primary discloses backup preservation"),
            PostLoadFeedback.Contains(
                TEXT("validated recovery checkpoint was preserved")));
        TArray<uint8> PreservedBackupBytes;
        TestTrue(
            TEXT("The last valid backup is not displaced by a corrupt primary"),
            FFileHelper::LoadFileToArray(
                PreservedBackupBytes,
                *(SavePath + TEXT(".bak"))) &&
                PreservedBackupBytes == ValidBackupBytes);

        TestTrue(
            TEXT("The replacement primary is independently loadable"),
            MoveScout(echoes::sim::Vec2::FromTiles(22, 22)) &&
                Bridge->QuickLoadScenario(PostLoadFeedback));
        TestEqual(
            TEXT("The replacement primary restores its exact tick"),
            Bridge->GetSimulation()->CurrentTick(),
            RecoverySaveTick);
        TestEqual(
            TEXT("The replacement primary restores its exact checksum"),
            Bridge->GetSimulation()->StateChecksum(),
            RecoverySaveChecksum);

        TestTrue(
            TEXT("The replacement primary can be corrupted independently"),
            FFileHelper::SaveStringToFile(
                TEXT("corrupt replacement"),
                *SavePath));
        TestTrue(
            TEXT("The preserved recovery checkpoint still restores"),
            Bridge->QuickLoadScenario(PostLoadFeedback) &&
                PostLoadFeedback.Contains(TEXT("prior-generation backup")) &&
                Bridge->GetSimulation()->CurrentTick() == FirstTick &&
                Bridge->GetSimulation()->StateChecksum() == FirstChecksum);
    }
    else
    {
        AddInfo(FString::Printf(TEXT("Backup load feedback: %s"), *Feedback));
    }

    TestTrue(
        TEXT("The primary can be corrupted for the no-valid-generation boundary"),
        FFileHelper::SaveStringToFile(
            TEXT("corrupt terminal primary"),
            *SavePath));
    TestTrue(
        TEXT("The backup can be corrupted for the no-valid-generation boundary"),
        FFileHelper::SaveStringToFile(
            TEXT("corrupt terminal backup"),
            *(SavePath + TEXT(".bak"))));
    TestTrue(
        TEXT("The staged recovery can be corrupted for the no-valid-generation boundary"),
        FFileHelper::SaveStringToFile(
            TEXT("corrupt terminal staged recovery"),
            *(SavePath + TEXT(".bak.tmp"))));
    FString InvalidGenerationFeedback;
    TestTrue(
        TEXT("A new primary can commit over three invalid generations"),
        Bridge->QuickSaveScenario(InvalidGenerationFeedback));
    TestTrue(
        TEXT("The player is told that only the new primary is validated"),
        InvalidGenerationFeedback.Contains(
            TEXT("only validated generation")) &&
            !InvalidGenerationFeedback.Contains(
                TEXT("validated recovery checkpoint was preserved")));
    TestTrue(
        TEXT("The sole validated primary is independently loadable"),
        Bridge->QuickLoadScenario(InvalidGenerationFeedback));
    TestTrue(
        TEXT("All three checkpoint generations can be invalidated"),
        FFileHelper::SaveStringToFile(
            TEXT("corrupt final primary"),
            *SavePath) &&
            FFileHelper::SaveStringToFile(
                TEXT("corrupt final backup"),
                *(SavePath + TEXT(".bak"))) &&
            FFileHelper::SaveStringToFile(
                TEXT("corrupt final staged recovery"),
                *(SavePath + TEXT(".bak.tmp"))));
    TestFalse(
        TEXT("Loading fails closed when no validated generation remains"),
        Bridge->QuickLoadScenario(InvalidGenerationFeedback));
    TestTrue(
        TEXT("The fail-closed result reports all exhausted generations"),
        InvalidGenerationFeedback.Contains(
            TEXT("[LOAD_NO_VALID_CHECKPOINT]")) &&
            InvalidGenerationFeedback.Contains(TEXT("staged=")));

    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors() && !WorldWrapper.HasFailed();
}

#endif
