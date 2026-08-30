#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesGameUserSettings.h"
#include "Engine/Engine.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesGameUserSettingsTest,
    "Echoes.Runtime.Accessibility.GameUserSettings",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesGameUserSettingsTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    TestNotNull(TEXT("Engine is available"), GEngine);
    if (GEngine != nullptr)
    {
        TestTrue(
            TEXT("Echoes settings class is configured as the engine user-settings class"),
            GEngine->GameUserSettingsClass == UEchoesGameUserSettings::StaticClass());
    }

    UEchoesGameUserSettings* Settings =
        NewObject<UEchoesGameUserSettings>(GetTransientPackage());
    TestNotNull(TEXT("Echoes settings object can be constructed"), Settings);
    if (Settings == nullptr)
    {
        return false;
    }

    Settings->SetToDefaults();
    TestEqual(TEXT("Default HUD scale is 100%"), Settings->GetHudScale(), 1.0f);
    TestFalse(TEXT("High contrast defaults off"), Settings->IsHighContrastHudEnabled());
    TestFalse(TEXT("Reduced motion defaults off"), Settings->IsReducedMotionEnabled());
    TestFalse(TEXT("Reduced flashing defaults off"), Settings->IsReducedFlashingEnabled());
    TestTrue(TEXT("Edge pan defaults on"), Settings->IsEdgePanEnabled());
    TestEqual(TEXT("Effects volume defaults to 100%"),
              Settings->GetEffectsVolume(),
              1.0f);
    TestFalse(TEXT("Reduced dynamic range defaults off"),
              Settings->IsReducedDynamicRangeEnabled());

    Settings->SetHudScale(99.0f);
    Settings->SetCameraPanSpeedScale(0.01f);
    Settings->SetCameraZoomScale(99.0f);
    Settings->SetHighContrastHudEnabled(true);
    Settings->SetReducedMotionEnabled(true);
    Settings->SetReducedFlashingEnabled(true);
    Settings->SetEdgePanEnabled(false);
    Settings->SetEffectsVolume(99.0f);
    Settings->SetReducedDynamicRangeEnabled(true);
    Settings->ValidateSettings();

    TestEqual(TEXT("HUD scale is clamped"), Settings->GetHudScale(), 1.35f);
    TestEqual(TEXT("Pan speed scale is clamped"), Settings->GetCameraPanSpeedScale(), 0.5f);
    TestEqual(TEXT("Zoom scale is clamped"), Settings->GetCameraZoomScale(), 2.0f);
    TestTrue(TEXT("High contrast can be enabled"), Settings->IsHighContrastHudEnabled());
    TestTrue(TEXT("Reduced motion can be enabled"), Settings->IsReducedMotionEnabled());
    TestTrue(TEXT("Reduced flashing can be enabled"), Settings->IsReducedFlashingEnabled());
    TestFalse(TEXT("Edge pan can be disabled"), Settings->IsEdgePanEnabled());
    TestEqual(TEXT("Effects volume is clamped"),
              Settings->GetEffectsVolume(),
              1.0f);
    TestTrue(TEXT("Reduced dynamic range can be enabled"),
             Settings->IsReducedDynamicRangeEnabled());

    return true;
}

#endif
