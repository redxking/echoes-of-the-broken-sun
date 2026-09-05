void TestProjectilePersistenceRegression() {
    SimulationConfig config{32, 32, 20, 0xB4111571CULL};
    config.enableBallisticProjectiles = true;
    Simulation simulation(config);
    REQUIRE(simulation.AddPlayer(0, Faction::MeridianCompact,
                                 ResourcePool{0, 0}));
    REQUIRE(simulation.AddPlayer(1, Faction::KharuunAssemblies,
                                 ResourcePool{0, 0}));
    const EntityId attacker = simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Soldier,
        Vec2::FromTiles(10, 10));
    const EntityId target = simulation.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::Soldier,
        Vec2::FromTiles(14, 10));
    REQUIRE(attacker != 0 && target != 0);
    const std::int32_t targetHealth = simulation.FindEntity(target)->hitPoints;

    Command attack = MakeCommand(
        simulation.CurrentTick(), 0, 1, CommandType::Attack, attacker);
    attack.target = target;
    REQUIRE(simulation.QueueCommand(attack));
    simulation.Step();
    REQUIRE(simulation.Projectiles().size() == 1);
    REQUIRE(simulation.FindEntity(target)->hitPoints == targetHealth);

    const std::vector<std::uint8_t> snapshot = simulation.SaveSnapshot();
    REQUIRE(ReadU32(snapshot, 4) == kSnapshotVersion);
    std::string error;
    std::optional<Simulation> restored =
        Simulation::LoadSnapshot(snapshot, &error);
    REQUIRE(restored.has_value());
    REQUIRE(error.empty());
    REQUIRE(restored->Config().enableBallisticProjectiles);
    REQUIRE(restored->Projectiles() == simulation.Projectiles());
    REQUIRE(restored->StateChecksum() == simulation.StateChecksum());

    simulation.Step(12);
    restored->Step(12);
    REQUIRE(simulation.Projectiles() == restored->Projectiles());
    REQUIRE(simulation.FindEntity(target)->hitPoints ==
            restored->FindEntity(target)->hitPoints);
    REQUIRE(simulation.FindEntity(target)->hitPoints < targetHealth);
    REQUIRE(simulation.StateChecksum() == restored->StateChecksum());

    // The extension is fully bounded before it reserves memory or parses
    // records. A malformed count cannot make a trusted snapshot appear valid.
    constexpr std::size_t mapTileCount = 32U * 32U;
    const std::size_t projectileHeaderOffset =
        SnapshotProjectileHeaderOffset(snapshot, mapTileCount);
    const std::size_t projectileCountOffset = projectileHeaderOffset + 5U;
    REQUIRE(ReadU32(snapshot, projectileCountOffset) == 1U);
    const std::size_t projectileOffset = projectileHeaderOffset + 9U;
    REQUIRE(projectileOffset <= snapshot.size() &&
            snapshot.size() - projectileOffset >= kSerializedProjectileBytes);

    std::vector<std::uint8_t> malformedCount = snapshot;
    WriteU32(malformedCount, projectileCountOffset,
             std::numeric_limits<std::uint32_t>::max());
    ResignSnapshot(malformedCount);
    REQUIRE(!Simulation::LoadSnapshot(malformedCount, &error).has_value());

    // Speed must remain positive, even when the payload integrity tag has
    // been recomputed. The speed field begins 33 bytes into a projectile.
    std::vector<std::uint8_t> invalidSpeed = snapshot;
    WriteU32(invalidSpeed, projectileOffset + 33, 0);
    ResignSnapshot(invalidSpeed);
    REQUIRE(!Simulation::LoadSnapshot(invalidSpeed, &error).has_value());

    // A projectile is an already-authorized historical action. Its shooter
    // may be removed before the next impact tick; that retired ID remains a
    // valid reference in a persisted flight record.
    Simulation retiredReferences(config);
    REQUIRE(retiredReferences.AddPlayer(
        0, Faction::MeridianCompact, ResourcePool{0, 0}));
    REQUIRE(retiredReferences.AddPlayer(
        1, Faction::KharuunAssemblies, ResourcePool{0, 0}));
    const EntityId retiredSource = retiredReferences.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Soldier,
        Vec2::FromTiles(10, 10));
    const EntityId retiredTarget = retiredReferences.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::Soldier,
        Vec2::FromTiles(14, 10));
    Command retiredAttack = MakeCommand(
        retiredReferences.CurrentTick(), 0, 1, CommandType::Attack,
        retiredSource);
    retiredAttack.target = retiredTarget;
    REQUIRE(retiredReferences.QueueCommand(retiredAttack));
    retiredReferences.Step();
    REQUIRE(retiredReferences.Projectiles().size() == 1);
    REQUIRE(retiredReferences.MutableEntityForTesting(retiredSource) != nullptr);
    retiredReferences.MutableEntityForTesting(retiredSource)->hitPoints = 0;
    retiredReferences.Step();
    REQUIRE(retiredReferences.FindEntity(retiredSource) == nullptr);
    REQUIRE(retiredReferences.FindEntity(retiredTarget) != nullptr);
    REQUIRE(retiredReferences.Projectiles().size() == 1);

    const std::vector<std::uint8_t> retiredSnapshot =
        retiredReferences.SaveSnapshot();
    std::optional<Simulation> restoredRetired =
        Simulation::LoadSnapshot(retiredSnapshot, &error);
    REQUIRE(restoredRetired.has_value());
    REQUIRE(restoredRetired->Projectiles() ==
            retiredReferences.Projectiles());
    // Removing the target cancels its surviving flight deterministically.
    REQUIRE(retiredReferences.MutableEntityForTesting(retiredTarget) != nullptr);
    REQUIRE(restoredRetired->MutableEntityForTesting(retiredTarget) != nullptr);
    retiredReferences.MutableEntityForTesting(retiredTarget)->hitPoints = 0;
    restoredRetired->MutableEntityForTesting(retiredTarget)->hitPoints = 0;
    retiredReferences.Step();
    restoredRetired->Step();
    REQUIRE(retiredReferences.Projectiles().empty());
    REQUIRE(restoredRetired->Projectiles().empty());
    REQUIRE(restoredRetired->Projectiles() ==
            retiredReferences.Projectiles());
    REQUIRE(restoredRetired->StateChecksum() ==
            retiredReferences.StateChecksum());
}


void TestBallisticCoverAndTrackingRegression() {
    SimulationConfig config{24, 24, 20, 0xC0FE26ULL};
    config.enableBallisticProjectiles = true;
    config.rules.mineralCover.durationTicks = 100;
    config.rules.mineralCover.maxHitPoints = 100;
    Simulation sim(config);
    AddTwoPlayers(sim, {0, 0}, {0, 100});
    const EntityId caster = sim.SpawnEntity(1, Faction::KharuunAssemblies,
        EntityType::HeavyUnit, Vec2::FromTiles(10, 10));
    const EntityId target = sim.SpawnEntity(1, Faction::KharuunAssemblies,
        EntityType::Soldier, Vec2::FromTiles(8, 10));
    const EntityId attacker = sim.SpawnEntity(0, Faction::MeridianCompact,
        EntityType::Soldier, Vec2::FromTiles(12, 10));
    const int targetHp = sim.FindEntity(target)->hitPoints;
    Command raise = MakeCommand(0, 1, 1, CommandType::RaiseMineralCover, caster);
    raise.position = Vec2::FromTiles(9, 10);
    Command attack = MakeCommand(0, 0, 1, CommandType::Attack, attacker);
    attack.target = target;
    REQUIRE(sim.QueueCommand(raise));
    REQUIRE(sim.QueueCommand(attack));
    sim.Step();
    const auto found = std::find_if(sim.Entities().begin(), sim.Entities().end(),
        [](const Entity& e) { return e.temporaryMineralCover; });
    REQUIRE(found != sim.Entities().end());
    const EntityId cover = found->id;
    REQUIRE(sim.Projectiles().size() == 1);
    std::string error;
    auto restored = Simulation::LoadSnapshot(sim.SaveSnapshot(), &error);
    REQUIRE(restored.has_value());
    sim.Step(6);
    restored->Step(6);
    REQUIRE(sim.FindEntity(cover)->hitPoints < 100);
    REQUIRE(sim.FindEntity(target)->hitPoints == targetHp);
    REQUIRE(sim.StateChecksum() == restored->StateChecksum());

    Simulation tracking(config);
    AddTwoPlayers(tracking, {0, 0}, {0, 0});
    const auto shooter = tracking.SpawnEntity(0, Faction::MeridianCompact,
        EntityType::Soldier, Vec2::FromTiles(6, 6));
    const auto mover = tracking.SpawnEntity(1, Faction::KharuunAssemblies,
        EntityType::Soldier, Vec2::FromTiles(10, 6));
    attack = MakeCommand(0, 0, 1, CommandType::Attack, shooter);
    attack.target = mover;
    REQUIRE(tracking.QueueCommand(attack));
    tracking.Step();
    Command move = MakeCommand(tracking.CurrentTick(), 1, 1, CommandType::Move, mover);
    move.position = Vec2::FromTiles(10, 12);
    REQUIRE(tracking.QueueCommand(move));
    tracking.Step();
    REQUIRE(tracking.Projectiles().size() == 1);
    REQUIRE(tracking.Projectiles().front().destination == tracking.FindEntity(mover)->position);
    REQUIRE(tracking.Projectiles().front().destination != Vec2::FromTiles(10, 6));
}
