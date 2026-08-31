#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesBattlefieldPresentation.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesTerrainView.h"
#include "EchoesTestSaveEnvironment.h"
#include "EchoesWeatherView.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesBattlefieldPresentationProfileTest,
    "Echoes.Runtime.Map.PresentationProfiles",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesBattlefieldPresentationProfileTest::RunTest(
    const FString& Parameters)
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
        AddError(TEXT("Could not create the battlefield-profile test world."));
        return false;
    }

    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge = World != nullptr
        ? World->GetSubsystem<UEchoesSimulationSubsystem>()
        : nullptr;
    if (!TestNotNull(TEXT("Profile world owns the simulation subsystem"), Bridge) ||
        !TestTrue(TEXT("Profile fixture starts"),
                  Bridge != nullptr && Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const EEchoesSkirmishMapPreset Presets[] = {
        EEchoesSkirmishMapPreset::GlassScar,
        EEchoesSkirmishMapPreset::CrownfallBasin,
        EEchoesSkirmishMapPreset::SorynConfluence};
    AActor* PresentationProbes[UE_ARRAY_COUNT(Presets)] = {};
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Presets); ++Index)
    {
        PresentationProbes[Index] = World->SpawnActor<AActor>();
        if (PresentationProbes[Index] != nullptr)
        {
            PresentationProbes[Index]->Tags.Add(
                EchoesBattlefieldPresentation::RootTag());
            PresentationProbes[Index]->Tags.Add(
                EchoesBattlefieldPresentation::TagForPreset(Presets[Index]));
        }
    }
    AEchoesWeatherView* Weather = World->SpawnActor<AEchoesWeatherView>();
    if (!TestNotNull(TEXT("Glass Scar presentation probe exists"),
                     PresentationProbes[0]) ||
        !TestNotNull(TEXT("Crownfall presentation probe exists"),
                     PresentationProbes[1]) ||
        !TestNotNull(TEXT("Soryn presentation probe exists"),
                     PresentationProbes[2]) ||
        !TestNotNull(TEXT("Map-aware weather view exists"), Weather))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    float FogDensities[UE_ARRAY_COUNT(Presets)] = {};
    const auto VerifyProfile =
        [this, Bridge, Weather, &PresentationProbes, &FogDensities,
         &Presets](EEchoesSkirmishMapPreset Preset, int32 PresetIndex,
                   const TCHAR* Label)
    {
        AEchoesTerrainView* Terrain = Bridge->GetTerrainView();
        bool bVisibilityExact = true;
        for (int32 Index = 0; Index < UE_ARRAY_COUNT(Presets); ++Index)
        {
            bVisibilityExact &= PresentationProbes[Index] != nullptr &&
                PresentationProbes[Index]->IsHidden() ==
                    (Index != PresetIndex);
        }
        FogDensities[PresetIndex] = Weather != nullptr
            ? Weather->GetCurrentFogDensity()
            : 0.0f;
        return TestTrue(
            FString::Printf(TEXT("%s profile is selected atomically"), Label),
            Bridge->GetActiveSkirmishSetup().MapPreset == Preset &&
                Terrain != nullptr && Terrain->GetMapPreset() == Preset &&
                Terrain->IsUsingAuthoredTerrainMeshes() &&
                Terrain->GetBlockedTileCount() ==
                    FEchoesSkirmishSetupModel::ExpectedBlockedTileCount(Preset) &&
                Weather != nullptr && Weather->GetMapPreset() == Preset &&
                Weather->GetCurrentFogDensity() > 0.0f &&
                bVisibilityExact);
    };

    FString Feedback;
    // Begin with a different preset so the existing-active fast path cannot
    // hide a missing runtime synchronization step in this fixture.
    const int32 ApplyOrder[] = {1, 2, 0};
    for (const int32 Index : ApplyOrder)
    {
        FEchoesSkirmishSetup Setup =
            FEchoesSkirmishSetupModel::DefaultSetup();
        Setup.MapPreset = Presets[Index];
        Feedback.Reset();
        TestTrue(
            FString::Printf(
                TEXT("%s setup applies"),
                EchoesBattlefieldPresentation::StableName(Presets[Index])),
            Bridge->ApplySkirmishSetup(Setup, Feedback));
        VerifyProfile(
            Presets[Index],
            Index,
            EchoesBattlefieldPresentation::StableName(Presets[Index]));
    }
    TestTrue(
        TEXT("Each map owns a distinct atmospheric density profile"),
        !FMath::IsNearlyEqual(FogDensities[0], FogDensities[1], 0.000001f) &&
            !FMath::IsNearlyEqual(FogDensities[0], FogDensities[2], 0.000001f) &&
            !FMath::IsNearlyEqual(FogDensities[1], FogDensities[2], 0.000001f));

    FEchoesSkirmishSetup CrownfallSetup =
        FEchoesSkirmishSetupModel::DefaultSetup();
    CrownfallSetup.MapPreset = EEchoesSkirmishMapPreset::CrownfallBasin;
    Feedback.Reset();
    TestTrue(TEXT("Crownfall can be restored for lifecycle coverage"),
             Bridge->ApplySkirmishSetup(CrownfallSetup, Feedback));
    TestTrue(TEXT("Crownfall profile survives restart"),
             Bridge->RestartPrototypeScenario() &&
                 VerifyProfile(
                     EEchoesSkirmishMapPreset::CrownfallBasin,
                     1,
                     TEXT("CROWNFALL_RESTART")));
    Feedback.Reset();
    TestTrue(TEXT("Crownfall profile checkpoint saves"),
             Bridge->QuickSaveScenario(Feedback));

    FEchoesSkirmishSetup SorynSetup =
        FEchoesSkirmishSetupModel::DefaultSetup();
    SorynSetup.MapPreset = EEchoesSkirmishMapPreset::SorynConfluence;
    Feedback.Reset();
    TestTrue(TEXT("Soryn can replace the saved profile"),
             Bridge->ApplySkirmishSetup(SorynSetup, Feedback));
    Feedback.Reset();
    TestTrue(TEXT("Quickload restores terrain, weather, and composition profile"),
             Bridge->QuickLoadScenario(Feedback) &&
                 VerifyProfile(
                     EEchoesSkirmishMapPreset::CrownfallBasin,
                     1,
                     TEXT("CROWNFALL_QUICKLOAD")));

    for (AActor* Probe : PresentationProbes)
    {
        if (Probe != nullptr)
        {
            Probe->Destroy();
        }
    }
    Weather->Destroy();
    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors() && !WorldWrapper.HasFailed();
}

#endif
