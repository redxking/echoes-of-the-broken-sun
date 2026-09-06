#include "EchoesTutorialSelectionObservation.h"
#include "EchoesFieldHudView.h"

#include <algorithm>

namespace
{
using echoes::sim::Entity;
using echoes::sim::EntityId;
using echoes::sim::EntityType;
using echoes::sim::PlayerView;

const Entity* FindEntity(const PlayerView& View, EntityId Id)
{
    const auto Found = std::find_if(
        View.Entities().begin(),
        View.Entities().end(),
        [Id](const Entity& Candidate)
        {
            return Candidate.id == Id;
        });
    return Found != View.Entities().end() ? &*Found : nullptr;
}

bool IsMobileUnit(EntityType Type)
{
    return Type == EntityType::Worker || Type == EntityType::Soldier ||
        Type == EntityType::HeavyUnit || Type == EntityType::ScoutUnit;
}

bool CopyCanonicalIds(
    TConstArrayView<EntityId> Ids,
    TArray<EntityId>& OutIds)
{
    OutIds.Reset(Ids.Num());
    for (const EntityId Id : Ids)
    {
        if (Id == 0 || OutIds.Contains(Id))
        {
            OutIds.Reset();
            return false;
        }
        OutIds.Add(Id);
    }
    OutIds.Sort();
    return true;
}

bool SameIds(const TArray<EntityId>& Left, const TArray<EntityId>& Right)
{
    return Left == Right;
}

bool IncludesAll(
    const TArray<EntityId>& Selection,
    const TArray<EntityId>& Required)
{
    for (const EntityId Id : Required)
    {
        if (!Selection.Contains(Id)) return false;
    }
    return true;
}

bool IsOwnedAliveSelection(
    const PlayerView& View,
    const TArray<EntityId>& Selection,
    bool bRequireMobile)
{
    if (!View.Player().active) return false;
    for (const EntityId Id : Selection)
    {
        const Entity* Selected = FindEntity(View, Id);
        if (Selected == nullptr || Selected->owner != View.Player().id ||
            Selected->hitPoints <= 0 ||
            (bRequireMobile && !IsMobileUnit(Selected->type)))
        {
            return false;
        }
    }
    return true;
}

bool ContainsType(
    const PlayerView& View,
    const TArray<EntityId>& Selection,
    EntityType Type)
{
    for (const EntityId Id : Selection)
    {
        const Entity* Selected = FindEntity(View, Id);
        if (Selected != nullptr && Selected->type == Type) return true;
    }
    return false;
}

bool HasMixedTypes(
    const PlayerView& View,
    const TArray<EntityId>& Selection)
{
    if (Selection.IsEmpty()) return false;
    const Entity* First = FindEntity(View, Selection[0]);
    if (First == nullptr) return false;
    for (int32 Index = 1; Index < Selection.Num(); ++Index)
    {
        const Entity* Selected = FindEntity(View, Selection[Index]);
        if (Selected != nullptr && Selected->type != First->type) return true;
    }
    return false;
}
}

void FEchoesTutorialSelectionObservation::Reset()
{
    *this = FEchoesTutorialSelectionObservation{};
}

bool FEchoesTutorialSelectionObservation::BeginRoster(
    uint64 Session,
    const echoes::sim::PlayerView& View,
    echoes::sim::EntityId StagedSurveyorId)
{
    Reset();
    const echoes::sim::Entity* Surveyor = FindEntity(View, StagedSurveyorId);
    if (Session == 0 || !View.Player().active || Surveyor == nullptr ||
        Surveyor->owner != View.Player().id || Surveyor->hitPoints <= 0 ||
        Surveyor->faction != echoes::sim::Faction::MeridianCompact ||
        Surveyor->type != echoes::sim::EntityType::Worker)
    {
        return false;
    }

    Stage = EEchoesTutorialSelectionStage::Roster;
    Player = View.Player().id;
    ActiveSession = Session;
    StagedEntityIds.Add(StagedSurveyorId);
    return true;
}

bool FEchoesTutorialSelectionObservation::BeginMuster(
    uint64 Session,
    const echoes::sim::PlayerView& View,
    TConstArrayView<echoes::sim::EntityId> StagedMobileUnitIds)
{
    Reset();
    TArray<echoes::sim::EntityId> CanonicalStage;
    if (Session == 0 || !View.Player().active ||
        !CopyCanonicalIds(StagedMobileUnitIds, CanonicalStage) ||
        CanonicalStage.Num() < 2 ||
        !IsOwnedAliveSelection(View, CanonicalStage, true) ||
        !HasMixedTypes(View, CanonicalStage))
    {
        return false;
    }

    Stage = EEchoesTutorialSelectionStage::Muster;
    Player = View.Player().id;
    ActiveSession = Session;
    StagedEntityIds = MoveTemp(CanonicalStage);
    return true;
}

void FEchoesTutorialSelectionObservation::Observe(
    const FEchoesTutorialSelectionEvent& Input,
    const echoes::sim::PlayerView& View,
    TConstArrayView<echoes::sim::EntityId> ActualSelection,
    TConstArrayView<echoes::sim::EntityId> SavedControlGroupSelection)
{
    if (!IsActive() || Input.Session != ActiveSession || Input.Sequence == 0 ||
        Input.Sequence <= LastSequence)
    {
        return;
    }

    // Consume every new event before checking provenance or payload. A later
    // retry needs a later sequence; a rejected event cannot be replayed into
    // a successful observation.
    LastSequence = Input.Sequence;
    if (Input.Origin != EEchoesTutorialSelectionInputOrigin::PlayerInput)
    {
        return;
    }

    if (!View.Player().active || View.Player().id != Player ||
        !IsOwnedAliveSelection(View, StagedEntityIds, Stage ==
            EEchoesTutorialSelectionStage::Muster))
    {
        // A crossed authority or lost staged actor requires authored restaging.
        Reset();
        return;
    }

    TArray<echoes::sim::EntityId> Selection;
    if (!CopyCanonicalIds(ActualSelection, Selection)) return;

    if (Stage == EEchoesTutorialSelectionStage::Roster)
    {
        if (Input.Event ==
                EEchoesTutorialSelectionInputEvent::SingleClickSelection &&
            IsOwnedAliveSelection(View, Selection, false) &&
            SameIds(Selection, StagedEntityIds))
        {
            Roster.bSingleClickSelected = true;
            RosterSelectionFrame = Input.PresentationFrame;
            LastSelection = Selection;
        }
        else if (Input.Event ==
                     EEchoesTutorialSelectionInputEvent::TerrainClear &&
                 Roster.bSingleClickSelected && Roster.bHudPublished &&
                 Selection.IsEmpty())
        {
            Roster.bTerrainCleared = true;
            LastSelection.Reset();
        }
        return;
    }

    if (Stage != EEchoesTutorialSelectionStage::Muster ||
        (!Selection.IsEmpty() &&
            !IsOwnedAliveSelection(View, Selection, true)))
    {
        return;
    }

    switch (Input.Event)
    {
        case EEchoesTutorialSelectionInputEvent::DragSelection:
            if (!Selection.IsEmpty() && IncludesAll(Selection, StagedEntityIds))
            {
                Muster.bDragSelected = true;
                LastSelection = Selection;
            }
            break;

        case EEchoesTutorialSelectionInputEvent::SelectionModified:
            if (Muster.bDragSelected && !Selection.IsEmpty() &&
                !SameIds(Selection, LastSelection))
            {
                Muster.bSelectionModified = true;
                LastSelection = Selection;
            }
            break;

        case EEchoesTutorialSelectionInputEvent::SubgroupChanged:
            if (Muster.bDragSelected && SameIds(Selection, LastSelection) &&
                IncludesAll(Selection, StagedEntityIds) &&
                HasMixedTypes(View, Selection) &&
                Input.PreviousSubgroupType != Input.ActiveSubgroupType &&
                ContainsType(View, Selection, Input.PreviousSubgroupType) &&
                ContainsType(View, Selection, Input.ActiveSubgroupType))
            {
                Muster.bSubgroupChanged = true;
            }
            break;

        case EEchoesTutorialSelectionInputEvent::ControlGroupAssigned:
        {
            TArray<echoes::sim::EntityId> SavedSelection;
            if (Muster.bDragSelected && Input.ControlGroupIndex >= 0 &&
                Input.ControlGroupIndex < 10 && !Selection.IsEmpty() &&
                IncludesAll(Selection, StagedEntityIds) &&
                SameIds(Selection, LastSelection) &&
                CopyCanonicalIds(SavedControlGroupSelection, SavedSelection) &&
                SameIds(SavedSelection, Selection))
            {
                Muster.bControlGroupAssigned = true;
                AssignedControlGroupIndex = Input.ControlGroupIndex;
                AssignedControlGroupSelection = MoveTemp(SavedSelection);
            }
            break;
        }

        case EEchoesTutorialSelectionInputEvent::ControlGroupRecalled:
        {
            TArray<echoes::sim::EntityId> SavedSelection;
            if (Muster.bControlGroupAssigned &&
                Input.ControlGroupIndex == AssignedControlGroupIndex &&
                CopyCanonicalIds(SavedControlGroupSelection, SavedSelection) &&
                SameIds(SavedSelection, AssignedControlGroupSelection) &&
                SameIds(Selection, AssignedControlGroupSelection))
            {
                Muster.bControlGroupRecalled = true;
                LastSelection = Selection;
            }
            break;
        }

        default:
            break;
    }
}

void FEchoesTutorialSelectionObservation::ObserveRosterHudPublication(
    const FEchoesTutorialSelectionEvent& Input,
    const echoes::sim::PlayerView& View,
    TConstArrayView<echoes::sim::EntityId> ActualSelection,
    const FEchoesFieldHudView& PublishedView)
{
    if (Stage != EEchoesTutorialSelectionStage::Roster ||
        Input.Event != EEchoesTutorialSelectionInputEvent::RosterHudPublished ||
        Input.Session != ActiveSession || Input.Sequence == 0 ||
        Input.Sequence <= LastSequence)
    {
        return;
    }
    LastSequence = Input.Sequence;
    if (Input.Origin != EEchoesTutorialSelectionInputOrigin::PlayerInput ||
        !Roster.bSingleClickSelected ||
        Input.PresentationFrame <= RosterSelectionFrame)
    {
        return;
    }
    if (!View.Player().active || View.Player().id != Player ||
        !IsOwnedAliveSelection(View, StagedEntityIds, false))
    {
        Reset();
        return;
    }
    TArray<echoes::sim::EntityId> Selection;
    if (!CopyCanonicalIds(ActualSelection, Selection) ||
        !SameIds(Selection, StagedEntityIds) ||
        PublishedView.Authority != EEchoesFieldHudAuthority::LivePlayerView ||
        PublishedView.Surface != EEchoesFieldHudSurface::Battlefield ||
        !PublishedView.Selection.bVisible ||
        PublishedView.Selection.Entries.Num() != 1 ||
        !PublishedView.Commands.bVisible ||
        PublishedView.Commands.Controls.IsEmpty())
    {
        return;
    }
    const FEchoesFieldHudSelectionEntry& Entry =
        PublishedView.Selection.Entries[0];
    if (Entry.EntityId != StagedEntityIds[0] || !Entry.bOwned ||
        Entry.Name.IsEmpty() || Entry.Purpose.IsEmpty() ||
        Entry.Order.IsEmpty() || Entry.MaxHitPoints <= 0 ||
        Entry.HitPoints <= 0 || Entry.HitPoints > Entry.MaxHitPoints ||
        !PublishedView.Commands.Controls.ContainsByPredicate(
            [](const FEchoesFieldHudControl& Control)
            {
                return Control.bEnabled &&
                    Control.Action == EEchoesFieldHudAction::CommandDeck;
            }))
    {
        return;
    }
    Roster.bHudPublished = true;
}
