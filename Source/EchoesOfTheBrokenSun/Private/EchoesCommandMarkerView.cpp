#include "EchoesCommandMarkerView.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
const FName MarkerColorParameterName(TEXT("Color"));
const FName MarkerEmissiveParameterName(TEXT("EmissiveStrength"));
constexpr float MinimumMarkerLifetimeSeconds = 0.4f;
constexpr float MaximumMarkerLifetimeSeconds = 30.0f;
}

AEchoesCommandMarkerView::AEchoesCommandMarkerView()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostPhysics;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    MarkerDisc = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MarkerDisc"));
    MarkerDisc->SetupAttachment(SceneRoot);
    GlyphA = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GlyphA"));
    GlyphA->SetupAttachment(SceneRoot);
    GlyphB = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GlyphB"));
    GlyphB->SetupAttachment(SceneRoot);

    for (UStaticMeshComponent* Component : {MarkerDisc, GlyphA, GlyphB})
    {
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Component->SetGenerateOverlapEvents(false);
        Component->SetCastShadow(false);
        Component->SetReceivesDecals(false);
        Component->SetCanEverAffectNavigation(false);
    }

    static ConstructorHelpers::FObjectFinder<UStaticMesh> MoveFinder(
        TEXT("/Game/Art/Generated/VFX/SM_VFX_CommandMove.SM_VFX_CommandMove"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> AttackMoveFinder(
        TEXT("/Game/Art/Generated/VFX/SM_VFX_CommandAttackMove.SM_VFX_CommandAttackMove"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> PatrolFinder(
        TEXT("/Game/Art/Generated/VFX/SM_VFX_CommandPatrol.SM_VFX_CommandPatrol"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> GuardFinder(
        TEXT("/Game/Art/Generated/VFX/SM_VFX_CommandGuard.SM_VFX_CommandGuard"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> BuildFinder(
        TEXT("/Game/Art/Generated/VFX/SM_VFX_CommandBuild.SM_VFX_CommandBuild"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> InteractFinder(
        TEXT("/Game/Art/Generated/VFX/SM_VFX_CommandInteract.SM_VFX_CommandInteract"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> OrbitFinder(
        TEXT("/Game/Art/Generated/VFX/SM_VFX_CommandOrbit.SM_VFX_CommandOrbit"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(
        TEXT("/Game/Art/Generated/Materials/M_EchoesPresentationVFX.M_EchoesPresentationVFX"));
    MoveMesh = MoveFinder.Object;
    AttackMoveMesh = AttackMoveFinder.Object;
    PatrolMesh = PatrolFinder.Object;
    GuardMesh = GuardFinder.Object;
    BuildMesh = BuildFinder.Object;
    InteractMesh = InteractFinder.Object;
    OrbitMesh = OrbitFinder.Object;
    VFXMaterial = MaterialFinder.Object;

    GlyphA->SetStaticMesh(OrbitMesh);
    GlyphB->SetStaticMesh(OrbitMesh);
    MarkerDisc->SetRelativeLocation(FVector(0.0f, 0.0f, 2.0f));
    GlyphA->SetRelativeLocation(FVector(56.0f, 0.0f, 7.0f));
    GlyphB->SetRelativeLocation(FVector(-56.0f, 0.0f, 8.0f));
    Tags.Add(TEXT("EchoesCommandMarkerView"));
}

void AEchoesCommandMarkerView::InitializeMarker(
    EEchoesCommandMarkerType InType,
    bool bInReducedMotion,
    bool bInReducedFlashing,
    float InLifetimeSeconds)
{
    MarkerType = InType;
    bReducedMotion = bInReducedMotion;
    bReducedFlashing = bInReducedFlashing;
    ElapsedSeconds = 0.0f;
    PresentationLifetimeSeconds = FMath::Clamp(
        InLifetimeSeconds,
        MinimumMarkerLifetimeSeconds,
        MaximumMarkerLifetimeSeconds);

    MarkerDisc->SetStaticMesh(MeshForMarkerType(MarkerType));
    GlyphA->SetStaticMesh(OrbitMesh);
    GlyphB->SetStaticMesh(OrbitMesh);
    MarkerDisc->SetVisibility(true);
    GlyphA->SetVisibility(true);
    GlyphB->SetVisibility(true);
    MarkerDisc->SetRelativeLocation(FVector(0.0f, 0.0f, 2.0f));
    MarkerDisc->SetRelativeRotation(FRotator::ZeroRotator);
    MarkerDisc->SetRelativeScale3D(FVector(1.35f, 1.35f, 1.0f));
    GlyphA->SetRelativeLocation(FVector(56.0f, 0.0f, 7.0f));
    GlyphB->SetRelativeLocation(FVector(-56.0f, 0.0f, 8.0f));
    GlyphA->SetRelativeRotation(FRotator::ZeroRotator);
    GlyphB->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
    GlyphA->SetRelativeScale3D(FVector(0.72f, 0.72f, 0.72f));
    GlyphB->SetRelativeScale3D(FVector(0.72f, 0.72f, 0.72f));

    switch (MarkerType)
    {
        case EEchoesCommandMarkerType::Move:
            BaseColor = FLinearColor(0.05f, 0.92f, 1.0f);
            break;
        case EEchoesCommandMarkerType::AttackMove:
            BaseColor = FLinearColor(1.0f, 0.34f, 0.04f);
            break;
        case EEchoesCommandMarkerType::Patrol:
            BaseColor = FLinearColor(0.76f, 0.24f, 1.0f);
            break;
        case EEchoesCommandMarkerType::Guard:
            BaseColor = FLinearColor(0.20f, 1.0f, 0.42f);
            break;
        case EEchoesCommandMarkerType::Build:
            BaseColor = FLinearColor(0.98f, 0.84f, 0.22f);
            break;
        case EEchoesCommandMarkerType::Interact:
            BaseColor = FLinearColor(0.32f, 0.95f, 0.82f);
            break;
    }

    BaseScale = GetActorScale3D();
    BaseDiscRotation = MarkerDisc->GetRelativeRotation();
    BaseGlyphALocation = GlyphA->GetRelativeLocation();
    BaseGlyphBLocation = GlyphB->GetRelativeLocation();
    DiscMaterial = UMaterialInstanceDynamic::Create(VFXMaterial, this);
    GlyphAMaterial = UMaterialInstanceDynamic::Create(VFXMaterial, this);
    GlyphBMaterial = UMaterialInstanceDynamic::Create(VFXMaterial, this);
    MarkerDisc->SetMaterial(0, DiscMaterial);
    GlyphA->SetMaterial(0, GlyphAMaterial);
    GlyphB->SetMaterial(0, GlyphBMaterial);
    ApplyAppearance(BaseColor, bReducedFlashing ? 1.35f : 2.7f);
}

void AEchoesCommandMarkerView::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    ElapsedSeconds += DeltaSeconds;
    if (ElapsedSeconds >= PresentationLifetimeSeconds)
    {
        Destroy();
        return;
    }

    if (!bReducedMotion)
    {
        const float IntroAlpha = FMath::Clamp(ElapsedSeconds / 0.28f, 0.0f, 1.0f);
        const float SettledAlpha = 1.0f - FMath::Pow(1.0f - IntroAlpha, 3.0f);
        const float RemainingSeconds = PresentationLifetimeSeconds - ElapsedSeconds;
        const float ExitAlpha = FMath::Clamp(RemainingSeconds / 0.42f, 0.0f, 1.0f);
        const float PresentationScale =
            FMath::Lerp(0.72f, 1.0f, SettledAlpha) *
            FMath::Lerp(0.90f, 1.0f, ExitAlpha);
        SetActorScale3D(BaseScale * PresentationScale);

        const float DiscYaw = ElapsedSeconds * 18.0f;
        MarkerDisc->SetRelativeRotation(
            BaseDiscRotation + FRotator(0.0f, DiscYaw, 0.0f));
        const float OrbitAngle = FMath::DegreesToRadians(ElapsedSeconds * 72.0f);
        const FVector OrbitOffset(
            FMath::Cos(OrbitAngle) * 56.0f,
            FMath::Sin(OrbitAngle) * 56.0f,
            7.0f);
        GlyphA->SetRelativeLocation(OrbitOffset);
        GlyphB->SetRelativeLocation(FVector(-OrbitOffset.X, -OrbitOffset.Y, 8.0f));
        GlyphA->SetRelativeRotation(
            FRotator(0.0f, FMath::RadiansToDegrees(OrbitAngle), 0.0f));
        GlyphB->SetRelativeRotation(
            FRotator(0.0f, FMath::RadiansToDegrees(OrbitAngle) + 180.0f, 0.0f));
    }
    else
    {
        SetActorScale3D(BaseScale);
        MarkerDisc->SetRelativeRotation(BaseDiscRotation);
        GlyphA->SetRelativeLocation(BaseGlyphALocation);
        GlyphB->SetRelativeLocation(BaseGlyphBLocation);
    }

    const float EmissiveStrength = bReducedFlashing
                                       ? 1.35f
                                       : 2.7f + 0.35f *
                                             (1.0f - FMath::Clamp(
                                                         ElapsedSeconds / 0.32f,
                                                         0.0f,
                                                         1.0f));
    ApplyAppearance(BaseColor, EmissiveStrength);
}

void AEchoesCommandMarkerView::ApplyAppearance(
    const FLinearColor& Color,
    float EmissiveStrength)
{
    CurrentEmissiveStrength = EmissiveStrength;
    if (DiscMaterial != nullptr)
    {
        DiscMaterial->SetVectorParameterValue(
            MarkerColorParameterName,
            Color * 0.42f);
        DiscMaterial->SetScalarParameterValue(
            MarkerEmissiveParameterName,
            EmissiveStrength * 0.72f);
    }
    for (UMaterialInstanceDynamic* Material : {GlyphAMaterial, GlyphBMaterial})
    {
        if (Material != nullptr)
        {
            Material->SetVectorParameterValue(MarkerColorParameterName, Color);
            Material->SetScalarParameterValue(
                MarkerEmissiveParameterName,
                EmissiveStrength);
        }
    }
}

UStaticMesh* AEchoesCommandMarkerView::MeshForMarkerType(
    EEchoesCommandMarkerType Type) const
{
    switch (Type)
    {
        case EEchoesCommandMarkerType::Move:
            return MoveMesh;
        case EEchoesCommandMarkerType::AttackMove:
            return AttackMoveMesh;
        case EEchoesCommandMarkerType::Patrol:
            return PatrolMesh;
        case EEchoesCommandMarkerType::Guard:
            return GuardMesh;
        case EEchoesCommandMarkerType::Build:
            return BuildMesh;
        case EEchoesCommandMarkerType::Interact:
            return InteractMesh;
    }
    return MoveMesh;
}

bool AEchoesCommandMarkerView::HasCollisionDisabled() const
{
    return MarkerDisc != nullptr && GlyphA != nullptr && GlyphB != nullptr &&
           MarkerDisc->GetCollisionEnabled() == ECollisionEnabled::NoCollision &&
           GlyphA->GetCollisionEnabled() == ECollisionEnabled::NoCollision &&
           GlyphB->GetCollisionEnabled() == ECollisionEnabled::NoCollision;
}

bool AEchoesCommandMarkerView::HasNavigationDisabled() const
{
    return MarkerDisc != nullptr && GlyphA != nullptr && GlyphB != nullptr &&
           !MarkerDisc->CanEverAffectNavigation() &&
           !GlyphA->CanEverAffectNavigation() &&
           !GlyphB->CanEverAffectNavigation();
}

bool AEchoesCommandMarkerView::IsUsingAuthoredVFXAssets() const
{
    const auto IsAuthoredMesh = [](const UStaticMeshComponent* Component)
    {
        return Component != nullptr && Component->GetStaticMesh() != nullptr &&
               Component->GetStaticMesh()->GetPathName().StartsWith(
                   TEXT("/Game/Art/Generated/VFX/SM_VFX_"));
    };
    return IsAuthoredMesh(MarkerDisc) && IsAuthoredMesh(GlyphA) &&
           IsAuthoredMesh(GlyphB) && VFXMaterial != nullptr &&
           VFXMaterial->GetPathName().StartsWith(
               TEXT("/Game/Art/Generated/Materials/M_EchoesPresentationVFX"));
}

float AEchoesCommandMarkerView::GetMarkerDiscYaw() const
{
    return MarkerDisc != nullptr ? MarkerDisc->GetRelativeRotation().Yaw : 0.0f;
}
