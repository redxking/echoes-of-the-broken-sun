#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "EchoesSimCore/Simulation.h"
#include "EchoesSimulationSubsystem.generated.h"

class AEchoesEntityView;

/**
 * Owns the deterministic simulation for the current game world.
 *
 * The simulation is authoritative. Unreal actors are disposable, one-way views
 * rebuilt from the simulation state after every fixed tick.
 */
UCLASS()
class ECHOESOFTHEBROKENSUN_API UEchoesSimulationSubsystem final
    : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    static constexpr float TileWorldSize = 200.0f;
    static constexpr uint8 LocalPlayerId = 0;
    static constexpr uint8 OpponentPlayerId = 1;

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override;

    /** Creates the bounded runtime-only technical-prototype scenario. */
    bool StartPrototypeScenario();

    /** Stops the prototype and releases every disposable presentation view. */
    void StopPrototypeScenario();

    /** Queues one player command for the next deterministic simulation tick. */
    bool IssueCommand(
        echoes::sim::CommandType CommandType,
        uint32 ActorId,
        uint32 TargetId,
        const FVector& WorldPosition,
        echoes::sim::FutureWellChoice WellChoice,
        FString& OutFeedback);

    [[nodiscard]] const echoes::sim::Simulation* GetSimulation() const;
    [[nodiscard]] const echoes::sim::Entity* FindEntity(uint32 EntityId) const;
    [[nodiscard]] AEchoesEntityView* FindEntityView(uint32 EntityId) const;
    [[nodiscard]] FVector SimToWorld(const echoes::sim::Vec2& Position) const;
    [[nodiscard]] echoes::sim::Vec2 WorldToSim(const FVector& Position) const;
    [[nodiscard]] bool IsScenarioReady() const { return bScenarioReady; }
    [[nodiscard]] int32 GetMapWidthTiles() const;
    [[nodiscard]] int32 GetMapHeightTiles() const;

private:
    bool ValidatePrototypeCommand(
        echoes::sim::CommandType CommandType,
        const echoes::sim::Entity& Actor,
        uint32 TargetId,
        const echoes::sim::Vec2& Position,
        echoes::sim::FutureWellChoice WellChoice,
        FString& OutFeedback) const;
    void QueueOpponentCommands();
    bool SyncEntityViews(bool bTeleportNewViews);
    void DestroyEntityViews();

    TUniquePtr<echoes::sim::Simulation> Simulation;
    TMap<uint32, TWeakObjectPtr<AEchoesEntityView>> EntityViews;
    double FixedTimeAccumulator = 0.0;
    uint64 NextPlayerCommandSequence = 1;
    bool bScenarioReady = false;
    bool bWarnedAboutTimeClamp = false;
    bool bLoggedFirstTick = false;
};
