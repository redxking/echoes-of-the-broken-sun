#include "EchoesPlayerController.h"

#include "EchoesMatchReplay.h"
#include "Async/Async.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesSkirmishSetup.h"
#include "HAL/PlatformTime.h"

#define LOCTEXT_NAMESPACE "EchoesPlayerReplay"

namespace
{
constexpr uint64 ReplayTicksPerSecond = 20;

FText ReplayMapDisplayName(const FString& MapId)
{
    if (MapId == TEXT("glass-scar"))
    {
        return LOCTEXT("ReplayMapGlassScar", "Glass Scar");
    }
    if (MapId == TEXT("crownfall-basin"))
    {
        return LOCTEXT("ReplayMapCrownfallBasin", "Crownfall Basin");
    }
    if (MapId == TEXT("soryn-confluence"))
    {
        return LOCTEXT("ReplayMapSorynConfluence", "Soryn Confluence");
    }
    return LOCTEXT("ReplayMapCampaignBattlefield", "Campaign battlefield");
}

FText ReplayResultDisplayName(EEchoesReplayOperationResult Result)
{
    switch (Result)
    {
        case EEchoesReplayOperationResult::Player0Victory:
            return LOCTEXT("ReplayResultPlayer1Victory", "Player 1 victory");
        case EEchoesReplayOperationResult::Player1Victory:
            return LOCTEXT("ReplayResultPlayer2Victory", "Player 2 victory");
        case EEchoesReplayOperationResult::Player2Victory:
            return LOCTEXT("ReplayResultPlayer3Victory", "Player 3 victory");
        case EEchoesReplayOperationResult::Player3Victory:
            return LOCTEXT("ReplayResultPlayer4Victory", "Player 4 victory");
        case EEchoesReplayOperationResult::Draw:
            return LOCTEXT("ReplayResultDraw", "Draw");
        case EEchoesReplayOperationResult::CampaignSuccess:
            return LOCTEXT("ReplayResultCampaignSuccess", "Operation completed");
        case EEchoesReplayOperationResult::CampaignFailure:
            return LOCTEXT("ReplayResultCampaignFailure", "Operation failed");
        case EEchoesReplayOperationResult::Unknown:
            return LOCTEXT("ReplayResultUnavailable", "Result unavailable");
    }
    return LOCTEXT("ReplayResultUnavailableFallback", "Result unavailable");
}

FText ReplaySpeedDisplayName(EEchoesReplaySpeed Speed)
{
    switch (Speed)
    {
        case EEchoesReplaySpeed::Half: return LOCTEXT("ReplaySpeedHalf", "0.5×");
        case EEchoesReplaySpeed::Normal: return LOCTEXT("ReplaySpeedNormal", "1×");
        case EEchoesReplaySpeed::Double: return LOCTEXT("ReplaySpeedDouble", "2×");
        case EEchoesReplaySpeed::Quadruple: return LOCTEXT("ReplaySpeedQuadruple", "4×");
        case EEchoesReplaySpeed::Octuple: return LOCTEXT("ReplaySpeedOctuple", "8×");
    }
    return LOCTEXT("ReplaySpeedFallback", "1×");
}

FText ReplayPerspectiveDisplayName(EEchoesReplayPerspective Perspective)
{
    switch (Perspective)
    {
        case EEchoesReplayPerspective::Player0: return LOCTEXT("ReplayPerspectivePlayer1", "Player 1");
        case EEchoesReplayPerspective::Player1: return LOCTEXT("ReplayPerspectivePlayer2", "Player 2");
        case EEchoesReplayPerspective::Player2: return LOCTEXT("ReplayPerspectivePlayer3", "Player 3");
        case EEchoesReplayPerspective::Player3: return LOCTEXT("ReplayPerspectivePlayer4", "Player 4");
        case EEchoesReplayPerspective::OmniscientObserver:
            return LOCTEXT("ReplayPerspectiveObserver", "Observer");
    }
    return LOCTEXT("ReplayPerspectiveObserverFallback", "Observer");
}

FText ReplayEventDisplayName(echoes::sim::ReplayTimelineEventType Type)
{
    switch (Type)
    {
        case echoes::sim::ReplayTimelineEventType::FirstCombatContact:
            return LOCTEXT("ReplayEventFirstCombatContact", "First combat contact");
        case echoes::sim::ReplayTimelineEventType::FutureWellProtocol:
            return LOCTEXT("ReplayEventFutureWellProtocol", "Future Well decision");
        case echoes::sim::ReplayTimelineEventType::CommandCoreLoss:
            return LOCTEXT("ReplayEventCommandCoreLoss", "Command Core loss");
        case echoes::sim::ReplayTimelineEventType::MajorArmyClash:
            return LOCTEXT("ReplayEventMajorArmyClash", "Major army clash");
    }
    return LOCTEXT("ReplayEventRecorded", "Recorded event");
}

FText ReplayFactionDisplayName(echoes::sim::Faction Faction)
{
    switch (Faction)
    {
        case echoes::sim::Faction::MeridianCompact:
            return LOCTEXT("ReplayFactionMeridianCompact", "MERIDIAN COMPACT");
        case echoes::sim::Faction::KharuunAssemblies:
            return LOCTEXT("ReplayFactionKharuunAssemblies", "KHARUUN ASSEMBLIES");
        case echoes::sim::Faction::HollowChoir:
            return LOCTEXT("ReplayFactionHollowChoir", "HOLLOW CHOIR");
    }
    return LOCTEXT("ReplayFactionUnknown", "UNKNOWN FORCE");
}

FText ReplayFactionList(const TArray<echoes::sim::Faction>& Factions)
{
    TArray<FText> Names;
    Names.Reserve(Factions.Num());
    for (const echoes::sim::Faction Faction : Factions)
    {
        Names.Add(ReplayFactionDisplayName(Faction));
    }
    return Names.IsEmpty()
        ? LOCTEXT("ReplayFactionsUnavailable", "Unavailable")
        : FText::Join(LOCTEXT("ReplayFactionSeparator", " / "), Names);
}

FText ReplayPaddedNumber(uint64 Value, int32 MinimumDigits)
{
    const FNumberFormattingOptions Options = FNumberFormattingOptions()
        .SetUseGrouping(false)
        .SetMinimumIntegralDigits(MinimumDigits);
    return FText::AsNumber(Value, &Options);
}

FText ReplayDuration(uint64 Ticks)
{
    const uint64 TotalSeconds = Ticks / ReplayTicksPerSecond;
    const uint64 Hours = TotalSeconds / 3600;
    const uint64 Minutes = (TotalSeconds / 60) % 60;
    const uint64 Seconds = TotalSeconds % 60;
    return Hours > 0
        ? FText::Format(
              LOCTEXT("ReplayDurationHours", "{0}:{1}:{2}"),
              ReplayPaddedNumber(Hours, 1),
              ReplayPaddedNumber(Minutes, 2),
              ReplayPaddedNumber(Seconds, 2))
        : FText::Format(
              LOCTEXT("ReplayDurationMinutes", "{0}:{1}"),
              ReplayPaddedNumber(Minutes, 2),
              ReplayPaddedNumber(Seconds, 2));
}

FText ReplayRecordedUtc(const FDateTime& RecordedUtc)
{
    if (RecordedUtc == FDateTime::MinValue())
    {
        return LOCTEXT("ReplayRecordedDateUnavailable", "Date unavailable");
    }
    return FText::Format(
        LOCTEXT("ReplayRecordedUtc", "{0}-{1}-{2} {3}:{4} UTC"),
        ReplayPaddedNumber(RecordedUtc.GetYear(), 4),
        ReplayPaddedNumber(RecordedUtc.GetMonth(), 2),
        ReplayPaddedNumber(RecordedUtc.GetDay(), 2),
        ReplayPaddedNumber(RecordedUtc.GetHour(), 2),
        ReplayPaddedNumber(RecordedUtc.GetMinute(), 2));
}

FText ReplayOperationTypeDisplayName(EEchoesReplayOperationType OperationType)
{
    return OperationType == EEchoesReplayOperationType::Campaign
        ? LOCTEXT("ReplayOperationCampaign", "Campaign")
        : LOCTEXT("ReplayOperationSkirmish", "Skirmish");
}

int32 ReplayMapFilterIndex(const FString& MapId)
{
    if (MapId == TEXT("glass-scar")) return 1;
    if (MapId == TEXT("crownfall-basin")) return 2;
    if (MapId == TEXT("soryn-confluence")) return 3;
    return 0;
}

FString ReplayMapFilterAt(int32 Index)
{
    switch (Index)
    {
        case 1: return TEXT("glass-scar");
        case 2: return TEXT("crownfall-basin");
        case 3: return TEXT("soryn-confluence");
        default: return {};
    }
}

FText ReplayMapFilterDisplayName(int32 Index)
{
    switch (Index)
    {
        case 1: return LOCTEXT("ReplayMapFilterGlassScar", "Glass Scar");
        case 2: return LOCTEXT("ReplayMapFilterCrownfallBasin", "Crownfall Basin");
        case 3: return LOCTEXT("ReplayMapFilterSorynConfluence", "Soryn Confluence");
        default: return LOCTEXT("ReplayMapFilterAll", "All maps");
    }
}

int32 NextReplayDateFilter(int32 Current)
{
    if (Current == 0) return 7;
    if (Current == 7) return 30;
    return 0;
}

FText ReplayDateFilterDisplayName(int32 Days)
{
    if (Days == 7) return LOCTEXT("ReplayDateFilterLast7Days", "Last 7 days");
    if (Days == 30) return LOCTEXT("ReplayDateFilterLast30Days", "Last 30 days");
    return LOCTEXT("ReplayDateFilterAll", "All dates");
}

EEchoesReplaySpeed AdjacentReplaySpeed(
    EEchoesReplaySpeed Current,
    int32 Direction)
{
    const int32 Raw = FMath::Clamp(
        static_cast<int32>(Current) + Direction,
        static_cast<int32>(EEchoesReplaySpeed::Half),
        static_cast<int32>(EEchoesReplaySpeed::Octuple));
    return static_cast<EEchoesReplaySpeed>(Raw);
}

TArray<EEchoesReplayPerspective> AvailableReplayPerspectives(
    const FEchoesReplayMetadata* Metadata)
{
    TArray<EEchoesReplayPerspective> Result;
    const int32 PlayerCount = Metadata != nullptr
        ? FMath::Clamp(Metadata->PlayerFactions.Num(), 0, 4)
        : 0;
    for (int32 Player = 0; Player < PlayerCount; ++Player)
    {
        Result.Add(static_cast<EEchoesReplayPerspective>(Player));
    }
    Result.Add(EEchoesReplayPerspective::OmniscientObserver);
    return Result;
}
}

void AEchoesPlayerController::BuildReplayShellView(FEchoesShellView& View) const
{
    const UEchoesSimulationSubsystem* Bridge = GetWorld()
        ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
        : nullptr;
    const auto Button = [&View](
                            FText Label,
                            EEchoesShellAction Action,
                            bool bEnabled = true,
                            int32 Argument = 0)
    {
        View.Buttons.Add({Label, Action, Argument, bEnabled});
    };

    if (View.Screen == EEchoesShellScreen::ReplayBrowser)
    {
        View.Title = LOCTEXT("ReplayBrowserTitle", "Replay archive");
        const int32 MapFilterIndex = ReplayMapFilterIndex(
            ReplayBrowserMapFilter);
        TArray<FText> Rows;
        Rows.Reserve(ReplayBrowserEntries.Num());
        for (int32 Index = 0; Index < ReplayBrowserEntries.Num(); ++Index)
        {
            const FEchoesReplayMetadata& Metadata = ReplayBrowserEntries[Index];
            Rows.Add(FText::Format(
                LOCTEXT(
                    "ReplayBrowserRow",
                    "{0}. {1}\n{2} | {3}\n{4} | {5}\nDuration {6}"),
                FText::AsNumber(
                    Index + 1,
                    &FNumberFormattingOptions::DefaultNoGrouping()),
                ReplayRecordedUtc(Metadata.RecordedUtc),
                ReplayMapDisplayName(Metadata.MapId),
                ReplayOperationTypeDisplayName(Metadata.OperationType),
                ReplayFactionList(Metadata.PlayerFactions),
                ReplayResultDisplayName(Metadata.OperationResult),
                ReplayDuration(Metadata.DurationTicks)));
            Button(
                FText::Format(
                    LOCTEXT("OpenArchivedReplay", "Open replay {0}: {1}"),
                    FText::AsNumber(Index + 1),
                    ReplayMapDisplayName(Metadata.MapId)),
                EEchoesShellAction::OpenReplay,
                true,
                Index);
        }
        View.Body = bReplayBrowserLoading
            ? LOCTEXT("ReplayBrowserLoading", "Checking saved replays…")
            : Rows.IsEmpty()
            ? LOCTEXT(
                  "NoArchivedReplays",
                  "No valid completed replays match these filters.")
            : FText::Join(FText::FromString(TEXT("\n\n")), Rows);
        Button(
            FText::Format(
                LOCTEXT("ReplayMapFilter", "Map: {0} — change"),
                ReplayMapFilterDisplayName(MapFilterIndex)),
            EEchoesShellAction::ReplayMapFilter,
            true,
            (MapFilterIndex + 1) % 4);
        Button(
            FText::Format(
                LOCTEXT("ReplayDateFilter", "Date: {0} — change"),
                ReplayDateFilterDisplayName(ReplayBrowserDateFilter)),
            EEchoesShellAction::ReplayDateFilter,
            true,
            NextReplayDateFilter(ReplayBrowserDateFilter));
        Button(
            LOCTEXT("CloseReplayBrowser", "Back"),
            EEchoesShellAction::ExitReplay);
        return;
    }

    if (View.Screen != EEchoesShellScreen::ReplayTransport)
    {
        return;
    }

    View.Title = LOCTEXT("ReplayTransportTitle", "Replay");
    const FEchoesReplayPlaybackState State = Bridge != nullptr
        ? Bridge->GetReplayPlaybackState()
        : FEchoesReplayPlaybackState{};
    const FEchoesReplayMetadata* Metadata = Bridge != nullptr
        ? Bridge->GetActiveReplayMetadata()
        : nullptr;
    const echoes::sim::MatchReport* Report = Bridge != nullptr
        ? Bridge->GetActiveReplayReport()
        : nullptr;
    if (!State.bActive || Metadata == nullptr)
    {
        View.Body = LOCTEXT(
            "ReplayTransportUnavailable",
            "The replay session is no longer active.");
        Button(
            LOCTEXT("ExitUnavailableReplay", "Exit replay"),
            EEchoesShellAction::ExitReplay);
        return;
    }

    View.Body = FText::Format(
        LOCTEXT(
            "ReplayTransportSummary",
            "{0} | {1}\n{2}\n{3}\n{4} / {5} | {6} | {7}"),
        ReplayMapDisplayName(Metadata->MapId),
        ReplayOperationTypeDisplayName(Metadata->OperationType),
        ReplayFactionList(Metadata->PlayerFactions),
        ReplayResultDisplayName(Metadata->OperationResult),
        ReplayDuration(State.CurrentTick),
        ReplayDuration(State.FinalTick),
        ReplaySpeedDisplayName(State.Speed),
        ReplayPerspectiveDisplayName(State.Perspective));
    if (!State.Error.IsEmpty())
    {
        View.Status = FText::FromString(State.Error);
    }
    const float NormalizedTick = State.FinalTick > 0
        ? FMath::Clamp(
              static_cast<float>(
                  static_cast<double>(State.CurrentTick) /
                  static_cast<double>(State.FinalTick)),
              0.0f,
              1.0f)
        : 0.0f;
    View.Sliders.Add({
        FText::Format(
            LOCTEXT("ReplayTimeline", "Timeline: {0} / {1}"),
            ReplayDuration(State.CurrentTick),
            ReplayDuration(State.FinalTick)),
        EEchoesShellAction::ReplaySeek,
        NormalizedTick,
        0.0f,
        1.0f});
    Button(
        State.bPaused
            ? LOCTEXT("ReplayPlay", "Play")
            : LOCTEXT("ReplayPause", "Pause"),
        EEchoesShellAction::ReplayPlayPause);
    Button(
        LOCTEXT("ReplaySpeedPrevious", "Slower"),
        EEchoesShellAction::ReplaySpeedPrevious,
        State.Speed != EEchoesReplaySpeed::Half);
    Button(
        FText::Format(
            LOCTEXT("ReplaySpeedNext", "Speed: {0} — faster"),
            ReplaySpeedDisplayName(State.Speed)),
        EEchoesShellAction::ReplaySpeedNext,
        State.Speed != EEchoesReplaySpeed::Octuple);
    Button(
        LOCTEXT("ReplayStep", "Step one tick"),
        EEchoesShellAction::ReplayStep,
        State.bPaused && State.CurrentTick < State.FinalTick);
    Button(
        LOCTEXT("ReplayPerspectivePrevious", "Previous perspective"),
        EEchoesShellAction::ReplayPerspectivePrevious);
    Button(
        FText::Format(
            LOCTEXT("ReplayPerspectiveNext", "Perspective: {0} — next"),
            ReplayPerspectiveDisplayName(State.Perspective)),
        EEchoesShellAction::ReplayPerspectiveNext);
    if (Report != nullptr)
    {
        for (int32 Index = 0;
             Index < static_cast<int32>(Report->events.size());
             ++Index)
        {
            const echoes::sim::ReplayTimelineEvent& Event =
                Report->events[static_cast<size_t>(Index)];
            Button(
                FText::Format(
                    LOCTEXT("ReplayBookmark", "{0} — {1}"),
                    ReplayEventDisplayName(Event.type),
                    ReplayDuration(Event.tick)),
                EEchoesShellAction::ReplayBookmark,
                true,
                Index);
        }
    }
    Button(
        LOCTEXT("ExitReplay", "Exit replay"),
        EEchoesShellAction::ExitReplay);
}

void AEchoesPlayerController::RefreshReplayBrowser()
{
    if (ReplayBrowserCancellation) ReplayBrowserCancellation->store(true);
    ++ReplayBrowserGeneration;
    ReplayBrowserEntries.Reset();
    ShellMessage.Reset();
    if (!GetWorld() || !GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>())
    {
        bReplayBrowserLoading = false;
        bReplayBrowserRefreshQueued = false;
        ShellMessage = LOCTEXT("ReplayStorageUnavailable", "Replay storage is unavailable.").ToString();
        return;
    }
    RequestedReplayBrowserFilter = {};
    RequestedReplayBrowserFilter.MapId = ReplayBrowserMapFilter;
    if (ReplayBrowserDateFilter == 7 || ReplayBrowserDateFilter == 30)
        RequestedReplayBrowserFilter.EarliestUtc = FDateTime::UtcNow() - FTimespan::FromDays(ReplayBrowserDateFilter);
    // Resolve storage while on the game thread. The worker captures only values.
    RequestedReplayBrowserDirectory = FEchoesMatchReplayStore::GetReplayDirectory();
    bReplayBrowserLoading = true;
    bReplayBrowserRefreshQueued = true;
    PollReplayBrowser();
}

void AEchoesPlayerController::PollReplayBrowser()
{
    if (ReplayBrowserScan.IsValid() && ReplayBrowserScan.IsReady())
    {
        FEchoesReplayBrowserScanResult Result = ReplayBrowserScan.Get();
        ReplayBrowserScan = {};
        ReplayBrowserCancellation.reset();
        if (Result.Generation == ReplayBrowserGeneration &&
            PlayerFlow.BaseScreen() == EEchoesShellScreen::ReplayBrowser)
        {
            ReplayBrowserEntries = MoveTemp(Result.Entries);
            bReplayBrowserLoading = false;
            ShellMessage = Result.Errors.IsEmpty() ? FString() : FText::Format(
                LOCTEXT("ReplayScanErrors", "Some replay files could not be read:\n{0}"),
                FText::FromString(FString::Join(Result.Errors, TEXT("\n")))).ToString();
        }
    }
    // At most one scan is active. Rapid filter changes replace the queued
    // request; a stale completion cannot publish over a newer browser view.
    if (!ReplayBrowserScan.IsValid() && bReplayBrowserRefreshQueued)
    {
        bReplayBrowserRefreshQueued = false;
        const FString Directory = RequestedReplayBrowserDirectory;
        const FEchoesReplayBrowserFilter Filter = RequestedReplayBrowserFilter;
        const uint64 Generation = ReplayBrowserGeneration;
        ReplayBrowserCancellation = std::make_shared<std::atomic_bool>(false);
        const auto Cancellation = ReplayBrowserCancellation;
        ReplayBrowserScan = Async(EAsyncExecution::ThreadPool, [Directory, Filter, Generation, Cancellation]()
        {
            FEchoesReplayBrowserScanResult Result;
            Result.Generation = Generation;
            // Bound verification work without placing a duration limit on the
            // recorded match. Cancelled records are never published as valid.
            const double Deadline = FPlatformTime::Seconds() + 30.0;
            const FEchoesReplayCancellationCheck ShouldCancel = [Cancellation, Deadline]()
            {
                return Cancellation->load() || FPlatformTime::Seconds() >= Deadline;
            };
            Result.Entries = FEchoesMatchReplayStore::ListMetadata(Directory, Filter, Result.Errors, ShouldCancel);
            return Result;
        });
    }
}

void AEchoesPlayerController::CancelReplayBrowserScan()
{
    if (ReplayBrowserCancellation) ReplayBrowserCancellation->store(true);
    ++ReplayBrowserGeneration;
    bReplayBrowserRefreshQueued = false;
    bReplayBrowserLoading = false;
    ReplayBrowserEntries.Reset();
    // Async owns the promise and captures values only. Dropping our future
    // retires publication without waiting for filesystem I/O during EndPlay.
    ReplayBrowserScan = {};
    ReplayBrowserCancellation.reset();
}

#if WITH_DEV_AUTOMATION_TESTS
void AEchoesPlayerController::DrainReplayBrowserScan()
{
    if (ReplayBrowserCancellation) ReplayBrowserCancellation->store(true);
    ++ReplayBrowserGeneration;
    bReplayBrowserRefreshQueued = false;
    bReplayBrowserLoading = false;
    if (ReplayBrowserScan.IsValid())
    {
        ReplayBrowserScan.Wait();
        (void)ReplayBrowserScan.Get();
        ReplayBrowserScan = {};
    }
    ReplayBrowserCancellation.reset();
}
#endif

bool AEchoesPlayerController::HandleReplayShellAction(
    EEchoesShellAction Action,
    int32 Argument,
    bool bConfirmed)
{
    UEchoesSimulationSubsystem* Bridge = GetWorld()
        ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
        : nullptr;
    const auto Fail = [&](const FString& Feedback)
    {
        ShellMessage = Feedback.IsEmpty()
            ? TEXT("[REPLAY_ACTION_FAILED] The replay action could not be completed.")
            : Feedback;
        PendingShellAction = Action;
        PendingShellArgument = Argument;
        PlayerFlow.Push(EEchoesShellScreen::Error);
    };
    const auto BeginTransport = [&](bool bOpened, const FString& Feedback)
    {
        if (!bOpened)
        {
            Fail(Feedback);
            return;
        }
        ClearSelection();
        bSelectionButtonDown = false;
        bMinimapDragging = false;
        bControlGroupAssignmentArmed = false;
        bKeyboardTargetingEnabled = false;
        ArmedDeckAction = EEchoesCommandDeckAction::None;
        bTechnologyPanelVisible = false;
        bCampaignOperationsMapVisible = false;
        ShellMessage = Feedback;
        PlayerFlow.SetVisible(EEchoesShellScreen::ReplayTransport, true);
        ResetIgnoreMoveInput();
        ResetIgnoreLookInput();
    };
    const auto RequireActiveReplay = [&]()
    {
        if (Bridge != nullptr && Bridge->IsReplayPlaybackActive())
        {
            return true;
        }
        Fail(TEXT("[REPLAY_NOT_ACTIVE] Open a replay before using transport controls."));
        return false;
    };

    switch (Action)
    {
        case EEchoesShellAction::OpenReplayBrowser:
            ReplayReturnScreen = PlayerFlow.BaseScreen() ==
                    EEchoesShellScreen::Results
                ? EEchoesShellScreen::Results
                : EEchoesShellScreen::Title;
            PlayerFlow.SetVisible(EEchoesShellScreen::ReplayBrowser, true);
            RefreshReplayBrowser();
            ResetIgnoreMoveInput();
            ResetIgnoreLookInput();
            SetIgnoreMoveInput(true);
            SetIgnoreLookInput(true);
            return true;

        case EEchoesShellAction::OpenReplay:
        {
            if (Bridge == nullptr || !ReplayBrowserEntries.IsValidIndex(Argument))
            {
                Fail(TEXT("[REPLAY_SELECTION_INVALID] The selected replay is no longer available."));
                return true;
            }
            FString Feedback;
            BeginTransport(
                Bridge->BeginReplay(
                    ReplayBrowserEntries[Argument].FilePath, Feedback),
                Feedback);
            return true;
        }

        case EEchoesShellAction::ViewReplay:
        {
            if (Bridge == nullptr)
            {
                Fail(TEXT("[REPLAY_RUNTIME_UNAVAILABLE] Replay playback is unavailable."));
                return true;
            }
            ReplayReturnScreen = EEchoesShellScreen::Results;
            FString Feedback;
            BeginTransport(Bridge->BeginCompletedReplay(Feedback), Feedback);
            return true;
        }

        case EEchoesShellAction::ReplayPlayPause:
        {
            if (!RequireActiveReplay()) return true;
            const FEchoesReplayPlaybackState State =
                Bridge->GetReplayPlaybackState();
            Bridge->SetReplayPaused(!State.bPaused);
            ShellMessage = State.bPaused
                ? LOCTEXT("ReplayNowPlaying", "Replay playing.").ToString()
                : LOCTEXT("ReplayNowPaused", "Replay paused.").ToString();
            return true;
        }

        case EEchoesShellAction::ReplaySpeedPrevious:
        case EEchoesShellAction::ReplaySpeedNext:
        {
            if (!RequireActiveReplay()) return true;
            const FEchoesReplayPlaybackState State =
                Bridge->GetReplayPlaybackState();
            const EEchoesReplaySpeed Speed = AdjacentReplaySpeed(
                State.Speed,
                Action == EEchoesShellAction::ReplaySpeedNext ? 1 : -1);
            Bridge->SetReplaySpeed(Speed);
            ShellMessage = FText::Format(
                LOCTEXT("ReplaySpeedChanged", "Replay speed: {0}."),
                ReplaySpeedDisplayName(Speed)).ToString();
            return true;
        }

        case EEchoesShellAction::ReplayStep:
        {
            if (!RequireActiveReplay()) return true;
            const FEchoesReplayPlaybackState State =
                Bridge->GetReplayPlaybackState();
            if (!State.bPaused)
            {
                Fail(TEXT("[REPLAY_STEP_REQUIRES_PAUSE] Pause the replay before stepping."));
                return true;
            }
            if (!bConfirmed) PendingReplayTick = FMath::Min(State.CurrentTick + 1, State.FinalTick);
            FString Feedback;
            const bool bStepped = bConfirmed ? Bridge->SeekReplayTick(PendingReplayTick, Feedback) : Bridge->StepReplay(Feedback);
            if (!bStepped)
            {
                Fail(Feedback);
            }
            else
            {
                ShellMessage = Feedback;
            }
            return true;
        }

        case EEchoesShellAction::ReplaySeek:
        {
            if (!RequireActiveReplay()) return true;
            FString Feedback;
            if (!Bridge->SeekReplayTick(PendingReplayTick, Feedback)) Fail(Feedback);
            else ShellMessage = Feedback;
            return true;
        }

        case EEchoesShellAction::ReplayPerspectivePrevious:
        case EEchoesShellAction::ReplayPerspectiveNext:
        {
            if (!RequireActiveReplay()) return true;
            const FEchoesReplayPlaybackState State =
                Bridge->GetReplayPlaybackState();
            const TArray<EEchoesReplayPerspective> Perspectives =
                AvailableReplayPerspectives(Bridge->GetActiveReplayMetadata());
            int32 Index = Perspectives.IndexOfByKey(State.Perspective);
            if (Index == INDEX_NONE) Index = Perspectives.Num() - 1;
            const int32 Direction =
                Action == EEchoesShellAction::ReplayPerspectiveNext ? 1 : -1;
            Index = bConfirmed ? Perspectives.IndexOfByKey(static_cast<EEchoesReplayPerspective>(Argument)) :
                (Index + Direction + Perspectives.Num()) % Perspectives.Num();
            if (!Perspectives.IsValidIndex(Index))
            {
                Fail(TEXT("[REPLAY_PERSPECTIVE_INVALID] That recorded perspective is unavailable."));
                return true;
            }
            FString Feedback;
            if (!Bridge->SetReplayPerspective(Perspectives[Index], Feedback))
            {
                Fail(Feedback);
                PendingShellArgument = static_cast<int32>(Perspectives[Index]);
            }
            else
            {
                ShellMessage = FText::Format(
                    LOCTEXT(
                        "ReplayPerspectiveChanged",
                        "Replay perspective: {0}."),
                    ReplayPerspectiveDisplayName(Perspectives[Index])).ToString();
            }
            return true;
        }

        case EEchoesShellAction::ReplayBookmark:
        {
            if (!RequireActiveReplay()) return true;
            FString Feedback;
            if (!Bridge->SeekReplayEvent(Argument, Feedback))
            {
                Fail(Feedback);
            }
            else
            {
                ShellMessage = Feedback;
            }
            return true;
        }

        case EEchoesShellAction::ReplayMapFilter:
            if (Argument < 0 || Argument > 3)
            {
                Fail(TEXT("[REPLAY_FILTER_INVALID] That map filter is unavailable."));
                return true;
            }
            ReplayBrowserMapFilter = ReplayMapFilterAt(Argument);
            RefreshReplayBrowser();
            return true;

        case EEchoesShellAction::ReplayDateFilter:
            if (Argument != 0 && Argument != 7 && Argument != 30)
            {
                Fail(TEXT("[REPLAY_FILTER_INVALID] That date filter is unavailable."));
                return true;
            }
            ReplayBrowserDateFilter = Argument;
            RefreshReplayBrowser();
            return true;

        case EEchoesShellAction::ExitReplay:
        {
            if (ReplayBrowserCancellation) ReplayBrowserCancellation->store(true);
            ++ReplayBrowserGeneration;
            bReplayBrowserRefreshQueued = false;
            bReplayBrowserLoading = false;
            ReplayBrowserEntries.Reset();
            if (Bridge != nullptr)
            {
                Bridge->EndReplay();
            }
            const EEchoesShellScreen Destination =
                ReplayReturnScreen == EEchoesShellScreen::Results
                ? EEchoesShellScreen::Results
                : EEchoesShellScreen::Title;
            ShellMessage.Reset();
            PlayerFlow.SetVisible(Destination, true);
            ResetIgnoreMoveInput();
            ResetIgnoreLookInput();
            SetIgnoreMoveInput(true);
            SetIgnoreLookInput(true);
            return true;
        }

        case EEchoesShellAction::Rematch:
        {
            if (!RequireOperationMastery(EEchoesOperationMode::Skirmish)) return true;
            if (Bridge == nullptr ||
                Bridge->GetOperationMode() != EEchoesOperationMode::Skirmish)
            {
                Fail(TEXT("[REMATCH_SKIRMISH_ONLY] Rematch is available after a skirmish result."));
                return true;
            }
            FEchoesSkirmishSetup Setup = Bridge->GetActiveSkirmishSetup();
            const uint64 PriorSeed = Setup.Seed;
            uint64 FreshSeed = FPlatformTime::Cycles64() ^
                static_cast<uint64>(FDateTime::UtcNow().GetTicks());
            if (FreshSeed == 0 || FreshSeed == PriorSeed)
            {
                FreshSeed = PriorSeed ^ 0x9e3779b97f4a7c15ULL;
            }
            if (FreshSeed == 0 || FreshSeed == PriorSeed)
            {
                FreshSeed = PriorSeed + 1;
                if (FreshSeed == 0) FreshSeed = 1;
            }
            Setup.Seed = FreshSeed;
            FString Feedback;
            if (!Bridge->ApplySkirmishSetup(Setup, Feedback))
            {
                Fail(Feedback);
                return true;
            }
            PendingSkirmishSetup = Bridge->GetActiveSkirmishSetup();
            Bridge->SetScenarioPaused(false);
            ShellMessage = Feedback;
            PlayerFlow.SetVisible(EEchoesShellScreen::Gameplay, true);
            ResetIgnoreMoveInput();
            ResetIgnoreLookInput();
            return true;
        }

        default:
            return false;
    }
}

#undef LOCTEXT_NAMESPACE
