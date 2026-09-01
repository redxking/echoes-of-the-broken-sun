#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "EchoesHUD.generated.h"

class AEchoesPlayerController;
class UEchoesGameUserSettings;
class UEchoesSimulationSubsystem;
struct FEchoesVisualTheme;
enum class EEchoesVisualFaction : uint8;
namespace echoes::sim
{
class PlayerView;
}

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
    void DrawOnlineTitleEntry(const UEchoesGameUserSettings* Settings);
    void DrawOnlineFrontDoor(
        const AEchoesPlayerController* EchoesController,
        const UEchoesGameUserSettings* Settings);
    void DrawOnlineLocalMenu(
        const AEchoesPlayerController* EchoesController,
        const UEchoesGameUserSettings* Settings);
    void DrawNetworkReconnectBanner(
        const AEchoesPlayerController* EchoesController,
        const UEchoesGameUserSettings* Settings);
    void DrawSkirmishSetup(
        const AEchoesPlayerController* EchoesController,
        const UEchoesGameUserSettings* Settings);
    void DrawSkirmishDeploymentSummary(
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
    void DrawTechnologyPanel(
        const AEchoesPlayerController* EchoesController,
        const UEchoesSimulationSubsystem* Bridge,
        const UEchoesGameUserSettings* Settings);
    void DrawMatchResult(
        const AEchoesPlayerController* EchoesController,
        const UEchoesSimulationSubsystem* Bridge,
        const UEchoesGameUserSettings* Settings);
    void DrawTacticalMinimap(
        const UEchoesSimulationSubsystem* Bridge,
        const AEchoesPlayerController* EchoesController,
        const UEchoesGameUserSettings* Settings,
        const echoes::sim::PlayerView* PlayerView);
    void DrawVibrationSignatures(
        const UEchoesSimulationSubsystem* Bridge,
        const UEchoesGameUserSettings* Settings,
        const echoes::sim::PlayerView* PlayerView);
    void DrawCommandDeck(
        const AEchoesPlayerController* EchoesController,
        const UEchoesSimulationSubsystem* Bridge,
        const UEchoesGameUserSettings* Settings);
    void DrawVisualPanel(
        const FBox2D& Bounds,
        const FEchoesVisualTheme& Theme,
        bool bEmphasized = true);
    void DrawShatteredSunMotif(
        const FBox2D& Bounds,
        const FEchoesVisualTheme& Theme,
        const UEchoesGameUserSettings* Settings,
        float Opacity);
    void DrawFactionSigil(
        EEchoesVisualFaction Faction,
        const FVector2D& Center,
        float Radius,
        const FEchoesVisualTheme& Theme,
        float Opacity = 1.0f);
    void DrawSelectionRectangle();

    bool bLoggedTacticalOverviewReady = false;
    bool bLoggedObjectiveTrackerReady = false;
    bool bLoggedVibrationPresentationReady = false;
    bool bLoggedCommandDeckReady = false;
};
