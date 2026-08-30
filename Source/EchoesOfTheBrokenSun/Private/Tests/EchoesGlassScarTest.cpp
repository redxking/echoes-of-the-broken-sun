#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesSimCore/Simulation.h"
#include "EchoesEntityView.h"
#include "EchoesGameMode.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesTerrainView.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInterface.h"
#include "PhysicsEngine/BodySetup.h"
#include "Tests/AutomationCommon.h"

#include <algorithm>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesGlassScarTest,
    "Echoes.Runtime.Map.GlassScar",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesGlassScarTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the Glass Scar test world."));
        return false;
    }

    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge =
        World != nullptr
            ? World->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (!TestNotNull(TEXT("Glass Scar world owns the simulation subsystem"), Bridge) ||
        !TestTrue(TEXT("Glass Scar scenario starts"),
                  Bridge != nullptr && Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const echoes::sim::Simulation* Simulation = Bridge->GetSimulation();
    AEchoesTerrainView* TerrainView = Bridge->GetTerrainView();
    if (!TestNotNull(TEXT("Authoritative Glass Scar simulation exists"), Simulation) ||
        !TestNotNull(TEXT("Glass Scar presentation exists"), TerrainView))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    TestTrue(
        TEXT("Blocked and scarred terrain use project-authored world meshes"),
        TerrainView->IsUsingAuthoredTerrainMeshes());

    const TCHAR* AuthoredWorldMeshes[] = {
        TEXT("/Game/Art/Generated/World/Environment/SM_World_GlassScarShelf.SM_World_GlassScarShelf"),
        TEXT("/Game/Art/Generated/World/Environment/SM_World_GlassScarRidge.SM_World_GlassScarRidge"),
        TEXT("/Game/Art/Generated/World/Environment/SM_World_GlassScarShard.SM_World_GlassScarShard"),
        TEXT("/Game/Art/Generated/World/Environment/SM_World_GlassScarAshCut.SM_World_GlassScarAshCut"),
        TEXT("/Game/Art/Generated/World/Environment/SM_World_GlassScarBuriedCauseway.SM_World_GlassScarBuriedCauseway"),
        TEXT("/Game/Art/Generated/World/Environment/SM_World_GlassScarFoldedVerge.SM_World_GlassScarFoldedVerge"),
        TEXT("/Game/Art/Generated/World/Resources/SM_World_MatterDeposit.SM_World_MatterDeposit")};
    for (const TCHAR* MeshPath : AuthoredWorldMeshes)
    {
        UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, MeshPath);
        TestNotNull(
            FString::Printf(TEXT("Authored Glass Scar mesh loads: %s"), MeshPath),
            Mesh);
        if (Mesh != nullptr)
        {
            TestTrue(
                FString::Printf(TEXT("Authored Glass Scar mesh has two LODs: %s"), MeshPath),
                Mesh->GetNumLODs() >= 2);
            TestTrue(
                FString::Printf(TEXT("Authored Glass Scar mesh has four material zones: %s"), MeshPath),
                Mesh->GetStaticMaterials().Num() >= 4);
        }
    }

    UStaticMesh* AshCutMesh = LoadObject<UStaticMesh>(
        nullptr,
        TEXT("/Game/Art/Generated/World/Environment/SM_World_GlassScarAshCut.SM_World_GlassScarAshCut"));
    if (TestNotNull(TEXT("Production-oriented Ash Cut mesh loads"), AshCutMesh))
    {
        TestTrue(TEXT("Ash Cut LOD0 has a surface and lightmap UV channel"),
                 AshCutMesh->GetNumUVChannels(0) >= 2);
        TestTrue(TEXT("Ash Cut LOD1 has a surface and lightmap UV channel"),
                 AshCutMesh->GetNumUVChannels(1) >= 2);
        const UBodySetup* BodySetup = AshCutMesh->GetBodySetup();
        TestNotNull(TEXT("Ash Cut owns authored collision data"), BodySetup);
        if (BodySetup != nullptr)
        {
            TestTrue(TEXT("Ash Cut has at least one simple collision primitive"),
                     BodySetup->AggGeom.GetElementCount() > 0);
            TestEqual(TEXT("Ash Cut asset uses simple-and-complex collision policy"),
                      BodySetup->GetCollisionTraceFlag(),
                      ECollisionTraceFlag::CTF_UseSimpleAndComplex);
        }

        const TCHAR* ExpectedMaterials[] = {
            TEXT("/Game/Art/Generated/Materials/MI_GlassScarAshCut_Basalt.MI_GlassScarAshCut_Basalt"),
            TEXT("/Game/Art/Generated/Materials/MI_GlassScarAshCut_Ash.MI_GlassScarAshCut_Ash"),
            TEXT("/Game/Art/Generated/Materials/MI_GlassScarAshCut_Glass.MI_GlassScarAshCut_Glass"),
            TEXT("/Game/Art/Generated/Materials/MI_GlassScarAshCut_Vein.MI_GlassScarAshCut_Vein")};
        for (int32 MaterialIndex = 0;
             MaterialIndex < UE_ARRAY_COUNT(ExpectedMaterials);
             ++MaterialIndex)
        {
            const UMaterialInterface* Material = AshCutMesh->GetMaterial(MaterialIndex);
            TestNotNull(
                FString::Printf(TEXT("Ash Cut material zone %d loads"), MaterialIndex),
                Material);
            if (Material != nullptr)
            {
                TestEqual(
                    FString::Printf(TEXT("Ash Cut material zone %d is route-specific"), MaterialIndex),
                    Material->GetPathName(),
                    FString(ExpectedMaterials[MaterialIndex]));
            }
        }
    }

    AEchoesGameMode* PresentationGameMode = World->SpawnActor<AEchoesGameMode>();
    if (TestNotNull(TEXT("Glass Scar presentation GameMode can be created"), PresentationGameMode) &&
        TestTrue(TEXT("Glass Scar production environment can be spawned"),
                 PresentationGameMode != nullptr &&
                     PresentationGameMode->SpawnPrototypeEnvironmentForTesting()))
    {
        AStaticMeshActor* AshCutActor = nullptr;
        for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
        {
            if (It->ActorHasTag(TEXT("EchoesRouteAshCut")))
            {
                AshCutActor = *It;
                break;
            }
        }
        if (TestNotNull(TEXT("Runtime Ash Cut route actor exists"), AshCutActor))
        {
            UStaticMeshComponent* AshCutComponent =
                AshCutActor->GetStaticMeshComponent();
            if (TestNotNull(TEXT("Runtime Ash Cut route has a mesh component"), AshCutComponent))
            {
                TestEqual(TEXT("Runtime Ash Cut collision remains disabled"),
                          AshCutComponent->GetCollisionEnabled(),
                          ECollisionEnabled::NoCollision);
                TestTrue(TEXT("Runtime Ash Cut does not affect navigation"),
                         !AshCutComponent->CanEverAffectNavigation());
                TestTrue(TEXT("Runtime Ash Cut retains its route material family"),
                         AshCutComponent->GetMaterial(0) != nullptr &&
                             AshCutComponent->GetMaterial(0)->GetPathName().Contains(
                                 TEXT("MI_GlassScarAshCut_Basalt")));
            }
        }
    }

    AEchoesEntityView* MatterPreview = World->SpawnActor<AEchoesEntityView>();
    if (TestNotNull(TEXT("Matter presentation view can be created"), MatterPreview))
    {
        echoes::sim::Entity MatterState{};
        MatterState.id = 920001;
        MatterState.owner = echoes::sim::kNeutralPlayer;
        MatterState.type = echoes::sim::EntityType::ResourceNode;
        MatterState.position = echoes::sim::Vec2::FromTiles(12, 12);
        MatterState.hitPoints = 1;
        MatterState.maxHitPoints = 1;
        MatterPreview->ApplyAuthoritativeState(MatterState, true);
        TestTrue(
            TEXT("Matter presentation uses the project-authored deposit mesh"),
            MatterPreview->IsUsingAuthoredResourceMesh());
        MatterPreview->Destroy();
    }

    int32 AuthoritativeBlockedTiles = 0;
    for (int32 TileY = 0; TileY < Simulation->Config().mapHeightTiles; ++TileY)
    {
        for (int32 TileX = 0; TileX < Simulation->Config().mapWidthTiles; ++TileX)
        {
            AuthoritativeBlockedTiles +=
                Simulation->TerrainAt(TileX, TileY) == echoes::sim::Terrain::Blocked
                    ? 1
                    : 0;
        }
    }
    TestEqual(TEXT("Glass Scar has the authored blocked-tile count"),
              AuthoritativeBlockedTiles,
              165);
    TestEqual(TEXT("Presentation mirrors every blocked Glass Scar tile"),
              TerrainView->GetBlockedTileCount(),
              AuthoritativeBlockedTiles);
    TestEqual(TEXT("Glass Scar begins without harvested terrain scars"),
              TerrainView->GetScarredTileCount(),
              0);

    for (const int32 CrossingX : {13, 32, 49})
    {
        TestTrue(
            *FString::Printf(TEXT("Crossing at x=%d remains passable"), CrossingX),
            Simulation->IsPositionPassable(
                echoes::sim::Vec2::FromTiles(CrossingX, 32)));
    }
    for (const int32 BarrierX : {8, 20, 40, 55})
    {
        TestFalse(
            *FString::Printf(TEXT("Fractured span at x=%d remains blocked"), BarrierX),
            Simulation->IsPositionPassable(
                echoes::sim::Vec2::FromTiles(BarrierX, 32)));
    }
    TestTrue(
        TEXT("The Future Well occupies the open central crossing"),
        Simulation->Entities().end() != std::find_if(
            Simulation->Entities().begin(),
            Simulation->Entities().end(),
            [](const echoes::sim::Entity& Entity)
            {
                return Entity.type == echoes::sim::EntityType::FutureWell &&
                       Entity.position == echoes::sim::Vec2::FromTiles(32, 32);
            }));

    echoes::sim::EntityId RouteScout = 0;
    for (const echoes::sim::Entity& Entity : Simulation->Entities())
    {
        if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            Entity.type == echoes::sim::EntityType::Soldier &&
            Entity.position == echoes::sim::Vec2::FromTiles(16, 10))
        {
            RouteScout = Entity.id;
            break;
        }
    }
    if (!TestTrue(TEXT("Glass Scar route scout exists"), RouteScout != 0))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const echoes::sim::Vec2 NorthernBasin =
        echoes::sim::Vec2::FromTiles(20, 38);
    FString Feedback;
    if (!TestTrue(
            TEXT("Route scout accepts a move across the fractured span"),
            Bridge->IssueCommand(
                echoes::sim::CommandType::Move,
                RouteScout,
                0,
                Bridge->SimToWorld(NorthernBasin),
                echoes::sim::FutureWellChoice::Dormant,
                Feedback)))
    {
        AddInfo(FString::Printf(TEXT("Route command feedback: %s"), *Feedback));
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    bool bCrossedSpan = false;
    bool bReachedNorthernBasin = false;
    for (int32 TickIndex = 0; TickIndex < 800; ++TickIndex)
    {
        Bridge->Tick(0.05f);
        const echoes::sim::Entity* Scout = Bridge->FindEntity(RouteScout);
        if (Scout == nullptr)
        {
            break;
        }
        bCrossedSpan |= Scout->position.y.FloorToInt() > 34;
        if (Scout->position == NorthernBasin)
        {
            bReachedNorthernBasin = true;
            break;
        }
    }
    TestTrue(TEXT("Deterministic routing crosses the Glass Scar"), bCrossedSpan);
    TestTrue(TEXT("Deterministic routing reaches the northern basin"),
             bReachedNorthernBasin);

    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors() && !WorldWrapper.HasFailed();
}

#endif
