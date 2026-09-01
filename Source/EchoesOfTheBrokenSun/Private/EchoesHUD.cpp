#include "EchoesHUD.h"

#include "EchoesNarrativeSubsystem.h"

#include "EchoesContentSubsystem.h"
#include "EchoesAssemblyOfTheMissingMissionModel.h"
#include "EchoesBrokenSunMissionModel.h"
#include "EchoesSeveralVoicesOneCommandMissionModel.h"
#include "EchoesCommandDeckModel.h"
#include "EchoesContactIndicatorLayout.h"
#include "EchoesEntityView.h"
#include "EchoesFactionPolicy.h"
#include "EchoesGameUserSettings.h"
#include "EchoesGameInstance.h"
#include "EchoesHudLayout.h"
#include "EchoesOfTheBrokenSun.h"
#include "EchoesPlayerController.h"
#include "EchoesOnlineFrontDoorLayout.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesSkirmishOverlayLayout.h"
#include "EchoesSkirmishSetup.h"
#include "EchoesTechnologyPanelLayout.h"
#include "EchoesVisualTheme.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

#include <algorithm>
#include <optional>

namespace
{
EEchoesVisualFaction VisualFactionFromLabel(const FString& Label)
{
    if (Label.Contains(TEXT("MERIDIAN"), ESearchCase::IgnoreCase))
    {
        return EEchoesVisualFaction::MeridianCompact;
    }
    if (Label.Contains(TEXT("KHARUUN"), ESearchCase::IgnoreCase))
    {
        return EEchoesVisualFaction::KharuunAssemblies;
    }
    if (Label.Contains(TEXT("CHOIR"), ESearchCase::IgnoreCase))
    {
        return EEchoesVisualFaction::HollowChoir;
    }
    return EEchoesVisualFaction::Neutral;
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
        case echoes::sim::ResearchType::ChoirHeldAlternatives:
            return TEXT("Held Alternatives");
        case echoes::sim::ResearchType::ChoirSharedResolution:
            return TEXT("Shared Resolution");
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

const TCHAR* ChoirIdentityDisplayName(
    echoes::sim::ChoirIdentityState State)
{
    switch (State)
    {
        case echoes::sim::ChoirIdentityState::Manifest:
            return TEXT("MANIFEST");
        case echoes::sim::ChoirIdentityState::Possible:
            return TEXT("POSSIBLE");
        case echoes::sim::ChoirIdentityState::DualResolveManifest:
            return TEXT("RESOLVING TO MANIFEST");
        case echoes::sim::ChoirIdentityState::DualResolvePossible:
            return TEXT("RESOLVING TO POSSIBLE");
        default:
            return TEXT("UNRESOLVED");
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

FString ActiveSkirmishMapDisplayName(
    const UEchoesSimulationSubsystem* Bridge)
{
    const EEchoesSkirmishMapPreset Preset = Bridge != nullptr
        ? Bridge->GetActiveSkirmishSetup().MapPreset
        : EEchoesSkirmishMapPreset::GlassScar;
    return FEchoesSkirmishSetupModel::MapDisplayName(Preset);
}
}

void AEchoesHUD::DrawVisualPanel(
    const FBox2D& Bounds,
    const FEchoesVisualTheme& Theme,
    const bool bEmphasized)
{
    if (Canvas == nullptr || Bounds.GetSize().X <= 0.0f ||
        Bounds.GetSize().Y <= 0.0f)
    {
        return;
    }
    const FVector2D Size = Bounds.GetSize();
    const float Border = bEmphasized
        ? Theme.EmphasisThickness : Theme.BorderThickness;
    const float Corner = FMath::Min(
        Theme.CornerLength,
        FMath::Min(Size.X, Size.Y) * 0.22f);
    DrawRect(Theme.ElevatedSurface,
             Bounds.Min.X, Bounds.Min.Y, Size.X, Size.Y);
    DrawLine(Bounds.Min.X, Bounds.Min.Y, Bounds.Max.X, Bounds.Min.Y,
             Theme.Border, Theme.BorderThickness);
    DrawLine(Bounds.Max.X, Bounds.Min.Y, Bounds.Max.X, Bounds.Max.Y,
             Theme.Border, Theme.BorderThickness);
    DrawLine(Bounds.Max.X, Bounds.Max.Y, Bounds.Min.X, Bounds.Max.Y,
             Theme.Border, Theme.BorderThickness);
    DrawLine(Bounds.Min.X, Bounds.Max.Y, Bounds.Min.X, Bounds.Min.Y,
             Theme.Border, Theme.BorderThickness);
    DrawLine(Bounds.Min.X, Bounds.Min.Y, Bounds.Min.X + Corner, Bounds.Min.Y,
             Theme.Accent, Border);
    DrawLine(Bounds.Min.X, Bounds.Min.Y, Bounds.Min.X, Bounds.Min.Y + Corner,
             Theme.Accent, Border);
    DrawLine(Bounds.Max.X - Corner, Bounds.Max.Y, Bounds.Max.X, Bounds.Max.Y,
             Theme.Accent, Border);
    DrawLine(Bounds.Max.X, Bounds.Max.Y - Corner, Bounds.Max.X, Bounds.Max.Y,
             Theme.Accent, Border);
}

void AEchoesHUD::DrawShatteredSunMotif(
    const FBox2D& Bounds,
    const FEchoesVisualTheme& Theme,
    const UEchoesGameUserSettings* Settings,
    const float Opacity)
{
    if (Canvas == nullptr || Opacity <= 0.0f)
    {
        return;
    }
    const bool bReducedMotion =
        Settings != nullptr && Settings->IsReducedMotionEnabled();
    const float Time = !bReducedMotion && GetWorld() != nullptr
        ? GetWorld()->GetTimeSeconds() * 0.018f : 0.0f;
    const FVector2D Size = Bounds.GetSize();
    const FVector2D Center(
        Bounds.Min.X + Size.X * 0.78f,
        Bounds.Min.Y + Size.Y * 0.24f);
    const float BaseRadius = FMath::Clamp(
        FMath::Min(Size.X, Size.Y) * 0.16f,
        38.0f,
        150.0f);
    const FLinearColor Ring = Theme.WithAlpha(Theme.MissionCritical, Opacity);
    const FLinearColor Fracture = Theme.WithAlpha(Theme.Accent, Opacity * 0.65f);

    for (int32 RingIndex = 0; RingIndex < 3; ++RingIndex)
    {
        const float Radius = BaseRadius * (0.66f + RingIndex * 0.22f);
        constexpr int32 SegmentCount = 24;
        for (int32 Segment = 0; Segment < SegmentCount; ++Segment)
        {
            if ((Segment + RingIndex * 2) % 5 == 0)
            {
                continue;
            }
            const float StartAngle = Time * (RingIndex % 2 == 0 ? 1.0f : -1.0f) +
                static_cast<float>(Segment) * UE_TWO_PI / SegmentCount;
            const float EndAngle = StartAngle + UE_TWO_PI / SegmentCount * 0.68f;
            const FVector2D Start = Center + FVector2D(
                FMath::Cos(StartAngle), FMath::Sin(StartAngle)) * Radius;
            const FVector2D End = Center + FVector2D(
                FMath::Cos(EndAngle), FMath::Sin(EndAngle)) * Radius;
            DrawLine(Start.X, Start.Y, End.X, End.Y, Ring,
                     RingIndex == 1 ? 2.0f : 1.0f);
        }
    }
    for (int32 Ray = 0; Ray < 12; ++Ray)
    {
        if (Ray == 2 || Ray == 7)
        {
            continue;
        }
        const float Angle = static_cast<float>(Ray) * UE_TWO_PI / 12.0f - Time;
        const FVector2D Direction(FMath::Cos(Angle), FMath::Sin(Angle));
        const FVector2D Start = Center + Direction * BaseRadius * 0.22f;
        const FVector2D End = Center + Direction * BaseRadius *
            (Ray % 3 == 0 ? 1.38f : 1.08f);
        DrawLine(Start.X, Start.Y, End.X, End.Y, Fracture, 1.0f);
    }
    const float Core = BaseRadius * 0.18f;
    DrawLine(Center.X, Center.Y - Core, Center.X + Core, Center.Y,
             Ring, 2.0f);
    DrawLine(Center.X + Core, Center.Y, Center.X, Center.Y + Core,
             Ring, 2.0f);
    DrawLine(Center.X, Center.Y + Core, Center.X - Core, Center.Y,
             Ring, 2.0f);
    DrawLine(Center.X - Core, Center.Y, Center.X, Center.Y - Core,
             Ring, 2.0f);
}

void AEchoesHUD::DrawFactionSigil(
    const EEchoesVisualFaction Faction,
    const FVector2D& Center,
    const float Radius,
    const FEchoesVisualTheme& Theme,
    const float Opacity)
{
    if (Canvas == nullptr || Radius <= 0.0f)
    {
        return;
    }
    const FLinearColor Color = Theme.WithAlpha(
        Theme.FactionColor(Faction), Opacity);
    const float Weight = FMath::Max(1.0f, Theme.BorderThickness);
    const auto Line = [this, Color, Weight](
                          const FVector2D& A,
                          const FVector2D& B)
    {
        DrawLine(A.X, A.Y, B.X, B.Y, Color, Weight);
    };
    if (Faction == EEchoesVisualFaction::MeridianCompact)
    {
        const FVector2D Top(Center.X, Center.Y - Radius);
        const FVector2D Right(Center.X + Radius, Center.Y);
        const FVector2D Bottom(Center.X, Center.Y + Radius);
        const FVector2D Left(Center.X - Radius, Center.Y);
        Line(Top, Right); Line(Right, Bottom); Line(Bottom, Left); Line(Left, Top);
        Line(FVector2D(Center.X, Center.Y - Radius * 0.68f),
             FVector2D(Center.X, Center.Y + Radius * 0.68f));
        Line(FVector2D(Center.X - Radius * 0.42f, Center.Y),
             FVector2D(Center.X + Radius * 0.42f, Center.Y));
    }
    else if (Faction == EEchoesVisualFaction::KharuunAssemblies)
    {
        const FVector2D Top(Center.X, Center.Y - Radius);
        const FVector2D BottomRight(Center.X + Radius * 0.86f,
                                    Center.Y + Radius * 0.62f);
        const FVector2D BottomLeft(Center.X - Radius * 0.86f,
                                   Center.Y + Radius * 0.62f);
        Line(Top, BottomRight); Line(BottomRight, BottomLeft); Line(BottomLeft, Top);
        Line(FVector2D(Center.X, Center.Y - Radius * 0.34f),
             FVector2D(Center.X - Radius * 0.52f, Center.Y + Radius));
        Line(FVector2D(Center.X, Center.Y - Radius * 0.34f),
             FVector2D(Center.X + Radius * 0.52f, Center.Y + Radius));
    }
    else if (Faction == EEchoesVisualFaction::HollowChoir)
    {
        const float Offset = Radius * 0.24f;
        Line(FVector2D(Center.X - Radius, Center.Y - Offset),
             FVector2D(Center.X, Center.Y - Radius));
        Line(FVector2D(Center.X, Center.Y - Radius),
             FVector2D(Center.X + Radius, Center.Y - Offset));
        Line(FVector2D(Center.X + Radius, Center.Y - Offset),
             FVector2D(Center.X, Center.Y + Radius - Offset));
        Line(FVector2D(Center.X, Center.Y + Radius - Offset),
             FVector2D(Center.X - Radius, Center.Y - Offset));
        Line(FVector2D(Center.X - Radius * 0.72f, Center.Y + Offset),
             FVector2D(Center.X, Center.Y - Radius + Offset));
        Line(FVector2D(Center.X, Center.Y - Radius + Offset),
             FVector2D(Center.X + Radius * 0.72f, Center.Y + Offset));
        Line(FVector2D(Center.X + Radius * 0.72f, Center.Y + Offset),
             FVector2D(Center.X, Center.Y + Radius));
        Line(FVector2D(Center.X, Center.Y + Radius),
             FVector2D(Center.X - Radius * 0.72f, Center.Y + Offset));
    }
    else
    {
        Line(FVector2D(Center.X - Radius, Center.Y),
             FVector2D(Center.X + Radius, Center.Y));
        Line(FVector2D(Center.X, Center.Y - Radius),
             FVector2D(Center.X, Center.Y + Radius));
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
    const FEchoesHudLayout HudLayout = FEchoesHudLayout::Build(
        Canvas != nullptr
            ? FVector2D(Canvas->ClipX, Canvas->ClipY)
            : FVector2D(1280.0f, 720.0f),
        HudScale,
        false);
    const bool bHighContrast =
        Settings != nullptr && Settings->IsHighContrastHudEnabled();
    const FEchoesVisualTheme Theme =
        UEchoesVisualThemeSettings::Resolve(bHighContrast);
    const float TextX = 34.0f;
    const auto HudY = [HudScale](float Offset)
    {
        return 18.0f + Offset * HudScale;
    };
    const float PanelWidth = HudLayout.MainPanel.GetSize().X;
    const FLinearColor PanelColor = Theme.Surface;
    const FLinearColor AccentColor = Theme.Accent;
    const FLinearColor SecondaryColor = Theme.TextSecondary;

    if (EchoesController != nullptr &&
        EchoesController->IsOnlineFrontDoorVisible())
    {
        DrawOnlineFrontDoor(EchoesController, Settings);
        return;
    }

    if (Canvas != nullptr && EchoesController != nullptr &&
        EchoesController->IsNetworkCompatibilityAccepted() &&
        !EchoesController->IsNetworkMatchStarted())
    {
        const float LobbyWidth = FMath::Min(Canvas->ClipX - 80.0f, 720.0f);
        const float LobbyHeight = 286.0f;
        const float LobbyX = (Canvas->ClipX - LobbyWidth) * 0.5f;
        const float LobbyY = (Canvas->ClipY - LobbyHeight) * 0.5f;
        DrawRect(PanelColor, LobbyX, LobbyY, LobbyWidth, LobbyHeight);
        DrawText(
            TEXT("ONLINE LOBBY // GLASS SCAR"),
            AccentColor,
            LobbyX + 32.0f,
            LobbyY + 32.0f,
            GEngine != nullptr ? GEngine->GetMediumFont() : nullptr,
            1.15f * HudScale,
            false);
        DrawText(
            FString::Printf(
                TEXT("Connection-bound seat %u // exact build, rules, map, and settings admitted"),
                EchoesController->GetNetworkSeat()),
            FLinearColor::White,
            LobbyX + 32.0f,
            LobbyY + 94.0f,
            GEngine != nullptr ? GEngine->GetSmallFont() : nullptr,
            0.95f * HudScale,
            false);
        DrawText(
            TEXT("The authority remains paused until this seat is ready."),
            SecondaryColor,
            LobbyX + 32.0f,
            LobbyY + 128.0f,
            GEngine != nullptr ? GEngine->GetSmallFont() : nullptr,
            0.92f * HudScale,
            false);
        DrawText(
            TEXT("[ENTER]  READY AND START MATCH"),
            AccentColor,
            LobbyX + 32.0f,
            LobbyY + 184.0f,
            GEngine != nullptr ? GEngine->GetMediumFont() : nullptr,
            1.0f * HudScale,
            false);
        DrawText(
            TEXT("[ESCAPE]  CANCEL TO ONLINE MENU"),
            SecondaryColor,
            LobbyX + 32.0f,
            LobbyY + 226.0f,
            GEngine != nullptr ? GEngine->GetSmallFont() : nullptr,
            0.88f * HudScale,
            false);
        return;
    }

    if (EchoesController != nullptr && EchoesController->IsTitleScreenVisible())
    {
        if (EchoesController->IsSkirmishSetupVisible())
        {
            DrawSkirmishSetup(EchoesController, Settings);
        }
        else
        {
            DrawTitleScreen(EchoesController, Settings);
        }
        DrawOnlineTitleEntry(Settings);
        return;
    }

    if (EchoesController != nullptr && EchoesController->IsMissionBriefingVisible())
    {
        if (EchoesController->IsSkirmishDeploymentSummaryVisible())
        {
            DrawSkirmishDeploymentSummary(EchoesController, Settings);
        }
        else
        {
            DrawMissionBriefing(EchoesController, Settings);
        }
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

    DrawRect(
        PanelColor,
        HudLayout.MainPanel.Min.X,
        HudLayout.MainPanel.Min.Y,
        PanelWidth,
        HudLayout.MainPanel.GetSize().Y);
    DrawText(
        TEXT("ECHOES OF THE BROKEN SUN  //  SORYN TACTICAL AUTHORITY"),
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
            const echoes::presentation::FFactionTechnologyProfile Profile =
                echoes::presentation::TechnologyProfile(Player->faction);
            const echoes::sim::ResearchType First = Profile.TierOne;
            const echoes::sim::ResearchType Second = Profile.TierTwo;
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
    else if (EchoesController != nullptr)
    {
        if (const echoes::sim::net::ScopedViewKeyframe* NetworkView =
                EchoesController->GetNetworkScopedView())
        {
            ResourceLine = FString::Printf(
                TEXT("Matter  %d     Dawnshards  %d     Logistics  %d/%d     NETWORK ACTIVE     Tick  %llu     Snapshot  %llu"),
                NetworkView->resources.material,
                NetworkView->resources.dawnshards,
                NetworkView->populationUsed,
                NetworkView->populationCapacity,
                static_cast<unsigned long long>(
                    NetworkView->simulationTick),
                static_cast<unsigned long long>(NetworkView->snapshotId));
            ResearchLine = FString::Printf(
                TEXT("REMOTE BATTLEFIELD  %d scoped entities     full state and base-linked deltas only"),
                EchoesController->GetNetworkPresentedEntityCount());
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
            if (const echoes::sim::Entity* Entity =
                    Bridge->FindEntity(SelectedIds[0]);
                Entity != nullptr &&
                Entity->choirIdentityState !=
                    echoes::sim::ChoirIdentityState::NotChoir)
            {
                SelectedType += FString::Printf(
                    TEXT(" — %s"),
                    ChoirIdentityDisplayName(
                        Entity->choirIdentityState));
                if (Entity->choirIdentityResolveAtTick >
                    Sim->CurrentTick())
                {
                    const double SecondsRemaining =
                        static_cast<double>(
                            Entity->choirIdentityResolveAtTick -
                            Sim->CurrentTick()) /
                        static_cast<double>(
                            FMath::Max<uint32>(
                                1,
                                Sim->Config().ticksPerSecond));
                    SelectedType += FString::Printf(
                        TEXT(" %.1fs"), SecondsRemaining);
                }
                else if (Entity->choirIdentityNextAvailableTick >
                         Sim->CurrentTick())
                {
                    const double SecondsRemaining =
                        static_cast<double>(
                            Entity->choirIdentityNextAvailableTick -
                            Sim->CurrentTick()) /
                        static_cast<double>(
                            FMath::Max<uint32>(
                                1,
                                Sim->Config().ticksPerSecond));
                    SelectedType += FString::Printf(
                        TEXT(" — RECONCILIATION %.1fs"),
                        SecondsRemaining);
                }
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
        else if (Bridge != nullptr &&
                 Bridge->GetOperationMode() ==
                     EEchoesOperationMode::CampaignShapeBesideUs)
        {
            SelectionLine = FString::Printf(
                TEXT("Selected  %d%s     Formation  %s     Overlap branch  %s"),
                SelectedIds.Num(),
                *SelectedType,
                *EchoesController->GetFormationLabel(),
                WellChoiceDisplayName(Bridge->GetRecordedPrologueChoice()));
        }
        else if (Bridge != nullptr &&
                 Bridge->GetOperationMode() ==
                     EEchoesOperationMode::CampaignReserveAuthority)
        {
            SelectionLine = FString::Printf(
                TEXT("Selected  %d%s     Formation  %s     Reserve doctrine  %s"),
                SelectedIds.Num(),
                *SelectedType,
                *EchoesController->GetFormationLabel(),
                Bridge->GetReserveAuthorityPlan().DisplayName);
        }
        else if (Bridge != nullptr &&
                 Bridge->GetOperationMode() ==
                     EEchoesOperationMode::CampaignChoirAtLumeReach)
        {
            const FEchoesObjectiveSnapshot Objective =
                Bridge->GetLocalObjectiveSnapshot();
            SelectionLine = FString::Printf(
                TEXT("Selected  %d%s     Formation  %s     Approach  %s     Lume Well  %s"),
                SelectedIds.Num(),
                *SelectedType,
                *EchoesController->GetFormationLabel(),
                Bridge->GetChoirAtLumeReachPlan().DisplayName,
                WellChoiceDisplayName(
                    Objective.ChoirAtLumeReachWellChoice));
        }
        else if (Bridge != nullptr &&
                 Bridge->GetOperationMode() ==
                     EEchoesOperationMode::CampaignNoNeutralLedger)
        {
            const FEchoesNoNeutralLedgerPlan Plan =
                Bridge->GetNoNeutralLedgerPlan();
            SelectionLine = FString::Printf(
                TEXT("Selected  %d%s     Formation  %s     Route  %s     Recorded Lume protocol  %s"),
                SelectedIds.Num(),
                *SelectedType,
                *EchoesController->GetFormationLabel(),
                Plan.RouteDisplayName,
                Plan.ProtocolDisplayName);
        }
        else if (Bridge != nullptr &&
                 Bridge->GetOperationMode() ==
                     EEchoesOperationMode::CampaignFutureThatWon)
        {
            const FEchoesFutureThatWonPlan Plan =
                Bridge->GetFutureThatWonPlan();
            SelectionLine = FString::Printf(
                TEXT("Selected  %d%s     Formation  %s     Restoration plan  %02u     Recorded protocol  %s"),
                SelectedIds.Num(),
                *SelectedType,
                *EchoesController->GetFormationLabel(),
                Plan.StablePlanKey,
                Plan.ProtocolDisplayName);
        }
        else if (Bridge != nullptr &&
                 Bridge->GetOperationMode() ==
                     EEchoesOperationMode::CampaignAssemblyOfTheMissing)
        {
            const FEchoesAssemblyOfTheMissingPlan Plan =
                Bridge->GetAssemblyOfTheMissingPlan();
            SelectionLine = FString::Printf(
                TEXT("Selected  %d%s     Formation  %s     Public assembly plan  %02u     Recorded protocol  %s"),
                SelectedIds.Num(),
                *SelectedType,
                *EchoesController->GetFormationLabel(),
                Plan.StablePlanKey,
                Plan.ProtocolDisplayName);
        }
        else if (Bridge != nullptr &&
                 Bridge->GetOperationMode() ==
                     EEchoesOperationMode::CampaignSeveralVoicesOneCommand)
        {
            const FEchoesSeveralVoicesOneCommandPlan Plan =
                Bridge->GetSeveralVoicesOneCommandPlan();
            SelectionLine = FString::Printf(
                TEXT("Selected  %d%s     Formation  %s     Choir crisis plan  %02u     Recorded protocol  %s"),
                SelectedIds.Num(),
                *SelectedType,
                *EchoesController->GetFormationLabel(),
                Plan.StablePlanKey,
                Plan.ProtocolDisplayName);
        }
        else if (Bridge != nullptr &&
                 Bridge->GetOperationMode() ==
                     EEchoesOperationMode::CampaignTheBrokenSun)
        {
            const FEchoesBrokenSunPlan Plan = Bridge->GetBrokenSunPlan();
            const FEchoesObjectiveSnapshot Objective =
                Bridge->GetLocalObjectiveSnapshot();
            SelectionLine = FString::Printf(
                TEXT("Selected  %d%s     Formation  %s     Final plan  %02u     Resolution  %s"),
                SelectedIds.Num(),
                *SelectedType,
                *EchoesController->GetFormationLabel(),
                Plan.StablePlanKey,
                FEchoesBrokenSunMissionModel::ResolutionDisplayName(
                    Objective.BrokenSunFinalResolution));
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
        if (const echoes::sim::net::ScopedViewKeyframe* RemoteView =
                EchoesController->GetNetworkScopedView())
        {
            SelectionLine = FString::Printf(
                TEXT("AUTHORITY SNAPSHOT  %llu     Selected  %d     Formation  %s"),
                static_cast<unsigned long long>(RemoteView->snapshotId),
                SelectedIds.Num(),
                *EchoesController->GetFormationLabel());
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

    const echoes::sim::net::ScopedViewKeyframe* NetworkView =
        EchoesController != nullptr
            ? EchoesController->GetNetworkScopedView()
            : nullptr;
    const bool bNetworkRemoteView = NetworkView != nullptr;
    DrawText(
        bNetworkRemoteView
            ? TEXT("ONLINE  WASD / edge: pan  Wheel: zoom  LMB/drag: select  RMB: pointer order  [Space] keyboard order")
            : TEXT("WASD / edge: pan  Wheel: zoom  LMB/drag: select  RMB: pointer order  [Home] Key target  [End] Snap  [Arrows] Move  [Space] Order"),
        SecondaryColor,
        TextX,
        HudY(90.0f),
        GEngine != nullptr ? GEngine->GetSmallFont() : nullptr,
        0.86f * HudScale,
        false);
    const echoes::sim::Faction PresentedFaction =
        NetworkView != nullptr
            ? NetworkView->faction
            : Bridge != nullptr
                  ? Bridge->GetLocalFaction()
                  : echoes::sim::Faction::MeridianCompact;
    FString FactionControlLine;
    switch (PresentedFaction)
    {
        case echoes::sim::Faction::MeridianCompact:
            FactionControlLine = TEXT("[F] Attack-move  [T] Patrol  [H] Hold  [J] Guard  [X] Stop  [\\] Bulwark  [=] Relay  [B/N/M] Build");
            break;
        case echoes::sim::Faction::KharuunAssemblies:
            FactionControlLine = TEXT("[F] Attack-move  [T] Patrol  [H] Hold  [J] Guard  [X] Stop  [F3] Waystone  [F4/F5] Warform  [F6] Cover");
            break;
        case echoes::sim::Faction::HollowChoir:
            FactionControlLine = TEXT("[F] Attack-move  [T] Patrol  [H] Hold  [J] Guard  [X] Stop  [Shift+F3] Manifest  [Shift+F4] Possible");
            break;
    }
    DrawText(
        FactionControlLine,
        SecondaryColor,
        TextX,
        HudY(113.0f),
        GEngine != nullptr ? GEngine->GetSmallFont() : nullptr,
        0.86f * HudScale,
        false);
    const FString GroupControlLine = bNetworkRemoteView
        ? FString::Printf(
              TEXT("Seat %u  //  two-Hz scoped state  //  base-linked delta recovery  //  hidden authority state excluded"),
              EchoesController->GetNetworkSeat())
        : FString::Printf(
              TEXT("[Tab] Next owned    [Backspace] Previous    [F7] Combat force    [F8] Formation %s    [1-0] Recall    [G + 1-0] Assign    [F2] Tech    [P] Pause"),
              EchoesController != nullptr
                  ? *EchoesController->GetFormationLabel()
                  : TEXT("BOX"));
    DrawText(
        GroupControlLine,
        SecondaryColor,
        TextX,
        HudY(136.0f),
        GEngine != nullptr ? GEngine->GetSmallFont() : nullptr,
        0.86f * HudScale,
        false);
    const FString FactionStatusLine = bNetworkRemoteView
        ? FString::Printf(
              TEXT("Connection-bound local seat: %s     Opponent: %s"),
              *EchoesController->GetLocalFactionLabel(),
              *EchoesController->GetOpponentFactionLabel())
        : FString::Printf(
              TEXT("[Z] Harvest    [C] Preserve    [V] Reshape    Local: %s"),
              EchoesController != nullptr
                  ? *EchoesController->GetLocalFactionLabel()
                  : TEXT("MERIDIAN COMPACT"));
    DrawText(
        FactionStatusLine,
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
            const FEchoesHudLayout FeedbackLayout = FEchoesHudLayout::Build(
                Canvas != nullptr
                    ? FVector2D(Canvas->ClipX, Canvas->ClipY)
                    : FVector2D(1280.0f, 720.0f),
                HudScale,
                true);
            DrawRect(
                PanelColor,
                FeedbackLayout.StatusPanel.Min.X,
                FeedbackLayout.StatusPanel.Min.Y,
                FeedbackLayout.StatusPanel.GetSize().X,
                FeedbackLayout.StatusPanel.GetSize().Y);
            DrawText(
                Feedback,
                Feedback.StartsWith(TEXT("["))
                    ? FLinearColor(1.0f, 0.48f, 0.18f)
                    : FLinearColor(0.25f, 1.0f, 0.66f),
                TextX,
                FeedbackLayout.StatusPanel.Min.Y + 14.0f,
                GEngine != nullptr ? GEngine->GetSmallFont() : nullptr,
                0.95f * HudScale,
                false);
        }
    }

    DrawNarrativeSubtitle(EchoesController, Settings);

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
    DrawNetworkReconnectBanner(EchoesController, Settings);
    DrawSelectionRectangle();
    if (EchoesController != nullptr &&
        EchoesController->IsOnlineLocalMenuVisible())
    {
        DrawOnlineLocalMenu(EchoesController, Settings);
    }
    else if (EchoesController != nullptr && EchoesController->IsTechnologyPanelVisible())
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

void AEchoesHUD::DrawOnlineTitleEntry(
    const UEchoesGameUserSettings* Settings)
{
    if (Canvas == nullptr)
    {
        return;
    }
    const float HudScale = Settings != nullptr ? Settings->GetHudScale() : 1.0f;
    const bool bHighContrast =
        Settings != nullptr && Settings->IsHighContrastHudEnabled();
    const FEchoesVisualTheme Theme =
        UEchoesVisualThemeSettings::Resolve(bHighContrast);
    const FBox2D Button = FEchoesOnlineFrontDoorLayout::BuildTitleEntry(
        FVector2D(Canvas->ClipX, Canvas->ClipY), HudScale);
    const FLinearColor Accent = Theme.Accent;
    const FLinearColor TextColor = Theme.ActionText;
    DrawRect(
        Accent,
        Button.Min.X,
        Button.Min.Y,
        Button.GetSize().X,
        Button.GetSize().Y);
    DrawText(
        TEXT("[F8 / GAMEPAD VIEW]  ONLINE 1v1  //  FIXED GLASS SCAR"),
        TextColor,
        Button.Min.X + 16.0f * HudScale,
        Button.Min.Y + 12.0f * HudScale,
        GEngine != nullptr ? GEngine->GetSmallFont() : nullptr,
        0.82f * HudScale,
        false);
}

void AEchoesHUD::DrawOnlineFrontDoor(
    const AEchoesPlayerController* EchoesController,
    const UEchoesGameUserSettings* Settings)
{
    if (Canvas == nullptr || EchoesController == nullptr)
    {
        return;
    }
    const UEchoesGameInstance* EchoesGameInstance =
        GetGameInstance<UEchoesGameInstance>();
    if (EchoesGameInstance == nullptr)
    {
        return;
    }
    const EEchoesOnlineFrontDoorState State =
        EchoesGameInstance->GetOnlineState();
    const float HudScale = Settings != nullptr ? Settings->GetHudScale() : 1.0f;
    const bool bHighContrast =
        Settings != nullptr && Settings->IsHighContrastHudEnabled();
    const FEchoesVisualTheme Theme =
        UEchoesVisualThemeSettings::Resolve(bHighContrast);
    const FEchoesOnlineFrontDoorLayout Layout =
        FEchoesOnlineFrontDoorLayout::Build(
            FVector2D(Canvas->ClipX, Canvas->ClipY), HudScale);
    const FLinearColor Accent = Theme.Accent;
    const FLinearColor Body = Theme.TextPrimary;
    const FLinearColor Muted = Theme.TextSecondary;
    const FLinearColor ErrorColor = Theme.Danger;
    UFont* SmallFont = GEngine != nullptr ? GEngine->GetSmallFont() : nullptr;
    const float Left = Layout.Origin.X;
    const float Top = Layout.Origin.Y;
    const float ContentScale = Layout.ContentScale;
    const float TextScale = Layout.TextScale;

    DrawRect(Theme.Scrim, 0.0f, 0.0f, Canvas->ClipX, Canvas->ClipY);
    DrawVisualPanel(FBox2D(Layout.Origin, Layout.Origin + Layout.Size),
                    Theme, true);
    DrawShatteredSunMotif(
        FBox2D(Layout.Origin, Layout.Origin + Layout.Size),
        Theme, Settings, bHighContrast ? 0.12f : 0.055f);
    DrawText(
        State == EEchoesOnlineFrontDoorState::Hosting
            ? TEXT("HOSTING ONLINE 1v1")
        : State == EEchoesOnlineFrontDoorState::Connecting
            ? TEXT("CONNECTING TO HOST")
        : State == EEchoesOnlineFrontDoorState::Failed
            ? TEXT("CONNECTION FAILED")
            : TEXT("ONLINE 1v1 // DIRECT CONNECT"),
        State == EEchoesOnlineFrontDoorState::Failed ? ErrorColor : Accent,
        Left + 46.0f,
        Top + 38.0f * ContentScale,
        SmallFont,
        1.48f * TextScale,
        false);
    DrawText(
        TEXT("FIXED RULES  //  GLASS SCAR  //  MERIDIAN vs KHARUUN  //  STANDARD RESOURCES"),
        Body,
        Left + 46.0f,
        Top + 82.0f * ContentScale,
        SmallFont,
        0.78f * TextScale,
        false);
    DrawText(
        TEXT("Direct connection only. The first release has no matchmaking, invitations, or NAT traversal."),
        Muted,
        Left + 46.0f,
        Top + 112.0f * ContentScale,
        SmallFont,
        0.74f * TextScale,
        false);

    const auto DrawControl = [this, Accent, Body, Muted, Theme,
                              SmallFont, TextScale, ContentScale](
                                 const FBox2D& Box,
                                 const FString& Label,
                                 bool bFocused,
                                 bool bPrimary)
    {
        const FLinearColor Fill = bFocused || bPrimary
            ? Accent
            : Theme.WithAlpha(Theme.Border, 0.46f);
        const FLinearColor LabelColor = bFocused || bPrimary
            ? Theme.ActionText
            : Body;
        DrawRect(Fill, Box.Min.X, Box.Min.Y,
                 Box.GetSize().X, Box.GetSize().Y);
        if (!bFocused && !bPrimary)
        {
            DrawLine(Box.Min.X, Box.Max.Y, Box.Max.X, Box.Max.Y,
                     Muted, 1.5f);
        }
        DrawText(Label, LabelColor,
                 Box.Min.X + 18.0f * ContentScale,
                 Box.Min.Y + 15.0f * ContentScale,
                 SmallFont, 0.86f * TextScale, false);
    };

    if (State == EEchoesOnlineFrontDoorState::JoinSetup)
    {
        const int32 Focus = EchoesGameInstance->GetOnlineFocusIndex();
        DrawControl(Layout.HostButton,
                    TEXT("HOST FIXED-RULES MATCH  //  PORT 7777"),
                    Focus == 0, false);
        const FString Endpoint = EchoesGameInstance->GetDirectConnectEndpoint();
        DrawControl(
            Layout.EndpointField,
            FString::Printf(
                TEXT("JOIN ADDRESS  //  %s%s"),
                *Endpoint,
                Focus == 1 ? TEXT("_") : TEXT("")),
            Focus == 1,
            false);
        DrawControl(Layout.JoinButton,
                    TEXT("JOIN HOST  //  ENTER OR CLICK"),
                    Focus == 2, false);
        DrawText(
            Focus == 1
                ? TEXT("TYPE OR COMMAND+V  //  BACKSPACE EDITS  //  HOSTNAME OR IPv4:PORT")
                : TEXT("UP / DOWN OR D-PAD SELECTS  //  ENTER / A CONFIRMS"),
            Muted,
            Left + 52.0f,
            Top + 398.0f * ContentScale,
            SmallFont,
            0.76f * TextScale,
            false);
        DrawControl(Layout.BackButton,
                    TEXT("ESCAPE / GAMEPAD B / CLICK: BACK TO OPERATIONS"),
                    Focus == 3, false);
        return;
    }

    if (State == EEchoesOnlineFrontDoorState::Failed)
    {
        DrawText(
            EchoesGameInstance->GetOnlineFailureMessage(),
            ErrorColor,
            Left + 52.0f,
            Top + 188.0f * ContentScale,
            SmallFont,
            0.92f * TextScale,
            false);
        DrawText(
            FString::Printf(
                TEXT("RETAINED ADDRESS  //  %s"),
                *EchoesGameInstance->GetDirectConnectEndpoint()),
            Body,
            Left + 52.0f,
            Top + 246.0f * ContentScale,
            SmallFont,
            0.80f * TextScale,
            false);
        DrawControl(Layout.RetryButton,
                    EchoesGameInstance->HasUsableReconnectContext()
                        ? FString::Printf(
                              TEXT("ENTER / A / CLICK: REJOIN MATCH  //  %d SEC"),
                              EchoesGameInstance->GetReconnectSecondsRemaining())
                        : TEXT("ENTER / A / CLICK: RETRY ONLINE MENU"),
                    true, true);
        DrawControl(Layout.BackButton,
                    TEXT("ESCAPE / GAMEPAD B / CLICK: BACK TO OPERATIONS"),
                    false, false);
        return;
    }

    DrawText(
        State == EEchoesOnlineFrontDoorState::Hosting
            ? TEXT("Waiting for one opponent to connect, pass compatibility, and ready.")
            : TEXT("Contacting the host and validating the exact build, map, rules, and settings."),
        Body,
        Left + 52.0f,
        Top + 194.0f * ContentScale,
        SmallFont,
        0.94f * TextScale,
        false);
    if (State == EEchoesOnlineFrontDoorState::Hosting)
    {
        const FString HostEndpoint =
            EchoesGameInstance->GetHostShareEndpoint().IsEmpty()
                ? TEXT("LAN ADDRESS UNAVAILABLE")
                : EchoesGameInstance->GetHostShareEndpoint();
        DrawControl(
            Layout.EndpointField,
            FString::Printf(
                TEXT("ENTER OR CLICK TO COPY LAN ADDRESS  //  %s"),
                *HostEndpoint),
            false,
            !EchoesGameInstance->GetHostShareEndpoint().IsEmpty());
        DrawText(
            TEXT("LAN/direct only. macOS firewall, router policy, and Internet NAT can still block the connection."),
            Muted,
            Left + 52.0f,
            Top + 322.0f * ContentScale,
            SmallFont,
            0.76f * TextScale,
            false);
    }
    else
    {
        DrawText(
            FString::Printf(
                TEXT("DESTINATION  //  %s"),
                *EchoesGameInstance->GetDirectConnectEndpoint()),
            Muted,
            Left + 52.0f,
            Top + 246.0f * ContentScale,
            SmallFont,
            0.80f * TextScale,
            false);
    }
    DrawControl(
        Layout.BackButton,
        TEXT("ESCAPE / GAMEPAD B / CLICK: CANCEL TO ONLINE MENU"),
        false,
        false);
}

void AEchoesHUD::DrawNetworkReconnectBanner(
    const AEchoesPlayerController* EchoesController,
    const UEchoesGameUserSettings* Settings)
{
    if (Canvas == nullptr || EchoesController == nullptr ||
        !EchoesController->IsOpponentReconnectGraceActive())
    {
        return;
    }
    const float HudScale = Settings != nullptr ? Settings->GetHudScale() : 1.0f;
    const bool bHighContrast =
        Settings != nullptr && Settings->IsHighContrastHudEnabled();
    const FEchoesVisualTheme Theme =
        UEchoesVisualThemeSettings::Resolve(bHighContrast);
    const int32 Remaining =
        EchoesController->GetOpponentReconnectSecondsRemaining();
    const int32 Minutes = Remaining / 60;
    const int32 Seconds = Remaining % 60;
    const float Width = FMath::Min(Canvas->ClipX - 40.0f, 760.0f);
    const float Height = 54.0f * HudScale;
    const float Left = (Canvas->ClipX - Width) * 0.5f;
    const float Top = 84.0f * HudScale;
    const FLinearColor Accent = Theme.Warning;
    DrawRect(Theme.WithAlpha(Theme.ElevatedSurface, 0.98f),
             Left, Top, Width, Height);
    DrawLine(Left, Top, Left + Width, Top, Accent, 3.0f);
    DrawText(
        FString::Printf(
            TEXT("OPPONENT DISCONNECTED  //  AUTHORITY PAUSED  //  RECONNECT %d:%02d"),
            Minutes,
            Seconds),
        Accent,
        Left + 20.0f,
        Top + 17.0f * HudScale,
        GEngine != nullptr ? GEngine->GetSmallFont() : nullptr,
        0.86f * HudScale,
        false);
}

void AEchoesHUD::DrawOnlineLocalMenu(
    const AEchoesPlayerController* EchoesController,
    const UEchoesGameUserSettings* Settings)
{
    if (Canvas == nullptr || EchoesController == nullptr ||
        !EchoesController->IsOnlineLocalMenuVisible())
    {
        return;
    }
    const float HudScale = Settings != nullptr ? Settings->GetHudScale() : 1.0f;
    const bool bHighContrast =
        Settings != nullptr && Settings->IsHighContrastHudEnabled();
    const FEchoesVisualTheme Theme =
        UEchoesVisualThemeSettings::Resolve(bHighContrast);
    const FEchoesOnlineLocalMenuLayout Layout =
        FEchoesOnlineLocalMenuLayout::Build(
            FVector2D(Canvas->ClipX, Canvas->ClipY), HudScale);
    const FLinearColor Accent = Theme.Accent;
    const FLinearColor Body = Theme.TextPrimary;
    const FLinearColor Muted = Theme.TextSecondary;
    UFont* SmallFont = GEngine != nullptr ? GEngine->GetSmallFont() : nullptr;
    const float Left = Layout.Origin.X;
    const float Top = Layout.Origin.Y;
    const float Scale = Layout.ContentScale;
    DrawRect(Theme.Scrim, 0.0f, 0.0f, Canvas->ClipX, Canvas->ClipY);
    DrawVisualPanel(FBox2D(Layout.Origin, Layout.Origin + Layout.Size),
                    Theme, true);
    DrawText(TEXT("ONLINE MATCH"), Accent,
             Left + 42.0f, Top + 36.0f * Scale,
             SmallFont, 1.55f * Layout.TextScale, false);
    DrawText(
        EchoesController->IsOpponentReconnectGraceActive()
            ? TEXT("The authority is paused for the bounded reconnect grace period.")
            : TEXT("This menu is local only. The authoritative match continues for both players."),
        Body,
        Left + 42.0f,
        Top + 94.0f * Scale,
        SmallFont,
        0.90f * Layout.TextScale,
        false);
    DrawText(
        TEXT("Restart is disabled in online play. Leaving ends your connection to this match."),
        Muted,
        Left + 42.0f,
        Top + 132.0f * Scale,
        SmallFont,
        0.80f * Layout.TextScale,
        false);
    DrawRect(Accent,
             Layout.ResumeButton.Min.X, Layout.ResumeButton.Min.Y,
             Layout.ResumeButton.GetSize().X, Layout.ResumeButton.GetSize().Y);
    DrawText(TEXT("ENTER / ESCAPE / P / CLICK: RESUME"),
             Theme.ActionText,
             Layout.ResumeButton.Min.X + 18.0f * Scale,
             Layout.ResumeButton.Min.Y + 15.0f * Scale,
             SmallFont, 0.90f * Layout.TextScale, false);
    DrawRect(Theme.WithAlpha(Theme.Danger, 0.88f),
             Layout.LeaveButton.Min.X, Layout.LeaveButton.Min.Y,
             Layout.LeaveButton.GetSize().X, Layout.LeaveButton.GetSize().Y);
    DrawText(TEXT("F10 / MENU / GAMEPAD X / CLICK: LEAVE ONLINE MATCH"), Body,
             Layout.LeaveButton.Min.X + 18.0f * Scale,
             Layout.LeaveButton.Min.Y + 15.0f * Scale,
             SmallFont, 0.90f * Layout.TextScale, false);
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
    const FEchoesVisualTheme Theme =
        UEchoesVisualThemeSettings::Resolve(bHighContrast);
    const float HudScale = Settings != nullptr ? Settings->GetHudScale() : 1.0f;
    const FEchoesHudLayout Layout = FEchoesHudLayout::Build(
        FVector2D(Canvas->ClipX, Canvas->ClipY), HudScale, false);
    if (!Layout.bCommandDeckVisible)
    {
        return;
    }
    const float PanelWidth = Layout.CommandDeckPanel.GetSize().X;
    const float PanelHeight = Layout.CommandDeckPanel.GetSize().Y;
    const float Left = Layout.CommandDeckPanel.Min.X;
    const float Top = Layout.CommandDeckPanel.Min.Y;

    const FLinearColor Accent = Theme.Accent;
    const FLinearColor Body = Theme.TextPrimary;
    const FLinearColor Muted = Theme.TextSecondary;
    UFont* SmallFont = GEngine != nullptr ? GEngine->GetSmallFont() : nullptr;
    DrawVisualPanel(Layout.CommandDeckPanel, Theme, true);
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
    else if (
        Bridge->GetLocalFaction() == echoes::sim::Faction::HollowChoir &&
        Profile.CombatCount > 0)
    {
        ContextLine += TEXT("    [SHIFT+F3/F4] RECONCILE IDENTITY");
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
    const FEchoesVisualTheme Theme =
        UEchoesVisualThemeSettings::Resolve(bHighContrast);
    const FEchoesTechnologyPanelLayout Layout =
        FEchoesTechnologyPanelLayout::Build(
            FVector2D(Canvas->ClipX, Canvas->ClipY), HudScale);
    const FLinearColor AccentColor = Theme.Accent;
    const FLinearColor MutedColor = Theme.TextSecondary;

    DrawRect(Theme.Scrim, 0.0f, 0.0f, Canvas->ClipX, Canvas->ClipY);
    DrawVisualPanel(FBox2D(Layout.Origin, Layout.Origin + Layout.Size),
                    Theme, true);

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

    const echoes::presentation::FFactionTechnologyProfile Profile =
        echoes::presentation::TechnologyProfile(Player->faction);
    const echoes::sim::ResearchType Technologies[2] = {
        Profile.TierOne,
        Profile.TierTwo,
    };
    const TCHAR* StableIds[2] = {
        Profile.TierOneContentId,
        Profile.TierTwoContentId,
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
            StatusColor = Theme.Success;
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
            StatusColor = Theme.Danger;
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
            StatusColor = Theme.Danger;
        }
        else if (!bSelectedProducer)
        {
            Status = TEXT("READY — select a production structure");
            StatusColor = Theme.Warning;
        }
        else
        {
            Status = TEXT("READY — click to research");
        }

        const FLinearColor RowColor = bComplete
            ? Theme.WithAlpha(Theme.Success, 0.12f)
            : bActive
                ? Theme.WithAlpha(Theme.Selected, 0.16f)
                : bInterrupted
                    ? Theme.WithAlpha(Theme.Danger, 0.16f)
                    : Theme.WithAlpha(Theme.Border, 0.20f);
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
            Theme.TextPrimary,
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

void AEchoesHUD::DrawSkirmishSetup(
    const AEchoesPlayerController* EchoesController,
    const UEchoesGameUserSettings* Settings)
{
    if (Canvas == nullptr || EchoesController == nullptr ||
        !EchoesController->IsSkirmishSetupVisible())
    {
        return;
    }

    const bool bHighContrast =
        Settings != nullptr && Settings->IsHighContrastHudEnabled();
    const FEchoesVisualTheme Theme =
        UEchoesVisualThemeSettings::Resolve(bHighContrast);
    const float HudScale = Settings != nullptr ? Settings->GetHudScale() : 1.0f;
    const FEchoesSkirmishSetupOverlayLayout Layout =
        FEchoesSkirmishSetupOverlayLayout::Build(
            FVector2D(Canvas->ClipX, Canvas->ClipY),
            HudScale);
    const float PanelWidth = Layout.Size.X;
    const float PanelHeight = Layout.Size.Y;
    const float Left = Layout.Origin.X;
    const float Top = Layout.Origin.Y;
    const float ContentScale = Layout.ContentScale;
    const float TextScale = Layout.TextScale;
    const FLinearColor Accent = Theme.Accent;
    const FLinearColor Body = Theme.TextPrimary;
    const FLinearColor Muted = Theme.TextSecondary;
    UFont* SmallFont = GEngine != nullptr ? GEngine->GetSmallFont() : nullptr;
    const FEchoesSkirmishSetup& Setup =
        EchoesController->GetPendingSkirmishSetup();
    const int32 FocusRow = EchoesController->GetSkirmishSetupFocusRow();
    const FString Labels[] = {
        TEXT("LOCAL FORCE"),
        TEXT("OPPOSING FORCE"),
        TEXT("BATTLEFIELD"),
        TEXT("AI PROFILE"),
        TEXT("STARTING RESOURCES")};
    const FString Values[] = {
        FEchoesSkirmishSetupModel::FactionDisplayName(Setup.LocalFaction),
        FEchoesSkirmishSetupModel::FactionDisplayName(Setup.OpponentFaction),
        FEchoesSkirmishSetupModel::MapDisplayName(Setup.MapPreset),
        FEchoesSkirmishSetupModel::AiDisplayName(Setup.AiPersonality),
        FEchoesSkirmishSetupModel::ResourceDisplayName(Setup.ResourceLevel)};

    DrawRect(Theme.Scrim, 0.0f, 0.0f, Canvas->ClipX, Canvas->ClipY);
    DrawVisualPanel(
        FBox2D(Layout.Origin, Layout.Origin + Layout.Size), Theme, true);
    DrawShatteredSunMotif(
        FBox2D(Layout.Origin, Layout.Origin + Layout.Size),
        Theme, Settings, bHighContrast ? 0.10f : 0.045f);
    DrawText(TEXT("SKIRMISH SETUP"), Accent,
             Left + 46.0f, Top + 34.0f * ContentScale,
             SmallFont, 1.58f * TextScale, false);
    DrawText(TEXT("SORYN DEPLOYMENT AUTHORITY  //  OFFLINE 1V1"), Muted,
             Left + 46.0f, Top + 76.0f * ContentScale,
             SmallFont, 0.86f * TextScale, false);
    DrawText(TEXT("[UP/DOWN or D-PAD] FOCUS    [LEFT/RIGHT or CLICK HALF] CHANGE"), Body,
             Left + 46.0f, Top + 108.0f * ContentScale,
             SmallFont, 0.82f * TextScale, false);

    for (int32 Row = 0; Row < UE_ARRAY_COUNT(Labels); ++Row)
    {
        const FBox2D& RowBox = Layout.SettingRows[Row];
        const float RowTop = RowBox.Min.Y + 8.0f * ContentScale;
        const bool bFocused = Row == FocusRow;
        DrawRect(
            bFocused
                ? Theme.WithAlpha(Theme.Selected, bHighContrast ? 0.24f : 0.30f)
                : Theme.WithAlpha(Theme.Border, 0.18f),
            RowBox.Min.X,
            RowBox.Min.Y,
            RowBox.GetSize().X,
            RowBox.GetSize().Y);
        if (bFocused)
        {
            DrawLine(
                RowBox.Min.X,
                RowBox.Min.Y,
                RowBox.Min.X,
                RowBox.Max.Y,
                Accent,
                4.0f);
        }
        DrawText(
            FString::Printf(TEXT("%s%s"), bFocused ? TEXT(">  ") : TEXT("   "), *Labels[Row]),
            bFocused ? Accent : Muted,
            Left + 52.0f,
            RowTop,
            SmallFont,
            0.86f * TextScale,
            false);
        DrawText(
            FString::Printf(TEXT("<  %s  >"), *Values[Row]),
            Body,
            Left + PanelWidth * 0.47f,
            RowTop,
            SmallFont,
            0.88f * TextScale,
            false);
        if (Row < 2)
        {
            DrawFactionSigil(
                VisualFactionFromLabel(Values[Row]),
                FVector2D(RowBox.Max.X - 22.0f * ContentScale,
                          (RowBox.Min.Y + RowBox.Max.Y) * 0.5f),
                11.0f * ContentScale,
                Theme,
                bFocused ? 1.0f : 0.68f);
        }
    }

    DrawText(TEXT("BATTLEFIELD CONTRACT"), Accent,
             Left + 46.0f, Top + 448.0f * ContentScale,
             SmallFont, 0.88f * TextScale, false);
    DrawText(
        FEchoesSkirmishSetupModel::MapDescription(Setup.MapPreset),
        Body, Left + 46.0f, Top + 478.0f * ContentScale,
        SmallFont, 0.78f * TextScale, false);
    DrawText(
        FEchoesSkirmishSetupModel::AiDescription(Setup.AiPersonality),
        Muted, Left + 46.0f, Top + 506.0f * ContentScale,
        SmallFont, 0.76f * TextScale, false);
    DrawText(
        TEXT("Choices are staged only. The active simulation changes after deployment confirmation."),
        Muted, Left + 46.0f, Top + 542.0f * ContentScale,
        SmallFont, 0.76f * TextScale, false);
    DrawText(
        TEXT("[F9] CAMPAIGN OPERATIONS    [C] CONTINUE CAMPAIGN    [TAB] NEXT LOCAL FORCE"),
        Body, Left + 46.0f, Top + 574.0f * ContentScale,
        SmallFont, 0.76f * TextScale, false);

    DrawRect(Accent,
             Layout.ReviewButton.Min.X,
             Layout.ReviewButton.Min.Y,
             Layout.ReviewButton.GetSize().X,
             Layout.ReviewButton.GetSize().Y);
    DrawText(TEXT("ENTER / GAMEPAD A: REVIEW DEPLOYMENT"),
             Theme.ActionText,
             Left + PanelWidth * 0.5f - 194.0f * TextScale,
             Top + PanelHeight - 65.0f,
             SmallFont, 0.92f * TextScale, false);
}

void AEchoesHUD::DrawSkirmishDeploymentSummary(
    const AEchoesPlayerController* EchoesController,
    const UEchoesGameUserSettings* Settings)
{
    if (Canvas == nullptr || EchoesController == nullptr ||
        !EchoesController->IsSkirmishDeploymentSummaryVisible())
    {
        return;
    }
    const bool bHighContrast =
        Settings != nullptr && Settings->IsHighContrastHudEnabled();
    const FEchoesVisualTheme Theme =
        UEchoesVisualThemeSettings::Resolve(bHighContrast);
    const float HudScale = Settings != nullptr ? Settings->GetHudScale() : 1.0f;
    const FEchoesSkirmishSummaryOverlayLayout Layout =
        FEchoesSkirmishSummaryOverlayLayout::Build(
            FVector2D(Canvas->ClipX, Canvas->ClipY),
            HudScale);
    const float PanelWidth = Layout.Size.X;
    const float PanelHeight = Layout.Size.Y;
    const float Left = Layout.Origin.X;
    const float Top = Layout.Origin.Y;
    const float ContentScale = Layout.ContentScale;
    const float TextScale = Layout.TextScale;
    const FLinearColor Accent = Theme.Accent;
    const FLinearColor Body = Theme.TextPrimary;
    const FLinearColor Muted = Theme.TextSecondary;
    UFont* SmallFont = GEngine != nullptr ? GEngine->GetSmallFont() : nullptr;
    const FEchoesSkirmishSetup& Setup =
        EchoesController->GetPendingSkirmishSetup();

    DrawRect(Theme.Scrim, 0.0f, 0.0f, Canvas->ClipX, Canvas->ClipY);
    DrawVisualPanel(
        FBox2D(Layout.Origin, Layout.Origin + Layout.Size), Theme, true);
    DrawText(TEXT("DEPLOYMENT SUMMARY"), Accent,
             Left + 46.0f, Top + 38.0f * ContentScale,
             SmallFont, 1.52f * TextScale, false);
    DrawText(TEXT("VALIDATE THE COMPLETE CONTRACT BEFORE THE FIELD CHANGES"), Muted,
             Left + 46.0f, Top + 80.0f * ContentScale,
             SmallFont, 0.82f * TextScale, false);

    const FString SummaryLines[] = {
        FString::Printf(TEXT("COMMAND AUTHORITY     %s"),
            FEchoesSkirmishSetupModel::FactionDisplayName(Setup.LocalFaction)),
        FString::Printf(TEXT("OPPOSITION            %s"),
            FEchoesSkirmishSetupModel::FactionDisplayName(Setup.OpponentFaction)),
        FString::Printf(TEXT("BATTLEFIELD           %s  //  64 x 64"),
            FEchoesSkirmishSetupModel::MapDisplayName(Setup.MapPreset)),
        FString::Printf(TEXT("AI PROFILE            %s"),
            FEchoesSkirmishSetupModel::AiDisplayName(Setup.AiPersonality)),
        FString::Printf(TEXT("STARTING RESOURCES    %s"),
            FEchoesSkirmishSetupModel::ResourceDisplayName(Setup.ResourceLevel))};
    for (int32 Row = 0; Row < UE_ARRAY_COUNT(SummaryLines); ++Row)
    {
        DrawText(SummaryLines[Row], Row == 0 ? Accent : Body,
                 Left + 54.0f,
                 Top + (136.0f + Row * 48.0f) * ContentScale,
                 SmallFont, 0.92f * TextScale, false);
    }
    DrawFactionSigil(
        VisualFactionFromLabel(
            FEchoesSkirmishSetupModel::FactionDisplayName(Setup.LocalFaction)),
        FVector2D(Left + PanelWidth - 86.0f * ContentScale,
                  Top + 90.0f * ContentScale),
        27.0f * ContentScale,
        Theme);
    DrawText(
        FEchoesSkirmishSetupModel::MapDescription(Setup.MapPreset),
        Muted, Left + 54.0f, Top + 392.0f * ContentScale,
        SmallFont, 0.76f * TextScale, false);
    DrawText(
        FEchoesSkirmishSetupModel::AiDescription(Setup.AiPersonality),
        Muted, Left + 54.0f, Top + 424.0f * ContentScale,
        SmallFont, 0.76f * TextScale, false);
    DrawText(
        TEXT("ENTER applies changed choices atomically; an unchanged setup resumes the paused field. On failure, the prior match is restored."),
        Body, Left + 54.0f, Top + 474.0f * ContentScale,
        SmallFont, 0.78f * TextScale, false);
    DrawRect(
        FLinearColor(Muted.R, Muted.G, Muted.B, 0.22f),
        Layout.BackButton.Min.X,
        Layout.BackButton.Min.Y,
        Layout.BackButton.GetSize().X,
        Layout.BackButton.GetSize().Y);
    DrawLine(
        Layout.BackButton.Min.X,
        Layout.BackButton.Max.Y,
        Layout.BackButton.Max.X,
        Layout.BackButton.Max.Y,
        Muted,
        1.5f);
    DrawText(TEXT("ESCAPE / GAMEPAD B / CLICK: ADJUST SETUP"), Muted,
             Layout.BackButton.Min.X + 12.0f * ContentScale,
             Layout.BackButton.Min.Y + 9.0f * ContentScale,
             SmallFont, 0.80f * TextScale, false);

    DrawRect(Accent,
             Layout.DeployButton.Min.X,
             Layout.DeployButton.Min.Y,
             Layout.DeployButton.GetSize().X,
             Layout.DeployButton.GetSize().Y);
    DrawText(TEXT("ENTER / GAMEPAD A: DEPLOY"),
             Theme.ActionText,
             Left + PanelWidth * 0.5f - 144.0f * TextScale,
             Top + PanelHeight - 65.0f,
             SmallFont, 0.94f * TextScale, false);
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
    const FEchoesVisualTheme Theme =
        UEchoesVisualThemeSettings::Resolve(bHighContrast);
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
    const FLinearColor Accent = Theme.Accent;
    const FLinearColor Body = Theme.TextPrimary;
    const FLinearColor Muted = Theme.TextSecondary;
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
    const FString SkirmishMapName =
        ActiveSkirmishMapDisplayName(Bridge);
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
    const bool bShapeBesideUs = Bridge != nullptr &&
        Bridge->GetOperationMode() ==
            EEchoesOperationMode::CampaignShapeBesideUs;
    const bool bReserveAuthority = Bridge != nullptr &&
        Bridge->GetOperationMode() ==
            EEchoesOperationMode::CampaignReserveAuthority;
    const bool bChoirAtLumeReach = Bridge != nullptr &&
        Bridge->GetOperationMode() ==
            EEchoesOperationMode::CampaignChoirAtLumeReach;
    const bool bNoNeutralLedger = Bridge != nullptr &&
        Bridge->GetOperationMode() ==
            EEchoesOperationMode::CampaignNoNeutralLedger;
    const bool bFutureThatWon = Bridge != nullptr &&
        Bridge->GetOperationMode() ==
            EEchoesOperationMode::CampaignFutureThatWon;
    const bool bAssemblyOfTheMissing = Bridge != nullptr &&
        Bridge->GetOperationMode() ==
            EEchoesOperationMode::CampaignAssemblyOfTheMissing;
    const bool bSeveralVoicesOneCommand = Bridge != nullptr &&
        Bridge->GetOperationMode() ==
            EEchoesOperationMode::CampaignSeveralVoicesOneCommand;
    const bool bBrokenSun = Bridge != nullptr &&
        Bridge->GetOperationMode() ==
            EEchoesOperationMode::CampaignTheBrokenSun;
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
    const FEchoesCampaignJourney CampaignJourney = Bridge != nullptr
        ? Bridge->GetCampaignJourney()
        : FEchoesCampaignJourney{};
    const bool bNewCampaignArmed =
        EchoesController->IsNewCampaignConfirmationArmed();
    const bool bCampaignRestoreArmed =
        EchoesController->IsCampaignRestoreConfirmationArmed();

    DrawRect(Theme.Canvas, 0.0f, 0.0f, Canvas->ClipX, Canvas->ClipY);
    const FBox2D ScreenBounds(
        FVector2D::ZeroVector,
        FVector2D(Canvas->ClipX, Canvas->ClipY));
    const FBox2D PanelBounds(
        FVector2D(Left, Top),
        FVector2D(Left + PanelWidth, Top + PanelHeight));
    DrawShatteredSunMotif(
        ScreenBounds, Theme, Settings, bHighContrast ? 0.22f : 0.10f);
    DrawVisualPanel(PanelBounds, Theme, true);
    DrawShatteredSunMotif(
        PanelBounds, Theme, Settings, bHighContrast ? 0.10f : 0.045f);
    DrawFactionSigil(
        VisualFactionFromLabel(LocalFaction),
        FVector2D(Left + PanelWidth - 82.0f * ContentScale,
                  Top + 84.0f * ContentScale),
        30.0f * ContentScale,
        Theme,
        0.92f);

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
             : bShapeBesideUs ? TEXT("THE SHAPE BESIDE US")
             : bReserveAuthority ? TEXT("RESERVE AUTHORITY")
             : bChoirAtLumeReach ? TEXT("THE CHOIR AT LUME REACH")
             : bNoNeutralLedger ? TEXT("NO NEUTRAL LEDGER")
             : bFutureThatWon ? TEXT("THE FUTURE THAT WON")
             : bAssemblyOfTheMissing ? TEXT("ASSEMBLY OF THE MISSING")
             : bSeveralVoicesOneCommand ? TEXT("SEVERAL VOICES, ONE COMMAND")
             : bBrokenSun ? TEXT("THE BROKEN SUN")
                              : *SkirmishMapName, Body,
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
        : bShapeBesideUs
            ? FString::Printf(
                  TEXT("CAMPAIGN MISSION 08  //  %s  //  TALAR + STATE WITNESSES"),
                  *LocalFaction)
        : bReserveAuthority
            ? FString::Printf(
                  TEXT("CAMPAIGN MISSION 09  //  %s  //  MARA + DISTRICT NETWORK"),
                  *LocalFaction)
        : bChoirAtLumeReach
            ? FString::Printf(
                  TEXT("CAMPAIGN MISSION 10  //  %s  //  ORUUN + LISTENING FORCE"),
                  *LocalFaction)
        : bNoNeutralLedger
            ? FString::Printf(
                  TEXT("CAMPAIGN MISSION 11  //  %s  //  ORUUN + LEDGER WITNESS"),
                  *LocalFaction)
        : bFutureThatWon
            ? FString::Printf(
                  TEXT("CAMPAIGN MISSION 12  //  %s  //  ORUUN + INDEPENDENT VERIFIER"),
                  *LocalFaction)
        : bAssemblyOfTheMissing
            ? FString::Printf(
                  TEXT("CAMPAIGN MISSION 13  //  %s  //  ORUUN + INDEPENDENT VERIFIER"),
                  *LocalFaction)
        : bSeveralVoicesOneCommand
            ? FString::Printf(
                  TEXT("CAMPAIGN MISSION 14  //  %s  //  NEME + PROTECTED VOICES"),
                  *LocalFaction)
        : bBrokenSun
            ? FString::Printf(
                  TEXT("CAMPAIGN MISSION 15  //  %s  //  FINAL ACCORD + NAMED WITNESSES"),
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
    : bShapeBesideUs
        ? TEXT("[F9] CHANGE OPERATION  //  MERIDIAN COMPACT  //  MISSION 08")
    : bReserveAuthority
        ? TEXT("[F9] CHANGE OPERATION  //  MERIDIAN AUTHORITY  //  MISSION 09")
    : bChoirAtLumeReach
        ? TEXT("[F9] CHANGE OPERATION  //  KHARUUN AUTHORITY  //  MISSION 10")
    : bNoNeutralLedger
        ? TEXT("[F9] CHANGE OPERATION  //  KHARUUN AUTHORITY  //  MISSION 11")
    : bFutureThatWon
        ? TEXT("[F9] CHANGE OPERATION  //  KHARUUN AUTHORITY  //  MISSION 12")
    : bAssemblyOfTheMissing
        ? TEXT("[F9] CHANGE OPERATION  //  KHARUUN AUTHORITY  //  MISSION 13")
    : bSeveralVoicesOneCommand
        ? TEXT("[F9] CHANGE OPERATION  //  HOLLOW CHOIR AUTHORITY  //  MISSION 14")
    : bBrokenSun
        ? TEXT("[F9] CHANGE OPERATION  //  HOLLOW CHOIR AUTHORITY  //  MISSION 15")
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
    if (CampaignJourney.State == EEchoesCampaignJourneyState::Ready)
    {
        CampaignControlLine += FString::Printf(
            TEXT("  //  [C] CONTINUE M%02d %s"),
            CampaignJourney.CompletedMissionCount + 1,
            FEchoesCampaignJourneyModel::OperationDisplayName(
                CampaignJourney.NextOperation));
    }
    else if (CampaignJourney.State == EEchoesCampaignJourneyState::Complete)
    {
        CampaignControlLine += TEXT("  //  COMPLETE 15/15");
    }
    else
    {
        CampaignControlLine += TEXT("  //  CONTINUATION UNAVAILABLE");
    }
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
        : bShapeBesideUs
            ? TEXT("Seven consistent records let Neme answer Talar with two routes that remain true at once.")
        : bReserveAuthority
            ? TEXT("Eight consistent records grant Mara a finite reserve for three failing districts.")
        : bChoirAtLumeReach
            ? TEXT("Nine consistent records carry Oruun to Lume Reach, where Mara serves as an off-map liaison to a public local contact.")
        : bNoNeutralLedger
            ? TEXT("Ten ordered records admit one local coalition route. Mission 01, Mission 09, and Mission 10 select its inherited geometry and protocol.")
        : bFutureThatWon
            ? TEXT("Eleven ordered records bind one local restoration demonstrator to the founding doctrine, recorded district pair, and exact Lume protocol.")
        : bAssemblyOfTheMissing
            ? TEXT("Twelve ordered records bind three neutral public interfaces to the existing plan without adding hidden authorship, trust, or mixed-faction command.")
        : bSeveralVoicesOneCommand
            ? TEXT("Thirteen ordered records grant one bounded Hollow Choir command perspective while retaining the exact inherited protocol context.")
        : bBrokenSun
            ? TEXT("Fourteen ordered records admit the final operation. Prior doctrine, district allocation, and Lume protocol determine the explicit earned resolution set.")
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
        : bShapeBesideUs
            ? TEXT("Observe the first echo, raise its relay, traverse both states, then reach the convergence.")
        : bReserveAuthority
            ? TEXT("Secure authority, power exactly two districts, then reach the intact deferred district.")
        : bChoirAtLumeReach
            ? TEXT("Resolve the deferred liability, raise both Listening Spines, commit the Lume Well, then reach its branch resolution.")
        : bNoNeutralLedger
            ? TEXT("Secure the route, integrate both contributing districts, attest both public evidence channels, apply the recorded protocol, then rally.")
        : bFutureThatWon
            ? TEXT("Establish independent readback, verify both district inputs, activate the recorded protocol, hold stability, then observe both readbacks.")
        : bAssemblyOfTheMissing
            ? TEXT("Establish paired public-record readback, link the Crownfall index, then observe the assembly from two independent witness sites.")
        : bSeveralVoicesOneCommand
            ? TEXT("Resolve protected voices into incompatible states, research their shared use, then raise a Phase Anchor and hold the crisis for 8.0 seconds.")
        : bBrokenSun
            ? TEXT("Secure the approach, assemble the witnessed accord, confirm one earned resolution, then raise its distinct conduit and hold the exact final contract.")
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
    DrawText(
             CampaignJourney.State == EEchoesCampaignJourneyState::Ready
                 ? TEXT("ENTER: OPEN SELECTED BRIEF   //   C: CONTINUE CAMPAIGN")
             : CampaignJourney.State == EEchoesCampaignJourneyState::Complete
                 ? TEXT("ENTER: OPEN SELECTED BRIEF   //   CAMPAIGN COMPLETE")
                 : TEXT("PRESS ENTER TO OPEN THE OPERATIONS BRIEF"),
             Theme.ActionText,
             Left + PanelWidth * 0.5f - 236.0f * TextScale,
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
    const FEchoesVisualTheme Theme =
        UEchoesVisualThemeSettings::Resolve(bHighContrast);
    const FEchoesHudLayout Layout = FEchoesHudLayout::Build(
        FVector2D(Canvas->ClipX, Canvas->ClipY), HudScale, false);
    if (!Layout.bObjectiveVisible)
    {
        return;
    }
    const float PanelWidth = Layout.ObjectivePanel.GetSize().X;
    const float PanelHeight = Layout.ObjectivePanel.GetSize().Y;
    const float Left = Layout.ObjectivePanel.Min.X;
    const float Top = Layout.ObjectivePanel.Min.Y;

    const FLinearColor Backdrop = Theme.Surface;
    const FLinearColor Accent = Theme.Accent;
    const FLinearColor Active = Theme.TextPrimary;
    const FLinearColor Complete = Theme.Success;
    const FLinearColor Failed = Theme.Danger;
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
        FString PlayerLinkSites;
        for (const auto& Site : Plan.PlayerPowerLinkSites)
        {
            if (!PlayerLinkSites.IsEmpty())
            {
                PlayerLinkSites += TEXT(", ");
            }
            PlayerLinkSites += FString::Printf(
                TEXT("%d,%d"),
                Site.x.FloorToInt(),
                Site.y.FloorToInt());
        }
        const FString NetworkState = bFailed
            ? TEXT("INTERFACES LOST")
            : bNetworksReady
                ? TEXT("SYNCHRONIZED")
                : FString::Printf(
                      TEXT("BUILD P %s  //  MERIDIAN %s  KHARUUN %s"),
                      *PlayerLinkSites,
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

    if (Objective.OperationMode ==
        EEchoesOperationMode::CampaignShapeBesideUs)
    {
        const FEchoesShapeBesideUsPlan Plan =
            Bridge->GetShapeBesideUsPlan();
        const bool bFailed =
            Objective.ShapeBesideUsPhase ==
            EEchoesShapeBesideUsPhase::Failed;
        const bool bStatesTraversed =
            Objective.bFirstStateTraversed &&
            Objective.bSecondStateTraversed;
        const FString EchoState = bFailed
            ? TEXT("CONTACT LOST")
            : Objective.bFirstEchoObserved
                ? TEXT("OBSERVED")
                : FString::Printf(
                      TEXT("TALAR TO %d,%d"),
                      Plan.FirstEchoSite.x.FloorToInt(),
                      Plan.FirstEchoSite.y.FloorToInt());
        const FString RelayState = bFailed
            ? TEXT("RELAY LOST")
            : Objective.bEchoRelayRaised
                ? TEXT("RAISED")
                : Objective.bFirstEchoObserved
                    ? FString::Printf(
                          TEXT("BUILD AT %d,%d"),
                          Plan.EchoRelaySite.x.FloorToInt(),
                          Plan.EchoRelaySite.y.FloorToInt())
                    : TEXT("WAITING — OBSERVE ECHO");
        const FString ConvergenceState = bFailed
            ? TEXT("OVERLAP LOST")
            : Objective.bShapeBesideUsTalarAtConvergence
                ? TEXT("RECIPROCAL CONTACT RECORDED")
                : bStatesTraversed
                    ? FString::Printf(
                          TEXT("TALAR TO %d,%d"),
                          Plan.ConvergenceSite.x.FloorToInt(),
                          Plan.ConvergenceSite.y.FloorToInt())
                : Objective.bEchoRelayRaised
                    ? TEXT("POSITION BOTH STATE WITNESSES")
                    : TEXT("WAITING — RAISE RELAY");
        DrawRect(Backdrop, Left, Top, PanelWidth, PanelHeight);
        DrawLine(Left, Top, Left + PanelWidth, Top, Accent, 2.0f);
        DrawText(TEXT("THE SHAPE BESIDE US  //  MISSION 08"), Accent,
                 Left + 18.0f, Top + 15.0f, SmallFont,
                 0.90f * TextScale, false);
        DrawText(
            FString::Printf(TEXT("01  FIRST ECHO        %s"), *EchoState),
            bFailed ? Failed : Objective.bFirstEchoObserved ? Complete : Active,
            Left + 18.0f, Top + 52.0f, SmallFont,
            0.80f * TextScale, false);
        DrawText(
            FString::Printf(TEXT("02  ECHO RELAY        %s"), *RelayState),
            bFailed ? Failed : Objective.bEchoRelayRaised ? Complete : Active,
            Left + 18.0f, Top + 89.0f, SmallFont,
            0.80f * TextScale, false);
        DrawText(
            FString::Printf(TEXT("03  PAIRED CONVERGENCE %s"), *ConvergenceState),
            bFailed ? Failed
                    : Objective.bShapeBesideUsTalarAtConvergence
                        ? Complete : Active,
            Left + 18.0f, Top + 126.0f, SmallFont,
            0.80f * TextScale, false);
        if (!bLoggedObjectiveTrackerReady)
        {
            bLoggedObjectiveTrackerReady = true;
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_SHAPE_BESIDE_US_OBJECTIVES_READY] phase=%s branch=%s talar=%u witnessA=%u witnessB=%u reconstructable=true claimBoundary=reciprocalContactOnly hollowChoirFactionImplemented=false"),
                FEchoesShapeBesideUsMissionModel::StableName(
                    Objective.ShapeBesideUsPhase),
                Plan.StableName,
                Objective.ShapeBesideUsTalarId,
                Objective.FirstStateWitnessId,
                Objective.SecondStateWitnessId);
        }
        return;
    }

    if (Objective.OperationMode ==
        EEchoesOperationMode::CampaignReserveAuthority)
    {
        const FEchoesReserveAuthorityPlan Plan =
            Bridge->GetReserveAuthorityPlan();
        const bool bFailed =
            Objective.ReserveAuthorityPhase ==
            EEchoesReserveAuthorityPhase::Failed;
        const int32 PoweredCount =
            (Objective.bLifeSupportPowered ? 1 : 0) +
            (Objective.bTransitPowered ? 1 : 0) +
            (Objective.bArchivePowered ? 1 : 0);
        const bool bAllocationReady = PoweredCount == 2;
        const EEchoesCityDistrict Deferred =
            Objective.ReserveAuthorityDeferredDistrict;
        const bool bLifeDeferred = bAllocationReady &&
            Deferred == EEchoesCityDistrict::LifeSupport;
        const bool bTransitDeferred = bAllocationReady &&
            Deferred == EEchoesCityDistrict::Transit;
        const bool bArchiveDeferred = bAllocationReady &&
            Deferred == EEchoesCityDistrict::Archive;
        const FString AuthorityState = bFailed
            ? TEXT("AUTHORITY LOST")
            : Objective.bReserveAuthoritySecured
                ? TEXT("SECURED")
                : FString::Printf(
                      TEXT("MARA TO %d,%d"),
                      Plan.AuthoritySite.x.FloorToInt(),
                      Plan.AuthoritySite.y.FloorToInt());
        const FString AllocationState = bFailed
            ? TEXT("RESERVE FAILED")
            : FString::Printf(
                  TEXT("%d/2  L:%s  T:%s  A:%s"),
                  PoweredCount,
                  Objective.bLifeSupportPowered
                      ? TEXT("POWER")
                      : bLifeDeferred ? TEXT("DEFER") : TEXT("OPEN"),
                  Objective.bTransitPowered
                      ? TEXT("POWER")
                      : bTransitDeferred ? TEXT("DEFER") : TEXT("OPEN"),
                  Objective.bArchivePowered
                      ? TEXT("POWER")
                      : bArchiveDeferred ? TEXT("DEFER") : TEXT("OPEN"));
        const FString DeferredState = bFailed
            ? TEXT("DISTRICT LINE LOST")
            : Objective.bReserveAuthorityMaraAtDeferredDistrict
                ? TEXT("INTACT — DECISION RECORDED")
                : bAllocationReady
                    ? FString::Printf(
                          TEXT("MARA TO %s"),
                          FEchoesCityReserveMissionModel::DistrictDisplayName(
                              Deferred))
                    : TEXT("WAITING — POWER EXACTLY TWO");
        DrawRect(Backdrop, Left, Top, PanelWidth, PanelHeight);
        DrawLine(Left, Top, Left + PanelWidth, Top, Accent, 2.0f);
        DrawText(TEXT("RESERVE AUTHORITY  //  MISSION 09"), Accent,
                 Left + 18.0f, Top + 15.0f, SmallFont,
                 0.90f * TextScale, false);
        DrawText(
            FString::Printf(TEXT("01  AUTHORITY SITE    %s"), *AuthorityState),
            bFailed ? Failed
                    : Objective.bReserveAuthoritySecured ? Complete : Active,
            Left + 18.0f, Top + 52.0f, SmallFont,
            0.80f * TextScale, false);
        DrawText(
            FString::Printf(TEXT("02  DISTRICT RESERVE  %s"), *AllocationState),
            bFailed ? Failed : bAllocationReady ? Complete : Active,
            Left + 18.0f, Top + 89.0f, SmallFont,
            0.80f * TextScale, false);
        DrawText(
            FString::Printf(TEXT("03  DEFERRED DISTRICT %s"), *DeferredState),
            bFailed ? Failed
                    : Objective.bReserveAuthorityMaraAtDeferredDistrict
                        ? Complete : Active,
            Left + 18.0f, Top + 126.0f, SmallFont,
            0.80f * TextScale, false);
        if (!bLoggedObjectiveTrackerReady)
        {
            bLoggedObjectiveTrackerReady = true;
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_RESERVE_AUTHORITY_OBJECTIVES_READY] phase=%s branch=%s mara=%u powered=%d deferred=%u reconstructable=true claimBoundary=localAllocationOnly widerCityRestored=false"),
                FEchoesReserveAuthorityMissionModel::StableName(
                    Objective.ReserveAuthorityPhase),
                Plan.StableName,
                Objective.ReserveAuthorityMaraId,
                PoweredCount,
                static_cast<uint8>(Deferred));
        }
        return;
    }

    if (Objective.OperationMode ==
        EEchoesOperationMode::CampaignChoirAtLumeReach)
    {
        const FEchoesChoirAtLumeReachPlan Plan =
            Bridge->GetChoirAtLumeReachPlan();
        const bool bFailed =
            Objective.ChoirAtLumeReachPhase ==
            EEchoesChoirAtLumeReachPhase::Failed;
        const int32 AnchorCount =
            (Objective.bChoirFirstAnchorRaised ? 1 : 0) +
            (Objective.bChoirSecondAnchorRaised ? 1 : 0);
        const bool bWellCommitted =
            Objective.ChoirAtLumeReachWellChoice !=
            echoes::sim::FutureWellChoice::Dormant;
        const FString ContactState = bFailed
            ? TEXT("CONTACT LINE LOST")
            : Objective.bChoirDeferredLiabilityResolved
                ? FString::Printf(
                      TEXT("WAYSTONE ROOTED — %s LIABILITY"),
                      FEchoesCityReserveMissionModel::DistrictDisplayName(
                          Plan.DeferredDistrict))
            : Objective.bChoirContactEstablished
                ? FString::Printf(
                      TEXT("ROOT WAYSTONE AT %d,%d"),
                      Plan.LiabilitySite.x.FloorToInt(),
                      Plan.LiabilitySite.y.FloorToInt())
                : FString::Printf(
                      TEXT("ORUUN TO CONTACT %d,%d"),
                      Plan.ContactSite.x.FloorToInt(),
                      Plan.ContactSite.y.FloorToInt());
        const FString AnchorState = bFailed
            ? TEXT("LISTENING LINE LOST")
            : AnchorCount == 2
                ? TEXT("2/2 LISTENING")
            : Objective.bChoirDeferredLiabilityResolved
                ? FString::Printf(
                      TEXT("%d/2 — BUILD AT %d,%d"),
                      AnchorCount,
                      AnchorCount == 0
                          ? Plan.FirstAnchorSite.x.FloorToInt()
                          : Plan.SecondAnchorSite.x.FloorToInt(),
                      AnchorCount == 0
                          ? Plan.FirstAnchorSite.y.FloorToInt()
                          : Plan.SecondAnchorSite.y.FloorToInt())
                : TEXT("WAITING — ROOT WAYSTONE");
        const echoes::sim::Vec2 ResolutionSite =
            FEchoesChoirAtLumeReachMissionModel::ResolutionSiteForChoice(
                Objective.ChoirAtLumeReachWellChoice);
        const FString ResolutionState = bFailed
            ? Objective.bChoirReshapeWindowExpired
                ? TEXT("RESHAPE EXIT EXPIRED")
                : TEXT("WELL ROUTE LOST")
            : Objective.bChoirBranchResolutionCompleted
                ? FString::Printf(
                      TEXT("%s — RESOLVED"),
                      WellChoiceDisplayName(
                          Objective.ChoirAtLumeReachWellChoice))
            : bWellCommitted
                ? FString::Printf(
                      TEXT("%s — ORUUN TO %d,%d"),
                      WellChoiceDisplayName(
                          Objective.ChoirAtLumeReachWellChoice),
                      ResolutionSite.x.FloorToInt(),
                      ResolutionSite.y.FloorToInt())
            : AnchorCount == 2
                ? FString::Printf(
                      TEXT("Z/C/V — WELL %d,%d"),
                      Plan.FutureWellSite.x.FloorToInt(),
                      Plan.FutureWellSite.y.FloorToInt())
                : TEXT("WAITING — RAISE BOTH SPINES");
        DrawRect(Backdrop, Left, Top, PanelWidth, PanelHeight);
        DrawLine(Left, Top, Left + PanelWidth, Top, Accent, 2.0f);
        DrawText(TEXT("THE CHOIR AT LUME REACH  //  MISSION 10"), Accent,
                 Left + 18.0f, Top + 15.0f, SmallFont,
                 0.90f * TextScale, false);
        DrawText(
            FString::Printf(TEXT("01  CONTACT + LIABILITY %s"), *ContactState),
            bFailed ? Failed
                    : Objective.bChoirDeferredLiabilityResolved
                        ? Complete : Active,
            Left + 18.0f, Top + 52.0f, SmallFont,
            0.76f * TextScale, false);
        DrawText(
            FString::Printf(TEXT("02  LISTENING ANCHORS   %s"), *AnchorState),
            bFailed ? Failed : AnchorCount == 2 ? Complete : Active,
            Left + 18.0f, Top + 89.0f, SmallFont,
            0.76f * TextScale, false);
        DrawText(
            FString::Printf(TEXT("03  WELL + RESOLUTION   %s"), *ResolutionState),
            bFailed ? Failed
                    : Objective.bChoirBranchResolutionCompleted
                        ? Complete : Active,
            Left + 18.0f, Top + 126.0f, SmallFont,
            0.76f * TextScale, false);
        if (!bLoggedObjectiveTrackerReady)
        {
            bLoggedObjectiveTrackerReady = true;
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_CHOIR_AT_LUME_REACH_OBJECTIVES_READY] phase=%s approach=%s priorBranch=%u deferred=%u oruun=%u waystone=%u well=%u reconstructable=true maraPresence=liaisonOnly choirPresence=nonPlayablePublicContact mixedFactionCommand=false"),
                FEchoesChoirAtLumeReachMissionModel::StableName(
                    Objective.ChoirAtLumeReachPhase),
                Plan.StableName,
                static_cast<uint8>(Plan.PriorChoice),
                static_cast<uint8>(Plan.DeferredDistrict),
                Objective.ChoirAtLumeReachOruunId,
                Objective.ChoirAtLumeReachWaystoneId,
                Objective.ChoirAtLumeReachWellId);
        }
        return;
    }

    if (Objective.OperationMode ==
        EEchoesOperationMode::CampaignNoNeutralLedger)
    {
        const FEchoesNoNeutralLedgerPlan Plan =
            Bridge->GetNoNeutralLedgerPlan();
        const bool bFailed =
            Objective.NoNeutralLedgerPhase ==
            EEchoesNoNeutralLedgerPhase::Failed;
        const int32 DistrictCount =
            (Objective.bNoNeutralFirstDistrictIntegrated ? 1 : 0) +
            (Objective.bNoNeutralSecondDistrictIntegrated ? 1 : 0);
        const FString RouteAndDistrictState = bFailed
            ? TEXT("ROUTE OR DISTRICT LINE LOST")
            : DistrictCount == 2
                ? TEXT("ROUTE SECURE — DISTRICTS 2/2")
            : Objective.bNoNeutralRouteSecured
                ? FString::Printf(
                      TEXT("DISTRICTS %d/2 — LINK NEAR %d,%d"),
                      DistrictCount,
                      !Objective.bNoNeutralFirstDistrictIntegrated
                          ? Plan.FirstDistrictSite.x.FloorToInt()
                          : Plan.SecondDistrictSite.x.FloorToInt(),
                      !Objective.bNoNeutralFirstDistrictIntegrated
                          ? Plan.FirstDistrictSite.y.FloorToInt()
                          : Plan.SecondDistrictSite.y.FloorToInt())
                : FString::Printf(
                      TEXT("ROOT WAYSTONE AT %d,%d"),
                      Plan.RouteSite.x.FloorToInt(),
                      Plan.RouteSite.y.FloorToInt());
        const FString EvidenceState = bFailed
            ? TEXT("PUBLIC CHANNELS LOST")
            : Objective.bNoNeutralEvidenceAttested
                ? TEXT("MERIDIAN + KHARUUN ATTESTED")
            : DistrictCount == 2
                ? FString::Printf(
                      TEXT("ORUUN %d,%d + WITNESS %d,%d"),
                      Plan.KharuunEvidenceSite.x.FloorToInt(),
                      Plan.KharuunEvidenceSite.y.FloorToInt(),
                      Plan.MeridianEvidenceSite.x.FloorToInt(),
                      Plan.MeridianEvidenceSite.y.FloorToInt())
                : TEXT("WAITING — INTEGRATE 2 DISTRICTS");
        const FString ProtocolAndRallyState = bFailed
            ? Objective.bNoNeutralReshapeWindowExpired
                ? TEXT("RESHAPE RALLY WINDOW EXPIRED")
                : TEXT("COALITION RECORD LOST")
            : Objective.bNoNeutralCoalitionRallied
                ? FString::Printf(
                      TEXT("%s — COALITION RALLIED"),
                      Plan.ProtocolDisplayName)
            : Objective.bNoNeutralProtocolApplied
                ? FString::Printf(
                      TEXT("%s — RALLY %d,%d"),
                      Plan.ProtocolDisplayName,
                      Plan.RallySite.x.FloorToInt(),
                      Plan.RallySite.y.FloorToInt())
            : Objective.bNoNeutralEvidenceAttested
                ? FString::Printf(
                      TEXT("APPLY RECORDED %s AT %d,%d"),
                      Plan.ProtocolDisplayName,
                      Plan.FutureWellSite.x.FloorToInt(),
                      Plan.FutureWellSite.y.FloorToInt())
                : TEXT("WAITING — ATTEST BOTH CHANNELS");

        DrawRect(Backdrop, Left, Top, PanelWidth, PanelHeight);
        DrawLine(Left, Top, Left + PanelWidth, Top, Accent, 2.0f);
        DrawText(TEXT("NO NEUTRAL LEDGER  //  MISSION 11"), Accent,
                 Left + 18.0f, Top + 15.0f, SmallFont,
                 0.90f * TextScale, false);
        DrawText(
            FString::Printf(TEXT("01  ROUTE + DISTRICTS  %s"),
                            *RouteAndDistrictState),
            bFailed ? Failed : DistrictCount == 2 ? Complete : Active,
            Left + 18.0f, Top + 52.0f, SmallFont,
            0.72f * TextScale, false);
        DrawText(
            FString::Printf(TEXT("02  EVIDENCE CHANNELS %s"),
                            *EvidenceState),
            bFailed ? Failed
                    : Objective.bNoNeutralEvidenceAttested
                        ? Complete : Active,
            Left + 18.0f, Top + 89.0f, SmallFont,
            0.72f * TextScale, false);
        DrawText(
            FString::Printf(TEXT("03  PROTOCOL + RALLY  %s"),
                            *ProtocolAndRallyState),
            bFailed ? Failed
                    : Objective.bNoNeutralCoalitionRallied
                        ? Complete : Active,
            Left + 18.0f, Top + 126.0f, SmallFont,
            0.72f * TextScale, false);
        if (!bLoggedObjectiveTrackerReady)
        {
            bLoggedObjectiveTrackerReady = true;
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_NO_NEUTRAL_LEDGER_OBJECTIVES_READY] phase=%s plan=%u foundingDoctrine=%u districtPair=%u+%u deferred=%u lumeProtocol=%u oruun=%u waystone=%u witness=%u districtInterfaces=%u:%u evidenceInterfaces=%u:%u well=%u reconstructable=true exactOrderedLedger=true commandAuthority=Kharuun meridianInterfaces=neutralPoweredPublicNonCommandable kharuunEvidenceInterface=neutralPublicNonCommandable choirPresence=publicNonCommandable mixedFactionCommand=false hiddenTrustScore=false hiddenAttribution=false causationClaim=false"),
                FEchoesNoNeutralLedgerMissionModel::StableName(
                    Objective.NoNeutralLedgerPhase),
                Plan.StablePlanKey,
                static_cast<uint8>(Plan.FoundingDoctrine),
                static_cast<uint8>(Plan.FirstContributingDistrict),
                static_cast<uint8>(Plan.SecondContributingDistrict),
                static_cast<uint8>(Plan.DeferredDistrict),
                static_cast<uint8>(Plan.LumeProtocol),
                Objective.NoNeutralOruunId,
                Objective.NoNeutralWaystoneId,
                Objective.NoNeutralLedgerWitnessId,
                Objective.NoNeutralFirstDistrictInterfaceId,
                Objective.NoNeutralSecondDistrictInterfaceId,
                Objective.NoNeutralMeridianEvidenceInterfaceId,
                Objective.NoNeutralKharuunEvidenceInterfaceId,
                Objective.NoNeutralWellId);
        }
        return;
    }

    if (Objective.OperationMode ==
        EEchoesOperationMode::CampaignFutureThatWon)
    {
        const FEchoesFutureThatWonPlan Plan =
            Bridge->GetFutureThatWonPlan();
        const bool bFailed =
            Objective.FutureThatWonPhase ==
            EEchoesFutureThatWonPhase::Failed;
        const int32 InputCount =
            (Objective.bFutureWonFirstInputVerified ? 1 : 0) +
            (Objective.bFutureWonSecondInputVerified ? 1 : 0);
        const bool bBothReadbacksObserved =
            Objective.bFutureWonFirstDistrictReadbackObserved &&
            Objective.bFutureWonSecondDistrictReadbackObserved;
        const FString InputState = bFailed
            ? TEXT("RECORDED INPUT LINE LOST")
            : InputCount == 2
                ? TEXT("RECORDED DISTRICTS 2/2 VERIFIED")
            : Objective.bFutureWonIndependentReadbackEstablished
                ? FString::Printf(
                      TEXT("%d/2 — LINK NEAR %d,%d"),
                      InputCount,
                      !Objective.bFutureWonFirstInputVerified
                          ? Plan.FirstDistrictInputSite.x.FloorToInt()
                          : Plan.SecondDistrictInputSite.x.FloorToInt(),
                      !Objective.bFutureWonFirstInputVerified
                          ? Plan.FirstDistrictInputSite.y.FloorToInt()
                          : Plan.SecondDistrictInputSite.y.FloorToInt())
                : TEXT("WAITING — ESTABLISH READBACK");
        const FString ReadbackState = bFailed
            ? TEXT("PUBLIC APPARATUS LOST")
            : Objective.bFutureWonProtocolBound
                ? FString::Printf(
                      TEXT("RECORDED %s ACTIVE"),
                      Plan.ProtocolDisplayName)
            : Objective.bFutureWonIndependentReadbackEstablished &&
                    InputCount == 2
                ? FString::Printf(
                      TEXT("APPLY %s AT %d,%d"),
                      Plan.ProtocolDisplayName,
                      Plan.FutureWellSite.x.FloorToInt(),
                      Plan.FutureWellSite.y.FloorToInt())
                : FString::Printf(
                      TEXT("ORUUN %d,%d + VERIFIER %d,%d"),
                      Plan.KharuunReadbackSite.x.FloorToInt(),
                      Plan.KharuunReadbackSite.y.FloorToInt(),
                      Plan.MeridianReadbackSite.x.FloorToInt(),
                      Plan.MeridianReadbackSite.y.FloorToInt());
        const FString StabilityState = bFailed
            ? TEXT("DEMONSTRATION CONTRACT LOST")
            : bBothReadbacksObserved
                ? TEXT("WINDOW HELD — BOTH READBACKS OBSERVED")
            : Objective.bFutureWonStabilityWindowHeld
                ? FString::Printf(
                      TEXT("OBSERVE DISTRICTS %d,%d + %d,%d"),
                      Plan.FirstDistrictInputSite.x.FloorToInt(),
                      Plan.FirstDistrictInputSite.y.FloorToInt(),
                      Plan.SecondDistrictInputSite.x.FloorToInt(),
                      Plan.SecondDistrictInputSite.y.FloorToInt())
            : Objective.bFutureWonProtocolBound
                ? FString::Printf(
                      TEXT("HOLD THROUGH T%llu"),
                      static_cast<unsigned long long>(
                          Objective.FutureWonStabilityEndTick))
                : TEXT("WAITING — BIND RECORDED PROTOCOL");

        DrawRect(Backdrop, Left, Top, PanelWidth, PanelHeight);
        DrawLine(Left, Top, Left + PanelWidth, Top, Accent, 2.0f);
        DrawText(TEXT("THE FUTURE THAT WON  //  MISSION 12"), Accent,
                 Left + 18.0f, Top + 15.0f, SmallFont,
                 0.90f * TextScale, false);
        DrawText(
            FString::Printf(TEXT("01  RECORDED INPUTS   %s"), *InputState),
            bFailed ? Failed : InputCount == 2 ? Complete : Active,
            Left + 18.0f, Top + 52.0f, SmallFont,
            0.70f * TextScale, false);
        DrawText(
            FString::Printf(TEXT("02  PUBLIC READBACK   %s"), *ReadbackState),
            bFailed ? Failed
                    : Objective.bFutureWonProtocolBound ? Complete : Active,
            Left + 18.0f, Top + 89.0f, SmallFont,
            0.70f * TextScale, false);
        DrawText(
            FString::Printf(TEXT("03  STABILITY + OBS.  %s"), *StabilityState),
            bFailed ? Failed : bBothReadbacksObserved ? Complete : Active,
            Left + 18.0f, Top + 126.0f, SmallFont,
            0.70f * TextScale, false);
        if (!bLoggedObjectiveTrackerReady)
        {
            bLoggedObjectiveTrackerReady = true;
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_FUTURE_THAT_WON_OBJECTIVES_READY] phase=%s plan=%u foundingDoctrine=%u districtPair=%u+%u deferred=%u protocol=%u oruun=%u verifier=%u districtInterfaces=%u:%u readbackInterfaces=%u:%u demonstrator=%u well=%u stabilityTicks=%llu reconstructable=true exactOrderedLedger=true commandAuthority=Kharuun rhysePresence=attributablePublicApparatusOnly mixedFactionCommand=false civilianCountsUnmodeled=true populationRestorationUnproven=true permanentFutureUnproven=true ethicalJustificationUnproven=true"),
                FEchoesFutureThatWonMissionModel::StableName(
                    Objective.FutureThatWonPhase),
                Plan.StablePlanKey,
                static_cast<uint8>(Plan.FoundingDoctrine),
                static_cast<uint8>(Plan.FirstContributingDistrict),
                static_cast<uint8>(Plan.SecondContributingDistrict),
                static_cast<uint8>(Plan.DeferredDistrict),
                static_cast<uint8>(Plan.RecordedProtocol),
                Objective.FutureWonOruunId,
                Objective.FutureWonVerifierId,
                Objective.FutureWonFirstDistrictInterfaceId,
                Objective.FutureWonSecondDistrictInterfaceId,
                Objective.FutureWonMeridianReadbackInterfaceId,
                Objective.FutureWonKharuunReadbackInterfaceId,
                Objective.FutureWonDemonstratorInterfaceId,
                Objective.FutureWonWellId,
                static_cast<unsigned long long>(Plan.StabilityWindowTicks));
        }
        return;
    }

    if (Objective.OperationMode ==
        EEchoesOperationMode::CampaignAssemblyOfTheMissing)
    {
        const FEchoesAssemblyOfTheMissingPlan Plan =
            Bridge->GetAssemblyOfTheMissingPlan();
        const bool bFailed =
            Objective.AssemblyOfTheMissingPhase ==
            EEchoesAssemblyOfTheMissingPhase::Failed;
        const int32 ObservationCount =
            (Objective.bAssemblyMeridianWitnessObserved ? 1 : 0) +
            (Objective.bAssemblyKharuunWitnessObserved ? 1 : 0);
        const FString ReadbackState = bFailed
            ? TEXT("PUBLIC INTERFACE LINE LOST")
            : Objective.bAssemblyPublicRecordReadbackEstablished
                ? TEXT("PAIRED PUBLIC READBACK ESTABLISHED")
                : FString::Printf(
                      TEXT("ORUUN %d,%d + VERIFIER %d,%d"),
                      Plan.KharuunPublicRecordSite.x.FloorToInt(),
                      Plan.KharuunPublicRecordSite.y.FloorToInt(),
                      Plan.MeridianPublicRecordSite.x.FloorToInt(),
                      Plan.MeridianPublicRecordSite.y.FloorToInt());
        const FString IndexState = bFailed
            ? TEXT("CROWNFALL LINK CONTRACT LOST")
            : Objective.bAssemblyCrownfallIndexLinked
                ? TEXT("CROWNFALL PUBLIC INDEX LINKED")
            : Objective.bAssemblyPublicRecordReadbackEstablished
                ? FString::Printf(
                      TEXT("BUILD [N] WITHIN 3 TILES OF %d,%d"),
                      Plan.CrownfallIndexSite.x.FloorToInt(),
                      Plan.CrownfallIndexSite.y.FloorToInt())
                : TEXT("WAITING — ESTABLISH PUBLIC READBACK");
        const FString ObservationState = bFailed
            ? TEXT("INDEPENDENT OBSERVATION LOST")
            : ObservationCount == 2
                ? TEXT("BOTH PUBLIC ASSEMBLY VIEWS OBSERVED")
            : Objective.bAssemblyCrownfallIndexLinked
                ? FString::Printf(
                      TEXT("%d/2 — ORUUN %d,%d + VERIFIER %d,%d"),
                      ObservationCount,
                      Plan.KharuunAssemblyWitnessSite.x.FloorToInt(),
                      Plan.KharuunAssemblyWitnessSite.y.FloorToInt(),
                      Plan.MeridianAssemblyWitnessSite.x.FloorToInt(),
                      Plan.MeridianAssemblyWitnessSite.y.FloorToInt())
                : TEXT("WAITING — LINK CROWNFALL INDEX");

        DrawRect(Backdrop, Left, Top, PanelWidth, PanelHeight);
        DrawLine(Left, Top, Left + PanelWidth, Top, Accent, 2.0f);
        DrawText(TEXT("ASSEMBLY OF THE MISSING  //  MISSION 13"), Accent,
                 Left + 18.0f, Top + 15.0f, SmallFont,
                 0.90f * TextScale, false);
        DrawText(
            FString::Printf(TEXT("01  PUBLIC READBACK   %s"), *ReadbackState),
            bFailed ? Failed
                    : Objective.bAssemblyPublicRecordReadbackEstablished
                        ? Complete : Active,
            Left + 18.0f, Top + 52.0f, SmallFont,
            0.70f * TextScale, false);
        DrawText(
            FString::Printf(TEXT("02  CROWNFALL INDEX   %s"), *IndexState),
            bFailed ? Failed
                    : Objective.bAssemblyCrownfallIndexLinked
                        ? Complete : Active,
            Left + 18.0f, Top + 89.0f, SmallFont,
            0.70f * TextScale, false);
        DrawText(
            FString::Printf(TEXT("03  INDEPENDENT OBS.  %s"), *ObservationState),
            bFailed ? Failed : ObservationCount == 2 ? Complete : Active,
            Left + 18.0f, Top + 126.0f, SmallFont,
            0.70f * TextScale, false);
        if (!bLoggedObjectiveTrackerReady)
        {
            bLoggedObjectiveTrackerReady = true;
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_ASSEMBLY_OF_THE_MISSING_OBJECTIVES_READY] phase=%s plan=%u foundingDoctrine=%u districtPair=%u+%u deferred=%u protocol=%u oruun=%u verifier=%u publicRecordInterfaces=%u:%u crownfallIndex=%u reconstructable=true exactOrderedLedger=true commandAuthority=Kharuun publicInterfaces=neutralNonCommandable mixedFactionCommand=false responsibilityUnassigned=true hiddenAuthorshipUnproven=true trustUnproven=true consentUnproven=true civilianStateUnmodeled=true"),
                FEchoesAssemblyOfTheMissingMissionModel::StableName(
                    Objective.AssemblyOfTheMissingPhase),
                Plan.StablePlanKey,
                static_cast<uint8>(Plan.FoundingDoctrine),
                static_cast<uint8>(Plan.FirstContributingDistrict),
                static_cast<uint8>(Plan.SecondContributingDistrict),
                static_cast<uint8>(Plan.DeferredDistrict),
                static_cast<uint8>(Plan.RecordedProtocol),
                Objective.AssemblyOruunId,
                Objective.AssemblyVerifierId,
                Objective.AssemblyMeridianPublicRecordInterfaceId,
                Objective.AssemblyKharuunPublicRecordInterfaceId,
                Objective.AssemblyCrownfallIndexInterfaceId);
        }
        return;
    }

    if (Objective.OperationMode ==
        EEchoesOperationMode::CampaignSeveralVoicesOneCommand)
    {
        const FEchoesSeveralVoicesOneCommandPlan Plan =
            Bridge->GetSeveralVoicesOneCommandPlan();
        const bool bFailed =
            Objective.SeveralVoicesOneCommandPhase ==
            EEchoesSeveralVoicesOneCommandPhase::Failed;
        const echoes::sim::Simulation* MissionSimulation =
            Bridge->GetSimulation();
        const double TicksPerSecond =
            MissionSimulation != nullptr
                ? static_cast<double>(FMath::Max<uint32>(
                      1,
                      MissionSimulation->Config().ticksPerSecond))
                : 20.0;
        const double PossibleSeconds =
            static_cast<double>(
                Objective.SeveralVoicesPossibleResolveTicksRemaining) /
            TicksPerSecond;
        const double ManifestSeconds =
            static_cast<double>(
                Objective.SeveralVoicesManifestResolveTicksRemaining) /
            TicksPerSecond;
        const double CrisisSeconds =
            static_cast<double>(
                Objective.SeveralVoicesCrisisTicksRemaining) /
            TicksPerSecond;
        const FString PossibleTimer =
            Objective.SeveralVoicesPossibleResolveTicksRemaining > 0
                ? FString::Printf(TEXT(" %.1fs"), PossibleSeconds)
                : FString{};
        const FString ManifestTimer =
            Objective.SeveralVoicesManifestResolveTicksRemaining > 0
                ? FString::Printf(TEXT(" %.1fs"), ManifestSeconds)
                : FString{};
        const FString ResearchState = bFailed
            ? TEXT("RESEARCH OR PROTECTED LINE LOST")
            : Objective.bSeveralVoicesSharedResolutionResearched
                ? TEXT("HELD ALTERNATIVES + SHARED RESOLUTION")
            : Objective.bSeveralVoicesHeldAlternativesResearched
                ? Objective.bSeveralVoicesPossibleAtSite &&
                          Objective.bSeveralVoicesManifestAtSite &&
                          Objective.bSeveralVoicesNemeAtCommandSite
                    ? TEXT("F2 — RESEARCH SHARED RESOLUTION")
                    : TEXT("HELD ALTERNATIVES — VOICE CONTRACT PENDING")
                : TEXT("F2 — RESEARCH HELD ALTERNATIVES");
        const FString VoiceState = bFailed
            ? TEXT("INCOMPATIBLE VOICE CONTRACT LOST")
            : FString::Printf(
                  TEXT("P:%s%s%s  M:%s%s%s  N:%s"),
                  ChoirIdentityDisplayName(
                      Objective.SeveralVoicesPossibleState),
                  *PossibleTimer,
                  Objective.bSeveralVoicesPossibleAtSite
                      ? TEXT("@SITE") : TEXT(""),
                  ChoirIdentityDisplayName(
                      Objective.SeveralVoicesManifestState),
                  *ManifestTimer,
                  Objective.bSeveralVoicesManifestAtSite
                      ? TEXT("@SITE") : TEXT(""),
                  Objective.bSeveralVoicesNemeAtCommandSite
                      ? TEXT("COMMAND") : TEXT("MOVE"));
        const FString CrisisState = bFailed
            ? TEXT("PHASE ANCHOR CONTRACT FAILED")
            : Objective.bSeveralVoicesCrisisWindowHeld
                ? TEXT("PHASE ANCHOR HELD — CRISIS RESOLVED")
            : Objective.bSeveralVoicesPhaseAnchorComplete
                ? FString::Printf(
                      TEXT("HOLD BOTH STATES — %.1fs"),
                      CrisisSeconds)
            : Objective.bSeveralVoicesSharedResolutionResearched &&
                    Objective.bSeveralVoicesPossibleAtSite &&
                    Objective.bSeveralVoicesManifestAtSite &&
                    Objective.bSeveralVoicesNemeAtCommandSite
                ? FString::Printf(
                      TEXT("BUILD [M] PHASE ANCHOR AT %d,%d"),
                      Plan.CrisisAnchorSite.x.FloorToInt(),
                      Plan.CrisisAnchorSite.y.FloorToInt())
                : TEXT("WAITING — RESOLVE VOICES + NEME");

        DrawRect(Backdrop, Left, Top, PanelWidth, PanelHeight);
        DrawLine(Left, Top, Left + PanelWidth, Top, Accent, 2.0f);
        DrawText(TEXT("SEVERAL VOICES, ONE COMMAND  //  MISSION 14"), Accent,
                 Left + 18.0f, Top + 15.0f, SmallFont,
                 0.84f * TextScale, false);
        DrawText(
            FString::Printf(TEXT("01  RESEARCH   %s"), *ResearchState),
            bFailed ? Failed
                    : Objective.bSeveralVoicesSharedResolutionResearched
                        ? Complete : Active,
            Left + 18.0f, Top + 52.0f, SmallFont,
            0.66f * TextScale, false);
        DrawText(
            FString::Printf(TEXT("02  VOICES     %s"), *VoiceState),
            bFailed ? Failed
                    : Objective.bSeveralVoicesPossibleAtSite &&
                          Objective.bSeveralVoicesManifestAtSite &&
                          Objective.bSeveralVoicesNemeAtCommandSite
                        ? Complete : Active,
            Left + 18.0f, Top + 89.0f, SmallFont,
            0.60f * TextScale, false);
        DrawText(
            FString::Printf(TEXT("03  CRISIS     %s"), *CrisisState),
            bFailed ? Failed
                    : Objective.bSeveralVoicesCrisisWindowHeld
                        ? Complete : Active,
            Left + 18.0f, Top + 126.0f, SmallFont,
            0.66f * TextScale, false);
        if (!bLoggedObjectiveTrackerReady)
        {
            bLoggedObjectiveTrackerReady = true;
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_SEVERAL_VOICES_ONE_COMMAND_OBJECTIVES_READY] phase=%s plan=%u protocol=%u possibleVoice=%u manifestVoice=%u neme=%u researchLoom=%u phaseAnchor=%u identityTimersVisible=true crisisTimerVisible=true identityResolveTicks=160 crisisHoldTicks=160 reconstructable=true exactOrderedLedger=true commandAuthority=HollowChoir finalChoirFateDecided=false campaignBalanceUnproven=true"),
                FEchoesSeveralVoicesOneCommandMissionModel::StableName(
                    Objective.SeveralVoicesOneCommandPhase),
                Plan.StablePlanKey,
                static_cast<uint8>(Plan.RecordedProtocol),
                Objective.SeveralVoicesPossibleVoiceId,
                Objective.SeveralVoicesManifestVoiceId,
                Objective.SeveralVoicesNemeId,
                Objective.SeveralVoicesResearchLoomId,
                Objective.SeveralVoicesPhaseAnchorId);
        }
        return;
    }

    if (Objective.OperationMode ==
        EEchoesOperationMode::CampaignTheBrokenSun)
    {
        const FEchoesBrokenSunPlan Plan = Bridge->GetBrokenSunPlan();
        const bool bFailed =
            Objective.BrokenSunPhase == EEchoesBrokenSunPhase::Failed;
        const bool bAccord =
            Objective.bBrokenSunMeridianAccordEstablished &&
            Objective.bBrokenSunKharuunAccordEstablished &&
            Objective.bBrokenSunChoirAccordEstablished;
        const echoes::sim::Simulation* MissionSimulation =
            Bridge->GetSimulation();
        const double TicksPerSecond = MissionSimulation != nullptr
            ? static_cast<double>(FMath::Max<uint32>(
                  1,
                  MissionSimulation->Config().ticksPerSecond))
            : 20.0;
        const double RemainingSeconds =
            static_cast<double>(
                Objective.BrokenSunResolutionTicksRemaining) /
            TicksPerSecond;
        const FString ApproachState = bFailed
            ? TEXT("CONTRACT LOST")
            : Objective.bBrokenSunApproachSecured
                ? FString::Printf(
                      TEXT("SECURED — EXACT ANCHOR %u"),
                      Objective.BrokenSunApproachAnchorId)
                : FString::Printf(
                      TEXT("WORKER [M] APPROACH ANCHOR AT %d,%d"),
                      Plan.CrownfallApproachSite.x.FloorToInt(),
                      Plan.CrownfallApproachSite.y.FloorToInt());
        const int32 AccordCount =
            (Objective.bBrokenSunMeridianAccordEstablished ? 1 : 0) +
            (Objective.bBrokenSunKharuunAccordEstablished ? 1 : 0) +
            (Objective.bBrokenSunChoirAccordEstablished ? 1 : 0);
        const FString AccordState = bFailed
            ? TEXT("NAMED WITNESS OR COMMAND CONTRACT LOST")
            : bAccord
                ? TEXT("MARA + ORUUN + NEME + TALAR WITNESSED")
                : FString::Printf(
                      TEXT("%d/3 COMMAND SITES — PROTECT M/O/T + F2 BOTH"),
                      AccordCount);
        FString AvailableChoices;
        const auto AppendChoice = [&AvailableChoices](
            bool bAvailable,
            const TCHAR* Key,
            const TCHAR* Label)
        {
            if (!bAvailable)
            {
                return;
            }
            if (!AvailableChoices.IsEmpty())
            {
                AvailableChoices += TEXT("  ");
            }
            AvailableChoices += FString::Printf(
                TEXT("[%s] %s"),
                Key,
                Label);
        };
        const uint8 Available =
            Objective.BrokenSunAvailableFinalResolutions;
        AppendChoice(
            (Available & static_cast<uint8>(
                EEchoesFinalResolutionAvailability::Restoration)) != 0,
            TEXT("S+1"),
            TEXT("RESTORE"));
        AppendChoice(
            (Available & static_cast<uint8>(
                EEchoesFinalResolutionAvailability::ControlledStabilization)) != 0,
            TEXT("S+2"),
            TEXT("STABILIZE"));
        AppendChoice(
            (Available & static_cast<uint8>(
                EEchoesFinalResolutionAvailability::Extinguishment)) != 0,
            TEXT("S+3"),
            TEXT("EXTINGUISH"));
        AppendChoice(
            (Available & static_cast<uint8>(
                EEchoesFinalResolutionAvailability::OpenEvolution)) != 0,
            TEXT("S+4"),
            TEXT("EVOLVE"));
        FString ResolutionState = AvailableChoices;
        if (bFailed)
        {
            ResolutionState = TEXT("FINAL CONTRACT FAILED — NO ENDING COMMITTED");
        }
        else if (Objective.BrokenSunFinalResolution !=
                 EEchoesFinalResolution::None)
        {
            const echoes::sim::Vec2 Site =
                FEchoesBrokenSunMissionModel::ResolutionConvergenceSite(
                    Plan,
                    Objective.BrokenSunFinalResolution);
            ResolutionState = Objective.bBrokenSunResolutionWindowHeld
                ? FString::Printf(
                      TEXT("%s — WINDOW HELD"),
                      FEchoesBrokenSunMissionModel::ResolutionDisplayName(
                          Objective.BrokenSunFinalResolution))
            : Objective.bBrokenSunResolutionConduitComplete
                ? FString::Printf(
                      TEXT("HOLD %s — %.1fs"),
                      FEchoesBrokenSunMissionModel::ResolutionDisplayName(
                          Objective.BrokenSunFinalResolution),
                      RemainingSeconds)
                : FString::Printf(
                      TEXT("%s LOCKED — WORKER [M] AT %d,%d"),
                      FEchoesBrokenSunMissionModel::ResolutionDisplayName(
                          Objective.BrokenSunFinalResolution),
                      Site.x.FloorToInt(),
                      Site.y.FloorToInt());
        }
        else if (Objective.BrokenSunPendingFinalResolution !=
                 EEchoesFinalResolution::None)
        {
            ResolutionState = FString::Printf(
                TEXT("ARMED %s — PRESS THE SAME SHIFT+NUMBER AGAIN"),
                FEchoesBrokenSunMissionModel::ResolutionDisplayName(
                    Objective.BrokenSunPendingFinalResolution));
        }
        else if (!bAccord)
        {
            ResolutionState = TEXT("WAITING — COMPLETE THE WITNESSED ACCORD");
        }

        DrawRect(Backdrop, Left, Top, PanelWidth, PanelHeight);
        DrawLine(Left, Top, Left + PanelWidth, Top, Accent, 2.0f);
        DrawText(TEXT("THE BROKEN SUN  //  MISSION 15"), Accent,
                 Left + 18.0f, Top + 15.0f, SmallFont,
                 0.90f * TextScale, false);
        DrawText(
            FString::Printf(TEXT("01  APPROACH   %s"), *ApproachState),
            bFailed ? Failed
                    : Objective.bBrokenSunApproachSecured
                        ? Complete : Active,
            Left + 18.0f, Top + 52.0f, SmallFont,
            0.64f * TextScale, false);
        DrawText(
            FString::Printf(TEXT("02  ACCORD     %s"), *AccordState),
            bFailed ? Failed : bAccord ? Complete : Active,
            Left + 18.0f, Top + 89.0f, SmallFont,
            0.60f * TextScale, false);
        DrawText(
            FString::Printf(TEXT("03  RESOLUTION %s"), *ResolutionState),
            bFailed ? Failed
                    : Objective.bBrokenSunResolutionWindowHeld
                        ? Complete : Active,
            Left + 18.0f, Top + 126.0f, SmallFont,
            0.54f * TextScale, false);
        if (!bLoggedObjectiveTrackerReady)
        {
            bLoggedObjectiveTrackerReady = true;
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_BROKEN_SUN_OBJECTIVES_READY] phase=%s plan=%u founding=%u districtPair=%u+%u deferred=%u protocol=%u availability=0x%02X voice=%u heavy=%u neme=%u mara=%u oruun=%u talar=%u approach=%u conduit=%u exactOrderedLedger=true explicitChoice=true doubleConfirmation=true resolutionSpecificGeometry=true resolutionSpecificHold=true commandAuthority=HollowChoir mixedFactionCommand=false releaseReadinessUnproven=true"),
                FEchoesBrokenSunMissionModel::StableName(
                    Objective.BrokenSunPhase),
                Plan.StablePlanKey,
                static_cast<uint8>(Plan.FoundingDoctrine),
                static_cast<uint8>(Plan.FirstContributingDistrict),
                static_cast<uint8>(Plan.SecondContributingDistrict),
                static_cast<uint8>(Plan.DeferredDistrict),
                static_cast<uint8>(Plan.RecordedProtocol),
                Plan.AvailableFinalResolutions,
                Objective.BrokenSunAccordVoiceId,
                Objective.BrokenSunAccordHeavyId,
                Objective.BrokenSunNemeId,
                Objective.BrokenSunMaraId,
                Objective.BrokenSunOruunId,
                Objective.BrokenSunTalarId,
                Objective.BrokenSunApproachAnchorId,
                Objective.BrokenSunResolutionConduitId);
        }
        return;
    }

    const FString SkirmishMapName =
        ActiveSkirmishMapDisplayName(Bridge);
    FString WellState = TEXT("UNLOCATED — SEARCH THE BATTLEFIELD");
    FLinearColor WellColor = Active;
    if (Objective.bFutureWellVisible)
    {
        switch (Objective.VisibleFutureWellChoice)
        {
            case echoes::sim::FutureWellChoice::Harvest:
                WellState = TEXT("PROTOCOL ACTIVE — HARVEST");
                WellColor = Theme.FutureWellHarvest;
                break;
            case echoes::sim::FutureWellChoice::Preserve:
                WellState = TEXT("PROTOCOL ACTIVE — PRESERVE");
                WellColor = Theme.FutureWellPreserve;
                break;
            case echoes::sim::FutureWellChoice::Reshape:
                WellState = TEXT("PROTOCOL ACTIVE — RESHAPE");
                WellColor = Theme.FutureWellReshape;
                break;
            case echoes::sim::FutureWellChoice::Dormant:
            default:
                WellState = TEXT("IN SIGHT — AWAITING PROTOCOL");
                WellColor = Theme.FutureWellDormant;
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
    DrawText(
             FString::Printf(
                 TEXT("OPERATION %s  //  OBJECTIVES"),
                 *SkirmishMapName),
             Accent,
             Left + 18.0f, Top + 15.0f, SmallFont, 0.90f * TextScale, false);
    DrawText(FString::Printf(TEXT("01  FUTURE WELL     %s"), *WellState), WellColor,
             Left + 18.0f, Top + 52.0f, SmallFont, 0.82f * TextScale, false);
    DrawText(FString::Printf(TEXT("02  COMMAND CORE    %s"), *LocalCoreState),
             Objective.bLocalCoreIntact ? Active : Failed,
             Left + 18.0f, Top + 89.0f, SmallFont, 0.82f * TextScale, false);
    const FString OpponentShortName =
        echoes::presentation::FactionShortName(
            Bridge->GetOpponentFaction());
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

void AEchoesHUD::DrawNarrativeSubtitle(
    const AEchoesPlayerController* EchoesController,
    const UEchoesGameUserSettings* Settings)
{
    if (Canvas == nullptr || EchoesController == nullptr ||
        EchoesController->IsModalOverlayVisible() || GetWorld() == nullptr)
    {
        return;
    }
    UEchoesNarrativeSubsystem* Narrative =
        GetGameInstance() != nullptr
            ? GetGameInstance()->GetSubsystem<UEchoesNarrativeSubsystem>()
            : nullptr;
    if (Narrative == nullptr)
    {
        return;
    }
    FString Speaker;
    FString Text;
    if (!Narrative->GetActiveSubtitle(
            GetWorld()->GetRealTimeSeconds(), Speaker, Text))
    {
        return;
    }
    const bool bHighContrast =
        Settings != nullptr && Settings->IsHighContrastHudEnabled();
    const FEchoesVisualTheme Theme =
        UEchoesVisualThemeSettings::Resolve(bHighContrast);
    const float HudScale = Settings != nullptr ? Settings->GetHudScale() : 1.0f;
    UFont* SmallFont = GEngine != nullptr ? GEngine->GetSmallFont() : nullptr;

    const FString Composite =
        FString::Printf(TEXT("%s — %s"), *Speaker.ToUpper(), *Text);
    const float TextScaleValue = 0.95f * HudScale;
    const float ApproxWidth =
        7.2f * TextScaleValue * static_cast<float>(Composite.Len());
    const float BandWidth =
        FMath::Min(ApproxWidth + 48.0f, Canvas->ClipX - 40.0f);
    const float BandHeight = 34.0f * HudScale;
    const float BandLeft = (Canvas->ClipX - BandWidth) * 0.5f;
    const float BandTop = Canvas->ClipY - 96.0f * HudScale - BandHeight;
    DrawRect(
        Theme.Surface.CopyWithNewOpacity(bHighContrast ? 1.0f : 0.82f),
        BandLeft,
        BandTop,
        BandWidth,
        BandHeight);
    DrawText(
        Composite,
        Theme.TextPrimary,
        BandLeft + 24.0f,
        BandTop + 9.0f * HudScale,
        SmallFont,
        TextScaleValue,
        false);
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
    const FEchoesVisualTheme Theme =
        UEchoesVisualThemeSettings::Resolve(bHighContrast);
    const float HudScale = Settings != nullptr ? Settings->GetHudScale() : 1.0f;
    const FEchoesResultOverlayLayout Layout =
        FEchoesResultOverlayLayout::Build(
            FVector2D(Canvas->ClipX, Canvas->ClipY),
            HudScale);
    const float PanelWidth = Layout.Size.X;
    const float PanelHeight = Layout.Size.Y;
    const float Left = Layout.Origin.X;
    const float Top = Layout.Origin.Y;
    const float ContentScale = Layout.ContentScale;
    const float TextScale = Layout.TextScale;
    const echoes::sim::MatchOutcome Outcome =
        EchoesController->GetPresentedMatchOutcome();
    const bool bCampaignResult = EchoesController->IsCampaignResult();
    const FEchoesCampaignJourney CampaignJourney = Bridge != nullptr
        ? Bridge->GetCampaignJourney()
        : FEchoesCampaignJourney{};
    const bool bCampaignResultCanAdvance =
        EchoesController->CanAdvanceCampaignResult();
    const bool bCanReturnToOperations =
        EchoesController->CanReturnCompletedSkirmishToOperations();
    const bool bOnlineResult = EchoesController->IsOnlineMatchResult();
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
    const bool bShapeBesideUsResult = bCampaignResult &&
        EchoesController->GetPresentedCampaignOperation() ==
            EEchoesOperationMode::CampaignShapeBesideUs;
    const bool bReserveAuthorityResult = bCampaignResult &&
        EchoesController->GetPresentedCampaignOperation() ==
            EEchoesOperationMode::CampaignReserveAuthority;
    const bool bChoirAtLumeReachResult = bCampaignResult &&
        EchoesController->GetPresentedCampaignOperation() ==
            EEchoesOperationMode::CampaignChoirAtLumeReach;
    const bool bNoNeutralLedgerResult = bCampaignResult &&
        EchoesController->GetPresentedCampaignOperation() ==
            EEchoesOperationMode::CampaignNoNeutralLedger;
    const bool bFutureThatWonResult = bCampaignResult &&
        EchoesController->GetPresentedCampaignOperation() ==
            EEchoesOperationMode::CampaignFutureThatWon;
    const bool bAssemblyOfTheMissingResult = bCampaignResult &&
        EchoesController->GetPresentedCampaignOperation() ==
            EEchoesOperationMode::CampaignAssemblyOfTheMissing;
    const bool bSeveralVoicesOneCommandResult = bCampaignResult &&
        EchoesController->GetPresentedCampaignOperation() ==
            EEchoesOperationMode::CampaignSeveralVoicesOneCommand;
    const bool bBrokenSunResult = bCampaignResult &&
        EchoesController->GetPresentedCampaignOperation() ==
            EEchoesOperationMode::CampaignTheBrokenSun;
    const EEchoesFinalResolution FinalResolution =
        EchoesController->GetCampaignFinalResolution();
    const EEchoesFinalResolution RecordedFinalResolution =
        EchoesController->GetRecordedCampaignFinalResolution();
    const EEchoesCampaignCommitStatus CampaignCommitStatus =
        EchoesController->GetCampaignCommitStatus();
    const bool bReplayConflict = bCampaignResult &&
        CampaignCommitStatus == EEchoesCampaignCommitStatus::ReplayConflict;
    const bool bBrokenSunEndingRecorded = bBrokenSunResult &&
        (CampaignCommitStatus == EEchoesCampaignCommitStatus::Added ||
         CampaignCommitStatus ==
             EEchoesCampaignCommitStatus::AlreadyRecorded);
    const bool bVictory = bCampaignResult
        ? EchoesController->WasCampaignSuccessful()
        : EchoesController->DidPresentedLocalPlayerWin();
    const bool bDraw = !bCampaignResult &&
        Outcome == echoes::sim::MatchOutcome::Draw;
    const FString SkirmishMapName =
        ActiveSkirmishMapDisplayName(Bridge);
    const FString Result = bCampaignResult
        ? bBrokenSunResult
            ? bVictory
                ? bBrokenSunEndingRecorded
                    ? TEXT("FINAL RESOLUTION RECORDED")
                : CampaignCommitStatus ==
                        EEchoesCampaignCommitStatus::ReplayConflict
                    ? TEXT("ALTERNATE RESOLUTION REPLAY COMPLETE")
                    : TEXT("FINAL OPERATION COMPLETE — LEDGER NOT UPDATED")
                : TEXT("FINAL RESOLUTION CONTRACT FAILED")
        : bSeveralVoicesOneCommandResult
            ? bVictory ? TEXT("CHOIR CRISIS HELD")
                       : TEXT("CHOIR CRISIS CONTRACT FAILED")
        : bAssemblyOfTheMissingResult
            ? bVictory ? TEXT("PUBLIC ASSEMBLY RECEIPT RECORDED")
                       : TEXT("PUBLIC ASSEMBLY FAILED")
        : bFutureThatWonResult
            ? bVictory ? TEXT("RESTORATION PROTOCOL RECORDED")
                       : TEXT("RESTORATION DEMONSTRATION FAILED")
        : bNoNeutralLedgerResult
            ? bVictory ? TEXT("COALITION LEDGER COMMITTED")
                       : TEXT("COALITION ADMISSION FAILED")
        : bChoirAtLumeReachResult
            ? bVictory ? TEXT("LUME REACH DECISION COMMITTED")
                       : TEXT("LUME REACH CONTACT FAILED")
        : bReserveAuthorityResult
            ? bVictory ? TEXT("RESERVE ALLOCATION COMMITTED")
                       : TEXT("RESERVE ALLOCATION FAILED")
            : bVictory ? TEXT("MISSION COMPLETE") : TEXT("MISSION FAILED")
        : bVictory ? TEXT("VICTORY") : bDraw ? TEXT("DRAW") : TEXT("DEFEAT");
    const FString LocalFaction = EchoesController->GetLocalFactionLabel();
    const FString OpponentFaction = EchoesController->GetOpponentFactionLabel();
    const FString Headline = bCampaignResult
        ? bBrokenSunResult
            ? bVictory
                ? bBrokenSunEndingRecorded
                    ? FString::Printf(
                          TEXT("%s IS COMMITTED"),
                          FEchoesBrokenSunMissionModel::ResolutionDisplayName(
                              FinalResolution))
                : CampaignCommitStatus ==
                        EEchoesCampaignCommitStatus::ReplayConflict
                    ? FString::Printf(
                          TEXT("%s COMPLETED; %s REMAINS RECORDED"),
                          FEchoesBrokenSunMissionModel::ResolutionDisplayName(
                              FinalResolution),
                          FEchoesBrokenSunMissionModel::ResolutionDisplayName(
                              RecordedFinalResolution))
                    : FString::Printf(
                          TEXT("%s COMPLETED IN SIMULATION"),
                          FEchoesBrokenSunMissionModel::ResolutionDisplayName(
                              FinalResolution))
                : TEXT("THE FINAL ACCORD BREAKS")
        : bSeveralVoicesOneCommandResult
            ? bVictory ? TEXT("SEVERAL VOICES HOLD ONE COMMAND")
                       : TEXT("THE SHARED RESOLUTION BREAKS")
        : bAssemblyOfTheMissingResult
            ? bVictory ? TEXT("THE MISSING RECORD HOLDS IN PUBLIC")
                       : TEXT("THE ASSEMBLY CANNOT BE OBSERVED")
        : bFutureThatWonResult
            ? bVictory ? TEXT("THE RECORDED FUTURE HOLDS—HERE, FOR NOW")
                       : TEXT("THE PUBLIC READBACK CANNOT CLOSE")
        : bNoNeutralLedgerResult
            ? bVictory ? TEXT("THE RECORD HOLDS WITHOUT A NEUTRAL HAND")
                       : TEXT("THE COALITION LINE CANNOT BE ATTESTED")
        : bChoirAtLumeReachResult
            ? bVictory ? TEXT("THE LISTENING LINE OPENS A WAY THROUGH")
                       : TEXT("THE CHOIR FALLS BEYOND THE ANCHORS")
        : bReserveAuthorityResult
            ? bVictory ? TEXT("TWO DISTRICTS HOLD; ONE REMAINS DEFERRED")
                       : TEXT("THE DISTRICT RESERVE EXCEEDS ITS SAFE MARGIN")
        : bShapeBesideUsResult
            ? bVictory ? TEXT("NEME ANSWERS FROM BOTH SIDES OF THE ROUTE")
                       : TEXT("THE OVERLAP CLOSES AROUND THE WITNESSES")
        : bShapeOfSilenceResult
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
        : bVictory
            ? FString::Printf(
                  TEXT("%s SECURED"),
                  *SkirmishMapName)
        : bDraw ? TEXT("NO COMMAND CORE REMAINS")
                : FString::Printf(TEXT("THE %s LINE BROKE"), *LocalFaction);
    const FString Summary = bCampaignResult
        ? bBrokenSunResult
            ? bVictory
                ? FString::Printf(
                      TEXT("The Crownfall approach, three-part accord, exact Resolution Conduit, and fixed hold contract completed under %s. Cost: %s %s Wider social consequences, campaign balance, and release readiness remain unproven."),
                      FEchoesBrokenSunMissionModel::ResolutionDisplayName(
                          FinalResolution),
                      FEchoesBrokenSunMissionModel::ResolutionCostSummary(
                          FinalResolution),
                      bBrokenSunEndingRecorded
                          ? TEXT("The campaign ledger records this bounded resolution.")
                      : CampaignCommitStatus ==
                              EEchoesCampaignCommitStatus::ReplayConflict
                          ? TEXT("This was a replay; the earlier ledger ending was not rewritten.")
                          : TEXT("The simulation completed, but no durable campaign ending was saved."))
                : TEXT("The local Core, Hollow Choir command force, Neme, Mara, Oruun, Talar, exact approach anchor, accord, chosen conduit, or fixed hold contract failed before the final resolution could be recorded.")
        : bSeveralVoicesOneCommandResult
            ? bVictory
                ? FString::Printf(
                      TEXT("One protected voice resolved to Possible, one remained Manifest, and Neme sustained both under the recorded %s protocol context through the full Phase Anchor crisis window. This completes one bounded Choir command perspective; it does not decide the Choir's final fate, prove campaign balance, or establish release readiness."),
                      WellChoiceDisplayName(
                          EchoesController->GetCampaignConsequence()))
                : TEXT("The local Core, Neme, a protected voice, or the Research Loom was lost, the match ended, or the Phase Anchor contract was violated before the crisis window closed.")
        : bAssemblyOfTheMissingResult
            ? bVictory
                ? FString::Printf(
                      TEXT("The public-record pair, Crownfall index link, and both independent assembly views were observed under the recorded %s protocol. This does not assign authorship or responsibility, establish consent or trust, model civilian state, prove cryptographic authenticity, or establish wider cause."),
                      WellChoiceDisplayName(
                          EchoesController->GetCampaignConsequence()))
                : TEXT("Oruun, the independent verifier, the local Core, or one of the three neutral public interfaces was lost before both assembly views were observed.")
        : bFutureThatWonResult
            ? bVictory
                ? FString::Printf(
                      TEXT("The recorded %s protocol held through the bounded local stability window and both district readbacks were observed. This does not establish civilian counts, population restoration, a permanent future, trust, consent, or ethical justification."),
                      WellChoiceDisplayName(
                          EchoesController->GetCampaignConsequence()))
                : TEXT("Oruun, the independent verifier, the local Core, the Future Well, or a public readback interface was lost before the bounded demonstration completed.")
        : bNoNeutralLedgerResult
            ? bVictory
                ? FString::Printf(
                      TEXT("The inherited route, both contributing district systems, and both public evidence channels support the recorded %s protocol. This establishes one local coalition rally, not mixed-faction command, hidden authorship, casualty counts, or wider cause."),
                      WellChoiceDisplayName(
                          EchoesController->GetCampaignConsequence()))
                : TEXT("Oruun, the ledger witness, the Waystone, the local Core, the Lume Well, or the active Reshape rally window was lost before coalition admission.")
        : bChoirAtLumeReachResult
            ? bVictory
                ? FString::Printf(
                      TEXT("Both Listening Spines held while Oruun completed the %s resolution. This proves one local contact operation and Well decision; the Choir remains non-commandable in Mission 10 and no hidden authorship or wider cause is established."),
                      WellChoiceDisplayName(
                          EchoesController->GetCampaignConsequence()))
                : TEXT("Oruun, the Waystone, a committed Listening Spine, the local Core, the Lume Well, or the active Reshape exit window was lost before resolution.")
        : bReserveAuthorityResult
            ? bVictory
                ? FString::Printf(
                      TEXT("Two districts retain power; %s remains intact but deferred. This proves one local allocation, not wider city recovery or unmodeled civilian survival."),
                      FEchoesCityReserveMissionModel::DistrictDisplayName(
                          Bridge != nullptr
                              ? Bridge->GetReserveAuthorityDeferredDistrict()
                              : EEchoesCityDistrict::LifeSupport))
                : TEXT("Mara, a protected district, the local Core, or the exact two-district allocation contract was lost before the record could be committed.")
        : bShapeBesideUsResult
            ? bVictory
                ? FString::Printf(
                      TEXT("The %s overlap answered Talar with repeatable, actionable correspondence across both states. This establishes reciprocal contact, not one Choir identity, hidden authorship, or cause."),
                      WellChoiceDisplayName(
                          EchoesController->GetCampaignConsequence()))
                : TEXT("Talar, a state witness, the local Core, or the overlap operation was lost before convergence.")
        : bShapeOfSilenceResult
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
                CampaignPersistenceLine = bBrokenSunResult
                    ? FString::Printf(
                          TEXT("MISSION 15 RECORDED // %s fixed as the campaign's final resolution."),
                          FEchoesBrokenSunMissionModel::ResolutionDisplayName(
                              RecordedFinalResolution))
                : bSeveralVoicesOneCommandResult
                    ? FString::Printf(
                          TEXT("MISSION 14 RECORDED // %s Choir crisis contract fixed."),
                          RecordedChoice)
                : bAssemblyOfTheMissingResult
                    ? FString::Printf(
                          TEXT("MISSION 13 RECORDED // %s public assembly/readback contract fixed."),
                          RecordedChoice)
                : bFutureThatWonResult
                    ? FString::Printf(
                          TEXT("MISSION 12 RECORDED // %s local protocol/readback contract fixed."),
                          RecordedChoice)
                : bNoNeutralLedgerResult
                    ? FString::Printf(
                          TEXT("MISSION 11 RECORDED // %s local coalition rally fixed."),
                          RecordedChoice)
                : bChoirAtLumeReachResult
                    ? FString::Printf(
                          TEXT("MISSION 10 RECORDED // %s Lume Reach decision fixed."),
                          RecordedChoice)
                : bReserveAuthorityResult
                    ? FString::Printf(
                          TEXT("MISSION 09 RECORDED // %s local reserve allocation fixed."),
                          RecordedChoice)
                : bShapeBesideUsResult
                    ? FString::Printf(
                          TEXT("MISSION 08 RECORDED // %s reciprocal contact observed."),
                          RecordedChoice)
                : bShapeOfSilenceResult
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
                CampaignPersistenceLine = bBrokenSunResult
                    ? FString::Printf(
                          TEXT("LEDGER VERIFIED // %s remains the recorded final resolution."),
                          FEchoesBrokenSunMissionModel::ResolutionDisplayName(
                              RecordedFinalResolution))
                    : FString::Printf(
                          TEXT("LEDGER VERIFIED // %s remains recorded."),
                          RecordedChoice);
                break;
            case EEchoesCampaignCommitStatus::ReplayConflict:
                CampaignPersistenceLine = bBrokenSunResult
                    ? FString::Printf(
                          TEXT("REPLAY ONLY // campaign ledger remains %s."),
                          FEchoesBrokenSunMissionModel::ResolutionDisplayName(
                              RecordedFinalResolution))
                    : FString::Printf(
                          TEXT("REPLAY ONLY // campaign ledger remains %s."),
                          RecordedChoice);
                break;
            default:
                CampaignPersistenceLine =
                    TEXT("LEDGER NOT SAVED // mission complete; progress unavailable.");
                break;
        }
    }
    const FLinearColor Accent = bVictory
        ? Theme.Success
        : bDraw ? Theme.Warning : Theme.Danger;
    const FLinearColor Body = Theme.TextPrimary;
    const FLinearColor Muted = Theme.TextSecondary;
    UFont* SmallFont = GEngine != nullptr ? GEngine->GetSmallFont() : nullptr;
    const uint64 FinalTick = bOnlineResult
        ? EchoesController->GetPresentedFinalTick()
        : Bridge != nullptr && Bridge->GetSimulation() != nullptr
            ? Bridge->GetSimulation()->CurrentTick()
            : 0;

    DrawRect(Theme.Scrim, 0.0f, 0.0f, Canvas->ClipX, Canvas->ClipY);
    DrawVisualPanel(
        FBox2D(Layout.Origin, Layout.Origin + Layout.Size), Theme, true);
    DrawShatteredSunMotif(
        FBox2D(Layout.Origin, Layout.Origin + Layout.Size),
        Theme, Settings, bHighContrast ? 0.12f : 0.05f);
    DrawLine(Left, Top, Left + PanelWidth, Top, Accent,
             Theme.EmphasisThickness);
    DrawText(Result, Accent, Left + 44.0f, Top + 42.0f * ContentScale,
             SmallFont, 1.9f * TextScale, false);
    DrawText(Headline, Body, Left + 44.0f, Top + 96.0f * ContentScale,
             SmallFont, 1.22f * TextScale, false);
    DrawText(Summary, Body, Left + 44.0f, Top + 148.0f * ContentScale,
             SmallFont, 0.92f * TextScale, false);
    const FString FinalTickLine = bCampaignResult
        ? bBrokenSunResult
            ? FString::Printf(
                  TEXT("MISSION 15 — THE BROKEN SUN  //  FINAL TICK %llu"),
                  static_cast<unsigned long long>(FinalTick))
        : bSeveralVoicesOneCommandResult
            ? FString::Printf(
                  TEXT("MISSION 14 — SEVERAL VOICES, ONE COMMAND  //  FINAL TICK %llu"),
                  static_cast<unsigned long long>(FinalTick))
        : bAssemblyOfTheMissingResult
            ? FString::Printf(
                  TEXT("MISSION 13 — ASSEMBLY OF THE MISSING  //  FINAL TICK %llu"),
                  static_cast<unsigned long long>(FinalTick))
        : bFutureThatWonResult
            ? FString::Printf(
                  TEXT("MISSION 12 — THE FUTURE THAT WON  //  FINAL TICK %llu"),
                  static_cast<unsigned long long>(FinalTick))
        : bNoNeutralLedgerResult
            ? FString::Printf(
                  TEXT("MISSION 11 — NO NEUTRAL LEDGER  //  FINAL TICK %llu"),
                  static_cast<unsigned long long>(FinalTick))
        : bChoirAtLumeReachResult
            ? FString::Printf(
                  TEXT("MISSION 10 — THE CHOIR AT LUME REACH  //  FINAL TICK %llu"),
                  static_cast<unsigned long long>(FinalTick))
        : bReserveAuthorityResult
            ? FString::Printf(
                  TEXT("MISSION 09 — RESERVE AUTHORITY  //  FINAL TICK %llu"),
                  static_cast<unsigned long long>(FinalTick))
        : bShapeBesideUsResult
            ? FString::Printf(
                  TEXT("MISSION 08 — THE SHAPE BESIDE US  //  FINAL TICK %llu"),
                  static_cast<unsigned long long>(FinalTick))
        : bShapeOfSilenceResult
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
              TEXT("OPERATION %s  //  FINAL TICK %llu"),
              *SkirmishMapName,
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

    const FLinearColor ButtonText = Theme.ActionText;
    const bool bHasDistinctPrimaryAction = bCanReturnToOperations ||
        bCampaignResultCanAdvance || bReplayConflict;
    if (bOnlineResult)
    {
        DrawRect(
            EchoesController->IsNetworkResultExitEnabled()
                ? Accent
                : FLinearColor(Accent.R, Accent.G, Accent.B, 0.45f),
            Layout.FullButton.Min.X,
            Layout.FullButton.Min.Y,
            Layout.FullButton.GetSize().X,
            Layout.FullButton.GetSize().Y);
        DrawText(
            EchoesController->IsNetworkResultExitEnabled()
                ? TEXT("ENTER / A / R / CLICK: ONLINE MENU")
                : TEXT("FINALIZING RESULT DELIVERY..."),
            ButtonText,
            Layout.FullButton.Min.X + 26.0f * ContentScale,
            Layout.FullButton.Min.Y + 13.0f,
            SmallFont,
            0.90f * TextScale,
            false);
    }
    else if (bHasDistinctPrimaryAction)
    {
        DrawRect(
            Accent,
            Layout.PrimaryButton.Min.X,
            Layout.PrimaryButton.Min.Y,
            Layout.PrimaryButton.GetSize().X,
            Layout.PrimaryButton.GetSize().Y);
        DrawRect(
            FLinearColor(Accent.R, Accent.G, Accent.B, 0.76f),
            Layout.RestartButton.Min.X,
            Layout.RestartButton.Min.Y,
            Layout.RestartButton.GetSize().X,
            Layout.RestartButton.GetSize().Y);
        const FString PrimaryLabel = bCanReturnToOperations
            ? TEXT("ENTER / A: OPERATIONS")
            : bReplayConflict
                ? TEXT("ESC / CLICK: JOURNEY")
            : CampaignJourney.State == EEchoesCampaignJourneyState::Complete
                ? TEXT("ENTER / A: TITLE")
                : TEXT("ENTER / A: CONTINUE");
        DrawText(
            PrimaryLabel,
            ButtonText,
            Layout.PrimaryButton.Min.X + 18.0f * ContentScale,
            Layout.PrimaryButton.Min.Y + 13.0f,
            SmallFont,
            0.82f * TextScale,
            false);
        DrawText(
            bCampaignResult ? TEXT("R / CLICK: REPLAY")
                            : TEXT("R / CLICK: RESTART"),
            ButtonText,
            Layout.RestartButton.Min.X + 18.0f * ContentScale,
            Layout.RestartButton.Min.Y + 13.0f,
            SmallFont,
            0.82f * TextScale,
            false);
    }
    else
    {
        DrawRect(
            Accent,
            Layout.FullButton.Min.X,
            Layout.FullButton.Min.Y,
            Layout.FullButton.GetSize().X,
            Layout.FullButton.GetSize().Y);
        DrawText(
            bCampaignResult ? TEXT("ENTER / R / CLICK: REPLAY MISSION")
                            : TEXT("R / CLICK: RESTART MATCH"),
            ButtonText,
            Layout.FullButton.Min.X + 26.0f * ContentScale,
            Layout.FullButton.Min.Y + 13.0f,
            SmallFont,
            0.90f * TextScale,
            false);
    }
    // Authored result copy from the narrative pack, consumed at runtime.
    if (bCampaignResult)
    {
        const UEchoesNarrativeSubsystem* Narrative =
            GetGameInstance() != nullptr
                ? GetGameInstance()->GetSubsystem<UEchoesNarrativeSubsystem>()
                : nullptr;
        const EEchoesCampaignCommitStatus CommitStatus =
            EchoesController->GetCampaignCommitStatus();
        const TCHAR* StatusKey =
            CommitStatus == EEchoesCampaignCommitStatus::Added
                ? TEXT("Added")
            : CommitStatus == EEchoesCampaignCommitStatus::AlreadyRecorded
                ? TEXT("AlreadyRecorded")
            : CommitStatus == EEchoesCampaignCommitStatus::ReplayConflict
                ? TEXT("ReplayConflict")
            : CommitStatus == EEchoesCampaignCommitStatus::StorageFailure
                ? TEXT("StorageFailure")
                : nullptr;
        const FString AuthoredResult =
            Narrative != nullptr && StatusKey != nullptr
                ? Narrative->GetResultCopy(
                      EchoesController->GetPresentedCampaignOperation(),
                      StatusKey)
                : FString();
        if (!AuthoredResult.IsEmpty())
        {
            const float ResultScale = 0.82f * TextScale;
            const int32 CharsPerLine = FMath::Max(
                40,
                FMath::FloorToInt(
                    (PanelWidth - 84.0f) / (7.2f * ResultScale)));
            TArray<FString> Wrapped;
            FString Remaining = AuthoredResult;
            while (!Remaining.IsEmpty() && Wrapped.Num() < 3)
            {
                if (Remaining.Len() <= CharsPerLine)
                {
                    Wrapped.Add(Remaining);
                    break;
                }
                int32 Break = CharsPerLine;
                while (Break > 0 && Remaining[Break] != TEXT(' '))
                {
                    --Break;
                }
                if (Break == 0)
                {
                    Break = CharsPerLine;
                }
                Wrapped.Add(Remaining.Left(Break));
                Remaining.RightChopInline(Break + 1);
            }
            UFont* ResultFont =
                GEngine != nullptr ? GEngine->GetSmallFont() : nullptr;
            const FEchoesVisualTheme ResultTheme =
                UEchoesVisualThemeSettings::Resolve(
                    Settings != nullptr &&
                    Settings->IsHighContrastHudEnabled());
            for (int32 Index = 0; Index < Wrapped.Num(); ++Index)
            {
                DrawText(
                    Wrapped[Index],
                    ResultTheme.TextSecondary,
                    Left + 42.0f,
                    Top + PanelHeight -
                        (78.0f - 15.0f * Index) * ContentScale,
                    ResultFont,
                    ResultScale,
                    false);
            }
        }
    }

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
    const FEchoesVisualTheme Theme =
        UEchoesVisualThemeSettings::Resolve(bHighContrast);
    const float HudScale = Settings != nullptr ? Settings->GetHudScale() : 1.0f;
    const FEchoesPauseOverlayLayout Layout =
        FEchoesPauseOverlayLayout::Build(
            FVector2D(Canvas->ClipX, Canvas->ClipY),
            HudScale);
    const float PanelWidth = Layout.Size.X;
    const float PanelHeight = Layout.Size.Y;
    const float Left = Layout.Origin.X;
    const float Top = Layout.Origin.Y;
    const float ContentScale = Layout.ContentScale;
    const float TextScale = Layout.TextScale;
    const FLinearColor Accent = Theme.Accent;
    const FLinearColor Body = Theme.TextPrimary;
    const FLinearColor Muted = Theme.TextSecondary;
    UFont* SmallFont = GEngine != nullptr ? GEngine->GetSmallFont() : nullptr;
    const UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    const bool bSkirmish = Bridge != nullptr &&
        Bridge->GetOperationMode() == EEchoesOperationMode::Skirmish;
    const bool bCanReturnToOperations = bSkirmish &&
        !Bridge->IsNetworkHumanOpponentEnabled();
    const FString OperationLabel = Bridge != nullptr
        ? Bridge->GetOperationLabel()
        : TEXT("OPERATION");
    const FString OperationLine = bSkirmish
        ? FString::Printf(
              TEXT("%s  //  %s VS %s  //  MATCH PAUSED"),
              FEchoesSkirmishSetupModel::MapDisplayName(
                  Bridge->GetActiveSkirmishSetup().MapPreset),
              FEchoesSkirmishSetupModel::FactionDisplayName(
                  Bridge->GetActiveSkirmishSetup().LocalFaction),
              FEchoesSkirmishSetupModel::FactionDisplayName(
                  Bridge->GetActiveSkirmishSetup().OpponentFaction))
        : FString::Printf(
              TEXT("%s  //  OPERATION PAUSED"),
              *OperationLabel);
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

    DrawRect(Theme.Scrim, 0.0f, 0.0f, Canvas->ClipX, Canvas->ClipY);
    DrawVisualPanel(
        FBox2D(Layout.Origin, Layout.Origin + Layout.Size), Theme, true);
    DrawText(TEXT("FIELD MENU"), Accent, Left + 42.0f,
             Top + 34.0f * ContentScale, SmallFont, 1.65f * TextScale, false);
    DrawText(OperationLine,
             Muted, Left + 42.0f, Top + 78.0f * ContentScale,
             SmallFont, 0.88f * TextScale, false);

    DrawText(TEXT("MATCH CONTROL"), Accent, Left + 42.0f,
             Top + 132.0f * ContentScale, SmallFont, 0.92f * TextScale, false);
    DrawRect(
        Theme.WithAlpha(Theme.Selected, 0.13f),
        Layout.ResumeButton.Min.X,
        Layout.ResumeButton.Min.Y,
        Layout.ResumeButton.GetSize().X,
        Layout.ResumeButton.GetSize().Y);
    DrawText(TEXT("[ENTER / ESCAPE / P]  RESUME OPERATION"), Body,
             Left + 42.0f, Top + 162.0f * ContentScale,
             SmallFont, 1.0f * TextScale, false);
    DrawRect(
        Theme.WithAlpha(Theme.Selected, 0.13f),
        Layout.RestartButton.Min.X,
        Layout.RestartButton.Min.Y,
        Layout.RestartButton.GetSize().X,
        Layout.RestartButton.GetSize().Y);
    DrawText(TEXT("[R]  RESTART OPERATION FROM ITS DETERMINISTIC BASELINE"), Body,
             Left + 42.0f, Top + 192.0f * ContentScale,
             SmallFont, 0.92f * TextScale, false);
    if (bCanReturnToOperations)
    {
        DrawRect(
            EchoesController->IsReturnToOperationsConfirmationArmed()
                ? Theme.WithAlpha(Theme.Warning, 0.30f)
                : Theme.WithAlpha(Theme.Selected, 0.13f),
            Layout.ReturnButton.Min.X,
            Layout.ReturnButton.Min.Y,
            Layout.ReturnButton.GetSize().X,
            Layout.ReturnButton.GetSize().Y);
        DrawText(
            EchoesController->IsReturnToOperationsConfirmationArmed()
                ? TEXT("[F10 / MENU]  CONFIRM RETURN TO OPERATIONS — FIELD REMAINS PAUSED")
                : TEXT("[F10 / MENU]  RETURN TO OPERATIONS — CONFIRMATION REQUIRED"),
            EchoesController->IsReturnToOperationsConfirmationArmed()
                ? Accent
                : Body,
            Left + 42.0f, Top + 222.0f * ContentScale,
            SmallFont, 0.84f * TextScale, false);
    }

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

    DrawRect(Accent,
             Layout.PrimaryButton.Min.X,
             Layout.PrimaryButton.Min.Y,
             Layout.PrimaryButton.GetSize().X,
             Layout.PrimaryButton.GetSize().Y);
    DrawText(
             bCanReturnToOperations &&
                     EchoesController->IsReturnToOperationsConfirmationArmed()
                 ? TEXT("F10 / MENU AGAIN: RETURN TO OPERATIONS")
                 : TEXT("PRESS ENTER TO RESUME OPERATION"),
             Theme.ActionText,
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
    const FEchoesVisualTheme Theme =
        UEchoesVisualThemeSettings::Resolve(bHighContrast);
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
    const FLinearColor Accent = Theme.Accent;
    const FLinearColor Body = Theme.TextPrimary;
    const FLinearColor Muted = Theme.TextSecondary;
    UFont* SmallFont = GEngine != nullptr ? GEngine->GetSmallFont() : nullptr;
    const FString LocalFaction = EchoesController->GetLocalFactionLabel();
    const FString OpponentFaction = EchoesController->GetOpponentFactionLabel();
    const UEchoesSimulationSubsystem* BriefingBridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    const FString SkirmishMapName =
        ActiveSkirmishMapDisplayName(BriefingBridge);
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
    const bool bShapeBesideUs = BriefingBridge != nullptr &&
        BriefingBridge->GetOperationMode() ==
            EEchoesOperationMode::CampaignShapeBesideUs;
    const bool bReserveAuthority = BriefingBridge != nullptr &&
        BriefingBridge->GetOperationMode() ==
            EEchoesOperationMode::CampaignReserveAuthority;
    const bool bChoirAtLumeReach = BriefingBridge != nullptr &&
        BriefingBridge->GetOperationMode() ==
            EEchoesOperationMode::CampaignChoirAtLumeReach;
    const bool bNoNeutralLedger = BriefingBridge != nullptr &&
        BriefingBridge->GetOperationMode() ==
            EEchoesOperationMode::CampaignNoNeutralLedger;
    const bool bFutureThatWon = BriefingBridge != nullptr &&
        BriefingBridge->GetOperationMode() ==
            EEchoesOperationMode::CampaignFutureThatWon;
    const bool bAssemblyOfTheMissing = BriefingBridge != nullptr &&
        BriefingBridge->GetOperationMode() ==
            EEchoesOperationMode::CampaignAssemblyOfTheMissing;
    const bool bSeveralVoicesOneCommand = BriefingBridge != nullptr &&
        BriefingBridge->GetOperationMode() ==
            EEchoesOperationMode::CampaignSeveralVoicesOneCommand;
    const bool bBrokenSun = BriefingBridge != nullptr &&
        BriefingBridge->GetOperationMode() ==
            EEchoesOperationMode::CampaignTheBrokenSun;
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
    FString ContinuancePlayerLinkSites;
    for (const auto& Site : ContinuancePlan.PlayerPowerLinkSites)
    {
        if (!ContinuancePlayerLinkSites.IsEmpty())
        {
            ContinuancePlayerLinkSites += TEXT(", ");
        }
        ContinuancePlayerLinkSites += FString::Printf(
            TEXT("%d,%d"),
            Site.x.FloorToInt(),
            Site.y.FloorToInt());
    }
    const FEchoesNamesWithoutBirthsPlan NamesPlan =
        BriefingBridge != nullptr
            ? BriefingBridge->GetNamesWithoutBirthsPlan()
            : FEchoesNamesWithoutBirthsPlan{};
    const FEchoesShapeOfSilencePlan ShapePlan =
        BriefingBridge != nullptr
            ? BriefingBridge->GetShapeOfSilencePlan()
            : FEchoesShapeOfSilencePlan{};
    const FEchoesShapeBesideUsPlan BesidePlan =
        BriefingBridge != nullptr
            ? BriefingBridge->GetShapeBesideUsPlan()
            : FEchoesShapeBesideUsPlan{};
    const FEchoesReserveAuthorityPlan ReservePlan =
        BriefingBridge != nullptr
            ? BriefingBridge->GetReserveAuthorityPlan()
            : FEchoesReserveAuthorityPlan{};
    const FEchoesChoirAtLumeReachPlan ChoirPlan =
        BriefingBridge != nullptr
            ? BriefingBridge->GetChoirAtLumeReachPlan()
            : FEchoesChoirAtLumeReachPlan{};
    const FEchoesNoNeutralLedgerPlan NoNeutralPlan =
        BriefingBridge != nullptr
            ? BriefingBridge->GetNoNeutralLedgerPlan()
            : FEchoesNoNeutralLedgerPlan{};
    const FEchoesFutureThatWonPlan FutureThatWonPlan =
        BriefingBridge != nullptr
            ? BriefingBridge->GetFutureThatWonPlan()
            : FEchoesFutureThatWonPlan{};
    const FEchoesAssemblyOfTheMissingPlan AssemblyPlan =
        BriefingBridge != nullptr
            ? BriefingBridge->GetAssemblyOfTheMissingPlan()
            : FEchoesAssemblyOfTheMissingPlan{};
    const FEchoesSeveralVoicesOneCommandPlan SeveralVoicesPlan =
        BriefingBridge != nullptr
            ? BriefingBridge->GetSeveralVoicesOneCommandPlan()
            : FEchoesSeveralVoicesOneCommandPlan{};
    const FEchoesBrokenSunPlan BrokenSunPlan =
        BriefingBridge != nullptr
            ? BriefingBridge->GetBrokenSunPlan()
            : FEchoesBrokenSunPlan{};
    FString BrokenSunAvailableResolutions;
    const auto AppendBrokenSunResolution =
        [&BrokenSunAvailableResolutions](const TCHAR* Name)
        {
            if (!BrokenSunAvailableResolutions.IsEmpty())
            {
                BrokenSunAvailableResolutions += TEXT(" / ");
            }
            BrokenSunAvailableResolutions += Name;
        };
    if (FEchoesBrokenSunMissionModel::IsResolutionAvailable(
            BrokenSunPlan, EEchoesFinalResolution::Restoration))
    {
        AppendBrokenSunResolution(TEXT("RESTORATION"));
    }
    if (FEchoesBrokenSunMissionModel::IsResolutionAvailable(
            BrokenSunPlan,
            EEchoesFinalResolution::ControlledStabilization))
    {
        AppendBrokenSunResolution(TEXT("CONTROLLED"));
    }
    if (FEchoesBrokenSunMissionModel::IsResolutionAvailable(
            BrokenSunPlan, EEchoesFinalResolution::Extinguishment))
    {
        AppendBrokenSunResolution(TEXT("EXTINGUISHMENT"));
    }
    if (FEchoesBrokenSunMissionModel::IsResolutionAvailable(
            BrokenSunPlan, EEchoesFinalResolution::OpenEvolution))
    {
        AppendBrokenSunResolution(TEXT("OPEN EVOLUTION"));
    }
    FString FactionSystems;
    const echoes::sim::Faction BriefingFaction =
        BriefingBridge != nullptr
            ? BriefingBridge->GetLocalFaction()
            : echoes::sim::Faction::MeridianCompact;
    switch (BriefingFaction)
    {
        case echoes::sim::Faction::MeridianCompact:
            FactionSystems = TEXT("Meridian systems: [Backslash] Bulwark deployment  |  [=] Relay supply");
            break;
        case echoes::sim::Faction::KharuunAssemblies:
            FactionSystems = TEXT("Kharuun systems: [F3] Waystone  |  [F4/F5] warform  |  [F6] Cairnback cover");
            break;
        case echoes::sim::Faction::HollowChoir:
            FactionSystems = TEXT("Hollow Choir systems: [Shift+F3] Manifest  |  [Shift+F4] Possible  |  coherence draws Dawnshards");
            break;
    }

    DrawRect(Theme.Scrim, 0.0f, 0.0f, Canvas->ClipX, Canvas->ClipY);
    const FBox2D BriefingBounds(
        FVector2D(Left, Top),
        FVector2D(Left + PanelWidth, Top + PanelHeight));
    DrawVisualPanel(BriefingBounds, Theme, true);
    DrawShatteredSunMotif(
        BriefingBounds, Theme, Settings, bHighContrast ? 0.10f : 0.045f);
    DrawFactionSigil(
        VisualFactionFromLabel(LocalFaction),
        FVector2D(Left + PanelWidth - 76.0f * ContentScale,
                  Top + 70.0f * ContentScale),
        26.0f * ContentScale,
        Theme,
        0.92f);

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
        : bShapeBesideUs
            ? FString::Printf(
                  TEXT("THE SHAPE BESIDE US  //  MISSION 08  //  %s"),
                  *LocalFaction)
        : bReserveAuthority
            ? FString::Printf(
                  TEXT("RESERVE AUTHORITY  //  MISSION 09  //  %s"),
                  *LocalFaction)
        : bChoirAtLumeReach
            ? FString::Printf(
                  TEXT("THE CHOIR AT LUME REACH  //  MISSION 10  //  %s"),
                  *LocalFaction)
        : bNoNeutralLedger
            ? FString::Printf(
                  TEXT("NO NEUTRAL LEDGER  //  MISSION 11  //  %s"),
                  *LocalFaction)
        : bFutureThatWon
            ? FString::Printf(
                  TEXT("THE FUTURE THAT WON  //  MISSION 12  //  %s"),
                  *LocalFaction)
        : bAssemblyOfTheMissing
            ? FString::Printf(
                  TEXT("ASSEMBLY OF THE MISSING  //  MISSION 13  //  %s"),
                  *LocalFaction)
        : bSeveralVoicesOneCommand
            ? FString::Printf(
                  TEXT("SEVERAL VOICES, ONE COMMAND  //  MISSION 14  //  %s"),
                  *LocalFaction)
        : bBrokenSun
            ? FString::Printf(
                  TEXT("THE BROKEN SUN  //  MISSION 15  //  %s"),
                  *LocalFaction)
        : FString::Printf(
              TEXT("%s  //  OPERATIONS BRIEF  //  %s"),
              *SkirmishMapName,
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
        : bShapeBesideUs
            ? FString::Printf(
                  TEXT("Seven consistent records reveal the %s, where Neme's guidance remains coherent across two incompatible routes."),
                  BesidePlan.DisplayName)
        : bReserveAuthority
            ? FString::Printf(
                  TEXT("Eight consistent records authorize the %s. Three Lume Reach districts are failing; the reserve can sustain exactly two."),
                  ReservePlan.DisplayName)
        : bChoirAtLumeReach
            ? FString::Printf(
                  TEXT("Nine consistent records open the %s. The deferred %s liability remains public at tile %d,%d."),
                  ChoirPlan.DisplayName,
                  FEchoesCityReserveMissionModel::DistrictDisplayName(
                      ChoirPlan.DeferredDistrict),
                  ChoirPlan.LiabilitySite.x.FloorToInt(),
                  ChoirPlan.LiabilitySite.y.FloorToInt())
        : bNoNeutralLedger
            ? FString::Printf(
                  TEXT("Ten exact ordered records admit plan %02u: the %s route, two powered district systems, and the recorded %s Lume protocol."),
                  NoNeutralPlan.StablePlanKey,
                  NoNeutralPlan.RouteDisplayName,
                  NoNeutralPlan.ProtocolDisplayName)
        : bFutureThatWon
            ? FString::Printf(
                  TEXT("Eleven exact ordered records admit plan %02u: two recorded district inputs and one bounded %s restoration demonstrator."),
                  FutureThatWonPlan.StablePlanKey,
                  FutureThatWonPlan.ProtocolDisplayName)
        : bAssemblyOfTheMissing
            ? FString::Printf(
                  TEXT("Twelve exact ordered records admit plan %02u: paired public readback, one Crownfall index link, and two independent assembly views under recorded %s."),
                  AssemblyPlan.StablePlanKey,
                  AssemblyPlan.ProtocolDisplayName)
        : bSeveralVoicesOneCommand
            ? FString::Printf(
                  TEXT("Thirteen exact ordered records admit plan %02u: one bounded Hollow Choir command crisis under the inherited %s protocol context."),
                  SeveralVoicesPlan.StablePlanKey,
                  SeveralVoicesPlan.ProtocolDisplayName)
        : bBrokenSun
            ? FString::Printf(
                  TEXT("Plan %02u inherits %s, %s, protected neutral witnesses, and the earned final-resolution set."),
                  BrokenSunPlan.StablePlanKey,
                  BrokenSunPlan.RouteDisplayName,
                  BrokenSunPlan.ProtocolDisplayName)
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
        : bShapeBesideUs
            ? TEXT("Talar and both witnesses are Meridian-authoritative proxies. Neme is represented by observable route correspondence; Mission 08 does not yet grant command over the Hollow Choir.")
        : bReserveAuthority
            ? FString::Printf(
                  TEXT("Mara holds Meridian authority. %s is the inherited recommendation, but the allocation remains the player's irreversible choice."),
                  FEchoesCityReserveMissionModel::DistrictDisplayName(
                      ReservePlan.RecommendedFirstDistrict))
        : bChoirAtLumeReach
            ? TEXT("Oruun commands Kharuun. Mara is off-map; the Choir is public contact only. Meridian units are proxies, not evidence of Mara or Compact-wide action.")
        : bNoNeutralLedger
            ? FString::Printf(
                  TEXT("Oruun's Kharuun force alone is commandable. %s and %s contribute public district interfaces; %s remains a named liability, not a casualty claim."),
                  FEchoesCityReserveMissionModel::DistrictDisplayName(
                      NoNeutralPlan.FirstContributingDistrict),
                  FEchoesCityReserveMissionModel::DistrictDisplayName(
                      NoNeutralPlan.SecondContributingDistrict),
                  FEchoesCityReserveMissionModel::DistrictDisplayName(
                      NoNeutralPlan.DeferredDistrict))
        : bFutureThatWon
            ? FString::Printf(
                  TEXT("Oruun and a Kharuun verifier remain commandable. Rhyse appears only as public apparatus; %s stays deferred without casualty or recovery inference."),
                  FEchoesCityReserveMissionModel::DistrictDisplayName(
                      FutureThatWonPlan.DeferredDistrict))
        : bAssemblyOfTheMissing
            ? FString::Printf(
                  TEXT("Oruun and a Kharuun verifier alone are commandable. All three public interfaces remain neutral; %s remains deferred."),
                  FEchoesCityReserveMissionModel::DistrictDisplayName(
                      AssemblyPlan.DeferredDistrict))
        : bSeveralVoicesOneCommand
            ? TEXT("Neme and the local Hollow Choir are commandable. The protected Soldier must resolve to Possible while the protected Heavy remains Manifest; both transitions use visible deterministic timers.")
        : bBrokenSun
            ? FString::Printf(
                  TEXT("Command the local Choir and Neme. Protect neutral Mara/Oruun/Talar. Earned: %s."),
                  *BrokenSunAvailableResolutions)
            : FString::Printf(
                  TEXT("%s forces hold the eastern approach. Every protocol changes what survives."),
                  *OpponentFaction),
             Body, TextLeft, Top + 172.0f * ContentScale, SmallFont, 1.0f * TextScale, false);

    // The authored field brief from the narrative pack, consumed at runtime.
    // Additive presentation: the plan-derived situation lines above stay the
    // mechanical truth; this paragraph is the mission's in-world voice.
    if (const UEchoesNarrativeSubsystem* Narrative =
            GetGameInstance() != nullptr
                ? GetGameInstance()->GetSubsystem<UEchoesNarrativeSubsystem>()
                : nullptr)
    {
        const EEchoesOperationMode BriefOperation =
            BriefingBridge != nullptr
                ? BriefingBridge->GetOperationMode()
                : EEchoesOperationMode::Skirmish;
        const FString AuthoredBrief = Narrative->GetBriefing(BriefOperation);
        if (!AuthoredBrief.IsEmpty())
        {
            const float BriefScale = 0.82f * TextScale;
            const int32 CharsPerLine = FMath::Max(
                40,
                FMath::FloorToInt(
                    (PanelWidth - 84.0f) / (7.2f * BriefScale)));
            TArray<FString> Wrapped;
            FString Remaining = AuthoredBrief;
            while (!Remaining.IsEmpty() && Wrapped.Num() < 3)
            {
                if (Remaining.Len() <= CharsPerLine)
                {
                    Wrapped.Add(Remaining);
                    break;
                }
                int32 Break = CharsPerLine;
                while (Break > 0 && Remaining[Break] != TEXT(' '))
                {
                    --Break;
                }
                if (Break == 0)
                {
                    Break = CharsPerLine;
                }
                Wrapped.Add(Remaining.Left(Break));
                Remaining.RightChopInline(Break + 1);
            }
            for (int32 Index = 0; Index < Wrapped.Num(); ++Index)
            {
                DrawText(
                    Wrapped[Index],
                    Muted,
                    TextLeft,
                    Top + (194.0f + 15.0f * Index) * ContentScale,
                    SmallFont,
                    BriefScale,
                    false);
            }
        }
    }

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
                  TEXT("01  Build [N] Power Links at %s; then synchronize interfaces at %d,%d and %d,%d."),
                  *ContinuancePlayerLinkSites,
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
        : bShapeBesideUs
            ? FString::Printf(
                  TEXT("01  Bring Talar to Neme's first echo at %d,%d, then raise an [N] relay at %d,%d."),
                  BesidePlan.FirstEchoSite.x.FloorToInt(),
                  BesidePlan.FirstEchoSite.y.FloorToInt(),
                  BesidePlan.EchoRelaySite.x.FloorToInt(),
                  BesidePlan.EchoRelaySite.y.FloorToInt())
        : bReserveAuthority
            ? FString::Printf(
                  TEXT("01  Bring Mara to reserve authority at %d,%d, then use workers and [N] Power Links."),
                  ReservePlan.AuthoritySite.x.FloorToInt(),
                  ReservePlan.AuthoritySite.y.FloorToInt())
        : bChoirAtLumeReach
            ? FString::Printf(
                  TEXT("01  Bring Oruun to contact %d,%d, then re-root the Waystone at the deferred liability %d,%d."),
                  ChoirPlan.ContactSite.x.FloorToInt(),
                  ChoirPlan.ContactSite.y.FloorToInt(),
                  ChoirPlan.LiabilitySite.x.FloorToInt(),
                  ChoirPlan.LiabilitySite.y.FloorToInt())
        : bNoNeutralLedger
            ? FString::Printf(
                  TEXT("01  Re-root the Waystone at %d,%d; build [N] Kharuun links within 3 tiles of public interfaces at %d,%d and %d,%d."),
                  NoNeutralPlan.RouteSite.x.FloorToInt(),
                  NoNeutralPlan.RouteSite.y.FloorToInt(),
                  NoNeutralPlan.FirstDistrictSite.x.FloorToInt(),
                  NoNeutralPlan.FirstDistrictSite.y.FloorToInt(),
                  NoNeutralPlan.SecondDistrictSite.x.FloorToInt(),
                  NoNeutralPlan.SecondDistrictSite.y.FloorToInt())
        : bFutureThatWon
            ? FString::Printf(
                  TEXT("01  Establish public readback with Oruun at %d,%d and the verifier at %d,%d; build [N] Kharuun links near inputs %d,%d and %d,%d."),
                  FutureThatWonPlan.KharuunReadbackSite.x.FloorToInt(),
                  FutureThatWonPlan.KharuunReadbackSite.y.FloorToInt(),
                  FutureThatWonPlan.MeridianReadbackSite.x.FloorToInt(),
                  FutureThatWonPlan.MeridianReadbackSite.y.FloorToInt(),
                  FutureThatWonPlan.FirstDistrictInputSite.x.FloorToInt(),
                  FutureThatWonPlan.FirstDistrictInputSite.y.FloorToInt(),
                  FutureThatWonPlan.SecondDistrictInputSite.x.FloorToInt(),
                  FutureThatWonPlan.SecondDistrictInputSite.y.FloorToInt())
        : bAssemblyOfTheMissing
            ? FString::Printf(
                  TEXT("01  Establish public readback with Oruun at %d,%d and the verifier at %d,%d; then build [N] within 3 tiles of the Crownfall index at %d,%d."),
                  AssemblyPlan.KharuunPublicRecordSite.x.FloorToInt(),
                  AssemblyPlan.KharuunPublicRecordSite.y.FloorToInt(),
                  AssemblyPlan.MeridianPublicRecordSite.x.FloorToInt(),
                  AssemblyPlan.MeridianPublicRecordSite.y.FloorToInt(),
                  AssemblyPlan.CrownfallIndexSite.x.FloorToInt(),
                  AssemblyPlan.CrownfallIndexSite.y.FloorToInt())
        : bSeveralVoicesOneCommand
            ? FString::Printf(
                  TEXT("01  Research Held Alternatives [F2]; resolve the protected Soldier to Possible [Shift+F4] at %d,%d, keep the Heavy Manifest at %d,%d, and move Neme to %d,%d."),
                  SeveralVoicesPlan.PossibleVoiceSite.x.FloorToInt(),
                  SeveralVoicesPlan.PossibleVoiceSite.y.FloorToInt(),
                  SeveralVoicesPlan.ManifestVoiceSite.x.FloorToInt(),
                  SeveralVoicesPlan.ManifestVoiceSite.y.FloorToInt(),
                  SeveralVoicesPlan.NemeCommandSite.x.FloorToInt(),
                  SeveralVoicesPlan.NemeCommandSite.y.FloorToInt())
        : bBrokenSun
            ? FString::Printf(
                  TEXT("01  Build APPROACH ANCHOR [M] at %d,%d. Research both [F2]; place Possible, Manifest, and Neme at their marked sites."),
                  BrokenSunPlan.CrownfallApproachSite.x.FloorToInt(),
                  BrokenSunPlan.CrownfallApproachSite.y.FloorToInt())
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
        : bShapeBesideUs
            ? FString::Printf(
                  TEXT("02  Position state witnesses at %d,%d and %d,%d, then bring Talar to the convergence at %d,%d."),
                  BesidePlan.FirstStateSite.x.FloorToInt(),
                  BesidePlan.FirstStateSite.y.FloorToInt(),
                  BesidePlan.SecondStateSite.x.FloorToInt(),
                  BesidePlan.SecondStateSite.y.FloorToInt(),
                  BesidePlan.ConvergenceSite.x.FloorToInt(),
                  BesidePlan.ConvergenceSite.y.FloorToInt())
        : bReserveAuthority
            ? TEXT("02  Power two different districts, keep all three intact, then bring Mara to the deferred district.")
        : bChoirAtLumeReach
            ? FString::Printf(
                  TEXT("02  Build [N] Listening Spines at %d,%d and %d,%d; commit the Well at %d,%d; then move Oruun to the branch resolution."),
                  ChoirPlan.FirstAnchorSite.x.FloorToInt(),
                  ChoirPlan.FirstAnchorSite.y.FloorToInt(),
                  ChoirPlan.SecondAnchorSite.x.FloorToInt(),
                  ChoirPlan.SecondAnchorSite.y.FloorToInt(),
                  ChoirPlan.FutureWellSite.x.FloorToInt(),
                  ChoirPlan.FutureWellSite.y.FloorToInt())
        : bNoNeutralLedger
            ? FString::Printf(
                  TEXT("02  Attest with Oruun at %d,%d and the ledger witness at %d,%d; apply recorded %s at %d,%d; rally both at %d,%d."),
                  NoNeutralPlan.KharuunEvidenceSite.x.FloorToInt(),
                  NoNeutralPlan.KharuunEvidenceSite.y.FloorToInt(),
                  NoNeutralPlan.MeridianEvidenceSite.x.FloorToInt(),
                  NoNeutralPlan.MeridianEvidenceSite.y.FloorToInt(),
                  NoNeutralPlan.ProtocolDisplayName,
                  NoNeutralPlan.FutureWellSite.x.FloorToInt(),
                  NoNeutralPlan.FutureWellSite.y.FloorToInt(),
                  NoNeutralPlan.RallySite.x.FloorToInt(),
                  NoNeutralPlan.RallySite.y.FloorToInt())
        : bFutureThatWon
            ? FString::Printf(
                  TEXT("02  Apply only recorded %s at %d,%d; hold %llu ticks; then observe district readbacks at %d,%d and %d,%d."),
                  FutureThatWonPlan.ProtocolDisplayName,
                  FutureThatWonPlan.FutureWellSite.x.FloorToInt(),
                  FutureThatWonPlan.FutureWellSite.y.FloorToInt(),
                  static_cast<unsigned long long>(
                      FutureThatWonPlan.StabilityWindowTicks),
                  FutureThatWonPlan.FirstDistrictInputSite.x.FloorToInt(),
                  FutureThatWonPlan.FirstDistrictInputSite.y.FloorToInt(),
                  FutureThatWonPlan.SecondDistrictInputSite.x.FloorToInt(),
                  FutureThatWonPlan.SecondDistrictInputSite.y.FloorToInt())
        : bAssemblyOfTheMissing
            ? FString::Printf(
                  TEXT("02  After the public link is complete, move Oruun to %d,%d and the verifier to %d,%d to observe both assembly views."),
                  AssemblyPlan.KharuunAssemblyWitnessSite.x.FloorToInt(),
                  AssemblyPlan.KharuunAssemblyWitnessSite.y.FloorToInt(),
                  AssemblyPlan.MeridianAssemblyWitnessSite.x.FloorToInt(),
                  AssemblyPlan.MeridianAssemblyWitnessSite.y.FloorToInt())
        : bSeveralVoicesOneCommand
            ? FString::Printf(
                  TEXT("02  Research Shared Resolution [F2], build a Phase Anchor [M] at %d,%d, and hold both incompatible voices plus Neme through the visible 8.0-second crisis timer."),
                  SeveralVoicesPlan.CrisisAnchorSite.x.FloorToInt(),
                  SeveralVoicesPlan.CrisisAnchorSite.y.FloorToInt())
        : bBrokenSun
            ? FString::Printf(
                  TEXT("02  Confirm one earned ending twice [Shift+1-4], build its [M] Resolution Conduit, then hold for %llu-%llu ticks."),
                  static_cast<unsigned long long>(
                      BrokenSunPlan.ResolutionHoldTicks),
                  static_cast<unsigned long long>(
                      BrokenSunPlan.ResolutionHoldTicks + 120))
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
             : bShapeBesideUs
                 ? TEXT("The overlap geometry is inherited from all seven prior records; reciprocal contact is not proof of one Choir identity, cause, or authorship.")
             : bReserveAuthority
                 ? TEXT("The doctrine is inherited from eight records, but it is advisory. A third powered district violates the finite-reserve contract.")
             : bChoirAtLumeReach
                 ? TEXT("The prior branch shapes the approach and Mission 09 fixes the deferred liability. This mission's Lume Well is a new, separate irreversible decision.")
             : bNoNeutralLedger
                 ? TEXT("Mission 01 selects the route; Mission 09 selects the two contributing districts; Mission 10 fixes the only admissible Lume protocol. Missions 02–08 remain required evidence seals, not hidden branch variables.")
             : bFutureThatWon
                 ? TEXT("M01 fixes doctrine; M09 fixes the district pair; M10 fixes the protocol. M11 is the admission receipt, not another branch.")
             : bAssemblyOfTheMissing
                 ? TEXT("M01 fixes doctrine; M09 fixes the district pair; M10 fixes the protocol. M11 and M12 remain required public receipts, not new branch variables.")
             : bSeveralVoicesOneCommand
                 ? TEXT("M01 fixes doctrine; M09 fixes the district pair; M10 fixes the protocol. M11–M13 admit this perspective but do not decide the Choir's fate.")
             : bBrokenSun
                 ? TEXT("M01/M09/M10 determine earned endings. M11-M14 are required receipts. There is no hidden morality score.")
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
        : bShapeBesideUs
            ? TEXT("Victory is relay-backed traversal and reciprocal convergence. Any protected loss or terminal Core outcome invalidates the contact evidence.")
        : bReserveAuthority
            ? TEXT("Victory is one exact two-district allocation. It does not establish wider city recovery or unmodeled civilian survival.")
        : bChoirAtLumeReach
            ? TEXT("Victory is one anchored local contact and branch resolution. The Choir is not commandable in Mission 10; no hidden authorship, unified identity, or wider cause is established.")
        : bNoNeutralLedger
            ? TEXT("Victory is one local Kharuun-authoritative coalition rally. Meridian and Choir interfaces remain public and non-commandable; no trust score, casualty total, hidden identity, authorship, or wider causation is inferred.")
        : bFutureThatWon
            ? TEXT("Victory proves one bounded local activation and paired readback—not population restoration, permanence, civilian counts, trust, consent, or moral justification.")
        : bAssemblyOfTheMissing
            ? TEXT("Victory records one bounded public assembly/readback receipt—not authorship, responsibility, trust, consent, civilian state, cryptographic authenticity, or wider cause.")
        : bSeveralVoicesOneCommand
            ? TEXT("Victory completes one bounded Hollow Choir command crisis. It does not decide the Choir's final fate, prove campaign balance, establish ordinary-player completion, or imply release readiness.")
        : bBrokenSun
            ? TEXT("Victory records one ending after its conduit and hold succeed. Other endings stay unchosen; wider consequences and release readiness remain unproven.")
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
        : bShapeBesideUs
            ? TEXT("F9 CHANGES OPERATION  //  ENTER DEPLOYS TALAR + STATE WITNESSES")
        : bReserveAuthority
            ? TEXT("F9 CHANGES OPERATION  //  ENTER DEPLOYS MARA + DISTRICT NETWORK")
        : bChoirAtLumeReach
            ? TEXT("F9 CHANGES OPERATION  //  ENTER DEPLOYS ORUUN + LISTENING FORCE")
        : bNoNeutralLedger
            ? TEXT("F9 CHANGES OPERATION  //  ENTER DEPLOYS ORUUN + LEDGER WITNESS")
        : bFutureThatWon
            ? TEXT("F9 CHANGES OPERATION  //  ENTER DEPLOYS ORUUN + VERIFIER")
        : bAssemblyOfTheMissing
            ? TEXT("F9 CHANGES OPERATION  //  ENTER DEPLOYS ORUUN + VERIFIER")
        : bSeveralVoicesOneCommand
            ? TEXT("F9 CHANGES OPERATION  //  ENTER DEPLOYS NEME + HOLLOW CHOIR")
        : bBrokenSun
            ? TEXT("F9 CHANGES OPERATION  //  ENTER DEPLOYS THE FINAL ACCORD")
            : TEXT("F9 OPERATION  //  TAB FACTION  //  ENTER DEPLOYS"),
             Theme.ActionText,
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
    const FEchoesVisualTheme Theme =
        UEchoesVisualThemeSettings::Resolve(bHighContrast);
    const float HudScale = Settings != nullptr ? Settings->GetHudScale() : 1.0f;
    const FEchoesHudLayout Layout = FEchoesHudLayout::Build(
        FVector2D(Canvas->ClipX, Canvas->ClipY), HudScale, false);
    if (!Layout.bMinimapVisible)
    {
        return;
    }
    const float Size = Layout.MinimapPanel.GetSize().X;
    const float Left = Layout.MinimapPanel.Min.X;
    const float Top = Layout.MinimapPanel.Min.Y;

    const FLinearColor Border = Theme.Accent;
    const FLinearColor Background = Theme.Surface;
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
        FLinearColor Color = Theme.OwnerColor(Entity.owner);
        if (Entity.type == echoes::sim::EntityType::ResourceNode)
        {
            Color = Theme.Warning;
        }
        else if (Entity.type == echoes::sim::EntityType::FutureWell)
        {
            Color = Theme.FutureWellReshape;
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
            const FLinearColor PowerColor = Theme.MissionCritical;
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
        const FLinearColor SignatureColor = Theme.Warning;
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
            EEchoesOperationMode::CampaignShapeOfSilence ||
        Bridge->GetOperationMode() ==
            EEchoesOperationMode::CampaignShapeBesideUs ||
        Bridge->GetOperationMode() ==
            EEchoesOperationMode::CampaignReserveAuthority ||
        Bridge->GetOperationMode() ==
            EEchoesOperationMode::CampaignChoirAtLumeReach ||
        Bridge->GetOperationMode() ==
            EEchoesOperationMode::CampaignNoNeutralLedger ||
        Bridge->GetOperationMode() ==
            EEchoesOperationMode::CampaignFutureThatWon ||
        Bridge->GetOperationMode() ==
            EEchoesOperationMode::CampaignAssemblyOfTheMissing ||
        Bridge->GetOperationMode() ==
            EEchoesOperationMode::CampaignSeveralVoicesOneCommand ||
        Bridge->GetOperationMode() ==
            EEchoesOperationMode::CampaignTheBrokenSun)
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
            const echoes::sim::Simulation* Simulation =
                Bridge->GetSimulation();
            for (const auto& Site : Plan.PlayerPowerLinkSites)
            {
                bool bPlayerLinkComplete = false;
                if (Simulation != nullptr)
                {
                    for (const echoes::sim::Entity& Entity :
                         Simulation->Entities())
                    {
                        if (Entity.owner ==
                                UEchoesSimulationSubsystem::LocalPlayerId &&
                            Entity.faction ==
                                echoes::sim::Faction::MeridianCompact &&
                            Entity.type ==
                                echoes::sim::EntityType::Dropoff &&
                            Entity.position == Site &&
                            Entity.hitPoints > 0 && Entity.completed)
                        {
                            bPlayerLinkComplete = true;
                            break;
                        }
                    }
                }
                DrawMissionSite(
                    Site,
                    TEXT("P"),
                    bPlayerLinkComplete
                        ? FLinearColor(0.25f, 1.0f, 0.66f)
                        : Border);
            }
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
        else if (Bridge->GetOperationMode() ==
                 EEchoesOperationMode::CampaignShapeOfSilence)
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
        else if (Bridge->GetOperationMode() ==
                 EEchoesOperationMode::CampaignShapeBesideUs)
        {
            const FEchoesShapeBesideUsPlan Plan =
                Bridge->GetShapeBesideUsPlan();
            DrawMissionSite(
                Plan.FirstEchoSite,
                TEXT("N"),
                Objective.bFirstEchoObserved
                    ? FLinearColor(0.25f, 1.0f, 0.66f)
                    : Border);
            DrawMissionSite(
                Plan.EchoRelaySite,
                TEXT("R"),
                Objective.bEchoRelayRaised
                    ? FLinearColor(0.25f, 1.0f, 0.66f)
                    : Objective.bFirstEchoObserved
                        ? Border
                        : FLinearColor(0.48f, 0.55f, 0.62f));
            DrawMissionSite(
                Plan.FirstStateSite,
                TEXT("1"),
                Objective.bFirstStateTraversed
                    ? FLinearColor(0.25f, 1.0f, 0.66f)
                    : Objective.bEchoRelayRaised
                        ? Border
                        : FLinearColor(0.48f, 0.55f, 0.62f));
            DrawMissionSite(
                Plan.SecondStateSite,
                TEXT("2"),
                Objective.bSecondStateTraversed
                    ? FLinearColor(0.25f, 1.0f, 0.66f)
                    : Objective.bEchoRelayRaised
                        ? Border
                        : FLinearColor(0.48f, 0.55f, 0.62f));
            DrawMissionSite(
                Plan.ConvergenceSite,
                TEXT("T"),
                Objective.bShapeBesideUsTalarAtConvergence
                    ? FLinearColor(0.25f, 1.0f, 0.66f)
                    : Objective.bFirstStateTraversed &&
                            Objective.bSecondStateTraversed
                        ? Border
                        : FLinearColor(0.48f, 0.55f, 0.62f));
        }
        else if (Bridge->GetOperationMode() ==
                 EEchoesOperationMode::CampaignReserveAuthority)
        {
            const FEchoesReserveAuthorityPlan Plan =
                Bridge->GetReserveAuthorityPlan();
            const int32 PoweredCount =
                (Objective.bLifeSupportPowered ? 1 : 0) +
                (Objective.bTransitPowered ? 1 : 0) +
                (Objective.bArchivePowered ? 1 : 0);
            const bool bAllocationReady = PoweredCount == 2;
            const EEchoesCityDistrict Deferred =
                Objective.ReserveAuthorityDeferredDistrict;
            const FLinearColor DeferredColor =
                bHighContrast ? FLinearColor(1.0f, 0.9f, 0.1f)
                              : FLinearColor(1.0f, 0.72f, 0.18f);
            const FLinearColor WaitingColor(0.48f, 0.55f, 0.62f);
            DrawMissionSite(
                Plan.AuthoritySite,
                TEXT("R"),
                Objective.bReserveAuthoritySecured
                    ? FLinearColor(0.25f, 1.0f, 0.66f)
                    : Border);
            DrawMissionSite(
                FEchoesCityReserveMissionModel::SiteForDistrict(
                    EEchoesCityDistrict::LifeSupport),
                TEXT("L"),
                Objective.bLifeSupportPowered
                    ? FLinearColor(0.25f, 1.0f, 0.66f)
                    : bAllocationReady &&
                            Deferred == EEchoesCityDistrict::LifeSupport
                        ? DeferredColor : WaitingColor);
            DrawMissionSite(
                FEchoesCityReserveMissionModel::SiteForDistrict(
                    EEchoesCityDistrict::Transit),
                TEXT("T"),
                Objective.bTransitPowered
                    ? FLinearColor(0.25f, 1.0f, 0.66f)
                    : bAllocationReady &&
                            Deferred == EEchoesCityDistrict::Transit
                        ? DeferredColor : WaitingColor);
            DrawMissionSite(
                FEchoesCityReserveMissionModel::SiteForDistrict(
                    EEchoesCityDistrict::Archive),
                TEXT("A"),
                Objective.bArchivePowered
                    ? FLinearColor(0.25f, 1.0f, 0.66f)
                    : bAllocationReady &&
                            Deferred == EEchoesCityDistrict::Archive
                        ? DeferredColor : WaitingColor);
            if (const echoes::sim::Entity* Mara =
                    Sim->FindEntity(Objective.ReserveAuthorityMaraId))
            {
                DrawMissionSite(
                    Mara->position,
                    TEXT("M"),
                    Objective.bReserveAuthorityMaraAtDeferredDistrict
                        ? FLinearColor(0.25f, 1.0f, 0.66f)
                        : Border);
            }
        }
        else if (Bridge->GetOperationMode() ==
                 EEchoesOperationMode::CampaignChoirAtLumeReach)
        {
            const FEchoesChoirAtLumeReachPlan Plan =
                Bridge->GetChoirAtLumeReachPlan();
            const FLinearColor CompleteColor(0.25f, 1.0f, 0.66f);
            const FLinearColor WaitingColor(0.48f, 0.55f, 0.62f);
            DrawMissionSite(
                Plan.ContactSite,
                TEXT("C"),
                Objective.bChoirContactEstablished
                    ? CompleteColor : Border);
            DrawMissionSite(
                Plan.LiabilitySite,
                TEXT("L"),
                Objective.bChoirDeferredLiabilityResolved
                    ? CompleteColor
                    : Objective.bChoirContactEstablished
                        ? Border : WaitingColor);
            DrawMissionSite(
                Plan.FirstAnchorSite,
                TEXT("1"),
                Objective.bChoirFirstAnchorRaised
                    ? CompleteColor
                    : Objective.bChoirDeferredLiabilityResolved
                        ? Border : WaitingColor);
            DrawMissionSite(
                Plan.SecondAnchorSite,
                TEXT("2"),
                Objective.bChoirSecondAnchorRaised
                    ? CompleteColor
                    : Objective.bChoirFirstAnchorRaised
                        ? Border : WaitingColor);
            DrawMissionSite(
                Plan.FutureWellSite,
                TEXT("W"),
                Objective.ChoirAtLumeReachWellChoice !=
                        echoes::sim::FutureWellChoice::Dormant
                    ? CompleteColor
                    : Objective.bChoirFirstAnchorRaised &&
                            Objective.bChoirSecondAnchorRaised
                        ? Border : WaitingColor);
            if (Objective.ChoirAtLumeReachWellChoice !=
                echoes::sim::FutureWellChoice::Dormant)
            {
                DrawMissionSite(
                    FEchoesChoirAtLumeReachMissionModel::ResolutionSiteForChoice(
                        Objective.ChoirAtLumeReachWellChoice),
                    TEXT("R"),
                    Objective.bChoirBranchResolutionCompleted
                        ? CompleteColor : Border);
            }
            if (const echoes::sim::Entity* Oruun =
                    Sim->FindEntity(Objective.ChoirAtLumeReachOruunId))
            {
                DrawMissionSite(
                    Oruun->position,
                    TEXT("O"),
                    Objective.bChoirBranchResolutionCompleted
                        ? CompleteColor : Border);
            }
        }
        else if (Bridge->GetOperationMode() ==
                 EEchoesOperationMode::CampaignNoNeutralLedger)
        {
            const FEchoesNoNeutralLedgerPlan Plan =
                Bridge->GetNoNeutralLedgerPlan();
            const FLinearColor CompleteColor(0.25f, 1.0f, 0.66f);
            const FLinearColor WaitingColor(0.48f, 0.55f, 0.62f);
            DrawMissionSite(
                Plan.RouteSite,
                TEXT("R"),
                Objective.bNoNeutralRouteSecured
                    ? CompleteColor : Border);
            DrawMissionSite(
                Plan.FirstDistrictSite,
                TEXT("1"),
                Objective.bNoNeutralFirstDistrictIntegrated
                    ? CompleteColor
                    : Objective.bNoNeutralRouteSecured
                        ? Border : WaitingColor);
            DrawMissionSite(
                Plan.SecondDistrictSite,
                TEXT("2"),
                Objective.bNoNeutralSecondDistrictIntegrated
                    ? CompleteColor
                    : Objective.bNoNeutralRouteSecured
                        ? Border : WaitingColor);
            DrawMissionSite(
                Plan.MeridianEvidenceSite,
                TEXT("M"),
                Objective.bNoNeutralEvidenceAttested
                    ? CompleteColor
                    : Objective.bNoNeutralFirstDistrictIntegrated &&
                            Objective.bNoNeutralSecondDistrictIntegrated
                        ? Border : WaitingColor);
            DrawMissionSite(
                Plan.KharuunEvidenceSite,
                TEXT("K"),
                Objective.bNoNeutralEvidenceAttested
                    ? CompleteColor
                    : Objective.bNoNeutralFirstDistrictIntegrated &&
                            Objective.bNoNeutralSecondDistrictIntegrated
                        ? Border : WaitingColor);
            DrawMissionSite(
                Plan.FutureWellSite,
                TEXT("W"),
                Objective.bNoNeutralProtocolApplied
                    ? CompleteColor
                    : Objective.bNoNeutralEvidenceAttested
                        ? Border : WaitingColor);
            DrawMissionSite(
                Plan.RallySite,
                TEXT("C"),
                Objective.bNoNeutralCoalitionRallied
                    ? CompleteColor
                    : Objective.bNoNeutralProtocolApplied
                        ? Border : WaitingColor);
        }
        else if (Bridge->GetOperationMode() ==
                 EEchoesOperationMode::CampaignFutureThatWon)
        {
            const FEchoesFutureThatWonPlan Plan =
                Bridge->GetFutureThatWonPlan();
            const FLinearColor CompleteColor(0.25f, 1.0f, 0.66f);
            const FLinearColor WaitingColor(0.48f, 0.55f, 0.62f);
            const bool bInputsVerified =
                Objective.bFutureWonFirstInputVerified &&
                Objective.bFutureWonSecondInputVerified;
            const bool bReadbacksObserved =
                Objective.bFutureWonFirstDistrictReadbackObserved &&
                Objective.bFutureWonSecondDistrictReadbackObserved;
            DrawMissionSite(
                Plan.KharuunReadbackSite,
                TEXT("K"),
                Objective.bFutureWonIndependentReadbackEstablished
                    ? CompleteColor : Border);
            DrawMissionSite(
                Plan.MeridianReadbackSite,
                TEXT("M"),
                Objective.bFutureWonIndependentReadbackEstablished
                    ? CompleteColor : Border);
            DrawMissionSite(
                Plan.FirstDistrictInputSite,
                TEXT("1"),
                Objective.bFutureWonFirstDistrictReadbackObserved
                    ? CompleteColor
                : Objective.bFutureWonFirstInputVerified
                    ? Border
                : Objective.bFutureWonIndependentReadbackEstablished
                    ? Border : WaitingColor);
            DrawMissionSite(
                Plan.SecondDistrictInputSite,
                TEXT("2"),
                Objective.bFutureWonSecondDistrictReadbackObserved
                    ? CompleteColor
                : Objective.bFutureWonSecondInputVerified
                    ? Border
                : Objective.bFutureWonIndependentReadbackEstablished
                    ? Border : WaitingColor);
            DrawMissionSite(
                Plan.RestorationDemonstratorSite,
                TEXT("D"),
                Objective.bFutureWonProtocolBound
                    ? CompleteColor
                    : bInputsVerified ? Border : WaitingColor);
            DrawMissionSite(
                Plan.FutureWellSite,
                TEXT("W"),
                bReadbacksObserved
                    ? CompleteColor
                : Objective.bFutureWonProtocolBound
                    ? Border
                    : bInputsVerified ? Border : WaitingColor);
        }
        else if (Bridge->GetOperationMode() ==
                 EEchoesOperationMode::CampaignAssemblyOfTheMissing)
        {
            const FEchoesAssemblyOfTheMissingPlan Plan =
                Bridge->GetAssemblyOfTheMissingPlan();
            const FLinearColor CompleteColor(0.25f, 1.0f, 0.66f);
            const FLinearColor WaitingColor(0.48f, 0.55f, 0.62f);
            DrawMissionSite(
                Plan.MeridianPublicRecordSite,
                TEXT("M"),
                Objective.bAssemblyPublicRecordReadbackEstablished
                    ? CompleteColor : Border);
            DrawMissionSite(
                Plan.KharuunPublicRecordSite,
                TEXT("K"),
                Objective.bAssemblyPublicRecordReadbackEstablished
                    ? CompleteColor : Border);
            DrawMissionSite(
                Plan.CrownfallIndexSite,
                TEXT("C"),
                Objective.bAssemblyCrownfallIndexLinked
                    ? CompleteColor
                    : Objective.bAssemblyPublicRecordReadbackEstablished
                        ? Border : WaitingColor);
            DrawMissionSite(
                Plan.MeridianAssemblyWitnessSite,
                TEXT("1"),
                Objective.bAssemblyMeridianWitnessObserved
                    ? CompleteColor
                    : Objective.bAssemblyCrownfallIndexLinked
                        ? Border : WaitingColor);
            DrawMissionSite(
                Plan.KharuunAssemblyWitnessSite,
                TEXT("2"),
                Objective.bAssemblyKharuunWitnessObserved
                    ? CompleteColor
                    : Objective.bAssemblyCrownfallIndexLinked
                        ? Border : WaitingColor);
        }
        else if (Bridge->GetOperationMode() ==
                 EEchoesOperationMode::CampaignSeveralVoicesOneCommand)
        {
            const FEchoesSeveralVoicesOneCommandPlan Plan =
                Bridge->GetSeveralVoicesOneCommandPlan();
            const FLinearColor CompleteColor(0.25f, 1.0f, 0.66f);
            const FLinearColor WaitingColor(0.48f, 0.55f, 0.62f);
            DrawMissionSite(
                Plan.PossibleVoiceSite,
                TEXT("P"),
                Objective.bSeveralVoicesPossibleAtSite
                    ? CompleteColor : Border);
            DrawMissionSite(
                Plan.ManifestVoiceSite,
                TEXT("M"),
                Objective.bSeveralVoicesManifestAtSite
                    ? CompleteColor : Border);
            DrawMissionSite(
                Plan.NemeCommandSite,
                TEXT("N"),
                Objective.bSeveralVoicesNemeAtCommandSite
                    ? CompleteColor : Border);
            DrawMissionSite(
                Plan.CrisisAnchorSite,
                TEXT("A"),
                Objective.bSeveralVoicesCrisisWindowHeld
                    ? CompleteColor
                : Objective.bSeveralVoicesPhaseAnchorComplete
                    ? Border
                : Objective.bSeveralVoicesSharedResolutionResearched &&
                      Objective.bSeveralVoicesPossibleAtSite &&
                      Objective.bSeveralVoicesManifestAtSite &&
                      Objective.bSeveralVoicesNemeAtCommandSite
                    ? Border : WaitingColor);
        }
        else if (Bridge->GetOperationMode() ==
                 EEchoesOperationMode::CampaignTheBrokenSun)
        {
            const FEchoesBrokenSunPlan Plan = Bridge->GetBrokenSunPlan();
            const FLinearColor CompleteColor(0.25f, 1.0f, 0.66f);
            const FLinearColor WaitingColor(0.48f, 0.55f, 0.62f);
            DrawMissionSite(
                Plan.CrownfallApproachSite,
                TEXT("A"),
                Objective.bBrokenSunApproachSecured
                    ? CompleteColor : Border);
            DrawMissionSite(
                Plan.MaraAccordSite,
                TEXT("M"),
                Objective.bBrokenSunMeridianAccordEstablished
                    ? CompleteColor
                    : Objective.bBrokenSunApproachSecured
                        ? Border : WaitingColor);
            DrawMissionSite(
                Plan.OruunAccordSite,
                TEXT("O"),
                Objective.bBrokenSunKharuunAccordEstablished
                    ? CompleteColor
                    : Objective.bBrokenSunApproachSecured
                        ? Border : WaitingColor);
            DrawMissionSite(
                Plan.NemeAccordSite,
                TEXT("N"),
                Objective.bBrokenSunChoirAccordEstablished
                    ? CompleteColor
                    : Objective.bBrokenSunApproachSecured
                        ? Border : WaitingColor);
            DrawMissionSite(
                Plan.TalarPublicRecordSite,
                TEXT("T"),
                Objective.BrokenSunFinalResolution !=
                        EEchoesFinalResolution::None
                    ? Border : WaitingColor);
            if (Objective.BrokenSunFinalResolution !=
                EEchoesFinalResolution::None)
            {
                DrawMissionSite(
                    FEchoesBrokenSunMissionModel::ResolutionConvergenceSite(
                        Plan,
                        Objective.BrokenSunFinalResolution),
                    TEXT("F"),
                    Objective.bBrokenSunResolutionWindowHeld
                        ? CompleteColor
                    : Objective.bBrokenSunResolutionConduitComplete
                        ? Border : WaitingColor);
            }
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
        : Bridge->GetOperationMode() ==
                EEchoesOperationMode::CampaignShapeBesideUs
            ? TEXT("MISSION NAV  |  NEME + RELAY + PAIRED STATES")
        : Bridge->GetOperationMode() ==
                EEchoesOperationMode::CampaignReserveAuthority
            ? TEXT("MISSION NAV  |  AUTHORITY + L/T/A")
        : Bridge->GetOperationMode() ==
                EEchoesOperationMode::CampaignChoirAtLumeReach
            ? TEXT("MISSION NAV  |  CONTACT + LIABILITY + SPINES + WELL")
        : Bridge->GetOperationMode() ==
                EEchoesOperationMode::CampaignNoNeutralLedger
            ? TEXT("MISSION NAV  |  ROUTE + DISTRICTS + EVIDENCE + RALLY")
        : Bridge->GetOperationMode() ==
                EEchoesOperationMode::CampaignFutureThatWon
            ? TEXT("MISSION NAV  |  M/K + 1/2 + D/W")
        : Bridge->GetOperationMode() ==
                EEchoesOperationMode::CampaignAssemblyOfTheMissing
            ? TEXT("MISSION NAV  |  M/K + C + 1/2")
        : Bridge->GetOperationMode() ==
                EEchoesOperationMode::CampaignSeveralVoicesOneCommand
            ? TEXT("MISSION NAV  |  P/M/N/A")
        : Bridge->GetOperationMode() ==
                EEchoesOperationMode::CampaignTheBrokenSun
            ? TEXT("MISSION NAV  |  A + M/O/N/T + F")
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
    const FEchoesVisualTheme Theme =
        UEchoesVisualThemeSettings::Resolve(bHighContrast);
    const float HudScale = Settings != nullptr ? Settings->GetHudScale() : 1.0f;
    const FLinearColor SignatureColor = Theme.Warning;
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
        const FLinearColor AlertBackdrop =
            Theme.WithAlpha(Theme.ElevatedSurface, 0.96f);
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
            Theme.TextPrimary,
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
        const FLinearColor LabelBackdrop =
            Theme.WithAlpha(Theme.ElevatedSurface, 0.94f);
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
            Theme.TextSecondary,
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
    const FEchoesVisualTheme Theme =
        UEchoesVisualThemeSettings::Resolve(bHighContrast);
    const FLinearColor BorderColor = Theme.Selected;

    DrawRect(
        Theme.WithAlpha(Theme.Selected, bHighContrast ? 0.18f : 0.10f),
        MinX,
        MinY,
        MaxX - MinX,
        MaxY - MinY);
    DrawLine(MinX, MinY, MaxX, MinY, BorderColor, 1.5f);
    DrawLine(MaxX, MinY, MaxX, MaxY, BorderColor, 1.5f);
    DrawLine(MaxX, MaxY, MinX, MaxY, BorderColor, 1.5f);
    DrawLine(MinX, MaxY, MinX, MinY, BorderColor, 1.5f);
}
