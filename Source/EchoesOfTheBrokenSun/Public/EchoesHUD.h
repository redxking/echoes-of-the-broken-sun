#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "EchoesHUD.generated.h"

class AEchoesPlayerController;
class UEchoesGameUserSettings;
class UEchoesSimulationSubsystem;

/** Code-only tactical HUD for state, controls, feedback, and battlefield overview. */
UCLASS(NotBlueprintable)
class ECHOESOFTHEBROKENSUN_API AEchoesHUD final : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;

private:
    void DrawTitleScreen(
        const AEchoesPlayerController* EchoesController,
        const UEchoesGameUserSettings* Settings);
    void DrawMissionBriefing(
        const AEchoesPlayerController* EchoesController,
        const UEchoesGameUserSettings* Settings);
    void DrawObjectiveTracker(
        const UEchoesSimulationSubsystem* Bridge,
        const UEchoesGameUserSettings* Settings);
    void DrawPauseMenu(
        const AEchoesPlayerController* EchoesController,
        const UEchoesGameUserSettings* Settings);
    void DrawMatchResult(
        const AEchoesPlayerController* EchoesController,
        const UEchoesSimulationSubsystem* Bridge,
        const UEchoesGameUserSettings* Settings);
    void DrawTacticalMinimap(
        const UEchoesSimulationSubsystem* Bridge,
        const AEchoesPlayerController* EchoesController,
        const UEchoesGameUserSettings* Settings);
    void DrawSelectionRectangle();

    bool bLoggedTacticalOverviewReady = false;
    bool bLoggedObjectiveTrackerReady = false;
};
