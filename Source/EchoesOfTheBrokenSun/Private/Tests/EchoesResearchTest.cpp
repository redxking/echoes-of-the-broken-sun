#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "EchoesSimCore/Simulation.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesTechnologyPanelLayout.h"
#include "Engine/World.h"
#include "Misc/ConfigCacheIni.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesResearchTest,
    "Echoes.Runtime.Gameplay.FactionResearch",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesResearchTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    FEchoesScopedTestSaveEnvironment TestSaveEnvironment(*this);
    if (!TestSaveEnvironment.IsReady())
    {
        return false;
    }
    // Per-player terrain and object memory is now serialized into snapshots,
    // so the native snapshot schema moved 24 -> 25. The replay envelope shape
    // did not change, so the replay schema deliberately stays at 24.
    TestEqual(TEXT("Research interruption uses snapshot schema 26"),
              echoes::sim::kSnapshotVersion, 26U);
    TestEqual(TEXT("Research interruption uses replay schema 24"),
              echoes::sim::kReplayVersion, 24U);
    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the faction-research test world."));
        return false;
    }
    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge =
        World != nullptr
            ? World->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (!TestNotNull(TEXT("Research world owns the simulation subsystem"), Bridge) ||
        !TestTrue(TEXT("Research scenario starts"),
                  Bridge != nullptr && Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    const echoes::sim::Simulation* Scenario = Bridge->GetSimulation();
    if (!TestNotNull(TEXT("Research scenario has deterministic state"), Scenario))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const echoes::sim::ResearchRules* Targeting = Scenario->ResearchDefinition(
        echoes::sim::ResearchType::MeridianPrismaticTargeting);
    const echoes::sim::ResearchRules* Lattice = Scenario->ResearchDefinition(
        echoes::sim::ResearchType::MeridianHorizonLattice);
    const echoes::sim::ResearchRules* Cartography = Scenario->ResearchDefinition(
        echoes::sim::ResearchType::KharuunEchoCartography);
    TestNotNull(TEXT("Prismatic Targeting is compiled into runtime rules"), Targeting);
    TestNotNull(TEXT("Horizon Lattice is compiled into runtime rules"), Lattice);
    TestNotNull(TEXT("Echo Cartography is compiled into runtime rules"), Cartography);
    if (Targeting != nullptr && Lattice != nullptr && Cartography != nullptr)
    {
        TestEqual(TEXT("Targeting has authored Matter cost"),
                  Targeting->cost.material, 120);
        TestEqual(TEXT("Targeting has authored damage modifier"),
                  Targeting->combatDamagePercent, 115);
        TestTrue(TEXT("Lattice requires Targeting"),
                 Lattice->prerequisite ==
                     echoes::sim::ResearchType::MeridianPrismaticTargeting);
        TestEqual(TEXT("Cartography has authored vision modifier"),
                  Cartography->combatVisionPercent, 120);
    }

    uint32 ScenarioFoundry = 0;
    for (const echoes::sim::Entity& Entity : Scenario->Entities())
    {
        if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            Entity.type == echoes::sim::EntityType::Barracks)
        {
            ScenarioFoundry = Entity.id;
            break;
        }
    }
    FString Feedback;
    TestFalse(
        TEXT("Unfunded research is rejected before command admission"),
        Bridge->IssueResearchCommand(
            ScenarioFoundry,
            echoes::sim::ResearchType::MeridianPrismaticTargeting,
            Feedback));
    TestTrue(TEXT("Research rejection explains the resource boundary"),
             Feedback.Contains(TEXT("INSUFFICIENT_RESOURCES")));

    echoes::sim::Simulation Simulation(Scenario->Config());
    TestTrue(TEXT("Standalone authored-rules player initializes"),
             Simulation.AddPlayer(
                 0, echoes::sim::Faction::MeridianCompact, {1000, 500}));
    const echoes::sim::EntityId Foundry = Simulation.SpawnEntity(
        0,
        echoes::sim::Faction::MeridianCompact,
        echoes::sim::EntityType::Barracks,
        echoes::sim::Vec2::FromTiles(6, 6));
    const echoes::sim::EntityId Lancer = Simulation.SpawnEntity(
        0,
        echoes::sim::Faction::MeridianCompact,
        echoes::sim::EntityType::Soldier,
        echoes::sim::Vec2::FromTiles(7, 6));
    echoes::sim::Command Research;
    Research.executeTick = 0;
    Research.player = 0;
    Research.sequence = 1;
    Research.type = echoes::sim::CommandType::Research;
    Research.actor = Foundry;
    Research.researchType =
        echoes::sim::ResearchType::MeridianPrismaticTargeting;
    Simulation.CaptureReplayBaseline();
    TestTrue(TEXT("Authored research command is admitted"),
             Simulation.QueueCommand(Research));
    Simulation.Step(180);
    const echoes::sim::PlayerState* Player = Simulation.FindPlayer(0);
    TestTrue(TEXT("Research completes only after its authored duration"),
             Player != nullptr && Player->HasCompletedResearch(
                 echoes::sim::ResearchType::MeridianPrismaticTargeting));
    TestEqual(TEXT("Completed research updates an existing Lancer"),
              Simulation.FindEntity(Lancer)->attackDamage, 20);
    std::string ReplayError;
    const std::optional<echoes::sim::Simulation> Replayed =
        echoes::sim::Simulation::ReplayToEnd(
            Simulation.ExportReplay(), &ReplayError);
    TestTrue(TEXT("Research replay reaches the same final state"),
             Replayed.has_value() &&
                 Replayed->StateChecksum() == Simulation.StateChecksum());

    echoes::sim::Simulation Cancelled(Scenario->Config());
    TestTrue(TEXT("Cancellation test player initializes"),
             Cancelled.AddPlayer(
                 0, echoes::sim::Faction::MeridianCompact, {1000, 500}));
    const echoes::sim::EntityId CancelledFoundry = Cancelled.SpawnEntity(
        0,
        echoes::sim::Faction::MeridianCompact,
        echoes::sim::EntityType::Barracks,
        echoes::sim::Vec2::FromTiles(6, 6));
    echoes::sim::Command CancellableResearch;
    CancellableResearch.executeTick = 0;
    CancellableResearch.player = 0;
    CancellableResearch.sequence = 1;
    CancellableResearch.type = echoes::sim::CommandType::Research;
    CancellableResearch.actor = CancelledFoundry;
    CancellableResearch.researchType =
        echoes::sim::ResearchType::MeridianPrismaticTargeting;
    TestTrue(TEXT("Player research starts before cancellation"),
             Cancelled.QueueCommand(CancellableResearch));
    Cancelled.Step();
    const echoes::sim::ResourcePool ResourcesAfterCommit =
        Cancelled.FindPlayer(0)->resources;
    echoes::sim::Command CancelResearch;
    CancelResearch.executeTick = Cancelled.CurrentTick();
    CancelResearch.player = 0;
    CancelResearch.sequence = 2;
    CancelResearch.type = echoes::sim::CommandType::Stop;
    CancelResearch.actor = CancelledFoundry;
    TestTrue(TEXT("Stop command admits a player-driven research interruption"),
             Cancelled.QueueCommand(CancelResearch));
    Cancelled.Step();
    const echoes::sim::PlayerState* CancelledPlayer =
        Cancelled.FindPlayer(0);
    TestTrue(TEXT("Player-driven interruption clears active research"),
             CancelledPlayer != nullptr &&
                 CancelledPlayer->activeResearch ==
                     echoes::sim::ResearchType::None &&
                 CancelledPlayer->lastInterruptedResearch ==
                     echoes::sim::ResearchType::MeridianPrismaticTargeting);
    TestTrue(TEXT("Player-driven interruption does not refund committed costs"),
             CancelledPlayer != nullptr &&
                 CancelledPlayer->resources == ResourcesAfterCommit);

    TArray<FString> InputMappings;
    GConfig->GetArray(
        TEXT("/Script/Engine.InputSettings"),
        TEXT("ActionMappings"),
        InputMappings,
        GInputIni);
    TestTrue(
        TEXT("Shift+R faction research mapping is present"),
        InputMappings.ContainsByPredicate(
            [](const FString& Mapping)
            {
                return Mapping.Contains(TEXT("ActionName=\"ResearchNext\"")) &&
                       Mapping.Contains(TEXT("bShift=True")) &&
                       Mapping.Contains(TEXT("Key=R"));
            }));
    TestTrue(
        TEXT("F2 technology archive mapping is present"),
        InputMappings.ContainsByPredicate(
            [](const FString& Mapping)
            {
                return Mapping.Contains(
                           TEXT("ActionName=\"ToggleTechnologyPanel\"")) &&
                       Mapping.Contains(TEXT("Key=F2"));
            }));
    TestTrue(
        TEXT("Up technology archive focus mapping is present"),
        InputMappings.ContainsByPredicate(
            [](const FString& Mapping)
            {
                return Mapping.Contains(
                           TEXT("ActionName=\"TechnologyFocusPrevious\"")) &&
                       Mapping.Contains(TEXT("Key=Up"));
            }));
    TestTrue(
        TEXT("Down technology archive focus mapping is present"),
        InputMappings.ContainsByPredicate(
            [](const FString& Mapping)
            {
                return Mapping.Contains(
                           TEXT("ActionName=\"TechnologyFocusNext\"")) &&
                       Mapping.Contains(TEXT("Key=Down"));
            }));

    const FEchoesTechnologyPanelLayout PanelLayout =
        FEchoesTechnologyPanelLayout::Build(FVector2D(1600.0f, 900.0f), 1.0f);
    TestTrue(TEXT("Technology panel remains inside the accepted viewport"),
             PanelLayout.Origin.X >= 0.0f && PanelLayout.Origin.Y >= 0.0f &&
                 PanelLayout.Origin.X + PanelLayout.Size.X <= 1600.0f &&
                 PanelLayout.Origin.Y + PanelLayout.Size.Y <= 900.0f);
    TestFalse(TEXT("Technology tier pointer targets do not overlap"),
              PanelLayout.TechnologyRows[0].Intersect(
                  PanelLayout.TechnologyRows[1]));
    TestTrue(TEXT("Technology close target is distinct from both tiers"),
             !PanelLayout.CloseButton.Intersect(
                  PanelLayout.TechnologyRows[0]) &&
                 !PanelLayout.CloseButton.Intersect(
                  PanelLayout.TechnologyRows[1]));

    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors() && !WorldWrapper.HasFailed();
}

#endif
