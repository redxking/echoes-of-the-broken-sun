#include "EchoesShapeBesideUsMissionModel.h"

EEchoesShapeBesideUsPhase
FEchoesShapeBesideUsMissionModel::DeterminePhase(
    const FEchoesShapeBesideUsMissionFacts& Facts)
{
    if (!Facts.bOperationActive)
    {
        return EEchoesShapeBesideUsPhase::Inactive;
    }
    if (!Facts.bLocalCoreIntact || !Facts.bTalarIntact ||
        !Facts.bFirstStateWitnessIntact ||
        !Facts.bSecondStateWitnessIntact || !Facts.bSkirmishStillOngoing)
    {
        return EEchoesShapeBesideUsPhase::Failed;
    }
    if (!Facts.bFirstEchoObserved)
    {
        return EEchoesShapeBesideUsPhase::ReachFirstEcho;
    }
    if (!Facts.bEchoRelayRaised)
    {
        return EEchoesShapeBesideUsPhase::RaiseEchoRelay;
    }
    if (!Facts.bFirstStateTraversed || !Facts.bSecondStateTraversed)
    {
        return EEchoesShapeBesideUsPhase::TraversePairedStates;
    }
    if (!Facts.bTalarAtConvergence)
    {
        return EEchoesShapeBesideUsPhase::ReachConvergence;
    }
    return EEchoesShapeBesideUsPhase::Complete;
}

const TCHAR* FEchoesShapeBesideUsMissionModel::StableName(
    EEchoesShapeBesideUsPhase Phase)
{
    switch (Phase)
    {
        case EEchoesShapeBesideUsPhase::Inactive: return TEXT("inactive");
        case EEchoesShapeBesideUsPhase::ReachFirstEcho: return TEXT("reach_first_echo");
        case EEchoesShapeBesideUsPhase::RaiseEchoRelay: return TEXT("raise_echo_relay");
        case EEchoesShapeBesideUsPhase::TraversePairedStates: return TEXT("traverse_paired_states");
        case EEchoesShapeBesideUsPhase::ReachConvergence: return TEXT("reach_convergence");
        case EEchoesShapeBesideUsPhase::Complete: return TEXT("complete");
        case EEchoesShapeBesideUsPhase::Failed: return TEXT("failed");
    }
    return TEXT("unknown");
}

FEchoesShapeBesideUsPlan
FEchoesShapeBesideUsMissionModel::PlanForChoice(
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
                TEXT("exhausted_echo"),
                TEXT("EXHAUSTED ECHO")};
        case echoes::sim::FutureWellChoice::Preserve:
            return {
                Choice,
                echoes::sim::Vec2::FromTiles(32, 28),
                echoes::sim::Vec2::FromTiles(32, 38),
                echoes::sim::Vec2::FromTiles(28, 45),
                echoes::sim::Vec2::FromTiles(36, 45),
                echoes::sim::Vec2::FromTiles(32, 50),
                TEXT("held_echo"),
                TEXT("HELD ECHO")};
        case echoes::sim::FutureWellChoice::Reshape:
            return {
                Choice,
                echoes::sim::Vec2::FromTiles(50, 28),
                echoes::sim::Vec2::FromTiles(50, 38),
                echoes::sim::Vec2::FromTiles(46, 45),
                echoes::sim::Vec2::FromTiles(54, 45),
                echoes::sim::Vec2::FromTiles(50, 50),
                TEXT("folded_echo"),
                TEXT("FOLDED ECHO")};
        default:
            return {};
    }
}
