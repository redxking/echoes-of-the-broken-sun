#pragma once

#include "CoreMinimal.h"
#include "EchoesCampaignProgress.h"
#include "EchoesFormationLayout.h"
#include "EchoesPrologueMissionModel.h"
#include "GameFramework/PlayerController.h"
#include "EchoesSimCore/Simulation.h"
#include "EchoesPlayerController.generated.h"

class AEchoesEntityView;
enum class EEchoesCommandMarkerType : uint8;
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
    virtual void PlayerTick(float DeltaTime) override;
    virtual void SetupInputComponent() override;

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

private:
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
    void IssueContextOrder(const FHitResult& HitResult);
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
