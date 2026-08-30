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
#include "Misc/CommandLine.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"

#include <algorithm>

namespace
{
constexpr int32 PrototypeMapWidthTiles = 64;
constexpr int32 PrototypeMapHeightTiles = 64;
constexpr uint32 PrototypeTicksPerSecond = 20;
constexpr uint64 PrototypeSeed = 0xE0C0'B5A1ULL;
constexpr int32 MaximumCatchUpTicksPerFrame = 8;
constexpr int32 PrologueSiteRadiusTiles = 3;
constexpr int32 SevenAccountsSiteRadiusTiles = 3;
constexpr int32 UnburiedRoadSiteRadiusTiles = 3;
constexpr int32 TermsOfContinuanceSiteRadiusTiles = 3;
constexpr int32 NamesWithoutBirthsSiteRadiusTiles = 3;
constexpr int32 ShapeOfSilenceSiteRadiusTiles = 3;

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

[[nodiscard]] const TCHAR* ResearchStableName(echoes::sim::ResearchType Value)
{
    switch (Value)
    {
        case echoes::sim::ResearchType::MeridianPrismaticTargeting:
            return TEXT("MeridianPrismaticTargeting");
        case echoes::sim::ResearchType::MeridianHorizonLattice:
            return TEXT("MeridianHorizonLattice");
        case echoes::sim::ResearchType::KharuunEchoCartography:
            return TEXT("KharuunEchoCartography");
        case echoes::sim::ResearchType::KharuunAncestralEdge:
            return TEXT("KharuunAncestralEdge");
        default:
            return TEXT("None");
    }
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

[[nodiscard]] int32 ApplySevenAccountsTerrain(
    echoes::sim::Simulation& Simulation,
    FutureWellChoice Branch)
{
    int32 Delta = 0;
    if (Branch == FutureWellChoice::Harvest)
    {
        for (int32 TileY = 30; TileY <= 34; ++TileY)
        {
            for (int32 TileX = 29; TileX <= 35; ++TileX)
            {
                if (Simulation.SetTerrainTile(TileX, TileY, Terrain::Blocked))
                {
                    ++Delta;
                }
            }
        }
    }
    else if (Branch == FutureWellChoice::Reshape)
    {
        constexpr int32 OpenColumns[] = {27, 28, 36, 37};
        for (int32 TileY = 30; TileY <= 34; ++TileY)
        {
            for (const int32 TileX : OpenColumns)
            {
                if (Simulation.SetTerrainTile(TileX, TileY, Terrain::Open))
                {
                    --Delta;
                }
            }
        }
    }
    return Delta;
}

[[nodiscard]] int32 ApplyUnburiedRoadTerrain(
    echoes::sim::Simulation& Simulation,
    FutureWellChoice Branch)
{
    int32 Delta = 0;
    for (int32 TileY = 30; TileY <= 34; ++TileY)
    {
        for (int32 TileX = 8; TileX <= 55; ++TileX)
        {
            const bool bWestern = TileX >= 12 && TileX <= 15;
            const bool bCentral = TileX >= 29 && TileX <= 35;
            const bool bEastern = TileX >= 48 && TileX <= 51;
            const bool bSelected =
                (Branch == FutureWellChoice::Harvest && bWestern) ||
                (Branch == FutureWellChoice::Preserve && bCentral) ||
                (Branch == FutureWellChoice::Reshape && bEastern);
            if ((bWestern || bCentral || bEastern) && !bSelected &&
                Simulation.SetTerrainTile(TileX, TileY, Terrain::Blocked))
            {
                ++Delta;
            }
        }
    }
    return Delta;
}

[[nodiscard]] uint8 WellChoiceMask(FutureWellChoice Choice)
{
    switch (Choice)
    {
        case FutureWellChoice::Harvest: return 1 << 0;
        case FutureWellChoice::Preserve: return 1 << 1;
        case FutureWellChoice::Reshape: return 1 << 2;
        default: return 0;
    }
}

[[nodiscard]] bool IsWithinTiles(
    const Vec2& Position,
    const Vec2& Site,
    int32 RadiusTiles)
{
    const int64 DeltaX = static_cast<int64>(Position.x.Raw()) - Site.x.Raw();
    const int64 DeltaY = static_cast<int64>(Position.y.Raw()) - Site.y.Raw();
    const int64 RadiusRaw = Vec2::FromTiles(RadiusTiles, 0).x.Raw();
    return DeltaX * DeltaX + DeltaY * DeltaY <= RadiusRaw * RadiusRaw;
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
    bResearchPresentationScenario = false;
    bResearchInterruptionPresentationScenario = false;
    bKharuunSystemsPresentationScenario = false;
    bPrologueCompletionPresentationScenario = false;
    bLoggedResearchPresentationActive = false;
    bLoggedResearchPresentationComplete = false;
    bLoggedResearchPresentationInterrupted = false;
    bLoggedKharuunSystemsPresentation = false;
    PrologueCompletionPresentationStage = 0;
    ProloguePresentationWorkerId = 0;
    ProloguePresentationWellId = 0;
    bSimulationPaused = false;
    bMatchResultReported = false;
    LocalFaction = Faction::MeridianCompact;
    SelectedOperation = EEchoesOperationMode::Skirmish;
    ArchiveCarrierId = 0;
    MemoryBearerId = 0;
    MigrationWaystoneId = 0;
    LifeSupportDistrictId = 0;
    TransitDistrictId = 0;
    ArchiveDistrictId = 0;
    MeridianContinuanceRelayId = 0;
    KharuunContinuanceSpineId = 0;
    MeridianContinuanceWitnessId = 0;
    KharuunContinuanceWitnessId = 0;
    TalarId = 0;
    CensusArchiveId = 0;
    FirstCivilianId = 0;
    SecondCivilianId = 0;
    OruunId = 0;
    FirstMemoryWitnessId = 0;
    SecondMemoryWitnessId = 0;
    CampaignProgress = FEchoesCampaignProgress{};
    CampaignBackupProgress = FEchoesCampaignProgress{};
    bCampaignBackupAvailable = false;
    CampaignProgressPath = FEchoesCampaignProgressStore::GetDefaultPath();
#if !UE_BUILD_SHIPPING
    FString CampaignPathOverride;
    if (FParse::Value(
            FCommandLine::Get(),
            TEXT("EchoesCampaignProgressPath="),
            CampaignPathOverride) &&
        !CampaignPathOverride.IsEmpty())
    {
        CampaignProgressPath = FPaths::ConvertRelativePathToFull(
            CampaignPathOverride);
    }
#endif
    FString CampaignFeedback;
    bCampaignProgressAvailable =
        FEchoesCampaignProgressStore::LoadWithBackup(
            CampaignProgressPath,
            CampaignProgress,
            CampaignFeedback);
    if (bCampaignProgressAvailable)
    {
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_CAMPAIGN_LEDGER_LOAD] available=true records=%d detail=%s"),
            CampaignProgress.Decisions.Num(),
            *CampaignFeedback);
    }
    else
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_CAMPAIGN_LEDGER_LOAD] available=false records=%d detail=%s"),
            CampaignProgress.Decisions.Num(),
            *CampaignFeedback);
    }
    RefreshCampaignBackupState();
    ResearchPresentationTechnology = echoes::sim::ResearchType::None;
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

echoes::sim::Vec2 UEchoesSimulationSubsystem::GetArchiveRecoverySite()
{
    return Vec2::FromTiles(22, 18);
}

echoes::sim::Vec2 UEchoesSimulationSubsystem::GetEvacuationSite()
{
    return Vec2::FromTiles(6, 17);
}

FString UEchoesSimulationSubsystem::GetOperationLabel() const
{
    switch (SelectedOperation)
    {
        case EEchoesOperationMode::CampaignPrologue:
            return TEXT("WHAT THE LEDGER KEEPS");
        case EEchoesOperationMode::CampaignSevenAccounts:
            return TEXT("SEVEN ACCOUNTS OF RAIN");
        case EEchoesOperationMode::CampaignCityReserve:
            return TEXT("A CITY ON RESERVE");
        case EEchoesOperationMode::CampaignUnburiedRoad:
            return TEXT("THE UNBURIED ROAD");
        case EEchoesOperationMode::CampaignTermsOfContinuance:
            return TEXT("TERMS OF CONTINUANCE");
        case EEchoesOperationMode::CampaignNamesWithoutBirths:
            return TEXT("NAMES WITHOUT BIRTHS");
        case EEchoesOperationMode::CampaignShapeOfSilence:
            return TEXT("THE SHAPE OF SILENCE");
        default:
            return TEXT("GLASS SCAR");
    }
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

    if (SelectedOperation == EEchoesOperationMode::CampaignSevenAccounts &&
        !IsSevenAccountsUnlocked())
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_SEVEN_ACCOUNTS_LOCKED] reason=WhatTheLedgerKeeps completion required"));
        return false;
    }
    if (SelectedOperation == EEchoesOperationMode::CampaignCityReserve &&
        !IsCityReserveUnlocked())
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_CITY_RESERVE_LOCKED] reason=two consistent prior mission records required"));
        return false;
    }
    if (SelectedOperation == EEchoesOperationMode::CampaignUnburiedRoad &&
        !IsUnburiedRoadUnlocked())
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_UNBURIED_ROAD_LOCKED] reason=three consistent prior mission records required"));
        return false;
    }
    if (SelectedOperation ==
            EEchoesOperationMode::CampaignTermsOfContinuance &&
        !IsTermsOfContinuanceUnlocked())
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_TERMS_OF_CONTINUANCE_LOCKED] reason=four consistent prior mission records required"));
        return false;
    }
    if (SelectedOperation ==
            EEchoesOperationMode::CampaignNamesWithoutBirths &&
        !IsNamesWithoutBirthsUnlocked())
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NAMES_WITHOUT_BIRTHS_LOCKED] reason=five consistent prior mission records required"));
        return false;
    }
    if (SelectedOperation ==
            EEchoesOperationMode::CampaignShapeOfSilence &&
        !IsShapeOfSilenceUnlocked())
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_SHAPE_OF_SILENCE_LOCKED] reason=six consistent prior mission records required"));
        return false;
    }

#if UE_BUILD_SHIPPING
    const bool bUseResearchPresentation = false;
    const bool bUseResearchInterruptionPresentation = false;
    const bool bUseKharuunSystemsPresentation = false;
    const bool bUsePrologueCompletionPresentation = false;
#else
    const bool bUseResearchPresentation =
        !bUseStressScenario &&
        FParse::Param(FCommandLine::Get(), TEXT("EchoesResearchPresentation"));
    const bool bUseResearchInterruptionPresentation =
        !bUseStressScenario &&
        FParse::Param(
            FCommandLine::Get(),
            TEXT("EchoesResearchInterruptionPresentation"));
    const bool bUseKharuunSystemsPresentation =
        !bUseStressScenario &&
        FParse::Param(
            FCommandLine::Get(),
            TEXT("EchoesKharuunSystemsPresentation"));
    const bool bUsePrologueCompletionPresentation =
        !bUseStressScenario &&
        FParse::Param(
            FCommandLine::Get(),
            TEXT("EchoesPrologueCompletionPresentation"));
#endif
    const int32 PresentationModeCount =
        (bUseResearchPresentation ? 1 : 0) +
        (bUseResearchInterruptionPresentation ? 1 : 0) +
        (bUseKharuunSystemsPresentation ? 1 : 0) +
        (bUsePrologueCompletionPresentation ? 1 : 0);
    if (PresentationModeCount > 1)
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_PRESENTATION_MODE_FAILED] reason=conflicting presentation modes"));
        return false;
    }
    const bool bUseAnyResearchPresentation =
        bUseResearchPresentation || bUseResearchInterruptionPresentation;
    const bool bUseAnyControlledPresentation =
        bUseAnyResearchPresentation || bUseKharuunSystemsPresentation ||
        bUsePrologueCompletionPresentation;
    if (bUsePrologueCompletionPresentation &&
        SelectedOperation != EEchoesOperationMode::CampaignPrologue)
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_PROLOGUE_COMPLETION_PRESENTATION_FAILED] reason=campaign prologue operation required"));
        return false;
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
    const int32 BaseGlassScarBlockedTiles = ConfigureGlassScar(*Simulation);
    if (BaseGlassScarBlockedTiles != 165)
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_GLASS_SCAR_INIT_FAILED] blocked=%d expected=165"),
            BaseGlassScarBlockedTiles);
        Simulation.Reset();
        return false;
    }
    const FutureWellChoice SevenAccountsBranch = GetRecordedPrologueChoice();
    const int32 SevenAccountsTerrainDelta =
        SelectedOperation == EEchoesOperationMode::CampaignSevenAccounts
            ? ApplySevenAccountsTerrain(*Simulation, SevenAccountsBranch)
            : 0;
    const int32 UnburiedRoadTerrainDelta =
        (SelectedOperation == EEchoesOperationMode::CampaignUnburiedRoad ||
         SelectedOperation ==
             EEchoesOperationMode::CampaignTermsOfContinuance ||
         SelectedOperation ==
             EEchoesOperationMode::CampaignNamesWithoutBirths ||
         SelectedOperation ==
             EEchoesOperationMode::CampaignShapeOfSilence)
            ? ApplyUnburiedRoadTerrain(*Simulation, SevenAccountsBranch)
            : 0;
    const int32 GlassScarBlockedTiles =
        BaseGlassScarBlockedTiles + SevenAccountsTerrainDelta +
        UnburiedRoadTerrainDelta;
    const Faction ScenarioLocalFaction =
        bUseStressScenario ? Faction::MeridianCompact
        : SelectedOperation == EEchoesOperationMode::CampaignPrologue
            ? Faction::MeridianCompact
        : SelectedOperation == EEchoesOperationMode::CampaignSevenAccounts
            ? Faction::KharuunAssemblies
        : SelectedOperation == EEchoesOperationMode::CampaignCityReserve
            ? Faction::MeridianCompact
        : SelectedOperation == EEchoesOperationMode::CampaignUnburiedRoad
            ? Faction::KharuunAssemblies
        : SelectedOperation ==
                  EEchoesOperationMode::CampaignTermsOfContinuance
            ? Faction::MeridianCompact
        : SelectedOperation ==
                  EEchoesOperationMode::CampaignNamesWithoutBirths
            ? Faction::MeridianCompact
        : SelectedOperation ==
                  EEchoesOperationMode::CampaignShapeOfSilence
            ? Faction::KharuunAssemblies
            : LocalFaction;
    const Faction ScenarioOpponentFaction =
        ScenarioLocalFaction == Faction::MeridianCompact
            ? Faction::KharuunAssemblies
            : Faction::MeridianCompact;
    if (bUseKharuunSystemsPresentation &&
        ScenarioLocalFaction != Faction::KharuunAssemblies)
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_KHARUUN_SYSTEMS_PRESENTATION_FAILED] reason=local faction must be KharuunAssemblies"));
        Simulation.Reset();
        return false;
    }
    if (!Simulation->AddPlayer(
            LocalPlayerId,
            ScenarioLocalFaction,
            bUseAnyControlledPresentation ||
                    SelectedOperation ==
                        EEchoesOperationMode::CampaignCityReserve ||
                    SelectedOperation ==
                        EEchoesOperationMode::CampaignUnburiedRoad ||
                    SelectedOperation ==
                        EEchoesOperationMode::CampaignTermsOfContinuance ||
                    SelectedOperation ==
                        EEchoesOperationMode::CampaignNamesWithoutBirths ||
                    SelectedOperation ==
                        EEchoesOperationMode::CampaignShapeOfSilence
                ? ResourcePool{1000, 500}
                : ResourcePool{500, 30}) ||
        !Simulation->AddPlayer(
            OpponentPlayerId,
            ScenarioOpponentFaction,
            ResourcePool{500, 30}) ||
        ((bUseResearchInterruptionPresentation ||
          bUseKharuunSystemsPresentation) &&
         !Simulation->AddPlayer(
             2,
             ScenarioOpponentFaction,
             ResourcePool{0, 0})) ||
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
    ArchiveCarrierId = 0;
    MemoryBearerId = 0;
    MigrationWaystoneId = 0;
    LifeSupportDistrictId = 0;
    TransitDistrictId = 0;
    ArchiveDistrictId = 0;
    MeridianContinuanceRelayId = 0;
    KharuunContinuanceSpineId = 0;
    MeridianContinuanceWitnessId = 0;
    KharuunContinuanceWitnessId = 0;
    TalarId = 0;
    CensusArchiveId = 0;
    FirstCivilianId = 0;
    SecondCivilianId = 0;
    OruunId = 0;
    FirstMemoryWitnessId = 0;
    SecondMemoryWitnessId = 0;
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
        if (SelectedOperation == EEchoesOperationMode::CampaignPrologue &&
            Owner == LocalPlayerId && Type == EntityType::ScoutUnit)
        {
            ArchiveCarrierId = Spawned;
        }
        if ((SelectedOperation == EEchoesOperationMode::CampaignSevenAccounts ||
             SelectedOperation == EEchoesOperationMode::CampaignUnburiedRoad ||
             SelectedOperation == EEchoesOperationMode::CampaignShapeOfSilence) &&
            Owner == LocalPlayerId)
        {
            if (Type == EntityType::ScoutUnit)
            {
                MemoryBearerId = Spawned;
            }
            else if (Type == EntityType::Dropoff)
            {
                MigrationWaystoneId = Spawned;
            }
        }
        return Spawned;
    };

    EntityId KharuunSystemsMover = 0;
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

        if (SelectedOperation ==
            EEchoesOperationMode::CampaignCityReserve)
        {
            const echoes::sim::Vec2 LifeSupportSite =
                FEchoesCityReserveMissionModel::SiteForDistrict(
                    EEchoesCityDistrict::LifeSupport);
            const echoes::sim::Vec2 TransitSite =
                FEchoesCityReserveMissionModel::SiteForDistrict(
                    EEchoesCityDistrict::Transit);
            const echoes::sim::Vec2 ArchiveSite =
                FEchoesCityReserveMissionModel::SiteForDistrict(
                    EEchoesCityDistrict::Archive);
            LifeSupportDistrictId = SpawnUnit(
                LocalPlayerId,
                Faction::MeridianCompact,
                EntityType::UtilityStructure,
                LifeSupportSite.x.FloorToInt(),
                LifeSupportSite.y.FloorToInt());
            TransitDistrictId = SpawnUnit(
                LocalPlayerId,
                Faction::MeridianCompact,
                EntityType::UtilityStructure,
                TransitSite.x.FloorToInt(),
                TransitSite.y.FloorToInt());
            ArchiveDistrictId = SpawnUnit(
                LocalPlayerId,
                Faction::MeridianCompact,
                EntityType::UtilityStructure,
                ArchiveSite.x.FloorToInt(),
                ArchiveSite.y.FloorToInt());
        }

        if (SelectedOperation ==
            EEchoesOperationMode::CampaignTermsOfContinuance)
        {
            const FEchoesTermsOfContinuancePlan Plan =
                GetTermsOfContinuancePlan();
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_TERMS_OF_CONTINUANCE_SPAWN] branch=%s begin=true"),
                Plan.StableName);
            MeridianContinuanceRelayId = SpawnUnit(
                LocalPlayerId,
                Faction::MeridianCompact,
                EntityType::UtilityStructure,
                Plan.MeridianRelaySite.x.FloorToInt(),
                Plan.MeridianRelaySite.y.FloorToInt());
            KharuunContinuanceSpineId = SpawnUnit(
                LocalPlayerId,
                Faction::MeridianCompact,
                EntityType::UtilityStructure,
                Plan.KharuunSpineSite.x.FloorToInt(),
                Plan.KharuunSpineSite.y.FloorToInt());
            MeridianContinuanceWitnessId = SpawnUnit(
                LocalPlayerId,
                Faction::MeridianCompact,
                EntityType::ScoutUnit,
                20,
                24);
            KharuunContinuanceWitnessId = SpawnUnit(
                LocalPlayerId,
                Faction::MeridianCompact,
                EntityType::ScoutUnit,
                23,
                24);
            const FIntPoint TreatyLinks[] = {
                {18, 10}, {24, 15}, {29, 20}, {29, 36}, {29, 40}};
            for (const FIntPoint& Link : TreatyLinks)
            {
                SpawnUnit(
                    LocalPlayerId,
                    Faction::MeridianCompact,
                    EntityType::Dropoff,
                    Link.X,
                    Link.Y);
            }
            for (int32 Index = 0; Index < 2; ++Index)
            {
                SpawnUnit(
                    OpponentPlayerId,
                    ScenarioOpponentFaction,
                    EntityType::ScoutUnit,
                    48 + Index * 3,
                    56);
            }
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_TERMS_OF_CONTINUANCE_SPAWN] meridianRelay=%u kharuunSpine=%u meridianWitness=%u kharuunWitness=%u success=%s"),
                MeridianContinuanceRelayId,
                KharuunContinuanceSpineId,
                MeridianContinuanceWitnessId,
                KharuunContinuanceWitnessId,
                bSpawnSucceeded ? TEXT("true") : TEXT("false"));
        }

        if (SelectedOperation ==
            EEchoesOperationMode::CampaignNamesWithoutBirths)
        {
            const FEchoesNamesWithoutBirthsPlan Plan =
                GetNamesWithoutBirthsPlan();
            CensusArchiveId = SpawnUnit(
                LocalPlayerId,
                Faction::MeridianCompact,
                EntityType::UtilityStructure,
                Plan.CensusSite.x.FloorToInt(),
                Plan.CensusSite.y.FloorToInt());
            TalarId = SpawnUnit(
                LocalPlayerId,
                Faction::MeridianCompact,
                EntityType::ScoutUnit,
                18,
                20);
            FirstCivilianId = SpawnUnit(
                LocalPlayerId,
                Faction::MeridianCompact,
                EntityType::Worker,
                20,
                24);
            SecondCivilianId = SpawnUnit(
                LocalPlayerId,
                Faction::MeridianCompact,
                EntityType::Worker,
                23,
                24);
            const FIntPoint CommonArchiveLinks[] = {{18, 10}, {24, 15}};
            for (const FIntPoint& Link : CommonArchiveLinks)
            {
                SpawnUnit(
                    LocalPlayerId,
                    Faction::MeridianCompact,
                    EntityType::Dropoff,
                    Link.X,
                    Link.Y);
            }
            if (Plan.PriorChoice == FutureWellChoice::Reshape)
            {
                const FIntPoint EasternArchiveLinks[] = {{31, 17}, {38, 19}};
                for (const FIntPoint& Link : EasternArchiveLinks)
                {
                    SpawnUnit(
                        LocalPlayerId,
                        Faction::MeridianCompact,
                        EntityType::Dropoff,
                        Link.X,
                        Link.Y);
                }
            }
            for (int32 Index = 0; Index < 3; ++Index)
            {
                SpawnUnit(
                    OpponentPlayerId,
                    ScenarioOpponentFaction,
                    EntityType::ScoutUnit,
                    46 + Index * 3,
                    54);
            }
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_NAMES_WITHOUT_BIRTHS_SPAWN] branch=%s talar=%u archive=%u civilianA=%u civilianB=%u requiredLink=%d,%d success=%s"),
                Plan.StableName,
                TalarId,
                CensusArchiveId,
                FirstCivilianId,
                SecondCivilianId,
                Plan.PowerLinkSite.x.FloorToInt(),
                Plan.PowerLinkSite.y.FloorToInt(),
                bSpawnSucceeded ? TEXT("true") : TEXT("false"));
        }

        if (SelectedOperation ==
            EEchoesOperationMode::CampaignShapeOfSilence)
        {
            const FEchoesShapeOfSilencePlan Plan =
                GetShapeOfSilencePlan();
            OruunId = MemoryBearerId;
            FirstMemoryWitnessId = SpawnUnit(
                LocalPlayerId,
                Faction::KharuunAssemblies,
                EntityType::ScoutUnit,
                20,
                24);
            SecondMemoryWitnessId = SpawnUnit(
                LocalPlayerId,
                Faction::KharuunAssemblies,
                EntityType::ScoutUnit,
                23,
                24);
            MemoryBearerId = OruunId;
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_SHAPE_OF_SILENCE_SPAWN] branch=%s oruun=%u witnessA=%u witnessB=%u waystone=%u success=%s"),
                Plan.StableName,
                OruunId,
                FirstMemoryWitnessId,
                SecondMemoryWitnessId,
                MigrationWaystoneId,
                bSpawnSucceeded ? TEXT("true") : TEXT("false"));
        }

        if (bUseResearchInterruptionPresentation)
        {
            constexpr int32 InterruptionAttackerCount = 32;
            for (int32 Index = 0; Index < InterruptionAttackerCount; ++Index)
            {
                SpawnUnit(
                    2,
                    ScenarioOpponentFaction,
                    EntityType::Soldier,
                    17,
                    10);
            }
        }
        if (bUseKharuunSystemsPresentation)
        {
            KharuunSystemsMover = SpawnUnit(
                2,
                Faction::MeridianCompact,
                EntityType::Soldier,
                31,
                0);
        }

        const TArray<FIntPoint> MatterNodeTiles = {
            SelectedOperation == EEchoesOperationMode::CampaignCityReserve
                ? FIntPoint{17, 27}
                : FIntPoint{16, 16},
            {21, 13}, {25, 28}, {33, 22},
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
    ProloguePresentationWorkerId = 0;
    ProloguePresentationWellId = 0;
    if (bUsePrologueCompletionPresentation)
    {
        for (const echoes::sim::Entity& Entity : Simulation->Entities())
        {
            if (Entity.owner == LocalPlayerId &&
                Entity.type == EntityType::Worker &&
                ProloguePresentationWorkerId == 0)
            {
                ProloguePresentationWorkerId = Entity.id;
            }
            if (Entity.type == EntityType::FutureWell)
            {
                ProloguePresentationWellId = Entity.id;
            }
        }
        if (ArchiveCarrierId == 0 || ProloguePresentationWorkerId == 0 ||
            ProloguePresentationWellId == 0)
        {
            UE_LOG(
                LogEchoes,
                Error,
                TEXT("[ECHOES_PROLOGUE_COMPLETION_PRESENTATION_FAILED] reason=fixture entities unavailable carrier=%u worker=%u well=%u"),
                ArchiveCarrierId,
                ProloguePresentationWorkerId,
                ProloguePresentationWellId);
            Simulation.Reset();
            return false;
        }
    }
    ResearchPresentationTechnology = echoes::sim::ResearchType::None;
    if (bUseAnyResearchPresentation)
    {
        uint32 ProducerId = 0;
        for (const echoes::sim::Entity& Entity : Simulation->Entities())
        {
            if (Entity.owner == LocalPlayerId &&
                Entity.type == EntityType::Barracks)
            {
                ProducerId = Entity.id;
                break;
            }
        }
        if (ProducerId == 0)
        {
            UE_LOG(
                LogEchoes,
                Error,
                TEXT("[ECHOES_RESEARCH_PRESENTATION_FAILED] producer=0 reason=local production structure unavailable"));
            Simulation.Reset();
            return false;
        }
        ResearchPresentationTechnology =
            ScenarioLocalFaction == Faction::MeridianCompact
                ? echoes::sim::ResearchType::MeridianPrismaticTargeting
                : echoes::sim::ResearchType::KharuunEchoCartography;
        echoes::sim::Command Command{};
        Command.executeTick = Simulation->CurrentTick() + 1;
        Command.player = LocalPlayerId;
        Command.sequence = 1;
        Command.type = echoes::sim::CommandType::Research;
        Command.actor = ProducerId;
        Command.researchType = ResearchPresentationTechnology;
        std::string Rejection;
        if (!Simulation->QueueCommand(Command, &Rejection))
        {
            UE_LOG(
                LogEchoes,
                Error,
                TEXT("[ECHOES_RESEARCH_PRESENTATION_FAILED] producer=%u reason=%s"),
                ProducerId,
                UTF8_TO_TCHAR(Rejection.c_str()));
            Simulation.Reset();
            ResearchPresentationTechnology =
                echoes::sim::ResearchType::None;
            return false;
        }
        if (bUseResearchInterruptionPresentation)
        {
            uint64 InterruptionSequence = 1;
            int32 QueuedAttackers = 0;
            for (const echoes::sim::Entity& Entity : Simulation->Entities())
            {
                if (Entity.owner != 2 || Entity.type != EntityType::Soldier)
                {
                    continue;
                }
                echoes::sim::Command Attack{};
                Attack.executeTick = 60;
                Attack.player = 2;
                Attack.sequence = InterruptionSequence++;
                Attack.type = echoes::sim::CommandType::Attack;
                Attack.actor = Entity.id;
                Attack.target = ProducerId;
                if (!Simulation->QueueCommand(Attack, &Rejection))
                {
                    UE_LOG(
                        LogEchoes,
                        Error,
                        TEXT("[ECHOES_RESEARCH_PRESENTATION_FAILED] attacker=%u reason=%s"),
                        Entity.id,
                        UTF8_TO_TCHAR(Rejection.c_str()));
                    Simulation.Reset();
                    ResearchPresentationTechnology =
                        echoes::sim::ResearchType::None;
                    return false;
                }
                ++QueuedAttackers;
            }
            if (QueuedAttackers != 32)
            {
                UE_LOG(
                    LogEchoes,
                    Error,
                    TEXT("[ECHOES_RESEARCH_PRESENTATION_FAILED] interruptionAttackers=%d expected=32"),
                    QueuedAttackers);
                Simulation.Reset();
                ResearchPresentationTechnology =
                    echoes::sim::ResearchType::None;
                return false;
            }
        }
    }
    if (bUseKharuunSystemsPresentation)
    {
        uint32 WaystoneId = 0;
        uint32 BasinId = 0;
        uint32 WarformId = 0;
        uint32 CairnbackId = 0;
        for (const echoes::sim::Entity& Entity : Simulation->Entities())
        {
            if (Entity.owner != LocalPlayerId)
            {
                continue;
            }
            WaystoneId = Entity.type == EntityType::Dropoff
                             ? Entity.id
                             : WaystoneId;
            BasinId = Entity.type == EntityType::Barracks
                          ? Entity.id
                          : BasinId;
            CairnbackId = Entity.type == EntityType::HeavyUnit
                              ? Entity.id
                              : CairnbackId;
        }
        const echoes::sim::Entity* Basin = Simulation->FindEntity(BasinId);
        uint64 NearestWarformDistance = TNumericLimits<uint64>::Max();
        if (Basin != nullptr)
        {
            for (const echoes::sim::Entity& Entity : Simulation->Entities())
            {
                if (Entity.owner != LocalPlayerId ||
                    Entity.type != EntityType::Soldier)
                {
                    continue;
                }
                const int64 DeltaX =
                    static_cast<int64>(Entity.position.x.Raw()) -
                    Basin->position.x.Raw();
                const int64 DeltaY =
                    static_cast<int64>(Entity.position.y.Raw()) -
                    Basin->position.y.Raw();
                const uint64 Distance = static_cast<uint64>(
                    DeltaX * DeltaX + DeltaY * DeltaY);
                if (Distance < NearestWarformDistance)
                {
                    NearestWarformDistance = Distance;
                    WarformId = Entity.id;
                }
            }
        }
        if (WaystoneId == 0 || BasinId == 0 || WarformId == 0 ||
            CairnbackId == 0 || KharuunSystemsMover == 0)
        {
            UE_LOG(
                LogEchoes,
                Error,
                TEXT("[ECHOES_KHARUUN_SYSTEMS_PRESENTATION_FAILED] reason=fixture entities unavailable"));
            Simulation.Reset();
            return false;
        }
        const auto QueueFixtureCommand = [this](
                                             echoes::sim::Command Command,
                                             const TCHAR* Label)
        {
            std::string Rejection;
            if (Simulation->QueueCommand(Command, &Rejection))
            {
                return true;
            }
            UE_LOG(
                LogEchoes,
                Error,
                TEXT("[ECHOES_KHARUUN_SYSTEMS_PRESENTATION_FAILED] command=%s reason=%s"),
                Label,
                UTF8_TO_TCHAR(Rejection.c_str()));
            return false;
        };

        echoes::sim::Command Waystone{};
        Waystone.executeTick = 1;
        Waystone.player = LocalPlayerId;
        Waystone.sequence = 1;
        Waystone.type = echoes::sim::CommandType::ToggleWaystoneRoot;
        Waystone.actor = WaystoneId;

        echoes::sim::Command Adapt{};
        Adapt.executeTick = 1;
        Adapt.player = LocalPlayerId;
        Adapt.sequence = 2;
        Adapt.type = echoes::sim::CommandType::AdaptWarform;
        Adapt.actor = WarformId;
        Adapt.target = BasinId;
        Adapt.warformAdaptation = echoes::sim::WarformAdaptation::Carapace;

        echoes::sim::Command Cover{};
        Cover.executeTick = 1;
        Cover.player = LocalPlayerId;
        Cover.sequence = 3;
        Cover.type = echoes::sim::CommandType::RaiseMineralCover;
        Cover.actor = CairnbackId;
        Cover.position = Vec2::FromTiles(7, 4);

        echoes::sim::Command Move{};
        Move.executeTick = 1;
        Move.player = 2;
        Move.sequence = 1;
        Move.type = echoes::sim::CommandType::Move;
        Move.actor = KharuunSystemsMover;
        Move.position = Vec2::FromTiles(30, 0);

        if (!QueueFixtureCommand(Waystone, TEXT("waystone")) ||
            !QueueFixtureCommand(Adapt, TEXT("carapace")) ||
            !QueueFixtureCommand(Cover, TEXT("mineral_cover")) ||
            !QueueFixtureCommand(Move, TEXT("hidden_movement")))
        {
            Simulation.Reset();
            return false;
        }
    }
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
    NextPlayerCommandSequence = bUseKharuunSystemsPresentation
                                    ? 4
                                    : bUseAnyResearchPresentation ? 2 : 1;
    bLoggedFirstTick = false;
    bLoggedStressCombat = false;
    bLoggedAiExpansion = false;
    bLoggedAiRetreat = false;
    bLoggedAiPlayerView = false;
    bLoggedAiAdaptation = false;
    bLoggedAiMineralCover = false;
    bLoggedAiVibrationResponse = false;
    bResearchPresentationScenario = bUseResearchPresentation;
    bResearchInterruptionPresentationScenario =
        bUseResearchInterruptionPresentation;
    bKharuunSystemsPresentationScenario =
        bUseKharuunSystemsPresentation;
    bPrologueCompletionPresentationScenario =
        bUsePrologueCompletionPresentation;
    bLoggedResearchPresentationActive = false;
    bLoggedResearchPresentationComplete = false;
    bLoggedResearchPresentationInterrupted = false;
    bLoggedKharuunSystemsPresentation = false;
    PrologueCompletionPresentationStage = 0;
    bSimulationPaused = false;
    bMatchResultReported = false;
    bStressScenario = bUseStressScenario;
    if (SelectedOperation == EEchoesOperationMode::CampaignPrologue &&
        ArchiveCarrierId == 0)
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_PROLOGUE_INIT_FAILED] reason=archive carrier unavailable"));
        Simulation.Reset();
        return false;
    }
    if (SelectedOperation == EEchoesOperationMode::CampaignSevenAccounts &&
        (MemoryBearerId == 0 || MigrationWaystoneId == 0))
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_SEVEN_ACCOUNTS_INIT_FAILED] reason=mission entities unavailable bearer=%u waystone=%u"),
            MemoryBearerId,
            MigrationWaystoneId);
        Simulation.Reset();
        return false;
    }
    if (SelectedOperation == EEchoesOperationMode::CampaignUnburiedRoad &&
        (MemoryBearerId == 0 || MigrationWaystoneId == 0))
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_UNBURIED_ROAD_INIT_FAILED] reason=mission entities unavailable bearer=%u waystone=%u"),
            MemoryBearerId,
            MigrationWaystoneId);
        Simulation.Reset();
        return false;
    }
    if (SelectedOperation == EEchoesOperationMode::CampaignCityReserve &&
        (LifeSupportDistrictId == 0 || TransitDistrictId == 0 ||
         ArchiveDistrictId == 0))
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_CITY_RESERVE_INIT_FAILED] reason=district entities unavailable life=%u transit=%u archive=%u"),
            LifeSupportDistrictId,
            TransitDistrictId,
            ArchiveDistrictId);
        Simulation.Reset();
        return false;
    }
    if (SelectedOperation ==
            EEchoesOperationMode::CampaignTermsOfContinuance &&
        (MeridianContinuanceRelayId == 0 ||
         KharuunContinuanceSpineId == 0 ||
         MeridianContinuanceWitnessId == 0 ||
         KharuunContinuanceWitnessId == 0))
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_TERMS_OF_CONTINUANCE_INIT_FAILED] reason=mission entities unavailable meridianRelay=%u kharuunSpine=%u meridianWitness=%u kharuunWitness=%u"),
            MeridianContinuanceRelayId,
            KharuunContinuanceSpineId,
            MeridianContinuanceWitnessId,
            KharuunContinuanceWitnessId);
        Simulation.Reset();
        return false;
    }
    if (SelectedOperation ==
            EEchoesOperationMode::CampaignNamesWithoutBirths &&
        (TalarId == 0 || CensusArchiveId == 0 || FirstCivilianId == 0 ||
         SecondCivilianId == 0))
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NAMES_WITHOUT_BIRTHS_INIT_FAILED] reason=mission entities unavailable talar=%u archive=%u civilianA=%u civilianB=%u"),
            TalarId,
            CensusArchiveId,
            FirstCivilianId,
            SecondCivilianId);
        Simulation.Reset();
        return false;
    }
    if (SelectedOperation ==
            EEchoesOperationMode::CampaignShapeOfSilence &&
        (OruunId == 0 || FirstMemoryWitnessId == 0 ||
         SecondMemoryWitnessId == 0 || MigrationWaystoneId == 0))
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_SHAPE_OF_SILENCE_INIT_FAILED] reason=mission entities unavailable oruun=%u witnessA=%u witnessB=%u waystone=%u"),
            OruunId,
            FirstMemoryWitnessId,
            SecondMemoryWitnessId,
            MigrationWaystoneId);
        Simulation.Reset();
        return false;
    }
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
        if (SelectedOperation == EEchoesOperationMode::CampaignPrologue)
        {
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_PROLOGUE_READY] mission=WhatTheLedgerKeeps carrier=%u archive=(22,18) evacuation=(6,17) faction=MeridianCompact completion=withdrawal"),
                ArchiveCarrierId);
        }
        else if (SelectedOperation == EEchoesOperationMode::CampaignSevenAccounts)
        {
            const FEchoesSevenAccountsRoute Route = GetSevenAccountsRoute();
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_SEVEN_ACCOUNTS_READY] branch=%s waystone=%u bearer=%u anchor=(%d,%d) account=(%d,%d) terrainDelta=%d blocked=%d"),
                Route.StableName,
                MigrationWaystoneId,
                MemoryBearerId,
                Route.WaystoneAnchor.x.FloorToInt(),
                Route.WaystoneAnchor.y.FloorToInt(),
                Route.MemoryAccountSite.x.FloorToInt(),
                Route.MemoryAccountSite.y.FloorToInt(),
                SevenAccountsTerrainDelta,
                GlassScarBlockedTiles);
        }
        else if (SelectedOperation ==
                 EEchoesOperationMode::CampaignCityReserve)
        {
            const FEchoesCityReserveGrid Grid = GetCityReserveGrid();
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_CITY_RESERVE_READY] branch=%s priority=%s secondary=%s final=%s life=%u transit=%u archive=%u powered=0 inheritedRecords=2"),
                Grid.StableName,
                FEchoesCityReserveMissionModel::DistrictDisplayName(
                    Grid.Priority),
                FEchoesCityReserveMissionModel::DistrictDisplayName(
                    Grid.Secondary),
                FEchoesCityReserveMissionModel::DistrictDisplayName(
                    Grid.Final),
                LifeSupportDistrictId,
                TransitDistrictId,
                ArchiveDistrictId);
        }
        else if (SelectedOperation ==
                 EEchoesOperationMode::CampaignUnburiedRoad)
        {
            const FEchoesUnburiedRoadRoute Route = GetUnburiedRoadRoute();
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_UNBURIED_ROAD_READY] branch=%s waystone=%u bearer=%u roadhead=(%d,%d) listeningSpine=(%d,%d) shard=(%d,%d) terrainDelta=%d blocked=%d inheritedRecords=3"),
                Route.StableName,
                MigrationWaystoneId,
                MemoryBearerId,
                Route.Roadhead.x.FloorToInt(),
                Route.Roadhead.y.FloorToInt(),
                Route.ListeningSpineSite.x.FloorToInt(),
                Route.ListeningSpineSite.y.FloorToInt(),
                Route.MemoryShardSite.x.FloorToInt(),
                Route.MemoryShardSite.y.FloorToInt(),
                UnburiedRoadTerrainDelta,
                GlassScarBlockedTiles);
        }
        else if (SelectedOperation ==
                 EEchoesOperationMode::CampaignTermsOfContinuance)
        {
            const FEchoesTermsOfContinuancePlan Plan =
                GetTermsOfContinuancePlan();
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_TERMS_OF_CONTINUANCE_READY] branch=%s meridianRelay=%u kharuunSpine=%u meridianWitness=%u kharuunWitness=%u relay=(%d,%d) spine=(%d,%d) extraction=(%d,%d) window=(%llu,%llu) pressureProxies=2 proxyAuthority=MeridianCompact pressureFaction=KharuunAssemblies pressureBehavior=genericAdaptive terrainDelta=%d blocked=%d inheritedRecords=4"),
                Plan.StableName,
                MeridianContinuanceRelayId,
                KharuunContinuanceSpineId,
                MeridianContinuanceWitnessId,
                KharuunContinuanceWitnessId,
                Plan.MeridianRelaySite.x.FloorToInt(),
                Plan.MeridianRelaySite.y.FloorToInt(),
                Plan.KharuunSpineSite.x.FloorToInt(),
                Plan.KharuunSpineSite.y.FloorToInt(),
                Plan.WitnessExtractionSite.x.FloorToInt(),
                Plan.WitnessExtractionSite.y.FloorToInt(),
                static_cast<unsigned long long>(
                    Plan.ContinuanceWindowStartTick),
                static_cast<unsigned long long>(
                    Plan.ContinuanceWindowEndTick),
                UnburiedRoadTerrainDelta,
                GlassScarBlockedTiles);
        }
        else if (SelectedOperation ==
                 EEchoesOperationMode::CampaignNamesWithoutBirths)
        {
            const FEchoesNamesWithoutBirthsPlan Plan =
                GetNamesWithoutBirthsPlan();
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_NAMES_WITHOUT_BIRTHS_READY] branch=%s talar=%u archive=%u civilianA=%u civilianB=%u census=(%d,%d) shelter=(%d,%d) extraction=(%d,%d) pressureProxies=3 pressureFaction=KharuunAssemblies pressureBehavior=genericAdaptive inheritedRecords=5"),
                Plan.StableName,
                TalarId,
                CensusArchiveId,
                FirstCivilianId,
                SecondCivilianId,
                Plan.CensusSite.x.FloorToInt(),
                Plan.CensusSite.y.FloorToInt(),
                Plan.CivilianShelterSite.x.FloorToInt(),
                Plan.CivilianShelterSite.y.FloorToInt(),
                Plan.EvidenceExtractionSite.x.FloorToInt(),
                Plan.EvidenceExtractionSite.y.FloorToInt());
        }
        else if (SelectedOperation ==
                 EEchoesOperationMode::CampaignShapeOfSilence)
        {
            const FEchoesShapeOfSilencePlan Plan =
                GetShapeOfSilencePlan();
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_SHAPE_OF_SILENCE_READY] branch=%s oruun=%u witnessA=%u witnessB=%u waystone=%u anchor=(%d,%d) spine=(%d,%d) witnessSites=(%d,%d):(%d,%d) confluence=(%d,%d) observedCorrespondenceOnly=true hiddenAttribution=false inheritedRecords=6 terrainDelta=%d blocked=%d"),
                Plan.StableName,
                OruunId,
                FirstMemoryWitnessId,
                SecondMemoryWitnessId,
                MigrationWaystoneId,
                Plan.WaystoneAnchor.x.FloorToInt(),
                Plan.WaystoneAnchor.y.FloorToInt(),
                Plan.ListeningSpineSite.x.FloorToInt(),
                Plan.ListeningSpineSite.y.FloorToInt(),
                Plan.FirstWitnessSite.x.FloorToInt(),
                Plan.FirstWitnessSite.y.FloorToInt(),
                Plan.SecondWitnessSite.x.FloorToInt(),
                Plan.SecondWitnessSite.y.FloorToInt(),
                Plan.ConfluenceSite.x.FloorToInt(),
                Plan.ConfluenceSite.y.FloorToInt(),
                UnburiedRoadTerrainDelta,
                GlassScarBlockedTiles);
        }
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
        if (bResearchPresentationScenario)
        {
            const echoes::sim::PlayerState* Player =
                Simulation->FindPlayer(LocalPlayerId);
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_RESEARCH_PRESENTATION_READY] technology=%s producerQueued=true controlled=true release=false material=%d dawn=%d"),
                ResearchStableName(ResearchPresentationTechnology),
                Player != nullptr ? Player->resources.material : 0,
                Player != nullptr ? Player->resources.dawnshards : 0);
        }
        if (bResearchInterruptionPresentationScenario)
        {
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_RESEARCH_INTERRUPTION_PRESENTATION_READY] technology=%s attackers=32 attackTick=60 controlled=true release=false"),
                ResearchStableName(ResearchPresentationTechnology));
        }
        if (bKharuunSystemsPresentationScenario)
        {
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_KHARUUN_SYSTEMS_PRESENTATION_READY] commands=3 hiddenMovers=1 controlled=true release=false"));
        }
        if (bPrologueCompletionPresentationScenario)
        {
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_PROLOGUE_COMPLETION_PRESENTATION_READY] carrier=%u worker=%u well=%u protocol=Preserve controlled=true release=false ledgerPath=%s"),
                ArchiveCarrierId,
                ProloguePresentationWorkerId,
                ProloguePresentationWellId,
                *CampaignProgressPath);
        }
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
    bResearchPresentationScenario = false;
    bResearchInterruptionPresentationScenario = false;
    bKharuunSystemsPresentationScenario = false;
    bPrologueCompletionPresentationScenario = false;
    bLoggedResearchPresentationActive = false;
    bLoggedResearchPresentationComplete = false;
    bLoggedResearchPresentationInterrupted = false;
    bLoggedKharuunSystemsPresentation = false;
    PrologueCompletionPresentationStage = 0;
    ProloguePresentationWorkerId = 0;
    ProloguePresentationWellId = 0;
    bSimulationPaused = false;
    bMatchResultReported = false;
    bStressScenario = false;
    ArchiveCarrierId = 0;
    MemoryBearerId = 0;
    MigrationWaystoneId = 0;
    LifeSupportDistrictId = 0;
    TransitDistrictId = 0;
    ArchiveDistrictId = 0;
    MeridianContinuanceRelayId = 0;
    KharuunContinuanceSpineId = 0;
    MeridianContinuanceWitnessId = 0;
    KharuunContinuanceWitnessId = 0;
    TalarId = 0;
    CensusArchiveId = 0;
    FirstCivilianId = 0;
    SecondCivilianId = 0;
    OruunId = 0;
    FirstMemoryWitnessId = 0;
    SecondMemoryWitnessId = 0;
    ResearchPresentationTechnology = echoes::sim::ResearchType::None;
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
    if (SelectedOperation == EEchoesOperationMode::CampaignPrologue &&
        NewFaction != Faction::MeridianCompact)
    {
        OutFeedback = TEXT("[FACTION_PROLOGUE_LOCKED] Mara Vey deploys with the Meridian Compact.");
        return false;
    }
    if (SelectedOperation == EEchoesOperationMode::CampaignSevenAccounts &&
        NewFaction != Faction::KharuunAssemblies)
    {
        OutFeedback = TEXT("[FACTION_SEVEN_ACCOUNTS_LOCKED] Oruun deploys with the Kharuun Assemblies.");
        return false;
    }
    if (SelectedOperation == EEchoesOperationMode::CampaignUnburiedRoad &&
        NewFaction != Faction::KharuunAssemblies)
    {
        OutFeedback = TEXT("[FACTION_UNBURIED_ROAD_LOCKED] Oruun deploys with the Kharuun Assemblies.");
        return false;
    }
    if (SelectedOperation ==
            EEchoesOperationMode::CampaignTermsOfContinuance &&
        NewFaction != Faction::MeridianCompact)
    {
        OutFeedback = TEXT("[FACTION_TERMS_OF_CONTINUANCE_LOCKED] Meridian-authoritative treaty proxies are fixed for this prototype mission.");
        return false;
    }
    if (SelectedOperation ==
            EEchoesOperationMode::CampaignNamesWithoutBirths &&
        NewFaction != Faction::MeridianCompact)
    {
        OutFeedback = TEXT("[FACTION_NAMES_WITHOUT_BIRTHS_LOCKED] Talar's protected archive convoy deploys under Meridian command authority.");
        return false;
    }
    if (SelectedOperation ==
            EEchoesOperationMode::CampaignShapeOfSilence &&
        NewFaction != Faction::KharuunAssemblies)
    {
        OutFeedback = TEXT("[FACTION_SHAPE_OF_SILENCE_LOCKED] Oruun and both memory witnesses deploy under Kharuun authority.");
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

bool UEchoesSimulationSubsystem::SelectOperationMode(
    EEchoesOperationMode NewOperation,
    FString& OutFeedback)
{
    OutFeedback.Reset();
    if (bStressScenario)
    {
        OutFeedback = TEXT("[OPERATION_STRESS_LOCKED] The scale fixture has a fixed operation.");
        return false;
    }
    if (NewOperation == EEchoesOperationMode::CampaignSevenAccounts &&
        !IsSevenAccountsUnlocked())
    {
        OutFeedback = TEXT("[CAMPAIGN_MISSION_LOCKED] Complete What the Ledger Keeps before Seven Accounts of Rain.");
        return false;
    }
    if (NewOperation == EEchoesOperationMode::CampaignCityReserve &&
        !IsCityReserveUnlocked())
    {
        OutFeedback = TEXT("[CAMPAIGN_MISSION_LOCKED] Complete Seven Accounts of Rain with a consistent ledger before A City on Reserve.");
        return false;
    }
    if (NewOperation == EEchoesOperationMode::CampaignUnburiedRoad &&
        !IsUnburiedRoadUnlocked())
    {
        OutFeedback = TEXT("[CAMPAIGN_MISSION_LOCKED] Complete A City on Reserve with a consistent ledger before The Unburied Road.");
        return false;
    }
    if (NewOperation ==
            EEchoesOperationMode::CampaignTermsOfContinuance &&
        !IsTermsOfContinuanceUnlocked())
    {
        OutFeedback = TEXT("[CAMPAIGN_MISSION_LOCKED] Complete The Unburied Road with a consistent ledger before Terms of Continuance.");
        return false;
    }
    if (NewOperation ==
            EEchoesOperationMode::CampaignNamesWithoutBirths &&
        !IsNamesWithoutBirthsUnlocked())
    {
        OutFeedback = TEXT("[CAMPAIGN_MISSION_LOCKED] Complete Terms of Continuance with a consistent ledger before Names Without Births.");
        return false;
    }
    if (NewOperation ==
            EEchoesOperationMode::CampaignShapeOfSilence &&
        !IsShapeOfSilenceUnlocked())
    {
        OutFeedback = TEXT("[CAMPAIGN_MISSION_LOCKED] Complete Names Without Births with a consistent ledger before The Shape of Silence.");
        return false;
    }
    if (NewOperation == SelectedOperation)
    {
        OutFeedback = FString::Printf(TEXT("OPERATION: %s already selected."), *GetOperationLabel());
        return true;
    }

    const EEchoesOperationMode PreviousOperation = SelectedOperation;
    const Faction PreviousFaction = LocalFaction;
    const bool bHadScenario = bScenarioReady && Simulation.IsValid();
    const bool bWasPaused = bSimulationPaused;
    if (bHadScenario)
    {
        StopPrototypeScenario();
    }
    SelectedOperation = NewOperation;
    if (SelectedOperation == EEchoesOperationMode::CampaignPrologue)
    {
        LocalFaction = Faction::MeridianCompact;
    }
    else if (SelectedOperation == EEchoesOperationMode::CampaignSevenAccounts)
    {
        LocalFaction = Faction::KharuunAssemblies;
    }
    else if (SelectedOperation == EEchoesOperationMode::CampaignCityReserve)
    {
        LocalFaction = Faction::MeridianCompact;
    }
    else if (SelectedOperation == EEchoesOperationMode::CampaignUnburiedRoad)
    {
        LocalFaction = Faction::KharuunAssemblies;
    }
    else if (SelectedOperation ==
             EEchoesOperationMode::CampaignTermsOfContinuance)
    {
        LocalFaction = Faction::MeridianCompact;
    }
    else if (SelectedOperation ==
             EEchoesOperationMode::CampaignNamesWithoutBirths)
    {
        LocalFaction = Faction::MeridianCompact;
    }
    else if (SelectedOperation ==
             EEchoesOperationMode::CampaignShapeOfSilence)
    {
        LocalFaction = Faction::KharuunAssemblies;
    }
    if (!bHadScenario || StartScenario(false))
    {
        if (bHadScenario)
        {
            SetScenarioPaused(bWasPaused);
        }
        OutFeedback = FString::Printf(
            TEXT("OPERATION SELECTED: %s%s"),
            *GetOperationLabel(),
            SelectedOperation == EEchoesOperationMode::CampaignPrologue
                ? TEXT(" — Mara Vey's Meridian force is locked for this mission.")
            : SelectedOperation == EEchoesOperationMode::CampaignSevenAccounts
                ? TEXT(" — Oruun's Kharuun migration force is locked for this mission.")
            : SelectedOperation == EEchoesOperationMode::CampaignCityReserve
                ? TEXT(" — Mara Vey's Meridian grid force is locked for this mission.")
            : SelectedOperation == EEchoesOperationMode::CampaignUnburiedRoad
                ? TEXT(" — Oruun's Kharuun road force is locked for this mission.")
            : SelectedOperation ==
                      EEchoesOperationMode::CampaignTermsOfContinuance
                ? TEXT(" — Meridian-authoritative treaty proxies are locked for this prototype mission.")
            : SelectedOperation ==
                      EEchoesOperationMode::CampaignNamesWithoutBirths
                ? TEXT(" — Talar and the civilian archive convoy are locked under Meridian authority.")
            : SelectedOperation ==
                      EEchoesOperationMode::CampaignShapeOfSilence
                ? TEXT(" — Oruun and both memory witnesses are locked under Kharuun authority.")
                : TEXT("."));
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_OPERATION_SELECTED] operation=%s scenarioReset=%s paused=%s"),
            SelectedOperation == EEchoesOperationMode::CampaignPrologue
                ? TEXT("WhatTheLedgerKeeps")
            : SelectedOperation == EEchoesOperationMode::CampaignSevenAccounts
                ? TEXT("SevenAccountsOfRain")
            : SelectedOperation == EEchoesOperationMode::CampaignCityReserve
                ? TEXT("ACityOnReserve")
            : SelectedOperation == EEchoesOperationMode::CampaignUnburiedRoad
                ? TEXT("TheUnburiedRoad")
            : SelectedOperation ==
                      EEchoesOperationMode::CampaignTermsOfContinuance
                ? TEXT("TermsOfContinuance")
            : SelectedOperation ==
                      EEchoesOperationMode::CampaignNamesWithoutBirths
                ? TEXT("NamesWithoutBirths")
            : SelectedOperation ==
                      EEchoesOperationMode::CampaignShapeOfSilence
                ? TEXT("TheShapeOfSilence")
                : TEXT("GlassScar"),
            bHadScenario ? TEXT("true") : TEXT("false"),
            bWasPaused ? TEXT("true") : TEXT("false"));
        return true;
    }

    StopPrototypeScenario();
    SelectedOperation = PreviousOperation;
    LocalFaction = PreviousFaction;
    const bool bRollbackSucceeded = !bHadScenario || StartScenario(false);
    if (bRollbackSucceeded && bHadScenario)
    {
        SetScenarioPaused(bWasPaused);
    }
    OutFeedback = bRollbackSucceeded
                      ? TEXT("[OPERATION_REBUILD_FAILED] The prior operation was restored.")
                      : TEXT("[OPERATION_ROLLBACK_FAILED] The operation could not be restored.");
    return false;
}

bool UEchoesSimulationSubsystem::StartNewCampaign(FString& OutFeedback)
{
    OutFeedback.Reset();
    if (!bCampaignProgressAvailable)
    {
        OutFeedback = TEXT("[CAMPAIGN_LEDGER_UNAVAILABLE] A new campaign could not be created because campaign storage is unavailable.");
        return false;
    }
    if (CampaignProgress.Decisions.IsEmpty())
    {
        OutFeedback = TEXT("NEW CAMPAIGN: the campaign ledger is already empty.");
        return true;
    }

    FEchoesCampaignProgress EmptyCampaign;
    FString SaveFeedback;
    if (!FEchoesCampaignProgressStore::SaveAtomic(
            CampaignProgressPath,
            EmptyCampaign,
            SaveFeedback))
    {
        OutFeedback = SaveFeedback;
        return false;
    }

    const int32 ReplacedDecisionCount = CampaignProgress.Decisions.Num();
    const bool bHadScenario = bScenarioReady && Simulation.IsValid();
    const bool bWasPaused = bSimulationPaused;
    CampaignProgress = MoveTemp(EmptyCampaign);
    RefreshCampaignBackupState();
    if (bHadScenario)
    {
        StopPrototypeScenario();
    }
    SelectedOperation = EEchoesOperationMode::Skirmish;
    LocalFaction = Faction::MeridianCompact;
    if (bHadScenario && !StartScenario(false))
    {
        OutFeedback = TEXT("[NEW_CAMPAIGN_REBUILD_FAILED] The empty ledger was committed, but the default operation could not be rebuilt.");
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NEW_CAMPAIGN_FAILED] stage=scenario_rebuild replacedRecords=%d backupRetained=true"),
            ReplacedDecisionCount);
        return false;
    }
    if (bHadScenario)
    {
        SetScenarioPaused(bWasPaused);
    }
    OutFeedback = FString::Printf(
        TEXT("NEW CAMPAIGN CREATED: %d prior mission record%s replaced; one prior ledger generation retained as backup."),
        ReplacedDecisionCount,
        ReplacedDecisionCount == 1 ? TEXT("") : TEXT("s"));
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_NEW_CAMPAIGN_CREATED] replacedRecords=%d operation=GlassScar local=MeridianCompact backupRetained=true scenarioReset=%s"),
        ReplacedDecisionCount,
        bHadScenario ? TEXT("true") : TEXT("false"));
    return true;
}

void UEchoesSimulationSubsystem::RefreshCampaignBackupState()
{
    CampaignBackupProgress = FEchoesCampaignProgress{};
    bCampaignBackupAvailable = false;
    if (!bCampaignProgressAvailable)
    {
        return;
    }

    FString BackupFeedback;
    FEchoesCampaignProgress Candidate;
    if (!FEchoesCampaignProgressStore::LoadGeneration(
            CampaignProgressPath + TEXT(".bak"),
            Candidate,
            BackupFeedback))
    {
        return;
    }
    if (Candidate.Decisions == CampaignProgress.Decisions)
    {
        return;
    }
    CampaignBackupProgress = MoveTemp(Candidate);
    bCampaignBackupAvailable = true;
}

bool UEchoesSimulationSubsystem::RestoreCampaignBackup(FString& OutFeedback)
{
    OutFeedback.Reset();
    if (!bCampaignProgressAvailable)
    {
        OutFeedback = TEXT("[CAMPAIGN_LEDGER_UNAVAILABLE] Campaign recovery is unavailable because campaign storage is unavailable.");
        return false;
    }

    RefreshCampaignBackupState();
    if (!bCampaignBackupAvailable)
    {
        OutFeedback = TEXT("[CAMPAIGN_BACKUP_UNAVAILABLE] No distinct validated prior campaign generation is available.");
        return false;
    }

    const FEchoesCampaignProgress RestoredCampaign = CampaignBackupProgress;
    const int32 ReplacedDecisionCount = CampaignProgress.Decisions.Num();
    const int32 RestoredDecisionCount = RestoredCampaign.Decisions.Num();
    FString SaveFeedback;
    if (!FEchoesCampaignProgressStore::SaveAtomic(
            CampaignProgressPath,
            RestoredCampaign,
            SaveFeedback))
    {
        OutFeedback = SaveFeedback;
        return false;
    }

    const bool bHadScenario = bScenarioReady && Simulation.IsValid();
    const bool bWasPaused = bSimulationPaused;
    CampaignProgress = RestoredCampaign;
    RefreshCampaignBackupState();
    if (bHadScenario)
    {
        StopPrototypeScenario();
    }
    SelectedOperation = EEchoesOperationMode::Skirmish;
    LocalFaction = Faction::MeridianCompact;
    if (bHadScenario && !StartScenario(false))
    {
        OutFeedback = TEXT("[CAMPAIGN_RESTORE_REBUILD_FAILED] The prior ledger was restored, but the default operation could not be rebuilt.");
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_CAMPAIGN_RESTORE_FAILED] stage=scenario_rebuild restoredRecords=%d replacedRecords=%d activeRetainedAsBackup=true"),
            RestoredDecisionCount,
            ReplacedDecisionCount);
        return false;
    }
    if (bHadScenario)
    {
        SetScenarioPaused(bWasPaused);
    }
    OutFeedback = FString::Printf(
        TEXT("CAMPAIGN RESTORED: prior generation with %d mission record%s is active; the replaced %d-record generation is retained as backup."),
        RestoredDecisionCount,
        RestoredDecisionCount == 1 ? TEXT("") : TEXT("s"),
        ReplacedDecisionCount);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_CAMPAIGN_RESTORED] restoredRecords=%d replacedRecords=%d operation=GlassScar local=MeridianCompact activeRetainedAsBackup=true scenarioReset=%s"),
        RestoredDecisionCount,
        ReplacedDecisionCount,
        bHadScenario ? TEXT("true") : TEXT("false"));
    return true;
}

FString UEchoesSimulationSubsystem::GetQuickSavePath()
{
    return FPaths::Combine(
        FPaths::ProjectSavedDir(),
        TEXT("SaveGames"),
        TEXT("EchoesQuickSave.bin"));
}

FString UEchoesSimulationSubsystem::GetActiveQuickSavePath() const
{
    if (SelectedOperation == EEchoesOperationMode::CampaignPrologue)
    {
        return FPaths::Combine(
            FPaths::ProjectSavedDir(),
            TEXT("SaveGames"),
            TEXT("EchoesQuickSaveWhatTheLedgerKeeps.bin"));
    }
    if (SelectedOperation == EEchoesOperationMode::CampaignSevenAccounts)
    {
        return FPaths::Combine(
            FPaths::ProjectSavedDir(),
            TEXT("SaveGames"),
            TEXT("EchoesQuickSaveSevenAccountsOfRain.bin"));
    }
    if (SelectedOperation == EEchoesOperationMode::CampaignCityReserve)
    {
        return FPaths::Combine(
            FPaths::ProjectSavedDir(),
            TEXT("SaveGames"),
            TEXT("EchoesQuickSaveACityOnReserve.bin"));
    }
    if (SelectedOperation == EEchoesOperationMode::CampaignUnburiedRoad)
    {
        return FPaths::Combine(
            FPaths::ProjectSavedDir(),
            TEXT("SaveGames"),
            TEXT("EchoesQuickSaveTheUnburiedRoad.bin"));
    }
    if (SelectedOperation ==
            EEchoesOperationMode::CampaignTermsOfContinuance ||
        SelectedOperation ==
            EEchoesOperationMode::CampaignNamesWithoutBirths ||
        SelectedOperation ==
            EEchoesOperationMode::CampaignShapeOfSilence)
    {
        TArray<uint8> LedgerBytes;
        FString LedgerError;
        if (FEchoesCampaignProgressStore::Encode(
                CampaignProgress,
                LedgerBytes,
                LedgerError) &&
            !LedgerBytes.IsEmpty())
        {
            const uint32 LedgerFingerprint =
                FCrc::MemCrc32(
                    LedgerBytes.GetData(),
                    LedgerBytes.Num() - static_cast<int32>(sizeof(uint32)));
            const TCHAR* Prefix =
                SelectedOperation ==
                        EEchoesOperationMode::CampaignTermsOfContinuance
                    ? TEXT("EchoesQuickSaveTermsOfContinuance")
                : SelectedOperation ==
                        EEchoesOperationMode::CampaignNamesWithoutBirths
                    ? TEXT("EchoesQuickSaveNamesWithoutBirths")
                    : TEXT("EchoesQuickSaveTheShapeOfSilence");
            return FPaths::Combine(
                FPaths::ProjectSavedDir(),
                TEXT("SaveGames"),
                FString::Printf(TEXT("%s-%08X.bin"), Prefix, LedgerFingerprint));
        }
        return FPaths::Combine(
            FPaths::ProjectSavedDir(),
            TEXT("SaveGames"),
            SelectedOperation ==
                    EEchoesOperationMode::CampaignTermsOfContinuance
                ? TEXT("EchoesQuickSaveTermsOfContinuance-InvalidLedger.bin")
            : SelectedOperation ==
                    EEchoesOperationMode::CampaignNamesWithoutBirths
                ? TEXT("EchoesQuickSaveNamesWithoutBirths-InvalidLedger.bin")
                : TEXT("EchoesQuickSaveTheShapeOfSilence-InvalidLedger.bin"));
    }
    return GetQuickSavePath();
}

EEchoesCampaignCommitStatus UEchoesSimulationSubsystem::CommitPrologueCompletion(
    echoes::sim::FutureWellChoice CurrentChoice,
    echoes::sim::FutureWellChoice& OutRecordedChoice,
    FString& OutFeedback)
{
    OutRecordedChoice = CurrentChoice;
    if (!bCampaignProgressAvailable || !Simulation.IsValid())
    {
        OutFeedback = TEXT("[CAMPAIGN_LEDGER_UNAVAILABLE] Mission completion is valid, but campaign progress could not be saved.");
        return EEchoesCampaignCommitStatus::StorageFailure;
    }
    if (SelectedOperation != EEchoesOperationMode::CampaignPrologue ||
        GetProloguePhase() != EEchoesProloguePhase::Complete ||
        CurrentChoice == echoes::sim::FutureWellChoice::Dormant)
    {
        OutFeedback = TEXT("[CAMPAIGN_COMPLETION_UNVERIFIED] No completed authoritative prologue can be recorded.");
        return EEchoesCampaignCommitStatus::StorageFailure;
    }

    FEchoesCampaignDecisionRecord Record;
    Record.Mission = EEchoesCampaignMissionId::WhatTheLedgerKeeps;
    Record.WellChoice = CurrentChoice;
    Record.AvailableWellChoices = 0x07;
    Record.VerifiedFacts =
        static_cast<uint8>(EEchoesCampaignDecisionFact::ArchiveRecovered) |
        static_cast<uint8>(EEchoesCampaignDecisionFact::CarrierEvacuated) |
        static_cast<uint8>(EEchoesCampaignDecisionFact::LocalCoreSurvived) |
        static_cast<uint8>(EEchoesCampaignDecisionFact::FutureWellControlled);
    Record.SimulationSnapshotVersion = echoes::sim::kSnapshotVersion;
    Record.CompletionTick = Simulation->CurrentTick();
    Record.FinalStateChecksum = Simulation->StateChecksum();

    FEchoesCampaignProgress Candidate = CampaignProgress;
    const EEchoesCampaignCommitStatus Status =
        Candidate.AppendDecision(Record, OutFeedback);
    if (Status == EEchoesCampaignCommitStatus::StorageFailure)
    {
        return Status;
    }
    if (const FEchoesCampaignDecisionRecord* Existing =
            Candidate.FindDecision(Record.Mission))
    {
        OutRecordedChoice = Existing->WellChoice;
    }
    if (Status != EEchoesCampaignCommitStatus::Added)
    {
        return Status;
    }

    FString SaveFeedback;
    if (!FEchoesCampaignProgressStore::SaveAtomic(
            CampaignProgressPath,
            Candidate,
            SaveFeedback))
    {
        OutFeedback = SaveFeedback;
        return EEchoesCampaignCommitStatus::StorageFailure;
    }
    CampaignProgress = MoveTemp(Candidate);
    RefreshCampaignBackupState();
    OutFeedback = SaveFeedback;
    return EEchoesCampaignCommitStatus::Added;
}

EEchoesCampaignCommitStatus
UEchoesSimulationSubsystem::CommitSevenAccountsCompletion(
    echoes::sim::FutureWellChoice& OutRecordedChoice,
    FString& OutFeedback)
{
    const FutureWellChoice Branch = GetRecordedPrologueChoice();
    OutRecordedChoice = Branch;
    if (!bCampaignProgressAvailable || !Simulation.IsValid())
    {
        OutFeedback = TEXT("[CAMPAIGN_LEDGER_UNAVAILABLE] Mission completion is valid, but campaign progress could not be saved.");
        return EEchoesCampaignCommitStatus::StorageFailure;
    }
    if (SelectedOperation != EEchoesOperationMode::CampaignSevenAccounts ||
        GetSevenAccountsPhase() != EEchoesSevenAccountsPhase::Complete ||
        Branch == FutureWellChoice::Dormant)
    {
        OutFeedback = TEXT("[CAMPAIGN_COMPLETION_UNVERIFIED] No completed authoritative Seven Accounts operation can be recorded.");
        return EEchoesCampaignCommitStatus::StorageFailure;
    }

    FEchoesCampaignDecisionRecord Record;
    Record.Mission = EEchoesCampaignMissionId::SevenAccountsOfRain;
    Record.WellChoice = Branch;
    Record.AvailableWellChoices = WellChoiceMask(Branch);
    Record.VerifiedFacts =
        static_cast<uint8>(EEchoesSevenAccountsCompletionFact::WaystoneRootedAtAnchor) |
        static_cast<uint8>(EEchoesSevenAccountsCompletionFact::MemoryBearerArrived) |
        static_cast<uint8>(EEchoesSevenAccountsCompletionFact::LocalCoreSurvived) |
        static_cast<uint8>(EEchoesSevenAccountsCompletionFact::PriorDecisionConsumed);
    Record.SimulationSnapshotVersion = echoes::sim::kSnapshotVersion;
    Record.CompletionTick = Simulation->CurrentTick();
    Record.FinalStateChecksum = Simulation->StateChecksum();

    FEchoesCampaignProgress Candidate = CampaignProgress;
    const EEchoesCampaignCommitStatus Status =
        Candidate.AppendDecision(Record, OutFeedback);
    if (Status == EEchoesCampaignCommitStatus::StorageFailure)
    {
        return Status;
    }
    if (const FEchoesCampaignDecisionRecord* Existing =
            Candidate.FindDecision(Record.Mission))
    {
        OutRecordedChoice = Existing->WellChoice;
    }
    if (Status != EEchoesCampaignCommitStatus::Added)
    {
        return Status;
    }

    FString SaveFeedback;
    if (!FEchoesCampaignProgressStore::SaveAtomic(
            CampaignProgressPath,
            Candidate,
            SaveFeedback))
    {
        OutFeedback = SaveFeedback;
        return EEchoesCampaignCommitStatus::StorageFailure;
    }
    CampaignProgress = MoveTemp(Candidate);
    RefreshCampaignBackupState();
    OutFeedback = SaveFeedback;
    return EEchoesCampaignCommitStatus::Added;
}

EEchoesCampaignCommitStatus
UEchoesSimulationSubsystem::CommitCityReserveCompletion(
    echoes::sim::FutureWellChoice& OutRecordedChoice,
    FString& OutFeedback)
{
    const FutureWellChoice Branch = GetRecordedPrologueChoice();
    OutRecordedChoice = Branch;
    if (!bCampaignProgressAvailable || !Simulation.IsValid())
    {
        OutFeedback = TEXT("[CAMPAIGN_LEDGER_UNAVAILABLE] Mission completion is valid, but campaign progress could not be saved.");
        return EEchoesCampaignCommitStatus::StorageFailure;
    }
    if (SelectedOperation != EEchoesOperationMode::CampaignCityReserve ||
        GetCityReservePhase() != EEchoesCityReservePhase::Complete ||
        !IsCityReserveUnlocked() ||
        Branch == FutureWellChoice::Dormant)
    {
        OutFeedback = TEXT("[CAMPAIGN_COMPLETION_UNVERIFIED] No completed authoritative City on Reserve operation can be recorded.");
        return EEchoesCampaignCommitStatus::StorageFailure;
    }

    FEchoesCampaignDecisionRecord Record;
    Record.Mission = EEchoesCampaignMissionId::ACityOnReserve;
    Record.WellChoice = Branch;
    Record.AvailableWellChoices = WellChoiceMask(Branch);
    Record.VerifiedFacts =
        static_cast<uint8>(EEchoesCityReserveCompletionFact::LifeSupportPowered) |
        static_cast<uint8>(EEchoesCityReserveCompletionFact::TransitPowered) |
        static_cast<uint8>(EEchoesCityReserveCompletionFact::ArchivePowered) |
        static_cast<uint8>(EEchoesCityReserveCompletionFact::LocalCoreSurvived) |
        static_cast<uint8>(EEchoesCityReserveCompletionFact::PriorLedgerConsumed);
    Record.SimulationSnapshotVersion = echoes::sim::kSnapshotVersion;
    Record.CompletionTick = Simulation->CurrentTick();
    Record.FinalStateChecksum = Simulation->StateChecksum();

    FEchoesCampaignProgress Candidate = CampaignProgress;
    const EEchoesCampaignCommitStatus Status =
        Candidate.AppendDecision(Record, OutFeedback);
    if (Status == EEchoesCampaignCommitStatus::StorageFailure)
    {
        return Status;
    }
    if (const FEchoesCampaignDecisionRecord* Existing =
            Candidate.FindDecision(Record.Mission))
    {
        OutRecordedChoice = Existing->WellChoice;
    }
    if (Status != EEchoesCampaignCommitStatus::Added)
    {
        return Status;
    }

    FString SaveFeedback;
    if (!FEchoesCampaignProgressStore::SaveAtomic(
            CampaignProgressPath,
            Candidate,
            SaveFeedback))
    {
        OutFeedback = SaveFeedback;
        return EEchoesCampaignCommitStatus::StorageFailure;
    }
    CampaignProgress = MoveTemp(Candidate);
    RefreshCampaignBackupState();
    OutFeedback = SaveFeedback;
    return EEchoesCampaignCommitStatus::Added;
}

EEchoesCampaignCommitStatus
UEchoesSimulationSubsystem::CommitUnburiedRoadCompletion(
    echoes::sim::FutureWellChoice& OutRecordedChoice,
    FString& OutFeedback)
{
    const FutureWellChoice Branch = GetRecordedPrologueChoice();
    OutRecordedChoice = Branch;
    if (!bCampaignProgressAvailable || !Simulation.IsValid())
    {
        OutFeedback = TEXT("[CAMPAIGN_LEDGER_UNAVAILABLE] Mission completion is valid, but campaign progress could not be saved.");
        return EEchoesCampaignCommitStatus::StorageFailure;
    }
    if (SelectedOperation != EEchoesOperationMode::CampaignUnburiedRoad ||
        GetUnburiedRoadPhase() != EEchoesUnburiedRoadPhase::Complete ||
        !IsUnburiedRoadUnlocked() || Branch == FutureWellChoice::Dormant)
    {
        OutFeedback = TEXT("[CAMPAIGN_COMPLETION_UNVERIFIED] No completed authoritative Unburied Road operation can be recorded.");
        return EEchoesCampaignCommitStatus::StorageFailure;
    }

    FEchoesCampaignDecisionRecord Record;
    Record.Mission = EEchoesCampaignMissionId::TheUnburiedRoad;
    Record.WellChoice = Branch;
    Record.AvailableWellChoices = WellChoiceMask(Branch);
    Record.VerifiedFacts =
        static_cast<uint8>(EEchoesUnburiedRoadCompletionFact::WaystoneRootedAtRoadhead) |
        static_cast<uint8>(EEchoesUnburiedRoadCompletionFact::ListeningSpineRaised) |
        static_cast<uint8>(EEchoesUnburiedRoadCompletionFact::MemoryShardRecovered) |
        static_cast<uint8>(EEchoesUnburiedRoadCompletionFact::LocalCoreSurvived) |
        static_cast<uint8>(EEchoesUnburiedRoadCompletionFact::PriorLedgerConsumed);
    Record.SimulationSnapshotVersion = echoes::sim::kSnapshotVersion;
    Record.CompletionTick = Simulation->CurrentTick();
    Record.FinalStateChecksum = Simulation->StateChecksum();

    FEchoesCampaignProgress Candidate = CampaignProgress;
    const EEchoesCampaignCommitStatus Status =
        Candidate.AppendDecision(Record, OutFeedback);
    if (Status == EEchoesCampaignCommitStatus::StorageFailure)
    {
        return Status;
    }
    if (const FEchoesCampaignDecisionRecord* Existing =
            Candidate.FindDecision(Record.Mission))
    {
        OutRecordedChoice = Existing->WellChoice;
    }
    if (Status != EEchoesCampaignCommitStatus::Added)
    {
        return Status;
    }

    FString SaveFeedback;
    if (!FEchoesCampaignProgressStore::SaveAtomic(
            CampaignProgressPath,
            Candidate,
            SaveFeedback))
    {
        OutFeedback = SaveFeedback;
        return EEchoesCampaignCommitStatus::StorageFailure;
    }
    CampaignProgress = MoveTemp(Candidate);
    RefreshCampaignBackupState();
    OutFeedback = SaveFeedback;
    return EEchoesCampaignCommitStatus::Added;
}

EEchoesCampaignCommitStatus
UEchoesSimulationSubsystem::CommitTermsOfContinuanceCompletion(
    echoes::sim::FutureWellChoice& OutRecordedChoice,
    FString& OutFeedback)
{
    const FutureWellChoice Branch = GetRecordedPrologueChoice();
    OutRecordedChoice = Branch;
    if (!bCampaignProgressAvailable || !Simulation.IsValid())
    {
        OutFeedback = TEXT("[CAMPAIGN_LEDGER_UNAVAILABLE] Mission completion is valid, but campaign progress could not be saved.");
        return EEchoesCampaignCommitStatus::StorageFailure;
    }
    if (SelectedOperation !=
            EEchoesOperationMode::CampaignTermsOfContinuance ||
        GetTermsOfContinuancePhase() !=
            EEchoesTermsOfContinuancePhase::Complete ||
        !IsTermsOfContinuanceUnlocked() ||
        Branch == FutureWellChoice::Dormant)
    {
        OutFeedback = TEXT("[CAMPAIGN_COMPLETION_UNVERIFIED] No completed authoritative Terms of Continuance operation can be recorded.");
        return EEchoesCampaignCommitStatus::StorageFailure;
    }

    FEchoesCampaignDecisionRecord Record;
    Record.Mission = EEchoesCampaignMissionId::TermsOfContinuance;
    Record.WellChoice = Branch;
    Record.AvailableWellChoices = WellChoiceMask(Branch);
    Record.VerifiedFacts =
        static_cast<uint8>(EEchoesTermsOfContinuanceCompletionFact::MeridianRelaySynchronized) |
        static_cast<uint8>(EEchoesTermsOfContinuanceCompletionFact::KharuunSpineSynchronized) |
        static_cast<uint8>(EEchoesTermsOfContinuanceCompletionFact::ContinuanceWindowHeld) |
        static_cast<uint8>(EEchoesTermsOfContinuanceCompletionFact::BothWitnessesExtracted) |
        static_cast<uint8>(EEchoesTermsOfContinuanceCompletionFact::LocalCoreSurvived) |
        static_cast<uint8>(EEchoesTermsOfContinuanceCompletionFact::PriorLedgerConsumed);
    Record.SimulationSnapshotVersion = echoes::sim::kSnapshotVersion;
    Record.CompletionTick = Simulation->CurrentTick();
    Record.FinalStateChecksum = Simulation->StateChecksum();

    FEchoesCampaignProgress Candidate = CampaignProgress;
    const EEchoesCampaignCommitStatus Status =
        Candidate.AppendDecision(Record, OutFeedback);
    if (Status == EEchoesCampaignCommitStatus::StorageFailure)
    {
        return Status;
    }
    if (const FEchoesCampaignDecisionRecord* Existing =
            Candidate.FindDecision(Record.Mission))
    {
        OutRecordedChoice = Existing->WellChoice;
    }
    if (Status != EEchoesCampaignCommitStatus::Added)
    {
        return Status;
    }

    FString SaveFeedback;
    if (!FEchoesCampaignProgressStore::SaveAtomic(
            CampaignProgressPath,
            Candidate,
            SaveFeedback))
    {
        OutFeedback = SaveFeedback;
        return EEchoesCampaignCommitStatus::StorageFailure;
    }
    CampaignProgress = MoveTemp(Candidate);
    RefreshCampaignBackupState();
    OutFeedback = SaveFeedback;
    return EEchoesCampaignCommitStatus::Added;
}

EEchoesCampaignCommitStatus
UEchoesSimulationSubsystem::CommitNamesWithoutBirthsCompletion(
    echoes::sim::FutureWellChoice& OutRecordedChoice,
    FString& OutFeedback)
{
    const FutureWellChoice Branch = GetRecordedPrologueChoice();
    OutRecordedChoice = Branch;
    if (!bCampaignProgressAvailable || !Simulation.IsValid())
    {
        OutFeedback = TEXT("[CAMPAIGN_LEDGER_UNAVAILABLE] Mission completion is valid, but campaign progress could not be saved.");
        return EEchoesCampaignCommitStatus::StorageFailure;
    }
    if (SelectedOperation !=
            EEchoesOperationMode::CampaignNamesWithoutBirths ||
        GetNamesWithoutBirthsPhase() !=
            EEchoesNamesWithoutBirthsPhase::Complete ||
        !IsNamesWithoutBirthsUnlocked() ||
        Branch == FutureWellChoice::Dormant)
    {
        OutFeedback = TEXT("[CAMPAIGN_COMPLETION_UNVERIFIED] No completed authoritative Names Without Births operation can be recorded.");
        return EEchoesCampaignCommitStatus::StorageFailure;
    }

    FEchoesCampaignDecisionRecord Record;
    Record.Mission = EEchoesCampaignMissionId::NamesWithoutBirths;
    Record.WellChoice = Branch;
    Record.AvailableWellChoices = WellChoiceMask(Branch);
    Record.VerifiedFacts =
        static_cast<uint8>(EEchoesNamesWithoutBirthsCompletionFact::CensusEvidenceLocated) |
        static_cast<uint8>(EEchoesNamesWithoutBirthsCompletionFact::ArchivePowered) |
        static_cast<uint8>(EEchoesNamesWithoutBirthsCompletionFact::BothCiviliansSheltered) |
        static_cast<uint8>(EEchoesNamesWithoutBirthsCompletionFact::EvidenceExtracted) |
        static_cast<uint8>(EEchoesNamesWithoutBirthsCompletionFact::LocalCoreSurvived) |
        static_cast<uint8>(EEchoesNamesWithoutBirthsCompletionFact::PriorLedgerConsumed);
    Record.SimulationSnapshotVersion = echoes::sim::kSnapshotVersion;
    Record.CompletionTick = Simulation->CurrentTick();
    Record.FinalStateChecksum = Simulation->StateChecksum();

    FEchoesCampaignProgress Candidate = CampaignProgress;
    const EEchoesCampaignCommitStatus Status =
        Candidate.AppendDecision(Record, OutFeedback);
    if (Status == EEchoesCampaignCommitStatus::StorageFailure)
    {
        return Status;
    }
    if (const FEchoesCampaignDecisionRecord* Existing =
            Candidate.FindDecision(Record.Mission))
    {
        OutRecordedChoice = Existing->WellChoice;
    }
    if (Status != EEchoesCampaignCommitStatus::Added)
    {
        return Status;
    }
    FString SaveFeedback;
    if (!FEchoesCampaignProgressStore::SaveAtomic(
            CampaignProgressPath, Candidate, SaveFeedback))
    {
        OutFeedback = SaveFeedback;
        return EEchoesCampaignCommitStatus::StorageFailure;
    }
    CampaignProgress = MoveTemp(Candidate);
    RefreshCampaignBackupState();
    OutFeedback = SaveFeedback;
    return EEchoesCampaignCommitStatus::Added;
}

EEchoesCampaignCommitStatus
UEchoesSimulationSubsystem::CommitShapeOfSilenceCompletion(
    echoes::sim::FutureWellChoice& OutRecordedChoice,
    FString& OutFeedback)
{
    const FutureWellChoice Branch = GetRecordedPrologueChoice();
    OutRecordedChoice = Branch;
    if (!bCampaignProgressAvailable || !Simulation.IsValid())
    {
        OutFeedback = TEXT("[CAMPAIGN_LEDGER_UNAVAILABLE] Mission completion is valid, but campaign progress could not be saved.");
        return EEchoesCampaignCommitStatus::StorageFailure;
    }
    if (SelectedOperation !=
            EEchoesOperationMode::CampaignShapeOfSilence ||
        GetShapeOfSilencePhase() !=
            EEchoesShapeOfSilencePhase::Complete ||
        !IsShapeOfSilenceUnlocked() ||
        Branch == FutureWellChoice::Dormant)
    {
        OutFeedback = TEXT("[CAMPAIGN_COMPLETION_UNVERIFIED] No completed authoritative The Shape of Silence operation can be recorded.");
        return EEchoesCampaignCommitStatus::StorageFailure;
    }

    FEchoesCampaignDecisionRecord Record;
    Record.Mission = EEchoesCampaignMissionId::TheShapeOfSilence;
    Record.WellChoice = Branch;
    Record.AvailableWellChoices = WellChoiceMask(Branch);
    Record.VerifiedFacts =
        static_cast<uint8>(EEchoesShapeOfSilenceCompletionFact::WaystoneRootedAtListeningAnchor) |
        static_cast<uint8>(EEchoesShapeOfSilenceCompletionFact::ListeningSpineRaised) |
        static_cast<uint8>(EEchoesShapeOfSilenceCompletionFact::BothMemoryWitnessesPositioned) |
        static_cast<uint8>(EEchoesShapeOfSilenceCompletionFact::OruunReachedConfluence) |
        static_cast<uint8>(EEchoesShapeOfSilenceCompletionFact::LocalCoreSurvived) |
        static_cast<uint8>(EEchoesShapeOfSilenceCompletionFact::PriorLedgerConsumed);
    Record.SimulationSnapshotVersion = echoes::sim::kSnapshotVersion;
    Record.CompletionTick = Simulation->CurrentTick();
    Record.FinalStateChecksum = Simulation->StateChecksum();

    FEchoesCampaignProgress Candidate = CampaignProgress;
    const EEchoesCampaignCommitStatus Status =
        Candidate.AppendDecision(Record, OutFeedback);
    if (Status == EEchoesCampaignCommitStatus::StorageFailure)
    {
        return Status;
    }
    if (const FEchoesCampaignDecisionRecord* Existing =
            Candidate.FindDecision(Record.Mission))
    {
        OutRecordedChoice = Existing->WellChoice;
    }
    if (Status != EEchoesCampaignCommitStatus::Added)
    {
        return Status;
    }
    FString SaveFeedback;
    if (!FEchoesCampaignProgressStore::SaveAtomic(
            CampaignProgressPath, Candidate, SaveFeedback))
    {
        OutFeedback = SaveFeedback;
        return EEchoesCampaignCommitStatus::StorageFailure;
    }
    CampaignProgress = MoveTemp(Candidate);
    RefreshCampaignBackupState();
    OutFeedback = SaveFeedback;
    return EEchoesCampaignCommitStatus::Added;
}

void UEchoesSimulationSubsystem::AdvancePrologueCompletionPresentation()
{
    if (!bPrologueCompletionPresentationScenario ||
        PrologueCompletionPresentationStage < 0 ||
        !bScenarioReady || !Simulation.IsValid())
    {
        return;
    }

    FString Feedback;
    bool bCommandAccepted = false;
    const auto FailFixture = [this, &Feedback](const TCHAR* Stage)
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_PROLOGUE_COMPLETION_PRESENTATION_FAILED] stage=%s detail=%s"),
            Stage,
            Feedback.IsEmpty() ? TEXT("command rejected") : *Feedback);
        PrologueCompletionPresentationStage = -1;
        bSimulationPaused = true;
    };

    switch (PrologueCompletionPresentationStage)
    {
        case 0:
            bCommandAccepted = IssueCommand(
                echoes::sim::CommandType::Move,
                ArchiveCarrierId,
                0,
                SimToWorld(GetArchiveRecoverySite()),
                echoes::sim::FutureWellChoice::Dormant,
                Feedback);
            if (!bCommandAccepted)
            {
                FailFixture(TEXT("recover_archive"));
                return;
            }
            PrologueCompletionPresentationStage = 1;
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_PROLOGUE_COMPLETION_PRESENTATION_STAGE] stage=recover_archive command=ordinary_move"));
            return;

        case 1:
            if (GetProloguePhase() !=
                EEchoesProloguePhase::DecideFutureWell)
            {
                return;
            }
            bCommandAccepted = IssueCommand(
                echoes::sim::CommandType::Move,
                ProloguePresentationWorkerId,
                0,
                SimToWorld(Vec2::FromTiles(29, 29)),
                echoes::sim::FutureWellChoice::Dormant,
                Feedback);
            if (!bCommandAccepted)
            {
                FailFixture(TEXT("approach_well"));
                return;
            }
            PrologueCompletionPresentationStage = 2;
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_PROLOGUE_COMPLETION_PRESENTATION_STAGE] stage=decide_well command=ordinary_move protocol=Preserve"));
            return;

        case 2:
            if (!Simulation->IsEntityVisibleTo(
                    LocalPlayerId,
                    ProloguePresentationWellId))
            {
                return;
            }
            if (const echoes::sim::Entity* Well =
                    Simulation->FindEntity(ProloguePresentationWellId))
            {
                bCommandAccepted = IssueCommand(
                    echoes::sim::CommandType::FutureWell,
                    ProloguePresentationWorkerId,
                    ProloguePresentationWellId,
                    SimToWorld(Well->position),
                    echoes::sim::FutureWellChoice::Preserve,
                    Feedback);
            }
            if (!bCommandAccepted)
            {
                FailFixture(TEXT("commit_preserve"));
                return;
            }
            PrologueCompletionPresentationStage = 3;
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_PROLOGUE_COMPLETION_PRESENTATION_STAGE] stage=preserve command=ordinary_future_well"));
            return;

        case 3:
            if (GetProloguePhase() != EEchoesProloguePhase::Withdraw)
            {
                return;
            }
            bCommandAccepted = IssueCommand(
                echoes::sim::CommandType::Move,
                ArchiveCarrierId,
                0,
                SimToWorld(GetEvacuationSite()),
                echoes::sim::FutureWellChoice::Dormant,
                Feedback);
            if (!bCommandAccepted)
            {
                FailFixture(TEXT("withdraw"));
                return;
            }
            PrologueCompletionPresentationStage = 4;
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_PROLOGUE_COMPLETION_PRESENTATION_STAGE] stage=withdraw command=ordinary_move"));
            return;

        case 4:
            if (GetProloguePhase() == EEchoesProloguePhase::Complete &&
                bMatchResultReported)
            {
                PrologueCompletionPresentationStage = 5;
                UE_LOG(
                    LogEchoes,
                    Display,
                    TEXT("[ECHOES_PROLOGUE_COMPLETION_PRESENTATION_COMPLETE] phase=complete resultPresented=true ledgerRecords=%d controlled=true release=false"),
                    CampaignProgress.Decisions.Num());
            }
            return;

        default:
            return;
    }
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

    const FString SavePath = GetActiveQuickSavePath();
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

    const FString SavePath = GetActiveQuickSavePath();
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
        if (SelectedOperation ==
            EEchoesOperationMode::CampaignTermsOfContinuance)
        {
            const FEchoesTermsOfContinuancePlan Plan =
                GetTermsOfContinuancePlan();
            const echoes::sim::Entity* MeridianRelay =
                Candidate->FindEntity(MeridianContinuanceRelayId);
            const echoes::sim::Entity* KharuunSpine =
                Candidate->FindEntity(KharuunContinuanceSpineId);
            const echoes::sim::Entity* MeridianWitness =
                Candidate->FindEntity(MeridianContinuanceWitnessId);
            const echoes::sim::Entity* KharuunWitness =
                Candidate->FindEntity(KharuunContinuanceWitnessId);
            const auto IsMeridianProxy = [](const echoes::sim::Entity* Entity,
                                            echoes::sim::EntityType Type)
            {
                return Entity != nullptr &&
                       Entity->owner == LocalPlayerId &&
                       Entity->faction ==
                           echoes::sim::Faction::MeridianCompact &&
                       Entity->type == Type;
            };
            if (!IsMeridianProxy(
                    MeridianRelay,
                    echoes::sim::EntityType::UtilityStructure) ||
                !IsMeridianProxy(
                    KharuunSpine,
                    echoes::sim::EntityType::UtilityStructure) ||
                !IsMeridianProxy(
                    MeridianWitness,
                    echoes::sim::EntityType::ScoutUnit) ||
                !IsMeridianProxy(
                    KharuunWitness,
                    echoes::sim::EntityType::ScoutUnit) ||
                MeridianRelay->position != Plan.MeridianRelaySite ||
                KharuunSpine->position != Plan.KharuunSpineSite)
            {
                OutFailure = TEXT(
                    "snapshot does not match the active Terms of Continuance ledger branch");
                return false;
            }
        }
        if (SelectedOperation ==
            EEchoesOperationMode::CampaignNamesWithoutBirths)
        {
            const FEchoesNamesWithoutBirthsPlan Plan =
                GetNamesWithoutBirthsPlan();
            const echoes::sim::Entity* Talar =
                Candidate->FindEntity(TalarId);
            const echoes::sim::Entity* Archive =
                Candidate->FindEntity(CensusArchiveId);
            const echoes::sim::Entity* FirstCivilian =
                Candidate->FindEntity(FirstCivilianId);
            const echoes::sim::Entity* SecondCivilian =
                Candidate->FindEntity(SecondCivilianId);
            const auto IsMeridianProxy = [](const echoes::sim::Entity* Entity,
                                            EntityType Type)
            {
                return Entity != nullptr &&
                       Entity->owner == LocalPlayerId &&
                       Entity->faction == Faction::MeridianCompact &&
                       Entity->type == Type;
            };
            if (!IsMeridianProxy(Talar, EntityType::ScoutUnit) ||
                !IsMeridianProxy(Archive, EntityType::UtilityStructure) ||
                !IsMeridianProxy(FirstCivilian, EntityType::Worker) ||
                !IsMeridianProxy(SecondCivilian, EntityType::Worker) ||
                Archive->position != Plan.CensusSite)
            {
                OutFailure = TEXT(
                    "snapshot does not match the active Names Without Births ledger branch");
                return false;
            }
        }
        if (SelectedOperation ==
            EEchoesOperationMode::CampaignShapeOfSilence)
        {
            const echoes::sim::Entity* Waystone =
                Candidate->FindEntity(MigrationWaystoneId);
            const echoes::sim::Entity* Oruun =
                Candidate->FindEntity(OruunId);
            const echoes::sim::Entity* FirstWitness =
                Candidate->FindEntity(FirstMemoryWitnessId);
            const echoes::sim::Entity* SecondWitness =
                Candidate->FindEntity(SecondMemoryWitnessId);
            const auto IsKharuunProxy = [](const echoes::sim::Entity* Entity,
                                           EntityType Type)
            {
                return Entity != nullptr &&
                       Entity->owner == LocalPlayerId &&
                       Entity->faction == Faction::KharuunAssemblies &&
                       Entity->type == Type;
            };
            if (!IsKharuunProxy(Waystone, EntityType::Dropoff) ||
                !IsKharuunProxy(Oruun, EntityType::ScoutUnit) ||
                !IsKharuunProxy(FirstWitness, EntityType::ScoutUnit) ||
                !IsKharuunProxy(SecondWitness, EntityType::ScoutUnit) ||
                OruunId == FirstMemoryWitnessId ||
                OruunId == SecondMemoryWitnessId ||
                FirstMemoryWitnessId == SecondMemoryWitnessId)
            {
                OutFailure = TEXT(
                    "snapshot does not match the active The Shape of Silence ledger branch");
                return false;
            }
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

EEchoesProloguePhase UEchoesSimulationSubsystem::GetProloguePhase() const
{
    FEchoesPrologueMissionFacts Facts;
    Facts.bOperationActive =
        SelectedOperation == EEchoesOperationMode::CampaignPrologue &&
        bScenarioReady && Simulation.IsValid();
    if (!Facts.bOperationActive)
    {
        return EEchoesProloguePhase::Inactive;
    }

    const echoes::sim::Entity* Carrier = Simulation->FindEntity(ArchiveCarrierId);
    Facts.bArchiveCarrierIntact = Carrier != nullptr && Carrier->hitPoints > 0;
    if (Facts.bArchiveCarrierIntact)
    {
        Facts.bArchiveCarrierAtRecoverySite = IsWithinTiles(
            Carrier->position,
            GetArchiveRecoverySite(),
            PrologueSiteRadiusTiles);
        Facts.bArchiveCarrierAtEvacuationSite = IsWithinTiles(
            Carrier->position,
            GetEvacuationSite(),
            PrologueSiteRadiusTiles);
    }
    Facts.bSkirmishStillOngoing =
        Simulation->Outcome() == echoes::sim::MatchOutcome::Ongoing;
    for (const echoes::sim::Entity& Entity : Simulation->Entities())
    {
        if (Entity.owner == LocalPlayerId &&
            Entity.type == echoes::sim::EntityType::CommandCore &&
            Entity.hitPoints > 0)
        {
            Facts.bLocalCoreIntact = true;
        }
        if (Entity.type == echoes::sim::EntityType::FutureWell &&
            Entity.wellChoice != echoes::sim::FutureWellChoice::Dormant)
        {
            Facts.bFutureWellProtocolChosen = Entity.owner == LocalPlayerId;
            Facts.bFutureWellLost = Entity.owner != LocalPlayerId;
        }
    }
    return FEchoesPrologueMissionModel::DeterminePhase(Facts);
}

bool UEchoesSimulationSubsystem::IsSevenAccountsUnlocked() const
{
    return bCampaignProgressAvailable &&
           CampaignProgress.FindDecision(
               EEchoesCampaignMissionId::WhatTheLedgerKeeps) != nullptr;
}

bool UEchoesSimulationSubsystem::IsCityReserveUnlocked() const
{
    if (!bCampaignProgressAvailable)
    {
        return false;
    }
    const FEchoesCampaignDecisionRecord* Prologue =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::WhatTheLedgerKeeps);
    const FEchoesCampaignDecisionRecord* SevenAccounts =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::SevenAccountsOfRain);
    return Prologue != nullptr && SevenAccounts != nullptr &&
           Prologue->WellChoice == SevenAccounts->WellChoice;
}

bool UEchoesSimulationSubsystem::IsUnburiedRoadUnlocked() const
{
    if (!IsCityReserveUnlocked())
    {
        return false;
    }
    const FEchoesCampaignDecisionRecord* Prologue =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::WhatTheLedgerKeeps);
    const FEchoesCampaignDecisionRecord* CityReserve =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::ACityOnReserve);
    return Prologue != nullptr && CityReserve != nullptr &&
           Prologue->WellChoice == CityReserve->WellChoice;
}

bool UEchoesSimulationSubsystem::IsTermsOfContinuanceUnlocked() const
{
    if (!IsUnburiedRoadUnlocked())
    {
        return false;
    }
    const FEchoesCampaignDecisionRecord* Prologue =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::WhatTheLedgerKeeps);
    const FEchoesCampaignDecisionRecord* UnburiedRoad =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::TheUnburiedRoad);
    return Prologue != nullptr && UnburiedRoad != nullptr &&
           Prologue->WellChoice == UnburiedRoad->WellChoice;
}

bool UEchoesSimulationSubsystem::IsNamesWithoutBirthsUnlocked() const
{
    if (!IsTermsOfContinuanceUnlocked())
    {
        return false;
    }
    const FEchoesCampaignDecisionRecord* Prologue =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::WhatTheLedgerKeeps);
    const FEchoesCampaignDecisionRecord* Continuance =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::TermsOfContinuance);
    return Prologue != nullptr && Continuance != nullptr &&
           Prologue->WellChoice == Continuance->WellChoice;
}

bool UEchoesSimulationSubsystem::IsShapeOfSilenceUnlocked() const
{
    if (!IsNamesWithoutBirthsUnlocked())
    {
        return false;
    }
    const FEchoesCampaignDecisionRecord* Prologue =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::WhatTheLedgerKeeps);
    const FEchoesCampaignDecisionRecord* Names =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::NamesWithoutBirths);
    return Prologue != nullptr && Names != nullptr &&
           Prologue->WellChoice == Names->WellChoice;
}

FutureWellChoice UEchoesSimulationSubsystem::GetRecordedPrologueChoice() const
{
    const FEchoesCampaignDecisionRecord* Record =
        CampaignProgress.FindDecision(
            EEchoesCampaignMissionId::WhatTheLedgerKeeps);
    return Record != nullptr ? Record->WellChoice : FutureWellChoice::Dormant;
}

FEchoesSevenAccountsRoute
UEchoesSimulationSubsystem::GetSevenAccountsRoute() const
{
    return FEchoesSevenAccountsMissionModel::RouteForChoice(
        GetRecordedPrologueChoice());
}

FEchoesCityReserveGrid UEchoesSimulationSubsystem::GetCityReserveGrid() const
{
    return FEchoesCityReserveMissionModel::GridForChoice(
        GetRecordedPrologueChoice());
}

FEchoesUnburiedRoadRoute
UEchoesSimulationSubsystem::GetUnburiedRoadRoute() const
{
    return FEchoesUnburiedRoadMissionModel::RouteForChoice(
        GetRecordedPrologueChoice());
}

FEchoesTermsOfContinuancePlan
UEchoesSimulationSubsystem::GetTermsOfContinuancePlan() const
{
    return FEchoesTermsOfContinuanceMissionModel::PlanForChoice(
        GetRecordedPrologueChoice());
}

FEchoesNamesWithoutBirthsPlan
UEchoesSimulationSubsystem::GetNamesWithoutBirthsPlan() const
{
    return FEchoesNamesWithoutBirthsMissionModel::PlanForChoice(
        GetRecordedPrologueChoice());
}

FEchoesShapeOfSilencePlan
UEchoesSimulationSubsystem::GetShapeOfSilencePlan() const
{
    return FEchoesShapeOfSilenceMissionModel::PlanForChoice(
        GetRecordedPrologueChoice());
}

echoes::sim::EntityId UEchoesSimulationSubsystem::GetCityDistrictId(
    EEchoesCityDistrict District) const
{
    switch (District)
    {
        case EEchoesCityDistrict::LifeSupport:
            return LifeSupportDistrictId;
        case EEchoesCityDistrict::Transit:
            return TransitDistrictId;
        case EEchoesCityDistrict::Archive:
            return ArchiveDistrictId;
    }
    return 0;
}

EEchoesSevenAccountsPhase
UEchoesSimulationSubsystem::GetSevenAccountsPhase() const
{
    FEchoesSevenAccountsMissionFacts Facts;
    Facts.bOperationActive =
        SelectedOperation == EEchoesOperationMode::CampaignSevenAccounts &&
        bScenarioReady && Simulation.IsValid();
    if (!Facts.bOperationActive)
    {
        return EEchoesSevenAccountsPhase::Inactive;
    }

    const FEchoesSevenAccountsRoute Route = GetSevenAccountsRoute();
    const echoes::sim::Entity* Bearer =
        Simulation->FindEntity(MemoryBearerId);
    const echoes::sim::Entity* Waystone =
        Simulation->FindEntity(MigrationWaystoneId);
    Facts.bMemoryBearerIntact = Bearer != nullptr && Bearer->hitPoints > 0;
    Facts.bWaystoneIntact = Waystone != nullptr && Waystone->hitPoints > 0;
    Facts.bMemoryBearerAtAccountSite =
        Facts.bMemoryBearerIntact &&
        IsWithinTiles(
            Bearer->position,
            Route.MemoryAccountSite,
            SevenAccountsSiteRadiusTiles);
    Facts.bWaystoneRootedAtAnchor =
        Facts.bWaystoneIntact &&
        Waystone->waystoneMode == echoes::sim::WaystoneMode::Rooted &&
        IsWithinTiles(
            Waystone->position,
            Route.WaystoneAnchor,
            SevenAccountsSiteRadiusTiles);
    Facts.bSkirmishStillOngoing =
        Simulation->Outcome() == echoes::sim::MatchOutcome::Ongoing;
    for (const echoes::sim::Entity& Entity : Simulation->Entities())
    {
        if (Entity.owner == LocalPlayerId &&
            Entity.type == EntityType::CommandCore && Entity.hitPoints > 0)
        {
            Facts.bLocalCoreIntact = true;
            break;
        }
    }
    return FEchoesSevenAccountsMissionModel::DeterminePhase(Facts);
}

EEchoesCityReservePhase UEchoesSimulationSubsystem::GetCityReservePhase() const
{
    FEchoesCityReserveMissionFacts Facts;
    Facts.bOperationActive =
        SelectedOperation == EEchoesOperationMode::CampaignCityReserve &&
        bScenarioReady && Simulation.IsValid();
    if (!Facts.bOperationActive)
    {
        return EEchoesCityReservePhase::Inactive;
    }

    const echoes::sim::Entity* LifeSupport =
        Simulation->FindEntity(LifeSupportDistrictId);
    const echoes::sim::Entity* Transit =
        Simulation->FindEntity(TransitDistrictId);
    const echoes::sim::Entity* Archive =
        Simulation->FindEntity(ArchiveDistrictId);
    Facts.bLifeSupportIntact =
        LifeSupport != nullptr && LifeSupport->hitPoints > 0;
    Facts.bTransitIntact = Transit != nullptr && Transit->hitPoints > 0;
    Facts.bArchiveIntact = Archive != nullptr && Archive->hitPoints > 0;
    Facts.bLifeSupportPowered =
        Facts.bLifeSupportIntact && LifeSupport->aegisPowered;
    Facts.bTransitPowered =
        Facts.bTransitIntact && Transit->aegisPowered;
    Facts.bArchivePowered =
        Facts.bArchiveIntact && Archive->aegisPowered;
    Facts.bSkirmishStillOngoing =
        Simulation->Outcome() == echoes::sim::MatchOutcome::Ongoing;
    for (const echoes::sim::Entity& Entity : Simulation->Entities())
    {
        if (Entity.owner == LocalPlayerId &&
            Entity.type == EntityType::CommandCore && Entity.hitPoints > 0)
        {
            Facts.bLocalCoreIntact = true;
            break;
        }
    }
    return FEchoesCityReserveMissionModel::DeterminePhase(
        Facts,
        GetCityReserveGrid());
}

EEchoesUnburiedRoadPhase
UEchoesSimulationSubsystem::GetUnburiedRoadPhase() const
{
    FEchoesUnburiedRoadMissionFacts Facts;
    Facts.bOperationActive =
        SelectedOperation == EEchoesOperationMode::CampaignUnburiedRoad &&
        bScenarioReady && Simulation.IsValid();
    if (!Facts.bOperationActive)
    {
        return EEchoesUnburiedRoadPhase::Inactive;
    }

    const FEchoesUnburiedRoadRoute Route = GetUnburiedRoadRoute();
    const echoes::sim::Entity* Bearer =
        Simulation->FindEntity(MemoryBearerId);
    const echoes::sim::Entity* Waystone =
        Simulation->FindEntity(MigrationWaystoneId);
    Facts.bMemoryBearerIntact = Bearer != nullptr && Bearer->hitPoints > 0;
    Facts.bWaystoneIntact = Waystone != nullptr && Waystone->hitPoints > 0;
    Facts.bMemoryBearerAtShard =
        Facts.bMemoryBearerIntact &&
        IsWithinTiles(
            Bearer->position,
            Route.MemoryShardSite,
            UnburiedRoadSiteRadiusTiles);
    Facts.bWaystoneRootedAtRoadhead =
        Facts.bWaystoneIntact &&
        Waystone->waystoneMode == echoes::sim::WaystoneMode::Rooted &&
        IsWithinTiles(
            Waystone->position,
            Route.Roadhead,
            UnburiedRoadSiteRadiusTiles);
    Facts.bSkirmishStillOngoing =
        Simulation->Outcome() == echoes::sim::MatchOutcome::Ongoing;
    for (const echoes::sim::Entity& Entity : Simulation->Entities())
    {
        if (Entity.owner == LocalPlayerId && Entity.hitPoints > 0 &&
            Entity.type == EntityType::CommandCore)
        {
            Facts.bLocalCoreIntact = true;
        }
        if (Entity.owner == LocalPlayerId && Entity.hitPoints > 0 &&
            Entity.completed && Entity.type == EntityType::UtilityStructure &&
            IsWithinTiles(
                Entity.position,
                Route.ListeningSpineSite,
                UnburiedRoadSiteRadiusTiles))
        {
            Facts.bListeningSpineComplete = true;
        }
    }
    return FEchoesUnburiedRoadMissionModel::DeterminePhase(Facts);
}

EEchoesTermsOfContinuancePhase
UEchoesSimulationSubsystem::GetTermsOfContinuancePhase() const
{
    FEchoesTermsOfContinuanceMissionFacts Facts;
    Facts.bOperationActive =
        SelectedOperation ==
            EEchoesOperationMode::CampaignTermsOfContinuance &&
        bScenarioReady && Simulation.IsValid();
    if (!Facts.bOperationActive)
    {
        return EEchoesTermsOfContinuancePhase::Inactive;
    }

    const FEchoesTermsOfContinuancePlan Plan =
        GetTermsOfContinuancePlan();
    const echoes::sim::Entity* MeridianRelay =
        Simulation->FindEntity(MeridianContinuanceRelayId);
    const echoes::sim::Entity* KharuunSpine =
        Simulation->FindEntity(KharuunContinuanceSpineId);
    const echoes::sim::Entity* MeridianWitness =
        Simulation->FindEntity(MeridianContinuanceWitnessId);
    const echoes::sim::Entity* KharuunWitness =
        Simulation->FindEntity(KharuunContinuanceWitnessId);
    Facts.bMeridianRelayIntact =
        MeridianRelay != nullptr && MeridianRelay->hitPoints > 0;
    Facts.bKharuunSpineIntact =
        KharuunSpine != nullptr && KharuunSpine->hitPoints > 0;
    Facts.bMeridianWitnessIntact =
        MeridianWitness != nullptr && MeridianWitness->hitPoints > 0;
    Facts.bKharuunWitnessIntact =
        KharuunWitness != nullptr && KharuunWitness->hitPoints > 0;
    Facts.bMeridianRelaySynchronized =
        Facts.bMeridianRelayIntact && MeridianRelay->aegisPowered;
    Facts.bKharuunSpineSynchronized =
        Facts.bKharuunSpineIntact && KharuunSpine->aegisPowered;
    const uint64 CurrentTick = Simulation->CurrentTick();
    Facts.bContinuanceWindowHeld =
        CurrentTick >= Plan.ContinuanceWindowEndTick;
    Facts.bContinuanceWindowCompromised =
        CurrentTick >= Plan.ContinuanceWindowStartTick &&
        (!Facts.bMeridianRelaySynchronized ||
         !Facts.bKharuunSpineSynchronized);
    const bool bMeridianWitnessAtExtraction =
        Facts.bMeridianWitnessIntact &&
        IsWithinTiles(
            MeridianWitness->position,
            Plan.WitnessExtractionSite,
            TermsOfContinuanceSiteRadiusTiles);
    const bool bKharuunWitnessAtExtraction =
        Facts.bKharuunWitnessIntact &&
        IsWithinTiles(
            KharuunWitness->position,
            Plan.WitnessExtractionSite,
            TermsOfContinuanceSiteRadiusTiles);
    const auto IsEarlyExtractionOrder = [&Plan](
                                            const echoes::sim::Entity* Witness)
    {
        return Witness != nullptr &&
               Witness->order.type == echoes::sim::OrderType::Move &&
               IsWithinTiles(
                   Witness->order.destination,
                   Plan.WitnessExtractionSite,
                   TermsOfContinuanceSiteRadiusTiles);
    };
    Facts.bWitnessExtractionStartedEarly =
        CurrentTick < Plan.ContinuanceWindowEndTick &&
        (bMeridianWitnessAtExtraction || bKharuunWitnessAtExtraction ||
         IsEarlyExtractionOrder(MeridianWitness) ||
         IsEarlyExtractionOrder(KharuunWitness));
    Facts.bMeridianWitnessExtracted =
        Facts.bContinuanceWindowHeld && bMeridianWitnessAtExtraction;
    Facts.bKharuunWitnessExtracted =
        Facts.bContinuanceWindowHeld && bKharuunWitnessAtExtraction;
    Facts.bSkirmishStillOngoing =
        Simulation->Outcome() == echoes::sim::MatchOutcome::Ongoing;
    for (const echoes::sim::Entity& Entity : Simulation->Entities())
    {
        if (Entity.owner == LocalPlayerId && Entity.hitPoints > 0 &&
            Entity.type == EntityType::CommandCore)
        {
            Facts.bLocalCoreIntact = true;
            break;
        }
    }
    return FEchoesTermsOfContinuanceMissionModel::DeterminePhase(Facts);
}

EEchoesNamesWithoutBirthsPhase
UEchoesSimulationSubsystem::GetNamesWithoutBirthsPhase() const
{
    FEchoesNamesWithoutBirthsMissionFacts Facts;
    Facts.bOperationActive =
        SelectedOperation ==
            EEchoesOperationMode::CampaignNamesWithoutBirths &&
        bScenarioReady && Simulation.IsValid();
    if (!Facts.bOperationActive)
    {
        return EEchoesNamesWithoutBirthsPhase::Inactive;
    }

    const FEchoesNamesWithoutBirthsPlan Plan =
        GetNamesWithoutBirthsPlan();
    const echoes::sim::Entity* Talar = Simulation->FindEntity(TalarId);
    const echoes::sim::Entity* Archive =
        Simulation->FindEntity(CensusArchiveId);
    const echoes::sim::Entity* FirstCivilian =
        Simulation->FindEntity(FirstCivilianId);
    const echoes::sim::Entity* SecondCivilian =
        Simulation->FindEntity(SecondCivilianId);
    Facts.bTalarIntact = Talar != nullptr && Talar->hitPoints > 0;
    Facts.bArchiveIntact = Archive != nullptr && Archive->hitPoints > 0;
    Facts.bFirstCivilianIntact =
        FirstCivilian != nullptr && FirstCivilian->hitPoints > 0;
    Facts.bSecondCivilianIntact =
        SecondCivilian != nullptr && SecondCivilian->hitPoints > 0;
    Facts.bArchivePowered =
        Facts.bArchiveIntact && Archive->aegisPowered;
    Facts.bCensusEvidenceLocated =
        Facts.bArchivePowered ||
        (Facts.bTalarIntact &&
         IsWithinTiles(
             Talar->position,
             Plan.CensusSite,
             NamesWithoutBirthsSiteRadiusTiles));
    Facts.bFirstCivilianSheltered =
        Facts.bFirstCivilianIntact &&
        IsWithinTiles(
            FirstCivilian->position,
            Plan.CivilianShelterSite,
            NamesWithoutBirthsSiteRadiusTiles);
    Facts.bSecondCivilianSheltered =
        Facts.bSecondCivilianIntact &&
        IsWithinTiles(
            SecondCivilian->position,
            Plan.CivilianShelterSite,
            NamesWithoutBirthsSiteRadiusTiles);
    Facts.bTalarAtEvidenceExtraction =
        Facts.bTalarIntact && Facts.bArchivePowered &&
        Facts.bFirstCivilianSheltered && Facts.bSecondCivilianSheltered &&
        IsWithinTiles(
            Talar->position,
            Plan.EvidenceExtractionSite,
            NamesWithoutBirthsSiteRadiusTiles);
    Facts.bSkirmishStillOngoing =
        Simulation->Outcome() == echoes::sim::MatchOutcome::Ongoing;
    for (const echoes::sim::Entity& Entity : Simulation->Entities())
    {
        if (Entity.owner == LocalPlayerId && Entity.hitPoints > 0 &&
            Entity.type == EntityType::CommandCore)
        {
            Facts.bLocalCoreIntact = true;
            break;
        }
    }
    return FEchoesNamesWithoutBirthsMissionModel::DeterminePhase(Facts);
}

EEchoesShapeOfSilencePhase
UEchoesSimulationSubsystem::GetShapeOfSilencePhase() const
{
    FEchoesShapeOfSilenceMissionFacts Facts;
    Facts.bOperationActive =
        SelectedOperation ==
            EEchoesOperationMode::CampaignShapeOfSilence &&
        bScenarioReady && Simulation.IsValid();
    if (!Facts.bOperationActive)
    {
        return EEchoesShapeOfSilencePhase::Inactive;
    }

    const FEchoesShapeOfSilencePlan Plan = GetShapeOfSilencePlan();
    const echoes::sim::Entity* Waystone =
        Simulation->FindEntity(MigrationWaystoneId);
    const echoes::sim::Entity* Oruun = Simulation->FindEntity(OruunId);
    const echoes::sim::Entity* FirstWitness =
        Simulation->FindEntity(FirstMemoryWitnessId);
    const echoes::sim::Entity* SecondWitness =
        Simulation->FindEntity(SecondMemoryWitnessId);
    Facts.bWaystoneIntact = Waystone != nullptr && Waystone->hitPoints > 0;
    Facts.bOruunIntact = Oruun != nullptr && Oruun->hitPoints > 0;
    Facts.bFirstWitnessIntact =
        FirstWitness != nullptr && FirstWitness->hitPoints > 0;
    Facts.bSecondWitnessIntact =
        SecondWitness != nullptr && SecondWitness->hitPoints > 0;
    Facts.bWaystoneRootedAtAnchor =
        Facts.bWaystoneIntact &&
        Waystone->waystoneMode == echoes::sim::WaystoneMode::Rooted &&
        IsWithinTiles(
            Waystone->position,
            Plan.WaystoneAnchor,
            ShapeOfSilenceSiteRadiusTiles);
    for (const echoes::sim::Entity& Entity : Simulation->Entities())
    {
        if (Entity.owner == LocalPlayerId && Entity.hitPoints > 0 &&
            Entity.type == EntityType::CommandCore)
        {
            Facts.bLocalCoreIntact = true;
        }
        if (Entity.owner == LocalPlayerId && Entity.hitPoints > 0 &&
            Entity.completed && Entity.type == EntityType::UtilityStructure &&
            IsWithinTiles(
                Entity.position,
                Plan.ListeningSpineSite,
                ShapeOfSilenceSiteRadiusTiles))
        {
            Facts.bListeningSpineRaised = true;
        }
    }
    Facts.bFirstWitnessPositioned =
        Facts.bFirstWitnessIntact &&
        IsWithinTiles(
            FirstWitness->position,
            Plan.FirstWitnessSite,
            ShapeOfSilenceSiteRadiusTiles);
    Facts.bSecondWitnessPositioned =
        Facts.bSecondWitnessIntact &&
        IsWithinTiles(
            SecondWitness->position,
            Plan.SecondWitnessSite,
            ShapeOfSilenceSiteRadiusTiles);
    Facts.bOruunAtConfluence =
        Facts.bOruunIntact && Facts.bFirstWitnessPositioned &&
        Facts.bSecondWitnessPositioned &&
        IsWithinTiles(
            Oruun->position,
            Plan.ConfluenceSite,
            ShapeOfSilenceSiteRadiusTiles);
    Facts.bSkirmishStillOngoing =
        Simulation->Outcome() == echoes::sim::MatchOutcome::Ongoing;
    return FEchoesShapeOfSilenceMissionModel::DeterminePhase(Facts);
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
    Snapshot.OperationMode = SelectedOperation;
    Snapshot.ProloguePhase = GetProloguePhase();
    Snapshot.SevenAccountsPhase = GetSevenAccountsPhase();
    Snapshot.SevenAccountsBranch = GetRecordedPrologueChoice();
    Snapshot.CityReservePhase = GetCityReservePhase();
    Snapshot.CityReserveBranch = GetRecordedPrologueChoice();
    Snapshot.UnburiedRoadPhase = GetUnburiedRoadPhase();
    Snapshot.UnburiedRoadBranch = GetRecordedPrologueChoice();
    Snapshot.TermsOfContinuancePhase = GetTermsOfContinuancePhase();
    Snapshot.TermsOfContinuanceBranch = GetRecordedPrologueChoice();
    Snapshot.NamesWithoutBirthsPhase = GetNamesWithoutBirthsPhase();
    Snapshot.NamesWithoutBirthsBranch = GetRecordedPrologueChoice();
    Snapshot.ShapeOfSilencePhase = GetShapeOfSilencePhase();
    Snapshot.ShapeOfSilenceBranch = GetRecordedPrologueChoice();
    Snapshot.ArchiveCarrierId = ArchiveCarrierId;
    Snapshot.MemoryBearerId = MemoryBearerId;
    Snapshot.MigrationWaystoneId = MigrationWaystoneId;
    Snapshot.LifeSupportDistrictId = LifeSupportDistrictId;
    Snapshot.TransitDistrictId = TransitDistrictId;
    Snapshot.ArchiveDistrictId = ArchiveDistrictId;
    Snapshot.MeridianContinuanceRelayId = MeridianContinuanceRelayId;
    Snapshot.KharuunContinuanceSpineId = KharuunContinuanceSpineId;
    Snapshot.MeridianContinuanceWitnessId = MeridianContinuanceWitnessId;
    Snapshot.KharuunContinuanceWitnessId = KharuunContinuanceWitnessId;
    Snapshot.TalarId = TalarId;
    Snapshot.CensusArchiveId = CensusArchiveId;
    Snapshot.FirstCivilianId = FirstCivilianId;
    Snapshot.SecondCivilianId = SecondCivilianId;
    Snapshot.OruunId = OruunId;
    Snapshot.FirstMemoryWitnessId = FirstMemoryWitnessId;
    Snapshot.SecondMemoryWitnessId = SecondMemoryWitnessId;
    const FEchoesSevenAccountsRoute SevenAccountsRoute =
        GetSevenAccountsRoute();
    const FEchoesUnburiedRoadRoute UnburiedRoadRoute =
        GetUnburiedRoadRoute();
    const FEchoesTermsOfContinuancePlan ContinuancePlan =
        GetTermsOfContinuancePlan();
    const FEchoesNamesWithoutBirthsPlan NamesPlan =
        GetNamesWithoutBirthsPlan();
    const FEchoesShapeOfSilencePlan ShapePlan =
        GetShapeOfSilencePlan();
    for (const echoes::sim::Entity& Entity : Simulation->Entities())
    {
        if (Entity.id == ArchiveCarrierId)
        {
            Snapshot.bArchiveCarrierIntact = Entity.hitPoints > 0;
            Snapshot.ArchiveCarrierHitPoints = Entity.hitPoints;
        }
        if (Entity.id == MemoryBearerId)
        {
            Snapshot.bMemoryBearerIntact = Entity.hitPoints > 0;
            Snapshot.bMemoryBearerAtAccountSite =
                Snapshot.bMemoryBearerIntact &&
                IsWithinTiles(
                    Entity.position,
                    SevenAccountsRoute.MemoryAccountSite,
                    SevenAccountsSiteRadiusTiles);
            Snapshot.bMemoryBearerAtShard =
                Snapshot.bMemoryBearerIntact &&
                IsWithinTiles(
                    Entity.position,
                    UnburiedRoadRoute.MemoryShardSite,
                    UnburiedRoadSiteRadiusTiles);
        }
        if (Entity.id == MigrationWaystoneId)
        {
            Snapshot.bWaystoneIntact = Entity.hitPoints > 0;
            Snapshot.bWaystoneRootedAtAnchor =
                Snapshot.bWaystoneIntact &&
                Entity.waystoneMode == echoes::sim::WaystoneMode::Rooted &&
                IsWithinTiles(
                    Entity.position,
                    SevenAccountsRoute.WaystoneAnchor,
                    SevenAccountsSiteRadiusTiles);
            Snapshot.bWaystoneRootedAtRoadhead =
                Snapshot.bWaystoneIntact &&
                Entity.waystoneMode == echoes::sim::WaystoneMode::Rooted &&
                IsWithinTiles(
                    Entity.position,
                    UnburiedRoadRoute.Roadhead,
                    UnburiedRoadSiteRadiusTiles);
            Snapshot.bShapeWaystoneRooted =
                Snapshot.bWaystoneIntact &&
                Entity.waystoneMode == echoes::sim::WaystoneMode::Rooted &&
                IsWithinTiles(
                    Entity.position,
                    ShapePlan.WaystoneAnchor,
                    ShapeOfSilenceSiteRadiusTiles);
        }
        if (Entity.owner == LocalPlayerId && Entity.hitPoints > 0 &&
            Entity.completed && Entity.type == EntityType::UtilityStructure &&
            IsWithinTiles(
                Entity.position,
                UnburiedRoadRoute.ListeningSpineSite,
                UnburiedRoadSiteRadiusTiles))
        {
            Snapshot.bListeningSpineComplete = true;
        }
        if (Entity.owner == LocalPlayerId && Entity.hitPoints > 0 &&
            Entity.completed && Entity.type == EntityType::UtilityStructure &&
            IsWithinTiles(
                Entity.position,
                ShapePlan.ListeningSpineSite,
                ShapeOfSilenceSiteRadiusTiles))
        {
            Snapshot.bShapeListeningSpineRaised = true;
        }
        if (Entity.id == LifeSupportDistrictId)
        {
            Snapshot.bLifeSupportPowered =
                Entity.hitPoints > 0 && Entity.aegisPowered;
        }
        if (Entity.id == TransitDistrictId)
        {
            Snapshot.bTransitPowered =
                Entity.hitPoints > 0 && Entity.aegisPowered;
        }
        if (Entity.id == ArchiveDistrictId)
        {
            Snapshot.bArchivePowered =
                Entity.hitPoints > 0 && Entity.aegisPowered;
        }
        if (Entity.id == MeridianContinuanceRelayId)
        {
            Snapshot.bMeridianRelaySynchronized =
                Entity.hitPoints > 0 && Entity.aegisPowered;
        }
        if (Entity.id == KharuunContinuanceSpineId)
        {
            Snapshot.bKharuunSpineSynchronized =
                Entity.hitPoints > 0 && Entity.aegisPowered;
        }
        if (Entity.id == MeridianContinuanceWitnessId)
        {
            Snapshot.bMeridianWitnessExtracted =
                Entity.hitPoints > 0 &&
                IsWithinTiles(
                    Entity.position,
                    ContinuancePlan.WitnessExtractionSite,
                    TermsOfContinuanceSiteRadiusTiles);
        }
        if (Entity.id == KharuunContinuanceWitnessId)
        {
            Snapshot.bKharuunWitnessExtracted =
                Entity.hitPoints > 0 &&
                IsWithinTiles(
                    Entity.position,
                    ContinuancePlan.WitnessExtractionSite,
                    TermsOfContinuanceSiteRadiusTiles);
        }
        if (Entity.id == CensusArchiveId)
        {
            Snapshot.bCensusArchivePowered =
                Entity.hitPoints > 0 && Entity.aegisPowered;
        }
        if (Entity.id == TalarId)
        {
            Snapshot.bCensusEvidenceLocated =
                Entity.hitPoints > 0 &&
                (Snapshot.bCensusArchivePowered ||
                 IsWithinTiles(
                     Entity.position,
                     NamesPlan.CensusSite,
                     NamesWithoutBirthsSiteRadiusTiles));
        }
        if (Entity.id == FirstCivilianId)
        {
            Snapshot.bFirstCivilianSheltered =
                Entity.hitPoints > 0 &&
                IsWithinTiles(
                    Entity.position,
                    NamesPlan.CivilianShelterSite,
                    NamesWithoutBirthsSiteRadiusTiles);
        }
        if (Entity.id == SecondCivilianId)
        {
            Snapshot.bSecondCivilianSheltered =
                Entity.hitPoints > 0 &&
                IsWithinTiles(
                    Entity.position,
                    NamesPlan.CivilianShelterSite,
                    NamesWithoutBirthsSiteRadiusTiles);
        }
        if (Entity.id == FirstMemoryWitnessId)
        {
            Snapshot.bFirstMemoryWitnessPositioned =
                Entity.hitPoints > 0 &&
                IsWithinTiles(
                    Entity.position,
                    ShapePlan.FirstWitnessSite,
                    ShapeOfSilenceSiteRadiusTiles);
        }
        if (Entity.id == SecondMemoryWitnessId)
        {
            Snapshot.bSecondMemoryWitnessPositioned =
                Entity.hitPoints > 0 &&
                IsWithinTiles(
                    Entity.position,
                    ShapePlan.SecondWitnessSite,
                    ShapeOfSilenceSiteRadiusTiles);
        }
        if (Entity.owner == LocalPlayerId &&
            Entity.type == echoes::sim::EntityType::CommandCore)
        {
            Snapshot.bLocalCoreIntact = true;
            Snapshot.LocalCoreHitPoints = Entity.hitPoints;
            Snapshot.LocalCoreMaxHitPoints = Entity.maxHitPoints;
            continue;
        }
        if (SelectedOperation == EEchoesOperationMode::CampaignPrologue &&
            Entity.type == echoes::sim::EntityType::FutureWell &&
            (Entity.wellChoice == echoes::sim::FutureWellChoice::Dormant ||
             Entity.owner == LocalPlayerId))
        {
            Snapshot.PrologueWellChoice = Entity.wellChoice;
        }

        const bool bVisible =
            Simulation->IsEntityVisibleTo(LocalPlayerId, Entity.id);
        if (!bVisible)
        {
            continue;
        }
        if (Entity.type == echoes::sim::EntityType::FutureWell)
        {
            Snapshot.PrologueWellChoice = Entity.wellChoice;
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
    Snapshot.bContinuanceWindowHeld =
        Simulation->CurrentTick() >=
        ContinuancePlan.ContinuanceWindowEndTick;
    const echoes::sim::Entity* Talar = Simulation->FindEntity(TalarId);
    Snapshot.bTalarAtEvidenceExtraction =
        Talar != nullptr && Talar->hitPoints > 0 &&
        Snapshot.bCensusArchivePowered &&
        Snapshot.bFirstCivilianSheltered &&
        Snapshot.bSecondCivilianSheltered &&
        IsWithinTiles(
            Talar->position,
            NamesPlan.EvidenceExtractionSite,
            NamesWithoutBirthsSiteRadiusTiles);
    const echoes::sim::Entity* Oruun = Simulation->FindEntity(OruunId);
    Snapshot.bOruunAtConfluence =
        Oruun != nullptr && Oruun->hitPoints > 0 &&
        Snapshot.bFirstMemoryWitnessPositioned &&
        Snapshot.bSecondMemoryWitnessPositioned &&
        IsWithinTiles(
            Oruun->position,
            ShapePlan.ConfluenceSite,
            ShapeOfSilenceSiteRadiusTiles);
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
            const EEchoesProloguePhase ProloguePhase = GetProloguePhase();
            const bool bPrologueFinished =
                ProloguePhase == EEchoesProloguePhase::Complete ||
                ProloguePhase == EEchoesProloguePhase::Failed;
            if (SelectedOperation == EEchoesOperationMode::CampaignPrologue &&
                bPrologueFinished && !bMatchResultReported)
            {
                bMatchResultReported = true;
                bSimulationPaused = true;
                FutureWellChoice Consequence = FutureWellChoice::Dormant;
                for (const echoes::sim::Entity& Entity : Simulation->Entities())
                {
                    if (Entity.type == EntityType::FutureWell)
                    {
                        Consequence = Entity.wellChoice;
                        break;
                    }
                }
                FutureWellChoice RecordedConsequence = Consequence;
                FString CampaignFeedback;
                const EEchoesCampaignCommitStatus CampaignStatus =
                    ProloguePhase == EEchoesProloguePhase::Complete
                        ? CommitPrologueCompletion(
                              Consequence,
                              RecordedConsequence,
                              CampaignFeedback)
                        : EEchoesCampaignCommitStatus::NotApplicable;
                if (AEchoesPlayerController* Controller =
                        Cast<AEchoesPlayerController>(
                            GetWorld()->GetFirstPlayerController()))
                {
                    Controller->NotifyCampaignPrologueFinished(
                        ProloguePhase == EEchoesProloguePhase::Complete,
                        Consequence,
                        RecordedConsequence,
                        CampaignStatus);
                }
                const TCHAR* ResultName =
                    ProloguePhase == EEchoesProloguePhase::Complete
                        ? TEXT("success")
                        : TEXT("failure");
                const TCHAR* CampaignDetail = CampaignFeedback.IsEmpty()
                    ? TEXT("not-applicable")
                    : *CampaignFeedback;
                if (CampaignStatus ==
                    EEchoesCampaignCommitStatus::StorageFailure)
                {
                    UE_LOG(
                        LogEchoes,
                        Error,
                        TEXT("[ECHOES_PROLOGUE_FINISHED] result=%s phase=%s consequence=%u recordedConsequence=%u campaignStatus=%u tick=%llu detail=%s"),
                        ResultName,
                        FEchoesPrologueMissionModel::StableName(ProloguePhase),
                        static_cast<uint8>(Consequence),
                        static_cast<uint8>(RecordedConsequence),
                        static_cast<uint8>(CampaignStatus),
                        static_cast<unsigned long long>(Simulation->CurrentTick()),
                        CampaignDetail);
                }
                else
                {
                    UE_LOG(
                        LogEchoes,
                        Display,
                        TEXT("[ECHOES_PROLOGUE_FINISHED] result=%s phase=%s consequence=%u recordedConsequence=%u campaignStatus=%u tick=%llu detail=%s"),
                        ResultName,
                        FEchoesPrologueMissionModel::StableName(ProloguePhase),
                        static_cast<uint8>(Consequence),
                        static_cast<uint8>(RecordedConsequence),
                        static_cast<uint8>(CampaignStatus),
                        static_cast<unsigned long long>(Simulation->CurrentTick()),
                        CampaignDetail);
                }
            }
            else if (SelectedOperation ==
                         EEchoesOperationMode::CampaignSevenAccounts &&
                     !bMatchResultReported)
            {
                const EEchoesSevenAccountsPhase SevenAccountsPhase =
                    GetSevenAccountsPhase();
                const bool bSevenAccountsFinished =
                    SevenAccountsPhase == EEchoesSevenAccountsPhase::Complete ||
                    SevenAccountsPhase == EEchoesSevenAccountsPhase::Failed;
                if (bSevenAccountsFinished)
                {
                    bMatchResultReported = true;
                    bSimulationPaused = true;
                    const FutureWellChoice Consequence =
                        GetRecordedPrologueChoice();
                    FutureWellChoice RecordedConsequence = Consequence;
                    FString CampaignFeedback;
                    const EEchoesCampaignCommitStatus CampaignStatus =
                        SevenAccountsPhase == EEchoesSevenAccountsPhase::Complete
                            ? CommitSevenAccountsCompletion(
                                  RecordedConsequence,
                                  CampaignFeedback)
                            : EEchoesCampaignCommitStatus::NotApplicable;
                    if (AEchoesPlayerController* Controller =
                            Cast<AEchoesPlayerController>(
                                GetWorld()->GetFirstPlayerController()))
                    {
                        Controller->NotifySevenAccountsFinished(
                            SevenAccountsPhase ==
                                EEchoesSevenAccountsPhase::Complete,
                            Consequence,
                            RecordedConsequence,
                            CampaignStatus);
                    }
                    UE_LOG(
                        LogEchoes,
                        Display,
                        TEXT("[ECHOES_SEVEN_ACCOUNTS_FINISHED] result=%s phase=%s branch=%u recordedBranch=%u campaignStatus=%u tick=%llu detail=%s"),
                        SevenAccountsPhase ==
                                EEchoesSevenAccountsPhase::Complete
                            ? TEXT("success")
                            : TEXT("failure"),
                        FEchoesSevenAccountsMissionModel::StableName(
                            SevenAccountsPhase),
                        static_cast<uint8>(Consequence),
                        static_cast<uint8>(RecordedConsequence),
                        static_cast<uint8>(CampaignStatus),
                        static_cast<unsigned long long>(
                            Simulation->CurrentTick()),
                    CampaignFeedback.IsEmpty()
                            ? TEXT("not-applicable")
                            : *CampaignFeedback);
                }
            }
            else if (SelectedOperation ==
                         EEchoesOperationMode::CampaignCityReserve &&
                     !bMatchResultReported)
            {
                const EEchoesCityReservePhase CityReservePhase =
                    GetCityReservePhase();
                const bool bCityReserveFinished =
                    CityReservePhase == EEchoesCityReservePhase::Complete ||
                    CityReservePhase == EEchoesCityReservePhase::Failed;
                if (bCityReserveFinished)
                {
                    bMatchResultReported = true;
                    bSimulationPaused = true;
                    const FutureWellChoice Consequence =
                        GetRecordedPrologueChoice();
                    FutureWellChoice RecordedConsequence = Consequence;
                    FString CampaignFeedback;
                    const EEchoesCampaignCommitStatus CampaignStatus =
                        CityReservePhase ==
                                EEchoesCityReservePhase::Complete
                            ? CommitCityReserveCompletion(
                                  RecordedConsequence,
                                  CampaignFeedback)
                            : EEchoesCampaignCommitStatus::NotApplicable;
                    if (AEchoesPlayerController* Controller =
                            Cast<AEchoesPlayerController>(
                                GetWorld()->GetFirstPlayerController()))
                    {
                        Controller->NotifyCityReserveFinished(
                            CityReservePhase ==
                                EEchoesCityReservePhase::Complete,
                            Consequence,
                            RecordedConsequence,
                            CampaignStatus);
                    }
                    UE_LOG(
                        LogEchoes,
                        Display,
                        TEXT("[ECHOES_CITY_RESERVE_FINISHED] result=%s phase=%s branch=%u recordedBranch=%u campaignStatus=%u tick=%llu detail=%s"),
                        CityReservePhase ==
                                EEchoesCityReservePhase::Complete
                            ? TEXT("success")
                            : TEXT("failure"),
                        FEchoesCityReserveMissionModel::StableName(
                            CityReservePhase),
                        static_cast<uint8>(Consequence),
                        static_cast<uint8>(RecordedConsequence),
                        static_cast<uint8>(CampaignStatus),
                        static_cast<unsigned long long>(
                            Simulation->CurrentTick()),
                        CampaignFeedback.IsEmpty()
                            ? TEXT("not-applicable")
                            : *CampaignFeedback);
                }
            }
            else if (SelectedOperation ==
                         EEchoesOperationMode::CampaignUnburiedRoad &&
                     !bMatchResultReported)
            {
                const EEchoesUnburiedRoadPhase UnburiedRoadPhase =
                    GetUnburiedRoadPhase();
                const bool bUnburiedRoadFinished =
                    UnburiedRoadPhase == EEchoesUnburiedRoadPhase::Complete ||
                    UnburiedRoadPhase == EEchoesUnburiedRoadPhase::Failed;
                if (bUnburiedRoadFinished)
                {
                    bMatchResultReported = true;
                    bSimulationPaused = true;
                    const FutureWellChoice Consequence =
                        GetRecordedPrologueChoice();
                    FutureWellChoice RecordedConsequence = Consequence;
                    FString CampaignFeedback;
                    const EEchoesCampaignCommitStatus CampaignStatus =
                        UnburiedRoadPhase ==
                                EEchoesUnburiedRoadPhase::Complete
                            ? CommitUnburiedRoadCompletion(
                                  RecordedConsequence,
                                  CampaignFeedback)
                            : EEchoesCampaignCommitStatus::NotApplicable;
                    if (AEchoesPlayerController* Controller =
                            Cast<AEchoesPlayerController>(
                                GetWorld()->GetFirstPlayerController()))
                    {
                        Controller->NotifyUnburiedRoadFinished(
                            UnburiedRoadPhase ==
                                EEchoesUnburiedRoadPhase::Complete,
                            Consequence,
                            RecordedConsequence,
                            CampaignStatus);
                    }
                    UE_LOG(
                        LogEchoes,
                        Display,
                        TEXT("[ECHOES_UNBURIED_ROAD_FINISHED] result=%s phase=%s branch=%u recordedBranch=%u campaignStatus=%u tick=%llu detail=%s"),
                        UnburiedRoadPhase ==
                                EEchoesUnburiedRoadPhase::Complete
                            ? TEXT("success")
                            : TEXT("failure"),
                        FEchoesUnburiedRoadMissionModel::StableName(
                            UnburiedRoadPhase),
                        static_cast<uint8>(Consequence),
                        static_cast<uint8>(RecordedConsequence),
                        static_cast<uint8>(CampaignStatus),
                        static_cast<unsigned long long>(
                            Simulation->CurrentTick()),
                        CampaignFeedback.IsEmpty()
                            ? TEXT("not-applicable")
                            : *CampaignFeedback);
                }
            }
            else if (SelectedOperation ==
                         EEchoesOperationMode::CampaignTermsOfContinuance &&
                     !bMatchResultReported)
            {
                const EEchoesTermsOfContinuancePhase ContinuancePhase =
                    GetTermsOfContinuancePhase();
                const bool bContinuanceFinished =
                    ContinuancePhase ==
                        EEchoesTermsOfContinuancePhase::Complete ||
                    ContinuancePhase ==
                        EEchoesTermsOfContinuancePhase::Failed;
                if (bContinuanceFinished)
                {
                    bMatchResultReported = true;
                    bSimulationPaused = true;
                    const FutureWellChoice Consequence =
                        GetRecordedPrologueChoice();
                    FutureWellChoice RecordedConsequence = Consequence;
                    FString CampaignFeedback;
                    const EEchoesCampaignCommitStatus CampaignStatus =
                        ContinuancePhase ==
                                EEchoesTermsOfContinuancePhase::Complete
                            ? CommitTermsOfContinuanceCompletion(
                                  RecordedConsequence,
                                  CampaignFeedback)
                            : EEchoesCampaignCommitStatus::NotApplicable;
                    if (AEchoesPlayerController* Controller =
                            Cast<AEchoesPlayerController>(
                                GetWorld()->GetFirstPlayerController()))
                    {
                        Controller->NotifyTermsOfContinuanceFinished(
                            ContinuancePhase ==
                                EEchoesTermsOfContinuancePhase::Complete,
                            Consequence,
                            RecordedConsequence,
                            CampaignStatus);
                    }
                    UE_LOG(
                        LogEchoes,
                        Display,
                        TEXT("[ECHOES_TERMS_OF_CONTINUANCE_FINISHED] result=%s phase=%s branch=%u recordedBranch=%u campaignStatus=%u tick=%llu detail=%s"),
                        ContinuancePhase ==
                                EEchoesTermsOfContinuancePhase::Complete
                            ? TEXT("success")
                            : TEXT("failure"),
                        FEchoesTermsOfContinuanceMissionModel::StableName(
                            ContinuancePhase),
                        static_cast<uint8>(Consequence),
                        static_cast<uint8>(RecordedConsequence),
                        static_cast<uint8>(CampaignStatus),
                        static_cast<unsigned long long>(
                            Simulation->CurrentTick()),
                        CampaignFeedback.IsEmpty()
                            ? TEXT("not-applicable")
                            : *CampaignFeedback);
                }
            }
            else if (SelectedOperation ==
                         EEchoesOperationMode::CampaignNamesWithoutBirths &&
                     !bMatchResultReported)
            {
                const EEchoesNamesWithoutBirthsPhase NamesPhase =
                    GetNamesWithoutBirthsPhase();
                const bool bNamesFinished =
                    NamesPhase == EEchoesNamesWithoutBirthsPhase::Complete ||
                    NamesPhase == EEchoesNamesWithoutBirthsPhase::Failed;
                if (bNamesFinished)
                {
                    bMatchResultReported = true;
                    bSimulationPaused = true;
                    const FutureWellChoice Consequence =
                        GetRecordedPrologueChoice();
                    FutureWellChoice RecordedConsequence = Consequence;
                    FString CampaignFeedback;
                    const EEchoesCampaignCommitStatus CampaignStatus =
                        NamesPhase ==
                                EEchoesNamesWithoutBirthsPhase::Complete
                            ? CommitNamesWithoutBirthsCompletion(
                                  RecordedConsequence, CampaignFeedback)
                            : EEchoesCampaignCommitStatus::NotApplicable;
                    if (AEchoesPlayerController* Controller =
                            Cast<AEchoesPlayerController>(
                                GetWorld()->GetFirstPlayerController()))
                    {
                        Controller->NotifyNamesWithoutBirthsFinished(
                            NamesPhase ==
                                EEchoesNamesWithoutBirthsPhase::Complete,
                            Consequence,
                            RecordedConsequence,
                            CampaignStatus);
                    }
                    UE_LOG(
                        LogEchoes,
                        Display,
                        TEXT("[ECHOES_NAMES_WITHOUT_BIRTHS_FINISHED] result=%s phase=%s branch=%u recordedBranch=%u campaignStatus=%u tick=%llu detail=%s"),
                        NamesPhase ==
                                EEchoesNamesWithoutBirthsPhase::Complete
                            ? TEXT("success")
                            : TEXT("failure"),
                        FEchoesNamesWithoutBirthsMissionModel::StableName(
                            NamesPhase),
                        static_cast<uint8>(Consequence),
                        static_cast<uint8>(RecordedConsequence),
                        static_cast<uint8>(CampaignStatus),
                        static_cast<unsigned long long>(
                            Simulation->CurrentTick()),
                        CampaignFeedback.IsEmpty()
                            ? TEXT("not-applicable")
                            : *CampaignFeedback);
                }
            }
            else if (SelectedOperation ==
                         EEchoesOperationMode::CampaignShapeOfSilence &&
                     !bMatchResultReported)
            {
                const EEchoesShapeOfSilencePhase ShapePhase =
                    GetShapeOfSilencePhase();
                const bool bShapeFinished =
                    ShapePhase == EEchoesShapeOfSilencePhase::Complete ||
                    ShapePhase == EEchoesShapeOfSilencePhase::Failed;
                if (bShapeFinished)
                {
                    bMatchResultReported = true;
                    bSimulationPaused = true;
                    const FutureWellChoice Consequence =
                        GetRecordedPrologueChoice();
                    FutureWellChoice RecordedConsequence = Consequence;
                    FString CampaignFeedback;
                    const EEchoesCampaignCommitStatus CampaignStatus =
                        ShapePhase == EEchoesShapeOfSilencePhase::Complete
                            ? CommitShapeOfSilenceCompletion(
                                  RecordedConsequence, CampaignFeedback)
                            : EEchoesCampaignCommitStatus::NotApplicable;
                    if (AEchoesPlayerController* Controller =
                            Cast<AEchoesPlayerController>(
                                GetWorld()->GetFirstPlayerController()))
                    {
                        Controller->NotifyShapeOfSilenceFinished(
                            ShapePhase ==
                                EEchoesShapeOfSilencePhase::Complete,
                            Consequence,
                            RecordedConsequence,
                            CampaignStatus);
                    }
                    UE_LOG(
                        LogEchoes,
                        Display,
                        TEXT("[ECHOES_SHAPE_OF_SILENCE_FINISHED] result=%s phase=%s branch=%u recordedBranch=%u campaignStatus=%u tick=%llu detail=%s"),
                        ShapePhase == EEchoesShapeOfSilencePhase::Complete
                            ? TEXT("success")
                            : TEXT("failure"),
                        FEchoesShapeOfSilenceMissionModel::StableName(
                            ShapePhase),
                        static_cast<uint8>(Consequence),
                        static_cast<uint8>(RecordedConsequence),
                        static_cast<uint8>(CampaignStatus),
                        static_cast<unsigned long long>(
                            Simulation->CurrentTick()),
                        CampaignFeedback.IsEmpty()
                            ? TEXT("not-applicable")
                            : *CampaignFeedback);
                }
            }
            else if (SelectedOperation == EEchoesOperationMode::Skirmish &&
                     Outcome != echoes::sim::MatchOutcome::Ongoing &&
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
            AdvancePrologueCompletionPresentation();
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
            if (bResearchPresentationScenario ||
                bResearchInterruptionPresentationScenario)
            {
                const echoes::sim::PlayerState* Player =
                    Simulation->FindPlayer(LocalPlayerId);
                if (Player != nullptr &&
                    !bLoggedResearchPresentationActive &&
                    Player->activeResearch == ResearchPresentationTechnology)
                {
                    UE_LOG(
                        LogEchoes,
                        Display,
                        TEXT("[ECHOES_RESEARCH_PRESENTATION_ACTIVE] technology=%s progress=%d required=%d controlled=true"),
                        ResearchStableName(ResearchPresentationTechnology),
                        Player->researchProgress,
                        Player->researchRequired);
                    bLoggedResearchPresentationActive = true;
                }
                if (Player != nullptr &&
                    !bLoggedResearchPresentationComplete &&
                    bResearchPresentationScenario &&
                    Player->HasCompletedResearch(
                        ResearchPresentationTechnology))
                {
                    UE_LOG(
                        LogEchoes,
                        Display,
                        TEXT("[ECHOES_RESEARCH_PRESENTATION_COMPLETE] technology=%s completed=true controlled=true"),
                        ResearchStableName(ResearchPresentationTechnology));
                    bLoggedResearchPresentationComplete = true;
                }
                if (Player != nullptr &&
                    !bLoggedResearchPresentationInterrupted &&
                    bResearchInterruptionPresentationScenario &&
                    Player->lastInterruptedResearch ==
                        ResearchPresentationTechnology)
                {
                    UE_LOG(
                        LogEchoes,
                        Display,
                        TEXT("[ECHOES_RESEARCH_PRESENTATION_INTERRUPTED] technology=%s producerDestroyed=true costsRefunded=false controlled=true"),
                        ResearchStableName(ResearchPresentationTechnology));
                    bLoggedResearchPresentationInterrupted = true;
                    if (AEchoesPlayerController* Controller =
                            Cast<AEchoesPlayerController>(
                                GetWorld()->GetFirstPlayerController()))
                    {
                        if (!Controller->IsTechnologyPanelVisible())
                        {
                            Controller->ToggleTechnologyPanel();
                        }
                        if (Controller->IsTechnologyPanelVisible())
                        {
                            UE_LOG(
                                LogEchoes,
                                Display,
                                TEXT("[ECHOES_RESEARCH_INTERRUPTION_PANEL_READY] visible=true paused=true controlled=true release=false"));
                        }
                    }
                }
            }
            if (bKharuunSystemsPresentationScenario &&
                !bLoggedKharuunSystemsPresentation)
            {
                bool bWaystoneTransitioning = false;
                bool bCarapaceMoltActive = false;
                bool bMineralCoverActive = false;
                for (const echoes::sim::Entity& Entity : Simulation->Entities())
                {
                    if (Entity.owner == LocalPlayerId &&
                        Entity.type == EntityType::Dropoff &&
                        Entity.waystoneMode ==
                            echoes::sim::WaystoneMode::Uprooting)
                    {
                        bWaystoneTransitioning = true;
                    }
                    if (Entity.owner == LocalPlayerId &&
                        Entity.pendingWarformAdaptation ==
                            echoes::sim::WarformAdaptation::Carapace)
                    {
                        bCarapaceMoltActive = true;
                    }
                    if (Entity.owner == LocalPlayerId &&
                        Entity.temporaryMineralCover && Entity.hitPoints > 0)
                    {
                        bMineralCoverActive = true;
                    }
                }
                const std::optional<echoes::sim::PlayerView> LocalView =
                    Simulation->CreatePlayerView(LocalPlayerId);
                const int32 VibrationContacts =
                    LocalView.has_value()
                        ? static_cast<int32>(
                              LocalView->VibrationSignatures().size())
                        : 0;
                const bool bHiddenSourceDisclosed =
                    LocalView.has_value() &&
                    std::any_of(
                        LocalView->Entities().begin(),
                        LocalView->Entities().end(),
                        [](const echoes::sim::Entity& Entity)
                        {
                            return Entity.owner == 2;
                        });
                if (bWaystoneTransitioning && bCarapaceMoltActive &&
                    bMineralCoverActive && VibrationContacts > 0 &&
                    !bHiddenSourceDisclosed)
                {
                    bLoggedKharuunSystemsPresentation = true;
                    bSimulationPaused = true;
                    UE_LOG(
                        LogEchoes,
                        Display,
                        TEXT("[ECHOES_KHARUUN_SYSTEMS_PRESENTATION_ACTIVE] waystone=uprooting warform=carapace_molt cover=active vibrationContacts=%d anonymous=true hiddenSourceDisclosed=false paused=true controlled=true release=false"),
                        VibrationContacts);
                }
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
            if (SelectedOperation == EEchoesOperationMode::CampaignPrologue &&
                GetProloguePhase() != EEchoesProloguePhase::DecideFutureWell)
            {
                OutFeedback = TEXT("[ARCHIVE_REQUIRED] Mara Vey's archive carrier must hold the recovery site at tile 22,18 before a Well protocol can be committed.");
                return false;
            }
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
            if (SelectedOperation ==
                    EEchoesOperationMode::CampaignNamesWithoutBirths &&
                BuildType == EntityType::Dropoff &&
                GetNamesWithoutBirthsPhase() ==
                    EEchoesNamesWithoutBirthsPhase::LocateCensus)
            {
                OutFeedback = TEXT("[CENSUS_TRACE_REQUIRED] Talar must reach the inherited census site before a Power Link can stabilize its archive.");
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
