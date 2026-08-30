#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "EchoesCampaignProgress.h"
#include "EchoesChoirAtLumeReachMissionModel.h"
#include "EchoesCityReserveMissionModel.h"
#include "EchoesFutureThatWonMissionModel.h"
#include "EchoesNamesWithoutBirthsMissionModel.h"
#include "EchoesNoNeutralLedgerMissionModel.h"
#include "EchoesPrologueMissionModel.h"
#include "EchoesReserveAuthorityMissionModel.h"
#include "EchoesSevenAccountsMissionModel.h"
#include "EchoesShapeOfSilenceMissionModel.h"
#include "EchoesShapeBesideUsMissionModel.h"
#include "EchoesTermsOfContinuanceMissionModel.h"
#include "EchoesUnburiedRoadMissionModel.h"
#include "EchoesSimCore/Simulation.h"
#include "EchoesSimCore/NetworkProtocol.h"
#include "EchoesSimulationSubsystem.generated.h"

class AEchoesEntityView;
class AEchoesFogView;
class AEchoesTerrainView;
#if WITH_DEV_AUTOMATION_TESTS
class FEchoesPrologueMissionTest;
#endif

/** Information the local presentation may use without exposing hidden state. */
struct FEchoesObjectiveSnapshot final
{
    bool bScenarioReady = false;
    bool bLocalCoreIntact = false;
    int32 LocalCoreHitPoints = 0;
    int32 LocalCoreMaxHitPoints = 0;
    bool bFutureWellVisible = false;
    echoes::sim::FutureWellChoice VisibleFutureWellChoice =
        echoes::sim::FutureWellChoice::Dormant;
    bool bHostileCoreVisible = false;
    echoes::sim::MatchOutcome Outcome = echoes::sim::MatchOutcome::Ongoing;
    EEchoesOperationMode OperationMode = EEchoesOperationMode::Skirmish;
    EEchoesProloguePhase ProloguePhase = EEchoesProloguePhase::Inactive;
    bool bArchiveCarrierIntact = false;
    int32 ArchiveCarrierHitPoints = 0;
    echoes::sim::EntityId ArchiveCarrierId = 0;
    echoes::sim::FutureWellChoice PrologueWellChoice =
        echoes::sim::FutureWellChoice::Dormant;
    EEchoesSevenAccountsPhase SevenAccountsPhase =
        EEchoesSevenAccountsPhase::Inactive;
    echoes::sim::FutureWellChoice SevenAccountsBranch =
        echoes::sim::FutureWellChoice::Dormant;
    echoes::sim::EntityId MemoryBearerId = 0;
    echoes::sim::EntityId MigrationWaystoneId = 0;
    bool bMemoryBearerIntact = false;
    bool bWaystoneIntact = false;
    bool bWaystoneRootedAtAnchor = false;
    bool bMemoryBearerAtAccountSite = false;
    EEchoesCityReservePhase CityReservePhase =
        EEchoesCityReservePhase::Inactive;
    echoes::sim::FutureWellChoice CityReserveBranch =
        echoes::sim::FutureWellChoice::Dormant;
    echoes::sim::EntityId LifeSupportDistrictId = 0;
    echoes::sim::EntityId TransitDistrictId = 0;
    echoes::sim::EntityId ArchiveDistrictId = 0;
    bool bLifeSupportPowered = false;
    bool bTransitPowered = false;
    bool bArchivePowered = false;
    EEchoesUnburiedRoadPhase UnburiedRoadPhase =
        EEchoesUnburiedRoadPhase::Inactive;
    echoes::sim::FutureWellChoice UnburiedRoadBranch =
        echoes::sim::FutureWellChoice::Dormant;
    bool bWaystoneRootedAtRoadhead = false;
    bool bListeningSpineComplete = false;
    bool bMemoryBearerAtShard = false;
    EEchoesTermsOfContinuancePhase TermsOfContinuancePhase =
        EEchoesTermsOfContinuancePhase::Inactive;
    echoes::sim::FutureWellChoice TermsOfContinuanceBranch =
        echoes::sim::FutureWellChoice::Dormant;
    echoes::sim::EntityId MeridianContinuanceRelayId = 0;
    echoes::sim::EntityId KharuunContinuanceSpineId = 0;
    echoes::sim::EntityId MeridianContinuanceWitnessId = 0;
    echoes::sim::EntityId KharuunContinuanceWitnessId = 0;
    bool bMeridianRelaySynchronized = false;
    bool bKharuunSpineSynchronized = false;
    bool bContinuanceWindowHeld = false;
    bool bMeridianWitnessExtracted = false;
    bool bKharuunWitnessExtracted = false;
    EEchoesNamesWithoutBirthsPhase NamesWithoutBirthsPhase =
        EEchoesNamesWithoutBirthsPhase::Inactive;
    echoes::sim::FutureWellChoice NamesWithoutBirthsBranch =
        echoes::sim::FutureWellChoice::Dormant;
    echoes::sim::EntityId TalarId = 0;
    echoes::sim::EntityId CensusArchiveId = 0;
    echoes::sim::EntityId FirstCivilianId = 0;
    echoes::sim::EntityId SecondCivilianId = 0;
    bool bCensusEvidenceLocated = false;
    bool bCensusArchivePowered = false;
    bool bFirstCivilianSheltered = false;
    bool bSecondCivilianSheltered = false;
    bool bTalarAtEvidenceExtraction = false;
    EEchoesShapeOfSilencePhase ShapeOfSilencePhase =
        EEchoesShapeOfSilencePhase::Inactive;
    echoes::sim::FutureWellChoice ShapeOfSilenceBranch =
        echoes::sim::FutureWellChoice::Dormant;
    echoes::sim::EntityId OruunId = 0;
    echoes::sim::EntityId FirstMemoryWitnessId = 0;
    echoes::sim::EntityId SecondMemoryWitnessId = 0;
    bool bShapeWaystoneRooted = false;
    bool bShapeListeningSpineRaised = false;
    bool bFirstMemoryWitnessPositioned = false;
    bool bSecondMemoryWitnessPositioned = false;
    bool bOruunAtConfluence = false;
    EEchoesShapeBesideUsPhase ShapeBesideUsPhase =
        EEchoesShapeBesideUsPhase::Inactive;
    echoes::sim::FutureWellChoice ShapeBesideUsBranch =
        echoes::sim::FutureWellChoice::Dormant;
    echoes::sim::EntityId ShapeBesideUsTalarId = 0;
    echoes::sim::EntityId FirstStateWitnessId = 0;
    echoes::sim::EntityId SecondStateWitnessId = 0;
    bool bFirstEchoObserved = false;
    bool bEchoRelayRaised = false;
    bool bFirstStateTraversed = false;
    bool bSecondStateTraversed = false;
    bool bShapeBesideUsTalarAtConvergence = false;
    EEchoesReserveAuthorityPhase ReserveAuthorityPhase =
        EEchoesReserveAuthorityPhase::Inactive;
    echoes::sim::FutureWellChoice ReserveAuthorityBranch =
        echoes::sim::FutureWellChoice::Dormant;
    echoes::sim::EntityId ReserveAuthorityMaraId = 0;
    EEchoesCityDistrict ReserveAuthorityRecommendedDistrict =
        EEchoesCityDistrict::LifeSupport;
    EEchoesCityDistrict ReserveAuthorityDeferredDistrict =
        EEchoesCityDistrict::LifeSupport;
    bool bReserveAuthoritySecured = false;
    bool bReserveAuthorityMaraAtDeferredDistrict = false;
    EEchoesChoirAtLumeReachPhase ChoirAtLumeReachPhase =
        EEchoesChoirAtLumeReachPhase::Inactive;
    echoes::sim::FutureWellChoice ChoirAtLumeReachPriorBranch =
        echoes::sim::FutureWellChoice::Dormant;
    echoes::sim::FutureWellChoice ChoirAtLumeReachWellChoice =
        echoes::sim::FutureWellChoice::Dormant;
    EEchoesCityDistrict ChoirAtLumeReachDeferredDistrict =
        EEchoesCityDistrict::LifeSupport;
    echoes::sim::EntityId ChoirAtLumeReachOruunId = 0;
    echoes::sim::EntityId ChoirAtLumeReachWaystoneId = 0;
    echoes::sim::EntityId ChoirAtLumeReachWellId = 0;
    bool bChoirContactEstablished = false;
    bool bChoirDeferredLiabilityResolved = false;
    bool bChoirFirstAnchorRaised = false;
    bool bChoirSecondAnchorRaised = false;
    bool bChoirBranchResolutionCompleted = false;
    bool bChoirReshapeWindowExpired = false;
    EEchoesNoNeutralLedgerPhase NoNeutralLedgerPhase =
        EEchoesNoNeutralLedgerPhase::Inactive;
    echoes::sim::FutureWellChoice NoNeutralFoundingDoctrine =
        echoes::sim::FutureWellChoice::Dormant;
    echoes::sim::FutureWellChoice NoNeutralLumeProtocol =
        echoes::sim::FutureWellChoice::Dormant;
    EEchoesCityDistrict NoNeutralFirstDistrict =
        EEchoesCityDistrict::LifeSupport;
    EEchoesCityDistrict NoNeutralSecondDistrict =
        EEchoesCityDistrict::Transit;
    EEchoesCityDistrict NoNeutralDeferredDistrict =
        EEchoesCityDistrict::Archive;
    echoes::sim::EntityId NoNeutralOruunId = 0;
    echoes::sim::EntityId NoNeutralWaystoneId = 0;
    echoes::sim::EntityId NoNeutralLedgerWitnessId = 0;
    echoes::sim::EntityId NoNeutralFirstDistrictInterfaceId = 0;
    echoes::sim::EntityId NoNeutralSecondDistrictInterfaceId = 0;
    echoes::sim::EntityId NoNeutralMeridianEvidenceInterfaceId = 0;
    echoes::sim::EntityId NoNeutralKharuunEvidenceInterfaceId = 0;
    echoes::sim::EntityId NoNeutralWellId = 0;
    bool bNoNeutralRouteSecured = false;
    bool bNoNeutralPublicInterfacesIntact = false;
    bool bNoNeutralFirstDistrictIntegrated = false;
    bool bNoNeutralSecondDistrictIntegrated = false;
    bool bNoNeutralEvidenceAttested = false;
    bool bNoNeutralProtocolApplied = false;
    bool bNoNeutralCoalitionRallied = false;
    bool bNoNeutralReshapeWindowExpired = false;
    EEchoesFutureThatWonPhase FutureThatWonPhase =
        EEchoesFutureThatWonPhase::Inactive;
    echoes::sim::FutureWellChoice FutureWonFoundingDoctrine =
        echoes::sim::FutureWellChoice::Dormant;
    echoes::sim::FutureWellChoice FutureWonRecordedProtocol =
        echoes::sim::FutureWellChoice::Dormant;
    EEchoesCityDistrict FutureWonFirstDistrict =
        EEchoesCityDistrict::LifeSupport;
    EEchoesCityDistrict FutureWonSecondDistrict =
        EEchoesCityDistrict::Transit;
    EEchoesCityDistrict FutureWonDeferredDistrict =
        EEchoesCityDistrict::Archive;
    echoes::sim::EntityId FutureWonOruunId = 0;
    echoes::sim::EntityId FutureWonVerifierId = 0;
    echoes::sim::EntityId FutureWonFirstDistrictInterfaceId = 0;
    echoes::sim::EntityId FutureWonSecondDistrictInterfaceId = 0;
    echoes::sim::EntityId FutureWonMeridianReadbackInterfaceId = 0;
    echoes::sim::EntityId FutureWonKharuunReadbackInterfaceId = 0;
    echoes::sim::EntityId FutureWonDemonstratorInterfaceId = 0;
    echoes::sim::EntityId FutureWonWellId = 0;
    bool bFutureWonPublicInterfacesIntact = false;
    bool bFutureWonIndependentReadbackEstablished = false;
    bool bFutureWonFirstInputVerified = false;
    bool bFutureWonSecondInputVerified = false;
    bool bFutureWonProtocolBound = false;
    bool bFutureWonStabilityWindowHeld = false;
    bool bFutureWonFirstDistrictReadbackObserved = false;
    bool bFutureWonSecondDistrictReadbackObserved = false;
    bool bFutureWonReshapeWindowExpired = false;
    uint64 FutureWonActivationTick = 0;
    uint64 FutureWonStabilityEndTick = 0;
};

/**
 * Owns the deterministic simulation for the current game world.
 *
 * The simulation is authoritative. Unreal actors are disposable, one-way views
 * rebuilt from the simulation state after every fixed tick.
 */
UCLASS()
class ECHOESOFTHEBROKENSUN_API UEchoesSimulationSubsystem final
    : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    static constexpr float TileWorldSize = 200.0f;
    static constexpr uint8 LocalPlayerId = 0;
    static constexpr uint8 OpponentPlayerId = 1;

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override;

    /** Creates the bounded runtime-only technical-prototype scenario. */
    bool StartPrototypeScenario();

    /** Creates the opt-in 400-unit/four-team presentation scale scenario. */
    bool StartStressScenario();

    /** Stops the prototype and releases every disposable presentation view. */
    void StopPrototypeScenario();

    /** Recreates the bounded match from its deterministic initial state. */
    bool RestartPrototypeScenario();

    /** Rebuilds the undeployed operation for one of the two playable factions. */
    bool SelectLocalFaction(
        echoes::sim::Faction NewFaction,
        FString& OutFeedback);

    /** Rebuilds the undeployed runtime for the selected operation. */
    bool SelectOperationMode(
        EEchoesOperationMode NewOperation,
        FString& OutFeedback);

    /** Replaces active campaign decisions with an empty ledger and retains one backup. */
    bool StartNewCampaign(FString& OutFeedback);

    /** Restores the validated prior generation and retains the replaced active one. */
    bool RestoreCampaignBackup(FString& OutFeedback);

    /** Atomically writes a validated deterministic snapshot and retains one backup. */
    bool QuickSaveScenario(FString& OutFeedback) const;

    /** Restores the newest valid quick save, falling back to its prior generation. */
    bool QuickLoadScenario(FString& OutFeedback);

    [[nodiscard]] static FString GetQuickSavePath();

    /** Queues one player command for the next deterministic simulation tick. */
    bool IssueCommand(
        echoes::sim::CommandType CommandType,
        uint32 ActorId,
        uint32 TargetId,
        const FVector& WorldPosition,
        echoes::sim::FutureWellChoice WellChoice,
        FString& OutFeedback);

    bool IssueBuildCommand(
        uint32 WorkerId,
        echoes::sim::EntityType BuildingType,
        const FVector& WorldPosition,
        FString& OutFeedback);

    bool IssueProductionCommand(
        uint32 ProducerId,
        echoes::sim::EntityType UnitType,
        FString& OutFeedback);

    bool IssueResearchCommand(
        uint32 ProducerId,
        echoes::sim::ResearchType ResearchType,
        FString& OutFeedback);

    bool IssueWarformAdaptation(
        uint32 ActorId,
        uint32 SiteId,
        echoes::sim::WarformAdaptation Adaptation,
        FString& OutFeedback);

    bool IssueMineralCover(
        uint32 ActorId,
        const FVector& WorldPosition,
        FString& OutFeedback);

    void SetScenarioPaused(bool bPaused);
    [[nodiscard]] bool IsScenarioPaused() const { return bSimulationPaused; }
    [[nodiscard]] echoes::sim::MatchOutcome GetMatchOutcome() const;
    [[nodiscard]] FEchoesObjectiveSnapshot GetLocalObjectiveSnapshot() const;

    [[nodiscard]] const echoes::sim::Simulation* GetSimulation() const;
    echoes::sim::net::CommandAdmissionStatus AdmitNetworkCommand(
        const echoes::sim::net::CommandRequest& Request,
        echoes::sim::net::CommandAdmissionContext& Context,
        std::string* SimulationRejection = nullptr);
    void SetNetworkHumanOpponent(bool bEnabled);
    [[nodiscard]] const echoes::sim::Entity* FindEntity(uint32 EntityId) const;
    [[nodiscard]] AEchoesEntityView* FindEntityView(uint32 EntityId) const;
    [[nodiscard]] AEchoesFogView* GetFogView() const;
    [[nodiscard]] AEchoesTerrainView* GetTerrainView() const;
    [[nodiscard]] FVector SimToWorld(const echoes::sim::Vec2& Position) const;
    [[nodiscard]] echoes::sim::Vec2 WorldToSim(const FVector& Position) const;
    [[nodiscard]] bool IsScenarioReady() const { return bScenarioReady; }
    [[nodiscard]] bool IsStressScenario() const { return bStressScenario; }
    [[nodiscard]] EEchoesOperationMode GetOperationMode() const
    {
        return SelectedOperation;
    }
    [[nodiscard]] FString GetOperationLabel() const;
    [[nodiscard]] EEchoesProloguePhase GetProloguePhase() const;
    [[nodiscard]] EEchoesSevenAccountsPhase GetSevenAccountsPhase() const;
    [[nodiscard]] bool IsSevenAccountsUnlocked() const;
    [[nodiscard]] EEchoesCityReservePhase GetCityReservePhase() const;
    [[nodiscard]] bool IsCityReserveUnlocked() const;
    [[nodiscard]] EEchoesUnburiedRoadPhase GetUnburiedRoadPhase() const;
    [[nodiscard]] bool IsUnburiedRoadUnlocked() const;
    [[nodiscard]] EEchoesTermsOfContinuancePhase
    GetTermsOfContinuancePhase() const;
    [[nodiscard]] bool IsTermsOfContinuanceUnlocked() const;
    [[nodiscard]] EEchoesNamesWithoutBirthsPhase
    GetNamesWithoutBirthsPhase() const;
    [[nodiscard]] bool IsNamesWithoutBirthsUnlocked() const;
    [[nodiscard]] EEchoesShapeOfSilencePhase
    GetShapeOfSilencePhase() const;
    [[nodiscard]] bool IsShapeOfSilenceUnlocked() const;
    [[nodiscard]] EEchoesShapeBesideUsPhase GetShapeBesideUsPhase() const;
    [[nodiscard]] bool IsShapeBesideUsUnlocked() const;
    [[nodiscard]] EEchoesReserveAuthorityPhase
    GetReserveAuthorityPhase() const;
    [[nodiscard]] bool IsReserveAuthorityUnlocked() const;
    [[nodiscard]] EEchoesChoirAtLumeReachPhase
    GetChoirAtLumeReachPhase() const;
    [[nodiscard]] bool IsChoirAtLumeReachUnlocked() const;
    [[nodiscard]] EEchoesNoNeutralLedgerPhase
    GetNoNeutralLedgerPhase() const;
    [[nodiscard]] bool IsNoNeutralLedgerUnlocked() const;
    [[nodiscard]] EEchoesFutureThatWonPhase
    GetFutureThatWonPhase() const;
    [[nodiscard]] bool IsFutureThatWonUnlocked() const;
    [[nodiscard]] echoes::sim::FutureWellChoice GetRecordedPrologueChoice() const;
    [[nodiscard]] FEchoesSevenAccountsRoute GetSevenAccountsRoute() const;
    [[nodiscard]] FEchoesCityReserveGrid GetCityReserveGrid() const;
    [[nodiscard]] FEchoesUnburiedRoadRoute GetUnburiedRoadRoute() const;
    [[nodiscard]] FEchoesTermsOfContinuancePlan
    GetTermsOfContinuancePlan() const;
    [[nodiscard]] FEchoesNamesWithoutBirthsPlan
    GetNamesWithoutBirthsPlan() const;
    [[nodiscard]] FEchoesShapeOfSilencePlan
    GetShapeOfSilencePlan() const;
    [[nodiscard]] FEchoesShapeBesideUsPlan GetShapeBesideUsPlan() const;
    [[nodiscard]] FEchoesReserveAuthorityPlan
    GetReserveAuthorityPlan() const;
    [[nodiscard]] EEchoesCityDistrict
    GetReserveAuthorityDeferredDistrict() const;
    [[nodiscard]] FEchoesChoirAtLumeReachPlan
    GetChoirAtLumeReachPlan() const;
    [[nodiscard]] FEchoesNoNeutralLedgerPlan
    GetNoNeutralLedgerPlan() const;
    [[nodiscard]] FEchoesFutureThatWonPlan
    GetFutureThatWonPlan() const;
    [[nodiscard]] echoes::sim::EntityId GetCityDistrictId(
        EEchoesCityDistrict District) const;
    [[nodiscard]] echoes::sim::EntityId GetMemoryBearerId() const
    {
        return MemoryBearerId;
    }
    [[nodiscard]] echoes::sim::EntityId GetMigrationWaystoneId() const
    {
        return MigrationWaystoneId;
    }
    [[nodiscard]] echoes::sim::EntityId GetArchiveCarrierId() const
    {
        return ArchiveCarrierId;
    }
    [[nodiscard]] static echoes::sim::Vec2 GetArchiveRecoverySite();
    [[nodiscard]] static echoes::sim::Vec2 GetEvacuationSite();
    [[nodiscard]] const FEchoesCampaignProgress& GetCampaignProgress() const
    {
        return CampaignProgress;
    }
    [[nodiscard]] bool IsCampaignProgressAvailable() const
    {
        return bCampaignProgressAvailable;
    }
    [[nodiscard]] bool HasRestorableCampaignBackup() const
    {
        return bCampaignBackupAvailable;
    }
    [[nodiscard]] int32 GetCampaignBackupDecisionCount() const
    {
        return bCampaignBackupAvailable
                   ? CampaignBackupProgress.Decisions.Num()
                   : 0;
    }
    [[nodiscard]] echoes::sim::Faction GetLocalFaction() const
    {
        return bStressScenario
                   ? echoes::sim::Faction::MeridianCompact
                   : LocalFaction;
    }
    [[nodiscard]] echoes::sim::Faction GetOpponentFaction() const
    {
        return GetLocalFaction() == echoes::sim::Faction::MeridianCompact
                   ? echoes::sim::Faction::KharuunAssemblies
                   : echoes::sim::Faction::MeridianCompact;
    }
    [[nodiscard]] int32 GetMapWidthTiles() const;
    [[nodiscard]] int32 GetMapHeightTiles() const;

private:
#if WITH_DEV_AUTOMATION_TESTS
    friend class FEchoesPrologueMissionTest;
#endif
    [[nodiscard]] FString GetActiveQuickSavePath() const;
    void RefreshCampaignBackupState();
    EEchoesCampaignCommitStatus CommitPrologueCompletion(
        echoes::sim::FutureWellChoice CurrentChoice,
        echoes::sim::FutureWellChoice& OutRecordedChoice,
        FString& OutFeedback);
    EEchoesCampaignCommitStatus CommitSevenAccountsCompletion(
        echoes::sim::FutureWellChoice& OutRecordedChoice,
        FString& OutFeedback);
    EEchoesCampaignCommitStatus CommitCityReserveCompletion(
        echoes::sim::FutureWellChoice& OutRecordedChoice,
        FString& OutFeedback);
    EEchoesCampaignCommitStatus CommitUnburiedRoadCompletion(
        echoes::sim::FutureWellChoice& OutRecordedChoice,
        FString& OutFeedback);
    EEchoesCampaignCommitStatus CommitTermsOfContinuanceCompletion(
        echoes::sim::FutureWellChoice& OutRecordedChoice,
        FString& OutFeedback);
    EEchoesCampaignCommitStatus CommitNamesWithoutBirthsCompletion(
        echoes::sim::FutureWellChoice& OutRecordedChoice,
        FString& OutFeedback);
    EEchoesCampaignCommitStatus CommitShapeOfSilenceCompletion(
        echoes::sim::FutureWellChoice& OutRecordedChoice,
        FString& OutFeedback);
    EEchoesCampaignCommitStatus CommitShapeBesideUsCompletion(
        echoes::sim::FutureWellChoice& OutRecordedChoice,
        FString& OutFeedback);
    EEchoesCampaignCommitStatus CommitReserveAuthorityCompletion(
        echoes::sim::FutureWellChoice& OutRecordedChoice,
        EEchoesCityDistrict& OutRecordedDeferredDistrict,
        FString& OutFeedback);
    EEchoesCampaignCommitStatus CommitChoirAtLumeReachCompletion(
        echoes::sim::FutureWellChoice CurrentChoice,
        echoes::sim::FutureWellChoice& OutRecordedChoice,
        FString& OutFeedback);
    EEchoesCampaignCommitStatus CommitNoNeutralLedgerCompletion(
        echoes::sim::FutureWellChoice& OutRecordedProtocol,
        FString& OutFeedback);
    EEchoesCampaignCommitStatus CommitFutureThatWonCompletion(
        echoes::sim::FutureWellChoice& OutRecordedProtocol,
        FString& OutFeedback);
    void AdvancePrologueCompletionPresentation();
    bool StartScenario(bool bUseStressScenario);
    bool ValidatePrototypeCommand(
        echoes::sim::CommandType CommandType,
        const echoes::sim::Entity& Actor,
        uint32 TargetId,
        const echoes::sim::Vec2& Position,
        echoes::sim::FutureWellChoice WellChoice,
        echoes::sim::EntityType BuildType,
        FString& OutFeedback) const;
    bool QueuePlayerCommand(
        echoes::sim::CommandType CommandType,
        uint32 ActorId,
        uint32 TargetId,
        const echoes::sim::Vec2& Position,
        echoes::sim::FutureWellChoice WellChoice,
        echoes::sim::EntityType BuildType,
        FString& OutFeedback);
    [[nodiscard]] echoes::sim::Tick ResolvePlayerExecuteTick(
        echoes::sim::Tick OfflineDelayTicks) const;
    void QueueOpponentCommands();
    bool SyncEntityViews(bool bTeleportNewViews);
    bool SpawnFogView();
    bool SyncFogView();
    bool SpawnTerrainView();
    bool SyncTerrainView();
    void DestroyEntityViews();
    void DestroyFogView();
    void DestroyTerrainView();

    TUniquePtr<echoes::sim::Simulation> Simulation;
    TMap<uint32, TWeakObjectPtr<AEchoesEntityView>> EntityViews;
    TWeakObjectPtr<AEchoesFogView> FogView;
    TWeakObjectPtr<AEchoesTerrainView> TerrainView;
    double FixedTimeAccumulator = 0.0;
    uint64 NextPlayerCommandSequence = 1;
    bool bScenarioReady = false;
    bool bWarnedAboutTimeClamp = false;
    bool bLoggedFirstTick = false;
    bool bLoggedStressCombat = false;
    bool bLoggedAiExpansion = false;
    bool bLoggedAiRetreat = false;
    bool bLoggedAiPlayerView = false;
    bool bLoggedAiAdaptation = false;
    bool bLoggedAiMineralCover = false;
    bool bLoggedAiVibrationResponse = false;
    bool bResearchPresentationScenario = false;
    bool bResearchInterruptionPresentationScenario = false;
    bool bKharuunSystemsPresentationScenario = false;
    bool bPrologueCompletionPresentationScenario = false;
    bool bPointerCombatGuardPresentationScenario = false;
    bool bLoggedResearchPresentationActive = false;
    bool bLoggedResearchPresentationComplete = false;
    bool bLoggedResearchPresentationInterrupted = false;
    bool bLoggedKharuunSystemsPresentation = false;
    int32 PrologueCompletionPresentationStage = 0;
    echoes::sim::EntityId ProloguePresentationWorkerId = 0;
    echoes::sim::EntityId ProloguePresentationWellId = 0;
    bool bSimulationPaused = false;
    bool bMatchResultReported = false;
    bool bStressScenario = false;
    bool bNetworkHumanOpponent = false;
    echoes::sim::Faction LocalFaction =
        echoes::sim::Faction::MeridianCompact;
    EEchoesOperationMode SelectedOperation = EEchoesOperationMode::Skirmish;
    echoes::sim::EntityId ArchiveCarrierId = 0;
    echoes::sim::EntityId MemoryBearerId = 0;
    echoes::sim::EntityId MigrationWaystoneId = 0;
    echoes::sim::EntityId LifeSupportDistrictId = 0;
    echoes::sim::EntityId TransitDistrictId = 0;
    echoes::sim::EntityId ArchiveDistrictId = 0;
    echoes::sim::EntityId MeridianContinuanceRelayId = 0;
    echoes::sim::EntityId KharuunContinuanceSpineId = 0;
    echoes::sim::EntityId MeridianContinuanceWitnessId = 0;
    echoes::sim::EntityId KharuunContinuanceWitnessId = 0;
    echoes::sim::EntityId TalarId = 0;
    echoes::sim::EntityId CensusArchiveId = 0;
    echoes::sim::EntityId FirstCivilianId = 0;
    echoes::sim::EntityId SecondCivilianId = 0;
    echoes::sim::EntityId OruunId = 0;
    echoes::sim::EntityId FirstMemoryWitnessId = 0;
    echoes::sim::EntityId SecondMemoryWitnessId = 0;
    echoes::sim::EntityId ShapeBesideUsTalarId = 0;
    echoes::sim::EntityId FirstStateWitnessId = 0;
    echoes::sim::EntityId SecondStateWitnessId = 0;
    echoes::sim::EntityId ReserveAuthorityMaraId = 0;
    echoes::sim::EntityId ChoirAtLumeReachOruunId = 0;
    echoes::sim::EntityId ChoirAtLumeReachWaystoneId = 0;
    echoes::sim::EntityId ChoirAtLumeReachWellId = 0;
    echoes::sim::EntityId NoNeutralOruunId = 0;
    echoes::sim::EntityId NoNeutralWaystoneId = 0;
    echoes::sim::EntityId NoNeutralLedgerWitnessId = 0;
    echoes::sim::EntityId NoNeutralFirstDistrictInterfaceId = 0;
    echoes::sim::EntityId NoNeutralSecondDistrictInterfaceId = 0;
    echoes::sim::EntityId NoNeutralMeridianEvidenceInterfaceId = 0;
    echoes::sim::EntityId NoNeutralKharuunEvidenceInterfaceId = 0;
    echoes::sim::EntityId NoNeutralWellId = 0;
    echoes::sim::EntityId FutureWonOruunId = 0;
    echoes::sim::EntityId FutureWonVerifierId = 0;
    echoes::sim::EntityId FutureWonFirstDistrictInterfaceId = 0;
    echoes::sim::EntityId FutureWonSecondDistrictInterfaceId = 0;
    echoes::sim::EntityId FutureWonMeridianReadbackInterfaceId = 0;
    echoes::sim::EntityId FutureWonKharuunReadbackInterfaceId = 0;
    echoes::sim::EntityId FutureWonDemonstratorInterfaceId = 0;
    echoes::sim::EntityId FutureWonWellId = 0;
    FEchoesCampaignProgress CampaignProgress;
    FEchoesCampaignProgress CampaignBackupProgress;
    bool bCampaignProgressAvailable = false;
    bool bCampaignBackupAvailable = false;
    FString CampaignProgressPath;
    echoes::sim::ResearchType ResearchPresentationTechnology =
        echoes::sim::ResearchType::None;
};
