#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "EchoesAudioMixSubsystem.h"
#include "EchoesGameUserSettings.h"
#include "EchoesInterfaceAudioSubsystem.h"
#include "Engine/World.h"
#include "Sound/SoundBase.h"
#include "Tests/AutomationCommon.h"

#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesInterfaceAudioTest,
    "Echoes.Runtime.Audio.InterfaceCues",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesInterfaceAudioTest::RunTest(const FString& Parameters)
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
        AddError(TEXT("Could not create the interface-audio test world."));
        return false;
    }
    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesInterfaceAudioSubsystem* Interface =
        World != nullptr
            ? World->GetSubsystem<UEchoesInterfaceAudioSubsystem>()
            : nullptr;
    if (!TestNotNull(TEXT("Interface-audio subsystem is available"),
                     Interface))
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
    const float RestoreInterface = Settings->GetInterfaceVolume();
    const float RestoreMaster = Settings->GetMasterVolume();
    Settings->SetInterfaceVolume(1.0f);
    Settings->SetMasterVolume(1.0f);

    // --- Registered cues load and route ------------------------------------

    TestTrue(TEXT("All twelve registered interface cues load"),
             Interface->HasAllAuthoredCues());
    TestEqual(TEXT("Interface cue count is twelve"),
              Interface->GetLoadedCueCount(),
              12);
    TestTrue(TEXT("Every cue's base submix is the interface submix"),
             Interface->HasInterfaceSubmixRouting());

    TSet<USoundBase*> Distinct;
    const EEchoesInterfaceCue AllInterface[] = {
        EEchoesInterfaceCue::Hover,        EEchoesInterfaceCue::Select,
        EEchoesInterfaceCue::Confirm,      EEchoesInterfaceCue::Reject,
        EEchoesInterfaceCue::MenuOpen,     EEchoesInterfaceCue::MenuClose,
        EEchoesInterfaceCue::BriefAdvance,
    };
    const EEchoesAlertCue AllAlerts[] = {
        EEchoesAlertCue::UnderAttack,
        EEchoesAlertCue::StructureLost,
        EEchoesAlertCue::ProductionComplete,
        EEchoesAlertCue::ResearchComplete,
        EEchoesAlertCue::CapacityLow,
    };
    for (const EEchoesInterfaceCue Cue : AllInterface)
    {
        Distinct.Add(Interface->ResolveInterfaceCue(Cue));
    }
    for (const EEchoesAlertCue Alert : AllAlerts)
    {
        Distinct.Add(Interface->ResolveAlertCue(Alert));
    }
    Distinct.Remove(nullptr);
    TestEqual(TEXT("The twelve events resolve to twelve distinct cues"),
              Distinct.Num(),
              12);

    // --- Interface admission windows ---------------------------------------

    Interface->ResetRateLimitsForTest();
    TestTrue(TEXT("A first select is admitted"),
             Interface->ReserveInterfaceCueForTest(
                 EEchoesInterfaceCue::Select, 10.0));
    TestFalse(TEXT("A same-cue burst inside 60 ms is rejected"),
              Interface->ReserveInterfaceCueForTest(
                  EEchoesInterfaceCue::Select, 10.02));
    TestTrue(TEXT("The cue reopens after its window"),
             Interface->ReserveInterfaceCueForTest(
                 EEchoesInterfaceCue::Select, 10.07));
    TestTrue(TEXT("A different interface cue has its own window"),
             Interface->ReserveInterfaceCueForTest(
                 EEchoesInterfaceCue::Reject, 10.02));
    TestFalse(TEXT("Non-finite time never reserves a voice"),
              Interface->ReserveInterfaceCueForTest(
                  EEchoesInterfaceCue::Hover,
                  std::numeric_limits<double>::quiet_NaN()));

    // --- Alert rate limiting under a simultaneous-event burst --------------

    Interface->ResetRateLimitsForTest();
    int32 AdmittedFirstBurst = 0;
    for (const EEchoesAlertCue Alert : AllAlerts)
    {
        if (Interface->ReserveAlertForTest(Alert, 20.0))
        {
            ++AdmittedFirstBurst;
        }
    }
    TestEqual(TEXT("A simultaneous burst admits one cue per alert class"),
              AdmittedFirstBurst,
              5);
    int32 AdmittedSecondBurst = 0;
    for (const EEchoesAlertCue Alert : AllAlerts)
    {
        if (Interface->ReserveAlertForTest(Alert, 21.5))
        {
            ++AdmittedSecondBurst;
        }
    }
    TestEqual(TEXT("An immediate repeat burst is fully rate-limited"),
              AdmittedSecondBurst,
              0);
    TestTrue(TEXT("An alert class reopens after its window"),
             Interface->ReserveAlertForTest(
                 EEchoesAlertCue::UnderAttack,
                 20.0 +
                     UEchoesInterfaceAudioSubsystem::GetAlertCooldownSeconds() +
                     0.01));

    // --- Muted interface volume blocks every cue ---------------------------

    Interface->ResetRateLimitsForTest();
    Settings->SetInterfaceVolume(0.0f);
    TestFalse(TEXT("A muted interface volume blocks interface cues"),
              Interface->ReserveInterfaceCueForTest(
                  EEchoesInterfaceCue::Confirm, 30.0));
    TestFalse(TEXT("A muted interface volume blocks alerts"),
              Interface->ReserveAlertForTest(
                  EEchoesAlertCue::StructureLost, 30.0));
    Settings->SetInterfaceVolume(1.0f);
    Settings->SetMasterVolume(0.0f);
    TestFalse(TEXT("A muted master blocks interface cues"),
              Interface->ReserveInterfaceCueForTest(
                  EEchoesInterfaceCue::Confirm, 31.0));
    Settings->SetMasterVolume(1.0f);

    // --- Playback reaches Unreal -------------------------------------------

    Interface->ResetRateLimitsForTest();
    TestTrue(TEXT("An interface cue reaches playback"),
             Interface->PlayInterfaceCue(EEchoesInterfaceCue::MenuOpen));
    TestTrue(TEXT("An alert reaches playback"),
             Interface->PlayAlert(EEchoesAlertCue::ResearchComplete));
    TestEqual(TEXT("One interface playback was recorded"),
              Interface->GetInterfacePlayCountForTest(),
              1);
    TestEqual(TEXT("One alert playback was recorded"),
              Interface->GetAlertPlayCountForTest(),
              1);

    Settings->SetInterfaceVolume(RestoreInterface);
    Settings->SetMasterVolume(RestoreMaster);
    return true;
}

#endif
