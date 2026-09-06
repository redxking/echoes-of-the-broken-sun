#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTutorialSelectionObservation.h"
#include "EchoesFieldHudView.h"

namespace
{
using echoes::sim::EntityId;
using echoes::sim::EntityType;
using echoes::sim::Faction;
using echoes::sim::PlayerView;
using echoes::sim::Simulation;
using echoes::sim::SimulationConfig;
using echoes::sim::Vec2;

FEchoesTutorialSelectionEvent Input(
    uint64 Session,
    uint64 Sequence,
    EEchoesTutorialSelectionInputEvent Event)
{
    FEchoesTutorialSelectionEvent Result;
    Result.Session = Session;
    Result.Sequence = Sequence;
    Result.Origin = EEchoesTutorialSelectionInputOrigin::PlayerInput;
    Result.Event = Event;
    Result.PresentationFrame = Sequence;
    return Result;
}

FEchoesFieldHudView PublishedRosterView(
    const PlayerView& View,
    EntityId Surveyor)
{
    FEchoesFieldHudView Result =
        FEchoesFieldHudModel::BuildPlayerScoped(View, {Surveyor}, false);
    Result.Commands.bVisible = true;
    FEchoesFieldHudControl Stop;
    Stop.Label = FText::FromString(TEXT("STOP"));
    Stop.Action = EEchoesFieldHudAction::CommandDeck;
    Stop.Argument = static_cast<int32>(EEchoesCommandDeckAction::Stop);
    Result.Commands.Controls.Add(MoveTemp(Stop));
    return Result;
}

struct FSelectionFixture final
{
    FSelectionFixture()
    {
        SimulationConfig Config;
        Config.mapWidthTiles = 16;
        Config.mapHeightTiles = 16;
        Sim = Simulation(Config);
        Sim.AddPlayer(0, Faction::MeridianCompact, {500, 30});
        Sim.AddPlayer(1, Faction::KharuunAssemblies, {500, 30});
        Surveyor = Sim.SpawnEntity(
            0, Faction::MeridianCompact, EntityType::Worker,
            Vec2::FromTiles(2, 2));
        Lancer = Sim.SpawnEntity(
            0, Faction::MeridianCompact, EntityType::Soldier,
            Vec2::FromTiles(3, 2));
        Relay = Sim.SpawnEntity(
            0, Faction::MeridianCompact, EntityType::ScoutUnit,
            Vec2::FromTiles(4, 2));
        Enemy = Sim.SpawnEntity(
            1, Faction::KharuunAssemblies, EntityType::Soldier,
            Vec2::FromTiles(5, 2));
    }

    [[nodiscard]] std::optional<PlayerView> View() const
    {
        return Sim.CreatePlayerView(0);
    }

    Simulation Sim;
    EntityId Surveyor = 0;
    EntityId Lancer = 0;
    EntityId Relay = 0;
    EntityId Enemy = 0;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesTutorialSelectionObservationTest,
    "Echoes.Runtime.Campaign.TutorialSelectionObservation",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesTutorialSelectionObservationTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    FSelectionFixture Fixture;
    const std::optional<PlayerView> InitialView = Fixture.View();
    if (!TestTrue(TEXT("Simulation produces the scoped lesson-opening view"),
            InitialView.has_value()))
    {
        return false;
    }

    FEchoesTutorialSelectionObservation Observer;
    TestFalse(TEXT("A hostile cannot be staged as the Roster Surveyor"),
        Observer.BeginRoster(1, *InitialView, Fixture.Enemy));
    TestFalse(TEXT("A non-Worker cannot be staged as the Roster Surveyor"),
        Observer.BeginRoster(1, *InitialView, Fixture.Lancer));
    TestTrue(TEXT("Roster opens from the live owned Surveyor"),
        Observer.BeginRoster(2, *InitialView, Fixture.Surveyor));
    TestFalse(TEXT("Opening and elapsed time do not supply selection input"),
        Observer.RosterProgress().PredicateSatisfied());

    const TArray<EntityId> SurveyorSelection{Fixture.Surveyor};
    FEchoesTutorialSelectionEvent Replayed = Input(
        2, 1, EEchoesTutorialSelectionInputEvent::SingleClickSelection);
    Replayed.Origin = EEchoesTutorialSelectionInputOrigin::Replay;
    Observer.Observe(Replayed, *InitialView, SurveyorSelection);
    FEchoesTutorialSelectionEvent Programmatic = Input(
        2, 2, EEchoesTutorialSelectionInputEvent::SingleClickSelection);
    Programmatic.Origin = EEchoesTutorialSelectionInputOrigin::Programmatic;
    Observer.Observe(Programmatic, *InitialView, SurveyorSelection);
    TestFalse(TEXT("Replay and programmatic selection cannot satisfy Roster"),
        Observer.RosterProgress().bSingleClickSelected);

    const TArray<EntityId> EmptySelection;
    Observer.Observe(
        Input(2, 3, EEchoesTutorialSelectionInputEvent::SingleClickSelection),
        *InitialView,
        EmptySelection);
    const TArray<EntityId> HostileSelection{Fixture.Enemy};
    Observer.Observe(
        Input(2, 4, EEchoesTutorialSelectionInputEvent::SingleClickSelection),
        *InitialView,
        HostileSelection);
    TestFalse(TEXT("Empty and wrong-owner clicks cannot fake Roster selection"),
        Observer.RosterProgress().bSingleClickSelected);

    Observer.Observe(
        Input(99, 5, EEchoesTutorialSelectionInputEvent::SingleClickSelection),
        *InitialView,
        SurveyorSelection);
    TestFalse(TEXT("A crossed session cannot satisfy Roster"),
        Observer.RosterProgress().bSingleClickSelected);
    Observer.Observe(
        Input(2, 5, EEchoesTutorialSelectionInputEvent::SingleClickSelection),
        *InitialView,
        SurveyorSelection);
    TestTrue(TEXT("An exact live single-click selects the staged Surveyor"),
        Observer.RosterProgress().bSingleClickSelected);

    Observer.Observe(
        Input(2, 5, EEchoesTutorialSelectionInputEvent::TerrainClear),
        *InitialView,
        EmptySelection);
    TestFalse(TEXT("A stale event cannot supply the terrain clear"),
        Observer.RosterProgress().bTerrainCleared);
    FEchoesTutorialSelectionEvent RapidClear = Input(
        2, 6, EEchoesTutorialSelectionInputEvent::TerrainClear);
    RapidClear.PresentationFrame = 5;
    Observer.Observe(RapidClear, *InitialView, EmptySelection);
    TestFalse(TEXT("Select and clear in one presentation frame cannot satisfy Roster"),
        Observer.RosterProgress().bTerrainCleared);
    FEchoesTutorialSelectionEvent SameFramePublication = Input(
        2, 7, EEchoesTutorialSelectionInputEvent::RosterHudPublished);
    SameFramePublication.PresentationFrame = 5;
    Observer.ObserveRosterHudPublication(SameFramePublication, *InitialView,
        SurveyorSelection, PublishedRosterView(*InitialView, Fixture.Surveyor));
    TestFalse(TEXT("A same-frame HUD snapshot is not a render opportunity"),
        Observer.RosterProgress().bHudPublished);
    FEchoesTutorialSelectionEvent Publication = Input(
        2, 8, EEchoesTutorialSelectionInputEvent::RosterHudPublished);
    Observer.ObserveRosterHudPublication(Publication, *InitialView,
        SurveyorSelection, PublishedRosterView(*InitialView, Fixture.Surveyor));
    TestTrue(TEXT("A later field-HUD publication contains purpose, health, order and controls"),
        Observer.RosterProgress().bHudPublished);
    Observer.Observe(
        Input(2, 9, EEchoesTutorialSelectionInputEvent::TerrainClear),
        *InitialView,
        SurveyorSelection);
    TestFalse(TEXT("Terrain clear requires the actual selection to be empty"),
        Observer.RosterProgress().bTerrainCleared);
    Observer.Observe(
        Input(2, 10, EEchoesTutorialSelectionInputEvent::TerrainClear),
        *InitialView,
        EmptySelection);
    TestTrue(TEXT("Roster derives both required selection components"),
        Observer.RosterProgress().PredicateSatisfied());

    TestTrue(TEXT("A new session starts a clean retry"),
        Observer.BeginRoster(3, *InitialView, Fixture.Surveyor));
    TestFalse(TEXT("Retry does not inherit prior Roster progress"),
        Observer.RosterProgress().PredicateSatisfied());
    Observer.Observe(
        Input(3, 1, EEchoesTutorialSelectionInputEvent::TerrainClear),
        *InitialView,
        EmptySelection);
    TestFalse(TEXT("Clearing before selection cannot complete a retry"),
        Observer.RosterProgress().bTerrainCleared);
    Observer.Observe(
        Input(3, 2, EEchoesTutorialSelectionInputEvent::SingleClickSelection),
        *InitialView,
        SurveyorSelection);
    FEchoesTutorialSelectionEvent RetryPublication = Input(
        3, 3, EEchoesTutorialSelectionInputEvent::RosterHudPublished);
    Observer.ObserveRosterHudPublication(RetryPublication, *InitialView,
        SurveyorSelection, PublishedRosterView(*InitialView, Fixture.Surveyor));
    Observer.Observe(
        Input(3, 4, EEchoesTutorialSelectionInputEvent::TerrainClear),
        *InitialView,
        EmptySelection);
    TestTrue(TEXT("The retried Roster attempt can complete normally"),
        Observer.RosterProgress().PredicateSatisfied());

    const TArray<EntityId> EmptyStage;
    TestFalse(TEXT("Muster rejects an empty fake stage"),
        Observer.BeginMuster(10, *InitialView, EmptyStage));
    const TArray<EntityId> SameTypeStage{Fixture.Surveyor};
    TestFalse(TEXT("Muster rejects a subgroup-incapable stage"),
        Observer.BeginMuster(10, *InitialView, SameTypeStage));
    const TArray<EntityId> WrongOwnerStage{Fixture.Surveyor, Fixture.Enemy};
    TestFalse(TEXT("Muster rejects a stage containing a wrong-owner unit"),
        Observer.BeginMuster(10, *InitialView, WrongOwnerStage));

    const TArray<EntityId> StagedSection{Fixture.Surveyor, Fixture.Lancer};
    TestTrue(TEXT("Muster opens from an owned alive mixed mobile section"),
        Observer.BeginMuster(11, *InitialView, StagedSection));
    TestFalse(TEXT("No input cannot satisfy any Muster component"),
        Observer.MusterProgress().PredicateSatisfied());

    const TArray<EntityId> PartialDrag{Fixture.Surveyor};
    Observer.Observe(
        Input(11, 1, EEchoesTutorialSelectionInputEvent::DragSelection),
        *InitialView,
        PartialDrag);
    TestFalse(TEXT("A partial drag does not include the staged section"),
        Observer.MusterProgress().bDragSelected);
    const TArray<EntityId> WrongOwnerDrag{
        Fixture.Surveyor, Fixture.Lancer, Fixture.Enemy};
    Observer.Observe(
        Input(11, 2, EEchoesTutorialSelectionInputEvent::DragSelection),
        *InitialView,
        WrongOwnerDrag);
    TestFalse(TEXT("A drag containing a wrong-owner entity is rejected"),
        Observer.MusterProgress().bDragSelected);
    Observer.Observe(
        Input(11, 3, EEchoesTutorialSelectionInputEvent::DragSelection),
        *InitialView,
        StagedSection);
    TestTrue(TEXT("A drag including every staged mobile unit is observed"),
        Observer.MusterProgress().bDragSelected);

    Observer.Observe(
        Input(11, 3, EEchoesTutorialSelectionInputEvent::SelectionModified),
        *InitialView,
        PartialDrag);
    TestFalse(TEXT("A stale reordered event cannot modify progress"),
        Observer.MusterProgress().bSelectionModified);
    Observer.Observe(
        Input(11, 4, EEchoesTutorialSelectionInputEvent::SelectionModified),
        *InitialView,
        StagedSection);
    TestFalse(TEXT("An unchanged selection is not a modification"),
        Observer.MusterProgress().bSelectionModified);
    const TArray<EntityId> ModifiedSection{
        Fixture.Surveyor, Fixture.Lancer, Fixture.Relay};
    Observer.Observe(
        Input(11, 5, EEchoesTutorialSelectionInputEvent::SelectionModified),
        *InitialView,
        ModifiedSection);
    TestTrue(TEXT("A live changed selection supplies the modify component"),
        Observer.MusterProgress().bSelectionModified);

    FEchoesTutorialSelectionEvent NoSubgroupChange = Input(
        11, 6, EEchoesTutorialSelectionInputEvent::SubgroupChanged);
    NoSubgroupChange.PreviousSubgroupType = EntityType::Worker;
    NoSubgroupChange.ActiveSubgroupType = EntityType::Worker;
    Observer.Observe(NoSubgroupChange, *InitialView, ModifiedSection);
    TestFalse(TEXT("Reporting the same subgroup is not a subgroup change"),
        Observer.MusterProgress().bSubgroupChanged);
    FEchoesTutorialSelectionEvent SubgroupChange = Input(
        11, 7, EEchoesTutorialSelectionInputEvent::SubgroupChanged);
    SubgroupChange.PreviousSubgroupType = EntityType::Worker;
    SubgroupChange.ActiveSubgroupType = EntityType::Soldier;
    Observer.Observe(SubgroupChange, *InitialView, ModifiedSection);
    TestTrue(TEXT("Muster observes a real change between represented subgroups"),
        Observer.MusterProgress().bSubgroupChanged);

    FEchoesTutorialSelectionEvent Assignment = Input(
        11, 8, EEchoesTutorialSelectionInputEvent::ControlGroupAssigned);
    Assignment.ControlGroupIndex = 2;
    Observer.Observe(
        Assignment, *InitialView, ModifiedSection, ModifiedSection);
    TestTrue(TEXT("The saved group must exactly match the assigned selection"),
        Observer.MusterProgress().bControlGroupAssigned);

    FEchoesTutorialSelectionEvent WrongRecall = Input(
        11, 9, EEchoesTutorialSelectionInputEvent::ControlGroupRecalled);
    WrongRecall.ControlGroupIndex = 3;
    Observer.Observe(
        WrongRecall, *InitialView, ModifiedSection, ModifiedSection);
    TestFalse(TEXT("A different control group cannot satisfy recall"),
        Observer.MusterProgress().bControlGroupRecalled);
    const TArray<EntityId> MismatchedSavedGroup{
        Fixture.Surveyor, Fixture.Lancer};
    FEchoesTutorialSelectionEvent MismatchedRecall = Input(
        11, 10, EEchoesTutorialSelectionInputEvent::ControlGroupRecalled);
    MismatchedRecall.ControlGroupIndex = 2;
    Observer.Observe(
        MismatchedRecall,
        *InitialView,
        ModifiedSection,
        MismatchedSavedGroup);
    TestFalse(TEXT("Recall cannot substitute a different saved selection"),
        Observer.MusterProgress().bControlGroupRecalled);
    FEchoesTutorialSelectionEvent Recall = Input(
        11, 11, EEchoesTutorialSelectionInputEvent::ControlGroupRecalled);
    Recall.ControlGroupIndex = 2;
    Observer.Observe(Recall, *InitialView, ModifiedSection, ModifiedSection);
    TestTrue(TEXT("Muster derives every authored selection component"),
        Observer.MusterProgress().PredicateSatisfied());

    TestTrue(TEXT("A second Muster attempt opens before a staged loss"),
        Observer.BeginMuster(12, *InitialView, StagedSection));
    if (echoes::sim::Entity* Lost =
            Fixture.Sim.MutableEntityForTesting(Fixture.Lancer))
    {
        Lost->hitPoints = 0;
    }
    const std::optional<PlayerView> LostView = Fixture.View();
    if (!TestTrue(TEXT("Simulation produces a view containing the staged loss"),
            LostView.has_value()))
    {
        return false;
    }
    Observer.Observe(
        Input(12, 1, EEchoesTutorialSelectionInputEvent::DragSelection),
        *LostView,
        StagedSection);
    TestFalse(TEXT("A dead staged entity invalidates the attempt"),
        Observer.IsActive());

    const EntityId Replacement = Fixture.Sim.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Soldier,
        Vec2::FromTiles(6, 2));
    const std::optional<PlayerView> RetryView = Fixture.View();
    if (!TestTrue(TEXT("Simulation produces the restaged retry view"),
            RetryView.has_value()))
    {
        return false;
    }
    const TArray<EntityId> RestagedSection{Fixture.Surveyor, Replacement};
    TestTrue(TEXT("Muster can retry with a new live staged section"),
        Observer.BeginMuster(13, *RetryView, RestagedSection));
    Observer.Observe(
        Input(13, 1, EEchoesTutorialSelectionInputEvent::DragSelection),
        *RetryView,
        RestagedSection);
    TestTrue(TEXT("The retry earns progress only from its new session"),
        Observer.MusterProgress().bDragSelected);

    return true;
}

#endif
