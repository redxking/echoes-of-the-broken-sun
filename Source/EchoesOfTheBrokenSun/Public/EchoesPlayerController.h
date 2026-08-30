#pragma once

#include "CoreMinimal.h"
#include "EchoesCampaignProgress.h"
#include "EchoesFormationLayout.h"
#include "EchoesNetworkSession.h"
#include "EchoesPrologueMissionModel.h"
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

    /** Server-only connection-to-seat binding for the initial 1v1 slice. */
    void ConfigureNetworkSeat(uint8 Seat);
    void ConfigureNetworkResume(
        uint8 Seat,
        uint64 LastAcceptedBatchId,
        uint64 DisconnectTick,
        bool bMatchWasStarted);
    void ConfigureNetworkResumeCredential(const FString& Credential);
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
    void PresentTitleScreen();
    void ConfirmTitleScreen();
    void PresentMissionBriefing();
    void ConfirmMissionBriefing();
    void ConfirmPrimaryAction();
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
    bool HandleTechnologyPanelPointer(const FVector2D& ScreenPosition);
    [[nodiscard]] FString GetLocalFactionLabel() const;
    [[nodiscard]] FString GetOpponentFactionLabel() const;
    [[nodiscard]] bool IsMissionBriefingVisible() const
    {
        return bMissionBriefingVisible;
    }
    [[nodiscard]] bool IsTitleScreenVisible() const
    {
        return bTitleScreenVisible;
    }
    [[nodiscard]] bool IsMatchResultVisible() const
    {
        return bMatchResultVisible;
    }
    [[nodiscard]] bool IsNewCampaignConfirmationArmed() const;
    [[nodiscard]] bool IsCampaignRestoreConfirmationArmed() const;
    [[nodiscard]] bool IsPauseMenuVisible() const
    {
        return bPauseMenuVisible;
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
    [[nodiscard]] bool IsModalOverlayVisible() const
    {
        return bTitleScreenVisible || bMissionBriefingVisible ||
               bPauseMenuVisible || bTechnologyPanelVisible ||
               bMatchResultVisible;
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
    UFUNCTION(Server, Reliable)
    void ServerSubmitNetworkResumeCredential(const FString& Credential);

    UFUNCTION(Client, Reliable)
    void ClientReceiveNetworkResumeCredentialResult(
        bool bAccepted,
        const FString& StableReason);

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
    void BeginNetworkMatch();
    void ResumeNetworkMatch();
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
    bool SyncNetworkPresentation(
        const echoes::sim::net::ScopedViewKeyframe& Keyframe);
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
    void RestartScenario();
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
    void NudgeKeyboardTarget(const FVector2D& Direction);
    void IssueContextOrder(const FHitResult& HitResult, bool bPointerSource);
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
    bool bPointerCombatGuardReviewActive = false;
    double StatusMessageExpiresAt = 0.0;
    double ControlGroupAssignmentExpiresAt = 0.0;
    double NewCampaignConfirmationExpiresAt = 0.0;
    double CampaignRestoreConfirmationExpiresAt = 0.0;
    int32 TechnologyPanelFocusedTier = 0;
    bool bSelectionButtonDown = false;
    bool bRuntimeStateKnown = false;
    bool bControlGroupAssignmentArmed = false;
    bool bTitleScreenVisible = false;
    bool bMissionBriefingVisible = false;
    bool bPauseMenuVisible = false;
    bool bTechnologyPanelVisible = false;
    bool bTechnologyPanelWasScenarioPaused = false;
    bool bKeyboardTargetingEnabled = false;
    bool bMatchResultVisible = false;
    bool bNewCampaignConfirmationArmed = false;
    bool bCampaignRestoreConfirmationArmed = false;
    bool bCampaignResult = false;
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
    bool bNetworkMatchResultSent = false;
    bool bNetworkMatchResultReceived = false;
    bool bNetworkMatchCommandSubmitted = false;
    bool bNetworkMatchBatchAdmitted = false;
    bool bNetworkMatchSmokeCompletionSent = false;
    echoes::network::ScopedViewState NetworkViewState{};
    std::optional<echoes::sim::net::ScopedViewKeyframe>
        LastSentNetworkKeyframe{};
    TMap<uint64, uint64> PendingNetworkSnapshotDigests;
    TMap<uint32, TWeakObjectPtr<AEchoesEntityView>> NetworkEntityViews;
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
    TArray<uint8> PendingNetworkFaultDelta;
    double LastScopedRecoveryRequestClientSeconds = -1000.0;
    double LastScopedRecoveryRequestServerSeconds = -1000.0;
    echoes::network::CommandRateLimiter NetworkCommandRateLimiter{};
    bool bCampaignSuccess = false;
    echoes::sim::FutureWellChoice CampaignConsequence =
        echoes::sim::FutureWellChoice::Dormant;
    echoes::sim::FutureWellChoice RecordedCampaignConsequence =
        echoes::sim::FutureWellChoice::Dormant;
    EEchoesCampaignCommitStatus CampaignCommitStatus =
        EEchoesCampaignCommitStatus::NotApplicable;
    EEchoesOperationMode PresentedCampaignOperation =
        EEchoesOperationMode::Skirmish;
    echoes::sim::MatchOutcome PresentedMatchOutcome =
        echoes::sim::MatchOutcome::Ongoing;
};
