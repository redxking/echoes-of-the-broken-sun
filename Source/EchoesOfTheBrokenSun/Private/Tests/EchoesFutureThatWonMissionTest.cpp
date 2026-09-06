#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"
#include "EchoesCampaignLedgerProbe.h"

#include "EchoesCampaignProgress.h"
#include "EchoesCampaignTerrainBinding.h"
#include "EchoesFutureThatWonMissionModel.h"
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
struct FPreservedFutureThatWonFile final
{
    explicit FPreservedFutureThatWonFile(FString InPath)
        : Path(MoveTemp(InPath))
    {
        bExisted = IFileManager::Get().FileExists(*Path);
        if (bExisted)
        {
            FFileHelper::LoadFileToArray(Contents, *Path);
        }
    }

    ~FPreservedFutureThatWonFile()
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

uint8 FutureThatWonChoiceMask(echoes::sim::FutureWellChoice Choice)
{
    switch (Choice)
    {
        case echoes::sim::FutureWellChoice::Harvest: return 1 << 0;
        case echoes::sim::FutureWellChoice::Preserve: return 1 << 1;
        case echoes::sim::FutureWellChoice::Reshape: return 1 << 2;
        default: return 0;
    }
}

FEchoesCampaignDecisionRecord MakeFutureThatWonRecord(
    EEchoesCampaignMissionId Mission,
    echoes::sim::FutureWellChoice FoundingChoice,
    echoes::sim::FutureWellChoice LumeChoice,
    uint8 ReserveFacts = 0x7B)
{
    FEchoesCampaignDecisionRecord Record;
    Record.Mission = Mission;
    Record.WellChoice =
        Mission == EEchoesCampaignMissionId::ChoirAtLumeReach ||
                Mission == EEchoesCampaignMissionId::NoNeutralLedger ||
                Mission == EEchoesCampaignMissionId::TheFutureThatWon
            ? LumeChoice
            : FoundingChoice;
    Record.AvailableWellChoices =
        Mission == EEchoesCampaignMissionId::WhatTheLedgerKeeps ||
                Mission == EEchoesCampaignMissionId::ChoirAtLumeReach
            ? 0x07
        : Mission == EEchoesCampaignMissionId::NoNeutralLedger ||
                Mission == EEchoesCampaignMissionId::TheFutureThatWon
            ? FutureThatWonChoiceMask(LumeChoice)
            : FutureThatWonChoiceMask(FoundingChoice);
    Record.SimulationSnapshotVersion = echoes::sim::kSnapshotVersion;
    Record.CompletionTick = 100 + static_cast<uint8>(Mission) * 300;
    Record.FinalStateChecksum =
        0xF017A000ULL + static_cast<uint8>(Mission);
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
            Record.VerifiedFacts = 0xFF;
            break;
    }
    return Record;
}

FEchoesCampaignProgress MakeFutureThatWonPrerequisites(
    echoes::sim::FutureWellChoice FoundingChoice,
    echoes::sim::FutureWellChoice LumeChoice,
    uint8 ReserveFacts,
    int32 ThroughMission,
    FString& OutFeedback)
{
    FEchoesCampaignProgress Progress;
    for (int32 MissionValue = 1;
         MissionValue <= FMath::Min(ThroughMission, 11);
         ++MissionValue)
    {
        Progress.AppendDecision(
            MakeFutureThatWonRecord(
                static_cast<EEchoesCampaignMissionId>(MissionValue),
                FoundingChoice,
                LumeChoice,
                ReserveFacts),
            OutFeedback);
    }
    return Progress;
}

FString FutureThatWonQuickSavePath(
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
            TEXT("EchoesQuickSaveTheFutureThatWon-%08X.bin"),
            Fingerprint));
}

FString DescribeFutureThatWonEntity(
    const TCHAR* Label,
    echoes::sim::EntityId Id,
    const echoes::sim::Entity* Current)
{
    if (Current == nullptr)
    {
        return FString::Printf(TEXT("%s{id=%u missing}"), Label, Id);
    }
    return FString::Printf(
        TEXT("%s{id=%u hp=%d/%d pos=(%d,%d) order=%u target=%u destination=(%d,%d)}"),
        Label,
        Id,
        Current->hitPoints,
        Current->maxHitPoints,
        Current->position.x.FloorToInt(),
        Current->position.y.FloorToInt(),
        static_cast<uint8>(Current->order.type),
        Current->order.target,
        Current->order.destination.x.FloorToInt(),
        Current->order.destination.y.FloorToInt());
}

bool FutureThatWonEntityWithinTiles(
    const UEchoesSimulationSubsystem* Bridge,
    echoes::sim::EntityId EntityId,
    const echoes::sim::Vec2& Site,
    int32 RadiusTiles)
{
    const echoes::sim::Entity* Current = Bridge->FindEntity(EntityId);
    if (Current == nullptr || Current->hitPoints <= 0)
    {
        return false;
    }
    const int64 DeltaX = static_cast<int64>(Current->position.x.Raw()) -
        Site.x.Raw();
    const int64 DeltaY = static_cast<int64>(Current->position.y.Raw()) -
        Site.y.Raw();
    const int64 RadiusRaw =
        static_cast<int64>(RadiusTiles) * echoes::sim::kFixedScale;
    return DeltaX * DeltaX + DeltaY * DeltaY <= RadiusRaw * RadiusRaw;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesFutureThatWonMissionTest,
    "Echoes.Runtime.Campaign.FutureThatWon",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesFutureThatWonMissionTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    FEchoesScopedTestSaveEnvironment TestSaveEnvironment(*this);
    if (!TestSaveEnvironment.IsReady())
    {
        return false;
    }

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
                FEchoesFutureThatWonPlan Plan;
                const bool bPlanned =
                    FEchoesFutureThatWonMissionModel::TryPlanForLedger(
                        FoundingChoice,
                        ReserveFacts,
                        LumeChoice,
                        Plan);
                TestTrue(
                    TEXT("Every founding, district-pair, and protocol tuple has a bounded restoration plan"),
                    bPlanned && Plan.FirstDistrictInputSite != Vec2{} &&
                        Plan.SecondDistrictInputSite != Vec2{} &&
                        Plan.MeridianReadbackSite ==
                            Vec2::FromTiles(26, 43) &&
                        Plan.KharuunReadbackSite ==
                            Vec2::FromTiles(38, 43) &&
                        Plan.RestorationDemonstratorSite ==
                            Vec2::FromTiles(32, 49) &&
                        Plan.FutureWellSite ==
                            FEchoesNoNeutralLedgerMissionModel::
                                RallySiteForProtocol(LumeChoice) &&
                        Plan.StabilityWindowTicks == 300);
                PlanKeys.Add(Plan.StablePlanKey);
                ++PlanContracts;
            }
        }
    }
    TestEqual(
        TEXT("The inherited acceptance matrix contains exactly 27 plans"),
        PlanContracts,
        27);
    TestEqual(
        TEXT("All 27 restoration plans retain unique stable keys"),
        PlanKeys.Num(),
        27);
    FEchoesFutureThatWonPlan InvalidPlan;
    TestFalse(
        TEXT("A reserve record without exactly two powered districts is rejected"),
        FEchoesFutureThatWonMissionModel::TryPlanForLedger(
            FutureWellChoice::Harvest,
            0x79,
            FutureWellChoice::Preserve,
            InvalidPlan));
    TestFalse(
        TEXT("A dormant recorded protocol cannot produce Mission 12"),
        FEchoesFutureThatWonMissionModel::TryPlanForLedger(
            FutureWellChoice::Harvest,
            0x7B,
            FutureWellChoice::Dormant,
            InvalidPlan));

    FEchoesFutureThatWonMissionFacts Facts;
    TestTrue(
        TEXT("Inactive facts remain outside Mission 12"),
        FEchoesFutureThatWonMissionModel::DeterminePhase(Facts) ==
            EEchoesFutureThatWonPhase::Inactive);
    Facts.bOperationActive = true;
    Facts.bLocalCoreIntact = true;
    Facts.bOruunIntact = true;
    Facts.bVerifierIntact = true;
    Facts.bFutureWellIntact = true;
    Facts.bPublicInterfacesIntact = true;
    Facts.bSkirmishStillOngoing = true;
    TestTrue(
        TEXT("The reducer begins with independent public readback"),
        FEchoesFutureThatWonMissionModel::DeterminePhase(Facts) ==
            EEchoesFutureThatWonPhase::EstablishIndependentReadback);
    Facts.bIndependentPublicReadbackEstablished = true;
    TestTrue(
        TEXT("Independent readback opens recorded input verification"),
        FEchoesFutureThatWonMissionModel::DeterminePhase(Facts) ==
            EEchoesFutureThatWonPhase::VerifyRecordedInputs);
    Facts.bFirstRecordedInputVerified = true;
    Facts.bSecondRecordedInputVerified = true;
    TestTrue(
        TEXT("Both recorded inputs open recorded-protocol binding"),
        FEchoesFutureThatWonMissionModel::DeterminePhase(Facts) ==
            EEchoesFutureThatWonPhase::BindRecordedProtocol);
    Facts.bRecordedProtocolBound = true;
    TestTrue(
        TEXT("The exact recorded protocol opens the stability hold"),
        FEchoesFutureThatWonMissionModel::DeterminePhase(Facts) ==
            EEchoesFutureThatWonPhase::HoldStabilityWindow);
    Facts.bReshapeWindowExpired = true;
    TestTrue(
        TEXT("Normal temporary Reshape expiry does not rewrite the activation receipt"),
        FEchoesFutureThatWonMissionModel::DeterminePhase(Facts) ==
            EEchoesFutureThatWonPhase::HoldStabilityWindow);
    Facts.bStabilityWindowHeld = true;
    TestTrue(
        TEXT("The held apparatus opens paired district observation"),
        FEchoesFutureThatWonMissionModel::DeterminePhase(Facts) ==
            EEchoesFutureThatWonPhase::ObserveDistrictReadbacks);
    Facts.bFirstDistrictReadbackObserved = true;
    Facts.bSecondDistrictReadbackObserved = true;
    TestTrue(
        TEXT("Both observed readbacks complete the bounded contract"),
        FEchoesFutureThatWonMissionModel::DeterminePhase(Facts) ==
            EEchoesFutureThatWonPhase::Complete);
    Facts.bConflictingProtocolBound = true;
    TestTrue(
        TEXT("A conflicting protocol fails closed"),
        FEchoesFutureThatWonMissionModel::DeterminePhase(Facts) ==
            EEchoesFutureThatWonPhase::Failed);
    Facts.bConflictingProtocolBound = false;
    Facts.bVerifierIntact = false;
    TestTrue(
        TEXT("Independent-verifier loss fails the operation"),
        FEchoesFutureThatWonMissionModel::DeterminePhase(Facts) ==
            EEchoesFutureThatWonPhase::Failed);

    TestEqual(
        TEXT("Campaign persistence uses the current schema"),
        FEchoesCampaignProgress::SchemaVersion,
        static_cast<uint16>(2));
    // Schema 28 appends player-hostility masks after schema 27 lifecycle state.
    // The replay envelope shape did not change and stays at 24; this assertion
    // pins the native snapshot schema only.
    TestEqual(
        TEXT("Mission 12 accepts the current simulation snapshot schema"),
        echoes::sim::kSnapshotVersion,
        static_cast<uint32>(28));

    const FString CampaignPath =
        FEchoesCampaignProgressStore::GetDefaultPath();
    FPreservedFutureThatWonFile PreservedPrimary(CampaignPath);
    FPreservedFutureThatWonFile PreservedBackup(CampaignPath + TEXT(".bak"));
    FPreservedFutureThatWonFile PreservedTemporary(CampaignPath + TEXT(".tmp"));
    IFileManager::Get().Delete(*CampaignPath, false, true, true);
    IFileManager::Get().Delete(
        *(CampaignPath + TEXT(".bak")), false, true, true);
    IFileManager::Get().Delete(
        *(CampaignPath + TEXT(".tmp")), false, true, true);

    FString Feedback;
    FEchoesCampaignProgress TenRecords = MakeFutureThatWonPrerequisites(
        FutureWellChoice::Harvest,
        FutureWellChoice::Preserve,
        0x7B,
        10,
        Feedback);
    TestTrue(
        TEXT("The ten-record lock fixture is stored"),
        TenRecords.Decisions.Num() == 10 &&
            FEchoesCampaignProgressStore::SaveAtomic(
                CampaignPath, TenRecords, Feedback));
    {
        FTestWorldWrapper LockedWorld;
        if (!LockedWorld.CreateTestWorld(EWorldType::Game))
        {
            LockedWorld.ForwardErrorMessages(this);
            AddError(TEXT("Could not create the locked Mission 12 world."));
            return false;
        }
        UEchoesSimulationSubsystem* LockedBridge =
            LockedWorld.GetTestWorld()->GetSubsystem<
                UEchoesSimulationSubsystem>();
        TestFalse(
            TEXT("Mission 12 rejects a ledger without Mission 11"),
            LockedBridge != nullptr &&
                LockedBridge->SelectOperationMode(
                    EEchoesOperationMode::CampaignFutureThatWon,
                    Feedback));
        TestTrue(
            TEXT("The lock response names No Neutral Ledger"),
            Feedback.Contains(TEXT("No Neutral Ledger")));
        LockedWorld.ForwardErrorMessages(this);
    }

    FEchoesCampaignProgress ElevenRecords =
        MakeFutureThatWonPrerequisites(
            FutureWellChoice::Harvest,
            FutureWellChoice::Preserve,
            0x7B,
            11,
            Feedback);
    TestTrue(
        TEXT("The exact eleven-record prerequisite retains Mission 10 and Mission 11 Preserve"),
        ElevenRecords.Decisions.Num() == 11 &&
            ElevenRecords.Decisions[9].WellChoice ==
                FutureWellChoice::Preserve &&
            ElevenRecords.Decisions[9].AvailableWellChoices == 0x07 &&
            ElevenRecords.Decisions[10].WellChoice ==
                FutureWellChoice::Preserve &&
            ElevenRecords.Decisions[10].AvailableWellChoices ==
                FutureThatWonChoiceMask(FutureWellChoice::Preserve));

    FEchoesCampaignDecisionRecord InvalidRestoration =
        MakeFutureThatWonRecord(
            EEchoesCampaignMissionId::TheFutureThatWon,
            FutureWellChoice::Harvest,
            FutureWellChoice::Preserve);
    InvalidRestoration.AvailableWellChoices = 0x07;
    FEchoesCampaignProgress InvalidProgress = ElevenRecords;
    TestTrue(
        TEXT("Mission 12 cannot claim alternate protocols were available"),
        InvalidProgress.AppendDecision(InvalidRestoration, Feedback) ==
            EEchoesCampaignCommitStatus::StorageFailure);
    InvalidRestoration = MakeFutureThatWonRecord(
        EEchoesCampaignMissionId::TheFutureThatWon,
        FutureWellChoice::Harvest,
        FutureWellChoice::Harvest);
    TestTrue(
        TEXT("Mission 12 cannot rewrite the Mission 11 protocol receipt"),
        InvalidProgress.AppendDecision(InvalidRestoration, Feedback) ==
            EEchoesCampaignCommitStatus::StorageFailure &&
            Feedback.Contains(TEXT("RESTORATION_LUME_PROTOCOL")));
    InvalidRestoration = MakeFutureThatWonRecord(
        EEchoesCampaignMissionId::TheFutureThatWon,
        FutureWellChoice::Harvest,
        FutureWellChoice::Preserve);
    InvalidRestoration.VerifiedFacts = 0xFE;
    TestTrue(
        TEXT("Mission 12 rejects any missing completion fact"),
        InvalidProgress.AppendDecision(InvalidRestoration, Feedback) ==
            EEchoesCampaignCommitStatus::StorageFailure);
    InvalidRestoration = MakeFutureThatWonRecord(
        EEchoesCampaignMissionId::TheFutureThatWon,
        FutureWellChoice::Harvest,
        FutureWellChoice::Preserve);
    InvalidRestoration.SimulationSnapshotVersion = 20;
    TestTrue(
        TEXT("Mission 12 rejects forged schema-20 activation provenance"),
        InvalidProgress.AppendDecision(InvalidRestoration, Feedback) ==
                EEchoesCampaignCommitStatus::StorageFailure &&
            Feedback.Contains(TEXT("CAMPAIGN_SNAPSHOT_VERSION_REQUIRED")) &&
            Feedback.Contains(TEXT("schema 21")));
    InvalidRestoration = MakeFutureThatWonRecord(
        EEchoesCampaignMissionId::TheFutureThatWon,
        FutureWellChoice::Harvest,
        FutureWellChoice::Preserve);
    InvalidRestoration.SimulationSnapshotVersion =
        echoes::sim::kSnapshotVersion + 1;
    TestTrue(
        TEXT("Mission 12 rejects unsupported future snapshot provenance"),
        InvalidProgress.AppendDecision(InvalidRestoration, Feedback) ==
                EEchoesCampaignCommitStatus::StorageFailure &&
            Feedback.Contains(TEXT("CAMPAIGN_SNAPSHOT_VERSION_REQUIRED")));

    FEchoesCampaignProgress SyntheticComplete = ElevenRecords;
    const FEchoesCampaignDecisionRecord ValidRestoration =
        MakeFutureThatWonRecord(
            EEchoesCampaignMissionId::TheFutureThatWon,
            FutureWellChoice::Harvest,
            FutureWellChoice::Preserve);
    TestTrue(
        TEXT("An exact single-protocol twelve-record chain validates"),
        SyntheticComplete.AppendDecision(ValidRestoration, Feedback) ==
            EEchoesCampaignCommitStatus::Added);
    TArray<uint8> EncodedComplete;
    FEchoesCampaignProgress DecodedComplete;
    TestTrue(
        TEXT("The twelve-record chain encodes and decodes without changing campaign schema"),
        FEchoesCampaignProgressStore::Encode(
            SyntheticComplete, EncodedComplete, Feedback) &&
            FEchoesCampaignProgressStore::Decode(
                EncodedComplete, DecodedComplete, Feedback) &&
            DecodedComplete.Decisions.Num() == 12 &&
            DecodedComplete.Decisions[11].VerifiedFacts == 0xFF);

    FEchoesCampaignProgress AlternateEleven =
        MakeFutureThatWonPrerequisites(
            FutureWellChoice::Reshape,
            FutureWellChoice::Harvest,
            0x7D,
            11,
            Feedback);
    const FString QuickSavePath =
        FutureThatWonQuickSavePath(ElevenRecords);
    const FString AlternateQuickSavePath =
        FutureThatWonQuickSavePath(AlternateEleven);
    TestTrue(
        TEXT("Distinct eleven-record ledgers retain distinct checkpoint namespaces"),
        !QuickSavePath.IsEmpty() && !AlternateQuickSavePath.IsEmpty() &&
            QuickSavePath != AlternateQuickSavePath);
    FPreservedFutureThatWonFile PreservedQuickSave(QuickSavePath);
    FPreservedFutureThatWonFile PreservedQuickSaveBackup(
        QuickSavePath + TEXT(".bak"));
    FPreservedFutureThatWonFile PreservedQuickSaveTemporary(
        QuickSavePath + TEXT(".tmp"));
    FPreservedFutureThatWonFile PreservedAlternateQuickSave(
        AlternateQuickSavePath);
    FPreservedFutureThatWonFile PreservedAlternateQuickSaveBackup(
        AlternateQuickSavePath + TEXT(".bak"));
    FPreservedFutureThatWonFile PreservedAlternateQuickSaveTemporary(
        AlternateQuickSavePath + TEXT(".tmp"));
    for (const FString& Path : {
             QuickSavePath,
             QuickSavePath + TEXT(".bak"),
             QuickSavePath + TEXT(".tmp"),
             AlternateQuickSavePath,
             AlternateQuickSavePath + TEXT(".bak"),
             AlternateQuickSavePath + TEXT(".tmp")})
    {
        IFileManager::Get().Delete(*Path, false, true, true);
    }

    TestTrue(
        TEXT("The accepted eleven-record campaign is stored"),
        FEchoesCampaignProgressStore::SaveAtomic(
            CampaignPath, ElevenRecords, Feedback));
    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the Mission 12 test world."));
        return false;
    }
    UEchoesSimulationSubsystem* Bridge =
        WorldWrapper.GetTestWorld()->GetSubsystem<
            UEchoesSimulationSubsystem>();
    if (!TestNotNull(TEXT("Mission 12 world owns the simulation subsystem"), Bridge) ||
        !TestTrue(
            TEXT("Eleven exact records unlock Mission 12"),
            Bridge != nullptr && Bridge->IsFutureThatWonUnlocked()) ||
        !TestTrue(
            TEXT("Mission 12 operation can be selected"),
            Bridge->SelectOperationMode(
                EEchoesOperationMode::CampaignFutureThatWon,
                Feedback)) ||
        !TestTrue(
            TEXT("Mission 12 scenario starts"),
            Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const FEchoesFutureThatWonPlan Plan =
        Bridge->GetFutureThatWonPlan();
    const FEchoesObjectiveSnapshot Start =
        Bridge->GetLocalObjectiveSnapshot();
    AEchoesPlayerController* Controller =
        WorldWrapper.GetTestWorld()->SpawnActor<AEchoesPlayerController>();
    if (Controller != nullptr)
    {
        Controller->PresentTitleScreen();
    }
    TestTrue(
        TEXT("The normal Mission 12 title path binds the controller to recorded Preserve"),
        Controller != nullptr &&
            Controller->GetFutureWellChoice() == FutureWellChoice::Preserve);
    if (Controller != nullptr)
    {
        Controller->Destroy();
    }
    TestTrue(
        TEXT("Harvest founding plus Life Support/Transit plus Preserve retains plan 07"),
        Plan.StablePlanKey == 7 &&
            Plan.FoundingDoctrine == FutureWellChoice::Harvest &&
            Plan.RecordedProtocol == FutureWellChoice::Preserve &&
            Plan.FirstContributingDistrict ==
                EEchoesCityDistrict::LifeSupport &&
            Plan.SecondContributingDistrict ==
                EEchoesCityDistrict::Transit &&
            Plan.DeferredDistrict == EEchoesCityDistrict::Archive);
    const echoes::sim::Entity* Oruun =
        Bridge->FindEntity(Start.FutureWonOruunId);
    const echoes::sim::Entity* Verifier =
        Bridge->FindEntity(Start.FutureWonVerifierId);
    const echoes::sim::Entity* FirstDistrict =
        Bridge->FindEntity(Start.FutureWonFirstDistrictInterfaceId);
    const echoes::sim::Entity* SecondDistrict =
        Bridge->FindEntity(Start.FutureWonSecondDistrictInterfaceId);
    const echoes::sim::Entity* MeridianReadback =
        Bridge->FindEntity(Start.FutureWonMeridianReadbackInterfaceId);
    const echoes::sim::Entity* KharuunReadback =
        Bridge->FindEntity(Start.FutureWonKharuunReadbackInterfaceId);
    const echoes::sim::Entity* Demonstrator =
        Bridge->FindEntity(Start.FutureWonDemonstratorInterfaceId);
    const echoes::sim::Entity* Well =
        Bridge->FindEntity(Start.FutureWonWellId);
    TestTrue(
        TEXT("Only two Kharuun scouts are commandable while all public apparatus and the Well remain neutral"),
        Oruun != nullptr && Verifier != nullptr &&
            Oruun->owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            Verifier->owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            Oruun->faction == echoes::sim::Faction::KharuunAssemblies &&
            Verifier->faction == echoes::sim::Faction::KharuunAssemblies &&
            Oruun->type == echoes::sim::EntityType::ScoutUnit &&
            Verifier->type == echoes::sim::EntityType::ScoutUnit &&
        FirstDistrict != nullptr && SecondDistrict != nullptr &&
            FirstDistrict->owner == echoes::sim::kNeutralPlayer &&
            SecondDistrict->owner == echoes::sim::kNeutralPlayer &&
            FirstDistrict->faction == echoes::sim::Faction::MeridianCompact &&
            SecondDistrict->faction == echoes::sim::Faction::MeridianCompact &&
            FirstDistrict->aegisPowered && SecondDistrict->aegisPowered &&
        MeridianReadback != nullptr &&
            MeridianReadback->owner == echoes::sim::kNeutralPlayer &&
            MeridianReadback->faction ==
                echoes::sim::Faction::MeridianCompact &&
            MeridianReadback->aegisPowered &&
        KharuunReadback != nullptr &&
            KharuunReadback->owner == echoes::sim::kNeutralPlayer &&
            KharuunReadback->faction ==
                echoes::sim::Faction::KharuunAssemblies &&
            !KharuunReadback->aegisPowered &&
        Demonstrator != nullptr &&
            Demonstrator->owner == echoes::sim::kNeutralPlayer &&
            Demonstrator->faction ==
                echoes::sim::Faction::MeridianCompact &&
            Demonstrator->position == Plan.RestorationDemonstratorSite &&
            Demonstrator->attackDamage == 0 &&
            Demonstrator->visionTiles == 0 &&
        Well != nullptr &&
            Well->owner == echoes::sim::kNeutralPlayer &&
            Well->type == echoes::sim::EntityType::FutureWell &&
            Well->position == Plan.FutureWellSite &&
            Well->wellActivationTick == 0);
    int32 BlockedTiles = 0;
    for (int32 TileY = 0;
         TileY < Bridge->GetSimulation()->Config().mapHeightTiles;
         ++TileY)
    {
        for (int32 TileX = 0;
             TileX < Bridge->GetSimulation()->Config().mapWidthTiles;
             ++TileX)
        {
            if (Bridge->GetSimulation()->TerrainAt(TileX, TileY) ==
                echoes::sim::Terrain::Blocked)
            {
                ++BlockedTiles;
            }
        }
    }
    const auto* TerrainFounding = Bridge->GetCampaignProgress().FindDecision(
        EEchoesCampaignMissionId::WhatTheLedgerKeeps);
    if (!TestNotNull(TEXT("Terrain binding has the founding record"), TerrainFounding)) return false;
    const auto TerrainContract = echoes::world::CheckCampaignTerrain(12, TerrainFounding->WellChoice);
    TestTrue(TEXT("Dedicated mission terrain contract validates"), TerrainContract.ok);
    TestEqual(TEXT("Dedicated mission census matches its source"), BlockedTiles, TerrainContract.blocked_cells);
    for (int32 Y = 0; Y < 64; ++Y)
        for (int32 X = 0; X < 64; ++X)
            TestEqual(TEXT("Mission terrain matches every source cell"),
                Bridge->GetSimulation()->TerrainAt(X,Y) != echoes::sim::Terrain::Blocked,
                echoes::world::IsCampaignTerrainPassable(12, TerrainFounding->WellChoice, X,Y));
    TestTrue(
        TEXT("Mission 12 begins at independent readback"),
        Bridge->GetFutureThatWonPhase() ==
            EEchoesFutureThatWonPhase::EstablishIndependentReadback);
    TestTrue(
        TEXT("The initial state quick-saves into the eleven-record namespace"),
        Bridge->QuickSaveScenario(Feedback) &&
            IFileManager::Get().FileExists(*QuickSavePath));
    TestTrue(
        TEXT("The initial Mission 12 reducer state reconstructs after quick load"),
        Bridge->QuickLoadScenario(Feedback) &&
            Bridge->GetFutureThatWonPhase() ==
                EEchoesFutureThatWonPhase::EstablishIndependentReadback);

    TArray<uint8> CheckpointBytes;
    TestTrue(
        TEXT("The checkpoint can seed both cross-ledger generations"),
        FFileHelper::LoadFileToArray(CheckpointBytes, *QuickSavePath) &&
            FFileHelper::SaveArrayToFile(
                CheckpointBytes, *AlternateQuickSavePath) &&
            FFileHelper::SaveArrayToFile(
                CheckpointBytes,
                *(AlternateQuickSavePath + TEXT(".bak"))));
    TestTrue(
        TEXT("The alternate eleven-record campaign is stored for the probe"),
        FEchoesCampaignProgressStore::SaveAtomic(
            CampaignPath, AlternateEleven, Feedback));
    {
        FTestWorldWrapper AlternateWorld;
        if (!AlternateWorld.CreateTestWorld(EWorldType::Game))
        {
            AlternateWorld.ForwardErrorMessages(this);
            AddError(TEXT("Could not create the alternate Mission 12 world."));
            return false;
        }
        UEchoesSimulationSubsystem* AlternateBridge =
            AlternateWorld.GetTestWorld()->GetSubsystem<
                UEchoesSimulationSubsystem>();
        if (!TestNotNull(
                TEXT("The alternate world owns a simulation subsystem"),
                AlternateBridge) ||
            !TestTrue(
                TEXT("The alternate campaign can select Mission 12"),
                AlternateBridge->SelectOperationMode(
                    EEchoesOperationMode::CampaignFutureThatWon,
                    Feedback)) ||
            !TestTrue(
                TEXT("The alternate campaign can start Mission 12"),
                AlternateBridge->StartPrototypeScenario()))
        {
            AlternateWorld.ForwardErrorMessages(this);
            return false;
        }
        TestFalse(
            TEXT("A renamed checkpoint cannot cross eleven-record chains"),
            AlternateBridge->QuickLoadScenario(Feedback));
        TestTrue(TEXT("The foreign founding doctrine is refused by the map envelope"),
                 Feedback.Contains(TEXT("CAMPAIGN_MAP_STALE")));
        if (!TestTrue(TEXT("A current-map envelope can carry the deliberate foreign-ledger probe"),
                      EchoesCampaignTest::BindForeignLedgerToCurrentMap(*AlternateBridge))) return false;
        TestFalse(TEXT("The inner ledger binding independently refuses the foreign payload"),
                  AlternateBridge->QuickLoadScenario(Feedback));
        TestTrue(
            TEXT("Cross-ledger loading reports exact binding rejection"),
            Feedback.Contains(TEXT("ledger binding does not match")));
        AlternateBridge->StopPrototypeScenario();
        AlternateWorld.ForwardErrorMessages(this);
    }
    TestTrue(
        TEXT("The accepted eleven-record campaign is restored"),
        FEchoesCampaignProgressStore::SaveAtomic(
            CampaignPath, ElevenRecords, Feedback));

    TArray<echoes::sim::EntityId> Workers;
    TArray<echoes::sim::EntityId> Soldiers;
    TArray<echoes::sim::EntityId> Heavies;
    for (const echoes::sim::Entity& Entity :
         Bridge->GetSimulation()->Entities())
    {
        if (Entity.owner != UEchoesSimulationSubsystem::LocalPlayerId)
        {
            continue;
        }
        if (Entity.type == echoes::sim::EntityType::Worker)
        {
            Workers.Add(Entity.id);
        }
        else if (Entity.type == echoes::sim::EntityType::Soldier)
        {
            Soldiers.Add(Entity.id);
        }
        else if (Entity.type == echoes::sim::EntityType::HeavyUnit)
        {
            Heavies.Add(Entity.id);
        }
    }
    Soldiers.Sort();
    Heavies.Sort();
    if (!TestTrue(
            TEXT("Mission 12 exposes three workers, three Soldiers, and one Heavy for ordinary guarded play"),
            Workers.Num() >= 3 && Soldiers.Num() >= 3 &&
                Heavies.Num() >= 1))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const TArray<echoes::sim::EntityId> GuardIds = {
        Soldiers[0], Heavies[0], Soldiers[1], Soldiers[2]};
    TArray<echoes::sim::EntityId> GuardTargetIds = {
        Start.FutureWonOruunId,
        Start.FutureWonOruunId,
        Start.FutureWonOruunId,
        Start.FutureWonOruunId};
    Bridge->SetScenarioPaused(false);
    bool bGuardCommandsAccepted = true;
    for (int32 GuardIndex = 0; GuardIndex < GuardIds.Num(); ++GuardIndex)
    {
        const echoes::sim::Entity* Ward =
            Bridge->FindEntity(GuardTargetIds[GuardIndex]);
        const bool bAccepted = Ward != nullptr && Bridge->IssueCommand(
                echoes::sim::CommandType::Guard,
                GuardIds[GuardIndex],
                GuardTargetIds[GuardIndex],
                Bridge->SimToWorld(Ward != nullptr ? Ward->position : Vec2{}),
                FutureWellChoice::Dormant,
                Feedback);
        bGuardCommandsAccepted = bAccepted && bGuardCommandsAccepted;
    }
    if (!TestTrue(
            TEXT("Three Soldiers and one Heavy guard Oruun on the exposed district route"),
            bGuardCommandsAccepted))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    Bridge->Tick(0.05f);

    FString FirstTacticalFailure;
    const auto WriteTacticalFailure = [
        this,
        Bridge,
        Start,
        &GuardIds,
        &Workers,
        &FirstTacticalFailure](const TCHAR* Context)
    {
        if (!FirstTacticalFailure.IsEmpty())
        {
            return;
        }
        FirstTacticalFailure = FString::Printf(
            TEXT("[M12_TACTICAL_FAILURE] context=%s reason=%s phase=%s tick=%llu %s %s %s %s %s %s"),
            Context,
            *Bridge->GetMissionFailureReasonCode(),
            FEchoesFutureThatWonMissionModel::StableName(
                Bridge->GetFutureThatWonPhase()),
            static_cast<unsigned long long>(
                Bridge->GetSimulation()->CurrentTick()),
            *DescribeFutureThatWonEntity(
                TEXT("oruun"),
                Start.FutureWonOruunId,
                Bridge->FindEntity(Start.FutureWonOruunId)),
            *DescribeFutureThatWonEntity(
                TEXT("verifier"),
                Start.FutureWonVerifierId,
                Bridge->FindEntity(Start.FutureWonVerifierId)),
            *DescribeFutureThatWonEntity(
                TEXT("oruunGuard1"),
                GuardIds[0],
                Bridge->FindEntity(GuardIds[0])),
            *DescribeFutureThatWonEntity(
                TEXT("oruunGuard2"),
                GuardIds[1],
                Bridge->FindEntity(GuardIds[1])),
            *DescribeFutureThatWonEntity(
                TEXT("oruunGuard3"),
                GuardIds[2],
                Bridge->FindEntity(GuardIds[2])),
            *DescribeFutureThatWonEntity(
                TEXT("oruunGuard4"),
                GuardIds[3],
                Bridge->FindEntity(GuardIds[3])));
        for (const auto WorkerId : Workers)
            FirstTacticalFailure += TEXT(" ") + DescribeFutureThatWonEntity(
                TEXT("worker"), WorkerId, Bridge->FindEntity(WorkerId));
        const auto* Well = Bridge->FindEntity(Start.FutureWonWellId);
        if (Well != nullptr)
            FirstTacticalFailure += FString::Printf(TEXT(" wellOwner=%u capturePlayer=%u captureProgress=%u contested=%d"),
                Well->owner, Well->wellCapturePlayer, Well->wellCaptureProgress,
                Bridge->GetSimulation()->IsFutureWellContested(*Well) ? 1 : 0);
        AddInfo(FirstTacticalFailure);
    };
    const auto MaintainGuards = [
        Bridge,
        &GuardIds,
        &GuardTargetIds,
        &Workers,
        &WriteTacticalFailure]()
    {
        for (auto& Target : GuardTargetIds)
        {
            if (Workers.Contains(Target) && Bridge->FindEntity(Target) == nullptr)
                for (auto Candidate : Workers)
                {
                    const auto* Worker = Bridge->FindEntity(Candidate);
                    if (Worker != nullptr && Worker->hitPoints > 0)
                    { Target = Candidate; break; }
                }
        }
        for (int32 GuardIndex = 0; GuardIndex < GuardIds.Num(); ++GuardIndex)
        {
            const echoes::sim::Entity* Guard =
                Bridge->FindEntity(GuardIds[GuardIndex]);
            const echoes::sim::Entity* Ward =
                Bridge->FindEntity(GuardTargetIds[GuardIndex]);
            if (Ward == nullptr || Ward->hitPoints <= 0)
            {
                WriteTacticalFailure(TEXT("protected-witness-loss"));
                return false;
            }
            if (Guard == nullptr || Guard->hitPoints <= 0)
            {
                continue;
            }
            if (Guard->order.type != echoes::sim::OrderType::Guard ||
                Guard->order.target != GuardTargetIds[GuardIndex])
            {
                FString GuardFeedback;
                if (!Bridge->IssueCommand(
                        echoes::sim::CommandType::Guard,
                        GuardIds[GuardIndex],
                        GuardTargetIds[GuardIndex],
                        Bridge->SimToWorld(Ward->position),
                        FutureWellChoice::Dormant,
                        GuardFeedback))
                {
                    WriteTacticalFailure(TEXT("guard-reassertion"));
                    return false;
                }
            }
        }
        return true;
    };
    const auto TickUntil = [
        this,
        Bridge,
        &MaintainGuards,
        &WriteTacticalFailure,
        &FirstTacticalFailure](const TFunction<bool()>& Predicate,
                               int32 MaximumTicks)
    {
        for (int32 TickIndex = 0; TickIndex < MaximumTicks; ++TickIndex)
        {
            if (Predicate())
            {
                return true;
            }
            if (Bridge->GetFutureThatWonPhase() ==
                    EEchoesFutureThatWonPhase::Failed ||
                !MaintainGuards())
            {
                WriteTacticalFailure(TEXT("wait"));
                return false;
            }
            Bridge->Tick(0.05f);
        }
        if (Predicate())
        {
            return true;
        }
        if (Bridge->GetFutureThatWonPhase() ==
                EEchoesFutureThatWonPhase::Failed ||
            !MaintainGuards())
        {
            WriteTacticalFailure(TEXT("wait-boundary"));
        }
        return false;
    };
    const auto PaceWitness = [
        Bridge,
        &GuardIds,
        &GuardTargetIds,
        &MaintainGuards,
        &WriteTacticalFailure](
            echoes::sim::EntityId WitnessId,
            int32 FirstGuardIndex,
            const Vec2& Goal,
            int32 MaximumTicks)
    {
        (void)FirstGuardIndex;
        constexpr int32 StepRaw = 2 * echoes::sim::kFixedScale;
        int32 RemainingTicks = MaximumTicks;
        const auto EscortsReformed = [
            Bridge,
            WitnessId,
            &GuardIds,
            &GuardTargetIds]()
        {
            const echoes::sim::Entity* Witness =
                Bridge->FindEntity(WitnessId);
            if (Witness == nullptr || Witness->hitPoints <= 0)
            {
                return false;
            }
            for (int32 GuardIndex = 0; GuardIndex < GuardIds.Num(); ++GuardIndex)
            {
                if (GuardTargetIds[GuardIndex] != WitnessId) continue;
                const echoes::sim::Entity* Guard =
                    Bridge->FindEntity(GuardIds[GuardIndex]);
                if (Guard == nullptr || Guard->hitPoints <= 0)
                {
                    continue;
                }
                if (Guard->order.type != echoes::sim::OrderType::Guard ||
                    Guard->order.target != GuardTargetIds[GuardIndex] ||
                    !FutureThatWonEntityWithinTiles(
                        Bridge, GuardIds[GuardIndex], Witness->position, 3))
                {
                    return false;
                }
            }
            return true;
        };
        while (RemainingTicks > 0)
        {
            if (Bridge->GetFutureThatWonPhase() ==
                EEchoesFutureThatWonPhase::Complete)
            {
                return FutureThatWonEntityWithinTiles(
                    Bridge, WitnessId, Goal, 3);
            }
            while (RemainingTicks > 0 && !EscortsReformed())
            {
                if (Bridge->GetFutureThatWonPhase() ==
                        EEchoesFutureThatWonPhase::Failed ||
                    !MaintainGuards())
                {
                    WriteTacticalFailure(TEXT("convoy-regroup"));
                    return false;
                }
                Bridge->Tick(0.05f);
                --RemainingTicks;
            }
            if (FutureThatWonEntityWithinTiles(
                    Bridge, WitnessId, Goal, 1))
            {
                return true;
            }
            const echoes::sim::Entity* Witness =
                Bridge->FindEntity(WitnessId);
            if (Witness == nullptr || Witness->hitPoints <= 0 ||
                RemainingTicks <= 0)
            {
                WriteTacticalFailure(TEXT("convoy-witness"));
                return false;
            }
            const Vec2 StepStart = Witness->position;
            FString MoveFeedback;
            if (!Bridge->IssueCommand(
                    echoes::sim::CommandType::Move,
                    WitnessId,
                    0,
                    Bridge->SimToWorld(Goal),
                    FutureWellChoice::Dormant,
                    MoveFeedback))
            {
                WriteTacticalFailure(TEXT("convoy-move"));
                return false;
            }
            bool bStepComplete = false;
            while (RemainingTicks > 0 && !bStepComplete)
            {
                if (Bridge->GetFutureThatWonPhase() ==
                        EEchoesFutureThatWonPhase::Failed ||
                    !MaintainGuards())
                {
                    WriteTacticalFailure(TEXT("convoy-step"));
                    return false;
                }
                Bridge->Tick(0.05f);
                --RemainingTicks;
                if (Bridge->GetFutureThatWonPhase() ==
                    EEchoesFutureThatWonPhase::Complete)
                {
                    return FutureThatWonEntityWithinTiles(
                        Bridge, WitnessId, Goal, 3);
                }
                Witness = Bridge->FindEntity(WitnessId);
                if (Witness == nullptr || Witness->hitPoints <= 0)
                {
                    WriteTacticalFailure(TEXT("convoy-witness"));
                    return false;
                }
                const int64 DeltaX =
                    static_cast<int64>(Witness->position.x.Raw()) -
                    StepStart.x.Raw();
                const int64 DeltaY =
                    static_cast<int64>(Witness->position.y.Raw()) -
                    StepStart.y.Raw();
                bStepComplete = FutureThatWonEntityWithinTiles(
                    Bridge, WitnessId, Goal, 1) ||
                    DeltaX * DeltaX + DeltaY * DeltaY >=
                        static_cast<int64>(StepRaw) * StepRaw;
            }
            if (FutureThatWonEntityWithinTiles(
                    Bridge, WitnessId, Goal, 1))
            {
                continue;
            }
            Witness = Bridge->FindEntity(WitnessId);
            FString StopFeedback;
            if (Witness == nullptr ||
                !Bridge->IssueCommand(
                    echoes::sim::CommandType::Stop,
                    WitnessId,
                    0,
                    Bridge->SimToWorld(Witness->position),
                    FutureWellChoice::Dormant,
                    StopFeedback))
            {
                WriteTacticalFailure(TEXT("convoy-stop"));
                return false;
            }
        }
        WriteTacticalFailure(TEXT("convoy-budget"));
        return false;
    };
    const Vec2 FirstLinkSite = Vec2::FromTiles(
        Plan.FirstDistrictInputSite.x.FloorToInt() + 2,
        Plan.FirstDistrictInputSite.y.FloorToInt());
    const Vec2 SecondLinkSite = Vec2::FromTiles(
        Plan.SecondDistrictInputSite.x.FloorToInt() + 2,
        Plan.SecondDistrictInputSite.y.FloorToInt());

    TestFalse(
        TEXT("The local player cannot command Rhyse's public demonstrator"),
        Bridge->IssueCommand(
            echoes::sim::CommandType::Move,
            Start.FutureWonDemonstratorInterfaceId,
            0,
            Bridge->SimToWorld(Plan.FutureWellSite),
            FutureWellChoice::Dormant,
            Feedback));
    TestFalse(
        TEXT("District input construction is locked before independent readback"),
        Bridge->IssueBuildCommand(
            Workers[0],
            echoes::sim::EntityType::UtilityStructure,
            Bridge->SimToWorld(FirstLinkSite),
            Feedback));
    TestTrue(
        TEXT("The early construction rejection names the readback prerequisite"),
        Feedback.Contains(TEXT("FUTURE_THAT_WON_READBACK_REQUIRED")));
    TestFalse(
        TEXT("The recorded protocol is locked before readback and inputs"),
        Bridge->IssueCommand(
            echoes::sim::CommandType::FutureWell,
            Workers[0],
            Start.FutureWonWellId,
            Bridge->SimToWorld(Plan.FutureWellSite),
            FutureWellChoice::Preserve,
            Feedback));
    TestTrue(
        TEXT("The early Well rejection is reason-coded"),
        Feedback.Contains(TEXT("FUTURE_THAT_WON_PROTOCOL_REQUIRED")));

    // Stage workers concurrently with the convoy, before the readback unlocks
    // construction, so Oruun need not wait exposed for their entire base journey.
    for (int32 Index = 0; Index < 2; ++Index)
    {
        TestTrue(TEXT("An input worker accepts ordinary advance staging"),
            Bridge->IssueCommand(echoes::sim::CommandType::Move, Workers[Index], 0,
                Bridge->SimToWorld(Index == 0 ? Plan.FirstDistrictInputSite : Plan.SecondDistrictInputSite),
                FutureWellChoice::Dormant, Feedback));
    }
    TestTrue(TEXT("The third worker stages as a capture reserve"),
        Bridge->IssueCommand(echoes::sim::CommandType::Move, Workers[2], 0,
            Bridge->SimToWorld(Plan.FirstDistrictInputSite), FutureWellChoice::Dormant, Feedback));
    TestTrue(
        TEXT("Oruun reaches the Kharuun public readback with surviving escorts regrouped"),
        PaceWitness(
            Start.FutureWonOruunId,
            0,
            Plan.KharuunReadbackSite,
            1800));
    TestTrue(
        TEXT("The verifier reaches the Meridian public readback"),
        PaceWitness(
            Start.FutureWonVerifierId,
            2,
            Plan.MeridianReadbackSite,
            1800));
    TestTrue(
        TEXT("Two-person public readback opens recorded input verification"),
        TickUntil(
            [Bridge]()
            {
                return Bridge->GetFutureThatWonPhase() ==
                    EEchoesFutureThatWonPhase::VerifyRecordedInputs;
            },
            5600));
    TestTrue(
        TEXT("The readback state survives a schema-25 quick save and load"),
        Bridge->QuickSaveScenario(Feedback) &&
            Bridge->QuickLoadScenario(Feedback) &&
            Bridge->GetLocalObjectiveSnapshot().
                bFutureWonIndependentReadbackEstablished);
    TestFalse(
        TEXT("An unrecorded district site is rejected"),
        Bridge->IssueBuildCommand(
            Workers[0],
            echoes::sim::EntityType::UtilityStructure,
            Bridge->SimToWorld(Vec2::FromTiles(46, 35)),
            Feedback));
    TestTrue(
        TEXT("The unrecorded site rejection is reason-coded"),
        Feedback.Contains(TEXT("FUTURE_THAT_WON_DISTRICT_SITE")));
    TestTrue(
        TEXT("A worker accepts the first Kharuun input link"),
        Bridge->IssueBuildCommand(
            Workers[0],
            echoes::sim::EntityType::UtilityStructure,
            Bridge->SimToWorld(FirstLinkSite),
            Feedback));
    TestTrue(
        TEXT("A second worker accepts the second Kharuun input link"),
        Bridge->IssueBuildCommand(
            Workers[1],
            echoes::sim::EntityType::UtilityStructure,
            Bridge->SimToWorld(SecondLinkSite),
            Feedback));
    TestTrue(
        TEXT("Both completed district inputs open recorded-protocol binding"),
        TickUntil(
            [Bridge]()
            {
                return Bridge->GetFutureThatWonPhase() ==
                    EEchoesFutureThatWonPhase::BindRecordedProtocol;
            },
            5600));

    const Vec2 WellApproach = Vec2::FromTiles(
        Plan.FutureWellSite.x.FloorToInt() - 2,
        Plan.FutureWellSite.y.FloorToInt() - 2);
    TestTrue(
        TEXT("A worker accepts an ordinary approach to the Future Well"),
        Bridge->IssueCommand(
            echoes::sim::CommandType::Move,
            Workers[0],
            0,
            Bridge->SimToWorld(WellApproach),
            FutureWellChoice::Dormant,
            Feedback));
    TestTrue(
        TEXT("The player legitimately sees the Future Well"),
        TickUntil(
            [Bridge, Start]()
            {
                return Bridge->GetSimulation()->IsEntityVisibleTo(
                    UEchoesSimulationSubsystem::LocalPlayerId,
                    Start.FutureWonWellId);
            },
            2600));
    TestFalse(
        TEXT("A non-recorded Harvest protocol is rejected"),
        Bridge->IssueCommand(
            echoes::sim::CommandType::FutureWell,
            Workers[0],
            Start.FutureWonWellId,
            Bridge->SimToWorld(Plan.FutureWellSite),
            FutureWellChoice::Harvest,
            Feedback));
    TestTrue(
        TEXT("The protocol rejection names the recorded Preserve plan"),
        Feedback.Contains(TEXT("FUTURE_THAT_WON_PROTOCOL_REQUIRED")) &&
            Feedback.Contains(Plan.ProtocolDisplayName));
    TestTrue(
        TEXT("The exact recorded Preserve protocol is accepted"),
        Bridge->IssueCommand(
            echoes::sim::CommandType::FutureWell,
            Workers[0],
            Start.FutureWonWellId,
            Bridge->SimToWorld(Plan.FutureWellSite),
            FutureWellChoice::Preserve,
            Feedback));
    TestTrue(TEXT("The second input worker supports the same capture without increasing its rate"),
        Bridge->IssueCommand(echoes::sim::CommandType::FutureWell, Workers[1], Start.FutureWonWellId,
            Bridge->SimToWorld(Plan.FutureWellSite), FutureWellChoice::Preserve, Feedback));
    TestTrue(TEXT("The reserve worker supports the same fixed-rate capture"),
        Bridge->IssueCommand(echoes::sim::CommandType::FutureWell, Workers[2], Start.FutureWonWellId,
            Bridge->SimToWorld(Plan.FutureWellSite), FutureWellChoice::Preserve, Feedback));
    for (auto& Target : GuardTargetIds) Target = Workers[0];
    TestTrue(TEXT("Surviving escorts protect the worker's approach and capture"), MaintainGuards());
    TestTrue(TEXT("Oruun withdraws to the first district during Well capture"),
        Bridge->IssueCommand(echoes::sim::CommandType::Move, Start.FutureWonOruunId, 0,
            Bridge->SimToWorld(Plan.FirstDistrictInputSite),
            FutureWellChoice::Dormant, Feedback));
    TestTrue(TEXT("The verifier also leaves the exposed readback during Well capture"),
        Bridge->IssueCommand(echoes::sim::CommandType::Move, Start.FutureWonVerifierId, 0,
            Bridge->SimToWorld(Vec2::FromTiles(
                Plan.FirstDistrictInputSite.x.FloorToInt() - 2,
                Plan.FirstDistrictInputSite.y.FloorToInt())),
            FutureWellChoice::Dormant, Feedback));
    TestTrue(TEXT("The guarded worker begins authoritative Well capture"),
        TickUntil([Bridge, Start, &Workers]()
        {
            const auto* Well = Bridge->FindEntity(Start.FutureWonWellId);
            const auto* Worker = Bridge->FindEntity(Workers[0]);
            return Well != nullptr && Worker != nullptr &&
                (Well->owner == UEchoesSimulationSubsystem::LocalPlayerId ||
                 (Well->wellCapturePlayer == UEchoesSimulationSubsystem::LocalPlayerId &&
                  Well->wellCaptureProgress > 0));
        }, 1000));
    TestTrue(
        TEXT("The activation enters the bounded stability hold"),
        TickUntil(
            [Bridge]()
            {
                return Bridge->GetFutureThatWonPhase() ==
                    EEchoesFutureThatWonPhase::HoldStabilityWindow;
            },
            2600));
    for (auto& Target : GuardTargetIds) Target = Start.FutureWonWellId;
    TestTrue(TEXT("Surviving escorts hold the committed Well independently of worker survival"), MaintainGuards());
    const FEchoesObjectiveSnapshot Activated =
        Bridge->GetLocalObjectiveSnapshot();
    TestTrue(
        TEXT("Schema 21 records the exact activation boundary"),
        Activated.bFutureWonProtocolBound &&
            Activated.FutureWonActivationTick > 0 &&
            Activated.FutureWonStabilityEndTick ==
                Activated.FutureWonActivationTick +
                    Plan.StabilityWindowTicks);
    TestTrue(
        TEXT("Two activation-state saves retain a recoverable prior generation"),
        Bridge->QuickSaveScenario(Feedback) &&
            Bridge->QuickSaveScenario(Feedback) &&
            IFileManager::Get().FileExists(
                *(QuickSavePath + TEXT(".bak"))));
    TArray<uint8> CorruptedCheckpoint;
    TestTrue(
        TEXT("The primary checkpoint can be corrupted for fallback verification"),
        FFileHelper::LoadFileToArray(
            CorruptedCheckpoint, *QuickSavePath) &&
            CorruptedCheckpoint.Num() > 16);
    if (CorruptedCheckpoint.Num() > 16)
    {
        CorruptedCheckpoint[16] ^= 0x5A;
        TestTrue(
            TEXT("The corrupted primary checkpoint is written"),
            FFileHelper::SaveArrayToFile(
                CorruptedCheckpoint, *QuickSavePath));
    }
    TestTrue(
        TEXT("Quick load falls back to the bound activation backup"),
        Bridge->QuickLoadScenario(Feedback) &&
            Bridge->GetFutureThatWonPhase() ==
                EEchoesFutureThatWonPhase::HoldStabilityWindow &&
            Bridge->GetLocalObjectiveSnapshot().FutureWonActivationTick ==
                Activated.FutureWonActivationTick);
    TestTrue(
        TEXT("The 300-tick apparatus hold opens paired observation"),
        TickUntil(
            [Bridge]()
            {
                return Bridge->GetFutureThatWonPhase() ==
                    EEchoesFutureThatWonPhase::ObserveDistrictReadbacks;
            },
            420));

    for (auto& Target : GuardTargetIds) Target = Start.FutureWonVerifierId;
    TestTrue(TEXT("Surviving escorts protect the verifier's final readback"), MaintainGuards());
    TestTrue(
        TEXT("Oruun reaches the first recorded district readback with surviving escorts regrouped"),
        PaceWitness(
            Start.FutureWonOruunId,
            0,
            Plan.FirstDistrictInputSite,
            1800));
    TestTrue(
        TEXT("The verifier reaches the second recorded district readback"),
        PaceWitness(
            Start.FutureWonVerifierId,
            2,
            Plan.SecondDistrictInputSite,
            1800));
    TestTrue(
        TEXT("Ordinary paired readback commits Mission 12"),
        TickUntil(
            [Bridge]()
            {
                return Bridge->GetCampaignProgress().FindDecision(
                           EEchoesCampaignMissionId::TheFutureThatWon) !=
                    nullptr;
            },
            6200));

    const FEchoesCampaignDecisionRecord* MissionRecord =
        Bridge->GetCampaignProgress().FindDecision(
            EEchoesCampaignMissionId::TheFutureThatWon);
    // The commit is written now, so it carries native schema-28 provenance.
    // The replay envelope shape did not change and stays at 24.
    TestTrue(
        TEXT("Mission 12 stores one recorded protocol, all eight facts, and schema-28 provenance"),
        MissionRecord != nullptr &&
            MissionRecord->WellChoice == FutureWellChoice::Preserve &&
            MissionRecord->AvailableWellChoices ==
                FutureThatWonChoiceMask(FutureWellChoice::Preserve) &&
            MissionRecord->VerifiedFacts == 0xFF &&
            MissionRecord->SimulationSnapshotVersion ==
                echoes::sim::kSnapshotVersion &&
            MissionRecord->CompletionTick > 0 &&
            MissionRecord->FinalStateChecksum != 0);
    FEchoesCampaignProgress Reloaded;
    TestTrue(
        TEXT("The twelve-record campaign reloads transactionally"),
        FEchoesCampaignProgressStore::LoadWithBackup(
            CampaignPath, Reloaded, Feedback) &&
            Reloaded.Decisions.Num() == 12);
    if (MissionRecord != nullptr)
    {
        FEchoesCampaignDecisionRecord ReplayConflict = *MissionRecord;
        ReplayConflict.WellChoice = FutureWellChoice::Harvest;
        ReplayConflict.AvailableWellChoices =
            FutureThatWonChoiceMask(FutureWellChoice::Harvest);
        ReplayConflict.CompletionTick += 1;
        ReplayConflict.FinalStateChecksum += 1;
        TestTrue(
            TEXT("A conflicting Mission 12 replay remains only a replay conflict"),
            Reloaded.AppendDecision(ReplayConflict, Feedback) ==
                EEchoesCampaignCommitStatus::ReplayConflict);
        TestTrue(
            TEXT("The replay conflict preserves the original Preserve record"),
            Reloaded.FindDecision(
                EEchoesCampaignMissionId::TheFutureThatWon) != nullptr &&
            Reloaded.FindDecision(
                EEchoesCampaignMissionId::TheFutureThatWon)->WellChoice ==
                FutureWellChoice::Preserve);
    }

    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors() && !WorldWrapper.HasFailed();
}

#endif
