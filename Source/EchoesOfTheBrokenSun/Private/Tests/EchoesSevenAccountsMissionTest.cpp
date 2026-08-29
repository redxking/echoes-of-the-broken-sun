#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesCampaignProgress.h"
#include "EchoesPlayerController.h"
#include "EchoesSevenAccountsMissionModel.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Tests/AutomationCommon.h"

namespace
{
struct FPreservedSevenAccountsFile final
{
    explicit FPreservedSevenAccountsFile(FString InPath)
        : Path(MoveTemp(InPath))
    {
        bExisted = IFileManager::Get().FileExists(*Path);
        if (bExisted)
        {
            FFileHelper::LoadFileToArray(Contents, *Path);
        }
    }

    ~FPreservedSevenAccountsFile()
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

FEchoesCampaignDecisionRecord MakePrologueRecord(
    echoes::sim::FutureWellChoice Choice)
{
    FEchoesCampaignDecisionRecord Record;
    Record.Mission = EEchoesCampaignMissionId::WhatTheLedgerKeeps;
    Record.WellChoice = Choice;
    Record.AvailableWellChoices = 0x07;
    Record.VerifiedFacts =
        static_cast<uint8>(EEchoesCampaignDecisionFact::ArchiveRecovered) |
        static_cast<uint8>(EEchoesCampaignDecisionFact::CarrierEvacuated) |
        static_cast<uint8>(EEchoesCampaignDecisionFact::LocalCoreSurvived) |
        static_cast<uint8>(EEchoesCampaignDecisionFact::FutureWellControlled);
    Record.CompletionTick = 120;
    Record.FinalStateChecksum = 0x7A11A2ULL;
    return Record;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesSevenAccountsMissionTest,
    "Echoes.Runtime.Campaign.SevenAccountsOfRain",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesSevenAccountsMissionTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    FEchoesSevenAccountsMissionFacts Facts;
    TestTrue(TEXT("Inactive facts stay outside mission two"),
             FEchoesSevenAccountsMissionModel::DeterminePhase(Facts) ==
                 EEchoesSevenAccountsPhase::Inactive);
    Facts.bOperationActive = true;
    Facts.bLocalCoreIntact = true;
    Facts.bMemoryBearerIntact = true;
    Facts.bWaystoneIntact = true;
    Facts.bSkirmishStillOngoing = true;
    TestTrue(TEXT("A live route begins with Waystone establishment"),
             FEchoesSevenAccountsMissionModel::DeterminePhase(Facts) ==
                 EEchoesSevenAccountsPhase::EstablishWaystone);
    Facts.bWaystoneRootedAtAnchor = true;
    TestTrue(TEXT("A rooted Waystone unlocks memory recall"),
             FEchoesSevenAccountsMissionModel::DeterminePhase(Facts) ==
                 EEchoesSevenAccountsPhase::RecallMemory);
    Facts.bMemoryBearerAtAccountSite = true;
    TestTrue(TEXT("Waystone and memory arrival complete the mission"),
             FEchoesSevenAccountsMissionModel::DeterminePhase(Facts) ==
                 EEchoesSevenAccountsPhase::Complete);
    Facts.bMemoryBearerIntact = false;
    TestTrue(TEXT("Losing Oruun fails the operation"),
             FEchoesSevenAccountsMissionModel::DeterminePhase(Facts) ==
                 EEchoesSevenAccountsPhase::Failed);

    const FEchoesSevenAccountsRoute Harvest =
        FEchoesSevenAccountsMissionModel::RouteForChoice(
            echoes::sim::FutureWellChoice::Harvest);
    const FEchoesSevenAccountsRoute Preserve =
        FEchoesSevenAccountsMissionModel::RouteForChoice(
            echoes::sim::FutureWellChoice::Preserve);
    const FEchoesSevenAccountsRoute Reshape =
        FEchoesSevenAccountsMissionModel::RouteForChoice(
            echoes::sim::FutureWellChoice::Reshape);
    TestTrue(TEXT("Harvest selects the western account"),
             Harvest.WaystoneAnchor == echoes::sim::Vec2::FromTiles(20, 42));
    TestTrue(TEXT("Preserve selects the central account"),
             Preserve.WaystoneAnchor == echoes::sim::Vec2::FromTiles(35, 40));
    TestTrue(TEXT("Reshape selects the eastern account"),
             Reshape.WaystoneAnchor == echoes::sim::Vec2::FromTiles(40, 42));

    const FString CampaignPath =
        FEchoesCampaignProgressStore::GetDefaultPath();
    FPreservedSevenAccountsFile PreservedPrimary(CampaignPath);
    FPreservedSevenAccountsFile PreservedBackup(CampaignPath + TEXT(".bak"));
    FPreservedSevenAccountsFile PreservedTemporary(CampaignPath + TEXT(".tmp"));
    const FString MissionQuickSavePath = FPaths::Combine(
        FPaths::ProjectSavedDir(),
        TEXT("SaveGames"),
        TEXT("EchoesQuickSaveSevenAccountsOfRain.bin"));
    FPreservedSevenAccountsFile PreservedQuickSave(MissionQuickSavePath);
    FPreservedSevenAccountsFile PreservedQuickSaveBackup(
        MissionQuickSavePath + TEXT(".bak"));
    FPreservedSevenAccountsFile PreservedQuickSaveTemporary(
        MissionQuickSavePath + TEXT(".tmp"));
    IFileManager::Get().Delete(*CampaignPath, false, true, true);
    IFileManager::Get().Delete(*(CampaignPath + TEXT(".bak")), false, true, true);
    IFileManager::Get().Delete(*(CampaignPath + TEXT(".tmp")), false, true, true);

    {
        FTestWorldWrapper LockedWorld;
        if (!LockedWorld.CreateTestWorld(EWorldType::Game))
        {
            LockedWorld.ForwardErrorMessages(this);
            AddError(TEXT("Could not create the locked campaign test world."));
            return false;
        }
        UEchoesSimulationSubsystem* LockedBridge =
            LockedWorld.GetTestWorld()->GetSubsystem<
                UEchoesSimulationSubsystem>();
        FString LockedFeedback;
        TestFalse(TEXT("Mission two rejects a campaign without mission one"),
                  LockedBridge != nullptr && LockedBridge->SelectOperationMode(
                      EEchoesOperationMode::CampaignSevenAccounts,
                      LockedFeedback));
        TestTrue(TEXT("The locked response names the prerequisite"),
                 LockedFeedback.Contains(TEXT("What the Ledger Keeps")));
        LockedWorld.ForwardErrorMessages(this);
    }

    const echoes::sim::FutureWellChoice Branches[] = {
        echoes::sim::FutureWellChoice::Harvest,
        echoes::sim::FutureWellChoice::Preserve,
        echoes::sim::FutureWellChoice::Reshape};
    for (const echoes::sim::FutureWellChoice Branch : Branches)
    {
        FEchoesCampaignProgress BranchProgress;
        FString BranchFeedback;
        TestTrue(TEXT("A branch fixture accepts its prologue record"),
                 BranchProgress.AppendDecision(
                     MakePrologueRecord(Branch),
                     BranchFeedback) == EEchoesCampaignCommitStatus::Added);
        TestTrue(TEXT("A branch fixture saves transactionally"),
                 FEchoesCampaignProgressStore::SaveAtomic(
                     CampaignPath,
                     BranchProgress,
                     BranchFeedback));
        FTestWorldWrapper BranchWorld;
        if (!BranchWorld.CreateTestWorld(EWorldType::Game))
        {
            BranchWorld.ForwardErrorMessages(this);
            AddError(TEXT("Could not create a branch-terrain test world."));
            return false;
        }
        UEchoesSimulationSubsystem* BranchBridge =
            BranchWorld.GetTestWorld()->GetSubsystem<
                UEchoesSimulationSubsystem>();
        TestTrue(TEXT("The branch operation initializes"),
                 BranchBridge != nullptr &&
                     BranchBridge->SelectOperationMode(
                         EEchoesOperationMode::CampaignSevenAccounts,
                         BranchFeedback) &&
                     BranchBridge->StartPrototypeScenario());
        const echoes::sim::Simulation* BranchSimulation =
            BranchBridge != nullptr ? BranchBridge->GetSimulation() : nullptr;
        if (TestNotNull(TEXT("Branch terrain is authoritative"),
                        BranchSimulation))
        {
            const echoes::sim::Terrain Central =
                BranchSimulation->TerrainAt(32, 32);
            const echoes::sim::Terrain ReshapedFlank =
                BranchSimulation->TerrainAt(27, 32);
            if (Branch == echoes::sim::FutureWellChoice::Harvest)
            {
                TestTrue(TEXT("Harvest closes the central crossing"),
                         Central == echoes::sim::Terrain::Blocked);
            }
            else if (Branch == echoes::sim::FutureWellChoice::Preserve)
            {
                TestTrue(TEXT("Preserve retains the authored crossing"),
                         Central == echoes::sim::Terrain::Open &&
                             ReshapedFlank == echoes::sim::Terrain::Blocked);
            }
            else
            {
                TestTrue(TEXT("Reshape opens the flanking columns"),
                         Central == echoes::sim::Terrain::Open &&
                             ReshapedFlank == echoes::sim::Terrain::Open);
            }
        }
        if (BranchBridge != nullptr)
        {
            BranchBridge->StopPrototypeScenario();
        }
        BranchWorld.ForwardErrorMessages(this);
    }

    FEchoesCampaignProgress SeedProgress;
    FString Feedback;
    TestTrue(TEXT("The Preserve prologue prerequisite is valid"),
             SeedProgress.AppendDecision(
                 MakePrologueRecord(echoes::sim::FutureWellChoice::Preserve),
                 Feedback) == EEchoesCampaignCommitStatus::Added);
    TestTrue(TEXT("The prerequisite campaign ledger is written"),
             FEchoesCampaignProgressStore::SaveAtomic(
                 CampaignPath,
                 SeedProgress,
                 Feedback));

    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the Seven Accounts test world."));
        return false;
    }
    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge =
        World != nullptr
            ? World->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (!TestNotNull(TEXT("Mission world owns the simulation subsystem"), Bridge) ||
        !TestTrue(TEXT("Mission two is unlocked by the prologue record"),
                  Bridge != nullptr && Bridge->IsSevenAccountsUnlocked()) ||
        !TestTrue(TEXT("Mission two operation can be selected"),
                  Bridge->SelectOperationMode(
                      EEchoesOperationMode::CampaignSevenAccounts,
                      Feedback)) ||
        !TestTrue(TEXT("Mission two scenario starts"),
                  Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    TestTrue(TEXT("Mission two locks the Kharuun force"),
             Bridge->GetLocalFaction() ==
                 echoes::sim::Faction::KharuunAssemblies);
    TestTrue(TEXT("The stored Preserve decision selects the central route"),
             Bridge->GetSevenAccountsRoute().PriorChoice ==
                     echoes::sim::FutureWellChoice::Preserve &&
                 Bridge->GetSevenAccountsRoute().WaystoneAnchor ==
                     echoes::sim::Vec2::FromTiles(35, 40));
    TestTrue(TEXT("The mission begins at Waystone establishment"),
             Bridge->GetSevenAccountsPhase() ==
                 EEchoesSevenAccountsPhase::EstablishWaystone);
    TestTrue(TEXT("Mission two writes to its isolated quick-save slot"),
             Bridge->QuickSaveScenario(Feedback) &&
                 IFileManager::Get().FileExists(*MissionQuickSavePath));
    TestTrue(TEXT("Mission two restores its reconstructable initial phase"),
             Bridge->QuickLoadScenario(Feedback) &&
                 Bridge->GetSevenAccountsPhase() ==
                     EEchoesSevenAccountsPhase::EstablishWaystone);

    const echoes::sim::EntityId WaystoneId =
        Bridge->GetMigrationWaystoneId();
    const echoes::sim::EntityId BearerId = Bridge->GetMemoryBearerId();
    TestTrue(TEXT("The mission binds an authoritative Waystone and Oruun"),
             WaystoneId != 0 && BearerId != 0);
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
    TestTrue(TEXT("The rooted Waystone accepts an uproot order"),
             Bridge->IssueCommand(
                 echoes::sim::CommandType::ToggleWaystoneRoot,
                 WaystoneId,
                 0,
                 FVector::ZeroVector,
                 echoes::sim::FutureWellChoice::Dormant,
                 Feedback));
    TestTrue(TEXT("The Waystone completes its authored uproot transition"),
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
    const FEchoesSevenAccountsRoute Route = Bridge->GetSevenAccountsRoute();
    TestTrue(TEXT("The mobile Waystone accepts the migration order"),
             Bridge->IssueCommand(
                 echoes::sim::CommandType::Move,
                 WaystoneId,
                 0,
                 Bridge->SimToWorld(Route.WaystoneAnchor),
                 echoes::sim::FutureWellChoice::Dormant,
                 Feedback));
    TestTrue(TEXT("The Waystone reaches the branch-specific anchor"),
             TickUntil(
                 [Bridge, WaystoneId, Route]()
                 {
                     const echoes::sim::Entity* Waystone =
                         Bridge->FindEntity(WaystoneId);
                     if (Waystone == nullptr)
                     {
                         return false;
                     }
                     const int32 Dx = FMath::Abs(
                         Waystone->position.x.Raw() -
                         Route.WaystoneAnchor.x.Raw());
                     const int32 Dy = FMath::Abs(
                         Waystone->position.y.Raw() -
                         Route.WaystoneAnchor.y.Raw());
                     return Dx <= echoes::sim::kFixedScale / 2 &&
                            Dy <= echoes::sim::kFixedScale / 2;
                 },
                 2400));
    TestTrue(TEXT("The migrated Waystone accepts a root order"),
             Bridge->IssueCommand(
                 echoes::sim::CommandType::ToggleWaystoneRoot,
                 WaystoneId,
                 0,
                 FVector::ZeroVector,
                 echoes::sim::FutureWellChoice::Dormant,
                 Feedback));
    TestTrue(TEXT("Rooting at the anchor unlocks Oruun's account"),
             TickUntil(
                 [Bridge]()
                 {
                     return Bridge->GetSevenAccountsPhase() ==
                         EEchoesSevenAccountsPhase::RecallMemory;
                 },
                 400));
    TestTrue(TEXT("Oruun accepts the route recall order"),
             Bridge->IssueCommand(
                 echoes::sim::CommandType::Move,
                 BearerId,
                 0,
                 Bridge->SimToWorld(Route.MemoryAccountSite),
                 echoes::sim::FutureWellChoice::Dormant,
                 Feedback));
    TestTrue(TEXT("Ordinary movement completes the migration and recall"),
             TickUntil(
                 [Bridge]()
                 {
                     return Bridge->GetSevenAccountsPhase() ==
                         EEchoesSevenAccountsPhase::Complete;
                 },
                 2600));

    const FEchoesCampaignDecisionRecord* MissionRecord =
        Bridge->GetCampaignProgress().FindDecision(
            EEchoesCampaignMissionId::SevenAccountsOfRain);
    TestTrue(TEXT("The tick path appends mission two to the ledger"),
             MissionRecord != nullptr &&
                 MissionRecord->WellChoice ==
                     echoes::sim::FutureWellChoice::Preserve &&
                 MissionRecord->AvailableWellChoices == (1 << 1));
    FEchoesCampaignProgress Reloaded;
    TestTrue(TEXT("The two-record campaign reloads transactionally"),
             FEchoesCampaignProgressStore::LoadWithBackup(
                 CampaignPath,
                 Reloaded,
                 Feedback) &&
                 Reloaded.Decisions.Num() == 2);

    AEchoesPlayerController* Controller =
        World->SpawnActor<AEchoesPlayerController>();
    if (TestNotNull(TEXT("Mission result controller can be created"), Controller))
    {
        Controller->NotifySevenAccountsFinished(
            true,
            echoes::sim::FutureWellChoice::Preserve,
            echoes::sim::FutureWellChoice::Preserve,
            EEchoesCampaignCommitStatus::AlreadyRecorded);
        TestTrue(TEXT("Mission two has a dedicated successful result"),
                 Controller->IsMatchResultVisible() &&
                     Controller->WasCampaignSuccessful() &&
                     Controller->GetPresentedCampaignOperation() ==
                         EEchoesOperationMode::CampaignSevenAccounts);
        Controller->ConfirmPrimaryAction();
        TestTrue(TEXT("Replay retains mission two and reconstructs its start"),
                 Bridge->GetOperationMode() ==
                         EEchoesOperationMode::CampaignSevenAccounts &&
                     Bridge->GetSevenAccountsPhase() ==
                         EEchoesSevenAccountsPhase::EstablishWaystone);
        Controller->PresentTitleScreen();
        Controller->RequestNewCampaign();
        TestTrue(TEXT("The first F10 press only arms confirmation"),
                 Controller->IsNewCampaignConfirmationArmed() &&
                     Bridge->GetCampaignProgress().Decisions.Num() == 2);
        Controller->RequestNewCampaign();
        TestTrue(TEXT("The confirmed new campaign clears active decisions"),
                 !Controller->IsNewCampaignConfirmationArmed() &&
                     Bridge->GetCampaignProgress().Decisions.IsEmpty());
        TestTrue(TEXT("New-campaign authority returns to the default operation"),
                 Bridge->GetOperationMode() ==
                         EEchoesOperationMode::Skirmish &&
                     Bridge->GetLocalFaction() ==
                         echoes::sim::Faction::MeridianCompact &&
                     Bridge->IsScenarioPaused());
        FEchoesCampaignProgress EmptyReload;
        TestTrue(TEXT("The empty active ledger reloads transactionally"),
                 FEchoesCampaignProgressStore::LoadWithBackup(
                     CampaignPath,
                     EmptyReload,
                     Feedback) &&
                     EmptyReload.Decisions.IsEmpty());
        FEchoesCampaignProgress RetainedPriorGeneration;
        TestTrue(TEXT("One prior campaign generation remains recoverable"),
                 FEchoesCampaignProgressStore::LoadWithBackup(
                     CampaignPath + TEXT(".bak"),
                     RetainedPriorGeneration,
                     Feedback) &&
                     RetainedPriorGeneration.Decisions.Num() == 2);
        Controller->Destroy();
    }

    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return true;
}

#endif
