// Author: Angelis Pseftis
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EchoesFieldHudView.h"
#include "EchoesPowerNetworkView.generated.h"
class UInstancedStaticMeshComponent;
class UMaterialInterface;

/** Cosmetic consumer of scoped network evidence; never a power or path authority. */
UCLASS(Transient, NotBlueprintable)
class ECHOESOFTHEBROKENSUN_API AEchoesPowerNetworkView final : public AActor
{
    GENERATED_BODY()
public:
    AEchoesPowerNetworkView();
    void SetView(const FEchoesFieldHudView& View);
    virtual void Tick(float DeltaSeconds) override;
private:
    // Strong reflected ownership keeps components/materials alive across garbage collection.
    UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> Conduits;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UInstancedStaticMeshComponent> Energy;
    UPROPERTY(Transient) TObjectPtr<UMaterialInterface> Material;
    TArray<FEchoesNetworkConnectionView> Links;
    bool bReducedMotion = false;
    double Phase = 0;
};
