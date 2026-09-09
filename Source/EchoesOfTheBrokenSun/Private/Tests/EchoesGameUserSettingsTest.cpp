#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "EchoesGameUserSettings.h"
#include "EchoesHudLayout.h"
#include "EchoesPointerCombatGuardReview.h"
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

    FEchoesScopedTestSaveEnvironment TestSaveEnvironment(*this);
    if (!TestSaveEnvironment.IsReady())
    {
        return false;
    }

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

    // ACC/UI requires an 80-150% HUD scale range. This expectation carried the
    // pre-correction 135% ceiling; re-derived, not relaxed.
    TestEqual(TEXT("HUD scale is clamped"), Settings->GetHudScale(), 1.50f);
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

    const FVector2D ReviewViewport(1600.0f, 900.0f);
    const FEchoesHudLayout MaximumLayout =
        FEchoesHudLayout::Build(ReviewViewport, 1.35f, true);
    TestTrue(TEXT("Maximum-scale objectives are inside the lower console"),
        MaximumLayout.ObjectivePanel.Min.Y >= MaximumLayout.BottomBar.Min.Y &&
        MaximumLayout.ObjectivePanel.Max.Y + 14 <= MaximumLayout.SelectionPanel.Min.Y);
    TestTrue(TEXT("Maximum-scale command card is inside the console to the right"),
        MaximumLayout.bCommandDeckVisible &&
        MaximumLayout.CommandDeckPanel.Min.Y >= MaximumLayout.BottomBar.Min.Y &&
        MaximumLayout.CommandDeckPanel.Max.Y <= MaximumLayout.BottomBar.Max.Y &&
        MaximumLayout.CommandDeckPanel.Min.X >= MaximumLayout.SelectionPanel.Max.X + 14);
    TestTrue(TEXT("Maximum-scale minimap is square and left of selection"),
        MaximumLayout.bMinimapVisible &&
        FMath::IsNearlyEqual(MaximumLayout.MinimapPanel.GetSize().X, MaximumLayout.MinimapPanel.GetSize().Y) &&
        MaximumLayout.MinimapPanel.Max.X + 14 <= MaximumLayout.SelectionPanel.Min.X);
    TestTrue(TEXT("Status remains above and clear of the console"),
        MaximumLayout.bStatusVisible && MaximumLayout.StatusPanel.Max.Y < MaximumLayout.BottomBar.Min.Y);
    TestTrue(TEXT("Compact global resources remain inside the viewport and clear of the console"),
        MaximumLayout.bResourceVisible && MaximumLayout.ResourcePanel.Min.X >= 0 &&
        MaximumLayout.ResourcePanel.Min.Y >= 0 &&
        MaximumLayout.ResourcePanel.Max.X <= ReviewViewport.X - 220 &&
        MaximumLayout.ResourcePanel.Max.Y < MaximumLayout.BottomBar.Min.Y);
    TestFalse(TEXT("Compact global resources leave the battlefield centre clear"),
        MaximumLayout.ResourcePanel.IsInside(ReviewViewport * .5f));

    const FEchoesHudLayout CompactMaximumLayout =
        FEchoesHudLayout::Build(FVector2D(1280.0f, 720.0f), 1.35f, true);
    TestTrue(
        TEXT("Compact maximum-scale objective remains visible"),
        CompactMaximumLayout.bObjectiveVisible);
    TestTrue(
        TEXT("Compact maximum-scale command card still fits the bar"),
        CompactMaximumLayout.bCommandDeckVisible &&
            CompactMaximumLayout.bMinimapVisible &&
            CompactMaximumLayout.MinimapPanel.Max.X + 14.0f <=
                CompactMaximumLayout.CommandDeckPanel.Min.X);

    const FEchoesHudLayout DefaultLayout =
        FEchoesHudLayout::Build(ReviewViewport, 1.0f, true);
    TestFalse(
        TEXT("Battlefield visibility rejects a target behind the command strip"),
        DefaultLayout.IsBattlefieldPointClear(
            DefaultLayout.MainPanel.GetCenter(), ReviewViewport));
    TestFalse(
        TEXT("Battlefield visibility rejects a target behind the bottom bar"),
        DefaultLayout.IsBattlefieldPointClear(
            DefaultLayout.BottomBar.GetCenter(), ReviewViewport));
    TestTrue(
        TEXT("Battlefield visibility accepts a clear default target"),
        DefaultLayout.IsBattlefieldPointClear(
            FVector2D(997.9f, 191.8f), ReviewViewport));
    TestTrue(
        TEXT("Battlefield visibility accepts a clear maximum-scale target"),
        MaximumLayout.IsBattlefieldPointClear(
            FVector2D(1484.3f, 166.4f), ReviewViewport));
    TestFalse(
        TEXT("Battlefield visibility rejects actor bounds crossing the viewport edge"),
        DefaultLayout.IsBattlefieldBoxClear(
            FBox2D(
                FVector2D(1560.0f, 320.0f),
                FVector2D(1592.0f, 360.0f)),
            ReviewViewport));
    TestTrue(
        TEXT("Battlefield visibility accepts complete actor bounds in clear field space"),
        DefaultLayout.IsBattlefieldBoxClear(
            FBox2D(
                FVector2D(980.0f, 310.0f),
                FVector2D(1020.0f, 350.0f)),
            ReviewViewport));

    FEchoesPointerCombatGuardReview ReviewConfiguration;
    TestTrue(
        TEXT("All bounded pointer-review variants resolve"),
        FEchoesPointerCombatGuardReview::TryResolve(
            TEXT("Default"), ReviewConfiguration) &&
        FEchoesPointerCombatGuardReview::TryResolve(
            TEXT("Offset"), ReviewConfiguration) &&
        FEchoesPointerCombatGuardReview::TryResolve(
            TEXT("MinHud"), ReviewConfiguration) &&
        FEchoesPointerCombatGuardReview::TryResolve(
            TEXT("MaxHud"), ReviewConfiguration) &&
        FEchoesPointerCombatGuardReview::TryResolve(
            TEXT("Compact"), ReviewConfiguration) &&
        FEchoesPointerCombatGuardReview::TryResolve(
            TEXT("Mac16x10"), ReviewConfiguration) &&
        FEchoesPointerCombatGuardReview::TryResolve(
            TEXT("FullHD"), ReviewConfiguration));
    TestFalse(
        TEXT("Unknown pointer-review variants are rejected"),
        FEchoesPointerCombatGuardReview::TryResolve(
            TEXT("Unknown"), ReviewConfiguration));

    return true;
}

#endif
