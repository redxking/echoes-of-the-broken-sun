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

    // SPEC-SIM-003 and SPEC-UI-008.F13: the preview may not promise a placement the
    // authority will refuse. It used to, around every ore patch on every map.
    //
    // Simulation::FootprintHalfExtentFor gives ResourceNode kFixedScale/3 and FutureWell
    // kFixedScale/2, but the preview read the archetype table for every type. Neither of
    // those two is in kConfigurableEntityTypes, so nothing writes their rows -- while
    // kConfigurableEntityTypeCount is larger than that list, so their indices passed the
    // bounds check and returned the struct default kFixedScale/8. The preview therefore
    // sized an ore patch at 128 where the authority uses 341, leaving a band that
    // previewed valid, accepted the click, queued the order and then reported only
    // "Order had no effect" with no cause and no resources spent.
    //
    // This asserts agreement rather than a single hand-picked tile, so it also covers the
    // Well case and any future divergence in either direction.
    // Sited beside the worker's own build area, not out in the dark. The preview
    // additionally requires every footprint tile to be Visibility::Visible, which the
    // authority neither has nor should have -- so a node in fog produces a wall of
    // UnknownTerrain refusals that compare against an omniscient Valid and prove
    // nothing about footprints. The first version of this sweep did exactly that.
    const auto Node = Simulation.SpawnResourceNode(
        echoes::sim::Vec2::FromTiles(13, 8), 1500);
    if (TestTrue(TEXT("Resource node spawns for the footprint agreement sweep"), Node != 0))
    {
        Simulation.Step();
        auto SweepView = Simulation.CreatePlayerView(0);
        if (TestTrue(TEXT("Footprint agreement sweep view exists"), SweepView.has_value()))
        {
            int32 Compared = 0;
            int32 Disagreements = 0;
            int32 PreviewPromisedRefusal = 0;
            int32 FogScoped = 0;
            int32 AuthorityAccepted = 0;
            int32 AuthorityRefused = 0;
            // Quarter-tile steps across the contested band on both axes. The old preview
            // and the authority differ by 213 raw units around a node, which is a fifth of
            // a tile, so a coarser sweep would step straight over the defect.
            for (int32 StepX = -8; StepX <= 8; ++StepX)
            {
                for (int32 StepY = -8; StepY <= 8; ++StepY)
                {
                    const echoes::sim::Vec2 Candidate{
                        echoes::sim::Fixed::FromRaw(
                            echoes::sim::Vec2::FromTiles(13, 8).x.Raw() +
                            StepX * (echoes::sim::kFixedScale / 4)),
                        echoes::sim::Fixed::FromRaw(
                            echoes::sim::Vec2::FromTiles(13, 8).y.Raw() +
                            StepY * (echoes::sim::kFixedScale / 4))};
                    const auto Previewed = FEchoesBuildPlacementModel::Evaluate(
                        *SweepView, Worker, echoes::sim::EntityType::Dropoff, Candidate);
                    const bool bAuthorityAccepts =
                        Simulation.ValidatePlacement(
                            0, echoes::sim::EntityType::Dropoff, Candidate) ==
                        echoes::sim::PlacementResult::Valid;
                    // Range and resources are the preview's own concerns and are not part
                    // of the authority's placement verdict, so only compare where the
                    // preview's refusal is a footprint or terrain claim.
                    // Compare only where both sides are answering the same question.
                    // UnknownTerrain is a fog refusal the authority has no counterpart
                    // for -- it sees the whole map by definition -- and resources and
                    // worker range are the preview's own concerns. Footprint, terrain
                    // and map bounds are the shared ground.
                    if (Previewed.Validity == EEchoesBuildPreviewValidity::UnknownTerrain)
                    {
                        ++FogScoped;
                        continue;
                    }
                    if (!Previewed.IsValid() &&
                        Previewed.Validity != EEchoesBuildPreviewValidity::Occupied &&
                        Previewed.Validity != EEchoesBuildPreviewValidity::TerrainBlocked &&
                        Previewed.Validity != EEchoesBuildPreviewValidity::OutsideMap)
                    {
                        continue;
                    }
                    ++Compared;
                    if (bAuthorityAccepts) { ++AuthorityAccepted; } else { ++AuthorityRefused; }
                    if (Previewed.IsValid() != bAuthorityAccepts)
                    {
                        ++Disagreements;
                        if (Previewed.IsValid())
                        {
                            ++PreviewPromisedRefusal;
                        }
                    }
                }
            }
            // Non-vacuity, stated as a property rather than a count. A raw threshold is one
            // map or vision change away from silently passing on nothing -- an earlier version
            // of this sweep compared 200+ positions and every one of them was fog. What makes
            // the comparison meaningful is that it STRADDLES the authority's boundary: some
            // positions it accepts, some it refuses. If either side is empty the sweep sits
            // entirely inside or entirely outside the footprint and proves nothing, whatever
            // the count says.
            TestTrue(TEXT("Footprint sweep straddles the authority's boundary rather than sitting on one side"),
                AuthorityAccepted > 0 && AuthorityRefused > 0 && Compared > 50);
            TestEqual(
                TEXT("The preview never promises a placement the authority refuses"),
                PreviewPromisedRefusal, 0);
            TestEqual(
                TEXT("Preview and authority agree on every footprint verdict near a resource node"),
                Disagreements, 0);
            UE_LOG(LogEchoes, Display,
                TEXT("[ECHOES_PLACEMENT_TEST] case=footprint_agreement compared=%d disagreements=%d falseValid=%d fogScoped=%d authorityAccepted=%d authorityRefused=%d"),
                Compared, Disagreements, PreviewPromisedRefusal, FogScoped,
                AuthorityAccepted, AuthorityRefused);
        }
    }
    return true;
}

#endif
