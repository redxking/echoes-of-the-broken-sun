#include "EchoesWeatherView.h"

#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SceneComponent.h"
#include "EchoesGameUserSettings.h"

namespace
{
constexpr float DriftCycleSeconds = 28.0f;
}

AEchoesWeatherView::AEchoesWeatherView()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.1f;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    HeightFog = CreateDefaultSubobject<UExponentialHeightFogComponent>(
        TEXT("GlassScarAtmosphere"));
    HeightFog->SetupAttachment(SceneRoot);
    HeightFog->SetFogHeightFalloff(0.22f);
    HeightFog->SetFogMaxOpacity(0.20f);
    HeightFog->SetStartDistance(900.0f);
    HeightFog->SetVolumetricFog(false);

    Tags.Add(TEXT("EchoesPlaceholder"));
    Tags.Add(TEXT("EchoesWeatherView"));
    ApplyAtmosphere(0.5f);
}

void AEchoesWeatherView::ApplyMapPreset(
    EEchoesSkirmishMapPreset MapPreset)
{
    ActiveMapPreset = MapPreset;
    ApplyAtmosphere(0.5f);
}

void AEchoesWeatherView::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    const UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get();
    if (Settings != nullptr && Settings->IsReducedMotionEnabled())
    {
        ApplyAtmosphere(0.5f);
        return;
    }

    DriftPhaseSeconds = FMath::Fmod(
        DriftPhaseSeconds + FMath::Max(0.0f, DeltaSeconds),
        DriftCycleSeconds);
    const float PhaseRadians =
        DriftPhaseSeconds / DriftCycleSeconds * 2.0f * PI;
    ApplyAtmosphere(0.5f + 0.5f * FMath::Sin(PhaseRadians));
}

void AEchoesWeatherView::ApplyAtmosphere(float NormalizedDrift)
{
    const float Drift = FMath::Clamp(NormalizedDrift, 0.0f, 1.0f);
    float BaseDensity = 0.0007f;
    float DensityRange = 0.00035f;
    FLinearColor LowColor(0.035f, 0.075f, 0.13f);
    FLinearColor HighColor(0.15f, 0.055f, 0.19f);
    float HeightFalloff = 0.22f;
    float StartDistance = 900.0f;
    if (ActiveMapPreset == EEchoesSkirmishMapPreset::CrownfallBasin)
    {
        BaseDensity = 0.00052f;
        DensityRange = 0.00024f;
        LowColor = FLinearColor(0.055f, 0.11f, 0.12f);
        HighColor = FLinearColor(0.20f, 0.14f, 0.055f);
        HeightFalloff = 0.29f;
        StartDistance = 720.0f;
    }
    else if (ActiveMapPreset == EEchoesSkirmishMapPreset::SorynConfluence)
    {
        BaseDensity = 0.00078f;
        DensityRange = 0.00030f;
        LowColor = FLinearColor(0.025f, 0.085f, 0.13f);
        HighColor = FLinearColor(0.12f, 0.06f, 0.22f);
        HeightFalloff = 0.18f;
        StartDistance = 1020.0f;
    }
    CurrentFogDensity = BaseDensity + DensityRange * Drift;
    if (HeightFog == nullptr)
    {
        return;
    }

    HeightFog->SetFogDensity(CurrentFogDensity);
    HeightFog->SetFogHeightFalloff(HeightFalloff);
    HeightFog->SetStartDistance(StartDistance);
    HeightFog->SetFogInscatteringColor(FMath::Lerp(
        LowColor,
        HighColor,
        Drift));
}
