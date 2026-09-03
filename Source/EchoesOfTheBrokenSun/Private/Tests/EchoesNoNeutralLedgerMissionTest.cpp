#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "EchoesCampaignProgress.h"
#include "EchoesNoNeutralLedgerMissionModel.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Tests/AutomationCommon.h"

namespace
{
struct FPreservedNoNeutralFile final
{
    explicit FPreservedNoNeutralFile(FString InPath) : Path(MoveTemp(InPath))
    {
        bExisted = IFileManager::Get().FileExists(*Path);
        if (bExisted)
        {
            FFileHelper::LoadFileToArray(Contents, *Path);
        }
    }

    ~FPreservedNoNeutralFile()
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

uint8 NoNeutralChoiceMask(echoes::sim::FutureWellChoice Choice)
{
    switch (Choice)
    {
        case echoes::sim::FutureWellChoice::Harvest: return 1 << 0;
        case echoes::sim::FutureWellChoice::Preserve: return 1 << 1;
        case echoes::sim::FutureWellChoice::Reshape: return 1 << 2;
        default: return 0;
    }
}

echoes::sim::Vec2 TestOwnedLedgerRallySite(
    echoes::sim::FutureWellChoice Choice)
{
    using echoes::sim::FutureWellChoice;
    using echoes::sim::Vec2;
    switch (Choice)
    {
        case FutureWellChoice::Harvest: return Vec2::FromTiles(18, 56);
        case FutureWellChoice::Preserve: return Vec2::FromTiles(32, 56);
        case FutureWellChoice::Reshape: return Vec2::FromTiles(32, 43);
        default: return {};
    }
}

FEchoesCampaignDecisionRecord MakeNoNeutralRecord(
    EEchoesCampaignMissionId Mission,
    echoes::sim::FutureWellChoice FoundingChoice,
    echoes::sim::FutureWellChoice LumeChoice,
    uint8 ReserveFacts = 0x7B)
{
    FEchoesCampaignDecisionRecord Record;
    Record.Mission = Mission;
    Record.WellChoice =
        Mission == EEchoesCampaignMissionId::ChoirAtLumeReach ||
                Mission == EEchoesCampaignMissionId::NoNeutralLedger
            ? LumeChoice
            : FoundingChoice;
    Record.AvailableWellChoices =
        Mission == EEchoesCampaignMissionId::WhatTheLedgerKeeps ||
                Mission == EEchoesCampaignMissionId::ChoirAtLumeReach
            ? 0x07
        : Mission == EEchoesCampaignMissionId::NoNeutralLedger
            ? NoNeutralChoiceMask(LumeChoice)
            : NoNeutralChoiceMask(FoundingChoice);
    Record.SimulationSnapshotVersion = echoes::sim::kSnapshotVersion;
    Record.CompletionTick = 100 + static_cast<uint8>(Mission) * 300;
    Record.FinalStateChecksum =
        0x11A1100ULL + static_cast<uint8>(Mission);
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
            Record.VerifiedFacts = 0xFF;
            break;
    }
    return Record;
}

FEchoesCampaignProgress MakeNoNeutralPrerequisites(
    echoes::sim::FutureWellChoice FoundingChoice,
    echoes::sim::FutureWellChoice LumeChoice,
    uint8 ReserveFacts,
    int32 ThroughMission,
    FString& OutFeedback)
{
    FEchoesCampaignProgress Progress;
    for (int32 MissionValue = 1;
         MissionValue <= FMath::Min(ThroughMission, 10);
         ++MissionValue)
    {
        Progress.AppendDecision(
            MakeNoNeutralRecord(
                static_cast<EEchoesCampaignMissionId>(MissionValue),
                FoundingChoice,
                LumeChoice,
                ReserveFacts),
            OutFeedback);
    }
    return Progress;
}

FString NoNeutralQuickSavePath(const FEchoesCampaignProgress& Progress)
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
            TEXT("EchoesQuickSaveNoNeutralLedger-%08X.bin"),
            Fingerprint));
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesNoNeutralLedgerMissionTest,
    "Echoes.Runtime.Campaign.NoNeutralLedger",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesNoNeutralLedgerMissionTest::RunTest(const FString& Parameters)
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
                FEchoesNoNeutralLedgerPlan Plan;
                const bool bPlanned =
                    FEchoesNoNeutralLedgerMissionModel::TryPlanForLedger(
                        FoundingChoice,
                        ReserveFacts,
                        LumeChoice,
                        Plan);
                TestTrue(
                    TEXT("Every founding, district-pair, and Lume tuple has an authored plan"),
                    bPlanned && Plan.RouteSite != Vec2{} &&
                        Plan.FirstDistrictSite != Vec2{} &&
                        Plan.SecondDistrictSite != Vec2{} &&
                        Plan.MeridianEvidenceSite ==
                            Vec2::FromTiles(26, 43) &&
                        Plan.KharuunEvidenceSite ==
                            Vec2::FromTiles(38, 43) &&
                        Plan.FutureWellSite == Vec2::FromTiles(32, 49) &&
                        Plan.RallySite ==
                            TestOwnedLedgerRallySite(LumeChoice));
                TestTrue(
                    TEXT("Every alliance rally stays distinct from contributing districts and public evidence readbacks"),
                    Plan.RallySite != Plan.FirstDistrictSite &&
                        Plan.RallySite != Plan.SecondDistrictSite &&
                        Plan.RallySite != Plan.MeridianEvidenceSite &&
                        Plan.RallySite != Plan.KharuunEvidenceSite);
                PlanKeys.Add(Plan.StablePlanKey);
                ++PlanContracts;
            }
        }
    }
    TestEqual(
        TEXT("The acceptance matrix contains exactly 27 plans"),
        PlanContracts,
        27);
    TestEqual(
        TEXT("All 27 plans have stable unique keys"),
        PlanKeys.Num(),
        27);
    FEchoesNoNeutralLedgerPlan Plan17;
    TestTrue(
        TEXT("Plan 17 maps Preserve and Reshape to the literal authored central rally"),
        FEchoesNoNeutralLedgerMissionModel::TryPlanForLedger(
            FutureWellChoice::Preserve,
            0x7B,
            FutureWellChoice::Reshape,
            Plan17) &&
            Plan17.StablePlanKey == 17 &&
            Plan17.FoundingDoctrine == FutureWellChoice::Preserve &&
            Plan17.LumeProtocol == FutureWellChoice::Reshape &&
            Plan17.RallySite == Vec2::FromTiles(32, 43));
    FEchoesNoNeutralLedgerPlan InvalidPlan;
    TestFalse(
        TEXT("A reserve record with fewer than two contributing districts is rejected"),
        FEchoesNoNeutralLedgerMissionModel::TryPlanForLedger(
            FutureWellChoice::Harvest,
            0x79,
            FutureWellChoice::Preserve,
            InvalidPlan));
    TestFalse(
        TEXT("A dormant Lume protocol cannot become an alliance plan"),
        FEchoesNoNeutralLedgerMissionModel::TryPlanForLedger(
            FutureWellChoice::Harvest,
            0x7B,
            FutureWellChoice::Dormant,
            InvalidPlan));

    FEchoesNoNeutralLedgerMissionFacts Facts;
    TestTrue(
        TEXT("Inactive facts stay outside Mission 11"),
        FEchoesNoNeutralLedgerMissionModel::DeterminePhase(Facts) ==
            EEchoesNoNeutralLedgerPhase::Inactive);
    Facts.bOperationActive = true;
    Facts.bLocalCoreIntact = true;
    Facts.bOruunIntact = true;
    Facts.bWaystoneIntact = true;
    Facts.bLedgerWitnessIntact = true;
    Facts.bFutureWellIntact = true;
    Facts.bPublicInterfacesIntact = true;
    Facts.bSkirmishStillOngoing = true;
    TestTrue(
        TEXT("The reducer begins at inherited-route security"),
        FEchoesNoNeutralLedgerMissionModel::DeterminePhase(Facts) ==
            EEchoesNoNeutralLedgerPhase::SecureInheritedRoute);
    Facts.bInheritedRouteSecured = true;
    TestTrue(
        TEXT("The secured route opens district integration"),
        FEchoesNoNeutralLedgerMissionModel::DeterminePhase(Facts) ==
            EEchoesNoNeutralLedgerPhase::IntegrateDistrictContributions);
    Facts.bFirstDistrictIntegrated = true;
    Facts.bSecondDistrictIntegrated = true;
    TestTrue(
        TEXT("Both district contributions open public attestation"),
        FEchoesNoNeutralLedgerMissionModel::DeterminePhase(Facts) ==
            EEchoesNoNeutralLedgerPhase::AttestEvidenceChannels);
    Facts.bBothEvidenceChannelsAttested = true;
    TestTrue(
        TEXT("Both evidence channels open the recorded protocol"),
        FEchoesNoNeutralLedgerMissionModel::DeterminePhase(Facts) ==
            EEchoesNoNeutralLedgerPhase::ApplyRecordedProtocol);
    Facts.bRecordedProtocolApplied = true;
    TestTrue(
        TEXT("The recorded protocol opens the coalition rally"),
        FEchoesNoNeutralLedgerMissionModel::DeterminePhase(Facts) ==
            EEchoesNoNeutralLedgerPhase::RallyCoalition);
    Facts.bCoalitionRallied = true;
    TestTrue(
        TEXT("Only the completed rally succeeds"),
        FEchoesNoNeutralLedgerMissionModel::DeterminePhase(Facts) ==
            EEchoesNoNeutralLedgerPhase::Complete);
    Facts.bCoalitionRallied = false;
    Facts.bConflictingProtocolApplied = true;
    TestTrue(
        TEXT("A conflicting protocol fails closed"),
        FEchoesNoNeutralLedgerMissionModel::DeterminePhase(Facts) ==
            EEchoesNoNeutralLedgerPhase::Failed);
    Facts.bConflictingProtocolApplied = false;
    Facts.bReshapeWindowExpired = true;
    TestTrue(
        TEXT("An expired unresolved Reshape rally fails closed"),
        FEchoesNoNeutralLedgerMissionModel::DeterminePhase(Facts) ==
            EEchoesNoNeutralLedgerPhase::Failed);
    Facts.bReshapeWindowExpired = false;
    Facts.bLedgerWitnessIntact = false;
    TestTrue(
        TEXT("Ledger-witness loss fails the operation"),
        FEchoesNoNeutralLedgerMissionModel::DeterminePhase(Facts) ==
            EEchoesNoNeutralLedgerPhase::Failed);
    Facts.bLedgerWitnessIntact = true;
    Facts.bPublicInterfacesIntact = false;
    TestTrue(
        TEXT("Public-interface loss fails the operation"),
        FEchoesNoNeutralLedgerMissionModel::DeterminePhase(Facts) ==
            EEchoesNoNeutralLedgerPhase::Failed);
    Facts.bPublicInterfacesIntact = true;
    Facts.bLocalCoreIntact = false;
    TestTrue(
        TEXT("Local-Core loss fails the operation"),
        FEchoesNoNeutralLedgerMissionModel::DeterminePhase(Facts) ==
            EEchoesNoNeutralLedgerPhase::Failed);

    TestEqual(
        TEXT("Campaign persistence uses the current schema"),
        FEchoesCampaignProgress::SchemaVersion,
        static_cast<uint16>(2));
    // Per-player terrain and object memory is now serialized into the
    // snapshot, so the native snapshot schema advanced from 24 to 25.
    // The replay envelope shape did not change and stays at 24; this
    // assertion pins the snapshot schema only.
    TestEqual(
        TEXT("Simulation snapshot schema advances to twenty-five"),
        echoes::sim::kSnapshotVersion,
        static_cast<uint32>(25));

    const FString CampaignPath =
        FEchoesCampaignProgressStore::GetDefaultPath();
    FPreservedNoNeutralFile PreservedPrimary(CampaignPath);
    FPreservedNoNeutralFile PreservedBackup(CampaignPath + TEXT(".bak"));
    FPreservedNoNeutralFile PreservedTemporary(CampaignPath + TEXT(".tmp"));
    IFileManager::Get().Delete(*CampaignPath, false, true, true);
    IFileManager::Get().Delete(
        *(CampaignPath + TEXT(".bak")), false, true, true);
    IFileManager::Get().Delete(
        *(CampaignPath + TEXT(".tmp")), false, true, true);

    FString Feedback;
    FEchoesCampaignProgress NineRecords = MakeNoNeutralPrerequisites(
        FutureWellChoice::Harvest,
        FutureWellChoice::Preserve,
        0x7B,
        9,
        Feedback);
    TestEqual(
        TEXT("The lock fixture contains exactly nine records"),
        NineRecords.Decisions.Num(),
        9);
    TestTrue(
        TEXT("The nine-record fixture is stored"),
        FEchoesCampaignProgressStore::SaveAtomic(
            CampaignPath, NineRecords, Feedback));
    {
        FTestWorldWrapper LockedWorld;
        if (!LockedWorld.CreateTestWorld(EWorldType::Game))
        {
            LockedWorld.ForwardErrorMessages(this);
            AddError(TEXT("Could not create the locked Mission 11 world."));
            return false;
        }
        UEchoesSimulationSubsystem* LockedBridge =
            LockedWorld.GetTestWorld()->GetSubsystem<
                UEchoesSimulationSubsystem>();
        TestFalse(
            TEXT("Mission 11 rejects a nine-record ledger"),
            LockedBridge != nullptr &&
                LockedBridge->SelectOperationMode(
                    EEchoesOperationMode::CampaignNoNeutralLedger,
                    Feedback));
        TestTrue(
            TEXT("The lock response names The Choir at Lume Reach"),
            Feedback.Contains(TEXT("Choir at Lume Reach")));
        LockedWorld.ForwardErrorMessages(this);
    }

    FEchoesCampaignProgress TenRecords = MakeNoNeutralPrerequisites(
        FutureWellChoice::Harvest,
        FutureWellChoice::Preserve,
        0x7B,
        10,
        Feedback);
    TestEqual(
        TEXT("The admission fixture contains exactly ten records"),
        TenRecords.Decisions.Num(),
        10);
    TestTrue(
        TEXT("Mission 10 retains its independent Preserve choice"),
        TenRecords.Decisions[0].WellChoice ==
                FutureWellChoice::Harvest &&
            TenRecords.Decisions[9].WellChoice ==
                FutureWellChoice::Preserve &&
            TenRecords.Decisions[9].AvailableWellChoices == 0x07);

    FEchoesCampaignDecisionRecord InvalidAlliance =
        MakeNoNeutralRecord(
            EEchoesCampaignMissionId::NoNeutralLedger,
            FutureWellChoice::Harvest,
            FutureWellChoice::Preserve);
    InvalidAlliance.AvailableWellChoices = 0x07;
    FEchoesCampaignProgress InvalidProgress = TenRecords;
    TestTrue(
        TEXT("Mission 11 cannot claim all three protocols were available"),
        InvalidProgress.AppendDecision(InvalidAlliance, Feedback) ==
            EEchoesCampaignCommitStatus::StorageFailure);
    InvalidAlliance = MakeNoNeutralRecord(
        EEchoesCampaignMissionId::NoNeutralLedger,
        FutureWellChoice::Harvest,
        FutureWellChoice::Harvest);
    TestTrue(
        TEXT("Mission 11 cannot rewrite Mission 10's Lume protocol"),
        InvalidProgress.AppendDecision(InvalidAlliance, Feedback) ==
            EEchoesCampaignCommitStatus::StorageFailure &&
            Feedback.Contains(TEXT("LUME_PROTOCOL")));
    InvalidAlliance = MakeNoNeutralRecord(
        EEchoesCampaignMissionId::NoNeutralLedger,
        FutureWellChoice::Harvest,
        FutureWellChoice::Preserve);
    InvalidAlliance.VerifiedFacts = 0xFE;
    TestTrue(
        TEXT("Mission 11 rejects any missing completion fact"),
        InvalidProgress.AppendDecision(InvalidAlliance, Feedback) ==
            EEchoesCampaignCommitStatus::StorageFailure);

    FEchoesCampaignProgress SyntheticComplete = TenRecords;
    const FEchoesCampaignDecisionRecord ValidAlliance =
        MakeNoNeutralRecord(
            EEchoesCampaignMissionId::NoNeutralLedger,
            FutureWellChoice::Harvest,
            FutureWellChoice::Preserve);
    TestTrue(
        TEXT("An exact single-protocol eleven-record chain validates"),
        SyntheticComplete.AppendDecision(ValidAlliance, Feedback) ==
            EEchoesCampaignCommitStatus::Added);
    TArray<uint8> EncodedComplete;
    TestTrue(
        TEXT("The exact chain encodes without changing campaign schema"),
        FEchoesCampaignProgressStore::Encode(
            SyntheticComplete, EncodedComplete, Feedback));
    FEchoesCampaignProgress DecodedComplete;
    TestTrue(
        TEXT("The exact eleven-record chain decodes losslessly"),
        FEchoesCampaignProgressStore::Decode(
            EncodedComplete, DecodedComplete, Feedback) &&
            DecodedComplete.Decisions.Num() == 11 &&
            DecodedComplete.Decisions[10].AvailableWellChoices ==
                NoNeutralChoiceMask(FutureWellChoice::Preserve) &&
            DecodedComplete.Decisions[10].VerifiedFacts == 0xFF);

    FEchoesCampaignProgress AlternateTen = MakeNoNeutralPrerequisites(
        FutureWellChoice::Reshape,
        FutureWellChoice::Preserve,
        0x7D,
        10,
        Feedback);
    const FString QuickSavePath = NoNeutralQuickSavePath(TenRecords);
    const FString AlternateQuickSavePath =
        NoNeutralQuickSavePath(AlternateTen);
    TestTrue(
        TEXT("Different ten-record chains retain distinct checkpoint namespaces"),
        !QuickSavePath.IsEmpty() && !AlternateQuickSavePath.IsEmpty() &&
            QuickSavePath != AlternateQuickSavePath);
    FPreservedNoNeutralFile PreservedQuickSave(QuickSavePath);
    FPreservedNoNeutralFile PreservedQuickSaveBackup(
        QuickSavePath + TEXT(".bak"));
    FPreservedNoNeutralFile PreservedQuickSaveTemporary(
        QuickSavePath + TEXT(".tmp"));
    FPreservedNoNeutralFile PreservedAlternateQuickSave(
        AlternateQuickSavePath);
    FPreservedNoNeutralFile PreservedAlternateQuickSaveBackup(
        AlternateQuickSavePath + TEXT(".bak"));
    FPreservedNoNeutralFile PreservedAlternateQuickSaveTemporary(
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
        TEXT("The exact ten-record ledger is stored"),
        FEchoesCampaignProgressStore::SaveAtomic(
            CampaignPath, TenRecords, Feedback));
    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the No Neutral Ledger test world."));
        return false;
    }
    UEchoesSimulationSubsystem* Bridge =
        WorldWrapper.GetTestWorld()->GetSubsystem<
            UEchoesSimulationSubsystem>();
    if (!TestNotNull(TEXT("Mission world owns the simulation subsystem"), Bridge) ||
        !TestTrue(
            TEXT("Ten exact records unlock Mission 11"),
            Bridge != nullptr && Bridge->IsNoNeutralLedgerUnlocked()) ||
        !TestTrue(
            TEXT("Mission 11 operation can be selected"),
            Bridge->SelectOperationMode(
                EEchoesOperationMode::CampaignNoNeutralLedger,
                Feedback)) ||
        !TestTrue(
            TEXT("Mission 11 scenario starts"),
            Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const FEchoesNoNeutralLedgerPlan Plan =
        Bridge->GetNoNeutralLedgerPlan();
    const Vec2 FirstDistrictLinkSite = Vec2::FromTiles(
        Plan.FirstDistrictSite.x.FloorToInt() + 2,
        Plan.FirstDistrictSite.y.FloorToInt());
    const Vec2 SecondDistrictLinkSite = Vec2::FromTiles(
        Plan.SecondDistrictSite.x.FloorToInt() + 2,
        Plan.SecondDistrictSite.y.FloorToInt());
    const FEchoesObjectiveSnapshot Start =
        Bridge->GetLocalObjectiveSnapshot();
    TestTrue(
        TEXT("The independent Harvest/Preserve pair selects stable plan 07"),
        Plan.StablePlanKey == 7 &&
            Plan.FoundingDoctrine == FutureWellChoice::Harvest &&
            Plan.LumeProtocol == FutureWellChoice::Preserve &&
            Plan.RouteSite == Vec2::FromTiles(18, 30) &&
            Plan.FirstContributingDistrict ==
                EEchoesCityDistrict::LifeSupport &&
            Plan.SecondContributingDistrict ==
                EEchoesCityDistrict::Transit &&
            Plan.DeferredDistrict == EEchoesCityDistrict::Archive);
    TestTrue(
        TEXT("Mission 11 locks one Kharuun authority"),
        Bridge->GetLocalFaction() ==
            echoes::sim::Faction::KharuunAssemblies);
    TestTrue(
        TEXT("Command, public-interface, and Well identities are nonzero"),
        Start.NoNeutralOruunId != 0 &&
            Start.NoNeutralWaystoneId != 0 &&
            Start.NoNeutralLedgerWitnessId != 0 &&
            Start.NoNeutralFirstDistrictInterfaceId != 0 &&
            Start.NoNeutralSecondDistrictInterfaceId != 0 &&
            Start.NoNeutralMeridianEvidenceInterfaceId != 0 &&
            Start.NoNeutralKharuunEvidenceInterfaceId != 0 &&
            Start.NoNeutralWellId != 0 &&
            Start.bNoNeutralPublicInterfacesIntact);
    const echoes::sim::Entity* Oruun =
        Bridge->FindEntity(Start.NoNeutralOruunId);
    const echoes::sim::Entity* Waystone =
        Bridge->FindEntity(Start.NoNeutralWaystoneId);
    const echoes::sim::Entity* Witness =
        Bridge->FindEntity(Start.NoNeutralLedgerWitnessId);
    const echoes::sim::Entity* FirstDistrictInterface =
        Bridge->FindEntity(Start.NoNeutralFirstDistrictInterfaceId);
    const echoes::sim::Entity* SecondDistrictInterface =
        Bridge->FindEntity(Start.NoNeutralSecondDistrictInterfaceId);
    const echoes::sim::Entity* MeridianEvidenceInterface =
        Bridge->FindEntity(Start.NoNeutralMeridianEvidenceInterfaceId);
    const echoes::sim::Entity* KharuunEvidenceInterface =
        Bridge->FindEntity(Start.NoNeutralKharuunEvidenceInterfaceId);
    const echoes::sim::Entity* Well =
        Bridge->FindEntity(Start.NoNeutralWellId);
    TestTrue(
        TEXT("Only the Kharuun force is commandable; contribution interfaces and the Well are neutral"),
        Oruun != nullptr &&
            Oruun->owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            Oruun->faction == echoes::sim::Faction::KharuunAssemblies &&
            Oruun->type == echoes::sim::EntityType::ScoutUnit &&
        Waystone != nullptr &&
            Waystone->owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            Waystone->faction == echoes::sim::Faction::KharuunAssemblies &&
            Waystone->type == echoes::sim::EntityType::Dropoff &&
        Witness != nullptr &&
            Witness->owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            Witness->faction == echoes::sim::Faction::KharuunAssemblies &&
            Witness->type == echoes::sim::EntityType::ScoutUnit &&
        FirstDistrictInterface != nullptr &&
            FirstDistrictInterface->owner == echoes::sim::kNeutralPlayer &&
            FirstDistrictInterface->faction ==
                echoes::sim::Faction::MeridianCompact &&
            FirstDistrictInterface->type ==
                echoes::sim::EntityType::UtilityStructure &&
            FirstDistrictInterface->position == Plan.FirstDistrictSite &&
            FirstDistrictInterface->aegisPowered &&
            FirstDistrictInterface->attackRangeRaw == 0 &&
            FirstDistrictInterface->attackDamage == 0 &&
            FirstDistrictInterface->visionTiles == 0 &&
        SecondDistrictInterface != nullptr &&
            SecondDistrictInterface->owner == echoes::sim::kNeutralPlayer &&
            SecondDistrictInterface->faction ==
                echoes::sim::Faction::MeridianCompact &&
            SecondDistrictInterface->type ==
                echoes::sim::EntityType::UtilityStructure &&
            SecondDistrictInterface->position == Plan.SecondDistrictSite &&
            SecondDistrictInterface->aegisPowered &&
        MeridianEvidenceInterface != nullptr &&
            MeridianEvidenceInterface->owner ==
                echoes::sim::kNeutralPlayer &&
            MeridianEvidenceInterface->faction ==
                echoes::sim::Faction::MeridianCompact &&
            MeridianEvidenceInterface->position ==
                Plan.MeridianEvidenceSite &&
            MeridianEvidenceInterface->aegisPowered &&
        KharuunEvidenceInterface != nullptr &&
            KharuunEvidenceInterface->owner ==
                echoes::sim::kNeutralPlayer &&
            KharuunEvidenceInterface->faction ==
                echoes::sim::Faction::KharuunAssemblies &&
            KharuunEvidenceInterface->position ==
                Plan.KharuunEvidenceSite &&
            !KharuunEvidenceInterface->aegisPowered &&
        Well != nullptr &&
            Well->owner == echoes::sim::kNeutralPlayer &&
            Well->type == echoes::sim::EntityType::FutureWell &&
            Well->position == Plan.FutureWellSite);
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
        TEXT("Mission 11 retains the exact 223-tile Lume topology"),
        BlockedTiles,
        223);
    TestTrue(
        TEXT("Mission 11 begins at inherited-route security"),
        Bridge->GetNoNeutralLedgerPhase() ==
            EEchoesNoNeutralLedgerPhase::SecureInheritedRoute);
    TestTrue(
        TEXT("The initial state quick-saves into the ten-record namespace"),
        Bridge->QuickSaveScenario(Feedback) &&
            IFileManager::Get().FileExists(*QuickSavePath));
    TestTrue(
        TEXT("The initial reducer state reconstructs after quick load"),
        Bridge->QuickLoadScenario(Feedback) &&
            Bridge->GetNoNeutralLedgerPhase() ==
                EEchoesNoNeutralLedgerPhase::SecureInheritedRoute);

    TArray<uint8> CheckpointBytes;
    TestTrue(
        TEXT("The checkpoint can seed an explicit cross-ledger probe"),
        FFileHelper::LoadFileToArray(CheckpointBytes, *QuickSavePath) &&
            FFileHelper::SaveArrayToFile(
                CheckpointBytes, *AlternateQuickSavePath) &&
            FFileHelper::SaveArrayToFile(
                CheckpointBytes,
                *(AlternateQuickSavePath + TEXT(".bak"))));
    TestTrue(
        TEXT("The alternate ten-record campaign is stored for the probe"),
        FEchoesCampaignProgressStore::SaveAtomic(
            CampaignPath, AlternateTen, Feedback));
    {
        FTestWorldWrapper AlternateWorld;
        if (!AlternateWorld.CreateTestWorld(EWorldType::Game))
        {
            AlternateWorld.ForwardErrorMessages(this);
            AddError(TEXT("Could not create the alternate Mission 11 world."));
            return false;
        }
        UEchoesSimulationSubsystem* AlternateBridge =
            AlternateWorld.GetTestWorld()->GetSubsystem<
                UEchoesSimulationSubsystem>();
        if (!TestNotNull(
                TEXT("The alternate world owns a simulation subsystem"),
                AlternateBridge) ||
            !TestTrue(
                TEXT("The alternate campaign can select Mission 11"),
                AlternateBridge->SelectOperationMode(
                    EEchoesOperationMode::CampaignNoNeutralLedger,
                    Feedback)) ||
            !TestTrue(
                TEXT("The alternate campaign can start Mission 11"),
                AlternateBridge->StartPrototypeScenario()))
        {
            AlternateWorld.ForwardErrorMessages(this);
            return false;
        }
        TestFalse(
            TEXT("A renamed checkpoint cannot cross ten-record chains"),
            AlternateBridge->QuickLoadScenario(Feedback));
        TestTrue(
            TEXT("Cross-ledger loading reports exact binding rejection"),
            Feedback.Contains(TEXT("ledger binding does not match")));
        AlternateBridge->StopPrototypeScenario();
        AlternateWorld.ForwardErrorMessages(this);
    }
    TestTrue(
        TEXT("The accepted ten-record campaign is restored"),
        FEchoesCampaignProgressStore::SaveAtomic(
            CampaignPath, TenRecords, Feedback));

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
    const auto IsAtSite = [Bridge](echoes::sim::EntityId EntityId,
                                   const Vec2& Site)
    {
        const echoes::sim::Entity* Entity = Bridge->FindEntity(EntityId);
        if (Entity == nullptr)
        {
            return false;
        }
        return FMath::Abs(Entity->position.x.Raw() - Site.x.Raw()) <=
                   echoes::sim::kFixedScale / 2 &&
               FMath::Abs(Entity->position.y.Raw() - Site.y.Raw()) <=
                   echoes::sim::kFixedScale / 2;
    };

    Bridge->SetScenarioPaused(false);
    TestFalse(
        TEXT("The local player cannot command a public district interface"),
        Bridge->IssueCommand(
            echoes::sim::CommandType::Move,
            Start.NoNeutralFirstDistrictInterfaceId,
            0,
            Bridge->SimToWorld(FirstDistrictLinkSite),
            FutureWellChoice::Dormant,
            Feedback));
    TestFalse(
        TEXT("District construction is locked before route security"),
        Bridge->IssueBuildCommand(
            Workers[0],
            echoes::sim::EntityType::UtilityStructure,
            Bridge->SimToWorld(Plan.FirstDistrictSite),
            Feedback));
    TestTrue(
        TEXT("The early district rejection names the route prerequisite"),
        Feedback.Contains(TEXT("NO_NEUTRAL_LEDGER_ROUTE_REQUIRED")));
    TestFalse(
        TEXT("The Lume protocol is locked before evidence attestation"),
        Bridge->IssueCommand(
            echoes::sim::CommandType::FutureWell,
            Workers[0],
            Start.NoNeutralWellId,
            Bridge->SimToWorld(Plan.FutureWellSite),
            FutureWellChoice::Preserve,
            Feedback));
    TestTrue(
        TEXT("The early Well rejection is reason-coded"),
        Feedback.Contains(TEXT("NO_NEUTRAL_LEDGER_PROTOCOL_REQUIRED")));

    TestTrue(
        TEXT("The rooted Waystone accepts an ordinary uproot command"),
        Bridge->IssueCommand(
            echoes::sim::CommandType::ToggleWaystoneRoot,
            Start.NoNeutralWaystoneId,
            0,
            FVector::ZeroVector,
            FutureWellChoice::Dormant,
            Feedback));
    TestTrue(
        TEXT("The Waystone completes its mobile transition"),
        TickUntil(
            [Bridge, Start]()
            {
                const echoes::sim::Entity* Current =
                    Bridge->FindEntity(Start.NoNeutralWaystoneId);
                return Current != nullptr &&
                    Current->waystoneMode ==
                        echoes::sim::WaystoneMode::Mobile;
            },
            500));
    TestTrue(
        TEXT("The mobile Waystone accepts the inherited route"),
        Bridge->IssueCommand(
            echoes::sim::CommandType::Move,
            Start.NoNeutralWaystoneId,
            0,
            Bridge->SimToWorld(Plan.RouteSite),
            FutureWellChoice::Dormant,
            Feedback));
    TestTrue(
        TEXT("The Waystone reaches the inherited route"),
        TickUntil(
            [&IsAtSite, Start, Plan]()
            {
                return IsAtSite(
                    Start.NoNeutralWaystoneId,
                    Plan.RouteSite);
            },
            5200));
    TestTrue(
        TEXT("The inherited route accepts the root command"),
        Bridge->IssueCommand(
            echoes::sim::CommandType::ToggleWaystoneRoot,
            Start.NoNeutralWaystoneId,
            0,
            FVector::ZeroVector,
            FutureWellChoice::Dormant,
            Feedback));
    TestTrue(
        TEXT("Rooting the Waystone opens district integration"),
        TickUntil(
            [Bridge]()
            {
                return Bridge->GetNoNeutralLedgerPhase() ==
                    EEchoesNoNeutralLedgerPhase::
                        IntegrateDistrictContributions;
            },
            600));
    TestFalse(
        TEXT("A third unrecorded district site is rejected"),
        Bridge->IssueBuildCommand(
            Workers[0],
            echoes::sim::EntityType::UtilityStructure,
            Bridge->SimToWorld(
                FEchoesNoNeutralLedgerMissionModel::
                    DistrictContributionSite(
                        EEchoesCityDistrict::Archive)),
            Feedback));
    TestTrue(
        TEXT("The unrecorded district rejection is reason-coded"),
        Feedback.Contains(TEXT("NO_NEUTRAL_LEDGER_DISTRICT_SITE")));
    TestTrue(
        TEXT("A worker accepts a Kharuun link beside the first public district interface"),
        Bridge->IssueBuildCommand(
            Workers[0],
            echoes::sim::EntityType::UtilityStructure,
            Bridge->SimToWorld(FirstDistrictLinkSite),
            Feedback));
    TestTrue(
        TEXT("A second worker accepts a Kharuun link beside the second public district interface"),
        Bridge->IssueBuildCommand(
            Workers[1],
            echoes::sim::EntityType::UtilityStructure,
            Bridge->SimToWorld(SecondDistrictLinkSite),
            Feedback));
    TestTrue(
        TEXT("Both completed district interfaces open evidence attestation"),
        TickUntil(
            [Bridge]()
            {
                return Bridge->GetNoNeutralLedgerPhase() ==
                    EEchoesNoNeutralLedgerPhase::AttestEvidenceChannels;
            },
            5600));
    TestTrue(
        TEXT("The integrated state quick-saves"),
        Bridge->QuickSaveScenario(Feedback));
    TestTrue(
        TEXT("Quick load reconstructs both district contributions"),
        Bridge->QuickLoadScenario(Feedback) &&
            Bridge->GetNoNeutralLedgerPhase() ==
                EEchoesNoNeutralLedgerPhase::AttestEvidenceChannels &&
            Bridge->GetLocalObjectiveSnapshot().
                bNoNeutralFirstDistrictIntegrated &&
            Bridge->GetLocalObjectiveSnapshot().
                bNoNeutralSecondDistrictIntegrated);

    TestTrue(
        TEXT("Oruun accepts the Kharuun public evidence site"),
        Bridge->IssueCommand(
            echoes::sim::CommandType::Move,
            Start.NoNeutralOruunId,
            0,
            Bridge->SimToWorld(Plan.KharuunEvidenceSite),
            FutureWellChoice::Dormant,
            Feedback));
    TestTrue(
        TEXT("The ledger witness accepts the Meridian public evidence site"),
        Bridge->IssueCommand(
            echoes::sim::CommandType::Move,
            Start.NoNeutralLedgerWitnessId,
            0,
            Bridge->SimToWorld(Plan.MeridianEvidenceSite),
            FutureWellChoice::Dormant,
            Feedback));
    TestTrue(
        TEXT("Simultaneous public attestation opens the recorded protocol"),
        TickUntil(
            [Bridge]()
            {
                return Bridge->GetNoNeutralLedgerPhase() ==
                    EEchoesNoNeutralLedgerPhase::ApplyRecordedProtocol;
            },
            5400));
    TestTrue(
        TEXT("A worker accepts an ordinary approach to the Lume Well"),
        Bridge->IssueCommand(
            echoes::sim::CommandType::Move,
            Workers[0],
            0,
            Bridge->SimToWorld(Vec2::FromTiles(30, 47)),
            FutureWellChoice::Dormant,
            Feedback));
    TestTrue(
        TEXT("The worker legitimately reveals the Lume Well"),
        TickUntil(
            [Bridge, Start]()
            {
                return Bridge->GetSimulation()->IsEntityVisibleTo(
                    UEchoesSimulationSubsystem::LocalPlayerId,
                    Start.NoNeutralWellId);
            },
            2200));
    TestFalse(
        TEXT("A non-recorded Harvest protocol is rejected"),
        Bridge->IssueCommand(
            echoes::sim::CommandType::FutureWell,
            Workers[0],
            Start.NoNeutralWellId,
            Bridge->SimToWorld(Plan.FutureWellSite),
            FutureWellChoice::Harvest,
            Feedback));
    TestTrue(
        TEXT("The protocol rejection names the recorded Preserve choice"),
        Feedback.Contains(TEXT("NO_NEUTRAL_LEDGER_PROTOCOL_REQUIRED")) &&
            Feedback.Contains(Plan.ProtocolDisplayName));
    TestTrue(
        TEXT("The exact recorded Preserve protocol is accepted"),
        Bridge->IssueCommand(
            echoes::sim::CommandType::FutureWell,
            Workers[0],
            Start.NoNeutralWellId,
            Bridge->SimToWorld(Plan.FutureWellSite),
            FutureWellChoice::Preserve,
            Feedback));
    TestTrue(
        TEXT("The applied protocol opens the coalition rally"),
        TickUntil(
            [Bridge]()
            {
                return Bridge->GetNoNeutralLedgerPhase() ==
                    EEchoesNoNeutralLedgerPhase::RallyCoalition;
            },
            2600));
    TestTrue(
        TEXT("The applied protocol state quick-saves"),
        Bridge->QuickSaveScenario(Feedback));
    TestTrue(
        TEXT("Quick load reconstructs durable attestation and the rally gate"),
        Bridge->QuickLoadScenario(Feedback) &&
            Bridge->GetNoNeutralLedgerPhase() ==
                EEchoesNoNeutralLedgerPhase::RallyCoalition &&
            Bridge->GetLocalObjectiveSnapshot().
                bNoNeutralEvidenceAttested &&
            Bridge->GetLocalObjectiveSnapshot().
                bNoNeutralProtocolApplied);
    TestTrue(
        TEXT("Oruun accepts the protocol-specific rally site"),
        Bridge->IssueCommand(
            echoes::sim::CommandType::Move,
            Start.NoNeutralOruunId,
            0,
            Bridge->SimToWorld(Plan.RallySite),
            FutureWellChoice::Dormant,
            Feedback));
    TestTrue(
        TEXT("The ledger witness accepts the same rally site"),
        Bridge->IssueCommand(
            echoes::sim::CommandType::Move,
            Start.NoNeutralLedgerWitnessId,
            0,
            Bridge->SimToWorld(Plan.RallySite),
            FutureWellChoice::Dormant,
            Feedback));
    TestTrue(
        TEXT("The ordinary rally route commits Mission 11"),
        TickUntil(
            [Bridge]()
            {
                return Bridge->GetCampaignProgress().FindDecision(
                           EEchoesCampaignMissionId::NoNeutralLedger) !=
                    nullptr;
            },
            6200));

    const FEchoesCampaignDecisionRecord* MissionRecord =
        Bridge->GetCampaignProgress().FindDecision(
            EEchoesCampaignMissionId::NoNeutralLedger);
    TestTrue(
        TEXT("Mission 11 stores the exact single-bit Lume protocol and all eight facts"),
        MissionRecord != nullptr &&
            MissionRecord->WellChoice == FutureWellChoice::Preserve &&
            MissionRecord->AvailableWellChoices ==
                NoNeutralChoiceMask(FutureWellChoice::Preserve) &&
            MissionRecord->VerifiedFacts == 0xFF &&
            MissionRecord->SimulationSnapshotVersion ==
                echoes::sim::kSnapshotVersion &&
            MissionRecord->CompletionTick > 0 &&
            MissionRecord->FinalStateChecksum != 0);
    FEchoesCampaignProgress Reloaded;
    TestTrue(
        TEXT("The eleven-record campaign reloads transactionally"),
        FEchoesCampaignProgressStore::LoadWithBackup(
            CampaignPath, Reloaded, Feedback) &&
            Reloaded.Decisions.Num() == 11);
    if (MissionRecord != nullptr)
    {
        FEchoesCampaignDecisionRecord ReplayConflict = *MissionRecord;
        ReplayConflict.WellChoice = FutureWellChoice::Harvest;
        ReplayConflict.AvailableWellChoices =
            NoNeutralChoiceMask(FutureWellChoice::Harvest);
        ReplayConflict.CompletionTick += 1;
        ReplayConflict.FinalStateChecksum += 1;
        TestTrue(
            TEXT("A different replay protocol is retained only as a replay conflict"),
            Reloaded.AppendDecision(ReplayConflict, Feedback) ==
                EEchoesCampaignCommitStatus::ReplayConflict);
        TestTrue(
            TEXT("The replay conflict preserves the original Preserve record"),
            Reloaded.FindDecision(
                EEchoesCampaignMissionId::NoNeutralLedger) != nullptr &&
            Reloaded.FindDecision(
                EEchoesCampaignMissionId::NoNeutralLedger)->WellChoice ==
                FutureWellChoice::Preserve);
    }

    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors() && !WorldWrapper.HasFailed();
}

#endif
