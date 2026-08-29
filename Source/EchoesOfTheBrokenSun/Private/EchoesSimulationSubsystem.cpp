#include "EchoesSimulationSubsystem.h"

#include "EchoesContentSubsystem.h"
#include "EchoesEntityView.h"
#include "EchoesFogView.h"
#include "EchoesOfTheBrokenSun.h"
#include "EchoesPlayerController.h"
#include "EchoesTerrainView.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#include <algorithm>

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

[[nodiscard]] const TCHAR* FactionStableName(Faction Value)
{
    return Value == Faction::KharuunAssemblies
               ? TEXT("KharuunAssemblies")
               : TEXT("MeridianCompact");
}

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
    bLoggedStressCombat = false;
    bLoggedAiExpansion = false;
    bLoggedAiRetreat = false;
    bLoggedAiPlayerView = false;
    bLoggedAiAdaptation = false;
    bLoggedAiMineralCover = false;
    bLoggedAiVibrationResponse = false;
    bSimulationPaused = false;
    bMatchResultReported = false;
    LocalFaction = Faction::MeridianCompact;
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
    return StartScenario(false);
}

bool UEchoesSimulationSubsystem::StartStressScenario()
{
    return StartScenario(true);
}

bool UEchoesSimulationSubsystem::StartScenario(bool bUseStressScenario)
{
    if (bScenarioReady && Simulation.IsValid())
    {
        UE_LOG(
            LogEchoes,
            Verbose,
            TEXT("[ECHOES_SIM_ALREADY_READY] Prototype simulation start ignored."));
        return true;
    }

    const UWorld* World = GetWorld();
    const UGameInstance* GameInstance = World != nullptr
                                            ? World->GetGameInstance()
                                            : nullptr;
    const UEchoesContentSubsystem* Content =
        GameInstance != nullptr
            ? GameInstance->GetSubsystem<UEchoesContentSubsystem>()
            : nullptr;
    if (Content == nullptr || !Content->IsReady())
    {
        const FString Reason = Content != nullptr
                                   ? Content->GetFailureReason()
                                   : TEXT("CONTENT_SUBSYSTEM_UNAVAILABLE");
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_SIM_CONTENT_REJECTED] reason=%s"),
            *Reason);
        return false;
    }

    echoes::sim::SimulationConfig Config;
    Config.mapWidthTiles = PrototypeMapWidthTiles;
    Config.mapHeightTiles = PrototypeMapHeightTiles;
    Config.ticksPerSecond = PrototypeTicksPerSecond;
    Config.randomSeed = PrototypeSeed;
    FString RulesError;
    if (!Content->GetCatalog().BuildSimulationRules(
            Config.ticksPerSecond,
            Config.rules,
            RulesError))
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_SIM_CONTENT_REJECTED] reason=%s"),
            *RulesError);
        return false;
    }
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_SIM_RULES_READY] version=%u sha256=%s rosterArchetypes=16 catalogUnits=%d catalogBuildings=%d technologies=4 research=authored futureWell=authored bulwarkDeployment=authored relaySupply=authored waystoneMigration=authored warformAdaptation=authored mineralCover=authored vibrationDetection=authored poweredAegis=authored"),
        Config.rules.version,
        *Content->GetCatalog().Sha256,
        Content->GetCatalog().Units.Num(),
        Content->GetCatalog().Buildings.Num());

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
    const Faction ScenarioLocalFaction =
        bUseStressScenario ? Faction::MeridianCompact : LocalFaction;
    const Faction ScenarioOpponentFaction =
        ScenarioLocalFaction == Faction::MeridianCompact
            ? Faction::KharuunAssemblies
            : Faction::MeridianCompact;
    if (!Simulation->AddPlayer(
            LocalPlayerId,
            ScenarioLocalFaction,
            ResourcePool{500, 30}) ||
        !Simulation->AddPlayer(
            OpponentPlayerId,
            ScenarioOpponentFaction,
            ResourcePool{500, 30}) ||
        (bUseStressScenario &&
         (!Simulation->AddPlayer(
              2,
              Faction::KharuunAssemblies,
              ResourcePool{500, 30}) ||
          !Simulation->AddPlayer(
              3,
              Faction::MeridianCompact,
              ResourcePool{500, 30}))))
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_SIM_PLAYER_INIT_FAILED] Could not initialize the requested scenario players."));
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

    if (bUseStressScenario)
    {
        constexpr int32 GridX[10] = {3, 9, 15, 21, 27, 36, 42, 48, 54, 60};
        constexpr int32 GridY[10] = {3, 8, 13, 18, 23, 28, 36, 43, 50, 57};
        constexpr uint8 Owners[4] = {0, 1, 2, 3};
        constexpr Faction Factions[4] = {
            Faction::MeridianCompact,
            Faction::KharuunAssemblies,
            Faction::KharuunAssemblies,
            Faction::MeridianCompact};
        constexpr int32 OffsetX[4] = {0, 1, 0, 1};
        constexpr int32 OffsetY[4] = {0, 0, 1, 1};
        for (int32 Team = 0; Team < 4; ++Team)
        {
            int32 TeamUnits = 0;
            for (int32 Row = 0; Row < 10; ++Row)
            {
                for (int32 Column = 0; Column < 10; ++Column)
                {
                    const EntityType Type = TeamUnits == 0
                                                ? EntityType::CommandCore
                                                : TeamUnits % 3 == 1
                                                      ? EntityType::Soldier
                                                      : TeamUnits % 3 == 2
                                                            ? EntityType::HeavyUnit
                                                            : EntityType::ScoutUnit;
                    SpawnUnit(
                        Owners[Team],
                        Factions[Team],
                        Type,
                        GridX[Column] + OffsetX[Team],
                        GridY[Row] + OffsetY[Team]);
                    ++TeamUnits;
                }
            }
        }
        bSpawnSucceeded &=
            Simulation->SpawnFutureWell(Vec2::FromTiles(32, 32)) != 0;
    }
    else
    {
        const auto SpawnForce = [&SpawnUnit](
                                    uint8 Owner,
                                    Faction ForceFaction,
                                    bool bSouthwest)
        {
            if (bSouthwest)
            {
                SpawnUnit(Owner, ForceFaction, EntityType::CommandCore, 10, 10);
                SpawnUnit(Owner, ForceFaction, EntityType::Barracks, 14, 10);
                SpawnUnit(Owner, ForceFaction, EntityType::Dropoff, 6, 17);
                SpawnUnit(Owner, ForceFaction, EntityType::Worker, 8, 13);
                SpawnUnit(Owner, ForceFaction, EntityType::Worker, 11, 14);
                SpawnUnit(Owner, ForceFaction, EntityType::Worker, 14, 12);
                SpawnUnit(Owner, ForceFaction, EntityType::Soldier, 8, 8);
                SpawnUnit(Owner, ForceFaction, EntityType::Soldier, 12, 7);
                SpawnUnit(Owner, ForceFaction, EntityType::Soldier, 16, 10);
                SpawnUnit(Owner, ForceFaction, EntityType::HeavyUnit, 7, 6);
                SpawnUnit(Owner, ForceFaction, EntityType::ScoutUnit, 15, 6);
                SpawnUnit(Owner, ForceFaction, EntityType::UtilityStructure, 6, 11);
                return;
            }
            SpawnUnit(Owner, ForceFaction, EntityType::CommandCore, 54, 54);
            SpawnUnit(Owner, ForceFaction, EntityType::Barracks, 50, 54);
            SpawnUnit(Owner, ForceFaction, EntityType::Dropoff, 58, 48);
            SpawnUnit(Owner, ForceFaction, EntityType::Worker, 51, 53);
            SpawnUnit(Owner, ForceFaction, EntityType::Worker, 54, 50);
            SpawnUnit(Owner, ForceFaction, EntityType::Worker, 57, 52);
            SpawnUnit(Owner, ForceFaction, EntityType::Soldier, 50, 57);
            SpawnUnit(Owner, ForceFaction, EntityType::Soldier, 54, 58);
            SpawnUnit(Owner, ForceFaction, EntityType::HeavyUnit, 57, 58);
            SpawnUnit(Owner, ForceFaction, EntityType::ScoutUnit, 49, 58);
            SpawnUnit(Owner, ForceFaction, EntityType::UtilityStructure, 58, 53);
        };
        SpawnForce(LocalPlayerId, ScenarioLocalFaction, true);
        SpawnForce(OpponentPlayerId, ScenarioOpponentFaction, false);

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
    }

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
    int32 StressAttackMoveCommands = 0;
    if (bUseStressScenario)
    {
        constexpr Vec2 TeamDestinations[4] = {
            Vec2::FromTiles(46, 46),
            Vec2::FromTiles(18, 46),
            Vec2::FromTiles(46, 18),
            Vec2::FromTiles(18, 18)};
        uint64 TeamSequences[4] = {1, 1, 1, 1};
        for (const echoes::sim::Entity& Entity : Simulation->Entities())
        {
            if (Entity.owner >= 4 ||
                (Entity.type != EntityType::Soldier &&
                 Entity.type != EntityType::HeavyUnit &&
                 Entity.type != EntityType::ScoutUnit))
            {
                continue;
            }
            echoes::sim::Command Command;
            Command.executeTick = Simulation->CurrentTick() + 1;
            Command.player = Entity.owner;
            Command.sequence = TeamSequences[Entity.owner]++;
            Command.type = echoes::sim::CommandType::AttackMove;
            Command.actor = Entity.id;
            Command.position = TeamDestinations[Entity.owner];
            std::string Rejection;
            if (!Simulation->QueueCommand(Command, &Rejection))
            {
                UE_LOG(
                    LogEchoes,
                    Error,
                    TEXT("[ECHOES_STRESS_ORDER_FAILED] actor=%u owner=%u reason=%s"),
                    Entity.id,
                    Entity.owner,
                    UTF8_TO_TCHAR(Rejection.c_str()));
                Simulation.Reset();
                return false;
            }
            ++StressAttackMoveCommands;
        }
        if (StressAttackMoveCommands != 396)
        {
            UE_LOG(
                LogEchoes,
                Error,
                TEXT("[ECHOES_STRESS_ORDER_COUNT_FAILED] attackMove=%d expected=396"),
                StressAttackMoveCommands);
            Simulation.Reset();
            return false;
        }
    }
    FixedTimeAccumulator = 0.0;
    NextPlayerCommandSequence = 1;
    bLoggedFirstTick = false;
    bLoggedStressCombat = false;
    bLoggedAiExpansion = false;
    bLoggedAiRetreat = false;
    bLoggedAiPlayerView = false;
    bLoggedAiAdaptation = false;
    bLoggedAiMineralCover = false;
    bLoggedAiVibrationResponse = false;
    bSimulationPaused = false;
    bMatchResultReported = false;
    bStressScenario = bUseStressScenario;
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
        bStressScenario = false;
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
    if (!bStressScenario)
    {
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_FACTION_SCENARIO_READY] local=%s opposition=%s selectable=true"),
            FactionStableName(ScenarioLocalFaction),
            FactionStableName(ScenarioOpponentFaction));
        const int32 PoweredAegisCount = static_cast<int32>(std::count_if(
            Simulation->Entities().begin(),
            Simulation->Entities().end(),
            [](const echoes::sim::Entity& Entity)
            {
                return Entity.faction ==
                           echoes::sim::Faction::MeridianCompact &&
                       Entity.type ==
                           echoes::sim::EntityType::UtilityStructure &&
                       Entity.aegisPowered;
            }));
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_POWERED_AEGIS_READY] powered=%d publicState=true networkCounterplay=true"),
            PoweredAegisCount);
    }
    if (bStressScenario)
    {
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_STRESS_ORDERS_READY] attackMove=%d teams=4 executeTick=1"),
            StressAttackMoveCommands);
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_STRESS_READY] units=400 teams=4 entities=%d visibleViews=%d"),
            static_cast<int32>(Simulation->Entities().size()),
            EntityViews.Num());
    }
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
    bLoggedStressCombat = false;
    bLoggedAiExpansion = false;
    bLoggedAiRetreat = false;
    bLoggedAiPlayerView = false;
    bLoggedAiAdaptation = false;
    bLoggedAiMineralCover = false;
    bLoggedAiVibrationResponse = false;
    bSimulationPaused = false;
    bMatchResultReported = false;
    bStressScenario = false;
}

bool UEchoesSimulationSubsystem::RestartPrototypeScenario()
{
    const bool bRestartStressScenario = bStressScenario;
    StopPrototypeScenario();
    const bool bRestarted = StartScenario(bRestartStressScenario);
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

bool UEchoesSimulationSubsystem::SelectLocalFaction(
    echoes::sim::Faction NewFaction,
    FString& OutFeedback)
{
    OutFeedback.Reset();
    if (NewFaction != Faction::MeridianCompact &&
        NewFaction != Faction::KharuunAssemblies)
    {
        OutFeedback = TEXT("[FACTION_INVALID] That force is not playable in Glass Scar.");
        return false;
    }
    if (bStressScenario)
    {
        OutFeedback = TEXT("[FACTION_STRESS_LOCKED] The scale fixture has fixed teams.");
        return false;
    }
    if (NewFaction == LocalFaction)
    {
        OutFeedback = FString::Printf(
            TEXT("FACTION: %s already selected."),
            FactionStableName(LocalFaction));
        return true;
    }

    const Faction PreviousFaction = LocalFaction;
    const bool bHadScenario = bScenarioReady && Simulation.IsValid();
    const bool bWasPaused = bSimulationPaused;
    if (!bHadScenario)
    {
        LocalFaction = NewFaction;
        OutFeedback = FString::Printf(
            TEXT("FACTION SELECTED: %s."),
            FactionStableName(LocalFaction));
        return true;
    }

    StopPrototypeScenario();
    LocalFaction = NewFaction;
    if (StartScenario(false))
    {
        SetScenarioPaused(bWasPaused);
        OutFeedback = FString::Printf(
            TEXT("FACTION SELECTED: %s. Glass Scar reset for deployment."),
            FactionStableName(LocalFaction));
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_FACTION_SELECTED] local=%s scenarioReset=true paused=%s"),
            FactionStableName(LocalFaction),
            bWasPaused ? TEXT("true") : TEXT("false"));
        return true;
    }

    StopPrototypeScenario();
    LocalFaction = PreviousFaction;
    const bool bRollbackSucceeded = StartScenario(false);
    if (bRollbackSucceeded)
    {
        SetScenarioPaused(bWasPaused);
    }
    OutFeedback = bRollbackSucceeded
                      ? TEXT("[FACTION_REBUILD_FAILED] The prior faction was restored.")
                      : TEXT("[FACTION_ROLLBACK_FAILED] The operation could not be restored.");
    return false;
}

FString UEchoesSimulationSubsystem::GetQuickSavePath()
{
    return FPaths::Combine(
        FPaths::ProjectSavedDir(),
        TEXT("SaveGames"),
        TEXT("EchoesQuickSave.bin"));
}

bool UEchoesSimulationSubsystem::QuickSaveScenario(FString& OutFeedback) const
{
    OutFeedback.Reset();
    if (!bScenarioReady || !Simulation.IsValid())
    {
        OutFeedback = TEXT("[SAVE_SIM_NOT_READY] No active scenario can be saved.");
        return false;
    }

    const std::vector<uint8> Snapshot = Simulation->SaveSnapshot();
    if (Snapshot.empty() || Snapshot.size() > MAX_int32)
    {
        OutFeedback = TEXT("[SAVE_SNAPSHOT_INVALID] The deterministic snapshot could not be created.");
        return false;
    }
    TArray<uint8> SnapshotBytes;
    SnapshotBytes.Append(Snapshot.data(), static_cast<int32>(Snapshot.size()));

    const FString SavePath = GetQuickSavePath();
    const FString SaveDirectory = FPaths::GetPath(SavePath);
    const FString TemporaryPath = SavePath + TEXT(".tmp");
    const FString BackupPath = SavePath + TEXT(".bak");
    IFileManager& Files = IFileManager::Get();
    if (!Files.MakeDirectory(*SaveDirectory, true))
    {
        OutFeedback = TEXT("[SAVE_DIRECTORY_FAILED] The save directory could not be created.");
        return false;
    }
    Files.Delete(*TemporaryPath, false, true, true);
    if (!FFileHelper::SaveArrayToFile(SnapshotBytes, *TemporaryPath))
    {
        OutFeedback = TEXT("[SAVE_WRITE_FAILED] The temporary checkpoint could not be written.");
        return false;
    }

    TArray<uint8> VerificationBytes;
    std::string VerificationError;
    const bool bTemporaryValid =
        FFileHelper::LoadFileToArray(VerificationBytes, *TemporaryPath) &&
        echoes::sim::Simulation::LoadSnapshot(
            std::span<const uint8>(
                VerificationBytes.GetData(),
                static_cast<size_t>(VerificationBytes.Num())),
            &VerificationError)
            .has_value();
    if (!bTemporaryValid)
    {
        Files.Delete(*TemporaryPath, false, true, true);
        OutFeedback = FString::Printf(
            TEXT("[SAVE_VALIDATION_FAILED] %s"),
            VerificationError.empty()
                ? TEXT("The temporary checkpoint could not be reopened.")
                : UTF8_TO_TCHAR(VerificationError.c_str()));
        return false;
    }

    const bool bHadPriorSave = Files.FileExists(*SavePath);
    if (bHadPriorSave)
    {
        Files.Delete(*BackupPath, false, true, true);
        if (!Files.Move(*BackupPath, *SavePath, true, true, true, true))
        {
            Files.Delete(*TemporaryPath, false, true, true);
            OutFeedback = TEXT("[SAVE_BACKUP_FAILED] The prior checkpoint could not be retained.");
            return false;
        }
    }
    if (!Files.Move(*SavePath, *TemporaryPath, true, true, true, true))
    {
        if (bHadPriorSave && Files.FileExists(*BackupPath))
        {
            Files.Move(*SavePath, *BackupPath, true, true, true, true);
        }
        Files.Delete(*TemporaryPath, false, true, true);
        OutFeedback = TEXT("[SAVE_COMMIT_FAILED] The validated checkpoint was not committed.");
        return false;
    }

    OutFeedback = FString::Printf(
        TEXT("QUICK SAVE: tick %llu committed."),
        static_cast<unsigned long long>(Simulation->CurrentTick()));
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_QUICK_SAVE] tick=%llu bytes=%d backup=%s"),
        static_cast<unsigned long long>(Simulation->CurrentTick()),
        SnapshotBytes.Num(),
        bHadPriorSave ? TEXT("retained") : TEXT("none"));
    return true;
}

bool UEchoesSimulationSubsystem::QuickLoadScenario(FString& OutFeedback)
{
    OutFeedback.Reset();
    if (!bScenarioReady || !Simulation.IsValid())
    {
        OutFeedback = TEXT("[LOAD_SIM_NOT_READY] Start a scenario before loading.");
        return false;
    }

    const FString SavePath = GetQuickSavePath();
    const FString BackupPath = SavePath + TEXT(".bak");
    TUniquePtr<echoes::sim::Simulation> LoadedSimulation;
    FString SelectedPath;
    FString PrimaryFailure;
    const auto TryLoad = [this, &LoadedSimulation](
                             const FString& CandidatePath,
                             FString& OutFailure)
    {
        TArray<uint8> Bytes;
        if (!FFileHelper::LoadFileToArray(Bytes, *CandidatePath))
        {
            OutFailure = TEXT("file unavailable");
            return false;
        }
        std::string Error;
        std::optional<echoes::sim::Simulation> Candidate =
            echoes::sim::Simulation::LoadSnapshot(
                std::span<const uint8>(
                    Bytes.GetData(),
                    static_cast<size_t>(Bytes.Num())),
                &Error);
        if (!Candidate.has_value())
        {
            OutFailure = UTF8_TO_TCHAR(Error.c_str());
            return false;
        }
        const echoes::sim::SimulationConfig& Config = Candidate->Config();
        if (Config.mapWidthTiles != PrototypeMapWidthTiles ||
            Config.mapHeightTiles != PrototypeMapHeightTiles ||
            Config.ticksPerSecond != PrototypeTicksPerSecond ||
            Config.randomSeed != PrototypeSeed ||
            !Candidate->NextCommandSequence(LocalPlayerId).has_value() ||
            Candidate->FindPlayer(LocalPlayerId) == nullptr ||
            Candidate->FindPlayer(LocalPlayerId)->faction != LocalFaction)
        {
            OutFailure = TEXT("snapshot is not a compatible Glass Scar scenario");
            return false;
        }
        LoadedSimulation =
            MakeUnique<echoes::sim::Simulation>(std::move(*Candidate));
        return true;
    };

    if (TryLoad(SavePath, PrimaryFailure))
    {
        SelectedPath = SavePath;
    }
    else
    {
        FString BackupFailure;
        if (!TryLoad(BackupPath, BackupFailure))
        {
            OutFeedback = FString::Printf(
                TEXT("[LOAD_NO_VALID_CHECKPOINT] primary=%s; backup=%s"),
                *PrimaryFailure,
                *BackupFailure);
            return false;
        }
        SelectedPath = BackupPath;
    }

    TUniquePtr<echoes::sim::Simulation> PreviousSimulation =
        MoveTemp(Simulation);
    DestroyEntityViews();
    DestroyFogView();
    DestroyTerrainView();
    Simulation = MoveTemp(LoadedSimulation);
    bScenarioReady = false;
    const bool bViewsRestored =
        SpawnTerrainView() && SpawnFogView() && SyncEntityViews(true);
    if (!bViewsRestored)
    {
        DestroyEntityViews();
        DestroyFogView();
        DestroyTerrainView();
        Simulation = MoveTemp(PreviousSimulation);
        const bool bRollbackViews =
            SpawnTerrainView() && SpawnFogView() && SyncEntityViews(true);
        bScenarioReady = bRollbackViews;
        OutFeedback = bRollbackViews
                          ? TEXT("[LOAD_VIEW_RESTORE_FAILED] The prior live match was restored.")
                          : TEXT("[LOAD_ROLLBACK_FAILED] Presentation recovery failed.");
        return false;
    }

    FixedTimeAccumulator = 0.0;
    NextPlayerCommandSequence =
        *Simulation->NextCommandSequence(LocalPlayerId);
    bScenarioReady = true;
    bWarnedAboutTimeClamp = false;
    bLoggedFirstTick = true;
    bSimulationPaused = false;
    bMatchResultReported =
        Simulation->Outcome() != echoes::sim::MatchOutcome::Ongoing;
    const bool bUsedBackup = SelectedPath == BackupPath;
    OutFeedback = FString::Printf(
        TEXT("QUICK LOAD: tick %llu restored%s."),
        static_cast<unsigned long long>(Simulation->CurrentTick()),
        bUsedBackup ? TEXT(" from prior-generation backup") : TEXT(""));
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_QUICK_LOAD] tick=%llu source=%s"),
        static_cast<unsigned long long>(Simulation->CurrentTick()),
        bUsedBackup ? TEXT("backup") : TEXT("primary"));
    return true;
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

FEchoesObjectiveSnapshot
UEchoesSimulationSubsystem::GetLocalObjectiveSnapshot() const
{
    FEchoesObjectiveSnapshot Snapshot;
    Snapshot.bScenarioReady = bScenarioReady && Simulation.IsValid();
    if (!Snapshot.bScenarioReady)
    {
        return Snapshot;
    }

    Snapshot.Outcome = Simulation->Outcome();
    for (const echoes::sim::Entity& Entity : Simulation->Entities())
    {
        if (Entity.owner == LocalPlayerId &&
            Entity.type == echoes::sim::EntityType::CommandCore)
        {
            Snapshot.bLocalCoreIntact = true;
            Snapshot.LocalCoreHitPoints = Entity.hitPoints;
            Snapshot.LocalCoreMaxHitPoints = Entity.maxHitPoints;
            continue;
        }

        const bool bVisible =
            Simulation->IsEntityVisibleTo(LocalPlayerId, Entity.id);
        if (!bVisible)
        {
            continue;
        }
        if (Entity.type == echoes::sim::EntityType::FutureWell)
        {
            Snapshot.bFutureWellVisible = true;
            Snapshot.VisibleFutureWellChoice = Entity.wellChoice;
        }
        else if (Entity.owner != echoes::sim::kNeutralPlayer &&
                 Entity.owner != LocalPlayerId &&
                 Entity.type == echoes::sim::EntityType::CommandCore)
        {
            Snapshot.bHostileCoreVisible = true;
        }
    }
    return Snapshot;
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
            if (bStressScenario && !bLoggedStressCombat &&
                Simulation->CurrentTick() >= 20)
            {
                int32 RemainingSoldiers = 0;
                int32 DamagedSoldiers = 0;
                for (const echoes::sim::Entity& Entity : Simulation->Entities())
                {
                    if (Entity.type != EntityType::Soldier &&
                        Entity.type != EntityType::HeavyUnit &&
                        Entity.type != EntityType::ScoutUnit)
                    {
                        continue;
                    }
                    ++RemainingSoldiers;
                    DamagedSoldiers += Entity.hitPoints < Entity.maxHitPoints ? 1 : 0;
                }
                const int32 DestroyedSoldiers = 396 - RemainingSoldiers;
                if (DamagedSoldiers > 0 || DestroyedSoldiers > 0)
                {
                    UE_LOG(
                        LogEchoes,
                        Display,
                        TEXT("[ECHOES_STRESS_COMBAT_ACTIVE] tick=%llu damaged=%d destroyed=%d remaining=%d visibleViews=%d"),
                        static_cast<unsigned long long>(Simulation->CurrentTick()),
                        DamagedSoldiers,
                        DestroyedSoldiers,
                        RemainingSoldiers,
                        EntityViews.Num());
                    bLoggedStressCombat = true;
                }
            }
        }
    }
}

void UEchoesSimulationSubsystem::QueueOpponentCommands()
{
    if (bStressScenario || !Simulation.IsValid() ||
        Simulation->CurrentTick() % Simulation->Config().ticksPerSecond != 0)
    {
        return;
    }

    const std::optional<echoes::sim::PlayerView> PlayerView =
        Simulation->CreatePlayerView(OpponentPlayerId);
    if (!PlayerView.has_value())
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_AI_PLAYER_VIEW_FAILED] player=%u"),
            OpponentPlayerId);
        return;
    }
    if (!bLoggedAiPlayerView)
    {
        int32 OwnedEntities = 0;
        for (const echoes::sim::Entity& Entity : PlayerView->Entities())
        {
            OwnedEntities += Entity.owner == OpponentPlayerId ? 1 : 0;
        }
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_AI_PLAYER_VIEW] player=%u owned=%d observed=%d hiddenEntitiesExcluded=true opponentInternalsRedacted=true authoritativeWorldHandle=false"),
            OpponentPlayerId,
            OwnedEntities,
            static_cast<int32>(PlayerView->Entities().size()));
        bLoggedAiPlayerView = true;
    }
    const std::vector<echoes::sim::Command> Commands =
        echoes::sim::Simulation::GenerateAiCommands(
            *PlayerView,
            echoes::sim::AiPersonality::Adaptive);
    for (const echoes::sim::Command& Command : Commands)
    {
        std::string Rejection;
        if (Simulation->QueueCommand(Command, &Rejection))
        {
            if (!bLoggedAiExpansion &&
                Command.type == echoes::sim::CommandType::Build)
            {
                UE_LOG(
                    LogEchoes,
                    Display,
                    TEXT("[ECHOES_AI_EXPANSION] personality=adaptive actor=%u buildType=%u tile=(%d,%d) visibilityBounded=true"),
                    Command.actor,
                    static_cast<uint8>(Command.buildType),
                    Command.position.x.FloorToInt(),
                    Command.position.y.FloorToInt());
                bLoggedAiExpansion = true;
            }
            if (!bLoggedAiRetreat &&
                (Command.type == echoes::sim::CommandType::Move ||
                 Command.type == echoes::sim::CommandType::Hold))
            {
                const echoes::sim::Entity* Actor =
                    Simulation->FindEntity(Command.actor);
                if (Actor != nullptr &&
                    (Actor->type == echoes::sim::EntityType::Soldier ||
                     Actor->type == echoes::sim::EntityType::HeavyUnit ||
                     Actor->type == echoes::sim::EntityType::ScoutUnit) &&
                    Actor->maxHitPoints > 0 &&
                    static_cast<int64>(Actor->hitPoints) * 100 <=
                        static_cast<int64>(Actor->maxHitPoints) * 35)
                {
                    UE_LOG(
                        LogEchoes,
                        Display,
                        TEXT("[ECHOES_AI_RETREAT] personality=adaptive actor=%u health=%d/%d action=%s visibilityBounded=true"),
                        Command.actor,
                        Actor->hitPoints,
                        Actor->maxHitPoints,
                        Command.type == echoes::sim::CommandType::Hold
                            ? TEXT("hold-near-core")
                            : TEXT("withdraw-to-core"));
                    bLoggedAiRetreat = true;
                }
            }
            if (!bLoggedAiAdaptation &&
                Command.type == echoes::sim::CommandType::AdaptWarform)
            {
                UE_LOG(
                    LogEchoes,
                    Display,
                    TEXT("[ECHOES_AI_ADAPTATION] personality=adaptive actor=%u site=%u form=%u compositionVisible=true"),
                    Command.actor,
                    Command.target,
                    static_cast<uint8>(Command.warformAdaptation));
                bLoggedAiAdaptation = true;
            }
            if (!bLoggedAiMineralCover &&
                Command.type == echoes::sim::CommandType::RaiseMineralCover)
            {
                UE_LOG(
                    LogEchoes,
                    Display,
                    TEXT("[ECHOES_AI_MINERAL_COVER] personality=adaptive actor=%u tile=(%d,%d) visibilityBounded=true"),
                    Command.actor,
                    Command.position.x.FloorToInt(),
                    Command.position.y.FloorToInt());
                bLoggedAiMineralCover = true;
            }
            if (!bLoggedAiVibrationResponse &&
                Command.type == echoes::sim::CommandType::AttackMove &&
                !PlayerView->VibrationSignatures().empty())
            {
                UE_LOG(
                    LogEchoes,
                    Display,
                    TEXT("[ECHOES_AI_VIBRATION_RESPONSE] personality=adaptive actor=%u tile=(%d,%d) anonymous=true visibilityBounded=true"),
                    Command.actor,
                    Command.position.x.FloorToInt(),
                    Command.position.y.FloorToInt());
                bLoggedAiVibrationResponse = true;
            }
        }
        else
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

bool UEchoesSimulationSubsystem::IssueResearchCommand(
    uint32 ProducerId,
    echoes::sim::ResearchType ResearchType,
    FString& OutFeedback)
{
    OutFeedback.Reset();
    if (!Simulation.IsValid() || !bScenarioReady)
    {
        OutFeedback = TEXT("[SIM_NOT_READY] The deterministic simulation is unavailable.");
        return false;
    }
    const echoes::sim::ResearchResult Result = Simulation->ValidateResearch(
        LocalPlayerId, ProducerId, ResearchType);
    if (Result != echoes::sim::ResearchResult::Valid)
    {
        switch (Result)
        {
            case echoes::sim::ResearchResult::InvalidPlayer:
            case echoes::sim::ResearchResult::InvalidProducer:
                OutFeedback = TEXT("[RESEARCH_PRODUCER_INVALID] Select an owned production structure.");
                break;
            case echoes::sim::ResearchResult::ProducerIncomplete:
                OutFeedback = TEXT("[RESEARCH_PRODUCER_INCOMPLETE] Construction must finish before research.");
                break;
            case echoes::sim::ResearchResult::ProducerBusy:
                OutFeedback = TEXT("[RESEARCH_BUSY] Production or another research project is active.");
                break;
            case echoes::sim::ResearchResult::InvalidTechnology:
            case echoes::sim::ResearchResult::WrongFaction:
                OutFeedback = TEXT("[RESEARCH_UNAVAILABLE] This technology is unavailable to the local faction.");
                break;
            case echoes::sim::ResearchResult::AlreadyCompleted:
                OutFeedback = TEXT("[RESEARCH_COMPLETE] This technology is already operational.");
                break;
            case echoes::sim::ResearchResult::PrerequisiteMissing:
                OutFeedback = TEXT("[RESEARCH_PREREQUISITE] Complete the preceding technology first.");
                break;
            case echoes::sim::ResearchResult::InsufficientResources:
                OutFeedback = TEXT("[INSUFFICIENT_RESOURCES] The selected research cannot be funded.");
                break;
            case echoes::sim::ResearchResult::Valid:
                break;
        }
        return false;
    }
    echoes::sim::Command Command{};
    Command.executeTick = Simulation->CurrentTick();
    Command.player = LocalPlayerId;
    Command.sequence = NextPlayerCommandSequence++;
    Command.type = echoes::sim::CommandType::Research;
    Command.actor = ProducerId;
    Command.researchType = ResearchType;
    std::string Rejection;
    if (!Simulation->QueueCommand(Command, &Rejection))
    {
        OutFeedback = FString::Printf(
            TEXT("[RESEARCH_REJECTED] %s"), UTF8_TO_TCHAR(Rejection.c_str()));
        return false;
    }
    OutFeedback = TEXT("RESEARCH QUEUED: production is suspended until completion.");
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_RESEARCH_QUEUED] player=%u producer=%u technology=%u"),
        LocalPlayerId,
        ProducerId,
        static_cast<uint8>(ResearchType));
    return true;
}

bool UEchoesSimulationSubsystem::IssueWarformAdaptation(
    uint32 ActorId,
    uint32 SiteId,
    echoes::sim::WarformAdaptation Adaptation,
    FString& OutFeedback)
{
    OutFeedback.Reset();
    if (!Simulation.IsValid() || !bScenarioReady)
    {
        OutFeedback = TEXT("[SIM_NOT_READY] The deterministic simulation is unavailable.");
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
        OutFeedback = TEXT("[ACTOR_NOT_OWNED] Only the local faction accepts player orders.");
        return false;
    }
    switch (Simulation->ValidateWarformAdaptation(
        LocalPlayerId, ActorId, SiteId, Adaptation))
    {
        case echoes::sim::WarformAdaptationResult::Valid:
            break;
        case echoes::sim::WarformAdaptationResult::InvalidPlayer:
        case echoes::sim::WarformAdaptationResult::InvalidActor:
            OutFeedback = TEXT("[WARFORM_REQUIRED] Select a Kharuun combat warform.");
            return false;
        case echoes::sim::WarformAdaptationResult::InvalidAdaptation:
            OutFeedback = TEXT("[ADAPTATION_INVALID] Choose Carapace or Striker form.");
            return false;
        case echoes::sim::WarformAdaptationResult::AlreadyAdapted:
            OutFeedback = TEXT("[ALREADY_ADAPTED] This warform already has the chosen form.");
            return false;
        case echoes::sim::WarformAdaptationResult::MoltActive:
            OutFeedback = TEXT("[MOLT_ACTIVE] This warform is already molting.");
            return false;
        case echoes::sim::WarformAdaptationResult::InvalidSite:
            OutFeedback = TEXT("[GROWTH_BASIN_REQUIRED] Choose a completed friendly Growth Basin.");
            return false;
        case echoes::sim::WarformAdaptationResult::OutsideSiteRadius:
            OutFeedback = TEXT("[OUTSIDE_MOLT_SITE] Move within the Growth Basin's adaptation field.");
            return false;
        case echoes::sim::WarformAdaptationResult::InsufficientDawn:
            OutFeedback = TEXT("[INSUFFICIENT_DAWN] The adaptation cannot be funded.");
            return false;
    }

    echoes::sim::Command Command;
    Command.executeTick = Simulation->CurrentTick() + 1;
    Command.player = LocalPlayerId;
    Command.sequence = NextPlayerCommandSequence++;
    Command.type = echoes::sim::CommandType::AdaptWarform;
    Command.actor = ActorId;
    Command.target = SiteId;
    Command.position = Actor->position;
    Command.warformAdaptation = Adaptation;
    std::string Rejection;
    if (!Simulation->QueueCommand(Command, &Rejection))
    {
        OutFeedback = FString::Printf(
            TEXT("[CORE_REJECTED] %s"), UTF8_TO_TCHAR(Rejection.c_str()));
        return false;
    }
    OutFeedback = TEXT("[QUEUED] Warform molt accepted for the next simulation tick.");
    return true;
}

bool UEchoesSimulationSubsystem::IssueMineralCover(
    uint32 ActorId,
    const FVector& WorldPosition,
    FString& OutFeedback)
{
    return QueuePlayerCommand(
        echoes::sim::CommandType::RaiseMineralCover,
        ActorId,
        0,
        WorldToSim(WorldPosition),
        echoes::sim::FutureWellChoice::Dormant,
        echoes::sim::EntityType::Barracks,
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
        OutFeedback = TEXT("[ACTOR_NOT_OWNED] Only locally owned units accept player orders.");
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
            Verbose,
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
            if (Actor.waystoneMode != echoes::sim::WaystoneMode::NotWaystone &&
                Actor.waystoneMode != echoes::sim::WaystoneMode::Mobile)
            {
                OutFeedback = TEXT("[WAYSTONE_MUST_BE_MOBILE] Uproot the Waystone before moving it.");
                return false;
            }
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
        case CommandType::Patrol:
            if (Actor.movementPerTickRaw <= 0 || Actor.attackDamage <= 0)
            {
                OutFeedback = TEXT("[PATROL_REQUIRES_COMBAT_UNIT] Select a mobile combat unit.");
                return false;
            }
            if (!Simulation->IsPositionPassable(Position))
            {
                OutFeedback = TEXT("[INVALID_DESTINATION] The patrol endpoint is outside or blocked.");
                return false;
            }
            if (Position == Actor.position)
            {
                OutFeedback = TEXT("[PATROL_ENDPOINT_UNCHANGED] Choose a different patrol endpoint.");
                return false;
            }
            return true;
        case CommandType::ToggleDeploy:
            if (Actor.faction != echoes::sim::Faction::MeridianCompact ||
                Actor.type != EntityType::HeavyUnit)
            {
                OutFeedback = TEXT("[BULWARK_REQUIRED] Deployment requires a Meridian Bulwark Team.");
                return false;
            }
            if (!Actor.deployed && Position == Actor.position)
            {
                OutFeedback = TEXT("[DEPLOY_FACING_REQUIRED] Point away from the Bulwark to set cover facing.");
                return false;
            }
            return true;
        case CommandType::ActivateRelaySupply:
            switch (Simulation->ValidateRelaySupply(LocalPlayerId, Actor.id))
            {
                case echoes::sim::RelaySupplyResult::Valid:
                    return true;
                case echoes::sim::RelaySupplyResult::InvalidPlayer:
                case echoes::sim::RelaySupplyResult::InvalidActor:
                    OutFeedback = TEXT("[RELAY_REQUIRED] Select a Meridian Relay Skiff.");
                    break;
                case echoes::sim::RelaySupplyResult::AlreadyActive:
                    OutFeedback = TEXT("[RELAY_ALREADY_ACTIVE] This Relay is already extending logistics.");
                    break;
                case echoes::sim::RelaySupplyResult::CooldownActive:
                    OutFeedback = TEXT("[RELAY_COOLDOWN] This Relay has not recovered its reserve.");
                    break;
                case echoes::sim::RelaySupplyResult::Disconnected:
                    OutFeedback = TEXT("[RELAY_DISCONNECTED] Move within range of an Anchor or Power Link.");
                    break;
            }
            return false;
        case CommandType::ToggleWaystoneRoot:
            switch (Simulation->ValidateWaystoneRoot(LocalPlayerId, Actor.id))
            {
                case echoes::sim::WaystoneRootResult::Valid:
                    return true;
                case echoes::sim::WaystoneRootResult::InvalidPlayer:
                case echoes::sim::WaystoneRootResult::InvalidActor:
                    OutFeedback = TEXT("[WAYSTONE_REQUIRED] Select a Kharuun Waystone.");
                    break;
                case echoes::sim::WaystoneRootResult::TransitionActive:
                    OutFeedback = TEXT("[WAYSTONE_TRANSITION_ACTIVE] The Waystone is already changing state.");
                    break;
                case echoes::sim::WaystoneRootResult::RootingBlocked:
                    OutFeedback = TEXT("[WAYSTONE_ROOTING_BLOCKED] Move to a clear, passable footprint.");
                    break;
            }
            return false;
        case CommandType::AdaptWarform:
            OutFeedback = TEXT("[ADAPTATION_FORM_REQUIRED] Use a declared warform adaptation command.");
            return false;
        case CommandType::RaiseMineralCover:
            switch (Simulation->ValidateMineralCover(
                LocalPlayerId, Actor.id, Position))
            {
                case echoes::sim::MineralCoverResult::Valid:
                    return true;
                case echoes::sim::MineralCoverResult::InvalidPlayer:
                case echoes::sim::MineralCoverResult::InvalidActor:
                    OutFeedback = TEXT("[CAIRNBACK_REQUIRED] Select a Kharuun Cairnback.");
                    break;
                case echoes::sim::MineralCoverResult::MoltActive:
                    OutFeedback = TEXT("[MOLT_ACTIVE] A molting Cairnback cannot raise cover.");
                    break;
                case echoes::sim::MineralCoverResult::CooldownActive:
                    OutFeedback = TEXT("[MINERAL_COVER_COOLDOWN] This Cairnback has not regrown its mineral reserve.");
                    break;
                case echoes::sim::MineralCoverResult::OutsideCastRange:
                    OutFeedback = TEXT("[MINERAL_COVER_OUT_OF_RANGE] Choose a position closer to the Cairnback.");
                    break;
                case echoes::sim::MineralCoverResult::InvalidPosition:
                    OutFeedback = TEXT("[MINERAL_COVER_TERRAIN_INVALID] Choose an open or scarred battlefield position.");
                    break;
                case echoes::sim::MineralCoverResult::Occupied:
                    OutFeedback = TEXT("[MINERAL_COVER_OCCUPIED] The mineral barrier needs a clear footprint.");
                    break;
                case echoes::sim::MineralCoverResult::InsufficientDawn:
                    OutFeedback = TEXT("[INSUFFICIENT_DAWN] The mineral barrier cannot be funded.");
                    break;
                case echoes::sim::MineralCoverResult::EntityCapacityReached:
                    OutFeedback = TEXT("[ENTITY_CAPACITY_REACHED] No additional battlefield object can be created.");
                    break;
            }
            return false;
        case CommandType::Research:
            OutFeedback = TEXT("[RESEARCH_FORM_REQUIRED] Use a declared research command.");
            return false;
        case CommandType::Hold:
            if (Actor.attackDamage <= 0)
            {
                OutFeedback = TEXT("[HOLD_REQUIRES_DEFENDER] Select an attack-capable unit.");
                return false;
            }
            return true;
        case CommandType::Guard:
            if (Actor.attackDamage <= 0)
            {
                OutFeedback = TEXT("[GUARD_REQUIRES_DEFENDER] Select an attack-capable unit.");
                return false;
            }
            if (Target == nullptr || Target->owner != LocalPlayerId ||
                Target->id == Actor.id)
            {
                OutFeedback = TEXT("[GUARD_TARGET_INVALID] Guard requires a different live owned entity.");
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
