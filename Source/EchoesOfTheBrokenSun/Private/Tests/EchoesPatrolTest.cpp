#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesSimCore/Simulation.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesPatrolTest,
    "Echoes.Runtime.Gameplay.Patrol",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesPatrolTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the patrol test world."));
        return false;
    }
    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge =
        World != nullptr ? World->GetSubsystem<UEchoesSimulationSubsystem>() : nullptr;
    if (!TestNotNull(TEXT("Patrol world owns the simulation subsystem"), Bridge) ||
        !TestTrue(TEXT("Patrol scenario starts"),
                  Bridge != nullptr && Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    uint32 Soldier = 0;
    uint32 CommandCore = 0;
    for (const echoes::sim::Entity& Entity : Bridge->GetSimulation()->Entities())
    {
        if (Entity.owner != UEchoesSimulationSubsystem::LocalPlayerId)
        {
            continue;
        }
        if (Soldier == 0 && Entity.type == echoes::sim::EntityType::Soldier)
        {
            Soldier = Entity.id;
        }
        else if (CommandCore == 0 &&
                 Entity.type == echoes::sim::EntityType::CommandCore)
        {
            CommandCore = Entity.id;
        }
    }
    if (!TestTrue(TEXT("Patrol candidates exist"),
                  Soldier != 0 && CommandCore != 0))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const echoes::sim::Vec2 Anchor = Bridge->FindEntity(Soldier)->position;
    echoes::sim::Vec2 Endpoint = Anchor;
    for (int32 Radius = 3; Radius <= 10 && Endpoint == Anchor; ++Radius)
    {
        const echoes::sim::Vec2 Candidates[] = {
            echoes::sim::Vec2::FromRaw(
                Anchor.x.Raw() + Radius * echoes::sim::kFixedScale,
                Anchor.y.Raw()),
            echoes::sim::Vec2::FromRaw(
                Anchor.x.Raw() - Radius * echoes::sim::kFixedScale,
                Anchor.y.Raw()),
            echoes::sim::Vec2::FromRaw(
                Anchor.x.Raw(),
                Anchor.y.Raw() + Radius * echoes::sim::kFixedScale),
            echoes::sim::Vec2::FromRaw(
                Anchor.x.Raw(),
                Anchor.y.Raw() - Radius * echoes::sim::kFixedScale)};
        for (const echoes::sim::Vec2 Candidate : Candidates)
        {
            if (Bridge->GetSimulation()->IsPositionPassable(Candidate))
            {
                Endpoint = Candidate;
                break;
            }
        }
    }
    if (!TestTrue(TEXT("A distinct passable patrol endpoint exists"),
                  Endpoint != Anchor))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    FString Feedback;
    TestFalse(TEXT("An immobile structure rejects patrol"),
              Bridge->IssueCommand(
                  echoes::sim::CommandType::Patrol, CommandCore, 0,
                  Bridge->SimToWorld(Endpoint),
                  echoes::sim::FutureWellChoice::Harvest, Feedback));
    TestTrue(TEXT("Structure rejection is reason-coded"),
             Feedback.StartsWith(TEXT("[PATROL_REQUIRES_COMBAT_UNIT]")));

    Feedback.Reset();
    TestFalse(TEXT("An unchanged endpoint rejects patrol"),
              Bridge->IssueCommand(
                  echoes::sim::CommandType::Patrol, Soldier, 0,
                  Bridge->SimToWorld(Anchor),
                  echoes::sim::FutureWellChoice::Harvest, Feedback));
    TestTrue(TEXT("Unchanged endpoint rejection is reason-coded"),
             Feedback.StartsWith(TEXT("[PATROL_ENDPOINT_UNCHANGED]")));

    Feedback.Reset();
    TestTrue(TEXT("A mobile combat unit accepts patrol"),
             Bridge->IssueCommand(
                 echoes::sim::CommandType::Patrol, Soldier, 0,
                 Bridge->SimToWorld(Endpoint),
                 echoes::sim::FutureWellChoice::Harvest, Feedback));
    Bridge->Tick(0.05f);
    Bridge->Tick(0.05f);
    const echoes::sim::Entity* Patrolling = Bridge->FindEntity(Soldier);
    TestNotNull(TEXT("Patrolling soldier remains alive"), Patrolling);
    if (Patrolling != nullptr)
    {
        TestTrue(TEXT("Unreal adapter applies patrol"),
                 Patrolling->order.type == echoes::sim::OrderType::Patrol);
        TestTrue(TEXT("Patrol records its first endpoint"),
                 Patrolling->order.anchor == Anchor);
        TestTrue(TEXT("Patrol records its second endpoint"),
                 Patrolling->order.destination == Endpoint);
    }

    Feedback.Reset();
    TestTrue(TEXT("Stop interrupts patrol"),
             Bridge->IssueCommand(
                 echoes::sim::CommandType::Stop, Soldier, 0,
                 Bridge->SimToWorld(Bridge->FindEntity(Soldier)->position),
                 echoes::sim::FutureWellChoice::Harvest, Feedback));
    Bridge->Tick(0.05f);
    Bridge->Tick(0.05f);
    TestTrue(TEXT("Stop clears patrol"),
             Bridge->FindEntity(Soldier)->order.type ==
                 echoes::sim::OrderType::None);

    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors() && !WorldWrapper.HasFailed();
}

#endif
