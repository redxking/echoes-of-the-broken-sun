#pragma once

#include "CoreMinimal.h"
#include "Async/Future.h"
#include <atomic>
#include <memory>
#include "EchoesCampaignProgress.h"
#include "EchoesPlayerFlow.h"
#include "EchoesPlayerProfile.h"
#include "EchoesTutorialConstructionObservation.h"
#include "EchoesTutorialOrderObservation.h"
#include "EchoesTutorialSelectionObservation.h"
#include "EchoesTutorialSurveyObservation.h"
#include "EchoesMatchReplay.h"
#include "EchoesCampaignMapLayout.h"
#include "EchoesCommandDeckModel.h"
#include "EchoesFormationLayout.h"
#include "EchoesFeedbackHistoryModel.h"
#include "EchoesFieldHudView.h"
#include "EchoesInputBindingModel.h"
#include "EchoesNetworkSession.h"
#include "EchoesNetworkSnapshotFlow.h"
#include "EchoesGameplayFeedbackPacket.h"
#include "EchoesPrologueMissionModel.h"
#include "EchoesSkirmishSetup.h"
#include "EchoesTitleOverlayLayout.h"
#include "GameFramework/PlayerController.h"
#include "EchoesSimCore/NetworkProtocol.h"
#include "EchoesSimCore/Simulation.h"
#include "EchoesPlayerController.generated.h"

class AEchoesEntityView;
class AEchoesFogView;
class AEchoesTerrainView;
class AStaticMeshActor;
class ADirectionalLight;
class ASkyLight;
enum class EEchoesCommandMarkerType : uint8;
enum class EEchoesCityDistrict : uint8;
class UEchoesSimulationSubsystem;
class UEchoesGameInstance;
class UEchoesShellWidget;
class AEchoesBuildPlacementPreview;
class UEchoesContextCursorWidget;
class UEchoesFieldHudWidget;
class AEchoesPowerNetworkView;
struct FKey;

/** Value-only result of a background replay directory scan. */
struct FEchoesReplayBrowserScanResult
{
    uint64 Generation = 0;
    TArray<FEchoesReplayMetadata> Entries;
    TArray<FString> Errors;
};

#if UE_BUILD_DEVELOPMENT && WITH_DEV_AUTOMATION_TESTS
namespace echoes::network::testing
{
[[nodiscard]] bool ValidateDevelopmentCredentialStagingFile(
    const FString& Candidate,
    FString& OutNormalized,
    FString& OutReason);
[[nodiscard]] bool StageDevelopmentResumeCredential(
    const FString& NormalizedPath,
    const FString& Credential,
    FString& OutReason);
[[nodiscard]] bool ConsumeDevelopmentResumeCredential(
    const FString& Candidate,
    FString& OutCredential,
    FString& OutReason);
}
#endif

/** RTS selection, faction choice, and context-order input for the local player. */
UCLASS(NotBlueprintable)
class ECHOESOFTHEBROKENSUN_API AEchoesPlayerController final
    : public APlayerController
{
    GENERATED_BODY()

public:
    AEchoesPlayerController();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void PlayerTick(float DeltaTime) override;
    [[nodiscard]] FText GetTutorialInstruction() const { return TutorialInstruction; }
    [[nodiscard]] bool IsTutorialOperationAuthorized() const { return bTutorialOperationAuthorized; }
    void SetTutorialOperationAuthorized(bool bAuthorized) { bTutorialOperationAuthorized = bAuthorized; }
    [[nodiscard]] bool IsTutorialSkipModalVisible() const { return TutorialSkipModal.bVisible; }
    [[nodiscard]] uint16 GetTutorialSkippedMask() const { return TutorialSkippedMask; }
    // Guidance progress may include deliberate skips and later genuine actions.
    // Only TutorialVerifiedMask is durable mastery and can qualify readiness.
    [[nodiscard]] uint16 GetTutorialProgressMask() const
    {
        return PlayerProfile.TutorialVerifiedMask | TutorialSkippedMask | TutorialSessionVerifiedMask;
    }
    [[nodiscard]] const FEchoesTutorialSurveyObservation& GetTutorialSurvey() const { return TutorialSurvey; }
    [[nodiscard]] const FEchoesTutorialSelectionObservation& GetTutorialSelection() const { return TutorialSelection; }
    [[nodiscard]] const FEchoesTutorialOrderObservation& GetTutorialOrders() const { return TutorialOrders; }
    [[nodiscard]] bool IsTutorialCoreSelected() const { return bTutorialCoreSelected; }
    [[nodiscard]] uint16 GetTutorialPresentedLessonBit() const { return TutorialPresentedLessonBit; }
    void OpenTutorialSkipModal();
    void CloseTutorialSkipModal(bool bRestorePause);
    void SkipTutorialCurrentStep();
    void EndAllTutorials();
    void CancelTutorialSkipModal();
    virtual void SetupInputComponent() override;
    virtual bool InputKey(const FInputKeyEventArgs& Params) override;

    /** Server-only connection-to-seat binding for the initial 1v1 slice. */
    void ConfigureNetworkSeat(uint8 Seat);
    void ConfigureNetworkResume(
        uint8 Seat,
        uint64 LastAcceptedBatchId,
        uint64 DisconnectTick,
        bool bMatchWasStarted);
    void ConfigureNetworkResumeCredential(const FString& Credential);
    /** Sends a stable server rejection through the player-facing failure path. */
    void RejectNetworkSessionFromServer(const FString& StableReason);
    void PresentNetworkReconnectGrace(float GraceSeconds);
    void ClearNetworkReconnectGrace();
    void NotifyNetworkOpponentForfeit(
        uint64 FinalTick,
        const FString& StableReason,
        bool bWaitForResultRecipient);
    void NotifyNetworkHostSurrender(
        uint64 FinalTick,
        const FString& StableReason);
    [[nodiscard]] uint64 GetLastAcceptedNetworkBatchId() const
    {
        return LastAcceptedNetworkBatchId;
    }
    [[nodiscard]] bool HasNetworkMatchStarted() const
    {
        return bNetworkMatchStarted;
    }
    [[nodiscard]] bool IsNetworkResumePending() const
    {
        return bNetworkResumePending;
    }

    [[nodiscard]] bool IsDraggingSelection() const;
    [[nodiscard]] FVector2D GetSelectionStartScreenPosition() const;
    [[nodiscard]] FVector2D GetSelectionCurrentScreenPosition() const;
    [[nodiscard]] const TArray<uint32>& GetSelectedEntityIds() const;
    [[nodiscard]] TOptional<echoes::sim::EntityType>
        GetActiveSelectionSubgroupType() const;
    [[nodiscard]] uint64 GetTutorialPendingRejectionAttempt() const
    {
        return TutorialPendingRejectionAttempt;
    }
    [[nodiscard]] uint16 GetTutorialPracticeTargetBit() const
    {
        return TutorialPractice.TargetLessonBit();
    }
    [[nodiscard]] uint16 GetTutorialPracticeAttemptMask() const
    {
        return TutorialPractice.VerifiedAttemptMask();
    }
    [[nodiscard]] echoes::sim::FutureWellChoice GetFutureWellChoice() const;
    [[nodiscard]] FString GetFutureWellChoiceLabel() const;
    [[nodiscard]] FString GetStatusMessage() const;
    [[nodiscard]] bool IsBuildPlacementActive() const { return bBuildPlacementActive; }
    [[nodiscard]] FString GetBuildPlacementGuidance() const { return BuildPlacementGuidance; }
    [[nodiscard]] FString GetFormationLabel() const
    {
        return FEchoesFormationLayout::DisplayName(CurrentFormation);
    }
    [[nodiscard]] EEchoesFormationType GetFormationType() const
    {
        return CurrentFormation;
    }
    bool SetControlGroup(
        int32 GroupIndex,
        const TArray<uint32>& EntityIds,
        FString& OutFeedback);
    [[nodiscard]] TArray<uint32> GetValidControlGroup(int32 GroupIndex) const;
    void NotifyRuntimeReady();
    void StartPointerCombatGuardReview();
    /**
     * Bounded non-shipping driver for SPEC-UI-009.CONFIRM/.TIMEOUT in a real
     * rendered window: open Options, apply a changed display mode, then let the
     * fifteen-second wall-time deadline expire unattended and report the window
     * the player is left with. Runs the ordinary shell actions, so it exercises
     * the production path rather than a parallel one. Agent-driven; it is not
     * physical input and not human acceptance.
     */
    void StartDisplayRevertReview();
    /**
     * Bounded non-shipping driver for SPEC-OUT-002 / SPEC-OUT-006 in a real
     * rendered window: deploy an ordinary skirmish through the title, concede it
     * from the pause menu, and report what the end-of-match banner and the result
     * dossier actually say. It drives the same shell actions a player presses.
     * Agent-driven; it is not physical input and not human acceptance.
     */
    void StartConcessionResultReview();
    /**
     * Bounded non-shipping driver for the DeliveryPlan D2 exit chain in a real
     * rendered window: deploy an ordinary Glass Scar skirmish through the title,
     * then gather to delivery, place and finish a structure, harvest the Well,
     * train to the 30-entity limit and read its refusal, move and fight with
     * visible health change, quick-save and quick-load, repair a damaged owned
     * target, and end with a truthful result. It drives the same controller and
     * bridge actions the player's bindings call and writes one capture per
     * stage. Agent-driven in-process review; not physical input, not packaged
     * execution and not human acceptance.
     */
    void StartD2ExitReview();
    void NotifyRuntimeFailure(const FString& FailureCode);
    void NotifyMatchFinished(echoes::sim::MatchOutcome Outcome);
    void NotifyCampaignPrologueFinished(
        bool bSuccess,
        echoes::sim::FutureWellChoice Consequence,
        echoes::sim::FutureWellChoice RecordedConsequence,
        EEchoesCampaignCommitStatus CommitStatus);
    void NotifySevenAccountsFinished(
        bool bSuccess,
        echoes::sim::FutureWellChoice Consequence,
        echoes::sim::FutureWellChoice RecordedConsequence,
        EEchoesCampaignCommitStatus CommitStatus);
    void NotifyCityReserveFinished(
        bool bSuccess,
        echoes::sim::FutureWellChoice Consequence,
        echoes::sim::FutureWellChoice RecordedConsequence,
        EEchoesCampaignCommitStatus CommitStatus);
    void NotifyUnburiedRoadFinished(
        bool bSuccess,
        echoes::sim::FutureWellChoice Consequence,
        echoes::sim::FutureWellChoice RecordedConsequence,
        EEchoesCampaignCommitStatus CommitStatus);
    void NotifyTermsOfContinuanceFinished(
        bool bSuccess,
        echoes::sim::FutureWellChoice Consequence,
        echoes::sim::FutureWellChoice RecordedConsequence,
        EEchoesCampaignCommitStatus CommitStatus);
    void NotifyNamesWithoutBirthsFinished(
        bool bSuccess,
        echoes::sim::FutureWellChoice Consequence,
        echoes::sim::FutureWellChoice RecordedConsequence,
        EEchoesCampaignCommitStatus CommitStatus);
    void NotifyShapeOfSilenceFinished(
        bool bSuccess,
        echoes::sim::FutureWellChoice Consequence,
        echoes::sim::FutureWellChoice RecordedConsequence,
        EEchoesCampaignCommitStatus CommitStatus);
    void NotifyShapeBesideUsFinished(
        bool bSuccess,
        echoes::sim::FutureWellChoice Consequence,
        echoes::sim::FutureWellChoice RecordedConsequence,
        EEchoesCampaignCommitStatus CommitStatus);
    void NotifyReserveAuthorityFinished(
        bool bSuccess,
        echoes::sim::FutureWellChoice Consequence,
        echoes::sim::FutureWellChoice RecordedConsequence,
        EEchoesCityDistrict DeferredDistrict,
        EEchoesCityDistrict RecordedDeferredDistrict,
        EEchoesCampaignCommitStatus CommitStatus);
    void NotifyChoirAtLumeReachFinished(
        bool bSuccess,
        echoes::sim::FutureWellChoice Consequence,
        echoes::sim::FutureWellChoice RecordedConsequence,
        EEchoesCampaignCommitStatus CommitStatus);
    void NotifyNoNeutralLedgerFinished(
        bool bSuccess,
        echoes::sim::FutureWellChoice Consequence,
        echoes::sim::FutureWellChoice RecordedConsequence,
        EEchoesCampaignCommitStatus CommitStatus);
    void NotifyFutureThatWonFinished(
        bool bSuccess,
        echoes::sim::FutureWellChoice Consequence,
        echoes::sim::FutureWellChoice RecordedConsequence,
        EEchoesCampaignCommitStatus CommitStatus);
    void NotifyAssemblyOfTheMissingFinished(
        bool bSuccess,
        echoes::sim::FutureWellChoice Consequence,
        echoes::sim::FutureWellChoice RecordedConsequence,
        EEchoesCampaignCommitStatus CommitStatus);
    void NotifySeveralVoicesOneCommandFinished(
        bool bSuccess,
        echoes::sim::FutureWellChoice Consequence,
        echoes::sim::FutureWellChoice RecordedConsequence,
        EEchoesCampaignCommitStatus CommitStatus);
    void NotifyBrokenSunFinished(
        bool bSuccess,
        EEchoesFinalResolution Resolution,
        EEchoesFinalResolution RecordedResolution,
        EEchoesCampaignCommitStatus CommitStatus);
    FEchoesFieldHudView BuildFieldHudView() const;
    void RefreshFieldHud();
    void HandleFieldHudAction(EEchoesFieldHudAction Action, int32 Argument = 0);
    bool HandleFieldHudPointer(const FVector2D& NormalizedMapPosition, bool bIssueOrder);
    void HandleFieldHudEndpoint(const FString& Endpoint);
    UEchoesFieldHudWidget* GetFieldHudWidget() const { return FieldHudWidget; }
    FEchoesShellView BuildShellView() const;
    void HandleShellAction(EEchoesShellAction Action, int32 Argument = 0);
    void HandleShellValue(EEchoesShellAction Action, float Value, bool bCommit);
    void BuildResourceMonitorShellView(FEchoesShellView& View) const;
    bool OpenResourceMonitor();
    void BuildFeedbackHistoryShellView(FEchoesShellView& View) const;
    bool HandleFeedbackHistoryShellAction(EEchoesShellAction Action);
    FEchoesShellView BuildControlsShellView() const;
    bool HandleControlsShellAction(EEchoesShellAction Action, int32 Argument);
    bool IsCapturingControlBinding() const;
    void CaptureControlBinding(
        const FKey& Key,
        bool bShift,
        bool bCtrl,
        bool bAlt,
        bool bCmd);
    void BuildReplayShellView(FEchoesShellView& View) const;
    bool HandleReplayShellAction(EEchoesShellAction Action, int32 Argument, bool bConfirmed);
    void RefreshReplayBrowser();
    void PollReplayBrowser();
    void CancelReplayBrowserScan();
#if WITH_DEV_AUTOMATION_TESTS
    void DrainReplayBrowserScan();
#endif
    bool IsReplayBrowserLoading() const { return bReplayBrowserLoading; }
    const TArray<FEchoesReplayMetadata>& GetReplayBrowserEntries() const { return ReplayBrowserEntries; }
    void AppendMatchResultDossier(FEchoesShellView& View) const;
    void RevertPendingDisplay();
    /**
     * The live game window, which is the only authority for the presentation the
     * player is actually looking at. UEchoesGameUserSettings can disagree with it:
     * a -windowed/-ResX command line, an engine clamp during window creation, or a
     * mode the platform refused all leave the stored preference describing a window
     * that was never adopted. SPEC-UI-009.CONFIRM/.TIMEOUT restore "the previous
     * valid mode", so the previous mode is read from here, not from the setting.
     * Returns false when no game window exists (unattended automation, PIE), where
     * the caller must fall back to the stored settings.
     */
    bool GetLiveDisplayPresentation(FIntPoint& OutResolution, EWindowMode::Type& OutMode) const;
    void RefreshShell();
    bool InitializePlayerProfile();
    bool CommitPlayerProfile();
    bool RequireOperationProfile();
    const FEchoesPlayerProfile& GetPlayerProfile() const { return PlayerProfile; }
    bool UsesShellWidget() const;
    const FEchoesPlayerFlow& GetPlayerFlow() const { return PlayerFlow; }
    void PresentTitleScreen();
    void OpenOnlineFrontDoor();
    void ConfirmOnlineFrontDoorAction();
    void CancelOnlineFrontDoor();
    void LeaveOnlineMatch();
    /** Opens a presentation-only route from the live online field menu. */
    bool OpenOnlineLocalMenuShellScreen(EEchoesShellScreen Screen);
    [[nodiscard]] bool IsOnlineLocalMenuShellRouteActive() const;
    void BeginHostedNetworkMatchPresentation();
    void ConfirmTitleScreen();
    void PresentMissionBriefing();
    void ConfirmMissionBriefing();
    void ConfirmPrimaryAction();
    void FocusPreviousSkirmishSetting();
    void FocusNextSkirmishSetting();
    void DecreaseSkirmishSetting();
    void IncreaseSkirmishSetting();
    bool SetPendingSkirmishSetup(
        const FEchoesSkirmishSetup& Setup,
        FString& OutFeedback);
    void ReturnToSkirmishSetup();
    void RequestReturnToOperations();
    /** Opens the exact next briefing derived from durable campaign progress. */
    void ContinueCampaign();
    void OpenCampaignOperationsMap();
    void CloseCampaignOperationsMap();
    void ToggleCampaignOperationsMap();
    void SelectNextCampaignMapNode();
    void SelectPreviousCampaignMapNode();
    void SetSelectedCampaignMapNodeIndex(int32 Index);
    void DeploySelectedCampaignOperation();
    [[nodiscard]] bool IsCampaignOperationsMapVisible() const
    {
        return bCampaignOperationsMapVisible;
    }
    [[nodiscard]] int32 GetSelectedCampaignMapNodeIndex() const
    {
        return SelectedCampaignMapNodeIndex;
    }
    void ChooseFinalRestoration();
    void ChooseFinalStabilization();
    void ChooseFinalExtinguishment();
    void ChooseFinalEvolution();
    void CyclePlayableFaction();
    void CycleOperation();
    void RequestNewCampaign();
    void RequestCampaignRestore();
    void CycleOwnedEntityNext();
    void CycleOwnedEntityPrevious();
    void CycleSelectionSubgroupPrevious();
    void SelectCombatForce();
    void CycleFormation();
    void ToggleKeyboardTargeting();
    void ToggleTechnologyPanel();
    void FocusPreviousTechnologyTier();
    void FocusNextTechnologyTier();
    void TogglePauseMenu();
    void RestartScenario();
    void RepairOrRestartPressed();
    void RepairAtCursor();
    bool TryIssueWorkerMaintenanceContext(uint32 TargetId);
    bool IssueSelectedWorkerMaintenance(uint32 TargetId, bool bConstructionAssist);
    void CancelSelectedConstruction();
    /** Owner-scoped transient evidence for presentation; never command authority. */
    [[nodiscard]] const echoes::feedback::GameplayFeedbackState& GetGameplayFeedback() const;
    [[nodiscard]] bool IsGameplayFeedbackRecoveryPending() const
    {
        return bGameplayFeedbackReseedRequested;
    }
    void ToggleTacticalPause();
#if WITH_DEV_AUTOMATION_TESTS
    // Historical explicit-position fixtures; shipping input is routed by UMG.
    bool HandleMinimapPointer(const FVector2D& ScreenPosition, const FVector2D& ViewportSize, bool bIssueOrder);
    bool HandleTechnologyPanelPointer(const FVector2D& ScreenPosition);
    /** Activates the visible modal control at a shared-layout screen position. */
    bool HandleModalOverlayPointer(
        const FVector2D& ScreenPosition,
        const FVector2D& ViewportSize,
        float HudScale);
    bool HandleOnlineFrontDoorPointer(
        const FVector2D& ScreenPosition,
        const FVector2D& ViewportSize,
        float HudScale);
    bool HandleCampaignOperationsMapPointer(
        const FVector2D& ScreenPosition,
        const FVector2D& ViewportSize,
        float HudScale);
    /**
     * Battlefield press at an explicit screen position. Returns true when the
     * press was consumed by a HUD panel (command deck, objectives, status,
     * minimap, main) so it must not reach battlefield selection. Selection
     * itself is never cleared by a consumed press.
     */
    bool HandleBattlefieldPointerPressed(
        const FVector2D& ScreenPosition,
        const FVector2D& ViewportSize);
#endif
    /** Runs one command-deck action; cursor-targeted actions arm instead. */
    void ActivateCommandDeckAction(EEchoesCommandDeckAction Action);
    /**
     * Which campaign title controls exist right now. The HUD draws from this
     * and the pointer handler hit-tests from it, so a control cannot be drawn
     * without being clickable or clickable without being drawn.
     */
    [[nodiscard]] FEchoesTitleOverlayFacts BuildTitleOverlayFacts() const;
    /** Selection profile driving the deck's labels, buttons, and hit-tests. */
    [[nodiscard]] FEchoesCommandDeckProfile BuildCommandDeckProfile() const;
    [[nodiscard]] EEchoesCommandDeckAction GetArmedDeckAction() const
    {
        return ArmedDeckAction;
    }
    /**
     * SPEC-RES-006.INSPECT: the visible Matter deposit the player clicked to
     * read its stock, or 0. Inspection is not selection: the deposit joins the
     * selection card and the status line but never the command deck, and any
     * change of the owned selection clears it.
     */
    [[nodiscard]] uint32 GetInspectedEntityId() const { return InspectedEntityId; }
    [[nodiscard]] FString GetLocalFactionLabel() const;
    [[nodiscard]] FString GetOpponentFactionLabel() const;
    [[nodiscard]] bool IsMissionBriefingVisible() const
    {
        return PlayerFlow.Is(EEchoesShellScreen::Briefing);
    }
    [[nodiscard]] bool IsTitleScreenVisible() const
    {
        return PlayerFlow.Is(EEchoesShellScreen::Title);
    }
    [[nodiscard]] bool IsMatchResultVisible() const
    {
        return PlayerFlow.Is(EEchoesShellScreen::Results);
    }
    [[nodiscard]] bool IsNewCampaignConfirmationArmed() const;
    [[nodiscard]] bool IsCampaignRestoreConfirmationArmed() const;
    [[nodiscard]] bool IsReturnToOperationsConfirmationArmed() const;
    [[nodiscard]] bool IsSkirmishSetupVisible() const;
    [[nodiscard]] bool IsSkirmishDeploymentSummaryVisible() const;
    [[nodiscard]] bool CanReturnCompletedSkirmishToOperations() const;
    [[nodiscard]] bool CanLeaveNetworkMatchToOnlineMenu() const;
    [[nodiscard]] bool IsOnlineMatchResult() const;
    [[nodiscard]] bool IsActiveOnlineNetworkMatch() const;
    [[nodiscard]] bool IsOnlineFrontDoorVisible() const;
    [[nodiscard]] bool IsOnlineLocalMenuVisible() const
    {
        return bOnlineLocalMenuVisible;
    }
    [[nodiscard]] bool IsNetworkResultExitEnabled() const
    {
        return bNetworkResultExitEnabled;
    }
    [[nodiscard]] uint64 GetPresentedFinalTick() const
    {
        return PresentedFinalTick;
    }
    [[nodiscard]] bool IsOpponentReconnectGraceActive() const;
    [[nodiscard]] int32 GetOpponentReconnectSecondsRemaining() const;
    [[nodiscard]] const FEchoesSkirmishSetup&
    GetPendingSkirmishSetup() const
    {
        return PendingSkirmishSetup;
    }
    [[nodiscard]] int32 GetSkirmishSetupFocusRow() const
    {
        return SkirmishSetupFocusRow;
    }
    [[nodiscard]] bool IsPauseMenuVisible() const
    {
        return PlayerFlow.Is(EEchoesShellScreen::Pause);
    }
    [[nodiscard]] bool IsTechnologyPanelVisible() const
    {
        return bTechnologyPanelVisible;
    }
    [[nodiscard]] bool IsProductionCancellationConfirmationVisible() const
    {
        return PendingProductionCancellation.bVisible;
    }
    [[nodiscard]] bool GetProductionCancellationConfirmation(
        FEchoesFieldHudProductionCancellationView& OutView) const;
    [[nodiscard]] int32 GetTechnologyPanelFocusedTier() const
    {
        return TechnologyPanelFocusedTier;
    }
    [[nodiscard]] bool IsKeyboardTargetingEnabled() const
    {
        return bKeyboardTargetingEnabled;
    }
    [[nodiscard]] FVector2D GetKeyboardTargetOffset() const
    {
        return KeyboardTargetOffset;
    }
    [[nodiscard]] bool IsReplayInputActive() const;
    [[nodiscard]] bool IsModalOverlayVisible() const
    {
        return bM01OpeningPending || PlayerFlow.HasOverlay() || PlayerFlow.Is(EEchoesShellScreen::Title) || PlayerFlow.Is(EEchoesShellScreen::Briefing) ||
               PlayerFlow.Is(EEchoesShellScreen::Pause) || bTechnologyPanelVisible ||
               PendingProductionCancellation.bVisible || TutorialSkipModal.bVisible ||
               PlayerFlow.Is(EEchoesShellScreen::Results) || bOnlineLocalMenuVisible ||
               bCampaignOperationsMapVisible ||
               IsOpponentReconnectGraceActive() ||
               IsOnlineFrontDoorVisible() ||
               (bNetworkCompatibilityAccepted && !bNetworkMatchStarted);
    }
    [[nodiscard]] echoes::sim::MatchOutcome GetPresentedMatchOutcome() const
    {
        return PresentedMatchOutcome;
    }
    [[nodiscard]] bool DidPresentedLocalPlayerWin() const;
    [[nodiscard]] EEchoesOperationMode GetPresentedCampaignOperation() const
    {
        return PresentedCampaignOperation;
    }
    [[nodiscard]] bool IsCampaignResult() const { return bCampaignResult; }
    [[nodiscard]] bool WasCampaignSuccessful() const
    {
        return bCampaignResult && bCampaignSuccess;
    }
    [[nodiscard]] bool CanAdvanceCampaignResult() const
    {
        return bCampaignResult && bCampaignSuccess &&
            (CampaignCommitStatus == EEchoesCampaignCommitStatus::Added ||
             CampaignCommitStatus ==
                 EEchoesCampaignCommitStatus::AlreadyRecorded);
    }
    [[nodiscard]] echoes::sim::FutureWellChoice GetCampaignConsequence() const
    {
        return CampaignConsequence;
    }
    [[nodiscard]] echoes::sim::FutureWellChoice
    GetRecordedCampaignConsequence() const
    {
        return RecordedCampaignConsequence;
    }
    [[nodiscard]] EEchoesCampaignCommitStatus GetCampaignCommitStatus() const
    {
        return CampaignCommitStatus;
    }
    [[nodiscard]] EEchoesFinalResolution GetCampaignFinalResolution() const
    {
        return CampaignFinalResolution;
    }
    [[nodiscard]] EEchoesFinalResolution
    GetRecordedCampaignFinalResolution() const
    {
        return RecordedCampaignFinalResolution;
    }
    [[nodiscard]] const echoes::sim::net::ScopedViewKeyframe*
    GetNetworkScopedView() const
    {
        return NetworkViewState.Current().has_value()
                   ? &*NetworkViewState.Current()
                   : nullptr;
    }
    [[nodiscard]] bool IsNetworkRemoteBattlefieldReady() const
    {
        return bNetworkRemoteBattlefieldReady;
    }
    [[nodiscard]] bool IsNetworkCompatibilityAccepted() const
    {
        return bNetworkCompatibilityAccepted;
    }
    [[nodiscard]] bool IsNetworkMatchStarted() const
    {
        return bNetworkMatchStarted;
    }
    [[nodiscard]] uint8 GetNetworkSeat() const { return NetworkSeat; }
    [[nodiscard]] int32 GetNetworkPresentedEntityCount() const
    {
        return NetworkEntityViews.Num();
    }

private:
    bool OpenProductionCancellationConfirmation(
        const FEchoesFieldHudProductionView& Production,
        int32 Slot);
    void ConfirmProductionCancellation();
    void CloseProductionCancellationConfirmation(bool bRestoreScenarioPause);
    void ValidateProductionCancellationConfirmation();
    [[nodiscard]] bool IsProductionCancellationCurrent(
        const UEchoesSimulationSubsystem& Bridge) const;

    void InitializeTacticalInputPresentation();
    void ShutdownTacticalInputPresentation();
    void UpdateTacticalInputPresentation();
    void BeginBuildPlacement(echoes::sim::EntityType BuildingType);
    bool ConfirmBuildPlacement();
    void CancelBuildPlacement(bool bShowFeedback = true);
    UPROPERTY(Transient)
    TObjectPtr<UEchoesContextCursorWidget> ContextCursorWidget;
    UPROPERTY(Transient)
    TObjectPtr<AEchoesBuildPlacementPreview> BuildPlacementPreview;
    echoes::sim::EntityId BuildPlacementWorkerId = 0;
    echoes::sim::EntityType BuildPlacementType = echoes::sim::EntityType::Barracks;
    FVector BuildPlacementWorldPosition = FVector::ZeroVector;
    int32 BuildPlacementHalfExtentRaw = 0;
    bool bBuildPlacementActive = false;
    FString BuildPlacementGuidance;
    // Presentation diagnostics are intentionally absent from saves/replay commands.
    FString BuildPlacementAttempt;
    FString LastBuildPlacementDiagnostic;
    FString LastBuildPlacementSemantic;
    double LastBuildPlacementLogTime = -1.0;
    bool bBuildPlacementValid = false;
#if WITH_DEV_AUTOMATION_TESTS
    friend class FEchoesNetworkProtocolTest;
    friend class FEchoesOnlineLocalMenuRouteTest;
    friend class FEchoesResourceMonitorShellTest;
    friend class FEchoesTrainingReadinessOperationTest;
    friend class FEchoesTutorialAnchorSelectionTest;
#endif

    UFUNCTION(Server, Reliable)
    void ServerSubmitNetworkResumeCredential(const FString& Credential);

    [[nodiscard]] UEchoesGameInstance* GetEchoesGameInstance() const;

    /** Title, deployment, and result music/ambience selection. Presentation
     *  only: these read presented state and never feed validation or the
     *  simulation. */
    void PresentTitleAudio();
    void PresentDeploymentAudio();
    void PresentResultAudio(bool bSuccess);
    void PresentEndingAudio(
        EEchoesFinalResolution RecordedResolution,
        bool bSuccess);

    bool HandleOnlineEndpointKey(const FInputKeyEventArgs& Params);
    void CopyOnlineHostEndpoint();
#if !UE_BUILD_SHIPPING
    void StartOnlineFrontDoorHostSmoke();
    void StartOnlineFrontDoorClientSmoke();
#endif

    UFUNCTION(Client, Reliable)
    void ClientReceiveNetworkResumeCredentialResult(
        bool bAccepted,
        const FString& StableReason);

    UFUNCTION(Client, Reliable)
    void ClientReceiveOnlineSessionFailure(const FString& StableReason);

    UFUNCTION(Server, Reliable)
    void ServerSubmitCompatibilityHello(const TArray<uint8>& Packet);

    UFUNCTION(Client, Reliable)
    void ClientReceiveCompatibilityResult(
        bool bAccepted,
        const FString& StableReason);

    UFUNCTION(Client, Reliable)
    void ClientReceiveScopedKeyframe(const TArray<uint8>& Packet);

    UFUNCTION(Client, Unreliable)
    void ClientReceiveScopedDelta(const TArray<uint8>& Packet);

    UFUNCTION(Server, Reliable)
    void ServerSetNetworkReady();

    UFUNCTION(Server, Reliable)
    void ServerLeaveNetworkMatch();

    UFUNCTION(Client, Reliable)
    void ClientReceiveNetworkLobbyState(
        bool bStarted,
        uint8 AssignedSeat,
        uint64 AuthorityTick,
        uint8 InputDelayTicks);

    UFUNCTION(Client, Reliable)
    void ClientReceiveNetworkResumeCredential(
        const FString& Credential,
        float GraceSeconds);

    UFUNCTION(Client, Reliable)
    void ClientReceiveNetworkResumeState(
        bool bResumed,
        uint64 NextBatchId,
        uint64 LastAcceptedSequence,
        uint64 AuthorityTick,
        uint64 DisconnectTick);

    UFUNCTION(Server, Reliable)
    void ServerAcknowledgeScopedKeyframe(
        uint64 SnapshotId,
        uint64 ScopedDigest);

    UFUNCTION(Server, Reliable)
    void ServerRequestScopedKeyframe(uint64 LastAcceptedSnapshotId);

    UFUNCTION(Server, Reliable)
    void ServerSubmitNetworkCommand(const TArray<uint8>& Packet);

    UFUNCTION(Server, Reliable)
    void ServerSubmitNetworkCommandBatch(const TArray<uint8>& Packet);

    UFUNCTION(Client, Reliable)
    void ClientReceiveGameplayFeedback(
        uint64 Generation, uint8 Recipient, uint64 AfterEventId,
        bool bResetStream, bool bHistoryTruncated,
        const FEchoesGameplayFeedbackLossPacket& Loss,
        const TArray<FEchoesGameplayFeedbackPacket>& Events);

    UFUNCTION(Server, Reliable)
    void ServerRequestGameplayFeedbackReseed();

    void PublishGameplayFeedback();
    void RequestGameplayFeedbackReseed();
    void ResetNetworkGameplayFeedback();

    UFUNCTION(Client, Reliable)
    void ClientReceiveCommandAdmission(
        uint8 Status,
        uint64 ServerTick,
        const FString& SimulationReason);

    UFUNCTION(Client, Reliable)
    void ClientReceiveCommandBatchAdmission(
        uint64 BatchId,
        int32 AcceptedCount,
        int32 RejectedCount,
        uint64 ServerTick,
        const FString& FirstRejection);

    UFUNCTION(Client, Reliable)
    void ClientReceiveNetworkMatchResult(
        uint8 Outcome,
        uint64 FinalTick,
        uint64 FinalSnapshotId,
        uint64 FinalScopedDigest);

    UFUNCTION(Server, Reliable)
    void ServerAcknowledgeNetworkMatchResult(
        uint8 Outcome,
        uint64 FinalTick,
        uint64 FinalSnapshotId,
        uint64 FinalScopedDigest);

    UFUNCTION(Client, Reliable)
    void ClientReceiveCommandExecution(
        bool bExecuted,
        uint32 ActorId,
        int32 PositionXRaw,
        int32 PositionYRaw,
        uint64 ServerTick);

    UFUNCTION(Server, Reliable)
    void ServerConfirmNetworkSmokeComplete(uint64 SnapshotId);

    UFUNCTION(Client, Reliable)
    void ClientConfirmNetworkSmokeComplete(uint64 SnapshotId);

    UFUNCTION(Server, Reliable)
    void ServerConfirmNetworkMatchSmokeComplete(
        uint8 Outcome,
        uint64 FinalTick,
        uint64 FinalSnapshotId,
        uint64 FinalScopedDigest);

    UFUNCTION(Server, Reliable)
    void ServerConfirmNetworkReconnectSmokeComplete(
        uint64 SnapshotId,
        uint64 LastAcceptedSequence,
        uint64 LastAcceptedBatchId);

    void SubmitNetworkResumeCredential();
    void SubmitNetworkCompatibilityHello();
    void RejectNetworkCompatibility(const FString& StableReason);
    void BeginNetworkMatch();
    bool ResumeNetworkMatch();
    void ResetNetworkSnapshotTransmission();
    bool CanSendNetworkSnapshot();
    void AcknowledgeNetworkSnapshot(uint64 SnapshotId, uint64 ScopedDigest);
    void DeliverDelayedNetworkAcknowledgement();
    void SendScopedKeyframe();
    void SendScopedUpdate();
    bool BuildNextScopedKeyframe(
        echoes::sim::net::ScopedViewKeyframe& OutKeyframe,
        FString& OutError);
    void RequestScopedKeyframeRecovery(const FString& Reason);
    void ProcessScopedDeltaPacket(const TArray<uint8>& Packet);
    void DeliverDelayedNetworkDelta();
    [[nodiscard]] bool IsNetworkClientControlActive() const;
    [[nodiscard]] const echoes::sim::net::ScopedEntityState*
    FindNetworkEntity(uint32 EntityId) const;
    [[nodiscard]] FVector NetworkSimToWorld(
        const echoes::sim::Vec2& Position) const;
    [[nodiscard]] echoes::sim::Vec2 NetworkWorldToSim(
        const FVector& Position) const;
    bool SubmitNetworkCommandBatch(
        TArray<echoes::sim::net::CommandIntent> Intents,
        const FString& OrderLabel,
        const FVector& MarkerLocation,
        EEchoesCommandMarkerType MarkerType);
    bool SubmitNetworkSelectionCommand(
        echoes::sim::CommandType CommandType,
        uint32 TargetId,
        const FVector& Destination,
        bool bUseFormation,
        bool bUseActorPosition,
        const FString& OrderLabel,
        EEchoesCommandMarkerType MarkerType,
        echoes::sim::EntityType BuildType =
            echoes::sim::EntityType::Barracks,
        echoes::sim::WarformAdaptation Adaptation =
            echoes::sim::WarformAdaptation::None,
        echoes::sim::ResearchType Research =
            echoes::sim::ResearchType::None);
    void PublishNetworkMatchResultIfFinished(
        const echoes::sim::net::ScopedViewKeyframe& FinalView);
    void StartNetworkHandshakeTimeout();
    void StartNetworkReadyTimeout();
    void ClearNetworkConnectionTimeouts();
    void HandleNetworkHandshakeTimeout();
    void HandleNetworkReadyTimeout();
    void HandlePlayerOnlineFailure(
        const FString& StableReason,
        bool bPreserveReconnect);
    void BeginHostNetworkResultDeliveryWait();
    void AllowHostNetworkResultExitAfterTimeout();
    void EnableHostNetworkResultExit(
        bool bAcknowledged,
        const FString& StableReason);
    bool SyncNetworkPresentation(
        const echoes::sim::net::ScopedViewKeyframe& Keyframe);
    [[nodiscard]] static echoes::sim::Entity BuildNetworkPresentationEntity(
        const echoes::sim::net::ScopedEntityState& Scoped);
    [[nodiscard]] AEchoesEntityView* AcquireNetworkEntityView();
    void ReleaseNetworkEntityView(AEchoesEntityView* View);
    void DestroyNetworkPresentation();
    void QueueNetworkSmokeHostCommand();
    void VerifyRemoteCommandExecution();
    void TryFinishNetworkClientSmoke();
    void TrySubmitNetworkMatchSmoke(
        const echoes::sim::net::ScopedViewKeyframe& Keyframe);
    void TryAdvanceNetworkReconnectSmoke(
        const echoes::sim::net::ScopedViewKeyframe& Keyframe);
    bool SubmitNetworkReconnectSmokeBatch(
        const echoes::sim::net::ScopedViewKeyframe& Keyframe);
    void FinishNetworkClientSmoke();
    void RunPointerCombatGuardReviewStage(float DeltaTime);
    void RunDisplayRevertReviewStage(float DeltaTime);
    void RunConcessionResultReviewStage(float DeltaTime);
    void RunD2ExitReviewStage(float DeltaTime);
    void FinishD2ExitReview(const TCHAR* Result, const FString& Detail);
    void CaptureD2ExitReview(const TCHAR* Stage);
    void AdvanceD2ExitReview(int32 NextStage, const TCHAR* StageName, const FString& Detail);
    void FinishConcessionResultReview(const TCHAR* Result, const FString& Detail);
    void FinishDisplayRevertReview(const TCHAR* Result, const FString& Detail);
    void LogDisplayRevertReviewPresentation(const TCHAR* Stage) const;
    bool MoveReviewPointerToEntity(uint32 EntityId, const TCHAR* StageLabel);
    void FailPointerCombatGuardReview(const FString& Reason);

    /** Asks the engine for the viewport the active variant's camera and HUD
        coordinates were authored against. */
    void RequestPointerReviewViewport();

    /** Puts back the HUD scale the review overrode. */
    void RestorePointerReviewHudScale();
    friend class AEchoesRTSCameraPawn;
    bool ResolvePointerScreenPosition(
        FVector2D& OutScreenPosition,
        FVector2D* OutViewportSize = nullptr);
    void ChooseFinalResolution(EEchoesFinalResolution Resolution);
    void SelectionPressed();
    void SelectionReleased();
    void ContextOrderPressed();
    void KeyboardContextOrderPressed();
    void NudgeKeyboardTargetLeft();
    void NudgeKeyboardTargetRight();
    void SnapKeyboardTargetToSelection();
    void ChooseHarvest();
    void ChoosePreserve();
    void ChooseReshape();
    void BuildBarracks();
    void BuildDropoff();
    void BuildUtility();
    void ProduceWorker();
    void ProduceSoldier();
    void ProduceHeavy();
    void ProduceScout();
    void ResearchNextTechnology();
    void ResearchTechnologyByTier(int32 TierIndex);
    void ResearchTechnology(echoes::sim::ResearchType Research);
    void AttackMoveAtCursor();
    void PatrolAtCursor();
    void HoldSelectedUnits();
    void GuardAtCursor();
    void StopSelectedUnits();
    void ToggleBulwarkDeploymentAtCursor();
    void ActivateRelaySupply();
    void ToggleWaystoneRoot();
    void AdaptSelectedWarformsCarapace();
    void AdaptSelectedWarformsStriker();
    void AdaptSelectedWarforms(echoes::sim::WarformAdaptation Adaptation);
    void RaiseSelectedCairnbackCoverAtCursor();
    void ReconcileSelectedChoirToManifest();
    void ReconcileSelectedChoirToPossible();
    void ReconcileSelectedChoirIdentities(
        echoes::sim::ChoirIdentityState StableState);
    void QuickSaveScenario();
    void QuickLoadScenario();
    void CycleHudScale();
    void ToggleHighContrast();
    void ToggleReducedMotion();
    void ToggleReducedFlashing();
    void ToggleEdgePan();
    /** SPEC-UI-008.F25 / SPEC-TUT-008 chapter 5: frame the most recent off-screen attack. */
    void JumpToLatestAlert();
    void DecreaseCameraPanSpeed();
    void IncreaseCameraPanSpeed();
    void DecreaseCameraZoomSpeed();
    void IncreaseCameraZoomSpeed();
    void CycleEffectsVolume();
    void ToggleReducedDynamicRange();
    void AdjustCameraPanSpeed(float Delta);
    void AdjustCameraZoomSpeed(float Delta);
    void ArmControlGroupAssignment();
    void RecallControlGroup1();
    void RecallControlGroup2();
    void RecallControlGroup3();
    void RecallControlGroup4();
    void RecallControlGroup5();
    void RecallControlGroup6();
    void RecallControlGroup7();
    void RecallControlGroup8();
    void RecallControlGroup9();
    void RecallControlGroup0();
    void CycleOwnedEntity(int32 Direction);
    TArray<FVector> BuildSelectedFormationDestinations(
        const FVector& Anchor,
        int32 UnitCount);

    void SelectAtCursor(bool bAdditive);
    void InspectDeposit(const class AEchoesEntityView& View);
    void SelectInScreenRectangle(bool bAdditive);
    void SetEntitySelected(uint32 EntityId, bool bSelected);
    void ClearSelection();
    void PruneSelection();
    [[nodiscard]] static int32 ControlGroupDisplayNumber(int32 GroupIndex);
    void AssignControlGroupFromSelection(int32 GroupIndex);
    void RecallControlGroup(int32 GroupIndex);
    void ClearControlGroups();
    void CycleSelectionSubgroup(bool bPrevious);
    void CycleSelectionSubgroupOrOwned(bool bPrevious);
    void NormalizeSelectionSubgroup();
    [[nodiscard]] TArray<echoes::sim::EntityType>
        GetSelectionSubgroupTypes() const;
    bool TraceCursor(FHitResult& OutHitResult);
    bool TraceKeyboardTarget(FHitResult& OutHitResult);
    bool TraceCommandTarget(FHitResult& OutHitResult);
    // Ground position and entity identity are two answers, so they are two
    // traces. TraceCommandTarget keeps returning the ECC_Visibility ground hit
    // that every command site reads as a battlefield point; this one runs the
    // same screen ray on ECC_EchoesEntityPick and returns the entity view under
    // the cursor, or null. Neither trace can move the other's answer.
    bool ResolveCommandScreenPosition(
        bool bPointerSource,
        FVector2D& OutScreenPosition);

    [[nodiscard]] class AEchoesEntityView* TraceEntityUnderCommandTarget(
        const FVector2D& ScreenPosition);
    void NudgeKeyboardTarget(const FVector2D& Direction);
    void IssueContextOrder(const FHitResult& HitResult, bool bPointerSource);
    void SynchronizeBoundCampaignProtocol();
    void SetFutureWellChoice(echoes::sim::FutureWellChoice Choice);
    void BuildAtCursor(echoes::sim::EntityType BuildingType);
    void ProduceUnit(echoes::sim::EntityType UnitType);
    void SetStatusMessage(const FString& Message, float DisplaySeconds = 4.0f);
    void ShowAcceptedCommandMarker(
        const FVector& WorldLocation,
        EEchoesCommandMarkerType MarkerType,
        int32 AcceptedCount);
    FString CommandLabel(echoes::sim::CommandType CommandType) const;

    TArray<uint32> SelectedEntityIds;
    uint32 InspectedEntityId = 0;
    TArray<uint32> ControlGroups[10];
    int32 ActiveSelectionSubgroupIndex = INDEX_NONE;
    double LastControlGroupRecallRealTime[10]{};
    FVector2D SelectionStartScreenPosition = FVector2D::ZeroVector;
    FVector2D SelectionCurrentScreenPosition = FVector2D::ZeroVector;
    echoes::sim::FutureWellChoice FutureWellChoice =
        echoes::sim::FutureWellChoice::Harvest;
    EEchoesFormationType CurrentFormation = EEchoesFormationType::Box;
    FVector LastFormationForward = FVector(1.0f, 0.0f, 0.0f);
    FString StatusMessage;
    FVector2D KeyboardTargetOffset = FVector2D::ZeroVector;
    FVector2D LastPointerScreenPosition = FVector2D::ZeroVector;
    uint32 PointerReviewDefenderId = 0;
    uint32 PointerReviewProtectedId = 0;
    uint32 PointerReviewHostileId = 0;
    int32 PointerReviewInitialHostileHitPoints = 0;
    int32 PointerReviewStage = 0;
    float PointerReviewStageElapsedSeconds = 0.0f;
    float PointerReviewTotalElapsedSeconds = 0.0f;
    FString PointerReviewVariant = TEXT("Default");
    float PointerReviewHudScale = 1.0f;
    FIntPoint PointerReviewExpectedViewport = FIntPoint(1600, 900);
    /** Seconds into the review at which the viewport reached the variant's
        declared size, or a negative value while it has not. The resolution
        request is asynchronous, so the review waits rather than judging the
        first frame it happens to see. */
    float PointerReviewViewportSettledAtSeconds = -1.0f;
    /** HUD scale in force before the review overrode it, or negative when the
        review is not holding one. Restored when the review ends so a controlled
        run cannot rewrite a player-owned setting. */
    float PointerReviewPriorHudScale = -1.0f;
    bool bPointerCombatGuardReviewActive = false;
    bool bDisplayRevertReviewActive = false;
    bool bConcessionResultReviewActive = false;
    bool bD2ExitReviewActive = false;
    int32 D2ExitReviewStage = 0;
    float D2ExitReviewStageElapsedSeconds = 0.0f;
    float D2ExitReviewTotalElapsedSeconds = 0.0f;
    int32 D2ExitReviewCaptureIndex = 0;
    FString D2ExitReviewOutputDir;
    FString D2ExitReviewStagesPassed;
    FString D2ExitReviewStagesUnproven;
    int32 D2ExitReviewMatterBeforeGather = 0;
    int32 D2ExitReviewDawnBeforeWell = 0;
    uint32 D2ExitReviewBuilderId = 0;
    uint32 D2ExitReviewWellWorkerId = 0;
    uint32 D2ExitReviewWellId = 0;
    uint32 D2ExitReviewBuiltStructureId = 0;
    uint32 D2ExitReviewRepairWorkerId = 0;
    uint32 D2ExitReviewRepairTargetId = 0;
    int32 D2ExitReviewRepairStartHitPoints = 0;
    int32 D2ExitReviewPeakArmy = 0;
    bool bD2ExitReviewArmyLimitRefused = false;
    bool bD2ExitReviewCombatObserved = false;
    uint64 D2ExitReviewSaveTick = 0;
    uint64 D2ExitReviewSaveChecksum = 0;
    int32 ConcessionReviewStage = 0;
    float ConcessionReviewStageElapsedSeconds = 0.0f;
    float ConcessionReviewTotalElapsedSeconds = 0.0f;
    int32 DisplayRevertReviewStage = 0;
    float DisplayRevertReviewStageElapsedSeconds = 0.0f;
    float DisplayRevertReviewTotalElapsedSeconds = 0.0f;
    FIntPoint DisplayRevertReviewEntryResolution = FIntPoint(0, 0);
    EWindowMode::Type DisplayRevertReviewEntryMode = EWindowMode::Windowed;
    FIntPoint DisplayRevertReviewAppliedResolution = FIntPoint(0, 0);
    EWindowMode::Type DisplayRevertReviewAppliedMode = EWindowMode::Windowed;
    double StatusMessageExpiresAt = 0.0;
    double ControlGroupAssignmentExpiresAt = 0.0;
    double NewCampaignConfirmationExpiresAt = 0.0;
    double CampaignRestoreConfirmationExpiresAt = 0.0;
    double ReturnToOperationsConfirmationExpiresAt = 0.0;
    int32 TechnologyPanelFocusedTier = 0;
    int32 SkirmishSetupFocusRow = 0;
    bool bSelectionButtonDown = false;
    bool bRuntimeStateKnown = false;
    bool bControlGroupAssignmentArmed = false;
    /**
     * Deck action awaiting a battlefield target, armed by a deck button whose
     * handler resolves its position at the cursor. Presentation/input state
     * only, in the same family as bControlGroupAssignmentArmed.
     */
    EEchoesCommandDeckAction ArmedDeckAction = EEchoesCommandDeckAction::None;
    bool bOnlineLocalMenuVisible = false;
    FEchoesPlayerFlow PlayerFlow;
    UPROPERTY(Transient)
    TObjectPtr<UEchoesShellWidget> ShellWidget;
    UPROPERTY(Transient)
    TObjectPtr<UEchoesFieldHudWidget> FieldHudWidget;
    UPROPERTY(Transient)
    TObjectPtr<AEchoesPowerNetworkView> PowerNetworkView;
    mutable FString LastFieldHudError;
    bool bFieldHudWasModal = false;
    EEchoesFieldHudSurface LastFieldHudSurface = EEchoesFieldHudSurface::Hidden;
    EEchoesShellAction PendingShellAction = EEchoesShellAction::Cancel;
    int32 PendingShellArgument = 0;
    FString ShellMessage;
    EEchoesFeedbackHistoryFilter FeedbackHistoryFilter =
        EEchoesFeedbackHistoryFilter::All;
    void ApplyConfirmedDefaultBindings();
    int32 PendingControlBindingIndex = INDEX_NONE;
    TOptional<FEchoesInputBinding> PendingControlBinding;
    FString ControlBindingMessage;
    TArray<FEchoesReplayMetadata> ReplayBrowserEntries;
    TFuture<FEchoesReplayBrowserScanResult> ReplayBrowserScan;
    std::shared_ptr<std::atomic_bool> ReplayBrowserCancellation;
    FEchoesReplayBrowserFilter RequestedReplayBrowserFilter;
    FString RequestedReplayBrowserDirectory;
    uint64 ReplayBrowserGeneration = 0;
    bool bReplayBrowserRefreshQueued = false;
    bool bReplayBrowserLoading = false;
    FString ReplayBrowserMapFilter;
    int32 ReplayBrowserDateFilter = 0;
    EEchoesShellScreen ReplayReturnScreen = EEchoesShellScreen::Title;
    uint64 PendingReplayTick = 0;
    FEchoesPlayerProfile PlayerProfile;
    bool bPlayerProfileInitialized = false;
    bool bTutorialOperationAuthorized = false;
    FEchoesTutorialPracticeState TutorialPractice;
    void ResetTutorialObservation();
    void TickTutorialObservation();
    void TickM01Opening();
    void FinishMissionDeployment();
    void SetNarrativePlaybackPausedOutsideCinematic(bool bPaused);
    bool bM01OpeningPending = false;
    double OpeningSpacePressedAt = -1.0;
    void ObserveTutorialSelection(uint32 ClickedEntity, bool bGroundClick);
    void ObserveTutorialSelectionEvent(
        EEchoesTutorialSelectionInputEvent Event,
        int32 ControlGroupIndex = INDEX_NONE,
        echoes::sim::EntityType PreviousSubgroupType =
            echoes::sim::EntityType::Worker,
        echoes::sim::EntityType ActiveSubgroupType =
            echoes::sim::EntityType::Worker);
    void ObserveTutorialRosterHudPublication(
        const FEchoesFieldHudView& PublishedView,
        uint64 PresentationFrame);
    void ObserveTutorialAcceptedCommand(
        uint64 Sequence,
        EEchoesTutorialOrderCommandOrigin Origin);
    void CaptureTutorialAcceptedCommand(
        UEchoesSimulationSubsystem* Bridge,
        const TOptional<uint64>& SequenceBefore,
        EEchoesTutorialOrderCommandOrigin Origin);
    uint64 ObserveTutorialRejectedCommandAttempt(
        const FEchoesTutorialExpectedCommand& Attempt);
    void ObserveTutorialRejectionAcknowledged(uint64 InputAttemptSequence);
    // ---- RESERVED DECLARATIONS (L2-CONTROL owns this header; bodies live elsewhere) ----
    // Declared here so downstream lanes can implement against a frozen header without
    // editing it. Each body belongs in the named file, owned by the named lane.
    // Both adapter headers are FROZEN after this commit: further header changes are
    // batched requests through L2-CONTROL.
    //
    // L3-HUD   — body in EchoesPlayerFieldHud.cpp
    void PresentOffscreenCombatAlert(const FVector2D& WorldLocation);
    // L6-AUDIO — body in a new EchoesPlayerCinematicHooks.cpp
    void PresentCampaignCinematicForSignal(const FString& Signal);
    // L4-ONBOARD — bodies in EchoesPlayerTutorial.cpp. Signatures were left open in the
    // lane brief and are chosen here to match the existing ObserveTutorial* style; if
    // L4 needs different parameters, request the change rather than editing this header.
    void ObserveTutorialConstructionEvent(
        echoes::sim::EntityType StructureType,
        uint32 BuilderEntity,
        echoes::sim::Vec2 Site);
    void ObserveTutorialProductionEvent(
        echoes::sim::EntityType ProducedType,
        uint32 ProducerEntity);
    // Lesson six (Link): a placement the simulation refused, and the finished
    // Link described in the field HUD on a frame after the click that chose it.
    void ObserveTutorialPlacementRejected(
        echoes::sim::EntityType StructureType,
        echoes::sim::Vec2 Site);
    void ObserveTutorialConstructionHudPublication(
        const FEchoesFieldHudView& PublishedView,
        uint64 PresentationFrame);
    // ---- end reserved declarations ----
    bool CommitTutorialLesson(uint16 Bit, const TCHAR* LessonName);
    FEchoesTutorialSurveyObservation TutorialSurvey;
    FEchoesTutorialSelectionObservation TutorialSelection;
    FEchoesTutorialOrderObservation TutorialOrders;
    // ---- RESERVED MEMBERS for L4-ONBOARD (lessons 6-10); written only by that lane ----
    FEchoesTutorialConstructionObservation TutorialConstruction;
    uint16 TutorialConstructionLessonBit = 0;
    uint16 TutorialProductionLessonBit = 0;
    uint32 TutorialObservedConstructionEntity = 0;
    uint32 TutorialObservedProductionEntity = 0;
    uint64 TutorialConstructionSequence = 0;
    uint64 TutorialProductionSequence = 0;
    uint32 TutorialConstructionHudCandidateEntity = 0;
    uint64 TutorialConstructionHudCandidateFrame = 0;
    // Lesson seven (Foundry): every entity id known when the lesson opened is
    // below this; a Lancer above it is the one the player's order produced.
    uint32 TutorialFoundryMaxKnownEntityId = 0;
    bool bTutorialFoundryEmerged = false;
    // Lesson eight (Probe): the scripted contact's units, the workers that must
    // survive it, and the player's own attack-move and guard orders.
    TArray<uint32> TutorialProbeUnits;
    TArray<uint32> TutorialProbeProtectedWorkers;
    uint64 TutorialProbeAttackSequence = 0;
    uint64 TutorialProbeGuardSequence = 0;
    // Lesson nine (Board): an attack flagged off-screen after the lesson opened,
    // and the jump that framed it.
    double TutorialBoardOpenedSeconds = 0.0;
    bool bTutorialBoardJumped = false;
    // Lesson ten (Well): the player's own committed protocol at the readiness Well.
    uint32 TutorialWellId = 0;
    uint64 TutorialWellSequence = 0;
    // ---- end reserved members ----
    FText TutorialInstruction;
    uint64 TutorialSession = 0;
    uint64 TutorialSelectionSequence = 0;
    uint64 TutorialRosterHudCandidateFrame = 0;
    uint32 TutorialRosterHudCandidateEntity = 0;
    uint64 TutorialAuthorityGeneration = 0;
    uint64 TutorialLastAcceptedCommandSequence = 0;
    TArray<TPair<uint64, EEchoesTutorialOrderCommandOrigin>>
        TutorialAcceptedCommandSequences;
    uint64 TutorialInputAttemptSequence = 0;
    uint64 TutorialPendingRejectionAttempt = 0;
    uint16 TutorialActiveLessonBit = 0;
    uint16 TutorialPresentedLessonBit = 0;
    uint16 TutorialSkippedMask = 0;
    // Transient: preserve genuine later completions without forging a contiguous
    // durable mastery prefix across a skipped lesson. Reset on a fresh tutorial.
    uint16 TutorialSessionVerifiedMask = 0;
    struct FEchoesTutorialSkipModal final
    {
        bool bVisible = false;
        bool bScenarioWasPaused = false;
    } TutorialSkipModal;
    uint64 TutorialLastTick = 0;
    uint64 TutorialInitialNavigationRevision = 0;
    bool bTutorialHasTick = false;
    bool bTutorialCoreSelected = false;
    bool bTutorialWorkerSelected = false;
    bool bTutorialProgressSaveFailed = false;
    bool bTutorialReserveMonitorInspected = false;
    uint32 TutorialCoreId = 0;
    uint32 TutorialWorkerId = 0;
    /** Last emitted observation trace; transitions only, never per tick. */
    FString TutorialObservationTrace;
    void TraceTutorialObservation(const FString& State);
    bool bPlayerProfileAvailable = false;
    bool bShellWasVisible = false;
    bool bMinimapDragging = false;
    bool bTacticalPaused = false;
    uint64 PresentedCheckpointRequestId = 0;
    bool bPresentedCheckpointPending = false;
#if WITH_DEV_AUTOMATION_TESTS
public:
    // Unattended automation has no game window, so the live presentation must be
    // injectable to exercise the settings/window disagreement that this repair covers.
    static void SetLiveDisplayPresentationForTesting(
        bool bPresent,
        FIntPoint Resolution = FIntPoint(1280, 720),
        EWindowMode::Type Mode = EWindowMode::Windowed);
private:
#endif
    void SeedPendingDisplayFromLivePresentation();
    FIntPoint PendingDisplayResolution = FIntPoint(1280, 720);
    EWindowMode::Type PendingDisplayMode = EWindowMode::Windowed;
    FIntPoint PreviousDisplayResolution = FIntPoint(1280, 720);
    EWindowMode::Type PreviousDisplayMode = EWindowMode::Windowed;
    double DisplayRevertDeadline = 0.0;
    bool bTechnologyPanelVisible = false;
    bool bTechnologyPanelWasScenarioPaused = false;
    struct FEchoesPendingProductionCancellation final
    {
        bool bVisible = false;
        bool bScenarioWasPaused = false;
        uint64 AuthorityGeneration = 0;
        uint32 ProducerId = 0;
        uint64 ItemId = 0;
        int32 Slot = 0;
        echoes::sim::EntityType UnitType = echoes::sim::EntityType::Worker;
        FText Unit;
        int32 Progress = 0;
        int32 RequiredTicks = 0;
        int32 ConfiguredMatter = 0;
        int32 ConfiguredDawn = 0;
        int32 InvestedMatter = 0;
        int32 InvestedDawn = 0;
        int32 Logistics = 0;
        int32 RefundPercent = 0;
        int32 RefundMatter = 0;
        int32 RefundDawn = 0;
    } PendingProductionCancellation;
    bool bKeyboardTargetingEnabled = false;
    bool bNewCampaignConfirmationArmed = false;
    bool bCampaignRestoreConfirmationArmed = false;
    bool bReturnToOperationsConfirmationArmed = false;
    bool bCampaignOperationsMapVisible = false;
    int32 SelectedCampaignMapNodeIndex = 0;
    bool bCampaignResult = false;
    FEchoesSkirmishSetup PendingSkirmishSetup =
        FEchoesSkirmishSetupModel::DefaultSetup();
    uint8 NetworkSeat = echoes::sim::kNeutralPlayer;
    bool bNetworkCompatibilityAccepted = false;
    bool bNetworkReady = false;
    bool bNetworkMatchStarted = false;
    bool bNetworkClientSmoke = false;
    bool bNetworkMatchSmoke = false;
    bool bNetworkReconnectPhaseOneSmoke = false;
    bool bNetworkReconnectPhaseTwoSmoke = false;
    bool bNetworkResumePending = false;
    bool bNetworkResumeMatchWasStarted = false;
    bool bNetworkResumeAccepted = false;
    bool bNetworkReconnectBatchSubmitted = false;
    bool bNetworkReconnectBatchAdmitted = false;
    bool bNetworkReconnectCompletionSent = false;
    bool bNetworkCommandSubmitted = false;
    bool bNetworkRemoteExecutionReceived = false;
    bool bNetworkSmokeCompletionSent = false;
    bool bNetworkCommandExecutionVerified = false;
    bool bNetworkHostExecutionVerified = false;
    bool bNetworkRemoteBattlefieldReady = false;
    bool bNetworkDroppedFirstDeltaForSmoke = false;
    bool bNetworkDelayFirstDeltaForSmoke = false;
    bool bNetworkDuplicateFirstDeltaForSmoke = false;
    bool bNetworkReorderFirstTwoDeltasForSmoke = false;
    bool bNetworkDropDeltaBurstForSmoke = false;
    bool bNetworkFaultInjectionPerformed = false;
    bool bNetworkFaultRecoveryObserved = false;
    bool bNetworkDelayedDeltaDelivered = false;
    bool bNetworkDuplicateDeltaIgnored = false;
    uint8 NetworkDroppedDeltaCount = 0;
    uint64 LastNetworkSnapshotId = 0;
    uint64 LastAcknowledgedNetworkSnapshotId = 0;
    uint64 NetworkSnapshotAcknowledgementCount = 0;
    uint64 NetworkSmokeCompletionSnapshotId = 0;
    uint64 NextNetworkBatchId = 1;
    uint64 LastAcceptedNetworkBatchId = 0;
    uint64 NetworkResumeDisconnectTick = 0;
    uint64 NetworkReconnectExpectedSequence = 0;
    uint64 NetworkReconnectExpectedBatchId = 0;
    uint8 NetworkInputDelayTicks = 0;
    uint32 NetworkReconnectActorId = 0;
    echoes::sim::Vec2 NetworkReconnectInitialPosition{};
    FString NetworkResumeCredential;
    FString DevelopmentResumeCredentialFilePath;
    bool bNetworkMatchResultSent = false;
    bool bNetworkMatchResultReceived = false;
    bool bNetworkMatchResultAcknowledged = false;
    bool bNetworkResultExitEnabled = false;
    bool bOpponentReconnectGraceActive = false;
    bool bReturnHostToOnlineAfterResultDelivery = false;
    bool bNetworkMatchCommandSubmitted = false;
    bool bNetworkMatchBatchAdmitted = false;
    bool bNetworkMatchSmokeCompletionSent = false;
    echoes::network::ScopedViewState NetworkViewState{};
    uint8 NetworkSentResultOutcome = 0;
    uint64 NetworkSentResultTick = 0;
    uint64 NetworkSentResultSnapshotId = 0;
    uint64 NetworkSentResultScopedDigest = 0;
    uint64 PresentedFinalTick = 0;
    double OpponentReconnectExpiresAtSeconds = 0.0;
    std::optional<echoes::sim::net::ScopedViewKeyframe>
        LastSentNetworkKeyframe{};
    TMap<uint64, uint64> PendingNetworkSnapshotDigests;
    echoes::network::SnapshotFlowControl NetworkSnapshotFlow;
    bool bNetworkSnapshotBackpressure = false;
    bool bNetworkSnapshotClosing = false;
    TMap<uint32, TWeakObjectPtr<AEchoesEntityView>> NetworkEntityViews;
    UPROPERTY(Transient)
    TArray<TObjectPtr<AEchoesEntityView>> NetworkFreeEntityViews;
    TWeakObjectPtr<AEchoesFogView> NetworkFogView;
    TWeakObjectPtr<AEchoesTerrainView> NetworkTerrainView;
    TWeakObjectPtr<AStaticMeshActor> NetworkGroundView;
    TWeakObjectPtr<ADirectionalLight> NetworkDirectionalLight;
    TWeakObjectPtr<ASkyLight> NetworkSkyLight;
    echoes::sim::net::CommandAdmissionContext NetworkCommandContext{};
    echoes::sim::net::CommandRequest PendingRemoteCommand{};
    echoes::sim::Vec2 PendingRemoteInitialPosition{};
    uint32 PendingHostCommandActor = 0;
    uint64 PendingHostCommandExecuteTick = 0;
    echoes::sim::Vec2 PendingHostCommandInitialPosition{};
    echoes::sim::Vec2 PendingHostCommandTargetPosition{};
    FTimerHandle NetworkExecutionTimer;
    FTimerHandle NetworkKeyframeTimer;
    FTimerHandle NetworkClientExitTimer;
    FTimerHandle NetworkServerExitTimer;
    FTimerHandle NetworkFaultDeliveryTimer;
    FTimerHandle NetworkAcknowledgementDelayTimer;
    uint64 DelayedNetworkAcknowledgementId = 0;
    uint64 DelayedNetworkAcknowledgementDigest = 0;
    bool bNetworkAcknowledgementDelayPerformed = false;
    FTimerHandle NetworkHandshakeTimer;
    FTimerHandle NetworkReadyTimer;
    FTimerHandle NetworkResultAcknowledgementTimer;
    TArray<uint8> PendingNetworkFaultDelta;
    double LastScopedRecoveryRequestClientSeconds = -1000.0;
    double LastScopedRecoveryRequestServerSeconds = -1000.0;
    echoes::network::CommandRateLimiter NetworkCommandRateLimiter{};
    echoes::feedback::GameplayFeedbackState NetworkGameplayFeedback;
    echoes::feedback::GameplayFeedbackLoss SentGameplayFeedbackLoss;
    uint64 SentGameplayFeedbackGeneration = 0;
    uint64 SentGameplayFeedbackEventId = 0;
    bool bSentGameplayFeedbackInitialized = false;
    bool bGameplayFeedbackReseedRequested = false;
    double LastGameplayFeedbackReseedSeconds = -1.0;
    double LastGameplayFeedbackRequestSeconds = -1.0;
    FTimerHandle GameplayFeedbackReseedTimer;
    bool bCampaignSuccess = false;
    echoes::sim::FutureWellChoice CampaignConsequence =
        echoes::sim::FutureWellChoice::Dormant;
    echoes::sim::FutureWellChoice RecordedCampaignConsequence =
        echoes::sim::FutureWellChoice::Dormant;
    EEchoesFinalResolution CampaignFinalResolution =
        EEchoesFinalResolution::None;
    EEchoesFinalResolution RecordedCampaignFinalResolution =
        EEchoesFinalResolution::None;
    EEchoesCampaignCommitStatus CampaignCommitStatus =
        EEchoesCampaignCommitStatus::NotApplicable;
    EEchoesOperationMode PresentedCampaignOperation =
        EEchoesOperationMode::Skirmish;
    echoes::sim::MatchOutcome PresentedMatchOutcome =
        echoes::sim::MatchOutcome::Ongoing;
};
