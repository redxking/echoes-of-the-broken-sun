#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EchoesSimCore/Simulation.h"
#include "EchoesEntityView.generated.h"

class UMaterialInterface;
class UMaterialInstanceDynamic;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

/** A presentation-only actor for one deterministic simulation entity. */
UCLASS(NotBlueprintable)
class ECHOESOFTHEBROKENSUN_API AEchoesEntityView final : public AActor
{
    GENERATED_BODY()

public:
    AEchoesEntityView();

    virtual void Tick(float DeltaSeconds) override;

    void ApplyAuthoritativeState(
        const echoes::sim::Entity& State,
        bool bTeleport);
    void SetSelected(bool bInSelected);

    [[nodiscard]] uint32 GetEntityId() const { return EntityId; }
    [[nodiscard]] uint8 GetOwnerPlayerId() const { return OwnerPlayerId; }
    [[nodiscard]] echoes::sim::EntityType GetEntityType() const { return EntityType; }
    [[nodiscard]] bool IsSelected() const { return bSelected; }
    [[nodiscard]] float GetDisplayedHealthFraction() const
    {
        return DisplayedHealthFraction;
    }
    [[nodiscard]] bool IsHealthBarVisible() const;
    [[nodiscard]] bool IsOwnerMarkerVisible() const;
    [[nodiscard]] bool IsDeploymentCoverVisible() const;
    [[nodiscard]] bool IsDamagePulseActive() const
    {
        return DamagePulseRemainingSeconds > 0.0f;
    }
    [[nodiscard]] uint8 GetOwnerMarkerVariant() const;
    [[nodiscard]] FString GetDisplayName() const;

private:
    void ConfigureAppearance(const echoes::sim::Entity& State);
    void SetBodyColor(const FLinearColor& Color);
    void UpdateHealthBar();

    UPROPERTY(VisibleAnywhere, Category = "Echoes|View")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, Category = "Echoes|View")
    TObjectPtr<UStaticMeshComponent> BodyMesh;

    UPROPERTY(VisibleAnywhere, Category = "Echoes|View")
    TObjectPtr<UStaticMeshComponent> SelectionRing;

    UPROPERTY(VisibleAnywhere, Category = "Echoes|View")
    TObjectPtr<UStaticMeshComponent> HealthBarBackground;

    UPROPERTY(VisibleAnywhere, Category = "Echoes|View")
    TObjectPtr<UStaticMeshComponent> HealthBarFill;

    UPROPERTY(VisibleAnywhere, Category = "Echoes|View")
    TObjectPtr<UStaticMeshComponent> OwnerMarker;

    UPROPERTY(VisibleAnywhere, Category = "Echoes|View")
    TObjectPtr<UStaticMeshComponent> DeploymentCover;

    UPROPERTY()
    TObjectPtr<UStaticMesh> CubeMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh> SphereMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh> CylinderMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh> ConeMesh;

    UPROPERTY()
    TObjectPtr<UMaterialInterface> BasicMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> BodyMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> RingMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> HealthBarBackgroundMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> HealthBarFillMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> OwnerMarkerMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> DeploymentCoverMaterial;

    FVector AuthoritativeWorldLocation = FVector::ZeroVector;
    uint32 EntityId = 0;
    uint8 OwnerPlayerId = echoes::sim::kNeutralPlayer;
    echoes::sim::Faction EntityFaction = echoes::sim::Faction::MeridianCompact;
    echoes::sim::EntityType EntityType = echoes::sim::EntityType::Worker;
    echoes::sim::FutureWellChoice WellChoice =
        echoes::sim::FutureWellChoice::Dormant;
    int32 HitPoints = 1;
    int32 MaxHitPoints = 1;
    float DisplayedHealthFraction = 1.0f;
    float HealthBarWidthScale = 0.9f;
    float HealthBarHeight = 92.0f;
    FLinearColor BaseBodyColor = FLinearColor::White;
    float DamagePulseRemainingSeconds = 0.0f;
    bool bHasAuthoritativeLocation = false;
    bool bSelected = false;
    bool bDeployed = false;
    echoes::sim::Vec2 DeploymentFacing =
        echoes::sim::Vec2::FromRaw(echoes::sim::kFixedScale, 0);
};
