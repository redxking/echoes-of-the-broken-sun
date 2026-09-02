#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "EchoesGameMode.generated.h"

class AEchoesPlayerController;

/** Boots the code-only battlefield so no project Content assets are required. */
UCLASS(NotBlueprintable)
class ECHOESOFTHEBROKENSUN_API AEchoesGameMode final : public AGameModeBase
{
    GENERATED_BODY()

public:
    AEchoesGameMode();

    virtual void BeginPlay() override;
    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void Logout(AController* Exiting) override;
    bool TryResumeNetworkPlayer(
        AEchoesPlayerController* Controller,
        const FString& Credential,
        FString& OutError);
    [[nodiscard]] bool IsBoundNetworkController(
        const AEchoesPlayerController* Controller) const;
    [[nodiscard]] bool IsAwaitingNetworkResumeCredential(
        const AEchoesPlayerController* Controller) const;
    bool NotifyNetworkCompatibilityAccepted(
        AEchoesPlayerController* Controller);
    void NotifyNetworkPlayerReady(AEchoesPlayerController* Controller);
    void RejectNetworkResumeAttempt(
        AEchoesPlayerController* Controller,
        const FString& StableReason);
    void NotifyNetworkMatchFinished();
    bool SurrenderNetworkHost(const FString& StableReason);
    void ReleaseNetworkSeat(
        AEchoesPlayerController* Controller,
        const FString& StableReason,
        bool bNotifyClient);
    bool ForfeitNetworkOpponent(const FString& StableReason);

#if WITH_DEV_AUTOMATION_TESTS
    bool SpawnPrototypeEnvironmentForTesting()
    {
        return SpawnPrototypeEnvironment();
    }

    [[nodiscard]] FString GenerateNetworkResumeCredentialForTesting() const
    {
        return GenerateNetworkResumeCredential();
    }
#endif

private:
    static constexpr float NetworkResumeGraceSeconds = 120.0f;
    static constexpr float NetworkHelloTimeoutSeconds = 12.0f;
    static constexpr float NetworkReadyTimeoutSeconds = 45.0f;

    bool SpawnPrototypeEnvironment();
    void CleanupPrototypeEnvironment();
    void ExpireNetworkSeatReservation();
    void ExpireNetworkResumeValidation();
    void ExpireNetworkHello();
    void ExpireNetworkReady();
    void StartNetworkHelloTimeout(AEchoesPlayerController* Controller);
    void ClearNetworkAdmissionTimers();
    void PresentHostReconnectGrace(bool bActive, float RemainingSeconds);
    [[nodiscard]] bool IsNetworkSeatReservationAvailable() const;
    [[nodiscard]] FString GenerateNetworkResumeCredential() const;
    TWeakObjectPtr<APlayerController> NetworkRemoteController;
    FString NetworkResumeCredential;
    uint64 NetworkReservedLastBatchId = 0;
    uint64 NetworkReservedDisconnectTick = 0;
    double NetworkReservationExpiresAt = 0.0;
    bool bNetworkSeatReserved = false;
    bool bNetworkReservedMatchStarted = false;
    bool bNetworkResumeValidationPending = false;
    bool bNetworkResumeCompatibilityPending = false;
    bool bNetworkSessionFinished = false;
    FTimerHandle NetworkReservationTimer;
    FTimerHandle NetworkResumeValidationTimer;
    FTimerHandle NetworkHelloTimer;
    FTimerHandle NetworkReadyTimer;
};
