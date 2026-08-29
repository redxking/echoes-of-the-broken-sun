#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EchoesCommandMarkerView.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

/** Shape-coded, presentation-only confirmation for an accepted local order. */
UENUM()
enum class EEchoesCommandMarkerType : uint8
{
    Move,
    AttackMove,
    Patrol,
    Guard,
    Build
};

UCLASS(NotBlueprintable, Transient)
class ECHOESOFTHEBROKENSUN_API AEchoesCommandMarkerView final : public AActor
{
    GENERATED_BODY()

public:
    AEchoesCommandMarkerView();

    virtual void Tick(float DeltaSeconds) override;

    void InitializeMarker(
        EEchoesCommandMarkerType InType,
        bool bInReducedMotion,
        bool bInReducedFlashing);

    [[nodiscard]] EEchoesCommandMarkerType GetMarkerType() const
    {
        return MarkerType;
    }
    [[nodiscard]] bool IsReducedMotionApplied() const
    {
        return bReducedMotion;
    }
    [[nodiscard]] bool IsReducedFlashingApplied() const
    {
        return bReducedFlashing;
    }
    [[nodiscard]] bool HasCollisionDisabled() const;
    [[nodiscard]] float GetPresentationLifetimeSeconds() const
    {
        return PresentationLifetimeSeconds;
    }

private:
    void ApplyColor(const FLinearColor& Color);

    UPROPERTY(VisibleAnywhere, Category = "Echoes|Command Feedback")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, Category = "Echoes|Command Feedback")
    TObjectPtr<UStaticMeshComponent> MarkerDisc;

    UPROPERTY(VisibleAnywhere, Category = "Echoes|Command Feedback")
    TObjectPtr<UStaticMeshComponent> GlyphA;

    UPROPERTY(VisibleAnywhere, Category = "Echoes|Command Feedback")
    TObjectPtr<UStaticMeshComponent> GlyphB;

    UPROPERTY()
    TObjectPtr<UStaticMesh> CubeMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh> CylinderMesh;

    UPROPERTY()
    TObjectPtr<UMaterialInterface> BasicMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> DiscMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> GlyphAMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> GlyphBMaterial;

    EEchoesCommandMarkerType MarkerType = EEchoesCommandMarkerType::Move;
    FLinearColor BaseColor = FLinearColor::White;
    FVector BaseScale = FVector::OneVector;
    float ElapsedSeconds = 0.0f;
    float PresentationLifetimeSeconds = 2.4f;
    bool bReducedMotion = false;
    bool bReducedFlashing = false;
};
