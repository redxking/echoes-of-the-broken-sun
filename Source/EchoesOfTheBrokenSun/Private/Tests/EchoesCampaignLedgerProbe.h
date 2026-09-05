#pragma once

#include "EchoesCampaignMapCheckpoint.h"
#include "EchoesSimulationSubsystem.h"
#include "Misc/FileHelper.h"

namespace EchoesCampaignTest
{
// Deliberately put a mismatched ledger payload inside the current map envelope.
// This lets ledger refusal tests reach the inner boundary after separately
// proving that the original, mismatched map envelope is rejected first.
inline bool BindForeignLedgerToCurrentMap(UEchoesSimulationSubsystem& Bridge)
{
    const FString Path = Bridge.GetActiveQuickSavePath();
    TArray<uint8> ForeignEnvelope, ForeignPayload, CurrentEnvelope, CurrentPayload, Probe;
    FEchoesCampaignMapCheckpointIdentity ForeignIdentity, CurrentIdentity;
    EEchoesCampaignMapCheckpointFailure Failure{};
    FString Feedback;
    if (!FFileHelper::LoadFileToArray(ForeignEnvelope, *Path) ||
        !FEchoesCampaignMapCheckpoint::Inspect(ForeignEnvelope, ForeignIdentity, ForeignPayload, Failure) ||
        !Bridge.QuickSaveScenario(Feedback) ||
        !FFileHelper::LoadFileToArray(CurrentEnvelope, *Path) ||
        !FEchoesCampaignMapCheckpoint::Inspect(CurrentEnvelope, CurrentIdentity, CurrentPayload, Failure) ||
        !FEchoesCampaignMapCheckpoint::Wrap(CurrentIdentity, ForeignPayload, Probe, Failure))
    {
        return false;
    }
    return FFileHelper::SaveArrayToFile(Probe, *Path) &&
        FFileHelper::SaveArrayToFile(Probe, *(Path + TEXT(".bak")));
}
}
