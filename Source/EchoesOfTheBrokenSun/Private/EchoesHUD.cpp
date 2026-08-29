#include "EchoesHUD.h"

#include "EchoesEntityView.h"
#include "EchoesGameUserSettings.h"
#include "EchoesOfTheBrokenSun.h"
#include "EchoesPlayerController.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

namespace
{
FLinearColor MinimapOwnerColor(uint8 Owner, bool bHighContrast)
{
    if (bHighContrast)
    {
        switch (Owner)
        {
            case 0: return FLinearColor(0.1f, 0.95f, 1.0f);
            case 1: return FLinearColor(1.0f, 0.35f, 0.12f);
            case 2: return FLinearColor(1.0f, 0.9f, 0.1f);
            case 3: return FLinearColor(0.86f, 0.55f, 1.0f);
            default: return FLinearColor::White;
        }
    }
    switch (Owner)
    {
        case 0: return FLinearColor(0.04f, 0.72f, 0.88f);
        case 1: return FLinearColor(0.92f, 0.30f, 0.05f);
        case 2: return FLinearColor(0.95f, 0.74f, 0.08f);
        case 3: return FLinearColor(0.62f, 0.30f, 0.95f);
        default: return FLinearColor(0.72f, 0.72f, 0.72f);
    }
}
}

void AEchoesHUD::DrawHUD()
{
    Super::DrawHUD();

    const AEchoesPlayerController* EchoesController =
        Cast<AEchoesPlayerController>(GetOwningPlayerController());
    const UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    const echoes::sim::Simulation* Sim =
        Bridge != nullptr ? Bridge->GetSimulation() : nullptr;

    const UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get();
    const float HudScale = Settings != nullptr ? Settings->GetHudScale() : 1.0f;
    const bool bHighContrast =
        Settings != nullptr && Settings->IsHighContrastHudEnabled();
    const float TextX = 34.0f;
    const auto HudY = [HudScale](float Offset)
    {
        return 18.0f + Offset * HudScale;
    };
    const float MaximumPanelWidth =
        Canvas != nullptr ? FMath::Max(320.0f, Canvas->ClipX - 36.0f) : 920.0f;
    const float PanelWidth = FMath::Min(920.0f * HudScale, MaximumPanelWidth);
    const FLinearColor PanelColor =
        bHighContrast
            ? FLinearColor(0.0f, 0.0f, 0.0f, 0.98f)
            : FLinearColor(0.008f, 0.018f, 0.035f, 0.88f);
    const FLinearColor AccentColor =
        bHighContrast
            ? FLinearColor(1.0f, 0.9f, 0.1f)
            : FLinearColor(0.15f, 0.88f, 1.0f);
    const FLinearColor SecondaryColor =
        bHighContrast ? FLinearColor::White : FLinearColor(0.73f, 0.76f, 0.82f);

    if (EchoesController != nullptr && EchoesController->IsMissionBriefingVisible())
    {
        DrawMissionBriefing(EchoesController, Settings);
        return;
    }

    DrawRect(PanelColor, 18.0f, 18.0f, PanelWidth, 276.0f * HudScale);
    DrawText(
        TEXT("ECHOES OF THE BROKEN SUN  |  PLAYABLE SYSTEMS BUILD — ACTIVE DEVELOPMENT"),
        AccentColor,
        TextX,
        HudY(13.0f),
        GEngine != nullptr ? GEngine->GetSmallFont() : nullptr,
        1.08f * HudScale,
        false);

    FString ResourceLine = TEXT("Simulation unavailable");
    if (Sim != nullptr)
    {
        const echoes::sim::PlayerState* Player =
            Sim->FindPlayer(UEchoesSimulationSubsystem::LocalPlayerId);
        if (Player != nullptr)
        {
            FString MatchState = Bridge != nullptr && Bridge->IsScenarioPaused()
                                     ? TEXT("PAUSED")
                                     : TEXT("ACTIVE");
            const echoes::sim::MatchOutcome Outcome = Sim->Outcome();
            if (Outcome == echoes::sim::MatchOutcome::Player0Victory)
            {
                MatchState = TEXT("VICTORY");
            }
            else if (Outcome == echoes::sim::MatchOutcome::Player1Victory ||
                     Outcome == echoes::sim::MatchOutcome::Player2Victory ||
                     Outcome == echoes::sim::MatchOutcome::Player3Victory)
            {
                MatchState = TEXT("DEFEAT");
            }
            else if (Outcome == echoes::sim::MatchOutcome::Draw)
            {
                MatchState = TEXT("DRAW");
            }
            ResourceLine = FString::Printf(
                TEXT("Matter  %d     Dawnshards  %d     Logistics  %d/%d     %s     Tick  %llu @ %u Hz"),
                Player->resources.material,
                Player->resources.dawnshards,
                Sim->PopulationUsed(UEchoesSimulationSubsystem::LocalPlayerId),
                Sim->PopulationCapacity(UEchoesSimulationSubsystem::LocalPlayerId),
                *MatchState,
                static_cast<unsigned long long>(Sim->CurrentTick()),
                Sim->Config().ticksPerSecond);
        }
    }
    DrawText(
        ResourceLine,
        FLinearColor::White,
        TextX,
        HudY(40.0f),
        GEngine != nullptr ? GEngine->GetSmallFont() : nullptr,
        1.0f * HudScale,
        false);

    FString SelectionLine = TEXT("Selected  0");
    if (EchoesController != nullptr)
    {
        const TArray<uint32>& SelectedIds = EchoesController->GetSelectedEntityIds();
        FString SelectedType;
        if (SelectedIds.Num() == 1 && Bridge != nullptr)
        {
            if (const AEchoesEntityView* View = Bridge->FindEntityView(SelectedIds[0]))
            {
                SelectedType = FString::Printf(TEXT(" (%s)"), *View->GetDisplayName());
            }
            if (const echoes::sim::Entity* Entity =
                    Bridge->FindEntity(SelectedIds[0]);
                Entity != nullptr && Entity->productionRequired > 0)
            {
                const int32 Percent = FMath::Clamp(
                    Entity->productionProgress * 100 /
                        FMath::Max(1, Entity->productionRequired),
                    0,
                    100);
                SelectedType += FString::Printf(
                    TEXT(" — producing %s %d%%"),
                    Entity->productionType == echoes::sim::EntityType::Worker
                        ? TEXT("Worker")
                        : TEXT("Soldier"),
                    Percent);
            }
        }
        SelectionLine = FString::Printf(
            TEXT("Selected  %d%s     Future Well protocol  %s"),
            SelectedIds.Num(),
            *SelectedType,
            *EchoesController->GetFutureWellChoiceLabel());
    }
    DrawText(
        SelectionLine,
        FLinearColor(0.76f, 0.92f, 1.0f),
        TextX,
        HudY(64.0f),
        GEngine != nullptr ? GEngine->GetSmallFont() : nullptr,
        1.0f * HudScale,
        false);

    DrawText(
        TEXT("WASD / screen edge: pan    Wheel: zoom    LMB / drag: select    Shift: add/remove    RMB: context order"),
        SecondaryColor,
        TextX,
        HudY(90.0f),
        GEngine != nullptr ? GEngine->GetSmallFont() : nullptr,
        0.86f * HudScale,
        false);
    DrawText(
        TEXT("[F] Attack-move  [T] Patrol  [H] Hold  [J] Guard  [X] Stop  [B] Barracks  [N] Drop-off  [Q] Worker  [E] Soldier"),
        SecondaryColor,
        TextX,
        HudY(113.0f),
        GEngine != nullptr ? GEngine->GetSmallFont() : nullptr,
        0.86f * HudScale,
        false);
    DrawText(
        TEXT("[1-0] Recall group    [G then 1-0] Assign group    [P] Pause    [R] Restart"),
        SecondaryColor,
        TextX,
        HudY(136.0f),
        GEngine != nullptr ? GEngine->GetSmallFont() : nullptr,
        0.86f * HudScale,
        false);
    DrawText(
        TEXT("[Z] Harvest    [C] Preserve    [V] Reshape    Cyan: Meridian    Red: Kharuun"),
        SecondaryColor,
        TextX,
        HudY(159.0f),
        GEngine != nullptr ? GEngine->GetSmallFont() : nullptr,
        0.86f * HudScale,
        false);
    DrawText(
        TEXT("[K] Checkpoint    [L] Load Checkpoint    Validated with one backup"),
        SecondaryColor,
        TextX,
        HudY(182.0f),
        GEngine != nullptr ? GEngine->GetSmallFont() : nullptr,
        0.86f * HudScale,
        false);

    const FString SettingsLine = FString::Printf(
        TEXT("[U] UI %d%%  [I] Contrast %s  [O] Reduced motion %s  [/] Reduced flash %s  [Y] Edge pan %s"),
        FMath::RoundToInt(HudScale * 100.0f),
        bHighContrast ? TEXT("ON") : TEXT("OFF"),
        Settings != nullptr && Settings->IsReducedMotionEnabled() ? TEXT("ON") : TEXT("OFF"),
        Settings != nullptr && Settings->IsReducedFlashingEnabled() ? TEXT("ON") : TEXT("OFF"),
        Settings == nullptr || Settings->IsEdgePanEnabled() ? TEXT("ON") : TEXT("OFF"));
    DrawText(
        SettingsLine,
        AccentColor,
        TextX,
        HudY(205.0f),
        GEngine != nullptr ? GEngine->GetSmallFont() : nullptr,
        0.86f * HudScale,
        false);

    const FString CameraSettingsLine = FString::Printf(
        TEXT("[Left/Right bracket] Pan speed %d%%    [Comma/Period] Zoom step %d%%"),
        FMath::RoundToInt(
            (Settings != nullptr ? Settings->GetCameraPanSpeedScale() : 1.0f) * 100.0f),
        FMath::RoundToInt(
            (Settings != nullptr ? Settings->GetCameraZoomScale() : 1.0f) * 100.0f));
    DrawText(
        CameraSettingsLine,
        SecondaryColor,
        TextX,
        HudY(228.0f),
        GEngine != nullptr ? GEngine->GetSmallFont() : nullptr,
        0.86f * HudScale,
        false);

    if (EchoesController != nullptr)
    {
        const FString Feedback = EchoesController->GetStatusMessage();
        if (!Feedback.IsEmpty())
        {
            const float FeedbackWidth = FMath::Min(
                920.0f,
                Canvas != nullptr ? Canvas->ClipX - 36.0f : 920.0f);
            DrawRect(
                PanelColor,
                18.0f,
                Canvas != nullptr ? Canvas->ClipY - 72.0f : 700.0f,
                FeedbackWidth,
                48.0f);
            DrawText(
                Feedback,
                Feedback.StartsWith(TEXT("["))
                    ? FLinearColor(1.0f, 0.48f, 0.18f)
                    : FLinearColor(0.25f, 1.0f, 0.66f),
                TextX,
                Canvas != nullptr ? Canvas->ClipY - 58.0f : 714.0f,
                GEngine != nullptr ? GEngine->GetSmallFont() : nullptr,
                0.95f * HudScale,
                false);
        }
    }

    DrawTacticalMinimap(Bridge, EchoesController, Settings);
    DrawSelectionRectangle();
}

void AEchoesHUD::DrawMissionBriefing(
    const AEchoesPlayerController* EchoesController,
    const UEchoesGameUserSettings* Settings)
{
    if (Canvas == nullptr || EchoesController == nullptr ||
        !EchoesController->IsMissionBriefingVisible())
    {
        return;
    }

    const bool bHighContrast =
        Settings != nullptr && Settings->IsHighContrastHudEnabled();
    const float HudScale = Settings != nullptr ? Settings->GetHudScale() : 1.0f;
    const float PanelWidth = FMath::Min(920.0f, Canvas->ClipX - 60.0f);
    const float PanelHeight = FMath::Min(590.0f, Canvas->ClipY - 60.0f);
    const float Left = (Canvas->ClipX - PanelWidth) * 0.5f;
    const float Top = (Canvas->ClipY - PanelHeight) * 0.5f;
    const float TextLeft = Left + 42.0f;
    const float WidthScale = FMath::Clamp(PanelWidth / 920.0f, 0.72f, 1.0f);
    const float TextScale = HudScale * WidthScale;
    const FLinearColor Backdrop =
        bHighContrast ? FLinearColor(0.0f, 0.0f, 0.0f, 1.0f)
                      : FLinearColor(0.005f, 0.012f, 0.026f, 0.98f);
    const FLinearColor Accent =
        bHighContrast ? FLinearColor(1.0f, 0.9f, 0.1f)
                      : FLinearColor(0.12f, 0.92f, 1.0f);
    const FLinearColor Body =
        bHighContrast ? FLinearColor::White : FLinearColor(0.82f, 0.88f, 0.94f);
    const FLinearColor Muted =
        bHighContrast ? FLinearColor(0.9f, 0.9f, 0.9f)
                      : FLinearColor(0.55f, 0.64f, 0.72f);
    UFont* SmallFont = GEngine != nullptr ? GEngine->GetSmallFont() : nullptr;

    DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.72f), 0.0f, 0.0f, Canvas->ClipX, Canvas->ClipY);
    DrawRect(Backdrop, Left, Top, PanelWidth, PanelHeight);
    DrawLine(Left, Top, Left + PanelWidth, Top, Accent, 3.0f);
    DrawLine(Left, Top + PanelHeight, Left + PanelWidth, Top + PanelHeight, Accent, 3.0f);

    DrawText(TEXT("ECHOES OF THE BROKEN SUN"), Accent, TextLeft, Top + 34.0f,
             SmallFont, 1.55f * TextScale, false);
    DrawText(TEXT("GLASS SCAR  //  OPERATIONS BRIEF  //  MERIDIAN COMPACT"),
             Muted, TextLeft, Top + 72.0f, SmallFont, 0.90f * TextScale, false);

    DrawText(TEXT("SITUATION"), Accent, TextLeft, Top + 122.0f,
             SmallFont, 0.95f * TextScale, false);
    DrawText(TEXT("A dormant Future Well lies inside the shattered crossing."),
             Body, TextLeft, Top + 148.0f, SmallFont, 1.0f * TextScale, false);
    DrawText(TEXT("Kharuun forces hold the eastern approach. Every protocol changes what survives."),
             Body, TextLeft, Top + 172.0f, SmallFont, 1.0f * TextScale, false);

    DrawText(TEXT("PRIMARY OBJECTIVES"), Accent, TextLeft, Top + 220.0f,
             SmallFont, 0.95f * TextScale, false);
    DrawText(TEXT("01  Secure and choose a protocol for the central Future Well."),
             Body, TextLeft, Top + 247.0f, SmallFont, 1.0f * TextScale, false);
    DrawText(TEXT("02  Destroy the Kharuun Command Core without losing your own."),
             Body, TextLeft, Top + 273.0f, SmallFont, 1.0f * TextScale, false);

    DrawText(TEXT("FIELD DOCTRINE"), Accent, TextLeft, Top + 322.0f,
             SmallFont, 0.95f * TextScale, false);
    DrawText(TEXT("Harvest: immediate power  |  Preserve: sustained possibility  |  Reshape: temporary terrain"),
             Body, TextLeft, Top + 349.0f, SmallFont, 0.92f * TextScale, false);
    DrawText(TEXT("Select with LMB or drag. Issue context orders with RMB. WASD and screen edge move the camera."),
             Body, TextLeft, Top + 375.0f, SmallFont, 0.92f * TextScale, false);

    DrawText(TEXT("ACCESSIBILITY BEFORE DEPLOYMENT"), Accent, TextLeft, Top + 424.0f,
             SmallFont, 0.95f * TextScale, false);
    DrawText(TEXT("[U] UI scale   [I] high contrast   [O] reduced motion   [/] reduced flashing"),
             Body, TextLeft, Top + 451.0f, SmallFont, 0.92f * TextScale, false);

    DrawRect(Accent, Left + 42.0f, Top + PanelHeight - 74.0f,
             PanelWidth - 84.0f, 42.0f);
    DrawText(TEXT("PRESS ENTER TO DEPLOY"),
             bHighContrast ? FLinearColor::Black : FLinearColor(0.0f, 0.06f, 0.09f),
             Left + PanelWidth * 0.5f - 104.0f,
             Top + PanelHeight - 64.0f,
             SmallFont,
             1.05f * TextScale,
             false);
}

void AEchoesHUD::DrawTacticalMinimap(
    const UEchoesSimulationSubsystem* Bridge,
    const AEchoesPlayerController* EchoesController,
    const UEchoesGameUserSettings* Settings)
{
    if (Canvas == nullptr || Bridge == nullptr || Bridge->GetSimulation() == nullptr)
    {
        return;
    }

    const echoes::sim::Simulation* Sim = Bridge->GetSimulation();
    const bool bHighContrast =
        Settings != nullptr && Settings->IsHighContrastHudEnabled();
    const float HudScale = Settings != nullptr ? Settings->GetHudScale() : 1.0f;
    const float Size = FMath::Clamp(
        FMath::Min(220.0f * HudScale, Canvas->ClipY * 0.30f),
        150.0f,
        240.0f);
    const float Left = Canvas->ClipX - Size - 20.0f;
    const float Top = Canvas->ClipY - Size - 92.0f;
    if (Left < 18.0f || Top < 310.0f)
    {
        return;
    }

    const FLinearColor Border =
        bHighContrast ? FLinearColor(1.0f, 0.9f, 0.1f) : FLinearColor(0.15f, 0.88f, 1.0f);
    const FLinearColor Background =
        bHighContrast ? FLinearColor(0.0f, 0.0f, 0.0f, 0.98f)
                      : FLinearColor(0.008f, 0.018f, 0.035f, 0.93f);
    const FLinearColor Scar =
        bHighContrast ? FLinearColor(0.42f, 0.42f, 0.42f)
                      : FLinearColor(0.12f, 0.16f, 0.22f);
    DrawRect(Background, Left, Top, Size, Size);

    const int32 MapWidth = FMath::Max(1, Sim->Config().mapWidthTiles);
    const int32 MapHeight = FMath::Max(1, Sim->Config().mapHeightTiles);
    const float CellWidth = Size / static_cast<float>(MapWidth);
    const float CellHeight = Size / static_cast<float>(MapHeight);
    for (int32 TileY = 0; TileY < MapHeight; ++TileY)
    {
        for (int32 TileX = 0; TileX < MapWidth; ++TileX)
        {
            if (Sim->TerrainAt(TileX, TileY) == echoes::sim::Terrain::Blocked &&
                Sim->VisibilityAt(
                    UEchoesSimulationSubsystem::LocalPlayerId,
                    echoes::sim::Vec2::FromTiles(TileX, TileY)) !=
                    echoes::sim::Visibility::Unexplored)
            {
                DrawRect(
                    Scar,
                    Left + static_cast<float>(TileX) * CellWidth,
                    Top + static_cast<float>(TileY) * CellHeight,
                    FMath::Max(1.0f, CellWidth),
                    FMath::Max(1.0f, CellHeight));
            }
        }
    }

    const TArray<uint32>* SelectedIds =
        EchoesController != nullptr ? &EchoesController->GetSelectedEntityIds() : nullptr;
    int32 VisibleMarkerCount = 0;
    for (const echoes::sim::Entity& Entity : Sim->Entities())
    {
        if (!Sim->IsEntityVisibleTo(
                UEchoesSimulationSubsystem::LocalPlayerId,
                Entity.id))
        {
            continue;
        }
        ++VisibleMarkerCount;
        const float X = Left +
            FMath::Clamp(
                static_cast<float>(Entity.position.x.Raw()) /
                    static_cast<float>(echoes::sim::kFixedScale * MapWidth),
                0.0f,
                1.0f) * Size;
        const float Y = Top +
            FMath::Clamp(
                static_cast<float>(Entity.position.y.Raw()) /
                    static_cast<float>(echoes::sim::kFixedScale * MapHeight),
                0.0f,
                1.0f) * Size;
        const bool bStructure =
            Entity.type == echoes::sim::EntityType::CommandCore ||
            Entity.type == echoes::sim::EntityType::Dropoff ||
            Entity.type == echoes::sim::EntityType::Barracks;
        const float MarkerSize = bStructure ? 5.0f : 3.0f;
        FLinearColor Color = MinimapOwnerColor(Entity.owner, bHighContrast);
        if (Entity.type == echoes::sim::EntityType::ResourceNode)
        {
            Color = FLinearColor(1.0f, 0.62f, 0.08f);
        }
        else if (Entity.type == echoes::sim::EntityType::FutureWell)
        {
            Color = FLinearColor(0.78f, 0.3f, 1.0f);
        }

        const bool bSelected = SelectedIds != nullptr && SelectedIds->Contains(Entity.id);
        if (bSelected)
        {
            DrawRect(FLinearColor::White, X - MarkerSize, Y - MarkerSize,
                     MarkerSize * 2.0f, MarkerSize * 2.0f);
        }
        const float HalfMarker = MarkerSize * 0.5f;
        switch (Entity.owner)
        {
            case 1:
                DrawLine(X - HalfMarker, Y - HalfMarker, X + HalfMarker, Y + HalfMarker, Color, 1.5f);
                DrawLine(X + HalfMarker, Y - HalfMarker, X - HalfMarker, Y + HalfMarker, Color, 1.5f);
                break;
            case 2:
                DrawLine(X, Y - HalfMarker, X + HalfMarker, Y, Color, 1.5f);
                DrawLine(X + HalfMarker, Y, X, Y + HalfMarker, Color, 1.5f);
                DrawLine(X, Y + HalfMarker, X - HalfMarker, Y, Color, 1.5f);
                DrawLine(X - HalfMarker, Y, X, Y - HalfMarker, Color, 1.5f);
                break;
            case 3:
                DrawLine(X - HalfMarker, Y, X + HalfMarker, Y, Color, 1.5f);
                DrawLine(X, Y - HalfMarker, X, Y + HalfMarker, Color, 1.5f);
                break;
            default:
                DrawRect(Color, X - HalfMarker, Y - HalfMarker, MarkerSize, MarkerSize);
                break;
        }
    }

    if (const APawn* CameraPawn = GetOwningPawn())
    {
        const echoes::sim::Vec2 CameraPosition =
            Bridge->WorldToSim(CameraPawn->GetActorLocation());
        const float CameraX = Left +
            FMath::Clamp(
                static_cast<float>(CameraPosition.x.Raw()) /
                    static_cast<float>(echoes::sim::kFixedScale * MapWidth),
                0.0f,
                1.0f) * Size;
        const float CameraY = Top +
            FMath::Clamp(
                static_cast<float>(CameraPosition.y.Raw()) /
                    static_cast<float>(echoes::sim::kFixedScale * MapHeight),
                0.0f,
                1.0f) * Size;
        DrawLine(CameraX - 6.0f, CameraY, CameraX + 6.0f, CameraY, FLinearColor::White, 1.0f);
        DrawLine(CameraX, CameraY - 6.0f, CameraX, CameraY + 6.0f, FLinearColor::White, 1.0f);
    }

    DrawLine(Left, Top, Left + Size, Top, Border, 2.0f);
    DrawLine(Left + Size, Top, Left + Size, Top + Size, Border, 2.0f);
    DrawLine(Left + Size, Top + Size, Left, Top + Size, Border, 2.0f);
    DrawLine(Left, Top + Size, Left, Top, Border, 2.0f);
    DrawText(
        TEXT("TACTICAL OVERVIEW  |  fog-respecting"),
        Border,
        Left,
        Top - 18.0f,
        GEngine != nullptr ? GEngine->GetSmallFont() : nullptr,
        0.72f * HudScale,
        false);

    if (!bLoggedTacticalOverviewReady)
    {
        bLoggedTacticalOverviewReady = true;
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_MINIMAP_READY] fogRespecting=true terrainAware=true nonColorTeams=true visibleMarkers=%d"),
            VisibleMarkerCount);
    }
}

void AEchoesHUD::DrawSelectionRectangle()
{
    const AEchoesPlayerController* EchoesController =
        Cast<AEchoesPlayerController>(GetOwningPlayerController());
    if (EchoesController == nullptr || !EchoesController->IsDraggingSelection())
    {
        return;
    }

    const FVector2D Start = EchoesController->GetSelectionStartScreenPosition();
    const FVector2D Current = EchoesController->GetSelectionCurrentScreenPosition();
    const float MinX = FMath::Min(Start.X, Current.X);
    const float MaxX = FMath::Max(Start.X, Current.X);
    const float MinY = FMath::Min(Start.Y, Current.Y);
    const float MaxY = FMath::Max(Start.Y, Current.Y);
    const UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get();
    const bool bHighContrast =
        Settings != nullptr && Settings->IsHighContrastHudEnabled();
    const FLinearColor BorderColor =
        bHighContrast
            ? FLinearColor(1.0f, 0.9f, 0.1f, 1.0f)
            : FLinearColor(0.12f, 0.92f, 1.0f, 0.95f);

    DrawRect(
        bHighContrast
            ? FLinearColor(1.0f, 0.9f, 0.1f, 0.18f)
            : FLinearColor(0.12f, 0.75f, 1.0f, 0.10f),
        MinX,
        MinY,
        MaxX - MinX,
        MaxY - MinY);
    DrawLine(MinX, MinY, MaxX, MinY, BorderColor, 1.5f);
    DrawLine(MaxX, MinY, MaxX, MaxY, BorderColor, 1.5f);
    DrawLine(MaxX, MaxY, MinX, MaxY, BorderColor, 1.5f);
    DrawLine(MinX, MaxY, MinX, MinY, BorderColor, 1.5f);
}
