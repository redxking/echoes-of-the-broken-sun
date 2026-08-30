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

#if WITH_DEV_AUTOMATION_TESTS
    bool SpawnPrototypeEnvironmentForTesting()
    {
        return SpawnPrototypeEnvironment();
    }
#endif

private:
    static constexpr float NetworkResumeGraceSeconds = 120.0f;

    bool SpawnPrototypeEnvironment();
    void CleanupPrototypeEnvironment();
    void ExpireNetworkSeatReservation();
    void ExpireNetworkResumeValidation();
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
    FTimerHandle NetworkReservationTimer;
    FTimerHandle NetworkResumeValidationTimer;
};
