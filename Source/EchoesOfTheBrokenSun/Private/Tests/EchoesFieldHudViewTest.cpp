// Copyright Echoes of the Broken Sun. All Rights Reserved.
// Author: Angelis Pseftis

#include "EchoesFieldHudView.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "EchoesTestSaveEnvironment.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesFieldHudViewTest,
    "Echoes.Runtime.Presentation.FieldHudAuthority",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesFieldHudViewTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    FEchoesScopedTestSaveEnvironment SaveEnvironment(*this);
    if (!SaveEnvironment.IsReady())
    {
        return false;
    }

    using namespace echoes::sim;
    SimulationConfig Config;
    Config.mapWidthTiles = 16;
    Config.mapHeightTiles = 16;
    Simulation SimulationValue(Config);
    TestTrue(TEXT("Local player is admitted"),
        SimulationValue.AddPlayer(0, Faction::MeridianCompact, {900, 120}));
    TestTrue(TEXT("Opponent is admitted"),
        SimulationValue.AddPlayer(1, Faction::KharuunAssemblies, {900, 120}));
    const EntityId LocalCore = SimulationValue.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::CommandCore,
        Vec2::FromTiles(2, 2));
    const EntityId LocalProducer = SimulationValue.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Barracks,
        Vec2::FromTiles(3, 2));
    const EntityId LocalSurveyor = SimulationValue.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Worker,
        Vec2::FromTiles(4, 2));
    const EntityId HiddenEnemy = SimulationValue.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::Soldier,
        Vec2::FromTiles(14, 14));
    const EntityId HiddenEnemyProducer = SimulationValue.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::Barracks,
        Vec2::FromTiles(13, 14));
    TestTrue(TEXT("Fixture entities exist"),
        LocalCore != 0 && LocalProducer != 0 && LocalSurveyor != 0 &&
            HiddenEnemy != 0 && HiddenEnemyProducer != 0);
    if (Entity* Producer = SimulationValue.MutableEntityForTesting(LocalProducer))
    {
        Producer->productionType = EntityType::Worker;
        Producer->activeProductionItemId = 7001;
        Producer->productionProgress = 9;
        Producer->productionRequired = 30;
        Producer->productionInvestedCost = {60, 12};
        Producer->productionLogisticsCost = 1;
        Producer->productionSpawnBlockedAlert = true;
        Producer->rallyRouteAlert = true;
        Order Rally;
        Rally.type = OrderType::Move;
        Rally.destination = Vec2::FromTiles(5, 2);
        Producer->rallyRoute.push_back(Rally);
        ProductionQueueItem Soldier;
        Soldier.itemId = 7002;
        Soldier.unitType = EntityType::Soldier;
        Soldier.configuredCost = {85, 20};
        Soldier.requiredTicks = 100;
        Soldier.logisticsCost = 2;
        Producer->productionQueue.push_back(Soldier);
        ProductionQueueItem Heavy;
        Heavy.itemId = 7003;
        Heavy.unitType = EntityType::HeavyUnit;
        Heavy.configuredCost = {130, 25};
        Heavy.requiredTicks = 140;
        Heavy.logisticsCost = 3;
        Producer->productionQueue.push_back(Heavy);
    }

    const std::optional<PlayerView> Player = SimulationValue.CreatePlayerView(0);
    if (!TestTrue(TEXT("Scoped player view materializes"), Player.has_value()))
    {
        return false;
    }
    const TArray<uint32> Selected{LocalProducer, HiddenEnemy};
    const FEchoesFieldHudView Live =
        FEchoesFieldHudModel::BuildPlayerScoped(*Player, Selected, false);
    TestEqual(TEXT("Live view declares player-view authority"),
        Live.Authority, EEchoesFieldHudAuthority::LivePlayerView);
    TestTrue(TEXT("Live ledger comes from scoped player resources"),
        Live.Resources.bVisible && Live.Resources.Matter == 900 &&
            Live.Resources.Dawn == 120);
    TestEqual(TEXT("Hidden selected entity is omitted"),
        Live.Selection.Entries.Num(), 1);
    if (Live.Selection.Entries.Num() == 1)
    {
        TestEqual(TEXT("Visible selected producer retains identity"),
            Live.Selection.Entries[0].EntityId, static_cast<uint32>(LocalProducer));
        TestEqual(TEXT("Production progress is semantic data"),
            Live.Selection.Entries[0].ProductionPercent, 30);
    }
    TestFalse(TEXT("A stale mixed selection cannot publish producer controls"),
        Live.Production.bVisible);
    const FEchoesFieldHudView ProducerOnly =
        FEchoesFieldHudModel::BuildPlayerScoped(*Player, {LocalProducer}, false);
    TestTrue(TEXT("Exact owned producer publishes its player-scoped queue"),
        ProducerOnly.Production.bVisible &&
        ProducerOnly.Production.ProducerId == LocalProducer &&
        ProducerOnly.Production.Items.Num() == 3);
    if (ProducerOnly.Production.Items.Num() == 3)
    {
        const FEchoesFieldHudProductionItem& Active =
            ProducerOnly.Production.Items[0];
        const FEchoesFieldHudProductionItem& FirstWaiting =
            ProducerOnly.Production.Items[1];
        TestTrue(TEXT("Active queue item distinguishes progress, investment, and reserved Logistics"),
            Active.bActive && Active.Slot == 0 &&
            Active.ItemId == 7001 &&
            Active.ProgressPercent == 30 &&
            Active.InvestedMatter == 60 && Active.InvestedDawn == 12 &&
            Active.Logistics == 1);
        TestTrue(TEXT("Waiting queue item remains unpaid and reports activation cost"),
            !FirstWaiting.bActive && FirstWaiting.Slot == 1 &&
            FirstWaiting.ItemId == 7002 &&
            FirstWaiting.ConfiguredMatter == 85 &&
            FirstWaiting.ConfiguredDawn == 20 &&
            FirstWaiting.InvestedMatter == 0 &&
            FirstWaiting.InvestedDawn == 0 &&
            FirstWaiting.Logistics == 2);
    }
    TestTrue(TEXT("Queue alerts and rally length come only from scoped producer state"),
        ProducerOnly.Production.bSpawnBlocked &&
        ProducerOnly.Production.bRallyNeedsAttention &&
        ProducerOnly.Production.RallyWaypointCount == 1);
    TestTrue(TEXT("Waiting queue exposes explicit cancel and enabled directional reorder controls"),
        ProducerOnly.Production.Controls.ContainsByPredicate(
            [](const FEchoesFieldHudControl& Control)
            {
                return Control.Action ==
                        EEchoesFieldHudAction::ProductionCancel &&
                    Control.Argument == 1 && Control.bEnabled;
            }) &&
        ProducerOnly.Production.Controls.ContainsByPredicate(
            [](const FEchoesFieldHudControl& Control)
            {
                return Control.Action ==
                        EEchoesFieldHudAction::ProductionMoveDown &&
                    Control.Argument == 1 && Control.bEnabled;
            }) &&
        ProducerOnly.Production.Controls.ContainsByPredicate(
            [](const FEchoesFieldHudControl& Control)
            {
                return Control.Action ==
                        EEchoesFieldHudAction::ProductionMoveUp &&
                    Control.Argument == 2 && Control.bEnabled;
            }));
    TestTrue(TEXT("Waiting cancellation displays its exact zero refund before review"),
        ProducerOnly.Production.Controls.ContainsByPredicate(
            [](const FEchoesFieldHudControl& Control)
            {
                return Control.Action ==
                        EEchoesFieldHudAction::ProductionCancel &&
                    Control.Argument == 1 &&
                    Control.Detail.ToString().Contains(
                        TEXT("0 Matter / 0 Dawn"));
            }));
    TestTrue(TEXT("Active cancellation presents the exact current refund before dispatch"),
        ProducerOnly.Production.Controls.ContainsByPredicate(
            [](const FEchoesFieldHudControl& Control)
            {
                return Control.Action ==
                        EEchoesFieldHudAction::ProductionCancel &&
                    Control.Argument == 0 &&
                    Control.Detail.ToString().Contains(
                        TEXT("45 Matter / 9 Dawn"));
            }));
    const FEchoesFieldHudView HiddenProducer =
        FEchoesFieldHudModel::BuildPlayerScoped(
            *Player, {HiddenEnemyProducer}, false);
    TestFalse(TEXT("An enemy producer never publishes its queue"),
        HiddenProducer.Production.bVisible);
    const FEchoesFieldHudView Roster =
        FEchoesFieldHudModel::BuildPlayerScoped(*Player, {LocalSurveyor}, false);
    TestTrue(TEXT("Surveyor selection publishes canonical mechanical purpose"),
        Roster.Selection.Entries.Num() == 1 &&
        Roster.Selection.Entries[0].Name.ToString() == TEXT("Surveyor") &&
        Roster.Selection.Entries[0].Purpose.ToString().Contains(
            TEXT("Core economic builder and logistics conduit")) &&
        Roster.Selection.Entries[0].MaxHitPoints > 0 &&
        !Roster.Selection.Entries[0].Order.IsEmpty());
    TestFalse(TEXT("Hidden opponent does not leak onto scoped minimap"),
        Live.Minimap.Markers.ContainsByPredicate(
            [HiddenEnemy](const FEchoesFieldHudMapMarker& Marker)
            {
                return Marker.EntityId == HiddenEnemy;
            }));
    TestEqual(TEXT("Minimap has one semantic tile per map tile"),
        Live.Minimap.Tiles.Num(), Config.mapWidthTiles * Config.mapHeightTiles);

    const FEchoesFieldHudView ReplayPlayer =
        FEchoesFieldHudModel::BuildPlayerScoped(*Player, Selected, true);
    TestEqual(TEXT("Replay perspective declares detached player authority"),
        ReplayPlayer.Authority, EEchoesFieldHudAuthority::ReplayPlayerView);
    TestTrue(TEXT("Replay retains its scoped minimap"),
        ReplayPlayer.Minimap.bVisible);
    TestFalse(TEXT("Replay never inherits the live economy ledger"),
        ReplayPlayer.Resources.bVisible);
    TestFalse(TEXT("Replay never inherits live selection"),
        ReplayPlayer.Selection.bVisible);
    TestFalse(TEXT("Replay never inherits a live producer queue"),
        ReplayPlayer.Production.bVisible);
    TestFalse(TEXT("Replay never inherits live commands"),
        ReplayPlayer.Commands.bVisible);
    TestFalse(TEXT("Replay never inherits live technology"),
        ReplayPlayer.Technology.bVisible);
    TestTrue(TEXT("Replay never inherits live objectives"),
        ReplayPlayer.ObjectiveLines.IsEmpty());

    const FEchoesFieldHudView Observer =
        FEchoesFieldHudModel::BuildReplayObserver(SimulationValue);
    TestEqual(TEXT("Observer view declares detached observer authority"),
        Observer.Authority, EEchoesFieldHudAuthority::ReplayObserver);
    TestTrue(TEXT("Observer minimap may show recorded opponent state"),
        Observer.Minimap.Markers.ContainsByPredicate(
            [HiddenEnemy](const FEchoesFieldHudMapMarker& Marker)
            {
                return Marker.EntityId == HiddenEnemy;
            }));
    TestFalse(TEXT("Observer still has no live economy ledger"),
        Observer.Resources.bVisible);

    echoes::sim::net::ScopedViewKeyframe Keyframe;
    Keyframe.mapWidthTiles = 2;
    Keyframe.mapHeightTiles = 2;
    Keyframe.player = 1;
    Keyframe.faction = Faction::KharuunAssemblies;
    Keyframe.resources = {77, 13};
    Keyframe.populationUsed = 3;
    Keyframe.populationCapacity = 9;
    Keyframe.simulationTick = 88;
    Keyframe.tiles.assign(4, echoes::sim::net::ScopedTileState{
        Visibility::Visible, Terrain::Open, true});
    echoes::sim::net::ScopedEntityState ScopedEntity;
    ScopedEntity.id = 41;
    ScopedEntity.owner = 1;
    ScopedEntity.faction = Faction::KharuunAssemblies;
    ScopedEntity.type = EntityType::Worker;
    ScopedEntity.position = Vec2::FromTiles(1, 1);
    ScopedEntity.hitPoints = 25;
    ScopedEntity.maxHitPoints = 40;
    Keyframe.entities.push_back(ScopedEntity);
    const FEchoesFieldHudView Network =
        FEchoesFieldHudModel::BuildNetworkScoped(Keyframe, {41, 999});
    TestEqual(TEXT("Network view declares keyframe authority"),
        Network.Authority, EEchoesFieldHudAuthority::NetworkKeyframe);
    TestTrue(TEXT("Network ledger is exactly keyframe-scoped"),
        Network.Resources.Matter == 77 && Network.Resources.Dawn == 13 &&
            Network.Resources.SimulationTick == 88);
    TestEqual(TEXT("Unknown selected IDs do not create entries"),
        Network.Selection.Entries.Num(), 1);
    TestEqual(TEXT("Network minimap consumes keyframe tiles only"),
        Network.Minimap.Tiles.Num(), 4);
    TestEqual(TEXT("Network minimap consumes keyframe entities only"),
        Network.Minimap.Markers.Num(), 1);
    TestFalse(TEXT("Network keyframes without scoped queue data reveal no producer queue"),
        Network.Production.bVisible);
    TestTrue(TEXT("Network commands derive from the owned scoped selection"),
        Network.Commands.bVisible &&
            Network.Commands.Controls.ContainsByPredicate(
                [](const FEchoesFieldHudControl& Control)
                {
                    return Control.Argument == static_cast<int32>(
                        EEchoesCommandDeckAction::BuildBarracks);
                }));

    FEchoesFieldHudView FailedOutput = Observer;
    FString Error;
    FEchoesFieldHudBuildContext Missing;
    TestFalse(TEXT("Missing authorities are refused"),
        FEchoesFieldHudModel::Build(Missing, FailedOutput, Error));
    TestTrue(TEXT("Refusal returns a stable actionable reason"),
        Error.StartsWith(TEXT("[FIELD_HUD_SOURCE_MISSING]")));
    TestEqual(TEXT("Refusal clears stale output"),
        FailedOutput.Surface, EEchoesFieldHudSurface::Hidden);
    // Separate real simulation fixture: dynamic range, chained coverage, severance,
    // unfinished nodes and replay scoping must agree with authoritative power state.
    SimulationConfig NetworkConfig;
    NetworkConfig.mapWidthTiles = 32;
    NetworkConfig.mapHeightTiles = 32;
    NetworkConfig.rules.poweredAegis.connectionRadiusRaw = 4 * kFixedScale;
    Simulation PowerSimulation(NetworkConfig);
    if (!TestTrue(TEXT("Network fixture player admitted"),
        PowerSimulation.AddPlayer(0, Faction::MeridianCompact, {2000, 2000}))) return false;
    const EntityId Root = PowerSimulation.SpawnEntity(0, Faction::MeridianCompact, EntityType::CommandCore, Vec2::FromTiles(4, 8));
    const EntityId Link = PowerSimulation.SpawnEntity(0, Faction::MeridianCompact, EntityType::Dropoff, Vec2::FromTiles(8, 8));
    const EntityId FarLink = PowerSimulation.SpawnEntity(0, Faction::MeridianCompact, EntityType::Dropoff, Vec2::FromTiles(12, 8));
    Entity* Middle = PowerSimulation.MutableEntityForTesting(Link);
    Entity* Far = PowerSimulation.MutableEntityForTesting(FarLink);
    if (!TestTrue(TEXT("Network fixture prerequisites exist"), Root != 0 && Middle != nullptr && Far != nullptr)) return false;
    PowerSimulation.Step();
    auto NetworkPlayer = PowerSimulation.CreatePlayerView(0);
    if (!TestTrue(TEXT("Network view exists"), NetworkPlayer.has_value())) return false;
    auto NetworkHud = FEchoesFieldHudModel::BuildPlayerScoped(*NetworkPlayer, {FarLink}, false);
    if (!TestEqual(TEXT("Selected network node exposes one coverage area"), NetworkHud.NetworkCoverage.Num(), 1)) return false;
    TestEqual(TEXT("Coverage uses configured tiles converted to world units"), NetworkHud.NetworkCoverage[0].Radius, 4.0f * UEchoesSimulationSubsystem::TileWorldSize);
    TestTrue(TEXT("Exact-radius two-hop connection is operational"), NetworkHud.NetworkCoverage[0].bOperational);
    TestEqual(TEXT("Connection display reaches root through two edges"), NetworkHud.NetworkConnections.Num(), 2);
    const auto UnselectedNetwork = FEchoesFieldHudModel::BuildPlayerScoped(*NetworkPlayer, {}, false);
    TestEqual(TEXT("Live conduits remain visible without selecting infrastructure"), UnselectedNetwork.NetworkConnections.Num(), 2);
    TestTrue(TEXT("Unselected conduits do not add coverage clutter"), UnselectedNetwork.NetworkCoverage.IsEmpty());
    TestTrue(TEXT("Replay cannot inherit live conduits"),
        FEchoesFieldHudModel::BuildPlayerScoped(*NetworkPlayer, {}, true).NetworkConnections.IsEmpty());
    TestTrue(TEXT("Power Link has canonical name and real benefits"),
        NetworkHud.Selection.Entries.Num() == 1 && NetworkHud.Selection.Entries[0].Name.ToString() == TEXT("Power Link") &&
        NetworkHud.Selection.Entries[0].Purpose.ToString().Contains(TEXT("Matter drop-off")) &&
        NetworkHud.Selection.Entries[0].Purpose.ToString().Contains(TEXT("4 tiles")));
    TestTrue(TEXT("Replay cannot expose live selection network overlay"),
        FEchoesFieldHudModel::BuildPlayerScoped(*NetworkPlayer, {FarLink}, true).NetworkCoverage.IsEmpty());
    const EntityId Aegis = PowerSimulation.SpawnEntity(0, Faction::MeridianCompact, EntityType::UtilityStructure, Vec2::FromTiles(16, 8));
    const EntityId BeyondAegis = PowerSimulation.SpawnEntity(0, Faction::MeridianCompact, EntityType::Dropoff, Vec2::FromTiles(20, 8));
    if (!TestTrue(TEXT("Aegis endpoint fixture entities exist"), Aegis != 0 && BeyondAegis != 0)) return false;
    PowerSimulation.Step();
    NetworkPlayer = PowerSimulation.CreatePlayerView(0);
    if (!TestTrue(TEXT("Aegis endpoint scoped view exists"), NetworkPlayer.has_value())) return false;
    NetworkHud = FEchoesFieldHudModel::BuildPlayerScoped(*NetworkPlayer, {Aegis}, false);
    TestEqual(TEXT("Powered Aegis adds one terminal conduit"), NetworkHud.NetworkConnections.Num(), 3);
    TestTrue(TEXT("Aegis never advertises relay coverage"), NetworkHud.NetworkCoverage.IsEmpty());
    TestTrue(TEXT("Aegis explains its powered non-relay role"), NetworkHud.Selection.Entries.Num() == 1 &&
        NetworkHud.Selection.Entries[0].Purpose.ToString().Contains(TEXT("weapons powered")) &&
        NetworkHud.Selection.Entries[0].Purpose.ToString().Contains(TEXT("does not extend")));
    const Entity* Beyond = PowerSimulation.MutableEntityForTesting(BeyondAegis);
    if (!TestNotNull(TEXT("Beyond-Aegis node exists"), Beyond)) return false;
    TestFalse(TEXT("Aegis does not power a downstream relay"), Beyond->networkOperational);
    TestTrue(TEXT("Aegis cannot leak live conduits into replay"),
        FEchoesFieldHudModel::BuildPlayerScoped(*NetworkPlayer, {Aegis}, true).NetworkConnections.IsEmpty());
    Middle = PowerSimulation.MutableEntityForTesting(Link);
    if (!TestNotNull(TEXT("Middle node remains addressable"), Middle)) return false;
    Middle->hitPoints = 0;
    PowerSimulation.Step();
    NetworkPlayer = PowerSimulation.CreatePlayerView(0);
    if (!TestTrue(TEXT("Severed network view exists"), NetworkPlayer.has_value())) return false;
    NetworkHud = FEchoesFieldHudModel::BuildPlayerScoped(*NetworkPlayer, {FarLink}, false);
    TestTrue(TEXT("Severed link shows potential range but no active connection"),
        NetworkHud.NetworkCoverage.Num() == 1 && !NetworkHud.NetworkCoverage[0].bOperational && NetworkHud.NetworkConnections.IsEmpty());
    const auto OfflineAegis = FEchoesFieldHudModel::BuildPlayerScoped(*NetworkPlayer, {Aegis}, false);
    TestTrue(TEXT("Power loss removes Aegis conduit and identifies offline weapons"),
        OfflineAegis.NetworkConnections.IsEmpty() && OfflineAegis.Selection.Entries.Num() == 1 &&
        OfflineAegis.Selection.Entries[0].Purpose.ToString().Contains(TEXT("weapons offline")));
    Far = PowerSimulation.MutableEntityForTesting(FarLink);
    if (!TestNotNull(TEXT("Far node remains addressable"), Far)) return false;
    Far->completed = false;
    PowerSimulation.Step();
    NetworkPlayer = PowerSimulation.CreatePlayerView(0);
    if (!TestTrue(TEXT("Foundation network view exists"), NetworkPlayer.has_value())) return false;
    TestTrue(TEXT("Foundation does not advertise operational coverage"),
        FEchoesFieldHudModel::BuildPlayerScoped(*NetworkPlayer, {FarLink}, false).NetworkCoverage.IsEmpty());
    // Rebuild the severed relay through the simulation fixture and verify the
    // presentation follows restored authority rather than retaining an outage.
    const EntityId Replacement = PowerSimulation.SpawnEntity(0, Faction::MeridianCompact, EntityType::Dropoff, Vec2::FromTiles(8, 8));
    Far = PowerSimulation.MutableEntityForTesting(FarLink);
    if (!TestTrue(TEXT("Restoration prerequisites exist"), Replacement != 0 && Far != nullptr)) return false;
    Far->completed = true;
    PowerSimulation.Step();
    NetworkPlayer = PowerSimulation.CreatePlayerView(0);
    if (!TestTrue(TEXT("Restored endpoint scoped view exists"), NetworkPlayer.has_value())) return false;
    const auto RestoredAegis = FEchoesFieldHudModel::BuildPlayerScoped(*NetworkPlayer, {Aegis}, false);
    TestEqual(TEXT("Restoring the relay restores the Aegis terminal conduit"), RestoredAegis.NetworkConnections.Num(), 3);

    // Barrier feedback must follow real production state changes, including
    // the interval where an order was accepted but protection is not active.
    Simulation BarrierSimulation(SimulationConfig{64, 64, 20, 0x42415252494552ULL});
    if (!TestTrue(TEXT("Barrier player exists"), BarrierSimulation.AddPlayer(0, Faction::MeridianCompact, {1000, 500}))) return false;
    const auto Barrier = BarrierSimulation.SpawnEntity(0, Faction::MeridianCompact, EntityType::HeavyUnit, Vec2::FromTiles(10, 10));
    const auto BarrierScout = BarrierSimulation.SpawnEntity(0, Faction::MeridianCompact, EntityType::ScoutUnit, Vec2::FromTiles(12, 10));
    if (!TestTrue(TEXT("Barrier feedback prerequisites spawn"), Barrier && BarrierScout)) return false;
    const auto BarrierView = [&](const TArray<uint32>& Selection)
    {
        const auto Scoped = BarrierSimulation.CreatePlayerView(0);
        return Scoped ? FEchoesFieldHudModel::BuildPlayerScoped(*Scoped, Selection, false) : FEchoesFieldHudView{};
    };
    auto BarrierHud = BarrierView({Barrier});
    if (!TestEqual(TEXT("Bulwark has one ability"), BarrierHud.Commands.Controls.Num(), 1) ||
        !TestEqual(TEXT("Bulwark has one selection entry"), BarrierHud.Selection.Entries.Num(), 1)) return false;
    TestEqual(TEXT("Bulwark uses the roster name"), BarrierHud.Selection.Entries[0].Name.ToString(), FString(TEXT("Bulwark Team")));
    TestTrue(TEXT("Current live Bulwark explains current commitments"),
        BarrierHud.Selection.Entries[0].Purpose.ToString().Contains(TEXT("Deploy: 1s; pack: 0.75s")));

    // Authenticate the historical writer's oracle, then follow the same
    // snapshot plus replay-prefix continuation used by live checkpoint load.
    TArray<uint8> LegacyBytes;
    if (!TestTrue(TEXT("Historical Bulwark baseline loads"), FFileHelper::LoadFileToArray(
        LegacyBytes, *(FPaths::ProjectDir() / TEXT("Tests/Native/Fixtures/LegacyReplay/schema30-bulwark-baseline.bin")))) ||
        !TestTrue(TEXT("Historical Bulwark baseline is nonempty"), LegacyBytes.Num() > 0)) return false;
    ReplayRecord LegacyReplay;
    LegacyReplay.version = kLinkMechanicsReplayVersion;
    LegacyReplay.initialSnapshot.assign(LegacyBytes.GetData(), LegacyBytes.GetData() + LegacyBytes.Num());
    LegacyReplay.finalTick = 2;
    LegacyReplay.finalChecksum = 17785241889350991135ULL;
    Command LegacyDeploy{};
    LegacyDeploy.player = 0; LegacyDeploy.actor = 1; LegacyDeploy.sequence = 1;
    LegacyDeploy.type = CommandType::ToggleDeploy; LegacyDeploy.position = Vec2::FromTiles(12, 10);
    Command LegacyAttack{};
    LegacyAttack.player = 1; LegacyAttack.actor = 3; LegacyAttack.sequence = 1;
    LegacyAttack.type = CommandType::Attack; LegacyAttack.target = 2;
    auto LegacyPack = LegacyDeploy; LegacyPack.executeTick = 1; LegacyPack.sequence = 2;
    LegacyReplay.commands = {LegacyDeploy, LegacyAttack, LegacyPack};
    std::string LegacyError;
    auto Historical = Simulation::ReplayToEnd(LegacyReplay, &LegacyError);
    if (!TestTrue(TEXT("Historical Bulwark replay verifies authentic checksum"), Historical.has_value()))
    { AddError(UTF8_TO_TCHAR(LegacyError.c_str())); return false; }
    auto Continued = Simulation::LoadSnapshot(Historical->SaveSnapshot(), &LegacyError);
    if (!TestTrue(TEXT("Historical checkpoint snapshot restores"), Continued.has_value())) return false;
    if (!TestTrue(TEXT("Historical checkpoint continues replay prefix"), Continued->ContinueReplayRecording(LegacyReplay, &LegacyError)))
    { AddError(UTF8_TO_TCHAR(LegacyError.c_str())); return false; }
    auto LegacyView = Continued->CreatePlayerView(0);
    if (!TestTrue(TEXT("Continued historical owner view exists"), LegacyView.has_value())) return false;
    auto LegacyHud = FEchoesFieldHudModel::BuildPlayerScoped(*LegacyView, {1}, false);
    if (!TestEqual(TEXT("Historical live HUD has selected Bulwark"), LegacyHud.Selection.Entries.Num(), 1)) return false;
    TestFalse(TEXT("Historical live HUD does not advertise timed deployment"),
        LegacyHud.Selection.Entries[0].Purpose.ToString().Contains(TEXT("Deploy: 1s")));
    LegacyDeploy.executeTick = Continued->CurrentTick(); LegacyDeploy.sequence = 3;
    if (!TestTrue(TEXT("Historical live deploy is admitted"), Continued->QueueCommand(LegacyDeploy))) return false;
    Continued->Step();
    const auto* HistoricalUnit = Continued->FindEntity(1);
    if (!TestNotNull(TEXT("Historical deployed Bulwark remains present"), HistoricalUnit)) return false;
    TestTrue(TEXT("Historical commitment remains instantaneous"), HistoricalUnit->deployed &&
        HistoricalUnit->deploymentPhase == BulwarkDeploymentPhase::None);
    LegacyView = Continued->CreatePlayerView(0);
    if (!TestTrue(TEXT("Historical deployed owner view exists"), LegacyView.has_value())) return false;
    LegacyHud = FEchoesFieldHudModel::BuildPlayerScoped(*LegacyView, {1}, false);
    TestFalse(TEXT("Historical deployed feedback does not claim current 120-degree arc"),
        LegacyHud.Commands.AbilityStatus.ToString().Contains(TEXT("120-degree")));
    TestTrue(TEXT("Historical deployed feedback explains its front protection"),
        LegacyHud.Commands.AbilityStatus.ToString().Contains(TEXT("front")));
    TestTrue(TEXT("Mobile Bulwark offers deployment and no active protection"),
        BarrierHud.Commands.Controls[0].bEnabled && BarrierHud.Commands.Controls[0].Label.ToString() == TEXT("DEPLOY BARRIER") &&
        BarrierHud.Commands.AbilityStatus.ToString().Contains(TEXT("barrier inactive")));
    const auto MixedBarrierHud = BarrierView({Barrier, BarrierScout});
    TestTrue(TEXT("Mixed selection retains both distinct ability states"),
        MixedBarrierHud.Commands.Controls.Num() == 2 && MixedBarrierHud.Commands.AbilityStatus.ToString().Contains(TEXT("EXTEND RELAY")) &&
        MixedBarrierHud.Commands.AbilityStatus.ToString().Contains(TEXT("BARRIER")));
    Command BarrierOrder{};
    BarrierOrder.player = 0; BarrierOrder.actor = Barrier; BarrierOrder.sequence = 1;
    BarrierOrder.type = CommandType::ToggleDeploy; BarrierOrder.position = Vec2::FromTiles(20, 10);
    if (!TestTrue(TEXT("Barrier deploy admitted"), BarrierSimulation.QueueCommand(BarrierOrder))) return false;
    BarrierSimulation.Step(10);
    BarrierHud = BarrierView({Barrier});
    if (!TestEqual(TEXT("Deploying ability remains present"), BarrierHud.Commands.Controls.Num(), 1)) return false;
    TestTrue(TEXT("Mid-deployment shows progress and disables repeat"),
        !BarrierHud.Commands.Controls[0].bEnabled && BarrierHud.Commands.AbilityStatus.ToString().Contains(TEXT("Deploying 50%")) &&
        BarrierHud.Commands.AbilityStatus.ToString().Contains(TEXT("not active yet")));
    echoes::sim::net::ScopedViewKeyframe BarrierKeyframe;
    BarrierKeyframe.player = 0; BarrierKeyframe.simulationTick = BarrierSimulation.CurrentTick();
    echoes::sim::net::ScopedEntityState RemoteBarrier;
    const auto* RealBarrier = BarrierSimulation.FindEntity(Barrier);
    if (!TestNotNull(TEXT("Executing barrier remains live"), RealBarrier)) return false;
    RemoteBarrier.id = Barrier; RemoteBarrier.owner = 0;
    RemoteBarrier.type = EntityType::HeavyUnit; RemoteBarrier.faction = Faction::MeridianCompact;
    RemoteBarrier.deployed = RealBarrier->deployed; RemoteBarrier.deploymentPhase = RealBarrier->deploymentPhase;
    RemoteBarrier.deploymentTransitionUntilTick = RealBarrier->deploymentTransitionUntilTick;
    RemoteBarrier.deploymentFacing = RealBarrier->deploymentFacing;
    BarrierKeyframe.entities.push_back(RemoteBarrier);
    const auto RemoteBarrierHud = FEchoesFieldHudModel::BuildNetworkScoped(BarrierKeyframe, {Barrier});
    TestTrue(TEXT("Remote owner sees the same authoritative deployment progress"),
        RemoteBarrierHud.Commands.AbilityStatus.EqualTo(BarrierHud.Commands.AbilityStatus) &&
        RemoteBarrierHud.Commands.Controls.ContainsByPredicate([](const auto& Control)
        { return Control.Action == EEchoesFieldHudAction::CommandDeck && !Control.bEnabled &&
            Control.Argument == static_cast<int32>(EEchoesCommandDeckAction::ToggleBulwarkDeployment); }));
    BarrierKeyframe.player = 1;
    const auto EnemyBarrierHud = FEchoesFieldHudModel::BuildNetworkScoped(BarrierKeyframe, {Barrier});
    TestTrue(TEXT("Remote enemy cannot receive the Bulwark ability control"), EnemyBarrierHud.Commands.AbilityStatus.IsEmpty());
    BarrierSimulation.Step(10);
    BarrierHud = BarrierView({Barrier});
    if (!TestEqual(TEXT("Deployed ability remains present"), BarrierHud.Commands.Controls.Num(), 1)) return false;
    TestTrue(TEXT("Deployed state offers packing and explains frontal protection"),
        BarrierHud.Commands.Controls[0].bEnabled && BarrierHud.Commands.Controls[0].Label.ToString() == TEXT("PACK BARRIER") &&
        BarrierHud.Commands.AbilityStatus.ToString().Contains(TEXT("120-degree")) &&
        BarrierHud.Commands.AbilityStatus.ToString().Contains(TEXT("40%")));
    BarrierOrder.sequence = 2; BarrierOrder.executeTick = BarrierSimulation.CurrentTick();
    if (!TestTrue(TEXT("Barrier pack admitted"), BarrierSimulation.QueueCommand(BarrierOrder))) return false;
    BarrierSimulation.Step(5);
    BarrierHud = BarrierView({Barrier});
    if (!TestEqual(TEXT("Packing ability remains present"), BarrierHud.Commands.Controls.Num(), 1)) return false;
    TestTrue(TEXT("Packing keeps protection and refuses repeat"), !BarrierHud.Commands.Controls[0].bEnabled &&
        BarrierHud.Commands.AbilityStatus.ToString().Contains(TEXT("Packing")) &&
        BarrierHud.Commands.AbilityStatus.ToString().Contains(TEXT("remains active")));
    BarrierSimulation.Step(10);
    TestTrue(TEXT("Completed packing restores inactive feedback"),
        BarrierView({Barrier}).Commands.AbilityStatus.ToString().Contains(TEXT("barrier inactive")));

    // Relay UI is derived from owned state and follows real command execution,
    // movement, expiry and restoration. No seeded ability/cooldown flags.
    Simulation RelaySimulation(SimulationConfig{64, 64, 20, 0x52454C4159ULL});
    if (!TestTrue(TEXT("Relay feedback player exists"), RelaySimulation.AddPlayer(0, Faction::MeridianCompact, {1000, 500}))) return false;
    const auto RelayCore = RelaySimulation.SpawnEntity(0, Faction::MeridianCompact, EntityType::CommandCore, Vec2::FromTiles(10, 10));
    const auto RelayLink = RelaySimulation.SpawnEntity(0, Faction::MeridianCompact, EntityType::Dropoff, Vec2::FromTiles(13, 12));
    const auto Skiff = RelaySimulation.SpawnEntity(0, Faction::MeridianCompact, EntityType::ScoutUnit, Vec2::FromTiles(12, 10));
    const auto IsolatedSkiff = RelaySimulation.SpawnEntity(0, Faction::MeridianCompact, EntityType::ScoutUnit, Vec2::FromTiles(30, 30));
    if (!TestTrue(TEXT("Relay feedback prerequisites spawned"), RelayCore && RelayLink && Skiff && IsolatedSkiff)) return false;
    const auto RelayView = [&](const TArray<uint32>& Selection)
    {
        const auto Scoped = RelaySimulation.CreatePlayerView(0);
        return Scoped ? FEchoesFieldHudModel::BuildPlayerScoped(*Scoped, Selection, false) : FEchoesFieldHudView{};
    };
    auto RelayHud = RelayView({Skiff});
    if (!TestEqual(TEXT("One Skiff emits its ability"), RelayHud.Commands.Controls.Num(), 1)) return false;
    TestTrue(TEXT("Ready Skiff offers authoritative activation"), RelayHud.Commands.Controls[0].bEnabled &&
        RelaySimulation.ValidateRelaySupply(0, Skiff) == RelaySupplyResult::Valid);
    if (!TestEqual(TEXT("Skiff selection detail exists"), RelayHud.Selection.Entries.Num(), 1)) return false;
    TestEqual(TEXT("Skiff uses its roster name"), RelayHud.Selection.Entries[0].Name.ToString(), FString(TEXT("Relay Skiff")));
    TestTrue(TEXT("Skiff explains benefit and duration"), RelayHud.Selection.Entries[0].Purpose.ToString().Contains(TEXT("+4 Logistics for 20s")));
    TestTrue(TEXT("Relay availability is separate from disabled control content"),
        RelayHud.Commands.AbilityStatus.ToString().Contains(TEXT("EXTEND RELAY")) &&
        RelayHud.Commands.AbilityStatus.ToString().Contains(TEXT("Ready")));
    const auto MixedRelayHud = RelayView({RelayLink, IsolatedSkiff, Skiff});
    TestTrue(TEXT("Mixed selection counts only eligible Skiffs"), MixedRelayHud.Commands.Controls.Num() == 1 &&
        MixedRelayHud.Commands.Controls[0].bEnabled && MixedRelayHud.Commands.Controls[0].Detail.ToString().Contains(TEXT("1/2")));
    TestTrue(TEXT("Power Link never advertises the Skiff ability"), RelayView({RelayLink}).Commands.Controls.IsEmpty());
    TestTrue(TEXT("Missing selection never advertises an ability"), RelayView({999999}).Commands.Controls.IsEmpty());
    const int32 BaseCapacity = RelaySimulation.PopulationCapacity(0);
    RelaySimulation.CaptureReplayBaseline();
    Command Extend;
    Extend.player = 0; Extend.actor = Skiff; Extend.sequence = 1;
    Extend.executeTick = RelaySimulation.CurrentTick(); Extend.type = CommandType::ActivateRelaySupply;
    if (!TestTrue(TEXT("Real Extend Relay command is admitted"), RelaySimulation.QueueCommand(Extend))) return false;
    RelaySimulation.Step();
    RelayHud = RelayView({Skiff});
    TestTrue(TEXT("Active state disables repeat input and describes real capacity"),
        RelayHud.Commands.Controls.Num() == 1 && !RelayHud.Commands.Controls[0].bEnabled && RelayHud.Commands.Controls[0].Detail.ToString().Contains(TEXT("Active: +4")) &&
        RelaySimulation.PopulationCapacity(0) == BaseCapacity + 4);
    Command Leave = Extend;
    Leave.sequence = 2; Leave.executeTick = RelaySimulation.CurrentTick(); Leave.type = CommandType::Move;
    Leave.position = Vec2::FromTiles(30, 20);
    if (!TestTrue(TEXT("Active Skiff can leave its connection"), RelaySimulation.QueueCommand(Leave))) return false;
    RelaySimulation.Step(200);
    RelayHud = RelayView({Skiff});
    TestTrue(TEXT("Active disconnect shows zero benefit and actual lost capacity"),
        RelayHud.Commands.Controls.Num() == 1 && RelayHud.Commands.Controls[0].Detail.ToString().Contains(TEXT("Disconnected: +0")) &&
        RelaySimulation.PopulationCapacity(0) == BaseCapacity);
    RelaySimulation.Step(RelaySimulation.Config().rules.relaySupply.durationTicks - RelaySimulation.CurrentTick());
    RelayHud = RelayView({Skiff});
    TestTrue(TEXT("Expired ability shows cooldown without offering recast"), RelayHud.Commands.Controls.Num() == 1 && !RelayHud.Commands.Controls[0].bEnabled &&
        RelayHud.Commands.Controls[0].Detail.ToString().Contains(TEXT("Cooldown:")));
    RelaySimulation.Step(RelaySimulation.Config().rules.relaySupply.cooldownTicks - RelaySimulation.CurrentTick());
    RelayHud = RelayView({Skiff});
    TestTrue(TEXT("Cooldown completion cannot hide missing connection"), RelayHud.Commands.Controls.Num() == 1 && !RelayHud.Commands.Controls[0].bEnabled &&
        RelayHud.Commands.Controls[0].Detail.ToString().StartsWith(TEXT("Disconnected:")));
    Command Return = Leave;
    Return.sequence = 3; Return.executeTick = RelaySimulation.CurrentTick(); Return.position = Vec2::FromTiles(12, 10);
    if (!TestTrue(TEXT("Skiff return is admitted"), RelaySimulation.QueueCommand(Return))) return false;
    RelaySimulation.Step(200);
    RelayHud = RelayView({Skiff});
    TestTrue(TEXT("Returning to a valid network restores readiness"), RelayHud.Commands.Controls.Num() == 1 && RelayHud.Commands.Controls[0].bEnabled &&
        RelaySimulation.ValidateRelaySupply(0, Skiff) == RelaySupplyResult::Valid);
    std::string RelayError;
    auto RestoredRelay = Simulation::LoadSnapshot(RelaySimulation.SaveSnapshot(), &RelayError);
    if (!TestTrue(TEXT("Relay state snapshot restores"), RestoredRelay.has_value())) return false;
    const auto RestoredRelayPlayer = RestoredRelay->CreatePlayerView(0);
    if (!TestTrue(TEXT("Restored Relay owner view exists"), RestoredRelayPlayer.has_value())) return false;
    const auto RestoredRelayHud = FEchoesFieldHudModel::BuildPlayerScoped(*RestoredRelayPlayer, {Skiff}, false);
    TestTrue(TEXT("Restored ability remains ready"), RestoredRelayHud.Commands.Controls.Num() == 1 && RestoredRelayHud.Commands.Controls[0].bEnabled);
    TestTrue(TEXT("Replay never emits actionable Relay control"), FEchoesFieldHudModel::BuildPlayerScoped(*RestoredRelayPlayer, {Skiff}, true).Commands.Controls.IsEmpty());
    auto ReplayedRelay = Simulation::ReplayToEnd(RelaySimulation.ExportReplay(), &RelayError);
    TestTrue(TEXT("Relay presentation leaves deterministic replay unchanged"), ReplayedRelay.has_value() &&
        ReplayedRelay->StateChecksum() == RelaySimulation.StateChecksum());

    return true;
}

#endif
