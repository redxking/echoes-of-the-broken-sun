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
    Build,
    Interact
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
        bool bInReducedFlashing,
        float InLifetimeSeconds = 2.4f);

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
    [[nodiscard]] bool HasNavigationDisabled() const;
    [[nodiscard]] bool IsUsingAuthoredVFXAssets() const;
    [[nodiscard]] float GetMarkerDiscYaw() const;
    [[nodiscard]] float GetCurrentEmissiveStrength() const
    {
        return CurrentEmissiveStrength;
    }
    [[nodiscard]] float GetPresentationLifetimeSeconds() const
    {
        return PresentationLifetimeSeconds;
    }

private:
    void ApplyAppearance(const FLinearColor& Color, float EmissiveStrength);
    [[nodiscard]] UStaticMesh* MeshForMarkerType(
        EEchoesCommandMarkerType Type) const;

    UPROPERTY(VisibleAnywhere, Category = "Echoes|Command Feedback")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, Category = "Echoes|Command Feedback")
    TObjectPtr<UStaticMeshComponent> MarkerDisc;

    UPROPERTY(VisibleAnywhere, Category = "Echoes|Command Feedback")
    TObjectPtr<UStaticMeshComponent> GlyphA;

    UPROPERTY(VisibleAnywhere, Category = "Echoes|Command Feedback")
    TObjectPtr<UStaticMeshComponent> GlyphB;

    UPROPERTY()
    TObjectPtr<UStaticMesh> MoveMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh> AttackMoveMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh> PatrolMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh> GuardMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh> BuildMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh> InteractMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh> OrbitMesh;

    UPROPERTY()
    TObjectPtr<UMaterialInterface> VFXMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> DiscMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> GlyphAMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> GlyphBMaterial;

    EEchoesCommandMarkerType MarkerType = EEchoesCommandMarkerType::Move;
    FLinearColor BaseColor = FLinearColor::White;
    FVector BaseScale = FVector::OneVector;
    FRotator BaseDiscRotation = FRotator::ZeroRotator;
    FVector BaseGlyphALocation = FVector::ZeroVector;
    FVector BaseGlyphBLocation = FVector::ZeroVector;
    float ElapsedSeconds = 0.0f;
    float PresentationLifetimeSeconds = 2.4f;
    float CurrentEmissiveStrength = 0.0f;
    bool bReducedMotion = false;
    bool bReducedFlashing = false;
};
