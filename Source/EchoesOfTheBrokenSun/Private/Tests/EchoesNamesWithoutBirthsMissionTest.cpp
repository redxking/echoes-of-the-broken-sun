#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesCampaignProgress.h"
#include "EchoesNamesWithoutBirthsMissionModel.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Tests/AutomationCommon.h"

namespace
{
struct FPreservedNamesFile final
{
    explicit FPreservedNamesFile(FString InPath) : Path(MoveTemp(InPath))
    {
        bExisted = IFileManager::Get().FileExists(*Path);
        if (bExisted)
        {
            FFileHelper::LoadFileToArray(Contents, *Path);
        }
    }

    ~FPreservedNamesFile()
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

uint8 NamesChoiceMask(echoes::sim::FutureWellChoice Choice)
{
    switch (Choice)
    {
        case echoes::sim::FutureWellChoice::Harvest: return 1 << 0;
        case echoes::sim::FutureWellChoice::Preserve: return 1 << 1;
        case echoes::sim::FutureWellChoice::Reshape: return 1 << 2;
        default: return 0;
    }
}

FEchoesCampaignDecisionRecord MakeNamesPrerequisite(
    EEchoesCampaignMissionId Mission,
    echoes::sim::FutureWellChoice Choice)
{
    FEchoesCampaignDecisionRecord Record;
    Record.Mission = Mission;
    Record.WellChoice = Choice;
    Record.AvailableWellChoices =
        Mission == EEchoesCampaignMissionId::WhatTheLedgerKeeps
            ? 0x07
            : NamesChoiceMask(Choice);
    Record.SimulationSnapshotVersion = echoes::sim::kSnapshotVersion;
    Record.CompletionTick = 100 + static_cast<uint8>(Mission) * 300;
    Record.FinalStateChecksum = 0x7A11A1ULL + static_cast<uint8>(Mission);
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
        default:
            break;
    }
    return Record;
}

FString NamesQuickSavePath(const FEchoesCampaignProgress& Progress)
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
            TEXT("EchoesQuickSaveNamesWithoutBirths-%08X.bin"),
            Fingerprint));
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesNamesWithoutBirthsMissionTest,
    "Echoes.Runtime.Campaign.NamesWithoutBirths",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesNamesWithoutBirthsMissionTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    const FEchoesNamesWithoutBirthsPlan HarvestPlan =
        FEchoesNamesWithoutBirthsMissionModel::PlanForChoice(
            echoes::sim::FutureWellChoice::Harvest);
    const FEchoesNamesWithoutBirthsPlan PreservePlan =
        FEchoesNamesWithoutBirthsMissionModel::PlanForChoice(
            echoes::sim::FutureWellChoice::Preserve);
    const FEchoesNamesWithoutBirthsPlan ReshapePlan =
        FEchoesNamesWithoutBirthsMissionModel::PlanForChoice(
            echoes::sim::FutureWellChoice::Reshape);
    TestTrue(TEXT("Each inherited choice produces distinct census geometry"),
             HarvestPlan.CensusSite == echoes::sim::Vec2::FromTiles(16, 22) &&
                 PreservePlan.CensusSite == echoes::sim::Vec2::FromTiles(32, 22) &&
                 ReshapePlan.CensusSite == echoes::sim::Vec2::FromTiles(48, 22) &&
                 HarvestPlan.PowerLinkSite != PreservePlan.PowerLinkSite &&
                 PreservePlan.PowerLinkSite != ReshapePlan.PowerLinkSite);

    FEchoesNamesWithoutBirthsMissionFacts Facts;
    TestTrue(TEXT("Inactive facts stay outside mission six"),
             FEchoesNamesWithoutBirthsMissionModel::DeterminePhase(Facts) ==
                 EEchoesNamesWithoutBirthsPhase::Inactive);
    Facts.bOperationActive = true;
    Facts.bLocalCoreIntact = true;
    Facts.bTalarIntact = true;
    Facts.bArchiveIntact = true;
    Facts.bFirstCivilianIntact = true;
    Facts.bSecondCivilianIntact = true;
    Facts.bSkirmishStillOngoing = true;
    TestTrue(TEXT("The operation begins by locating the census"),
             FEchoesNamesWithoutBirthsMissionModel::DeterminePhase(Facts) ==
                 EEchoesNamesWithoutBirthsPhase::LocateCensus);
    Facts.bCensusEvidenceLocated = true;
    TestTrue(TEXT("Located evidence opens archive stabilization"),
             FEchoesNamesWithoutBirthsMissionModel::DeterminePhase(Facts) ==
                 EEchoesNamesWithoutBirthsPhase::StabilizeArchive);
    Facts.bArchivePowered = true;
    TestTrue(TEXT("A powered archive opens civilian shelter"),
             FEchoesNamesWithoutBirthsMissionModel::DeterminePhase(Facts) ==
                 EEchoesNamesWithoutBirthsPhase::ShelterCivilians);
    Facts.bFirstCivilianSheltered = true;
    Facts.bSecondCivilianSheltered = true;
    TestTrue(TEXT("Both sheltered civilians open evidence extraction"),
             FEchoesNamesWithoutBirthsMissionModel::DeterminePhase(Facts) ==
                 EEchoesNamesWithoutBirthsPhase::ExtractEvidence);
    Facts.bTalarAtEvidenceExtraction = true;
    TestTrue(TEXT("Talar completes the full ordered operation"),
             FEchoesNamesWithoutBirthsMissionModel::DeterminePhase(Facts) ==
                 EEchoesNamesWithoutBirthsPhase::Complete);
    Facts.bFirstCivilianIntact = false;
    TestTrue(TEXT("Any protected loss fails the operation"),
             FEchoesNamesWithoutBirthsMissionModel::DeterminePhase(Facts) ==
                 EEchoesNamesWithoutBirthsPhase::Failed);
    Facts.bFirstCivilianIntact = true;
    Facts.bSkirmishStillOngoing = false;
    TestTrue(TEXT("Either terminal Core outcome fails instead of substituting for extraction"),
             FEchoesNamesWithoutBirthsMissionModel::DeterminePhase(Facts) ==
                 EEchoesNamesWithoutBirthsPhase::Failed);

    const FString CampaignPath =
        FEchoesCampaignProgressStore::GetDefaultPath();
    FPreservedNamesFile PreservedPrimary(CampaignPath);
    FPreservedNamesFile PreservedBackup(CampaignPath + TEXT(".bak"));
    FPreservedNamesFile PreservedTemporary(CampaignPath + TEXT(".tmp"));
    IFileManager::Get().Delete(*CampaignPath, false, true, true);
    IFileManager::Get().Delete(*(CampaignPath + TEXT(".bak")), false, true, true);
    IFileManager::Get().Delete(*(CampaignPath + TEXT(".tmp")), false, true, true);

    FString Feedback;
    FEchoesCampaignProgress FourRecords;
    for (const EEchoesCampaignMissionId Mission : {
             EEchoesCampaignMissionId::WhatTheLedgerKeeps,
             EEchoesCampaignMissionId::SevenAccountsOfRain,
             EEchoesCampaignMissionId::ACityOnReserve,
             EEchoesCampaignMissionId::TheUnburiedRoad})
    {
        TestTrue(TEXT("The fixture accepts a consistent prior record"),
                 FourRecords.AppendDecision(
                     MakeNamesPrerequisite(
                         Mission, echoes::sim::FutureWellChoice::Preserve),
                     Feedback) == EEchoesCampaignCommitStatus::Added);
    }
    TestTrue(TEXT("A four-record ledger is stored for the lock check"),
             FEchoesCampaignProgressStore::SaveAtomic(
                 CampaignPath, FourRecords, Feedback));
    {
        FTestWorldWrapper LockedWorld;
        if (!LockedWorld.CreateTestWorld(EWorldType::Game))
        {
            LockedWorld.ForwardErrorMessages(this);
            AddError(TEXT("Could not create the locked mission-six world."));
            return false;
        }
        UEchoesSimulationSubsystem* LockedBridge =
            LockedWorld.GetTestWorld()->GetSubsystem<
                UEchoesSimulationSubsystem>();
        TestFalse(TEXT("Mission six rejects a four-record ledger"),
                  LockedBridge != nullptr &&
                      LockedBridge->SelectOperationMode(
                          EEchoesOperationMode::CampaignNamesWithoutBirths,
                          Feedback));
        TestTrue(TEXT("The locked response names Terms of Continuance"),
                 Feedback.Contains(TEXT("Terms of Continuance")));
        LockedWorld.ForwardErrorMessages(this);
    }

    FEchoesCampaignProgress FiveRecords = FourRecords;
    TestTrue(TEXT("The fifth prerequisite record is accepted"),
             FiveRecords.AppendDecision(
                 MakeNamesPrerequisite(
                     EEchoesCampaignMissionId::TermsOfContinuance,
                     echoes::sim::FutureWellChoice::Preserve),
                 Feedback) == EEchoesCampaignCommitStatus::Added);
    const FString QuickSavePath = NamesQuickSavePath(FiveRecords);
    FPreservedNamesFile PreservedQuickSave(QuickSavePath);
    FPreservedNamesFile PreservedQuickSaveBackup(QuickSavePath + TEXT(".bak"));
    FPreservedNamesFile PreservedQuickSaveTemporary(QuickSavePath + TEXT(".tmp"));
    IFileManager::Get().Delete(*QuickSavePath, false, true, true);
    IFileManager::Get().Delete(*(QuickSavePath + TEXT(".bak")), false, true, true);
    IFileManager::Get().Delete(*(QuickSavePath + TEXT(".tmp")), false, true, true);
    TestTrue(TEXT("The five-record ledger is stored"),
             FEchoesCampaignProgressStore::SaveAtomic(
                 CampaignPath, FiveRecords, Feedback));

    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create Names Without Births test world."));
        return false;
    }
    UEchoesSimulationSubsystem* Bridge =
        WorldWrapper.GetTestWorld()->GetSubsystem<UEchoesSimulationSubsystem>();
    if (!TestNotNull(TEXT("Mission world owns the simulation subsystem"), Bridge) ||
        !TestTrue(TEXT("Five consistent records unlock mission six"),
                  Bridge != nullptr && Bridge->IsNamesWithoutBirthsUnlocked()) ||
        !TestTrue(TEXT("Mission six operation can be selected"),
                  Bridge->SelectOperationMode(
                      EEchoesOperationMode::CampaignNamesWithoutBirths,
                      Feedback)) ||
        !TestTrue(TEXT("Mission six scenario starts"),
                  Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const FEchoesObjectiveSnapshot Start = Bridge->GetLocalObjectiveSnapshot();
    TestTrue(TEXT("Talar, the archive, and both civilian proxies are authoritative"),
             Start.TalarId != 0 && Start.CensusArchiveId != 0 &&
                 Start.FirstCivilianId != 0 && Start.SecondCivilianId != 0);
    TestTrue(TEXT("The census archive begins unpowered"),
             !Start.bCensusArchivePowered &&
                 Bridge->GetNamesWithoutBirthsPhase() ==
                     EEchoesNamesWithoutBirthsPhase::LocateCensus);
    TestTrue(TEXT("Mission six uses its ledger-bound quick-save slot"),
             Bridge->QuickSaveScenario(Feedback) &&
                 IFileManager::Get().FileExists(*QuickSavePath));
    TestTrue(TEXT("The initial locate phase reconstructs after quick load"),
             Bridge->QuickLoadScenario(Feedback) &&
                 Bridge->GetNamesWithoutBirthsPhase() ==
                     EEchoesNamesWithoutBirthsPhase::LocateCensus);
    TestFalse(TEXT("Archive power is rejected before Talar locates the trace"),
              Bridge->IssueBuildCommand(
                  Start.FirstCivilianId,
                  echoes::sim::EntityType::Dropoff,
                  Bridge->SimToWorld(PreservePlan.PowerLinkSite),
                  Feedback));
    TestTrue(TEXT("The rejection exposes the census prerequisite"),
             Feedback.Contains(TEXT("CENSUS_TRACE_REQUIRED")));

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
    TestTrue(TEXT("Talar accepts movement to the inherited census"),
             Bridge->IssueCommand(
                 echoes::sim::CommandType::Move,
                 Start.TalarId,
                 0,
                 Bridge->SimToWorld(PreservePlan.CensusSite),
                 echoes::sim::FutureWellChoice::Dormant,
                 Feedback));
    TestTrue(TEXT("Ordinary movement locates the branch-specific census"),
             TickUntil(
                 [Bridge]()
                 {
                     return Bridge->GetNamesWithoutBirthsPhase() ==
                         EEchoesNamesWithoutBirthsPhase::StabilizeArchive;
                 },
                 500));
    const bool bPowerLinkAccepted = Bridge->IssueBuildCommand(
        Start.FirstCivilianId,
        echoes::sim::EntityType::Dropoff,
        Bridge->SimToWorld(PreservePlan.PowerLinkSite),
        Feedback);
    if (!bPowerLinkAccepted)
    {
        AddInfo(FString::Printf(
            TEXT("Mission-six Power Link rejection: %s"), *Feedback));
    }
    TestTrue(TEXT("A civilian worker accepts the required Power Link"),
             bPowerLinkAccepted);
    TestTrue(TEXT("Ordinary construction powers the census archive"),
             TickUntil(
                 [Bridge]()
                 {
                     return Bridge->GetNamesWithoutBirthsPhase() ==
                         EEchoesNamesWithoutBirthsPhase::ShelterCivilians;
                 },
                 900));
    TestTrue(TEXT("The first civilian accepts shelter movement"),
             Bridge->IssueCommand(
                 echoes::sim::CommandType::Move,
                 Start.FirstCivilianId,
                 0,
                 Bridge->SimToWorld(PreservePlan.CivilianShelterSite),
                 echoes::sim::FutureWellChoice::Dormant,
                 Feedback));
    TestTrue(TEXT("The second civilian accepts shelter movement"),
             Bridge->IssueCommand(
                 echoes::sim::CommandType::Move,
                 Start.SecondCivilianId,
                 0,
                 Bridge->SimToWorld(PreservePlan.CivilianShelterSite),
                 echoes::sim::FutureWellChoice::Dormant,
                 Feedback));
    TestTrue(TEXT("Both civilians reaching shelter opens extraction"),
             TickUntil(
                 [Bridge]()
                 {
                     return Bridge->GetNamesWithoutBirthsPhase() ==
                         EEchoesNamesWithoutBirthsPhase::ExtractEvidence;
                 },
                 1000));
    TestTrue(TEXT("Talar accepts evidence extraction movement"),
             Bridge->IssueCommand(
                 echoes::sim::CommandType::Move,
                 Start.TalarId,
                 0,
                 Bridge->SimToWorld(PreservePlan.EvidenceExtractionSite),
                 echoes::sim::FutureWellChoice::Dormant,
                 Feedback));
    TestTrue(TEXT("The ordinary mission path commits the sixth ledger record"),
             TickUntil(
                 [Bridge]()
                 {
                     return Bridge->GetCampaignProgress().FindDecision(
                                EEchoesCampaignMissionId::NamesWithoutBirths) !=
                         nullptr;
                 },
                 1000));
    const FEchoesCampaignDecisionRecord* MissionRecord =
        Bridge->GetCampaignProgress().FindDecision(
            EEchoesCampaignMissionId::NamesWithoutBirths);
    TestTrue(TEXT("The sixth record preserves branch and all verified facts"),
             MissionRecord != nullptr &&
                 MissionRecord->WellChoice ==
                     echoes::sim::FutureWellChoice::Preserve &&
                 MissionRecord->AvailableWellChoices == (1 << 1) &&
                 MissionRecord->VerifiedFacts == 0x3F);
    FEchoesCampaignProgress Reloaded;
    TestTrue(TEXT("The six-record campaign reloads transactionally"),
             FEchoesCampaignProgressStore::LoadWithBackup(
                 CampaignPath, Reloaded, Feedback) &&
                 Reloaded.Decisions.Num() == 6);

    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return true;
}

#endif
