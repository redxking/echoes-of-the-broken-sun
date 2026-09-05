// Copyright Angelis Pseftis. Presentation-only continuous cliff chunk geometry.
#pragma once

#include "CoreMinimal.h"

namespace EchoesCliffMesh
{
struct FGeometry
{
    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UV0;

    void Reset()
    {
        Vertices.Reset();
        Triangles.Reset();
        Normals.Reset();
        UV0.Reset();
    }
};

/** Generates cosmetic chunks from copied presentation mask data only. */
FGeometry BuildChunk(
    int32 Width, int32 Height, float TileSize,
    const TArray<uint8>& ExposedBlockedMask,
    int32 MinX, int32 MinY, int32 MaxXExclusive, int32 MaxYExclusive);

/** Fixed public M01 backdrop; Side 0..3 shares corner samples, never private cells. */
FGeometry BuildExteriorBank(int32 Width, int32 Height, float TileSize, int32 Side);

}
