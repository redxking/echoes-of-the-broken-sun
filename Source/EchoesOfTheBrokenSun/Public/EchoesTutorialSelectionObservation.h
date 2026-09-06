#pragma once

#include "CoreMinimal.h"
#include "EchoesSimCore/Simulation.h"

struct FEchoesFieldHudView;

/** The two selection lessons observed by this bounded predicate model. */
enum class EEchoesTutorialSelectionStage : uint8
{
    None,
    Roster,
    Muster,
};

/**
 * Provenance assigned at the controller input boundary. PlayerInput means the
 * ordinary live-input route; it is not evidence that a physical human acted.
 */
enum class EEchoesTutorialSelectionInputOrigin : uint8
{
    Unknown,
    PlayerInput,
    Replay,
    Programmatic,
};

/** Closed vocabulary for selection actions that the controller may report. */
enum class EEchoesTutorialSelectionInputEvent : uint8
{
    None,
    SingleClickSelection,
    TerrainClear,
    DragSelection,
    SelectionModified,
    SubgroupChanged,
    ControlGroupAssigned,
    ControlGroupRecalled,
    RosterHudPublished,
};

/**
 * One controller-bound input observation. Sequence is strictly increasing
 * within a lesson attempt. Subgroup and control-group fields are meaningful
 * only for their matching event kinds.
 */
struct FEchoesTutorialSelectionEvent final
{
    uint64 Session = 0;
    uint64 Sequence = 0;
    EEchoesTutorialSelectionInputOrigin Origin =
        EEchoesTutorialSelectionInputOrigin::Unknown;
    EEchoesTutorialSelectionInputEvent Event =
        EEchoesTutorialSelectionInputEvent::None;
    int32 ControlGroupIndex = INDEX_NONE;
    echoes::sim::EntityType PreviousSubgroupType =
        echoes::sim::EntityType::Worker;
    echoes::sim::EntityType ActiveSubgroupType =
        echoes::sim::EntityType::Worker;
    /** Slate frame at the controller boundary; meaningful for Roster publication. */
    uint64 PresentationFrame = 0;
};

/** Component observations for SPEC-LSN-002. */
struct FEchoesTutorialRosterSelectionProgress final
{
    bool bSingleClickSelected = false;
    bool bHudPublished = false;
    bool bTerrainCleared = false;

    [[nodiscard]] bool PredicateSatisfied() const
    {
        return bSingleClickSelected && bHudPublished && bTerrainCleared;
    }
};

/** Component observations for SPEC-LSN-003. */
struct FEchoesTutorialMusterSelectionProgress final
{
    bool bDragSelected = false;
    bool bSelectionModified = false;
    bool bSubgroupChanged = false;
    bool bControlGroupAssigned = false;
    bool bControlGroupRecalled = false;

    [[nodiscard]] bool PredicateSatisfied() const
    {
        return bDragSelected && bSelectionModified && bSubgroupChanged &&
            bControlGroupAssigned && bControlGroupRecalled;
    }
};

/**
 * Observes selection lesson components from a live PlayerView and the actual
 * controller selection. It does not award curriculum state, write a profile,
 * publish narrative signals, or accept a UI-supplied success value.
 */
class ECHOESOFTHEBROKENSUN_API FEchoesTutorialSelectionObservation final
{
public:
    /** Open Roster with the staged Surveyor resolved from this PlayerView. */
    bool BeginRoster(
        uint64 Session,
        const echoes::sim::PlayerView& View,
        echoes::sim::EntityId StagedSurveyorId);

    /**
     * Open Muster with the authored mobile section resolved from this
     * PlayerView. The section must support the authored subgroup action.
     */
    bool BeginMuster(
        uint64 Session,
        const echoes::sim::PlayerView& View,
        TConstArrayView<echoes::sim::EntityId> StagedMobileUnitIds);

    /**
     * Observe post-mutation controller state. SavedControlGroupSelection is
     * required only for assignment and recall events.
     */
    void Observe(
        const FEchoesTutorialSelectionEvent& Input,
        const echoes::sim::PlayerView& View,
        TConstArrayView<echoes::sim::EntityId> ActualSelection,
        TConstArrayView<echoes::sim::EntityId> SavedControlGroupSelection = {});

    /**
     * Observe the exact value snapshot supplied to the field-HUD widget. The
     * observer checks the real selected entity, purpose, health, order, and
     * command controls; the caller cannot submit a completion boolean.
     */
    void ObserveRosterHudPublication(
        const FEchoesTutorialSelectionEvent& Input,
        const echoes::sim::PlayerView& View,
        TConstArrayView<echoes::sim::EntityId> ActualSelection,
        const FEchoesFieldHudView& PublishedView);

    void Reset();

    [[nodiscard]] bool IsActive() const
    {
        return Stage != EEchoesTutorialSelectionStage::None;
    }
    [[nodiscard]] EEchoesTutorialSelectionStage ActiveStage() const
    {
        return Stage;
    }
    [[nodiscard]] FEchoesTutorialRosterSelectionProgress RosterProgress() const
    {
        return Roster;
    }
    [[nodiscard]] FEchoesTutorialMusterSelectionProgress MusterProgress() const
    {
        return Muster;
    }

private:
    EEchoesTutorialSelectionStage Stage =
        EEchoesTutorialSelectionStage::None;
    echoes::sim::PlayerId Player = echoes::sim::kNeutralPlayer;
    uint64 ActiveSession = 0;
    uint64 LastSequence = 0;
    uint64 RosterSelectionFrame = 0;
    TArray<echoes::sim::EntityId> StagedEntityIds;
    TArray<echoes::sim::EntityId> LastSelection;
    TArray<echoes::sim::EntityId> AssignedControlGroupSelection;
    int32 AssignedControlGroupIndex = INDEX_NONE;
    FEchoesTutorialRosterSelectionProgress Roster;
    FEchoesTutorialMusterSelectionProgress Muster;
};
