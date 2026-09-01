#pragma once

#include "CoreMinimal.h"
#include "EchoesSkirmishSetup.h"
#include "GameFramework/Actor.h"
#include "EchoesWeatherView.generated.h"

class UExponentialHeightFogComponent;
class USceneComponent;

/** Lightweight presentation-only atmospheric drift over the Glass Scar. */
UCLASS(NotBlueprintable)
class ECHOESOFTHEBROKENSUN_API AEchoesWeatherView final : public AActor
{
    GENERATED_BODY()

public:
    AEchoesWeatherView();

    virtual void Tick(float DeltaSeconds) override;
    void ApplyMapPreset(EEchoesSkirmishMapPreset MapPreset);

    [[nodiscard]] float GetCurrentFogDensity() const
    {
        return CurrentFogDensity;
    }
    [[nodiscard]] EEchoesSkirmishMapPreset GetMapPreset() const
    {
        return ActiveMapPreset;
    }

private:
    void ApplyAtmosphere(float NormalizedDrift);

    UPROPERTY(VisibleAnywhere, Category = "Echoes|Weather")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, Category = "Echoes|Weather")
    TObjectPtr<UExponentialHeightFogComponent> HeightFog;

    float DriftPhaseSeconds = 0.0f;
    float CurrentFogDensity = 0.0007f;
    EEchoesSkirmishMapPreset ActiveMapPreset =
        EEchoesSkirmishMapPreset::GlassScar;
};
