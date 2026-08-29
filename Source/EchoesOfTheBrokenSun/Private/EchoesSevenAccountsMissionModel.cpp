#include "EchoesSevenAccountsMissionModel.h"

EEchoesSevenAccountsPhase FEchoesSevenAccountsMissionModel::DeterminePhase(
    const FEchoesSevenAccountsMissionFacts& Facts)
{
    if (!Facts.bOperationActive)
    {
        return EEchoesSevenAccountsPhase::Inactive;
    }
    if (!Facts.bLocalCoreIntact || !Facts.bMemoryBearerIntact ||
        !Facts.bWaystoneIntact || !Facts.bSkirmishStillOngoing)
    {
        return EEchoesSevenAccountsPhase::Failed;
    }
    if (!Facts.bWaystoneRootedAtAnchor)
    {
        return EEchoesSevenAccountsPhase::EstablishWaystone;
    }
    return Facts.bMemoryBearerAtAccountSite
               ? EEchoesSevenAccountsPhase::Complete
               : EEchoesSevenAccountsPhase::RecallMemory;
}

const TCHAR* FEchoesSevenAccountsMissionModel::StableName(
    EEchoesSevenAccountsPhase Phase)
{
    switch (Phase)
    {
        case EEchoesSevenAccountsPhase::Inactive: return TEXT("inactive");
        case EEchoesSevenAccountsPhase::EstablishWaystone:
            return TEXT("establish_waystone");
        case EEchoesSevenAccountsPhase::RecallMemory:
            return TEXT("recall_memory");
        case EEchoesSevenAccountsPhase::Complete: return TEXT("complete");
        case EEchoesSevenAccountsPhase::Failed: return TEXT("failed");
    }
    return TEXT("unknown");
}

FEchoesSevenAccountsRoute FEchoesSevenAccountsMissionModel::RouteForChoice(
    echoes::sim::FutureWellChoice Choice)
{
    using echoes::sim::FutureWellChoice;
    using echoes::sim::Vec2;
    switch (Choice)
    {
        case FutureWellChoice::Harvest:
            return {
                Choice,
                Vec2::FromTiles(20, 42),
                Vec2::FromTiles(23, 44),
                TEXT("fractured_western_account"),
                TEXT("FRACTURED WESTERN ACCOUNT")};
        case FutureWellChoice::Preserve:
            return {
                Choice,
                Vec2::FromTiles(35, 40),
                Vec2::FromTiles(38, 43),
                TEXT("archive_verified_central_account"),
                TEXT("ARCHIVE-VERIFIED CENTRAL ACCOUNT")};
        case FutureWellChoice::Reshape:
            return {
                Choice,
                Vec2::FromTiles(40, 42),
                Vec2::FromTiles(43, 44),
                TEXT("manifested_eastern_account"),
                TEXT("MANIFESTED EASTERN ACCOUNT")};
        default:
            return {};
    }
}
