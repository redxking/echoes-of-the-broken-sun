#include "EchoesNoNeutralLedgerMissionModel.h"

#include "EchoesCampaignProgress.h"
#include "EchoesUnburiedRoadMissionModel.h"

namespace
{
uint8 ChoiceIndex(echoes::sim::FutureWellChoice Choice)
{
    switch (Choice)
    {
        case echoes::sim::FutureWellChoice::Harvest: return 0;
        case echoes::sim::FutureWellChoice::Preserve: return 1;
        case echoes::sim::FutureWellChoice::Reshape: return 2;
        default: return 0xFF;
    }
}

const TCHAR* ProtocolStableName(echoes::sim::FutureWellChoice Choice)
{
    switch (Choice)
    {
        case echoes::sim::FutureWellChoice::Harvest:
            return TEXT("harvest_conversion");
        case echoes::sim::FutureWellChoice::Preserve:
            return TEXT("preserve_hold");
        case echoes::sim::FutureWellChoice::Reshape:
            return TEXT("reshape_window");
        default:
            return TEXT("unavailable");
    }
}

const TCHAR* ProtocolDisplayName(echoes::sim::FutureWellChoice Choice)
{
    switch (Choice)
    {
        case echoes::sim::FutureWellChoice::Harvest:
            return TEXT("HARVEST CONVERSION");
        case echoes::sim::FutureWellChoice::Preserve:
            return TEXT("PRESERVE HOLD");
        case echoes::sim::FutureWellChoice::Reshape:
            return TEXT("RESHAPE WINDOW");
        default:
            return TEXT("UNAVAILABLE PROTOCOL");
    }
}
}

EEchoesNoNeutralLedgerPhase
FEchoesNoNeutralLedgerMissionModel::DeterminePhase(
    const FEchoesNoNeutralLedgerMissionFacts& Facts)
{
    if (!Facts.bOperationActive)
    {
        return EEchoesNoNeutralLedgerPhase::Inactive;
    }
    if (!Facts.bLocalCoreIntact || !Facts.bOruunIntact ||
        !Facts.bWaystoneIntact || !Facts.bLedgerWitnessIntact ||
        !Facts.bFutureWellIntact || !Facts.bPublicInterfacesIntact ||
        !Facts.bSkirmishStillOngoing)
    {
        return EEchoesNoNeutralLedgerPhase::Failed;
    }
    const bool bBothDistricts =
        Facts.bFirstDistrictIntegrated && Facts.bSecondDistrictIntegrated;
    if (Facts.bConflictingProtocolApplied ||
        (Facts.bRecordedProtocolApplied &&
         (!Facts.bInheritedRouteSecured || !bBothDistricts ||
          !Facts.bBothEvidenceChannelsAttested)))
    {
        return EEchoesNoNeutralLedgerPhase::Failed;
    }
    if (Facts.bReshapeWindowExpired && !Facts.bCoalitionRallied)
    {
        return EEchoesNoNeutralLedgerPhase::Failed;
    }
    if (!Facts.bInheritedRouteSecured)
    {
        return EEchoesNoNeutralLedgerPhase::SecureInheritedRoute;
    }
    if (!bBothDistricts)
    {
        return EEchoesNoNeutralLedgerPhase::IntegrateDistrictContributions;
    }
    if (!Facts.bBothEvidenceChannelsAttested)
    {
        return EEchoesNoNeutralLedgerPhase::AttestEvidenceChannels;
    }
    if (!Facts.bRecordedProtocolApplied)
    {
        return EEchoesNoNeutralLedgerPhase::ApplyRecordedProtocol;
    }
    if (!Facts.bCoalitionRallied)
    {
        return EEchoesNoNeutralLedgerPhase::RallyCoalition;
    }
    return EEchoesNoNeutralLedgerPhase::Complete;
}

const TCHAR* FEchoesNoNeutralLedgerMissionModel::StableName(
    EEchoesNoNeutralLedgerPhase Phase)
{
    switch (Phase)
    {
        case EEchoesNoNeutralLedgerPhase::Inactive: return TEXT("inactive");
        case EEchoesNoNeutralLedgerPhase::SecureInheritedRoute:
            return TEXT("secure_inherited_route");
        case EEchoesNoNeutralLedgerPhase::IntegrateDistrictContributions:
            return TEXT("integrate_district_contributions");
        case EEchoesNoNeutralLedgerPhase::AttestEvidenceChannels:
            return TEXT("attest_evidence_channels");
        case EEchoesNoNeutralLedgerPhase::ApplyRecordedProtocol:
            return TEXT("apply_recorded_protocol");
        case EEchoesNoNeutralLedgerPhase::RallyCoalition:
            return TEXT("rally_coalition");
        case EEchoesNoNeutralLedgerPhase::Complete: return TEXT("complete");
        case EEchoesNoNeutralLedgerPhase::Failed: return TEXT("failed");
    }
    return TEXT("unknown");
}

bool FEchoesNoNeutralLedgerMissionModel::TryPlanForLedger(
    echoes::sim::FutureWellChoice FoundingDoctrine,
    uint8 ReserveAuthorityFacts,
    echoes::sim::FutureWellChoice LumeProtocol,
    FEchoesNoNeutralLedgerPlan& OutPlan)
{
    const uint8 FoundingIndex = ChoiceIndex(FoundingDoctrine);
    const uint8 LumeIndex = ChoiceIndex(LumeProtocol);
    if (FoundingIndex == 0xFF || LumeIndex == 0xFF)
    {
        OutPlan = {};
        return false;
    }

    const uint8 LifeBit = static_cast<uint8>(
        EEchoesReserveAuthorityCompletionFact::LifeSupportPowered);
    const uint8 TransitBit = static_cast<uint8>(
        EEchoesReserveAuthorityCompletionFact::TransitPowered);
    const uint8 ArchiveBit = static_cast<uint8>(
        EEchoesReserveAuthorityCompletionFact::ArchivePowered);
    const bool bLife = (ReserveAuthorityFacts & LifeBit) != 0;
    const bool bTransit = (ReserveAuthorityFacts & TransitBit) != 0;
    const bool bArchive = (ReserveAuthorityFacts & ArchiveBit) != 0;
    if ((bLife ? 1 : 0) + (bTransit ? 1 : 0) + (bArchive ? 1 : 0) != 2)
    {
        OutPlan = {};
        return false;
    }

    OutPlan = {};
    OutPlan.FoundingDoctrine = FoundingDoctrine;
    OutPlan.LumeProtocol = LumeProtocol;
    const FEchoesUnburiedRoadRoute Route =
        FEchoesUnburiedRoadMissionModel::RouteForChoice(FoundingDoctrine);
    OutPlan.RouteSite =
        FoundingDoctrine == echoes::sim::FutureWellChoice::Harvest
            ? echoes::sim::Vec2::FromTiles(18, 30)
        : FoundingDoctrine == echoes::sim::FutureWellChoice::Preserve
            ? echoes::sim::Vec2::FromTiles(32, 30)
            : echoes::sim::Vec2::FromTiles(46, 30);
    OutPlan.RouteStableName = Route.StableName;
    OutPlan.RouteDisplayName = Route.DisplayName;
    OutPlan.ProtocolStableName = ProtocolStableName(LumeProtocol);
    OutPlan.ProtocolDisplayName = ProtocolDisplayName(LumeProtocol);

    TArray<EEchoesCityDistrict, TInlineAllocator<2>> Contributing;
    if (bLife)
    {
        Contributing.Add(EEchoesCityDistrict::LifeSupport);
    }
    else
    {
        OutPlan.DeferredDistrict = EEchoesCityDistrict::LifeSupport;
    }
    if (bTransit)
    {
        Contributing.Add(EEchoesCityDistrict::Transit);
    }
    else
    {
        OutPlan.DeferredDistrict = EEchoesCityDistrict::Transit;
    }
    if (bArchive)
    {
        Contributing.Add(EEchoesCityDistrict::Archive);
    }
    else
    {
        OutPlan.DeferredDistrict = EEchoesCityDistrict::Archive;
    }
    if (Contributing.Num() != 2)
    {
        OutPlan = {};
        return false;
    }
    OutPlan.FirstContributingDistrict = Contributing[0];
    OutPlan.SecondContributingDistrict = Contributing[1];
    OutPlan.FirstDistrictSite = DistrictContributionSite(Contributing[0]);
    OutPlan.SecondDistrictSite = DistrictContributionSite(Contributing[1]);
    OutPlan.MeridianEvidenceSite = echoes::sim::Vec2::FromTiles(26, 43);
    OutPlan.KharuunEvidenceSite = echoes::sim::Vec2::FromTiles(38, 43);
    OutPlan.FutureWellSite = echoes::sim::Vec2::FromTiles(32, 49);
    OutPlan.RallySite = RallySiteForProtocol(LumeProtocol);

    const uint8 DistrictPairIndex =
        !bLife ? 0 : !bTransit ? 1 : 2;
    OutPlan.StablePlanKey = static_cast<uint8>(
        FoundingIndex * 9 + DistrictPairIndex * 3 + LumeIndex);
    return true;
}

echoes::sim::Vec2
FEchoesNoNeutralLedgerMissionModel::DistrictContributionSite(
    EEchoesCityDistrict District)
{
    using echoes::sim::Vec2;
    switch (District)
    {
        case EEchoesCityDistrict::LifeSupport:
            return Vec2::FromTiles(18, 35);
        case EEchoesCityDistrict::Transit:
            // (32,34) and (32,35) both close on some route maps; (32,33) is
            // open on every one of them.
            return Vec2::FromTiles(32, 33);
        case EEchoesCityDistrict::Archive:
            return Vec2::FromTiles(46, 35);
    }
    return {};
}

echoes::sim::Vec2
FEchoesNoNeutralLedgerMissionModel::RallySiteForProtocol(
    echoes::sim::FutureWellChoice Protocol)
{
    using echoes::sim::FutureWellChoice;
    using echoes::sim::Vec2;
    switch (Protocol)
    {
        case FutureWellChoice::Harvest: return Vec2::FromTiles(18, 56);
        case FutureWellChoice::Preserve: return Vec2::FromTiles(32, 56);
        case FutureWellChoice::Reshape:
            // Shared M11-M15 objective stays in the protected central corridor.
            return Vec2::FromTiles(32, 43);
        default: return {};
    }
}
