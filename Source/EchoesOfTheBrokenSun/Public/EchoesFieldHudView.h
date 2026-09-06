// Copyright Echoes of the Broken Sun. All Rights Reserved.
// Author: Angelis Pseftis

#pragma once

#include "CoreMinimal.h"
#include "EchoesCampaignMapLayout.h"
#include "EchoesCommandDeckModel.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesSimCore/NetworkProtocol.h"
#include "EchoesSimCore/Simulation.h"

class AEchoesPlayerController;
class UEchoesGameUserSettings;
class UEchoesNarrativeSubsystem;
class UEchoesSimulationSubsystem;

/** The only state source from which a field-HUD snapshot was materialized. */
enum class EEchoesFieldHudAuthority : uint8
{
    None,
    LivePlayerView,
    NetworkKeyframe,
    ReplayPlayerView,
    ReplayObserver
};

/** Mutually exclusive top-level field presentation. */
enum class EEchoesFieldHudSurface : uint8
{
    Hidden,
    Battlefield,
    Replay,
    CampaignOperations,
    OnlineFrontDoor,
    NetworkLobby,
    OnlineLocalMenu,
    Reconnect
};

enum class EEchoesFieldHudTone : uint8
{
    Normal,
    Muted,
    Accent,
    Success,
    Warning,
    Danger
};

/** Semantic actions emitted by field widgets. Argument meanings are documented per entry. */
enum class EEchoesFieldHudAction : uint8
{
    None,
    CommandDeck,              // Argument is EEchoesCommandDeckAction.
    ToggleTechnology,
    TechnologyPrevious,
    TechnologyNext,
    TechnologyResearchTier,   // Argument is the zero-based tier.
    CampaignSelectNode,       // Argument is the zero-based campaign node.
    CampaignDeploy,
    CampaignBack,
    OnlineHost,
    OnlineEditEndpoint,
    OnlineJoin,
    OnlineCopyHostAddress,
    OnlineBack,
    OnlineRetry,
    NetworkReady,
    OnlineResume,
    OnlineLeave
};

struct ECHOESOFTHEBROKENSUN_API FEchoesFieldHudControl final
{
    FText Label;
    FText Detail;
    EEchoesFieldHudAction Action = EEchoesFieldHudAction::None;
    int32 Argument = 0;
    bool bEnabled = true;
    bool bFocused = false;
    bool bPrimary = false;
};

struct ECHOESOFTHEBROKENSUN_API FEchoesFieldHudLine final
{
    FText Label;
    FText Value;
    EEchoesFieldHudTone Tone = EEchoesFieldHudTone::Normal;
};

struct ECHOESOFTHEBROKENSUN_API FEchoesFieldHudResourceView final
{
    bool bVisible = false;
    int32 Matter = 0;
    int32 Dawn = 0;
    int32 PopulationUsed = 0;
    int32 PopulationCapacity = 0;
    uint64 SimulationTick = 0;
    FText LocalFaction;
    FText OpponentFaction;
    FText MatchState;
    FText ResearchStatus;
};

struct ECHOESOFTHEBROKENSUN_API FEchoesFieldHudSelectionEntry final
{
    uint32 EntityId = 0;
    FText Name;
    FText Faction;
    FText Order;
    int32 Count = 1;
    int32 HitPoints = 0;
    int32 MaxHitPoints = 0;
    int32 Cargo = 0;
    int32 CargoCapacity = 0;
    int32 Damage = 0;
    int32 Armor = 0;
    FText Production;
    int32 ProductionPercent = 0;
    bool bOwned = false;
    bool bStructure = false;
};

struct ECHOESOFTHEBROKENSUN_API FEchoesFieldHudSelectionView final
{
    bool bVisible = false;
    TArray<FEchoesFieldHudSelectionEntry> Entries;
};

struct ECHOESOFTHEBROKENSUN_API FEchoesFieldHudCommandView final
{
    bool bVisible = false;
    FText Formation;
    EEchoesCommandDeckAction ArmedAction = EEchoesCommandDeckAction::None;
    TArray<FEchoesFieldHudControl> Controls;
};

struct ECHOESOFTHEBROKENSUN_API FEchoesFieldHudTechnologyTier final
{
    int32 Tier = 0;
    FText Name;
    FText Description;
    FText Cost;
    FText State;
    EEchoesFieldHudTone Tone = EEchoesFieldHudTone::Normal;
    bool bFocused = false;
    bool bEnabled = false;
};

struct ECHOESOFTHEBROKENSUN_API FEchoesFieldHudTechnologyView final
{
    bool bVisible = false;
    FText Title;
    FText ActiveResearch;
    TArray<FEchoesFieldHudTechnologyTier> Tiers;
    TArray<FEchoesFieldHudControl> Controls;
};

enum class EEchoesFieldHudTileState : uint8
{
    Unexplored,
    ExploredBlocked,
    ExploredOpen,
    ExploredScarred,
    VisibleBlocked,
    VisibleOpen,
    VisibleScarred
};

struct ECHOESOFTHEBROKENSUN_API FEchoesFieldHudMapMarker final
{
    uint32 EntityId = 0;
    FVector2D NormalizedPosition = FVector2D::ZeroVector;
    FText Label;
    uint8 Owner = 0;
    uint8 Faction = 0;
    uint8 EntityType = 0;
    bool bFriendly = false;
    bool bRemembered = false;
    bool bResource = false;
    bool bFutureWell = false;
    bool bTelegraphed = false;
    uint64 TelegraphRemainingTicks = 0;
    uint8 FutureWellChoice = 0;
};

struct ECHOESOFTHEBROKENSUN_API FEchoesFieldHudContact final
{
    FVector2D NormalizedMapPosition = FVector2D::ZeroVector;
    FVector2D NormalizedScreenPosition = FVector2D::ZeroVector;
    FText PrimaryLabel;
    FText SecondaryLabel;
    bool bScreenPlacementValid = false;
    bool bClampedToScreenEdge = false;
};

struct ECHOESOFTHEBROKENSUN_API FEchoesFieldHudMissionMarker final
{
    FVector2D NormalizedMapPosition = FVector2D::ZeroVector;
    FText Label;
    EEchoesFieldHudTone Tone = EEchoesFieldHudTone::Accent;
};

struct ECHOESOFTHEBROKENSUN_API FEchoesFieldHudMinimapView final
{
    bool bVisible = false;
    int32 Width = 0;
    int32 Height = 0;
    TArray<EEchoesFieldHudTileState> Tiles;
    TArray<FEchoesFieldHudMapMarker> Markers;
    TArray<FEchoesFieldHudContact> Contacts;
    TArray<FEchoesFieldHudMissionMarker> MissionMarkers;
    /** Ordered perimeter corners, normalized to map dimensions. */
    TArray<FVector2D> CameraFrustum;
};

struct ECHOESOFTHEBROKENSUN_API FEchoesFieldHudTargetingView final
{
    bool bKeyboardTargetVisible = false;
    FVector2D KeyboardTargetNormalizedOffset = FVector2D::ZeroVector;
    bool bSelectionDragVisible = false;
    FVector2D SelectionStartNormalized = FVector2D::ZeroVector;
    FVector2D SelectionEndNormalized = FVector2D::ZeroVector;
};

struct ECHOESOFTHEBROKENSUN_API FEchoesFieldHudCampaignView final
{
    bool bVisible = false;
    FText Title;
    FText LedgerSummary;
    FEchoesCampaignMapLayout Layout;
    FText SelectedSector;
    FText SelectedTitle;
    FText SelectedBiome;
    FText SelectedStatus;
    FText Briefing;
    FText Reward;
    TArray<FEchoesFieldHudControl> Controls;
};

struct ECHOESOFTHEBROKENSUN_API FEchoesFieldHudOnlineView final
{
    bool bVisible = false;
    FText Title;
    FText State;
    FText Endpoint;
    FText Failure;
    FText Reconnect;
    TArray<FEchoesFieldHudControl> Controls;
};

/**
 * Immutable-by-convention snapshot consumed by UMG. It owns all strings and
 * arrays and contains no simulation, controller, actor, or UObject pointer.
 */
struct ECHOESOFTHEBROKENSUN_API FEchoesFieldHudView final
{
    EEchoesFieldHudAuthority Authority = EEchoesFieldHudAuthority::None;
    EEchoesFieldHudSurface Surface = EEchoesFieldHudSurface::Hidden;
    bool bHighContrast = false;
    bool bReducedMotion = false;
    bool bReducedFlashing = false;
    float HudScale = 1.0f;
    FEchoesFieldHudResourceView Resources;
    FEchoesFieldHudSelectionView Selection;
    FEchoesFieldHudCommandView Commands;
    /** Existing reconstructable mission model, copied only from live authority. */
    bool bObjectiveVisible = false;
    FEchoesObjectiveSnapshot Objective;
    TArray<FEchoesFieldHudLine> ObjectiveLines;
    FText ObjectiveTitle;
    FText Status;
    FText SubtitleSpeaker;
    FText Subtitle;
    FEchoesFieldHudTechnologyView Technology;
    FEchoesFieldHudMinimapView Minimap;
    FEchoesFieldHudTargetingView Targeting;
    FEchoesFieldHudCampaignView Campaign;
    FEchoesFieldHudOnlineView Online;
};

struct ECHOESOFTHEBROKENSUN_API FEchoesFieldHudBuildContext final
{
    const AEchoesPlayerController* Controller = nullptr;
    const UEchoesSimulationSubsystem* Simulation = nullptr;
    const UEchoesGameUserSettings* Settings = nullptr;
    UEchoesNarrativeSubsystem* Narrative = nullptr;
    FVector2D ViewportSize = FVector2D(1920.0f, 1080.0f);
    double RealTimeSeconds = 0.0;
};

/** Pure snapshot builder. Presentation never receives a live authority pointer. */
struct ECHOESOFTHEBROKENSUN_API FEchoesFieldHudModel final
{
    [[nodiscard]] static bool Build(
        const FEchoesFieldHudBuildContext& Context,
        FEchoesFieldHudView& OutView,
        FString& OutError);

    /** Testable fair-information boundaries used by Build. */
    [[nodiscard]] static FEchoesFieldHudView BuildPlayerScoped(
        const echoes::sim::PlayerView& PlayerView,
        const TArray<uint32>& SelectedEntityIds,
        bool bReplay);
    [[nodiscard]] static FEchoesFieldHudView BuildNetworkScoped(
        const echoes::sim::net::ScopedViewKeyframe& Keyframe,
        const TArray<uint32>& SelectedEntityIds);
    [[nodiscard]] static FEchoesFieldHudView BuildReplayObserver(
        const echoes::sim::Simulation& ReplaySimulation);
};

/**
 * Pointer contract used by AEchoesPlayerController::HandleFieldHudPointer:
 * coordinates are normalized map coordinates in [0,1], and bIssueOrder
 * selects right-click order semantics instead of camera pan semantics.
 */
