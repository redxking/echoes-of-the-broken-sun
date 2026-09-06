// Copyright Echoes of the Broken Sun. All Rights Reserved.
// Author: Angelis Pseftis

#include "EchoesFieldHudView.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "EchoesTestSaveEnvironment.h"
#include "Misc/AutomationTest.h"

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
    const EntityId HiddenEnemy = SimulationValue.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::Soldier,
        Vec2::FromTiles(14, 14));
    TestTrue(TEXT("Fixture entities exist"),
        LocalCore != 0 && LocalProducer != 0 && HiddenEnemy != 0);
    if (Entity* Producer = SimulationValue.MutableEntityForTesting(LocalProducer))
    {
        Producer->productionType = EntityType::Worker;
        Producer->productionProgress = 9;
        Producer->productionRequired = 30;
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
    return true;
}

#endif
