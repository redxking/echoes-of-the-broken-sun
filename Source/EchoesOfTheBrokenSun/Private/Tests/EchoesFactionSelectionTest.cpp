#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesPlayerController.h"
#include "EchoesSimCore/Simulation.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/World.h"
#include "Misc/ConfigCacheIni.h"
#include "Tests/AutomationCommon.h"

#include <algorithm>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesFactionSelectionTest,
    "Echoes.Runtime.Gameplay.FactionSelection",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesFactionSelectionTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the faction-selection test world."));
        return false;
    }

    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge =
        World != nullptr
            ? World->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (!TestNotNull(TEXT("Faction world owns the simulation subsystem"), Bridge) ||
        !TestTrue(TEXT("Default Glass Scar scenario starts"),
                  Bridge != nullptr && Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    TestTrue(
        TEXT("Glass Scar defaults to Meridian for compatibility"),
        Bridge->GetLocalFaction() ==
            echoes::sim::Faction::MeridianCompact);
    AEchoesPlayerController* Controller =
        World->SpawnActor<AEchoesPlayerController>();
    if (!TestNotNull(TEXT("Faction controller can be created"), Controller))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    Controller->PresentTitleScreen();
    Controller->CyclePlayableFaction();
    TestTrue(TEXT("Faction choice remains on the title screen"),
             Controller->IsTitleScreenVisible());
    TestTrue(TEXT("Faction rebuild stays paused before deployment"),
             Bridge->IsScenarioPaused());
    TestTrue(
        TEXT("Tab-path faction selection assigns Kharuun to player zero"),
        Bridge->GetLocalFaction() ==
            echoes::sim::Faction::KharuunAssemblies);
    TestTrue(
        TEXT("The adaptive opponent becomes Meridian"),
        Bridge->GetOpponentFaction() ==
            echoes::sim::Faction::MeridianCompact);
    TestEqual(TEXT("Controller reports the selected force"),
              Controller->GetLocalFactionLabel(),
              FString(TEXT("KHARUUN ASSEMBLIES")));

    const echoes::sim::Simulation* Selected = Bridge->GetSimulation();
    const echoes::sim::PlayerState* LocalPlayer =
        Selected != nullptr
            ? Selected->FindPlayer(UEchoesSimulationSubsystem::LocalPlayerId)
            : nullptr;
    const echoes::sim::PlayerState* OpponentPlayer =
        Selected != nullptr
            ? Selected->FindPlayer(UEchoesSimulationSubsystem::OpponentPlayerId)
            : nullptr;
    TestNotNull(TEXT("Selected local player exists"), LocalPlayer);
    TestNotNull(TEXT("Selected opponent exists"), OpponentPlayer);
    if (LocalPlayer != nullptr)
    {
        TestTrue(TEXT("Authoritative player faction is Kharuun"),
                 LocalPlayer->faction ==
                     echoes::sim::Faction::KharuunAssemblies);
    }
    if (OpponentPlayer != nullptr)
    {
        TestTrue(TEXT("Authoritative opponent faction is Meridian"),
                 OpponentPlayer->faction ==
                     echoes::sim::Faction::MeridianCompact);
    }

    uint32 LocalCore = 0;
    uint32 LocalBasin = 0;
    uint32 LocalWaystone = 0;
    uint32 LocalResonant = 0;
    uint32 LocalListeningSpine = 0;
    bool bAllLocalEntitiesAreKharuun = true;
    bool bAllOpponentEntitiesAreMeridian = true;
    if (Selected != nullptr)
    {
        for (const echoes::sim::Entity& Entity : Selected->Entities())
        {
            if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId)
            {
                bAllLocalEntitiesAreKharuun &=
                    Entity.faction ==
                    echoes::sim::Faction::KharuunAssemblies;
                LocalCore = Entity.type == echoes::sim::EntityType::CommandCore
                                ? Entity.id
                                : LocalCore;
                LocalBasin = Entity.type == echoes::sim::EntityType::Barracks
                                 ? Entity.id
                                 : LocalBasin;
                LocalWaystone = Entity.type == echoes::sim::EntityType::Dropoff
                                    ? Entity.id
                                    : LocalWaystone;
                LocalResonant = Entity.type == echoes::sim::EntityType::ScoutUnit
                                    ? Entity.id
                                    : LocalResonant;
                LocalListeningSpine =
                    Entity.type == echoes::sim::EntityType::UtilityStructure &&
                            !Entity.temporaryMineralCover
                        ? Entity.id
                        : LocalListeningSpine;
            }
            else if (Entity.owner ==
                     UEchoesSimulationSubsystem::OpponentPlayerId)
            {
                bAllOpponentEntitiesAreMeridian &=
                    Entity.faction ==
                    echoes::sim::Faction::MeridianCompact;
            }
        }
    }
    TestTrue(TEXT("Every locally owned roster entity is Kharuun"),
             bAllLocalEntitiesAreKharuun);
    TestTrue(TEXT("Every opposing roster entity is Meridian"),
             bAllOpponentEntitiesAreMeridian);
    TestTrue(TEXT("Direct Kharuun roster contains a Memory Hearth"),
             LocalCore != 0);
    TestTrue(TEXT("Direct Kharuun roster contains a Growth Basin"),
             LocalBasin != 0);
    TestTrue(TEXT("Direct Kharuun roster contains a Waystone"),
             LocalWaystone != 0);
    TestTrue(TEXT("Direct Kharuun roster contains a Resonant"),
             LocalResonant != 0);
    TestTrue(TEXT("Direct Kharuun roster contains a Listening Spine"),
             LocalListeningSpine != 0);

    FString Feedback;
    TestTrue(
        TEXT("The locally owned Waystone accepts its faction command"),
        LocalWaystone != 0 &&
            Bridge->IssueCommand(
                echoes::sim::CommandType::ToggleWaystoneRoot,
                LocalWaystone,
                0,
                Bridge->SimToWorld(
                    Bridge->FindEntity(LocalWaystone)->position),
                echoes::sim::FutureWellChoice::Dormant,
                Feedback));
    TestTrue(
        TEXT("The Kharuun Memory Hearth accepts Tender production"),
        LocalCore != 0 &&
            Bridge->IssueProductionCommand(
                LocalCore,
                echoes::sim::EntityType::Worker,
                Feedback));
    TestTrue(
        TEXT("The Kharuun Growth Basin accepts Riftstalker production"),
        LocalBasin != 0 &&
            Bridge->IssueProductionCommand(
                LocalBasin,
                echoes::sim::EntityType::Soldier,
                Feedback));

    Bridge->SetScenarioPaused(false);
    Bridge->Tick(0.10f);
    const echoes::sim::Entity* Waystone = Bridge->FindEntity(LocalWaystone);
    if (TestNotNull(TEXT("Local Waystone remains authoritative"), Waystone))
    {
        TestTrue(
            TEXT("Direct player command begins the authored uproot transition"),
            Waystone->waystoneMode ==
                echoes::sim::WaystoneMode::Uprooting);
    }

    echoes::sim::Simulation* MutableSimulation =
        const_cast<echoes::sim::Simulation*>(Bridge->GetSimulation());
    const echoes::sim::EntityId HiddenMover =
        MutableSimulation != nullptr
            ? MutableSimulation->SpawnEntity(
                  UEchoesSimulationSubsystem::OpponentPlayerId,
                  echoes::sim::Faction::MeridianCompact,
                  echoes::sim::EntityType::Soldier,
                  echoes::sim::Vec2::FromTiles(33, 6))
            : 0;
    TestTrue(TEXT("Local detection reachability fixture spawns"),
             HiddenMover != 0);
    if (MutableSimulation != nullptr && HiddenMover != 0)
    {
        echoes::sim::Command Move;
        Move.executeTick = MutableSimulation->CurrentTick() + 1;
        Move.player = UEchoesSimulationSubsystem::OpponentPlayerId;
        Move.sequence = *MutableSimulation->NextCommandSequence(
            UEchoesSimulationSubsystem::OpponentPlayerId);
        Move.type = echoes::sim::CommandType::Move;
        Move.actor = HiddenMover;
        Move.position = echoes::sim::Vec2::FromTiles(32, 6);
        TestTrue(TEXT("Hidden Meridian movement is admitted"),
                 MutableSimulation->QueueCommand(Move));
        Bridge->Tick(0.10f);
        const std::optional<echoes::sim::PlayerView> LocalView =
            MutableSimulation->CreatePlayerView(
                UEchoesSimulationSubsystem::LocalPlayerId);
        if (TestTrue(TEXT("Direct Kharuun player view materializes"),
                     LocalView.has_value()))
        {
            TestTrue(
                TEXT("Direct Kharuun play receives an anonymous vibration contact"),
                !LocalView->VibrationSignatures().empty());
            TestFalse(
                TEXT("The direct local contact does not reveal the hidden entity"),
                std::any_of(
                    LocalView->Entities().begin(),
                    LocalView->Entities().end(),
                    [HiddenMover](const echoes::sim::Entity& Entity)
                    {
                        return Entity.id == HiddenMover;
                    }));
        }
    }

    Controller->ConfirmPrimaryAction();
    TestTrue(TEXT("Selected Kharuun force reaches the operations brief"),
             Controller->IsMissionBriefingVisible());
    TestTrue(
        TEXT("Faction selection survives title-to-brief transition"),
        Bridge->GetLocalFaction() ==
            echoes::sim::Faction::KharuunAssemblies);

    Controller->ConfirmPrimaryAction();
    TestFalse(TEXT("Second Enter deploys from the operations brief"),
              Controller->IsMissionBriefingVisible());
    Controller->CyclePlayableFaction();
    TestEqual(TEXT("Live Tab selects one owned entity without a pointer"),
              Controller->GetSelectedEntityIds().Num(), 1);
    const uint32 FirstKeyboardSelection =
        Controller->GetSelectedEntityIds().IsEmpty()
            ? 0
            : Controller->GetSelectedEntityIds()[0];
    TestTrue(TEXT("Keyboard selection is locally owned"),
             FirstKeyboardSelection != 0 &&
                 Bridge->FindEntity(FirstKeyboardSelection) != nullptr &&
                 Bridge->FindEntity(FirstKeyboardSelection)->owner ==
                     UEchoesSimulationSubsystem::LocalPlayerId);
    Controller->CycleOwnedEntityPrevious();
    TestEqual(TEXT("Backspace retains one pointer-independent selection"),
              Controller->GetSelectedEntityIds().Num(), 1);
    TestNotEqual(TEXT("Reverse cycle wraps to a different owned entity"),
                 Controller->GetSelectedEntityIds().IsEmpty()
                     ? 0u
                     : Controller->GetSelectedEntityIds()[0],
                 FirstKeyboardSelection);
    TestFalse(TEXT("Keyboard center targeting defaults off"),
              Controller->IsKeyboardTargetingEnabled());
    Controller->ToggleKeyboardTargeting();
    TestTrue(TEXT("Home path enables fair screen-reticle targeting"),
             Controller->IsKeyboardTargetingEnabled());
    TestEqual(TEXT("Home centers the keyboard target"),
              Controller->GetKeyboardTargetOffset(), FVector2D::ZeroVector);
    Controller->ToggleKeyboardTargeting();
    TestFalse(TEXT("Home path restores pointer targeting"),
              Controller->IsKeyboardTargetingEnabled());

    TArray<FString> InputMappings;
    GConfig->GetArray(
        TEXT("/Script/Engine.InputSettings"),
        TEXT("ActionMappings"),
        InputMappings,
        GInputIni);
    TestTrue(
        TEXT("Tab faction-choice mapping is present"),
        InputMappings.ContainsByPredicate(
            [](const FString& Mapping)
            {
                return Mapping.Contains(
                           TEXT("ActionName=\"CyclePlayableFaction\"")) &&
                       Mapping.Contains(TEXT("Key=Tab"));
            }));
    TestTrue(
        TEXT("Backspace reverse-selection mapping is present"),
        InputMappings.ContainsByPredicate(
            [](const FString& Mapping)
            {
                return Mapping.Contains(
                           TEXT("ActionName=\"CycleOwnedEntityPrevious\"")) &&
                       Mapping.Contains(TEXT("Key=BackSpace"));
            }));
    TestTrue(
        TEXT("Home center-target mapping is present"),
        InputMappings.ContainsByPredicate(
            [](const FString& Mapping)
            {
                return Mapping.Contains(
                           TEXT("ActionName=\"ToggleKeyboardTargeting\"")) &&
                       Mapping.Contains(TEXT("Key=Home"));
            }));
    TestTrue(
        TEXT("Space center-order mapping is present"),
        InputMappings.ContainsByPredicate(
            [](const FString& Mapping)
            {
                return Mapping.Contains(
                           TEXT("ActionName=\"KeyboardContextOrder\"")) &&
                       Mapping.Contains(TEXT("Key=SpaceBar"));
            }));
    TestTrue(
        TEXT("Left keyboard-target mapping is present"),
        InputMappings.ContainsByPredicate(
            [](const FString& Mapping)
            {
                return Mapping.Contains(
                           TEXT("ActionName=\"KeyboardTargetLeft\"")) &&
                       Mapping.Contains(TEXT("Key=Left"));
            }));
    TestTrue(
        TEXT("Right keyboard-target mapping is present"),
        InputMappings.ContainsByPredicate(
            [](const FString& Mapping)
            {
                return Mapping.Contains(
                           TEXT("ActionName=\"KeyboardTargetRight\"")) &&
                       Mapping.Contains(TEXT("Key=Right"));
            }));

    Controller->Destroy();
    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors() && !WorldWrapper.HasFailed();
}

#endif
