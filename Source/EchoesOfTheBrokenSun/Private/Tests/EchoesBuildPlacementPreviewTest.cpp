#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesBuildPlacementPreview.h"
#include "EchoesPowerNetworkView.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"
#include "EchoesPlacementDiagnostics.h"
#include "EchoesOfTheBrokenSun.h"
#include "Materials/MaterialInterface.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesBuildPlacementPreviewTest,
    "Echoes.Runtime.Input.BuildPlacementPreview",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesBuildPlacementPreviewTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/Art/Generated/Materials/M_EchoesPresentationVFX.M_EchoesPresentationVFX"));
    if (!TestNotNull(TEXT("Authored preview material loads"), Material)) return false;
    TestTrue(TEXT("Authored VFX material supports conduit instancing without default-material fallback"),
        Material->GetUsageByFlag(MATUSAGE_InstancedStaticMeshes));
    FLinearColor AuthoredTint;
    TestTrue(TEXT("Preview tint parameter exists in the actual material"),
        Material->GetVectorParameterValue(FMaterialParameterInfo(AEchoesBuildPlacementPreview::TintParameterName()), AuthoredTint));
    FTestWorldWrapper ConduitWorld;
    if (!ConduitWorld.CreateTestWorld(EWorldType::Game))
    {
        ConduitWorld.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the conduit presentation test world."));
        return false;
    }
    UWorld* World = ConduitWorld.GetTestWorld();
    if (!TestNotNull(TEXT("Conduit test world exists"), World)) return false;
    auto* Conduit = World->SpawnActor<AEchoesPowerNetworkView>();
    if (!TestNotNull(TEXT("Conduit actor spawns"), Conduit)) return false;
    FEchoesFieldHudView ConduitView;
    ConduitView.Authority = EEchoesFieldHudAuthority::LivePlayerView;
    ConduitView.Surface = EEchoesFieldHudSurface::Battlefield;
    ConduitView.NetworkConnections.Add({FVector(0, 0, 20), FVector(800, 0, 20)});
    Conduit->SetView(ConduitView);
    TArray<UInstancedStaticMeshComponent*> Parts;
    Conduit->GetComponents(Parts);
    if (!TestEqual(TEXT("Conduit has casing and energy components"), Parts.Num(), 2)) return false;
    for (auto* Part : Parts)
    {
        TestNotNull(TEXT("Conduit mesh prerequisite loads"), Part->GetStaticMesh().Get());
        TestEqual(TEXT("Conduit cannot intercept pointer or combat traces"), Part->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
        TestFalse(TEXT("Conduit cannot change navigation"), Part->CanEverAffectNavigation());
        TestFalse(TEXT("Conduit generates no overlap events"), Part->GetGenerateOverlapEvents());
        TestEqual(TEXT("One instance per scoped route"), Part->GetInstanceCount(), 1);
        FTransform Transform;
        TestTrue(TEXT("Conduit transform exists"), Part->GetInstanceTransform(0, Transform, true));
        TestTrue(TEXT("Conduit is centered on supplied route"), Transform.GetLocation().Equals(FVector(400, 0, 20)));
    }
    ConduitView.bReducedMotion = true;
    Conduit->SetView(ConduitView);
    TestFalse(TEXT("Reduced motion disables conduit animation"), Conduit->IsActorTickEnabled());
    ConduitView.bReducedMotion = false;
    Conduit->SetView(ConduitView);
    TestTrue(TEXT("Restoring motion reenables conduit animation"), Conduit->IsActorTickEnabled());
    ConduitView.Authority = EEchoesFieldHudAuthority::None;
    Conduit->SetView(ConduitView);
    TestTrue(TEXT("Unavailable authority hides stale network"), Conduit->IsHidden());
    TestFalse(TEXT("Unavailable authority disables conduit tick"), Conduit->IsActorTickEnabled());
    for (auto* Part : Parts)
        TestEqual(TEXT("Unavailable authority clears stale geometry"), Part->GetInstanceCount(), 0);
    Conduit->Destroy();
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
    const auto Core = Simulation.SpawnEntity(0, echoes::sim::Faction::MeridianCompact,
        echoes::sim::EntityType::CommandCore, echoes::sim::Vec2::FromTiles(4, 8));
    if (!TestTrue(TEXT("Placement network root exists"), Core != 0)) return false;
    Simulation.Step();
    auto ConnectedView = Simulation.CreatePlayerView(0);
    if (!TestTrue(TEXT("Placement network view exists"), ConnectedView.has_value())) return false;
    const auto Connected = FEchoesBuildPlacementModel::Evaluate(*ConnectedView, Worker,
        echoes::sim::EntityType::Dropoff, echoes::sim::Vec2::FromTiles(12, 8));
    TestTrue(TEXT("Valid site at network boundary connects after completion"),
        Connected.IsValid() && Connected.bWillConnect && Connected.ConnectionNodeId == Core);
    TestTrue(TEXT("Connection guidance remains explicit without color"),
        FEchoesBuildPlacementModel::Guidance(Connected).Contains(TEXT("connects when completed")));
    const auto Disconnected = FEchoesBuildPlacementModel::Evaluate(*ConnectedView, Worker,
        echoes::sim::EntityType::Dropoff, echoes::sim::Vec2::FromTiles(12, 9));
    TestTrue(TEXT("Disconnected placement stays legal without promising power"),
        Disconnected.IsValid() && !Disconnected.bWillConnect &&
        FEchoesBuildPlacementModel::Guidance(Disconnected).Contains(TEXT("outside the active network")));
    TestTrue(TEXT("Blocked site never claims a network connection"),
        !Occupied.bWillConnect && FEchoesBuildPlacementModel::Guidance(Occupied).Contains(TEXT("Cannot place here")));
    // Reproduce the diagnostic signature of a cursor crossing the range boundary
    // between preview and click. This is a synthetic reproduction, not attribution
    // of the owner's earlier placement, whose exact click was not recorded.
    const FString Before = FEchoesPlacementDiagnostics::Preview(*ConnectedView, Worker,
        echoes::sim::EntityType::Dropoff, echoes::sim::Vec2::FromTiles(12, 8),
        FVector2D(300, 200), FVector2D(1280, 720), FVector(0, -800, 0), Connected, true);
    const FString After = FEchoesPlacementDiagnostics::Preview(*ConnectedView, Worker,
        echoes::sim::EntityType::Dropoff, echoes::sim::Vec2::FromTiles(12, 9),
        FVector2D(300, 210), FVector2D(1280, 720), FVector(0, -600, 0), Disconnected, true);
    TestTrue(TEXT("Trace distinguishes preview and click connection state"),
        Before.Contains(TEXT("connects=1")) && After.Contains(TEXT("connects=0")) && Before != After);
    TestTrue(TEXT("Trace includes exact input and authoritative accounting context"),
        Before.Contains(TEXT("viewport=(1280,720)")) && Before.Contains(TEXT("raw=(")) &&
        Before.Contains(TEXT("matter=2000 dawn=2000")) && Before.Contains(TEXT("node=")));
    UE_LOG(LogEchoes, Display, TEXT("[ECHOES_PLACEMENT_TEST] case=boundary_crossing stage=previous %s"), *Before);
    UE_LOG(LogEchoes, Display, TEXT("[ECHOES_PLACEMENT_TEST] case=boundary_crossing stage=resolved %s"), *After);
    return true;
}

#endif
