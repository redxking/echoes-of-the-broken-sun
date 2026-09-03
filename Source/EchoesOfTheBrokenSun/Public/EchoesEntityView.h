#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EchoesSimCore/Simulation.h"
#include "EchoesEntityView.generated.h"

class UMaterialInterface;
class UMaterialInstanceDynamic;
class UCapsuleComponent;
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

    /** Fully reactivates a pooled actor for one authoritative entity. */
    void ActivateForEntity(
        const echoes::sim::Entity& State,
        bool bTeleport = true);

    /** Removes every identity-local presentation state before pooling. */
    void PrepareForPool();

    void ApplyAuthoritativeState(
        const echoes::sim::Entity& State,
        bool bTeleport);
    void SetSelected(bool bInSelected);

    [[nodiscard]] uint32 GetEntityId() const { return EntityId; }
    [[nodiscard]] uint8 GetOwnerPlayerId() const { return OwnerPlayerId; }
    [[nodiscard]] echoes::sim::EntityType GetEntityType() const { return EntityType; }
    [[nodiscard]] echoes::sim::Faction GetEntityFaction() const { return EntityFaction; }
    [[nodiscard]] bool IsSelected() const { return bSelected; }
    [[nodiscard]] bool IsPreparedForPool() const { return bPreparedForPool; }
    [[nodiscard]] uint64 GetOwnedMIDCreationCount() const
    {
        return OwnedMIDCreationCount;
    }
    [[nodiscard]] bool HasBodySelectionCollisionEnabled() const;
    // The cursor must be able to reach every part of this entity the player
    // reads as the entity. These report whether a component currently blocks
    // ECC_EchoesEntityPick - the entity-resolution channel, never the
    // ground-position one - so a test can assert that a drawn overlay is
    // pickable and a hidden one is not.
    [[nodiscard]] bool IsHealthBarEntityPickable() const;
    [[nodiscard]] bool IsOwnerMarkerEntityPickable() const;
    [[nodiscard]] bool IsSilhouetteAccentEntityPickable() const;
    [[nodiscard]] bool IsFutureWellPresentationEntityPickable() const;
    // The footprint volume. It is never rendered; it exists so that entity
    // resolution covers the whole readable footprint of entities whose drawn
    // geometry is small, low, or carries no usable collision at all - which is
    // every Matter deposit and every Future Well, since both suppress the
    // silhouette accent and neither has any other overlay.
    [[nodiscard]] bool IsEntityPickProxyEnabled() const;
    [[nodiscard]] bool IsEntityPickProxyHidden() const;
    [[nodiscard]] bool DoesEntityPickProxyBlockGroundTrace() const;
    [[nodiscard]] float GetEntityPickProxyRadius() const;
    [[nodiscard]] float GetEntityPickProxyTopHeight() const;
    [[nodiscard]] bool IsUsingAuthoredSelectionVFX() const
    {
        return bUsingAuthoredSelectionVFX;
    }
    [[nodiscard]] bool IsSelectionVFXVisible() const;
    [[nodiscard]] bool HasSelectionVFXCollisionDisabled() const;
    [[nodiscard]] bool HasSelectionVFXNavigationDisabled() const;
    [[nodiscard]] bool IsSelectionReducedMotionApplied() const
    {
        return bSelectionReducedMotionApplied;
    }
    [[nodiscard]] bool IsSelectionReducedFlashingApplied() const
    {
        return bSelectionReducedFlashingApplied;
    }
    [[nodiscard]] float GetSelectionVFXYaw() const;
    [[nodiscard]] float GetSelectionVFXEmissiveStrength() const
    {
        return SelectionVFXEmissiveStrength;
    }
    [[nodiscard]] float GetDisplayedHealthFraction() const
    {
        return DisplayedHealthFraction;
    }
    [[nodiscard]] bool IsHealthBarVisible() const;
    [[nodiscard]] bool IsOwnerMarkerVisible() const;
    [[nodiscard]] bool IsDeploymentCoverVisible() const;
    [[nodiscard]] bool IsRelaySupplyFieldVisible() const;
    [[nodiscard]] bool IsWaystoneStateVisible() const;
    [[nodiscard]] bool IsWarformStateVisible() const;
    [[nodiscard]] bool IsChoirIdentityStateVisible() const;
    [[nodiscard]] bool IsAegisPowerFieldVisible() const;
    [[nodiscard]] bool IsTemporaryMineralCover() const
    {
        return bTemporaryMineralCover;
    }
    [[nodiscard]] echoes::sim::WaystoneMode GetWaystoneMode() const
    {
        return WaystoneMode;
    }
    [[nodiscard]] echoes::sim::WarformAdaptation GetWarformAdaptation() const
    {
        return WarformAdaptation;
    }
    [[nodiscard]] echoes::sim::WarformAdaptation GetPendingWarformAdaptation() const
    {
        return PendingWarformAdaptation;
    }
    [[nodiscard]] echoes::sim::ChoirIdentityState GetChoirIdentityState() const
    {
        return ChoirIdentityState;
    }
    // The flashing half of the hit feedback: a luminance ramp on the body.
    // Reduced flashing suppresses this one, and only this one.
    [[nodiscard]] bool IsDamagePulseActive() const
    {
        return DamagePulseRemainingSeconds > 0.0f;
    }
    // The hit EVENT itself. Armed for the same window whatever the
    // accessibility settings are, so reduced flashing changes which channel
    // reports "this entity was just hit" and never whether it is reported.
    [[nodiscard]] bool IsDamageAcknowledgementActive() const
    {
        return DamageAcknowledgeRemainingSeconds > 0.0f;
    }
    [[nodiscard]] float GetDamageAcknowledgementRemainingSeconds() const
    {
        return DamageAcknowledgeRemainingSeconds;
    }
    [[nodiscard]] bool IsDamageAcknowledgeMarkerVisible() const;
    [[nodiscard]] bool IsDamageAcknowledgeMarkerEntityPickable() const;
    // The drawn radius of the two ability discs, in centimetres, as actually
    // scaled onto the mesh. A test compares these against the authoritative
    // rule radius so a decorative disc can never drift back in.
    [[nodiscard]] float GetRelaySupplyFieldRadiusCentimetres() const;
    [[nodiscard]] float GetAegisPowerFieldRadiusCentimetres() const;
    // Which protocol shape the fallback Future Well accent is drawing:
    // 0 Dormant, 1 Harvest, 2 Preserve, 3 Reshape, 255 when the accent is not
    // the channel in use because the authored Well presentation is.
    [[nodiscard]] uint8 GetFutureWellProtocolAccentVariant() const
    {
        return FutureWellProtocolAccentVariant;
    }
    [[nodiscard]] bool IsUsingAuthoredRosterMesh() const
    {
        return bUsingAuthoredRosterMesh;
    }
    [[nodiscard]] bool IsUsingAuthoredFutureWellMesh() const
    {
        return bUsingAuthoredFutureWellMesh;
    }
    [[nodiscard]] bool IsUsingAuthoredResourceMesh() const
    {
        return bUsingAuthoredResourceMesh;
    }
    [[nodiscard]] bool IsFutureWellPresentationVisible() const;
    [[nodiscard]] echoes::sim::FutureWellChoice GetFutureWellVisualChoice() const
    {
        return FutureWellVisualChoice;
    }
    [[nodiscard]] bool IsSilhouetteAccentVisible() const;
    [[nodiscard]] uint8 GetOwnerMarkerVariant() const;
    [[nodiscard]] FString GetDisplayName() const;

private:
    enum class EBodyMaterialFamily : uint8
    {
        Basic,
        AuthoredSurface,
        AuthoredWorldSurface,
    };

    void ConfigureAppearance(const echoes::sim::Entity& State);
    void ConfigureFutureWellPresentation(const echoes::sim::Entity& State);
    void EnsureFutureWellMaterialSet(
        UStaticMeshComponent* Component,
        TArray<TObjectPtr<UMaterialInstanceDynamic>>& Materials);
    void EnsureBodyMaterialSet(
        EBodyMaterialFamily Family,
        UMaterialInterface* Parent,
        int32 MaterialCount);
    [[nodiscard]] UMaterialInstanceDynamic* CreateOwnedMaterial(
        UMaterialInterface* Parent);
    void ResetOwnedMaterialParameters();
    void ResetPresentationComponentsForPool();
    // The single gate for every overlay that is part of the entity's drawn
    // silhouette. Visibility and entity-pick collision are set together and
    // never apart, so a hidden overlay is never an invisible pick plate and a
    // drawn overlay is never unpickable. Neither state touches ECC_Visibility:
    // an overlay must not move the ground answer.
    static void SetOverlayVisibleAndPickable(
        UStaticMeshComponent* Component,
        bool bVisible);
    static void SetOverlayUnpickableForPool(UStaticMeshComponent* Component);
    [[nodiscard]] static bool IsOverlayEntityPickable(
        const UStaticMeshComponent* Component);
    void ConfigureEntityPickProxy(float SelectionHaloScale);
    void SetBodyColor(const FLinearColor& Color);
    void UpdateHealthBar();
    void UpdateDamageAcknowledgeMarker();

    UPROPERTY(VisibleAnywhere, Category = "Echoes|View")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, Category = "Echoes|View")
    TObjectPtr<UStaticMeshComponent> BodyMesh;

    UPROPERTY(VisibleAnywhere, Category = "Echoes|View")
    TObjectPtr<UStaticMeshComponent> SilhouetteAccent;

    UPROPERTY(VisibleAnywhere, Category = "Echoes|View")
    TObjectPtr<UCapsuleComponent> EntityPickProxy;

    UPROPERTY(VisibleAnywhere, Category = "Echoes|View")
    TObjectPtr<UStaticMeshComponent> SelectionRing;

    UPROPERTY(VisibleAnywhere, Category = "Echoes|View")
    TObjectPtr<UStaticMeshComponent> HealthBarBackground;

    UPROPERTY(VisibleAnywhere, Category = "Echoes|View")
    TObjectPtr<UStaticMeshComponent> HealthBarFill;

    UPROPERTY(VisibleAnywhere, Category = "Echoes|View")
    TObjectPtr<UStaticMeshComponent> OwnerMarker;

    UPROPERTY(VisibleAnywhere, Category = "Echoes|View")
    TObjectPtr<UStaticMeshComponent> DamageAcknowledgeMarker;

    UPROPERTY(VisibleAnywhere, Category = "Echoes|View")
    TObjectPtr<UStaticMeshComponent> DeploymentCover;

    UPROPERTY(VisibleAnywhere, Category = "Echoes|View")
    TObjectPtr<UStaticMeshComponent> RelaySupplyField;

    UPROPERTY(VisibleAnywhere, Category = "Echoes|View")
    TObjectPtr<UStaticMeshComponent> WaystoneStateField;

    UPROPERTY(VisibleAnywhere, Category = "Echoes|View")
    TObjectPtr<UStaticMeshComponent> WarformStateField;

    UPROPERTY(VisibleAnywhere, Category = "Echoes|View")
    TObjectPtr<UStaticMeshComponent> ChoirIdentityField;

    UPROPERTY(VisibleAnywhere, Category = "Echoes|View")
    TObjectPtr<UStaticMeshComponent> AegisPowerField;

    UPROPERTY(VisibleAnywhere, Category = "Echoes|View|FutureWell")
    TObjectPtr<UStaticMeshComponent> FutureWellOrbitOuter;

    UPROPERTY(VisibleAnywhere, Category = "Echoes|View|FutureWell")
    TObjectPtr<UStaticMeshComponent> FutureWellOrbitInner;

    UPROPERTY(VisibleAnywhere, Category = "Echoes|View|FutureWell")
    TObjectPtr<UStaticMeshComponent> FutureWellCore;

    UPROPERTY(VisibleAnywhere, Category = "Echoes|View|FutureWell")
    TObjectPtr<UStaticMeshComponent> FutureWellGroundGlyphA;

    UPROPERTY(VisibleAnywhere, Category = "Echoes|View|FutureWell")
    TObjectPtr<UStaticMeshComponent> FutureWellGroundGlyphB;

    UPROPERTY()
    TObjectPtr<UStaticMesh> CubeMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh> SphereMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh> CylinderMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh> ConeMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh> FutureWellOrbitMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh> FutureWellCoreMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh> FutureWellGlyphMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh> SelectionHaloMesh;

    UPROPERTY()
    TObjectPtr<UMaterialInterface> BasicMaterial;

    UPROPERTY()
    TObjectPtr<UMaterialInterface> AuthoredSurfaceMaterial;

    UPROPERTY()
    TObjectPtr<UMaterialInterface> AuthoredWorldSurfaceMaterial;

    UPROPERTY()
    TObjectPtr<UMaterialInterface> AuthoredPresentationVFXMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> BodyMaterial;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UMaterialInstanceDynamic>> BodyMaterials;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UMaterialInstanceDynamic>> BasicBodyMaterialCache;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UMaterialInstanceDynamic>> AuthoredBodyMaterialCache;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UMaterialInstanceDynamic>> WorldBodyMaterialCache;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UMaterialInstanceDynamic>> FutureWellOrbitOuterMaterials;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UMaterialInstanceDynamic>> FutureWellOrbitInnerMaterials;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UMaterialInstanceDynamic>> FutureWellCoreMaterials;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UMaterialInstanceDynamic>> FutureWellGroundGlyphAMaterials;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UMaterialInstanceDynamic>> FutureWellGroundGlyphBMaterials;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> SilhouetteAccentMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> RingMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> HealthBarBackgroundMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> HealthBarFillMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> OwnerMarkerMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> DamageAcknowledgeMarkerMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> DeploymentCoverMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> RelaySupplyFieldMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> WaystoneStateFieldMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> WarformStateFieldMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> ChoirIdentityFieldMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> AegisPowerFieldMaterial;

    FVector AuthoritativeWorldLocation = FVector::ZeroVector;
    EBodyMaterialFamily ActiveBodyMaterialFamily =
        EBodyMaterialFamily::Basic;
    int32 ActiveBodyMaterialSlotCount = 0;
    uint64 OwnedMIDCreationCount = 0;
    uint32 EntityId = 0;
    uint8 OwnerPlayerId = echoes::sim::kNeutralPlayer;
    echoes::sim::Faction EntityFaction = echoes::sim::Faction::MeridianCompact;
    echoes::sim::EntityType EntityType = echoes::sim::EntityType::Worker;
    echoes::sim::FutureWellChoice WellChoice =
        echoes::sim::FutureWellChoice::Dormant;
    echoes::sim::FutureWellChoice FutureWellVisualChoice =
        echoes::sim::FutureWellChoice::Dormant;
    int32 HitPoints = 1;
    int32 MaxHitPoints = 1;
    float DisplayedHealthFraction = 1.0f;
    float HealthBarWidthScale = 0.9f;
    float HealthBarHeight = 92.0f;
    float EntityPickProxyRadius = 0.0f;
    float EntityPickProxyTopHeight = 0.0f;
    FLinearColor BaseBodyColor = FLinearColor::White;
    float DamagePulseRemainingSeconds = 0.0f;
    float DamageAcknowledgeRemainingSeconds = 0.0f;
    float RelaySupplyFieldRadiusCentimetres = 0.0f;
    float AegisPowerFieldRadiusCentimetres = 0.0f;
    uint8 FutureWellProtocolAccentVariant = 255;
    bool bHasAuthoritativeLocation = false;
    bool bSelected = false;
    bool bDeployed = false;
    echoes::sim::Vec2 DeploymentFacing =
        echoes::sim::Vec2::FromRaw(echoes::sim::kFixedScale, 0);
    bool bRelaySupplyActive = false;
    echoes::sim::WaystoneMode WaystoneMode =
        echoes::sim::WaystoneMode::NotWaystone;
    echoes::sim::WarformAdaptation WarformAdaptation =
        echoes::sim::WarformAdaptation::None;
    echoes::sim::WarformAdaptation PendingWarformAdaptation =
        echoes::sim::WarformAdaptation::None;
    echoes::sim::ChoirIdentityState ChoirIdentityState =
        echoes::sim::ChoirIdentityState::NotChoir;
    bool bTemporaryMineralCover = false;
    bool bAegisPowered = false;
    bool bUsingAuthoredRosterMesh = false;
    bool bUsingAuthoredFutureWellMesh = false;
    bool bUsingAuthoredResourceMesh = false;
    float FutureWellVisualTimeSeconds = 0.0f;
    FVector FutureWellCoreBaseScale = FVector::OneVector;
    FVector SelectionVFXBaseScale = FVector::OneVector;
    float SelectionVFXTimeSeconds = 0.0f;
    float SelectionVFXEmissiveStrength = 0.0f;
    bool bUsingAuthoredSelectionVFX = false;
    bool bSelectionReducedMotionApplied = false;
    bool bSelectionReducedFlashingApplied = false;
    bool bPreparedForPool = false;
};
