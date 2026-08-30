#pragma once

#include "CoreMinimal.h"

/** Player-facing operation selected before deployment. */
enum class EEchoesOperationMode : uint8
{
    Skirmish,
    CampaignPrologue,
    CampaignSevenAccounts,
    CampaignCityReserve,
    CampaignUnburiedRoad,
    CampaignTermsOfContinuance,
    CampaignNamesWithoutBirths,
    CampaignShapeOfSilence,
    CampaignShapeBesideUs,
    CampaignReserveAuthority,
    CampaignChoirAtLumeReach,
    CampaignNoNeutralLedger,
    CampaignFutureThatWon,
    CampaignAssemblyOfTheMissing,
    CampaignSeveralVoicesOneCommand
};

/** Reconstructable objective state for What the Ledger Keeps. */
enum class EEchoesProloguePhase : uint8
{
    Inactive,
    RecoverArchive,
    DecideFutureWell,
    Withdraw,
    Complete,
    Failed
};

struct FEchoesPrologueMissionFacts final
{
    bool bOperationActive = false;
    bool bLocalCoreIntact = false;
    bool bArchiveCarrierIntact = false;
    bool bArchiveCarrierAtRecoverySite = false;
    bool bFutureWellProtocolChosen = false;
    bool bFutureWellLost = false;
    bool bArchiveCarrierAtEvacuationSite = false;
    bool bSkirmishStillOngoing = true;
};

/** Pure campaign objective reducer; it owns no simulation authority. */
struct FEchoesPrologueMissionModel final
{
    [[nodiscard]] static EEchoesProloguePhase DeterminePhase(
        const FEchoesPrologueMissionFacts& Facts);
    [[nodiscard]] static const TCHAR* StableName(EEchoesProloguePhase Phase);
};
