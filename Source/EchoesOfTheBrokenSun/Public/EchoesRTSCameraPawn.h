#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "EchoesRTSCameraPawn.generated.h"

class UCameraComponent;
class USceneComponent;
class USpringArmComponent;

/** Mouse-and-keyboard top-down camera for the runtime prototype. */
UCLASS(NotBlueprintable)
class ECHOESOFTHEBROKENSUN_API AEchoesRTSCameraPawn final : public APawn
{
    GENERATED_BODY()

public:
    AEchoesRTSCameraPawn();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    /**
     * Applies an orthographic frame using the existing authored perspective
     * arm-distance/FOV vocabulary. Editor and review callers that alter the
     * arm directly while ticking is disabled must call this explicitly.
     */
    void SetCameraFraming(float LegacyArmLength, float LegacyFieldOfViewDegrees = 55.0f);

private:
    /** Applies the authored exposure, tonemapper, and bloom baseline
     *  (revision exposure-authored-v1) to the camera. Every mode inherits
     *  this; review fixtures override only the exposure bias. */
    void ApplyAuthoredPostProcess();

    void SetForwardInput(float Value);
    void SetRightInput(float Value);
    void ZoomIn();
    void ZoomOut();
    void ApplyZoom(float Direction);
    /** Keeps direct arm edits made while ticking in sync with OrthoWidth. */
    void SynchronizeOrthographicFraming();
    void ClampToBattlefield();

    UPROPERTY(VisibleAnywhere, Category = "Echoes|Camera")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, Category = "Echoes|Camera")
    TObjectPtr<USpringArmComponent> SpringArm;

    UPROPERTY(VisibleAnywhere, Category = "Echoes|Camera")
    TObjectPtr<UCameraComponent> Camera;

    float ForwardInput = 0.0f;
    float RightInput = 0.0f;
    bool bEdgePanArmed = false;
    bool bArtReviewMode = false;
    bool bArtReviewScreenshotRequested = false;
    float ArtReviewElapsedSeconds = 0.0f;
    float ArtReviewAssetWaitSeconds = 0.0f;
    float ArtReviewCaptureDelaySeconds = 2.0f;
    bool bArtReviewScoutIssued = false;
    // Review-only high-contrast override: the previous user value is restored
    // at end of play so a capture never rewrites the saved setting.
    bool bArtReviewContrastOverridden = false;
    bool bArtReviewContrastPrevious = false;
    float ActiveFramingFieldOfViewDegrees = 55.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Echoes|Camera")
    float PanSpeed = 2400.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Echoes|Camera")
    float EdgePanPixels = 18.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Echoes|Camera")
    float ZoomStep = 500.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Echoes|Camera")
    float MinimumZoom = 1400.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Echoes|Camera")
    float MaximumZoom = 6200.0f;
};
