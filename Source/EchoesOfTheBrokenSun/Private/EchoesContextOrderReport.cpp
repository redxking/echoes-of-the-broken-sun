#include "EchoesContextOrderReport.h"

namespace
{
/**
 * Names the substituted moves by the facts that were observed about them, and
 * nothing else. A worker with an empty hold and a unit that is not a worker
 * are different observations, so they are never merged into one claim.
 */
[[nodiscard]] FString ComposeSubstitutionBreakdown(
    const FEchoesContextOrderOutcome& Outcome)
{
    if (Outcome.MovedCannotCarryCount <= 0)
    {
        return TEXT("with an empty hold");
    }
    if (Outcome.MovedNotCarryingCount <= 0)
    {
        return TEXT("unable to carry Matter");
    }
    return FString::Printf(
        TEXT("%d with an empty hold, %d unable to carry Matter"),
        Outcome.MovedNotCarryingCount,
        Outcome.MovedCannotCarryCount);
}

/** True when the breakdown is a list rather than a single trailing phrase. */
[[nodiscard]] bool IsMixedSubstitution(
    const FEchoesContextOrderOutcome& Outcome)
{
    return Outcome.MovedNotCarryingCount > 0 &&
           Outcome.MovedCannotCarryCount > 0;
}
}

void FEchoesContextOrderOutcome::RecordRejection(
    uint32 EntityId,
    const FString& AuthorityReason)
{
    const bool bFirstRejection = RejectedCount <= 0;
    ++RejectedCount;
    if (!bFirstRejection && EntityId >= RejectionEntityId)
    {
        return;
    }
    RejectionEntityId = EntityId;
    RejectionReason = AuthorityReason;
}

FString FEchoesContextOrderReport::ComposeRejectionSuffix(
    const FEchoesContextOrderOutcome& Outcome)
{
    if (Outcome.RejectedCount <= 0)
    {
        return TEXT(".");
    }
    // SIM-003: the rejection carries the authority's own stable reason code and
    // plain-language recovery. Quote it rather than composing a parallel
    // explanation here, which would risk naming a cause the authority never
    // gave. An authority that returned no text is reported as no text, never
    // as a guess.
    return Outcome.RejectionReason.IsEmpty()
               ? FString::Printf(
                     TEXT(", %d rejected."),
                     Outcome.RejectedCount)
               : FString::Printf(
                     TEXT(", %d rejected. %s"),
                     Outcome.RejectedCount,
                     *Outcome.RejectionReason);
}

FString FEchoesContextOrderReport::ComposeDeliverBanner(
    const FEchoesContextOrderOutcome& Outcome)
{
    const FString Suffix = ComposeRejectionSuffix(Outcome);
    const int32 MovedCount = Outcome.SubstitutedMoveCount();
    if (Outcome.DeliveredCount > 0 && MovedCount > 0)
    {
        return FString::Printf(
            TEXT("DELIVER MATTER: %d queued, %d moved to the drop-off%s %s%s"),
            Outcome.DeliveredCount,
            MovedCount,
            IsMixedSubstitution(Outcome) ? TEXT(":") : TEXT(""),
            *ComposeSubstitutionBreakdown(Outcome),
            *Suffix);
    }
    if (MovedCount <= 0)
    {
        // Nothing was substituted, so the only things this banner can state are
        // the count it queued and whatever the authority said about the rest.
        return FString::Printf(
            TEXT("DELIVER MATTER: %d queued%s"),
            Outcome.DeliveredCount,
            *Suffix);
    }
    // Nothing was delivered. The banner reports the moves it made and what was
    // observed about each of them. It does not claim that no selected worker
    // was carrying Matter: a unit the authority refused was never examined for
    // cargo here, so that cause was never established.
    return FString::Printf(
        TEXT("MOVE TO DROP-OFF: %d queued%s %s%s"),
        MovedCount,
        IsMixedSubstitution(Outcome) ? TEXT(":") : TEXT(""),
        *ComposeSubstitutionBreakdown(Outcome),
        *Suffix);
}

FString FEchoesContextOrderReport::ComposeOrderBanner(
    const FString& OrderLabel,
    const FEchoesContextOrderOutcome& Outcome)
{
    return FString::Printf(
        TEXT("%s: %d queued%s"),
        *OrderLabel,
        Outcome.AcceptedCount(),
        *ComposeRejectionSuffix(Outcome));
}
