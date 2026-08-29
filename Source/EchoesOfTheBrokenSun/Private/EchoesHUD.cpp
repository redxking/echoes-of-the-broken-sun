#include "EchoesHUD.h"

#include "EchoesContentSubsystem.h"
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
                const TCHAR* Name =
                    Player->activeResearch ==
                            echoes::sim::ResearchType::MeridianPrismaticTargeting
                        ? TEXT("Prismatic Targeting")
                        : Player->activeResearch ==
                                  echoes::sim::ResearchType::MeridianHorizonLattice
                              ? TEXT("Horizon Lattice")
                              : Player->activeResearch ==
                                        echoes::sim::ResearchType::KharuunEchoCartography
                                    ? TEXT("Echo Cartography")
                                    : TEXT("Ancestral Edge");
                ResearchLine = FString::Printf(
                    TEXT("RESEARCH  %s  %d%%     production suspended"),
                    Name,
                    Percent);
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
    const FString FactionControlLine =
        Bridge != nullptr &&
                Bridge->GetLocalFaction() ==
                    echoes::sim::Faction::KharuunAssemblies
            ? TEXT("[F] Attack-move  [T] Patrol  [H] Hold  [J] Guard  [X] Stop  [-] Waystone  Shift+[ / ] warform  Shift+; cover")
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
        TEXT("[1-0] Recall group    [G then 1-0] Assign group    [F2] Technologies    [P] Pause    [R] Restart"),
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
        TEXT("Research occupies the selected production structure. Destruction interrupts the project without refund."),
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
                      : FLinearColor(0.025f, 0.055f, 0.09f, 0.94f);
        DrawRect(RowColor, Row.Min.X, Row.Min.Y,
                 Row.GetSize().X, Row.GetSize().Y);
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
        TEXT("LMB: choose tier    Enter / Shift+R: next available    F2 / Escape / P: close"),
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
    const FString LocalFaction = EchoesController->GetLocalFactionLabel();
    const FString OpponentFaction = EchoesController->GetOpponentFactionLabel();

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
    DrawText(TEXT("GLASS SCAR"), Body,
             Left + 48.0f, Top + 202.0f * ContentScale,
             SmallFont, 1.42f * TextScale, false);
    DrawText(FString::Printf(
                 TEXT("SINGLE-PLAYER  //  %s  //  FUTURE WELL CONTEST"),
                 *LocalFaction),
             Muted, Left + 48.0f, Top + 246.0f * ContentScale,
             SmallFont, 0.86f * TextScale, false);
    DrawText(FString::Printf(
                 TEXT("[TAB] CHANGE FACTION  //  OPPOSITION: ADAPTIVE %s  //  STANDARD"),
                 *OpponentFaction),
             Accent, Left + 48.0f, Top + 272.0f * ContentScale,
             SmallFont, 0.80f * TextScale, false);
    DrawText(TEXT("Cross the shattered approaches, choose what the Well becomes,"),
             Body, Left + 48.0f, Top + 310.0f * ContentScale,
             SmallFont, 0.96f * TextScale, false);
    DrawText(TEXT("and break the opposing Command Core before your own line collapses."),
             Body, Left + 48.0f, Top + 338.0f * ContentScale,
             SmallFont, 0.96f * TextScale, false);

    DrawText(TEXT("ACCESSIBILITY BEFORE DEPLOYMENT"), Accent,
             Left + 48.0f, Top + 404.0f * ContentScale,
             SmallFont, 0.90f * TextScale, false);
    DrawText(AccessLine, Body,
             Left + 48.0f, Top + 438.0f * ContentScale,
             SmallFont, 0.80f * TextScale, false);

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
    const bool bVictory = Outcome == echoes::sim::MatchOutcome::Player0Victory;
    const bool bDraw = Outcome == echoes::sim::MatchOutcome::Draw;
    const FString Result = bVictory ? TEXT("VICTORY") : bDraw ? TEXT("DRAW") : TEXT("DEFEAT");
    const FString LocalFaction = EchoesController->GetLocalFactionLabel();
    const FString OpponentFaction = EchoesController->GetOpponentFactionLabel();
    const FString Headline = bVictory
        ? TEXT("THE GLASS SCAR HOLDS")
        : bDraw ? TEXT("NO COMMAND CORE REMAINS")
                : FString::Printf(TEXT("THE %s LINE BROKE"), *LocalFaction);
    const FString Summary = bVictory
        ? FString::Printf(
              TEXT("The %s Command Core is silent. The Future Well remains a consequence, not a prize."),
              *OpponentFaction)
        : bDraw ? TEXT("Both command structures fell in the same deterministic tick. Neither force controls the crossing.")
                : FString::Printf(
                      TEXT("Your Command Core has fallen. The %s retain the eastern approach and the initiative."),
                      *OpponentFaction);
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
    DrawText(
        FString::Printf(TEXT("OPERATION GLASS SCAR  //  FINAL TICK %llu"),
                        static_cast<unsigned long long>(FinalTick)),
        Muted, Left + 44.0f, Top + 204.0f * ContentScale,
        SmallFont, 0.82f * TextScale, false);
    DrawText(TEXT("The simulation is stopped. Battlefield commands are locked."),
             Muted, Left + 44.0f, Top + 244.0f * ContentScale,
             SmallFont, 0.82f * TextScale, false);

    DrawRect(Accent, Left + 44.0f, Top + PanelHeight - 82.0f,
             PanelWidth - 88.0f, 46.0f);
    DrawText(TEXT("PRESS ENTER TO REDEPLOY   //   R TO RESTART"),
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
    DrawText(
        TEXT("Only implemented, behavior-verified options are exposed in this build."),
        Muted, Left + 42.0f, Top + 430.0f * ContentScale,
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
    const bool bLocalKharuun =
        LocalFaction == TEXT("KHARUUN ASSEMBLIES");
    const FString FactionSystems = bLocalKharuun
        ? TEXT("Kharuun systems: [-] Waystone  |  Shift+[ / ] warform  |  Shift+; Cairnback cover")
        : TEXT("Meridian systems: [Backslash] Bulwark deployment  |  [=] Relay supply");

    DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.72f), 0.0f, 0.0f, Canvas->ClipX, Canvas->ClipY);
    DrawRect(Backdrop, Left, Top, PanelWidth, PanelHeight);
    DrawLine(Left, Top, Left + PanelWidth, Top, Accent, 3.0f);
    DrawLine(Left, Top + PanelHeight, Left + PanelWidth, Top + PanelHeight, Accent, 3.0f);

    DrawText(TEXT("ECHOES OF THE BROKEN SUN"), Accent, TextLeft, Top + 34.0f * ContentScale,
             SmallFont, 1.55f * TextScale, false);
    DrawText(FString::Printf(
                 TEXT("GLASS SCAR  //  OPERATIONS BRIEF  //  %s"),
                 *LocalFaction),
             Muted, TextLeft, Top + 72.0f * ContentScale, SmallFont, 0.90f * TextScale, false);

    DrawText(TEXT("SITUATION"), Accent, TextLeft, Top + 122.0f * ContentScale,
             SmallFont, 0.95f * TextScale, false);
    DrawText(TEXT("A dormant Future Well lies inside the shattered crossing."),
             Body, TextLeft, Top + 148.0f * ContentScale, SmallFont, 1.0f * TextScale, false);
    DrawText(FString::Printf(
                 TEXT("%s forces hold the eastern approach. Every protocol changes what survives."),
                 *OpponentFaction),
             Body, TextLeft, Top + 172.0f * ContentScale, SmallFont, 1.0f * TextScale, false);

    DrawText(TEXT("PRIMARY OBJECTIVES"), Accent, TextLeft, Top + 220.0f * ContentScale,
             SmallFont, 0.95f * TextScale, false);
    DrawText(TEXT("01  Secure and choose a protocol for the central Future Well."),
             Body, TextLeft, Top + 247.0f * ContentScale, SmallFont, 1.0f * TextScale, false);
    DrawText(FString::Printf(
                 TEXT("02  Destroy the %s Command Core without losing your own."),
                 *OpponentFaction),
             Body, TextLeft, Top + 273.0f * ContentScale, SmallFont, 1.0f * TextScale, false);

    DrawText(TEXT("FIELD DOCTRINE"), Accent, TextLeft, Top + 322.0f * ContentScale,
             SmallFont, 0.95f * TextScale, false);
    DrawText(TEXT("Harvest: immediate power  |  Preserve: sustained possibility  |  Reshape: temporary terrain"),
             Body, TextLeft, Top + 349.0f * ContentScale, SmallFont, 0.92f * TextScale, false);
    DrawText(FactionSystems,
             Body, TextLeft, Top + 375.0f * ContentScale, SmallFont, 0.92f * TextScale, false);

    DrawText(TEXT("ACCESSIBILITY BEFORE DEPLOYMENT"), Accent, TextLeft, Top + 424.0f * ContentScale,
             SmallFont, 0.95f * TextScale, false);
    DrawText(TEXT("[U] UI scale   [I] high contrast   [O] reduced motion   [/] reduced flashing"),
             Body, TextLeft, Top + 451.0f * ContentScale, SmallFont, 0.92f * TextScale, false);

    DrawRect(Accent, Left + 42.0f, Top + PanelHeight - 74.0f,
             PanelWidth - 84.0f, 42.0f);
    DrawText(TEXT("TAB CHANGES FACTION  //  ENTER DEPLOYS"),
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
    int32 Presented = 0;
    for (const echoes::sim::VibrationSignature& Signature :
         PlayerView->VibrationSignatures())
    {
        FVector WorldPosition = Bridge->SimToWorld(Signature.approximatePosition);
        WorldPosition.Z = 90.0f;
        FVector2D ScreenPosition;
        if (!Controller->ProjectWorldLocationToScreen(
                WorldPosition, ScreenPosition, true) ||
            ScreenPosition.X < 16.0f || ScreenPosition.Y < 280.0f ||
            ScreenPosition.X > Canvas->ClipX - 16.0f ||
            ScreenPosition.Y > Canvas->ClipY - 90.0f)
        {
            continue;
        }
        ++Presented;
        const float Radius = 12.0f * HudScale;
        DrawLine(ScreenPosition.X, ScreenPosition.Y - Radius,
                 ScreenPosition.X + Radius, ScreenPosition.Y,
                 SignatureColor, 2.0f);
        DrawLine(ScreenPosition.X + Radius, ScreenPosition.Y,
                 ScreenPosition.X, ScreenPosition.Y + Radius,
                 SignatureColor, 2.0f);
        DrawLine(ScreenPosition.X, ScreenPosition.Y + Radius,
                 ScreenPosition.X - Radius, ScreenPosition.Y,
                 SignatureColor, 2.0f);
        DrawLine(ScreenPosition.X - Radius, ScreenPosition.Y,
                 ScreenPosition.X, ScreenPosition.Y - Radius,
                 SignatureColor, 2.0f);
        DrawLine(ScreenPosition.X - Radius * 1.5f, ScreenPosition.Y,
                 ScreenPosition.X + Radius * 1.5f, ScreenPosition.Y,
                 SignatureColor, 1.0f);
        DrawText(
            TEXT("VIBRATION CONTACT"),
            SignatureColor,
            ScreenPosition.X + Radius + 5.0f,
            ScreenPosition.Y - 8.0f,
            GEngine != nullptr ? GEngine->GetSmallFont() : nullptr,
            0.68f * HudScale,
            false);
    }
    if (Presented > 0 && !bLoggedVibrationPresentationReady)
    {
        bLoggedVibrationPresentationReady = true;
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_VIBRATION_PRESENTATION_READY] contacts=%d anonymous=true quantized=true nonColor=true"),
            Presented);
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
