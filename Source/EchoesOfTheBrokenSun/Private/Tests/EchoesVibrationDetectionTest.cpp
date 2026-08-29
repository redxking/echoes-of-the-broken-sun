#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesHUD.h"
#include "EchoesSimCore/Simulation.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

#include <algorithm>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesVibrationDetectionTest,
    "Echoes.Runtime.Gameplay.VibrationDetection",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesVibrationDetectionTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the vibration-detection test world."));
        return false;
    }

    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge =
        World != nullptr
            ? World->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (!TestNotNull(TEXT("Detection world owns the simulation subsystem"), Bridge) ||
        !TestTrue(TEXT("Detection scenario starts"),
                  Bridge != nullptr && Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    bool bFoundResonant = false;
    bool bFoundListeningSpine = false;
    for (const echoes::sim::Entity& Entity : Bridge->GetSimulation()->Entities())
    {
        bFoundResonant |=
            Entity.owner == UEchoesSimulationSubsystem::OpponentPlayerId &&
            Entity.faction == echoes::sim::Faction::KharuunAssemblies &&
            Entity.type == echoes::sim::EntityType::ScoutUnit;
        bFoundListeningSpine |=
            Entity.owner == UEchoesSimulationSubsystem::OpponentPlayerId &&
            Entity.faction == echoes::sim::Faction::KharuunAssemblies &&
            Entity.type == echoes::sim::EntityType::UtilityStructure &&
            !Entity.temporaryMineralCover;
    }
    TestTrue(TEXT("The authored scenario contains a Resonant"), bFoundResonant);
    TestTrue(TEXT("The authored scenario contains a Listening Spine"),
             bFoundListeningSpine);

    echoes::sim::SimulationConfig Config = Bridge->GetSimulation()->Config();
    Config.mapWidthTiles = 64;
    Config.mapHeightTiles = 64;
    Config.randomSeed = 0x56494252415545ULL;
    echoes::sim::Simulation Detection(Config);
    TestTrue(TEXT("Meridian detection target player is admitted"),
             Detection.AddPlayer(
                 0, echoes::sim::Faction::MeridianCompact, {0, 0}));
    TestTrue(TEXT("Kharuun detector player is admitted"),
             Detection.AddPlayer(
                 1, echoes::sim::Faction::KharuunAssemblies, {0, 0}));
    const echoes::sim::EntityId Resonant = Detection.SpawnEntity(
        1,
        echoes::sim::Faction::KharuunAssemblies,
        echoes::sim::EntityType::ScoutUnit,
        echoes::sim::Vec2::FromTiles(20, 20));
    const echoes::sim::EntityId HiddenLancer = Detection.SpawnEntity(
        0,
        echoes::sim::Faction::MeridianCompact,
        echoes::sim::EntityType::Soldier,
        echoes::sim::Vec2::FromTiles(38, 20));
    TestTrue(TEXT("Authored detection fixture spawns"),
             Resonant != 0 && HiddenLancer != 0);

    echoes::sim::Command Move;
    Move.executeTick = 0;
    Move.player = 0;
    Move.sequence = 1;
    Move.type = echoes::sim::CommandType::Move;
    Move.actor = HiddenLancer;
    Move.position = echoes::sim::Vec2::FromTiles(37, 20);
    TestTrue(TEXT("Hidden movement command is admitted"),
             Detection.QueueCommand(Move));
    Detection.Step();

    const std::optional<echoes::sim::PlayerView> View =
        Detection.CreatePlayerView(1);
    if (TestTrue(TEXT("Kharuun player view materializes"), View.has_value()))
    {
        TestEqual(TEXT("One anonymous vibration contact is exposed"),
                  static_cast<int32>(View->VibrationSignatures().size()), 1);
        TestFalse(
            TEXT("The hidden source entity is not disclosed"),
            std::any_of(
                View->Entities().begin(),
                View->Entities().end(),
                [HiddenLancer](const echoes::sim::Entity& Entity)
                {
                    return Entity.id == HiddenLancer;
                }));
        if (!View->VibrationSignatures().empty())
        {
            TestEqual(
                TEXT("The contact is quantized to the authored cell resolution"),
                View->VibrationSignatures()[0].approximatePosition,
                echoes::sim::Vec2::FromTiles(37, 21));
        }
        const std::vector<echoes::sim::Command> AiCommands =
            echoes::sim::Simulation::GenerateAiCommands(
                *View, echoes::sim::AiPersonality::Adaptive);
        TestTrue(
            TEXT("Adaptive AI investigates the anonymous contact without direct targeting"),
            std::any_of(
                AiCommands.begin(),
                AiCommands.end(),
                [Resonant](const echoes::sim::Command& Command)
                {
                    return Command.actor == Resonant &&
                           Command.type ==
                               echoes::sim::CommandType::AttackMove &&
                           Command.target == 0;
                }));
    }

    AEchoesHUD* Hud = World->SpawnActor<AEchoesHUD>();
    TestNotNull(TEXT("The HUD with anonymous contact presentation spawns"), Hud);
    if (Hud != nullptr)
    {
        Hud->Destroy();
    }

    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors() && !WorldWrapper.HasFailed();
}

#endif
