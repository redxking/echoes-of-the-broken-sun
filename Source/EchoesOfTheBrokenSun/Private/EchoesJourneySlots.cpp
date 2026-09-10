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
    // SPEC-OUT-007: the player may continue or concede at any time. This was gated to
    // Skirmish, so a campaign operation going badly had no exit except winning or
    // losing it -- and losing requires the opponent to finish the job.
    // TrainingReadiness stays excluded: it is a passive-AI mastery drill whose result
    // feeds the readiness proof, and it already has an explicit opt-out. Conceding it
    // would be a second, unaudited way to end a drill that grants an unlock.
    if (!bScenarioReady || !Simulation.IsValid() || bNetworkHumanOpponent ||
        SelectedOperation == EEchoesOperationMode::TrainingReadiness ||
        Simulation->Outcome() != echoes::sim::MatchOutcome::Ongoing)
    {
        OutFeedback = TEXT("[CONCEDE_UNAVAILABLE] No active offline operation can be conceded.");
        return false;
    }
    if (!Simulation->ForfeitPlayer(LocalPlayerId)) return false;
    FixedTimeAccumulator = 0.0;
    if (SelectedOperation == EEchoesOperationMode::Skirmish)
    {
        bSimulationPaused = true;
        bMatchResultReported = true;
        BeginReplayArchiveForCurrentResult();
        OutFeedback = TEXT("Match conceded.");
        return true;
    }
    // A campaign operation must finish through its OWN authored path, so the mission
    // consequence, the campaign ledger commit and the per-mission result screen all
    // behave exactly as they do for a mission lost in play. Every mission model fails
    // on `!Facts.bLocalCoreIntact`, and ForfeitPlayer retires precisely that Core, so
    // the next evaluation moves the operation to its Failed phase and the existing
    // dispatch reports it.
    //
    // Deliberately NOT setting bMatchResultReported or pausing here: that dispatch is
    // guarded on `!bMatchResultReported`, so claiming the result now would skip the
    // consequence and the ledger commit and leave the player on a screen the campaign
    // never recorded. The concession has to be an ordinary mission failure, not a
    // second, quieter way to end an operation.
    OutFeedback = TEXT("Operation conceded.");
    return true;
}
