#include "EchoesTermsOfContinuanceMissionModel.h"

EEchoesTermsOfContinuancePhase
FEchoesTermsOfContinuanceMissionModel::DeterminePhase(
    const FEchoesTermsOfContinuanceMissionFacts& Facts)
{
    if (!Facts.bOperationActive)
    {
        return EEchoesTermsOfContinuancePhase::Inactive;
    }
    if (!Facts.bLocalCoreIntact || !Facts.bMeridianRelayIntact ||
        !Facts.bKharuunSpineIntact || !Facts.bMeridianWitnessIntact ||
        !Facts.bKharuunWitnessIntact || !Facts.bSkirmishStillOngoing ||
        Facts.bContinuanceWindowCompromised ||
        Facts.bWitnessExtractionStartedEarly)
    {
        return EEchoesTermsOfContinuancePhase::Failed;
    }
    if (!Facts.bMeridianRelaySynchronized ||
        !Facts.bKharuunSpineSynchronized)
    {
        return EEchoesTermsOfContinuancePhase::SynchronizeNetworks;
    }
    if (!Facts.bContinuanceWindowHeld)
    {
        return EEchoesTermsOfContinuancePhase::HoldContinuanceWindow;
    }
    if (!Facts.bMeridianWitnessExtracted ||
        !Facts.bKharuunWitnessExtracted)
    {
        return EEchoesTermsOfContinuancePhase::ExtractWitnesses;
    }
    return EEchoesTermsOfContinuancePhase::Complete;
}

const TCHAR* FEchoesTermsOfContinuanceMissionModel::StableName(
    EEchoesTermsOfContinuancePhase Phase)
{
    switch (Phase)
    {
        case EEchoesTermsOfContinuancePhase::Inactive:
            return TEXT("inactive");
        case EEchoesTermsOfContinuancePhase::SynchronizeNetworks:
            return TEXT("synchronize_networks");
        case EEchoesTermsOfContinuancePhase::HoldContinuanceWindow:
            return TEXT("hold_continuance_window");
        case EEchoesTermsOfContinuancePhase::ExtractWitnesses:
            return TEXT("extract_witnesses");
        case EEchoesTermsOfContinuancePhase::Complete:
            return TEXT("complete");
        case EEchoesTermsOfContinuancePhase::Failed:
            return TEXT("failed");
    }
    return TEXT("unknown");
}

FEchoesTermsOfContinuancePlan
FEchoesTermsOfContinuanceMissionModel::PlanForChoice(
    echoes::sim::FutureWellChoice Choice)
{
    using echoes::sim::FutureWellChoice;
    using echoes::sim::Vec2;
    switch (Choice)
    {
        case FutureWellChoice::Harvest:
            return {
                Choice,
                Vec2::FromTiles(14, 27),
                Vec2::FromTiles(14, 39),
                Vec2::FromTiles(20, 47),
                {Vec2::FromTiles(19, 21),
                 Vec2::FromTiles(17, 28),
                 Vec2::FromTiles(15, 34)},
                {Vec2::FromTiles(18, 10),
                 Vec2::FromTiles(24, 15),
                 Vec2::FromTiles(29, 20),
                 Vec2::FromTiles(29, 36),
                 Vec2::FromTiles(29, 40)},
                300,
                900,
                TEXT("iron_clause"),
                TEXT("IRON CLAUSE")};
        case FutureWellChoice::Preserve:
            return {
                Choice,
                Vec2::FromTiles(32, 27),
                Vec2::FromTiles(32, 39),
                Vec2::FromTiles(32, 47),
                {Vec2::FromTiles(29, 28)},
                {Vec2::FromTiles(18, 10),
                 Vec2::FromTiles(24, 15),
                 Vec2::FromTiles(29, 20),
                 Vec2::FromTiles(29, 36),
                 Vec2::FromTiles(29, 40)},
                300,
                900,
                TEXT("witness_clause"),
                TEXT("WITNESS CLAUSE")};
        case FutureWellChoice::Reshape:
            return {
                Choice,
                Vec2::FromTiles(50, 27),
                Vec2::FromTiles(50, 39),
                Vec2::FromTiles(44, 47),
                {Vec2::FromTiles(18, 10)},
                {Vec2::FromTiles(24, 15),
                 Vec2::FromTiles(30, 20),
                 Vec2::FromTiles(37, 23),
                 Vec2::FromTiles(44, 26),
                 Vec2::FromTiles(50, 31)},
                300,
                900,
                TEXT("folded_clause"),
                TEXT("FOLDED CLAUSE")};
        default:
            return {};
    }
}
