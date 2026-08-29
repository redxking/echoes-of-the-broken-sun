#pragma once

#include "CoreMinimal.h"
#include "EchoesSimCore/Simulation.h"

enum class EEchoesNamesWithoutBirthsPhase : uint8
{
    Inactive,
    LocateCensus,
    StabilizeArchive,
    ShelterCivilians,
    ExtractEvidence,
    Complete,
    Failed
};

struct FEchoesNamesWithoutBirthsMissionFacts final
{
    bool bOperationActive = false;
    bool bLocalCoreIntact = false;
    bool bTalarIntact = false;
    bool bArchiveIntact = false;
    bool bFirstCivilianIntact = false;
    bool bSecondCivilianIntact = false;
    bool bCensusEvidenceLocated = false;
    bool bArchivePowered = false;
    bool bFirstCivilianSheltered = false;
    bool bSecondCivilianSheltered = false;
    bool bTalarAtEvidenceExtraction = false;
    bool bSkirmishStillOngoing = true;
};

struct FEchoesNamesWithoutBirthsPlan final
{
    echoes::sim::FutureWellChoice PriorChoice =
        echoes::sim::FutureWellChoice::Dormant;
    echoes::sim::Vec2 CensusSite;
    echoes::sim::Vec2 PowerLinkSite;
    echoes::sim::Vec2 CivilianShelterSite;
    echoes::sim::Vec2 EvidenceExtractionSite;
    const TCHAR* StableName = TEXT("unavailable");
    const TCHAR* DisplayName = TEXT("UNAVAILABLE CENSUS TRACE");
};

/** Pure mission-06 reducer and inherited five-record census geometry. */
struct ECHOESOFTHEBROKENSUN_API FEchoesNamesWithoutBirthsMissionModel final
{
    [[nodiscard]] static EEchoesNamesWithoutBirthsPhase DeterminePhase(
        const FEchoesNamesWithoutBirthsMissionFacts& Facts);
    [[nodiscard]] static const TCHAR* StableName(
        EEchoesNamesWithoutBirthsPhase Phase);
    [[nodiscard]] static FEchoesNamesWithoutBirthsPlan PlanForChoice(
        echoes::sim::FutureWellChoice Choice);
};
