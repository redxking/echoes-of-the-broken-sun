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
struct FEchoesContentCatalog;
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
    OnlineOptions,
    OnlineControls,
    OnlineCommandHistory,
    OpenPauseMenu,
    OpenResourceMonitor,
    OnlineLeave,
    ProductionCancel,          // Opens review for active slot 0 or a 1-based waiting slot.
    ProductionCancelConfirm,   // Confirms only the controller-captured stable item.
    ProductionCancelBack,
    ProductionMoveUp,          // Argument is a 1-based waiting slot.
    ProductionMoveDown,        // Argument is a 1-based waiting slot.
    AcknowledgeTutorialRejection,
    InspectTutorialReserve,
    OpenTutorialSkipModal,
    TutorialSkipCurrentStep,
    TutorialEndAll,
    TutorialCancelSkipModal,
    ActivateRelaySupply
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
    /** Semantic entry carried by the live resource strip; no decorative click target. */
    FEchoesFieldHudControl MonitorControl;
};

/**
 * SPEC-HUD-003 selection guidance for one roster slot, plus the canonical name
 * used when no content catalog is available to resolve it. Docs/Requirements.md
 * owns this wording through SPEC-UNIT-001..012 and the section 13 structure
 * records; this is a presentation restatement and never a second authority.
 */
struct ECHOESOFTHEBROKENSUN_API FEchoesRosterGuidance final
{
    FText Name;
    /**
     * The authored role a player reads. The catalog's own role string is a schema
     * classification token that the content validator depends on, so it is never
     * shown; structures take this wording from their requirement Data Metrics and
     * units from the role named in their Player Purpose clause.
     */
    FText DisplayRole;
    FText Purpose;
    FText StrongUse;
    FText Limitation;
    FText Counterplay;
};

struct ECHOESOFTHEBROKENSUN_API FEchoesFieldHudSelectionEntry final
{
    uint32 EntityId = 0;
    FText Name;
    FText Faction;
    /** Canonical authored roster role, when this entity has a catalog binding. */
    FText Role;
    /** Canonical mechanical role text for the selected roster element. */
    FText Purpose;
    /** SPEC-HUD-003: when to reach for this, what it cannot do, how it is answered. */
    FText StrongUse;
    FText Limitation;
    FText Counterplay;
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

/** Persistent, local presentation action available over a live battlefield. */
struct ECHOESOFTHEBROKENSUN_API FEchoesFieldHudMenuView final
{
    bool bVisible = false;
    FEchoesFieldHudControl Control;
};

struct ECHOESOFTHEBROKENSUN_API FEchoesFieldHudProductionItem final
{
    /** Slot 0 is active; waiting slots are 1-based. */
    int32 Slot = 0;
    uint64 ItemId = 0;
    FText Unit;
    int32 ProgressPercent = 0;
    int32 RequiredTicks = 0;
    int32 ConfiguredMatter = 0;
    int32 ConfiguredDawn = 0;
    int32 InvestedMatter = 0;
    int32 InvestedDawn = 0;
    int32 Logistics = 0;
    bool bActive = false;
};

/** Read-only cancellation quote captured from one stable production item. */
struct ECHOESOFTHEBROKENSUN_API FEchoesFieldHudProductionCancellationView final
{
    bool bVisible = false;
    uint32 ProducerId = 0;
    uint64 ItemId = 0;
    int32 Slot = 0;
    FText Unit;
    int32 ProgressPercent = 0;
    int32 RefundPercent = 0;
    int32 InvestedMatter = 0;
    int32 InvestedDawn = 0;
    int32 RefundMatter = 0;
    int32 RefundDawn = 0;
    bool bActive = false;
};

/** Player-scoped queue for one exactly selected owned producer. */
struct ECHOESOFTHEBROKENSUN_API FEchoesFieldHudProductionView final
{
    bool bVisible = false;
    uint32 ProducerId = 0;
    bool bSpawnBlocked = false;
    bool bRallyNeedsAttention = false;
    int32 RallyWaypointCount = 0;
    TArray<FEchoesFieldHudProductionItem> Items;
    TArray<FEchoesFieldHudControl> Controls;
    FEchoesFieldHudProductionCancellationView Cancellation;
};

struct ECHOESOFTHEBROKENSUN_API FEchoesFieldHudCommandView final
{
    bool bVisible = false;
    FText Formation;
    FText AbilityStatus;
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

/** SPEC-TUT-005 spotlight data for tutorial onboarding. */
struct ECHOESOFTHEBROKENSUN_API FEchoesTutorialSpotlightView final
{
    bool bActive = false;
    FVector2D ScreenCenter = FVector2D::ZeroVector;
    FVector2D ScreenSize = FVector2D::ZeroVector;
    FText TargetName;
    FText ActionPrompt;
    FText InputBinding;
};

/** SPEC-TUT-006 tutorial skip modal data. */
struct ECHOESOFTHEBROKENSUN_API FEchoesTutorialSkipModalView final
{
    bool bVisible = false;
    FText Title;
    FText Description;
    TArray<FEchoesFieldHudControl> Controls;
};

/**
 * Immutable-by-convention snapshot consumed by UMG. It owns all strings and
 * arrays and contains no simulation, controller, actor, or UObject pointer.
 */
/** Presentation-only, owned network geometry; never grants gameplay connectivity. */
/** One drawn leg of a plan the player has already given.
 *
 * Before this, an accepted order produced a single 2.4s destination pip and a
 * multi-leg route existed only as the words "RALLY n WAYPOINTS" - a player who
 * set a three-leg route saw the number three and nothing on the map. A leg is
 * emitted per queued order so the whole plan is visible while its owner is
 * selected, and disappears with the selection. Presentation only. */
struct FEchoesFieldHudRouteLeg final
{
    FVector From = FVector::ZeroVector;
    FVector To = FVector::ZeroVector;
    /** 1-based position in its own route, drawn at To. */
    int32 Ordinal = 0;
    /** A producer's rally route rather than a selected unit's march. */
    bool bRally = false;

    friend bool operator==(const FEchoesFieldHudRouteLeg&,
                           const FEchoesFieldHudRouteLeg&) = default;
};

struct FEchoesNetworkCoverageView final
{
    FVector Center = FVector::ZeroVector;
    float Radius = 0.0f;
    bool bOperational = false;
};

struct FEchoesNetworkConnectionView final
{
    FVector From = FVector::ZeroVector;
    FVector To = FVector::ZeroVector;
};

struct ECHOESOFTHEBROKENSUN_API FEchoesFieldHudView final
{
    EEchoesFieldHudAuthority Authority = EEchoesFieldHudAuthority::None;
    EEchoesFieldHudSurface Surface = EEchoesFieldHudSurface::Hidden;
    bool bHighContrast = false;
    bool bReducedMotion = false;
    bool bReducedFlashing = false;
    float HudScale = 1.0f;
    bool bTutorialActive = false;
    FText TutorialLessonTitle;
    FText TutorialInstruction;
    FEchoesTutorialSpotlightView TutorialSpotlight;
    FEchoesTutorialSkipModalView TutorialSkipModal;
    FEchoesFieldHudMenuView Menu;
    FEchoesFieldHudResourceView Resources;
    FEchoesFieldHudSelectionView Selection;
    TArray<FEchoesNetworkCoverageView> NetworkCoverage;
    /** Queued-order breadcrumbs and rally routes for the current selection. */
    TArray<FEchoesFieldHudRouteLeg> OrderRoutes;
    TArray<FEchoesNetworkConnectionView> NetworkConnections;
    FEchoesFieldHudProductionView Production;
    FEchoesFieldHudCommandView Commands;
    /** Existing reconstructable mission model, copied only from live authority. */
    bool bObjectiveVisible = false;
    FEchoesObjectiveSnapshot Objective;
    TArray<FEchoesFieldHudLine> ObjectiveLines;
    TArray<FEchoesFieldHudControl> ObjectiveControls;
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

    /** Name and type of the entity under the pointer, for the hover readout.
     *
     * Resolved from the player-scoped view only. An entity the scoped view does
     * not carry - anything under fog - resolves to empty rather than to a name,
     * so hovering can never report knowledge the player has not earned. The
     * pointer path already resolves ownership and legality from this same view
     * but carried no identity, so hovering never said WHAT was under it. */
    struct FHoverIdentity final
    {
        FText Name;
        FText TypeLabel;
        [[nodiscard]] bool IsKnown() const { return !Name.IsEmpty(); }
    };
    [[nodiscard]] static FHoverIdentity HoverIdentity(
        const echoes::sim::PlayerView& PlayerView,
        uint32 EntityId,
        const FEchoesContentCatalog* Catalog = nullptr);

    /** The match state as one seat experiences it.
     *
     * Exposed because the mapping was seat-blind: a network client is always
     * bound to seat 1, so a client who won read DEFEAT on the HUD while the
     * result screen said victory. Asserting it needs the seat in hand. */
    [[nodiscard]] static FText MatchStateText(
        echoes::sim::MatchOutcome Outcome,
        echoes::sim::PlayerId ViewerSeat);

    /** Testable fair-information boundaries used by Build. */
    [[nodiscard]] static FEchoesFieldHudView BuildPlayerScoped(
        const echoes::sim::PlayerView& PlayerView,
        const TArray<uint32>& SelectedEntityIds,
        bool bReplay,
        const FEchoesContentCatalog* Catalog = nullptr);
    [[nodiscard]] static FEchoesFieldHudView BuildNetworkScoped(
        const echoes::sim::net::ScopedViewKeyframe& Keyframe,
        const TArray<uint32>& SelectedEntityIds,
        const FEchoesContentCatalog* Catalog = nullptr);
    [[nodiscard]] static FEchoesFieldHudView BuildReplayObserver(
        const echoes::sim::Simulation& ReplaySimulation,
        const FEchoesContentCatalog* Catalog = nullptr);

    /**
     * SPEC-HUD-003 guidance for a faction roster slot. Returns empty fields for
     * ResourceNode and FutureWell, which belong to the world rather than to any
     * faction roster. The content catalog remains the authority for the display
     * name; the name here answers a caller that has no catalog to consult.
     */
    [[nodiscard]] static FEchoesRosterGuidance RosterGuidance(
        echoes::sim::Faction FactionValue,
        echoes::sim::EntityType Type);
};

/**
 * Pointer contract used by AEchoesPlayerController::HandleFieldHudPointer:
 * coordinates are normalized map coordinates in [0,1], and bIssueOrder
 * selects right-click order semantics instead of camera pan semantics.
 */
