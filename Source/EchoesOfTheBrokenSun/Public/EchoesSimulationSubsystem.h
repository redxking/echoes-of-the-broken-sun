#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "EchoesCampaignJourneyModel.h"
#include "EchoesCampaignProgress.h"
#include "EchoesBrokenSunMissionModel.h"
#include "EchoesAssemblyOfTheMissingMissionModel.h"
#include "EchoesChoirAtLumeReachMissionModel.h"
#include "EchoesCityReserveMissionModel.h"
#include "EchoesFutureThatWonMissionModel.h"
#include "EchoesNamesWithoutBirthsMissionModel.h"
#include "EchoesNoNeutralLedgerMissionModel.h"
#include "EchoesPrologueMissionModel.h"
#include "EchoesReserveAuthorityMissionModel.h"
#include "EchoesSeveralVoicesOneCommandMissionModel.h"
#include "EchoesSevenAccountsMissionModel.h"
#include "EchoesShapeOfSilenceMissionModel.h"
#include "EchoesShapeBesideUsMissionModel.h"
#include "EchoesSkirmishSetup.h"
#include "EchoesTermsOfContinuanceMissionModel.h"
#include "EchoesUnburiedRoadMissionModel.h"
#include "EchoesSimCore/Simulation.h"
#include "EchoesSimCore/NetworkProtocol.h"
#include "EchoesSimulationSubsystem.generated.h"

class AEchoesCombatEffectView;
class AEchoesDestructionView;
class AEchoesEntityView;
class AEchoesFogView;
class AEchoesTerrainView;
#if WITH_DEV_AUTOMATION_TESTS
class FEchoesPrologueMissionTest;
class FEchoesFreshCampaignJourneyTest;
class FEchoesAutosaveRecoveryTest;
#endif

UENUM(BlueprintType)
enum class EEchoesAutosaveReason : uint8
{
    None = 0,
    MissionEntry = 1,
    PhaseTransition = 2,
    Manual = 3
};

/** Candidate checkpoint discovered during interrupted-session recovery scans. */
struct ECHOESOFTHEBROKENSUN_API FEchoesRecoveryCandidate final
{
    bool bAvailable = false;
    EEchoesOperationMode OperationMode = EEchoesOperationMode::Skirmish;
    EEchoesCampaignMissionId MissionId = EEchoesCampaignMissionId::WhatTheLedgerKeeps;
    FString MissionName;
    FString PhaseName;
    uint64 SimulationTick = 0;
    uint64 StateChecksum = 0;
    FDateTime SaveTimestamp = FDateTime::MinValue();
    FString SourcePath;
    bool bRecoveredFromBackup = false;
    uint32 ContainerCrc = 0;
    FString RecoveryStatusText;
    FString HonestLimitationNotice;
};

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
    EEchoesAssemblyOfTheMissingPhase AssemblyOfTheMissingPhase =
        EEchoesAssemblyOfTheMissingPhase::Inactive;
    echoes::sim::FutureWellChoice AssemblyFoundingDoctrine =
        echoes::sim::FutureWellChoice::Dormant;
    echoes::sim::FutureWellChoice AssemblyRecordedProtocol =
        echoes::sim::FutureWellChoice::Dormant;
    EEchoesCityDistrict AssemblyFirstDistrict =
        EEchoesCityDistrict::LifeSupport;
    EEchoesCityDistrict AssemblySecondDistrict =
        EEchoesCityDistrict::Transit;
    EEchoesCityDistrict AssemblyDeferredDistrict =
        EEchoesCityDistrict::Archive;
    echoes::sim::EntityId AssemblyOruunId = 0;
    echoes::sim::EntityId AssemblyVerifierId = 0;
    echoes::sim::EntityId AssemblyMeridianPublicRecordInterfaceId = 0;
    echoes::sim::EntityId AssemblyKharuunPublicRecordInterfaceId = 0;
    echoes::sim::EntityId AssemblyCrownfallIndexInterfaceId = 0;
    bool bAssemblyPublicInterfacesIntact = false;
    bool bAssemblyPublicRecordReadbackEstablished = false;
    bool bAssemblyCrownfallIndexLinked = false;
    bool bAssemblyMeridianWitnessObserved = false;
    bool bAssemblyKharuunWitnessObserved = false;
    EEchoesSeveralVoicesOneCommandPhase SeveralVoicesOneCommandPhase =
        EEchoesSeveralVoicesOneCommandPhase::Inactive;
    echoes::sim::FutureWellChoice SeveralVoicesRecordedProtocol =
        echoes::sim::FutureWellChoice::Dormant;
    echoes::sim::EntityId SeveralVoicesPossibleVoiceId = 0;
    echoes::sim::EntityId SeveralVoicesManifestVoiceId = 0;
    echoes::sim::EntityId SeveralVoicesNemeId = 0;
    echoes::sim::EntityId SeveralVoicesResearchLoomId = 0;
    echoes::sim::EntityId SeveralVoicesPhaseAnchorId = 0;
    echoes::sim::ChoirIdentityState SeveralVoicesPossibleState =
        echoes::sim::ChoirIdentityState::NotChoir;
    echoes::sim::ChoirIdentityState SeveralVoicesManifestState =
        echoes::sim::ChoirIdentityState::NotChoir;
    uint64 SeveralVoicesPossibleResolveTicksRemaining = 0;
    uint64 SeveralVoicesManifestResolveTicksRemaining = 0;
    bool bSeveralVoicesHeldAlternativesResearched = false;
    bool bSeveralVoicesPossibleAtSite = false;
    bool bSeveralVoicesManifestAtSite = false;
    bool bSeveralVoicesNemeAtCommandSite = false;
    bool bSeveralVoicesSharedResolutionResearched = false;
    bool bSeveralVoicesPhaseAnchorComplete = false;
    uint64 SeveralVoicesCrisisTicksRemaining = 0;
    bool bSeveralVoicesCrisisWindowHeld = false;
    EEchoesBrokenSunPhase BrokenSunPhase =
        EEchoesBrokenSunPhase::Inactive;
    EEchoesFinalResolution BrokenSunPendingFinalResolution =
        EEchoesFinalResolution::None;
    EEchoesFinalResolution BrokenSunFinalResolution =
        EEchoesFinalResolution::None;
    uint8 BrokenSunAvailableFinalResolutions = 0;
    echoes::sim::EntityId BrokenSunAccordVoiceId = 0;
    echoes::sim::EntityId BrokenSunAccordHeavyId = 0;
    echoes::sim::EntityId BrokenSunNemeId = 0;
    echoes::sim::EntityId BrokenSunWorkerId = 0;
    echoes::sim::EntityId BrokenSunMaraId = 0;
    echoes::sim::EntityId BrokenSunOruunId = 0;
    echoes::sim::EntityId BrokenSunTalarId = 0;
    echoes::sim::EntityId BrokenSunApproachAnchorId = 0;
    echoes::sim::EntityId BrokenSunResolutionConduitId = 0;
    bool bBrokenSunApproachSecured = false;
    bool bBrokenSunMeridianAccordEstablished = false;
    bool bBrokenSunKharuunAccordEstablished = false;
    bool bBrokenSunChoirAccordEstablished = false;
    bool bBrokenSunResolutionConduitComplete = false;
    uint64 BrokenSunResolutionTicksRemaining = 0;
    bool bBrokenSunResolutionWindowHeld = false;
    bool bBrokenSunResolutionContractFailed = false;
};

/** Scalar-only presentation-pool evidence; never participates in simulation. */
struct FEchoesPresentationPoolStats final
{
    int32 ActiveEntityViews = 0;
    int32 FreeEntityViews = 0;
    int32 EntityFreeCapacity = 0;
    int32 ActiveDestructionViews = 0;
    int32 FreeDestructionViews = 0;
    int32 DestructionCapacity = 0;
    int32 LastDestructionOverflowSlot = -1;
    uint64 EntityCreated = 0;
    uint64 EntityReused = 0;
    uint64 EntityReleased = 0;
    uint64 EntityRetentionOverflow = 0;
    uint64 DestructionCreated = 0;
    uint64 DestructionReused = 0;
    uint64 DestructionActivated = 0;
    uint64 DestructionReleased = 0;
    uint64 DestructionOverflow = 0;
    uint64 DestructionCoalesced = 0;
    uint64 EntityOwnedMIDCreated = 0;
    uint64 DestructionOwnedMIDCreated = 0;
    int32 ActiveCombatEffectViews = 0;
    int32 FreeCombatEffectViews = 0;
    int32 CombatEffectCapacity = 0;
    int32 LastCombatEffectOverflowSlot = -1;
    uint64 CombatEffectCreated = 0;
    uint64 CombatEffectReused = 0;
    uint64 CombatEffectActivated = 0;
    uint64 CombatEffectReleased = 0;
    uint64 CombatEffectOverflow = 0;
    uint64 CombatEffectCoalesced = 0;
    uint64 CombatEffectOwnedMIDCreated = 0;
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

    /**
     * Creates the non-Shipping, self-maintaining 400-owned-unit endurance
     * fixture. Deterministic replacement is a subsystem qualification action,
     * not a SimCore command-replay event; this fixture is excluded from save
     * and replay qualification.
     */
    bool StartSustainedStressScenario();

    /** Stops the prototype and releases every disposable presentation view. */
    void StopPrototypeScenario();

    /** Recreates the bounded match from its deterministic initial state. */
    bool RestartPrototypeScenario();

    /** Rebuilds the undeployed skirmish for one of the three playable factions. */
    bool SelectLocalFaction(
        echoes::sim::Faction NewFaction,
        FString& OutFeedback);

    /**
     * Validates and applies one complete offline skirmish deployment. A failed
     * rebuild restores the prior setup and its paused scenario.
     */
    bool ApplySkirmishSetup(
        const FEchoesSkirmishSetup& Setup,
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
    [[nodiscard]] FString GetActiveQuickSavePath() const;

    /** Atomically writes a validated deterministic autosave snapshot and retains one backup. */
    bool AutosaveScenario(EEchoesAutosaveReason Reason, FString& OutFeedback);

    /** The canonical autosave path. */
    [[nodiscard]] static FString GetAutosavePath();

    /** The active scenario's autosave file path inside the save directory. */
    [[nodiscard]] FString GetActiveAutosavePath() const;

    [[nodiscard]] uint64 GetLastAutosavedTick() const { return LastAutosavedTick; }
    [[nodiscard]] EEchoesAutosaveReason GetLastAutosavedReason() const { return LastAutosavedReason; }
    [[nodiscard]] uint8 GetLastAutosavedPhase() const { return LastAutosavedPhase; }

    /** Checks if a recoverable interrupted session checkpoint exists in the save directory. */
    bool CheckInterruptedSessionRecovery(
        FEchoesRecoveryCandidate& OutCandidate,
        FString& OutFeedback) const;

    /** Restores an interrupted session by finding the newest valid candidate checkpoint. */
    bool RecoverInterruptedSession(FString& OutFeedback);

    /** Restores an interrupted session from the candidate checkpoint. */
    bool RecoverInterruptedSession(
        const FEchoesRecoveryCandidate& Candidate,
        FString& OutFeedback);

    /** Dismisses/archives the candidate interrupted session checkpoint. */
    bool DismissInterruptedSession(
        const FEchoesRecoveryCandidate& Candidate,
        FString& OutFeedback);

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

    bool IssueChoirReconciliation(
        uint32 ActorId,
        echoes::sim::ChoirIdentityState StableState,
        FString& OutFeedback);

    /** Arms and then confirms one earned Mission 15 resolution. */
    bool ChooseFinalResolution(
        EEchoesFinalResolution Resolution,
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
    /** Ends a fixed online match by recording one player's deterministic forfeit. */
    bool ForfeitNetworkPlayer(uint8 ForfeitingPlayer, FString& OutFeedback);
    [[nodiscard]] bool IsNetworkHumanOpponentEnabled() const
    {
        return bNetworkHumanOpponent;
    }
    [[nodiscard]] const echoes::sim::Entity* FindEntity(uint32 EntityId) const;
    [[nodiscard]] AEchoesEntityView* FindEntityView(uint32 EntityId) const;
    [[nodiscard]] FEchoesPresentationPoolStats GetPresentationPoolStats() const;
    [[nodiscard]] static int32 GetEntityViewFreePoolCapacity();
    [[nodiscard]] static int32 GetDestructionPoolCapacityForEffectsQuality(
        int32 EffectsQuality);
    [[nodiscard]] AEchoesFogView* GetFogView() const;
    [[nodiscard]] AEchoesTerrainView* GetTerrainView() const;
    [[nodiscard]] FVector SimToWorld(const echoes::sim::Vec2& Position) const;
    [[nodiscard]] echoes::sim::Vec2 WorldToSim(const FVector& Position) const;
    [[nodiscard]] bool IsScenarioReady() const { return bScenarioReady; }
    [[nodiscard]] bool IsStressScenario() const { return bStressScenario; }
    [[nodiscard]] bool IsSustainedStressScenario() const
    {
        return bSustainedStressScenario;
    }
    [[nodiscard]] bool HasSustainedStressFailed() const
    {
        return bSustainedStressFailed;
    }
    [[nodiscard]] bool IsSustainedStressTimingReady() const
    {
        return bSustainedStressTimingReady;
    }
    [[nodiscard]] uint64 GetSustainedStressReplacementCount() const
    {
        return SustainedStressCumulativeReplacements;
    }
    [[nodiscard]] EEchoesOperationMode GetOperationMode() const
    {
        return SelectedOperation;
    }
    [[nodiscard]] FString GetOperationLabel() const;
    /** Resolves the exact next campaign operation from the validated ledger. */
    [[nodiscard]] FEchoesCampaignJourney GetCampaignJourney() const;
    /** Stable failure reason code for the active campaign operation's
     *  authored failure variants; "generic" when no specific cause is
     *  derivable (or the mission's binding is not implemented yet). */
    [[nodiscard]] FString GetMissionFailureReasonCode() const;

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
    [[nodiscard]] EEchoesAssemblyOfTheMissingPhase
    GetAssemblyOfTheMissingPhase() const;
    [[nodiscard]] bool IsAssemblyOfTheMissingUnlocked() const;
    [[nodiscard]] EEchoesSeveralVoicesOneCommandPhase
    GetSeveralVoicesOneCommandPhase() const;
    [[nodiscard]] bool IsSeveralVoicesOneCommandUnlocked() const;
    [[nodiscard]] EEchoesBrokenSunPhase GetBrokenSunPhase() const;
    [[nodiscard]] bool IsBrokenSunUnlocked() const;
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
    [[nodiscard]] FEchoesAssemblyOfTheMissingPlan
    GetAssemblyOfTheMissingPlan() const;
    [[nodiscard]] FEchoesSeveralVoicesOneCommandPlan
    GetSeveralVoicesOneCommandPlan() const;
    [[nodiscard]] FEchoesBrokenSunPlan GetBrokenSunPlan() const;
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
        if (!bStressScenario &&
            SelectedOperation == EEchoesOperationMode::Skirmish)
        {
            return ActiveSkirmishSetup.OpponentFaction;
        }
        switch (GetLocalFaction())
        {
            case echoes::sim::Faction::MeridianCompact:
                return echoes::sim::Faction::KharuunAssemblies;
            case echoes::sim::Faction::KharuunAssemblies:
            case echoes::sim::Faction::HollowChoir:
                return echoes::sim::Faction::MeridianCompact;
        }
        return echoes::sim::Faction::MeridianCompact;
    }
    [[nodiscard]] const FEchoesSkirmishSetup& GetActiveSkirmishSetup() const
    {
        return ActiveSkirmishSetup;
    }
    [[nodiscard]] float GetEffectiveGameSpeedMultiplier() const;
    [[nodiscard]] EEchoesSkirmishDifficulty GetActiveSkirmishDifficulty() const
    {
        return ActiveSkirmishSetup.Difficulty;
    }
    [[nodiscard]] EEchoesSkirmishVictoryCondition GetActiveSkirmishVictoryCondition() const
    {
        return ActiveSkirmishSetup.VictoryCondition;
    }
    [[nodiscard]] EEchoesSkirmishGameSpeed GetActiveSkirmishGameSpeed() const
    {
        return ActiveSkirmishSetup.GameSpeed;
    }
    [[nodiscard]] EEchoesSkirmishTeamSetup GetActiveSkirmishTeamSetup() const
    {
        return ActiveSkirmishSetup.TeamSetup;
    }
    [[nodiscard]] int32 GetMapWidthTiles() const;
    [[nodiscard]] int32 GetMapHeightTiles() const;
#if WITH_DEV_AUTOMATION_TESTS
    /** One-shot fault injection for transactional scenario-transition tests. */
    void FailNextScenarioStartForTesting()
    {
        bFailNextScenarioStartForTesting = true;
    }
    /** One-shot fault injection for checkpoint rename/rotation tests. */
    void FailNextQuickSaveBackupRotationForTesting()
    {
        bFailNextQuickSaveBackupRotationForTesting = true;
    }
#endif

private:
#if WITH_DEV_AUTOMATION_TESTS
    friend class FEchoesPrologueMissionTest;
    friend class FEchoesFreshCampaignJourneyTest;
    friend class FEchoesPresentationPoolingTest;
    friend class FEchoesCombatEffectsTest;
    friend class FEchoesAutosaveRecoveryTest;
#endif
    bool LoadScenarioFromPath(const FString& SavePath, FString& OutFeedback);
    bool ValidateCheckpointFileOnDisk(
        const FString& CandidatePath,
        uint64 ExpectedCampaignBranchIdentity,
        FString& OutFailure) const;
    void CheckPhaseTransitionAutosave();
    [[nodiscard]] uint8 GetCurrentOperationPhase() const;
    [[nodiscard]] static bool IsOperationPhaseTerminal(EEchoesOperationMode Mode, uint8 Phase);
    [[nodiscard]] static FString GetOperationDisplayName(EEchoesOperationMode Mode);
    [[nodiscard]] static FString GetPhaseDisplayName(EEchoesOperationMode Mode, uint8 Phase);
    [[nodiscard]] static bool GetMissionIdForOperation(EEchoesOperationMode Mode, EEchoesCampaignMissionId& OutMissionId);
    static bool InspectSaveContainer(
        const TArray<uint8>& Bytes,
        uint8& OutVersion,
        EEchoesOperationMode& OutOperation,
        echoes::sim::Faction& OutFaction,
        uint64& OutBranchIdentity,
        uint32& OutCrc,
        TArray<uint8>& OutPayload,
        FString& OutError);
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
    EEchoesCampaignCommitStatus CommitAssemblyOfTheMissingCompletion(
        echoes::sim::FutureWellChoice& OutRecordedProtocol,
        FString& OutFeedback);
    EEchoesCampaignCommitStatus CommitSeveralVoicesOneCommandCompletion(
        echoes::sim::FutureWellChoice& OutRecordedProtocol,
        FString& OutFeedback);
    EEchoesCampaignCommitStatus CommitBrokenSunCompletion(
        EEchoesFinalResolution& OutRecordedResolution,
        FString& OutFeedback);
    void AdvancePrologueCompletionPresentation();
    bool StartScenario(
        bool bUseStressScenario,
        bool bUseSustainedStressScenario = false);
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
    void AuditSeveralVoicesOneCommandContractAfterFixedStep();
    void AuditBrokenSunContractAfterFixedStep();
    [[nodiscard]] int64 GetSustainedStressCombatHitPoints() const;
    [[nodiscard]] bool MaintainSustainedStressContractAfterFixedStep(
        int64 CombatHitPointsBeforeStep);
    [[nodiscard]] bool ValidateSustainedStressContract(
        bool bRequireSynchronizedViews,
        bool bRequireRecentActivity,
        bool bEmitHeartbeat);
    [[nodiscard]] bool FindSustainedStressReplacementPosition(
        int32 SlotIndex,
        echoes::sim::Vec2& OutPosition) const;
    void FailSustainedStressContract(
        const TCHAR* Code,
        const FString& Detail);
    [[nodiscard]] AEchoesEntityView* AcquireEntityView();
    void ReleaseEntityView(AEchoesEntityView* View);
    [[nodiscard]] AEchoesDestructionView* AcquireDestructionView(
        uint64 SimulationTick,
        uint32 RemovedEntityId);
    void ReleaseDestructionView(AEchoesDestructionView* View);
    void ReclaimFinishedDestructionViews();
    void ResetDestructionViewsForScenario();
    void DestroyPooledPresentationActors();
    void EmitDestructionPresentation(
        uint32 RemovedEntityId,
        const FVector& WorldLocation,
        echoes::sim::Faction Faction,
        echoes::sim::EntityType EntityType);
    [[nodiscard]] static int32 GetCombatEffectPoolCapacityForEffectsQuality(
        int32 EffectsQuality);
    [[nodiscard]] AEchoesCombatEffectView* AcquireCombatEffectView(
        uint64 SimulationTick,
        uint32 AttackerEntityId);
    void ReleaseCombatEffectView(AEchoesCombatEffectView* View);
    void ReclaimFinishedCombatEffectViews();
    void ResetCombatEffectViewsForScenario();
    void EmitCombatEffectPresentation(
        echoes::sim::Faction Faction,
        echoes::sim::EntityType EntityType,
        const FVector& SourceLocation,
        const FVector& TargetLocation);
    void EmitRuntimeMemoryPoolTelemetry(uint64 Tick, uint64 WallMs);
    void OnPreGarbageCollect();
    void OnPostGarbageCollect();
    bool SyncEntityViews(bool bTeleportNewViews);

    /** Research/production/capacity alert observation over authoritative
     *  local-player state. Baseline-guarded so a fresh scenario or a restored
     *  save re-observes silently instead of firing a burst of alerts.
     *  Presentation only. */
    void UpdateAlertPresentation();

    /** Gameplay-event audio observation over the fair-visibility entity set.
     *  Diffs per-entity authoritative state (attack cooldown reset, damage,
     *  cargo, construction, Well protocol, Reshape window) and local research
     *  state, and reports events to the gameplay-audio subsystem. Entities
     *  entering visibility re-baseline silently, so fog reveals and restored
     *  saves never fire a burst. Presentation only. */
    void UpdateGameplayAudioPresentation(
        const TArray<const echoes::sim::Entity*>& VisibleEntities);

    /** The current mission phase as its authored contract name; empty for
     *  Skirmish. */
    [[nodiscard]] FString CurrentMissionPhaseName() const;

    /** Fires authored mid-mission lines on real phase transitions. Terminal
     *  phases are owned by the result flow; scenario changes and restored
     *  saves re-baseline silently. Presentation only. */
    void UpdateNarrativeDispatch();

    /** Drives the tension and combat music layers from authoritative state:
     *  tension while hostiles are visible, combat while authoritative damage
     *  is recent, both with hysteresis so layers never flap. Presentation
     *  only. */
    void UpdateThreatMusicPresentation(
        const TArray<const echoes::sim::Entity*>& VisibleEntities);
    bool SpawnFogView();
    bool SyncFogView();
    bool SpawnTerrainView();
    bool SyncTerrainView();
    void SynchronizeSkirmishEnvironmentPresentation();
    void DestroyEntityViews();
    void DestroyFogView();
    void DestroyTerrainView();

    TUniquePtr<echoes::sim::Simulation> Simulation;

    // Alert-observation baseline. Rebuilt silently whenever the simulation
    // instance changes or authoritative time moves backwards.
    const echoes::sim::Simulation* AlertBaselineSimulation = nullptr;
    uint64 AlertLastObservedTick = 0;
    uint32 AlertKnownResearchMask = 0;
    TSet<uint32> AlertKnownOwnedUnitIds;
    bool bAlertCapacityLowLatched = false;

    /** Last observed authoritative state per visible entity, for the
     *  gameplay-audio observer. Entries drop when visibility is lost. */
    struct FGameplayAudioSnapshot final
    {
        int32 HitPoints = 0;
        int32 Cargo = 0;
        int32 ConstructionProgress = 0;
        uint64 AttackCooldownTicks = 0;
        uint64 ReshapeUntilTick = 0;
        echoes::sim::FutureWellChoice WellChoice =
            echoes::sim::FutureWellChoice::Dormant;
        uint8 Owner = echoes::sim::kNeutralPlayer;
        bool bCompleted = true;
        bool bTemporaryMineralCover = false;
    };
    TMap<uint32, FGameplayAudioSnapshot> GameplayAudioSnapshots;
    const echoes::sim::Simulation* GameplayAudioBaselineSimulation = nullptr;
    const echoes::sim::Simulation* NarrativeBaselineSimulation = nullptr;
    FString NarrativeLastPhaseName;
    double LastHostileVisibleSeconds = -1000.0;
    double LastAuthoritativeDamageSeconds = -1000.0;
    echoes::sim::ResearchType GameplayAudioLastActiveResearch =
        echoes::sim::ResearchType::None;
    echoes::sim::ResearchType GameplayAudioLastInterruptedResearch =
        echoes::sim::ResearchType::None;
    TMap<uint32, TWeakObjectPtr<AEchoesEntityView>> EntityViews;
    UPROPERTY(Transient)
    TArray<TObjectPtr<AEchoesEntityView>> FreeEntityViews;
    UPROPERTY(Transient)
    TArray<TObjectPtr<AEchoesDestructionView>> ActiveDestructionViews;
    UPROPERTY(Transient)
    TArray<TObjectPtr<AEchoesDestructionView>> FreeDestructionViews;
    UPROPERTY(Transient)
    TArray<TObjectPtr<AEchoesCombatEffectView>> ActiveCombatEffectViews;
    UPROPERTY(Transient)
    TArray<TObjectPtr<AEchoesCombatEffectView>> FreeCombatEffectViews;
    TWeakObjectPtr<AEchoesFogView> FogView;
    TWeakObjectPtr<AEchoesTerrainView> TerrainView;
    double FixedTimeAccumulator = 0.0;
    uint64 NextPlayerCommandSequence = 1;
    bool bScenarioReady = false;
    uint8 LastAutosavedPhase = 0xFF;
    uint64 LastAutosavedTick = 0;
    EEchoesAutosaveReason LastAutosavedReason = EEchoesAutosaveReason::None;
    bool bSuppressAutosave = false;
#if WITH_DEV_AUTOMATION_TESTS
    bool bFailNextScenarioStartForTesting = false;
    mutable bool bFailNextQuickSaveBackupRotationForTesting = false;
#endif
    bool bWarnedAboutTimeClamp = false;
    bool bLoggedFirstTick = false;
    bool bLoggedStressCombat = false;
    bool bLoggedAiExpansion = false;
    bool bLoggedAiRetreat = false;
    bool bLoggedAiPlayerView = false;
    bool bLoggedAiAdaptation = false;
    bool bLoggedAiMineralCover = false;
    bool bLoggedAiVibrationResponse = false;
    bool bLoggedAssistedAiTelemetry = false;
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
    [[nodiscard]] FEchoesPrologueMissionFacts GatherPrologueFacts() const;
    [[nodiscard]] FEchoesSevenAccountsMissionFacts GatherSevenAccountsFacts() const;
    [[nodiscard]] FEchoesCityReserveMissionFacts GatherCityReserveFacts() const;
    [[nodiscard]] FEchoesUnburiedRoadMissionFacts GatherUnburiedRoadFacts() const;
    [[nodiscard]] FEchoesTermsOfContinuanceMissionFacts
    GatherTermsOfContinuanceFacts() const;
    [[nodiscard]] FEchoesNamesWithoutBirthsMissionFacts
    GatherNamesWithoutBirthsFacts() const;
    [[nodiscard]] FEchoesShapeOfSilenceMissionFacts
    GatherShapeOfSilenceFacts() const;
    [[nodiscard]] FEchoesShapeBesideUsMissionFacts
    GatherShapeBesideUsFacts() const;
    [[nodiscard]] FEchoesReserveAuthorityMissionFacts
    GatherReserveAuthorityFacts() const;
    [[nodiscard]] FEchoesChoirAtLumeReachMissionFacts
    GatherChoirAtLumeReachFacts() const;
    [[nodiscard]] FEchoesNoNeutralLedgerMissionFacts
    GatherNoNeutralLedgerFacts() const;
    [[nodiscard]] FEchoesFutureThatWonMissionFacts
    GatherFutureThatWonFacts() const;
    [[nodiscard]] FEchoesAssemblyOfTheMissingMissionFacts
    GatherAssemblyOfTheMissingFacts() const;
    [[nodiscard]] FEchoesSeveralVoicesOneCommandMissionFacts
    GatherSeveralVoicesOneCommandFacts() const;
    [[nodiscard]] FEchoesBrokenSunMissionFacts GatherBrokenSunFacts() const;
    bool bSimulationPaused = false;
    bool bMatchResultReported = false;
    bool bStressScenario = false;
    bool bSustainedStressScenario = false;
    bool bSustainedStressFailed = false;
    bool bSustainedStressTimingReady = false;
    bool bSustainedStressQualificationLogged = false;
    FString SustainedStressFailureCode;
    TArray<uint32> SustainedStressCombatEntityIds;
    TArray<uint8> SustainedStressCombatOwners;
    TArray<echoes::sim::Faction> SustainedStressCombatFactions;
    TArray<echoes::sim::EntityType> SustainedStressCombatTypes;
    TArray<echoes::sim::Vec2> SustainedStressCombatSpawnPositions;
    std::array<echoes::sim::EntityId, echoes::sim::kMaximumPlayers>
        SustainedStressCommandCoreIds{};
    uint64 SustainedStressIntervalDamage = 0;
    uint64 SustainedStressIntervalCombatLosses = 0;
    uint64 SustainedStressCumulativeCombatLosses = 0;
    uint64 SustainedStressIntervalReplacements = 0;
    uint64 SustainedStressCumulativeReplacements = 0;
    uint64 SustainedStressIntervalOrderRenewals = 0;
    uint64 SustainedStressCumulativeOrderRenewals = 0;
    uint64 SustainedStressLastActivityTick = 0;
    uint64 SustainedStressLastHeartbeatTick = 0;
    uint64 SustainedStressLastHeartbeatWallMs = 0;
    uint32 SustainedStressStartupStableFrames = 0;
    double SustainedStressStartupStableSeconds = 0.0;
    std::array<int32, echoes::sim::kMaximumPlayers>
        SustainedStressRenewalCursorByPlayer{};
    double SustainedStressReadyWallSeconds = 0.0;
    uint64 EntityViewCreatedCount = 0;
    uint64 EntityViewReusedCount = 0;
    uint64 EntityViewReleasedCount = 0;
    uint64 EntityViewRetentionOverflowCount = 0;
    uint64 DestructionViewCreatedCount = 0;
    uint64 DestructionViewReusedCount = 0;
    uint64 DestructionViewActivationCount = 0;
    uint64 DestructionViewReleasedCount = 0;
    uint64 DestructionViewOverflowCount = 0;
    uint64 DestructionViewCoalescedCount = 0;
    uint64 EntityViewOwnedMIDCreationCount = 0;
    uint64 DestructionViewOwnedMIDCreationCount = 0;
    int32 LastDestructionOverflowSlot = -1;
    uint64 CombatEffectViewCreatedCount = 0;
    uint64 CombatEffectViewReusedCount = 0;
    uint64 CombatEffectViewActivationCount = 0;
    uint64 CombatEffectViewReleasedCount = 0;
    uint64 CombatEffectViewOverflowCount = 0;
    uint64 CombatEffectViewCoalescedCount = 0;
    uint64 CombatEffectViewOwnedMIDCreationCount = 0;
    int32 LastCombatEffectOverflowSlot = -1;
    FDelegateHandle PreGarbageCollectHandle;
    FDelegateHandle PostGarbageCollectHandle;
    uint64 NaturalGarbageCollectionCount = 0;
    uint64 LastGcPreUsedPhysicalBytes = 0;
    uint64 LastGcPostUsedPhysicalBytes = 0;
    int64 LastGcUsedPhysicalDeltaBytes = 0;
    int32 LastGcPreObjectSlots = 0;
    int32 LastGcPreClaimedObjectSlots = 0;
    int64 LastGcObjectSlotDelta = 0;
    int64 LastGcClaimedObjectSlotDelta = 0;
    uint64 LastRuntimeMemoryTelemetryTick = MAX_uint64;
    bool bNetworkHumanOpponent = false;
    echoes::sim::Faction LocalFaction =
        echoes::sim::Faction::MeridianCompact;
    EEchoesOperationMode SelectedOperation = EEchoesOperationMode::Skirmish;
    FEchoesSkirmishSetup ActiveSkirmishSetup =
        FEchoesSkirmishSetupModel::DefaultSetup();
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
    echoes::sim::EntityId AssemblyOruunId = 0;
    echoes::sim::EntityId AssemblyVerifierId = 0;
    echoes::sim::EntityId AssemblyMeridianPublicRecordInterfaceId = 0;
    echoes::sim::EntityId AssemblyKharuunPublicRecordInterfaceId = 0;
    echoes::sim::EntityId AssemblyCrownfallIndexInterfaceId = 0;
    echoes::sim::EntityId SeveralVoicesPossibleVoiceId = 0;
    echoes::sim::EntityId SeveralVoicesManifestVoiceId = 0;
    echoes::sim::EntityId SeveralVoicesNemeId = 0;
    echoes::sim::EntityId SeveralVoicesResearchLoomId = 0;
    bool bSeveralVoicesCrisisHoldStarted = false;
    bool bSeveralVoicesCrisisContractFailed = false;
    echoes::sim::EntityId BrokenSunAccordVoiceId = 0;
    echoes::sim::EntityId BrokenSunAccordHeavyId = 0;
    echoes::sim::EntityId BrokenSunNemeId = 0;
    echoes::sim::EntityId BrokenSunWorkerId = 0;
    echoes::sim::EntityId BrokenSunMaraId = 0;
    echoes::sim::EntityId BrokenSunOruunId = 0;
    echoes::sim::EntityId BrokenSunTalarId = 0;
    echoes::sim::EntityId BrokenSunApproachAnchorId = 0;
    echoes::sim::EntityId BrokenSunResolutionConduitId = 0;
    EEchoesFinalResolution PendingBrokenSunResolution =
        EEchoesFinalResolution::None;
    EEchoesFinalResolution SelectedBrokenSunResolution =
        EEchoesFinalResolution::None;
    bool bBrokenSunResolutionHoldStarted = false;
    bool bBrokenSunResolutionContractFailed = false;
    uint64 BrokenSunResolutionStartTick = 0;
    FEchoesCampaignProgress CampaignProgress;
    FEchoesCampaignProgress CampaignBackupProgress;
    bool bCampaignProgressAvailable = false;
    bool bCampaignBackupAvailable = false;
    FString CampaignProgressPath;
    echoes::sim::ResearchType ResearchPresentationTechnology =
        echoes::sim::ResearchType::None;
};
