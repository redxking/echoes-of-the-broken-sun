#pragma once

#include "CoreMinimal.h"
#include "EchoesSimCore/Simulation.h"
#include "GameFramework/Actor.h"
#include "EchoesDestructionView.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

/** Transient presentation-only confirmation after a previously visible entity is removed. */
UCLASS(NotBlueprintable, Transient)
class ECHOESOFTHEBROKENSUN_API AEchoesDestructionView final : public AActor
{
    GENERATED_BODY()

public:
    AEchoesDestructionView();

    virtual void Tick(float DeltaSeconds) override;

    void InitializeDestruction(
        echoes::sim::Faction Faction,
        echoes::sim::EntityType EntityType,
        bool bInReducedMotion,
        bool bInReducedFlashing,
        float InLifetimeSeconds = 1.6f);

    [[nodiscard]] bool IsReducedMotionApplied() const { return bReducedMotion; }
    [[nodiscard]] bool IsReducedFlashingApplied() const { return bReducedFlashing; }
    [[nodiscard]] bool HasCollisionDisabled() const;
    [[nodiscard]] bool HasNavigationDisabled() const;
    [[nodiscard]] bool IsUsingAuthoredVFXAssets() const;
    [[nodiscard]] float GetCurrentEmissiveStrength() const
    {
        return CurrentEmissiveStrength;
    }
    [[nodiscard]] FLinearColor GetBaseColor() const { return BaseColor; }
    [[nodiscard]] FVector GetRingScale() const;
    [[nodiscard]] FVector GetShardALocation() const;
    [[nodiscard]] float GetPresentationLifetimeSeconds() const
    {
        return PresentationLifetimeSeconds;
    }

private:
    void ApplyAppearance(const FLinearColor& Color, float EmissiveStrength);

    UPROPERTY(VisibleAnywhere, Category = "Echoes|Destruction Feedback")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, Category = "Echoes|Destruction Feedback")
    TObjectPtr<UStaticMeshComponent> ShockRing;

    UPROPERTY(VisibleAnywhere, Category = "Echoes|Destruction Feedback")
    TObjectPtr<UStaticMeshComponent> CoreEmber;

    UPROPERTY(VisibleAnywhere, Category = "Echoes|Destruction Feedback")
    TObjectPtr<UStaticMeshComponent> ShardA;

    UPROPERTY(VisibleAnywhere, Category = "Echoes|Destruction Feedback")
    TObjectPtr<UStaticMeshComponent> ShardB;

    UPROPERTY()
    TObjectPtr<UStaticMesh> RingMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh> CoreMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh> ShardMesh;

    UPROPERTY()
    TObjectPtr<UMaterialInterface> VFXMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> RingMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> CoreMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> ShardAMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> ShardBMaterial;

    FLinearColor BaseColor = FLinearColor::White;
    FVector BaseRingScale = FVector::OneVector;
    FVector BaseCoreScale = FVector::OneVector;
    FVector BaseShardALocation = FVector::ZeroVector;
    FVector BaseShardBLocation = FVector::ZeroVector;
    float ElapsedSeconds = 0.0f;
    float PresentationLifetimeSeconds = 1.6f;
    float CurrentEmissiveStrength = 0.0f;
    bool bReducedMotion = false;
    bool bReducedFlashing = false;
};
