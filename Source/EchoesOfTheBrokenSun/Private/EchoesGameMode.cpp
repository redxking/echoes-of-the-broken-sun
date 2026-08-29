#include "EchoesGameMode.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EchoesHUD.h"
#include "EchoesOfTheBrokenSun.h"
#include "EchoesPlayerController.h"
#include "EchoesRTSCameraPawn.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesWeatherView.h"
#include "Engine/DirectionalLight.h"
#include "Engine/Engine.h"
#include "Engine/PointLight.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
const FName EnvironmentColorParameterName(TEXT("Color"));
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
    FString RequestedFaction;
    if (!bStressScenario &&
        FParse::Value(
            FCommandLine::Get(),
            TEXT("EchoesFaction="),
            RequestedFaction))
    {
        echoes::sim::Faction Requested =
            echoes::sim::Faction::MeridianCompact;
        if (RequestedFaction.Equals(
                TEXT("Kharuun"),
                ESearchCase::IgnoreCase) ||
            RequestedFaction.Equals(
                TEXT("KharuunAssemblies"),
                ESearchCase::IgnoreCase))
        {
            Requested = echoes::sim::Faction::KharuunAssemblies;
        }
        else if (!RequestedFaction.Equals(
                     TEXT("Meridian"),
                     ESearchCase::IgnoreCase) &&
                 !RequestedFaction.Equals(
                     TEXT("MeridianCompact"),
                     ESearchCase::IgnoreCase))
        {
            UE_LOG(
                LogEchoes,
                Error,
                TEXT("[ECHOES_FACTION_REQUEST_REJECTED] value=%s"),
                *RequestedFaction);
            CleanupPrototypeEnvironment();
            return;
        }
        FString FactionFeedback;
        if (!Bridge->SelectLocalFaction(Requested, FactionFeedback))
        {
            UE_LOG(
                LogEchoes,
                Error,
                TEXT("[ECHOES_FACTION_REQUEST_REJECTED] value=%s detail=%s"),
                *RequestedFaction,
                *FactionFeedback);
            CleanupPrototypeEnvironment();
            return;
        }
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_FACTION_REQUESTED] value=%s accepted=true"),
            *RequestedFaction);
    }
    const bool bCampaignPrologue =
        !bStressScenario &&
        FParse::Param(FCommandLine::Get(), TEXT("EchoesCampaignPrologue"));
    const bool bCampaignSevenAccounts =
        !bStressScenario &&
        FParse::Param(
            FCommandLine::Get(),
            TEXT("EchoesCampaignSevenAccounts"));
    const bool bCampaignCityReserve =
        !bStressScenario &&
        FParse::Param(
            FCommandLine::Get(),
            TEXT("EchoesCampaignCityReserve"));
    const bool bCampaignUnburiedRoad =
        !bStressScenario &&
        FParse::Param(
            FCommandLine::Get(),
            TEXT("EchoesCampaignUnburiedRoad"));
    const int32 CampaignOperationCount =
        (bCampaignPrologue ? 1 : 0) +
        (bCampaignSevenAccounts ? 1 : 0) +
        (bCampaignCityReserve ? 1 : 0) +
        (bCampaignUnburiedRoad ? 1 : 0);
    if (CampaignOperationCount > 1)
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_OPERATION_REQUEST_REJECTED] reason=conflicting campaign operation flags"));
        CleanupPrototypeEnvironment();
        return;
    }
    if (CampaignOperationCount == 1)
    {
        const EEchoesOperationMode RequestedOperation =
            bCampaignUnburiedRoad
                ? EEchoesOperationMode::CampaignUnburiedRoad
            : bCampaignCityReserve
                ? EEchoesOperationMode::CampaignCityReserve
            : bCampaignSevenAccounts
                ? EEchoesOperationMode::CampaignSevenAccounts
                : EEchoesOperationMode::CampaignPrologue;
        FString OperationFeedback;
        if (!Bridge->SelectOperationMode(
                RequestedOperation,
                OperationFeedback))
        {
            UE_LOG(
                LogEchoes,
                Error,
                TEXT("[ECHOES_OPERATION_REQUEST_REJECTED] operation=%s detail=%s"),
                bCampaignUnburiedRoad
                    ? TEXT("TheUnburiedRoad")
                : bCampaignCityReserve
                    ? TEXT("ACityOnReserve")
                : bCampaignSevenAccounts
                    ? TEXT("SevenAccountsOfRain")
                    : TEXT("WhatTheLedgerKeeps"),
                *OperationFeedback);
            CleanupPrototypeEnvironment();
            return;
        }
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_OPERATION_REQUESTED] operation=%s accepted=true"),
            bCampaignUnburiedRoad
                ? TEXT("TheUnburiedRoad")
            : bCampaignCityReserve
                ? TEXT("ACityOnReserve")
            : bCampaignSevenAccounts
                ? TEXT("SevenAccountsOfRain")
                : TEXT("WhatTheLedgerKeeps"));
    }
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
#if WITH_EDITOR
        FString ResultPreview;
        if (FParse::Value(
                FCommandLine::Get(),
                TEXT("EchoesResultPreview="),
                ResultPreview))
        {
            echoes::sim::MatchOutcome PreviewOutcome =
                echoes::sim::MatchOutcome::Player0Victory;
            if (ResultPreview.Equals(TEXT("Defeat"), ESearchCase::IgnoreCase))
            {
                PreviewOutcome = echoes::sim::MatchOutcome::Player1Victory;
            }
            else if (ResultPreview.Equals(TEXT("Draw"), ESearchCase::IgnoreCase))
            {
                PreviewOutcome = echoes::sim::MatchOutcome::Draw;
            }
            Bridge->SetScenarioPaused(true);
            Controller->NotifyMatchFinished(PreviewOutcome);
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_RESULT_PREVIEW] authoritative=false outcome=%u editorOnly=true"),
                static_cast<uint8>(PreviewOutcome));
        }
        else
#endif
        if (!FApp::IsUnattended() &&
            !FParse::Param(FCommandLine::Get(), TEXT("EchoesAutoStart")))
        {
            Controller->PresentTitleScreen();
        }
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
    UStaticMesh* ConeMesh = LoadObject<UStaticMesh>(
        nullptr,
        TEXT("/Engine/BasicShapes/Cone.Cone"));
    UMaterialInterface* BasicMaterial = LoadObject<UMaterialInterface>(
        nullptr,
        TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (CubeMesh == nullptr || ConeMesh == nullptr || BasicMaterial == nullptr)
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
            EnvironmentColorParameterName,
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
            Material->SetVectorParameterValue(EnvironmentColorParameterName, Color);
            Mesh->SetMaterial(0, Material);
        }
        return true;
    };

    const bool bLocalPadReady = SpawnFactionPad(
        FVector(-4400.0f, -4400.0f, 2.0f),
        FLinearColor(0.02f, 0.24f, 0.31f));

    const auto SpawnScarAccent = [World, BasicMaterial](
                                     UStaticMesh* MeshAsset,
                                     const FVector& Location,
                                     const FRotator& Rotation,
                                     const FVector& Scale,
                                     const FLinearColor& Color,
                                     const FName& DetailTag,
                                     bool bCastShadow)
    {
        FActorSpawnParameters AccentSpawnParameters;
        AccentSpawnParameters.SpawnCollisionHandlingOverride =
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AStaticMeshActor* Accent = World->SpawnActor<AStaticMeshActor>(
            Location,
            Rotation,
            AccentSpawnParameters);
        if (Accent == nullptr)
        {
            return false;
        }
        Accent->Tags.Add(TEXT("EchoesPlaceholder"));
        Accent->Tags.Add(DetailTag);
        UStaticMeshComponent* AccentMesh = Accent->GetStaticMeshComponent();
        AccentMesh->SetMobility(EComponentMobility::Movable);
        AccentMesh->SetStaticMesh(MeshAsset);
        AccentMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        AccentMesh->SetGenerateOverlapEvents(false);
        AccentMesh->SetCastShadow(bCastShadow);
        AccentMesh->SetReceivesDecals(false);
        Accent->SetActorScale3D(Scale);
        UMaterialInstanceDynamic* AccentMaterial =
            UMaterialInstanceDynamic::Create(BasicMaterial, Accent);
        if (AccentMaterial == nullptr)
        {
            Accent->Destroy();
            return false;
        }
        AccentMaterial->SetVectorParameterValue(
            EnvironmentColorParameterName,
            Color);
        AccentMesh->SetMaterial(0, AccentMaterial);
        return true;
    };

    struct FScarBandSpec final
    {
        FVector Location;
        float YawDegrees;
        FVector Scale;
        FLinearColor Color;
    };
    const FScarBandSpec ScarBands[] = {
        {FVector(-5450.0f, -90.0f, 1.5f), -7.0f,
         FVector(18.0f, 1.15f, 0.035f), FLinearColor(0.19f, 0.025f, 0.09f)},
        {FVector(-3650.0f, 40.0f, 1.6f), 9.0f,
         FVector(18.5f, 1.35f, 0.038f), FLinearColor(0.24f, 0.055f, 0.035f)},
        {FVector(-1830.0f, -55.0f, 1.7f), -11.0f,
         FVector(18.0f, 1.45f, 0.040f), FLinearColor(0.22f, 0.025f, 0.10f)},
        {FVector(0.0f, 65.0f, 1.8f), 8.0f,
         FVector(18.5f, 1.55f, 0.042f), FLinearColor(0.27f, 0.07f, 0.025f)},
        {FVector(1830.0f, -45.0f, 1.7f), -10.0f,
         FVector(18.0f, 1.40f, 0.040f), FLinearColor(0.22f, 0.025f, 0.10f)},
        {FVector(3650.0f, 55.0f, 1.6f), 10.0f,
         FVector(18.5f, 1.30f, 0.038f), FLinearColor(0.24f, 0.055f, 0.035f)},
        {FVector(5450.0f, -75.0f, 1.5f), -8.0f,
         FVector(18.0f, 1.10f, 0.035f), FLinearColor(0.19f, 0.025f, 0.09f)},
    };
    int32 SpawnedScarBands = 0;
    for (const FScarBandSpec& Spec : ScarBands)
    {
        SpawnedScarBands += SpawnScarAccent(
                                CubeMesh,
                                Spec.Location,
                                FRotator(0.0f, Spec.YawDegrees, 0.0f),
                                Spec.Scale,
                                Spec.Color,
                                TEXT("EchoesScarBand"),
                                false)
                                ? 1
                                : 0;
    }

    struct FGlassShardSpec final
    {
        FVector Location;
        float YawDegrees;
        FVector Scale;
        FLinearColor Color;
    };
    const FGlassShardSpec GlassShards[] = {
        {FVector(-5250.0f, -360.0f, 75.0f), -18.0f,
         FVector(0.40f, 0.40f, 1.50f), FLinearColor(0.26f, 0.08f, 0.12f)},
        {FVector(-4700.0f, 330.0f, 52.0f), 21.0f,
         FVector(0.32f, 0.32f, 1.05f), FLinearColor(0.34f, 0.13f, 0.045f)},
        {FVector(-3500.0f, -410.0f, 62.0f), 8.0f,
         FVector(0.36f, 0.36f, 1.25f), FLinearColor(0.29f, 0.055f, 0.13f)},
        {FVector(-2550.0f, 370.0f, 45.0f), -27.0f,
         FVector(0.28f, 0.28f, 0.90f), FLinearColor(0.38f, 0.15f, 0.05f)},
        {FVector(-1400.0f, -390.0f, 70.0f), 14.0f,
         FVector(0.38f, 0.38f, 1.40f), FLinearColor(0.27f, 0.045f, 0.14f)},
        {FVector(-520.0f, 420.0f, 48.0f), -11.0f,
         FVector(0.30f, 0.30f, 0.95f), FLinearColor(0.40f, 0.17f, 0.055f)},
        {FVector(520.0f, -420.0f, 48.0f), 11.0f,
         FVector(0.30f, 0.30f, 0.95f), FLinearColor(0.40f, 0.17f, 0.055f)},
        {FVector(1400.0f, 390.0f, 70.0f), -14.0f,
         FVector(0.38f, 0.38f, 1.40f), FLinearColor(0.27f, 0.045f, 0.14f)},
        {FVector(2550.0f, -370.0f, 45.0f), 27.0f,
         FVector(0.28f, 0.28f, 0.90f), FLinearColor(0.38f, 0.15f, 0.05f)},
        {FVector(3500.0f, 410.0f, 62.0f), -8.0f,
         FVector(0.36f, 0.36f, 1.25f), FLinearColor(0.29f, 0.055f, 0.13f)},
        {FVector(4700.0f, -330.0f, 52.0f), -21.0f,
         FVector(0.32f, 0.32f, 1.05f), FLinearColor(0.34f, 0.13f, 0.045f)},
        {FVector(5250.0f, 360.0f, 75.0f), 18.0f,
         FVector(0.40f, 0.40f, 1.50f), FLinearColor(0.26f, 0.08f, 0.12f)},
    };
    int32 SpawnedGlassShards = 0;
    for (const FGlassShardSpec& Spec : GlassShards)
    {
        SpawnedGlassShards += SpawnScarAccent(
                                  ConeMesh,
                                  Spec.Location,
                                  FRotator(0.0f, Spec.YawDegrees, 0.0f),
                                  Spec.Scale,
                                  Spec.Color,
                                  TEXT("EchoesGlassShard"),
                                  true)
                                  ? 1
                                  : 0;
    }
    struct FScarGlowSpec final
    {
        FVector Location;
        FLinearColor Color;
        float Intensity;
        float AttenuationRadius;
    };
    const FScarGlowSpec ScarGlows[] = {
        {FVector(-4800.0f, -35.0f, 180.0f), FLinearColor(0.72f, 0.06f, 0.22f),
         2200.0f, 1450.0f},
        {FVector(-2400.0f, 45.0f, 160.0f), FLinearColor(0.95f, 0.24f, 0.045f),
         1800.0f, 1350.0f},
        {FVector(0.0f, 0.0f, 190.0f), FLinearColor(0.76f, 0.08f, 0.30f),
         2600.0f, 1650.0f},
        {FVector(2400.0f, -45.0f, 160.0f), FLinearColor(0.95f, 0.24f, 0.045f),
         1800.0f, 1350.0f},
        {FVector(4800.0f, 35.0f, 180.0f), FLinearColor(0.72f, 0.06f, 0.22f),
         2200.0f, 1450.0f},
    };
    int32 SpawnedScarGlows = 0;
    for (const FScarGlowSpec& Spec : ScarGlows)
    {
        APointLight* Glow = World->SpawnActor<APointLight>(
            Spec.Location,
            FRotator::ZeroRotator,
            SpawnParameters);
        if (Glow == nullptr)
        {
            continue;
        }
        Glow->Tags.Add(TEXT("EchoesPlaceholder"));
        Glow->Tags.Add(TEXT("EchoesScarGlow"));
        UPointLightComponent* GlowComponent = Glow->PointLightComponent;
        if (GlowComponent == nullptr)
        {
            Glow->Destroy();
            continue;
        }
        GlowComponent->SetMobility(EComponentMobility::Movable);
        GlowComponent->SetLightColor(Spec.Color);
        GlowComponent->SetIntensity(Spec.Intensity);
        GlowComponent->SetAttenuationRadius(Spec.AttenuationRadius);
        GlowComponent->SetSourceRadius(110.0f);
        GlowComponent->SetCastShadows(false);
        ++SpawnedScarGlows;
    }
    if (SpawnedScarBands != UE_ARRAY_COUNT(ScarBands) ||
        SpawnedGlassShards != UE_ARRAY_COUNT(GlassShards) ||
        SpawnedScarGlows != UE_ARRAY_COUNT(ScarGlows))
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_SCAR_COMPOSITION_FAILED] bands=%d/%d shards=%d/%d glows=%d/%d"),
            SpawnedScarBands,
            UE_ARRAY_COUNT(ScarBands),
            SpawnedGlassShards,
            UE_ARRAY_COUNT(GlassShards),
            SpawnedScarGlows,
            UE_ARRAY_COUNT(ScarGlows));
        return false;
    }

    AEchoesWeatherView* Weather = World->SpawnActor<AEchoesWeatherView>(
        FVector::ZeroVector,
        FRotator::ZeroRotator,
        SpawnParameters);
    if (Weather == nullptr)
    {
        UE_LOG(LogEchoes, Error, TEXT("[ECHOES_WEATHER_SPAWN_FAILED]"));
        return false;
    }

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
        TEXT("[ECHOES_ENV_READY] terrainComposition=glass_scar_v1 bands=7 shards=12 glows=5 collisionAuthority=false shadowCasting=false finalArt=false"));
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_WEATHER_READY] glassScarDrift=active reducedMotionAware=true finalArt=false"));
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
