#include "EchoesGameMode.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EchoesBattlefieldPresentation.h"
#include "EchoesCommandMarkerView.h"
#include "EchoesDestructionView.h"
#include "EchoesHUD.h"
#include "EchoesEntityView.h"
#include "EchoesFactionPolicy.h"
#include "EchoesFogView.h"
#include "EchoesGameInstance.h"
#include "EchoesGameUserSettings.h"
#include "EchoesNetworkSession.h"
#include "EchoesOfTheBrokenSun.h"
#include "EchoesPlayerController.h"
#include "EchoesPresentationAudioSubsystem.h"
#include "EchoesRTSCameraPawn.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesTerrainView.h"
#include "EchoesWeatherView.h"
#include "Engine/DirectionalLight.h"
#include "Engine/Engine.h"
#include "Engine/NetConnection.h"
#include "Engine/PointLight.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/HUD.h"
#include "HAL/PlatformTime.h"
#include "IPAddress.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/Guid.h"
#include "Misc/Parse.h"
#include "TimerManager.h"

#if PLATFORM_APPLE || PLATFORM_UNIX
#include <sys/random.h>
#else
#error GenerateNetworkResumeCredential requires an OS CSPRNG source for this platform.
#endif

namespace
{
const FName EnvironmentColorParameterName(TEXT("Color"));
const FName EnvironmentMetallicParameterName(TEXT("Metallic"));
const FName EnvironmentRoughnessParameterName(TEXT("Roughness"));
const FName EnvironmentEmissiveParameterName(TEXT("EmissiveStrength"));

[[nodiscard]] bool IsBoundedServerResumeCredential(
    const FString& Credential)
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
    if (!IsBoundedServerResumeCredential(Candidate) ||
        !IsBoundedServerResumeCredential(Expected))
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

[[nodiscard]] bool IsDevelopmentLoopbackRemote(
    APlayerController* Controller)
{
#if !UE_BUILD_DEVELOPMENT
    (void)Controller;
    return false;
#else
    UNetConnection* Connection =
        Controller != nullptr ? Controller->GetNetConnection() : nullptr;
    const TSharedPtr<const FInternetAddr> RemoteAddress =
        Connection != nullptr ? Connection->GetRemoteAddr() : nullptr;
    if (!RemoteAddress.IsValid() || !RemoteAddress->IsValid())
    {
        return false;
    }
    const TArray<uint8> RawAddress = RemoteAddress->GetRawIp();
    return RawAddress.Num() == 4 && RawAddress[0] == 127;
#endif
}

AEchoesPlayerController* FindLocalEchoesController(UWorld* World)
{
    if (World == nullptr)
    {
        return nullptr;
    }
    for (FConstPlayerControllerIterator It =
             World->GetPlayerControllerIterator();
         It;
         ++It)
    {
        AEchoesPlayerController* Controller =
            Cast<AEchoesPlayerController>(It->Get());
        if (Controller != nullptr && Controller->IsLocalController())
        {
            return Controller;
        }
    }
    return nullptr;
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
    if (EchoesController == nullptr ||
        !IsDevelopmentLoopbackRemote(NewPlayer))
    {
        constexpr TCHAR Reason[] = TEXT("NET_SECURITY_LOOPBACK_ONLY");
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NETWORK_SEAT_REJECTED] reason=%s seatAssigned=false credentialIssued=false security=development_loopback_only"),
            Reason);
        if (EchoesController != nullptr)
        {
            EchoesController->RejectNetworkSessionFromServer(Reason);
            EchoesController->ClientReturnToMainMenuWithTextReason(
                FText::FromString(Reason));
        }
        return;
    }
    if (bNetworkSeatReserved && !IsNetworkSeatReservationAvailable())
    {
        ExpireNetworkSeatReservation();
    }
    if (bNetworkSessionFinished || NetworkRemoteController.IsValid())
    {
        const FString Reason = bNetworkSessionFinished
            ? TEXT("NET_MATCH_FINISHED")
            : TEXT("NET_SEAT_UNAVAILABLE");
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NETWORK_SEAT_REJECTED] reason=%s"),
            *Reason);
        if (EchoesController != nullptr)
        {
            EchoesController->RejectNetworkSessionFromServer(Reason);
            EchoesController->ClientReturnToMainMenuWithTextReason(
                FText::FromString(Reason));
        }
        return;
    }
    NetworkRemoteController = NewPlayer;
    if (bNetworkSeatReserved)
    {
        bNetworkResumeValidationPending = true;
        bNetworkResumeCompatibilityPending = false;
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
    StartNetworkHelloTimeout(EchoesController);
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
        ClearNetworkAdmissionTimers();
        if (bNetworkResumeValidationPending ||
            bNetworkResumeCompatibilityPending)
        {
            const bool bCredentialAuthenticated =
                bNetworkResumeCompatibilityPending;
            NetworkRemoteController.Reset();
            bNetworkResumeValidationPending = false;
            bNetworkResumeCompatibilityPending = false;
            GetWorldTimerManager().ClearTimer(NetworkResumeValidationTimer);
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_NETWORK_RESUME_ATTEMPT_ENDED] player=%u seatReservationPreserved=true credentialAuthenticated=%s matchStarted=%s authorityPaused=%s"),
                UEchoesSimulationSubsystem::OpponentPlayerId,
                bCredentialAuthenticated ? TEXT("true") : TEXT("false"),
                bNetworkReservedMatchStarted ? TEXT("true") : TEXT("false"),
                bNetworkReservedMatchStarted ? TEXT("true") : TEXT("false"));
            Super::Logout(Exiting);
            return;
        }
        if (bNetworkSessionFinished)
        {
            NetworkRemoteController.Reset();
            NetworkResumeCredential.Reset();
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
        if (bNetworkSeatReserved)
        {
            if (bNetworkReservedMatchStarted)
            {
                if (UEchoesSimulationSubsystem* MutableBridge =
                        GetWorld() != nullptr
                            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
                            : nullptr)
                {
                    MutableBridge->SetScenarioPaused(true);
                }
            }
            PresentHostReconnectGrace(true, NetworkResumeGraceSeconds);
            GetWorldTimerManager().SetTimer(
                NetworkReservationTimer,
                this,
                &AEchoesGameMode::ExpireNetworkSeatReservation,
                NetworkResumeGraceSeconds,
                false);
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_NETWORK_SEAT_RESERVED] player=%u disconnectTick=%llu lastAcceptedBatch=%llu matchStarted=%s graceSeconds=%.0f authorityPaused=%s aiControl=false credentialLogged=false"),
                UEchoesSimulationSubsystem::OpponentPlayerId,
                static_cast<unsigned long long>(NetworkReservedDisconnectTick),
                static_cast<unsigned long long>(NetworkReservedLastBatchId),
                bNetworkReservedMatchStarted ? TEXT("true") : TEXT("false"),
                NetworkResumeGraceSeconds,
                bNetworkReservedMatchStarted ? TEXT("true") : TEXT("false"));
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
    bNetworkResumeCompatibilityPending = true;
    GetWorldTimerManager().ClearTimer(NetworkResumeValidationTimer);
    StartNetworkHelloTimeout(Controller);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_NETWORK_RESUME_AUTHENTICATED] player=%u disconnectTick=%llu lastAcceptedBatch=%llu credentialMatched=true compatibilityPending=true seatReservationPreserved=true authorityPaused=%s credentialLogged=false"),
        UEchoesSimulationSubsystem::OpponentPlayerId,
        static_cast<unsigned long long>(NetworkReservedDisconnectTick),
        static_cast<unsigned long long>(NetworkReservedLastBatchId),
        bNetworkReservedMatchStarted ? TEXT("true") : TEXT("false"));
    return true;
}

bool AEchoesGameMode::IsBoundNetworkController(
    const AEchoesPlayerController* Controller) const
{
    return Controller != nullptr &&
        NetworkRemoteController.Get() == Controller &&
        !bNetworkResumeValidationPending &&
        !bNetworkSessionFinished;
}

bool AEchoesGameMode::IsAwaitingNetworkResumeCredential(
    const AEchoesPlayerController* Controller) const
{
    return Controller != nullptr &&
        NetworkRemoteController.Get() == Controller &&
        bNetworkSeatReserved && bNetworkResumeValidationPending &&
        !bNetworkSessionFinished;
}

bool AEchoesGameMode::NotifyNetworkCompatibilityAccepted(
    AEchoesPlayerController* Controller)
{
    if (Controller == nullptr || NetworkRemoteController.Get() != Controller ||
        bNetworkSessionFinished)
    {
        return false;
    }
    if (Controller->IsNetworkResumePending())
    {
        if (!bNetworkSeatReserved ||
            !bNetworkResumeCompatibilityPending ||
            !IsNetworkSeatReservationAvailable())
        {
            return false;
        }
    }
    GetWorldTimerManager().ClearTimer(NetworkHelloTimer);
    GetWorldTimerManager().SetTimer(
        NetworkReadyTimer,
        this,
        &AEchoesGameMode::ExpireNetworkReady,
        NetworkReadyTimeoutSeconds,
        false);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_NETWORK_READY_GATE_OPEN] timeoutSeconds=%.0f seat=%u"),
        NetworkReadyTimeoutSeconds,
        UEchoesSimulationSubsystem::OpponentPlayerId);
    return true;
}

void AEchoesGameMode::NotifyNetworkPlayerReady(
    AEchoesPlayerController* Controller)
{
    if (Controller == nullptr || NetworkRemoteController.Get() != Controller)
    {
        return;
    }
    if (Controller->IsNetworkResumePending())
    {
        if (!bNetworkSeatReserved ||
            !bNetworkResumeCompatibilityPending)
        {
            return;
        }
        const uint64 ResumedDisconnectTick = NetworkReservedDisconnectTick;
        const uint64 ResumedLastBatchId = NetworkReservedLastBatchId;
        NetworkResumeCredential = GenerateNetworkResumeCredential();
        Controller->ConfigureNetworkResumeCredential(
            NetworkResumeCredential);
        bNetworkSeatReserved = false;
        bNetworkResumeCompatibilityPending = false;
        NetworkReservationExpiresAt = 0.0;
        GetWorldTimerManager().ClearTimer(NetworkReservationTimer);
        PresentHostReconnectGrace(false, 0.0f);
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_NETWORK_SEAT_RESUMED] player=%u disconnectTick=%llu lastAcceptedBatch=%llu credentialMatched=true credentialRotated=true credentialLogged=false sharedControl=false compatibilityAccepted=true matchResumed=true seatReservationConsumed=true"),
            UEchoesSimulationSubsystem::OpponentPlayerId,
            static_cast<unsigned long long>(ResumedDisconnectTick),
            static_cast<unsigned long long>(ResumedLastBatchId));
    }
    ClearNetworkAdmissionTimers();
}

void AEchoesGameMode::RejectNetworkResumeAttempt(
    AEchoesPlayerController* Controller,
    const FString& StableReason)
{
    if (Controller == nullptr || NetworkRemoteController.Get() != Controller ||
        (!bNetworkResumeValidationPending &&
         !bNetworkResumeCompatibilityPending))
    {
        return;
    }
    Controller->RejectNetworkSessionFromServer(StableReason);
    Controller->ClientReturnToMainMenuWithTextReason(
        FText::FromString(StableReason.Left(160)));
    NetworkRemoteController.Reset();
    bNetworkResumeValidationPending = false;
    bNetworkResumeCompatibilityPending = false;
    GetWorldTimerManager().ClearTimer(NetworkResumeValidationTimer);
    ClearNetworkAdmissionTimers();
    UE_LOG(
        LogEchoes,
        Warning,
        TEXT("[ECHOES_NETWORK_RESUME_ATTEMPT_ENDED] player=%u seatReservationPreserved=true reason=%s disconnected=true"),
        UEchoesSimulationSubsystem::OpponentPlayerId,
        *StableReason.Left(96));
}

void AEchoesGameMode::NotifyNetworkMatchFinished()
{
    bNetworkSessionFinished = true;
    bNetworkSeatReserved = false;
    bNetworkResumeValidationPending = false;
    bNetworkResumeCompatibilityPending = false;
    NetworkResumeCredential.Reset();
    NetworkReservationExpiresAt = 0.0;
    GetWorldTimerManager().ClearTimer(NetworkReservationTimer);
    GetWorldTimerManager().ClearTimer(NetworkResumeValidationTimer);
    ClearNetworkAdmissionTimers();
    PresentHostReconnectGrace(false, 0.0f);
}

bool AEchoesGameMode::SurrenderNetworkHost(const FString& StableReason)
{
    if (bNetworkSessionFinished)
    {
        return false;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    FString Feedback;
    if (Bridge == nullptr ||
        !Bridge->ForfeitNetworkPlayer(
            UEchoesSimulationSubsystem::LocalPlayerId, Feedback))
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NETWORK_HOST_SURRENDER_FAILED] reason=%s detail=%s"),
            *StableReason.Left(96),
            *Feedback);
        return false;
    }
    NotifyNetworkMatchFinished();
    if (AEchoesPlayerController* Host = FindLocalEchoesController(GetWorld()))
    {
        const echoes::sim::Simulation* Simulation = Bridge->GetSimulation();
        Host->NotifyNetworkHostSurrender(
            Simulation != nullptr ? Simulation->CurrentTick() : 0,
            StableReason);
    }
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_NETWORK_HOST_SURRENDERED] outcome=2 clientResultPending=true hostReturnAfterAck=true"));
    return true;
}

void AEchoesGameMode::ReleaseNetworkSeat(
    AEchoesPlayerController* Controller,
    const FString& StableReason,
    bool bNotifyClient)
{
    if (Controller == nullptr || NetworkRemoteController.Get() != Controller)
    {
        return;
    }
    if (bNetworkSeatReserved &&
        (bNetworkResumeValidationPending ||
         bNetworkResumeCompatibilityPending))
    {
        RejectNetworkResumeAttempt(Controller, StableReason);
        return;
    }
    if (bNotifyClient)
    {
        Controller->RejectNetworkSessionFromServer(StableReason);
        Controller->ClientReturnToMainMenuWithTextReason(
            FText::FromString(StableReason.Left(160)));
    }
    NetworkRemoteController.Reset();
    NetworkResumeCredential.Reset();
    NetworkReservedLastBatchId = 0;
    NetworkReservedDisconnectTick = 0;
    NetworkReservationExpiresAt = 0.0;
    bNetworkSeatReserved = false;
    bNetworkReservedMatchStarted = false;
    bNetworkResumeValidationPending = false;
    bNetworkResumeCompatibilityPending = false;
    GetWorldTimerManager().ClearTimer(NetworkReservationTimer);
    GetWorldTimerManager().ClearTimer(NetworkResumeValidationTimer);
    ClearNetworkAdmissionTimers();
    PresentHostReconnectGrace(false, 0.0f);
    UE_LOG(
        LogEchoes,
        Warning,
        TEXT("[ECHOES_NETWORK_SEAT_RELEASED] player=%u reason=%s aiControl=false hostStillWaiting=true"),
        UEchoesSimulationSubsystem::OpponentPlayerId,
        *StableReason.Left(96));
}

bool AEchoesGameMode::ForfeitNetworkOpponent(const FString& StableReason)
{
    if (bNetworkSessionFinished)
    {
        return false;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    FString Feedback;
    if (Bridge == nullptr ||
        !Bridge->ForfeitNetworkPlayer(
            UEchoesSimulationSubsystem::OpponentPlayerId, Feedback))
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NETWORK_FORFEIT_FAILED] reason=%s detail=%s"),
            *StableReason.Left(96),
            *Feedback);
        return false;
    }
    const bool bHasResultRecipient = NetworkRemoteController.IsValid();
    NotifyNetworkMatchFinished();
    if (AEchoesPlayerController* Host = FindLocalEchoesController(GetWorld()))
    {
        const echoes::sim::Simulation* Simulation = Bridge->GetSimulation();
        Host->NotifyNetworkOpponentForfeit(
            Simulation != nullptr ? Simulation->CurrentTick() : 0,
            StableReason,
            bHasResultRecipient);
    }
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_NETWORK_FORFEIT_PRESENTED] player=%u reason=%s resultRecipient=%s hostLeaveEnabled=%s"),
        UEchoesSimulationSubsystem::OpponentPlayerId,
        *StableReason.Left(96),
        bHasResultRecipient ? TEXT("true") : TEXT("false"),
        bHasResultRecipient ? TEXT("false") : TEXT("true"));
    return true;
}

bool AEchoesGameMode::IsNetworkSeatReservationAvailable() const
{
    return bNetworkSeatReserved && !NetworkResumeCredential.IsEmpty() &&
           FPlatformTime::Seconds() < NetworkReservationExpiresAt;
}

FString AEchoesGameMode::GenerateNetworkResumeCredential() const
{
    // 128-bit one-use resume credential drawn from the OS CSPRNG, formatted
    // as the same 32 uppercase hex digits the previous FGuid path produced.
    // A failed entropy read returns empty, which every consumer already
    // treats as "no resume capability": the seat-reservation predicate
    // requires a non-empty credential and the bounded-format checks reject
    // the empty string, so the failure mode is fail-closed rather than a
    // fallback to a weaker source.
    uint8 RandomBytes[16] = {};
    if (getentropy(RandomBytes, sizeof(RandomBytes)) != 0)
    {
        return FString();
    }
    return BytesToHex(RandomBytes, sizeof(RandomBytes));
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
    const bool bExpiredStartedMatch = bNetworkReservedMatchStarted;
    if (AEchoesPlayerController* PendingController =
            Cast<AEchoesPlayerController>(NetworkRemoteController.Get());
        PendingController != nullptr &&
        (bNetworkResumeValidationPending ||
         bNetworkResumeCompatibilityPending))
    {
        PendingController->RejectNetworkSessionFromServer(
            TEXT("NET_RECONNECT_GRACE_EXPIRED"));
        PendingController->ClientReturnToMainMenuWithTextReason(
            FText::FromString(TEXT("NET_RECONNECT_GRACE_EXPIRED")));
        NetworkRemoteController.Reset();
    }
    bNetworkSeatReserved = false;
    bNetworkResumeValidationPending = false;
    bNetworkResumeCompatibilityPending = false;
    NetworkResumeCredential.Reset();
    NetworkReservedLastBatchId = 0;
    NetworkReservedDisconnectTick = 0;
    NetworkReservationExpiresAt = 0.0;
    bNetworkReservedMatchStarted = false;
    GetWorldTimerManager().ClearTimer(NetworkReservationTimer);
    if (bExpiredStartedMatch)
    {
        (void)ForfeitNetworkOpponent(TEXT("NET_RECONNECT_GRACE_EXPIRED"));
        return;
    }
    PresentHostReconnectGrace(false, 0.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_NETWORK_SEAT_RELEASED] player=%u reason=resumeGraceExpired preMatch=true aiControl=false hostStillWaiting=true"),
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
    if (AEchoesPlayerController* Controller =
            Cast<AEchoesPlayerController>(NetworkRemoteController.Get()))
    {
        RejectNetworkResumeAttempt(
            Controller, TEXT("NET_RESUME_VALIDATION_TIMEOUT"));
        return;
    }
    NetworkRemoteController.Reset();
    bNetworkResumeValidationPending = false;
    bNetworkResumeCompatibilityPending = false;
    GetWorldTimerManager().ClearTimer(NetworkResumeValidationTimer);
}

void AEchoesGameMode::StartNetworkHelloTimeout(
    AEchoesPlayerController* Controller)
{
    if (Controller == nullptr || NetworkRemoteController.Get() != Controller)
    {
        return;
    }
    GetWorldTimerManager().ClearTimer(NetworkReadyTimer);
    GetWorldTimerManager().SetTimer(
        NetworkHelloTimer,
        this,
        &AEchoesGameMode::ExpireNetworkHello,
        NetworkHelloTimeoutSeconds,
        false);
}

void AEchoesGameMode::ExpireNetworkHello()
{
    AEchoesPlayerController* Controller =
        Cast<AEchoesPlayerController>(NetworkRemoteController.Get());
    if (Controller != nullptr)
    {
        ReleaseNetworkSeat(Controller, TEXT("NET_HELLO_TIMEOUT"), true);
    }
}

void AEchoesGameMode::ExpireNetworkReady()
{
    AEchoesPlayerController* Controller =
        Cast<AEchoesPlayerController>(NetworkRemoteController.Get());
    if (Controller != nullptr)
    {
        ReleaseNetworkSeat(Controller, TEXT("NET_READY_TIMEOUT"), true);
    }
}

void AEchoesGameMode::ClearNetworkAdmissionTimers()
{
    GetWorldTimerManager().ClearTimer(NetworkHelloTimer);
    GetWorldTimerManager().ClearTimer(NetworkReadyTimer);
}

void AEchoesGameMode::PresentHostReconnectGrace(
    bool bActive,
    float RemainingSeconds)
{
    if (AEchoesPlayerController* Host = FindLocalEchoesController(GetWorld()))
    {
        if (bActive)
        {
            Host->PresentNetworkReconnectGrace(RemainingSeconds);
        }
        else
        {
            Host->ClearNetworkReconnectGrace();
        }
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

    const bool bLegacyStressScenario =
        FParse::Param(FCommandLine::Get(), TEXT("EchoesStress400"));
#if UE_BUILD_SHIPPING
    const bool bSustainedStressScenario = false;
#else
    const bool bSustainedStressScenario =
        FParse::Param(
            FCommandLine::Get(),
            TEXT("EchoesStress400Sustained"));
#endif
    if (bLegacyStressScenario && bSustainedStressScenario)
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_STRESS_SUSTAINED_FAILED] code=CONFLICTING_FLAGS tick=0 detail=Choose exactly one stress fixture flag."));
        CleanupPrototypeEnvironment();
        return;
    }
    const bool bStressScenario =
        bLegacyStressScenario || bSustainedStressScenario;
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
        else if (RequestedFaction.Equals(
                     TEXT("Choir"),
                     ESearchCase::IgnoreCase) ||
                 RequestedFaction.Equals(
                     TEXT("HollowChoir"),
                     ESearchCase::IgnoreCase))
        {
            Requested = echoes::sim::Faction::HollowChoir;
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
        if (FApp::IsUnattended() ||
            FParse::Param(FCommandLine::Get(), TEXT("EchoesAutoStart")))
        {
            Bridge->SetScenarioPaused(false);
        }
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
    const bool bCampaignChoirAtLumeReach =
        !bStressScenario &&
        FParse::Param(
            FCommandLine::Get(),
            TEXT("EchoesCampaignChoirAtLumeReach"));
    const bool bCampaignNoNeutralLedger =
        !bStressScenario &&
        FParse::Param(
            FCommandLine::Get(),
            TEXT("EchoesCampaignNoNeutralLedger"));
    const bool bCampaignFutureThatWon =
        !bStressScenario &&
        FParse::Param(
            FCommandLine::Get(),
            TEXT("EchoesCampaignFutureThatWon"));
    const bool bCampaignAssemblyOfTheMissing =
        !bStressScenario &&
        FParse::Param(
            FCommandLine::Get(),
            TEXT("EchoesCampaignAssemblyOfTheMissing"));
    const bool bCampaignSeveralVoicesOneCommand =
        !bStressScenario &&
        FParse::Param(
            FCommandLine::Get(),
            TEXT("EchoesCampaignSeveralVoicesOneCommand"));
    const bool bCampaignTheBrokenSun =
        !bStressScenario &&
        FParse::Param(
            FCommandLine::Get(),
            TEXT("EchoesCampaignTheBrokenSun"));
    const int32 CampaignOperationCount =
        (bCampaignPrologue ? 1 : 0) +
        (bCampaignSevenAccounts ? 1 : 0) +
        (bCampaignCityReserve ? 1 : 0) +
        (bCampaignUnburiedRoad ? 1 : 0) +
        (bCampaignTermsOfContinuance ? 1 : 0) +
        (bCampaignNamesWithoutBirths ? 1 : 0) +
        (bCampaignShapeOfSilence ? 1 : 0) +
        (bCampaignShapeBesideUs ? 1 : 0) +
        (bCampaignReserveAuthority ? 1 : 0) +
        (bCampaignChoirAtLumeReach ? 1 : 0) +
        (bCampaignNoNeutralLedger ? 1 : 0) +
        (bCampaignFutureThatWon ? 1 : 0) +
        (bCampaignAssemblyOfTheMissing ? 1 : 0) +
        (bCampaignSeveralVoicesOneCommand ? 1 : 0) +
        (bCampaignTheBrokenSun ? 1 : 0);
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
            bCampaignTheBrokenSun
                ? EEchoesOperationMode::CampaignTheBrokenSun
            : bCampaignSeveralVoicesOneCommand
                ? EEchoesOperationMode::CampaignSeveralVoicesOneCommand
            : bCampaignAssemblyOfTheMissing
                ? EEchoesOperationMode::CampaignAssemblyOfTheMissing
            : bCampaignFutureThatWon
                ? EEchoesOperationMode::CampaignFutureThatWon
            : bCampaignNoNeutralLedger
                ? EEchoesOperationMode::CampaignNoNeutralLedger
            : bCampaignChoirAtLumeReach
                ? EEchoesOperationMode::CampaignChoirAtLumeReach
            : bCampaignReserveAuthority
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
                bCampaignTheBrokenSun
                    ? TEXT("TheBrokenSun")
                : bCampaignSeveralVoicesOneCommand
                    ? TEXT("SeveralVoicesOneCommand")
                : bCampaignAssemblyOfTheMissing
                    ? TEXT("AssemblyOfTheMissing")
                : bCampaignFutureThatWon
                    ? TEXT("TheFutureThatWon")
                : bCampaignNoNeutralLedger
                    ? TEXT("NoNeutralLedger")
                : bCampaignChoirAtLumeReach
                    ? TEXT("ChoirAtLumeReach")
                : bCampaignReserveAuthority
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
            bCampaignTheBrokenSun
                ? TEXT("TheBrokenSun")
            : bCampaignSeveralVoicesOneCommand
                ? TEXT("SeveralVoicesOneCommand")
            : bCampaignAssemblyOfTheMissing
                ? TEXT("AssemblyOfTheMissing")
            : bCampaignFutureThatWon
                ? TEXT("TheFutureThatWon")
            : bCampaignNoNeutralLedger
                ? TEXT("NoNeutralLedger")
            : bCampaignChoirAtLumeReach
                ? TEXT("ChoirAtLumeReach")
            : bCampaignReserveAuthority
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
    const bool bSimulationReady = bSustainedStressScenario
                                      ? Bridge->StartSustainedStressScenario()
                                  : bLegacyStressScenario
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
        const FEchoesSkirmishSetup OnlineSetup =
            FEchoesSkirmishSetupModel::CanonicalOnlineSetup();
        FString OnlineSetupFeedback;
        const bool bOnlineRulesReady =
            Bridge->GetOperationMode() == EEchoesOperationMode::Skirmish &&
            Bridge->ApplySkirmishSetup(
                OnlineSetup, OnlineSetupFeedback) &&
            FEchoesSkirmishSetupModel::IsCanonicalOnlineSetup(
                Bridge->GetActiveSkirmishSetup()) &&
            echoes::network::SupportsNetworkSession(
                Bridge->GetSimulation());
        if (!bOnlineRulesReady)
        {
            UE_LOG(
                LogEchoes,
                Error,
                TEXT("[ECHOES_ONLINE_FIXED_RULES_FAILED] detail=%s operation=%u"),
                *OnlineSetupFeedback,
                static_cast<uint8>(Bridge->GetOperationMode()));
            UEchoesGameInstance* EchoesGameInstance =
                Cast<UEchoesGameInstance>(GetGameInstance());
            if (EchoesGameInstance != nullptr)
            {
                EchoesGameInstance->ReportOnlineFailure(
                    TEXT("ONLINE_FIXED_RULES_UNAVAILABLE"));
            }
            Bridge->StopPrototypeScenario();
            CleanupPrototypeEnvironment();
            if (EchoesGameInstance != nullptr)
            {
                EchoesGameInstance->ReturnToFailedFrontDoor(
                    FindLocalEchoesController(GetWorld()));
            }
            return;
        }
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_ONLINE_FIXED_RULES_READY] map=GLASS_SCAR player0=MERIDIAN_COMPACT player1=KHARUUN_ASSEMBLIES resources=STANDARD protocol=fixed_1v1"));
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
            echoes::sim::Faction::HollowChoir,
            echoes::sim::Faction::MeridianCompact,
            echoes::sim::Faction::KharuunAssemblies,
            echoes::sim::Faction::HollowChoir,
            echoes::sim::Faction::MeridianCompact,
            echoes::sim::Faction::KharuunAssemblies,
            echoes::sim::Faction::HollowChoir,
        };
        const echoes::sim::EntityType Types[] = {
            echoes::sim::EntityType::Soldier,
            echoes::sim::EntityType::Soldier,
            echoes::sim::EntityType::Soldier,
            echoes::sim::EntityType::HeavyUnit,
            echoes::sim::EntityType::HeavyUnit,
            echoes::sim::EntityType::HeavyUnit,
            echoes::sim::EntityType::CommandCore,
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
            FIntPoint(9, 15),
            FIntPoint(12, 15),
            FIntPoint(15, 15),
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
            const FVector ChoirReviewLocation =
                Bridge->SimToWorld(echoes::sim::Vec2::FromTiles(32, 40));
            TWeakObjectPtr<UEchoesPresentationAudioSubsystem> WeakAudio(Audio);
            TSharedRef<bool> KharuunPlayedResult = MakeShared<bool>(false);
            FTimerHandle KharuunTimer;
            GetWorldTimerManager().SetTimer(
                KharuunTimer,
                FTimerDelegate::CreateWeakLambda(
                    this,
                    [WeakAudio, KharuunPlayedResult, KharuunReviewLocation]()
                    {
                        *KharuunPlayedResult = WeakAudio.IsValid() &&
                            WeakAudio->PlayDestruction(
                                echoes::sim::Faction::KharuunAssemblies,
                                KharuunReviewLocation);
                    }),
                0.16f,
                false);
            FTimerHandle ChoirTimer;
            GetWorldTimerManager().SetTimer(
                ChoirTimer,
                FTimerDelegate::CreateWeakLambda(
                    this,
                    [WeakAudio,
                     KharuunPlayedResult,
                     bCommandPlayed,
                     bMeridianPlayed,
                     bReducedDynamicRange,
                     ChoirReviewLocation]()
                    {
                        const bool bChoirPlayed = WeakAudio.IsValid() &&
                            WeakAudio->PlayDestruction(
                                echoes::sim::Faction::HollowChoir,
                                ChoirReviewLocation);
                        UE_LOG(
                            LogEchoes,
                            Display,
                            TEXT("[ECHOES_AUDIO_REVIEW_COMPLETE] revision=presentation-audio-v1 cuesPlayed=%d command2D=%s meridian3D=%s kharuun3D=%s choir3D=%s reducedDynamicRange=%s effectsVolume=1.00 rateLimited=true authoritative=false editorOnly=true finalAudio=false"),
                            (bCommandPlayed ? 1 : 0) +
                                (bMeridianPlayed ? 1 : 0) +
                                (*KharuunPlayedResult ? 1 : 0) +
                                (bChoirPlayed ? 1 : 0),
                            bCommandPlayed ? TEXT("true") : TEXT("false"),
                            bMeridianPlayed ? TEXT("true") : TEXT("false"),
                            *KharuunPlayedResult ? TEXT("true") : TEXT("false"),
                            bChoirPlayed ? TEXT("true") : TEXT("false"),
                            bReducedDynamicRange ? TEXT("true") : TEXT("false"));
                    }),
                0.32f,
                false);
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_AUDIO_REVIEW_READY] revision=presentation-audio-v1 cues=4 authored=%s sourceRate=48000 channels=1 reducedDynamicRange=%s effectsVolume=1.00 commandCooldownMs=80 destructionCooldownMs=140 authoritative=false editorOnly=true finalAudio=false"),
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
            GlassScarReviewMode.Equals(TEXT("FoldedVerge"), ESearchCase::IgnoreCase) ||
            GlassScarReviewMode.Equals(TEXT("BrokenSun"), ESearchCase::IgnoreCase) ||
            GlassScarReviewMode.Equals(TEXT("VerticalSlice"), ESearchCase::IgnoreCase);
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
                // The chasm layers are the ground of the composed frame; only
                // the per-tile silhouettes and dressing step aside for review.
                TerrainView->SetTileLayersVisible(false);
            }

            int32 PreviewEntityCount = 0;
            const auto SpawnPreview = [this, &PreviewEntityCount](
                                          uint32 Id,
                                          echoes::sim::EntityType Type,
                                          echoes::sim::Faction Faction,
                                          uint8 Owner,
                                          int32 TileX,
                                          int32 TileY,
                                          bool bDeployed = false,
                                          echoes::sim::Vec2 DeploymentFacing = echoes::sim::Vec2::FromTiles(1, 0),
                                          float ZOffset = 12.0f,
                                          TOptional<FVector> CustomWorldLocation = TOptional<FVector>(),
                                          float HeadingYaw = 0.0f)
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
                State.deployed = bDeployed;
                State.deploymentFacing = DeploymentFacing;
                if (Type == echoes::sim::EntityType::FutureWell)
                {
                    State.wellChoice = echoes::sim::FutureWellChoice::Preserve;
                }
                Preview->ApplyAuthoritativeState(State, true);
                if (Type != echoes::sim::EntityType::FutureWell)
                {
                    if (CustomWorldLocation.IsSet())
                    {
                        Preview->SetAuthoritativeWorldLocation(CustomWorldLocation.GetValue());
                    }
                    else
                    {
                        const UEchoesSimulationSubsystem* Subsystem =
                            GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>();
                        if (Subsystem != nullptr)
                        {
                            const FVector SimPos = Subsystem->SimToWorld(State.position);
                            Preview->SetAuthoritativeWorldLocation(SimPos + FVector(0.0f, 0.0f, ZOffset));
                        }
                    }
                    Preview->SetAuthoritativeHeadingYaw(HeadingYaw);
                }
                if (Preview->GetBodyMesh() != nullptr)
                {
                    Preview->GetBodyMesh()->SetVisibility(true, true);
                }
                Preview->SetActorHiddenInGame(false);
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
                    32,
                    false,
                    echoes::sim::Vec2::FromTiles(1, 0),
                    0.0f);
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
                        Tile.Y,
                        false,
                        echoes::sim::Vec2::FromTiles(1, 0),
                        0.0f);
                }
            }
            else if (GlassScarReviewMode.Equals(
                         TEXT("VerticalSlice"),
                         ESearchCase::IgnoreCase))
            {
                // Central Future Well on Buried Causeway circular dais
                SpawnPreview(
                    920000,
                    echoes::sim::EntityType::FutureWell,
                    echoes::sim::Faction::MeridianCompact,
                    echoes::sim::kNeutralPlayer,
                    32,
                    32,
                    false,
                    echoes::sim::Vec2::FromTiles(1, 0),
                    0.0f);

                // West Cliff (Screen Left): Meridian Compact strike force
                // Frontline Bulwarks deployed with hexagonal holographic energy shield barrier wings.
                // The shield wings sit on the mesh local +X; with the review camera at yaw 43 a
                // heading near 135 puts the wings edge-on to the lens. From the south bank the Well bears
                // ~112; headings near 150 split the difference and keep the cells three-quarter.
                // Bulwark 1: Holding the cliff rim overlooking the Future Well dais
                SpawnPreview(
                    920101,
                    echoes::sim::EntityType::HeavyUnit,
                    echoes::sim::Faction::MeridianCompact,
                    0,
                    34,
                    31,
                    true,
                    echoes::sim::Vec2::FromTiles(-1, 1),
                    12.0f,
                    FVector(330.0f, -880.0f, 12.0f),
                    150.0f);

                // Bulwark 2: Midground battle line anchor
                SpawnPreview(
                    920102,
                    echoes::sim::EntityType::HeavyUnit,
                    echoes::sim::Faction::MeridianCompact,
                    0,
                    35,
                    30,
                    true,
                    echoes::sim::Vec2::FromTiles(-1, 1),
                    12.0f,
                    FVector(620.0f, -1120.0f, 12.0f),
                    155.0f);

                // Bulwark 3: Outer flank defense
                SpawnPreview(
                    920103,
                    echoes::sim::EntityType::HeavyUnit,
                    echoes::sim::Faction::MeridianCompact,
                    0,
                    36,
                    30,
                    true,
                    echoes::sim::Vec2::FromTiles(-1, 1),
                    12.0f,
                    FVector(900.0f, -960.0f, 12.0f),
                    145.0f);

                // Lancers in advancing tactical wedge formation along the bridge ramp and cliff rim
                // Lancer 1: Foreground bridge approach
                SpawnPreview(
                    920105,
                    echoes::sim::EntityType::Soldier,
                    echoes::sim::Faction::MeridianCompact,
                    0,
                    33,
                    29,
                    false,
                    echoes::sim::Vec2::FromTiles(0, 1),
                    12.0f,
                    FVector(120.0f, -900.0f, 12.0f),
                    50.0f);

                // Lancer 2: Advancing along bridge side
                SpawnPreview(
                    920106,
                    echoes::sim::EntityType::Soldier,
                    echoes::sim::Faction::MeridianCompact,
                    0,
                    34,
                    30,
                    false,
                    echoes::sim::Vec2::FromTiles(0, 1),
                    12.0f,
                    FVector(170.0f, -720.0f, 12.0f),
                    45.0f);

                // Lancer 3: Second rank advancing
                SpawnPreview(
                    920107,
                    echoes::sim::EntityType::Soldier,
                    echoes::sim::Faction::MeridianCompact,
                    0,
                    33,
                    30,
                    false,
                    echoes::sim::Vec2::FromTiles(0, 1),
                    12.0f,
                    FVector(30.0f, -560.0f, 12.0f),
                    40.0f);

                // Lancer 4: Point scout approaching the dais
                SpawnPreview(
                    920108,
                    echoes::sim::EntityType::Soldier,
                    echoes::sim::Faction::MeridianCompact,
                    0,
                    34,
                    31,
                    false,
                    echoes::sim::Vec2::FromTiles(0, 1),
                    12.0f,
                    FVector(120.0f, -400.0f, 12.0f),
                    35.0f);

                // Lancer 5: Flank guard behind front ranks
                SpawnPreview(
                    920109,
                    echoes::sim::EntityType::Soldier,
                    echoes::sim::Faction::MeridianCompact,
                    0,
                    34,
                    30,
                    false,
                    echoes::sim::Vec2::FromTiles(0, 1),
                    12.0f,
                    FVector(-80.0f, -840.0f, 12.0f),
                    45.0f);

                // Surveyor engineering exoframe mechs in command positions behind the line
                // Surveyor 1: Standing tall behind the front Bulwark observing the Well
                SpawnPreview(
                    920110,
                    echoes::sim::EntityType::Worker,
                    echoes::sim::Faction::MeridianCompact,
                    0,
                    35,
                    31,
                    false,
                    echoes::sim::Vec2::FromTiles(-1, 1),
                    12.0f,
                    FVector(420.0f, -1300.0f, 12.0f),
                    95.0f);

                // Surveyor 2: On the upper shelf scanning the chasm
                SpawnPreview(
                    920111,
                    echoes::sim::EntityType::Worker,
                    echoes::sim::Faction::MeridianCompact,
                    0,
                    36,
                    31,
                    false,
                    echoes::sim::Vec2::FromTiles(-1, 1),
                    12.0f,
                    FVector(820.0f, -1360.0f, 12.0f),
                    110.0f);

                // Relay Skiff hovering above the strike force with its antenna halo and propulsion
                SpawnPreview(
                    920112,
                    echoes::sim::EntityType::ScoutUnit,
                    echoes::sim::Faction::MeridianCompact,
                    0,
                    35,
                    30,
                    false,
                    echoes::sim::Vec2::FromTiles(-1, 1),
                    150.0f,
                    FVector(560.0f, -560.0f, 170.0f),
                    120.0f);

                // Radiant cyan Matter crystals on the West cliff
                // Deposit 1: Foreground cliff outcrop framing the bridge entrance
                SpawnPreview(
                    920113,
                    echoes::sim::EntityType::ResourceNode,
                    echoes::sim::Faction::MeridianCompact,
                    echoes::sim::kNeutralPlayer,
                    33,
                    28,
                    false,
                    echoes::sim::Vec2::FromTiles(1, 0),
                    0.0f,
                    FVector(520.0f, -1380.0f, 0.0f),
                    25.0f);

                // Deposit 2: Outer shelf rim
                SpawnPreview(
                    920114,
                    echoes::sim::EntityType::ResourceNode,
                    echoes::sim::Faction::MeridianCompact,
                    echoes::sim::kNeutralPlayer,
                    37,
                    31,
                    false,
                    echoes::sim::Vec2::FromTiles(1, 0),
                    0.0f,
                    FVector(1100.0f, -1000.0f, 0.0f),
                    110.0f);

                // East Cliff (Screen Right): Kharuun Assemblies assault cluster
                // Cairnback heavy assault dreadnought holding the cliff rim facing the Future Well
                SpawnPreview(
                    920201,
                    echoes::sim::EntityType::HeavyUnit,
                    echoes::sim::Faction::KharuunAssemblies,
                    1,
                    30,
                    33,
                    true,
                    echoes::sim::Vec2::FromTiles(1, -1),
                    12.0f,
                    FVector(-560.0f, 900.0f, 12.0f),
                    -45.0f);

                // Riftstalkers predatory quadrupeds prowling the cliff edge
                // Riftstalker 1: Advanced point at the cliff rim in front of Cairnback
                SpawnPreview(
                    920202,
                    echoes::sim::EntityType::Soldier,
                    echoes::sim::Faction::KharuunAssemblies,
                    1,
                    30,
                    33,
                    true,
                    echoes::sim::Vec2::FromTiles(1, -1),
                    12.0f,
                    FVector(-380.0f, 880.0f, 12.0f),
                    -50.0f);

                // Riftstalker 2: Flanking position beside Cairnback
                SpawnPreview(
                    920203,
                    echoes::sim::EntityType::Soldier,
                    echoes::sim::Faction::KharuunAssemblies,
                    1,
                    29,
                    33,
                    true,
                    echoes::sim::Vec2::FromTiles(1, -1),
                    12.0f,
                    FVector(-820.0f, 900.0f, 12.0f),
                    -40.0f);

                // Resonant crystalline tripod scout standing proudly on the elevated rear shelf
                SpawnPreview(
                    920204,
                    echoes::sim::EntityType::ScoutUnit,
                    echoes::sim::Faction::KharuunAssemblies,
                    1,
                    29,
                    34,
                    true,
                    echoes::sim::Vec2::FromTiles(1, -1),
                    12.0f,
                    FVector(-1000.0f, 1100.0f, 12.0f),
                    -45.0f);

                // Tender worker unit supporting the cluster
                SpawnPreview(
                    920205,
                    echoes::sim::EntityType::Worker,
                    echoes::sim::Faction::KharuunAssemblies,
                    1,
                    29,
                    35,
                    true,
                    echoes::sim::Vec2::FromTiles(1, -1),
                    12.0f,
                    FVector(-720.0f, 1240.0f, 12.0f),
                    -45.0f);

                // Matter deposits on East cliff shelf
                // Deposit 1: Midground shelf behind Cairnback
                SpawnPreview(
                    920206,
                    echoes::sim::EntityType::ResourceNode,
                    echoes::sim::Faction::MeridianCompact,
                    echoes::sim::kNeutralPlayer,
                    28,
                    34,
                    false,
                    echoes::sim::Vec2::FromTiles(1, 0),
                    0.0f,
                    FVector(-960.0f, 940.0f, 0.0f),
                    45.0f);

                // Deposit 2: Rear shelf near Resonant
                SpawnPreview(
                    920207,
                    echoes::sim::EntityType::ResourceNode,
                    echoes::sim::Faction::MeridianCompact,
                    echoes::sim::kNeutralPlayer,
                    28,
                    35,
                    false,
                    echoes::sim::Vec2::FromTiles(1, 0),
                    0.0f,
                    FVector(-900.0f, 1240.0f, 0.0f),
                    -60.0f);

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
                 TEXT("EchoesGlassScarArtReview")) ||
             FParse::Param(
                 FCommandLine::Get(),
                 TEXT("EchoesArtReviewHideUI"))) &&
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
    UStaticMesh* BrokenSunSkyMesh = LoadObject<UStaticMesh>(
        nullptr,
        TEXT("/Game/Art/Generated/World/Environment/SM_World_BrokenSunSky.SM_World_BrokenSunSky"));
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
    EchoesBattlefieldPresentation::RegisterSharedActorTags(
        Floor->Tags,
        EchoesBattlefieldPresentation::FloorTag());
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
        FloorMaterial->SetScalarParameterValue(TEXT("Metallic"), 0.02f);
        FloorMaterial->SetScalarParameterValue(TEXT("Roughness"), 0.94f);
        FloorMaterial->SetScalarParameterValue(TEXT("EmissiveStrength"), 0.0f);
        FloorMesh->SetMaterial(0, FloorMaterial);
    }

    FString ReviewModeCheck;
    const bool bVerticalSliceMode =
        FParse::Value(FCommandLine::Get(), TEXT("EchoesGlassScarReview="), ReviewModeCheck) &&
        ReviewModeCheck.Equals(TEXT("VerticalSlice"), ESearchCase::IgnoreCase);
    if (bVerticalSliceMode)
    {
        Floor->SetActorHiddenInGame(true);
        FloorMesh->SetVisibility(false, true);
        FloorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    const auto SpawnScarAccent = [World, SurfaceMaterial, bVerticalSliceMode](
                                     UStaticMesh* MeshAsset,
                                     const FVector& Location,
                                     const FRotator& Rotation,
                                     const FVector& Scale,
                                     const FLinearColor& Color,
                                     const FName& DetailTag,
                                     bool bCastShadow,
                                     TOptional<FLinearColor> GlowOverride = TOptional<FLinearColor>())
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
        Accent->Tags.Add(
            EchoesBattlefieldPresentation::LegacyGlassScarTag());
        EchoesBattlefieldPresentation::RegisterPresetActorTags(
            Accent->Tags,
            EEchoesSkirmishMapPreset::GlassScar);
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
        if (bVerticalSliceMode && (DetailTag == TEXT("EchoesTerrainShelf") ||
                                   DetailTag == TEXT("EchoesRouteAshCut") ||
                                   DetailTag == TEXT("EchoesRouteFoldedVerge") ||
                                   DetailTag == TEXT("EchoesChasmRim") ||
                                   DetailTag == TEXT("EchoesScarBand") ||
                                   DetailTag == TEXT("EchoesGlassShard") ||
                                   DetailTag == TEXT("EchoesScarGlow")))
        {
            Accent->SetActorHiddenInGame(true);
            AccentMesh->SetVisibility(false, true);
        }
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
            GlowOverride.IsSet()
                ? GlowOverride.GetValue()
                : FLinearColor(
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

    // The four corner shelves of glass_scar_v5 are retired: the terrain view's bank
    // plates now carry the ground from rim to map edge.
    constexpr int32 SpawnedTerrainShelves = 0;

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
         FVector(0.0f, 0.0f, -59.0f), // deck top at z 0: units on the span stand on it
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

    // The chasm, banks, terrace, rim teeth, bed, and fissure light are composed by
    // AEchoesTerrainView from the live terrain rows (gate 50), for play and review alike.

    // Scar accents authored at ground level now lie on the chasm bed the terrain
    // view draws at -680: magenta fracture veins and glass along the bottom of the drop.
    constexpr float ChasmBedAccentZ = -690.0f;

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
                                Spec.Location + FVector(0.0f, 0.0f, ChasmBedAccentZ),
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
                                      10.0f + ChasmBedAccentZ),
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
        {FVector(-4800.0f, -35.0f, 50.0f), FLinearColor(0.72f, 0.06f, 0.22f),
         2200.0f, 1450.0f},
        {FVector(-2400.0f, 45.0f, 40.0f), FLinearColor(0.95f, 0.24f, 0.045f),
         1800.0f, 1350.0f},
        {FVector(0.0f, 0.0f, 50.0f), FLinearColor(0.76f, 0.08f, 0.30f),
         2600.0f, 1650.0f},
        {FVector(2400.0f, -45.0f, 40.0f), FLinearColor(0.95f, 0.24f, 0.045f),
         1800.0f, 1350.0f},
        {FVector(4800.0f, 35.0f, 50.0f), FLinearColor(0.72f, 0.06f, 0.22f),
         2200.0f, 1450.0f},
    };
    int32 SpawnedScarGlows = 0;
    for (const FScarGlowSpec& Spec : ScarGlows)
    {
        APointLight* Glow = World->SpawnActor<APointLight>(
            Spec.Location + FVector(0.0f, 0.0f, ChasmBedAccentZ + 80.0f),
            FRotator::ZeroRotator,
            SpawnParameters);
        if (Glow == nullptr)
        {
            continue;
        }
        Glow->Tags.Add(TEXT("EchoesPlaceholder"));
        Glow->Tags.Add(
            EchoesBattlefieldPresentation::LegacyGlassScarTag());
        EchoesBattlefieldPresentation::RegisterPresetActorTags(
            Glow->Tags,
            EEchoesSkirmishMapPreset::GlassScar);
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
    if (SpawnedRoutes != UE_ARRAY_COUNT(Routes) ||
        SpawnedScarBands != UE_ARRAY_COUNT(ScarBands) ||
        SpawnedGlassShards != UE_ARRAY_COUNT(GlassShards) ||
        SpawnedScarGlows != UE_ARRAY_COUNT(ScarGlows))
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_SCAR_COMPOSITION_FAILED] shelves=%d/0 routes=%d/%d bands=%d/%d shards=%d/%d glows=%d/%d"),
            SpawnedTerrainShelves,
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

    // The eight flat rim plates of glass_scar_v5 are retired: the terrain view's
    // bank faces and terrace are the cliff now.

    AEchoesWeatherView* Weather = World->SpawnActor<AEchoesWeatherView>(
        FVector::ZeroVector,
        FRotator::ZeroRotator,
        SpawnParameters);
    if (Weather == nullptr)
    {
        UE_LOG(LogEchoes, Error, TEXT("[ECHOES_WEATHER_SPAWN_FAILED]"));
        return false;
    }
    EchoesBattlefieldPresentation::RegisterSharedActorTags(
        Weather->Tags,
        EchoesBattlefieldPresentation::WeatherTag());

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
        EchoesBattlefieldPresentation::RegisterSharedActorTags(
            Sun->Tags,
            EchoesBattlefieldPresentation::SunTag());
    }
    if (Sky != nullptr)
    {
        Sky->Tags.Add(TEXT("EchoesPlaceholder"));
        EchoesBattlefieldPresentation::RegisterSharedActorTags(
            Sky->Tags,
            EchoesBattlefieldPresentation::SkyTag());
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
    // Glass Scar rig: the fractured sun's gold key against a cool indigo
    // ambient, per the authored Crownfall-sky direction (A1).
    // Frame hierarchy: a gentler warm key keeps actor faces warm while the
    // stronger cool fill lets the dark terrain recede — layer separation by
    // temperature as well as value.
    SunComponent->SetIntensity(bVerticalSliceMode ? 14.0f : 10.0f);
    SunComponent->SetLightColor(bVerticalSliceMode ? FLinearColor(1.0f, 0.82f, 0.52f) : FLinearColor(1.0f, 0.95f, 0.86f));
    if (bVerticalSliceMode)
    {
        Sun->SetActorRotation(FRotator(-24.0f, -137.0f, 0.0f));
    }
    Sky->GetLightComponent()->SetIntensity(bVerticalSliceMode ? 1.6f : 1.6f);
    Sky->GetLightComponent()->SetLightColor(bVerticalSliceMode ? FLinearColor(0.12f, 0.22f, 0.52f) : FLinearColor(0.42f, 0.55f, 0.82f));

    if (BrokenSunSkyMesh != nullptr)
    {
        FActorSpawnParameters SkyActorSpawnParameters;
        SkyActorSpawnParameters.SpawnCollisionHandlingOverride =
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        const FVector SunLocation = bVerticalSliceMode
            ? FVector(11700.0f, 10900.0f, 215.0f) // 16 km out on the review axis: the sphere fills the sky band above the far bank
            : FVector(-14000.0f, 9800.0f, 24000.0f);
        const FRotator SunRotation = bVerticalSliceMode
            ? FRotator(-16.0f, -137.0f, 0.0f)
            : FRotator(-55.0f, -35.0f, 0.0f);
        const FVector SunScale = bVerticalSliceMode
            ? FVector(0.85f, 0.85f, 0.85f) // a full sphere now; 0.85 keeps ~10 deg of frame
            : FVector(6.0f, 6.0f, 6.0f);

        AStaticMeshActor* BrokenSunSky = World->SpawnActor<AStaticMeshActor>(
            SunLocation,
            SunRotation,
            SkyActorSpawnParameters);
        if (BrokenSunSky != nullptr)
        {
            BrokenSunSky->Tags.Add(TEXT("EchoesPlaceholder"));
            BrokenSunSky->Tags.Add(
                EchoesBattlefieldPresentation::LegacyGlassScarTag());
            EchoesBattlefieldPresentation::RegisterPresetActorTags(
                BrokenSunSky->Tags,
                EEchoesSkirmishMapPreset::GlassScar);
            BrokenSunSky->Tags.Add(TEXT("EchoesBrokenSunSky"));
            UStaticMeshComponent* BrokenSunSkyComp =
                BrokenSunSky->GetStaticMeshComponent();
            BrokenSunSkyComp->SetMobility(EComponentMobility::Movable);
            BrokenSunSkyComp->SetStaticMesh(BrokenSunSkyMesh);
            BrokenSunSkyComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            BrokenSunSkyComp->SetCanEverAffectNavigation(false);
            BrokenSunSkyComp->SetGenerateOverlapEvents(false);
            BrokenSunSkyComp->SetCastShadow(false);
            BrokenSunSkyComp->SetReceivesDecals(false);
            BrokenSunSky->SetActorScale3D(SunScale);

            const FLinearColor SunColors[4] = {
                FLinearColor(0.015f, 0.018f, 0.045f),
                FLinearColor(0.060f, 0.048f, 0.052f),
                FLinearColor(0.92f, 0.42f, 0.09f),
                FLinearColor(1.0f, 0.56f, 0.11f)
            };
            const float SunMetallic[4] = {0.0f, 0.05f, 0.10f, 0.0f};
            const float SunRoughness[4] = {0.95f, 0.88f, 0.35f, 0.20f};
            // Core emissive held below the clip point so the sphere reads gold
            // through its cracks instead of a white disc (A1: no clipped highlights).
            const float SunEmissive[4] = {0.0f, 0.0f, 1.5f, 2.4f};

            for (int32 Slot = 0; Slot < 4; ++Slot)
            {
                UMaterialInstanceDynamic* SunMat =
                    UMaterialInstanceDynamic::Create(SurfaceMaterial, BrokenSunSky);
                if (SunMat != nullptr)
                {
                    SunMat->SetVectorParameterValue(
                        EnvironmentColorParameterName, SunColors[Slot]);
                    SunMat->SetScalarParameterValue(
                        EnvironmentMetallicParameterName, SunMetallic[Slot]);
                    SunMat->SetScalarParameterValue(
                        EnvironmentRoughnessParameterName, SunRoughness[Slot]);
                    SunMat->SetScalarParameterValue(
                        EnvironmentEmissiveParameterName, SunEmissive[Slot]);
                    BrokenSunSkyComp->SetMaterial(Slot, SunMat);
                }
            }
        }

        if (bVerticalSliceMode)
        {
            // Upward warm golden aperture illumination inside the Future Well dais
            APointLight* DaisLight = World->SpawnActor<APointLight>(
                FVector(0.0f, 0.0f, 35.0f),
                FRotator::ZeroRotator,
                SkyActorSpawnParameters);
            if (DaisLight != nullptr && DaisLight->PointLightComponent != nullptr)
            {
                DaisLight->Tags.Add(TEXT("EchoesPlaceholder"));
                DaisLight->Tags.Add(EchoesBattlefieldPresentation::LegacyGlassScarTag());
                DaisLight->PointLightComponent->SetLightColor(FLinearColor(1.0f, 0.70f, 0.20f));
                DaisLight->PointLightComponent->SetIntensity(1600.0f);
                DaisLight->PointLightComponent->SetAttenuationRadius(750.0f);
                DaisLight->PointLightComponent->SetSourceRadius(100.0f);
                DaisLight->PointLightComponent->SetCastShadows(false);
            }

            // Molten amber fissure illumination ascending from the deep chasm abyss
            const struct FChasmLightSpec { FVector Location; float Intensity; float Radius; } ChasmSpecs[] = {
                { FVector(0.0f, -400.0f, -250.0f), 4800.0f, 1600.0f },
                { FVector(0.0f, 400.0f, -250.0f), 4800.0f, 1600.0f },
                { FVector(-600.0f, 0.0f, -320.0f), 3800.0f, 1500.0f },
                { FVector(600.0f, 0.0f, -320.0f), 3800.0f, 1500.0f },
                { FVector(0.0f, 0.0f, -450.0f), 5200.0f, 1800.0f },
            };
            for (const auto& Spec : ChasmSpecs)
            {
                APointLight* FissureLight = World->SpawnActor<APointLight>(
                    Spec.Location,
                    FRotator::ZeroRotator,
                    SkyActorSpawnParameters);
                if (FissureLight != nullptr && FissureLight->PointLightComponent != nullptr)
                {
                    FissureLight->Tags.Add(TEXT("EchoesPlaceholder"));
                    FissureLight->Tags.Add(EchoesBattlefieldPresentation::LegacyGlassScarTag());
                    FissureLight->PointLightComponent->SetLightColor(FLinearColor(1.0f, 0.38f, 0.05f));
                    FissureLight->PointLightComponent->SetIntensity(Spec.Intensity);
                    FissureLight->PointLightComponent->SetAttenuationRadius(Spec.Radius);
                    FissureLight->PointLightComponent->SetCastShadows(false);
                }
            }

            // Strike force tactical accent lights highlighting unit silhouettes against deep indigo twilight
            // 1. Meridian Strike Force (West Cliff / Screen Left): cool cyan fill & rim highlight
            APointLight* MeridianAccent = World->SpawnActor<APointLight>(
                FVector(550.0f, -1000.0f, 220.0f),
                FRotator::ZeroRotator,
                SkyActorSpawnParameters);
            if (MeridianAccent != nullptr && MeridianAccent->PointLightComponent != nullptr)
            {
                MeridianAccent->Tags.Add(TEXT("EchoesPlaceholder"));
                MeridianAccent->Tags.Add(EchoesBattlefieldPresentation::LegacyGlassScarTag());
                MeridianAccent->PointLightComponent->SetLightColor(FLinearColor(0.25f, 0.65f, 1.0f));
                MeridianAccent->PointLightComponent->SetIntensity(3500.0f);
                MeridianAccent->PointLightComponent->SetAttenuationRadius(1400.0f);
                MeridianAccent->PointLightComponent->SetCastShadows(false);
            }

            // 2. Kharuun Assault Cluster (East Cliff / Screen Right): warm bronze/amber fill & rim highlight
            APointLight* KharuunAccent = World->SpawnActor<APointLight>(
                FVector(-620.0f, 980.0f, 220.0f),
                FRotator::ZeroRotator,
                SkyActorSpawnParameters);
            if (KharuunAccent != nullptr && KharuunAccent->PointLightComponent != nullptr)
            {
                KharuunAccent->Tags.Add(TEXT("EchoesPlaceholder"));
                KharuunAccent->Tags.Add(EchoesBattlefieldPresentation::LegacyGlassScarTag());
                KharuunAccent->PointLightComponent->SetLightColor(FLinearColor(1.0f, 0.60f, 0.22f));
                KharuunAccent->PointLightComponent->SetIntensity(3500.0f);
                KharuunAccent->PointLightComponent->SetAttenuationRadius(1400.0f);
                KharuunAccent->PointLightComponent->SetCastShadows(false);
            }

            // 3. Foreground Right Cliff Shelf: subtle cyan/amber edge accent
            APointLight* ForegroundAccent = World->SpawnActor<APointLight>(
                FVector(-640.0f, -1000.0f, 120.0f),
                FRotator::ZeroRotator,
                SkyActorSpawnParameters);
            if (ForegroundAccent != nullptr && ForegroundAccent->PointLightComponent != nullptr)
            {
                ForegroundAccent->Tags.Add(TEXT("EchoesPlaceholder"));
                ForegroundAccent->Tags.Add(EchoesBattlefieldPresentation::LegacyGlassScarTag());
                ForegroundAccent->PointLightComponent->SetLightColor(FLinearColor(0.35f, 0.75f, 1.0f));
                ForegroundAccent->PointLightComponent->SetIntensity(2200.0f);
                ForegroundAccent->PointLightComponent->SetAttenuationRadius(850.0f);
                ForegroundAccent->PointLightComponent->SetCastShadows(false);
            }
        }
    }

    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_ENV_READY] terrainComposition=glass_scar_v6 authoredAssets=7 shelves=0 chasm=terrain-view routes=3 ashCutRouteKit=production_v1 ashCutUVs=2 ashCutMaterials=4 ashCutRuntimeCollision=false buriedCausewayRouteKit=production_v1 buriedCausewayUVs=2 buriedCausewayMaterials=4 buriedCausewayRuntimeCollision=false foldedVergeRouteKit=production_v1 foldedVergeUVs=2 foldedVergeMaterials=4 foldedVergeRuntimeCollision=false bands=7 shards=12 glows=5 collisionAuthority=false routeAuthority=false finalArt=false"));
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
