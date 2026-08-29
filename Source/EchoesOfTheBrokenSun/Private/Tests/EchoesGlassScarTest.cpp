#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesSimCore/Simulation.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesTerrainView.h"
#include "Engine/World.h"
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
