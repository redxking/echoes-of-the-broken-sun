#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "EchoesAudioMixSubsystem.h"
#include "EchoesGameUserSettings.h"
#include "EchoesGameplayAudioSubsystem.h"
#include "Engine/World.h"
#include "Sound/SoundBase.h"
#include "Tests/AutomationCommon.h"

#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesGameplayAudioTest,
    "Echoes.Runtime.Audio.GameplayCues",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesGameplayAudioTest::RunTest(const FString& Parameters)
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
        AddError(TEXT("Could not create the gameplay-audio test world."));
        return false;
    }
    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesGameplayAudioSubsystem* Audio =
        World != nullptr
            ? World->GetSubsystem<UEchoesGameplayAudioSubsystem>()
            : nullptr;
    if (!TestNotNull(TEXT("Gameplay-audio subsystem is available"), Audio))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get();
    if (!TestNotNull(TEXT("Player settings are available"), Settings))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    const float RestoreEffects = Settings->GetEffectsVolume();
    const float RestoreMaster = Settings->GetMasterVolume();
    Settings->SetEffectsVolume(1.0f);
    Settings->SetMasterVolume(1.0f);

    // --- Coverage: every authoritative event has a registered, loaded cue --

    constexpr int32 EventCount =
        static_cast<int32>(EEchoesGameplayAudioEvent::Count);
    TestEqual(TEXT("The coverage contract enumerates eighteen events"),
              EventCount,
              18);
    TestTrue(TEXT("All eighteen registered gameplay cues load"),
             Audio->HasAllAuthoredCues());
    TestEqual(TEXT("Loaded cue count matches the event count"),
              Audio->GetLoadedCueCount(),
              EventCount);
    for (int32 Index = 0; Index < EventCount; ++Index)
    {
        const EEchoesGameplayAudioEvent Event =
            static_cast<EEchoesGameplayAudioEvent>(Index);
        TestNotNull(
            *FString::Printf(
                TEXT("Event %d resolves to a registered cue — no unmapped event"),
                Index),
            Audio->ResolveEventCue(Event));
        TestTrue(
            *FString::Printf(
                TEXT("Event %d carries a positive admission window"),
                Index),
            UEchoesGameplayAudioSubsystem::GetEventCooldownSeconds(Event) >
                0.0f);
    }
    TestTrue(TEXT("Every cue routes to the effects category submix"),
             Audio->HasEffectsSubmixRouting());
    TestTrue(TEXT("Spatial playback uses bounded attenuation"),
             Audio->HasBoundedSpatialAttenuation());

    // --- Event mapping helpers ---------------------------------------------

    TestEqual(TEXT("Scouts fire the light weapon"),
              static_cast<uint8>(
                  UEchoesGameplayAudioSubsystem::WeaponEventForType(
                      echoes::sim::EntityType::ScoutUnit)),
              static_cast<uint8>(
                  EEchoesGameplayAudioEvent::WeaponFireLight));
    TestEqual(TEXT("Soldiers fire the line weapon"),
              static_cast<uint8>(
                  UEchoesGameplayAudioSubsystem::WeaponEventForType(
                      echoes::sim::EntityType::Soldier)),
              static_cast<uint8>(EEchoesGameplayAudioEvent::WeaponFireLine));
    TestEqual(TEXT("Heavies fire the heavy weapon"),
              static_cast<uint8>(
                  UEchoesGameplayAudioSubsystem::WeaponEventForType(
                      echoes::sim::EntityType::HeavyUnit)),
              static_cast<uint8>(
                  EEchoesGameplayAudioEvent::WeaponFireHeavy));
    TestEqual(TEXT("Harvest commits play the Harvest cue"),
              static_cast<uint8>(
                  UEchoesGameplayAudioSubsystem::WellEventForChoice(
                      echoes::sim::FutureWellChoice::Harvest)),
              static_cast<uint8>(EEchoesGameplayAudioEvent::WellHarvest));
    TestEqual(TEXT("Preserve commits play the Preserve cue"),
              static_cast<uint8>(
                  UEchoesGameplayAudioSubsystem::WellEventForChoice(
                      echoes::sim::FutureWellChoice::Preserve)),
              static_cast<uint8>(EEchoesGameplayAudioEvent::WellPreserve));
    TestEqual(TEXT("Reshape commits play the Reshape cue"),
              static_cast<uint8>(
                  UEchoesGameplayAudioSubsystem::WellEventForChoice(
                      echoes::sim::FutureWellChoice::Reshape)),
              static_cast<uint8>(EEchoesGameplayAudioEvent::WellReshape));

    // --- Admission windows under combat load --------------------------------

    Audio->ResetRateLimitsForTest();
    TestTrue(TEXT("A first line-weapon shot is admitted"),
             Audio->ReserveEventForTest(
                 EEchoesGameplayAudioEvent::WeaponFireLine, 10.0));
    TestFalse(TEXT("A same-class shot burst is limited"),
              Audio->ReserveEventForTest(
                  EEchoesGameplayAudioEvent::WeaponFireLine, 10.03));
    TestTrue(TEXT("A different weapon class has its own window"),
             Audio->ReserveEventForTest(
                 EEchoesGameplayAudioEvent::WeaponFireHeavy, 10.03));
    TestTrue(TEXT("The weapon class reopens after its window"),
             Audio->ReserveEventForTest(
                 EEchoesGameplayAudioEvent::WeaponFireLine, 10.1));
    TestTrue(TEXT("Impacts are limited independently of weapons"),
             Audio->ReserveEventForTest(
                 EEchoesGameplayAudioEvent::ImpactHit, 10.03));
    TestFalse(TEXT("Non-finite time never reserves a voice"),
              Audio->ReserveEventForTest(
                  EEchoesGameplayAudioEvent::ImpactHit,
                  std::numeric_limits<double>::quiet_NaN()));

    // --- Muted effects block every event -----------------------------------

    Audio->ResetRateLimitsForTest();
    Settings->SetEffectsVolume(0.0f);
    TestFalse(TEXT("Muted effects block gameplay events"),
              Audio->ReserveEventForTest(
                  EEchoesGameplayAudioEvent::GatherMatter, 20.0));
    Settings->SetEffectsVolume(1.0f);
    Settings->SetMasterVolume(0.0f);
    TestFalse(TEXT("A muted master blocks gameplay events"),
              Audio->ReserveEventForTest(
                  EEchoesGameplayAudioEvent::GatherMatter, 21.0));
    Settings->SetMasterVolume(1.0f);

    // --- Playback reaches Unreal -------------------------------------------

    Audio->ResetRateLimitsForTest();
    TestTrue(TEXT("A spatial event reaches playback"),
             Audio->PlayEvent(
                 EEchoesGameplayAudioEvent::ConstructionComplete,
                 FVector::ZeroVector));
    TestTrue(TEXT("A 2D event reaches playback"),
             Audio->PlayEvent2D(EEchoesGameplayAudioEvent::ResearchStart));
    TestEqual(TEXT("Both playbacks were recorded"),
              Audio->GetPlayCountForTest(),
              2);

    Settings->SetEffectsVolume(RestoreEffects);
    Settings->SetMasterVolume(RestoreMaster);
    return true;
}

#endif
