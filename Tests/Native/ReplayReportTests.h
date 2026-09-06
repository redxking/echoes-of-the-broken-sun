#pragma once

// Included by SimCoreTests.cpp after its REQUIRE and MakeCommand helpers.
// Kept separate so the P2 replay/report package does not contend with the
// active P0/P1 edits in that large registry source.
void TestReplayReportAuthorityAndContinuation() {
    SimulationConfig config{32, 32, 20, 0x52505441555448ULL};
    auto& soldierRules = config.rules.archetypes
        [static_cast<std::size_t>(Faction::MeridianCompact)]
        [static_cast<std::size_t>(EntityType::Soldier)];
    soldierRules.productionTicks = 1;
    soldierRules.cost = {};
    soldierRules.attackDamage = 5000;
    soldierRules.attackRangeRaw = 3 * kFixedScale;
    soldierRules.attackPeriodTicks = 1;

    Simulation simulation(config);
    REQUIRE(simulation.AddPlayer(
        0, Faction::MeridianCompact, ResourcePool{1000, 1000}));
    REQUIRE(simulation.AddPlayer(
        1, Faction::KharuunAssemblies, ResourcePool{1000, 1000}));
    const EntityId localCore = simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::CommandCore,
        Vec2::FromTiles(2, 2));
    const EntityId enemyCore = simulation.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::CommandCore,
        Vec2::FromTiles(20, 20));
    const EntityId barracks = simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Barracks,
        Vec2::FromTiles(4, 4));
    const EntityId gatherer = simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Worker,
        Vec2::FromTiles(3, 2));
    const EntityId wellWorker = simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Worker,
        Vec2::FromTiles(6, 6));
    const EntityId attacker = simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Soldier,
        Vec2::FromTiles(19, 20));
    const EntityId resource = simulation.SpawnResourceNode(
        Vec2::FromTiles(3, 2), 100);
    const EntityId well = simulation.SpawnFutureWell(Vec2::FromTiles(7, 6));
    REQUIRE(localCore != 0 && enemyCore != 0 && barracks != 0 &&
            gatherer != 0 && wellWorker != 0 && attacker != 0 &&
            resource != 0 && well != 0);
    simulation.CaptureReplayBaseline();

    Command gather = MakeCommand(0, 0, 1, CommandType::Gather, gatherer);
    gather.target = resource;
    REQUIRE(simulation.QueueCommand(gather));
    Command produce = MakeCommand(0, 0, 2, CommandType::Produce, barracks);
    produce.buildType = EntityType::Soldier;
    REQUIRE(simulation.QueueCommand(produce));
    Command preserve = MakeCommand(
        0, 0, 3, CommandType::FutureWell, wellWorker);
    preserve.target = well;
    preserve.wellChoice = FutureWellChoice::Preserve;
    REQUIRE(simulation.QueueCommand(preserve));
    Command attack = MakeCommand(500, 0, 4, CommandType::Attack, attacker);
    attack.target = enemyCore;
    REQUIRE(simulation.QueueCommand(attack));
    simulation.Step(502);
    REQUIRE(simulation.Outcome() == MatchOutcome::Player0Victory);

    const ReplayRecord replay = simulation.ExportReplay();
    std::string error;
    const auto report = Simulation::BuildMatchReport(replay, &error);
    REQUIRE(report.has_value());
    REQUIRE(error.empty());
    REQUIRE(report->baselineTick == 0);
    REQUIRE(report->durationTicks == 502);
    REQUIRE(report->outcome == MatchOutcome::Player0Victory);
    REQUIRE(report->finalChecksum == simulation.StateChecksum());
    REQUIRE(report->players[0].unitsTrained == 1);
    REQUIRE(report->players[0].unitsLost == 0);
    REQUIRE(report->players[0].materialCollected > 0);
    REQUIRE(report->players[0].materialDelivered > 0);
    REQUIRE(report->players[0].materialCollected >=
            report->players[0].materialDelivered);
    REQUIRE(report->players[0].admittedCommands == 4);
    REQUIRE(report->commands.size() == 4);
    REQUIRE(std::all_of(
        report->commands.begin(), report->commands.end(),
        [](const MatchCommandRecord& record) { return record.resolved; }));
    REQUIRE(report->wellDecisions.size() == 1);
    REQUIRE(report->wellDecisions.front().wellId == well);
    REQUIRE(report->wellDecisions.front().choice == FutureWellChoice::Preserve);
    REQUIRE(std::any_of(
        report->events.begin(), report->events.end(),
        [](const ReplayTimelineEvent& event) {
            return event.type == ReplayTimelineEventType::FirstCombatContact;
        }));
    REQUIRE(std::any_of(
        report->events.begin(), report->events.end(),
        [enemyCore](const ReplayTimelineEvent& event) {
            return event.type == ReplayTimelineEventType::CommandCoreLoss &&
                event.subject == enemyCore;
        }));
    REQUIRE(!report->apmSamples.empty());
    REQUIRE(report->timelineSamples.front().tick == 0);
    REQUIRE(report->timelineSamples.back().tick == replay.finalTick);

    // Differentially preserve the former eager-admission semantics while the
    // production replay path streams commands at their execution frontier.
    SimulationConfig streamConfig{24, 24, 20, 0x53545245414d5250ULL};
    Simulation streamAuthority(streamConfig);
    REQUIRE(streamAuthority.AddPlayer(
        0, Faction::MeridianCompact, ResourcePool{1000, 1000}));
    REQUIRE(streamAuthority.AddPlayer(
        1, Faction::KharuunAssemblies, ResourcePool{1000, 1000}));
    REQUIRE(streamAuthority.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::CommandCore,
        Vec2::FromTiles(2, 2)) != 0);
    REQUIRE(streamAuthority.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::CommandCore,
        Vec2::FromTiles(20, 20)) != 0);
    const EntityId streamActor0 = streamAuthority.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Soldier,
        Vec2::FromTiles(5, 5));
    const EntityId streamActor1 = streamAuthority.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::Soldier,
        Vec2::FromTiles(18, 18));
    REQUIRE(streamActor0 != 0 && streamActor1 != 0);
    Command baselinePending = MakeCommand(
        3, 0, 1, CommandType::Hold, streamActor0);
    REQUIRE(streamAuthority.QueueCommand(baselinePending));
    streamAuthority.CaptureReplayBaseline();

    std::uint64_t streamSequence0 = 2;
    std::uint64_t streamSequence1 = 1;
    std::uint32_t resolvedStreamCommands0 = 0;
    std::uint32_t resolvedStreamCommands1 = 0;
    for (Tick tick = 3; tick <= 130; ++tick) {
        for (int sameTick = 0; sameTick < 2; ++sameTick) {
            REQUIRE(streamAuthority.QueueCommand(MakeCommand(
                tick, 0, streamSequence0++, CommandType::Hold,
                streamActor0)));
            REQUIRE(streamAuthority.QueueCommand(MakeCommand(
                tick, 1, streamSequence1++, CommandType::Hold,
                streamActor1)));
            ++resolvedStreamCommands0;
            ++resolvedStreamCommands1;
        }
    }
    constexpr Tick streamFinalTick = 160;
    REQUIRE(streamAuthority.QueueCommand(MakeCommand(
        streamFinalTick, 0, streamSequence0++, CommandType::Hold,
        streamActor0)));
    REQUIRE(streamAuthority.QueueCommand(MakeCommand(
        streamFinalTick, 1, streamSequence1++, CommandType::Hold,
        streamActor1)));
    REQUIRE(streamAuthority.QueueCommand(MakeCommand(
        streamFinalTick + 10, 0, streamSequence0++, CommandType::Hold,
        streamActor0)));
    REQUIRE(streamAuthority.QueueCommand(MakeCommand(
        streamFinalTick + 10, 1, streamSequence1++, CommandType::Hold,
        streamActor1)));
    streamAuthority.Step(streamFinalTick);
    const ReplayRecord streamReplay = streamAuthority.ExportReplay(&error);
    REQUIRE(error.empty());

    auto eagerReference = Simulation::LoadSnapshot(
        streamReplay.initialSnapshot, &error);
    REQUIRE(eagerReference.has_value());
    for (const Command& command : streamReplay.commands) {
        REQUIRE(eagerReference->QueueCommand(command, &error));
    }
    eagerReference->Step(
        streamReplay.finalTick - eagerReference->CurrentTick());
    REQUIRE(eagerReference->StateChecksum() == streamReplay.finalChecksum);

    const auto streamedReplay = Simulation::ReplayToEnd(streamReplay, &error);
    REQUIRE(streamedReplay.has_value());
    REQUIRE(error.empty());
    REQUIRE(streamedReplay->StateChecksum() == eagerReference->StateChecksum());
    REQUIRE(streamedReplay->SaveSnapshot() == eagerReference->SaveSnapshot());
    REQUIRE(streamedReplay->PendingCommands().size() == 4);
    const ReplayRecord reexportedStream = streamedReplay->ExportReplay(&error);
    REQUIRE(error.empty());
    REQUIRE(reexportedStream.initialSnapshot == streamReplay.initialSnapshot);
    REQUIRE(reexportedStream.commands == streamReplay.commands);
    REQUIRE(reexportedStream.finalTick == streamReplay.finalTick);
    REQUIRE(reexportedStream.finalChecksum == streamReplay.finalChecksum);

    const auto streamReport =
        Simulation::BuildMatchReport(streamReplay, &error);
    REQUIRE(streamReport.has_value());
    REQUIRE(error.empty());
    REQUIRE(streamReport->commands.size() == streamReplay.commands.size());
    REQUIRE(streamReport->players[0].admittedCommands ==
            resolvedStreamCommands0);
    REQUIRE(streamReport->players[1].admittedCommands ==
            resolvedStreamCommands1);
    REQUIRE(std::count_if(
                streamReport->commands.begin(), streamReport->commands.end(),
                [](const MatchCommandRecord& record) {
                    return !record.resolved;
                }) == 4);
    REQUIRE(streamReport->timelineSamples.front().tick == 0);
    REQUIRE(streamReport->timelineSamples.back().tick == streamFinalTick);
    REQUIRE(streamReport->apmSamples.size() == 2);
    REQUIRE(streamReport->apmSamples[0].commandCount ==
            resolvedStreamCommands0);
    REQUIRE(streamReport->apmSamples[1].commandCount ==
            resolvedStreamCommands1);

    // A baseline can legitimately carry more retained same-tick receipts than
    // the presentation receipt deque keeps. Report reconstruction consumes a
    // tick-local outcome sink so a recorded command cannot be evicted before
    // its authoritative outcome is counted.
    Simulation receiptAuthority(streamConfig);
    REQUIRE(receiptAuthority.AddPlayer(
        0, Faction::MeridianCompact, ResourcePool{1000, 1000}));
    REQUIRE(receiptAuthority.AddPlayer(
        1, Faction::KharuunAssemblies, ResourcePool{1000, 1000}));
    REQUIRE(receiptAuthority.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::CommandCore,
        Vec2::FromTiles(2, 2)) != 0);
    REQUIRE(receiptAuthority.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::CommandCore,
        Vec2::FromTiles(20, 20)) != 0);
    const EntityId receiptActor = receiptAuthority.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Soldier,
        Vec2::FromTiles(5, 5));
    REQUIRE(receiptActor != 0);
    for (std::uint64_t sequence = 2;
         sequence < 2 + kMaximumCommandResolutionReceipts;
         ++sequence) {
        REQUIRE(receiptAuthority.QueueCommand(MakeCommand(
            0, 0, sequence, CommandType::Hold, receiptActor)));
    }
    receiptAuthority.CaptureReplayBaseline();
    REQUIRE(receiptAuthority.QueueCommand(MakeCommand(
        0, 0, 1, CommandType::Hold, receiptActor)));
    receiptAuthority.Step();
    REQUIRE(!receiptAuthority.FindCommandResolutionReceipt(0, 1).has_value());
    const ReplayRecord receiptReplay = receiptAuthority.ExportReplay(&error);
    REQUIRE(error.empty());
    REQUIRE(Simulation::ReplayToEnd(receiptReplay, &error).has_value());
    const auto receiptReport =
        Simulation::BuildMatchReport(receiptReplay, &error);
    REQUIRE(receiptReport.has_value());
    REQUIRE(error.empty());
    REQUIRE(receiptReport->commands.size() == 1);
    REQUIRE(receiptReport->commands.front().resolved);
    REQUIRE(receiptReport->commands.front().outcome ==
            CommandResolutionOutcome::Applied);
    REQUIRE(receiptReport->players[0].admittedCommands == 1);

    static_assert(std::is_same_v<
        decltype(MatchApmSample{}.actionsPerMinuteX100), std::uint64_t>);
    constexpr std::uint64_t MaximumCapacityOneTickApmX100 =
        static_cast<std::uint64_t>(kMaximumCommandLogEntries) *
        kMatchReportApmIntervalTicks * 100U;
    REQUIRE(MaximumCapacityOneTickApmX100 >
            std::numeric_limits<std::uint32_t>::max());

    int replaySuccessCancellationChecks = 0;
    REQUIRE(Simulation::ReplayToEnd(
        replay,
        &error,
        [&replaySuccessCancellationChecks] {
            ++replaySuccessCancellationChecks;
            return false;
        }).has_value());
    int replayFinalCancellationCheck = 0;
    REQUIRE(!Simulation::ReplayToEnd(
        replay,
        &error,
        [&] {
            return ++replayFinalCancellationCheck ==
                replaySuccessCancellationChecks;
        }).has_value());
    REQUIRE(error == "replay validation cancelled");

    int reportSuccessCancellationChecks = 0;
    REQUIRE(Simulation::BuildMatchReport(
        replay,
        &error,
        [&reportSuccessCancellationChecks] {
            ++reportSuccessCancellationChecks;
            return false;
        }).has_value());
    int reportFinalCancellationCheck = 0;
    REQUIRE(!Simulation::BuildMatchReport(
        replay,
        &error,
        [&] {
            return ++reportFinalCancellationCheck ==
                reportSuccessCancellationChecks;
        }).has_value());
    REQUIRE(error == "replay validation cancelled");

    const auto cancelledBeforeValidation = Simulation::BuildMatchReport(
        replay, &error, [] { return true; });
    REQUIRE(!cancelledBeforeValidation.has_value());
    REQUIRE(error == "replay validation cancelled");
    ReplayRecord enormousTickReplay = replay;
    enormousTickReplay.finalTick =
        std::numeric_limits<Tick>::max() / 2U;
    const ReplayRecord replayBeforeCancellation = enormousTickReplay;
    int cancellationChecks = 0;
    const auto cancelledMidValidation = Simulation::BuildMatchReport(
        enormousTickReplay, &error,
        [&cancellationChecks] { return ++cancellationChecks >= 4; });
    REQUIRE(!cancelledMidValidation.has_value());
    REQUIRE(error == "replay validation cancelled");
    REQUIRE(cancellationChecks >= 3);
    REQUIRE(enormousTickReplay.version == replayBeforeCancellation.version);
    REQUIRE(enormousTickReplay.initialSnapshot ==
            replayBeforeCancellation.initialSnapshot);
    REQUIRE(enormousTickReplay.commands == replayBeforeCancellation.commands);
    REQUIRE(enormousTickReplay.finalTick == replayBeforeCancellation.finalTick);
    REQUIRE(enormousTickReplay.finalChecksum ==
            replayBeforeCancellation.finalChecksum);
    REQUIRE(enormousTickReplay.forfeitingPlayer ==
            replayBeforeCancellation.forfeitingPlayer);

    int snapshotCancellationChecks = 0;
    const auto cancelledSnapshot = Simulation::LoadSnapshot(
        replay.initialSnapshot,
        &error,
        kDefaultHostilityMasks,
        [&snapshotCancellationChecks] {
            return ++snapshotCancellationChecks >= 3;
        });
    REQUIRE(!cancelledSnapshot.has_value());
    REQUIRE(error == "snapshot load cancelled");
    REQUIRE(snapshotCancellationChecks >= 3);

    const auto restored = Simulation::LoadSnapshot(
        simulation.SaveSnapshot(), &error);
    REQUIRE(restored.has_value());
    Simulation resumed = *restored;
    REQUIRE(resumed.ContinueReplayRecording(replay, &error));
    REQUIRE(error.empty());
    REQUIRE(resumed.ExportReplay().initialSnapshot == replay.initialSnapshot);
    REQUIRE(resumed.ExportReplay().commands == replay.commands);

    ReplayRecord corrupt = replay;
    corrupt.finalChecksum ^= 1U;
    REQUIRE(!Simulation::BuildMatchReport(corrupt, &error).has_value());
    REQUIRE(error == "replay final checksum does not match");
    REQUIRE(!resumed.ContinueReplayRecording(corrupt, &error));
    REQUIRE(error.find("replay prefix is invalid") != std::string::npos);

    Simulation wrongState = *restored;
    REQUIRE(wrongState.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Worker,
        Vec2::FromTiles(10, 10)) != 0);
    REQUIRE(!wrongState.ContinueReplayRecording(replay, &error));
    REQUIRE(error == "replay prefix state does not match restored state");

    ReplayRecord legacy = replay;
    legacy.version = kLegacyReplayVersion;
    REQUIRE(Simulation::ReplayToEnd(legacy, &error).has_value());

    Simulation conceded(config);
    REQUIRE(conceded.AddPlayer(
        0, Faction::MeridianCompact, ResourcePool{1000, 1000}));
    REQUIRE(conceded.AddPlayer(
        1, Faction::KharuunAssemblies, ResourcePool{1000, 1000}));
    REQUIRE(conceded.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::CommandCore,
        Vec2::FromTiles(2, 2)) != 0);
    REQUIRE(conceded.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::CommandCore,
        Vec2::FromTiles(20, 20)) != 0);
    conceded.CaptureReplayBaseline();
    conceded.Step(9);
    REQUIRE(conceded.ForfeitPlayer(0));
    const ReplayRecord concession = conceded.ExportReplay(&error);
    REQUIRE(error.empty());
    REQUIRE(concession.version == kReplayVersion);
    REQUIRE(concession.forfeitingPlayer == 0);
    const auto concessionReport =
        Simulation::BuildMatchReport(concession, &error);
    REQUIRE(concessionReport.has_value());
    REQUIRE(concessionReport->outcome == MatchOutcome::Player1Victory);
    REQUIRE(concessionReport->outcomeCause == MatchOutcomeCause::PlayerForfeit);
    REQUIRE(concessionReport->forfeitingPlayer == 0);
    ReplayRecord invalidLegacyConcession = concession;
    invalidLegacyConcession.version = kLegacyReplayVersion;
    REQUIRE(!Simulation::ReplayToEnd(invalidLegacyConcession, &error).has_value());
    REQUIRE(error == "replay forfeit marker is invalid");
}
