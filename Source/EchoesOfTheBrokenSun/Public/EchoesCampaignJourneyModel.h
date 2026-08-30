#pragma once

#include "CoreMinimal.h"
#include "EchoesCampaignProgress.h"
#include "EchoesPrologueMissionModel.h"

/** Result of resolving the next player-facing campaign step. */
enum class EEchoesCampaignJourneyState : uint8
{
    Unavailable,
    Ready,
    Complete
};

/**
 * Pure ordered-ledger to campaign-operation projection.
 *
 * The caller remains responsible for supplying a validated campaign ledger and
 * applying the operation's authoritative admission rules before deployment.
 */
struct ECHOESOFTHEBROKENSUN_API FEchoesCampaignJourney final
{
    EEchoesCampaignJourneyState State =
        EEchoesCampaignJourneyState::Unavailable;
    EEchoesOperationMode NextOperation = EEchoesOperationMode::Skirmish;
    int32 CompletedMissionCount = 0;
};

struct ECHOESOFTHEBROKENSUN_API FEchoesCampaignJourneyModel final
{
    [[nodiscard]] static FEchoesCampaignJourney Resolve(
        const FEchoesCampaignProgress& Progress);

    [[nodiscard]] static const TCHAR* OperationDisplayName(
        EEchoesOperationMode Operation);
};
