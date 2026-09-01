#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "EchoesPlayerController.h"
#include "EchoesPrologueMissionModel.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Tests/AutomationCommon.h"

namespace
{
struct FPreservedPrologueCampaignFile final
{
    explicit FPreservedPrologueCampaignFile(FString InPath)
        : Path(MoveTemp(InPath))
    {
        bExisted = IFileManager::Get().FileExists(*Path);
        if (bExisted)
        {
            FFileHelper::LoadFileToArray(Contents, *Path);
        }
    }

    ~FPreservedPrologueCampaignFile()
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
    FPreservedPrologueCampaignFile PreservedBackup(
        CampaignPath + TEXT(".bak"));
    FPreservedPrologueCampaignFile PreservedTemporary(
        CampaignPath + TEXT(".tmp"));
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
    if (Simulation != nullptr)
    {
        for (const echoes::sim::Entity& Entity : Simulation->Entities())
        {
            if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
                Entity.type == echoes::sim::EntityType::Worker && WorkerId == 0)
            {
                WorkerId = Entity.id;
            }
            if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
                Entity.type == echoes::sim::EntityType::Barracks &&
                BarracksId == 0)
            {
                BarracksId = Entity.id;
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
    Feedback.Reset();
    TestTrue(
        TEXT("Ordinary Soldier production queues before the Well decision"),
        Bridge->IssueProductionCommand(
            BarracksId,
            echoes::sim::EntityType::Soldier,
            Feedback));
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
