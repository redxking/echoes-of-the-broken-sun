#include "EchoesGameMode.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EchoesCommandMarkerView.h"
#include "EchoesDestructionView.h"
#include "EchoesHUD.h"
#include "EchoesEntityView.h"
#include "EchoesFogView.h"
#include "EchoesGameUserSettings.h"
#include "EchoesOfTheBrokenSun.h"
#include "EchoesPlayerController.h"
#include "EchoesPresentationAudioSubsystem.h"
#include "EchoesRTSCameraPawn.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesTerrainView.h"
#include "EchoesWeatherView.h"
#include "Engine/DirectionalLight.h"
#include "Engine/Engine.h"
#include "Engine/PointLight.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/HUD.h"
#include "HAL/PlatformTime.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/Guid.h"
#include "Misc/Parse.h"
#include "TimerManager.h"

namespace
{
const FName EnvironmentColorParameterName(TEXT("Color"));
const FName EnvironmentMetallicParameterName(TEXT("Metallic"));
const FName EnvironmentRoughnessParameterName(TEXT("Roughness"));
const FName EnvironmentEmissiveParameterName(TEXT("EmissiveStrength"));

[[nodiscard]] bool IsBoundedResumeCredential(const FString& Credential)
{
    if (Credential.Len() != 32)
    {
        return false;
    }
    for (const TCHAR Character : Credential)
    {
        if (!FChar::IsHexDigit(Character))
        {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool ResumeCredentialsMatch(
    const FString& Candidate,
    const FString& Expected)
{
    if (!IsBoundedResumeCredential(Candidate) ||
        !IsBoundedResumeCredential(Expected))
    {
        return false;
    }
    uint32 Difference = 0;
    for (int32 Index = 0; Index < 32; ++Index)
    {
        Difference |= static_cast<uint32>(Candidate[Index] ^ Expected[Index]);
    }
    return Difference == 0;
}
}

AEchoesGameMode::AEchoesGameMode()
{
    DefaultPawnClass = AEchoesRTSCameraPawn::StaticClass();
    PlayerControllerClass = AEchoesPlayerController::StaticClass();
    HUDClass = AEchoesHUD::StaticClass();
}

void AEchoesGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);
    if (GetNetMode() != NM_ListenServer || NewPlayer == nullptr ||
        NewPlayer->IsLocalController())
    {
        return;
    }
    AEchoesPlayerController* EchoesController =
        Cast<AEchoesPlayerController>(NewPlayer);
    if (bNetworkSeatReserved && !IsNetworkSeatReservationAvailable())
    {
        ExpireNetworkSeatReservation();
    }
    if (EchoesController == nullptr || NetworkRemoteController.IsValid())
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NETWORK_SEAT_REJECTED] reason=NET_SEAT_UNAVAILABLE"));
        NewPlayer->ClientReturnToMainMenuWithTextReason(
            FText::FromString(TEXT("NET_SEAT_UNAVAILABLE")));
        return;
    }
    NetworkRemoteController = NewPlayer;
    if (bNetworkSeatReserved)
    {
        bNetworkResumeValidationPending = true;
        GetWorldTimerManager().SetTimer(
            NetworkResumeValidationTimer,
            this,
            &AEchoesGameMode::ExpireNetworkResumeValidation,
            5.0f,
            false);
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_NETWORK_RESUME_VALIDATION_PENDING] player=%u disconnectTick=%llu timeoutSeconds=5 seatActivated=false"),
            UEchoesSimulationSubsystem::OpponentPlayerId,
            static_cast<unsigned long long>(NetworkReservedDisconnectTick));
        return;
    }

    NetworkResumeCredential = GenerateNetworkResumeCredential();
    NetworkReservedLastBatchId = 0;
    NetworkReservedDisconnectTick = 0;
    bNetworkReservedMatchStarted = false;
    EchoesController->ConfigureNetworkSeat(
        UEchoesSimulationSubsystem::OpponentPlayerId);
    EchoesController->ConfigureNetworkResumeCredential(
        NetworkResumeCredential);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_NETWORK_SEAT_BOUND] player=%u connectionBound=true sharedControl=false"),
        UEchoesSimulationSubsystem::OpponentPlayerId);
}

void AEchoesGameMode::Logout(AController* Exiting)
{
    if (NetworkRemoteController.Get() == Exiting)
    {
        if (bNetworkResumeValidationPending)
        {
            NetworkRemoteController.Reset();
            bNetworkResumeValidationPending = false;
            GetWorldTimerManager().ClearTimer(NetworkResumeValidationTimer);
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_NETWORK_RESUME_ATTEMPT_ENDED] player=%u seatReservationPreserved=true"),
                UEchoesSimulationSubsystem::OpponentPlayerId);
            Super::Logout(Exiting);
            return;
        }
        const AEchoesPlayerController* EchoesController =
            Cast<AEchoesPlayerController>(Exiting);
        NetworkReservedLastBatchId =
            EchoesController != nullptr
                ? EchoesController->GetLastAcceptedNetworkBatchId()
                : 0;
        bNetworkReservedMatchStarted =
            EchoesController != nullptr &&
            EchoesController->HasNetworkMatchStarted();
        const UEchoesSimulationSubsystem* Bridge =
            GetWorld() != nullptr
                ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
                : nullptr;
        const echoes::sim::Simulation* Simulation =
            Bridge != nullptr ? Bridge->GetSimulation() : nullptr;
        NetworkReservedDisconnectTick =
            Simulation != nullptr ? Simulation->CurrentTick() : 0;
        bNetworkSeatReserved =
            EchoesController != nullptr &&
            EchoesController->IsNetworkCompatibilityAccepted() &&
            !NetworkResumeCredential.IsEmpty();
        NetworkReservationExpiresAt =
            FPlatformTime::Seconds() + NetworkResumeGraceSeconds;
        NetworkRemoteController.Reset();
        if (!bNetworkReservedMatchStarted)
        {
            if (UEchoesSimulationSubsystem* MutableBridge =
                    GetWorld() != nullptr
                        ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
                        : nullptr)
            {
                MutableBridge->SetNetworkHumanOpponent(false);
            }
        }
        if (bNetworkSeatReserved)
        {
            GetWorldTimerManager().SetTimer(
                NetworkReservationTimer,
                this,
                &AEchoesGameMode::ExpireNetworkSeatReservation,
                NetworkResumeGraceSeconds,
                false);
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_NETWORK_SEAT_RESERVED] player=%u disconnectTick=%llu lastAcceptedBatch=%llu matchStarted=%s graceSeconds=%.0f aiControl=false credentialLogged=false"),
                UEchoesSimulationSubsystem::OpponentPlayerId,
                static_cast<unsigned long long>(NetworkReservedDisconnectTick),
                static_cast<unsigned long long>(NetworkReservedLastBatchId),
                bNetworkReservedMatchStarted ? TEXT("true") : TEXT("false"),
                NetworkResumeGraceSeconds);
        }
        else
        {
            ExpireNetworkSeatReservation();
        }
    }
    Super::Logout(Exiting);
}

bool AEchoesGameMode::TryResumeNetworkPlayer(
    AEchoesPlayerController* Controller,
    const FString& Credential,
    FString& OutError)
{
    OutError.Reset();
    if (Controller == nullptr || NetworkRemoteController.Get() != Controller ||
        !bNetworkResumeValidationPending ||
        !IsNetworkSeatReservationAvailable() ||
        !ResumeCredentialsMatch(Credential, NetworkResumeCredential))
    {
        OutError = TEXT("NET_RESUME_CREDENTIAL_INVALID_OR_UNAVAILABLE");
        UE_LOG(
            LogEchoes,
            Warning,
            TEXT("[ECHOES_NETWORK_RESUME_REJECTED] reason=%s credentialLogged=false seatReservationPreserved=true"),
            *OutError);
        return false;
    }

    Controller->ConfigureNetworkResume(
        UEchoesSimulationSubsystem::OpponentPlayerId,
        NetworkReservedLastBatchId,
        NetworkReservedDisconnectTick,
        bNetworkReservedMatchStarted);
    bNetworkResumeValidationPending = false;
    bNetworkSeatReserved = false;
    NetworkReservationExpiresAt = 0.0;
    GetWorldTimerManager().ClearTimer(NetworkResumeValidationTimer);
    GetWorldTimerManager().ClearTimer(NetworkReservationTimer);
    NetworkResumeCredential = GenerateNetworkResumeCredential();
    Controller->ConfigureNetworkResumeCredential(NetworkResumeCredential);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_NETWORK_SEAT_RESUMED] player=%u disconnectTick=%llu lastAcceptedBatch=%llu credentialMatched=true credentialRotated=true credentialLogged=false sharedControl=false"),
        UEchoesSimulationSubsystem::OpponentPlayerId,
        static_cast<unsigned long long>(NetworkReservedDisconnectTick),
        static_cast<unsigned long long>(NetworkReservedLastBatchId));
    return true;
}

bool AEchoesGameMode::IsNetworkSeatReservationAvailable() const
{
    return bNetworkSeatReserved && !NetworkResumeCredential.IsEmpty() &&
           FPlatformTime::Seconds() < NetworkReservationExpiresAt;
}

FString AEchoesGameMode::GenerateNetworkResumeCredential() const
{
    return FGuid::NewGuid().ToString(EGuidFormats::Digits);
}

void AEchoesGameMode::ExpireNetworkSeatReservation()
{
    if (!bNetworkSeatReserved && NetworkResumeCredential.IsEmpty())
    {
        return;
    }
    const double Now = FPlatformTime::Seconds();
    if (bNetworkSeatReserved && Now < NetworkReservationExpiresAt)
    {
        GetWorldTimerManager().SetTimer(
            NetworkReservationTimer,
            this,
            &AEchoesGameMode::ExpireNetworkSeatReservation,
            static_cast<float>(NetworkReservationExpiresAt - Now),
            false);
        return;
    }
    bNetworkSeatReserved = false;
    NetworkResumeCredential.Reset();
    NetworkReservedLastBatchId = 0;
    NetworkReservedDisconnectTick = 0;
    NetworkReservationExpiresAt = 0.0;
    bNetworkReservedMatchStarted = false;
    GetWorldTimerManager().ClearTimer(NetworkReservationTimer);
    if (UEchoesSimulationSubsystem* Bridge =
            GetWorld() != nullptr
                ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
                : nullptr)
    {
        Bridge->SetNetworkHumanOpponent(false);
    }
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_NETWORK_SEAT_RELEASED] player=%u reason=resumeGraceExpired aiControl=true"),
        UEchoesSimulationSubsystem::OpponentPlayerId);
}

void AEchoesGameMode::ExpireNetworkResumeValidation()
{
    if (!bNetworkResumeValidationPending)
    {
        return;
    }
    UE_LOG(
        LogEchoes,
        Warning,
        TEXT("[ECHOES_NETWORK_RESUME_REJECTED] reason=NET_RESUME_VALIDATION_TIMEOUT credentialLogged=false seatReservationPreserved=true"));
    if (APlayerController* Controller = NetworkRemoteController.Get())
    {
        Controller->ClientReturnToMainMenuWithTextReason(
            FText::FromString(TEXT("NET_RESUME_VALIDATION_TIMEOUT")));
    }
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
    const bool bCampaignTermsOfContinuance =
        !bStressScenario &&
        FParse::Param(
            FCommandLine::Get(),
            TEXT("EchoesCampaignTermsOfContinuance"));
    const bool bCampaignNamesWithoutBirths =
        !bStressScenario &&
        FParse::Param(
            FCommandLine::Get(),
            TEXT("EchoesCampaignNamesWithoutBirths"));
    const bool bCampaignShapeOfSilence =
        !bStressScenario &&
        FParse::Param(
            FCommandLine::Get(),
            TEXT("EchoesCampaignShapeOfSilence"));
    const bool bCampaignShapeBesideUs =
        !bStressScenario &&
        FParse::Param(
            FCommandLine::Get(),
            TEXT("EchoesCampaignShapeBesideUs"));
    const bool bCampaignReserveAuthority =
        !bStressScenario &&
        FParse::Param(
            FCommandLine::Get(),
            TEXT("EchoesCampaignReserveAuthority"));
    const int32 CampaignOperationCount =
        (bCampaignPrologue ? 1 : 0) +
        (bCampaignSevenAccounts ? 1 : 0) +
        (bCampaignCityReserve ? 1 : 0) +
        (bCampaignUnburiedRoad ? 1 : 0) +
        (bCampaignTermsOfContinuance ? 1 : 0) +
        (bCampaignNamesWithoutBirths ? 1 : 0) +
        (bCampaignShapeOfSilence ? 1 : 0) +
        (bCampaignShapeBesideUs ? 1 : 0) +
        (bCampaignReserveAuthority ? 1 : 0);
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
            bCampaignReserveAuthority
                ? EEchoesOperationMode::CampaignReserveAuthority
            : bCampaignShapeBesideUs
                ? EEchoesOperationMode::CampaignShapeBesideUs
            : bCampaignShapeOfSilence
                ? EEchoesOperationMode::CampaignShapeOfSilence
            : bCampaignNamesWithoutBirths
                ? EEchoesOperationMode::CampaignNamesWithoutBirths
            : bCampaignTermsOfContinuance
                ? EEchoesOperationMode::CampaignTermsOfContinuance
            : bCampaignUnburiedRoad
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
                bCampaignReserveAuthority
                    ? TEXT("ReserveAuthority")
                : bCampaignShapeBesideUs
                    ? TEXT("TheShapeBesideUs")
                : bCampaignShapeOfSilence
                    ? TEXT("TheShapeOfSilence")
                : bCampaignNamesWithoutBirths
                    ? TEXT("NamesWithoutBirths")
                : bCampaignUnburiedRoad
                    ? TEXT("TheUnburiedRoad")
                : bCampaignTermsOfContinuance
                    ? TEXT("TermsOfContinuance")
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
            bCampaignReserveAuthority
                ? TEXT("ReserveAuthority")
            : bCampaignShapeBesideUs
                ? TEXT("TheShapeBesideUs")
            : bCampaignShapeOfSilence
                ? TEXT("TheShapeOfSilence")
            : bCampaignNamesWithoutBirths
                ? TEXT("NamesWithoutBirths")
            : bCampaignUnburiedRoad
                ? TEXT("TheUnburiedRoad")
            : bCampaignTermsOfContinuance
                ? TEXT("TermsOfContinuance")
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

    if (GetNetMode() == NM_ListenServer)
    {
        Bridge->SetNetworkHumanOpponent(true);
        Bridge->SetScenarioPaused(true);
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_NETWORK_AUTHORITY_WAITING] tick=%llu paused=true player=%u readyGate=true smoke=%s"),
            static_cast<unsigned long long>(
                Bridge->GetSimulation() != nullptr
                    ? Bridge->GetSimulation()->CurrentTick()
                    : 0),
            UEchoesSimulationSubsystem::OpponentPlayerId,
            FParse::Param(
                FCommandLine::Get(), TEXT("EchoesNetworkListenSmoke")) ||
                    FParse::Param(
                        FCommandLine::Get(), TEXT("EchoesNetworkMatchSmoke"))
                ? TEXT("true")
                : TEXT("false"));
    }

#if !UE_BUILD_SHIPPING
    const bool bPresentationVFXReview =
        FParse::Param(FCommandLine::Get(), TEXT("EchoesPresentationVFXReview"));
    if (bPresentationVFXReview)
    {
        const bool bReducedPresentation = FParse::Param(
            FCommandLine::Get(),
            TEXT("EchoesReviewReducedPresentation"));
        if (UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get())
        {
            Settings->SetReducedMotionEnabled(bReducedPresentation);
            Settings->SetReducedFlashingEnabled(bReducedPresentation);
        }
        if (AEchoesFogView* FogView = Bridge->GetFogView())
        {
            FogView->SetActorHiddenInGame(true);
        }
        if (AEchoesTerrainView* TerrainView = Bridge->GetTerrainView())
        {
            TerrainView->SetActorHiddenInGame(true);
        }

        int32 HiddenOrdinaryViewCount = 0;
        if (const echoes::sim::Simulation* Simulation = Bridge->GetSimulation())
        {
            for (const echoes::sim::Entity& Entity : Simulation->Entities())
            {
                if (AEchoesEntityView* View = Bridge->FindEntityView(Entity.id))
                {
                    View->SetActorHiddenInGame(true);
                    ++HiddenOrdinaryViewCount;
                }
            }
        }
        int32 HiddenTerrainShelfCount = 0;
        for (TActorIterator<AStaticMeshActor> ActorIterator(GetWorld());
             ActorIterator;
             ++ActorIterator)
        {
            if (ActorIterator->ActorHasTag(TEXT("EchoesTerrainShelf")))
            {
                ActorIterator->SetActorHiddenInGame(true);
                ++HiddenTerrainShelfCount;
            }
        }

        int32 SelectedPreviewCount = 0;
        const auto SpawnSelectedPreview = [this, &SelectedPreviewCount](
                                               uint32 Id,
                                               echoes::sim::EntityType Type,
                                               int32 TileX,
                                               int32 TileY)
        {
            AEchoesEntityView* Preview =
                GetWorld()->SpawnActor<AEchoesEntityView>();
            if (Preview == nullptr)
            {
                return;
            }
            echoes::sim::Entity State{};
            State.id = Id;
            State.owner = UEchoesSimulationSubsystem::LocalPlayerId;
            State.faction = echoes::sim::Faction::MeridianCompact;
            State.type = Type;
            State.position = echoes::sim::Vec2::FromTiles(TileX, TileY);
            State.hitPoints = 100;
            State.maxHitPoints = 100;
            Preview->ApplyAuthoritativeState(State, true);
            Preview->SetSelected(true);
            ++SelectedPreviewCount;
        };
        SpawnSelectedPreview(920001, echoes::sim::EntityType::Worker, 8, 8);
        SpawnSelectedPreview(920002, echoes::sim::EntityType::Soldier, 10, 8);
        SpawnSelectedPreview(920003, echoes::sim::EntityType::HeavyUnit, 12, 8);
        SpawnSelectedPreview(920004, echoes::sim::EntityType::ScoutUnit, 14, 8);

        const EEchoesCommandMarkerType MarkerTypes[] = {
            EEchoesCommandMarkerType::Move,
            EEchoesCommandMarkerType::Attack,
            EEchoesCommandMarkerType::AttackMove,
            EEchoesCommandMarkerType::Patrol,
            EEchoesCommandMarkerType::Guard,
            EEchoesCommandMarkerType::Build,
            EEchoesCommandMarkerType::Interact,
        };
        const FIntPoint MarkerTiles[] = {
            FIntPoint(7, 11),
            FIntPoint(10, 11),
            FIntPoint(13, 11),
            FIntPoint(16, 11),
            FIntPoint(8, 13),
            FIntPoint(12, 13),
            FIntPoint(16, 13),
        };
        int32 SpawnedMarkerCount = 0;
        int32 AuthoredMarkerCount = 0;
        for (int32 Index = 0; Index < UE_ARRAY_COUNT(MarkerTypes); ++Index)
        {
            FActorSpawnParameters SpawnParameters;
            SpawnParameters.ObjectFlags |= RF_Transient;
            SpawnParameters.SpawnCollisionHandlingOverride =
                ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            const FVector MarkerWorldLocation = Bridge->SimToWorld(
                echoes::sim::Vec2::FromTiles(
                    MarkerTiles[Index].X,
                    MarkerTiles[Index].Y));
            AEchoesCommandMarkerView* Marker =
                GetWorld()->SpawnActor<AEchoesCommandMarkerView>(
                    MarkerWorldLocation + FVector(0.0f, 0.0f, 12.0f),
                    FRotator::ZeroRotator,
                    SpawnParameters);
            if (Marker == nullptr)
            {
                continue;
            }
            Marker->InitializeMarker(
                MarkerTypes[Index],
                bReducedPresentation,
                bReducedPresentation,
                30.0f);
            ++SpawnedMarkerCount;
            AuthoredMarkerCount += Marker->IsUsingAuthoredVFXAssets() ? 1 : 0;
        }
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_PRESENTATION_VFX_REVIEW_READY] revision=selection-command-vfx-v2 markers=%d authoredMarkers=%d selected=%d ordinaryViewsHidden=%d terrainShelvesHidden=%d reducedMotion=%s reducedFlashing=%s collision=false authoritative=false editorOnly=true finalArt=false"),
            SpawnedMarkerCount,
            AuthoredMarkerCount,
            SelectedPreviewCount,
            HiddenOrdinaryViewCount,
            HiddenTerrainShelfCount,
            bReducedPresentation ? TEXT("true") : TEXT("false"),
            bReducedPresentation ? TEXT("true") : TEXT("false"));
    }
#endif

#if !UE_BUILD_SHIPPING
    const bool bDestructionVFXReview =
        FParse::Param(FCommandLine::Get(), TEXT("EchoesDestructionVFXReview"));
    if (bDestructionVFXReview)
    {
        const bool bReducedPresentation = FParse::Param(
            FCommandLine::Get(),
            TEXT("EchoesReviewReducedPresentation"));
        if (UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get())
        {
            Settings->SetReducedMotionEnabled(bReducedPresentation);
            Settings->SetReducedFlashingEnabled(bReducedPresentation);
        }
        if (AEchoesFogView* FogView = Bridge->GetFogView())
        {
            FogView->SetActorHiddenInGame(true);
        }
        if (AEchoesTerrainView* TerrainView = Bridge->GetTerrainView())
        {
            TerrainView->SetActorHiddenInGame(true);
        }
        int32 HiddenOrdinaryViewCount = 0;
        if (const echoes::sim::Simulation* Simulation = Bridge->GetSimulation())
        {
            for (const echoes::sim::Entity& Entity : Simulation->Entities())
            {
                if (AEchoesEntityView* View = Bridge->FindEntityView(Entity.id))
                {
                    View->SetActorHiddenInGame(true);
                    ++HiddenOrdinaryViewCount;
                }
            }
        }
        int32 HiddenTerrainShelfCount = 0;
        for (TActorIterator<AStaticMeshActor> ActorIterator(GetWorld());
             ActorIterator;
             ++ActorIterator)
        {
            if (ActorIterator->ActorHasTag(TEXT("EchoesTerrainShelf")))
            {
                ActorIterator->SetActorHiddenInGame(true);
                ++HiddenTerrainShelfCount;
            }
        }

        const echoes::sim::Faction Factions[] = {
            echoes::sim::Faction::MeridianCompact,
            echoes::sim::Faction::KharuunAssemblies,
            echoes::sim::Faction::MeridianCompact,
            echoes::sim::Faction::KharuunAssemblies,
            echoes::sim::Faction::MeridianCompact,
            echoes::sim::Faction::KharuunAssemblies,
        };
        const echoes::sim::EntityType Types[] = {
            echoes::sim::EntityType::Soldier,
            echoes::sim::EntityType::Soldier,
            echoes::sim::EntityType::HeavyUnit,
            echoes::sim::EntityType::HeavyUnit,
            echoes::sim::EntityType::CommandCore,
            echoes::sim::EntityType::CommandCore,
        };
        const FIntPoint ReviewTiles[] = {
            FIntPoint(9, 9),
            FIntPoint(12, 9),
            FIntPoint(15, 9),
            FIntPoint(9, 12),
            FIntPoint(12, 12),
            FIntPoint(15, 12),
        };
        int32 SpawnedDestructionCount = 0;
        int32 AuthoredDestructionCount = 0;
        for (int32 Index = 0; Index < UE_ARRAY_COUNT(Types); ++Index)
        {
            FActorSpawnParameters SpawnParameters;
            SpawnParameters.ObjectFlags |= RF_Transient;
            SpawnParameters.SpawnCollisionHandlingOverride =
                ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            const FVector ReviewLocation = Bridge->SimToWorld(
                echoes::sim::Vec2::FromTiles(
                    ReviewTiles[Index].X,
                    ReviewTiles[Index].Y));
            AEchoesDestructionView* Destruction =
                GetWorld()->SpawnActor<AEchoesDestructionView>(
                    ReviewLocation + FVector(0.0f, 0.0f, 10.0f),
                    FRotator::ZeroRotator,
                    SpawnParameters);
            if (Destruction == nullptr)
            {
                continue;
            }
            Destruction->InitializeDestruction(
                Factions[Index],
                Types[Index],
                bReducedPresentation,
                bReducedPresentation,
                30.0f);
            ++SpawnedDestructionCount;
            AuthoredDestructionCount +=
                Destruction->IsUsingAuthoredVFXAssets() ? 1 : 0;
        }
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_DESTRUCTION_VFX_REVIEW_READY] revision=destruction-vfx-v1 presentations=%d authored=%d ordinaryViewsHidden=%d terrainShelvesHidden=%d reducedMotion=%s reducedFlashing=%s collision=false navigation=false authoritative=false editorOnly=true finalArt=false"),
            SpawnedDestructionCount,
            AuthoredDestructionCount,
            HiddenOrdinaryViewCount,
            HiddenTerrainShelfCount,
            bReducedPresentation ? TEXT("true") : TEXT("false"),
            bReducedPresentation ? TEXT("true") : TEXT("false"));
    }
#endif

#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("EchoesAudioReview")))
    {
        const bool bReducedDynamicRange = FParse::Param(
            FCommandLine::Get(),
            TEXT("EchoesReviewReducedPresentation"));
        if (UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get())
        {
            Settings->SetEffectsVolume(1.0f);
            Settings->SetReducedDynamicRangeEnabled(bReducedDynamicRange);
        }
        if (UEchoesPresentationAudioSubsystem* Audio =
                GetWorld()->GetSubsystem<UEchoesPresentationAudioSubsystem>())
        {
            const bool bCommandPlayed = Audio->PlayCommandConfirmation();
            const bool bMeridianPlayed = Audio->PlayDestruction(
                echoes::sim::Faction::MeridianCompact,
                Bridge->SimToWorld(echoes::sim::Vec2::FromTiles(24, 24)));
            const FVector KharuunReviewLocation =
                Bridge->SimToWorld(echoes::sim::Vec2::FromTiles(40, 40));
            TWeakObjectPtr<UEchoesPresentationAudioSubsystem> WeakAudio(Audio);
            FTimerHandle KharuunTimer;
            GetWorldTimerManager().SetTimer(
                KharuunTimer,
                FTimerDelegate::CreateWeakLambda(
                    this,
                    [WeakAudio,
                     bCommandPlayed,
                     bMeridianPlayed,
                     bReducedDynamicRange,
                     KharuunReviewLocation]()
                    {
                        const bool bKharuunPlayed = WeakAudio.IsValid() &&
                            WeakAudio->PlayDestruction(
                                echoes::sim::Faction::KharuunAssemblies,
                                KharuunReviewLocation);
                        UE_LOG(
                            LogEchoes,
                            Display,
                            TEXT("[ECHOES_AUDIO_REVIEW_COMPLETE] revision=presentation-audio-v1 cuesPlayed=%d command2D=%s meridian3D=%s kharuun3D=%s reducedDynamicRange=%s effectsVolume=1.00 rateLimited=true authoritative=false editorOnly=true finalAudio=false"),
                            (bCommandPlayed ? 1 : 0) +
                                (bMeridianPlayed ? 1 : 0) +
                                (bKharuunPlayed ? 1 : 0),
                            bCommandPlayed ? TEXT("true") : TEXT("false"),
                            bMeridianPlayed ? TEXT("true") : TEXT("false"),
                            bKharuunPlayed ? TEXT("true") : TEXT("false"),
                            bReducedDynamicRange ? TEXT("true") : TEXT("false"));
                    }),
                0.16f,
                false);
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_AUDIO_REVIEW_READY] revision=presentation-audio-v1 cues=3 authored=%s sourceRate=48000 channels=1 reducedDynamicRange=%s effectsVolume=1.00 commandCooldownMs=80 destructionCooldownMs=140 authoritative=false editorOnly=true finalAudio=false"),
                Audio->HasAllAuthoredCueAssets() &&
                        Audio->HasBoundedSpatialAttenuation()
                    ? TEXT("true") : TEXT("false"),
                bReducedDynamicRange ? TEXT("true") : TEXT("false"));
        }
    }
#endif

#if !UE_BUILD_SHIPPING
    if (FParse::Param(
            FCommandLine::Get(),
            TEXT("EchoesFutureWellArtReview")))
    {
        if (const echoes::sim::Simulation* Simulation = Bridge->GetSimulation())
        {
            for (const echoes::sim::Entity& Entity : Simulation->Entities())
            {
                if (AEchoesEntityView* ExistingView =
                        Bridge->FindEntityView(Entity.id))
                {
                    ExistingView->SetActorHiddenInGame(true);
                }
            }
        }
        if (AEchoesFogView* FogView = Bridge->GetFogView())
        {
            FogView->SetActorHiddenInGame(true);
        }
        if (AEchoesTerrainView* TerrainView = Bridge->GetTerrainView())
        {
            TerrainView->SetActorHiddenInGame(true);
        }
        int32 HiddenEnvironmentActorCount = 0;
        int32 RecoloredEnvironmentActorCount = 0;
        for (TActorIterator<AStaticMeshActor> ActorIterator(GetWorld());
             ActorIterator;
             ++ActorIterator)
        {
            AStaticMeshActor* EnvironmentActor = *ActorIterator;
            if (EnvironmentActor->ActorHasTag(TEXT("EchoesScarBand")) ||
                EnvironmentActor->ActorHasTag(TEXT("EchoesGlassShard")))
            {
                EnvironmentActor->SetActorHiddenInGame(true);
                ++HiddenEnvironmentActorCount;
                continue;
            }
            if (EnvironmentActor->ActorHasTag(TEXT("EchoesPlaceholder")))
            {
                UStaticMeshComponent* EnvironmentMesh =
                    EnvironmentActor->GetStaticMeshComponent();
                UMaterialInstanceDynamic* EnvironmentMaterial =
                    EnvironmentMesh != nullptr
                        ? EnvironmentMesh->CreateDynamicMaterialInstance(0)
                        : nullptr;
                if (EnvironmentMaterial == nullptr)
                {
                    continue;
                }
                const bool bArenaFloor =
                    EnvironmentActor->GetActorScale3D().X > 50.0f;
                EnvironmentMaterial->SetVectorParameterValue(
                    EnvironmentColorParameterName,
                    bArenaFloor
                        ? FLinearColor(0.010f, 0.016f, 0.024f)
                        : FLinearColor(0.024f, 0.038f, 0.052f));
                ++RecoloredEnvironmentActorCount;
            }
        }
        AEchoesEntityView* Preview =
            GetWorld()->SpawnActor<AEchoesEntityView>();
        if (Preview != nullptr)
        {
            echoes::sim::Entity PreviewState{};
            PreviewState.id = 900001;
            PreviewState.owner = echoes::sim::kNeutralPlayer;
            PreviewState.type = echoes::sim::EntityType::FutureWell;
            PreviewState.position = echoes::sim::Vec2::FromTiles(10, 10);
            PreviewState.hitPoints = 1;
            PreviewState.maxHitPoints = 1;
            Preview->ApplyAuthoritativeState(PreviewState, true);
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_FUTURE_WELL_ART_REVIEW_READY] preview=true ordinaryViewsHidden=true environmentActorsHidden=%d environmentActorsRecolored=%d tile=(10,10) editorOnly=true"),
                HiddenEnvironmentActorCount,
                RecoloredEnvironmentActorCount);
        }
        else
        {
            UE_LOG(
                LogEchoes,
                Error,
                TEXT("[ECHOES_FUTURE_WELL_ART_REVIEW_FAILED] reason=preview-spawn"));
        }
    }
#endif

#if !UE_BUILD_SHIPPING
    FString GlassScarReviewMode;
    const bool bGlassScarArtReview =
        FParse::Value(
            FCommandLine::Get(),
            TEXT("EchoesGlassScarReview="),
            GlassScarReviewMode) ||
        FParse::Param(
            FCommandLine::Get(),
            TEXT("EchoesGlassScarArtReview"));
    if (bGlassScarArtReview)
    {
        if (GlassScarReviewMode.IsEmpty())
        {
            GlassScarReviewMode = TEXT("Overview");
        }
        const bool bKnownMode =
            GlassScarReviewMode.Equals(TEXT("Overview"), ESearchCase::IgnoreCase) ||
            GlassScarReviewMode.Equals(TEXT("AshCut"), ESearchCase::IgnoreCase) ||
            GlassScarReviewMode.Equals(TEXT("BuriedCauseway"), ESearchCase::IgnoreCase) ||
            GlassScarReviewMode.Equals(TEXT("FoldedVerge"), ESearchCase::IgnoreCase);
        if (!bKnownMode)
        {
            UE_LOG(
                LogEchoes,
                Error,
                TEXT("[ECHOES_GLASS_SCAR_ART_REVIEW_FAILED] reason=unknown-mode value=%s"),
                *GlassScarReviewMode);
        }
        else
        {
            if (const echoes::sim::Simulation* Simulation = Bridge->GetSimulation())
            {
                for (const echoes::sim::Entity& Entity : Simulation->Entities())
                {
                    if (AEchoesEntityView* ExistingView =
                            Bridge->FindEntityView(Entity.id))
                    {
                        ExistingView->SetActorHiddenInGame(true);
                    }
                }
            }
            if (AEchoesFogView* FogView = Bridge->GetFogView())
            {
                FogView->SetActorHiddenInGame(true);
            }
            if (AEchoesTerrainView* TerrainView = Bridge->GetTerrainView())
            {
                TerrainView->SetActorHiddenInGame(true);
            }

            int32 PreviewEntityCount = 0;
            const auto SpawnPreview = [this, &PreviewEntityCount](
                                          uint32 Id,
                                          echoes::sim::EntityType Type,
                                          echoes::sim::Faction Faction,
                                          uint8 Owner,
                                          int32 TileX,
                                          int32 TileY)
            {
                AEchoesEntityView* Preview =
                    GetWorld()->SpawnActor<AEchoesEntityView>();
                if (Preview == nullptr)
                {
                    return;
                }
                echoes::sim::Entity State{};
                State.id = Id;
                State.type = Type;
                State.faction = Faction;
                State.owner = Owner;
                State.position = echoes::sim::Vec2::FromTiles(TileX, TileY);
                State.hitPoints = 1;
                State.maxHitPoints = 1;
                Preview->ApplyAuthoritativeState(State, true);
                ++PreviewEntityCount;
            };

            if (GlassScarReviewMode.Equals(
                    TEXT("Overview"),
                    ESearchCase::IgnoreCase))
            {
                SpawnPreview(
                    910001,
                    echoes::sim::EntityType::CommandCore,
                    echoes::sim::Faction::MeridianCompact,
                    0,
                    10,
                    10);
                SpawnPreview(
                    910002,
                    echoes::sim::EntityType::CommandCore,
                    echoes::sim::Faction::KharuunAssemblies,
                    1,
                    54,
                    54);
                SpawnPreview(
                    910003,
                    echoes::sim::EntityType::FutureWell,
                    echoes::sim::Faction::MeridianCompact,
                    echoes::sim::kNeutralPlayer,
                    32,
                    32);
                for (const FIntPoint Tile : {
                         FIntPoint(14, 16),
                         FIntPoint(48, 16),
                         FIntPoint(32, 25),
                         FIntPoint(16, 48),
                         FIntPoint(49, 47)})
                {
                    SpawnPreview(
                        910100 + PreviewEntityCount,
                        echoes::sim::EntityType::ResourceNode,
                        echoes::sim::Faction::MeridianCompact,
                        echoes::sim::kNeutralPlayer,
                        Tile.X,
                        Tile.Y);
                }
            }
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_GLASS_SCAR_ART_REVIEW_READY] mode=%s previewEntities=%d ordinaryViewsHidden=true fogHidden=true terrainGridHidden=true editorOnly=true"),
                *GlassScarReviewMode,
                PreviewEntityCount);
        }
    }
#endif

    if (AEchoesPlayerController* Controller =
            Cast<AEchoesPlayerController>(GetWorld()->GetFirstPlayerController()))
    {
        Controller->NotifyRuntimeReady();
#if !UE_BUILD_SHIPPING
        if (FParse::Param(
                FCommandLine::Get(),
                TEXT("EchoesPointerCombatGuardReview")))
        {
            Controller->StartPointerCombatGuardReview();
        }
        if ((FParse::Param(
                 FCommandLine::Get(),
                 TEXT("EchoesFutureWellArtReview")) ||
             FParse::Param(
                 FCommandLine::Get(),
                 TEXT("EchoesPresentationVFXReview")) ||
             FParse::Param(
                 FCommandLine::Get(),
                 TEXT("EchoesDestructionVFXReview")) ||
             FParse::Value(
                 FCommandLine::Get(),
                 TEXT("EchoesGlassScarReview="),
                 GlassScarReviewMode) ||
             FParse::Param(
                 FCommandLine::Get(),
                 TEXT("EchoesGlassScarArtReview"))) &&
            Controller->GetHUD() != nullptr)
        {
            Controller->GetHUD()->bShowHUD = false;
        }
#endif
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
    UMaterialInterface* SurfaceMaterial = LoadObject<UMaterialInterface>(
        nullptr,
        TEXT("/Game/Art/Generated/Materials/M_EchoesWorldSurface.M_EchoesWorldSurface"));
    UStaticMesh* ShelfMesh = LoadObject<UStaticMesh>(
        nullptr,
        TEXT("/Game/Art/Generated/World/Environment/SM_World_GlassScarShelf.SM_World_GlassScarShelf"));
    UStaticMesh* RidgeMesh = LoadObject<UStaticMesh>(
        nullptr,
        TEXT("/Game/Art/Generated/World/Environment/SM_World_GlassScarRidge.SM_World_GlassScarRidge"));
    UStaticMesh* ShardMesh = LoadObject<UStaticMesh>(
        nullptr,
        TEXT("/Game/Art/Generated/World/Environment/SM_World_GlassScarShard.SM_World_GlassScarShard"));
    UStaticMesh* AshCutMesh = LoadObject<UStaticMesh>(
        nullptr,
        TEXT("/Game/Art/Generated/World/Environment/SM_World_GlassScarAshCut.SM_World_GlassScarAshCut"));
    UStaticMesh* BuriedCausewayMesh = LoadObject<UStaticMesh>(
        nullptr,
        TEXT("/Game/Art/Generated/World/Environment/SM_World_GlassScarBuriedCauseway.SM_World_GlassScarBuriedCauseway"));
    UStaticMesh* FoldedVergeMesh = LoadObject<UStaticMesh>(
        nullptr,
        TEXT("/Game/Art/Generated/World/Environment/SM_World_GlassScarFoldedVerge.SM_World_GlassScarFoldedVerge"));
    if (CubeMesh == nullptr || SurfaceMaterial == nullptr || ShelfMesh == nullptr ||
        RidgeMesh == nullptr || ShardMesh == nullptr ||
        AshCutMesh == nullptr || BuriedCausewayMesh == nullptr ||
        FoldedVergeMesh == nullptr)
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_ENV_ASSET_MISSING] Required collision or authored Glass Scar assets were not found."));
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
    FloorMesh->SetCastShadow(false);
    FloorMesh->SetVisibility(true, true);
    Floor->SetActorScale3D(FVector(
        ArenaWidth / 100.0f,
        ArenaHeight / 100.0f,
        FloorThickness / 100.0f));
    UMaterialInstanceDynamic* FloorMaterial =
        UMaterialInstanceDynamic::Create(SurfaceMaterial, Floor);
    if (FloorMaterial != nullptr)
    {
        FloorMaterial->SetVectorParameterValue(
            EnvironmentColorParameterName,
            FLinearColor(0.018f, 0.027f, 0.032f));
        FloorMaterial->SetScalarParameterValue(TEXT("Metallic"), 0.08f);
        FloorMaterial->SetScalarParameterValue(TEXT("Roughness"), 0.88f);
        FloorMaterial->SetScalarParameterValue(TEXT("EmissiveStrength"), 0.0f);
        FloorMesh->SetMaterial(0, FloorMaterial);
    }

    const auto SpawnScarAccent = [World, SurfaceMaterial](
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
        AccentMesh->SetCanEverAffectNavigation(false);
        AccentMesh->SetGenerateOverlapEvents(false);
        AccentMesh->SetCastShadow(bCastShadow);
        AccentMesh->SetReceivesDecals(false);
        Accent->SetActorScale3D(Scale);
        if (DetailTag == TEXT("EchoesRouteAshCut") ||
            DetailTag == TEXT("EchoesRouteBuriedCauseway") ||
            DetailTag == TEXT("EchoesRouteFoldedVerge"))
        {
            // Production-oriented route meshes own their UV-driven material
            // instances. Other environment candidates still receive the shared
            // prototype palette below.
            return true;
        }
        const FLinearColor Palette[] = {
            Color,
            FLinearColor(Color.R * 0.22f, Color.G * 0.22f, Color.B * 0.25f),
            FLinearColor(
                FMath::Min(Color.R * 1.75f + 0.04f, 1.0f),
                FMath::Min(Color.G * 1.75f + 0.04f, 1.0f),
                FMath::Min(Color.B * 1.75f + 0.04f, 1.0f)),
            FLinearColor(
                FMath::Min(Color.R * 3.2f + 0.08f, 1.0f),
                FMath::Min(Color.G * 3.2f + 0.04f, 1.0f),
                FMath::Min(Color.B * 3.2f + 0.10f, 1.0f))};
        for (int32 MaterialIndex = 0; MaterialIndex < 4; ++MaterialIndex)
        {
            UMaterialInstanceDynamic* AccentMaterial =
                UMaterialInstanceDynamic::Create(SurfaceMaterial, Accent);
            if (AccentMaterial == nullptr)
            {
                Accent->Destroy();
                return false;
            }
            AccentMaterial->SetVectorParameterValue(
                EnvironmentColorParameterName,
                Palette[MaterialIndex]);
            AccentMaterial->SetScalarParameterValue(
                EnvironmentMetallicParameterName,
                MaterialIndex == 1 ? 0.46f : 0.14f);
            AccentMaterial->SetScalarParameterValue(
                EnvironmentRoughnessParameterName,
                MaterialIndex == 1 ? 0.18f : 0.66f);
            AccentMaterial->SetScalarParameterValue(
                EnvironmentEmissiveParameterName,
                MaterialIndex == 3 ? 1.8f : 0.0f);
            AccentMesh->SetMaterial(MaterialIndex, AccentMaterial);
        }
        return true;
    };

    struct FTerrainShelfSpec final
    {
        FVector Location;
        float YawDegrees;
    };
    const FTerrainShelfSpec TerrainShelves[] = {
        {FVector(-4550.0f, -4550.0f, 2.0f), 7.0f},
        {FVector(4550.0f, -4550.0f, 2.0f), 83.0f},
        {FVector(-4550.0f, 4550.0f, 2.0f), -83.0f},
        {FVector(4550.0f, 4550.0f, 2.0f), 173.0f},
    };
    int32 SpawnedTerrainShelves = 0;
    for (const FTerrainShelfSpec& Spec : TerrainShelves)
    {
        SpawnedTerrainShelves += SpawnScarAccent(
                                     ShelfMesh,
                                     Spec.Location,
                                     FRotator(0.0f, Spec.YawDegrees, 0.0f),
                                     FVector(4.2f, 4.2f, 0.72f),
                                     FLinearColor(0.034f, 0.047f, 0.055f),
                                     TEXT("EchoesTerrainShelf"),
                                     false)
                                     ? 1
                                     : 0;
    }

    struct FRouteSpec final
    {
        UStaticMesh* Mesh;
        FVector Location;
        FLinearColor Color;
        FName Tag;
    };
    const FRouteSpec Routes[] = {
        {AshCutMesh,
         FVector(-3800.0f, 0.0f, 18.0f),
         FLinearColor(0.040f, 0.032f, 0.030f),
         TEXT("EchoesRouteAshCut")},
        {BuriedCausewayMesh,
         FVector(0.0f, 0.0f, 20.0f),
         FLinearColor(0.13f, 0.12f, 0.10f),
         TEXT("EchoesRouteBuriedCauseway")},
        {FoldedVergeMesh,
         FVector(3400.0f, 0.0f, 20.0f),
         FLinearColor(0.040f, 0.026f, 0.068f),
         TEXT("EchoesRouteFoldedVerge")},
    };
    int32 SpawnedRoutes = 0;
    for (const FRouteSpec& Spec : Routes)
    {
        SpawnedRoutes += SpawnScarAccent(
                             Spec.Mesh,
                             Spec.Location,
                             FRotator::ZeroRotator,
                             FVector::OneVector,
                             Spec.Color,
                             Spec.Tag,
                             true)
                             ? 1
                             : 0;
    }

    struct FScarBandSpec final
    {
        FVector Location;
        float YawDegrees;
        FVector Scale;
        FLinearColor Color;
    };
    const FScarBandSpec ScarBands[] = {
        {FVector(-5720.0f, -75.0f, 1.5f), -7.0f,
         FVector(5.8f, 1.15f, 0.62f), FLinearColor(0.19f, 0.025f, 0.09f)},
        {FVector(-4750.0f, 45.0f, 1.6f), 9.0f,
         FVector(3.7f, 1.30f, 0.62f), FLinearColor(0.24f, 0.055f, 0.035f)},
        {FVector(-2820.0f, -55.0f, 1.7f), -11.0f,
         FVector(6.7f, 1.42f, 0.62f), FLinearColor(0.22f, 0.025f, 0.10f)},
        {FVector(-1450.0f, 55.0f, 1.8f), 8.0f,
         FVector(4.8f, 1.48f, 0.62f), FLinearColor(0.27f, 0.07f, 0.025f)},
        {FVector(1300.0f, -45.0f, 1.7f), -10.0f,
         FVector(5.2f, 1.38f, 0.62f), FLinearColor(0.22f, 0.025f, 0.10f)},
        {FVector(2400.0f, 55.0f, 1.6f), 10.0f,
         FVector(3.7f, 1.28f, 0.62f), FLinearColor(0.24f, 0.055f, 0.035f)},
        {FVector(5200.0f, -70.0f, 1.5f), -8.0f,
         FVector(11.5f, 1.15f, 0.62f), FLinearColor(0.19f, 0.025f, 0.09f)},
    };
    int32 SpawnedScarBands = 0;
    for (const FScarBandSpec& Spec : ScarBands)
    {
        SpawnedScarBands += SpawnScarAccent(
                                RidgeMesh,
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
                                  ShardMesh,
                                  FVector(
                                      Spec.Location.X,
                                      Spec.Location.Y,
                                      10.0f),
                                  FRotator(0.0f, Spec.YawDegrees, 0.0f),
                                  FVector(
                                      Spec.Scale.X * 1.5f,
                                      Spec.Scale.Y * 1.5f,
                                      Spec.Scale.Z * 0.65f),
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
    if (SpawnedTerrainShelves != UE_ARRAY_COUNT(TerrainShelves) ||
        SpawnedRoutes != UE_ARRAY_COUNT(Routes) ||
        SpawnedScarBands != UE_ARRAY_COUNT(ScarBands) ||
        SpawnedGlassShards != UE_ARRAY_COUNT(GlassShards) ||
        SpawnedScarGlows != UE_ARRAY_COUNT(ScarGlows))
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_SCAR_COMPOSITION_FAILED] shelves=%d/%d routes=%d/%d bands=%d/%d shards=%d/%d glows=%d/%d"),
            SpawnedTerrainShelves,
            UE_ARRAY_COUNT(TerrainShelves),
            SpawnedRoutes,
            UE_ARRAY_COUNT(Routes),
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
    SunComponent->SetIntensity(12.0f);
    SunComponent->SetLightColor(FLinearColor(1.0f, 0.86f, 0.72f));
    Sky->GetLightComponent()->SetIntensity(1.1f);

    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_ENV_READY] terrainComposition=glass_scar_v5 authoredAssets=7 shelves=4 routes=3 ashCutRouteKit=production_v1 ashCutUVs=2 ashCutMaterials=4 ashCutRuntimeCollision=false buriedCausewayRouteKit=production_v1 buriedCausewayUVs=2 buriedCausewayMaterials=4 buriedCausewayRuntimeCollision=false foldedVergeRouteKit=production_v1 foldedVergeUVs=2 foldedVergeMaterials=4 foldedVergeRuntimeCollision=false bands=7 shards=12 glows=5 collisionAuthority=false routeAuthority=false finalArt=false"));
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
