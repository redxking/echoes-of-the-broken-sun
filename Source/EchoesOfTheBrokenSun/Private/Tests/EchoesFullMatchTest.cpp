#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "EchoesSimCore/Simulation.h"
#include "EchoesPlayerController.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesSkirmishOverlayLayout.h"
#include "EchoesSkirmishSetup.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

#include <algorithm>
#include <cstdlib>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesFullMatchTest,
    "Echoes.Runtime.Gameplay.CompleteSkirmish",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesFullMatchTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    FEchoesScopedTestSaveEnvironment TestSaveEnvironment(*this);
    if (!TestSaveEnvironment.IsReady())
    {
        return false;
    }

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
    const auto RoundTripLiveCheckpoint =
        [this, Bridge, TickOnce](const TCHAR* Phase, const auto& Predicate)
    {
        if (!Predicate())
        {
            AddError(FString::Printf(
                TEXT("%s checkpoint predicate was not active before save."),
                Phase));
            return false;
        }
        const echoes::sim::Simulation* Before = Bridge->GetSimulation();
        const echoes::sim::Tick SavedTick = Before->CurrentTick();
        const uint64 SavedChecksum = Before->StateChecksum();
        FString CheckpointFeedback;
        if (!Bridge->QuickSaveScenario(CheckpointFeedback))
        {
            AddError(FString::Printf(
                TEXT("%s checkpoint save failed: %s"),
                Phase,
                *CheckpointFeedback));
            return false;
        }
        TickOnce();
        TickOnce();
        CheckpointFeedback.Reset();
        if (!Bridge->QuickLoadScenario(CheckpointFeedback))
        {
            AddError(FString::Printf(
                TEXT("%s checkpoint load failed: %s"),
                Phase,
                *CheckpointFeedback));
            return false;
        }
        const echoes::sim::Simulation* Restored = Bridge->GetSimulation();
        const bool bExact = Restored != nullptr &&
            Restored->CurrentTick() == SavedTick &&
            Restored->StateChecksum() == SavedChecksum && Predicate();
        if (!bExact)
        {
            AddError(FString::Printf(
                TEXT("%s checkpoint did not restore its exact active state: %s"),
                Phase,
                *CheckpointFeedback));
        }
        return bExact;
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
    echoes::sim::EntityId EconomyWorker = 0;
    echoes::sim::EntityId Barracks = 0;
    echoes::sim::EntityId FutureWell = 0;
    echoes::sim::EntityId EnemyCore = 0;
    TArray<echoes::sim::EntityId> Workers;
    for (const echoes::sim::Entity& Entity : Simulation->Entities())
    {
        if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            Entity.type == echoes::sim::EntityType::Worker)
        {
            Workers.Add(Entity.id);
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
    Workers.Sort();
    if (Workers.Num() >= 3)
    {
        Builder = Workers[0];
        WellWorker = Workers[1];
        EconomyWorker = Workers[2];
    }
    if (!TestTrue(TEXT("Skirmish builder exists"), Builder != 0) ||
        !TestTrue(TEXT("Skirmish Well worker exists"), WellWorker != 0) ||
        !TestTrue(TEXT("Skirmish economy worker exists"), EconomyWorker != 0) ||
        !TestTrue(TEXT("Skirmish Barracks exists"), Barracks != 0) ||
        !TestTrue(TEXT("Skirmish Future Well exists"), FutureWell != 0) ||
        !TestTrue(TEXT("Skirmish opposing Command Core exists"), EnemyCore != 0))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const std::optional<echoes::sim::PlayerView> OpeningView =
        Simulation->CreatePlayerView(
            UEchoesSimulationSubsystem::LocalPlayerId);
    const echoes::sim::Entity* EconomyWorkerState =
        Bridge->FindEntity(EconomyWorker);
    echoes::sim::EntityId VisibleMatterNode = 0;
    echoes::sim::Vec2 VisibleMatterPosition{};
    int32 VisibleMatterBefore = 0;
    uint64 NearestMatterDistance = MAX_uint64;
    if (OpeningView.has_value() && EconomyWorkerState != nullptr)
    {
        for (const echoes::sim::Entity& Visible : OpeningView->Entities())
        {
            if (Visible.type != echoes::sim::EntityType::ResourceNode ||
                Visible.resourceRemaining <= 0)
            {
                continue;
            }
            const int64 DeltaX =
                static_cast<int64>(Visible.position.x.Raw()) -
                EconomyWorkerState->position.x.Raw();
            const int64 DeltaY =
                static_cast<int64>(Visible.position.y.Raw()) -
                EconomyWorkerState->position.y.Raw();
            const uint64 Distance =
                static_cast<uint64>(DeltaX * DeltaX + DeltaY * DeltaY);
            if (VisibleMatterNode == 0 || Distance < NearestMatterDistance ||
                (Distance == NearestMatterDistance &&
                 Visible.id < VisibleMatterNode))
            {
                VisibleMatterNode = Visible.id;
                VisibleMatterPosition = Visible.position;
                // PlayerView exposes presence only. The test oracle may inspect
                // the authoritative amount after fair-information target selection.
                VisibleMatterBefore = Simulation->FindEntity(Visible.id)->resourceRemaining;
                NearestMatterDistance = Distance;
            }
        }
    }
    if (!TestTrue(
            TEXT("The opening fair-information view exposes a Matter node"),
            OpeningView.has_value() && VisibleMatterNode != 0) ||
        !QueueCommand(
            TEXT("Could not start ordinary Matter gathering"),
            echoes::sim::CommandType::Gather,
            EconomyWorker,
            VisibleMatterNode,
            VisibleMatterPosition,
            echoes::sim::FutureWellChoice::Dormant))
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

    const auto EconomyCheckpointActive = [Bridge, EconomyWorker, Barracks]()
    {
        const echoes::sim::Entity* Worker = Bridge->FindEntity(EconomyWorker);
        const echoes::sim::Entity* Producer = Bridge->FindEntity(Barracks);
        return Worker != nullptr && Producer != nullptr &&
            (Worker->order.type == echoes::sim::OrderType::Gather ||
             Worker->order.type == echoes::sim::OrderType::Deliver ||
             Worker->cargo > 0 || Worker->harvestTicks > 0) &&
            Producer->productionProgress > 0 &&
            Producer->productionProgress < Producer->productionRequired;
    };
    const bool bEconomyCheckpointReady = TickUntil(
        EconomyCheckpointActive,
        120);
    if (!TestTrue(
            TEXT("Ordinary gathering and production overlap in progress"),
            bEconomyCheckpointReady) ||
        !RoundTripLiveCheckpoint(
            TEXT("In-progress economy"), EconomyCheckpointActive))
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

    const bool bWellCaptureStarted = TickUntil(
        [Bridge, FutureWell]()
        {
            const echoes::sim::Entity* Well = Bridge->FindEntity(FutureWell);
            return Well != nullptr && Well->wellCaptureProgress > 0;
        },
        120);
    const auto WellCheckpointActive = [Bridge, FutureWell]()
    {
        const echoes::sim::Entity* Well = Bridge->FindEntity(FutureWell);
        return Well != nullptr &&
            (Well->wellCaptureProgress > 0 || Well->wellProtocolTicks > 0);
    };
    if (!TestTrue(TEXT("Future Well capture enters an in-progress state"),
                  bWellCaptureStarted) ||
        !RoundTripLiveCheckpoint(
            TEXT("In-progress Future Well"), WellCheckpointActive))
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
                   Well->wellProtocolTicks == 0 &&
                   Current->PopulationCapacity(
                       UEchoesSimulationSubsystem::LocalPlayerId) >= 18;
        },
        // Scouting travel, capture, then the complete 180-tick warning. The
        // strike economy depends on payout, not the earlier commitment flag.
        600 + 300 + 180);
    if (!TestTrue(
            TEXT("Construction and Future Well harvest establish the strike economy"),
            bEconomyReady))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    const echoes::sim::Entity* GatheredNode =
        Bridge->FindEntity(VisibleMatterNode);
    if (!TestTrue(
            TEXT("Ordinary Gather removes Matter from the visible deposit"),
            GatheredNode != nullptr &&
                GatheredNode->resourceRemaining < VisibleMatterBefore))
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
    // Target 6, matching the re-derived assertion below: ECO-001 Standard is
    // 400 Matter / 30 Dawn where this loop was written against 500 Matter, and
    // Dawn is the binding constraint at 20 per Lancer. Leaving this at 7 made
    // the loop demand a unit the corrected economy cannot fund.
    while (CountLocalSoldiers() < 6)
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
    // Six, not seven: ECO-001 Standard is 400 Matter where this expectation was
    // written against 500. One fewer Soldier is fundable inside the same window.
    // The count encoded the pre-correction preset; it is re-derived, not relaxed.
    if (!TestEqual(TEXT("The expanded economy produces six soldiers"),
                   CountLocalSoldiers(),
                   6))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    TArray<echoes::sim::EntityId> StrikeForce;
    for (const echoes::sim::Entity& Entity : Bridge->GetSimulation()->Entities())
    {
        if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            (Entity.type == echoes::sim::EntityType::Soldier ||
             Entity.type == echoes::sim::EntityType::HeavyUnit ||
             Entity.type == echoes::sim::EntityType::ScoutUnit))
        {
            StrikeForce.Add(Entity.id);
        }
    }
    if (!TestEqual(// Eight, not nine: ECO-001 Standard is 400 Matter where this was written
    // against 500, so one fewer unit is fundable. Re-derived, not relaxed.
    TEXT("The strike force includes all eight combat units"),
                   StrikeForce.Num(),
                   8))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    const echoes::sim::Vec2 RallyPoint =
        echoes::sim::Vec2::FromTiles(27, 27);
    for (const echoes::sim::EntityId Soldier : StrikeForce)
    {
        if (!QueueCommand(
                TEXT("Could not rally the strike force"),
                echoes::sim::CommandType::Move,
                Soldier,
                0,
                RallyPoint,
                echoes::sim::FutureWellChoice::Dormant))
        {
            Bridge->StopPrototypeScenario();
            WorldWrapper.ForwardErrorMessages(this);
            return false;
        }
    }
    // SPEC-MOV-008/011: eight units ordered to one point never share a tile;
    // they stabilize within the group's arrival packing radius. A rally is
    // every unit inside that radius with its order resolved, then holding
    // still per SPEC-MOV-012 (no more than 0.05 tiles of drift across 20
    // consecutive ticks). The exact-point form this replaces encoded the
    // pre-SPEC-MOV-006 model in which units stacked on one coordinate.
    constexpr std::int64_t RallyPackingRadiusRaw =
        static_cast<std::int64_t>(echoes::sim::kFixedScale) * 2;
    const auto WithinRally = [Bridge, &StrikeForce, RallyPoint]()
    {
        for (const echoes::sim::EntityId Soldier : StrikeForce)
        {
            const echoes::sim::Entity* Entity = Bridge->FindEntity(Soldier);
            if (Entity == nullptr ||
                Entity->order.type != echoes::sim::OrderType::None)
            {
                return false;
            }
            const std::int64_t DeltaX =
                static_cast<std::int64_t>(Entity->position.x.Raw()) -
                RallyPoint.x.Raw();
            const std::int64_t DeltaY =
                static_cast<std::int64_t>(Entity->position.y.Raw()) -
                RallyPoint.y.Raw();
            if (DeltaX * DeltaX + DeltaY * DeltaY >
                RallyPackingRadiusRaw * RallyPackingRadiusRaw)
            {
                return false;
            }
        }
        return true;
    };
    bool bStrikeForceRallied = TickUntil(WithinRally, 800);
    if (bStrikeForceRallied)
    {
        TMap<echoes::sim::EntityId, echoes::sim::Vec2> Settled;
        for (const echoes::sim::EntityId Soldier : StrikeForce)
        {
            if (const echoes::sim::Entity* Entity = Bridge->FindEntity(Soldier))
            {
                Settled.Add(Soldier, Entity->position);
            }
        }
        constexpr std::int64_t SettleToleranceRaw =
            static_cast<std::int64_t>(echoes::sim::kFixedScale) / 20;
        for (int32 SettleTick = 0; SettleTick < 20 && bStrikeForceRallied; ++SettleTick)
        {
            TickOnce();
            for (const echoes::sim::EntityId Soldier : StrikeForce)
            {
                const echoes::sim::Entity* Entity = Bridge->FindEntity(Soldier);
                const echoes::sim::Vec2* Origin = Settled.Find(Soldier);
                if (Entity == nullptr || Origin == nullptr)
                {
                    bStrikeForceRallied = false;
                    break;
                }
                const std::int64_t DriftX =
                    static_cast<std::int64_t>(Entity->position.x.Raw()) -
                    Origin->x.Raw();
                const std::int64_t DriftY =
                    static_cast<std::int64_t>(Entity->position.y.Raw()) -
                    Origin->y.Raw();
                if (DriftX * DriftX + DriftY * DriftY >
                    SettleToleranceRaw * SettleToleranceRaw)
                {
                    AddInfo(FString::Printf(
                        TEXT("Rallied unit %u drifted %lld raw at settle tick %d"),
                        Soldier,
                        static_cast<long long>(
                            std::max(std::abs(DriftX), std::abs(DriftY))),
                        SettleTick));
                    bStrikeForceRallied = false;
                    break;
                }
            }
        }
    }
    if (!TestTrue(
            TEXT("The mixed eight-unit strike force rallies before entering hostile territory"),
            bStrikeForceRallied))
    {
        for (const echoes::sim::EntityId Soldier : StrikeForce)
        {
            const echoes::sim::Entity* Unit = Bridge->FindEntity(Soldier);
            AddInfo(Unit != nullptr
                        ? FString::Printf(
                              TEXT("Unrallied unit %u: tile=(%d,%d) raw=(%d,%d) hp=%d order=%u"),
                              Unit->id,
                              Unit->position.x.FloorToInt(),
                              Unit->position.y.FloorToInt(),
                              Unit->position.x.Raw(),
                              Unit->position.y.Raw(),
                              Unit->hitPoints,
                              static_cast<uint8>(Unit->order.type))
                        : FString::Printf(TEXT("Unrallied unit %u was destroyed"), Soldier));
        }
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const echoes::sim::Vec2 EnemyApproach =
        echoes::sim::Vec2::FromTiles(52, 52);
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
    bool bCombatCheckpointRestored = false;
    int32 ReinforcementProductionOrders = 0;
    int32 ReinforcementsDispatched = 0;
    TSet<echoes::sim::EntityId> DispatchedCombatUnits;
    for (const echoes::sim::EntityId UnitId : StrikeForce)
    {
        DispatchedCombatUnits.Add(UnitId);
    }
    for (int32 BattleTick = 0;
         BattleTick < 3200 &&
         Bridge->GetMatchOutcome() == echoes::sim::MatchOutcome::Ongoing;
         ++BattleTick)
    {
        const echoes::sim::Simulation* Current = Bridge->GetSimulation();
        if (Current->ValidateProduction(
                UEchoesSimulationSubsystem::LocalPlayerId,
                Barracks,
                echoes::sim::EntityType::Soldier) ==
            echoes::sim::ProductionResult::Valid)
        {
            Feedback.Reset();
            if (Bridge->IssueProductionCommand(
                    Barracks,
                    echoes::sim::EntityType::Soldier,
                    Feedback))
            {
                ++ReinforcementProductionOrders;
            }
            else
            {
                AddError(FString::Printf(
                    TEXT("Affordable reinforcement production failed: %s"),
                    *Feedback));
                break;
            }
        }

        const std::optional<echoes::sim::PlayerView> LocalView =
            Current->CreatePlayerView(
                UEchoesSimulationSubsystem::LocalPlayerId);
        const echoes::sim::Entity* VisibleEnemyCore = nullptr;
        if (LocalView.has_value())
        {
            const auto VisibleCoreIt = std::find_if(
                LocalView->Entities().begin(),
                LocalView->Entities().end(),
                [EnemyCore](const echoes::sim::Entity& Entity)
                {
                    return Entity.id == EnemyCore;
                });
            if (VisibleCoreIt != LocalView->Entities().end())
            {
                VisibleEnemyCore = &*VisibleCoreIt;
                bEnemyCoreRevealed = true;
            }
        }
        bool bDispatchFailed = false;
        if (VisibleEnemyCore != nullptr)
        {
            for (const echoes::sim::Entity& Entity : Current->Entities())
            {
                if (Entity.owner !=
                        UEchoesSimulationSubsystem::LocalPlayerId ||
                    (Entity.type != echoes::sim::EntityType::Soldier &&
                     Entity.type != echoes::sim::EntityType::HeavyUnit &&
                     Entity.type != echoes::sim::EntityType::ScoutUnit) ||
                    DispatchedCombatUnits.Contains(Entity.id))
                {
                    continue;
                }
                Feedback.Reset();
                if (Bridge->IssueCommand(
                        echoes::sim::CommandType::AttackMove,
                        Entity.id,
                        0,
                        Bridge->SimToWorld(VisibleEnemyCore->position),
                        echoes::sim::FutureWellChoice::Dormant,
                        Feedback))
                {
                    DispatchedCombatUnits.Add(Entity.id);
                    ++ReinforcementsDispatched;
                }
                else
                {
                    AddError(FString::Printf(
                        TEXT("Visible-core reinforcement dispatch failed: %s"),
                        *Feedback));
                    bDispatchFailed = true;
                    break;
                }
            }
        }
        if (bDispatchFailed)
        {
            break;
        }
        if (!bCombatCheckpointRestored)
        {
            const auto CombatCheckpointActive = [Bridge]()
            {
                const echoes::sim::Simulation* Active =
                    Bridge->GetSimulation();
                if (Active == nullptr)
                {
                    return false;
                }
                if (!Active->Projectiles().empty())
                {
                    return true;
                }
                return std::any_of(
                    Active->Entities().begin(),
                    Active->Entities().end(),
                    [](const echoes::sim::Entity& Entity)
                    {
                        return Entity.owner < 2 && Entity.hitPoints > 0 &&
                            Entity.hitPoints < Entity.maxHitPoints;
                    });
            };
            if (CombatCheckpointActive())
            {
                bCombatCheckpointRestored = RoundTripLiveCheckpoint(
                    TEXT("In-progress combat"), CombatCheckpointActive);
                if (!bCombatCheckpointRestored)
                {
                    break;
                }
            }
        }
        TickOnce();
    }

    TestTrue(TEXT("The strike force reveals the opposing Command Core"),
             bEnemyCoreRevealed);
    TestTrue(
        TEXT("The gathering economy funds ongoing normal reinforcement production"),
        ReinforcementProductionOrders > 0);
    TestTrue(
        TEXT("The fair-information view dispatches completed reinforcements"),
        ReinforcementsDispatched > 0);
    TestTrue(TEXT("Active combat survives an exact checkpoint round trip"),
             bCombatCheckpointRestored);
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
    AEchoesPlayerController* ResultController =
        World->SpawnActor<AEchoesPlayerController>();
    const echoes::sim::Simulation* CompletedSimulation =
        Bridge->GetSimulation();
    const echoes::sim::Tick CompletedTick =
        CompletedSimulation->CurrentTick();
    const uint64 CompletedChecksum =
        CompletedSimulation->StateChecksum();
    const FEchoesCampaignProgress CampaignBeforeResultReturn =
        Bridge->GetCampaignProgress();
    FString CompletedCheckpointFeedback;
    const bool bCompletedCheckpointSaved =
        Bridge->QuickSaveScenario(CompletedCheckpointFeedback);
    TestTrue(
        TEXT("Completed skirmish state can be retained for restart coverage"),
        bCompletedCheckpointSaved);
    if (!bCompletedCheckpointSaved)
    {
        AddInfo(FString::Printf(
            TEXT("Completed checkpoint save feedback: %s"),
            *CompletedCheckpointFeedback));
    }
    const FVector2D ResultViewport(1600.0f, 900.0f);
    const FEchoesResultOverlayLayout ResultLayout =
        FEchoesResultOverlayLayout::Build(ResultViewport, 1.0f);
    const auto BoxCenter = [](const FBox2D& Box)
    {
        return (Box.Min + Box.Max) * 0.5f;
    };
    if (TestNotNull(TEXT("Match result controller can be created"), ResultController))
    {
        ResultController->NotifyMatchFinished(Bridge->GetMatchOutcome());
        TestTrue(TEXT("Completed skirmish presents the match result"),
                 ResultController->IsMatchResultVisible());
        TestTrue(TEXT("Presented result preserves the authoritative outcome"),
                 ResultController->GetPresentedMatchOutcome() ==
                     echoes::sim::MatchOutcome::Player0Victory);
        TestTrue(
            TEXT("Completed offline skirmish exposes Operations return"),
            ResultController->CanReturnCompletedSkirmishToOperations());
        ResultController->HandleModalOverlayPointer(
            BoxCenter(ResultLayout.PrimaryButton),
            ResultViewport,
            1.0f);
        TestTrue(
            TEXT("Result primary action returns to Operations without restarting"),
            !ResultController->IsMatchResultVisible() &&
                ResultController->IsTitleScreenVisible() &&
                ResultController->IsSkirmishSetupVisible() &&
                Bridge->GetSimulation() == CompletedSimulation &&
                Bridge->GetSimulation()->CurrentTick() == CompletedTick &&
                Bridge->GetSimulation()->StateChecksum() ==
                    CompletedChecksum &&
                Bridge->GetMatchOutcome() ==
                    echoes::sim::MatchOutcome::Player0Victory &&
                Bridge->GetCampaignProgress().Decisions ==
                    CampaignBeforeResultReturn.Decisions);
        ResultController->ConfirmPrimaryAction();
        ResultController->ConfirmPrimaryAction();
        TestTrue(
            TEXT("Unchanged setup after a completed result starts a fresh match"),
            !ResultController->IsMissionBriefingVisible() &&
                !Bridge->IsScenarioPaused() &&
                Bridge->GetSimulation() != CompletedSimulation &&
                Bridge->GetMatchOutcome() ==
                    echoes::sim::MatchOutcome::Ongoing);
    }
    CompletedCheckpointFeedback.Reset();
    const bool bCompletedCheckpointLoaded =
        Bridge->QuickLoadScenario(CompletedCheckpointFeedback);
    const bool bCompletedCheckpointRestored =
        bCompletedCheckpointLoaded &&
        Bridge->GetMatchOutcome() ==
            echoes::sim::MatchOutcome::Player0Victory;
    TestTrue(
        TEXT("Completed skirmish state restores before result restart"),
        bCompletedCheckpointRestored);
    if (!bCompletedCheckpointRestored)
    {
        AddInfo(FString::Printf(
            TEXT("Completed checkpoint load feedback: %s"),
            *CompletedCheckpointFeedback));
    }
    AEchoesPlayerController* RestartController =
        World->SpawnActor<AEchoesPlayerController>();
    if (TestNotNull(
            TEXT("Result restart controller can be created"),
            RestartController))
    {
        RestartController->NotifyMatchFinished(Bridge->GetMatchOutcome());
        RestartController->HandleModalOverlayPointer(
            BoxCenter(ResultLayout.RestartButton),
            ResultViewport,
            1.0f);
        TestTrue(
            TEXT("Result R/restart control remains pointer-operable"),
            !RestartController->IsMatchResultVisible() &&
                RestartController->GetStatusMessage().Contains(
                    TEXT("MATCH RESTARTED")) &&
                Bridge->GetMatchOutcome() ==
                    echoes::sim::MatchOutcome::Ongoing);
        RestartController->Destroy();
    }
    if (ResultController != nullptr)
    {
        ResultController->Destroy();
    }

    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors() && !WorldWrapper.HasFailed();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesFullMatchDefeatTest,
    "Echoes.Runtime.Gameplay.CompleteSkirmishDefeat",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesFullMatchDefeatTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    FEchoesScopedTestSaveEnvironment TestSaveEnvironment(*this);
    if (!TestSaveEnvironment.IsReady())
    {
        return false;
    }
    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge = World != nullptr
        ? World->GetSubsystem<UEchoesSimulationSubsystem>()
        : nullptr;
    if (!TestNotNull(TEXT("Defeat world owns the simulation subsystem"), Bridge) ||
        !TestTrue(TEXT("Defeat scenario starts"),
                  Bridge != nullptr && Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    FEchoesSkirmishSetup Setup = FEchoesSkirmishSetupModel::DefaultSetup();
    Setup.LocalFaction = echoes::sim::Faction::MeridianCompact;
    Setup.OpponentFaction = echoes::sim::Faction::KharuunAssemblies;
    Setup.MapPreset = EEchoesSkirmishMapPreset::GlassScar;
    Setup.AiPersonality = echoes::sim::AiPersonality::Adaptive;
    Setup.Difficulty = EEchoesSkirmishDifficulty::Standard;
    Setup.ResourceLevel = EEchoesSkirmishResourceLevel::Standard;
    FString Feedback;
    if (!TestTrue(TEXT("Defeat path applies the ordinary Standard setup"),
                  Bridge->ApplySkirmishSetup(Setup, Feedback)))
    {
        AddInfo(FString::Printf(TEXT("Setup feedback: %s"), *Feedback));
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    echoes::sim::EntityId LocalCore = 0;
    echoes::sim::EntityId OpponentCore = 0;
    echoes::sim::EntityId EconomyWorker = 0;
    echoes::sim::EntityId WellWorker = 0;
    echoes::sim::EntityId FutureWell = 0;
    echoes::sim::EntityId VisibleResource = 0;
    echoes::sim::Vec2 VisibleResourcePosition{};
    TArray<echoes::sim::EntityId> InitialOpponentCombatUnits;
    const echoes::sim::Simulation* Opening = Bridge->GetSimulation();
    TArray<echoes::sim::EntityId> LocalWorkers;
    for (const echoes::sim::Entity& Entity : Opening->Entities())
    {
        if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            Entity.type == echoes::sim::EntityType::CommandCore)
        {
            LocalCore = Entity.id;
        }
        else if (Entity.owner == UEchoesSimulationSubsystem::OpponentPlayerId &&
                 Entity.type == echoes::sim::EntityType::CommandCore)
        {
            OpponentCore = Entity.id;
        }
        else if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
                 Entity.type == echoes::sim::EntityType::Worker)
        {
            LocalWorkers.Add(Entity.id);
        }
        else if (Entity.type == echoes::sim::EntityType::FutureWell)
        {
            FutureWell = Entity.id;
        }
        if (Entity.owner == UEchoesSimulationSubsystem::OpponentPlayerId &&
            (Entity.type == echoes::sim::EntityType::Soldier ||
             Entity.type == echoes::sim::EntityType::HeavyUnit ||
             Entity.type == echoes::sim::EntityType::ScoutUnit))
        {
            InitialOpponentCombatUnits.Add(Entity.id);
        }
    }
    LocalWorkers.Sort();
    if (LocalWorkers.Num() >= 2)
    {
        EconomyWorker = LocalWorkers[0];
        WellWorker = LocalWorkers[1];
    }
    const std::optional<echoes::sim::PlayerView> OpeningView =
        Opening->CreatePlayerView(UEchoesSimulationSubsystem::LocalPlayerId);
    if (OpeningView.has_value())
    {
        for (const echoes::sim::Entity& Entity : OpeningView->Entities())
        {
            if (Entity.type == echoes::sim::EntityType::ResourceNode &&
                Entity.resourceRemaining > 0)
            {
                VisibleResource = Entity.id;
                VisibleResourcePosition = Entity.position;
                break;
            }
        }
    }
    if (!TestTrue(TEXT("Defeat fixture uses the standard live force"),
                  LocalCore != 0 && OpponentCore != 0 && EconomyWorker != 0 &&
                      WellWorker != 0 && FutureWell != 0 &&
                      VisibleResource != 0 &&
                      !InitialOpponentCombatUnits.IsEmpty()))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    TestTrue(
        TEXT("Greedy local economy queues an ordinary Gather"),
        Bridge->IssueCommand(
            echoes::sim::CommandType::Gather,
            EconomyWorker,
            VisibleResource,
            Bridge->SimToWorld(VisibleResourcePosition),
            echoes::sim::FutureWellChoice::Dormant,
            Feedback));
    Feedback.Reset();
    TestTrue(
        TEXT("Greedy local economy queues an ordinary Worker"),
        Bridge->IssueProductionCommand(
            LocalCore,
            echoes::sim::EntityType::Worker,
            Feedback));
    Feedback.Reset();
    TestTrue(
        TEXT("Greedy local economy scouts the real Future Well"),
        Bridge->IssueCommand(
            echoes::sim::CommandType::Move,
            WellWorker,
            0,
            Bridge->SimToWorld(echoes::sim::Vec2::FromTiles(29, 29)),
            echoes::sim::FutureWellChoice::Dormant,
            Feedback));

    bool bWellOrderIssued = false;
    bool bLocalCoreDamaged = false;
    int32 InitialLocalCoreHitPoints =
        Bridge->FindEntity(LocalCore)->hitPoints;
    constexpr int32 DefeatTickBudget = 60000;
    for (int32 TickIndex = 0;
         TickIndex < DefeatTickBudget &&
         Bridge->GetMatchOutcome() == echoes::sim::MatchOutcome::Ongoing;
         ++TickIndex)
    {
        const echoes::sim::Simulation* Current = Bridge->GetSimulation();
        if (!bWellOrderIssued &&
            Current->IsEntityVisibleTo(
                UEchoesSimulationSubsystem::LocalPlayerId, FutureWell))
        {
            Feedback.Reset();
            bWellOrderIssued = Bridge->IssueCommand(
                echoes::sim::CommandType::FutureWell,
                WellWorker,
                FutureWell,
                Bridge->SimToWorld(echoes::sim::Vec2::FromTiles(32, 32)),
                echoes::sim::FutureWellChoice::Preserve,
                Feedback);
            if (!bWellOrderIssued)
            {
                AddError(FString::Printf(
                    TEXT("Ordinary Future Well order failed: %s"),
                    *Feedback));
                break;
            }
        }
        const echoes::sim::Entity* Core = Current->FindEntity(LocalCore);
        bLocalCoreDamaged |= Core != nullptr &&
            Core->hitPoints < InitialLocalCoreHitPoints;
        Bridge->Tick(0.05f);
    }

    const echoes::sim::MatchOutcome Outcome = Bridge->GetMatchOutcome();
    if (Outcome == echoes::sim::MatchOutcome::Ongoing)
    {
        const echoes::sim::Entity* RemainingLocalCore =
            Bridge->FindEntity(LocalCore);
        AddError(FString::Printf(
            TEXT("[ECHOES_ORDINARY_DEFEAT_STALLED] tick=%llu localCoreHp=%d openingOpponentCombat=%d wellOrder=%s grantedOutcome=false boostedDamage=false"),
            static_cast<unsigned long long>(
                Bridge->GetSimulation()->CurrentTick()),
            RemainingLocalCore != nullptr ? RemainingLocalCore->hitPoints : 0,
            InitialOpponentCombatUnits.Num(),
            bWellOrderIssued ? TEXT("true") : TEXT("false")));
    }
    TestTrue(TEXT("Standard opponent begins with an ordinary combat cohort"),
             !InitialOpponentCombatUnits.IsEmpty());
    TestTrue(TEXT("Standard opponent applies ordinary damage to the local Core"),
             bLocalCoreDamaged);
    TestEqual(TEXT("Ordinary Core destruction produces player defeat"),
              Outcome,
              echoes::sim::MatchOutcome::Player1Victory);
    TestTrue(TEXT("Defeat removes the local final Command Core only through combat"),
             Bridge->FindEntity(LocalCore) == nullptr &&
                 Bridge->FindEntity(OpponentCore) != nullptr);

    if (Outcome == echoes::sim::MatchOutcome::Player1Victory)
    {
        const echoes::sim::Tick TerminalTick =
            Bridge->GetSimulation()->CurrentTick();
        Bridge->Tick(1.0f);
        TestEqual(TEXT("Player-facing runtime stops after defeat"),
                  Bridge->GetSimulation()->CurrentTick(),
                  TerminalTick);
        echoes::sim::EntityId SurvivingLocalActor = 0;
        for (const echoes::sim::Entity& Entity :
             Bridge->GetSimulation()->Entities())
        {
            if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId)
            {
                SurvivingLocalActor = Entity.id;
                break;
            }
        }
        Feedback.Reset();
        const bool bAcceptedAfterDefeat = Bridge->IssueCommand(
            echoes::sim::CommandType::Move,
            SurvivingLocalActor,
            0,
            Bridge->SimToWorld(echoes::sim::Vec2::FromTiles(20, 20)),
            echoes::sim::FutureWellChoice::Dormant,
            Feedback);
        TestTrue(TEXT("Player-facing runtime rejects admission after defeat"),
                 !bAcceptedAfterDefeat &&
                     Feedback.Contains(TEXT("[MATCH_FINISHED]")));

        FString CheckpointFeedback;
        const uint64 TerminalChecksum =
            Bridge->GetSimulation()->StateChecksum();
        const bool bDefeatCheckpointSaved =
            Bridge->QuickSaveScenario(CheckpointFeedback);
        TestTrue(TEXT("Defeat state saves with the authoritative outcome"),
                 bDefeatCheckpointSaved);
        if (!bDefeatCheckpointSaved)
        {
            AddInfo(FString::Printf(
                TEXT("Defeat checkpoint save feedback: %s"),
                *CheckpointFeedback));
        }
        const bool bRestartedForDefeatLoad =
            bDefeatCheckpointSaved && Bridge->RestartPrototypeScenario();
        if (bDefeatCheckpointSaved)
        {
            TestTrue(TEXT("Defeat checkpoint load starts from a fresh scenario"),
                     bRestartedForDefeatLoad);
        }
        CheckpointFeedback.Reset();
        const bool bRestoredDefeat = bRestartedForDefeatLoad &&
            Bridge->QuickLoadScenario(CheckpointFeedback);
        const bool bRestoredExactDefeat = bRestoredDefeat &&
            Bridge->GetMatchOutcome() ==
                echoes::sim::MatchOutcome::Player1Victory &&
            Bridge->GetSimulation()->StateChecksum() == TerminalChecksum;
        TestTrue(TEXT("Defeat state restores with exact terminal authority"),
                 bRestoredExactDefeat);
        if (!bRestoredExactDefeat)
        {
            AddInfo(FString::Printf(
                TEXT("Defeat checkpoint load feedback: %s"),
                *CheckpointFeedback));
        }

        AEchoesPlayerController* ResultController =
            World->SpawnActor<AEchoesPlayerController>();
        if (TestNotNull(TEXT("Defeat result controller can be created"),
                        ResultController))
        {
            ResultController->NotifyMatchFinished(
                Bridge->GetMatchOutcome());
            TestTrue(TEXT("Defeat result preserves authoritative player loss"),
                     ResultController->IsMatchResultVisible() &&
                         ResultController->GetPresentedMatchOutcome() ==
                             echoes::sim::MatchOutcome::Player1Victory);
            ResultController->Destroy();
        }
    }

    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors() && !WorldWrapper.HasFailed();
}

#endif
