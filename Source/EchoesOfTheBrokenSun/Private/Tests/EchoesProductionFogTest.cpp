#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "EchoesFogView.h"
#include "EchoesGameUserSettings.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesSimCore/Simulation.h"
#include "Engine/World.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "HAL/PlatformTime.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesProductionFogTest,
    "Echoes.Runtime.Presentation.ProductionFog",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesProductionFogTest::RunTest(const FString& Parameters)
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
        AddError(TEXT("Could not create the production fog test world."));
        return false;
    }
    UWorld* World = WorldWrapper.GetTestWorld();
    if (!TestNotNull(TEXT("Production fog test world is available"), World))
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
    // 1. AEchoesFogView Invariants (Collision, Shadows, Navigation, Overlaps)
    // -------------------------------------------------------------------------
    AEchoesFogView* FogView = World->SpawnActor<AEchoesFogView>();
    if (TestNotNull(TEXT("Fog actor spawns successfully"), FogView))
    {
        TestTrue(TEXT("Fog initializes 64x64 scoped grid"),
                 FogView->InitializeScopedFog(64, 64, 200.0f));

        TestTrue(TEXT("Fog has all collision and traces disabled"),
                 FogView->HasCollisionDisabled());
        TestTrue(TEXT("Fog cannot affect navigation mesh generation"),
                 FogView->HasNavigationDisabled());
        TestTrue(TEXT("Fog has dynamic shadows disabled"),
                 FogView->HasShadowsDisabled());
        TestTrue(TEXT("Fog has overlap events disabled"),
                 FogView->HasOverlapsDisabled());

        // ---------------------------------------------------------------------
        // 2. Visual Palette and Material Signatures
        // ---------------------------------------------------------------------
        const FLinearColor UnexploredBase = FogView->GetUnexploredBaseColor();
        const FLinearColor UnexploredBleed = FogView->GetUnexploredBleedColor();
        const FLinearColor ExploredColor = FogView->GetExploredColor();

        TestTrue(TEXT("Unexplored base is deep charcoal basalt"),
                 UnexploredBase.R <= 0.02f && UnexploredBase.G <= 0.02f && UnexploredBase.B <= 0.02f);
        TestTrue(TEXT("Unexplored bleed carries magenta/Possibility spectral tone"),
                 UnexploredBleed.R >= 0.04f && UnexploredBleed.B >= 0.05f);
        TestTrue(TEXT("Explored color is distinct from unexplored base"),
                 !ExploredColor.Equals(UnexploredBase));
        TestTrue(TEXT("Explored color carries desaturated memory tone"),
                 ExploredColor.B >= ExploredColor.R);

        // ---------------------------------------------------------------------
        // 3. Accessibility Compliance (Reduced Motion & Reduced Flashing)
        // ---------------------------------------------------------------------
        FogView->UpdateAccessibilitySettings(true, true);
        TestTrue(TEXT("Reduced motion setting applied"),
                 FogView->IsReducedMotionApplied());
        TestTrue(TEXT("Reduced flashing setting applied"),
                 FogView->IsReducedFlashingApplied());

        FogView->UpdateAccessibilitySettings(false, false);
        TestFalse(TEXT("Reduced motion reset"),
                  FogView->IsReducedMotionApplied());
        TestFalse(TEXT("Reduced flashing reset"),
                  FogView->IsReducedFlashingApplied());

        const FTransform LegacyUnknown = FogView->TileTransform(10, 10, true);
        TestEqual(TEXT("Other maps retain the original shroud bottom"),
            LegacyUnknown.GetLocation().Z - 50.0f * LegacyUnknown.GetScale3D().Z, -16.0);
        TestTrue(TEXT("Other maps retain the tall opaque shroud"),
            LegacyUnknown.GetLocation().Z + 50.0f * LegacyUnknown.GetScale3D().Z >= 184.0f);
        TestTrue(TEXT("M01 knowledge surface initializes through its explicit profile"),
            FogView->InitializeScopedFog(64, 64, 200.0f, true));
        const FTransform M01Unknown = FogView->TileTransform(10, 10, true);
        TestEqual(TEXT("M01 shroud preserves the exact tile XY footprint"),
            FVector2D(M01Unknown.GetLocation()), FVector2D(LegacyUnknown.GetLocation()));
        TestEqual(TEXT("M01 retains the required full occlusion height"),
            M01Unknown.GetLocation().Z + 50.0f * M01Unknown.GetScale3D().Z, 184.0);
        TestEqual(TEXT("M01 conceals unknown ravines down to the authored bed"),
            M01Unknown.GetLocation().Z - 50.0f * M01Unknown.GetScale3D().Z, -1766.0);
        TestTrue(TEXT("M01 unknown material has no scene-lighting response"),
            FogView->UnexploredMaterial != nullptr &&
            FogView->UnexploredMaterial->GetMaterial()->GetShadingModels().HasShadingModel(MSM_Unlit));
        TestTrue(TEXT("M01 unknown remains opaque"),
            FogView->UnexploredMaterial != nullptr &&
            FogView->UnexploredMaterial->GetBlendMode() == BLEND_Opaque);
        TestTrue(TEXT("M01 remembered terrain uses a translucent tint"),
            FogView->ExploredMaterial != nullptr &&
            FogView->ExploredMaterial->GetBlendMode() == BLEND_Translucent);
        FogView->Destroy();
    }

    // -------------------------------------------------------------------------
    // 4. Subsystem Visibility Accounting & Performance Budget (<= 1.5 ms)
    // -------------------------------------------------------------------------
    UEchoesSimulationSubsystem* Bridge =
        World->GetSubsystem<UEchoesSimulationSubsystem>();
    if (TestNotNull(TEXT("World owns simulation subsystem"), Bridge) &&
        TestTrue(TEXT("Prototype scenario starts"), Bridge->StartPrototypeScenario()))
    {
        AEchoesFogView* SubsystemFog = Bridge->GetFogView();
        if (TestNotNull(TEXT("Subsystem fog view is available"), SubsystemFog))
        {
            const int32 TotalTiles = SubsystemFog->GetUnexploredTileCount() +
                                     SubsystemFog->GetExploredTileCount() +
                                     SubsystemFog->GetVisibleTileCount();
            TestEqual(TEXT("Fog accounts for exactly 4,096 map tiles"),
                      TotalTiles,
                      64 * 64);

            TestTrue(TEXT("Initial scenario has unexplored tiles"),
                     SubsystemFog->GetUnexploredTileCount() > 0);
            TestTrue(TEXT("Initial scenario has visible tiles around bases"),
                     SubsystemFog->GetVisibleTileCount() > 0);

            // Performance timing check: last sync duration must be <= 1.5 ms budget
            const double LastDuration = SubsystemFog->GetLastSyncDurationMs();
            TestTrue(TEXT("Fog synchronization is within the 1.5 ms budget"),
                     LastDuration <= 1.5);

            // Measure 20 consecutive simulation sync cycles under active scenario
            double MaxSyncDuration = 0.0;
            for (int32 Step = 0; Step < 20; ++Step)
            {
                Bridge->Tick(0.05f);
                const double Duration = SubsystemFog->GetLastSyncDurationMs();
                if (Duration > MaxSyncDuration)
                {
                    MaxSyncDuration = Duration;
                }
            }

            TestTrue(TEXT("Peak incremental fog sync across 20 ticks stays within 1.5 ms budget"),
                     MaxSyncDuration <= 1.5);
            TestTrue(TEXT("Incremental sync takes less than 0.2 ms on average"),
                     MaxSyncDuration < 0.8);
        }

        Bridge->StopPrototypeScenario();
    }

    // -------------------------------------------------------------------------
    // 5. Zero Simulation Touch (SIM-002 Determinism Invariant)
    // -------------------------------------------------------------------------
    {
        echoes::sim::SimulationConfig Config{64, 64, 20, 0x5EEDULL};
        echoes::sim::Simulation SimA(Config);
        SimA.AddPlayer(0, echoes::sim::Faction::MeridianCompact, {500, 50});
        SimA.AddPlayer(1, echoes::sim::Faction::KharuunAssemblies, {500, 50});

        SimA.SpawnEntity(0, echoes::sim::Faction::MeridianCompact, echoes::sim::EntityType::Soldier, echoes::sim::Vec2::FromTiles(10, 10));
        SimA.SpawnEntity(0, echoes::sim::Faction::MeridianCompact, echoes::sim::EntityType::ScoutUnit, echoes::sim::Vec2::FromTiles(12, 12));
        SimA.SpawnEntity(1, echoes::sim::Faction::KharuunAssemblies, echoes::sim::EntityType::Soldier, echoes::sim::Vec2::FromTiles(40, 40));

        // Exact clone for comparison
        echoes::sim::Simulation SimB = SimA;
        TestEqual(TEXT("Initial simulation checksums match exactly"),
                  SimA.StateChecksum(),
                  SimB.StateChecksum());

        // Spawn a standalone fog view synchronized with SimB
        AEchoesFogView* SimBFog = World->SpawnActor<AEchoesFogView>();
        if (SimBFog != nullptr)
        {
            SimBFog->InitializeFog(SimB, 0, 200.0f);
        }

        // Step both simulations for 40 ticks
        for (int32 Step = 0; Step < 40; ++Step)
        {
            SimA.Step();
            SimB.Step();

            if (SimBFog != nullptr)
            {
                SimBFog->SyncVisibility(SimB);
            }
        }

        TestEqual(TEXT("Simulation checksums remain 100% identical after 40 ticks with active fog sync (SIM-002)"),
                  SimA.StateChecksum(),
                  SimB.StateChecksum());
        TestEqual(TEXT("Simulation current ticks match"),
                  SimA.CurrentTick(),
                  SimB.CurrentTick());
        TestEqual(TEXT("Simulation entity counts match"),
                  SimA.Entities().size(),
                  SimB.Entities().size());

        if (SimBFog != nullptr)
        {
            SimBFog->Destroy();
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
