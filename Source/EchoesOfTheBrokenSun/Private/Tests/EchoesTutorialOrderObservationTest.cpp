#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTutorialOrderObservation.h"

namespace
{
namespace sim = echoes::sim;

sim::Command MakeCommand(
    const sim::Simulation& Simulation,
    sim::PlayerId Player,
    std::uint64_t Sequence,
    sim::CommandType Type,
    sim::EntityId Actor)
{
    sim::Command Command{};
    Command.executeTick = Simulation.CurrentTick();
    Command.player = Player;
    Command.sequence = Sequence;
    Command.type = Type;
    Command.actor = Actor;
    return Command;
}

bool QueueStepAndObserve(
    FAutomationTestBase& Test,
    sim::Simulation& Simulation,
    FEchoesTutorialOrderObservation& Observer,
    uint64 Session,
    uint64 SnapshotToken,
    sim::Command Command,
    EEchoesTutorialOrderCommandOrigin Origin)
{
    std::string Error;
    if (!Simulation.QueueCommand(Command, &Error))
    {
        Test.AddError(FString::Printf(
            TEXT("Command was not structurally accepted: %s"),
            UTF8_TO_TCHAR(Error.c_str())));
        return false;
    }
    Simulation.Step();
    Observer.ObserveAcceptedCommand(
        Session, SnapshotToken, Simulation, Command.sequence, Origin);
    return true;
}

FEchoesTutorialRouteSetup MakeRouteSetup(
    sim::EntityId Mover,
    sim::EntityId Guard,
    sim::EntityId Worker,
    sim::EntityId Node)
{
    FEchoesTutorialRouteSetup Setup;
    Setup.LocalPlayer = 0;
    Setup.FirstInputAttemptSequence = 1;
    Setup.MoveActor = Mover;
    Setup.MoveDestination = sim::Vec2::FromTiles(9, 5);
    Setup.MoveInputToleranceRaw = sim::kFixedScale / 2;
    Setup.MoveArrivalToleranceRaw = 0;
    Setup.StopActor = Mover;
    Setup.PatrolActor = Mover;
    Setup.PatrolDestination = sim::Vec2::FromTiles(13, 5);
    Setup.PatrolInputToleranceRaw = sim::kFixedScale / 2;
    Setup.GuardActor = Guard;
    Setup.GuardTarget = Worker;
    Setup.ContextAction.Type = sim::CommandType::Gather;
    Setup.ContextAction.Actor = Worker;
    Setup.ContextAction.Target = Node;
    Setup.RejectedAction.Type = sim::CommandType::Move;
    Setup.RejectedAction.Actor = Mover;
    Setup.RejectedAction.Position = sim::Vec2::FromTiles(7, 6);
    Setup.RejectedInputToleranceRaw = sim::kFixedScale / 2;
    Setup.ExpectedRejectionOutcome =
        sim::CommandResolutionOutcome::DestinationOccupied;
    return Setup;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesTutorialOrderObservationTest,
    "Echoes.Runtime.Campaign.TutorialOrderObservation",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesTutorialOrderObservationTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    namespace sim = echoes::sim;

    {
        sim::Simulation Simulation(sim::SimulationConfig{32, 32, 20, 0x704EULL});
        TestTrue(TEXT("Route local player joins"), Simulation.AddPlayer(
            0, sim::Faction::MeridianCompact, {10000, 100}));
        TestTrue(TEXT("Route opponent joins"), Simulation.AddPlayer(
            1, sim::Faction::KharuunAssemblies, {10000, 100}));
        const sim::EntityId Mover = Simulation.SpawnEntity(
            0, sim::Faction::MeridianCompact, sim::EntityType::Soldier,
            sim::Vec2::FromTiles(5, 5));
        const sim::EntityId Guard = Simulation.SpawnEntity(
            0, sim::Faction::MeridianCompact, sim::EntityType::Soldier,
            sim::Vec2::FromTiles(6, 8));
        const sim::EntityId Worker = Simulation.SpawnEntity(
            0, sim::Faction::MeridianCompact, sim::EntityType::Worker,
            sim::Vec2::FromTiles(9, 8));
        const sim::EntityId Core = Simulation.SpawnEntity(
            0, sim::Faction::MeridianCompact, sim::EntityType::CommandCore,
            sim::Vec2::FromTiles(8, 10));
        const sim::EntityId Node = Simulation.SpawnResourceNode(
            sim::Vec2::FromTiles(10, 8), 1000);
        const sim::EntityId Opponent = Simulation.SpawnEntity(
            1, sim::Faction::KharuunAssemblies, sim::EntityType::Soldier,
            sim::Vec2::FromTiles(24, 24));
        const sim::EntityId OpponentCore = Simulation.SpawnEntity(
            1, sim::Faction::KharuunAssemblies, sim::EntityType::CommandCore,
            sim::Vec2::FromTiles(26, 26));
        TestTrue(TEXT("Route fixture spawned"),
            Mover && Guard && Worker && Core && Node && Opponent && OpponentCore);
        TestTrue(TEXT("Authored rejection fixture uses known blocked ground"),
            Simulation.SetTerrainTile(7, 6, sim::Terrain::Blocked));

        // A resolved command from before Begin is inside the snapshot but
        // outside the lesson sequence frontier.
        sim::Command OldMove = MakeCommand(
            Simulation, 0, 1, sim::CommandType::Move, Mover);
        OldMove.position = sim::Vec2::FromTiles(6, 5);
        TestTrue(TEXT("Old move queues"), Simulation.QueueCommand(OldMove));
        Simulation.Step();
        for (int32 Tick = 0;
             Tick < 200 &&
                 Simulation.FindEntity(Mover)->order.type != sim::OrderType::None;
             ++Tick)
            Simulation.Step();
        TestEqual(TEXT("Old move reaches its destination"),
            Simulation.FindEntity(Mover)->order.type, sim::OrderType::None);

        constexpr uint64 RouteSession = 71;
        constexpr uint64 RouteSnapshot = 7001;
        FEchoesTutorialOrderObservation Observer;
        const FEchoesTutorialRouteSetup Setup =
            MakeRouteSetup(Mover, Guard, Worker, Node);
        FEchoesTutorialRouteSetup WrongRejectionSetup = Setup;
        WrongRejectionSetup.ExpectedRejectionOutcome =
            sim::CommandResolutionOutcome::NoPath;
        FEchoesTutorialOrderObservation WrongRejectionObserver;
        TestFalse(TEXT("Route setup fails closed on a mismatched rejection reason"),
            WrongRejectionObserver.BeginRoute(
                RouteSession, RouteSnapshot, Simulation,
                WrongRejectionSetup));
        TestTrue(TEXT("Authored Route setup begins"), Observer.BeginRoute(
            RouteSession, RouteSnapshot, Simulation, Setup));
        Observer.ObserveAcceptedCommand(
            RouteSession, RouteSnapshot, Simulation, 1,
            EEchoesTutorialOrderCommandOrigin::DirectPlayerCommand);
        TestFalse(TEXT("A pre-lesson command cannot grant Route"),
            Observer.RouteSimulationPredicateSatisfied());

        sim::Command OpponentMove = MakeCommand(
            Simulation, 1, 99, sim::CommandType::Move, Opponent);
        OpponentMove.position = sim::Vec2::FromTiles(23, 24);
        TestTrue(TEXT("Opponent move queues"), Simulation.QueueCommand(OpponentMove));
        Simulation.Step();
        Observer.ObserveState(RouteSession, RouteSnapshot, Simulation);
        Observer.ObserveAcceptedCommand(
            RouteSession, RouteSnapshot, Simulation, 99,
            EEchoesTutorialOrderCommandOrigin::DirectPlayerCommand);
        TestFalse(TEXT("Another player's sequence cannot grant Route"),
            Observer.RouteSimulationPredicateSatisfied());

        // Missing receipts are pending evidence. Resolving the same accepted
        // command later is the first point at which it may advance.
        sim::Command Move = MakeCommand(
            Simulation, 0, 2, sim::CommandType::Move, Mover);
        Move.executeTick += 2;
        Move.position = sim::Vec2::FromRaw(
            Setup.MoveDestination.x.Raw() + sim::kFixedScale / 4,
            Setup.MoveDestination.y.Raw() + sim::kFixedScale / 4);
        TestTrue(TEXT("Deferred Route move queues"), Simulation.QueueCommand(Move));
        Observer.ObserveAcceptedCommand(
            RouteSession, RouteSnapshot, Simulation, 2,
            EEchoesTutorialOrderCommandOrigin::ContextAction);
        TestFalse(TEXT("A pending command has no Applied receipt"),
            Observer.RouteSimulationPredicateSatisfied());
        for (int32 Tick = 0;
             Tick < 10 &&
                 !Simulation.FindCommandResolutionReceipt(0, 2).has_value();
             ++Tick)
        {
            Simulation.Step();
            Observer.ObserveState(RouteSession, RouteSnapshot, Simulation);
        }
        TestTrue(TEXT("Deferred Route move eventually resolves"),
            Simulation.FindCommandResolutionReceipt(0, 2).has_value());
        Observer.ObserveAcceptedCommand(
            RouteSession, RouteSnapshot, Simulation, 2,
            EEchoesTutorialOrderCommandOrigin::ContextAction);
        for (int32 Tick = 0;
             Tick < 300 &&
                 Simulation.FindEntity(Mover)->order.type != sim::OrderType::None;
             ++Tick)
        {
            Simulation.Step();
            Observer.ObserveState(RouteSession, RouteSnapshot, Simulation);
        }
        TestEqual(TEXT("Route move arrives and clears naturally"),
            Simulation.FindEntity(Mover)->order.type, sim::OrderType::None);

        sim::Command Context = MakeCommand(
            Simulation, 0, 3, sim::CommandType::Gather, Worker);
        Context.target = Node;
        TestTrue(TEXT("Context Gather resolves through the real simulation"),
            QueueStepAndObserve(*this, Simulation, Observer, RouteSession,
                RouteSnapshot, Context,
                EEchoesTutorialOrderCommandOrigin::ContextAction));

        sim::Command WrongPatrol = MakeCommand(
            Simulation, 0, 4, sim::CommandType::Patrol, Core);
        WrongPatrol.position = sim::Vec2::FromTiles(16, 16);
        TestTrue(TEXT("NoEffect command resolves"),
            QueueStepAndObserve(*this, Simulation, Observer, RouteSession,
                RouteSnapshot, WrongPatrol,
                EEchoesTutorialOrderCommandOrigin::DirectPlayerCommand));
        TestFalse(TEXT("NoEffect cannot satisfy a positive command gate"),
            Observer.RouteSimulationPredicateSatisfied());

        sim::Command Patrol = MakeCommand(
            Simulation, 0, 5, sim::CommandType::Patrol, Mover);
        Patrol.position = sim::Vec2::FromRaw(
            Setup.PatrolDestination.x.Raw() + sim::kFixedScale / 4,
            Setup.PatrolDestination.y.Raw() + sim::kFixedScale / 4);
        TestTrue(TEXT("Patrol resolves through the real simulation"),
            QueueStepAndObserve(*this, Simulation, Observer, RouteSession,
                RouteSnapshot, Patrol,
                EEchoesTutorialOrderCommandOrigin::DirectPlayerCommand));
        sim::Command Stop = MakeCommand(
            Simulation, 0, 6, sim::CommandType::Stop, Mover);
        TestTrue(TEXT("Stop resolves through the real simulation"),
            QueueStepAndObserve(*this, Simulation, Observer, RouteSession,
                RouteSnapshot, Stop,
                EEchoesTutorialOrderCommandOrigin::DirectPlayerCommand));
        TestEqual(TEXT("Stop clears the active Patrol order"),
            Simulation.FindEntity(Mover)->order.type, sim::OrderType::None);

        sim::Command GuardCommand = MakeCommand(
            Simulation, 0, 7, sim::CommandType::Guard, Guard);
        GuardCommand.target = Worker;
        TestTrue(TEXT("Guard resolves through the real simulation"),
            QueueStepAndObserve(*this, Simulation, Observer, RouteSession,
                RouteSnapshot, GuardCommand,
                EEchoesTutorialOrderCommandOrigin::DirectPlayerCommand));

        TestTrue(TEXT("Blocked-ground rejection becomes valid ground"),
            Simulation.SetTerrainTile(7, 6, sim::Terrain::Open));
        Simulation.Step();
        Observer.ObserveState(RouteSession, RouteSnapshot, Simulation);
        TestFalse(TEXT("Stale authored rejection state cannot validate a new attempt"),
            Observer.ObserveRejectedAttempt(
                RouteSession, RouteSnapshot, Simulation, 1,
                Setup.RejectedAction));
        Observer.ObserveRejectionAcknowledged(
            RouteSession, RouteSnapshot, Simulation, 1);
        TestFalse(TEXT("Acknowledging an unverified attempt cannot grant Route"),
            Observer.RouteSimulationPredicateSatisfied());

        TestTrue(TEXT("Authored rejection ground blocks again"),
            Simulation.SetTerrainTile(7, 6, sim::Terrain::Blocked));
        Simulation.Step();
        Observer.ObserveState(RouteSession, RouteSnapshot, Simulation);
        FEchoesTutorialExpectedCommand OffsetRejectedAttempt =
            Setup.RejectedAction;
        OffsetRejectedAttempt.Position = sim::Vec2::FromRaw(
            Setup.RejectedAction.Position.x.Raw() + sim::kFixedScale / 4,
            Setup.RejectedAction.Position.y.Raw() + sim::kFixedScale / 4);
        FEchoesTutorialExpectedCommand AdjacentTileAttempt =
            Setup.RejectedAction;
        AdjacentTileAttempt.Position = sim::Vec2::FromRaw(
            Setup.RejectedAction.Position.x.Raw() - 1,
            Setup.RejectedAction.Position.y.Raw());
        TestFalse(TEXT("A nearby point in another tile cannot impersonate the blocked site"),
            Observer.ObserveRejectedAttempt(
                RouteSession, RouteSnapshot, Simulation, 2,
                AdjacentTileAttempt));
        FEchoesTutorialExpectedCommand OutOfRadiusAttempt =
            Setup.RejectedAction;
        OutOfRadiusAttempt.Position = sim::Vec2::FromRaw(
            Setup.RejectedAction.Position.x.Raw() +
                sim::kFixedScale * 3 / 4,
            Setup.RejectedAction.Position.y.Raw());
        TestFalse(TEXT("The blocked tile remains bounded by its authored input radius"),
            Observer.ObserveRejectedAttempt(
                RouteSession, RouteSnapshot, Simulation, 3,
                OutOfRadiusAttempt));
        TestTrue(TEXT("Observer independently derives the blocked Move rejection"),
            Observer.ObserveRejectedAttempt(
                RouteSession, RouteSnapshot, Simulation, 4,
                OffsetRejectedAttempt));
        TestFalse(TEXT("A rejection attempt alone does not prove it was read"),
            Observer.RouteSimulationPredicateSatisfied());
        Observer.ObserveRejectionAcknowledged(
            RouteSession, RouteSnapshot, Simulation, 1);
        TestFalse(TEXT("A stale attempt acknowledgment cannot grant Route"),
            Observer.RouteSimulationPredicateSatisfied());
        TestTrue(TEXT("Rejected ground may change after the recorded attempt"),
            Simulation.SetTerrainTile(7, 6, sim::Terrain::Open));
        Simulation.Step();
        Observer.ObserveState(RouteSession, RouteSnapshot, Simulation);
        Observer.ObserveRejectionAcknowledged(
            RouteSession, RouteSnapshot, Simulation, 4);
        TestTrue(TEXT("A later explicit acknowledgment retains the historical rejection"),
            Observer.RouteSimulationPredicateSatisfied());
        TestFalse(TEXT("An acknowledged physical attempt cannot be reused"),
            Observer.ObserveRejectedAttempt(
                RouteSession, RouteSnapshot, Simulation, 4,
                Setup.RejectedAction));
        const FEchoesTutorialRouteProgress RouteProgress =
            Observer.RouteProgress();
        TestTrue(TEXT("Route progress exposes natural Move arrival"),
            RouteProgress.bMoveArrived);
        TestTrue(TEXT("Route progress exposes acknowledged rejection"),
            RouteProgress.bRejectionAcknowledged);
        TestTrue(TEXT("Move arrival, context action, Stop, Patrol, Guard and rejection satisfy Route source predicate"),
            Observer.RouteSimulationPredicateSatisfied());

        Observer.ObserveState(RouteSession, RouteSnapshot + 1, Simulation);
        TestFalse(TEXT("Snapshot replacement invalidates Route progress"),
            Observer.IsActive());
    }

    {
        sim::Simulation Simulation(sim::SimulationConfig{32, 32, 20, 0xAE5E7EULL});
        TestTrue(TEXT("Reserve local player joins"), Simulation.AddPlayer(
            0, sim::Faction::MeridianCompact, {10000, 100}));
        const sim::EntityId Core = Simulation.SpawnEntity(
            0, sim::Faction::MeridianCompact, sim::EntityType::CommandCore,
            sim::Vec2::FromTiles(8, 8));
        const sim::EntityId Worker = Simulation.SpawnEntity(
            0, sim::Faction::MeridianCompact, sim::EntityType::Worker,
            sim::Vec2::FromTiles(10, 8));
        const sim::EntityId Node = Simulation.SpawnResourceNode(
            sim::Vec2::FromTiles(11, 8), 1000);
        const sim::EntityId OtherWorker = Simulation.SpawnEntity(
            0, sim::Faction::MeridianCompact, sim::EntityType::Worker,
            sim::Vec2::FromTiles(10, 14));
        const sim::EntityId OtherNode = Simulation.SpawnResourceNode(
            sim::Vec2::FromTiles(11, 14), 1000);
        TestTrue(TEXT("Reserve fixture spawned"),
            Core && Worker && Node && OtherWorker && OtherNode);

        FEchoesTutorialReserveSetup Setup;
        Setup.LocalPlayer = 0;
        Setup.Worker = Worker;
        Setup.MatterNode = Node;
        constexpr uint64 ReserveSession = 81;
        constexpr uint64 ReserveSnapshot = 8001;
        FEchoesTutorialOrderObservation Observer;
        TestTrue(TEXT("Authored Reserve setup begins"), Observer.BeginReserve(
            ReserveSession, ReserveSnapshot, Simulation, Setup));
        TestFalse(TEXT("A large initial balance cannot complete Reserve"),
            Observer.ReserveSimulationPredicateSatisfied());
        TestEqual(TEXT("No delivered Matter is inferred from initial balance"),
            Observer.DeliveredMatterObserved(), uint64{0});

        sim::Command WrongWorkerGather = MakeCommand(
            Simulation, 0, 1, sim::CommandType::Gather, OtherWorker);
        WrongWorkerGather.target = OtherNode;
        TestTrue(TEXT("Other worker Gather resolves"),
            QueueStepAndObserve(*this, Simulation, Observer, ReserveSession,
                ReserveSnapshot, WrongWorkerGather,
                EEchoesTutorialOrderCommandOrigin::ContextAction));
        TestFalse(TEXT("Wrong actor cannot start the bound Reserve route"),
            Observer.ReserveSimulationPredicateSatisfied());

        sim::Command NoEffectGather = MakeCommand(
            Simulation, 0, 2, sim::CommandType::Gather, Worker);
        NoEffectGather.target = 999999;
        TestTrue(TEXT("Invalid Gather reaches NoEffect receipt"),
            QueueStepAndObserve(*this, Simulation, Observer, ReserveSession,
                ReserveSnapshot, NoEffectGather,
                EEchoesTutorialOrderCommandOrigin::ContextAction));
        TestTrue(TEXT("Gather NoEffect is observable"),
            Simulation.FindCommandResolutionReceipt(0, 2)->outcome ==
                sim::CommandResolutionOutcome::NoEffect);
        TestFalse(TEXT("Gather NoEffect cannot start Reserve"),
            Observer.ReserveSimulationPredicateSatisfied());

        sim::Command Gather = MakeCommand(
            Simulation, 0, 3, sim::CommandType::Gather, Worker);
        Gather.target = Node;
        TestTrue(TEXT("Bound Gather resolves through the real simulation"),
            QueueStepAndObserve(*this, Simulation, Observer, ReserveSession,
                ReserveSnapshot, Gather,
                EEchoesTutorialOrderCommandOrigin::ContextAction));
        for (int32 Tick = 0;
             Tick < 2000 && !Observer.ReserveSimulationPredicateSatisfied();
             ++Tick)
        {
            Simulation.Step();
            Observer.ObserveState(ReserveSession, ReserveSnapshot, Simulation);
        }
        TestEqual(TEXT("Only the bound worker's genuine deliveries accumulate"),
            Observer.DeliveredMatterObserved(),
            FEchoesTutorialReserveSetup::RequiredDeliveredMatter);
        const FEchoesTutorialReserveProgress ReserveProgress =
            Observer.ReserveProgress();
        TestTrue(TEXT("Reserve progress exposes the Gather start"),
            ReserveProgress.bGatherStarted);
        TestTrue(TEXT("Reserve progress exposes a Deliver cycle"),
            ReserveProgress.bDeliverObserved);
        TestEqual(TEXT("Reserve progress exposes genuine delivered Matter"),
            ReserveProgress.DeliveredMatter,
            FEchoesTutorialReserveSetup::RequiredDeliveredMatter);
        TestTrue(TEXT("Gather, Deliver, 200 Matter and a continuing route satisfy Reserve source predicate"),
            Observer.ReserveSimulationPredicateSatisfied());

        TestTrue(TEXT("Retry begins a fresh Reserve observation"),
            Observer.BeginReserve(
                ReserveSession + 1, ReserveSnapshot + 1, Simulation, Setup));
        TestEqual(TEXT("Retry clears delivered Matter progress"),
            Observer.DeliveredMatterObserved(), uint64{0});
        TestFalse(TEXT("Retry does not retain Reserve completion"),
            Observer.ReserveSimulationPredicateSatisfied());
        Simulation.MutableEntityForTesting(Worker)->hitPoints = 0;
        Simulation.Step();
        Observer.ObserveState(
            ReserveSession + 1, ReserveSnapshot + 1, Simulation);
        TestFalse(TEXT("A dead required worker safely invalidates the attempt"),
            Observer.IsActive());
    }

    {
        // A snapshot can retain an accepted pending command without a
        // resolution receipt. Beginning after load captures the advanced
        // sequence frontier, so the old command cannot grant later either.
        sim::Simulation Source(sim::SimulationConfig{32, 32, 20, 0x10ADEDULL});
        TestTrue(TEXT("Loaded scenario player joins"), Source.AddPlayer(
            0, sim::Faction::MeridianCompact, {400, 80}));
        Source.SpawnEntity(0, sim::Faction::MeridianCompact,
            sim::EntityType::CommandCore, sim::Vec2::FromTiles(8, 8));
        const sim::EntityId Worker = Source.SpawnEntity(
            0, sim::Faction::MeridianCompact, sim::EntityType::Worker,
            sim::Vec2::FromTiles(10, 8));
        const sim::EntityId Node = Source.SpawnResourceNode(
            sim::Vec2::FromTiles(11, 8), 1000);
        sim::Command Pending = MakeCommand(
            Source, 0, 1, sim::CommandType::Gather, Worker);
        Pending.executeTick = 2;
        Pending.target = Node;
        TestTrue(TEXT("Pending Gather queues before save"),
            Source.QueueCommand(Pending));
        std::string Error;
        std::optional<sim::Simulation> Loaded =
            sim::Simulation::LoadSnapshot(Source.SaveSnapshot(), &Error);
        if (!TestTrue(TEXT("Pending snapshot loads"), Loaded.has_value()))
            return false;
        FEchoesTutorialReserveSetup Setup;
        Setup.LocalPlayer = 0;
        Setup.Worker = Worker;
        Setup.MatterNode = Node;
        FEchoesTutorialOrderObservation Observer;
        TestTrue(TEXT("Reserve begins on loaded snapshot"),
            Loaded.has_value() && Observer.BeginReserve(91, 9001, *Loaded, Setup));
        Observer.ObserveAcceptedCommand(
            91, 9001, *Loaded, 1,
            EEchoesTutorialOrderCommandOrigin::ContextAction);
        TestFalse(TEXT("Loaded command without a receipt cannot grant"),
            Observer.ReserveSimulationPredicateSatisfied());
        while (Loaded->CurrentTick() <= Pending.executeTick)
        {
            Loaded->Step();
            Observer.ObserveState(91, 9001, *Loaded);
        }
        Observer.ObserveAcceptedCommand(
            91, 9001, *Loaded, 1,
            EEchoesTutorialOrderCommandOrigin::ContextAction);
        TestFalse(TEXT("A pre-Begin loaded command stays outside the lesson frontier after resolution"),
            Observer.ReserveSimulationPredicateSatisfied());

        sim::Simulation ReplaySource(
            sim::SimulationConfig{32, 32, 20, 0x7E91A9ULL});
        TestTrue(TEXT("Replay scenario player joins"), ReplaySource.AddPlayer(
            0, sim::Faction::MeridianCompact, {400, 80}));
        ReplaySource.SpawnEntity(0, sim::Faction::MeridianCompact,
            sim::EntityType::CommandCore, sim::Vec2::FromTiles(8, 8));
        const sim::EntityId ReplayWorker = ReplaySource.SpawnEntity(
            0, sim::Faction::MeridianCompact, sim::EntityType::Worker,
            sim::Vec2::FromTiles(10, 8));
        const sim::EntityId ReplayNode = ReplaySource.SpawnResourceNode(
            sim::Vec2::FromTiles(11, 8), 1000);
        ReplaySource.CaptureReplayBaseline();
        sim::Command ReplayGather = MakeCommand(
            ReplaySource, 0, 1, sim::CommandType::Gather, ReplayWorker);
        ReplayGather.target = ReplayNode;
        TestTrue(TEXT("Replay Gather queues"),
            ReplaySource.QueueCommand(ReplayGather));
        ReplaySource.Step();
        std::string ReplayError;
        const sim::ReplayRecord Replay = ReplaySource.ExportReplay(&ReplayError);
        std::optional<sim::Simulation> Replayed =
            sim::Simulation::ReplayToEnd(Replay, &ReplayError);
        if (!TestTrue(TEXT("Replay reconstructs for provenance check"),
                Replayed.has_value()))
            return false;
        FEchoesTutorialOrderObservation ReplayObserver;
        FEchoesTutorialReserveSetup ReplaySetup;
        ReplaySetup.LocalPlayer = 0;
        ReplaySetup.Worker = ReplayWorker;
        ReplaySetup.MatterNode = ReplayNode;
        TestTrue(TEXT("Reserve can begin after replay reconstruction"),
            Replayed.has_value() &&
                ReplayObserver.BeginReserve(
                    92, 9002, *Replayed, ReplaySetup));
        ReplayObserver.ObserveAcceptedCommand(
            92, 9002, *Replayed, 1,
            EEchoesTutorialOrderCommandOrigin::ContextAction);
        TestFalse(TEXT("A replayed command cannot grant a new lesson attempt"),
            ReplayObserver.ReserveSimulationPredicateSatisfied());
    }

    return true;
}

#endif
