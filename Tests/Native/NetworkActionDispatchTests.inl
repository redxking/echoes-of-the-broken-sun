// Author and owner: Angelis Pseftis
// Native coverage of client intent selection against real authority snapshots.

void TestNetworkBulwarkGestureDispatch() {
    using echoes::network::BulwarkActionDispatchState;
    using echoes::network::BulwarkAdmissionResult;
    Simulation sim({64, 64, 20, 0x4e455442554c57ULL});
    REQUIRE(sim.AddPlayer(0, Faction::MeridianCompact, {1000, 500}));
    REQUIRE(sim.AddPlayer(1, Faction::MeridianCompact, {1000, 500}));
    const auto near = sim.SpawnEntity(0, Faction::MeridianCompact,
        EntityType::HeavyUnit, Vec2::FromTiles(10, 10));
    const auto far = sim.SpawnEntity(0, Faction::MeridianCompact,
        EntityType::HeavyUnit, Vec2::FromTiles(20, 10));
    const auto peer = sim.SpawnEntity(0, Faction::MeridianCompact,
        EntityType::HeavyUnit, Vec2::FromTiles(10, 20));
    const auto scout = sim.SpawnEntity(0, Faction::MeridianCompact,
        EntityType::ScoutUnit, Vec2::FromTiles(14, 14));
    const auto enemy = sim.SpawnEntity(1, Faction::MeridianCompact,
        EntityType::HeavyUnit, Vec2::FromTiles(15, 15));
    REQUIRE(near && far && peer && scout && enemy);
    const std::vector<EntityId> selected{enemy, scout, peer, far, near, near, 999999};
    const Vec2 target = Vec2::FromTiles(15, 15);
    std::uint64_t snapshotId = 0;
    const auto snapshot = [&]() {
        auto player = sim.CreatePlayerView(0);
        REQUIRE(player.has_value());
        net::ScopedViewKeyframe view;
        REQUIRE(net::BuildScopedViewKeyframe(*player, ++snapshotId, 0, view));
        return view;
    };
    auto view = snapshot();
    BulwarkActionDispatchState pending;
    const auto checksum = sim.StateChecksum();
    REQUIRE(pending.Resolve(view, 0, target, {}, false).empty());
    REQUIRE(pending.Resolve(view, 1, target, selected, false).empty());
    auto first = pending.Resolve(view, 0, target, selected, false);
    REQUIRE(first.size() == 1 && first[0].actor == near);
    REQUIRE(pending.ReserveBatch(1, first));
    // A second gesture before either acknowledgement or new snapshot chooses
    // the next entity by exact distance and stable identifier.
    auto second = pending.Resolve(view, 0, target, selected, false);
    REQUIRE(second.size() == 1 && second[0].actor == far);
    REQUIRE(pending.ReserveBatch(2, second));
    auto third = pending.Resolve(view, 0, target, selected, false);
    REQUIRE(third.size() == 1 && third[0].actor == peer);
    REQUIRE(sim.StateChecksum() == checksum); // Input bookkeeping is cosmetic.

    REQUIRE(pending.ApplyAdmission(1, 0, 1, 0, 3, view) ==
        BulwarkAdmissionResult::Rejected);
    REQUIRE(!pending.IsPending(near));
    REQUIRE(pending.IsPending(far));
    net::CommandAdmissionContext context;
    net::CommandRequest deploy;
    deploy.sequence = 1;
    deploy.executeTick = 3;
    deploy.type = CommandType::ToggleDeploy;
    deploy.actor = far;
    deploy.position = target;
    REQUIRE(net::AdmitCommandRequest(deploy, context, sim) ==
        net::CommandAdmissionStatus::Accepted);
    REQUIRE(pending.ApplyAdmission(2, 1, 0, 0, 3, view) ==
        BulwarkAdmissionResult::Applied);
    sim.Step(3);
    view = snapshot();
    pending.ReconcileAcceptedView(view);
    REQUIRE(pending.IsPending(far)); // Tick 3 has not executed yet.
    sim.Step();
    view = snapshot();
    pending.ReconcileAcceptedView(view);
    REQUIRE(sim.FindEntity(far)->deploymentPhase == BulwarkDeploymentPhase::Deploying);
    auto during = pending.Resolve(view, 0, target, std::vector<EntityId>{far}, false);
    REQUIRE(during.empty());
    sim.Step(19);
    view = snapshot();
    REQUIRE(sim.FindEntity(far)->deployed);
    // Pack may target the caster's own position; deploy must have a direction.
    auto pack = pending.Resolve(view, 0, sim.FindEntity(far)->position,
        std::vector<EntityId>{far}, false);
    REQUIRE(pack.size() == 1 && pack[0].baselineDeployed);
    REQUIRE(pending.ReserveBatch(3, pack));
    deploy.sequence = 2;
    deploy.executeTick = sim.CurrentTick() + 3;
    deploy.position = sim.FindEntity(far)->position;
    REQUIRE(net::AdmitCommandRequest(deploy, context, sim) ==
        net::CommandAdmissionStatus::Accepted);
    REQUIRE(pending.ApplyAdmission(3, 1, 0, sim.CurrentTick(), 3, view) ==
        BulwarkAdmissionResult::Applied);
    pending.ReconcileAcceptedView(view);
    REQUIRE(pending.IsPending(far)); // Already deployed is not pack completion.
    sim.Step(4);
    view = snapshot();
    pending.ReconcileAcceptedView(view);
    REQUIRE(sim.FindEntity(far)->deployed);
    REQUIRE(sim.FindEntity(far)->deploymentPhase == BulwarkDeploymentPhase::Packing);
    REQUIRE(pending.Resolve(view, 0, target, std::vector<EntityId>{far}, false).empty());
    sim.Step(14);
    view = snapshot();
    pending.ReconcileAcceptedView(view);
    REQUIRE(!sim.FindEntity(far)->deployed);
    REQUIRE(pending.Resolve(view, 0, sim.FindEntity(far)->position,
        std::vector<EntityId>{far}, false).empty());
    auto all = pending.Resolve(view, 0, target, selected, true);
    REQUIRE(all.size() == 3);
    REQUIRE(all[0].actor == near && all[1].actor == far && all[2].actor == peer);
}

void TestNetworkBulwarkPendingRecovery() {
    using echoes::network::BulwarkActionDispatchState;
    using echoes::network::BulwarkAdmissionResult;
    Simulation sim({32, 32, 20, 0x4e45545245434fULL});
    AddTwoPlayers(sim);
    const auto a = sim.SpawnEntity(0, Faction::MeridianCompact,
        EntityType::HeavyUnit, Vec2::FromTiles(10, 10));
    const auto b = sim.SpawnEntity(0, Faction::MeridianCompact,
        EntityType::HeavyUnit, Vec2::FromTiles(15, 10));
    REQUIRE(a && b);
    std::uint64_t nextSnapshot = 0;
    const auto snapshot = [&]() {
        auto player = sim.CreatePlayerView(0);
        REQUIRE(player.has_value());
        net::ScopedViewKeyframe view;
        REQUIRE(net::BuildScopedViewKeyframe(*player, ++nextSnapshot, 0, view));
        return view;
    };
    auto view = snapshot();
    BulwarkActionDispatchState pending;
    const std::vector<EntityId> selected{b, a};
    auto all = pending.Resolve(view, 0, Vec2::FromTiles(20, 10), selected, true);
    REQUIRE(all.size() == 2);
    REQUIRE(pending.ReserveBatch(1, all));
    REQUIRE(!pending.ReserveBatch(1, all));
    REQUIRE(pending.ApplyAdmission(1, 1, 1, 0, 3, view) ==
        BulwarkAdmissionResult::Applied);
    // The count-only partial response conservatively holds both actors. Even
    // unchanged state cannot free them until their assigned tick has run.
    sim.Step(3);
    view = snapshot();
    pending.ReconcileAcceptedView(view);
    REQUIRE(pending.IsPending(a) && pending.IsPending(b));
    sim.Step();
    view = snapshot();
    pending.ReconcileAcceptedView(view);
    REQUIRE(pending.PendingActorCount() == 0);
    // A newer snapshot may arrive before the reliable admission callback.
    REQUIRE(pending.ReserveBatch(2, all));
    REQUIRE(pending.ApplyAdmission(2, 1, 1, 0, 3, view) ==
        BulwarkAdmissionResult::Applied);
    REQUIRE(pending.PendingActorCount() == 0);
    REQUIRE(pending.ReserveBatch(3, all));
    REQUIRE(pending.RejectBatch(3));
    REQUIRE(pending.PendingBatchCount() == 0);
    REQUIRE(pending.ReserveBatch(4, all));
    pending.Reset();
    REQUIRE(pending.PendingActorCount() == 0);
    REQUIRE(pending.PendingBatchCount() == 0);
    REQUIRE(!pending.ReserveBatch(0, all));
    REQUIRE(pending.ReserveBatch(5, all));
    REQUIRE(pending.ApplyAdmission(5, 1, 1,
        std::numeric_limits<Tick>::max(), 3, view) ==
        BulwarkAdmissionResult::TickOverflow);
    REQUIRE(pending.PendingActorCount() == 0);
}

void TestNetworkBulwarkArithmeticBounds() {
    using echoes::network::BulwarkActionDispatchState;
    using echoes::network::BulwarkAdmissionResult;
    // Defensive helper arithmetic fixture, deliberately outside map bounds.
    // This does not claim that the wire decoder admits an off-map entity.
    net::ScopedViewKeyframe view;
    view.player = 0;
    const auto low = std::numeric_limits<std::int32_t>::min();
    const auto high = std::numeric_limits<std::int32_t>::max();
    for (EntityId id : {EntityId{1}, EntityId{2}}) {
        net::ScopedEntityState actor;
        actor.id = id;
        actor.owner = 0;
        actor.faction = Faction::MeridianCompact;
        actor.type = EntityType::HeavyUnit;
        actor.completed = true;
        actor.hitPoints = actor.maxHitPoints = 100;
        actor.position = Vec2::FromRaw(low + static_cast<std::int32_t>(id - 1), low);
        view.entities.push_back(actor);
    }
    const std::vector<EntityId> selected{1, 2};
    BulwarkActionDispatchState pending;
    const auto chosen = pending.Resolve(view, 0, Vec2::FromRaw(high, high), selected, false);
    REQUIRE(chosen.size() == 1 && chosen[0].actor == 2);
    REQUIRE(pending.ReserveBatch(1, chosen));
    REQUIRE(pending.ApplyAdmission(1, -1, 2, 0, 3, view) ==
        BulwarkAdmissionResult::InvalidCounts);
    REQUIRE(!pending.HasBatch(1));
    REQUIRE(pending.ReserveBatch(2, chosen));
    REQUIRE(pending.ApplyAdmission(2, 2, -1, 0, 3, view) ==
        BulwarkAdmissionResult::InvalidCounts);
    REQUIRE(!pending.HasBatch(2));
}

void TestNetworkSnapshotAcknowledgementBackpressure() {
    using echoes::network::SnapshotFlowControl;
    using echoes::network::SnapshotSendDecision;
    SnapshotFlowControl flow;
    REQUIRE(flow.Evaluate(0, 100.0) == SnapshotSendDecision::Send);
    flow.RecordSend(0, 100.0);
    for (std::size_t pending = 1; pending < 8; ++pending) {
        REQUIRE(flow.Evaluate(pending, 100.0 + pending * 0.5) ==
            SnapshotSendDecision::Send);
        flow.RecordSend(pending, 100.0 + pending * 0.5);
    }
    REQUIRE(flow.Evaluate(8, 104.0) == SnapshotSendDecision::WaitForAcknowledgement);
    REQUIRE(flow.Evaluate(8, 108.0) == SnapshotSendDecision::WaitForAcknowledgement);
    REQUIRE(flow.Evaluate(8, 144.999) == SnapshotSendDecision::WaitForAcknowledgement);
    // Full-window retransmissions and continued sends cannot renew the timeout.
    flow.RecordSend(8, 144.0);
    REQUIRE(flow.Evaluate(8, 145.0) == SnapshotSendDecision::TimedOut);
    flow.RecordValidAcknowledgement(7, 145.0);
    REQUIRE(flow.Evaluate(7, 145.0) == SnapshotSendDecision::Send);
    flow.RecordSend(7, 145.5);
    REQUIRE(flow.Evaluate(8, 189.999) == SnapshotSendDecision::WaitForAcknowledgement);
    REQUIRE(flow.Evaluate(8, 190.0) == SnapshotSendDecision::TimedOut);
    flow.RecordValidAcknowledgement(0, 190.0);
    REQUIRE(flow.Evaluate(0, 1000.0) == SnapshotSendDecision::Send);
    flow.RecordSend(0, 1000.0);
    REQUIRE(flow.Evaluate(1, 1044.999) == SnapshotSendDecision::Send);
    REQUIRE(flow.Evaluate(1, 1045.0) == SnapshotSendDecision::TimedOut);
    flow.Reset();
    REQUIRE(flow.Evaluate(0, 2000.0) == SnapshotSendDecision::Send);
}
