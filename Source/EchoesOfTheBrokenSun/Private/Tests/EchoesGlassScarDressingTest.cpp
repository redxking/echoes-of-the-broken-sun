#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "EchoesGlassScarCompiledMapPack.h"
#include "EchoesGlassScarDressingPack.h"
#include "EchoesSimCore/Simulation.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesTerrainView.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

#include <optional>

/**
 * Glass Scar dressing consumer (map_dressing_v1 -> terrain view).
 *
 * Proves the four properties that make dressing safe to draw: the records are
 * bound to the exact compiled map pack the runtime carries; every record's
 * cell is Blocked in the live simulation (conformance re-derived, not
 * trusted); nothing draws on an unexplored tile for the scoped player; and
 * the layers touch no collision, overlap, shadow, decal, or navigation state.
 * It does not judge how the dressing looks - that is rendered review.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesGlassScarDressingTest,
    "Echoes.Runtime.Map.GlassScarDressing",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesGlassScarDressingTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    namespace dressing = echoes::world::glass_scar_dressing;
    namespace map_pack = echoes::world::glass_scar_pack;

    // Compile-time binding, restated as a runtime assertion so the report
    // names it.
    TestEqual(
        TEXT("Dressing records are bound to the runtime's compiled map pack"),
        FString(ANSI_TO_TCHAR(dressing::kBaseCompiledPackSha256)),
        FString(ANSI_TO_TCHAR(map_pack::kCompiledPackSha256)));
    TestEqual(
        TEXT("Dressing pack carries the authored record count"),
        dressing::kRecordCount,
        29);
    // Every emitted record stands on a compiled-pack blocked cell.
    int32 EmittedOnBlocked = 0;
    for (int32 Index = 0; Index < dressing::kRecordCount; ++Index)
    {
        const dressing::FDressingRecord& Record = dressing::kRecords[Index];
        const int32 CellIndex =
            static_cast<int32>(Record.Y) * map_pack::kGridWidthTiles +
            static_cast<int32>(Record.X);
        TestEqual(
            FString::Printf(
                TEXT("Record %s cell index agrees with its coordinates"),
                ANSI_TO_TCHAR(Record.Id)),
            Record.CellIndex,
            CellIndex);
        if ((map_pack::kMovementMask[CellIndex] & map_pack::kGroundMovementMask) == 0)
        {
            ++EmittedOnBlocked;
        }
    }
    TestEqual(
        TEXT("Every emitted record stands on a compiled-pack blocked cell"),
        EmittedOnBlocked,
        dressing::kRecordCount);

    FEchoesScopedTestSaveEnvironment TestSaveEnvironment(*this);
    if (!TestSaveEnvironment.IsReady())
    {
        return false;
    }
    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the Glass Scar dressing test world."));
        return false;
    }
    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge =
        World != nullptr
            ? World->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (!TestNotNull(TEXT("World owns the simulation subsystem"), Bridge) ||
        !TestTrue(
            TEXT("Glass Scar scenario starts"),
            Bridge != nullptr && Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    const echoes::sim::Simulation* Simulation = Bridge->GetSimulation();
    AEchoesTerrainView* TerrainView = Bridge->GetTerrainView();
    if (!TestNotNull(TEXT("Authoritative simulation exists"), Simulation) ||
        !TestNotNull(TEXT("Terrain view exists"), TerrainView))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    TestTrue(
        TEXT("Dressing layer is active on the Glass Scar preset"),
        TerrainView->IsDressingActive());
    TestEqual(
        TEXT("Terrain view consumed every dressing record"),
        TerrainView->GetDressingRecordCount(),
        dressing::kRecordCount);
    TestEqual(
        TEXT("Every record's cell is Blocked in the live simulation"),
        TerrainView->GetDressingPlacedCount(),
        dressing::kRecordCount);
    TestEqual(
        TEXT("No record was refused at runtime"),
        TerrainView->GetDressingRefusedCount(),
        0);
    TestTrue(
        TEXT("Dressing layers carry no collision, overlap, shadow, decal, or navigation influence"),
        TerrainView->AreDressingLayersPresentationOnly());

    // Fog gate: a record draws only where the local player's information
    // state is not Unexplored, and nothing draws on a cell the simulation
    // does not report Blocked or currently holds reshaped open.
    const std::optional<echoes::sim::PlayerId> ScopedPlayer =
        TerrainView->GetScopedPlayer();
    int32 ExpectedDrawn = 0;
    bool bEveryDrawnRecordIsAuthorized = true;
    bool bEveryAuthorizedRecordIsDrawn = true;
    for (int32 Index = 0; Index < dressing::kRecordCount; ++Index)
    {
        const dressing::FDressingRecord& Record = dressing::kRecords[Index];
        const int32 X = Record.X;
        const int32 Y = Record.Y;
        const bool bBlocked =
            Simulation->TerrainAt(X, Y) == echoes::sim::Terrain::Blocked;
        const bool bKnown =
            !ScopedPlayer.has_value() ||
            Simulation->VisibilityAt(
                *ScopedPlayer, echoes::sim::Vec2::FromTiles(X, Y)) !=
                echoes::sim::Visibility::Unexplored;
        const bool bOpen =
            bBlocked &&
            Simulation->IsPositionPassable(echoes::sim::Vec2::FromTiles(X, Y));
        const bool bAuthorized = bBlocked && bKnown && !bOpen;
        const bool bDrawn = TerrainView->IsDressingRecordInstanced(Index);
        ExpectedDrawn += bAuthorized ? 1 : 0;
        bEveryDrawnRecordIsAuthorized &= !bDrawn || bAuthorized;
        bEveryAuthorizedRecordIsDrawn &= !bAuthorized || bDrawn;
    }
    TestTrue(
        TEXT("No dressing record is drawn on an unexplored, non-blocked, or reshaped-open tile"),
        bEveryDrawnRecordIsAuthorized);
    TestTrue(
        TEXT("Every authorized dressing record is drawn"),
        bEveryAuthorizedRecordIsDrawn);
    TestEqual(
        TEXT("Instanced dressing count equals the authorized count"),
        TerrainView->GetDressingInstancedCount(),
        ExpectedDrawn);
    if (ScopedPlayer.has_value())
    {
        TestTrue(
            TEXT("The fog gate withholds at least one record at match start"),
            ExpectedDrawn < dressing::kRecordCount);
    }
    else
    {
        // Legacy full-disclosure path: the view was handed no player, so
        // every conformant record is drawn. The scoped-network path is where
        // the information-state gate is exercised for dressing.
        TestEqual(
            TEXT("Unscoped view draws every conformant record"),
            TerrainView->GetDressingInstancedCount(),
            TerrainView->GetDressingPlacedCount());
    }

    AddInfo(FString::Printf(
        TEXT("[ECHOES_DRESSING_TEST] records=%d placed=%d refused=%d instanced=%d packSha256=%s"),
        TerrainView->GetDressingRecordCount(),
        TerrainView->GetDressingPlacedCount(),
        TerrainView->GetDressingRefusedCount(),
        TerrainView->GetDressingInstancedCount(),
        ANSI_TO_TCHAR(dressing::kPackSha256)));

    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return true;
}

#endif
