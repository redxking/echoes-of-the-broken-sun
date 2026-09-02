#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "EchoesDestructionView.h"
#include "EchoesEntityView.h"
#include "EchoesGameUserSettings.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesPresentationPoolingTest,
    "Echoes.Runtime.Presentation.Pooling",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesPresentationPoolingTest::RunTest(const FString& Parameters)
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
        AddError(TEXT("Could not create the presentation-pooling test world."));
        return false;
    }
    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge =
        World != nullptr
            ? World->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (!TestNotNull(TEXT("Pooling world owns the simulation subsystem"), Bridge) ||
        !TestTrue(TEXT("Pooling fixture starts an authoritative scenario"),
                  Bridge != nullptr && Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get();
    const int32 PreviousEffectsQuality =
        Settings != nullptr ? Settings->GetVisualEffectQuality() : 3;
    const float PreviousEffectsVolume =
        Settings != nullptr ? Settings->GetEffectsVolume() : 1.0f;
    const bool bPreviousReducedMotion =
        Settings != nullptr && Settings->IsReducedMotionEnabled();
    const bool bPreviousReducedFlashing =
        Settings != nullptr && Settings->IsReducedFlashingEnabled();
    if (Settings != nullptr)
    {
        Settings->SetReducedMotionEnabled(false);
        Settings->SetReducedFlashingEnabled(false);
    }

    AEchoesEntityView* RebindView = World->SpawnActor<AEchoesEntityView>();
    if (TestNotNull(TEXT("Cross-archetype rebind actor spawns"), RebindView))
    {
        const auto MakeEntity = [](
            uint32 Id,
            uint8 Owner,
            echoes::sim::Faction Faction,
            echoes::sim::EntityType Type)
        {
            echoes::sim::Entity State{};
            State.id = Id;
            State.owner = Owner;
            State.faction = Faction;
            State.type = Type;
            State.position = echoes::sim::Vec2::FromTiles(8, 8);
            State.hitPoints = 120;
            State.maxHitPoints = 120;
            State.completed = true;
            return State;
        };
        echoes::sim::Entity Covered = MakeEntity(
            900001,
            1,
            echoes::sim::Faction::KharuunAssemblies,
            echoes::sim::EntityType::HeavyUnit);
        Covered.deployed = true;
        Covered.relaySupplyActive = true;
        Covered.waystoneMode = echoes::sim::WaystoneMode::Rooted;
        Covered.warformAdaptation =
            echoes::sim::WarformAdaptation::Carapace;
        Covered.pendingWarformAdaptation =
            echoes::sim::WarformAdaptation::Striker;
        Covered.temporaryMineralCover = true;
        Covered.aegisPowered = true;
        RebindView->ActivateForEntity(Covered, true);
        RebindView->SetSelected(true);
        echoes::sim::Entity Damaged = Covered;
        Damaged.hitPoints = 60;
        RebindView->ApplyAuthoritativeState(Damaged, true);
        TestTrue(TEXT("Warm actor carries identity-local presentation state"),
                 RebindView->IsSelected() &&
                     RebindView->IsDamagePulseActive() &&
                     RebindView->IsTemporaryMineralCover() &&
                     RebindView->IsWarformStateVisible());

        RebindView->PrepareForPool();
        TestTrue(TEXT("Entity actor enters the prepared pool state"),
                 RebindView->IsPreparedForPool());
        TestEqual(TEXT("Pooling clears entity identity"),
                  RebindView->GetEntityId(),
                  static_cast<uint32>(0));
        TestFalse(TEXT("Pooling clears selection"), RebindView->IsSelected());
        TestFalse(TEXT("Pooling clears damage state"),
                  RebindView->IsDamagePulseActive());
        TestFalse(TEXT("Pooling clears temporary cover"),
                  RebindView->IsTemporaryMineralCover());
        TestFalse(TEXT("Pooling clears warform presentation"),
                  RebindView->IsWarformStateVisible());
        TestTrue(TEXT("Pooling resets the actor transform"),
                 RebindView->GetActorTransform().Equals(FTransform::Identity));

        echoes::sim::Entity Deployment = MakeEntity(
            900002,
            0,
            echoes::sim::Faction::MeridianCompact,
            echoes::sim::EntityType::HeavyUnit);
        Deployment.deployed = true;
        RebindView->ActivateForEntity(Deployment, true);
        TestTrue(TEXT("Deployment reset fixture is visibly active before pooling"),
                 RebindView->IsDeploymentCoverVisible());
        RebindView->PrepareForPool();
        TestFalse(TEXT("Pooling clears deployment presentation"),
                  RebindView->IsDeploymentCoverVisible());

        echoes::sim::Entity Relay = MakeEntity(
            900003,
            0,
            echoes::sim::Faction::MeridianCompact,
            echoes::sim::EntityType::ScoutUnit);
        Relay.relaySupplyActive = true;
        RebindView->ActivateForEntity(Relay, true);
        TestTrue(TEXT("Relay reset fixture is visibly active before pooling"),
                 RebindView->IsRelaySupplyFieldVisible());
        RebindView->PrepareForPool();
        TestFalse(TEXT("Pooling clears relay presentation"),
                  RebindView->IsRelaySupplyFieldVisible());

        echoes::sim::Entity Waystone = MakeEntity(
            900004,
            1,
            echoes::sim::Faction::KharuunAssemblies,
            echoes::sim::EntityType::Dropoff);
        Waystone.waystoneMode = echoes::sim::WaystoneMode::Rooted;
        RebindView->ActivateForEntity(Waystone, true);
        TestTrue(TEXT("Waystone reset fixture is visibly active before pooling"),
                 RebindView->IsWaystoneStateVisible());
        RebindView->PrepareForPool();
        TestFalse(TEXT("Pooling clears waystone presentation"),
                  RebindView->IsWaystoneStateVisible());

        echoes::sim::Entity Choir = MakeEntity(
            900005,
            2,
            echoes::sim::Faction::HollowChoir,
            echoes::sim::EntityType::Soldier);
        Choir.choirIdentityState =
            echoes::sim::ChoirIdentityState::Possible;
        RebindView->ActivateForEntity(Choir, true);
        TestTrue(TEXT("Choir reset fixture is visibly active before pooling"),
                 RebindView->IsChoirIdentityStateVisible());
        RebindView->PrepareForPool();
        TestFalse(TEXT("Pooling clears Choir identity presentation"),
                  RebindView->IsChoirIdentityStateVisible());

        echoes::sim::Entity Aegis = MakeEntity(
            900006,
            0,
            echoes::sim::Faction::MeridianCompact,
            echoes::sim::EntityType::UtilityStructure);
        Aegis.aegisPowered = true;
        RebindView->ActivateForEntity(Aegis, true);
        TestTrue(TEXT("Aegis reset fixture is visibly active before pooling"),
                 RebindView->IsAegisPowerFieldVisible());
        RebindView->PrepareForPool();
        TestFalse(TEXT("Pooling clears Aegis power presentation"),
                  RebindView->IsAegisPowerFieldVisible());

        echoes::sim::Entity FutureWell = MakeEntity(
            900007,
            echoes::sim::kNeutralPlayer,
            echoes::sim::Faction::MeridianCompact,
            echoes::sim::EntityType::FutureWell);
        FutureWell.position = echoes::sim::Vec2::FromTiles(12, 12);
        FutureWell.hitPoints = 500;
        FutureWell.maxHitPoints = 500;
        FutureWell.wellChoice = echoes::sim::FutureWellChoice::Reshape;
        RebindView->ActivateForEntity(FutureWell, true);
        TestEqual(TEXT("Rebind applies only the new Future Well identity"),
                  RebindView->GetEntityId(),
                  FutureWell.id);
        TestTrue(TEXT("Rebind restores body selection collision"),
                 RebindView->HasBodySelectionCollisionEnabled());
        TestTrue(TEXT("Rebind applies the new Future Well presentation"),
                 RebindView->IsFutureWellPresentationVisible());
        // The Future Well suppresses its silhouette accent, so the orbit, core
        // and glyphs are the only drawn geometry a player can point at besides
        // the base. They carry the entity-pick region, and they are 145-224 cm
        // in the air, so they must stay out of the ground trace.
        TestTrue(TEXT("A drawn Future Well orbit answers entity resolution"),
                 RebindView->IsFutureWellPresentationEntityPickable());
        TestFalse(TEXT("A Future Well pick volume stays out of the ground trace"),
                  RebindView->DoesEntityPickProxyBlockGroundTrace());
        const float WellPickRadius = RebindView->GetEntityPickProxyRadius();
        TestTrue(TEXT("A Future Well has a footprint-sized pick radius"),
                 WellPickRadius > 0.0f);
        TestFalse(TEXT("Rebind does not inherit old selection"),
                  RebindView->IsSelected());
        RebindView->PrepareForPool();
        TestFalse(TEXT("Pooling clears Future Well presentation"),
                  RebindView->IsFutureWellPresentationVisible());
        // A pooled view must leave no pick region behind for the next entity
        // to inherit while it is not drawn.
        TestFalse(TEXT("Pooling clears the health bar pick region"),
                  RebindView->IsHealthBarEntityPickable());
        TestFalse(TEXT("Pooling clears the owner marker pick region"),
                  RebindView->IsOwnerMarkerEntityPickable());
        TestFalse(TEXT("Pooling clears the silhouette accent pick region"),
                  RebindView->IsSilhouetteAccentEntityPickable());
        TestFalse(TEXT("Pooling clears the footprint pick volume"),
                  RebindView->IsEntityPickProxyEnabled());
        TestFalse(TEXT("Pooling clears the Future Well pick region"),
                  RebindView->IsFutureWellPresentationEntityPickable());

        echoes::sim::Entity Resource = FutureWell;
        Resource.id = 900008;
        Resource.type = echoes::sim::EntityType::ResourceNode;
        Resource.wellChoice = echoes::sim::FutureWellChoice::Dormant;
        RebindView->ActivateForEntity(Resource, true);
        // On the reactivated view the pick region and the drawn overlay move
        // together, so a rebound full-health entity carries no invisible pick
        // plate above it.
        TestTrue(TEXT("Rebound health bar pick region matches what is drawn"),
                 RebindView->IsHealthBarEntityPickable() ==
                     RebindView->IsHealthBarVisible());
        TestTrue(TEXT("Rebound owner marker pick region matches what is drawn"),
                 RebindView->IsOwnerMarkerEntityPickable() ==
                     RebindView->IsOwnerMarkerVisible());
        TestTrue(TEXT("Rebound silhouette accent pick region matches what is drawn"),
                 RebindView->IsSilhouetteAccentEntityPickable() ==
                     RebindView->IsSilhouetteAccentVisible());
        // The reported defect: a Matter deposit draws no accent, no owner
        // marker and no Future Well geometry. Without the footprint volume the
        // only thing under the cursor is whatever collision its authored mesh
        // happens to carry, and a miss becomes a move order on the ground
        // behind it.
        TestFalse(TEXT("A Matter deposit draws no silhouette accent"),
                  RebindView->IsSilhouetteAccentVisible());
        TestFalse(TEXT("A Matter deposit draws no Future Well geometry"),
                  RebindView->IsFutureWellPresentationVisible());
        TestTrue(TEXT("A Matter deposit still answers entity resolution"),
                 RebindView->IsEntityPickProxyEnabled());
        TestTrue(TEXT("A Matter deposit's pick volume is never rendered"),
                 RebindView->IsEntityPickProxyHidden());
        TestFalse(TEXT("A Matter deposit's pick volume stays out of the ground trace"),
                  RebindView->DoesEntityPickProxyBlockGroundTrace());
        TestTrue(TEXT("A Matter deposit has a footprint-sized pick radius"),
                 RebindView->GetEntityPickProxyRadius() > 0.0f);
        TestTrue(TEXT("A Matter deposit's pick volume clears its health bar"),
                 RebindView->GetEntityPickProxyTopHeight() > 0.0f);
        TestTrue(TEXT("Body selection collision survives the rebind"),
                 RebindView->HasBodySelectionCollisionEnabled());
        const uint64 WarmMIDCount = RebindView->GetOwnedMIDCreationCount();
        for (const echoes::sim::Entity* WarmState :
             {&Covered, &Deployment, &Relay, &Waystone,
              &Choir, &Aegis, &FutureWell, &Resource})
        {
            RebindView->PrepareForPool();
            RebindView->ActivateForEntity(*WarmState, true);
        }
        TestEqual(TEXT("Warmed cross-archetype rebinds create no further MIDs"),
                  RebindView->GetOwnedMIDCreationCount(),
                  WarmMIDCount);
        RebindView->Destroy();
    }

    if (Settings != nullptr)
    {
        Settings->SetVisualEffectQuality(3);
        Settings->SetEffectsVolume(0.0f);
    }
    const int32 Capacity =
        UEchoesSimulationSubsystem::GetDestructionPoolCapacityForEffectsQuality(3);
    TestEqual(TEXT("High effects tier admits the full 396-unit burst"),
              Capacity,
              396);
    const uint64 ChecksumBeforeBurst =
        Bridge->GetSimulation()->StateChecksum();
    for (int32 Index = 0; Index < Capacity + 1; ++Index)
    {
        Bridge->EmitDestructionPresentation(
            static_cast<uint32>(910000 + Index),
            FVector(static_cast<float>(Index), 0.0f, 0.0f),
            static_cast<echoes::sim::Faction>(Index % 3),
            echoes::sim::EntityType::Soldier);
    }
    const FEchoesPresentationPoolStats BurstStats =
        Bridge->GetPresentationPoolStats();
    TestEqual(TEXT("Burst fills but never exceeds the destruction capacity"),
              BurstStats.ActiveDestructionViews,
              Capacity);
    TestEqual(TEXT("Burst creates no spare destruction actor"),
              BurstStats.FreeDestructionViews,
              0);
    TestEqual(TEXT("Burst creates one actor per admitted slot"),
              BurstStats.DestructionCreated,
              static_cast<uint64>(Capacity));
    TestEqual(TEXT("Burst overflow is counted exactly"),
              BurstStats.DestructionOverflow,
              static_cast<uint64>(1));
    TestEqual(TEXT("Burst overflow is deterministically coalesced"),
              BurstStats.DestructionCoalesced,
              static_cast<uint64>(1));
    TestEqual(TEXT("Destruction MID ownership is bounded at four per actor"),
              BurstStats.DestructionOwnedMIDCreated,
              static_cast<uint64>(Capacity * 4));
    TestEqual(TEXT("Presentation burst cannot change simulation authority"),
              Bridge->GetSimulation()->StateChecksum(),
              ChecksumBeforeBurst);

    Bridge->ResetDestructionViewsForScenario();
    const FEchoesPresentationPoolStats ResetStats =
        Bridge->GetPresentationPoolStats();
    TestEqual(TEXT("Scenario reset clears active destruction views"),
              ResetStats.ActiveDestructionViews,
              0);
    TestEqual(TEXT("Scenario reset retains exactly the bounded destruction pool"),
              ResetStats.FreeDestructionViews,
              Capacity);
    Bridge->EmitDestructionPresentation(
        920000,
        FVector::ZeroVector,
        echoes::sim::Faction::HollowChoir,
        echoes::sim::EntityType::CommandCore);
    const FEchoesPresentationPoolStats ReuseStats =
        Bridge->GetPresentationPoolStats();
    TestEqual(TEXT("Post-reset activation reuses a destruction actor"),
              ReuseStats.DestructionReused,
              static_cast<uint64>(1));
    TestEqual(TEXT("Post-reset activation creates no new destruction actor"),
              ReuseStats.DestructionCreated,
              BurstStats.DestructionCreated);
    TestEqual(TEXT("Post-reset activation creates no new destruction MID"),
              ReuseStats.DestructionOwnedMIDCreated,
              BurstStats.DestructionOwnedMIDCreated);

    Bridge->SetScenarioPaused(true);
    const uint64 ChecksumBeforeAutomaticReclaim =
        Bridge->GetSimulation()->StateChecksum();
    TestEqual(TEXT("Automatic lifecycle fixture owns one active destruction view"),
              ReuseStats.ActiveDestructionViews,
              1);
    TestEqual(TEXT("Automatic lifecycle fixture leaves one pool slot occupied"),
              ReuseStats.FreeDestructionViews,
              Capacity - 1);
    AEchoesDestructionView* LifecycleView =
        Bridge->ActiveDestructionViews.Num() == 1
            ? Bridge->ActiveDestructionViews[0].Get()
            : nullptr;
    if (TestNotNull(TEXT("Automatic lifecycle actor is retained by the active pool"),
                    LifecycleView))
    {
        TestTrue(TEXT("Automatic lifecycle actor begins active"),
                 LifecycleView->IsPresentationActive());
        LifecycleView->Tick(
            LifecycleView->GetPresentationLifetimeSeconds() + 0.01f);
        TestFalse(TEXT("Real presentation lifetime deactivates the actor"),
                  LifecycleView->IsPresentationActive());
        TestFalse(TEXT("Lifetime expiry does not destroy the pooled actor"),
                  LifecycleView->IsActorBeingDestroyed());
    }

    const FEchoesPresentationPoolStats ExpiredBeforeReclaimStats =
        Bridge->GetPresentationPoolStats();
    TestEqual(TEXT("Expired actor remains active-accounted until subsystem reclaim"),
              ExpiredBeforeReclaimStats.ActiveDestructionViews,
              1);
    TestEqual(TEXT("Expired actor has not bypassed the bounded free pool"),
              ExpiredBeforeReclaimStats.FreeDestructionViews,
              Capacity - 1);

    Bridge->Tick(0.0f);
    const FEchoesPresentationPoolStats AutomaticReclaimStats =
        Bridge->GetPresentationPoolStats();
    TestEqual(TEXT("Subsystem tick reclaims the expired active actor"),
              AutomaticReclaimStats.ActiveDestructionViews,
              0);
    TestEqual(TEXT("Subsystem reclaim restores full destruction capacity"),
              AutomaticReclaimStats.FreeDestructionViews,
              Capacity);
    TestEqual(TEXT("Subsystem reclaim records one production release"),
              AutomaticReclaimStats.DestructionReleased,
              ReuseStats.DestructionReleased + 1);
    TestEqual(TEXT("Automatic reclaim creates no replacement actor"),
              AutomaticReclaimStats.DestructionCreated,
              ReuseStats.DestructionCreated);
    TestEqual(TEXT("Automatic reclaim creates no replacement MID"),
              AutomaticReclaimStats.DestructionOwnedMIDCreated,
              ReuseStats.DestructionOwnedMIDCreated);
    TestTrue(TEXT("Automatically reclaimed actor remains valid in the free pool"),
             IsValid(LifecycleView) &&
                 !LifecycleView->IsActorBeingDestroyed() &&
                 Bridge->FreeDestructionViews.Contains(LifecycleView));

    Bridge->EmitDestructionPresentation(
        920001,
        FVector(10.0f, 0.0f, 0.0f),
        echoes::sim::Faction::MeridianCompact,
        echoes::sim::EntityType::HeavyUnit);
    const FEchoesPresentationPoolStats AutomaticReuseStats =
        Bridge->GetPresentationPoolStats();
    TestEqual(TEXT("Emission after automatic reclaim is admitted"),
              AutomaticReuseStats.ActiveDestructionViews,
              1);
    TestEqual(TEXT("Emission after automatic reclaim consumes one free slot"),
              AutomaticReuseStats.FreeDestructionViews,
              Capacity - 1);
    TestEqual(TEXT("Emission after automatic reclaim reuses the actor"),
              AutomaticReuseStats.DestructionReused,
              ReuseStats.DestructionReused + 1);
    TestEqual(TEXT("Automatic reuse creates no new destruction actor"),
              AutomaticReuseStats.DestructionCreated,
              ReuseStats.DestructionCreated);
    TestEqual(TEXT("Automatic reuse creates no new destruction MID"),
              AutomaticReuseStats.DestructionOwnedMIDCreated,
              ReuseStats.DestructionOwnedMIDCreated);
    TestEqual(TEXT("Automatic reuse does not add overflow"),
              AutomaticReuseStats.DestructionOverflow,
              ReuseStats.DestructionOverflow);
    TestEqual(TEXT("Automatic reuse does not silently coalesce"),
              AutomaticReuseStats.DestructionCoalesced,
              ReuseStats.DestructionCoalesced);
    TestTrue(TEXT("Automatic reuse returns the exact expired actor"),
             Bridge->ActiveDestructionViews.Num() == 1 &&
                 Bridge->ActiveDestructionViews[0].Get() == LifecycleView);
    TestEqual(TEXT("Automatic destruction lifecycle cannot change authority"),
              Bridge->GetSimulation()->StateChecksum(),
              ChecksumBeforeAutomaticReclaim);
    if (Settings != nullptr)
    {
        Settings->SetVisualEffectQuality(PreviousEffectsQuality);
        Settings->SetEffectsVolume(PreviousEffectsVolume);
        Settings->SetReducedMotionEnabled(bPreviousReducedMotion);
        Settings->SetReducedFlashingEnabled(bPreviousReducedFlashing);
    }

    Bridge->StopPrototypeScenario();
    Bridge->DestroyPooledPresentationActors();
    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors() && !WorldWrapper.HasFailed();
}

#endif
