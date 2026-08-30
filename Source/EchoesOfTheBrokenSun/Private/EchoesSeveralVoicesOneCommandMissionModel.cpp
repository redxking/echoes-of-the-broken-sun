#include "EchoesSeveralVoicesOneCommandMissionModel.h"

EEchoesSeveralVoicesOneCommandPhase
FEchoesSeveralVoicesOneCommandMissionModel::DeterminePhase(
    const FEchoesSeveralVoicesOneCommandMissionFacts& Facts)
{
    if (!Facts.bOperationActive)
    {
        return EEchoesSeveralVoicesOneCommandPhase::Inactive;
    }
    if (!Facts.bLocalCoreIntact || !Facts.bPossibleVoiceIntact ||
        !Facts.bManifestVoiceIntact || !Facts.bNemeIntact ||
        !Facts.bResearchLoomIntact || !Facts.bSkirmishStillOngoing ||
        Facts.bCrisisContractFailed)
    {
        return EEchoesSeveralVoicesOneCommandPhase::Failed;
    }

    const bool bVoicesResolved =
        Facts.bPossibleVoiceResolved && Facts.bPossibleVoiceAtSite &&
        Facts.bManifestVoiceResolved && Facts.bManifestVoiceAtSite &&
        Facts.bNemeAtCommandSite;
    const bool bAnchorContractValid =
        Facts.bHeldAlternativesResearched && bVoicesResolved &&
        Facts.bSharedResolutionResearched;
    if (Facts.bPhaseAnchorComplete && !bAnchorContractValid)
    {
        return EEchoesSeveralVoicesOneCommandPhase::Failed;
    }
    if (!Facts.bHeldAlternativesResearched)
    {
        return EEchoesSeveralVoicesOneCommandPhase::ResearchHeldAlternatives;
    }
    if (!bVoicesResolved)
    {
        return EEchoesSeveralVoicesOneCommandPhase::ResolveIncompatibleVoices;
    }
    if (!Facts.bSharedResolutionResearched)
    {
        return EEchoesSeveralVoicesOneCommandPhase::ResearchSharedResolution;
    }
    if (!Facts.bPhaseAnchorComplete)
    {
        return EEchoesSeveralVoicesOneCommandPhase::AnchorCrisis;
    }
    if (!Facts.bCrisisWindowHeld)
    {
        return EEchoesSeveralVoicesOneCommandPhase::HoldSharedResolution;
    }
    return EEchoesSeveralVoicesOneCommandPhase::Complete;
}

const TCHAR* FEchoesSeveralVoicesOneCommandMissionModel::StableName(
    EEchoesSeveralVoicesOneCommandPhase Phase)
{
    switch (Phase)
    {
        case EEchoesSeveralVoicesOneCommandPhase::Inactive:
            return TEXT("inactive");
        case EEchoesSeveralVoicesOneCommandPhase::ResearchHeldAlternatives:
            return TEXT("research_held_alternatives");
        case EEchoesSeveralVoicesOneCommandPhase::ResolveIncompatibleVoices:
            return TEXT("resolve_incompatible_voices");
        case EEchoesSeveralVoicesOneCommandPhase::ResearchSharedResolution:
            return TEXT("research_shared_resolution");
        case EEchoesSeveralVoicesOneCommandPhase::AnchorCrisis:
            return TEXT("anchor_crisis");
        case EEchoesSeveralVoicesOneCommandPhase::HoldSharedResolution:
            return TEXT("hold_shared_resolution");
        case EEchoesSeveralVoicesOneCommandPhase::Complete:
            return TEXT("complete");
        case EEchoesSeveralVoicesOneCommandPhase::Failed:
            return TEXT("failed");
    }
    return TEXT("unknown");
}

bool FEchoesSeveralVoicesOneCommandMissionModel::TryPlanForLedger(
    echoes::sim::FutureWellChoice FoundingDoctrine,
    uint8 ReserveAuthorityFacts,
    echoes::sim::FutureWellChoice RecordedProtocol,
    FEchoesSeveralVoicesOneCommandPlan& OutPlan)
{
    FEchoesAssemblyOfTheMissingPlan PriorPlan;
    if (!FEchoesAssemblyOfTheMissingMissionModel::TryPlanForLedger(
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
    OutPlan.RecordedProtocol = PriorPlan.RecordedProtocol;
    OutPlan.PossibleVoiceSite = PriorPlan.MeridianAssemblyWitnessSite;
    OutPlan.ManifestVoiceSite = PriorPlan.KharuunAssemblyWitnessSite;
    OutPlan.NemeCommandSite = PriorPlan.KharuunPublicRecordSite;
    OutPlan.CrisisAnchorSite = PriorPlan.CrownfallIndexSite;
    OutPlan.StablePlanKey = PriorPlan.StablePlanKey;
    OutPlan.RouteStableName = PriorPlan.RouteStableName;
    OutPlan.RouteDisplayName = PriorPlan.RouteDisplayName;
    OutPlan.ProtocolStableName = PriorPlan.ProtocolStableName;
    OutPlan.ProtocolDisplayName = PriorPlan.ProtocolDisplayName;
    return true;
}
