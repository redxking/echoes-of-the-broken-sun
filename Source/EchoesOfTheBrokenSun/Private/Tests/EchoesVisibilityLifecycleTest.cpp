#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "EchoesEntityView.h"
#include "EchoesFogView.h"
#include "EchoesGameUserSettings.h"
#include "EchoesSimCore/Simulation.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesVisibilityLifecycleTest,
    "Echoes.Runtime.Visibility.ActorLifecycle",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesVisibilityLifecycleTest::RunTest(const FString& Parameters)
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
        AddError(TEXT("Could not create the temporary game world."));
        return false;
    }

    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge =
        World != nullptr
            ? World->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (!TestNotNull(TEXT("Temporary world owns the simulation subsystem"), Bridge) ||
        !TestTrue(TEXT("Prototype scenario starts in the temporary world"),
                  Bridge != nullptr && Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const echoes::sim::Simulation* InitialSimulation = Bridge->GetSimulation();
    if (!TestNotNull(TEXT("Prototype simulation is available"), InitialSimulation))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    AEchoesFogView* FogView = Bridge->GetFogView();
    if (!TestNotNull(TEXT("Local fog/shroud presentation is available"), FogView))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    TestEqual(
        TEXT("Fog presentation accounts for every map tile"),
        FogView->GetUnexploredTileCount() + FogView->GetExploredTileCount() +
            FogView->GetVisibleTileCount(),
        64 * 64);
    TestTrue(
        TEXT("The initial map contains both visible and unexplored tiles"),
        FogView->GetVisibleTileCount() > 0 &&
            FogView->GetUnexploredTileCount() > 0);
    const int32 InitialKnownTiles = FogView->GetKnownTileCount();

    const echoes::sim::Vec2 ScoutStart = echoes::sim::Vec2::FromTiles(16, 10);
    const echoes::sim::Vec2 RevealPoint = echoes::sim::Vec2::FromTiles(21, 24);
    const echoes::sim::Vec2 TargetPosition = echoes::sim::Vec2::FromTiles(25, 28);
    echoes::sim::EntityId ScoutId = 0;
    echoes::sim::EntityId TargetId = 0;
    for (const echoes::sim::Entity& Entity : InitialSimulation->Entities())
    {
        if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            Entity.type == echoes::sim::EntityType::Soldier &&
            Entity.position == ScoutStart)
        {
            ScoutId = Entity.id;
        }
        if (Entity.type == echoes::sim::EntityType::ResourceNode &&
            Entity.position == TargetPosition)
        {
            TargetId = Entity.id;
        }
    }

    if (!TestTrue(TEXT("Visibility scout was found by scenario state"), ScoutId != 0) ||
        !TestTrue(TEXT("Visibility target was found by scenario state"), TargetId != 0))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    AEchoesEntityView* ScoutView = Bridge->FindEntityView(ScoutId);
    if (TestNotNull(TEXT("Visible local scout has a presentation actor"), ScoutView))
    {
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
        TestFalse(TEXT("Full-health scout health bar starts hidden"),
                  ScoutView->IsHealthBarVisible());
        ScoutView->SetSelected(true);
        TestTrue(TEXT("Selecting a scout exposes its health bar"),
                 ScoutView->IsHealthBarVisible());
        TestTrue(TEXT("Selection uses the project-authored VFX family"),
                 ScoutView->IsUsingAuthoredSelectionVFX());
        TestTrue(TEXT("Selection VFX is visible while selected"),
                 ScoutView->IsSelectionVFXVisible());
        TestTrue(TEXT("Selection VFX has no collision or overlaps"),
                 ScoutView->HasSelectionVFXCollisionDisabled());
        TestTrue(TEXT("Selection VFX cannot affect navigation"),
                 ScoutView->HasSelectionVFXNavigationDisabled());
        const float StandardYawBeforeTick = ScoutView->GetSelectionVFXYaw();
        ScoutView->Tick(0.25f);
        TestNotEqual(TEXT("Standard selection halo rotates gently"),
                     ScoutView->GetSelectionVFXYaw(),
                     StandardYawBeforeTick);
        TestTrue(TEXT("Standard selection halo retains readable emission"),
                 ScoutView->GetSelectionVFXEmissiveStrength() > 1.7f);
        TestEqual(TEXT("Full-health scout reports a complete health fraction"),
                  ScoutView->GetDisplayedHealthFraction(),
                  1.0f);
        ScoutView->SetSelected(false);
        TestFalse(TEXT("Deselection hides the selection VFX"),
                  ScoutView->IsSelectionVFXVisible());
        if (Settings != nullptr)
        {
            Settings->SetReducedMotionEnabled(true);
            Settings->SetReducedFlashingEnabled(true);
            ScoutView->SetSelected(true);
            const float ReducedYawBeforeTick = ScoutView->GetSelectionVFXYaw();
            ScoutView->Tick(0.25f);
            TestTrue(TEXT("Reduced motion is applied to selection VFX"),
                     ScoutView->IsSelectionReducedMotionApplied());
            TestTrue(TEXT("Reduced flashing is applied to selection VFX"),
                     ScoutView->IsSelectionReducedFlashingApplied());
            TestEqual(TEXT("Reduced-motion selection halo remains steady"),
                      ScoutView->GetSelectionVFXYaw(),
                      ReducedYawBeforeTick);
            TestTrue(TEXT("Reduced-flashing selection halo uses low emission"),
                     ScoutView->GetSelectionVFXEmissiveStrength() <= 1.25f);
            ScoutView->SetSelected(false);
            Settings->SetReducedMotionEnabled(bPreviousReducedMotion);
            Settings->SetReducedFlashingEnabled(false);
        }
        TestFalse(TEXT("Deselection hides a full-health scout health bar"),
                  ScoutView->IsHealthBarVisible());
        echoes::sim::Entity DamagedScout =
            *InitialSimulation->FindEntity(ScoutId);
        DamagedScout.hitPoints = DamagedScout.maxHitPoints / 4;
        ScoutView->ApplyAuthoritativeState(DamagedScout, true);
        TestTrue(TEXT("A damaged scout exposes its health bar without selection"),
                 ScoutView->IsHealthBarVisible());
        TestEqual(TEXT("Damaged scout health fraction mirrors authoritative hit points"),
                  ScoutView->GetDisplayedHealthFraction(),
                  static_cast<float>(DamagedScout.hitPoints) /
                      static_cast<float>(DamagedScout.maxHitPoints));
        TestTrue(TEXT("Damage starts a readable presentation pulse"),
                 ScoutView->IsDamagePulseActive());
        if (Settings != nullptr)
        {
            Settings->SetReducedFlashingEnabled(true);
            echoes::sim::Entity MoreDamagedScout = DamagedScout;
            MoreDamagedScout.hitPoints -= 1;
            ScoutView->ApplyAuthoritativeState(MoreDamagedScout, true);
            TestFalse(TEXT("Reduced flashing suppresses the combat pulse"),
                      ScoutView->IsDamagePulseActive());
            Settings->SetReducedFlashingEnabled(bPreviousReducedFlashing);
            Settings->SetReducedMotionEnabled(bPreviousReducedMotion);
        }
        ScoutView->ApplyAuthoritativeState(
            *InitialSimulation->FindEntity(ScoutId),
            true);
        TestFalse(TEXT("Restored full health hides the unselected health bar"),
                  ScoutView->IsHealthBarVisible());
    }

    TestFalse(
        TEXT("Distant resource starts outside local simulation visibility"),
        InitialSimulation->IsEntityVisibleTo(
            UEchoesSimulationSubsystem::LocalPlayerId,
            TargetId));
    TestNull(
        TEXT("No presentation actor exists for the initially hidden resource"),
        Bridge->FindEntityView(TargetId));

    const auto IssueMove = [this, Bridge, ScoutId](
                               const TCHAR* Description,
                               const echoes::sim::Vec2& Destination)
    {
        FString Feedback;
        const bool bQueued = Bridge->IssueCommand(
            echoes::sim::CommandType::Move,
            ScoutId,
            0,
            Bridge->SimToWorld(Destination),
            echoes::sim::FutureWellChoice::Dormant,
            Feedback);
        if (!bQueued)
        {
            AddError(FString::Printf(TEXT("%s: %s"), Description, *Feedback));
        }
        return bQueued;
    };

    const auto TickUntil = [Bridge](const auto& Predicate, int32 MaximumTicks)
    {
        for (int32 TickIndex = 0; TickIndex < MaximumTicks; ++TickIndex)
        {
            if (Predicate())
            {
                return true;
            }
            Bridge->Tick(0.05f);
        }
        return Predicate();
    };

    if (!IssueMove(TEXT("Could not queue the reveal movement"), RevealPoint))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const auto IsTargetPresented = [Bridge, TargetId]()
    {
        const echoes::sim::Simulation* CurrentSimulation = Bridge->GetSimulation();
        return CurrentSimulation != nullptr &&
               CurrentSimulation->IsEntityVisibleTo(
                   UEchoesSimulationSubsystem::LocalPlayerId,
                   TargetId) &&
               Bridge->FindEntityView(TargetId) != nullptr;
    };
    if (!TestTrue(
            TEXT("A real entity view is created when the resource enters visibility"),
            TickUntil(IsTargetPresented, 256)))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    AEchoesEntityView* FirstView = Bridge->FindEntityView(TargetId);
    TWeakObjectPtr<AEchoesEntityView> FirstViewWeak = FirstView;
    const FEchoesPresentationPoolStats PresentedPoolStats =
        Bridge->GetPresentationPoolStats();
    TestNotNull(TEXT("First visible presentation actor is available"), FirstView);
    TestTrue(
        TEXT("Scouting expands the persistent explored map"),
        FogView->GetKnownTileCount() > InitialKnownTiles);

    if (!IssueMove(TEXT("Could not queue the retreat movement"), ScoutStart))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const auto IsTargetHidden = [Bridge, TargetId]()
    {
        const echoes::sim::Simulation* CurrentSimulation = Bridge->GetSimulation();
        return CurrentSimulation != nullptr &&
               !CurrentSimulation->IsEntityVisibleTo(
                   UEchoesSimulationSubsystem::LocalPlayerId,
                   TargetId) &&
               Bridge->FindEntityView(TargetId) == nullptr;
    };
    TestTrue(
        TEXT("The presentation actor is removed when the resource leaves visibility"),
        TickUntil(IsTargetHidden, 32));
    TestTrue(
        TEXT("A previously seen target tile becomes explored shroud"),
        Bridge->GetSimulation()->VisibilityAt(
            UEchoesSimulationSubsystem::LocalPlayerId,
            TargetPosition) == echoes::sim::Visibility::Explored);
    TestTrue(
        TEXT("The removed presentation actor remains available to the bounded pool"),
        FirstViewWeak.IsValid());
    if (FirstViewWeak.IsValid())
    {
        TestTrue(TEXT("The pooled actor is fully prepared for reuse"),
                 FirstViewWeak->IsPreparedForPool());
        TestEqual(TEXT("The pooled actor retains no entity identity"),
                  FirstViewWeak->GetEntityId(),
                  static_cast<uint32>(0));
        TestTrue(TEXT("The pooled actor is hidden"), FirstViewWeak->IsHidden());
        TestFalse(TEXT("The pooled actor cannot tick"),
                  FirstViewWeak->IsActorTickEnabled());
        TestFalse(TEXT("The pooled actor cannot be selected by collision"),
                  FirstViewWeak->HasBodySelectionCollisionEnabled());
        TestFalse(TEXT("The pooled actor retains no selection"),
                  FirstViewWeak->IsSelected());
        TestFalse(TEXT("The pooled actor retains no damage pulse"),
                  FirstViewWeak->IsDamagePulseActive());
        TestFalse(TEXT("The pooled actor retains no Future Well presentation"),
                  FirstViewWeak->IsFutureWellPresentationVisible());
    }
    TestTrue(
        TEXT("Visibility retirement records a free pooled view"),
        Bridge->GetPresentationPoolStats().FreeEntityViews > 0);

    if (!IssueMove(TEXT("Could not queue the visibility reentry movement"), RevealPoint))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    if (!TestTrue(
            TEXT("The resource view is recreated when visibility returns"),
            TickUntil(IsTargetPresented, 32)))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    AEchoesEntityView* ReenteredView = Bridge->FindEntityView(TargetId);
    TestNotNull(TEXT("Reentered presentation actor is available"), ReenteredView);
    const FEchoesPresentationPoolStats ReenteredPoolStats =
        Bridge->GetPresentationPoolStats();
    TestEqual(
        TEXT("Visibility reentry does not allocate another entity actor"),
        ReenteredPoolStats.EntityCreated,
        PresentedPoolStats.EntityCreated);
    TestTrue(
        TEXT("Visibility reentry records deterministic pool reuse"),
        ReenteredPoolStats.EntityReused > PresentedPoolStats.EntityReused);
    if (ReenteredView != nullptr)
    {
        TestEqual(
            TEXT("Reentered actor remains bound to the authoritative entity"),
            ReenteredView->GetEntityId(),
            TargetId);
    }

    int32 LiveTargetViewCount = 0;
    for (TActorIterator<AEchoesEntityView> It(World); It; ++It)
    {
        if (!It->IsActorBeingDestroyed() && It->GetEntityId() == TargetId)
        {
            ++LiveTargetViewCount;
        }
    }
    TestEqual(
        TEXT("Exactly one live actor presents the reentered entity"),
        LiveTargetViewCount,
        1);

    // FTestWorldWrapper removes its world context before DestroyWorld invokes
    // subsystem teardown. Stop the scenario while the context is still valid;
    // the later subsystem teardown is intentionally idempotent.
    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors() && !WorldWrapper.HasFailed();
}

#endif
