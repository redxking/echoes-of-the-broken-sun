#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesCheckpointWorker.h"
#include "EchoesCampaignProgress.h"
#include "EchoesTestSaveEnvironment.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTLS.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Templates/Atomic.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesCheckpointWorkerTest,
    "Echoes.Runtime.Persistence.ImmutableCheckpointWorker",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

namespace
{
FEchoesCheckpointWriteRequest MakeWorkerRequest(
    uint64 RequestId,
    const FString& SavePath,
    uint8 Generation,
    const TSharedRef<TAtomic<uint32>, ESPMode::ThreadSafe>& WorkerThreadId)
{
    FEchoesCheckpointWriteRequest Request;
    Request.RequestId = RequestId;
    Request.SavePath = SavePath;
    Request.SimulationTick = RequestId * 100;
    Request.Encode = [Generation, WorkerThreadId]
    {
        WorkerThreadId->Store(FPlatformTLS::GetCurrentThreadId());
        FEchoesCheckpointEncodeResult Encoded;
        Encoded.Bytes = {'E', 'C', 'H', 'O', Generation};
        return Encoded;
    };
    Request.ValidateGenerated = [](
        const FEchoesCheckpointEncodeResult& Encoded,
        FString& OutFailure)
    {
        const TArray<uint8>& Bytes = Encoded.Bytes;
        const bool bValid =
            Bytes.Num() == 5 && Bytes[0] == 'E' && Bytes[1] == 'C' &&
            Bytes[2] == 'H' && Bytes[3] == 'O' && Bytes[4] != 0xFF;
        if (!bValid)
        {
            OutFailure = TEXT("synthetic generated checkpoint failed validation");
        }
        return bValid;
    };
    Request.Validate = [](const TArray<uint8>& Bytes, FString& OutFailure)
    {
        const bool bValid =
            Bytes.Num() == 5 && Bytes[0] == 'E' && Bytes[1] == 'C' &&
            Bytes[2] == 'H' && Bytes[3] == 'O' && Bytes[4] != 0xFF;
        if (!bValid)
        {
            OutFailure = TEXT("synthetic checkpoint failed validation");
        }
        return bValid;
    };
    return Request;
}
}

bool FEchoesCheckpointWorkerTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    FEchoesScopedTestSaveEnvironment Saves(*this);
    if (!Saves.IsReady())
    {
        return false;
    }

    const FString SavePath = FPaths::Combine(
        FEchoesCampaignProgressStore::GetSaveGameDirectory(),
        TEXT("WorkerCheckpoint.bin"));
    const uint32 GameThreadId = FPlatformTLS::GetCurrentThreadId();
    const TSharedRef<TAtomic<uint32>, ESPMode::ThreadSafe> WorkerThreadId =
        MakeShared<TAtomic<uint32>, ESPMode::ThreadSafe>(0);

    FEchoesCheckpointCoordinator Coordinator;
    FString Failure;

    const FString TrustedPath = FPaths::Combine(
        FEchoesCampaignProgressStore::GetSaveGameDirectory(),
        TEXT("WorkerTrustedCheckpoint.bin"));
    const TSharedRef<TAtomic<uint32>, ESPMode::ThreadSafe>
        GeneratedValidationCount =
            MakeShared<TAtomic<uint32>, ESPMode::ThreadSafe>(0);
    const TSharedRef<TAtomic<uint32>, ESPMode::ThreadSafe>
        UntrustedValidationCount =
            MakeShared<TAtomic<uint32>, ESPMode::ThreadSafe>(0);
    FEchoesCheckpointWriteRequest TrustedRequest =
        MakeWorkerRequest(10, TrustedPath, 10, WorkerThreadId);
    TrustedRequest.ValidateGenerated = [GeneratedValidationCount](
        const FEchoesCheckpointEncodeResult& Encoded,
        FString& OutFailure)
    {
        ++(*GeneratedValidationCount);
        if (Encoded.Bytes != TArray<uint8>({'E', 'C', 'H', 'O', 10}))
        {
            OutFailure = TEXT("generated proof did not match encoded bytes");
            return false;
        }
        return true;
    };
    TrustedRequest.Validate = [UntrustedValidationCount](
        const TArray<uint8>&,
        FString& OutFailure)
    {
        ++(*UntrustedValidationCount);
        OutFailure = TEXT("untrusted semantic validation was unexpected");
        return false;
    };
    TestTrue(TEXT("Trusted generated checkpoint is accepted"),
        Coordinator.Enqueue(MoveTemp(TrustedRequest), Failure));
    TArray<FEchoesCheckpointWriteResult> Results;
    Coordinator.WaitForAll(Results);
    TestTrue(TEXT("Trusted generated checkpoint commits"),
        Results.Num() == 1 && Results[0].bSucceeded);
    TestEqual(TEXT("Generated checkpoint receives one structural validation"),
        GeneratedValidationCount->Load(), 1U);
    TestEqual(TEXT("Byte-identical temporary readback avoids semantic replay"),
        UntrustedValidationCount->Load(), 0U);

    TestTrue(
        TEXT("First immutable generation is accepted"),
        Coordinator.Enqueue(
            MakeWorkerRequest(1, SavePath, 1, WorkerThreadId), Failure));
    TestTrue(
        TEXT("Second immutable generation queues behind the first"),
        Coordinator.Enqueue(
            MakeWorkerRequest(2, SavePath, 2, WorkerThreadId), Failure));
    TestTrue(TEXT("Coordinator reports pending work"), Coordinator.HasWork());

    Results.Reset();
    Coordinator.WaitForAll(Results);
    TestEqual(TEXT("Both ordered writes complete"), Results.Num(), 2);
    TestTrue(
        TEXT("Both ordered writes report success"),
        Results.Num() == 2 && Results[0].bSucceeded &&
            Results[1].bSucceeded);
    TestNotEqual(
        TEXT("Encoding ran away from the game thread"),
        WorkerThreadId->Load(),
        GameThreadId);

    TArray<uint8> Primary;
    TArray<uint8> Backup;
    TestTrue(
        TEXT("Newest generation is the primary"),
        FFileHelper::LoadFileToArray(Primary, *SavePath) &&
            Primary == TArray<uint8>({'E', 'C', 'H', 'O', 2}));
    TestTrue(
        TEXT("Prior generation rotated to backup"),
        FFileHelper::LoadFileToArray(Backup, *(SavePath + TEXT(".bak"))) &&
            Backup == TArray<uint8>({'E', 'C', 'H', 'O', 1}));

    FEchoesCheckpointWriteRequest Deferred =
        MakeWorkerRequest(3, SavePath, 3, WorkerThreadId);
    Deferred.bForceBackupRotationFailure = true;
    TestTrue(
        TEXT("Rotation-failure generation is accepted"),
        Coordinator.Enqueue(MoveTemp(Deferred), Failure));
    Results.Reset();
    Coordinator.WaitForAll(Results);
    TestTrue(
        TEXT("Primary commits while failed rotation is reported explicitly"),
        Results.Num() == 1 && Results[0].bSucceeded &&
            Results[0].bBackupRotationDeferred);
    TestTrue(
        TEXT("Prior valid primary remains staged after rotation failure"),
        FFileHelper::LoadFileToArray(
            Backup, *(SavePath + TEXT(".bak.tmp"))) &&
            Backup == TArray<uint8>({'E', 'C', 'H', 'O', 2}));

    FEchoesCheckpointWriteRequest Invalid =
        MakeWorkerRequest(4, SavePath, 0xFF, WorkerThreadId);
    TestTrue(
        TEXT("Invalid generation is accepted for background validation"),
        Coordinator.Enqueue(MoveTemp(Invalid), Failure));
    Results.Reset();
    Coordinator.WaitForAll(Results);
    TestTrue(
        TEXT("Invalid generation reports failure"),
        Results.Num() == 1 && !Results[0].bSucceeded &&
            Results[0].Feedback.Contains(TEXT("SAVE_VALIDATION_FAILED")));
    TestTrue(
        TEXT("Validation failure preserves the committed primary"),
        FFileHelper::LoadFileToArray(Primary, *SavePath) &&
            Primary == TArray<uint8>({'E', 'C', 'H', 'O', 3}));

    const FString ShutdownPath = FPaths::Combine(
        FEchoesCampaignProgressStore::GetSaveGameDirectory(),
        TEXT("WorkerShutdown.bin"));
    {
        FEchoesCheckpointCoordinator ShutdownCoordinator;
        TestTrue(
            TEXT("Shutdown-bound generation is accepted"),
            ShutdownCoordinator.Enqueue(
                MakeWorkerRequest(
                    5, ShutdownPath, 5, WorkerThreadId),
                Failure));
    }
    TestTrue(
        TEXT("Coordinator shutdown drains its accepted generation"),
        FFileHelper::LoadFileToArray(Primary, *ShutdownPath) &&
            Primary == TArray<uint8>({'E', 'C', 'H', 'O', 5}));
    return true;
}

#endif
