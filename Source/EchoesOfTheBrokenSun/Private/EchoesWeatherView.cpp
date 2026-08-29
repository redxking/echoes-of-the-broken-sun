#include "EchoesWeatherView.h"

#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SceneComponent.h"
#include "EchoesGameUserSettings.h"

namespace
{
constexpr float DriftCycleSeconds = 28.0f;
constexpr float BaseFogDensity = 0.0007f;
constexpr float FogDensityRange = 0.00035f;
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
    CurrentFogDensity = BaseFogDensity + FogDensityRange * Drift;
    if (HeightFog == nullptr)
    {
        return;
    }

    HeightFog->SetFogDensity(CurrentFogDensity);
    HeightFog->SetFogInscatteringColor(FMath::Lerp(
        FLinearColor(0.035f, 0.075f, 0.13f),
        FLinearColor(0.15f, 0.055f, 0.19f),
        Drift));
}
