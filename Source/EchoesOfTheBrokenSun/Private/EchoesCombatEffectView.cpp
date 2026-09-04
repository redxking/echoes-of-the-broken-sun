#include "EchoesCombatEffectView.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
const FName CombatVFXColorParam(TEXT("Color"));
const FName CombatVFXEmissiveParam(TEXT("EmissiveStrength"));
constexpr float MinimumCombatLifetimeSeconds = 0.1f;
constexpr float MaximumCombatLifetimeSeconds = 5.0f;
}

AEchoesCombatEffectView::AEchoesCombatEffectView()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostPhysics;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    BeamMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BeamMesh"));
    MuzzleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MuzzleMesh"));
    ImpactMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ImpactMesh"));
    AfterimageMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AfterimageMesh"));

    for (UStaticMeshComponent* Component : {BeamMesh, MuzzleMesh, ImpactMesh, AfterimageMesh})
    {
        Component->SetupAttachment(SceneRoot);
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Component->SetGenerateOverlapEvents(false);
        Component->SetCanEverAffectNavigation(false);
        Component->SetCastShadow(false);
        Component->SetReceivesDecals(false);
        Component->SetVisibility(false);
    }

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(
        TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> RingFinder(
        TEXT("/Game/Art/Generated/VFX/SM_VFX_DestructionRing.SM_VFX_DestructionRing"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CoreFinder(
        TEXT("/Game/Art/Generated/VFX/SM_VFX_DestructionCore.SM_VFX_DestructionCore"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(
        TEXT("/Game/Art/Generated/Materials/M_EchoesPresentationVFX.M_EchoesPresentationVFX"));

    CylinderMesh = CylinderFinder.Object;
    RingMesh = RingFinder.Object;
    CoreMesh = CoreFinder.Object;
    VFXMaterial = MaterialFinder.Object;

    if (CylinderMesh != nullptr)
    {
        BeamMesh->SetStaticMesh(CylinderMesh);
        AfterimageMesh->SetStaticMesh(CylinderMesh);
    }
    if (CoreMesh != nullptr)
    {
        MuzzleMesh->SetStaticMesh(CoreMesh);
    }
    else if (CylinderMesh != nullptr)
    {
        MuzzleMesh->SetStaticMesh(CylinderMesh);
    }

    if (RingMesh != nullptr)
    {
        ImpactMesh->SetStaticMesh(RingMesh);
    }
    else if (CylinderMesh != nullptr)
    {
        ImpactMesh->SetStaticMesh(CylinderMesh);
    }

    SetActorHiddenInGame(true);
    SetActorTickEnabled(false);
}

void AEchoesCombatEffectView::InitializeCombatEffect(
    echoes::sim::Faction Faction,
    echoes::sim::EntityType EntityType,
    const FVector& InSourceLocation,
    const FVector& InTargetLocation,
    bool bInReducedMotion,
    bool bInReducedFlashing,
    float InLifetimeSeconds)
{
    (void)EntityType;

    SourceLocation = InSourceLocation;
    TargetLocation = InTargetLocation;
    bReducedMotion = bInReducedMotion;
    bReducedFlashing = bInReducedFlashing;
    PresentationLifetimeSeconds = FMath::Clamp(
        InLifetimeSeconds,
        MinimumCombatLifetimeSeconds,
        MaximumCombatLifetimeSeconds);
    RemainingLifetimeSeconds = PresentationLifetimeSeconds;
    bPresentationActive = true;

    // Faction styling per GameCompletionDirective ART-A6 specification
    switch (Faction)
    {
    case echoes::sim::Faction::MeridianCompact:
        BaseColor = FLinearColor(0.12f, 0.88f, 1.0f, 1.0f); // Clean directional cyan
        BaseBeamThickness = 0.06f;
        BaseMuzzleScale = 0.22f;
        BaseImpactScale = 0.32f;
        bAfterimageActive = false;
        break;
    case echoes::sim::Faction::KharuunAssemblies:
        BaseColor = FLinearColor(1.0f, 0.52f, 0.06f, 1.0f); // Incandescent molten amber
        BaseBeamThickness = 0.13f;
        BaseMuzzleScale = 0.30f;
        BaseImpactScale = 0.40f;
        bAfterimageActive = false;
        break;
    case echoes::sim::Faction::HollowChoir:
        BaseColor = FLinearColor(0.85f, 0.22f, 0.95f, 1.0f); // Phase coherent magenta
        BaseBeamThickness = 0.08f;
        BaseMuzzleScale = 0.26f;
        BaseImpactScale = 0.36f;
        bAfterimageActive = !bInReducedMotion; // Dual offset afterimage trace
        break;
    default:
        BaseColor = FLinearColor(0.9f, 0.9f, 0.9f, 1.0f);
        BaseBeamThickness = 0.08f;
        BaseMuzzleScale = 0.25f;
        BaseImpactScale = 0.35f;
        bAfterimageActive = false;
        break;
    }

    InitialEmissiveStrength = bReducedFlashing ? 1.0f : 5.0f;
    CurrentEmissiveStrength = InitialEmissiveStrength;

    // Calculate beam geometry and orientation
    FVector Direction = (TargetLocation - SourceLocation);
    BeamLength = Direction.Size();
    if (BeamLength < 1.0f)
    {
        Direction = FVector::ForwardVector;
        BeamLength = 1.0f;
    }
    const FVector DirNorm = Direction / BeamLength;
    const FVector Midpoint = (SourceLocation + TargetLocation) * 0.5f;
    const FRotator BeamRotation = FRotationMatrix::MakeFromZ(DirNorm).Rotator();

    SetActorLocationAndRotation(
        FVector::ZeroVector,
        FRotator::ZeroRotator,
        false,
        nullptr,
        ETeleportType::TeleportPhysics);

    // Position components in world space via relative coordinates
    BeamMesh->SetWorldLocationAndRotation(Midpoint, BeamRotation);
    BeamMesh->SetRelativeScale3D(FVector(
        BaseBeamThickness,
        BaseBeamThickness,
        BeamLength / 100.0f));
    BeamMesh->SetVisibility(true);

    MuzzleMesh->SetWorldLocationAndRotation(SourceLocation, DirNorm.Rotation());
    MuzzleMesh->SetRelativeScale3D(FVector(BaseMuzzleScale));
    MuzzleMesh->SetVisibility(true);

    ImpactMesh->SetWorldLocationAndRotation(TargetLocation, (-DirNorm).Rotation());
    ImpactMesh->SetRelativeScale3D(FVector(BaseImpactScale));
    ImpactMesh->SetVisibility(true);

    if (bAfterimageActive)
    {
        FVector Perp = FVector::CrossProduct(DirNorm, FVector::UpVector).GetSafeNormal();
        if (Perp.IsNearlyZero())
        {
            Perp = FVector::CrossProduct(DirNorm, FVector::RightVector).GetSafeNormal();
        }
        const FVector AfterimageOffset = Perp * 18.0f; // 18 cm lateral phase displacement
        AfterimageMesh->SetWorldLocationAndRotation(Midpoint + AfterimageOffset, BeamRotation);
        AfterimageMesh->SetRelativeScale3D(FVector(
            BaseBeamThickness * 0.7f,
            BaseBeamThickness * 0.7f,
            BeamLength / 100.0f));
        AfterimageMesh->SetVisibility(true);
    }
    else
    {
        AfterimageMesh->SetVisibility(false);
    }

    ApplyAppearance(BaseColor, CurrentEmissiveStrength);

    SetActorHiddenInGame(false);
    SetActorTickEnabled(true);
}

void AEchoesCombatEffectView::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!bPresentationActive)
    {
        return;
    }

    RemainingLifetimeSeconds -= DeltaSeconds;
    if (RemainingLifetimeSeconds <= 0.0f)
    {
        PrepareForPool();
        return;
    }

    const float NormalizedAge = 1.0f - FMath::Clamp(
        RemainingLifetimeSeconds / PresentationLifetimeSeconds,
        0.0f,
        1.0f);

    CurrentEmissiveStrength = bReducedFlashing
        ? FMath::Lerp(InitialEmissiveStrength, 0.0f, NormalizedAge)
        : FMath::Lerp(InitialEmissiveStrength, 0.0f, NormalizedAge * NormalizedAge);

    ApplyAppearance(BaseColor, CurrentEmissiveStrength);

    if (!bReducedMotion)
    {
        const float CurrentThickness = BaseBeamThickness * FMath::Lerp(1.0f, 0.2f, NormalizedAge);
        const float CurrentMuzzle = BaseMuzzleScale * FMath::Lerp(1.1f, 0.1f, NormalizedAge);
        const float CurrentImpact = BaseImpactScale * FMath::Lerp(0.8f, 1.5f, NormalizedAge);

        BeamMesh->SetRelativeScale3D(FVector(
            CurrentThickness,
            CurrentThickness,
            BeamLength / 100.0f));
        MuzzleMesh->SetRelativeScale3D(FVector(CurrentMuzzle));
        ImpactMesh->SetRelativeScale3D(FVector(CurrentImpact));

        if (bAfterimageActive)
        {
            AfterimageMesh->SetRelativeScale3D(FVector(
                CurrentThickness * 0.7f,
                CurrentThickness * 0.7f,
                BeamLength / 100.0f));
        }
    }
}

void AEchoesCombatEffectView::PrepareForPool()
{
    bPresentationActive = false;
    RemainingLifetimeSeconds = 0.0f;
    bAfterimageActive = false;
    CoalescedOverflowCount = 0;

    BeamMesh->SetVisibility(false);
    MuzzleMesh->SetVisibility(false);
    ImpactMesh->SetVisibility(false);
    AfterimageMesh->SetVisibility(false);

    SetActorHiddenInGame(true);
    SetActorTickEnabled(false);
}

void AEchoesCombatEffectView::RegisterOverflowCoalesced()
{
    if (!bPresentationActive)
    {
        return;
    }
    ++CoalescedOverflowCount;
}

bool AEchoesCombatEffectView::HasCollisionDisabled() const
{
    for (const UStaticMeshComponent* Component : {BeamMesh, MuzzleMesh, ImpactMesh, AfterimageMesh})
    {
        if (Component != nullptr && Component->GetCollisionEnabled() != ECollisionEnabled::NoCollision)
        {
            return false;
        }
    }
    return true;
}

bool AEchoesCombatEffectView::HasNavigationDisabled() const
{
    for (const UStaticMeshComponent* Component : {BeamMesh, MuzzleMesh, ImpactMesh, AfterimageMesh})
    {
        if (Component != nullptr && Component->CanEverAffectNavigation())
        {
            return false;
        }
    }
    return true;
}

bool AEchoesCombatEffectView::HasShadowsDisabled() const
{
    for (const UStaticMeshComponent* Component : {BeamMesh, MuzzleMesh, ImpactMesh, AfterimageMesh})
    {
        if (Component != nullptr && Component->CastShadow)
        {
            return false;
        }
    }
    return true;
}

bool AEchoesCombatEffectView::HasOverlapsDisabled() const
{
    for (const UStaticMeshComponent* Component : {BeamMesh, MuzzleMesh, ImpactMesh, AfterimageMesh})
    {
        if (Component != nullptr && Component->GetGenerateOverlapEvents())
        {
            return false;
        }
    }
    return true;
}

bool AEchoesCombatEffectView::IsUsingAuthoredVFXAssets() const
{
    return VFXMaterial != nullptr;
}

UMaterialInstanceDynamic* AEchoesCombatEffectView::CreateOwnedMaterial()
{
    if (VFXMaterial == nullptr)
    {
        return nullptr;
    }
    UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(VFXMaterial, this);
    if (MID != nullptr)
    {
        ++OwnedMIDCreationCount;
    }
    return MID;
}

void AEchoesCombatEffectView::ApplyAppearance(const FLinearColor& Color, float EmissiveStrength)
{
    if (BeamMaterial == nullptr)
    {
        BeamMaterial = CreateOwnedMaterial();
        if (BeamMaterial != nullptr)
        {
            BeamMesh->SetMaterial(0, BeamMaterial);
        }
    }
    if (MuzzleMaterial == nullptr)
    {
        MuzzleMaterial = CreateOwnedMaterial();
        if (MuzzleMaterial != nullptr)
        {
            MuzzleMesh->SetMaterial(0, MuzzleMaterial);
        }
    }
    if (ImpactMaterial == nullptr)
    {
        ImpactMaterial = CreateOwnedMaterial();
        if (ImpactMaterial != nullptr)
        {
            ImpactMesh->SetMaterial(0, ImpactMaterial);
        }
    }
    if (AfterimageMaterial == nullptr)
    {
        AfterimageMaterial = CreateOwnedMaterial();
        if (AfterimageMaterial != nullptr)
        {
            AfterimageMesh->SetMaterial(0, AfterimageMaterial);
        }
    }

    if (BeamMaterial != nullptr)
    {
        BeamMaterial->SetVectorParameterValue(CombatVFXColorParam, Color);
        BeamMaterial->SetScalarParameterValue(CombatVFXEmissiveParam, EmissiveStrength);
    }
    if (MuzzleMaterial != nullptr)
    {
        MuzzleMaterial->SetVectorParameterValue(CombatVFXColorParam, Color);
        MuzzleMaterial->SetScalarParameterValue(CombatVFXEmissiveParam, EmissiveStrength * 1.25f);
    }
    if (ImpactMaterial != nullptr)
    {
        ImpactMaterial->SetVectorParameterValue(CombatVFXColorParam, Color);
        ImpactMaterial->SetScalarParameterValue(CombatVFXEmissiveParam, EmissiveStrength * 1.5f);
    }
    if (AfterimageMaterial != nullptr)
    {
        const FLinearColor AfterimageColor(Color.R * 0.7f, Color.G * 0.7f, Color.B * 0.9f, Color.A);
        AfterimageMaterial->SetVectorParameterValue(CombatVFXColorParam, AfterimageColor);
        AfterimageMaterial->SetScalarParameterValue(CombatVFXEmissiveParam, EmissiveStrength * 0.6f);
    }
}
