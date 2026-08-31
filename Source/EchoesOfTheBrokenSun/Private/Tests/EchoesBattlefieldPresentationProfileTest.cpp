#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesBattlefieldPresentation.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesTerrainView.h"
#include "EchoesTestSaveEnvironment.h"
#include "EchoesWeatherView.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
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
            EchoesBattlefieldPresentation::RegisterPresetActorTags(
                PresentationProbes[Index]->Tags,
                Presets[Index]);
        }
    }
    AStaticMeshActor* SharedFloor = World->SpawnActor<AStaticMeshActor>();
    AActor* SharedSun = World->SpawnActor<AActor>();
    AActor* SharedSky = World->SpawnActor<AActor>();
    AActor* LegacyGlassScarProbe = World->SpawnActor<AActor>();
    AActor* MalformedRootOnlyProbe = World->SpawnActor<AActor>();
    UStaticMeshComponent* SharedFloorMesh = SharedFloor != nullptr
        ? SharedFloor->GetStaticMeshComponent()
        : nullptr;
    if (SharedFloor != nullptr)
    {
        EchoesBattlefieldPresentation::RegisterSharedActorTags(
            SharedFloor->Tags,
            EchoesBattlefieldPresentation::FloorTag());
        SharedFloor->SetActorEnableCollision(true);
    }
    if (SharedFloorMesh != nullptr)
    {
        SharedFloorMesh->SetCollisionEnabled(
            ECollisionEnabled::QueryAndPhysics);
    }
    if (SharedSun != nullptr)
    {
        EchoesBattlefieldPresentation::RegisterSharedActorTags(
            SharedSun->Tags,
            EchoesBattlefieldPresentation::SunTag());
    }
    if (SharedSky != nullptr)
    {
        EchoesBattlefieldPresentation::RegisterSharedActorTags(
            SharedSky->Tags,
            EchoesBattlefieldPresentation::SkyTag());
    }
    if (LegacyGlassScarProbe != nullptr)
    {
        LegacyGlassScarProbe->Tags.Add(
            EchoesBattlefieldPresentation::LegacyGlassScarTag());
    }
    if (MalformedRootOnlyProbe != nullptr)
    {
        MalformedRootOnlyProbe->Tags.Add(
            EchoesBattlefieldPresentation::RootTag());
    }
    AEchoesWeatherView* Weather = World->SpawnActor<AEchoesWeatherView>();
    AEchoesWeatherView* MalformedWeather =
        World->SpawnActor<AEchoesWeatherView>();
    if (Weather != nullptr)
    {
        EchoesBattlefieldPresentation::RegisterSharedActorTags(
            Weather->Tags,
            EchoesBattlefieldPresentation::WeatherTag());
    }
    if (MalformedWeather != nullptr)
    {
        MalformedWeather->Tags.Add(
            EchoesBattlefieldPresentation::RootTag());
        MalformedWeather->Tags.Add(
            EchoesBattlefieldPresentation::WeatherTag());
    }
    if (!TestNotNull(TEXT("Glass Scar presentation probe exists"),
                     PresentationProbes[0]) ||
        !TestNotNull(TEXT("Crownfall presentation probe exists"),
                     PresentationProbes[1]) ||
        !TestNotNull(TEXT("Soryn presentation probe exists"),
                     PresentationProbes[2]) ||
        !TestNotNull(TEXT("Shared floor probe exists"), SharedFloor) ||
        !TestNotNull(TEXT("Shared floor mesh component exists"),
                     SharedFloorMesh) ||
        !TestNotNull(TEXT("Shared sun probe exists"), SharedSun) ||
        !TestNotNull(TEXT("Shared sky probe exists"), SharedSky) ||
        !TestNotNull(TEXT("Legacy Glass Scar probe exists"),
                     LegacyGlassScarProbe) ||
        !TestNotNull(TEXT("Malformed root-only probe exists"),
                     MalformedRootOnlyProbe) ||
        !TestNotNull(TEXT("Registered map-aware weather view exists"),
                     Weather) ||
        !TestNotNull(TEXT("Malformed weather probe exists"),
                     MalformedWeather))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    float FogDensities[UE_ARRAY_COUNT(Presets)] = {};
    const auto VerifyProfile =
        [this, Bridge, Weather, MalformedWeather, SharedFloor,
         SharedFloorMesh, SharedSun, SharedSky, LegacyGlassScarProbe,
         MalformedRootOnlyProbe, &PresentationProbes, &FogDensities, &Presets](
            EEchoesSkirmishMapPreset Preset,
            int32 PresetIndex,
            const TCHAR* Label)
    {
        AEchoesTerrainView* Terrain = Bridge->GetTerrainView();
        bool bPassed = true;
        bPassed &= TestTrue(
            FString::Printf(TEXT("%s: active setup matches"), Label),
            Bridge->GetActiveSkirmishSetup().MapPreset == Preset);
        bPassed &= TestNotNull(
            FString::Printf(TEXT("%s: terrain view exists"), Label),
            Terrain);
        if (Terrain != nullptr)
        {
            bPassed &= TestTrue(
                FString::Printf(TEXT("%s: terrain preset matches"), Label),
                Terrain->GetMapPreset() == Preset);
            bPassed &= TestTrue(
                FString::Printf(
                    TEXT("%s: authored terrain meshes remain active"),
                    Label),
                Terrain->IsUsingAuthoredTerrainMeshes());
            bPassed &= TestEqual(
                FString::Printf(
                    TEXT("%s: blocked-tile presentation count matches"),
                    Label),
                Terrain->GetBlockedTileCount(),
                FEchoesSkirmishSetupModel::ExpectedBlockedTileCount(Preset));
        }
        bPassed &= TestTrue(
            FString::Printf(TEXT("%s: registered weather preset matches"),
                            Label),
            Weather->GetMapPreset() == Preset);
        bPassed &= TestTrue(
            FString::Printf(TEXT("%s: registered weather density is valid"),
                            Label),
            Weather->GetCurrentFogDensity() > 0.0f);
        bPassed &= TestTrue(
            FString::Printf(
                TEXT("%s: malformed weather remains untouched"),
                Label),
            MalformedWeather->GetMapPreset() ==
                EEchoesSkirmishMapPreset::GlassScar);
        bPassed &= TestTrue(
            FString::Printf(
                TEXT("%s: malformed weather fails closed"),
                Label),
            MalformedWeather->IsHidden());
        for (int32 Index = 0; Index < UE_ARRAY_COUNT(Presets); ++Index)
        {
            bPassed &= TestEqual(
                FString::Printf(
                    TEXT("%s: %s scoped actor visibility matches"),
                    Label,
                    EchoesBattlefieldPresentation::StableName(Presets[Index])),
                PresentationProbes[Index]->IsHidden(),
                Index != PresetIndex);
        }
        bPassed &= TestFalse(
            FString::Printf(TEXT("%s: shared floor remains visible"), Label),
            SharedFloor->IsHidden());
        bPassed &= TestTrue(
            FString::Printf(TEXT("%s: shared floor collision remains enabled"),
                            Label),
            SharedFloor->GetActorEnableCollision());
        bPassed &= TestEqual(
            FString::Printf(
                TEXT("%s: shared floor primitive collision remains enabled"),
                Label),
            SharedFloorMesh->GetCollisionEnabled(),
            ECollisionEnabled::QueryAndPhysics);
        bPassed &= TestFalse(
            FString::Printf(TEXT("%s: shared sun remains visible"), Label),
            SharedSun->IsHidden());
        bPassed &= TestFalse(
            FString::Printf(TEXT("%s: shared sky remains visible"), Label),
            SharedSky->IsHidden());
        bPassed &= TestEqual(
            FString::Printf(
                TEXT("%s: legacy Glass Scar visibility remains compatible"),
                Label),
            LegacyGlassScarProbe->IsHidden(),
            Preset != EEchoesSkirmishMapPreset::GlassScar);
        bPassed &= TestTrue(
            FString::Printf(
                TEXT("%s: malformed root-only actor fails closed"),
                Label),
            MalformedRootOnlyProbe->IsHidden());
        FogDensities[PresetIndex] = Weather != nullptr
            ? Weather->GetCurrentFogDensity()
            : 0.0f;
        return bPassed;
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
    const bool bRestarted = Bridge->RestartPrototypeScenario();
    TestTrue(TEXT("Crownfall profile restarts"), bRestarted);
    if (bRestarted)
    {
        VerifyProfile(
            EEchoesSkirmishMapPreset::CrownfallBasin,
            1,
            TEXT("CROWNFALL_RESTART"));
    }
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
    const bool bQuickLoaded = Bridge->QuickLoadScenario(Feedback);
    TestTrue(TEXT("Quickload restores the saved map setup"), bQuickLoaded);
    if (bQuickLoaded)
    {
        VerifyProfile(
            EEchoesSkirmishMapPreset::CrownfallBasin,
            1,
            TEXT("CROWNFALL_QUICKLOAD"));
    }

    for (AActor* Probe : PresentationProbes)
    {
        if (Probe != nullptr)
        {
            Probe->Destroy();
        }
    }
    SharedFloor->Destroy();
    SharedSun->Destroy();
    SharedSky->Destroy();
    LegacyGlassScarProbe->Destroy();
    MalformedRootOnlyProbe->Destroy();
    Weather->Destroy();
    MalformedWeather->Destroy();
    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors() && !WorldWrapper.HasFailed();
}

#endif
