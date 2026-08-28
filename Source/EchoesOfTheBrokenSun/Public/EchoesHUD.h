#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "EchoesHUD.generated.h"

/** Minimal code-only HUD for prototype state, controls, and order feedback. */
UCLASS(NotBlueprintable)
class ECHOESOFTHEBROKENSUN_API AEchoesHUD final : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;

private:
    void DrawSelectionRectangle();
};
