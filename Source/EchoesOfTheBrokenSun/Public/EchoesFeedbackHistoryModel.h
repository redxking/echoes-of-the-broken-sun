// Author: Angelis Pseftis
#pragma once

#include "CoreMinimal.h"
#include "EchoesGameplayFeedback.h"

/** The active operator-selected category; filtering never alters source history. */
enum class EEchoesFeedbackHistoryFilter : uint8
{
    All,
    Orders,
    Construction,
    Production,
};

/** Severity is a presentation treatment, never a diagnosis of a command result. */
enum class EEchoesFeedbackHistorySeverity : uint8
{
    Informational,
    Warning,
};

/** Queued, executed, and no-effect evidence stay separately readable. */
enum class EEchoesFeedbackHistoryDisposition : uint8
{
    Queued,
    Applied,
    NoEffect,
    Transition,
};

/**
 * A player-facing, read-only projection of one exact feedback event. Entity
 * IDs are retained for a future, separately-authorized map-jump binding.
 */
struct FEchoesFeedbackHistoryRow final
{
    uint64 Generation = 0;
    uint64 EventId = 0;
    uint64 ObservedTick = 0;
    uint32 ActorEntityId = 0;
    uint32 TargetEntityId = 0;
    uint32 ProducerEntityId = 0;
    uint64 ProductionItemId = 0;
    EEchoesFeedbackHistorySeverity Severity =
        EEchoesFeedbackHistorySeverity::Informational;
    EEchoesFeedbackHistoryDisposition Disposition =
        EEchoesFeedbackHistoryDisposition::Transition;
    FText Category;
    FText Summary;
    TOptional<int64> MatterSpent;
    TOptional<int64> DawnSpent;
    TOptional<int64> MatterRefunded;
    TOptional<int64> DawnRefunded;
    TOptional<int64> HealthDelta;
    TOptional<int64> ProgressDelta;
    TOptional<int64> LogisticsDelta;
};

/**
 * The visible history and an honest retained-history condition. Counts allow
 * a UI to say what a selected filter has hidden without discarding source
 * facts or exposing another recipient's records.
 */
struct FEchoesFeedbackHistoryView final
{
    EEchoesFeedbackHistoryFilter Filter = EEchoesFeedbackHistoryFilter::All;
    TArray<FEchoesFeedbackHistoryRow> Rows;
    int32 EligibleEventCount = 0;
    int32 FilteredOutCount = 0;
    bool bRecipientRejected = false;
    bool bHistoryIncomplete = false;
    FText FilterSummary;
    FText HistoryNotice;
};

/** SPEC-HUD-007 presentation adapter. It consumes, but never changes, feedback state. */
class ECHOESOFTHEBROKENSUN_API FEchoesFeedbackHistoryModel final
{
public:
    static FEchoesFeedbackHistoryView Build(
        const echoes::feedback::GameplayFeedbackState& State,
        echoes::sim::PlayerId CurrentRecipient,
        EEchoesFeedbackHistoryFilter Filter);
};
