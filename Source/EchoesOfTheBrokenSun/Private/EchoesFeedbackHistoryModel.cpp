// Copyright Echoes of the Broken Sun. All Rights Reserved.
// Author: Angelis Pseftis

#include "EchoesFeedbackHistoryModel.h"

#define LOCTEXT_NAMESPACE "EchoesFeedbackHistory"

namespace
{
using echoes::feedback::GameplayFeedbackEvent;
using echoes::feedback::GameplayFeedbackKind;

bool IsOrderKind(GameplayFeedbackKind Kind)
{
    return Kind == GameplayFeedbackKind::Queued ||
        Kind == GameplayFeedbackKind::Applied ||
        Kind == GameplayFeedbackKind::NoEffect;
}

bool IsConstructionKind(GameplayFeedbackKind Kind)
{
    return Kind == GameplayFeedbackKind::ConstructionCreated ||
        Kind == GameplayFeedbackKind::ConstructionProgressed ||
        Kind == GameplayFeedbackKind::ConstructionCompleted ||
        Kind == GameplayFeedbackKind::ConstructionCancelled ||
        Kind == GameplayFeedbackKind::RepairApplied;
}

bool IsProductionKind(GameplayFeedbackKind Kind)
{
    return Kind == GameplayFeedbackKind::ProductionQueued ||
        Kind == GameplayFeedbackKind::ProductionActivated ||
        Kind == GameplayFeedbackKind::ProductionCancelled ||
        Kind == GameplayFeedbackKind::ProductionCompleted ||
        Kind == GameplayFeedbackKind::ProductionSpawnBlocked ||
        Kind == GameplayFeedbackKind::ProductionSpawnResumed;
}

bool MatchesFilter(
    GameplayFeedbackKind Kind,
    EEchoesFeedbackHistoryFilter Filter)
{
    switch (Filter)
    {
        case EEchoesFeedbackHistoryFilter::All:
            return true;
        case EEchoesFeedbackHistoryFilter::Orders:
            return IsOrderKind(Kind);
        case EEchoesFeedbackHistoryFilter::Construction:
            return IsConstructionKind(Kind);
        case EEchoesFeedbackHistoryFilter::Production:
            return IsProductionKind(Kind);
    }
    return false;
}

void AddCarried(TOptional<int64>& Destination, int64 Value)
{
    // Zero is the feedback contract's "not carried" default. The UI never
    // turns it into a claimed zero-cost/refund/health/progress observation.
    // Deltas may be negative, however, and a loss of health or progress is
    // still an exact source-carried observation.
    if (Value != 0)
    {
        Destination = Value;
    }
}

void DescribeKind(
    GameplayFeedbackKind Kind,
    FEchoesFeedbackHistoryRow& OutRow)
{
    OutRow.Severity = EEchoesFeedbackHistorySeverity::Informational;
    OutRow.Disposition = EEchoesFeedbackHistoryDisposition::Transition;
    switch (Kind)
    {
        case GameplayFeedbackKind::Queued:
            OutRow.Category = LOCTEXT("OrdersCategory", "Orders");
            OutRow.Summary = LOCTEXT("Queued", "Order queued");
            OutRow.Disposition = EEchoesFeedbackHistoryDisposition::Queued;
            return;
        case GameplayFeedbackKind::Applied:
            OutRow.Category = LOCTEXT("OrdersCategory", "Orders");
            OutRow.Summary = LOCTEXT("Applied", "Order carried out");
            OutRow.Disposition = EEchoesFeedbackHistoryDisposition::Applied;
            return;
        case GameplayFeedbackKind::NoEffect:
            OutRow.Category = LOCTEXT("OrdersCategory", "Orders");
            OutRow.Summary = LOCTEXT("NoEffect", "Order had no effect");
            OutRow.Severity = EEchoesFeedbackHistorySeverity::Warning;
            OutRow.Disposition = EEchoesFeedbackHistoryDisposition::NoEffect;
            return;
        case GameplayFeedbackKind::ConstructionCreated:
            OutRow.Category = LOCTEXT("ConstructionCategory", "Construction");
            OutRow.Summary = LOCTEXT("ConstructionCreated", "Construction created");
            return;
        case GameplayFeedbackKind::ConstructionProgressed:
            OutRow.Category = LOCTEXT("ConstructionCategory", "Construction");
            OutRow.Summary = LOCTEXT("ConstructionProgressed", "Construction progressed");
            return;
        case GameplayFeedbackKind::ConstructionCompleted:
            OutRow.Category = LOCTEXT("ConstructionCategory", "Construction");
            OutRow.Summary = LOCTEXT("ConstructionCompleted", "Construction completed");
            return;
        case GameplayFeedbackKind::ConstructionCancelled:
            OutRow.Category = LOCTEXT("ConstructionCategory", "Construction");
            OutRow.Summary = LOCTEXT("ConstructionCancelled", "Construction cancelled");
            return;
        case GameplayFeedbackKind::RepairApplied:
            OutRow.Category = LOCTEXT("ConstructionCategory", "Construction");
            OutRow.Summary = LOCTEXT("RepairApplied", "Repair applied");
            return;
        case GameplayFeedbackKind::ProductionQueued:
            OutRow.Category = LOCTEXT("ProductionCategory", "Production");
            OutRow.Summary = LOCTEXT("ProductionQueued", "Production queued");
            return;
        case GameplayFeedbackKind::ProductionActivated:
            OutRow.Category = LOCTEXT("ProductionCategory", "Production");
            OutRow.Summary = LOCTEXT("ProductionActivated", "Production activated");
            return;
        case GameplayFeedbackKind::ProductionCancelled:
            OutRow.Category = LOCTEXT("ProductionCategory", "Production");
            OutRow.Summary = LOCTEXT("ProductionCancelled", "Production cancelled");
            return;
        case GameplayFeedbackKind::ProductionCompleted:
            OutRow.Category = LOCTEXT("ProductionCategory", "Production");
            OutRow.Summary = LOCTEXT("ProductionCompleted", "Production completed");
            return;
        case GameplayFeedbackKind::ProductionSpawnBlocked:
            OutRow.Category = LOCTEXT("ProductionCategory", "Production");
            OutRow.Summary = LOCTEXT("ProductionSpawnBlocked", "Production emergence blocked");
            OutRow.Severity = EEchoesFeedbackHistorySeverity::Warning;
            return;
        case GameplayFeedbackKind::ProductionSpawnResumed:
            OutRow.Category = LOCTEXT("ProductionCategory", "Production");
            OutRow.Summary = LOCTEXT("ProductionSpawnResumed", "Production emergence resumed");
            return;
    }
}

FEchoesFeedbackHistoryRow MakeRow(const GameplayFeedbackEvent& Event)
{
    FEchoesFeedbackHistoryRow Row;
    Row.Generation = Event.generation;
    Row.EventId = Event.eventId;
    Row.ObservedTick = Event.tick;
    Row.ActorEntityId = Event.actor;
    Row.TargetEntityId = Event.target;
    Row.ProducerEntityId = Event.producer;
    Row.ProductionItemId = Event.itemId;
    AddCarried(Row.MatterSpent, Event.resourcesSpent.material);
    AddCarried(Row.DawnSpent, Event.resourcesSpent.dawnshards);
    AddCarried(Row.MatterRefunded, Event.resourcesRefunded.material);
    AddCarried(Row.DawnRefunded, Event.resourcesRefunded.dawnshards);
    AddCarried(Row.HealthDelta, Event.healthDelta);
    AddCarried(Row.ProgressDelta, Event.progressDelta);
    AddCarried(Row.LogisticsDelta, Event.logisticsDelta);
    DescribeKind(Event.kind, Row);
    return Row;
}

bool HasHistoryGap(const echoes::feedback::GameplayFeedbackLoss& Loss)
{
    return Loss.reseedRequired || Loss.retainedHistoryWasTruncated ||
        Loss.evictedHistoryEvents > 0 || Loss.untrackedCommands > 0 ||
        Loss.lostTransientReceipts > 0 || Loss.missedObservationTicks > 0;
}
} // namespace

FEchoesFeedbackHistoryView FEchoesFeedbackHistoryModel::Build(
    const echoes::feedback::GameplayFeedbackState& State,
    echoes::sim::PlayerId CurrentRecipient,
    EEchoesFeedbackHistoryFilter Filter)
{
    FEchoesFeedbackHistoryView View;
    View.Filter = Filter;
    if (CurrentRecipient >= echoes::sim::kMaximumPlayers ||
        State.Recipient() != CurrentRecipient)
    {
        View.bRecipientRejected = true;
        View.HistoryNotice = LOCTEXT("RecipientUnavailable",
            "Feedback history is unavailable for this player.");
        return View;
    }

    for (const GameplayFeedbackEvent& Event : State.History())
    {
        // The state normally guarantees this. Keep it at the presentation
        // boundary so an invalid adapter source cannot disclose another
        // player's entity IDs, results, or resource observations.
        if (Event.recipient != CurrentRecipient)
        {
            View.bRecipientRejected = true;
            continue;
        }
        ++View.EligibleEventCount;
        if (!MatchesFilter(Event.kind, Filter))
        {
            ++View.FilteredOutCount;
            continue;
        }
        View.Rows.Add(MakeRow(Event));
    }

    View.FilterSummary = Filter == EEchoesFeedbackHistoryFilter::All
        ? FText::Format(LOCTEXT("AllFilterSummary", "Showing {0} feedback entries."),
            FText::AsNumber(View.EligibleEventCount))
        : FText::Format(LOCTEXT("FilterSummary", "Showing {0} of {1} feedback entries."),
            FText::AsNumber(View.Rows.Num()), FText::AsNumber(View.EligibleEventCount));

    View.bHistoryIncomplete = HasHistoryGap(State.Loss());
    if (View.bRecipientRejected)
    {
        View.HistoryNotice = LOCTEXT("PartialRecipientUnavailable",
            "Some feedback history is unavailable for this player.");
    }
    else if (State.Loss().retainedHistoryWasTruncated ||
        State.Loss().evictedHistoryEvents > 0)
    {
        View.HistoryNotice = LOCTEXT("RetainedHistoryTruncated",
            "Some earlier feedback is no longer retained.");
    }
    else if (View.bHistoryIncomplete)
    {
        View.HistoryNotice = LOCTEXT("FeedbackIncomplete",
            "Some feedback observations are unavailable.");
    }
    return View;
}

#undef LOCTEXT_NAMESPACE
