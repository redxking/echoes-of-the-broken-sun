#include "EchoesSimulationSubsystem.h"

#include "EchoesEntityView.h"
#include "EchoesFogView.h"
#include "EchoesOfTheBrokenSun.h"
#include "EchoesPlayerController.h"
#include "EchoesTerrainView.h"
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
using echoes::sim::Terrain;
using echoes::sim::Vec2;

[[nodiscard]] bool IsGlassScarCrossing(int32 TileX)
{
    const bool bWesternCavern = TileX >= 12 && TileX <= 15;
    const bool bFutureWellSpan = TileX >= 29 && TileX <= 35;
    const bool bEasternCavern = TileX >= 48 && TileX <= 51;
    return bWesternCavern || bFutureWellSpan || bEasternCavern;
}

[[nodiscard]] int32 ConfigureGlassScar(echoes::sim::Simulation& Simulation)
{
    int32 BlockedTiles = 0;
    for (int32 TileY = 30; TileY <= 34; ++TileY)
    {
        for (int32 TileX = 8; TileX <= 55; ++TileX)
        {
            if (IsGlassScarCrossing(TileX))
            {
                continue;
            }
            if (Simulation.SetTerrainTile(TileX, TileY, Terrain::Blocked))
            {
                ++BlockedTiles;
            }
        }
    }
    return BlockedTiles;
}
}

void UEchoesSimulationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    FixedTimeAccumulator = 0.0;
    NextPlayerCommandSequence = 1;
    bScenarioReady = false;
    bWarnedAboutTimeClamp = false;
    bLoggedFirstTick = false;
    bSimulationPaused = false;
    bMatchResultReported = false;
    FogView.Reset();
    TerrainView.Reset();
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
    const int32 GlassScarBlockedTiles = ConfigureGlassScar(*Simulation);
    if (GlassScarBlockedTiles != 165)
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_GLASS_SCAR_INIT_FAILED] blocked=%d expected=165"),
            GlassScarBlockedTiles);
        Simulation.Reset();
        return false;
    }
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
    SpawnUnit(LocalPlayerId, Faction::MeridianCompact, EntityType::Barracks, 14, 10);
    SpawnUnit(LocalPlayerId, Faction::MeridianCompact, EntityType::Worker, 8, 13);
    SpawnUnit(LocalPlayerId, Faction::MeridianCompact, EntityType::Worker, 11, 14);
    SpawnUnit(LocalPlayerId, Faction::MeridianCompact, EntityType::Worker, 14, 12);
    SpawnUnit(LocalPlayerId, Faction::MeridianCompact, EntityType::Soldier, 8, 8);
    SpawnUnit(LocalPlayerId, Faction::MeridianCompact, EntityType::Soldier, 12, 7);
    SpawnUnit(LocalPlayerId, Faction::MeridianCompact, EntityType::Soldier, 16, 10);

    // Kharuun Assemblies: deterministic prototype opposition in the northeast.
    SpawnUnit(OpponentPlayerId, Faction::KharuunAssemblies, EntityType::CommandCore, 54, 54);
    SpawnUnit(OpponentPlayerId, Faction::KharuunAssemblies, EntityType::Barracks, 50, 54);
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
    bSimulationPaused = false;
    bMatchResultReported = false;
    if (!SpawnTerrainView() || !SpawnFogView() || !SyncEntityViews(true))
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_SIM_VIEW_INIT_FAILED] Initial visible entity views could not be created."));
        DestroyEntityViews();
        DestroyFogView();
        DestroyTerrainView();
        Simulation.Reset();
        bScenarioReady = false;
        return false;
    }
    bScenarioReady = true;

    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_GLASS_SCAR_READY] blocked=%d crossings=3 centralWell=(32,32)"),
        GlassScarBlockedTiles);

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
    DestroyFogView();
    DestroyTerrainView();
    Simulation.Reset();
    FixedTimeAccumulator = 0.0;
    NextPlayerCommandSequence = 1;
    bScenarioReady = false;
    bWarnedAboutTimeClamp = false;
    bLoggedFirstTick = false;
    bSimulationPaused = false;
    bMatchResultReported = false;
}

bool UEchoesSimulationSubsystem::RestartPrototypeScenario()
{
    StopPrototypeScenario();
    const bool bRestarted = StartPrototypeScenario();
    if (bRestarted)
    {
        UE_LOG(LogEchoes, Display, TEXT("[ECHOES_MATCH_RESTARTED]"));
    }
    else
    {
        UE_LOG(LogEchoes, Error, TEXT("[ECHOES_MATCH_RESTART_FAILED]"));
    }
    return bRestarted;
}

void UEchoesSimulationSubsystem::SetScenarioPaused(bool bPaused)
{
    if (!bScenarioReady || !Simulation.IsValid() ||
        Simulation->Outcome() != echoes::sim::MatchOutcome::Ongoing)
    {
        return;
    }
    bSimulationPaused = bPaused;
    FixedTimeAccumulator = 0.0;
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_MATCH_PAUSE] paused=%s"),
        bSimulationPaused ? TEXT("true") : TEXT("false"));
}

echoes::sim::MatchOutcome UEchoesSimulationSubsystem::GetMatchOutcome() const
{
    return Simulation.IsValid() ? Simulation->Outcome()
                                : echoes::sim::MatchOutcome::Ongoing;
}

void UEchoesSimulationSubsystem::Tick(float DeltaTime)
{
    if (!bScenarioReady || !Simulation.IsValid() || bSimulationPaused ||
        Simulation->Outcome() != echoes::sim::MatchOutcome::Ongoing)
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
        if (!SyncEntityViews(false) || !SyncTerrainView() || !SyncFogView())
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
        else
        {
            const echoes::sim::MatchOutcome Outcome = Simulation->Outcome();
            if (Outcome != echoes::sim::MatchOutcome::Ongoing &&
                !bMatchResultReported)
            {
                bMatchResultReported = true;
                if (AEchoesPlayerController* Controller =
                        Cast<AEchoesPlayerController>(
                            GetWorld()->GetFirstPlayerController()))
                {
                    Controller->NotifyMatchFinished(Outcome);
                }
                UE_LOG(
                    LogEchoes,
                    Display,
                    TEXT("[ECHOES_MATCH_FINISHED] outcome=%u tick=%llu"),
                    static_cast<uint8>(Outcome),
                    static_cast<unsigned long long>(Simulation->CurrentTick()));
            }
            if (!bLoggedFirstTick)
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
    return QueuePlayerCommand(
        CommandType,
        ActorId,
        TargetId,
        WorldToSim(WorldPosition),
        WellChoice,
        echoes::sim::EntityType::Barracks,
        OutFeedback);
}

bool UEchoesSimulationSubsystem::IssueBuildCommand(
    uint32 WorkerId,
    echoes::sim::EntityType BuildingType,
    const FVector& WorldPosition,
    FString& OutFeedback)
{
    return QueuePlayerCommand(
        echoes::sim::CommandType::Build,
        WorkerId,
        0,
        WorldToSim(WorldPosition),
        echoes::sim::FutureWellChoice::Dormant,
        BuildingType,
        OutFeedback);
}

bool UEchoesSimulationSubsystem::IssueProductionCommand(
    uint32 ProducerId,
    echoes::sim::EntityType UnitType,
    FString& OutFeedback)
{
    const echoes::sim::Entity* Producer = FindEntity(ProducerId);
    const echoes::sim::Vec2 Position =
        Producer != nullptr ? Producer->position : echoes::sim::Vec2{};
    return QueuePlayerCommand(
        echoes::sim::CommandType::Produce,
        ProducerId,
        0,
        Position,
        echoes::sim::FutureWellChoice::Dormant,
        UnitType,
        OutFeedback);
}

bool UEchoesSimulationSubsystem::QueuePlayerCommand(
    echoes::sim::CommandType CommandType,
    uint32 ActorId,
    uint32 TargetId,
    const echoes::sim::Vec2& SimPosition,
    echoes::sim::FutureWellChoice WellChoice,
    echoes::sim::EntityType BuildType,
    FString& OutFeedback)
{
    OutFeedback.Reset();
    if (!Simulation.IsValid() || !bScenarioReady)
    {
        OutFeedback = TEXT("[SIM_NOT_READY] The deterministic simulation is unavailable.");
        UE_LOG(LogEchoes, Error, TEXT("[ECHOES_CMD_SIM_NOT_READY] actor=%u"), ActorId);
        return false;
    }
    if (Simulation->Outcome() != echoes::sim::MatchOutcome::Ongoing)
    {
        OutFeedback = TEXT("[MATCH_FINISHED] Press R to restart before issuing orders.");
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

    if (!ValidatePrototypeCommand(
            CommandType,
            *Actor,
            TargetId,
            SimPosition,
            WellChoice,
            BuildType,
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
    Command.buildType = BuildType;

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
    echoes::sim::EntityType BuildType,
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
        case CommandType::AttackMove:
            if (Actor.movementPerTickRaw <= 0 || Actor.attackDamage <= 0)
            {
                OutFeedback = TEXT("[ATTACK_MOVE_REQUIRES_COMBAT_UNIT] Select a mobile combat unit.");
                return false;
            }
            if (!Simulation->IsPositionPassable(Position))
            {
                OutFeedback = TEXT("[INVALID_DESTINATION] The attack-move destination is outside or blocked.");
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
        {
            if (Actor.type != EntityType::Worker)
            {
                OutFeedback = TEXT("[BUILD_REQUIRES_WORKER] Select a worker before placing a structure.");
                return false;
            }
            if (Actor.order.type == echoes::sim::OrderType::Build)
            {
                OutFeedback = TEXT("[WORKER_BUSY] This worker already has a construction order.");
                return false;
            }
            const echoes::sim::PlacementResult Placement =
                Simulation->ValidatePlacement(LocalPlayerId, BuildType, Position);
            if (Placement != echoes::sim::PlacementResult::Valid)
            {
                OutFeedback = FString::Printf(
                    TEXT("[BUILD_PLACEMENT_INVALID] Placement rejected with code %u."),
                    static_cast<uint8>(Placement));
                return false;
            }
            const echoes::sim::PlayerState* Player =
                Simulation->FindPlayer(LocalPlayerId);
            const echoes::sim::ResourcePool Cost =
                Simulation->BuildCost(Actor.faction, BuildType);
            if (Player == nullptr || Player->resources.material < Cost.material ||
                Player->resources.dawnshards < Cost.dawnshards)
            {
                OutFeedback = FString::Printf(
                    TEXT("[INSUFFICIENT_RESOURCES] Requires %d Matter and %d Dawnshards."),
                    Cost.material,
                    Cost.dawnshards);
                return false;
            }
            return true;
        }
        case CommandType::Produce:
        {
            const echoes::sim::ProductionResult Result =
                Simulation->ValidateProduction(
                    LocalPlayerId,
                    Actor.id,
                    BuildType);
            switch (Result)
            {
                case echoes::sim::ProductionResult::Valid:
                    return true;
                case echoes::sim::ProductionResult::InvalidPlayer:
                case echoes::sim::ProductionResult::InvalidProducer:
                    OutFeedback = TEXT("[PRODUCER_INVALID] Select an owned production structure.");
                    break;
                case echoes::sim::ProductionResult::ProducerIncomplete:
                    OutFeedback = TEXT("[PRODUCER_INCOMPLETE] Construction must finish before production.");
                    break;
                case echoes::sim::ProductionResult::ProducerBusy:
                    OutFeedback = TEXT("[PRODUCER_BUSY] This structure already has an active production order.");
                    break;
                case echoes::sim::ProductionResult::UnsupportedUnit:
                    OutFeedback = TEXT("[UNIT_UNSUPPORTED] Command Cores produce workers; Barracks produce soldiers.");
                    break;
                case echoes::sim::ProductionResult::InsufficientResources:
                    OutFeedback = TEXT("[INSUFFICIENT_RESOURCES] The selected unit cannot be funded.");
                    break;
                case echoes::sim::ProductionResult::CapacityReached:
                    OutFeedback = TEXT("[LOGISTICS_CAPACITY] Build a drop-off before adding more units.");
                    break;
                case echoes::sim::ProductionResult::EntityCapacityReached:
                    OutFeedback = TEXT("[ENTITY_CAPACITY] The deterministic entity limit was reached.");
                    break;
            }
            return false;
        }
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

bool UEchoesSimulationSubsystem::SpawnFogView()
{
    if (!Simulation.IsValid() || GetWorld() == nullptr)
    {
        return false;
    }
    DestroyFogView();
    FActorSpawnParameters SpawnParameters;
    SpawnParameters.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AEchoesFogView* NewFogView = GetWorld()->SpawnActor<AEchoesFogView>(
        AEchoesFogView::StaticClass(),
        FVector::ZeroVector,
        FRotator::ZeroRotator,
        SpawnParameters);
    if (NewFogView == nullptr ||
        !NewFogView->InitializeFog(
            *Simulation,
            LocalPlayerId,
            TileWorldSize))
    {
        if (NewFogView != nullptr)
        {
            NewFogView->Destroy();
        }
        UE_LOG(LogEchoes, Error, TEXT("[ECHOES_FOG_INIT_FAILED]"));
        return false;
    }
    FogView = NewFogView;
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_FOG_READY] tiles=%d visible=%d explored=%d unexplored=%d"),
        NewFogView->GetVisibleTileCount() +
            NewFogView->GetExploredTileCount() +
            NewFogView->GetUnexploredTileCount(),
        NewFogView->GetVisibleTileCount(),
        NewFogView->GetExploredTileCount(),
        NewFogView->GetUnexploredTileCount());
    return true;
}

bool UEchoesSimulationSubsystem::SpawnTerrainView()
{
    if (!Simulation.IsValid() || GetWorld() == nullptr)
    {
        return false;
    }
    DestroyTerrainView();
    FActorSpawnParameters SpawnParameters;
    SpawnParameters.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AEchoesTerrainView* NewTerrainView = GetWorld()->SpawnActor<AEchoesTerrainView>(
        AEchoesTerrainView::StaticClass(),
        FVector::ZeroVector,
        FRotator::ZeroRotator,
        SpawnParameters);
    if (NewTerrainView == nullptr ||
        !NewTerrainView->InitializeTerrain(*Simulation, TileWorldSize))
    {
        if (NewTerrainView != nullptr)
        {
            NewTerrainView->Destroy();
        }
        UE_LOG(LogEchoes, Error, TEXT("[ECHOES_TERRAIN_VIEW_INIT_FAILED]"));
        return false;
    }
    TerrainView = NewTerrainView;
    return true;
}

bool UEchoesSimulationSubsystem::SyncTerrainView()
{
    AEchoesTerrainView* View = TerrainView.Get();
    return Simulation.IsValid() && View != nullptr &&
           View->SyncTerrain(*Simulation);
}

void UEchoesSimulationSubsystem::DestroyTerrainView()
{
    if (AEchoesTerrainView* View = TerrainView.Get())
    {
        View->Destroy();
    }
    TerrainView.Reset();
}

bool UEchoesSimulationSubsystem::SyncFogView()
{
    AEchoesFogView* View = FogView.Get();
    return Simulation.IsValid() && View != nullptr &&
           View->SyncVisibility(*Simulation);
}

void UEchoesSimulationSubsystem::DestroyFogView()
{
    if (AEchoesFogView* View = FogView.Get())
    {
        View->Destroy();
    }
    FogView.Reset();
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

AEchoesFogView* UEchoesSimulationSubsystem::GetFogView() const
{
    return FogView.Get();
}

AEchoesTerrainView* UEchoesSimulationSubsystem::GetTerrainView() const
{
    return TerrainView.Get();
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
