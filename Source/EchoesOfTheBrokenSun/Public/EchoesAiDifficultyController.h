#pragma once

#include "CoreMinimal.h"

#include "EchoesSimCore/Simulation.h"
#include "EchoesSkirmishSetup.h"

#include <span>
#include <vector>

/**
 * One deterministic opponent planning result. The adapter changes when and
 * how often scoped-view decisions enter authority; it never changes rules,
 * resources, visibility, or combat statistics.
 */
struct ECHOESOFTHEBROKENSUN_API FEchoesAiDifficultyPlan final
{
    echoes::sim::Tick ObservationTick = 0;
    echoes::sim::Tick ScheduledExecutionTick = 0;
    bool bGroupCommandWindowOpen = false;
    bool bStrategicReview = false;
    int32 GroupCommandsPerSecond = 0;
    std::vector<echoes::sim::Command> Commands{};
};

/** Stateless difficulty admission over the simulation's scoped PlayerView. */
class ECHOESOFTHEBROKENSUN_API FEchoesAiDifficultyController final
{
public:
    /**
     * A balanced absolute-tick schedule admits at most the selected number of
     * authoritative commands in every rolling one-second tick window.
     */
    [[nodiscard]] static bool IsGroupCommandTick(
        echoes::sim::Tick Tick,
        uint32 TicksPerSecond,
        int32 GroupCommandsPerSecond);

    [[nodiscard]] static bool IsStrategicReviewTick(
        echoes::sim::Tick Tick,
        const FEchoesAiDifficultyPolicy& Policy);

    [[nodiscard]] static bool IsStrategicCommand(
        echoes::sim::CommandType Type);

    /**
     * Generates only from View, filters actors with deferred commands, and
     * schedules one admitted command after the selected reaction interval.
     * PendingCommands is immutable persisted authority state, never a live
     * UObject or presentation reference.
     */
    [[nodiscard]] static FEchoesAiDifficultyPlan BuildPlan(
        const echoes::sim::PlayerView& View,
        echoes::sim::AiPersonality Personality,
        const FEchoesAiDifficultyPolicy& Policy,
        std::span<const echoes::sim::Command> PendingCommands);
};
