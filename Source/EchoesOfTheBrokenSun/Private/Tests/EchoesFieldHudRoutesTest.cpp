#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "EchoesFieldHudView.h"
#include "EchoesMatchReplay.h"
#include "EchoesNetworkSession.h"
#include "EchoesPlayerController.h"
#include "EchoesRTSCameraPawn.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesSkirmishSetup.h"
#include "Engine/World.h"
#include "Misc/DateTime.h"
#include "Misc/Guid.h"
#include "Tests/AutomationCommon.h"

#include <algorithm>
#include <limits>
#include <optional>
#include <span>

namespace
{
template <typename Digest>
// File-unique by name. Separate anonymous namespaces merge into one when the
// build batches test files into a single translation unit, so a helper called
// DigestHex here collides with the identical helpers in
// EchoesSimulationSubsystemReplay.cpp and EchoesTrainingReadinessOperationTest.cpp.
// The error names the symbol rather than the collision, which reads as a local
// mistake; that exact class of break cost another lane time on 2026-09-10.
FString FieldHudRoutesDigestHex(const Digest& Value)
{
    FString Result;
    Result.Reserve(static_cast<int32>(Value.size() * 2));
    for (const uint8 Byte : Value)
    {
        Result += FString::Printf(TEXT("%02x"), Byte);
    }
    return Result;
}

echoes::sim::ReplayRecord MakeFieldHudReplay()
{
    using namespace echoes::sim;
    SimulationConfig Config{
        FEchoesSkirmishSetupModel::MapWidthTiles,
        FEchoesSkirmishSetupModel::MapHeightTiles,
        20,
        0x4649454c44485544ULL};
    // This direct core fixture bypasses the authored content catalog.
    Config.rules.contentSha256 =
        echoes::network::BuildCompatibilityManifest(nullptr).rulesPackSha256;
    auto& Soldier = Config.rules.archetypes
        [static_cast<size_t>(Faction::MeridianCompact)]
        [static_cast<size_t>(EntityType::Soldier)];
    Soldier.attackDamage = 5000;
    Soldier.attackRangeRaw = 3 * kFixedScale;
    Soldier.attackPeriodTicks = 1;

    Simulation Simulation(Config);
    for (int32 Y = 0; Y < FEchoesSkirmishSetupModel::MapHeightTiles; ++Y)
    {
        for (int32 X = 0; X < FEchoesSkirmishSetupModel::MapWidthTiles; ++X)
        {
            if (FEchoesSkirmishSetupModel::IsBlockedTile(
                    EEchoesSkirmishMapPreset::GlassScar, X, Y))
            {
                (void)Simulation.SetTerrainTile(X, Y, Terrain::Blocked);
            }
        }
    }
    Simulation.AddPlayer(0, Faction::MeridianCompact, {1000, 1000});
    Simulation.AddPlayer(1, Faction::KharuunAssemblies, {1000, 1000});
    Simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::CommandCore,
        Vec2::FromTiles(2, 2));
    const EntityId EnemyCore = Simulation.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::CommandCore,
        Vec2::FromTiles(18, 18));
    const EntityId Attacker = Simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Soldier,
        Vec2::FromTiles(17, 18));
    Simulation.CaptureReplayBaseline();
    Command Attack;
    Attack.executeTick = 2;
    Attack.player = 0;
    Attack.sequence = 1;
    Attack.type = CommandType::Attack;
    Attack.actor = Attacker;
    Attack.target = EnemyCore;
    Simulation.QueueCommand(Attack);
    Simulation.Step(4);
    return Simulation.ExportReplay();
}

bool PlayerViewContains(
    const echoes::sim::PlayerView& View,
    echoes::sim::EntityId EntityId)
{
    return std::any_of(
        View.Entities().begin(),
        View.Entities().end(),
        [EntityId](const echoes::sim::Entity& Entity)
        {
            return Entity.id == EntityId;
        });
}

FVector2D NormalizeMapPosition(
    const echoes::sim::Vec2& Position,
    const echoes::sim::SimulationConfig& Config)
{
    return FVector2D(
        static_cast<float>(Position.x.Raw()) /
            static_cast<float>(Config.mapWidthTiles * echoes::sim::kFixedScale),
        static_cast<float>(Position.y.Raw()) /
            static_cast<float>(Config.mapHeightTiles * echoes::sim::kFixedScale));
}

bool SelectExactOwnedEntity(
    AEchoesPlayerController& Controller,
    echoes::sim::EntityId EntityId,
    int32 MaximumAttempts)
{
    for (int32 Attempt = 0; Attempt < MaximumAttempts; ++Attempt)
    {
        Controller.CycleOwnedEntityPrevious();
        const TArray<uint32>& Selected = Controller.GetSelectedEntityIds();
        if (Selected.Num() == 1 && Selected[0] == EntityId)
        {
            return true;
        }
    }
    return false;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesFieldHudRoutesTest,
    "Echoes.Runtime.FieldHud.ControllerAuthorityRoutes",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesFieldHudRoutesTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    FEchoesScopedTestSaveEnvironment TestSaveEnvironment(*this);
    if (!TestSaveEnvironment.IsReady())
    {
        return false;
    }

    AddInfo(
        TEXT("Scope: semantic field-HUD controller dispatch, normalized "
             "minimap routing, and replay/live authority isolation. This is "
             "direct automation, not packaged physical-input or rendered "
             "acceptance evidence."));

    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the field-HUD route test world."));
        return false;
    }
    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge = World != nullptr
        ? World->GetSubsystem<UEchoesSimulationSubsystem>()
        : nullptr;
    if (!TestNotNull(TEXT("Field-HUD world owns a simulation subsystem"), Bridge) ||
        !TestTrue(TEXT("Field-HUD scenario starts"),
                  Bridge != nullptr && Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    FString Feedback;
    FEchoesSkirmishSetup FundedSetup = Bridge->GetActiveSkirmishSetup();
    FundedSetup.ResourceLevel = EEchoesSkirmishResourceLevel::Abundant;
    if (!TestTrue(
            TEXT("Route fixture applies an authored funded deployment"),
            Bridge->ApplySkirmishSetup(FundedSetup, Feedback)))
    {
        AddError(Feedback);
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    AEchoesPlayerController* Controller =
        World->SpawnActor<AEchoesPlayerController>();
    AEchoesRTSCameraPawn* Camera = World->SpawnActor<AEchoesRTSCameraPawn>();
    if (!TestNotNull(TEXT("Field-HUD controller spawns"), Controller) ||
        !TestNotNull(TEXT("Field-HUD minimap camera spawns"), Camera))
    {
        if (Controller != nullptr) Controller->Destroy();
        if (Camera != nullptr) Camera->Destroy();
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    Controller->InitInputSystem();
    Controller->Possess(Camera);
    if (Controller->IsTitleScreenVisible())
    {
        Controller->ConfirmPrimaryAction();
    }
    if (Controller->IsMissionBriefingVisible())
    {
        Controller->ConfirmPrimaryAction();
    }

    const echoes::sim::Simulation* Simulation = Bridge->GetSimulation();
    if (!TestNotNull(TEXT("Funded field simulation is available"), Simulation))
    {
        Controller->Destroy();
        Camera->Destroy();
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    FEchoesFieldHudView View = Controller->BuildFieldHudView();
    TestTrue(
        TEXT("Deployed field view is live and player scoped"),
        View.Surface == EEchoesFieldHudSurface::Battlefield &&
            View.Authority == EEchoesFieldHudAuthority::LivePlayerView);

    // SPEC-TUT-006 Tutorial skip modal routes
    Controller->SetTutorialOperationAuthorized(true);
    Controller->OpenTutorialSkipModal();
    TestTrue(TEXT("Tutorial skip modal is visible after open"), Controller->IsTutorialSkipModalVisible());
    View = Controller->BuildFieldHudView();
    TestTrue(TEXT("Tutorial skip modal is reflected in field HUD view"), View.TutorialSkipModal.bVisible);
    TestEqual(TEXT("Tutorial skip modal has 3 action controls"), View.TutorialSkipModal.Controls.Num(), 3);

    Controller->HandleFieldHudAction(EEchoesFieldHudAction::TutorialSkipCurrentStep);
    TestFalse(TEXT("Skip current step closes modal"), Controller->IsTutorialSkipModalVisible());
    TestTrue(TEXT("Skip current step records in session skipped mask"), (Controller->GetTutorialSkippedMask() & 1) != 0);
    TestEqual(TEXT("Skip current step does not grant durable profile mastery"), Controller->GetPlayerProfile().TutorialVerifiedMask, static_cast<uint16>(0));

    Controller->OpenTutorialSkipModal();
    Controller->HandleFieldHudAction(EEchoesFieldHudAction::TutorialCancelSkipModal);
    TestFalse(TEXT("Cancel closes tutorial skip modal"), Controller->IsTutorialSkipModalVisible());

    Controller->OpenTutorialSkipModal();
    // Profile initialization selects its journey slot and therefore requires a
    // paused menu, just as the real startup flow does.
    if (!TestTrue(TEXT("End-all fixture has a writable initialized profile"), Controller->InitializePlayerProfile())) return false;
    const auto ProfileBeforeFailure = Controller->GetPlayerProfile();
    FEchoesPlayerProfileStore::FailNextCommitForTesting();
    Controller->HandleFieldHudAction(EEchoesFieldHudAction::TutorialEndAll);
    TestTrue(TEXT("Failed end-all persistence preserves authorization and modal"),
        Controller->IsTutorialOperationAuthorized() && Controller->IsTutorialSkipModalVisible());
    TestTrue(TEXT("Failed end-all persistence rolls back profile"), Controller->GetPlayerProfile() == ProfileBeforeFailure);
    Controller->HandleFieldHudAction(EEchoesFieldHudAction::TutorialEndAll);
    TestFalse(TEXT("End all tutorials turns off tutorial authorization"), Controller->IsTutorialOperationAuthorized());
    TestFalse(TEXT("End all tutorials closes skip modal"), Controller->IsTutorialSkipModalVisible());
    FEchoesPlayerProfile PersistedOptOut;
    bool bProfileExists = false;
    TestTrue(TEXT("End all tutorials persists opt-out without mastery"),
        FEchoesPlayerProfileStore::LoadWithBackup(FEchoesPlayerProfileStore::GetDefaultPath(), PersistedOptOut, bProfileExists, Feedback) &&
        bProfileExists && PersistedOptOut.bTutorialOptOut && PersistedOptOut.TutorialVerifiedMask == 0);

    TestEqual(TEXT("End all tutorials does not grant durable profile mastery"), Controller->GetPlayerProfile().TutorialVerifiedMask, static_cast<uint16>(0));

    // Exercise the actual model-emitted ability route with a mixed force.
    // Admission and completion remain separate; a UI acknowledgement is not
    // evidence that the fixed-step ability has executed.
    Controller->SelectCombatForce();
    const auto RelayRouteView = Controller->BuildFieldHudView();
    if (!TestTrue(TEXT("Live mixed force offers an eligible Relay action"),
        RelayRouteView.Commands.Controls.ContainsByPredicate([](const FEchoesFieldHudControl& Control)
        { return Control.Action == EEchoesFieldHudAction::ActivateRelaySupply && Control.bEnabled; })))
    {
        Controller->Destroy(); Camera->Destroy(); Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this); return false;
    }
    const auto RelayPendingBefore = Simulation->PendingCommands().size();
    Controller->HandleFieldHudAction(EEchoesFieldHudAction::ActivateRelaySupply);
    const auto RelayPending = Simulation->PendingCommands();
    TArray<uint32> RoutedRelays;
    for (size_t Index = RelayPendingBefore; Index < RelayPending.size(); ++Index)
    {
        const auto* Caster = Simulation->FindEntity(RelayPending[Index].actor);
        TestTrue(TEXT("Ability card only dispatches owned Skiffs from a mixed group"),
            Caster && Caster->owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            Caster->type == echoes::sim::EntityType::ScoutUnit &&
            Caster->faction == echoes::sim::Faction::MeridianCompact &&
            RelayPending[Index].type == echoes::sim::CommandType::ActivateRelaySupply);
        RoutedRelays.Add(RelayPending[Index].actor);
    }
    if (!TestTrue(TEXT("Ability card queued at least one real activation"), !RoutedRelays.IsEmpty()))
    {
        Controller->Destroy(); Camera->Destroy(); Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this); return false;
    }
    Bridge->Tick(0.05f);
    Bridge->Tick(0.05f);
    for (uint32 RelayId : RoutedRelays)
    {
        const auto* Active = Simulation->FindEntity(RelayId);
        TestTrue(TEXT("Routed Skiff command executes in the simulation"), Active && Active->relaySupplyActive);
    }
    const auto ActiveRelayView = Controller->BuildFieldHudView();
    TestTrue(TEXT("Executed Relay action disables repeat activation"),
        ActiveRelayView.Commands.Controls.ContainsByPredicate([](const FEchoesFieldHudControl& Control)
        { return Control.Action == EEchoesFieldHudAction::ActivateRelaySupply && !Control.bEnabled; }));
    const auto BeforeRepeatedRelay = Simulation->PendingCommands().size();
    Controller->HandleFieldHudAction(EEchoesFieldHudAction::ActivateRelaySupply);
    TestEqual(TEXT("Stale repeated ability callback queues nothing"),
        static_cast<int32>(Simulation->PendingCommands().size()), static_cast<int32>(BeforeRepeatedRelay));

    // A model-emitted command-card action reaches the existing controller
    // authority and nowhere else.
    Controller->SelectCombatForce();
    const TArray<uint32> CombatSelection = Controller->GetSelectedEntityIds();
    View = Controller->BuildFieldHudView();
    const int32 HoldArgument =
        static_cast<int32>(EEchoesCommandDeckAction::Hold);
    TestTrue(
        TEXT("Live combat view emits the Hold command-card control"),
        View.Commands.bVisible &&
            View.Commands.Controls.ContainsByPredicate(
                [HoldArgument](const FEchoesFieldHudControl& Control)
                {
                    return Control.bEnabled &&
                        Control.Action == EEchoesFieldHudAction::CommandDeck &&
                        Control.Argument == HoldArgument;
                }));
    const int32 BeforeHold =
        static_cast<int32>(Simulation->PendingCommands().size());
    Controller->HandleFieldHudAction(
        EEchoesFieldHudAction::CommandDeck, HoldArgument);
    const std::span<const echoes::sim::Command> AfterHoldCommands =
        Simulation->PendingCommands();
    TestEqual(
        TEXT("Hold dispatch queues one authoritative command per selected unit"),
        static_cast<int32>(AfterHoldCommands.size()),
        BeforeHold + CombatSelection.Num());
    for (int32 Index = BeforeHold;
         Index < static_cast<int32>(AfterHoldCommands.size()); ++Index)
    {
        TestTrue(
            TEXT("Field-HUD command dispatch retains local Hold authority"),
            AfterHoldCommands[Index].player ==
                    UEchoesSimulationSubsystem::LocalPlayerId &&
                AfterHoldCommands[Index].type ==
                    echoes::sim::CommandType::Hold);
    }

    const int32 BarrierArgument = static_cast<int32>(EEchoesCommandDeckAction::ToggleBulwarkDeployment);
    const auto BarrierCard = Controller->BuildFieldHudView();
    if (!TestTrue(TEXT("Combat selection offers the Bulwark direction control"),
        BarrierCard.Commands.Controls.ContainsByPredicate([BarrierArgument](const auto& Control)
        { return Control.Action == EEchoesFieldHudAction::CommandDeck && Control.Argument == BarrierArgument && Control.bEnabled; })))
    {
        Controller->Destroy(); Camera->Destroy(); Bridge->StopPrototypeScenario(); return false;
    }
    const auto BeforeBarrierArm = Simulation->PendingCommands().size();
    Controller->HandleFieldHudAction(EEchoesFieldHudAction::CommandDeck, BarrierArgument);
    TestEqual(TEXT("Barrier button arms the battlefield direction route"),
        Controller->GetArmedDeckAction(), EEchoesCommandDeckAction::ToggleBulwarkDeployment);
    TestEqual(TEXT("Barrier button does not cast into the command card"),
        Simulation->PendingCommands().size(), BeforeBarrierArm);

    // Shell/modal ownership makes the semantic view hidden. Stale UMG
    // callbacks must therefore fail closed even if a retained widget invokes
    // them after the surface transition.
    Controller->TogglePauseMenu();
    TestEqual(
        TEXT("Pause shell hides the field-HUD semantic surface"),
        static_cast<int32>(Controller->BuildFieldHudView().Surface),
        static_cast<int32>(EEchoesFieldHudSurface::Hidden));
    const int32 CommandsBeforeStale =
        static_cast<int32>(Simulation->PendingCommands().size());
    const int32 CampaignNodeBeforeStale =
        Controller->GetSelectedCampaignMapNodeIndex();
    Controller->HandleFieldHudAction(
        EEchoesFieldHudAction::CommandDeck,
        static_cast<int32>(EEchoesCommandDeckAction::Stop));
    Controller->HandleFieldHudAction(
        EEchoesFieldHudAction::TechnologyResearchTier, 0);
    Controller->HandleFieldHudAction(
        EEchoesFieldHudAction::CampaignSelectNode, 5);
    Controller->HandleFieldHudAction(EEchoesFieldHudAction::OnlineJoin);
    Controller->HandleFieldHudEndpoint(TEXT("stale.invalid:7777"));
    TestFalse(
        TEXT("Modal field rejects normalized tactical orders"),
        Controller->HandleFieldHudPointer(FVector2D(0.5f, 0.5f), true));
    TestEqual(
        TEXT("Stale modal callbacks append no authoritative commands"),
        static_cast<int32>(Simulation->PendingCommands().size()),
        CommandsBeforeStale);
    TestEqual(
        TEXT("Stale campaign callback cannot change hidden map state"),
        Controller->GetSelectedCampaignMapNodeIndex(),
        CampaignNodeBeforeStale);
    Controller->TogglePauseMenu();

    const uint64 ChecksumBeforePan = Simulation->StateChecksum();
    const uint64 TickBeforePan = Simulation->CurrentTick();
    Camera->SetActorLocation(FVector(700.0f, -700.0f, 2200.0f));
    const FVector ExpectedCenter = Bridge->SimToWorld(
        echoes::sim::Vec2::FromTiles(
            Simulation->Config().mapWidthTiles / 2,
            Simulation->Config().mapHeightTiles / 2));
    TestTrue(
        TEXT("Normalized minimap left click is accepted"),
        Controller->HandleFieldHudPointer(FVector2D(0.5f, 0.5f), false));
    TestTrue(
        TEXT("Normalized minimap left click pans the camera to map center"),
        FMath::IsNearlyEqual(Camera->GetActorLocation().X, ExpectedCenter.X, 0.1f) &&
            FMath::IsNearlyEqual(Camera->GetActorLocation().Y, ExpectedCenter.Y, 0.1f));
    for (const FVector2D Corner : {FVector2D(0, 0), FVector2D(1, 0),
                                   FVector2D(0, 1), FVector2D(1, 1)})
    {
        TestTrue(TEXT("Each minimap corner accepts navigation"),
            Controller->HandleFieldHudPointer(Corner, false));
        const FVector Target = Bridge->SimToWorld(echoes::sim::Vec2::FromTiles(
            Corner.X * Simulation->Config().mapWidthTiles,
            Corner.Y * Simulation->Config().mapHeightTiles));
        TestTrue(TEXT("Each minimap corner is reachable as the camera target"),
            FVector2D::Distance(FVector2D(Camera->GetActorLocation()),
                               FVector2D(Target)) < 1.0f);
    }
    TestEqual(TEXT("Minimap pan preserves simulation tick"),
              Simulation->CurrentTick(), TickBeforePan);
    TestEqual(TEXT("Minimap pan preserves simulation checksum"),
              Simulation->StateChecksum(), ChecksumBeforePan);
    const FVector LocationAfterPan = Camera->GetActorLocation();
    TestFalse(
        TEXT("Normalized minimap rejects negative coordinates"),
        Controller->HandleFieldHudPointer(FVector2D(-0.01f, 0.5f), false));
    TestFalse(
        TEXT("Normalized minimap rejects coordinates above one"),
        Controller->HandleFieldHudPointer(FVector2D(0.5f, 1.01f), true));
    TestFalse(
        TEXT("Normalized minimap rejects non-finite coordinates"),
        Controller->HandleFieldHudPointer(
            FVector2D(std::numeric_limits<float>::quiet_NaN(), 0.5f),
            false));
    TestTrue(TEXT("Rejected minimap coordinates cannot move the camera"),
             Camera->GetActorLocation().Equals(LocationAfterPan, 0.01f));

    // Create one visible hostile and retain another hostile beyond current
    // vision. The order adapter must target only the entity present in the
    // player-scoped view; the full simulation cannot upgrade the hidden click
    // to an attack.
    echoes::sim::Simulation* MutableSimulation =
        const_cast<echoes::sim::Simulation*>(Simulation);
    echoes::sim::EntityId LocalCombat = 0;
    echoes::sim::EntityId VisibleHostile = 0;
    for (const echoes::sim::Entity& Entity : Simulation->Entities())
    {
        const bool bMobileCombat =
            Entity.type == echoes::sim::EntityType::Soldier ||
            Entity.type == echoes::sim::EntityType::HeavyUnit ||
            Entity.type == echoes::sim::EntityType::ScoutUnit;
        if (LocalCombat == 0 && bMobileCombat &&
            Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId)
        {
            LocalCombat = Entity.id;
        }
        else if (Entity.owner != echoes::sim::kNeutralPlayer &&
                 Entity.owner != UEchoesSimulationSubsystem::LocalPlayerId &&
                 VisibleHostile == 0 && bMobileCombat)
        {
            VisibleHostile = Entity.id;
        }
    }
    if (!TestTrue(
            TEXT("Visibility route fixture finds local and hostile combat entities"),
            LocalCombat != 0 && VisibleHostile != 0) ||
        !TestTrue(
            TEXT("Controller can select one exact owned combat entity"),
            SelectExactOwnedEntity(
                *Controller,
                LocalCombat,
                static_cast<int32>(Simulation->Entities().size()) + 2)))
    {
        Controller->Destroy();
        Camera->Destroy();
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    echoes::sim::Entity* VisibleHostileEntity =
        MutableSimulation->MutableEntityForTesting(VisibleHostile);
    const echoes::sim::Entity* LocalCombatEntity =
        Simulation->FindEntity(LocalCombat);
    if (!TestNotNull(TEXT("Visible hostile fixture remains mutable"),
                     VisibleHostileEntity) ||
        !TestNotNull(TEXT("Selected local combat entity remains live"),
                     LocalCombatEntity))
    {
        Controller->Destroy();
        Camera->Destroy();
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    VisibleHostileEntity->position = LocalCombatEntity->position;
    // A normal deterministic tick is the shipping visibility refresh path.
    MutableSimulation->Step();
    const std::optional<echoes::sim::PlayerView> ScopedView =
        Simulation->CreatePlayerView(UEchoesSimulationSubsystem::LocalPlayerId);
    echoes::sim::EntityId HiddenHostile = 0;
    if (ScopedView.has_value())
    {
        for (const echoes::sim::Entity& Entity : Simulation->Entities())
        {
            if (Entity.id == VisibleHostile ||
                Entity.owner == echoes::sim::kNeutralPlayer ||
                Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId ||
                PlayerViewContains(*ScopedView, Entity.id))
            {
                continue;
            }
            const bool bVisibleHostileWithinOrderRadius = std::any_of(
                ScopedView->Entities().begin(),
                ScopedView->Entities().end(),
                [&Entity, &Simulation](const echoes::sim::Entity& Candidate)
                {
                    if (!Simulation->Config().IsHostile(
                            UEchoesSimulationSubsystem::LocalPlayerId,
                            Candidate.owner))
                    {
                        return false;
                    }
                    const int64 DX =
                        static_cast<int64>(Candidate.position.x.Raw()) -
                        Entity.position.x.Raw();
                    const int64 DY =
                        static_cast<int64>(Candidate.position.y.Raw()) -
                        Entity.position.y.Raw();
                    const int64 Radius = echoes::sim::kFixedScale;
                    return DX * DX + DY * DY <= Radius * Radius;
                });
            if (!bVisibleHostileWithinOrderRadius)
            {
                HiddenHostile = Entity.id;
                break;
            }
        }
    }
    TestTrue(
        TEXT("Controlled hostile is present in the player-scoped view"),
        ScopedView.has_value() && PlayerViewContains(*ScopedView, VisibleHostile));
    TestTrue(
        TEXT("Fixture finds a distinct hostile absent from the player-scoped view"),
        HiddenHostile != 0);

    const int32 BeforeVisibleOrder =
        static_cast<int32>(Simulation->PendingCommands().size());
    TestTrue(
        TEXT("Normalized context order accepts a visible hostile position"),
        Controller->HandleFieldHudPointer(
            NormalizeMapPosition(
                VisibleHostileEntity->position, Simulation->Config()),
            true));
    const std::span<const echoes::sim::Command> VisibleCommands =
        Simulation->PendingCommands();
    TestEqual(TEXT("Visible-target click appends one selected-unit command"),
              static_cast<int32>(VisibleCommands.size()),
              BeforeVisibleOrder + 1);
    TestTrue(
        TEXT("Visible-target click issues an exact attack"),
        VisibleCommands.size() > static_cast<size_t>(BeforeVisibleOrder) &&
            VisibleCommands.back().type == echoes::sim::CommandType::Attack &&
            VisibleCommands.back().target == VisibleHostile);

    const echoes::sim::Entity* HiddenHostileEntity =
        Simulation->FindEntity(HiddenHostile);
    if (!TestNotNull(TEXT("Hidden hostile remains in full authority state"),
                     HiddenHostileEntity))
    {
        Controller->Destroy();
        Camera->Destroy();
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    const int32 BeforeHiddenOrder =
        static_cast<int32>(Simulation->PendingCommands().size());
    TestTrue(
        TEXT("Normalized context order accepts a hidden hostile location"),
        Controller->HandleFieldHudPointer(
            NormalizeMapPosition(
                HiddenHostileEntity->position, Simulation->Config()),
            true));
    const std::span<const echoes::sim::Command> HiddenCommands =
        Simulation->PendingCommands();
    TestEqual(TEXT("Hidden-location click appends one selected-unit command"),
              static_cast<int32>(HiddenCommands.size()),
              BeforeHiddenOrder + 1);
    TestTrue(
        TEXT("Hidden hostile cannot be promoted into an attack target"),
        HiddenCommands.size() > static_cast<size_t>(BeforeHiddenOrder) &&
            HiddenCommands.back().type == echoes::sim::CommandType::Move &&
            HiddenCommands.back().target == 0);

    // Technology controls use the same exact semantic action and argument
    // emitted by the view. Invalid and stale tier callbacks append nothing.
    echoes::sim::EntityId LocalBarracks = 0;
    for (const echoes::sim::Entity& Entity : Simulation->Entities())
    {
        if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            Entity.type == echoes::sim::EntityType::Barracks)
        {
            LocalBarracks = Entity.id;
            break;
        }
    }
    if (!TestTrue(TEXT("Funded field exposes an owned production structure"),
                  LocalBarracks != 0) ||
        !TestTrue(
            TEXT("Controller can select the production structure"),
            SelectExactOwnedEntity(
                *Controller,
                LocalBarracks,
                static_cast<int32>(Simulation->Entities().size()) + 2)))
    {
        Controller->Destroy();
        Camera->Destroy();
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    echoes::sim::Entity* ProducerFixture =
        MutableSimulation->MutableEntityForTesting(LocalBarracks);
    if (!TestNotNull(TEXT("Selected production structure remains mutable"),
                     ProducerFixture))
    {
        Controller->Destroy();
        Camera->Destroy();
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    ProducerFixture->productionType = echoes::sim::EntityType::Worker;
    ProducerFixture->activeProductionItemId = 8101;
    ProducerFixture->productionProgress = 10;
    ProducerFixture->productionRequired = 60;
    ProducerFixture->productionInvestedCost = {50, 20};
    ProducerFixture->productionLogisticsCost = 1;
    ProducerFixture->productionQueue.clear();
    echoes::sim::ProductionQueueItem FirstWaiting;
    FirstWaiting.itemId = 8102;
    FirstWaiting.unitType = echoes::sim::EntityType::Soldier;
    FirstWaiting.configuredCost = {85, 20};
    FirstWaiting.requiredTicks = 100;
    FirstWaiting.logisticsCost = 2;
    ProducerFixture->productionQueue.push_back(FirstWaiting);
    echoes::sim::ProductionQueueItem SecondWaiting;
    SecondWaiting.itemId = 8103;
    SecondWaiting.unitType = echoes::sim::EntityType::HeavyUnit;
    SecondWaiting.configuredCost = {130, 25};
    SecondWaiting.requiredTicks = 140;
    SecondWaiting.logisticsCost = 3;
    ProducerFixture->productionQueue.push_back(SecondWaiting);

    View = Controller->BuildFieldHudView();
    TestTrue(TEXT("Selected owned producer emits active and waiting queue controls"),
        View.Production.bVisible &&
        View.Production.ProducerId == LocalBarracks &&
        View.Production.Items.Num() == 3);
    const int32 BeforeDisabledReorder =
        static_cast<int32>(Simulation->PendingCommands().size());
    Controller->HandleFieldHudAction(
        EEchoesFieldHudAction::ProductionMoveUp, 1);
    TestEqual(TEXT("Disabled first-slot move-up emits no command"),
        static_cast<int32>(Simulation->PendingCommands().size()),
        BeforeDisabledReorder);
    Controller->HandleFieldHudAction(
        EEchoesFieldHudAction::ProductionMoveDown, 1);
    TestTrue(TEXT("Enabled waiting reorder reaches the local command bridge"),
        Simulation->PendingCommands().size() ==
                static_cast<size_t>(BeforeDisabledReorder + 1) &&
        Simulation->PendingCommands().back().type ==
            echoes::sim::CommandType::ReorderProduction &&
        Simulation->PendingCommands().back().actor == LocalBarracks &&
        Simulation->PendingCommands().back().target ==
            (1U | (2U << 8U)));
    const int32 BeforeCancellation =
        static_cast<int32>(Simulation->PendingCommands().size());
    Controller->HandleFieldHudAction(
        EEchoesFieldHudAction::ProductionCancel, 0);
    View = Controller->BuildFieldHudView();
    TestEqual(TEXT("Opening active cancellation does not queue a command"),
        static_cast<int32>(Simulation->PendingCommands().size()),
        BeforeCancellation);
    TestTrue(TEXT("Active cancellation pauses locally and quotes both exact refunds"),
        Bridge->IsScenarioPaused() &&
        Controller->IsProductionCancellationConfirmationVisible() &&
        View.Production.Cancellation.bVisible &&
        View.Production.Cancellation.ItemId == 8101 &&
        View.Production.Cancellation.ProgressPercent == 16 &&
        View.Production.Cancellation.RefundPercent == 75 &&
        View.Production.Cancellation.InvestedMatter == 50 &&
        View.Production.Cancellation.InvestedDawn == 20 &&
        View.Production.Cancellation.RefundMatter == 37 &&
        View.Production.Cancellation.RefundDawn == 15);
    TestTrue(TEXT("Cancellation modal exposes focusable Back and Confirm actions"),
        View.Production.Controls.Num() == 2 &&
        View.Production.Controls[0].Action ==
            EEchoesFieldHudAction::ProductionCancelBack &&
        View.Production.Controls[0].bPrimary &&
        View.Production.Controls[1].Action ==
            EEchoesFieldHudAction::ProductionCancelConfirm);
    Controller->HandleFieldHudAction(
        EEchoesFieldHudAction::ProductionCancelBack);
    TestTrue(TEXT("Back keeps the active item and restores the prior running state"),
        !Controller->IsProductionCancellationConfirmationVisible() &&
        !Bridge->IsScenarioPaused() &&
        Simulation->PendingCommands().size() ==
            static_cast<size_t>(BeforeCancellation));
    Controller->HandleFieldHudAction(
        EEchoesFieldHudAction::ProductionCancelConfirm);
    TestEqual(TEXT("A closed confirmation callback emits no command"),
        static_cast<int32>(Simulation->PendingCommands().size()),
        BeforeCancellation);

    Controller->HandleFieldHudAction(
        EEchoesFieldHudAction::ProductionCancel, 0);
    ProducerFixture->activeProductionItemId = 8199;
    Controller->HandleFieldHudAction(
        EEchoesFieldHudAction::ProductionCancelConfirm);
    TestTrue(TEXT("A replacement active item closes stale review without mutation"),
        !Controller->IsProductionCancellationConfirmationVisible() &&
        Simulation->PendingCommands().size() ==
            static_cast<size_t>(BeforeCancellation));
    ProducerFixture->activeProductionItemId = 8101;

    Controller->HandleFieldHudAction(
        EEchoesFieldHudAction::ProductionCancel, 0);
    ProducerFixture->owner = 1;
    Controller->HandleFieldHudAction(
        EEchoesFieldHudAction::ProductionCancelConfirm);
    TestTrue(TEXT("A selection that no longer has local authority cannot cancel"),
        !Controller->IsProductionCancellationConfirmationVisible() &&
        Simulation->PendingCommands().size() ==
            static_cast<size_t>(BeforeCancellation));
    ProducerFixture->owner = UEchoesSimulationSubsystem::LocalPlayerId;

    Controller->HandleFieldHudAction(
        EEchoesFieldHudAction::ProductionCancel, 1);
    View = Controller->BuildFieldHudView();
    TestTrue(TEXT("Waiting review explicitly quotes an uncharged zero refund"),
        View.Production.Cancellation.bVisible &&
        !View.Production.Cancellation.bActive &&
        View.Production.Cancellation.ItemId == 8102 &&
        View.Production.Cancellation.RefundMatter == 0 &&
        View.Production.Cancellation.RefundDawn == 0);
    std::swap(
        ProducerFixture->productionQueue[0],
        ProducerFixture->productionQueue[1]);
    Controller->HandleFieldHudAction(
        EEchoesFieldHudAction::ProductionCancelConfirm);
    TestTrue(TEXT("A shifted waiting slot cannot cancel its replacement"),
        !Controller->IsProductionCancellationConfirmationVisible() &&
        Simulation->PendingCommands().size() ==
            static_cast<size_t>(BeforeCancellation));
    std::swap(
        ProducerFixture->productionQueue[0],
        ProducerFixture->productionQueue[1]);

    ProducerFixture->productionProgress = 30;
    Controller->HandleFieldHudAction(
        EEchoesFieldHudAction::ProductionCancel, 0);
    View = Controller->BuildFieldHudView();
    TestTrue(TEXT("Half-progress active review quotes the exact lower refund band"),
        View.Production.Cancellation.bVisible &&
        View.Production.Cancellation.ProgressPercent == 50 &&
        View.Production.Cancellation.RefundPercent == 50 &&
        View.Production.Cancellation.RefundMatter == 25 &&
        View.Production.Cancellation.RefundDawn == 10);
    Controller->HandleFieldHudAction(
        EEchoesFieldHudAction::ProductionCancelBack);
    ProducerFixture->productionProgress = 10;

    Bridge->SetScenarioPaused(true);
    Controller->HandleFieldHudAction(
        EEchoesFieldHudAction::ProductionCancel, 0);
    Controller->HandleFieldHudAction(
        EEchoesFieldHudAction::ProductionCancelBack);
    TestTrue(TEXT("Cancellation review preserves an already paused scenario"),
        Bridge->IsScenarioPaused());
    Bridge->SetScenarioPaused(false);

    const TOptional<uint64> BeforeCancellationSequence =
        Bridge->GetLastAcceptedLocalCommandSequence();
    Controller->HandleFieldHudAction(
        EEchoesFieldHudAction::ProductionCancel, 0);
    Controller->HandleFieldHudAction(
        EEchoesFieldHudAction::ProductionCancelConfirm);
    const TOptional<uint64> AfterCancellationSequence =
        Bridge->GetLastAcceptedLocalCommandSequence();
    const echoes::sim::Command* CancellationCommand =
        Simulation->PendingCommands().empty()
            ? nullptr
            : &Simulation->PendingCommands().back();
    const uint64 EncodedCancellationItemId = CancellationCommand != nullptr
        ? static_cast<uint64>(static_cast<uint32>(
              CancellationCommand->position.x.Raw())) |
              (static_cast<uint64>(static_cast<uint32>(
                   CancellationCommand->position.y.Raw())) << 32U)
        : 0;
    TestTrue(TEXT("Confirm admits exactly one stable-item cancellation command"),
        Simulation->PendingCommands().size() ==
                static_cast<size_t>(BeforeCancellation + 1) &&
        CancellationCommand != nullptr &&
        CancellationCommand->type ==
            echoes::sim::CommandType::CancelProduction &&
        CancellationCommand->actor == LocalBarracks &&
        CancellationCommand->target == 0 &&
        EncodedCancellationItemId == 8101 &&
        AfterCancellationSequence.IsSet() &&
        (!BeforeCancellationSequence.IsSet() ||
         AfterCancellationSequence.GetValue() ==
             BeforeCancellationSequence.GetValue() + 1) &&
        !Bridge->IsScenarioPaused());
    const int32 BeforeRally =
        static_cast<int32>(Simulation->PendingCommands().size());
    TestTrue(TEXT("Producer minimap context action is consumed"),
        Controller->HandleFieldHudPointer(FVector2D(0.5f, 0.5f), true));
    TestTrue(TEXT("Selected producer context sets rally instead of issuing a unit move"),
        Simulation->PendingCommands().size() ==
                static_cast<size_t>(BeforeRally + 1) &&
        Simulation->PendingCommands().back().type ==
            echoes::sim::CommandType::SetRallyRoute &&
        Simulation->PendingCommands().back().actor == LocalBarracks &&
        Simulation->PendingCommands().back().target == 0);
    Controller->TogglePauseMenu();
    const int32 BeforeStaleQueueAction =
        static_cast<int32>(Simulation->PendingCommands().size());
    Controller->HandleFieldHudAction(
        EEchoesFieldHudAction::ProductionCancel, 1);
    TestEqual(TEXT("Modal shell rejects stale producer queue callbacks"),
        static_cast<int32>(Simulation->PendingCommands().size()),
        BeforeStaleQueueAction);
    Controller->TogglePauseMenu();

    Controller->HandleFieldHudAction(
        EEchoesFieldHudAction::CommandDeck,
        static_cast<int32>(EEchoesCommandDeckAction::ToggleTechnology));
    View = Controller->BuildFieldHudView();
    TestTrue(
        TEXT("Command-card technology action opens the semantic archive"),
        Controller->IsTechnologyPanelVisible() &&
            View.Technology.bVisible && View.Technology.Tiers.Num() == 2 &&
            !View.Production.bVisible);
    TestTrue(
        TEXT("Active production disables research instead of advertising a command the authority will reject"),
        View.Technology.Tiers.Num() == 2 &&
            !View.Technology.Tiers[0].bEnabled &&
            View.Technology.Tiers[0].State.ToString().Contains(TEXT("PRODUCTION IS ACTIVE")));
    const int32 BeforeTechnologyQueueCallback =
        static_cast<int32>(Simulation->PendingCommands().size());
    Controller->HandleFieldHudAction(
        EEchoesFieldHudAction::ProductionCancel, 1);
    TestEqual(TEXT("Technology archive rejects hidden queue callbacks"),
        static_cast<int32>(Simulation->PendingCommands().size()),
        BeforeTechnologyQueueCallback);
    Controller->HandleFieldHudAction(
        EEchoesFieldHudAction::TechnologyResearchTier, 0);
    TestEqual(TEXT("Production-busy technology callback appends no command"),
        static_cast<int32>(Simulation->PendingCommands().size()),
        BeforeTechnologyQueueCallback);

    // The production controls above intentionally use a direct queue-state
    // fixture. Clear that fixture before testing the independent ready-research
    // route so the semantic control and bridge validate the same producer.
    ProducerFixture->productionType = echoes::sim::EntityType::Worker;
    ProducerFixture->activeProductionItemId = 0;
    ProducerFixture->productionProgress = 0;
    ProducerFixture->productionRequired = 0;
    ProducerFixture->productionInvestedCost = {};
    ProducerFixture->productionLogisticsCost = 0;
    ProducerFixture->productionQueue.clear();
    View = Controller->BuildFieldHudView();
    TestTrue(TEXT("Idle selected producer exposes the enabled first technology"),
        View.Technology.Tiers.Num() == 2 &&
            View.Technology.Tiers[0].bEnabled);
    Controller->HandleFieldHudAction(EEchoesFieldHudAction::TechnologyNext);
    TestEqual(TEXT("Semantic next-tier action updates exact focus"),
              Controller->GetTechnologyPanelFocusedTier(), 1);
    const int32 BeforeInvalidTier =
        static_cast<int32>(Simulation->PendingCommands().size());
    Controller->HandleFieldHudAction(
        EEchoesFieldHudAction::TechnologyResearchTier, 99);
    TestEqual(TEXT("Invalid technology argument appends no command"),
              static_cast<int32>(Simulation->PendingCommands().size()),
              BeforeInvalidTier);
    Controller->HandleFieldHudAction(
        EEchoesFieldHudAction::TechnologyResearchTier, 0);
    const std::span<const echoes::sim::Command> ResearchCommands =
        Simulation->PendingCommands();
    TestTrue(
        TEXT("Exact enabled tier dispatches research through authority"),
        ResearchCommands.size() ==
                static_cast<size_t>(BeforeInvalidTier + 1) &&
            ResearchCommands.back().type ==
                echoes::sim::CommandType::Research &&
            ResearchCommands.back().actor == LocalBarracks &&
            ResearchCommands.back().researchType ==
                echoes::sim::ResearchType::MeridianPrismaticTargeting);
    Controller->HandleFieldHudAction(
        EEchoesFieldHudAction::ToggleTechnology);
    TestFalse(TEXT("Semantic toggle closes the technology archive"),
              Controller->IsTechnologyPanelVisible());
    const int32 BeforeStaleTier =
        static_cast<int32>(Simulation->PendingCommands().size());
    Controller->HandleFieldHudAction(
        EEchoesFieldHudAction::TechnologyResearchTier, 1);
    Controller->HandleFieldHudAction(
        EEchoesFieldHudAction::ProductionCancel, 0);
    Controller->HandleFieldHudAction(
        EEchoesFieldHudAction::ProductionCancelConfirm);
    TestEqual(TEXT("Hidden technology callback appends no command"),
              static_cast<int32>(Simulation->PendingCommands().size()),
              BeforeStaleTier);

    // Detached replay presentation may pan its own camera but must never emit
    // a command into the retained live match.
    const echoes::sim::ReplayRecord Replay = MakeFieldHudReplay();
    const echoes::sim::net::CompatibilityManifest Compatibility =
        echoes::network::BuildCompatibilityManifest(Simulation);
    FEchoesReplayMetadata Metadata;
    Metadata.ReplayId = FString::Printf(
        TEXT("field-hud-%s"),
        *FGuid::NewGuid().ToString(EGuidFormats::Digits));
    Metadata.MapId = TEXT("glass-scar");
    Metadata.OperationId = TEXT("skirmish");
    Metadata.BuildIdentity = FieldHudRoutesDigestHex(Compatibility.buildIdSha256);
    Metadata.RulesIdentity = FieldHudRoutesDigestHex(Compatibility.rulesPackSha256);
    Metadata.RecordedUtc = FDateTime(2026, 9, 5, 12, 0, 0);
    Metadata.OperationType = EEchoesReplayOperationType::Skirmish;
    Metadata.bOperationCompleted = true;
    FEchoesReplayEnvelope Envelope;
    FString ReplayError;
    FString ReplayPath;
    if (!TestTrue(
            TEXT("Field-HUD replay envelope finalizes"),
            FEchoesMatchReplayStore::FinalizeEnvelope(
                Metadata, Replay, Envelope, ReplayError)) ||
        !TestTrue(
            TEXT("Field-HUD replay envelope saves in isolated storage"),
            FEchoesMatchReplayStore::SaveAtomic(
                TestSaveEnvironment.Directory,
                Envelope,
                ReplayPath,
                ReplayError)) ||
        !TestTrue(
            TEXT("Field-HUD route opens detached replay playback"),
            Bridge->BeginReplay(ReplayPath, ReplayError)))
    {
        AddError(ReplayError);
        Controller->Destroy();
        Camera->Destroy();
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    const uint64 LiveTickBeforeReplayInput = Simulation->CurrentTick();
    const uint64 LiveChecksumBeforeReplayInput = Simulation->StateChecksum();
    const int32 LiveCommandsBeforeReplayInput =
        static_cast<int32>(Simulation->PendingCommands().size());
    View = Controller->BuildFieldHudView();
    TestTrue(
        TEXT("Replay field view identifies detached read-only authority"),
        View.Surface == EEchoesFieldHudSurface::Replay &&
            (View.Authority == EEchoesFieldHudAuthority::ReplayPlayerView ||
             View.Authority == EEchoesFieldHudAuthority::ReplayObserver) &&
            !View.Commands.bVisible);
    Controller->HandleFieldHudAction(
        EEchoesFieldHudAction::CommandDeck,
        static_cast<int32>(EEchoesCommandDeckAction::Hold));
    Controller->HandleFieldHudAction(
        EEchoesFieldHudAction::TechnologyResearchTier, 1);
    Controller->HandleFieldHudAction(
        EEchoesFieldHudAction::ProductionCancel, 0);
    Controller->HandleFieldHudAction(
        EEchoesFieldHudAction::ProductionCancelConfirm);
    TestTrue(
        TEXT("Replay minimap right click is consumed as read-only"),
        Controller->HandleFieldHudPointer(FVector2D(0.5f, 0.5f), true));
    TestTrue(
        TEXT("Replay actions preserve the retained live authority"),
        Simulation->CurrentTick() == LiveTickBeforeReplayInput &&
            Simulation->StateChecksum() == LiveChecksumBeforeReplayInput &&
            static_cast<int32>(Simulation->PendingCommands().size()) ==
                LiveCommandsBeforeReplayInput &&
            !Controller->IsProductionCancellationConfirmationVisible());
    Bridge->EndReplay();
    TestTrue(
        TEXT("Leaving replay returns to the same live field state"),
        Bridge->GetSimulation() == Simulation &&
            Simulation->CurrentTick() == LiveTickBeforeReplayInput &&
            Simulation->StateChecksum() == LiveChecksumBeforeReplayInput);

    ProducerFixture->activeProductionItemId = 8201;
    ProducerFixture->productionType = echoes::sim::EntityType::Worker;
    ProducerFixture->productionProgress = 5;
    ProducerFixture->productionRequired = 60;
    ProducerFixture->productionInvestedCost = {50, 20};
    TestTrue(TEXT("Producer is selected again for authority reset review"),
        SelectExactOwnedEntity(
            *Controller,
            LocalBarracks,
            static_cast<int32>(Simulation->Entities().size()) + 2));
    Controller->HandleFieldHudAction(
        EEchoesFieldHudAction::ProductionCancel, 0);
    const uint64 ReviewAuthority = Bridge->GetScenarioAuthorityGeneration();
    TestTrue(TEXT("Authority reset fixture opens a stable cancellation review"),
        Controller->IsProductionCancellationConfirmationVisible());
    Bridge->StopPrototypeScenario();
    TestTrue(TEXT("Replacement scenario starts for stale callback isolation"),
        Bridge->StartPrototypeScenario());
    const echoes::sim::Simulation* ReplacementSimulation = Bridge->GetSimulation();
    const int32 ReplacementCommandCount = ReplacementSimulation != nullptr
        ? static_cast<int32>(ReplacementSimulation->PendingCommands().size())
        : -1;
    Controller->HandleFieldHudAction(
        EEchoesFieldHudAction::ProductionCancelConfirm);
    TestTrue(TEXT("Authority replacement closes review without touching the new scenario"),
        ReplacementSimulation != nullptr &&
        Bridge->GetScenarioAuthorityGeneration() != ReviewAuthority &&
        !Controller->IsProductionCancellationConfirmationVisible() &&
        static_cast<int32>(ReplacementSimulation->PendingCommands().size()) ==
            ReplacementCommandCount);

    // Campaign-map actions are accepted only while that semantic surface is
    // current; a disabled locked-sector deploy cannot escape the map.
    const int32 HiddenCampaignIndex =
        Controller->GetSelectedCampaignMapNodeIndex();
    Controller->HandleFieldHudAction(
        EEchoesFieldHudAction::CampaignSelectNode, 4);
    TestEqual(TEXT("Hidden campaign-map selection is rejected"),
              Controller->GetSelectedCampaignMapNodeIndex(),
              HiddenCampaignIndex);
    Controller->PresentTitleScreen();
    Controller->OpenCampaignOperationsMap();
    View = Controller->BuildFieldHudView();
    TestTrue(
        TEXT("Operations map becomes the semantic field surface"),
        View.Surface == EEchoesFieldHudSurface::CampaignOperations &&
            View.Campaign.bVisible && View.Campaign.Layout.Nodes.Num() == 15);
    Controller->HandleFieldHudAction(
        EEchoesFieldHudAction::CampaignSelectNode, 1);
    TestEqual(TEXT("Campaign node action selects its exact argument"),
              Controller->GetSelectedCampaignMapNodeIndex(), 1);
    Controller->HandleFieldHudAction(EEchoesFieldHudAction::CampaignDeploy);
    TestTrue(TEXT("Disabled locked-sector deploy remains on the map"),
             Controller->IsCampaignOperationsMapVisible());
    Controller->HandleFieldHudAction(
        EEchoesFieldHudAction::CampaignSelectNode, 0);
    Controller->HandleFieldHudAction(EEchoesFieldHudAction::CampaignDeploy);
    TestTrue(
        TEXT("Enabled current-sector deploy reaches the mission briefing"),
        !Controller->IsCampaignOperationsMapVisible() &&
            Controller->IsMissionBriefingVisible() &&
            Bridge->GetOperationMode() ==
                EEchoesOperationMode::CampaignPrologue);

    Bridge->EndReplay();
    Controller->Destroy();
    Camera->Destroy();
    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors() && !WorldWrapper.HasFailed();
}

#endif
