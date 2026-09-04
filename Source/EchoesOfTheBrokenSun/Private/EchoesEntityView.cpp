#include "EchoesEntityView.h"

#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EchoesCollisionChannels.h"
#include "EchoesOfTheBrokenSun.h"
#include "EchoesGameUserSettings.h"
#include "EchoesInterfaceAudioSubsystem.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
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
const FName BaseColorMapParameterName(TEXT("BaseColorMap"));
const FName MREMapParameterName(TEXT("MREMap"));
const FName NormalMapParameterName(TEXT("NormalMap"));
const FName EmissiveTintParameterName(TEXT("EmissiveTint"));
const FName MaskedEmissiveStrengthParameterName(TEXT("MaskedEmissiveStrength"));
const FName ViewShiftParameterName(TEXT("ViewShift"));

/**
 * Registered A3 surface-texture families (surface-textures-v8). Each family
 * is a modulation set around the ceramic albedo the slot-colour tables were
 * tuned against, so the existing per-slot tints keep their meaning; the
 * family carries grain, seams, strata, lattice, or facets plus an emissive
 * mask for its own glow. Presentation only: nothing here reaches the
 * simulation, fog authority, saves, replays, or checksums.
 */
enum class ESurfaceTextureFamily : uint8
{
    CeramicCivic,
    CompactMetal,
    KharuunMineral,
    ChoirCoherent,
    MatterCrystal,
    VitrifiedGlass,
};

[[nodiscard]] const TCHAR* SurfaceTextureFamilyName(
    ESurfaceTextureFamily Family)
{
    switch (Family)
    {
        case ESurfaceTextureFamily::CompactMetal:
            return TEXT("CompactMetal");
        case ESurfaceTextureFamily::KharuunMineral:
            return TEXT("KharuunMineral");
        case ESurfaceTextureFamily::ChoirCoherent:
            return TEXT("ChoirCoherent");
        case ESurfaceTextureFamily::MatterCrystal:
            return TEXT("MatterCrystal");
        case ESurfaceTextureFamily::VitrifiedGlass:
            return TEXT("VitrifiedGlass");
        case ESurfaceTextureFamily::CeramicCivic:
        default:
            return TEXT("CeramicCivic");
    }
}

/**
 * Binds one registered family's three maps to a body MID. A map that is not
 * present in the content leaves the master's ceramic default in place and is
 * reported once, so a partial regeneration degrades to the accepted v6 look
 * rather than to a missing-texture checkerboard.
 */
void ApplySurfaceTextureFamily(
    UMaterialInstanceDynamic* Material,
    ESurfaceTextureFamily Family)
{
    if (Material == nullptr)
    {
        return;
    }
    static TSet<FString> ReportedMissing;
    const TCHAR* FamilyName = SurfaceTextureFamilyName(Family);
    const TPair<FName, const TCHAR*> Maps[] = {
        {BaseColorMapParameterName, TEXT("BaseColor")},
        {MREMapParameterName, TEXT("MRE")},
        {NormalMapParameterName, TEXT("Normal")},
    };
    for (const TPair<FName, const TCHAR*>& Map : Maps)
    {
        const FString Path = FString::Printf(
            TEXT("/Game/Art/Generated/Textures/T_Echoes%s_%s.T_Echoes%s_%s"),
            FamilyName,
            Map.Value,
            FamilyName,
            Map.Value);
        UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *Path);
        if (Texture == nullptr)
        {
            if (!ReportedMissing.Contains(Path))
            {
                ReportedMissing.Add(Path);
                UE_LOG(
                    LogEchoes,
                    Warning,
                    TEXT("[ECHOES_SURFACE_FAMILY_MISSING] family=%s map=%s path=%s fallback=ceramic"),
                    FamilyName,
                    Map.Value,
                    *Path);
            }
            continue;
        }
        Material->SetTextureParameterValue(Map.Key, Texture);
    }
}
constexpr float DamagePulseDurationSeconds = 0.18f;
// SM_VFX_SelectionHalo carries its acquisition brackets out to 68 cm from its
// own origin, so a relative scale of one draws a 68 cm halo radius.
constexpr float SelectionHaloExtentCentimetres = 68.0f;
// /Engine/BasicShapes/Cylinder is a 100 cm cube-bounded primitive, so at a
// relative scale of one it draws a 50 cm radius. Every ability disc below is
// scaled through this, never through a hand-picked number.
constexpr float UnitCylinderRadiusCentimetres = 50.0f;
// The hit tick sits between the health bar and the owner marker, inside the
// pick volume's headroom, so it never pokes out of the entity's click target.
constexpr float DamageAcknowledgeMarkerHeightCentimetres = 13.0f;
constexpr uint8 FutureWellProtocolAccentNone = 255;
// A floor under the pick footprint so the smallest entity still answers a
// click, and headroom above the drawn geometry so the health bar and owner
// marker sit inside the volume rather than poking out of the top of it.
constexpr float MinimumEntityPickHaloScale = 0.85f;
constexpr float EntityPickHeadroomCentimetres = 26.0f;

// An ability disc is an area-of-effect claim. Its drawn radius is derived from
// the authoritative rule radius so the circle a player reads is the circle the
// simulation actually applies - a decorative approximation here would be a
// false affordance (VAL-003). Reading the rule is one-way: nothing here writes
// back into the simulation (SIM-002).
[[nodiscard]] float AbilityDiscRadiusCentimetres(std::int32_t RadiusRaw)
{
    return static_cast<float>(RadiusRaw) /
           static_cast<float>(echoes::sim::kFixedScale) *
           UEchoesSimulationSubsystem::TileWorldSize;
}

[[nodiscard]] FVector AbilityDiscScale(float RadiusCentimetres, float Thickness)
{
    const float PlanarScale =
        RadiusCentimetres / UnitCylinderRadiusCentimetres;
    return FVector(PlanarScale, PlanarScale, Thickness);
}

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

    BodyPivot = CreateDefaultSubobject<USceneComponent>(TEXT("BodyPivot"));
    BodyPivot->SetupAttachment(SceneRoot);
    BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
    BodyMesh->SetupAttachment(BodyPivot);
    BodyMesh->SetCollisionObjectType(ECC_WorldDynamic);
    BodyMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    BodyMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    // The body stands on the ground, so it answers both questions: it is the
    // ground trace's entity hit that selection reads, and it is part of the
    // entity pick region.
    BodyMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    BodyMesh->SetCollisionResponseToChannel(ECC_EchoesEntityPick, ECR_Block);
    BodyMesh->SetGenerateOverlapEvents(false);
    BodyMesh->SetCastShadow(true);

    // Never rendered, never in the ground trace, never in the simulation: a
    // footprint-sized query volume that exists only so ECC_EchoesEntityPick
    // finds this entity anywhere inside the footprint the selection halo
    // draws. A shape component is used rather than a mesh because a shape's
    // body setup is CTF_UseSimpleAsComplex, so the complex screen trace the
    // controller runs always resolves against it.
    EntityPickProxy = CreateDefaultSubobject<UCapsuleComponent>(
        TEXT("EntityPickProxy"));
    EntityPickProxy->SetupAttachment(SceneRoot);
    EntityPickProxy->SetCollisionObjectType(ECC_WorldDynamic);
    EntityPickProxy->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    EntityPickProxy->SetCollisionResponseToAllChannels(ECR_Ignore);
    EntityPickProxy->SetCollisionResponseToChannel(
        ECC_EchoesEntityPick, ECR_Block);
    EntityPickProxy->SetGenerateOverlapEvents(false);
    EntityPickProxy->SetCanEverAffectNavigation(false);
    EntityPickProxy->SetCastShadow(false);
    EntityPickProxy->SetReceivesDecals(false);
    EntityPickProxy->bDrawOnlyIfSelected = true;
    EntityPickProxy->SetHiddenInGame(true);
    EntityPickProxy->SetVisibility(false);

    SilhouetteAccent = CreateDefaultSubobject<UStaticMeshComponent>(
        TEXT("SilhouetteAccent"));
    SilhouetteAccent->SetupAttachment(SceneRoot);
    SilhouetteAccent->SetCollisionObjectType(ECC_WorldDynamic);
    SilhouetteAccent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SilhouetteAccent->SetCollisionResponseToAllChannels(ECR_Ignore);
    SilhouetteAccent->SetCollisionResponseToChannel(
        ECC_EchoesEntityPick, ECR_Block);
    SilhouetteAccent->SetGenerateOverlapEvents(false);
    SilhouetteAccent->SetCanEverAffectNavigation(false);
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
        Bar->SetCollisionObjectType(ECC_WorldDynamic);
        Bar->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Bar->SetCollisionResponseToAllChannels(ECR_Ignore);
        Bar->SetCollisionResponseToChannel(ECC_EchoesEntityPick, ECR_Block);
        Bar->SetGenerateOverlapEvents(false);
        Bar->SetCanEverAffectNavigation(false);
        Bar->SetCastShadow(false);
        Bar->SetReceivesDecals(false);
        Bar->SetVisibility(false);
    }

    OwnerMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OwnerMarker"));
    OwnerMarker->SetupAttachment(SceneRoot);
    OwnerMarker->SetCollisionObjectType(ECC_WorldDynamic);
    OwnerMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    OwnerMarker->SetCollisionResponseToAllChannels(ECR_Ignore);
    OwnerMarker->SetCollisionResponseToChannel(
        ECC_EchoesEntityPick, ECR_Block);
    OwnerMarker->SetGenerateOverlapEvents(false);
    OwnerMarker->SetCanEverAffectNavigation(false);
    OwnerMarker->SetCastShadow(false);
    OwnerMarker->SetReceivesDecals(false);
    OwnerMarker->SetVisibility(false);

    // The non-colour half of the "this entity was just hit" event. Reduced
    // flashing removes the body's luminance ramp; it must not remove the
    // event, so the hit is also carried by a shape that appears for the same
    // window in every accessibility mode (ACC-001, and the reduced-variant
    // rule that a reduced effect still identifies start, active state and
    // expiry). Presentation only: it reads damage, it never causes it.
    DamageAcknowledgeMarker = CreateDefaultSubobject<UStaticMeshComponent>(
        TEXT("DamageAcknowledgeMarker"));
    DamageAcknowledgeMarker->SetupAttachment(SceneRoot);
    DamageAcknowledgeMarker->SetCollisionObjectType(ECC_WorldDynamic);
    DamageAcknowledgeMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    DamageAcknowledgeMarker->SetCollisionResponseToAllChannels(ECR_Ignore);
    DamageAcknowledgeMarker->SetCollisionResponseToChannel(
        ECC_EchoesEntityPick, ECR_Block);
    DamageAcknowledgeMarker->SetGenerateOverlapEvents(false);
    DamageAcknowledgeMarker->SetCanEverAffectNavigation(false);
    DamageAcknowledgeMarker->SetCastShadow(false);
    DamageAcknowledgeMarker->SetReceivesDecals(false);
    DamageAcknowledgeMarker->SetVisibility(false);

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
    RelaySupplyField->SetCanEverAffectNavigation(false);
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
    AegisPowerField->SetCanEverAffectNavigation(false);
    AegisPowerField->SetCastShadow(false);
    AegisPowerField->SetReceivesDecals(false);
    AegisPowerField->SetVisibility(false);

    GatherBeam = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GatherBeam"));
    ConstructionField = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ConstructionField"));
    ReshapeTelegraph = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ReshapeTelegraph"));

    for (UStaticMeshComponent* VFXComp : {GatherBeam, ConstructionField, ReshapeTelegraph})
    {
        VFXComp->SetupAttachment(SceneRoot);
        VFXComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        VFXComp->SetGenerateOverlapEvents(false);
        VFXComp->SetCanEverAffectNavigation(false);
        VFXComp->SetCastShadow(false);
        VFXComp->SetReceivesDecals(false);
        VFXComp->SetVisibility(false);
    }

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
        FutureWellComponent->SetCollisionObjectType(ECC_WorldDynamic);
        FutureWellComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        FutureWellComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
        FutureWellComponent->SetCollisionResponseToChannel(
            ECC_EchoesEntityPick, ECR_Block);
        FutureWellComponent->SetGenerateOverlapEvents(false);
        FutureWellComponent->SetCanEverAffectNavigation(false);
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
    DamageAcknowledgeMarker->SetStaticMesh(CubeMesh);
    RelaySupplyField->SetStaticMesh(CylinderMesh);
    RelaySupplyField->SetRelativeLocation(FVector(0.0f, 0.0f, 5.0f));
    // Sized from the authored default rule, not from a literal. The live rule
    // set re-derives it in ConfigureAppearance once a simulation exists; this
    // only stops a never-configured actor from drawing an arbitrary circle.
    RelaySupplyField->SetRelativeScale3D(AbilityDiscScale(
        AbilityDiscRadiusCentimetres(
            echoes::sim::RelaySupplyRules{}.connectionRadiusRaw),
        0.025f));
    WaystoneStateField->SetStaticMesh(CylinderMesh);
    WaystoneStateField->SetRelativeLocation(FVector(0.0f, 0.0f, 4.0f));
    WarformStateField->SetStaticMesh(CylinderMesh);
    WarformStateField->SetRelativeLocation(FVector(0.0f, 0.0f, 5.0f));
    ChoirIdentityField->SetStaticMesh(CylinderMesh);
    ChoirIdentityField->SetRelativeLocation(FVector(0.0f, 0.0f, 6.0f));
    AegisPowerField->SetStaticMesh(CylinderMesh);
    AegisPowerField->SetRelativeLocation(FVector(0.0f, 0.0f, 6.0f));
    AegisPowerField->SetRelativeScale3D(AbilityDiscScale(
        AbilityDiscRadiusCentimetres(
            echoes::sim::PoweredAegisRules{}.connectionRadiusRaw),
        0.045f));
    FutureWellOrbitOuter->SetStaticMesh(FutureWellOrbitMesh);
    FutureWellOrbitInner->SetStaticMesh(FutureWellOrbitMesh);
    FutureWellCore->SetStaticMesh(FutureWellCoreMesh);
    FutureWellGroundGlyphA->SetStaticMesh(FutureWellGlyphMesh);
    FutureWellGroundGlyphB->SetStaticMesh(FutureWellGlyphMesh);
    GatherBeam->SetStaticMesh(CylinderMesh);
    ConstructionField->SetStaticMesh(CylinderMesh);
    ReshapeTelegraph->SetStaticMesh(CylinderMesh);
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
    BodyMesh->SetCollisionResponseToChannel(ECC_EchoesEntityPick, ECR_Block);
    BodyMesh->SetGenerateOverlapEvents(false);
    BodyMesh->SetVisibility(true, true);
    // The footprint volume answers entity picking only. It is restored here and
    // sized by ConfigureAppearance below, once the footprint is known.
    EntityPickProxy->SetCollisionObjectType(ECC_WorldDynamic);
    EntityPickProxy->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    EntityPickProxy->SetCollisionResponseToAllChannels(ECR_Ignore);
    EntityPickProxy->SetCollisionResponseToChannel(
        ECC_EchoesEntityPick, ECR_Block);
    EntityPickProxy->SetGenerateOverlapEvents(false);
    EntityPickProxy->SetHiddenInGame(true);
    EntityPickProxy->SetVisibility(false, true);
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
    EntityPickProxy->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    EntityPickProxy->SetCollisionResponseToAllChannels(ECR_Ignore);
    EntityPickProxy->SetGenerateOverlapEvents(false);
    EntityPickProxyRadius = 0.0f;
    EntityPickProxyTopHeight = 0.0f;

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
    DamageAcknowledgeRemainingSeconds = 0.0f;
    RelaySupplyFieldRadiusCentimetres = 0.0f;
    AegisPowerFieldRadiusCentimetres = 0.0f;
    FutureWellProtocolAccentVariant = FutureWellProtocolAccentNone;
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
    PreviousAuthoritativeLocation = FVector::ZeroVector;
    AuthoritativeVelocity = FVector::ZeroVector;
    AuthoritativeSpeed = 0.0f;
    CurrentHeadingYaw = 0.0f;
    TargetHeadingYaw = 0.0f;
    WalkCyclePhase = 0.0f;
    HoverPhaseTime = 0.0f;
    IdlePhaseTime = 0.0f;
    WorkerHarvestPhaseTime = 0.0f;
    HoverBobOffsetCentimetres = 0.0f;
    CarriedCargoAmount = 0;
    bLocomotionActive = false;
    bIsHoverUnit = false;
    bIsWalkerUnit = false;
    bWorkerHarvestingActive = false;
    bMotionReducedMotionApplied = false;
    BaseSilhouetteAccentRotation = FRotator::ZeroRotator;
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

void AEchoesEntityView::SetOverlayVisibleAndPickable(
    UStaticMeshComponent* Component,
    bool bVisible)
{
    if (Component == nullptr)
    {
        return;
    }
    // These overlays are drawn above the body and are read by the player as
    // part of the entity, so entity resolution must reach them - but only
    // while they are actually on screen. Setting both here, and only here, is
    // what keeps a hidden overlay from becoming an invisible pick plate.
    //
    // Only ECC_EchoesEntityPick is involved. The constructor never gives these
    // components an ECC_Visibility response, so no overlay can appear in the
    // ground trace and hand a command site a point in the air.
    Component->SetVisibility(bVisible, true);
    Component->SetCollisionEnabled(
        bVisible ? ECollisionEnabled::QueryOnly
                 : ECollisionEnabled::NoCollision);
}

void AEchoesEntityView::SetOverlayUnpickableForPool(
    UStaticMeshComponent* Component)
{
    SetOverlayVisibleAndPickable(Component, false);
}

bool AEchoesEntityView::IsOverlayEntityPickable(
    const UStaticMeshComponent* Component)
{
    return Component != nullptr &&
           Component->GetCollisionEnabled() == ECollisionEnabled::QueryOnly &&
           Component->GetCollisionResponseToChannel(ECC_EchoesEntityPick) ==
               ECR_Block &&
           Component->GetCollisionResponseToChannel(ECC_Visibility) ==
               ECR_Ignore;
}

bool AEchoesEntityView::IsHealthBarEntityPickable() const
{
    return GetActorEnableCollision() &&
           IsOverlayEntityPickable(HealthBarBackground);
}

bool AEchoesEntityView::IsOwnerMarkerEntityPickable() const
{
    return GetActorEnableCollision() && IsOverlayEntityPickable(OwnerMarker);
}

bool AEchoesEntityView::IsSilhouetteAccentEntityPickable() const
{
    return GetActorEnableCollision() &&
           IsOverlayEntityPickable(SilhouetteAccent);
}

bool AEchoesEntityView::IsFutureWellPresentationEntityPickable() const
{
    return GetActorEnableCollision() &&
           IsOverlayEntityPickable(FutureWellOrbitOuter) &&
           IsOverlayEntityPickable(FutureWellOrbitInner) &&
           IsOverlayEntityPickable(FutureWellCore);
}

bool AEchoesEntityView::IsEntityPickProxyEnabled() const
{
    return GetActorEnableCollision() && EntityPickProxy != nullptr &&
           EntityPickProxy->GetCollisionEnabled() ==
               ECollisionEnabled::QueryOnly &&
           EntityPickProxy->GetCollisionResponseToChannel(
               ECC_EchoesEntityPick) == ECR_Block;
}

bool AEchoesEntityView::IsEntityPickProxyHidden() const
{
    return EntityPickProxy != nullptr && !EntityPickProxy->IsVisible();
}

bool AEchoesEntityView::DoesEntityPickProxyBlockGroundTrace() const
{
    return EntityPickProxy != nullptr &&
           EntityPickProxy->GetCollisionResponseToChannel(ECC_Visibility) ==
               ECR_Block;
}

float AEchoesEntityView::GetEntityPickProxyRadius() const
{
    return EntityPickProxyRadius;
}

float AEchoesEntityView::GetEntityPickProxyTopHeight() const
{
    return EntityPickProxyTopHeight;
}

bool AEchoesEntityView::GetClickableBounds(
    FVector& OutOrigin,
    FVector& OutExtent) const
{
    // Prefer the pick proxy: it is the geometry a click is actually resolved
    // against, so it is the honest answer to "can the player reach this unit".
    const UPrimitiveComponent* Source = nullptr;
    if (EntityPickProxy != nullptr && EntityPickProxy->IsRegistered())
    {
        Source = EntityPickProxy;
    }
    else if (BodyMesh != nullptr && BodyMesh->IsRegistered())
    {
        Source = BodyMesh;
    }
    if (Source == nullptr)
    {
        return false;
    }
    const FBoxSphereBounds ClickableBounds = Source->Bounds;
    OutOrigin = ClickableBounds.Origin;
    OutExtent = ClickableBounds.BoxExtent;
    return !OutExtent.IsNearlyZero();
}

void AEchoesEntityView::ConfigureEntityPickProxy(float SelectionHaloScale)
{
    if (EntityPickProxy == nullptr)
    {
        return;
    }
    // The footprint a right-click resolves over is whichever is larger of the
    // two things the player can actually see: the body's own drawn extent and
    // the selection halo's radius. Taking the body's bounds rather than a
    // per-type constant is what makes this correct for an authored mesh whose
    // silhouette has nothing to do with the primitive it replaced - a Matter
    // deposit's spires and collar reach far past the small sphere the
    // fallback body would have drawn.
    float BodyHorizontalReach = 0.0f;
    float BodyTopReach = 0.0f;
    if (const UStaticMesh* CurrentBodyMesh = BodyMesh->GetStaticMesh())
    {
        const FBoxSphereBounds MeshBounds = CurrentBodyMesh->GetBounds();
        const FVector BodyScale =
            BodyMesh->GetRelativeScale3D() * BodyPivot->GetRelativeScale3D();
        BodyHorizontalReach = static_cast<float>(FMath::Max(
            (FMath::Abs(MeshBounds.Origin.X) + MeshBounds.BoxExtent.X) *
                FMath::Abs(BodyScale.X),
            (FMath::Abs(MeshBounds.Origin.Y) + MeshBounds.BoxExtent.Y) *
                FMath::Abs(BodyScale.Y)));
        BodyTopReach = static_cast<float>(
            BodyMesh->GetRelativeLocation().Z * BodyPivot->GetRelativeScale3D().Z +
            (MeshBounds.Origin.Z + MeshBounds.BoxExtent.Z) *
                FMath::Abs(BodyScale.Z));
    }
    EntityPickProxyRadius = FMath::Max(
        BodyHorizontalReach,
        FMath::Max(SelectionHaloScale, MinimumEntityPickHaloScale) *
            SelectionHaloExtentCentimetres);
    // The top clears the health bar so the bar and the owner marker sit inside
    // the volume instead of above it, and clears the body for a tall mesh.
    EntityPickProxyTopHeight = FMath::Max(
        HealthBarHeight + EntityPickHeadroomCentimetres,
        BodyTopReach + EntityPickHeadroomCentimetres);
    EntityPickProxy->SetCapsuleSize(
        EntityPickProxyRadius,
        FMath::Max(EntityPickProxyRadius, EntityPickProxyTopHeight * 0.5f),
        false);
    EntityPickProxy->SetRelativeLocation(
        FVector(0.0f, 0.0f, EntityPickProxyTopHeight * 0.5f));
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
        Material->SetVectorParameterValue(
            EmissiveTintParameterName,
            FLinearColor::White);
        Material->SetScalarParameterValue(
            MaskedEmissiveStrengthParameterName, 0.0f);
        Material->SetScalarParameterValue(ViewShiftParameterName, 0.0f);
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
             DamageAcknowledgeMarkerMaterial,
             DeploymentCoverMaterial,
             RelaySupplyFieldMaterial,
             WaystoneStateFieldMaterial,
             WarformStateFieldMaterial,
             ChoirIdentityFieldMaterial,
             AegisPowerFieldMaterial,
             GatherBeamMaterial,
             ConstructionFieldMaterial,
             ReshapeTelegraphMaterial})
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
    BodyPivot->SetRelativeScale3D(FVector::OneVector);
    PresentationScale = 1.0f;

    SetOverlayUnpickableForPool(SilhouetteAccent);
    SilhouetteAccent->SetRenderCustomDepth(false);
    SilhouetteAccent->SetCustomDepthStencilValue(0);
    SilhouetteAccent->SetRelativeLocation(FVector::ZeroVector);
    SilhouetteAccent->SetRelativeRotation(FRotator::ZeroRotator);
    SilhouetteAccent->SetRelativeScale3D(FVector::OneVector);

    EntityPickProxy->SetRelativeLocation(FVector::ZeroVector);
    EntityPickProxy->SetRelativeRotation(FRotator::ZeroRotator);
    EntityPickProxy->SetRelativeScale3D(FVector::OneVector);
    EntityPickProxy->SetCapsuleSize(1.0f, 1.0f, false);

    SelectionRing->SetVisibility(false, true);
    SelectionRing->SetRelativeLocation(FVector(0.0f, 0.0f, 3.0f));
    SelectionRing->SetRelativeRotation(FRotator::ZeroRotator);
    SelectionRing->SetRelativeScale3D(FVector::OneVector);

    for (UStaticMeshComponent* Component : {
             HealthBarBackground,
             HealthBarFill,
             OwnerMarker,
             DamageAcknowledgeMarker,
             DeploymentCover,
             RelaySupplyField,
             WaystoneStateField,
             WarformStateField,
             ChoirIdentityField,
             AegisPowerField,
             GatherBeam,
             ConstructionField,
             ReshapeTelegraph,
             FutureWellOrbitOuter,
             FutureWellOrbitInner,
             FutureWellCore,
             FutureWellGroundGlyphA,
             FutureWellGroundGlyphB})
    {
        // Same gate as every other hide: a pooled view must leave no
        // pickable overlay behind for the next entity to inherit.
        SetOverlayUnpickableForPool(Component);
        Component->SetRelativeRotation(FRotator::ZeroRotator);
        Component->SetRelativeScale3D(FVector::OneVector);
    }
    HealthBarBackground->SetRelativeLocation(FVector::ZeroVector);
    HealthBarFill->SetRelativeLocation(FVector::ZeroVector);
    OwnerMarker->SetRelativeLocation(FVector::ZeroVector);
    OwnerMarker->SetStaticMesh(nullptr);
    DamageAcknowledgeMarker->SetRelativeLocation(FVector::ZeroVector);
    DeploymentCover->SetRelativeLocation(FVector::ZeroVector);
    RelaySupplyField->SetRelativeLocation(FVector(0.0f, 0.0f, 5.0f));
    WaystoneStateField->SetRelativeLocation(FVector(0.0f, 0.0f, 4.0f));
    WarformStateField->SetRelativeLocation(FVector(0.0f, 0.0f, 5.0f));
    ChoirIdentityField->SetRelativeLocation(FVector(0.0f, 0.0f, 6.0f));
    AegisPowerField->SetRelativeLocation(FVector(0.0f, 0.0f, 6.0f));
    GatherBeam->SetRelativeLocation(FVector::ZeroVector);
    ConstructionField->SetRelativeLocation(FVector::ZeroVector);
    ReshapeTelegraph->SetRelativeLocation(FVector::ZeroVector);
    FutureWellOrbitOuter->SetRelativeLocation(FVector::ZeroVector);
    FutureWellOrbitInner->SetRelativeLocation(FVector::ZeroVector);
    FutureWellCore->SetRelativeLocation(FVector::ZeroVector);
    FutureWellGroundGlyphA->SetRelativeLocation(FVector::ZeroVector);
    FutureWellGroundGlyphB->SetRelativeLocation(FVector::ZeroVector);

    bGatherBeamActive = false;
    bConstructionFieldActive = false;
    bReshapeTelegraphActive = false;
    ConstructionFraction = 0.0f;
    GatherBeamPulsePhase = 0.0f;
    ReshapeTelegraphPulsePhase = 0.0f;
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
    bMotionReducedMotionApplied =
        Settings != nullptr && Settings->IsReducedMotionEnabled();

    if (bIsHoverUnit || bIsWalkerUnit)
    {
        if (!bMotionReducedMotionApplied)
        {
            CurrentHeadingYaw = FMath::FInterpTo(
                CurrentHeadingYaw,
                TargetHeadingYaw,
                DeltaSeconds,
                10.0f);
        }
        else
        {
            CurrentHeadingYaw = TargetHeadingYaw;
        }
    }
    UpdateComponentMotion(DeltaSeconds, bMotionReducedMotionApplied);
    const bool bReducedFlashing =
        Settings != nullptr && Settings->IsReducedFlashingEnabled();
    UpdateCombatVFX(DeltaSeconds, bMotionReducedMotionApplied, bReducedFlashing);

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
    // Reduced flashing removes the LUMINANCE RAMP, never the event. The
    // acknowledgement window below runs at the same length in every mode, so
    // the hit still has a start, an active state and an expiry - it is just
    // carried by the marker's shape instead of by the body's brightness.
    if (Settings != nullptr && Settings->IsReducedFlashingEnabled())
    {
        DamagePulseRemainingSeconds = 0.0f;
    }
    if (DamageAcknowledgeRemainingSeconds > 0.0f)
    {
        DamageAcknowledgeRemainingSeconds = FMath::Max(
            0.0f,
            DamageAcknowledgeRemainingSeconds - DeltaSeconds);
    }
    UpdateDamageAcknowledgeMarker();
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

    bIsHoverUnit = false;
    bIsWalkerUnit = false;

    if (EntityType == echoes::sim::EntityType::ScoutUnit)
    {
        if (EntityFaction == echoes::sim::Faction::MeridianCompact ||
            EntityFaction == echoes::sim::Faction::HollowChoir)
        {
            bIsHoverUnit = true;
        }
        else
        {
            bIsWalkerUnit = true;
        }
    }
    else if (EntityFaction == echoes::sim::Faction::HollowChoir)
    {
        if (EntityType == echoes::sim::EntityType::Worker ||
            EntityType == echoes::sim::EntityType::Soldier ||
            EntityType == echoes::sim::EntityType::HeavyUnit)
        {
            bIsHoverUnit = true;
        }
    }
    else if (EntityType == echoes::sim::EntityType::Worker ||
             EntityType == echoes::sim::EntityType::Soldier ||
             EntityType == echoes::sim::EntityType::HeavyUnit)
    {
        bIsWalkerUnit = true;
    }

    bWorkerHarvestingActive =
        (EntityType == echoes::sim::EntityType::Worker) && (State.harvestTicks > 0);
    CarriedCargoAmount = State.cargo;
    bGatherBeamActive = bWorkerHarvestingActive;
    bConstructionFieldActive = (!State.completed && State.constructionProgress > 0);
    ConstructionFraction = (State.constructionRequired > 0)
        ? FMath::Clamp(static_cast<float>(State.constructionProgress) / static_cast<float>(State.constructionRequired), 0.0f, 1.0f)
        : 0.0f;
    bReshapeTelegraphActive = (State.reshapeUntilTick > 0);

    const FVector NewWorldLocation = Bridge->SimToWorld(State.position);
    if (!bTeleport && bHasAuthoritativeLocation)
    {
        const FVector Displacement = NewWorldLocation - PreviousAuthoritativeLocation;
        AuthoritativeVelocity = Displacement * 20.0f;
        AuthoritativeSpeed = AuthoritativeVelocity.Size2D();
        if (AuthoritativeSpeed > 8.0f)
        {
            TargetHeadingYaw = FMath::RadiansToDegrees(
                FMath::Atan2(Displacement.Y, Displacement.X));
            bLocomotionActive = true;
        }
        else
        {
            bLocomotionActive = false;
            AuthoritativeVelocity = FVector::ZeroVector;
            AuthoritativeSpeed = 0.0f;
        }
    }
    else
    {
        AuthoritativeVelocity = FVector::ZeroVector;
        AuthoritativeSpeed = 0.0f;
        bLocomotionActive = false;
        if (bDeployed)
        {
            TargetHeadingYaw = FMath::RadiansToDegrees(
                FMath::Atan2(
                    static_cast<float>(DeploymentFacing.y.Raw()),
                    static_cast<float>(DeploymentFacing.x.Raw())));
            CurrentHeadingYaw = TargetHeadingYaw;
        }
    }
    if (bDeployed)
    {
        TargetHeadingYaw = FMath::RadiansToDegrees(
            FMath::Atan2(
                static_cast<float>(DeploymentFacing.y.Raw()),
                static_cast<float>(DeploymentFacing.x.Raw())));
    }
    PreviousAuthoritativeLocation = NewWorldLocation;
    AuthoritativeWorldLocation = NewWorldLocation;

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
        // An owned entity just took authoritative damage: raise the
        // rate-limited under-attack alert. Presentation only; the alert
        // subsystem's per-class window absorbs sustained fire.
        if (State.owner == 0 /* local player */ && GetWorld() != nullptr)
        {
            if (UEchoesInterfaceAudioSubsystem* InterfaceAudio =
                    GetWorld()
                        ->GetSubsystem<UEchoesInterfaceAudioSubsystem>())
            {
                InterfaceAudio->PlayAlert(EEchoesAlertCue::UnderAttack);
            }
        }
        const UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get();
        const bool bReducedFlashing =
            Settings != nullptr && Settings->IsReducedFlashingEnabled();
        // The colour ramp is the flashing part and is the only part reduced
        // flashing suppresses. The acknowledgement window is armed either way
        // and re-arms on every further hit, so sustained fire holds one steady
        // marker instead of producing a train of luminance transitions.
        DamageAcknowledgeRemainingSeconds = DamagePulseDurationSeconds;
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
    UpdateDamageAcknowledgeMarker();
}

void AEchoesEntityView::SetAuthoritativeWorldLocation(const FVector& InLocation)
{
    AuthoritativeWorldLocation = InLocation;
    PreviousAuthoritativeLocation = InLocation;
    SetActorLocation(InLocation, false, nullptr, ETeleportType::TeleportPhysics);
}

void AEchoesEntityView::SetAuthoritativeHeadingYaw(float InYaw)
{
    TargetHeadingYaw = InYaw;
    CurrentHeadingYaw = InYaw;
    if (BodyMesh != nullptr)
    {
        BodyMesh->SetRelativeRotation(FRotator(0.0f, InYaw, 0.0f));
    }
}

void AEchoesEntityView::UpdateDamageAcknowledgeMarker()
{
    if (DamageAcknowledgeMarker == nullptr)
    {
        return;
    }
    const bool bActive = DamageAcknowledgeRemainingSeconds > 0.0f;
    if (bActive)
    {
        // A short, thick tick above the health bar: a different silhouette
        // from the bar's long thin fill, so the two are not read as one
        // widget. It stays inside the pick volume's headroom.
        DamageAcknowledgeMarker->SetRelativeLocation(FVector(
            0.0f,
            0.0f,
            HealthBarHeight + DamageAcknowledgeMarkerHeightCentimetres));
        DamageAcknowledgeMarker->SetRelativeScale3D(FVector(
            0.22f * HealthBarWidthScale,
            0.14f,
            0.14f));
    }
    SetOverlayVisibleAndPickable(DamageAcknowledgeMarker, bActive);
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
    PresentationScale = 1.0f;
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

                // Readability scale (gate 50): units are drawn larger than their
                // authoritative footprint, as StarCraft-class RTS presentation does,
                // so a heavy unit reads near one fifth of the Well dais at the
                // gameplay camera. Structures, landmarks, and deposits keep 1.0.
                // Collision, pathing, and tile occupancy are simulation-owned and
                // never see this value.
                switch (State.type)
                {
                    case echoes::sim::EntityType::Worker:
                        PresentationScale = 1.50f;
                        break;
                    case echoes::sim::EntityType::Soldier:
                        PresentationScale = 1.60f;
                        break;
                    case echoes::sim::EntityType::HeavyUnit:
                        PresentationScale = 1.75f;
                        break;
                    case echoes::sim::EntityType::ScoutUnit:
                        PresentationScale = 1.50f;
                        break;
                    case echoes::sim::EntityType::ResourceNode:
                        // Deposits go the other way: the authored cluster is
                        // taller than a heavy unit, so it is drawn smaller than
                        // its footprint to stay subordinate to the Well.
                        PresentationScale = 0.62f;
                        break;
                    default:
                        PresentationScale = 1.0f;
                        break;
                }
                HealthBarHeight *= PresentationScale;
                SelectionRadius *= FMath::Lerp(1.0f, PresentationScale, 0.35f);
            }
        }
    }

    BodyMesh->SetStaticMesh(DesiredMesh);
    BodyMesh->SetRelativeScale3D(BodyScale);
    BodyMesh->SetRelativeLocation(BodyOffset);
    BodyPivot->SetRelativeScale3D(FVector(PresentationScale));
    BodyMesh->SetVisibility(true, true);
    UStaticMesh* AccentMesh = CubeMesh;
    FVector AccentScale(0.18f, 0.54f, 0.12f);
    FVector AccentOffset(0.0f, 0.0f, HealthBarHeight - 28.0f);
    FRotator AccentRotation = FRotator::ZeroRotator;
    bool bShowSilhouetteAccent = !State.temporaryMineralCover;
    FutureWellProtocolAccentVariant = FutureWellProtocolAccentNone;
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
            bShowSilhouetteAccent = false;
            break;
        case echoes::sim::EntityType::FutureWell:
            // Only reached when the authored Well presentation is unavailable,
            // because the visibility gate below suppresses this accent once
            // that mesh set is in use. Without it the fallback Well is one
            // uniform disc for all four protocols and COLOUR is the only thing
            // separating Harvest from Preserve from Reshape from Dormant,
            // which ACC-001 forbids. Each protocol gets its own primitive and
            // orientation, so the protocol survives a colour-vision preset and
            // survives reduced motion, which cannot take a static shape away.
            switch (State.wellChoice)
            {
                case echoes::sim::FutureWellChoice::Harvest:
                    AccentMesh = ConeMesh;
                    AccentScale = FVector(0.40f, 0.40f, 0.44f);
                    AccentRotation = FRotator::ZeroRotator;
                    FutureWellProtocolAccentVariant = 1;
                    break;
                case echoes::sim::FutureWellChoice::Preserve:
                    AccentMesh = CylinderMesh;
                    AccentScale = FVector(0.70f, 0.70f, 0.10f);
                    AccentRotation = FRotator::ZeroRotator;
                    FutureWellProtocolAccentVariant = 2;
                    break;
                case echoes::sim::FutureWellChoice::Reshape:
                    AccentMesh = CubeMesh;
                    AccentScale = FVector(0.80f, 0.20f, 0.20f);
                    AccentRotation = FRotator(0.0f, 45.0f, 0.0f);
                    FutureWellProtocolAccentVariant = 3;
                    break;
                case echoes::sim::FutureWellChoice::Dormant:
                    AccentMesh = SphereMesh;
                    AccentScale = FVector(0.26f, 0.26f, 0.26f);
                    AccentRotation = FRotator::ZeroRotator;
                    FutureWellProtocolAccentVariant = 0;
                    break;
            }
            // Above the fallback Well's flat 18 cm disc but below its 42 cm
            // health bar and its owner marker, and well inside the pick
            // volume - the accent must add a shape channel, not a new thing
            // sticking out of the entity's click target.
            AccentOffset.Z = 24.0f;
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
    AccentOffset.Z *= PresentationScale;
    SilhouetteAccent->SetRelativeLocation(AccentOffset);
    SilhouetteAccent->SetRelativeRotation(AccentRotation);
    BaseSilhouetteAccentRotation = AccentRotation;
    const bool bSilhouetteAccentDrawn =
        bShowSilhouetteAccent && !bUsingAuthoredRosterMesh &&
        !bUsingAuthoredFutureWellMesh;
    SetOverlayVisibleAndPickable(SilhouetteAccent, bSilhouetteAccentDrawn);
    if (!bSilhouetteAccentDrawn)
    {
        // The authored Well presentation carries the protocol in its own
        // glyph count, orbit tilt and core proportions, so the fallback
        // accent stands down and reports that it is not the channel in use.
        FutureWellProtocolAccentVariant = FutureWellProtocolAccentNone;
    }
    SelectionRing->SetRelativeScale3D(FVector(
        SelectionRadius,
        SelectionRadius,
        SelectionHaloMesh != nullptr ? 1.0f : 0.025f));
    SelectionVFXBaseScale = SelectionRing->GetRelativeScale3D();
    // Entity resolution covers the same footprint the halo draws. This is what
    // gives a Matter deposit and a Future Well a click target: both suppress
    // the silhouette accent above, and a deposit has no other overlay at all,
    // so without this volume the only thing standing for them is whatever
    // collision their authored mesh happens to carry.
    ConfigureEntityPickProxy(SelectionRadius);

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
            bUsingAuthoredRosterMesh
                ? FVector(0.001f, 0.001f, 0.001f)
                : (bFacesAlongX
                       ? FVector(0.10f, 1.45f, 0.58f)
                       : FVector(1.45f, 0.10f, 0.58f)));
    }
    DeploymentCover->SetVisibility(bShowDeploymentCover, true);

    // Both discs are area-of-effect statements, so both are drawn at the
    // radius the simulation actually uses. The live rule set is preferred; the
    // authored defaults stand in only when no simulation is reachable, and
    // both are the same authority - never a hand-chosen mesh scale.
    const UEchoesSimulationSubsystem* RulesBridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    const echoes::sim::Simulation* RulesSimulation =
        RulesBridge != nullptr ? RulesBridge->GetSimulation() : nullptr;
    if (RulesSimulation != nullptr)
    {
        const echoes::sim::SimulationRules& LiveRules =
            RulesSimulation->Config().rules;
        RelaySupplyFieldRadiusCentimetres = AbilityDiscRadiusCentimetres(
            LiveRules.relaySupply.connectionRadiusRaw);
        AegisPowerFieldRadiusCentimetres = AbilityDiscRadiusCentimetres(
            LiveRules.poweredAegis.connectionRadiusRaw);
    }
    else
    {
        const echoes::sim::SimulationRules DefaultRules =
            echoes::sim::DefaultSimulationRules();
        RelaySupplyFieldRadiusCentimetres = AbilityDiscRadiusCentimetres(
            DefaultRules.relaySupply.connectionRadiusRaw);
        AegisPowerFieldRadiusCentimetres = AbilityDiscRadiusCentimetres(
            DefaultRules.poweredAegis.connectionRadiusRaw);
    }
    RelaySupplyField->SetRelativeScale3D(
        AbilityDiscScale(RelaySupplyFieldRadiusCentimetres, 0.025f));
    AegisPowerField->SetRelativeScale3D(
        AbilityDiscScale(AegisPowerFieldRadiusCentimetres, 0.045f));

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
    SetOverlayVisibleAndPickable(OwnerMarker, MarkerMesh != nullptr);

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
        if (DamageAcknowledgeMarkerMaterial == nullptr)
        {
            DamageAcknowledgeMarkerMaterial =
                CreateOwnedMaterial(BasicMaterial);
            DamageAcknowledgeMarker->SetMaterial(
                0, DamageAcknowledgeMarkerMaterial);
        }
        if (DamageAcknowledgeMarkerMaterial != nullptr)
        {
            // Held flat: no emissive, no per-frame change. The marker's
            // information is its presence and its shape, not its brightness.
            DamageAcknowledgeMarkerMaterial->SetVectorParameterValue(
                EntityColorParameterName,
                FLinearColor(1.0f, 0.82f, 0.28f));
            DamageAcknowledgeMarkerMaterial->SetScalarParameterValue(
                EmissiveStrengthParameterName,
                0.0f);
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
            // A3 surface families per slot. Compact: machined metal on the
            // load-bearing slots, pale ceramic on the LIGHT plates. Kharuun
            // and Choir carry one family across the body so strata and
            // lattice run continuously over the silhouette.
            const ESurfaceTextureFamily SlotFamilies[] = {
                bChoir ? ESurfaceTextureFamily::ChoirCoherent
                : bKharuun ? ESurfaceTextureFamily::KharuunMineral
                           : ESurfaceTextureFamily::CompactMetal,
                bChoir ? ESurfaceTextureFamily::ChoirCoherent
                : bKharuun ? ESurfaceTextureFamily::KharuunMineral
                           : ESurfaceTextureFamily::CompactMetal,
                bChoir ? ESurfaceTextureFamily::ChoirCoherent
                : bKharuun ? ESurfaceTextureFamily::KharuunMineral
                           : ESurfaceTextureFamily::CeramicCivic,
                bChoir ? ESurfaceTextureFamily::ChoirCoherent
                : bKharuun ? ESurfaceTextureFamily::KharuunMineral
                           : ESurfaceTextureFamily::CompactMetal};
            // Mask-driven glow in the family's own colour: amber nodules,
            // magenta-lilac lattice. Compact metal carries no mask.
            const FLinearColor FamilyEmissiveTint =
                bChoir
                    ? FLinearColor(0.851f, 0.412f, 0.553f)
                : bKharuun
                    ? FLinearColor(1.0f, 0.55f, 0.12f)
                    : FLinearColor::White;
            const float MaskedEmissiveValues[] = {
                bChoir ? 0.9f : bKharuun ? 0.8f : 0.0f,
                bChoir ? 0.6f : bKharuun ? 0.5f : 0.0f,
                bChoir ? 1.2f : bKharuun ? 1.1f : 0.0f,
                bChoir ? 1.5f : bKharuun ? 1.3f : 0.0f};
            const UEchoesGameUserSettings* SurfaceSettings =
                UEchoesGameUserSettings::Get();
            const bool bHoldSurfaceSteady =
                SurfaceSettings != nullptr &&
                SurfaceSettings->IsReducedMotionEnabled();
            const float ViewShift =
                bChoir && !bHoldSurfaceSteady ? 0.6f : 0.0f;
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
                ApplySurfaceTextureFamily(Material, SlotFamilies[MaterialIndex]);
                Material->SetVectorParameterValue(
                    EmissiveTintParameterName,
                    FamilyEmissiveTint);
                Material->SetScalarParameterValue(
                    MaskedEmissiveStrengthParameterName,
                    MaskedEmissiveValues[MaterialIndex]);
                Material->SetScalarParameterValue(
                    ViewShiftParameterName,
                    ViewShift);
            }
        }
        if (bUsingAuthoredResourceMesh && BodyMaterials.Num() >= 4)
        {
            // Gate 50: the spire slot was pale ceramic-cyan and read as white
            // under the sun; the concept's deposits are deep cyan glass with an
            // interior glow, subordinate to the Well in brightness.
            const FLinearColor SlotColors[] = {
                FLinearColor(0.055f, 0.075f, 0.085f),
                FLinearColor(0.012f, 0.020f, 0.027f),
                FLinearColor(0.04f, 0.30f, 0.42f),
                FLinearColor(0.16f, 0.80f, 1.0f)};
            const float MetallicValues[] = {0.16f, 0.48f, 0.30f, 0.10f};
            const float RoughnessValues[] = {0.72f, 0.18f, 0.22f, 0.14f};
            const float EmissiveValues[] = {0.0f, 0.0f, 0.35f, 1.6f};
            // A3: Matter deposit crystal on every slot; interior glow pools
            // in cyan-white through the family mask, held steady.
            const float MaskedEmissiveValues[] = {1.0f, 0.4f, 1.4f, 1.8f};
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
                ApplySurfaceTextureFamily(
                    Material, ESurfaceTextureFamily::MatterCrystal);
                Material->SetVectorParameterValue(
                    EmissiveTintParameterName,
                    FLinearColor(0.56f, 0.94f, 1.0f));
                Material->SetScalarParameterValue(
                    MaskedEmissiveStrengthParameterName,
                    MaskedEmissiveValues[MaterialIndex]);
                Material->SetScalarParameterValue(ViewShiftParameterName, 0.0f);
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
    if (FParse::Param(FCommandLine::Get(), TEXT("EchoesArtReviewHideUI")))
    {
        if (OwnerMarker != nullptr) OwnerMarker->SetVisibility(false, true);
        if (HealthBarBackground != nullptr) HealthBarBackground->SetVisibility(false, true);
        if (HealthBarFill != nullptr) HealthBarFill->SetVisibility(false, true);
        if (SilhouetteAccent != nullptr) SilhouetteAccent->SetVisibility(false, true);
        if (SelectionRing != nullptr) SelectionRing->SetVisibility(false, true);
        if (DamageAcknowledgeMarker != nullptr) DamageAcknowledgeMarker->SetVisibility(false, true);
    }
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
            SetOverlayVisibleAndPickable(Component, false);
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
            OuterLocation.Z = 240.0f;
            InnerLocation.Z = 185.0f;
            CoreLocation.Z = 220.0f;
            OuterScale = FVector(1.42f);
            InnerScale = FVector(1.10f);
            CoreScale = FVector(1.35f, 1.35f, 2.05f);
            GlyphAScale = FVector(1.15f, 1.15f, 0.45f);
            GlyphBScale = FVector(0.85f, 0.85f, 0.45f);
            OuterRotation = FRotator(26.0f, -18.0f, 12.0f);
            InnerRotation = FRotator(-24.0f, 35.0f, -14.0f);
            GlyphBRotation.Yaw = 22.5f;
            bShowGlyphA = true;
            bShowGlyphB = true;
            SecondaryColor = FLinearColor(0.04f, 0.62f, 0.78f);
            GlowColor = FLinearColor(1.0f, 0.75f, 0.18f);
            SecondaryEmissive = 1.1f;
            GlowEmissive = 3.2f;
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

    // The orbit, core and glyphs are what a player points at when they point
    // at a Future Well, so they carry the entity-pick region with them. They
    // still stay out of ECC_Visibility: the orbit sits 145-224 cm above the
    // ground and must never become a ground answer.
    SetOverlayVisibleAndPickable(FutureWellOrbitOuter, true);
    SetOverlayVisibleAndPickable(FutureWellOrbitInner, true);
    SetOverlayVisibleAndPickable(FutureWellCore, true);
    SetOverlayVisibleAndPickable(FutureWellGroundGlyphA, bShowGlyphA);
    SetOverlayVisibleAndPickable(FutureWellGroundGlyphB, bShowGlyphB);

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
    // A3 landmark families: the foundation, orbits, and ground glyphs are
    // vitrified glass (charcoal body, magenta micro-fracture), the
    // unrealized-future core is Matter crystal. The mask-driven glow takes
    // the state colour so Dormant/Harvest/Preserve/Reshape stay readable
    // through the family rather than despite it; it is held steady.
    auto ApplyPalette = [&StoneColor,
                         &GlassColor,
                         &SecondaryColor,
                         &GlowColor](
                            TArray<TObjectPtr<UMaterialInstanceDynamic>>& Materials,
                            float InSecondaryEmissive,
                            float InGlowEmissive,
                            ESurfaceTextureFamily Family,
                            float InMaskedEmissive)
    {
        if (Materials.Num() < 4)
        {
            return;
        }
        const float MaskedEmissive[] = {
            InMaskedEmissive * 0.35f,
            InMaskedEmissive * 0.20f,
            InMaskedEmissive * 0.70f,
            InMaskedEmissive};
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
            ApplySurfaceTextureFamily(Material, Family);
            Material->SetVectorParameterValue(
                EmissiveTintParameterName,
                GlowColor);
            Material->SetScalarParameterValue(
                MaskedEmissiveStrengthParameterName,
                MaskedEmissive[MaterialIndex]);
            Material->SetScalarParameterValue(ViewShiftParameterName, 0.0f);
        }
    };

    ApplyPalette(
        BodyMaterials,
        SecondaryEmissive * 0.08f,
        GlowEmissive * 0.72f,
        ESurfaceTextureFamily::VitrifiedGlass,
        GlowEmissive * 0.30f);
    ApplyPalette(
        FutureWellOrbitOuterMaterials,
        SecondaryEmissive * 0.30f,
        GlowEmissive,
        ESurfaceTextureFamily::VitrifiedGlass,
        GlowEmissive * 0.45f);
    ApplyPalette(
        FutureWellOrbitInnerMaterials,
        SecondaryEmissive * 0.38f,
        GlowEmissive * 1.08f,
        ESurfaceTextureFamily::VitrifiedGlass,
        GlowEmissive * 0.50f);
    ApplyPalette(
        FutureWellCoreMaterials,
        SecondaryEmissive * 0.85f,
        GlowEmissive * 1.28f,
        ESurfaceTextureFamily::MatterCrystal,
        GlowEmissive * 0.90f);
    ApplyPalette(
        FutureWellGroundGlyphAMaterials,
        SecondaryEmissive,
        GlowEmissive * 0.90f,
        ESurfaceTextureFamily::VitrifiedGlass,
        GlowEmissive * 0.40f);
    ApplyPalette(
        FutureWellGroundGlyphBMaterials,
        SecondaryEmissive,
        GlowEmissive * 0.90f,
        ESurfaceTextureFamily::VitrifiedGlass,
        GlowEmissive * 0.40f);
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
    SetOverlayVisibleAndPickable(HealthBarBackground, bShowHealth);
    SetOverlayVisibleAndPickable(
        HealthBarFill, bShowHealth && DisplayedHealthFraction > 0.0f);
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

bool AEchoesEntityView::IsDamageAcknowledgeMarkerVisible() const
{
    return DamageAcknowledgeMarker != nullptr &&
           DamageAcknowledgeMarker->IsVisible();
}

bool AEchoesEntityView::IsDamageAcknowledgeMarkerEntityPickable() const
{
    return IsOverlayEntityPickable(DamageAcknowledgeMarker);
}

float AEchoesEntityView::GetRelaySupplyFieldRadiusCentimetres() const
{
    return RelaySupplyFieldRadiusCentimetres;
}

float AEchoesEntityView::GetAegisPowerFieldRadiusCentimetres() const
{
    return AegisPowerFieldRadiusCentimetres;
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

void AEchoesEntityView::UpdateComponentMotion(
    float DeltaSeconds,
    bool bReducedMotion)
{
    if (BodyMesh == nullptr)
    {
        return;
    }

    if (bReducedMotion)
    {
        BodyMesh->SetRelativeLocation(FVector::ZeroVector);
        BodyMesh->SetRelativeRotation(FRotator(0.0f, CurrentHeadingYaw, 0.0f));
        BodyMesh->SetRelativeScale3D(FVector::OneVector);
        if (SilhouetteAccent != nullptr)
        {
            SilhouetteAccent->SetRelativeLocation(FVector::ZeroVector);
            SilhouetteAccent->SetRelativeRotation(
                BaseSilhouetteAccentRotation + FRotator(0.0f, CurrentHeadingYaw, 0.0f));
            SilhouetteAccent->SetRelativeScale3D(FVector::OneVector);
        }
        HoverBobOffsetCentimetres = 0.0f;
        UpdateTacticalStateMotion(DeltaSeconds, true);
        return;
    }

    if (bIsWalkerUnit)
    {
        UpdateWalkerMotion(DeltaSeconds, AuthoritativeSpeed, false);
    }
    else if (bIsHoverUnit)
    {
        UpdateHoverMotion(DeltaSeconds, AuthoritativeSpeed, false);
    }
    else
    {
        UpdateIdleMotion(DeltaSeconds, false);
    }

    UpdateTacticalStateMotion(DeltaSeconds, false);
    UpdateWorkerResourceMotion(DeltaSeconds, false);
}

void AEchoesEntityView::UpdateWalkerMotion(
    float DeltaSeconds,
    float Speed,
    bool bReducedMotion)
{
    if (bReducedMotion)
    {
        BodyMesh->SetRelativeLocation(FVector::ZeroVector);
        BodyMesh->SetRelativeRotation(FRotator(0.0f, CurrentHeadingYaw, 0.0f));
        BodyMesh->SetRelativeScale3D(FVector::OneVector);
        return;
    }

    if (Speed > 8.0f)
    {
        const float StrideCadence = 0.065f;
        WalkCyclePhase += Speed * DeltaSeconds * StrideCadence;
        if (WalkCyclePhase > 2.0f * PI)
        {
            WalkCyclePhase = FMath::Fmod(WalkCyclePhase, 2.0f * PI);
        }

        const float StrideBounce = FMath::Abs(FMath::Sin(WalkCyclePhase)) * 4.5f;
        const float StridePitch = FMath::Sin(WalkCyclePhase * 2.0f) * 2.2f;
        const float StrideRoll = FMath::Cos(WalkCyclePhase) * 2.5f;

        BodyMesh->SetRelativeLocation(FVector(0.0f, 0.0f, StrideBounce));
        BodyMesh->SetRelativeRotation(
            FRotator(StridePitch, CurrentHeadingYaw, StrideRoll));
        BodyMesh->SetRelativeScale3D(FVector::OneVector);
        if (SilhouetteAccent != nullptr)
        {
            SilhouetteAccent->SetRelativeLocation(FVector(0.0f, 0.0f, StrideBounce));
            SilhouetteAccent->SetRelativeRotation(
                BaseSilhouetteAccentRotation +
                FRotator(StridePitch, CurrentHeadingYaw, StrideRoll));
        }
    }
    else
    {
        UpdateIdleMotion(DeltaSeconds, false);
    }
}

void AEchoesEntityView::UpdateHoverMotion(
    float DeltaSeconds,
    float Speed,
    bool bReducedMotion)
{
    if (bReducedMotion)
    {
        HoverBobOffsetCentimetres = 0.0f;
        BodyMesh->SetRelativeLocation(FVector::ZeroVector);
        BodyMesh->SetRelativeRotation(FRotator(0.0f, CurrentHeadingYaw, 0.0f));
        BodyMesh->SetRelativeScale3D(FVector::OneVector);
        return;
    }

    HoverPhaseTime += DeltaSeconds;
    const float BobZ = FMath::Sin(HoverPhaseTime * 2.2f) * 5.5f;
    HoverBobOffsetCentimetres = BobZ;

    float BankRoll = 0.0f;
    float AccelPitch = 0.0f;
    if (Speed > 8.0f)
    {
        const float YawDelta =
            FMath::FindDeltaAngleDegrees(CurrentHeadingYaw, TargetHeadingYaw);
        BankRoll = FMath::Clamp(-YawDelta * 0.35f, -8.0f, 8.0f);
        AccelPitch = -2.5f;
    }
    else
    {
        BankRoll = FMath::Sin(HoverPhaseTime * 1.4f) * 1.2f;
        AccelPitch = FMath::Cos(HoverPhaseTime * 1.1f) * 1.0f;
    }

    const float BaseHoverElevation = 10.0f;
    BodyMesh->SetRelativeLocation(
        FVector(0.0f, 0.0f, BaseHoverElevation + BobZ));
    BodyMesh->SetRelativeRotation(
        FRotator(AccelPitch, CurrentHeadingYaw, BankRoll));
    BodyMesh->SetRelativeScale3D(FVector::OneVector);

    if (SilhouetteAccent != nullptr)
    {
        SilhouetteAccent->SetRelativeLocation(
            FVector(0.0f, 0.0f, BaseHoverElevation + BobZ));
        SilhouetteAccent->SetRelativeRotation(
            BaseSilhouetteAccentRotation +
            FRotator(AccelPitch, CurrentHeadingYaw, BankRoll));
    }
}

void AEchoesEntityView::UpdateIdleMotion(
    float DeltaSeconds,
    bool bReducedMotion)
{
    if (bReducedMotion)
    {
        BodyMesh->SetRelativeLocation(FVector::ZeroVector);
        BodyMesh->SetRelativeRotation(FRotator(0.0f, CurrentHeadingYaw, 0.0f));
        BodyMesh->SetRelativeScale3D(FVector::OneVector);
        if (SilhouetteAccent != nullptr)
        {
            SilhouetteAccent->SetRelativeLocation(FVector::ZeroVector);
        }
        return;
    }

    IdlePhaseTime += DeltaSeconds;

    if (EntityFaction == echoes::sim::Faction::MeridianCompact)
    {
        const float ServoCycle = FMath::Sin(IdlePhaseTime * 0.8f);
        const float ServoTick =
            (ServoCycle > 0.85f) ? (ServoCycle - 0.85f) * 6.0f * 0.8f : 0.0f;
        BodyMesh->SetRelativeLocation(FVector::ZeroVector);
        BodyMesh->SetRelativeRotation(
            FRotator(0.0f, CurrentHeadingYaw + ServoTick, 0.0f));
        BodyMesh->SetRelativeScale3D(FVector::OneVector);
        if (SilhouetteAccent != nullptr)
        {
            SilhouetteAccent->SetRelativeLocation(FVector::ZeroVector);
            SilhouetteAccent->SetRelativeRotation(
                BaseSilhouetteAccentRotation +
                FRotator(0.0f, CurrentHeadingYaw + ServoTick, 0.0f));
        }
    }
    else if (EntityFaction == echoes::sim::Faction::KharuunAssemblies)
    {
        const float Breath = FMath::Sin(IdlePhaseTime * 1.5f);
        const float ScaleY = 1.0f + 0.015f * Breath;
        const float ScaleZ = 1.0f + 0.018f * Breath;
        BodyMesh->SetRelativeLocation(FVector::ZeroVector);
        BodyMesh->SetRelativeRotation(FRotator(0.0f, CurrentHeadingYaw, 0.0f));
        BodyMesh->SetRelativeScale3D(FVector(1.0f, ScaleY, ScaleZ));
        if (SilhouetteAccent != nullptr)
        {
            SilhouetteAccent->SetRelativeLocation(FVector::ZeroVector);
            SilhouetteAccent->SetRelativeScale3D(FVector(1.0f, ScaleY, ScaleZ));
        }
    }
    else if (EntityFaction == echoes::sim::Faction::HollowChoir)
    {
        const float DriftX = FMath::Sin(IdlePhaseTime * 1.3f) * 2.2f;
        const float DriftY = FMath::Cos(IdlePhaseTime * 1.7f) * 2.2f;
        const float DriftZ = FMath::Sin(IdlePhaseTime * 2.1f) * 1.8f;

        BodyMesh->SetRelativeLocation(FVector(DriftX, DriftY, DriftZ));
        BodyMesh->SetRelativeRotation(FRotator(0.0f, CurrentHeadingYaw, 0.0f));
        BodyMesh->SetRelativeScale3D(FVector::OneVector);

        if (SilhouetteAccent != nullptr)
        {
            const float OffsetX =
                FMath::Sin((IdlePhaseTime + 0.8f) * 1.3f) * -2.0f;
            const float OffsetY =
                FMath::Cos((IdlePhaseTime + 0.8f) * 1.7f) * -2.0f;
            const float OffsetZ =
                FMath::Sin((IdlePhaseTime + 0.8f) * 2.1f) * -1.5f;
            SilhouetteAccent->SetRelativeLocation(
                FVector(OffsetX, OffsetY, OffsetZ));
        }
    }
    else
    {
        BodyMesh->SetRelativeLocation(FVector::ZeroVector);
        BodyMesh->SetRelativeRotation(FRotator(0.0f, CurrentHeadingYaw, 0.0f));
        BodyMesh->SetRelativeScale3D(FVector::OneVector);
    }
}

void AEchoesEntityView::UpdateTacticalStateMotion(
    float DeltaSeconds,
    bool bReducedMotion)
{
    if (DeploymentCover != nullptr && DeploymentCover->IsVisible())
    {
        const FVector TargetCoverScale =
            bDeployed
                ? (bUsingAuthoredRosterMesh ? FVector(0.001f, 0.001f, 0.001f)
                                           : FVector(1.0f, 1.0f, 1.0f))
                : FVector(0.01f, 0.01f, 0.01f);
        if (bReducedMotion)
        {
            DeploymentCover->SetRelativeScale3D(TargetCoverScale);
        }
        else
        {
            DeploymentCover->SetRelativeScale3D(
                FMath::VInterpTo(
                    DeploymentCover->GetRelativeScale3D(),
                    TargetCoverScale,
                    DeltaSeconds,
                    8.0f));
        }
    }

    if (bTemporaryMineralCover && BodyMesh != nullptr)
    {
        FVector CurrentLoc = BodyMesh->GetRelativeLocation();
        CurrentLoc.Z -= 18.0f;
        BodyMesh->SetRelativeLocation(CurrentLoc);
    }

    if (WaystoneStateField != nullptr && WaystoneStateField->IsVisible())
    {
        if (WaystoneMode == echoes::sim::WaystoneMode::Rooted)
        {
            WaystoneStateField->SetRelativeLocation(FVector(0.0f, 0.0f, 2.0f));
        }
        else if (WaystoneMode == echoes::sim::WaystoneMode::Mobile)
        {
            const float ElevateZ =
                bReducedMotion
                    ? 6.0f
                    : 6.0f + FMath::Sin(IdlePhaseTime * 2.0f) * 1.5f;
            WaystoneStateField->SetRelativeLocation(
                FVector(0.0f, 0.0f, ElevateZ));
        }
    }
}

void AEchoesEntityView::UpdateWorkerResourceMotion(
    float DeltaSeconds,
    bool bReducedMotion)
{
    if (EntityType != echoes::sim::EntityType::Worker || BodyMesh == nullptr)
    {
        return;
    }

    if (bWorkerHarvestingActive)
    {
        WorkerHarvestPhaseTime += DeltaSeconds;
        if (!bReducedMotion)
        {
            const float HarvestPitch =
                FMath::Sin(WorkerHarvestPhaseTime * 5.0f) * 3.5f;
            FRotator CurrentRot = BodyMesh->GetRelativeRotation();
            CurrentRot.Pitch += HarvestPitch;
            BodyMesh->SetRelativeRotation(CurrentRot);
        }
    }
    else
    {
        WorkerHarvestPhaseTime = 0.0f;
    }

    if (CarriedCargoAmount > 0 && !bReducedMotion)
    {
        FRotator CurrentRot = BodyMesh->GetRelativeRotation();
        CurrentRot.Pitch -= 1.8f;
        BodyMesh->SetRelativeRotation(CurrentRot);
    }
}

FVector AEchoesEntityView::GetBodyMeshRelativeLocation() const
{
    return BodyMesh != nullptr ? BodyMesh->GetRelativeLocation() : FVector::ZeroVector;
}

FRotator AEchoesEntityView::GetBodyMeshRelativeRotation() const
{
    return BodyMesh != nullptr ? BodyMesh->GetRelativeRotation() : FRotator::ZeroRotator;
}

FVector AEchoesEntityView::GetBodyMeshRelativeScale() const
{
    return BodyMesh != nullptr ? BodyMesh->GetRelativeScale3D() : FVector::OneVector;
}

FVector AEchoesEntityView::GetSilhouetteAccentRelativeLocation() const
{
    return SilhouetteAccent != nullptr
               ? SilhouetteAccent->GetRelativeLocation()
               : FVector::ZeroVector;
}

void AEchoesEntityView::UpdateCombatVFX(
    float DeltaSeconds,
    bool bReducedMotion,
    bool bReducedFlashing)
{
    // 1. Worker Gather Beam
    if (bGatherBeamActive && GatherBeam != nullptr)
    {
        if (GatherBeamMaterial == nullptr && AuthoredPresentationVFXMaterial != nullptr)
        {
            GatherBeamMaterial = CreateOwnedMaterial(AuthoredPresentationVFXMaterial);
            GatherBeam->SetMaterial(0, GatherBeamMaterial);
        }
        GatherBeamPulsePhase += DeltaSeconds * 6.0f;
        FLinearColor BeamColor;
        switch (EntityFaction)
        {
        case echoes::sim::Faction::MeridianCompact:
            BeamColor = FLinearColor(0.12f, 0.88f, 1.0f, 1.0f);
            break;
        case echoes::sim::Faction::KharuunAssemblies:
            BeamColor = FLinearColor(1.0f, 0.52f, 0.06f, 1.0f);
            break;
        case echoes::sim::Faction::HollowChoir:
            BeamColor = FLinearColor(0.85f, 0.22f, 0.95f, 1.0f);
            break;
        default:
            BeamColor = FLinearColor::White;
            break;
        }

        const float BaseEmissive = bReducedFlashing ? 1.0f : 3.5f;
        const float Pulse = bReducedMotion ? 0.0f : FMath::Sin(GatherBeamPulsePhase) * 0.5f;
        if (GatherBeamMaterial != nullptr)
        {
            GatherBeamMaterial->SetVectorParameterValue(TEXT("Color"), BeamColor);
            GatherBeamMaterial->SetScalarParameterValue(
                TEXT("EmissiveStrength"),
                BaseEmissive + Pulse);
        }

        GatherBeam->SetRelativeLocation(FVector(40.0f, 0.0f, 25.0f));
        GatherBeam->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
        GatherBeam->SetRelativeScale3D(FVector(0.04f, 0.04f, 0.75f));
        GatherBeam->SetVisibility(true);
    }
    else if (GatherBeam != nullptr)
    {
        GatherBeam->SetVisibility(false);
    }

    // 2. Construction Assembly Field
    if (bConstructionFieldActive && ConstructionField != nullptr)
    {
        if (ConstructionFieldMaterial == nullptr && AuthoredPresentationVFXMaterial != nullptr)
        {
            ConstructionFieldMaterial = CreateOwnedMaterial(AuthoredPresentationVFXMaterial);
            ConstructionField->SetMaterial(0, ConstructionFieldMaterial);
        }
        FLinearColor FieldColor;
        switch (EntityFaction)
        {
        case echoes::sim::Faction::MeridianCompact:
            FieldColor = FLinearColor(0.15f, 0.85f, 1.0f, 0.8f);
            break;
        case echoes::sim::Faction::KharuunAssemblies:
            FieldColor = FLinearColor(1.0f, 0.6f, 0.1f, 0.8f);
            break;
        case echoes::sim::Faction::HollowChoir:
            FieldColor = FLinearColor(0.8f, 0.2f, 0.9f, 0.8f);
            break;
        default:
            FieldColor = FLinearColor(0.7f, 0.7f, 0.7f, 0.8f);
            break;
        }

        if (ConstructionFieldMaterial != nullptr)
        {
            ConstructionFieldMaterial->SetVectorParameterValue(TEXT("Color"), FieldColor);
            ConstructionFieldMaterial->SetScalarParameterValue(
                TEXT("EmissiveStrength"),
                bReducedFlashing ? 0.8f : 1.8f);
        }

        const float BaseRadius = 1.4f;
        const float CurrentHeight = FMath::Lerp(0.15f, 1.2f, ConstructionFraction);
        ConstructionField->SetRelativeLocation(FVector(0.0f, 0.0f, 20.0f * CurrentHeight));
        ConstructionField->SetRelativeRotation(FRotator::ZeroRotator);
        ConstructionField->SetRelativeScale3D(FVector(BaseRadius, BaseRadius, CurrentHeight));
        ConstructionField->SetVisibility(true);
    }
    else if (ConstructionField != nullptr)
    {
        ConstructionField->SetVisibility(false);
    }

    // 3. Reshape Telegraph Ground Sigil
    if (bReshapeTelegraphActive && ReshapeTelegraph != nullptr)
    {
        if (ReshapeTelegraphMaterial == nullptr && AuthoredPresentationVFXMaterial != nullptr)
        {
            ReshapeTelegraphMaterial = CreateOwnedMaterial(AuthoredPresentationVFXMaterial);
            ReshapeTelegraph->SetMaterial(0, ReshapeTelegraphMaterial);
        }
        ReshapeTelegraphPulsePhase += DeltaSeconds * 3.0f;
        const FLinearColor TelegraphColor(0.92f, 0.12f, 0.85f, 1.0f);
        const float Pulse = bReducedMotion ? 0.0f : FMath::Sin(ReshapeTelegraphPulsePhase) * 0.4f;
        if (ReshapeTelegraphMaterial != nullptr)
        {
            ReshapeTelegraphMaterial->SetVectorParameterValue(TEXT("Color"), TelegraphColor);
            ReshapeTelegraphMaterial->SetScalarParameterValue(
                TEXT("EmissiveStrength"),
                (bReducedFlashing ? 0.9f : 2.5f) + Pulse);
        }

        ReshapeTelegraph->SetRelativeLocation(FVector(0.0f, 0.0f, 4.0f));
        ReshapeTelegraph->SetRelativeRotation(FRotator::ZeroRotator);
        ReshapeTelegraph->SetRelativeScale3D(FVector(3.5f, 3.5f, 0.05f));
        ReshapeTelegraph->SetVisibility(true);
    }
    else if (ReshapeTelegraph != nullptr)
    {
        ReshapeTelegraph->SetVisibility(false);
    }
}


