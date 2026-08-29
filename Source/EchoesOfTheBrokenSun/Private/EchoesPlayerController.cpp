#include "EchoesPlayerController.h"

#include "EchoesEntityView.h"
#include "EchoesOfTheBrokenSun.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"

namespace
{
constexpr float DragSelectionThresholdPixels = 8.0f;
constexpr float FormationSpacingWorldUnits = 150.0f;
}

AEchoesPlayerController::AEchoesPlayerController()
{
    bShowMouseCursor = true;
    bEnableClickEvents = false;
    bEnableMouseOverEvents = true;
    DefaultMouseCursor = EMouseCursor::Crosshairs;
}

void AEchoesPlayerController::BeginPlay()
{
    Super::BeginPlay();

    FInputModeGameAndUI InputMode;
    InputMode.SetHideCursorDuringCapture(false);
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(InputMode);
    bShowMouseCursor = true;
    if (!bRuntimeStateKnown)
    {
        const UEchoesSimulationSubsystem* Bridge =
            GetWorld() != nullptr
                ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
                : nullptr;
        if (Bridge != nullptr && Bridge->IsScenarioReady())
        {
            NotifyRuntimeReady();
        }
        else
        {
            SetStatusMessage(
                TEXT("Initializing runtime technical prototype..."),
                15.0f);
        }
    }
}

void AEchoesPlayerController::NotifyRuntimeReady()
{
    bRuntimeStateKnown = true;
    SetStatusMessage(
        TEXT("Runtime prototype ready. Select Meridian units, then right-click a destination or target."),
        7.0f);
}

void AEchoesPlayerController::NotifyRuntimeFailure(const FString& FailureCode)
{
    bRuntimeStateKnown = true;
    SetStatusMessage(
        FString::Printf(
            TEXT("[%s] Runtime prototype initialization failed; inspect LogEchoes."),
            *FailureCode),
        15.0f);
}

void AEchoesPlayerController::NotifyMatchFinished(
    echoes::sim::MatchOutcome Outcome)
{
    ClearSelection();
    FString Message =
        TEXT("DRAW — both Command Cores fell in the same deterministic tick. Press R to restart.");
    if (Outcome == echoes::sim::MatchOutcome::Player0Victory)
    {
        Message =
            TEXT("VICTORY — the opposing Command Core has fallen. Press R to restart.");
    }
    else if (Outcome == echoes::sim::MatchOutcome::Player1Victory)
    {
        Message =
            TEXT("DEFEAT — your Command Core has fallen. Press R to restart.");
    }
    SetStatusMessage(Message, 3600.0f);
}

void AEchoesPlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);
    if (bSelectionButtonDown)
    {
        float MouseX = 0.0f;
        float MouseY = 0.0f;
        if (GetMousePosition(MouseX, MouseY))
        {
            SelectionCurrentScreenPosition = FVector2D(MouseX, MouseY);
        }
    }
    PruneSelection();
}

void AEchoesPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    check(InputComponent != nullptr);

    InputComponent->BindAction(
        TEXT("Select"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::SelectionPressed);
    InputComponent->BindAction(
        TEXT("Select"),
        IE_Released,
        this,
        &AEchoesPlayerController::SelectionReleased);
    InputComponent->BindAction(
        TEXT("ContextOrder"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::ContextOrderPressed);
    InputComponent->BindAction(
        TEXT("ChooseHarvest"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::ChooseHarvest);
    InputComponent->BindAction(
        TEXT("ChoosePreserve"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::ChoosePreserve);
    InputComponent->BindAction(
        TEXT("ChooseReshape"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::ChooseReshape);
    InputComponent->BindAction(
        TEXT("BuildBarracks"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::BuildBarracks);
    InputComponent->BindAction(
        TEXT("BuildDropoff"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::BuildDropoff);
    InputComponent->BindAction(
        TEXT("ProduceWorker"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::ProduceWorker);
    InputComponent->BindAction(
        TEXT("ProduceSoldier"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::ProduceSoldier);
    InputComponent->BindAction(
        TEXT("AttackMoveAtCursor"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::AttackMoveAtCursor);
    InputComponent->BindAction(
        TEXT("StopSelected"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::StopSelectedUnits);
    InputComponent->BindAction(
        TEXT("PauseScenario"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::TogglePause);
    InputComponent->BindAction(
        TEXT("RestartScenario"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::RestartScenario);
    InputComponent->BindAction(
        TEXT("QuickSaveScenario"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::QuickSaveScenario);
    InputComponent->BindAction(
        TEXT("QuickLoadScenario"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::QuickLoadScenario);
}

void AEchoesPlayerController::SelectionPressed()
{
    float MouseX = 0.0f;
    float MouseY = 0.0f;
    if (!GetMousePosition(MouseX, MouseY))
    {
        SetStatusMessage(TEXT("[CURSOR_UNAVAILABLE] Selection could not read the pointer position."));
        return;
    }
    SelectionStartScreenPosition = FVector2D(MouseX, MouseY);
    SelectionCurrentScreenPosition = SelectionStartScreenPosition;
    bSelectionButtonDown = true;
}

void AEchoesPlayerController::SelectionReleased()
{
    if (!bSelectionButtonDown)
    {
        return;
    }

    float MouseX = SelectionCurrentScreenPosition.X;
    float MouseY = SelectionCurrentScreenPosition.Y;
    if (GetMousePosition(MouseX, MouseY))
    {
        SelectionCurrentScreenPosition = FVector2D(MouseX, MouseY);
    }
    bSelectionButtonDown = false;

    const bool bAdditive = IsInputKeyDown(EKeys::LeftShift) ||
                           IsInputKeyDown(EKeys::RightShift);
    if (FVector2D::Distance(
            SelectionStartScreenPosition,
            SelectionCurrentScreenPosition) >= DragSelectionThresholdPixels)
    {
        SelectInScreenRectangle(bAdditive);
    }
    else
    {
        SelectAtCursor(bAdditive);
    }
}

void AEchoesPlayerController::SelectAtCursor(bool bAdditive)
{
    FHitResult HitResult;
    AEchoesEntityView* View = nullptr;
    if (TraceCursor(HitResult))
    {
        View = Cast<AEchoesEntityView>(HitResult.GetActor());
    }

    if (View == nullptr ||
        View->GetOwnerPlayerId() != UEchoesSimulationSubsystem::LocalPlayerId)
    {
        if (!bAdditive)
        {
            ClearSelection();
        }
        return;
    }

    const uint32 EntityId = View->GetEntityId();
    if (!bAdditive)
    {
        ClearSelection();
    }

    if (bAdditive && SelectedEntityIds.Contains(EntityId))
    {
        SetEntitySelected(EntityId, false);
        SelectedEntityIds.Remove(EntityId);
    }
    else if (!SelectedEntityIds.Contains(EntityId))
    {
        SelectedEntityIds.Add(EntityId);
        SetEntitySelected(EntityId, true);
    }

    SetStatusMessage(
        FString::Printf(
            TEXT("Selected %d Meridian entit%s."),
            SelectedEntityIds.Num(),
            SelectedEntityIds.Num() == 1 ? TEXT("y") : TEXT("ies")),
        2.0f);
}

void AEchoesPlayerController::SelectInScreenRectangle(bool bAdditive)
{
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    const echoes::sim::Simulation* Sim =
        Bridge != nullptr ? Bridge->GetSimulation() : nullptr;
    if (Bridge == nullptr || Sim == nullptr)
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Drag selection is unavailable."));
        return;
    }

    if (!bAdditive)
    {
        ClearSelection();
    }

    const float MinX = FMath::Min(
        SelectionStartScreenPosition.X,
        SelectionCurrentScreenPosition.X);
    const float MaxX = FMath::Max(
        SelectionStartScreenPosition.X,
        SelectionCurrentScreenPosition.X);
    const float MinY = FMath::Min(
        SelectionStartScreenPosition.Y,
        SelectionCurrentScreenPosition.Y);
    const float MaxY = FMath::Max(
        SelectionStartScreenPosition.Y,
        SelectionCurrentScreenPosition.Y);

    for (const echoes::sim::Entity& Entity : Sim->Entities())
    {
        if (Entity.owner != UEchoesSimulationSubsystem::LocalPlayerId)
        {
            continue;
        }
        AEchoesEntityView* View = Bridge->FindEntityView(Entity.id);
        if (View == nullptr)
        {
            continue;
        }

        FVector2D ScreenPosition;
        if (ProjectWorldLocationToScreen(
                View->GetActorLocation() + FVector(0.0f, 0.0f, 60.0f),
                ScreenPosition,
                false) &&
            ScreenPosition.X >= MinX && ScreenPosition.X <= MaxX &&
            ScreenPosition.Y >= MinY && ScreenPosition.Y <= MaxY &&
            !SelectedEntityIds.Contains(Entity.id))
        {
            SelectedEntityIds.Add(Entity.id);
            View->SetSelected(true);
        }
    }

    SetStatusMessage(
        FString::Printf(
            TEXT("Drag-selected %d Meridian entit%s."),
            SelectedEntityIds.Num(),
            SelectedEntityIds.Num() == 1 ? TEXT("y") : TEXT("ies")),
        2.0f);
}

void AEchoesPlayerController::ContextOrderPressed()
{
    PruneSelection();
    if (SelectedEntityIds.IsEmpty())
    {
        SetStatusMessage(TEXT("[NO_SELECTION] Select one or more Meridian units first."));
        return;
    }

    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Orders cannot be issued."));
        return;
    }
    if (Bridge->GetMatchOutcome() != echoes::sim::MatchOutcome::Ongoing)
    {
        SetStatusMessage(TEXT("[MATCH_FINISHED] Press R to restart."));
        return;
    }

    FHitResult HitResult;
    if (!TraceCursor(HitResult))
    {
        SetStatusMessage(TEXT("[NO_WORLD_HIT] Point at the battlefield or an entity."));
        return;
    }

    const AEchoesEntityView* TargetView =
        Cast<AEchoesEntityView>(HitResult.GetActor());
    const echoes::sim::Entity* TargetEntity =
        TargetView != nullptr
            ? Bridge->FindEntity(TargetView->GetEntityId())
            : nullptr;

    echoes::sim::CommandType CommandType = echoes::sim::CommandType::Move;
    uint32 TargetId = 0;
    FVector Destination = HitResult.Location;
    if (TargetEntity != nullptr)
    {
        TargetId = TargetEntity->id;
        Destination = Bridge->SimToWorld(TargetEntity->position);
        if (TargetEntity->type == echoes::sim::EntityType::ResourceNode)
        {
            CommandType = echoes::sim::CommandType::Gather;
        }
        else if (TargetEntity->type == echoes::sim::EntityType::FutureWell)
        {
            CommandType = echoes::sim::CommandType::FutureWell;
        }
        else if (TargetEntity->owner != echoes::sim::kNeutralPlayer &&
                 TargetEntity->owner != UEchoesSimulationSubsystem::LocalPlayerId)
        {
            CommandType = echoes::sim::CommandType::Attack;
        }
        else if (TargetEntity->owner == UEchoesSimulationSubsystem::LocalPlayerId &&
                 (TargetEntity->type == echoes::sim::EntityType::CommandCore ||
                  TargetEntity->type == echoes::sim::EntityType::Dropoff))
        {
            CommandType = echoes::sim::CommandType::Deliver;
        }
    }

    const int32 UnitCount = SelectedEntityIds.Num();
    const int32 FormationWidth = FMath::Max(1, FMath::CeilToInt(FMath::Sqrt(static_cast<float>(UnitCount))));
    int32 AcceptedCount = 0;
    int32 RejectedCount = 0;
    FString LastRejection;
    for (int32 Index = 0; Index < UnitCount; ++Index)
    {
        echoes::sim::CommandType ActorCommandType = CommandType;
        uint32 ActorTargetId = TargetId;
        const echoes::sim::Entity* ActorState =
            Bridge->FindEntity(SelectedEntityIds[Index]);
        if (CommandType == echoes::sim::CommandType::Deliver &&
            (ActorState == nullptr ||
             ActorState->type != echoes::sim::EntityType::Worker ||
             ActorState->cargo <= 0))
        {
            ActorCommandType = echoes::sim::CommandType::Move;
            ActorTargetId = 0;
        }

        FVector UnitDestination = Destination;
        if (ActorCommandType == echoes::sim::CommandType::Move)
        {
            const int32 Row = Index / FormationWidth;
            const int32 Column = Index % FormationWidth;
            UnitDestination.X +=
                (static_cast<float>(Column) -
                 static_cast<float>(FormationWidth - 1) * 0.5f) *
                FormationSpacingWorldUnits;
            UnitDestination.Y +=
                (static_cast<float>(Row) -
                 static_cast<float>(FormationWidth - 1) * 0.5f) *
                FormationSpacingWorldUnits;
        }

        FString Feedback;
        if (Bridge->IssueCommand(
                ActorCommandType,
                SelectedEntityIds[Index],
                ActorTargetId,
                UnitDestination,
                FutureWellChoice,
                Feedback))
        {
            ++AcceptedCount;
        }
        else
        {
            ++RejectedCount;
            LastRejection = Feedback;
        }
    }

    if (AcceptedCount > 0)
    {
        const FString OrderLabel =
            CommandType == echoes::sim::CommandType::Deliver
                ? TEXT("CONTEXT MOVE / DELIVER")
                : CommandLabel(CommandType);
        const FString RejectionSuffix =
            RejectedCount > 0
                ? FString::Printf(TEXT(", %d rejected."), RejectedCount)
                : TEXT(".");
        SetStatusMessage(
            FString::Printf(
                TEXT("%s: %d queued%s"),
                *OrderLabel,
                AcceptedCount,
                *RejectionSuffix));
    }
    else
    {
        SetStatusMessage(LastRejection.IsEmpty()
                             ? TEXT("[ORDER_REJECTED] No selected entity accepted the order.")
                             : LastRejection);
    }
}

void AEchoesPlayerController::SetEntitySelected(uint32 EntityId, bool bSelected)
{
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge != nullptr)
    {
        if (AEchoesEntityView* View = Bridge->FindEntityView(EntityId))
        {
            View->SetSelected(bSelected);
        }
    }
}

void AEchoesPlayerController::ClearSelection()
{
    for (const uint32 EntityId : SelectedEntityIds)
    {
        SetEntitySelected(EntityId, false);
    }
    SelectedEntityIds.Reset();
}

void AEchoesPlayerController::PruneSelection()
{
    const UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    for (int32 Index = SelectedEntityIds.Num() - 1; Index >= 0; --Index)
    {
        const echoes::sim::Entity* Entity =
            Bridge != nullptr ? Bridge->FindEntity(SelectedEntityIds[Index]) : nullptr;
        if (Entity == nullptr ||
            Entity->owner != UEchoesSimulationSubsystem::LocalPlayerId)
        {
            SelectedEntityIds.RemoveAtSwap(Index, 1, EAllowShrinking::No);
        }
    }
}

bool AEchoesPlayerController::TraceCursor(FHitResult& OutHitResult)
{
    return GetHitResultUnderCursorByChannel(
        UEngineTypes::ConvertToTraceType(ECC_Visibility),
        true,
        OutHitResult);
}

void AEchoesPlayerController::ChooseHarvest()
{
    SetFutureWellChoice(echoes::sim::FutureWellChoice::Harvest);
}

void AEchoesPlayerController::ChoosePreserve()
{
    SetFutureWellChoice(echoes::sim::FutureWellChoice::Preserve);
}

void AEchoesPlayerController::ChooseReshape()
{
    SetFutureWellChoice(echoes::sim::FutureWellChoice::Reshape);
}

void AEchoesPlayerController::BuildBarracks()
{
    BuildAtCursor(echoes::sim::EntityType::Barracks);
}

void AEchoesPlayerController::BuildDropoff()
{
    BuildAtCursor(echoes::sim::EntityType::Dropoff);
}

void AEchoesPlayerController::ProduceWorker()
{
    ProduceUnit(echoes::sim::EntityType::Worker);
}

void AEchoesPlayerController::ProduceSoldier()
{
    ProduceUnit(echoes::sim::EntityType::Soldier);
}

void AEchoesPlayerController::AttackMoveAtCursor()
{
    PruneSelection();
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Attack-move is unavailable."));
        return;
    }
    if (Bridge->GetMatchOutcome() != echoes::sim::MatchOutcome::Ongoing)
    {
        SetStatusMessage(TEXT("[MATCH_FINISHED] Press R to restart."));
        return;
    }
    if (SelectedEntityIds.IsEmpty())
    {
        SetStatusMessage(TEXT("[NO_SELECTION] Select one or more Meridian combat units first."));
        return;
    }
    FHitResult HitResult;
    if (!TraceCursor(HitResult))
    {
        SetStatusMessage(TEXT("[NO_WORLD_HIT] Point at the attack-move destination."));
        return;
    }

    const int32 UnitCount = SelectedEntityIds.Num();
    const int32 FormationWidth = FMath::Max(
        1,
        FMath::CeilToInt(FMath::Sqrt(static_cast<float>(UnitCount))));
    int32 AcceptedCount = 0;
    int32 RejectedCount = 0;
    FString LastRejection;
    for (int32 Index = 0; Index < UnitCount; ++Index)
    {
        const int32 Row = Index / FormationWidth;
        const int32 Column = Index % FormationWidth;
        FVector UnitDestination = HitResult.Location;
        UnitDestination.X +=
            (static_cast<float>(Column) -
             static_cast<float>(FormationWidth - 1) * 0.5f) *
            FormationSpacingWorldUnits;
        UnitDestination.Y +=
            (static_cast<float>(Row) -
             static_cast<float>(FormationWidth - 1) * 0.5f) *
            FormationSpacingWorldUnits;
        FString Feedback;
        if (Bridge->IssueCommand(
                echoes::sim::CommandType::AttackMove,
                SelectedEntityIds[Index],
                0,
                UnitDestination,
                FutureWellChoice,
                Feedback))
        {
            ++AcceptedCount;
        }
        else
        {
            ++RejectedCount;
            LastRejection = Feedback;
        }
    }
    if (AcceptedCount > 0)
    {
        const FString RejectionSuffix =
            RejectedCount > 0
                ? FString::Printf(TEXT(", %d rejected."), RejectedCount)
                : TEXT(".");
        SetStatusMessage(FString::Printf(
            TEXT("ATTACK-MOVE: %d queued%s"),
            AcceptedCount,
            *RejectionSuffix));
    }
    else
    {
        SetStatusMessage(
            LastRejection.IsEmpty()
                ? TEXT("[ATTACK_MOVE_REJECTED] No selected entity can attack-move.")
                : LastRejection);
    }
}

void AEchoesPlayerController::StopSelectedUnits()
{
    PruneSelection();
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Stop is unavailable."));
        return;
    }
    if (SelectedEntityIds.IsEmpty())
    {
        SetStatusMessage(TEXT("[NO_SELECTION] Select one or more Meridian units first."));
        return;
    }
    int32 AcceptedCount = 0;
    FString LastRejection;
    for (const uint32 EntityId : SelectedEntityIds)
    {
        const echoes::sim::Entity* Entity = Bridge->FindEntity(EntityId);
        FString Feedback;
        if (Entity != nullptr && Bridge->IssueCommand(
                echoes::sim::CommandType::Stop,
                EntityId,
                0,
                Bridge->SimToWorld(Entity->position),
                FutureWellChoice,
                Feedback))
        {
            ++AcceptedCount;
        }
        else
        {
            LastRejection = Feedback;
        }
    }
    SetStatusMessage(
        AcceptedCount > 0
            ? FString::Printf(TEXT("STOP: %d unit%s ordered to hold."),
                              AcceptedCount,
                              AcceptedCount == 1 ? TEXT("") : TEXT("s"))
            : LastRejection.IsEmpty()
                  ? TEXT("[STOP_REJECTED] No selected entity accepted the order.")
                  : LastRejection);
}

void AEchoesPlayerController::QuickSaveScenario()
{
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    FString Feedback;
    if (Bridge == nullptr)
    {
        Feedback = TEXT("[SAVE_SIM_NOT_READY] No active scenario can be saved.");
    }
    else
    {
        Bridge->QuickSaveScenario(Feedback);
    }
    SetStatusMessage(Feedback, 6.0f);
}

void AEchoesPlayerController::QuickLoadScenario()
{
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    FString Feedback;
    if (Bridge == nullptr)
    {
        Feedback = TEXT("[LOAD_SIM_NOT_READY] Start a scenario before loading.");
    }
    else if (Bridge->QuickLoadScenario(Feedback))
    {
        ClearSelection();
    }
    SetStatusMessage(Feedback, 7.0f);
}

void AEchoesPlayerController::BuildAtCursor(
    echoes::sim::EntityType BuildingType)
{
    PruneSelection();
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Construction is unavailable."));
        return;
    }
    uint32 WorkerId = 0;
    for (const uint32 EntityId : SelectedEntityIds)
    {
        const echoes::sim::Entity* Entity = Bridge->FindEntity(EntityId);
        if (Entity != nullptr &&
            Entity->type == echoes::sim::EntityType::Worker)
        {
            WorkerId = EntityId;
            break;
        }
    }
    if (WorkerId == 0)
    {
        SetStatusMessage(
            TEXT("[BUILD_REQUIRES_WORKER] Select a worker, point at open ground, then press B or N."));
        return;
    }
    FHitResult HitResult;
    if (!TraceCursor(HitResult))
    {
        SetStatusMessage(TEXT("[NO_WORLD_HIT] Point at open battlefield ground."));
        return;
    }
    FString Feedback;
    if (Bridge->IssueBuildCommand(
            WorkerId,
            BuildingType,
            HitResult.Location,
            Feedback))
    {
        SetStatusMessage(
            BuildingType == echoes::sim::EntityType::Barracks
                ? TEXT("BARRACKS: construction order queued.")
                : TEXT("DROP-OFF: construction order queued."));
    }
    else
    {
        SetStatusMessage(Feedback);
    }
}

void AEchoesPlayerController::ProduceUnit(echoes::sim::EntityType UnitType)
{
    PruneSelection();
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Production is unavailable."));
        return;
    }
    int32 Accepted = 0;
    FString LastFeedback;
    for (const uint32 EntityId : SelectedEntityIds)
    {
        const echoes::sim::Entity* Entity = Bridge->FindEntity(EntityId);
        const bool bCompatible = Entity != nullptr &&
            ((UnitType == echoes::sim::EntityType::Worker &&
              Entity->type == echoes::sim::EntityType::CommandCore) ||
             (UnitType == echoes::sim::EntityType::Soldier &&
              Entity->type == echoes::sim::EntityType::Barracks));
        if (!bCompatible)
        {
            continue;
        }
        FString Feedback;
        if (Bridge->IssueProductionCommand(EntityId, UnitType, Feedback))
        {
            ++Accepted;
        }
        else
        {
            LastFeedback = Feedback;
        }
    }
    if (Accepted > 0)
    {
        SetStatusMessage(
            FString::Printf(
                TEXT("%s: %d production order%s queued."),
                UnitType == echoes::sim::EntityType::Worker
                    ? TEXT("WORKER")
                    : TEXT("SOLDIER"),
                Accepted,
                Accepted == 1 ? TEXT("") : TEXT("s")));
    }
    else
    {
        SetStatusMessage(
            LastFeedback.IsEmpty()
                ? TEXT("[NO_COMPATIBLE_PRODUCER] Select a Command Core for Q or a Barracks for E.")
                : LastFeedback);
    }
}

void AEchoesPlayerController::TogglePause()
{
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Pause is unavailable."));
        return;
    }
    if (Bridge->GetMatchOutcome() != echoes::sim::MatchOutcome::Ongoing)
    {
        SetStatusMessage(TEXT("[MATCH_FINISHED] Press R to restart."));
        return;
    }
    const bool bPause = !Bridge->IsScenarioPaused();
    Bridge->SetScenarioPaused(bPause);
    SetStatusMessage(
        bPause ? TEXT("MATCH PAUSED — press P to resume.")
               : TEXT("MATCH RESUMED."),
        bPause ? 3600.0f : 3.0f);
}

void AEchoesPlayerController::RestartScenario()
{
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    ClearSelection();
    if (Bridge != nullptr && Bridge->RestartPrototypeScenario())
    {
        bRuntimeStateKnown = true;
        SetStatusMessage(TEXT("MATCH RESTARTED — deterministic initial state restored."));
    }
    else
    {
        NotifyRuntimeFailure(TEXT("ECHOES_MATCH_RESTART_FAILED"));
    }
}

void AEchoesPlayerController::SetFutureWellChoice(
    echoes::sim::FutureWellChoice Choice)
{
    FutureWellChoice = Choice;
    SetStatusMessage(
        FString::Printf(
            TEXT("Future Well protocol set to %s. Right-click a dormant Well with a worker selected."),
            *GetFutureWellChoiceLabel()),
        5.0f);
}

void AEchoesPlayerController::SetStatusMessage(
    const FString& Message,
    float DisplaySeconds)
{
    StatusMessage = Message;
    StatusMessageExpiresAt =
        GetWorld() != nullptr ? GetWorld()->GetTimeSeconds() + DisplaySeconds : 0.0;
    UE_LOG(LogEchoes, Display, TEXT("[ECHOES_PLAYER_FEEDBACK] %s"), *Message);
}

bool AEchoesPlayerController::IsDraggingSelection() const
{
    return bSelectionButtonDown &&
           FVector2D::Distance(
               SelectionStartScreenPosition,
               SelectionCurrentScreenPosition) >= DragSelectionThresholdPixels;
}

FVector2D AEchoesPlayerController::GetSelectionStartScreenPosition() const
{
    return SelectionStartScreenPosition;
}

FVector2D AEchoesPlayerController::GetSelectionCurrentScreenPosition() const
{
    return SelectionCurrentScreenPosition;
}

const TArray<uint32>& AEchoesPlayerController::GetSelectedEntityIds() const
{
    return SelectedEntityIds;
}

echoes::sim::FutureWellChoice AEchoesPlayerController::GetFutureWellChoice() const
{
    return FutureWellChoice;
}

FString AEchoesPlayerController::GetFutureWellChoiceLabel() const
{
    switch (FutureWellChoice)
    {
        case echoes::sim::FutureWellChoice::Harvest:
            return TEXT("HARVEST");
        case echoes::sim::FutureWellChoice::Preserve:
            return TEXT("PRESERVE");
        case echoes::sim::FutureWellChoice::Reshape:
            return TEXT("RESHAPE");
        case echoes::sim::FutureWellChoice::Dormant:
            return TEXT("DORMANT");
    }
    return TEXT("UNKNOWN");
}

FString AEchoesPlayerController::GetStatusMessage() const
{
    if (GetWorld() == nullptr || GetWorld()->GetTimeSeconds() > StatusMessageExpiresAt)
    {
        return FString();
    }
    return StatusMessage;
}

FString AEchoesPlayerController::CommandLabel(
    echoes::sim::CommandType CommandType) const
{
    switch (CommandType)
    {
        case echoes::sim::CommandType::Stop:
            return TEXT("STOP");
        case echoes::sim::CommandType::Move:
            return TEXT("MOVE");
        case echoes::sim::CommandType::Gather:
            return TEXT("GATHER MATTER");
        case echoes::sim::CommandType::Deliver:
            return TEXT("DELIVER MATTER");
        case echoes::sim::CommandType::Build:
            return TEXT("BUILD");
        case echoes::sim::CommandType::Attack:
            return TEXT("ATTACK");
        case echoes::sim::CommandType::FutureWell:
            return FString::Printf(TEXT("FUTURE WELL: %s"), *GetFutureWellChoiceLabel());
        case echoes::sim::CommandType::Produce:
            return TEXT("PRODUCE");
        case echoes::sim::CommandType::AttackMove:
            return TEXT("ATTACK-MOVE");
    }
    return TEXT("ORDER");
}
