// Author: Angelis Pseftis
#include "EchoesPowerNetworkView.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AEchoesPowerNetworkView::AEchoesPowerNetworkView()
{
    PrimaryActorTick.bCanEverTick = true;
    SetCanBeDamaged(false);
    SetReplicates(false);
    Conduits = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Conduits"));
    SetRootComponent(Conduits);
    Energy = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Energy"));
    Energy->SetupAttachment(Conduits);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Shader(TEXT("/Game/Art/Generated/Materials/M_EchoesPresentationVFX.M_EchoesPresentationVFX"));
    Material = Shader.Object;
    for (UInstancedStaticMeshComponent* Part : {Conduits.Get(), Energy.Get()})
    {
        Part->SetStaticMesh(Cylinder.Object);
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part->SetGenerateOverlapEvents(false);
        Part->SetCanEverAffectNavigation(false);
        Part->SetCastShadow(false);
    }
}

void AEchoesPowerNetworkView::SetView(const FEchoesFieldHudView& View)
{
    const bool bVisible = View.Authority == EEchoesFieldHudAuthority::LivePlayerView &&
        View.Surface == EEchoesFieldHudSurface::Battlefield;
    SetActorHiddenInGame(!bVisible);
    bReducedMotion = View.bReducedMotion || View.bReducedFlashing;
    SetActorTickEnabled(bVisible && !bReducedMotion && !View.NetworkConnections.IsEmpty());
    const TArray<FEchoesNetworkConnectionView> Empty;
    const auto& Next = bVisible ? View.NetworkConnections : Empty;
    bool bChanged = Links.Num() != Next.Num();
    if (!bChanged)
        for (int32 I = 0; I < Links.Num(); ++I)
            bChanged |= Links[I].From != Next[I].From || Links[I].To != Next[I].To;
    if (bChanged)
    {
        Links = Next;
        Conduits->ClearInstances();
        Energy->ClearInstances();
        for (const auto& Link : Links)
        {
            const FVector Delta = Link.To - Link.From;
            const FQuat Rotation = FRotationMatrix::MakeFromZ(Delta).ToQuat();
            // Engine cylinder is 100 cm long with a 50 cm radius. No physics geometry.
            Conduits->AddInstance(FTransform(Rotation, (Link.From + Link.To) * 0.5,
                FVector(0.13, 0.13, Delta.Size() / 100.0)), true);
            Energy->AddInstance(FTransform(Rotation, (Link.From + Link.To) * 0.5,
                FVector(0.19, 0.19, 0.65)), true);
        }
    }
    // Selection/placement raises contrast without adding another gameplay connection.
    const bool bEmphasized = !View.NetworkCoverage.IsEmpty();
    for (UInstancedStaticMeshComponent* Part : {Conduits.Get(), Energy.Get()})
    {
        auto* Dynamic = Cast<UMaterialInstanceDynamic>(Part->GetMaterial(0));
        if (!Dynamic && Material)
        {
            Dynamic = UMaterialInstanceDynamic::Create(Material, this);
            Part->SetMaterial(0, Dynamic);
        }
        if (Dynamic)
        {
            Dynamic->SetVectorParameterValue(TEXT("Color"), Part == Energy ?
                FLinearColor(0.15f, 0.85f, 1.0f) : FLinearColor(0.05f, 0.25f, 0.32f));
            Dynamic->SetScalarParameterValue(TEXT("EmissiveStrength"), bEmphasized ? 1.2f : 0.55f);
        }
    }
    // Reduced motion holds a steady lit segment, including after changing the setting.
    if (bReducedMotion) { Phase = 0.5; Tick(0); }
}

void AEchoesPowerNetworkView::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Phase = FMath::Fmod(Phase + DeltaSeconds * 0.35, 1.0);
    for (int32 I = 0; I < Links.Num(); ++I)
    {
        const auto& Link = Links[I];
        Energy->UpdateInstanceTransform(I, FTransform(
            FRotationMatrix::MakeFromZ(Link.To - Link.From).ToQuat(),
            FMath::Lerp(Link.From, Link.To, Phase), FVector(0.19, 0.19, 0.65)),
            true, I == Links.Num() - 1, true);
    }
}
