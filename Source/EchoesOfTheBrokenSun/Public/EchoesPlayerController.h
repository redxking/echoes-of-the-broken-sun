#pragma once

#include "CoreMinimal.h"
#include "Async/Future.h"
#include <atomic>
#include <memory>
#include "EchoesCampaignProgress.h"
#include "EchoesPlayerFlow.h"
#include "EchoesPlayerProfile.h"
#include "EchoesMatchReplay.h"
#include "EchoesCampaignMapLayout.h"
#include "EchoesCommandDeckModel.h"
#include "EchoesFormationLayout.h"
#include "EchoesFieldHudView.h"
#include "EchoesNetworkSession.h"
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
    [[nodiscard]] echoes::sim::FutureWellChoice GetFutureWellChoice() const;
    [[nodiscard]] FString GetFutureWellChoiceLabel() const;
    [[nodiscard]] FString GetStatusMessage() const;
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
    void RefreshShell();
    bool InitializePlayerProfile();
    bool CommitPlayerProfile();
    bool RequireOperationMastery(EEchoesOperationMode Operation, bool bLearningCheckpoint = false);
    const FEchoesPlayerProfile& GetPlayerProfile() const { return PlayerProfile; }
    bool UsesShellWidget() const;
    const FEchoesPlayerFlow& GetPlayerFlow() const { return PlayerFlow; }
    void PresentTitleScreen();
    void OpenOnlineFrontDoor();
    void ConfirmOnlineFrontDoorAction();
    void CancelOnlineFrontDoor();
    void LeaveOnlineMatch();
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
    void CycleOwnedEntityPrevious();
    void SelectCombatForce();
    void CycleFormation();
    void ToggleKeyboardTargeting();
    void ToggleTechnologyPanel();
    void FocusPreviousTechnologyTier();
    void FocusNextTechnologyTier();
    void TogglePauseMenu();
    void RestartScenario();
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
        return PlayerFlow.HasOverlay() || PlayerFlow.Is(EEchoesShellScreen::Title) || PlayerFlow.Is(EEchoesShellScreen::Briefing) ||
               PlayerFlow.Is(EEchoesShellScreen::Pause) || bTechnologyPanelVisible ||
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
    bool bBuildPlacementValid = false;
#if WITH_DEV_AUTOMATION_TESTS
    friend class FEchoesNetworkProtocolTest;
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
    bool MoveReviewPointerToEntity(uint32 EntityId, const TCHAR* StageLabel);
    void FailPointerCombatGuardReview(const FString& Reason);

    /** Asks the engine for the viewport the active variant's camera and HUD
        coordinates were authored against. */
    void RequestPointerReviewViewport();

    /** Puts back the HUD scale the review overrode. */
    void RestorePointerReviewHudScale();
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
    void SelectInScreenRectangle(bool bAdditive);
    void SetEntitySelected(uint32 EntityId, bool bSelected);
    void ClearSelection();
    void PruneSelection();
    [[nodiscard]] static int32 ControlGroupDisplayNumber(int32 GroupIndex);
    void AssignControlGroupFromSelection(int32 GroupIndex);
    void RecallControlGroup(int32 GroupIndex);
    void ClearControlGroups();
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
    TArray<uint32> ControlGroups[10];
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
    mutable FString LastFieldHudError;
    bool bFieldHudWasModal = false;
    EEchoesFieldHudSurface LastFieldHudSurface = EEchoesFieldHudSurface::Hidden;
    EEchoesShellAction PendingShellAction = EEchoesShellAction::Cancel;
    int32 PendingShellArgument = 0;
    FString ShellMessage;
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
    bool bPlayerProfileAvailable = false;
    bool bShellWasVisible = false;
    bool bMinimapDragging = false;
    bool bTacticalPaused = false;
    uint64 PresentedCheckpointRequestId = 0;
    bool bPresentedCheckpointPending = false;
    FIntPoint PendingDisplayResolution = FIntPoint(1280, 720);
    EWindowMode::Type PendingDisplayMode = EWindowMode::Windowed;
    FIntPoint PreviousDisplayResolution = FIntPoint(1280, 720);
    EWindowMode::Type PreviousDisplayMode = EWindowMode::Windowed;
    double DisplayRevertDeadline = 0.0;
    bool bTechnologyPanelVisible = false;
    bool bTechnologyPanelWasScenarioPaused = false;
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
    FTimerHandle NetworkHandshakeTimer;
    FTimerHandle NetworkReadyTimer;
    FTimerHandle NetworkResultAcknowledgementTimer;
    TArray<uint8> PendingNetworkFaultDelta;
    double LastScopedRecoveryRequestClientSeconds = -1000.0;
    double LastScopedRecoveryRequestServerSeconds = -1000.0;
    echoes::network::CommandRateLimiter NetworkCommandRateLimiter{};
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
