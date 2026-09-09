#include "EchoesSimulationSubsystem.h"

const echoes::feedback::GameplayFeedbackState&
UEchoesSimulationSubsystem::GetGameplayFeedbackForPlayer(uint8 Player) const
{
    static const echoes::feedback::GameplayFeedbackState Unavailable;
    if (!bScenarioReady || !Simulation.IsValid() || bReplayPlaybackActive ||
        Player >= echoes::sim::kMaximumPlayers || !Simulation->FindPlayer(Player))
        return Unavailable;
    auto& Feed = GameplayFeedbackByPlayer[Player];
    (void)Feed.BeginGeneration(ScenarioAuthorityGeneration, Player);
    return Feed;
}

void UEchoesSimulationSubsystem::TrackGameplayFeedbackCommand(
    const echoes::sim::Command& Command)
{
    if (!bScenarioReady || !Simulation.IsValid() || bReplayPlaybackActive ||
        Command.player >= echoes::sim::kMaximumPlayers) return;
    auto& Feed = GameplayFeedbackByPlayer[Command.player];
    (void)Feed.BeginGeneration(ScenarioAuthorityGeneration, Command.player);
    (void)Feed.TrackQueued(Command, ScenarioAuthorityGeneration, Simulation->CurrentTick());
}

void UEchoesSimulationSubsystem::ObserveGameplayFeedbackFixedStep()
{
    if (!bScenarioReady || !Simulation.IsValid() || bReplayPlaybackActive) return;
    // Step-local receipts disappear at the next Step. Observe inside the fixed
    // loop, including each catch-up tick, before controllers receive callbacks.
    for (uint8 Player = 0; Player < echoes::sim::kMaximumPlayers; ++Player)
    {
        if (!Simulation->FindPlayer(Player)) continue;
        (void)GameplayFeedbackByPlayer[Player].ObserveFixedStep(
            *Simulation, Player, ScenarioAuthorityGeneration);
    }
}
