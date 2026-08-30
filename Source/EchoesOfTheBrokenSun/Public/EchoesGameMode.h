#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "EchoesGameMode.generated.h"

/** Boots the code-only battlefield so no project Content assets are required. */
UCLASS(NotBlueprintable)
class ECHOESOFTHEBROKENSUN_API AEchoesGameMode final : public AGameModeBase
{
    GENERATED_BODY()

public:
    AEchoesGameMode();

    virtual void BeginPlay() override;

#if WITH_DEV_AUTOMATION_TESTS
    bool SpawnPrototypeEnvironmentForTesting()
    {
        return SpawnPrototypeEnvironment();
    }
#endif

private:
    bool SpawnPrototypeEnvironment();
    void CleanupPrototypeEnvironment();
};
