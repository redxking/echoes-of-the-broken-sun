#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"
#include "EchoesLumeReachDressingPack.h"
#include "EchoesSimCore/Simulation.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesTerrainView.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

#include <optional>

/**
 * Lume Reach dressing consumer (map_dressing_v1 -> terrain view).
 *
 * Verifies:
 * 1. Digest binding against overlay map pack.
 * 2. Authored record count (39 records).
 * 3. In-engine activation under Lume Reach operation profile.
 * 4. Zero runtime refusals on conformant terrain.
 * 5. Presentation-only layers (NoCollision, no overlap, no shadows, no nav influence).
 * 6. Auto-detection of Lume Reach gate topology.
 * 7. Fog and Exploration gating: unexplored tiles instance zero dressing records.
 * 8. Live cell passability refusal.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesLumeReachDressingTest,
    "Echoes.Runtime.Map.LumeReachDressing",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesLumeReachDressingTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    namespace lr_dressing = echoes::world::lume_reach_dressing;

    TestEqual(
        TEXT("Lume Reach dressing pack carries 39 records"),
        lr_dressing::kRecordCount,
        39);

    // Verify sidecar SHA is 64 lowercase hex characters
    const FString BasePackSha = ANSI_TO_TCHAR(lr_dressing::kBaseCompiledPackSha256);
    TestEqual(TEXT("Base compiled pack SHA-256 length is 64"), BasePackSha.Len(), 64);

    echoes::sim::SimulationConfig SimConfig{64, 64, 20, 0x5EEDULL};
    echoes::sim::Simulation StandaloneSim(SimConfig);

    // Populate the 39 authored blocked cells on StandaloneSim
    for (int32 Index = 0; Index < lr_dressing::kRecordCount; ++Index)
    {
        const lr_dressing::FDressingRecord& Record = lr_dressing::kRecords[Index];
        StandaloneSim.SetTerrainTile(Record.X, Record.Y, echoes::sim::Terrain::Blocked);
    }

    FEchoesScopedTestSaveEnvironment TestSaveEnvironment(*this);
    if (!TestSaveEnvironment.IsReady())
    {
        return false;
    }
    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create Lume Reach dressing test world."));
        return false;
    }
    UWorld* World = WorldWrapper.GetTestWorld();
    FActorSpawnParameters SpawnParameters;
    SpawnParameters.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AEchoesTerrainView* TerrainView = World->SpawnActor<AEchoesTerrainView>(
        AEchoesTerrainView::StaticClass(),
        FVector::ZeroVector,
        FRotator::ZeroRotator,
        SpawnParameters);
    if (!TestNotNull(TEXT("Terrain view spawned"), TerrainView))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    // 1. Historical overlay fixture, explicitly separate from current M10.
    TerrainView->UseDiagnosticDressingTopologyForTesting();
    const bool bInitOk = TerrainView->InitializeTerrain(
        StandaloneSim,
        200.0f,
        EEchoesSkirmishMapPreset::GlassScar,
        std::nullopt,
        EEchoesOperationMode::CampaignChoirAtLumeReach);

    TestTrue(TEXT("InitializeTerrain succeeds for Lume Reach operation"), bInitOk);
    TestTrue(TEXT("Dressing is active on Lume Reach"), TerrainView->IsDressingActive());
    TestEqual(
        TEXT("Active dressing site is lume-reach"),
        FString(TerrainView->GetActiveDressingSiteId()),
        TEXT("lume-reach"));
    TestEqual(
        TEXT("Dressing record count is 39"),
        TerrainView->GetDressingRecordCount(),
        39);
    TestEqual(
        TEXT("All 39 records are placed on blocked cells"),
        TerrainView->GetDressingPlacedCount(),
        39);
    TestEqual(
        TEXT("Zero records refused at runtime"),
        TerrainView->GetDressingRefusedCount(),
        0);
    TestEqual(
        TEXT("All 39 records are instanced"),
        TerrainView->GetDressingInstancedCount(),
        39);
    TestTrue(
        TEXT("Dressing layers are presentation-only"),
        TerrainView->AreDressingLayersPresentationOnly());

    // 2. Topology auto-detection test (no OperationMode passed)
    AEchoesTerrainView* AutoDetectTerrainView = World->SpawnActor<AEchoesTerrainView>(
        AEchoesTerrainView::StaticClass(),
        FVector::ZeroVector,
        FRotator::ZeroRotator,
        SpawnParameters);
    if (TestNotNull(TEXT("Auto-detect terrain view spawned"), AutoDetectTerrainView))
    {
        AutoDetectTerrainView->UseDiagnosticDressingTopologyForTesting();
        const bool bAutoInitOk = AutoDetectTerrainView->InitializeTerrain(
            StandaloneSim,
            200.0f,
            EEchoesSkirmishMapPreset::GlassScar,
            std::nullopt,
            std::nullopt);
        TestTrue(TEXT("Auto-detect InitializeTerrain succeeds"), bAutoInitOk);
        TestTrue(TEXT("Auto-detected dressing is active"), AutoDetectTerrainView->IsDressingActive());
        TestEqual(
            TEXT("Auto-detected site is lume-reach"),
            FString(AutoDetectTerrainView->GetActiveDressingSiteId()),
            TEXT("lume-reach"));
        AutoDetectTerrainView->Destroy();
    }

    // 3. Fog and Exploration gating
    const echoes::sim::PlayerId TestPlayer = 1;
    StandaloneSim.AddPlayer(TestPlayer, echoes::sim::Faction::KharuunAssemblies, echoes::sim::ResourcePool{});
    AEchoesTerrainView* ScopedTerrainView = World->SpawnActor<AEchoesTerrainView>(
        AEchoesTerrainView::StaticClass(),
        FVector::ZeroVector,
        FRotator::ZeroRotator,
        SpawnParameters);
    if (TestNotNull(TEXT("Scoped terrain view spawned"), ScopedTerrainView))
    {
        const bool bScopedInitOk = ScopedTerrainView->InitializeTerrain(
            StandaloneSim,
            200.0f,
            EEchoesSkirmishMapPreset::GlassScar,
            TestPlayer,
            EEchoesOperationMode::CampaignChoirAtLumeReach);
        TestTrue(TEXT("Scoped InitializeTerrain succeeds"), bScopedInitOk);
        TestFalse(TEXT("Legacy overlay dressing cannot bind current M10 through unexplored sentinels"),
            ScopedTerrainView->IsDressingActive());
        TestEqual(TEXT("Current M10 topology rejects the 34 incompatible legacy records before reveal"),
            ScopedTerrainView->GetDressingRefusedCount(), 34);
        // Fresh simulation without scouts has all tiles Unexplored for TestPlayer
        TestEqual(
            TEXT("Unexplored tiles instance zero dressing records"),
            ScopedTerrainView->GetDressingInstancedCount(),
            0);
        TestTrue(
            TEXT("Scoped dressing layers remain presentation-only"),
            ScopedTerrainView->AreDressingLayersPresentationOnly());
        ScopedTerrainView->Destroy();
    }

    // 4. Live cell passability refusal
    // Modify one record cell to Open in simulation
    const lr_dressing::FDressingRecord& RefusedRecord = lr_dressing::kRecords[0];
    StandaloneSim.SetTerrainTile(RefusedRecord.X, RefusedRecord.Y, echoes::sim::Terrain::Open);
    AddExpectedError(
        TEXT("ECHOES_DRESSING_REFUSED"),
        EAutomationExpectedErrorFlags::Contains,
        1);
    TerrainView->SyncTerrain(StandaloneSim);
    TestEqual(
        TEXT("Refused record count is 1 after cell unblocked"),
        TerrainView->GetDressingRefusedCount(),
        1);
    TestEqual(
        TEXT("Placed record count drops to 38"),
        TerrainView->GetDressingPlacedCount(),
        38);
    TestFalse(
        TEXT("Refused record is no longer instanced"),
        TerrainView->IsDressingRecordInstanced(0));

    TerrainView->Destroy();
    WorldWrapper.ForwardErrorMessages(this);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
