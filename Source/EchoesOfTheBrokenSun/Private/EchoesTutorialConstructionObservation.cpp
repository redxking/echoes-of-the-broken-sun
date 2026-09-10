#include "EchoesTutorialConstructionObservation.h"

#include "EchoesFieldHudView.h"

// A uniquely named namespace, not an anonymous one. Unreal batches several
// .cpp files into one unity translation unit, and every anonymous namespace in
// that unit is the SAME namespace: two file-local helpers sharing a signature
// become a redefinition, and a sibling's parameter shadows a constant here.
// Each file still compiles alone, so nothing catches it until the batch.
// Aliasing the same namespace in each batched file is a redeclaration,
// not a conflict, so this is safe at file scope where the bodies below
// can see it.
namespace sim = echoes::sim;

namespace EchoesTutorialConstructionDetail
{

/**
 * The Power Link is the Dropoff archetype (`SPEC-BLD-015.MC.LINK`). It is the
 * structure this lesson places, assists, completes and repairs.
 */
constexpr sim::EntityType PowerLinkType = sim::EntityType::Dropoff;

bool IsLivingOwnedWorker(
    const sim::Simulation& Simulation,
    sim::PlayerId Player,
    sim::EntityId Id)
{
    const sim::Entity* Entity = Simulation.FindEntity(Id);
    return Entity != nullptr && Entity->owner == Player &&
        Entity->type == sim::EntityType::Worker && Entity->completed &&
        Entity->hitPoints > 0;
}

const sim::Command* FindCommand(
    const sim::Simulation& Simulation,
    sim::PlayerId Player,
    std::uint64_t Sequence)
{
    for (const sim::Command& Command : Simulation.CommandLog())
    {
        if (Command.player == Player && Command.sequence == Sequence)
        {
            return &Command;
        }
    }
    return nullptr;
}

/** Applied is the only outcome that proves the simulation acted on a command. */
bool WasApplied(
    const sim::Simulation& Simulation,
    sim::PlayerId Player,
    std::uint64_t Sequence)
{
    const auto Receipt =
        Simulation.FindCommandResolutionReceipt(Player, Sequence);
    return Receipt.has_value() &&
        Receipt->outcome == sim::CommandResolutionOutcome::Applied;
}

/** A worker actively building this exact site, as the simulation records it. */
bool IsBuildingSite(
    const sim::Simulation& Simulation,
    sim::PlayerId Player,
    sim::EntityId Worker,
    sim::EntityId Site)
{
    const sim::Entity* Entity = Simulation.FindEntity(Worker);
    return Site != 0 && Entity != nullptr && Entity->owner == Player &&
        Entity->hitPoints > 0 && Entity->order.type == sim::OrderType::Build &&
        Entity->order.target == Site;
}
}  // namespace EchoesTutorialConstructionDetail

void FEchoesTutorialConstructionObservation::Reset()
{
    *this = FEchoesTutorialConstructionObservation{};
}

bool FEchoesTutorialConstructionObservation::ValidateAuthority(
    uint64 Session,
    uint64 Generation,
    const sim::Simulation& Simulation) const
{
    // A scenario reload or a restarted lesson replaces the authority this
    // observation was opened against; carrying evidence across either would
    // credit a lesson the player did not perform in the current scenario.
    return bActive && Session != 0 && Session == SessionId &&
        Generation == GenerationId && Simulation.CurrentTick() >= BeginTick;
}

bool FEchoesTutorialConstructionObservation::ValidateEvent(
    const FEchoesTutorialConstructionInput& Input,
    const sim::Simulation& Simulation) const
{
    // Only real player input teaches. A replayed or programmatic origin
    // reproduces the same authoritative state without the player acting, so
    // it is refused here rather than being distinguished later.
    return ValidateAuthority(
               Input.Session, Input.AuthorityGeneration, Simulation) &&
        Input.Origin == EEchoesTutorialConstructionOrigin::PlayerInput &&
        Input.InputSequence > LastInputSequence;
}

bool FEchoesTutorialConstructionObservation::Begin(
    uint64 Session,
    uint64 AuthorityGeneration,
    const sim::Simulation& Simulation,
    const FEchoesTutorialConstructionSetup& Setup)
{
    Reset();
    const auto FirstSequence = Simulation.NextCommandSequence(Setup.LocalPlayer);
    if (Session == 0 || AuthorityGeneration == 0 || !FirstSequence.has_value() ||
        Setup.FirstInputSequence == 0 || Setup.Builder == 0 ||
        Setup.Assistant == 0 || Setup.Builder == Setup.Assistant ||
        !EchoesTutorialConstructionDetail::IsLivingOwnedWorker(Simulation, Setup.LocalPlayer, Setup.Builder) ||
        !EchoesTutorialConstructionDetail::IsLivingOwnedWorker(Simulation, Setup.LocalPlayer, Setup.Assistant))
    {
        return false;
    }

    // The repair target is authored staging: one owned, completed Power Link
    // spawned below full health. Discovering it from authoritative state keeps
    // the lesson bound to the structure the scenario actually damaged rather
    // than to a coordinate this observation would have to be told twice.
    sim::EntityId DamagedCandidate = 0;
    std::int32_t DamagedHitPoints = 0;
    for (const sim::Entity& Entity : Simulation.Entities())
    {
        if (Entity.owner != Setup.LocalPlayer ||
            Entity.type != EchoesTutorialConstructionDetail::PowerLinkType || !Entity.completed ||
            Entity.hitPoints <= 0 || Entity.hitPoints >= Entity.maxHitPoints)
        {
            continue;
        }
        if (DamagedCandidate != 0)
        {
            // Two damaged Links cannot identify one repair target.
            return false;
        }
        DamagedCandidate = Entity.id;
        DamagedHitPoints = Entity.hitPoints;
    }
    if (DamagedCandidate == 0)
    {
        return false;
    }
    // Repairability is deliberately not a precondition here: the worker is
    // normally nowhere near the Link when the lesson opens, and requiring a
    // valid repair at open time would refuse to start the lesson that teaches
    // the player to walk over and repair it.

    BoundSetup = Setup;
    bActive = true;
    SessionId = Session;
    GenerationId = AuthorityGeneration;
    BeginTick = Simulation.CurrentTick();
    LastTick = BeginTick;
    LastInputSequence = Setup.FirstInputSequence - 1;
    DamagedLink = DamagedCandidate;
    RepairedHp = DamagedHitPoints;
    return true;
}

bool FEchoesTutorialConstructionObservation::ObserveRejectedPlacement(
    const FEchoesTutorialConstructionInput& Input,
    const sim::Simulation& Simulation,
    sim::Vec2 AttemptedSite)
{
    if (!ValidateEvent(Input, Simulation)) return false;
    // The refusal has to be the simulation's, not the tutorial's opinion. A
    // site the simulation would accept teaches the player the wrong rule.
    if (Simulation.ValidatePlacement(
            BoundSetup.LocalPlayer, EchoesTutorialConstructionDetail::PowerLinkType, AttemptedSite) ==
        sim::PlacementResult::Valid)
    {
        return false;
    }
    LastInputSequence = Input.InputSequence;
    RejectedInputSequence = Input.InputSequence;
    CurrentProgress.bRejectedPlacementObserved = true;
    // A fresh refusal needs its own acknowledgement: the explanation is the
    // lesson, so a stale acknowledgement must not answer a later mistake.
    CurrentProgress.bRejectionAcknowledged = false;
    return true;
}

bool FEchoesTutorialConstructionObservation::ObserveRejectionAcknowledged(
    const FEchoesTutorialConstructionInput& Input,
    const sim::Simulation& Simulation,
    uint64 InRejectedInputSequence)
{
    if (!ValidateEvent(Input, Simulation) ||
        !CurrentProgress.bRejectedPlacementObserved ||
        RejectedInputSequence == 0 ||
        InRejectedInputSequence != RejectedInputSequence)
    {
        return false;
    }
    LastInputSequence = Input.InputSequence;
    CurrentProgress.bRejectionAcknowledged = true;
    return true;
}

bool FEchoesTutorialConstructionObservation::ObserveAcceptedCommand(
    const FEchoesTutorialConstructionInput& Input,
    const sim::Simulation& Simulation)
{
    if (!ValidateEvent(Input, Simulation) || Input.CommandSequence == 0)
    {
        return false;
    }
    const sim::Command* Command = EchoesTutorialConstructionDetail::FindCommand(
        Simulation, BoundSetup.LocalPlayer, Input.CommandSequence);
    if (Command == nullptr ||
        !EchoesTutorialConstructionDetail::WasApplied(
            Simulation, BoundSetup.LocalPlayer, Input.CommandSequence))
    {
        return false;
    }

    // A placement: an untargeted Build of a Power Link by the bound builder.
    if (Command->type == sim::CommandType::Build && Command->target == 0 &&
        Command->actor == BoundSetup.Builder &&
        Command->buildType == EchoesTutorialConstructionDetail::PowerLinkType && BuildSequence == 0)
    {
        LastInputSequence = Input.InputSequence;
        BuildSequence = Input.CommandSequence;
        return true;
    }

    // An assist: a targeted Build by the second worker on the same site.
    if (Command->type == sim::CommandType::Build && Command->target != 0 &&
        Command->actor == BoundSetup.Assistant && Site != 0 &&
        Command->target == Site)
    {
        LastInputSequence = Input.InputSequence;
        AssistSequence = Input.CommandSequence;
        return true;
    }

    // A repair of the authored damaged Link by an owned worker.
    if (Command->type == sim::CommandType::Repair &&
        Command->target == DamagedLink && DamagedLink != 0 &&
        EchoesTutorialConstructionDetail::IsLivingOwnedWorker(
            Simulation, BoundSetup.LocalPlayer, Command->actor))
    {
        LastInputSequence = Input.InputSequence;
        RepairSequence = Input.CommandSequence;
        RepairWorker = Command->actor;
        RepairCommandTick = Simulation.CurrentTick();
        return true;
    }
    return false;
}

void FEchoesTutorialConstructionObservation::ObserveState(
    uint64 Session,
    uint64 AuthorityGeneration,
    const sim::Simulation& Simulation)
{
    if (!ValidateAuthority(Session, AuthorityGeneration, Simulation))
    {
        return;
    }
    const sim::Tick CurrentTick = Simulation.CurrentTick();
    if (CurrentTick < LastTick)
    {
        // A rollback replaces the state this evidence was drawn from.
        Reset();
        return;
    }
    LastTick = CurrentTick;

    // Bind the placed site once the placement command has been accepted. The
    // simulation spawns it incomplete at the commanded position, so the owned
    // incomplete Link at that position is the structure the player placed.
    if (BuildSequence != 0 && Site == 0)
    {
        const sim::Command* Placement = EchoesTutorialConstructionDetail::FindCommand(
            Simulation, BoundSetup.LocalPlayer, BuildSequence);
        if (Placement != nullptr)
        {
            for (const sim::Entity& Entity : Simulation.Entities())
            {
                if (Entity.owner == BoundSetup.LocalPlayer &&
                    Entity.type == EchoesTutorialConstructionDetail::PowerLinkType && !Entity.completed &&
                    Entity.hitPoints > 0 &&
                    Entity.position == Placement->position)
                {
                    Site = Entity.id;
                    CurrentProgress.bConstructionStarted = true;
                    break;
                }
            }
        }
    }

    if (Site != 0)
    {
        const sim::Entity* SiteEntity = Simulation.FindEntity(Site);
        if (SiteEntity == nullptr || SiteEntity->hitPoints <= 0)
        {
            // A cancelled or destroyed site is not a completed structure, and
            // its earlier facts cannot stand in for one that was finished.
            Site = 0;
            BuildSequence = 0;
            AssistSequence = 0;
            CurrentProgress.bConstructionStarted = false;
            CurrentProgress.bSimultaneousAssistObserved = false;
            CurrentProgress.bConstructionCompleted = false;
            CurrentProgress.bOperationalSelectionPublished = false;
            SelectionFrame = 0;
        }
        else
        {
            // Simultaneous is the point of the lesson: both workers building
            // the same unfinished site on the same authoritative tick.
            if (!SiteEntity->completed && AssistSequence != 0 &&
                EchoesTutorialConstructionDetail::IsBuildingSite(
                    Simulation, BoundSetup.LocalPlayer,
                    BoundSetup.Builder, Site) &&
                EchoesTutorialConstructionDetail::IsBuildingSite(
                    Simulation, BoundSetup.LocalPlayer,
                    BoundSetup.Assistant, Site))
            {
                CurrentProgress.bSimultaneousAssistObserved = true;
            }
            if (SiteEntity->completed)
            {
                CurrentProgress.bConstructionCompleted = true;
            }
        }
    }

    if (DamagedLink != 0 && RepairSequence != 0)
    {
        const sim::Entity* LinkEntity = Simulation.FindEntity(DamagedLink);
        if (LinkEntity == nullptr || LinkEntity->hitPoints <= 0)
        {
            // The authored repair target was lost; nothing can repair it now.
            DamagedLink = 0;
            RepairSequence = 0;
            CurrentProgress.bRepairCompleted = false;
        }
        else if (LinkEntity->hitPoints >= LinkEntity->maxHitPoints &&
            LinkEntity->hitPoints > RepairedHp)
        {
            // Restored to full, and higher than the health the lesson opened
            // on, so an already-whole structure cannot pass as a repair.
            CurrentProgress.bRepairCompleted = true;
        }
    }
}

bool FEchoesTutorialConstructionObservation::ObserveSelection(
    const FEchoesTutorialConstructionInput& Input,
    const sim::Simulation& Simulation,
    sim::EntityId SelectedStructure)
{
    if (!ValidateEvent(Input, Simulation) ||
        !CurrentProgress.bConstructionCompleted || Site == 0 ||
        SelectedStructure != Site || Input.PresentationFrame == 0)
    {
        return false;
    }
    LastInputSequence = Input.InputSequence;
    SelectionFrame = Input.PresentationFrame;
    return true;
}

bool FEchoesTutorialConstructionObservation::ObserveHudPublication(
    uint64 Session,
    uint64 AuthorityGeneration,
    uint64 PresentationFrame,
    const sim::Simulation& Simulation,
    const TArray<uint32>& SelectedIds,
    const FEchoesFieldHudView& PublishedView)
{
    if (!ValidateAuthority(Session, AuthorityGeneration, Simulation) ||
        SelectionFrame == 0 || PresentationFrame <= SelectionFrame ||
        !CurrentProgress.bConstructionCompleted || Site == 0)
    {
        // A later frame than the selection is required: the lesson claims the
        // player saw the finished structure described, and the frame that
        // carried the click was composed before that description existed.
        return false;
    }
    if (SelectedIds.Num() != 1 ||
        SelectedIds[0] != static_cast<uint32>(Site) ||
        !PublishedView.Selection.bVisible ||
        PublishedView.Selection.Entries.Num() != 1)
    {
        return false;
    }
    const FEchoesFieldHudSelectionEntry& Entry =
        PublishedView.Selection.Entries[0];
    if (Entry.EntityId != static_cast<uint32>(Site) || !Entry.bOwned ||
        !Entry.bStructure || Entry.Purpose.IsEmpty() ||
        Entry.MaxHitPoints <= 0)
    {
        return false;
    }
    CurrentProgress.bOperationalSelectionPublished = true;
    return true;
}
