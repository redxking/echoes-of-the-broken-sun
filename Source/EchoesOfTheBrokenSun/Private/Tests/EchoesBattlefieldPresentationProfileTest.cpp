#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesBattlefieldPresentation.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesTerrainView.h"
#include "EchoesTestSaveEnvironment.h"
#include "EchoesWeatherView.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Tests/AutomationCommon.h"

namespace
{
struct FEchoesIdentityColor final
{
    const TCHAR* Label = TEXT("");
    FLinearColor Color = FLinearColor::Black;
};

/**
 * Mirror of the identity palette that terrain accents must never approach:
 * owner colors, HollowChoir, ResourceNode, FutureWell choices, and the
 * temporary mineral cover from EchoesEntityView.cpp ColorForState, plus the
 * command-marker base colors from EchoesCommandMarkerView.cpp
 * InitializeMarker. If a production identity color changes, update this
 * table in the same slice.
 */
const FEchoesIdentityColor IdentityColors[] = {
    {TEXT("owner-0"), FLinearColor(0.04f, 0.72f, 0.88f)},
    {TEXT("owner-1"), FLinearColor(0.92f, 0.30f, 0.05f)},
    {TEXT("owner-2"), FLinearColor(0.95f, 0.74f, 0.08f)},
    {TEXT("owner-3"), FLinearColor(0.62f, 0.30f, 0.95f)},
    {TEXT("owner-neutral"), FLinearColor(0.72f, 0.72f, 0.72f)},
    {TEXT("hollow-choir"), FLinearColor(0.788f, 0.824f, 0.941f)},
    {TEXT("resource-node"), FLinearColor(0.95f, 0.56f, 0.08f)},
    {TEXT("well-harvest"), FLinearColor(1.0f, 0.43f, 0.05f)},
    {TEXT("well-preserve"), FLinearColor(0.12f, 0.86f, 0.44f)},
    {TEXT("well-reshape"), FLinearColor(0.95f, 0.08f, 0.16f)},
    {TEXT("well-dormant"), FLinearColor(0.62f, 0.18f, 1.0f)},
    {TEXT("mineral-cover"), FLinearColor(0.42f, 0.28f, 0.16f)},
    {TEXT("marker-move"), FLinearColor(0.05f, 0.92f, 1.0f)},
    {TEXT("marker-attack"), FLinearColor(1.0f, 0.12f, 0.04f)},
    {TEXT("marker-attack-move"), FLinearColor(1.0f, 0.34f, 0.04f)},
    {TEXT("marker-patrol"), FLinearColor(0.76f, 0.24f, 1.0f)},
    {TEXT("marker-guard"), FLinearColor(0.20f, 1.0f, 0.42f)},
    {TEXT("marker-build"), FLinearColor(0.98f, 0.84f, 0.22f)},
    {TEXT("marker-interact"), FLinearColor(0.32f, 0.95f, 0.82f)}};

constexpr float MinimumIdentitySeparation = 0.30f;

/** Brightness-independent chromaticity: each channel divided by the max. */
[[nodiscard]] FVector NormalizedChromaticity(const FLinearColor& Color)
{
    const float MaxChannel = FMath::Max3(Color.R, Color.G, Color.B);
    if (MaxChannel <= KINDA_SMALL_NUMBER)
    {
        return FVector::ZeroVector;
    }
    return FVector(
        Color.R / MaxChannel,
        Color.G / MaxChannel,
        Color.B / MaxChannel);
}

[[nodiscard]] float ChromaticitySeparation(
    const FLinearColor& First,
    const FLinearColor& Second)
{
    return static_cast<float>(FVector::Distance(
        NormalizedChromaticity(First),
        NormalizedChromaticity(Second)));
}

/**
 * Reads the slot-3 accent Color parameter from the named instanced tile
 * layer of the real terrain actor.
 */
[[nodiscard]] bool ReadAccentColor(
    const AEchoesTerrainView& Terrain,
    const TCHAR* ComponentName,
    FLinearColor& OutColor)
{
    TArray<UInstancedStaticMeshComponent*> TileLayers;
    Terrain.GetComponents<UInstancedStaticMeshComponent>(TileLayers);
    for (const UInstancedStaticMeshComponent* Layer : TileLayers)
    {
        if (Layer == nullptr || Layer->GetName() != ComponentName)
        {
            continue;
        }
        const UMaterialInstanceDynamic* AccentMaterial =
            Cast<UMaterialInstanceDynamic>(Layer->GetMaterial(3));
        return AccentMaterial != nullptr &&
               AccentMaterial->GetVectorParameterValue(
                   FHashedMaterialParameterInfo(TEXT("Color")),
                   OutColor);
    }
    return false;
}
}

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
    FLinearColor BlockedAccents[UE_ARRAY_COUNT(Presets)] = {
        FLinearColor::Black, FLinearColor::Black, FLinearColor::Black};
    FLinearColor ScarredAccents[UE_ARRAY_COUNT(Presets)] = {
        FLinearColor::Black, FLinearColor::Black, FLinearColor::Black};
    const auto VerifyProfile =
        [this, Bridge, Weather, MalformedWeather, SharedFloor,
         SharedFloorMesh, SharedSun, SharedSky, LegacyGlassScarProbe,
         MalformedRootOnlyProbe, &PresentationProbes, &FogDensities,
         &BlockedAccents, &ScarredAccents, &Presets](
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
                    TEXT("%s: real terrain owns the presentation root tag"),
                    Label),
                Terrain->ActorHasTag(
                    EchoesBattlefieldPresentation::RootTag()));
            bPassed &= TestTrue(
                FString::Printf(
                    TEXT("%s: real terrain owns the selected preset tag"),
                    Label),
                Terrain->ActorHasTag(
                    EchoesBattlefieldPresentation::TagForPreset(Preset)));
            for (int32 Index = 0; Index < UE_ARRAY_COUNT(Presets); ++Index)
            {
                if (Index == PresetIndex)
                {
                    continue;
                }
                bPassed &= TestFalse(
                    FString::Printf(
                        TEXT("%s: real terrain lacks non-selected %s tag"),
                        Label,
                        EchoesBattlefieldPresentation::StableName(
                            Presets[Index])),
                    Terrain->ActorHasTag(
                        EchoesBattlefieldPresentation::TagForPreset(
                            Presets[Index])));
            }
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
            const struct
            {
                const TCHAR* LayerName;
                FLinearColor* Store;
            } AccentLayers[] = {
                {TEXT("BlockedTiles"), &BlockedAccents[PresetIndex]},
                {TEXT("ScarredTiles"), &ScarredAccents[PresetIndex]}};
            for (const auto& AccentLayer : AccentLayers)
            {
                FLinearColor Accent = FLinearColor::Black;
                const bool bAccentRead = ReadAccentColor(
                    *Terrain,
                    AccentLayer.LayerName,
                    Accent);
                bPassed &= TestTrue(
                    FString::Printf(
                        TEXT("%s: %s exposes its accent color"),
                        Label,
                        AccentLayer.LayerName),
                    bAccentRead);
                if (!bAccentRead)
                {
                    continue;
                }
                *AccentLayer.Store = Accent;
                for (const FEchoesIdentityColor& Identity : IdentityColors)
                {
                    const float Separation =
                        ChromaticitySeparation(Accent, Identity.Color);
                    bPassed &= TestTrue(
                        FString::Printf(
                            TEXT("%s: %s accent keeps chromatic distance ")
                            TEXT("%.3f >= %.2f from %s"),
                            Label,
                            AccentLayer.LayerName,
                            Separation,
                            MinimumIdentitySeparation,
                            Identity.Label),
                        Separation >= MinimumIdentitySeparation);
                }
            }
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
        // Every site now has authored ground. The Engine cube remains hidden
        // while its pointer-trace collision stays unchanged.
        const AEchoesTerrainView* ProfileTerrain =
            Bridge != nullptr ? Bridge->GetTerrainView() : nullptr;
        const bool bFloorSurfaceReplaced =
            ProfileTerrain != nullptr;
        bPassed &= TestEqual(
            FString::Printf(
                TEXT("%s: shared Engine floor hides under authored site ground"),
                Label),
            SharedFloor->IsHidden(),
            bFloorSurfaceReplaced);
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
    for (int32 FirstIndex = 0; FirstIndex < UE_ARRAY_COUNT(Presets);
         ++FirstIndex)
    {
        for (int32 SecondIndex = FirstIndex + 1;
             SecondIndex < UE_ARRAY_COUNT(Presets); ++SecondIndex)
        {
            TestTrue(
                FString::Printf(
                    TEXT("%s and %s keep distinct blocked accents"),
                    EchoesBattlefieldPresentation::StableName(
                        Presets[FirstIndex]),
                    EchoesBattlefieldPresentation::StableName(
                        Presets[SecondIndex])),
                ChromaticitySeparation(
                    BlockedAccents[FirstIndex],
                    BlockedAccents[SecondIndex]) >=
                    MinimumIdentitySeparation);
            TestTrue(
                FString::Printf(
                    TEXT("%s and %s keep distinct scarred accents"),
                    EchoesBattlefieldPresentation::StableName(
                        Presets[FirstIndex]),
                    EchoesBattlefieldPresentation::StableName(
                        Presets[SecondIndex])),
                ChromaticitySeparation(
                    ScarredAccents[FirstIndex],
                    ScarredAccents[SecondIndex]) >=
                    MinimumIdentitySeparation);
        }
    }

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
