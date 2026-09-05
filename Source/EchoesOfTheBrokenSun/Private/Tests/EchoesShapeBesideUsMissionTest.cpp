#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "EchoesCampaignProgress.h"
#include "EchoesCampaignMapCheckpoint.h"
#include "EchoesShapeBesideUsMissionModel.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Tests/AutomationCommon.h"

namespace
{
struct FPreservedBesideFile final
{
    explicit FPreservedBesideFile(FString InPath) : Path(MoveTemp(InPath))
    {
        bExisted = IFileManager::Get().FileExists(*Path);
        if (bExisted)
        {
            FFileHelper::LoadFileToArray(Contents, *Path);
        }
    }

    ~FPreservedBesideFile()
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

uint8 BesideChoiceMask(echoes::sim::FutureWellChoice Choice)
{
    switch (Choice)
    {
        case echoes::sim::FutureWellChoice::Harvest: return 1 << 0;
        case echoes::sim::FutureWellChoice::Preserve: return 1 << 1;
        case echoes::sim::FutureWellChoice::Reshape: return 1 << 2;
        default: return 0;
    }
}

FEchoesCampaignDecisionRecord MakeBesidePrerequisite(
    EEchoesCampaignMissionId Mission,
    echoes::sim::FutureWellChoice Choice)
{
    FEchoesCampaignDecisionRecord Record;
    Record.Mission = Mission;
    Record.WellChoice = Choice;
    Record.AvailableWellChoices =
        Mission == EEchoesCampaignMissionId::WhatTheLedgerKeeps
            ? 0x07
            : BesideChoiceMask(Choice);
    Record.SimulationSnapshotVersion = echoes::sim::kSnapshotVersion;
    Record.CompletionTick = 100 + static_cast<uint8>(Mission) * 300;
    Record.FinalStateChecksum = 0x8E5100ULL + static_cast<uint8>(Mission);
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
        case EEchoesCampaignMissionId::NamesWithoutBirths:
            Record.VerifiedFacts =
                static_cast<uint8>(EEchoesNamesWithoutBirthsCompletionFact::CensusEvidenceLocated) |
                static_cast<uint8>(EEchoesNamesWithoutBirthsCompletionFact::ArchivePowered) |
                static_cast<uint8>(EEchoesNamesWithoutBirthsCompletionFact::BothCiviliansSheltered) |
                static_cast<uint8>(EEchoesNamesWithoutBirthsCompletionFact::EvidenceExtracted) |
                static_cast<uint8>(EEchoesNamesWithoutBirthsCompletionFact::LocalCoreSurvived) |
                static_cast<uint8>(EEchoesNamesWithoutBirthsCompletionFact::PriorLedgerConsumed);
            break;
        case EEchoesCampaignMissionId::TheShapeOfSilence:
            Record.VerifiedFacts =
                static_cast<uint8>(EEchoesShapeOfSilenceCompletionFact::WaystoneRootedAtListeningAnchor) |
                static_cast<uint8>(EEchoesShapeOfSilenceCompletionFact::ListeningSpineRaised) |
                static_cast<uint8>(EEchoesShapeOfSilenceCompletionFact::BothMemoryWitnessesPositioned) |
                static_cast<uint8>(EEchoesShapeOfSilenceCompletionFact::OruunReachedConfluence) |
                static_cast<uint8>(EEchoesShapeOfSilenceCompletionFact::LocalCoreSurvived) |
                static_cast<uint8>(EEchoesShapeOfSilenceCompletionFact::PriorLedgerConsumed);
            break;
        default:
            break;
    }
    return Record;
}

FString BesideQuickSavePath(const FEchoesCampaignProgress& Progress)
{
    TArray<uint8> LedgerBytes;
    FString Error;
    if (!FEchoesCampaignProgressStore::Encode(
            Progress, LedgerBytes, Error) || LedgerBytes.IsEmpty())
    {
        return {};
    }
    const uint32 Fingerprint = FCrc::MemCrc32(
        LedgerBytes.GetData(),
        LedgerBytes.Num() - static_cast<int32>(sizeof(uint32)));
    return FPaths::Combine(
        FEchoesCampaignProgressStore::GetSaveGameDirectory(),
        FString::Printf(
            TEXT("EchoesQuickSaveTheShapeBesideUs-%08X.bin"),
            Fingerprint));
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesShapeBesideUsMissionTest,
    "Echoes.Runtime.Campaign.TheShapeBesideUs",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesShapeBesideUsMissionTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    FEchoesScopedTestSaveEnvironment TestSaveEnvironment(*this);
    if (!TestSaveEnvironment.IsReady())
    {
        return false;
    }

    const FEchoesShapeBesideUsPlan HarvestPlan =
        FEchoesShapeBesideUsMissionModel::PlanForChoice(
            echoes::sim::FutureWellChoice::Harvest);
    const FEchoesShapeBesideUsPlan PreservePlan =
        FEchoesShapeBesideUsMissionModel::PlanForChoice(
            echoes::sim::FutureWellChoice::Preserve);
    const FEchoesShapeBesideUsPlan ReshapePlan =
        FEchoesShapeBesideUsMissionModel::PlanForChoice(
            echoes::sim::FutureWellChoice::Reshape);
    TestTrue(TEXT("All three inherited choices produce distinct overlap geometry"),
             HarvestPlan.FirstEchoSite == echoes::sim::Vec2::FromTiles(14, 28) &&
                 PreservePlan.FirstEchoSite == echoes::sim::Vec2::FromTiles(32, 28) &&
                 ReshapePlan.FirstEchoSite == echoes::sim::Vec2::FromTiles(46, 20) &&
                 HarvestPlan.FirstStateSite != PreservePlan.FirstStateSite &&
                 PreservePlan.ConvergenceSite != ReshapePlan.ConvergenceSite);

    FEchoesShapeBesideUsMissionFacts Facts;
    TestTrue(TEXT("Inactive facts stay outside mission eight"),
             FEchoesShapeBesideUsMissionModel::DeterminePhase(Facts) ==
                 EEchoesShapeBesideUsPhase::Inactive);
    Facts.bOperationActive = true;
    Facts.bLocalCoreIntact = true;
    Facts.bTalarIntact = true;
    Facts.bFirstStateWitnessIntact = true;
    Facts.bSecondStateWitnessIntact = true;
    Facts.bSkirmishStillOngoing = true;
    TestTrue(TEXT("The operation begins at Neme's first echo"),
             FEchoesShapeBesideUsMissionModel::DeterminePhase(Facts) ==
                 EEchoesShapeBesideUsPhase::ReachFirstEcho);
    Facts.bFirstEchoObserved = true;
    TestTrue(TEXT("Observation opens relay construction"),
             FEchoesShapeBesideUsMissionModel::DeterminePhase(Facts) ==
                 EEchoesShapeBesideUsPhase::RaiseEchoRelay);
    Facts.bEchoRelayRaised = true;
    TestTrue(TEXT("The relay opens paired-state traversal"),
             FEchoesShapeBesideUsMissionModel::DeterminePhase(Facts) ==
                 EEchoesShapeBesideUsPhase::TraversePairedStates);
    Facts.bFirstStateTraversed = true;
    Facts.bSecondStateTraversed = true;
    TestTrue(TEXT("Both states open Talar's convergence"),
             FEchoesShapeBesideUsMissionModel::DeterminePhase(Facts) ==
                 EEchoesShapeBesideUsPhase::ReachConvergence);
    Facts.bTalarAtConvergence = true;
    TestTrue(TEXT("The full ordered contact completes"),
             FEchoesShapeBesideUsMissionModel::DeterminePhase(Facts) ==
                 EEchoesShapeBesideUsPhase::Complete);
    Facts.bSecondStateWitnessIntact = false;
    TestTrue(TEXT("Any protected loss fails the operation"),
             FEchoesShapeBesideUsMissionModel::DeterminePhase(Facts) ==
                 EEchoesShapeBesideUsPhase::Failed);
    Facts.bSecondStateWitnessIntact = true;
    Facts.bSkirmishStillOngoing = false;
    TestTrue(TEXT("Either terminal Core outcome invalidates contact"),
             FEchoesShapeBesideUsMissionModel::DeterminePhase(Facts) ==
                 EEchoesShapeBesideUsPhase::Failed);

    const FString CampaignPath =
        FEchoesCampaignProgressStore::GetDefaultPath();
    FPreservedBesideFile PreservedPrimary(CampaignPath);
    FPreservedBesideFile PreservedBackup(CampaignPath + TEXT(".bak"));
    FPreservedBesideFile PreservedTemporary(CampaignPath + TEXT(".tmp"));
    IFileManager::Get().Delete(*CampaignPath, false, true, true);
    IFileManager::Get().Delete(*(CampaignPath + TEXT(".bak")), false, true, true);
    IFileManager::Get().Delete(*(CampaignPath + TEXT(".tmp")), false, true, true);

    FString Feedback;
    FEchoesCampaignProgress SixRecords;
    for (const EEchoesCampaignMissionId Mission : {
             EEchoesCampaignMissionId::WhatTheLedgerKeeps,
             EEchoesCampaignMissionId::SevenAccountsOfRain,
             EEchoesCampaignMissionId::ACityOnReserve,
             EEchoesCampaignMissionId::TheUnburiedRoad,
             EEchoesCampaignMissionId::TermsOfContinuance,
             EEchoesCampaignMissionId::NamesWithoutBirths})
    {
        TestTrue(TEXT("The fixture accepts a consistent prior record"),
                 SixRecords.AppendDecision(
                     MakeBesidePrerequisite(
                         Mission, echoes::sim::FutureWellChoice::Preserve),
                     Feedback) == EEchoesCampaignCommitStatus::Added);
    }
    TestTrue(TEXT("A six-record ledger is stored for the lock check"),
             FEchoesCampaignProgressStore::SaveAtomic(
                 CampaignPath, SixRecords, Feedback));
    {
        FTestWorldWrapper LockedWorld;
        if (!LockedWorld.CreateTestWorld(EWorldType::Game))
        {
            LockedWorld.ForwardErrorMessages(this);
            AddError(TEXT("Could not create the locked mission-eight world."));
            return false;
        }
        UEchoesSimulationSubsystem* LockedBridge =
            LockedWorld.GetTestWorld()->GetSubsystem<
                UEchoesSimulationSubsystem>();
        TestFalse(TEXT("Mission eight rejects a six-record ledger"),
                  LockedBridge != nullptr &&
                      LockedBridge->SelectOperationMode(
                          EEchoesOperationMode::CampaignShapeBesideUs,
                          Feedback));
        TestTrue(TEXT("The locked response names The Shape of Silence"),
                 Feedback.Contains(TEXT("The Shape of Silence")));
        LockedWorld.ForwardErrorMessages(this);
    }

    FEchoesCampaignProgress SevenRecords = SixRecords;
    TestTrue(TEXT("The seventh prerequisite record is accepted"),
             SevenRecords.AppendDecision(
                 MakeBesidePrerequisite(
                     EEchoesCampaignMissionId::TheShapeOfSilence,
                     echoes::sim::FutureWellChoice::Preserve),
                 Feedback) == EEchoesCampaignCommitStatus::Added);
    const FString QuickSavePath = BesideQuickSavePath(SevenRecords);
    FPreservedBesideFile PreservedQuickSave(QuickSavePath);
    FPreservedBesideFile PreservedQuickSaveBackup(
        QuickSavePath + TEXT(".bak"));
    FPreservedBesideFile PreservedQuickSaveTemporary(
        QuickSavePath + TEXT(".tmp"));
    IFileManager::Get().Delete(*QuickSavePath, false, true, true);
    IFileManager::Get().Delete(*(QuickSavePath + TEXT(".bak")), false, true, true);
    IFileManager::Get().Delete(*(QuickSavePath + TEXT(".tmp")), false, true, true);
    TestTrue(TEXT("The seven-record ledger is stored"),
             FEchoesCampaignProgressStore::SaveAtomic(
                 CampaignPath, SevenRecords, Feedback));

    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create The Shape Beside Us test world."));
        return false;
    }
    UEchoesSimulationSubsystem* Bridge =
        WorldWrapper.GetTestWorld()->GetSubsystem<UEchoesSimulationSubsystem>();
    if (!TestNotNull(TEXT("Mission world owns the simulation subsystem"), Bridge) ||
        !TestTrue(TEXT("Seven consistent records unlock mission eight"),
                  Bridge != nullptr && Bridge->IsShapeBesideUsUnlocked()) ||
        !TestTrue(TEXT("Mission eight operation can be selected"),
                  Bridge->SelectOperationMode(
                      EEchoesOperationMode::CampaignShapeBesideUs,
                      Feedback)) ||
        !TestTrue(TEXT("Mission eight scenario starts"),
                  Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    FEchoesObjectiveSnapshot Start = Bridge->GetLocalObjectiveSnapshot();
    TestTrue(TEXT("Talar and both state witnesses have distinct authority"),
             Start.ShapeBesideUsTalarId != 0 &&
                 Start.FirstStateWitnessId != 0 &&
                 Start.SecondStateWitnessId != 0 &&
                 Start.ShapeBesideUsTalarId != Start.FirstStateWitnessId &&
                 Start.ShapeBesideUsTalarId != Start.SecondStateWitnessId &&
                 Start.FirstStateWitnessId != Start.SecondStateWitnessId);
    TestTrue(TEXT("The operation begins at first-echo observation"),
             Bridge->GetShapeBesideUsPhase() ==
                 EEchoesShapeBesideUsPhase::ReachFirstEcho);
    TestTrue(TEXT("Mission eight uses its ledger-bound quick-save slot"),
             Bridge->QuickSaveScenario(Feedback) &&
                 IFileManager::Get().FileExists(*QuickSavePath));
    TestTrue(TEXT("The initial phase reconstructs after quick load"),
             Bridge->QuickLoadScenario(Feedback) &&
                 Bridge->GetShapeBesideUsPhase() ==
                     EEchoesShapeBesideUsPhase::ReachFirstEcho);

    // Mission eight stamps topology revision one, so a checkpoint written
    // before the route moved out of the hostile envelope must fail closed
    // rather than restore geometry the operation no longer uses. The probe
    // rewrites only the revision byte and repairs the container checksum, so
    // the rejection can come from the topology rule alone and not from a
    // detectable corruption.
    constexpr int32 TopologyRevisionOffset = 11;
    constexpr int32 ChecksumSize = 4;
    TArray<uint8> CurrentTopologyBytes;
    TArray<uint8> CurrentMapEnvelope;
    FEchoesCampaignMapCheckpointIdentity MapIdentity;
    EEchoesCampaignMapCheckpointFailure MapFailure{};
    if (!TestTrue(
            TEXT("The current Mission 08 checkpoint carries topology revision one"),
            FFileHelper::LoadFileToArray(CurrentMapEnvelope, *QuickSavePath) &&
                FEchoesCampaignMapCheckpoint::Inspect(CurrentMapEnvelope, MapIdentity, CurrentTopologyBytes, MapFailure) &&
                CurrentTopologyBytes.Num() > 16 &&
                CurrentTopologyBytes[TopologyRevisionOffset] == 1))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    TArray<uint8> LegacyTopologyBytes = CurrentTopologyBytes;
    LegacyTopologyBytes[TopologyRevisionOffset] = 0;
    const int32 ChecksumOffset = LegacyTopologyBytes.Num() - ChecksumSize;
    const uint32 LegacyChecksum = FCrc::MemCrc32(
        LegacyTopologyBytes.GetData(), ChecksumOffset);
    for (int32 ByteIndex = 0; ByteIndex < ChecksumSize; ++ByteIndex)
    {
        LegacyTopologyBytes[ChecksumOffset + ByteIndex] =
            static_cast<uint8>(LegacyChecksum >> (ByteIndex * 8));
    }
    if (!TestTrue(
            TEXT("The revision-zero Mission 08 fixture retains a valid checksum"),
            [&]() { TArray<uint8> Envelope; return FEchoesCampaignMapCheckpoint::Wrap(MapIdentity, LegacyTopologyBytes, Envelope, MapFailure) && FFileHelper::SaveArrayToFile(Envelope, *QuickSavePath); }()))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    TestFalse(TEXT("QuickLoad rejects the revision-zero Mission 08 topology"),
              Bridge->QuickLoadScenario(Feedback));
    TestTrue(TEXT("The Mission 08 topology rejection is explicit and stable"),
             Feedback.Contains(TEXT("LOAD_SHAPE_BESIDE_US_TOPOLOGY_MISMATCH")));
    if (!TestTrue(
            TEXT("The current Mission 08 checkpoint is restored after the legacy probe"),
            FFileHelper::SaveArrayToFile(CurrentMapEnvelope, *QuickSavePath)))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    TestTrue(TEXT("The restored Mission 08 checkpoint reloads cleanly"),
             Bridge->QuickLoadScenario(Feedback) &&
                 Bridge->GetShapeBesideUsPhase() ==
                     EEchoesShapeBesideUsPhase::ReachFirstEcho);

    echoes::sim::EntityId WorkerId = 0;
    for (const echoes::sim::Entity& Entity :
         Bridge->GetSimulation()->Entities())
    {
        if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            Entity.type == echoes::sim::EntityType::Worker &&
            Entity.id != Start.FirstStateWitnessId)
        {
            WorkerId = Entity.id;
            break;
        }
    }
    if (!TestTrue(TEXT("A separate construction worker is available"),
                  WorkerId != 0))
    {
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
    TestTrue(TEXT("Talar accepts the first-echo route"),
             Bridge->IssueCommand(
                 echoes::sim::CommandType::Move,
                 Start.ShapeBesideUsTalarId,
                 0,
                 Bridge->SimToWorld(PreservePlan.FirstEchoSite),
                 echoes::sim::FutureWellChoice::Dormant,
                 Feedback));
    TestTrue(TEXT("Talar's arrival opens relay construction"),
             TickUntil(
                 [Bridge]()
                 {
                     return Bridge->GetShapeBesideUsPhase() ==
                         EEchoesShapeBesideUsPhase::RaiseEchoRelay;
                 },
                 3000));
    TestTrue(TEXT("An ordinary worker accepts the echo-relay build"),
             Bridge->IssueBuildCommand(
                 WorkerId,
                 echoes::sim::EntityType::UtilityStructure,
                 Bridge->SimToWorld(PreservePlan.EchoRelaySite),
                 Feedback));
    TestTrue(TEXT("Completed construction opens paired-state traversal"),
             TickUntil(
                 [Bridge]()
                 {
                     return Bridge->GetShapeBesideUsPhase() ==
                         EEchoesShapeBesideUsPhase::TraversePairedStates;
                 },
                 3400));
    TestTrue(TEXT("The first witness accepts its state route"),
             Bridge->IssueCommand(
                 echoes::sim::CommandType::Move,
                 Start.FirstStateWitnessId,
                 0,
                 Bridge->SimToWorld(PreservePlan.FirstStateSite),
                 echoes::sim::FutureWellChoice::Dormant,
                 Feedback));
    TestTrue(TEXT("The second witness accepts its state route"),
             Bridge->IssueCommand(
                 echoes::sim::CommandType::Move,
                 Start.SecondStateWitnessId,
                 0,
                 Bridge->SimToWorld(PreservePlan.SecondStateSite),
                 echoes::sim::FutureWellChoice::Dormant,
                 Feedback));
    TestTrue(TEXT("Both ordinary movements open Talar's convergence"),
             TickUntil(
                 [Bridge]()
                 {
                     return Bridge->GetShapeBesideUsPhase() ==
                         EEchoesShapeBesideUsPhase::ReachConvergence;
                 },
                 3600));
    TestTrue(TEXT("Talar accepts Neme's convergence route"),
             Bridge->IssueCommand(
                 echoes::sim::CommandType::Move,
                 Start.ShapeBesideUsTalarId,
                 0,
                 Bridge->SimToWorld(PreservePlan.ConvergenceSite),
                 echoes::sim::FutureWellChoice::Dormant,
                 Feedback));
    TestTrue(TEXT("The ordinary mission path commits the eighth ledger record"),
             TickUntil(
                 [Bridge]()
                 {
                     return Bridge->GetCampaignProgress().FindDecision(
                                EEchoesCampaignMissionId::TheShapeBesideUs) !=
                         nullptr;
                 },
                 3600));
    const FEchoesCampaignDecisionRecord* MissionRecord =
        Bridge->GetCampaignProgress().FindDecision(
            EEchoesCampaignMissionId::TheShapeBesideUs);
    TestTrue(TEXT("The eighth record preserves branch and all verified facts"),
             MissionRecord != nullptr &&
                 MissionRecord->WellChoice ==
                     echoes::sim::FutureWellChoice::Preserve &&
                 MissionRecord->AvailableWellChoices == (1 << 1) &&
                 MissionRecord->VerifiedFacts == 0x3F);
    FEchoesCampaignProgress Reloaded;
    TestTrue(TEXT("The eight-record campaign reloads transactionally"),
             FEchoesCampaignProgressStore::LoadWithBackup(
                 CampaignPath, Reloaded, Feedback) &&
                 Reloaded.Decisions.Num() == 8);

    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return true;
}

#endif
