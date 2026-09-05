#include "EchoesRTSCameraPawn.h"

#include "AssetCompilingManager.h"

#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/SceneComponent.h"
#include "EchoesEntityView.h"
#include "EchoesGameUserSettings.h"
#include "EchoesOfTheBrokenSun.h"
#include "EchoesPlayerController.h"
#include "EchoesPointerCombatGuardReview.h"
#include "EchoesSimulationSubsystem.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#if WITH_EDITOR
#include "HAL/IConsoleManager.h"
#endif
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "String/LexFromString.h"
#include "UnrealClient.h"

namespace
{
#if WITH_EDITOR
TAutoConsoleVariable<int32> CVarEchoesEditorArtReviewCameraEnabled(
    TEXT("Echoes.EditorArtReviewCameraEnabled"), 1,
    TEXT("PIE only: 0 restores the ordinary camera on next play without restarting the editor."), ECVF_Default);
TAutoConsoleVariable<FString> CVarEchoesEditorArtReviewCenter(
    TEXT("Echoes.EditorArtReviewCenter"), TEXT(""),
    TEXT("Editor EchoesArtReview override: world X,Y center; empty uses command line."), ECVF_Default);
TAutoConsoleVariable<float> CVarEchoesEditorArtReviewZoom(
    TEXT("Echoes.EditorArtReviewZoom"), -1.0f,
    TEXT("Editor EchoesArtReview override: camera zoom; -1 uses command line."), ECVF_Default);
TAutoConsoleVariable<FString> CVarEchoesEditorArtReviewScout(
    TEXT("Echoes.EditorArtReviewScout"), TEXT(""),
    TEXT("Editor EchoesArtReview override: world X,Y owned-unit scout destination; empty uses command line."), ECVF_Default);
TAutoConsoleVariable<float> CVarEchoesEditorArtReviewDelay(
    TEXT("Echoes.EditorArtReviewDelay"), -1.0f,
    TEXT("Editor EchoesArtReview override: screenshot delay seconds; -1 uses command line."), ECVF_Default);
TAutoConsoleVariable<FString> CVarEchoesEditorArtReviewOutput(
    TEXT("Echoes.EditorArtReviewOutput"), TEXT(""),
    TEXT("Editor EchoesArtReview override: screenshot output path; empty uses command line."), ECVF_Default);

bool TryParseArtReviewPoint(const FString& Text, const UWorld* World, FVector& OutPoint)
{
    FString XText, YText;
    if (!Text.Split(TEXT(","), &XText, &YText)) return false;
    float X = 0.0f, Y = 0.0f;
    if (!LexTryParseString(X, *XText) || !LexTryParseString(Y, *YText)) return false;
    const UEchoesSimulationSubsystem* Bridge =
        World != nullptr ? World->GetSubsystem<UEchoesSimulationSubsystem>() : nullptr;
    if (Bridge == nullptr || !FMath::IsFinite(X) || !FMath::IsFinite(Y)) return false;
    const float HalfWidth = Bridge->GetMapWidthTiles() * UEchoesSimulationSubsystem::TileWorldSize * 0.5f;
    const float HalfHeight = Bridge->GetMapHeightTiles() * UEchoesSimulationSubsystem::TileWorldSize * 0.5f;
    if (X < -HalfWidth || X > HalfWidth || Y < -HalfHeight || Y > HalfHeight) return false;
    OutPoint = FVector(X, Y, 0.0f);
    return true;
}
#endif
}

AEchoesRTSCameraPawn::AEchoesRTSCameraPawn()
{
    PrimaryActorTick.bCanEverTick = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(SceneRoot);
    SpringArm->SetRelativeRotation(FRotator(-48.0f, -45.0f, 0.0f));
    SpringArm->bDoCollisionTest = false;
    SpringArm->bEnableCameraLag = true;
    SpringArm->CameraLagSpeed = 12.0f;

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
    Camera->ProjectionMode = ECameraProjectionMode::Orthographic;
    SetCameraFraming(3800.0f);

}

void AEchoesRTSCameraPawn::ApplyAuthoredPostProcess()
{
    if (Camera == nullptr)
    {
        return;
    }
    FPostProcessSettings& Settings = Camera->PostProcessSettings;

    // REL-ART-019: equal histogram limits hold exposure during tactical pans.
    // Site lighting supplies brightness differences; the camera does not adapt
    // to the amount of shroud or bright terrain in the frame. Review fixtures
    // may still apply their explicitly authored exposure bias below.
    Settings.bOverride_AutoExposureMethod = true;
    Settings.AutoExposureMethod = EAutoExposureMethod::AEM_Histogram;
    Settings.bOverride_AutoExposureMinBrightness = true;
    Settings.AutoExposureMinBrightness = 1.0f;
    Settings.bOverride_AutoExposureMaxBrightness = true;
    Settings.AutoExposureMaxBrightness = 1.0f;
    Settings.bOverride_AutoExposureSpeedUp = true;
    Settings.AutoExposureSpeedUp = 3.0f;
    Settings.bOverride_AutoExposureSpeedDown = true;
    Settings.AutoExposureSpeedDown = 1.0f;
    Settings.bOverride_AutoExposureBias = true;
    Settings.AutoExposureBias = 0.0f;

    // Filmic tonemapper chosen against the charcoal / pale-ceramic /
    // broken-sun-amber palette: a slightly relaxed slope with an earlier,
    // stronger shoulder and a tight white clip rolls ceramic and vitrified
    // glass into detail instead of clipping them to paper.
    Settings.bOverride_FilmSlope = true;
    Settings.FilmSlope = 0.84f;
    Settings.bOverride_FilmShoulder = true;
    Settings.FilmShoulder = 0.45f;
    Settings.bOverride_FilmWhiteClip = true;
    Settings.FilmWhiteClip = 0.012f;

    // The prototype's glare bleed came from default bloom over hot specular;
    // keep bloom present but restrained.
    Settings.bOverride_BloomIntensity = true;
    Settings.BloomIntensity = 0.3f;

    // Restrained contact shading keeps small feet and shaded equipment from
    // merging into wide black halos at RTS distance. The world-space radius
    // follows the equipment scale instead of expanding with camera distance.
    Settings.bOverride_AmbientOcclusionIntensity = true;
    Settings.AmbientOcclusionIntensity = 0.3f;
    Settings.bOverride_AmbientOcclusionRadius = true;
    Settings.AmbientOcclusionRadius = 80.0f;
    Settings.bOverride_AmbientOcclusionRadiusInWS = true;
    Settings.AmbientOcclusionRadiusInWS = true;

    // Stable edge and silhouette clarity across the whole tactical viewport.
    Settings.bOverride_MotionBlurAmount = true;
    Settings.MotionBlurAmount = 0.0f;
    Settings.bOverride_LensFlareIntensity = true;
    Settings.LensFlareIntensity = 0.0f;
    Settings.bOverride_SceneFringeIntensity = true;
    Settings.SceneFringeIntensity = 0.0f;
    Settings.bOverride_FilmGrainIntensity = true;
    Settings.FilmGrainIntensity = 0.0f;
    Settings.bOverride_VignetteIntensity = true;
    Settings.VignetteIntensity = 0.0f;

    Camera->PostProcessBlendWeight = 1.0f;

    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_EXPOSURE_AUTHORED] method=histogram minBrightness=%.2f maxBrightness=%.2f bias=%.2f filmSlope=%.2f filmShoulder=%.2f whiteClip=%.3f bloom=%.2f ao=%.2f aoRadiusCm=%.1f revision=exposure-authored-v2"),
        Settings.AutoExposureMinBrightness,
        Settings.AutoExposureMaxBrightness,
        Settings.AutoExposureBias,
        Settings.FilmSlope,
        Settings.FilmShoulder,
        Settings.FilmWhiteClip,
        Settings.BloomIntensity,
        Settings.AmbientOcclusionIntensity,
        Settings.AmbientOcclusionRadius);
}

void AEchoesRTSCameraPawn::BeginPlay()
{
    Super::BeginPlay();
    bEdgePanArmed = false;
    // The authored exposure/tonemapper baseline applies to every camera mode;
    // the non-shipping review fixtures below may override the bias only.
    ApplyAuthoredPostProcess();
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
        SetCameraFraming(ReviewConfiguration.CameraZoom, 52.0f);
        SpringArm->SetRelativeRotation(FRotator(-60.0f, -45.0f, 0.0f));
        SpringArm->bEnableCameraLag = false;
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
        SetCameraFraming(2500.0f, 52.0f);
        SpringArm->SetRelativeRotation(FRotator(-60.0f, -45.0f, 0.0f));
        SpringArm->bEnableCameraLag = false;
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
        SetCameraFraming(2500.0f, 52.0f);
        SpringArm->SetRelativeRotation(FRotator(-60.0f, -45.0f, 0.0f));
        SpringArm->bEnableCameraLag = false;
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
        const bool bVerticalSlice = GlassScarReviewMode.Equals(
            TEXT("VerticalSlice"),
            ESearchCase::IgnoreCase);
        const bool bFoldedVerge = GlassScarReviewMode.Equals(
            TEXT("FoldedVerge"),
            ESearchCase::IgnoreCase);
        const bool bBrokenSun = GlassScarReviewMode.Equals(
            TEXT("BrokenSun"),
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
        if (bVerticalSlice)
        {
            SetActorLocation(FVector(0.0f, 0.0f, 100.0f));
            // Gate 50 frame: both banks, the chasm floor, and the dais in one view. No
            // fixture-only exposure override: the frame renders under the A1 rig as play does.
            SetCameraFraming(3200.0f, 62.0f);
            SpringArm->SetRelativeRotation(FRotator(-19.0f, 43.0f, 0.0f));
        }
        else if (bBrokenSun)
        {
            SetActorLocation(FVector(2800.0f, -2000.0f, 650.0f));
            // Perspective used an arm length of zero for this close review
            // frame. Orthographic scale needs a finite width, so the shared
            // conversion applies its documented minimum framing distance.
            SetCameraFraming(0.0f, 72.0f);
            SpringArm->SetRelativeRotation(FRotator(32.0f, 145.0f, 0.0f));
            Camera->PostProcessSettings.bOverride_AutoExposureBias = true;
            Camera->PostProcessSettings.AutoExposureBias = 0.0f;
        }
        else
        {
            SetCameraFraming(
                bOverview
                    ? 10800.0f
                    : (bFoldedVerge ? 3350.0f : (bBuriedCauseway ? 2850.0f : 2300.0f)),
                bOverview ? 58.0f : 52.0f);
            SpringArm->SetRelativeRotation(
                FRotator(bOverview ? -68.0f : -58.0f, bOverview ? -90.0f : -45.0f, 0.0f));
            Camera->PostProcessSettings.bOverride_AutoExposureBias = true;
            Camera->PostProcessSettings.AutoExposureBias = bOverview ? -0.05f : 0.15f;
        }
        SpringArm->bEnableCameraLag = false;
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
        SetCameraFraming(1350.0f);
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
    if (FParse::Param(
            FCommandLine::Get(), TEXT("EchoesNetworkVisualReview")))
    {
        bArtReviewMode = true;
        SetActorLocation(FVector(4000.0f, 4000.0f, 100.0f));
        SetCameraFraming(2600.0f, 52.0f);
        SpringArm->SetRelativeRotation(FRotator(-60.0f, -45.0f, 0.0f));
        SpringArm->bEnableCameraLag = false;
        Camera->PostProcessSettings.bOverride_AutoExposureBias = true;
        Camera->PostProcessSettings.AutoExposureBias = 0.10f;
        Camera->PostProcessBlendWeight = 1.0f;
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_NETWORK_VISUAL_REVIEW_CAMERA] scopedSeat=1 centerTile=(52,52) zoom=2600 editorOnly=true"));
        return;
    }
    bool bUseArtReviewCamera = FParse::Param(FCommandLine::Get(), TEXT("EchoesArtReview"));
#if WITH_EDITOR
    if (GetWorld() && GetWorld()->WorldType == EWorldType::PIE &&
        CVarEchoesEditorArtReviewCameraEnabled.GetValueOnGameThread() == 0)
        bUseArtReviewCamera = false;
#endif
    if (bUseArtReviewCamera)
    {
        bArtReviewMode = true;
        // Default: the local base. -EchoesArtReviewCenter=X,Y and
        // -EchoesArtReviewZoom= frame any world position at the unchanged
        // gameplay pitch and yaw, which is what a live-play capture must show.
        FVector Center(-4400.0f, -4400.0f, 100.0f);
        FString CenterText;
        if (FParse::Value(FCommandLine::Get(), TEXT("EchoesArtReviewCenter="), CenterText, /*bShouldStopOnSeparator*/ false))
        {
            FString XText;
            FString YText;
            if (CenterText.Split(TEXT(","), &XText, &YText))
            {
                Center.X = FCString::Atof(*XText);
                Center.Y = FCString::Atof(*YText);
            }
        }
        float Zoom = 1900.0f;
        FParse::Value(FCommandLine::Get(), TEXT("EchoesArtReviewZoom="), Zoom);
        // -EchoesArtReviewDelay= holds the capture so a scripted scout
        // (-EchoesArtReviewScout=X,Y, issued through the ordinary command
        // path) can reach and reveal the framed area first.
        FParse::Value(FCommandLine::Get(), TEXT("EchoesArtReviewDelay="), ArtReviewCaptureDelaySeconds);
#if WITH_EDITOR
        if (GIsEditor)
        {
            const FString EditorCenter = CVarEchoesEditorArtReviewCenter.GetValueOnGameThread();
            if (!EditorCenter.IsEmpty())
            {
                FVector Candidate;
                if (TryParseArtReviewPoint(EditorCenter, GetWorld(), Candidate))
                {
                    Center = Candidate;
                    Center.Z = 100.0f;
                    CenterText = EditorCenter;
                }
                else
                {
                    UE_LOG(LogEchoes, Error, TEXT("[ECHOES_EDITOR_ART_REVIEW_REJECTED] field=center value=%s"), *EditorCenter);
                }
            }
            const float EditorZoom = CVarEchoesEditorArtReviewZoom.GetValueOnGameThread();
            if (EditorZoom >= 0.0f)
            {
                if (FMath::IsFinite(EditorZoom) && EditorZoom >= 600.0f && EditorZoom <= 6000.0f)
                    Zoom = EditorZoom;
                else
                    UE_LOG(LogEchoes, Error, TEXT("[ECHOES_EDITOR_ART_REVIEW_REJECTED] field=zoom value=%.2f"), EditorZoom);
            }
            const float EditorDelay = CVarEchoesEditorArtReviewDelay.GetValueOnGameThread();
            if (EditorDelay >= 0.0f)
            {
                if (FMath::IsFinite(EditorDelay) && EditorDelay >= 2.0f && EditorDelay <= 600.0f)
                    ArtReviewCaptureDelaySeconds = EditorDelay;
                else
                    UE_LOG(LogEchoes, Error, TEXT("[ECHOES_EDITOR_ART_REVIEW_REJECTED] field=delay value=%.2f"), EditorDelay);
            }
        }
#endif
        // -EchoesArtReviewHighContrast renders the review in the high-contrast
        // HUD variant so the accessibility branch is captured, not assumed.
        if (FParse::Param(FCommandLine::Get(), TEXT("EchoesArtReviewHighContrast")))
        {
            if (UEchoesGameUserSettings* ReviewSettings = UEchoesGameUserSettings::Get())
            {
                bArtReviewContrastOverridden = true;
                bArtReviewContrastPrevious = ReviewSettings->IsHighContrastHudEnabled();
                ReviewSettings->SetHighContrastHudEnabled(true);
            }
        }
        ArtReviewCaptureDelaySeconds = FMath::Clamp(ArtReviewCaptureDelaySeconds, 2.0f, 600.0f);
        SetActorLocation(Center);
        SetCameraFraming(FMath::Clamp(Zoom, 600.0f, 6000.0f));
        SpringArm->bEnableCameraLag = false;
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_ART_REVIEW_CAMERA] localBaseCentered=%s center=(%.0f,%.0f) zoom=%.0f editorOnly=true"),
            CenterText.IsEmpty() ? TEXT("true") : TEXT("false"),
            Center.X,
            Center.Y,
            SpringArm->TargetArmLength);
        return;
    }
#endif
    SetActorLocation(FVector(-3000.0f, -3000.0f, 100.0f));
}

void AEchoesRTSCameraPawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (bArtReviewContrastOverridden)
    {
        if (UEchoesGameUserSettings* ReviewSettings = UEchoesGameUserSettings::Get())
        {
            ReviewSettings->SetHighContrastHudEnabled(bArtReviewContrastPrevious);
        }
        bArtReviewContrastOverridden = false;
    }
    Super::EndPlay(EndPlayReason);
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
    SynchronizeOrthographicFraming();

#if !UE_BUILD_SHIPPING
    if (bArtReviewMode && !bArtReviewScreenshotRequested)
    {
        // Editor sessions compile meshes, textures, and shaders
        // asynchronously; a capture before the queue drains photographs
        // fallback materials. Hold the timer until everything is built,
        // but cap asset wait so background distance-field tasks do not
        // permanently starve screenshot capture in fast benchmark runs.
        const int32 RemainingAssets = FAssetCompilingManager::Get().GetNumRemainingAssets();
        if (RemainingAssets > 0 && ArtReviewAssetWaitSeconds < 5.0f)
        {
            ArtReviewAssetWaitSeconds += DeltaSeconds;
            ArtReviewElapsedSeconds = 0.0f;
        }
        else
        {
            ArtReviewElapsedSeconds += DeltaSeconds;
        }
        if (!bArtReviewScoutIssued && ArtReviewElapsedSeconds >= 0.5f)
        {
            bArtReviewScoutIssued = true;
            FString ScoutText;
            bool bEditorScoutOverride = false;
            if (FParse::Value(
                    FCommandLine::Get(),
                    TEXT("EchoesArtReviewScout="),
                    ScoutText,
                    /*bShouldStopOnSeparator*/ false))
            {
                // Command-line behavior remains unchanged when no editor
                // override is active.
            }
#if WITH_EDITOR
            if (GIsEditor && FParse::Param(FCommandLine::Get(), TEXT("EchoesArtReview")))
            {
                const FString EditorScout = CVarEchoesEditorArtReviewScout.GetValueOnGameThread();
                if (!EditorScout.IsEmpty())
                {
                    ScoutText = EditorScout;
                    bEditorScoutOverride = true;
                }
            }
#endif
            if (!ScoutText.IsEmpty())
            {
                FString XText;
                FString YText;
                UEchoesSimulationSubsystem* ScoutBridge =
                    GetWorld() != nullptr
                        ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
                        : nullptr;
                const echoes::sim::Simulation* ScoutSimulation =
                    ScoutBridge != nullptr ? ScoutBridge->GetSimulation() : nullptr;
                FVector Target;
                bool bTargetValid = ScoutText.Split(TEXT(","), &XText, &YText);
#if WITH_EDITOR
                if (bEditorScoutOverride)
                    bTargetValid = TryParseArtReviewPoint(ScoutText, GetWorld(), Target);
#endif
                if (bTargetValid && ScoutSimulation != nullptr)
                {
                    if (!bEditorScoutOverride)
                        Target = FVector(FCString::Atof(*XText), FCString::Atof(*YText), 0.0f);
                    // A review capture of explored ground must be earned the
                    // way a player earns it: real move orders on the local
                    // player's own mobile units, resolved by the simulation.
                    ScoutBridge->SetScenarioPaused(false);
                    int32 Ordered = 0;
                    for (const echoes::sim::Entity& Entity : ScoutSimulation->Entities())
                    {
                        const bool bMobile =
                            Entity.type == echoes::sim::EntityType::Worker ||
                            Entity.type == echoes::sim::EntityType::Soldier ||
                            Entity.type == echoes::sim::EntityType::HeavyUnit ||
                            Entity.type == echoes::sim::EntityType::ScoutUnit;
                        const bool bScoutOnly = FParse::Param(FCommandLine::Get(), TEXT("EchoesArtReviewScoutOnly"));
                        if (Entity.owner != UEchoesSimulationSubsystem::LocalPlayerId || !bMobile ||
                            (bScoutOnly && Entity.type != echoes::sim::EntityType::ScoutUnit))
                        {
                            continue;
                        }
                        FString Feedback;
                        if (ScoutBridge->IssueCommand(
                                echoes::sim::CommandType::Move,
                                Entity.id,
                                0,
                                Target,
                                echoes::sim::FutureWellChoice::Dormant,
                                Feedback))
                        {
                            ++Ordered;
                        }
                    }
                    // -EchoesArtReviewSelectForce keeps the ordered combat force
                    // selected through the ordinary selection path, so the
                    // selection panel and command card are captured armed.
                    if (FParse::Param(FCommandLine::Get(), TEXT("EchoesArtReviewSelectForce")))
                    {
                        if (AEchoesPlayerController* ReviewController =
                                Cast<AEchoesPlayerController>(GetController()))
                        {
                            ReviewController->SelectCombatForce();
                        }
                    }
                    UE_LOG(
                        LogEchoes,
                        Display,
                        TEXT("[ECHOES_ART_REVIEW_SCOUT] target=(%.0f,%.0f) ordered=%d captureDelay=%.1f editorOnly=true"),
                        Target.X,
                        Target.Y,
                        Ordered,
                        ArtReviewCaptureDelaySeconds);
                }
                else if (bEditorScoutOverride)
                {
                    UE_LOG(LogEchoes, Error, TEXT("[ECHOES_EDITOR_ART_REVIEW_REJECTED] field=scout value=%s"), *ScoutText);
                }
            }
        }
        if (ArtReviewElapsedSeconds >= ArtReviewCaptureDelaySeconds)
        {
            // -EchoesArtReviewPauseMenu opens the field menu through the
            // ordinary toggle at capture time so the relocated key legend is
            // photographed, not assumed.
            if (FParse::Param(FCommandLine::Get(), TEXT("EchoesArtReviewPauseMenu")))
            {
                if (AEchoesPlayerController* MenuController =
                        Cast<AEchoesPlayerController>(GetController()))
                {
                    if (!MenuController->IsPauseMenuVisible())
                    {
                        MenuController->TogglePauseMenu();
                    }
                }
            }
            FAssetCompilingManager::Get().FinishAllCompilation();

            for (TActorIterator<AEchoesEntityView> It(GetWorld()); It; ++It)
            {
                AEchoesEntityView* View = *It;
                const FVector Loc = View->GetActorLocation();
                const bool bHidden = View->IsHidden();
                UStaticMeshComponent* MeshComp = View->GetBodyMesh();
                const bool bCompVis = MeshComp ? MeshComp->IsVisible() : false;
                UStaticMesh* Mesh = MeshComp ? MeshComp->GetStaticMesh() : nullptr;
                const FBoxSphereBounds Bounds = MeshComp ? MeshComp->Bounds : FBoxSphereBounds();
                UE_LOG(
                    LogEchoes,
                    Display,
                    TEXT("[ART_REVIEW_VIEW_AUDIT] id=%u type=%d faction=%d owner=%d loc=%s hidden=%d compVis=%d mesh=%s boundsOrigin=%s boundsBoxExt=%s"),
                    View->GetEntityId(),
                    static_cast<int32>(View->GetEntityType()),
                    static_cast<int32>(View->GetEntityFaction()),
                    View->GetOwnerPlayerId(),
                    *Loc.ToString(),
                    bHidden ? 1 : 0,
                    bCompVis ? 1 : 0,
                    Mesh ? *Mesh->GetName() : TEXT("None"),
                    *Bounds.Origin.ToString(),
                    *Bounds.BoxExtent.ToString());
            }

            FString OutputPath;
            FParse::Value(
                FCommandLine::Get(),
                TEXT("EchoesArtReviewOutput="),
                OutputPath);
#if WITH_EDITOR
            if (GIsEditor && FParse::Param(FCommandLine::Get(), TEXT("EchoesArtReview")))
            {
                const FString EditorOutput = CVarEchoesEditorArtReviewOutput.GetValueOnGameThread();
                if (!EditorOutput.IsEmpty()) OutputPath = EditorOutput;
            }
#endif
            const bool bShowUI = !FParse::Param(
                FCommandLine::Get(),
                TEXT("EchoesArtReviewHideUI"));
            if (!OutputPath.IsEmpty())
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
                TEXT("[ECHOES_ART_REVIEW_CAPTURE] requested=true showUI=%s delay=%.1f output=%s"),
                bShowUI ? TEXT("true") : TEXT("false"),
                ArtReviewCaptureDelaySeconds,
                OutputPath.IsEmpty() ? TEXT("default") : *OutputPath);
        }
    }
#endif

#if !UE_BUILD_SHIPPING
    // Authoring captures must not depend on the desktop pointer position.
    if (bArtReviewMode)
    {
        SpringArm->bEnableCameraLag = false;
        return;
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
    SetCameraFraming(FMath::Clamp(
        SpringArm->TargetArmLength + ZoomStep * ZoomScale * Direction,
        MinimumZoom,
        MaximumZoom));
}

void AEchoesRTSCameraPawn::SetCameraFraming(
    float LegacyArmLength,
    float LegacyFieldOfViewDegrees)
{
    check(SpringArm != nullptr);
    check(Camera != nullptr);

    ActiveFramingFieldOfViewDegrees = LegacyFieldOfViewDegrees;
    SpringArm->TargetArmLength = FMath::Max(0.0f, LegacyArmLength);
    SynchronizeOrthographicFraming();
}

void AEchoesRTSCameraPawn::SynchronizeOrthographicFraming()
{
    check(SpringArm != nullptr);
    check(Camera != nullptr);

    // The authored review presets and player zoom values were specified as a
    // perspective camera's distance and horizontal FOV. Preserve their
    // on-ground framing with width = 2 * distance * tan(horizontalFOV / 2).
    // A zero-arm cinematic preset has no perspective distance to convert, so
    // use the smallest authored review distance (600 cm) for a stable close
    // orthographic frame rather than producing an invalid zero-width camera.
    constexpr float MinimumFramingDistance = 600.0f;
    const float SafeArmLength = FMath::Max(0.0f, SpringArm->TargetArmLength);
    const float SafeFieldOfView = FMath::Clamp(ActiveFramingFieldOfViewDegrees, 1.0f, 170.0f);
    const float ProjectionDistance = FMath::Max(SafeArmLength, MinimumFramingDistance);
    const float EquivalentOrthoWidth = 2.0f * ProjectionDistance *
        FMath::Tan(FMath::DegreesToRadians(SafeFieldOfView * 0.5f));

    SpringArm->TargetArmLength = SafeArmLength;
    Camera->ProjectionMode = ECameraProjectionMode::Orthographic;
    Camera->SetOrthoWidth(EquivalentOrthoWidth);
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
