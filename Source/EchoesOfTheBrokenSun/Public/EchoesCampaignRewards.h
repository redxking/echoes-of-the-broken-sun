// Copyright Echoes of the Broken Sun. All Rights Reserved.
// Author: Angelis Pseftis

#pragma once

#include "CoreMinimal.h"
#include "EchoesCampaignProgress.h"
#include "EchoesPrologueMissionModel.h"

/**
 * Pure metadata representing persistent rewards unlocked upon completing a campaign mission.
 * Presentation-only: reads durable campaign progress ledger without modifying simulation state.
 */
struct ECHOESOFTHEBROKENSUN_API FEchoesMissionReward final
{
    EEchoesCampaignMissionId MissionId = EEchoesCampaignMissionId::WhatTheLedgerKeeps;
    FString MissionCode;
    FString Title;
    FString BiomeName;
    FString SkirmishMapUnlock;
    FString FactionUnlock;
    FString DoctrineUnlock;
    FString CodexUnlock;
    FString Summary;
};

class ECHOESOFTHEBROKENSUN_API FEchoesCampaignRewards final
{
public:
    /** Returns authored persistent rewards for a mission ID. */
    [[nodiscard]] static const FEchoesMissionReward* GetReward(
        EEchoesCampaignMissionId MissionId);

    /** Returns authored persistent rewards for an operation mode. */
    [[nodiscard]] static const FEchoesMissionReward* GetRewardForOperation(
        EEchoesOperationMode Operation);

    /** Returns all 15 campaign mission reward definitions. */
    [[nodiscard]] static TArray<FEchoesMissionReward> GetAllRewards();

    /** Returns all rewards unlocked by the durable progress ledger. */
    [[nodiscard]] static TArray<FEchoesMissionReward> GetUnlockedRewards(
        const FEchoesCampaignProgress& Progress);

    /** Checks whether a mission's rewards have been earned in the ledger. */
    [[nodiscard]] static bool IsRewardUnlocked(
        EEchoesCampaignMissionId MissionId,
        const FEchoesCampaignProgress& Progress);
};
