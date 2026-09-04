#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "EchoesEntityView.h"
#include "EchoesGameUserSettings.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesSimCore/Simulation.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesEntityMotionTest,
    "Echoes.Runtime.Presentation.MotionFamilies",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesEntityMotionTest::RunTest(const FString& Parameters)
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
        AddError(TEXT("Could not create the motion-families test world."));
        return false;
    }
    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge =
        World != nullptr
            ? World->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (!TestNotNull(TEXT("Motion world owns the simulation subsystem"), Bridge) ||
        !TestTrue(TEXT("Motion fixture starts an authoritative scenario"),
                  Bridge != nullptr && Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get();
    const bool bPreviousReducedMotion =
        Settings != nullptr && Settings->IsReducedMotionEnabled();
    if (Settings != nullptr)
    {
        Settings->SetReducedMotionEnabled(false);
    }

    const auto MakeEntity = [](
        uint32 Id,
        uint8 Owner,
        echoes::sim::Faction Faction,
        echoes::sim::EntityType Type,
        std::int32_t TileX = 10,
        std::int32_t TileY = 10)
    {
        echoes::sim::Entity State{};
        State.id = Id;
        State.owner = Owner;
        State.faction = Faction;
        State.type = Type;
        State.position = echoes::sim::Vec2::FromTiles(TileX, TileY);
        State.hitPoints = 100;
        State.maxHitPoints = 100;
        State.completed = true;
        return State;
    };

    // -------------------------------------------------------------------------
    // 1. Walker Locomotion & Stride Cadence (Meridian Lancer)
    // -------------------------------------------------------------------------
    {
        AEchoesEntityView* Walker = World->SpawnActor<AEchoesEntityView>();
        TestNotNull(TEXT("Walker actor spawns"), Walker);
        if (Walker != nullptr)
        {
            echoes::sim::Entity Lancer = MakeEntity(
                5001,
                0,
                echoes::sim::Faction::MeridianCompact,
                echoes::sim::EntityType::Soldier,
                10,
                10);
            Walker->ActivateForEntity(Lancer, true);
            TestTrue(TEXT("Lancer is recognized as a walker unit"),
                     Walker->IsWalkerUnit());
            TestFalse(TEXT("Stationary walker has no locomotion active"),
                      Walker->IsLocomotionMotionActive());

            // Move forward along +X: (10, 10) -> (11, 10)
            echoes::sim::Entity MovingLancer = Lancer;
            MovingLancer.position = echoes::sim::Vec2::FromTiles(11, 10);
            Walker->ApplyAuthoritativeState(MovingLancer, false);

            TestTrue(TEXT("Moving walker activates locomotion motion"),
                     Walker->IsLocomotionMotionActive());
            TestTrue(TEXT("Moving walker has positive authoritative speed"),
                     Walker->GetAuthoritativeVelocity().Size2D() > 0.0f);

            Walker->Tick(0.1f);
            TestTrue(TEXT("Walker advances walk cycle phase"),
                     Walker->GetLocomotionWalkPhase() > 0.0f);
            TestTrue(TEXT("Walker exhibits stride vertical bounce"),
                     Walker->GetBodyMeshRelativeLocation().Z > 0.0f);

            // Verify ReducedMotion compliance
            if (Settings != nullptr)
            {
                Settings->SetReducedMotionEnabled(true);
            }
            Walker->Tick(0.05f);
            TestTrue(TEXT("Reduced motion flag is detected by walker view"),
                     Walker->IsMotionReducedMotionApplied());
            TestTrue(TEXT("Reduced motion locks walker body location to origin"),
                     Walker->GetBodyMeshRelativeLocation().IsNearlyZero());

            if (Settings != nullptr)
            {
                Settings->SetReducedMotionEnabled(false);
            }
            Walker->PrepareForPool();
            Walker->Destroy();
        }
    }

    // -------------------------------------------------------------------------
    // 2. Hover Locomotion & Banking (Meridian RelaySkiff & Choir Afterimage)
    // -------------------------------------------------------------------------
    {
        AEchoesEntityView* HoverView = World->SpawnActor<AEchoesEntityView>();
        TestNotNull(TEXT("Hover actor spawns"), HoverView);
        if (HoverView != nullptr)
        {
            echoes::sim::Entity Skiff = MakeEntity(
                5002,
                0,
                echoes::sim::Faction::MeridianCompact,
                echoes::sim::EntityType::ScoutUnit,
                12,
                12);
            HoverView->ActivateForEntity(Skiff, true);
            TestTrue(TEXT("Relay Skiff is recognized as a hover unit"),
                     HoverView->IsHoverUnit());
            TestFalse(TEXT("Relay Skiff is not a walker unit"),
                      HoverView->IsWalkerUnit());

            // Tick stationary: should exhibit sinusoidal vertical hover bobbing
            HoverView->Tick(0.2f);
            TestFalse(TEXT("Stationary hover unit has non-zero bob offset"),
                      FMath::IsNearlyZero(HoverView->GetHoverBobOffsetCentimetres()));
            TestTrue(TEXT("Hover body location incorporates elevation and bob"),
                     HoverView->GetBodyMeshRelativeLocation().Z > 0.0f);

            // Move along +Y: banking roll and acceleration pitch
            echoes::sim::Entity MovingSkiff = Skiff;
            MovingSkiff.position = echoes::sim::Vec2::FromTiles(12, 13);
            HoverView->ApplyAuthoritativeState(MovingSkiff, false);
            HoverView->Tick(0.1f);
            TestTrue(TEXT("Moving hover unit activates locomotion motion"),
                     HoverView->IsLocomotionMotionActive());

            // Verify ReducedMotion compliance
            if (Settings != nullptr)
            {
                Settings->SetReducedMotionEnabled(true);
            }
            HoverView->Tick(0.05f);
            TestEqual(TEXT("Reduced motion zeroes hover bob offset"),
                      HoverView->GetHoverBobOffsetCentimetres(),
                      0.0f);
            TestTrue(TEXT("Reduced motion zeroes relative hover offset"),
                     HoverView->GetBodyMeshRelativeLocation().IsNearlyZero());

            if (Settings != nullptr)
            {
                Settings->SetReducedMotionEnabled(false);
            }
            HoverView->PrepareForPool();
            HoverView->Destroy();
        }
    }

    // -------------------------------------------------------------------------
    // 3. Faction Idle Micro-Motion (Kharuun Strata Respiration & Choir Lissajous)
    // -------------------------------------------------------------------------
    {
        // Kharuun Tender: Strata respiration (scale dilation on Y and Z)
        AEchoesEntityView* KharuunView = World->SpawnActor<AEchoesEntityView>();
        TestNotNull(TEXT("Kharuun actor spawns"), KharuunView);
        if (KharuunView != nullptr)
        {
            echoes::sim::Entity Tender = MakeEntity(
                5003,
                1,
                echoes::sim::Faction::KharuunAssemblies,
                echoes::sim::EntityType::Worker,
                14,
                14);
            KharuunView->ActivateForEntity(Tender, true);
            KharuunView->Tick(0.5f);

            const FVector RespirationScale =
                KharuunView->GetBodyMeshRelativeScale();
            TestFalse(TEXT("Kharuun strata respiration dilates Y scale"),
                      FMath::IsNearlyEqual(RespirationScale.Y, 1.0f, 0.0001f));
            TestFalse(TEXT("Kharuun strata respiration dilates Z scale"),
                      FMath::IsNearlyEqual(RespirationScale.Z, 1.0f, 0.0001f));

            if (Settings != nullptr)
            {
                Settings->SetReducedMotionEnabled(true);
            }
            KharuunView->Tick(0.05f);
            TestTrue(TEXT("Reduced motion resets Kharuun scale to exactly One"),
                     KharuunView->GetBodyMeshRelativeScale().Equals(FVector::OneVector));

            if (Settings != nullptr)
            {
                Settings->SetReducedMotionEnabled(false);
            }
            KharuunView->PrepareForPool();
            KharuunView->Destroy();
        }

        // Hollow Choir Intervalist: Lissajous quantum phase drift
        AEchoesEntityView* ChoirView = World->SpawnActor<AEchoesEntityView>();
        TestNotNull(TEXT("Choir actor spawns"), ChoirView);
        if (ChoirView != nullptr)
        {
            echoes::sim::Entity Intervalist = MakeEntity(
                5004,
                2,
                echoes::sim::Faction::HollowChoir,
                echoes::sim::EntityType::Soldier,
                16,
                16);
            ChoirView->ActivateForEntity(Intervalist, true);
            ChoirView->Tick(0.5f);

            const FVector ChoirOffset =
                ChoirView->GetBodyMeshRelativeLocation();
            TestFalse(TEXT("Choir unit exhibits Lissajous drift offset"),
                      ChoirOffset.IsNearlyZero());

            if (Settings != nullptr)
            {
                Settings->SetReducedMotionEnabled(true);
            }
            ChoirView->Tick(0.05f);
            TestTrue(TEXT("Reduced motion resets Choir Lissajous offset to zero"),
                     ChoirView->GetBodyMeshRelativeLocation().IsNearlyZero());

            if (Settings != nullptr)
            {
                Settings->SetReducedMotionEnabled(false);
            }
            ChoirView->PrepareForPool();
            ChoirView->Destroy();
        }
    }

    // -------------------------------------------------------------------------
    // 4. Tactical States & Worker Gestures
    // -------------------------------------------------------------------------
    {
        // Bulwark Deploy
        AEchoesEntityView* BulwarkView = World->SpawnActor<AEchoesEntityView>();
        if (BulwarkView != nullptr)
        {
            echoes::sim::Entity Bulwark = MakeEntity(
                5005,
                0,
                echoes::sim::Faction::MeridianCompact,
                echoes::sim::EntityType::HeavyUnit,
                18,
                18);
            Bulwark.deployed = true;
            Bulwark.deploymentFacing = echoes::sim::Vec2::FromRaw(0, echoes::sim::kFixedScale); // Facing +Y (90 deg)
            BulwarkView->ActivateForEntity(Bulwark, true);
            BulwarkView->Tick(0.2f);
            TestTrue(TEXT("Deployed Bulwark faces deployment direction"),
                     FMath::IsNearlyEqual(BulwarkView->GetHeadingYaw(), 90.0f, 1.0f));
            TestTrue(TEXT("Deployed Bulwark has deployment cover visible"),
                     BulwarkView->IsDeploymentCoverVisible());

            BulwarkView->PrepareForPool();
            BulwarkView->Destroy();
        }

        // Cairnback Defensive Mineral Cover Hunker
        AEchoesEntityView* CairnbackView = World->SpawnActor<AEchoesEntityView>();
        if (CairnbackView != nullptr)
        {
            echoes::sim::Entity Cairnback = MakeEntity(
                5006,
                1,
                echoes::sim::Faction::KharuunAssemblies,
                echoes::sim::EntityType::HeavyUnit,
                20,
                20);
            Cairnback.temporaryMineralCover = true;
            CairnbackView->ActivateForEntity(Cairnback, true);
            CairnbackView->Tick(0.1f);

            TestTrue(TEXT("Cairnback in mineral cover hunkers down (negative Z offset)"),
                     CairnbackView->GetBodyMeshRelativeLocation().Z < -10.0f);

            CairnbackView->PrepareForPool();
            CairnbackView->Destroy();
        }

        // Worker Harvesting and Cargo
        AEchoesEntityView* WorkerView = World->SpawnActor<AEchoesEntityView>();
        if (WorkerView != nullptr)
        {
            echoes::sim::Entity Surveyor = MakeEntity(
                5007,
                0,
                echoes::sim::Faction::MeridianCompact,
                echoes::sim::EntityType::Worker,
                22,
                22);
            Surveyor.cargo = 20;
            Surveyor.harvestTicks = 5;
            WorkerView->ActivateForEntity(Surveyor, true);
            WorkerView->Tick(0.1f);

            TestTrue(TEXT("Worker recognizes active harvesting"),
                     WorkerView->IsWorkerHarvestingActive());
            TestEqual(TEXT("Worker tracks carried cargo amount"),
                      WorkerView->GetCarriedCargoAmount(),
                      20);

            WorkerView->PrepareForPool();
            WorkerView->Destroy();
        }
    }

    // -------------------------------------------------------------------------
    // 5. Zero Simulation Mutation Invariant (SIM-002)
    // -------------------------------------------------------------------------
    {
        echoes::sim::SimulationConfig Config{32, 32, 20, 0x1337ULL};
        echoes::sim::Simulation SimA(Config);
        SimA.AddPlayer(0, echoes::sim::Faction::MeridianCompact, {500, 50});
        SimA.AddPlayer(1, echoes::sim::Faction::KharuunAssemblies, {500, 50});

        SimA.SpawnEntity(0, echoes::sim::Faction::MeridianCompact, echoes::sim::EntityType::Soldier, echoes::sim::Vec2::FromTiles(10, 10));
        SimA.SpawnEntity(0, echoes::sim::Faction::MeridianCompact, echoes::sim::EntityType::ScoutUnit, echoes::sim::Vec2::FromTiles(12, 12));
        SimA.SpawnEntity(1, echoes::sim::Faction::KharuunAssemblies, echoes::sim::EntityType::Soldier, echoes::sim::Vec2::FromTiles(20, 20));

        // Create an exact clone
        echoes::sim::Simulation SimB = SimA;

        TestEqual(TEXT("Cloned simulations have identical initial checksums"),
                  SimA.StateChecksum(),
                  SimB.StateChecksum());

        // Spawn presentation views for SimB's entities
        TArray<AEchoesEntityView*> ActiveViews;
        for (const echoes::sim::Entity& E : SimB.Entities())
        {
            AEchoesEntityView* View = World->SpawnActor<AEchoesEntityView>();
            if (View != nullptr)
            {
                View->ActivateForEntity(E, true);
                ActiveViews.Add(View);
            }
        }

        // Step both simulations for 40 ticks
        for (int32 Step = 0; Step < 40; ++Step)
        {
            SimA.Step();
            SimB.Step();

            // Tick presentation views with varying delta times, apply authoritative states,
            // and toggle reduced motion
            for (AEchoesEntityView* View : ActiveViews)
            {
                if (View != nullptr)
                {
                    const echoes::sim::Entity* LiveEntity =
                        SimB.FindEntity(View->GetEntityId());
                    if (LiveEntity != nullptr)
                    {
                        View->ApplyAuthoritativeState(*LiveEntity, false);
                    }
                    View->Tick(0.033f + 0.01f * (Step % 3));
                }
            }
        }

        TestEqual(TEXT("Simulation checksums remain 100% identical after 40 ticks with active motion views (SIM-002)"),
                  SimA.StateChecksum(),
                  SimB.StateChecksum());
        TestEqual(TEXT("Simulation ticks remain identical"),
                  SimA.CurrentTick(),
                  SimB.CurrentTick());
        TestEqual(TEXT("Simulation entity counts remain identical"),
                  SimA.Entities().size(),
                  SimB.Entities().size());

        for (AEchoesEntityView* View : ActiveViews)
        {
            if (View != nullptr)
            {
                View->PrepareForPool();
                View->Destroy();
            }
        }
    }

    if (Bridge != nullptr)
    {
        Bridge->StopPrototypeScenario();
    }

    if (Settings != nullptr)
    {
        Settings->SetReducedMotionEnabled(bPreviousReducedMotion);
    }

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
