#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "CineCameraActor.h"
#include "EchoesCinematicSubsystem.h"
#include "EchoesSimCore/Simulation.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/World.h"
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

    // Data-driven trigger path: the authored signal resolves to the
    // reference sequence, and unregistered signals resolve to nothing.
    const TOptional<EEchoesCinematicSequence> Resolved =
        UEchoesCinematicSubsystem::ResolveSequenceForSignal(
            TEXT("cinematic:reference"));
    TestTrue(TEXT("Reference signal resolves to a sequence"),
             Resolved.IsSet() &&
                 Resolved.GetValue() == EEchoesCinematicSequence::Reference);
    TestFalse(TEXT("Unregistered signals drive no sequence"),
              UEchoesCinematicSubsystem::ResolveSequenceForSignal(
                  TEXT("phase_entered:Ongoing"))
                  .IsSet());

    // Trigger and playback.
    TestFalse(TEXT("No sequence active before the trigger"),
              Cinematics->IsSequenceActive());
    TestTrue(TEXT("Reference sequence starts"),
             Cinematics->PlaySequence(Resolved.GetValue()));
    TestTrue(TEXT("Sequence reports active during playback"),
             Cinematics->IsSequenceActive());
    TestNotNull(TEXT("Playback owns a level sequence actor"),
                Cinematics->GetSequenceActorForTest());
    TestNotNull(TEXT("Playback owns the possessed camera"),
                Cinematics->GetCameraActorForTest());
    TestTrue(TEXT("Playback paused the running scenario"),
             Cinematics->WasScenarioPausedBySequenceForTest() &&
                 Bridge->IsScenarioPaused());
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
    TestFalse(TEXT("Return to play restores the unpaused scenario"),
              Bridge->IsScenarioPaused());

    // Presentation-only: the sequence wrote nothing into simulation state.
    const echoes::sim::Simulation* SimulationAfter = Bridge->GetSimulation();
    TestTrue(TEXT("Sequence playback mutated no simulation state"),
             SimulationAfter != nullptr &&
                 SimulationAfter->StateChecksum() == ChecksumBeforeSequence);

    // A paused scenario stays paused across a sequence: the pipeline
    // restores exactly the state it found.
    Bridge->SetScenarioPaused(true);
    TestTrue(TEXT("Second playback starts over a paused scenario"),
             Cinematics->PlaySequence(EEchoesCinematicSequence::Reference));
    TestFalse(TEXT("An already-paused scenario is not re-paused by playback"),
              Cinematics->WasScenarioPausedBySequenceForTest());
    Cinematics->SkipActiveSequence();
    // Verify signal resolution for all authored cinematics.
    const struct
    {
        const TCHAR* Signal;
        EEchoesCinematicSequence Expected;
        float ExpectedDuration;
    } AuthoredSequences[] = {
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
        TestTrue(*FString::Printf(TEXT("Signal %s resolves correctly"), Entry.Signal),
                 Res.IsSet() && Res.GetValue() == Entry.Expected);
        TestEqual(*FString::Printf(TEXT("Sequence %s duration matches"), Entry.Signal),
                  UEchoesCinematicSubsystem::GetSequenceDurationSeconds(Entry.Expected),
                  Entry.ExpectedDuration);

        // Verify each sequence can be spawned, played, and skipped cleanly
        TestTrue(*FString::Printf(TEXT("Sequence %s plays"), Entry.Signal),
                 Cinematics->PlaySequence(Entry.Expected));
        TestTrue(TEXT("Sequence reports active"), Cinematics->IsSequenceActive());
        Cinematics->SkipActiveSequence();
        TestFalse(TEXT("Sequence inactive after skip"), Cinematics->IsSequenceActive());
    }

    Bridge->StopPrototypeScenario();

    WorldWrapper.ForwardErrorMessages(this);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
