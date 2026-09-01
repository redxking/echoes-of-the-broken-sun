#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "EchoesVisualTheme.generated.h"

/** Semantic presentation roles. Gameplay state must select a role, not a raw color. */
enum class EEchoesVisualStatus : uint8
{
    Neutral,
    Success,
    Warning,
    Danger,
    Disabled,
    Selected,
    Hostile,
    MissionCritical,
};

/** Stable visual identities used by scalable HUD glyphs and palette variants. */
enum class EEchoesVisualFaction : uint8
{
    Neutral,
    MeridianCompact,
    KharuunAssemblies,
    HollowChoir,
};

/** Centralized runtime tokens for the player-facing Canvas presentation. */
struct ECHOESOFTHEBROKENSUN_API FEchoesVisualTheme final
{
    FLinearColor Canvas;
    FLinearColor Surface;
    FLinearColor ElevatedSurface;
    FLinearColor Scrim;
    FLinearColor Border;
    FLinearColor Accent;
    FLinearColor TextPrimary;
    FLinearColor TextSecondary;
    FLinearColor ActionText;
    FLinearColor Success;
    FLinearColor Warning;
    FLinearColor Danger;
    FLinearColor Disabled;
    FLinearColor Selected;
    FLinearColor Hostile;
    FLinearColor Neutral;
    FLinearColor MissionCritical;
    FLinearColor Meridian;
    FLinearColor Kharuun;
    FLinearColor Choir;
    FLinearColor FutureWellDormant;
    FLinearColor FutureWellHarvest;
    FLinearColor FutureWellPreserve;
    FLinearColor FutureWellReshape;

    float Grid = 4.0f;
    float BorderThickness = 1.5f;
    float EmphasisThickness = 3.0f;
    float CornerLength = 16.0f;

    [[nodiscard]] FLinearColor WithAlpha(
        const FLinearColor& Color,
        float Alpha) const;
    [[nodiscard]] FLinearColor StatusColor(EEchoesVisualStatus Status) const;
    [[nodiscard]] FLinearColor FactionColor(EEchoesVisualFaction Faction) const;
    [[nodiscard]] FLinearColor OwnerColor(uint8 Owner) const;

    [[nodiscard]] static float RelativeLuminance(const FLinearColor& Color);
    [[nodiscard]] static float ContrastRatio(
        const FLinearColor& Foreground,
        const FLinearColor& Background);
};

/**
 * Packaged visual tokens. Values live in DefaultGame.ini so art direction can
 * be tuned without duplicating constants across HUD screens.
 */
UCLASS(Config=Game, DefaultConfig)
class ECHOESOFTHEBROKENSUN_API UEchoesVisualThemeSettings final : public UObject
{
    GENERATED_BODY()

public:
    UEchoesVisualThemeSettings();

    [[nodiscard]] FEchoesVisualTheme BuildTheme(bool bHighContrast) const;
    [[nodiscard]] static FEchoesVisualTheme Resolve(bool bHighContrast);

private:
    UPROPERTY(Config)
    FLinearColor StandardCanvas;
    UPROPERTY(Config)
    FLinearColor StandardSurface;
    UPROPERTY(Config)
    FLinearColor StandardElevatedSurface;
    UPROPERTY(Config)
    FLinearColor StandardScrim;
    UPROPERTY(Config)
    FLinearColor StandardBorder;
    UPROPERTY(Config)
    FLinearColor StandardAccent;
    UPROPERTY(Config)
    FLinearColor StandardTextPrimary;
    UPROPERTY(Config)
    FLinearColor StandardTextSecondary;
    UPROPERTY(Config)
    FLinearColor StandardActionText;

    UPROPERTY(Config)
    FLinearColor HighContrastCanvas;
    UPROPERTY(Config)
    FLinearColor HighContrastSurface;
    UPROPERTY(Config)
    FLinearColor HighContrastElevatedSurface;
    UPROPERTY(Config)
    FLinearColor HighContrastScrim;
    UPROPERTY(Config)
    FLinearColor HighContrastBorder;
    UPROPERTY(Config)
    FLinearColor HighContrastAccent;
    UPROPERTY(Config)
    FLinearColor HighContrastTextPrimary;
    UPROPERTY(Config)
    FLinearColor HighContrastTextSecondary;
    UPROPERTY(Config)
    FLinearColor HighContrastActionText;

    UPROPERTY(Config)
    FLinearColor StandardSuccess;
    UPROPERTY(Config)
    FLinearColor StandardWarning;
    UPROPERTY(Config)
    FLinearColor StandardDanger;
    UPROPERTY(Config)
    FLinearColor StandardDisabled;
    UPROPERTY(Config)
    FLinearColor StandardSelected;
    UPROPERTY(Config)
    FLinearColor StandardHostile;
    UPROPERTY(Config)
    FLinearColor StandardNeutral;
    UPROPERTY(Config)
    FLinearColor StandardMissionCritical;

    UPROPERTY(Config)
    FLinearColor HighContrastSuccess;
    UPROPERTY(Config)
    FLinearColor HighContrastWarning;
    UPROPERTY(Config)
    FLinearColor HighContrastDanger;
    UPROPERTY(Config)
    FLinearColor HighContrastDisabled;
    UPROPERTY(Config)
    FLinearColor HighContrastSelected;
    UPROPERTY(Config)
    FLinearColor HighContrastHostile;
    UPROPERTY(Config)
    FLinearColor HighContrastNeutral;
    UPROPERTY(Config)
    FLinearColor HighContrastMissionCritical;

    UPROPERTY(Config)
    FLinearColor Meridian;
    UPROPERTY(Config)
    FLinearColor Kharuun;
    UPROPERTY(Config)
    FLinearColor Choir;
    UPROPERTY(Config)
    FLinearColor FutureWellDormant;
    UPROPERTY(Config)
    FLinearColor FutureWellHarvest;
    UPROPERTY(Config)
    FLinearColor FutureWellPreserve;
    UPROPERTY(Config)
    FLinearColor FutureWellReshape;

    UPROPERTY(Config)
    float Grid = 4.0f;
    UPROPERTY(Config)
    float BorderThickness = 1.5f;
    UPROPERTY(Config)
    float EmphasisThickness = 3.0f;
    UPROPERTY(Config)
    float CornerLength = 16.0f;
};
