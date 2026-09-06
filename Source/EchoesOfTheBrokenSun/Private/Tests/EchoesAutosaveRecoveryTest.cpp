#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "EchoesCampaignProgress.h"
#include "EchoesEntityView.h"
#include "EchoesPrologueMissionModel.h"
#include "EchoesSimCore/Simulation.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesAutosaveRecoveryTest,
    "Echoes.Runtime.Persistence.AutosaveRecovery",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesAutosaveRecoveryTest::RunTest(const FString& Parameters)
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
        AddError(TEXT("Could not create the autosave recovery test world."));
        return false;
    }

    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge =
        World != nullptr
            ? World->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;

    if (!TestNotNull(TEXT("Autosave world owns the simulation subsystem"), Bridge))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    FString Feedback;

    // =========================================================================
    // 1. Autosave on mission entry
    // =========================================================================
    if (!TestTrue(
        TEXT("Select Prologue mission operation mode"),
        Bridge->SelectOperationMode(
            EEchoesOperationMode::CampaignPrologue, Feedback)))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    if (!TestTrue(
        TEXT("Prologue starts and triggers mission-entry autosave"),
        Bridge->StartPrototypeScenario()) ||
        !TestTrue(
        TEXT("Mission-entry autosave reaches committed storage"),
        Bridge->WaitForCheckpointSaves(Feedback)))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const FString ExpectedAutosavePath = Bridge->GetActiveAutosavePath();
    TestFalse(TEXT("Active autosave path is non-empty"), ExpectedAutosavePath.IsEmpty());
    if (!TestTrue(
        TEXT("Autosave file exists on disk after mission entry"),
        IFileManager::Get().FileExists(*ExpectedAutosavePath)))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    TestEqual(
        TEXT("Last autosave reason is MissionEntry"),
        Bridge->GetLastAutosavedReason(),
        EEchoesAutosaveReason::MissionEntry);

    TestEqual(
        TEXT("Last autosaved tick is 0"),
        Bridge->GetLastAutosavedTick(),
        0ULL);

    TestEqual(
        TEXT("Last autosaved phase is RecoverArchive"),
        Bridge->GetLastAutosavedPhase(),
        static_cast<uint8>(EEchoesProloguePhase::RecoverArchive));

    FString ValidateError;
    if (!TestTrue(
        TEXT("Autosave checkpoint file validates cleanly on disk"),
        Bridge->ValidateCheckpointFileOnDisk(
            ExpectedAutosavePath, 0, ValidateError)))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    TArray<uint8> EntryAutosaveBytes;
    if (!TestTrue(
        TEXT("Load entry autosave bytes for container inspection and backup comparison"),
        FFileHelper::LoadFileToArray(
            EntryAutosaveBytes, *ExpectedAutosavePath) &&
            !EntryAutosaveBytes.IsEmpty()))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    uint8 ContainerVersion = 0;
    EEchoesOperationMode ContainerOp = EEchoesOperationMode::Skirmish;
    echoes::sim::Faction ContainerFaction = echoes::sim::Faction::MeridianCompact;
    uint64 ContainerBranchId = 0;
    uint32 ContainerCrc = 0;
    TArray<uint8> ContainerPayload;
    FString InspectError;
    if (!TestTrue(
        TEXT("InspectSaveContainer successfully validates container integrity and CRC-32"),
        UEchoesSimulationSubsystem::InspectSaveContainer(
            EntryAutosaveBytes,
            ContainerVersion,
            ContainerOp,
            ContainerFaction,
            ContainerBranchId,
            ContainerCrc,
            ContainerPayload,
            InspectError)))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    TestEqual(
        TEXT("Container operation matches CampaignPrologue"),
        ContainerOp,
        EEchoesOperationMode::CampaignPrologue);
    TestEqual(
        TEXT("Container faction matches MeridianCompact"),
        ContainerFaction,
        echoes::sim::Faction::MeridianCompact);
    TestTrue(
        TEXT("Container payload is non-empty"),
        !ContainerPayload.IsEmpty());
    TestTrue(
        TEXT("Container CRC is non-zero"),
        ContainerCrc != 0);

    // =========================================================================
    // 2. Autosave on phase transition & backup rotation
    // =========================================================================
    const echoes::sim::EntityId CarrierId = Bridge->GetArchiveCarrierId();
    if (!TestTrue(TEXT("Archive carrier entity exists"), CarrierId != 0))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    Bridge->SetScenarioPaused(false);
    AddExpectedError(
        TEXT("ECHOES_CHECKPOINT_COMPLETE"),
        EAutomationExpectedErrorFlags::Contains,
        2);
    Bridge->FailNextCheckpointWriteForTesting();
    if (!TestTrue(
        TEXT("Carrier accepts move order to recovery site"),
        Bridge->IssueCommand(
            echoes::sim::CommandType::Move,
            CarrierId,
            0,
            Bridge->SimToWorld(UEchoesSimulationSubsystem::GetArchiveRecoverySite()),
            echoes::sim::FutureWellChoice::Dormant,
            Feedback)))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    // Tick until phase transitions from RecoverArchive to DecideFutureWell
    bool bReachedNextPhase = false;
    for (int32 TickIndex = 0; TickIndex < 800; ++TickIndex)
    {
        if (Bridge->GetProloguePhase() == EEchoesProloguePhase::DecideFutureWell)
        {
            bReachedNextPhase = true;
            break;
        }
        Bridge->Tick(0.05f);
    }
    if (!TestTrue(
            TEXT("Carrier moved and Prologue reached DecideFutureWell phase"),
            bReachedNextPhase) ||
        !TestFalse(
            TEXT("Injected phase-transition autosave failure is reported"),
            Bridge->WaitForCheckpointSaves(Feedback)))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    TestTrue(TEXT("Failed phase autosave exposes its write failure"),
        Feedback.Contains(TEXT("[SAVE_WRITE_FAILED_TEST]")));
    Bridge->Tick(0.05f);
    if (!TestTrue(
        TEXT("Failed phase autosave retries in the same phase on the next fixed step"),
        Bridge->WaitForCheckpointSaves(Feedback)))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    TestEqual(
        TEXT("Autosave reason updated to PhaseTransition"),
        Bridge->GetLastAutosavedReason(),
        EEchoesAutosaveReason::PhaseTransition);

    TestEqual(
        TEXT("Autosaved phase updated to DecideFutureWell"),
        Bridge->GetLastAutosavedPhase(),
        static_cast<uint8>(EEchoesProloguePhase::DecideFutureWell));

    TestTrue(
        TEXT("Autosaved tick is greater than 0"),
        Bridge->GetLastAutosavedTick() > 0);

    const FString BackupPath = ExpectedAutosavePath + TEXT(".bak");
    if (!TestTrue(
        TEXT("Backup file exists after phase-transition autosave"),
        IFileManager::Get().FileExists(*BackupPath)))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    TArray<uint8> BackupBytes;
    if (!TestTrue(
        TEXT("Backup bytes match entry autosave bytes (rotation succeeded)"),
        FFileHelper::LoadFileToArray(BackupBytes, *BackupPath) &&
            !BackupBytes.IsEmpty() &&
            BackupBytes == EntryAutosaveBytes))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    // =========================================================================
    // 3. Interrupted session detection
    // =========================================================================
    FEchoesRecoveryCandidate Candidate;
    if (!TestTrue(
        TEXT("CheckInterruptedSessionRecovery finds the active autosave"),
        Bridge->CheckInterruptedSessionRecovery(Candidate, Feedback)))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    TestTrue(TEXT("Candidate is available"), Candidate.bAvailable);
    TestFalse(TEXT("Candidate recovered from backup is false for healthy primary"), Candidate.bRecoveredFromBackup);
    TestEqual(
        TEXT("Candidate operation is CampaignPrologue"),
        Candidate.OperationMode,
        EEchoesOperationMode::CampaignPrologue);
    TestEqual(
        TEXT("Candidate mission ID is WhatTheLedgerKeeps"),
        Candidate.MissionId,
        EEchoesCampaignMissionId::WhatTheLedgerKeeps);
    TestEqual(
        TEXT("Candidate tick matches simulation tick"),
        Candidate.SimulationTick,
        Bridge->GetSimulation()->CurrentTick());
    TestEqual(
        TEXT("Candidate state checksum matches simulation checksum"),
        Candidate.StateChecksum,
        Bridge->GetSimulation()->StateChecksum());
    TestTrue(
        TEXT("Candidate limitation notice contains honest CRC limitation statement"),
        Candidate.HonestLimitationNotice.Contains(
            TEXT("CRC confirms uncorrupted disk storage; it is not cryptographic authentication")));

    // =========================================================================
    // 4. Killed-process recovery execution
    // =========================================================================
    const uint64 PreCrashTick = Bridge->GetSimulation()->CurrentTick();
    const uint64 PreCrashChecksum = Bridge->GetSimulation()->StateChecksum();

    // Simulate process termination / abrupt stoppage
    Bridge->StopPrototypeScenario();
    TestNull(TEXT("Simulation reset after crash simulation"), Bridge->GetSimulation());
    TestFalse(TEXT("Scenario is not ready after stop"), Bridge->IsScenarioReady());

    // Recover interrupted session
    if (!TestTrue(
        TEXT("RecoverInterruptedSession succeeds"),
        Bridge->RecoverInterruptedSession(Feedback)) ||
        !TestNotNull(TEXT("Simulation restored"), Bridge->GetSimulation()))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    TestTrue(TEXT("Scenario is ready after recovery"), Bridge->IsScenarioReady());
    TestEqual(
        TEXT("Restored tick matches pre-crash tick"),
        Bridge->GetSimulation()->CurrentTick(),
        PreCrashTick);
    TestEqual(
        TEXT("Restored checksum matches pre-crash checksum"),
        Bridge->GetSimulation()->StateChecksum(),
        PreCrashChecksum);
    TestNotNull(
        TEXT("Restored carrier entity view exists"),
        Bridge->FindEntityView(CarrierId));

    // =========================================================================
    // 5. Corrupt primary fallback to backup
    // =========================================================================
    // Invalidate primary container on disk
    if (!TestTrue(
        TEXT("Corrupt primary autosave file on disk"),
        FFileHelper::SaveStringToFile(
            TEXT("garbage corrupted data"), *ExpectedAutosavePath)) ||
        !TestTrue(
            TEXT("CheckInterruptedSessionRecovery discovers backup fallback"),
            Bridge->CheckInterruptedSessionRecovery(Candidate, Feedback)))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    TestTrue(TEXT("Candidate remains available via backup"), Candidate.bAvailable);
    TestTrue(TEXT("Candidate explicitly flags recovered from backup"), Candidate.bRecoveredFromBackup);
    TestEqual(TEXT("Candidate tick is from the backup (tick 0)"), Candidate.SimulationTick, 0ULL);

    // Stop scenario and recover from backup
    Bridge->StopPrototypeScenario();
    if (!TestTrue(
        TEXT("RecoverInterruptedSession from backup succeeds"),
        Bridge->RecoverInterruptedSession(Feedback)) ||
        !TestNotNull(
            TEXT("Backup recovery restores a simulation"),
            Bridge->GetSimulation()))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    TestTrue(
        TEXT("Recovery feedback mentions prior-generation backup"),
        Feedback.Contains(TEXT("prior-generation backup")));
    TestEqual(
        TEXT("Restored simulation tick matches backup tick (0)"),
        Bridge->GetSimulation()->CurrentTick(),
        0ULL);

    // Startup recovery must discover a staged prior generation even when a
    // failed rotation left no primary or ordinary backup to enumerate.
    const FString StagedBackupPath = BackupPath + TEXT(".tmp");
    TestTrue(
        TEXT("A standalone staged prior generation can be retained"),
        FFileHelper::SaveArrayToFile(BackupBytes, *StagedBackupPath));
    TestTrue(TEXT("Primary is removed for staged-only discovery"),
        IFileManager::Get().Delete(*ExpectedAutosavePath, false, true, true));
    TestTrue(TEXT("Ordinary backup is removed for staged-only discovery"),
        IFileManager::Get().Delete(*BackupPath, false, true, true));
    Candidate = FEchoesRecoveryCandidate();
    if (!TestTrue(
            TEXT("Startup recovery discovers a standalone staged generation"),
            Bridge->CheckInterruptedSessionRecovery(Candidate, Feedback)))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    TestTrue(TEXT("Staged recovery identifies a prior generation"),
        Candidate.bRecoveredFromBackup);
    TestEqual(TEXT("Staged recovery retains the exact source path"),
        Candidate.SourcePath, StagedBackupPath);
    Bridge->StopPrototypeScenario();
    if (!TestTrue(
            TEXT("Standalone staged recovery canonicalizes to its primary path"),
            Bridge->RecoverInterruptedSession(Candidate, Feedback)) ||
        !TestNotNull(TEXT("Staged recovery restores a simulation"),
            Bridge->GetSimulation()))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    TestTrue(TEXT("Staged recovery source is disclosed"),
        Feedback.Contains(TEXT("staged prior-generation recovery")));

    // =========================================================================
    // 6. Double-corruption fail-closed safety
    // =========================================================================
    // Corrupt both primary and backup
    TestTrue(
        TEXT("Corrupt primary file on disk"),
        FFileHelper::SaveStringToFile(
            TEXT("garbage corrupt primary"), *ExpectedAutosavePath));
    TestTrue(
        TEXT("Corrupt backup file on disk"),
        FFileHelper::SaveStringToFile(TEXT("garbage corrupt backup"), *BackupPath));
    TestTrue(
        TEXT("Corrupt staged recovery file on disk"),
        FFileHelper::SaveStringToFile(
            TEXT("garbage corrupt staged recovery"), *StagedBackupPath));
    IFileManager::Get().Delete(*(ExpectedAutosavePath + TEXT(".tmp")), false, true, true);

    Candidate = FEchoesRecoveryCandidate();
    TestFalse(
        TEXT("CheckInterruptedSessionRecovery rejects double-corrupted autosaves"),
        Bridge->CheckInterruptedSessionRecovery(Candidate, Feedback));
    TestFalse(TEXT("Candidate is marked not available"), Candidate.bAvailable);

    Bridge->StopPrototypeScenario();
    TestFalse(
        TEXT("RecoverInterruptedSession fails closed when all generations are corrupted"),
        Bridge->RecoverInterruptedSession(Feedback));
    TestTrue(
        TEXT("Fail-closed feedback indicates corrupted container or no valid checkpoint"),
        Feedback.Contains(TEXT("[RECOVERY_CONTAINER_CORRUPT]")) ||
        Feedback.Contains(TEXT("[LOAD_NO_VALID_CHECKPOINT]")));
    TestNull(TEXT("Simulation remains uninitialized after fail-closed recovery"), Bridge->GetSimulation());

    // =========================================================================
    // 7. Named slots and autosave coexistence without pollution
    // =========================================================================
    using FStore = FEchoesCampaignProgressStore;
    FEchoesCampaignProgress TestProgress;
    FEchoesCampaignDecisionRecord Record;
    Record.Mission = EEchoesCampaignMissionId::WhatTheLedgerKeeps;
    Record.WellChoice = echoes::sim::FutureWellChoice::Preserve;
    Record.AvailableWellChoices = 0x07;
    Record.VerifiedFacts = 0x0F;
    Record.CompletionTick = 200;
    Record.FinalStateChecksum = 12345;
    TestProgress.AppendDecision(Record, Feedback);

    const FString SlotName = TEXT("AutosaveCoexistenceSlot");
    TestTrue(
        TEXT("Save named slot"),
        FStore::SaveSlot(SlotName, TestProgress, Feedback));

    // Re-create every logical autosave generation to prove dismissal
    // canonicalizes the selected source and removes the complete family.
    TestTrue(
        TEXT("Write dummy autosave in same directory"),
        FFileHelper::SaveArrayToFile(BackupBytes, *ExpectedAutosavePath));
    TestTrue(TEXT("Write autosave temporary generation"),
        FFileHelper::SaveArrayToFile(
            BackupBytes, *(ExpectedAutosavePath + TEXT(".tmp"))));
    TestTrue(TEXT("Write autosave backup generation"),
        FFileHelper::SaveArrayToFile(BackupBytes, *BackupPath));
    TestTrue(TEXT("Write autosave staged backup generation"),
        FFileHelper::SaveArrayToFile(BackupBytes, *StagedBackupPath));

    TArray<FStore::FSlotSummary> Slots = FStore::ListSlots();
    TestEqual(TEXT("Only named slots are listed (no autosave pollution)"), Slots.Num(), 1);
    if (Slots.Num() == 1)
    {
        TestEqual(TEXT("Slot name matches"), Slots[0].SlotName, SlotName);
    }

    // Dismiss interrupted session cleanup test
    TestTrue(
        TEXT("Check candidate finds recreated autosave"),
        Bridge->CheckInterruptedSessionRecovery(Candidate, Feedback));
    TestTrue(TEXT("Candidate available for dismissal"), Candidate.bAvailable);

    TestTrue(
        TEXT("DismissInterruptedSession cleans up candidate files"),
        Bridge->DismissInterruptedSession(Candidate, Feedback));
    TestFalse(
        TEXT("Autosave primary deleted by dismissal"),
        IFileManager::Get().FileExists(*ExpectedAutosavePath));
    TestFalse(
        TEXT("Autosave backup deleted by dismissal"),
        IFileManager::Get().FileExists(*BackupPath));
    TestFalse(
        TEXT("Autosave temporary generation deleted by dismissal"),
        IFileManager::Get().FileExists(
            *(ExpectedAutosavePath + TEXT(".tmp"))));
    TestFalse(
        TEXT("Autosave staged backup deleted by dismissal"),
        IFileManager::Get().FileExists(*StagedBackupPath));

    // Verify named slot is still intact after autosave dismissal
    Slots = FStore::ListSlots();
    TestEqual(TEXT("Named slot still exists after autosave dismissal"), Slots.Num(), 1);

    // =========================================================================
    // 8. Offline-skirmish cadence retries after a failed completion
    // =========================================================================
    Bridge->StopPrototypeScenario();
    if (!TestTrue(TEXT("Select offline skirmish for cadence coverage"),
            Bridge->SelectOperationMode(
                EEchoesOperationMode::Skirmish, Feedback)) ||
        !TestTrue(TEXT("Start offline skirmish for cadence coverage"),
            Bridge->StartPrototypeScenario()) ||
        !TestNotNull(TEXT("Cadence fixture owns a simulation"),
            Bridge->GetSimulation()))
    {
        FStore::DeleteSlot(SlotName, Feedback);
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    echoes::sim::Simulation* CadenceSimulation =
        const_cast<echoes::sim::Simulation*>(Bridge->GetSimulation());
    constexpr uint64 RequiredAutosaveCadenceTicks = 6'000;
    for (uint64 TickIndex = 0;
         TickIndex < RequiredAutosaveCadenceTicks;
         ++TickIndex)
    {
        CadenceSimulation->Step();
    }
    TestEqual(TEXT("Cadence fixture reaches exactly 6000 native ticks"),
        CadenceSimulation->CurrentTick(),
        RequiredAutosaveCadenceTicks);
    Bridge->FailNextCheckpointWriteForTesting();
    Bridge->Tick(0.05f);
    TestFalse(TEXT("Injected cadence autosave failure is reported"),
        Bridge->WaitForCheckpointSaves(Feedback));
    TestTrue(TEXT("Failed cadence autosave exposes its write failure"),
        Feedback.Contains(TEXT("[SAVE_WRITE_FAILED_TEST]")));
    const uint64 FailedCadenceTick =
        Bridge->GetCheckpointSaveStatus().SimulationTick;
    Bridge->Tick(0.05f);
    if (!TestTrue(
            TEXT("Failed cadence autosave retries on the next fixed step"),
            Bridge->WaitForCheckpointSaves(Feedback)))
    {
        FStore::DeleteSlot(SlotName, Feedback);
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    TestEqual(TEXT("Retried autosave reports cadence reason"),
        Bridge->GetLastAutosavedReason(), EEchoesAutosaveReason::TickCadence);
    TestTrue(TEXT("Cadence retry commits after the failed request tick"),
        Bridge->GetLastAutosavedTick() > FailedCadenceTick);

    // =========================================================================
    // 9. Cold offline-skirmish recovery retains its serialized setup
    // =========================================================================
    FEchoesSkirmishSetup InterruptedSetup =
        FEchoesSkirmishSetupModel::DefaultSetup();
    InterruptedSetup.Seed = 0x5C17A15EULL;
    InterruptedSetup.LocalFaction = echoes::sim::Faction::HollowChoir;
    InterruptedSetup.OpponentFaction =
        echoes::sim::Faction::KharuunAssemblies;
    InterruptedSetup.MapPreset =
        EEchoesSkirmishMapPreset::SorynConfluence;
    InterruptedSetup.AiPersonality = echoes::sim::AiPersonality::Economic;
    InterruptedSetup.ResourceLevel =
        EEchoesSkirmishResourceLevel::Abundant;
    InterruptedSetup.Difficulty = EEchoesSkirmishDifficulty::Veteran;
    InterruptedSetup.GameSpeed = EEchoesSkirmishGameSpeed::Fast;
    if (!TestTrue(
            TEXT("Interrupted match applies a nondefault skirmish setup"),
            Bridge->ApplySkirmishSetup(InterruptedSetup, Feedback)) ||
        !TestNotNull(
            TEXT("Interrupted skirmish owns an authoritative simulation"),
            Bridge->GetSimulation()))
    {
        FStore::DeleteSlot(SlotName, Feedback);
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const uint64 InterruptedTick = Bridge->GetSimulation()->CurrentTick();
    const uint64 InterruptedChecksum =
        Bridge->GetSimulation()->StateChecksum();
    if (!TestTrue(
            TEXT("Offline skirmish autosave is accepted asynchronously"),
            Bridge->AutosaveScenario(
                EEchoesAutosaveReason::TickCadence, Feedback)) ||
        !TestEqual(
            TEXT("Accepted skirmish autosave remains pending until completion"),
            Bridge->GetCheckpointSaveStatus().State,
            EEchoesCheckpointSaveState::Pending) ||
        !TestTrue(
            TEXT("Offline skirmish autosave commits successfully"),
            Bridge->WaitForCheckpointSaves(Feedback)))
    {
        FStore::DeleteSlot(SlotName, Feedback);
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const FString SkirmishAutosavePath = Bridge->GetActiveAutosavePath();
    const FString SkirmishBackupPath = SkirmishAutosavePath + TEXT(".bak");
    TArray<uint8> SkirmishAutosaveBytes;
    if (!TestTrue(
            TEXT("Committed skirmish autosave has checkpoint bytes"),
            FFileHelper::LoadFileToArray(
                SkirmishAutosaveBytes, *SkirmishAutosavePath) &&
                !SkirmishAutosaveBytes.IsEmpty()) ||
        !TestTrue(
            TEXT("Valid prior skirmish generation is retained for fallback"),
            FFileHelper::SaveArrayToFile(
                SkirmishAutosaveBytes, *SkirmishBackupPath)) ||
        !TestTrue(
            TEXT("Interrupted skirmish primary can be corrupted without a crash"),
            FFileHelper::SaveStringToFile(
                TEXT("corrupt interrupted skirmish primary"),
                *SkirmishAutosavePath)))
    {
        FStore::DeleteSlot(SlotName, Feedback);
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    Candidate = FEchoesRecoveryCandidate();
    if (!TestTrue(
            TEXT("Scanner admits the valid skirmish backup"),
            Bridge->CheckInterruptedSessionRecovery(Candidate, Feedback)))
    {
        FStore::DeleteSlot(SlotName, Feedback);
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    TestTrue(TEXT("Skirmish recovery candidate is available"),
        Candidate.bAvailable);
    TestTrue(TEXT("Skirmish candidate identifies backup recovery"),
        Candidate.bRecoveredFromBackup);
    TestEqual(TEXT("Skirmish candidate retains its operation"),
        Candidate.OperationMode, EEchoesOperationMode::Skirmish);
    TestEqual(TEXT("Skirmish candidate retains the captured tick"),
        Candidate.SimulationTick, InterruptedTick);
    TestEqual(TEXT("Skirmish candidate retains the captured checksum"),
        Candidate.StateChecksum, InterruptedChecksum);
    TestTrue(TEXT("Skirmish recovery discloses its setup binding"),
        Candidate.RecoveryStatusText.Contains(
            TEXT("serialized skirmish setup")));

    Bridge->StopPrototypeScenario();
    FTestWorldWrapper FreshSkirmishWorld;
    if (!FreshSkirmishWorld.CreateTestWorld(EWorldType::Game))
    {
        FStore::DeleteSlot(SlotName, Feedback);
        FreshSkirmishWorld.ForwardErrorMessages(this);
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    UWorld* FreshWorld = FreshSkirmishWorld.GetTestWorld();
    UEchoesSimulationSubsystem* FreshBridge = FreshWorld != nullptr
        ? FreshWorld->GetSubsystem<UEchoesSimulationSubsystem>()
        : nullptr;
    if (!TestNotNull(
            TEXT("Cold recovery world owns a simulation subsystem"),
            FreshBridge) ||
        !TestTrue(
            TEXT("Cold subsystem starts from the default skirmish setup"),
            FreshBridge->GetActiveSkirmishSetup() ==
                FEchoesSkirmishSetupModel::DefaultSetup()) ||
        !TestTrue(
            TEXT("Cold recovery loads the serialized skirmish setup from backup"),
            FreshBridge->RecoverInterruptedSession(Candidate, Feedback)) ||
        !TestNotNull(
            TEXT("Cold skirmish recovery restores a simulation"),
            FreshBridge->GetSimulation()))
    {
        FStore::DeleteSlot(SlotName, Feedback);
        if (FreshBridge != nullptr)
        {
            FreshBridge->StopPrototypeScenario();
        }
        FreshSkirmishWorld.ForwardErrorMessages(this);
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    TestTrue(TEXT("Recovered skirmish restores the complete setup"),
        FreshBridge->GetActiveSkirmishSetup() == InterruptedSetup);
    TestEqual(TEXT("Recovered skirmish restores local authority"),
        FreshBridge->GetLocalFaction(), InterruptedSetup.LocalFaction);
    TestEqual(TEXT("Recovered skirmish restores opponent authority"),
        FreshBridge->GetOpponentFaction(), InterruptedSetup.OpponentFaction);
    TestEqual(TEXT("Recovered skirmish restores the captured tick"),
        FreshBridge->GetSimulation()->CurrentTick(), InterruptedTick);
    TestEqual(TEXT("Recovered skirmish restores the captured checksum"),
        FreshBridge->GetSimulation()->StateChecksum(), InterruptedChecksum);
    if (!TestTrue(
            TEXT("Recovered skirmish can enter its ordinary terminal result"),
            FreshBridge->ConcedeOfflineMatch(Feedback)) ||
        !TestTrue(
            TEXT("Terminal skirmish checkpoint remains available to ordinary load"),
            FreshBridge->AutosaveScenario(
                EEchoesAutosaveReason::TickCadence, Feedback)) ||
        !TestTrue(
            TEXT("Terminal skirmish checkpoint commits"),
            FreshBridge->WaitForCheckpointSaves(Feedback)))
    {
        FStore::DeleteSlot(SlotName, Feedback);
        FreshBridge->StopPrototypeScenario();
        FreshSkirmishWorld.ForwardErrorMessages(this);
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    Candidate = FEchoesRecoveryCandidate();
    TestFalse(
        TEXT("Finished skirmish is not presented as an interrupted session"),
        FreshBridge->CheckInterruptedSessionRecovery(Candidate, Feedback));
    TestFalse(TEXT("Finished skirmish has no recovery candidate"),
        Candidate.bAvailable);

    // Clean up
    FStore::DeleteSlot(SlotName, Feedback);
    FreshBridge->StopPrototypeScenario();
    FreshSkirmishWorld.ForwardErrorMessages(this);
    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors() && !FreshSkirmishWorld.HasFailed() &&
        !WorldWrapper.HasFailed();
}

#endif
