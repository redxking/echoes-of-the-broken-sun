#include "EchoesShapeOfSilenceMissionModel.h"

EEchoesShapeOfSilencePhase
FEchoesShapeOfSilenceMissionModel::DeterminePhase(
    const FEchoesShapeOfSilenceMissionFacts& Facts)
{
    if (!Facts.bOperationActive)
    {
        return EEchoesShapeOfSilencePhase::Inactive;
    }
    if (!Facts.bLocalCoreIntact || !Facts.bWaystoneIntact ||
        !Facts.bOruunIntact || !Facts.bFirstWitnessIntact ||
        !Facts.bSecondWitnessIntact || !Facts.bSkirmishStillOngoing)
    {
        return EEchoesShapeOfSilencePhase::Failed;
    }
    if (!Facts.bWaystoneRootedAtAnchor)
    {
        return EEchoesShapeOfSilencePhase::RootWaystone;
    }
    if (!Facts.bListeningSpineRaised)
    {
        return EEchoesShapeOfSilencePhase::RaiseListeningSpine;
    }
    if (!Facts.bFirstWitnessPositioned || !Facts.bSecondWitnessPositioned)
    {
        return EEchoesShapeOfSilencePhase::PositionMemoryWitnesses;
    }
    if (!Facts.bOruunAtConfluence)
    {
        return EEchoesShapeOfSilencePhase::ReachConfluence;
    }
    return EEchoesShapeOfSilencePhase::Complete;
}

const TCHAR* FEchoesShapeOfSilenceMissionModel::StableName(
    EEchoesShapeOfSilencePhase Phase)
{
    switch (Phase)
    {
        case EEchoesShapeOfSilencePhase::Inactive: return TEXT("inactive");
        case EEchoesShapeOfSilencePhase::RootWaystone: return TEXT("root_waystone");
        case EEchoesShapeOfSilencePhase::RaiseListeningSpine: return TEXT("raise_listening_spine");
        case EEchoesShapeOfSilencePhase::PositionMemoryWitnesses: return TEXT("position_memory_witnesses");
        case EEchoesShapeOfSilencePhase::ReachConfluence: return TEXT("reach_confluence");
        case EEchoesShapeOfSilencePhase::Complete: return TEXT("complete");
        case EEchoesShapeOfSilencePhase::Failed: return TEXT("failed");
    }
    return TEXT("unknown");
}

FEchoesShapeOfSilencePlan
FEchoesShapeOfSilenceMissionModel::PlanForChoice(
    echoes::sim::FutureWellChoice Choice)
{
    switch (Choice)
    {
        case echoes::sim::FutureWellChoice::Harvest:
            return {
                Choice,
                echoes::sim::Vec2::FromTiles(14, 28),
                echoes::sim::Vec2::FromTiles(14, 38),
                echoes::sim::Vec2::FromTiles(10, 45),
                echoes::sim::Vec2::FromTiles(18, 45),
                echoes::sim::Vec2::FromTiles(14, 50),
                TEXT("cinder_hollow"),
                TEXT("CINDER HOLLOW")};
        case echoes::sim::FutureWellChoice::Preserve:
            return {
                Choice,
                echoes::sim::Vec2::FromTiles(32, 28),
                echoes::sim::Vec2::FromTiles(32, 38),
                echoes::sim::Vec2::FromTiles(28, 45),
                echoes::sim::Vec2::FromTiles(36, 45),
                echoes::sim::Vec2::FromTiles(32, 50),
                TEXT("held_hollow"),
                TEXT("HELD HOLLOW")};
        case echoes::sim::FutureWellChoice::Reshape:
            return {
                Choice,
                echoes::sim::Vec2::FromTiles(50, 28),
                echoes::sim::Vec2::FromTiles(39, 37),
                echoes::sim::Vec2::FromTiles(22, 38),
                echoes::sim::Vec2::FromTiles(30, 38),
                echoes::sim::Vec2::FromTiles(25, 50),
                TEXT("folded_hollow"),
                TEXT("FOLDED HOLLOW")};
        default:
            return {};
    }
}
