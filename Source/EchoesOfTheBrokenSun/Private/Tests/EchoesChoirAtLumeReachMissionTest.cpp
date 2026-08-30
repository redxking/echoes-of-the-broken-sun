#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesCampaignProgress.h"
#include "EchoesChoirAtLumeReachMissionModel.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Tests/AutomationCommon.h"

namespace
{
struct FPreservedLumeReachFile final
{
    explicit FPreservedLumeReachFile(FString InPath) : Path(MoveTemp(InPath))
    {
        bExisted = IFileManager::Get().FileExists(*Path);
        if (bExisted)
        {
            FFileHelper::LoadFileToArray(Contents, *Path);
        }
    }

    ~FPreservedLumeReachFile()
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

uint8 LumeReachChoiceMask(echoes::sim::FutureWellChoice Choice)
{
    switch (Choice)
    {
        case echoes::sim::FutureWellChoice::Harvest: return 1 << 0;
        case echoes::sim::FutureWellChoice::Preserve: return 1 << 1;
        case echoes::sim::FutureWellChoice::Reshape: return 1 << 2;
        default: return 0;
    }
}

FEchoesCampaignDecisionRecord MakeLumeReachPrerequisite(
    EEchoesCampaignMissionId Mission,
    echoes::sim::FutureWellChoice Choice)
{
    FEchoesCampaignDecisionRecord Record;
    Record.Mission = Mission;
    Record.WellChoice = Choice;
    Record.AvailableWellChoices =
        Mission == EEchoesCampaignMissionId::WhatTheLedgerKeeps
            ? 0x07
            : LumeReachChoiceMask(Choice);
    Record.SimulationSnapshotVersion = echoes::sim::kSnapshotVersion;
    Record.CompletionTick = 100 + static_cast<uint8>(Mission) * 300;
    Record.FinalStateChecksum = 0x10A0E00ULL + static_cast<uint8>(Mission);
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
            Record.VerifiedFacts = 0x7B;
            break;
        default:
            break;
    }
    return Record;
}

FString LumeReachQuickSavePath(const FEchoesCampaignProgress& Progress)
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
        FPaths::ProjectSavedDir(),
        TEXT("SaveGames"),
        FString::Printf(
            TEXT("EchoesQuickSaveTheChoirAtLumeReach-%08X.bin"),
            Fingerprint));
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesChoirAtLumeReachMissionTest,
    "Echoes.Runtime.Campaign.ChoirAtLumeReach",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesChoirAtLumeReachMissionTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    using echoes::sim::FutureWellChoice;
    using echoes::sim::Vec2;
    const FutureWellChoice PriorChoices[] = {
        FutureWellChoice::Harvest,
        FutureWellChoice::Preserve,
        FutureWellChoice::Reshape};
    const EEchoesCityDistrict DeferredDistricts[] = {
        EEchoesCityDistrict::LifeSupport,
        EEchoesCityDistrict::Transit,
        EEchoesCityDistrict::Archive};
    int32 BranchContracts = 0;
    for (const FutureWellChoice PriorChoice : PriorChoices)
    {
        for (const EEchoesCityDistrict District : DeferredDistricts)
        {
            const FEchoesChoirAtLumeReachPlan Plan =
                FEchoesChoirAtLumeReachMissionModel::PlanForChoice(
                    PriorChoice, District);
            TestTrue(
                TEXT("Every inherited branch and deferred district has authored public geometry"),
                Plan.PriorChoice == PriorChoice &&
                    Plan.DeferredDistrict == District &&
                    Plan.ContactSite != Vec2{} &&
                    Plan.LiabilitySite ==
                        FEchoesChoirAtLumeReachMissionModel::
                            LiabilitySiteForDistrict(District) &&
                    Plan.FirstAnchorSite == Vec2::FromTiles(28, 39) &&
                    Plan.SecondAnchorSite == Vec2::FromTiles(36, 39) &&
                    Plan.FutureWellSite == Vec2::FromTiles(32, 43));
            for (const FutureWellChoice NewChoice : PriorChoices)
            {
                TestTrue(
                    TEXT("Every new Lume Well choice has a public resolution site"),
                    FEchoesChoirAtLumeReachMissionModel::
                        ResolutionSiteForChoice(NewChoice) != Vec2{});
                ++BranchContracts;
            }
        }
    }
    TestEqual(
        TEXT("The authored plan matrix covers 3 prior branches x 3 liabilities x 3 new choices"),
        BranchContracts,
        27);
    TestTrue(
        TEXT("The three prior doctrines establish distinct contact approaches"),
        FEchoesChoirAtLumeReachMissionModel::PlanForChoice(
            FutureWellChoice::Harvest,
            EEchoesCityDistrict::Archive).ContactSite ==
                Vec2::FromTiles(18, 20) &&
        FEchoesChoirAtLumeReachMissionModel::PlanForChoice(
            FutureWellChoice::Preserve,
            EEchoesCityDistrict::Archive).ContactSite ==
                Vec2::FromTiles(32, 20) &&
        FEchoesChoirAtLumeReachMissionModel::PlanForChoice(
            FutureWellChoice::Reshape,
            EEchoesCityDistrict::Archive).ContactSite ==
                Vec2::FromTiles(46, 20));
    TestTrue(
        TEXT("The three new choices establish distinct public resolution sites"),
        FEchoesChoirAtLumeReachMissionModel::ResolutionSiteForChoice(
            FutureWellChoice::Harvest) == Vec2::FromTiles(18, 50) &&
        FEchoesChoirAtLumeReachMissionModel::ResolutionSiteForChoice(
            FutureWellChoice::Preserve) == Vec2::FromTiles(32, 50) &&
        FEchoesChoirAtLumeReachMissionModel::ResolutionSiteForChoice(
            FutureWellChoice::Reshape) == Vec2::FromTiles(46, 50));

    FEchoesChoirAtLumeReachMissionFacts Facts;
    TestTrue(
        TEXT("Inactive facts stay outside mission ten"),
        FEchoesChoirAtLumeReachMissionModel::DeterminePhase(Facts) ==
            EEchoesChoirAtLumeReachPhase::Inactive);
    Facts.bOperationActive = true;
    Facts.bLocalCoreIntact = true;
    Facts.bOruunIntact = true;
    Facts.bWaystoneIntact = true;
    Facts.bFutureWellIntact = true;
    Facts.bSkirmishStillOngoing = true;
    TestTrue(
        TEXT("The operation begins by establishing public contact"),
        FEchoesChoirAtLumeReachMissionModel::DeterminePhase(Facts) ==
            EEchoesChoirAtLumeReachPhase::EstablishContact);
    Facts.bContactEstablished = true;
    TestTrue(
        TEXT("Contact opens the inherited district liability"),
        FEchoesChoirAtLumeReachMissionModel::DeterminePhase(Facts) ==
            EEchoesChoirAtLumeReachPhase::ResolveDeferredLiability);
    Facts.bDeferredLiabilityResolved = true;
    TestTrue(
        TEXT("The rooted liability opens the first public anchor"),
        FEchoesChoirAtLumeReachMissionModel::DeterminePhase(Facts) ==
            EEchoesChoirAtLumeReachPhase::RaiseFirstAnchor);
    Facts.bFirstAnchorRaised = true;
    TestTrue(
        TEXT("The first anchor opens the second public anchor"),
        FEchoesChoirAtLumeReachMissionModel::DeterminePhase(Facts) ==
            EEchoesChoirAtLumeReachPhase::RaiseSecondAnchor);
    Facts.bSecondAnchorRaised = true;
    TestTrue(
        TEXT("Both anchors expose the separate Lume Reach Well choice"),
        FEchoesChoirAtLumeReachMissionModel::DeterminePhase(Facts) ==
            EEchoesChoirAtLumeReachPhase::CommitFutureWell);
    Facts.bFutureWellProtocolChosen = true;
    TestTrue(
        TEXT("A committed Well opens its branch resolution"),
        FEchoesChoirAtLumeReachMissionModel::DeterminePhase(Facts) ==
            EEchoesChoirAtLumeReachPhase::ResolveFutureWell);
    Facts.bBranchResolutionCompleted = true;
    TestTrue(
        TEXT("Only the completed branch resolution succeeds"),
        FEchoesChoirAtLumeReachMissionModel::DeterminePhase(Facts) ==
            EEchoesChoirAtLumeReachPhase::Complete);
    Facts.bFirstAnchorRaised = false;
    TestTrue(
        TEXT("Losing a committed anchor fails closed"),
        FEchoesChoirAtLumeReachMissionModel::DeterminePhase(Facts) ==
            EEchoesChoirAtLumeReachPhase::Failed);
    Facts.bFirstAnchorRaised = true;
    Facts.bBranchResolutionCompleted = false;
    Facts.bReshapeWindowExpired = true;
    TestTrue(
        TEXT("An expired Reshape window fails before extraction"),
        FEchoesChoirAtLumeReachMissionModel::DeterminePhase(Facts) ==
            EEchoesChoirAtLumeReachPhase::Failed);
    Facts.bReshapeWindowExpired = false;
    Facts.bOruunIntact = false;
    TestTrue(
        TEXT("Oruun loss fails the operation"),
        FEchoesChoirAtLumeReachMissionModel::DeterminePhase(Facts) ==
            EEchoesChoirAtLumeReachPhase::Failed);
    Facts.bOruunIntact = true;
    Facts.bLocalCoreIntact = false;
    TestTrue(
        TEXT("Local Core loss fails the operation"),
        FEchoesChoirAtLumeReachMissionModel::DeterminePhase(Facts) ==
            EEchoesChoirAtLumeReachPhase::Failed);
    Facts.bLocalCoreIntact = true;
    Facts.bSkirmishStillOngoing = false;
    TestTrue(
        TEXT("Either terminal match outcome invalidates the operation"),
        FEchoesChoirAtLumeReachMissionModel::DeterminePhase(Facts) ==
            EEchoesChoirAtLumeReachPhase::Failed);

    const FString CampaignPath =
        FEchoesCampaignProgressStore::GetDefaultPath();
    FPreservedLumeReachFile PreservedPrimary(CampaignPath);
    FPreservedLumeReachFile PreservedBackup(CampaignPath + TEXT(".bak"));
    FPreservedLumeReachFile PreservedTemporary(CampaignPath + TEXT(".tmp"));
    IFileManager::Get().Delete(*CampaignPath, false, true, true);
    IFileManager::Get().Delete(
        *(CampaignPath + TEXT(".bak")), false, true, true);
    IFileManager::Get().Delete(
        *(CampaignPath + TEXT(".tmp")), false, true, true);

    FString Feedback;
    FEchoesCampaignProgress EightRecords;
    for (const EEchoesCampaignMissionId Mission : {
             EEchoesCampaignMissionId::WhatTheLedgerKeeps,
             EEchoesCampaignMissionId::SevenAccountsOfRain,
             EEchoesCampaignMissionId::ACityOnReserve,
             EEchoesCampaignMissionId::TheUnburiedRoad,
             EEchoesCampaignMissionId::TermsOfContinuance,
             EEchoesCampaignMissionId::NamesWithoutBirths,
             EEchoesCampaignMissionId::TheShapeOfSilence,
             EEchoesCampaignMissionId::TheShapeBesideUs})
    {
        TestTrue(
            TEXT("The fixture accepts a consistent prior campaign record"),
            EightRecords.AppendDecision(
                MakeLumeReachPrerequisite(
                    Mission, FutureWellChoice::Preserve),
                Feedback) == EEchoesCampaignCommitStatus::Added);
    }
    TestTrue(
        TEXT("An eight-record ledger is stored for the mission-ten lock check"),
        FEchoesCampaignProgressStore::SaveAtomic(
            CampaignPath, EightRecords, Feedback));
    {
        FTestWorldWrapper LockedWorld;
        if (!LockedWorld.CreateTestWorld(EWorldType::Game))
        {
            LockedWorld.ForwardErrorMessages(this);
            AddError(TEXT("Could not create the locked mission-ten world."));
            return false;
        }
        UEchoesSimulationSubsystem* LockedBridge =
            LockedWorld.GetTestWorld()->GetSubsystem<
                UEchoesSimulationSubsystem>();
        TestFalse(
            TEXT("Mission ten rejects an eight-record ledger"),
            LockedBridge != nullptr &&
                LockedBridge->SelectOperationMode(
                    EEchoesOperationMode::CampaignChoirAtLumeReach,
                    Feedback));
        TestTrue(
            TEXT("The locked response names Reserve Authority"),
            Feedback.Contains(TEXT("Reserve Authority")));
        LockedWorld.ForwardErrorMessages(this);
    }

    FEchoesCampaignProgress MismatchedNineRecords = EightRecords;
    TestTrue(
        TEXT("A structurally valid but inconsistent ninth record can be isolated"),
        MismatchedNineRecords.AppendDecision(
            MakeLumeReachPrerequisite(
                EEchoesCampaignMissionId::ReserveAuthority,
                FutureWellChoice::Harvest),
            Feedback) == EEchoesCampaignCommitStatus::Added);
    TestTrue(
        TEXT("The inconsistent nine-record fixture is stored"),
        FEchoesCampaignProgressStore::SaveAtomic(
            CampaignPath, MismatchedNineRecords, Feedback));
    {
        FTestWorldWrapper MismatchedWorld;
        if (!MismatchedWorld.CreateTestWorld(EWorldType::Game))
        {
            MismatchedWorld.ForwardErrorMessages(this);
            AddError(TEXT("Could not create the mismatched mission-ten world."));
            return false;
        }
        UEchoesSimulationSubsystem* MismatchedBridge =
            MismatchedWorld.GetTestWorld()->GetSubsystem<
                UEchoesSimulationSubsystem>();
        TestFalse(
            TEXT("Mission ten rejects a ninth record that rewrites the inherited branch"),
            MismatchedBridge != nullptr &&
                MismatchedBridge->SelectOperationMode(
                    EEchoesOperationMode::CampaignChoirAtLumeReach,
                    Feedback));
        MismatchedWorld.ForwardErrorMessages(this);
    }

    FEchoesCampaignProgress NineRecords = EightRecords;
    TestTrue(
        TEXT("The consistent Reserve Authority prerequisite is accepted"),
        NineRecords.AppendDecision(
            MakeLumeReachPrerequisite(
                EEchoesCampaignMissionId::ReserveAuthority,
                FutureWellChoice::Preserve),
            Feedback) == EEchoesCampaignCommitStatus::Added);
    FEchoesCampaignDecisionRecord InvalidLumeRecord;
    InvalidLumeRecord.Mission =
        EEchoesCampaignMissionId::ChoirAtLumeReach;
    InvalidLumeRecord.WellChoice = FutureWellChoice::Preserve;
    InvalidLumeRecord.AvailableWellChoices = 1 << 1;
    InvalidLumeRecord.VerifiedFacts = 0xFF;
    InvalidLumeRecord.SimulationSnapshotVersion =
        echoes::sim::kSnapshotVersion;
    InvalidLumeRecord.CompletionTick = 3200;
    InvalidLumeRecord.FinalStateChecksum = 0x10A0E0AULL;
    FEchoesCampaignProgress InvalidProgress = NineRecords;
    TestTrue(
        TEXT("A fabricated Lume record cannot hide the two unchosen protocols"),
        InvalidProgress.AppendDecision(InvalidLumeRecord, Feedback) ==
            EEchoesCampaignCommitStatus::StorageFailure);
    InvalidLumeRecord.AvailableWellChoices = 0x07;
    InvalidLumeRecord.VerifiedFacts = 0xFE;
    TestTrue(
        TEXT("A Lume record missing any required fact is rejected"),
        InvalidProgress.AppendDecision(InvalidLumeRecord, Feedback) ==
            EEchoesCampaignCommitStatus::StorageFailure);

    FEchoesCampaignProgress AlternateNineRecords;
    for (const EEchoesCampaignMissionId Mission : {
             EEchoesCampaignMissionId::WhatTheLedgerKeeps,
             EEchoesCampaignMissionId::SevenAccountsOfRain,
             EEchoesCampaignMissionId::ACityOnReserve,
             EEchoesCampaignMissionId::TheUnburiedRoad,
             EEchoesCampaignMissionId::TermsOfContinuance,
             EEchoesCampaignMissionId::NamesWithoutBirths,
             EEchoesCampaignMissionId::TheShapeOfSilence,
             EEchoesCampaignMissionId::TheShapeBesideUs,
             EEchoesCampaignMissionId::ReserveAuthority})
    {
        TestTrue(
            TEXT("The alternate branch accepts a consistent Harvest record"),
            AlternateNineRecords.AppendDecision(
                MakeLumeReachPrerequisite(
                    Mission, FutureWellChoice::Harvest),
                Feedback) == EEchoesCampaignCommitStatus::Added);
    }

    const FString QuickSavePath = LumeReachQuickSavePath(NineRecords);
    const FString AlternateQuickSavePath =
        LumeReachQuickSavePath(AlternateNineRecords);
    TestTrue(
        TEXT("The Preserve and Harvest ledgers retain distinct checkpoint namespaces"),
        !QuickSavePath.IsEmpty() &&
            !AlternateQuickSavePath.IsEmpty() &&
            QuickSavePath != AlternateQuickSavePath);
    FPreservedLumeReachFile PreservedQuickSave(QuickSavePath);
    FPreservedLumeReachFile PreservedQuickSaveBackup(
        QuickSavePath + TEXT(".bak"));
    FPreservedLumeReachFile PreservedQuickSaveTemporary(
        QuickSavePath + TEXT(".tmp"));
    FPreservedLumeReachFile PreservedAlternateQuickSave(
        AlternateQuickSavePath);
    FPreservedLumeReachFile PreservedAlternateQuickSaveBackup(
        AlternateQuickSavePath + TEXT(".bak"));
    FPreservedLumeReachFile PreservedAlternateQuickSaveTemporary(
        AlternateQuickSavePath + TEXT(".tmp"));
    IFileManager::Get().Delete(*QuickSavePath, false, true, true);
    IFileManager::Get().Delete(
        *(QuickSavePath + TEXT(".bak")), false, true, true);
    IFileManager::Get().Delete(
        *(QuickSavePath + TEXT(".tmp")), false, true, true);
    IFileManager::Get().Delete(
        *AlternateQuickSavePath, false, true, true);
    IFileManager::Get().Delete(
        *(AlternateQuickSavePath + TEXT(".bak")), false, true, true);
    IFileManager::Get().Delete(
        *(AlternateQuickSavePath + TEXT(".tmp")), false, true, true);
    TestTrue(
        TEXT("The consistent nine-record ledger is stored"),
        FEchoesCampaignProgressStore::SaveAtomic(
            CampaignPath, NineRecords, Feedback));

    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create The Choir at Lume Reach test world."));
        return false;
    }
    UEchoesSimulationSubsystem* Bridge =
        WorldWrapper.GetTestWorld()->GetSubsystem<
            UEchoesSimulationSubsystem>();
    if (!TestNotNull(TEXT("Mission world owns the simulation subsystem"), Bridge) ||
        !TestTrue(
            TEXT("Nine consistent records unlock mission ten"),
            Bridge != nullptr && Bridge->IsChoirAtLumeReachUnlocked()) ||
        !TestTrue(
            TEXT("Mission ten operation can be selected"),
            Bridge->SelectOperationMode(
                EEchoesOperationMode::CampaignChoirAtLumeReach,
                Feedback)) ||
        !TestTrue(
            TEXT("Mission ten scenario starts"),
            Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const FEchoesChoirAtLumeReachPlan PreservePlan =
        Bridge->GetChoirAtLumeReachPlan();
    const FEchoesObjectiveSnapshot Start =
        Bridge->GetLocalObjectiveSnapshot();
    TestTrue(
        TEXT("The operation deploys Kharuun authority"),
        Bridge->GetLocalFaction() ==
            echoes::sim::Faction::KharuunAssemblies);
    TestTrue(
        TEXT("Oruun, the Waystone, and the Lume Well have distinct authority IDs"),
        Start.ChoirAtLumeReachOruunId != 0 &&
            Start.ChoirAtLumeReachWaystoneId != 0 &&
            Start.ChoirAtLumeReachWellId != 0 &&
            Start.ChoirAtLumeReachOruunId !=
                Start.ChoirAtLumeReachWaystoneId &&
            Start.ChoirAtLumeReachOruunId !=
                Start.ChoirAtLumeReachWellId &&
            Start.ChoirAtLumeReachWaystoneId !=
                Start.ChoirAtLumeReachWellId);
    const echoes::sim::Entity* StartOruun =
        Bridge->FindEntity(Start.ChoirAtLumeReachOruunId);
    const echoes::sim::Entity* StartWaystone =
        Bridge->FindEntity(Start.ChoirAtLumeReachWaystoneId);
    const echoes::sim::Entity* StartWell =
        Bridge->FindEntity(Start.ChoirAtLumeReachWellId);
    TestTrue(
        TEXT("The three protected mission entities match their public authority contract"),
        StartOruun != nullptr &&
            StartOruun->owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            StartOruun->faction ==
                echoes::sim::Faction::KharuunAssemblies &&
            StartOruun->type == echoes::sim::EntityType::ScoutUnit &&
        StartWaystone != nullptr &&
            StartWaystone->owner ==
                UEchoesSimulationSubsystem::LocalPlayerId &&
            StartWaystone->faction ==
                echoes::sim::Faction::KharuunAssemblies &&
            StartWaystone->type == echoes::sim::EntityType::Dropoff &&
        StartWell != nullptr &&
            StartWell->owner == echoes::sim::kNeutralPlayer &&
            StartWell->type == echoes::sim::EntityType::FutureWell &&
            StartWell->position == PreservePlan.FutureWellSite);
    int32 BlockedTiles = 0;
    const echoes::sim::Simulation* StartSimulation =
        Bridge->GetSimulation();
    if (StartSimulation != nullptr)
    {
        for (int32 TileY = 0;
             TileY < StartSimulation->Config().mapHeightTiles;
             ++TileY)
        {
            for (int32 TileX = 0;
                 TileX < StartSimulation->Config().mapWidthTiles;
                 ++TileX)
            {
                if (StartSimulation->TerrainAt(TileX, TileY) ==
                    echoes::sim::Terrain::Blocked)
                {
                    ++BlockedTiles;
                }
            }
        }
    }
    TestEqual(
        TEXT("The dedicated Preserve Lume Reach topology is exact"),
        BlockedTiles,
        223);
    TestTrue(
        TEXT("Mission ten begins at public contact"),
        Bridge->GetChoirAtLumeReachPhase() ==
            EEchoesChoirAtLumeReachPhase::EstablishContact);
    TestTrue(
        TEXT("Mission ten uses its nine-record-fingerprinted quick-save slot"),
        Bridge->QuickSaveScenario(Feedback) &&
            IFileManager::Get().FileExists(*QuickSavePath));
    TestTrue(
        TEXT("The initial phase reconstructs after quick load"),
        Bridge->QuickLoadScenario(Feedback) &&
            Bridge->GetChoirAtLumeReachPhase() ==
                EEchoesChoirAtLumeReachPhase::EstablishContact);
    TArray<uint8> PreserveCheckpointBytes;
    TestTrue(
        TEXT("The Preserve checkpoint can seed an explicit cross-ledger probe"),
        FFileHelper::LoadFileToArray(
            PreserveCheckpointBytes, *QuickSavePath) &&
            FFileHelper::SaveArrayToFile(
                PreserveCheckpointBytes, *AlternateQuickSavePath) &&
            FFileHelper::SaveArrayToFile(
                PreserveCheckpointBytes,
                *(AlternateQuickSavePath + TEXT(".bak"))));
    TestTrue(
        TEXT("The alternate nine-record campaign is stored for the probe"),
        FEchoesCampaignProgressStore::SaveAtomic(
            CampaignPath, AlternateNineRecords, Feedback));
    {
        FTestWorldWrapper AlternateWorld;
        if (!AlternateWorld.CreateTestWorld(EWorldType::Game))
        {
            AlternateWorld.ForwardErrorMessages(this);
            AddError(TEXT("Could not create the alternate Mission 10 world."));
            return false;
        }
        UEchoesSimulationSubsystem* AlternateBridge =
            AlternateWorld.GetTestWorld()->GetSubsystem<
                UEchoesSimulationSubsystem>();
        if (!TestNotNull(
                TEXT("The alternate world owns a simulation subsystem"),
                AlternateBridge) ||
            !TestTrue(
                TEXT("The Harvest campaign can select Mission 10"),
                AlternateBridge->SelectOperationMode(
                    EEchoesOperationMode::CampaignChoirAtLumeReach,
                    Feedback)) ||
            !TestTrue(
                TEXT("The Harvest campaign can start Mission 10"),
                AlternateBridge->StartPrototypeScenario()))
        {
            AlternateWorld.ForwardErrorMessages(this);
            return false;
        }
        TestFalse(
            TEXT("A renamed Preserve checkpoint cannot load into a Harvest ledger"),
            AlternateBridge->QuickLoadScenario(Feedback));
        TestTrue(
            TEXT("Cross-ledger loading reports the exact ledger-binding rejection"),
            Feedback.Contains(TEXT("ledger binding does not match")));
        AlternateBridge->StopPrototypeScenario();
        AlternateWorld.ForwardErrorMessages(this);
    }
    TestTrue(
        TEXT("The Preserve campaign ledger is restored after the probe"),
        FEchoesCampaignProgressStore::SaveAtomic(
            CampaignPath, NineRecords, Feedback));

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
            TEXT("At least two construction workers are available"),
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
    Bridge->SetScenarioPaused(false);
    TestFalse(
        TEXT("Listening Spine construction is locked before contact and liability resolution"),
        Bridge->IssueBuildCommand(
            Workers[0],
            echoes::sim::EntityType::UtilityStructure,
            Bridge->SimToWorld(PreservePlan.FirstAnchorSite),
            Feedback));
    TestTrue(
        TEXT("The early build rejection names the ordered anchor sequence"),
        Feedback.Contains(TEXT("LUME_REACH_ANCHOR_SEQUENCE")));
    TestFalse(
        TEXT("The separate Lume Well is locked before both anchors"),
        Bridge->IssueCommand(
            echoes::sim::CommandType::FutureWell,
            Workers[0],
            Start.ChoirAtLumeReachWellId,
            Bridge->SimToWorld(PreservePlan.FutureWellSite),
            FutureWellChoice::Preserve,
            Feedback));
    TestTrue(
        TEXT("The early Well rejection names the missing anchors"),
        Feedback.Contains(TEXT("LUME_REACH_ANCHORS_REQUIRED")));
    TestTrue(
        TEXT("Oruun accepts the branch-specific public contact route"),
        Bridge->IssueCommand(
            echoes::sim::CommandType::Move,
            Start.ChoirAtLumeReachOruunId,
            0,
            Bridge->SimToWorld(PreservePlan.ContactSite),
            FutureWellChoice::Dormant,
            Feedback));
    TestTrue(
        TEXT("Oruun's arrival opens the inherited liability"),
        TickUntil(
            [Bridge]()
            {
                return Bridge->GetChoirAtLumeReachPhase() ==
                    EEchoesChoirAtLumeReachPhase::
                        ResolveDeferredLiability;
            },
            3200));

    const echoes::sim::EntityId WaystoneId =
        Start.ChoirAtLumeReachWaystoneId;
    TestTrue(
        TEXT("The rooted Waystone accepts an ordinary uproot order"),
        Bridge->IssueCommand(
            echoes::sim::CommandType::ToggleWaystoneRoot,
            WaystoneId,
            0,
            FVector::ZeroVector,
            FutureWellChoice::Dormant,
            Feedback));
    TestTrue(
        TEXT("The Waystone completes its uproot transition"),
        TickUntil(
            [Bridge, WaystoneId]()
            {
                const echoes::sim::Entity* Waystone =
                    Bridge->FindEntity(WaystoneId);
                return Waystone != nullptr &&
                    Waystone->waystoneMode ==
                        echoes::sim::WaystoneMode::Mobile;
            },
            400));
    TestTrue(
        TEXT("The mobile Waystone accepts the deferred Archive route"),
        Bridge->IssueCommand(
            echoes::sim::CommandType::Move,
            WaystoneId,
            0,
            Bridge->SimToWorld(PreservePlan.LiabilitySite),
            FutureWellChoice::Dormant,
            Feedback));
    TestTrue(
        TEXT("The Waystone reaches the public liability site"),
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
                    PreservePlan.LiabilitySite.x.Raw());
                const int32 Dy = FMath::Abs(
                    Waystone->position.y.Raw() -
                    PreservePlan.LiabilitySite.y.Raw());
                return Dx <= echoes::sim::kFixedScale / 2 &&
                       Dy <= echoes::sim::kFixedScale / 2;
            },
            5200));
    TestTrue(
        TEXT("The liability site accepts a root order"),
        Bridge->IssueCommand(
            echoes::sim::CommandType::ToggleWaystoneRoot,
            WaystoneId,
            0,
            FVector::ZeroVector,
            FutureWellChoice::Dormant,
            Feedback));
    TestTrue(
        TEXT("Rooting the Waystone opens the first Listening Spine"),
        TickUntil(
            [Bridge]()
            {
                return Bridge->GetChoirAtLumeReachPhase() ==
                    EEchoesChoirAtLumeReachPhase::RaiseFirstAnchor;
            },
            500));
    TestFalse(
        TEXT("The second anchor site cannot be raised first"),
        Bridge->IssueBuildCommand(
            Workers[0],
            echoes::sim::EntityType::UtilityStructure,
            Bridge->SimToWorld(PreservePlan.SecondAnchorSite),
            Feedback));
    TestTrue(
        TEXT("The out-of-sequence site rejection is reason-coded"),
        Feedback.Contains(TEXT("LUME_REACH_ANCHOR_SITE")));
    TestTrue(
        TEXT("An ordinary worker accepts the first Listening Spine"),
        Bridge->IssueBuildCommand(
            Workers[0],
            echoes::sim::EntityType::UtilityStructure,
            Bridge->SimToWorld(PreservePlan.FirstAnchorSite),
            Feedback));
    TestTrue(
        TEXT("Completed construction opens the second anchor"),
        TickUntil(
            [Bridge]()
            {
                return Bridge->GetChoirAtLumeReachPhase() ==
                    EEchoesChoirAtLumeReachPhase::RaiseSecondAnchor;
            },
            4200));
    TestTrue(
        TEXT("The one-anchor state quick-saves"),
        Bridge->QuickSaveScenario(Feedback));
    TestTrue(
        TEXT("Quick load reconstructs the first anchor without inventing the second"),
        Bridge->QuickLoadScenario(Feedback) &&
            Bridge->GetChoirAtLumeReachPhase() ==
                EEchoesChoirAtLumeReachPhase::RaiseSecondAnchor &&
            Bridge->GetLocalObjectiveSnapshot().bChoirFirstAnchorRaised &&
            !Bridge->GetLocalObjectiveSnapshot().bChoirSecondAnchorRaised);
    TestTrue(
        TEXT("A second worker accepts the second Listening Spine"),
        Bridge->IssueBuildCommand(
            Workers[1],
            echoes::sim::EntityType::UtilityStructure,
            Bridge->SimToWorld(PreservePlan.SecondAnchorSite),
            Feedback));
    TestTrue(
        TEXT("Both public anchors open the new Lume Well decision"),
        TickUntil(
            [Bridge]()
            {
                return Bridge->GetChoirAtLumeReachPhase() ==
                    EEchoesChoirAtLumeReachPhase::CommitFutureWell;
            },
            4200));

    TestTrue(
        TEXT("A worker accepts an ordinary approach to the visible Lume Well"),
        Bridge->IssueCommand(
            echoes::sim::CommandType::Move,
            Workers[0],
            0,
            Bridge->SimToWorld(Vec2::FromTiles(30, 41)),
            FutureWellChoice::Dormant,
            Feedback));
    TestTrue(
        TEXT("The ordinary approach reveals the Lume Well"),
        TickUntil(
            [Bridge, Start]()
            {
                const echoes::sim::Simulation* Current =
                    Bridge->GetSimulation();
                return Current != nullptr &&
                    Current->IsEntityVisibleTo(
                        UEchoesSimulationSubsystem::LocalPlayerId,
                        Start.ChoirAtLumeReachWellId);
            },
            1800));
    TestTrue(
        TEXT("The worker accepts a genuinely new Preserve decision"),
        Bridge->IssueCommand(
            echoes::sim::CommandType::FutureWell,
            Workers[0],
            Start.ChoirAtLumeReachWellId,
            Bridge->SimToWorld(PreservePlan.FutureWellSite),
            FutureWellChoice::Preserve,
            Feedback));
    TestTrue(
        TEXT("The new Well decision opens its public resolution route"),
        TickUntil(
            [Bridge]()
            {
                return Bridge->GetChoirAtLumeReachPhase() ==
                    EEchoesChoirAtLumeReachPhase::ResolveFutureWell;
            },
            2200));
    TestTrue(
        TEXT("The committed Well state quick-saves"),
        Bridge->QuickSaveScenario(Feedback));
    TestTrue(
        TEXT("Quick load reconstructs the committed branch-resolution phase"),
        Bridge->QuickLoadScenario(Feedback) &&
            Bridge->GetChoirAtLumeReachPhase() ==
                EEchoesChoirAtLumeReachPhase::ResolveFutureWell &&
            Bridge->GetLocalObjectiveSnapshot().
                ChoirAtLumeReachWellChoice ==
                    FutureWellChoice::Preserve);
    TestTrue(
        TEXT("Oruun accepts the Preserve resolution route"),
        Bridge->IssueCommand(
            echoes::sim::CommandType::Move,
            Start.ChoirAtLumeReachOruunId,
            0,
            Bridge->SimToWorld(
                FEchoesChoirAtLumeReachMissionModel::
                    ResolutionSiteForChoice(FutureWellChoice::Preserve)),
            FutureWellChoice::Dormant,
            Feedback));
    TestTrue(
        TEXT("The ordinary route commits the tenth campaign record"),
        TickUntil(
            [Bridge]()
            {
                return Bridge->GetCampaignProgress().FindDecision(
                           EEchoesCampaignMissionId::ChoirAtLumeReach) !=
                    nullptr;
            },
            5400));

    const FEchoesCampaignDecisionRecord* MissionRecord =
        Bridge->GetCampaignProgress().FindDecision(
            EEchoesCampaignMissionId::ChoirAtLumeReach);
    TestTrue(
        TEXT("The tenth record preserves all offered choices and all eight facts"),
        MissionRecord != nullptr &&
            MissionRecord->WellChoice == FutureWellChoice::Preserve &&
            MissionRecord->AvailableWellChoices == 0x07 &&
            MissionRecord->VerifiedFacts == 0xFF);
    FEchoesCampaignProgress Reloaded;
    TestTrue(
        TEXT("The ten-record campaign reloads transactionally"),
        FEchoesCampaignProgressStore::LoadWithBackup(
            CampaignPath, Reloaded, Feedback) &&
            Reloaded.Decisions.Num() == 10);
    if (MissionRecord != nullptr)
    {
        FEchoesCampaignDecisionRecord AlternateDecision = *MissionRecord;
        AlternateDecision.WellChoice = FutureWellChoice::Harvest;
        AlternateDecision.CompletionTick += 1;
        AlternateDecision.FinalStateChecksum += 1;
        TestTrue(
            TEXT("An alternate Lume Well decision is a replay conflict"),
            Reloaded.AppendDecision(AlternateDecision, Feedback) ==
                EEchoesCampaignCommitStatus::ReplayConflict);
        TestTrue(
            TEXT("The replay conflict preserves the original Lume Well decision"),
            Reloaded.FindDecision(
                EEchoesCampaignMissionId::ChoirAtLumeReach) != nullptr &&
            Reloaded.FindDecision(
                EEchoesCampaignMissionId::ChoirAtLumeReach)->WellChoice ==
                FutureWellChoice::Preserve);
    }

    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors() && !WorldWrapper.HasFailed();
}

#endif
