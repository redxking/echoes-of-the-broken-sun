#include "EchoesEntityView.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EchoesOfTheBrokenSun.h"
#include "EchoesGameUserSettings.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
const FName EntityColorParameterName(TEXT("Color"));
constexpr float DamagePulseDurationSeconds = 0.18f;

FLinearColor ColorForState(const echoes::sim::Entity& State)
{
    if (State.temporaryMineralCover)
    {
        return FLinearColor(0.42f, 0.28f, 0.16f);
    }
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
    switch (State.owner)
    {
        case 0:
            return FLinearColor(0.04f, 0.72f, 0.88f);
        case 1:
            return FLinearColor(0.92f, 0.30f, 0.05f);
        case 2:
            return FLinearColor(0.95f, 0.74f, 0.08f);
        case 3:
            return FLinearColor(0.62f, 0.30f, 0.95f);
        default:
            return FLinearColor(0.72f, 0.72f, 0.72f);
    }
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

    HealthBarBackground = CreateDefaultSubobject<UStaticMeshComponent>(
        TEXT("HealthBarBackground"));
    HealthBarBackground->SetupAttachment(SceneRoot);
    HealthBarFill = CreateDefaultSubobject<UStaticMeshComponent>(
        TEXT("HealthBarFill"));
    HealthBarFill->SetupAttachment(SceneRoot);
    for (UStaticMeshComponent* Bar : {HealthBarBackground, HealthBarFill})
    {
        Bar->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Bar->SetGenerateOverlapEvents(false);
        Bar->SetCastShadow(false);
        Bar->SetReceivesDecals(false);
        Bar->SetVisibility(false);
    }

    OwnerMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OwnerMarker"));
    OwnerMarker->SetupAttachment(SceneRoot);
    OwnerMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    OwnerMarker->SetGenerateOverlapEvents(false);
    OwnerMarker->SetCastShadow(false);
    OwnerMarker->SetReceivesDecals(false);
    OwnerMarker->SetVisibility(false);

    DeploymentCover = CreateDefaultSubobject<UStaticMeshComponent>(
        TEXT("DeploymentCover"));
    DeploymentCover->SetupAttachment(SceneRoot);
    DeploymentCover->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    DeploymentCover->SetGenerateOverlapEvents(false);
    DeploymentCover->SetCastShadow(false);
    DeploymentCover->SetReceivesDecals(false);
    DeploymentCover->SetVisibility(false);

    RelaySupplyField = CreateDefaultSubobject<UStaticMeshComponent>(
        TEXT("RelaySupplyField"));
    RelaySupplyField->SetupAttachment(SceneRoot);
    RelaySupplyField->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    RelaySupplyField->SetGenerateOverlapEvents(false);
    RelaySupplyField->SetCastShadow(false);
    RelaySupplyField->SetReceivesDecals(false);
    RelaySupplyField->SetVisibility(false);

    WaystoneStateField = CreateDefaultSubobject<UStaticMeshComponent>(
        TEXT("WaystoneStateField"));
    WaystoneStateField->SetupAttachment(SceneRoot);
    WaystoneStateField->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WaystoneStateField->SetGenerateOverlapEvents(false);
    WaystoneStateField->SetCastShadow(false);
    WaystoneStateField->SetReceivesDecals(false);
    WaystoneStateField->SetVisibility(false);

    WarformStateField = CreateDefaultSubobject<UStaticMeshComponent>(
        TEXT("WarformStateField"));
    WarformStateField->SetupAttachment(SceneRoot);
    WarformStateField->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WarformStateField->SetGenerateOverlapEvents(false);
    WarformStateField->SetCastShadow(false);
    WarformStateField->SetReceivesDecals(false);
    WarformStateField->SetVisibility(false);

    AegisPowerField = CreateDefaultSubobject<UStaticMeshComponent>(
        TEXT("AegisPowerField"));
    AegisPowerField->SetupAttachment(SceneRoot);
    AegisPowerField->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    AegisPowerField->SetGenerateOverlapEvents(false);
    AegisPowerField->SetCastShadow(false);
    AegisPowerField->SetReceivesDecals(false);
    AegisPowerField->SetVisibility(false);

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
    HealthBarBackground->SetStaticMesh(CubeMesh);
    HealthBarFill->SetStaticMesh(CubeMesh);
    DeploymentCover->SetStaticMesh(CubeMesh);
    RelaySupplyField->SetStaticMesh(CylinderMesh);
    RelaySupplyField->SetRelativeLocation(FVector(0.0f, 0.0f, 5.0f));
    RelaySupplyField->SetRelativeScale3D(FVector(1.65f, 1.65f, 0.025f));
    WaystoneStateField->SetStaticMesh(CylinderMesh);
    WaystoneStateField->SetRelativeLocation(FVector(0.0f, 0.0f, 4.0f));
    WarformStateField->SetStaticMesh(CylinderMesh);
    WarformStateField->SetRelativeLocation(FVector(0.0f, 0.0f, 5.0f));
    AegisPowerField->SetStaticMesh(CylinderMesh);
    AegisPowerField->SetRelativeLocation(FVector(0.0f, 0.0f, 6.0f));
    AegisPowerField->SetRelativeScale3D(FVector(1.35f, 1.35f, 0.045f));
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

    const UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get();
    if (Settings != nullptr && Settings->IsReducedFlashingEnabled())
    {
        DamagePulseRemainingSeconds = 0.0f;
    }
    if (DamagePulseRemainingSeconds > 0.0f)
    {
        DamagePulseRemainingSeconds = FMath::Max(
            0.0f,
            DamagePulseRemainingSeconds - DeltaSeconds);
        const float PulseAlpha = FMath::Clamp(
            DamagePulseRemainingSeconds / DamagePulseDurationSeconds,
            0.0f,
            1.0f);
        SetBodyColor(FMath::Lerp(
            BaseBodyColor,
            FLinearColor(1.0f, 0.82f, 0.28f),
            PulseAlpha));
    }
    else
    {
        SetBodyColor(BaseBodyColor);
    }
}

void AEchoesEntityView::ApplyAuthoritativeState(
    const echoes::sim::Entity& State,
    bool bTeleport)
{
    const bool bHadAuthoritativeState = EntityId != 0;
    const int32 PreviousHitPoints = HitPoints;
    const bool bNeedsAppearance = EntityId == 0 || EntityType != State.type ||
                                  OwnerPlayerId != State.owner ||
                                  EntityFaction != State.faction ||
                                  WellChoice != State.wellChoice ||
                                  bDeployed != State.deployed ||
                                  DeploymentFacing != State.deploymentFacing ||
                                  bRelaySupplyActive != State.relaySupplyActive ||
                                  WaystoneMode != State.waystoneMode ||
                                  WarformAdaptation != State.warformAdaptation ||
                                  PendingWarformAdaptation !=
                                      State.pendingWarformAdaptation ||
                                  bTemporaryMineralCover !=
                                      State.temporaryMineralCover ||
                                  bAegisPowered != State.aegisPowered;
    EntityId = State.id;
    OwnerPlayerId = State.owner;
    EntityFaction = State.faction;
    EntityType = State.type;
    WellChoice = State.wellChoice;
    bDeployed = State.deployed;
    DeploymentFacing = State.deploymentFacing;
    bRelaySupplyActive = State.relaySupplyActive;
    WaystoneMode = State.waystoneMode;
    WarformAdaptation = State.warformAdaptation;
    PendingWarformAdaptation = State.pendingWarformAdaptation;
    bTemporaryMineralCover = State.temporaryMineralCover;
    bAegisPowered = State.aegisPowered;
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

    if (bHadAuthoritativeState && HitPoints < PreviousHitPoints)
    {
        const UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get();
        const bool bReducedFlashing =
            Settings != nullptr && Settings->IsReducedFlashingEnabled();
        DamagePulseRemainingSeconds =
            bReducedFlashing ? 0.0f : DamagePulseDurationSeconds;
        SetBodyColor(
            bReducedFlashing
                ? BaseBodyColor
                : FLinearColor(1.0f, 0.82f, 0.28f));
    }

    DisplayedHealthFraction = MaxHitPoints > 0
                                  ? FMath::Clamp(
                                        static_cast<float>(HitPoints) /
                                            static_cast<float>(MaxHitPoints),
                                        0.0f,
                                        1.0f)
                                  : 0.0f;
    BodyMesh->SetCustomPrimitiveDataFloat(0, DisplayedHealthFraction);
    UpdateHealthBar();
}

void AEchoesEntityView::ConfigureAppearance(const echoes::sim::Entity& State)
{
    UStaticMesh* DesiredMesh = CylinderMesh;
    FVector BodyScale(0.48f, 0.48f, 0.70f);
    FVector BodyOffset(0.0f, 0.0f, 35.0f);
    float SelectionRadius = 0.74f;
    HealthBarWidthScale = 0.9f;
    HealthBarHeight = 92.0f;

    if (State.temporaryMineralCover)
    {
        DesiredMesh = CubeMesh;
        BodyScale = FVector(1.50f, 1.50f, 1.10f);
        BodyOffset.Z = 55.0f;
        SelectionRadius = 1.35f;
        HealthBarWidthScale = 1.55f;
        HealthBarHeight = 132.0f;
    }
    else
    {
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
            HealthBarWidthScale = 1.05f;
            HealthBarHeight = 112.0f;
            break;
        case echoes::sim::EntityType::HeavyUnit:
            DesiredMesh = SphereMesh;
            BodyScale = FVector(0.90f, 0.78f, 0.72f);
            BodyOffset.Z = 36.0f;
            SelectionRadius = 1.05f;
            HealthBarWidthScale = 1.25f;
            HealthBarHeight = 102.0f;
            break;
        case echoes::sim::EntityType::ScoutUnit:
            DesiredMesh = ConeMesh;
            BodyScale = FVector(0.42f, 0.42f, 0.62f);
            BodyOffset.Z = 42.0f;
            SelectionRadius = 0.72f;
            HealthBarWidthScale = 0.85f;
            HealthBarHeight = 94.0f;
            break;
        case echoes::sim::EntityType::CommandCore:
            DesiredMesh = CubeMesh;
            BodyScale = FVector(2.0f, 2.0f, 1.25f);
            BodyOffset.Z = 62.5f;
            SelectionRadius = 2.35f;
            HealthBarWidthScale = 2.2f;
            HealthBarHeight = 158.0f;
            break;
        case echoes::sim::EntityType::Dropoff:
            DesiredMesh = CubeMesh;
            BodyScale = FVector(1.35f, 1.35f, 0.85f);
            BodyOffset.Z = 42.5f;
            SelectionRadius = 1.65f;
            HealthBarWidthScale = 1.55f;
            HealthBarHeight = 112.0f;
            break;
        case echoes::sim::EntityType::Barracks:
            DesiredMesh = CubeMesh;
            BodyScale = FVector(1.8f, 1.4f, 0.9f);
            BodyOffset.Z = 45.0f;
            SelectionRadius = 2.15f;
            HealthBarWidthScale = 1.95f;
            HealthBarHeight = 120.0f;
            break;
        case echoes::sim::EntityType::UtilityStructure:
            DesiredMesh = CylinderMesh;
            BodyScale = FVector(0.85f, 0.85f, 1.45f);
            BodyOffset.Z = 72.5f;
            SelectionRadius = 1.15f;
            HealthBarWidthScale = 1.3f;
            HealthBarHeight = 165.0f;
            break;
        case echoes::sim::EntityType::ResourceNode:
            DesiredMesh = SphereMesh;
            BodyScale = FVector(0.74f, 0.74f, 0.74f);
            BodyOffset.Z = 37.0f;
            SelectionRadius = 0.95f;
            HealthBarWidthScale = 1.05f;
            HealthBarHeight = 92.0f;
            break;
        case echoes::sim::EntityType::FutureWell:
            DesiredMesh = CylinderMesh;
            BodyScale = FVector(1.35f, 1.35f, 0.18f);
            BodyOffset.Z = 9.0f;
            SelectionRadius = 1.6f;
            HealthBarWidthScale = 1.5f;
            HealthBarHeight = 42.0f;
            break;
        }
    }

    BodyMesh->SetStaticMesh(DesiredMesh);
    BodyMesh->SetRelativeScale3D(BodyScale);
    BodyMesh->SetRelativeLocation(BodyOffset);
    SelectionRing->SetRelativeScale3D(
        FVector(SelectionRadius, SelectionRadius, 0.025f));

    const bool bShowDeploymentCover =
        State.deployed &&
        State.faction == echoes::sim::Faction::MeridianCompact &&
        State.type == echoes::sim::EntityType::HeavyUnit;
    if (bShowDeploymentCover)
    {
        const bool bFacesAlongX = State.deploymentFacing.x.Raw() != 0;
        const float Sign = bFacesAlongX
                               ? (State.deploymentFacing.x.Raw() > 0 ? 1.0f : -1.0f)
                               : (State.deploymentFacing.y.Raw() > 0 ? 1.0f : -1.0f);
        DeploymentCover->SetRelativeLocation(
            bFacesAlongX
                ? FVector(58.0f * Sign, 0.0f, 58.0f)
                : FVector(0.0f, 58.0f * Sign, 58.0f));
        DeploymentCover->SetRelativeScale3D(
            bFacesAlongX
                ? FVector(0.10f, 1.45f, 0.58f)
                : FVector(1.45f, 0.10f, 0.58f));
    }
    DeploymentCover->SetVisibility(bShowDeploymentCover, true);
    RelaySupplyField->SetVisibility(
        State.relaySupplyActive &&
            State.faction == echoes::sim::Faction::MeridianCompact &&
            State.type == echoes::sim::EntityType::ScoutUnit,
        true);
    const bool bIsWaystone =
        State.faction == echoes::sim::Faction::KharuunAssemblies &&
        State.type == echoes::sim::EntityType::Dropoff &&
        State.waystoneMode != echoes::sim::WaystoneMode::NotWaystone;
    if (bIsWaystone)
    {
        switch (State.waystoneMode)
        {
            case echoes::sim::WaystoneMode::Rooted:
                WaystoneStateField->SetRelativeScale3D(FVector(1.65f, 1.65f, 0.035f));
                break;
            case echoes::sim::WaystoneMode::Uprooting:
            case echoes::sim::WaystoneMode::Rooting:
                WaystoneStateField->SetRelativeScale3D(FVector(1.25f, 1.25f, 0.08f));
                break;
            case echoes::sim::WaystoneMode::Mobile:
                WaystoneStateField->SetRelativeScale3D(FVector(0.72f, 0.72f, 0.10f));
                break;
            case echoes::sim::WaystoneMode::NotWaystone:
                break;
        }
    }
    WaystoneStateField->SetVisibility(bIsWaystone, true);
    const bool bIsPublicWarformState =
        State.faction == echoes::sim::Faction::KharuunAssemblies &&
        (State.type == echoes::sim::EntityType::Soldier ||
         State.type == echoes::sim::EntityType::HeavyUnit ||
         State.type == echoes::sim::EntityType::ScoutUnit) &&
        (State.warformAdaptation != echoes::sim::WarformAdaptation::None ||
         State.pendingWarformAdaptation !=
             echoes::sim::WarformAdaptation::None);
    if (bIsPublicWarformState)
    {
        if (State.pendingWarformAdaptation !=
            echoes::sim::WarformAdaptation::None)
        {
            WarformStateField->SetStaticMesh(CylinderMesh);
            WarformStateField->SetRelativeScale3D(FVector(1.15f, 1.15f, 0.08f));
        }
        else if (State.warformAdaptation ==
                 echoes::sim::WarformAdaptation::Carapace)
        {
            WarformStateField->SetStaticMesh(CubeMesh);
            WarformStateField->SetRelativeScale3D(FVector(0.72f, 0.72f, 0.06f));
        }
        else
        {
            WarformStateField->SetStaticMesh(ConeMesh);
            WarformStateField->SetRelativeScale3D(FVector(0.34f, 0.34f, 0.18f));
        }
    }
    WarformStateField->SetVisibility(bIsPublicWarformState, true);
    AegisPowerField->SetVisibility(
        State.aegisPowered &&
            State.faction == echoes::sim::Faction::MeridianCompact &&
            State.type == echoes::sim::EntityType::UtilityStructure,
        true);

    UStaticMesh* MarkerMesh = nullptr;
    switch (State.owner)
    {
        case 0:
            MarkerMesh = CubeMesh;
            break;
        case 1:
            MarkerMesh = ConeMesh;
            break;
        case 2:
            MarkerMesh = SphereMesh;
            break;
        case 3:
            MarkerMesh = CylinderMesh;
            break;
        default:
            break;
    }
    OwnerMarker->SetStaticMesh(MarkerMesh);
    OwnerMarker->SetRelativeLocation(FVector(0.0f, 0.0f, HealthBarHeight + 28.0f));
    OwnerMarker->SetRelativeScale3D(
        State.owner == 1
            ? FVector(0.20f, 0.20f, 0.28f)
            : State.owner == 2
                  ? FVector(0.18f, 0.18f, 0.18f)
                  : State.owner == 3
                        ? FVector(0.18f, 0.18f, 0.08f)
                        : FVector(0.18f, 0.18f, 0.12f));
    OwnerMarker->SetVisibility(MarkerMesh != nullptr, true);

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
                EntityColorParameterName,
                FLinearColor(0.08f, 1.0f, 0.68f));
            SelectionRing->SetMaterial(0, RingMaterial);
        }
        if (HealthBarBackgroundMaterial == nullptr)
        {
            HealthBarBackgroundMaterial =
                UMaterialInstanceDynamic::Create(BasicMaterial, this);
            HealthBarBackgroundMaterial->SetVectorParameterValue(
                EntityColorParameterName,
                FLinearColor(0.008f, 0.012f, 0.018f));
            HealthBarBackground->SetMaterial(0, HealthBarBackgroundMaterial);
        }
        if (HealthBarFillMaterial == nullptr)
        {
            HealthBarFillMaterial =
                UMaterialInstanceDynamic::Create(BasicMaterial, this);
            HealthBarFill->SetMaterial(0, HealthBarFillMaterial);
        }
        if (OwnerMarkerMaterial == nullptr)
        {
            OwnerMarkerMaterial = UMaterialInstanceDynamic::Create(BasicMaterial, this);
            OwnerMarker->SetMaterial(0, OwnerMarkerMaterial);
        }
        if (DeploymentCoverMaterial == nullptr)
        {
            DeploymentCoverMaterial =
                UMaterialInstanceDynamic::Create(BasicMaterial, this);
            DeploymentCoverMaterial->SetVectorParameterValue(
                EntityColorParameterName,
                FLinearColor(0.10f, 0.88f, 0.92f));
            DeploymentCover->SetMaterial(0, DeploymentCoverMaterial);
        }
        if (RelaySupplyFieldMaterial == nullptr)
        {
            RelaySupplyFieldMaterial =
                UMaterialInstanceDynamic::Create(BasicMaterial, this);
            RelaySupplyFieldMaterial->SetVectorParameterValue(
                EntityColorParameterName,
                FLinearColor(0.95f, 0.76f, 0.18f));
            RelaySupplyField->SetMaterial(0, RelaySupplyFieldMaterial);
        }
        if (WaystoneStateFieldMaterial == nullptr)
        {
            WaystoneStateFieldMaterial =
                UMaterialInstanceDynamic::Create(BasicMaterial, this);
            WaystoneStateFieldMaterial->SetVectorParameterValue(
                EntityColorParameterName,
                FLinearColor(0.58f, 0.22f, 0.92f));
            WaystoneStateField->SetMaterial(0, WaystoneStateFieldMaterial);
        }
        if (WarformStateFieldMaterial == nullptr)
        {
            WarformStateFieldMaterial =
                UMaterialInstanceDynamic::Create(BasicMaterial, this);
            WarformStateFieldMaterial->SetVectorParameterValue(
                EntityColorParameterName,
                FLinearColor(0.88f, 0.56f, 0.14f));
            WarformStateField->SetMaterial(0, WarformStateFieldMaterial);
        }
        if (AegisPowerFieldMaterial == nullptr)
        {
            AegisPowerFieldMaterial =
                UMaterialInstanceDynamic::Create(BasicMaterial, this);
            AegisPowerFieldMaterial->SetVectorParameterValue(
                EntityColorParameterName,
                FLinearColor(0.98f, 0.84f, 0.18f));
            AegisPowerField->SetMaterial(0, AegisPowerFieldMaterial);
        }
        const FLinearColor TeamColor = ColorForState(State);
        BaseBodyColor = TeamColor;
        SetBodyColor(BaseBodyColor);
        OwnerMarkerMaterial->SetVectorParameterValue(
            EntityColorParameterName,
            TeamColor);
    }
}

bool AEchoesEntityView::IsDeploymentCoverVisible() const
{
    return DeploymentCover != nullptr && DeploymentCover->IsVisible();
}

bool AEchoesEntityView::IsRelaySupplyFieldVisible() const
{
    return RelaySupplyField != nullptr && RelaySupplyField->IsVisible();
}

bool AEchoesEntityView::IsWaystoneStateVisible() const
{
    return WaystoneStateField != nullptr && WaystoneStateField->IsVisible();
}

bool AEchoesEntityView::IsWarformStateVisible() const
{
    return WarformStateField != nullptr && WarformStateField->IsVisible();
}

bool AEchoesEntityView::IsAegisPowerFieldVisible() const
{
    return AegisPowerField != nullptr && AegisPowerField->IsVisible();
}

void AEchoesEntityView::SetBodyColor(const FLinearColor& Color)
{
    if (BodyMaterial != nullptr)
    {
        BodyMaterial->SetVectorParameterValue(EntityColorParameterName, Color);
    }
}

void AEchoesEntityView::SetSelected(bool bInSelected)
{
    bSelected = bInSelected;
    SelectionRing->SetVisibility(bSelected, true);
    BodyMesh->SetRenderCustomDepth(bSelected);
    BodyMesh->SetCustomDepthStencilValue(bSelected ? 1 : 0);
    UpdateHealthBar();
}

void AEchoesEntityView::UpdateHealthBar()
{
    if (HealthBarBackground == nullptr || HealthBarFill == nullptr)
    {
        return;
    }
    const bool bShowHealth = bSelected || DisplayedHealthFraction < 0.999f;
    HealthBarBackground->SetVisibility(bShowHealth, true);
    HealthBarFill->SetVisibility(bShowHealth && DisplayedHealthFraction > 0.0f, true);
    HealthBarBackground->SetRelativeLocation(
        FVector(0.0f, 0.0f, HealthBarHeight));
    HealthBarBackground->SetRelativeScale3D(
        FVector(HealthBarWidthScale, 0.10f, 0.045f));

    const float FillWidth = HealthBarWidthScale * DisplayedHealthFraction;
    const float FillOffsetX =
        -50.0f * HealthBarWidthScale * (1.0f - DisplayedHealthFraction);
    HealthBarFill->SetRelativeLocation(
        FVector(FillOffsetX, 0.0f, HealthBarHeight + 1.0f));
    HealthBarFill->SetRelativeScale3D(
        FVector(FillWidth, 0.075f, 0.030f));
    if (HealthBarFillMaterial != nullptr)
    {
        const FLinearColor HealthColor =
            DisplayedHealthFraction > 0.60f
                ? FLinearColor(0.10f, 0.92f, 0.35f)
                : DisplayedHealthFraction > 0.30f
                      ? FLinearColor(1.0f, 0.62f, 0.08f)
                      : FLinearColor(1.0f, 0.10f, 0.08f);
        HealthBarFillMaterial->SetVectorParameterValue(
            EntityColorParameterName,
            HealthColor);
    }
}

bool AEchoesEntityView::IsHealthBarVisible() const
{
    return HealthBarBackground != nullptr &&
           HealthBarBackground->IsVisible();
}

bool AEchoesEntityView::IsOwnerMarkerVisible() const
{
    return OwnerMarker != nullptr && OwnerMarker->IsVisible();
}

uint8 AEchoesEntityView::GetOwnerMarkerVariant() const
{
    return OwnerPlayerId < echoes::sim::kMaximumPlayers
               ? OwnerPlayerId
               : echoes::sim::kNeutralPlayer;
}

FString AEchoesEntityView::GetDisplayName() const
{
    switch (EntityType)
    {
        case echoes::sim::EntityType::Worker:
            return TEXT("Worker");
        case echoes::sim::EntityType::Soldier:
            return EntityFaction == echoes::sim::Faction::MeridianCompact
                       ? TEXT("Lancer")
                       : TEXT("Riftstalker");
        case echoes::sim::EntityType::HeavyUnit:
            return EntityFaction == echoes::sim::Faction::MeridianCompact
                       ? TEXT("Bulwark Team")
                       : TEXT("Cairnback");
        case echoes::sim::EntityType::ScoutUnit:
            return EntityFaction == echoes::sim::Faction::MeridianCompact
                       ? TEXT("Relay Skiff")
                       : TEXT("Resonant");
        case echoes::sim::EntityType::CommandCore:
            return TEXT("Command Core");
        case echoes::sim::EntityType::Dropoff:
            return EntityFaction == echoes::sim::Faction::MeridianCompact
                       ? TEXT("Power Link")
                       : TEXT("Waystone");
        case echoes::sim::EntityType::Barracks:
            return EntityFaction == echoes::sim::Faction::MeridianCompact
                       ? TEXT("Array Foundry")
                       : TEXT("Growth Basin");
        case echoes::sim::EntityType::UtilityStructure:
            return bTemporaryMineralCover
                       ? TEXT("Mineral Cover")
                       : EntityFaction == echoes::sim::Faction::MeridianCompact
                       ? TEXT("Aegis Post")
                       : TEXT("Listening Spine");
        case echoes::sim::EntityType::ResourceNode:
            return TEXT("Matter Node");
        case echoes::sim::EntityType::FutureWell:
            return TEXT("Future Well");
    }
    return TEXT("Unknown Entity");
}
