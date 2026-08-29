#include "EchoesPrologueMissionModel.h"

EEchoesProloguePhase FEchoesPrologueMissionModel::DeterminePhase(
    const FEchoesPrologueMissionFacts& Facts)
{
    if (!Facts.bOperationActive)
    {
        return EEchoesProloguePhase::Inactive;
    }
    if (!Facts.bLocalCoreIntact || !Facts.bArchiveCarrierIntact ||
        Facts.bFutureWellLost ||
        !Facts.bSkirmishStillOngoing)
    {
        return EEchoesProloguePhase::Failed;
    }
    if (Facts.bFutureWellProtocolChosen)
    {
        return Facts.bArchiveCarrierAtEvacuationSite
                   ? EEchoesProloguePhase::Complete
                   : EEchoesProloguePhase::Withdraw;
    }
    return Facts.bArchiveCarrierAtRecoverySite
               ? EEchoesProloguePhase::DecideFutureWell
               : EEchoesProloguePhase::RecoverArchive;
}

const TCHAR* FEchoesPrologueMissionModel::StableName(
    EEchoesProloguePhase Phase)
{
    switch (Phase)
    {
        case EEchoesProloguePhase::Inactive: return TEXT("inactive");
        case EEchoesProloguePhase::RecoverArchive: return TEXT("recover_archive");
        case EEchoesProloguePhase::DecideFutureWell: return TEXT("decide_future_well");
        case EEchoesProloguePhase::Withdraw: return TEXT("withdraw");
        case EEchoesProloguePhase::Complete: return TEXT("complete");
        case EEchoesProloguePhase::Failed: return TEXT("failed");
    }
    return TEXT("unknown");
}
