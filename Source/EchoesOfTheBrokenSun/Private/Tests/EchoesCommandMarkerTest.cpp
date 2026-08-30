#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/StaticMeshComponent.h"
#include "EchoesCommandMarkerView.h"
#include "EchoesSimCore/Simulation.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesCommandMarkerTest,
    "Echoes.Runtime.Presentation.CommandMarkers",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesCommandMarkerTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the command-marker test world."));
        return false;
    }

    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge =
        World != nullptr
            ? World->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (!TestNotNull(TEXT("Marker world owns the simulation subsystem"), Bridge) ||
        !TestTrue(TEXT("Marker scenario starts"),
                  Bridge != nullptr && Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const uint64 AuthoritativeChecksum = Bridge->GetSimulation()->StateChecksum();
    const EEchoesCommandMarkerType MarkerTypes[] = {
        EEchoesCommandMarkerType::Move,
        EEchoesCommandMarkerType::Attack,
        EEchoesCommandMarkerType::AttackMove,
        EEchoesCommandMarkerType::Patrol,
        EEchoesCommandMarkerType::Guard,
        EEchoesCommandMarkerType::Build,
        EEchoesCommandMarkerType::Interact,
    };
    FString DirectAttackMeshPath;
    FString AttackMoveMeshPath;
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(MarkerTypes); ++Index)
    {
        FActorSpawnParameters SpawnParameters;
        SpawnParameters.ObjectFlags |= RF_Transient;
        AEchoesCommandMarkerView* Marker =
            World->SpawnActor<AEchoesCommandMarkerView>(
                FVector(static_cast<float>(Index) * 150.0f, 0.0f, 8.0f),
                FRotator::ZeroRotator,
                SpawnParameters);
        if (!TestNotNull(
                *FString::Printf(TEXT("Marker %d spawns"), Index),
                Marker))
        {
            continue;
        }

        const bool bReducedPresentation = Index == 5;
        Marker->InitializeMarker(
            MarkerTypes[Index],
            bReducedPresentation,
            bReducedPresentation);
        TestTrue(
            *FString::Printf(TEXT("Marker %d is transient"), Index),
            Marker->HasAnyFlags(RF_Transient));
        TestTrue(
            *FString::Printf(TEXT("Marker %d is presentation tagged"), Index),
            Marker->ActorHasTag(TEXT("EchoesCommandMarkerView")));
        TestTrue(
            *FString::Printf(TEXT("Marker %d has no collision"), Index),
            Marker->HasCollisionDisabled());
        TestTrue(
            *FString::Printf(TEXT("Marker %d cannot affect navigation"), Index),
            Marker->HasNavigationDisabled());
        TestTrue(
            *FString::Printf(TEXT("Marker %d uses authored VFX assets"), Index),
            Marker->IsUsingAuthoredVFXAssets());
        TestTrue(
            *FString::Printf(TEXT("Marker %d retains its shape code"), Index),
            Marker->GetMarkerType() == MarkerTypes[Index]);
        TestTrue(
            *FString::Printf(TEXT("Marker %d has a readable lifetime"), Index),
            Marker->GetPresentationLifetimeSeconds() >= 2.0f);
        if (MarkerTypes[Index] == EEchoesCommandMarkerType::Attack)
        {
            DirectAttackMeshPath = Marker->GetMarkerMeshPath();
        }
        else if (MarkerTypes[Index] == EEchoesCommandMarkerType::AttackMove)
        {
            AttackMoveMeshPath = Marker->GetMarkerMeshPath();
        }

        TArray<UStaticMeshComponent*> MeshComponents;
        Marker->GetComponents<UStaticMeshComponent>(MeshComponents);
        TestEqual(
            *FString::Printf(TEXT("Marker %d owns three authored mesh layers"), Index),
            MeshComponents.Num(),
            3);
        for (UStaticMeshComponent* Component : MeshComponents)
        {
            TestNotNull(
                *FString::Printf(TEXT("Marker %d mesh layer is assigned"), Index),
                Component->GetStaticMesh().Get());
            if (Component->GetStaticMesh() != nullptr)
            {
                TestTrue(
                    *FString::Printf(TEXT("Marker %d layer is project-authored"), Index),
                    Component->GetStaticMesh()->GetPathName().StartsWith(
                        TEXT("/Game/Art/Generated/VFX/SM_VFX_")));
            }
            TestFalse(
                *FString::Printf(TEXT("Marker %d primitive does not overlap"), Index),
                Component->GetGenerateOverlapEvents());
            TestFalse(
                *FString::Printf(TEXT("Marker %d primitive casts no shadow"), Index),
                Component->CastShadow);
        }

        if (bReducedPresentation)
        {
            TestTrue(TEXT("Reduced-motion marker records the setting"),
                     Marker->IsReducedMotionApplied());
            TestTrue(TEXT("Reduced-flashing marker records the setting"),
                     Marker->IsReducedFlashingApplied());
            const FVector ScaleBeforeTick = Marker->GetActorScale3D();
            const float YawBeforeTick = Marker->GetMarkerDiscYaw();
            Marker->Tick(0.25f);
            TestTrue(TEXT("Reduced-motion marker does not pulse its scale"),
                     Marker->GetActorScale3D().Equals(ScaleBeforeTick));
            TestEqual(TEXT("Reduced-motion marker keeps its sigil steady"),
                      Marker->GetMarkerDiscYaw(),
                      YawBeforeTick);
            TestTrue(TEXT("Reduced-flashing marker uses steady low emission"),
                     Marker->GetCurrentEmissiveStrength() <= 1.35f);
        }
        else if (Index == 0)
        {
            const float YawBeforeTick = Marker->GetMarkerDiscYaw();
            Marker->Tick(0.25f);
            TestNotEqual(TEXT("Standard marker rotates its authored sigil"),
                         Marker->GetMarkerDiscYaw(),
                         YawBeforeTick);
            TestTrue(TEXT("Standard marker retains readable emissive energy"),
                     Marker->GetCurrentEmissiveStrength() > 2.6f);
        }
    }

    TestTrue(TEXT("Direct attack owns a named authored sigil"),
             DirectAttackMeshPath.EndsWith(
                 TEXT("SM_VFX_CommandAttack.SM_VFX_CommandAttack")));
    TestTrue(TEXT("Attack-move owns its area-order sigil"),
             AttackMoveMeshPath.EndsWith(
                 TEXT("SM_VFX_CommandAttackMove.SM_VFX_CommandAttackMove")));
    TestNotEqual(TEXT("Direct attack and attack-move are shape-distinct"),
                 DirectAttackMeshPath,
                 AttackMoveMeshPath);

    TestEqual(
        TEXT("Presentation markers do not alter authoritative checksum"),
        Bridge->GetSimulation()->StateChecksum(),
        AuthoritativeChecksum);

    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif
