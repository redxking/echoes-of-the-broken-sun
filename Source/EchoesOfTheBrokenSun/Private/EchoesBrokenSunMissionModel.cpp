#include "EchoesBrokenSunMissionModel.h"

#include "EchoesAssemblyOfTheMissingMissionModel.h"
#include "EchoesFutureThatWonMissionModel.h"

EEchoesBrokenSunPhase FEchoesBrokenSunMissionModel::DeterminePhase(
    const FEchoesBrokenSunMissionFacts& Facts)
{
    if (!Facts.bOperationActive)
    {
        return EEchoesBrokenSunPhase::Inactive;
    }
    if (!Facts.bLocalCoreIntact || !Facts.bMaraIntact ||
        !Facts.bOruunIntact || !Facts.bTalarIntact ||
        !Facts.bNemeIntact || !Facts.bCommandForceIntact ||
        !Facts.bSkirmishStillOngoing ||
        Facts.bResolutionContractFailed)
    {
        return EEchoesBrokenSunPhase::Failed;
    }

    const bool bAccordEstablished =
        Facts.bMeridianAccordEstablished &&
        Facts.bKharuunAccordEstablished &&
        Facts.bChoirAccordEstablished;
    const bool bResolutionSelected =
        Facts.SelectedResolution != EEchoesFinalResolution::None;
    if ((bResolutionSelected &&
         (!Facts.bApproachAnchorComplete || !bAccordEstablished ||
          !Facts.bSelectedResolutionEligible)) ||
        (Facts.bResolutionConduitComplete && !bResolutionSelected) ||
        (Facts.bResolutionWindowHeld &&
         (!bResolutionSelected || !Facts.bResolutionConduitComplete)))
    {
        return EEchoesBrokenSunPhase::Failed;
    }
    if (!Facts.bApproachAnchorComplete)
    {
        return EEchoesBrokenSunPhase::SecureCrownfallApproach;
    }
    if (!bAccordEstablished)
    {
        return EEchoesBrokenSunPhase::AssembleAccord;
    }
    if (!bResolutionSelected)
    {
        return EEchoesBrokenSunPhase::ChooseFinalResolution;
    }
    if (!Facts.bResolutionConduitComplete)
    {
        return EEchoesBrokenSunPhase::RaiseResolutionConduit;
    }
    if (!Facts.bResolutionWindowHeld)
    {
        return EEchoesBrokenSunPhase::HoldFinalResolution;
    }
    return EEchoesBrokenSunPhase::Complete;
}

const TCHAR* FEchoesBrokenSunMissionModel::StableName(
    EEchoesBrokenSunPhase Phase)
{
    switch (Phase)
    {
        case EEchoesBrokenSunPhase::Inactive: return TEXT("inactive");
        case EEchoesBrokenSunPhase::SecureCrownfallApproach:
            return TEXT("secure_crownfall_approach");
        case EEchoesBrokenSunPhase::AssembleAccord:
            return TEXT("assemble_accord");
        case EEchoesBrokenSunPhase::ChooseFinalResolution:
            return TEXT("choose_final_resolution");
        case EEchoesBrokenSunPhase::RaiseResolutionConduit:
            return TEXT("raise_resolution_conduit");
        case EEchoesBrokenSunPhase::HoldFinalResolution:
            return TEXT("hold_final_resolution");
        case EEchoesBrokenSunPhase::Complete: return TEXT("complete");
        case EEchoesBrokenSunPhase::Failed: return TEXT("failed");
    }
    return TEXT("unknown");
}

bool FEchoesBrokenSunMissionModel::TryPlanForLedger(
    echoes::sim::FutureWellChoice FoundingDoctrine,
    uint8 ReserveAuthorityFacts,
    echoes::sim::FutureWellChoice RecordedProtocol,
    FEchoesBrokenSunPlan& OutPlan)
{
    FEchoesSeveralVoicesOneCommandPlan VoicesPlan;
    FEchoesAssemblyOfTheMissingPlan AssemblyPlan;
    FEchoesFutureThatWonPlan ActivationPlan;
    if (!FEchoesSeveralVoicesOneCommandMissionModel::TryPlanForLedger(
            FoundingDoctrine,
            ReserveAuthorityFacts,
            RecordedProtocol,
            VoicesPlan) ||
        !FEchoesAssemblyOfTheMissingMissionModel::TryPlanForLedger(
            FoundingDoctrine,
            ReserveAuthorityFacts,
            RecordedProtocol,
            AssemblyPlan) ||
        !FEchoesFutureThatWonMissionModel::TryPlanForLedger(
            FoundingDoctrine,
            ReserveAuthorityFacts,
            RecordedProtocol,
            ActivationPlan))
    {
        OutPlan = {};
        return false;
    }

    OutPlan = {};
    OutPlan.FoundingDoctrine = FoundingDoctrine;
    OutPlan.RecordedProtocol = RecordedProtocol;
    OutPlan.FirstContributingDistrict =
        AssemblyPlan.FirstContributingDistrict;
    OutPlan.SecondContributingDistrict =
        AssemblyPlan.SecondContributingDistrict;
    OutPlan.DeferredDistrict = AssemblyPlan.DeferredDistrict;
    OutPlan.MaraAccordSite = VoicesPlan.PossibleVoiceSite;
    OutPlan.OruunAccordSite = VoicesPlan.ManifestVoiceSite;
    OutPlan.NemeAccordSite = VoicesPlan.NemeCommandSite;
    OutPlan.TalarPublicRecordSite =
        AssemblyPlan.MeridianPublicRecordSite;
    OutPlan.CrownfallApproachSite = VoicesPlan.CrisisAnchorSite;
    // The inherited rally site is already the Mission 14 crisis anchor. The
    // separate public Future Well demonstrator is the final convergence so
    // one structure can never satisfy both ordered objectives.
    OutPlan.FinalConvergenceSite =
        ActivationPlan.RestorationDemonstratorSite;
    OutPlan.StablePlanKey = VoicesPlan.StablePlanKey;
    OutPlan.RouteStableName = VoicesPlan.RouteStableName;
    OutPlan.RouteDisplayName = VoicesPlan.RouteDisplayName;
    OutPlan.ProtocolStableName = VoicesPlan.ProtocolStableName;
    OutPlan.ProtocolDisplayName = VoicesPlan.ProtocolDisplayName;

    OutPlan.AvailableFinalResolutions = static_cast<uint8>(
        EEchoesFinalResolutionAvailability::ControlledStabilization);
    const uint8 LifeSupportBit = static_cast<uint8>(
        EEchoesReserveAuthorityCompletionFact::LifeSupportPowered);
    if (RecordedProtocol == echoes::sim::FutureWellChoice::Preserve &&
        (ReserveAuthorityFacts & LifeSupportBit) != 0)
    {
        OutPlan.AvailableFinalResolutions |= static_cast<uint8>(
            EEchoesFinalResolutionAvailability::Restoration);
    }
    if (FoundingDoctrine == echoes::sim::FutureWellChoice::Harvest ||
        RecordedProtocol == echoes::sim::FutureWellChoice::Harvest)
    {
        OutPlan.AvailableFinalResolutions |= static_cast<uint8>(
            EEchoesFinalResolutionAvailability::Extinguishment);
    }
    if (FoundingDoctrine == echoes::sim::FutureWellChoice::Reshape ||
        RecordedProtocol == echoes::sim::FutureWellChoice::Reshape)
    {
        OutPlan.AvailableFinalResolutions |= static_cast<uint8>(
            EEchoesFinalResolutionAvailability::OpenEvolution);
    }

    switch (OutPlan.DeferredDistrict)
    {
        case EEchoesCityDistrict::LifeSupport:
            OutPlan.ResolutionHoldTicks = 320;
            break;
        case EEchoesCityDistrict::Transit:
            OutPlan.ResolutionHoldTicks = 280;
            break;
        case EEchoesCityDistrict::Archive:
            OutPlan.ResolutionHoldTicks = 240;
            break;
    }
    return OutPlan.AvailableFinalResolutions != 0 &&
        OutPlan.ResolutionHoldTicks != 0;
}

uint8 FEchoesBrokenSunMissionModel::ResolutionMask(
    EEchoesFinalResolution Resolution)
{
    switch (Resolution)
    {
        case EEchoesFinalResolution::Restoration:
            return static_cast<uint8>(
                EEchoesFinalResolutionAvailability::Restoration);
        case EEchoesFinalResolution::ControlledStabilization:
            return static_cast<uint8>(
                EEchoesFinalResolutionAvailability::ControlledStabilization);
        case EEchoesFinalResolution::Extinguishment:
            return static_cast<uint8>(
                EEchoesFinalResolutionAvailability::Extinguishment);
        case EEchoesFinalResolution::OpenEvolution:
            return static_cast<uint8>(
                EEchoesFinalResolutionAvailability::OpenEvolution);
        case EEchoesFinalResolution::None:
            return 0;
    }
    return 0;
}

bool FEchoesBrokenSunMissionModel::IsResolutionAvailable(
    const FEchoesBrokenSunPlan& Plan,
    EEchoesFinalResolution Resolution)
{
    const uint8 Mask = ResolutionMask(Resolution);
    return Mask != 0 && (Plan.AvailableFinalResolutions & Mask) != 0;
}

echoes::sim::Vec2 FEchoesBrokenSunMissionModel::ResolutionConvergenceSite(
    const FEchoesBrokenSunPlan& Plan,
    EEchoesFinalResolution Resolution)
{
    const int32 BaseX = Plan.FinalConvergenceSite.x.FloorToInt();
    const int32 BaseY = Plan.FinalConvergenceSite.y.FloorToInt();
    using echoes::sim::Vec2;
    switch (Resolution)
    {
        case EEchoesFinalResolution::Restoration:
            return Vec2::FromTiles(BaseX, BaseY);
        case EEchoesFinalResolution::ControlledStabilization:
            return Vec2::FromTiles(BaseX, BaseY - 5);
        case EEchoesFinalResolution::Extinguishment:
            return Vec2::FromTiles(BaseX - 6, BaseY);
        case EEchoesFinalResolution::OpenEvolution:
            return Vec2::FromTiles(BaseX + 6, BaseY);
        case EEchoesFinalResolution::None:
            return {};
    }
    return {};
}

uint64 FEchoesBrokenSunMissionModel::ResolutionHoldTicks(
    const FEchoesBrokenSunPlan& Plan,
    EEchoesFinalResolution Resolution)
{
    switch (Resolution)
    {
        case EEchoesFinalResolution::Restoration:
            return Plan.ResolutionHoldTicks + 80;
        case EEchoesFinalResolution::ControlledStabilization:
            return Plan.ResolutionHoldTicks;
        case EEchoesFinalResolution::Extinguishment:
            return Plan.ResolutionHoldTicks + 40;
        case EEchoesFinalResolution::OpenEvolution:
            return Plan.ResolutionHoldTicks + 120;
        case EEchoesFinalResolution::None:
            return 0;
    }
    return 0;
}

const TCHAR* FEchoesBrokenSunMissionModel::ResolutionStableName(
    EEchoesFinalResolution Resolution)
{
    switch (Resolution)
    {
        case EEchoesFinalResolution::Restoration:
            return TEXT("restoration");
        case EEchoesFinalResolution::ControlledStabilization:
            return TEXT("controlled_stabilization");
        case EEchoesFinalResolution::Extinguishment:
            return TEXT("extinguishment");
        case EEchoesFinalResolution::OpenEvolution:
            return TEXT("open_evolution");
        case EEchoesFinalResolution::None:
            return TEXT("none");
    }
    return TEXT("unknown");
}

const TCHAR* FEchoesBrokenSunMissionModel::ResolutionDisplayName(
    EEchoesFinalResolution Resolution)
{
    switch (Resolution)
    {
        case EEchoesFinalResolution::Restoration:
            return TEXT("RESTORATION");
        case EEchoesFinalResolution::ControlledStabilization:
            return TEXT("CONTROLLED STABILIZATION");
        case EEchoesFinalResolution::Extinguishment:
            return TEXT("EXTINGUISHMENT");
        case EEchoesFinalResolution::OpenEvolution:
            return TEXT("OPEN EVOLUTION");
        case EEchoesFinalResolution::None:
            return TEXT("NO RESOLUTION");
    }
    return TEXT("UNKNOWN RESOLUTION");
}

const TCHAR* FEchoesBrokenSunMissionModel::ResolutionCostSummary(
    EEchoesFinalResolution Resolution)
{
    switch (Resolution)
    {
        case EEchoesFinalResolution::Restoration:
            return TEXT("One stable future; unrealized alternatives close.");
        case EEchoesFinalResolution::ControlledStabilization:
            return TEXT("The Crownfall is bounded; authority remains concentrated.");
        case EEchoesFinalResolution::Extinguishment:
            return TEXT("Well access ends with its power and possible knowledge.");
        case EEchoesFinalResolution::OpenEvolution:
            return TEXT("Possible futures coexist with unresolved systemic risk.");
        case EEchoesFinalResolution::None:
            return TEXT("No final commitment has been made.");
    }
    return TEXT("The cost is unknown.");
}
