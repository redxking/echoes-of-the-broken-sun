#include "EchoesFutureThatWonMissionModel.h"

EEchoesFutureThatWonPhase
FEchoesFutureThatWonMissionModel::DeterminePhase(
    const FEchoesFutureThatWonMissionFacts& Facts)
{
    if (!Facts.bOperationActive)
    {
        return EEchoesFutureThatWonPhase::Inactive;
    }
    if (!Facts.bLocalCoreIntact || !Facts.bOruunIntact ||
        !Facts.bVerifierIntact || !Facts.bFutureWellIntact ||
        !Facts.bPublicInterfacesIntact || !Facts.bSkirmishStillOngoing)
    {
        return EEchoesFutureThatWonPhase::Failed;
    }
    const bool bBothInputs =
        Facts.bFirstRecordedInputVerified &&
        Facts.bSecondRecordedInputVerified;
    if (Facts.bConflictingProtocolBound ||
        (Facts.bRecordedProtocolBound &&
         (!Facts.bIndependentPublicReadbackEstablished || !bBothInputs)))
    {
        return EEchoesFutureThatWonPhase::Failed;
    }
    // Reshape's terrain effect is intentionally temporary. Mission 12 holds
    // the attributable apparatus and activation receipt, not transformed
    // terrain, so normal effect expiry cannot invalidate the bounded readback.
    if (!Facts.bIndependentPublicReadbackEstablished)
    {
        return EEchoesFutureThatWonPhase::EstablishIndependentReadback;
    }
    if (!bBothInputs)
    {
        return EEchoesFutureThatWonPhase::VerifyRecordedInputs;
    }
    if (!Facts.bRecordedProtocolBound)
    {
        return EEchoesFutureThatWonPhase::BindRecordedProtocol;
    }
    if (!Facts.bStabilityWindowHeld)
    {
        return EEchoesFutureThatWonPhase::HoldStabilityWindow;
    }
    if (!Facts.bFirstDistrictReadbackObserved ||
        !Facts.bSecondDistrictReadbackObserved)
    {
        return EEchoesFutureThatWonPhase::ObserveDistrictReadbacks;
    }
    return EEchoesFutureThatWonPhase::Complete;
}

const TCHAR* FEchoesFutureThatWonMissionModel::StableName(
    EEchoesFutureThatWonPhase Phase)
{
    switch (Phase)
    {
        case EEchoesFutureThatWonPhase::Inactive:
            return TEXT("inactive");
        case EEchoesFutureThatWonPhase::EstablishIndependentReadback:
            return TEXT("establish_independent_readback");
        case EEchoesFutureThatWonPhase::VerifyRecordedInputs:
            return TEXT("verify_recorded_inputs");
        case EEchoesFutureThatWonPhase::BindRecordedProtocol:
            return TEXT("bind_recorded_protocol");
        case EEchoesFutureThatWonPhase::HoldStabilityWindow:
            return TEXT("hold_stability_window");
        case EEchoesFutureThatWonPhase::ObserveDistrictReadbacks:
            return TEXT("observe_district_readbacks");
        case EEchoesFutureThatWonPhase::Complete:
            return TEXT("complete");
        case EEchoesFutureThatWonPhase::Failed:
            return TEXT("failed");
    }
    return TEXT("unknown");
}

bool FEchoesFutureThatWonMissionModel::TryPlanForLedger(
    echoes::sim::FutureWellChoice FoundingDoctrine,
    uint8 ReserveAuthorityFacts,
    echoes::sim::FutureWellChoice RecordedProtocol,
    FEchoesFutureThatWonPlan& OutPlan)
{
    FEchoesNoNeutralLedgerPlan PriorPlan;
    if (!FEchoesNoNeutralLedgerMissionModel::TryPlanForLedger(
            FoundingDoctrine,
            ReserveAuthorityFacts,
            RecordedProtocol,
            PriorPlan))
    {
        OutPlan = {};
        return false;
    }

    OutPlan = {};
    OutPlan.FoundingDoctrine = PriorPlan.FoundingDoctrine;
    OutPlan.RecordedProtocol = PriorPlan.LumeProtocol;
    OutPlan.FirstContributingDistrict =
        PriorPlan.FirstContributingDistrict;
    OutPlan.SecondContributingDistrict =
        PriorPlan.SecondContributingDistrict;
    OutPlan.DeferredDistrict = PriorPlan.DeferredDistrict;
    OutPlan.FirstDistrictInputSite = PriorPlan.FirstDistrictSite;
    OutPlan.SecondDistrictInputSite = PriorPlan.SecondDistrictSite;
    OutPlan.MeridianReadbackSite = PriorPlan.MeridianEvidenceSite;
    OutPlan.KharuunReadbackSite = PriorPlan.KharuunEvidenceSite;
    OutPlan.RestorationDemonstratorSite = PriorPlan.FutureWellSite;
    OutPlan.FutureWellSite = PriorPlan.RallySite;
    OutPlan.StabilityWindowTicks = 300;
    OutPlan.StablePlanKey = PriorPlan.StablePlanKey;
    OutPlan.RouteStableName = PriorPlan.RouteStableName;
    OutPlan.RouteDisplayName = PriorPlan.RouteDisplayName;
    OutPlan.ProtocolStableName = PriorPlan.ProtocolStableName;
    OutPlan.ProtocolDisplayName = PriorPlan.ProtocolDisplayName;
    return true;
}
