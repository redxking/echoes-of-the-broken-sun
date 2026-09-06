#include "EchoesCheckpointWorker.h"

#include "Async/Async.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
[[nodiscard]] bool AtomicReplaceFile(
    const FString& Destination,
    const FString& Source)
{
#if PLATFORM_MAC
    return FPlatformFileManager::Get()
        .GetPlatformFile()
        .MoveFile(*Destination, *Source);
#else
    return IFileManager::Get().Move(
        *Destination, *Source, true, true, true, true);
#endif
}

[[nodiscard]] uint64 ElapsedMicroseconds(double StartedSeconds)
{
    return static_cast<uint64>(FMath::Max(
        0.0,
        (FPlatformTime::Seconds() - StartedSeconds) * 1'000'000.0));
}
}

FEchoesCheckpointCoordinator::~FEchoesCheckpointCoordinator()
{
    TArray<FEchoesCheckpointWriteResult> Ignored;
    WaitForAll(Ignored);
}

bool FEchoesCheckpointCoordinator::Enqueue(
    FEchoesCheckpointWriteRequest&& Request,
    FString& OutFailure)
{
    OutFailure.Reset();
    if (Request.SavePath.IsEmpty() || !Request.Encode ||
        !Request.ValidateGenerated || !Request.Validate)
    {
        OutFailure = TEXT("[SAVE_REQUEST_INVALID] The checkpoint request is incomplete.");
        return false;
    }
    if (Queued.Num() >= 8)
    {
        OutFailure = TEXT("[SAVE_QUEUE_FULL] The checkpoint queue is busy; retry after the pending save completes.");
        return false;
    }
    Queued.Add(MoveTemp(Request));
    LaunchNext();
    return true;
}

bool FEchoesCheckpointCoordinator::PollCompleted(
    FEchoesCheckpointWriteResult& OutResult)
{
    if (!Active.IsSet() || !Active->IsReady())
    {
        return false;
    }
    OutResult = Active->Get();
    Active.Reset();
    LaunchNext();
    return true;
}

void FEchoesCheckpointCoordinator::WaitForAll(
    TArray<FEchoesCheckpointWriteResult>& OutResults)
{
    while (Active.IsSet() || !Queued.IsEmpty())
    {
        LaunchNext();
        if (Active.IsSet())
        {
            OutResults.Add(Active->Get());
            Active.Reset();
        }
    }
}

bool FEchoesCheckpointCoordinator::HasWork() const
{
    return Active.IsSet() || !Queued.IsEmpty();
}

void FEchoesCheckpointCoordinator::LaunchNext()
{
    if (Active.IsSet() || Queued.IsEmpty())
    {
        return;
    }
    FEchoesCheckpointWriteRequest Request = MoveTemp(Queued[0]);
    Queued.RemoveAt(0, 1, EAllowShrinking::No);
    Active.Emplace(Async(
        EAsyncExecution::ThreadPool,
        [Request = MoveTemp(Request)]() mutable
        {
            return Execute(MoveTemp(Request));
        }));
}

FEchoesCheckpointWriteResult FEchoesCheckpointCoordinator::Execute(
    FEchoesCheckpointWriteRequest&& Request)
{
    FEchoesCheckpointWriteResult Result;
    Result.RequestId = Request.RequestId;
    Result.Kind = Request.Kind;
    Result.Reason = Request.Reason;
    Result.Phase = Request.Phase;
    Result.SimulationTick = Request.SimulationTick;
    Result.CaptureMicroseconds = Request.CaptureMicroseconds;
    Result.InitiationMicroseconds = Request.InitiationMicroseconds;
    Result.SavePath = Request.SavePath;

    const double CompletionStarted = FPlatformTime::Seconds();
    if (Request.EnqueuedAtSeconds > 0.0)
    {
        Result.QueueMicroseconds = static_cast<uint64>(FMath::Max(
            0.0,
            (CompletionStarted - Request.EnqueuedAtSeconds) * 1'000'000.0));
    }
    auto Finish = [&Result, &Request, CompletionStarted]() mutable
    {
        Result.CompletionMicroseconds =
            ElapsedMicroseconds(CompletionStarted);
        Result.TotalCompletionMicroseconds =
            Request.RequestStartedSeconds > 0.0
                ? ElapsedMicroseconds(Request.RequestStartedSeconds)
                : Result.CompletionMicroseconds;
        return MoveTemp(Result);
    };
    const double EncodingStarted = FPlatformTime::Seconds();
    FEchoesCheckpointEncodeResult Encoded = Request.Encode();
    Result.EncodingMicroseconds = ElapsedMicroseconds(EncodingStarted);
    if (!Encoded.Error.IsEmpty() || Encoded.Bytes.IsEmpty())
    {
        Result.Feedback = Encoded.Error.IsEmpty()
            ? TEXT("[SAVE_ENCODING_FAILED] The immutable checkpoint could not be encoded.")
            : MoveTemp(Encoded.Error);
        return Finish();
    }

    FString ValidationFailure;
    if (!Request.ValidateGenerated(Encoded, ValidationFailure))
    {
        Result.Feedback = FString::Printf(
            TEXT("[SAVE_VALIDATION_FAILED] %s"),
            ValidationFailure.IsEmpty()
                ? TEXT("The immutable checkpoint did not reopen.")
                : *ValidationFailure);
        return Finish();
    }

#if WITH_DEV_AUTOMATION_TESTS
    if (Request.bForceWriteFailure)
    {
        Result.Feedback = TEXT(
            "[SAVE_WRITE_FAILED_TEST] Forced checkpoint write failure.");
        return Finish();
    }
#endif

    const FString SaveDirectory = FPaths::GetPath(Request.SavePath);
    const FString TemporaryPath = Request.SavePath + TEXT(".tmp");
    const FString BackupPath = Request.SavePath + TEXT(".bak");
    const FString BackupTemporaryPath = BackupPath + TEXT(".tmp");
    IFileManager& Files = IFileManager::Get();
    if (!Files.MakeDirectory(*SaveDirectory, true))
    {
        Result.Feedback = TEXT("[SAVE_DIRECTORY_FAILED] The save directory could not be created.");
        return Finish();
    }

    Files.Delete(*TemporaryPath, false, true, true);
    if (!FFileHelper::SaveArrayToFile(Encoded.Bytes, *TemporaryPath))
    {
        Result.Feedback = TEXT("[SAVE_WRITE_FAILED] The temporary checkpoint could not be written.");
        return Finish();
    }

    TArray<uint8> ReopenedBytes;
    if (!FFileHelper::LoadFileToArray(ReopenedBytes, *TemporaryPath) ||
        ReopenedBytes != Encoded.Bytes)
    {
        Files.Delete(*TemporaryPath, false, true, true);
        Result.Feedback = FString::Printf(
            TEXT("[SAVE_VALIDATION_FAILED] %s"),
            ValidationFailure.IsEmpty()
                ? TEXT("The temporary checkpoint did not reopen byte-for-byte.")
                : *ValidationFailure);
        return Finish();
    }

    const bool bHadPriorPrimary = Files.FileExists(*Request.SavePath);
    bool bPriorPrimaryValid = false;
    bool bExistingBackupValid = false;
    bool bExistingStagedBackupValid = false;
    TArray<uint8> PriorPrimaryBytes;
    TArray<uint8> ExistingBytes;
    if (bHadPriorPrimary &&
        FFileHelper::LoadFileToArray(PriorPrimaryBytes, *Request.SavePath))
    {
        bPriorPrimaryValid =
            Request.Validate(PriorPrimaryBytes, ValidationFailure);
    }
    // A valid prior primary is the recovery generation that will replace the
    // old backup. Existing backup generations only need semantic validation
    // when the primary cannot be trusted and must be preserved as-is.
    if (!bPriorPrimaryValid && Files.FileExists(*BackupPath) &&
        FFileHelper::LoadFileToArray(ExistingBytes, *BackupPath))
    {
        bExistingBackupValid = Request.Validate(ExistingBytes, ValidationFailure);
    }
    ExistingBytes.Reset();
    if (!bPriorPrimaryValid && Files.FileExists(*BackupTemporaryPath) &&
        FFileHelper::LoadFileToArray(ExistingBytes, *BackupTemporaryPath))
    {
        bExistingStagedBackupValid = Request.Validate(
            ExistingBytes, ValidationFailure);
    }

    if (bPriorPrimaryValid)
    {
        Files.Delete(*BackupTemporaryPath, false, true, true);
        if (!FFileHelper::SaveArrayToFile(
                PriorPrimaryBytes, *BackupTemporaryPath))
        {
            Files.Delete(*TemporaryPath, false, true, true);
            Files.Delete(*BackupTemporaryPath, false, true, true);
            Result.Feedback = TEXT("[SAVE_BACKUP_FAILED] The prior validated checkpoint could not be staged safely.");
            return Finish();
        }
        TArray<uint8> StagedBytes;
        if (!FFileHelper::LoadFileToArray(
                StagedBytes, *BackupTemporaryPath) ||
            StagedBytes != PriorPrimaryBytes)
        {
            Files.Delete(*TemporaryPath, false, true, true);
            Files.Delete(*BackupTemporaryPath, false, true, true);
            Result.Feedback = TEXT("[SAVE_BACKUP_VALIDATION_FAILED] The staged recovery checkpoint did not reopen.");
            return Finish();
        }
    }

    if (!AtomicReplaceFile(Request.SavePath, TemporaryPath))
    {
        Files.Delete(*TemporaryPath, false, true, true);
        Result.Feedback = bPriorPrimaryValid
            ? TEXT("[SAVE_COMMIT_FAILED] The new checkpoint was not committed; the prior checkpoint remains active and staged for recovery.")
            : TEXT("[SAVE_COMMIT_FAILED] The new checkpoint was not committed.");
        return Finish();
    }

    if (bPriorPrimaryValid)
    {
        bool bForceRotationFailure = false;
#if WITH_DEV_AUTOMATION_TESTS
        bForceRotationFailure = Request.bForceBackupRotationFailure;
#endif
        if (bForceRotationFailure ||
            !AtomicReplaceFile(BackupPath, BackupTemporaryPath))
        {
            Result.bBackupRotationDeferred = true;
        }
    }

    Result.bSucceeded = true;
    Result.PersistedBytes = Encoded.Bytes.Num();
    if (Result.bBackupRotationDeferred)
    {
        Result.Feedback = TEXT("QUICK SAVE: the new primary committed; backup rotation was deferred and the prior validated checkpoint remains staged for recovery.");
    }
    else if (bHadPriorPrimary && !bPriorPrimaryValid &&
             (bExistingBackupValid || bExistingStagedBackupValid))
    {
        Result.Feedback = FString::Printf(
            TEXT("QUICK SAVE: tick %llu committed; the existing validated recovery checkpoint was preserved."),
            static_cast<unsigned long long>(Result.SimulationTick));
    }
    else if (bHadPriorPrimary && !bPriorPrimaryValid)
    {
        Result.Feedback = FString::Printf(
            TEXT("QUICK SAVE: tick %llu committed; the prior primary was invalid and this checkpoint is the only validated generation."),
            static_cast<unsigned long long>(Result.SimulationTick));
    }
    else if (Request.Kind == EEchoesCheckpointWriteKind::Autosave)
    {
        Result.Feedback = TEXT("[AUTOSAVE_SUCCESS]");
    }
    else
    {
        Result.Feedback = FString::Printf(
            TEXT("QUICK SAVE: tick %llu committed."),
            static_cast<unsigned long long>(Result.SimulationTick));
    }
    return Finish();
}
