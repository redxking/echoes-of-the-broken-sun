#pragma once

#include "CoreMinimal.h"
#include "EchoesSimCore/Simulation.h"

enum class EEchoesCampaignMissionId : uint8
{
    WhatTheLedgerKeeps = 1,
    SevenAccountsOfRain = 2
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
};
