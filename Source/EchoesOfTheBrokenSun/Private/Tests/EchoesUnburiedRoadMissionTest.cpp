#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesCampaignProgress.h"
#include "EchoesPlayerController.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesUnburiedRoadMissionModel.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Tests/AutomationCommon.h"

namespace
{
struct FPreservedUnburiedRoadFile final
{
    explicit FPreservedUnburiedRoadFile(FString InPath)
        : Path(MoveTemp(InPath))
    {
        bExisted = IFileManager::Get().FileExists(*Path);
        if (bExisted)
        {
            FFileHelper::LoadFileToArray(Contents, *Path);
        }
    }

    ~FPreservedUnburiedRoadFile()
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

uint8 ChoiceMask(echoes::sim::FutureWellChoice Choice)
{
    switch (Choice)
    {
        case echoes::sim::FutureWellChoice::Harvest: return 1 << 0;
        case echoes::sim::FutureWellChoice::Preserve: return 1 << 1;
        case echoes::sim::FutureWellChoice::Reshape: return 1 << 2;
        default: return 0;
    }
}

FEchoesCampaignDecisionRecord MakeUnburiedRoadPriorRecord(
    EEchoesCampaignMissionId Mission,
    echoes::sim::FutureWellChoice Choice)
{
    FEchoesCampaignDecisionRecord Record;
    Record.Mission = Mission;
    Record.WellChoice = Choice;
    Record.AvailableWellChoices =
        Mission == EEchoesCampaignMissionId::WhatTheLedgerKeeps
            ? 0x07
            : ChoiceMask(Choice);
    switch (Mission)
    {
        case EEchoesCampaignMissionId::WhatTheLedgerKeeps:
            Record.VerifiedFacts =
                static_cast<uint8>(EEchoesCampaignDecisionFact::ArchiveRecovered) |
                static_cast<uint8>(EEchoesCampaignDecisionFact::CarrierEvacuated) |
                static_cast<uint8>(EEchoesCampaignDecisionFact::LocalCoreSurvived) |
                static_cast<uint8>(EEchoesCampaignDecisionFact::FutureWellControlled);
            Record.CompletionTick = 120;
            Record.FinalStateChecksum = 0x7A11A2ULL;
            break;
        case EEchoesCampaignMissionId::SevenAccountsOfRain:
            Record.VerifiedFacts =
                static_cast<uint8>(EEchoesSevenAccountsCompletionFact::WaystoneRootedAtAnchor) |
                static_cast<uint8>(EEchoesSevenAccountsCompletionFact::MemoryBearerArrived) |
                static_cast<uint8>(EEchoesSevenAccountsCompletionFact::LocalCoreSurvived) |
                static_cast<uint8>(EEchoesSevenAccountsCompletionFact::PriorDecisionConsumed);
            Record.CompletionTick = 420;
            Record.FinalStateChecksum = 0x7A11A3ULL;
            break;
        case EEchoesCampaignMissionId::ACityOnReserve:
            Record.VerifiedFacts =
                static_cast<uint8>(EEchoesCityReserveCompletionFact::LifeSupportPowered) |
                static_cast<uint8>(EEchoesCityReserveCompletionFact::TransitPowered) |
                static_cast<uint8>(EEchoesCityReserveCompletionFact::ArchivePowered) |
                static_cast<uint8>(EEchoesCityReserveCompletionFact::LocalCoreSurvived) |
                static_cast<uint8>(EEchoesCityReserveCompletionFact::PriorLedgerConsumed);
            Record.CompletionTick = 720;
            Record.FinalStateChecksum = 0x7A11A4ULL;
            break;
        default:
            break;
    }
    return Record;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesUnburiedRoadMissionTest,
    "Echoes.Runtime.Campaign.TheUnburiedRoad",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesUnburiedRoadMissionTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    const FEchoesUnburiedRoadRoute PreserveRoute =
        FEchoesUnburiedRoadMissionModel::RouteForChoice(
            echoes::sim::FutureWellChoice::Preserve);
    TestTrue(TEXT("Preserve selects the central buried causeway"),
             PreserveRoute.Roadhead == echoes::sim::Vec2::FromTiles(32, 28) &&
                 PreserveRoute.ListeningSpineSite ==
                     echoes::sim::Vec2::FromTiles(32, 37) &&
                 PreserveRoute.MemoryShardSite ==
                     echoes::sim::Vec2::FromTiles(38, 43));
    TestTrue(TEXT("Harvest selects the western ash cut"),
             FEchoesUnburiedRoadMissionModel::RouteForChoice(
                 echoes::sim::FutureWellChoice::Harvest).Roadhead ==
                 echoes::sim::Vec2::FromTiles(14, 28));
    TestTrue(TEXT("Reshape selects the eastern folded verge"),
             FEchoesUnburiedRoadMissionModel::RouteForChoice(
                 echoes::sim::FutureWellChoice::Reshape).Roadhead ==
                 echoes::sim::Vec2::FromTiles(50, 28));

    FEchoesUnburiedRoadMissionFacts Facts;
    TestTrue(TEXT("Inactive facts stay outside mission four"),
             FEchoesUnburiedRoadMissionModel::DeterminePhase(Facts) ==
                 EEchoesUnburiedRoadPhase::Inactive);
    Facts.bOperationActive = true;
    Facts.bLocalCoreIntact = true;
    Facts.bMemoryBearerIntact = true;
    Facts.bWaystoneIntact = true;
    Facts.bSkirmishStillOngoing = true;
    TestTrue(TEXT("A live operation begins at the roadhead"),
             FEchoesUnburiedRoadMissionModel::DeterminePhase(Facts) ==
                 EEchoesUnburiedRoadPhase::EstablishRoadhead);
    Facts.bWaystoneRootedAtRoadhead = true;
    TestTrue(TEXT("A rooted Waystone opens Listening Spine construction"),
             FEchoesUnburiedRoadMissionModel::DeterminePhase(Facts) ==
                 EEchoesUnburiedRoadPhase::RaiseListeningSpine);
    Facts.bListeningSpineComplete = true;
    TestTrue(TEXT("A completed Listening Spine exposes shard recovery"),
             FEchoesUnburiedRoadMissionModel::DeterminePhase(Facts) ==
                 EEchoesUnburiedRoadPhase::RecoverMemoryShard);
    Facts.bMemoryBearerAtShard = true;
    TestTrue(TEXT("Infrastructure-backed recovery completes the mission"),
             FEchoesUnburiedRoadMissionModel::DeterminePhase(Facts) ==
                 EEchoesUnburiedRoadPhase::Complete);
    Facts.bWaystoneIntact = false;
    TestTrue(TEXT("Losing mobile infrastructure fails the mission"),
             FEchoesUnburiedRoadMissionModel::DeterminePhase(Facts) ==
                 EEchoesUnburiedRoadPhase::Failed);

    const FString CampaignPath =
        FEchoesCampaignProgressStore::GetDefaultPath();
    const FString QuickSavePath = FPaths::Combine(
        FPaths::ProjectSavedDir(),
        TEXT("SaveGames"),
        TEXT("EchoesQuickSaveTheUnburiedRoad.bin"));
    FPreservedUnburiedRoadFile PreservedPrimary(CampaignPath);
    FPreservedUnburiedRoadFile PreservedBackup(CampaignPath + TEXT(".bak"));
    FPreservedUnburiedRoadFile PreservedTemporary(CampaignPath + TEXT(".tmp"));
    FPreservedUnburiedRoadFile PreservedQuickSave(QuickSavePath);
    FPreservedUnburiedRoadFile PreservedQuickSaveBackup(
        QuickSavePath + TEXT(".bak"));
    FPreservedUnburiedRoadFile PreservedQuickSaveTemporary(
        QuickSavePath + TEXT(".tmp"));
    IFileManager::Get().Delete(*CampaignPath, false, true, true);
    IFileManager::Get().Delete(*(CampaignPath + TEXT(".bak")), false, true, true);
    IFileManager::Get().Delete(*(CampaignPath + TEXT(".tmp")), false, true, true);

    FString Feedback;
    FEchoesCampaignProgress LockedProgress;
    for (const EEchoesCampaignMissionId Mission : {
             EEchoesCampaignMissionId::WhatTheLedgerKeeps,
             EEchoesCampaignMissionId::SevenAccountsOfRain})
    {
        TestTrue(TEXT("The locked fixture accepts a consistent prior record"),
                 LockedProgress.AppendDecision(
                     MakeUnburiedRoadPriorRecord(
                         Mission,
                         echoes::sim::FutureWellChoice::Preserve),
                     Feedback) == EEchoesCampaignCommitStatus::Added);
    }
    TestTrue(TEXT("The two-record locked fixture is stored"),
             FEchoesCampaignProgressStore::SaveAtomic(
                 CampaignPath,
                 LockedProgress,
                 Feedback));
    {
        FTestWorldWrapper LockedWorld;
        if (!LockedWorld.CreateTestWorld(EWorldType::Game))
        {
            LockedWorld.ForwardErrorMessages(this);
            AddError(TEXT("Could not create the locked mission-four world."));
            return false;
        }
        UEchoesSimulationSubsystem* LockedBridge =
            LockedWorld.GetTestWorld()->GetSubsystem<
                UEchoesSimulationSubsystem>();
        TestFalse(TEXT("Mission four rejects a two-record ledger"),
                  LockedBridge != nullptr &&
                      LockedBridge->SelectOperationMode(
                          EEchoesOperationMode::CampaignUnburiedRoad,
                          Feedback));
        TestTrue(TEXT("The locked response names mission three"),
                 Feedback.Contains(TEXT("A City on Reserve")));
        LockedWorld.ForwardErrorMessages(this);
    }

    FEchoesCampaignProgress MismatchProgress = LockedProgress;
    TestTrue(TEXT("The mismatch fixture accepts an individually valid mission-three record"),
             MismatchProgress.AppendDecision(
                 MakeUnburiedRoadPriorRecord(
                     EEchoesCampaignMissionId::ACityOnReserve,
                     echoes::sim::FutureWellChoice::Harvest),
                 Feedback) == EEchoesCampaignCommitStatus::Added);
    TestTrue(TEXT("The mismatch fixture is stored"),
             FEchoesCampaignProgressStore::SaveAtomic(
                 CampaignPath,
                 MismatchProgress,
                 Feedback));
    {
        FTestWorldWrapper MismatchWorld;
        if (!MismatchWorld.CreateTestWorld(EWorldType::Game))
        {
            MismatchWorld.ForwardErrorMessages(this);
            AddError(TEXT("Could not create the mismatched mission-four world."));
            return false;
        }
        UEchoesSimulationSubsystem* MismatchBridge =
            MismatchWorld.GetTestWorld()->GetSubsystem<
                UEchoesSimulationSubsystem>();
        TestFalse(TEXT("Mission four rejects inconsistent prior choices"),
                  MismatchBridge != nullptr &&
                      MismatchBridge->IsUnburiedRoadUnlocked());
        TestFalse(TEXT("The inconsistent ledger cannot select mission four"),
                  MismatchBridge != nullptr &&
                      MismatchBridge->SelectOperationMode(
                          EEchoesOperationMode::CampaignUnburiedRoad,
                          Feedback));
        MismatchWorld.ForwardErrorMessages(this);
    }

    FEchoesCampaignProgress SeedProgress = LockedProgress;
    TestTrue(TEXT("The fixture accepts the consistent mission-three record"),
             SeedProgress.AppendDecision(
                 MakeUnburiedRoadPriorRecord(
                     EEchoesCampaignMissionId::ACityOnReserve,
                     echoes::sim::FutureWellChoice::Preserve),
                 Feedback) == EEchoesCampaignCommitStatus::Added);
    TestTrue(TEXT("The three-record ledger is stored"),
             FEchoesCampaignProgressStore::SaveAtomic(
                 CampaignPath,
                 SeedProgress,
                 Feedback));

    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create The Unburied Road test world."));
        return false;
    }
    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge =
        World != nullptr
            ? World->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (!TestNotNull(TEXT("Mission world owns the simulation subsystem"), Bridge) ||
        !TestTrue(TEXT("Mission four is unlocked by three consistent records"),
                  Bridge != nullptr && Bridge->IsUnburiedRoadUnlocked()) ||
        !TestTrue(TEXT("Mission four operation can be selected"),
                  Bridge->SelectOperationMode(
                      EEchoesOperationMode::CampaignUnburiedRoad,
                      Feedback)) ||
        !TestTrue(TEXT("Mission four scenario starts"),
                  Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    TestTrue(TEXT("Mission four locks Oruun's Kharuun force"),
             Bridge->GetLocalFaction() ==
                 echoes::sim::Faction::KharuunAssemblies);
    TestTrue(TEXT("Preserve keeps only the central Glass Scar crossing open"),
             Bridge->GetSimulation()->IsPositionPassable(
                 echoes::sim::Vec2::FromTiles(32, 32)) &&
                 !Bridge->GetSimulation()->IsPositionPassable(
                     echoes::sim::Vec2::FromTiles(14, 32)) &&
                 !Bridge->GetSimulation()->IsPositionPassable(
                     echoes::sim::Vec2::FromTiles(50, 32)));
    TestTrue(TEXT("The operation binds Oruun and a mobile Waystone"),
             Bridge->GetMemoryBearerId() != 0 &&
                 Bridge->GetMigrationWaystoneId() != 0);
    Bridge->SetScenarioPaused(false);
    Bridge->Tick(0.05f);
    TestTrue(TEXT("The operation begins at roadhead establishment"),
             Bridge->GetUnburiedRoadPhase() ==
                 EEchoesUnburiedRoadPhase::EstablishRoadhead);
    TestTrue(TEXT("Mission four uses an isolated quick-save slot"),
             Bridge->QuickSaveScenario(Feedback) &&
                 IFileManager::Get().FileExists(*QuickSavePath));
    TestTrue(TEXT("The initial roadhead phase reconstructs after quick load"),
             Bridge->QuickLoadScenario(Feedback) &&
                 Bridge->GetUnburiedRoadPhase() ==
                     EEchoesUnburiedRoadPhase::EstablishRoadhead);

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
    const echoes::sim::EntityId WaystoneId =
        Bridge->GetMigrationWaystoneId();
    const echoes::sim::EntityId BearerId = Bridge->GetMemoryBearerId();
    const FEchoesUnburiedRoadRoute Route = Bridge->GetUnburiedRoadRoute();
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
    TestTrue(TEXT("The mobile Waystone accepts the roadhead order"),
             Bridge->IssueCommand(
                 echoes::sim::CommandType::Move,
                 WaystoneId,
                 0,
                 Bridge->SimToWorld(Route.Roadhead),
                 echoes::sim::FutureWellChoice::Dormant,
                 Feedback));
    TestTrue(TEXT("The Waystone reaches the inherited roadhead"),
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
                         Waystone->position.x.Raw() - Route.Roadhead.x.Raw());
                     const int32 Dy = FMath::Abs(
                         Waystone->position.y.Raw() - Route.Roadhead.y.Raw());
                     return Dx <= echoes::sim::kFixedScale / 2 &&
                            Dy <= echoes::sim::kFixedScale / 2;
                 },
                 2600));
    TestTrue(TEXT("The roadhead Waystone accepts a root order"),
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
                     return Bridge->GetUnburiedRoadPhase() ==
                         EEchoesUnburiedRoadPhase::RaiseListeningSpine;
                 },
                 400));
    TestTrue(TEXT("An ordinary worker accepts the Listening Spine build"),
             Bridge->IssueBuildCommand(
                 WorkerId,
                 echoes::sim::EntityType::UtilityStructure,
                 Bridge->SimToWorld(Route.ListeningSpineSite),
                 Feedback));
    TestTrue(TEXT("Completed construction exposes the missing shard"),
             TickUntil(
                 [Bridge]()
                 {
                     return Bridge->GetUnburiedRoadPhase() ==
                         EEchoesUnburiedRoadPhase::RecoverMemoryShard;
                 },
                 3200));
    TestTrue(TEXT("Oruun accepts the shard recovery order"),
             Bridge->IssueCommand(
                 echoes::sim::CommandType::Move,
                 BearerId,
                 0,
                 Bridge->SimToWorld(Route.MemoryShardSite),
                 echoes::sim::FutureWellChoice::Dormant,
                 Feedback));
    TestTrue(TEXT("Ordinary movement completes shard recovery"),
             TickUntil(
                 [Bridge]()
                 {
                     return Bridge->GetUnburiedRoadPhase() ==
                         EEchoesUnburiedRoadPhase::Complete;
                 },
                 3200));

    const FEchoesCampaignDecisionRecord* MissionRecord =
        Bridge->GetCampaignProgress().FindDecision(
            EEchoesCampaignMissionId::TheUnburiedRoad);
    TestTrue(TEXT("The tick path appends mission four to the ledger"),
             MissionRecord != nullptr &&
                 MissionRecord->WellChoice ==
                     echoes::sim::FutureWellChoice::Preserve &&
                 MissionRecord->AvailableWellChoices == (1 << 1));
    FEchoesCampaignProgress Reloaded;
    TestTrue(TEXT("The four-record campaign reloads transactionally"),
             FEchoesCampaignProgressStore::LoadWithBackup(
                 CampaignPath,
                 Reloaded,
                 Feedback) &&
                 Reloaded.Decisions.Num() == 4);

    AEchoesPlayerController* Controller =
        World->SpawnActor<AEchoesPlayerController>();
    if (TestNotNull(TEXT("Mission-four result controller can be created"),
                    Controller))
    {
        Controller->NotifyUnburiedRoadFinished(
            true,
            echoes::sim::FutureWellChoice::Preserve,
            echoes::sim::FutureWellChoice::Preserve,
            EEchoesCampaignCommitStatus::AlreadyRecorded);
        TestTrue(TEXT("Mission four has a dedicated successful result"),
                 Controller->IsMatchResultVisible() &&
                     Controller->WasCampaignSuccessful() &&
                     Controller->GetPresentedCampaignOperation() ==
                         EEchoesOperationMode::CampaignUnburiedRoad);
        Controller->ConfirmPrimaryAction();
        TestTrue(TEXT("Mission 04 advances to Terms of Continuance briefing"),
                 Bridge->GetOperationMode() ==
                         EEchoesOperationMode::CampaignTermsOfContinuance &&
                     Controller->IsMissionBriefingVisible() &&
                     Bridge->GetTermsOfContinuancePhase() ==
                         EEchoesTermsOfContinuancePhase::SynchronizeNetworks &&
                     Bridge->IsScenarioPaused());
        Controller->Destroy();
    }

    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return true;
}

#endif
