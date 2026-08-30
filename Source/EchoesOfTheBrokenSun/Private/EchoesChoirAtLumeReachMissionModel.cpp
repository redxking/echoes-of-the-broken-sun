#include "EchoesChoirAtLumeReachMissionModel.h"

EEchoesChoirAtLumeReachPhase
FEchoesChoirAtLumeReachMissionModel::DeterminePhase(
    const FEchoesChoirAtLumeReachMissionFacts& Facts)
{
    if (!Facts.bOperationActive)
    {
        return EEchoesChoirAtLumeReachPhase::Inactive;
    }
    if (!Facts.bLocalCoreIntact || !Facts.bOruunIntact ||
        !Facts.bWaystoneIntact || !Facts.bFutureWellIntact ||
        !Facts.bSkirmishStillOngoing)
    {
        return EEchoesChoirAtLumeReachPhase::Failed;
    }
    if (Facts.bFutureWellProtocolChosen &&
        (!Facts.bFirstAnchorRaised || !Facts.bSecondAnchorRaised))
    {
        return EEchoesChoirAtLumeReachPhase::Failed;
    }
    if (Facts.bReshapeWindowExpired &&
        !Facts.bBranchResolutionCompleted)
    {
        return EEchoesChoirAtLumeReachPhase::Failed;
    }
    if (!Facts.bContactEstablished)
    {
        return EEchoesChoirAtLumeReachPhase::EstablishContact;
    }
    if (!Facts.bDeferredLiabilityResolved)
    {
        return EEchoesChoirAtLumeReachPhase::ResolveDeferredLiability;
    }
    if (!Facts.bFirstAnchorRaised)
    {
        return EEchoesChoirAtLumeReachPhase::RaiseFirstAnchor;
    }
    if (!Facts.bSecondAnchorRaised)
    {
        return EEchoesChoirAtLumeReachPhase::RaiseSecondAnchor;
    }
    if (!Facts.bFutureWellProtocolChosen)
    {
        return EEchoesChoirAtLumeReachPhase::CommitFutureWell;
    }
    if (!Facts.bBranchResolutionCompleted)
    {
        return EEchoesChoirAtLumeReachPhase::ResolveFutureWell;
    }
    return EEchoesChoirAtLumeReachPhase::Complete;
}

const TCHAR* FEchoesChoirAtLumeReachMissionModel::StableName(
    EEchoesChoirAtLumeReachPhase Phase)
{
    switch (Phase)
    {
        case EEchoesChoirAtLumeReachPhase::Inactive: return TEXT("inactive");
        case EEchoesChoirAtLumeReachPhase::EstablishContact:
            return TEXT("establish_contact");
        case EEchoesChoirAtLumeReachPhase::ResolveDeferredLiability:
            return TEXT("resolve_deferred_liability");
        case EEchoesChoirAtLumeReachPhase::RaiseFirstAnchor:
            return TEXT("raise_first_anchor");
        case EEchoesChoirAtLumeReachPhase::RaiseSecondAnchor:
            return TEXT("raise_second_anchor");
        case EEchoesChoirAtLumeReachPhase::CommitFutureWell:
            return TEXT("commit_future_well");
        case EEchoesChoirAtLumeReachPhase::ResolveFutureWell:
            return TEXT("resolve_future_well");
        case EEchoesChoirAtLumeReachPhase::Complete: return TEXT("complete");
        case EEchoesChoirAtLumeReachPhase::Failed: return TEXT("failed");
    }
    return TEXT("unknown");
}

FEchoesChoirAtLumeReachPlan
FEchoesChoirAtLumeReachMissionModel::PlanForChoice(
    echoes::sim::FutureWellChoice PriorChoice,
    EEchoesCityDistrict DeferredDistrict)
{
    using echoes::sim::FutureWellChoice;
    FEchoesChoirAtLumeReachPlan Plan;
    Plan.PriorChoice = PriorChoice;
    Plan.DeferredDistrict = DeferredDistrict;
    Plan.LiabilitySite = LiabilitySiteForDistrict(DeferredDistrict);
    Plan.FirstAnchorSite = echoes::sim::Vec2::FromTiles(28, 39);
    Plan.SecondAnchorSite = echoes::sim::Vec2::FromTiles(36, 39);
    Plan.FutureWellSite = echoes::sim::Vec2::FromTiles(32, 43);
    switch (PriorChoice)
    {
        case FutureWellChoice::Harvest:
            Plan.ContactSite = echoes::sim::Vec2::FromTiles(18, 20);
            Plan.StableName = TEXT("ashward_approach");
            Plan.DisplayName = TEXT("ASHWARD APPROACH");
            return Plan;
        case FutureWellChoice::Preserve:
            Plan.ContactSite = echoes::sim::Vec2::FromTiles(32, 20);
            Plan.StableName = TEXT("held_vault_approach");
            Plan.DisplayName = TEXT("HELD-VAULT APPROACH");
            return Plan;
        case FutureWellChoice::Reshape:
            Plan.ContactSite = echoes::sim::Vec2::FromTiles(46, 20);
            Plan.StableName = TEXT("foldward_approach");
            Plan.DisplayName = TEXT("FOLDWARD APPROACH");
            return Plan;
        default:
            return {};
    }
}

echoes::sim::Vec2
FEchoesChoirAtLumeReachMissionModel::LiabilitySiteForDistrict(
    EEchoesCityDistrict District)
{
    using echoes::sim::Vec2;
    switch (District)
    {
        case EEchoesCityDistrict::LifeSupport:
            return Vec2::FromTiles(18, 34);
        case EEchoesCityDistrict::Transit:
            return Vec2::FromTiles(32, 33);
        case EEchoesCityDistrict::Archive:
            return Vec2::FromTiles(46, 34);
    }
    return {};
}

echoes::sim::Vec2
FEchoesChoirAtLumeReachMissionModel::ResolutionSiteForChoice(
    echoes::sim::FutureWellChoice Choice)
{
    using echoes::sim::FutureWellChoice;
    using echoes::sim::Vec2;
    switch (Choice)
    {
        case FutureWellChoice::Harvest: return Vec2::FromTiles(18, 50);
        case FutureWellChoice::Preserve: return Vec2::FromTiles(32, 50);
        case FutureWellChoice::Reshape: return Vec2::FromTiles(46, 50);
        default: return {};
    }
}
