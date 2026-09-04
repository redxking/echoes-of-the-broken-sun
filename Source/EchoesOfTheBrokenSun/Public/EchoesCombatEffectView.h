#pragma once

#include "CoreMinimal.h"
#include "EchoesSimCore/Simulation.h"
#include "GameFramework/Actor.h"
#include "EchoesCombatEffectView.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

/** Transient presentation-only weapon beam, muzzle flash, and impact feedback actor. */
UCLASS(NotBlueprintable, Transient)
class ECHOESOFTHEBROKENSUN_API AEchoesCombatEffectView final : public AActor
{
    GENERATED_BODY()

public:
    AEchoesCombatEffectView();

    virtual void Tick(float DeltaSeconds) override;

    void InitializeCombatEffect(
        echoes::sim::Faction Faction,
        echoes::sim::EntityType EntityType,
        const FVector& InSourceLocation,
        const FVector& InTargetLocation,
        bool bInReducedMotion,
        bool bInReducedFlashing,
        float InLifetimeSeconds = 0.32f);

    /** Deactivates this presentation actor without destroying components or MIDs. */
    void PrepareForPool();

    /** Coalesces a deterministic overflow event without changing authority or lifetime. */
    void RegisterOverflowCoalesced();

    [[nodiscard]] bool IsReducedMotionApplied() const { return bReducedMotion; }
    [[nodiscard]] bool IsReducedFlashingApplied() const { return bReducedFlashing; }
    [[nodiscard]] bool HasCollisionDisabled() const;
    [[nodiscard]] bool HasNavigationDisabled() const;
    [[nodiscard]] bool HasShadowsDisabled() const;
    [[nodiscard]] bool HasOverlapsDisabled() const;
    [[nodiscard]] bool IsUsingAuthoredVFXAssets() const;

    [[nodiscard]] float GetCurrentEmissiveStrength() const
    {
        return CurrentEmissiveStrength;
    }
    [[nodiscard]] FLinearColor GetBaseColor() const { return BaseColor; }
    [[nodiscard]] FVector GetSourceLocation() const { return SourceLocation; }
    virtual FVector GetTargetLocation(AActor* RequestedBy = nullptr) const override
    {
        (void)RequestedBy;
        return TargetLocation;
    }
    [[nodiscard]] FVector GetEffectTargetLocation() const { return TargetLocation; }
    [[nodiscard]] float GetBeamLength() const { return BeamLength; }
    [[nodiscard]] float GetPresentationLifetimeSeconds() const
    {
        return PresentationLifetimeSeconds;
    }
    [[nodiscard]] float GetRemainingLifetimeSeconds() const
    {
        return RemainingLifetimeSeconds;
    }
    [[nodiscard]] bool IsPresentationActive() const
    {
        return bPresentationActive;
    }
    [[nodiscard]] bool IsAfterimageActive() const
    {
        return bAfterimageActive;
    }
    [[nodiscard]] uint64 GetOwnedMIDCreationCount() const
    {
        return OwnedMIDCreationCount;
    }
    [[nodiscard]] uint64 GetCoalescedOverflowCount() const
    {
        return CoalescedOverflowCount;
    }

private:
#if WITH_DEV_AUTOMATION_TESTS
    friend class FEchoesCombatEffectsTest;
#endif
    void ApplyAppearance(const FLinearColor& Color, float EmissiveStrength);
    [[nodiscard]] UMaterialInstanceDynamic* CreateOwnedMaterial();

    UPROPERTY(VisibleAnywhere, Category = "Echoes|Combat Feedback")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, Category = "Echoes|Combat Feedback")
    TObjectPtr<UStaticMeshComponent> BeamMesh;

    UPROPERTY(VisibleAnywhere, Category = "Echoes|Combat Feedback")
    TObjectPtr<UStaticMeshComponent> MuzzleMesh;

    UPROPERTY(VisibleAnywhere, Category = "Echoes|Combat Feedback")
    TObjectPtr<UStaticMeshComponent> ImpactMesh;

    UPROPERTY(VisibleAnywhere, Category = "Echoes|Combat Feedback")
    TObjectPtr<UStaticMeshComponent> AfterimageMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh> CylinderMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh> RingMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh> CoreMesh;

    UPROPERTY()
    TObjectPtr<UMaterialInterface> VFXMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> BeamMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> MuzzleMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> ImpactMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> AfterimageMaterial;

    FLinearColor BaseColor = FLinearColor::White;
    FVector SourceLocation = FVector::ZeroVector;
    FVector TargetLocation = FVector::ZeroVector;
    float BeamLength = 0.0f;
    float PresentationLifetimeSeconds = 0.32f;
    float RemainingLifetimeSeconds = 0.0f;
    float InitialEmissiveStrength = 4.5f;
    float CurrentEmissiveStrength = 0.0f;
    float BaseBeamThickness = 0.08f;
    float BaseMuzzleScale = 0.25f;
    float BaseImpactScale = 0.35f;
    bool bPresentationActive = false;
    bool bReducedMotion = false;
    bool bReducedFlashing = false;
    bool bAfterimageActive = false;
    uint64 OwnedMIDCreationCount = 0;
    uint64 CoalescedOverflowCount = 0;
};
