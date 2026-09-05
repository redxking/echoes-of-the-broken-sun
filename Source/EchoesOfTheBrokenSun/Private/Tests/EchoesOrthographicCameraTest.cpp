#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "EchoesRTSCameraPawn.h"
#include "EchoesSimCore/Simulation.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputCoreTypes.h"
#include "Tests/AutomationCommon.h"

namespace
{
constexpr float DefaultLegacyFovDegrees = 55.0f;

float ExpectedOrthoWidth(const float ArmLength, const float FieldOfViewDegrees)
{
    return 2.0f * FMath::Max(ArmLength, 600.0f) *
        FMath::Tan(FMath::DegreesToRadians(FieldOfViewDegrees * 0.5f));
}

const FInputActionBinding* FindPressedAction(
    const UInputComponent& InputComponent,
    const FName ActionName)
{
    for (int32 Index = 0; Index < InputComponent.GetNumActionBindings(); ++Index)
    {
        const FInputActionBinding& Binding = InputComponent.GetActionBinding(Index);
        if (Binding.GetActionName() == ActionName && Binding.KeyEvent == IE_Pressed)
        {
            return &Binding;
        }
    }
    return nullptr;
}

const FInputAxisBinding* FindAxis(
    const UInputComponent& InputComponent,
    const FName AxisName)
{
    for (const FInputAxisBinding& Binding : InputComponent.AxisBindings)
    {
        if (Binding.AxisName == AxisName)
        {
            return &Binding;
        }
    }
    return nullptr;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesOrthographicCameraTest,
    "Echoes.Runtime.Camera.OrthographicFraming",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesOrthographicCameraTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the orthographic-camera test world."));
        return false;
    }

    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge = World != nullptr
        ? World->GetSubsystem<UEchoesSimulationSubsystem>()
        : nullptr;
    if (Bridge == nullptr || !Bridge->StartPrototypeScenario())
    {
        AddError(TEXT("Could not start the isolated camera test scenario."));
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    AEchoesRTSCameraPawn* Pawn = World->SpawnActor<AEchoesRTSCameraPawn>();
    UCameraComponent* Camera = Pawn != nullptr
        ? Pawn->FindComponentByClass<UCameraComponent>()
        : nullptr;
    USpringArmComponent* SpringArm = Pawn != nullptr
        ? Pawn->FindComponentByClass<USpringArmComponent>()
        : nullptr;
    if (!TestNotNull(TEXT("Native RTS camera spawns"), Pawn) ||
        !TestNotNull(TEXT("Native RTS camera has a camera component"), Camera) ||
        !TestNotNull(TEXT("Native RTS camera has a spring arm"), SpringArm))
    {
        if (Pawn != nullptr) Pawn->Destroy();
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    TestEqual(
        TEXT("Default projection is orthographic"),
        static_cast<int32>(Camera->ProjectionMode),
        static_cast<int32>(ECameraProjectionMode::Orthographic));
    TestTrue(
        TEXT("Default orthographic width is finite and positive"),
        FMath::IsFinite(Camera->OrthoWidth) && Camera->OrthoWidth > 0.0f);
    TestTrue(
        TEXT("Default arm framing converts to the expected orthographic width"),
        FMath::IsNearlyEqual(
            Camera->OrthoWidth,
            ExpectedOrthoWidth(SpringArm->TargetArmLength, DefaultLegacyFovDegrees),
            0.01f));

    // Direct arm edits are common in editor review scripts. Runtime ticking
    // repairs the corresponding width; scripts that disable ticking must call
    // SetCameraFraming explicitly after changing the arm.
    SpringArm->TargetArmLength = 2700.0f;
    Camera->SetOrthoWidth(1.0f);
    Pawn->Tick(0.0f);
    TestTrue(
        TEXT("Tick synchronizes an externally changed arm to orthographic width"),
        FMath::IsNearlyEqual(
            Camera->OrthoWidth,
            ExpectedOrthoWidth(2700.0f, DefaultLegacyFovDegrees),
            0.01f));

    UInputComponent* InputComponent = NewObject<UInputComponent>(Pawn);
    Pawn->SetupPlayerInputComponent(InputComponent);
    const FInputActionBinding* ZoomIn = FindPressedAction(*InputComponent, TEXT("CameraZoomIn"));
    const FInputActionBinding* ZoomOut = FindPressedAction(*InputComponent, TEXT("CameraZoomOut"));
    const FInputAxisBinding* Forward = FindAxis(*InputComponent, TEXT("CameraForward"));
    if (!TestNotNull(TEXT("Camera zoom-in action is bound"), ZoomIn) ||
        !TestNotNull(TEXT("Camera zoom-out action is bound"), ZoomOut) ||
        !TestNotNull(TEXT("Camera-forward axis is bound"), Forward))
    {
        Pawn->Destroy();
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    for (int32 Index = 0; Index < 32; ++Index)
    {
        ZoomOut->ActionDelegate.Execute(EKeys::MouseScrollDown);
    }
    TestEqual(TEXT("Zoom-out clamps at the authored maximum"), SpringArm->TargetArmLength, 6200.0f);
    TestTrue(
        TEXT("Maximum zoom updates orthographic width"),
        FMath::IsNearlyEqual(
            Camera->OrthoWidth,
            ExpectedOrthoWidth(6200.0f, DefaultLegacyFovDegrees),
            0.01f));

    for (int32 Index = 0; Index < 32; ++Index)
    {
        ZoomIn->ActionDelegate.Execute(EKeys::MouseScrollUp);
    }
    TestEqual(TEXT("Zoom-in clamps at the authored minimum"), SpringArm->TargetArmLength, 1400.0f);
    TestTrue(
        TEXT("Minimum zoom updates orthographic width"),
        FMath::IsNearlyEqual(
            Camera->OrthoWidth,
            ExpectedOrthoWidth(1400.0f, DefaultLegacyFovDegrees),
            0.01f));

    const echoes::sim::Simulation* Simulation = Bridge->GetSimulation();
    const uint64 TickBeforePan = Simulation != nullptr ? Simulation->CurrentTick() : 0;
    const uint64 ChecksumBeforePan = Simulation != nullptr ? Simulation->StateChecksum() : 0;
    Pawn->SetActorLocation(FVector::ZeroVector);
    Forward->AxisDelegate.Execute(1.0f);
    Pawn->Tick(0.25f);
    const FVector PanDelta = Pawn->GetActorLocation();
    FRotator HorizontalRotation(0.0f, SpringArm->GetComponentRotation().Yaw, 0.0f);
    TestTrue(
        TEXT("Forward pan follows the camera-relative horizontal forward vector"),
        FVector::DotProduct(PanDelta.GetSafeNormal(), HorizontalRotation.Vector()) > 0.99f);
    TestEqual(
        TEXT("Camera pan does not advance simulation time"),
        Simulation != nullptr ? Simulation->CurrentTick() : 0,
        TickBeforePan);
    TestEqual(
        TEXT("Camera pan does not alter the simulation checksum"),
        Simulation != nullptr ? Simulation->StateChecksum() : 0,
        ChecksumBeforePan);

    Pawn->Destroy();
    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return !WorldWrapper.HasFailed();
}

#endif
