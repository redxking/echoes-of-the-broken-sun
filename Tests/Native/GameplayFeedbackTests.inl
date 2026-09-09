// Author and owner: Angelis Pseftis
// Source-only native coverage for the bounded transient player feedback feed.

void TestGameplayFeedbackCommandResolutionAndBounds() {
    using namespace echoes::feedback;

    Simulation sim({24, 24, 20, 0x4645454442415345ULL});
    AddTwoPlayers(sim);
    const EntityId core = sim.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::CommandCore,
        Vec2::FromTiles(4, 4));
    const EntityId worker = sim.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Worker,
        Vec2::FromTiles(5, 4));
    REQUIRE(core && worker);

    GameplayFeedbackState feed;
    REQUIRE(feed.BeginGeneration(11, 0) == GameplayFeedbackStatus::Accepted);
    Command stop = MakeCommand(2, 0, 1, CommandType::Stop, worker);
    Command noEffect = MakeCommand(2, 0, 2, CommandType::Repair, worker);
    noEffect.target = core; // A complete, undamaged target is a semantic no-op.
    REQUIRE(sim.QueueCommand(stop));
    REQUIRE(feed.TrackQueued(stop, 11, sim.CurrentTick()) ==
        GameplayFeedbackStatus::Accepted);
    REQUIRE(sim.QueueCommand(noEffect));
    REQUIRE(feed.TrackQueued(noEffect, 11, sim.CurrentTick()) ==
        GameplayFeedbackStatus::Accepted);
    REQUIRE(feed.History().size() == 2);
    REQUIRE(feed.History()[0].tick == 0);
    REQUIRE(feed.History()[0].eventId == 1);

    for (int index = 0; index < 3; ++index) {
        sim.Step();
        REQUIRE(feed.ObserveFixedStep(sim, 0, 11) ==
            GameplayFeedbackStatus::Accepted);
    }
    REQUIRE(feed.PendingCount() == 0);
    const auto applied = std::find_if(
        feed.History().begin(), feed.History().end(),
        [](const GameplayFeedbackEvent& event) {
            return event.kind == GameplayFeedbackKind::Applied &&
                event.sequence == 1;
        });
    const auto refused = std::find_if(
        feed.History().begin(), feed.History().end(),
        [](const GameplayFeedbackEvent& event) {
            return event.kind == GameplayFeedbackKind::NoEffect &&
                event.sequence == 2;
        });
    REQUIRE(applied != feed.History().end());
    REQUIRE(applied->tick == 2);
    REQUIRE(applied->resolutionOutcome == CommandResolutionOutcome::Applied);
    REQUIRE(refused != feed.History().end());
    REQUIRE(refused->tick == 2);
    REQUIRE(refused->resolutionOutcome != CommandResolutionOutcome::Applied);
    REQUIRE(feed.ObserveFixedStep(sim, 0, 11) ==
        GameplayFeedbackStatus::Duplicate);

    GameplayFeedbackState observer;
    REQUIRE(observer.BeginGeneration(14, 0) ==
        GameplayFeedbackStatus::Accepted);
    sim.Step();
    REQUIRE(observer.ObserveFixedStep(sim, 0, 14) ==
        GameplayFeedbackStatus::Accepted);
    sim.Step(2);
    REQUIRE(observer.ObserveFixedStep(sim, 0, 14) ==
        GameplayFeedbackStatus::ObservationGap);
    REQUIRE(observer.Loss().missedObservationTicks == 1);
    REQUIRE(observer.Loss().reseedRequired);

    GameplayFeedbackState bounded;
    REQUIRE(bounded.BeginGeneration(12, 0) ==
        GameplayFeedbackStatus::Accepted);
    for (std::size_t index = 0;
         index < GameplayFeedbackState::PendingLimit; ++index) {
        Command queued = MakeCommand(
            sim.CurrentTick(), 0, 100 + index,
            CommandType::Stop, worker);
        REQUIRE(sim.QueueCommand(queued));
        REQUIRE(bounded.TrackQueued(queued, 12, sim.CurrentTick()) ==
            GameplayFeedbackStatus::Accepted);
    }
    Command overflow = MakeCommand(
        sim.CurrentTick(), 0, 100 + GameplayFeedbackState::PendingLimit,
        CommandType::Stop, worker);
    REQUIRE(sim.QueueCommand(overflow));
    REQUIRE(bounded.TrackQueued(overflow, 12, sim.CurrentTick()) ==
        GameplayFeedbackStatus::CapacityExceeded);
    REQUIRE(bounded.PendingCount() == GameplayFeedbackState::PendingLimit);
    REQUIRE(bounded.Loss().untrackedCommands == 1);
    REQUIRE(bounded.Loss().reseedRequired);

    REQUIRE(bounded.BeginGeneration(13, 0) ==
        GameplayFeedbackStatus::Accepted);
    REQUIRE(bounded.PendingCount() == 0);
    REQUIRE(bounded.History().empty());
    REQUIRE(!bounded.Loss().reseedRequired);
    Command restarted = MakeCommand(
        sim.CurrentTick(), 0, 1000, CommandType::Stop, worker);
    REQUIRE(sim.QueueCommand(restarted));
    REQUIRE(bounded.TrackQueued(restarted, 13, sim.CurrentTick()) ==
        GameplayFeedbackStatus::Accepted);
    REQUIRE(bounded.History().front().eventId == 1);

    GameplayFeedbackState retained;
    REQUIRE(retained.BeginRemoteStream(20, 0, 0) ==
        GameplayFeedbackStatus::Accepted);
    for (std::uint64_t eventId = 1;
         eventId <= GameplayFeedbackState::HistoryLimit + 1; ++eventId) {
        GameplayFeedbackEvent remote{};
        remote.generation = 20;
        remote.eventId = eventId;
        remote.recipient = 0;
        remote.tick = eventId;
        remote.kind = GameplayFeedbackKind::Queued;
        remote.sequence = eventId;
        REQUIRE(retained.IngestRemote(remote, 0, 20) ==
            GameplayFeedbackStatus::Accepted);
    }
    REQUIRE(retained.History().size() == GameplayFeedbackState::HistoryLimit);
    REQUIRE(retained.History().front().eventId == 2);
    REQUIRE(retained.Loss().evictedHistoryEvents == 1);
}

void TestGameplayFeedbackExecutionHorizon() {
    using namespace echoes::feedback;

    Simulation source({16, 16, 20, 0x46454544484f5249ULL});
    REQUIRE(source.AddPlayer(
        0, Faction::MeridianCompact, ResourcePool{100, 0}));
    const EntityId worker = source.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Worker,
        Vec2::FromTiles(4, 4));
    REQUIRE(worker != 0);

    constexpr std::size_t kSnapshotCurrentTickOffset = 2407;
    constexpr Tick kTerminalTick =
        std::numeric_limits<Tick>::max() / 2;
    std::vector<std::uint8_t> nearHorizon = source.SaveSnapshot();
    REQUIRE(ReadU64(nearHorizon, kSnapshotCurrentTickOffset) == 0);
    WriteU64(
        nearHorizon, kSnapshotCurrentTickOffset, kTerminalTick - 1);
    ResignSnapshot(nearHorizon);
    std::string error;
    std::optional<Simulation> horizon =
        Simulation::LoadSnapshot(nearHorizon, &error);
    REQUIRE(horizon.has_value());
    REQUIRE(horizon->CurrentTick() == kTerminalTick - 1);
    REQUIRE(Simulation::IsExecutableCommandTick(kTerminalTick - 1));
    REQUIRE(!Simulation::IsExecutableCommandTick(kTerminalTick));

    Command executable = MakeCommand(
        kTerminalTick - 1, 0, 1, CommandType::Stop, worker);
    Command terminal = MakeCommand(
        kTerminalTick, 0, 2, CommandType::Stop, worker);
    REQUIRE(horizon->QueueCommand(executable));
    // Historical replay compatibility deliberately keeps this raw envelope
    // admissible even though Step cannot execute the terminal boundary tick.
    REQUIRE(horizon->QueueCommand(terminal));

    GameplayFeedbackState feed;
    REQUIRE(feed.TrackQueued(executable, 60, horizon->CurrentTick()) ==
        GameplayFeedbackStatus::Accepted);
    REQUIRE(feed.PendingCount() == 1);
    REQUIRE(feed.History().size() == 1);
    REQUIRE(feed.TrackQueued(terminal, 60, horizon->CurrentTick()) ==
        GameplayFeedbackStatus::ExecutionUnavailable);
    REQUIRE(feed.PendingCount() == 1);
    REQUIRE(feed.History().size() == 1);
    REQUIRE(feed.Loss().untrackedCommands == 1);

    horizon->Step();
    REQUIRE(horizon->CurrentTick() == kTerminalTick);
    REQUIRE(feed.ObserveFixedStep(*horizon, 0, 60) ==
        GameplayFeedbackStatus::Accepted);
    REQUIRE(feed.PendingCount() == 0);
    const auto applied = horizon->FindCommandResolutionReceipt(0, 1);
    REQUIRE(applied.has_value());
    REQUIRE(applied->outcome == CommandResolutionOutcome::Applied);
    REQUIRE(!horizon->FindCommandResolutionReceipt(0, 2).has_value());
    REQUIRE(std::any_of(
        feed.History().begin(), feed.History().end(),
        [](const GameplayFeedbackEvent& event) {
            return event.sequence == 1 &&
                event.kind == GameplayFeedbackKind::Applied;
        }));
    REQUIRE(std::none_of(
        feed.History().begin(), feed.History().end(),
        [](const GameplayFeedbackEvent& event) {
            return event.sequence == 2;
        }));

    const std::uint64_t terminalChecksum = horizon->StateChecksum();
    const std::size_t terminalHistorySize = feed.History().size();
    horizon->Step();
    REQUIRE(horizon->CurrentTick() == kTerminalTick);
    REQUIRE(horizon->StateChecksum() == terminalChecksum);
    REQUIRE(feed.ObserveFixedStep(*horizon, 0, 60) ==
        GameplayFeedbackStatus::Duplicate);
    REQUIRE(feed.History().size() == terminalHistorySize);
    REQUIRE(std::none_of(
        feed.History().begin(), feed.History().end(),
        [](const GameplayFeedbackEvent& event) {
            return event.sequence == 2 &&
                event.kind == GameplayFeedbackKind::NoEffect;
        }));
}

void TestGameplayFeedbackConstructionAndRepairDeltas() {
    using namespace echoes::feedback;

    SimulationConfig buildConfig{32, 32, 20, 0x464545444255494cULL};
    buildConfig.rules.poweredAegis.connectionRadiusRaw = 8 * kFixedScale;
    Simulation build(buildConfig);
    AddTwoPlayers(build, {2000, 500}, {1000, 500});
    const EntityId core = build.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::CommandCore,
        Vec2::FromTiles(4, 4));
    const EntityId builder = build.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Worker,
        Vec2::FromTiles(7, 8));
    REQUIRE(core && builder);
    const ResourcePool beforeBuild = build.FindPlayer(0)->resources;
    Command buildOrder = MakeCommand(
        build.CurrentTick(), 0, 1, CommandType::Build, builder);
    buildOrder.buildType = EntityType::Dropoff;
    buildOrder.position = Vec2::FromTiles(8, 8);
    GameplayFeedbackState construction;
    REQUIRE(build.QueueCommand(buildOrder));
    REQUIRE(construction.TrackQueued(
        buildOrder, 30, build.CurrentTick()) ==
        GameplayFeedbackStatus::Accepted);

    EntityId site = 0;
    for (int step = 0; step < 140; ++step) {
        build.Step();
        const GameplayFeedbackStatus status =
            construction.ObserveFixedStep(build, 0, 30);
        REQUIRE(status == GameplayFeedbackStatus::Accepted);
        if (site == 0) {
            const auto created = std::find_if(
                construction.History().begin(), construction.History().end(),
                [](const GameplayFeedbackEvent& event) {
                    return event.kind ==
                        GameplayFeedbackKind::ConstructionCreated;
                });
            if (created != construction.History().end()) site = created->target;
        }
        if (site != 0 && build.FindEntity(site) != nullptr &&
            build.FindEntity(site)->completed) break;
    }
    REQUIRE(site != 0);
    const Entity* completedSite = build.FindEntity(site);
    REQUIRE(completedSite != nullptr && completedSite->completed);
    const ResourcePool buildCost{
        beforeBuild.material - build.FindPlayer(0)->resources.material,
        beforeBuild.dawnshards - build.FindPlayer(0)->resources.dawnshards};
    std::int64_t observedProgress = 0;
    std::size_t progressEvents = 0;
    bool sawCreated = false;
    bool sawCompleted = false;
    for (const GameplayFeedbackEvent& event : construction.History()) {
        if (event.kind == GameplayFeedbackKind::ConstructionCreated) {
            sawCreated = true;
            REQUIRE(event.target == site);
            REQUIRE((event.resourcesSpent == FeedbackResourceDelta{
                buildCost.material, buildCost.dawnshards}));
        } else if (event.kind ==
                   GameplayFeedbackKind::ConstructionProgressed) {
            ++progressEvents;
            observedProgress += event.progressDelta;
        } else if (event.kind ==
                   GameplayFeedbackKind::ConstructionCompleted) {
            sawCompleted = true;
            REQUIRE(event.target == site);
        }
    }
    REQUIRE(sawCreated && sawCompleted);
    REQUIRE(observedProgress == completedSite->constructionRequired);
    REQUIRE(progressEvents > 0 && progressEvents <= 6);
    REQUIRE(!construction.Loss().reseedRequired);

    SimulationConfig repairConfig{24, 24, 20, 0x4645454452455041ULL};
    repairConfig.rules.poweredAegis.connectionRadiusRaw = 8 * kFixedScale;
    Simulation repair(repairConfig);
    AddTwoPlayers(repair, {1000, 500}, {1000, 500});
    const EntityId repairCore = repair.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::CommandCore,
        Vec2::FromTiles(4, 4));
    const EntityId damaged = repair.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Dropoff,
        Vec2::FromTiles(10, 4), 490);
    const EntityId worker = repair.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Worker,
        Vec2::FromTiles(10, 5));
    REQUIRE(repairCore && damaged && worker);
    const std::int32_t healthBefore = repair.FindEntity(damaged)->hitPoints;
    const std::int32_t matterBefore = repair.FindPlayer(0)->resources.material;
    Command repairOrder = MakeCommand(
        repair.CurrentTick(), 0, 1, CommandType::Repair, worker);
    repairOrder.target = damaged;
    GameplayFeedbackState repairs;
    REQUIRE(repair.QueueCommand(repairOrder));
    REQUIRE(repairs.TrackQueued(
        repairOrder, 31, repair.CurrentTick()) ==
        GameplayFeedbackStatus::Accepted);
    // The feed waits through its bounded inactive window before publishing a
    // short repair burst, preserving one exact aggregate instead of flooding.
    for (int step = 0; step < 45; ++step) {
        repair.Step();
        REQUIRE(repairs.ObserveFixedStep(repair, 0, 31) ==
            GameplayFeedbackStatus::Accepted);
    }
    std::int64_t restored = 0;
    std::int64_t spent = 0;
    std::size_t repairEvents = 0;
    for (const GameplayFeedbackEvent& event : repairs.History()) {
        if (event.kind != GameplayFeedbackKind::RepairApplied) continue;
        ++repairEvents;
        restored += event.healthDelta;
        spent += event.resourcesSpent.material;
        REQUIRE(event.actor == worker && event.target == damaged);
    }
    REQUIRE(repairEvents == 1);
    REQUIRE(restored ==
        repair.FindEntity(damaged)->hitPoints - healthBefore);
    REQUIRE(spent == matterBefore - repair.FindPlayer(0)->resources.material);
    REQUIRE(restored == 10 && spent == 1);
}

void TestGameplayFeedbackBlockedProductionRecovery() {
    using namespace echoes::feedback;

    SimulationConfig config{24, 24, 20, 0x4645454450524f44ULL};
    auto& workerRules = config.rules.archetypes[0][
        static_cast<std::size_t>(EntityType::Worker)];
    workerRules.cost = {10, 0};
    workerRules.productionTicks = 2;
    Simulation sim(config);
    REQUIRE(sim.AddPlayer(
        0, Faction::MeridianCompact, ResourcePool{100, 0}));
    const EntityId producer = sim.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::CommandCore,
        Vec2::FromTiles(12, 12));
    REQUIRE(producer != 0);
    Command produce = MakeCommand(
        sim.CurrentTick(), 0, 1, CommandType::Produce, producer);
    produce.buildType = EntityType::Worker;
    GameplayFeedbackState feed;
    REQUIRE(sim.QueueCommand(produce));
    REQUIRE(feed.TrackQueued(produce, 40, sim.CurrentTick()) ==
        GameplayFeedbackStatus::Accepted);
    sim.Step();
    REQUIRE(feed.ObserveFixedStep(sim, 0, 40) ==
        GameplayFeedbackStatus::Accepted);
    Command waiting = MakeCommand(
        sim.CurrentTick(), 0, 2, CommandType::Produce, producer);
    waiting.buildType = EntityType::Worker;
    REQUIRE(sim.QueueCommand(waiting));
    REQUIRE(feed.TrackQueued(waiting, 40, sim.CurrentTick()) ==
        GameplayFeedbackStatus::Accepted);
    for (std::int32_t y = 0; y < 24; ++y) {
        for (std::int32_t x = 0; x < 24; ++x) {
            REQUIRE(sim.SetTerrainTile(x, y, Terrain::Blocked));
        }
    }
    for (int step = 0; step < 100; ++step) {
        sim.Step();
        REQUIRE(feed.ObserveFixedStep(sim, 0, 40) ==
            GameplayFeedbackStatus::Accepted);
    }
    const auto blocked = std::find_if(
        feed.History().begin(), feed.History().end(),
        [](const GameplayFeedbackEvent& event) {
            return event.kind ==
                GameplayFeedbackKind::ProductionSpawnBlocked;
        });
    REQUIRE(blocked != feed.History().end());
    REQUIRE(blocked->recipient == 0 && blocked->producer == producer);
    REQUIRE(blocked->itemId != 0);
    const ProductionItemId blockedItemId = blocked->itemId;

    REQUIRE(sim.SetTerrainTile(14, 14, Terrain::Open));
    REQUIRE(sim.SetTerrainTile(15, 14, Terrain::Open));
    REQUIRE(sim.SetTerrainTile(14, 15, Terrain::Open));
    REQUIRE(sim.SetTerrainTile(15, 15, Terrain::Open));
    for (int step = 0; step < 3; ++step) {
        sim.Step();
        REQUIRE(feed.ObserveFixedStep(sim, 0, 40) ==
            GameplayFeedbackStatus::Accepted);
    }
    const auto resumed = std::find_if(
        feed.History().begin(), feed.History().end(),
        [](const GameplayFeedbackEvent& event) {
            return event.kind ==
                GameplayFeedbackKind::ProductionSpawnResumed;
        });
    const auto completed = std::find_if(
        feed.History().begin(), feed.History().end(),
        [](const GameplayFeedbackEvent& event) {
            return event.kind == GameplayFeedbackKind::ProductionCompleted;
        });
    REQUIRE(resumed != feed.History().end());
    REQUIRE(completed != feed.History().end());
    REQUIRE(resumed->itemId == blockedItemId);
    REQUIRE(completed->itemId == blockedItemId);
    REQUIRE(completed->actor != 0);
    REQUIRE(std::any_of(
        feed.History().begin(), feed.History().end(),
        [](const GameplayFeedbackEvent& event) {
            return event.kind == GameplayFeedbackKind::ProductionQueued;
        }));
    REQUIRE(std::any_of(
        feed.History().begin(), feed.History().end(),
        [](const GameplayFeedbackEvent& event) {
            return event.kind == GameplayFeedbackKind::ProductionActivated;
        }));
    REQUIRE(feed.History().size() < GameplayFeedbackState::HistoryLimit);
}

void TestGameplayFeedbackRemoteLineageAndLoss() {
    using namespace echoes::feedback;

    GameplayFeedbackState remote;
    REQUIRE(remote.BeginRemoteStream(50, 0, 40, true) ==
        GameplayFeedbackStatus::Accepted);
    REQUIRE(remote.Generation() == 50 && remote.LastEventId() == 40);
    REQUIRE(remote.Loss().retainedHistoryWasTruncated);
    GameplayFeedbackEvent event{};
    event.generation = 50;
    event.eventId = 41;
    event.recipient = 0;
    event.tick = 100;
    event.kind = GameplayFeedbackKind::Queued;
    event.sequence = 1;
    REQUIRE(remote.IngestRemote(event, 0, 50) ==
        GameplayFeedbackStatus::Accepted);
    REQUIRE(remote.IngestRemote(event, 0, 50) ==
        GameplayFeedbackStatus::Duplicate);

    GameplayFeedbackEvent gap = event;
    gap.eventId = 43;
    gap.sequence = 3;
    REQUIRE(remote.IngestRemote(gap, 0, 50) ==
        GameplayFeedbackStatus::OutOfOrder);
    REQUIRE(remote.LastEventId() == 41);
    REQUIRE(remote.Loss().reseedRequired);
    GameplayFeedbackEvent wrongOwner = event;
    wrongOwner.eventId = 42;
    wrongOwner.sequence = 2;
    wrongOwner.recipient = 1;
    REQUIRE(remote.IngestRemote(wrongOwner, 0, 50) ==
        GameplayFeedbackStatus::WrongRecipient);
    GameplayFeedbackEvent wrongGeneration = event;
    wrongGeneration.eventId = 42;
    wrongGeneration.sequence = 2;
    wrongGeneration.generation = 51;
    REQUIRE(remote.IngestRemote(wrongGeneration, 0, 50) ==
        GameplayFeedbackStatus::WrongGeneration);
    GameplayFeedbackEvent validNext = event;
    validNext.eventId = 42;
    validNext.sequence = 2;
    std::vector<GameplayFeedbackEvent> malformedEvents;
    const auto AddMalformed = [&](const auto& mutate) {
        GameplayFeedbackEvent malformed = validNext;
        mutate(malformed);
        malformedEvents.push_back(malformed);
    };
    AddMalformed([](GameplayFeedbackEvent& malformed) {
        malformed.kind = static_cast<GameplayFeedbackKind>(255);
    });
    AddMalformed([](GameplayFeedbackEvent& malformed) {
        malformed.commandType = static_cast<CommandType>(255);
    });
    AddMalformed([](GameplayFeedbackEvent& malformed) {
        malformed.resolutionOutcome =
            static_cast<CommandResolutionOutcome>(255);
    });
    AddMalformed([](GameplayFeedbackEvent& malformed) {
        malformed.entityType = static_cast<EntityType>(255);
    });
    AddMalformed([](GameplayFeedbackEvent& malformed) {
        malformed.productionBlockReason =
            static_cast<ProductionStartBlockReason>(255);
    });
    AddMalformed([](GameplayFeedbackEvent& malformed) {
        malformed.resourcesSpent.material = -1;
    });
    AddMalformed([](GameplayFeedbackEvent& malformed) {
        malformed.resourcesRefunded.dawnshards = -1;
    });
    AddMalformed([](GameplayFeedbackEvent& malformed) {
        malformed.healthDelta = -1;
    });
    AddMalformed([](GameplayFeedbackEvent& malformed) {
        malformed.progressDelta = -1;
    });
    AddMalformed([](GameplayFeedbackEvent& malformed) {
        malformed.sequence = 0;
    });
    AddMalformed([](GameplayFeedbackEvent& malformed) {
        malformed.kind = GameplayFeedbackKind::Applied;
        malformed.resolutionOutcome = CommandResolutionOutcome::NoEffect;
    });
    AddMalformed([](GameplayFeedbackEvent& malformed) {
        malformed.kind = GameplayFeedbackKind::NoEffect;
        malformed.resolutionOutcome = CommandResolutionOutcome::Applied;
    });
    AddMalformed([](GameplayFeedbackEvent& malformed) {
        malformed.kind = GameplayFeedbackKind::ConstructionCreated;
        malformed.target = 0;
    });
    AddMalformed([](GameplayFeedbackEvent& malformed) {
        malformed.kind = GameplayFeedbackKind::ConstructionProgressed;
        malformed.actor = 1;
        malformed.target = 2;
        malformed.progressDelta = 0;
    });
    AddMalformed([](GameplayFeedbackEvent& malformed) {
        malformed.kind = GameplayFeedbackKind::RepairApplied;
        malformed.actor = 1;
        malformed.target = 2;
        malformed.healthDelta = 0;
    });
    AddMalformed([](GameplayFeedbackEvent& malformed) {
        malformed.kind = GameplayFeedbackKind::ProductionQueued;
        malformed.producer = 1;
        malformed.itemId = 0;
    });
    const std::size_t historyBeforeMalformed = remote.History().size();
    for (const GameplayFeedbackEvent& malformed : malformedEvents) {
        REQUIRE(remote.IngestRemote(malformed, 0, 50) ==
            GameplayFeedbackStatus::InvalidEvent);
        REQUIRE(remote.LastEventId() == 41);
        REQUIRE(remote.History().size() == historyBeforeMalformed);
    }
    GameplayFeedbackEvent next = event;
    next.eventId = 42;
    next.sequence = 2;
    REQUIRE(remote.IngestRemote(next, 0, 50) ==
        GameplayFeedbackStatus::Accepted);

    GameplayFeedbackLoss sourceLoss{};
    sourceLoss.reseedRequired = true;
    sourceLoss.retainedHistoryWasTruncated = true;
    sourceLoss.evictedHistoryEvents = 7;
    sourceLoss.untrackedCommands = 3;
    sourceLoss.lostTransientReceipts = 2;
    sourceLoss.missedObservationTicks = 5;
    REQUIRE(remote.ReportRemoteObservationLoss(sourceLoss, 0, 50) ==
        GameplayFeedbackStatus::Accepted);
    REQUIRE(remote.ReportRemoteObservationLoss(sourceLoss, 0, 50) ==
        GameplayFeedbackStatus::NoChange);
    REQUIRE(remote.ReportRemoteObservationLoss(sourceLoss, 1, 50) ==
        GameplayFeedbackStatus::WrongRecipient);
    REQUIRE(remote.ReportRemoteObservationLoss(sourceLoss, 0, 51) ==
        GameplayFeedbackStatus::WrongGeneration);
    REQUIRE(remote.LastEventId() == 42);
    REQUIRE(remote.Loss().evictedHistoryEvents == 7);
    REQUIRE(remote.Loss().untrackedCommands == 3);
    REQUIRE(remote.Loss().lostTransientReceipts == 2);
    REQUIRE(remote.Loss().missedObservationTicks == 5);

    // Ordinary ingestion cannot switch generation or establish a new floor.
    GameplayFeedbackEvent future = next;
    future.generation = 51;
    future.eventId = 100;
    REQUIRE(remote.IngestRemote(future, 0, 51) ==
        GameplayFeedbackStatus::WrongGeneration);
    REQUIRE(remote.Generation() == 50 && remote.LastEventId() == 42);
    REQUIRE(remote.BeginRemoteStream(51, 1, 99) ==
        GameplayFeedbackStatus::Accepted);
    REQUIRE(remote.History().empty());
    REQUIRE(remote.Recipient() == 1 && remote.LastEventId() == 99);
    REQUIRE(!remote.Loss().reseedRequired);
    future.recipient = 1;
    future.sequence = 10;
    REQUIRE(remote.IngestRemote(future, 1, 51) ==
        GameplayFeedbackStatus::Accepted);
    remote.Reset();
    REQUIRE(remote.Generation() == 0 && remote.History().empty());
}
