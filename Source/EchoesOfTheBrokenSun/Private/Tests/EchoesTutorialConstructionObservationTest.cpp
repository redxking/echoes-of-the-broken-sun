#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTutorialConstructionObservation.h"

// A uniquely named namespace, not an anonymous one. Unreal batches several
// .cpp files into one unity translation unit, and every anonymous namespace in
// that unit is the SAME namespace: two file-local helpers sharing a signature
// become a redefinition, and a sibling's parameter shadows a constant here.
// Each file still compiles alone, so nothing catches it until the batch.
// Aliasing the same namespace in each batched file is a redeclaration,
// not a conflict, so this is safe at file scope where the bodies below
// can see it.
namespace sim = echoes::sim;

namespace EchoesTutorialConstructionObservationTestDetail
{

constexpr sim::PlayerId LocalPlayer = 0;
/** The Power Link archetype this lesson places, assists, completes and repairs. */
constexpr sim::EntityType PowerLink = sim::EntityType::Dropoff;
constexpr uint64 Session = 7;
constexpr uint64 Generation = 3;

FEchoesTutorialConstructionInput MakeInput(
    uint64 InputSequence,
    uint64 CommandSequence = 0)
{
    FEchoesTutorialConstructionInput Input;
    Input.Session = Session;
    Input.AuthorityGeneration = Generation;
    Input.InputSequence = InputSequence;
    Input.CommandSequence = CommandSequence;
    Input.Origin = EEchoesTutorialConstructionOrigin::PlayerInput;
    return Input;
}

/** A staged readiness map: an Anchor, two Surveyors and one damaged Link. */
struct FConstructionFixture final
{
    sim::Simulation Simulation{sim::SimulationConfig{32, 32, 20, 0xC0FFEEULL}};
    sim::EntityId Builder = 0;
    sim::EntityId Assistant = 0;
    sim::EntityId DamagedLink = 0;

    bool Build(FAutomationTestBase& Test)
    {
        if (!Test.TestTrue(TEXT("Local player joins"),
                Simulation.AddPlayer(
                    LocalPlayer, sim::Faction::MeridianCompact, {10000, 10000})))
        {
            return false;
        }
        Simulation.SpawnEntity(
            LocalPlayer, sim::Faction::MeridianCompact,
            sim::EntityType::CommandCore, sim::Vec2::FromTiles(10, 10));
        Builder = Simulation.SpawnEntity(
            LocalPlayer, sim::Faction::MeridianCompact,
            sim::EntityType::Worker, sim::Vec2::FromTiles(8, 13));
        Assistant = Simulation.SpawnEntity(
            LocalPlayer, sim::Faction::MeridianCompact,
            sim::EntityType::Worker, sim::Vec2::FromTiles(9, 13));
        // Spawned below full health, exactly as the readiness staging spawns
        // the Link the player is taught to repair.
        DamagedLink = Simulation.SpawnEntity(
            LocalPlayer, sim::Faction::MeridianCompact, PowerLink,
            sim::Vec2::FromTiles(6, 17), 440);
        return Test.TestTrue(TEXT("Construction fixture spawned"),
            Builder != 0 && Assistant != 0 && DamagedLink != 0);
    }

    FEchoesTutorialConstructionSetup Setup() const
    {
        FEchoesTutorialConstructionSetup Value;
        Value.LocalPlayer = LocalPlayer;
        Value.Builder = Builder;
        Value.Assistant = Assistant;
        Value.FirstInputSequence = 1;
        return Value;
    }
};
}  // namespace EchoesTutorialConstructionObservationTestDetail

/**
 * The Link lesson's observer. Its header shipped with no implementation
 * anywhere in the checkout, so lesson six had no predicate at all. These are
 * the refusals that matter: a placement the simulation would accept cannot
 * teach a rejection, a command with no Applied receipt cannot prove the player
 * built anything, and a replayed or programmatic origin cannot stand in for
 * the player acting.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesTutorialConstructionObservationTest,
    "Echoes.Runtime.Campaign.TutorialConstructionObservation",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesTutorialConstructionObservationTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    // --- Opening the lesson ------------------------------------------------
    {
        EchoesTutorialConstructionObservationTestDetail::FConstructionFixture Fixture;
        if (!Fixture.Build(*this)) return false;
        FEchoesTutorialConstructionObservation Observer;
        TestFalse(TEXT("A fresh observer is inactive"), Observer.IsActive());
        TestTrue(TEXT("A staged readiness force opens the lesson"),
            Observer.Begin(
                EchoesTutorialConstructionObservationTestDetail::Session, EchoesTutorialConstructionObservationTestDetail::Generation, Fixture.Simulation, Fixture.Setup()));
        TestTrue(TEXT("The lesson is active once opened"), Observer.IsActive());
        TestTrue(TEXT("The damaged Link is bound as the repair target"),
            Observer.RepairTarget() == Fixture.DamagedLink);
        TestFalse(TEXT("Opening the lesson earns nothing"),
            Observer.Progress().PredicateSatisfied());

        FEchoesTutorialConstructionSetup Same = Fixture.Setup();
        Same.Assistant = Same.Builder;
        FEchoesTutorialConstructionObservation Rejecting;
        TestFalse(TEXT("One worker cannot be both builder and assistant"),
            Rejecting.Begin(EchoesTutorialConstructionObservationTestDetail::Session, EchoesTutorialConstructionObservationTestDetail::Generation, Fixture.Simulation, Same));
        FEchoesTutorialConstructionSetup NoSession = Fixture.Setup();
        TestFalse(TEXT("An unidentified session cannot open the lesson"),
            Rejecting.Begin(0, EchoesTutorialConstructionObservationTestDetail::Generation, Fixture.Simulation, NoSession));
    }

    // --- A rejection has to be the simulation's ----------------------------
    {
        EchoesTutorialConstructionObservationTestDetail::FConstructionFixture Fixture;
        if (!Fixture.Build(*this)) return false;
        FEchoesTutorialConstructionObservation Observer;
        if (!TestTrue(TEXT("Rejection fixture opens"),
                Observer.Begin(
                    EchoesTutorialConstructionObservationTestDetail::Session, EchoesTutorialConstructionObservationTestDetail::Generation, Fixture.Simulation, Fixture.Setup())))
        {
            return false;
        }

        // Open ground the simulation would accept teaches the player nothing
        // about why a placement fails, so it is refused as a lesson event.
        const sim::Vec2 OpenGround = sim::Vec2::FromTiles(20, 20);
        TestTrue(TEXT("The fixture's open ground really is placeable"),
            Fixture.Simulation.ValidatePlacement(
                EchoesTutorialConstructionObservationTestDetail::LocalPlayer, EchoesTutorialConstructionObservationTestDetail::PowerLink, OpenGround) ==
                sim::PlacementResult::Valid);
        TestFalse(TEXT("A placement the simulation would accept is not a rejection"),
            Observer.ObserveRejectedPlacement(
                EchoesTutorialConstructionObservationTestDetail::MakeInput(1), Fixture.Simulation, OpenGround));
        TestFalse(TEXT("No rejection was recorded"),
            Observer.Progress().bRejectedPlacementObserved);

        // The Anchor's own footprint is occupied, so the simulation refuses it.
        const sim::Vec2 Occupied = sim::Vec2::FromTiles(10, 10);
        TestTrue(TEXT("The fixture's occupied tile really is refused"),
            Fixture.Simulation.ValidatePlacement(
                EchoesTutorialConstructionObservationTestDetail::LocalPlayer, EchoesTutorialConstructionObservationTestDetail::PowerLink, Occupied) !=
                sim::PlacementResult::Valid);
        TestTrue(TEXT("A placement the simulation refuses is a rejection"),
            Observer.ObserveRejectedPlacement(
                EchoesTutorialConstructionObservationTestDetail::MakeInput(2), Fixture.Simulation, Occupied));
        TestTrue(TEXT("The rejection is recorded"),
            Observer.Progress().bRejectedPlacementObserved);
        TestFalse(TEXT("A rejection is not yet an explanation the player read"),
            Observer.Progress().bRejectionAcknowledged);

        TestFalse(TEXT("Acknowledging some other attempt proves nothing"),
            Observer.ObserveRejectionAcknowledged(
                EchoesTutorialConstructionObservationTestDetail::MakeInput(3), Fixture.Simulation, 99));
        TestTrue(TEXT("Acknowledging this rejection is recorded"),
            Observer.ObserveRejectionAcknowledged(
                EchoesTutorialConstructionObservationTestDetail::MakeInput(4), Fixture.Simulation, 2));
        TestTrue(TEXT("The acknowledgement is recorded"),
            Observer.Progress().bRejectionAcknowledged);

        // A later mistake needs its own explanation.
        TestTrue(TEXT("A second refused placement is observed"),
            Observer.ObserveRejectedPlacement(
                EchoesTutorialConstructionObservationTestDetail::MakeInput(5), Fixture.Simulation, Occupied));
        TestFalse(TEXT("A stale acknowledgement does not answer a new mistake"),
            Observer.Progress().bRejectionAcknowledged);
    }

    // --- Provenance and receipts -------------------------------------------
    {
        EchoesTutorialConstructionObservationTestDetail::FConstructionFixture Fixture;
        if (!Fixture.Build(*this)) return false;
        FEchoesTutorialConstructionObservation Observer;
        if (!TestTrue(TEXT("Provenance fixture opens"),
                Observer.Begin(
                    EchoesTutorialConstructionObservationTestDetail::Session, EchoesTutorialConstructionObservationTestDetail::Generation, Fixture.Simulation, Fixture.Setup())))
        {
            return false;
        }

        const sim::Vec2 Occupied = sim::Vec2::FromTiles(10, 10);
        FEchoesTutorialConstructionInput Replayed = EchoesTutorialConstructionObservationTestDetail::MakeInput(1);
        Replayed.Origin = EEchoesTutorialConstructionOrigin::Replay;
        TestFalse(TEXT("A replayed origin cannot teach a lesson"),
            Observer.ObserveRejectedPlacement(
                Replayed, Fixture.Simulation, Occupied));
        FEchoesTutorialConstructionInput Programmatic = EchoesTutorialConstructionObservationTestDetail::MakeInput(1);
        Programmatic.Origin =
            EEchoesTutorialConstructionOrigin::Programmatic;
        TestFalse(TEXT("A programmatic origin cannot teach a lesson"),
            Observer.ObserveRejectedPlacement(
                Programmatic, Fixture.Simulation, Occupied));
        FEchoesTutorialConstructionInput Unknown = EchoesTutorialConstructionObservationTestDetail::MakeInput(1);
        Unknown.Origin = EEchoesTutorialConstructionOrigin::Unknown;
        TestFalse(TEXT("An unattributed origin cannot teach a lesson"),
            Observer.ObserveRejectedPlacement(
                Unknown, Fixture.Simulation, Occupied));

        // Input sequences must advance, so one action cannot be counted twice.
        TestTrue(TEXT("The first player rejection is observed"),
            Observer.ObserveRejectedPlacement(
                EchoesTutorialConstructionObservationTestDetail::MakeInput(5), Fixture.Simulation, Occupied));
        TestFalse(TEXT("A replayed input sequence is refused"),
            Observer.ObserveRejectedPlacement(
                EchoesTutorialConstructionObservationTestDetail::MakeInput(5), Fixture.Simulation, Occupied));
        TestFalse(TEXT("An older input sequence is refused"),
            Observer.ObserveRejectedPlacement(
                EchoesTutorialConstructionObservationTestDetail::MakeInput(4), Fixture.Simulation, Occupied));

        // A command with no resolution receipt proves nothing happened.
        TestFalse(TEXT("An unknown command sequence is refused"),
            Observer.ObserveAcceptedCommand(
                EchoesTutorialConstructionObservationTestDetail::MakeInput(6, 4242), Fixture.Simulation));
        TestFalse(TEXT("A command sequence of zero is refused"),
            Observer.ObserveAcceptedCommand(
                EchoesTutorialConstructionObservationTestDetail::MakeInput(7, 0), Fixture.Simulation));
        TestFalse(TEXT("Nothing was constructed"),
            Observer.Progress().bConstructionStarted);

        // A different session or authority generation is a different scenario.
        FEchoesTutorialConstructionInput OtherSession = EchoesTutorialConstructionObservationTestDetail::MakeInput(8);
        OtherSession.Session = EchoesTutorialConstructionObservationTestDetail::Session + 1;
        TestFalse(TEXT("Another session's event is refused"),
            Observer.ObserveRejectedPlacement(
                OtherSession, Fixture.Simulation, Occupied));
        FEchoesTutorialConstructionInput OtherGeneration = EchoesTutorialConstructionObservationTestDetail::MakeInput(9);
        OtherGeneration.AuthorityGeneration = EchoesTutorialConstructionObservationTestDetail::Generation + 1;
        TestFalse(TEXT("Another authority generation's event is refused"),
            Observer.ObserveRejectedPlacement(
                OtherGeneration, Fixture.Simulation, Occupied));
    }

    // --- Repair is measured against the health the lesson opened on --------
    {
        EchoesTutorialConstructionObservationTestDetail::FConstructionFixture Fixture;
        if (!Fixture.Build(*this)) return false;
        FEchoesTutorialConstructionObservation Observer;
        if (!TestTrue(TEXT("Repair fixture opens"),
                Observer.Begin(
                    EchoesTutorialConstructionObservationTestDetail::Session, EchoesTutorialConstructionObservationTestDetail::Generation, Fixture.Simulation, Fixture.Setup())))
        {
            return false;
        }
        Observer.ObserveState(EchoesTutorialConstructionObservationTestDetail::Session, EchoesTutorialConstructionObservationTestDetail::Generation, Fixture.Simulation);
        TestFalse(TEXT("An unrepaired Link is not a completed repair"),
            Observer.Progress().bRepairCompleted);
        TestFalse(TEXT("The whole Link lesson is unearned"),
            Observer.Progress().PredicateSatisfied());
    }

    // --- A lesson without its staged repair target cannot open -------------
    {
        sim::Simulation Bare{sim::SimulationConfig{32, 32, 20, 0xBEEFULL}};
        TestTrue(TEXT("Bare local player joins"), Bare.AddPlayer(
            EchoesTutorialConstructionObservationTestDetail::LocalPlayer, sim::Faction::MeridianCompact, {10000, 10000}));
        const sim::EntityId Builder = Bare.SpawnEntity(
            EchoesTutorialConstructionObservationTestDetail::LocalPlayer, sim::Faction::MeridianCompact,
            sim::EntityType::Worker, sim::Vec2::FromTiles(8, 13));
        const sim::EntityId Assistant = Bare.SpawnEntity(
            EchoesTutorialConstructionObservationTestDetail::LocalPlayer, sim::Faction::MeridianCompact,
            sim::EntityType::Worker, sim::Vec2::FromTiles(9, 13));
        // A Link at full health is not the damaged one the lesson repairs.
        Bare.SpawnEntity(
            EchoesTutorialConstructionObservationTestDetail::LocalPlayer, sim::Faction::MeridianCompact, EchoesTutorialConstructionObservationTestDetail::PowerLink,
            sim::Vec2::FromTiles(6, 17));
        FEchoesTutorialConstructionSetup Setup;
        Setup.LocalPlayer = EchoesTutorialConstructionObservationTestDetail::LocalPlayer;
        Setup.Builder = Builder;
        Setup.Assistant = Assistant;
        Setup.FirstInputSequence = 1;
        FEchoesTutorialConstructionObservation Observer;
        TestFalse(TEXT("Without a damaged Link the lesson refuses to open"),
            Observer.Begin(EchoesTutorialConstructionObservationTestDetail::Session, EchoesTutorialConstructionObservationTestDetail::Generation, Bare, Setup));
        TestFalse(TEXT("A refused lesson stays inactive"), Observer.IsActive());
    }

    return true;
}

#endif
