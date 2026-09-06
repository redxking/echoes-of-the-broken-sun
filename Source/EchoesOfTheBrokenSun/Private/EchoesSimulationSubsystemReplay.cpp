#include "EchoesSimulationSubsystem.h"

#include "Async/Async.h"
#include "EchoesCampaignTerrainBinding.h"
#include "EchoesMatchReplay.h"
#include "EchoesNetworkSession.h"
#include "EchoesOfTheBrokenSun.h"
#include "EchoesSkirmishSetup.h"
#include "HAL/PlatformTime.h"
#include "Misc/Guid.h"

namespace
{
template <typename Digest>
FString DigestHex(const Digest& Value)
{
    FString Result;
    Result.Reserve(static_cast<int32>(Value.size() * 2));
    for (const uint8 Byte : Value)
    {
        Result += FString::Printf(TEXT("%02x"), Byte);
    }
    return Result;
}

FString OperationStableId(EEchoesOperationMode Operation)
{
    switch (Operation)
    {
        case EEchoesOperationMode::Skirmish: return TEXT("skirmish");
        case EEchoesOperationMode::CampaignPrologue: return TEXT("m01-what-the-ledger-keeps");
        case EEchoesOperationMode::CampaignSevenAccounts: return TEXT("m02-seven-accounts-of-rain");
        case EEchoesOperationMode::CampaignCityReserve: return TEXT("m03-a-city-on-reserve");
        case EEchoesOperationMode::CampaignUnburiedRoad: return TEXT("m04-the-unburied-road");
        case EEchoesOperationMode::CampaignTermsOfContinuance: return TEXT("m05-terms-of-continuance");
        case EEchoesOperationMode::CampaignNamesWithoutBirths: return TEXT("m06-names-without-births");
        case EEchoesOperationMode::CampaignShapeOfSilence: return TEXT("m07-the-shape-of-silence");
        case EEchoesOperationMode::CampaignShapeBesideUs: return TEXT("m08-the-shape-beside-us");
        case EEchoesOperationMode::CampaignReserveAuthority: return TEXT("m09-reserve-authority");
        case EEchoesOperationMode::CampaignChoirAtLumeReach: return TEXT("m10-choir-at-lume-reach");
        case EEchoesOperationMode::CampaignNoNeutralLedger: return TEXT("m11-no-neutral-ledger");
        case EEchoesOperationMode::CampaignFutureThatWon: return TEXT("m12-the-future-that-won");
        case EEchoesOperationMode::CampaignAssemblyOfTheMissing: return TEXT("m13-assembly-of-the-missing");
        case EEchoesOperationMode::CampaignSeveralVoicesOneCommand: return TEXT("m14-several-voices-one-command");
        case EEchoesOperationMode::CampaignTheBrokenSun: return TEXT("m15-the-broken-sun");
    }
    return TEXT("unknown-operation");
}

FString SkirmishMapStableId(EEchoesSkirmishMapPreset Preset)
{
    switch (Preset)
    {
        case EEchoesSkirmishMapPreset::GlassScar: return TEXT("glass-scar");
        case EEchoesSkirmishMapPreset::CrownfallBasin: return TEXT("crownfall-basin");
        case EEchoesSkirmishMapPreset::SorynConfluence: return TEXT("soryn-confluence");
    }
    return TEXT("unknown-skirmish-map");
}

EEchoesOperationMode OperationFromStableId(const FString& Id)
{
    for (uint8 Raw = static_cast<uint8>(EEchoesOperationMode::Skirmish);
         Raw <= static_cast<uint8>(EEchoesOperationMode::CampaignTheBrokenSun);
         ++Raw)
    {
        const EEchoesOperationMode Candidate =
            static_cast<EEchoesOperationMode>(Raw);
        if (OperationStableId(Candidate) == Id)
        {
            return Candidate;
        }
    }
    return EEchoesOperationMode::Skirmish;
}

FString CampaignMapId(
    EEchoesOperationMode Operation,
    const FEchoesCampaignProgress& Progress)
{
    EEchoesCampaignMissionId Mission{};
    if (!UEchoesSimulationSubsystem::GetMissionIdForOperation(
            Operation, Mission))
    {
        return {};
    }
    const FEchoesCampaignDecisionRecord* Founding = Progress.FindDecision(
        EEchoesCampaignMissionId::WhatTheLedgerKeeps);
    const echoes::sim::FutureWellChoice Doctrine =
        Mission == EEchoesCampaignMissionId::WhatTheLedgerKeeps
            ? echoes::sim::FutureWellChoice::Preserve
            : Founding != nullptr
                ? Founding->WellChoice
                : echoes::sim::FutureWellChoice::Dormant;
    const echoes::world::CampaignTerrainResult Map =
        echoes::world::CheckCampaignTerrain(
            static_cast<uint8>(Mission), Doctrine);
    return Map.ok ? UTF8_TO_TCHAR(Map.map_id) : FString();
}
}

const echoes::sim::MatchReport*
UEchoesSimulationSubsystem::GetCompletedMatchReport() const
{
    const FEchoesReplayEnvelope* Completed =
        ReplayResultAuthority.GetCompleted();
    return Completed != nullptr ? &Completed->Report : nullptr;
}

const FEchoesReplayMetadata*
UEchoesSimulationSubsystem::GetCompletedReplayMetadata() const
{
    const FEchoesReplayEnvelope* Completed =
        ReplayResultAuthority.GetCompleted();
    return Completed != nullptr ? &Completed->Metadata : nullptr;
}

EEchoesReplayArchiveState
UEchoesSimulationSubsystem::GetReplayArchiveState() const
{
    return ReplayResultAuthority.GetState();
}

const FString& UEchoesSimulationSubsystem::GetReplayArchiveError() const
{
    return ReplayResultAuthority.GetError();
}

bool UEchoesSimulationSubsystem::IsReplayPlaybackActive() const
{
    return bReplayPlaybackActive;
}

TArray<FEchoesReplayMetadata> UEchoesSimulationSubsystem::BrowseReplays(
    const FEchoesReplayBrowserFilter& Filter,
    TArray<FString>& OutErrors) const
{
    return FEchoesMatchReplayStore::ListMetadata(
        FEchoesMatchReplayStore::GetReplayDirectory(), Filter, OutErrors);
}

bool UEchoesSimulationSubsystem::BeginLatestReplay(FString& OutFeedback)
{
    TArray<FString> Errors;
    FEchoesReplayBrowserFilter Filter;
    const TArray<FEchoesReplayMetadata> Replays = BrowseReplays(Filter, Errors);
    if (Replays.IsEmpty())
    {
        OutFeedback = Errors.IsEmpty()
            ? TEXT("[REPLAY_NONE] No completed replay is available.")
            : FString::Printf(TEXT("[REPLAY_BROWSER_INVALID] %s"), *Errors[0]);
        return false;
    }
    return BeginReplay(Replays[0].FilePath, OutFeedback);
}

bool UEchoesSimulationSubsystem::BeginCompletedReplay(FString& OutFeedback)
{
    if (ReplayResultAuthority.GetState() == EEchoesReplayArchiveState::Pending)
    {
        OutFeedback = TEXT("[REPLAY_ARCHIVE_PENDING] The completed replay is still being verified.");
        return false;
    }
    const FEchoesReplayEnvelope* Completed =
        ReplayResultAuthority.GetCompleted();
    if (Completed == nullptr || Completed->Metadata.FilePath.IsEmpty())
    {
        OutFeedback = TEXT("[REPLAY_COMPLETED_UNAVAILABLE] This result has no verified replay archive.");
        return false;
    }
    return BeginReplay(Completed->Metadata.FilePath, OutFeedback);
}

bool UEchoesSimulationSubsystem::BeginReplay(
    const FString& Path,
    FString& OutFeedback)
{
    constexpr double ReplayOpenValidationBudgetSeconds = 30.0;
    const double ValidationDeadline =
        FPlatformTime::Seconds() + ReplayOpenValidationBudgetSeconds;
    const FEchoesReplayCancellationCheck ShouldCancel =
        [ValidationDeadline]()
        {
            return FPlatformTime::Seconds() >= ValidationDeadline;
        };
    FEchoesReplayEnvelope Candidate;
    if (!FEchoesMatchReplayStore::Load(
            Path, Candidate, OutFeedback, ShouldCancel))
    {
        return false;
    }
    const EEchoesOperationMode CandidateOperation = OperationFromStableId(
        Candidate.Metadata.OperationId);
    if (OperationStableId(CandidateOperation) !=
        Candidate.Metadata.OperationId)
    {
        OutFeedback = TEXT("[REPLAY_OPERATION_INVALID] Replay operation identity is unsupported.");
        return false;
    }
    EEchoesSkirmishMapPreset CandidateMapPreset =
        EEchoesSkirmishMapPreset::GlassScar;
    if (CandidateOperation == EEchoesOperationMode::Skirmish)
    {
        if (Candidate.Metadata.OperationType !=
            EEchoesReplayOperationType::Skirmish)
        {
            OutFeedback = TEXT("[REPLAY_OPERATION_INVALID] Replay operation type does not match its identity.");
            return false;
        }
        const FString& MapId = Candidate.Metadata.MapId;
        if (MapId == TEXT("glass-scar"))
        {
            CandidateMapPreset = EEchoesSkirmishMapPreset::GlassScar;
        }
        else if (MapId == TEXT("crownfall-basin"))
        {
            CandidateMapPreset = EEchoesSkirmishMapPreset::CrownfallBasin;
        }
        else if (MapId == TEXT("soryn-confluence"))
        {
            CandidateMapPreset = EEchoesSkirmishMapPreset::SorynConfluence;
        }
        else
        {
            OutFeedback = TEXT("[REPLAY_MAP_INVALID] Replay map identity is unsupported.");
            return false;
        }
    }
    else if (Candidate.Metadata.OperationType !=
             EEchoesReplayOperationType::Campaign)
    {
        OutFeedback = TEXT("[REPLAY_OPERATION_INVALID] Replay operation type does not match its identity.");
        return false;
    }
    const echoes::sim::net::CompatibilityManifest Compatibility =
        echoes::network::BuildCompatibilityManifest(Simulation.Get());
    FEchoesReplayPlaybackSession CandidatePlayback;
    if (!CandidatePlayback.Initialize(
            Candidate,
            DigestHex(Compatibility.buildIdSha256),
            DigestHex(Compatibility.rulesPackSha256),
            Candidate.Metadata.ContentIdentity,
            OutFeedback,
            ShouldCancel))
    {
        return false;
    }

    ReplayPlayback = MoveTemp(CandidatePlayback);
    ActiveReplayEnvelope = MoveTemp(Candidate);
    ReplayPresentationOperation = CandidateOperation;
    ReplayPresentationMapPreset = CandidateMapPreset;
    bReplayPlaybackActive = true;
    bReplayPresentationDirty = true;
    ReplayTimeAccumulator = 0.0;
    ReplayPlaybackError.Reset();
    ResetReplayPresentationObservers();
    if (!SpawnTerrainView() || !SpawnFogView() ||
        !CompleteReplayPresentationSync(true, OutFeedback))
    {
        EndReplay();
        OutFeedback = TEXT("[REPLAY_PRESENTATION_FAILED] Replay state was valid but could not be presented.");
        return false;
    }
    SynchronizeSkirmishEnvironmentPresentation();
    OutFeedback = TEXT("Replay opened paused at its first authoritative tick.");
    return true;
}

void UEchoesSimulationSubsystem::EndReplay()
{
    if (!bReplayPlaybackActive)
    {
        return;
    }
    bReplayPlaybackActive = false;
    bReplayPresentationDirty = false;
    ReplayTimeAccumulator = 0.0;
    ReplayPlaybackError.Reset();
    ReplayPlayback = {};
    ActiveReplayEnvelope.Reset();
    ResetReplayPresentationObservers();
    if (bScenarioReady && Simulation.IsValid())
    {
        SpawnTerrainView();
        SpawnFogView();
        SynchronizeSkirmishEnvironmentPresentation();
        SyncEntityViews(true);
    }
}

void UEchoesSimulationSubsystem::SetReplayPaused(bool bPaused)
{
    if (bReplayPlaybackActive)
    {
        ReplayPlayback.SetPaused(bPaused);
    }
}

void UEchoesSimulationSubsystem::SetReplaySpeed(EEchoesReplaySpeed Speed)
{
    if (bReplayPlaybackActive)
    {
        ReplayPlayback.SetSpeed(Speed);
    }
}

bool UEchoesSimulationSubsystem::SetReplayPerspective(
    EEchoesReplayPerspective Perspective,
    FString& OutFeedback)
{
    if (!bReplayPlaybackActive)
    {
        OutFeedback = TEXT("[REPLAY_NOT_ACTIVE] Open a replay before changing perspective.");
        return false;
    }
    if (bReplayPresentationDirty &&
        !CompleteReplayPresentationSync(true, OutFeedback))
    {
        return false;
    }
    if (ReplayPlayback.GetPerspective() == Perspective)
    {
        OutFeedback.Reset();
        return true;
    }
    if (!ReplayPlayback.SetPerspective(Perspective, OutFeedback))
    {
        ReplayPlaybackError = OutFeedback;
        return false;
    }
    ResetReplayPresentationObservers();
    if (!SpawnTerrainView() || !SpawnFogView())
    {
        OutFeedback = TEXT("[REPLAY_PRESENTATION_FAILED] The selected perspective could not be presented.");
        ReplayPlayback.SetPaused(true);
        ReplayPlaybackError = OutFeedback;
        bReplayPresentationDirty = true;
        return false;
    }
    SynchronizeSkirmishEnvironmentPresentation();
    return CompleteReplayPresentationSync(true, OutFeedback);
}

bool UEchoesSimulationSubsystem::StepReplay(FString& OutFeedback)
{
    if (!bReplayPlaybackActive)
    {
        OutFeedback = TEXT("[REPLAY_NOT_ACTIVE] Open a replay before stepping.");
        return false;
    }
    if (bReplayPresentationDirty)
    {
        return CompleteReplayPresentationSync(false, OutFeedback);
    }
    if (!ReplayPlayback.StepForward(OutFeedback))
    {
        ReplayPlaybackError = OutFeedback;
        return false;
    }
    return CompleteReplayPresentationSync(false, OutFeedback);
}

bool UEchoesSimulationSubsystem::SeekReplayTick(
    uint64 Tick,
    FString& OutFeedback)
{
    if (!bReplayPlaybackActive)
    {
        OutFeedback = TEXT("[REPLAY_NOT_ACTIVE] Open a replay before seeking.");
        return false;
    }
    if (bReplayPresentationDirty &&
        !CompleteReplayPresentationSync(true, OutFeedback))
    {
        return false;
    }
    if (ReplayPlayback.GetCurrentTick() == Tick)
    {
        OutFeedback.Reset();
        return true;
    }
    constexpr double ReplaySeekValidationBudgetSeconds = 30.0;
    const double SeekDeadline =
        FPlatformTime::Seconds() + ReplaySeekValidationBudgetSeconds;
    const FEchoesReplayCancellationCheck ShouldCancel = [SeekDeadline]()
    {
        return FPlatformTime::Seconds() >= SeekDeadline;
    };
    if (!ReplayPlayback.Seek(Tick, OutFeedback, ShouldCancel))
    {
        ReplayPlaybackError = OutFeedback;
        return false;
    }
    ReplayTimeAccumulator = 0.0;
    ResetReplayPresentationObservers();
    return CompleteReplayPresentationSync(true, OutFeedback);
}

bool UEchoesSimulationSubsystem::SeekReplayEvent(
    int32 EventIndex,
    FString& OutFeedback)
{
    if (!ActiveReplayEnvelope.IsSet() || EventIndex < 0 ||
        static_cast<size_t>(EventIndex) >=
            ActiveReplayEnvelope->Report.events.size())
    {
        OutFeedback = TEXT("[REPLAY_EVENT_INVALID] The selected timeline event is unavailable.");
        return false;
    }
    return SeekReplayTick(
        ActiveReplayEnvelope->Report.events[EventIndex].tick, OutFeedback);
}

FEchoesReplayPlaybackState
UEchoesSimulationSubsystem::GetReplayPlaybackState() const
{
    FEchoesReplayPlaybackState State;
    State.bActive = bReplayPlaybackActive;
    if (bReplayPlaybackActive)
    {
        State.bPaused = ReplayPlayback.IsPaused();
        State.CurrentTick = ReplayPlayback.GetCurrentTick();
        State.FinalTick = ReplayPlayback.GetFinalTick();
        State.Speed = ReplayPlayback.GetSpeed();
        State.Perspective = ReplayPlayback.GetPerspective();
        State.Error = ReplayPlaybackError;
    }
    return State;
}

void UEchoesSimulationSubsystem::BeginReplayArchiveForCurrentResult()
{
    if (!Simulation.IsValid() || bStressScenario)
    {
        return;
    }

    // A new terminal result supersedes UI ownership of an older archive job,
    // but the older atomic write is allowed to finish in the background.
    if (ReplayArchiveFuture.IsValid())
    {
        if (ReplayArchiveFuture.IsReady())
        {
            ReplayArchiveFuture.Get();
        }
        else
        {
            SupersededReplayArchiveFutures.Add(
                MoveTemp(ReplayArchiveFuture));
        }
        ReplayArchiveFuture = TFuture<FEchoesReplayArchiveResult>();
    }
    ReplayArchiveGeneration = ReplayResultAuthority.BeginResult();

    std::string ExportError;
    echoes::sim::ReplayRecord Replay = Simulation->ExportReplay(&ExportError);
    if (Replay.version == 0)
    {
        FEchoesReplayArchiveResult Failure;
        Failure.Generation = ReplayArchiveGeneration;
        Failure.Error = UTF8_TO_TCHAR(ExportError.c_str());
        if (!ReplayResultAuthority.Publish(MoveTemp(Failure)))
        {
            UE_LOG(
                LogEchoes,
                Error,
                TEXT("[ECHOES_REPLAY_ARCHIVE_AUTHORITY_FAILED] Replay export failure could not be bound to its result."));
        }
        return;
    }

    FEchoesReplayMetadata Metadata;
    Metadata.ReplayId = FString::Printf(
        TEXT("%s-%s"),
        *FDateTime::UtcNow().ToString(TEXT("%Y%m%dT%H%M%SZ")),
        *FGuid::NewGuid().ToString(EGuidFormats::Digits));
    Metadata.RecordedUtc = FDateTime::UtcNow();
    Metadata.OperationId = OperationStableId(SelectedOperation);
    Metadata.OperationType = SelectedOperation == EEchoesOperationMode::Skirmish
        ? EEchoesReplayOperationType::Skirmish
        : EEchoesReplayOperationType::Campaign;
    Metadata.MapId = SelectedOperation == EEchoesOperationMode::Skirmish
        ? SkirmishMapStableId(ActiveSkirmishSetup.MapPreset)
        : CampaignMapId(SelectedOperation, CampaignProgress);
    Metadata.bOperationCompleted = true;
    const echoes::sim::net::CompatibilityManifest Compatibility =
        echoes::network::BuildCompatibilityManifest(Simulation.Get());
    Metadata.BuildIdentity = DigestHex(Compatibility.buildIdSha256);
    Metadata.RulesIdentity = DigestHex(Compatibility.rulesPackSha256);

    if (Metadata.OperationType == EEchoesReplayOperationType::Campaign)
    {
        const bool bSucceeded = CurrentMissionPhaseName() == TEXT("Complete");
        Metadata.OperationResult = bSucceeded
            ? EEchoesReplayOperationResult::CampaignSuccess
            : EEchoesReplayOperationResult::CampaignFailure;
        Metadata.OutcomeCause = bSucceeded
            ? EEchoesReplayOutcomeCause::CampaignObjectivesComplete
            : EEchoesReplayOutcomeCause::CampaignFailurePredicate;
        Metadata.OutcomeReasonId = bSucceeded
            ? TEXT("mandatory_objectives_complete")
            : TEXT("mission_failure_predicate");
        EEchoesCampaignMissionId Mission{};
        if (GetMissionIdForOperation(SelectedOperation, Mission))
        {
            if (const FEchoesCampaignDecisionRecord* Record =
                    CampaignProgress.FindDecision(Mission))
            {
                Metadata.IrreversibleRecordId = FString::Printf(
                    TEXT("mission-%02u-tick-%llu-checksum-%016llx"),
                    static_cast<uint8>(Record->Mission),
                    static_cast<unsigned long long>(Record->CompletionTick),
                    static_cast<unsigned long long>(Record->FinalStateChecksum));
            }
        }
    }

    const uint64 Generation = ReplayArchiveGeneration;
    const FString ReplayDirectory =
        FEchoesMatchReplayStore::GetReplayDirectory();
    ReplayArchiveFuture = Async(
        EAsyncExecution::ThreadPool,
        [Generation, Metadata = MoveTemp(Metadata),
         Replay = MoveTemp(Replay), ReplayDirectory]() mutable
        {
            FEchoesReplayArchiveResult Result;
            Result.Generation = Generation;
            if (!FEchoesMatchReplayStore::FinalizeEnvelope(
                    Metadata, Replay, Result.Envelope, Result.Error))
            {
                return Result;
            }
            Result.bFinalized = true;
            Result.bSucceeded = FEchoesMatchReplayStore::SaveAtomic(
                ReplayDirectory,
                Result.Envelope, Result.Path, Result.Error);
            if (Result.bSucceeded)
            {
                Result.Envelope.Metadata.FilePath = Result.Path;
            }
            return Result;
        });
}

void UEchoesSimulationSubsystem::PollReplayArchive()
{
    for (int32 Index = SupersededReplayArchiveFutures.Num() - 1;
         Index >= 0; --Index)
    {
        TFuture<FEchoesReplayArchiveResult>& Future =
            SupersededReplayArchiveFutures[Index];
        if (Future.IsValid() && Future.IsReady())
        {
            Future.Get();
            SupersededReplayArchiveFutures.RemoveAtSwap(Index);
        }
    }

    if (ReplayResultAuthority.GetState() !=
            EEchoesReplayArchiveState::Pending ||
        !ReplayArchiveFuture.IsValid() || !ReplayArchiveFuture.IsReady())
    {
        return;
    }
    TFuture<FEchoesReplayArchiveResult> CompletedFuture =
        MoveTemp(ReplayArchiveFuture);
    ReplayArchiveFuture = TFuture<FEchoesReplayArchiveResult>();
    FEchoesReplayArchiveResult Result = CompletedFuture.Get();
    if (Result.Generation != ReplayArchiveGeneration ||
        !ReplayResultAuthority.Publish(MoveTemp(Result)))
    {
        UE_LOG(
            LogEchoes,
            Warning,
            TEXT("[ECHOES_REPLAY_ARCHIVE_STALE] An obsolete replay archive completion was ignored."));
    }
}

void UEchoesSimulationSubsystem::DrainReplayArchiveJobs()
{
    for (TFuture<FEchoesReplayArchiveResult>& Future :
         SupersededReplayArchiveFutures)
    {
        if (Future.IsValid())
        {
            Future.Wait();
            Future.Get();
        }
    }
    SupersededReplayArchiveFutures.Reset();

    if (!ReplayArchiveFuture.IsValid())
    {
        return;
    }
    ReplayArchiveFuture.Wait();
    TFuture<FEchoesReplayArchiveResult> CompletedFuture =
        MoveTemp(ReplayArchiveFuture);
    ReplayArchiveFuture = TFuture<FEchoesReplayArchiveResult>();
    FEchoesReplayArchiveResult Result = CompletedFuture.Get();
    if (Result.Generation == ReplayArchiveGeneration &&
        ReplayResultAuthority.GetState() ==
            EEchoesReplayArchiveState::Pending)
    {
        if (!ReplayResultAuthority.Publish(MoveTemp(Result)))
        {
            UE_LOG(
                LogEchoes,
                Error,
                TEXT("[ECHOES_REPLAY_ARCHIVE_DRAIN_AUTHORITY_FAILED] Current replay archive could not be published during shutdown."));
        }
    }
}

bool UEchoesSimulationSubsystem::TickReplayPlayback(float DeltaTime)
{
    if (!bReplayPlaybackActive)
    {
        return false;
    }
    if (!FMath::IsFinite(DeltaTime) || DeltaTime < 0.0F)
    {
        return true;
    }
    if (bReplayPresentationDirty)
    {
        FString SyncError;
        CompleteReplayPresentationSync(false, SyncError);
        return true;
    }
    ReplayTimeAccumulator += FMath::Min(static_cast<double>(DeltaTime), 0.25);
    constexpr double ReplayCadenceSeconds = 1.0 / 20.0;
    int32 Cadences = 0;
    FString Error;
    bool bAdvanceFailed = false;
    while (ReplayTimeAccumulator >= ReplayCadenceSeconds && Cadences < 8)
    {
        if (!ReplayPlayback.AdvanceOneCadence(Error))
        {
            ReplayPlayback.SetPaused(true);
            ReplayPlaybackError = MoveTemp(Error);
            bAdvanceFailed = true;
            break;
        }
        ReplayTimeAccumulator -= ReplayCadenceSeconds;
        ++Cadences;
    }
    if (ReplayPlayback.GetCurrentTick() == ReplayPlayback.GetFinalTick())
    {
        ReplayPlayback.SetPaused(true);
    }
    if (!bAdvanceFailed && Cadences > 0)
    {
        FString SyncError;
        CompleteReplayPresentationSync(false, SyncError);
    }
    return true;
}

const echoes::sim::Simulation*
UEchoesSimulationSubsystem::GetPresentedSimulation() const
{
    return bReplayPlaybackActive
        ? ReplayPlayback.GetSimulation()
        : Simulation.Get();
}

const echoes::sim::Simulation*
UEchoesSimulationSubsystem::GetReplayPresentationSimulation() const
{
    return bReplayPlaybackActive ? ReplayPlayback.GetSimulation() : nullptr;
}

std::optional<echoes::sim::PlayerView>
UEchoesSimulationSubsystem::GetReplayPresentationPlayerView() const
{
    return bReplayPlaybackActive ? ReplayPlayback.GetPlayerView() : std::nullopt;
}

EEchoesOperationMode
UEchoesSimulationSubsystem::GetReplayPresentationOperation() const
{
    return ReplayPresentationOperation;
}

EEchoesSkirmishMapPreset
UEchoesSimulationSubsystem::GetReplayPresentationMapPreset() const
{
    return ReplayPresentationMapPreset;
}

const FEchoesReplayMetadata*
UEchoesSimulationSubsystem::GetActiveReplayMetadata() const
{
    return ActiveReplayEnvelope.IsSet()
        ? &ActiveReplayEnvelope->Metadata
        : nullptr;
}

const echoes::sim::MatchReport*
UEchoesSimulationSubsystem::GetActiveReplayReport() const
{
    return ActiveReplayEnvelope.IsSet()
        ? &ActiveReplayEnvelope->Report
        : nullptr;
}

void UEchoesSimulationSubsystem::ResetReplayPresentationObservers()
{
    DestroyEntityViews();
    ResetDestructionViewsForScenario();
    ResetCombatEffectViewsForScenario();
    NarrativeBaselineSimulation = nullptr;
    NarrativeLastPhaseName.Reset();
}

bool UEchoesSimulationSubsystem::SyncReplayPresentation(bool bTeleport)
{
#if WITH_DEV_AUTOMATION_TESTS
    if (bFailNextReplayPresentationSyncForTesting)
    {
        bFailNextReplayPresentationSyncForTesting = false;
        return false;
    }
#endif
    return SyncEntityViews(bTeleport) && SyncTerrainView() && SyncFogView();
}

bool UEchoesSimulationSubsystem::CompleteReplayPresentationSync(
    bool bTeleport,
    FString& OutFeedback)
{
    bReplayPresentationDirty = true;
    if (SyncReplayPresentation(bTeleport))
    {
        bReplayPresentationDirty = false;
        ReplayPlaybackError.Reset();
        OutFeedback.Reset();
        return true;
    }

    ReplayPlayback.SetPaused(true);
    ReplayPlaybackError = TEXT(
        "[REPLAY_PRESENTATION_FAILED] Playback paused because the authoritative replay state could not be synchronized to its presentation.");
    OutFeedback = ReplayPlaybackError;
    return false;
}
