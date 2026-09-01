#include "EchoesVisualTheme.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesVisualThemeTest,
    "Echoes.Runtime.Presentation.VisualTheme",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEchoesVisualThemeTest::RunTest(const FString& Parameters)
{
    const FEchoesVisualTheme Standard =
        UEchoesVisualThemeSettings::Resolve(false);
    const FEchoesVisualTheme HighContrast =
        UEchoesVisualThemeSettings::Resolve(true);

    TestTrue(TEXT("Standard body text clears WCAG AA contrast on the HUD surface"),
             FEchoesVisualTheme::ContrastRatio(
                 Standard.TextPrimary, Standard.Surface) >= 4.5f);
    TestTrue(TEXT("Standard secondary text clears WCAG AA contrast on the HUD surface"),
             FEchoesVisualTheme::ContrastRatio(
                 Standard.TextSecondary, Standard.Surface) >= 4.5f);
    TestTrue(TEXT("Standard action text clears contrast on the primary action"),
             FEchoesVisualTheme::ContrastRatio(
                 Standard.ActionText, Standard.Accent) >= 4.5f);
    TestTrue(TEXT("High-contrast text clears enhanced contrast"),
             FEchoesVisualTheme::ContrastRatio(
                 HighContrast.TextPrimary, HighContrast.Surface) >= 7.0f);
    TestTrue(TEXT("High-contrast action text clears enhanced contrast"),
             FEchoesVisualTheme::ContrastRatio(
                 HighContrast.ActionText, HighContrast.Accent) >= 7.0f);

    TestFalse(TEXT("Success and warning remain distinct"),
              Standard.Success.Equals(Standard.Warning, 0.01f));
    TestFalse(TEXT("Warning and danger remain distinct"),
              Standard.Warning.Equals(Standard.Danger, 0.01f));
    TestFalse(TEXT("Meridian and Kharuun remain distinct"),
              Standard.Meridian.Equals(Standard.Kharuun, 0.01f));
    TestFalse(TEXT("Kharuun and Choir remain distinct"),
              Standard.Kharuun.Equals(Standard.Choir, 0.01f));
    TestFalse(TEXT("Owner glyph colors do not collapse in high contrast"),
              HighContrast.OwnerColor(0).Equals(
                  HighContrast.OwnerColor(1), 0.01f));

    const FLinearColor TransparentAccent = Standard.WithAlpha(Standard.Accent, -1.0f);
    const FLinearColor OpaqueAccent = Standard.WithAlpha(Standard.Accent, 2.0f);
    TestEqual(TEXT("Opacity helper clamps low"), TransparentAccent.A, 0.0f);
    TestEqual(TEXT("Opacity helper clamps high"), OpaqueAccent.A, 1.0f);
    TestEqual(TEXT("Opacity helper preserves red"), OpaqueAccent.R, Standard.Accent.R);
    TestEqual(TEXT("Opacity helper preserves green"), OpaqueAccent.G, Standard.Accent.G);
    TestEqual(TEXT("Opacity helper preserves blue"), OpaqueAccent.B, Standard.Accent.B);
    return true;
}

#endif
