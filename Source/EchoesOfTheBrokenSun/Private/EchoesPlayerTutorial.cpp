#include "EchoesPlayerController.h"
#include "EchoesOfTheBrokenSun.h"
#include "EchoesRTSCameraPawn.h"
#include "EchoesCinematicSubsystem.h"
#include "EchoesNarrativeSubsystem.h"
#include "Engine/GameInstance.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/GameViewportClient.h"
#include "EngineGlobals.h"
#include "Engine/World.h"
#include "UnrealClient.h"

#define LOCTEXT_NAMESPACE "EchoesTutorial"

namespace
{
// Resolve on presentation so remapping never leaves stale default key hints.
FText BoundTutorialText(const FText& Pattern)
{
    return FText::FromString(UEchoesNarrativeSubsystem::ResolveInputTokens(Pattern.ToString()));
}
}


void AEchoesPlayerController::ResetTutorialObservation()
{
    TutorialSurvey.Reset();
    TutorialSelection.Reset();
    TutorialOrders.Reset();
    TutorialActiveLessonBit = 0;
    TutorialPresentedLessonBit = 0;
    TutorialSelectionSequence = 0;
    TutorialRosterHudCandidateFrame = 0;
    TutorialRosterHudCandidateEntity = 0;
    TutorialAuthorityGeneration = 0;
    TutorialInputAttemptSequence = 0;
    TutorialLastAcceptedCommandSequence = 0;
    TutorialPendingRejectionAttempt = 0;
    bTutorialReserveMonitorInspected = false;
    TutorialAcceptedCommandSequences.Reset();
    TutorialInstruction = FText::GetEmpty();
    ++TutorialSession;
    bTutorialHasTick = false;
    bTutorialCoreSelected = false;
    bTutorialWorkerSelected = false;
    bTutorialProgressSaveFailed = false;
    TutorialCoreId = 0;
    TutorialWorkerId = 0;
    // The saved profile is the durable record of what the player skipped and
    // what they earned behind a skip; every write goes to it as it happens.
    // Re-reading it here keeps the live masks correct on every route into a
    // tutorial -- explicit start, quick load, scenario restart -- rather than
    // relying on each of those routes to remember to restore them.
    TutorialSkippedMask = PlayerProfile.TutorialSkippedMask;
    TutorialSessionVerifiedMask = PlayerProfile.TutorialSessionVerifiedMask;
}

void AEchoesPlayerController::TraceTutorialObservation(const FString& State)
{
    if (State == TutorialObservationTrace) return;
    TutorialObservationTrace = State;
    UE_LOG(LogEchoes, Display, TEXT("[ECHOES_TUTORIAL_OBSERVATION] %s"), *State);
}

void AEchoesPlayerController::ObserveTutorialSelection(uint32 ClickedEntity, bool bGroundClick)
{
    if (!bTutorialOperationAuthorized || IsModalOverlayVisible() || IsReplayInputActive()) return;
    if (ClickedEntity != 0 && ClickedEntity == TutorialCoreId)
    {
        bTutorialCoreSelected = true;
        TraceTutorialObservation(FString::Printf(TEXT("anchor_selected core=%u session=%llu"),
            ClickedEntity, static_cast<unsigned long long>(TutorialSession)));
    }
    else if (ClickedEntity != 0 && TutorialCoreId != 0)
    {
        TraceTutorialObservation(FString::Printf(TEXT("click_not_anchor entity=%u core=%u"),
            ClickedEntity, TutorialCoreId));
    }
    if ((GetTutorialProgressMask() & 1) == 0) return;
    if (ClickedEntity != 0 && ClickedEntity == TutorialWorkerId) bTutorialWorkerSelected = true;
    // Further lesson predicates are connected separately; a click never awards a mask bit.
    (void)bGroundClick;
}

bool AEchoesPlayerController::CommitTutorialLesson(uint16 Bit, const TCHAR* LessonName)
{
    const bool bPractice = TutorialPractice.IsActive();
    if (Bit == 0 || Bit > 512 || (Bit & (Bit - 1)) != 0 ||
        TutorialPresentedLessonBit != Bit)
    {
        return false;
    }
    if (bPractice)
    {
        if (TutorialPractice.TargetLessonBit() != Bit) return false;
    }
    else if ((GetTutorialProgressMask() & (Bit - 1)) != Bit - 1 ||
             (GetTutorialProgressMask() & Bit) != 0 ||
             bTutorialProgressSaveFailed)
    {
        return false;
    }
    auto* World = GetWorld();
    auto* Bridge = World ? World->GetSubsystem<UEchoesSimulationSubsystem>() : nullptr;
    if (!bTutorialOperationAuthorized || !bPlayerProfileAvailable || !Bridge || IsReplayInputActive() ||
        Bridge->GetOperationMode() != EEchoesOperationMode::TrainingReadiness) return false;
    if (bPractice)
    {
        if (!TutorialPractice.RecordVerified(Bit)) return false;
        TutorialActiveLessonBit = 0;
        TutorialPresentedLessonBit = 0;
        bTutorialOperationAuthorized = false;
        TutorialInstruction = FText::GetEmpty();
        PresentTitleScreen();
        PlayerFlow.Push(EEchoesShellScreen::Help);
        ShellMessage = FText::Format(LOCTEXT("PracticeComplete", "Practice complete: {0}."),
            FText::FromString(LessonName)).ToString();
        return true;
    }
    if ((TutorialSkippedMask & (Bit - 1)) != 0)
    {
        // Retain this genuinely observed completion separately from skips. The
        // saved profile requires an ordered prefix, so it cannot contain this bit
        // across the earlier gap. All guidance consumers share the session union.
        TutorialSessionVerifiedMask |= Bit;
        // It is durable all the same. Holding it only in memory meant a player
        // who skipped one lesson and then earned the rest lost every one of
        // them on quit and restarted at lesson one.
        const auto PriorAfterSkip = PlayerProfile;
        PlayerProfile.TutorialSessionVerifiedMask |= Bit;
        if (!CommitPlayerProfile())
        {
            PlayerProfile = PriorAfterSkip;
            TutorialSessionVerifiedMask &= static_cast<uint16>(~Bit);
            bTutorialProgressSaveFailed = true;
            TutorialInstruction = LOCTEXT("LessonSaveRetry", "Progress could not be saved. Return to the title menu and restart the readiness check to retry.");
            return false;
        }
        TutorialActiveLessonBit = 0;
        if (auto* Narrative = GetGameInstance() ? GetGameInstance()->GetSubsystem<UEchoesNarrativeSubsystem>() : nullptr)
        {
            const FString Signal = FString::Printf(TEXT("tutorial_lesson_verified:%s"), LessonName);
            Narrative->EnqueueSignal(EEchoesOperationMode::CampaignPrologue, Signal, World->GetRealTimeSeconds());
        }
        return true;
    }
    const auto Prior = PlayerProfile;
    PlayerProfile.TutorialVerifiedMask |= Bit;
    if (!CommitPlayerProfile())
    {
        PlayerProfile = Prior;
        bTutorialProgressSaveFailed = true;
        TutorialInstruction = LOCTEXT("LessonSaveRetry", "Progress could not be saved. Return to the title menu and restart the readiness check to retry.");
        return false;
    }
    TutorialActiveLessonBit = 0;
    if (auto* Narrative = GetGameInstance() ? GetGameInstance()->GetSubsystem<UEchoesNarrativeSubsystem>() : nullptr)
    {
        const FString Signal = FString::Printf(TEXT("tutorial_lesson_verified:%s"), LessonName);
        Narrative->EnqueueSignal(EEchoesOperationMode::CampaignPrologue, Signal, World->GetRealTimeSeconds());

    }
    return true;
}

void AEchoesPlayerController::ObserveTutorialSelectionEvent(
    EEchoesTutorialSelectionInputEvent Event, int32 ControlGroupIndex,
    echoes::sim::EntityType PreviousSubgroupType, echoes::sim::EntityType ActiveSubgroupType)
{
    auto* Bridge = GetWorld() ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>() : nullptr;
    if (!bTutorialOperationAuthorized || IsModalOverlayVisible() || IsReplayInputActive() || !Bridge ||
        Bridge->IsScenarioPaused() || Bridge->GetOperationMode() != EEchoesOperationMode::TrainingReadiness ||
        !TutorialSelection.IsActive()) return;
    const auto* Simulation = Bridge->GetSimulation();
    const auto View = Simulation ? Simulation->CreatePlayerView(UEchoesSimulationSubsystem::LocalPlayerId) : std::nullopt;
    if (!View) return;
    FEchoesTutorialSelectionEvent Input;
    Input.Session = TutorialSession;
    Input.Sequence = ++TutorialSelectionSequence;
    Input.Origin = EEchoesTutorialSelectionInputOrigin::PlayerInput;
    Input.Event = Event;
    Input.ControlGroupIndex = ControlGroupIndex;
    Input.PreviousSubgroupType = PreviousSubgroupType;
    Input.ActiveSubgroupType = ActiveSubgroupType;
    Input.PresentationFrame = GFrameCounter;
    const TConstArrayView<uint32> Saved = ControlGroupIndex >= 0 && ControlGroupIndex < 10
        ? TConstArrayView<uint32>(ControlGroups[ControlGroupIndex]) : TConstArrayView<uint32>();
    TutorialSelection.Observe(Input, *View, SelectedEntityIds, Saved);
}

void AEchoesPlayerController::ObserveTutorialRosterHudPublication(
    const FEchoesFieldHudView& PublishedView,
    uint64 PresentationFrame)
{
    auto* Bridge = GetWorld() ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>() : nullptr;
    if (!bTutorialOperationAuthorized || IsModalOverlayVisible() || IsReplayInputActive() ||
        !Bridge || Bridge->IsScenarioPaused() ||
        Bridge->GetOperationMode() != EEchoesOperationMode::TrainingReadiness ||
        TutorialSelection.ActiveStage() != EEchoesTutorialSelectionStage::Roster)
    {
        return;
    }
    const auto* Simulation = Bridge->GetSimulation();
    const auto View = Simulation
        ? Simulation->CreatePlayerView(UEchoesSimulationSubsystem::LocalPlayerId)
        : std::nullopt;
    if (!View) return;
    FEchoesTutorialSelectionEvent Input;
    Input.Session = TutorialSession;
    Input.Sequence = ++TutorialSelectionSequence;
    Input.Origin = EEchoesTutorialSelectionInputOrigin::PlayerInput;
    Input.Event = EEchoesTutorialSelectionInputEvent::RosterHudPublished;
    Input.PresentationFrame = PresentationFrame;
    TutorialSelection.ObserveRosterHudPublication(
        Input, *View, SelectedEntityIds, PublishedView);
}

void AEchoesPlayerController::ObserveTutorialAcceptedCommand(uint64 Sequence, EEchoesTutorialOrderCommandOrigin Origin)
{
    if (!bTutorialOperationAuthorized || IsModalOverlayVisible() || IsReplayInputActive() || !TutorialOrders.IsActive()) return;
    TutorialAcceptedCommandSequences.Emplace(Sequence, Origin);
}

uint64 AEchoesPlayerController::ObserveTutorialRejectedCommandAttempt(const FEchoesTutorialExpectedCommand& Attempt)
{
    auto* Bridge = GetWorld() ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>() : nullptr;
    if (!bTutorialOperationAuthorized || IsModalOverlayVisible() || IsReplayInputActive() || !Bridge ||
        Bridge->IsScenarioPaused() || Bridge->GetOperationMode() != EEchoesOperationMode::TrainingReadiness ||
        !Bridge->GetSimulation()) return 0;
    const uint64 Input = ++TutorialInputAttemptSequence;
    if (!TutorialOrders.ObserveRejectedAttempt(TutorialSession, Bridge->GetScenarioAuthorityGeneration(),
            *Bridge->GetSimulation(), Input, Attempt)) return 0;
    TutorialPendingRejectionAttempt = Input;
    return Input;
}

void AEchoesPlayerController::ObserveTutorialRejectionAcknowledged(uint64 InputAttemptSequence)
{
    auto* Bridge = GetWorld() ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>() : nullptr;
    if (!Bridge || !Bridge->GetSimulation() || InputAttemptSequence == 0 ||
        InputAttemptSequence != TutorialPendingRejectionAttempt || IsReplayInputActive() ||
        !bTutorialOperationAuthorized || Bridge->IsScenarioPaused()) return;
    TutorialOrders.ObserveRejectionAcknowledged(TutorialSession, Bridge->GetScenarioAuthorityGeneration(),
        *Bridge->GetSimulation(), InputAttemptSequence);
    if (TutorialOrders.RouteProgress().bRejectionAcknowledged || !TutorialOrders.IsActive())
        TutorialPendingRejectionAttempt = 0;
}

void AEchoesPlayerController::TickTutorialObservation()
{
    if (bM01OpeningPending) return;
    auto* World = GetWorld();
    auto* Bridge = World ? World->GetSubsystem<UEchoesSimulationSubsystem>() : nullptr;
    auto* Camera = Cast<AEchoesRTSCameraPawn>(GetPawn());
    if (!bTutorialOperationAuthorized || !bPlayerProfileAvailable || !Bridge || !Camera ||
        Bridge->GetOperationMode() != EEchoesOperationMode::TrainingReadiness || IsReplayInputActive())
    {
        TutorialInstruction = FText::GetEmpty();
        return;
    }
    if (IsModalOverlayVisible() || Bridge->IsScenarioPaused() || bTutorialProgressSaveFailed)
    {
        TraceTutorialObservation(FString::Printf(TEXT("held modal=%d paused=%d saveFailed=%d"),
            IsModalOverlayVisible(), Bridge->IsScenarioPaused(), bTutorialProgressSaveFailed));
        return;
    }
    // A background window withholds camera credit only. Instruction delivery
    // and the Anchor-selection gate stay live, otherwise a pointer click the
    // controller already recorded leaves the HUD frozen on a stale demand.
    const auto* Viewport = World->GetGameViewport();
    const bool bForegroundWindow =
        !(Viewport && Viewport->Viewport && !Viewport->Viewport->IsForegroundWindow());
    const auto* Simulation = Bridge->GetSimulation();
    if (!Simulation) return;
    const auto Player = Simulation->CreatePlayerView(UEchoesSimulationSubsystem::LocalPlayerId);
    if (!Player) return;
    const uint64 AuthorityGeneration = Bridge->GetScenarioAuthorityGeneration();
    if (TutorialAuthorityGeneration != AuthorityGeneration)
    {
        ResetTutorialObservation();
        TutorialAuthorityGeneration = AuthorityGeneration;
    }
    const uint64 Tick = Player->CurrentTick();
    if (bTutorialHasTick && Tick < TutorialLastTick) ResetTutorialObservation();
    if (bTutorialHasTick && Tick == TutorialLastTick) return;
    TutorialLastTick = Tick;
    bTutorialHasTick = true;
    FVector2D Center;
    if (!Camera->GetNavigationCenter(Center)) return;
    if (!TutorialSurvey.IsActive())
    {
        const echoes::sim::Entity* Core = nullptr;
        for (const auto& Entity : Player->Entities())
        {
            if (Entity.owner != UEchoesSimulationSubsystem::LocalPlayerId) continue;
            if (Entity.type == echoes::sim::EntityType::CommandCore) Core = &Entity;
            if (!TutorialWorkerId && Entity.type == echoes::sim::EntityType::Worker) TutorialWorkerId = Entity.id;
        }
        if (!Core) return;
        // The camera observer restarts after teleports, cinematics or restores.
        // The Anchor-selection gate is separate evidence: keep it while the same
        // owned Core remains bound, so a camera restart never re-demands a click
        // the player already made. Authority changes and rollbacks still clear
        // it through ResetTutorialObservation.
        const bool bSameCore = TutorialCoreId == Core->id;
        TutorialCoreId = Core->id;
        FEchoesTutorialSurveySetup Setup;
        Setup.InitialCenter = Center;
        Setup.InitialZoom = Camera->GetNavigationZoom();
        Setup.MinimumZoom = Camera->GetMinimumNavigationZoom();
        Setup.MaximumZoom = Camera->GetMaximumNavigationZoom();
        // Camera tolerances use the same 200 cm acceptance radius as the designated sites.
        Setup.RequiredPanDistance = UEchoesSimulationSubsystem::TileWorldSize * 2;
        Setup.RecenterTolerance = 200;
        for (const auto Site : {Core->position, Bridge->GetArchiveRecoverySite(), Bridge->GetEvacuationSite()})
        {
            const FVector Position = Bridge->SimToWorld(Site);
            Setup.Waypoints.Add(FVector2D(Position.X, Position.Y));
        }
        ++TutorialSession;
        if (!bSameCore) bTutorialCoreSelected = false;
        if (!TutorialSurvey.Begin(TutorialSession, Setup))
        {
            TraceTutorialObservation(FString::Printf(TEXT("survey_begin_rejected core=%u zoom=%.0f"),
                TutorialCoreId, Setup.InitialZoom));
            return;
        }
        TutorialInitialNavigationRevision = Camera->GetNavigationRevision();
        TraceTutorialObservation(FString::Printf(
            TEXT("survey_begin core=%u session=%llu restart=%d anchorSelected=%d tick=%llu"),
            TutorialCoreId, static_cast<unsigned long long>(TutorialSession), bSameCore,
            bTutorialCoreSelected, static_cast<unsigned long long>(Tick)));

    }
    const uint16 PracticeTarget = TutorialPractice.TargetLessonBit();
    const uint16 ProgressMask = PracticeTarget != 0
        ? static_cast<uint16>(FEchoesTutorialPracticeState::ImplementedLessonMask &
            ~PracticeTarget)
        : GetTutorialProgressMask();
    static const TCHAR* LessonNames[] = {
        TEXT("survey"), TEXT("roster"), TEXT("muster"),
        TEXT("route"), TEXT("reserve")};
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(LessonNames); ++Index)
    {
        const uint16 Bit = 1u << Index;
        if (PracticeTarget != 0 ? Bit != PracticeTarget
                                : (ProgressMask & Bit) != 0) continue;
        if (TutorialPresentedLessonBit != Bit)
        {
            if (auto* Narrative = GetGameInstance() ? GetGameInstance()->GetSubsystem<UEchoesNarrativeSubsystem>() : nullptr)
                Narrative->EnqueueSignal(EEchoesOperationMode::CampaignPrologue,
                    FString::Printf(TEXT("tutorial_lesson_opened:%s"), LessonNames[Index]), World->GetRealTimeSeconds());
            TutorialPresentedLessonBit = Bit;
        }
        break;
    }
    if ((ProgressMask & 1) == 0)
    {
        const int32 Waypoint = TutorialSurvey.CompletedWaypoints();
        if (!bTutorialCoreSelected)
        {
            TutorialInstruction = BoundTutorialText(LOCTEXT("SurveySelectAnchor", "Survey: use {select_key} on your Anchor to begin."));
        }
        else if (!TutorialSurvey.HasPanned())
        {
            TutorialInstruction = BoundTutorialText(LOCTEXT("SurveyPan", "Survey: pan the camera across the map using {pan_keys}."));
        }
        else if (!(TutorialSurvey.HasZoomedMin() && TutorialSurvey.HasZoomedMax()))
        {
            TutorialInstruction = BoundTutorialText(LOCTEXT("SurveyZoom", "Survey: use {zoom_in_key} and {zoom_out_key} to zoom fully in and out."));
        }
        else if (!TutorialSurvey.HasRecentered() || Waypoint == 0)
        {
            TutorialInstruction = BoundTutorialText(LOCTEXT("SurveyRecenter", "Survey: use {recenter_key} and keep the camera centered over it for 1.5 seconds."));
        }
        else if (Waypoint == 1)
        {
            TutorialInstruction = LOCTEXT("SurveySite1",
                "Anchor verified. Next, locate the Archive Recovery Site marked on your minimap and keep the camera centered over it for 1.5 seconds.");
        }
        else if (Waypoint == 2)
        {
            TutorialInstruction = LOCTEXT("SurveySite2",
                "Archive Recovery Site verified. Next, find the Evacuation Site marked on your minimap and keep the camera centered over it for 1.5 seconds.");
        }
        else
        {
            TutorialInstruction = LOCTEXT("SurveySiteComplete",
                "All survey sites verified: Anchor, Archive Recovery Site, and Evacuation Site. Select your Anchor to complete the survey.");
        }
        if (!bForegroundWindow)
        {
            TraceTutorialObservation(FString::Printf(
                TEXT("camera_credit_held background_window=1 anchorSelected=%d"), bTutorialCoreSelected));
        }
        else if (Camera->GetNavigationRevision() != TutorialInitialNavigationRevision)
        {
            FEchoesTutorialSurveySample Sample;
            Sample.Session = TutorialSession;
            Sample.Tick = Tick;
            Sample.Center = Center;
            Sample.Zoom = Camera->GetNavigationZoom();
            Sample.Source = Camera->WasLastNavigationPlayerDriven()
                ? EEchoesSurveyCameraSource::PlayerNavigation : EEchoesSurveyCameraSource::Programmatic;
            TutorialSurvey.Observe(Sample);
            if (!TutorialSurvey.IsActive())
            {
                TraceTutorialObservation(FString::Printf(
                    TEXT("survey_reset source=%s zoom=%.0f tick=%llu anchorSelected=%d"),
                    Sample.Source == EEchoesSurveyCameraSource::PlayerNavigation ? TEXT("player") : TEXT("programmatic"),
                    Sample.Zoom, static_cast<unsigned long long>(Tick), bTutorialCoreSelected));
            }
        }
        if (TutorialSurvey.CameraPredicateSatisfied() && bTutorialCoreSelected &&
            SelectedEntityIds.Num() == 1 && SelectedEntityIds[0] == TutorialCoreId)
        {
            CommitTutorialLesson(1, TEXT("survey"));
        }
    }
    else if ((ProgressMask & 2) == 0)
    {
        if (TutorialActiveLessonBit != 2)
        {
            TutorialSelection.Reset();
            if (!TutorialSelection.BeginRoster(TutorialSession, *Player, TutorialWorkerId))
            {
                TutorialInstruction = LOCTEXT("RosterUnavailable", "The staged Surveyor is unavailable. Restart the readiness check from the title menu.");
                return;
            }
            TutorialActiveLessonBit = 2;
        }
        const auto RosterProgress = TutorialSelection.RosterProgress();
        TutorialInstruction = !RosterProgress.bSingleClickSelected
            ? LOCTEXT("RosterSelect", "Roster: select the first Surveyor beside the Anchor. Read its purpose, health, order and command card.")
            : !RosterProgress.bHudPublished
                ? LOCTEXT("RosterPublished", "Roster: keep the Surveyor selected while its purpose, health, current order, and command controls appear in the field HUD.")
                : LOCTEXT("RosterClear", "Roster: the field HUD has presented the Surveyor's role and controls. Click clear terrain to release it.");
        if (TutorialSelection.RosterProgress().PredicateSatisfied()) CommitTutorialLesson(2, TEXT("roster"));
    }
    else if ((ProgressMask & 4) == 0)
    {
        if (TutorialActiveLessonBit != 4)
        {
            TArray<echoes::sim::EntityId> Staged;
            // The authored opening section consists of the six Surveyors and two Lancers.
            for (const auto& Entity : Player->Entities())
                if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
                    (Entity.type == echoes::sim::EntityType::Worker || Entity.type == echoes::sim::EntityType::Soldier))
                    Staged.Add(Entity.id);
            TutorialSelection.Reset();
            if (!TutorialSelection.BeginMuster(TutorialSession, *Player, Staged))
            {
                TutorialInstruction = LOCTEXT("MusterUnavailable", "The opening section is unavailable. Restart the readiness check from the title menu.");
                return;
            }
            TutorialActiveLessonBit = 4;
        }
        const auto Progress = TutorialSelection.MusterProgress();
        if (!Progress.bDragSelected)
            TutorialInstruction = LOCTEXT("MusterDrag", "Muster: drag a box around your six Surveyors and two Lancers.");
        else if (!Progress.bSelectionModified)
            TutorialInstruction = LOCTEXT("MusterModify", "Muster: use Shift with selection to change the section, then restore the whole section.");
        else if (!Progress.bSubgroupChanged)
            TutorialInstruction = LOCTEXT("MusterSubgroup", "Muster: with the mixed section selected, cycle its active subgroup.");
        else if (!Progress.bControlGroupAssigned)
            TutorialInstruction = LOCTEXT("MusterAssign", "Muster: assign the whole section to a control group.");
        else
            TutorialInstruction = LOCTEXT("MusterRecall", "Muster: recall the control group you assigned.");
        if (Progress.PredicateSatisfied()) CommitTutorialLesson(4, TEXT("muster"));
    }
    else if ((ProgressMask & 24) != 24)
    {
        const bool bReserve = (ProgressMask & 8) != 0;
        const uint16 Bit = bReserve ? 16 : 8;
        if (TutorialActiveLessonBit != Bit)
        {
            uint32 Node = 0, Soldier = 0, Guard = 0;
            for (const auto& Entity : Player->Entities())
            {
                if (Entity.type == echoes::sim::EntityType::ResourceNode &&
                    Entity.position == echoes::sim::Vec2::FromTiles(16, 16)) Node = Entity.id;
                if (Entity.owner != UEchoesSimulationSubsystem::LocalPlayerId) continue;
                if (!Soldier && Entity.type == echoes::sim::EntityType::Soldier) Soldier = Entity.id;
                if (!Guard && Entity.type == echoes::sim::EntityType::HeavyUnit) Guard = Entity.id;
            }
            TutorialOrders.Reset();
            TutorialAcceptedCommandSequences.Reset();
            bool bOpened = false;
            if (bReserve)
            {
                FEchoesTutorialReserveSetup Setup;
                Setup.LocalPlayer = UEchoesSimulationSubsystem::LocalPlayerId;
                Setup.Worker = TutorialWorkerId;
                Setup.MatterNode = Node;
                bOpened = TutorialOrders.BeginReserve(TutorialSession, AuthorityGeneration, *Simulation, Setup);
            }
            else
            {
                FEchoesTutorialRouteSetup Setup;
                Setup.LocalPlayer = UEchoesSimulationSubsystem::LocalPlayerId;
                Setup.FirstInputAttemptSequence = TutorialInputAttemptSequence + 1;
                Setup.MoveActor = Setup.StopActor = TutorialWorkerId;
                Setup.MoveDestination = echoes::sim::Vec2::FromTiles(14, 18);
                Setup.MoveArrivalToleranceRaw = echoes::sim::kFixedScale / 2;
                Setup.MoveInputToleranceRaw = echoes::sim::kFixedScale / 2;
                Setup.PatrolInputToleranceRaw = echoes::sim::kFixedScale / 2;
                Setup.RejectedInputToleranceRaw = echoes::sim::kFixedScale / 2;
                Setup.PatrolActor = Soldier;
                Setup.PatrolDestination = echoes::sim::Vec2::FromTiles(14, 20);
                Setup.GuardActor = Guard;
                Setup.GuardTarget = TutorialWorkerId;
                Setup.ContextAction.Type = echoes::sim::CommandType::Gather;
                Setup.ContextAction.Actor = TutorialWorkerId;
                Setup.ContextAction.Target = Node;
                Setup.RejectedAction.Type = echoes::sim::CommandType::Move;
                Setup.RejectedAction.Actor = TutorialWorkerId;
                Setup.RejectedAction.Position = echoes::sim::Vec2::FromTiles(19, 10);
                bOpened = TutorialOrders.BeginRoute(TutorialSession, AuthorityGeneration, *Simulation, Setup);
            }
            if (!bOpened)
            {
                TutorialInstruction = LOCTEXT("OrderStageUnavailable", "The readiness staging is unavailable. Restart the readiness check from the title menu.");
                return;
            }
            TutorialActiveLessonBit = Bit;
        }
        TutorialOrders.ObserveState(TutorialSession, AuthorityGeneration, *Simulation);
        for (int32 Index = TutorialAcceptedCommandSequences.Num() - 1; Index >= 0; --Index)
        {
            const auto& Command = TutorialAcceptedCommandSequences[Index];
            if (Simulation->FindCommandResolutionReceipt(UEchoesSimulationSubsystem::LocalPlayerId, Command.Key))
            {
                TutorialOrders.ObserveAcceptedCommand(TutorialSession, AuthorityGeneration, *Simulation, Command.Key, Command.Value);
                TutorialAcceptedCommandSequences.RemoveAt(Index);
            }
        }
        if (!TutorialOrders.IsActive())
        {
            TutorialActiveLessonBit = 0;
            TutorialInstruction = LOCTEXT("OrderStageReset", "The readiness observation was interrupted. Repeat the current check.");
            return;
        }
        if (!bReserve)
        {
            const auto Progress = TutorialOrders.RouteProgress();
            if (!Progress.bMoveArrived)
                TutorialInstruction = LOCTEXT("RouteMove", "Route: send the first Surveyor to R on the minimap. Let it arrive.");
            else if (!Progress.bContextActionCompleted)
                TutorialInstruction = LOCTEXT("RouteContext", "Route: right-click the Matter seam beside R with that Surveyor selected.");
            else if (!Progress.bStopCompleted)
                TutorialInstruction = LOCTEXT("RouteStop", "Route: stop the Surveyor while its order is active.");
            else if (!Progress.bPatrolCompleted)
                TutorialInstruction = LOCTEXT("RoutePatrol", "Route: give the first Lancer a Patrol order to P.");
            else if (!Progress.bGuardCompleted)
                TutorialInstruction = LOCTEXT("RouteGuard", "Route: order the Bulwark to Guard the first Surveyor.");
            else
                TutorialInstruction = LOCTEXT("RouteRejection", "Route: try moving the first Surveyor onto the blocked outcrop marked X. Read and acknowledge the rejection.");
            if (Progress.PredicateSatisfied()) CommitTutorialLesson(8, TEXT("route"));
        }
        else
        {
            TutorialInstruction = FText::Format(LOCTEXT("ReserveDelivery", "Reserve: send the first Surveyor back to the Matter seam. Keep its route running. Delivered: {0}/200. Inspect the reserve monitor."),
                FText::AsNumber(TutorialOrders.DeliveredMatterObserved()));
            if (TutorialOrders.ReserveSimulationPredicateSatisfied() && bTutorialReserveMonitorInspected)
                CommitTutorialLesson(16, TEXT("reserve"));
        }
    }
    else
    {
        TutorialActiveLessonBit = 0;
        TutorialInstruction = LOCTEXT("LaterLessonsUnavailable",
            "The first five readiness lessons are complete. Further lessons are not available yet.");
    }
}
void AEchoesPlayerController::TickM01Opening()
{
    if (!bM01OpeningPending || !GetWorld()) return;
    auto* Cinematic = GetWorld()->GetSubsystem<UEchoesCinematicSubsystem>();
    if (!Cinematic) return;
    TutorialInstruction = Cinematic->IsSequencePaused()
        ? LOCTEXT("OpeningPaused", "Opening paused · Pause to resume · Escape or hold Space for 1 second to skip")
        : LOCTEXT("OpeningSkip", "Pause to pause · Escape or hold Space for 1 second to skip");
    const auto* Viewport = GetWorld()->GetGameViewport();
    if (Viewport && Viewport->Viewport && !Viewport->Viewport->IsForegroundWindow())
    {
        OpeningSpacePressedAt = -1.0;
        Cinematic->SetSequencePaused(true);
    }
    if (OpeningSpacePressedAt >= 0.0 && GetWorld()->GetRealTimeSeconds() - OpeningSpacePressedAt >= 1.0)
    {
        OpeningSpacePressedAt = -1.0;
        Cinematic->SkipActiveSequence();
    }
    if (!Cinematic->IsSequenceActive() && !Cinematic->HasSequenceCompleted(EEchoesCinematicSequence::M01Opening))
    {
        bM01OpeningPending = false;
        OpeningSpacePressedAt = -1.0;
        PlayerFlow.SetVisible(EEchoesShellScreen::Briefing, true);
        TutorialInstruction = FText::GetEmpty();
        SetStatusMessage(LOCTEXT("OpeningInterrupted", "The opening was interrupted. Deployment remains paused; try Deploy again.").ToString(), 15.0f);
        return;
    }
    if (Cinematic->HasSequenceCompleted(EEchoesCinematicSequence::M01Opening))
    {
        bM01OpeningPending = false;
        OpeningSpacePressedAt = -1.0;
        ResetTutorialObservation();
        FinishMissionDeployment();
    }
}

void AEchoesPlayerController::OpenTutorialSkipModal()
{
    if (TutorialSkipModal.bVisible || !bTutorialOperationAuthorized) return;
    auto* World = GetWorld();
    auto* Bridge = World ? World->GetSubsystem<UEchoesSimulationSubsystem>() : nullptr;
    TutorialSkipModal.bVisible = true;
    TutorialSkipModal.bScenarioWasPaused = Bridge ? Bridge->IsScenarioPaused() : false;
    if (Bridge && Bridge->IsScenarioReady())
    {
        Bridge->SetScenarioPaused(true);
        SetNarrativePlaybackPausedOutsideCinematic(true);
    }
    SetIgnoreMoveInput(true);
    SetIgnoreLookInput(true);
    RefreshFieldHud();
}

void AEchoesPlayerController::CloseTutorialSkipModal(bool bRestorePause)
{
    if (!TutorialSkipModal.bVisible) return;
    const bool bWasPaused = TutorialSkipModal.bScenarioWasPaused;
    TutorialSkipModal.bVisible = false;
    TutorialSkipModal.bScenarioWasPaused = false;
    auto* World = GetWorld();
    auto* Bridge = World ? World->GetSubsystem<UEchoesSimulationSubsystem>() : nullptr;
    if (bRestorePause && Bridge && Bridge->IsScenarioReady())
    {
        Bridge->SetScenarioPaused(bWasPaused);
        SetNarrativePlaybackPausedOutsideCinematic(bWasPaused);
    }
    else if (!bRestorePause && Bridge && Bridge->IsScenarioReady())
    {
        Bridge->SetScenarioPaused(false);
        SetNarrativePlaybackPausedOutsideCinematic(false);
    }
    const bool bKeepInputHeld = IsModalOverlayVisible();
    SetIgnoreMoveInput(bKeepInputHeld);
    SetIgnoreLookInput(bKeepInputHeld);
    RefreshFieldHud();
}

void AEchoesPlayerController::SkipTutorialCurrentStep()
{
    if (!bTutorialOperationAuthorized)
    {
        CloseTutorialSkipModal(false);
        return;
    }
    const uint16 CurrentBit = TutorialPresentedLessonBit != 0
        ? TutorialPresentedLessonBit
        : (TutorialActiveLessonBit != 0 ? TutorialActiveLessonBit : 1);
    TutorialSkippedMask |= CurrentBit;
    // A skip is a durable fact about this player's onboarding, not a per-run
    // one: without it a relaunch re-demands the step they deliberately passed.
    const auto PriorSkipProfile = PlayerProfile;
    PlayerProfile.TutorialSkippedMask |= CurrentBit;
    const bool bSkipRecorded = CommitPlayerProfile();
    if (!bSkipRecorded)
    {
        // The step is still skipped for this run; only the record failed. Say
        // so rather than silently promising it will be remembered.
        PlayerProfile = PriorSkipProfile;
    }
    CloseTutorialSkipModal(false);
    TutorialActiveLessonBit = 0;
    TutorialPresentedLessonBit = 0;
    SetStatusMessage(
        (bSkipRecorded
            ? LOCTEXT("StepSkipped", "Step skipped. Progress recorded as skipped (no mastery awarded).")
            : LOCTEXT("StepSkipNotRecorded", "Step skipped for this session. It could not be saved, so it will be offered again next time."))
            .ToString(),
        8.0f);
    RefreshFieldHud();
}

void AEchoesPlayerController::EndAllTutorials()
{
    // The existing profile bit is a durable preference, never mastery. Commit
    // before unfreezing so failed storage does not promise a choice we lost.
    if (!InitializePlayerProfile())
    {
        SetStatusMessage(ShellMessage, 8.0f);
        return;
    }
    const FEchoesPlayerProfile PriorProfile = PlayerProfile;
    PlayerProfile.bOnboardingOffered = true;
    PlayerProfile.bTutorialOptOut = true;
    if (!CommitPlayerProfile())
    {
        PlayerProfile = PriorProfile;
        SetStatusMessage(ShellMessage, 8.0f);
        return;
    }
    CloseTutorialSkipModal(false);
    bTutorialOperationAuthorized = false;
    TutorialActiveLessonBit = 0;
    TutorialPresentedLessonBit = 0;
    TutorialInstruction = FText::GetEmpty();
    ResetTutorialObservation();
    SetStatusMessage(LOCTEXT("TutorialEnded", "Tutorial ended. Standard controls restored.").ToString(), 8.0f);
    RefreshFieldHud();
}

void AEchoesPlayerController::CancelTutorialSkipModal()
{
    CloseTutorialSkipModal(true);
}

#undef LOCTEXT_NAMESPACE
