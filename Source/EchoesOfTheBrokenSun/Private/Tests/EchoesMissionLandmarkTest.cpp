#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "EchoesTerrainView.h"
#include "EchoesCampaignTerrainBinding.h"
#include "EchoesTestSaveEnvironment.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "ProceduralMeshComponent.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"
#include "../../../../Content/World/Generated/Presentation/EchoesMissionLandmarks.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEchoesMissionLandmarkTest,
    "Echoes.Runtime.Map.MissionLandmarkVisibility",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEchoesMissionLandmarkTest::RunTest(const FString&)
{
    FEchoesScopedTestSaveEnvironment Saves(*this);
    if (!Saves.IsReady()) return false;
    FTestWorldWrapper World;
    if (!World.CreateTestWorld(EWorldType::Game)) return false;
    auto* View = World.GetTestWorld()->SpawnActor<AEchoesTerrainView>();
    if (!TestNotNull(TEXT("Terrain view"), View) || !TestTrue(TEXT("M01 landmarks initialize"),
        View->InitializeScopedTerrain(64,64,200,EEchoesSkirmishMapPreset::GlassScar,
                                     EEchoesOperationMode::CampaignPrologue))) return false;
    TArray<UInstancedStaticMeshComponent*> Components;
    View->GetComponents(Components);
    TArray<UInstancedStaticMeshComponent*> Layers;
    for (auto* Layer : Components)
        // Public M01 exterior substrate is a terrain layer, not a landmark.
        if (Layer->GetName() == TEXT("M01ArchiveCradle") ||
            Layer->GetName() == TEXT("M01ArchiveFrame") ||
            Layer->GetName() == TEXT("M01RoutePaving") ||
            Layer->GetName() == TEXT("M01ServiceConduit") ||
            Layer->GetName() == TEXT("M01ArchiveApron") ||
            Layer->GetName() == TEXT("M01ArchiveLoadingFace"))
        {
            Layers.Add(Layer);
            TestTrue(TEXT("Landmark has no collision"), Layer->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
            TestFalse(TEXT("Landmark has no navigation"), Layer->CanEverAffectNavigation());
            TestFalse(TEXT("Landmark has no overlap"), Layer->GetGenerateOverlapEvents());
            TestFalse(TEXT("Landmark has no shadow leak"), Layer->CastShadow);
            if (!TestNotNull(TEXT("Registered landmark mesh"), Layer->GetStaticMesh().Get())) return false;
            TestEqual(TEXT("Two authored LODs"), Layer->GetStaticMesh()->GetNumLODs(), 2);
        }
    TestEqual(TEXT("Six functional prop families"), Layers.Num(), 6);
    UProceduralMeshComponent* Cliffs = nullptr;
    TArray<UProceduralMeshComponent*> ProceduralLayers;
    View->GetComponents(ProceduralLayers);
    for (auto* Layer : ProceduralLayers)
        if (Layer->GetName() == TEXT("M01ContinuousCliffs")) Cliffs = Layer;
    if (!TestNotNull(TEXT("Continuous M01 cliff presentation"), Cliffs)) return false;
    TestTrue(TEXT("Cliffs have no collision"), Cliffs->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
    TestFalse(TEXT("Cliffs cannot affect navigation"), Cliffs->CanEverAffectNavigation());
    TestFalse(TEXT("Cliffs cast no hidden-state shadow"), Cliffs->CastShadow);
    const auto CliffVertices = [Cliffs]()
    {
        int32 Count=0;
        for (int32 Section=0; Section<Cliffs->GetNumSections(); ++Section)
            if (const FProcMeshSection* Mesh = Cliffs->GetProcMeshSection(Section))
                Count += Mesh->ProcVertexBuffer.Num();
        return Count;
    };
    const auto Drawn = [&Layers]()
    {
        int32 Count = 0;
        for (auto* Layer : Layers)
            for (int32 Index=0; Index<Layer->GetInstanceCount(); ++Index)
            {
                FTransform Transform;
                Layer->GetInstanceTransform(Index,Transform);
                Count += !Transform.GetScale3D().IsNearlyZero();
            }
        return Count;
    };
    using namespace echoes::sim;
    std::vector<net::ScopedTileState> Tiles(4096);
    for (auto& Tile : Tiles) { Tile.terrain=Terrain::Blocked; Tile.visibility=Visibility::Unexplored; }
    TestTrue(TEXT("Unknown terrain syncs"), View->SyncScopedTerrain(Tiles));
    TestEqual(TEXT("Unknown sites reveal no props"), Drawn(), 0);
    TestEqual(TEXT("Unknown terrain reveals no cliff geometry"), CliffVertices(), 0);
    for (int32 Y=0; Y<64; ++Y)
        for (int32 X=0; X<64; ++X)
        {
            auto& Tile=Tiles[Y*64+X];
            Tile.terrain=echoes::world::IsCampaignTerrainPassable(1,FutureWellChoice::Preserve,X,Y) ? Terrain::Open : Terrain::Blocked;
            Tile.visibility=Visibility::Visible;
        }
    TestTrue(TEXT("Visible source terrain syncs"), View->SyncScopedTerrain(Tiles));
    const int32 KnownCliffVertices = CliffVertices();
    TestTrue(TEXT("Known blocked terrain produces connected cliffs"), KnownCliffVertices > 0);
    const int32 RecordCount=static_cast<int32>(echoes::world::mission_landmarks::kRecords.size());
    TestEqual(TEXT("Every authored site draws on its legal terrain"), Drawn(), RecordCount);
    UInstancedStaticMeshComponent* Apron = nullptr;
    for (auto* Layer : Layers) if (Layer->GetName() == TEXT("M01ArchiveApron")) Apron = Layer;
    if (!TestNotNull(TEXT("Archive working apron"), Apron)) return false;
    TestEqual(TEXT("One composed archive apron"), Apron->GetInstanceCount(), 1);
    const auto ApronVisible = [Apron]()
    {
        FTransform Transform;
        return Apron->GetInstanceTransform(0, Transform) && !Transform.GetScale3D().IsNearlyZero();
    };
    FTransform ApronTransform;
    Apron->GetInstanceTransform(0, ApronTransform);
    TestTrue(TEXT("Authored half-tile pivot aligns apron to its footprint"),
        ApronTransform.GetLocation().Equals(FVector(-2400, -2900, 0), .01f));
    Tiles[16*64+18].visibility = Visibility::Unexplored;
    View->SyncScopedTerrain(Tiles);
    TestFalse(TEXT("One unknown corner hides entire shared apron"), ApronVisible());
    Tiles[16*64+18].visibility = Visibility::Explored;
    View->SyncScopedTerrain(Tiles);
    TestTrue(TEXT("Known footprint restores apron"), ApronVisible());
    Tiles[16*64+18].terrain = Terrain::Blocked;
    View->SyncScopedTerrain(Tiles);
    TestFalse(TEXT("Incompatible footprint terrain removes apron"), ApronVisible());
    Tiles[16*64+18].terrain = Terrain::Open;
    View->SyncScopedTerrain(Tiles);
    TestTrue(TEXT("Restored open footprint restores apron"), ApronVisible());
    UInstancedStaticMeshComponent* LoadingFace = nullptr;
    for (auto* Layer : Layers) if (Layer->GetName() == TEXT("M01ArchiveLoadingFace")) LoadingFace = Layer;
    if (!TestNotNull(TEXT("Composed archive loading face"), LoadingFace)) return false;
    TestEqual(TEXT("One supported loading installation"), LoadingFace->GetInstanceCount(), 1);
    const auto LoadingFaceVisible = [LoadingFace]()
    {
        FTransform Transform;
        return LoadingFace->GetInstanceTransform(0, Transform) && !Transform.GetScale3D().IsNearlyZero();
    };
    TestTrue(TEXT("Known blocked footprint shows loading face"), LoadingFaceVisible());
    Tiles[19*64+23].visibility = Visibility::Unexplored;
    View->SyncScopedTerrain(Tiles);
    TestFalse(TEXT("One unknown loading cell hides shared structure"), LoadingFaceVisible());
    Tiles[19*64+23].visibility = Visibility::Explored;
    View->SyncScopedTerrain(Tiles);
    TestTrue(TEXT("Known loading footprint restores structure"), LoadingFaceVisible());
    const auto ChangedCellHasCliff = [Cliffs]()
    {
        const float CenterX=(12-32)*200.0f;
        const float CenterY=(20-32)*200.0f;
        for (int32 Section=0; Section<Cliffs->GetNumSections(); ++Section)
            if (const FProcMeshSection* Mesh=Cliffs->GetProcMeshSection(Section))
                for (const FProcMeshVertex& Vertex : Mesh->ProcVertexBuffer)
                    if (FMath::Abs(Vertex.Position.X-CenterX) < 99 &&
                        FMath::Abs(Vertex.Position.Y-CenterY) < 99 && Vertex.Position.Z > 0)
                        return true;
        return false;
    };
    TestTrue(TEXT("Ordinary blocked cell initially has a cliff surface"), ChangedCellHasCliff());
    Tiles[20*64+12].passable=true;
    View->SyncScopedTerrain(Tiles);
    TestFalse(TEXT("Reshape passability removes the obstructing cliff"), ChangedCellHasCliff());
    Tiles[20*64+12].passable=false;
    View->SyncScopedTerrain(Tiles);
    TestTrue(TEXT("Restored blockage restores the cliff"), ChangedCellHasCliff());
    for (auto& Tile : Tiles) Tile.visibility=Visibility::Explored;
    TestTrue(TEXT("Explored memory syncs"), View->SyncScopedTerrain(Tiles));
    TestEqual(TEXT("Explored sites retain their known architecture"), Drawn(), RecordCount);
    TestEqual(TEXT("Explored terrain retains its known cliff mesh"), CliffVertices(), KnownCliffVertices);
    const auto& First=echoes::world::mission_landmarks::kRecords.front();
    auto& ChangedTile = Tiles[First.y*64+First.x];
    ChangedTile.terrain=ChangedTile.terrain == Terrain::Blocked ? Terrain::Open : Terrain::Blocked;
    TestTrue(TEXT("Changed terrain syncs"), View->SyncScopedTerrain(Tiles));
    TestEqual(TEXT("A terrain change removes the conflicting prop"), Drawn(), RecordCount-1);
    for (auto& Tile : Tiles) Tile.visibility=Visibility::Unexplored;
    View->SyncScopedTerrain(Tiles);
    TestEqual(TEXT("Knowledge reset hides all props"), Drawn(), 0);
    TestEqual(TEXT("Knowledge reset removes all cliff geometry"), CliffVertices(), 0);
    TestTrue(TEXT("Another mission reinitializes"), View->InitializeScopedTerrain(64,64,200,
        EEchoesSkirmishMapPreset::GlassScar,EEchoesOperationMode::CampaignSevenAccounts));
    TestEqual(TEXT("M01 cliffs do not persist into another mission"), CliffVertices(), 0);
    // The six legacy-named components are a reusable layer pool. M02 now has
    // its own registered pack: require replacement, not an empty destination.
    namespace Landmarks = echoes::world::mission_landmarks;
    int32 M02Counts[6] = {};
    for (const auto& Record : Landmarks::m02::kRecords) ++M02Counts[Record.kind];
    for (int32 Kind = 0; Kind < 6; ++Kind)
    {
        const FString LayerName = FString(TEXT("M01")) + UTF8_TO_TCHAR(Landmarks::m01::kKindNames[Kind]);
        UInstancedStaticMeshComponent* Layer = nullptr;
        for (auto* Candidate : Layers) if (Candidate->GetName() == LayerName) Layer = Candidate;
        if (!TestNotNull(TEXT("Shared landmark layer remains available"), Layer)) return false;
        TestEqual(TEXT("Destination pack replaces every source instance"), Layer->GetInstanceCount(), M02Counts[Kind]);
        if (Kind < static_cast<int32>(Landmarks::m02::kKindNames.size()))
        {
            const FString ExpectedMesh = FString(TEXT("SM_World_M02")) + UTF8_TO_TCHAR(Landmarks::m02::kKindNames[Kind]);
            TestTrue(TEXT("Destination layer uses its registered mesh"), Layer->GetStaticMesh() &&
                     Layer->GetStaticMesh()->GetName() == ExpectedMesh);
        }
    }
    TestEqual(TEXT("Mission change does not inherit prior terrain knowledge"), Drawn(), 0);
    World.ForwardErrorMessages(this);
    return true;
}
#endif
