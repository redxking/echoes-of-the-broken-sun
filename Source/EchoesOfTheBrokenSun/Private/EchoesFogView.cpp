#include "EchoesFogView.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "EchoesSimCore/NetworkProtocol.h"
#include "EchoesGameUserSettings.h"
#include "HAL/PlatformTime.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
const FName FogColorParameterName(TEXT("Color"));

// /Engine/BasicShapes/Cube.Cube is a 100 uu centre-origin cube, so a component
// scale of 1.0 spans 100 world units.
constexpr float BasicCubeSize = 100.0f;

// The unexplored shroud has to occlude every silhouette a tile can raise, not
// merely tint the ground plate under it. AEchoesTerrainView instances
// SM_World_GlassScarRidge with a Z scale of 0.62 lifted 8 uu; that authored
// tooth reaches 244 uu in mesh space (Scripts/generate_art_assets.py,
// world_glass_scar_ridge: its tallest cone rises 196 uu from a 48 uu base) and
// bottoms out at -9 uu, so a blocked tile occupies roughly 2..160 uu of world
// height. The shelf used for scarred tiles stays under 7 uu. These bounds give
// the shroud margin at both ends; raise the top if a taller authored tile mesh
// is ever instanced by the terrain view.
constexpr float UnexploredShroudBottom = -16.0f;
constexpr float UnexploredShroudTop = 184.0f;
// M01 has an authored bed at -1750. Conceal unknown space all the way to
// that substrate so an oblique camera cannot look under the shroud into sky.
constexpr float M01UnexploredShroudBottom = -1766.0f;

// Explored tiles must keep showing remembered terrain, so that layer stays the
// thin dimming slab it has always been and deliberately does not occlude.
constexpr float ExploredShroudCentre = 14.0f;
constexpr float ExploredShroudHeight = 6.0f;
}

AEchoesFogView::AEchoesFogView()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    UnexploredTiles = CreateDefaultSubobject<UInstancedStaticMeshComponent>(
        TEXT("UnexploredTiles"));
    UnexploredTiles->SetupAttachment(SceneRoot);
    ExploredTiles = CreateDefaultSubobject<UInstancedStaticMeshComponent>(
        TEXT("ExploredTiles"));
    ExploredTiles->SetupAttachment(SceneRoot);

    for (UInstancedStaticMeshComponent* Layer : {UnexploredTiles, ExploredTiles})
    {
        Layer->SetMobility(EComponentMobility::Movable);
        Layer->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Layer->SetCanEverAffectNavigation(false);
        Layer->SetGenerateOverlapEvents(false);
        Layer->SetCastShadow(false);
        Layer->SetReceivesDecals(false);
    }

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(
        TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(
        TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    CubeMesh = CubeFinder.Object;
    BasicMaterial = MaterialFinder.Object;
    Tags.Add(TEXT("EchoesFogView"));
}

FTransform AEchoesFogView::TileTransform(
    int32 TileX,
    int32 TileY,
    bool bUnexplored) const
{
    const float HalfWidth = static_cast<float>(MapWidthTiles) * 0.5f;
    const float HalfHeight = static_cast<float>(MapHeightTiles) * 0.5f;
    const float Bottom = bUseM01Material ? M01UnexploredShroudBottom : UnexploredShroudBottom;
    const float Height =
        bUnexplored ? UnexploredShroudTop - Bottom
                    : ExploredShroudHeight;
    const float Centre = bUnexplored
        ? Bottom + Height * 0.5f
        : ExploredShroudCentre;
    return FTransform(
        FRotator::ZeroRotator,
        FVector(
            (static_cast<float>(TileX) - HalfWidth) * WorldUnitsPerTile,
            (static_cast<float>(TileY) - HalfHeight) * WorldUnitsPerTile,
            Centre),
        FVector(
            WorldUnitsPerTile / BasicCubeSize,
            WorldUnitsPerTile / BasicCubeSize,
            Height / BasicCubeSize));
}

FTransform AEchoesFogView::HiddenTransform()
{
    return FTransform(
        FRotator::ZeroRotator,
        FVector::ZeroVector,
        FVector::ZeroVector);
}

void AEchoesFogView::ApplyMaterials()
{
    if (UnexploredMaterial == nullptr || ExploredMaterial == nullptr)
    {
        return;
    }

    const float EffectiveBleedStrength = bReducedFlashing ? 0.05f : UnexploredBleedStrength;
    const FLinearColor CompositeUnexplored =
        UnexploredBaseColor + (UnexploredBleedColor * EffectiveBleedStrength);

    UnexploredMaterial->SetVectorParameterValue(FogColorParameterName, CompositeUnexplored);
    ExploredMaterial->SetVectorParameterValue(FogColorParameterName, ExploredColor);
}

void AEchoesFogView::UpdateAccessibilitySettings(bool bInReducedMotion, bool bInReducedFlashing)
{
    bReducedMotion = bInReducedMotion;
    bReducedFlashing = bInReducedFlashing;
    ApplyMaterials();
}

bool AEchoesFogView::HasCollisionDisabled() const
{
    for (const UInstancedStaticMeshComponent* Layer : {UnexploredTiles.Get(), ExploredTiles.Get()})
    {
        if (Layer != nullptr && Layer->GetCollisionEnabled() != ECollisionEnabled::NoCollision)
        {
            return false;
        }
    }
    return true;
}

bool AEchoesFogView::HasNavigationDisabled() const
{
    for (const UInstancedStaticMeshComponent* Layer : {UnexploredTiles.Get(), ExploredTiles.Get()})
    {
        if (Layer != nullptr && Layer->CanEverAffectNavigation())
        {
            return false;
        }
    }
    return true;
}

bool AEchoesFogView::HasShadowsDisabled() const
{
    for (const UInstancedStaticMeshComponent* Layer : {UnexploredTiles.Get(), ExploredTiles.Get()})
    {
        if (Layer != nullptr && Layer->CastShadow)
        {
            return false;
        }
    }
    return true;
}

bool AEchoesFogView::HasOverlapsDisabled() const
{
    for (const UInstancedStaticMeshComponent* Layer : {UnexploredTiles.Get(), ExploredTiles.Get()})
    {
        if (Layer != nullptr && Layer->GetGenerateOverlapEvents())
        {
            return false;
        }
    }
    return true;
}

bool AEchoesFogView::InitializeFog(
    const echoes::sim::Simulation& Simulation,
    echoes::sim::PlayerId Player,
    float TileWorldSize,
    bool bM01Material)
{
    if (Simulation.FindPlayer(Player) == nullptr ||
        !InitializeScopedFog(
            Simulation.Config().mapWidthTiles,
            Simulation.Config().mapHeightTiles,
            TileWorldSize, bM01Material))
    {
        return false;
    }

    PlayerId = Player;
    return SyncVisibility(Simulation);
}

bool AEchoesFogView::InitializeScopedFog(
    int32 InMapWidthTiles,
    int32 InMapHeightTiles,
    float TileWorldSize,
    bool bM01Material)
{
    if (CubeMesh == nullptr || BasicMaterial == nullptr ||
        InMapWidthTiles <= 0 || InMapHeightTiles <= 0 ||
        InMapWidthTiles > 256 || InMapHeightTiles > 256 ||
        TileWorldSize <= 0.0f)
    {
        return false;
    }
    bUseM01Material = bM01Material;
    MapWidthTiles = InMapWidthTiles;
    MapHeightTiles = InMapHeightTiles;
    WorldUnitsPerTile = TileWorldSize;
    const int32 TileCount = MapWidthTiles * MapHeightTiles;
    CachedVisibility.Init(
        static_cast<uint8>(echoes::sim::Visibility::Unexplored),
        TileCount);
    UnexploredTiles->SetStaticMesh(CubeMesh);
    ExploredTiles->SetStaticMesh(CubeMesh);
    UMaterialInterface* UnexploredParent = bUseM01Material
        ? LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Game/Art/Generated/Materials/M_EchoesM01Shroud"))
        : BasicMaterial.Get();
    if (UnexploredParent == nullptr) return false;
    UnexploredMaterial = UMaterialInstanceDynamic::Create(UnexploredParent, this);
    UMaterialInterface* ExploredParent = bUseM01Material
        ? LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Game/Art/Generated/Materials/M_EchoesM01Explored"))
        : BasicMaterial.Get();
    if (ExploredParent == nullptr) return false;
    ExploredMaterial = UMaterialInstanceDynamic::Create(ExploredParent, this);
    if (UnexploredMaterial == nullptr || ExploredMaterial == nullptr)
    {
        return false;
    }
    if (const UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get())
    {
        bReducedMotion = Settings->IsReducedMotionEnabled();
        bReducedFlashing = Settings->IsReducedFlashingEnabled();
    }
    ApplyMaterials();
    UnexploredTiles->SetMaterial(0, UnexploredMaterial);
    ExploredTiles->SetMaterial(0, ExploredMaterial);
    UnexploredTiles->ClearInstances();
    ExploredTiles->ClearInstances();
    UnexploredTiles->PreAllocateInstancesMemory(TileCount);
    ExploredTiles->PreAllocateInstancesMemory(TileCount);
    const FTransform Hidden = HiddenTransform();
    for (int32 TileIndex = 0; TileIndex < TileCount; ++TileIndex)
    {
        const int32 TileX = TileIndex % MapWidthTiles;
        const int32 TileY = TileIndex / MapWidthTiles;
        UnexploredTiles->AddInstance(TileTransform(TileX, TileY, true), false);
        ExploredTiles->AddInstance(Hidden, false);
    }
    return true;
}

bool AEchoesFogView::SyncScopedVisibility(
    const std::vector<echoes::sim::net::ScopedTileState>& Tiles)
{
    if (Tiles.size() != static_cast<std::size_t>(
            MapWidthTiles * MapHeightTiles) ||
        CachedVisibility.Num() != MapWidthTiles * MapHeightTiles ||
        UnexploredTiles->GetInstanceCount() != CachedVisibility.Num() ||
        ExploredTiles->GetInstanceCount() != CachedVisibility.Num())
    {
        return false;
    }
    const double StartSeconds = FPlatformTime::Seconds();
    UnexploredTileCount = 0;
    ExploredTileCount = 0;
    VisibleTileCount = 0;
    bool bChanged = false;
    const FTransform Hidden = HiddenTransform();
    for (int32 TileY = 0; TileY < MapHeightTiles; ++TileY)
    {
        for (int32 TileX = 0; TileX < MapWidthTiles; ++TileX)
        {
            const int32 TileIndex = TileY * MapWidthTiles + TileX;
            const echoes::sim::Visibility Visibility =
                Tiles[static_cast<std::size_t>(TileIndex)].visibility;
            const uint8 Encoded = static_cast<uint8>(Visibility);
            UnexploredTileCount +=
                Visibility == echoes::sim::Visibility::Unexplored ? 1 : 0;
            ExploredTileCount +=
                Visibility == echoes::sim::Visibility::Explored ? 1 : 0;
            VisibleTileCount +=
                Visibility == echoes::sim::Visibility::Visible ? 1 : 0;
            if (CachedVisibility[TileIndex] == Encoded)
            {
                continue;
            }
            CachedVisibility[TileIndex] = Encoded;
            UnexploredTiles->UpdateInstanceTransform(
                TileIndex,
                Visibility == echoes::sim::Visibility::Unexplored
                    ? TileTransform(TileX, TileY, true)
                    : Hidden,
                false,
                false,
                true);
            ExploredTiles->UpdateInstanceTransform(
                TileIndex,
                Visibility == echoes::sim::Visibility::Explored
                    ? TileTransform(TileX, TileY, false)
                    : Hidden,
                false,
                false,
                true);
            bChanged = true;
        }
    }
    if (bChanged)
    {
        UnexploredTiles->MarkRenderStateDirty();
        ExploredTiles->MarkRenderStateDirty();
    }
    LastSyncDurationMs = (FPlatformTime::Seconds() - StartSeconds) * 1000.0;
    ++TotalSyncCount;
    return true;
}

bool AEchoesFogView::SyncVisibility(
    const echoes::sim::Simulation& Simulation)
{
    if (Simulation.Config().mapWidthTiles != MapWidthTiles ||
        Simulation.Config().mapHeightTiles != MapHeightTiles ||
        CachedVisibility.Num() != MapWidthTiles * MapHeightTiles ||
        UnexploredTiles->GetInstanceCount() != CachedVisibility.Num() ||
        ExploredTiles->GetInstanceCount() != CachedVisibility.Num())
    {
        return false;
    }

    const double StartSeconds = FPlatformTime::Seconds();
    UnexploredTileCount = 0;
    ExploredTileCount = 0;
    VisibleTileCount = 0;
    bool bChanged = false;
    const FTransform Hidden = HiddenTransform();
    for (int32 TileY = 0; TileY < MapHeightTiles; ++TileY)
    {
        for (int32 TileX = 0; TileX < MapWidthTiles; ++TileX)
        {
            const int32 TileIndex = TileY * MapWidthTiles + TileX;
            const echoes::sim::Visibility Visibility = Simulation.VisibilityAt(
                PlayerId,
                echoes::sim::Vec2::FromTiles(TileX, TileY));
            const uint8 Encoded = static_cast<uint8>(Visibility);
            UnexploredTileCount +=
                Visibility == echoes::sim::Visibility::Unexplored ? 1 : 0;
            ExploredTileCount +=
                Visibility == echoes::sim::Visibility::Explored ? 1 : 0;
            VisibleTileCount +=
                Visibility == echoes::sim::Visibility::Visible ? 1 : 0;
            if (CachedVisibility[TileIndex] == Encoded)
            {
                continue;
            }
            CachedVisibility[TileIndex] = Encoded;
            UnexploredTiles->UpdateInstanceTransform(
                TileIndex,
                Visibility == echoes::sim::Visibility::Unexplored
                    ? TileTransform(TileX, TileY, true)
                    : Hidden,
                false,
                false,
                true);
            ExploredTiles->UpdateInstanceTransform(
                TileIndex,
                Visibility == echoes::sim::Visibility::Explored
                    ? TileTransform(TileX, TileY, false)
                    : Hidden,
                false,
                false,
                true);
            bChanged = true;
        }
    }
    if (bChanged)
    {
        UnexploredTiles->MarkRenderStateDirty();
        ExploredTiles->MarkRenderStateDirty();
    }
    LastSyncDurationMs = (FPlatformTime::Seconds() - StartSeconds) * 1000.0;
    ++TotalSyncCount;
    return true;
}
