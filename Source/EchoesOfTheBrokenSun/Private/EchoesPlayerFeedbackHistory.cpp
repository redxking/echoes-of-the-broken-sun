// Copyright Echoes of the Broken Sun. All Rights Reserved.
// Author: Angelis Pseftis

#include "EchoesPlayerController.h"

#include "EchoesSimulationSubsystem.h"

#define LOCTEXT_NAMESPACE "EchoesPlayerFeedbackHistory"

namespace
{
FText DispositionLabel(EEchoesFeedbackHistoryDisposition Disposition)
{
    switch (Disposition)
    {
        case EEchoesFeedbackHistoryDisposition::Queued:
            return LOCTEXT("QueuedDisposition", "QUEUED");
        case EEchoesFeedbackHistoryDisposition::Applied:
            return LOCTEXT("AppliedDisposition", "APPLIED");
        case EEchoesFeedbackHistoryDisposition::NoEffect:
            return LOCTEXT("NoEffectDisposition", "NO EFFECT");
        case EEchoesFeedbackHistoryDisposition::Transition:
            // The model uses this bucket for lifecycle transitions, progress,
            // and repairs. "Recorded" does not mislabel the latter two.
            return LOCTEXT("RecordedDisposition", "RECORDED");
    }
    return LOCTEXT("UnknownDisposition", "UNKNOWN");
}

FText SeverityLabel(EEchoesFeedbackHistorySeverity Severity)
{
    return Severity == EEchoesFeedbackHistorySeverity::Warning
        ? LOCTEXT("WarningSeverity", "WARNING")
        : LOCTEXT("InformationalSeverity", "INFO");
}

FText ElapsedMatchTime(uint64 Tick)
{
    // The deterministic simulation runs at the canonical 20 Hz cadence.
    const uint64 Seconds = Tick / 20;
    const uint64 Hundredths = (Tick % 20) * 5;
    return FText::FromString(FString::Printf(
        TEXT("%llu:%02llu.%02llu"),
        Seconds / 60,
        Seconds % 60,
        Hundredths));
}

void AddIdentifier(
    TArray<FText>& Identifiers,
    const FText& Label,
    uint64 Value)
{
    if (Value == 0) return;
    Identifiers.Add(FText::Format(
        LOCTEXT("Identifier", "{0} #{1}"),
        Label,
        FText::AsNumber(Value, &FNumberFormattingOptions::DefaultNoGrouping())));
}

void AddCarriedValue(
    TArray<FText>& Values,
    const FText& Label,
    const TOptional<int64>& Value)
{
    if (!Value.IsSet()) return;
    Values.Add(FText::Format(
        LOCTEXT("CarriedValue", "{0} {1}"),
        Label,
        FText::AsNumber(
            Value.GetValue(),
            &FNumberFormattingOptions::DefaultNoGrouping())));
}

FText DescribeRow(const FEchoesFeedbackHistoryRow& Row)
{
    TArray<FText> Sources;
    AddIdentifier(Sources, LOCTEXT("ActorSource", "Unit"), Row.ActorEntityId);
    AddIdentifier(Sources, LOCTEXT("TargetSource", "Target"), Row.TargetEntityId);
    AddIdentifier(Sources, LOCTEXT("ProducerSource", "Producer"), Row.ProducerEntityId);
    AddIdentifier(Sources, LOCTEXT("ProductionItemSource", "Queue item"), Row.ProductionItemId);

    TArray<FText> ExactValues;
    AddCarriedValue(ExactValues, LOCTEXT("MatterSpent", "Matter spent"), Row.MatterSpent);
    AddCarriedValue(ExactValues, LOCTEXT("DawnSpent", "Dawnshards spent"), Row.DawnSpent);
    AddCarriedValue(ExactValues, LOCTEXT("MatterRefunded", "Matter refunded"), Row.MatterRefunded);
    AddCarriedValue(ExactValues, LOCTEXT("DawnRefunded", "Dawnshards refunded"), Row.DawnRefunded);
    AddCarriedValue(ExactValues, LOCTEXT("HealthDelta", "Health change"), Row.HealthDelta);
    AddCarriedValue(ExactValues, LOCTEXT("ProgressDelta", "Progress change"), Row.ProgressDelta);
    AddCarriedValue(ExactValues, LOCTEXT("LogisticsDelta", "Logistics change"), Row.LogisticsDelta);

    TArray<FText> Lines;
    Lines.Add(FText::Format(
        LOCTEXT("HistoryRowHeading", "{0}  ·  {1}  ·  {2}  ·  {3}"),
        ElapsedMatchTime(Row.ObservedTick),
        Row.Category,
        DispositionLabel(Row.Disposition),
        SeverityLabel(Row.Severity)));
    Lines.Add(Row.Summary);
    if (!Sources.IsEmpty())
    {
        Lines.Add(FText::Format(
            LOCTEXT("Sources", "Source: {0}"),
            FText::Join(LOCTEXT("SourceSeparator", "  /  "), Sources)));
    }
    if (!ExactValues.IsEmpty())
    {
        Lines.Add(FText::Format(
            LOCTEXT("ActualChanges", "Actual: {0}"),
            FText::Join(LOCTEXT("ValueSeparator", "  /  "), ExactValues)));
    }
    return FText::Join(FText::FromString(TEXT("\n")), Lines);
}

FText FilterButtonLabel(
    const FText& Label,
    bool bSelected)
{
    return bSelected
        ? FText::Format(LOCTEXT("SelectedFilter", "Selected — {0}"), Label)
        : Label;
}
} // namespace

void AEchoesPlayerController::BuildFeedbackHistoryShellView(
    FEchoesShellView& View) const
{
    View.Screen = EEchoesShellScreen::FeedbackHistory;
    View.Eyebrow = LOCTEXT("FeedbackEyebrow", "FIELD COMMAND");
    View.Title = LOCTEXT("FeedbackTitle", "Command history");
    View.Buttons.Reset();
    View.Sliders.Reset();
    View.Charts.Reset();

    const auto Button = [&View](
        const FText& Label,
        EEchoesShellAction Action,
        bool bEnabled = true)
    {
        View.Buttons.Add({Label, Action, 0, bEnabled});
    };

    // The receipt stream belongs only to a live match. Replay playback has a
    // separately scrubbed authority and must never be mixed with it. Online
    // players may inspect the same scoped stream from their local menu while
    // simulation continues; opening this view only redirects their input.
    const bool bPausedLiveMatch = PlayerFlow.Is(EEchoesShellScreen::Pause) &&
        !IsReplayInputActive();
    const bool bOnlineLiveMenu = bOnlineLocalMenuVisible &&
        IsActiveOnlineNetworkMatch() && !IsReplayInputActive();
    if (!bPausedLiveMatch && !bOnlineLiveMenu)
    {
        View.Status = LOCTEXT("FeedbackOutsideMatchStatus",
            "Command history is unavailable in the current view.");
        View.Body = LOCTEXT("FeedbackOutsideMatchBody",
            "No command history is available.");
        Button(LOCTEXT("Back", "Back"), EEchoesShellAction::Back);
        return;
    }

    const echoes::sim::PlayerId Recipient = GetNetMode() == NM_Client
        ? static_cast<echoes::sim::PlayerId>(GetNetworkSeat())
        : static_cast<echoes::sim::PlayerId>(
            UEchoesSimulationSubsystem::LocalPlayerId);
    const FEchoesFeedbackHistoryView History = FEchoesFeedbackHistoryModel::Build(
        GetGameplayFeedback(), Recipient, FeedbackHistoryFilter);

    const FText HistoryStatus = History.bRecipientRejected
        ? LOCTEXT("FeedbackUnavailableStatus", "Command history is unavailable.")
        : History.bHistoryIncomplete
            ? LOCTEXT("FeedbackIncompleteStatus", "Some command history is unavailable.")
            : LOCTEXT("FeedbackRetainedStatus", "Recent command activity");
    View.Status = bOnlineLiveMenu
        ? FText::Format(LOCTEXT("OnlineHistoryStatus",
            "ONLINE MATCH CONTINUES — local controls are held while this menu is open.\n{0}"),
            HistoryStatus)
        : HistoryStatus;

    TArray<FText> Sections;
    Sections.Add(FeedbackHistoryFilter == EEchoesFeedbackHistoryFilter::All
        ? FText::Format(LOCTEXT("AllFilterSummary", "Showing {0} command events."),
            FText::AsNumber(History.EligibleEventCount))
        : FText::Format(LOCTEXT("FilterSummary", "Showing {0} of {1} command events."),
            FText::AsNumber(History.Rows.Num()),
            FText::AsNumber(History.EligibleEventCount)));
    if (History.bHistoryIncomplete && !History.bRecipientRejected)
    {
        Sections.Add(LOCTEXT("FeedbackIncompleteBody",
            "Some earlier command activity is no longer available."));
    }

    if (History.Rows.IsEmpty())
    {
        Sections.Add(History.bRecipientRejected
            ? LOCTEXT("RecipientRejectedBody",
                "Command history is unavailable.")
            : LOCTEXT("NoMatchingFeedback",
                "No recent command activity matches this filter."));
    }
    else
    {
        TArray<FText> Rows;
        Rows.Reserve(History.Rows.Num());
        // The source retains chronological order; a recent-alert view presents
        // newest first without mutating or dropping the underlying evidence.
        for (int32 Index = History.Rows.Num() - 1; Index >= 0; --Index)
        {
            Rows.Add(DescribeRow(History.Rows[Index]));
        }
        Sections.Add(FText::Join(FText::FromString(TEXT("\n\n")), Rows));
    }
    View.Body = FText::Join(FText::FromString(TEXT("\n\n")), Sections);

    const auto FilterButton = [&](const FText& Label,
        EEchoesShellAction Action,
        EEchoesFeedbackHistoryFilter Filter)
    {
        const bool bSelected = FeedbackHistoryFilter == Filter;
        // Keep the selected filter operable so a refresh does not strand
        // keyboard focus on a control that just disabled itself.
        Button(FilterButtonLabel(Label, bSelected), Action);
    };
    FilterButton(LOCTEXT("AllFilter", "All"),
        EEchoesShellAction::FeedbackHistoryAll,
        EEchoesFeedbackHistoryFilter::All);
    FilterButton(LOCTEXT("OrdersFilter", "Orders"),
        EEchoesShellAction::FeedbackHistoryOrders,
        EEchoesFeedbackHistoryFilter::Orders);
    FilterButton(LOCTEXT("ConstructionFilter", "Construction"),
        EEchoesShellAction::FeedbackHistoryConstruction,
        EEchoesFeedbackHistoryFilter::Construction);
    FilterButton(LOCTEXT("ProductionFilter", "Production"),
        EEchoesShellAction::FeedbackHistoryProduction,
        EEchoesFeedbackHistoryFilter::Production);
    Button(LOCTEXT("Back", "Back"), EEchoesShellAction::Back);
}

bool AEchoesPlayerController::HandleFeedbackHistoryShellAction(
    EEchoesShellAction Action)
{
    const bool bPausedLiveMatch = PlayerFlow.Is(EEchoesShellScreen::Pause) &&
        !IsReplayInputActive();
    const bool bOnlineLiveMenu = bOnlineLocalMenuVisible &&
        IsActiveOnlineNetworkMatch() && !IsReplayInputActive();
    if (Action == EEchoesShellAction::OpenFeedbackHistory)
    {
        const bool bOpeningFromPause =
            PlayerFlow.Current() == EEchoesShellScreen::Pause && bPausedLiveMatch;
        const bool bOpeningFromOnlineMenu =
            PlayerFlow.Current() == EEchoesShellScreen::Gameplay && bOnlineLiveMenu;
        if (!bOpeningFromPause && !bOpeningFromOnlineMenu) return false;
        FeedbackHistoryFilter = EEchoesFeedbackHistoryFilter::All;
        PlayerFlow.Push(EEchoesShellScreen::FeedbackHistory);
        return true;
    }

    if (PlayerFlow.Current() != EEchoesShellScreen::FeedbackHistory ||
        (!bPausedLiveMatch && !bOnlineLiveMenu))
    {
        return false;
    }

    switch (Action)
    {
        case EEchoesShellAction::FeedbackHistoryAll:
            FeedbackHistoryFilter = EEchoesFeedbackHistoryFilter::All;
            return true;
        case EEchoesShellAction::FeedbackHistoryOrders:
            FeedbackHistoryFilter = EEchoesFeedbackHistoryFilter::Orders;
            return true;
        case EEchoesShellAction::FeedbackHistoryConstruction:
            FeedbackHistoryFilter = EEchoesFeedbackHistoryFilter::Construction;
            return true;
        case EEchoesShellAction::FeedbackHistoryProduction:
            FeedbackHistoryFilter = EEchoesFeedbackHistoryFilter::Production;
            return true;
        default:
            return false;
    }
}

#undef LOCTEXT_NAMESPACE
