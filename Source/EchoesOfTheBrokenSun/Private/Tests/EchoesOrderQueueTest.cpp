// Author: Angelis Pseftis
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "EchoesTestSaveEnvironment.h"
#include "EchoesPlayerController.h"
#include "EchoesRTSCameraPawn.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

// Queued player orders. The simulation has implemented a 16-deep per-entity
// order queue the whole time (Entity::orderQueue, kMaxQueuedOrders), but the
// adapter's IssueCommand had no queue parameter, so every player order replaced
// the last one. This proves the queue reaches the simulation and that a queued
// sequence is deterministic.
//
// Scope: this drives IssueCommand's queue flag directly. A headless automation
// test cannot hold Shift, so the four call sites that read the modifier
// (IssueContextOrder, AttackMoveAtCursor, PatrolAtCursor and the minimap order
// path) are covered by inspection, not by this test.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEchoesOrderQueueTest,
    "Echoes.Runtime.Gameplay.OrderQueue",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEchoesOrderQueueTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    using namespace echoes::sim;
    FEchoesScopedTestSaveEnvironment Saves(*this);
    if (!Saves.IsReady()) return false;
    FTestWorldWrapper Fixture;
    if (!Fixture.CreateTestWorld(EWorldType::Game))
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    UWorld* World = Fixture.GetTestWorld();
    auto* Bridge = World ? World->GetSubsystem<UEchoesSimulationSubsystem>() : nullptr;
    if (!TestNotNull(TEXT("Order-queue bridge exists"), Bridge) ||
        !TestTrue(TEXT("Order-queue scenario starts"),
                  Bridge && Bridge->StartPrototypeScenario())) return false;
    auto* Controller = World->SpawnActor<AEchoesPlayerController>();
    auto* Camera = World->SpawnActor<AEchoesRTSCameraPawn>();
    if (!TestNotNull(TEXT("Order-queue controller exists"), Controller) ||
        !TestNotNull(TEXT("Order-queue camera exists"), Camera)) return false;
    Controller->InitInputSystem();
    Controller->Possess(Camera);
    if (Controller->IsTitleScreenVisible()) Controller->ConfirmPrimaryAction();
    if (Controller->IsMissionBriefingVisible()) Controller->ConfirmPrimaryAction();

    // Deploying through the briefing can leave the scenario paused or a tick
    // short of ready; orders are refused in both states.
    Bridge->SetScenarioPaused(false);
    for (int32 Settle = 0; Settle < 3; ++Settle) Bridge->Tick(0.05f);

    auto* Sim = const_cast<Simulation*>(Bridge->GetSimulation());
    if (!TestNotNull(TEXT("Order-queue authority exists"), Sim)) return false;

    uint32 MoverId = 0;
    for (const auto& Entity : Sim->Entities())
    {
        if (Entity.owner != UEchoesSimulationSubsystem::LocalPlayerId) continue;
        if (Entity.type == EntityType::Soldier) { MoverId = Entity.id; break; }
    }
    if (!TestTrue(TEXT("Fixture has a local mover"), MoverId != 0)) return false;

    Entity* Mover = Sim->MutableEntityForTesting(MoverId);
    Mover->order = {};
    Mover->orderQueue.clear();
    const Vec2 Origin = Mover->position;

    // Destinations must satisfy the predicate the simulation itself applies at
    // execute time, not a proxy for it. IsPositionPassableFor answers from
    // ground truth; ValidateMoveOrder answers from the ordering player's
    // knowledge (IsTileKnownGroundOpenTo), so a passable but unexplored tile is
    // receipted as accepted by IssueCommand and then silently refused inside
    // the tick, leaving the actor with no order at all. Selecting against
    // ValidateMoveOrder keeps this test measuring queueing rather than fog.
    TArray<Vec2> Legs;
    constexpr int32 Dirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    for (int32 Ring = 3; Ring <= 12 && Legs.Num() < 3; ++Ring)
    {
        for (int32 Dir = 0; Dir < 4 && Legs.Num() < 3; ++Dir)
        {
            const Vec2 Candidate{Origin.x + Fixed::FromInt(Ring * Dirs[Dir][0]),
                                 Origin.y + Fixed::FromInt(Ring * Dirs[Dir][1])};
            if (Sim->ValidateMoveOrder(UEchoesSimulationSubsystem::LocalPlayerId,
                                       MoverId, Candidate)
                != echoes::sim::CommandResolutionOutcome::Applied) continue;
            if (!Sim->IsPositionPassableFor(0, Candidate)) continue;
            // A goal on the actor's own tile validates trivially and would
            // produce no movement to observe.
            if (Candidate.x.FloorToInt() == Origin.x.FloorToInt() &&
                Candidate.y.FloorToInt() == Origin.y.FloorToInt()) continue;
            bool bDistinct = true;
            for (const Vec2& Taken : Legs)
                if (Taken.x == Candidate.x && Taken.y == Candidate.y) bDistinct = false;
            if (bDistinct) Legs.Add(Candidate);
        }
    }
    if (Legs.Num() < 3)
    {
        // Name why every candidate was refused rather than only that none was.
        FString Probe;
        for (int32 Ring = 3; Ring <= 12; Ring += 3)
            for (int32 Dir = 0; Dir < 4; ++Dir)
            {
                const Vec2 C{Origin.x + Fixed::FromInt(Ring * Dirs[Dir][0]),
                             Origin.y + Fixed::FromInt(Ring * Dirs[Dir][1])};
                Probe += FString::Printf(TEXT("(%d,%d)=%d "),
                    C.x.FloorToInt(), C.y.FloorToInt(),
                    static_cast<int32>(Sim->ValidateMoveOrder(
                        UEchoesSimulationSubsystem::LocalPlayerId, MoverId, C)));
            }
        const Entity* M = Sim->FindEntity(MoverId);
        AddError(FString::Printf(
            TEXT("No admissible destination. mover=%u type=%d owner=%u completed=%s hp=%d movePerTick=%lld waystone=%d origin=(%d,%d) probes: %s"),
            MoverId, static_cast<int32>(M->type), M->owner,
            M->completed ? TEXT("true") : TEXT("false"),
            static_cast<int32>(M->hitPoints),
            static_cast<long long>(M->movementPerTickRaw),
            static_cast<int32>(M->waystoneMode),
            Origin.x.FloorToInt(), Origin.y.FloorToInt(), *Probe));
    }
    if (!TestEqual(TEXT("Fixture found three admissible destinations"),
                   Legs.Num(), 3))
    {
        Bridge->StopPrototypeScenario();
        return false;
    }
    const auto World1 = Bridge->SimToWorld(Legs[0]);
    const auto World2 = Bridge->SimToWorld(Legs[1]);
    const auto World3 = Bridge->SimToWorld(Legs[2]);

    FString Feedback;
    // First order replaces, as an unmodified order always has.
    const bool bFirstAccepted =
        Bridge->IssueCommand(CommandType::Move, MoverId, 0, World1,
                             FutureWellChoice::Dormant, Feedback, false);
    if (!bFirstAccepted)
    {
        AddError(FString::Printf(
            TEXT("Unqueued move refused: %s (mover=%u paused=%s ready=%s)"),
            *Feedback, MoverId,
            Bridge->IsScenarioPaused() ? TEXT("true") : TEXT("false"),
            Bridge->IsScenarioReady() ? TEXT("true") : TEXT("false")));
        Bridge->StopPrototypeScenario();
        return false;
    }
    Bridge->Tick(0.05f);
    TestEqual(TEXT("An unqueued order leaves the queue empty"),
        static_cast<int32>(Sim->FindEntity(MoverId)->orderQueue.size()), 0);

    // Two queued orders append rather than replace.
    // Both queued orders are issued before ticking, so they are measured while
    // the first leg is still executing rather than after the queue has drained.
    TestTrue(TEXT("Queued move is accepted"),
        Bridge->IssueCommand(CommandType::Move, MoverId, 0, World2,
                             FutureWellChoice::Dormant, Feedback, true));
    TestTrue(TEXT("Second queued order is accepted"),
        Bridge->IssueCommand(CommandType::Move, MoverId, 0, World3,
                             FutureWellChoice::Dormant, Feedback, true));
    int32 Depth = 0;
    for (int32 Settle = 0; Settle < 4; ++Settle)
    {
        Bridge->Tick(0.05f);
        Depth = FMath::Max(Depth,
            static_cast<int32>(Sim->FindEntity(MoverId)->orderQueue.size()));
    }
    {
        const Entity* Now = Sim->FindEntity(MoverId);
        AddInfo(FString::Printf(
            TEXT("mover=%u type=%d orderType=%d queue=%d pos=(%d,%d) origin=(%d,%d) legs=(%d,%d,%d) paused=%s outcome=%d"),
            MoverId, static_cast<int32>(Now->type),
            static_cast<int32>(Now->order.type),
            static_cast<int32>(Now->orderQueue.size()),
            Now->position.x.FloorToInt(), Now->position.y.FloorToInt(),
            Origin.x.FloorToInt(), Origin.y.FloorToInt(),
            Legs[0].x.FloorToInt(), Legs[1].x.FloorToInt(), Legs[2].x.FloorToInt(),
            Bridge->IsScenarioPaused() ? TEXT("true") : TEXT("false"),
            static_cast<int32>(Sim->Outcome())));
    }
    TestEqual(TEXT("Two queued orders leave queue depth 2"), Depth, 2);

    // The queued legs are consumed in issue order, not collapsed.
    const Vec2 StartPos = Sim->FindEntity(MoverId)->position;
    int32 Observed = 0;
    int32 ConsumeTicks = 0;
    for (int32 Step = 0; Step < 900; ++Step)
    {
        Bridge->Tick(0.05f);
        ++ConsumeTicks;
        const Entity* Now = Sim->FindEntity(MoverId);
        if (Now == nullptr) break;
        Observed = static_cast<int32>(Now->orderQueue.size());
        if (Observed == 0) break;
    }
    TestEqual(TEXT("Every queued leg is consumed"), Observed, 0);
    // Legs may run along either axis, so movement is any change of position.
    const Vec2 EndPos = Sim->FindEntity(MoverId)->position;
    TestTrue(TEXT("The mover advanced along the queued route"),
        EndPos.x != StartPos.x || EndPos.y != StartPos.y);

    // Determinism: the same queued sequence from the same seed reproduces the
    // same authoritative checksum.
    const uint64 FirstChecksum = Sim->StateChecksum();
    Bridge->StopPrototypeScenario();
    if (!TestTrue(TEXT("Replay scenario restarts"),
                  Bridge->StartPrototypeScenario())) return false;
    // The checksum folds in the tick counter and every other entity, so the
    // second run has to repeat the first run's whole timeline: the same
    // unpause, the same three settle ticks before the mover is reset, the
    // same tick between the unqueued leg and the queued pair, the same four
    // settle ticks and the same consume count. Any tick added or dropped on
    // one side is a different match, not a determinism failure.
    Bridge->SetScenarioPaused(false);
    for (int32 Settle = 0; Settle < 3; ++Settle) Bridge->Tick(0.05f);
    auto* Replay = const_cast<Simulation*>(Bridge->GetSimulation());
    if (!TestNotNull(TEXT("Replay authority exists"), Replay)) return false;
    Entity* ReplayMover = Replay->MutableEntityForTesting(MoverId);
    if (TestNotNull(TEXT("Replay fixture recreates the mover"), ReplayMover))
    {
        ReplayMover->order = {};
        ReplayMover->orderQueue.clear();
        TestTrue(TEXT("Replay mover restarts at the first run's origin"),
            ReplayMover->position.x == Origin.x && ReplayMover->position.y == Origin.y);
        TestTrue(TEXT("Replay unqueued move is accepted"),
            Bridge->IssueCommand(CommandType::Move, MoverId, 0, World1,
                                 FutureWellChoice::Dormant, Feedback, false));
        Bridge->Tick(0.05f);
        TestTrue(TEXT("Replay first queued move is accepted"),
            Bridge->IssueCommand(CommandType::Move, MoverId, 0, World2,
                                 FutureWellChoice::Dormant, Feedback, true));
        TestTrue(TEXT("Replay second queued move is accepted"),
            Bridge->IssueCommand(CommandType::Move, MoverId, 0, World3,
                                 FutureWellChoice::Dormant, Feedback, true));
        for (int32 Settle = 0; Settle < 4; ++Settle) Bridge->Tick(0.05f);
        for (int32 Step = 0; Step < ConsumeTicks; ++Step) Bridge->Tick(0.05f);
        TestEqual(TEXT("A queued sequence replays to the same checksum"),
            Replay->StateChecksum(), FirstChecksum);
    }

    Bridge->StopPrototypeScenario();
    return true;
}

#endif
