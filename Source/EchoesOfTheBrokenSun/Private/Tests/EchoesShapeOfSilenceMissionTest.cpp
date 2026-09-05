#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "EchoesCampaignProgress.h"
#include "EchoesCampaignMapCheckpoint.h"
#include "EchoesShapeOfSilenceMissionModel.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Tests/AutomationCommon.h"

namespace
{
struct FPreservedShapeFile final
{
    explicit FPreservedShapeFile(FString InPath) : Path(MoveTemp(InPath))
    {
        bExisted = IFileManager::Get().FileExists(*Path);
        if (bExisted)
        {
            FFileHelper::LoadFileToArray(Contents, *Path);
        }
    }

    ~FPreservedShapeFile()
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

uint8 ShapeChoiceMask(echoes::sim::FutureWellChoice Choice)
{
    switch (Choice)
    {
        case echoes::sim::FutureWellChoice::Harvest: return 1 << 0;
        case echoes::sim::FutureWellChoice::Preserve: return 1 << 1;
        case echoes::sim::FutureWellChoice::Reshape: return 1 << 2;
        default: return 0;
    }
}

FEchoesCampaignDecisionRecord MakeShapePrerequisite(
    EEchoesCampaignMissionId Mission,
    echoes::sim::FutureWellChoice Choice)
{
    FEchoesCampaignDecisionRecord Record;
    Record.Mission = Mission;
    Record.WellChoice = Choice;
    Record.AvailableWellChoices =
        Mission == EEchoesCampaignMissionId::WhatTheLedgerKeeps
            ? 0x07
            : ShapeChoiceMask(Choice);
    Record.SimulationSnapshotVersion = echoes::sim::kSnapshotVersion;
    Record.CompletionTick = 100 + static_cast<uint8>(Mission) * 300;
    Record.FinalStateChecksum = 0x5A7100ULL + static_cast<uint8>(Mission);
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
        default:
            break;
    }
    return Record;
}

FString ShapeQuickSavePath(const FEchoesCampaignProgress& Progress)
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
            TEXT("EchoesQuickSaveTheShapeOfSilence-%08X.bin"),
            Fingerprint));
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesShapeOfSilenceMissionTest,
    "Echoes.Runtime.Campaign.TheShapeOfSilence",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesShapeOfSilenceMissionTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    FEchoesScopedTestSaveEnvironment TestSaveEnvironment(*this);
    if (!TestSaveEnvironment.IsReady())
    {
        return false;
    }

    const FEchoesShapeOfSilencePlan HarvestPlan =
        FEchoesShapeOfSilenceMissionModel::PlanForChoice(
            echoes::sim::FutureWellChoice::Harvest);
    const FEchoesShapeOfSilencePlan PreservePlan =
        FEchoesShapeOfSilenceMissionModel::PlanForChoice(
            echoes::sim::FutureWellChoice::Preserve);
    const FEchoesShapeOfSilencePlan ReshapePlan =
        FEchoesShapeOfSilenceMissionModel::PlanForChoice(
            echoes::sim::FutureWellChoice::Reshape);
    TestTrue(
        TEXT("All three inherited choices retain their exact listening geometry"),
        HarvestPlan.WaystoneAnchor == echoes::sim::Vec2::FromTiles(14, 28) &&
            HarvestPlan.ListeningSpineSite ==
                echoes::sim::Vec2::FromTiles(14, 38) &&
            HarvestPlan.FirstWitnessSite ==
                echoes::sim::Vec2::FromTiles(10, 45) &&
            HarvestPlan.SecondWitnessSite ==
                echoes::sim::Vec2::FromTiles(18, 45) &&
            HarvestPlan.ConfluenceSite ==
                echoes::sim::Vec2::FromTiles(14, 50) &&
            PreservePlan.WaystoneAnchor ==
                echoes::sim::Vec2::FromTiles(32, 28) &&
            PreservePlan.ListeningSpineSite ==
                echoes::sim::Vec2::FromTiles(32, 38) &&
            PreservePlan.FirstWitnessSite ==
                echoes::sim::Vec2::FromTiles(28, 45) &&
            PreservePlan.SecondWitnessSite ==
                echoes::sim::Vec2::FromTiles(36, 45) &&
            PreservePlan.ConfluenceSite ==
                echoes::sim::Vec2::FromTiles(32, 50) &&
            ReshapePlan.WaystoneAnchor ==
                echoes::sim::Vec2::FromTiles(48, 20) &&
            ReshapePlan.ListeningSpineSite ==
                echoes::sim::Vec2::FromTiles(39, 37) &&
            ReshapePlan.FirstWitnessSite ==
                echoes::sim::Vec2::FromTiles(22, 38) &&
            ReshapePlan.SecondWitnessSite ==
                echoes::sim::Vec2::FromTiles(30, 38) &&
            ReshapePlan.ConfluenceSite ==
                echoes::sim::Vec2::FromTiles(25, 50));

    FEchoesShapeOfSilenceMissionFacts Facts;
    TestTrue(TEXT("Inactive facts stay outside mission seven"),
             FEchoesShapeOfSilenceMissionModel::DeterminePhase(Facts) ==
                 EEchoesShapeOfSilencePhase::Inactive);
    Facts.bOperationActive = true;
    Facts.bLocalCoreIntact = true;
    Facts.bWaystoneIntact = true;
    Facts.bOruunIntact = true;
    Facts.bFirstWitnessIntact = true;
    Facts.bSecondWitnessIntact = true;
    Facts.bSkirmishStillOngoing = true;
    TestTrue(TEXT("The operation begins by rooting the Waystone"),
             FEchoesShapeOfSilenceMissionModel::DeterminePhase(Facts) ==
                 EEchoesShapeOfSilencePhase::RootWaystone);
    Facts.bWaystoneRootedAtAnchor = true;
    TestTrue(TEXT("Rooting opens Listening Spine construction"),
             FEchoesShapeOfSilenceMissionModel::DeterminePhase(Facts) ==
                 EEchoesShapeOfSilencePhase::RaiseListeningSpine);
    Facts.bListeningSpineRaised = true;
    TestTrue(TEXT("The Listening Spine opens paired witnessing"),
             FEchoesShapeOfSilenceMissionModel::DeterminePhase(Facts) ==
                 EEchoesShapeOfSilencePhase::PositionMemoryWitnesses);
    Facts.bFirstWitnessPositioned = true;
    Facts.bSecondWitnessPositioned = true;
    TestTrue(TEXT("Both witnesses open Oruun's confluence"),
             FEchoesShapeOfSilenceMissionModel::DeterminePhase(Facts) ==
                 EEchoesShapeOfSilencePhase::ReachConfluence);
    Facts.bOruunAtConfluence = true;
    TestTrue(TEXT("The full ordered observation completes"),
             FEchoesShapeOfSilenceMissionModel::DeterminePhase(Facts) ==
                 EEchoesShapeOfSilencePhase::Complete);
    Facts.bFirstWitnessIntact = false;
    TestTrue(TEXT("Any protected loss fails the operation"),
             FEchoesShapeOfSilenceMissionModel::DeterminePhase(Facts) ==
                 EEchoesShapeOfSilencePhase::Failed);
    Facts.bFirstWitnessIntact = true;
    Facts.bSkirmishStillOngoing = false;
    TestTrue(TEXT("Either terminal Core outcome invalidates the observation"),
             FEchoesShapeOfSilenceMissionModel::DeterminePhase(Facts) ==
                 EEchoesShapeOfSilencePhase::Failed);

    const FString CampaignPath =
        FEchoesCampaignProgressStore::GetDefaultPath();
    FPreservedShapeFile PreservedPrimary(CampaignPath);
    FPreservedShapeFile PreservedBackup(CampaignPath + TEXT(".bak"));
    FPreservedShapeFile PreservedTemporary(CampaignPath + TEXT(".tmp"));
    IFileManager::Get().Delete(*CampaignPath, false, true, true);
    IFileManager::Get().Delete(*(CampaignPath + TEXT(".bak")), false, true, true);
    IFileManager::Get().Delete(*(CampaignPath + TEXT(".tmp")), false, true, true);

    FString Feedback;
    FEchoesCampaignProgress FiveRecords;
    for (const EEchoesCampaignMissionId Mission : {
             EEchoesCampaignMissionId::WhatTheLedgerKeeps,
             EEchoesCampaignMissionId::SevenAccountsOfRain,
             EEchoesCampaignMissionId::ACityOnReserve,
             EEchoesCampaignMissionId::TheUnburiedRoad,
             EEchoesCampaignMissionId::TermsOfContinuance})
    {
        TestTrue(TEXT("The fixture accepts a consistent prior record"),
                 FiveRecords.AppendDecision(
                     MakeShapePrerequisite(
                         Mission, echoes::sim::FutureWellChoice::Preserve),
                     Feedback) == EEchoesCampaignCommitStatus::Added);
    }
    TestTrue(TEXT("A five-record ledger is stored for the lock check"),
             FEchoesCampaignProgressStore::SaveAtomic(
                 CampaignPath, FiveRecords, Feedback));
    {
        FTestWorldWrapper LockedWorld;
        if (!LockedWorld.CreateTestWorld(EWorldType::Game))
        {
            LockedWorld.ForwardErrorMessages(this);
            AddError(TEXT("Could not create the locked mission-seven world."));
            return false;
        }
        UEchoesSimulationSubsystem* LockedBridge =
            LockedWorld.GetTestWorld()->GetSubsystem<
                UEchoesSimulationSubsystem>();
        TestFalse(TEXT("Mission seven rejects a five-record ledger"),
                  LockedBridge != nullptr &&
                      LockedBridge->SelectOperationMode(
                          EEchoesOperationMode::CampaignShapeOfSilence,
                          Feedback));
        TestTrue(TEXT("The locked response names Names Without Births"),
                 Feedback.Contains(TEXT("Names Without Births")));
        LockedWorld.ForwardErrorMessages(this);
    }

    FEchoesCampaignProgress SixRecords = FiveRecords;
    TestTrue(TEXT("The sixth prerequisite record is accepted"),
             SixRecords.AppendDecision(
                 MakeShapePrerequisite(
                     EEchoesCampaignMissionId::NamesWithoutBirths,
                     echoes::sim::FutureWellChoice::Preserve),
                 Feedback) == EEchoesCampaignCommitStatus::Added);
    const FString QuickSavePath = ShapeQuickSavePath(SixRecords);
    FPreservedShapeFile PreservedQuickSave(QuickSavePath);
    FPreservedShapeFile PreservedQuickSaveBackup(QuickSavePath + TEXT(".bak"));
    FPreservedShapeFile PreservedQuickSaveTemporary(QuickSavePath + TEXT(".tmp"));
    IFileManager::Get().Delete(*QuickSavePath, false, true, true);
    IFileManager::Get().Delete(*(QuickSavePath + TEXT(".bak")), false, true, true);
    IFileManager::Get().Delete(*(QuickSavePath + TEXT(".tmp")), false, true, true);
    TestTrue(TEXT("The six-record ledger is stored"),
             FEchoesCampaignProgressStore::SaveAtomic(
                 CampaignPath, SixRecords, Feedback));

    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create The Shape of Silence test world."));
        return false;
    }
    UEchoesSimulationSubsystem* Bridge =
        WorldWrapper.GetTestWorld()->GetSubsystem<UEchoesSimulationSubsystem>();
    if (!TestNotNull(TEXT("Mission world owns the simulation subsystem"), Bridge) ||
        !TestTrue(TEXT("Six consistent records unlock mission seven"),
                  Bridge != nullptr && Bridge->IsShapeOfSilenceUnlocked()) ||
        !TestTrue(TEXT("Mission seven operation can be selected"),
                  Bridge->SelectOperationMode(
                      EEchoesOperationMode::CampaignShapeOfSilence,
                      Feedback)) ||
        !TestTrue(TEXT("Mission seven scenario starts"),
                  Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    FEchoesObjectiveSnapshot Start = Bridge->GetLocalObjectiveSnapshot();
    TestTrue(TEXT("Oruun and both memory witnesses have distinct authority"),
             Start.OruunId != 0 && Start.FirstMemoryWitnessId != 0 &&
                 Start.SecondMemoryWitnessId != 0 &&
                 Start.OruunId != Start.FirstMemoryWitnessId &&
                 Start.OruunId != Start.SecondMemoryWitnessId &&
                 Start.FirstMemoryWitnessId != Start.SecondMemoryWitnessId);
    TestTrue(TEXT("The operation begins at Waystone establishment"),
             Bridge->GetShapeOfSilencePhase() ==
                 EEchoesShapeOfSilencePhase::RootWaystone);
    if (!TestTrue(TEXT("Mission seven uses its ledger-bound quick-save slot"),
                  Bridge->QuickSaveScenario(Feedback) &&
                      IFileManager::Get().FileExists(*QuickSavePath)))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    constexpr int32 TopologyRevisionOffset = 11;
    constexpr int32 ChecksumSize = 4;
    TArray<uint8> CurrentTopologyBytes;
    TArray<uint8> CurrentMapEnvelope;
    FEchoesCampaignMapCheckpointIdentity MapIdentity;
    EEchoesCampaignMapCheckpointFailure MapFailure{};
    if (!TestTrue(
            TEXT("The current Mission 07 checkpoint carries topology revision two"),
            FFileHelper::LoadFileToArray(
                CurrentMapEnvelope,
                *QuickSavePath) &&
                FEchoesCampaignMapCheckpoint::Inspect(CurrentMapEnvelope, MapIdentity, CurrentTopologyBytes, MapFailure) &&
                CurrentTopologyBytes.Num() > 16 &&
                CurrentTopologyBytes[TopologyRevisionOffset] == 2))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    TestTrue(
        TEXT("Topology revision one binds the prior Reshape anchor and downstream geometry"),
        echoes::sim::Vec2::FromTiles(50, 28) !=
                ReshapePlan.WaystoneAnchor &&
            echoes::sim::Vec2::FromTiles(39, 37) ==
                ReshapePlan.ListeningSpineSite &&
            echoes::sim::Vec2::FromTiles(22, 38) ==
                ReshapePlan.FirstWitnessSite &&
            echoes::sim::Vec2::FromTiles(30, 38) ==
                ReshapePlan.SecondWitnessSite &&
            echoes::sim::Vec2::FromTiles(25, 50) ==
                ReshapePlan.ConfluenceSite);
    TestTrue(
        TEXT("Topology revision zero binds the original Reshape listening geometry"),
        echoes::sim::Vec2::FromTiles(50, 28) !=
                ReshapePlan.WaystoneAnchor &&
            echoes::sim::Vec2::FromTiles(50, 38) !=
                ReshapePlan.ListeningSpineSite &&
            echoes::sim::Vec2::FromTiles(46, 45) !=
                ReshapePlan.FirstWitnessSite &&
            echoes::sim::Vec2::FromTiles(54, 45) !=
                ReshapePlan.SecondWitnessSite &&
            echoes::sim::Vec2::FromTiles(50, 50) !=
                ReshapePlan.ConfluenceSite);
    TArray<uint8> LegacyTopologyBytes = CurrentTopologyBytes;
    LegacyTopologyBytes[TopologyRevisionOffset] = 1;
    const int32 ChecksumOffset =
        LegacyTopologyBytes.Num() - ChecksumSize;
    uint32 LegacyChecksum = FCrc::MemCrc32(
        LegacyTopologyBytes.GetData(),
        ChecksumOffset);
    for (int32 ByteIndex = 0;
         ByteIndex < ChecksumSize;
         ++ByteIndex)
    {
        LegacyTopologyBytes[ChecksumOffset + ByteIndex] =
            static_cast<uint8>(
                LegacyChecksum >> (ByteIndex * 8));
    }
    if (!TestTrue(
            TEXT("The revision-one Mission 07 fixture retains a valid checksum"),
            [&]() { TArray<uint8> Envelope; return FEchoesCampaignMapCheckpoint::Wrap(MapIdentity, LegacyTopologyBytes, Envelope, MapFailure) && FFileHelper::SaveArrayToFile(Envelope, *QuickSavePath); }()))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    TestFalse(
        TEXT("QuickLoad rejects the revision-one Mission 07 topology"),
        Bridge->QuickLoadScenario(Feedback));
    TestTrue(
        TEXT("The revision-one Mission 07 topology rejection is explicit and stable"),
        Feedback.Contains(TEXT("LOAD_SHAPE_OF_SILENCE_TOPOLOGY_MISMATCH")));
    LegacyTopologyBytes[TopologyRevisionOffset] = 0;
    LegacyChecksum = FCrc::MemCrc32(
        LegacyTopologyBytes.GetData(),
        ChecksumOffset);
    for (int32 ByteIndex = 0;
         ByteIndex < ChecksumSize;
         ++ByteIndex)
    {
        LegacyTopologyBytes[ChecksumOffset + ByteIndex] =
            static_cast<uint8>(
                LegacyChecksum >> (ByteIndex * 8));
    }
    if (!TestTrue(
            TEXT("The revision-zero Mission 07 fixture retains a valid checksum"),
            [&]() { TArray<uint8> Envelope; return FEchoesCampaignMapCheckpoint::Wrap(MapIdentity, LegacyTopologyBytes, Envelope, MapFailure) && FFileHelper::SaveArrayToFile(Envelope, *QuickSavePath); }()))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    TestFalse(
        TEXT("QuickLoad rejects the revision-zero Mission 07 topology"),
        Bridge->QuickLoadScenario(Feedback));
    TestTrue(
        TEXT("The Mission 07 topology rejection is explicit and stable"),
        Feedback.Contains(TEXT("LOAD_SHAPE_OF_SILENCE_TOPOLOGY_MISMATCH")));
    if (!TestTrue(
            TEXT("The current Mission 07 checkpoint is restored after the legacy probe"),
            FFileHelper::SaveArrayToFile(CurrentMapEnvelope, *QuickSavePath)))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    TestTrue(TEXT("The initial phase reconstructs after quick load"),
             Bridge->QuickLoadScenario(Feedback) &&
                 Bridge->GetShapeOfSilencePhase() ==
                     EEchoesShapeOfSilencePhase::RootWaystone);

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
    if (!TestTrue(TEXT("A construction worker is available"), WorkerId != 0))
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
    const echoes::sim::EntityId WaystoneId = Start.MigrationWaystoneId;
    TestTrue(TEXT("The rooted Waystone accepts an uproot order"),
             Bridge->IssueCommand(
                 echoes::sim::CommandType::ToggleWaystoneRoot,
                 WaystoneId,
                 0,
                 FVector::ZeroVector,
                 echoes::sim::FutureWellChoice::Dormant,
                 Feedback));
    TestTrue(TEXT("The Waystone completes its uproot transition"),
             TickUntil(
                 [Bridge, WaystoneId]()
                 {
                     const echoes::sim::Entity* Waystone =
                         Bridge->FindEntity(WaystoneId);
                     return Waystone != nullptr &&
                         Waystone->waystoneMode ==
                             echoes::sim::WaystoneMode::Mobile;
                 },
                 300));
    TestTrue(TEXT("The mobile Waystone accepts the inherited anchor"),
             Bridge->IssueCommand(
                 echoes::sim::CommandType::Move,
                 WaystoneId,
                 0,
                 Bridge->SimToWorld(PreservePlan.WaystoneAnchor),
                 echoes::sim::FutureWellChoice::Dormant,
                 Feedback));
    TestTrue(TEXT("The Waystone reaches the listening anchor"),
             TickUntil(
                 [Bridge, WaystoneId, PreservePlan]()
                 {
                     const echoes::sim::Entity* Waystone =
                         Bridge->FindEntity(WaystoneId);
                     if (Waystone == nullptr)
                     {
                         return false;
                     }
                     const int32 Dx = FMath::Abs(
                         Waystone->position.x.Raw() -
                         PreservePlan.WaystoneAnchor.x.Raw());
                     const int32 Dy = FMath::Abs(
                         Waystone->position.y.Raw() -
                         PreservePlan.WaystoneAnchor.y.Raw());
                     return Dx <= echoes::sim::kFixedScale / 2 &&
                            Dy <= echoes::sim::kFixedScale / 2;
                 },
                 2600));
    TestTrue(TEXT("The listening anchor accepts a root order"),
             Bridge->IssueCommand(
                 echoes::sim::CommandType::ToggleWaystoneRoot,
                 WaystoneId,
                 0,
                 FVector::ZeroVector,
                 echoes::sim::FutureWellChoice::Dormant,
                 Feedback));
    TestTrue(TEXT("Rooting opens Listening Spine construction"),
             TickUntil(
                 [Bridge]()
                 {
                     return Bridge->GetShapeOfSilencePhase() ==
                         EEchoesShapeOfSilencePhase::RaiseListeningSpine;
                 },
                 400));
    TestTrue(TEXT("An ordinary worker accepts the Listening Spine build"),
             Bridge->IssueBuildCommand(
                 WorkerId,
                 echoes::sim::EntityType::UtilityStructure,
                 Bridge->SimToWorld(PreservePlan.ListeningSpineSite),
                 Feedback));
    TestTrue(TEXT("Completed construction opens paired witnessing"),
             TickUntil(
                 [Bridge]()
                 {
                     return Bridge->GetShapeOfSilencePhase() ==
                         EEchoesShapeOfSilencePhase::PositionMemoryWitnesses;
                 },
                 3200));
    TestTrue(TEXT("The first witness accepts its observation site"),
             Bridge->IssueCommand(
                 echoes::sim::CommandType::Move,
                 Start.FirstMemoryWitnessId,
                 0,
                 Bridge->SimToWorld(PreservePlan.FirstWitnessSite),
                 echoes::sim::FutureWellChoice::Dormant,
                 Feedback));
    TestTrue(TEXT("The second witness accepts its observation site"),
             Bridge->IssueCommand(
                 echoes::sim::CommandType::Move,
                 Start.SecondMemoryWitnessId,
                 0,
                 Bridge->SimToWorld(PreservePlan.SecondWitnessSite),
                 echoes::sim::FutureWellChoice::Dormant,
                 Feedback));
    TestTrue(TEXT("Both ordinary movements open Oruun's confluence"),
             TickUntil(
                 [Bridge]()
                 {
                     return Bridge->GetShapeOfSilencePhase() ==
                         EEchoesShapeOfSilencePhase::ReachConfluence;
                 },
                 3400));
    TestTrue(TEXT("Oruun accepts the confluence order"),
             Bridge->IssueCommand(
                 echoes::sim::CommandType::Move,
                 Start.OruunId,
                 0,
                 Bridge->SimToWorld(PreservePlan.ConfluenceSite),
                 echoes::sim::FutureWellChoice::Dormant,
                 Feedback));
    TestTrue(TEXT("The ordinary mission path commits the seventh ledger record"),
             TickUntil(
                 [Bridge]()
                 {
                     return Bridge->GetCampaignProgress().FindDecision(
                                EEchoesCampaignMissionId::TheShapeOfSilence) !=
                         nullptr;
                 },
                 3400));
    const FEchoesCampaignDecisionRecord* MissionRecord =
        Bridge->GetCampaignProgress().FindDecision(
            EEchoesCampaignMissionId::TheShapeOfSilence);
    TestTrue(TEXT("The seventh record preserves branch and all verified facts"),
             MissionRecord != nullptr &&
                 MissionRecord->WellChoice ==
                     echoes::sim::FutureWellChoice::Preserve &&
                 MissionRecord->AvailableWellChoices == (1 << 1) &&
                 MissionRecord->VerifiedFacts == 0x3F);
    FEchoesCampaignProgress Reloaded;
    TestTrue(TEXT("The seven-record campaign reloads transactionally"),
             FEchoesCampaignProgressStore::LoadWithBackup(
                 CampaignPath, Reloaded, Feedback) &&
                 Reloaded.Decisions.Num() == 7);

    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return true;
}

#endif
