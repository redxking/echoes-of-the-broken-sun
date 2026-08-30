#include "EchoesDestructionView.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
const FName DestructionColorParameterName(TEXT("Color"));
const FName DestructionEmissiveParameterName(TEXT("EmissiveStrength"));
constexpr float MinimumDestructionLifetimeSeconds = 0.5f;
constexpr float MaximumDestructionLifetimeSeconds = 30.0f;
}

AEchoesDestructionView::AEchoesDestructionView()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostPhysics;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);
    ShockRing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShockRing"));
    CoreEmber = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CoreEmber"));
    ShardA = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShardA"));
    ShardB = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShardB"));

    for (UStaticMeshComponent* Component : {ShockRing, CoreEmber, ShardA, ShardB})
    {
        Component->SetupAttachment(SceneRoot);
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Component->SetGenerateOverlapEvents(false);
        Component->SetCanEverAffectNavigation(false);
        Component->SetCastShadow(false);
        Component->SetReceivesDecals(false);
    }

    static ConstructorHelpers::FObjectFinder<UStaticMesh> RingFinder(
        TEXT("/Game/Art/Generated/VFX/SM_VFX_DestructionRing.SM_VFX_DestructionRing"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CoreFinder(
        TEXT("/Game/Art/Generated/VFX/SM_VFX_DestructionCore.SM_VFX_DestructionCore"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> ShardFinder(
        TEXT("/Game/Art/Generated/VFX/SM_VFX_DestructionShard.SM_VFX_DestructionShard"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(
        TEXT("/Game/Art/Generated/Materials/M_EchoesPresentationVFX.M_EchoesPresentationVFX"));
    RingMesh = RingFinder.Object;
    CoreMesh = CoreFinder.Object;
    ShardMesh = ShardFinder.Object;
    VFXMaterial = MaterialFinder.Object;

    ShockRing->SetStaticMesh(RingMesh);
    CoreEmber->SetStaticMesh(CoreMesh);
    ShardA->SetStaticMesh(ShardMesh);
    ShardB->SetStaticMesh(ShardMesh);
    ShockRing->SetRelativeLocation(FVector(0.0f, 0.0f, 5.0f));
    CoreEmber->SetRelativeLocation(FVector(0.0f, 0.0f, 12.0f));
    ShardA->SetRelativeLocation(FVector(38.0f, 8.0f, 16.0f));
    ShardB->SetRelativeLocation(FVector(-34.0f, -12.0f, 19.0f));
    ShardB->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
    Tags.Add(TEXT("EchoesDestructionView"));
}

void AEchoesDestructionView::InitializeDestruction(
    echoes::sim::Faction Faction,
    echoes::sim::EntityType EntityType,
    bool bInReducedMotion,
    bool bInReducedFlashing,
    float InLifetimeSeconds)
{
    bReducedMotion = bInReducedMotion;
    bReducedFlashing = bInReducedFlashing;
    ElapsedSeconds = 0.0f;
    PresentationLifetimeSeconds = FMath::Clamp(
        InLifetimeSeconds,
        MinimumDestructionLifetimeSeconds,
        MaximumDestructionLifetimeSeconds);
    switch (Faction)
    {
        case echoes::sim::Faction::MeridianCompact:
            BaseColor = FLinearColor(0.05f, 0.92f, 1.0f);
            break;
        case echoes::sim::Faction::KharuunAssemblies:
            BaseColor = FLinearColor(1.0f, 0.48f, 0.08f);
            break;
        case echoes::sim::Faction::HollowChoir:
            BaseColor = FLinearColor(0.851f, 0.412f, 0.553f);
            break;
    }

    float SizeScale = 1.0f;
    switch (EntityType)
    {
        case echoes::sim::EntityType::CommandCore:
            SizeScale = 1.7f;
            break;
        case echoes::sim::EntityType::Dropoff:
        case echoes::sim::EntityType::Barracks:
        case echoes::sim::EntityType::UtilityStructure:
            SizeScale = 1.45f;
            break;
        case echoes::sim::EntityType::HeavyUnit:
            SizeScale = 1.2f;
            break;
        default:
            break;
    }
    SetActorScale3D(FVector(SizeScale));
    ShockRing->SetRelativeScale3D(FVector(0.72f, 0.72f, 1.0f));
    CoreEmber->SetRelativeScale3D(FVector(1.0f));
    ShardA->SetRelativeLocation(FVector(38.0f, 8.0f, 16.0f));
    ShardB->SetRelativeLocation(FVector(-34.0f, -12.0f, 19.0f));
    BaseRingScale = ShockRing->GetRelativeScale3D();
    BaseCoreScale = CoreEmber->GetRelativeScale3D();
    BaseShardALocation = ShardA->GetRelativeLocation();
    BaseShardBLocation = ShardB->GetRelativeLocation();

    RingMaterial = UMaterialInstanceDynamic::Create(VFXMaterial, this);
    CoreMaterial = UMaterialInstanceDynamic::Create(VFXMaterial, this);
    ShardAMaterial = UMaterialInstanceDynamic::Create(VFXMaterial, this);
    ShardBMaterial = UMaterialInstanceDynamic::Create(VFXMaterial, this);
    ShockRing->SetMaterial(0, RingMaterial);
    CoreEmber->SetMaterial(0, CoreMaterial);
    ShardA->SetMaterial(0, ShardAMaterial);
    ShardB->SetMaterial(0, ShardBMaterial);
    ApplyAppearance(BaseColor, bReducedFlashing ? 1.15f : 3.6f);
}

void AEchoesDestructionView::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    ElapsedSeconds += DeltaSeconds;
    if (ElapsedSeconds >= PresentationLifetimeSeconds)
    {
        Destroy();
        return;
    }

    const float Alpha = FMath::Clamp(
        ElapsedSeconds / PresentationLifetimeSeconds,
        0.0f,
        1.0f);
    if (!bReducedMotion)
    {
        const float EaseOut = 1.0f - FMath::Pow(1.0f - Alpha, 3.0f);
        ShockRing->SetRelativeScale3D(
            BaseRingScale * FMath::Lerp(1.0f, 2.15f, EaseOut));
        CoreEmber->SetRelativeScale3D(
            BaseCoreScale * FMath::Lerp(1.0f, 0.32f, Alpha));
        ShardA->SetRelativeLocation(
            BaseShardALocation + FVector(72.0f, 28.0f, 46.0f) * EaseOut);
        ShardB->SetRelativeLocation(
            BaseShardBLocation + FVector(-64.0f, -34.0f, 52.0f) * EaseOut);
        ShardA->SetRelativeRotation(FRotator(0.0f, Alpha * 150.0f, Alpha * 34.0f));
        ShardB->SetRelativeRotation(
            FRotator(0.0f, 180.0f - Alpha * 132.0f, -Alpha * 28.0f));
    }
    else
    {
        ShockRing->SetRelativeScale3D(BaseRingScale);
        CoreEmber->SetRelativeScale3D(BaseCoreScale);
        ShardA->SetRelativeLocation(BaseShardALocation);
        ShardB->SetRelativeLocation(BaseShardBLocation);
    }

    const float EmissiveStrength = bReducedFlashing
                                       ? 1.15f
                                       : FMath::Lerp(3.6f, 1.25f, Alpha);
    ApplyAppearance(BaseColor, EmissiveStrength);
}

void AEchoesDestructionView::ApplyAppearance(
    const FLinearColor& Color,
    float EmissiveStrength)
{
    CurrentEmissiveStrength = EmissiveStrength;
    for (UMaterialInstanceDynamic* Material :
         {RingMaterial, CoreMaterial, ShardAMaterial, ShardBMaterial})
    {
        if (Material != nullptr)
        {
            Material->SetVectorParameterValue(DestructionColorParameterName, Color);
            Material->SetScalarParameterValue(
                DestructionEmissiveParameterName,
                EmissiveStrength);
        }
    }
}

bool AEchoesDestructionView::HasCollisionDisabled() const
{
    for (const UStaticMeshComponent* Component : {ShockRing, CoreEmber, ShardA, ShardB})
    {
        if (Component == nullptr ||
            Component->GetCollisionEnabled() != ECollisionEnabled::NoCollision ||
            Component->GetGenerateOverlapEvents())
        {
            return false;
        }
    }
    return true;
}

bool AEchoesDestructionView::HasNavigationDisabled() const
{
    for (const UStaticMeshComponent* Component : {ShockRing, CoreEmber, ShardA, ShardB})
    {
        if (Component == nullptr || Component->CanEverAffectNavigation())
        {
            return false;
        }
    }
    return true;
}

bool AEchoesDestructionView::IsUsingAuthoredVFXAssets() const
{
    const auto IsAuthoredMesh = [](const UStaticMeshComponent* Component)
    {
        return Component != nullptr && Component->GetStaticMesh() != nullptr &&
               Component->GetStaticMesh()->GetPathName().StartsWith(
                   TEXT("/Game/Art/Generated/VFX/SM_VFX_Destruction"));
    };
    return IsAuthoredMesh(ShockRing) && IsAuthoredMesh(CoreEmber) &&
           IsAuthoredMesh(ShardA) && IsAuthoredMesh(ShardB) &&
           VFXMaterial != nullptr;
}

FVector AEchoesDestructionView::GetRingScale() const
{
    return ShockRing != nullptr
               ? ShockRing->GetRelativeScale3D()
               : FVector::ZeroVector;
}

FVector AEchoesDestructionView::GetShardALocation() const
{
    return ShardA != nullptr
               ? ShardA->GetRelativeLocation()
               : FVector::ZeroVector;
}
