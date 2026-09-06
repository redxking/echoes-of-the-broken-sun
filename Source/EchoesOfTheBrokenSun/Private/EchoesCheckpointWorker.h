#pragma once

#include "CoreMinimal.h"
#include "Async/Future.h"
#include "Templates/Function.h"

enum class EEchoesCheckpointWriteKind : uint8
{
    QuickSave,
    Autosave
};

struct FEchoesCheckpointEncodeResult final
{
    TArray<uint8> Bytes;
    // Exact nested replay bytes whose semantic authority was established by
    // the encoder. Generated-container validation may compare these bytes
    // structurally without replaying the complete history again.
    TArray<uint8> ValidatedReplayBytes;
    uint64 CapturedTick = 0;
    uint64 CapturedChecksum = 0;
    FString Error;
};

struct FEchoesCheckpointWriteRequest final
{
    uint64 RequestId = 0;
    EEchoesCheckpointWriteKind Kind = EEchoesCheckpointWriteKind::QuickSave;
    uint8 Reason = 0;
    uint8 Phase = 0;
    uint64 SimulationTick = 0;
    uint64 CaptureMicroseconds = 0;
    uint64 InitiationMicroseconds = 0;
    double RequestStartedSeconds = 0.0;
    double EnqueuedAtSeconds = 0.0;
    FString SavePath;
    TUniqueFunction<FEchoesCheckpointEncodeResult()> Encode;
    TUniqueFunction<bool(const FEchoesCheckpointEncodeResult&, FString&)>
        ValidateGenerated;
    TUniqueFunction<bool(const TArray<uint8>&, FString&)> Validate;
#if WITH_DEV_AUTOMATION_TESTS
    bool bForceBackupRotationFailure = false;
    bool bForceWriteFailure = false;
#endif
};

struct FEchoesCheckpointWriteResult final
{
    uint64 RequestId = 0;
    EEchoesCheckpointWriteKind Kind = EEchoesCheckpointWriteKind::QuickSave;
    uint8 Reason = 0;
    uint8 Phase = 0;
    uint64 SimulationTick = 0;
    uint64 CaptureMicroseconds = 0;
    uint64 InitiationMicroseconds = 0;
    uint64 QueueMicroseconds = 0;
    uint64 EncodingMicroseconds = 0;
    uint64 CompletionMicroseconds = 0;
    uint64 TotalCompletionMicroseconds = 0;
    int32 PersistedBytes = 0;
    FString SavePath;
    FString Feedback;
    bool bSucceeded = false;
    bool bBackupRotationDeferred = false;
};

/**
 * Owns the single ordered checkpoint write stream. Requests contain only
 * immutable value state and callbacks that must not capture UObjects or live
 * simulation state. All encoding, validation, disk I/O and rotation run on a
 * background thread; results are consumed by the game thread.
 */
class FEchoesCheckpointCoordinator final
{
public:
    FEchoesCheckpointCoordinator() = default;
    ~FEchoesCheckpointCoordinator();

    FEchoesCheckpointCoordinator(const FEchoesCheckpointCoordinator&) = delete;
    FEchoesCheckpointCoordinator& operator=(const FEchoesCheckpointCoordinator&) = delete;

    bool Enqueue(FEchoesCheckpointWriteRequest&& Request, FString& OutFailure);
    bool PollCompleted(FEchoesCheckpointWriteResult& OutResult);
    void WaitForAll(TArray<FEchoesCheckpointWriteResult>& OutResults);
    [[nodiscard]] bool HasWork() const;

private:
    static FEchoesCheckpointWriteResult Execute(FEchoesCheckpointWriteRequest&& Request);
    void LaunchNext();

    TOptional<TFuture<FEchoesCheckpointWriteResult>> Active;
    TArray<FEchoesCheckpointWriteRequest> Queued;
};
