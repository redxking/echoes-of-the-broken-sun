#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "EchoesCompiledMapBinding.h"
#include "EchoesCampaignTerrainBinding.h"
#include "EchoesGlassScarCompiledMapPack.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesSimCore/Simulation.h"
#include "Engine/World.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Tests/AutomationCommon.h"

namespace EchoesCompiledMapBindingTestInternal
{
namespace pack = echoes::world::glass_scar_pack;

// Compares one authoritative terrain grid against the compiled movement mask.
// The compiled contract binds ground passability; Terrain::Scarred, if it ever
// appears, still counts as passable ground for this parity purpose.
[[nodiscard]] int32 CountParityMismatches(
    const echoes::sim::Simulation& Simulation,
    int32& OutBlockedTiles)
{
    int32 Mismatches = 0;
    OutBlockedTiles = 0;
    for (int32 TileY = 0; TileY < pack::kGridHeightTiles; ++TileY)
    {
        for (int32 TileX = 0; TileX < pack::kGridWidthTiles; ++TileX)
        {
            const bool bRuntimeBlocked =
                Simulation.TerrainAt(TileX, TileY) ==
                echoes::sim::Terrain::Blocked;
            if (bRuntimeBlocked)
            {
                ++OutBlockedTiles;
            }
            if (bRuntimeBlocked ==
                echoes::world::IsCompiledGroundPassable(TileX, TileY))
            {
                ++Mismatches;
            }
        }
    }
    return Mismatches;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesCompiledMapBindingTest,
    "Echoes.Runtime.Map.CompiledMapBinding",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesCompiledMapBindingTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    using namespace EchoesCompiledMapBindingTestInternal;
    namespace sim = echoes::sim;
    namespace world = echoes::world;

    FEchoesScopedTestSaveEnvironment TestSaveEnvironment(*this);
    if (!TestSaveEnvironment.IsReady())
    {
        return false;
    }

    // 1. Structural self-consistency of the generated constant tables.
    const world::CompiledMapCheckResult Tables =
        world::VerifyCompiledGlassScarTables();
    TestTrue(
        FString::Printf(
            TEXT("Compiled tables verify structurally (%s)"),
            ANSI_TO_TCHAR(Tables.detail)),
        Tables.ok);

    // 2. Landmark contract values.
    TestTrue(TEXT("Future Well primary tile (32,32) is passable"),
             world::IsCompiledGroundPassable(32, 32));
    TestFalse(TEXT("Scar interior tile (10,32) is blocked"),
              world::IsCompiledGroundPassable(10, 32));
    TestTrue(TEXT("West edge corridor tile (0,32) is passable"),
             world::IsCompiledGroundPassable(0, 32));
    TestTrue(TEXT("Eastern crossing tile (50,32) is passable"),
             world::IsCompiledGroundPassable(50, 32));
    TestFalse(TEXT("Out-of-range tiles are impassable"),
              world::IsCompiledGroundPassable(-1, 0) ||
                  world::IsCompiledGroundPassable(64, 0));
    const world::CompiledTile Primary = world::FutureWellPrimaryTile();
    TestTrue(TEXT("Future Well primary tile matches the contract (32,32)"),
             Primary.X == 32 && Primary.Y == 32);
    TestEqual(TEXT("Future Well fallback count is 4"),
              world::FutureWellFallbackCount(), 4);
    const world::CompiledTile FirstFallback = world::FutureWellFallbackTile(0);
    TestTrue(TEXT("First ordered fallback tile is (31,32)"),
             FirstFallback.X == 31 && FirstFallback.Y == 32);
    const world::CompiledTile InvalidFallback = world::FutureWellFallbackTile(4);
    TestTrue(TEXT("Out-of-range fallback ordinal returns the sentinel tile"),
             InvalidFallback.X == -1 && InvalidFallback.Y == -1);
    const world::CompiledCameraBounds Camera = world::CameraTileBounds();
    TestTrue(TEXT("Camera bounds are the half-open [0,64) x [0,64) contract"),
             Camera.MinX == 0 && Camera.MinY == 0 &&
                 Camera.MaxXExclusive == 64 && Camera.MaxYExclusive == 64);

    // 3. Binding parity on a fresh simulation: applying the compiled mask
    // reproduces the exact blocked census on an otherwise open grid.
    {
        sim::SimulationConfig Config;
        Config.rules = sim::DefaultSimulationRules();
        sim::Simulation Simulation(Config);
        const int32 Applied = world::ApplyCompiledGlassScar(Simulation);
        TestEqual(TEXT("Binding applies exactly the contract blocked census"),
                  Applied, pack::kExpectedBlockedCellCount);
        int32 BlockedTiles = 0;
        const int32 Mismatches = CountParityMismatches(Simulation, BlockedTiles);
        TestEqual(TEXT("Fresh-simulation terrain matches the compiled mask on every tile"),
                  Mismatches, 0);
        TestEqual(TEXT("Fresh-simulation blocked census is exact"),
                  BlockedTiles, pack::kExpectedBlockedCellCount);
        TestTrue(TEXT("Future Well primary position is passable after binding"),
                 Simulation.IsPositionPassable(
                     sim::Vec2::FromTiles(Primary.X, Primary.Y)));
    }

    // 4. Parity against the live skirmish author. StartPrototypeScenario runs
    // ConfigureSkirmishTerrain with the Glass Scar preset, not
    // ConfigureGlassScar: SelectedOperation defaults to Skirmish and neither
    // the stress nor a controlled-presentation flag is set here, so the
    // subsystem takes its bConfiguredSkirmish branch. The preset geometry is
    // bound to the frozen skirmish descriptor; section 4b covers the distinct
    // campaign contract.
    {
        FTestWorldWrapper WorldWrapper;
        if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
        {
            WorldWrapper.ForwardErrorMessages(this);
            AddError(TEXT("Could not create the compiled-map parity test world."));
            return false;
        }
        UWorld* World = WorldWrapper.GetTestWorld();
        UEchoesSimulationSubsystem* Bridge =
            World != nullptr
                ? World->GetSubsystem<UEchoesSimulationSubsystem>()
                : nullptr;
        if (!TestNotNull(TEXT("Parity world owns the simulation subsystem"), Bridge) ||
            !TestTrue(TEXT("Prototype Glass Scar scenario starts"),
                      Bridge != nullptr && Bridge->StartPrototypeScenario()))
        {
            WorldWrapper.ForwardErrorMessages(this);
            return false;
        }
        const sim::Simulation* Simulation = Bridge->GetSimulation();
        if (TestNotNull(TEXT("Authoritative simulation exists"), Simulation))
        {
            int32 BlockedTiles = 0;
            const int32 Mismatches =
                CountParityMismatches(*Simulation, BlockedTiles);
            TestEqual(
                TEXT("Live skirmish-preset terrain matches the compiled mask on every tile"),
                Mismatches, 0);
            TestEqual(TEXT("Live skirmish-preset blocked census matches the contract"),
                      BlockedTiles, pack::kExpectedBlockedCellCount);
        }
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
    }

    // 4b. Campaign M01 now has its own authored evacuation-margin contract.
    // Skirmish parity above still uses the frozen Glass Scar descriptor.
    {
        FTestWorldWrapper WorldWrapper;
        if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
        {
            WorldWrapper.ForwardErrorMessages(this);
            AddError(TEXT("Could not create the campaign parity test world."));
            return false;
        }
        UWorld* World = WorldWrapper.GetTestWorld();
        UEchoesSimulationSubsystem* Bridge =
            World != nullptr
                ? World->GetSubsystem<UEchoesSimulationSubsystem>()
                : nullptr;
        FString Feedback;
        if (!TestNotNull(TEXT("Campaign parity world owns the simulation subsystem"), Bridge) ||
            !TestTrue(TEXT("Campaign prologue operation can be selected"),
                      Bridge != nullptr &&
                          Bridge->SelectOperationMode(
                              EEchoesOperationMode::CampaignPrologue, Feedback)) ||
            !TestTrue(TEXT("Campaign prologue scenario starts"),
                      Bridge->StartPrototypeScenario()))
        {
            WorldWrapper.ForwardErrorMessages(this);
            return false;
        }
        const sim::Simulation* Simulation = Bridge->GetSimulation();
        if (TestNotNull(TEXT("Authoritative campaign simulation exists"), Simulation))
        {
            const auto Contract = world::CheckCampaignTerrain(1, sim::FutureWellChoice::Preserve);
            TestTrue(TEXT("M01 source contract validates"), Contract.ok);
            int32 BlockedTiles = 0, Mismatches = 0;
            for (int32 Y = 0; Y < 64; ++Y)
                for (int32 X = 0; X < 64; ++X)
                {
                    const bool bOpen = Simulation->TerrainAt(X,Y) != sim::Terrain::Blocked;
                    BlockedTiles += !bOpen;
                    Mismatches += bOpen != world::IsCampaignTerrainPassable(1, sim::FutureWellChoice::Preserve, X,Y);
                }
            TestEqual(TEXT("Live M01 terrain matches its own compiled map"), Mismatches, 0);
            TestEqual(TEXT("Live M01 census matches its contract"), BlockedTiles, Contract.blocked_cells);
        }
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
    }

    // 5. Provenance: the header digest constant equals the checked-in sidecar.
    {
        const FString SidecarPath = FPaths::Combine(
            FPaths::ProjectContentDir(),
            TEXT("World/Generated/GlassScar/glass_scar_compiled_map_pack_v1.sha256"));
        FString SidecarText;
        if (TestTrue(TEXT("Compiled pack digest sidecar loads"),
                     FFileHelper::LoadFileToString(SidecarText, *SidecarPath)))
        {
            TestEqual(
                TEXT("Header digest constant equals the sidecar digest"),
                SidecarText.TrimStartAndEnd(),
                FString(ANSI_TO_TCHAR(world::CompiledPackSha256())));
        }
    }

    return true;
}

#endif
