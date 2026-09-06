#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "CineCameraActor.h"
#include "Camera/CameraActor.h"
#include "Camera/PlayerCameraManager.h"
#include "EchoesCinematicSubsystem.h"
#include "EchoesNarrativeSubsystem.h"
#include "EchoesSimCore/Simulation.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "LevelSequenceActor.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesCinematicPipelineTest,
    "Echoes.Runtime.Cinematics.ReferenceSequence",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesCinematicPipelineTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    FEchoesScopedTestSaveEnvironment TestSaveEnvironment(*this);
    if (!TestSaveEnvironment.IsReady())
    {
        return false;
    }

    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the temporary cinematic world."));
        return false;
    }

    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesCinematicSubsystem* Cinematics =
        World != nullptr ? World->GetSubsystem<UEchoesCinematicSubsystem>()
                         : nullptr;
    UEchoesSimulationSubsystem* Bridge =
        World != nullptr ? World->GetSubsystem<UEchoesSimulationSubsystem>()
                         : nullptr;
    if (!TestNotNull(TEXT("World owns the cinematic subsystem"), Cinematics) ||
        !TestNotNull(TEXT("World owns the simulation subsystem"), Bridge) ||
        !TestTrue(TEXT("Scenario starts"),
                  Bridge != nullptr && Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const echoes::sim::Simulation* Simulation = Bridge->GetSimulation();
    if (!TestNotNull(TEXT("Simulation is available"), Simulation))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    const uint64 ChecksumBeforeSequence = Simulation->StateChecksum();

    APlayerController* PlayerController =
        World->SpawnActor<APlayerController>();
    ACameraActor* OriginalCamera = World->SpawnActor<ACameraActor>();
    if (!TestNotNull(TEXT("A player controller can host camera cuts"),
                     PlayerController) ||
        !TestNotNull(TEXT("An original view target is available"),
                     OriginalCamera))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    // Test worlds do not run the normal player login/initialization path.
    // Register the controller that Sequencer actually enumerates and give it
    // a real camera manager before asserting camera/input ownership.
    World->AddController(PlayerController);
    if (!TestNotNull(TEXT("Local player requires an Engine outer"), GEngine))
    {
        return false;
    }
    PlayerController->Player = NewObject<ULocalPlayer>(GEngine);
    PlayerController->SetAsLocalPlayerController();
    PlayerController->InitInputSystem();
    if (!TestNotNull(TEXT("Cinematic fixture initializes player input"), PlayerController->PlayerInput.Get()))
    {
        return false;
    }
    if (!TestTrue(TEXT("Cinematic fixture is a local controller"),
                  PlayerController->IsLocalController()))
    {
        return false;
    }
    if (!PlayerController->PlayerCameraManager)
    {
        PlayerController->PlayerCameraManager = World->SpawnActor<APlayerCameraManager>();
        if (!TestNotNull(TEXT("Cinematic fixture owns a camera manager"), PlayerController->PlayerCameraManager.Get())) return false;
        PlayerController->PlayerCameraManager->InitializeFor(PlayerController);
    }
    PlayerController->SetViewTarget(OriginalCamera);
    TestTrue(TEXT("Fixture starts with its requested prior view target"), PlayerController->GetViewTarget() == OriginalCamera);

    // Data-driven trigger path: the authored signal resolves to the
    // reference sequence, and unregistered signals resolve to nothing.
    const TOptional<EEchoesCinematicSequence> Resolved =
        UEchoesCinematicSubsystem::ResolveSequenceForSignal(
            TEXT("cinematic:reference"));
    if (!TestTrue(TEXT("Reference signal resolves to a sequence"),
             Resolved.IsSet() &&
                 Resolved.GetValue() == EEchoesCinematicSequence::Reference))
    {
        return false;
    }
    TestFalse(TEXT("Unregistered signals drive no sequence"),
              UEchoesCinematicSubsystem::ResolveSequenceForSignal(
                  TEXT("phase_entered:Ongoing"))
                  .IsSet());

    // Trigger and playback.
    TestFalse(TEXT("No sequence active before the trigger"),
              Cinematics->IsSequenceActive());
    if (!TestTrue(TEXT("Reference sequence starts"),
             Cinematics->PlaySequence(Resolved.GetValue())))
    {
        return false;
    }
    TestTrue(TEXT("Sequence reports active during playback"),
             Cinematics->IsSequenceActive());
    TestTrue(TEXT("The active sequence identity is queryable"),
             Cinematics->IsSequenceActive(
                 EEchoesCinematicSequence::Reference));
    TestNotNull(TEXT("Playback owns a level sequence actor"),
                Cinematics->GetSequenceActorForTest());
    TestNotNull(TEXT("Playback owns the possessed camera"),
                Cinematics->GetCameraActorForTest());
    TestTrue(TEXT("Playback paused the running scenario"),
             Cinematics->WasScenarioPausedBySequenceForTest() &&
                 Bridge->IsScenarioPaused());
    TestTrue(TEXT("Playback suppresses movement input"),
             PlayerController->IsMoveInputIgnored());
    TestTrue(TEXT("Playback suppresses look input"),
             PlayerController->IsLookInputIgnored());
    TestFalse(TEXT("A second sequence cannot start over the first"),
              Cinematics->PlaySequence(EEchoesCinematicSequence::Reference));

    // Real playback: tick the world one simulated second and require the
    // possessed camera to have advanced along the authored travel keys.
    const ACineCameraActor* Camera = Cinematics->GetCameraActorForTest();
    const float CameraStartX =
        Camera != nullptr ? Camera->GetActorLocation().X : 0.0f;
    for (int32 TickIndex = 0; TickIndex < 20; ++TickIndex)
    {
        Cinematics->AdvanceActiveSequenceForTest(0.05f);
    }
    TestTrue(TEXT("The sequence is still active mid-playback"),
             Cinematics->IsSequenceActive());
    TestTrue(TEXT("Playback advances the possessed camera along its move"),
             Camera != nullptr &&
                 Camera->GetActorLocation().X > CameraStartX + 1.0f);

    // Skip and return to play.
    Cinematics->SkipActiveSequence();
    TestFalse(TEXT("Skip deactivates the sequence"),
              Cinematics->IsSequenceActive());
    TestNull(TEXT("Skip destroys the level sequence actor"),
             Cinematics->GetSequenceActorForTest());
    TestTrue(TEXT("Skip counts one completed playback"),
             Cinematics->GetCompletedPlaybackCountForTest() == 1);
    TestTrue(TEXT("Skip exposes a completed result"),
             Cinematics->HasSequenceCompleted(
                 EEchoesCinematicSequence::Reference) &&
                 Cinematics->GetLastCompletion() ==
                     EEchoesCinematicCompletion::Skipped);
    TestTrue(TEXT("Skip restores the prior view target"),
             PlayerController->GetViewTarget() == OriginalCamera);
    TestFalse(TEXT("Skip restores movement input"),
              PlayerController->IsMoveInputIgnored());
    TestFalse(TEXT("Skip restores look input"),
              PlayerController->IsLookInputIgnored());
    TestFalse(TEXT("Return to play restores the unpaused scenario"),
              Bridge->IsScenarioPaused());
    Cinematics->SkipActiveSequence();
    TestEqual(TEXT("Repeated skip cannot complete twice"),
              Cinematics->GetCompletedPlaybackCountForTest(),
              1);

    // Presentation-only: the sequence wrote nothing into simulation state.
    const echoes::sim::Simulation* SimulationAfter = Bridge->GetSimulation();
    TestTrue(TEXT("Sequence playback mutated no simulation state"),
             SimulationAfter != nullptr &&
                 SimulationAfter->StateChecksum() == ChecksumBeforeSequence);

    // A paused scenario stays paused across a sequence: the pipeline
    // restores exactly the state it found.
    Bridge->SetScenarioPaused(true);
    if (!TestTrue(TEXT("Second playback starts over a paused scenario"),
             Cinematics->PlaySequence(EEchoesCinematicSequence::Reference)))
    {
        return false;
    }
    TestFalse(TEXT("An already-paused scenario is not re-paused by playback"),
              Cinematics->WasScenarioPausedBySequenceForTest());
    Cinematics->SkipActiveSequence();
    TestTrue(TEXT("A sequence preserves an already-paused scenario"),
             Bridge->IsScenarioPaused());

    // The M01 sequence consumes the registered source contract and exposes
    // enough lifecycle state for the briefing controller to defer deployment.
    const TOptional<EEchoesCinematicSequence> M01Resolved =
        UEchoesCinematicSubsystem::ResolveSequenceForSignal(
            TEXT("nar_m01_cin_opening"));
    if (!TestTrue(TEXT("The canonical M01 cinematic id resolves"),
             M01Resolved.IsSet() &&
                 M01Resolved.GetValue() ==
                     EEchoesCinematicSequence::M01Opening))
    {
        return false;
    }
    if (!TestTrue(TEXT("M01 opening starts from the registered contract"),
             Cinematics->PlaySequence(
                 EEchoesCinematicSequence::M01Opening)))
    {
        return false;
    }
    TestEqual(TEXT("M01 opening uses the source-authored duration"),
              Cinematics->GetSequenceDurationSeconds(
                  EEchoesCinematicSequence::M01Opening),
              32.7f);
    TestEqual(TEXT("M01 opening builds all four authored shots"),
              Cinematics->GetCameraCutCountForTest(),
              4);
    UEchoesNarrativeSubsystem* Narrative =
        World->GetGameInstance() != nullptr
            ? World->GetGameInstance()->GetSubsystem<
                  UEchoesNarrativeSubsystem>()
            : nullptr;
    if (TestNotNull(TEXT("M01 opening has a subtitle clock"), Narrative))
    {
        Narrative->ClearSubtitleQueue();
        Narrative->EnqueueOperationStart(
            EEchoesOperationMode::CampaignPrologue,
            World->GetRealTimeSeconds());
    }
    Cinematics->AdvanceActiveSequenceForTest(2.0f);
    const double ElapsedBeforePause =
        Cinematics->GetSequenceElapsedSeconds();
    TestTrue(TEXT("M01 opening pauses in place"),
             Cinematics->SetSequencePaused(true));
    TestTrue(TEXT("M01 opening exposes paused state"),
             Cinematics->IsSequencePaused());
    TestTrue(TEXT("M01 pause also freezes subtitle playback"),
             Narrative != nullptr &&
                 Narrative->IsSubtitlePlaybackPaused());
    Cinematics->AdvanceActiveSequenceForTest(5.0f);
    Cinematics->Tick(5.0f);
    TestTrue(TEXT("Paused M01 remains active"),
             Cinematics->IsSequenceActive(
                 EEchoesCinematicSequence::M01Opening));
    TestTrue(TEXT("Pause holds accumulated sequence elapsed time"),
             FMath::IsNearlyEqual(
                 Cinematics->GetSequenceElapsedSeconds(),
                 ElapsedBeforePause));
    TestTrue(TEXT("M01 pause keeps the simulation paused"),
             Bridge->IsScenarioPaused());
    TestTrue(TEXT("M01 opening resumes from its held frame"),
             Cinematics->SetSequencePaused(false));
    TestFalse(TEXT("M01 opening clears paused state on resume"),
              Cinematics->IsSequencePaused());
    TestFalse(TEXT("M01 resume releases the subtitle clock"),
              Narrative != nullptr &&
                  Narrative->IsSubtitlePlaybackPaused());
    Cinematics->AdvanceActiveSequenceForTest(
        Cinematics->GetSequenceDurationSeconds(EEchoesCinematicSequence::M01Opening) -
        static_cast<float>(Cinematics->GetSequenceElapsedSeconds()) + 0.1f);
    Cinematics->Tick(0.0f);
    TestFalse(TEXT("M01 natural finish clears active state"),
              Cinematics->IsSequenceActive());
    TestTrue(TEXT("M01 natural finish is distinguishable from skip"),
             Cinematics->HasSequenceCompleted(
                 EEchoesCinematicSequence::M01Opening) &&
                 Cinematics->GetLastCompletion() ==
                     EEchoesCinematicCompletion::NaturalFinish);
    TestTrue(TEXT("M01 natural finish preserves briefing pause"),
             Bridge->IsScenarioPaused());
    TestTrue(TEXT("M01 natural finish restores the prior view target"),
             PlayerController->GetViewTarget() == OriginalCamera);
    TestFalse(TEXT("M01 natural finish restores movement input"),
              PlayerController->IsMoveInputIgnored());
    TestFalse(TEXT("M01 natural finish restores look input"),
              PlayerController->IsLookInputIgnored());

    const int32 CompletionsBeforeFailure =
        Cinematics->GetCompletedPlaybackCountForTest();
    TestFalse(TEXT("An unknown sequence type fails closed"),
              Cinematics->PlaySequence(
                  static_cast<EEchoesCinematicSequence>(255)));
    TestTrue(TEXT("Start failure is observable by the controller"),
             Cinematics->GetLastCompletion() ==
                 EEchoesCinematicCompletion::FailedToStart);
    TestEqual(TEXT("Start failure does not count as completed playback"),
              Cinematics->GetCompletedPlaybackCountForTest(),
              CompletionsBeforeFailure);
    TestTrue(TEXT("Start failure does not unpause the scenario"),
             Bridge->IsScenarioPaused());
    TestTrue(TEXT("Start failure preserves the view target"),
             PlayerController->GetViewTarget() == OriginalCamera);
    // Verify signal resolution for all authored cinematics.
    const struct
    {
        const TCHAR* Signal;
        EEchoesCinematicSequence Expected;
        float ExpectedDuration;
    } AuthoredSequences[] = {
        { TEXT("nar_m01_cin_opening"),
          EEchoesCinematicSequence::M01Opening,
          32.7f },
        { TEXT("cinematic:title"), EEchoesCinematicSequence::TitleSequence, 72.0f },
        { TEXT("cinematic:act1_to_act2"), EEchoesCinematicSequence::Act1ToAct2Transition, 28.0f },
        { TEXT("cinematic:act2_to_act3"), EEchoesCinematicSequence::Act2ToAct3Transition, 30.0f },
        { TEXT("cinematic:act3_climax"), EEchoesCinematicSequence::Act3ClimaxTransition, 24.0f },
        { TEXT("cinematic:ending_restoration"), EEchoesCinematicSequence::EndingRestoration, 32.0f },
        { TEXT("cinematic:ending_controlled_stabilization"), EEchoesCinematicSequence::EndingControlledStabilization, 32.0f },
        { TEXT("cinematic:ending_extinguishment"), EEchoesCinematicSequence::EndingExtinguishment, 32.0f },
        { TEXT("cinematic:ending_open_evolution"), EEchoesCinematicSequence::EndingOpenEvolution, 32.0f },
    };

    for (const auto& Entry : AuthoredSequences)
    {
        const TOptional<EEchoesCinematicSequence> Res =
            UEchoesCinematicSubsystem::ResolveSequenceForSignal(Entry.Signal);
        if (!TestTrue(*FString::Printf(TEXT("Signal %s resolves correctly"), Entry.Signal),
                 Res.IsSet() && Res.GetValue() == Entry.Expected)) return false;
        TestEqual(*FString::Printf(TEXT("Sequence %s duration matches"), Entry.Signal),
                  Cinematics->GetSequenceDurationSeconds(Entry.Expected),
                  Entry.ExpectedDuration);

        // Verify each sequence can be spawned, played, and skipped cleanly
        if (!TestTrue(*FString::Printf(TEXT("Sequence %s plays"), Entry.Signal),
                 Cinematics->PlaySequence(Entry.Expected))) return false;
        TestTrue(TEXT("Sequence reports active"), Cinematics->IsSequenceActive());
        if (Entry.Expected == EEchoesCinematicSequence::M01Opening)
        {
            TestTrue(TEXT("M01 can be paused before an explicit skip"),
                     Cinematics->SetSequencePaused(true));
        }
        Cinematics->SkipActiveSequence();
        TestFalse(TEXT("Sequence inactive after skip"), Cinematics->IsSequenceActive());
        TestFalse(TEXT("Skip clears paused sequence state"),
                  Cinematics->IsSequencePaused());
        TestFalse(TEXT("Skip releases a paused subtitle clock"),
                  Narrative != nullptr &&
                      Narrative->IsSubtitlePlaybackPaused());
        TestTrue(TEXT("Sequence skip restores the prior view target"),
                 PlayerController->GetViewTarget() == OriginalCamera);
        TestFalse(TEXT("Sequence skip restores movement input"),
                  PlayerController->IsMoveInputIgnored());
        TestFalse(TEXT("Sequence skip restores look input"),
                  PlayerController->IsLookInputIgnored());
    }

    Bridge->StopPrototypeScenario();

    WorldWrapper.ForwardErrorMessages(this);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
