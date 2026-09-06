#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesCampaignProgress.h"
#include "EchoesMatchReplay.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesTestSaveEnvironment.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/FileHelper.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesAsyncCheckpointTest,
    "Echoes.Runtime.Persistence.AsyncCheckpointLifecycle",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesAsyncCheckpointTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    FEchoesScopedTestSaveEnvironment Saves(*this);
    if (!Saves.IsReady())
    {
        return false;
    }

    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the async-checkpoint test world."));
        return false;
    }
    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge = World != nullptr
        ? World->GetSubsystem<UEchoesSimulationSubsystem>()
        : nullptr;
    if (!TestNotNull(TEXT("World owns simulation subsystem"), Bridge) ||
        !TestTrue(
            TEXT("Offline skirmish starts"),
            Bridge != nullptr && Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    FString Feedback;
    Bridge->SetScenarioPaused(false);
    const uint64 CapturedTick = Bridge->GetSimulation()->CurrentTick();
    const uint64 CapturedChecksum = Bridge->GetSimulation()->StateChecksum();
    TestTrue(
        TEXT("Player quick save returns pending without blocking"),
        Bridge->RequestQuickSaveScenario(Feedback));
    TestEqual(
        TEXT("Accepted request reports pending"),
        Bridge->GetCheckpointSaveStatus().State,
        EEchoesCheckpointSaveState::Pending);
    TestTrue(
        TEXT("Pending feedback is explicit"),
        Feedback.Contains(TEXT("SAVE_PENDING")));

    // Move authoritative time after capture. The worker owns a value copy and
    // must serialize the earlier tick without touching this live simulation.
    echoes::sim::Simulation* LiveSimulation =
        const_cast<echoes::sim::Simulation*>(Bridge->GetSimulation());
    LiveSimulation->Step(20);
    TestEqual(
        TEXT("Live simulation advanced after immutable capture"),
        LiveSimulation->CurrentTick(),
        CapturedTick + 20);

    TestTrue(
        TEXT("Load drains the pending request and restores it"),
        Bridge->QuickLoadScenario(Feedback));
    TestEqual(
        TEXT("Load restored the captured tick"),
        Bridge->GetSimulation()->CurrentTick(),
        CapturedTick);
    TestEqual(
        TEXT("Load restored the captured checksum"),
        Bridge->GetSimulation()->StateChecksum(),
        CapturedChecksum);
    TestEqual(
        TEXT("Completed request reports success"),
        Bridge->GetCheckpointSaveStatus().State,
        EEchoesCheckpointSaveState::Succeeded);

    const FString SlotOneCheckpoint = Bridge->GetActiveQuickSavePath();
    TestTrue(
        TEXT("Second pending checkpoint is accepted before slot switch"),
        Bridge->RequestQuickSaveScenario(Feedback));
    FEchoesCampaignProgress EmptySlot;
    const FString SlotTwoPath =
        UEchoesSimulationSubsystem::GetJourneySlotPath(2);
    TestTrue(
        TEXT("Independent slot fixture is created"),
        FEchoesCampaignProgressStore::SaveAtomic(
            SlotTwoPath, EmptySlot, Feedback));
    Bridge->SetScenarioPaused(true);
    TestTrue(
        TEXT("Slot switch drains the checkpoint bound to its original slot"),
        Bridge->SelectJourneySlot(2, Feedback));
    TestEqual(TEXT("Slot 2 becomes active"), Bridge->GetActiveJourneySlot(), 2);
    TestTrue(
        TEXT("Pending Slot 1 generation committed to the captured path"),
        IFileManager::Get().FileExists(*SlotOneCheckpoint));

    Bridge->SetScenarioPaused(false);
    LiveSimulation =
        const_cast<echoes::sim::Simulation*>(Bridge->GetSimulation());
    LiveSimulation->Step(6'000);
    Bridge->Tick(0.05F);
    TestTrue(
        TEXT("6,000-tick offline skirmish cadence requests an autosave"),
        Bridge->GetCheckpointSaveStatus().bAutosave &&
            Bridge->GetCheckpointSaveStatus().Reason ==
                EEchoesAutosaveReason::TickCadence);
    TestTrue(
        TEXT("Cadence autosave completes"),
        Bridge->WaitForCheckpointSaves(Feedback));
    TestEqual(
        TEXT("Cadence completion reports success"),
        Bridge->GetCheckpointSaveStatus().State,
        EEchoesCheckpointSaveState::Succeeded);
    TestTrue(
        TEXT("Dedicated autosave generation exists"),
        IFileManager::Get().FileExists(*Bridge->GetActiveAutosavePath()));
    TestTrue(
        TEXT("Measured immutable capture is within the release snapshot budget"),
        Bridge->GetCheckpointSaveStatus().CaptureMicroseconds <= 10'000);
    TestTrue(
        TEXT("Measured initiation is within the release initiation budget"),
        Bridge->GetCheckpointSaveStatus().InitiationMicroseconds <= 250'000);
    TestTrue(
        TEXT("Background encoding and completion latency are reported"),
        Bridge->GetCheckpointSaveStatus().EncodingMicroseconds > 0 &&
            Bridge->GetCheckpointSaveStatus().CompletionMicroseconds > 0 &&
            Bridge->GetCheckpointSaveStatus().TotalCompletionMicroseconds >=
                Bridge->GetCheckpointSaveStatus().CompletionMicroseconds);

    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesAsyncCheckpointReplayBindingFailureTest,
    "Echoes.Runtime.Persistence.AsyncCheckpointReplayBindingFailure",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesAsyncCheckpointReplayBindingFailureTest::RunTest(
    const FString& Parameters)
{
    (void)Parameters;
    FEchoesScopedTestSaveEnvironment Saves(*this);
    if (!Saves.IsReady())
    {
        return false;
    }

    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the replay-binding failure test world."));
        return false;
    }
    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge = World != nullptr
        ? World->GetSubsystem<UEchoesSimulationSubsystem>()
        : nullptr;
    if (!TestNotNull(TEXT("Failure world owns simulation subsystem"), Bridge) ||
        !TestTrue(TEXT("Failure fixture starts an offline skirmish"),
            Bridge != nullptr && Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    FString Feedback;
    if (!TestTrue(TEXT("First durable checkpoint baseline commits"),
            Bridge->QuickSaveScenario(Feedback)))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    echoes::sim::Simulation* LiveSimulation =
        const_cast<echoes::sim::Simulation*>(Bridge->GetSimulation());
    LiveSimulation->Step();
    if (!TestTrue(TEXT("Second durable checkpoint baseline rotates backup"),
            Bridge->QuickSaveScenario(Feedback)))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const FString PrimaryPath = Bridge->GetActiveQuickSavePath();
    const FString BackupPath = PrimaryPath + TEXT(".bak");
    TArray<uint8> PrimaryBefore;
    TArray<uint8> BackupBefore;
    if (!TestTrue(TEXT("Primary and backup baselines are readable"),
            FFileHelper::LoadFileToArray(PrimaryBefore, *PrimaryPath) &&
            !PrimaryBefore.IsEmpty() &&
            FFileHelper::LoadFileToArray(BackupBefore, *BackupPath) &&
            !BackupBefore.IsEmpty()))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    std::string ReplayProofError;
    const echoes::sim::ReplayRecord ReplayProof =
        LiveSimulation->ExportReplay(&ReplayProofError);
    TArray<uint8> ValidatedReplayBytes;
    FString ReplayBindingError;
    if (!TestTrue(TEXT("Current replay proof encodes semantically"),
            ReplayProof.version != 0 && ReplayProofError.empty() &&
            FEchoesMatchReplayStore::EncodeReplayRecord(
                ReplayProof, ValidatedReplayBytes, ReplayBindingError)))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    TArray<uint8> GeneratedCheckpointPayload;
    TestTrue(TEXT("Committed generation matches its exact validated replay proof"),
        FEchoesMatchReplayStore::ExtractGeneratedCheckpointPayload(
            PrimaryBefore,
            ValidatedReplayBytes,
            GeneratedCheckpointPayload,
            ReplayBindingError));
    if (!ValidatedReplayBytes.IsEmpty())
    {
        ValidatedReplayBytes.Last() ^= 0x01;
    }
    TestFalse(TEXT("Mutated replay proof cannot authorize generated bytes"),
        FEchoesMatchReplayStore::ExtractGeneratedCheckpointPayload(
            PrimaryBefore,
            ValidatedReplayBytes,
            GeneratedCheckpointPayload,
            ReplayBindingError));
    TestTrue(TEXT("Replay proof mismatch is explicit"),
        ReplayBindingError.Contains(TEXT("does not match")));

    AddExpectedError(
        TEXT("ECHOES_CHECKPOINT_COMPLETE"),
        EAutomationExpectedErrorFlags::Contains,
        2);

    const echoes::sim::Terrain PriorTerrain =
        LiveSimulation->TerrainAt(0, 0);
    const echoes::sim::Terrain MutatedTerrain =
        PriorTerrain == echoes::sim::Terrain::Open
            ? echoes::sim::Terrain::Blocked
            : echoes::sim::Terrain::Open;
    TestTrue(TEXT("Fixture creates an unrecorded authoritative mutation"),
        LiveSimulation->SetTerrainTile(0, 0, MutatedTerrain));
    TestTrue(TEXT("Snapshot-mismatched checkpoint is accepted asynchronously"),
        Bridge->RequestQuickSaveScenario(Feedback));
    TestFalse(TEXT("Snapshot and replay mismatch fails before durable write"),
        Bridge->WaitForCheckpointSaves(Feedback));
    TestTrue(TEXT("Snapshot mismatch is attributed to replay binding"),
        Feedback.Contains(TEXT("[SAVE_REPLAY_BINDING_FAILED]")) &&
        Feedback.Contains(TEXT("checksum")));
    TArray<uint8> PrimaryAfterMutation;
    TArray<uint8> BackupAfterMutation;
    TestTrue(TEXT("Snapshot mismatch leaves primary byte-identical"),
        FFileHelper::LoadFileToArray(PrimaryAfterMutation, *PrimaryPath) &&
        PrimaryAfterMutation == PrimaryBefore);
    TestTrue(TEXT("Snapshot mismatch leaves backup byte-identical"),
        FFileHelper::LoadFileToArray(BackupAfterMutation, *BackupPath) &&
        BackupAfterMutation == BackupBefore);

    LiveSimulation->DisableReplayExport();
    TestTrue(TEXT("Replay-disabled checkpoint is accepted asynchronously"),
        Bridge->RequestQuickSaveScenario(Feedback));
    TestEqual(TEXT("Replay-disabled request first reports pending"),
        Bridge->GetCheckpointSaveStatus().State,
        EEchoesCheckpointSaveState::Pending);
    TestFalse(TEXT("Replay-disabled checkpoint fails during background encoding"),
        Bridge->WaitForCheckpointSaves(Feedback));
    TestEqual(TEXT("Replay-binding failure is explicit in save status"),
        Bridge->GetCheckpointSaveStatus().State,
        EEchoesCheckpointSaveState::Failed);
    TestTrue(TEXT("Replay-binding failure is actionable and attributable"),
        Feedback.Contains(TEXT("[SAVE_REPLAY_BINDING_FAILED]")) &&
        Feedback.Contains(TEXT("authoritative replay prefix")));

    TArray<uint8> PrimaryAfter;
    TArray<uint8> BackupAfter;
    TestTrue(TEXT("Failed replay binding leaves primary byte-identical"),
        FFileHelper::LoadFileToArray(PrimaryAfter, *PrimaryPath) &&
        PrimaryAfter == PrimaryBefore);
    TestTrue(TEXT("Failed replay binding leaves backup byte-identical"),
        FFileHelper::LoadFileToArray(BackupAfter, *BackupPath) &&
        BackupAfter == BackupBefore);
    TestFalse(TEXT("Failed encoding leaves no primary temporary file"),
        IFileManager::Get().FileExists(*(PrimaryPath + TEXT(".tmp"))));
    TestFalse(TEXT("Failed encoding leaves no staged backup file"),
        IFileManager::Get().FileExists(*(BackupPath + TEXT(".tmp"))));

    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors() && !WorldWrapper.HasFailed();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesAsyncCheckpointStressBudgetTest,
    "Echoes.Runtime.Persistence.AsyncCheckpointStressBudget",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesAsyncCheckpointStressBudgetTest::RunTest(
    const FString& Parameters)
{
    (void)Parameters;
    FEchoesScopedTestSaveEnvironment Saves(*this);
    if (!Saves.IsReady())
    {
        return false;
    }

    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the checkpoint-budget test world."));
        return false;
    }
    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge = World != nullptr
        ? World->GetSubsystem<UEchoesSimulationSubsystem>()
        : nullptr;
    if (!TestNotNull(TEXT("Budget world owns simulation subsystem"), Bridge) ||
        !TestTrue(
            TEXT("Four-force checkpoint qualification fixture starts"),
            Bridge != nullptr && Bridge->StartStressScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const echoes::sim::Simulation* Simulation = Bridge->GetSimulation();
    if (!TestNotNull(TEXT("Qualification simulation exists"), Simulation))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    TestEqual(
        TEXT("Qualification capture contains the authored 400-unit scale"),
        static_cast<int32>(Simulation->Entities().size()),
        401);

    std::string ReplayError;
    const echoes::sim::ReplayRecord ExpectedReplay =
        Simulation->ExportReplay(&ReplayError);
    if (!TestTrue(
            TEXT("Scale fixture exports a valid full replay prefix"),
            ExpectedReplay.version != 0 && ReplayError.empty()) ||
        !TestEqual(
            TEXT("Replay prefix contains all authored stress orders"),
            static_cast<int32>(ExpectedReplay.commands.size()),
            396))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    Bridge->AllowNextStressCheckpointForTesting();
    FString Feedback;
    const double RequestStarted = FPlatformTime::Seconds();
    const bool bRequested = Bridge->RequestQuickSaveScenario(Feedback);
    const uint64 RequestWallMicroseconds = static_cast<uint64>(FMath::Max(
        0.0,
        (FPlatformTime::Seconds() - RequestStarted) * 1'000'000.0));
    if (!TestTrue(
            TEXT("One-shot scale checkpoint request is accepted"),
            bRequested))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const FEchoesCheckpointSaveStatus PendingStatus =
        Bridge->GetCheckpointSaveStatus();
    TestEqual(
        TEXT("Scale request reports pending before worker completion"),
        PendingStatus.State,
        EEchoesCheckpointSaveState::Pending);
    TestTrue(
        TEXT("400-unit immutable capture stays within ten milliseconds"),
        PendingStatus.CaptureMicroseconds <= 10'000);
    TestTrue(
        TEXT("Complete game-thread request stays within 250 milliseconds"),
        RequestWallMicroseconds <= 250'000 &&
            PendingStatus.InitiationMicroseconds <= 250'000);

    echoes::sim::Simulation* LiveSimulation =
        const_cast<echoes::sim::Simulation*>(Simulation);
    LiveSimulation->Step(2);
    TestTrue(
        TEXT("Live scale simulation advances while captured state is immutable"),
        LiveSimulation->CurrentTick() > ExpectedReplay.finalTick);
    if (!TestTrue(
            TEXT("Scale checkpoint worker commits successfully"),
            Bridge->WaitForCheckpointSaves(Feedback)))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const FEchoesCheckpointSaveStatus CompletedStatus =
        Bridge->GetCheckpointSaveStatus();
    TestEqual(
        TEXT("Scale completion is reported as success"),
        CompletedStatus.State,
        EEchoesCheckpointSaveState::Succeeded);
    TestTrue(
        TEXT("Worker, encoding, and end-to-end completion scopes are distinct"),
        CompletedStatus.EncodingMicroseconds > 0 &&
            CompletedStatus.CompletionMicroseconds >=
                CompletedStatus.EncodingMicroseconds &&
            CompletedStatus.TotalCompletionMicroseconds >=
                CompletedStatus.CompletionMicroseconds);

    TArray<uint8> PersistedBytes;
    if (!TestTrue(
            TEXT("Committed scale checkpoint is readable"),
            FFileHelper::LoadFileToArray(
                PersistedBytes, *Bridge->GetActiveQuickSavePath()) &&
                !PersistedBytes.IsEmpty()))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    TArray<uint8> CheckpointPayload;
    echoes::sim::ReplayRecord PersistedReplay;
    FString ExtractError;
    if (!TestTrue(
            TEXT("Scale checkpoint carries a CRC-bound replay prefix"),
            FEchoesMatchReplayStore::ExtractCheckpointPayload(
                PersistedBytes,
                CheckpointPayload,
                PersistedReplay,
                ExtractError) ==
                EEchoesCheckpointReplayBindingRead::Bound))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    TestTrue(
        TEXT("Persisted replay prefix retains every captured command"),
        PersistedReplay.commands == ExpectedReplay.commands);
    TestTrue(
        TEXT("Persisted replay prefix retains the full scale baseline"),
        PersistedReplay.initialSnapshot == ExpectedReplay.initialSnapshot);
    TestEqual(
        TEXT("Persisted replay prefix remains bound to captured tick"),
        PersistedReplay.finalTick,
        ExpectedReplay.finalTick);
    TestEqual(
        TEXT("Persisted replay prefix remains bound to captured checksum"),
        PersistedReplay.finalChecksum,
        ExpectedReplay.finalChecksum);

    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors() && !WorldWrapper.HasFailed();
}

#endif
