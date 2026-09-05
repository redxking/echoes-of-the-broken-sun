// Author and owner: Angelis Pseftis
// Editor-only review commands use the same admission path as player orders.
#if WITH_EDITOR
#include "EchoesSimulationSubsystem.h"
#include "EchoesEntityView.h"
#include "EchoesPlayerController.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"

namespace
{
UEchoesSimulationSubsystem* M01ReviewBridge()
{
    if (!GEngine) return nullptr;
    UWorld* World = nullptr;
    for (const FWorldContext& Context : GEngine->GetWorldContexts())
        if (Context.WorldType == EWorldType::PIE)
        {
            if (World) return nullptr;
            World = Context.World();
        }
    auto* Bridge = World ? World->GetSubsystem<UEchoesSimulationSubsystem>() : nullptr;
    return Bridge && Bridge->IsScenarioReady() &&
        Bridge->GetOperationMode() == EEchoesOperationMode::CampaignPrologue ? Bridge : nullptr;
}

FAutoConsoleCommand M01ReviewState(TEXT("Echoes.EditorM01State"),
    TEXT("Log only local-owned or currently visible M01 entities for repeatable editor inspection."),
    FConsoleCommandDelegate::CreateLambda([]
    {
        auto* Bridge = M01ReviewBridge();
        if (!Bridge || !Bridge->GetSimulation()) return;
        const auto* Sim = Bridge->GetSimulation();
        UE_LOG(LogTemp, Display, TEXT("[ECHOES_M01_REVIEW_STATE] tick=%llu paused=%d class=EDT"),
            static_cast<unsigned long long>(Sim->CurrentTick()), Bridge->IsScenarioPaused());
        for (const auto& Entity : Sim->Entities())
        {
            if (Entity.owner != UEchoesSimulationSubsystem::LocalPlayerId &&
                !Sim->IsEntityVisibleTo(UEchoesSimulationSubsystem::LocalPlayerId, Entity.id)) continue;
            const auto* View = Bridge->FindEntityView(Entity.id);
            if (!View) continue;
            const FVector Location = Bridge->SimToWorld(Entity.position);
            UE_LOG(LogTemp, Display, TEXT("[ECHOES_M01_REVIEW_ENTITY] id=%u owner=%u type=%u name=%s world=%.0f,%.0f hp=%d/%d cargo=%d completed=%d view=%s"),
                Entity.id, Entity.owner, static_cast<uint32>(Entity.type), *View->GetDisplayName(),
                Location.X, Location.Y, Entity.hitPoints, Entity.maxHitPoints, Entity.cargo,
                Entity.completed, *View->GetName());
        }
    }));

FAutoConsoleCommand M01ReviewScreen(TEXT("Echoes.EditorM01Screen"),
    TEXT("Open canonical M01 presentation paths for editor review: title, brief, deploy, pause. Not physical-input evidence."),
    FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
    {
        auto* Bridge = M01ReviewBridge();
        auto* Controller = Bridge ? Cast<AEchoesPlayerController>(Bridge->GetWorld()->GetFirstPlayerController()) : nullptr;
        if (!Controller || Args.Num() != 1) return;
        if (Args[0] == TEXT("title")) Controller->PresentTitleScreen();
        else if (Args[0] == TEXT("brief")) Controller->PresentMissionBriefing();
        else if (Args[0] == TEXT("deploy")) Controller->ConfirmMissionBriefing();
        else if (Args[0] == TEXT("pause")) Controller->TogglePauseMenu();
        else return;
        UE_LOG(LogTemp, Display, TEXT("[ECHOES_M01_REVIEW_SCREEN] requested=%s title=%d briefing=%d paused=%d class=EDT_direct_presentation_path"),
            *Args[0], Controller->IsTitleScreenVisible(), Controller->IsMissionBriefingVisible(), Controller->IsPauseMenuVisible());
    }));

FAutoConsoleCommand M01ReviewFreeze(TEXT("Echoes.EditorM01Freeze"),
    TEXT("Pause/resume the existing M01 simulation for static, already-revealed composition review. <1|0>. No fog reveal or overlay; not motion evidence."),
    FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
    {
        auto* Bridge = M01ReviewBridge();
        if (!Bridge || Args.Num() != 1 || (Args[0] != TEXT("1") && Args[0] != TEXT("0"))) return;
        Bridge->SetScenarioPaused(Args[0] == TEXT("1"));
        UE_LOG(LogTemp, Display, TEXT("[ECHOES_M01_REVIEW_FREEZE] paused=%d class=EDT_static_composition_no_knowledge_change"),
            Bridge->IsScenarioPaused());
    }));

FAutoConsoleCommand M01ReviewOrder(TEXT("Echoes.EditorM01Order"),
    TEXT("Ordinary M01 PIE order: <move|attack_move|gather|deliver|attack|stop|hold|guard|patrol|deploy> <ownedActor> <visibleTargetOr0> <tileX> <tileY>. No state writes or economy grants."),
    FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
    {
        auto* Bridge = M01ReviewBridge();
        uint32 Actor = 0, Target = 0; int32 X = 0, Y = 0;
        if (!Bridge || Args.Num() != 5 || !LexTryParseString(Actor, *Args[1]) ||
            !LexTryParseString(Target, *Args[2]) || !LexTryParseString(X, *Args[3]) ||
            !LexTryParseString(Y, *Args[4]) || X < 0 || Y < 0 ||
            X >= Bridge->GetMapWidthTiles() || Y >= Bridge->GetMapHeightTiles()) return;
        const auto* Entity = Bridge->FindEntity(Actor);
        if (!Entity || Entity->owner != UEchoesSimulationSubsystem::LocalPlayerId ||
            (Target != 0 && (!Bridge->GetSimulation()->IsEntityVisibleTo(
                UEchoesSimulationSubsystem::LocalPlayerId, Target) || !Bridge->FindEntityView(Target)))) return;
        using echoes::sim::CommandType;
        CommandType Type;
        if (Args[0] == TEXT("move")) Type = CommandType::Move;
        else if (Args[0] == TEXT("attack_move")) Type = CommandType::AttackMove;
        else if (Args[0] == TEXT("gather")) Type = CommandType::Gather;
        else if (Args[0] == TEXT("deliver")) Type = CommandType::Deliver;
        else if (Args[0] == TEXT("attack")) Type = CommandType::Attack;
        else if (Args[0] == TEXT("stop")) Type = CommandType::Stop;
        else if (Args[0] == TEXT("hold")) Type = CommandType::Hold;
        else if (Args[0] == TEXT("guard")) Type = CommandType::Guard;
        else if (Args[0] == TEXT("patrol")) Type = CommandType::Patrol;
        else if (Args[0] == TEXT("deploy")) Type = CommandType::ToggleDeploy;
        else return;
        FString Feedback;
        const bool Accepted = Bridge->IssueCommand(Type, Actor, Target,
            Bridge->SimToWorld(echoes::sim::Vec2::FromTiles(X, Y)),
            echoes::sim::FutureWellChoice::Dormant, Feedback);
        UE_LOG(LogTemp, Display, TEXT("[ECHOES_M01_REVIEW_ORDER] action=%s actor=%u target=%u tile=%d,%d accepted=%d detail=%s class=EDT_command_admission"),
            *Args[0], Actor, Target, X, Y, Accepted, *Feedback);
    }));
}
#endif
