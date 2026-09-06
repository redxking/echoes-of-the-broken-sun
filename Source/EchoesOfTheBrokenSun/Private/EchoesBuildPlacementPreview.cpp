#include "EchoesBuildPlacementPreview.h"

#include "EchoesSimulationSubsystem.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

#include <algorithm>

namespace
{
bool IsConstructable(echoes::sim::EntityType Type)
{
    return Type == echoes::sim::EntityType::Dropoff ||
        Type == echoes::sim::EntityType::Barracks ||
        Type == echoes::sim::EntityType::UtilityStructure;
}

int32 TypeIndex(echoes::sim::EntityType Type)
{
    return static_cast<int32>(Type);
}

int32 FactionIndex(echoes::sim::Faction Faction)
{
    return static_cast<int32>(Faction);
}
}

FEchoesBuildPlacementEvaluation FEchoesBuildPlacementModel::Evaluate(
    const echoes::sim::PlayerView& View,
    echoes::sim::EntityId WorkerId,
    echoes::sim::EntityType BuildingType,
    echoes::sim::Vec2 Position)
{
    FEchoesBuildPlacementEvaluation Result;
    const auto Worker = std::find_if(
        View.Entities().begin(),
        View.Entities().end(),
        [WorkerId](const echoes::sim::Entity& Entity)
        {
            return Entity.id == WorkerId;
        });
    if (Worker == View.Entities().end() ||
        Worker->owner != View.Player().id ||
        Worker->type != echoes::sim::EntityType::Worker ||
        Worker->hitPoints <= 0)
    {
        Result.Validity = EEchoesBuildPreviewValidity::InvalidWorker;
        return Result;
    }
    if (!IsConstructable(BuildingType))
    {
        Result.Validity = EEchoesBuildPreviewValidity::InvalidBuilding;
        return Result;
    }

    const int32 BuildingIndex = TypeIndex(BuildingType);
    const int32 OwnerFactionIndex = FactionIndex(View.Player().faction);
    if (BuildingIndex < 0 ||
        BuildingIndex >= static_cast<int32>(echoes::sim::kConfigurableEntityTypeCount) ||
        OwnerFactionIndex < 0 ||
        OwnerFactionIndex >= static_cast<int32>(echoes::sim::kFactionCount))
    {
        Result.Validity = EEchoesBuildPreviewValidity::InvalidBuilding;
        return Result;
    }
    const echoes::sim::EntityArchetypeRules& BuildingRules =
        View.Config().rules.archetypes[OwnerFactionIndex][BuildingIndex];
    Result.FootprintHalfExtentRaw = BuildingRules.footprintHalfExtentRaw;
    const int64 X = Position.x.Raw();
    const int64 Y = Position.y.Raw();
    const int64 MapWidthRaw =
        static_cast<int64>(View.Config().mapWidthTiles) * echoes::sim::kFixedScale;
    const int64 MapHeightRaw =
        static_cast<int64>(View.Config().mapHeightTiles) * echoes::sim::kFixedScale;
    if (X - Result.FootprintHalfExtentRaw < 0 ||
        Y - Result.FootprintHalfExtentRaw < 0 ||
        X + Result.FootprintHalfExtentRaw >= MapWidthRaw ||
        Y + Result.FootprintHalfExtentRaw >= MapHeightRaw)
    {
        Result.Validity = EEchoesBuildPreviewValidity::OutsideMap;
        return Result;
    }

    const int32 MinimumTileX =
        (Position.x.Raw() - Result.FootprintHalfExtentRaw) /
        echoes::sim::kFixedScale;
    const int32 MinimumTileY =
        (Position.y.Raw() - Result.FootprintHalfExtentRaw) /
        echoes::sim::kFixedScale;
    const int32 MaximumTileX =
        (Position.x.Raw() + Result.FootprintHalfExtentRaw - 1) /
        echoes::sim::kFixedScale;
    const int32 MaximumTileY =
        (Position.y.Raw() + Result.FootprintHalfExtentRaw - 1) /
        echoes::sim::kFixedScale;
    for (int32 TileY = MinimumTileY; TileY <= MaximumTileY; ++TileY)
    {
        for (int32 TileX = MinimumTileX; TileX <= MaximumTileX; ++TileX)
        {
            const echoes::sim::Vec2 Tile =
                echoes::sim::Vec2::FromTiles(TileX, TileY);
            if (View.VisibilityAt(Tile) != echoes::sim::Visibility::Visible)
            {
                Result.Validity = EEchoesBuildPreviewValidity::UnknownTerrain;
                return Result;
            }
            if (View.TerrainAt(TileX, TileY) != echoes::sim::Terrain::Open)
            {
                Result.Validity = EEchoesBuildPreviewValidity::TerrainBlocked;
                return Result;
            }
        }
    }

    for (const echoes::sim::Entity& Entity : View.Entities())
    {
        const int32 EntityFactionIndex = FactionIndex(Entity.faction);
        const int32 EntityTypeIndex = TypeIndex(Entity.type);
        if (Entity.hitPoints <= 0 || EntityFactionIndex < 0 ||
            EntityFactionIndex >= static_cast<int32>(echoes::sim::kFactionCount) ||
            EntityTypeIndex < 0 ||
            EntityTypeIndex >= static_cast<int32>(echoes::sim::kConfigurableEntityTypeCount))
        {
            continue;
        }
        const int32 CombinedExtent = Result.FootprintHalfExtentRaw +
            View.Config().rules.archetypes[EntityFactionIndex][EntityTypeIndex]
                .footprintHalfExtentRaw;
        if (FMath::Abs(static_cast<int64>(Position.x.Raw()) - Entity.position.x.Raw()) <
                CombinedExtent &&
            FMath::Abs(static_cast<int64>(Position.y.Raw()) - Entity.position.y.Raw()) <
                CombinedExtent)
        {
            Result.Validity = EEchoesBuildPreviewValidity::Occupied;
            return Result;
        }
    }

    if (View.Player().resources.material < BuildingRules.cost.material ||
        View.Player().resources.dawnshards < BuildingRules.cost.dawnshards)
    {
        Result.Validity = EEchoesBuildPreviewValidity::InsufficientResources;
        return Result;
    }
    Result.Validity = EEchoesBuildPreviewValidity::Valid;
    return Result;
}

const TCHAR* FEchoesBuildPlacementModel::Feedback(
    EEchoesBuildPreviewValidity Validity)
{
    switch (Validity)
    {
        case EEchoesBuildPreviewValidity::Valid:
            return TEXT("Placement valid. Left-click to confirm; right-click cancels.");
        case EEchoesBuildPreviewValidity::InvalidWorker:
            return TEXT("[BUILD_REQUIRES_WORKER] Select an owned worker.");
        case EEchoesBuildPreviewValidity::InvalidBuilding:
            return TEXT("[BUILD_TYPE_INVALID] This structure cannot be placed.");
        case EEchoesBuildPreviewValidity::OutsideMap:
            return TEXT("[FOOTPRINT_BLOCKED] Move the full footprint inside the battlefield.");
        case EEchoesBuildPreviewValidity::UnknownTerrain:
            return TEXT("[FOOTPRINT_UNKNOWN] Scout the full footprint before placing it.");
        case EEchoesBuildPreviewValidity::TerrainBlocked:
            return TEXT("[FOOTPRINT_BLOCKED] Choose visible, passable, unscarred ground.");
        case EEchoesBuildPreviewValidity::Occupied:
            return TEXT("[FOOTPRINT_BLOCKED] Move clear of units and structures.");
        case EEchoesBuildPreviewValidity::InsufficientResources:
            return TEXT("[INSUFFICIENT_RESOURCES] Accumulate the required Matter and Dawnshards.");
        default:
            return TEXT("[BUILD_PLACEMENT_INVALID] Choose another location.");
    }
}

AEchoesBuildPlacementPreview::AEchoesBuildPlacementPreview()
{
    PrimaryActorTick.bCanEverTick = false;
    SetCanBeDamaged(false);
    Grid = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("PlacementGrid"));
    SetRootComponent(Grid);
    Grid->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Grid->SetCastShadow(false);
    Grid->bUseAsyncCooking = false;
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(
        TEXT("/Game/Art/Generated/Materials/M_EchoesPresentationVFX.M_EchoesPresentationVFX"));
    PreviewMaterial = MaterialFinder.Object;
}

void AEchoesBuildPlacementPreview::SetPreview(
    const FVector& WorldPosition,
    int32 FootprintHalfExtentRaw,
    bool bValid)
{
    FVector Grounded = WorldPosition;
    Grounded.Z += 7.0f;
    SetActorLocation(Grounded);
    SetActorHiddenInGame(false);
    if (DisplayedHalfExtentRaw != FootprintHalfExtentRaw ||
        bDisplayedValid != bValid)
    {
        RebuildGrid(FootprintHalfExtentRaw, bValid);
        DisplayedHalfExtentRaw = FootprintHalfExtentRaw;
        bDisplayedValid = bValid;
    }
}

void AEchoesBuildPlacementPreview::RebuildGrid(
    int32 FootprintHalfExtentRaw,
    bool bValid)
{
    const int32 DiameterRaw = FMath::Max(
        echoes::sim::kFixedScale,
        FootprintHalfExtentRaw * 2);
    const int32 CellCount = FMath::Max(
        1,
        FMath::DivideAndRoundUp(DiameterRaw, echoes::sim::kFixedScale));
    const float DiameterWorld =
        static_cast<float>(DiameterRaw) /
        static_cast<float>(echoes::sim::kFixedScale) *
        UEchoesSimulationSubsystem::TileWorldSize;
    const float CellWorld = DiameterWorld / static_cast<float>(CellCount);
    const float Half = DiameterWorld * 0.5f;
    const float Inset = FMath::Min(5.0f, CellWorld * 0.06f);
    const FLinearColor Color = bValid
        ? FLinearColor(0.08f, 0.72f, 1.0f, 0.72f)
        : FLinearColor(1.0f, 0.08f, 0.05f, 0.78f);
    if (GridMaterial == nullptr && PreviewMaterial != nullptr)
    {
        GridMaterial = UMaterialInstanceDynamic::Create(PreviewMaterial, this);
        Grid->SetMaterial(0, GridMaterial);
    }
    if (GridMaterial != nullptr)
    {
        GridMaterial->SetVectorParameterValue(TEXT("BaseColor"), Color);
        GridMaterial->SetScalarParameterValue(
            TEXT("EmissiveStrength"), bValid ? 1.65f : 2.1f);
    }

    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
    TArray<FLinearColor> Colors;
    TArray<FProcMeshTangent> Tangents;
    Vertices.Reserve(CellCount * CellCount * 4);
    Triangles.Reserve(CellCount * CellCount * 6);
    for (int32 Y = 0; Y < CellCount; ++Y)
    {
        for (int32 X = 0; X < CellCount; ++X)
        {
            const float MinX = -Half + X * CellWorld + Inset;
            const float MinY = -Half + Y * CellWorld + Inset;
            const float MaxX = -Half + (X + 1) * CellWorld - Inset;
            const float MaxY = -Half + (Y + 1) * CellWorld - Inset;
            const int32 Base = Vertices.Num();
            Vertices.Append({{MinX, MinY, 0}, {MaxX, MinY, 0},
                             {MaxX, MaxY, 0}, {MinX, MaxY, 0}});
            Triangles.Append({Base, Base + 1, Base + 2,
                              Base, Base + 2, Base + 3});
            Normals.Append({FVector::UpVector, FVector::UpVector,
                            FVector::UpVector, FVector::UpVector});
            UVs.Append({{0, 0}, {1, 0}, {1, 1}, {0, 1}});
            Colors.Append({Color, Color, Color, Color});
            Tangents.Append({FProcMeshTangent(1, 0, 0), FProcMeshTangent(1, 0, 0),
                             FProcMeshTangent(1, 0, 0), FProcMeshTangent(1, 0, 0)});
        }
    }
    Grid->CreateMeshSection_LinearColor(
        0, Vertices, Triangles, Normals, UVs, Colors, Tangents, false);
}
