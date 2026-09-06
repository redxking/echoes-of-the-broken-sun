#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesSimCore/Simulation.h"
#include "EchoesSkirmishSetup.h"
#include "EchoesGlassScarDressingPack.h"
#include "EchoesTerrainView.h"
#include "EchoesTestSaveEnvironment.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

#include <optional>
#include <string>
#include <vector>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesScopedTerrainMemoryTest,
    "Echoes.Runtime.Replay.ScopedTerrainMemory",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesScopedTerrainMemoryTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    using namespace echoes::sim;

    FEchoesScopedTestSaveEnvironment TestSaveEnvironment(*this);
    if (!TestSaveEnvironment.IsReady())
    {
        return false;
    }

    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the scoped-terrain memory test world."));
        return false;
    }

    constexpr PlayerId LocalPlayer = 0;
    constexpr int32 RememberedTileX = 19;
    constexpr int32 RememberedTileY = 30;
    const Vec2 RememberedPosition =
        Vec2::FromTiles(RememberedTileX, RememberedTileY);
    const Vec2 ScoutStart = Vec2::FromTiles(19, 27);
    const Vec2 ScoutAway = Vec2::FromTiles(19, 10);

    Simulation SimulationValue(SimulationConfig{64, 64, 20, 0x51C0A7ULL});
    bool bCanonicalTerrainApplied = true;
    for (int32 TileY = 0; TileY < 64; ++TileY)
    {
        for (int32 TileX = 0; TileX < 64; ++TileX)
        {
            if (FEchoesSkirmishSetupModel::IsBlockedTile(
                    EEchoesSkirmishMapPreset::GlassScar,
                    TileX,
                    TileY))
            {
                bCanonicalTerrainApplied &= SimulationValue.SetTerrainTile(
                    TileX,
                    TileY,
                    Terrain::Blocked);
            }
        }
    }
    const int32 CanonicalBlockedCount =
        FEchoesSkirmishSetupModel::ExpectedBlockedTileCount(
            EEchoesSkirmishMapPreset::GlassScar);
    if (!TestTrue(
            TEXT("Canonical Glass Scar terrain applies to the fixture"),
            bCanonicalTerrainApplied) ||
        !TestTrue(
            TEXT("Local Meridian player is admitted"),
            SimulationValue.AddPlayer(
                LocalPlayer,
                Faction::MeridianCompact,
                ResourcePool{})) ||
        !TestEqual(
            TEXT("Remembered fixture tile begins blocked"),
            SimulationValue.TerrainAt(RememberedTileX, RememberedTileY),
            Terrain::Blocked))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const EntityId Scout = SimulationValue.SpawnEntity(
        LocalPlayer,
        Faction::MeridianCompact,
        EntityType::ScoutUnit,
        ScoutStart);
    if (!TestTrue(TEXT("Meridian scout spawns through native simulation"), Scout != 0))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const std::optional<PlayerView> InitiallyScoped =
        SimulationValue.CreatePlayerView(LocalPlayer);
    if (!TestTrue(TEXT("Initial scoped player view materializes"), InitiallyScoped.has_value()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    TestEqual(
        TEXT("Scout initially sees the fixture tile"),
        InitiallyScoped->VisibilityAt(RememberedPosition),
        Visibility::Visible);
    TestEqual(
        TEXT("Visible scoped terrain starts from authoritative blocked state"),
        InitiallyScoped->TerrainAt(RememberedTileX, RememberedTileY),
        Terrain::Blocked);

    UWorld* World = WorldWrapper.GetTestWorld();
    FActorSpawnParameters SpawnParameters;
    SpawnParameters.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    const auto SpawnTerrainView = [&]() -> AEchoesTerrainView*
    {
        return World != nullptr
            ? World->SpawnActor<AEchoesTerrainView>(
                  AEchoesTerrainView::StaticClass(),
                  FVector::ZeroVector,
                  FRotator::ZeroRotator,
                  SpawnParameters)
            : nullptr;
    };

    AEchoesTerrainView* ScopedTerrain = SpawnTerrainView();
    AEchoesTerrainView* ObserverTerrain = SpawnTerrainView();
    if (!TestNotNull(TEXT("Scoped terrain actor spawns"), ScopedTerrain) ||
        !TestNotNull(TEXT("Observer terrain actor spawns"), ObserverTerrain))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const bool bScopedInitialized = ScopedTerrain->InitializeTerrain(
        SimulationValue,
        200.0f,
        EEchoesSkirmishMapPreset::GlassScar,
        std::optional<PlayerId>{LocalPlayer},
        EEchoesOperationMode::Skirmish);
    const bool bObserverInitialized = ObserverTerrain->InitializeTerrain(
        SimulationValue,
        200.0f,
        EEchoesSkirmishMapPreset::GlassScar,
        std::nullopt,
        EEchoesOperationMode::Skirmish);
    if (!TestTrue(TEXT("Scoped Glass Scar terrain initializes"), bScopedInitialized) ||
        !TestTrue(TEXT("Observer Glass Scar terrain initializes"), bObserverInitialized))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    TestTrue(
        TEXT("Scoped terrain uses authored runtime meshes"),
        ScopedTerrain->IsUsingAuthoredTerrainMeshes());
    TestTrue(
        TEXT("Observer terrain uses authored runtime meshes"),
        ObserverTerrain->IsUsingAuthoredTerrainMeshes());
    const int32 InitiallyKnownBlockedCount =
        ScopedTerrain->GetInstancedBlockedTileCount();
    TestTrue(
        TEXT("Scoped presentation initially renders visible blocked terrain"),
        InitiallyKnownBlockedCount > 0 &&
            InitiallyKnownBlockedCount < CanonicalBlockedCount);
    TestEqual(
        TEXT("Observer presentation initially renders canonical blocked terrain"),
        ObserverTerrain->GetInstancedBlockedTileCount(),
        CanonicalBlockedCount);

    const std::optional<uint64> AwaySequence =
        SimulationValue.NextCommandSequence(LocalPlayer);
    if (!TestTrue(TEXT("Scout move obtains a native command sequence"), AwaySequence.has_value()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    Command MoveAway;
    MoveAway.executeTick = SimulationValue.CurrentTick();
    MoveAway.player = LocalPlayer;
    MoveAway.sequence = *AwaySequence;
    MoveAway.type = CommandType::Move;
    MoveAway.actor = Scout;
    MoveAway.position = ScoutAway;
    std::string AwayRejection;
    const bool bMoveAwayQueued =
        SimulationValue.QueueCommand(MoveAway, &AwayRejection);
    if (!TestTrue(
            FString::Printf(
                TEXT("Ordinary scout move away is admitted: %s"),
                UTF8_TO_TCHAR(AwayRejection.c_str())),
            bMoveAwayQueued))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    bool bTileBecameExplored = false;
    for (int32 StepIndex = 0; StepIndex < 320; ++StepIndex)
    {
        SimulationValue.Step();
        if (SimulationValue.VisibilityAt(LocalPlayer, RememberedPosition) ==
            Visibility::Explored)
        {
            bTileBecameExplored = true;
            break;
        }
    }
    if (!TestTrue(
            TEXT("Native move and ticks carry the scout beyond the tile's vision radius"),
            bTileBecameExplored))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    if (!TestTrue(
            TEXT("Authoritative fixture changes the out-of-sight tile to scarred"),
            SimulationValue.SetTerrainTile(
                RememberedTileX,
                RememberedTileY,
                Terrain::Scarred)))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    const std::optional<PlayerView> MemoryScoped =
        SimulationValue.CreatePlayerView(LocalPlayer);
    if (!TestTrue(TEXT("Scoped view remains available after terrain mutation"), MemoryScoped.has_value()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    TestEqual(
        TEXT("Out-of-sight tile remains explored"),
        MemoryScoped->VisibilityAt(RememberedPosition),
        Visibility::Explored);
    TestEqual(
        TEXT("Scoped player view retains last observed blocked terrain"),
        MemoryScoped->TerrainAt(RememberedTileX, RememberedTileY),
        Terrain::Blocked);
    TestEqual(
        TEXT("Observer reads the new authoritative scarred terrain"),
        SimulationValue.TerrainAt(RememberedTileX, RememberedTileY),
        Terrain::Scarred);

    TestTrue(
        TEXT("Scoped terrain synchronizes from player information"),
        ScopedTerrain->SyncTerrain(SimulationValue));
    TestTrue(
        TEXT("Observer terrain synchronizes from authority"),
        ObserverTerrain->SyncTerrain(SimulationValue));
    TestEqual(
        TEXT("Scoped raw census still reports current authoritative blocked count"),
        ScopedTerrain->GetBlockedTileCount(),
        CanonicalBlockedCount - 1);
    TestEqual(
        TEXT("Scoped raw census still reports current authoritative scarred count"),
        ScopedTerrain->GetScarredTileCount(),
        1);
    TestEqual(
        TEXT("Scoped rendering retains one remembered blocked tile"),
        ScopedTerrain->GetInstancedBlockedTileCount(),
        InitiallyKnownBlockedCount);
    TestEqual(
        TEXT("Scoped rendering does not disclose the hidden scarred change"),
        ScopedTerrain->GetInstancedScarredTileCount(),
        0);
    TestEqual(
        TEXT("Observer rendering drops the old blocked tile"),
        ObserverTerrain->GetInstancedBlockedTileCount(),
        CanonicalBlockedCount - 1);
    TestEqual(
        TEXT("Observer rendering exposes the new scarred tile"),
        ObserverTerrain->GetInstancedScarredTileCount(),
        1);

    const std::vector<uint8> Snapshot = SimulationValue.SaveSnapshot();
    std::string LoadError;
    std::optional<Simulation> Detached =
        Simulation::LoadSnapshot(Snapshot, &LoadError);
    if (!TestTrue(
            FString::Printf(
                TEXT("Detached terrain-memory snapshot loads: %s"),
                UTF8_TO_TCHAR(LoadError.c_str())),
            Detached.has_value()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const std::optional<PlayerView> DetachedMemory =
        Detached->CreatePlayerView(LocalPlayer);
    if (!TestTrue(
            TEXT("Detached snapshot materializes the scoped information state"),
            DetachedMemory.has_value()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    TestEqual(
        TEXT("Detached snapshot retains explored visibility"),
        DetachedMemory->VisibilityAt(RememberedPosition),
        Visibility::Explored);
    TestEqual(
        TEXT("Detached snapshot retains remembered blocked terrain"),
        DetachedMemory->TerrainAt(RememberedTileX, RememberedTileY),
        Terrain::Blocked);

    AEchoesTerrainView* DetachedTerrain = SpawnTerrainView();
    if (!TestNotNull(TEXT("Detached scoped terrain actor spawns"), DetachedTerrain) ||
        !TestTrue(
            TEXT("Detached scoped terrain initializes"),
            DetachedTerrain != nullptr && DetachedTerrain->InitializeTerrain(
                *Detached,
                200.0f,
                EEchoesSkirmishMapPreset::GlassScar,
                std::optional<PlayerId>{LocalPlayer},
                EEchoesOperationMode::Skirmish)))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    TestEqual(
        TEXT("Detached adapter renders remembered blocked terrain"),
        DetachedTerrain->GetInstancedBlockedTileCount(),
        InitiallyKnownBlockedCount);
    TestEqual(
        TEXT("Detached adapter withholds the hidden scarred terrain"),
        DetachedTerrain->GetInstancedScarredTileCount(),
        0);
    TestEqual(
        TEXT("Detached adapter preserves the authoritative scarred census"),
        DetachedTerrain->GetScarredTileCount(),
        1);

    const std::optional<uint64> ReturnSequence =
        Detached->NextCommandSequence(LocalPlayer);
    if (!TestTrue(TEXT("Returning scout obtains the next native sequence"), ReturnSequence.has_value()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    Command MoveBack;
    MoveBack.executeTick = Detached->CurrentTick();
    MoveBack.player = LocalPlayer;
    MoveBack.sequence = *ReturnSequence;
    MoveBack.type = CommandType::Move;
    MoveBack.actor = Scout;
    MoveBack.position = ScoutStart;
    std::string ReturnRejection;
    const bool bMoveBackQueued =
        Detached->QueueCommand(MoveBack, &ReturnRejection);
    if (!TestTrue(
            FString::Printf(
                TEXT("Ordinary scout return is admitted: %s"),
                UTF8_TO_TCHAR(ReturnRejection.c_str())),
            bMoveBackQueued))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    bool bTileBecameVisibleAgain = false;
    for (int32 StepIndex = 0; StepIndex < 320; ++StepIndex)
    {
        Detached->Step();
        if (Detached->VisibilityAt(LocalPlayer, RememberedPosition) ==
            Visibility::Visible)
        {
            bTileBecameVisibleAgain = true;
            break;
        }
    }
    if (!TestTrue(
            TEXT("Native return move refreshes visibility of the fixture tile"),
            bTileBecameVisibleAgain))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const std::optional<PlayerView> RefreshedScoped =
        Detached->CreatePlayerView(LocalPlayer);
    if (!TestTrue(TEXT("Refreshed scoped player view materializes"), RefreshedScoped.has_value()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    TestEqual(
        TEXT("Re-observation refreshes scoped terrain to scarred"),
        RefreshedScoped->TerrainAt(RememberedTileX, RememberedTileY),
        Terrain::Scarred);
    TestTrue(
        TEXT("Detached terrain synchronizes after re-observation"),
        DetachedTerrain->SyncTerrain(*Detached));
    TestEqual(
        TEXT("Refreshed adapter drops the stale blocked memory"),
        DetachedTerrain->GetInstancedBlockedTileCount(),
        InitiallyKnownBlockedCount - 1);
    TestEqual(
        TEXT("Refreshed adapter renders the newly observed scarred tile"),
        DetachedTerrain->GetInstancedScarredTileCount(),
        1);

    // Observer playback may first materialize at a seek tick after terrain
    // changes. Admission must use authored topology, not that seek's state.
    namespace Dressing = echoes::world::glass_scar_dressing;
    const auto& ChangedRecord = Dressing::kRecords[0];
    TestTrue(TEXT("Observer fixture changes one authored dressing cell"),
        Detached->SetTerrainTile(ChangedRecord.X, ChangedRecord.Y, Terrain::Open));
    AEchoesTerrainView* SeekObserver = SpawnTerrainView();
    if (TestNotNull(TEXT("Seek observer terrain actor spawns"), SeekObserver))
    {
        TestTrue(TEXT("Observer initializes at a changed terrain tick"),
            SeekObserver->InitializeTerrain(*Detached, 200.f,
                EEchoesSkirmishMapPreset::GlassScar, std::nullopt,
                EEchoesOperationMode::Skirmish));
        TestTrue(TEXT("One changed cell cannot deactivate canonically valid dressing"),
            SeekObserver->IsDressingActive());
        TestFalse(TEXT("Observer hides the changed record at first seek"),
            SeekObserver->IsDressingRecordInstanced(0));
        TestTrue(TEXT("Other compatible observer records remain drawn"),
            SeekObserver->GetDressingInstancedCount() > 0);
        SeekObserver->Destroy();
    }

    DetachedTerrain->Destroy();
    ObserverTerrain->Destroy();
    ScopedTerrain->Destroy();
    WorldWrapper.ForwardErrorMessages(this);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
