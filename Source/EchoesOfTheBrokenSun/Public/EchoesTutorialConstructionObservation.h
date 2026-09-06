#pragma once

#include "CoreMinimal.h"
#include "EchoesSimCore/Simulation.h"

/** Explicit input provenance; missing or scripted origin cannot teach a lesson. */
enum class EEchoesTutorialConstructionOrigin : uint8
{
    Unknown,
    PlayerInput,
    Replay,
    Programmatic
};

struct FEchoesTutorialConstructionInput
{
    uint64 Session = 0;
    uint64 AuthorityGeneration = 0;
    uint64 InputSequence = 0;
    uint64 CommandSequence = 0;
    uint64 PresentationFrame = 0;
    EEchoesTutorialConstructionOrigin Origin = EEchoesTutorialConstructionOrigin::Unknown;
};

struct FEchoesTutorialConstructionSetup
{
    echoes::sim::PlayerId LocalPlayer = 0;
    echoes::sim::EntityId Builder = 0;
    echoes::sim::EntityId Assistant = 0;
    uint64 FirstInputSequence = 1;
};

struct FEchoesTutorialConstructionProgress
{
    bool bRejectedPlacementObserved = false;
    bool bRejectionAcknowledged = false;
    bool bConstructionStarted = false;
    bool bSimultaneousAssistObserved = false;
    bool bConstructionCompleted = false;
    bool bRepairCompleted = false;
    bool bOperationalSelectionPublished = false;

    [[nodiscard]] bool PredicateSatisfied() const
    {
        return bRejectedPlacementObserved && bRejectionAcknowledged &&
            bConstructionStarted && bSimultaneousAssistObserved &&
            bConstructionCompleted && bRepairCompleted && bOperationalSelectionPublished;
    }
};

struct FEchoesFieldHudView;

/** Read-only Link lesson proof from player input, fixed-tick receipts and actual HUD publication.
 * The controller alone persists the lesson bit after PredicateSatisfied().
 */
class ECHOESOFTHEBROKENSUN_API FEchoesTutorialConstructionObservation final
{
public:
    void Reset();
    bool Begin(uint64 Session, uint64 AuthorityGeneration,
               const echoes::sim::Simulation& Simulation,
               const FEchoesTutorialConstructionSetup& Setup);
    bool ObserveRejectedPlacement(const FEchoesTutorialConstructionInput& Input,
                                  const echoes::sim::Simulation& Simulation,
                                  echoes::sim::Vec2 AttemptedSite);
    bool ObserveRejectionAcknowledged(const FEchoesTutorialConstructionInput& Input,
                                     const echoes::sim::Simulation& Simulation,
                                     uint64 RejectedInputSequence);
    bool ObserveAcceptedCommand(const FEchoesTutorialConstructionInput& Input,
                                const echoes::sim::Simulation& Simulation);
    void ObserveState(uint64 Session, uint64 AuthorityGeneration,
                      const echoes::sim::Simulation& Simulation);
    bool ObserveSelection(const FEchoesTutorialConstructionInput& Input,
                          const echoes::sim::Simulation& Simulation,
                          echoes::sim::EntityId SelectedStructure);
    bool ObserveHudPublication(uint64 Session, uint64 AuthorityGeneration,
                               uint64 PresentationFrame,
                               const echoes::sim::Simulation& Simulation,
                               const TArray<uint32>& SelectedIds,
                               const FEchoesFieldHudView& PublishedView);
    [[nodiscard]] bool IsActive() const { return bActive; }
    [[nodiscard]] FEchoesTutorialConstructionProgress Progress() const { return CurrentProgress; }
    [[nodiscard]] echoes::sim::EntityId ConstructedSite() const { return Site; }
    [[nodiscard]] echoes::sim::EntityId RepairTarget() const { return DamagedLink; }

private:
    bool ValidateEvent(const FEchoesTutorialConstructionInput& Input,
                       const echoes::sim::Simulation& Simulation) const;
    bool ValidateAuthority(uint64 Session, uint64 Generation,
                           const echoes::sim::Simulation& Simulation) const;
    bool bActive = false;
    uint64 SessionId = 0;
    uint64 GenerationId = 0;
    uint64 LastInputSequence = 0;
    uint64 RejectedInputSequence = 0;
    uint64 SelectionFrame = 0;
    echoes::sim::Tick LastTick = 0;
    echoes::sim::Tick BeginTick = 0;
    echoes::sim::Tick RepairCommandTick = 0;
    echoes::sim::EntityId Site = 0;
    echoes::sim::EntityId DamagedLink = 0;
    echoes::sim::EntityId RepairWorker = 0;
    uint64 BuildSequence = 0;
    uint64 AssistSequence = 0;
    uint64 RepairSequence = 0;
    int32 RepairedHp = 0;
    FEchoesTutorialConstructionSetup BoundSetup;
    FEchoesTutorialConstructionProgress CurrentProgress;
};
