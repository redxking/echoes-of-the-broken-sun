#include "EchoesEntityView.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EchoesOfTheBrokenSun.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
const FName ColorParameterName(TEXT("Color"));

FLinearColor ColorForState(const echoes::sim::Entity& State)
{
    if (State.type == echoes::sim::EntityType::FutureWell)
    {
        switch (State.wellChoice)
        {
            case echoes::sim::FutureWellChoice::Harvest:
                return FLinearColor(1.0f, 0.43f, 0.05f);
            case echoes::sim::FutureWellChoice::Preserve:
                return FLinearColor(0.12f, 0.86f, 0.44f);
            case echoes::sim::FutureWellChoice::Reshape:
                return FLinearColor(0.95f, 0.08f, 0.16f);
            case echoes::sim::FutureWellChoice::Dormant:
                return FLinearColor(0.62f, 0.18f, 1.0f);
        }
    }
    if (State.type == echoes::sim::EntityType::ResourceNode)
    {
        return FLinearColor(0.95f, 0.56f, 0.08f);
    }
    return State.owner == UEchoesSimulationSubsystem::LocalPlayerId
               ? FLinearColor(0.04f, 0.72f, 0.88f)
               : FLinearColor(0.84f, 0.08f, 0.16f);
}
}

AEchoesEntityView::AEchoesEntityView()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostPhysics;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
    BodyMesh->SetupAttachment(SceneRoot);
    BodyMesh->SetCollisionObjectType(ECC_WorldDynamic);
    BodyMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    BodyMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    BodyMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    BodyMesh->SetGenerateOverlapEvents(false);
    BodyMesh->SetCastShadow(true);

    SelectionRing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SelectionRing"));
    SelectionRing->SetupAttachment(SceneRoot);
    SelectionRing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SelectionRing->SetCastShadow(false);
    SelectionRing->SetVisibility(false);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(
        TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(
        TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(
        TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeFinder(
        TEXT("/Engine/BasicShapes/Cone.Cone"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(
        TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

    CubeMesh = CubeFinder.Object;
    SphereMesh = SphereFinder.Object;
    CylinderMesh = CylinderFinder.Object;
    ConeMesh = ConeFinder.Object;
    BasicMaterial = MaterialFinder.Object;

    BodyMesh->SetStaticMesh(CylinderMesh);
    SelectionRing->SetStaticMesh(CylinderMesh);
    SelectionRing->SetRelativeLocation(FVector(0.0f, 0.0f, 3.0f));
    SelectionRing->SetRelativeScale3D(FVector(0.72f, 0.72f, 0.025f));

    Tags.Add(TEXT("EchoesEntityView"));
}

void AEchoesEntityView::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!bHasAuthoritativeLocation)
    {
        return;
    }

    const FVector SmoothedLocation = FMath::VInterpTo(
        GetActorLocation(),
        AuthoritativeWorldLocation,
        DeltaSeconds,
        14.0f);
    SetActorLocation(SmoothedLocation, false, nullptr, ETeleportType::None);
}

void AEchoesEntityView::ApplyAuthoritativeState(
    const echoes::sim::Entity& State,
    bool bTeleport)
{
    const bool bNeedsAppearance = EntityId == 0 || EntityType != State.type ||
                                  OwnerPlayerId != State.owner ||
                                  WellChoice != State.wellChoice;
    EntityId = State.id;
    OwnerPlayerId = State.owner;
    EntityType = State.type;
    WellChoice = State.wellChoice;
    HitPoints = State.hitPoints;
    MaxHitPoints = State.maxHitPoints;

    const UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr)
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_VIEW_NO_BRIDGE] entity=%u"),
            EntityId);
        return;
    }

    AuthoritativeWorldLocation = Bridge->SimToWorld(State.position);
    if (bTeleport || !bHasAuthoritativeLocation)
    {
        SetActorLocation(
            AuthoritativeWorldLocation,
            false,
            nullptr,
            ETeleportType::TeleportPhysics);
    }
    bHasAuthoritativeLocation = true;

    if (bNeedsAppearance)
    {
        ConfigureAppearance(State);
    }

    const float HealthFraction = MaxHitPoints > 0
                                     ? FMath::Clamp(
                                           static_cast<float>(HitPoints) /
                                               static_cast<float>(MaxHitPoints),
                                           0.0f,
                                           1.0f)
                                     : 0.0f;
    BodyMesh->SetCustomPrimitiveDataFloat(0, HealthFraction);
}

void AEchoesEntityView::ConfigureAppearance(const echoes::sim::Entity& State)
{
    UStaticMesh* DesiredMesh = CylinderMesh;
    FVector BodyScale(0.48f, 0.48f, 0.70f);
    FVector BodyOffset(0.0f, 0.0f, 35.0f);
    float SelectionRadius = 0.74f;

    switch (State.type)
    {
        case echoes::sim::EntityType::Worker:
            DesiredMesh = CylinderMesh;
            break;
        case echoes::sim::EntityType::Soldier:
            DesiredMesh = ConeMesh;
            BodyScale = FVector(0.58f, 0.58f, 0.90f);
            BodyOffset.Z = 45.0f;
            SelectionRadius = 0.84f;
            break;
        case echoes::sim::EntityType::CommandCore:
            DesiredMesh = CubeMesh;
            BodyScale = FVector(2.0f, 2.0f, 1.25f);
            BodyOffset.Z = 62.5f;
            SelectionRadius = 2.35f;
            break;
        case echoes::sim::EntityType::Dropoff:
            DesiredMesh = CubeMesh;
            BodyScale = FVector(1.35f, 1.35f, 0.85f);
            BodyOffset.Z = 42.5f;
            SelectionRadius = 1.65f;
            break;
        case echoes::sim::EntityType::Barracks:
            DesiredMesh = CubeMesh;
            BodyScale = FVector(1.8f, 1.4f, 0.9f);
            BodyOffset.Z = 45.0f;
            SelectionRadius = 2.15f;
            break;
        case echoes::sim::EntityType::ResourceNode:
            DesiredMesh = SphereMesh;
            BodyScale = FVector(0.74f, 0.74f, 0.74f);
            BodyOffset.Z = 37.0f;
            SelectionRadius = 0.95f;
            break;
        case echoes::sim::EntityType::FutureWell:
            DesiredMesh = CylinderMesh;
            BodyScale = FVector(1.35f, 1.35f, 0.18f);
            BodyOffset.Z = 9.0f;
            SelectionRadius = 1.6f;
            break;
    }

    BodyMesh->SetStaticMesh(DesiredMesh);
    BodyMesh->SetRelativeScale3D(BodyScale);
    BodyMesh->SetRelativeLocation(BodyOffset);
    SelectionRing->SetRelativeScale3D(
        FVector(SelectionRadius, SelectionRadius, 0.025f));

    if (BasicMaterial != nullptr)
    {
        if (BodyMaterial == nullptr)
        {
            BodyMaterial = UMaterialInstanceDynamic::Create(BasicMaterial, this);
            BodyMesh->SetMaterial(0, BodyMaterial);
        }
        if (RingMaterial == nullptr)
        {
            RingMaterial = UMaterialInstanceDynamic::Create(BasicMaterial, this);
            RingMaterial->SetVectorParameterValue(
                ColorParameterName,
                FLinearColor(0.08f, 1.0f, 0.68f));
            SelectionRing->SetMaterial(0, RingMaterial);
        }
        SetBodyColor(ColorForState(State));
    }
}

void AEchoesEntityView::SetBodyColor(const FLinearColor& Color)
{
    if (BodyMaterial != nullptr)
    {
        BodyMaterial->SetVectorParameterValue(ColorParameterName, Color);
    }
}

void AEchoesEntityView::SetSelected(bool bInSelected)
{
    bSelected = bInSelected;
    SelectionRing->SetVisibility(bSelected, true);
    BodyMesh->SetRenderCustomDepth(bSelected);
    BodyMesh->SetCustomDepthStencilValue(bSelected ? 1 : 0);
}

FString AEchoesEntityView::GetDisplayName() const
{
    switch (EntityType)
    {
        case echoes::sim::EntityType::Worker:
            return TEXT("Worker");
        case echoes::sim::EntityType::Soldier:
            return TEXT("Soldier");
        case echoes::sim::EntityType::CommandCore:
            return TEXT("Command Core");
        case echoes::sim::EntityType::Dropoff:
            return TEXT("Matter Drop-off");
        case echoes::sim::EntityType::Barracks:
            return TEXT("Barracks");
        case echoes::sim::EntityType::ResourceNode:
            return TEXT("Matter Node");
        case echoes::sim::EntityType::FutureWell:
            return TEXT("Future Well");
    }
    return TEXT("Unknown Entity");
}
