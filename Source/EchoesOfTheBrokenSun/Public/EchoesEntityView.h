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
    [[nodiscard]] UStaticMeshComponent* GetBodyMesh() const { return BodyMesh; }
    void SetAuthoritativeWorldLocation(const FVector& InLocation);
    void SetAuthoritativeHeadingYaw(float InYaw);
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

    /** World-space bounds of the region a pointer must actually hit to select
        this entity, for callers that need to reason about its on-screen
        silhouette. Deliberately NOT GetActorBounds(): that unions every
        component, including the Relay supply, Aegis power, Waystone, Warform,
        Choir identity and Future Well orbit fields, which are wide radius
        volumes. At ordinary zoom their union projects larger than the viewport
        itself, so any HUD element intersects it and the entity reads as
        occluded wherever the camera is pointed. Returns false when no
        registered pick or body geometry exists yet. */
    [[nodiscard]] bool GetClickableBounds(
        FVector& OutOrigin,
        FVector& OutExtent) const;
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
    [[nodiscard]] bool IsFutureWellCollapsedRemnantVisible() const;
    [[nodiscard]] bool IsFutureWellTerminallyCollapsed() const
    {
        return bFutureWellTerminallyCollapsed;
    }
    [[nodiscard]] echoes::sim::FutureWellChoice GetFutureWellVisualChoice() const
    {
        return FutureWellVisualChoice;
    }
    [[nodiscard]] bool IsSilhouetteAccentVisible() const;
    [[nodiscard]] uint8 GetOwnerMarkerVariant() const;
    [[nodiscard]] FString GetDisplayName() const;

    // Presentation motion families (Track A5 / Gate 7)
    [[nodiscard]] bool IsLocomotionMotionActive() const { return bLocomotionActive; }
    [[nodiscard]] bool IsHoverUnit() const { return bIsHoverUnit; }
    [[nodiscard]] bool IsWalkerUnit() const { return bIsWalkerUnit; }
    [[nodiscard]] float GetLocomotionWalkPhase() const { return WalkCyclePhase; }
    [[nodiscard]] float GetHoverBobOffsetCentimetres() const
    {
        return HoverBobOffsetCentimetres;
    }
    [[nodiscard]] FVector GetBodyMeshRelativeLocation() const;
    [[nodiscard]] FRotator GetBodyMeshRelativeRotation() const;
    [[nodiscard]] FVector GetBodyMeshRelativeScale() const;
    /** Readability scale applied to unit presentation meshes only; the simulation footprint is untouched. */
    [[nodiscard]] float GetPresentationScale() const { return PresentationScale; }
    [[nodiscard]] FVector GetSilhouetteAccentRelativeLocation() const;
    [[nodiscard]] bool IsMotionReducedMotionApplied() const
    {
        return bMotionReducedMotionApplied;
    }
    [[nodiscard]] bool IsWorkerHarvestingActive() const
    {
        return bWorkerHarvestingActive;
    }
    [[nodiscard]] int32 GetCarriedCargoAmount() const { return CarriedCargoAmount; }
    [[nodiscard]] float GetHeadingYaw() const { return CurrentHeadingYaw; }
    [[nodiscard]] FVector GetAuthoritativeVelocity() const
    {
        return AuthoritativeVelocity;
    }
    /** M01-only presentation diagnostics. These expose visual state only and
        never change deterministic movement, fog, collision, or navigation. */
    UFUNCTION(BlueprintPure, Category = "Echoes|M01|Presentation")
    float GetM01SurveyorGaitPhase() const { return M01GaitPhase; }
    UFUNCTION(BlueprintPure, Category = "Echoes|M01|Presentation")
    bool IsM01SurveyorFootSwinging(int32 Side) const
    {
        return Side >= 0 && Side < UE_ARRAY_COUNT(M01Feet) && M01Feet[Side].bSwing;
    }
    UFUNCTION(BlueprintPure, Category = "Echoes|M01|Presentation")
    FVector GetM01SurveyorFootWorld(int32 Side) const
    {
        return Side >= 0 && Side < UE_ARRAY_COUNT(M01Feet) ? M01Feet[Side].World : FVector::ZeroVector;
    }
    UFUNCTION(BlueprintPure, Category = "Echoes|M01|Presentation")
    FVector GetM01SurveyorFootSwingStartWorld(int32 Side) const
    {
        return Side >= 0 && Side < UE_ARRAY_COUNT(M01Feet) ? M01Feet[Side].SwingStart : FVector::ZeroVector;
    }
    UFUNCTION(BlueprintPure, Category = "Echoes|M01|Presentation")
    FVector GetM01SurveyorFootSwingEndWorld(int32 Side) const
    {
        return Side >= 0 && Side < UE_ARRAY_COUNT(M01Feet) ? M01Feet[Side].SwingEnd : FVector::ZeroVector;
    }
    UFUNCTION(BlueprintPure, Category = "Echoes|M01|Presentation")
    float GetM01SurveyorTargetHeadingYaw() const { return TargetHeadingYaw; }
    UFUNCTION(BlueprintPure, Category = "Echoes|M01|Presentation")
    int32 GetM01SurveyorEmergencyReplantCount() const { return M01EmergencyReplantCount; }
    UFUNCTION(BlueprintPure, Category = "Echoes|M01|Presentation")
    int32 GetM01SurveyorPoseResetCount() const { return M01PoseResetCount; }
    UFUNCTION(BlueprintPure, Category = "Echoes|M01|Presentation")
    int32 GetM01SurveyorDiscontinuityPoseResetCount() const { return M01DiscontinuityPoseResetCount; }
    UFUNCTION(BlueprintPure, Category = "Echoes|M01|Presentation")
    int32 GetM01SurveyorPlannedSwingStartCount() const { return M01PlannedSwingStartCount; }
    UFUNCTION(BlueprintPure, Category = "Echoes|M01|Presentation")
    int32 GetM01SurveyorPlannedLandingCount() const { return M01PlannedLandingCount; }
    [[nodiscard]] bool IsGatherBeamActive() const { return bGatherBeamActive; }
    [[nodiscard]] bool IsConstructionFieldActive() const
    {
        return bConstructionFieldActive;
    }
    [[nodiscard]] bool IsReshapeTelegraphActive() const
    {
        return bReshapeTelegraphActive;
    }
    [[nodiscard]] float GetConstructionFraction() const
    {
        return ConstructionFraction;
    }

private:
    enum class EBodyMaterialFamily : uint8
    {
        Basic,
        AuthoredSurface,
        AuthoredWorldSurface,
    };

    void ConfigureAppearance(const echoes::sim::Entity& State);
    void ConfigureFutureWellPresentation(const echoes::sim::Entity& State);
    void ConfigureFutureWellCollapseRemnant(bool bVisible);
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
    void UpdateComponentMotion(float DeltaSeconds, bool bReducedMotion);
    void UpdateWalkerMotion(float DeltaSeconds, float Speed, bool bReducedMotion);
    void UpdateHoverMotion(float DeltaSeconds, float Speed, bool bReducedMotion);
    void UpdateIdleMotion(float DeltaSeconds, bool bReducedMotion);
    void UpdateTacticalStateMotion(float DeltaSeconds, bool bReducedMotion);
    void UpdateWorkerResourceMotion(float DeltaSeconds, bool bReducedMotion);
    void UpdateCombatVFX(float DeltaSeconds, bool bReducedMotion, bool bReducedFlashing);
    [[nodiscard]] bool UsesProloguePresentation() const;
    [[nodiscard]] echoes::sim::EntityId GetPresentedArchiveCarrierId() const;
    void ConfigureM01SurveyorRig();
    void ResetM01SurveyorRig();
    void UpdateM01SurveyorRig(float DeltaSeconds, bool bReducedMotion);
    void ConfigureM01BulwarkParts();
    void ResetM01BulwarkParts();
    void UpdateM01BulwarkParts(float DeltaSeconds, bool bReducedMotion);

    bool bUsingM01BulwarkParts = false;
    float M01BulwarkFoldDegrees = 85.0f;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UStaticMeshComponent>> M01BulwarkWings;


    UPROPERTY(VisibleAnywhere, Category = "Echoes|View")
    TObjectPtr<USceneComponent> SceneRoot;

    /** Carries the readability scale so motion code can keep writing absolute BodyMesh scales. */
    UPROPERTY()
    TObjectPtr<USceneComponent> BodyPivot;

    UPROPERTY(VisibleAnywhere, Category = "Echoes|View")
    TObjectPtr<UStaticMeshComponent> BodyMesh;

    // Allocated only for an authored M01 Surveyor; never part of picking or simulation.
    UPROPERTY(Transient)
    TObjectPtr<USceneComponent> M01SurveyorRigRoot;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UStaticMeshComponent>> M01SurveyorParts;

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

    UPROPERTY(VisibleAnywhere, Category = "Echoes|View|VFX")
    TObjectPtr<UStaticMeshComponent> GatherBeam;

    UPROPERTY(VisibleAnywhere, Category = "Echoes|View|VFX")
    TObjectPtr<UStaticMeshComponent> ConstructionField;

    UPROPERTY(VisibleAnywhere, Category = "Echoes|View|VFX")
    TObjectPtr<UStaticMeshComponent> ReshapeTelegraph;

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

    // Terminal Harvest record: a low fractured rim and a dark inverted cavity
    // occupy the former Well site without restoring a selectable landmark.
    UPROPERTY(VisibleAnywhere, Category = "Echoes|View|FutureWell")
    TObjectPtr<UStaticMeshComponent> FutureWellCollapseRim;

    UPROPERTY(VisibleAnywhere, Category = "Echoes|View|FutureWell")
    TObjectPtr<UStaticMeshComponent> FutureWellCollapseCavity;

    UPROPERTY(VisibleAnywhere, Category = "Echoes|View|FutureWell")
    TObjectPtr<UStaticMeshComponent> FutureWellCollapseFractureA;

    UPROPERTY(VisibleAnywhere, Category = "Echoes|View|FutureWell")
    TObjectPtr<UStaticMeshComponent> FutureWellCollapseFractureB;

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
    TObjectPtr<UStaticMesh> AbilityRangeRingMesh;

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

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> GatherBeamMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> ConstructionFieldMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> ReshapeTelegraphMaterial;

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
    echoes::sim::FutureWellChoice WellPendingChoice =
        echoes::sim::FutureWellChoice::Dormant;
    int32 HitPoints = 1;
    int32 MaxHitPoints = 1;
    float DisplayedHealthFraction = 1.0f;
    float HealthBarWidthScale = 0.9f;
    float HealthBarHeight = 92.0f;
    float PresentationScale = 1.0f;
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
    bool bWellProtocolActive = false;
    bool bFutureWellTerminallyCollapsed = false;
    float FutureWellVisualTimeSeconds = 0.0f;
    FVector FutureWellCoreBaseScale = FVector::OneVector;
    FVector SelectionVFXBaseScale = FVector::OneVector;
    float SelectionVFXTimeSeconds = 0.0f;
    float SelectionVFXEmissiveStrength = 0.0f;
    bool bUsingAuthoredSelectionVFX = false;
    bool bSelectionReducedMotionApplied = false;
    bool bSelectionReducedFlashingApplied = false;
    bool bPreparedForPool = false;

    // Motion state tracking (Track A5 / Gate 7)
    FVector PreviousAuthoritativeLocation = FVector::ZeroVector;
    FVector AuthoritativeVelocity = FVector::ZeroVector;
    float AuthoritativeSpeed = 0.0f;
    float CurrentHeadingYaw = 0.0f;
    float TargetHeadingYaw = 0.0f;
    float WalkCyclePhase = 0.0f;
    float HoverPhaseTime = 0.0f;
    float IdlePhaseTime = 0.0f;
    float WorkerHarvestPhaseTime = 0.0f;
    float HoverBobOffsetCentimetres = 0.0f;
    int32 CarriedCargoAmount = 0;
    bool bLocomotionActive = false;
    bool bIsHoverUnit = false;
    bool bIsWalkerUnit = false;
    bool bWorkerHarvestingActive = false;
    bool bMotionReducedMotionApplied = false;
    FRotator BaseSilhouetteAccentRotation = FRotator::ZeroRotator;

    // Combat and presentation effects (Track A6 / Gate 8)
    bool bGatherBeamActive = false;
    bool bM01GatherTargetBound = false;
    FVector GatherTargetWorld = FVector::ZeroVector;
    struct FM01FootPose
    {
        FVector World = FVector::ZeroVector;
        FVector SwingStart = FVector::ZeroVector;
        FVector SwingEnd = FVector::ZeroVector;
        FVector SettleStart = FVector::ZeroVector;
        FQuat Rotation = FQuat::Identity;
        FQuat SwingRotation = FQuat::Identity;
        // The phase at which this particular airborne arc began. This lets a
        // restart enter late in swing with a C0 world-space foot pose.
        float SwingStartPhase = .60f;
        bool bSwing = false;
    };
    FM01FootPose M01Feet[2];
    FVector M01PreviousRoot = FVector::ZeroVector;
    float M01GaitPhase = 0.0f;
    float M01SettleTime = .28f;
    FVector M01LastDisplayedTravelForward = FVector::ForwardVector;
    int32 M01EmergencyReplantCount = 0;
    int32 M01PoseResetCount = 0;
    int32 M01DiscontinuityPoseResetCount = 0;
    int32 M01PlannedSwingStartCount = 0;
    int32 M01PlannedLandingCount = 0;
    // Alternates an exact target-heading reach tie so repeated restart tests
    // do not always lift the same visual leg.
    int32 M01NextRestartRephaseSide = 0;
    bool bM01HasLastTravelDirection = false;
    bool bM01TargetTurnRephaseLatched = false;
    float M01SurveyorPelvisYaw = 0.0f;
    bool bM01SurveyorPelvisYawInitialized = false;
    bool bUsingM01SurveyorRig = false;
    bool bM01RigInitialized = false;
    bool bM01RigMoving = false;
    // Retains the authoritative locomotion edge across render interpolation
    // gaps, so a stopped worker settles while an active zero-distance frame
    // preserves an already airborne foot.
    bool bM01RigLocomotionActive = false;
    // A stop arms the next displayed motion for the bounded restart planner.
    // Do not consume it until the root visibly advances.
    bool bM01RestartPending = false;
    bool bM01RigReduced = false;
    bool bConstructionFieldActive = false;
    bool bReshapeTelegraphActive = false;
    bool bM01ReshapeExpired = false;
    float ConstructionFraction = 0.0f;
    float GatherBeamPulsePhase = 0.0f;
    float ReshapeTelegraphPulsePhase = 0.0f;
};
