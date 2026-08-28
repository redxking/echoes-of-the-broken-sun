#include "EchoesSimulationSubsystem.h"

#include "EchoesEntityView.h"
#include "EchoesOfTheBrokenSun.h"
#include "EchoesPlayerController.h"
#include "Engine/World.h"

namespace
{
constexpr int32 PrototypeMapWidthTiles = 64;
constexpr int32 PrototypeMapHeightTiles = 64;
constexpr uint32 PrototypeTicksPerSecond = 20;
constexpr uint64 PrototypeSeed = 0xE0C0'B5A1ULL;
constexpr int32 MaximumCatchUpTicksPerFrame = 8;

using echoes::sim::EntityId;
using echoes::sim::EntityType;
using echoes::sim::Faction;
using echoes::sim::FutureWellChoice;
using echoes::sim::ResourcePool;
using echoes::sim::Vec2;
}

void UEchoesSimulationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    FixedTimeAccumulator = 0.0;
    NextPlayerCommandSequence = 1;
    bScenarioReady = false;
    bWarnedAboutTimeClamp = false;
    bLoggedFirstTick = false;
}

void UEchoesSimulationSubsystem::Deinitialize()
{
    StopPrototypeScenario();
    Super::Deinitialize();
}

TStatId UEchoesSimulationSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(
        UEchoesSimulationSubsystem,
        STATGROUP_Tickables);
}

bool UEchoesSimulationSubsystem::IsTickable() const
{
    const UWorld* World = GetWorld();
    return bScenarioReady && Simulation.IsValid() && World != nullptr &&
           World->IsGameWorld() && !World->bIsTearingDown;
}

bool UEchoesSimulationSubsystem::StartPrototypeScenario()
{
    if (bScenarioReady && Simulation.IsValid())
    {
        UE_LOG(
            LogEchoes,
            Verbose,
            TEXT("[ECHOES_SIM_ALREADY_READY] Prototype simulation start ignored."));
        return true;
    }

    echoes::sim::SimulationConfig Config;
    Config.mapWidthTiles = PrototypeMapWidthTiles;
    Config.mapHeightTiles = PrototypeMapHeightTiles;
    Config.ticksPerSecond = PrototypeTicksPerSecond;
    Config.randomSeed = PrototypeSeed;

    Simulation = MakeUnique<echoes::sim::Simulation>(Config);
    if (!Simulation->AddPlayer(
            LocalPlayerId,
            Faction::MeridianCompact,
            ResourcePool{500, 30}) ||
        !Simulation->AddPlayer(
            OpponentPlayerId,
            Faction::KharuunAssemblies,
            ResourcePool{500, 30}))
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_SIM_PLAYER_INIT_FAILED] Could not initialize both prototype players."));
        Simulation.Reset();
        return false;
    }

    bool bSpawnSucceeded = true;
    const auto SpawnUnit = [this, &bSpawnSucceeded](
                               uint8 Owner,
                               Faction UnitFaction,
                               EntityType Type,
                               int32 TileX,
                               int32 TileY)
    {
        const EntityId Spawned = Simulation->SpawnEntity(
            Owner,
            UnitFaction,
            Type,
            Vec2::FromTiles(TileX, TileY));
        bSpawnSucceeded &= Spawned != 0;
        return Spawned;
    };

    // Meridian Compact: the player-controlled force in the southwest.
    SpawnUnit(LocalPlayerId, Faction::MeridianCompact, EntityType::CommandCore, 10, 10);
    SpawnUnit(LocalPlayerId, Faction::MeridianCompact, EntityType::Worker, 8, 13);
    SpawnUnit(LocalPlayerId, Faction::MeridianCompact, EntityType::Worker, 11, 14);
    SpawnUnit(LocalPlayerId, Faction::MeridianCompact, EntityType::Worker, 14, 12);
    SpawnUnit(LocalPlayerId, Faction::MeridianCompact, EntityType::Soldier, 8, 8);
    SpawnUnit(LocalPlayerId, Faction::MeridianCompact, EntityType::Soldier, 12, 7);
    SpawnUnit(LocalPlayerId, Faction::MeridianCompact, EntityType::Soldier, 16, 10);

    // Kharuun Assemblies: deterministic prototype opposition in the northeast.
    SpawnUnit(OpponentPlayerId, Faction::KharuunAssemblies, EntityType::CommandCore, 54, 54);
    SpawnUnit(OpponentPlayerId, Faction::KharuunAssemblies, EntityType::Worker, 51, 53);
    SpawnUnit(OpponentPlayerId, Faction::KharuunAssemblies, EntityType::Worker, 54, 50);
    SpawnUnit(OpponentPlayerId, Faction::KharuunAssemblies, EntityType::Worker, 57, 52);
    SpawnUnit(OpponentPlayerId, Faction::KharuunAssemblies, EntityType::Soldier, 50, 57);
    SpawnUnit(OpponentPlayerId, Faction::KharuunAssemblies, EntityType::Soldier, 54, 58);
    SpawnUnit(OpponentPlayerId, Faction::KharuunAssemblies, EntityType::Soldier, 58, 55);

    const TArray<FIntPoint> MatterNodeTiles = {
        {16, 16}, {21, 13}, {25, 28}, {33, 22},
        {31, 43}, {43, 36}, {47, 50}, {52, 45}};
    for (const FIntPoint& Tile : MatterNodeTiles)
    {
        bSpawnSucceeded &=
            Simulation->SpawnResourceNode(Vec2::FromTiles(Tile.X, Tile.Y), 1600) != 0;
    }
    bSpawnSucceeded &=
        Simulation->SpawnFutureWell(Vec2::FromTiles(32, 32)) != 0;

    if (!bSpawnSucceeded)
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_SIM_SCENARIO_FAILED] At least one required prototype entity could not be spawned."));
        Simulation.Reset();
        return false;
    }

    Simulation->CaptureReplayBaseline();
    FixedTimeAccumulator = 0.0;
    NextPlayerCommandSequence = 1;
    bLoggedFirstTick = false;
    if (!SyncEntityViews(true))
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_SIM_VIEW_INIT_FAILED] Initial visible entity views could not be created."));
        DestroyEntityViews();
        Simulation.Reset();
        bScenarioReady = false;
        return false;
    }
    bScenarioReady = true;

    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_SIM_READY] Prototype initialized: %d entities, %d visible views, %u Hz, seed=%llu."),
        static_cast<int32>(Simulation->Entities().size()),
        EntityViews.Num(),
        Simulation->Config().ticksPerSecond,
        static_cast<unsigned long long>(Simulation->Config().randomSeed));
    return true;
}

void UEchoesSimulationSubsystem::StopPrototypeScenario()
{
    DestroyEntityViews();
    Simulation.Reset();
    FixedTimeAccumulator = 0.0;
    NextPlayerCommandSequence = 1;
    bScenarioReady = false;
    bWarnedAboutTimeClamp = false;
    bLoggedFirstTick = false;
}

void UEchoesSimulationSubsystem::Tick(float DeltaTime)
{
    if (!bScenarioReady || !Simulation.IsValid())
    {
        return;
    }

    const double TickInterval =
        1.0 / static_cast<double>(Simulation->Config().ticksPerSecond);
    FixedTimeAccumulator += FMath::Min(static_cast<double>(DeltaTime), 0.25);

    int32 TicksThisFrame = 0;
    while (FixedTimeAccumulator >= TickInterval &&
           TicksThisFrame < MaximumCatchUpTicksPerFrame)
    {
        QueueOpponentCommands();
        Simulation->Step();
        FixedTimeAccumulator -= TickInterval;
        ++TicksThisFrame;
    }

    if (FixedTimeAccumulator >= TickInterval)
    {
        FixedTimeAccumulator = FMath::Fmod(FixedTimeAccumulator, TickInterval);
        if (!bWarnedAboutTimeClamp)
        {
            UE_LOG(
                LogEchoes,
                Warning,
                TEXT("[ECHOES_SIM_TIME_CLAMP] Frame delay exceeded the fixed-step catch-up budget; excess wall time was discarded."));
            bWarnedAboutTimeClamp = true;
        }
    }

    if (TicksThisFrame > 0)
    {
        if (!SyncEntityViews(false))
        {
            UE_LOG(
                LogEchoes,
                Error,
                TEXT("[ECHOES_SIM_VIEW_SYNC_FAILED] A currently visible entity view could not be created; the prototype scenario was stopped."));
            if (AEchoesPlayerController* Controller =
                    Cast<AEchoesPlayerController>(GetWorld()->GetFirstPlayerController()))
            {
                Controller->NotifyRuntimeFailure(TEXT("ECHOES_VIEW_SYNC_FAILED"));
            }
            StopPrototypeScenario();
        }
        else if (!bLoggedFirstTick)
        {
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_SIM_FIRST_TICK] tick=%llu checksum=%llu visibleViews=%d."),
                static_cast<unsigned long long>(Simulation->CurrentTick()),
                static_cast<unsigned long long>(Simulation->StateChecksum()),
                EntityViews.Num());
            bLoggedFirstTick = true;
        }
    }
}

void UEchoesSimulationSubsystem::QueueOpponentCommands()
{
    if (!Simulation.IsValid() ||
        Simulation->CurrentTick() % Simulation->Config().ticksPerSecond != 0)
    {
        return;
    }

    const std::vector<echoes::sim::Command> Commands =
        Simulation->GenerateAiCommands(
            OpponentPlayerId,
            echoes::sim::AiPersonality::Raider);
    for (const echoes::sim::Command& Command : Commands)
    {
        std::string Rejection;
        if (!Simulation->QueueCommand(Command, &Rejection))
        {
            UE_LOG(
                LogEchoes,
                Warning,
                TEXT("[ECHOES_AI_COMMAND_REJECTED] actor=%u reason=%s"),
                Command.actor,
                UTF8_TO_TCHAR(Rejection.c_str()));
        }
    }
}

bool UEchoesSimulationSubsystem::IssueCommand(
    echoes::sim::CommandType CommandType,
    uint32 ActorId,
    uint32 TargetId,
    const FVector& WorldPosition,
    echoes::sim::FutureWellChoice WellChoice,
    FString& OutFeedback)
{
    OutFeedback.Reset();
    if (!Simulation.IsValid() || !bScenarioReady)
    {
        OutFeedback = TEXT("[SIM_NOT_READY] The deterministic simulation is unavailable.");
        UE_LOG(LogEchoes, Error, TEXT("[ECHOES_CMD_SIM_NOT_READY] actor=%u"), ActorId);
        return false;
    }

    const echoes::sim::Entity* Actor = Simulation->FindEntity(ActorId);
    if (Actor == nullptr)
    {
        OutFeedback = TEXT("[ACTOR_MISSING] The selected entity no longer exists.");
        return false;
    }
    if (Actor->owner != LocalPlayerId)
    {
        OutFeedback = TEXT("[ACTOR_NOT_OWNED] Only Meridian units accept player orders.");
        return false;
    }

    const echoes::sim::Vec2 SimPosition = WorldToSim(WorldPosition);
    if (!ValidatePrototypeCommand(
            CommandType,
            *Actor,
            TargetId,
            SimPosition,
            WellChoice,
            OutFeedback))
    {
        UE_LOG(
            LogEchoes,
            Warning,
            TEXT("[ECHOES_CMD_VALIDATION_REJECTED] actor=%u type=%u detail=%s"),
            ActorId,
            static_cast<uint8>(CommandType),
            *OutFeedback);
        return false;
    }

    echoes::sim::Command Command;
    Command.executeTick = Simulation->CurrentTick() + 1;
    Command.player = LocalPlayerId;
    Command.sequence = NextPlayerCommandSequence++;
    Command.type = CommandType;
    Command.actor = ActorId;
    Command.target = TargetId;
    Command.position = SimPosition;
    Command.wellChoice = WellChoice;

    std::string Rejection;
    if (!Simulation->QueueCommand(Command, &Rejection))
    {
        OutFeedback = FString::Printf(
            TEXT("[CORE_REJECTED] %s"),
            UTF8_TO_TCHAR(Rejection.c_str()));
        UE_LOG(
            LogEchoes,
            Warning,
            TEXT("[ECHOES_CMD_CORE_REJECTED] actor=%u sequence=%llu reason=%s"),
            ActorId,
            static_cast<unsigned long long>(Command.sequence),
            UTF8_TO_TCHAR(Rejection.c_str()));
        return false;
    }

    OutFeedback = TEXT("[QUEUED] Order accepted for the next simulation tick.");
    UE_LOG(
        LogEchoes,
        Verbose,
        TEXT("[ECHOES_CMD_QUEUED] tick=%llu actor=%u type=%u target=%u sequence=%llu"),
        static_cast<unsigned long long>(Command.executeTick),
        Command.actor,
        static_cast<uint8>(Command.type),
        Command.target,
        static_cast<unsigned long long>(Command.sequence));
    return true;
}

bool UEchoesSimulationSubsystem::ValidatePrototypeCommand(
    echoes::sim::CommandType CommandType,
    const echoes::sim::Entity& Actor,
    uint32 TargetId,
    const echoes::sim::Vec2& Position,
    echoes::sim::FutureWellChoice WellChoice,
    FString& OutFeedback) const
{
    using echoes::sim::CommandType;
    using echoes::sim::EntityType;
    using echoes::sim::FutureWellChoice;

    const echoes::sim::Entity* Target =
        TargetId != 0 ? Simulation->FindEntity(TargetId) : nullptr;
    switch (CommandType)
    {
        case CommandType::Stop:
            return true;
        case CommandType::Move:
            if (Actor.movementPerTickRaw <= 0)
            {
                OutFeedback = TEXT("[IMMOBILE_ACTOR] This entity cannot move.");
                return false;
            }
            if (!Simulation->IsPositionPassable(Position))
            {
                OutFeedback = TEXT("[INVALID_DESTINATION] The destination is outside or blocked.");
                return false;
            }
            return true;
        case CommandType::Gather:
            if (Actor.type != EntityType::Worker || Target == nullptr ||
                Target->type != EntityType::ResourceNode ||
                Target->resourceRemaining <= 0)
            {
                OutFeedback = TEXT("[GATHER_INVALID] Select a worker and an available Matter node.");
                return false;
            }
            break;
        case CommandType::Deliver:
            if (Actor.type != EntityType::Worker || Actor.cargo <= 0 ||
                Target == nullptr || Target->owner != LocalPlayerId ||
                (Target->type != EntityType::CommandCore &&
                 Target->type != EntityType::Dropoff) ||
                !Target->completed)
            {
                OutFeedback = TEXT("[DELIVER_INVALID] A worker carrying Matter needs a completed drop-off.");
                return false;
            }
            return true;
        case CommandType::Attack:
            if (Actor.attackDamage <= 0 || Target == nullptr ||
                Target->owner == echoes::sim::kNeutralPlayer ||
                Target->owner == LocalPlayerId)
            {
                OutFeedback = TEXT("[ATTACK_INVALID] The actor or target cannot be used for this attack.");
                return false;
            }
            break;
        case CommandType::FutureWell:
            if (Actor.type != EntityType::Worker || Target == nullptr ||
                Target->type != EntityType::FutureWell ||
                Target->wellChoice != FutureWellChoice::Dormant ||
                WellChoice == FutureWellChoice::Dormant)
            {
                OutFeedback = TEXT("[WELL_INVALID] A worker must target a dormant Future Well with a chosen protocol.");
                return false;
            }
            break;
        case CommandType::Build:
            OutFeedback = TEXT("[BUILD_UNAVAILABLE] Building placement is outside this technical-prototype slice.");
            return false;
    }

    if (Target == nullptr || !Simulation->IsEntityVisibleTo(LocalPlayerId, Target->id))
    {
        OutFeedback = TEXT("[TARGET_NOT_VISIBLE] The simulation does not currently expose that target to the player.");
        return false;
    }
    return true;
}

bool UEchoesSimulationSubsystem::SyncEntityViews(bool bTeleportNewViews)
{
    if (!Simulation.IsValid() || GetWorld() == nullptr)
    {
        return false;
    }

    bool bAllVisibleViewsReady = true;
    TSet<uint32> LiveEntityIds;
    LiveEntityIds.Reserve(static_cast<int32>(Simulation->Entities().size()));
    for (const echoes::sim::Entity& Entity : Simulation->Entities())
    {
        if (!Simulation->IsEntityVisibleTo(LocalPlayerId, Entity.id))
        {
            continue;
        }
        LiveEntityIds.Add(Entity.id);
        AEchoesEntityView* View = FindEntityView(Entity.id);
        bool bNewView = false;
        if (View == nullptr)
        {
            FActorSpawnParameters SpawnParameters;
            SpawnParameters.SpawnCollisionHandlingOverride =
                ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            View = GetWorld()->SpawnActor<AEchoesEntityView>(
                AEchoesEntityView::StaticClass(),
                SimToWorld(Entity.position),
                FRotator::ZeroRotator,
                SpawnParameters);
            if (View == nullptr)
            {
                UE_LOG(
                    LogEchoes,
                    Error,
                    TEXT("[ECHOES_VIEW_SPAWN_FAILED] entity=%u"),
                    Entity.id);
                bAllVisibleViewsReady = false;
                continue;
            }
            EntityViews.Add(Entity.id, View);
            bNewView = true;
        }
        View->ApplyAuthoritativeState(Entity, bTeleportNewViews || bNewView);
    }

    TArray<uint32> RemovedIds;
    for (const TPair<uint32, TWeakObjectPtr<AEchoesEntityView>>& Pair : EntityViews)
    {
        if (!LiveEntityIds.Contains(Pair.Key))
        {
            if (AEchoesEntityView* View = Pair.Value.Get())
            {
                View->Destroy();
            }
            RemovedIds.Add(Pair.Key);
        }
    }
    for (const uint32 RemovedId : RemovedIds)
    {
        EntityViews.Remove(RemovedId);
    }
    return bAllVisibleViewsReady;
}

void UEchoesSimulationSubsystem::DestroyEntityViews()
{
    for (const TPair<uint32, TWeakObjectPtr<AEchoesEntityView>>& Pair : EntityViews)
    {
        if (AEchoesEntityView* View = Pair.Value.Get())
        {
            View->Destroy();
        }
    }
    EntityViews.Reset();
}

const echoes::sim::Simulation* UEchoesSimulationSubsystem::GetSimulation() const
{
    return Simulation.Get();
}

const echoes::sim::Entity* UEchoesSimulationSubsystem::FindEntity(uint32 EntityId) const
{
    return Simulation.IsValid() ? Simulation->FindEntity(EntityId) : nullptr;
}

AEchoesEntityView* UEchoesSimulationSubsystem::FindEntityView(uint32 EntityId) const
{
    const TWeakObjectPtr<AEchoesEntityView>* View = EntityViews.Find(EntityId);
    return View != nullptr ? View->Get() : nullptr;
}

FVector UEchoesSimulationSubsystem::SimToWorld(const echoes::sim::Vec2& Position) const
{
    const float MapHalfX = static_cast<float>(GetMapWidthTiles()) * 0.5f;
    const float MapHalfY = static_cast<float>(GetMapHeightTiles()) * 0.5f;
    const float TileX = static_cast<float>(Position.x.Raw()) /
                        static_cast<float>(echoes::sim::kFixedScale);
    const float TileY = static_cast<float>(Position.y.Raw()) /
                        static_cast<float>(echoes::sim::kFixedScale);
    return FVector(
        (TileX - MapHalfX) * TileWorldSize,
        (TileY - MapHalfY) * TileWorldSize,
        0.0f);
}

echoes::sim::Vec2 UEchoesSimulationSubsystem::WorldToSim(const FVector& Position) const
{
    const double MapHalfX = static_cast<double>(GetMapWidthTiles()) * 0.5;
    const double MapHalfY = static_cast<double>(GetMapHeightTiles()) * 0.5;
    const double RawX =
        (static_cast<double>(Position.X) / TileWorldSize + MapHalfX) *
        echoes::sim::kFixedScale;
    const double RawY =
        (static_cast<double>(Position.Y) / TileWorldSize + MapHalfY) *
        echoes::sim::kFixedScale;
    return echoes::sim::Vec2::FromRaw(
        FMath::RoundToInt32(RawX),
        FMath::RoundToInt32(RawY));
}

int32 UEchoesSimulationSubsystem::GetMapWidthTiles() const
{
    return Simulation.IsValid() ? Simulation->Config().mapWidthTiles
                                : PrototypeMapWidthTiles;
}

int32 UEchoesSimulationSubsystem::GetMapHeightTiles() const
{
    return Simulation.IsValid() ? Simulation->Config().mapHeightTiles
                                : PrototypeMapHeightTiles;
}
