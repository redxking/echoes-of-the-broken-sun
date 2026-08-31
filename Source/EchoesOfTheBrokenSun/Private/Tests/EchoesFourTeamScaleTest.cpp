#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "EchoesEntityView.h"
#include "EchoesSimCore/Simulation.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Tests/AutomationCommon.h"

#include <array>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesFourTeamScaleTest,
    "Echoes.Runtime.Performance.FourTeamScale",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesFourTeamScaleTest::RunTest(const FString& Parameters)
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
        AddError(TEXT("Could not create the four-team scale test world."));
        return false;
    }

    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge =
        World != nullptr
            ? World->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (!TestNotNull(TEXT("Scale world owns the simulation subsystem"), Bridge) ||
        !TestTrue(TEXT("Four-team scale scenario starts"),
                  Bridge != nullptr && Bridge->StartStressScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const echoes::sim::Simulation* Simulation = Bridge->GetSimulation();
    if (!TestNotNull(TEXT("Scale simulation exists"), Simulation))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    TestTrue(TEXT("Scale scenario is explicitly identified"),
             Bridge->IsStressScenario());
    TestEqual(TEXT("Scale scenario contains 400 units and one Future Well"),
              static_cast<int32>(Simulation->Entities().size()),
              401);

    std::array<int32, echoes::sim::kMaximumPlayers> OwnedUnitCounts{};
    int32 FutureWellCount = 0;
    for (const echoes::sim::Entity& Entity : Simulation->Entities())
    {
        if (Entity.owner < OwnedUnitCounts.size())
        {
            ++OwnedUnitCounts[Entity.owner];
        }
        else if (Entity.type == echoes::sim::EntityType::FutureWell)
        {
            ++FutureWellCount;
        }
    }
    for (echoes::sim::PlayerId Player = 0;
         Player < echoes::sim::kMaximumPlayers;
         ++Player)
    {
        TestNotNull(
            *FString::Printf(TEXT("Scale player %u is active"), Player),
            Simulation->FindPlayer(Player));
        TestEqual(
            *FString::Printf(TEXT("Scale player %u owns 100 units"), Player),
            OwnedUnitCounts[Player],
            100);
    }
    TestEqual(TEXT("Scale scenario retains one central Future Well"),
              FutureWellCount,
              1);
    TestEqual(TEXT("Four surviving Command Cores keep the match active"),
              static_cast<uint8>(Simulation->Outcome()),
              static_cast<uint8>(echoes::sim::MatchOutcome::Ongoing));

    std::array<int32, echoes::sim::kMaximumPlayers> AttackMoveCounts{};
    for (const echoes::sim::Command& Command : Simulation->CommandLog())
    {
        if (Command.player < AttackMoveCounts.size() &&
            Command.type == echoes::sim::CommandType::AttackMove)
        {
            ++AttackMoveCounts[Command.player];
            TestEqual(TEXT("Scale attack-move executes on the first fixed tick"),
                      Command.executeTick,
                      static_cast<echoes::sim::Tick>(1));
        }
    }
    TestEqual(TEXT("Scale scenario queues 396 broad attack-move orders"),
              static_cast<int32>(Simulation->CommandLog().size()),
              396);
    for (echoes::sim::PlayerId Player = 0;
         Player < echoes::sim::kMaximumPlayers;
         ++Player)
    {
        TestEqual(
            *FString::Printf(TEXT("Player %u queues 99 attack-move orders"), Player),
            AttackMoveCounts[Player],
            99);
    }

    int32 VisibleViewCount = 0;
    std::array<int32, echoes::sim::kMaximumPlayers> MarkerCounts{};
    for (TActorIterator<AEchoesEntityView> It(World); It; ++It)
    {
        ++VisibleViewCount;
        if (It->GetOwnerMarkerVariant() < MarkerCounts.size())
        {
            TestTrue(TEXT("Owned scale view exposes a non-color marker"),
                     It->IsOwnerMarkerVisible());
            ++MarkerCounts[It->GetOwnerMarkerVariant()];
        }
    }
    TestEqual(TEXT("Real visibility exposes every scale entity to presentation"),
              VisibleViewCount,
              401);
    for (echoes::sim::PlayerId Player = 0;
         Player < echoes::sim::kMaximumPlayers;
         ++Player)
    {
        TestEqual(
            *FString::Printf(TEXT("Player %u has 100 geometric ownership markers"), Player),
            MarkerCounts[Player],
            100);
    }

    for (int32 TickIndex = 0; TickIndex < 20; ++TickIndex)
    {
        Bridge->Tick(0.05f);
    }
    int32 DamagedSoldiers = 0;
    int32 RemainingSoldiers = 0;
    for (const echoes::sim::Entity& Entity : Simulation->Entities())
    {
        if (Entity.type == echoes::sim::EntityType::Soldier)
        {
            ++RemainingSoldiers;
            DamagedSoldiers += Entity.hitPoints < Entity.maxHitPoints ? 1 : 0;
        }
    }
    TestTrue(
        TEXT("Four-team attack-move fixture enters real deterministic combat by tick 20"),
        DamagedSoldiers > 0 || RemainingSoldiers < 396);

    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif
