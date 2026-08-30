#include "EchoesHUD.h"

#include "EchoesContentSubsystem.h"
#include "EchoesCommandDeckModel.h"
#include "EchoesContactIndicatorLayout.h"
#include "EchoesEntityView.h"
#include "EchoesGameUserSettings.h"
#include "EchoesOfTheBrokenSun.h"
#include "EchoesPlayerController.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesTechnologyPanelLayout.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

#include <algorithm>
#include <optional>

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

const TCHAR* ResearchDisplayName(echoes::sim::ResearchType Technology)
{
    switch (Technology)
    {
        case echoes::sim::ResearchType::MeridianPrismaticTargeting:
            return TEXT("Prismatic Targeting");
        case echoes::sim::ResearchType::MeridianHorizonLattice:
            return TEXT("Horizon Lattice");
        case echoes::sim::ResearchType::KharuunEchoCartography:
            return TEXT("Echo Cartography");
        case echoes::sim::ResearchType::KharuunAncestralEdge:
            return TEXT("Ancestral Edge");
        default:
            return TEXT("Unknown Technology");
    }
}

const TCHAR* WellChoiceDisplayName(echoes::sim::FutureWellChoice Choice)
{
    switch (Choice)
    {
        case echoes::sim::FutureWellChoice::Harvest: return TEXT("HARVEST");
        case echoes::sim::FutureWellChoice::Preserve: return TEXT("PRESERVE");
        case echoes::sim::FutureWellChoice::Reshape: return TEXT("RESHAPE");
        default: return TEXT("UNRESOLVED");
    }
}

FVector2D FallbackContactProjection(
    APlayerController* Controller,
    const FVector& WorldPosition,
    const FVector2D& ViewportSize)
{
    FVector CameraLocation = FVector::ZeroVector;
    FRotator CameraRotation = FRotator::ZeroRotator;
    Controller->GetPlayerViewPoint(CameraLocation, CameraRotation);

    FVector GroundDirection = WorldPosition - CameraLocation;
    GroundDirection.Z = 0.0f;
    FVector CameraForward = CameraRotation.Vector();
    CameraForward.Z = 0.0f;
    if (!CameraForward.Normalize())
    {
        CameraForward = FVector::ForwardVector;
    }
    const FVector CameraRight =
        FVector::CrossProduct(FVector::UpVector, CameraForward).GetSafeNormal();
    FVector2D ScreenDirection(
        FVector::DotProduct(GroundDirection, CameraRight),
        -FVector::DotProduct(GroundDirection, CameraForward));
    if (!ScreenDirection.Normalize())
    {
        ScreenDirection = FVector2D(0.0f, -1.0f);
    }
    return ViewportSize * 0.5f +
        ScreenDirection * FMath::Max(ViewportSize.X, ViewportSize.Y) * 2.0f;
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

    if (EchoesController != nullptr && EchoesController->IsTitleScreenVisible())
    {
        DrawTitleScreen(EchoesController, Settings);
        return;
    }

    if (EchoesController != nullptr && EchoesController->IsMissionBriefingVisible())
    {
        DrawMissionBriefing(EchoesController, Settings);
        return;
    }

    if (Canvas != nullptr && EchoesController != nullptr &&
        EchoesController->IsKeyboardTargetingEnabled() &&
        !EchoesController->IsModalOverlayVisible())
    {
        const FVector2D TargetOffset = EchoesController->GetKeyboardTargetOffset();
        const float CenterX = Canvas->ClipX * 0.5f + TargetOffset.X;
        const float CenterY = Canvas->ClipY * 0.5f + TargetOffset.Y;
        DrawLine(CenterX - 16.0f, CenterY, CenterX - 5.0f, CenterY, AccentColor, 2.0f);
        DrawLine(CenterX + 5.0f, CenterY, CenterX + 16.0f, CenterY, AccentColor, 2.0f);
        DrawLine(CenterX, CenterY - 16.0f, CenterX, CenterY - 5.0f, AccentColor, 2.0f);
        DrawLine(CenterX, CenterY + 5.0f, CenterX, CenterY + 16.0f, AccentColor, 2.0f);
        DrawText(
            TEXT("KEYBOARD TARGET"),
            AccentColor,
            CenterX + 20.0f,
            CenterY + 10.0f,
            GEngine != nullptr ? GEngine->GetSmallFont() : nullptr,
            0.78f * HudScale,
            false);
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
    FString ResearchLine = TEXT("RESEARCH unavailable");
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
            const bool bMeridian =
                Player->faction == echoes::sim::Faction::MeridianCompact;
            const echoes::sim::ResearchType First = bMeridian
                ? echoes::sim::ResearchType::MeridianPrismaticTargeting
                : echoes::sim::ResearchType::KharuunEchoCartography;
            const echoes::sim::ResearchType Second = bMeridian
                ? echoes::sim::ResearchType::MeridianHorizonLattice
                : echoes::sim::ResearchType::KharuunAncestralEdge;
            if (Player->activeResearch != echoes::sim::ResearchType::None)
            {
                const int32 Percent = FMath::Clamp(
                    Player->researchProgress * 100 /
                        FMath::Max(1, Player->researchRequired),
                    0,
                    100);
                ResearchLine = FString::Printf(
                    TEXT("RESEARCH  %s  %d%%     [X] cancel selected producer // NO REFUND"),
                    ResearchDisplayName(Player->activeResearch),
                    Percent);
            }
            else if (Player->lastInterruptedResearch !=
                     echoes::sim::ResearchType::None)
            {
                ResearchLine = FString::Printf(
                    TEXT("RESEARCH INTERRUPTED  %s     NO REFUND"),
                    ResearchDisplayName(Player->lastInterruptedResearch));
            }
            else
            {
                const int32 Completed =
                    (Player->HasCompletedResearch(First) ? 1 : 0) +
                    (Player->HasCompletedResearch(Second) ? 1 : 0);
                ResearchLine = FString::Printf(
                    TEXT("RESEARCH  %d/2 complete     [Shift+R] next from selected production structure     [K/L] save/load"),
                    Completed);
            }
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
                if (View->GetPendingWarformAdaptation() ==
                    echoes::sim::WarformAdaptation::Carapace)
                {
                    SelectedType += TEXT(" — molting CARAPACE");
                }
                else if (View->GetPendingWarformAdaptation() ==
                         echoes::sim::WarformAdaptation::Striker)
                {
                    SelectedType += TEXT(" — molting STRIKER");
                }
                else if (View->GetWarformAdaptation() ==
                         echoes::sim::WarformAdaptation::Carapace)
                {
                    SelectedType += TEXT(" — CARAPACE");
                }
                else if (View->GetWarformAdaptation() ==
                         echoes::sim::WarformAdaptation::Striker)
                {
                    SelectedType += TEXT(" — STRIKER");
                }
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
                        : Entity->productionType == echoes::sim::EntityType::HeavyUnit
                              ? TEXT("Heavy")
                              : Entity->productionType == echoes::sim::EntityType::ScoutUnit
                                    ? TEXT("Scout")
                                    : TEXT("Line Unit"),
                    Percent);
            }
        }
        if (Bridge != nullptr &&
            Bridge->GetOperationMode() ==
                EEchoesOperationMode::CampaignSevenAccounts)
        {
            SelectionLine = FString::Printf(
                TEXT("Selected  %d%s     Formation  %s     Inherited route  %s"),
                SelectedIds.Num(),
                *SelectedType,
                *EchoesController->GetFormationLabel(),
                WellChoiceDisplayName(Bridge->GetRecordedPrologueChoice()));
        }
        else if (Bridge != nullptr &&
                 Bridge->GetOperationMode() ==
                     EEchoesOperationMode::CampaignCityReserve)
        {
            SelectionLine = FString::Printf(
                TEXT("Selected  %d%s     Formation  %s     Reserve plan  %s"),
                SelectedIds.Num(),
                *SelectedType,
                *EchoesController->GetFormationLabel(),
                WellChoiceDisplayName(Bridge->GetRecordedPrologueChoice()));
        }
        else if (Bridge != nullptr &&
                 Bridge->GetOperationMode() ==
                     EEchoesOperationMode::CampaignUnburiedRoad)
        {
            SelectionLine = FString::Printf(
                TEXT("Selected  %d%s     Formation  %s     Unburied route  %s"),
                SelectedIds.Num(),
                *SelectedType,
                *EchoesController->GetFormationLabel(),
                WellChoiceDisplayName(Bridge->GetRecordedPrologueChoice()));
        }
        else if (Bridge != nullptr &&
                 Bridge->GetOperationMode() ==
                     EEchoesOperationMode::CampaignTermsOfContinuance)
        {
            SelectionLine = FString::Printf(
                TEXT("Selected  %d%s     Formation  %s     Accord  %s"),
                SelectedIds.Num(),
                *SelectedType,
                *EchoesController->GetFormationLabel(),
                WellChoiceDisplayName(Bridge->GetRecordedPrologueChoice()));
        }
        else if (Bridge != nullptr &&
                 Bridge->GetOperationMode() ==
                     EEchoesOperationMode::CampaignNamesWithoutBirths)
        {
            SelectionLine = FString::Printf(
                TEXT("Selected  %d%s     Formation  %s     Census branch  %s"),
                SelectedIds.Num(),
                *SelectedType,
                *EchoesController->GetFormationLabel(),
                WellChoiceDisplayName(Bridge->GetRecordedPrologueChoice()));
        }
        else if (Bridge != nullptr &&
                 Bridge->GetOperationMode() ==
                     EEchoesOperationMode::CampaignShapeOfSilence)
        {
            SelectionLine = FString::Printf(
                TEXT("Selected  %d%s     Formation  %s     Listening branch  %s"),
                SelectedIds.Num(),
                *SelectedType,
                *EchoesController->GetFormationLabel(),
                WellChoiceDisplayName(Bridge->GetRecordedPrologueChoice()));
        }
        else
        {
            SelectionLine = FString::Printf(
                TEXT("Selected  %d%s     Formation  %s     Future Well protocol  %s"),
                SelectedIds.Num(),
                *SelectedType,
                *EchoesController->GetFormationLabel(),
                *EchoesController->GetFutureWellChoiceLabel());
        }
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
        TEXT("WASD / edge: pan  Wheel: zoom  LMB/drag: select  RMB: pointer order  [Home] Key target  [End] Snap  [Arrows] Move  [Space] Order"),
        SecondaryColor,
        TextX,
        HudY(90.0f),
        GEngine != nullptr ? GEngine->GetSmallFont() : nullptr,
        0.86f * HudScale,
        false);
    const FString FactionControlLine =
        Bridge != nullptr &&
                Bridge->GetLocalFaction() ==
                    echoes::sim::Faction::KharuunAssemblies
            ? TEXT("[F] Attack-move  [T] Patrol  [H] Hold  [J] Guard  [X] Stop  [F3] Waystone  [F4/F5] Warform  [F6] Cover")
            : TEXT("[F] Attack-move  [T] Patrol  [H] Hold  [J] Guard  [X] Stop  [\\] Bulwark  [=] Relay  [B/N/M] Build");
    DrawText(
        FactionControlLine,
        SecondaryColor,
        TextX,
        HudY(113.0f),
        GEngine != nullptr ? GEngine->GetSmallFont() : nullptr,
        0.86f * HudScale,
        false);
    DrawText(
        FString::Printf(
            TEXT("[Tab] Next owned    [Backspace] Previous    [F7] Combat force    [F8] Formation %s    [1-0] Recall    [G + 1-0] Assign    [F2] Tech    [P] Pause"),
            EchoesController != nullptr
                ? *EchoesController->GetFormationLabel()
                : TEXT("BOX")),
        SecondaryColor,
        TextX,
        HudY(136.0f),
        GEngine != nullptr ? GEngine->GetSmallFont() : nullptr,
        0.86f * HudScale,
        false);
    DrawText(
        FString::Printf(
            TEXT("[Z] Harvest    [C] Preserve    [V] Reshape    Local: %s"),
            EchoesController != nullptr
                ? *EchoesController->GetLocalFactionLabel()
                : TEXT("MERIDIAN COMPACT")),
        SecondaryColor,
        TextX,
        HudY(159.0f),
        GEngine != nullptr ? GEngine->GetSmallFont() : nullptr,
        0.86f * HudScale,
        false);
    DrawText(
        ResearchLine,
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

    DrawCommandDeck(EchoesController, Bridge, Settings);

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

    const echoes::sim::Simulation* HudSimulation =
        Bridge != nullptr ? Bridge->GetSimulation() : nullptr;
    const bool bHasLocalVibrationDetector =
        HudSimulation != nullptr &&
        std::any_of(
            HudSimulation->Entities().begin(),
            HudSimulation->Entities().end(),
            [](const echoes::sim::Entity& Entity)
            {
                return Entity.owner ==
                           UEchoesSimulationSubsystem::LocalPlayerId &&
                       Entity.faction ==
                           echoes::sim::Faction::KharuunAssemblies &&
                       Entity.completed && Entity.hitPoints > 0 &&
                       !Entity.temporaryMineralCover &&
                       (Entity.type == echoes::sim::EntityType::ScoutUnit ||
                        Entity.type ==
                            echoes::sim::EntityType::UtilityStructure);
            });
    const std::optional<echoes::sim::PlayerView> PlayerView =
        bHasLocalVibrationDetector
            ? HudSimulation->CreatePlayerView(
                  UEchoesSimulationSubsystem::LocalPlayerId)
            : std::nullopt;
    DrawObjectiveTracker(Bridge, Settings);
    DrawVibrationSignatures(
        Bridge, Settings, PlayerView.has_value() ? &*PlayerView : nullptr);
    DrawTacticalMinimap(
        Bridge,
        EchoesController,
        Settings,
        PlayerView.has_value() ? &*PlayerView : nullptr);
    DrawSelectionRectangle();
    if (EchoesController != nullptr && EchoesController->IsTechnologyPanelVisible())
    {
        DrawTechnologyPanel(EchoesController, Bridge, Settings);
    }
    else if (EchoesController != nullptr && EchoesController->IsPauseMenuVisible())
    {
        DrawPauseMenu(EchoesController, Settings);
    }
    else if (EchoesController != nullptr && EchoesController->IsMatchResultVisible())
    {
        DrawMatchResult(EchoesController, Bridge, Settings);
    }
}

void AEchoesHUD::DrawCommandDeck(
    const AEchoesPlayerController* EchoesController,
    const UEchoesSimulationSubsystem* Bridge,
    const UEchoesGameUserSettings* Settings)
{
    if (Canvas == nullptr || EchoesController == nullptr || Bridge == nullptr ||
        EchoesController->IsModalOverlayVisible())
    {
        return;
    }
    const TArray<uint32>& SelectedIds = EchoesController->GetSelectedEntityIds();
    if (SelectedIds.IsEmpty())
    {
        return;
    }

    FEchoesCommandDeckProfile Profile;
    for (const uint32 EntityId : SelectedIds)
    {
        const echoes::sim::Entity* Entity = Bridge->FindEntity(EntityId);
        if (Entity == nullptr || Entity->owner != UEchoesSimulationSubsystem::LocalPlayerId)
        {
            continue;
        }
        switch (Entity->type)
        {
            case echoes::sim::EntityType::Worker:
                ++Profile.WorkerCount;
                break;
            case echoes::sim::EntityType::Soldier:
            case echoes::sim::EntityType::HeavyUnit:
            case echoes::sim::EntityType::ScoutUnit:
                ++Profile.CombatCount;
                break;
            case echoes::sim::EntityType::CommandCore:
            case echoes::sim::EntityType::Dropoff:
            case echoes::sim::EntityType::Barracks:
            case echoes::sim::EntityType::UtilityStructure:
                ++Profile.StructureCount;
                Profile.bHasCommandCore =
                    Profile.bHasCommandCore ||
                    Entity->type == echoes::sim::EntityType::CommandCore;
                Profile.bHasBarracks =
                    Profile.bHasBarracks ||
                    Entity->type == echoes::sim::EntityType::Barracks;
                break;
            default:
                ++Profile.OtherCount;
                break;
        }
    }

    const bool bHighContrast =
        Settings != nullptr && Settings->IsHighContrastHudEnabled();
    const float HudScale = Settings != nullptr ? Settings->GetHudScale() : 1.0f;
    const float PanelWidth = FMath::Min(
        900.0f * HudScale,
        FMath::Max(520.0f, Canvas->ClipX - 300.0f));
    const float PanelHeight = 112.0f * HudScale;
    const float Left = 18.0f;
    const float Top = Canvas->ClipY - 84.0f - PanelHeight;
    if (Top < 306.0f)
    {
        return;
    }

    const FLinearColor Backdrop = bHighContrast
        ? FLinearColor(0.0f, 0.0f, 0.0f, 0.98f)
        : FLinearColor(0.008f, 0.018f, 0.035f, 0.93f);
    const FLinearColor Accent = bHighContrast
        ? FLinearColor(1.0f, 0.9f, 0.1f)
        : FLinearColor(0.15f, 0.88f, 1.0f);
    const FLinearColor Body = bHighContrast
        ? FLinearColor::White
        : FLinearColor(0.84f, 0.9f, 0.95f);
    const FLinearColor Muted = bHighContrast
        ? FLinearColor(0.9f, 0.9f, 0.9f)
        : FLinearColor(0.58f, 0.67f, 0.76f);
    UFont* SmallFont = GEngine != nullptr ? GEngine->GetSmallFont() : nullptr;

    DrawRect(Backdrop, Left, Top, PanelWidth, PanelHeight);
    DrawRect(Accent, Left, Top, PanelWidth, 3.0f * HudScale);
    DrawRect(
        FLinearColor(Accent.R, Accent.G, Accent.B, 0.75f),
        Left,
        Top,
        5.0f * HudScale,
        PanelHeight);
    DrawText(
        FString::Printf(
            TEXT("TACTICAL COMMAND  //  %d SELECTED  //  FORMATION %s"),
            SelectedIds.Num(),
            *EchoesController->GetFormationLabel()),
        Accent,
        Left + 18.0f * HudScale,
        Top + 13.0f * HudScale,
        SmallFont,
        0.88f * HudScale,
        false);
    DrawText(
        FString::Printf(
            TEXT("FORCE PROFILE    COMBAT %d    WORKERS %d    STRUCTURES %d    OTHER %d"),
            Profile.CombatCount,
            Profile.WorkerCount,
            Profile.StructureCount,
            Profile.OtherCount),
        Muted,
        Left + 18.0f * HudScale,
        Top + 39.0f * HudScale,
        SmallFont,
        0.76f * HudScale,
        false);

    const FString PrimaryActions =
        FEchoesCommandDeckModel::BuildPrimaryActions(Profile);
    DrawText(
        PrimaryActions,
        Body,
        Left + 18.0f * HudScale,
        Top + 63.0f * HudScale,
        SmallFont,
        0.75f * HudScale,
        false);

    EEchoesFormationType NextFormation = EEchoesFormationType::Box;
    switch (EchoesController->GetFormationType())
    {
        case EEchoesFormationType::Box:
            NextFormation = EEchoesFormationType::Line;
            break;
        case EEchoesFormationType::Line:
            NextFormation = EEchoesFormationType::Wedge;
            break;
        case EEchoesFormationType::Wedge:
            NextFormation = EEchoesFormationType::Box;
            break;
    }
    FString ContextLine = FString::Printf(
        TEXT("[F8] CYCLE FORMATION  %s -> %s    [G + 1-0] ASSIGN GROUP    [1-0] RECALL"),
        *EchoesController->GetFormationLabel(),
        *FEchoesFormationLayout::DisplayName(NextFormation));
    if (Bridge->GetLocalFaction() == echoes::sim::Faction::MeridianCompact &&
        Profile.CombatCount > 0)
    {
        ContextLine += TEXT("    [\\] BULWARK");
    }
    else if (
        Bridge->GetLocalFaction() == echoes::sim::Faction::KharuunAssemblies &&
        Profile.CombatCount > 0)
    {
        ContextLine += TEXT("    [F4/F5] WARFORM");
    }
    DrawText(
        ContextLine,
        Accent,
        Left + 18.0f * HudScale,
        Top + 87.0f * HudScale,
        SmallFont,
        0.72f * HudScale,
        false);

    if (!bLoggedCommandDeckReady)
    {
        bLoggedCommandDeckReady = true;
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_COMMAND_DECK_READY] selected=%d combat=%d workers=%d structures=%d formation=%s highContrast=%s finalUI=false"),
            SelectedIds.Num(),
            Profile.CombatCount,
            Profile.WorkerCount,
            Profile.StructureCount,
            *EchoesController->GetFormationLabel(),
            bHighContrast ? TEXT("true") : TEXT("false"));
    }
}

void AEchoesHUD::DrawTechnologyPanel(
    const AEchoesPlayerController* EchoesController,
    const UEchoesSimulationSubsystem* Bridge,
    const UEchoesGameUserSettings* Settings)
{
    if (Canvas == nullptr || EchoesController == nullptr || Bridge == nullptr ||
        !EchoesController->IsTechnologyPanelVisible())
    {
        return;
    }
    const echoes::sim::Simulation* Simulation = Bridge->GetSimulation();
    const echoes::sim::PlayerState* Player =
        Simulation != nullptr
            ? Simulation->FindPlayer(UEchoesSimulationSubsystem::LocalPlayerId)
            : nullptr;
    if (Simulation == nullptr || Player == nullptr)
    {
        return;
    }
    const float HudScale = Settings != nullptr ? Settings->GetHudScale() : 1.0f;
    const float Scale = FMath::Clamp(HudScale, 0.75f, 1.35f);
    const bool bHighContrast =
        Settings != nullptr && Settings->IsHighContrastHudEnabled();
    const FEchoesTechnologyPanelLayout Layout =
        FEchoesTechnologyPanelLayout::Build(
            FVector2D(Canvas->ClipX, Canvas->ClipY), HudScale);
    const FLinearColor PanelColor =
        bHighContrast
            ? FLinearColor(0.0f, 0.0f, 0.0f, 0.99f)
            : FLinearColor(0.008f, 0.018f, 0.035f, 0.97f);
    const FLinearColor AccentColor =
        bHighContrast
            ? FLinearColor(1.0f, 0.9f, 0.1f)
            : FLinearColor(0.15f, 0.88f, 1.0f);
    const FLinearColor MutedColor =
        bHighContrast ? FLinearColor::White : FLinearColor(0.68f, 0.73f, 0.82f);

    DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.58f), 0.0f, 0.0f,
             Canvas->ClipX, Canvas->ClipY);
    DrawRect(PanelColor, Layout.Origin.X, Layout.Origin.Y,
             Layout.Size.X, Layout.Size.Y);
    DrawRect(AccentColor, Layout.Origin.X, Layout.Origin.Y,
             Layout.Size.X, 3.0f * Scale);

    const float TextX = Layout.Origin.X + 34.0f * Scale;
    DrawText(
        FString::Printf(
            TEXT("%s  //  TECHNOLOGY ARCHIVE"),
            *EchoesController->GetLocalFactionLabel()),
        AccentColor,
        TextX,
        Layout.Origin.Y + 25.0f * Scale,
        GEngine != nullptr ? GEngine->GetSmallFont() : nullptr,
        1.12f * Scale,
        false);
    DrawText(
        TEXT("Research occupies the selected production structure. Close the archive, then X on that producer cancels without refund; destruction also interrupts."),
        MutedColor,
        TextX,
        Layout.Origin.Y + 58.0f * Scale,
        GEngine != nullptr ? GEngine->GetSmallFont() : nullptr,
        0.86f * Scale,
        false);

    const FBox2D& Close = Layout.CloseButton;
    DrawRect(
        FLinearColor(AccentColor.R, AccentColor.G, AccentColor.B, 0.18f),
        Close.Min.X, Close.Min.Y, Close.GetSize().X, Close.GetSize().Y);
    DrawText(
        TEXT("CLOSE"), AccentColor,
        Close.Min.X + 8.0f * Scale,
        Close.Min.Y + 8.0f * Scale,
        GEngine != nullptr ? GEngine->GetSmallFont() : nullptr,
        0.76f * Scale,
        false);

    bool bSelectedProducer = false;
    for (const uint32 EntityId : EchoesController->GetSelectedEntityIds())
    {
        const echoes::sim::Entity* Entity = Bridge->FindEntity(EntityId);
        if (Entity != nullptr &&
            Entity->owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            Entity->type == echoes::sim::EntityType::Barracks)
        {
            bSelectedProducer = true;
            break;
        }
    }

    const bool bMeridian =
        Player->faction == echoes::sim::Faction::MeridianCompact;
    const echoes::sim::ResearchType Technologies[2] = {
        bMeridian
            ? echoes::sim::ResearchType::MeridianPrismaticTargeting
            : echoes::sim::ResearchType::KharuunEchoCartography,
        bMeridian
            ? echoes::sim::ResearchType::MeridianHorizonLattice
            : echoes::sim::ResearchType::KharuunAncestralEdge,
    };
    const TCHAR* StableIds[2] = {
        bMeridian ? TEXT("mc_prismatic_targeting")
                  : TEXT("ka_echo_cartography"),
        bMeridian ? TEXT("mc_horizon_lattice")
                  : TEXT("ka_ancestral_edge"),
    };
    const UEchoesContentSubsystem* Content =
        GetWorld() != nullptr && GetWorld()->GetGameInstance() != nullptr
            ? GetWorld()->GetGameInstance()->GetSubsystem<UEchoesContentSubsystem>()
            : nullptr;

    for (int32 TierIndex = 0; TierIndex < 2; ++TierIndex)
    {
        const echoes::sim::ResearchType Technology = Technologies[TierIndex];
        const echoes::sim::ResearchRules* Rules =
            Simulation->ResearchDefinition(Technology);
        const FEchoesTechnologyContent* Definition =
            Content != nullptr && Content->IsReady()
                ? Content->GetCatalog().FindTechnology(StableIds[TierIndex])
                : nullptr;
        const FString Name =
            Definition != nullptr
                ? Definition->DisplayName
                : FString::Printf(TEXT("Tier %d technology"), TierIndex + 1);
        const FBox2D& Row = Layout.TechnologyRows[TierIndex];
        const bool bComplete = Player->HasCompletedResearch(Technology);
        const bool bActive = Player->activeResearch == Technology;
        const bool bInterrupted =
            Player->lastInterruptedResearch == Technology;
        const bool bFocused =
            EchoesController->GetTechnologyPanelFocusedTier() == TierIndex;
        const bool bPrerequisiteMet =
            Rules != nullptr &&
            (Rules->prerequisite == echoes::sim::ResearchType::None ||
             Player->HasCompletedResearch(Rules->prerequisite));
        const bool bFunded =
            Rules != nullptr &&
            Player->resources.material >= Rules->cost.material &&
            Player->resources.dawnshards >= Rules->cost.dawnshards;
        FString Status;
        FLinearColor StatusColor = AccentColor;
        if (bComplete)
        {
            Status = TEXT("COMPLETE");
            StatusColor = FLinearColor(0.25f, 1.0f, 0.66f);
        }
        else if (bActive)
        {
            const int32 Percent = FMath::Clamp(
                Player->researchProgress * 100 /
                    FMath::Max(1, Player->researchRequired),
                0, 100);
            Status = FString::Printf(TEXT("RESEARCHING  %d%%"), Percent);
        }
        else if (bInterrupted)
        {
            Status = TEXT("INTERRUPTED — costs lost; select the producer to restart");
            StatusColor = FLinearColor(1.0f, 0.38f, 0.22f);
        }
        else if (Player->activeResearch != echoes::sim::ResearchType::None)
        {
            Status = TEXT("BUSY — another project is active");
            StatusColor = MutedColor;
        }
        else if (!bPrerequisiteMet)
        {
            Status = TEXT("LOCKED — complete Tier 1 first");
            StatusColor = MutedColor;
        }
        else if (!bFunded)
        {
            Status = TEXT("INSUFFICIENT RESOURCES");
            StatusColor = FLinearColor(1.0f, 0.55f, 0.2f);
        }
        else if (!bSelectedProducer)
        {
            Status = TEXT("READY — select a production structure");
            StatusColor = FLinearColor(1.0f, 0.78f, 0.25f);
        }
        else
        {
            Status = TEXT("READY — click to research");
        }

        const FLinearColor RowColor =
            bComplete
                ? FLinearColor(0.05f, 0.18f, 0.15f, 0.94f)
                : bActive
                      ? FLinearColor(0.04f, 0.18f, 0.25f, 0.96f)
                      : bInterrupted
                            ? FLinearColor(0.22f, 0.055f, 0.045f, 0.96f)
                            : FLinearColor(0.025f, 0.055f, 0.09f, 0.94f);
        DrawRect(RowColor, Row.Min.X, Row.Min.Y,
                 Row.GetSize().X, Row.GetSize().Y);
        if (bFocused)
        {
            DrawLine(Row.Min.X, Row.Min.Y, Row.Max.X, Row.Min.Y,
                     AccentColor, 3.0f * Scale);
            DrawLine(Row.Min.X, Row.Max.Y, Row.Max.X, Row.Max.Y,
                     AccentColor, 3.0f * Scale);
            DrawLine(Row.Max.X, Row.Min.Y, Row.Max.X, Row.Max.Y,
                     AccentColor, 3.0f * Scale);
        }
        DrawRect(
            FLinearColor(AccentColor.R, AccentColor.G, AccentColor.B, 0.7f),
            Row.Min.X, Row.Min.Y, 4.0f * Scale, Row.GetSize().Y);
        DrawText(
            FString::Printf(TEXT("TIER %d  //  %s"), TierIndex + 1, *Name),
            FLinearColor::White,
            Row.Min.X + 20.0f * Scale,
            Row.Min.Y + 16.0f * Scale,
            GEngine != nullptr ? GEngine->GetSmallFont() : nullptr,
            1.0f * Scale,
            false);
        if (Rules != nullptr)
        {
            DrawText(
                FString::Printf(
                    TEXT("Matter %d    Dawn %d    Duration %.1fs"),
                    Rules->cost.material,
                    Rules->cost.dawnshards,
                    static_cast<double>(Rules->researchTicks) /
                        FMath::Max(1U, Simulation->Config().ticksPerSecond)),
                MutedColor,
                Row.Min.X + 20.0f * Scale,
                Row.Min.Y + 47.0f * Scale,
                GEngine != nullptr ? GEngine->GetSmallFont() : nullptr,
                0.83f * Scale,
                false);
            const FString Effect =
                Rules->combatDamagePercent > 100
                    ? FString::Printf(
                          TEXT("Effect: +%d%% combat damage"),
                          Rules->combatDamagePercent - 100)
                    : FString::Printf(
                          TEXT("Effect: +%d%% combat vision"),
                          Rules->combatVisionPercent - 100);
            DrawText(
                Effect,
                MutedColor,
                Row.Min.X + 20.0f * Scale,
                Row.Min.Y + 72.0f * Scale,
                GEngine != nullptr ? GEngine->GetSmallFont() : nullptr,
                0.83f * Scale,
                false);
        }
        DrawText(
            Status,
            StatusColor,
            Row.Max.X - 286.0f * Scale,
            Row.Min.Y + 18.0f * Scale,
            GEngine != nullptr ? GEngine->GetSmallFont() : nullptr,
            0.82f * Scale,
            false);
    }

    DrawText(
        TEXT("Up/Down: focus    Enter: activate    Shift+R: next available    F2 / Escape / P: close"),
        AccentColor,
        TextX,
        Layout.Origin.Y + Layout.Size.Y - 38.0f * Scale,
        GEngine != nullptr ? GEngine->GetSmallFont() : nullptr,
        0.86f * Scale,
        false);
}

void AEchoesHUD::DrawTitleScreen(
    const AEchoesPlayerController* EchoesController,
    const UEchoesGameUserSettings* Settings)
{
    if (Canvas == nullptr || EchoesController == nullptr ||
        !EchoesController->IsTitleScreenVisible())
    {
        return;
    }

    const bool bHighContrast =
        Settings != nullptr && Settings->IsHighContrastHudEnabled();
    const float HudScale = Settings != nullptr ? Settings->GetHudScale() : 1.0f;
    const float PanelWidth = FMath::Min(
        FMath::Max(700.0f, Canvas->ClipX - 60.0f),
        FMath::Clamp(Canvas->ClipX * 0.60f, 860.0f, 1220.0f));
    const float PanelHeight = FMath::Min(
        FMath::Max(560.0f, Canvas->ClipY - 60.0f),
        FMath::Clamp(Canvas->ClipY * 0.72f, 620.0f, 760.0f));
    const float Left = (Canvas->ClipX - PanelWidth) * 0.5f;
    const float Top = (Canvas->ClipY - PanelHeight) * 0.5f;
    const float ContentScale = FMath::Clamp(
        FMath::Min(PanelWidth / 940.0f, PanelHeight / 650.0f),
        0.76f,
        1.22f);
    const float TextScale = HudScale * ContentScale;
    const FLinearColor Backdrop =
        bHighContrast ? FLinearColor(0.0f, 0.0f, 0.0f, 1.0f)
                      : FLinearColor(0.005f, 0.012f, 0.026f, 0.985f);
    const FLinearColor Accent =
        bHighContrast ? FLinearColor(1.0f, 0.9f, 0.1f)
                      : FLinearColor(0.15f, 0.88f, 1.0f);
    const FLinearColor Body =
        bHighContrast ? FLinearColor::White : FLinearColor(0.84f, 0.9f, 0.95f);
    const FLinearColor Muted =
        bHighContrast ? FLinearColor(0.9f, 0.9f, 0.9f)
                      : FLinearColor(0.55f, 0.64f, 0.73f);
    UFont* SmallFont = GEngine != nullptr ? GEngine->GetSmallFont() : nullptr;
    const FString AccessLine = FString::Printf(
        TEXT("[U] UI %d%%    [I] HIGH CONTRAST %s    [O] REDUCED MOTION %s    [/] REDUCED FLASHING %s"),
        FMath::RoundToInt(HudScale * 100.0f),
        bHighContrast ? TEXT("ON") : TEXT("OFF"),
        Settings != nullptr && Settings->IsReducedMotionEnabled()
            ? TEXT("ON") : TEXT("OFF"),
        Settings != nullptr && Settings->IsReducedFlashingEnabled()
            ? TEXT("ON") : TEXT("OFF"));
    const FString AudioAccessLine = FString::Printf(
        TEXT("[PAGE DOWN] EFFECTS %d%%    [SHIFT+PAGE DOWN] REDUCED DYNAMIC RANGE %s"),
        FMath::RoundToInt(
            (Settings != nullptr ? Settings->GetEffectsVolume() : 1.0f) *
            100.0f),
        Settings != nullptr && Settings->IsReducedDynamicRangeEnabled()
            ? TEXT("ON") : TEXT("OFF"));
    const FString LocalFaction = EchoesController->GetLocalFactionLabel();
    const FString OpponentFaction = EchoesController->GetOpponentFactionLabel();
    const UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    const bool bPrologue = Bridge != nullptr &&
        Bridge->GetOperationMode() == EEchoesOperationMode::CampaignPrologue;
    const bool bSevenAccounts = Bridge != nullptr &&
        Bridge->GetOperationMode() ==
            EEchoesOperationMode::CampaignSevenAccounts;
    const bool bCityReserve = Bridge != nullptr &&
        Bridge->GetOperationMode() ==
            EEchoesOperationMode::CampaignCityReserve;
    const bool bUnburiedRoad = Bridge != nullptr &&
        Bridge->GetOperationMode() ==
            EEchoesOperationMode::CampaignUnburiedRoad;
    const bool bTermsOfContinuance = Bridge != nullptr &&
        Bridge->GetOperationMode() ==
            EEchoesOperationMode::CampaignTermsOfContinuance;
    const bool bNamesWithoutBirths = Bridge != nullptr &&
        Bridge->GetOperationMode() ==
            EEchoesOperationMode::CampaignNamesWithoutBirths;
    const bool bShapeOfSilence = Bridge != nullptr &&
        Bridge->GetOperationMode() ==
            EEchoesOperationMode::CampaignShapeOfSilence;
    const bool bCanStartNewCampaign = Bridge != nullptr &&
        !Bridge->GetCampaignProgress().Decisions.IsEmpty();
    const bool bCanRestoreCampaign = Bridge != nullptr &&
        Bridge->HasRestorableCampaignBackup();
    const int32 ActiveCampaignRecords = Bridge != nullptr
        ? Bridge->GetCampaignProgress().Decisions.Num()
        : 0;
    const int32 BackupCampaignRecords = Bridge != nullptr
        ? Bridge->GetCampaignBackupDecisionCount()
        : 0;
    const bool bNewCampaignArmed =
        EchoesController->IsNewCampaignConfirmationArmed();
    const bool bCampaignRestoreArmed =
        EchoesController->IsCampaignRestoreConfirmationArmed();

    DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.68f), 0.0f, 0.0f,
             Canvas->ClipX, Canvas->ClipY);
    DrawRect(Backdrop, Left, Top, PanelWidth, PanelHeight);
    DrawLine(Left, Top, Left + PanelWidth, Top, Accent, 4.0f);
    DrawLine(Left, Top + PanelHeight, Left + PanelWidth, Top + PanelHeight,
             Accent, 4.0f);

    DrawText(TEXT("ECHOES OF THE BROKEN SUN"), Accent,
             Left + 48.0f, Top + 42.0f * ContentScale,
             SmallFont, 1.85f * TextScale, false);
    DrawText(TEXT("THE FUTURE IS A RESOURCE. EVERY USE DESTROYS AN ALTERNATIVE."),
             Muted, Left + 48.0f, Top + 94.0f * ContentScale,
             SmallFont, 0.88f * TextScale, false);

    DrawText(TEXT("AVAILABLE OPERATION"), Accent,
             Left + 48.0f, Top + 164.0f * ContentScale,
             SmallFont, 0.92f * TextScale, false);
    DrawText(bPrologue ? TEXT("WHAT THE LEDGER KEEPS")
             : bSevenAccounts ? TEXT("SEVEN ACCOUNTS OF RAIN")
             : bCityReserve ? TEXT("A CITY ON RESERVE")
             : bUnburiedRoad ? TEXT("THE UNBURIED ROAD")
             : bTermsOfContinuance ? TEXT("TERMS OF CONTINUANCE")
             : bNamesWithoutBirths ? TEXT("NAMES WITHOUT BIRTHS")
             : bShapeOfSilence ? TEXT("THE SHAPE OF SILENCE")
                              : TEXT("GLASS SCAR"), Body,
             Left + 48.0f, Top + 202.0f * ContentScale,
             SmallFont, 1.42f * TextScale, false);
    const FString OperationMetadata = bPrologue
        ? FString::Printf(
              TEXT("CAMPAIGN PROLOGUE  //  %s  //  MARA VEY"),
              *LocalFaction)
        : bSevenAccounts
            ? FString::Printf(
                  TEXT("CAMPAIGN MISSION 02  //  %s  //  ORUUN"),
                  *LocalFaction)
        : bCityReserve
            ? FString::Printf(
                  TEXT("CAMPAIGN MISSION 03  //  %s  //  MARA VEY"),
                  *LocalFaction)
        : bUnburiedRoad
            ? FString::Printf(
                  TEXT("CAMPAIGN MISSION 04  //  %s  //  ORUUN"),
                  *LocalFaction)
        : bTermsOfContinuance
            ? TEXT("CAMPAIGN MISSION 05  //  MERIDIAN TREATY PROXIES")
        : bNamesWithoutBirths
            ? TEXT("CAMPAIGN MISSION 06  //  TALAR + CIVILIAN PROXIES")
        : bShapeOfSilence
            ? FString::Printf(
                  TEXT("CAMPAIGN MISSION 07  //  %s  //  ORUUN + MEMORY WITNESSES"),
                  *LocalFaction)
        : FString::Printf(
              TEXT("SINGLE-PLAYER SKIRMISH  //  %s  //  FUTURE WELL CONTEST"),
              *LocalFaction);
    DrawText(OperationMetadata,
             Muted, Left + 48.0f, Top + 246.0f * ContentScale,
             SmallFont, 0.86f * TextScale, false);
    FString OperationControlLine = bPrologue
        ? TEXT("[F9] CHANGE OPERATION  //  MERIDIAN COMPACT  //  MISSION 01")
    : bSevenAccounts
        ? TEXT("[F9] CHANGE OPERATION  //  KHARUUN ASSEMBLIES  //  MISSION 02")
    : bCityReserve
        ? TEXT("[F9] CHANGE OPERATION  //  MERIDIAN COMPACT  //  MISSION 03")
    : bUnburiedRoad
        ? TEXT("[F9] CHANGE OPERATION  //  KHARUUN ASSEMBLIES  //  MISSION 04")
    : bTermsOfContinuance
        ? TEXT("[F9] CHANGE OPERATION  //  MERIDIAN PROXY AUTHORITY  //  MISSION 05")
    : bNamesWithoutBirths
        ? TEXT("[F9] CHANGE OPERATION  //  MERIDIAN AUTHORITY  //  MISSION 06")
    : bShapeOfSilence
        ? TEXT("[F9] CHANGE OPERATION  //  KHARUUN ASSEMBLIES  //  MISSION 07")
        : FString::Printf(
              TEXT("[F9] CHANGE OPERATION  //  [TAB] FACTION  //  ADAPTIVE %s"),
              *OpponentFaction);
    DrawText(
        OperationControlLine,
             Accent, Left + 48.0f, Top + 272.0f * ContentScale,
             SmallFont, 0.80f * TextScale, false);
    FString CampaignControlLine = FString::Printf(
        TEXT("CAMPAIGN  //  ACTIVE %d RECORD%s"),
        ActiveCampaignRecords,
        ActiveCampaignRecords == 1 ? TEXT("") : TEXT("S"));
    if (bCanStartNewCampaign)
    {
        CampaignControlLine += TEXT("  //  [F10] NEW");
    }
    if (bCanRestoreCampaign)
    {
        CampaignControlLine += FString::Printf(
            TEXT("  //  [PAGE UP] RESTORE PRIOR %d"),
            BackupCampaignRecords);
    }
    DrawText(
        CampaignControlLine,
        Muted, Left + 48.0f, Top + 298.0f * ContentScale,
        SmallFont, 0.78f * TextScale, false);
    DrawText(
        bNewCampaignArmed
            ? TEXT("NEW CAMPAIGN CONFIRMATION ARMED — ACTIVE PROGRESS WILL BE REPLACED.")
        : bCampaignRestoreArmed
            ? TEXT("RESTORE CONFIRMATION ARMED — VALIDATED PRIOR PROGRESS WILL BECOME ACTIVE.")
        : bPrologue
            ? TEXT("Recover Talar Venn's displaced archive before the line collapses.")
        : bSevenAccounts
            ? TEXT("Carry the inherited account into terrain changed by the prior decision.")
        : bCityReserve
            ? TEXT("Lume Reach is running on reserve. Three districts are outside the stable grid.")
        : bUnburiedRoad
            ? TEXT("A route absent from every current map carries the echo of a missing memory shard.")
        : bTermsOfContinuance
            ? TEXT("A ceasefire channel opens under Meridian authority as generic unresolved pressure enters the operation.")
        : bNamesWithoutBirths
            ? TEXT("Five consistent records expose a branch-specific census whose named people were never assigned births.")
        : bShapeOfSilence
            ? TEXT("Six consistent records lead Oruun to a communal-memory hollow shaped like the recovered census absence.")
            : TEXT("Cross the shattered approaches, choose what the Well becomes,"),
             Body, Left + 48.0f, Top + 334.0f * ContentScale,
             SmallFont, 0.96f * TextScale, false);
    DrawText(
        bNewCampaignArmed
            ? TEXT("PRESS F10 AGAIN WITHIN 10 SECONDS. ONE PRIOR LEDGER GENERATION IS RETAINED.")
        : bCampaignRestoreArmed
            ? TEXT("PRESS PAGE UP AGAIN WITHIN 30 SECONDS. THE CURRENT GENERATION BECOMES THE BACKUP.")
        : bPrologue
            ? TEXT("Commit the Well's consequence, then withdraw to Lume Reach.")
        : bSevenAccounts
            ? TEXT("Re-root the Waystone, then bring Oruun to the matching memory site.")
        : bCityReserve
            ? TEXT("Extend Power Links and energize every district in the inherited priority order.")
        : bUnburiedRoad
            ? TEXT("Root the Waystone, raise a Listening Spine beyond the crossing, and recover the shard.")
        : bTermsOfContinuance
            ? TEXT("Synchronize both treaty proxies by tick 300, hold through tick 900, then extract both witness proxies.")
        : bNamesWithoutBirths
            ? TEXT("Locate the census with Talar, power its archive, shelter both civilians, then extract the evidence.")
        : bShapeOfSilence
            ? TEXT("Root the Waystone, raise a Listening Spine, place both witnesses, then reach the confluence.")
            : TEXT("and break the opposing Command Core before your own line collapses."),
             Body, Left + 48.0f, Top + 362.0f * ContentScale,
             SmallFont, 0.96f * TextScale, false);

    DrawText(TEXT("ACCESSIBILITY BEFORE DEPLOYMENT"), Accent,
             Left + 48.0f, Top + 410.0f * ContentScale,
             SmallFont, 0.90f * TextScale, false);
    DrawText(AccessLine, Body,
             Left + 48.0f, Top + 444.0f * ContentScale,
             SmallFont, 0.80f * TextScale, false);
    DrawText(AudioAccessLine, Body,
             Left + 48.0f, Top + 474.0f * ContentScale,
             SmallFont, 0.78f * TextScale, false);

    DrawRect(Accent, Left + 48.0f, Top + PanelHeight - 82.0f,
             PanelWidth - 96.0f, 46.0f);
    DrawText(TEXT("PRESS ENTER TO OPEN THE OPERATIONS BRIEF"),
             bHighContrast ? FLinearColor::Black
                           : FLinearColor(0.0f, 0.06f, 0.09f),
             Left + PanelWidth * 0.5f - 174.0f * TextScale,
             Top + PanelHeight - 69.0f,
             SmallFont, 0.94f * TextScale, false);
}

void AEchoesHUD::DrawObjectiveTracker(
    const UEchoesSimulationSubsystem* Bridge,
    const UEchoesGameUserSettings* Settings)
{
    if (Canvas == nullptr || Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        return;
    }

    const FEchoesObjectiveSnapshot Objective =
        Bridge->GetLocalObjectiveSnapshot();
    if (!Objective.bScenarioReady)
    {
        return;
    }

    const float HudScale = Settings != nullptr ? Settings->GetHudScale() : 1.0f;
    const bool bHighContrast =
        Settings != nullptr && Settings->IsHighContrastHudEnabled();
    const float PanelWidth = FMath::Clamp(460.0f * HudScale, 390.0f, 560.0f);
    const float PanelHeight = FMath::Clamp(178.0f * HudScale, 160.0f, 212.0f);
    float Left = Canvas->ClipX - PanelWidth - 20.0f;
    float Top = 18.0f;
    const float MainPanelRight =
        18.0f + FMath::Min(920.0f * HudScale, FMath::Max(320.0f, Canvas->ClipX - 36.0f));
    if (Left < MainPanelRight + 20.0f)
    {
        Left = 18.0f;
        Top = 310.0f;
    }
    if (Top + PanelHeight > Canvas->ClipY - 24.0f)
    {
        return;
    }

    const FLinearColor Backdrop =
        bHighContrast ? FLinearColor(0.0f, 0.0f, 0.0f, 0.98f)
                      : FLinearColor(0.008f, 0.018f, 0.035f, 0.93f);
    const FLinearColor Accent =
        bHighContrast ? FLinearColor(1.0f, 0.9f, 0.1f)
                      : FLinearColor(0.15f, 0.88f, 1.0f);
    const FLinearColor Active =
        bHighContrast ? FLinearColor::White : FLinearColor(0.78f, 0.86f, 0.92f);
    const FLinearColor Complete = FLinearColor(0.25f, 1.0f, 0.66f);
    const FLinearColor Failed = FLinearColor(1.0f, 0.35f, 0.18f);
    UFont* SmallFont = GEngine != nullptr ? GEngine->GetSmallFont() : nullptr;
    const float TextScale = FMath::Clamp(HudScale, 0.82f, 1.2f);

    if (Objective.OperationMode == EEchoesOperationMode::CampaignPrologue)
    {
        const bool bFailed =
            Objective.ProloguePhase == EEchoesProloguePhase::Failed;
        const bool bArchiveRecovered =
            Objective.ProloguePhase == EEchoesProloguePhase::DecideFutureWell ||
            Objective.ProloguePhase == EEchoesProloguePhase::Withdraw ||
            Objective.ProloguePhase == EEchoesProloguePhase::Complete;
        const bool bProtocolChosen =
            Objective.PrologueWellChoice !=
            echoes::sim::FutureWellChoice::Dormant;
        const FString ArchiveState = bFailed
            ? TEXT("LOST — MISSION FAILED")
            : bArchiveRecovered
                ? TEXT("RECOVERED — MARA VEY SECURE")
                : TEXT("RENDEZVOUS — TILE 22,18");
        const FString WellState = bProtocolChosen
            ? FString::Printf(
                  TEXT("COMMITTED — %s"),
                  WellChoiceDisplayName(Objective.PrologueWellChoice))
            : Objective.ProloguePhase == EEchoesProloguePhase::DecideFutureWell
                ? TEXT("AUTHORIZE Z / C / V AT WELL")
                : TEXT("LOCKED — RECOVER ARCHIVE FIRST");
        const FString WithdrawalState =
            Objective.ProloguePhase == EEchoesProloguePhase::Complete
                ? TEXT("COMPLETE — LUME REACH")
                : Objective.ProloguePhase == EEchoesProloguePhase::Withdraw
                    ? TEXT("RETURN — TILE 6,17")
                    : bFailed ? TEXT("FAILED") : TEXT("AWAITING WELL DECISION");

        DrawRect(Backdrop, Left, Top, PanelWidth, PanelHeight);
        DrawLine(Left, Top, Left + PanelWidth, Top, Accent, 2.0f);
        DrawText(TEXT("WHAT THE LEDGER KEEPS  //  MISSION 01"), Accent,
                 Left + 18.0f, Top + 15.0f, SmallFont, 0.90f * TextScale, false);
        DrawText(FString::Printf(TEXT("01  ARCHIVE CARRIER  %s"), *ArchiveState),
                 bFailed ? Failed : bArchiveRecovered ? Complete : Active,
                 Left + 18.0f, Top + 52.0f, SmallFont, 0.80f * TextScale, false);
        DrawText(FString::Printf(TEXT("02  FUTURE WELL     %s"), *WellState),
                 bProtocolChosen ? Complete : Active,
                 Left + 18.0f, Top + 89.0f, SmallFont, 0.80f * TextScale, false);
        DrawText(FString::Printf(TEXT("03  WITHDRAWAL      %s"), *WithdrawalState),
                 bFailed ? Failed
                         : Objective.ProloguePhase == EEchoesProloguePhase::Complete
                               ? Complete
                               : Active,
                 Left + 18.0f, Top + 126.0f, SmallFont, 0.80f * TextScale, false);
        if (!bLoggedObjectiveTrackerReady)
        {
            bLoggedObjectiveTrackerReady = true;
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_PROLOGUE_OBJECTIVES_READY] phase=%s carrier=%u reconstructable=true"),
                FEchoesPrologueMissionModel::StableName(Objective.ProloguePhase),
                Objective.ArchiveCarrierId);
        }
        return;
    }

    if (Objective.OperationMode ==
        EEchoesOperationMode::CampaignSevenAccounts)
    {
        const FEchoesSevenAccountsRoute Route = Bridge->GetSevenAccountsRoute();
        const bool bFailed =
            Objective.SevenAccountsPhase == EEchoesSevenAccountsPhase::Failed;
        const FString WaystoneState = bFailed
            ? TEXT("LOST — MISSION FAILED")
            : Objective.bWaystoneRootedAtAnchor
                ? TEXT("ROOTED — ANCHOR SECURE")
                : FString::Printf(
                      TEXT("MIGRATE — TILE %d,%d"),
                      Route.WaystoneAnchor.x.FloorToInt(),
                      Route.WaystoneAnchor.y.FloorToInt());
        const FString MemoryState = bFailed
            ? TEXT("LOST — MISSION FAILED")
            : Objective.bMemoryBearerAtAccountSite
                ? TEXT("ORUUN — ACCOUNT RECALLED")
                : Objective.bWaystoneRootedAtAnchor
                    ? FString::Printf(
                          TEXT("ORUUN — TILE %d,%d"),
                          Route.MemoryAccountSite.x.FloorToInt(),
                          Route.MemoryAccountSite.y.FloorToInt())
                    : TEXT("WAITING — ROOT WAYSTONE FIRST");
        DrawRect(Backdrop, Left, Top, PanelWidth, PanelHeight);
        DrawLine(Left, Top, Left + PanelWidth, Top, Accent, 2.0f);
        DrawText(TEXT("SEVEN ACCOUNTS OF RAIN  //  MISSION 02"), Accent,
                 Left + 18.0f, Top + 15.0f, SmallFont, 0.90f * TextScale, false);
        DrawText(
            FString::Printf(TEXT("01  WAYSTONE        %s"), *WaystoneState),
            bFailed ? Failed : Objective.bWaystoneRootedAtAnchor ? Complete : Active,
            Left + 18.0f, Top + 52.0f, SmallFont, 0.80f * TextScale, false);
        DrawText(
            FString::Printf(TEXT("02  MEMORY ACCOUNT  %s"), *MemoryState),
            bFailed ? Failed : Objective.bMemoryBearerAtAccountSite ? Complete : Active,
            Left + 18.0f, Top + 89.0f, SmallFont, 0.80f * TextScale, false);
        DrawText(
            FString::Printf(
                TEXT("03  PRIOR DECISION  %s // %s"),
                WellChoiceDisplayName(Objective.SevenAccountsBranch),
                Route.DisplayName),
            Complete,
            Left + 18.0f, Top + 126.0f, SmallFont, 0.80f * TextScale, false);
        if (!bLoggedObjectiveTrackerReady)
        {
            bLoggedObjectiveTrackerReady = true;
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_SEVEN_ACCOUNTS_OBJECTIVES_READY] phase=%s bearer=%u waystone=%u branch=%s reconstructable=true"),
                FEchoesSevenAccountsMissionModel::StableName(
                    Objective.SevenAccountsPhase),
                Objective.MemoryBearerId,
                Objective.MigrationWaystoneId,
                Route.StableName);
        }
        return;
    }

    if (Objective.OperationMode ==
        EEchoesOperationMode::CampaignCityReserve)
    {
        const FEchoesCityReserveGrid Grid = Bridge->GetCityReserveGrid();
        const bool bFailed =
            Objective.CityReservePhase == EEchoesCityReservePhase::Failed;
        const auto IsPowered = [&Objective](EEchoesCityDistrict District)
        {
            switch (District)
            {
                case EEchoesCityDistrict::LifeSupport:
                    return Objective.bLifeSupportPowered;
                case EEchoesCityDistrict::Transit:
                    return Objective.bTransitPowered;
                case EEchoesCityDistrict::Archive:
                    return Objective.bArchivePowered;
            }
            return false;
        };
        const auto DistrictLine = [&IsPowered, bFailed](
                                      EEchoesCityDistrict District)
        {
            return FString::Printf(
                TEXT("%s — %s"),
                FEchoesCityReserveMissionModel::DistrictDisplayName(District),
                bFailed ? TEXT("GRID LOST")
                        : IsPowered(District) ? TEXT("POWERED")
                                              : TEXT("CONNECT POWER LINK"));
        };
        DrawRect(Backdrop, Left, Top, PanelWidth, PanelHeight);
        DrawLine(Left, Top, Left + PanelWidth, Top, Accent, 2.0f);
        DrawText(TEXT("A CITY ON RESERVE  //  MISSION 03"), Accent,
                 Left + 18.0f, Top + 15.0f, SmallFont,
                 0.90f * TextScale, false);
        DrawText(
            FString::Printf(TEXT("01  %s"), *DistrictLine(Grid.Priority)),
            bFailed ? Failed : IsPowered(Grid.Priority) ? Complete : Active,
            Left + 18.0f, Top + 52.0f, SmallFont,
            0.80f * TextScale, false);
        DrawText(
            FString::Printf(TEXT("02  %s"), *DistrictLine(Grid.Secondary)),
            bFailed ? Failed : IsPowered(Grid.Secondary) ? Complete : Active,
            Left + 18.0f, Top + 89.0f, SmallFont,
            0.80f * TextScale, false);
        DrawText(
            FString::Printf(TEXT("03  %s"), *DistrictLine(Grid.Final)),
            bFailed ? Failed : IsPowered(Grid.Final) ? Complete : Active,
            Left + 18.0f, Top + 126.0f, SmallFont,
            0.80f * TextScale, false);
        if (!bLoggedObjectiveTrackerReady)
        {
            bLoggedObjectiveTrackerReady = true;
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_CITY_RESERVE_OBJECTIVES_READY] phase=%s branch=%s districts=%u,%u,%u reconstructable=true"),
                FEchoesCityReserveMissionModel::StableName(
                    Objective.CityReservePhase),
                Grid.StableName,
                Objective.LifeSupportDistrictId,
                Objective.TransitDistrictId,
                Objective.ArchiveDistrictId);
        }
        return;
    }

    if (Objective.OperationMode ==
        EEchoesOperationMode::CampaignUnburiedRoad)
    {
        const FEchoesUnburiedRoadRoute Route =
            Bridge->GetUnburiedRoadRoute();
        const bool bFailed =
            Objective.UnburiedRoadPhase == EEchoesUnburiedRoadPhase::Failed;
        const FString WaystoneState = bFailed
            ? TEXT("ROADHEAD LOST")
            : Objective.bWaystoneRootedAtRoadhead
                ? TEXT("ROOTED")
                : FString::Printf(
                      TEXT("MOVE TO %d,%d"),
                      Route.Roadhead.x.FloorToInt(),
                      Route.Roadhead.y.FloorToInt());
        const FString SpineState = bFailed
            ? TEXT("SPINE LOST")
            : Objective.bListeningSpineComplete
                ? TEXT("LISTENING")
                : Objective.bWaystoneRootedAtRoadhead
                    ? FString::Printf(
                          TEXT("BUILD AT %d,%d"),
                          Route.ListeningSpineSite.x.FloorToInt(),
                          Route.ListeningSpineSite.y.FloorToInt())
                    : TEXT("WAITING — ROOT WAYSTONE");
        const FString ShardState = bFailed
            ? TEXT("SHARD LOST")
            : Objective.bMemoryBearerAtShard
                ? TEXT("RECOVERED")
                : Objective.bListeningSpineComplete
                    ? FString::Printf(
                          TEXT("ORUUN TO %d,%d"),
                          Route.MemoryShardSite.x.FloorToInt(),
                          Route.MemoryShardSite.y.FloorToInt())
                    : TEXT("WAITING — RAISE SPINE");
        DrawRect(Backdrop, Left, Top, PanelWidth, PanelHeight);
        DrawLine(Left, Top, Left + PanelWidth, Top, Accent, 2.0f);
        DrawText(TEXT("THE UNBURIED ROAD  //  MISSION 04"), Accent,
                 Left + 18.0f, Top + 15.0f, SmallFont,
                 0.90f * TextScale, false);
        DrawText(
            FString::Printf(TEXT("01  WAYSTONE ROADHEAD  %s"), *WaystoneState),
            bFailed ? Failed
                    : Objective.bWaystoneRootedAtRoadhead ? Complete : Active,
            Left + 18.0f, Top + 52.0f, SmallFont,
            0.80f * TextScale, false);
        DrawText(
            FString::Printf(TEXT("02  LISTENING SPINE    %s"), *SpineState),
            bFailed ? Failed
                    : Objective.bListeningSpineComplete ? Complete : Active,
            Left + 18.0f, Top + 89.0f, SmallFont,
            0.80f * TextScale, false);
        DrawText(
            FString::Printf(TEXT("03  MEMORY SHARD       %s"), *ShardState),
            bFailed ? Failed
                    : Objective.bMemoryBearerAtShard ? Complete : Active,
            Left + 18.0f, Top + 126.0f, SmallFont,
            0.80f * TextScale, false);
        if (!bLoggedObjectiveTrackerReady)
        {
            bLoggedObjectiveTrackerReady = true;
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_UNBURIED_ROAD_OBJECTIVES_READY] phase=%s branch=%s bearer=%u waystone=%u reconstructable=true"),
                FEchoesUnburiedRoadMissionModel::StableName(
                    Objective.UnburiedRoadPhase),
                Route.StableName,
                Objective.MemoryBearerId,
                Objective.MigrationWaystoneId);
        }
        return;
    }

    if (Objective.OperationMode ==
        EEchoesOperationMode::CampaignTermsOfContinuance)
    {
        const FEchoesTermsOfContinuancePlan Plan =
            Bridge->GetTermsOfContinuancePlan();
        const bool bFailed =
            Objective.TermsOfContinuancePhase ==
            EEchoesTermsOfContinuancePhase::Failed;
        const bool bNetworksReady =
            Objective.bMeridianRelaySynchronized &&
            Objective.bKharuunSpineSynchronized;
        const bool bWitnessesExtracted =
            Objective.bMeridianWitnessExtracted &&
            Objective.bKharuunWitnessExtracted;
        const FString NetworkState = bFailed
            ? TEXT("INTERFACES LOST")
            : bNetworksReady
                ? TEXT("SYNCHRONIZED")
                : FString::Printf(
                      TEXT("MERIDIAN %s  KHARUUN %s"),
                      Objective.bMeridianRelaySynchronized
                          ? TEXT("UP") : TEXT("DOWN"),
                      Objective.bKharuunSpineSynchronized
                          ? TEXT("UP") : TEXT("DOWN"));
        const FString HoldState = bFailed
            ? TEXT("WINDOW BROKEN")
            : Objective.bContinuanceWindowHeld
                ? TEXT("HELD")
                : bNetworksReady
                    ? FString::Printf(
                          TEXT("HOLD TO TICK %llu"),
                          static_cast<unsigned long long>(
                              Plan.ContinuanceWindowEndTick))
                    : TEXT("WAITING — SYNC NETWORKS");
        const FString WitnessState = bFailed
            ? TEXT("WITNESS LOST")
            : bWitnessesExtracted
                ? TEXT("BOTH EXTRACTED")
                : Objective.bContinuanceWindowHeld
                    ? FString::Printf(
                          TEXT("BOTH TO %d,%d"),
                          Plan.WitnessExtractionSite.x.FloorToInt(),
                          Plan.WitnessExtractionSite.y.FloorToInt())
                    : TEXT("WAITING — HOLD WINDOW");
        DrawRect(Backdrop, Left, Top, PanelWidth, PanelHeight);
        DrawLine(Left, Top, Left + PanelWidth, Top, Accent, 2.0f);
        DrawText(TEXT("TERMS OF CONTINUANCE  //  MISSION 05"), Accent,
                 Left + 18.0f, Top + 15.0f, SmallFont,
                 0.90f * TextScale, false);
        DrawText(
            FString::Printf(TEXT("01  NETWORK INTERFACES  %s"), *NetworkState),
            bFailed ? Failed : bNetworksReady ? Complete : Active,
            Left + 18.0f, Top + 52.0f, SmallFont,
            0.80f * TextScale, false);
        DrawText(
            FString::Printf(TEXT("02  CONTINUANCE WINDOW  %s"), *HoldState),
            bFailed ? Failed
                    : Objective.bContinuanceWindowHeld ? Complete : Active,
            Left + 18.0f, Top + 89.0f, SmallFont,
            0.80f * TextScale, false);
        DrawText(
            FString::Printf(TEXT("03  WITNESS PROXIES     %s"), *WitnessState),
            bFailed ? Failed : bWitnessesExtracted ? Complete : Active,
            Left + 18.0f, Top + 126.0f, SmallFont,
            0.80f * TextScale, false);
        if (!bLoggedObjectiveTrackerReady)
        {
            bLoggedObjectiveTrackerReady = true;
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_TERMS_OF_CONTINUANCE_OBJECTIVES_READY] phase=%s branch=%s meridianRelay=%u kharuunSpine=%u meridianWitness=%u kharuunWitness=%u reconstructable=true"),
                FEchoesTermsOfContinuanceMissionModel::StableName(
                    Objective.TermsOfContinuancePhase),
                Plan.StableName,
                Objective.MeridianContinuanceRelayId,
                Objective.KharuunContinuanceSpineId,
                Objective.MeridianContinuanceWitnessId,
                Objective.KharuunContinuanceWitnessId);
        }
        return;
    }

    if (Objective.OperationMode ==
        EEchoesOperationMode::CampaignNamesWithoutBirths)
    {
        const FEchoesNamesWithoutBirthsPlan Plan =
            Bridge->GetNamesWithoutBirthsPlan();
        const bool bFailed =
            Objective.NamesWithoutBirthsPhase ==
            EEchoesNamesWithoutBirthsPhase::Failed;
        const bool bCiviliansSheltered =
            Objective.bFirstCivilianSheltered &&
            Objective.bSecondCivilianSheltered;
        const FString CensusState = bFailed
            ? TEXT("TRACE LOST")
            : Objective.bCensusEvidenceLocated
                ? TEXT("LOCATED")
                : FString::Printf(
                      TEXT("TALAR TO %d,%d"),
                      Plan.CensusSite.x.FloorToInt(),
                      Plan.CensusSite.y.FloorToInt());
        const FString ArchiveState = bFailed
            ? TEXT("ARCHIVE LOST")
            : Objective.bCensusArchivePowered
                ? TEXT("POWERED")
                : Objective.bCensusEvidenceLocated
                    ? FString::Printf(
                          TEXT("LINK AT %d,%d"),
                          Plan.PowerLinkSite.x.FloorToInt(),
                          Plan.PowerLinkSite.y.FloorToInt())
                    : TEXT("WAITING — LOCATE TRACE");
        const FString RecoveryState = bFailed
            ? TEXT("PROTECTED LINE LOST")
            : Objective.bTalarAtEvidenceExtraction && bCiviliansSheltered
                ? TEXT("EVIDENCE EXTRACTED")
                : bCiviliansSheltered
                    ? FString::Printf(
                          TEXT("TALAR TO %d,%d"),
                          Plan.EvidenceExtractionSite.x.FloorToInt(),
                          Plan.EvidenceExtractionSite.y.FloorToInt())
                    : Objective.bCensusArchivePowered
                        ? FString::Printf(
                              TEXT("BOTH CIVILIANS TO %d,%d"),
                              Plan.CivilianShelterSite.x.FloorToInt(),
                              Plan.CivilianShelterSite.y.FloorToInt())
                        : TEXT("WAITING — POWER ARCHIVE");
        DrawRect(Backdrop, Left, Top, PanelWidth, PanelHeight);
        DrawLine(Left, Top, Left + PanelWidth, Top, Accent, 2.0f);
        DrawText(TEXT("NAMES WITHOUT BIRTHS  //  MISSION 06"), Accent,
                 Left + 18.0f, Top + 15.0f, SmallFont,
                 0.90f * TextScale, false);
        DrawText(
            FString::Printf(TEXT("01  CENSUS TRACE       %s"), *CensusState),
            bFailed ? Failed : Objective.bCensusEvidenceLocated ? Complete : Active,
            Left + 18.0f, Top + 52.0f, SmallFont,
            0.80f * TextScale, false);
        DrawText(
            FString::Printf(TEXT("02  CENSUS ARCHIVE     %s"), *ArchiveState),
            bFailed ? Failed : Objective.bCensusArchivePowered ? Complete : Active,
            Left + 18.0f, Top + 89.0f, SmallFont,
            0.80f * TextScale, false);
        DrawText(
            FString::Printf(TEXT("03  PROTECTED RECOVERY %s"), *RecoveryState),
            bFailed ? Failed
                    : Objective.bTalarAtEvidenceExtraction && bCiviliansSheltered
                        ? Complete : Active,
            Left + 18.0f, Top + 126.0f, SmallFont,
            0.80f * TextScale, false);
        if (!bLoggedObjectiveTrackerReady)
        {
            bLoggedObjectiveTrackerReady = true;
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_NAMES_WITHOUT_BIRTHS_OBJECTIVES_READY] phase=%s branch=%s talar=%u archive=%u civilianA=%u civilianB=%u reconstructable=true"),
                FEchoesNamesWithoutBirthsMissionModel::StableName(
                    Objective.NamesWithoutBirthsPhase),
                Plan.StableName,
                Objective.TalarId,
                Objective.CensusArchiveId,
                Objective.FirstCivilianId,
                Objective.SecondCivilianId);
        }
        return;
    }

    if (Objective.OperationMode ==
        EEchoesOperationMode::CampaignShapeOfSilence)
    {
        const FEchoesShapeOfSilencePlan Plan =
            Bridge->GetShapeOfSilencePlan();
        const bool bFailed =
            Objective.ShapeOfSilencePhase ==
            EEchoesShapeOfSilencePhase::Failed;
        const bool bWitnessesPositioned =
            Objective.bFirstMemoryWitnessPositioned &&
            Objective.bSecondMemoryWitnessPositioned;
        const FString WaystoneState = bFailed
            ? TEXT("WAYSTONE LOST")
            : Objective.bShapeWaystoneRooted
                ? TEXT("ROOTED")
                : FString::Printf(
                      TEXT("ROOT AT %d,%d"),
                      Plan.WaystoneAnchor.x.FloorToInt(),
                      Plan.WaystoneAnchor.y.FloorToInt());
        const FString SpineState = bFailed
            ? TEXT("LISTENING LOST")
            : Objective.bShapeListeningSpineRaised
                ? TEXT("RAISED")
                : Objective.bShapeWaystoneRooted
                    ? FString::Printf(
                          TEXT("BUILD AT %d,%d"),
                          Plan.ListeningSpineSite.x.FloorToInt(),
                          Plan.ListeningSpineSite.y.FloorToInt())
                    : TEXT("WAITING — ROOT WAYSTONE");
        const FString ConfluenceState = bFailed
            ? TEXT("MEMORY LINE LOST")
            : Objective.bOruunAtConfluence
                ? TEXT("CORRESPONDENCE RECORDED")
                : bWitnessesPositioned
                    ? FString::Printf(
                          TEXT("ORUUN TO %d,%d"),
                          Plan.ConfluenceSite.x.FloorToInt(),
                          Plan.ConfluenceSite.y.FloorToInt())
                : Objective.bShapeListeningSpineRaised
                    ? TEXT("POSITION BOTH WITNESSES")
                    : TEXT("WAITING — RAISE SPINE");
        DrawRect(Backdrop, Left, Top, PanelWidth, PanelHeight);
        DrawLine(Left, Top, Left + PanelWidth, Top, Accent, 2.0f);
        DrawText(TEXT("THE SHAPE OF SILENCE  //  MISSION 07"), Accent,
                 Left + 18.0f, Top + 15.0f, SmallFont,
                 0.90f * TextScale, false);
        DrawText(
            FString::Printf(TEXT("01  WAYSTONE ANCHOR   %s"), *WaystoneState),
            bFailed ? Failed : Objective.bShapeWaystoneRooted ? Complete : Active,
            Left + 18.0f, Top + 52.0f, SmallFont,
            0.80f * TextScale, false);
        DrawText(
            FString::Printf(TEXT("02  LISTENING SPINE   %s"), *SpineState),
            bFailed ? Failed : Objective.bShapeListeningSpineRaised ? Complete : Active,
            Left + 18.0f, Top + 89.0f, SmallFont,
            0.80f * TextScale, false);
        DrawText(
            FString::Printf(TEXT("03  MEMORY CONFLUENCE %s"), *ConfluenceState),
            bFailed ? Failed : Objective.bOruunAtConfluence ? Complete : Active,
            Left + 18.0f, Top + 126.0f, SmallFont,
            0.80f * TextScale, false);
        if (!bLoggedObjectiveTrackerReady)
        {
            bLoggedObjectiveTrackerReady = true;
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_SHAPE_OF_SILENCE_OBJECTIVES_READY] phase=%s branch=%s oruun=%u witnessA=%u witnessB=%u reconstructable=true claimBoundary=correspondenceOnly"),
                FEchoesShapeOfSilenceMissionModel::StableName(
                    Objective.ShapeOfSilencePhase),
                Plan.StableName,
                Objective.OruunId,
                Objective.FirstMemoryWitnessId,
                Objective.SecondMemoryWitnessId);
        }
        return;
    }

    FString WellState = TEXT("UNLOCATED — SEARCH THE SCAR");
    FLinearColor WellColor = Active;
    if (Objective.bFutureWellVisible)
    {
        switch (Objective.VisibleFutureWellChoice)
        {
            case echoes::sim::FutureWellChoice::Harvest:
                WellState = TEXT("PROTOCOL ACTIVE — HARVEST");
                WellColor = Complete;
                break;
            case echoes::sim::FutureWellChoice::Preserve:
                WellState = TEXT("PROTOCOL ACTIVE — PRESERVE");
                WellColor = Complete;
                break;
            case echoes::sim::FutureWellChoice::Reshape:
                WellState = TEXT("PROTOCOL ACTIVE — RESHAPE");
                WellColor = Complete;
                break;
            case echoes::sim::FutureWellChoice::Dormant:
            default:
                WellState = TEXT("IN SIGHT — AWAITING PROTOCOL");
                break;
        }
    }

    const FString LocalCoreState = Objective.bLocalCoreIntact
        ? FString::Printf(
              TEXT("SECURE — %d / %d INTEGRITY"),
              Objective.LocalCoreHitPoints,
              Objective.LocalCoreMaxHitPoints)
        : TEXT("LOST");
    FString HostileCoreState = Objective.bHostileCoreVisible
                                   ? TEXT("IN SIGHT — DESTROY")
                                   : TEXT("UNLOCATED — RECONNOITER");
    FLinearColor HostileCoreColor = Active;
    if (Objective.Outcome == echoes::sim::MatchOutcome::Player0Victory)
    {
        HostileCoreState = TEXT("ELIMINATED");
        HostileCoreColor = Complete;
    }

    DrawRect(Backdrop, Left, Top, PanelWidth, PanelHeight);
    DrawLine(Left, Top, Left + PanelWidth, Top, Accent, 2.0f);
    DrawText(TEXT("OPERATION GLASS SCAR  //  OBJECTIVES"), Accent,
             Left + 18.0f, Top + 15.0f, SmallFont, 0.90f * TextScale, false);
    DrawText(FString::Printf(TEXT("01  FUTURE WELL     %s"), *WellState), WellColor,
             Left + 18.0f, Top + 52.0f, SmallFont, 0.82f * TextScale, false);
    DrawText(FString::Printf(TEXT("02  COMMAND CORE    %s"), *LocalCoreState),
             Objective.bLocalCoreIntact ? Active : Failed,
             Left + 18.0f, Top + 89.0f, SmallFont, 0.82f * TextScale, false);
    const FString OpponentShortName =
        Bridge->GetOpponentFaction() == echoes::sim::Faction::KharuunAssemblies
            ? TEXT("KHARUUN")
            : TEXT("MERIDIAN");
    DrawText(FString::Printf(
                 TEXT("03  %s CORE    %s"),
                 *OpponentShortName,
                 *HostileCoreState),
             HostileCoreColor,
             Left + 18.0f, Top + 126.0f, SmallFont, 0.82f * TextScale, false);

    if (!bLoggedObjectiveTrackerReady)
    {
        bLoggedObjectiveTrackerReady = true;
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_OBJECTIVES_READY] visibilityScoped=true wellVisible=%s hostileCoreVisible=%s"),
            Objective.bFutureWellVisible ? TEXT("true") : TEXT("false"),
            Objective.bHostileCoreVisible ? TEXT("true") : TEXT("false"));
    }
}

void AEchoesHUD::DrawMatchResult(
    const AEchoesPlayerController* EchoesController,
    const UEchoesSimulationSubsystem* Bridge,
    const UEchoesGameUserSettings* Settings)
{
    if (Canvas == nullptr || EchoesController == nullptr ||
        !EchoesController->IsMatchResultVisible())
    {
        return;
    }

    const bool bHighContrast =
        Settings != nullptr && Settings->IsHighContrastHudEnabled();
    const float HudScale = Settings != nullptr ? Settings->GetHudScale() : 1.0f;
    const float PanelWidth = FMath::Min(
        FMath::Max(620.0f, Canvas->ClipX - 60.0f),
        FMath::Clamp(Canvas->ClipX * 0.54f, 760.0f, 1080.0f));
    const float PanelHeight = FMath::Min(
        FMath::Max(390.0f, Canvas->ClipY - 60.0f),
        FMath::Clamp(Canvas->ClipY * 0.48f, 430.0f, 590.0f));
    const float Left = (Canvas->ClipX - PanelWidth) * 0.5f;
    const float Top = (Canvas->ClipY - PanelHeight) * 0.5f;
    const float ContentScale = FMath::Clamp(
        FMath::Min(PanelWidth / 820.0f, PanelHeight / 460.0f),
        0.76f,
        1.25f);
    const float TextScale = HudScale * ContentScale;
    const echoes::sim::MatchOutcome Outcome =
        EchoesController->GetPresentedMatchOutcome();
    const bool bCampaignResult = EchoesController->IsCampaignResult();
    const bool bSevenAccountsResult = bCampaignResult &&
        EchoesController->GetPresentedCampaignOperation() ==
            EEchoesOperationMode::CampaignSevenAccounts;
    const bool bCityReserveResult = bCampaignResult &&
        EchoesController->GetPresentedCampaignOperation() ==
            EEchoesOperationMode::CampaignCityReserve;
    const bool bUnburiedRoadResult = bCampaignResult &&
        EchoesController->GetPresentedCampaignOperation() ==
            EEchoesOperationMode::CampaignUnburiedRoad;
    const bool bTermsOfContinuanceResult = bCampaignResult &&
        EchoesController->GetPresentedCampaignOperation() ==
            EEchoesOperationMode::CampaignTermsOfContinuance;
    const bool bNamesWithoutBirthsResult = bCampaignResult &&
        EchoesController->GetPresentedCampaignOperation() ==
            EEchoesOperationMode::CampaignNamesWithoutBirths;
    const bool bShapeOfSilenceResult = bCampaignResult &&
        EchoesController->GetPresentedCampaignOperation() ==
            EEchoesOperationMode::CampaignShapeOfSilence;
    const bool bVictory = bCampaignResult
        ? EchoesController->WasCampaignSuccessful()
        : Outcome == echoes::sim::MatchOutcome::Player0Victory;
    const bool bDraw = !bCampaignResult &&
        Outcome == echoes::sim::MatchOutcome::Draw;
    const FString Result = bCampaignResult
        ? bVictory ? TEXT("MISSION COMPLETE") : TEXT("MISSION FAILED")
        : bVictory ? TEXT("VICTORY") : bDraw ? TEXT("DRAW") : TEXT("DEFEAT");
    const FString LocalFaction = EchoesController->GetLocalFactionLabel();
    const FString OpponentFaction = EchoesController->GetOpponentFactionLabel();
    const FString Headline = bCampaignResult
        ? bShapeOfSilenceResult
            ? bVictory ? TEXT("THE HOLLOW ANSWERS WITH AN ABSENCE")
                       : TEXT("THE MEMORY CONFLUENCE FALLS SILENT")
        : bNamesWithoutBirthsResult
            ? bVictory ? TEXT("THE CENSUS LEAVES WITH ITS NAMES")
                       : TEXT("THE UNBORN RECORD FALLS SILENT")
        : bTermsOfContinuanceResult
            ? bVictory ? TEXT("BOTH WITNESSES LEAVE THE BROKEN ACCORD")
                       : TEXT("THE CONTINUANCE TERMS COLLAPSE")
        : bUnburiedRoadResult
            ? bVictory ? TEXT("THE MISSING ROAD REMEMBERS")
                       : TEXT("THE ROAD CLOSES OVER THE SHARD")
        : bCityReserveResult
            ? bVictory ? TEXT("LUME REACH HOLDS ITS RESERVE")
                       : TEXT("THE CITY GRID FALLS BELOW MARGIN")
        : bSevenAccountsResult
            ? bVictory ? TEXT("THE ROUTE REMEMBERS ORUUN")
                       : TEXT("THE SEVENTH ACCOUNT FALLS SILENT")
            : bVictory ? TEXT("THE LEDGER RETURNS TO LUME REACH")
                       : TEXT("THE ARCHIVE LINE IS LOST")
        : bVictory ? TEXT("THE GLASS SCAR HOLDS")
        : bDraw ? TEXT("NO COMMAND CORE REMAINS")
                : FString::Printf(TEXT("THE %s LINE BROKE"), *LocalFaction);
    const FString Summary = bCampaignResult
        ? bShapeOfSilenceResult
            ? bVictory
                ? FString::Printf(
                      TEXT("The %s memory hollow corresponds with the recovered census absence. This record establishes correspondence, not cause or hidden authorship."),
                      WellChoiceDisplayName(
                          EchoesController->GetCampaignConsequence()))
                : TEXT("Oruun, a memory witness, the Waystone, the local Core, or the listening operation was lost before convergence.")
        : bNamesWithoutBirthsResult
            ? bVictory
                ? FString::Printf(
                      TEXT("Talar extracted the %s census trace after its archive was powered and both civilian proxies reached shelter."),
                      WellChoiceDisplayName(
                          EchoesController->GetCampaignConsequence()))
                : TEXT("Talar, the census archive, a civilian proxy, the local Core, or the operation was lost before evidence extraction.")
        : bTermsOfContinuanceResult
            ? bVictory
                ? FString::Printf(
                      TEXT("Both treaty proxies held, both witness proxies extracted, and the generic hostile pressure remains unresolved under the inherited %s accord."),
                      WellChoiceDisplayName(
                          EchoesController->GetCampaignConsequence()))
                : TEXT("A witness proxy, treaty interface, local Core, or the continuance window was lost before paired extraction.")
        : bUnburiedRoadResult
            ? bVictory
                ? FString::Printf(
                      TEXT("The Waystone holds the %s roadhead, the Listening Spine resolves the echo, and Oruun carries the missing shard."),
                      WellChoiceDisplayName(
                          EchoesController->GetCampaignConsequence()))
                : TEXT("Oruun, the Waystone, the Listening Spine, the local Core, or the unburied route was lost before recovery.")
        : bCityReserveResult
            ? bVictory
                ? FString::Printf(
                      TEXT("Life support, transit, and archive continuity are powered under the inherited %s reserve plan."),
                      WellChoiceDisplayName(
                          EchoesController->GetCampaignConsequence()))
                : TEXT("A district post, the local Core, or the grid line failed before reserve service was restored.")
        : bSevenAccountsResult
            ? bVictory
                ? FString::Printf(
                      TEXT("The Waystone is rooted, Oruun reached the %s account, and the inherited route remains traversable."),
                      WellChoiceDisplayName(
                          EchoesController->GetCampaignConsequence()))
                : TEXT("Oruun, the Waystone, the local Core, or the migration route was lost before the account could be reconciled.")
            : bVictory
                ? FString::Printf(
                  TEXT("Mara Vey recovered Talar Venn's archive, committed the %s protocol, and completed the withdrawal."),
                  WellChoiceDisplayName(
                      EchoesController->GetCampaignConsequence()))
                : TEXT("The archive carrier or withdrawal line was lost before the evacuation could be completed.")
        : bVictory
            ? FString::Printf(
                  TEXT("The %s Command Core is silent. The Future Well remains a consequence, not a prize."),
                  *OpponentFaction)
            : bDraw ? TEXT("Both command structures fell in the same deterministic tick. Neither force controls the crossing.")
                    : FString::Printf(
                          TEXT("Your Command Core has fallen. The %s retain the eastern approach and the initiative."),
                          *OpponentFaction);
    FString CampaignPersistenceLine;
    if (bCampaignResult && bVictory)
    {
        const TCHAR* RecordedChoice = WellChoiceDisplayName(
            EchoesController->GetRecordedCampaignConsequence());
        switch (EchoesController->GetCampaignCommitStatus())
        {
            case EEchoesCampaignCommitStatus::Added:
                CampaignPersistenceLine = bShapeOfSilenceResult
                    ? FString::Printf(
                          TEXT("MISSION 07 RECORDED // %s memory correspondence observed."),
                          RecordedChoice)
                : bNamesWithoutBirthsResult
                    ? FString::Printf(
                          TEXT("MISSION 06 RECORDED // %s census evidence extracted."),
                          RecordedChoice)
                : bTermsOfContinuanceResult
                    ? FString::Printf(
                          TEXT("MISSION 05 RECORDED // %s accord witnesses extracted."),
                          RecordedChoice)
                : bUnburiedRoadResult
                    ? FString::Printf(
                          TEXT("MISSION 04 RECORDED // %s road shard recovered."),
                          RecordedChoice)
                : bCityReserveResult
                    ? FString::Printf(
                          TEXT("MISSION 03 RECORDED // %s reserve grid stabilized."),
                          RecordedChoice)
                : bSevenAccountsResult
                    ? FString::Printf(
                          TEXT("MISSION 02 RECORDED // %s route secured."),
                          RecordedChoice)
                    : FString::Printf(
                          TEXT("LEDGER COMMITTED // %s is fixed for this campaign."),
                          RecordedChoice);
                break;
            case EEchoesCampaignCommitStatus::AlreadyRecorded:
                CampaignPersistenceLine = FString::Printf(
                    TEXT("LEDGER VERIFIED // %s remains recorded."),
                    RecordedChoice);
                break;
            case EEchoesCampaignCommitStatus::ReplayConflict:
                CampaignPersistenceLine = FString::Printf(
                    TEXT("REPLAY ONLY // campaign ledger remains %s."),
                    RecordedChoice);
                break;
            default:
                CampaignPersistenceLine =
                    TEXT("LEDGER NOT SAVED // mission complete; progress unavailable.");
                break;
        }
    }
    const FLinearColor Backdrop =
        bHighContrast ? FLinearColor(0.0f, 0.0f, 0.0f, 1.0f)
                      : FLinearColor(0.005f, 0.012f, 0.026f, 0.98f);
    const FLinearColor Accent = bVictory
        ? FLinearColor(0.25f, 1.0f, 0.66f)
        : bDraw ? FLinearColor(1.0f, 0.82f, 0.2f)
                : FLinearColor(1.0f, 0.32f, 0.16f);
    const FLinearColor Body =
        bHighContrast ? FLinearColor::White : FLinearColor(0.82f, 0.88f, 0.94f);
    const FLinearColor Muted =
        bHighContrast ? FLinearColor(0.9f, 0.9f, 0.9f)
                      : FLinearColor(0.56f, 0.65f, 0.74f);
    UFont* SmallFont = GEngine != nullptr ? GEngine->GetSmallFont() : nullptr;
    const uint64 FinalTick = Bridge != nullptr && Bridge->GetSimulation() != nullptr
                                 ? Bridge->GetSimulation()->CurrentTick()
                                 : 0;

    DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.78f), 0.0f, 0.0f,
             Canvas->ClipX, Canvas->ClipY);
    DrawRect(Backdrop, Left, Top, PanelWidth, PanelHeight);
    DrawLine(Left, Top, Left + PanelWidth, Top, Accent, 4.0f);
    DrawLine(Left, Top + PanelHeight, Left + PanelWidth, Top + PanelHeight, Accent, 4.0f);
    DrawText(Result, Accent, Left + 44.0f, Top + 42.0f * ContentScale,
             SmallFont, 1.9f * TextScale, false);
    DrawText(Headline, Body, Left + 44.0f, Top + 96.0f * ContentScale,
             SmallFont, 1.22f * TextScale, false);
    DrawText(Summary, Body, Left + 44.0f, Top + 148.0f * ContentScale,
             SmallFont, 0.92f * TextScale, false);
    const FString FinalTickLine = bCampaignResult
        ? bShapeOfSilenceResult
            ? FString::Printf(
                  TEXT("MISSION 07 — THE SHAPE OF SILENCE  //  FINAL TICK %llu"),
                  static_cast<unsigned long long>(FinalTick))
        : bNamesWithoutBirthsResult
            ? FString::Printf(
                  TEXT("MISSION 06 — NAMES WITHOUT BIRTHS  //  FINAL TICK %llu"),
                  static_cast<unsigned long long>(FinalTick))
        : bTermsOfContinuanceResult
            ? FString::Printf(
                  TEXT("MISSION 05 — TERMS OF CONTINUANCE  //  FINAL TICK %llu"),
                  static_cast<unsigned long long>(FinalTick))
        : bUnburiedRoadResult
            ? FString::Printf(
                  TEXT("MISSION 04 — THE UNBURIED ROAD  //  FINAL TICK %llu"),
                  static_cast<unsigned long long>(FinalTick))
        : bCityReserveResult
            ? FString::Printf(
                  TEXT("MISSION 03 — A CITY ON RESERVE  //  FINAL TICK %llu"),
                  static_cast<unsigned long long>(FinalTick))
        : bSevenAccountsResult
            ? FString::Printf(
                  TEXT("MISSION 02 — SEVEN ACCOUNTS OF RAIN  //  FINAL TICK %llu"),
                  static_cast<unsigned long long>(FinalTick))
            : FString::Printf(
                  TEXT("MISSION 01 — WHAT THE LEDGER KEEPS  //  FINAL TICK %llu"),
                  static_cast<unsigned long long>(FinalTick))
        : FString::Printf(
              TEXT("OPERATION GLASS SCAR  //  FINAL TICK %llu"),
              static_cast<unsigned long long>(FinalTick));
    DrawText(
        FinalTickLine,
        Muted, Left + 44.0f, Top + 204.0f * ContentScale,
        SmallFont, 0.82f * TextScale, false);
    DrawText(
             bCampaignResult && bVictory
                 ? CampaignPersistenceLine
                 : TEXT("The simulation is stopped. Battlefield commands are locked."),
             Muted, Left + 44.0f, Top + 244.0f * ContentScale,
             SmallFont, 0.82f * TextScale, false);

    DrawRect(Accent, Left + 44.0f, Top + PanelHeight - 82.0f,
             PanelWidth - 88.0f, 46.0f);
    DrawText(
             bCampaignResult
                 ? TEXT("PRESS ENTER OR R TO REPLAY MISSION")
                 : TEXT("PRESS ENTER TO REDEPLOY   //   R TO RESTART"),
             bHighContrast || !bVictory ? FLinearColor::Black
                                         : FLinearColor(0.0f, 0.08f, 0.05f),
             Left + PanelWidth * 0.5f - 176.0f * TextScale,
             Top + PanelHeight - 69.0f,
             SmallFont, 0.92f * TextScale, false);
}

void AEchoesHUD::DrawPauseMenu(
    const AEchoesPlayerController* EchoesController,
    const UEchoesGameUserSettings* Settings)
{
    if (Canvas == nullptr || EchoesController == nullptr ||
        !EchoesController->IsPauseMenuVisible())
    {
        return;
    }

    const bool bHighContrast =
        Settings != nullptr && Settings->IsHighContrastHudEnabled();
    const float HudScale = Settings != nullptr ? Settings->GetHudScale() : 1.0f;
    const float PanelWidth = FMath::Min(
        FMath::Max(620.0f, Canvas->ClipX - 60.0f),
        FMath::Clamp(Canvas->ClipX * 0.50f, 720.0f, 980.0f));
    const float PanelHeight = FMath::Min(
        FMath::Max(520.0f, Canvas->ClipY - 60.0f),
        FMath::Clamp(Canvas->ClipY * 0.64f, 560.0f, 700.0f));
    const float Left = (Canvas->ClipX - PanelWidth) * 0.5f;
    const float Top = (Canvas->ClipY - PanelHeight) * 0.5f;
    const float ContentScale = FMath::Clamp(
        FMath::Min(PanelWidth / 780.0f, PanelHeight / 590.0f),
        0.76f,
        1.2f);
    const float TextScale = HudScale * ContentScale;
    const FLinearColor Backdrop =
        bHighContrast ? FLinearColor(0.0f, 0.0f, 0.0f, 1.0f)
                      : FLinearColor(0.005f, 0.012f, 0.026f, 0.98f);
    const FLinearColor Accent =
        bHighContrast ? FLinearColor(1.0f, 0.9f, 0.1f)
                      : FLinearColor(0.15f, 0.88f, 1.0f);
    const FLinearColor Body =
        bHighContrast ? FLinearColor::White : FLinearColor(0.82f, 0.88f, 0.94f);
    const FLinearColor Muted =
        bHighContrast ? FLinearColor(0.9f, 0.9f, 0.9f)
                      : FLinearColor(0.56f, 0.65f, 0.74f);
    UFont* SmallFont = GEngine != nullptr ? GEngine->GetSmallFont() : nullptr;
    const FString SettingsLineA = FString::Printf(
        TEXT("[U] UI SCALE  %d%%       [I] HIGH CONTRAST  %s"),
        FMath::RoundToInt(HudScale * 100.0f),
        bHighContrast ? TEXT("ON") : TEXT("OFF"));
    const FString SettingsLineB = FString::Printf(
        TEXT("[O] REDUCED MOTION  %s       [/] REDUCED FLASHING  %s"),
        Settings != nullptr && Settings->IsReducedMotionEnabled()
            ? TEXT("ON") : TEXT("OFF"),
        Settings != nullptr && Settings->IsReducedFlashingEnabled()
            ? TEXT("ON") : TEXT("OFF"));
    const FString SettingsLineC = FString::Printf(
        TEXT("[Y] EDGE PAN  %s       [LEFT/RIGHT BRACKET] PAN SPEED  %d%%"),
        Settings == nullptr || Settings->IsEdgePanEnabled()
            ? TEXT("ON") : TEXT("OFF"),
        FMath::RoundToInt(
            (Settings != nullptr ? Settings->GetCameraPanSpeedScale() : 1.0f) *
            100.0f));
    const FString SettingsLineD = FString::Printf(
        TEXT("[COMMA/PERIOD] ZOOM STEP  %d%%"),
        FMath::RoundToInt(
            (Settings != nullptr ? Settings->GetCameraZoomScale() : 1.0f) *
            100.0f));
    const FString SettingsLineE = FString::Printf(
        TEXT("[PAGE DOWN] EFFECTS  %d%%       [SHIFT+PAGE DOWN] REDUCED DYNAMIC RANGE  %s"),
        FMath::RoundToInt(
            (Settings != nullptr ? Settings->GetEffectsVolume() : 1.0f) *
            100.0f),
        Settings != nullptr && Settings->IsReducedDynamicRangeEnabled()
            ? TEXT("ON") : TEXT("OFF"));

    DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.76f), 0.0f, 0.0f,
             Canvas->ClipX, Canvas->ClipY);
    DrawRect(Backdrop, Left, Top, PanelWidth, PanelHeight);
    DrawLine(Left, Top, Left + PanelWidth, Top, Accent, 4.0f);
    DrawLine(Left, Top + PanelHeight, Left + PanelWidth, Top + PanelHeight,
             Accent, 4.0f);
    DrawText(TEXT("FIELD MENU"), Accent, Left + 42.0f,
             Top + 34.0f * ContentScale, SmallFont, 1.65f * TextScale, false);
    DrawText(TEXT("OPERATION GLASS SCAR  //  DETERMINISTIC MATCH PAUSED"),
             Muted, Left + 42.0f, Top + 78.0f * ContentScale,
             SmallFont, 0.88f * TextScale, false);

    DrawText(TEXT("MATCH CONTROL"), Accent, Left + 42.0f,
             Top + 132.0f * ContentScale, SmallFont, 0.92f * TextScale, false);
    DrawText(TEXT("[ENTER / ESCAPE / P]  RESUME OPERATION"), Body,
             Left + 42.0f, Top + 162.0f * ContentScale,
             SmallFont, 1.0f * TextScale, false);
    DrawText(TEXT("[R]  RESTART GLASS SCAR FROM THE DETERMINISTIC BASELINE"), Body,
             Left + 42.0f, Top + 192.0f * ContentScale,
             SmallFont, 0.92f * TextScale, false);

    DrawText(TEXT("ACCESSIBILITY & CAMERA"), Accent, Left + 42.0f,
             Top + 250.0f * ContentScale, SmallFont, 0.92f * TextScale, false);
    DrawText(SettingsLineA, Body, Left + 42.0f, Top + 282.0f * ContentScale,
             SmallFont, 0.88f * TextScale, false);
    DrawText(SettingsLineB, Body, Left + 42.0f, Top + 316.0f * ContentScale,
             SmallFont, 0.88f * TextScale, false);
    DrawText(SettingsLineC, Body, Left + 42.0f, Top + 350.0f * ContentScale,
             SmallFont, 0.82f * TextScale, false);
    DrawText(SettingsLineD, Body, Left + 42.0f, Top + 384.0f * ContentScale,
             SmallFont, 0.88f * TextScale, false);
    DrawText(SettingsLineE, Body, Left + 42.0f, Top + 418.0f * ContentScale,
             SmallFont, 0.78f * TextScale, false);
    DrawText(
        TEXT("Only implemented, behavior-verified options are exposed in this build."),
        Muted, Left + 42.0f, Top + 458.0f * ContentScale,
        SmallFont, 0.78f * TextScale, false);

    DrawRect(Accent, Left + 42.0f, Top + PanelHeight - 76.0f,
             PanelWidth - 84.0f, 42.0f);
    DrawText(TEXT("PRESS ENTER TO RETURN TO THE SCAR"),
             bHighContrast ? FLinearColor::Black
                           : FLinearColor(0.0f, 0.06f, 0.09f),
             Left + PanelWidth * 0.5f - 142.0f * TextScale,
             Top + PanelHeight - 65.0f,
             SmallFont, 0.92f * TextScale, false);
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
    const float PanelWidth = FMath::Min(
        FMath::Max(620.0f, Canvas->ClipX - 60.0f),
        FMath::Clamp(Canvas->ClipX * 0.62f, 880.0f, 1280.0f));
    const float PanelHeight = FMath::Min(
        FMath::Max(500.0f, Canvas->ClipY - 60.0f),
        FMath::Clamp(Canvas->ClipY * 0.72f, 560.0f, 720.0f));
    const float Left = (Canvas->ClipX - PanelWidth) * 0.5f;
    const float Top = (Canvas->ClipY - PanelHeight) * 0.5f;
    const float TextLeft = Left + 42.0f;
    const float ContentScale = FMath::Clamp(
        FMath::Min(PanelWidth / 920.0f, PanelHeight / 590.0f),
        0.68f,
        1.25f);
    const float TextScale = HudScale * ContentScale;
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
    const FString LocalFaction = EchoesController->GetLocalFactionLabel();
    const FString OpponentFaction = EchoesController->GetOpponentFactionLabel();
    const UEchoesSimulationSubsystem* BriefingBridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    const bool bPrologue = BriefingBridge != nullptr &&
        BriefingBridge->GetOperationMode() ==
            EEchoesOperationMode::CampaignPrologue;
    const bool bSevenAccounts = BriefingBridge != nullptr &&
        BriefingBridge->GetOperationMode() ==
            EEchoesOperationMode::CampaignSevenAccounts;
    const bool bCityReserve = BriefingBridge != nullptr &&
        BriefingBridge->GetOperationMode() ==
            EEchoesOperationMode::CampaignCityReserve;
    const bool bUnburiedRoad = BriefingBridge != nullptr &&
        BriefingBridge->GetOperationMode() ==
            EEchoesOperationMode::CampaignUnburiedRoad;
    const bool bTermsOfContinuance = BriefingBridge != nullptr &&
        BriefingBridge->GetOperationMode() ==
            EEchoesOperationMode::CampaignTermsOfContinuance;
    const bool bNamesWithoutBirths = BriefingBridge != nullptr &&
        BriefingBridge->GetOperationMode() ==
            EEchoesOperationMode::CampaignNamesWithoutBirths;
    const bool bShapeOfSilence = BriefingBridge != nullptr &&
        BriefingBridge->GetOperationMode() ==
            EEchoesOperationMode::CampaignShapeOfSilence;
    const FEchoesSevenAccountsRoute SevenAccountsRoute =
        BriefingBridge != nullptr
            ? BriefingBridge->GetSevenAccountsRoute()
            : FEchoesSevenAccountsRoute{};
    const FEchoesCityReserveGrid CityReserveGrid =
        BriefingBridge != nullptr
            ? BriefingBridge->GetCityReserveGrid()
            : FEchoesCityReserveGrid{};
    const FEchoesUnburiedRoadRoute UnburiedRoadRoute =
        BriefingBridge != nullptr
            ? BriefingBridge->GetUnburiedRoadRoute()
            : FEchoesUnburiedRoadRoute{};
    const FEchoesTermsOfContinuancePlan ContinuancePlan =
        BriefingBridge != nullptr
            ? BriefingBridge->GetTermsOfContinuancePlan()
            : FEchoesTermsOfContinuancePlan{};
    const FEchoesNamesWithoutBirthsPlan NamesPlan =
        BriefingBridge != nullptr
            ? BriefingBridge->GetNamesWithoutBirthsPlan()
            : FEchoesNamesWithoutBirthsPlan{};
    const FEchoesShapeOfSilencePlan ShapePlan =
        BriefingBridge != nullptr
            ? BriefingBridge->GetShapeOfSilencePlan()
            : FEchoesShapeOfSilencePlan{};
    const bool bLocalKharuun =
        LocalFaction == TEXT("KHARUUN ASSEMBLIES");
    const FString FactionSystems = bLocalKharuun
        ? TEXT("Kharuun systems: [F3] Waystone  |  [F4/F5] warform  |  [F6] Cairnback cover")
        : TEXT("Meridian systems: [Backslash] Bulwark deployment  |  [=] Relay supply");

    DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.72f), 0.0f, 0.0f, Canvas->ClipX, Canvas->ClipY);
    DrawRect(Backdrop, Left, Top, PanelWidth, PanelHeight);
    DrawLine(Left, Top, Left + PanelWidth, Top, Accent, 3.0f);
    DrawLine(Left, Top + PanelHeight, Left + PanelWidth, Top + PanelHeight, Accent, 3.0f);

    DrawText(TEXT("ECHOES OF THE BROKEN SUN"), Accent, TextLeft, Top + 34.0f * ContentScale,
             SmallFont, 1.55f * TextScale, false);
    const FString BriefingTitle = bPrologue
        ? FString::Printf(
              TEXT("WHAT THE LEDGER KEEPS  //  CAMPAIGN PROLOGUE  //  %s"),
              *LocalFaction)
        : bSevenAccounts
            ? FString::Printf(
                  TEXT("SEVEN ACCOUNTS OF RAIN  //  MISSION 02  //  %s"),
                  *LocalFaction)
        : bCityReserve
            ? FString::Printf(
                  TEXT("A CITY ON RESERVE  //  MISSION 03  //  %s"),
                  *LocalFaction)
        : bUnburiedRoad
            ? FString::Printf(
                  TEXT("THE UNBURIED ROAD  //  MISSION 04  //  %s"),
                  *LocalFaction)
        : bTermsOfContinuance
            ? TEXT("TERMS OF CONTINUANCE  //  MISSION 05  //  MERIDIAN-AUTHORITATIVE PROXIES")
        : bNamesWithoutBirths
            ? TEXT("NAMES WITHOUT BIRTHS  //  MISSION 06  //  MERIDIAN AUTHORITY")
        : bShapeOfSilence
            ? FString::Printf(
                  TEXT("THE SHAPE OF SILENCE  //  MISSION 07  //  %s"),
                  *LocalFaction)
        : FString::Printf(
              TEXT("GLASS SCAR  //  OPERATIONS BRIEF  //  %s"),
              *LocalFaction);
    DrawText(BriefingTitle,
             Muted, TextLeft, Top + 72.0f * ContentScale, SmallFont, 0.90f * TextScale, false);

    DrawText(TEXT("SITUATION"), Accent, TextLeft, Top + 122.0f * ContentScale,
             SmallFont, 0.95f * TextScale, false);
    DrawText(
        bPrologue
            ? TEXT("Lume Reach is evacuating. Talar Venn's archive convoy is displaced at tile 22,18.")
        : bSevenAccounts
            ? FString::Printf(
                  TEXT("The prior %s decision altered the crossing; seven inherited accounts now disagree about the route."),
                  WellChoiceDisplayName(SevenAccountsRoute.PriorChoice))
        : bCityReserve
            ? FString::Printf(
                  TEXT("Lume Reach is below reserve margin. The inherited %s decision fixes which district receives power first."),
                  WellChoiceDisplayName(CityReserveGrid.PriorChoice))
        : bUnburiedRoad
            ? FString::Printf(
                  TEXT("Three consistent records point to the %s, a road absent from every current map."),
                  UnburiedRoadRoute.DisplayName)
        : bTermsOfContinuance
            ? FString::Printf(
                  TEXT("Four consistent records authorize the %s. A ceasefire channel opens across both networks."),
                  ContinuancePlan.DisplayName)
        : bNamesWithoutBirths
            ? FString::Printf(
                  TEXT("Five consistent records expose the %s census trace at tile %d,%d."),
                  NamesPlan.DisplayName,
                  NamesPlan.CensusSite.x.FloorToInt(),
                  NamesPlan.CensusSite.y.FloorToInt())
        : bShapeOfSilence
            ? FString::Printf(
                  TEXT("Six consistent records lead to the %s, a communal-memory hollow aligned with the recovered census absence."),
                  ShapePlan.DisplayName)
            : TEXT("A dormant Future Well lies inside the shattered crossing."),
             Body, TextLeft, Top + 148.0f * ContentScale, SmallFont, 1.0f * TextScale, false);
    DrawText(
        bPrologue
            ? TEXT("Oruun warns the collapse will reach a birthing cavern. Mara Vey must buy time, then withdraw.")
        : bSevenAccounts
            ? FString::Printf(
                  TEXT("Oruun carries the %s testimony. The Waystone must make that account traversable."),
                  SevenAccountsRoute.DisplayName)
        : bCityReserve
            ? FString::Printf(
                  TEXT("Mara Vey is operating under the %s. Three district Aegis posts are intact but disconnected."),
                  CityReserveGrid.DisplayName)
        : bUnburiedRoad
            ? TEXT("Oruun hears a removed memory shard beyond shifting terrain. The Waystone and a Listening Spine must make it recoverable.")
        : bTermsOfContinuance
            ? TEXT("Generic unresolved pressure threatens the operation. Both treaty witnesses use Meridian scout proxies in this prototype.")
        : bNamesWithoutBirths
            ? TEXT("Talar and two civilian workers are explicit Meridian-authoritative proxies. The record establishes branch geometry, not hidden authorship or cause.")
        : bShapeOfSilence
            ? TEXT("Oruun and both witnesses are Kharuun-authoritative. This operation tests correspondence only; it does not establish cause, a Choir identity, or hidden authorship.")
            : FString::Printf(
                  TEXT("%s forces hold the eastern approach. Every protocol changes what survives."),
                  *OpponentFaction),
             Body, TextLeft, Top + 172.0f * ContentScale, SmallFont, 1.0f * TextScale, false);

    DrawText(TEXT("PRIMARY OBJECTIVES"), Accent, TextLeft, Top + 220.0f * ContentScale,
             SmallFont, 0.95f * TextScale, false);
    DrawText(
        bPrologue
            ? TEXT("01  Move Mara Vey's scout carrier to the archive rendezvous at tile 22,18.")
        : bSevenAccounts
            ? FString::Printf(
                  TEXT("01  Uproot, move, and re-root the Waystone at tile %d,%d."),
                  SevenAccountsRoute.WaystoneAnchor.x.FloorToInt(),
                  SevenAccountsRoute.WaystoneAnchor.y.FloorToInt())
        : bCityReserve
            ? FString::Printf(
                  TEXT("01  Use workers and [N] Power Links to energize %s first."),
                  FEchoesCityReserveMissionModel::DistrictDisplayName(
                      CityReserveGrid.Priority))
        : bUnburiedRoad
            ? FString::Printf(
                  TEXT("01  Uproot, move, and re-root the Waystone at the roadhead at tile %d,%d."),
                  UnburiedRoadRoute.Roadhead.x.FloorToInt(),
                  UnburiedRoadRoute.Roadhead.y.FloorToInt())
        : bTermsOfContinuance
            ? FString::Printf(
                  TEXT("01  Use workers and [N] Power Links to synchronize both interfaces at %d,%d and %d,%d."),
                  ContinuancePlan.MeridianRelaySite.x.FloorToInt(),
                  ContinuancePlan.MeridianRelaySite.y.FloorToInt(),
                  ContinuancePlan.KharuunSpineSite.x.FloorToInt(),
                  ContinuancePlan.KharuunSpineSite.y.FloorToInt())
        : bNamesWithoutBirths
            ? FString::Printf(
                  TEXT("01  Bring Talar to census %d,%d, then build the missing [N] Power Link at %d,%d."),
                  NamesPlan.CensusSite.x.FloorToInt(),
                  NamesPlan.CensusSite.y.FloorToInt(),
                  NamesPlan.PowerLinkSite.x.FloorToInt(),
                  NamesPlan.PowerLinkSite.y.FloorToInt())
        : bShapeOfSilence
            ? FString::Printf(
                  TEXT("01  Root the Waystone at %d,%d, then raise a Listening Spine at %d,%d."),
                  ShapePlan.WaystoneAnchor.x.FloorToInt(),
                  ShapePlan.WaystoneAnchor.y.FloorToInt(),
                  ShapePlan.ListeningSpineSite.x.FloorToInt(),
                  ShapePlan.ListeningSpineSite.y.FloorToInt())
            : TEXT("01  Secure and choose a protocol for the central Future Well."),
             Body, TextLeft, Top + 247.0f * ContentScale, SmallFont, 1.0f * TextScale, false);
    DrawText(
        bPrologue
            ? TEXT("02  Hold the archive site, commit a Well protocol, then return the carrier to tile 6,17.")
        : bSevenAccounts
            ? FString::Printf(
                  TEXT("02  After the Waystone roots, bring Oruun's memory-bearer to tile %d,%d."),
                  SevenAccountsRoute.MemoryAccountSite.x.FloorToInt(),
                  SevenAccountsRoute.MemoryAccountSite.y.FloorToInt())
        : bCityReserve
            ? FString::Printf(
                  TEXT("02  Extend the same authoritative grid to %s, then %s."),
                  FEchoesCityReserveMissionModel::DistrictDisplayName(
                      CityReserveGrid.Secondary),
                  FEchoesCityReserveMissionModel::DistrictDisplayName(
                      CityReserveGrid.Final))
        : bUnburiedRoad
            ? FString::Printf(
                  TEXT("02  Build a Listening Spine at %d,%d, then bring Oruun to the shard at %d,%d."),
                  UnburiedRoadRoute.ListeningSpineSite.x.FloorToInt(),
                  UnburiedRoadRoute.ListeningSpineSite.y.FloorToInt(),
                  UnburiedRoadRoute.MemoryShardSite.x.FloorToInt(),
                  UnburiedRoadRoute.MemoryShardSite.y.FloorToInt())
        : bTermsOfContinuance
            ? FString::Printf(
                  TEXT("02  Synchronize by tick %llu, hold through tick %llu, then extract both witness proxies at %d,%d."),
                  static_cast<unsigned long long>(
                      ContinuancePlan.ContinuanceWindowStartTick),
                  static_cast<unsigned long long>(
                      ContinuancePlan.ContinuanceWindowEndTick),
                  ContinuancePlan.WitnessExtractionSite.x.FloorToInt(),
                  ContinuancePlan.WitnessExtractionSite.y.FloorToInt())
        : bNamesWithoutBirths
            ? FString::Printf(
                  TEXT("02  Shelter both civilian proxies at %d,%d, then extract Talar and the evidence at %d,%d."),
                  NamesPlan.CivilianShelterSite.x.FloorToInt(),
                  NamesPlan.CivilianShelterSite.y.FloorToInt(),
                  NamesPlan.EvidenceExtractionSite.x.FloorToInt(),
                  NamesPlan.EvidenceExtractionSite.y.FloorToInt())
        : bShapeOfSilence
            ? FString::Printf(
                  TEXT("02  Position witnesses at %d,%d and %d,%d, then bring Oruun to the confluence at %d,%d."),
                  ShapePlan.FirstWitnessSite.x.FloorToInt(),
                  ShapePlan.FirstWitnessSite.y.FloorToInt(),
                  ShapePlan.SecondWitnessSite.x.FloorToInt(),
                  ShapePlan.SecondWitnessSite.y.FloorToInt(),
                  ShapePlan.ConfluenceSite.x.FloorToInt(),
                  ShapePlan.ConfluenceSite.y.FloorToInt())
            : FString::Printf(
                  TEXT("02  Destroy the %s Command Core without losing your own."),
                  *OpponentFaction),
             Body, TextLeft, Top + 273.0f * ContentScale, SmallFont, 1.0f * TextScale, false);

    DrawText(TEXT("FIELD DOCTRINE"), Accent, TextLeft, Top + 322.0f * ContentScale,
             SmallFont, 0.95f * TextScale, false);
    DrawText(bSevenAccounts
                 ? TEXT("The route is inherited. Mission 01's choice cannot be changed here.")
             : bCityReserve
                 ? TEXT("The reserve priority is inherited from both prior records and cannot be changed here.")
             : bUnburiedRoad
                 ? TEXT("The road is derived from all three prior records and cannot be selected or rewritten here.")
             : bTermsOfContinuance
                 ? TEXT("The accord geometry is inherited from all four prior records and cannot be selected or rewritten here.")
             : bNamesWithoutBirths
                 ? TEXT("The census geometry is inherited from all five prior records; it does not identify an unseen actor or establish why births are absent.")
             : bShapeOfSilence
                 ? TEXT("The listening geometry is inherited from all six prior records; observed correspondence is not evidence of cause or authorship.")
                 : TEXT("Harvest: immediate power  |  Preserve: sustained possibility  |  Reshape: temporary terrain"),
             Body, TextLeft, Top + 349.0f * ContentScale, SmallFont, 0.92f * TextScale, false);
    DrawText(
        bPrologue
            ? TEXT("Mission victory is evacuation. Destroying the opposing Core does not replace withdrawal.")
        : bSevenAccounts
            ? TEXT("Victory is migration and recall. Destroying the opposing Core does not replace either objective.")
        : bCityReserve
            ? TEXT("Victory is three powered districts. Destroying the opposing Core does not stabilize Lume Reach.")
        : bUnburiedRoad
            ? TEXT("Victory is infrastructure-backed recovery. Destroying the opposing Core does not recover the missing shard.")
        : bTermsOfContinuance
            ? TEXT("Victory is synchronized survival and paired proxy extraction. Destroying the opposing Core invalidates the ceasefire evidence.")
        : bNamesWithoutBirths
            ? TEXT("Victory is protected evidence recovery. Any protected loss or either terminal Core outcome invalidates the operation.")
        : bShapeOfSilence
            ? TEXT("Victory is infrastructure-backed paired witnessing and convergence. Any protected loss or terminal Core outcome invalidates the observation.")
            : FactionSystems,
             Body, TextLeft, Top + 375.0f * ContentScale, SmallFont, 0.92f * TextScale, false);

    DrawText(TEXT("ACCESSIBILITY BEFORE DEPLOYMENT"), Accent, TextLeft, Top + 424.0f * ContentScale,
             SmallFont, 0.95f * TextScale, false);
    DrawText(TEXT("[U] UI scale   [I] high contrast   [O] reduced motion   [/] reduced flashing"),
             Body, TextLeft, Top + 451.0f * ContentScale, SmallFont, 0.92f * TextScale, false);

    DrawRect(Accent, Left + 42.0f, Top + PanelHeight - 74.0f,
             PanelWidth - 84.0f, 42.0f);
    DrawText(
        bPrologue
            ? TEXT("F9 CHANGES OPERATION  //  ENTER DEPLOYS MARA VEY")
        : bSevenAccounts
            ? TEXT("F9 CHANGES OPERATION  //  ENTER DEPLOYS ORUUN")
        : bCityReserve
            ? TEXT("F9 CHANGES OPERATION  //  ENTER DEPLOYS MARA VEY")
        : bUnburiedRoad
            ? TEXT("F9 CHANGES OPERATION  //  ENTER DEPLOYS ORUUN")
        : bTermsOfContinuance
            ? TEXT("F9 CHANGES OPERATION  //  ENTER DEPLOYS MERIDIAN TREATY PROXIES")
        : bNamesWithoutBirths
            ? TEXT("F9 CHANGES OPERATION  //  ENTER DEPLOYS TALAR + CIVILIAN PROXIES")
        : bShapeOfSilence
            ? TEXT("F9 CHANGES OPERATION  //  ENTER DEPLOYS ORUUN + MEMORY WITNESSES")
            : TEXT("F9 OPERATION  //  TAB FACTION  //  ENTER DEPLOYS"),
             bHighContrast ? FLinearColor::Black : FLinearColor(0.0f, 0.06f, 0.09f),
             Left + PanelWidth * 0.5f - 180.0f * TextScale,
             Top + PanelHeight - 64.0f,
             SmallFont,
             1.05f * TextScale,
             false);
}

void AEchoesHUD::DrawTacticalMinimap(
    const UEchoesSimulationSubsystem* Bridge,
    const AEchoesPlayerController* EchoesController,
    const UEchoesGameUserSettings* Settings,
    const echoes::sim::PlayerView* PlayerView)
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
            Entity.type == echoes::sim::EntityType::Barracks ||
            Entity.type == echoes::sim::EntityType::UtilityStructure;
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
        if (Entity.aegisPowered &&
            Entity.faction == echoes::sim::Faction::MeridianCompact &&
            Entity.type == echoes::sim::EntityType::UtilityStructure)
        {
            const float PowerRadius = MarkerSize + 2.5f;
            const FLinearColor PowerColor = bHighContrast
                ? FLinearColor::White
                : FLinearColor(1.0f, 0.84f, 0.18f);
            DrawLine(X, Y - PowerRadius, X + PowerRadius, Y, PowerColor, 1.5f);
            DrawLine(X + PowerRadius, Y, X, Y + PowerRadius, PowerColor, 1.5f);
            DrawLine(X, Y + PowerRadius, X - PowerRadius, Y, PowerColor, 1.5f);
            DrawLine(X - PowerRadius, Y, X, Y - PowerRadius, PowerColor, 1.5f);
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

    int32 VibrationMarkerCount = 0;
    if (PlayerView != nullptr)
    {
        const FLinearColor SignatureColor = bHighContrast
            ? FLinearColor(1.0f, 0.9f, 0.1f)
            : FLinearColor(1.0f, 0.48f, 0.12f);
        for (const echoes::sim::VibrationSignature& Signature :
             PlayerView->VibrationSignatures())
        {
            ++VibrationMarkerCount;
            const float X = Left +
                FMath::Clamp(
                    static_cast<float>(Signature.approximatePosition.x.Raw()) /
                        static_cast<float>(echoes::sim::kFixedScale * MapWidth),
                    0.0f,
                    1.0f) * Size;
            const float Y = Top +
                FMath::Clamp(
                    static_cast<float>(Signature.approximatePosition.y.Raw()) /
                        static_cast<float>(echoes::sim::kFixedScale * MapHeight),
                    0.0f,
                    1.0f) * Size;
            constexpr float Radius = 4.5f;
            DrawLine(X, Y - Radius, X + Radius, Y, SignatureColor, 1.5f);
            DrawLine(X + Radius, Y, X, Y + Radius, SignatureColor, 1.5f);
            DrawLine(X, Y + Radius, X - Radius, Y, SignatureColor, 1.5f);
            DrawLine(X - Radius, Y, X, Y - Radius, SignatureColor, 1.5f);
            DrawLine(X - Radius - 2.0f, Y, X + Radius + 2.0f, Y,
                     SignatureColor, 1.0f);
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

    if (Bridge->GetOperationMode() == EEchoesOperationMode::CampaignPrologue ||
        Bridge->GetOperationMode() ==
            EEchoesOperationMode::CampaignSevenAccounts ||
        Bridge->GetOperationMode() ==
            EEchoesOperationMode::CampaignCityReserve ||
        Bridge->GetOperationMode() ==
            EEchoesOperationMode::CampaignUnburiedRoad ||
        Bridge->GetOperationMode() ==
            EEchoesOperationMode::CampaignTermsOfContinuance ||
        Bridge->GetOperationMode() ==
            EEchoesOperationMode::CampaignNamesWithoutBirths ||
        Bridge->GetOperationMode() ==
            EEchoesOperationMode::CampaignShapeOfSilence)
    {
        const FEchoesObjectiveSnapshot Objective =
            Bridge->GetLocalObjectiveSnapshot();
        const auto DrawMissionSite = [this, Left, Top, Size, MapWidth, MapHeight](
                                         const echoes::sim::Vec2& Site,
                                         const TCHAR* Label,
                                         const FLinearColor& Color)
        {
            const float X = Left +
                FMath::Clamp(
                    static_cast<float>(Site.x.Raw()) /
                        static_cast<float>(echoes::sim::kFixedScale * MapWidth),
                    0.0f,
                    1.0f) * Size;
            const float Y = Top +
                FMath::Clamp(
                    static_cast<float>(Site.y.Raw()) /
                        static_cast<float>(echoes::sim::kFixedScale * MapHeight),
                    0.0f,
                    1.0f) * Size;
            constexpr float Radius = 7.0f;
            DrawLine(X, Y - Radius, X + Radius, Y, Color, 2.0f);
            DrawLine(X + Radius, Y, X, Y + Radius, Color, 2.0f);
            DrawLine(X, Y + Radius, X - Radius, Y, Color, 2.0f);
            DrawLine(X - Radius, Y, X, Y - Radius, Color, 2.0f);
            DrawText(Label, Color, X + 9.0f, Y - 8.0f,
                     GEngine != nullptr ? GEngine->GetSmallFont() : nullptr,
                     0.68f, false);
        };
        if (Bridge->GetOperationMode() ==
            EEchoesOperationMode::CampaignPrologue)
        {
            const FLinearColor ArchiveColor =
                Objective.ProloguePhase == EEchoesProloguePhase::RecoverArchive ||
                Objective.ProloguePhase == EEchoesProloguePhase::DecideFutureWell
                    ? Border
                    : FLinearColor(0.25f, 1.0f, 0.66f);
            const FLinearColor EvacColor =
                Objective.ProloguePhase == EEchoesProloguePhase::Withdraw ||
                Objective.ProloguePhase == EEchoesProloguePhase::Complete
                    ? Border
                    : FLinearColor(0.48f, 0.55f, 0.62f);
            DrawMissionSite(
                UEchoesSimulationSubsystem::GetArchiveRecoverySite(),
                TEXT("A"),
                ArchiveColor);
            DrawMissionSite(
                UEchoesSimulationSubsystem::GetEvacuationSite(),
                TEXT("E"),
                EvacColor);
        }
        else if (Bridge->GetOperationMode() ==
                 EEchoesOperationMode::CampaignSevenAccounts)
        {
            const FEchoesSevenAccountsRoute Route =
                Bridge->GetSevenAccountsRoute();
            DrawMissionSite(
                Route.WaystoneAnchor,
                TEXT("W"),
                Objective.bWaystoneRootedAtAnchor
                    ? FLinearColor(0.25f, 1.0f, 0.66f)
                    : Border);
            DrawMissionSite(
                Route.MemoryAccountSite,
                TEXT("M"),
                Objective.bWaystoneRootedAtAnchor
                    ? Border
                    : FLinearColor(0.48f, 0.55f, 0.62f));
        }
        else if (Bridge->GetOperationMode() ==
                 EEchoesOperationMode::CampaignCityReserve)
        {
            DrawMissionSite(
                FEchoesCityReserveMissionModel::SiteForDistrict(
                    EEchoesCityDistrict::LifeSupport),
                TEXT("L"),
                Objective.bLifeSupportPowered
                    ? FLinearColor(0.25f, 1.0f, 0.66f)
                    : Border);
            DrawMissionSite(
                FEchoesCityReserveMissionModel::SiteForDistrict(
                    EEchoesCityDistrict::Transit),
                TEXT("T"),
                Objective.bTransitPowered
                    ? FLinearColor(0.25f, 1.0f, 0.66f)
                    : Border);
            DrawMissionSite(
                FEchoesCityReserveMissionModel::SiteForDistrict(
                    EEchoesCityDistrict::Archive),
                TEXT("A"),
                Objective.bArchivePowered
                    ? FLinearColor(0.25f, 1.0f, 0.66f)
                    : Border);
        }
        else if (Bridge->GetOperationMode() ==
                 EEchoesOperationMode::CampaignUnburiedRoad)
        {
            const FEchoesUnburiedRoadRoute Route =
                Bridge->GetUnburiedRoadRoute();
            DrawMissionSite(
                Route.Roadhead,
                TEXT("W"),
                Objective.bWaystoneRootedAtRoadhead
                    ? FLinearColor(0.25f, 1.0f, 0.66f)
                    : Border);
            DrawMissionSite(
                Route.ListeningSpineSite,
                TEXT("L"),
                Objective.bListeningSpineComplete
                    ? FLinearColor(0.25f, 1.0f, 0.66f)
                    : Objective.bWaystoneRootedAtRoadhead
                        ? Border
                        : FLinearColor(0.48f, 0.55f, 0.62f));
            DrawMissionSite(
                Route.MemoryShardSite,
                TEXT("S"),
                Objective.bMemoryBearerAtShard
                    ? FLinearColor(0.25f, 1.0f, 0.66f)
                    : Objective.bListeningSpineComplete
                        ? Border
                        : FLinearColor(0.48f, 0.55f, 0.62f));
        }
        else if (Bridge->GetOperationMode() ==
                 EEchoesOperationMode::CampaignTermsOfContinuance)
        {
            const FEchoesTermsOfContinuancePlan Plan =
                Bridge->GetTermsOfContinuancePlan();
            DrawMissionSite(
                Plan.MeridianRelaySite,
                TEXT("A"),
                Objective.bMeridianRelaySynchronized
                    ? FLinearColor(0.25f, 1.0f, 0.66f)
                    : Border);
            DrawMissionSite(
                Plan.KharuunSpineSite,
                TEXT("K"),
                Objective.bKharuunSpineSynchronized
                    ? FLinearColor(0.25f, 1.0f, 0.66f)
                    : Border);
            DrawMissionSite(
                Plan.WitnessExtractionSite,
                TEXT("E"),
                Objective.bMeridianWitnessExtracted &&
                        Objective.bKharuunWitnessExtracted
                    ? FLinearColor(0.25f, 1.0f, 0.66f)
                    : Objective.bContinuanceWindowHeld
                        ? Border
                        : FLinearColor(0.48f, 0.55f, 0.62f));
        }
        else if (Bridge->GetOperationMode() ==
                 EEchoesOperationMode::CampaignNamesWithoutBirths)
        {
            const FEchoesNamesWithoutBirthsPlan Plan =
                Bridge->GetNamesWithoutBirthsPlan();
            DrawMissionSite(
                Plan.CensusSite,
                TEXT("C"),
                Objective.bCensusEvidenceLocated
                    ? FLinearColor(0.25f, 1.0f, 0.66f)
                    : Border);
            DrawMissionSite(
                Plan.PowerLinkSite,
                TEXT("P"),
                Objective.bCensusArchivePowered
                    ? FLinearColor(0.25f, 1.0f, 0.66f)
                    : Objective.bCensusEvidenceLocated
                        ? Border
                        : FLinearColor(0.48f, 0.55f, 0.62f));
            DrawMissionSite(
                Plan.CivilianShelterSite,
                TEXT("S"),
                Objective.bFirstCivilianSheltered &&
                        Objective.bSecondCivilianSheltered
                    ? FLinearColor(0.25f, 1.0f, 0.66f)
                    : Objective.bCensusArchivePowered
                        ? Border
                        : FLinearColor(0.48f, 0.55f, 0.62f));
            DrawMissionSite(
                Plan.EvidenceExtractionSite,
                TEXT("E"),
                Objective.bTalarAtEvidenceExtraction
                    ? FLinearColor(0.25f, 1.0f, 0.66f)
                    : Objective.bFirstCivilianSheltered &&
                            Objective.bSecondCivilianSheltered
                        ? Border
                        : FLinearColor(0.48f, 0.55f, 0.62f));
        }
        else
        {
            const FEchoesShapeOfSilencePlan Plan =
                Bridge->GetShapeOfSilencePlan();
            DrawMissionSite(
                Plan.WaystoneAnchor,
                TEXT("W"),
                Objective.bShapeWaystoneRooted
                    ? FLinearColor(0.25f, 1.0f, 0.66f)
                    : Border);
            DrawMissionSite(
                Plan.ListeningSpineSite,
                TEXT("L"),
                Objective.bShapeListeningSpineRaised
                    ? FLinearColor(0.25f, 1.0f, 0.66f)
                    : Objective.bShapeWaystoneRooted
                        ? Border
                        : FLinearColor(0.48f, 0.55f, 0.62f));
            DrawMissionSite(
                Plan.FirstWitnessSite,
                TEXT("1"),
                Objective.bFirstMemoryWitnessPositioned
                    ? FLinearColor(0.25f, 1.0f, 0.66f)
                    : Objective.bShapeListeningSpineRaised
                        ? Border
                        : FLinearColor(0.48f, 0.55f, 0.62f));
            DrawMissionSite(
                Plan.SecondWitnessSite,
                TEXT("2"),
                Objective.bSecondMemoryWitnessPositioned
                    ? FLinearColor(0.25f, 1.0f, 0.66f)
                    : Objective.bShapeListeningSpineRaised
                        ? Border
                        : FLinearColor(0.48f, 0.55f, 0.62f));
            DrawMissionSite(
                Plan.ConfluenceSite,
                TEXT("O"),
                Objective.bOruunAtConfluence
                    ? FLinearColor(0.25f, 1.0f, 0.66f)
                    : Objective.bFirstMemoryWitnessPositioned &&
                            Objective.bSecondMemoryWitnessPositioned
                        ? Border
                        : FLinearColor(0.48f, 0.55f, 0.62f));
        }
    }

    DrawLine(Left, Top, Left + Size, Top, Border, 2.0f);
    DrawLine(Left + Size, Top, Left + Size, Top + Size, Border, 2.0f);
    DrawLine(Left + Size, Top + Size, Left, Top + Size, Border, 2.0f);
    DrawLine(Left, Top + Size, Left, Top, Border, 2.0f);
    DrawText(
        Bridge->GetOperationMode() == EEchoesOperationMode::CampaignPrologue
            ? TEXT("MISSION NAV  |  ARCHIVE + EVAC")
        : Bridge->GetOperationMode() ==
                EEchoesOperationMode::CampaignSevenAccounts
            ? TEXT("MISSION NAV  |  WAYSTONE + MEMORY")
        : Bridge->GetOperationMode() ==
                EEchoesOperationMode::CampaignCityReserve
            ? TEXT("MISSION NAV  |  LIFE + TRANSIT + ARCHIVE")
        : Bridge->GetOperationMode() ==
                EEchoesOperationMode::CampaignUnburiedRoad
            ? TEXT("MISSION NAV  |  WAYSTONE + SPINE + SHARD")
        : Bridge->GetOperationMode() ==
                EEchoesOperationMode::CampaignTermsOfContinuance
            ? TEXT("MISSION NAV  |  AEGIS + TREATY + EXTRACTION")
        : Bridge->GetOperationMode() ==
                EEchoesOperationMode::CampaignNamesWithoutBirths
            ? TEXT("MISSION NAV  |  CENSUS + SHELTER + EVIDENCE")
        : Bridge->GetOperationMode() ==
                EEchoesOperationMode::CampaignShapeOfSilence
            ? TEXT("MISSION NAV  |  WAYSTONE + WITNESSES + ORUUN")
            : TEXT("TACTICAL OVERVIEW  |  fog-respecting"),
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
            TEXT("[ECHOES_MINIMAP_READY] fogRespecting=true terrainAware=true nonColorTeams=true visibleMarkers=%d vibrationMarkers=%d"),
            VisibleMarkerCount,
            VibrationMarkerCount);
    }
}

void AEchoesHUD::DrawVibrationSignatures(
    const UEchoesSimulationSubsystem* Bridge,
    const UEchoesGameUserSettings* Settings,
    const echoes::sim::PlayerView* PlayerView)
{
    if (Canvas == nullptr || Bridge == nullptr || PlayerView == nullptr ||
        PlayerView->VibrationSignatures().empty())
    {
        return;
    }
    APlayerController* Controller = GetOwningPlayerController();
    if (Controller == nullptr)
    {
        return;
    }
    const bool bHighContrast =
        Settings != nullptr && Settings->IsHighContrastHudEnabled();
    const float HudScale = Settings != nullptr ? Settings->GetHudScale() : 1.0f;
    const FLinearColor SignatureColor = bHighContrast
        ? FLinearColor(1.0f, 0.9f, 0.1f)
        : FLinearColor(1.0f, 0.48f, 0.12f);
    const float AlertWidth = FMath::Clamp(460.0f * HudScale, 390.0f, 560.0f);
    const float MainPanelRight =
        18.0f + FMath::Min(
            920.0f * HudScale,
            FMath::Max(320.0f, Canvas->ClipX - 36.0f));
    float AlertLeft = Canvas->ClipX - AlertWidth - 20.0f;
    float AlertTop = 208.0f;
    if (AlertLeft < MainPanelRight + 20.0f)
    {
        AlertLeft = 18.0f;
        AlertTop = 490.0f;
    }
    if (AlertTop + 64.0f <= Canvas->ClipY - 24.0f)
    {
        const FLinearColor AlertBackdrop = bHighContrast
            ? FLinearColor(0.0f, 0.0f, 0.0f, 0.98f)
            : FLinearColor(0.025f, 0.012f, 0.008f, 0.94f);
        DrawRect(AlertBackdrop, AlertLeft, AlertTop, AlertWidth, 64.0f);
        DrawLine(
            AlertLeft,
            AlertTop,
            AlertLeft + AlertWidth,
            AlertTop,
            SignatureColor,
            2.0f);
        DrawText(
            FString::Printf(
                TEXT("VIBRATION CONTACTS  %02d  //  MOVEMENT DETECTED"),
                static_cast<int32>(PlayerView->VibrationSignatures().size())),
            SignatureColor,
            AlertLeft + 18.0f,
            AlertTop + 12.0f,
            GEngine != nullptr ? GEngine->GetSmallFont() : nullptr,
            0.82f * HudScale,
            false);
        DrawText(
            TEXT("APPROXIMATE LOCATION  //  ANONYMOUS  //  NO DIRECT TARGET"),
            bHighContrast ? FLinearColor::White
                          : FLinearColor(0.82f, 0.86f, 0.90f),
            AlertLeft + 18.0f,
            AlertTop + 36.0f,
            GEngine != nullptr ? GEngine->GetSmallFont() : nullptr,
            0.72f * HudScale,
            false);
    }
    int32 Presented = 0;
    int32 EdgeIndicators = 0;
    const FVector2D ViewportSize(Canvas->ClipX, Canvas->ClipY);
    for (const echoes::sim::VibrationSignature& Signature :
         PlayerView->VibrationSignatures())
    {
        FVector WorldPosition = Bridge->SimToWorld(Signature.approximatePosition);
        WorldPosition.Z = 90.0f;
        FVector2D ScreenPosition;
        if (!Controller->ProjectWorldLocationToScreen(
                WorldPosition, ScreenPosition, true))
        {
            ScreenPosition = FallbackContactProjection(
                Controller, WorldPosition, ViewportSize);
        }
        const FEchoesContactIndicatorPlacement Placement =
            FEchoesContactIndicatorLayout::Calculate(
                ScreenPosition, ViewportSize, HudScale);
        ++Presented;
        EdgeIndicators += Placement.bClampedToEdge ? 1 : 0;
        const float Radius = 12.0f * HudScale;
        DrawLine(Placement.MarkerPosition.X, Placement.MarkerPosition.Y - Radius,
                 Placement.MarkerPosition.X + Radius, Placement.MarkerPosition.Y,
                 SignatureColor, 2.0f);
        DrawLine(Placement.MarkerPosition.X + Radius, Placement.MarkerPosition.Y,
                 Placement.MarkerPosition.X, Placement.MarkerPosition.Y + Radius,
                 SignatureColor, 2.0f);
        DrawLine(Placement.MarkerPosition.X, Placement.MarkerPosition.Y + Radius,
                 Placement.MarkerPosition.X - Radius, Placement.MarkerPosition.Y,
                 SignatureColor, 2.0f);
        DrawLine(Placement.MarkerPosition.X - Radius, Placement.MarkerPosition.Y,
                 Placement.MarkerPosition.X, Placement.MarkerPosition.Y - Radius,
                 SignatureColor, 2.0f);
        DrawLine(Placement.MarkerPosition.X - Radius * 1.5f,
                 Placement.MarkerPosition.Y,
                 Placement.MarkerPosition.X + Radius * 1.5f,
                 Placement.MarkerPosition.Y,
                 SignatureColor, 1.0f);
        if (Placement.bClampedToEdge)
        {
            FVector2D EdgeDirection =
                (ScreenPosition - Placement.MarkerPosition).GetSafeNormal();
            if (EdgeDirection.IsNearlyZero())
            {
                EdgeDirection = FVector2D(0.0f, -1.0f);
            }
            const FVector2D Perpendicular(-EdgeDirection.Y, EdgeDirection.X);
            const FVector2D Tip =
                Placement.MarkerPosition + EdgeDirection * Radius * 1.65f;
            const FVector2D LeftTail =
                Placement.MarkerPosition - Perpendicular * Radius * 0.55f;
            const FVector2D RightTail =
                Placement.MarkerPosition + Perpendicular * Radius * 0.55f;
            DrawLine(
                LeftTail.X,
                LeftTail.Y,
                Tip.X,
                Tip.Y,
                SignatureColor,
                2.0f);
            DrawLine(
                RightTail.X,
                RightTail.Y,
                Tip.X,
                Tip.Y,
                SignatureColor,
                2.0f);
        }
        const FLinearColor LabelBackdrop = bHighContrast
            ? FLinearColor(0.0f, 0.0f, 0.0f, 0.98f)
            : FLinearColor(0.025f, 0.012f, 0.008f, 0.90f);
        const float LabelPanelLeft =
            Placement.LabelPosition.X - 8.0f * HudScale;
        const float LabelPanelTop =
            Placement.LabelPosition.Y - 6.0f * HudScale;
        const float LabelPanelWidth = 270.0f * HudScale;
        const float LabelPanelHeight = 44.0f * HudScale;
        DrawRect(
            LabelBackdrop,
            LabelPanelLeft,
            LabelPanelTop,
            LabelPanelWidth,
            LabelPanelHeight);
        DrawLine(
            LabelPanelLeft,
            LabelPanelTop,
            LabelPanelLeft + LabelPanelWidth,
            LabelPanelTop,
            SignatureColor,
            1.5f);
        DrawLine(
            LabelPanelLeft,
            LabelPanelTop,
            LabelPanelLeft,
            LabelPanelTop + LabelPanelHeight,
            SignatureColor,
            2.0f);
        DrawText(
            FEchoesContactIndicatorLayout::BuildPrimaryLabel(
                Presented, Placement.bClampedToEdge),
            SignatureColor,
            Placement.LabelPosition.X,
            Placement.LabelPosition.Y,
            GEngine != nullptr ? GEngine->GetSmallFont() : nullptr,
            0.74f * HudScale,
            false);
        DrawText(
            TEXT("APPROXIMATE // NO UNIT ID"),
            bHighContrast ? FLinearColor::White
                          : FLinearColor(0.86f, 0.72f, 0.64f),
            Placement.LabelPosition.X,
            Placement.LabelPosition.Y + 17.0f * HudScale,
            GEngine != nullptr ? GEngine->GetSmallFont() : nullptr,
            0.62f * HudScale,
            false);
    }
    if (Presented > 0 && !bLoggedVibrationPresentationReady)
    {
        bLoggedVibrationPresentationReady = true;
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_VIBRATION_PRESENTATION_READY] contacts=%d edgeIndicators=%d anonymous=true quantized=true nonColor=true directTarget=false alertLegend=true"),
            Presented,
            EdgeIndicators);
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
