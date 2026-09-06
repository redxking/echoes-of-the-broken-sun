#include "EchoesPlayerController.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "EchoesResults"

namespace
{
FText ElapsedTime(uint64 Tick)
{
    const uint64 Seconds = Tick / 20;
    return FText::FromString(FString::Printf(TEXT("%llu:%02llu"), Seconds / 60, Seconds % 60));
}
FText SeatLabel(int32 Seat, echoes::sim::Faction Faction)
{
    return FText::Format(LOCTEXT("Seat", "Player {0} — {1}"), FText::AsNumber(Seat + 1),
        FText::FromString(FEchoesSkirmishSetupModel::FactionDisplayName(Faction)));
}
}

void AEchoesPlayerController::AppendMatchResultDossier(FEchoesShellView& View) const
{
    const auto* Bridge = GetWorld() ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>() : nullptr;
    const auto* Report = Bridge ? Bridge->GetCompletedMatchReport() : nullptr;
    const auto* Metadata = Bridge ? Bridge->GetCompletedReplayMetadata() : nullptr;
    TArray<FText> Sections;
    if (bCampaignResult) Sections.Add(FText::FromString(GetStatusMessage()));
    else if (Metadata && Metadata->OutcomeCause == EEchoesReplayOutcomeCause::PlayerForfeit)
        Sections.Add(LOCTEXT("Concession", "The match ended by concession."));
    else if (PresentedMatchOutcome == echoes::sim::MatchOutcome::Draw)
        Sections.Add(LOCTEXT("DrawCause", "The final opposing Command Cores fell in the same tick."));
    else
        Sections.Add(DidPresentedLocalPlayerWin() ? LOCTEXT("VictoryCause", "The opposing Command Core has fallen.") :
            LOCTEXT("DefeatCause", "Your Command Core has fallen."));
    Sections.Add(FText::Format(LOCTEXT("Elapsed", "Match duration: {0}"), ElapsedTime(Report ? Report->finalTick : PresentedFinalTick)));
    if (!Report)
    {
        View.Status = Bridge && Bridge->GetReplayArchiveState() == EEchoesReplayArchiveState::Pending
            ? LOCTEXT("Preparing", "Preparing match statistics and replay…")
            : FText::Format(LOCTEXT("Unavailable", "Match statistics are unavailable. {0}"),
                FText::FromString(Bridge ? Bridge->GetReplayArchiveError() : FString()));
        View.Body = FText::Join(FText::FromString(TEXT("\n\n")), Sections);
        return;
    }
    Sections.Add(Report->baselineTick == 0 ? LOCTEXT("FullCoverage", "Statistics cover the full match.") :
        FText::Format(LOCTEXT("PartialCoverage", "Statistics cover play from {0}. Earlier activity is unavailable in this legacy checkpoint."),
            ElapsedTime(Report->baselineTick)));
    for (int32 Seat = 0; Seat < static_cast<int32>(echoes::sim::kMaximumPlayers); ++Seat)
    {
        const auto& Player = Report->players[Seat];
        if (!Player.active) continue;
        Sections.Add(FText::Format(LOCTEXT("PlayerStatistics", "{0}\nUnits trained: {1}   Units lost: {2}\nMatter harvested: {3}   Matter delivered: {4}\nActions: {5}"),
            SeatLabel(Seat, Player.faction), FText::AsNumber(Player.unitsTrained), FText::AsNumber(Player.unitsLost),
            FText::AsNumber(Player.materialCollected), FText::AsNumber(Player.materialDelivered), FText::AsNumber(Player.admittedCommands)));
    }
    TArray<FText> Decisions;
    for (const auto& Decision : Report->wellDecisions)
    {
        const FText Choice = Decision.choice == echoes::sim::FutureWellChoice::Harvest ? LOCTEXT("Harvest", "Harvest") :
            Decision.choice == echoes::sim::FutureWellChoice::Preserve ? LOCTEXT("Preserve", "Preserve") :
            Decision.choice == echoes::sim::FutureWellChoice::Reshape ? LOCTEXT("Reshape", "Reshape") : LOCTEXT("Dormant", "Dormant");
        Decisions.Add(FText::Format(LOCTEXT("WellDecision", "{0} — Player {1}: {2}"), ElapsedTime(Decision.tick),
            FText::AsNumber(Decision.player + 1), Choice));
    }
    Sections.Add(FText::Format(LOCTEXT("Wells", "Future Well decisions\n{0}"), Decisions.IsEmpty()
        ? LOCTEXT("NoWell", "No Well decisions were recorded in this interval.")
        : FText::Join(FText::FromString(TEXT("\n")), Decisions)));
    if (bCampaignResult && Metadata && !Metadata->IrreversibleRecordId.IsEmpty())
        Sections.Add(LOCTEXT("LedgerRetained", "The campaign's irreversible decision remains in your journey ledger."));
    View.Body = FText::Join(FText::FromString(TEXT("\n\n")), Sections);
    if (Bridge->GetReplayArchiveState() == EEchoesReplayArchiveState::Failed)
        View.Status = FText::Format(LOCTEXT("ArchiveFailure", "Replay could not be saved. {0}"), FText::FromString(Bridge->GetReplayArchiveError()));
    else View.Status = FText::GetEmpty();

    const FLinearColor SeatColors[] = {FLinearColor(.1f,.9f,1.f), FLinearColor(1.f,.72f,.2f),
        FLinearColor(.9f,.45f,1.f), FLinearColor(.65f,1.f,.55f)};
    const FText Titles[] = {LOCTEXT("APM", "Actions per minute"), LOCTEXT("Trained", "Units trained"),
        LOCTEXT("Lost", "Units lost"), LOCTEXT("Harvested", "Matter harvested"), LOCTEXT("Delivered", "Matter delivered")};
    for (int32 Metric = 0; Metric < 5; ++Metric)
    {
        FEchoesShellChart Chart; Chart.Title = Titles[Metric];
        Chart.Unit = Metric == 0 ? LOCTEXT("ActionsUnit", "actions/min") : Metric < 3 ? LOCTEXT("UnitsUnit", "units") : LOCTEXT("MatterUnit", "Matter");
        for (int32 Seat = 0; Seat < static_cast<int32>(echoes::sim::kMaximumPlayers); ++Seat)
        {
            if (!Report->players[Seat].active) continue;
            FEchoesShellChartSeries Series; Series.Label = SeatLabel(Seat, Report->players[Seat].faction); Series.Color = SeatColors[Seat];
            if (Metric == 0)
            {
                for (const auto& Sample : Report->apmSamples)
                    if (Sample.player == Seat)
                    {
                        Series.Samples.Add(FVector2D(Sample.startTick / 20.0, Sample.actionsPerMinuteX100 / 100.0));
                        Series.Samples.Add(FVector2D(Sample.endTick / 20.0, Sample.actionsPerMinuteX100 / 100.0));
                    }
            }
            else
                for (const auto& Sample : Report->timelineSamples)
                    Series.Samples.Add(FVector2D(Sample.tick / 20.0, Metric == 1 ? Sample.unitsTrained[Seat] :
                        Metric == 2 ? Sample.unitsLost[Seat] : Metric == 3 ? Sample.materialCollected[Seat] : Sample.materialDelivered[Seat]));
            Chart.Series.Add(MoveTemp(Series));
        }
        View.Charts.Add(MoveTemp(Chart));
    }
}

#undef LOCTEXT_NAMESPACE
