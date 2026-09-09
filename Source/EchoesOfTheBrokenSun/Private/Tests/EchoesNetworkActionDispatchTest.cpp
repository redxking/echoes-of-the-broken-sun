#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesNetworkSession.h"
#include "EchoesSimCore/NetworkProtocol.h"
#include "EchoesSimCore/Simulation.h"

#include <algorithm>
#include <limits>
#include <span>
#include <string>
#include <vector>

namespace
{
using echoes::network::BulwarkActionDispatchState;
using echoes::network::BulwarkAdmissionResult;
using echoes::network::BulwarkDispatchActor;
using echoes::network::ScopedViewAcceptance;
using echoes::network::ScopedViewState;
using namespace echoes::sim;
using namespace echoes::sim::net;

[[nodiscard]] bool BuildScoped(
    const Simulation& SimulationValue,
    std::uint64_t SnapshotId,
    ScopedViewKeyframe& OutView,
    std::string& OutError)
{
    const std::optional<PlayerView> Player =
        SimulationValue.CreatePlayerView(0);
    return Player.has_value() &&
           BuildScopedViewKeyframe(
               *Player, SnapshotId, 0, OutView, &OutError);
}

[[nodiscard]] ScopedEntityState* FindScoped(
    ScopedViewKeyframe& View,
    EntityId Id)
{
    const auto Found = std::lower_bound(
        View.entities.begin(), View.entities.end(), Id,
        [](const ScopedEntityState& Entity, EntityId Candidate)
        {
            return Entity.id < Candidate;
        });
    return Found != View.entities.end() && Found->id == Id
        ? &*Found
        : nullptr;
}
}  // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesNetworkBulwarkDispatchTest,
    "Echoes.Runtime.Network.BulwarkActionDispatch",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesNetworkBulwarkDispatchTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    Simulation SimulationValue({32, 32, 20, 97});
    if (!TestTrue(
            TEXT("Dispatch fixture adds both players"),
            SimulationValue.AddPlayer(
                0, Faction::MeridianCompact, {1000, 500}) &&
            SimulationValue.AddPlayer(
                1, Faction::KharuunAssemblies, {1000, 500})))
    {
        return false;
    }
    const EntityId LocalCore = SimulationValue.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::CommandCore,
        Vec2::FromTiles(3, 3));
    const EntityId RemoteCore = SimulationValue.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::CommandCore,
        Vec2::FromTiles(28, 28));
    const EntityId UnderTarget = SimulationValue.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::HeavyUnit,
        Vec2::FromTiles(10, 10));
    const EntityId TieLow = SimulationValue.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::HeavyUnit,
        Vec2::FromTiles(9, 10));
    const EntityId TieHigh = SimulationValue.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::HeavyUnit,
        Vec2::FromTiles(11, 10));
    const EntityId Far = SimulationValue.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::HeavyUnit,
        Vec2::FromTiles(18, 10));
    const EntityId Scout = SimulationValue.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::ScoutUnit,
        Vec2::FromTiles(10, 11));
    if (!TestTrue(
            TEXT("Dispatch fixture spawns bounded caster roles"),
            LocalCore != 0 && RemoteCore != 0 && UnderTarget != 0 &&
            TieLow != 0 && TieHigh != 0 && Far != 0 && Scout != 0))
    {
        return false;
    }

    std::string Error;
    ScopedViewKeyframe Base{};
    if (!TestTrue(
            TEXT("Authority produces the initial scoped view"),
            BuildScoped(SimulationValue, 1, Base, Error)))
    {
        return false;
    }
    ScopedViewState Client;
    if (!TestTrue(
            TEXT("Client admits the authentic scoped baseline"),
            Client.Accept(Base) == ScopedViewAcceptance::AcceptedFirst))
    {
        return false;
    }

    const std::vector<EntityId> Selected{
        Far, TieHigh, Scout, TieLow, UnderTarget, TieLow, 999999};
    const Vec2 Target = Vec2::FromTiles(10, 10);
    const std::vector<BulwarkDispatchActor> First =
        Client.BulwarkActions().Resolve(Base, 0, Target, Selected, false);
    TestTrue(
        TEXT("Normal action excludes packed caster at target and uses stable-ID tie break"),
        First.size() == 1 && First.front().actor == TieLow);
    TestTrue(
        TEXT("Empty selection never falls back to the owned army"),
        Client.BulwarkActions()
            .Resolve(
                Base,
                0,
                Target,
                std::span<const EntityId>{},
                false)
            .empty());
    if (First.empty() ||
        !TestTrue(
            TEXT("First gesture reserves its one resolved actor"),
            Client.BulwarkActions().ReserveBatch(1, First)))
    {
        return false;
    }
    const std::vector<BulwarkDispatchActor> Second =
        Client.BulwarkActions().Resolve(Base, 0, Target, Selected, false);
    TestTrue(
        TEXT("A rapid second gesture chooses the next eligible caster"),
        Second.size() == 1 && Second.front().actor == TieHigh);

    ScopedViewDelta Stale{};
    Stale.player = 0;
    Stale.snapshotId = Base.snapshotId;
    TestTrue(
        TEXT("Stale delta cannot release a pending gesture"),
        Client.AcceptDelta(Stale, &Error) ==
                ScopedViewAcceptance::StaleOrDuplicate &&
            Client.BulwarkActions().IsPending(TieLow));
    ScopedViewDelta Invalid{};
    Invalid.player = 0;
    Invalid.baseSnapshotId = Base.snapshotId;
    Invalid.snapshotId = Base.snapshotId + 1;
    TestTrue(
        TEXT("Rejected delta cannot release a pending gesture"),
        Client.AcceptDelta(Invalid, &Error) ==
                ScopedViewAcceptance::DeltaRejected &&
            Client.BulwarkActions().IsPending(TieLow));

    Command Deploy{};
    Deploy.player = 0;
    Deploy.sequence = 1;
    Deploy.type = CommandType::ToggleDeploy;
    Deploy.actor = TieLow;
    Deploy.position = Vec2::FromTiles(20, 10);
    if (!TestTrue(
            TEXT("Authority queues the resolved deployment"),
            SimulationValue.QueueCommand(Deploy)))
    {
        return false;
    }
    SimulationValue.Step();
    ScopedViewKeyframe Deploying{};
    if (!TestTrue(
            TEXT("Authority produces the transitioning scoped view"),
            BuildScoped(SimulationValue, 2, Deploying, Error)))
    {
        return false;
    }
    ScopedViewDelta DeployingDelta{};
    if (!TestTrue(
            TEXT("Authority builds an authentic transition delta"),
            BuildScopedViewDelta(
                Base, Deploying, DeployingDelta, &Error)))
    {
        return false;
    }
    TestTrue(
        TEXT("Only an admitted transition delta releases the reservation"),
        Client.AcceptDelta(DeployingDelta, &Error) ==
                ScopedViewAcceptance::AcceptedDelta &&
            !Client.BulwarkActions().IsPending(TieLow));

    SimulationValue.Step(kBulwarkDeployTicks - 1);
    ScopedViewKeyframe Deployed{};
    if (!TestTrue(
            TEXT("Authority produces the completed deployment view"),
            BuildScoped(SimulationValue, 3, Deployed, Error)) ||
        !TestTrue(
            TEXT("Client accepts completed deployment lineage"),
            Client.Accept(Deployed) == ScopedViewAcceptance::AcceptedNext))
    {
        return false;
    }
    const std::vector<EntityId> PackSelection{TieLow};
    ScopedEntityState* DeployedActor = FindScoped(Deployed, TieLow);
    if (!TestNotNull(
            TEXT("Completed deployment retains the scoped Bulwark"),
            DeployedActor))
    {
        return false;
    }
    const std::vector<BulwarkDispatchActor> Pack =
        Client.BulwarkActions().Resolve(
            Deployed,
            0,
            DeployedActor->position,
            PackSelection,
            false);
    TestTrue(
        TEXT("A deployed Bulwark remains eligible to pack at its own position"),
        Pack.size() == 1 && Pack.front().actor == TieLow &&
            Pack.front().baselineDeployed);
    if (Pack.empty() ||
        !TestTrue(
            TEXT("Pack gesture reserves the deployed baseline"),
            Client.BulwarkActions().ReserveBatch(2, Pack)))
    {
        return false;
    }
    Deploy.sequence = 2;
    Deploy.executeTick = SimulationValue.CurrentTick();
    if (!TestTrue(
            TEXT("Authority queues real packing"),
            SimulationValue.QueueCommand(Deploy)))
    {
        return false;
    }
    SimulationValue.Step();
    ScopedViewKeyframe Packing{};
    if (!TestTrue(
            TEXT("Authority produces the packing scoped view"),
            BuildScoped(SimulationValue, 4, Packing, Error)))
    {
        return false;
    }
    ScopedViewDelta PackingDelta{};
    TestTrue(
        TEXT("Packing phase, while deployed remains true, releases the pack reservation"),
        BuildScopedViewDelta(Deployed, Packing, PackingDelta, &Error) &&
            Client.AcceptDelta(PackingDelta, &Error) ==
                ScopedViewAcceptance::AcceptedDelta &&
            !Client.BulwarkActions().IsPending(TieLow));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesNetworkBulwarkPendingReconciliationTest,
    "Echoes.Runtime.Network.BulwarkPendingReconciliation",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesNetworkBulwarkPendingReconciliationTest::RunTest(
    const FString& Parameters)
{
    (void)Parameters;

    ScopedViewKeyframe Base{};
    Base.snapshotId = 10;
    Base.simulationTick = 100;
    Base.player = 0;
    ScopedEntityState First{};
    First.id = 10;
    First.owner = 0;
    First.faction = Faction::MeridianCompact;
    First.type = EntityType::HeavyUnit;
    First.position = Vec2::FromTiles(8, 8);
    ScopedEntityState Second = First;
    Second.id = 20;
    Second.position = Vec2::FromTiles(12, 8);
    Base.entities = {First, Second};

    ScopedViewState Client;
    if (!TestTrue(
            TEXT("Pending fixture accepts the scoped baseline"),
            Client.Accept(Base) == ScopedViewAcceptance::AcceptedFirst))
    {
        return false;
    }
    const std::vector<EntityId> Selected{20, 10, 20};
    const std::vector<BulwarkDispatchActor> All =
        Client.BulwarkActions().Resolve(
            Base, 0, Vec2::FromTiles(10, 8), Selected, true);
    TestTrue(
        TEXT("Ctrl dispatch deduplicates and returns stable actor order"),
        All.size() == 2 && All[0].actor == 10 && All[1].actor == 20);
    if (!TestTrue(
            TEXT("Ctrl dispatch reserves one aggregate batch"),
            Client.BulwarkActions().ReserveBatch(10, All)))
    {
        return false;
    }

    ScopedViewKeyframe PreAck = Base;
    ++PreAck.snapshotId;
    ++PreAck.simulationTick;
    ScopedEntityState* Changed = FindScoped(PreAck, 10);
    if (!TestNotNull(
            TEXT("Pre-ack fixture retains first actor"), Changed))
    {
        return false;
    }
    Changed->deploymentPhase = BulwarkDeploymentPhase::Deploying;
    Changed->deploymentTransitionUntilTick =
        PreAck.simulationTick + kBulwarkDeployTicks;
    TestTrue(
        TEXT("Accepted pre-ack state releases only its changed actor"),
        Client.Accept(PreAck) == ScopedViewAcceptance::AcceptedNext &&
            !Client.BulwarkActions().IsPending(10) &&
            Client.BulwarkActions().IsPending(20) &&
            Client.BulwarkActions().PendingActorCount() == 1);

    constexpr Tick ServerTick = 110;
    constexpr Tick InputDelay = 3;
    TestTrue(
        TEXT("Partial admission uses immutable submitted count after pre-ack reconciliation"),
        Client.BulwarkActions().ApplyAdmission(
            10, 1, 1, ServerTick, InputDelay, Client.Current()) ==
                BulwarkAdmissionResult::Applied &&
            Client.BulwarkActions().IsPending(20));

    ScopedViewKeyframe AtDeadline = PreAck;
    ++AtDeadline.snapshotId;
    AtDeadline.simulationTick = ServerTick + InputDelay;
    TestTrue(
        TEXT("Unchanged actor remains reserved at the execution tick"),
        Client.Accept(AtDeadline) == ScopedViewAcceptance::AcceptedNext &&
            Client.BulwarkActions().IsPending(20));
    ScopedViewKeyframe AfterDeadline = AtDeadline;
    ++AfterDeadline.snapshotId;
    ++AfterDeadline.simulationTick;
    TestTrue(
        TEXT("No-effect execution releases unconditionally after its deadline"),
        Client.Accept(AfterDeadline) == ScopedViewAcceptance::AcceptedNext &&
            !Client.BulwarkActions().IsPending(20));

    const std::vector<EntityId> SecondOnly{20};
    const std::vector<BulwarkDispatchActor> Late =
        Client.BulwarkActions().Resolve(
            AfterDeadline,
            0,
            Vec2::FromTiles(15, 8),
            SecondOnly,
            false);
    if (!TestTrue(
            TEXT("Released no-effect actor can be selected again"),
            Late.size() == 1 &&
                Client.BulwarkActions().ReserveBatch(11, Late)))
    {
        return false;
    }
    ScopedViewKeyframe BeforeLateAck = AfterDeadline;
    ++BeforeLateAck.snapshotId;
    BeforeLateAck.simulationTick = 130;
    TestTrue(
        TEXT("Pre-ack unchanged view does not guess a deadline"),
        Client.Accept(BeforeLateAck) ==
                ScopedViewAcceptance::AcceptedNext &&
            Client.BulwarkActions().IsPending(20));
    TestTrue(
        TEXT("Late admission reconciles immediately against the newest accepted view"),
        Client.BulwarkActions().ApplyAdmission(
            11, 1, 0, 120, 3, Client.Current()) ==
                BulwarkAdmissionResult::Applied &&
            !Client.BulwarkActions().IsPending(20));

    if (!TestTrue(
            TEXT("Actor can reserve a full-rejection fixture"),
            Client.BulwarkActions().ReserveBatch(12, Late)))
    {
        return false;
    }
    TestTrue(
        TEXT("Full admission rejection frees the matching batch"),
        Client.BulwarkActions().ApplyAdmission(
            12, 0, 1, 130, 3, Client.Current()) ==
                BulwarkAdmissionResult::Rejected &&
            Client.BulwarkActions().PendingActorCount() == 0);

    if (!TestTrue(
            TEXT("Actor can reserve an overflow fixture"),
            Client.BulwarkActions().ReserveBatch(13, Late)))
    {
        return false;
    }
    TestTrue(
        TEXT("Admission tick overflow fails closed and releases reservations"),
        Client.BulwarkActions().ApplyAdmission(
            13,
            1,
            0,
            std::numeric_limits<Tick>::max(),
            1,
            Client.Current()) == BulwarkAdmissionResult::TickOverflow &&
            Client.BulwarkActions().PendingActorCount() == 0);
    TestFalse(
        TEXT("Batch zero fails the reservation precondition"),
        Client.BulwarkActions().ReserveBatch(0, Late));
    if (!TestTrue(
            TEXT("Missing-actor fixture reserves before accepted removal"),
            Client.BulwarkActions().ReserveBatch(14, Late)))
    {
        return false;
    }
    ScopedViewKeyframe Missing = BeforeLateAck;
    ++Missing.snapshotId;
    ++Missing.simulationTick;
    Missing.entities.erase(
        std::remove_if(
            Missing.entities.begin(),
            Missing.entities.end(),
            [](const ScopedEntityState& Entity)
            {
                return Entity.id == 20;
            }),
        Missing.entities.end());
    TestTrue(
        TEXT("Accepted missing-actor state releases its reservation early"),
        Client.Accept(Missing) == ScopedViewAcceptance::AcceptedNext &&
            Client.BulwarkActions().PendingActorCount() == 0);
    if (!TestTrue(
            TEXT("Malformed-admission fixture reserves before batch-zero recovery"),
            Client.BulwarkActions().ReserveBatch(15, Late)))
    {
        return false;
    }
    TestTrue(
        TEXT("Batch-zero admission clears uncorrelatable reservations"),
        Client.BulwarkActions().ApplyAdmission(
            0, 0, 0, 0, 0, Client.Current()) ==
                BulwarkAdmissionResult::InvalidBatchId &&
            Client.BulwarkActions().PendingActorCount() == 0);
    if (!TestTrue(
            TEXT("Lifecycle fixture reserves before explicit reset"),
            Client.BulwarkActions().ReserveBatch(16, Late)))
    {
        return false;
    }
    Client.ResetBulwarkActions();
    TestTrue(
        TEXT("Lifecycle reset clears every pending actor and batch"),
        Client.BulwarkActions().PendingActorCount() == 0 &&
            Client.BulwarkActions().PendingBatchCount() == 0);
    return true;
}

#endif
