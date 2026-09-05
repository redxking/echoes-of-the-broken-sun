#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "EchoesCampaignProgress.h"
#include "EchoesCampaignMapCheckpoint.h"
#include "EchoesSnapshotMigrationTestHelpers.h"
#include "EchoesSeveralVoicesOneCommandMissionModel.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Tests/AutomationCommon.h"

namespace
{
struct FPreservedSeveralVoicesFile final
{
    explicit FPreservedSeveralVoicesFile(FString InPath)
        : Path(MoveTemp(InPath))
    {
        bExisted = IFileManager::Get().FileExists(*Path);
        if (bExisted)
        {
            FFileHelper::LoadFileToArray(Contents, *Path);
        }
    }

    ~FPreservedSeveralVoicesFile()
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

uint8 SeveralVoicesChoiceMask(echoes::sim::FutureWellChoice Choice)
{
    switch (Choice)
    {
        case echoes::sim::FutureWellChoice::Harvest: return 1 << 0;
        case echoes::sim::FutureWellChoice::Preserve: return 1 << 1;
        case echoes::sim::FutureWellChoice::Reshape: return 1 << 2;
        default: return 0;
    }
}

FEchoesCampaignDecisionRecord MakeSeveralVoicesRecord(
    EEchoesCampaignMissionId Mission,
    echoes::sim::FutureWellChoice FoundingChoice,
    echoes::sim::FutureWellChoice LumeChoice,
    uint8 ReserveFacts = 0x7B)
{
    FEchoesCampaignDecisionRecord Record;
    Record.Mission = Mission;
    const bool bLumeReceipt =
        Mission == EEchoesCampaignMissionId::ChoirAtLumeReach ||
        Mission == EEchoesCampaignMissionId::NoNeutralLedger ||
        Mission == EEchoesCampaignMissionId::TheFutureThatWon ||
        Mission == EEchoesCampaignMissionId::AssemblyOfTheMissing ||
        Mission == EEchoesCampaignMissionId::SeveralVoicesOneCommand;
    Record.WellChoice = bLumeReceipt ? LumeChoice : FoundingChoice;
    Record.AvailableWellChoices =
        Mission == EEchoesCampaignMissionId::WhatTheLedgerKeeps ||
                Mission == EEchoesCampaignMissionId::ChoirAtLumeReach
            ? 0x07
        : bLumeReceipt
            ? SeveralVoicesChoiceMask(LumeChoice)
            : SeveralVoicesChoiceMask(FoundingChoice);
    Record.SimulationSnapshotVersion = echoes::sim::kSnapshotVersion;
    Record.CompletionTick = 1200 + static_cast<uint8>(Mission) * 400;
    Record.FinalStateChecksum =
        0xB14C0000ULL + static_cast<uint8>(Mission);
    switch (Mission)
    {
        case EEchoesCampaignMissionId::WhatTheLedgerKeeps:
        case EEchoesCampaignMissionId::SevenAccountsOfRain:
            Record.VerifiedFacts = 0x0F;
            break;
        case EEchoesCampaignMissionId::ACityOnReserve:
        case EEchoesCampaignMissionId::TheUnburiedRoad:
            Record.VerifiedFacts = 0x1F;
            break;
        case EEchoesCampaignMissionId::TermsOfContinuance:
        case EEchoesCampaignMissionId::NamesWithoutBirths:
        case EEchoesCampaignMissionId::TheShapeOfSilence:
        case EEchoesCampaignMissionId::TheShapeBesideUs:
            Record.VerifiedFacts = 0x3F;
            break;
        case EEchoesCampaignMissionId::ReserveAuthority:
            Record.VerifiedFacts = ReserveFacts;
            break;
        case EEchoesCampaignMissionId::ChoirAtLumeReach:
        case EEchoesCampaignMissionId::NoNeutralLedger:
        case EEchoesCampaignMissionId::TheFutureThatWon:
        case EEchoesCampaignMissionId::AssemblyOfTheMissing:
        case EEchoesCampaignMissionId::SeveralVoicesOneCommand:
            Record.VerifiedFacts = 0xFF;
            break;
    }
    return Record;
}

FEchoesCampaignProgress MakeSeveralVoicesPrerequisites(
    echoes::sim::FutureWellChoice FoundingChoice,
    echoes::sim::FutureWellChoice LumeChoice,
    uint8 ReserveFacts,
    int32 ThroughMission,
    FString& OutFeedback)
{
    FEchoesCampaignProgress Progress;
    for (int32 MissionValue = 1;
         MissionValue <= FMath::Min(ThroughMission, 13);
         ++MissionValue)
    {
        Progress.AppendDecision(
            MakeSeveralVoicesRecord(
                static_cast<EEchoesCampaignMissionId>(MissionValue),
                FoundingChoice,
                LumeChoice,
                ReserveFacts),
            OutFeedback);
    }
    return Progress;
}

FString SeveralVoicesQuickSavePath(
    const FEchoesCampaignProgress& Progress)
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
            TEXT("EchoesQuickSaveSeveralVoicesOneCommand-%08X.bin"),
            Fingerprint));
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesSeveralVoicesOneCommandMissionTest,
    "Echoes.Runtime.Campaign.SeveralVoicesOneCommand",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesSeveralVoicesOneCommandMissionTest::RunTest(
    const FString& Parameters)
{
    (void)Parameters;

    FEchoesScopedTestSaveEnvironment TestSaveEnvironment(*this);
    if (!TestSaveEnvironment.IsReady())
    {
        return false;
    }
    using echoes::sim::ChoirIdentityState;
    using echoes::sim::CommandType;
    using echoes::sim::EntityType;
    using echoes::sim::Faction;
    using echoes::sim::FutureWellChoice;
    using echoes::sim::ResearchType;
    using echoes::sim::Vec2;

    const FutureWellChoice Choices[] = {
        FutureWellChoice::Harvest,
        FutureWellChoice::Preserve,
        FutureWellChoice::Reshape};
    const uint8 ReservePairs[] = {0x7E, 0x7D, 0x7B};
    TSet<uint8> PlanKeys;
    int32 PlanContracts = 0;
    for (const FutureWellChoice FoundingChoice : Choices)
    {
        for (const uint8 ReserveFacts : ReservePairs)
        {
            for (const FutureWellChoice Protocol : Choices)
            {
                FEchoesSeveralVoicesOneCommandPlan Plan;
                const bool bPlanned =
                    FEchoesSeveralVoicesOneCommandMissionModel::
                        TryPlanForLedger(
                            FoundingChoice,
                            ReserveFacts,
                            Protocol,
                            Plan);
                TestTrue(
                    TEXT("Every accepted inherited tuple has one bounded Choir crisis plan"),
                    bPlanned &&
                        Plan.PossibleVoiceSite != Plan.ManifestVoiceSite &&
                        Plan.NemeCommandSite != Vec2{} &&
                        Plan.CrisisAnchorSite != Vec2{});
                PlanKeys.Add(Plan.StablePlanKey);
                ++PlanContracts;
            }
        }
    }
    TestEqual(TEXT("The inherited projection retains 27 valid plans"),
              PlanContracts, 27);
    TestEqual(TEXT("All inherited Choir plans retain unique stable keys"),
              PlanKeys.Num(), 27);
    FEchoesSeveralVoicesOneCommandPlan InvalidPlan;
    TestFalse(
        TEXT("A malformed reserve allocation cannot define Mission 14"),
        FEchoesSeveralVoicesOneCommandMissionModel::TryPlanForLedger(
            FutureWellChoice::Harvest,
            0x79,
            FutureWellChoice::Preserve,
            InvalidPlan));
    TestFalse(
        TEXT("A dormant recorded protocol cannot define Mission 14"),
        FEchoesSeveralVoicesOneCommandMissionModel::TryPlanForLedger(
            FutureWellChoice::Harvest,
            0x7B,
            FutureWellChoice::Dormant,
            InvalidPlan));

    FEchoesSeveralVoicesOneCommandMissionFacts Facts;
    TestTrue(
        TEXT("Inactive facts stay outside Mission 14"),
        FEchoesSeveralVoicesOneCommandMissionModel::DeterminePhase(Facts) ==
            EEchoesSeveralVoicesOneCommandPhase::Inactive);
    Facts.bOperationActive = true;
    Facts.bLocalCoreIntact = true;
    Facts.bPossibleVoiceIntact = true;
    Facts.bManifestVoiceIntact = true;
    Facts.bNemeIntact = true;
    Facts.bResearchLoomIntact = true;
    Facts.bSkirmishStillOngoing = true;
    TestTrue(
        TEXT("Mission 14 begins by researching Held Alternatives"),
        FEchoesSeveralVoicesOneCommandMissionModel::DeterminePhase(Facts) ==
            EEchoesSeveralVoicesOneCommandPhase::ResearchHeldAlternatives);
    Facts.bHeldAlternativesResearched = true;
    TestTrue(
        TEXT("Held Alternatives opens incompatible voice resolution"),
        FEchoesSeveralVoicesOneCommandMissionModel::DeterminePhase(Facts) ==
            EEchoesSeveralVoicesOneCommandPhase::ResolveIncompatibleVoices);
    Facts.bPossibleVoiceResolved = true;
    Facts.bPossibleVoiceAtSite = true;
    Facts.bManifestVoiceResolved = true;
    Facts.bManifestVoiceAtSite = true;
    Facts.bNemeAtCommandSite = true;
    TestTrue(
        TEXT("Resolved voices open Shared Resolution research"),
        FEchoesSeveralVoicesOneCommandMissionModel::DeterminePhase(Facts) ==
            EEchoesSeveralVoicesOneCommandPhase::ResearchSharedResolution);
    Facts.bSharedResolutionResearched = true;
    TestTrue(
        TEXT("Shared Resolution opens the bounded crisis anchor"),
        FEchoesSeveralVoicesOneCommandMissionModel::DeterminePhase(Facts) ==
            EEchoesSeveralVoicesOneCommandPhase::AnchorCrisis);
    Facts.bPhaseAnchorComplete = true;
    TestTrue(
        TEXT("The completed Phase Anchor starts the visible hold"),
        FEchoesSeveralVoicesOneCommandMissionModel::DeterminePhase(Facts) ==
            EEchoesSeveralVoicesOneCommandPhase::HoldSharedResolution);
    Facts.bCrisisWindowHeld = true;
    TestTrue(
        TEXT("Holding the complete contract resolves Mission 14"),
        FEchoesSeveralVoicesOneCommandMissionModel::DeterminePhase(Facts) ==
            EEchoesSeveralVoicesOneCommandPhase::Complete);
    Facts.bCrisisContractFailed = true;
    TestTrue(
        TEXT("A latched hold violation overrides repaired completion facts"),
        FEchoesSeveralVoicesOneCommandMissionModel::DeterminePhase(Facts) ==
            EEchoesSeveralVoicesOneCommandPhase::Failed);
    Facts.bCrisisContractFailed = false;
    Facts.bPossibleVoiceIntact = false;
    TestTrue(
        TEXT("Protected voice loss fails closed"),
        FEchoesSeveralVoicesOneCommandMissionModel::DeterminePhase(Facts) ==
            EEchoesSeveralVoicesOneCommandPhase::Failed);
    Facts.bPossibleVoiceIntact = true;
    Facts.bSkirmishStillOngoing = false;
    TestTrue(
        TEXT("A terminal match fails the mission contract"),
        FEchoesSeveralVoicesOneCommandMissionModel::DeterminePhase(Facts) ==
            EEchoesSeveralVoicesOneCommandPhase::Failed);
    Facts = {};
    Facts.bOperationActive = true;
    Facts.bLocalCoreIntact = true;
    Facts.bPossibleVoiceIntact = true;
    Facts.bManifestVoiceIntact = true;
    Facts.bNemeIntact = true;
    Facts.bResearchLoomIntact = true;
    Facts.bSkirmishStillOngoing = true;
    Facts.bPhaseAnchorComplete = true;
    TestTrue(
        TEXT("An illicit early Phase Anchor fails closed"),
        FEchoesSeveralVoicesOneCommandMissionModel::DeterminePhase(Facts) ==
            EEchoesSeveralVoicesOneCommandPhase::Failed);

    TestEqual(TEXT("Mission 14 uses the current campaign schema"),
              FEchoesCampaignProgress::SchemaVersion,
              static_cast<uint16>(2));
    // Per-player terrain and object memory is now serialized into the
    // snapshot, so the native schema moved from 24 to 25. The replay
    // envelope shape did not change and stays at 24.
    TestEqual(TEXT("Mission 14 writes native snapshot schema 26"),
              echoes::sim::kSnapshotVersion,
              static_cast<uint32>(26));

    FString Feedback;
    FEchoesCampaignProgress ThirteenRecords =
        MakeSeveralVoicesPrerequisites(
            FutureWellChoice::Harvest,
            FutureWellChoice::Preserve,
            0x7B,
            13,
            Feedback);
    TestEqual(TEXT("The accepted prerequisite contains exactly 13 records"),
              ThirteenRecords.Decisions.Num(), 13);
    FEchoesCampaignDecisionRecord InvalidVoices = MakeSeveralVoicesRecord(
        EEchoesCampaignMissionId::SeveralVoicesOneCommand,
        FutureWellChoice::Harvest,
        FutureWellChoice::Preserve);
    InvalidVoices.VerifiedFacts = 0xFE;
    FEchoesCampaignProgress InvalidProgress = ThirteenRecords;
    TestTrue(
        TEXT("Mission 14 rejects any missing completion fact"),
        InvalidProgress.AppendDecision(InvalidVoices, Feedback) ==
            EEchoesCampaignCommitStatus::StorageFailure);
    InvalidVoices = MakeSeveralVoicesRecord(
        EEchoesCampaignMissionId::SeveralVoicesOneCommand,
        FutureWellChoice::Harvest,
        FutureWellChoice::Preserve);
    InvalidVoices.SimulationSnapshotVersion = 21;
    TestTrue(
        TEXT("Mission 14 rejects non-native Choir provenance"),
        InvalidProgress.AppendDecision(InvalidVoices, Feedback) ==
                EEchoesCampaignCommitStatus::StorageFailure &&
            Feedback.Contains(TEXT("schema 22")));
    InvalidVoices = MakeSeveralVoicesRecord(
        EEchoesCampaignMissionId::SeveralVoicesOneCommand,
        FutureWellChoice::Harvest,
        FutureWellChoice::Harvest);
    TestTrue(
        TEXT("Mission 14 cannot rewrite the Mission 13 protocol"),
        InvalidProgress.AppendDecision(InvalidVoices, Feedback) ==
                EEchoesCampaignCommitStatus::StorageFailure &&
            Feedback.Contains(TEXT("CAMPAIGN_VOICES_LUME_PROTOCOL")));

    const FEchoesCampaignDecisionRecord ValidVoices =
        MakeSeveralVoicesRecord(
            EEchoesCampaignMissionId::SeveralVoicesOneCommand,
            FutureWellChoice::Harvest,
            FutureWellChoice::Preserve);
    FEchoesCampaignProgress SyntheticComplete = ThirteenRecords;
    TestTrue(
        TEXT("An exact 13-record chain accepts one Choir-command receipt"),
        SyntheticComplete.AppendDecision(ValidVoices, Feedback) ==
            EEchoesCampaignCommitStatus::Added);
    TArray<uint8> EncodedComplete;
    FEchoesCampaignProgress DecodedComplete;
    TestTrue(
        TEXT("The 14-record chain round-trips transactionally"),
        FEchoesCampaignProgressStore::Encode(
            SyntheticComplete, EncodedComplete, Feedback) &&
        FEchoesCampaignProgressStore::Decode(
            EncodedComplete, DecodedComplete, Feedback) &&
        DecodedComplete.Decisions.Num() == 14 &&
        DecodedComplete.Decisions[13].VerifiedFacts == 0xFF);
    TestTrue(
        TEXT("An identical Mission 14 replay is idempotent"),
        SyntheticComplete.AppendDecision(ValidVoices, Feedback) ==
            EEchoesCampaignCommitStatus::AlreadyRecorded);
    FEchoesCampaignDecisionRecord Conflict = ValidVoices;
    Conflict.WellChoice = FutureWellChoice::Harvest;
    Conflict.AvailableWellChoices =
        SeveralVoicesChoiceMask(FutureWellChoice::Harvest);
    TestTrue(
        TEXT("A divergent Mission 14 replay is refused without rewrite"),
        SyntheticComplete.AppendDecision(Conflict, Feedback) ==
            EEchoesCampaignCommitStatus::ReplayConflict);

    FEchoesCampaignProgress Reordered = ThirteenRecords;
    Swap(Reordered.Decisions[11], Reordered.Decisions[12]);
    TestTrue(
        TEXT("A reordered M12-M13 chain cannot admit Mission 14"),
        Reordered.AppendDecision(ValidVoices, Feedback) ==
                EEchoesCampaignCommitStatus::StorageFailure &&
            Feedback.Contains(TEXT("CAMPAIGN_VOICES_LEDGER_ORDER")));

    const FString CampaignPath =
        FEchoesCampaignProgressStore::GetDefaultPath();
    FPreservedSeveralVoicesFile PreservedCampaign(CampaignPath);
    FPreservedSeveralVoicesFile PreservedCampaignBackup(
        CampaignPath + TEXT(".bak"));
    FPreservedSeveralVoicesFile PreservedCampaignTemporary(
        CampaignPath + TEXT(".tmp"));
    for (const FString& Path : {
             CampaignPath,
             CampaignPath + TEXT(".bak"),
             CampaignPath + TEXT(".tmp")})
    {
        IFileManager::Get().Delete(*Path, false, true, true);
    }

    FEchoesCampaignProgress TwelveRecords =
        MakeSeveralVoicesPrerequisites(
            FutureWellChoice::Harvest,
            FutureWellChoice::Preserve,
            0x7B,
            12,
            Feedback);
    TestTrue(
        TEXT("The 12-record lock fixture is stored"),
        FEchoesCampaignProgressStore::SaveAtomic(
            CampaignPath, TwelveRecords, Feedback));
    {
        FTestWorldWrapper LockedWorld;
        if (!LockedWorld.CreateTestWorld(EWorldType::Game))
        {
            LockedWorld.ForwardErrorMessages(this);
            AddError(TEXT("Could not create the locked Mission 14 world."));
            return false;
        }
        UEchoesSimulationSubsystem* LockedBridge =
            LockedWorld.GetTestWorld()->GetSubsystem<
                UEchoesSimulationSubsystem>();
        TestFalse(
            TEXT("Mission 14 rejects a ledger without Mission 13"),
            LockedBridge != nullptr &&
                LockedBridge->SelectOperationMode(
                    EEchoesOperationMode::CampaignSeveralVoicesOneCommand,
                    Feedback));
        TestTrue(TEXT("The lock response names Assembly of the Missing"),
                 Feedback.Contains(TEXT("Assembly of the Missing")));
        LockedWorld.ForwardErrorMessages(this);
    }

    const FString QuickSavePath =
        SeveralVoicesQuickSavePath(ThirteenRecords);
    FPreservedSeveralVoicesFile PreservedQuickSave(QuickSavePath);
    FPreservedSeveralVoicesFile PreservedQuickSaveBackup(
        QuickSavePath + TEXT(".bak"));
    FPreservedSeveralVoicesFile PreservedQuickSaveStagedBackup(
        QuickSavePath + TEXT(".bak.tmp"));
    FPreservedSeveralVoicesFile PreservedQuickSaveTemporary(
        QuickSavePath + TEXT(".tmp"));
    for (const FString& Path : {
             QuickSavePath,
             QuickSavePath + TEXT(".bak"),
             QuickSavePath + TEXT(".bak.tmp"),
             QuickSavePath + TEXT(".tmp")})
    {
        IFileManager::Get().Delete(*Path, false, true, true);
    }
    TestTrue(
        TEXT("The exact 13-record campaign is stored"),
        FEchoesCampaignProgressStore::SaveAtomic(
            CampaignPath, ThirteenRecords, Feedback));

    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the Mission 14 test world."));
        return false;
    }
    UEchoesSimulationSubsystem* Bridge =
        WorldWrapper.GetTestWorld()->GetSubsystem<
            UEchoesSimulationSubsystem>();
    if (!TestNotNull(TEXT("Mission 14 owns a simulation subsystem"), Bridge) ||
        !TestTrue(TEXT("Thirteen exact records unlock Mission 14"),
                  Bridge != nullptr &&
                      Bridge->IsSeveralVoicesOneCommandUnlocked()) ||
        !TestTrue(TEXT("Mission 14 can be selected"),
                  Bridge->SelectOperationMode(
                      EEchoesOperationMode::CampaignSeveralVoicesOneCommand,
                      Feedback)) ||
        !TestTrue(TEXT("Mission 14 can start"),
                  Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const FEchoesSeveralVoicesOneCommandPlan Plan =
        Bridge->GetSeveralVoicesOneCommandPlan();
    FEchoesObjectiveSnapshot Objective =
        Bridge->GetLocalObjectiveSnapshot();
    const echoes::sim::Entity* PossibleVoice =
        Bridge->FindEntity(Objective.SeveralVoicesPossibleVoiceId);
    const echoes::sim::Entity* ManifestVoice =
        Bridge->FindEntity(Objective.SeveralVoicesManifestVoiceId);
    const echoes::sim::Entity* Neme =
        Bridge->FindEntity(Objective.SeveralVoicesNemeId);
    const echoes::sim::Entity* ResearchLoom =
        Bridge->FindEntity(Objective.SeveralVoicesResearchLoomId);
    TestTrue(
        TEXT("Mission 14 grants only a protected local Hollow Choir force"),
        PossibleVoice != nullptr && ManifestVoice != nullptr &&
            Neme != nullptr && ResearchLoom != nullptr &&
            PossibleVoice->owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            ManifestVoice->owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            Neme->owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            ResearchLoom->owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            PossibleVoice->faction == Faction::HollowChoir &&
            ManifestVoice->faction == Faction::HollowChoir &&
            Neme->faction == Faction::HollowChoir &&
            ResearchLoom->faction == Faction::HollowChoir &&
            PossibleVoice->type == EntityType::Soldier &&
            ManifestVoice->type == EntityType::HeavyUnit &&
            Neme->type == EntityType::ScoutUnit &&
            ResearchLoom->type == EntityType::Barracks &&
            PossibleVoice->choirIdentityState == ChoirIdentityState::Manifest &&
            ManifestVoice->choirIdentityState == ChoirIdentityState::Manifest);
    TestTrue(
        TEXT("Mission 14 inherits the accepted Preserve protocol and distinct sites"),
        Plan.RecordedProtocol == FutureWellChoice::Preserve &&
            Plan.PossibleVoiceSite != Plan.ManifestVoiceSite &&
            Plan.NemeCommandSite != Vec2{} && Plan.CrisisAnchorSite != Vec2{});
    TestTrue(
        TEXT("Mission 14 begins at Held Alternatives research"),
        Bridge->GetSeveralVoicesOneCommandPhase() ==
            EEchoesSeveralVoicesOneCommandPhase::ResearchHeldAlternatives);

    TArray<echoes::sim::EntityId> Workers;
    for (const echoes::sim::Entity& Entity :
         Bridge->GetSimulation()->Entities())
    {
        if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            Entity.faction == Faction::HollowChoir &&
            Entity.type == EntityType::Worker)
        {
            Workers.Add(Entity.id);
        }
    }
    if (!TestTrue(TEXT("A Hollow Choir construction worker is available"),
                  !Workers.IsEmpty()))
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
    TestFalse(
        TEXT("A protected voice cannot resolve before Held Alternatives"),
        Bridge->IssueChoirReconciliation(
            Objective.SeveralVoicesPossibleVoiceId,
            ChoirIdentityState::Possible,
            Feedback));
    TestTrue(TEXT("The early reconciliation rejection is reason-coded"),
             Feedback.Contains(TEXT("CHOIR_HELD_ALTERNATIVES_REQUIRED")));
    TestFalse(
        TEXT("A Phase Anchor cannot be raised before the crisis contract"),
        Bridge->IssueBuildCommand(
            Workers[0],
            EntityType::UtilityStructure,
            Bridge->SimToWorld(Vec2::FromTiles(46, 35)),
            Feedback));
    TestTrue(TEXT("The early anchor rejection is reason-coded"),
             Feedback.Contains(TEXT("CHOIR_CRISIS_CONTRACT_REQUIRED")));
    TestTrue(
        TEXT("The Research Loom accepts Held Alternatives"),
        Bridge->IssueResearchCommand(
            Objective.SeveralVoicesResearchLoomId,
            ResearchType::ChoirHeldAlternatives,
            Feedback));
    TestTrue(
        TEXT("Held Alternatives completes deterministically"),
        TickUntil(
            [Bridge]()
            {
                return Bridge->GetSeveralVoicesOneCommandPhase() ==
                    EEchoesSeveralVoicesOneCommandPhase::
                        ResolveIncompatibleVoices;
            },
            1000));
    TestFalse(
        TEXT("Shared Resolution cannot bypass the unresolved voice contract"),
        Bridge->IssueResearchCommand(
            Objective.SeveralVoicesResearchLoomId,
            ResearchType::ChoirSharedResolution,
            Feedback));
    TestTrue(
        TEXT("The out-of-order Shared Resolution rejection is reason-coded"),
        Feedback.Contains(TEXT("CHOIR_VOICE_CONTRACT_REQUIRED")));
    TestFalse(
        TEXT("A non-identity worker cannot receive reconciliation"),
        Bridge->IssueChoirReconciliation(
            Workers[0], ChoirIdentityState::Possible, Feedback));
    TestTrue(TEXT("The invalid identity actor is reason-coded"),
             Feedback.Contains(TEXT("CHOIR_IDENTITY_REQUIRED")));

    TestTrue(
        TEXT("The Possible voice accepts its inherited witness site"),
        Bridge->IssueCommand(
            CommandType::Move,
            Objective.SeveralVoicesPossibleVoiceId,
            0,
            Bridge->SimToWorld(Plan.PossibleVoiceSite),
            FutureWellChoice::Dormant,
            Feedback));
    TestTrue(
        TEXT("The Manifest voice accepts its separate inherited site"),
        Bridge->IssueCommand(
            CommandType::Move,
            Objective.SeveralVoicesManifestVoiceId,
            0,
            Bridge->SimToWorld(Plan.ManifestVoiceSite),
            FutureWellChoice::Dormant,
            Feedback));
    TestTrue(
        TEXT("Neme accepts the inherited public command site"),
        Bridge->IssueCommand(
            CommandType::Move,
            Objective.SeveralVoicesNemeId,
            0,
            Bridge->SimToWorld(Plan.NemeCommandSite),
            FutureWellChoice::Dormant,
            Feedback));
    const auto NativeReconciliationSequence =
        Bridge->GetSimulation()->NextCommandSequence(
            UEchoesSimulationSubsystem::LocalPlayerId);
    TestTrue(
        TEXT("The schema-26 reconciliation has a stable receipt sequence"),
        NativeReconciliationSequence.has_value());
    TestTrue(
        TEXT("The protected Soldier accepts Possible resolution"),
        Bridge->IssueChoirReconciliation(
            Objective.SeveralVoicesPossibleVoiceId,
            ChoirIdentityState::Possible,
            Feedback));
    TestTrue(
        TEXT("The queued reconciliation enters its timed dual state"),
        TickUntil(
            [Bridge]()
            {
                return Bridge->GetLocalObjectiveSnapshot().
                           SeveralVoicesPossibleState ==
                    ChoirIdentityState::DualResolvePossible;
            },
            4));
    Objective = Bridge->GetLocalObjectiveSnapshot();
    const uint64 InitialResolveRemaining =
        Objective.SeveralVoicesPossibleResolveTicksRemaining;
    TestTrue(
        TEXT("The deployed tracker exposes the active Possible duration"),
        Objective.SeveralVoicesPossibleState ==
                ChoirIdentityState::DualResolvePossible &&
            InitialResolveRemaining > 0 && InitialResolveRemaining <= 160);
    TestTrue(
        TEXT("The native schema-26 source retains the reconciliation receipt"),
        NativeReconciliationSequence.has_value() &&
            Bridge->GetSimulation()->FindCommandResolutionReceipt(
                UEchoesSimulationSubsystem::LocalPlayerId,
                *NativeReconciliationSequence)
                .has_value());

    TestTrue(
        TEXT("The resolving identity writes a native bound checkpoint"),
        Bridge->QuickSaveScenario(Feedback) &&
            IFileManager::Get().FileExists(*QuickSavePath));
    const uint64 V22ExpectedTick = Bridge->GetSimulation()->CurrentTick();
    const auto V22ExpectedNextCommandSequence =
        Bridge->GetSimulation()->NextCommandSequence(
            UEchoesSimulationSubsystem::LocalPlayerId);
    const FEchoesObjectiveSnapshot V22ExpectedObjective = Objective;
    TArray<uint8> NativeCheckpoint;
    TArray<uint8> NativeMapEnvelope;
    FEchoesCampaignMapCheckpointIdentity MapIdentity;
    EEchoesCampaignMapCheckpointFailure MapFailure{};
    EchoesSnapshotMigrationTestHelpers::FEmbeddedSnapshotLayout
        NativeLayout;
    TestTrue(
        TEXT("The Mission 14 schema-26 checkpoint exposes a bounded nonzero receipt block"),
        FFileHelper::LoadFileToArray(NativeMapEnvelope, *QuickSavePath) &&
            FEchoesCampaignMapCheckpoint::Inspect(NativeMapEnvelope, MapIdentity, NativeCheckpoint, MapFailure) &&
            EchoesSnapshotMigrationTestHelpers::
                InspectMission14EnvelopeSnapshot(
                    NativeCheckpoint, NativeLayout) &&
            NativeLayout.ReceiptCount > 0U &&
            NativeLayout.ReceiptBlockSize ==
                4 + static_cast<int32>(NativeLayout.ReceiptCount) * 19);
    // The schema-25 memory ledgers are measured against this mission's own map,
    // not taken on the inspector's word: four remembered-terrain grids of
    // exactly the live tile count, then one bounded object ledger per player.
    const int32 Mission14MapTiles =
        Bridge->GetSimulation()->Config().mapWidthTiles *
        Bridge->GetSimulation()->Config().mapHeightTiles;
    TestTrue(
        TEXT("The Mission 14 checkpoint carries one terrain memory per player and a bounded object memory"),
        Mission14MapTiles > 0 &&
            NativeLayout.RememberedTileCount ==
                static_cast<uint32>(Mission14MapTiles) &&
            NativeLayout.MemoryLedgerOffset > NativeLayout.SnapshotOffset &&
            NativeLayout.MemoryLedgerOffset + NativeLayout.MemoryLedgerSize <=
                NativeLayout.ReceiptBlockOffset &&
            NativeLayout.MemoryLedgerSize ==
                static_cast<int32>(echoes::sim::kMaximumPlayers) *
                        (4 + Mission14MapTiles) +
                    static_cast<int32>(echoes::sim::kMaximumPlayers) * 4 +
                    static_cast<int32>(NativeLayout.RememberedObjectCount) *
                        24 &&
            NativeLayout.RememberedObjectCount <=
                static_cast<uint32>(echoes::sim::kMaximumPlayers) *
                    static_cast<uint32>(
                        echoes::sim::kMaximumRememberedObjects));
    TArray<
        EchoesSnapshotMigrationTestHelpers::FEmbeddedSnapshotLayout>
        AmbiguousCandidates;
    AmbiguousCandidates.Add(NativeLayout);
    EchoesSnapshotMigrationTestHelpers::FEmbeddedSnapshotLayout
        DistinctAlternative = NativeLayout;
    ++DistinctAlternative.ReceiptCount;
    DistinctAlternative.ReceiptBlockOffset -= 19;
    DistinctAlternative.ReceiptBlockSize += 19;
    AmbiguousCandidates.Add(DistinctAlternative);
    EchoesSnapshotMigrationTestHelpers::FEmbeddedSnapshotLayout
        AmbiguousResult = NativeLayout;
    TestTrue(
        TEXT("Receipt discovery rejects two candidate layouts"),
        !EchoesSnapshotMigrationTestHelpers::SelectUniqueReceiptCandidate(
            AmbiguousCandidates, AmbiguousResult) &&
            AmbiguousResult.SnapshotOffset == INDEX_NONE &&
            AmbiguousResult.ReceiptBlockOffset == INDEX_NONE);
    const auto RejectsMission14ConversionWithoutMutation = [](
        TArray<uint8> Candidate)
    {
        const TArray<uint8> Before = Candidate;
        return !EchoesSnapshotMigrationTestHelpers::
                    ConvertMission14EnvelopeSnapshotToV22(Candidate) &&
            Candidate == Before;
    };

    TArray<uint8> BadEnvelopeChecksum = NativeCheckpoint;
    if (!BadEnvelopeChecksum.IsEmpty())
    {
        BadEnvelopeChecksum.Last() ^= 0x01;
    }
    TestTrue(
        TEXT("Mission 14 conversion rejects a bad envelope checksum without mutation"),
        RejectsMission14ConversionWithoutMutation(BadEnvelopeChecksum));

    TArray<uint8> WrongOperationEnvelope = NativeCheckpoint;
    if (WrongOperationEnvelope.IsValidIndex(9))
    {
        WrongOperationEnvelope[9] = static_cast<uint8>(
            EEchoesOperationMode::CampaignTheBrokenSun);
        EchoesSnapshotMigrationTestHelpers::UpdateEnvelopeChecksum(
            WrongOperationEnvelope);
    }
    TestTrue(
        TEXT("Mission 14 conversion rejects a cross-operation envelope without mutation"),
        RejectsMission14ConversionWithoutMutation(
            WrongOperationEnvelope));

    TArray<uint8> BadSnapshotIntegrity = NativeCheckpoint;
    if (NativeLayout.SnapshotOffset >= 0 &&
        BadSnapshotIntegrity.IsValidIndex(NativeLayout.SnapshotOffset + 8))
    {
        BadSnapshotIntegrity[NativeLayout.SnapshotOffset + 8] ^= 0x01;
        EchoesSnapshotMigrationTestHelpers::UpdateEnvelopeChecksum(
            BadSnapshotIntegrity);
    }
    TestTrue(
        TEXT("Mission 14 conversion rejects a bad snapshot signature without mutation"),
        RejectsMission14ConversionWithoutMutation(BadSnapshotIntegrity));

    TArray<uint8> ProtectedCoreSnapshot = NativeCheckpoint;
    bool bProtectedCoreSourceLoadable = false;
    if (NativeLayout.SnapshotOffset >= 0 &&
        ProtectedCoreSnapshot.IsValidIndex(NativeLayout.SnapshotOffset + 28))
    {
        ProtectedCoreSnapshot[NativeLayout.SnapshotOffset + 28] = 1U;
        if (EchoesSnapshotMigrationTestHelpers::ResignEmbeddedSnapshot(
                ProtectedCoreSnapshot,
                NativeLayout.SnapshotOffset,
                NativeLayout.SnapshotLength))
        {
            EchoesSnapshotMigrationTestHelpers::UpdateEnvelopeChecksum(
                ProtectedCoreSnapshot);
            // The source here is the checkpoint just written by this run,
            // so it carries the native schema. That native schema is 25
            // now that per-player terrain and object memory is serialized;
            // the replay envelope is unrelated and stays at 24.
            bProtectedCoreSourceLoadable =
                EchoesSnapshotMigrationTestHelpers::
                    IsLoadableEmbeddedSnapshot(
                        ProtectedCoreSnapshot,
                        NativeLayout.SnapshotOffset,
                        NativeLayout.SnapshotLength,
                        25U);
        }
    }
    TestTrue(
        TEXT("Mission 14 conversion rejects lossy protected-Core downgrade without mutation"),
        bProtectedCoreSourceLoadable &&
            RejectsMission14ConversionWithoutMutation(
                ProtectedCoreSnapshot));

    TArray<uint8> OversizedReceiptCount = NativeCheckpoint;
    if (NativeLayout.ReceiptBlockOffset >= 0)
    {
        EchoesSnapshotMigrationTestHelpers::WriteUint32(
            OversizedReceiptCount,
            NativeLayout.ReceiptBlockOffset,
            static_cast<uint32>(
                echoes::sim::kMaximumCommandResolutionReceipts + 1U));
        EchoesSnapshotMigrationTestHelpers::ResignEmbeddedSnapshot(
            OversizedReceiptCount,
            NativeLayout.SnapshotOffset,
            NativeLayout.SnapshotLength);
        EchoesSnapshotMigrationTestHelpers::UpdateEnvelopeChecksum(
            OversizedReceiptCount);
    }
    TestTrue(
        TEXT("Mission 14 conversion rejects an oversized receipt count without mutation"),
        RejectsMission14ConversionWithoutMutation(OversizedReceiptCount));

    TArray<uint8> TruncatedReceiptBlock = NativeCheckpoint;
    if (NativeLayout.ReceiptCount > 0U &&
        TruncatedReceiptBlock.IsValidIndex(
            NativeLayout.ReceiptBlockOffset + 4))
    {
        TruncatedReceiptBlock.RemoveAt(
            NativeLayout.ReceiptBlockOffset + 4,
            1,
            EAllowShrinking::No);
        const uint32 TruncatedSnapshotLength =
            NativeLayout.SnapshotLength - 1U;
        EchoesSnapshotMigrationTestHelpers::WriteUint32(
            TruncatedReceiptBlock, 15, TruncatedSnapshotLength);
        EchoesSnapshotMigrationTestHelpers::ResignEmbeddedSnapshot(
            TruncatedReceiptBlock,
            NativeLayout.SnapshotOffset,
            TruncatedSnapshotLength);
        EchoesSnapshotMigrationTestHelpers::UpdateEnvelopeChecksum(
            TruncatedReceiptBlock);
    }
    TestTrue(
        TEXT("Mission 14 conversion rejects a truncated receipt block without mutation"),
        RejectsMission14ConversionWithoutMutation(TruncatedReceiptBlock));

    TArray<uint8> UnderflowedSnapshotLength = NativeCheckpoint;
    if (UnderflowedSnapshotLength.Num() >= 19)
    {
        EchoesSnapshotMigrationTestHelpers::WriteUint32(
            UnderflowedSnapshotLength, 15, 40U);
        EchoesSnapshotMigrationTestHelpers::UpdateEnvelopeChecksum(
            UnderflowedSnapshotLength);
    }
    TestTrue(
        TEXT("Mission 14 conversion rejects a truncated declared snapshot without mutation"),
        RejectsMission14ConversionWithoutMutation(
            UnderflowedSnapshotLength));

    TArray<uint8> ZeroReceiptNative = NativeCheckpoint;
    if (NativeLayout.ReceiptBlockSize >= 4)
    {
        const int32 ReceiptRecordsSize =
            NativeLayout.ReceiptBlockSize - 4;
        ZeroReceiptNative.RemoveAt(
            NativeLayout.ReceiptBlockOffset + 4,
            ReceiptRecordsSize,
            EAllowShrinking::No);
        const uint32 ZeroReceiptSnapshotLength =
            NativeLayout.SnapshotLength -
            static_cast<uint32>(ReceiptRecordsSize);
        EchoesSnapshotMigrationTestHelpers::WriteUint32(
            ZeroReceiptNative, NativeLayout.ReceiptBlockOffset, 0U);
        EchoesSnapshotMigrationTestHelpers::WriteUint32(
            ZeroReceiptNative, 15, ZeroReceiptSnapshotLength);
        EchoesSnapshotMigrationTestHelpers::ResignEmbeddedSnapshot(
            ZeroReceiptNative,
            NativeLayout.SnapshotOffset,
            ZeroReceiptSnapshotLength);
        EchoesSnapshotMigrationTestHelpers::UpdateEnvelopeChecksum(
            ZeroReceiptNative);
    }
    EchoesSnapshotMigrationTestHelpers::FEmbeddedSnapshotLayout
        ZeroReceiptLayout;
    TestTrue(
        TEXT("Mission 14 exposes a loadable synthetic zero-receipt native checkpoint"),
        EchoesSnapshotMigrationTestHelpers::
                InspectMission14EnvelopeSnapshot(
                    ZeroReceiptNative, ZeroReceiptLayout) &&
            ZeroReceiptLayout.ReceiptCount == 0U &&
            ZeroReceiptLayout.ReceiptBlockSize == 4 &&
            ZeroReceiptLayout.MemoryLedgerSize ==
                NativeLayout.MemoryLedgerSize);
    TArray<uint8> MissingReceiptRecord = ZeroReceiptNative;
    if (ZeroReceiptLayout.ReceiptBlockOffset >= 0)
    {
        EchoesSnapshotMigrationTestHelpers::WriteUint32(
            MissingReceiptRecord,
            ZeroReceiptLayout.ReceiptBlockOffset,
            1U);
        EchoesSnapshotMigrationTestHelpers::ResignEmbeddedSnapshot(
            MissingReceiptRecord,
            ZeroReceiptLayout.SnapshotOffset,
            ZeroReceiptLayout.SnapshotLength);
        EchoesSnapshotMigrationTestHelpers::UpdateEnvelopeChecksum(
            MissingReceiptRecord);
    }
    TestTrue(
        TEXT("Mission 14 conversion rejects count one without a receipt record"),
        RejectsMission14ConversionWithoutMutation(MissingReceiptRecord));
    TArray<uint8> ZeroReceiptV22 = ZeroReceiptNative;
    // With no receipt records left, the only bytes the walk down to schema 22
    // may drop are the empty receipt count, the protection mask, and the
    // schema-25 memory ledgers this map's dimensions already pinned above,
    // plus the separately measured schema-26 work/projectile append.
    TestTrue(
        TEXT("Mission 14 converts zero receipts with the exact schema-26 append, memory-ledger, and five-byte shrink"),
        EchoesSnapshotMigrationTestHelpers::
                ConvertMission14EnvelopeSnapshotToV22(ZeroReceiptV22) &&
            ZeroReceiptNative.Num() - ZeroReceiptV22.Num() ==
                5 + NativeLayout.MemoryLedgerSize +
                    NativeLayout.Schema26AppendSize &&
            EchoesSnapshotMigrationTestHelpers::Mission14SnapshotVersion(
                ZeroReceiptV22) == 22U);

    TArray<uint8> V22Checkpoint = NativeCheckpoint;
    const uint64 ExpectedNativeToV22Shrink = 5ULL +
        static_cast<uint64>(NativeLayout.ReceiptCount) * 19ULL +
        static_cast<uint64>(NativeLayout.MemoryLedgerSize) +
        static_cast<uint64>(NativeLayout.Schema26AppendSize);
    TestTrue(
        TEXT("The Mission 14 checkpoint converts through schemas 24 and 23 to its genuine schema-22 shape"),
        EchoesSnapshotMigrationTestHelpers::
                ConvertMission14EnvelopeSnapshotToV22(V22Checkpoint) &&
            EchoesSnapshotMigrationTestHelpers::Mission14SnapshotVersion(
                V22Checkpoint) == 22U &&
            static_cast<uint64>(NativeCheckpoint.Num() - V22Checkpoint.Num()) ==
                ExpectedNativeToV22Shrink &&
            FMemory::Memcmp(
                NativeCheckpoint.GetData(), V22Checkpoint.GetData(), 15) == 0 &&
            FMemory::Memcmp(
                NativeCheckpoint.GetData() + 19,
                V22Checkpoint.GetData() + 19,
                NativeLayout.SnapshotOffset - 19) == 0 &&
            [&]() { TArray<uint8> Envelope; return FEchoesCampaignMapCheckpoint::Wrap(MapIdentity, V22Checkpoint, Envelope, MapFailure) && FFileHelper::SaveArrayToFile(Envelope, *QuickSavePath); }());
    TestTrue(
        TEXT("The converted Mission 14 primary is the only loadable generation"),
        !IFileManager::Get().FileExists(
            *(QuickSavePath + TEXT(".bak"))) &&
            !IFileManager::Get().FileExists(
                *(QuickSavePath + TEXT(".bak.tmp"))) &&
            !IFileManager::Get().FileExists(
                *(QuickSavePath + TEXT(".tmp"))));
    TestTrue(
        TEXT("Mission 14 loads the genuine schema-22 primary without trailing payload"),
        Bridge->QuickLoadScenario(Feedback) &&
            !Feedback.Contains(TEXT("prior-generation backup")) &&
            !Feedback.Contains(TEXT("staged prior-generation recovery")));
    Objective = Bridge->GetLocalObjectiveSnapshot();
    TestTrue(
        TEXT("Mission 14 schema migration preserves receipt-independent mission state"),
        Bridge->GetSimulation()->CurrentTick() == V22ExpectedTick &&
            Bridge->GetSimulation()->NextCommandSequence(
                UEchoesSimulationSubsystem::LocalPlayerId) ==
                V22ExpectedNextCommandSequence &&
            Bridge->GetSeveralVoicesOneCommandPhase() ==
                EEchoesSeveralVoicesOneCommandPhase::
                    ResolveIncompatibleVoices &&
            Objective.SeveralVoicesPossibleVoiceId ==
                V22ExpectedObjective.SeveralVoicesPossibleVoiceId &&
            Objective.SeveralVoicesManifestVoiceId ==
                V22ExpectedObjective.SeveralVoicesManifestVoiceId &&
            Objective.SeveralVoicesNemeId ==
                V22ExpectedObjective.SeveralVoicesNemeId &&
            Objective.SeveralVoicesResearchLoomId ==
                V22ExpectedObjective.SeveralVoicesResearchLoomId &&
            Objective.SeveralVoicesPossibleState ==
                ChoirIdentityState::DualResolvePossible &&
            Objective.SeveralVoicesPossibleResolveTicksRemaining ==
                InitialResolveRemaining &&
            Objective.bSeveralVoicesHeldAlternativesResearched ==
                V22ExpectedObjective.
                    bSeveralVoicesHeldAlternativesResearched &&
            NativeReconciliationSequence.has_value() &&
            !Bridge->GetSimulation()->FindCommandResolutionReceipt(
                UEchoesSimulationSubsystem::LocalPlayerId,
                *NativeReconciliationSequence)
                .has_value());
    for (int32 TickIndex = 0; TickIndex < 7; ++TickIndex)
    {
        Bridge->Tick(0.05f);
    }
    Objective = Bridge->GetLocalObjectiveSnapshot();
    TestTrue(
        TEXT("The visible identity duration decreases with simulation time"),
        Objective.SeveralVoicesPossibleResolveTicksRemaining > 0 &&
            Objective.SeveralVoicesPossibleResolveTicksRemaining <
                InitialResolveRemaining);
    TestTrue(
        TEXT("A second resolving save retains one prior generation"),
        Bridge->QuickSaveScenario(Feedback) &&
            IFileManager::Get().FileExists(
                *(QuickSavePath + TEXT(".bak"))));
    TArray<uint8> ResavedNativePrimary;
    EchoesSnapshotMigrationTestHelpers::FEmbeddedSnapshotLayout
        ResavedNativeLayout;
    // Resaving writes the native schema, which moved from 24 to 25 because
    // per-player terrain and object memory is now serialized. The retained
    // backup is a genuine migration fixture and stays at schema 22.
    TestTrue(
        TEXT("The legacy-loaded Mission 14 state resaves natively as schema 26"),
        FFileHelper::LoadFileToArray(
            NativeMapEnvelope, *QuickSavePath) &&
            FEchoesCampaignMapCheckpoint::Inspect(NativeMapEnvelope, MapIdentity, ResavedNativePrimary, MapFailure) &&
            EchoesSnapshotMigrationTestHelpers::
                InspectMission14EnvelopeSnapshot(
                    ResavedNativePrimary,
                    ResavedNativeLayout) &&
            EchoesSnapshotMigrationTestHelpers::Mission14SnapshotVersion(
                ResavedNativePrimary) == 26U);
    TArray<uint8> RetainedV22Backup;
    TestTrue(
        TEXT("The first schema-26 resave retains the valid schema-22 Mission 14 generation"),
        FFileHelper::LoadFileToArray(
            NativeMapEnvelope,
            *(QuickSavePath + TEXT(".bak"))) &&
            FEchoesCampaignMapCheckpoint::Inspect(NativeMapEnvelope, MapIdentity, RetainedV22Backup, MapFailure) &&
            EchoesSnapshotMigrationTestHelpers::Mission14SnapshotVersion(
                RetainedV22Backup) == 22U);
    TArray<uint8> CorruptedCheckpoint;
    TestTrue(
        TEXT("The primary Mission 14 envelope can be corrupted for recovery"),
        FFileHelper::LoadFileToArray(
            CorruptedCheckpoint, *QuickSavePath) &&
            CorruptedCheckpoint.Num() > 24);
    if (CorruptedCheckpoint.Num() > 24)
    {
        CorruptedCheckpoint[24] ^= 0x5A;
        TestTrue(TEXT("The corrupted primary is written"),
                 FFileHelper::SaveArrayToFile(
                     CorruptedCheckpoint, *QuickSavePath));
    }
    TestTrue(
        TEXT("Quick load falls back to the bound resolving backup"),
        Bridge->QuickLoadScenario(Feedback));
    Objective = Bridge->GetLocalObjectiveSnapshot();
    TestEqual(
        TEXT("Backup recovery restores the earlier exact identity duration"),
        Objective.SeveralVoicesPossibleResolveTicksRemaining,
        InitialResolveRemaining);

    TestTrue(
        TEXT("Possible, Manifest, and Neme reach their separate contracts"),
        TickUntil(
            [Bridge]()
            {
                return Bridge->GetSeveralVoicesOneCommandPhase() ==
                    EEchoesSeveralVoicesOneCommandPhase::
                        ResearchSharedResolution;
            },
            3000));
    Objective = Bridge->GetLocalObjectiveSnapshot();
    TestTrue(
        TEXT("Both incompatible stable states and all three sites are visible"),
        Objective.SeveralVoicesPossibleState ==
                ChoirIdentityState::Possible &&
            Objective.SeveralVoicesManifestState ==
                ChoirIdentityState::Manifest &&
            Objective.SeveralVoicesPossibleResolveTicksRemaining == 0 &&
            Objective.SeveralVoicesManifestResolveTicksRemaining == 0 &&
            Objective.bSeveralVoicesPossibleAtSite &&
            Objective.bSeveralVoicesManifestAtSite &&
            Objective.bSeveralVoicesNemeAtCommandSite);
    TestTrue(
        TEXT("The Research Loom accepts Shared Resolution"),
        Bridge->IssueResearchCommand(
            Objective.SeveralVoicesResearchLoomId,
            ResearchType::ChoirSharedResolution,
            Feedback));
    TestTrue(
        TEXT("Shared Resolution opens the bounded crisis anchor"),
        TickUntil(
            [Bridge]()
            {
                return Bridge->GetSeveralVoicesOneCommandPhase() ==
                    EEchoesSeveralVoicesOneCommandPhase::AnchorCrisis;
            },
            1000));
    TestFalse(
        TEXT("A Phase Anchor outside the inherited site is rejected"),
        Bridge->IssueBuildCommand(
            Workers[0],
            EntityType::UtilityStructure,
            Bridge->SimToWorld(Vec2::FromTiles(46, 35)),
            Feedback));
    TestTrue(TEXT("The wrong anchor site is reason-coded"),
             Feedback.Contains(TEXT("CHOIR_PHASE_ANCHOR_SITE")));
    const Vec2 AnchorBuildSite = Vec2::FromTiles(
        Plan.CrisisAnchorSite.x.FloorToInt() + 2,
        Plan.CrisisAnchorSite.y.FloorToInt());
    TestTrue(
        TEXT("A worker accepts the ordinary Phase Anchor build"),
        Bridge->IssueBuildCommand(
            Workers[0],
            EntityType::UtilityStructure,
            Bridge->SimToWorld(AnchorBuildSite),
            Feedback));
    TestTrue(
        TEXT("The completed Phase Anchor begins the shared-resolution hold"),
        TickUntil(
            [Bridge]()
            {
                return Bridge->GetSeveralVoicesOneCommandPhase() ==
                    EEchoesSeveralVoicesOneCommandPhase::
                        HoldSharedResolution;
            },
            4000));
    Objective = Bridge->GetLocalObjectiveSnapshot();
    const uint64 InitialCrisisRemaining =
        Objective.SeveralVoicesCrisisTicksRemaining;
    TestTrue(
        TEXT("The deployed tracker exposes the bounded crisis duration"),
        Objective.bSeveralVoicesPhaseAnchorComplete &&
            Objective.SeveralVoicesPhaseAnchorId != 0 &&
            InitialCrisisRemaining > 0 && InitialCrisisRemaining <= 160);
    TestTrue(
        TEXT("The crisis hold reconstructs through schema-26 quick load"),
        Bridge->QuickSaveScenario(Feedback) &&
            Bridge->QuickLoadScenario(Feedback) &&
            Bridge->GetSeveralVoicesOneCommandPhase() ==
                EEchoesSeveralVoicesOneCommandPhase::HoldSharedResolution &&
            Bridge->GetLocalObjectiveSnapshot().
                    SeveralVoicesCrisisTicksRemaining ==
                InitialCrisisRemaining);

    Vec2 ContractBreakSite;
    bool bFoundContractBreakSite = false;
    const echoes::sim::Simulation* ActiveSimulation =
        Bridge->GetSimulation();
    if (ActiveSimulation != nullptr)
    {
        for (int32 TileY = 1;
             TileY < ActiveSimulation->Config().mapHeightTiles - 1 &&
             !bFoundContractBreakSite;
             ++TileY)
        {
            for (int32 TileX = 1;
                 TileX < ActiveSimulation->Config().mapWidthTiles - 1;
                 ++TileX)
            {
                const Vec2 Candidate = Vec2::FromTiles(TileX, TileY);
                const int32 Distance =
                    FMath::Abs(
                        TileX - Plan.NemeCommandSite.x.FloorToInt()) +
                    FMath::Abs(
                        TileY - Plan.NemeCommandSite.y.FloorToInt());
                if (Distance >= 12 &&
                    ActiveSimulation->IsPositionPassable(Candidate))
                {
                    ContractBreakSite = Candidate;
                    bFoundContractBreakSite = true;
                    break;
                }
            }
        }
    }
    TestTrue(
        TEXT("A reachable site exists outside Neme's crisis contract"),
        bFoundContractBreakSite);
    if (!bFoundContractBreakSite)
    {
        Bridge->StopPrototypeScenario();
        return false;
    }
    TestTrue(
        TEXT("Neme accepts a movement order that breaks the active hold"),
        Bridge->IssueCommand(
            CommandType::Move,
            Objective.SeveralVoicesNemeId,
            0,
            Bridge->SimToWorld(ContractBreakSite),
            FutureWellChoice::Dormant,
            Feedback));
    TestTrue(
        TEXT("Leaving the command site irreversibly fails the crisis hold"),
        TickUntil(
            [Bridge]()
            {
                return Bridge->GetSeveralVoicesOneCommandPhase() ==
                    EEchoesSeveralVoicesOneCommandPhase::Failed;
            },
            2000));
    TestTrue(
        TEXT("Neme can be returned after the reported contract failure"),
        Bridge->IssueCommand(
            CommandType::Move,
            Objective.SeveralVoicesNemeId,
            0,
            Bridge->SimToWorld(Plan.NemeCommandSite),
            FutureWellChoice::Dormant,
            Feedback));
    Bridge->SetScenarioPaused(false);
    TestTrue(
        TEXT("Neme physically returns to the command site"),
        TickUntil(
            [Bridge]()
            {
                return Bridge->GetLocalObjectiveSnapshot().
                    bSeveralVoicesNemeAtCommandSite;
            },
            3000));
    TestEqual(
        TEXT("Repairing the physical condition cannot clear the failure latch"),
        Bridge->GetSeveralVoicesOneCommandPhase(),
        EEchoesSeveralVoicesOneCommandPhase::Failed);
    TestTrue(
        TEXT("The repaired-but-failed hold writes its irreversible latch"),
        Bridge->QuickSaveScenario(Feedback));
    TestTrue(
        TEXT("The Mission 14 envelope restores the irreversible failure"),
        Bridge->QuickLoadScenario(Feedback) &&
            Bridge->GetSeveralVoicesOneCommandPhase() ==
                EEchoesSeveralVoicesOneCommandPhase::Failed);
    TArray<uint8> TamperedFailureEnvelope;
    TArray<uint8> TamperedFailureMapEnvelope;
    TestTrue(
        TEXT("The failed primary exposes the integrity-bound crisis flags"),
        FFileHelper::LoadFileToArray(
            TamperedFailureMapEnvelope, *QuickSavePath) &&
            FEchoesCampaignMapCheckpoint::Inspect(
                TamperedFailureMapEnvelope, MapIdentity,
                TamperedFailureEnvelope, MapFailure) &&
            TamperedFailureEnvelope.Num() > 10 &&
            TamperedFailureEnvelope[10] == 0x03);
    if (TamperedFailureEnvelope.Num() > 10)
    {
        TamperedFailureEnvelope[10] ^= 0x02;
        TArray<uint8> RewrappedTamperedFailure;
        TestTrue(
            TEXT("A one-bit attempt to clear the failure latch is written"),
            FEchoesCampaignMapCheckpoint::Wrap(
                MapIdentity, TamperedFailureEnvelope,
                RewrappedTamperedFailure, MapFailure) &&
                FFileHelper::SaveArrayToFile(
                    RewrappedTamperedFailure, *QuickSavePath));
    }
    TestTrue(
        TEXT("The tampered failure latch is rejected in favor of the prior valid hold"),
        Bridge->QuickLoadScenario(Feedback) &&
            Feedback.Contains(TEXT("prior-generation backup")) &&
            Bridge->GetSeveralVoicesOneCommandPhase() ==
                EEchoesSeveralVoicesOneCommandPhase::HoldSharedResolution);
    for (int32 TickIndex = 0; TickIndex < 10; ++TickIndex)
    {
        Bridge->Tick(0.05f);
    }
    Objective = Bridge->GetLocalObjectiveSnapshot();
    TestTrue(
        TEXT("The crisis duration decreases only with authoritative ticks"),
        Objective.SeveralVoicesCrisisTicksRemaining > 0 &&
            Objective.SeveralVoicesCrisisTicksRemaining <
                InitialCrisisRemaining);

    TestTrue(
        TEXT("Holding the complete Choir contract commits Mission 14"),
        TickUntil(
            [Bridge]()
            {
                return Bridge->GetCampaignProgress().FindDecision(
                           EEchoesCampaignMissionId::
                               SeveralVoicesOneCommand) != nullptr;
            },
            1000));
    const FEchoesCampaignDecisionRecord* MissionRecord =
        Bridge->GetCampaignProgress().FindDecision(
            EEchoesCampaignMissionId::SeveralVoicesOneCommand);
    // The commit is written now, so it stamps the current native snapshot
    // schema. That moved from 24 to 25 because per-player terrain and object
    // memory is now serialized; the replay envelope stays at 24.
    TestTrue(
        TEXT("Mission 14 stores the protocol, all facts, and schema-26 provenance"),
        MissionRecord != nullptr &&
            MissionRecord->WellChoice == FutureWellChoice::Preserve &&
            MissionRecord->AvailableWellChoices ==
                SeveralVoicesChoiceMask(FutureWellChoice::Preserve) &&
            MissionRecord->VerifiedFacts == 0xFF &&
            MissionRecord->SimulationSnapshotVersion == 26 &&
            MissionRecord->CompletionTick > 0 &&
            MissionRecord->FinalStateChecksum != 0 &&
            Bridge->IsScenarioPaused());
    FEchoesCampaignProgress Reloaded;
    TestTrue(
        TEXT("The 14-record campaign reloads transactionally"),
        FEchoesCampaignProgressStore::LoadWithBackup(
            CampaignPath, Reloaded, Feedback) &&
            Reloaded.Decisions.Num() == 14);
    if (MissionRecord != nullptr)
    {
        TestTrue(
            TEXT("The completed Mission 14 receipt replays idempotently"),
            Reloaded.AppendDecision(*MissionRecord, Feedback) ==
                EEchoesCampaignCommitStatus::AlreadyRecorded);
        FEchoesCampaignDecisionRecord RuntimeConflict = *MissionRecord;
        RuntimeConflict.WellChoice = FutureWellChoice::Harvest;
        RuntimeConflict.AvailableWellChoices =
            SeveralVoicesChoiceMask(FutureWellChoice::Harvest);
        TestTrue(
            TEXT("A divergent runtime replay cannot rewrite the receipt"),
            Reloaded.AppendDecision(RuntimeConflict, Feedback) ==
                EEchoesCampaignCommitStatus::ReplayConflict);
    }

    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors() && !WorldWrapper.HasFailed();
}

#endif
