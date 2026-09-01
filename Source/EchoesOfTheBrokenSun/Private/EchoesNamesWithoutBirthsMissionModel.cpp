#include "EchoesNamesWithoutBirthsMissionModel.h"

EEchoesNamesWithoutBirthsPhase
FEchoesNamesWithoutBirthsMissionModel::DeterminePhase(
    const FEchoesNamesWithoutBirthsMissionFacts& Facts)
{
    if (!Facts.bOperationActive)
    {
        return EEchoesNamesWithoutBirthsPhase::Inactive;
    }
    if (!Facts.bLocalCoreIntact || !Facts.bTalarIntact ||
        !Facts.bArchiveIntact || !Facts.bFirstCivilianIntact ||
        !Facts.bSecondCivilianIntact || !Facts.bSkirmishStillOngoing)
    {
        return EEchoesNamesWithoutBirthsPhase::Failed;
    }
    if (!Facts.bCensusEvidenceLocated)
    {
        return EEchoesNamesWithoutBirthsPhase::LocateCensus;
    }
    if (!Facts.bArchivePowered)
    {
        return EEchoesNamesWithoutBirthsPhase::StabilizeArchive;
    }
    if (!Facts.bFirstCivilianSheltered || !Facts.bSecondCivilianSheltered)
    {
        return EEchoesNamesWithoutBirthsPhase::ShelterCivilians;
    }
    if (!Facts.bTalarAtEvidenceExtraction)
    {
        return EEchoesNamesWithoutBirthsPhase::ExtractEvidence;
    }
    return EEchoesNamesWithoutBirthsPhase::Complete;
}

const TCHAR* FEchoesNamesWithoutBirthsMissionModel::StableName(
    EEchoesNamesWithoutBirthsPhase Phase)
{
    switch (Phase)
    {
        case EEchoesNamesWithoutBirthsPhase::Inactive: return TEXT("inactive");
        case EEchoesNamesWithoutBirthsPhase::LocateCensus: return TEXT("locate_census");
        case EEchoesNamesWithoutBirthsPhase::StabilizeArchive: return TEXT("stabilize_archive");
        case EEchoesNamesWithoutBirthsPhase::ShelterCivilians: return TEXT("shelter_civilians");
        case EEchoesNamesWithoutBirthsPhase::ExtractEvidence: return TEXT("extract_evidence");
        case EEchoesNamesWithoutBirthsPhase::Complete: return TEXT("complete");
        case EEchoesNamesWithoutBirthsPhase::Failed: return TEXT("failed");
    }
    return TEXT("unknown");
}

FEchoesNamesWithoutBirthsPlan
FEchoesNamesWithoutBirthsMissionModel::PlanForChoice(
    echoes::sim::FutureWellChoice Choice)
{
    switch (Choice)
    {
        case echoes::sim::FutureWellChoice::Harvest:
            return {
                Choice,
                echoes::sim::Vec2::FromTiles(16, 22),
                echoes::sim::Vec2::FromTiles(16, 16),
                echoes::sim::Vec2::FromTiles(14, 48),
                echoes::sim::Vec2::FromTiles(22, 44),
                TEXT("foundry_roll"),
                TEXT("FOUNDRY ROLL")};
        case echoes::sim::FutureWellChoice::Preserve:
            return {
                Choice,
                echoes::sim::Vec2::FromTiles(32, 22),
                echoes::sim::Vec2::FromTiles(28, 16),
                echoes::sim::Vec2::FromTiles(32, 48),
                echoes::sim::Vec2::FromTiles(32, 44),
                TEXT("missing_quarter"),
                TEXT("MISSING QUARTER")};
        case echoes::sim::FutureWellChoice::Reshape:
            return {
                Choice,
                echoes::sim::Vec2::FromTiles(48, 22),
                echoes::sim::Vec2::FromTiles(45, 16),
                echoes::sim::Vec2::FromTiles(39, 37),
                echoes::sim::Vec2::FromTiles(42, 44),
                TEXT("folded_register"),
                TEXT("FOLDED REGISTER")};
        default:
            return {};
    }
}
