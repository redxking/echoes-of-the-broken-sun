#include "EchoesPlayerController.h"
#include "EchoesTutorialCurriculumModel.h"
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
    TutorialConstruction.Reset();
    TutorialConstructionHudCandidateEntity = 0;
    TutorialConstructionHudCandidateFrame = 0;
    TutorialProductionLessonBit = 0;
    TutorialObservedProductionEntity = 0;
    TutorialProductionSequence = 0;
    TutorialFoundryMaxKnownEntityId = 0;
    bTutorialFoundryEmerged = false;
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
        Bridge->IsScenarioPaused() || Bridge->GetOperationMode() != EEchoesOperationMode::TrainingReadiness) return;
    if (TutorialConstruction.IsActive() && Event == EEchoesTutorialSelectionInputEvent::SingleClickSelection &&
        SelectedEntityIds.Num() == 1 && Bridge->GetSimulation() != nullptr)
    {
        // Lesson six: the player selects the finished Link they built; the
        // HUD's description of it is proven on a later frame.
        FEchoesTutorialConstructionInput Input;
        Input.Session = TutorialSession;
        Input.AuthorityGeneration = Bridge->GetScenarioAuthorityGeneration();
        Input.InputSequence = ++TutorialInputAttemptSequence;
        Input.Origin = EEchoesTutorialConstructionOrigin::PlayerInput;
        Input.PresentationFrame = GFrameCounter;
        if (TutorialConstruction.ObserveSelection(Input, *Bridge->GetSimulation(), SelectedEntityIds[0]))
        {
            TutorialConstructionHudCandidateEntity = SelectedEntityIds[0];
            TutorialConstructionHudCandidateFrame = Input.PresentationFrame;
        }
        return;
    }
    if (!TutorialSelection.IsActive()) return;
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
    if (!bTutorialOperationAuthorized || IsModalOverlayVisible() || IsReplayInputActive() ||
        !(TutorialOrders.IsActive() || TutorialConstruction.IsActive() || TutorialProductionLessonBit != 0)) return;
    TutorialAcceptedCommandSequences.Emplace(Sequence, Origin);
}

void AEchoesPlayerController::ObserveTutorialPlacementRejected(
    echoes::sim::EntityType StructureType,
    echoes::sim::Vec2 Site)
{
    auto* Bridge = GetWorld() ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>() : nullptr;
    if (!bTutorialOperationAuthorized || IsModalOverlayVisible() || IsReplayInputActive() || !Bridge ||
        Bridge->IsScenarioPaused() || Bridge->GetOperationMode() != EEchoesOperationMode::TrainingReadiness ||
        !Bridge->GetSimulation() || !TutorialConstruction.IsActive() ||
        StructureType != echoes::sim::EntityType::Dropoff) return;
    FEchoesTutorialConstructionInput Input;
    Input.Session = TutorialSession;
    Input.AuthorityGeneration = Bridge->GetScenarioAuthorityGeneration();
    Input.InputSequence = ++TutorialInputAttemptSequence;
    Input.Origin = EEchoesTutorialConstructionOrigin::PlayerInput;
    Input.PresentationFrame = GFrameCounter;
    // The observer accepts only a site the simulation itself refuses.
    if (!TutorialConstruction.ObserveRejectedPlacement(Input, *Bridge->GetSimulation(), Site)) return;
    TutorialPendingRejectionAttempt = Input.InputSequence;
    if (auto* Narrative = GetGameInstance() ? GetGameInstance()->GetSubsystem<UEchoesNarrativeSubsystem>() : nullptr)
        Narrative->EnqueueSignal(EEchoesOperationMode::CampaignPrologue,
            TEXT("tutorial_placement_rejected:link"), GetWorld()->GetRealTimeSeconds());
    TraceTutorialObservation(FString::Printf(TEXT("link_placement_rejected input=%llu tile=%d,%d"),
        static_cast<unsigned long long>(Input.InputSequence), Site.x.FloorToInt(), Site.y.FloorToInt()));
}

void AEchoesPlayerController::ObserveTutorialConstructionHudPublication(
    const FEchoesFieldHudView& PublishedView,
    uint64 PresentationFrame)
{
    auto* Bridge = GetWorld() ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>() : nullptr;
    if (!bTutorialOperationAuthorized || IsModalOverlayVisible() || IsReplayInputActive() || !Bridge ||
        Bridge->IsScenarioPaused() || Bridge->GetOperationMode() != EEchoesOperationMode::TrainingReadiness ||
        !Bridge->GetSimulation() || !TutorialConstruction.IsActive()) return;
    if (TutorialConstruction.ObserveHudPublication(TutorialSession, Bridge->GetScenarioAuthorityGeneration(),
            PresentationFrame, *Bridge->GetSimulation(), SelectedEntityIds, PublishedView))
    {
        TutorialConstructionHudCandidateEntity = 0;
        TutorialConstructionHudCandidateFrame = 0;
    }
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
    if (TutorialConstruction.IsActive())
    {
        FEchoesTutorialConstructionInput Input;
        Input.Session = TutorialSession;
        Input.AuthorityGeneration = Bridge->GetScenarioAuthorityGeneration();
        Input.InputSequence = ++TutorialInputAttemptSequence;
        Input.Origin = EEchoesTutorialConstructionOrigin::PlayerInput;
        Input.PresentationFrame = GFrameCounter;
        if (TutorialConstruction.ObserveRejectionAcknowledged(Input, *Bridge->GetSimulation(), InputAttemptSequence) ||
            !TutorialConstruction.IsActive())
            TutorialPendingRejectionAttempt = 0;
        return;
    }
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
    // The lesson key comes from the curriculum model, which returns the demo
    // narrative contract's keys verbatim. A second hardcoded list here was a
    // list that could drift from the contract silently: a lesson renamed in one
    // place and not the other emits a signal no authored trigger listens for,
    // and nothing fails until a player reaches that lesson and hears nothing.
    for (int32 Index = 0; Index < EchoesTutorialLessonCount; ++Index)
    {
        const uint16 Bit = static_cast<uint16>(1u << Index);
        if (PracticeTarget != 0 ? Bit != PracticeTarget
                                : (ProgressMask & Bit) != 0) continue;
        if (TutorialPresentedLessonBit != Bit)
        {
            if (auto* Narrative = GetGameInstance() ? GetGameInstance()->GetSubsystem<UEchoesNarrativeSubsystem>() : nullptr)
                Narrative->EnqueueSignal(EEchoesOperationMode::CampaignPrologue,
                    FString::Printf(TEXT("tutorial_lesson_opened:%s"),
                        FEchoesTutorialCurriculumModel::StableName(
                            static_cast<EEchoesTutorialLesson>(Index))),
                    World->GetRealTimeSeconds());
            TutorialPresentedLessonBit = Bit;
        }
        break;
    }
    if ((ProgressMask & 1) == 0)
    {
        // SPEC-TUT-008 retires the mandatory centering, camera-distance and
        // waypoint-dwell exercises. A new player's first act was a camera
        // calibration chore -- pan a set distance, zoom to both limits,
        // recentre, then hold the view over three map sites for 1.5 seconds
        // each -- before a single RTS idea had been introduced. Camera help
        // stays, as help: offered alongside the real instruction while the
        // player has not moved the camera yet, and never gating progress.
        if (!bTutorialCoreSelected)
        {
            TutorialInstruction = BoundTutorialText(TutorialSurvey.HasPanned()
                ? LOCTEXT("SurveySelectAnchor", "Survey: use {select_key} on your Anchor to begin.")
                : LOCTEXT("SurveySelectAnchorWithCamera", "Survey: use {select_key} on your Anchor to begin. You can look around with {pan_keys} at any time."));
        }
        else if (SelectedEntityIds.Num() == 1 &&
            SelectedEntityIds[0] == TutorialCoreId)
        {
            TutorialInstruction = LOCTEXT("SurveyAnchorRead",
                "Anchor selected. Its readout names what it does, what it cannot do, and what it costs you to lose.");
        }
        else
        {
            // The Anchor was clicked at some point, but it is not what is
            // selected now -- a box that caught other units, or a later click
            // elsewhere. Without this the lesson would sit silently unadvanced
            // with no way for the player to tell what it was still waiting for.
            TutorialInstruction = BoundTutorialText(LOCTEXT("SurveyAnchorAlone",
                "Survey: select your Anchor on its own with {select_key}, so its readout is the one on screen."));
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
        // The lesson is the selection itself: the player found their Anchor and
        // read it. The camera predicate is no longer part of the gate.
        if (bTutorialCoreSelected && SelectedEntityIds.Num() == 1 &&
            SelectedEntityIds[0] == TutorialCoreId)
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
    else if ((ProgressMask & 32) == 0)
    {
        // Lesson six — Link (SPEC-TUT-008 chapter 2, "Build a useful
        // outpost"): a refused placement read and acknowledged, a Power Link
        // placed at the marked footprint and finished with two Surveyors on
        // it, the authored damaged Link repaired to full, and the finished
        // Link selected and described by the field HUD.
        if (TutorialActiveLessonBit != 32)
        {
            uint32 Builder = 0, Assistant = 0;
            for (const auto& Entity : Player->Entities())
            {
                if (Entity.owner != UEchoesSimulationSubsystem::LocalPlayerId ||
                    Entity.type != echoes::sim::EntityType::Worker || Entity.hitPoints <= 0) continue;
                if (Entity.id == TutorialWorkerId) { Builder = Entity.id; continue; }
                if (Assistant == 0) Assistant = Entity.id;
            }
            if (Builder == 0) { Builder = Assistant; Assistant = 0; }
            if (Assistant == 0)
            {
                for (const auto& Entity : Player->Entities())
                    if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
                        Entity.type == echoes::sim::EntityType::Worker && Entity.hitPoints > 0 &&
                        Entity.id != Builder) { Assistant = Entity.id; break; }
            }
            FEchoesTutorialConstructionSetup Setup;
            Setup.LocalPlayer = UEchoesSimulationSubsystem::LocalPlayerId;
            Setup.Builder = Builder;
            Setup.Assistant = Assistant;
            Setup.FirstInputSequence = TutorialInputAttemptSequence + 1;
            TutorialAcceptedCommandSequences.Reset();
            TutorialPendingRejectionAttempt = 0;
            TutorialConstructionHudCandidateEntity = 0;
            TutorialConstructionHudCandidateFrame = 0;
            if (!TutorialConstruction.Begin(TutorialSession, AuthorityGeneration, *Simulation, Setup))
            {
                TutorialInstruction = LOCTEXT("LinkStageUnavailable", "The Link staging is unavailable: two Surveyors and the damaged Power Link are required. Restart the readiness check from the title menu.");
                return;
            }
            TutorialWorkerId = Builder;
            TutorialActiveLessonBit = 32;
            TraceTutorialObservation(FString::Printf(TEXT("link_begin builder=%u assistant=%u repair=%u"),
                Builder, Assistant, static_cast<uint32>(TutorialConstruction.RepairTarget())));
        }
        TutorialConstruction.ObserveState(TutorialSession, AuthorityGeneration, *Simulation);
        for (int32 Index = TutorialAcceptedCommandSequences.Num() - 1; Index >= 0; --Index)
        {
            const auto& Command = TutorialAcceptedCommandSequences[Index];
            if (!Simulation->FindCommandResolutionReceipt(UEchoesSimulationSubsystem::LocalPlayerId, Command.Key)) continue;
            FEchoesTutorialConstructionInput Input;
            Input.Session = TutorialSession;
            Input.AuthorityGeneration = AuthorityGeneration;
            Input.InputSequence = ++TutorialInputAttemptSequence;
            Input.CommandSequence = Command.Key;
            Input.Origin = EEchoesTutorialConstructionOrigin::PlayerInput;
            (void)TutorialConstruction.ObserveAcceptedCommand(Input, *Simulation);
            TutorialAcceptedCommandSequences.RemoveAt(Index);
        }
        if (!TutorialConstruction.IsActive())
        {
            TutorialActiveLessonBit = 0;
            TutorialInstruction = LOCTEXT("LinkStageReset", "The Link observation was interrupted. Repeat the current check.");
            return;
        }
        const auto Progress = TutorialConstruction.Progress();
        if (!Progress.bRejectedPlacementObserved)
            TutorialInstruction = LOCTEXT("LinkReject", "Link: select a Surveyor, press N, and try the Power Link on the blocked outcrop marked X. Read why the ground refuses it.");
        else if (!Progress.bRejectionAcknowledged)
            TutorialInstruction = LOCTEXT("LinkAcknowledge", "Link: acknowledge the refused placement in the readiness panel, then find footing the network can hold.");
        else if (!Progress.bConstructionStarted)
            TutorialInstruction = LOCTEXT("LinkPlace", "Link: with a Surveyor selected, press N and place the Power Link on the footprint marked L. Green means the ground will take it.");
        else if (!Progress.bSimultaneousAssistObserved && !Progress.bConstructionCompleted)
            TutorialInstruction = LOCTEXT("LinkAssist", "Link: select a second Surveyor and right-click the unfinished Link so both crews build it together.");
        else if (!Progress.bConstructionCompleted)
            TutorialInstruction = LOCTEXT("LinkFinish", "Link: keep both Surveyors on the site until the Power Link completes.");
        else if (!Progress.bRepairCompleted)
            TutorialInstruction = LOCTEXT("LinkRepair", "Link: the older Power Link to the south-west is damaged. Select a Surveyor, press R, and click it; repair spends Matter until it is whole.");
        else if (!Progress.bOperationalSelectionPublished)
            TutorialInstruction = LOCTEXT("LinkInspect", "Link: select the Power Link you built and read its readout: connected, range, drop-off and Logistics.");
        if (Progress.PredicateSatisfied()) CommitTutorialLesson(32, TEXT("link"));
    }
    else if ((ProgressMask & 64) == 0)
    {
        // Lesson seven — Foundry (SPEC-TUT-008 chapter 3, "Prepare a first
        // force"): a Lancer queued at the staged Array Foundry by the player's
        // own Produce order, and the fielded Lancer that order produced.
        if (TutorialActiveLessonBit != 64)
        {
            uint32 Producer = 0;
            uint32 MaxId = 0;
            for (const auto& Entity : Player->Entities())
            {
                MaxId = FMath::Max(MaxId, static_cast<uint32>(Entity.id));
                if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
                    Entity.type == echoes::sim::EntityType::Barracks && Entity.completed &&
                    Entity.hitPoints > 0 && Producer == 0)
                    Producer = Entity.id;
            }
            if (Producer == 0)
            {
                TutorialInstruction = LOCTEXT("FoundryStageUnavailable", "The Foundry staging is unavailable. Restart the readiness check from the title menu.");
                return;
            }
            TutorialObservedProductionEntity = Producer;
            TutorialProductionSequence = 0;
            TutorialFoundryMaxKnownEntityId = MaxId;
            bTutorialFoundryEmerged = false;
            TutorialAcceptedCommandSequences.Reset();
            TutorialProductionLessonBit = 64;
            TutorialActiveLessonBit = 64;
            TraceTutorialObservation(FString::Printf(TEXT("foundry_begin producer=%u maxId=%u"), Producer, MaxId));
        }
        for (int32 Index = TutorialAcceptedCommandSequences.Num() - 1; Index >= 0; --Index)
        {
            const uint64 Sequence = TutorialAcceptedCommandSequences[Index].Key;
            const auto Receipt = Simulation->FindCommandResolutionReceipt(UEchoesSimulationSubsystem::LocalPlayerId, Sequence);
            if (!Receipt.has_value()) continue;
            TutorialAcceptedCommandSequences.RemoveAt(Index);
            if (Receipt->outcome != echoes::sim::CommandResolutionOutcome::Applied || TutorialProductionSequence != 0) continue;
            for (const auto& Command : Simulation->CommandLog())
            {
                if (Command.player == UEchoesSimulationSubsystem::LocalPlayerId && Command.sequence == Sequence &&
                    Command.type == echoes::sim::CommandType::Produce &&
                    Command.actor == TutorialObservedProductionEntity &&
                    Command.buildType == echoes::sim::EntityType::Soldier)
                {
                    TutorialProductionSequence = Sequence;
                    break;
                }
            }
        }
        const echoes::sim::Entity* Producer = nullptr;
        int32 Percent = 0;
        for (const auto& Entity : Player->Entities())
        {
            if (Entity.id == TutorialObservedProductionEntity) Producer = &Entity;
            if (TutorialProductionSequence != 0 && !bTutorialFoundryEmerged &&
                Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
                Entity.type == echoes::sim::EntityType::Soldier && Entity.completed &&
                Entity.hitPoints > 0 && static_cast<uint32>(Entity.id) > TutorialFoundryMaxKnownEntityId)
                bTutorialFoundryEmerged = true;
        }
        if (Producer == nullptr || Producer->hitPoints <= 0)
        {
            TutorialActiveLessonBit = 0;
            TutorialProductionLessonBit = 0;
            TutorialInstruction = LOCTEXT("FoundryStageReset", "The Array Foundry was lost. Restart the readiness check from the title menu.");
            return;
        }
        if (Producer->productionRequired > 0)
            Percent = FMath::Clamp(Producer->productionProgress * 100 / FMath::Max(1, Producer->productionRequired), 0, 100);
        if (TutorialProductionSequence == 0)
            TutorialInstruction = LOCTEXT("FoundryQueue", "Foundry: select the Array Foundry and queue a Lancer from its command card. The cost leaves the ledger the moment production starts.");
        else if (!bTutorialFoundryEmerged)
            TutorialInstruction = FText::Format(LOCTEXT("FoundryWait", "Foundry: Lancer in production, {0}%. It emerges at the exit and follows the rally when it is done."), FText::AsNumber(Percent));
        else
            TutorialInstruction = LOCTEXT("FoundryDone", "Foundry: your Lancer is on the field.");
        if (TutorialProductionSequence != 0 && bTutorialFoundryEmerged && CommitTutorialLesson(64, TEXT("foundry")))
            TutorialProductionLessonBit = 0;
    }
    else
    {
        TutorialActiveLessonBit = 0;
        TutorialInstruction = LOCTEXT("LaterLessonsUnavailable",
            "The first seven readiness lessons are complete. Further lessons are not available yet.");
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

// Call sites for the L4-ONBOARD cross-lane request recorded against
// SPEC-TUT-008.FLOW (lesson six): BuildAtCursor and ProduceUnit call these
// immediately after command acceptance, guarded by !IsReplayInputActive().
// The bodies are intentionally inert until the controller establishes the
// lesson session and authority generation that
// FEchoesTutorialConstructionObservation::ObserveAcceptedCommand requires;
// feeding it an unattributed input would be refused by its own provenance
// checks. No lesson is earned here and EchoesTutorialLessonCount stays five.
void AEchoesPlayerController::ObserveTutorialConstructionEvent(
    echoes::sim::EntityType StructureType,
    uint32 BuilderEntity,
    echoes::sim::Vec2 Site)
{
    (void)Site;
    auto* Bridge = GetWorld() ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>() : nullptr;
    if (!Bridge || !TutorialConstruction.IsActive() || StructureType != echoes::sim::EntityType::Dropoff ||
        BuilderEntity == 0) return;
    // The command was just accepted by the bridge; its sequence is judged
    // against its Applied receipt on the next fixed step.
    const TOptional<uint64> Sequence = Bridge->GetLastAcceptedLocalCommandSequence();
    if (Sequence.IsSet()) ObserveTutorialAcceptedCommand(Sequence.GetValue(), EEchoesTutorialOrderCommandOrigin::DirectPlayerCommand);
}

void AEchoesPlayerController::ObserveTutorialProductionEvent(
    echoes::sim::EntityType ProducedType,
    uint32 ProducerEntity)
{
    auto* Bridge = GetWorld() ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>() : nullptr;
    if (!Bridge || TutorialProductionLessonBit == 0 || ProducerEntity != TutorialObservedProductionEntity ||
        ProducedType != echoes::sim::EntityType::Soldier) return;
    const TOptional<uint64> Sequence = Bridge->GetLastAcceptedLocalCommandSequence();
    if (Sequence.IsSet()) ObserveTutorialAcceptedCommand(Sequence.GetValue(), EEchoesTutorialOrderCommandOrigin::DirectPlayerCommand);
}
