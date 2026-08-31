#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "EchoesEntityView.h"
#include "EchoesPlayerController.h"
#include "EchoesSimCore/Simulation.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/StaticMesh.h"
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

    FEchoesScopedTestSaveEnvironment TestSaveEnvironment(*this);
    if (!TestSaveEnvironment.IsReady())
    {
        return false;
    }

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

    const echoes::sim::Simulation* DefaultSimulation =
        Bridge->GetSimulation();
    const echoes::sim::Tick DefaultTick =
        DefaultSimulation != nullptr ? DefaultSimulation->CurrentTick() : 0;
    const uint64 DefaultChecksum =
        DefaultSimulation != nullptr ? DefaultSimulation->StateChecksum() : 0;
    Controller->PresentTitleScreen();
    FEchoesSkirmishSetup KharuunSetup =
        Bridge->GetActiveSkirmishSetup();
    KharuunSetup.LocalFaction = echoes::sim::Faction::KharuunAssemblies;
    KharuunSetup.OpponentFaction = echoes::sim::Faction::MeridianCompact;
    FString SetupFeedback;
    TestTrue(
        TEXT("A complete Kharuun deployment can be staged"),
        Controller->SetPendingSkirmishSetup(
            KharuunSetup, SetupFeedback));
    TestTrue(TEXT("Faction choice remains on the title screen"),
             Controller->IsTitleScreenVisible());
    TestTrue(TEXT("Faction setup stays paused before deployment"),
             Bridge->IsScenarioPaused());
    TestTrue(
        TEXT("Staging does not change active Meridian authority"),
        Bridge->GetLocalFaction() ==
            echoes::sim::Faction::MeridianCompact);
    TestTrue(
        TEXT("Staging does not rebuild or advance the live simulation"),
        Bridge->GetSimulation() == DefaultSimulation &&
            DefaultSimulation != nullptr &&
            DefaultSimulation->CurrentTick() == DefaultTick &&
            DefaultSimulation->StateChecksum() == DefaultChecksum);
    TestEqual(TEXT("Controller reports the selected force"),
              Controller->GetLocalFactionLabel(),
              FString(TEXT("KHARUUN ASSEMBLIES")));
    Controller->ConfirmPrimaryAction();
    TestTrue(TEXT("The first Enter reviews the complete Kharuun setup"),
             Controller->IsSkirmishDeploymentSummaryVisible());
    Controller->ConfirmPrimaryAction();
    TestFalse(TEXT("The second Enter deploys the Kharuun setup"),
              Controller->IsMissionBriefingVisible());
    TestTrue(
        TEXT("Confirmed deployment assigns Kharuun to player zero"),
        Bridge->GetLocalFaction() ==
            echoes::sim::Faction::KharuunAssemblies);
    TestTrue(
        TEXT("The explicit opponent becomes Meridian"),
        Bridge->GetOpponentFaction() ==
            echoes::sim::Faction::MeridianCompact);

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

    AEchoesEntityView* FutureWellPreview =
        World->SpawnActor<AEchoesEntityView>();
    TestNotNull(TEXT("A presentation-only Future Well view can be created"),
                FutureWellPreview);
    if (FutureWellPreview != nullptr)
    {
        echoes::sim::Entity PreviewState{};
        PreviewState.id = 900001;
        PreviewState.owner = echoes::sim::kNeutralPlayer;
        PreviewState.type = echoes::sim::EntityType::FutureWell;
        PreviewState.position = echoes::sim::Vec2::FromTiles(32, 32);
        PreviewState.hitPoints = 1;
        PreviewState.maxHitPoints = 1;
        const echoes::sim::FutureWellChoice Choices[] = {
            echoes::sim::FutureWellChoice::Dormant,
            echoes::sim::FutureWellChoice::Harvest,
            echoes::sim::FutureWellChoice::Preserve,
            echoes::sim::FutureWellChoice::Reshape};
        for (const echoes::sim::FutureWellChoice Choice : Choices)
        {
            PreviewState.wellChoice = Choice;
            FutureWellPreview->ApplyAuthoritativeState(PreviewState, true);
            TestTrue(
                TEXT("Each Future Well state uses authored landmark geometry"),
                FutureWellPreview->IsUsingAuthoredFutureWellMesh());
            TestTrue(
                TEXT("Each Future Well state presents its orbit and fractured core"),
                FutureWellPreview->IsFutureWellPresentationVisible());
            TestTrue(
                TEXT("Each Future Well state retains its requested visual variant"),
                FutureWellPreview->GetFutureWellVisualChoice() == Choice);
        }
        FutureWellPreview->Destroy();
    }

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

    Controller->PresentTitleScreen();
    FEchoesSkirmishSetup ChoirSetup = Bridge->GetActiveSkirmishSetup();
    ChoirSetup.LocalFaction = echoes::sim::Faction::HollowChoir;
    ChoirSetup.OpponentFaction = echoes::sim::Faction::MeridianCompact;
    SetupFeedback.Reset();
    TestTrue(
        TEXT("A complete Hollow Choir deployment can be staged"),
        Controller->SetPendingSkirmishSetup(ChoirSetup, SetupFeedback));
    Controller->ConfirmPrimaryAction();
    Controller->ConfirmPrimaryAction();
    TestTrue(
        TEXT("Confirmed setup exposes Hollow Choir as a playable faction"),
        Bridge->GetLocalFaction() == echoes::sim::Faction::HollowChoir);
    TestTrue(
        TEXT("The declared Choir skirmish matchup uses Meridian opposition"),
        Bridge->GetOpponentFaction() ==
            echoes::sim::Faction::MeridianCompact);
    TestEqual(
        TEXT("Controller reports the Hollow Choir explicitly"),
        Controller->GetLocalFactionLabel(),
        FString(TEXT("HOLLOW CHOIR")));

    const echoes::sim::Simulation* ChoirSimulation = Bridge->GetSimulation();
    uint32 ChoirIdentityUnit = 0;
    uint32 ChoirWorker = 0;
    bool bAllLocalEntitiesAreChoir = true;
    bool bAllChoirRosterRolesPresent = true;
    bool bAllChoirViewsUseAuthoredMeshes = true;
    TSet<echoes::sim::EntityType> ChoirRosterTypes;
    if (ChoirSimulation != nullptr)
    {
        for (const echoes::sim::Entity& Entity : ChoirSimulation->Entities())
        {
            if (Entity.owner != UEchoesSimulationSubsystem::LocalPlayerId)
            {
                continue;
            }
            bAllLocalEntitiesAreChoir &=
                Entity.faction == echoes::sim::Faction::HollowChoir;
            ChoirRosterTypes.Add(Entity.type);
            const AEchoesEntityView* ChoirView =
                Bridge->FindEntityView(Entity.id);
            bAllChoirViewsUseAuthoredMeshes &=
                ChoirView != nullptr &&
                ChoirView->IsUsingAuthoredRosterMesh();
            ChoirIdentityUnit =
                Entity.type == echoes::sim::EntityType::Soldier &&
                        ChoirIdentityUnit == 0
                    ? Entity.id
                    : ChoirIdentityUnit;
            ChoirWorker =
                Entity.type == echoes::sim::EntityType::Worker &&
                        ChoirWorker == 0
                    ? Entity.id
                    : ChoirWorker;
        }
        const echoes::sim::EntityType RequiredTypes[] = {
            echoes::sim::EntityType::Worker,
            echoes::sim::EntityType::Soldier,
            echoes::sim::EntityType::HeavyUnit,
            echoes::sim::EntityType::ScoutUnit,
            echoes::sim::EntityType::CommandCore,
            echoes::sim::EntityType::Dropoff,
            echoes::sim::EntityType::Barracks,
            echoes::sim::EntityType::UtilityStructure};
        for (const echoes::sim::EntityType Type : RequiredTypes)
        {
            bAllChoirRosterRolesPresent &= ChoirRosterTypes.Contains(Type);
        }
    }
    TestTrue(TEXT("Every locally owned roster entity is Hollow Choir"),
             bAllLocalEntitiesAreChoir);
    TestTrue(TEXT("The direct Choir roster contains all eight gameplay roles"),
             bAllChoirRosterRolesPresent);
    TestTrue(TEXT("Every direct Choir roster role uses authored Choir geometry"),
             bAllChoirViewsUseAuthoredMeshes);
    TestNotNull(
        TEXT("Held Alternatives is available to direct Choir play"),
        ChoirSimulation != nullptr
            ? ChoirSimulation->ResearchDefinition(
                  echoes::sim::ResearchType::ChoirHeldAlternatives)
            : nullptr);
    TestNotNull(
        TEXT("Shared Resolution is available to direct Choir play"),
        ChoirSimulation != nullptr
            ? ChoirSimulation->ResearchDefinition(
                  echoes::sim::ResearchType::ChoirSharedResolution)
            : nullptr);

    Feedback.Reset();
    TestFalse(
        TEXT("A Threadkeeper cannot reconcile combat identity"),
        ChoirWorker != 0 && Bridge->IssueChoirReconciliation(
            ChoirWorker,
            echoes::sim::ChoirIdentityState::Possible,
            Feedback));
    Feedback.Reset();
    TestTrue(
        TEXT("A direct Choir player can reconcile a voice toward Possible"),
        ChoirIdentityUnit != 0 && Bridge->IssueChoirReconciliation(
            ChoirIdentityUnit,
            echoes::sim::ChoirIdentityState::Possible,
            Feedback));
    Bridge->SetScenarioPaused(false);
    Bridge->Tick(0.10f);
    const echoes::sim::Entity* ReconcilingVoice =
        Bridge->FindEntity(ChoirIdentityUnit);
    if (TestNotNull(
            TEXT("The reconciling Choir voice remains authoritative"),
            ReconcilingVoice))
    {
        TestTrue(
            TEXT("The queued player command exposes the dual-resolution interval"),
            ReconcilingVoice->choirIdentityState ==
                echoes::sim::ChoirIdentityState::DualResolvePossible);
        const AEchoesEntityView* ReconcilingView =
            Bridge->FindEntityView(ChoirIdentityUnit);
        TestTrue(
            TEXT("The declared Choir identity remains visible in presentation"),
            ReconcilingView != nullptr &&
                ReconcilingView->IsChoirIdentityStateVisible() &&
                ReconcilingView->GetChoirIdentityState() ==
                    echoes::sim::ChoirIdentityState::DualResolvePossible);
    }

    Controller->PresentTitleScreen();
    SetupFeedback.Reset();
    TestTrue(
        TEXT("Kharuun can be staged again without mutating the Choir field"),
        Controller->SetPendingSkirmishSetup(
            KharuunSetup, SetupFeedback) &&
            Bridge->GetLocalFaction() ==
                echoes::sim::Faction::HollowChoir);
    Controller->ConfirmPrimaryAction();
    TestTrue(TEXT("Selected Kharuun force reaches the operations brief"),
             Controller->IsMissionBriefingVisible());
    TestTrue(
        TEXT("Faction selection remains pending through review"),
        Bridge->GetLocalFaction() ==
            echoes::sim::Faction::HollowChoir);

    Controller->ConfirmPrimaryAction();
    TestFalse(TEXT("Second Enter deploys from the operations brief"),
              Controller->IsMissionBriefingVisible());
    TestTrue(
        TEXT("The reviewed setup returns active authority to Kharuun"),
        Bridge->GetLocalFaction() ==
            echoes::sim::Faction::KharuunAssemblies);
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
    Controller->SelectCombatForce();
    TestEqual(TEXT("F7 selects the complete visible local combat force"),
              Controller->GetSelectedEntityIds().Num(), 5);
    for (const uint32 EntityId : Controller->GetSelectedEntityIds())
    {
        const echoes::sim::Entity* Entity = Bridge->FindEntity(EntityId);
        const AEchoesEntityView* EntityView = Bridge->FindEntityView(EntityId);
        TestTrue(
            FString::Printf(TEXT("Combat-force entity %u is an owned live combat presentation"), EntityId),
            Entity != nullptr &&
                Entity->owner == UEchoesSimulationSubsystem::LocalPlayerId &&
                Entity->hitPoints > 0 &&
                (Entity->type == echoes::sim::EntityType::Soldier ||
                 Entity->type == echoes::sim::EntityType::HeavyUnit ||
                 Entity->type == echoes::sim::EntityType::ScoutUnit) &&
                EntityView != nullptr);
        TestTrue(
            FString::Printf(TEXT("Combat-force entity %u exposes authored faction geometry or its fallback silhouette accent"), EntityId),
            EntityView != nullptr &&
                (EntityView->IsUsingAuthoredRosterMesh() ||
                 EntityView->IsSilhouetteAccentVisible()));
    }

    const TCHAR* AuthoredRosterMeshes[] = {
        TEXT("/Game/Art/Generated/Meridian/Units/SM_Meridian_Surveyor.SM_Meridian_Surveyor"),
        TEXT("/Game/Art/Generated/Meridian/Units/SM_Meridian_Lancer.SM_Meridian_Lancer"),
        TEXT("/Game/Art/Generated/Meridian/Units/SM_Meridian_Bulwark.SM_Meridian_Bulwark"),
        TEXT("/Game/Art/Generated/Meridian/Units/SM_Meridian_RelaySkiff.SM_Meridian_RelaySkiff"),
        TEXT("/Game/Art/Generated/Meridian/Structures/SM_Meridian_Anchor.SM_Meridian_Anchor"),
        TEXT("/Game/Art/Generated/Meridian/Structures/SM_Meridian_PowerLink.SM_Meridian_PowerLink"),
        TEXT("/Game/Art/Generated/Meridian/Structures/SM_Meridian_ArrayFoundry.SM_Meridian_ArrayFoundry"),
        TEXT("/Game/Art/Generated/Meridian/Structures/SM_Meridian_AegisPost.SM_Meridian_AegisPost"),
        TEXT("/Game/Art/Generated/Kharuun/Units/SM_Kharuun_Tender.SM_Kharuun_Tender"),
        TEXT("/Game/Art/Generated/Kharuun/Units/SM_Kharuun_Riftstalker.SM_Kharuun_Riftstalker"),
        TEXT("/Game/Art/Generated/Kharuun/Units/SM_Kharuun_Cairnback.SM_Kharuun_Cairnback"),
        TEXT("/Game/Art/Generated/Kharuun/Units/SM_Kharuun_Resonant.SM_Kharuun_Resonant"),
        TEXT("/Game/Art/Generated/Kharuun/Structures/SM_Kharuun_MemoryHearth.SM_Kharuun_MemoryHearth"),
        TEXT("/Game/Art/Generated/Kharuun/Structures/SM_Kharuun_Waystone.SM_Kharuun_Waystone"),
        TEXT("/Game/Art/Generated/Kharuun/Structures/SM_Kharuun_GrowthBasin.SM_Kharuun_GrowthBasin"),
        TEXT("/Game/Art/Generated/Kharuun/Structures/SM_Kharuun_ListeningSpine.SM_Kharuun_ListeningSpine"),
        TEXT("/Game/Art/Generated/Choir/Units/SM_Choir_Threadkeeper.SM_Choir_Threadkeeper"),
        TEXT("/Game/Art/Generated/Choir/Units/SM_Choir_Intervalist.SM_Choir_Intervalist"),
        TEXT("/Game/Art/Generated/Choir/Units/SM_Choir_LacunaWarden.SM_Choir_LacunaWarden"),
        TEXT("/Game/Art/Generated/Choir/Units/SM_Choir_Afterimage.SM_Choir_Afterimage"),
        TEXT("/Game/Art/Generated/Choir/Structures/SM_Choir_Concordance.SM_Choir_Concordance"),
        TEXT("/Game/Art/Generated/Choir/Structures/SM_Choir_IntervalLoom.SM_Choir_IntervalLoom"),
        TEXT("/Game/Art/Generated/Choir/Structures/SM_Choir_ChorusLoom.SM_Choir_ChorusLoom"),
        TEXT("/Game/Art/Generated/Choir/Structures/SM_Choir_PhaseAnchor.SM_Choir_PhaseAnchor")};
    for (const TCHAR* MeshPath : AuthoredRosterMeshes)
    {
        UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, MeshPath);
        TestNotNull(
            FString::Printf(TEXT("Authored roster mesh loads: %s"), MeshPath),
            Mesh);
        if (Mesh != nullptr)
        {
            TestTrue(
                FString::Printf(TEXT("Authored roster mesh has two LODs: %s"), MeshPath),
                Mesh->GetNumLODs() >= 2);
            TestTrue(
                FString::Printf(TEXT("Authored roster mesh has four material zones: %s"), MeshPath),
                Mesh->GetStaticMaterials().Num() >= 4);
        }
    }
    const TCHAR* AuthoredFutureWellMeshes[] = {
        TEXT("/Game/Art/Generated/World/Landmarks/SM_World_FutureWellBase.SM_World_FutureWellBase"),
        TEXT("/Game/Art/Generated/World/Landmarks/SM_World_FutureWellOrbit.SM_World_FutureWellOrbit"),
        TEXT("/Game/Art/Generated/World/Landmarks/SM_World_FutureWellCore.SM_World_FutureWellCore"),
        TEXT("/Game/Art/Generated/World/Landmarks/SM_World_FutureWellGlyph.SM_World_FutureWellGlyph")};
    for (const TCHAR* MeshPath : AuthoredFutureWellMeshes)
    {
        UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, MeshPath);
        TestNotNull(
            FString::Printf(TEXT("Authored Future Well mesh loads: %s"), MeshPath),
            Mesh);
        if (Mesh != nullptr)
        {
            TestTrue(
                FString::Printf(TEXT("Authored Future Well mesh has two LODs: %s"), MeshPath),
                Mesh->GetNumLODs() >= 2);
            TestTrue(
                FString::Printf(TEXT("Authored Future Well mesh has four material zones: %s"), MeshPath),
                Mesh->GetStaticMaterials().Num() >= 4);
        }
    }
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
        TEXT("Shift-F3 Choir Manifest mapping is present"),
        InputMappings.ContainsByPredicate(
            [](const FString& Mapping)
            {
                return Mapping.Contains(
                           TEXT("ActionName=\"ReconcileChoirManifest\"")) &&
                       Mapping.Contains(TEXT("bShift=True")) &&
                       Mapping.Contains(TEXT("Key=F3"));
            }));
    TestTrue(
        TEXT("Shift-F4 Choir Possible mapping is present"),
        InputMappings.ContainsByPredicate(
            [](const FString& Mapping)
            {
                return Mapping.Contains(
                           TEXT("ActionName=\"ReconcileChoirPossible\"")) &&
                       Mapping.Contains(TEXT("bShift=True")) &&
                       Mapping.Contains(TEXT("Key=F4"));
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
    TestTrue(
        TEXT("End selected-view target-snap mapping is present"),
        InputMappings.ContainsByPredicate(
            [](const FString& Mapping)
            {
                return Mapping.Contains(
                           TEXT("ActionName=\"SnapKeyboardTargetToSelection\"")) &&
                       Mapping.Contains(TEXT("Key=End"));
            }));
    TestTrue(
        TEXT("F7 combat-force selection mapping is present"),
        InputMappings.ContainsByPredicate(
            [](const FString& Mapping)
            {
                return Mapping.Contains(
                           TEXT("ActionName=\"SelectCombatForce\"")) &&
                       Mapping.Contains(TEXT("Key=F7"));
            }));

    Controller->Destroy();
    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors() && !WorldWrapper.HasFailed();
}

#endif
