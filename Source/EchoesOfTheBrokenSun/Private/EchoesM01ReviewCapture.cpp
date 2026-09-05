// Author and owner: Angelis Pseftis
// Editor evidence only. Never changes authoritative simulation or shipping input.
#if WITH_EDITOR
#include "Algo/AllOf.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesGameUserSettings.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "MovieSceneCapture.h"
#include "MovieSceneCaptureModule.h"
#include "Protocols/AudioCaptureProtocol.h"
#include "Protocols/VideoCaptureProtocol.h"
#include "Serialization/JsonSerializer.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "Slate/SceneViewport.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SViewport.h"
#include "Widgets/SWindow.h"
#include "Framework/Application/SlateApplication.h"

namespace
{
// Temporary review settings, restored on owned PIE teardown. No SaveSettings
// call: an accessibility inspection must not replace the player's preferences.
struct FM01AccessibilityPreview
{
    bool bOverridden = false;
    float PriorScale = 1.0f;
    bool bPriorContrast = false, bPriorMotion = false, bPriorFlash = false;
    FDelegateHandle EndPIE;
    FM01AccessibilityPreview()
    {
        EndPIE = FEditorDelegates::EndPIE.AddLambda([this](bool) { Restore(); });
    }
    void Restore()
    {
        if (!bOverridden) return;
        if (auto* Settings = UEchoesGameUserSettings::Get())
        {
            Settings->SetHudScale(PriorScale);
            Settings->SetHighContrastHudEnabled(bPriorContrast);
            Settings->SetReducedMotionEnabled(bPriorMotion);
            Settings->SetReducedFlashingEnabled(bPriorFlash);
        }
        bOverridden = false;
        UE_LOG(LogTemp, Display, TEXT("[ECHOES_M01_ACCESSIBILITY_RESTORED] savedConfig=false"));
    }
    void Apply(const TArray<FString>& Args)
    {
        if (Args.Num() == 1 && Args[0] == TEXT("restore")) { Restore(); return; }
        float Scale = 0.0f; int32 Contrast = 0, Motion = 0, Flash = 0;
        if (Args.Num() != 4 || !LexTryParseString(Scale, *Args[0]) || !FMath::IsFinite(Scale) ||
            Scale < .8f || Scale > 1.5f || !LexTryParseString(Contrast, *Args[1]) ||
            !LexTryParseString(Motion, *Args[2]) || !LexTryParseString(Flash, *Args[3]) ||
            Contrast < 0 || Contrast > 1 || Motion < 0 || Motion > 1 || Flash < 0 || Flash > 1) return;
        UWorld* World = nullptr;
        for (const FWorldContext& Context : GEngine->GetWorldContexts())
            if (Context.WorldType == EWorldType::PIE) { if (World) return; World = Context.World(); }
        auto* Simulation = World ? World->GetSubsystem<UEchoesSimulationSubsystem>() : nullptr;
        auto* Settings = UEchoesGameUserSettings::Get();
        if (!Simulation || Simulation->GetOperationMode() != EEchoesOperationMode::CampaignPrologue || !Settings) return;
        if (!bOverridden)
        {
            PriorScale = Settings->GetHudScale();
            bPriorContrast = Settings->IsHighContrastHudEnabled();
            bPriorMotion = Settings->IsReducedMotionEnabled();
            bPriorFlash = Settings->IsReducedFlashingEnabled();
            bOverridden = true;
        }
        Settings->SetHudScale(Scale);
        Settings->SetHighContrastHudEnabled(Contrast != 0);
        Settings->SetReducedMotionEnabled(Motion != 0);
        Settings->SetReducedFlashingEnabled(Flash != 0);
        UE_LOG(LogTemp, Display, TEXT("[ECHOES_M01_ACCESSIBILITY_PREVIEW] hud=%.2f contrast=%d reducedMotion=%d reducedFlash=%d savedConfig=false"),
            Scale, Contrast, Motion, Flash);
    }
    ~FM01AccessibilityPreview() { Restore(); FEditorDelegates::EndPIE.Remove(EndPIE); }
};
FM01AccessibilityPreview M01AccessibilityPreview;
FAutoConsoleCommand AccessibilityM01Command(TEXT("Echoes.EditorAccessibilityPreview"),
    TEXT("M01 PIE temporary preview: <scale .8..1.5> <contrast0|1> <reducedMotion0|1> <reducedFlash0|1>, or restore."),
    FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args) { M01AccessibilityPreview.Apply(Args); }));

// Base UMovieSceneCapture uses real time. ULevelCapture's fixed timestep would
// change the mission being observed and is deliberately not used here.
struct FM01ReviewCapture
{
    TStrongObjectPtr<UMovieSceneCapture> Capture;
    TWeakObjectPtr<UWorld> World;
    TWeakPtr<FSceneViewport> Viewport;
    FTSTicker::FDelegateHandle Ticker;
    FString OutputDirectory;
    FIntPoint OriginalSize = FIntPoint::ZeroValue;
    FIntPoint SlateSize = FIntPoint::ZeroValue;
    bool bWasFixed = false;
    bool bDraining = false;
    bool bFinished = false;
    bool bAborted = false;
    double StartedAt = 0;
    double Duration = 0;

    void WriteReceipt(const TCHAR* State)
    {
        const auto Json = MakeShared<FJsonObject>();
        Json->SetStringField(TEXT("author"), TEXT("Angelis Pseftis"));
        Json->SetStringField(TEXT("evidence_class"), TEXT("EDT live viewport; video only; not performance evidence"));
        Json->SetStringField(TEXT("state"), State);
        Json->SetStringField(TEXT("utc"), FDateTime::UtcNow().ToIso8601());
        Json->SetStringField(TEXT("output_directory"), OutputDirectory);
        const FString MoviePath = OutputDirectory / TEXT("M01_motion.mov");
        Json->SetStringField(TEXT("video_path"), MoviePath);
        Json->SetNumberField(TEXT("video_bytes"), IFileManager::Get().FileSize(*MoviePath));
        Json->SetBoolField(TEXT("aborted"), bAborted);
        Json->SetBoolField(TEXT("original_fixed_size"), bWasFixed);
        Json->SetNumberField(TEXT("original_width"), OriginalSize.X);
        Json->SetNumberField(TEXT("original_height"), OriginalSize.Y);
        Json->SetNumberField(TEXT("slate_width"), SlateSize.X);
        Json->SetNumberField(TEXT("slate_height"), SlateSize.Y);
        Json->SetBoolField(TEXT("viewport_resized_by_capture"), false);
        Json->SetNumberField(TEXT("requested_seconds"), Duration);
        Json->SetNumberField(TEXT("wall_seconds"), FPlatformTime::Seconds() - StartedAt);
        if (Capture.IsValid())
        {
            const FCachedMetrics& Metrics = Capture->GetMetrics();
            Json->SetNumberField(TEXT("width"), Metrics.Width);
            Json->SetNumberField(TEXT("height"), Metrics.Height);
            Json->SetNumberField(TEXT("frame_index_including_drops"), Metrics.Frame);
            Json->SetNumberField(TEXT("capture_elapsed_seconds"), Metrics.ElapsedSeconds);
            Json->SetNumberField(TEXT("fps"), Capture->Settings.FrameRate.AsDecimal());
        }
        FString Serialized;
        FJsonSerializer::Serialize(Json, TJsonWriterFactory<>::Create(&Serialized));
        FFileHelper::SaveStringToFile(Serialized, *(OutputDirectory / TEXT("capture.json")));
    }

    void Finished()
    {
        bFinished = true;
        Viewport.Reset();
        const bool bVideoExists = IFileManager::Get().FileSize(*(OutputDirectory / TEXT("M01_motion.mov"))) > 0;
        const TCHAR* State = bAborted ? TEXT("ABORTED") :
            bVideoExists ? TEXT("FINALIZED") : TEXT("FINALIZED_NO_VIDEO");
        WriteReceipt(State);
        UE_LOG(LogTemp, Display, TEXT("[ECHOES_M01_MOTION_FINISHED] state=%s path=%s"), State, *OutputDirectory);
        // Keep the UObject alive through its own Finalize stack. Tick releases it.
    }

    bool Tick(float)
    {
        if (bFinished)
        {
            Capture.Reset();
            Ticker.Reset();
            return false;
        }
        if (!World.IsValid() || World->bIsTearingDown || !Viewport.IsValid() ||
            (bDraining && FPlatformTime::Seconds() - StartedAt > Duration + 15.0))
        {
            bAborted = true;
            Capture->Finalize();
        }
        else if (!bDraining && FPlatformTime::Seconds() - StartedAt >= Duration)
        {
            bDraining = true;
            Capture->FinalizeWhenReady();
        }
        return true;
    }

    void Start(const TArray<FString>& Args)
    {
        int32 Seconds = 0, FPS = 0;
        if (Args.Num() != 3 || !LexTryParseString(Seconds, *Args[0]) ||
            !LexTryParseString(FPS, *Args[1]) || Seconds < 5 || Seconds > 180 ||
            (FPS != 24 && FPS != 30) || Args[2].IsEmpty() || Args[2].Len() > 48 ||
            !Algo::AllOf(Args[2], [](TCHAR C) { return FChar::IsAlnum(C) || C == '-' || C == '_'; }))
        {
            UE_LOG(LogTemp, Warning, TEXT("[ECHOES_M01_MOTION_REFUSED] expected: seconds[5..180] fps[24|30] label[letters/digits/-/_]"));
            return;
        }
        if (Capture.IsValid() || IMovieSceneCaptureModule::Get().GetFirstActiveMovieSceneCapture())
        {
            UE_LOG(LogTemp, Warning, TEXT("[ECHOES_M01_MOTION_REFUSED] another capture is active"));
            return;
        }
        UWorld* PIEWorld = nullptr;
        int32 PIEInstance = INDEX_NONE;
        for (const FWorldContext& Context : GEngine->GetWorldContexts())
            if (Context.WorldType == EWorldType::PIE)
            {
                if (PIEWorld) return; // Ambiguous multi-client session.
                PIEWorld = Context.World();
                PIEInstance = Context.PIEInstance;
            }
        const auto* Simulation = PIEWorld ? PIEWorld->GetSubsystem<UEchoesSimulationSubsystem>() : nullptr;
        UGameViewportClient* Client = PIEWorld ? PIEWorld->GetGameViewport() : nullptr;
        const auto Widget = Client ? Client->GetGameViewportWidget() : nullptr;
        const auto Scene = Widget ? StaticCastSharedPtr<FSceneViewport>(Widget->GetViewportInterface().Pin()) : nullptr;
        if (!Simulation || Simulation->GetOperationMode() != EEchoesOperationMode::CampaignPrologue ||
            !Scene || Scene.Get() != Client->GetGameViewport())
        {
            UE_LOG(LogTemp, Warning, TEXT("[ECHOES_M01_MOTION_REFUSED] requires one live M01 PIE viewport"));
            return;
        }
        OriginalSize = Scene->GetSize();
        if (OriginalSize.X < 64 || OriginalSize.Y < 16 || OriginalSize.X > 3840 || OriginalSize.Y > 2160)
        {
            UE_LOG(LogTemp, Warning, TEXT("[ECHOES_M01_MOTION_REFUSED] viewport outside 64..3840 by 16..2160 capture budget"));
            return;
        }
        const auto CaptureWidget = Scene->GetViewportWidget().Pin();
        const FVector2D AbsoluteSize = CaptureWidget ?
            CaptureWidget->GetCachedGeometry().GetAbsoluteSize() : FVector2D::ZeroVector;
        SlateSize = FIntPoint(FMath::RoundToInt(AbsoluteSize.X), FMath::RoundToInt(AbsoluteSize.Y));
        // Mac AVIWriter ignores CoreVideo row padding; FrameGrabber also needs
        // its target and arranged Slate capture rectangle to agree. Resizing
        // only the render target produces a partial raster with magenta borders.
        if (OriginalSize != SlateSize || OriginalSize.X % 64 != 0 || OriginalSize.Y % 2 != 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("[ECHOES_M01_MOTION_REFUSED] require matching native/Slate size, width divisible by64 and even height; native=%dx%d slate=%dx%d. Use a1280x720 or1280x960 New Editor Window PIE."),
                OriginalSize.X, OriginalSize.Y, SlateSize.X, SlateSize.Y);
            return;
        }
        OutputDirectory = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() /
            TEXT("BuildArtifacts/Evidence") / FString::Printf(TEXT("m01-motion-%s-%s-%s"),
                *Args[2], *FDateTime::UtcNow().ToString(TEXT("%Y%m%dT%H%M%SZ")),
                *FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(8)));
        if (IFileManager::Get().DirectoryExists(*OutputDirectory) ||
            !IFileManager::Get().MakeDirectory(*OutputDirectory, true) ||
            !FFileHelper::SaveStringToFile(TEXT("Preparing M01 video capture; no audio track.\n"),
                *(OutputDirectory / TEXT("capture-start.txt")))) return;

        World = PIEWorld;
        Viewport = Scene;
        bWasFixed = Scene->HasFixedSize();
        bDraining = bFinished = bAborted = false;
        Duration = Seconds;
        StartedAt = FPlatformTime::Seconds();
        Capture.Reset(NewObject<UMovieSceneCapture>(GetTransientPackage()));
        auto& Settings = Capture->Settings;
        Settings.OutputDirectory.Path = OutputDirectory;
        Settings.OutputFormat = TEXT("M01_motion");
        Settings.bOverwriteExisting = false;
        Settings.FrameRate = Settings.CustomFrameRate = FFrameRate(FPS, 1);
        Settings.bUseCustomFrameRate = true;
        Settings.bEnableTextureStreaming = true;
        Settings.bCinematicEngineScalability = Settings.bCinematicMode = false;
        Settings.bAllowMovement = Settings.bAllowTurning = Settings.bShowPlayer = Settings.bShowHUD = true;
        Settings.bUsePathTracer = false;
        Capture->SetImageCaptureProtocolType(UVideoCaptureProtocol::StaticClass());
        Capture->SetAudioCaptureProtocolType(UNullAudioCaptureProtocol::StaticClass());
        CastChecked<UVideoCaptureProtocol>(Capture->GetImageCaptureProtocol())->bUseCompression = false;
        Capture->OnCaptureFinished().AddRaw(this, &FM01ReviewCapture::Finished);
        Capture->Initialize(Scene, PIEInstance);
        const float Volume = FApp::GetVolumeMultiplier();
        const float UnfocusedVolume = FApp::GetUnfocusedVolumeMultiplier();
        Capture->StartCapture();
        FApp::SetVolumeMultiplier(Volume);
        FApp::SetUnfocusedVolumeMultiplier(UnfocusedVolume);
        Ticker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateRaw(this, &FM01ReviewCapture::Tick));
        WriteReceipt(TEXT("CAPTURING"));
        UE_LOG(LogTemp, Display, TEXT("[ECHOES_M01_MOTION_STARTED] seconds=%d fps=%d size=%dx%d audio=none path=%s"),
            Seconds, FPS, Scene->GetSize().X, Scene->GetSize().Y, *OutputDirectory);
    }

    ~FM01ReviewCapture()
    {
        if (Ticker.IsValid()) FTSTicker::GetCoreTicker().RemoveTicker(Ticker);
        if (Capture.IsValid() && !bFinished)
        {
            bAborted = true;
            Capture->Finalize();
        }
        Viewport.Reset();
    }
};

FM01ReviewCapture M01ReviewCapture;
FAutoConsoleCommand ResizeM01WindowCommand(TEXT("Echoes.EditorResizeM01"),
    TEXT("Fit the floating M01 viewport to <width> <height>; call again after a Slate tick to verify. At most two adjustments per target/world."),
    FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
    {
        int32 Width = 0, Height = 0;
        if (Args.Num() != 2 || !LexTryParseString(Width, *Args[0]) || !LexTryParseString(Height, *Args[1]) ||
            Width < 640 || Width > 3840 || Height < 360 || Height > 2160 || Width % 64 || Height % 2 ||
            M01ReviewCapture.Capture.IsValid()) return;
        UWorld* World = nullptr;
        for (const FWorldContext& Context : GEngine->GetWorldContexts())
            if (Context.WorldType == EWorldType::PIE) { if (World) return; World = Context.World(); }
        auto* Simulation = World ? World->GetSubsystem<UEchoesSimulationSubsystem>() : nullptr;
        auto* Client = World ? World->GetGameViewport() : nullptr;
        auto* Scene = Client ? Client->GetGameViewport() : nullptr;
        const auto Widget = Scene ? Scene->GetViewportWidget().Pin() : nullptr;
        const auto Window = Widget ? FSlateApplication::Get().FindWidgetWindow(Widget.ToSharedRef()) : nullptr;
        if (!Simulation || Simulation->GetOperationMode() != EEchoesOperationMode::CampaignPrologue ||
            !Window || Window->GetTag() != TEXT("PIEWindow") || !Window->IsViewportSizeDrivenByWindow() ||
            Scene->HasFixedSize()) return;
        const FIntPoint Target(Width, Height);
        const FVector2D Absolute = Widget->GetCachedGeometry().GetAbsoluteSize();
        const FIntPoint Slate(FMath::RoundToInt(Absolute.X), FMath::RoundToInt(Absolute.Y));
        static TWeakObjectPtr<UWorld> PriorWorld;
        static FIntPoint PriorTarget = FIntPoint::ZeroValue;
        static int32 Adjustments = 0;
        if (World != PriorWorld || Target != PriorTarget) { PriorWorld = World; PriorTarget = Target; Adjustments = 0; }
        if (Scene->GetSizeXY() == Target && Slate == Target)
        {
            UE_LOG(LogTemp, Display, TEXT("[ECHOES_M01_VIEWPORT_READY] native=%dx%d slate=%dx%d adjustments=%d"),
                Width, Height, Slate.X, Slate.Y, Adjustments);
            return;
        }
        if (Adjustments >= 2)
        {
            UE_LOG(LogTemp, Warning, TEXT("[ECHOES_M01_VIEWPORT_REFUSED] did not reach target after two native-window adjustments"));
            return;
        }
        const FVector2D ClientSize = Window->GetClientSizeInScreen();
        Window->Resize(ClientSize + FVector2D(Width - Slate.X, Height - Slate.Y));
        ++Adjustments;
        UE_LOG(LogTemp, Display, TEXT("[ECHOES_M01_VIEWPORT_RESIZE_REQUESTED] target=%dx%d previousNative=%dx%d previousSlate=%dx%d client=%.0fx%.0f adjustment=%d"),
            Width, Height, Scene->GetSizeXY().X, Scene->GetSizeXY().Y, Slate.X, Slate.Y, ClientSize.X, ClientSize.Y, Adjustments);
    }));
FAutoConsoleCommand StartM01FloatingPIECommand(TEXT("Echoes.EditorStartM01FloatingPIE"),
    TEXT("Open the configured M01 editor session in a native 1280x720 play window; refuses an existing session."),
    FConsoleCommandDelegate::CreateLambda([]()
    {
        const auto* Settings = GetDefault<ULevelEditorPlaySettings>();
        if (!GEditor || GEditor->IsPlaySessionInProgress() || GEditor->bIsCompiling ||
            GEditor->IsLightingBuildCurrentlyRunning() || M01ReviewCapture.Capture.IsValid() ||
            IMovieSceneCaptureModule::Get().GetFirstActiveMovieSceneCapture() ||
            Settings->NewWindowWidth != 1280 || Settings->NewWindowHeight != 720)
        {
            UE_LOG(LogTemp, Warning, TEXT("[ECHOES_M01_FLOATING_PIE_REFUSED] requires an idle editor, no capture, and 1280x720 play settings"));
            return;
        }
        // Match the engine's floating-PIE action: no DestinationSlateViewport.
        // Never replace or resize an existing session.
        FRequestPlaySessionParams Params;
        GEditor->RequestPlaySession(Params);
        UE_LOG(LogTemp, Display, TEXT("[ECHOES_M01_FLOATING_PIE_REQUESTED] native play window; editor diagnostic"));
    }));
FAutoConsoleCommand CaptureM01Command(TEXT("Echoes.EditorCaptureM01"),
    TEXT("Capture live M01 PIE video: <seconds 5..180> <fps 24|30> <label>. No audio or performance claim."),
    FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args) { M01ReviewCapture.Start(Args); }));
}
#endif
