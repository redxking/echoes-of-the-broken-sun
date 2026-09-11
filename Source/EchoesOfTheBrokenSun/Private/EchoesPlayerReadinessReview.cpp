// SPEC-TUT-008 readiness lessons six to ten, rendered review driver.
//
// Runs only when the process was started with -EchoesReadinessReview. From the
// title it opens each lesson as a practice target, deploys the readiness drill
// and performs the lesson's actions through the same controller and bridge
// entry points the player's bindings reach (build placement is issued through
// the bridge and reported to the tutorial through the controller's own hooks,
// because the pointer trace inside ConfirmBuildPlacement would follow the real
// cursor). Each lesson is proven by the controller committing it, which in
// practice mode returns the player to the Help screen.
//
// Evidence class: agent-driven in-process rendered review. It is not physical
// input, not packaged execution and not owner acceptance. Author and owner:
// Angelis Pseftis.

#include "EchoesPlayerController.h"

#include "EchoesCinematicSubsystem.h"
#include "EchoesFieldHudView.h"
#include "EchoesOfTheBrokenSun.h"
#include "EchoesRTSCameraPawn.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesTutorialCurriculumModel.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

namespace EchoesReadinessReviewDetail
{
using echoes::sim::Entity;
using echoes::sim::EntityType;
using echoes::sim::Vec2;

constexpr echoes::sim::PlayerId kSeat = UEchoesSimulationSubsystem::LocalPlayerId;
constexpr float kTotalBudgetSeconds = 1200.0f;
constexpr float kCaptureDelaySeconds = 0.4f;

enum class EStage : int32
{
    OpenHelp = 0,
    OpenPractice,
    Deploy,
    SkipOpening,
    LinkReject,
    LinkAcknowledge,
    LinkPlace,
    LinkAssist,
    LinkRepair,
    LinkInspect,
    LinkCommitted,
    FoundryQueue,
    FoundryCommitted,
    ProbeOrders,
    ProbeCommitted,
    BoardLookAway,
    BoardJump,
    BoardCommitted,
    WellApproach,
    WellCommit,
    WellCommitted,
    Done
};

const TCHAR* StageName(int32 Stage)
{
    switch (static_cast<EStage>(Stage))
    {
        case EStage::OpenHelp: return TEXT("open_help");
        case EStage::OpenPractice: return TEXT("open_practice");
        case EStage::Deploy: return TEXT("deploy");
        case EStage::SkipOpening: return TEXT("skip_opening");
        case EStage::LinkReject: return TEXT("link_reject");
        case EStage::LinkAcknowledge: return TEXT("link_acknowledge");
        case EStage::LinkPlace: return TEXT("link_place");
        case EStage::LinkAssist: return TEXT("link_assist");
        case EStage::LinkRepair: return TEXT("link_repair");
        case EStage::LinkInspect: return TEXT("link_inspect");
        case EStage::LinkCommitted: return TEXT("link_committed");
        case EStage::FoundryQueue: return TEXT("foundry_queue");
        case EStage::FoundryCommitted: return TEXT("foundry_committed");
        case EStage::ProbeOrders: return TEXT("probe_orders");
        case EStage::ProbeCommitted: return TEXT("probe_committed");
        case EStage::BoardLookAway: return TEXT("board_look_away");
        case EStage::BoardJump: return TEXT("board_jump");
        case EStage::BoardCommitted: return TEXT("board_committed");
        case EStage::WellApproach: return TEXT("well_approach");
        case EStage::WellCommit: return TEXT("well_commit");
        case EStage::WellCommitted: return TEXT("well_committed");
        default: return TEXT("done");
    }
}

/** The lesson each practice pass targets, in curriculum order. */
constexpr uint16 kLessonBits[] = {32, 64, 128, 256, 512};
constexpr int32 kLessonCount = 5;

bool GCapturePending = false;
FString GCaptureName;
}  // namespace EchoesReadinessReviewDetail

void AEchoesPlayerController::StartReadinessReview()
{
    using namespace EchoesReadinessReviewDetail;
    if (bReadinessReviewActive) return;
    bReadinessReviewActive = true;
    ReadinessReviewStage = static_cast<int32>(EStage::OpenHelp);
    ReadinessReviewLessonIndex = 0;
    ReadinessReviewStageElapsedSeconds = 0.0f;
    ReadinessReviewTotalElapsedSeconds = 0.0f;
    ReadinessReviewCaptureIndex = 0;
    ReadinessReviewStagesPassed.Reset();
    ReadinessReviewOutputDir.Reset();
    GCapturePending = false;
    GCaptureName.Reset();
    FParse::Value(FCommandLine::Get(), TEXT("EchoesReadinessReviewOutputDir="), ReadinessReviewOutputDir);
    if (!ReadinessReviewOutputDir.IsEmpty())
    {
        IFileManager::Get().MakeDirectory(*ReadinessReviewOutputDir, true);
    }
    UE_LOG(LogEchoes, Display,
        TEXT("[ECHOES_READINESS_REVIEW_STARTED] contract=SPEC-TUT-008 lessons=6-10 lessonCount=%d outputDir=%s agentDriven=true osInjection=false unaidedHuman=false packaged=false controlledNonshipping=true"),
        EchoesTutorialLessonCount, *ReadinessReviewOutputDir);
}

void AEchoesPlayerController::CaptureReadinessReview(const TCHAR* Stage)
{
    if (ReadinessReviewOutputDir.IsEmpty()) return;
    const FString OutputPath = FPaths::Combine(
        ReadinessReviewOutputDir,
        FString::Printf(TEXT("%02d-%s.png"), ReadinessReviewCaptureIndex, Stage));
    ++ReadinessReviewCaptureIndex;
    FScreenshotRequest::RequestScreenshot(OutputPath, true, false, false, FIntRect(), true);
    UE_LOG(LogEchoes, Display, TEXT("[ECHOES_READINESS_REVIEW_CAPTURE] stage=%s output=%s"), Stage, *OutputPath);
}

void AEchoesPlayerController::FinishReadinessReview(const TCHAR* Result, const FString& Detail)
{
    using namespace EchoesReadinessReviewDetail;
    if (GCapturePending)
    {
        CaptureReadinessReview(*GCaptureName);
        GCapturePending = false;
    }
    bReadinessReviewActive = false;
    UE_LOG(LogEchoes, Display,
        TEXT("[ECHOES_READINESS_REVIEW_COMPLETE] result=%s stage=%s passed=%s verifiedMask=0x%03x sessionMask=0x%03x elapsed=%.1f detail=%s agentDriven=true osInjection=false unaidedHuman=false packaged=false controlledNonshipping=true"),
        Result, StageName(ReadinessReviewStage),
        ReadinessReviewStagesPassed.IsEmpty() ? TEXT("none") : *ReadinessReviewStagesPassed,
        PlayerProfile.TutorialVerifiedMask, TutorialSessionVerifiedMask,
        ReadinessReviewTotalElapsedSeconds, *Detail);
}

void AEchoesPlayerController::RunReadinessReviewStage(float DeltaTime)
{
    using namespace EchoesReadinessReviewDetail;
    ReadinessReviewStageElapsedSeconds += DeltaTime;
    ReadinessReviewTotalElapsedSeconds += DeltaTime;
    if (ReadinessReviewTotalElapsedSeconds > kTotalBudgetSeconds)
    {
        FinishReadinessReview(TEXT("FAILED"), TEXT("TOTAL_BUDGET_EXPIRED"));
        return;
    }
    const float Elapsed = ReadinessReviewStageElapsedSeconds;
    if (GCapturePending && Elapsed >= kCaptureDelaySeconds)
    {
        GCapturePending = false;
        CaptureReadinessReview(*GCaptureName);
    }
    const auto Stage = [this]() { return static_cast<EStage>(ReadinessReviewStage); };
    const auto Pass = [this](EStage Next, const FString& Detail)
    {
        const TCHAR* Name = StageName(ReadinessReviewStage);
        UE_LOG(LogEchoes, Display, TEXT("[ECHOES_READINESS_REVIEW_STAGE] lesson=%d stage=%s result=PASSED elapsed=%.1f detail=%s"),
            ReadinessReviewLessonIndex < kLessonCount ? 6 + ReadinessReviewLessonIndex : 0,
            Name, ReadinessReviewStageElapsedSeconds, *Detail);
        if (!ReadinessReviewStagesPassed.IsEmpty()) ReadinessReviewStagesPassed += TEXT(",");
        ReadinessReviewStagesPassed += Name;
        if (GCapturePending) CaptureReadinessReview(*GCaptureName);
        GCapturePending = true;
        GCaptureName = FString::Printf(TEXT("l%d-%s"), 6 + FMath::Min(ReadinessReviewLessonIndex, kLessonCount - 1), Name);
        ReadinessReviewStage = static_cast<int32>(Next);
        ReadinessReviewStageElapsedSeconds = 0.0f;
        bReadinessReviewActionIssued = false;
        ReadinessReviewNextRetrySeconds = 2.0f;
    };
    const auto Fail = [this](const TCHAR* Reason)
    {
        FinishReadinessReview(TEXT("FAILED"), FString::Printf(
            TEXT("%s_AT_STAGE_%s shell=%d instruction=%s status=%s"),
            Reason, StageName(ReadinessReviewStage), static_cast<int32>(PlayerFlow.Current()),
            *TutorialInstruction.ToString(), *GetStatusMessage()));
    };
    const auto Select = [this](const TArray<uint32>& Ids)
    {
        ClearSelection();
        for (const uint32 Id : Ids)
        {
            SelectedEntityIds.Add(Id);
            SetEntitySelected(Id, true);
        }
        NormalizeSelectionSubgroup();
    };
    // A lesson commits in practice mode by returning the player to Help with
    // a "Practice complete" shell message; that transition is the proof.
    const auto Committed = [this]()
    {
        return PlayerFlow.Current() == EEchoesShellScreen::Help &&
            ShellMessage.Contains(TEXT("Practice complete"));
    };
    const auto NextLesson = [this, &Pass](const FString& Detail)
    {
        ++ReadinessReviewLessonIndex;
        if (ReadinessReviewLessonIndex >= kLessonCount)
        {
            Pass(EStage::Done, Detail);
            return;
        }
        Pass(EStage::OpenPractice, Detail);
    };

    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>() : nullptr;
    if (Bridge == nullptr) { Fail(TEXT("NO_BRIDGE")); return; }

    if (Stage() == EStage::OpenHelp)
    {
        if (Elapsed < 2.0f) return;
        PresentTitleScreen();
        HandleShellAction(EEchoesShellAction::Help);
        if (PlayerFlow.Current() != EEchoesShellScreen::Help) { Fail(TEXT("HELP_UNREACHABLE")); return; }
        Pass(EStage::OpenPractice, TEXT("screen=help"));
        return;
    }
    if (Stage() == EStage::OpenPractice)
    {
        if (Elapsed < 1.0f) return;
        if (PlayerFlow.Current() != EEchoesShellScreen::Help)
        {
            PresentTitleScreen();
            HandleShellAction(EEchoesShellAction::Help);
        }
        const uint16 Bit = kLessonBits[ReadinessReviewLessonIndex];
        HandleShellAction(EEchoesShellAction::PracticeTutorialLesson, Bit);
        if (PlayerFlow.Current() == EEchoesShellScreen::Confirmation)
        {
            HandleShellAction(EEchoesShellAction::Confirm);
        }
        if (PlayerFlow.Current() != EEchoesShellScreen::Briefing || TutorialPractice.TargetLessonBit() != Bit)
        {
            Fail(TEXT("PRACTICE_UNREACHABLE"));
            return;
        }
        Pass(EStage::Deploy, FString::Printf(TEXT("practiceBit=%u"), Bit));
        return;
    }
    if (Stage() == EStage::Deploy)
    {
        if (Elapsed < 0.75f) return;
        HandleShellAction(EEchoesShellAction::Primary);
        if (PlayerFlow.Current() != EEchoesShellScreen::Gameplay) { Fail(TEXT("DEPLOY_FAILED")); return; }
        Pass(EStage::SkipOpening, TEXT("screen=gameplay"));
        return;
    }
    if (Stage() == EStage::SkipOpening)
    {
        // The drill opens with the M01 opening sequence, which a background
        // window keeps paused; the player's skip (Escape / held Space) is the
        // same call the driver makes here.
        if (bM01OpeningPending)
        {
            if (auto* Cinematic = GetWorld()->GetSubsystem<UEchoesCinematicSubsystem>())
            {
                Cinematic->SetSequencePaused(false);
                Cinematic->SkipActiveSequence();
            }
            if (Elapsed > 30.0f) Fail(TEXT("OPENING_NOT_SKIPPED"));
            return;
        }
        if (Elapsed < 1.0f) return;
        static constexpr EStage FirstStageForLesson[] = {
            EStage::LinkReject, EStage::FoundryQueue, EStage::ProbeOrders,
            EStage::BoardLookAway, EStage::WellApproach};
        Pass(FirstStageForLesson[ReadinessReviewLessonIndex], TEXT("opening skipped"));
        return;
    }
    if (Stage() == EStage::Done)
    {
        FinishReadinessReview(TEXT("PASSED"), TEXT("lessons 6-10 committed in practice"));
        return;
    }

    if (Bridge->GetSimulation() == nullptr || !Bridge->IsScenarioReady())
    {
        if (Elapsed > 30.0f) Fail(TEXT("SIM_NOT_READY"));
        return;
    }
    const echoes::sim::Simulation& Sim = *Bridge->GetSimulation();
    const auto OwnedOfType = [&Sim](EntityType Type, TArray<uint32>& Out)
    {
        for (const Entity& E : Sim.Entities())
            if (E.owner == kSeat && E.type == Type && E.hitPoints > 0 && E.completed) Out.Add(E.id);
    };
    const auto ObserveLast = [this, Bridge]()
    {
        const TOptional<uint64> Sequence = Bridge->GetLastAcceptedLocalCommandSequence();
        if (Sequence.IsSet())
            ObserveTutorialAcceptedCommand(Sequence.GetValue(), EEchoesTutorialOrderCommandOrigin::DirectPlayerCommand);
    };

    // ---- Lesson six: Link ----------------------------------------------
    if (Stage() == EStage::LinkReject)
    {
        if (Elapsed < 1.5f) return;
        if (!TutorialConstruction.IsActive()) { if (Elapsed > 20.0f) Fail(TEXT("LINK_LESSON_NOT_OPEN")); return; }
        Select({TutorialWorkerId});
        ObserveTutorialPlacementRejected(EntityType::Dropoff, Vec2::FromTiles(19, 10));
        if (!TutorialConstruction.Progress().bRejectedPlacementObserved) { Fail(TEXT("REJECTION_NOT_OBSERVED")); return; }
        Pass(EStage::LinkAcknowledge, FString::Printf(TEXT("pending=%llu"), static_cast<unsigned long long>(TutorialPendingRejectionAttempt)));
        return;
    }
    if (Stage() == EStage::LinkAcknowledge)
    {
        if (Elapsed < 1.0f) return;
        HandleFieldHudAction(EEchoesFieldHudAction::AcknowledgeTutorialRejection);
        if (!TutorialConstruction.Progress().bRejectionAcknowledged) { Fail(TEXT("ACKNOWLEDGE_NOT_OBSERVED")); return; }
        Pass(EStage::LinkPlace, TEXT("acknowledged"));
        return;
    }
    if (Stage() == EStage::LinkPlace)
    {
        if (Elapsed < 1.0f) return;
        if (!TutorialConstruction.Progress().bConstructionStarted)
        {
            if (!bReadinessReviewActionIssued)
            {
                bReadinessReviewActionIssued = true;
                FString Feedback;
                Select({TutorialWorkerId});
                const Vec2 Site = Vec2::FromTiles(6, 14);
                if (!Bridge->IssueBuildCommand(TutorialWorkerId, EntityType::Dropoff, Bridge->SimToWorld(Site), Feedback))
                {
                    Fail(*FString::Printf(TEXT("PLACEMENT_REFUSED_%s"), *Feedback.Replace(TEXT(" "), TEXT("_"))));
                    return;
                }
                ObserveTutorialConstructionEvent(EntityType::Dropoff, TutorialWorkerId, Site);
            }
            if (Elapsed > 30.0f) Fail(TEXT("SITE_NOT_BOUND"));
            return;
        }
        Pass(EStage::LinkAssist, FString::Printf(TEXT("site=%u"), static_cast<uint32>(TutorialConstruction.ConstructedSite())));
        return;
    }
    if (Stage() == EStage::LinkAssist)
    {
        const auto Progress = TutorialConstruction.Progress();
        if (!Progress.bSimultaneousAssistObserved && Elapsed >= 1.0f && !bReadinessReviewActionIssued)
        {
            bReadinessReviewActionIssued = true;
            TArray<uint32> Workers;
            OwnedOfType(EntityType::Worker, Workers);
            uint32 Assistant = 0;
            for (const uint32 Id : Workers) if (Id != TutorialWorkerId) { Assistant = Id; break; }
            if (Assistant == 0) { Fail(TEXT("NO_ASSISTANT")); return; }
            Select({Assistant});
            if (!IssueSelectedWorkerMaintenance(static_cast<uint32>(TutorialConstruction.ConstructedSite()), true))
            {
                Fail(TEXT("ASSIST_REFUSED"));
                return;
            }
        }
        if (Progress.bConstructionCompleted && Progress.bSimultaneousAssistObserved)
        {
            Pass(EStage::LinkRepair, TEXT("assisted and completed"));
            return;
        }
        if (Elapsed > 90.0f) Fail(Progress.bSimultaneousAssistObserved ? TEXT("SITE_NOT_COMPLETED") : TEXT("ASSIST_NOT_OBSERVED"));
        return;
    }
    if (Stage() == EStage::LinkRepair)
    {
        if (Elapsed >= 1.0f && !bReadinessReviewActionIssued)
        {
            bReadinessReviewActionIssued = true;
            Select({TutorialWorkerId});
            if (!IssueSelectedWorkerMaintenance(static_cast<uint32>(TutorialConstruction.RepairTarget()), false))
            {
                Fail(TEXT("REPAIR_REFUSED"));
                return;
            }
        }
        if (TutorialConstruction.Progress().bRepairCompleted)
        {
            Pass(EStage::LinkInspect, FString::Printf(TEXT("repaired=%u"), static_cast<uint32>(TutorialConstruction.RepairTarget())));
            return;
        }
        if (Elapsed > 120.0f) Fail(TEXT("REPAIR_NOT_COMPLETED"));
        return;
    }
    if (Stage() == EStage::LinkInspect)
    {
        if (Elapsed >= 1.0f && !bReadinessReviewActionIssued)
        {
            bReadinessReviewActionIssued = true;
            Select({static_cast<uint32>(TutorialConstruction.ConstructedSite())});
            ObserveTutorialSelectionEvent(EEchoesTutorialSelectionInputEvent::SingleClickSelection);
            RefreshFieldHud();
        }
        if (Committed()) { Pass(EStage::LinkCommitted, TEXT("committed")); return; }
        if (Elapsed > 15.0f) Fail(TEXT("LINK_NOT_COMMITTED"));
        return;
    }
    if (Stage() == EStage::LinkCommitted) { NextLesson(TEXT("lesson six proven")); return; }

    // ---- Lesson seven: Foundry -----------------------------------------
    if (Stage() == EStage::FoundryQueue)
    {
        if (Committed()) { Pass(EStage::FoundryCommitted, TEXT("lancer fielded")); return; }
        if (Elapsed < 1.5f) return;
        if (TutorialActiveLessonBit != 64) { if (Elapsed > 20.0f) Fail(TEXT("FOUNDRY_LESSON_NOT_OPEN")); return; }
        if (!bReadinessReviewActionIssued)
        {
            bReadinessReviewActionIssued = true;
            Select({TutorialObservedProductionEntity});
            ProduceUnit(EntityType::Soldier);
        }
        if (Committed()) { Pass(EStage::FoundryCommitted, TEXT("lancer fielded")); return; }
        if (Elapsed > 60.0f) Fail(TEXT("FOUNDRY_NOT_COMMITTED"));
        return;
    }
    if (Stage() == EStage::FoundryCommitted) { NextLesson(TEXT("lesson seven proven")); return; }

    // ---- Lesson eight: Probe -------------------------------------------
    if (Stage() == EStage::ProbeOrders)
    {
        if (Committed()) { Pass(EStage::ProbeCommitted, TEXT("probe broken")); return; }
        if (Elapsed < 1.5f) return;
        if (TutorialActiveLessonBit != 128) { if (Elapsed > 20.0f) Fail(TEXT("PROBE_LESSON_NOT_OPEN")); return; }
        TArray<uint32> Lancers, Heavies, Workers;
        OwnedOfType(EntityType::Soldier, Lancers);
        OwnedOfType(EntityType::HeavyUnit, Heavies);
        OwnedOfType(EntityType::Worker, Workers);
        if (Lancers.IsEmpty() || Heavies.IsEmpty() || Workers.IsEmpty()) { Fail(TEXT("PROBE_FORCE_MISSING")); return; }
        FString Feedback;
        const Vec2 ContactSite = Vec2::FromTiles(20, 8);
        if (!bReadinessReviewActionIssued)
        {
            // Guard first: the Bulwark on the Surveyor nearest the contact's side.
            bReadinessReviewActionIssued = true;
            uint32 Guarded = Workers[0];
            int64 Best = -1;
            for (const uint32 Id : Workers)
            {
                const Entity* W = Sim.FindEntity(Id);
                if (W == nullptr) continue;
                const int64 Dx = static_cast<int64>(W->position.x.Raw()) - ContactSite.x.Raw();
                const int64 Dy = static_cast<int64>(W->position.y.Raw()) - ContactSite.y.Raw();
                const int64 D = Dx * Dx + Dy * Dy;
                if (Best < 0 || D < Best) { Best = D; Guarded = Id; }
            }
            Select({Heavies[0]});
            if (Bridge->IssueCommand(echoes::sim::CommandType::Guard, Heavies[0], Guarded,
                    Bridge->SimToWorld(Sim.FindEntity(Guarded)->position), echoes::sim::FutureWellChoice::Dormant, Feedback))
                ObserveLast();
            ReadinessReviewNextRetrySeconds = Elapsed;
        }
        // Attack-move the Lancers toward the contact: at its approach first,
        // then onto it once it is in sight, re-aimed every few seconds while
        // it lives, exactly as a player re-aims on a moving target.
        if (Elapsed >= ReadinessReviewNextRetrySeconds)
        {
            ReadinessReviewNextRetrySeconds = Elapsed + 4.0f;
            const std::optional<echoes::sim::PlayerView> View = Bridge->GetLocalPlayerView();
            const Entity* Contact = nullptr;
            if (View.has_value())
            {
                for (const Entity& E : View->Entities())
                    if (E.owner != kSeat && E.owner != echoes::sim::kNeutralPlayer && E.hitPoints > 0 &&
                        (E.type == EntityType::Soldier || E.type == EntityType::HeavyUnit || E.type == EntityType::ScoutUnit))
                    { Contact = &E; break; }
            }
            const Vec2 Aim = Contact != nullptr ? Contact->position : ContactSite;
            Select(Lancers);
            for (const uint32 Id : Lancers)
            {
                if (Bridge->IssueCommand(echoes::sim::CommandType::AttackMove, Id, 0,
                        Bridge->SimToWorld(Aim), echoes::sim::FutureWellChoice::Dormant, Feedback))
                    ObserveLast();
            }
        }
        if (Committed()) { Pass(EStage::ProbeCommitted, TEXT("probe broken")); return; }
        if (Elapsed > 240.0f) Fail(TEXT("PROBE_NOT_COMMITTED"));
        return;
    }
    if (Stage() == EStage::ProbeCommitted) { NextLesson(TEXT("lesson eight proven")); return; }

    // ---- Lesson nine: Board --------------------------------------------
    if (Stage() == EStage::BoardLookAway)
    {
        if (Elapsed < 1.5f) return;
        if (TutorialActiveLessonBit != 256) { if (Elapsed > 20.0f) Fail(TEXT("BOARD_LESSON_NOT_OPEN")); return; }
        // A player looking elsewhere: the camera goes to the far side of the
        // basin so the second contact lands off-screen and the deck flags it.
        if (AEchoesRTSCameraPawn* Camera = Cast<AEchoesRTSCameraPawn>(GetPawn()))
            Camera->PanToWorld(Bridge->SimToWorld(Vec2::FromTiles(40, 40)));
        Pass(EStage::BoardJump, TEXT("camera=40,40"));
        return;
    }
    if (Stage() == EStage::BoardJump)
    {
        FVector2D AlertLocation;
        double RaisedSeconds = 0.0;
        const bool bRaised = EchoesFieldHud::LatestOffscreenCombatAlert(this, AlertLocation, RaisedSeconds) &&
            RaisedSeconds >= TutorialBoardOpenedSeconds;
        if (bRaised && !bTutorialBoardJumped) JumpToLatestAlert();
        if (Committed()) { Pass(EStage::BoardCommitted, FString::Printf(TEXT("alert=%.0f,%.0f"), AlertLocation.X, AlertLocation.Y)); return; }
        if (Elapsed > 240.0f) Fail(bRaised ? TEXT("BOARD_JUMP_NOT_VERIFIED") : TEXT("NO_OFFSCREEN_ALERT"));
        return;
    }
    if (Stage() == EStage::BoardCommitted) { NextLesson(TEXT("lesson nine proven")); return; }

    // ---- Lesson ten: Well ----------------------------------------------
    if (Stage() == EStage::WellApproach)
    {
        if (Elapsed < 1.5f) return;
        if (TutorialActiveLessonBit != 512) { if (Elapsed > 20.0f) Fail(TEXT("WELL_LESSON_NOT_OPEN")); return; }
        const Entity* Well = Sim.FindEntity(TutorialWellId);
        TArray<uint32> Workers;
        OwnedOfType(EntityType::Worker, Workers);
        if (Well == nullptr || Workers.IsEmpty()) { Fail(TEXT("WELL_OR_WORKER_MISSING")); return; }
        ReadinessReviewWellWorkerId = Workers[0];
        FString Feedback;
        Select({ReadinessReviewWellWorkerId});
        if (!Bridge->IssueCommand(echoes::sim::CommandType::Move, ReadinessReviewWellWorkerId, 0,
                Bridge->SimToWorld(Well->position), echoes::sim::FutureWellChoice::Dormant, Feedback))
        {
            Fail(TEXT("WELL_APPROACH_REFUSED"));
            return;
        }
        Pass(EStage::WellCommit, FString::Printf(TEXT("worker=%u well=%u"), ReadinessReviewWellWorkerId, TutorialWellId));
        return;
    }
    if (Stage() == EStage::WellCommit)
    {
        // The Well order needs the target visible, exactly as for the player;
        // retry every two seconds while the worker walks into sight of it.
        if (TutorialWellSequence == 0 && Elapsed >= ReadinessReviewNextRetrySeconds)
        {
            ReadinessReviewNextRetrySeconds = Elapsed + 2.0f;
            const Entity* Well = Sim.FindEntity(TutorialWellId);
            if (Well == nullptr) { Fail(TEXT("WELL_MISSING")); return; }
            FString Feedback;
            Select({ReadinessReviewWellWorkerId});
            if (Bridge->IssueCommand(echoes::sim::CommandType::FutureWell, ReadinessReviewWellWorkerId, TutorialWellId,
                    Bridge->SimToWorld(Well->position), echoes::sim::FutureWellChoice::Preserve, Feedback))
                ObserveLast();
        }
        if (Committed()) { Pass(EStage::WellCommitted, TEXT("protocol committed")); return; }
        if (Elapsed > 300.0f) Fail(TEXT("WELL_NOT_COMMITTED"));
        return;
    }
    if (Stage() == EStage::WellCommitted) { NextLesson(TEXT("lesson ten proven")); return; }
}
