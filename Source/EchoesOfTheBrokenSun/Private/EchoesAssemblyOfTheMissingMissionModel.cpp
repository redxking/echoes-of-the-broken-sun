#include "EchoesAssemblyOfTheMissingMissionModel.h"

EEchoesAssemblyOfTheMissingPhase
FEchoesAssemblyOfTheMissingMissionModel::DeterminePhase(
    const FEchoesAssemblyOfTheMissingMissionFacts& Facts)
{
    if (!Facts.bOperationActive)
    {
        return EEchoesAssemblyOfTheMissingPhase::Inactive;
    }
    if (!Facts.bLocalCoreIntact || !Facts.bOruunIntact ||
        !Facts.bVerifierIntact || !Facts.bPublicInterfacesIntact ||
        !Facts.bSkirmishStillOngoing)
    {
        return EEchoesAssemblyOfTheMissingPhase::Failed;
    }
    if (Facts.bCrownfallIndexLinked &&
        !Facts.bPublicRecordReadbackEstablished)
    {
        return EEchoesAssemblyOfTheMissingPhase::Failed;
    }
    if (!Facts.bPublicRecordReadbackEstablished)
    {
        return EEchoesAssemblyOfTheMissingPhase::EstablishPublicRecordReadback;
    }
    if (!Facts.bCrownfallIndexLinked)
    {
        return EEchoesAssemblyOfTheMissingPhase::LinkCrownfallIndex;
    }
    if (!Facts.bMeridianAssemblyWitnessObserved ||
        !Facts.bKharuunAssemblyWitnessObserved)
    {
        return EEchoesAssemblyOfTheMissingPhase::ObserveAssembly;
    }
    return EEchoesAssemblyOfTheMissingPhase::Complete;
}

const TCHAR* FEchoesAssemblyOfTheMissingMissionModel::StableName(
    EEchoesAssemblyOfTheMissingPhase Phase)
{
    switch (Phase)
    {
        case EEchoesAssemblyOfTheMissingPhase::Inactive:
            return TEXT("inactive");
        case EEchoesAssemblyOfTheMissingPhase::EstablishPublicRecordReadback:
            return TEXT("establish_public_record_readback");
        case EEchoesAssemblyOfTheMissingPhase::LinkCrownfallIndex:
            return TEXT("link_crownfall_index");
        case EEchoesAssemblyOfTheMissingPhase::ObserveAssembly:
            return TEXT("observe_assembly");
        case EEchoesAssemblyOfTheMissingPhase::Complete:
            return TEXT("complete");
        case EEchoesAssemblyOfTheMissingPhase::Failed:
            return TEXT("failed");
    }
    return TEXT("unknown");
}

bool FEchoesAssemblyOfTheMissingMissionModel::TryPlanForLedger(
    echoes::sim::FutureWellChoice FoundingDoctrine,
    uint8 ReserveAuthorityFacts,
    echoes::sim::FutureWellChoice RecordedProtocol,
    FEchoesAssemblyOfTheMissingPlan& OutPlan)
{
    FEchoesFutureThatWonPlan PriorPlan;
    if (!FEchoesFutureThatWonMissionModel::TryPlanForLedger(
            FoundingDoctrine,
            ReserveAuthorityFacts,
            RecordedProtocol,
            PriorPlan))
    {
        OutPlan = {};
        return false;
    }

    FEchoesNoNeutralLedgerPlan CoalitionPlan;
    if (!FEchoesNoNeutralLedgerMissionModel::TryPlanForLedger(
            FoundingDoctrine,
            ReserveAuthorityFacts,
            RecordedProtocol,
            CoalitionPlan))
    {
        OutPlan = {};
        return false;
    }

    OutPlan = {};
    OutPlan.FoundingDoctrine = PriorPlan.FoundingDoctrine;
    OutPlan.RecordedProtocol = PriorPlan.RecordedProtocol;
    OutPlan.FirstContributingDistrict =
        PriorPlan.FirstContributingDistrict;
    OutPlan.SecondContributingDistrict =
        PriorPlan.SecondContributingDistrict;
    OutPlan.DeferredDistrict = PriorPlan.DeferredDistrict;
    OutPlan.MeridianPublicRecordSite = PriorPlan.MeridianReadbackSite;
    OutPlan.KharuunPublicRecordSite = PriorPlan.KharuunReadbackSite;
    OutPlan.CrownfallIndexSite = CoalitionPlan.RallySite;
    OutPlan.MeridianAssemblyWitnessSite =
        CoalitionPlan.FirstDistrictSite;
    OutPlan.KharuunAssemblyWitnessSite = CoalitionPlan.RouteSite;
    OutPlan.StablePlanKey = PriorPlan.StablePlanKey;
    OutPlan.RouteStableName = PriorPlan.RouteStableName;
    OutPlan.RouteDisplayName = PriorPlan.RouteDisplayName;
    OutPlan.ProtocolStableName = PriorPlan.ProtocolStableName;
    OutPlan.ProtocolDisplayName = PriorPlan.ProtocolDisplayName;
    return true;
}
