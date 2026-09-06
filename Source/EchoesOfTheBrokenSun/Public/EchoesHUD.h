#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "EchoesHUD.generated.h"

/** Stable game-mode HUD actor identity. Player UI is owned by UMG/Slate widgets. */
UCLASS(NotBlueprintable)
class ECHOESOFTHEBROKENSUN_API AEchoesHUD final : public AHUD
{
    GENERATED_BODY()
};
