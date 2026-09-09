#pragma once

#include "CoreMinimal.h"
#include "EchoesGameplayFeedback.h"
#include "EchoesGameplayFeedbackPacket.generated.h"

/** Recipient-owned presentation evidence. Never a command or saved game state. */
USTRUCT()
struct FEchoesGameplayFeedbackPacket
{
    GENERATED_BODY()

    UPROPERTY() uint64 Generation = 0;
    UPROPERTY() uint64 EventId = 0;
    UPROPERTY() uint8 Recipient = 0;
    UPROPERTY() uint64 Tick = 0;
    UPROPERTY() uint8 Kind = 0;
    UPROPERTY() uint8 CommandType = 0;
    UPROPERTY() uint8 ResolutionOutcome = 0;
    UPROPERTY() uint64 Sequence = 0;
    UPROPERTY() uint32 Actor = 0;
    UPROPERTY() uint32 Target = 0;
    UPROPERTY() uint32 Producer = 0;
    UPROPERTY() uint64 ItemId = 0;
    UPROPERTY() uint8 EntityType = 0;
    UPROPERTY() uint8 ProductionBlockReason = 0;
    UPROPERTY() int64 MatterSpent = 0;
    UPROPERTY() int64 DawnshardsSpent = 0;
    UPROPERTY() int64 MatterRefunded = 0;
    UPROPERTY() int64 DawnshardsRefunded = 0;
    UPROPERTY() int64 HealthDelta = 0;
    UPROPERTY() int64 ProgressDelta = 0;
    UPROPERTY() int64 LogisticsDelta = 0;

    static FEchoesGameplayFeedbackPacket FromEvent(
        const echoes::feedback::GameplayFeedbackEvent& Event);
    echoes::feedback::GameplayFeedbackEvent ToEvent() const;
};

/** Explicit bounded-history/observation loss, separate from action outcomes. */
USTRUCT()
struct FEchoesGameplayFeedbackLossPacket
{
    GENERATED_BODY()

    UPROPERTY() bool bReseedRequired = false;
    UPROPERTY() bool bRetainedHistoryTruncated = false;
    UPROPERTY() uint64 EvictedHistoryEvents = 0;
    UPROPERTY() uint64 UntrackedCommands = 0;
    UPROPERTY() uint64 LostTransientReceipts = 0;
    UPROPERTY() uint64 MissedObservationTicks = 0;

    static FEchoesGameplayFeedbackLossPacket FromLoss(
        const echoes::feedback::GameplayFeedbackLoss& Loss);
    echoes::feedback::GameplayFeedbackLoss ToLoss() const;
};
