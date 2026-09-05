void TestHarvestReservationRegression() {
    // One arrival-order slot is non-preemptive. A lower entity ID that
    // receives Gather later cannot displace the established holder; an
    // earlier waiter receives the released slot instead.
    Simulation reservation(SimulationConfig{48, 32, 20, 0xA11CE001ULL});
    REQUIRE(reservation.AddPlayer(
        0, Faction::MeridianCompact, ResourcePool{0, 0}));
    const EntityId core = reservation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::CommandCore,
        Vec2::FromTiles(8, 8));
    const EntityId node = reservation.SpawnResourceNode(
        Vec2::FromTiles(16, 8), 400);
    // This worker has the lowest ID, but it receives its command later.
    const EntityId lateLowerId = reservation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Worker,
        Vec2::FromTiles(16, 8));
    const EntityId earlyFirst = reservation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Worker,
        Vec2::FromTiles(16, 8));
    const EntityId olderWaiter = reservation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Worker,
        Vec2::FromTiles(16, 8));
    REQUIRE(core != 0 && node != 0 && lateLowerId != 0 &&
            earlyFirst != 0 && olderWaiter != 0);
    for (const EntityId worker :
         {lateLowerId, earlyFirst, olderWaiter}) {
        Entity* state = reservation.MutableEntityForTesting(worker);
        REQUIRE(state != nullptr);
        state->cargoCapacity = 10;
    }

    Command firstGather = MakeCommand(
        reservation.CurrentTick(), 0, 1, CommandType::Gather, earlyFirst);
    firstGather.target = node;
    Command waiterGather = MakeCommand(
        reservation.CurrentTick(), 0, 2, CommandType::Gather, olderWaiter);
    waiterGather.target = node;
    REQUIRE(reservation.QueueCommand(firstGather));
    REQUIRE(reservation.QueueCommand(waiterGather));
    reservation.Step();
    REQUIRE(reservation.FindEntity(earlyFirst)->harvestSlotHeld);
    REQUIRE(!reservation.FindEntity(olderWaiter)->harvestSlotHeld);
    REQUIRE(reservation.FindEntity(olderWaiter)->harvestQueueTicket != 0);

    Command lateGather = MakeCommand(
        reservation.CurrentTick(), 0, 3, CommandType::Gather, lateLowerId);
    lateGather.target = node;
    REQUIRE(reservation.QueueCommand(lateGather));
    reservation.Step();
    REQUIRE(reservation.FindEntity(earlyFirst)->harvestSlotHeld);
    REQUIRE(!reservation.FindEntity(lateLowerId)->harvestSlotHeld);
    REQUIRE(reservation.FindEntity(olderWaiter)->harvestQueueTicket <
            reservation.FindEntity(lateLowerId)->harvestQueueTicket);

    Command stopFirst = MakeCommand(
        reservation.CurrentTick(), 0, 4, CommandType::Stop, earlyFirst);
    REQUIRE(reservation.QueueCommand(stopFirst));
    reservation.Step();
    REQUIRE(reservation.FindEntity(earlyFirst)->harvestState ==
            HarvestState::Idle);
    REQUIRE(!reservation.FindEntity(earlyFirst)->harvestSlotHeld);
    REQUIRE(reservation.FindEntity(olderWaiter)->harvestSlotHeld);
    REQUIRE(!reservation.FindEntity(lateLowerId)->harvestSlotHeld);

    // A queued worker and active harvester retain their complete reservation
    // state across the current snapshot schema and advance identically.
    const std::vector<std::uint8_t> midHarvestSnapshot =
        reservation.SaveSnapshot();
    REQUIRE(ReadU32(midHarvestSnapshot, 4) == kSnapshotVersion);
    std::string snapshotError;
    std::optional<Simulation> restored =
        Simulation::LoadSnapshot(midHarvestSnapshot, &snapshotError);
    REQUIRE(restored.has_value());
    REQUIRE(snapshotError.empty());
    for (const EntityId worker : {olderWaiter, lateLowerId}) {
        const Entity* before = reservation.FindEntity(worker);
        const Entity* after = restored->FindEntity(worker);
        REQUIRE(before != nullptr && after != nullptr);
        REQUIRE(after->harvestState == before->harvestState);
        REQUIRE(after->assignedResourceNode == before->assignedResourceNode);
        REQUIRE(after->harvestQueueTicket == before->harvestQueueTicket);
        REQUIRE(after->harvestSlotHeld == before->harvestSlotHeld);
    }
    reservation.Step(80);
    restored->Step(80);
    REQUIRE(reservation.StateChecksum() == restored->StateChecksum());

    // The final partial load is delivered once, the original depleted node is
    // retained as the assignment, and a nearby live node is not substituted.
    Simulation depletion(SimulationConfig{32, 32, 20, 0xA11CE002ULL});
    REQUIRE(depletion.AddPlayer(0, Faction::MeridianCompact,
                                ResourcePool{0, 0}));
    const EntityId depletionCore = depletion.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::CommandCore,
        Vec2::FromTiles(4, 8));
    const EntityId depletedNode = depletion.SpawnResourceNode(
        Vec2::FromTiles(10, 8), 7);
    const EntityId nearbyLiveNode = depletion.SpawnResourceNode(
        Vec2::FromTiles(11, 8), 100);
    const EntityId depletionWorker = depletion.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Worker,
        Vec2::FromTiles(10, 8));
    REQUIRE(depletionCore != 0 && depletedNode != 0 && nearbyLiveNode != 0 &&
            depletionWorker != 0);
    REQUIRE(depletion.MutableEntityForTesting(depletionWorker) != nullptr);
    depletion.MutableEntityForTesting(depletionWorker)->cargoCapacity = 10;
    Command depletionGather = MakeCommand(
        depletion.CurrentTick(), 0, 1, CommandType::Gather, depletionWorker);
    depletionGather.target = depletedNode;
    REQUIRE(depletion.QueueCommand(depletionGather));
    bool deliveredPartialLoad = false;
    for (int32_t step = 0; step < 240; ++step) {
        depletion.Step();
        if (depletion.FindPlayer(0)->resources.material == 7) {
            deliveredPartialLoad = true;
            break;
        }
    }
    REQUIRE(deliveredPartialLoad);
    const Entity* depletedWorkerState = depletion.FindEntity(depletionWorker);
    REQUIRE(depletedWorkerState != nullptr);
    REQUIRE(depletion.FindEntity(depletedNode)->resourceRemaining == 0);
    REQUIRE(depletion.FindEntity(nearbyLiveNode)->resourceRemaining == 100);
    REQUIRE(depletedWorkerState->cargo == 0);
    REQUIRE(depletedWorkerState->harvestState == HarvestState::Idle);
    depletion.Step(40);
    REQUIRE(depletion.FindPlayer(0)->resources.material == 7);
    REQUIRE(depletion.FindEntity(nearbyLiveNode)->resourceRemaining == 100);

    // Delivery time includes physical movement. The wallet remains unchanged
    // until the worker reaches a depot, and a farther depot takes longer than
    // the identical near-depot fixture.
    const auto FirstDepositTick = [](int32_t coreX, std::uint64_t seed) {
        Simulation sim(SimulationConfig{48, 32, 20, seed});
        REQUIRE(sim.AddPlayer(0, Faction::MeridianCompact,
                              ResourcePool{0, 0}));
        const EntityId depot = sim.SpawnEntity(
            0, Faction::MeridianCompact, EntityType::CommandCore,
            Vec2::FromTiles(coreX, 8));
        const EntityId resource = sim.SpawnResourceNode(
            Vec2::FromTiles(20, 8), 80);
        const EntityId worker = sim.SpawnEntity(
            0, Faction::MeridianCompact, EntityType::Worker,
            Vec2::FromTiles(20, 8));
        REQUIRE(depot != 0 && resource != 0 && worker != 0);
        Entity* workerState = sim.MutableEntityForTesting(worker);
        REQUIRE(workerState != nullptr);
        workerState->cargoCapacity = 10;
        Command gather = MakeCommand(
            sim.CurrentTick(), 0, 1, CommandType::Gather, worker);
        gather.target = resource;
        REQUIRE(sim.QueueCommand(gather));
        for (int32_t step = 0; step < 300; ++step) {
            REQUIRE(sim.FindPlayer(0)->resources.material == 0);
            sim.Step();
            if (sim.FindPlayer(0)->resources.material > 0) {
                REQUIRE(sim.FindPlayer(0)->resources.material == 10);
                return sim.CurrentTick();
            }
        }
        REQUIRE(false);
        return Tick{0};
    };
    const Tick nearDepositTick = FirstDepositTick(18, 0xA11CE003ULL);
    const Tick farDepositTick = FirstDepositTick(4, 0xA11CE004ULL);
    REQUIRE(farDepositTick > nearDepositTick);

    // A worker does not discard gathered cargo when no depot exists. Once a
    // depot becomes operational, the retained load reaches the wallet once.
    Simulation noDepot(SimulationConfig{32, 32, 20, 0xA11CE005ULL});
    REQUIRE(noDepot.AddPlayer(0, Faction::MeridianCompact,
                              ResourcePool{0, 0}));
    const EntityId isolatedNode = noDepot.SpawnResourceNode(
        Vec2::FromTiles(12, 8), 10);
    const EntityId strandedWorker = noDepot.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Worker,
        Vec2::FromTiles(12, 8));
    REQUIRE(isolatedNode != 0 && strandedWorker != 0);
    Entity* strandedState = noDepot.MutableEntityForTesting(strandedWorker);
    REQUIRE(strandedState != nullptr);
    strandedState->cargoCapacity = 10;
    Command strandedGather = MakeCommand(
        noDepot.CurrentTick(), 0, 1, CommandType::Gather, strandedWorker);
    strandedGather.target = isolatedNode;
    REQUIRE(noDepot.QueueCommand(strandedGather));
    bool cargoWasRetained = false;
    for (int32_t step = 0; step < 160; ++step) {
        noDepot.Step();
        const Entity* worker = noDepot.FindEntity(strandedWorker);
        if (worker != nullptr && worker->cargo == 10 &&
            noDepot.FindEntity(isolatedNode)->resourceRemaining == 0) {
            cargoWasRetained = true;
            break;
        }
    }
    REQUIRE(cargoWasRetained);
    REQUIRE(noDepot.FindPlayer(0)->resources.material == 0);
    const EntityId recoveredDepot = noDepot.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::CommandCore,
        Vec2::FromTiles(4, 8));
    REQUIRE(recoveredDepot != 0);
    bool retainedCargoDelivered = false;
    for (int32_t step = 0; step < 160; ++step) {
        noDepot.Step();
        if (noDepot.FindPlayer(0)->resources.material == 10) {
            retainedCargoDelivered = true;
            break;
        }
    }
    REQUIRE(retainedCargoDelivered);
    REQUIRE(noDepot.FindEntity(strandedWorker)->cargo == 0);
    REQUIRE(noDepot.FindPlayer(0)->resources.material == 10);

    // A dense simultaneous arrival forms one stable FIFO queue. Parking must
    // not rewrite arrival tickets, and the one extraction position eventually
    // serves every worker exactly once from this finite node.
    Simulation dense(SimulationConfig{40, 28, 20, 0xA11CE006ULL});
    REQUIRE(dense.AddPlayer(0, Faction::MeridianCompact,
                            ResourcePool{0, 0}));
    const EntityId denseCore = dense.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::CommandCore,
        Vec2::FromTiles(4, 14));
    const EntityId denseNode = dense.SpawnResourceNode(
        Vec2::FromTiles(20, 14), 160);
    REQUIRE(denseCore != 0 && denseNode != 0);
    std::vector<EntityId> denseWorkers;
    for (std::uint64_t index = 0; index < 16; ++index) {
        const EntityId worker = dense.SpawnEntity(
            0, Faction::MeridianCompact, EntityType::Worker,
            Vec2::FromTiles(20, 14));
        REQUIRE(worker != 0);
        REQUIRE(dense.MutableEntityForTesting(worker) != nullptr);
        dense.MutableEntityForTesting(worker)->cargoCapacity = 10;
        denseWorkers.push_back(worker);
        Command gather = MakeCommand(
            dense.CurrentTick(), 0, index + 1, CommandType::Gather, worker);
        gather.target = denseNode;
        REQUIRE(dense.QueueCommand(gather));
    }
    dense.Step();
    std::vector<Tick> arrivalTickets;
    std::size_t heldSlots = 0;
    for (const EntityId worker : denseWorkers) {
        const Entity* state = dense.FindEntity(worker);
        REQUIRE(state != nullptr);
        REQUIRE(state->harvestQueueTicket != 0);
        arrivalTickets.push_back(state->harvestQueueTicket);
        heldSlots += state->harvestSlotHeld ? 1U : 0U;
    }
    REQUIRE(heldSlots == 1);
    for (int32_t step = 0; step < 10; ++step) {
        dense.Step();
        heldSlots = 0;
        for (std::size_t index = 0; index < denseWorkers.size(); ++index) {
            const Entity* state = dense.FindEntity(denseWorkers[index]);
            REQUIRE(state != nullptr);
            REQUIRE(state->harvestQueueTicket == arrivalTickets[index]);
            heldSlots += state->harvestSlotHeld ? 1U : 0U;
        }
        REQUIRE(heldSlots <= 1);
    }
    bool denseDelivered = false;
    for (int32_t step = 0; step < 1000; ++step) {
        dense.Step();
        heldSlots = 0;
        for (const EntityId worker : denseWorkers) {
            const Entity* state = dense.FindEntity(worker);
            REQUIRE(state != nullptr);
            heldSlots += state->harvestSlotHeld ? 1U : 0U;
        }
        REQUIRE(heldSlots <= 1);
        if (dense.FindPlayer(0)->resources.material == 160) {
            denseDelivered = true;
            break;
        }
    }
    REQUIRE(denseDelivered);
    REQUIRE(dense.FindEntity(denseNode)->resourceRemaining == 0);
    for (const EntityId worker : denseWorkers) {
        const Entity* state = dense.FindEntity(worker);
        REQUIRE(state != nullptr);
        REQUIRE(state->cargo == 0);
        REQUIRE(state->harvestState == HarvestState::Idle);
    }

    // The current schema preserves an actual queued shift-Move and
    // rejects both a second holder and a queue depth beyond the entity limit.
    Simulation queued(SimulationConfig{32, 24, 20, 0xA11CE007ULL});
    REQUIRE(queued.AddPlayer(0, Faction::MeridianCompact,
                             ResourcePool{0, 0}));
    const EntityId queuedCore = queued.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::CommandCore,
        Vec2::FromTiles(4, 12));
    const EntityId queuedNode = queued.SpawnResourceNode(
        Vec2::FromTiles(14, 12), 100);
    const EntityId queuedWorker = queued.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Worker,
        Vec2::FromTiles(14, 12));
    REQUIRE(queuedCore != 0 && queuedNode != 0 && queuedWorker != 0);
    Command queuedGather = MakeCommand(
        queued.CurrentTick(), 0, 1, CommandType::Gather, queuedWorker);
    queuedGather.target = queuedNode;
    Command queuedMove = MakeCommand(
        queued.CurrentTick(), 0, 2, CommandType::Move, queuedWorker);
    queuedMove.position = Vec2::FromTiles(18, 12);
    queuedMove.queue = true;
    REQUIRE(queued.QueueCommand(queuedGather));
    REQUIRE(queued.QueueCommand(queuedMove));
    queued.Step();
    REQUIRE(queued.FindEntity(queuedWorker)->orderQueue.size() == 1);
    REQUIRE(queued.FindEntity(queuedWorker)->orderQueue.front().type ==
            OrderType::Move);
    const std::vector<std::uint8_t> queuedSnapshot = queued.SaveSnapshot();
    std::string queuedError;
    std::optional<Simulation> queuedRestored =
        Simulation::LoadSnapshot(queuedSnapshot, &queuedError);
    REQUIRE(queuedRestored.has_value());
    REQUIRE(queuedRestored->FindEntity(queuedWorker)->orderQueue.size() == 1);
    REQUIRE(queuedRestored->FindEntity(queuedWorker)->orderQueue.front().type ==
            OrderType::Move);
    REQUIRE(queuedRestored->FindEntity(queuedWorker)->orderQueue.front().destination ==
            Vec2::FromTiles(18, 12));

    const std::size_t queuedMapTiles = 32U * 24U;
    const std::size_t queuedWorkOffset =
        SnapshotWorkBlockOffset(queuedSnapshot, queuedMapTiles);
    REQUIRE(ReadU32(queuedSnapshot, queuedWorkOffset) == 3);
    const std::size_t queuedWorkerWorkOffset =
        SnapshotWorkRecordOffset(queuedSnapshot, queuedWorkOffset, 2U);
    REQUIRE(ReadU32(queuedSnapshot, queuedWorkerWorkOffset) == queuedWorker);
    REQUIRE(queuedSnapshot[queuedWorkerWorkOffset + 30U] == 1);
    REQUIRE(queuedWorkerWorkOffset <= queuedSnapshot.size() &&
            queuedSnapshot.size() - queuedWorkerWorkOffset >=
                kSerializedWorkStateBytes + kSerializedQueuedOrderBytes);
    std::vector<std::uint8_t> oversizedQueue = queuedSnapshot;
    oversizedQueue[queuedWorkerWorkOffset + 30U] = 17;
    ResignSnapshot(oversizedQueue);
    REQUIRE(!Simulation::LoadSnapshot(oversizedQueue, &queuedError).has_value());

    // Use a fresh active queue for the duplicate-holder mutation rather than
    // mutating terminal workers from the completed dense fixture.
    Simulation duplicateHolder(SimulationConfig{24, 20, 20, 0xA11CE008ULL});
    REQUIRE(duplicateHolder.AddPlayer(0, Faction::MeridianCompact,
                                      ResourcePool{0, 0}));
    const EntityId holderCore = duplicateHolder.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::CommandCore,
        Vec2::FromTiles(4, 10));
    const EntityId holderNode = duplicateHolder.SpawnResourceNode(
        Vec2::FromTiles(12, 10), 100);
    const EntityId holderOne = duplicateHolder.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Worker,
        Vec2::FromTiles(12, 10));
    const EntityId holderTwo = duplicateHolder.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Worker,
        Vec2::FromTiles(12, 10));
    REQUIRE(holderCore != 0 && holderNode != 0 && holderOne != 0 &&
            holderTwo != 0);
    for (const auto [worker, sequence] :
         std::array<std::pair<EntityId, std::uint64_t>, 2>{{
             {holderOne, 1}, {holderTwo, 2}}}) {
        Command gather = MakeCommand(duplicateHolder.CurrentTick(), 0,
                                     sequence, CommandType::Gather, worker);
        gather.target = holderNode;
        REQUIRE(duplicateHolder.QueueCommand(gather));
    }
    duplicateHolder.Step();
    REQUIRE(duplicateHolder.FindEntity(holderOne)->harvestSlotHeld);
    REQUIRE(!duplicateHolder.FindEntity(holderTwo)->harvestSlotHeld);
    std::vector<std::uint8_t> duplicateHolderSnapshot =
        duplicateHolder.SaveSnapshot();
    const std::size_t holderWorkOffset = SnapshotWorkBlockOffset(
        duplicateHolderSnapshot, 24U * 20U);
    const std::size_t holderTwoWorkOffset = SnapshotWorkRecordOffset(
        duplicateHolderSnapshot, holderWorkOffset, 3U);
    REQUIRE(ReadU32(duplicateHolderSnapshot, holderTwoWorkOffset) == holderTwo);
    duplicateHolderSnapshot[holderTwoWorkOffset + 5U] = 1;
    ResignSnapshot(duplicateHolderSnapshot);
    REQUIRE(!Simulation::LoadSnapshot(duplicateHolderSnapshot,
                                      &queuedError).has_value());

    // Depot selection uses reachable path cost. The closer sealed depot is
    // never selected when an open, farther depot can receive the load.
    Simulation reachableDepot(SimulationConfig{32, 24, 20, 0xA11CE009ULL});
    REQUIRE(reachableDepot.AddPlayer(0, Faction::MeridianCompact,
                                     ResourcePool{0, 0}));
    const EntityId farDepot = reachableDepot.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::CommandCore,
        Vec2::FromTiles(4, 12));
    const EntityId sealedDepot = reachableDepot.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::CommandCore,
        Vec2::FromTiles(18, 12));
    const EntityId reachableNode = reachableDepot.SpawnResourceNode(
        Vec2::FromTiles(14, 12), 10);
    const EntityId reachableWorker = reachableDepot.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Worker,
        Vec2::FromTiles(14, 12));
    REQUIRE(farDepot != 0 && sealedDepot != 0 && reachableNode != 0 &&
            reachableWorker != 0);
    for (const auto [x, y] : std::array<std::pair<int32_t, int32_t>, 8>{{
             {17, 11}, {18, 11}, {19, 11}, {17, 12},
             {19, 12}, {17, 13}, {18, 13}, {19, 13}}}) {
        REQUIRE(reachableDepot.SetTerrainTile(x, y, Terrain::Blocked));
    }
    Command reachableGather = MakeCommand(reachableDepot.CurrentTick(), 0, 1,
                                           CommandType::Gather, reachableWorker);
    reachableGather.target = reachableNode;
    REQUIRE(reachableDepot.QueueCommand(reachableGather));
    bool choseReachableDepot = false;
    for (int32_t step = 0; step < 160; ++step) {
        reachableDepot.Step();
        const Entity* worker = reachableDepot.FindEntity(reachableWorker);
        if (worker->order.type == OrderType::Deliver && worker->cargo == 10) {
            REQUIRE(worker->order.target == farDepot);
            choseReachableDepot = true;
            break;
        }
    }
    REQUIRE(choseReachableDepot);

    // Destroying the chosen depot after departure retains the worker's cargo;
    // a missing drop-off cannot convert the load into wallet resources.
    Simulation destroyedDepot(SimulationConfig{32, 24, 20, 0xA11CE00AULL});
    REQUIRE(destroyedDepot.AddPlayer(0, Faction::MeridianCompact,
                                     ResourcePool{0, 0}));
    const EntityId doomedDepot = destroyedDepot.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::CommandCore,
        Vec2::FromTiles(4, 12));
    const EntityId doomedNode = destroyedDepot.SpawnResourceNode(
        Vec2::FromTiles(20, 12), 10);
    const EntityId doomedWorker = destroyedDepot.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Worker,
        Vec2::FromTiles(20, 12));
    REQUIRE(doomedDepot != 0 && doomedNode != 0 && doomedWorker != 0);
    Command doomedGather = MakeCommand(destroyedDepot.CurrentTick(), 0, 1,
                                       CommandType::Gather, doomedWorker);
    doomedGather.target = doomedNode;
    REQUIRE(destroyedDepot.QueueCommand(doomedGather));
    bool departedForDepot = false;
    for (int32_t step = 0; step < 200; ++step) {
        destroyedDepot.Step();
        const Entity* worker = destroyedDepot.FindEntity(doomedWorker);
        if (worker->order.type == OrderType::Deliver && worker->cargo == 10 &&
            worker->position != Vec2::FromTiles(20, 12)) {
            departedForDepot = true;
            break;
        }
    }
    REQUIRE(departedForDepot);
    REQUIRE(destroyedDepot.MutableEntityForTesting(doomedDepot) != nullptr);
    destroyedDepot.MutableEntityForTesting(doomedDepot)->hitPoints = 0;
    destroyedDepot.Step();
    const Entity* retainedWorker = destroyedDepot.FindEntity(doomedWorker);
    REQUIRE(retainedWorker != nullptr);
    REQUIRE(retainedWorker->cargo == 10);
    REQUIRE(retainedWorker->order.type == OrderType::Deliver);
    REQUIRE(retainedWorker->order.target == 0);
    REQUIRE(destroyedDepot.FindPlayer(0)->resources.material == 0);
}
