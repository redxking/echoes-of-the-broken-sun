#include "EchoesCampaignJourneyModel.h"

namespace
{
constexpr EEchoesOperationMode CampaignOperations[] = {
    EEchoesOperationMode::CampaignPrologue,
    EEchoesOperationMode::CampaignSevenAccounts,
    EEchoesOperationMode::CampaignCityReserve,
    EEchoesOperationMode::CampaignUnburiedRoad,
    EEchoesOperationMode::CampaignTermsOfContinuance,
    EEchoesOperationMode::CampaignNamesWithoutBirths,
    EEchoesOperationMode::CampaignShapeOfSilence,
    EEchoesOperationMode::CampaignShapeBesideUs,
    EEchoesOperationMode::CampaignReserveAuthority,
    EEchoesOperationMode::CampaignChoirAtLumeReach,
    EEchoesOperationMode::CampaignNoNeutralLedger,
    EEchoesOperationMode::CampaignFutureThatWon,
    EEchoesOperationMode::CampaignAssemblyOfTheMissing,
    EEchoesOperationMode::CampaignSeveralVoicesOneCommand,
    EEchoesOperationMode::CampaignTheBrokenSun};
}

FEchoesCampaignJourney FEchoesCampaignJourneyModel::Resolve(
    const FEchoesCampaignProgress& Progress)
{
    FEchoesCampaignJourney Journey;
    Journey.CompletedMissionCount = Progress.Decisions.Num();
    if (Progress.Decisions.Num() > UE_ARRAY_COUNT(CampaignOperations))
    {
        return Journey;
    }

    for (int32 Index = 0; Index < Progress.Decisions.Num(); ++Index)
    {
        const uint8 ExpectedMission = static_cast<uint8>(Index + 1);
        if (static_cast<uint8>(Progress.Decisions[Index].Mission) !=
            ExpectedMission)
        {
            return Journey;
        }
    }

    if (Progress.Decisions.Num() == UE_ARRAY_COUNT(CampaignOperations))
    {
        Journey.State = EEchoesCampaignJourneyState::Complete;
        return Journey;
    }

    Journey.State = EEchoesCampaignJourneyState::Ready;
    Journey.NextOperation = CampaignOperations[Progress.Decisions.Num()];
    return Journey;
}

const TCHAR* FEchoesCampaignJourneyModel::OperationDisplayName(
    EEchoesOperationMode Operation)
{
    switch (Operation)
    {
        case EEchoesOperationMode::CampaignPrologue:
            return TEXT("WHAT THE LEDGER KEEPS");
        case EEchoesOperationMode::CampaignSevenAccounts:
            return TEXT("SEVEN ACCOUNTS OF RAIN");
        case EEchoesOperationMode::CampaignCityReserve:
            return TEXT("A CITY ON RESERVE");
        case EEchoesOperationMode::CampaignUnburiedRoad:
            return TEXT("THE UNBURIED ROAD");
        case EEchoesOperationMode::CampaignTermsOfContinuance:
            return TEXT("TERMS OF CONTINUANCE");
        case EEchoesOperationMode::CampaignNamesWithoutBirths:
            return TEXT("NAMES WITHOUT BIRTHS");
        case EEchoesOperationMode::CampaignShapeOfSilence:
            return TEXT("THE SHAPE OF SILENCE");
        case EEchoesOperationMode::CampaignShapeBesideUs:
            return TEXT("THE SHAPE BESIDE US");
        case EEchoesOperationMode::CampaignReserveAuthority:
            return TEXT("RESERVE AUTHORITY");
        case EEchoesOperationMode::CampaignChoirAtLumeReach:
            return TEXT("THE CHOIR AT LUME REACH");
        case EEchoesOperationMode::CampaignNoNeutralLedger:
            return TEXT("NO NEUTRAL LEDGER");
        case EEchoesOperationMode::CampaignFutureThatWon:
            return TEXT("THE FUTURE THAT WON");
        case EEchoesOperationMode::CampaignAssemblyOfTheMissing:
            return TEXT("ASSEMBLY OF THE MISSING");
        case EEchoesOperationMode::CampaignSeveralVoicesOneCommand:
            return TEXT("SEVERAL VOICES, ONE COMMAND");
        case EEchoesOperationMode::CampaignTheBrokenSun:
            return TEXT("THE BROKEN SUN");
        default:
            return TEXT("GLASS SCAR");
    }
}
