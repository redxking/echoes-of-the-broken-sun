// Author and owner: Angelis Pseftis
// M01 state-driven assembly of the approved Bulwark screen. No simulation,
// collision, navigation, order, range, or protection authority lives here.
#include "EchoesEntityView.h"
#include "EchoesOfTheBrokenSun.h"
#include "EchoesSimulationSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"

void AEchoesEntityView::ResetM01BulwarkParts()
{
    bUsingM01BulwarkParts = false;
    M01BulwarkFoldDegrees = 85.0f;
    for (UStaticMeshComponent* Wing : M01BulwarkWings)
    {
        Wing->SetVisibility(false);
        Wing->SetRelativeTransform(FTransform::Identity);
        Wing->SetStaticMesh(nullptr);
        for (int32 Slot = 0; Slot < 4; ++Slot) Wing->SetMaterial(Slot, nullptr);
    }
}

void AEchoesEntityView::ConfigureM01BulwarkParts()
{
    const auto* Bridge = GetWorld() ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>() : nullptr;
    const bool bEligible = Bridge && Bridge->GetOperationMode() == EEchoesOperationMode::CampaignPrologue &&
        EntityFaction == echoes::sim::Faction::MeridianCompact &&
        EntityType == echoes::sim::EntityType::HeavyUnit && bUsingAuthoredRosterMesh;
    if (!bEligible)
    {
        if (bUsingM01BulwarkParts || !M01BulwarkWings.IsEmpty()) ResetM01BulwarkParts();
        return;
    }
    UStaticMesh* Meshes[3];
    const TCHAR* Suffixes[] = {TEXT("Body"), TEXT("LeftWing"), TEXT("RightWing")};
    for (int32 Index = 0; Index < 3; ++Index)
    {
        const FString Name = FString::Printf(TEXT("SM_Meridian_M01Bulwark%s"), Suffixes[Index]);
        Meshes[Index] = LoadObject<UStaticMesh>(nullptr,
            *FString::Printf(TEXT("/Game/Art/Generated/Meridian/Units/%s.%s"), *Name, *Name));
        if (!Meshes[Index])
        {
            // ConfigureAppearance already restored the complete standard mesh.
            // An incomplete derivative package must never expose a bare chassis.
            ResetM01BulwarkParts();
            return;
        }
    }
    if (M01BulwarkWings.IsEmpty())
    {
        for (int32 Side = 0; Side < 2; ++Side)
        {
            auto* Wing = NewObject<UStaticMeshComponent>(this,
                Side ? TEXT("M01BulwarkRightWing") : TEXT("M01BulwarkLeftWing"));
            AddInstanceComponent(Wing);
            // BodyMesh already carries the displayed deployment-facing transform
            // and the parent readability scale; hinge coordinates are source cm.
            Wing->SetupAttachment(BodyMesh);
            Wing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Wing->SetCollisionResponseToAllChannels(ECR_Ignore);
            Wing->SetGenerateOverlapEvents(false);
            Wing->SetCanEverAffectNavigation(false);
            Wing->SetReceivesDecals(false);
            Wing->SetCastShadow(true);
            Wing->RegisterComponent();
            M01BulwarkWings.Add(Wing);
        }
    }
    const bool bFirstBinding = !bUsingM01BulwarkParts;
    BodyMesh->SetStaticMesh(Meshes[0]);
    for (int32 Side = 0; Side < 2; ++Side)
    {
        UStaticMeshComponent* Wing = M01BulwarkWings[Side];
        Wing->SetStaticMesh(Meshes[Side + 1]);
        Wing->SetRelativeLocation(FVector(26, Side ? 24 : -24, 72));
        Wing->SetRelativeScale3D(FVector::OneVector);
        for (int32 Slot = 0; Slot < BodyMaterials.Num(); ++Slot) Wing->SetMaterial(Slot, BodyMaterials[Slot]);
        Wing->SetVisibility(true);
    }
    bUsingM01BulwarkParts = true;
    DeploymentCover->SetVisibility(false);
    if (bFirstBinding)
    {
        M01BulwarkFoldDegrees = bDeployed ? 0.0f : 85.0f;
        UE_LOG(LogEchoes, Display,
            TEXT("[ECHOES_M01_BULWARK_PARTS_BOUND] entity=%u deployed=%d revision=m01-bulwark-deployment-parts-v1 wings=2 collision=false"),
            EntityId, bDeployed);
    }
    UpdateM01BulwarkParts(0.0f, false);
}

void AEchoesEntityView::UpdateM01BulwarkParts(float DeltaSeconds, bool bReducedMotion)
{
    if (!bUsingM01BulwarkParts || M01BulwarkWings.Num() != 2) return;
    const float Target = bDeployed ? 0.0f : 85.0f;
    M01BulwarkFoldDegrees = bReducedMotion ? Target :
        FMath::FInterpConstantTo(M01BulwarkFoldDegrees, Target, DeltaSeconds, 250.0f);
    for (int32 Side = 0; Side < 2; ++Side)
        M01BulwarkWings[Side]->SetRelativeRotation(FRotator(0, (Side ? 1 : -1) * M01BulwarkFoldDegrees, 0));
    // The derivative wings carry this state. Suppress the legacy nearly-zero
    // cube instead of drawing a second, detached claim about protected space.
    DeploymentCover->SetVisibility(false);
}
