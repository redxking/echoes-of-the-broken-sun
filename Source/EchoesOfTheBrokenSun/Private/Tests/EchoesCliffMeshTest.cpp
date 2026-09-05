#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "EchoesCliffMesh.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesCliffMeshTest,
    "Echoes.Runtime.Map.ContinuousCliffGeometry",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEchoesCliffMeshTest::RunTest(const FString& Parameters)
{
    constexpr int32 Width = 4;
    constexpr int32 Height = 3;
    constexpr float TileSize = 100.0f;
    TArray<uint8> Mask;
    Mask.Init(0, Width * Height);
    Mask[1 * Width + 1] = 1;
    Mask[1 * Width + 2] = 1;
    EchoesCliffMesh::FGeometry Mesh = EchoesCliffMesh::BuildChunk(
        Width, Height, TileSize, Mask, 1, 1, 3, 2);
    TestTrue(TEXT("active cells produce geometry"), Mesh.Vertices.Num() > 0 && Mesh.Triangles.Num() > 0);
    TestEqual(TEXT("geometry streams stay aligned"), Mesh.Vertices.Num(), Mesh.Normals.Num());
    TestEqual(TEXT("UV stream stays aligned"), Mesh.Vertices.Num(), Mesh.UV0.Num());

    const float InternalBoundaryX = -50.0f;
    TMap<int32, float> BoundaryHeightsByY;
    for (int32 VertexIndex = 0; VertexIndex < Mesh.Vertices.Num(); ++VertexIndex)
    {
        const FVector& Vertex = Mesh.Vertices[VertexIndex];
        if (FMath::IsNearlyEqual(Vertex.X, InternalBoundaryX, .001f) && Vertex.Z > 0.0f)
        {
            if (Mesh.Normals[VertexIndex].Z > 0.0f)
            {
                const int32 QuantizedY = FMath::RoundToInt(Vertex.Y * 1000.0f);
                if (const float* ExistingHeight = BoundaryHeightsByY.Find(QuantizedY))
                {
                    TestTrue(TEXT("adjacent top patches share exact boundary height"),
                             FMath::IsNearlyEqual(Vertex.Z, *ExistingHeight, .001f));
                }
                else
                {
                    BoundaryHeightsByY.Add(QuantizedY, Vertex.Z);
                }
            }
        }
        TestTrue(TEXT("chunk vertices remain inside contributing cell footprints"),
                 Vertex.X >= -150.001f && Vertex.X <= 50.001f &&
                 Vertex.Y >= -100.001f && Vertex.Y <= 0.001f);
    }
    TestTrue(TEXT("shared boundary has top samples"), BoundaryHeightsByY.Num() >= 4);

    bool bFoundWestExteriorWall = false;
    bool bFoundInternalXWall = false;
    for (int32 TriangleIndex = 0; TriangleIndex < Mesh.Triangles.Num(); TriangleIndex += 3)
    {
        const int32 AIndex = Mesh.Triangles[TriangleIndex];
        const int32 BIndex = Mesh.Triangles[TriangleIndex + 1];
        const int32 CIndex = Mesh.Triangles[TriangleIndex + 2];
        const FVector& A = Mesh.Vertices[AIndex];
        const FVector& B = Mesh.Vertices[BIndex];
        const FVector& C = Mesh.Vertices[CIndex];
        const FVector GeometryCoreNormal = (C - A).Cross(B - A);
        TestTrue(TEXT("triangle is non-degenerate"), GeometryCoreNormal.SizeSquared() > UE_SMALL_NUMBER);
        TestTrue(TEXT("triangle winding agrees with stored outward normal"),
                 GeometryCoreNormal.Dot(Mesh.Normals[AIndex]) > 0.0f);

        const FVector Centroid = (A + B + C) / 3.0f;
        const FVector FaceNormal = GeometryCoreNormal.GetSafeNormal();
        // Warped north/south corner strata can enter the same XY window.
        // Classify by unsigned plane orientation so reversed west winding
        // still fails, and include every height stratum.
        const bool bXDominant = FMath::Abs(FaceNormal.X) > FMath::Abs(FaceNormal.Y) &&
                               FMath::Abs(FaceNormal.X) > FMath::Abs(FaceNormal.Z);
        if (Centroid.X >= -151.0f && Centroid.X <= -120.0f &&
            Centroid.Y >= -80.0f && Centroid.Y <= -20.0f &&
            bXDominant)
        {
            bFoundWestExteriorWall = true;
            TestTrue(TEXT("west exterior wall normal faces out of its source footprint"),
                     Mesh.Normals[AIndex].X < -0.5f);
        }
        if (Centroid.X > -55.0f && Centroid.X < -45.0f &&
            Centroid.Y > -95.0f && Centroid.Y < -5.0f &&
            Centroid.Z > 0.0f && Centroid.Z < 120.0f &&
            FMath::Abs(Mesh.Normals[AIndex].X) > .5f && Mesh.Normals[AIndex].Z <= 0.0f)
        {
            bFoundInternalXWall = true;
        }
    }
    TestTrue(TEXT("exposed west boundary emits an outward-facing wall"), bFoundWestExteriorWall);
    TestFalse(TEXT("active neighbours do not receive an internal X-facing wall"), bFoundInternalXWall);

    TArray<uint8> EmptyMask;
    EmptyMask.Init(0, Width * Height);
    const EchoesCliffMesh::FGeometry Empty = EchoesCliffMesh::BuildChunk(
        Width, Height, TileSize, EmptyMask, 1, 1, 3, 2);
    TestTrue(TEXT("zero or unknown mask cells emit no geometry"),
             Empty.Vertices.IsEmpty() && Empty.Triangles.IsEmpty());

    const EchoesCliffMesh::FGeometry Invalid = EchoesCliffMesh::BuildChunk(
        Width, Height, TileSize, TArray<uint8>(), 1, 1, 3, 2);
    TestTrue(TEXT("mismatched source mask emits no partial geometry"),
             Invalid.Vertices.IsEmpty() && Invalid.Triangles.IsEmpty());
    return true;
}
#endif
