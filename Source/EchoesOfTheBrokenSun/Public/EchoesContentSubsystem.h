#pragma once

#include "CoreMinimal.h"
#include "EchoesSimCore/Simulation.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "EchoesContentSubsystem.generated.h"

struct FEchoesFactionContent final
{
    FString Id;
    FString DisplayName;
    bool bVerticalSlicePlayable = false;
};

struct FEchoesUnitContent final
{
    FString Id;
    FString DisplayName;
    FString FactionId;
    FString Role;
    int32 MatterCost = 0;
    int32 DawnCost = 0;
    int32 MaxHealth = 0;
    int32 MoveSpeedCentimetersPerSecond = 0;
    int32 SightCentimeters = 0;
    int32 PopulationCost = 0;
    int32 ProductionTicks = 0;
    int32 WorkRate = 0;
    int32 CargoCapacity = 0;
    int32 AttackDamage = 0;
    int32 AttackRangeCentimeters = 0;
    int32 AttackCooldownTicks = 0;
    int32 DeploymentCoverDepthCentimeters = 0;
    int32 DeploymentCoverHalfWidthCentimeters = 0;
    int32 DeploymentDamageReductionPercent = 0;
    int32 DeploymentMoveSpeedPercent = 0;
};

struct FEchoesBuildingContent final
{
    FString Id;
    FString DisplayName;
    FString FactionId;
    FString Role;
    int32 MatterCost = 0;
    int32 DawnCost = 0;
    int32 MaxHealth = 0;
    int32 SightCentimeters = 0;
    int32 ConstructionTicks = 0;
    int32 LogisticsCapacity = 0;
    FIntPoint FootprintCells = FIntPoint::ZeroValue;
};

struct FEchoesFutureWellContent final
{
    int32 CaptureRadiusCentimeters = 0;
    int32 CaptureTicks = 0;
    int32 HarvestImmediateDawn = 0;
    int32 HarvestTelegraphTicks = 0;
    int32 PreserveDawnPerInterval = 0;
    int32 PreserveIntervalTicks = 0;
    int32 PreserveVisionRadiusCentimeters = 0;
    int32 ReshapeDawnCost = 0;
    int32 ReshapeManifestDurationTicks = 0;
    int32 ReshapeTelegraphTicks = 0;
};

/** Immutable runtime representation of the validated canonical data pack. */
struct ECHOESOFTHEBROKENSUN_API FEchoesContentCatalog final
{
    int32 PackVersion = 0;
    int32 SchemaVersion = 0;
    FString Sha256;
    TArray<FEchoesFactionContent> Factions;
    TArray<FEchoesUnitContent> Units;
    TArray<FEchoesBuildingContent> Buildings;
    FEchoesFutureWellContent FutureWell;

    [[nodiscard]] int32 PlayableFactionCount() const;
    [[nodiscard]] const FEchoesUnitContent* FindUnit(const FString& Id) const;
    [[nodiscard]] const FEchoesBuildingContent* FindBuilding(const FString& Id) const;
    [[nodiscard]] bool BuildSimulationRules(
        uint32 TicksPerSecond,
        echoes::sim::SimulationRules& OutRules,
        FString& OutError) const;

    static bool LoadCanonicalPack(
        const FString& PackPath,
        const FString& DigestPath,
        FEchoesContentCatalog& OutCatalog,
        FString& OutError);
};

/** Loads canonical authored gameplay data before any match can start. */
UCLASS()
class ECHOESOFTHEBROKENSUN_API UEchoesContentSubsystem final
    : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    [[nodiscard]] bool IsReady() const { return bReady; }
    [[nodiscard]] const FString& GetFailureReason() const { return FailureReason; }
    [[nodiscard]] const FEchoesContentCatalog& GetCatalog() const { return Catalog; }

    [[nodiscard]] static FString GetCanonicalPackPath();
    [[nodiscard]] static FString GetCanonicalDigestPath();

private:
    FEchoesContentCatalog Catalog;
    FString FailureReason;
    bool bReady = false;
};
