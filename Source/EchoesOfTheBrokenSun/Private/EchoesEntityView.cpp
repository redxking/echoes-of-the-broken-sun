#include "EchoesEntityView.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EchoesOfTheBrokenSun.h"
#include "EchoesGameUserSettings.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
const FName EntityColorParameterName(TEXT("Color"));
const FName MetallicParameterName(TEXT("Metallic"));
const FName RoughnessParameterName(TEXT("Roughness"));
const FName EmissiveStrengthParameterName(TEXT("EmissiveStrength"));
constexpr float DamagePulseDurationSeconds = 0.18f;

const TCHAR* AuthoredPresentationMeshPath(
    echoes::sim::Faction Faction,
    echoes::sim::EntityType Type)
{
    if (Type == echoes::sim::EntityType::ResourceNode)
    {
        return TEXT("/Game/Art/Generated/World/Resources/SM_World_MatterDeposit.SM_World_MatterDeposit");
    }
    if (Type == echoes::sim::EntityType::FutureWell)
    {
        return TEXT("/Game/Art/Generated/World/Landmarks/SM_World_FutureWellBase.SM_World_FutureWellBase");
    }
    if (Faction == echoes::sim::Faction::HollowChoir)
    {
        switch (Type)
        {
            case echoes::sim::EntityType::Worker:
                return TEXT("/Game/Art/Generated/Choir/Units/SM_Choir_Threadkeeper.SM_Choir_Threadkeeper");
            case echoes::sim::EntityType::Soldier:
                return TEXT("/Game/Art/Generated/Choir/Units/SM_Choir_Intervalist.SM_Choir_Intervalist");
            case echoes::sim::EntityType::HeavyUnit:
                return TEXT("/Game/Art/Generated/Choir/Units/SM_Choir_LacunaWarden.SM_Choir_LacunaWarden");
            case echoes::sim::EntityType::ScoutUnit:
                return TEXT("/Game/Art/Generated/Choir/Units/SM_Choir_Afterimage.SM_Choir_Afterimage");
            case echoes::sim::EntityType::CommandCore:
                return TEXT("/Game/Art/Generated/Choir/Structures/SM_Choir_Concordance.SM_Choir_Concordance");
            case echoes::sim::EntityType::Dropoff:
                return TEXT("/Game/Art/Generated/Choir/Structures/SM_Choir_IntervalLoom.SM_Choir_IntervalLoom");
            case echoes::sim::EntityType::Barracks:
                return TEXT("/Game/Art/Generated/Choir/Structures/SM_Choir_ChorusLoom.SM_Choir_ChorusLoom");
            case echoes::sim::EntityType::UtilityStructure:
                return TEXT("/Game/Art/Generated/Choir/Structures/SM_Choir_PhaseAnchor.SM_Choir_PhaseAnchor");
            case echoes::sim::EntityType::ResourceNode:
            case echoes::sim::EntityType::FutureWell:
                break;
        }
    }
    const bool bKharuun =
        Faction == echoes::sim::Faction::KharuunAssemblies;
    switch (Type)
    {
        case echoes::sim::EntityType::Worker:
            return bKharuun
                       ? TEXT("/Game/Art/Generated/Kharuun/Units/SM_Kharuun_Tender.SM_Kharuun_Tender")
                       : TEXT("/Game/Art/Generated/Meridian/Units/SM_Meridian_Surveyor.SM_Meridian_Surveyor");
        case echoes::sim::EntityType::Soldier:
            return bKharuun
                       ? TEXT("/Game/Art/Generated/Kharuun/Units/SM_Kharuun_Riftstalker.SM_Kharuun_Riftstalker")
                       : TEXT("/Game/Art/Generated/Meridian/Units/SM_Meridian_Lancer.SM_Meridian_Lancer");
        case echoes::sim::EntityType::HeavyUnit:
            return bKharuun
                       ? TEXT("/Game/Art/Generated/Kharuun/Units/SM_Kharuun_Cairnback.SM_Kharuun_Cairnback")
                       : TEXT("/Game/Art/Generated/Meridian/Units/SM_Meridian_Bulwark.SM_Meridian_Bulwark");
        case echoes::sim::EntityType::ScoutUnit:
            return bKharuun
                       ? TEXT("/Game/Art/Generated/Kharuun/Units/SM_Kharuun_Resonant.SM_Kharuun_Resonant")
                       : TEXT("/Game/Art/Generated/Meridian/Units/SM_Meridian_RelaySkiff.SM_Meridian_RelaySkiff");
        case echoes::sim::EntityType::CommandCore:
            return bKharuun
                       ? TEXT("/Game/Art/Generated/Kharuun/Structures/SM_Kharuun_MemoryHearth.SM_Kharuun_MemoryHearth")
                       : TEXT("/Game/Art/Generated/Meridian/Structures/SM_Meridian_Anchor.SM_Meridian_Anchor");
        case echoes::sim::EntityType::Dropoff:
            return bKharuun
                       ? TEXT("/Game/Art/Generated/Kharuun/Structures/SM_Kharuun_Waystone.SM_Kharuun_Waystone")
                       : TEXT("/Game/Art/Generated/Meridian/Structures/SM_Meridian_PowerLink.SM_Meridian_PowerLink");
        case echoes::sim::EntityType::Barracks:
            return bKharuun
                       ? TEXT("/Game/Art/Generated/Kharuun/Structures/SM_Kharuun_GrowthBasin.SM_Kharuun_GrowthBasin")
                       : TEXT("/Game/Art/Generated/Meridian/Structures/SM_Meridian_ArrayFoundry.SM_Meridian_ArrayFoundry");
        case echoes::sim::EntityType::UtilityStructure:
            return bKharuun
                       ? TEXT("/Game/Art/Generated/Kharuun/Structures/SM_Kharuun_ListeningSpine.SM_Kharuun_ListeningSpine")
                       : TEXT("/Game/Art/Generated/Meridian/Structures/SM_Meridian_AegisPost.SM_Meridian_AegisPost");
        case echoes::sim::EntityType::ResourceNode:
        case echoes::sim::EntityType::FutureWell:
            break;
    }
    return nullptr;
}

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
    if (State.faction == echoes::sim::Faction::HollowChoir)
    {
        return FLinearColor(0.788f, 0.824f, 0.941f);
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

    SilhouetteAccent = CreateDefaultSubobject<UStaticMeshComponent>(
        TEXT("SilhouetteAccent"));
    SilhouetteAccent->SetupAttachment(SceneRoot);
    SilhouetteAccent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SilhouetteAccent->SetGenerateOverlapEvents(false);
    SilhouetteAccent->SetCastShadow(true);
    SilhouetteAccent->SetReceivesDecals(false);
    SilhouetteAccent->SetVisibility(false);

    SelectionRing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SelectionRing"));
    SelectionRing->SetupAttachment(SceneRoot);
    SelectionRing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SelectionRing->SetGenerateOverlapEvents(false);
    SelectionRing->SetCastShadow(false);
    SelectionRing->SetReceivesDecals(false);
    SelectionRing->SetCanEverAffectNavigation(false);
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

    ChoirIdentityField = CreateDefaultSubobject<UStaticMeshComponent>(
        TEXT("ChoirIdentityField"));
    ChoirIdentityField->SetupAttachment(SceneRoot);
    ChoirIdentityField->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ChoirIdentityField->SetGenerateOverlapEvents(false);
    ChoirIdentityField->SetCastShadow(false);
    ChoirIdentityField->SetReceivesDecals(false);
    ChoirIdentityField->SetVisibility(false);

    AegisPowerField = CreateDefaultSubobject<UStaticMeshComponent>(
        TEXT("AegisPowerField"));
    AegisPowerField->SetupAttachment(SceneRoot);
    AegisPowerField->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    AegisPowerField->SetGenerateOverlapEvents(false);
    AegisPowerField->SetCastShadow(false);
    AegisPowerField->SetReceivesDecals(false);
    AegisPowerField->SetVisibility(false);

    FutureWellOrbitOuter = CreateDefaultSubobject<UStaticMeshComponent>(
        TEXT("FutureWellOrbitOuter"));
    FutureWellOrbitInner = CreateDefaultSubobject<UStaticMeshComponent>(
        TEXT("FutureWellOrbitInner"));
    FutureWellCore = CreateDefaultSubobject<UStaticMeshComponent>(
        TEXT("FutureWellCore"));
    FutureWellGroundGlyphA = CreateDefaultSubobject<UStaticMeshComponent>(
        TEXT("FutureWellGroundGlyphA"));
    FutureWellGroundGlyphB = CreateDefaultSubobject<UStaticMeshComponent>(
        TEXT("FutureWellGroundGlyphB"));
    for (UStaticMeshComponent* FutureWellComponent : {
             FutureWellOrbitOuter,
             FutureWellOrbitInner,
             FutureWellCore,
             FutureWellGroundGlyphA,
             FutureWellGroundGlyphB})
    {
        FutureWellComponent->SetupAttachment(SceneRoot);
        FutureWellComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        FutureWellComponent->SetGenerateOverlapEvents(false);
        FutureWellComponent->SetReceivesDecals(false);
        FutureWellComponent->SetCastShadow(true);
        FutureWellComponent->SetVisibility(false);
    }
    FutureWellGroundGlyphA->SetCastShadow(false);
    FutureWellGroundGlyphB->SetCastShadow(false);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(
        TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(
        TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(
        TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeFinder(
        TEXT("/Engine/BasicShapes/Cone.Cone"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> FutureWellOrbitFinder(
        TEXT("/Game/Art/Generated/World/Landmarks/SM_World_FutureWellOrbit.SM_World_FutureWellOrbit"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> FutureWellCoreFinder(
        TEXT("/Game/Art/Generated/World/Landmarks/SM_World_FutureWellCore.SM_World_FutureWellCore"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> FutureWellGlyphFinder(
        TEXT("/Game/Art/Generated/World/Landmarks/SM_World_FutureWellGlyph.SM_World_FutureWellGlyph"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SelectionHaloFinder(
        TEXT("/Game/Art/Generated/VFX/SM_VFX_SelectionHalo.SM_VFX_SelectionHalo"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(
        TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> ArtMaterialFinder(
        TEXT("/Game/Art/Generated/Materials/M_EchoesSurface.M_EchoesSurface"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> WorldMaterialFinder(
        TEXT("/Game/Art/Generated/Materials/M_EchoesWorldSurface.M_EchoesWorldSurface"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> PresentationVFXMaterialFinder(
        TEXT("/Game/Art/Generated/Materials/M_EchoesPresentationVFX.M_EchoesPresentationVFX"));

    CubeMesh = CubeFinder.Object;
    SphereMesh = SphereFinder.Object;
    CylinderMesh = CylinderFinder.Object;
    ConeMesh = ConeFinder.Object;
    FutureWellOrbitMesh = FutureWellOrbitFinder.Object;
    FutureWellCoreMesh = FutureWellCoreFinder.Object;
    FutureWellGlyphMesh = FutureWellGlyphFinder.Object;
    SelectionHaloMesh = SelectionHaloFinder.Object;
    BasicMaterial = MaterialFinder.Object;
    AuthoredSurfaceMaterial = ArtMaterialFinder.Succeeded()
                                  ? ArtMaterialFinder.Object
                                  : BasicMaterial;
    AuthoredWorldSurfaceMaterial = WorldMaterialFinder.Succeeded()
                                       ? WorldMaterialFinder.Object
                                       : AuthoredSurfaceMaterial;
    AuthoredPresentationVFXMaterial = PresentationVFXMaterialFinder.Succeeded()
                                          ? PresentationVFXMaterialFinder.Object
                                          : BasicMaterial;
    bUsingAuthoredSelectionVFX =
        SelectionHaloFinder.Succeeded() && PresentationVFXMaterialFinder.Succeeded();

    BodyMesh->SetStaticMesh(CylinderMesh);
    SilhouetteAccent->SetStaticMesh(CubeMesh);
    SelectionRing->SetStaticMesh(
        SelectionHaloMesh != nullptr ? SelectionHaloMesh : CylinderMesh);
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
    ChoirIdentityField->SetStaticMesh(CylinderMesh);
    ChoirIdentityField->SetRelativeLocation(FVector(0.0f, 0.0f, 6.0f));
    AegisPowerField->SetStaticMesh(CylinderMesh);
    AegisPowerField->SetRelativeLocation(FVector(0.0f, 0.0f, 6.0f));
    AegisPowerField->SetRelativeScale3D(FVector(1.35f, 1.35f, 0.045f));
    FutureWellOrbitOuter->SetStaticMesh(FutureWellOrbitMesh);
    FutureWellOrbitInner->SetStaticMesh(FutureWellOrbitMesh);
    FutureWellCore->SetStaticMesh(FutureWellCoreMesh);
    FutureWellGroundGlyphA->SetStaticMesh(FutureWellGlyphMesh);
    FutureWellGroundGlyphB->SetStaticMesh(FutureWellGlyphMesh);
    SelectionRing->SetRelativeLocation(FVector(0.0f, 0.0f, 3.0f));
    SelectionRing->SetRelativeScale3D(FVector(0.72f, 0.72f, 0.025f));

    Tags.Add(TEXT("EchoesEntityView"));
}

void AEchoesEntityView::ActivateForEntity(
    const echoes::sim::Entity& State,
    bool bTeleport)
{
    if (!bPreparedForPool)
    {
        PrepareForPool();
    }

    bPreparedForPool = false;
    SetActorEnableCollision(true);
    BodyMesh->SetCollisionObjectType(ECC_WorldDynamic);
    BodyMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    BodyMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    BodyMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    BodyMesh->SetGenerateOverlapEvents(false);
    BodyMesh->SetVisibility(true, true);
    SetActorTickEnabled(true);

    // Rebind while still hidden so no partially configured actor can be picked
    // or rendered between pool ownership and authoritative activation.
    ApplyAuthoritativeState(State, bTeleport);
    SetActorHiddenInGame(false);
}

void AEchoesEntityView::PrepareForPool()
{
    SetSelected(false);
    SetActorHiddenInGame(true);
    SetActorTickEnabled(false);
    SetActorEnableCollision(false);
    BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BodyMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    BodyMesh->SetGenerateOverlapEvents(false);

    EntityId = 0;
    OwnerPlayerId = echoes::sim::kNeutralPlayer;
    EntityFaction = echoes::sim::Faction::MeridianCompact;
    EntityType = echoes::sim::EntityType::Worker;
    WellChoice = echoes::sim::FutureWellChoice::Dormant;
    FutureWellVisualChoice = echoes::sim::FutureWellChoice::Dormant;
    HitPoints = 0;
    MaxHitPoints = 0;
    DisplayedHealthFraction = 0.0f;
    HealthBarWidthScale = 0.9f;
    HealthBarHeight = 92.0f;
    BaseBodyColor = FLinearColor::White;
    DamagePulseRemainingSeconds = 0.0f;
    AuthoritativeWorldLocation = FVector::ZeroVector;
    bHasAuthoritativeLocation = false;
    bSelected = false;
    bDeployed = false;
    DeploymentFacing = echoes::sim::Vec2::FromRaw(
        echoes::sim::kFixedScale,
        0);
    bRelaySupplyActive = false;
    WaystoneMode = echoes::sim::WaystoneMode::NotWaystone;
    WarformAdaptation = echoes::sim::WarformAdaptation::None;
    PendingWarformAdaptation = echoes::sim::WarformAdaptation::None;
    ChoirIdentityState = echoes::sim::ChoirIdentityState::NotChoir;
    bTemporaryMineralCover = false;
    bAegisPowered = false;
    bUsingAuthoredRosterMesh = false;
    bUsingAuthoredFutureWellMesh = false;
    bUsingAuthoredResourceMesh = false;
    FutureWellVisualTimeSeconds = 0.0f;
    FutureWellCoreBaseScale = FVector::OneVector;
    SelectionVFXBaseScale = FVector::OneVector;
    SelectionVFXTimeSeconds = 0.0f;
    SelectionVFXEmissiveStrength = 0.0f;
    bSelectionReducedMotionApplied = false;
    bSelectionReducedFlashingApplied = false;
    const int32 MaterialSlotsToClear =
        FMath::Max(ActiveBodyMaterialSlotCount, 4);
    for (int32 MaterialIndex = 0;
         MaterialIndex < MaterialSlotsToClear;
         ++MaterialIndex)
    {
        BodyMesh->SetMaterial(MaterialIndex, nullptr);
    }
    ActiveBodyMaterialFamily = EBodyMaterialFamily::Basic;
    ActiveBodyMaterialSlotCount = 0;
    BodyMaterial = nullptr;
    BodyMaterials.Reset();

    ResetPresentationComponentsForPool();
    ResetOwnedMaterialParameters();
    SetActorTransform(FTransform::Identity, false, nullptr,
                      ETeleportType::TeleportPhysics);
    bPreparedForPool = true;
}

bool AEchoesEntityView::HasBodySelectionCollisionEnabled() const
{
    return GetActorEnableCollision() && BodyMesh != nullptr &&
           BodyMesh->GetCollisionEnabled() == ECollisionEnabled::QueryOnly &&
           BodyMesh->GetCollisionResponseToChannel(ECC_Visibility) == ECR_Block;
}

UMaterialInstanceDynamic* AEchoesEntityView::CreateOwnedMaterial(
    UMaterialInterface* Parent)
{
    if (Parent == nullptr)
    {
        return nullptr;
    }
    UMaterialInstanceDynamic* Material =
        UMaterialInstanceDynamic::Create(Parent, this);
    if (Material != nullptr)
    {
        ++OwnedMIDCreationCount;
    }
    return Material;
}

void AEchoesEntityView::EnsureBodyMaterialSet(
    EBodyMaterialFamily Family,
    UMaterialInterface* Parent,
    int32 MaterialCount)
{
    TArray<TObjectPtr<UMaterialInstanceDynamic>>* Cache = nullptr;
    switch (Family)
    {
        case EBodyMaterialFamily::Basic:
            Cache = &BasicBodyMaterialCache;
            break;
        case EBodyMaterialFamily::AuthoredSurface:
            Cache = &AuthoredBodyMaterialCache;
            break;
        case EBodyMaterialFamily::AuthoredWorldSurface:
            Cache = &WorldBodyMaterialCache;
            break;
    }
    if (Cache == nullptr || Parent == nullptr || MaterialCount <= 0)
    {
        BodyMaterials.Reset();
        BodyMaterial = nullptr;
        ActiveBodyMaterialSlotCount = 0;
        return;
    }

    while (Cache->Num() < MaterialCount)
    {
        Cache->Add(CreateOwnedMaterial(Parent));
    }

    const int32 SlotsToClear = FMath::Max(ActiveBodyMaterialSlotCount, 4);
    for (int32 MaterialIndex = 0; MaterialIndex < SlotsToClear; ++MaterialIndex)
    {
        BodyMesh->SetMaterial(MaterialIndex, nullptr);
    }
    BodyMaterials.Reset(MaterialCount);
    for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
    {
        UMaterialInstanceDynamic* Material = (*Cache)[MaterialIndex];
        BodyMaterials.Add(Material);
        BodyMesh->SetMaterial(MaterialIndex, Material);
    }
    BodyMaterial = BodyMaterials.IsEmpty() ? nullptr : BodyMaterials[0];
    ActiveBodyMaterialFamily = Family;
    ActiveBodyMaterialSlotCount = MaterialCount;
}

void AEchoesEntityView::ResetOwnedMaterialParameters()
{
    const auto ResetMaterial = [](UMaterialInstanceDynamic* Material)
    {
        if (Material == nullptr)
        {
            return;
        }
        Material->SetVectorParameterValue(
            EntityColorParameterName,
            FLinearColor::White);
        Material->SetScalarParameterValue(MetallicParameterName, 0.0f);
        Material->SetScalarParameterValue(RoughnessParameterName, 0.5f);
        Material->SetScalarParameterValue(EmissiveStrengthParameterName, 0.0f);
    };
    const auto ResetArray = [&ResetMaterial](
                                const TArray<TObjectPtr<UMaterialInstanceDynamic>>& Materials)
    {
        for (UMaterialInstanceDynamic* Material : Materials)
        {
            ResetMaterial(Material);
        }
    };
    ResetArray(BasicBodyMaterialCache);
    ResetArray(AuthoredBodyMaterialCache);
    ResetArray(WorldBodyMaterialCache);
    ResetArray(FutureWellOrbitOuterMaterials);
    ResetArray(FutureWellOrbitInnerMaterials);
    ResetArray(FutureWellCoreMaterials);
    ResetArray(FutureWellGroundGlyphAMaterials);
    ResetArray(FutureWellGroundGlyphBMaterials);
    for (UMaterialInstanceDynamic* Material : {
             SilhouetteAccentMaterial,
             RingMaterial,
             HealthBarBackgroundMaterial,
             HealthBarFillMaterial,
             OwnerMarkerMaterial,
             DeploymentCoverMaterial,
             RelaySupplyFieldMaterial,
             WaystoneStateFieldMaterial,
             WarformStateFieldMaterial,
             ChoirIdentityFieldMaterial,
             AegisPowerFieldMaterial})
    {
        ResetMaterial(Material);
    }
}

void AEchoesEntityView::ResetPresentationComponentsForPool()
{
    BodyMesh->SetRenderCustomDepth(false);
    BodyMesh->SetCustomDepthStencilValue(0);
    BodyMesh->SetCustomPrimitiveDataFloat(0, 0.0f);
    BodyMesh->SetRelativeLocation(FVector::ZeroVector);
    BodyMesh->SetRelativeRotation(FRotator::ZeroRotator);
    BodyMesh->SetRelativeScale3D(FVector::OneVector);

    SilhouetteAccent->SetVisibility(false, true);
    SilhouetteAccent->SetRenderCustomDepth(false);
    SilhouetteAccent->SetCustomDepthStencilValue(0);
    SilhouetteAccent->SetRelativeLocation(FVector::ZeroVector);
    SilhouetteAccent->SetRelativeRotation(FRotator::ZeroRotator);
    SilhouetteAccent->SetRelativeScale3D(FVector::OneVector);

    SelectionRing->SetVisibility(false, true);
    SelectionRing->SetRelativeLocation(FVector(0.0f, 0.0f, 3.0f));
    SelectionRing->SetRelativeRotation(FRotator::ZeroRotator);
    SelectionRing->SetRelativeScale3D(FVector::OneVector);

    for (UStaticMeshComponent* Component : {
             HealthBarBackground,
             HealthBarFill,
             OwnerMarker,
             DeploymentCover,
             RelaySupplyField,
             WaystoneStateField,
             WarformStateField,
             ChoirIdentityField,
             AegisPowerField,
             FutureWellOrbitOuter,
             FutureWellOrbitInner,
             FutureWellCore,
             FutureWellGroundGlyphA,
             FutureWellGroundGlyphB})
    {
        Component->SetVisibility(false, true);
        Component->SetRelativeRotation(FRotator::ZeroRotator);
        Component->SetRelativeScale3D(FVector::OneVector);
    }
    HealthBarBackground->SetRelativeLocation(FVector::ZeroVector);
    HealthBarFill->SetRelativeLocation(FVector::ZeroVector);
    OwnerMarker->SetRelativeLocation(FVector::ZeroVector);
    OwnerMarker->SetStaticMesh(nullptr);
    DeploymentCover->SetRelativeLocation(FVector::ZeroVector);
    RelaySupplyField->SetRelativeLocation(FVector(0.0f, 0.0f, 5.0f));
    WaystoneStateField->SetRelativeLocation(FVector(0.0f, 0.0f, 4.0f));
    WarformStateField->SetRelativeLocation(FVector(0.0f, 0.0f, 5.0f));
    ChoirIdentityField->SetRelativeLocation(FVector(0.0f, 0.0f, 6.0f));
    AegisPowerField->SetRelativeLocation(FVector(0.0f, 0.0f, 6.0f));
    FutureWellOrbitOuter->SetRelativeLocation(FVector::ZeroVector);
    FutureWellOrbitInner->SetRelativeLocation(FVector::ZeroVector);
    FutureWellCore->SetRelativeLocation(FVector::ZeroVector);
    FutureWellGroundGlyphA->SetRelativeLocation(FVector::ZeroVector);
    FutureWellGroundGlyphB->SetRelativeLocation(FVector::ZeroVector);
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
    if (bSelected && SelectionRing != nullptr && RingMaterial != nullptr)
    {
        bSelectionReducedMotionApplied =
            Settings != nullptr && Settings->IsReducedMotionEnabled();
        bSelectionReducedFlashingApplied =
            Settings != nullptr && Settings->IsReducedFlashingEnabled();
        SelectionVFXTimeSeconds += DeltaSeconds;
        if (!bSelectionReducedMotionApplied)
        {
            SelectionRing->SetRelativeRotation(
                FRotator(0.0f, SelectionVFXTimeSeconds * 14.0f, 0.0f));
            const float BreathingScale =
                1.0f + 0.028f * FMath::Sin(SelectionVFXTimeSeconds * 2.2f);
            SelectionRing->SetRelativeScale3D(
                SelectionVFXBaseScale * BreathingScale);
        }
        else
        {
            SelectionRing->SetRelativeRotation(FRotator::ZeroRotator);
            SelectionRing->SetRelativeScale3D(SelectionVFXBaseScale);
        }
        SelectionVFXEmissiveStrength = bSelectionReducedFlashingApplied
                                           ? 1.25f
                                           : 1.9f + 0.14f *
                                                 FMath::Sin(
                                                     SelectionVFXTimeSeconds *
                                                     1.4f);
        RingMaterial->SetScalarParameterValue(
            EmissiveStrengthParameterName,
            SelectionVFXEmissiveStrength);
    }
    if (EntityType == echoes::sim::EntityType::FutureWell &&
        bUsingAuthoredFutureWellMesh &&
        FutureWellOrbitOuter != nullptr &&
        FutureWellOrbitInner != nullptr &&
        FutureWellCore != nullptr)
    {
        const bool bReducedMotion =
            Settings != nullptr && Settings->IsReducedMotionEnabled();
        if (!bReducedMotion)
        {
            FutureWellVisualTimeSeconds += DeltaSeconds;
            const float OuterSpeed =
                FutureWellVisualChoice == echoes::sim::FutureWellChoice::Harvest
                    ? 10.0f
                    : FutureWellVisualChoice == echoes::sim::FutureWellChoice::Reshape
                          ? 8.0f
                          : 4.0f;
            FutureWellOrbitOuter->AddLocalRotation(
                FRotator(0.0f, OuterSpeed * DeltaSeconds, 0.0f));
            FutureWellOrbitInner->AddLocalRotation(
                FRotator(0.0f, -OuterSpeed * 1.45f * DeltaSeconds, 0.0f));
            const float Pulse =
                1.0f + 0.035f * FMath::Sin(FutureWellVisualTimeSeconds * 1.6f);
            FutureWellCore->SetRelativeScale3D(
                FutureWellCoreBaseScale * Pulse);
        }
        else
        {
            FutureWellCore->SetRelativeScale3D(FutureWellCoreBaseScale);
        }
    }
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
                                  ChoirIdentityState !=
                                      State.choirIdentityState ||
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
    ChoirIdentityState = State.choirIdentityState;
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
    const bool bKharuun =
        State.faction == echoes::sim::Faction::KharuunAssemblies;
    const bool bChoir =
        State.faction == echoes::sim::Faction::HollowChoir;
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

    bUsingAuthoredRosterMesh = false;
    bUsingAuthoredFutureWellMesh = false;
    bUsingAuthoredResourceMesh = false;
    if (!State.temporaryMineralCover)
    {
        const TCHAR* AuthoredPath =
            AuthoredPresentationMeshPath(State.faction, State.type);
        if (AuthoredPath != nullptr)
        {
            if (UStaticMesh* AuthoredMesh =
                    LoadObject<UStaticMesh>(nullptr, AuthoredPath))
            {
                DesiredMesh = AuthoredMesh;
                BodyScale = FVector::OneVector;
                BodyOffset = FVector::ZeroVector;
                bUsingAuthoredFutureWellMesh =
                    State.type == echoes::sim::EntityType::FutureWell;
                bUsingAuthoredResourceMesh =
                    State.type == echoes::sim::EntityType::ResourceNode;
                bUsingAuthoredRosterMesh =
                    !bUsingAuthoredFutureWellMesh &&
                    !bUsingAuthoredResourceMesh;

                switch (State.type)
                {
                    case echoes::sim::EntityType::Worker:
                        SelectionRadius = bChoir ? 1.40f : bKharuun ? 1.35f : 1.45f;
                        HealthBarWidthScale = 1.20f;
                        HealthBarHeight = bChoir ? 172.0f : bKharuun ? 148.0f : 194.0f;
                        break;
                    case echoes::sim::EntityType::Soldier:
                        SelectionRadius = bChoir ? 1.50f : bKharuun ? 1.55f : 1.25f;
                        HealthBarWidthScale = 1.35f;
                        HealthBarHeight = bChoir ? 178.0f : bKharuun ? 146.0f : 166.0f;
                        break;
                    case echoes::sim::EntityType::HeavyUnit:
                        SelectionRadius = bChoir ? 1.90f : bKharuun ? 1.95f : 1.70f;
                        HealthBarWidthScale = 1.65f;
                        HealthBarHeight = bChoir ? 180.0f : bKharuun ? 158.0f : 132.0f;
                        break;
                    case echoes::sim::EntityType::ScoutUnit:
                        SelectionRadius = bChoir ? 1.85f : bKharuun ? 1.55f : 1.85f;
                        HealthBarWidthScale = 1.40f;
                        HealthBarHeight = bChoir ? 135.0f : bKharuun ? 216.0f : 126.0f;
                        break;
                    case echoes::sim::EntityType::CommandCore:
                        SelectionRadius = 3.95f;
                        HealthBarWidthScale = 3.10f;
                        HealthBarHeight = bChoir ? 286.0f : bKharuun ? 214.0f : 286.0f;
                        break;
                    case echoes::sim::EntityType::Dropoff:
                        SelectionRadius = bChoir ? 2.55f : bKharuun ? 2.55f : 3.10f;
                        HealthBarWidthScale = 2.35f;
                        HealthBarHeight = bChoir ? 225.0f : bKharuun ? 232.0f : 302.0f;
                        break;
                    case echoes::sim::EntityType::Barracks:
                        SelectionRadius = 3.55f;
                        HealthBarWidthScale = 3.00f;
                        HealthBarHeight = bChoir ? 225.0f : bKharuun ? 146.0f : 182.0f;
                        break;
                    case echoes::sim::EntityType::UtilityStructure:
                        SelectionRadius = bChoir ? 2.65f : bKharuun ? 2.65f : 2.90f;
                        HealthBarWidthScale = 2.25f;
                        HealthBarHeight = bChoir ? 330.0f : bKharuun ? 332.0f : 232.0f;
                        break;
                    case echoes::sim::EntityType::ResourceNode:
                        SelectionRadius = 1.45f;
                        HealthBarWidthScale = 1.25f;
                        HealthBarHeight = 278.0f;
                        break;
                    case echoes::sim::EntityType::FutureWell:
                        SelectionRadius = 3.15f;
                        HealthBarWidthScale = 2.75f;
                        HealthBarHeight = 322.0f;
                        break;
                }
            }
        }
    }

    BodyMesh->SetStaticMesh(DesiredMesh);
    BodyMesh->SetRelativeScale3D(BodyScale);
    BodyMesh->SetRelativeLocation(BodyOffset);
    UStaticMesh* AccentMesh = CubeMesh;
    FVector AccentScale(0.18f, 0.54f, 0.12f);
    FVector AccentOffset(0.0f, 0.0f, HealthBarHeight - 28.0f);
    FRotator AccentRotation = FRotator::ZeroRotator;
    bool bShowSilhouetteAccent = !State.temporaryMineralCover;
    switch (State.type)
    {
        case echoes::sim::EntityType::Worker:
            AccentMesh = bKharuun ? ConeMesh : CubeMesh;
            AccentScale = bKharuun
                              ? FVector(0.22f, 0.22f, 0.32f)
                              : FVector(0.18f, 0.44f, 0.12f);
            AccentOffset.Z = 72.0f;
            AccentRotation.Yaw = bKharuun ? 45.0f : 0.0f;
            break;
        case echoes::sim::EntityType::Soldier:
            AccentMesh = bKharuun ? ConeMesh : CubeMesh;
            AccentScale = bKharuun
                              ? FVector(0.30f, 0.30f, 0.40f)
                              : FVector(0.14f, 0.62f, 0.14f);
            AccentOffset.Z = bKharuun ? 93.0f : 82.0f;
            AccentRotation.Yaw = bKharuun ? 45.0f : 0.0f;
            break;
        case echoes::sim::EntityType::HeavyUnit:
            AccentMesh = bKharuun ? ConeMesh : CubeMesh;
            AccentScale = bKharuun
                              ? FVector(0.42f, 0.42f, 0.48f)
                              : FVector(0.94f, 0.20f, 0.14f);
            AccentOffset.Z = bKharuun ? 78.0f : 72.0f;
            AccentRotation.Yaw = bKharuun ? 45.0f : 0.0f;
            break;
        case echoes::sim::EntityType::ScoutUnit:
            AccentMesh = bKharuun ? ConeMesh : SphereMesh;
            AccentScale = bKharuun
                              ? FVector(0.24f, 0.24f, 0.34f)
                              : FVector(0.18f, 0.54f, 0.12f);
            AccentOffset.Z = 72.0f;
            AccentRotation.Yaw = bKharuun ? 45.0f : 0.0f;
            break;
        case echoes::sim::EntityType::CommandCore:
            AccentMesh = bKharuun ? ConeMesh : CylinderMesh;
            AccentScale = bKharuun
                              ? FVector(0.76f, 0.76f, 0.82f)
                              : FVector(1.05f, 1.05f, 0.20f);
            AccentOffset.Z = bKharuun ? 150.0f : 132.0f;
            AccentRotation.Yaw = bKharuun ? 45.0f : 0.0f;
            break;
        case echoes::sim::EntityType::Dropoff:
            AccentMesh = bKharuun ? ConeMesh : CylinderMesh;
            AccentScale = bKharuun
                              ? FVector(0.56f, 0.56f, 0.62f)
                              : FVector(0.72f, 0.72f, 0.18f);
            AccentOffset.Z = bKharuun ? 103.0f : 87.0f;
            AccentRotation.Yaw = bKharuun ? 45.0f : 0.0f;
            break;
        case echoes::sim::EntityType::Barracks:
            AccentMesh = bKharuun ? SphereMesh : CubeMesh;
            AccentScale = bKharuun
                              ? FVector(0.62f, 0.62f, 0.42f)
                              : FVector(1.28f, 0.28f, 0.18f);
            AccentOffset.Z = 102.0f;
            AccentRotation.Yaw = bKharuun ? 0.0f : 90.0f;
            break;
        case echoes::sim::EntityType::UtilityStructure:
            AccentMesh = bKharuun ? ConeMesh : SphereMesh;
            AccentScale = bKharuun
                              ? FVector(0.44f, 0.44f, 0.62f)
                              : FVector(0.42f, 0.42f, 0.24f);
            AccentOffset.Z = 154.0f;
            AccentRotation.Yaw = bKharuun ? 45.0f : 0.0f;
            break;
        case echoes::sim::EntityType::ResourceNode:
        case echoes::sim::EntityType::FutureWell:
            bShowSilhouetteAccent = false;
            break;
    }
    if (bChoir &&
        State.type != echoes::sim::EntityType::ResourceNode &&
        State.type != echoes::sim::EntityType::FutureWell)
    {
        AccentMesh = SphereMesh;
        AccentScale = FVector(0.26f, 0.26f, 0.12f);
        AccentOffset.Z = FMath::Max(64.0f, HealthBarHeight - 34.0f);
        AccentRotation = FRotator(0.0f, 45.0f, 0.0f);
    }
    SilhouetteAccent->SetStaticMesh(AccentMesh);
    SilhouetteAccent->SetRelativeScale3D(AccentScale);
    SilhouetteAccent->SetRelativeLocation(AccentOffset);
    SilhouetteAccent->SetRelativeRotation(AccentRotation);
    SilhouetteAccent->SetVisibility(
        bShowSilhouetteAccent && !bUsingAuthoredRosterMesh &&
            !bUsingAuthoredFutureWellMesh,
        true);
    SelectionRing->SetRelativeScale3D(FVector(
        SelectionRadius,
        SelectionRadius,
        SelectionHaloMesh != nullptr ? 1.0f : 0.025f));
    SelectionVFXBaseScale = SelectionRing->GetRelativeScale3D();

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
    const bool bIsChoirIdentityUnit =
        State.faction == echoes::sim::Faction::HollowChoir &&
        (State.type == echoes::sim::EntityType::Soldier ||
         State.type == echoes::sim::EntityType::HeavyUnit ||
         State.type == echoes::sim::EntityType::ScoutUnit) &&
        State.choirIdentityState !=
            echoes::sim::ChoirIdentityState::NotChoir;
    if (bIsChoirIdentityUnit)
    {
        switch (State.choirIdentityState)
        {
            case echoes::sim::ChoirIdentityState::Manifest:
                ChoirIdentityField->SetStaticMesh(CubeMesh);
                ChoirIdentityField->SetRelativeScale3D(
                    FVector(0.62f, 0.62f, 0.055f));
                break;
            case echoes::sim::ChoirIdentityState::Possible:
                ChoirIdentityField->SetStaticMesh(SphereMesh);
                ChoirIdentityField->SetRelativeScale3D(
                    FVector(0.34f, 0.34f, 0.12f));
                break;
            case echoes::sim::ChoirIdentityState::DualResolveManifest:
            case echoes::sim::ChoirIdentityState::DualResolvePossible:
                ChoirIdentityField->SetStaticMesh(CylinderMesh);
                ChoirIdentityField->SetRelativeScale3D(
                    FVector(0.96f, 0.96f, 0.045f));
                break;
            case echoes::sim::ChoirIdentityState::NotChoir:
                break;
        }
    }
    ChoirIdentityField->SetVisibility(bIsChoirIdentityUnit, true);
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
        const bool bUsingAuthoredPresentationMesh =
            bUsingAuthoredRosterMesh || bUsingAuthoredFutureWellMesh ||
            bUsingAuthoredResourceMesh;
        const int32 BodyMaterialCount =
            bUsingAuthoredPresentationMesh ? 4 : 1;
        UMaterialInterface* BodyParent =
            bUsingAuthoredResourceMesh && AuthoredWorldSurfaceMaterial != nullptr
                ? AuthoredWorldSurfaceMaterial
            : bUsingAuthoredPresentationMesh && AuthoredSurfaceMaterial != nullptr
                ? AuthoredSurfaceMaterial
                : BasicMaterial;
        const EBodyMaterialFamily BodyFamily =
            bUsingAuthoredResourceMesh
                ? EBodyMaterialFamily::AuthoredWorldSurface
            : bUsingAuthoredPresentationMesh
                ? EBodyMaterialFamily::AuthoredSurface
                : EBodyMaterialFamily::Basic;
        EnsureBodyMaterialSet(BodyFamily, BodyParent, BodyMaterialCount);
        if (SilhouetteAccentMaterial == nullptr)
        {
            SilhouetteAccentMaterial =
                CreateOwnedMaterial(BasicMaterial);
            SilhouetteAccent->SetMaterial(0, SilhouetteAccentMaterial);
        }
        if (RingMaterial == nullptr)
        {
            RingMaterial = CreateOwnedMaterial(AuthoredPresentationVFXMaterial);
            SelectionRing->SetMaterial(0, RingMaterial);
        }
        if (HealthBarBackgroundMaterial == nullptr)
        {
            HealthBarBackgroundMaterial =
                CreateOwnedMaterial(BasicMaterial);
            HealthBarBackground->SetMaterial(0, HealthBarBackgroundMaterial);
        }
        if (HealthBarFillMaterial == nullptr)
        {
            HealthBarFillMaterial =
                CreateOwnedMaterial(BasicMaterial);
            HealthBarFill->SetMaterial(0, HealthBarFillMaterial);
        }
        if (OwnerMarkerMaterial == nullptr)
        {
            OwnerMarkerMaterial = CreateOwnedMaterial(BasicMaterial);
            OwnerMarker->SetMaterial(0, OwnerMarkerMaterial);
        }
        if (DeploymentCoverMaterial == nullptr)
        {
            DeploymentCoverMaterial =
                CreateOwnedMaterial(BasicMaterial);
            DeploymentCover->SetMaterial(0, DeploymentCoverMaterial);
        }
        if (RelaySupplyFieldMaterial == nullptr)
        {
            RelaySupplyFieldMaterial =
                CreateOwnedMaterial(BasicMaterial);
            RelaySupplyField->SetMaterial(0, RelaySupplyFieldMaterial);
        }
        if (WaystoneStateFieldMaterial == nullptr)
        {
            WaystoneStateFieldMaterial =
                CreateOwnedMaterial(BasicMaterial);
            WaystoneStateField->SetMaterial(0, WaystoneStateFieldMaterial);
        }
        if (WarformStateFieldMaterial == nullptr)
        {
            WarformStateFieldMaterial =
                CreateOwnedMaterial(BasicMaterial);
            WarformStateField->SetMaterial(0, WarformStateFieldMaterial);
        }
        if (ChoirIdentityFieldMaterial == nullptr)
        {
            ChoirIdentityFieldMaterial =
                CreateOwnedMaterial(BasicMaterial);
            ChoirIdentityField->SetMaterial(
                0, ChoirIdentityFieldMaterial);
        }
        if (ChoirIdentityFieldMaterial != nullptr)
        {
            const bool bPossibleIdentity =
                State.choirIdentityState ==
                    echoes::sim::ChoirIdentityState::Possible ||
                State.choirIdentityState ==
                    echoes::sim::ChoirIdentityState::DualResolvePossible;
            const bool bDualIdentity =
                State.choirIdentityState ==
                    echoes::sim::ChoirIdentityState::DualResolveManifest ||
                State.choirIdentityState ==
                    echoes::sim::ChoirIdentityState::DualResolvePossible;
            ChoirIdentityFieldMaterial->SetVectorParameterValue(
                EntityColorParameterName,
                bDualIdentity
                    ? FLinearColor(0.86f, 0.54f, 0.76f)
                    : bPossibleIdentity
                          ? FLinearColor(0.851f, 0.412f, 0.553f)
                          : FLinearColor(0.788f, 0.824f, 0.941f));
        }
        if (AegisPowerFieldMaterial == nullptr)
        {
            AegisPowerFieldMaterial =
                CreateOwnedMaterial(BasicMaterial);
            AegisPowerField->SetMaterial(0, AegisPowerFieldMaterial);
        }
        if (RingMaterial != nullptr)
        {
            RingMaterial->SetVectorParameterValue(
                EntityColorParameterName,
                FLinearColor(0.08f, 1.0f, 0.68f));
            RingMaterial->SetScalarParameterValue(
                EmissiveStrengthParameterName,
                1.9f);
        }
        if (HealthBarBackgroundMaterial != nullptr)
        {
            HealthBarBackgroundMaterial->SetVectorParameterValue(
                EntityColorParameterName,
                FLinearColor(0.008f, 0.012f, 0.018f));
        }
        if (DeploymentCoverMaterial != nullptr)
        {
            DeploymentCoverMaterial->SetVectorParameterValue(
                EntityColorParameterName,
                FLinearColor(0.10f, 0.88f, 0.92f));
        }
        if (RelaySupplyFieldMaterial != nullptr)
        {
            RelaySupplyFieldMaterial->SetVectorParameterValue(
                EntityColorParameterName,
                FLinearColor(0.95f, 0.76f, 0.18f));
        }
        if (WaystoneStateFieldMaterial != nullptr)
        {
            WaystoneStateFieldMaterial->SetVectorParameterValue(
                EntityColorParameterName,
                FLinearColor(0.58f, 0.22f, 0.92f));
        }
        if (WarformStateFieldMaterial != nullptr)
        {
            WarformStateFieldMaterial->SetVectorParameterValue(
                EntityColorParameterName,
                FLinearColor(0.88f, 0.56f, 0.14f));
        }
        if (AegisPowerFieldMaterial != nullptr)
        {
            AegisPowerFieldMaterial->SetVectorParameterValue(
                EntityColorParameterName,
                FLinearColor(0.98f, 0.84f, 0.18f));
        }
        const FLinearColor TeamColor = ColorForState(State);
        BaseBodyColor = bUsingAuthoredFutureWellMesh
                            ? FLinearColor(0.030f, 0.034f, 0.042f)
                        : bUsingAuthoredResourceMesh
                            ? FLinearColor(0.055f, 0.075f, 0.085f)
                            : TeamColor;
        SetBodyColor(BaseBodyColor);
        if (bUsingAuthoredRosterMesh && BodyMaterials.Num() >= 4)
        {
            const FLinearColor DarkColor =
                bChoir
                    ? FLinearColor(0.014f, 0.019f, 0.038f)
                : bKharuun
                    ? FLinearColor(0.022f, 0.016f, 0.020f)
                    : FLinearColor(0.018f, 0.028f, 0.042f);
            const FLinearColor LightColor =
                bChoir
                    ? FLinearColor(0.788f, 0.824f, 0.941f)
                : bKharuun
                    ? FLinearColor(0.30f, 0.21f, 0.12f)
                    : FLinearColor(0.38f, 0.31f, 0.20f);
            const FLinearColor GlowColor =
                bChoir
                    ? FLinearColor(0.851f, 0.412f, 0.553f)
                : bKharuun
                    ? FLinearColor(1.0f, 0.26f, 0.015f)
                    : FLinearColor(0.018f, 0.82f, 1.0f);
            const FLinearColor SlotColors[] = {
                TeamColor,
                DarkColor,
                LightColor,
                GlowColor};
            const float MetallicValues[] = {0.30f, 0.48f, 0.08f, 0.22f};
            const float RoughnessValues[] = {0.34f, 0.28f, 0.52f, 0.20f};
            const float EmissiveValues[] = {0.0f, 0.0f, 0.0f, 1.8f};
            for (int32 MaterialIndex = 0; MaterialIndex < 4; ++MaterialIndex)
            {
                UMaterialInstanceDynamic* Material =
                    BodyMaterials[MaterialIndex];
                Material->SetVectorParameterValue(
                    EntityColorParameterName,
                    SlotColors[MaterialIndex]);
                Material->SetScalarParameterValue(
                    MetallicParameterName,
                    MetallicValues[MaterialIndex]);
                Material->SetScalarParameterValue(
                    RoughnessParameterName,
                    RoughnessValues[MaterialIndex]);
                Material->SetScalarParameterValue(
                    EmissiveStrengthParameterName,
                    EmissiveValues[MaterialIndex]);
            }
        }
        if (bUsingAuthoredResourceMesh && BodyMaterials.Num() >= 4)
        {
            const FLinearColor SlotColors[] = {
                FLinearColor(0.055f, 0.075f, 0.085f),
                FLinearColor(0.012f, 0.020f, 0.027f),
                FLinearColor(0.30f, 0.55f, 0.60f),
                FLinearColor(0.56f, 0.94f, 1.0f)};
            const float MetallicValues[] = {0.16f, 0.48f, 0.22f, 0.10f};
            const float RoughnessValues[] = {0.72f, 0.18f, 0.30f, 0.12f};
            const float EmissiveValues[] = {0.0f, 0.0f, 0.18f, 2.5f};
            for (int32 MaterialIndex = 0; MaterialIndex < 4; ++MaterialIndex)
            {
                UMaterialInstanceDynamic* Material =
                    BodyMaterials[MaterialIndex];
                Material->SetVectorParameterValue(
                    EntityColorParameterName,
                    SlotColors[MaterialIndex]);
                Material->SetScalarParameterValue(
                    MetallicParameterName,
                    MetallicValues[MaterialIndex]);
                Material->SetScalarParameterValue(
                    RoughnessParameterName,
                    RoughnessValues[MaterialIndex]);
                Material->SetScalarParameterValue(
                    EmissiveStrengthParameterName,
                    EmissiveValues[MaterialIndex]);
            }
        }
        SilhouetteAccentMaterial->SetVectorParameterValue(
            EntityColorParameterName,
            bKharuun
                ? FLinearColor(1.0f, 0.34f, 0.08f)
                : FLinearColor(0.42f, 0.96f, 1.0f));
        OwnerMarkerMaterial->SetVectorParameterValue(
            EntityColorParameterName,
            TeamColor);
    }

    ConfigureFutureWellPresentation(State);
}

void AEchoesEntityView::EnsureFutureWellMaterialSet(
    UStaticMeshComponent* Component,
    TArray<TObjectPtr<UMaterialInstanceDynamic>>& Materials)
{
    if (Component == nullptr || AuthoredSurfaceMaterial == nullptr)
    {
        return;
    }
    if (Materials.Num() != 4)
    {
        Materials.Reset();
        for (int32 MaterialIndex = 0; MaterialIndex < 4; ++MaterialIndex)
        {
            UMaterialInstanceDynamic* Material =
                CreateOwnedMaterial(AuthoredSurfaceMaterial);
            Materials.Add(Material);
        }
    }
    for (int32 MaterialIndex = 0; MaterialIndex < Materials.Num(); ++MaterialIndex)
    {
        Component->SetMaterial(MaterialIndex, Materials[MaterialIndex]);
    }
}

void AEchoesEntityView::ConfigureFutureWellPresentation(
    const echoes::sim::Entity& State)
{
    const bool bVisible =
        State.type == echoes::sim::EntityType::FutureWell &&
        bUsingAuthoredFutureWellMesh &&
        FutureWellOrbitMesh != nullptr &&
        FutureWellCoreMesh != nullptr &&
        FutureWellGlyphMesh != nullptr;
    if (!bVisible)
    {
        for (UStaticMeshComponent* Component : {
                 FutureWellOrbitOuter,
                 FutureWellOrbitInner,
                 FutureWellCore,
                 FutureWellGroundGlyphA,
                 FutureWellGroundGlyphB})
        {
            if (Component != nullptr)
            {
                Component->SetVisibility(false, true);
            }
        }
        return;
    }

    FutureWellOrbitOuter->SetStaticMesh(FutureWellOrbitMesh);
    FutureWellOrbitInner->SetStaticMesh(FutureWellOrbitMesh);
    FutureWellCore->SetStaticMesh(FutureWellCoreMesh);
    FutureWellGroundGlyphA->SetStaticMesh(FutureWellGlyphMesh);
    FutureWellGroundGlyphB->SetStaticMesh(FutureWellGlyphMesh);

    FVector OuterLocation(0.0f, 0.0f, 205.0f);
    FVector InnerLocation(0.0f, 0.0f, 145.0f);
    FVector CoreLocation(0.0f, 0.0f, 154.0f);
    FVector GlyphLocation(0.0f, 0.0f, 58.0f);
    FVector OuterScale(0.92f);
    FVector InnerScale(0.62f);
    FVector CoreScale(0.72f, 0.72f, 1.0f);
    FVector GlyphAScale(1.0f);
    FVector GlyphBScale(1.0f);
    FRotator OuterRotation = FRotator::ZeroRotator;
    FRotator InnerRotation(0.0f, 18.0f, 0.0f);
    FRotator GlyphARotation = FRotator::ZeroRotator;
    FRotator GlyphBRotation = FRotator::ZeroRotator;
    bool bShowGlyphA = false;
    bool bShowGlyphB = false;

    FLinearColor SecondaryColor(0.28f, 0.24f, 0.18f);
    FLinearColor GlowColor(1.0f, 0.58f, 0.10f);
    float SecondaryEmissive = 0.05f;
    float GlowEmissive = 1.25f;

    FutureWellVisualChoice = State.wellChoice;
#if !UE_BUILD_SHIPPING
    FString PreviewChoice;
    if (FParse::Value(
            FCommandLine::Get(),
            TEXT("EchoesFutureWellPreview="),
            PreviewChoice))
    {
        if (PreviewChoice.Equals(TEXT("Harvest"), ESearchCase::IgnoreCase))
        {
            FutureWellVisualChoice = echoes::sim::FutureWellChoice::Harvest;
        }
        else if (PreviewChoice.Equals(TEXT("Preserve"), ESearchCase::IgnoreCase))
        {
            FutureWellVisualChoice = echoes::sim::FutureWellChoice::Preserve;
        }
        else if (PreviewChoice.Equals(TEXT("Reshape"), ESearchCase::IgnoreCase))
        {
            FutureWellVisualChoice = echoes::sim::FutureWellChoice::Reshape;
        }
        else
        {
            FutureWellVisualChoice = echoes::sim::FutureWellChoice::Dormant;
        }
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_FUTURE_WELL_PREVIEW] requested=%s visualChoice=%u authoritativeChoice=%u editorOnly=true"),
            *PreviewChoice,
            static_cast<uint8>(FutureWellVisualChoice),
            static_cast<uint8>(State.wellChoice));
    }
#endif

    switch (FutureWellVisualChoice)
    {
        case echoes::sim::FutureWellChoice::Harvest:
            OuterLocation.Z = 145.0f;
            InnerLocation.Z = 105.0f;
            CoreLocation.Z = 106.0f;
            OuterScale = FVector(0.72f);
            InnerScale = FVector(0.48f);
            CoreScale = FVector(0.55f, 0.55f, 1.40f);
            GlyphAScale = FVector(0.66f, 0.66f, 0.35f);
            OuterRotation = FRotator(18.0f, 0.0f, 0.0f);
            InnerRotation = FRotator(-22.0f, 30.0f, 0.0f);
            bShowGlyphA = true;
            SecondaryColor = FLinearColor(0.72f, 0.12f, 0.025f);
            GlowColor = FLinearColor(1.0f, 0.46f, 0.045f);
            SecondaryEmissive = 0.45f;
            GlowEmissive = 3.1f;
            break;
        case echoes::sim::FutureWellChoice::Preserve:
            OuterLocation.Z = 224.0f;
            InnerLocation.Z = 164.0f;
            CoreLocation.Z = 160.0f;
            OuterScale = FVector(1.0f);
            InnerScale = FVector(0.78f);
            CoreScale = FVector(0.90f, 0.90f, 1.10f);
            GlyphAScale = FVector(1.0f, 1.0f, 0.40f);
            GlyphBScale = FVector(0.68f, 0.68f, 0.42f);
            InnerRotation = FRotator(0.0f, 28.0f, 0.0f);
            GlyphBRotation.Yaw = 22.5f;
            bShowGlyphA = true;
            bShowGlyphB = true;
            SecondaryColor = FLinearColor(0.04f, 0.62f, 0.78f);
            GlowColor = FLinearColor(1.0f, 0.72f, 0.16f);
            SecondaryEmissive = 0.85f;
            GlowEmissive = 2.4f;
            break;
        case echoes::sim::FutureWellChoice::Reshape:
            OuterLocation.Z = 188.0f;
            InnerLocation.Z = 172.0f;
            CoreLocation.Z = 150.0f;
            OuterScale = FVector(0.94f);
            InnerScale = FVector(0.82f);
            CoreScale = FVector(0.82f, 0.82f, 1.12f);
            GlyphAScale = FVector(1.38f, 0.42f, 0.42f);
            GlyphBScale = FVector(1.38f, 0.42f, 0.42f);
            OuterRotation = FRotator(54.0f, 15.0f, 12.0f);
            InnerRotation = FRotator(-50.0f, 72.0f, -8.0f);
            GlyphARotation.Yaw = 45.0f;
            GlyphBRotation.Yaw = -45.0f;
            bShowGlyphA = true;
            bShowGlyphB = true;
            SecondaryColor = FLinearColor(0.025f, 0.66f, 0.82f);
            GlowColor = FLinearColor(0.62f, 0.12f, 1.0f);
            SecondaryEmissive = 1.0f;
            GlowEmissive = 2.8f;
            break;
        case echoes::sim::FutureWellChoice::Dormant:
            break;
    }

    FutureWellOrbitOuter->SetRelativeLocation(OuterLocation);
    FutureWellOrbitOuter->SetRelativeScale3D(OuterScale);
    FutureWellOrbitOuter->SetRelativeRotation(OuterRotation);
    FutureWellOrbitInner->SetRelativeLocation(InnerLocation);
    FutureWellOrbitInner->SetRelativeScale3D(InnerScale);
    FutureWellOrbitInner->SetRelativeRotation(InnerRotation);
    FutureWellCore->SetRelativeLocation(CoreLocation);
    FutureWellCoreBaseScale = CoreScale;
    FutureWellCore->SetRelativeScale3D(CoreScale);
    FutureWellGroundGlyphA->SetRelativeLocation(GlyphLocation);
    FutureWellGroundGlyphA->SetRelativeScale3D(GlyphAScale);
    FutureWellGroundGlyphA->SetRelativeRotation(GlyphARotation);
    FutureWellGroundGlyphB->SetRelativeLocation(GlyphLocation);
    FutureWellGroundGlyphB->SetRelativeScale3D(GlyphBScale);
    FutureWellGroundGlyphB->SetRelativeRotation(GlyphBRotation);
    FutureWellVisualTimeSeconds = 0.0f;

    FutureWellOrbitOuter->SetVisibility(true, true);
    FutureWellOrbitInner->SetVisibility(true, true);
    FutureWellCore->SetVisibility(true, true);
    FutureWellGroundGlyphA->SetVisibility(bShowGlyphA, true);
    FutureWellGroundGlyphB->SetVisibility(bShowGlyphB, true);

    EnsureFutureWellMaterialSet(
        FutureWellOrbitOuter,
        FutureWellOrbitOuterMaterials);
    EnsureFutureWellMaterialSet(
        FutureWellOrbitInner,
        FutureWellOrbitInnerMaterials);
    EnsureFutureWellMaterialSet(FutureWellCore, FutureWellCoreMaterials);
    EnsureFutureWellMaterialSet(
        FutureWellGroundGlyphA,
        FutureWellGroundGlyphAMaterials);
    EnsureFutureWellMaterialSet(
        FutureWellGroundGlyphB,
        FutureWellGroundGlyphBMaterials);

    const FLinearColor StoneColor(0.030f, 0.034f, 0.042f);
    const FLinearColor GlassColor(0.006f, 0.008f, 0.014f);
    auto ApplyPalette = [&StoneColor,
                         &GlassColor,
                         &SecondaryColor,
                         &GlowColor](
                            TArray<TObjectPtr<UMaterialInstanceDynamic>>& Materials,
                            float InSecondaryEmissive,
                            float InGlowEmissive)
    {
        if (Materials.Num() < 4)
        {
            return;
        }
        const FLinearColor Colors[] = {
            StoneColor,
            GlassColor,
            SecondaryColor,
            GlowColor};
        const float Metallic[] = {0.12f, 0.56f, 0.32f, 0.18f};
        const float Roughness[] = {0.70f, 0.16f, 0.34f, 0.14f};
        const float Emissive[] = {
            0.0f,
            0.0f,
            InSecondaryEmissive,
            InGlowEmissive};
        for (int32 MaterialIndex = 0; MaterialIndex < 4; ++MaterialIndex)
        {
            UMaterialInstanceDynamic* Material = Materials[MaterialIndex];
            Material->SetVectorParameterValue(
                EntityColorParameterName,
                Colors[MaterialIndex]);
            Material->SetScalarParameterValue(
                MetallicParameterName,
                Metallic[MaterialIndex]);
            Material->SetScalarParameterValue(
                RoughnessParameterName,
                Roughness[MaterialIndex]);
            Material->SetScalarParameterValue(
                EmissiveStrengthParameterName,
                Emissive[MaterialIndex]);
        }
    };

    ApplyPalette(BodyMaterials, SecondaryEmissive * 0.08f, GlowEmissive * 0.72f);
    ApplyPalette(
        FutureWellOrbitOuterMaterials,
        SecondaryEmissive * 0.30f,
        GlowEmissive);
    ApplyPalette(
        FutureWellOrbitInnerMaterials,
        SecondaryEmissive * 0.38f,
        GlowEmissive * 1.08f);
    ApplyPalette(
        FutureWellCoreMaterials,
        SecondaryEmissive * 0.85f,
        GlowEmissive * 1.28f);
    ApplyPalette(
        FutureWellGroundGlyphAMaterials,
        SecondaryEmissive,
        GlowEmissive * 0.90f);
    ApplyPalette(
        FutureWellGroundGlyphBMaterials,
        SecondaryEmissive,
        GlowEmissive * 0.90f);
    BaseBodyColor = StoneColor;
    SetBodyColor(BaseBodyColor);
}

bool AEchoesEntityView::IsDeploymentCoverVisible() const
{
    return DeploymentCover != nullptr && DeploymentCover->IsVisible();
}

bool AEchoesEntityView::IsSilhouetteAccentVisible() const
{
    return SilhouetteAccent != nullptr && SilhouetteAccent->IsVisible();
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

bool AEchoesEntityView::IsChoirIdentityStateVisible() const
{
    return ChoirIdentityField != nullptr && ChoirIdentityField->IsVisible();
}

bool AEchoesEntityView::IsAegisPowerFieldVisible() const
{
    return AegisPowerField != nullptr && AegisPowerField->IsVisible();
}

bool AEchoesEntityView::IsFutureWellPresentationVisible() const
{
    return bUsingAuthoredFutureWellMesh &&
           FutureWellOrbitOuter != nullptr &&
           FutureWellOrbitOuter->IsVisible() &&
           FutureWellOrbitInner != nullptr &&
           FutureWellOrbitInner->IsVisible() &&
           FutureWellCore != nullptr &&
           FutureWellCore->IsVisible();
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
    SelectionVFXTimeSeconds = 0.0f;
    SelectionRing->SetRelativeRotation(FRotator::ZeroRotator);
    SelectionRing->SetRelativeScale3D(SelectionVFXBaseScale);
    const UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get();
    bSelectionReducedMotionApplied =
        bSelected && Settings != nullptr && Settings->IsReducedMotionEnabled();
    bSelectionReducedFlashingApplied =
        bSelected && Settings != nullptr && Settings->IsReducedFlashingEnabled();
    SelectionVFXEmissiveStrength = bSelectionReducedFlashingApplied ? 1.25f : 1.9f;
    if (RingMaterial != nullptr)
    {
        RingMaterial->SetScalarParameterValue(
            EmissiveStrengthParameterName,
            SelectionVFXEmissiveStrength);
    }
    SelectionRing->SetVisibility(bSelected, true);
    BodyMesh->SetRenderCustomDepth(bSelected);
    BodyMesh->SetCustomDepthStencilValue(bSelected ? 1 : 0);
    SilhouetteAccent->SetRenderCustomDepth(bSelected);
    SilhouetteAccent->SetCustomDepthStencilValue(bSelected ? 1 : 0);
    UpdateHealthBar();
}

bool AEchoesEntityView::IsSelectionVFXVisible() const
{
    return SelectionRing != nullptr && SelectionRing->IsVisible();
}

bool AEchoesEntityView::HasSelectionVFXCollisionDisabled() const
{
    return SelectionRing != nullptr &&
           SelectionRing->GetCollisionEnabled() == ECollisionEnabled::NoCollision &&
           !SelectionRing->GetGenerateOverlapEvents();
}

bool AEchoesEntityView::HasSelectionVFXNavigationDisabled() const
{
    return SelectionRing != nullptr &&
           !SelectionRing->CanEverAffectNavigation();
}

float AEchoesEntityView::GetSelectionVFXYaw() const
{
    return SelectionRing != nullptr
               ? SelectionRing->GetRelativeRotation().Yaw
               : 0.0f;
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
    if (EntityFaction == echoes::sim::Faction::HollowChoir)
    {
        switch (EntityType)
        {
            case echoes::sim::EntityType::Worker:
                return TEXT("Threadkeeper");
            case echoes::sim::EntityType::Soldier:
                return TEXT("Intervalist");
            case echoes::sim::EntityType::HeavyUnit:
                return TEXT("Lacuna Warden");
            case echoes::sim::EntityType::ScoutUnit:
                return TEXT("Afterimage");
            case echoes::sim::EntityType::CommandCore:
                return TEXT("Concordance");
            case echoes::sim::EntityType::Dropoff:
                return TEXT("Interval Loom");
            case echoes::sim::EntityType::Barracks:
                return TEXT("Chorus Loom");
            case echoes::sim::EntityType::UtilityStructure:
                return TEXT("Phase Anchor");
            case echoes::sim::EntityType::ResourceNode:
            case echoes::sim::EntityType::FutureWell:
                break;
        }
    }
    switch (EntityType)
    {
        case echoes::sim::EntityType::Worker:
            return EntityFaction == echoes::sim::Faction::MeridianCompact
                       ? TEXT("Surveyor")
                       : TEXT("Tender");
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
