// Copyright Echoes of the Broken Sun. All Rights Reserved.
// Author: Angelis Pseftis

#if WITH_DEV_AUTOMATION_TESTS

#include "EchoesFeedbackHistoryModel.h"

#include "Misc/AutomationTest.h"

namespace
{
using echoes::feedback::GameplayFeedbackEvent;
using echoes::feedback::GameplayFeedbackKind;
using echoes::feedback::GameplayFeedbackState;
using echoes::feedback::GameplayFeedbackStatus;
using echoes::sim::CommandResolutionOutcome;
using echoes::sim::CommandType;

GameplayFeedbackEvent MakeEvent(
    uint64 Generation,
    uint64 EventId,
    GameplayFeedbackKind Kind)
{
    GameplayFeedbackEvent Event;
    Event.generation = Generation;
    Event.eventId = EventId;
    Event.recipient = 0;
    Event.tick = static_cast<echoes::sim::Tick>(100 + EventId);
    Event.kind = Kind;
    Event.commandType = CommandType::Move;
    Event.sequence = EventId;
    Event.actor = 101;
    Event.target = 202;
    return Event;
}

bool Ingest(
    GameplayFeedbackState& State,
    const GameplayFeedbackEvent& Event)
{
    return State.IngestRemote(Event, 0, Event.generation) ==
        GameplayFeedbackStatus::Accepted;
}
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesFeedbackHistoryModelTest,
    "Echoes.Runtime.Presentation.FeedbackHistoryModel",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesFeedbackHistoryModelTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    constexpr uint64 Generation = 71;
    GameplayFeedbackState State;
    TestEqual(TEXT("Scoped stream begins"),
        State.BeginRemoteStream(Generation, 0, 0, true),
        GameplayFeedbackStatus::Accepted);

    GameplayFeedbackEvent Queued = MakeEvent(Generation, 1, GameplayFeedbackKind::Queued);
    TestTrue(TEXT("Queued source event is retained"), Ingest(State, Queued));

    GameplayFeedbackEvent Applied = MakeEvent(Generation, 2, GameplayFeedbackKind::Applied);
    Applied.resolutionOutcome = CommandResolutionOutcome::Applied;
    TestTrue(TEXT("Applied source event is retained"), Ingest(State, Applied));

    GameplayFeedbackEvent NoEffect = MakeEvent(Generation, 3, GameplayFeedbackKind::NoEffect);
    NoEffect.resolutionOutcome = CommandResolutionOutcome::RouteBlocked;
    TestTrue(TEXT("No-effect source event is retained"), Ingest(State, NoEffect));

    GameplayFeedbackEvent Construction = MakeEvent(
        Generation, 4, GameplayFeedbackKind::ConstructionProgressed);
    Construction.progressDelta = 13;
    Construction.resourcesSpent.material = 22;
    Construction.resourcesSpent.dawnshards = 4;
    TestTrue(TEXT("Construction source event is retained"), Ingest(State, Construction));

    GameplayFeedbackEvent Production = MakeEvent(
        Generation, 5, GameplayFeedbackKind::ProductionSpawnBlocked);
    Production.producer = 303;
    Production.itemId = 404;
    Production.target = 0;
    Production.logisticsDelta = 2;
    Production.productionBlockReason = echoes::sim::ProductionStartBlockReason::Busy;
    TestTrue(TEXT("Production source event is retained"), Ingest(State, Production));

    echoes::feedback::GameplayFeedbackLoss EvictedHistory;
    EvictedHistory.evictedHistoryEvents = 1;
    TestEqual(TEXT("An authoritative ring-history gap is retained"),
        State.ReportRemoteObservationLoss(EvictedHistory, 0, Generation),
        GameplayFeedbackStatus::Accepted);

    const FEchoesFeedbackHistoryView All = FEchoesFeedbackHistoryModel::Build(
        State, 0, EEchoesFeedbackHistoryFilter::All);
    TestFalse(TEXT("Correct recipient is not rejected"), All.bRecipientRejected);
    TestEqual(TEXT("All rows retain each recipient-scoped event"), All.Rows.Num(), 5);
    TestEqual(TEXT("All rows retain stable generation"), All.Rows[0].Generation, Generation);
    TestEqual(TEXT("All rows retain stable event ID"), All.Rows[4].EventId, uint64{5});
    TestTrue(TEXT("A retained-history gap is disclosed"),
        All.bHistoryIncomplete && !All.HistoryNotice.IsEmpty());
    TestTrue(TEXT("Filter status stays explicit"), !All.FilterSummary.IsEmpty());

    TestTrue(TEXT("Queue is not represented as execution"),
        All.Rows[0].Disposition == EEchoesFeedbackHistoryDisposition::Queued &&
        All.Rows[1].Disposition == EEchoesFeedbackHistoryDisposition::Applied &&
        All.Rows[2].Disposition == EEchoesFeedbackHistoryDisposition::NoEffect);
    TestTrue(TEXT("No-effect remains a localized player-facing result"),
        !All.Rows[2].Summary.IsEmpty() &&
        All.Rows[2].Category.EqualTo(All.Rows[0].Category));
    TestTrue(TEXT("Actual carried construction values are exposed"),
        All.Rows[3].MatterSpent.IsSet() && All.Rows[3].MatterSpent.GetValue() == 22 &&
        All.Rows[3].DawnSpent.IsSet() && All.Rows[3].DawnSpent.GetValue() == 4 &&
        All.Rows[3].ProgressDelta.IsSet() && All.Rows[3].ProgressDelta.GetValue() == 13);
    TestTrue(TEXT("Absent fields are not projected as observations"),
        !All.Rows[3].MatterRefunded.IsSet() && !All.Rows[3].HealthDelta.IsSet());
    TestTrue(TEXT("Production source IDs and exact logistics are retained"),
        All.Rows[4].ProducerEntityId == 303 && All.Rows[4].ProductionItemId == 404 &&
        All.Rows[4].LogisticsDelta.IsSet() && All.Rows[4].LogisticsDelta.GetValue() == 2);

    const FEchoesFeedbackHistoryView Orders = FEchoesFeedbackHistoryModel::Build(
        State, 0, EEchoesFeedbackHistoryFilter::Orders);
    const FEchoesFeedbackHistoryView ConstructionOnly = FEchoesFeedbackHistoryModel::Build(
        State, 0, EEchoesFeedbackHistoryFilter::Construction);
    const FEchoesFeedbackHistoryView ProductionOnly = FEchoesFeedbackHistoryModel::Build(
        State, 0, EEchoesFeedbackHistoryFilter::Production);
    TestTrue(TEXT("Order filter preserves all three distinct order states"),
        Orders.Rows.Num() == 3 && Orders.FilteredOutCount == 2 &&
        !Orders.FilterSummary.IsEmpty());
    TestTrue(TEXT("Construction filter accounts for filtered source records"),
        ConstructionOnly.Rows.Num() == 1 && ConstructionOnly.FilteredOutCount == 4 &&
        ConstructionOnly.Rows[0].EventId == 4);
    TestTrue(TEXT("Production filter accounts for filtered source records"),
        ProductionOnly.Rows.Num() == 1 && ProductionOnly.FilteredOutCount == 4 &&
        ProductionOnly.Rows[0].EventId == 5);

    GameplayFeedbackState OtherRecipient;
    TestEqual(TEXT("Other recipient stream begins"),
        OtherRecipient.BeginRemoteStream(Generation, 1, 0),
        GameplayFeedbackStatus::Accepted);
    GameplayFeedbackEvent Foreign = MakeEvent(Generation, 1, GameplayFeedbackKind::Queued);
    Foreign.recipient = 1;
    TestEqual(TEXT("Other recipient event is accepted by its own stream"),
        OtherRecipient.IngestRemote(Foreign, 1, Generation),
        GameplayFeedbackStatus::Accepted);
    const FEchoesFeedbackHistoryView Rejected = FEchoesFeedbackHistoryModel::Build(
        OtherRecipient, 0, EEchoesFeedbackHistoryFilter::All);
    TestTrue(TEXT("Wrong recipient source is rejected without rows"),
        Rejected.bRecipientRejected && Rejected.Rows.IsEmpty() &&
        !Rejected.HistoryNotice.IsEmpty());

    return !HasAnyErrors();
}

#endif
