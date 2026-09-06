#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "EchoesPresentationAudioSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/App.h"
#include "Sound/SoundConcurrency.h"
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

    FEchoesScopedTestSaveEnvironment TestSaveEnvironment(*this);
    if (!TestSaveEnvironment.IsReady())
    {
        return false;
    }
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
    TestTrue(TEXT("All four authored SoundWave cues load"),
             Audio->HasAllAuthoredCueAssets());
    TestEqual(TEXT("Exactly four registered cues are loaded"),
              Audio->GetLoadedCueCount(),
              4);
    TestTrue(TEXT("Destruction cues use bounded spatial attenuation"),
             Audio->HasBoundedSpatialAttenuation());
    TestTrue(TEXT("Command and destruction use bounded concurrency policies"),
             Audio->HasBoundedConcurrencyPolicies());

    const USoundConcurrency* CommandConcurrency =
        Audio->GetConcurrencyPolicyForTest(
            EEchoesPresentationAudioCue::CommandConfirm);
    const USoundConcurrency* MeridianDestructionConcurrency =
        Audio->GetConcurrencyPolicyForTest(
            EEchoesPresentationAudioCue::DestructionMeridian);
    const USoundConcurrency* KharuunDestructionConcurrency =
        Audio->GetConcurrencyPolicyForTest(
            EEchoesPresentationAudioCue::DestructionKharuun);
    const USoundConcurrency* ChoirDestructionConcurrency =
        Audio->GetConcurrencyPolicyForTest(
            EEchoesPresentationAudioCue::DestructionChoir);
    if (!TestNotNull(TEXT("Command concurrency policy is available"),
                     CommandConcurrency) ||
        !TestNotNull(TEXT("Destruction concurrency policy is available"),
                     MeridianDestructionConcurrency))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    TestTrue(TEXT("Command concurrency policy is subsystem-owned"),
             CommandConcurrency->GetOuter() == Audio);
    TestTrue(TEXT("Destruction concurrency policy is subsystem-owned"),
             MeridianDestructionConcurrency->GetOuter() == Audio);
    TestTrue(TEXT("Command concurrency policy is transient"),
             CommandConcurrency->HasAnyFlags(RF_Transient));
    TestTrue(TEXT("Destruction concurrency policy is transient"),
             MeridianDestructionConcurrency->HasAnyFlags(RF_Transient));
    TestEqual(TEXT("Command concurrency has a hard voice cap"),
              CommandConcurrency->Concurrency.MaxCount,
              Audio->GetCommandMaxConcurrentVoices());
    TestFalse(TEXT("Command concurrency is global rather than owner-scoped"),
              CommandConcurrency->Concurrency.bLimitToOwner);
    TestEqual(
        TEXT("Command concurrency prevents excess voices"),
        static_cast<uint8>(CommandConcurrency->Concurrency.ResolutionRule),
        static_cast<uint8>(EMaxConcurrentResolutionRule::PreventNew));
    TestTrue(TEXT("Command concurrency disables engine retrigger retention"),
             CommandConcurrency->Concurrency.RetriggerTime == 0.0f);
    TestEqual(TEXT("Destruction concurrency has a hard voice cap"),
              MeridianDestructionConcurrency->Concurrency.MaxCount,
              Audio->GetDestructionMaxConcurrentVoices());
    TestFalse(TEXT("Destruction concurrency is global rather than owner-scoped"),
              MeridianDestructionConcurrency->Concurrency.bLimitToOwner);
    TestEqual(
        TEXT("Destruction concurrency resolves farthest then oldest"),
        static_cast<uint8>(
            MeridianDestructionConcurrency->Concurrency.ResolutionRule),
        static_cast<uint8>(
            EMaxConcurrentResolutionRule::StopFarthestThenOldest));
    TestTrue(TEXT("Destruction concurrency disables engine retrigger retention"),
             MeridianDestructionConcurrency->Concurrency.RetriggerTime ==
                 0.0f);
    TestTrue(TEXT("All faction destruction cues share one concurrency policy"),
             MeridianDestructionConcurrency == KharuunDestructionConcurrency &&
                 MeridianDestructionConcurrency == ChoirDestructionConcurrency);
    TestTrue(TEXT("Command and destruction concurrency remain independent"),
             CommandConcurrency != MeridianDestructionConcurrency);
    TestTrue(TEXT("Command admission window remains 80 ms"),
             FMath::IsNearlyEqual(
                 Audio->GetCommandCooldownSeconds(),
                 0.08f));
    TestTrue(TEXT("Destruction admission window remains 140 ms"),
             FMath::IsNearlyEqual(
                 Audio->GetDestructionCooldownSeconds(),
                 0.14f));

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
    TestFalse(TEXT("Choir destruction shares the same bounded destruction channel"),
              Audio->ReserveCueForTest(
                  EEchoesPresentationAudioCue::DestructionChoir,
                  10.20,
                  1.0f));
    TestTrue(TEXT("Choir destruction is admitted after the shared cooldown"),
             Audio->ReserveCueForTest(
                 EEchoesPresentationAudioCue::DestructionChoir,
                 10.302,
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
    Audio->ResetRateLimitsForTest();
    TestTrue(TEXT("The authored Choir destruction cue reaches spatial playback"),
             Audio->PlayDestruction(
                 echoes::sim::Faction::HollowChoir,
                 FVector::ZeroVector));
    TestEqual(TEXT("Rate-limit reset clears the command playback fixture"),
              Audio->GetSuccessfulCommandPlayCountForTest(),
              0);
    TestEqual(TEXT("One Choir destruction playback was recorded"),
              Audio->GetSuccessfulDestructionPlayCountForTest(),
              1);

    // Sound-enabled enforcement: the gate fails closed rather than passing
    // silently in a soundless session. A -nosound, commandlet-muted, or
    // null-device run must fail here, not report a hollow pass.
    if (!TestTrue(
            TEXT("[ECHOES_AUDIO_DEVICE_ABSENT] The process can render audio"),
            FApp::CanEverRenderAudio()) ||
        !TestNotNull(
            TEXT("[ECHOES_AUDIO_DEVICE_ABSENT] An audio device manager exists"),
            GEngine != nullptr ? GEngine->GetAudioDeviceManager() : nullptr) ||
        !TestTrue(
            TEXT("[ECHOES_AUDIO_DEVICE_ABSENT] The test world resolves a live audio device"),
            Audio->HasLiveAudioDeviceForTest()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    // Live-voice evidence: submission must leave an observable voice carrying
    // this subsystem's own policy object inside a live concurrency group.
    // The cue asset can be shared by unrelated systems on the audio device,
    // so those voices are outside this test's concurrency-policy boundary.
    Audio->FlushAllVoicesForTest();
    Audio->ResetRateLimitsForTest();
    TestTrue(TEXT("Command playback is admitted on the live device"),
             Audio->PlayCommandConfirmation());
    const UEchoesPresentationAudioSubsystem::FLiveVoiceEvidenceForTest
        CommandEvidence = Audio->GatherLiveVoiceEvidenceForTest(
            EEchoesPresentationAudioCue::CommandConfirm);
    TestEqual(TEXT("Exactly one policy-scoped command voice exists after submission"),
              CommandEvidence.MatchingActiveVoices,
              1);
    TestEqual(TEXT("The live command voice carries the subsystem policy"),
              CommandEvidence.VoicesCarryingSubsystemPolicy,
              1);
    TestEqual(TEXT("The live command voice joined a live concurrency group"),
              CommandEvidence.VoicesInsideLiveConcurrencyGroup,
              1);

    TestTrue(TEXT("Destruction playback is admitted on the live device"),
             Audio->PlayDestruction(
                 echoes::sim::Faction::KharuunAssemblies,
                 FVector(100.0f, 0.0f, 0.0f)));
    const UEchoesPresentationAudioSubsystem::FLiveVoiceEvidenceForTest
        DestructionEvidence = Audio->GatherLiveVoiceEvidenceForTest(
            EEchoesPresentationAudioCue::DestructionKharuun);
    TestEqual(TEXT("Exactly one policy-scoped destruction voice exists after submission"),
              DestructionEvidence.MatchingActiveVoices,
              1);
    TestEqual(TEXT("The live destruction voice carries the subsystem policy"),
              DestructionEvidence.VoicesCarryingSubsystemPolicy,
              1);
    TestEqual(TEXT("The live destruction voice joined a live concurrency group"),
              DestructionEvidence.VoicesInsideLiveConcurrencyGroup,
              1);

    // Lifecycle: flushing the device must release every voice this test
    // created; nothing may linger once its sound is stopped.
    Audio->FlushAllVoicesForTest();
    const UEchoesPresentationAudioSubsystem::FLiveVoiceEvidenceForTest
        FlushedCommandEvidence = Audio->GatherLiveVoiceEvidenceForTest(
            EEchoesPresentationAudioCue::CommandConfirm);
    const UEchoesPresentationAudioSubsystem::FLiveVoiceEvidenceForTest
        FlushedDestructionEvidence = Audio->GatherLiveVoiceEvidenceForTest(
            EEchoesPresentationAudioCue::DestructionKharuun);
    TestEqual(TEXT("Flushing releases every policy-scoped command voice"),
              FlushedCommandEvidence.MatchingActiveVoices,
              0);
    TestEqual(TEXT("Flushing releases every policy-scoped destruction voice"),
              FlushedDestructionEvidence.MatchingActiveVoices,
              0);
    return true;
}

#endif
