#pragma once

#include "CoreMinimal.h"
#include "EchoesSimCore/Simulation.h"
#include "GameFramework/Actor.h"
#include "EchoesBuildPlacementPreview.generated.h"

class UProceduralMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;

enum class EEchoesBuildPreviewValidity : uint8
{
    Valid,
    InvalidWorker,
    InvalidBuilding,
    OutsideMap,
    UnknownTerrain,
    TerrainBlocked,
    Occupied,
    InsufficientResources
};

struct FEchoesBuildPlacementEvaluation final
{
    EEchoesBuildPreviewValidity Validity =
        EEchoesBuildPreviewValidity::InvalidWorker;
    int32 FootprintHalfExtentRaw = 0;

    [[nodiscard]] bool IsValid() const
    {
        return Validity == EEchoesBuildPreviewValidity::Valid;
    }
};

/** Player-scoped placement preview admission. Authority validates again on confirm. */
struct FEchoesBuildPlacementModel final
{
    [[nodiscard]] static FEchoesBuildPlacementEvaluation Evaluate(
        const echoes::sim::PlayerView& View,
        echoes::sim::EntityId WorkerId,
        echoes::sim::EntityType BuildingType,
        echoes::sim::Vec2 Position);

    [[nodiscard]] static const TCHAR* Feedback(
        EEchoesBuildPreviewValidity Validity);
};

/** Transient ground grid for the currently armed player build command. */
UCLASS(NotBlueprintable, Transient)
class ECHOESOFTHEBROKENSUN_API AEchoesBuildPlacementPreview final
    : public AActor
{
    GENERATED_BODY()

public:
    AEchoesBuildPlacementPreview();

    void SetPreview(
        const FVector& WorldPosition,
        int32 FootprintHalfExtentRaw,
        bool bValid);

private:
    void RebuildGrid(int32 FootprintHalfExtentRaw, bool bValid);

    UPROPERTY(VisibleAnywhere, Category = "Echoes|Construction")
    TObjectPtr<UProceduralMeshComponent> Grid;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInterface> PreviewMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> GridMaterial;

    int32 DisplayedHalfExtentRaw = INDEX_NONE;
    bool bDisplayedValid = false;
};
