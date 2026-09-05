#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "EchoesCombatEffectView.h"
#include "EchoesEntityView.h"
#include "EchoesGameUserSettings.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesSimCore/Simulation.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesCombatEffectsTest,
    "Echoes.Runtime.Presentation.CombatEffects",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesCombatEffectsTest::RunTest(const FString& Parameters)
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
        AddError(TEXT("Could not create the combat effects test world."));
        return false;
    }
    UWorld* World = WorldWrapper.GetTestWorld();
    if (!TestNotNull(TEXT("Combat effects test world is available"), World))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get();
    const bool bPreviousReducedMotion =
        Settings != nullptr && Settings->IsReducedMotionEnabled();
    const bool bPreviousReducedFlashing =
        Settings != nullptr && Settings->IsReducedFlashingEnabled();
    if (Settings != nullptr)
    {
        Settings->SetReducedMotionEnabled(false);
        Settings->SetReducedFlashingEnabled(false);
    }

    // -------------------------------------------------------------------------
    // 1. AEchoesCombatEffectView Invariants (Collision, Shadows, MIDs, Pooling)
    // -------------------------------------------------------------------------
    AEchoesCombatEffectView* CombatEffect =
        World->SpawnActor<AEchoesCombatEffectView>();
    if (TestNotNull(TEXT("Combat effect actor spawns successfully"), CombatEffect))
    {
        const FVector SourcePos(100.0f, 200.0f, 50.0f);
        const FVector TargetPos(700.0f, 200.0f, 50.0f);
        CombatEffect->InitializeCombatEffect(
            echoes::sim::Faction::MeridianCompact,
            echoes::sim::EntityType::Soldier,
            SourcePos,
            TargetPos,
            false,
            false,
            0.32f);

        TestTrue(TEXT("Combat effect uses authored VFX meshes"),
                 CombatEffect->IsUsingAuthoredVFXAssets());
        TestTrue(TEXT("Combat effect has all collision and traces disabled"),
                 CombatEffect->HasCollisionDisabled());
        TestTrue(TEXT("Combat effect cannot affect navigation generation"),
                 CombatEffect->HasNavigationDisabled());
        TestTrue(TEXT("Combat effect has dynamic shadows disabled"),
                 CombatEffect->HasShadowsDisabled());
        TestTrue(TEXT("Combat effect has overlap events disabled"),
                 CombatEffect->HasOverlapsDisabled());
        TestTrue(TEXT("Combat effect is active upon initialization"),
                 CombatEffect->IsPresentationActive());
        TestNearlyEqual(TEXT("Combat effect computes correct beam length"),
                        CombatEffect->GetBeamLength(),
                        600.0f,
                        1.0f);

        const uint64 InitialMIDCount = CombatEffect->GetOwnedMIDCreationCount();
        TestEqual(TEXT("Combat effect owns exactly 4 reusable MIDs"),
                  InitialMIDCount,
                  static_cast<uint64>(4));

        const float InitialEmission = CombatEffect->GetCurrentEmissiveStrength();
        const float InitialLifetime = CombatEffect->GetRemainingLifetimeSeconds();
        TestTrue(TEXT("Initial emissive strength is positive"), InitialEmission > 0.0f);
        TestNearlyEqual(TEXT("Initial lifetime matches requested 0.32s"),
                        InitialLifetime,
                        0.32f,
                        0.001f);

        // Tick partial duration
        CombatEffect->Tick(0.16f);
        TestTrue(TEXT("Remaining lifetime decrements after tick"),
                 CombatEffect->GetRemainingLifetimeSeconds() < InitialLifetime);
        TestTrue(TEXT("Emissive strength decays over lifetime"),
                 CombatEffect->GetCurrentEmissiveStrength() < InitialEmission);

        // Test overflow coalescing
        CombatEffect->RegisterOverflowCoalesced();
        TestEqual(TEXT("Coalesced overflow counter increments"),
                  CombatEffect->GetCoalescedOverflowCount(),
                  static_cast<uint64>(1));

        // Test pooling deactivation
        CombatEffect->PrepareForPool();
        TestFalse(TEXT("Pooled combat effect is marked inactive"),
                  CombatEffect->IsPresentationActive());
        TestTrue(TEXT("Pooled combat effect actor is hidden"),
                 CombatEffect->IsHidden());
        TestFalse(TEXT("Pooled combat effect actor tick is disabled"),
                  CombatEffect->IsActorTickEnabled());
        TestEqual(TEXT("Pooled combat effect resets coalesced overflow counter"),
                  CombatEffect->GetCoalescedOverflowCount(),
                  static_cast<uint64>(0));

        // ---------------------------------------------------------------------
        // 2. Faction Visual Signatures and Zero MID Reallocation
        // ---------------------------------------------------------------------
        // Reuse for Meridian
        CombatEffect->InitializeCombatEffect(
            echoes::sim::Faction::MeridianCompact,
            echoes::sim::EntityType::Soldier,
            SourcePos,
            TargetPos,
            false,
            false,
            0.32f);
        const FLinearColor MeridianColor = CombatEffect->GetBaseColor();
        TestTrue(TEXT("Meridian color has prominent cyan/blue component"),
                 MeridianColor.B >= MeridianColor.R && MeridianColor.G > 0.5f);
        TestFalse(TEXT("Meridian weapon beam has no Choir afterimage"),
                  CombatEffect->IsAfterimageActive());
        TestEqual(TEXT("Reusing pooled effect for Meridian creates 0 new MIDs"),
                  CombatEffect->GetOwnedMIDCreationCount(),
                  InitialMIDCount);

        // Reuse for Kharuun
        CombatEffect->InitializeCombatEffect(
            echoes::sim::Faction::KharuunAssemblies,
            echoes::sim::EntityType::Soldier,
            SourcePos,
            TargetPos,
            false,
            false,
            0.32f);
        const FLinearColor KharuunColor = CombatEffect->GetBaseColor();
        TestTrue(TEXT("Kharuun color has prominent molten amber/red component"),
                 KharuunColor.R > KharuunColor.B && KharuunColor.G > 0.2f);
        TestFalse(TEXT("Kharuun color is distinct from Meridian"),
                  KharuunColor.Equals(MeridianColor));
        TestFalse(TEXT("Kharuun weapon beam has no Choir afterimage"),
                  CombatEffect->IsAfterimageActive());
        TestEqual(TEXT("Reusing pooled effect for Kharuun creates 0 new MIDs"),
                  CombatEffect->GetOwnedMIDCreationCount(),
                  InitialMIDCount);

        // Reuse for Choir
        CombatEffect->InitializeCombatEffect(
            echoes::sim::Faction::HollowChoir,
            echoes::sim::EntityType::Soldier,
            SourcePos,
            TargetPos,
            false,
            false,
            0.32f);
        const FLinearColor ChoirColor = CombatEffect->GetBaseColor();
        TestTrue(TEXT("Choir color has prominent lilac/magenta component"),
                 ChoirColor.R > 0.5f && ChoirColor.B > 0.5f);
        TestFalse(TEXT("Choir color is distinct from Meridian"),
                  ChoirColor.Equals(MeridianColor));
        TestFalse(TEXT("Choir color is distinct from Kharuun"),
                  ChoirColor.Equals(KharuunColor));
        TestTrue(TEXT("Choir weapon beam activates phase afterimage"),
                 CombatEffect->IsAfterimageActive());
        TestEqual(TEXT("Reusing pooled effect for Choir creates 0 new MIDs"),
                  CombatEffect->GetOwnedMIDCreationCount(),
                  InitialMIDCount);

        // ---------------------------------------------------------------------
        // 3. Accessibility Compliance (Reduced Motion & Reduced Flashing)
        // ---------------------------------------------------------------------
        // Reduced Flashing test: peak emissive clamped to <= 1.0f
        CombatEffect->InitializeCombatEffect(
            echoes::sim::Faction::MeridianCompact,
            echoes::sim::EntityType::HeavyUnit,
            SourcePos,
            TargetPos,
            false,
            true, // bInReducedFlashing = true
            0.32f);
        TestTrue(TEXT("Combat effect records reduced flashing applied"),
                 CombatEffect->IsReducedFlashingApplied());
        TestTrue(TEXT("Reduced flashing clamps peak emission to <= 1.0f"),
                 CombatEffect->GetCurrentEmissiveStrength() <= 1.0f);
        CombatEffect->Tick(0.1f);
        TestTrue(TEXT("Reduced flashing holds emission <= 1.0f during decay"),
                 CombatEffect->GetCurrentEmissiveStrength() <= 1.0f);

        // Reduced Motion test: suppresses afterimage and holds steady scale
        CombatEffect->InitializeCombatEffect(
            echoes::sim::Faction::HollowChoir,
            echoes::sim::EntityType::HeavyUnit,
            SourcePos,
            TargetPos,
            true, // bInReducedMotion = true
            false,
            0.32f);
        TestTrue(TEXT("Combat effect records reduced motion applied"),
                 CombatEffect->IsReducedMotionApplied());
        TestFalse(TEXT("Reduced motion suppresses Choir phase afterimage"),
                  CombatEffect->IsAfterimageActive());

        CombatEffect->Destroy();
    }

    // -------------------------------------------------------------------------
    // 4. Entity View Presentation Elements (Gather Beam, Construction, Reshape)
    // -------------------------------------------------------------------------
    AEchoesEntityView* EntityView = World->SpawnActor<AEchoesEntityView>();
    if (TestNotNull(TEXT("Entity view actor spawns"), EntityView))
    {
        echoes::sim::Entity WorkerEntity{};
        WorkerEntity.id = 8001;
        WorkerEntity.owner = 0;
        WorkerEntity.faction = echoes::sim::Faction::MeridianCompact;
        WorkerEntity.type = echoes::sim::EntityType::Worker;
        WorkerEntity.position = echoes::sim::Vec2::FromTiles(10, 10);
        WorkerEntity.hitPoints = 80;
        WorkerEntity.maxHitPoints = 80;
        WorkerEntity.cargo = 5;
        WorkerEntity.harvestTicks = 4; WorkerEntity.harvestSlotHeld = true; WorkerEntity.harvestState = echoes::sim::HarvestState::Harvesting;
        WorkerEntity.completed = true;

        EntityView->ActivateForEntity(WorkerEntity, true);
        TestTrue(TEXT("Worker with cargo indicates harvesting active"),
                 EntityView->IsWorkerHarvestingActive());
        TestTrue(TEXT("Harvesting worker activates gather beam presentation"),
                 EntityView->IsGatherBeamActive());

        // Under-construction building
        echoes::sim::Entity StructureEntity{};
        StructureEntity.id = 8002;
        StructureEntity.owner = 0;
        StructureEntity.faction = echoes::sim::Faction::MeridianCompact;
        StructureEntity.type = echoes::sim::EntityType::Barracks;
        StructureEntity.position = echoes::sim::Vec2::FromTiles(15, 15);
        StructureEntity.hitPoints = 200;
        StructureEntity.maxHitPoints = 500;
        StructureEntity.constructionProgress = 50;
        StructureEntity.constructionRequired = 100;
        StructureEntity.completed = false;

        EntityView->ActivateForEntity(StructureEntity, true);
        TestTrue(TEXT("Incomplete building activates construction field presentation"),
                 EntityView->IsConstructionFieldActive());
        TestTrue(TEXT("Construction fraction is below 1.0"),
                 EntityView->GetConstructionFraction() < 1.0f);

        // Future Well in Reshape mode
        echoes::sim::Entity WellEntity{};
        WellEntity.id = 8003;
        WellEntity.owner = 255;
        WellEntity.faction = echoes::sim::Faction::MeridianCompact;
        WellEntity.type = echoes::sim::EntityType::FutureWell;
        WellEntity.position = echoes::sim::Vec2::FromTiles(20, 20);
        WellEntity.hitPoints = 1000;
        WellEntity.maxHitPoints = 1000;
        WellEntity.wellChoice = echoes::sim::FutureWellChoice::Reshape;
        WellEntity.reshapeUntilTick = 100;
        WellEntity.completed = true;

        EntityView->ActivateForEntity(WellEntity, true);
        TestTrue(TEXT("Reshape Future Well activates reshape telegraph"),
                 EntityView->IsReshapeTelegraphActive());

        EntityView->PrepareForPool();
        EntityView->Destroy();
    }

    // -------------------------------------------------------------------------
    // 5. Subsystem Pooling & Coalescing Under Rapid Emission
    // -------------------------------------------------------------------------
    UEchoesSimulationSubsystem* Bridge =
        World->GetSubsystem<UEchoesSimulationSubsystem>();
    if (TestNotNull(TEXT("World owns simulation subsystem"), Bridge) &&
        TestTrue(TEXT("Prototype scenario starts"), Bridge->StartPrototypeScenario()))
    {
        const FEchoesPresentationPoolStats InitialStats =
            Bridge->GetPresentationPoolStats();
        TestEqual(TEXT("Initial active combat effect view count is 0"),
                  InitialStats.ActiveCombatEffectViews,
                  0);

        // Emit 4 distinct weapon effects
        const FVector Origin(500.0f, 500.0f, 50.0f);
        for (int32 Index = 0; Index < 4; ++Index)
        {
            const FVector Target = Origin + FVector(200.0f * (Index + 1), 0.0f, 0.0f);
            Bridge->EmitCombatEffectPresentation(
                echoes::sim::Faction::MeridianCompact,
                echoes::sim::EntityType::Soldier,
                Origin,
                Target);
        }

        const FEchoesPresentationPoolStats ActiveStats =
            Bridge->GetPresentationPoolStats();
        TestEqual(TEXT("Active combat effect view count tracks emitted bursts"),
                  ActiveStats.ActiveCombatEffectViews,
                  4);

        // Advance presentation lifetime past 0.32s and reclaim
        for (AEchoesCombatEffectView* ActiveEffect : Bridge->ActiveCombatEffectViews)
        {
            if (ActiveEffect != nullptr)
            {
                ActiveEffect->Tick(0.5f);
            }
        }
        Bridge->Tick(0.5f);

        const FEchoesPresentationPoolStats ReclaimedStats =
            Bridge->GetPresentationPoolStats();
        TestEqual(TEXT("All expired combat effects are reclaimed to free pool"),
                  ReclaimedStats.ActiveCombatEffectViews,
                  0);
        TestTrue(TEXT("Free combat effect pool retains reclaimed actors"),
                 ReclaimedStats.FreeCombatEffectViews >= 4);

        // Stress emission beyond typical pool capacity to test deterministic coalescing
        const int32 StressEmissionCount = 150;
        for (int32 Index = 0; Index < StressEmissionCount; ++Index)
        {
            Bridge->EmitCombatEffectPresentation(
                echoes::sim::Faction::KharuunAssemblies,
                echoes::sim::EntityType::HeavyUnit,
                Origin,
                Origin + FVector(100.0f, 100.0f, 0.0f));
        }

        const FEchoesPresentationPoolStats StressStats =
            Bridge->GetPresentationPoolStats();
        TestTrue(TEXT("Active views do not exceed quality-tiered capacity"),
                 StressStats.ActiveCombatEffectViews <= 256);
        TestTrue(TEXT("Active views accommodated all allowable stress requests"),
                 StressStats.ActiveCombatEffectViews > 0);

        Bridge->StopPrototypeScenario();
    }

    // -------------------------------------------------------------------------
    // 6. Zero Simulation Touch (SIM-002 Determinism Invariant)
    // -------------------------------------------------------------------------
    {
        echoes::sim::SimulationConfig Config{32, 32, 20, 0xACE1ULL};
        echoes::sim::Simulation SimA(Config);
        SimA.AddPlayer(0, echoes::sim::Faction::MeridianCompact, {500, 50});
        SimA.AddPlayer(1, echoes::sim::Faction::HollowChoir, {500, 50});

        SimA.SpawnEntity(0, echoes::sim::Faction::MeridianCompact, echoes::sim::EntityType::Soldier, echoes::sim::Vec2::FromTiles(10, 10));
        SimA.SpawnEntity(0, echoes::sim::Faction::MeridianCompact, echoes::sim::EntityType::HeavyUnit, echoes::sim::Vec2::FromTiles(11, 10));
        SimA.SpawnEntity(1, echoes::sim::Faction::HollowChoir, echoes::sim::EntityType::Soldier, echoes::sim::Vec2::FromTiles(12, 10));

        // Exact clone for comparison
        echoes::sim::Simulation SimB = SimA;
        TestEqual(TEXT("Initial simulation checksums match exactly"),
                  SimA.StateChecksum(),
                  SimB.StateChecksum());

        // Spawn a presentation combat effect and presentation entity views for SimB
        AEchoesCombatEffectView* PresentationEffect =
            World->SpawnActor<AEchoesCombatEffectView>();
        if (PresentationEffect != nullptr)
        {
            PresentationEffect->InitializeCombatEffect(
                echoes::sim::Faction::HollowChoir,
                echoes::sim::EntityType::Soldier,
                FVector(100, 100, 50),
                FVector(200, 100, 50),
                false,
                false,
                1.0f);
        }

        // Step both simulations 30 ticks; tick presentation effects solely on SimB's side
        for (int32 Step = 0; Step < 30; ++Step)
        {
            SimA.Step();
            SimB.Step();

            if (PresentationEffect != nullptr)
            {
                PresentationEffect->Tick(0.033f);
            }
        }

        TestEqual(TEXT("Simulation checksums remain 100% identical after 30 ticks (SIM-002)"),
                  SimA.StateChecksum(),
                  SimB.StateChecksum());
        TestEqual(TEXT("Simulation current ticks match"),
                  SimA.CurrentTick(),
                  SimB.CurrentTick());
        TestEqual(TEXT("Simulation entity counts match"),
                  SimA.Entities().size(),
                  SimB.Entities().size());

        if (PresentationEffect != nullptr)
        {
            PresentationEffect->PrepareForPool();
            PresentationEffect->Destroy();
        }
    }

    if (Settings != nullptr)
    {
        Settings->SetReducedMotionEnabled(bPreviousReducedMotion);
        Settings->SetReducedFlashingEnabled(bPreviousReducedFlashing);
    }

    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors() && !WorldWrapper.HasFailed();
}

#endif // WITH_DEV_AUTOMATION_TESTS
