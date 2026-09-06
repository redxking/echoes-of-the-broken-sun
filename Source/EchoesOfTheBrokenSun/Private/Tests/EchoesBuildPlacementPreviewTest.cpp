#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesBuildPlacementPreview.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesBuildPlacementPreviewTest,
    "Echoes.Runtime.Input.BuildPlacementPreview",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesBuildPlacementPreviewTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    echoes::sim::SimulationConfig Config;
    Config.mapWidthTiles = 24;
    Config.mapHeightTiles = 24;
    echoes::sim::Simulation Simulation(Config);
    TestTrue(TEXT("Local player is created"),
        Simulation.AddPlayer(0, echoes::sim::Faction::MeridianCompact,
            echoes::sim::ResourcePool{2000, 2000}));
    const echoes::sim::EntityId Worker = Simulation.SpawnEntity(
        0,
        echoes::sim::Faction::MeridianCompact,
        echoes::sim::EntityType::Worker,
        echoes::sim::Vec2::FromTiles(8, 8));
    Simulation.Step();
    const std::optional<echoes::sim::PlayerView> View =
        Simulation.CreatePlayerView(0);
    if (!TestTrue(TEXT("Player-scoped construction view materializes"),
            View.has_value()))
    {
        return false;
    }

    const FEchoesBuildPlacementEvaluation Valid =
        FEchoesBuildPlacementModel::Evaluate(
            *View,
            Worker,
            echoes::sim::EntityType::Barracks,
            echoes::sim::Vec2::FromTiles(12, 8));
    TestTrue(TEXT("Visible clear ground previews as valid"), Valid.IsValid());
    TestTrue(TEXT("Preview exposes the authored footprint"),
        Valid.FootprintHalfExtentRaw > 0);

    const FEchoesBuildPlacementEvaluation Occupied =
        FEchoesBuildPlacementModel::Evaluate(
            *View,
            Worker,
            echoes::sim::EntityType::Barracks,
            echoes::sim::Vec2::FromTiles(8, 8));
    TestEqual(TEXT("Visible occupied ground previews as blocked"),
        Occupied.Validity, EEchoesBuildPreviewValidity::Occupied);

    const FEchoesBuildPlacementEvaluation Outside =
        FEchoesBuildPlacementModel::Evaluate(
            *View,
            Worker,
            echoes::sim::EntityType::Barracks,
            echoes::sim::Vec2::FromTiles(0, 0));
    TestEqual(TEXT("Out-of-map footprint previews as blocked"),
        Outside.Validity, EEchoesBuildPreviewValidity::OutsideMap);

    const FEchoesBuildPlacementEvaluation WrongActor =
        FEchoesBuildPlacementModel::Evaluate(
            *View,
            999999,
            echoes::sim::EntityType::Barracks,
            echoes::sim::Vec2::FromTiles(12, 8));
    TestEqual(TEXT("Missing worker cannot arm a placement"),
        WrongActor.Validity, EEchoesBuildPreviewValidity::InvalidWorker);
    TestTrue(TEXT("Every refusal includes stable player recovery text"),
        !FString(FEchoesBuildPlacementModel::Feedback(Occupied.Validity)).IsEmpty());
    return true;
}

#endif
