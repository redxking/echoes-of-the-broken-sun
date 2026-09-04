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
    TestTrue(
        TEXT("Select Prologue mission operation mode"),
        Bridge->SelectOperationMode(EEchoesOperationMode::CampaignPrologue, Feedback));

    TestTrue(
        TEXT("Prologue starts and triggers mission-entry autosave"),
        Bridge->StartPrototypeScenario());

    const FString ExpectedAutosavePath = Bridge->GetActiveAutosavePath();
    TestFalse(TEXT("Active autosave path is non-empty"), ExpectedAutosavePath.IsEmpty());
    TestTrue(
        TEXT("Autosave file exists on disk after mission entry"),
        IFileManager::Get().FileExists(*ExpectedAutosavePath));

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
    TestTrue(
        TEXT("Autosave checkpoint file validates cleanly on disk"),
        Bridge->ValidateCheckpointFileOnDisk(ExpectedAutosavePath, 0, ValidateError));

    TArray<uint8> EntryAutosaveBytes;
    TestTrue(
        TEXT("Load entry autosave bytes for container inspection and backup comparison"),
        FFileHelper::LoadFileToArray(EntryAutosaveBytes, *ExpectedAutosavePath) && !EntryAutosaveBytes.IsEmpty());

    uint8 ContainerVersion = 0;
    EEchoesOperationMode ContainerOp = EEchoesOperationMode::Skirmish;
    echoes::sim::Faction ContainerFaction = echoes::sim::Faction::MeridianCompact;
    uint64 ContainerBranchId = 0;
    uint32 ContainerCrc = 0;
    TArray<uint8> ContainerPayload;
    FString InspectError;
    TestTrue(
        TEXT("InspectSaveContainer successfully validates container integrity and CRC-32"),
        UEchoesSimulationSubsystem::InspectSaveContainer(
            EntryAutosaveBytes,
            ContainerVersion,
            ContainerOp,
            ContainerFaction,
            ContainerBranchId,
            ContainerCrc,
            ContainerPayload,
            InspectError));
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
    TestTrue(TEXT("Archive carrier entity exists"), CarrierId != 0);

    Bridge->SetScenarioPaused(false);
    TestTrue(
        TEXT("Carrier accepts move order to recovery site"),
        Bridge->IssueCommand(
            echoes::sim::CommandType::Move,
            CarrierId,
            0,
            Bridge->SimToWorld(UEchoesSimulationSubsystem::GetArchiveRecoverySite()),
            echoes::sim::FutureWellChoice::Dormant,
            Feedback));

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
    TestTrue(TEXT("Carrier moved and Prologue reached DecideFutureWell phase"), bReachedNextPhase);

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
    TestTrue(
        TEXT("Backup file exists after phase-transition autosave"),
        IFileManager::Get().FileExists(*BackupPath));

    TArray<uint8> BackupBytes;
    TestTrue(
        TEXT("Backup bytes match entry autosave bytes (rotation succeeded)"),
        FFileHelper::LoadFileToArray(BackupBytes, *BackupPath) && BackupBytes == EntryAutosaveBytes);

    // =========================================================================
    // 3. Interrupted session detection
    // =========================================================================
    FEchoesRecoveryCandidate Candidate;
    TestTrue(
        TEXT("CheckInterruptedSessionRecovery finds the active autosave"),
        Bridge->CheckInterruptedSessionRecovery(Candidate, Feedback));

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
    TestTrue(
        TEXT("RecoverInterruptedSession succeeds"),
        Bridge->RecoverInterruptedSession(Feedback));

    TestTrue(TEXT("Scenario is ready after recovery"), Bridge->IsScenarioReady());
    TestNotNull(TEXT("Simulation restored"), Bridge->GetSimulation());
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
    TestTrue(
        TEXT("Corrupt primary autosave file on disk"),
        FFileHelper::SaveStringToFile(TEXT("garbage corrupted data"), *ExpectedAutosavePath));

    TestTrue(
        TEXT("CheckInterruptedSessionRecovery discovers backup fallback"),
        Bridge->CheckInterruptedSessionRecovery(Candidate, Feedback));

    TestTrue(TEXT("Candidate remains available via backup"), Candidate.bAvailable);
    TestTrue(TEXT("Candidate explicitly flags recovered from backup"), Candidate.bRecoveredFromBackup);
    TestEqual(TEXT("Candidate tick is from the backup (tick 0)"), Candidate.SimulationTick, 0ULL);

    // Stop scenario and recover from backup
    Bridge->StopPrototypeScenario();
    TestTrue(
        TEXT("RecoverInterruptedSession from backup succeeds"),
        Bridge->RecoverInterruptedSession(Feedback));

    TestTrue(
        TEXT("Recovery feedback mentions prior-generation backup"),
        Feedback.Contains(TEXT("prior-generation backup")));
    TestEqual(
        TEXT("Restored simulation tick matches backup tick (0)"),
        Bridge->GetSimulation()->CurrentTick(),
        0ULL);

    // =========================================================================
    // 6. Double-corruption fail-closed safety
    // =========================================================================
    // Corrupt both primary and backup
    TestTrue(
        TEXT("Corrupt backup file on disk"),
        FFileHelper::SaveStringToFile(TEXT("garbage corrupt backup"), *BackupPath));
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

    // Re-create an autosave file in the directory to test coexistence
    TestTrue(
        TEXT("Write dummy autosave in same directory"),
        FFileHelper::SaveArrayToFile(BackupBytes, *ExpectedAutosavePath));

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

    // Verify named slot is still intact after autosave dismissal
    Slots = FStore::ListSlots();
    TestEqual(TEXT("Named slot still exists after autosave dismissal"), Slots.Num(), 1);

    // Clean up
    FStore::DeleteSlot(SlotName, Feedback);
    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors() && !WorldWrapper.HasFailed();
}

#endif
