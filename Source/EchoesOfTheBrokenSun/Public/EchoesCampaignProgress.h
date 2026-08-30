#pragma once

#include "CoreMinimal.h"
#include "EchoesSimCore/Simulation.h"

enum class EEchoesCampaignMissionId : uint8
{
    WhatTheLedgerKeeps = 1,
    SevenAccountsOfRain = 2,
    ACityOnReserve = 3,
    TheUnburiedRoad = 4,
    TermsOfContinuance = 5,
    NamesWithoutBirths = 6,
    TheShapeOfSilence = 7,
    TheShapeBesideUs = 8,
    ReserveAuthority = 9,
    ChoirAtLumeReach = 10,
    NoNeutralLedger = 11,
    TheFutureThatWon = 12,
    AssemblyOfTheMissing = 13,
    SeveralVoicesOneCommand = 14
};

enum class EEchoesCampaignDecisionFact : uint8
{
    ArchiveRecovered = 1 << 0,
    CarrierEvacuated = 1 << 1,
    LocalCoreSurvived = 1 << 2,
    FutureWellControlled = 1 << 3
};

enum class EEchoesSevenAccountsCompletionFact : uint8
{
    WaystoneRootedAtAnchor = 1 << 0,
    MemoryBearerArrived = 1 << 1,
    LocalCoreSurvived = 1 << 2,
    PriorDecisionConsumed = 1 << 3
};

enum class EEchoesCityReserveCompletionFact : uint8
{
    LifeSupportPowered = 1 << 0,
    TransitPowered = 1 << 1,
    ArchivePowered = 1 << 2,
    LocalCoreSurvived = 1 << 3,
    PriorLedgerConsumed = 1 << 4
};

enum class EEchoesUnburiedRoadCompletionFact : uint8
{
    WaystoneRootedAtRoadhead = 1 << 0,
    ListeningSpineRaised = 1 << 1,
    MemoryShardRecovered = 1 << 2,
    LocalCoreSurvived = 1 << 3,
    PriorLedgerConsumed = 1 << 4
};

enum class EEchoesTermsOfContinuanceCompletionFact : uint8
{
    MeridianRelaySynchronized = 1 << 0,
    KharuunSpineSynchronized = 1 << 1,
    ContinuanceWindowHeld = 1 << 2,
    BothWitnessesExtracted = 1 << 3,
    LocalCoreSurvived = 1 << 4,
    PriorLedgerConsumed = 1 << 5
};

enum class EEchoesNamesWithoutBirthsCompletionFact : uint8
{
    CensusEvidenceLocated = 1 << 0,
    ArchivePowered = 1 << 1,
    BothCiviliansSheltered = 1 << 2,
    EvidenceExtracted = 1 << 3,
    LocalCoreSurvived = 1 << 4,
    PriorLedgerConsumed = 1 << 5
};

enum class EEchoesShapeOfSilenceCompletionFact : uint8
{
    WaystoneRootedAtListeningAnchor = 1 << 0,
    ListeningSpineRaised = 1 << 1,
    BothMemoryWitnessesPositioned = 1 << 2,
    OruunReachedConfluence = 1 << 3,
    LocalCoreSurvived = 1 << 4,
    PriorLedgerConsumed = 1 << 5
};

enum class EEchoesShapeBesideUsCompletionFact : uint8
{
    FirstEchoObserved = 1 << 0,
    EchoRelayRaised = 1 << 1,
    BothStatesTraversed = 1 << 2,
    NemeConvergenceReached = 1 << 3,
    LocalCoreSurvived = 1 << 4,
    PriorLedgerConsumed = 1 << 5
};

enum class EEchoesReserveAuthorityCompletionFact : uint8
{
    LifeSupportPowered = 1 << 0,
    TransitPowered = 1 << 1,
    ArchivePowered = 1 << 2,
    ReserveAuthoritySecured = 1 << 3,
    DeferredDistrictReached = 1 << 4,
    LocalCoreSurvived = 1 << 5,
    PriorLedgerConsumed = 1 << 6
};

enum class EEchoesChoirAtLumeReachCompletionFact : uint8
{
    ContactEstablished = 1 << 0,
    DeferredLiabilityResolved = 1 << 1,
    BothAnchorsRaised = 1 << 2,
    WellChoiceCommitted = 1 << 3,
    BranchResolutionCompleted = 1 << 4,
    OruunSurvived = 1 << 5,
    LocalCoreSurvived = 1 << 6,
    PriorLedgerConsumed = 1 << 7
};

enum class EEchoesNoNeutralLedgerCompletionFact : uint8
{
    InheritedRouteSecured = 1 << 0,
    DistrictPairIntegrated = 1 << 1,
    BothEvidenceChannelsAttested = 1 << 2,
    RecordedProtocolApplied = 1 << 3,
    CoalitionRallied = 1 << 4,
    OruunSurvived = 1 << 5,
    LocalCoreSurvived = 1 << 6,
    PriorLedgerConsumed = 1 << 7
};

enum class EEchoesFutureThatWonCompletionFact : uint8
{
    PriorElevenRecordLedgerConsumed = 1 << 0,
    RecordedLumeProtocolBound = 1 << 1,
    BothRecordedDistrictInputsVerified = 1 << 2,
    IndependentPublicReadbackEstablished = 1 << 3,
    RecordedProtocolActivated = 1 << 4,
    StabilityWindowHeld = 1 << 5,
    BothDistrictReadbacksObserved = 1 << 6,
    LocalCoreSurvived = 1 << 7
};

enum class EEchoesAssemblyOfTheMissingCompletionFact : uint8
{
    PriorTwelveRecordLedgerConsumed = 1 << 0,
    ExistingPlanProjectionBound = 1 << 1,
    RecordedLumeProtocolBound = 1 << 2,
    PriorPublicReceiptsBound = 1 << 3,
    PublicRecordReadbackEstablished = 1 << 4,
    CrownfallIndexLinked = 1 << 5,
    IndependentAssemblyObserved = 1 << 6,
    LocalCoreSurvived = 1 << 7
};

enum class EEchoesSeveralVoicesOneCommandCompletionFact : uint8
{
    PriorThirteenRecordLedgerConsumed = 1 << 0,
    ChoirCommandAuthorityEstablished = 1 << 1,
    HeldAlternativesResearched = 1 << 2,
    IncompatibleVoicesResolved = 1 << 3,
    SharedResolutionResearched = 1 << 4,
    PhaseAnchorRaised = 1 << 5,
    CrisisWindowHeld = 1 << 6,
    LocalCoreSurvived = 1 << 7
};

enum class EEchoesCampaignCommitStatus : uint8
{
    NotApplicable,
    Added,
    AlreadyRecorded,
    ReplayConflict,
    StorageFailure
};

/** One factual, irreversible campaign decision produced by authoritative play. */
struct ECHOESOFTHEBROKENSUN_API FEchoesCampaignDecisionRecord final
{
    EEchoesCampaignMissionId Mission =
        EEchoesCampaignMissionId::WhatTheLedgerKeeps;
    echoes::sim::FutureWellChoice WellChoice =
        echoes::sim::FutureWellChoice::Dormant;
    uint8 AvailableWellChoices = 0;
    uint8 VerifiedFacts = 0;
    uint32 SimulationSnapshotVersion = echoes::sim::kSnapshotVersion;
    uint64 CompletionTick = 0;
    uint64 FinalStateChecksum = 0;

    friend bool operator==(
        const FEchoesCampaignDecisionRecord&,
        const FEchoesCampaignDecisionRecord&) = default;
};

/** In-memory campaign state. Mission records are append-only within a campaign. */
struct ECHOESOFTHEBROKENSUN_API FEchoesCampaignProgress final
{
    static constexpr uint16 SchemaVersion = 1;
    static constexpr int32 MaximumDecisionRecords = 64;

    TArray<FEchoesCampaignDecisionRecord> Decisions;

    [[nodiscard]] const FEchoesCampaignDecisionRecord* FindDecision(
        EEchoesCampaignMissionId Mission) const;

    /**
     * Adds a first decision, accepts an identical-choice replay idempotently,
     * and refuses to rewrite an earlier mission choice.
     */
    EEchoesCampaignCommitStatus AppendDecision(
        const FEchoesCampaignDecisionRecord& Record,
        FString& OutFeedback);
};

/** Versioned, checksummed, transactional campaign-progress persistence. */
class ECHOESOFTHEBROKENSUN_API FEchoesCampaignProgressStore final
{
public:
    [[nodiscard]] static FString GetDefaultPath();

    static bool Encode(
        const FEchoesCampaignProgress& Progress,
        TArray<uint8>& OutBytes,
        FString& OutError);

    static bool Decode(
        const TArray<uint8>& Bytes,
        FEchoesCampaignProgress& OutProgress,
        FString& OutError);

    /** Writes a validated temporary file, retaining one prior generation. */
    static bool SaveAtomic(
        const FString& Path,
        const FEchoesCampaignProgress& Progress,
        FString& OutFeedback);

    /** Loads the primary, falls back to backup, or accepts a wholly absent ledger. */
    static bool LoadWithBackup(
        const FString& Path,
        FEchoesCampaignProgress& OutProgress,
        FString& OutFeedback);

    /** Loads and validates exactly one named generation without fallback. */
    static bool LoadGeneration(
        const FString& Path,
        FEchoesCampaignProgress& OutProgress,
        FString& OutFeedback);
};
