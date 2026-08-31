#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "EchoesCampaignProgress.h"
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
    TestEqual(
        TEXT("Mission 12 accepts the current simulation snapshot schema"),
        echoes::sim::kSnapshotVersion,
        static_cast<uint32>(24));

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
    TestEqual(
        TEXT("Mission 12 retains the exact 223-tile Lume topology"),
        BlockedTiles,
        223);
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
    for (const echoes::sim::Entity& Entity :
         Bridge->GetSimulation()->Entities())
    {
        if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            Entity.type == echoes::sim::EntityType::Worker)
        {
            Workers.Add(Entity.id);
        }
    }
    if (!TestTrue(
            TEXT("At least two Kharuun construction workers are available"),
            Workers.Num() >= 2))
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
    const Vec2 FirstLinkSite = Vec2::FromTiles(
        Plan.FirstDistrictInputSite.x.FloorToInt() + 2,
        Plan.FirstDistrictInputSite.y.FloorToInt());
    const Vec2 SecondLinkSite = Vec2::FromTiles(
        Plan.SecondDistrictInputSite.x.FloorToInt() + 2,
        Plan.SecondDistrictInputSite.y.FloorToInt());

    Bridge->SetScenarioPaused(false);
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

    TestTrue(
        TEXT("Oruun accepts the Kharuun public readback"),
        Bridge->IssueCommand(
            echoes::sim::CommandType::Move,
            Start.FutureWonOruunId,
            0,
            Bridge->SimToWorld(Plan.KharuunReadbackSite),
            FutureWellChoice::Dormant,
            Feedback));
    TestTrue(
        TEXT("The verifier accepts the Meridian public readback"),
        Bridge->IssueCommand(
            echoes::sim::CommandType::Move,
            Start.FutureWonVerifierId,
            0,
            Bridge->SimToWorld(Plan.MeridianReadbackSite),
            FutureWellChoice::Dormant,
            Feedback));
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
        TEXT("The readback state survives a schema-24 quick save and load"),
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
        TEXT("The worker legitimately reveals the Future Well"),
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
    TestTrue(
        TEXT("The activation enters the bounded stability hold"),
        TickUntil(
            [Bridge]()
            {
                return Bridge->GetFutureThatWonPhase() ==
                    EEchoesFutureThatWonPhase::HoldStabilityWindow;
            },
            2600));
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

    TestTrue(
        TEXT("Oruun accepts the first recorded district readback"),
        Bridge->IssueCommand(
            echoes::sim::CommandType::Move,
            Start.FutureWonOruunId,
            0,
            Bridge->SimToWorld(Plan.FirstDistrictInputSite),
            FutureWellChoice::Dormant,
            Feedback));
    TestTrue(
        TEXT("The verifier accepts the second recorded district readback"),
        Bridge->IssueCommand(
            echoes::sim::CommandType::Move,
            Start.FutureWonVerifierId,
            0,
            Bridge->SimToWorld(Plan.SecondDistrictInputSite),
            FutureWellChoice::Dormant,
            Feedback));
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
    TestTrue(
        TEXT("Mission 12 stores one recorded protocol, all eight facts, and schema-24 provenance"),
        MissionRecord != nullptr &&
            MissionRecord->WellChoice == FutureWellChoice::Preserve &&
            MissionRecord->AvailableWellChoices ==
                FutureThatWonChoiceMask(FutureWellChoice::Preserve) &&
            MissionRecord->VerifiedFacts == 0xFF &&
            MissionRecord->SimulationSnapshotVersion == 24 &&
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
