#pragma once

#include "CoreMinimal.h"

/**
 * What one context order actually did, counted per selected unit.
 *
 * SIM-003 and the "Recoverable command" pillar both bind the banner: it states
 * what happened and, when something was refused, the authority's own reason.
 * Every field here is an observation the caller made, never an inference:
 *
 *  - DeliveredCount        accepted Deliver commands.
 *  - MovedNotCarryingCount a worker whose authoritative cargo was zero, so the
 *                          click became a move to the drop-off.
 *  - MovedCannotCarryCount a selected unit that is not a worker at all.
 *  - AcceptedOtherCount    accepted commands of any other order.
 *  - RejectedCount         commands the authority refused.
 *  - RejectionReason       the authority's own reason string, taken from the
 *                          refused unit with the lowest stable identifier.
 *
 * A unit whose authoritative state could not be read is deliberately absent
 * from the substitution counts: a failed lookup establishes nothing about
 * cargo, so that unit is sent to the authority unchanged and is reported
 * through RejectedCount and the authority's own reason.
 */
struct ECHOESOFTHEBROKENSUN_API FEchoesContextOrderOutcome final
{
    int32 DeliveredCount = 0;
    int32 MovedNotCarryingCount = 0;
    int32 MovedCannotCarryCount = 0;
    int32 AcceptedOtherCount = 0;
    int32 RejectedCount = 0;
    FString RejectionReason;
    /** The entity RejectionReason came from; meaningful only when refused. */
    uint32 RejectionEntityId = 0;

    [[nodiscard]] int32 SubstitutedMoveCount() const
    {
        return MovedNotCarryingCount + MovedCannotCarryCount;
    }

    [[nodiscard]] int32 AcceptedCount() const
    {
        return DeliveredCount + SubstitutedMoveCount() + AcceptedOtherCount;
    }

    /**
     * Keeps the reported reason independent of selection order. SIM-005 gives
     * every entity a stable identifier, so the lowest refused identifier always
     * wins and the same click always shows the same reason.
     */
    void RecordRejection(uint32 EntityId, const FString& AuthorityReason);
};

struct ECHOESOFTHEBROKENSUN_API FEchoesContextOrderReport final
{
    /**
     * The banner for a Deliver context order. It never states a cause the
     * caller did not establish: units are counted by what was observed about
     * them, and a refusal is quoted from the authority rather than explained
     * here.
     */
    [[nodiscard]] static FString ComposeDeliverBanner(
        const FEchoesContextOrderOutcome& Outcome);

    /** The banner for every other context order. */
    [[nodiscard]] static FString ComposeOrderBanner(
        const FString& OrderLabel,
        const FEchoesContextOrderOutcome& Outcome);

    /**
     * The trailing clause shared by both. With nothing refused it is a bare
     * full stop; otherwise it quotes the authority verbatim.
     */
    [[nodiscard]] static FString ComposeRejectionSuffix(
        const FEchoesContextOrderOutcome& Outcome);
};
