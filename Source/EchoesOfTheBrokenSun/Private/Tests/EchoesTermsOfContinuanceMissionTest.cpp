#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "EchoesCampaignProgress.h"
#include "EchoesPlayerController.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesTermsOfContinuanceMissionModel.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Tests/AutomationCommon.h"

namespace
{
struct FPreservedContinuanceFile final
{
    explicit FPreservedContinuanceFile(FString InPath)
        : Path(MoveTemp(InPath))
    {
        bExisted = IFileManager::Get().FileExists(*Path);
        if (bExisted)
        {
            FFileHelper::LoadFileToArray(Contents, *Path);
        }
    }

    ~FPreservedContinuanceFile()
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

uint8 ContinuanceChoiceMask(echoes::sim::FutureWellChoice Choice)
{
    switch (Choice)
    {
        case echoes::sim::FutureWellChoice::Harvest: return 1 << 0;
        case echoes::sim::FutureWellChoice::Preserve: return 1 << 1;
        case echoes::sim::FutureWellChoice::Reshape: return 1 << 2;
        default: return 0;
    }
}

FEchoesCampaignDecisionRecord MakeContinuanceRecord(
    EEchoesCampaignMissionId Mission,
    echoes::sim::FutureWellChoice Choice)
{
    FEchoesCampaignDecisionRecord Record;
    Record.Mission = Mission;
    Record.WellChoice = Choice;
    Record.AvailableWellChoices =
        Mission == EEchoesCampaignMissionId::WhatTheLedgerKeeps
            ? 0x07
            : ContinuanceChoiceMask(Choice);
    Record.SimulationSnapshotVersion = echoes::sim::kSnapshotVersion;
    Record.CompletionTick = 100 + static_cast<uint8>(Mission) * 300;
    Record.FinalStateChecksum =
        0x7A11A1ULL + static_cast<uint8>(Mission);
    switch (Mission)
    {
        case EEchoesCampaignMissionId::WhatTheLedgerKeeps:
            Record.VerifiedFacts =
                static_cast<uint8>(EEchoesCampaignDecisionFact::ArchiveRecovered) |
                static_cast<uint8>(EEchoesCampaignDecisionFact::CarrierEvacuated) |
                static_cast<uint8>(EEchoesCampaignDecisionFact::LocalCoreSurvived) |
                static_cast<uint8>(EEchoesCampaignDecisionFact::FutureWellControlled);
            break;
        case EEchoesCampaignMissionId::SevenAccountsOfRain:
            Record.VerifiedFacts =
                static_cast<uint8>(EEchoesSevenAccountsCompletionFact::WaystoneRootedAtAnchor) |
                static_cast<uint8>(EEchoesSevenAccountsCompletionFact::MemoryBearerArrived) |
                static_cast<uint8>(EEchoesSevenAccountsCompletionFact::LocalCoreSurvived) |
                static_cast<uint8>(EEchoesSevenAccountsCompletionFact::PriorDecisionConsumed);
            break;
        case EEchoesCampaignMissionId::ACityOnReserve:
            Record.VerifiedFacts =
                static_cast<uint8>(EEchoesCityReserveCompletionFact::LifeSupportPowered) |
                static_cast<uint8>(EEchoesCityReserveCompletionFact::TransitPowered) |
                static_cast<uint8>(EEchoesCityReserveCompletionFact::ArchivePowered) |
                static_cast<uint8>(EEchoesCityReserveCompletionFact::LocalCoreSurvived) |
                static_cast<uint8>(EEchoesCityReserveCompletionFact::PriorLedgerConsumed);
            break;
        case EEchoesCampaignMissionId::TheUnburiedRoad:
            Record.VerifiedFacts =
                static_cast<uint8>(EEchoesUnburiedRoadCompletionFact::WaystoneRootedAtRoadhead) |
                static_cast<uint8>(EEchoesUnburiedRoadCompletionFact::ListeningSpineRaised) |
                static_cast<uint8>(EEchoesUnburiedRoadCompletionFact::MemoryShardRecovered) |
                static_cast<uint8>(EEchoesUnburiedRoadCompletionFact::LocalCoreSurvived) |
                static_cast<uint8>(EEchoesUnburiedRoadCompletionFact::PriorLedgerConsumed);
            break;
        case EEchoesCampaignMissionId::TermsOfContinuance:
            Record.VerifiedFacts =
                static_cast<uint8>(EEchoesTermsOfContinuanceCompletionFact::MeridianRelaySynchronized) |
                static_cast<uint8>(EEchoesTermsOfContinuanceCompletionFact::KharuunSpineSynchronized) |
                static_cast<uint8>(EEchoesTermsOfContinuanceCompletionFact::ContinuanceWindowHeld) |
                static_cast<uint8>(EEchoesTermsOfContinuanceCompletionFact::BothWitnessesExtracted) |
                static_cast<uint8>(EEchoesTermsOfContinuanceCompletionFact::LocalCoreSurvived) |
                static_cast<uint8>(EEchoesTermsOfContinuanceCompletionFact::PriorLedgerConsumed);
            break;
    }
    return Record;
}

FString ContinuanceQuickSavePath(
    const FEchoesCampaignProgress& Progress)
{
    TArray<uint8> LedgerBytes;
    FString Error;
    if (!FEchoesCampaignProgressStore::Encode(
            Progress,
            LedgerBytes,
            Error) ||
        LedgerBytes.IsEmpty())
    {
        return {};
    }
    const uint32 LedgerFingerprint =
        FCrc::MemCrc32(
            LedgerBytes.GetData(),
            LedgerBytes.Num() - static_cast<int32>(sizeof(uint32)));
    return FPaths::Combine(
        FEchoesCampaignProgressStore::GetSaveGameDirectory(),
        FString::Printf(
            TEXT("EchoesQuickSaveTermsOfContinuance-%08X.bin"),
            LedgerFingerprint));
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesTermsOfContinuanceMissionTest,
    "Echoes.Runtime.Campaign.TermsOfContinuance",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesTermsOfContinuanceMissionTest::RunTest(
    const FString& Parameters)
{
    (void)Parameters;

    FEchoesScopedTestSaveEnvironment TestSaveEnvironment(*this);
    if (!TestSaveEnvironment.IsReady())
    {
        return false;
    }

    const FEchoesTermsOfContinuancePlan PreservePlan =
        FEchoesTermsOfContinuanceMissionModel::PlanForChoice(
            echoes::sim::FutureWellChoice::Preserve);
    TestTrue(TEXT("Preserve selects the central witness clause"),
             PreservePlan.MeridianRelaySite ==
                     echoes::sim::Vec2::FromTiles(32, 27) &&
                 PreservePlan.KharuunSpineSite ==
                     echoes::sim::Vec2::FromTiles(32, 39) &&
                 PreservePlan.WitnessExtractionSite ==
                     echoes::sim::Vec2::FromTiles(32, 47));
    TestTrue(TEXT("Harvest selects the western iron clause"),
             FEchoesTermsOfContinuanceMissionModel::PlanForChoice(
                 echoes::sim::FutureWellChoice::Harvest)
                     .MeridianRelaySite ==
                 echoes::sim::Vec2::FromTiles(14, 27));
    TestTrue(TEXT("Reshape selects the eastern folded clause"),
             FEchoesTermsOfContinuanceMissionModel::PlanForChoice(
                 echoes::sim::FutureWellChoice::Reshape)
                     .MeridianRelaySite ==
                 echoes::sim::Vec2::FromTiles(50, 27));

    FEchoesTermsOfContinuanceMissionFacts Facts;
    TestTrue(TEXT("Inactive facts stay outside mission five"),
             FEchoesTermsOfContinuanceMissionModel::DeterminePhase(Facts) ==
                 EEchoesTermsOfContinuancePhase::Inactive);
    Facts.bOperationActive = true;
    Facts.bLocalCoreIntact = true;
    Facts.bMeridianRelayIntact = true;
    Facts.bKharuunSpineIntact = true;
    Facts.bMeridianWitnessIntact = true;
    Facts.bKharuunWitnessIntact = true;
    Facts.bSkirmishStillOngoing = true;
    TestTrue(TEXT("A live ceasefire begins with network synchronization"),
             FEchoesTermsOfContinuanceMissionModel::DeterminePhase(Facts) ==
                 EEchoesTermsOfContinuancePhase::SynchronizeNetworks);
    Facts.bMeridianRelaySynchronized = true;
    Facts.bKharuunSpineSynchronized = true;
    TestTrue(TEXT("Synchronized networks open the continuance hold"),
             FEchoesTermsOfContinuanceMissionModel::DeterminePhase(Facts) ==
                 EEchoesTermsOfContinuancePhase::HoldContinuanceWindow);
    Facts.bContinuanceWindowHeld = true;
    TestTrue(TEXT("A held window opens witness extraction"),
             FEchoesTermsOfContinuanceMissionModel::DeterminePhase(Facts) ==
                 EEchoesTermsOfContinuancePhase::ExtractWitnesses);
    Facts.bMeridianWitnessExtracted = true;
    Facts.bKharuunWitnessExtracted = true;
    TestTrue(TEXT("Both extracted witnesses complete the mission"),
             FEchoesTermsOfContinuanceMissionModel::DeterminePhase(Facts) ==
                 EEchoesTermsOfContinuancePhase::Complete);
    Facts.bKharuunWitnessIntact = false;
    TestTrue(TEXT("Losing either witness fails the mission"),
             FEchoesTermsOfContinuanceMissionModel::DeterminePhase(Facts) ==
                 EEchoesTermsOfContinuancePhase::Failed);
    Facts.bKharuunWitnessIntact = true;
    Facts.bSkirmishStillOngoing = false;
    TestTrue(TEXT("Destroying the opposing Core cannot substitute for continuance"),
             FEchoesTermsOfContinuanceMissionModel::DeterminePhase(Facts) ==
                 EEchoesTermsOfContinuancePhase::Failed);
    Facts.bSkirmishStillOngoing = true;
    Facts.bContinuanceWindowCompromised = true;
    TestTrue(TEXT("A broken synchronized window fails the accord"),
             FEchoesTermsOfContinuanceMissionModel::DeterminePhase(Facts) ==
                 EEchoesTermsOfContinuancePhase::Failed);
    Facts.bContinuanceWindowCompromised = false;
    Facts.bWitnessExtractionStartedEarly = true;
    TestTrue(TEXT("Starting extraction before the window closes fails the accord"),
             FEchoesTermsOfContinuanceMissionModel::DeterminePhase(Facts) ==
                 EEchoesTermsOfContinuancePhase::Failed);

    const FString CampaignPath =
        FEchoesCampaignProgressStore::GetDefaultPath();
    FPreservedContinuanceFile PreservedPrimary(CampaignPath);
    FPreservedContinuanceFile PreservedBackup(CampaignPath + TEXT(".bak"));
    FPreservedContinuanceFile PreservedTemporary(CampaignPath + TEXT(".tmp"));
    IFileManager::Get().Delete(*CampaignPath, false, true, true);
    IFileManager::Get().Delete(*(CampaignPath + TEXT(".bak")), false, true, true);
    IFileManager::Get().Delete(*(CampaignPath + TEXT(".tmp")), false, true, true);

    FString Feedback;
    FEchoesCampaignProgress LockedProgress;
    for (const EEchoesCampaignMissionId Mission : {
             EEchoesCampaignMissionId::WhatTheLedgerKeeps,
             EEchoesCampaignMissionId::SevenAccountsOfRain,
             EEchoesCampaignMissionId::ACityOnReserve})
    {
        TestTrue(TEXT("The fixture accepts a consistent prior record"),
                 LockedProgress.AppendDecision(
                     MakeContinuanceRecord(
                         Mission,
                         echoes::sim::FutureWellChoice::Preserve),
                     Feedback) == EEchoesCampaignCommitStatus::Added);
    }
    TestTrue(TEXT("The three-record locked ledger is stored"),
             FEchoesCampaignProgressStore::SaveAtomic(
                 CampaignPath,
                 LockedProgress,
                 Feedback));
    {
        FTestWorldWrapper LockedWorld;
        if (!LockedWorld.CreateTestWorld(EWorldType::Game))
        {
            LockedWorld.ForwardErrorMessages(this);
            AddError(TEXT("Could not create the locked mission-five world."));
            return false;
        }
        UEchoesSimulationSubsystem* LockedBridge =
            LockedWorld.GetTestWorld()->GetSubsystem<
                UEchoesSimulationSubsystem>();
        TestFalse(TEXT("Mission five rejects a three-record ledger"),
                  LockedBridge != nullptr &&
                      LockedBridge->SelectOperationMode(
                          EEchoesOperationMode::CampaignTermsOfContinuance,
                          Feedback));
        TestTrue(TEXT("The locked response names mission four"),
                 Feedback.Contains(TEXT("The Unburied Road")));
        LockedWorld.ForwardErrorMessages(this);
    }

    FEchoesCampaignProgress MismatchProgress = LockedProgress;
    TestTrue(TEXT("The mismatch fixture accepts an individually valid mission-four record"),
             MismatchProgress.AppendDecision(
                 MakeContinuanceRecord(
                     EEchoesCampaignMissionId::TheUnburiedRoad,
                     echoes::sim::FutureWellChoice::Harvest),
                 Feedback) == EEchoesCampaignCommitStatus::Added);
    TestTrue(TEXT("The mismatched four-record ledger is stored"),
             FEchoesCampaignProgressStore::SaveAtomic(
                 CampaignPath,
                 MismatchProgress,
                 Feedback));
    {
        FTestWorldWrapper MismatchWorld;
        if (!MismatchWorld.CreateTestWorld(EWorldType::Game))
        {
            MismatchWorld.ForwardErrorMessages(this);
            AddError(TEXT("Could not create the mismatched mission-five world."));
            return false;
        }
        UEchoesSimulationSubsystem* MismatchBridge =
            MismatchWorld.GetTestWorld()->GetSubsystem<
                UEchoesSimulationSubsystem>();
        TestFalse(TEXT("Mission five rejects inconsistent prior choices"),
                  MismatchBridge != nullptr &&
                      MismatchBridge->IsTermsOfContinuanceUnlocked());
        TestFalse(TEXT("The inconsistent ledger cannot select mission five"),
                  MismatchBridge != nullptr &&
                      MismatchBridge->SelectOperationMode(
                          EEchoesOperationMode::CampaignTermsOfContinuance,
                          Feedback));
        MismatchWorld.ForwardErrorMessages(this);
    }

    FEchoesCampaignProgress SeedProgress = LockedProgress;
    TestTrue(TEXT("The fixture accepts the consistent mission-four record"),
             SeedProgress.AppendDecision(
                 MakeContinuanceRecord(
                     EEchoesCampaignMissionId::TheUnburiedRoad,
                     echoes::sim::FutureWellChoice::Preserve),
                 Feedback) == EEchoesCampaignCommitStatus::Added);

    FEchoesCampaignProgress AlternateProgress = SeedProgress;
    AlternateProgress.Decisions[0].CompletionTick += 1;
    TestTrue(
        TEXT("The exact-ledger variant retains the same choices and mission composition"),
        AlternateProgress.Decisions.Num() == SeedProgress.Decisions.Num() &&
            AlternateProgress.Decisions[0].WellChoice ==
                SeedProgress.Decisions[0].WellChoice &&
            AlternateProgress.Decisions[1].WellChoice ==
                SeedProgress.Decisions[1].WellChoice &&
            AlternateProgress.Decisions[2].WellChoice ==
                SeedProgress.Decisions[2].WellChoice &&
            AlternateProgress.Decisions[3].WellChoice ==
                SeedProgress.Decisions[3].WellChoice &&
            AlternateProgress.Decisions != SeedProgress.Decisions);
    const FString QuickSavePath =
        ContinuanceQuickSavePath(SeedProgress);
    const FString AlternateQuickSavePath =
        ContinuanceQuickSavePath(AlternateProgress);
    TestTrue(TEXT("Mission-five save namespaces bind to the exact prerequisite ledger"),
             !QuickSavePath.IsEmpty() &&
                 !AlternateQuickSavePath.IsEmpty() &&
                 QuickSavePath != AlternateQuickSavePath);
    FPreservedContinuanceFile PreservedQuickSave(QuickSavePath);
    FPreservedContinuanceFile PreservedQuickSaveBackup(
        QuickSavePath + TEXT(".bak"));
    FPreservedContinuanceFile PreservedQuickSaveStagedBackup(
        QuickSavePath + TEXT(".bak.tmp"));
    FPreservedContinuanceFile PreservedQuickSaveTemporary(
        QuickSavePath + TEXT(".tmp"));
    FPreservedContinuanceFile PreservedAlternateQuickSave(
        AlternateQuickSavePath);
    FPreservedContinuanceFile PreservedAlternateQuickSaveBackup(
        AlternateQuickSavePath + TEXT(".bak"));
    FPreservedContinuanceFile PreservedAlternateQuickSaveStagedBackup(
        AlternateQuickSavePath + TEXT(".bak.tmp"));
    FPreservedContinuanceFile PreservedAlternateQuickSaveTemporary(
        AlternateQuickSavePath + TEXT(".tmp"));
    for (const FString& Path : {
             QuickSavePath,
             QuickSavePath + TEXT(".bak"),
             QuickSavePath + TEXT(".bak.tmp"),
             QuickSavePath + TEXT(".tmp"),
             AlternateQuickSavePath,
             AlternateQuickSavePath + TEXT(".bak"),
             AlternateQuickSavePath + TEXT(".bak.tmp"),
             AlternateQuickSavePath + TEXT(".tmp")})
    {
        IFileManager::Get().Delete(*Path, false, true, true);
    }
    TestTrue(TEXT("The four-record ledger is stored"),
             FEchoesCampaignProgressStore::SaveAtomic(
                 CampaignPath,
                 SeedProgress,
                 Feedback));

    {
        FTestWorldWrapper DeadlineWorld;
        if (!DeadlineWorld.CreateTestWorld(EWorldType::Game))
        {
            DeadlineWorld.ForwardErrorMessages(this);
            AddError(TEXT("Could not create the synchronization-deadline world."));
            return false;
        }
        UEchoesSimulationSubsystem* DeadlineBridge =
            DeadlineWorld.GetTestWorld()->GetSubsystem<
                UEchoesSimulationSubsystem>();
        if (!TestNotNull(TEXT("The deadline world owns a simulation subsystem"),
                         DeadlineBridge) ||
            !TestTrue(TEXT("The deadline world selects mission five"),
                      DeadlineBridge->SelectOperationMode(
                          EEchoesOperationMode::CampaignTermsOfContinuance,
                          Feedback)) ||
            !TestTrue(TEXT("The deadline world starts mission five"),
                      DeadlineBridge->StartPrototypeScenario()))
        {
            DeadlineWorld.ForwardErrorMessages(this);
            return false;
        }
        DeadlineBridge->SetScenarioPaused(false);
        for (int32 TickIndex = 0;
             TickIndex < 360 &&
             DeadlineBridge->GetTermsOfContinuancePhase() !=
                 EEchoesTermsOfContinuancePhase::Failed;
             ++TickIndex)
        {
            DeadlineBridge->Tick(0.05f);
        }
        TestTrue(TEXT("Missing the synchronization deadline fails instead of skipping the hold"),
                 DeadlineBridge->GetSimulation()->CurrentTick() >=
                         PreservePlan.ContinuanceWindowStartTick &&
                     DeadlineBridge->GetTermsOfContinuancePhase() ==
                         EEchoesTermsOfContinuancePhase::Failed);
        DeadlineBridge->StopPrototypeScenario();
        DeadlineWorld.ForwardErrorMessages(this);
    }

    {
        FTestWorldWrapper EarlyExtractionWorld;
        if (!EarlyExtractionWorld.CreateTestWorld(EWorldType::Game))
        {
            EarlyExtractionWorld.ForwardErrorMessages(this);
            AddError(TEXT("Could not create the early-extraction world."));
            return false;
        }
        UEchoesSimulationSubsystem* EarlyBridge =
            EarlyExtractionWorld.GetTestWorld()->GetSubsystem<
                UEchoesSimulationSubsystem>();
        if (!TestNotNull(TEXT("The early-extraction world owns a simulation subsystem"),
                         EarlyBridge) ||
            !TestTrue(TEXT("The early-extraction world selects mission five"),
                      EarlyBridge->SelectOperationMode(
                          EEchoesOperationMode::CampaignTermsOfContinuance,
                          Feedback)) ||
            !TestTrue(TEXT("The early-extraction world starts mission five"),
                      EarlyBridge->StartPrototypeScenario()))
        {
            EarlyExtractionWorld.ForwardErrorMessages(this);
            return false;
        }
        const FEchoesObjectiveSnapshot EarlySnapshot =
            EarlyBridge->GetLocalObjectiveSnapshot();
        TestTrue(TEXT("An early witness accepts the attempted extraction order"),
                 EarlyBridge->IssueCommand(
                     echoes::sim::CommandType::Move,
                     EarlySnapshot.MeridianContinuanceWitnessId,
                     0,
                     EarlyBridge->SimToWorld(
                         PreservePlan.WitnessExtractionSite),
                     echoes::sim::FutureWellChoice::Dormant,
                     Feedback));
        EarlyBridge->SetScenarioPaused(false);
        for (int32 TickIndex = 0;
             TickIndex < 20 &&
             EarlyBridge->GetTermsOfContinuancePhase() !=
                 EEchoesTermsOfContinuancePhase::Failed;
             ++TickIndex)
        {
            EarlyBridge->Tick(0.05f);
        }
        TestTrue(TEXT("Extraction ordered before the fixed window closes fails immediately"),
                 EarlyBridge->GetSimulation()->CurrentTick() <
                         PreservePlan.ContinuanceWindowEndTick &&
                     EarlyBridge->GetTermsOfContinuancePhase() ==
                         EEchoesTermsOfContinuancePhase::Failed);
        EarlyBridge->StopPrototypeScenario();
        EarlyExtractionWorld.ForwardErrorMessages(this);
    }

    {
        FTestWorldWrapper InterruptedWorld;
        if (!InterruptedWorld.CreateTestWorld(EWorldType::Game))
        {
            InterruptedWorld.ForwardErrorMessages(this);
            AddError(TEXT("Could not create the interrupted-window world."));
            return false;
        }
        UEchoesSimulationSubsystem* InterruptedBridge =
            InterruptedWorld.GetTestWorld()->GetSubsystem<
                UEchoesSimulationSubsystem>();
        if (!TestNotNull(TEXT("The interrupted-window world owns a simulation subsystem"),
                         InterruptedBridge) ||
            !TestTrue(TEXT("The interrupted-window world selects mission five"),
                      InterruptedBridge->SelectOperationMode(
                          EEchoesOperationMode::CampaignTermsOfContinuance,
                          Feedback)) ||
            !TestTrue(TEXT("The interrupted-window world starts mission five"),
                      InterruptedBridge->StartPrototypeScenario()))
        {
            InterruptedWorld.ForwardErrorMessages(this);
            return false;
        }
        echoes::sim::EntityId InterruptedWorkerId = 0;
        for (const echoes::sim::Entity& Entity :
             InterruptedBridge->GetSimulation()->Entities())
        {
            if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
                Entity.type == echoes::sim::EntityType::Worker)
            {
                InterruptedWorkerId = Entity.id;
                break;
            }
        }
        InterruptedBridge->SetScenarioPaused(false);
        InterruptedBridge->Tick(0.05f);
        TestTrue(TEXT("The interrupted-window worker accepts the missing link"),
                 InterruptedWorkerId != 0 &&
                     InterruptedBridge->IssueBuildCommand(
                         InterruptedWorkerId,
                         echoes::sim::EntityType::Dropoff,
                         InterruptedBridge->SimToWorld(
                             echoes::sim::Vec2::FromTiles(29, 28)),
                         Feedback));
        for (int32 TickIndex = 0;
             TickIndex < 700 &&
             (InterruptedBridge->GetTermsOfContinuancePhase() !=
                  EEchoesTermsOfContinuancePhase::HoldContinuanceWindow ||
              InterruptedBridge->GetSimulation()->CurrentTick() <
                  PreservePlan.ContinuanceWindowStartTick + 20);
             ++TickIndex)
        {
            InterruptedBridge->Tick(0.05f);
        }
        TestTrue(TEXT("The interrupted fixture reaches the active synchronized window"),
                 InterruptedBridge->GetTermsOfContinuancePhase() ==
                         EEchoesTermsOfContinuancePhase::HoldContinuanceWindow &&
                     InterruptedBridge->GetSimulation()->CurrentTick() >=
                         PreservePlan.ContinuanceWindowStartTick + 20);
        echoes::sim::Simulation* InterruptedSimulation =
            const_cast<echoes::sim::Simulation*>(
                InterruptedBridge->GetSimulation());
        const FEchoesObjectiveSnapshot InterruptedSnapshot =
            InterruptedBridge->GetLocalObjectiveSnapshot();
        uint64 OpponentSequence =
            InterruptedSimulation->NextCommandSequence(
                UEchoesSimulationSubsystem::OpponentPlayerId)
                .value_or(0);
        int32 QueuedAttackers = 0;
        for (int32 Index = 0; Index < 20; ++Index)
        {
            const echoes::sim::EntityId AttackerId =
                InterruptedSimulation->SpawnEntity(
                    UEchoesSimulationSubsystem::OpponentPlayerId,
                    echoes::sim::Faction::KharuunAssemblies,
                    echoes::sim::EntityType::HeavyUnit,
                    echoes::sim::Vec2::FromTiles(
                        35 + Index % 5,
                        36 + Index / 5));
            if (AttackerId == 0)
            {
                continue;
            }
            echoes::sim::Command Attack;
            Attack.executeTick = InterruptedSimulation->CurrentTick() + 1;
            Attack.player = UEchoesSimulationSubsystem::OpponentPlayerId;
            Attack.sequence = OpponentSequence++;
            Attack.type = echoes::sim::CommandType::Attack;
            Attack.actor = AttackerId;
            Attack.target = InterruptedSnapshot.KharuunContinuanceSpineId;
            if (InterruptedSimulation->QueueCommand(Attack))
            {
                ++QueuedAttackers;
            }
        }
        TestTrue(TEXT("The interrupted fixture queues bounded hostile pressure"),
                 QueuedAttackers > 0);
        for (int32 TickIndex = 0;
             TickIndex < 500 &&
             InterruptedBridge->GetTermsOfContinuancePhase() !=
                 EEchoesTermsOfContinuancePhase::Failed;
             ++TickIndex)
        {
            InterruptedSimulation->Step();
        }
        TestTrue(TEXT("Losing synchronization during the fixed window fails before extraction"),
                 InterruptedBridge->GetSimulation()->CurrentTick() <
                         PreservePlan.ContinuanceWindowEndTick &&
                     InterruptedBridge->GetTermsOfContinuancePhase() ==
                         EEchoesTermsOfContinuancePhase::Failed);
        InterruptedBridge->StopPrototypeScenario();
        InterruptedWorld.ForwardErrorMessages(this);
    }

    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create Terms of Continuance test world."));
        return false;
    }
    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge =
        World != nullptr
            ? World->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (!TestNotNull(TEXT("Mission world owns the simulation subsystem"), Bridge) ||
        !TestTrue(TEXT("Mission five is unlocked by four consistent records"),
                  Bridge != nullptr && Bridge->IsTermsOfContinuanceUnlocked()) ||
        !TestTrue(TEXT("Mission five operation can be selected"),
                  Bridge->SelectOperationMode(
                      EEchoesOperationMode::CampaignTermsOfContinuance,
                      Feedback)) ||
        !TestTrue(TEXT("Mission five scenario starts"),
                  Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    TestTrue(TEXT("The joint operation runs under Meridian command authority"),
             Bridge->GetLocalFaction() ==
                 echoes::sim::Faction::MeridianCompact);
    TestTrue(TEXT("Preserve keeps only the central Glass Scar crossing open"),
             Bridge->GetSimulation()->IsPositionPassable(
                 echoes::sim::Vec2::FromTiles(32, 32)) &&
                 !Bridge->GetSimulation()->IsPositionPassable(
                     echoes::sim::Vec2::FromTiles(14, 32)) &&
                 !Bridge->GetSimulation()->IsPositionPassable(
                     echoes::sim::Vec2::FromTiles(50, 32)));
    const FEchoesObjectiveSnapshot Snapshot =
        Bridge->GetLocalObjectiveSnapshot();
    TestTrue(TEXT("Both network interfaces and witnesses are authoritative"),
             Snapshot.MeridianContinuanceRelayId != 0 &&
                 Snapshot.KharuunContinuanceSpineId != 0 &&
                 Snapshot.MeridianContinuanceWitnessId != 0 &&
                 Snapshot.KharuunContinuanceWitnessId != 0);
    bool bAllTreatyEntitiesAreMeridianProxies = true;
    for (const echoes::sim::EntityId TreatyEntityId : {
             Snapshot.MeridianContinuanceRelayId,
             Snapshot.KharuunContinuanceSpineId,
             Snapshot.MeridianContinuanceWitnessId,
             Snapshot.KharuunContinuanceWitnessId})
    {
        const echoes::sim::Entity* TreatyEntity =
            Bridge->FindEntity(TreatyEntityId);
        bAllTreatyEntitiesAreMeridianProxies &=
            TreatyEntity != nullptr &&
            TreatyEntity->owner ==
                UEchoesSimulationSubsystem::LocalPlayerId &&
            TreatyEntity->faction ==
                echoes::sim::Faction::MeridianCompact;
    }
    TestTrue(TEXT("Treaty entities are explicit Meridian-authoritative proxies"),
             bAllTreatyEntitiesAreMeridianProxies);
    TestTrue(TEXT("The live operation begins by synchronizing both networks"),
             Bridge->GetTermsOfContinuancePhase() ==
                 EEchoesTermsOfContinuancePhase::SynchronizeNetworks);
    TestTrue(TEXT("Mission five uses an isolated quick-save slot"),
             Bridge->QuickSaveScenario(Feedback) &&
                 IFileManager::Get().FileExists(*QuickSavePath));
    TestTrue(TEXT("The synchronization phase reconstructs after quick load"),
             Bridge->QuickLoadScenario(Feedback) &&
                 Bridge->GetTermsOfContinuancePhase() ==
                     EEchoesTermsOfContinuancePhase::SynchronizeNetworks);
    TestTrue(TEXT("The same-choice exact-ledger variant is stored for binding validation"),
             FEchoesCampaignProgressStore::SaveAtomic(
                 CampaignPath,
                 AlternateProgress,
                 Feedback));
    TArray<uint8> PreserveCheckpointBytes;
    TestTrue(TEXT("The original checkpoint can seed an exact-ledger mismatch probe"),
             FFileHelper::LoadFileToArray(
                 PreserveCheckpointBytes,
                 *QuickSavePath) &&
                 FFileHelper::SaveArrayToFile(
                     PreserveCheckpointBytes,
                     *AlternateQuickSavePath) &&
                 FFileHelper::SaveArrayToFile(
                     PreserveCheckpointBytes,
                     *(AlternateQuickSavePath + TEXT(".bak"))));
    {
        FTestWorldWrapper AlternateWorld;
        if (!AlternateWorld.CreateTestWorld(EWorldType::Game))
        {
            AlternateWorld.ForwardErrorMessages(this);
            AddError(TEXT("Could not create the exact-ledger variant mission-five world."));
            return false;
        }
        UEchoesSimulationSubsystem* AlternateBridge =
            AlternateWorld.GetTestWorld()->GetSubsystem<
                UEchoesSimulationSubsystem>();
        if (!TestNotNull(TEXT("The alternate world owns a simulation subsystem"),
                         AlternateBridge) ||
            !TestTrue(TEXT("The exact-ledger variant can select mission five"),
                      AlternateBridge->SelectOperationMode(
                          EEchoesOperationMode::CampaignTermsOfContinuance,
                          Feedback)) ||
            !TestTrue(TEXT("The exact-ledger variant can start mission five"),
                      AlternateBridge->StartPrototypeScenario()))
        {
            AlternateWorld.ForwardErrorMessages(this);
            return false;
        }
        TestFalse(TEXT("A checkpoint cannot load under a different exact prerequisite ledger"),
                  AlternateBridge->QuickLoadScenario(Feedback));
        TestTrue(TEXT("Exact-ledger mismatch is identified after coarse mission fields coincide"),
                 Feedback.Contains(TEXT("LOAD_LEDGER_BRANCH_MISMATCH")));
        AlternateBridge->StopPrototypeScenario();
        AlternateWorld.ForwardErrorMessages(this);
    }
    TestTrue(TEXT("The Preserve campaign ledger is restored after isolation testing"),
             FEchoesCampaignProgressStore::SaveAtomic(
                 CampaignPath,
                 SeedProgress,
                 Feedback));

    echoes::sim::EntityId WorkerId = 0;
    for (const echoes::sim::Entity& Entity :
         Bridge->GetSimulation()->Entities())
    {
        if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            Entity.type == echoes::sim::EntityType::Worker)
        {
            WorkerId = Entity.id;
            break;
        }
    }
    if (!TestTrue(TEXT("A treaty-grid construction worker is available"),
                  WorkerId != 0))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    const auto TickUntil = [Bridge](const TFunction<bool()>& Predicate,
                                    int32 MaximumTicks)
    {
        for (int32 TickIndex = 0; TickIndex < MaximumTicks; ++TickIndex)
        {
            if (Predicate())
            {
                return true;
            }
            Bridge->Tick(0.05f);
        }
        return Predicate();
    };
    Bridge->SetScenarioPaused(false);
    Bridge->Tick(0.05f);
    const FEchoesObjectiveSnapshot UnsynchronizedSnapshot =
        Bridge->GetLocalObjectiveSnapshot();
    TestTrue(TEXT("The authored grid begins with exactly one synchronized interface"),
             UnsynchronizedSnapshot.bMeridianRelaySynchronized &&
                 !UnsynchronizedSnapshot.bKharuunSpineSynchronized &&
                 Bridge->GetTermsOfContinuancePhase() ==
                     EEchoesTermsOfContinuancePhase::SynchronizeNetworks);
    TestTrue(TEXT("The worker accepts the missing treaty Power Link"),
             Bridge->IssueBuildCommand(
                 WorkerId,
                 echoes::sim::EntityType::Dropoff,
                 Bridge->SimToWorld(
                     echoes::sim::Vec2::FromTiles(29, 28)),
                 Feedback));
    TestTrue(TEXT("Ordinary construction synchronizes both interfaces"),
             TickUntil(
                 [Bridge]()
                 {
                     return Bridge->GetTermsOfContinuancePhase() ==
                         EEchoesTermsOfContinuancePhase::HoldContinuanceWindow;
                 },
                 700));
    TestTrue(TEXT("The synchronized interfaces hold through the fixed window"),
             TickUntil(
                 [Bridge]()
                 {
                     return Bridge->GetTermsOfContinuancePhase() ==
                         EEchoesTermsOfContinuancePhase::ExtractWitnesses;
                 },
                 1000));
    const FEchoesObjectiveSnapshot ExtractionSnapshot =
        Bridge->GetLocalObjectiveSnapshot();
    const FVector ExtractionWorld =
        Bridge->SimToWorld(PreservePlan.WitnessExtractionSite);
    TestTrue(TEXT("The Meridian witness accepts extraction movement"),
             Bridge->IssueCommand(
                 echoes::sim::CommandType::Move,
                 ExtractionSnapshot.MeridianContinuanceWitnessId,
                 0,
                 ExtractionWorld,
                 echoes::sim::FutureWellChoice::Dormant,
                 Feedback));
    TestTrue(TEXT("The Kharuun witness proxy accepts extraction movement"),
             Bridge->IssueCommand(
                 echoes::sim::CommandType::Move,
                 ExtractionSnapshot.KharuunContinuanceWitnessId,
                 0,
                 ExtractionWorld,
                 echoes::sim::FutureWellChoice::Dormant,
                 Feedback));
    TestTrue(TEXT("Ordinary movement completes the joint extraction"),
             TickUntil(
                 [Bridge]()
                 {
                     return Bridge->GetTermsOfContinuancePhase() ==
                         EEchoesTermsOfContinuancePhase::Complete;
                 },
                 1000));
    const FEchoesCampaignDecisionRecord* MissionRecord =
        Bridge->GetCampaignProgress().FindDecision(
            EEchoesCampaignMissionId::TermsOfContinuance);
    TestTrue(TEXT("The tick path appends mission five to the ledger"),
             MissionRecord != nullptr &&
                 MissionRecord->WellChoice ==
                     echoes::sim::FutureWellChoice::Preserve &&
                 MissionRecord->AvailableWellChoices == (1 << 1));
    FEchoesCampaignProgress Reloaded;
    TestTrue(TEXT("The five-record campaign reloads transactionally"),
             FEchoesCampaignProgressStore::LoadWithBackup(
                 CampaignPath,
                 Reloaded,
                 Feedback) &&
                 Reloaded.Decisions.Num() == 5);
    const FString ReplayQuickSavePath =
        ContinuanceQuickSavePath(Reloaded);
    FPreservedContinuanceFile PreservedReplayQuickSave(
        ReplayQuickSavePath);
    FPreservedContinuanceFile PreservedReplayQuickSaveBackup(
        ReplayQuickSavePath + TEXT(".bak"));
    FPreservedContinuanceFile PreservedReplayQuickSaveStagedBackup(
        ReplayQuickSavePath + TEXT(".bak.tmp"));
    FPreservedContinuanceFile PreservedReplayQuickSaveTemporary(
        ReplayQuickSavePath + TEXT(".tmp"));
    for (const FString& Path : {
             ReplayQuickSavePath,
             ReplayQuickSavePath + TEXT(".bak"),
             ReplayQuickSavePath + TEXT(".bak.tmp"),
             ReplayQuickSavePath + TEXT(".tmp")})
    {
        IFileManager::Get().Delete(*Path, false, true, true);
    }
    Bridge->StopPrototypeScenario();
    TestTrue(
        TEXT("Recorded Mission 05 remains selectable for replay"),
        !ReplayQuickSavePath.IsEmpty() &&
            Bridge->SelectOperationMode(
                EEchoesOperationMode::CampaignTermsOfContinuance,
                Feedback) &&
            Bridge->StartPrototypeScenario());
    TestTrue(
        TEXT("A recorded Mission 05 replay can write its exact-prerequisite checkpoint"),
        Bridge->QuickSaveScenario(Feedback));
    TestTrue(
        TEXT("A recorded Mission 05 replay can restore its exact-prerequisite checkpoint"),
        Bridge->QuickLoadScenario(Feedback) &&
            Bridge->GetTermsOfContinuancePhase() ==
                EEchoesTermsOfContinuancePhase::SynchronizeNetworks);

    FEchoesCampaignProgress CompletedProgress = SeedProgress;
    TestTrue(TEXT("A fully verified fifth record is accepted"),
             CompletedProgress.AppendDecision(
                 MakeContinuanceRecord(
                     EEchoesCampaignMissionId::TermsOfContinuance,
                     echoes::sim::FutureWellChoice::Preserve),
                 Feedback) == EEchoesCampaignCommitStatus::Added);
    TArray<uint8> Encoded;
    TestTrue(TEXT("The five-record ledger encodes"),
             FEchoesCampaignProgressStore::Encode(
                 CompletedProgress,
                 Encoded,
                 Feedback));
    FEchoesCampaignProgress Decoded;
    TestTrue(TEXT("The five-record ledger decodes"),
             FEchoesCampaignProgressStore::Decode(
                 Encoded,
                 Decoded,
                 Feedback));
    TestTrue(TEXT("The decoded ledger retains the fifth record"),
             Decoded.FindDecision(
                 EEchoesCampaignMissionId::TermsOfContinuance) != nullptr);

    AEchoesPlayerController* Controller =
        World->SpawnActor<AEchoesPlayerController>();
    if (TestNotNull(TEXT("Mission-five result controller can be created"),
                    Controller))
    {
        Controller->NotifyTermsOfContinuanceFinished(
            true,
            echoes::sim::FutureWellChoice::Preserve,
            echoes::sim::FutureWellChoice::Preserve,
            EEchoesCampaignCommitStatus::AlreadyRecorded);
        TestTrue(TEXT("Mission five has a dedicated successful result"),
                 Controller->IsMatchResultVisible() &&
                     Controller->WasCampaignSuccessful() &&
                     Controller->GetPresentedCampaignOperation() ==
                         EEchoesOperationMode::CampaignTermsOfContinuance);
        Controller->ConfirmPrimaryAction();
        TestTrue(TEXT("Mission 05 advances to Names Without Births briefing"),
                 Bridge->GetOperationMode() ==
                         EEchoesOperationMode::CampaignNamesWithoutBirths &&
                     Controller->IsMissionBriefingVisible() &&
                     Bridge->GetNamesWithoutBirthsPhase() ==
                         EEchoesNamesWithoutBirthsPhase::LocateCensus &&
                     Bridge->IsScenarioPaused());
        Controller->Destroy();
    }

    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return true;
}

#endif
