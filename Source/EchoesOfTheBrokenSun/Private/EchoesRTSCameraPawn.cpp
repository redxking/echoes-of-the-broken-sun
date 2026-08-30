#include "EchoesRTSCameraPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/SceneComponent.h"
#include "EchoesGameUserSettings.h"
#include "EchoesOfTheBrokenSun.h"
#include "EchoesPlayerController.h"
#include "EchoesPointerCombatGuardReview.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"

AEchoesRTSCameraPawn::AEchoesRTSCameraPawn()
{
    PrimaryActorTick.bCanEverTick = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(SceneRoot);
    SpringArm->TargetArmLength = 3800.0f;
    SpringArm->SetRelativeRotation(FRotator(-58.0f, -45.0f, 0.0f));
    SpringArm->bDoCollisionTest = false;
    SpringArm->bEnableCameraLag = true;
    SpringArm->CameraLagSpeed = 12.0f;

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
    Camera->SetFieldOfView(55.0f);

}

void AEchoesRTSCameraPawn::BeginPlay()
{
    Super::BeginPlay();
    bEdgePanArmed = false;
#if !UE_BUILD_SHIPPING
    if (FParse::Param(
            FCommandLine::Get(),
            TEXT("EchoesPointerCombatGuardReview")))
    {
        FEchoesPointerCombatGuardReview ReviewConfiguration;
        FString RequestedVariant;
        if (!FEchoesPointerCombatGuardReview::TryFromCommandLine(
                ReviewConfiguration,
                RequestedVariant))
        {
            UE_LOG(
                LogEchoes,
                Error,
                TEXT("[ECHOES_POINTER_COMBAT_GUARD_REVIEW_CAMERA_FAILED] reason=INVALID_VARIANT requested=%s"),
                *RequestedVariant);
            return;
        }
        SetActorLocation(ReviewConfiguration.CameraLocation);
        SpringArm->TargetArmLength = ReviewConfiguration.CameraZoom;
        SpringArm->SetRelativeRotation(FRotator(-60.0f, -45.0f, 0.0f));
        SpringArm->bEnableCameraLag = false;
        Camera->SetFieldOfView(52.0f);
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_POINTER_COMBAT_GUARD_REVIEW_CAMERA] variant=%s centerTile=(%.0f,%.0f) zoom=%.0f hudScale=%.2f expectedViewport=(%d,%d) exactScreenProjection=true controlledNonshipping=true"),
            *ReviewConfiguration.Variant,
            ReviewConfiguration.CameraCenterTile.X,
            ReviewConfiguration.CameraCenterTile.Y,
            ReviewConfiguration.CameraZoom,
            ReviewConfiguration.HudScale,
            ReviewConfiguration.ExpectedViewport.X,
            ReviewConfiguration.ExpectedViewport.Y);
        return;
    }
    if (FParse::Param(
            FCommandLine::Get(),
            TEXT("EchoesDestructionVFXReview")))
    {
        const bool bReducedPresentation = FParse::Param(
            FCommandLine::Get(),
            TEXT("EchoesReviewReducedPresentation"));
        bArtReviewMode = true;
        SetActorLocation(FVector(-4200.0f, -4200.0f, 100.0f));
        SpringArm->TargetArmLength = 2500.0f;
        SpringArm->SetRelativeRotation(FRotator(-60.0f, -45.0f, 0.0f));
        SpringArm->bEnableCameraLag = false;
        Camera->SetFieldOfView(52.0f);
        Camera->PostProcessSettings.bOverride_AutoExposureBias = true;
        Camera->PostProcessSettings.AutoExposureBias = -0.25f;
        Camera->PostProcessBlendWeight = 1.0f;
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_DESTRUCTION_VFX_REVIEW_CAMERA] center=(-4200,-4200) zoom=2500 reducedMotion=%s reducedFlashing=%s editorOnly=true"),
            bReducedPresentation ? TEXT("true") : TEXT("false"),
            bReducedPresentation ? TEXT("true") : TEXT("false"));
        return;
    }
    if (FParse::Param(
            FCommandLine::Get(),
            TEXT("EchoesPresentationVFXReview")))
    {
        const bool bReducedPresentation = FParse::Param(
            FCommandLine::Get(),
            TEXT("EchoesReviewReducedPresentation"));
        bArtReviewMode = true;
        SetActorLocation(FVector(-4200.0f, -4200.0f, 100.0f));
        SpringArm->TargetArmLength = 2500.0f;
        SpringArm->SetRelativeRotation(FRotator(-60.0f, -45.0f, 0.0f));
        SpringArm->bEnableCameraLag = false;
        Camera->SetFieldOfView(52.0f);
        Camera->PostProcessSettings.bOverride_AutoExposureBias = true;
        Camera->PostProcessSettings.AutoExposureBias = -0.25f;
        Camera->PostProcessBlendWeight = 1.0f;
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_PRESENTATION_VFX_REVIEW_CAMERA] center=(-4200,-4200) zoom=2500 reducedMotion=%s reducedFlashing=%s editorOnly=true"),
            bReducedPresentation ? TEXT("true") : TEXT("false"),
            bReducedPresentation ? TEXT("true") : TEXT("false"));
        return;
    }
    FString GlassScarReviewMode;
    if (FParse::Value(
            FCommandLine::Get(),
            TEXT("EchoesGlassScarReview="),
            GlassScarReviewMode) ||
        FParse::Param(
            FCommandLine::Get(),
            TEXT("EchoesGlassScarArtReview")))
    {
        if (GlassScarReviewMode.IsEmpty())
        {
            GlassScarReviewMode = TEXT("Overview");
        }
        const bool bOverview = GlassScarReviewMode.Equals(
            TEXT("Overview"),
            ESearchCase::IgnoreCase);
        const bool bBuriedCauseway = GlassScarReviewMode.Equals(
            TEXT("BuriedCauseway"),
            ESearchCase::IgnoreCase);
        const bool bFoldedVerge = GlassScarReviewMode.Equals(
            TEXT("FoldedVerge"),
            ESearchCase::IgnoreCase);
        const float CenterX =
            GlassScarReviewMode.Equals(TEXT("AshCut"), ESearchCase::IgnoreCase)
                ? -3800.0f
            : GlassScarReviewMode.Equals(
                  TEXT("FoldedVerge"),
                  ESearchCase::IgnoreCase)
                ? 3400.0f
                : 0.0f;
        bArtReviewMode = true;
        SetActorLocation(FVector(CenterX, 0.0f, 100.0f));
        SpringArm->TargetArmLength =
            bOverview
                ? 10800.0f
                : (bFoldedVerge ? 3350.0f : (bBuriedCauseway ? 2850.0f : 2300.0f));
        SpringArm->SetRelativeRotation(
            FRotator(bOverview ? -68.0f : -58.0f, bOverview ? -90.0f : -45.0f, 0.0f));
        SpringArm->bEnableCameraLag = false;
        Camera->SetFieldOfView(bOverview ? 58.0f : 52.0f);
        Camera->PostProcessSettings.bOverride_AutoExposureBias = true;
        Camera->PostProcessSettings.AutoExposureBias = bOverview ? -0.05f : 0.15f;
        Camera->PostProcessBlendWeight = 1.0f;
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_GLASS_SCAR_ART_REVIEW_CAMERA] mode=%s centerX=%.0f zoom=%.0f editorOnly=true"),
            *GlassScarReviewMode,
            CenterX,
            SpringArm->TargetArmLength);
        return;
    }
    if (FParse::Param(
            FCommandLine::Get(),
            TEXT("EchoesFutureWellArtReview")))
    {
        bArtReviewMode = true;
        SetActorLocation(FVector(-4400.0f, -4400.0f, 100.0f));
        SpringArm->TargetArmLength = 1350.0f;
        SpringArm->SetRelativeRotation(FRotator(-60.0f, -45.0f, 0.0f));
        SpringArm->bEnableCameraLag = false;
        Camera->PostProcessSettings.bOverride_AutoExposureBias = true;
        Camera->PostProcessSettings.AutoExposureBias = -1.75f;
        Camera->PostProcessBlendWeight = 1.0f;
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_FUTURE_WELL_ART_REVIEW_CAMERA] previewTile=(10,10) zoom=1350 editorOnly=true"));
        return;
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("EchoesArtReview")))
    {
        bArtReviewMode = true;
        SetActorLocation(FVector(-4400.0f, -4400.0f, 100.0f));
        SpringArm->TargetArmLength = 1900.0f;
        SpringArm->bEnableCameraLag = false;
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_ART_REVIEW_CAMERA] localBaseCentered=true zoom=1900 editorOnly=true"));
        return;
    }
#endif
    SetActorLocation(FVector(-3000.0f, -3000.0f, 100.0f));
}

void AEchoesRTSCameraPawn::SetupPlayerInputComponent(
    UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    check(PlayerInputComponent != nullptr);

    PlayerInputComponent->BindAxis(
        TEXT("CameraForward"),
        this,
        &AEchoesRTSCameraPawn::SetForwardInput);
    PlayerInputComponent->BindAxis(
        TEXT("CameraRight"),
        this,
        &AEchoesRTSCameraPawn::SetRightInput);
    PlayerInputComponent->BindAction(
        TEXT("CameraZoomIn"),
        IE_Pressed,
        this,
        &AEchoesRTSCameraPawn::ZoomIn);
    PlayerInputComponent->BindAction(
        TEXT("CameraZoomOut"),
        IE_Pressed,
        this,
        &AEchoesRTSCameraPawn::ZoomOut);
}

void AEchoesRTSCameraPawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

#if !UE_BUILD_SHIPPING
    if (bArtReviewMode && !bArtReviewScreenshotRequested)
    {
        ArtReviewElapsedSeconds += DeltaSeconds;
        if (ArtReviewElapsedSeconds >= 1.5f)
        {
            FString OutputPath;
            const bool bShowUI = !FParse::Param(
                FCommandLine::Get(),
                TEXT("EchoesArtReviewHideUI"));
            if (FParse::Value(
                    FCommandLine::Get(),
                    TEXT("EchoesArtReviewOutput="),
                    OutputPath) &&
                !OutputPath.IsEmpty())
            {
                FScreenshotRequest::RequestScreenshot(
                    OutputPath,
                    bShowUI,
                    false,
                    false,
                    FIntRect(),
                    true);
            }
            else
            {
                FScreenshotRequest::RequestScreenshot(bShowUI, true);
            }
            bArtReviewScreenshotRequested = true;
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_ART_REVIEW_CAPTURE] requested=true showUI=%s delay=1.5 output=%s"),
                bShowUI ? TEXT("true") : TEXT("false"),
                OutputPath.IsEmpty() ? TEXT("default") : *OutputPath);
        }
    }
#endif

    const UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get();
    const bool bEdgePanEnabled = Settings == nullptr || Settings->IsEdgePanEnabled();
    const float PanSpeedScale =
        Settings != nullptr ? Settings->GetCameraPanSpeedScale() : 1.0f;
    SpringArm->bEnableCameraLag =
        Settings == nullptr || !Settings->IsReducedMotionEnabled();

    FVector2D EdgeInput = FVector2D::ZeroVector;
    APlayerController* Controller = Cast<APlayerController>(GetController());
    const AEchoesPlayerController* EchoesController =
        Cast<AEchoesPlayerController>(Controller);
    if (EchoesController != nullptr && EchoesController->IsModalOverlayVisible())
    {
        ForwardInput = 0.0f;
        RightInput = 0.0f;
        bEdgePanArmed = false;
        return;
    }
    if (bEdgePanEnabled && Controller != nullptr &&
        (EchoesController == nullptr || !EchoesController->IsDraggingSelection()))
    {
        int32 ViewportWidth = 0;
        int32 ViewportHeight = 0;
        float MouseX = 0.0f;
        float MouseY = 0.0f;
        Controller->GetViewportSize(ViewportWidth, ViewportHeight);
        if (ViewportWidth > 0 && ViewportHeight > 0 &&
            Controller->GetMousePosition(MouseX, MouseY))
        {
            const bool bMouseInsideHorizontalEdges =
                MouseX > EdgePanPixels &&
                MouseX < static_cast<float>(ViewportWidth) - EdgePanPixels;
            const bool bMouseInsideVerticalEdges =
                MouseY > EdgePanPixels &&
                MouseY < static_cast<float>(ViewportHeight) - EdgePanPixels;
            if (!bEdgePanArmed)
            {
                // Do not let a pointer parked at an edge during application launch
                // move the camera before the player has entered the battlefield view.
                bEdgePanArmed =
                    bMouseInsideHorizontalEdges && bMouseInsideVerticalEdges;
            }
            else if (MouseX <= EdgePanPixels)
            {
                EdgeInput.Y = -1.0f;
            }
            else if (MouseX >= static_cast<float>(ViewportWidth) - EdgePanPixels)
            {
                EdgeInput.Y = 1.0f;
            }
            if (bEdgePanArmed && MouseY <= EdgePanPixels)
            {
                EdgeInput.X = 1.0f;
            }
            else if (bEdgePanArmed &&
                     MouseY >= static_cast<float>(ViewportHeight) - EdgePanPixels)
            {
                EdgeInput.X = -1.0f;
            }
        }
    }

    const float AppliedForward = FMath::Clamp(ForwardInput + EdgeInput.X, -1.0f, 1.0f);
    const float AppliedRight = FMath::Clamp(RightInput + EdgeInput.Y, -1.0f, 1.0f);
    const FRotator HorizontalViewRotation(
        0.0f,
        SpringArm->GetComponentRotation().Yaw,
        0.0f);
    const FVector ViewForward = HorizontalViewRotation.Vector();
    const FVector ViewRight = FRotationMatrix(HorizontalViewRotation).GetUnitAxis(EAxis::Y);
    const FVector PanDelta =
        (ViewForward * AppliedForward + ViewRight * AppliedRight) *
        PanSpeed * PanSpeedScale * DeltaSeconds;
    AddActorWorldOffset(PanDelta, false, nullptr, ETeleportType::None);
    ClampToBattlefield();
}

void AEchoesRTSCameraPawn::SetForwardInput(float Value)
{
    ForwardInput = Value;
}

void AEchoesRTSCameraPawn::SetRightInput(float Value)
{
    RightInput = Value;
}

void AEchoesRTSCameraPawn::ZoomIn()
{
    ApplyZoom(-1.0f);
}

void AEchoesRTSCameraPawn::ZoomOut()
{
    ApplyZoom(1.0f);
}

void AEchoesRTSCameraPawn::ApplyZoom(float Direction)
{
    if (const AEchoesPlayerController* EchoesController =
            Cast<AEchoesPlayerController>(GetController());
        EchoesController != nullptr && EchoesController->IsModalOverlayVisible())
    {
        return;
    }
    const UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get();
    const float ZoomScale =
        Settings != nullptr ? Settings->GetCameraZoomScale() : 1.0f;
    SpringArm->TargetArmLength = FMath::Clamp(
        SpringArm->TargetArmLength + ZoomStep * ZoomScale * Direction,
        MinimumZoom,
        MaximumZoom);
}

void AEchoesRTSCameraPawn::ClampToBattlefield()
{
    const UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr)
    {
        return;
    }

    const float HalfWidth =
        static_cast<float>(Bridge->GetMapWidthTiles()) *
        UEchoesSimulationSubsystem::TileWorldSize * 0.5f;
    const float HalfHeight =
        static_cast<float>(Bridge->GetMapHeightTiles()) *
        UEchoesSimulationSubsystem::TileWorldSize * 0.5f;
    FVector Location = GetActorLocation();
    Location.X = FMath::Clamp(Location.X, -HalfWidth, HalfWidth);
    Location.Y = FMath::Clamp(Location.Y, -HalfHeight, HalfHeight);
    SetActorLocation(Location);
}
