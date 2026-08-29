#include "EchoesHUD.h"

#include "EchoesEntityView.h"
#include "EchoesPlayerController.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

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

    DrawRect(FLinearColor(0.008f, 0.018f, 0.035f, 0.88f), 18.0f, 18.0f, 920.0f, 227.0f);
    DrawText(
        TEXT("ECHOES OF THE BROKEN SUN  |  PLAYABLE SYSTEMS BUILD — ACTIVE DEVELOPMENT"),
        FLinearColor(0.15f, 0.88f, 1.0f),
        34.0f,
        31.0f,
        GEngine != nullptr ? GEngine->GetSmallFont() : nullptr,
        1.08f,
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
            else if (Outcome == echoes::sim::MatchOutcome::Player1Victory)
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
        34.0f,
        58.0f,
        GEngine != nullptr ? GEngine->GetSmallFont() : nullptr,
        1.0f,
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
        34.0f,
        82.0f,
        GEngine != nullptr ? GEngine->GetSmallFont() : nullptr,
        1.0f,
        false);

    DrawText(
        TEXT("WASD / screen edge: pan    Wheel: zoom    LMB / drag: select    Shift: add/remove    RMB: context order"),
        FLinearColor(0.73f, 0.76f, 0.82f),
        34.0f,
        108.0f,
        GEngine != nullptr ? GEngine->GetSmallFont() : nullptr,
        0.86f,
        false);
    DrawText(
        TEXT("[F] Attack-move    [H] Hold    [X] Stop    [B] Barracks    [N] Drop-off    [Q] Worker    [E] Soldier"),
        FLinearColor(0.73f, 0.76f, 0.82f),
        34.0f,
        131.0f,
        GEngine != nullptr ? GEngine->GetSmallFont() : nullptr,
        0.86f,
        false);
    DrawText(
        TEXT("[1-0] Recall group    [G then 1-0] Assign group    [P] Pause    [R] Restart"),
        FLinearColor(0.73f, 0.76f, 0.82f),
        34.0f,
        154.0f,
        GEngine != nullptr ? GEngine->GetSmallFont() : nullptr,
        0.86f,
        false);
    DrawText(
        TEXT("[Z] Harvest    [C] Preserve    [V] Reshape    Cyan: Meridian    Red: Kharuun"),
        FLinearColor(0.73f, 0.76f, 0.82f),
        34.0f,
        177.0f,
        GEngine != nullptr ? GEngine->GetSmallFont() : nullptr,
        0.86f,
        false);
    DrawText(
        TEXT("[K] Checkpoint    [L] Load Checkpoint    Validated with one backup"),
        FLinearColor(0.73f, 0.76f, 0.82f),
        34.0f,
        200.0f,
        GEngine != nullptr ? GEngine->GetSmallFont() : nullptr,
        0.86f,
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
                FLinearColor(0.008f, 0.018f, 0.035f, 0.88f),
                18.0f,
                Canvas != nullptr ? Canvas->ClipY - 72.0f : 700.0f,
                FeedbackWidth,
                48.0f);
            DrawText(
                Feedback,
                Feedback.StartsWith(TEXT("["))
                    ? FLinearColor(1.0f, 0.48f, 0.18f)
                    : FLinearColor(0.25f, 1.0f, 0.66f),
                34.0f,
                Canvas != nullptr ? Canvas->ClipY - 58.0f : 714.0f,
                GEngine != nullptr ? GEngine->GetSmallFont() : nullptr,
                0.95f,
                false);
        }
    }

    DrawSelectionRectangle();
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
    const FLinearColor BorderColor(0.12f, 0.92f, 1.0f, 0.95f);

    DrawRect(FLinearColor(0.12f, 0.75f, 1.0f, 0.10f), MinX, MinY, MaxX - MinX, MaxY - MinY);
    DrawLine(MinX, MinY, MaxX, MinY, BorderColor, 1.5f);
    DrawLine(MaxX, MinY, MaxX, MaxY, BorderColor, 1.5f);
    DrawLine(MaxX, MaxY, MinX, MaxY, BorderColor, 1.5f);
    DrawLine(MinX, MaxY, MinX, MinY, BorderColor, 1.5f);
}
