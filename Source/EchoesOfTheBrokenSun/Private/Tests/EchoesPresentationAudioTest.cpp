#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesPresentationAudioSubsystem.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesPresentationAudioTest,
    "Echoes.Runtime.Presentation.AudioConfirmation",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesPresentationAudioTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the presentation-audio test world."));
        return false;
    }
    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesPresentationAudioSubsystem* Audio =
        World != nullptr
            ? World->GetSubsystem<UEchoesPresentationAudioSubsystem>()
            : nullptr;
    if (!TestNotNull(TEXT("Presentation-audio subsystem is available"), Audio))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    TestTrue(TEXT("All three authored SoundWave cues load"),
             Audio->HasAllAuthoredCueAssets());
    TestEqual(TEXT("Exactly three registered cues are loaded"),
              Audio->GetLoadedCueCount(),
              3);
    TestTrue(TEXT("Destruction cues use bounded spatial attenuation"),
             Audio->HasBoundedSpatialAttenuation());

    Audio->ResetRateLimitsForTest();
    TestTrue(TEXT("First accepted command cue is admitted"),
             Audio->ReserveCueForTest(
                 EEchoesPresentationAudioCue::CommandConfirm,
                 10.0,
                 1.0f));
    TestFalse(TEXT("Command bursts are rate-limited"),
              Audio->ReserveCueForTest(
                  EEchoesPresentationAudioCue::CommandConfirm,
                  10.02,
                  1.0f));
    TestTrue(TEXT("Command cue reopens after its cooldown"),
             Audio->ReserveCueForTest(
                 EEchoesPresentationAudioCue::CommandConfirm,
                 10.081,
                 1.0f));
    TestTrue(TEXT("Destruction uses an independent bounded channel"),
             Audio->ReserveCueForTest(
                 EEchoesPresentationAudioCue::DestructionMeridian,
                 10.02,
                 1.0f));
    TestFalse(TEXT("Faction destruction cues share the destruction limiter"),
              Audio->ReserveCueForTest(
                  EEchoesPresentationAudioCue::DestructionKharuun,
                  10.08,
                  1.0f));
    TestTrue(TEXT("Destruction cue reopens after its cooldown"),
             Audio->ReserveCueForTest(
                 EEchoesPresentationAudioCue::DestructionKharuun,
                 10.161,
                 1.0f));
    TestFalse(TEXT("Muted effects never reserve a voice"),
              Audio->ReserveCueForTest(
                  EEchoesPresentationAudioCue::CommandConfirm,
                  12.0,
                  0.0f));
    TestFalse(TEXT("Invalid time never reserves a voice"),
              Audio->ReserveCueForTest(
                  EEchoesPresentationAudioCue::CommandConfirm,
                  std::numeric_limits<double>::quiet_NaN(),
                  1.0f));

    const float StandardCommand = Audio->GetCueVolumeForTest(
        EEchoesPresentationAudioCue::CommandConfirm, 1.0f, false);
    const float StandardDestruction = Audio->GetCueVolumeForTest(
        EEchoesPresentationAudioCue::DestructionMeridian, 1.0f, false);
    const float ReducedCommand = Audio->GetCueVolumeForTest(
        EEchoesPresentationAudioCue::CommandConfirm, 1.0f, true);
    const float ReducedDestruction = Audio->GetCueVolumeForTest(
        EEchoesPresentationAudioCue::DestructionMeridian, 1.0f, true);
    TestTrue(TEXT("Standard destruction remains more prominent than confirmation"),
             StandardDestruction > StandardCommand);
    TestTrue(TEXT("Reduced dynamic range narrows cue-level separation"),
             (ReducedDestruction - ReducedCommand) <
                 (StandardDestruction - StandardCommand));
    TestTrue(TEXT("Effects volume scales cues linearly"),
             FMath::IsNearlyEqual(
                 Audio->GetCueVolumeForTest(
                     EEchoesPresentationAudioCue::CommandConfirm,
                     0.5f,
                     false),
                 StandardCommand * 0.5f));

    Audio->ResetRateLimitsForTest();
    TestTrue(TEXT("The authored command cue reaches Unreal playback"),
             Audio->PlayCommandConfirmation());
    TestTrue(TEXT("The authored Meridian destruction cue reaches spatial playback"),
             Audio->PlayDestruction(
                 echoes::sim::Faction::MeridianCompact,
                 FVector::ZeroVector));
    TestEqual(TEXT("One command playback was recorded"),
              Audio->GetSuccessfulCommandPlayCountForTest(),
              1);
    TestEqual(TEXT("One destruction playback was recorded"),
              Audio->GetSuccessfulDestructionPlayCountForTest(),
              1);
    return true;
}

#endif
