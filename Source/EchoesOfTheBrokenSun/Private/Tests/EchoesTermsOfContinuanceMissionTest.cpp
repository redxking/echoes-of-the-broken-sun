#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "EchoesCampaignProgress.h"
#include "EchoesCampaignMapCheckpoint.h"
#include "EchoesPlayerController.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesTermsOfContinuanceMissionModel.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/Crc.h"
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

FString ContinuanceQuickSavePath(
    const FEchoesCampaignProgress& Progress)
{
    TArray<uint8> LedgerBytes;
    FString Error;
    if (!FEchoesCampaignProgressStore::Encode(
            Progress,
            LedgerBytes,
            Error) ||
        LedgerBytes.IsEmpty())
    {
        return {};
    }
    const uint32 LedgerFingerprint =
        FCrc::MemCrc32(
            LedgerBytes.GetData(),
            LedgerBytes.Num() - static_cast<int32>(sizeof(uint32)));
    return FPaths::Combine(
        FEchoesCampaignProgressStore::GetSaveGameDirectory(),
        FString::Printf(
            TEXT("EchoesQuickSaveTermsOfContinuance-%08X.bin"),
            LedgerFingerprint));
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

    FEchoesScopedTestSaveEnvironment TestSaveEnvironment(*this);
    if (!TestSaveEnvironment.IsReady())
    {
        return false;
    }

    const FEchoesTermsOfContinuancePlan HarvestPlan =
        FEchoesTermsOfContinuanceMissionModel::PlanForChoice(
            echoes::sim::FutureWellChoice::Harvest);
    const FEchoesTermsOfContinuancePlan PreservePlan =
        FEchoesTermsOfContinuanceMissionModel::PlanForChoice(
            echoes::sim::FutureWellChoice::Preserve);
    const FEchoesTermsOfContinuancePlan ReshapePlan =
        FEchoesTermsOfContinuanceMissionModel::PlanForChoice(
            echoes::sim::FutureWellChoice::Reshape);
    TestTrue(TEXT("Preserve selects the central witness clause"),
             PreservePlan.MeridianRelaySite ==
                     echoes::sim::Vec2::FromTiles(32, 27) &&
                 PreservePlan.KharuunSpineSite ==
                     echoes::sim::Vec2::FromTiles(32, 39) &&
                 PreservePlan.WitnessExtractionSite ==
                     echoes::sim::Vec2::FromTiles(32, 47));
    TestTrue(TEXT("Harvest selects the western iron clause"),
             HarvestPlan.MeridianRelaySite ==
                     echoes::sim::Vec2::FromTiles(14, 27) &&
                 HarvestPlan.KharuunSpineSite ==
                     echoes::sim::Vec2::FromTiles(14, 39) &&
                 HarvestPlan.WitnessExtractionSite ==
                     echoes::sim::Vec2::FromTiles(20, 47));
    TestTrue(TEXT("Reshape selects the eastern folded clause"),
             ReshapePlan.MeridianRelaySite ==
                     echoes::sim::Vec2::FromTiles(50, 27) &&
                 ReshapePlan.KharuunSpineSite ==
                     echoes::sim::Vec2::FromTiles(44, 38) &&
                 ReshapePlan.WitnessExtractionSite ==
                     echoes::sim::Vec2::FromTiles(44, 47));
    TestTrue(
        TEXT("Harvest retains its three player links and five authored seeds"),
        HarvestPlan.PlayerPowerLinkSites ==
                TArray<echoes::sim::Vec2>{
                    echoes::sim::Vec2::FromTiles(19, 21),
                    echoes::sim::Vec2::FromTiles(17, 28),
                    echoes::sim::Vec2::FromTiles(15, 34)} &&
            HarvestPlan.SeedPowerLinkSites ==
                TArray<echoes::sim::Vec2>{
                    echoes::sim::Vec2::FromTiles(18, 10),
                    echoes::sim::Vec2::FromTiles(24, 15),
                    echoes::sim::Vec2::FromTiles(29, 20),
                    echoes::sim::Vec2::FromTiles(29, 36),
                    echoes::sim::Vec2::FromTiles(29, 40)});
    TestTrue(
        TEXT("Preserve retains its central player link and five authored seeds"),
        PreservePlan.PlayerPowerLinkSites ==
                TArray<echoes::sim::Vec2>{
                    echoes::sim::Vec2::FromTiles(29, 28)} &&
            PreservePlan.SeedPowerLinkSites ==
                TArray<echoes::sim::Vec2>{
                    echoes::sim::Vec2::FromTiles(18, 10),
                    echoes::sim::Vec2::FromTiles(24, 15),
                    echoes::sim::Vec2::FromTiles(29, 20),
                    echoes::sim::Vec2::FromTiles(29, 36),
                    echoes::sim::Vec2::FromTiles(29, 40)});
    TestTrue(
        TEXT("Reshape owns one near-base player link and the eastern seed chain"),
        ReshapePlan.PlayerPowerLinkSites ==
                TArray<echoes::sim::Vec2>{
                    echoes::sim::Vec2::FromTiles(18, 10)} &&
            ReshapePlan.SeedPowerLinkSites ==
                TArray<echoes::sim::Vec2>{
                    echoes::sim::Vec2::FromTiles(24, 15),
                    echoes::sim::Vec2::FromTiles(30, 20),
                    echoes::sim::Vec2::FromTiles(37, 23),
                    echoes::sim::Vec2::FromTiles(44, 26),
                    echoes::sim::Vec2::FromTiles(49, 32)});
    bool bReshapeSitesAreUniqueAndDisjoint =
        ReshapePlan.PlayerPowerLinkSites.Num() == 1 &&
        !ReshapePlan.SeedPowerLinkSites.Contains(
            ReshapePlan.PlayerPowerLinkSites[0]);
    for (int32 SeedIndex = 0;
         SeedIndex < ReshapePlan.SeedPowerLinkSites.Num();
         ++SeedIndex)
    {
        for (int32 OtherIndex = SeedIndex + 1;
             OtherIndex < ReshapePlan.SeedPowerLinkSites.Num();
             ++OtherIndex)
        {
            bReshapeSitesAreUniqueAndDisjoint &=
                ReshapePlan.SeedPowerLinkSites[SeedIndex] !=
                ReshapePlan.SeedPowerLinkSites[OtherIndex];
        }
    }
    TestTrue(
        TEXT("Reshape's player link and seed footprints have distinct authored centers"),
        bReshapeSitesAreUniqueAndDisjoint);
    const TArray<echoes::sim::Vec2> HarvestRouteGraphSeedSites{
        echoes::sim::Vec2::FromTiles(18, 10),
        echoes::sim::Vec2::FromTiles(24, 15),
        echoes::sim::Vec2::FromTiles(29, 20)};
    const TArray<echoes::sim::Vec2> PreserveRouteGraphSeedSites{
        echoes::sim::Vec2::FromTiles(18, 10),
        echoes::sim::Vec2::FromTiles(24, 15),
        echoes::sim::Vec2::FromTiles(29, 20),
        echoes::sim::Vec2::FromTiles(29, 36),
        echoes::sim::Vec2::FromTiles(29, 40)};
    const TArray<echoes::sim::Vec2> ReshapeRouteGraphSeedSites{
        echoes::sim::Vec2::FromTiles(24, 15),
        echoes::sim::Vec2::FromTiles(30, 20),
        echoes::sim::Vec2::FromTiles(37, 23),
        echoes::sim::Vec2::FromTiles(44, 26),
        echoes::sim::Vec2::FromTiles(49, 32)};
    const auto IsWithinRouteHop = [](
        const echoes::sim::Vec2& First,
        const echoes::sim::Vec2& Second)
    {
        const int64 DeltaX =
            First.x.FloorToInt() - Second.x.FloorToInt();
        const int64 DeltaY =
            First.y.FloorToInt() - Second.y.FloorToInt();
        return DeltaX * DeltaX + DeltaY * DeltaY <= 64;
    };
    TArray<echoes::sim::Vec2> ReshapeRouteChain{
        echoes::sim::Vec2::FromTiles(10, 10)};
    ReshapeRouteChain.Append(ReshapePlan.PlayerPowerLinkSites);
    ReshapeRouteChain.Append(ReshapePlan.SeedPowerLinkSites);
    bool bReshapeChainIsPhysical = ReshapeRouteChain.Num() == 7;
    for (int32 Index = 1;
         Index < ReshapeRouteChain.Num();
         ++Index)
    {
        bReshapeChainIsPhysical &= IsWithinRouteHop(
            ReshapeRouteChain[Index - 1],
            ReshapeRouteChain[Index]);
    }
    bReshapeChainIsPhysical &=
        !ReshapePlan.SeedPowerLinkSites.IsEmpty() &&
        IsWithinRouteHop(
            ReshapePlan.SeedPowerLinkSites.Last(),
            ReshapePlan.MeridianRelaySite) &&
        IsWithinRouteHop(
            ReshapePlan.SeedPowerLinkSites.Last(),
            ReshapePlan.KharuunSpineSite);
    TestTrue(
        TEXT("Reshape's authored chain reaches both eastern interfaces in eight-tile hops"),
        bReshapeChainIsPhysical);
    const auto PlanConnectsBothInterfaces = [IsWithinRouteHop](
        const FEchoesTermsOfContinuancePlan& Plan,
        const TArray<echoes::sim::Vec2>& RouteGraphSeedSites)
    {
        TArray<echoes::sim::Vec2> NetworkSites{
            echoes::sim::Vec2::FromTiles(10, 10)};
        NetworkSites.Append(RouteGraphSeedSites);
        NetworkSites.Append(Plan.PlayerPowerLinkSites);
        TArray<uint8> Reachable;
        Reachable.Init(0, NetworkSites.Num());
        Reachable[0] = 1;
        bool bChanged = true;
        while (bChanged)
        {
            bChanged = false;
            for (int32 CandidateIndex = 1;
                 CandidateIndex < NetworkSites.Num();
                 ++CandidateIndex)
            {
                if (Reachable[CandidateIndex] != 0)
                {
                    continue;
                }
                for (int32 ReachableIndex = 0;
                     ReachableIndex < NetworkSites.Num();
                     ++ReachableIndex)
                {
                    if (Reachable[ReachableIndex] != 0 &&
                        IsWithinRouteHop(
                            NetworkSites[ReachableIndex],
                            NetworkSites[CandidateIndex]))
                    {
                        Reachable[CandidateIndex] = 1;
                        bChanged = true;
                        break;
                    }
                }
            }
        }
        bool bRelayConnected = false;
        bool bSpineConnected = false;
        for (int32 Index = 0; Index < NetworkSites.Num(); ++Index)
        {
            if (Reachable[Index] == 0)
            {
                continue;
            }
            bRelayConnected |= IsWithinRouteHop(
                NetworkSites[Index], Plan.MeridianRelaySite);
            bSpineConnected |= IsWithinRouteHop(
                NetworkSites[Index], Plan.KharuunSpineSite);
        }
        bool bAllPlayerSitesReachable = true;
        const int32 PlayerSiteOffset =
            1 + RouteGraphSeedSites.Num();
        for (int32 PlayerIndex = 0;
             PlayerIndex < Plan.PlayerPowerLinkSites.Num();
             ++PlayerIndex)
        {
            bAllPlayerSitesReachable &=
                Reachable[PlayerSiteOffset + PlayerIndex] != 0;
        }
        return bAllPlayerSitesReachable &&
            bRelayConnected && bSpineConnected;
    };
    TestTrue(
        TEXT("Every player-built route site and both interfaces are reachable without requiring off-route seeds"),
        PlanConnectsBothInterfaces(
            HarvestPlan,
            HarvestRouteGraphSeedSites) &&
            PlanConnectsBothInterfaces(
                PreservePlan,
                PreserveRouteGraphSeedSites) &&
            PlanConnectsBothInterfaces(
                ReshapePlan,
                ReshapeRouteGraphSeedSites));

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
    Facts.bSkirmishStillOngoing = true;
    Facts.bContinuanceWindowCompromised = true;
    TestTrue(TEXT("A broken synchronized window fails the accord"),
             FEchoesTermsOfContinuanceMissionModel::DeterminePhase(Facts) ==
                 EEchoesTermsOfContinuancePhase::Failed);
    Facts.bContinuanceWindowCompromised = false;
    Facts.bWitnessExtractionStartedEarly = true;
    TestTrue(TEXT("Starting extraction before the window closes fails the accord"),
             FEchoesTermsOfContinuanceMissionModel::DeterminePhase(Facts) ==
                 EEchoesTermsOfContinuancePhase::Failed);

    const FString CampaignPath =
        FEchoesCampaignProgressStore::GetDefaultPath();
    FPreservedContinuanceFile PreservedPrimary(CampaignPath);
    FPreservedContinuanceFile PreservedBackup(CampaignPath + TEXT(".bak"));
    FPreservedContinuanceFile PreservedTemporary(CampaignPath + TEXT(".tmp"));
    IFileManager::Get().Delete(*CampaignPath, false, true, true);
    IFileManager::Get().Delete(*(CampaignPath + TEXT(".bak")), false, true, true);
    IFileManager::Get().Delete(*(CampaignPath + TEXT(".tmp")), false, true, true);

    FString Feedback;
    const auto BuildMissionFivePrerequisiteLedger = [&Feedback](
        echoes::sim::FutureWellChoice Choice,
        FEchoesCampaignProgress& OutProgress)
    {
        for (const EEchoesCampaignMissionId Mission : {
                 EEchoesCampaignMissionId::WhatTheLedgerKeeps,
                 EEchoesCampaignMissionId::SevenAccountsOfRain,
                 EEchoesCampaignMissionId::ACityOnReserve,
                 EEchoesCampaignMissionId::TheUnburiedRoad})
        {
            if (OutProgress.AppendDecision(
                    MakeContinuanceRecord(Mission, Choice),
                    Feedback) != EEchoesCampaignCommitStatus::Added)
            {
                return false;
            }
        }
        return true;
    };
    const auto RunLiveBranchCoverage = [
        this,
        &CampaignPath,
        &Feedback,
        &BuildMissionFivePrerequisiteLedger](
            echoes::sim::FutureWellChoice Choice,
            const FEchoesTermsOfContinuancePlan& ExpectedPlan,
            const TCHAR* BranchLabel)
    {
        const auto Check = [this, BranchLabel](
            const TCHAR* Detail,
            bool bCondition)
        {
            return TestTrue(
                *FString::Printf(
                    TEXT("%s live branch: %s"),
                    BranchLabel,
                    Detail),
                bCondition);
        };
        FEchoesCampaignProgress BranchProgress;
        if (!Check(
                TEXT("the four-record prerequisite ledger is valid"),
                BuildMissionFivePrerequisiteLedger(
                    Choice, BranchProgress)) ||
            !Check(
                TEXT("the prerequisite ledger is stored"),
                FEchoesCampaignProgressStore::SaveAtomic(
                    CampaignPath,
                    BranchProgress,
                    Feedback)))
        {
            return false;
        }
        const FString BranchQuickSavePath =
            ContinuanceQuickSavePath(BranchProgress);
        if (!Check(
                TEXT("the route owns a nonempty isolated checkpoint path"),
                !BranchQuickSavePath.IsEmpty()))
        {
            return false;
        }
        FPreservedContinuanceFile PreservedBranchQuickSave(
            BranchQuickSavePath);
        FPreservedContinuanceFile PreservedBranchQuickSaveBackup(
            BranchQuickSavePath + TEXT(".bak"));
        FPreservedContinuanceFile PreservedBranchQuickSaveStagedBackup(
            BranchQuickSavePath + TEXT(".bak.tmp"));
        FPreservedContinuanceFile PreservedBranchQuickSaveTemporary(
            BranchQuickSavePath + TEXT(".tmp"));
        for (const FString& Path : {
                 BranchQuickSavePath,
                 BranchQuickSavePath + TEXT(".bak"),
                 BranchQuickSavePath + TEXT(".bak.tmp"),
                 BranchQuickSavePath + TEXT(".tmp")})
        {
            IFileManager::Get().Delete(*Path, false, true, true);
        }

        FTestWorldWrapper BranchWorld;
        if (!BranchWorld.CreateTestWorld(EWorldType::Game))
        {
            BranchWorld.ForwardErrorMessages(this);
            AddError(FString::Printf(
                TEXT("Could not create the %s live branch world."),
                BranchLabel));
            return false;
        }
        UEchoesSimulationSubsystem* BranchBridge =
            BranchWorld.GetTestWorld()->GetSubsystem<
                UEchoesSimulationSubsystem>();
        if (!TestNotNull(
                *FString::Printf(
                    TEXT("%s live branch owns a simulation subsystem"),
                    BranchLabel),
                BranchBridge) ||
            !Check(
                TEXT("Mission 05 can be selected"),
                BranchBridge->SelectOperationMode(
                    EEchoesOperationMode::CampaignTermsOfContinuance,
                    Feedback)) ||
            !Check(
                TEXT("Mission 05 can start"),
                BranchBridge->StartPrototypeScenario()))
        {
            BranchWorld.ForwardErrorMessages(this);
            return false;
        }
        const FEchoesTermsOfContinuancePlan LivePlan =
            BranchBridge->GetTermsOfContinuancePlan();
        if (!Check(
                TEXT("the live route identity matches its recorded choice"),
                LivePlan.PriorChoice == Choice &&
                    LivePlan.MeridianRelaySite ==
                        ExpectedPlan.MeridianRelaySite &&
                    LivePlan.KharuunSpineSite ==
                        ExpectedPlan.KharuunSpineSite &&
                    LivePlan.WitnessExtractionSite ==
                        ExpectedPlan.WitnessExtractionSite &&
                    LivePlan.PlayerPowerLinkSites ==
                        ExpectedPlan.PlayerPowerLinkSites &&
                    LivePlan.SeedPowerLinkSites ==
                        ExpectedPlan.SeedPowerLinkSites))
        {
            BranchBridge->StopPrototypeScenario();
            BranchWorld.ForwardErrorMessages(this);
            return false;
        }
        const auto FindOwnedPowerLinkAt = [BranchBridge](
            const echoes::sim::Vec2& Site)
            -> const echoes::sim::Entity*
        {
            const echoes::sim::Simulation* Simulation =
                BranchBridge->GetSimulation();
            if (Simulation == nullptr)
            {
                return nullptr;
            }
            for (const echoes::sim::Entity& Entity :
                 Simulation->Entities())
            {
                if (Entity.owner ==
                        UEchoesSimulationSubsystem::LocalPlayerId &&
                    Entity.faction ==
                        echoes::sim::Faction::MeridianCompact &&
                    Entity.type == echoes::sim::EntityType::Dropoff &&
                    Entity.position == Site)
                {
                    return &Entity;
                }
            }
            return nullptr;
        };
        const auto HasExactLivingCompletedUnpoweredSeedTopology = [
            BranchBridge,
            &LivePlan]()
        {
            const echoes::sim::Simulation* Simulation =
                BranchBridge->GetSimulation();
            if (Simulation == nullptr ||
                LivePlan.SeedPowerLinkSites.Num() != 5)
            {
                return false;
            }
            TArray<echoes::sim::Vec2> ObservedSeedSites;
            int32 BaseDropoffCount = 0;
            for (const echoes::sim::Entity& Entity :
                 Simulation->Entities())
            {
                if (Entity.owner !=
                        UEchoesSimulationSubsystem::LocalPlayerId ||
                    Entity.faction !=
                        echoes::sim::Faction::MeridianCompact ||
                    Entity.type != echoes::sim::EntityType::Dropoff ||
                    Entity.hitPoints <= 0 || !Entity.completed)
                {
                    continue;
                }
                if (Entity.position ==
                    echoes::sim::Vec2::FromTiles(6, 17))
                {
                    ++BaseDropoffCount;
                    if (BaseDropoffCount > 1)
                    {
                        return false;
                    }
                    continue;
                }
                if (!LivePlan.SeedPowerLinkSites.Contains(
                        Entity.position) ||
                    ObservedSeedSites.Contains(Entity.position) ||
                    Entity.aegisPowered)
                {
                    return false;
                }
                ObservedSeedSites.Add(Entity.position);
            }
            return BaseDropoffCount == 1 &&
                ObservedSeedSites.Num() ==
                    LivePlan.SeedPowerLinkSites.Num();
        };
        const auto ArePlayerSitesOpenAndBuildable = [
            BranchBridge,
            &LivePlan,
            &FindOwnedPowerLinkAt]()
        {
            const echoes::sim::Simulation* Simulation =
                BranchBridge->GetSimulation();
            if (Simulation == nullptr ||
                LivePlan.PlayerPowerLinkSites.IsEmpty())
            {
                return false;
            }
            for (const echoes::sim::Vec2& PlayerSite :
                 LivePlan.PlayerPowerLinkSites)
            {
                if (FindOwnedPowerLinkAt(PlayerSite) != nullptr ||
                    Simulation->ValidatePlacement(
                        UEchoesSimulationSubsystem::LocalPlayerId,
                        echoes::sim::EntityType::Dropoff,
                        PlayerSite) !=
                        echoes::sim::PlacementResult::Valid)
                {
                    return false;
                }
            }
            return true;
        };
        const auto AreBothInterfacesInitiallyDown = [
            BranchBridge,
            &LivePlan]()
        {
            const FEchoesObjectiveSnapshot Objective =
                BranchBridge->GetLocalObjectiveSnapshot();
            const echoes::sim::Entity* Relay =
                BranchBridge->FindEntity(
                    Objective.MeridianContinuanceRelayId);
            const echoes::sim::Entity* Spine =
                BranchBridge->FindEntity(
                    Objective.KharuunContinuanceSpineId);
            return BranchBridge->GetTermsOfContinuancePhase() ==
                    EEchoesTermsOfContinuancePhase::
                        SynchronizeNetworks &&
                !Objective.bMeridianRelaySynchronized &&
                !Objective.bKharuunSpineSynchronized &&
                Relay != nullptr &&
                Relay->owner ==
                    UEchoesSimulationSubsystem::LocalPlayerId &&
                Relay->faction ==
                    echoes::sim::Faction::MeridianCompact &&
                Relay->type ==
                    echoes::sim::EntityType::UtilityStructure &&
                Relay->position == LivePlan.MeridianRelaySite &&
                Relay->hitPoints > 0 && Relay->completed &&
                !Relay->aegisPowered &&
                Spine != nullptr &&
                Spine->owner ==
                    UEchoesSimulationSubsystem::LocalPlayerId &&
                Spine->faction ==
                    echoes::sim::Faction::MeridianCompact &&
                Spine->type ==
                    echoes::sim::EntityType::UtilityStructure &&
                Spine->position == LivePlan.KharuunSpineSite &&
                Spine->hitPoints > 0 && Spine->completed &&
                !Spine->aegisPowered;
        };
        if (!Check(
                TEXT("all and only route-owned seeds are living, completed, and unpowered"),
                HasExactLivingCompletedUnpoweredSeedTopology()) ||
            !Check(
                TEXT("every authored player site is open and buildable"),
                ArePlayerSitesOpenAndBuildable()))
        {
            BranchBridge->StopPrototypeScenario();
            BranchWorld.ForwardErrorMessages(this);
            return false;
        }
        if (Choice == echoes::sim::FutureWellChoice::Reshape)
        {
            const TArray<echoes::sim::Vec2>
                DeprecatedSharedReshapeSites{
                    echoes::sim::Vec2::FromTiles(18, 10),
                    echoes::sim::Vec2::FromTiles(29, 20),
                    echoes::sim::Vec2::FromTiles(29, 36),
                    echoes::sim::Vec2::FromTiles(29, 40)};
            bool bDeprecatedSitesAbsent = true;
            for (const echoes::sim::Vec2& Site :
                 DeprecatedSharedReshapeSites)
            {
                bDeprecatedSitesAbsent &=
                    FindOwnedPowerLinkAt(Site) == nullptr;
            }
            if (!Check(
                    TEXT("the old shared Reshape links are absent"),
                    bDeprecatedSitesAbsent) ||
                !Check(
                    TEXT("both eastern interfaces begin living but unpowered"),
                    AreBothInterfacesInitiallyDown()) ||
                !Check(
                    TEXT("the pre-build Reshape checkpoint commits"),
                    BranchBridge->QuickSaveScenario(Feedback) &&
                        IFileManager::Get().FileExists(
                            *BranchQuickSavePath)) ||
                !Check(
                    TEXT("the pre-build Reshape checkpoint restores its exact open topology"),
                    BranchBridge->QuickLoadScenario(Feedback) &&
                        HasExactLivingCompletedUnpoweredSeedTopology() &&
                        ArePlayerSitesOpenAndBuildable() &&
                        AreBothInterfacesInitiallyDown()))
            {
                BranchBridge->StopPrototypeScenario();
                BranchWorld.ForwardErrorMessages(this);
                return false;
            }
        }

        const auto TickUntil = [BranchBridge](
            const TFunction<bool()>& Predicate,
            int32 MaximumTicks)
        {
            for (int32 TickIndex = 0;
                 TickIndex < MaximumTicks;
                 ++TickIndex)
            {
                if (Predicate())
                {
                    return true;
                }
                if (BranchBridge->GetTermsOfContinuancePhase() ==
                    EEchoesTermsOfContinuancePhase::Failed)
                {
                    return false;
                }
                BranchBridge->Tick(0.05f);
            }
            return BranchBridge->GetTermsOfContinuancePhase() !=
                    EEchoesTermsOfContinuancePhase::Failed &&
                Predicate();
        };
        const auto AreAllPlayerLinksLivingCompletedUnpowered = [
            &LivePlan,
            &FindOwnedPowerLinkAt]()
        {
            for (const echoes::sim::Vec2& PlayerSite :
                 LivePlan.PlayerPowerLinkSites)
            {
                const echoes::sim::Entity* PlayerLink =
                    FindOwnedPowerLinkAt(PlayerSite);
                if (PlayerLink == nullptr ||
                    PlayerLink->hitPoints <= 0 ||
                    !PlayerLink->completed ||
                    PlayerLink->aegisPowered)
                {
                    return false;
                }
            }
            return true;
        };
        const auto AreAllAuthoredSeedsLivingCompletedUnpowered = [
            &LivePlan,
            &FindOwnedPowerLinkAt]()
        {
            for (const echoes::sim::Vec2& SeedSite :
                 LivePlan.SeedPowerLinkSites)
            {
                const echoes::sim::Entity* Seed =
                    FindOwnedPowerLinkAt(SeedSite);
                if (Seed == nullptr || Seed->hitPoints <= 0 ||
                    !Seed->completed ||
                    Seed->aegisPowered)
                {
                    return false;
                }
            }
            return true;
        };
        const auto AreBothInterfacesLivingAndPowered = [
            BranchBridge,
            &LivePlan]()
        {
            const FEchoesObjectiveSnapshot Objective =
                BranchBridge->GetLocalObjectiveSnapshot();
            const echoes::sim::Entity* Relay =
                BranchBridge->FindEntity(
                    Objective.MeridianContinuanceRelayId);
            const echoes::sim::Entity* Spine =
                BranchBridge->FindEntity(
                    Objective.KharuunContinuanceSpineId);
            return Objective.bMeridianRelaySynchronized &&
                Objective.bKharuunSpineSynchronized &&
                Relay != nullptr &&
                Relay->owner ==
                    UEchoesSimulationSubsystem::LocalPlayerId &&
                Relay->faction ==
                    echoes::sim::Faction::MeridianCompact &&
                Relay->type ==
                    echoes::sim::EntityType::UtilityStructure &&
                Relay->position == LivePlan.MeridianRelaySite &&
                Relay->hitPoints > 0 &&
                Relay->completed && Relay->aegisPowered &&
                Spine != nullptr &&
                Spine->owner ==
                    UEchoesSimulationSubsystem::LocalPlayerId &&
                Spine->faction ==
                    echoes::sim::Faction::MeridianCompact &&
                Spine->type ==
                    echoes::sim::EntityType::UtilityStructure &&
                Spine->position == LivePlan.KharuunSpineSite &&
                Spine->hitPoints > 0 &&
                Spine->completed && Spine->aegisPowered;
        };
        BranchBridge->SetScenarioPaused(false);
        BranchBridge->Tick(0.05f);
        TArray<echoes::sim::EntityId> SelectedWorkers;
        for (int32 SiteIndex = 0;
             SiteIndex < LivePlan.PlayerPowerLinkSites.Num();
             ++SiteIndex)
        {
            const echoes::sim::Vec2 PlayerSite =
                LivePlan.PlayerPowerLinkSites[SiteIndex];
            echoes::sim::EntityId NearestWorkerId = 0;
            int64 NearestDistanceSquared = TNumericLimits<int64>::Max();
            for (const echoes::sim::Entity& Entity :
                 BranchBridge->GetSimulation()->Entities())
            {
                if (Entity.owner !=
                        UEchoesSimulationSubsystem::LocalPlayerId ||
                    Entity.type != echoes::sim::EntityType::Worker ||
                    Entity.hitPoints <= 0 || !Entity.completed ||
                    SelectedWorkers.Contains(Entity.id))
                {
                    continue;
                }
                const int64 DeltaX =
                    static_cast<int64>(Entity.position.x.Raw()) -
                    PlayerSite.x.Raw();
                const int64 DeltaY =
                    static_cast<int64>(Entity.position.y.Raw()) -
                    PlayerSite.y.Raw();
                const int64 DistanceSquared =
                    DeltaX * DeltaX + DeltaY * DeltaY;
                if (DistanceSquared < NearestDistanceSquared ||
                    (DistanceSquared == NearestDistanceSquared &&
                     (NearestWorkerId == 0 ||
                      Entity.id < NearestWorkerId)))
                {
                    NearestWorkerId = Entity.id;
                    NearestDistanceSquared = DistanceSquared;
                }
            }
            if (!Check(
                    TEXT("each player site receives a nearest distinct worker"),
                    NearestWorkerId != 0))
            {
                BranchBridge->StopPrototypeScenario();
                BranchWorld.ForwardErrorMessages(this);
                return false;
            }
            SelectedWorkers.Add(NearestWorkerId);
            if (!Check(
                    TEXT("ordinary construction accepts the authored player site"),
                    BranchBridge->IssueBuildCommand(
                        NearestWorkerId,
                        echoes::sim::EntityType::Dropoff,
                        BranchBridge->SimToWorld(PlayerSite),
                        Feedback)))
            {
                BranchBridge->StopPrototypeScenario();
                BranchWorld.ForwardErrorMessages(this);
                return false;
            }
        }
        if (Choice == echoes::sim::FutureWellChoice::Reshape &&
            !Check(
                TEXT("the queued near-base link exists while construction is incomplete"),
                TickUntil(
                    [&FindOwnedPowerLinkAt, &LivePlan]()
                    {
                        const echoes::sim::Entity* PlayerLink =
                            FindOwnedPowerLinkAt(
                                LivePlan.PlayerPowerLinkSites[0]);
                        return PlayerLink != nullptr &&
                            PlayerLink->hitPoints > 0 &&
                            !PlayerLink->completed;
                    },
                    100)))
        {
            BranchBridge->StopPrototypeScenario();
            BranchWorld.ForwardErrorMessages(this);
            return false;
        }
        const bool bSynchronized = TickUntil(
            [BranchBridge]()
            {
                const FEchoesObjectiveSnapshot Objective =
                    BranchBridge->GetLocalObjectiveSnapshot();
                return Objective.bMeridianRelaySynchronized &&
                    Objective.bKharuunSpineSynchronized &&
                    BranchBridge->GetTermsOfContinuancePhase() ==
                        EEchoesTermsOfContinuancePhase::
                            HoldContinuanceWindow;
            },
            700);
        const bool bCompletedUnpoweredPlayerLinks =
            bSynchronized &&
            AreAllPlayerLinksLivingCompletedUnpowered();
        const bool bLivingCompletedUnpoweredSeeds =
            bSynchronized &&
            AreAllAuthoredSeedsLivingCompletedUnpowered();
        const FEchoesObjectiveSnapshot SynchronizedObjective =
            BranchBridge->GetLocalObjectiveSnapshot();
        const echoes::sim::Entity* MeridianRelay =
            BranchBridge->FindEntity(
                SynchronizedObjective.MeridianContinuanceRelayId);
        const echoes::sim::Entity* KharuunSpine =
            BranchBridge->FindEntity(
                SynchronizedObjective.KharuunContinuanceSpineId);
        if (!Check(
                TEXT("ordinary construction synchronizes before T300"),
                bSynchronized &&
                    BranchBridge->GetSimulation()->CurrentTick() <
                        LivePlan.ContinuanceWindowStartTick) ||
            !Check(
                TEXT("every player link is living, completed, and unpowered at its exact site"),
                bCompletedUnpoweredPlayerLinks) ||
            !Check(
                TEXT("every authored seed is living, completed, and unpowered at its exact site"),
                bLivingCompletedUnpoweredSeeds) ||
            !Check(
                TEXT("both treaty interfaces are living, completed, and powered"),
                MeridianRelay != nullptr &&
                    MeridianRelay->owner ==
                        UEchoesSimulationSubsystem::LocalPlayerId &&
                    MeridianRelay->faction ==
                        echoes::sim::Faction::MeridianCompact &&
                    MeridianRelay->type ==
                        echoes::sim::EntityType::UtilityStructure &&
                    MeridianRelay->position ==
                        LivePlan.MeridianRelaySite &&
                    MeridianRelay->hitPoints > 0 &&
                    MeridianRelay->completed &&
                    MeridianRelay->aegisPowered &&
                    KharuunSpine != nullptr &&
                    KharuunSpine->owner ==
                        UEchoesSimulationSubsystem::LocalPlayerId &&
                    KharuunSpine->faction ==
                        echoes::sim::Faction::MeridianCompact &&
                    KharuunSpine->type ==
                        echoes::sim::EntityType::UtilityStructure &&
                    KharuunSpine->position ==
                        LivePlan.KharuunSpineSite &&
                    KharuunSpine->hitPoints > 0 &&
                    KharuunSpine->completed &&
                    KharuunSpine->aegisPowered))
        {
            BranchBridge->StopPrototypeScenario();
            BranchWorld.ForwardErrorMessages(this);
            return false;
        }
        if (Choice == echoes::sim::FutureWellChoice::Reshape &&
            (!Check(
                 TEXT("the completed Reshape network checkpoint commits before T300"),
                 BranchBridge->QuickSaveScenario(Feedback)) ||
             !Check(
                 TEXT("the completed Reshape network checkpoint restores before T300"),
                 BranchBridge->QuickLoadScenario(Feedback) &&
                     BranchBridge->GetTermsOfContinuancePhase() ==
                         EEchoesTermsOfContinuancePhase::
                             HoldContinuanceWindow &&
                     BranchBridge->GetSimulation()->CurrentTick() <
                         LivePlan.ContinuanceWindowStartTick &&
                     AreAllPlayerLinksLivingCompletedUnpowered() &&
                     AreAllAuthoredSeedsLivingCompletedUnpowered() &&
                     AreBothInterfacesLivingAndPowered())))
        {
            BranchBridge->StopPrototypeScenario();
            BranchWorld.ForwardErrorMessages(this);
            return false;
        }
        if (!Check(
                TEXT("the synchronized network holds through T900"),
                TickUntil(
                    [BranchBridge, LivePlan]()
                    {
                        return BranchBridge->
                                   GetTermsOfContinuancePhase() ==
                                   EEchoesTermsOfContinuancePhase::
                                       ExtractWitnesses &&
                            BranchBridge->GetSimulation()->CurrentTick() >=
                                LivePlan.ContinuanceWindowEndTick;
                    },
                    1000)))
        {
            BranchBridge->StopPrototypeScenario();
            BranchWorld.ForwardErrorMessages(this);
            return false;
        }
        if (!Check(
                TEXT("every player-built link remains living, completed, and unpowered at T900"),
                AreAllPlayerLinksLivingCompletedUnpowered()) ||
            !Check(
                TEXT("every authored seed remains living, completed, and unpowered at T900"),
                AreAllAuthoredSeedsLivingCompletedUnpowered()) ||
            !Check(
                TEXT("both treaty interfaces remain living, completed, and powered at T900"),
                AreBothInterfacesLivingAndPowered()))
        {
            BranchBridge->StopPrototypeScenario();
            BranchWorld.ForwardErrorMessages(this);
            return false;
        }
        const FEchoesObjectiveSnapshot ExtractionObjective =
            BranchBridge->GetLocalObjectiveSnapshot();
        const FVector ExtractionWorld =
            BranchBridge->SimToWorld(LivePlan.WitnessExtractionSite);
        if (!Check(
                TEXT("the Meridian witness accepts extraction"),
                BranchBridge->IssueCommand(
                    echoes::sim::CommandType::Move,
                    ExtractionObjective.MeridianContinuanceWitnessId,
                    0,
                    ExtractionWorld,
                    echoes::sim::FutureWellChoice::Dormant,
                    Feedback)) ||
            !Check(
                TEXT("the Kharuun witness accepts extraction"),
                BranchBridge->IssueCommand(
                    echoes::sim::CommandType::Move,
                    ExtractionObjective.KharuunContinuanceWitnessId,
                    0,
                    ExtractionWorld,
                    echoes::sim::FutureWellChoice::Dormant,
                    Feedback)) ||
            !Check(
                TEXT("ordinary movement completes Mission 05"),
                TickUntil(
                    [BranchBridge]()
                    {
                        return BranchBridge->
                                   GetTermsOfContinuancePhase() ==
                            EEchoesTermsOfContinuancePhase::Complete;
                    },
                    1000)))
        {
            BranchBridge->StopPrototypeScenario();
            BranchWorld.ForwardErrorMessages(this);
            return false;
        }
        const FEchoesCampaignDecisionRecord* MissionRecord =
            BranchBridge->GetCampaignProgress().FindDecision(
                EEchoesCampaignMissionId::TermsOfContinuance);
        FEchoesCampaignProgress ReloadedBranchProgress;
        if (!Check(
                TEXT("completion appends the route-specific Mission 05 decision"),
                MissionRecord != nullptr &&
                    MissionRecord->WellChoice == Choice &&
                    MissionRecord->AvailableWellChoices ==
                        ContinuanceChoiceMask(Choice)) ||
            !Check(
                TEXT("the route-specific five-record ledger reloads transactionally"),
                FEchoesCampaignProgressStore::LoadWithBackup(
                    CampaignPath,
                    ReloadedBranchProgress,
                    Feedback) &&
                    ReloadedBranchProgress.Decisions.Num() == 5 &&
                    ReloadedBranchProgress.FindDecision(
                        EEchoesCampaignMissionId::
                            TermsOfContinuance) != nullptr &&
                    ReloadedBranchProgress.FindDecision(
                        EEchoesCampaignMissionId::
                            TermsOfContinuance)->WellChoice == Choice))
        {
            BranchBridge->StopPrototypeScenario();
            BranchWorld.ForwardErrorMessages(this);
            return false;
        }
        BranchBridge->StopPrototypeScenario();
        BranchWorld.ForwardErrorMessages(this);
        return true;
    };
    if (!RunLiveBranchCoverage(
            echoes::sim::FutureWellChoice::Harvest,
            HarvestPlan,
            TEXT("Harvest")) ||
        !RunLiveBranchCoverage(
            echoes::sim::FutureWellChoice::Preserve,
            PreservePlan,
            TEXT("Preserve")) ||
        !RunLiveBranchCoverage(
            echoes::sim::FutureWellChoice::Reshape,
            ReshapePlan,
            TEXT("Reshape")))
    {
        return false;
    }

    FEchoesCampaignProgress LegacyTopologyProgress;
    if (!TestTrue(
            TEXT("The legacy-topology fixture owns a Reshape prerequisite ledger"),
            BuildMissionFivePrerequisiteLedger(
                echoes::sim::FutureWellChoice::Reshape,
                LegacyTopologyProgress)))
    {
        return false;
    }
    const FString LegacyTopologyQuickSavePath =
        ContinuanceQuickSavePath(LegacyTopologyProgress);
    FPreservedContinuanceFile PreservedLegacyTopologyQuickSave(
        LegacyTopologyQuickSavePath);
    FPreservedContinuanceFile PreservedLegacyTopologyQuickSaveBackup(
        LegacyTopologyQuickSavePath + TEXT(".bak"));
    FPreservedContinuanceFile PreservedLegacyTopologyStagedBackup(
        LegacyTopologyQuickSavePath + TEXT(".bak.tmp"));
    FPreservedContinuanceFile PreservedLegacyTopologyTemporary(
        LegacyTopologyQuickSavePath + TEXT(".tmp"));
    for (const FString& Path : {
             LegacyTopologyQuickSavePath,
             LegacyTopologyQuickSavePath + TEXT(".bak"),
             LegacyTopologyQuickSavePath + TEXT(".bak.tmp"),
             LegacyTopologyQuickSavePath + TEXT(".tmp")})
    {
        IFileManager::Get().Delete(*Path, false, true, true);
    }
    TestTrue(
        TEXT("The legacy-topology fixture stores its exact Reshape ledger"),
        FEchoesCampaignProgressStore::SaveAtomic(
            CampaignPath,
            LegacyTopologyProgress,
            Feedback));
    {
        FTestWorldWrapper LegacyTopologyWorld;
        if (!LegacyTopologyWorld.CreateTestWorld(EWorldType::Game))
        {
            LegacyTopologyWorld.ForwardErrorMessages(this);
            AddError(TEXT(
                "Could not create the legacy Mission 05 topology world."));
            return false;
        }
        UEchoesSimulationSubsystem* LegacyBridge =
            LegacyTopologyWorld.GetTestWorld()->GetSubsystem<
                UEchoesSimulationSubsystem>();
        if (!TestNotNull(
                TEXT("The legacy-topology world owns a simulation subsystem"),
                LegacyBridge) ||
            !TestTrue(
                TEXT("The legacy-topology world selects Mission 05"),
                LegacyBridge->SelectOperationMode(
                    EEchoesOperationMode::CampaignTermsOfContinuance,
                    Feedback)) ||
            !TestTrue(
                TEXT("The legacy-topology world starts Mission 05"),
                LegacyBridge->StartPrototypeScenario()))
        {
            LegacyTopologyWorld.ForwardErrorMessages(this);
            return false;
        }
        const TArray<echoes::sim::Vec2> PriorRevisionOneSeedSites{
            echoes::sim::Vec2::FromTiles(24, 15),
            echoes::sim::Vec2::FromTiles(30, 20),
            echoes::sim::Vec2::FromTiles(37, 23),
            echoes::sim::Vec2::FromTiles(44, 26),
            echoes::sim::Vec2::FromTiles(50, 31)};
        echoes::sim::Simulation* LegacySimulation =
            const_cast<echoes::sim::Simulation*>(
                LegacyBridge->GetSimulation());
        bool bLegacyTopologyMaterialized =
            PriorRevisionOneSeedSites.Num() ==
                ReshapePlan.SeedPowerLinkSites.Num();
        for (int32 Index = 0;
             Index < ReshapePlan.SeedPowerLinkSites.Num();
             ++Index)
        {
            echoes::sim::Entity* MutableSeed = nullptr;
            for (const echoes::sim::Entity& Entity :
                 LegacySimulation->Entities())
            {
                if (Entity.owner ==
                        UEchoesSimulationSubsystem::LocalPlayerId &&
                    Entity.faction ==
                        echoes::sim::Faction::MeridianCompact &&
                    Entity.type == echoes::sim::EntityType::Dropoff &&
                    Entity.position ==
                        ReshapePlan.SeedPowerLinkSites[Index])
                {
                    MutableSeed = const_cast<echoes::sim::Entity*>(
                        &Entity);
                    break;
                }
            }
            if (MutableSeed == nullptr)
            {
                bLegacyTopologyMaterialized = false;
                continue;
            }
            MutableSeed->position = PriorRevisionOneSeedSites[Index];
        }
        echoes::sim::Entity* MutableSpine = nullptr;
        const FEchoesObjectiveSnapshot LegacyObjective =
            LegacyBridge->GetLocalObjectiveSnapshot();
        for (const echoes::sim::Entity& Entity :
             LegacySimulation->Entities())
        {
            if (Entity.id ==
                LegacyObjective.KharuunContinuanceSpineId)
            {
                MutableSpine = const_cast<echoes::sim::Entity*>(
                    &Entity);
                break;
            }
        }
        bLegacyTopologyMaterialized &= MutableSpine != nullptr;
        if (MutableSpine != nullptr)
        {
            MutableSpine->position =
                echoes::sim::Vec2::FromTiles(50, 39);
        }
        if (!TestTrue(
                TEXT("The regression fixture materializes the exact prior revision-1 Reshape geometry"),
                bLegacyTopologyMaterialized) ||
            !TestTrue(
                TEXT("The incompatible Reshape checkpoint can be written for load validation"),
                LegacyBridge->QuickSaveScenario(Feedback)))
        {
            LegacyBridge->StopPrototypeScenario();
            LegacyTopologyWorld.ForwardErrorMessages(this);
            return false;
        }
        TArray<uint8> LegacyContainerBytes;
        TArray<uint8> LegacyMapEnvelope;
        FEchoesCampaignMapCheckpointIdentity LegacyMapIdentity;
        EEchoesCampaignMapCheckpointFailure LegacyMapFailure{};
        bool bLegacyRevisionMaterialized =
            FFileHelper::LoadFileToArray(
                LegacyMapEnvelope,
                *LegacyTopologyQuickSavePath) &&
            FEchoesCampaignMapCheckpoint::Inspect(
                LegacyMapEnvelope, LegacyMapIdentity, LegacyContainerBytes,
                LegacyMapFailure) &&
            LegacyContainerBytes.Num() > 16;
        if (bLegacyRevisionMaterialized)
        {
            constexpr int32 TopologyRevisionOffset = 11;
            constexpr int32 ChecksumSize = 4;
            LegacyContainerBytes[TopologyRevisionOffset] = 1;
            const int32 ChecksumOffset =
                LegacyContainerBytes.Num() - ChecksumSize;
            const uint32 Checksum = FCrc::MemCrc32(
                LegacyContainerBytes.GetData(),
                ChecksumOffset);
            for (int32 ByteIndex = 0;
                 ByteIndex < ChecksumSize;
                 ++ByteIndex)
            {
                LegacyContainerBytes[ChecksumOffset + ByteIndex] =
                    static_cast<uint8>(
                        Checksum >> (ByteIndex * 8));
            }
            TArray<uint8> RewrappedLegacyEnvelope;
            bLegacyRevisionMaterialized =
                FEchoesCampaignMapCheckpoint::Wrap(
                    LegacyMapIdentity, LegacyContainerBytes,
                    RewrappedLegacyEnvelope, LegacyMapFailure) &&
                FFileHelper::SaveArrayToFile(
                    RewrappedLegacyEnvelope,
                    *LegacyTopologyQuickSavePath);
        }
        if (!TestTrue(
                TEXT("The incompatible fixture carries the prior revision-1 topology revision with a valid checksum"),
                bLegacyRevisionMaterialized))
        {
            LegacyBridge->StopPrototypeScenario();
            LegacyTopologyWorld.ForwardErrorMessages(this);
            return false;
        }
        TestFalse(
            TEXT("QuickLoad rejects the prior revision-1 Reshape geometry"),
            LegacyBridge->QuickLoadScenario(Feedback));
        TestTrue(
            TEXT("The incompatible topology rejection is explicit and stable"),
            Feedback.Contains(TEXT("LOAD_TERMS_TOPOLOGY_MISMATCH")));
        LegacyBridge->StopPrototypeScenario();
        LegacyTopologyWorld.ForwardErrorMessages(this);
    }
    for (const FString& Path : {
             LegacyTopologyQuickSavePath,
             LegacyTopologyQuickSavePath + TEXT(".bak"),
             LegacyTopologyQuickSavePath + TEXT(".bak.tmp"),
             LegacyTopologyQuickSavePath + TEXT(".tmp")})
    {
        IFileManager::Get().Delete(*Path, false, true, true);
    }
    {
        FTestWorldWrapper DegradedTopologyWorld;
        if (!DegradedTopologyWorld.CreateTestWorld(EWorldType::Game))
        {
            DegradedTopologyWorld.ForwardErrorMessages(this);
            AddError(TEXT(
                "Could not create the degraded Mission 05 topology world."));
            return false;
        }
        UEchoesSimulationSubsystem* DegradedBridge =
            DegradedTopologyWorld.GetTestWorld()->GetSubsystem<
                UEchoesSimulationSubsystem>();
        if (!TestNotNull(
                TEXT("The degraded-topology world owns a simulation subsystem"),
                DegradedBridge) ||
            !TestTrue(
                TEXT("The degraded-topology world selects Mission 05"),
                DegradedBridge->SelectOperationMode(
                    EEchoesOperationMode::CampaignTermsOfContinuance,
                    Feedback)) ||
            !TestTrue(
                TEXT("The degraded-topology world starts Mission 05"),
                DegradedBridge->StartPrototypeScenario()))
        {
            DegradedTopologyWorld.ForwardErrorMessages(this);
            return false;
        }
        echoes::sim::Entity* DamagedSeed = nullptr;
        echoes::sim::Simulation* DegradedSimulation =
            const_cast<echoes::sim::Simulation*>(
                DegradedBridge->GetSimulation());
        for (const echoes::sim::Entity& Entity :
             DegradedSimulation->Entities())
        {
            if (Entity.owner ==
                    UEchoesSimulationSubsystem::LocalPlayerId &&
                Entity.faction ==
                    echoes::sim::Faction::MeridianCompact &&
                Entity.type == echoes::sim::EntityType::Dropoff &&
                Entity.position ==
                    ReshapePlan.SeedPowerLinkSites[0])
            {
                DamagedSeed = const_cast<echoes::sim::Entity*>(
                    &Entity);
                break;
            }
        }
        if (!TestNotNull(
                TEXT("The degraded fixture locates a current Reshape seed"),
                DamagedSeed))
        {
            DegradedBridge->StopPrototypeScenario();
            DegradedTopologyWorld.ForwardErrorMessages(this);
            return false;
        }
        const echoes::sim::EntityId DamagedSeedId = DamagedSeed->id;
        const echoes::sim::Vec2 DamagedSeedSite = DamagedSeed->position;
        DamagedSeed->hitPoints = 0;
        DegradedSimulation->Step();
        const auto IsDamagedSeedAbsent = [
            DegradedBridge,
            DamagedSeedId,
            DamagedSeedSite]()
        {
            if (DegradedBridge->FindEntity(DamagedSeedId) != nullptr)
            {
                return false;
            }
            const echoes::sim::Simulation* CurrentSimulation =
                DegradedBridge->GetSimulation();
            if (CurrentSimulation == nullptr)
            {
                return false;
            }
            for (const echoes::sim::Entity& Entity :
                 CurrentSimulation->Entities())
            {
                if (Entity.type == echoes::sim::EntityType::Dropoff &&
                    Entity.position == DamagedSeedSite)
                {
                    return false;
                }
            }
            return true;
        };
        if (!TestTrue(
                TEXT("Simulation cleanup removes the damaged seed ID and authored-site Dropoff before checkpointing"),
                IsDamagedSeedAbsent()))
        {
            DegradedBridge->StopPrototypeScenario();
            DegradedTopologyWorld.ForwardErrorMessages(this);
            return false;
        }
        TArray<uint8> CurrentTopologyBytes;
        TArray<uint8> CurrentMapEnvelope;
        FEchoesCampaignMapCheckpointIdentity CurrentMapIdentity;
        EEchoesCampaignMapCheckpointFailure CurrentMapFailure{};
        const bool bCurrentDegradedCheckpointWritten =
            DegradedBridge->QuickSaveScenario(Feedback) &&
            FFileHelper::LoadFileToArray(
                CurrentMapEnvelope,
                *LegacyTopologyQuickSavePath) &&
            FEchoesCampaignMapCheckpoint::Inspect(
                CurrentMapEnvelope, CurrentMapIdentity,
                CurrentTopologyBytes, CurrentMapFailure) &&
            CurrentTopologyBytes.Num() > 16 &&
            CurrentTopologyBytes[11] == 2;
        if (!TestTrue(
                TEXT("A checkpoint without the cleaned-up seed is written with topology revision two"),
                bCurrentDegradedCheckpointWritten))
        {
            DegradedBridge->StopPrototypeScenario();
            DegradedTopologyWorld.ForwardErrorMessages(this);
            return false;
        }
        if (!TestTrue(
                TEXT("QuickLoad accepts the current-format checkpoint without the cleaned-up seed"),
                DegradedBridge->QuickLoadScenario(Feedback)))
        {
            DegradedBridge->StopPrototypeScenario();
            DegradedTopologyWorld.ForwardErrorMessages(this);
            return false;
        }
        if (!TestTrue(
                TEXT("QuickLoad preserves the cleaned-up seed absence and synchronization phase"),
                IsDamagedSeedAbsent() &&
                    DegradedBridge->GetTermsOfContinuancePhase() ==
                        EEchoesTermsOfContinuancePhase::
                            SynchronizeNetworks))
        {
            DegradedBridge->StopPrototypeScenario();
            DegradedTopologyWorld.ForwardErrorMessages(this);
            return false;
        }
        DegradedBridge->StopPrototypeScenario();
        DegradedTopologyWorld.ForwardErrorMessages(this);
    }

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

    FEchoesCampaignProgress AlternateProgress = SeedProgress;
    AlternateProgress.Decisions[0].CompletionTick += 1;
    TestTrue(
        TEXT("The exact-ledger variant retains the same choices and mission composition"),
        AlternateProgress.Decisions.Num() == SeedProgress.Decisions.Num() &&
            AlternateProgress.Decisions[0].WellChoice ==
                SeedProgress.Decisions[0].WellChoice &&
            AlternateProgress.Decisions[1].WellChoice ==
                SeedProgress.Decisions[1].WellChoice &&
            AlternateProgress.Decisions[2].WellChoice ==
                SeedProgress.Decisions[2].WellChoice &&
            AlternateProgress.Decisions[3].WellChoice ==
                SeedProgress.Decisions[3].WellChoice &&
            AlternateProgress.Decisions != SeedProgress.Decisions);
    const FString QuickSavePath =
        ContinuanceQuickSavePath(SeedProgress);
    const FString AlternateQuickSavePath =
        ContinuanceQuickSavePath(AlternateProgress);
    TestTrue(TEXT("Mission-five save namespaces bind to the exact prerequisite ledger"),
             !QuickSavePath.IsEmpty() &&
                 !AlternateQuickSavePath.IsEmpty() &&
                 QuickSavePath != AlternateQuickSavePath);
    FPreservedContinuanceFile PreservedQuickSave(QuickSavePath);
    FPreservedContinuanceFile PreservedQuickSaveBackup(
        QuickSavePath + TEXT(".bak"));
    FPreservedContinuanceFile PreservedQuickSaveStagedBackup(
        QuickSavePath + TEXT(".bak.tmp"));
    FPreservedContinuanceFile PreservedQuickSaveTemporary(
        QuickSavePath + TEXT(".tmp"));
    FPreservedContinuanceFile PreservedAlternateQuickSave(
        AlternateQuickSavePath);
    FPreservedContinuanceFile PreservedAlternateQuickSaveBackup(
        AlternateQuickSavePath + TEXT(".bak"));
    FPreservedContinuanceFile PreservedAlternateQuickSaveStagedBackup(
        AlternateQuickSavePath + TEXT(".bak.tmp"));
    FPreservedContinuanceFile PreservedAlternateQuickSaveTemporary(
        AlternateQuickSavePath + TEXT(".tmp"));
    for (const FString& Path : {
             QuickSavePath,
             QuickSavePath + TEXT(".bak"),
             QuickSavePath + TEXT(".bak.tmp"),
             QuickSavePath + TEXT(".tmp"),
             AlternateQuickSavePath,
             AlternateQuickSavePath + TEXT(".bak"),
             AlternateQuickSavePath + TEXT(".bak.tmp"),
             AlternateQuickSavePath + TEXT(".tmp")})
    {
        IFileManager::Get().Delete(*Path, false, true, true);
    }
    TestTrue(TEXT("The four-record ledger is stored"),
             FEchoesCampaignProgressStore::SaveAtomic(
                 CampaignPath,
                 SeedProgress,
                 Feedback));

    {
        FTestWorldWrapper DeadlineWorld;
        if (!DeadlineWorld.CreateTestWorld(EWorldType::Game))
        {
            DeadlineWorld.ForwardErrorMessages(this);
            AddError(TEXT("Could not create the synchronization-deadline world."));
            return false;
        }
        UEchoesSimulationSubsystem* DeadlineBridge =
            DeadlineWorld.GetTestWorld()->GetSubsystem<
                UEchoesSimulationSubsystem>();
        if (!TestNotNull(TEXT("The deadline world owns a simulation subsystem"),
                         DeadlineBridge) ||
            !TestTrue(TEXT("The deadline world selects mission five"),
                      DeadlineBridge->SelectOperationMode(
                          EEchoesOperationMode::CampaignTermsOfContinuance,
                          Feedback)) ||
            !TestTrue(TEXT("The deadline world starts mission five"),
                      DeadlineBridge->StartPrototypeScenario()))
        {
            DeadlineWorld.ForwardErrorMessages(this);
            return false;
        }
        DeadlineBridge->SetScenarioPaused(false);
        for (int32 TickIndex = 0;
             TickIndex < 360 &&
             DeadlineBridge->GetTermsOfContinuancePhase() !=
                 EEchoesTermsOfContinuancePhase::Failed;
             ++TickIndex)
        {
            DeadlineBridge->Tick(0.05f);
        }
        TestTrue(TEXT("Missing the synchronization deadline fails instead of skipping the hold"),
                 DeadlineBridge->GetSimulation()->CurrentTick() >=
                         PreservePlan.ContinuanceWindowStartTick &&
                     DeadlineBridge->GetTermsOfContinuancePhase() ==
                         EEchoesTermsOfContinuancePhase::Failed);
        DeadlineBridge->StopPrototypeScenario();
        DeadlineWorld.ForwardErrorMessages(this);
    }

    {
        FTestWorldWrapper EarlyExtractionWorld;
        if (!EarlyExtractionWorld.CreateTestWorld(EWorldType::Game))
        {
            EarlyExtractionWorld.ForwardErrorMessages(this);
            AddError(TEXT("Could not create the early-extraction world."));
            return false;
        }
        UEchoesSimulationSubsystem* EarlyBridge =
            EarlyExtractionWorld.GetTestWorld()->GetSubsystem<
                UEchoesSimulationSubsystem>();
        if (!TestNotNull(TEXT("The early-extraction world owns a simulation subsystem"),
                         EarlyBridge) ||
            !TestTrue(TEXT("The early-extraction world selects mission five"),
                      EarlyBridge->SelectOperationMode(
                          EEchoesOperationMode::CampaignTermsOfContinuance,
                          Feedback)) ||
            !TestTrue(TEXT("The early-extraction world starts mission five"),
                      EarlyBridge->StartPrototypeScenario()))
        {
            EarlyExtractionWorld.ForwardErrorMessages(this);
            return false;
        }
        const FEchoesObjectiveSnapshot EarlySnapshot =
            EarlyBridge->GetLocalObjectiveSnapshot();
        TestTrue(TEXT("An early witness accepts the attempted extraction order"),
                 EarlyBridge->IssueCommand(
                     echoes::sim::CommandType::Move,
                     EarlySnapshot.MeridianContinuanceWitnessId,
                     0,
                     EarlyBridge->SimToWorld(
                         PreservePlan.WitnessExtractionSite),
                     echoes::sim::FutureWellChoice::Dormant,
                     Feedback));
        EarlyBridge->SetScenarioPaused(false);
        for (int32 TickIndex = 0;
             TickIndex < 20 &&
             EarlyBridge->GetTermsOfContinuancePhase() !=
                 EEchoesTermsOfContinuancePhase::Failed;
             ++TickIndex)
        {
            EarlyBridge->Tick(0.05f);
        }
        // OUT-005: "Each operation names failure predicates before play...
        // Ordinary unit loss is not a hidden failure." Ordering a witness toward
        // extraction early was an UNAUTHORED seventh instant-fail, reported as
        // `generic`, and it has been removed. The operation must now SURVIVE the
        // early order; the window is still enforced, because extraction only
        // counts once the continuance window has held. This block previously
        // asserted the defect itself, so the expectation is re-derived, not
        // weakened — and it now fails if the hidden predicate ever returns.
        TestTrue(TEXT("An early extraction order does not fail the operation"),
                 EarlyBridge->GetSimulation()->CurrentTick() <
                         PreservePlan.ContinuanceWindowEndTick &&
                     EarlyBridge->GetTermsOfContinuancePhase() !=
                         EEchoesTermsOfContinuancePhase::Failed);
        EarlyBridge->StopPrototypeScenario();
        EarlyExtractionWorld.ForwardErrorMessages(this);
    }

    {
        FTestWorldWrapper InterruptedWorld;
        if (!InterruptedWorld.CreateTestWorld(EWorldType::Game))
        {
            InterruptedWorld.ForwardErrorMessages(this);
            AddError(TEXT("Could not create the interrupted-window world."));
            return false;
        }
        UEchoesSimulationSubsystem* InterruptedBridge =
            InterruptedWorld.GetTestWorld()->GetSubsystem<
                UEchoesSimulationSubsystem>();
        if (!TestNotNull(TEXT("The interrupted-window world owns a simulation subsystem"),
                         InterruptedBridge) ||
            !TestTrue(TEXT("The interrupted-window world selects mission five"),
                      InterruptedBridge->SelectOperationMode(
                          EEchoesOperationMode::CampaignTermsOfContinuance,
                          Feedback)) ||
            !TestTrue(TEXT("The interrupted-window world starts mission five"),
                      InterruptedBridge->StartPrototypeScenario()))
        {
            InterruptedWorld.ForwardErrorMessages(this);
            return false;
        }
        echoes::sim::EntityId InterruptedWorkerId = 0;
        for (const echoes::sim::Entity& Entity :
             InterruptedBridge->GetSimulation()->Entities())
        {
            if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
                Entity.type == echoes::sim::EntityType::Worker &&
                Entity.hitPoints > 0 && Entity.completed)
            {
                InterruptedWorkerId = Entity.id;
                break;
            }
        }
        InterruptedBridge->SetScenarioPaused(false);
        InterruptedBridge->Tick(0.05f);
        TestTrue(TEXT("The interrupted-window worker accepts the missing link"),
                 InterruptedWorkerId != 0 &&
                     InterruptedBridge->IssueBuildCommand(
                         InterruptedWorkerId,
                         echoes::sim::EntityType::Dropoff,
                         InterruptedBridge->SimToWorld(
                             PreservePlan.PlayerPowerLinkSites[0]),
                         Feedback));
        for (int32 TickIndex = 0;
             TickIndex < 700 &&
             InterruptedBridge->GetTermsOfContinuancePhase() !=
                 EEchoesTermsOfContinuancePhase::Failed &&
             (InterruptedBridge->GetTermsOfContinuancePhase() !=
                  EEchoesTermsOfContinuancePhase::HoldContinuanceWindow ||
              InterruptedBridge->GetSimulation()->CurrentTick() <
                  PreservePlan.ContinuanceWindowStartTick + 20);
             ++TickIndex)
        {
            InterruptedBridge->Tick(0.05f);
        }
        TestTrue(TEXT("The interrupted fixture reaches the active synchronized window"),
                 InterruptedBridge->GetTermsOfContinuancePhase() ==
                         EEchoesTermsOfContinuancePhase::HoldContinuanceWindow &&
                     InterruptedBridge->GetSimulation()->CurrentTick() >=
                         PreservePlan.ContinuanceWindowStartTick + 20);
        echoes::sim::Simulation* InterruptedSimulation =
            const_cast<echoes::sim::Simulation*>(
                InterruptedBridge->GetSimulation());
        const FEchoesObjectiveSnapshot InterruptedSnapshot =
            InterruptedBridge->GetLocalObjectiveSnapshot();
        uint64 OpponentSequence =
            InterruptedSimulation->NextCommandSequence(
                UEchoesSimulationSubsystem::OpponentPlayerId)
                .value_or(0);
        int32 QueuedAttackers = 0;
        for (int32 Index = 0; Index < 20; ++Index)
        {
            const echoes::sim::EntityId AttackerId =
                InterruptedSimulation->SpawnEntity(
                    UEchoesSimulationSubsystem::OpponentPlayerId,
                    echoes::sim::Faction::KharuunAssemblies,
                    echoes::sim::EntityType::HeavyUnit,
                    echoes::sim::Vec2::FromTiles(
                        35 + Index % 5,
                        36 + Index / 5));
            if (AttackerId == 0)
            {
                continue;
            }
            echoes::sim::Command Attack;
            Attack.executeTick = InterruptedSimulation->CurrentTick() + 1;
            Attack.player = UEchoesSimulationSubsystem::OpponentPlayerId;
            Attack.sequence = OpponentSequence++;
            Attack.type = echoes::sim::CommandType::Attack;
            Attack.actor = AttackerId;
            Attack.target = InterruptedSnapshot.KharuunContinuanceSpineId;
            if (InterruptedSimulation->QueueCommand(Attack))
            {
                ++QueuedAttackers;
            }
        }
        TestTrue(TEXT("The interrupted fixture queues bounded hostile pressure"),
                 QueuedAttackers > 0);
        for (int32 TickIndex = 0;
             TickIndex < 500 &&
             InterruptedBridge->GetTermsOfContinuancePhase() !=
                 EEchoesTermsOfContinuancePhase::Failed;
             ++TickIndex)
        {
            InterruptedSimulation->Step();
        }
        TestTrue(TEXT("Losing synchronization during the fixed window fails before extraction"),
                 InterruptedBridge->GetSimulation()->CurrentTick() <
                         PreservePlan.ContinuanceWindowEndTick &&
                     InterruptedBridge->GetTermsOfContinuancePhase() ==
                         EEchoesTermsOfContinuancePhase::Failed);
        InterruptedBridge->StopPrototypeScenario();
        InterruptedWorld.ForwardErrorMessages(this);
    }

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
    bool bAllTreatyEntitiesAreMeridianProxies = true;
    for (const echoes::sim::EntityId TreatyEntityId : {
             Snapshot.MeridianContinuanceRelayId,
             Snapshot.KharuunContinuanceSpineId,
             Snapshot.MeridianContinuanceWitnessId,
             Snapshot.KharuunContinuanceWitnessId})
    {
        const echoes::sim::Entity* TreatyEntity =
            Bridge->FindEntity(TreatyEntityId);
        bAllTreatyEntitiesAreMeridianProxies &=
            TreatyEntity != nullptr &&
            TreatyEntity->owner ==
                UEchoesSimulationSubsystem::LocalPlayerId &&
            TreatyEntity->faction ==
                echoes::sim::Faction::MeridianCompact;
    }
    TestTrue(TEXT("Treaty entities are explicit Meridian-authoritative proxies"),
             bAllTreatyEntitiesAreMeridianProxies);
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
    TestTrue(TEXT("The same-choice exact-ledger variant is stored for binding validation"),
             FEchoesCampaignProgressStore::SaveAtomic(
                 CampaignPath,
                 AlternateProgress,
                 Feedback));
    TArray<uint8> PreserveCheckpointBytes;
    TestTrue(TEXT("The original checkpoint can seed an exact-ledger mismatch probe"),
             FFileHelper::LoadFileToArray(
                 PreserveCheckpointBytes,
                 *QuickSavePath) &&
                 FFileHelper::SaveArrayToFile(
                     PreserveCheckpointBytes,
                     *AlternateQuickSavePath) &&
                 FFileHelper::SaveArrayToFile(
                     PreserveCheckpointBytes,
                     *(AlternateQuickSavePath + TEXT(".bak"))));
    {
        FTestWorldWrapper AlternateWorld;
        if (!AlternateWorld.CreateTestWorld(EWorldType::Game))
        {
            AlternateWorld.ForwardErrorMessages(this);
            AddError(TEXT("Could not create the exact-ledger variant mission-five world."));
            return false;
        }
        UEchoesSimulationSubsystem* AlternateBridge =
            AlternateWorld.GetTestWorld()->GetSubsystem<
                UEchoesSimulationSubsystem>();
        if (!TestNotNull(TEXT("The alternate world owns a simulation subsystem"),
                         AlternateBridge) ||
            !TestTrue(TEXT("The exact-ledger variant can select mission five"),
                      AlternateBridge->SelectOperationMode(
                          EEchoesOperationMode::CampaignTermsOfContinuance,
                          Feedback)) ||
            !TestTrue(TEXT("The exact-ledger variant can start mission five"),
                      AlternateBridge->StartPrototypeScenario()))
        {
            AlternateWorld.ForwardErrorMessages(this);
            return false;
        }
        TestFalse(TEXT("A checkpoint cannot load under a different exact prerequisite ledger"),
                  AlternateBridge->QuickLoadScenario(Feedback));
        TestTrue(TEXT("Exact-ledger mismatch is identified after coarse mission fields coincide"),
                 Feedback.Contains(TEXT("LOAD_LEDGER_BRANCH_MISMATCH")));
        AlternateBridge->StopPrototypeScenario();
        AlternateWorld.ForwardErrorMessages(this);
    }
    TestTrue(TEXT("The Preserve campaign ledger is restored after isolation testing"),
             FEchoesCampaignProgressStore::SaveAtomic(
                 CampaignPath,
                 SeedProgress,
                 Feedback));

    const auto FindOwnedMeridianDropoffAt = [Bridge](
        const echoes::sim::Vec2& Site)
        -> const echoes::sim::Entity*
    {
        for (const echoes::sim::Entity& Entity :
             Bridge->GetSimulation()->Entities())
        {
            if (Entity.owner ==
                    UEchoesSimulationSubsystem::LocalPlayerId &&
                Entity.faction ==
                    echoes::sim::Faction::MeridianCompact &&
                Entity.type == echoes::sim::EntityType::Dropoff &&
                Entity.position == Site)
            {
                return &Entity;
            }
        }
        return nullptr;
    };
    bool bPreserveSeedsSpawned = true;
    for (const echoes::sim::Vec2& SeedSite :
         PreservePlan.SeedPowerLinkSites)
    {
        const echoes::sim::Entity* Seed =
            FindOwnedMeridianDropoffAt(SeedSite);
        bPreserveSeedsSpawned &= Seed != nullptr &&
            Seed->hitPoints > 0 && Seed->completed &&
            !Seed->aegisPowered;
    }
    TestTrue(
        TEXT("Preserve spawns every route-owned seed as a living, completed, unpowered Meridian Dropoff"),
        bPreserveSeedsSpawned);
    TestTrue(
        TEXT("Preserve leaves its player Power Link site unbuilt"),
        PreservePlan.PlayerPowerLinkSites.Num() == 1 &&
            FindOwnedMeridianDropoffAt(
                PreservePlan.PlayerPowerLinkSites[0]) == nullptr);

    echoes::sim::EntityId WorkerId = 0;
    int64 NearestWorkerDistanceSquared = TNumericLimits<int64>::Max();
    const echoes::sim::Vec2 PreservePlayerLinkSite =
        PreservePlan.PlayerPowerLinkSites.IsEmpty()
            ? echoes::sim::Vec2{}
            : PreservePlan.PlayerPowerLinkSites[0];
    for (const echoes::sim::Entity& Entity :
         Bridge->GetSimulation()->Entities())
    {
        if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            Entity.type == echoes::sim::EntityType::Worker &&
            Entity.hitPoints > 0 && Entity.completed)
        {
            const int64 DeltaX =
                static_cast<int64>(Entity.position.x.Raw()) -
                PreservePlayerLinkSite.x.Raw();
            const int64 DeltaY =
                static_cast<int64>(Entity.position.y.Raw()) -
                PreservePlayerLinkSite.y.Raw();
            const int64 DistanceSquared =
                DeltaX * DeltaX + DeltaY * DeltaY;
            if (DistanceSquared < NearestWorkerDistanceSquared ||
                (DistanceSquared == NearestWorkerDistanceSquared &&
                 (WorkerId == 0 || Entity.id < WorkerId)))
            {
                WorkerId = Entity.id;
                NearestWorkerDistanceSquared = DistanceSquared;
            }
        }
    }
    if (!TestTrue(
            TEXT("The nearest treaty-grid construction worker is available"),
            WorkerId != 0 &&
                NearestWorkerDistanceSquared <
                    TNumericLimits<int64>::Max()))
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
            if (Bridge->GetTermsOfContinuancePhase() ==
                EEchoesTermsOfContinuancePhase::Failed)
            {
                return false;
            }
            Bridge->Tick(0.05f);
        }
        return Bridge->GetTermsOfContinuancePhase() !=
                EEchoesTermsOfContinuancePhase::Failed &&
            Predicate();
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
                     PreservePlayerLinkSite),
                 Feedback));
    const bool bSynchronizedThroughOrdinaryConstruction = TickUntil(
        [Bridge]()
        {
            return Bridge->GetTermsOfContinuancePhase() ==
                EEchoesTermsOfContinuancePhase::HoldContinuanceWindow;
        },
        700);
    TestTrue(TEXT("Ordinary construction synchronizes both interfaces"),
             bSynchronizedThroughOrdinaryConstruction);
    TestTrue(
        TEXT("The authored player Power Link completes unpowered at the plan site"),
        [&FindOwnedMeridianDropoffAt, &PreservePlayerLinkSite]()
        {
            const echoes::sim::Entity* PlayerLink =
                FindOwnedMeridianDropoffAt(PreservePlayerLinkSite);
            return PlayerLink != nullptr &&
                PlayerLink->hitPoints > 0 &&
                PlayerLink->completed &&
                !PlayerLink->aegisPowered;
        }());
    TestTrue(
        TEXT("Both interfaces synchronize before the fixed treaty deadline"),
        bSynchronizedThroughOrdinaryConstruction &&
            Bridge->GetSimulation()->CurrentTick() <
                PreservePlan.ContinuanceWindowStartTick);
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
    const FString ReplayQuickSavePath =
        ContinuanceQuickSavePath(Reloaded);
    FPreservedContinuanceFile PreservedReplayQuickSave(
        ReplayQuickSavePath);
    FPreservedContinuanceFile PreservedReplayQuickSaveBackup(
        ReplayQuickSavePath + TEXT(".bak"));
    FPreservedContinuanceFile PreservedReplayQuickSaveStagedBackup(
        ReplayQuickSavePath + TEXT(".bak.tmp"));
    FPreservedContinuanceFile PreservedReplayQuickSaveTemporary(
        ReplayQuickSavePath + TEXT(".tmp"));
    for (const FString& Path : {
             ReplayQuickSavePath,
             ReplayQuickSavePath + TEXT(".bak"),
             ReplayQuickSavePath + TEXT(".bak.tmp"),
             ReplayQuickSavePath + TEXT(".tmp")})
    {
        IFileManager::Get().Delete(*Path, false, true, true);
    }
    Bridge->StopPrototypeScenario();
    TestTrue(
        TEXT("Recorded Mission 05 remains selectable for replay"),
        !ReplayQuickSavePath.IsEmpty() &&
            Bridge->SelectOperationMode(
                EEchoesOperationMode::CampaignTermsOfContinuance,
                Feedback) &&
            Bridge->StartPrototypeScenario());
    TestTrue(
        TEXT("A recorded Mission 05 replay can write its exact-prerequisite checkpoint"),
        Bridge->QuickSaveScenario(Feedback));
    TestTrue(
        TEXT("A recorded Mission 05 replay can restore its exact-prerequisite checkpoint"),
        Bridge->QuickLoadScenario(Feedback) &&
            Bridge->GetTermsOfContinuancePhase() ==
                EEchoesTermsOfContinuancePhase::SynchronizeNetworks);

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
        TestTrue(TEXT("Mission 05 advances to Names Without Births briefing"),
                 Bridge->GetOperationMode() ==
                         EEchoesOperationMode::CampaignNamesWithoutBirths &&
                     Controller->IsMissionBriefingVisible() &&
                     Bridge->GetNamesWithoutBirthsPhase() ==
                         EEchoesNamesWithoutBirthsPhase::LocateCensus &&
                     Bridge->IsScenarioPaused());
        Controller->Destroy();
    }

    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return true;
}

#endif
