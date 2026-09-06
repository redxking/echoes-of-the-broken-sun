#pragma once

#include "CoreMinimal.h"
#include "EchoesSimCore/Simulation.h"

#include <cstddef>
#include <optional>

enum class EEchoesReplayOperationType : uint8
{
    Skirmish = 0,
    Campaign = 1,
};

enum class EEchoesReplayPerspective : uint8
{
    Player0 = 0,
    Player1 = 1,
    Player2 = 2,
    Player3 = 3,
    OmniscientObserver = 4,
};

enum class EEchoesReplaySpeed : uint8
{
    Half = 0,
    Normal = 1,
    Double = 2,
    Quadruple = 3,
    Octuple = 4,
};

enum class EEchoesReplayOperationResult : uint8
{
    Unknown = 0,
    Player0Victory = 1,
    Player1Victory = 2,
    Draw = 3,
    Player2Victory = 4,
    Player3Victory = 5,
    CampaignSuccess = 6,
    CampaignFailure = 7,
};

enum class EEchoesReplayOutcomeCause : uint8
{
    None = 0,
    CommandCoreLoss = 1,
    PlayerForfeit = 2,
    CampaignObjectivesComplete = 3,
    CampaignFailurePredicate = 4,
};

enum class EEchoesCheckpointReplayBindingRead : uint8
{
    LegacyUnbound = 0,
    Bound = 1,
    Invalid = 2,
};

enum class EEchoesReplayArchiveState : uint8
{
    Idle = 0,
    Pending = 1,
    Succeeded = 2,
    Failed = 3,
};

/** Browser-visible metadata. Gameplay truth remains in ReplayRecord. */
struct ECHOESOFTHEBROKENSUN_API FEchoesReplayMetadata final
{
    FString ReplayId;
    FString MapId;
    /** Stable authored operation identifier; independent from current selection. */
    FString OperationId;
    FString BuildIdentity;
    /** Canonical current rules-pack identity used for runtime compatibility. */
    FString RulesIdentity;
    /** SHA-256 of the complete authoritative initial snapshot, re-derived on admission. */
    FString ContentIdentity;
    FDateTime RecordedUtc = FDateTime::MinValue();
    EEchoesReplayOperationType OperationType =
        EEchoesReplayOperationType::Skirmish;
    bool bOperationCompleted = false;
    uint64 CoverageStartTick = 0;
    /** Full operation elapsed time from tick zero. */
    uint64 DurationTicks = 0;
    /** Interval for which reconstructed statistics are authoritative. */
    uint64 StatisticsCoverageTicks = 0;
    echoes::sim::MatchOutcome Outcome = echoes::sim::MatchOutcome::Ongoing;
    EEchoesReplayOperationResult OperationResult =
        EEchoesReplayOperationResult::Unknown;
    EEchoesReplayOutcomeCause OutcomeCause =
        EEchoesReplayOutcomeCause::None;
    /** Stable authored cause/predicate identifier for exact result copy. */
    FString OutcomeReasonId;
    /** Campaign ledger record committed by live play; never applied by replay. */
    FString IrreversibleRecordId;
    uint64 FinalChecksum = 0;
    TArray<echoes::sim::Faction> PlayerFactions;
    FString FilePath;
};

struct ECHOESOFTHEBROKENSUN_API FEchoesReplayEnvelope final
{
    FEchoesReplayMetadata Metadata;
    echoes::sim::ReplayRecord Replay;
    echoes::sim::MatchReport Report;
};

struct ECHOESOFTHEBROKENSUN_API FEchoesReplayBrowserFilter final
{
    FString MapId;
    TOptional<FDateTime> EarliestUtc;
    TOptional<FDateTime> LatestUtc;
};

/** Cooperative validation cancellation; true refuses partial publication. */
using FEchoesReplayCancellationCheck = TFunction<bool()>;

struct ECHOESOFTHEBROKENSUN_API FEchoesReplayPlaybackState final
{
    bool bActive = false;
    bool bPaused = true;
    uint64 CurrentTick = 0;
    uint64 FinalTick = 0;
    EEchoesReplaySpeed Speed = EEchoesReplaySpeed::Normal;
    EEchoesReplayPerspective Perspective = EEchoesReplayPerspective::Player0;
    FString Error;
};

struct ECHOESOFTHEBROKENSUN_API FEchoesReplayArchiveResult final
{
    uint64 Generation = 0;
    bool bFinalized = false;
    bool bSucceeded = false;
    FString Path;
    FString Error;
    FEchoesReplayEnvelope Envelope;
};

/**
 * Publishes only the newest match result. Disk failure cannot erase a report
 * that was already derived from a valid authoritative replay.
 */
class ECHOESOFTHEBROKENSUN_API FEchoesReplayResultAuthority final
{
public:
    [[nodiscard]] uint64 BeginResult();
    [[nodiscard]] bool Publish(FEchoesReplayArchiveResult&& Result);
    [[nodiscard]] uint64 GetGeneration() const { return Generation; }
    [[nodiscard]] EEchoesReplayArchiveState GetState() const { return State; }
    [[nodiscard]] const FString& GetError() const { return Error; }
    [[nodiscard]] const FEchoesReplayEnvelope* GetCompleted() const
    {
        return Completed.IsSet() ? &Completed.GetValue() : nullptr;
    }

private:
    uint64 Generation = 0;
    EEchoesReplayArchiveState State = EEchoesReplayArchiveState::Idle;
    FString Error;
    TOptional<FEchoesReplayEnvelope> Completed;
};

/** Canonical, checksummed replay envelope and transactional disk store. */
class ECHOESOFTHEBROKENSUN_API FEchoesMatchReplayStore final
{
public:
    static constexpr uint16 SchemaVersion = 4;

    [[nodiscard]] static FString GetReplayDirectory();

    /**
     * Canonical validated replay-prefix blob for embedding beside a mid-match
     * snapshot. After load, pass the decoded prefix to
     * Simulation::ContinueReplayRecording before accepting new commands.
     */
    static bool EncodeReplayRecord(
        const echoes::sim::ReplayRecord& Replay,
        TArray<uint8>& OutBytes,
        FString& OutError);
    static bool DecodeReplayRecord(
        const TArray<uint8>& Bytes,
        echoes::sim::ReplayRecord& OutReplay,
        FString& OutError);

    /**
     * CRC-binds the replay prefix to an otherwise opaque checkpoint payload.
     * The binding is placed inside the campaign-map wrapper by the subsystem,
     * so mission and quick-save schemas remain unchanged.
     */
    static bool BindCheckpointPayload(
        const TArray<uint8>& CheckpointPayload,
        const echoes::sim::ReplayRecord& ReplayPrefix,
        TArray<uint8>& OutBoundPayload,
        FString& OutError);
    static bool BindCheckpointPayload(
        const TArray<uint8>& CheckpointPayload,
        const echoes::sim::ReplayRecord& ReplayPrefix,
        TArray<uint8>& OutBoundPayload,
        TArray<uint8>& OutValidatedReplayBytes,
        FString& OutError);

    /**
     * Trusted generated-write verification: parses only the outer checkpoint
     * binding and requires the embedded replay bytes to exactly match bytes
     * already returned by BindCheckpointPayload. Untrusted load and recovery
     * must continue to use ExtractCheckpointPayload.
     */
    static bool ExtractGeneratedCheckpointPayload(
        const TArray<uint8>& CandidatePayload,
        const TArray<uint8>& ExpectedReplayBytes,
        TArray<uint8>& OutCheckpointPayload,
        FString& OutError);

    /** Legacy payloads are returned unchanged with LegacyUnbound. */
    [[nodiscard]] static EEchoesCheckpointReplayBindingRead
    ExtractCheckpointPayload(
        const TArray<uint8>& CandidatePayload,
        TArray<uint8>& OutCheckpointPayload,
        echoes::sim::ReplayRecord& OutReplayPrefix,
        FString& OutError);

    /** Validates the complete replay and derives all authoritative metadata. */
    static bool FinalizeEnvelope(
        const FEchoesReplayMetadata& RequestedMetadata,
        const echoes::sim::ReplayRecord& Replay,
        FEchoesReplayEnvelope& OutEnvelope,
        FString& OutError,
        const FEchoesReplayCancellationCheck& ShouldCancel = {});

    static bool Encode(
        const FEchoesReplayEnvelope& Envelope,
        TArray<uint8>& OutBytes,
        FString& OutError);

    static bool Decode(
        const TArray<uint8>& Bytes,
        FEchoesReplayEnvelope& OutEnvelope,
        FString& OutError,
        const FEchoesReplayCancellationCheck& ShouldCancel = {});

    /** Writes `<ReplayId>.echoesreplay` through a validated temporary file. */
    static bool SaveAtomic(
        const FString& Directory,
        const FEchoesReplayEnvelope& Envelope,
        FString& OutPath,
        FString& OutError);

    static bool Load(
        const FString& Path,
        FEchoesReplayEnvelope& OutEnvelope,
        FString& OutError,
        const FEchoesReplayCancellationCheck& ShouldCancel = {});

    /** Corrupt entries are omitted and returned as attributable browser errors. */
    [[nodiscard]] static TArray<FEchoesReplayMetadata> ListMetadata(
        const FString& Directory,
        const FEchoesReplayBrowserFilter& Filter,
        TArray<FString>& OutErrors,
        const FEchoesReplayCancellationCheck& ShouldCancel = {});
};

/**
 * Detached, read-only replay transport. One cadence advance represents one
 * normal 20 Hz playback opportunity; speed only changes whole tick requests.
 */
class ECHOESOFTHEBROKENSUN_API FEchoesReplayPlaybackSession final
{
public:
    /** Shipping entry: refuses replay metadata from another build/content identity. */
    bool Initialize(
        const FEchoesReplayEnvelope& Envelope,
        const FString& ExpectedBuildIdentity,
        const FString& ExpectedRulesIdentity,
        const FString& ExpectedContentIdentity,
        FString& OutError,
        const FEchoesReplayCancellationCheck& ShouldCancel = {});
    void SetPaused(bool bInPaused) { bPaused = bInPaused; }
    [[nodiscard]] bool IsPaused() const { return bPaused; }
    void SetSpeed(EEchoesReplaySpeed InSpeed);
    [[nodiscard]] EEchoesReplaySpeed GetSpeed() const { return Speed; }
    bool SetPerspective(
        EEchoesReplayPerspective InPerspective,
        FString& OutError);
    [[nodiscard]] EEchoesReplayPerspective GetPerspective() const
    {
        return Perspective;
    }

    /** Advances according to the selected 0.5x/1x/2x/4x/8x cadence. */
    bool AdvanceOneCadence(FString& OutError);
    /** Advances exactly one authoritative tick while paused. */
    bool StepForward(FString& OutError);
    /** Rebuilds from the baseline and advances to the requested absolute tick. */
    bool Seek(
        uint64 Tick,
        FString& OutError,
        const FEchoesReplayCancellationCheck& ShouldCancel = {});

    [[nodiscard]] const echoes::sim::Simulation* GetSimulation() const;
    [[nodiscard]] std::optional<echoes::sim::PlayerView> GetPlayerView() const;
    [[nodiscard]] uint64 GetCurrentTick() const;
    [[nodiscard]] uint64 GetFinalTick() const { return ReplayRecord.finalTick; }

private:
    bool InitializeRecord(
        const echoes::sim::ReplayRecord& Replay,
        std::optional<echoes::sim::Simulation> Baseline,
        FString& OutError,
        const FEchoesReplayCancellationCheck& ShouldCancel);
    bool RebuildAt(
        uint64 Tick,
        FString& OutError,
        const FEchoesReplayCancellationCheck& ShouldCancel = {},
        std::optional<echoes::sim::Simulation> Baseline = std::nullopt);
    bool AdvanceTicks(uint64 TickCount, FString& OutError);

    echoes::sim::ReplayRecord ReplayRecord;
    std::optional<echoes::sim::Simulation> Simulation;
    EEchoesReplayPerspective Perspective =
        EEchoesReplayPerspective::Player0;
    EEchoesReplaySpeed Speed = EEchoesReplaySpeed::Normal;
    bool bPaused = true;
    uint8 HalfSpeedPhase = 0;
    std::size_t NextCommandIndex = 0;
};
