#include "EchoesSimulationSubsystem.h"
#include "EchoesCampaignProgress.h"
#include "Misc/Paths.h"

FString UEchoesSimulationSubsystem::GetJourneySlotPath(int32 Slot)
{
    // Slot 1 retains the existing campaign file in place. No implicit copy,
    // deletion or migration can overwrite a player's legacy journey.
    if (Slot == 1) return FEchoesCampaignProgressStore::GetDefaultPath();
    if (Slot < 1 || Slot > 3) return FString();
    return FEchoesCampaignProgressStore::GetSlotPath(FString::Printf(TEXT("Journey %d"), Slot));
}

FString UEchoesSimulationSubsystem::GetJourneyCheckpointDirectory() const
{
    const FString Root = FEchoesCampaignProgressStore::GetSaveGameDirectory();
    return ActiveJourneySlot == 1 ? Root : FPaths::Combine(Root,
        FString::Printf(TEXT("Journey%d"), ActiveJourneySlot));
}

bool UEchoesSimulationSubsystem::SelectJourneySlot(int32 Slot, FString& OutFeedback)
{
    if (Slot < 1 || Slot > 3 || bNetworkHumanOpponent ||
        (bScenarioReady && !bSimulationPaused))
    {
        OutFeedback = TEXT("[JOURNEY_SLOT_UNAVAILABLE] Choose Slot 1, 2 or 3 from the paused menu.");
        return false;
    }
    DrainCheckpointSaves();
    if (Slot == ActiveJourneySlot && bCampaignProgressAvailable) return true;
    const FString CandidatePath = Slot == 1 && !LegacyCampaignProgressPath.IsEmpty()
        ? LegacyCampaignProgressPath : GetJourneySlotPath(Slot);
    FEchoesCampaignProgress Candidate;
    if (!FEchoesCampaignProgressStore::LoadWithBackup(CandidatePath, Candidate, OutFeedback))
        return false;
    const FEchoesCampaignProgress Prior = CampaignProgress;
    const FString PriorPath = CampaignProgressPath;
    const int32 PriorSlot = ActiveJourneySlot;
    const bool bPriorAvailable = bCampaignProgressAvailable;
    CampaignProgress = MoveTemp(Candidate);
    CampaignProgressPath = CandidatePath;
    ActiveJourneySlot = Slot;
    bCampaignProgressAvailable = true;
    if (bScenarioReady && !SelectOperationMode(EEchoesOperationMode::Skirmish, OutFeedback))
    {
        CampaignProgress = Prior;
        CampaignProgressPath = PriorPath;
        ActiveJourneySlot = PriorSlot;
        bCampaignProgressAvailable = bPriorAvailable;
        RefreshCampaignBackupState();
        return false;
    }
    RefreshCampaignBackupState();
    OutFeedback = FString::Printf(TEXT("Slot %d selected. %d campaign decisions retained."),
        Slot, CampaignProgress.Decisions.Num());
    return true;
}

bool UEchoesSimulationSubsystem::ConcedeOfflineMatch(FString& OutFeedback)
{
    if (!bScenarioReady || !Simulation.IsValid() || bNetworkHumanOpponent ||
        SelectedOperation != EEchoesOperationMode::Skirmish ||
        Simulation->Outcome() != echoes::sim::MatchOutcome::Ongoing)
    {
        OutFeedback = TEXT("[CONCEDE_UNAVAILABLE] No active offline match can be conceded.");
        return false;
    }
    if (!Simulation->ForfeitPlayer(LocalPlayerId)) return false;
    bSimulationPaused = true;
    bMatchResultReported = true;
    FixedTimeAccumulator = 0.0;
    BeginReplayArchiveForCurrentResult();
    OutFeedback = TEXT("Match conceded.");
    return true;
}
