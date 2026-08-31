#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "EchoesAssemblyOfTheMissingMissionModel.h"
#include "EchoesCampaignProgress.h"
#include "EchoesPlayerController.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Tests/AutomationCommon.h"

namespace
{
struct FPreservedAssemblyFile final
{
    explicit FPreservedAssemblyFile(FString InPath)
        : Path(MoveTemp(InPath))
    {
        bExisted = IFileManager::Get().FileExists(*Path);
        if (bExisted)
        {
            FFileHelper::LoadFileToArray(Contents, *Path);
        }
    }

    ~FPreservedAssemblyFile()
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

uint8 AssemblyChoiceMask(echoes::sim::FutureWellChoice Choice)
{
    switch (Choice)
    {
        case echoes::sim::FutureWellChoice::Harvest: return 1 << 0;
        case echoes::sim::FutureWellChoice::Preserve: return 1 << 1;
        case echoes::sim::FutureWellChoice::Reshape: return 1 << 2;
        default: return 0;
    }
}

FEchoesCampaignDecisionRecord MakeAssemblyRecord(
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
        Mission == EEchoesCampaignMissionId::AssemblyOfTheMissing;
    Record.WellChoice = bLumeReceipt ? LumeChoice : FoundingChoice;
    Record.AvailableWellChoices =
        Mission == EEchoesCampaignMissionId::WhatTheLedgerKeeps ||
                Mission == EEchoesCampaignMissionId::ChoirAtLumeReach
            ? 0x07
        : bLumeReceipt
            ? AssemblyChoiceMask(LumeChoice)
            : AssemblyChoiceMask(FoundingChoice);
    Record.SimulationSnapshotVersion = echoes::sim::kSnapshotVersion;
    Record.CompletionTick = 1000 + static_cast<uint8>(Mission) * 400;
    Record.FinalStateChecksum =
        0xA551E000ULL + static_cast<uint8>(Mission);
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
            Record.VerifiedFacts = 0xFF;
            break;
    }
    return Record;
}

FEchoesCampaignProgress MakeAssemblyPrerequisites(
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
            MakeAssemblyRecord(
                static_cast<EEchoesCampaignMissionId>(MissionValue),
                FoundingChoice,
                LumeChoice,
                ReserveFacts),
            OutFeedback);
    }
    return Progress;
}

FString AssemblyQuickSavePath(const FEchoesCampaignProgress& Progress)
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
            TEXT("EchoesQuickSaveAssemblyOfTheMissing-%08X.bin"),
            Fingerprint));
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesAssemblyOfTheMissingMissionTest,
    "Echoes.Runtime.Campaign.AssemblyOfTheMissing",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesAssemblyOfTheMissingMissionTest::RunTest(
    const FString& Parameters)
{
    (void)Parameters;

    FEchoesScopedTestSaveEnvironment TestSaveEnvironment(*this);
    if (!TestSaveEnvironment.IsReady())
    {
        return false;
    }
    using echoes::sim::EntityType;
    using echoes::sim::Faction;
    using echoes::sim::FutureWellChoice;
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
            for (const FutureWellChoice LumeChoice : Choices)
            {
                FEchoesAssemblyOfTheMissingPlan Plan;
                const bool bPlanned =
                    FEchoesAssemblyOfTheMissingMissionModel::TryPlanForLedger(
                        FoundingChoice,
                        ReserveFacts,
                        LumeChoice,
                        Plan);
                TestTrue(
                    TEXT("Every founding, district-pair, and protocol tuple has one bounded public-assembly plan"),
                    bPlanned && Plan.MeridianPublicRecordSite != Vec2{} &&
                        Plan.KharuunPublicRecordSite != Vec2{} &&
                        Plan.CrownfallIndexSite ==
                            FEchoesNoNeutralLedgerMissionModel::
                                RallySiteForProtocol(LumeChoice) &&
                        Plan.MeridianAssemblyWitnessSite !=
                            Plan.KharuunAssemblyWitnessSite);
                PlanKeys.Add(Plan.StablePlanKey);
                ++PlanContracts;
            }
        }
    }
    TestEqual(TEXT("The inherited matrix contains 27 plans"),
              PlanContracts, 27);
    TestEqual(TEXT("All 27 public-assembly plans have unique stable keys"),
              PlanKeys.Num(), 27);
    FEchoesAssemblyOfTheMissingPlan InvalidPlan;
    TestFalse(
        TEXT("A malformed district allocation is rejected"),
        FEchoesAssemblyOfTheMissingMissionModel::TryPlanForLedger(
            FutureWellChoice::Harvest,
            0x79,
            FutureWellChoice::Preserve,
            InvalidPlan));
    TestFalse(
        TEXT("Dormant cannot be used as the recorded Mission 13 protocol"),
        FEchoesAssemblyOfTheMissingMissionModel::TryPlanForLedger(
            FutureWellChoice::Harvest,
            0x7B,
            FutureWellChoice::Dormant,
            InvalidPlan));

    FEchoesAssemblyOfTheMissingMissionFacts Facts;
    TestTrue(
        TEXT("Inactive facts stay outside Mission 13"),
        FEchoesAssemblyOfTheMissingMissionModel::DeterminePhase(Facts) ==
            EEchoesAssemblyOfTheMissingPhase::Inactive);
    Facts.bOperationActive = true;
    Facts.bLocalCoreIntact = true;
    Facts.bOruunIntact = true;
    Facts.bVerifierIntact = true;
    Facts.bPublicInterfacesIntact = true;
    Facts.bSkirmishStillOngoing = true;
    TestTrue(
        TEXT("Mission 13 begins at paired public-record readback"),
        FEchoesAssemblyOfTheMissingMissionModel::DeterminePhase(Facts) ==
            EEchoesAssemblyOfTheMissingPhase::EstablishPublicRecordReadback);
    Facts.bPublicRecordReadbackEstablished = true;
    TestTrue(
        TEXT("Paired readback opens the Crownfall index link"),
        FEchoesAssemblyOfTheMissingMissionModel::DeterminePhase(Facts) ==
            EEchoesAssemblyOfTheMissingPhase::LinkCrownfallIndex);
    Facts.bCrownfallIndexLinked = true;
    TestTrue(
        TEXT("The durable public link opens independent observation"),
        FEchoesAssemblyOfTheMissingMissionModel::DeterminePhase(Facts) ==
            EEchoesAssemblyOfTheMissingPhase::ObserveAssembly);
    Facts.bMeridianAssemblyWitnessObserved = true;
    Facts.bKharuunAssemblyWitnessObserved = true;
    TestTrue(
        TEXT("Both independent observations complete the bounded receipt"),
        FEchoesAssemblyOfTheMissingMissionModel::DeterminePhase(Facts) ==
            EEchoesAssemblyOfTheMissingPhase::Complete);
    Facts.bVerifierIntact = false;
    TestTrue(
        TEXT("Verifier loss fails closed"),
        FEchoesAssemblyOfTheMissingMissionModel::DeterminePhase(Facts) ==
            EEchoesAssemblyOfTheMissingPhase::Failed);
    Facts.bVerifierIntact = true;
    Facts.bPublicRecordReadbackEstablished = false;
    TestTrue(
        TEXT("A link without the prerequisite public readback fails closed"),
        FEchoesAssemblyOfTheMissingMissionModel::DeterminePhase(Facts) ==
            EEchoesAssemblyOfTheMissingPhase::Failed);

    TestEqual(TEXT("Mission 13 uses the current campaign schema"),
              FEchoesCampaignProgress::SchemaVersion,
              static_cast<uint16>(2));
    TestEqual(TEXT("Mission 13 accepts the current native snapshot schema"),
              echoes::sim::kSnapshotVersion,
              static_cast<uint32>(23));

    FString Feedback;
    FEchoesCampaignProgress TwelveRecords = MakeAssemblyPrerequisites(
        FutureWellChoice::Harvest,
        FutureWellChoice::Preserve,
        0x7B,
        12,
        Feedback);
    TestEqual(TEXT("The accepted prerequisite contains exactly 12 records"),
              TwelveRecords.Decisions.Num(), 12);
    FEchoesCampaignDecisionRecord InvalidAssembly = MakeAssemblyRecord(
        EEchoesCampaignMissionId::AssemblyOfTheMissing,
        FutureWellChoice::Harvest,
        FutureWellChoice::Preserve);
    InvalidAssembly.VerifiedFacts = 0xFE;
    FEchoesCampaignProgress InvalidProgress = TwelveRecords;
    TestTrue(
        TEXT("Mission 13 rejects any missing fact"),
        InvalidProgress.AppendDecision(InvalidAssembly, Feedback) ==
            EEchoesCampaignCommitStatus::StorageFailure);
    InvalidAssembly = MakeAssemblyRecord(
        EEchoesCampaignMissionId::AssemblyOfTheMissing,
        FutureWellChoice::Harvest,
        FutureWellChoice::Preserve);
    InvalidAssembly.SimulationSnapshotVersion = 20;
    TestTrue(
        TEXT("Mission 13 rejects schema-20 provenance"),
        InvalidProgress.AppendDecision(InvalidAssembly, Feedback) ==
                EEchoesCampaignCommitStatus::StorageFailure &&
            Feedback.Contains(TEXT("schema 21")));
    InvalidAssembly = MakeAssemblyRecord(
        EEchoesCampaignMissionId::AssemblyOfTheMissing,
        FutureWellChoice::Harvest,
        FutureWellChoice::Harvest);
    TestTrue(
        TEXT("Mission 13 cannot rewrite the Mission 12 protocol"),
        InvalidProgress.AppendDecision(InvalidAssembly, Feedback) ==
                EEchoesCampaignCommitStatus::StorageFailure &&
            Feedback.Contains(TEXT("ASSEMBLY_LUME_PROTOCOL")));

    FEchoesCampaignProgress SyntheticComplete = TwelveRecords;
    const FEchoesCampaignDecisionRecord ValidAssembly = MakeAssemblyRecord(
        EEchoesCampaignMissionId::AssemblyOfTheMissing,
        FutureWellChoice::Harvest,
        FutureWellChoice::Preserve);
    TestTrue(
        TEXT("An exact 13-record chain accepts one public-assembly receipt"),
        SyntheticComplete.AppendDecision(ValidAssembly, Feedback) ==
            EEchoesCampaignCommitStatus::Added);
    TArray<uint8> EncodedComplete;
    FEchoesCampaignProgress DecodedComplete;
    TestTrue(
        TEXT("The 13-record chain round-trips transactionally"),
        FEchoesCampaignProgressStore::Encode(
            SyntheticComplete, EncodedComplete, Feedback) &&
        FEchoesCampaignProgressStore::Decode(
            EncodedComplete, DecodedComplete, Feedback) &&
        DecodedComplete.Decisions.Num() == 13 &&
        DecodedComplete.Decisions[12].VerifiedFacts == 0xFF);

    FEchoesCampaignProgress Reordered = TwelveRecords;
    Swap(Reordered.Decisions[10], Reordered.Decisions[11]);
    TestTrue(
        TEXT("A reordered M11-M12 chain cannot admit Mission 13"),
        Reordered.AppendDecision(ValidAssembly, Feedback) ==
                EEchoesCampaignCommitStatus::StorageFailure &&
            Feedback.Contains(TEXT("ASSEMBLY_LEDGER_ORDER")));

    const FString CampaignPath =
        FEchoesCampaignProgressStore::GetDefaultPath();
    FPreservedAssemblyFile PreservedCampaign(CampaignPath);
    FPreservedAssemblyFile PreservedCampaignBackup(
        CampaignPath + TEXT(".bak"));
    FPreservedAssemblyFile PreservedCampaignTemporary(
        CampaignPath + TEXT(".tmp"));
    IFileManager::Get().Delete(*CampaignPath, false, true, true);
    IFileManager::Get().Delete(
        *(CampaignPath + TEXT(".bak")), false, true, true);
    IFileManager::Get().Delete(
        *(CampaignPath + TEXT(".tmp")), false, true, true);

    FEchoesCampaignProgress ElevenRecords = MakeAssemblyPrerequisites(
        FutureWellChoice::Harvest,
        FutureWellChoice::Preserve,
        0x7B,
        11,
        Feedback);
    TestTrue(
        TEXT("The 11-record lock fixture is stored"),
        FEchoesCampaignProgressStore::SaveAtomic(
            CampaignPath, ElevenRecords, Feedback));
    {
        FTestWorldWrapper LockedWorld;
        if (!LockedWorld.CreateTestWorld(EWorldType::Game))
        {
            LockedWorld.ForwardErrorMessages(this);
            AddError(TEXT("Could not create the locked Mission 13 world."));
            return false;
        }
        UEchoesSimulationSubsystem* LockedBridge =
            LockedWorld.GetTestWorld()->GetSubsystem<
                UEchoesSimulationSubsystem>();
        TestFalse(
            TEXT("Mission 13 rejects a ledger without Mission 12"),
            LockedBridge != nullptr &&
                LockedBridge->SelectOperationMode(
                    EEchoesOperationMode::CampaignAssemblyOfTheMissing,
                    Feedback));
        TestTrue(TEXT("The lock response names The Future That Won"),
                 Feedback.Contains(TEXT("The Future That Won")));
        LockedWorld.ForwardErrorMessages(this);
    }

    const FString QuickSavePath = AssemblyQuickSavePath(TwelveRecords);
    FPreservedAssemblyFile PreservedQuickSave(QuickSavePath);
    FPreservedAssemblyFile PreservedQuickSaveBackup(
        QuickSavePath + TEXT(".bak"));
    FPreservedAssemblyFile PreservedQuickSaveTemporary(
        QuickSavePath + TEXT(".tmp"));
    for (const FString& Path : {
             QuickSavePath,
             QuickSavePath + TEXT(".bak"),
             QuickSavePath + TEXT(".tmp")})
    {
        IFileManager::Get().Delete(*Path, false, true, true);
    }
    TestTrue(
        TEXT("The exact 12-record campaign is stored"),
        FEchoesCampaignProgressStore::SaveAtomic(
            CampaignPath, TwelveRecords, Feedback));

    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the Mission 13 test world."));
        return false;
    }
    UEchoesSimulationSubsystem* Bridge =
        WorldWrapper.GetTestWorld()->GetSubsystem<
            UEchoesSimulationSubsystem>();
    if (!TestNotNull(TEXT("Mission 13 owns a simulation subsystem"), Bridge) ||
        !TestTrue(TEXT("Twelve exact records unlock Mission 13"),
                  Bridge != nullptr &&
                      Bridge->IsAssemblyOfTheMissingUnlocked()) ||
        !TestTrue(TEXT("Mission 13 can be selected"),
                  Bridge->SelectOperationMode(
                      EEchoesOperationMode::CampaignAssemblyOfTheMissing,
                      Feedback)) ||
        !TestTrue(TEXT("Mission 13 can start"),
                  Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const FEchoesAssemblyOfTheMissingPlan Plan =
        Bridge->GetAssemblyOfTheMissingPlan();
    const FEchoesObjectiveSnapshot Start =
        Bridge->GetLocalObjectiveSnapshot();
    TestTrue(
        TEXT("Harvest plus Life Support/Transit plus Preserve retains plan 07"),
        Plan.StablePlanKey == 7 &&
            Plan.FoundingDoctrine == FutureWellChoice::Harvest &&
            Plan.RecordedProtocol == FutureWellChoice::Preserve &&
            Plan.FirstContributingDistrict ==
                EEchoesCityDistrict::LifeSupport &&
            Plan.SecondContributingDistrict ==
                EEchoesCityDistrict::Transit &&
            Plan.DeferredDistrict == EEchoesCityDistrict::Archive);
    const echoes::sim::Entity* Oruun =
        Bridge->FindEntity(Start.AssemblyOruunId);
    const echoes::sim::Entity* Verifier =
        Bridge->FindEntity(Start.AssemblyVerifierId);
    const echoes::sim::Entity* MeridianPublicRecord =
        Bridge->FindEntity(
            Start.AssemblyMeridianPublicRecordInterfaceId);
    const echoes::sim::Entity* KharuunPublicRecord =
        Bridge->FindEntity(
            Start.AssemblyKharuunPublicRecordInterfaceId);
    const echoes::sim::Entity* CrownfallIndex =
        Bridge->FindEntity(Start.AssemblyCrownfallIndexInterfaceId);
    TestTrue(
        TEXT("Two Kharuun scouts are commandable and exactly three mission interfaces are neutral"),
        Oruun != nullptr && Verifier != nullptr &&
            Oruun->owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            Verifier->owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            Oruun->faction == Faction::KharuunAssemblies &&
            Verifier->faction == Faction::KharuunAssemblies &&
            Oruun->type == EntityType::ScoutUnit &&
            Verifier->type == EntityType::ScoutUnit &&
        MeridianPublicRecord != nullptr &&
            MeridianPublicRecord->owner == echoes::sim::kNeutralPlayer &&
            MeridianPublicRecord->faction == Faction::MeridianCompact &&
            MeridianPublicRecord->attackDamage == 0 &&
            MeridianPublicRecord->visionTiles == 0 &&
        KharuunPublicRecord != nullptr &&
            KharuunPublicRecord->owner == echoes::sim::kNeutralPlayer &&
            KharuunPublicRecord->faction == Faction::KharuunAssemblies &&
            KharuunPublicRecord->attackDamage == 0 &&
            KharuunPublicRecord->visionTiles == 0 &&
        CrownfallIndex != nullptr &&
            CrownfallIndex->owner == echoes::sim::kNeutralPlayer &&
            CrownfallIndex->position == Plan.CrownfallIndexSite &&
            CrownfallIndex->attackDamage == 0 &&
            CrownfallIndex->visionTiles == 0);
    TestTrue(
        TEXT("Mission 13 begins at public-record readback"),
        Bridge->GetAssemblyOfTheMissingPhase() ==
            EEchoesAssemblyOfTheMissingPhase::EstablishPublicRecordReadback);

    AEchoesPlayerController* Controller =
        WorldWrapper.GetTestWorld()->SpawnActor<AEchoesPlayerController>();
    if (Controller != nullptr)
    {
        Controller->PresentTitleScreen();
    }
    TestTrue(
        TEXT("The title path binds the recorded Preserve context"),
        Controller != nullptr &&
            Controller->GetFutureWellChoice() == FutureWellChoice::Preserve);
    if (Controller != nullptr)
    {
        Controller->Destroy();
    }

    TestTrue(
        TEXT("The initial Mission 13 state quick-saves in its 12-record namespace"),
        Bridge->QuickSaveScenario(Feedback) &&
            IFileManager::Get().FileExists(*QuickSavePath));
    TestTrue(
        TEXT("The initial reducer state reconstructs after quick load"),
        Bridge->QuickLoadScenario(Feedback) &&
            Bridge->GetAssemblyOfTheMissingPhase() ==
                EEchoesAssemblyOfTheMissingPhase::
                    EstablishPublicRecordReadback);

    TArray<echoes::sim::EntityId> Workers;
    for (const echoes::sim::Entity& Entity :
         Bridge->GetSimulation()->Entities())
    {
        if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            Entity.type == EntityType::Worker)
        {
            Workers.Add(Entity.id);
        }
    }
    if (!TestTrue(TEXT("A Kharuun construction worker is available"),
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
    const Vec2 LinkSite = Vec2::FromTiles(
        Plan.CrownfallIndexSite.x.FloorToInt() + 2,
        Plan.CrownfallIndexSite.y.FloorToInt());
    Bridge->SetScenarioPaused(false);
    TestFalse(
        TEXT("The local player cannot command the neutral Crownfall index"),
        Bridge->IssueCommand(
            echoes::sim::CommandType::Move,
            Start.AssemblyCrownfallIndexInterfaceId,
            0,
            Bridge->SimToWorld(Plan.CrownfallIndexSite),
            FutureWellChoice::Dormant,
            Feedback));
    TestFalse(
        TEXT("The Crownfall link is locked before paired readback"),
        Bridge->IssueBuildCommand(
            Workers[0],
            EntityType::UtilityStructure,
            Bridge->SimToWorld(LinkSite),
            Feedback));
    TestTrue(TEXT("The early link rejection names public readback"),
             Feedback.Contains(TEXT("ASSEMBLY_PUBLIC_READBACK_REQUIRED")));
    TestTrue(
        TEXT("Oruun accepts the Kharuun public record site"),
        Bridge->IssueCommand(
            echoes::sim::CommandType::Move,
            Start.AssemblyOruunId,
            0,
            Bridge->SimToWorld(Plan.KharuunPublicRecordSite),
            FutureWellChoice::Dormant,
            Feedback));
    TestTrue(
        TEXT("The verifier accepts the Meridian public record site"),
        Bridge->IssueCommand(
            echoes::sim::CommandType::Move,
            Start.AssemblyVerifierId,
            0,
            Bridge->SimToWorld(Plan.MeridianPublicRecordSite),
            FutureWellChoice::Dormant,
            Feedback));
    TestTrue(
        TEXT("Paired readback opens the Crownfall link"),
        TickUntil(
            [Bridge]()
            {
                return Bridge->GetAssemblyOfTheMissingPhase() ==
                    EEchoesAssemblyOfTheMissingPhase::LinkCrownfallIndex;
            },
            6000));
    TestTrue(
        TEXT("The paired readback reconstructs through schema-23 quick load"),
        Bridge->QuickSaveScenario(Feedback) &&
            Bridge->QuickLoadScenario(Feedback) &&
            Bridge->GetLocalObjectiveSnapshot().
                bAssemblyPublicRecordReadbackEstablished);
    TestFalse(
        TEXT("A link outside the Crownfall index radius is rejected"),
        Bridge->IssueBuildCommand(
            Workers[0],
            EntityType::UtilityStructure,
            Bridge->SimToWorld(Vec2::FromTiles(46, 35)),
            Feedback));
    TestTrue(TEXT("The wrong link site is reason-coded"),
             Feedback.Contains(TEXT("ASSEMBLY_CROWNFALL_INDEX_SITE")));
    TestTrue(
        TEXT("A worker accepts the ordinary Crownfall Listening Spine build"),
        Bridge->IssueBuildCommand(
            Workers[0],
            EntityType::UtilityStructure,
            Bridge->SimToWorld(LinkSite),
            Feedback));
    TestTrue(
        TEXT("The completed link opens independent assembly observation"),
        TickUntil(
            [Bridge]()
            {
                return Bridge->GetAssemblyOfTheMissingPhase() ==
                    EEchoesAssemblyOfTheMissingPhase::ObserveAssembly;
            },
            7000));
    TestTrue(
        TEXT("Two observation-state saves retain a valid prior generation"),
        Bridge->QuickSaveScenario(Feedback) &&
            Bridge->QuickSaveScenario(Feedback) &&
            IFileManager::Get().FileExists(
                *(QuickSavePath + TEXT(".bak"))));
    TArray<uint8> CorruptedCheckpoint;
    TestTrue(
        TEXT("The primary envelope can be corrupted for backup recovery"),
        FFileHelper::LoadFileToArray(
            CorruptedCheckpoint, *QuickSavePath) &&
            CorruptedCheckpoint.Num() > 20);
    if (CorruptedCheckpoint.Num() > 20)
    {
        CorruptedCheckpoint[20] ^= 0x5A;
        TestTrue(TEXT("The corrupted primary is written"),
                 FFileHelper::SaveArrayToFile(
                     CorruptedCheckpoint, *QuickSavePath));
    }
    TestTrue(
        TEXT("Quick load falls back to the bound observation backup"),
        Bridge->QuickLoadScenario(Feedback) &&
            Bridge->GetAssemblyOfTheMissingPhase() ==
                EEchoesAssemblyOfTheMissingPhase::ObserveAssembly);

    TestTrue(
        TEXT("Oruun accepts the Kharuun assembly witness site"),
        Bridge->IssueCommand(
            echoes::sim::CommandType::Move,
            Start.AssemblyOruunId,
            0,
            Bridge->SimToWorld(Plan.KharuunAssemblyWitnessSite),
            FutureWellChoice::Dormant,
            Feedback));
    TestTrue(
        TEXT("The verifier accepts the Meridian assembly witness site"),
        Bridge->IssueCommand(
            echoes::sim::CommandType::Move,
            Start.AssemblyVerifierId,
            0,
            Bridge->SimToWorld(Plan.MeridianAssemblyWitnessSite),
            FutureWellChoice::Dormant,
            Feedback));
    TestTrue(
        TEXT("Ordinary paired observation commits Mission 13"),
        TickUntil(
            [Bridge]()
            {
                return Bridge->GetCampaignProgress().FindDecision(
                           EEchoesCampaignMissionId::
                               AssemblyOfTheMissing) != nullptr;
            },
            7000));
    const FEchoesCampaignDecisionRecord* MissionRecord =
        Bridge->GetCampaignProgress().FindDecision(
            EEchoesCampaignMissionId::AssemblyOfTheMissing);
    TestTrue(
        TEXT("Mission 13 stores one protocol, all facts, and schema-23 provenance"),
        MissionRecord != nullptr &&
            MissionRecord->WellChoice == FutureWellChoice::Preserve &&
            MissionRecord->AvailableWellChoices ==
                AssemblyChoiceMask(FutureWellChoice::Preserve) &&
            MissionRecord->VerifiedFacts == 0xFF &&
            MissionRecord->SimulationSnapshotVersion == 23 &&
            MissionRecord->CompletionTick > 0 &&
            MissionRecord->FinalStateChecksum != 0);
    FEchoesCampaignProgress Reloaded;
    TestTrue(
        TEXT("The 13-record campaign reloads transactionally"),
        FEchoesCampaignProgressStore::LoadWithBackup(
            CampaignPath, Reloaded, Feedback) &&
            Reloaded.Decisions.Num() == 13);
    if (MissionRecord != nullptr)
    {
        TestTrue(
            TEXT("An identical Mission 13 replay is idempotent"),
            Reloaded.AppendDecision(*MissionRecord, Feedback) ==
                EEchoesCampaignCommitStatus::AlreadyRecorded);
        FEchoesCampaignDecisionRecord Conflict = *MissionRecord;
        Conflict.WellChoice = FutureWellChoice::Harvest;
        Conflict.AvailableWellChoices =
            AssemblyChoiceMask(FutureWellChoice::Harvest);
        Conflict.CompletionTick += 1;
        Conflict.FinalStateChecksum += 1;
        TestTrue(
            TEXT("A divergent Mission 13 replay is refused without rewrite"),
            Reloaded.AppendDecision(Conflict, Feedback) ==
                EEchoesCampaignCommitStatus::ReplayConflict);
        TestTrue(
            TEXT("The original Preserve receipt remains authoritative"),
            Reloaded.FindDecision(
                EEchoesCampaignMissionId::AssemblyOfTheMissing)->WellChoice ==
                FutureWellChoice::Preserve);
    }

    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors() && !WorldWrapper.HasFailed();
}

#endif
