#include "EchoesCinematicSubsystem.h"

#include "EchoesSimulationSubsystem.h"

#include "Channels/MovieSceneChannelProxy.h"
#include "Channels/MovieSceneDoubleChannel.h"
#include "CineCameraActor.h"
#include "Engine/World.h"
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

/** Adds start/end cubic keys to one channel of the transform section. */
void AddTravelKeys(FMovieSceneDoubleChannel* Channel,
                   const FFrameNumber EndFrame,
                   const double StartValue,
                   const double EndValue)
{
    if (Channel == nullptr)
    {
        return;
    }
    Channel->AddCubicKey(FFrameNumber(0), StartValue);
    Channel->AddCubicKey(EndFrame, EndValue);
}
} // namespace

TOptional<EEchoesCinematicSequence>
UEchoesCinematicSubsystem::ResolveSequenceForSignal(const FString& Signal)
{
    if (Signal == TEXT("cinematic:reference"))
    {
        return EEchoesCinematicSequence::Reference;
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
    if (bSequenceActive && SequencePlayer != nullptr &&
        !SequencePlayer->IsPlaying())
    {
        FinishActiveSequence(false);
    }
}

bool UEchoesCinematicSubsystem::PlaySequence(
    const EEchoesCinematicSequence Sequence)
{
    UWorld* World = GetWorld();
    if (bSequenceActive || World == nullptr)
    {
        return false;
    }

    check(Sequence == EEchoesCinematicSequence::Reference);
    ULevelSequence* Built = BuildReferenceSequence();
    if (Built == nullptr)
    {
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
        if (CameraActor != nullptr)
        {
            CameraActor->Destroy();
            CameraActor = nullptr;
        }
        return false;
    }

    ActiveSequence = Built;
    SequenceActor = SpawnedActor;
    SequencePlayer = Player;
    bSequenceActive = true;

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

#if WITH_DEV_AUTOMATION_TESTS
void UEchoesCinematicSubsystem::AdvanceActiveSequenceForTest(
    const float DeltaSeconds)
{
    if (SequencePlayer != nullptr)
    {
        SequencePlayer->Update(DeltaSeconds);
    }
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
    if (SequencePlayer != nullptr)
    {
        if (bSkipped && SequencePlayer->IsPlaying())
        {
            SequencePlayer->Stop();
        }
        SequencePlayer = nullptr;
    }
    if (SequenceActor != nullptr)
    {
        SequenceActor->Destroy();
        SequenceActor = nullptr;
    }
    if (CameraActor != nullptr)
    {
        CameraActor->Destroy();
        CameraActor = nullptr;
    }
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
    ++CompletedPlaybackCount;
}

ULevelSequence* UEchoesCinematicSubsystem::BuildReferenceSequence()
{
    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return nullptr;
    }

    // The camera the sequence possesses. Spawned collision-free and
    // shadow-free per the presentation rules; destroyed on finish.
    FActorSpawnParameters SpawnParameters;
    SpawnParameters.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    ACineCameraActor* Camera = World->SpawnActor<ACineCameraActor>(
        FVector(ReferenceStartX, ReferenceStartY, GetReferenceCameraHeight()),
        FRotator(ReferenceCameraPitch, 0.0f, 0.0f), SpawnParameters);
    if (Camera == nullptr)
    {
        return nullptr;
    }
    Camera->SetActorEnableCollision(false);

    ULevelSequence* Sequence = NewObject<ULevelSequence>(
        this, TEXT("EchoesReferenceSequence"), RF_Transient);
    Sequence->Initialize();

    UMovieScene* MovieScene = Sequence->GetMovieScene();
    MovieScene->SetDisplayRate(FFrameRate(CinematicFramesPerSecond, 1));
    const FFrameRate TickResolution = MovieScene->GetTickResolution();
    const FFrameNumber EndFrame =
        (GetReferenceDurationSeconds() * TickResolution).CeilToFrame();
    MovieScene->SetPlaybackRange(TRange<FFrameNumber>(FFrameNumber(0), EndFrame));

    const FGuid CameraBinding = MovieScene->AddPossessable(
        TEXT("EchoesReferenceCamera"), ACineCameraActor::StaticClass());
    Sequence->BindPossessableObject(CameraBinding, *Camera, World);

    // One measured move: transform track with start/end keys per channel.
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
        AddTravelKeys(Channels[0], EndFrame, ReferenceStartX, ReferenceEndX);
        AddTravelKeys(Channels[1], EndFrame, ReferenceStartY, ReferenceEndY);
        AddTravelKeys(Channels[2], EndFrame, GetReferenceCameraHeight(),
                      GetReferenceCameraHeight());
        AddTravelKeys(Channels[4], EndFrame, ReferenceCameraPitch,
                      ReferenceCameraPitch);
    }

    // The camera cut hands the view to the possessed camera for the full
    // playback range.
    UMovieSceneCameraCutTrack* CutTrack = Cast<UMovieSceneCameraCutTrack>(
        MovieScene->AddCameraCutTrack(UMovieSceneCameraCutTrack::StaticClass()));
    CutTrack->AddNewCameraCut(
        UE::MovieScene::FRelativeObjectBindingID(CameraBinding),
        FFrameNumber(0));

    CameraActor = Camera;
    return Sequence;
}
