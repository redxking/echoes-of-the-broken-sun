#include "EchoesCinematicSubsystem.h"

#include "EchoesNarrativeSubsystem.h"
#include "EchoesSimulationSubsystem.h"

#include "Channels/MovieSceneChannelProxy.h"
#include "Channels/MovieSceneDoubleChannel.h"
#include "CineCameraActor.h"
#include "CineCameraComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "MovieScene.h"
#include "Sections/MovieScene3DTransformSection.h"
#include "Sections/MovieSceneCameraCutSection.h"
#include "Tracks/MovieScene3DTransformTrack.h"
#include "Tracks/MovieSceneCameraCutTrack.h"

namespace
{
/** Display rate every runtime-built sequence is authored against. */
constexpr int32 CinematicFramesPerSecond = 30;

/**
 * Reference-sequence camera language: a measured, ground-referenced push
 * toward the battlefield origin at constant height, fixed downward pitch.
 * World units; heights stay at RTS review scale rather than hero close-ups.
 */
constexpr double ReferenceStartX = -2600.0;
constexpr double ReferenceStartY = -1400.0;
constexpr double ReferenceEndX = -1500.0;
constexpr double ReferenceEndY = -800.0;
constexpr double ReferenceCameraPitch = -34.0;

constexpr TCHAR M01CinematicId[] = TEXT("nar_m01_cin_opening");
constexpr TCHAR M01CinematicTriggerId[] =
    TEXT("nar_m01_evt_operation_started");
constexpr int32 M01OpeningShotCount = 4;

/** Adds start/end cubic keys to one channel of the transform section. */
void AddTravelKeys(FMovieSceneDoubleChannel* Channel,
                   const FFrameNumber StartFrame,
                   const FFrameNumber EndFrame,
                   const double StartValue,
                   const double EndValue)
{
    if (Channel == nullptr)
    {
        return;
    }
    Channel->AddCubicKey(StartFrame, StartValue);
    Channel->AddCubicKey(EndFrame, EndValue);
}
} // namespace

float UEchoesCinematicSubsystem::GetSequenceDurationSeconds(
    const EEchoesCinematicSequence Sequence) const
{
    switch (Sequence)
    {
        case EEchoesCinematicSequence::Reference:
            return 8.0f;
        case EEchoesCinematicSequence::M01Opening:
        {
            const UWorld* World = GetWorld();
            const UGameInstance* GameInstance =
                World != nullptr ? World->GetGameInstance() : nullptr;
            const UEchoesNarrativeSubsystem* Narrative =
                GameInstance != nullptr
                    ? GameInstance->GetSubsystem<UEchoesNarrativeSubsystem>()
                    : nullptr;
            const FEchoesNarrativeCinematic* Contract =
                Narrative != nullptr
                    ? Narrative->GetCinematic(
                          EEchoesOperationMode::CampaignPrologue)
                    : nullptr;
            return Contract != nullptr
                ? Contract->GetEditorialDurationSeconds()
                : 0.0f;
        }
        case EEchoesCinematicSequence::TitleSequence:
            return 72.0f;
        case EEchoesCinematicSequence::Act1ToAct2Transition:
            return 28.0f;
        case EEchoesCinematicSequence::Act2ToAct3Transition:
            return 30.0f;
        case EEchoesCinematicSequence::Act3ClimaxTransition:
            return 24.0f;
        case EEchoesCinematicSequence::EndingRestoration:
        case EEchoesCinematicSequence::EndingControlledStabilization:
        case EEchoesCinematicSequence::EndingExtinguishment:
        case EEchoesCinematicSequence::EndingOpenEvolution:
            return 32.0f;
        default:
            return 8.0f;
    }
}

TOptional<EEchoesCinematicSequence>
UEchoesCinematicSubsystem::ResolveSequenceForSignal(const FString& Signal)
{
    if (Signal == TEXT("cinematic:reference"))
    {
        return EEchoesCinematicSequence::Reference;
    }
    if (Signal == M01CinematicId || Signal == M01CinematicTriggerId ||
        Signal == TEXT("cinematic:m01_opening"))
    {
        return EEchoesCinematicSequence::M01Opening;
    }
    if (Signal == TEXT("cinematic:title") || Signal == TEXT("nar_frontdoor_title"))
    {
        return EEchoesCinematicSequence::TitleSequence;
    }
    if (Signal == TEXT("cinematic:act1_to_act2") ||
        Signal == TEXT("nar_m05_evt_withdrawal_complete"))
    {
        return EEchoesCinematicSequence::Act1ToAct2Transition;
    }
    if (Signal == TEXT("cinematic:act2_to_act3") ||
        Signal == TEXT("nar_m09_evt_power_cascade"))
    {
        return EEchoesCinematicSequence::Act2ToAct3Transition;
    }
    if (Signal == TEXT("cinematic:act3_climax") ||
        Signal == TEXT("nar_m14_evt_sanctum_collapse"))
    {
        return EEchoesCinematicSequence::Act3ClimaxTransition;
    }
    if (Signal == TEXT("cinematic:ending_restoration") ||
        Signal == TEXT("nar_m15_evt_restoration_committed"))
    {
        return EEchoesCinematicSequence::EndingRestoration;
    }
    if (Signal == TEXT("cinematic:ending_controlled_stabilization") ||
        Signal == TEXT("nar_m15_evt_stabilization_committed"))
    {
        return EEchoesCinematicSequence::EndingControlledStabilization;
    }
    if (Signal == TEXT("cinematic:ending_extinguishment") ||
        Signal == TEXT("nar_m15_evt_extinguishment_committed"))
    {
        return EEchoesCinematicSequence::EndingExtinguishment;
    }
    if (Signal == TEXT("cinematic:ending_open_evolution") ||
        Signal == TEXT("nar_m15_evt_evolution_committed"))
    {
        return EEchoesCinematicSequence::EndingOpenEvolution;
    }
    return TOptional<EEchoesCinematicSequence>();
}

void UEchoesCinematicSubsystem::Deinitialize()
{
    if (bSequenceActive)
    {
        FinishActiveSequence(true);
    }
    Super::Deinitialize();
}

void UEchoesCinematicSubsystem::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    if (bSequenceActive && !bSequencePaused && SequencePlayer != nullptr &&
        !SequencePlayer->IsPlaying())
    {
        FinishActiveSequence(false);
    }
}

bool UEchoesCinematicSubsystem::PlaySequence(
    const EEchoesCinematicSequence Sequence)
{
    UWorld* World = GetWorld();
    if (bSequenceActive)
    {
        return false;
    }

    bHasCurrentSequence = true;
    CurrentSequence = Sequence;
    LastCompletion = EEchoesCinematicCompletion::None;
    bSequencePaused = false;
    bScenarioWasPaused = false;
    bPausedScenario = false;
    bViewTargetCaptured = false;
    PreviousViewTarget = nullptr;
    if (World == nullptr)
    {
        LastCompletion = EEchoesCinematicCompletion::FailedToStart;
        return false;
    }

    ULevelSequence* Built = nullptr;
    switch (Sequence)
    {
        case EEchoesCinematicSequence::Reference:
            Built = BuildReferenceSequence();
            break;
        case EEchoesCinematicSequence::M01Opening:
        {
            const UGameInstance* GameInstance = World->GetGameInstance();
            const UEchoesNarrativeSubsystem* Narrative =
                GameInstance != nullptr
                    ? GameInstance->GetSubsystem<UEchoesNarrativeSubsystem>()
                    : nullptr;
            const FEchoesNarrativeCinematic* Contract =
                Narrative != nullptr
                    ? Narrative->GetCinematic(
                          EEchoesOperationMode::CampaignPrologue)
                    : nullptr;
            if (Contract != nullptr && Contract->Id == M01CinematicId &&
                Contract->TriggerId == M01CinematicTriggerId &&
                Contract->Format == TEXT("in_engine_storyboard") &&
                !Contract->bNamedCharacterPhysicalPresenceAsserted &&
                Contract->Shots.Num() == M01OpeningShotCount &&
                Contract->GetEditorialDurationSeconds() > 0.0f &&
                Contract->GetEditorialDurationSeconds() <= 90.0f)
            {
                Built = BuildM01OpeningSequence(*Contract);
            }
            break;
        }
        case EEchoesCinematicSequence::TitleSequence:
            Built = BuildTitleSequence();
            break;
        case EEchoesCinematicSequence::Act1ToAct2Transition:
            Built = BuildAct1ToAct2Sequence();
            break;
        case EEchoesCinematicSequence::Act2ToAct3Transition:
            Built = BuildAct2ToAct3Sequence();
            break;
        case EEchoesCinematicSequence::Act3ClimaxTransition:
            Built = BuildAct3ClimaxSequence();
            break;
        case EEchoesCinematicSequence::EndingRestoration:
            Built = BuildEndingRestorationSequence();
            break;
        case EEchoesCinematicSequence::EndingControlledStabilization:
            Built = BuildEndingControlledStabilizationSequence();
            break;
        case EEchoesCinematicSequence::EndingExtinguishment:
            Built = BuildEndingExtinguishmentSequence();
            break;
        case EEchoesCinematicSequence::EndingOpenEvolution:
            Built = BuildEndingOpenEvolutionSequence();
            break;
        default:
            LastCompletion = EEchoesCinematicCompletion::FailedToStart;
            return false;
    }

    if (Built == nullptr)
    {
        DestroySpawnedCameras();
        LastCompletion = EEchoesCinematicCompletion::FailedToStart;
        return false;
    }

    FMovieSceneSequencePlaybackSettings Settings;
    Settings.bDisableMovementInput = true;
    Settings.bDisableLookAtInput = true;
    Settings.bHideHud = false;

    ALevelSequenceActor* SpawnedActor = nullptr;
    ULevelSequencePlayer* Player = ULevelSequencePlayer::CreateLevelSequencePlayer(
        World, Built, Settings, SpawnedActor);
    if (Player == nullptr || SpawnedActor == nullptr)
    {
        DestroySpawnedCameras();
        LastCompletion = EEchoesCinematicCompletion::FailedToStart;
        return false;
    }

    ActiveSequence = Built;
    SequenceActor = SpawnedActor;
    SequencePlayer = Player;
    bSequenceActive = true;

    if (APlayerController* Controller = World->GetFirstPlayerController())
    {
        PreviousViewTarget = Controller->GetViewTarget();
        bViewTargetCaptured = true;
        // Apply the player-facing lock synchronously. Level Sequence also owns
        // this state during playback and clears it on Stop, but its start hook
        // can be deferred until the first evaluation in a newly created world.
        Controller->SetCinematicMode(true, false, false, true, true);
    }

    // Presentation-only pause: remember the authoritative pause state the
    // sequence found so returning to play restores it exactly.
    if (UEchoesSimulationSubsystem* Bridge =
            World->GetSubsystem<UEchoesSimulationSubsystem>())
    {
        bScenarioWasPaused = Bridge->IsScenarioPaused();
        if (!bScenarioWasPaused)
        {
            Bridge->SetScenarioPaused(true);
            bPausedScenario = true;
        }
    }

    Player->Play();
    return true;
}

bool UEchoesCinematicSubsystem::SetSequencePaused(const bool bPaused)
{
    if (!bSequenceActive || SequencePlayer == nullptr)
    {
        return false;
    }
    if (bSequencePaused == bPaused)
    {
        return true;
    }

    UWorld* World = GetWorld();
    const double NowSeconds =
        World != nullptr ? World->GetRealTimeSeconds() : 0.0;
    UEchoesNarrativeSubsystem* Narrative = nullptr;
    if (UGameInstance* GameInstance =
            World != nullptr ? World->GetGameInstance() : nullptr)
    {
        Narrative = GameInstance->GetSubsystem<UEchoesNarrativeSubsystem>();
    }

    if (bPaused)
    {
        SequencePlayer->Pause();
        if (Narrative != nullptr)
        {
            Narrative->SetSubtitlePlaybackPaused(true, NowSeconds);
        }
    }
    else
    {
        if (Narrative != nullptr)
        {
            Narrative->SetSubtitlePlaybackPaused(false, NowSeconds);
        }
        SequencePlayer->Play();
    }
    bSequencePaused = bPaused;
    return true;
}

double UEchoesCinematicSubsystem::GetSequenceElapsedSeconds() const
{
    return SequencePlayer != nullptr
        ? SequencePlayer->GetCurrentTime().AsSeconds()
        : 0.0;
}

#if WITH_DEV_AUTOMATION_TESTS
void UEchoesCinematicSubsystem::AdvanceActiveSequenceForTest(
    const float DeltaSeconds)
{
    if (SequencePlayer != nullptr && !bSequencePaused)
    {
        SequencePlayer->Update(DeltaSeconds);
    }
}

int32 UEchoesCinematicSubsystem::GetCameraCutCountForTest() const
{
    const UMovieScene* MovieScene =
        ActiveSequence != nullptr ? ActiveSequence->GetMovieScene() : nullptr;
    const UMovieSceneCameraCutTrack* CutTrack =
        MovieScene != nullptr
            ? Cast<UMovieSceneCameraCutTrack>(MovieScene->GetCameraCutTrack())
            : nullptr;
    return CutTrack != nullptr ? CutTrack->GetAllSections().Num() : 0;
}
#endif

void UEchoesCinematicSubsystem::SkipActiveSequence()
{
    if (bSequenceActive)
    {
        FinishActiveSequence(true);
    }
}

void UEchoesCinematicSubsystem::FinishActiveSequence(const bool bSkipped)
{
    if (!bSequenceActive)
    {
        return;
    }

    if (SequencePlayer != nullptr)
    {
        if (bSequencePaused)
        {
            if (UWorld* World = GetWorld())
            {
                if (UGameInstance* GameInstance = World->GetGameInstance())
                {
                    if (UEchoesNarrativeSubsystem* Narrative =
                            GameInstance->GetSubsystem<
                                UEchoesNarrativeSubsystem>())
                    {
                        Narrative->SetSubtitlePlaybackPaused(
                            false, World->GetRealTimeSeconds());
                    }
                }
            }
        }
        SequencePlayer->Stop();
        SequencePlayer = nullptr;
    }
    bSequencePaused = false;
    if (SequenceActor != nullptr)
    {
        SequenceActor->Destroy();
        SequenceActor = nullptr;
    }

    if (bViewTargetCaptured)
    {
        if (UWorld* World = GetWorld())
        {
            if (APlayerController* Controller =
                    World->GetFirstPlayerController())
            {
                if (AActor* ViewTarget = PreviousViewTarget.Get())
                {
                    Controller->SetViewTarget(ViewTarget);
                }
            }
        }
        bViewTargetCaptured = false;
        PreviousViewTarget = nullptr;
    }
    DestroySpawnedCameras();
    ActiveSequence = nullptr;
    bSequenceActive = false;

    // Return to play: restore exactly the pause state the sequence found.
    if (bPausedScenario)
    {
        if (UWorld* World = GetWorld())
        {
            if (UEchoesSimulationSubsystem* Bridge =
                    World->GetSubsystem<UEchoesSimulationSubsystem>())
            {
                if (!bScenarioWasPaused)
                {
                    Bridge->SetScenarioPaused(false);
                }
            }
        }
        bPausedScenario = false;
    }
    bScenarioWasPaused = false;
    LastCompletion = bSkipped
        ? EEchoesCinematicCompletion::Skipped
        : EEchoesCinematicCompletion::NaturalFinish;
    ++CompletedPlaybackCount;
}

void UEchoesCinematicSubsystem::DestroySpawnedCameras()
{
    if (CameraActor != nullptr)
    {
        CameraActor->Destroy();
        CameraActor = nullptr;
    }
    for (ACineCameraActor* Camera : AdditionalCameraActors)
    {
        if (Camera != nullptr)
        {
            Camera->Destroy();
        }
    }
    AdditionalCameraActors.Reset();
}

ULevelSequence* UEchoesCinematicSubsystem::BuildCustomSequence(
    const TCHAR* SequenceName,
    const TCHAR* CameraBindingName,
    const float DurationSeconds,
    const FVector& StartLocation,
    const FRotator& StartRotation,
    const FVector& EndLocation,
    const FRotator& EndRotation)
{
    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return nullptr;
    }

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    ACineCameraActor* Camera = World->SpawnActor<ACineCameraActor>(
        StartLocation, StartRotation, SpawnParameters);
    if (Camera == nullptr)
    {
        return nullptr;
    }
    Camera->SetActorEnableCollision(false);

    ULevelSequence* Sequence = NewObject<ULevelSequence>(
        this, SequenceName, RF_Transient);
    Sequence->Initialize();

    UMovieScene* MovieScene = Sequence->GetMovieScene();
    MovieScene->SetDisplayRate(FFrameRate(CinematicFramesPerSecond, 1));
    const FFrameRate TickResolution = MovieScene->GetTickResolution();
    const FFrameNumber EndFrame =
        (DurationSeconds * TickResolution).CeilToFrame();
    MovieScene->SetPlaybackRange(TRange<FFrameNumber>(FFrameNumber(0), EndFrame));

    const FGuid CameraBinding = MovieScene->AddPossessable(
        CameraBindingName, ACineCameraActor::StaticClass());
    Sequence->BindPossessableObject(CameraBinding, *Camera, World);

    UMovieScene3DTransformTrack* TransformTrack =
        MovieScene->AddTrack<UMovieScene3DTransformTrack>(CameraBinding);
    UMovieScene3DTransformSection* TransformSection =
        Cast<UMovieScene3DTransformSection>(TransformTrack->CreateNewSection());
    TransformSection->SetRange(
        TRange<FFrameNumber>(FFrameNumber(0), EndFrame));
    TransformTrack->AddSection(*TransformSection);

    const TArrayView<FMovieSceneDoubleChannel*> Channels =
        TransformSection->GetChannelProxy()
            .GetChannels<FMovieSceneDoubleChannel>();
    if (Channels.Num() >= 9)
    {
        AddTravelKeys(
            Channels[0], FFrameNumber(0), EndFrame,
            StartLocation.X, EndLocation.X);
        AddTravelKeys(
            Channels[1], FFrameNumber(0), EndFrame,
            StartLocation.Y, EndLocation.Y);
        AddTravelKeys(
            Channels[2], FFrameNumber(0), EndFrame,
            StartLocation.Z, EndLocation.Z);
        AddTravelKeys(
            Channels[3], FFrameNumber(0), EndFrame,
            StartRotation.Roll, EndRotation.Roll);
        AddTravelKeys(
            Channels[4], FFrameNumber(0), EndFrame,
            StartRotation.Pitch, EndRotation.Pitch);
        AddTravelKeys(
            Channels[5], FFrameNumber(0), EndFrame,
            StartRotation.Yaw, EndRotation.Yaw);
    }

    UMovieSceneCameraCutTrack* CutTrack = Cast<UMovieSceneCameraCutTrack>(
        MovieScene->AddCameraCutTrack(UMovieSceneCameraCutTrack::StaticClass()));
    CutTrack->AddNewCameraCut(
        UE::MovieScene::FRelativeObjectBindingID(CameraBinding),
        FFrameNumber(0));

    CameraActor = Camera;
    return Sequence;
}

ULevelSequence* UEchoesCinematicSubsystem::BuildReferenceSequence()
{
    return BuildCustomSequence(
        TEXT("EchoesReferenceSequence"),
        TEXT("EchoesReferenceCamera"),
        GetReferenceDurationSeconds(),
        FVector(ReferenceStartX, ReferenceStartY, GetReferenceCameraHeight()),
        FRotator(ReferenceCameraPitch, 0.0f, 0.0f),
        FVector(ReferenceEndX, ReferenceEndY, GetReferenceCameraHeight()),
        FRotator(ReferenceCameraPitch, 0.0f, 0.0f));
}

ULevelSequence* UEchoesCinematicSubsystem::BuildM01OpeningSequence(
    const FEchoesNarrativeCinematic& Contract)
{
    UWorld* World = GetWorld();
    UEchoesSimulationSubsystem* Bridge =
        World != nullptr
            ? World->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (World == nullptr || Bridge == nullptr ||
        Contract.Shots.Num() != M01OpeningShotCount)
    {
        return nullptr;
    }

    // The source storyboard names objective geometry and explicitly asserts
    // no named-character physical staging. These cameras therefore frame only
    // the battlefield overview, recovery marker, Future Well, and tactical
    // handoff at the registered 64x64 M01 presentation scale.
    const FVector ArchiveSite = Bridge->SimToWorld(
        echoes::sim::Vec2::FromTiles(22, 18));
    const FVector FutureWell = Bridge->SimToWorld(
        echoes::sim::Vec2::FromTiles(32, 32));
    const FVector OverviewTarget =
        (ArchiveSite + FutureWell) * 0.5f;
    ULevelSequence* Sequence = NewObject<ULevelSequence>(
        this, TEXT("EchoesM01OpeningSequence"), RF_Transient);
    Sequence->Initialize();
    UMovieScene* MovieScene = Sequence->GetMovieScene();
    MovieScene->SetDisplayRate(FFrameRate(CinematicFramesPerSecond, 1));
    const FFrameRate TickResolution = MovieScene->GetTickResolution();
    const FFrameNumber FinalFrame =
        (Contract.GetEditorialDurationSeconds() * TickResolution).CeilToFrame();
    MovieScene->SetPlaybackRange(
        TRange<FFrameNumber>(FFrameNumber(0), FinalFrame));

    UMovieSceneCameraCutTrack* CutTrack = Cast<UMovieSceneCameraCutTrack>(
        MovieScene->AddCameraCutTrack(UMovieSceneCameraCutTrack::StaticClass()));
    if (CutTrack == nullptr)
    {
        return nullptr;
    }

    float ShotStartSeconds = 0.0f;
    for (int32 ShotIndex = 0;
         ShotIndex < Contract.Shots.Num();
         ++ShotIndex)
    {
        const FEchoesNarrativeCinematicShot& Shot = Contract.Shots[ShotIndex];
        const FFrameNumber ShotStartFrame =
            (ShotStartSeconds * TickResolution).RoundToFrame();
        const float ShotEndSeconds =
            ShotStartSeconds + Shot.EditorialTargetSeconds;
        const FFrameNumber ShotEndFrame =
            (ShotEndSeconds * TickResolution).RoundToFrame();

        FVector ShotTarget;
        FVector ShotOffset;
        if (Shot.VisualHookIds.Contains(TEXT("vis_m01_glass_scar_overview")))
        {
            ShotTarget = OverviewTarget;
            ShotOffset = FVector(-2400.0, -2600.0, 5200.0);
        }
        else if (Shot.VisualHookIds.Contains(
                     TEXT("vis_m01_archive_route_overlay")))
        {
            ShotTarget = ArchiveSite;
            ShotOffset = FVector(-1200.0, -900.0, 2300.0);
        }
        else if (Shot.VisualHookIds.Contains(
                     TEXT("vis_m01_future_well_propagation")))
        {
            ShotTarget = FutureWell;
            ShotOffset = FVector(1200.0, -1000.0, 2200.0);
        }
        else if (Shot.VisualHookIds.Contains(
                     TEXT("vis_m01_tactical_handoff")))
        {
            ShotTarget = ArchiveSite;
            ShotOffset = FVector(-700.0, -550.0, 3100.0);
        }
        else
        {
            return nullptr;
        }

        const FVector StartLocation = ShotTarget + ShotOffset;
        const FVector EndLocation = FMath::Lerp(
            StartLocation,
            ShotTarget + FVector(0.0, 0.0, 650.0),
            0.12f);
        const FRotator StartRotation =
            (ShotTarget - StartLocation).Rotation();
        const FRotator EndRotation =
            (ShotTarget - EndLocation).Rotation();

        FActorSpawnParameters SpawnParameters;
        SpawnParameters.SpawnCollisionHandlingOverride =
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        ACineCameraActor* Camera = World->SpawnActor<ACineCameraActor>(
            StartLocation, StartRotation, SpawnParameters);
        if (Camera == nullptr)
        {
            return nullptr;
        }
        Camera->SetActorEnableCollision(false);
        // A wide, fixed documentary lens preserves the geography in these
        // terrain-scale shots. A default portrait lens magnifies a few paving
        // tiles and loses the objective context at the same authored position.
        if (UCineCameraComponent* Lens = Camera->GetCineCameraComponent())
        {
            Lens->SetCurrentFocalLength(24.0f);
            Lens->FocusSettings.FocusMethod = ECameraFocusMethod::Disable;
        }
        if (ShotIndex == 0)
        {
            CameraActor = Camera;
        }
        else
        {
            AdditionalCameraActors.Add(Camera);
        }

        const FGuid CameraBinding = MovieScene->AddPossessable(
            *Shot.Id, ACineCameraActor::StaticClass());
        Sequence->BindPossessableObject(CameraBinding, *Camera, World);

        UMovieScene3DTransformTrack* TransformTrack =
            MovieScene->AddTrack<UMovieScene3DTransformTrack>(CameraBinding);
        UMovieScene3DTransformSection* TransformSection =
            TransformTrack != nullptr
                ? Cast<UMovieScene3DTransformSection>(
                      TransformTrack->CreateNewSection())
                : nullptr;
        if (TransformSection == nullptr)
        {
            return nullptr;
        }
        TransformSection->SetRange(
            TRange<FFrameNumber>(ShotStartFrame, ShotEndFrame));
        TransformTrack->AddSection(*TransformSection);
        const TArrayView<FMovieSceneDoubleChannel*> Channels =
            TransformSection->GetChannelProxy()
                .GetChannels<FMovieSceneDoubleChannel>();
        if (Channels.Num() < 9)
        {
            return nullptr;
        }
        AddTravelKeys(
            Channels[0], ShotStartFrame, ShotEndFrame,
            StartLocation.X, EndLocation.X);
        AddTravelKeys(
            Channels[1], ShotStartFrame, ShotEndFrame,
            StartLocation.Y, EndLocation.Y);
        AddTravelKeys(
            Channels[2], ShotStartFrame, ShotEndFrame,
            StartLocation.Z, EndLocation.Z);
        AddTravelKeys(
            Channels[3], ShotStartFrame, ShotEndFrame,
            StartRotation.Roll, EndRotation.Roll);
        AddTravelKeys(
            Channels[4], ShotStartFrame, ShotEndFrame,
            StartRotation.Pitch, EndRotation.Pitch);
        AddTravelKeys(
            Channels[5], ShotStartFrame, ShotEndFrame,
            StartRotation.Yaw, EndRotation.Yaw);

        CutTrack->AddNewCameraCut(
            UE::MovieScene::FRelativeObjectBindingID(CameraBinding),
            ShotStartFrame);
        ShotStartSeconds = ShotEndSeconds;
    }

    return Sequence;
}

ULevelSequence* UEchoesCinematicSubsystem::BuildTitleSequence()
{
    return BuildCustomSequence(
        TEXT("EchoesTitleSequence"),
        TEXT("EchoesTitleCamera"),
        GetSequenceDurationSeconds(EEchoesCinematicSequence::TitleSequence),
        FVector(-3200.0, -2400.0, 1800.0),
        FRotator(-28.0, 35.0, 0.0),
        FVector(0.0, 0.0, 1200.0),
        FRotator(-55.0, 0.0, 0.0));
}

ULevelSequence* UEchoesCinematicSubsystem::BuildAct1ToAct2Sequence()
{
    return BuildCustomSequence(
        TEXT("EchoesAct1ToAct2Sequence"),
        TEXT("EchoesAct1ToAct2Camera"),
        GetSequenceDurationSeconds(EEchoesCinematicSequence::Act1ToAct2Transition),
        FVector(600.0, 1700.0, 1500.0),
        FRotator(-42.0, -110.0, 0.0),
        FVector(200.0, 400.0, 800.0),
        FRotator(-30.0, -75.0, 0.0));
}

ULevelSequence* UEchoesCinematicSubsystem::BuildAct2ToAct3Sequence()
{
    return BuildCustomSequence(
        TEXT("EchoesAct2ToAct3Sequence"),
        TEXT("EchoesAct2ToAct3Camera"),
        GetSequenceDurationSeconds(EEchoesCinematicSequence::Act2ToAct3Transition),
        FVector(0.0, -2200.0, 2200.0),
        FRotator(-25.0, 90.0, 0.0),
        FVector(0.0, 0.0, 1200.0),
        FRotator(-52.0, 90.0, 0.0));
}

ULevelSequence* UEchoesCinematicSubsystem::BuildAct3ClimaxSequence()
{
    return BuildCustomSequence(
        TEXT("EchoesAct3ClimaxSequence"),
        TEXT("EchoesAct3ClimaxCamera"),
        GetSequenceDurationSeconds(EEchoesCinematicSequence::Act3ClimaxTransition),
        FVector(1400.0, 1400.0, 1600.0),
        FRotator(-45.0, -135.0, 0.0),
        FVector(0.0, 0.0, 950.0),
        FRotator(-55.0, -135.0, 0.0));
}

ULevelSequence* UEchoesCinematicSubsystem::BuildEndingRestorationSequence()
{
    return BuildCustomSequence(
        TEXT("EchoesEndingRestorationSequence"),
        TEXT("EchoesEndingRestorationCamera"),
        GetSequenceDurationSeconds(EEchoesCinematicSequence::EndingRestoration),
        FVector(0.0, -1800.0, 850.0),
        FRotator(-25.0, 90.0, 0.0),
        FVector(0.0, 0.0, 1600.0),
        FRotator(-60.0, 90.0, 0.0));
}

ULevelSequence* UEchoesCinematicSubsystem::BuildEndingControlledStabilizationSequence()
{
    return BuildCustomSequence(
        TEXT("EchoesEndingControlledStabilizationSequence"),
        TEXT("EchoesEndingControlledStabilizationCamera"),
        GetSequenceDurationSeconds(EEchoesCinematicSequence::EndingControlledStabilization),
        FVector(-1600.0, -1600.0, 900.0),
        FRotator(-32.0, 45.0, 0.0),
        FVector(0.0, 0.0, 1400.0),
        FRotator(-48.0, 45.0, 0.0));
}

ULevelSequence* UEchoesCinematicSubsystem::BuildEndingExtinguishmentSequence()
{
    return BuildCustomSequence(
        TEXT("EchoesEndingExtinguishmentSequence"),
        TEXT("EchoesEndingExtinguishmentCamera"),
        GetSequenceDurationSeconds(EEchoesCinematicSequence::EndingExtinguishment),
        FVector(1200.0, -1200.0, 1000.0),
        FRotator(-35.0, 135.0, 0.0),
        FVector(0.0, 0.0, 1800.0),
        FRotator(-70.0, 135.0, 0.0));
}

ULevelSequence* UEchoesCinematicSubsystem::BuildEndingOpenEvolutionSequence()
{
    return BuildCustomSequence(
        TEXT("EchoesEndingOpenEvolutionSequence"),
        TEXT("EchoesEndingOpenEvolutionCamera"),
        GetSequenceDurationSeconds(EEchoesCinematicSequence::EndingOpenEvolution),
        FVector(0.0, 1600.0, 750.0),
        FRotator(-20.0, -90.0, 0.0),
        FVector(0.0, 0.0, 1500.0),
        FRotator(-42.0, -90.0, 0.0));
}
