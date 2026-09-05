// Copyright Angelis Pseftis. Presentation-only continuous cliff chunk geometry.
#include "EchoesCliffMesh.h"

namespace
{
constexpr int32 SubdivisionsPerTile = 3;
constexpr float CliffBaseHeight = 320.0f;
constexpr float CliffMinimumHeight = 140.0f;
constexpr float CliffMaximumHeight = 500.0f;

bool IsActive(const int32 Width, const int32 Height, const TArray<uint8>& Mask, const int32 X, const int32 Y)
{
    return X >= 0 && X < Width && Y >= 0 && Y < Height && Mask[Y * Width + X] != 0;
}

FVector GridPosition(const int32 Width, const int32 Height, const float TileSize, const int32 SampleX, const int32 SampleY)
{
    const float GridScale = TileSize / static_cast<float>(SubdivisionsPerTile);
    return FVector(
        // Tile x is centred at (x - Width / 2) * TileSize.  The extra half tile
        // moves the sample lattice from those centres to the outer cell edges.
        (static_cast<float>(SampleX) - static_cast<float>(Width * SubdivisionsPerTile) * .5f - static_cast<float>(SubdivisionsPerTile) * .5f) * GridScale,
        (static_cast<float>(SampleY) - static_cast<float>(Height * SubdivisionsPerTile) * .5f - static_cast<float>(SubdivisionsPerTile) * .5f) * GridScale,
        0.0f);
}

bool SideHasActiveCells(
    const int32 Width,
    const int32 Height,
    const TArray<uint8>& Mask,
    const int32 SampleX,
    const int32 SampleY,
    const bool bAlongX,
    const bool bNegativeSide)
{
    const int32 Subdivision = bAlongX ? SampleX : SampleY;
    if (Subdivision % SubdivisionsPerTile != 0)
    {
        return false;
    }

    const int32 BoundaryTile = Subdivision / SubdivisionsPerTile;
    const int32 FixedTile = bNegativeSide ? BoundaryTile - 1 : BoundaryTile;
    const int32 PerpendicularSample = bAlongX ? SampleY : SampleX;
    const int32 PerpendicularTile = PerpendicularSample / SubdivisionsPerTile;
    const bool bAtPerpendicularBoundary = PerpendicularSample % SubdivisionsPerTile == 0;
    const int32 FirstPerpendicularTile = bAtPerpendicularBoundary ? PerpendicularTile - 1 : PerpendicularTile;
    const int32 LastPerpendicularTile = PerpendicularTile;
    for (int32 Perpendicular = FirstPerpendicularTile; Perpendicular <= LastPerpendicularTile; ++Perpendicular)
    {
        const int32 X = bAlongX ? FixedTile : Perpendicular;
        const int32 Y = bAlongX ? Perpendicular : FixedTile;
        if (IsActive(Width, Height, Mask, X, Y))
        {
            return true;
        }
    }
    return false;
}

FVector WarpedGridPosition(
    const int32 Width,
    const int32 Height,
    const float TileSize,
    const TArray<uint8>& Mask,
    const int32 SampleX,
    const int32 SampleY)
{
    FVector Position = GridPosition(Width, Height, TileSize, SampleX, SampleY);
    // Break the long cell-aligned retaining edge into broad cleaved shoulders.
    // Inset only: cosmetic erosion cannot claim a walkable neighbouring cell.
    const float FractureMagnitude = TileSize * (.055f + .16f * FMath::Abs(FMath::Sin(
        Position.X * .0081f + Position.Y * .0063f +
        .65f * FMath::Sin(Position.X * .002f - Position.Y * .003f))));
    const bool bLeftActive = SideHasActiveCells(Width, Height, Mask, SampleX, SampleY, true, true);
    const bool bRightActive = SideHasActiveCells(Width, Height, Mask, SampleX, SampleY, true, false);
    if (bLeftActive != bRightActive)
    {
        Position.X += bLeftActive ? -FractureMagnitude : FractureMagnitude;
    }
    const bool bBottomActive = SideHasActiveCells(Width, Height, Mask, SampleX, SampleY, false, true);
    const bool bTopActive = SideHasActiveCells(Width, Height, Mask, SampleX, SampleY, false, false);
    if (bBottomActive != bTopActive)
    {
        Position.Y += bBottomActive ? -FractureMagnitude : FractureMagnitude;
    }
    return Position;
}

float RimHeightScale(
    const int32 Width, const int32 Height,
    const TArray<uint8>& Mask, const int32 SampleX, const int32 SampleY)
{
    // Weathered shoulders fall toward the exposed perimeter. Samples use the
    // complete knowledge mask, including across chunk seams, so the rock crest
    // is continuous and its skirt stays inside the blocked footprint.
    const float X = static_cast<float>(SampleX) / SubdivisionsPerTile;
    const float Y = static_cast<float>(SampleY) / SubdivisionsPerTile;
    const int32 CellX = FMath::FloorToInt(X);
    const int32 CellY = FMath::FloorToInt(Y);
    float Distance = .75f;
    for (int32 DY = -1; DY <= 1; ++DY)
        for (int32 DX = -1; DX <= 1; ++DX)
        {
            const int32 NX = CellX + DX;
            const int32 NY = CellY + DY;
            if (IsActive(Width, Height, Mask, NX, NY)) continue;
            const float GapX = FMath::Max(FMath::Max(NX - X, X - (NX + 1)), 0.0f);
            const float GapY = FMath::Max(FMath::Max(NY - Y, Y - (NY + 1)), 0.0f);
            Distance = FMath::Min(Distance, FMath::Sqrt(GapX * GapX + GapY * GapY));
        }
    // A constant rim fraction creates a machined bevel around every bank.
    // Vary the erosion at metre scale, continuously across tile/chunk seams.
    const float Cleave = .5f + .5f * FMath::Sin(X * 1.17f + Y * .83f);
    const float Rim = FMath::Lerp(.26f, .52f, Cleave);
    const float Shoulder = FMath::Lerp(.48f, .82f,
        .5f + .5f * FMath::Sin(X * .63f - Y * .91f));
    return FMath::Lerp(Rim, 1.0f, FMath::SmoothStep(0.0f, Shoulder, Distance));
}

float TopHeightAtWorldPosition(
    const FVector& Position, const int32 Width, const int32 Height, const float TileSize)
{
    const float Macro = FMath::Sin(Position.X * .0023f + Position.Y * .001f) * 95.0f +
                        FMath::Cos(Position.Y * .003f - Position.X * .001f) * 70.0f;
    const float Cleave = FMath::Sin((Position.X + Position.Y) * .01f) * 22.0f;
    float Crest = CliffBaseHeight + Macro + Cleave;
    // This compositor currently serves only the registered 64x64 M01 map.
    // Shape its working-court bank as a low inner retaining edge rising into
    // a taller back crest. The existing exposed-blocked mask still determines
    // every emitted vertex; this changes neither passability nor knowledge.
    if (Width == 64 && Height == 64)
    {
        const float X = Position.X / TileSize + Width * .5f;
        const float Y = Position.Y / TileSize + Height * .5f;
        const float Grain = 12.0f * FMath::Sin(X * 1.1f + Y * .37f);
        if (X >= 22.5f && X <= 27.5f && Y >= 17.5f && Y <= 22.5f)
        {
            const float Rise = FMath::SmoothStep(18.5f, 22.5f, Y);
            Crest = FMath::Lerp(185.0f, 360.0f, Rise) + Grain;
        }
        else if (X >= 27.5f && X <= 29.5f && Y >= 13.5f && Y <= 22.5f)
        {
            const float Rise = FMath::SmoothStep(15.5f, 22.5f, Y);
            Crest = FMath::Lerp(180.0f, 300.0f, Rise) + Grain;
        }
        else if (X >= 22.5f && X <= 26.5f && Y >= 13.5f && Y <= 15.5f)
        {
            Crest = 160.0f + Grain;
        }
        else if (X >= 17.5f && X <= 20.5f && Y >= 8.5f && Y <= 12.5f)
        {
            // The service recess opens toward the ordinary southwest camera.
            // Lower its foreground shoulder, retaining the darker back crest.
            const float Back = FMath::Min(FMath::SmoothStep(17.5f, 20.5f, X),
                1.0f - FMath::SmoothStep(8.5f, 12.5f, Y));
            Crest = FMath::Lerp(185.0f, 265.0f, Back) + Grain;
        }
        else if (X >= -.5f && X <= 2.5f && Y >= 9.5f && Y <= 13.5f)
        {
            const float Back = FMath::Min(FMath::SmoothStep(-.5f, 2.5f, X),
                1.0f - FMath::SmoothStep(9.5f, 13.5f, Y));
            Crest = FMath::Lerp(180.0f, 250.0f, Back) + Grain;
        }
    }
    Crest = FMath::Clamp(Crest, CliffMinimumHeight, CliffMaximumHeight);
    if (Width == 64 && Height == 64)
    {
        const float X = Position.X / TileSize + Width * .5f;
        const float Y = Position.Y / TileSize + Height * .5f;
        // The worker-side service manifold sits in a shallow maintained recess.
        // A tall ring around its single occupied tile hid its fittings at the
        // tactical camera and made maintenance access look impossible. Keep
        // every blocked cell, but ease its foreground shoulder into low strata.
        const float Distance = FVector2D((X - 27.0f) / 2.2f, (Y - 26.0f) / 2.0f).Size();
        const float Weight = 1.0f - FMath::SmoothStep(.65f, 1.0f, Distance);
        const float Rear = FMath::SmoothStep(26.0f, 28.0f, X) *
            (1.0f - FMath::SmoothStep(24.0f, 26.0f, Y));
        const float ServiceCrest = FMath::Lerp(64.0f, 145.0f, Rear) +
            7.0f * FMath::Sin(X * 1.1f + Y * .37f);
        Crest = FMath::Lerp(Crest, ServiceCrest, Weight);
    }
    return Crest;
}

void AddTriangle(
    EchoesCliffMesh::FGeometry& Mesh,
    FVector A,
    FVector B,
    FVector C,
    FVector2D UvA,
    FVector2D UvB,
    FVector2D UvC,
    const FVector& DesiredNormal)
{
    FVector Normal = (C - A).Cross(B - A); // GeometryCore: edge2 cross edge1.
    if (Normal.Dot(DesiredNormal) < 0.0f)
    {
        Swap(B, C);
        Swap(UvB, UvC);
        Normal = (C - A).Cross(B - A);
    }
    const float LengthSquared = Normal.SizeSquared();
    if (LengthSquared <= UE_SMALL_NUMBER)
    {
        return;
    }
    Normal /= FMath::Sqrt(LengthSquared);
    const int32 Index = Mesh.Vertices.Num();
    Mesh.Vertices.Add(A);
    Mesh.Vertices.Add(B);
    Mesh.Vertices.Add(C);
    Mesh.Normals.Add(Normal);
    Mesh.Normals.Add(Normal);
    Mesh.Normals.Add(Normal);
    Mesh.UV0.Add(UvA);
    Mesh.UV0.Add(UvB);
    Mesh.UV0.Add(UvC);
    Mesh.Triangles.Add(Index);
    Mesh.Triangles.Add(Index + 1);
    Mesh.Triangles.Add(Index + 2);
}

void AddQuad(
    EchoesCliffMesh::FGeometry& Mesh,
    const FVector& A,
    const FVector& B,
    const FVector& C,
    const FVector& D,
    const FVector2D& UvA,
    const FVector2D& UvB,
    const FVector2D& UvC,
    const FVector2D& UvD,
    const FVector& DesiredNormal)
{
    AddTriangle(Mesh, A, B, C, UvA, UvB, UvC, DesiredNormal);
    AddTriangle(Mesh, C, B, D, UvC, UvB, UvD, DesiredNormal);
}

void AddStratifiedWall(
    EchoesCliffMesh::FGeometry& Mesh,
    const FVector& EdgeA,
    const FVector& EdgeB,
    const FVector& Inward,
    const float TileSize)
{
    const FVector Outward = -Inward;
    const float Levels[] = {0.0f, .23f, .49f, .74f, 1.0f};
    // The foot projects from a weathered upper face into the same blocked
    // footprint. The old inward-tapered foot made the bank look suspended.
    const float Recesses[] = {0.0f, TileSize*.024f, TileSize*.009f, TileSize*.018f, 0.0f};
    const float EdgeLength = FVector::Dist2D(EdgeA, EdgeB);
    for (int32 Layer = 0; Layer < UE_ARRAY_COUNT(Levels) - 1; ++Layer)
    {
        const float TopFraction = Levels[Layer];
        const float BottomFraction = Levels[Layer + 1];
        const float TopRecess = Recesses[Layer];
        const float BottomRecess = Recesses[Layer + 1];
        FVector TopA = EdgeA + Inward * TopRecess;
        FVector TopB = EdgeB + Inward * TopRecess;
        FVector BottomA = EdgeA + Inward * BottomRecess;
        FVector BottomB = EdgeB + Inward * BottomRecess;
        TopA.Z *= (1.0f - TopFraction);
        TopB.Z *= (1.0f - TopFraction);
        BottomA.Z *= (1.0f - BottomFraction);
        BottomB.Z *= (1.0f - BottomFraction);
        AddQuad(
            Mesh, TopA, TopB, BottomA, BottomB,
            FVector2D(0.0f, TopA.Z / TileSize),
            FVector2D(EdgeLength / TileSize, TopB.Z / TileSize),
            FVector2D(0.0f, BottomA.Z / TileSize),
            FVector2D(EdgeLength / TileSize, BottomB.Z / TileSize),
            Outward);
    }
}
}

EchoesCliffMesh::FGeometry EchoesCliffMesh::BuildChunk(
    const int32 Width,
    const int32 Height,
    const float TileSize,
    const TArray<uint8>& ExposedBlockedMask,
    const int32 RequestedMinX,
    const int32 RequestedMinY,
    const int32 RequestedMaxX,
    const int32 RequestedMaxY)
{
    EchoesCliffMesh::FGeometry OutMesh;
    if (Width <= 0 || Height <= 0 || TileSize <= 0.0f || ExposedBlockedMask.Num() != Width * Height)
    {
        return OutMesh;
    }

    const int32 MinX = FMath::Clamp(RequestedMinX, 0, Width);
    const int32 MinY = FMath::Clamp(RequestedMinY, 0, Height);
    const int32 MaxX = FMath::Clamp(RequestedMaxX, MinX, Width);
    const int32 MaxY = FMath::Clamp(RequestedMaxY, MinY, Height);
    const auto Top = [&ExposedBlockedMask, Width, Height, TileSize](const int32 X, const int32 Y)
    {
        FVector Result = WarpedGridPosition(Width, Height, TileSize, ExposedBlockedMask, X, Y);
        Result.Z = TopHeightAtWorldPosition(Result, Width, Height, TileSize) * RimHeightScale(Width, Height, ExposedBlockedMask, X, Y);
        return Result;
    };
    for (int32 CellY = MinY; CellY < MaxY; ++CellY)
    {
        for (int32 CellX = MinX; CellX < MaxX; ++CellX)
        {
            if (!IsActive(Width, Height, ExposedBlockedMask, CellX, CellY))
            {
                continue;
            }

            for (int32 LocalY = 0; LocalY < SubdivisionsPerTile; ++LocalY)
            {
                for (int32 LocalX = 0; LocalX < SubdivisionsPerTile; ++LocalX)
                {
                    const int32 SampleX = CellX * SubdivisionsPerTile + LocalX;
                    const int32 SampleY = CellY * SubdivisionsPerTile + LocalY;
                    const FVector A = Top(SampleX, SampleY);
                    const FVector B = Top(SampleX, SampleY + 1);
                    const FVector C = Top(SampleX + 1, SampleY);
                    const FVector D = Top(SampleX + 1, SampleY + 1);
                    AddQuad(
                        OutMesh, A, B, C, D,
                        FVector2D(A.X / TileSize, A.Y / TileSize),
                        FVector2D(B.X / TileSize, B.Y / TileSize),
                        FVector2D(C.X / TileSize, C.Y / TileSize),
                        FVector2D(D.X / TileSize, D.Y / TileSize),
                        FVector::UpVector);
                }
            }

            const int32 SampleX0 = CellX * SubdivisionsPerTile;
            const int32 SampleY0 = CellY * SubdivisionsPerTile;
            if (!IsActive(Width, Height, ExposedBlockedMask, CellX - 1, CellY))
            {
                for (int32 Step = 0; Step < SubdivisionsPerTile; ++Step)
                    AddStratifiedWall(OutMesh, Top(SampleX0, SampleY0 + Step + 1), Top(SampleX0, SampleY0 + Step), FVector::ForwardVector, TileSize);
            }
            if (!IsActive(Width, Height, ExposedBlockedMask, CellX + 1, CellY))
            {
                for (int32 Step = 0; Step < SubdivisionsPerTile; ++Step)
                    AddStratifiedWall(OutMesh, Top(SampleX0 + SubdivisionsPerTile, SampleY0 + Step), Top(SampleX0 + SubdivisionsPerTile, SampleY0 + Step + 1), -FVector::ForwardVector, TileSize);
            }
            if (!IsActive(Width, Height, ExposedBlockedMask, CellX, CellY - 1))
            {
                for (int32 Step = 0; Step < SubdivisionsPerTile; ++Step)
                    AddStratifiedWall(OutMesh, Top(SampleX0 + Step, SampleY0), Top(SampleX0 + Step + 1, SampleY0), FVector::RightVector, TileSize);
            }
            if (!IsActive(Width, Height, ExposedBlockedMask, CellX, CellY + 1))
            {
                for (int32 Step = 0; Step < SubdivisionsPerTile; ++Step)
                    AddStratifiedWall(OutMesh, Top(SampleX0 + Step + 1, SampleY0 + SubdivisionsPerTile), Top(SampleX0 + Step, SampleY0 + SubdivisionsPerTile), -FVector::RightVector, TileSize);
            }
        }
    }
    return OutMesh;
}

EchoesCliffMesh::FGeometry EchoesCliffMesh::BuildExteriorBank(
    const int32 Width, const int32 Height, const float TileSize, const int32 Side)
{
    FGeometry Mesh;
    if (Width <= 0 || Height <= 0 || !FMath::IsFinite(TileSize) || TileSize <= 0 || Side < 0 || Side > 3)
        return Mesh;

    // Fixed public geology: the four joined pieces occupy only the exterior
    // annulus. No terrain mask, visibility state or simulation enters this mesh.
    const float MinX = -(Width + 1) * TileSize * .5f;
    const float MaxX = (Width - 1) * TileSize * .5f;
    const float MinY = -(Height + 1) * TileSize * .5f;
    const float MaxY = (Height - 1) * TileSize * .5f;
    const int32 Segments = FMath::Clamp(FMath::CeilToInt(FMath::Max(Width, Height) * TileSize / 200.0f), 8, 128);
    constexpr float Depths[] = {0,120,300,550,850,1200,1600,2050,2550,3100,3700,4300,
        4900,5500,6100,6700,7300,7900,8500,9000,9400,9700,9900,10000};
    const auto Noise = [](const double X, const double Y, const float Scale, const FVector2D Offset)
    {
        return FMath::PerlinNoise2D(FVector2D(X / Scale, Y / Scale) + Offset);
    };
    const auto HeightAt = [&](const double X, const double Y)
    {
        const float Distance = FMath::Max(0.0f, FMath::Max(
            FMath::Max(MinX - static_cast<float>(X), static_cast<float>(X) - MaxX),
            FMath::Max(MinY - static_cast<float>(Y), static_cast<float>(Y) - MaxY)));
        // World-space masses and saddles, not a repeated height for each ring.
        // Two scales break the shoulder into broad cleaved crests. The low foot
        // varies in width; both contact rims return smoothly to the substrate.
        const float Mass = Noise(X,Y,2400.0f,FVector2D(17.3,31.7)) +
                           .28f * Noise(X,Y,1100.0f,FVector2D(43.1,9.6));
        const float Crest = FMath::SmoothStep(-.28f,.38f,Mass);
        const float RegionalHeight = 960.0f + 420.0f * Noise(X,Y,5700.0f,FVector2D(8.2,53.4));
        const float FootWidth = 800.0f + 450.0f * Noise(X,Y,2300.0f,FVector2D(21.8,4.6));
        const float Contact = FMath::SmoothStep(0.0f,FootWidth,Distance) *
                              (1.0f - FMath::SmoothStep(8000.0f,10000.0f,Distance));
        return -2.0f + Contact * (120.0f + RegionalHeight * Crest +
            65.0f * Noise(X,Y,1200.0f,FVector2D(61.4,12.7)));
    };
    const auto Point = [&](const int32 BankSide, const int32 Sample, const int32 Ring)
    {
        const float T = static_cast<float>(Sample) / Segments;
        const int32 PerimeterSample = (BankSide * Segments + Sample) % (4 * Segments);
        const float Phase = 2.0f * PI * PerimeterSample / (4 * Segments);
        const float BaseDepth = Depths[Ring];
        // The same bounded warp on every ring keeps their radial order:
        // dDepth/dBaseDepth >= 1 - .32*(1+.4) = .552. Contact rims stay fixed.
        const float Warp = FMath::Sin(Phase*3+.7f) + .4f * FMath::Cos(Phase*7-1.1f);
        const float Depth = BaseDepth + .32f * BaseDepth * (1.0f-BaseDepth/10000.0f) * Warp;
        const float Along = T + .008f * FMath::Sin(T*PI) * FMath::Sin(Phase*5);
        FVector P;
        switch (BankSide)
        {
            case 0: P = FVector(FMath::Lerp(MinX-Depth,MaxX+Depth,Along),MinY-Depth,0); break;
            case 1: P = FVector(MaxX+Depth,FMath::Lerp(MinY-Depth,MaxY+Depth,Along),0); break;
            case 2: P = FVector(FMath::Lerp(MaxX+Depth,MinX-Depth,Along),MaxY+Depth,0); break;
            default: P = FVector(MinX-Depth,FMath::Lerp(MaxY+Depth,MinY-Depth,Along),0); break;
        }
        P.Z = HeightAt(P.X,P.Y);
        return P;
    };
    const auto UV = [](const FVector& P) { return FVector2D(P.X / 200.0f,P.Y / 200.0f); };
    for (int32 Ring = 0; Ring < UE_ARRAY_COUNT(Depths)-1; ++Ring)
        for (int32 Sample = 0; Sample < Segments; ++Sample)
        {
            const FVector A = Point(Side,Sample,Ring), B = Point(Side,Sample+1,Ring);
            const FVector C = Point(Side,Sample,Ring+1), D = Point(Side,Sample+1,Ring+1);
            if ((Sample+Ring)%2 == 0)
                AddQuad(Mesh,A,B,C,D,UV(A),UV(B),UV(C),UV(D),FVector::UpVector);
            else
                AddQuad(Mesh,B,D,A,C,UV(B),UV(D),UV(A),UV(C),FVector::UpVector);
        }
    // Area-weighted normals follow the actual tessellation, including the
    // adjoining bank's triangles at each shared corner. Analytic heightfield
    // gradients can point across an undersampled steep face instead of along it.
    const auto Key = [](const FVector& P)
    {
        return FIntVector(FMath::RoundToInt(P.X*10),FMath::RoundToInt(P.Y*10),FMath::RoundToInt(P.Z*10));
    };
    TMap<FIntVector,FVector> NormalSums;
    const auto Accumulate = [&](const FVector& A, const FVector& B, const FVector& C)
    {
        FVector N = (C-A).Cross(B-A);
        if (N.Z < 0) N = -N;
        for (const FVector& P : {A,B,C}) NormalSums.FindOrAdd(Key(P),FVector::ZeroVector) += N;
    };
    for (int32 I = 0; I < Mesh.Triangles.Num(); I += 3)
        Accumulate(Mesh.Vertices[Mesh.Triangles[I]],Mesh.Vertices[Mesh.Triangles[I+1]],Mesh.Vertices[Mesh.Triangles[I+2]]);
    for (int32 Neighbour = 0; Neighbour < 2; ++Neighbour)
    {
        const int32 BankSide = (Side + (Neighbour == 0 ? 3 : 1)) % 4;
        const int32 Sample = Neighbour == 0 ? Segments-1 : 0;
        for (int32 Ring = 0; Ring < UE_ARRAY_COUNT(Depths)-1; ++Ring)
        {
            const FVector A = Point(BankSide,Sample,Ring), B = Point(BankSide,Sample+1,Ring);
            const FVector C = Point(BankSide,Sample,Ring+1), D = Point(BankSide,Sample+1,Ring+1);
            if ((Sample+Ring)%2 == 0) { Accumulate(A,B,C); Accumulate(C,B,D); }
            else { Accumulate(B,D,A); Accumulate(A,D,C); }
        }
    }
    for (int32 I = 0; I < Mesh.Vertices.Num(); ++I)
        Mesh.Normals[I] = NormalSums.FindChecked(Key(Mesh.Vertices[I])).GetSafeNormal();
    return Mesh;
}
