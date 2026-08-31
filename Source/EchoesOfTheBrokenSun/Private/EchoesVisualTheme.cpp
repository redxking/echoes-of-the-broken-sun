#include "EchoesVisualTheme.h"

#include "UObject/UObjectGlobals.h"

FLinearColor FEchoesVisualTheme::WithAlpha(
    const FLinearColor& Color,
    const float Alpha) const
{
    return FLinearColor(Color.R, Color.G, Color.B, FMath::Clamp(Alpha, 0.0f, 1.0f));
}

FLinearColor FEchoesVisualTheme::StatusColor(
    const EEchoesVisualStatus Status) const
{
    switch (Status)
    {
        case EEchoesVisualStatus::Success: return Success;
        case EEchoesVisualStatus::Warning: return Warning;
        case EEchoesVisualStatus::Danger: return Danger;
        case EEchoesVisualStatus::Disabled: return Disabled;
        case EEchoesVisualStatus::Selected: return Selected;
        case EEchoesVisualStatus::Hostile: return Hostile;
        case EEchoesVisualStatus::MissionCritical: return MissionCritical;
        case EEchoesVisualStatus::Neutral:
        default: return Neutral;
    }
}

FLinearColor FEchoesVisualTheme::FactionColor(
    const EEchoesVisualFaction Faction) const
{
    switch (Faction)
    {
        case EEchoesVisualFaction::MeridianCompact: return Meridian;
        case EEchoesVisualFaction::KharuunAssemblies: return Kharuun;
        case EEchoesVisualFaction::HollowChoir: return Choir;
        case EEchoesVisualFaction::Neutral:
        default: return Neutral;
    }
}

FLinearColor FEchoesVisualTheme::OwnerColor(const uint8 Owner) const
{
    switch (Owner)
    {
        case 0: return Meridian;
        case 1: return Hostile;
        case 2: return Kharuun;
        case 3: return Choir;
        default: return Neutral;
    }
}

float FEchoesVisualTheme::RelativeLuminance(const FLinearColor& Color)
{
    return 0.2126f * FMath::Max(0.0f, Color.R) +
           0.7152f * FMath::Max(0.0f, Color.G) +
           0.0722f * FMath::Max(0.0f, Color.B);
}

float FEchoesVisualTheme::ContrastRatio(
    const FLinearColor& Foreground,
    const FLinearColor& Background)
{
    const float First = RelativeLuminance(Foreground);
    const float Second = RelativeLuminance(Background);
    return (FMath::Max(First, Second) + 0.05f) /
           (FMath::Min(First, Second) + 0.05f);
}

UEchoesVisualThemeSettings::UEchoesVisualThemeSettings()
{
    StandardCanvas = FLinearColor(0.002f, 0.006f, 0.016f, 1.0f);
    StandardSurface = FLinearColor(0.006f, 0.014f, 0.030f, 0.93f);
    StandardElevatedSurface = FLinearColor(0.014f, 0.032f, 0.058f, 0.985f);
    StandardScrim = FLinearColor(0.0f, 0.0f, 0.0f, 0.76f);
    StandardBorder = FLinearColor(0.11f, 0.32f, 0.40f, 0.92f);
    StandardAccent = FLinearColor(0.13f, 0.86f, 1.0f, 1.0f);
    StandardTextPrimary = FLinearColor(0.84f, 0.90f, 0.95f, 1.0f);
    StandardTextSecondary = FLinearColor(0.55f, 0.65f, 0.75f, 1.0f);
    StandardActionText = FLinearColor(0.0f, 0.035f, 0.06f, 1.0f);

    HighContrastCanvas = FLinearColor(0.0f, 0.0f, 0.0f, 1.0f);
    HighContrastSurface = FLinearColor(0.0f, 0.0f, 0.0f, 0.99f);
    HighContrastElevatedSurface = FLinearColor(0.0f, 0.0f, 0.0f, 1.0f);
    HighContrastScrim = FLinearColor(0.0f, 0.0f, 0.0f, 0.92f);
    HighContrastBorder = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
    HighContrastAccent = FLinearColor(1.0f, 0.90f, 0.10f, 1.0f);
    HighContrastTextPrimary = FLinearColor::White;
    HighContrastTextSecondary = FLinearColor(0.90f, 0.90f, 0.90f, 1.0f);
    HighContrastActionText = FLinearColor::Black;

    StandardSuccess = FLinearColor(0.24f, 1.0f, 0.62f, 1.0f);
    StandardWarning = FLinearColor(1.0f, 0.70f, 0.18f, 1.0f);
    StandardDanger = FLinearColor(1.0f, 0.32f, 0.18f, 1.0f);
    StandardDisabled = FLinearColor(0.34f, 0.40f, 0.48f, 1.0f);
    StandardSelected = FLinearColor(0.13f, 0.86f, 1.0f, 1.0f);
    StandardHostile = FLinearColor(1.0f, 0.30f, 0.10f, 1.0f);
    StandardNeutral = FLinearColor(0.70f, 0.73f, 0.78f, 1.0f);
    StandardMissionCritical = FLinearColor(1.0f, 0.82f, 0.24f, 1.0f);

    HighContrastSuccess = FLinearColor(0.25f, 1.0f, 0.55f, 1.0f);
    HighContrastWarning = FLinearColor(1.0f, 0.90f, 0.10f, 1.0f);
    HighContrastDanger = FLinearColor(1.0f, 0.45f, 0.15f, 1.0f);
    HighContrastDisabled = FLinearColor(0.72f, 0.72f, 0.72f, 1.0f);
    HighContrastSelected = FLinearColor(0.10f, 0.95f, 1.0f, 1.0f);
    HighContrastHostile = FLinearColor(1.0f, 0.35f, 0.12f, 1.0f);
    HighContrastNeutral = FLinearColor::White;
    HighContrastMissionCritical = FLinearColor(1.0f, 0.90f, 0.10f, 1.0f);

    Meridian = FLinearColor(0.08f, 0.78f, 0.96f, 1.0f);
    Kharuun = FLinearColor(1.0f, 0.66f, 0.14f, 1.0f);
    Choir = FLinearColor(0.78f, 0.38f, 1.0f, 1.0f);
    FutureWellDormant = FLinearColor(0.60f, 0.64f, 0.72f, 1.0f);
    FutureWellHarvest = FLinearColor(1.0f, 0.36f, 0.12f, 1.0f);
    FutureWellPreserve = FLinearColor(0.18f, 0.92f, 1.0f, 1.0f);
    FutureWellReshape = FLinearColor(0.82f, 0.42f, 1.0f, 1.0f);
}

FEchoesVisualTheme UEchoesVisualThemeSettings::BuildTheme(
    const bool bHighContrast) const
{
    FEchoesVisualTheme Theme;
    Theme.Canvas = bHighContrast ? HighContrastCanvas : StandardCanvas;
    Theme.Surface = bHighContrast ? HighContrastSurface : StandardSurface;
    Theme.ElevatedSurface = bHighContrast
        ? HighContrastElevatedSurface : StandardElevatedSurface;
    Theme.Scrim = bHighContrast ? HighContrastScrim : StandardScrim;
    Theme.Border = bHighContrast ? HighContrastBorder : StandardBorder;
    Theme.Accent = bHighContrast ? HighContrastAccent : StandardAccent;
    Theme.TextPrimary = bHighContrast
        ? HighContrastTextPrimary : StandardTextPrimary;
    Theme.TextSecondary = bHighContrast
        ? HighContrastTextSecondary : StandardTextSecondary;
    Theme.ActionText = bHighContrast
        ? HighContrastActionText : StandardActionText;
    Theme.Success = bHighContrast ? HighContrastSuccess : StandardSuccess;
    Theme.Warning = bHighContrast ? HighContrastWarning : StandardWarning;
    Theme.Danger = bHighContrast ? HighContrastDanger : StandardDanger;
    Theme.Disabled = bHighContrast ? HighContrastDisabled : StandardDisabled;
    Theme.Selected = bHighContrast ? HighContrastSelected : StandardSelected;
    Theme.Hostile = bHighContrast ? HighContrastHostile : StandardHostile;
    Theme.Neutral = bHighContrast ? HighContrastNeutral : StandardNeutral;
    Theme.MissionCritical = bHighContrast
        ? HighContrastMissionCritical : StandardMissionCritical;
    Theme.Meridian = bHighContrast
        ? FLinearColor(0.10f, 0.95f, 1.0f, 1.0f) : Meridian;
    Theme.Kharuun = bHighContrast
        ? FLinearColor(1.0f, 0.72f, 0.12f, 1.0f) : Kharuun;
    Theme.Choir = bHighContrast
        ? FLinearColor(0.94f, 0.58f, 1.0f, 1.0f) : Choir;
    Theme.FutureWellDormant = bHighContrast ? HighContrastNeutral : FutureWellDormant;
    Theme.FutureWellHarvest = bHighContrast ? HighContrastDanger : FutureWellHarvest;
    Theme.FutureWellPreserve = bHighContrast ? HighContrastSelected : FutureWellPreserve;
    Theme.FutureWellReshape = bHighContrast
        ? FLinearColor(0.94f, 0.58f, 1.0f, 1.0f) : FutureWellReshape;
    Theme.Grid = FMath::Max(1.0f, Grid);
    Theme.BorderThickness = FMath::Clamp(BorderThickness, 1.0f, 4.0f);
    Theme.EmphasisThickness = FMath::Clamp(EmphasisThickness, 2.0f, 8.0f);
    Theme.CornerLength = FMath::Clamp(CornerLength, 8.0f, 40.0f);
    return Theme;
}

FEchoesVisualTheme UEchoesVisualThemeSettings::Resolve(
    const bool bHighContrast)
{
    const UEchoesVisualThemeSettings* Settings =
        GetDefault<UEchoesVisualThemeSettings>();
    if (Settings != nullptr)
    {
        return Settings->BuildTheme(bHighContrast);
    }
    UEchoesVisualThemeSettings Fallback;
    return Fallback.BuildTheme(bHighContrast);
}
