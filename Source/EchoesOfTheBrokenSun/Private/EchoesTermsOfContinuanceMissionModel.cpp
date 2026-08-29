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
        !Facts.bKharuunWitnessIntact || !Facts.bSkirmishStillOngoing)
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
                900,
                TEXT("iron_clause"),
                TEXT("IRON CLAUSE")};
        case FutureWellChoice::Preserve:
            return {
                Choice,
                Vec2::FromTiles(32, 27),
                Vec2::FromTiles(32, 39),
                Vec2::FromTiles(32, 47),
                900,
                TEXT("witness_clause"),
                TEXT("WITNESS CLAUSE")};
        case FutureWellChoice::Reshape:
            return {
                Choice,
                Vec2::FromTiles(50, 27),
                Vec2::FromTiles(50, 39),
                Vec2::FromTiles(44, 47),
                900,
                TEXT("folded_clause"),
                TEXT("FOLDED CLAUSE")};
        default:
            return {};
    }
}
