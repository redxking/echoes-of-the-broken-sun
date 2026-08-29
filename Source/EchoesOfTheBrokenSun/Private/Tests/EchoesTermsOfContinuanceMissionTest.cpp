#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesCampaignProgress.h"
#include "EchoesPlayerController.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesTermsOfContinuanceMissionModel.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
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

    const FString CampaignPath =
        FEchoesCampaignProgressStore::GetDefaultPath();
    const FString QuickSavePath = FPaths::Combine(
        FPaths::ProjectSavedDir(),
        TEXT("SaveGames"),
        TEXT("EchoesQuickSaveTermsOfContinuance.bin"));
    FPreservedContinuanceFile PreservedPrimary(CampaignPath);
    FPreservedContinuanceFile PreservedBackup(CampaignPath + TEXT(".bak"));
    FPreservedContinuanceFile PreservedTemporary(CampaignPath + TEXT(".tmp"));
    FPreservedContinuanceFile PreservedQuickSave(QuickSavePath);
    FPreservedContinuanceFile PreservedQuickSaveBackup(
        QuickSavePath + TEXT(".bak"));
    FPreservedContinuanceFile PreservedQuickSaveTemporary(
        QuickSavePath + TEXT(".tmp"));
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
    TestTrue(TEXT("The four-record ledger is stored"),
             FEchoesCampaignProgressStore::SaveAtomic(
                 CampaignPath,
                 SeedProgress,
                 Feedback));

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
        TestTrue(TEXT("Replay reconstructs Terms of Continuance"),
                 Bridge->GetOperationMode() ==
                         EEchoesOperationMode::CampaignTermsOfContinuance &&
                     Bridge->GetTermsOfContinuancePhase() ==
                         EEchoesTermsOfContinuancePhase::SynchronizeNetworks);
        Controller->Destroy();
    }

    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return true;
}

#endif
