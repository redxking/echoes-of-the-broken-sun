#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesSimCore/Simulation.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesFullMatchTest,
    "Echoes.Runtime.Gameplay.CompleteSkirmish",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesFullMatchTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the full-match test world."));
        return false;
    }
    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge =
        World != nullptr
            ? World->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (!TestNotNull(TEXT("Full-match world owns the simulation subsystem"), Bridge) ||
        !TestTrue(TEXT("Full-match scenario starts"),
                  Bridge != nullptr && Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const auto TickOnce = [Bridge]() { Bridge->Tick(0.05f); };
    const auto TickUntil = [Bridge](const auto& Predicate, int32 MaximumTicks)
    {
        for (int32 TickIndex = 0; TickIndex < MaximumTicks; ++TickIndex)
        {
            if (Predicate())
            {
                return true;
            }
            Bridge->Tick(0.05f);
        }
        return Predicate();
    };
    const auto QueueCommand = [this, Bridge](
                                  const TCHAR* Description,
                                  echoes::sim::CommandType Type,
                                  echoes::sim::EntityId Actor,
                                  echoes::sim::EntityId Target,
                                  echoes::sim::Vec2 Position,
                                  echoes::sim::FutureWellChoice WellChoice)
    {
        FString Feedback;
        const bool bQueued = Bridge->IssueCommand(
            Type,
            Actor,
            Target,
            Bridge->SimToWorld(Position),
            WellChoice,
            Feedback);
        if (!bQueued)
        {
            AddError(FString::Printf(TEXT("%s: %s"), Description, *Feedback));
        }
        return bQueued;
    };

    const echoes::sim::Simulation* Simulation = Bridge->GetSimulation();
    echoes::sim::EntityId Builder = 0;
    echoes::sim::EntityId WellWorker = 0;
    echoes::sim::EntityId Barracks = 0;
    echoes::sim::EntityId FutureWell = 0;
    echoes::sim::EntityId EnemyCore = 0;
    for (const echoes::sim::Entity& Entity : Simulation->Entities())
    {
        if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            Entity.type == echoes::sim::EntityType::Worker)
        {
            if (Builder == 0)
            {
                Builder = Entity.id;
            }
            else if (WellWorker == 0)
            {
                WellWorker = Entity.id;
            }
        }
        else if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
                 Entity.type == echoes::sim::EntityType::Barracks)
        {
            Barracks = Entity.id;
        }
        else if (Entity.type == echoes::sim::EntityType::FutureWell)
        {
            FutureWell = Entity.id;
        }
        else if (Entity.owner == UEchoesSimulationSubsystem::OpponentPlayerId &&
                 Entity.type == echoes::sim::EntityType::CommandCore)
        {
            EnemyCore = Entity.id;
        }
    }
    if (!TestTrue(TEXT("Skirmish builder exists"), Builder != 0) ||
        !TestTrue(TEXT("Skirmish Well worker exists"), WellWorker != 0) ||
        !TestTrue(TEXT("Skirmish Barracks exists"), Barracks != 0) ||
        !TestTrue(TEXT("Skirmish Future Well exists"), FutureWell != 0) ||
        !TestTrue(TEXT("Skirmish opposing Command Core exists"), EnemyCore != 0))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    FString Feedback;
    TestTrue(
        TEXT("Skirmish expansion queues a drop-off"),
        Bridge->IssueBuildCommand(
            Builder,
            echoes::sim::EntityType::Dropoff,
            Bridge->SimToWorld(echoes::sim::Vec2::FromTiles(19, 12)),
            Feedback));
    if (!Feedback.IsEmpty())
    {
        AddInfo(FString::Printf(TEXT("Drop-off feedback: %s"), *Feedback));
    }
    Feedback.Reset();
    TestTrue(
        TEXT("Skirmish queues its first reinforcement"),
        Bridge->IssueProductionCommand(
            Barracks,
            echoes::sim::EntityType::Soldier,
            Feedback));
    if (!QueueCommand(
            TEXT("Could not send the worker to scout the Future Well"),
            echoes::sim::CommandType::Move,
            WellWorker,
            0,
            echoes::sim::Vec2::FromTiles(29, 29),
            echoes::sim::FutureWellChoice::Dormant))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const bool bWellRevealed = TickUntil(
        [Bridge, FutureWell]()
        {
            const echoes::sim::Simulation* Current = Bridge->GetSimulation();
            return Current != nullptr &&
                   Current->IsEntityVisibleTo(
                       UEchoesSimulationSubsystem::LocalPlayerId,
                       FutureWell);
        },
        600);
    if (!TestTrue(TEXT("Scouting legitimately reveals the Future Well"),
                  bWellRevealed) ||
        !QueueCommand(
            TEXT("Could not queue the visible Future Well harvest"),
            echoes::sim::CommandType::FutureWell,
            WellWorker,
            FutureWell,
            echoes::sim::Vec2::FromTiles(32, 32),
            echoes::sim::FutureWellChoice::Harvest))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const bool bEconomyReady = TickUntil(
        [Bridge, FutureWell]()
        {
            const echoes::sim::Simulation* Current = Bridge->GetSimulation();
            const echoes::sim::Entity* Well = Current->FindEntity(FutureWell);
            return Well != nullptr &&
                   Well->owner == UEchoesSimulationSubsystem::LocalPlayerId &&
                   Well->wellChoice == echoes::sim::FutureWellChoice::Harvest &&
                   Current->PopulationCapacity(
                       UEchoesSimulationSubsystem::LocalPlayerId) >= 18;
        },
        400);
    if (!TestTrue(
            TEXT("Construction and Future Well harvest establish the strike economy"),
            bEconomyReady))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const auto CountLocalSoldiers = [Bridge]()
    {
        int32 Count = 0;
        for (const echoes::sim::Entity& Entity :
             Bridge->GetSimulation()->Entities())
        {
            Count += Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
                             Entity.type == echoes::sim::EntityType::Soldier
                         ? 1
                         : 0;
        }
        return Count;
    };
    while (CountLocalSoldiers() < 7)
    {
        const echoes::sim::Entity* Producer =
            Bridge->GetSimulation()->FindEntity(Barracks);
        if (Producer == nullptr)
        {
            AddError(TEXT("The local Barracks was lost before the strike force formed."));
            break;
        }
        if (Producer->productionRequired == 0)
        {
            Feedback.Reset();
            if (!Bridge->IssueProductionCommand(
                    Barracks,
                    echoes::sim::EntityType::Soldier,
                    Feedback))
            {
                AddError(FString::Printf(
                    TEXT("Strike-force production failed: %s"),
                    *Feedback));
                break;
            }
        }
        TickOnce();
    }
    if (!TestEqual(TEXT("The expanded economy produces seven soldiers"),
                   CountLocalSoldiers(),
                   7))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    TArray<echoes::sim::EntityId> StrikeForce;
    for (const echoes::sim::Entity& Entity : Bridge->GetSimulation()->Entities())
    {
        if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            Entity.type == echoes::sim::EntityType::Soldier)
        {
            StrikeForce.Add(Entity.id);
        }
    }
    for (const echoes::sim::EntityId Soldier : StrikeForce)
    {
        if (!QueueCommand(
                TEXT("Could not rally the strike force"),
                echoes::sim::CommandType::Move,
                Soldier,
                0,
                echoes::sim::Vec2::FromTiles(32, 32),
                echoes::sim::FutureWellChoice::Dormant))
        {
            Bridge->StopPrototypeScenario();
            WorldWrapper.ForwardErrorMessages(this);
            return false;
        }
    }
    const bool bStrikeForceRallied = TickUntil(
        [Bridge, &StrikeForce]()
        {
            for (const echoes::sim::EntityId Soldier : StrikeForce)
            {
                const echoes::sim::Entity* Entity = Bridge->FindEntity(Soldier);
                if (Entity == nullptr ||
                    Entity->position != echoes::sim::Vec2::FromTiles(32, 32))
                {
                    return false;
                }
            }
            return true;
        },
        800);
    if (!TestTrue(
            TEXT("The seven-soldier strike force rallies before entering hostile territory"),
            bStrikeForceRallied))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const echoes::sim::Vec2 EnemyApproach =
        echoes::sim::Vec2::FromTiles(50, 50);
    bool bCommandQueueFailed = false;
    for (const echoes::sim::EntityId Soldier : StrikeForce)
    {
        if (Bridge->FindEntity(Soldier) != nullptr &&
            !QueueCommand(
                TEXT("Could not attack-move the strike force"),
                echoes::sim::CommandType::AttackMove,
                Soldier,
                0,
                EnemyApproach,
                echoes::sim::FutureWellChoice::Dormant))
        {
            bCommandQueueFailed = true;
            break;
        }
    }
    if (!TestFalse(TEXT("Every strike unit accepts the attack-move order"),
                   bCommandQueueFailed))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    bool bEnemyCoreRevealed = false;
    for (int32 BattleTick = 0;
         BattleTick < 2400 &&
         Bridge->GetMatchOutcome() == echoes::sim::MatchOutcome::Ongoing;
         ++BattleTick)
    {
        const echoes::sim::Simulation* Current = Bridge->GetSimulation();
        bEnemyCoreRevealed |= Current->IsEntityVisibleTo(
            UEchoesSimulationSubsystem::LocalPlayerId,
            EnemyCore);
        TickOnce();
    }

    TestTrue(TEXT("The strike force reveals the opposing Command Core"),
             bEnemyCoreRevealed);
    const bool bPlayerVictory =
        Bridge->GetMatchOutcome() ==
        echoes::sim::MatchOutcome::Player0Victory;
    if (!bPlayerVictory)
    {
        int32 SurvivingStrikeUnits = 0;
        for (const echoes::sim::EntityId Soldier : StrikeForce)
        {
            if (const echoes::sim::Entity* Unit = Bridge->FindEntity(Soldier);
                Unit != nullptr)
            {
                ++SurvivingStrikeUnits;
                AddInfo(FString::Printf(
                    TEXT("Surviving strike unit %u: tile=(%d,%d) hp=%d order=%u target=%u"),
                    Unit->id,
                    Unit->position.x.FloorToInt(),
                    Unit->position.y.FloorToInt(),
                    Unit->hitPoints,
                    static_cast<uint8>(Unit->order.type),
                    Unit->order.target));
            }
        }
        const echoes::sim::Entity* RemainingCore =
            Bridge->FindEntity(EnemyCore);
        AddInfo(FString::Printf(
            TEXT("Incomplete battle: tick=%llu outcome=%u survivors=%d enemyCoreHp=%d commandQueueFailed=%s"),
            static_cast<unsigned long long>(
                Bridge->GetSimulation()->CurrentTick()),
            static_cast<uint8>(Bridge->GetMatchOutcome()),
            SurvivingStrikeUnits,
            RemainingCore != nullptr ? RemainingCore->hitPoints : 0,
            bCommandQueueFailed ? TEXT("true") : TEXT("false")));
    }
    TestTrue(TEXT("The composed skirmish reaches player victory"),
             bPlayerVictory);
    if (bPlayerVictory)
    {
        const echoes::sim::Tick VictoryTick =
            Bridge->GetSimulation()->CurrentTick();
        Bridge->Tick(1.0f);
        TestEqual(
            TEXT("A completed match no longer advances simulation time"),
            Bridge->GetSimulation()->CurrentTick(),
            VictoryTick);
    }
    TestTrue(TEXT("Restart succeeds after a completed skirmish"),
             Bridge->RestartPrototypeScenario());
    TestTrue(
        TEXT("Restart returns the skirmish to an ongoing result"),
        Bridge->GetMatchOutcome() == echoes::sim::MatchOutcome::Ongoing);

    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors() && !WorldWrapper.HasFailed();
}

#endif
