#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineBaseTypes.h"
#include "Engine/GameInstance.h"
#include "EchoesGameInstance.generated.h"

class APlayerController;
class UNetDriver;

UENUM()
enum class EEchoesOnlineFrontDoorState : uint8
{
    Idle,
    JoinSetup,
    Hosting,
    Connecting,
    ClientLobby,
    Failed
};

/** Travel-stable state and failure handling for fixed-rules direct 1v1 play. */
UCLASS(NotBlueprintable)
class ECHOESOFTHEBROKENSUN_API UEchoesGameInstance final
    : public UGameInstance
{
    GENERATED_BODY()

public:
    virtual void Init() override;
    virtual void Shutdown() override;

    [[nodiscard]] EEchoesOnlineFrontDoorState GetOnlineState() const
    {
        return OnlineState;
    }
    [[nodiscard]] const FString& GetDirectConnectEndpoint() const
    {
        return DirectConnectEndpoint;
    }
    [[nodiscard]] const FString& GetOnlineFailureMessage() const
    {
        return OnlineFailureMessage;
    }
    [[nodiscard]] const FString& GetHostShareEndpoint() const
    {
        return HostShareEndpoint;
    }
    [[nodiscard]] int32 GetOnlineFocusIndex() const
    {
        return OnlineFocusIndex;
    }
    [[nodiscard]] bool IsPlayerInitiatedOnlineSession() const
    {
        return bPlayerInitiatedOnlineSession;
    }
    [[nodiscard]] bool HasUsableReconnectContext() const;
    [[nodiscard]] int32 GetReconnectSecondsRemaining() const;
    [[nodiscard]] bool IsReconnectAttemptPending() const
    {
        return bReconnectAttemptPending;
    }

    void OpenOnlineFrontDoor();
    void CloseOnlineFrontDoor();
    void RetryOnlineFrontDoor(APlayerController* Controller);
    void FocusPreviousOnlineAction();
    void FocusNextOnlineAction();
    void FocusOnlineAction(int32 FocusIndex);
    bool AppendEndpointCharacter(TCHAR Character);
    bool BackspaceEndpointCharacter();
    void SetDirectConnectEndpoint(const FString& Endpoint);

    bool RequestFixedRulesHost(UWorld* World);
    bool RequestDirectJoin(APlayerController* Controller);
    bool RequestReconnect(APlayerController* Controller);
    void CancelOnlineRequest(APlayerController* Controller);
    void ReturnToOnlineFrontDoor(APlayerController* Controller);
    void ReturnToFailedFrontDoor(APlayerController* Controller);
    void NotifyControllerReady(APlayerController* Controller);
    void MarkClientLobby();
    void MarkNetworkMatchStarted();
    void MarkNetworkMatchResultReceived();
    void StoreNetworkResumeCredential(
        const FString& Credential,
        float GraceSeconds);
    [[nodiscard]] bool TryGetPendingReconnectCredential(
        FString& OutCredential) const;
    void MarkReconnectAttemptAccepted();
    void ClearReconnectContext(const TCHAR* StableReason);
    void ReportOnlineFailure(
        const FString& StableReason,
        bool bPreserveReconnect = false);

    [[nodiscard]] static bool NormalizeDirectEndpoint(
        const FString& Candidate,
        FString& OutNormalized,
        FString& OutError);

private:
#if WITH_DEV_AUTOMATION_TESTS
    friend class FEchoesOnlineFrontDoorTest;
#endif

    void HandleNetworkFailure(
        UWorld* World,
        UNetDriver* NetDriver,
        ENetworkFailure::Type FailureType,
        const FString& ErrorString);
    void HandleTravelFailure(
        UWorld* World,
        ETravelFailure::Type FailureType,
        const FString& ErrorString);
    [[nodiscard]] static FString PlayerFacingFailure(
        const FString& StableReason);
    [[nodiscard]] static bool HasExplicitDevelopmentLoopbackBind(
        const TCHAR* CommandLine);
    void ResolveHostShareEndpoint();
    [[nodiscard]] bool HasStoredReconnectCredential() const;
    bool ArmReconnectWindow();
    void QueueEntryTravelRetainingState();
    void TravelToEntryRetainingState();
    [[nodiscard]] static bool IsStandaloneEntryWorld(const UWorld* World);

    static constexpr int32 OnlineActionCount = 4;
    EEchoesOnlineFrontDoorState OnlineState =
        EEchoesOnlineFrontDoorState::Idle;
    FString DirectConnectEndpoint = TEXT("127.0.0.1:7777");
    FString HostShareEndpoint;
    FString BoundReconnectEndpoint;
    FString NetworkResumeCredential;
    FString OnlineFailureMessage;
    double NetworkResumeExpiresAtSeconds = 0.0;
    float NetworkResumeGraceSeconds = 0.0f;
    int32 OnlineFocusIndex = 0;
    bool bPlayerInitiatedOnlineSession = false;
    bool bIntentionalReturnPending = false;
    bool bReconnectAttemptPending = false;
    bool bReconnectWindowArmed = false;
    bool bEntryTravelQueued = false;
    bool bCompletedOnlineResult = false;
};
