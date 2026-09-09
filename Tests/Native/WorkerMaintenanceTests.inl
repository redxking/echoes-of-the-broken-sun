// Author and owner: Angelis Pseftis
void TestWorkerMaintenanceAdmissionAndAssist() {
    SimulationConfig config{40, 40, 20, 0x4d41494e5441494eULL};
    config.rules.poweredAegis.connectionRadiusRaw = 8 * kFixedScale;
    Simulation sim(config);
    AddTwoPlayers(sim, {2000, 500}, {2000, 500});
    const auto core = sim.SpawnEntity(0, Faction::MeridianCompact,
        EntityType::CommandCore, Vec2::FromTiles(4, 4));
    const auto link = sim.SpawnEntity(0, Faction::MeridianCompact,
        EntityType::Dropoff, Vec2::FromTiles(10, 4), 100);
    const auto worker = sim.SpawnEntity(0, Faction::MeridianCompact,
        EntityType::Worker, Vec2::FromTiles(10, 5));
    const auto builder = sim.SpawnEntity(0, Faction::MeridianCompact,
        EntityType::Worker, Vec2::FromTiles(7, 8));
    const auto assistant = sim.SpawnEntity(0, Faction::MeridianCompact,
        EntityType::Worker, Vec2::FromTiles(8, 7));
    const auto disconnected = sim.SpawnEntity(0, Faction::MeridianCompact,
        EntityType::Worker, Vec2::FromTiles(35, 35));
    const auto soldier = sim.SpawnEntity(0, Faction::MeridianCompact,
        EntityType::Soldier, Vec2::FromTiles(9, 6));
    const auto opposingCore = sim.SpawnEntity(1, Faction::KharuunAssemblies,
        EntityType::CommandCore, Vec2::FromTiles(30, 30));
    const auto tender = sim.SpawnEntity(1, Faction::KharuunAssemblies,
        EntityType::Worker, Vec2::FromTiles(29, 30));
    REQUIRE(core && link && worker && builder && assistant && disconnected &&
        soldier && opposingCore && tender);
    REQUIRE(sim.ValidateRepair(0, worker, link) == RepairResult::Valid);
    REQUIRE(sim.ValidateRepair(0, soldier, link) == RepairResult::InvalidWorker);
    REQUIRE(sim.ValidateRepair(0, tender, link) == RepairResult::InvalidWorker);
    REQUIRE(sim.ValidateRepair(0, worker, worker) == RepairResult::InvalidTarget);
    REQUIRE(sim.ValidateRepair(0, worker, opposingCore) == RepairResult::InvalidTarget);
    REQUIRE(sim.ValidateRepair(0, worker, soldier) == RepairResult::Undamaged);
    REQUIRE(sim.ValidateRepair(0, disconnected, link) == RepairResult::Disconnected);
    REQUIRE(sim.ValidateConstructionAssist(0, assistant, core) ==
        ConstructionAssistResult::SiteComplete);
    REQUIRE(sim.ValidateConstructionAssist(0, assistant, opposingCore) ==
        ConstructionAssistResult::InvalidSite);
    REQUIRE(sim.ValidateConstructionAssist(0, soldier, link) ==
        ConstructionAssistResult::InvalidWorker);

    sim.CaptureReplayBaseline();
    const auto hpBefore = sim.FindEntity(link)->hitPoints;
    const auto fundsBefore = sim.FindPlayer(0)->resources.material;
    auto repair = MakeCommand(sim.CurrentTick(), 0, 1, CommandType::Repair, worker);
    repair.target = link;
    REQUIRE(sim.QueueCommand(repair));
    sim.Step(20);
    REQUIRE(sim.FindEntity(link)->hitPoints == hpBefore + 10);
    REQUIRE(sim.FindPlayer(0)->resources.material == fundsBefore - 1);
    REQUIRE(sim.QueueCommand(MakeCommand(sim.CurrentTick(), 0, 2, CommandType::Stop, worker)));
    auto build = MakeCommand(sim.CurrentTick(), 0, 3, CommandType::Build, builder);
    build.buildType = EntityType::Dropoff;
    build.position = Vec2::FromTiles(8, 8);
    REQUIRE(sim.QueueCommand(build));
    sim.Step();
    auto player = sim.CreatePlayerView(0);
    REQUIRE(player.has_value() && !player->ConstructionReceipts().empty());
    const auto site = player->ConstructionReceipts().back().structure;
    REQUIRE(sim.FindEntity(site) != nullptr && !sim.FindEntity(site)->completed);
    REQUIRE(sim.ValidateConstructionAssist(0, builder, site) ==
        ConstructionAssistResult::WorkerBusy);
    REQUIRE(sim.ValidateConstructionAssist(0, assistant, site) ==
        ConstructionAssistResult::Valid);
    const auto paid = sim.FindPlayer(0)->resources;
    auto assist = MakeCommand(sim.CurrentTick(), 0, 4, CommandType::Build, assistant);
    assist.target = site;
    assist.position = sim.FindEntity(site)->position;
    REQUIRE(sim.QueueCommand(assist));
    sim.Step();
    REQUIRE(sim.FindEntity(assistant)->order.type == OrderType::Build);
    REQUIRE(sim.FindEntity(assistant)->order.target == site);
    REQUIRE(sim.FindPlayer(0)->resources == paid);
    std::string error;
    auto restored = Simulation::LoadSnapshot(sim.SaveSnapshot(), &error);
    REQUIRE(restored.has_value());
    REQUIRE(restored->StateChecksum() == sim.StateChecksum());
    auto replayed = Simulation::ReplayToEnd(sim.ExportReplay(), &error);
    REQUIRE(replayed.has_value());
    REQUIRE(replayed->StateChecksum() == sim.StateChecksum());
}

void TestConstructionAssistCancellationRace() {
    SimulationConfig config{32, 32, 20, 0x43414e43454c4153ULL};
    Simulation sim(config);
    AddTwoPlayers(sim);
    const auto core = sim.SpawnEntity(0, Faction::MeridianCompact,
        EntityType::CommandCore, Vec2::FromTiles(4, 4));
    const auto builder = sim.SpawnEntity(0, Faction::MeridianCompact,
        EntityType::Worker, Vec2::FromTiles(7, 8));
    const auto assistant = sim.SpawnEntity(0, Faction::MeridianCompact,
        EntityType::Worker, Vec2::FromTiles(8, 7));
    REQUIRE(core && builder && assistant);
    sim.CaptureReplayBaseline();
    auto build = MakeCommand(0, 0, 1, CommandType::Build, builder);
    build.buildType = EntityType::Dropoff;
    build.position = Vec2::FromTiles(8, 8);
    REQUIRE(sim.QueueCommand(build));
    sim.Step();
    const auto created = sim.CreatePlayerView(0);
    REQUIRE(created.has_value() && !created->ConstructionReceipts().empty());
    const auto site = created->ConstructionReceipts().front().structure;
    REQUIRE(sim.ValidateConstructionAssist(0, assistant, site) == ConstructionAssistResult::Valid);
    const auto paid = sim.FindPlayer(0)->resources;
    const auto cost = sim.FindEntity(site)->constructionInvestedCost;
    auto cancel = MakeCommand(sim.CurrentTick(), 0, 2, CommandType::CancelConstruction, site);
    auto assist = MakeCommand(sim.CurrentTick(), 0, 3, CommandType::Build, assistant);
    assist.target = site;
    assist.position = build.position;
    assist.buildType = EntityType::Dropoff;
    REQUIRE(sim.QueueCommand(cancel));
    REQUIRE(sim.QueueCommand(assist));
    sim.Step();
    REQUIRE(sim.FindEntity(site) == nullptr);
    REQUIRE(sim.FindEntity(assistant)->order.type != OrderType::Build);
    REQUIRE(sim.FindCommandResolutionReceipt(0, assist.sequence)->outcome == CommandResolutionOutcome::NoEffect);
    REQUIRE(sim.FindPlayer(0)->resources == (ResourcePool{
        paid.material + cost.material * 75 / 100,
        paid.dawnshards + cost.dawnshards * 75 / 100}));
    const auto afterCancel = sim.FindPlayer(0)->resources;
    const auto entityCount = sim.Entities().size();
    assist.executeTick = sim.CurrentTick();
    assist.sequence = 4;
    REQUIRE(sim.QueueCommand(assist));
    sim.Step();
    REQUIRE(sim.FindPlayer(0)->resources == afterCancel);
    REQUIRE(sim.Entities().size() == entityCount);
    REQUIRE(sim.FindCommandResolutionReceipt(0, assist.sequence)->outcome == CommandResolutionOutcome::NoEffect);
    std::string error;
    const auto replayed = Simulation::ReplayToEnd(sim.ExportReplay(), &error);
    REQUIRE(replayed.has_value());
    REQUIRE(replayed->StateChecksum() == sim.StateChecksum());
}

void TestAuthenticSchema31ConstructionAssistReplay() {
    // The receipt and baseline are emitted only by the archived candidate36
    // writer, whose source hashes accompany the retained oracle.
    const auto receiptBytes = ReadLegacyReplayFixture("schema31-assist-receipt.json");
    const std::string receiptText(receiptBytes.begin(), receiptBytes.end());
    const auto Oracle = [&](const std::string& key) -> std::uint64_t {
        const auto start = receiptText.find('"' + key + '"');
        REQUIRE(start != std::string::npos);
        const auto colon = receiptText.find(':', start);
        REQUIRE(colon != std::string::npos);
        return std::stoull(receiptText.substr(colon + 1));
    };
    REQUIRE(Oracle("snapshot_version") == 31);
    REQUIRE(Oracle("replay_version") == 28);
    ReplayRecord replay;
    replay.version = kBulwarkCommitmentReplayVersion;
    replay.initialSnapshot = ReadLegacyReplayFixture("schema31-assist-baseline.bin");
    replay.finalTick = 3;
    replay.finalChecksum = Oracle("final_checksum");
    auto build = MakeCommand(0, 0, 1, CommandType::Build, 2);
    build.buildType = EntityType::Dropoff;
    build.position = Vec2::FromTiles(8, 8);
    auto cancel = MakeCommand(1, 0, 2, CommandType::CancelConstruction, 5);
    auto assist = build;
    assist.executeTick = 1; assist.sequence = 3; assist.actor = 3; assist.target = 5;
    auto stale = build;
    stale.executeTick = 2; stale.sequence = 4; stale.actor = 4;
    stale.target = 105; stale.position = Vec2::FromTiles(8, 12);
    replay.commands = {build, cancel, assist, stale};
    std::string error;
    auto incremental = Simulation::BeginReplaySimulation(replay, &error);
    REQUIRE(incremental.has_value());
    for (Tick tick = 0; tick < 3; ++tick) {
        for (const auto& command : replay.commands) {
            if (command.executeTick == tick) REQUIRE(incremental->QueueCommand(command));
        }
        incremental->Step();
        if (tick == 0) REQUIRE(incremental->ReplayStateChecksum() == Oracle("build_tick1_checksum"));
        if (tick == 1) REQUIRE(incremental->ReplayStateChecksum() == Oracle("resurrection_tick2_checksum"));
    }
    REQUIRE(incremental->ReplayStateChecksum() == replay.finalChecksum);
    auto old = Simulation::ReplayToEnd(replay, &error);
    REQUIRE(old.has_value());
    REQUIRE(old->FindEntity(5) != nullptr && old->FindEntity(5)->hitPoints > 0);
    REQUIRE(old->FindEntity(6) != nullptr);
    REQUIRE(old->FindPlayer(0)->resources.material == static_cast<std::int32_t>(Oracle("final_material")));
    REQUIRE(old->FindPlayer(0)->resources.dawnshards == static_cast<std::int32_t>(Oracle("final_dawnshards")));
    const auto report = Simulation::BuildMatchReport(replay, &error);
    REQUIRE(report.has_value() && report->finalChecksum == replay.finalChecksum);
    auto continued = Simulation::LoadSnapshot(old->SaveSnapshot(), &error);
    REQUIRE(continued.has_value());
    REQUIRE(continued->ContinueReplayRecording(replay, &error));
    cancel.executeTick = continued->CurrentTick(); cancel.sequence = 5;
    assist.executeTick = continued->CurrentTick(); assist.sequence = 6;
    REQUIRE(continued->QueueCommand(cancel));
    REQUIRE(continued->QueueCommand(assist));
    continued->Step();
    REQUIRE(continued->FindEntity(5) != nullptr && continued->FindEntity(5)->hitPoints > 0);
    const auto prefix = continued->ExportReplay(&error);
    REQUIRE(prefix.version == kBulwarkCommitmentReplayVersion);
    REQUIRE(prefix.finalChecksum == Oracle("continued_tick4_checksum"));
    REQUIRE(Simulation::ReplayToEnd(prefix, &error).has_value());
    // A deliberately new recording rebases the same live world to the repaired
    // semantics, rather than silently changing the historical continuation.
    continued->CaptureReplayBaseline();
    REQUIRE(continued->ExportReplay().version == kMaintenanceReplayVersion);
    cancel.executeTick = continued->CurrentTick(); cancel.sequence = 7;
    assist.executeTick = continued->CurrentTick(); assist.sequence = 8;
    REQUIRE(continued->QueueCommand(cancel));
    REQUIRE(continued->QueueCommand(assist));
    continued->Step();
    REQUIRE(continued->FindEntity(5) == nullptr);
    REQUIRE(continued->FindCommandResolutionReceipt(0, 8)->outcome == CommandResolutionOutcome::NoEffect);
    REQUIRE(Simulation::ReplayToEnd(continued->ExportReplay(), &error).has_value());
}
