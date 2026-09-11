#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "EchoesPreservedTestFile.h"

#include "EchoesTestSaveEnvironment.h"

#include "EchoesPlayerController.h"
#include "EchoesPrologueMissionModel.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesNarrativeSubsystem.h"
#include "Algo/AnyOf.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Tests/AutomationCommon.h"

namespace
{
using FPreservedPrologueCampaignFile = FEchoesPreservedTestFile;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesPrologueMissionTest,
    "Echoes.Runtime.Campaign.WhatTheLedgerKeeps",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesPrologueMissionTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    FEchoesScopedTestSaveEnvironment TestSaveEnvironment(*this);
    if (!TestSaveEnvironment.IsReady())
    {
        return false;
    }

    const FString CampaignPath =
        FEchoesCampaignProgressStore::GetDefaultPath();
    FPreservedPrologueCampaignFile PreservedCampaign(CampaignPath);
    if (!PreservedCampaign.IsReady()) return false;
    FPreservedPrologueCampaignFile PreservedBackup(
        CampaignPath + TEXT(".bak"));
    if (!PreservedBackup.IsReady()) return false;
    FPreservedPrologueCampaignFile PreservedTemporary(
        CampaignPath + TEXT(".tmp"));
    if (!PreservedTemporary.IsReady()) return false;
    IFileManager::Get().Delete(*CampaignPath, false, true, true);
    IFileManager::Get().Delete(
        *(CampaignPath + TEXT(".bak")), false, true, true);
    IFileManager::Get().Delete(
        *(CampaignPath + TEXT(".tmp")), false, true, true);

    FEchoesPrologueMissionFacts Facts;
    TestTrue(
        TEXT("Inactive facts remain outside the campaign state machine"),
        FEchoesPrologueMissionModel::DeterminePhase(Facts) ==
            EEchoesProloguePhase::Inactive);
    Facts.bOperationActive = true;
    Facts.bLocalCoreIntact = true;
    Facts.bArchiveCarrierIntact = true;
    TestTrue(
        TEXT("A live prologue begins with archive recovery"),
        FEchoesPrologueMissionModel::DeterminePhase(Facts) ==
            EEchoesProloguePhase::RecoverArchive);
    Facts.bArchiveCarrierAtRecoverySite = true;
    TestTrue(
        TEXT("Holding the archive site unlocks the Well decision"),
        FEchoesPrologueMissionModel::DeterminePhase(Facts) ==
            EEchoesProloguePhase::DecideFutureWell);
    Facts.bFutureWellProtocolChosen = true;
    TestTrue(
        TEXT("A committed protocol changes the mission to withdrawal"),
        FEchoesPrologueMissionModel::DeterminePhase(Facts) ==
            EEchoesProloguePhase::Withdraw);
    Facts.bArchiveCarrierAtEvacuationSite = true;
    TestTrue(
        TEXT("Evacuation after a Well decision completes the mission"),
        FEchoesPrologueMissionModel::DeterminePhase(Facts) ==
            EEchoesProloguePhase::Complete);
    Facts.bArchiveCarrierIntact = false;
    TestTrue(
        TEXT("Losing the archive carrier fails the mission"),
        FEchoesPrologueMissionModel::DeterminePhase(Facts) ==
            EEchoesProloguePhase::Failed);
    Facts.bArchiveCarrierIntact = true;
    Facts.bFutureWellLost = true;
    TestTrue(
        TEXT("Losing the Future Well to the opposing force fails the mission"),
        FEchoesPrologueMissionModel::DeterminePhase(Facts) ==
            EEchoesProloguePhase::Failed);

    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the prologue test world."));
        return false;
    }
    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge =
        World != nullptr
            ? World->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (!TestNotNull(TEXT("Campaign world owns the simulation subsystem"), Bridge) ||
        !TestTrue(TEXT("Default skirmish starts before operation selection"),
                  Bridge != nullptr && Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    AEchoesPlayerController* Controller =
        World->SpawnActor<AEchoesPlayerController>();
    if (!TestNotNull(TEXT("Campaign controller can be created"), Controller))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    Controller->InitInputSystem();
    if (!TestNotNull(TEXT("Campaign controller initializes input"),
                     Controller->PlayerInput.Get()))
    {
        Controller->Destroy();
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    Controller->PresentTitleScreen();
    Controller->CycleOperation();
    TestTrue(TEXT("F9 path selects the campaign prologue"),
             Bridge->GetOperationMode() ==
                 EEchoesOperationMode::CampaignPrologue);
    TestTrue(TEXT("Campaign selection remains paused on the title screen"),
             Bridge->IsScenarioPaused());
    TestTrue(TEXT("The prologue is bound to Mara Vey's Meridian force"),
             Bridge->GetLocalFaction() ==
                 echoes::sim::Faction::MeridianCompact);
    TestTrue(TEXT("The prologue starts at archive recovery"),
             Bridge->GetProloguePhase() ==
                 EEchoesProloguePhase::RecoverArchive);

    const echoes::sim::EntityId CarrierId = Bridge->GetArchiveCarrierId();
    echoes::sim::Simulation* Simulation =
        const_cast<echoes::sim::Simulation*>(Bridge->GetSimulation());
    echoes::sim::EntityId WorkerId = 0;
    echoes::sim::EntityId BarracksId = 0;
    echoes::sim::EntityId WellId = 0;
    int32 LocalWorkerCount = 0;
    int32 LocalSoldierCount = 0;
    int32 LocalHeavyCount = 0;
    int32 LocalScoutCount = 0;
    int32 LocalCommandCoreCount = 0;
    int32 LocalBarracksCount = 0;
    int32 LocalDropoffCount = 0;
    int32 LocalUtilityCount = 0;
    TArray<FIntPoint> WorkerTiles;
    TArray<FIntPoint> SoldierTiles;
    if (Simulation != nullptr)
    {
        for (const echoes::sim::Entity& Entity : Simulation->Entities())
        {
            if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId)
            {
                switch (Entity.type)
                {
                    case echoes::sim::EntityType::Worker:
                        ++LocalWorkerCount;
                        WorkerId = WorkerId == 0 ? Entity.id : WorkerId;
                        WorkerTiles.Add(FIntPoint(
                            Entity.position.x.FloorToInt(),
                            Entity.position.y.FloorToInt()));
                        break;
                    case echoes::sim::EntityType::Soldier:
                        ++LocalSoldierCount;
                        SoldierTiles.Add(FIntPoint(
                            Entity.position.x.FloorToInt(),
                            Entity.position.y.FloorToInt()));
                        break;
                    case echoes::sim::EntityType::HeavyUnit:
                        ++LocalHeavyCount;
                        break;
                    case echoes::sim::EntityType::ScoutUnit:
                        ++LocalScoutCount;
                        break;
                    case echoes::sim::EntityType::CommandCore:
                        ++LocalCommandCoreCount;
                        break;
                    case echoes::sim::EntityType::Barracks:
                        ++LocalBarracksCount;
                        BarracksId = BarracksId == 0 ? Entity.id : BarracksId;
                        break;
                    case echoes::sim::EntityType::Dropoff:
                        ++LocalDropoffCount;
                        break;
                    case echoes::sim::EntityType::UtilityStructure:
                        ++LocalUtilityCount;
                        break;
                    case echoes::sim::EntityType::ResourceNode:
                    case echoes::sim::EntityType::FutureWell:
                        break;
                }
            }
            if (Entity.type == echoes::sim::EntityType::FutureWell)
            {
                WellId = Entity.id;
            }
        }
    }
    TestTrue(TEXT("Mara Vey's scout is the explicit archive carrier"),
             CarrierId != 0);
    TestTrue(
        TEXT("The mission retains a worker, Barracks, and central Future Well"),
        WorkerId != 0 && BarracksId != 0 && WellId != 0);
    TestTrue(
        TEXT("SPEC-PLAN-001 starts M01 with six Surveyors and two Lancers"),
        LocalWorkerCount == 6 && LocalSoldierCount == 2);
    TestTrue(
        TEXT("M01 preserves every other local starting role count"),
        LocalCommandCoreCount == 1 && LocalBarracksCount == 1 &&
        LocalDropoffCount == 1 && LocalHeavyCount == 1 &&
        LocalScoutCount == 1 && LocalUtilityCount == 1);
    WorkerTiles.Sort([](const FIntPoint& Left, const FIntPoint& Right)
    {
        return Left.Y == Right.Y ? Left.X < Right.X : Left.Y < Right.Y;
    });
    SoldierTiles.Sort([](const FIntPoint& Left, const FIntPoint& Right)
    {
        return Left.Y == Right.Y ? Left.X < Right.X : Left.Y < Right.Y;
    });
    TArray<FIntPoint> ExpectedWorkerTiles{
        {8, 13}, {14, 13}, {11, 14}, {14, 15}, {8, 16}, {11, 17}};
    TArray<FIntPoint> ExpectedSoldierTiles{{12, 7}, {6, 8}};
    TestTrue(
        TEXT("M01 Surveyor deployment uses the source-cleared starting tiles"),
        WorkerTiles == ExpectedWorkerTiles);
    TestTrue(
        TEXT("M01 Lancer deployment preserves the two authored starting tiles"),
        SoldierTiles == ExpectedSoldierTiles);
    const echoes::sim::PlayerState* ProloguePlayer =
        Simulation != nullptr
            ? Simulation->FindPlayer(
                  UEchoesSimulationSubsystem::LocalPlayerId)
            : nullptr;
    const int32 ReshapeDawnCost =
        Simulation != nullptr
            ? Simulation->Config().rules.futureWell.reshapeDawnCost
            : -1;
    const int32 StartingDawn =
        ProloguePlayer != nullptr
            ? ProloguePlayer->resources.dawnshards
            : -1;
    TestTrue(
        TEXT("The Prologue starts at the exact baseline-or-Reshape minimum"),
        ReshapeDawnCost > 0 &&
            StartingDawn == FMath::Max(30, ReshapeDawnCost));

    // Snapshot projection only: mutate the documented test seam without
    // ticking, and restore before the existing ordinary-command journey.
    auto* SnapshotWell = Simulation ? Simulation->MutableEntityForTesting(WellId) : nullptr;
    if (!TestNotNull(TEXT("objective projection Well fixture"), SnapshotWell)) return false;
    const auto SavedWellOwner = SnapshotWell->owner;
    const auto SavedWellChoice = SnapshotWell->wellChoice;
    const auto SavedWellUntil = SnapshotWell->reshapeUntilTick;
    const auto SnapshotTick = Simulation->CurrentTick();
    TestFalse(TEXT("initial Well is outside local visibility"),
        Simulation->IsEntityVisibleTo(UEchoesSimulationSubsystem::LocalPlayerId, WellId));
    SnapshotWell->owner = UEchoesSimulationSubsystem::OpponentPlayerId;
    SnapshotWell->wellChoice = echoes::sim::FutureWellChoice::Reshape;
    SnapshotWell->reshapeUntilTick = SnapshotTick + 200;
    const FEchoesObjectiveSnapshot HiddenWell = Bridge->GetLocalObjectiveSnapshot();
    TestTrue(TEXT("hidden enemy Well does not disclose protocol, control or deadline"),
        !HiddenWell.bFutureWellVisible && !HiddenWell.bPrologueWellEnemyControlled &&
        !HiddenWell.bPrologueReshapeExpired && HiddenWell.PrologueReshapeRemainingTicks == 0 &&
        HiddenWell.PrologueWellChoice == echoes::sim::FutureWellChoice::Dormant);
    SnapshotWell->owner = UEchoesSimulationSubsystem::LocalPlayerId;
    SnapshotWell->reshapeUntilTick = SnapshotTick + 201;
    const FEchoesObjectiveSnapshot ActiveWell = Bridge->GetLocalObjectiveSnapshot();
    TestTrue(TEXT("owned active Reshape publishes exact remaining interval"),
        ActiveWell.PrologueWellChoice == echoes::sim::FutureWellChoice::Reshape &&
        !ActiveWell.bPrologueWellEnemyControlled && !ActiveWell.bPrologueReshapeExpired &&
        ActiveWell.PrologueReshapeRemainingTicks == 201);
    SnapshotWell->reshapeUntilTick = SnapshotTick + 200;
    TestEqual(TEXT("ten-second warning boundary is200 authoritative ticks"),
        Bridge->GetLocalObjectiveSnapshot().PrologueReshapeRemainingTicks, static_cast<uint64>(200));
    SnapshotWell->reshapeUntilTick = 0;
    const FEchoesObjectiveSnapshot ExpiredWell = Bridge->GetLocalObjectiveSnapshot();
    TestTrue(TEXT("expired Reshape retains identity without a live countdown"),
        ExpiredWell.PrologueWellChoice == echoes::sim::FutureWellChoice::Reshape &&
        ExpiredWell.bPrologueReshapeExpired && ExpiredWell.PrologueReshapeRemainingTicks == 0);
    SnapshotWell->owner = SavedWellOwner;
    SnapshotWell->wellChoice = SavedWellChoice;
    SnapshotWell->reshapeUntilTick = SavedWellUntil;

    FString Feedback;
    const echoes::sim::Entity* Well = Bridge->FindEntity(WellId);
    TestFalse(
        TEXT("A Well protocol is rejected until the archive site is held"),
        WorkerId != 0 && Well != nullptr &&
            Bridge->IssueCommand(
                echoes::sim::CommandType::FutureWell,
                WorkerId,
                WellId,
                Bridge->SimToWorld(Well->position),
                echoes::sim::FutureWellChoice::Preserve,
                Feedback));
    TestTrue(TEXT("The rejection explains the archive prerequisite"),
             Feedback.Contains(TEXT("ARCHIVE_REQUIRED")));

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

    // The approved M01 force begins two Logistics above the Anchor's capacity.
    // Extend the live grid to the evacuation Power Link through the ordinary
    // construction path before exercising production and Dawn accounting.
    const echoes::sim::Vec2 ConnectingLinkSite =
        echoes::sim::Vec2::FromTiles(13, 17);
    Feedback.Reset();
    if (!TestTrue(
            TEXT("A Surveyor accepts the connecting Power Link construction"),
            Bridge->IssueBuildCommand(
                WorkerId,
                echoes::sim::EntityType::Dropoff,
                Bridge->SimToWorld(ConnectingLinkSite),
                Feedback)))
    {
        AddError(FString::Printf(
            TEXT("M01 connecting Power Link admission failed: %s"), *Feedback));
        return false;
    }
    TestTrue(
        TEXT("Ordinary construction completes the connecting Power Link"),
        TickUntil(
            [Bridge, ConnectingLinkSite]()
            {
                const echoes::sim::Simulation* Current =
                    Bridge->GetSimulation();
                return Current != nullptr &&
                    Algo::AnyOf(
                        Current->Entities(),
                        [ConnectingLinkSite](const echoes::sim::Entity& Entity)
                        {
                            return Entity.owner ==
                                       UEchoesSimulationSubsystem::LocalPlayerId &&
                                Entity.type == echoes::sim::EntityType::Dropoff &&
                                Entity.position == ConnectingLinkSite &&
                                Entity.completed && Entity.networkOperational;
                        });
            },
            500));
    Feedback.Reset();
    TestTrue(
        TEXT("The archive carrier accepts an ordinary move to the rendezvous"),
        Bridge->IssueCommand(
            echoes::sim::CommandType::Move,
            CarrierId,
            0,
            Bridge->SimToWorld(
                UEchoesSimulationSubsystem::GetArchiveRecoverySite()),
            echoes::sim::FutureWellChoice::Dormant,
            Feedback));
    TestTrue(
        TEXT("Ordinary deterministic movement reaches the archive objective"),
        TickUntil(
            [Bridge]()
            {
                return Bridge->GetProloguePhase() ==
                    EEchoesProloguePhase::DecideFutureWell;
            },
            800));

    Feedback.Reset();
    TestTrue(
        TEXT("A worker accepts an ordinary scouting move toward the Well"),
        Bridge->IssueCommand(
            echoes::sim::CommandType::Move,
            WorkerId,
            0,
            Bridge->SimToWorld(echoes::sim::Vec2::FromTiles(29, 29)),
            echoes::sim::FutureWellChoice::Dormant,
            Feedback));
    TestTrue(
        TEXT("The campaign player legitimately reveals the Future Well"),
        TickUntil(
            [Bridge, WellId]()
            {
                const echoes::sim::Simulation* Current =
                    Bridge->GetSimulation();
                return Current != nullptr &&
                    Current->IsEntityVisibleTo(
                        UEchoesSimulationSubsystem::LocalPlayerId,
                        WellId);
            },
            900));
    {
        auto* VisibleWell = Simulation->MutableEntityForTesting(WellId);
        const auto PriorOwner = VisibleWell->owner;
        const auto PriorChoice = VisibleWell->wellChoice;
        const auto PriorUntil = VisibleWell->reshapeUntilTick;
        VisibleWell->owner = UEchoesSimulationSubsystem::OpponentPlayerId;
        VisibleWell->wellChoice = echoes::sim::FutureWellChoice::Reshape;
        VisibleWell->reshapeUntilTick = Simulation->CurrentTick() + 200;
        const FEchoesObjectiveSnapshot VisibleEnemy = Bridge->GetLocalObjectiveSnapshot();
        TestTrue(TEXT("legitimate visibility admits enemy Well control and deadline"),
            VisibleEnemy.bFutureWellVisible && VisibleEnemy.bPrologueWellEnemyControlled &&
            !VisibleEnemy.bPrologueReshapeExpired && VisibleEnemy.PrologueReshapeRemainingTicks == 200);
        VisibleWell->owner = PriorOwner;
        VisibleWell->wellChoice = PriorChoice;
        VisibleWell->reshapeUntilTick = PriorUntil;
    }
    Feedback.Reset();
    if (!TestTrue(
            TEXT("Ordinary Soldier production queues before the Well decision"),
            Bridge->IssueProductionCommand(
                BarracksId,
                echoes::sim::EntityType::Soldier,
                Feedback)))
    {
        AddError(FString::Printf(
            TEXT("M01 production admission failed: %s"), *Feedback));
        return false;
    }
    TestTrue(
        TEXT("Queued production executes and reduces Dawn below Reshape cost"),
        TickUntil(
            [Bridge, StartingDawn, ReshapeDawnCost]()
            {
                const echoes::sim::Simulation* Current =
                    Bridge->GetSimulation();
                const echoes::sim::PlayerState* Player =
                    Current != nullptr
                        ? Current->FindPlayer(
                              UEchoesSimulationSubsystem::LocalPlayerId)
                        : nullptr;
                return Player != nullptr &&
                    Player->resources.dawnshards < StartingDawn &&
                    Player->resources.dawnshards < ReshapeDawnCost;
            },
            4));
    const echoes::sim::Simulation* PostProductionSimulation =
        Bridge->GetSimulation();
    const echoes::sim::PlayerState* PostProductionPlayer =
        PostProductionSimulation != nullptr
            ? PostProductionSimulation->FindPlayer(
                  UEchoesSimulationSubsystem::LocalPlayerId)
            : nullptr;
    const int32 DawnBeforeRejectedReshape =
        PostProductionPlayer != nullptr
            ? PostProductionPlayer->resources.dawnshards
            : -1;
    TestTrue(
        TEXT("Ordinary production leaves Reshape immediately unaffordable"),
        DawnBeforeRejectedReshape >= 0 &&
            DawnBeforeRejectedReshape < ReshapeDawnCost);
    Feedback.Reset();
    TestFalse(
        TEXT("An immediately unaffordable Reshape is rejected before queuing"),
        Bridge->IssueCommand(
            echoes::sim::CommandType::FutureWell,
            WorkerId,
            WellId,
            Bridge->SimToWorld(Bridge->FindEntity(WellId)->position),
            echoes::sim::FutureWellChoice::Reshape,
            Feedback));
    TestTrue(
        TEXT("The Reshape rejection reports the authored Dawn shortfall"),
        Feedback.Contains(TEXT("WELL_RESHAPE_INSUFFICIENT_DAWN")) &&
            !Feedback.Contains(TEXT("[QUEUED]")));
    const echoes::sim::Simulation* PostRejectionSimulation =
        Bridge->GetSimulation();
    const echoes::sim::PlayerState* PostRejectionPlayer =
        PostRejectionSimulation != nullptr
            ? PostRejectionSimulation->FindPlayer(
                  UEchoesSimulationSubsystem::LocalPlayerId)
            : nullptr;
    const echoes::sim::Entity* RejectedWell =
        Bridge->FindEntity(WellId);
    TestTrue(
        TEXT("Rejected Reshape neither spends Dawn nor changes the dormant Well"),
        PostRejectionPlayer != nullptr && RejectedWell != nullptr &&
            PostRejectionPlayer->resources.dawnshards ==
                DawnBeforeRejectedReshape &&
            RejectedWell->wellChoice ==
                echoes::sim::FutureWellChoice::Dormant &&
            RejectedWell->wellActivationTick == 0);
    UEchoesNarrativeSubsystem* Narrative = World->GetGameInstance()
        ? World->GetGameInstance()->GetSubsystem<UEchoesNarrativeSubsystem>() : nullptr;
    if (!TestNotNull(TEXT("Live mission owns its narrative subsystem"), Narrative)) return false;
    Narrative->ClearSubtitleQueue();
    Feedback.Reset();
    TestTrue(
        TEXT("The free Preserve protocol remains available after rejection"),
        Bridge->IssueCommand(
            echoes::sim::CommandType::FutureWell,
            WorkerId,
            WellId,
            Bridge->SimToWorld(Bridge->FindEntity(WellId)->position),
            echoes::sim::FutureWellChoice::Preserve,
            Feedback));
    TestTrue(
        TEXT("The chosen consequence changes the live mission to withdrawal"),
        TickUntil(
            [Bridge]()
            {
                return Bridge->GetProloguePhase() ==
                    EEchoesProloguePhase::Withdraw;
            },
            900));
    const echoes::sim::Entity* PreservedWell =
        Bridge->FindEntity(WellId);
    TestEqual(TEXT("Real Preserve commit dispatches only its trio and common withdrawal"),
        Narrative->GetQueuedLineCountForTest(), 6);
    TArray<FEchoesNarrativeLine> ExpectedLines = Narrative->GetLinesForSignal(
        EEchoesOperationMode::CampaignPrologue, TEXT("phase_entered:Withdraw:Preserve"));
    ExpectedLines.Append(Narrative->GetLinesForSignal(
        EEchoesOperationMode::CampaignPrologue, TEXT("phase_entered:Withdraw")));
    double SubtitleTime = World->GetRealTimeSeconds();
    for (const FEchoesNarrativeLine& Expected : ExpectedLines)
    {
        FString Speaker, Text;
        TestTrue(TEXT("Committed branch line reaches the live subtitle lane"),
            Narrative->GetActiveSubtitle(SubtitleTime, Speaker, Text));
        TestEqual(TEXT("Actual dispatch preserves chosen branch text order"), Text, Expected.Text);
        TestEqual(TEXT("Actual dispatch preserves the authored speaker"), Speaker, Expected.Speaker);
        SubtitleTime += UEchoesNarrativeSubsystem::SubtitleDurationSeconds(Expected.Text) + 0.01;
    }
    FString FinishedSpeaker, FinishedText;
    TestFalse(TEXT("No unchosen branch follows common withdrawal"),
        Narrative->GetActiveSubtitle(SubtitleTime, FinishedSpeaker, FinishedText));
    const echoes::sim::Simulation* PostPreserveSimulation =
        Bridge->GetSimulation();
    TestTrue(
        TEXT("Preserve activates under local authority after the rejected Reshape"),
        PostPreserveSimulation != nullptr && PreservedWell != nullptr &&
            PreservedWell->owner ==
                UEchoesSimulationSubsystem::LocalPlayerId &&
            PreservedWell->wellChoice ==
                echoes::sim::FutureWellChoice::Preserve &&
            PreservedWell->wellActivationTick > 0 &&
            PreservedWell->wellActivationTick <=
                PostPreserveSimulation->CurrentTick());

    Feedback.Reset();
    TestTrue(
        TEXT("The archive carrier accepts the authored withdrawal order"),
        Bridge->IssueCommand(
            echoes::sim::CommandType::Move,
            CarrierId,
            0,
            Bridge->SimToWorld(
                UEchoesSimulationSubsystem::GetEvacuationSite()),
            echoes::sim::FutureWellChoice::Dormant,
            Feedback));
    TestTrue(
        TEXT("Ordinary deterministic withdrawal completes the mission"),
        TickUntil(
            [Bridge]()
            {
                return Bridge->GetProloguePhase() ==
                    EEchoesProloguePhase::Complete;
            },
            900));

    echoes::sim::FutureWellChoice RecordedConsequence =
        echoes::sim::FutureWellChoice::Dormant;
    Feedback.Reset();
    const EEchoesCampaignCommitStatus CommitStatus =
        Bridge->CommitPrologueCompletion(
            echoes::sim::FutureWellChoice::Preserve,
            RecordedConsequence,
            Feedback);
    TestTrue(TEXT("The tick path already committed authoritative completion"),
             CommitStatus ==
                 EEchoesCampaignCommitStatus::AlreadyRecorded);
    TestTrue(TEXT("The committed ledger retains Preserve"),
             RecordedConsequence == echoes::sim::FutureWellChoice::Preserve);
    TestTrue(TEXT("The transactional campaign file exists"),
             IFileManager::Get().FileExists(*CampaignPath));
    FEchoesCampaignProgress PersistedProgress;
    TestTrue(TEXT("The integrated campaign record reloads from storage"),
             FEchoesCampaignProgressStore::LoadWithBackup(
                 CampaignPath,
                 PersistedProgress,
                 Feedback));
    const FEchoesCampaignDecisionRecord* PersistedDecision =
        PersistedProgress.FindDecision(
            EEchoesCampaignMissionId::WhatTheLedgerKeeps);
    TestTrue(TEXT("Reloaded progress binds the prologue to Preserve"),
             PersistedDecision != nullptr &&
                 PersistedDecision->WellChoice ==
                     echoes::sim::FutureWellChoice::Preserve);

    Controller->NotifyCampaignPrologueFinished(
        true,
        echoes::sim::FutureWellChoice::Preserve,
        RecordedConsequence,
        CommitStatus);
    TestTrue(TEXT("Campaign completion uses the dedicated result presentation"),
             Controller->IsMatchResultVisible() &&
                 Controller->IsCampaignResult() &&
                 Controller->WasCampaignSuccessful());
    TestTrue(TEXT("The Well consequence survives into the campaign result"),
             Controller->GetCampaignConsequence() ==
                 echoes::sim::FutureWellChoice::Preserve);
    TestTrue(TEXT("The result confirms the existing campaign consequence"),
             Controller->GetRecordedCampaignConsequence() ==
                     echoes::sim::FutureWellChoice::Preserve &&
                 Controller->GetCampaignCommitStatus() ==
                     EEchoesCampaignCommitStatus::AlreadyRecorded);
    Controller->ConfirmPrimaryAction();
    TestFalse(TEXT("Enter leaves the completed mission result"),
              Controller->IsMatchResultVisible());
    TestTrue(TEXT("Enter advances to the exact next campaign briefing"),
             Controller->IsMissionBriefingVisible() &&
                 Bridge->IsScenarioPaused());
    TestTrue(TEXT("Campaign continuation selects Mission 02"),
             Bridge->GetOperationMode() ==
                 EEchoesOperationMode::CampaignSevenAccounts);
    TestTrue(TEXT("Mission 02 reconstructs its inherited initial objective"),
             Bridge->GetSevenAccountsPhase() ==
                 EEchoesSevenAccountsPhase::EstablishWaystone);
    Controller->NotifySevenAccountsFinished(
        false,
        echoes::sim::FutureWellChoice::Preserve,
        echoes::sim::FutureWellChoice::Preserve,
        EEchoesCampaignCommitStatus::NotApplicable);
    Controller->ConfirmPrimaryAction();
    TestTrue(TEXT("A failed mission retries instead of advancing"),
             !Controller->IsMatchResultVisible() &&
                 !Controller->IsMissionBriefingVisible() &&
                 Bridge->GetOperationMode() ==
                     EEchoesOperationMode::CampaignSevenAccounts &&
                 Bridge->GetSevenAccountsPhase() ==
                     EEchoesSevenAccountsPhase::EstablishWaystone &&
                 !Bridge->IsScenarioPaused());

    Controller->Destroy();
    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return true;
}

#endif
