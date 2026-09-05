#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "EchoesCliffMesh.h"
#include "EchoesTerrainView.h"
#include "EchoesTestSaveEnvironment.h"
#include "Engine/World.h"
#include "ProceduralMeshComponent.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEchoesM01ExteriorBankTest,
    "Echoes.Runtime.Map.M01ExteriorBanks",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEchoesM01ExteriorBankTest::RunTest(const FString&)
{
    TMap<FIntVector, int32> Welded;
    TArray<FVector> Positions;
    TMap<FIntVector, FVector> ShadingNormals;
    bool bContinuousShading = true;
    TMap<uint64, int32> EdgeUses;
    bool bOutside = true, bFinite = true, bWinding = true, bRepeatable = true;
    int32 TriangleCount = 0;
    double ProjectedArea = 0;
    for (int32 Side = 0; Side < 4; ++Side)
    {
        const auto Mesh = EchoesCliffMesh::BuildExteriorBank(64, 64, 200, Side);
        const auto Repeat = EchoesCliffMesh::BuildExteriorBank(64, 64, 200, Side);
        TestTrue(TEXT("each public bank has geometry"), !Mesh.Vertices.IsEmpty());
        TestEqual(TEXT("normal stream aligned"), Mesh.Normals.Num(), Mesh.Vertices.Num());
        TestEqual(TEXT("UV stream aligned"), Mesh.UV0.Num(), Mesh.Vertices.Num());
        bRepeatable &= Mesh.Vertices == Repeat.Vertices && Mesh.Triangles == Repeat.Triangles && Mesh.Normals == Repeat.Normals;
        for (int32 I = 0; I < Mesh.Triangles.Num(); I += 3)
        {
            int32 Ids[3];
            FBox Bounds(ForceInit);
            for (int32 J = 0; J < 3; ++J)
            {
                const FVector P = Mesh.Vertices[Mesh.Triangles[I+J]];
                Bounds += P;
                bFinite &= !P.ContainsNaN();
                const FIntVector Key(FMath::RoundToInt(P.X*10), FMath::RoundToInt(P.Y*10), FMath::RoundToInt(P.Z*10));
                const FVector N = Mesh.Normals[Mesh.Triangles[I+J]];
                bContinuousShading &= !N.ContainsNaN() && FMath::IsNearlyEqual(N.SizeSquared(),1.0,1.e-4);
                if (const FVector* Existing = ShadingNormals.Find(Key))
                    bContinuousShading &= Existing->Equals(N,1.e-4);
                else ShadingNormals.Add(Key,N);
                int32* Found = Welded.Find(Key);
                if (!Found) { const int32 Id = Positions.Add(P); Welded.Add(Key, Id); Ids[J] = Id; }
                else Ids[J] = *Found;
            }
            bOutside &= Bounds.Max.X <= -6499.999 || Bounds.Min.X >= 6299.999 ||
                        Bounds.Max.Y <= -6499.999 || Bounds.Min.Y >= 6299.999;
            const FVector A = Mesh.Vertices[Mesh.Triangles[I]], B = Mesh.Vertices[Mesh.Triangles[I+1]], C = Mesh.Vertices[Mesh.Triangles[I+2]];
            const FVector Normal = (C-A).Cross(B-A);
            ProjectedArea += FMath::Abs(Normal.Z) * .5;
            bWinding &= Normal.SizeSquared() > UE_SMALL_NUMBER && Normal.Z > 0 &&
                        Normal.Dot(Mesh.Normals[Mesh.Triangles[I]]) > 0 &&
                        Normal.Dot(Mesh.Normals[Mesh.Triangles[I+1]]) > 0 &&
                        Normal.Dot(Mesh.Normals[Mesh.Triangles[I+2]]) > 0;
            for (int32 J = 0; J < 3; ++J)
            {
                const uint32 Low = FMath::Min(Ids[J], Ids[(J+1)%3]);
                const uint32 High = FMath::Max(Ids[J], Ids[(J+1)%3]);
                ++EdgeUses.FindOrAdd((static_cast<uint64>(Low)<<32) | High);
            }
            ++TriangleCount;
        }
    }
    TestTrue(TEXT("every complete triangle stays outside the playable rectangle"), bOutside);
    TestTrue(TEXT("positions are finite and source generation is repeatable"), bFinite && bRepeatable);
    TestTrue(TEXT("all faces are nondegenerate and wind upward"), bWinding);
    TestTrue(TEXT("shared vertices and corner seams have finite matching unit normals"), bContinuousShading);
    // Folded or overlapping quads add excess absolute projected area even when
    // the generator repairs their winding. Joined rims bound this exact annulus.
    TestTrue(TEXT("projected banks cover the annulus once without folds"),
        FMath::IsNearlyEqual(ProjectedArea, 32800.0*32800.0 - 12800.0*12800.0, 100.0));
    TestTrue(TEXT("bounded exterior geometry stays below 12000 triangles"), TriangleCount > 0 && TriangleCount <= 12000);
    bool bWatertightBetweenRims = true;
    int32 BoundaryEdges = 0;
    for (const auto& Edge : EdgeUses)
    {
        if (Edge.Value == 2) continue;
        const FVector A = Positions[static_cast<uint32>(Edge.Key>>32)];
        const FVector B = Positions[static_cast<uint32>(Edge.Key)];
        // Only inner and outer substrate-contact rims may have a free edge.
        bWatertightBetweenRims &= Edge.Value == 1 && FMath::IsNearlyEqual(A.Z, -2.0) && FMath::IsNearlyEqual(B.Z, -2.0);
        ++BoundaryEdges;
    }
    TestTrue(TEXT("all strata and four corner seams join without open interior edges"), bWatertightBetweenRims);
    TestEqual(TEXT("exactly two complete perimeter rims remain"), BoundaryEdges, 512);
    TestTrue(TEXT("invalid side refuses geometry"), EchoesCliffMesh::BuildExteriorBank(64,64,200,4).Vertices.IsEmpty());
    TestTrue(TEXT("invalid map refuses geometry"), EchoesCliffMesh::BuildExteriorBank(0,64,200,0).Vertices.IsEmpty());

    FEchoesScopedTestSaveEnvironment Saves(*this);
    if (!Saves.IsReady()) return false;
    FTestWorldWrapper World;
    if (!World.CreateTestWorld(EWorldType::Game)) return false;
    auto* View = World.GetTestWorld()->SpawnActor<AEchoesTerrainView>();
    if (!TestNotNull(TEXT("terrain actor"), View) || !TestTrue(TEXT("M01 initializes"),
        View->InitializeScopedTerrain(64,64,200,EEchoesSkirmishMapPreset::GlassScar,EEchoesOperationMode::CampaignPrologue))) return false;
    UProceduralMeshComponent* Banks = nullptr;
    TArray<UProceduralMeshComponent*> Layers;
    View->GetComponents(Layers);
    for (auto* Layer : Layers) if (Layer->GetName() == TEXT("M01ExteriorBanks")) Banks = Layer;
    if (!TestNotNull(TEXT("separate public exterior bank component"), Banks)) return false;
    TestTrue(TEXT("no collision"), Banks->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
    TestFalse(TEXT("no navigation"), Banks->CanEverAffectNavigation());
    TestFalse(TEXT("no overlaps"), Banks->GetGenerateOverlapEvents());
    TestFalse(TEXT("no shadow"), Banks->CastShadow);
    TestFalse(TEXT("no decal receiver"), Banks->bReceivesDecals);
    TestEqual(TEXT("four boundary sections"), Banks->GetNumSections(), 4);
    TArray<FVector> Before;
    for (int32 Side = 0; Side < 4; ++Side)
    {
        const auto* Section = Banks->GetProcMeshSection(Side);
        if (!TestNotNull(TEXT("side section"), Section)) return false;
        TestFalse(TEXT("no section collision"), Section->bEnableCollision);
        for (const auto& Vertex : Section->ProcVertexBuffer) Before.Add(Vertex.Position);
        TestTrue(TEXT("registered basalt material"), Banks->GetMaterial(Side) &&
            Banks->GetMaterial(Side)->GetPathName() == TEXT("/Game/Art/Generated/Materials/M_EchoesCliffSurface.M_EchoesCliffSurface"));
    }
    std::vector<echoes::sim::net::ScopedTileState> Tiles(4096);
    for (const auto Visibility : {echoes::sim::Visibility::Unexplored, echoes::sim::Visibility::Visible, echoes::sim::Visibility::Explored})
    {
        for (auto& Tile : Tiles) { Tile.terrain = echoes::sim::Terrain::Open; Tile.visibility = Visibility; }
        TestTrue(TEXT("scoped terrain transition"), View->SyncScopedTerrain(Tiles));
        TArray<FVector> After;
        for (int32 Side = 0; Side < 4; ++Side)
            if (const auto* Section = Banks->GetProcMeshSection(Side))
                for (const auto& Vertex : Section->ProcVertexBuffer) After.Add(Vertex.Position);
        TestTrue(TEXT("public bank geometry cannot disclose a fog or terrain change"), Before == After);
    }
    TestTrue(TEXT("same actor reinitializes for another mission"), View->InitializeScopedTerrain(
        64,64,200,EEchoesSkirmishMapPreset::GlassScar,EEchoesOperationMode::CampaignSevenAccounts));
    TestEqual(TEXT("M01 bank geometry cannot leak into another mission"), Banks->GetNumSections(), 0);
    View->Destroy();
    return true;
}
#endif
