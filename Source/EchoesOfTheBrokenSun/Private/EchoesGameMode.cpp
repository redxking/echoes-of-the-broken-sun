#include "EchoesGameMode.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EchoesHUD.h"
#include "EchoesOfTheBrokenSun.h"
#include "EchoesPlayerController.h"
#include "EchoesRTSCameraPawn.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/DirectionalLight.h"
#include "Engine/Engine.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
const FName ColorParameterName(TEXT("Color"));
}

AEchoesGameMode::AEchoesGameMode()
{
    DefaultPawnClass = AEchoesRTSCameraPawn::StaticClass();
    PlayerControllerClass = AEchoesPlayerController::StaticClass();
    HUDClass = AEchoesHUD::StaticClass();
}

void AEchoesGameMode::BeginPlay()
{
    Super::BeginPlay();

    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr)
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_BOOT_NO_SUBSYSTEM] Simulation subsystem was not created for the game world."));
        if (GEngine != nullptr)
        {
            GEngine->AddOnScreenDebugMessage(
                -1,
                15.0f,
                FColor::Red,
                TEXT("ECHOES BOOT FAILED: simulation subsystem unavailable"));
        }
        if (AEchoesPlayerController* Controller =
                Cast<AEchoesPlayerController>(GetWorld()->GetFirstPlayerController()))
        {
            Controller->NotifyRuntimeFailure(TEXT("ECHOES_BOOT_NO_SUBSYSTEM"));
        }
        return;
    }

    const bool bEnvironmentReady = SpawnPrototypeEnvironment();
    if (!bEnvironmentReady)
    {
        CleanupPrototypeEnvironment();
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_BOOT_INCOMPLETE] environment=failed simulation=not-started"));
        if (GEngine != nullptr)
        {
            GEngine->AddOnScreenDebugMessage(
                -1,
                15.0f,
                FColor::Red,
                TEXT("ECHOES BOOT INCOMPLETE: inspect LogEchoes for a stable failure code"));
        }
        if (AEchoesPlayerController* Controller =
                Cast<AEchoesPlayerController>(GetWorld()->GetFirstPlayerController()))
        {
            Controller->NotifyRuntimeFailure(TEXT("ECHOES_ENV_INIT_FAILED"));
        }
        return;
    }

    const bool bStressScenario =
        FParse::Param(FCommandLine::Get(), TEXT("EchoesStress400"));
    const bool bSimulationReady = bStressScenario
                                      ? Bridge->StartStressScenario()
                                      : Bridge->StartPrototypeScenario();
    if (!bSimulationReady)
    {
        CleanupPrototypeEnvironment();
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_BOOT_INCOMPLETE] environment=rolled-back simulation=failed"));
        if (GEngine != nullptr)
        {
            GEngine->AddOnScreenDebugMessage(
                -1,
                15.0f,
                FColor::Red,
                TEXT("ECHOES BOOT INCOMPLETE: inspect LogEchoes for a stable failure code"));
        }
        if (AEchoesPlayerController* Controller =
                Cast<AEchoesPlayerController>(GetWorld()->GetFirstPlayerController()))
        {
            Controller->NotifyRuntimeFailure(TEXT("ECHOES_SIM_INIT_FAILED"));
        }
        return;
    }

    if (AEchoesPlayerController* Controller =
            Cast<AEchoesPlayerController>(GetWorld()->GetFirstPlayerController()))
    {
        Controller->NotifyRuntimeReady();
    }

    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_BOOT_READY] Runtime technical prototype initialized."));
}

bool AEchoesGameMode::SpawnPrototypeEnvironment()
{
    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return false;
    }

    UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(
        nullptr,
        TEXT("/Engine/BasicShapes/Cube.Cube"));
    UMaterialInterface* BasicMaterial = LoadObject<UMaterialInterface>(
        nullptr,
        TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (CubeMesh == nullptr || BasicMaterial == nullptr)
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_ENV_ENGINE_ASSET_MISSING] Required Engine basic-shape assets were not found."));
        return false;
    }

    constexpr float ArenaWidth =
        64.0f * UEchoesSimulationSubsystem::TileWorldSize;
    constexpr float ArenaHeight =
        64.0f * UEchoesSimulationSubsystem::TileWorldSize;
    constexpr float FloorThickness = 30.0f;

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AStaticMeshActor* Floor = World->SpawnActor<AStaticMeshActor>(
        FVector(0.0f, 0.0f, -FloorThickness * 0.5f),
        FRotator::ZeroRotator,
        SpawnParameters);
    if (Floor == nullptr)
    {
        UE_LOG(LogEchoes, Error, TEXT("[ECHOES_ENV_FLOOR_SPAWN_FAILED]"));
        return false;
    }

    Floor->Tags.Add(TEXT("EchoesPlaceholder"));
    UStaticMeshComponent* FloorMesh = Floor->GetStaticMeshComponent();
    FloorMesh->SetMobility(EComponentMobility::Movable);
    FloorMesh->SetStaticMesh(CubeMesh);
    FloorMesh->SetCollisionProfileName(TEXT("BlockAll"));
    Floor->SetActorScale3D(FVector(
        ArenaWidth / 100.0f,
        ArenaHeight / 100.0f,
        FloorThickness / 100.0f));
    UMaterialInstanceDynamic* FloorMaterial =
        UMaterialInstanceDynamic::Create(BasicMaterial, Floor);
    if (FloorMaterial != nullptr)
    {
        FloorMaterial->SetVectorParameterValue(
            ColorParameterName,
            FLinearColor(0.025f, 0.045f, 0.065f));
        FloorMesh->SetMaterial(0, FloorMaterial);
    }

    // A low, colored pad makes the local spawn region legible without revealing
    // information about the opponent through the presentation layer.
    const auto SpawnFactionPad = [World, CubeMesh, BasicMaterial](
                                     const FVector& Location,
                                     const FLinearColor& Color)
    {
        FActorSpawnParameters PadSpawnParameters;
        PadSpawnParameters.SpawnCollisionHandlingOverride =
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AStaticMeshActor* Pad = World->SpawnActor<AStaticMeshActor>(
            Location,
            FRotator::ZeroRotator,
            PadSpawnParameters);
        if (Pad == nullptr)
        {
            return false;
        }
        Pad->Tags.Add(TEXT("EchoesPlaceholder"));
        UStaticMeshComponent* Mesh = Pad->GetStaticMeshComponent();
        Mesh->SetMobility(EComponentMobility::Movable);
        Mesh->SetStaticMesh(CubeMesh);
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->SetCastShadow(false);
        Pad->SetActorScale3D(FVector(7.0f, 7.0f, 0.04f));
        UMaterialInstanceDynamic* Material =
            UMaterialInstanceDynamic::Create(BasicMaterial, Pad);
        if (Material != nullptr)
        {
            Material->SetVectorParameterValue(ColorParameterName, Color);
            Mesh->SetMaterial(0, Material);
        }
        return true;
    };

    const bool bLocalPadReady = SpawnFactionPad(
        FVector(-4400.0f, -4400.0f, 2.0f),
        FLinearColor(0.02f, 0.24f, 0.31f));

    ADirectionalLight* Sun = World->SpawnActor<ADirectionalLight>(
        FVector(0.0f, 0.0f, 1800.0f),
        FRotator(-55.0f, -35.0f, 0.0f),
        SpawnParameters);
    ASkyLight* Sky = World->SpawnActor<ASkyLight>(
        FVector::ZeroVector,
        FRotator::ZeroRotator,
        SpawnParameters);
    if (Sun != nullptr)
    {
        Sun->Tags.Add(TEXT("EchoesPlaceholder"));
    }
    if (Sky != nullptr)
    {
        Sky->Tags.Add(TEXT("EchoesPlaceholder"));
    }
    if (Sun == nullptr || Sky == nullptr)
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_ENV_LIGHT_SPAWN_FAILED] directional=%s sky=%s"),
            Sun != nullptr ? TEXT("ready") : TEXT("failed"),
            Sky != nullptr ? TEXT("ready") : TEXT("failed"));
        return false;
    }

    UDirectionalLightComponent* SunComponent =
        Cast<UDirectionalLightComponent>(Sun->GetLightComponent());
    if (SunComponent == nullptr)
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_ENV_DIRECTIONAL_COMPONENT_MISSING]"));
        return false;
    }
    SunComponent->SetIntensity(7.0f);
    SunComponent->SetLightColor(FLinearColor(1.0f, 0.86f, 0.72f));
    Sky->GetLightComponent()->SetIntensity(0.7f);

    if (!bLocalPadReady)
    {
        UE_LOG(
            LogEchoes,
            Warning,
            TEXT("[ECHOES_ENV_PAD_SPAWN_FAILED] The nonessential local faction marker could not be created."));
    }

    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_ENV_READY] Engine basic shapes and runtime lighting loaded; all visuals are placeholders."));
    return true;
}

void AEchoesGameMode::CleanupPrototypeEnvironment()
{
    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return;
    }

    TArray<TWeakObjectPtr<AActor>> ActorsToDestroy;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        if (It->ActorHasTag(TEXT("EchoesPlaceholder")))
        {
            ActorsToDestroy.Add(*It);
        }
    }
    for (const TWeakObjectPtr<AActor>& Actor : ActorsToDestroy)
    {
        if (Actor.IsValid())
        {
            Actor->Destroy();
        }
    }
}
