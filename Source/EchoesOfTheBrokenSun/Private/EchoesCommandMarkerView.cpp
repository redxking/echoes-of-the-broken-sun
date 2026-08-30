#include "EchoesCommandMarkerView.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
const FName MarkerColorParameterName(TEXT("Color"));
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

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(
        TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(
        TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(
        TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    CubeMesh = CubeFinder.Object;
    CylinderMesh = CylinderFinder.Object;
    BasicMaterial = MaterialFinder.Object;

    MarkerDisc->SetStaticMesh(CylinderMesh);
    GlyphA->SetStaticMesh(CubeMesh);
    GlyphB->SetStaticMesh(CubeMesh);
    MarkerDisc->SetRelativeLocation(FVector(0.0f, 0.0f, 2.0f));
    GlyphA->SetRelativeLocation(FVector(0.0f, 0.0f, 6.0f));
    GlyphB->SetRelativeLocation(FVector(0.0f, 0.0f, 7.0f));
    Tags.Add(TEXT("EchoesCommandMarkerView"));
}

void AEchoesCommandMarkerView::InitializeMarker(
    EEchoesCommandMarkerType InType,
    bool bInReducedMotion,
    bool bInReducedFlashing)
{
    MarkerType = InType;
    bReducedMotion = bInReducedMotion;
    bReducedFlashing = bInReducedFlashing;
    ElapsedSeconds = 0.0f;

    MarkerDisc->SetVisibility(true);
    GlyphA->SetVisibility(true);
    GlyphB->SetVisibility(true);
    MarkerDisc->SetRelativeScale3D(FVector(0.86f, 0.86f, 0.025f));

    switch (MarkerType)
    {
        case EEchoesCommandMarkerType::Move:
            BaseColor = FLinearColor(0.05f, 0.92f, 1.0f);
            GlyphA->SetRelativeLocation(FVector(16.0f, 0.0f, 6.0f));
            GlyphA->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));
            GlyphA->SetRelativeScale3D(FVector(0.56f, 0.08f, 0.025f));
            GlyphB->SetRelativeLocation(FVector(42.0f, 0.0f, 7.0f));
            GlyphB->SetRelativeRotation(FRotator(0.0f, 45.0f, 0.0f));
            GlyphB->SetRelativeScale3D(FVector(0.24f, 0.08f, 0.025f));
            break;
        case EEchoesCommandMarkerType::AttackMove:
            BaseColor = FLinearColor(1.0f, 0.34f, 0.04f);
            GlyphA->SetRelativeRotation(FRotator(0.0f, 45.0f, 0.0f));
            GlyphB->SetRelativeRotation(FRotator(0.0f, -45.0f, 0.0f));
            GlyphA->SetRelativeScale3D(FVector(0.68f, 0.09f, 0.03f));
            GlyphB->SetRelativeScale3D(FVector(0.68f, 0.09f, 0.03f));
            break;
        case EEchoesCommandMarkerType::Patrol:
            BaseColor = FLinearColor(0.76f, 0.24f, 1.0f);
            MarkerDisc->SetRelativeRotation(FRotator(0.0f, 45.0f, 0.0f));
            MarkerDisc->SetRelativeScale3D(FVector(0.72f, 0.72f, 0.025f));
            GlyphA->SetRelativeLocation(FVector(-27.0f, 0.0f, 6.0f));
            GlyphB->SetRelativeLocation(FVector(27.0f, 0.0f, 7.0f));
            GlyphA->SetRelativeScale3D(FVector(0.18f, 0.18f, 0.03f));
            GlyphB->SetRelativeScale3D(FVector(0.18f, 0.18f, 0.03f));
            break;
        case EEchoesCommandMarkerType::Guard:
            BaseColor = FLinearColor(0.20f, 1.0f, 0.42f);
            GlyphA->SetRelativeScale3D(FVector(0.14f, 0.62f, 0.03f));
            GlyphB->SetRelativeScale3D(FVector(0.62f, 0.14f, 0.03f));
            break;
        case EEchoesCommandMarkerType::Build:
            BaseColor = FLinearColor(0.98f, 0.84f, 0.22f);
            MarkerDisc->SetRelativeScale3D(FVector(1.0f, 1.0f, 0.025f));
            GlyphA->SetRelativeScale3D(FVector(0.74f, 0.10f, 0.03f));
            GlyphB->SetRelativeScale3D(FVector(0.10f, 0.74f, 0.03f));
            break;
        case EEchoesCommandMarkerType::Interact:
            BaseColor = FLinearColor(0.32f, 0.95f, 0.82f);
            GlyphA->SetRelativeLocation(FVector(-18.0f, 0.0f, 6.0f));
            GlyphB->SetRelativeLocation(FVector(18.0f, 0.0f, 7.0f));
            GlyphA->SetRelativeScale3D(FVector(0.09f, 0.48f, 0.03f));
            GlyphB->SetRelativeScale3D(FVector(0.09f, 0.48f, 0.03f));
            break;
    }

    BaseScale = GetActorScale3D();
    DiscMaterial = UMaterialInstanceDynamic::Create(BasicMaterial, this);
    GlyphAMaterial = UMaterialInstanceDynamic::Create(BasicMaterial, this);
    GlyphBMaterial = UMaterialInstanceDynamic::Create(BasicMaterial, this);
    MarkerDisc->SetMaterial(0, DiscMaterial);
    GlyphA->SetMaterial(0, GlyphAMaterial);
    GlyphB->SetMaterial(0, GlyphBMaterial);
    ApplyColor(BaseColor);
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
        const float Pulse = 1.0f + 0.10f * FMath::Sin(ElapsedSeconds * 9.0f);
        SetActorScale3D(BaseScale * Pulse);
    }
    if (!bReducedFlashing)
    {
        const float Brightness = 0.72f + 0.28f *
            FMath::Abs(FMath::Sin(ElapsedSeconds * 7.0f));
        ApplyColor(BaseColor * Brightness);
    }
}

void AEchoesCommandMarkerView::ApplyColor(const FLinearColor& Color)
{
    if (DiscMaterial != nullptr)
    {
        DiscMaterial->SetVectorParameterValue(
            MarkerColorParameterName,
            Color * 0.22f);
    }
    for (UMaterialInstanceDynamic* Material : {GlyphAMaterial, GlyphBMaterial})
    {
        if (Material != nullptr)
        {
            Material->SetVectorParameterValue(MarkerColorParameterName, Color);
        }
    }
}

bool AEchoesCommandMarkerView::HasCollisionDisabled() const
{
    return MarkerDisc != nullptr && GlyphA != nullptr && GlyphB != nullptr &&
           MarkerDisc->GetCollisionEnabled() == ECollisionEnabled::NoCollision &&
           GlyphA->GetCollisionEnabled() == ECollisionEnabled::NoCollision &&
           GlyphB->GetCollisionEnabled() == ECollisionEnabled::NoCollision;
}
