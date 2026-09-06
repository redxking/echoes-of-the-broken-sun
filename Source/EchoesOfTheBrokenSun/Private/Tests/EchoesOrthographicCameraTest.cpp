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
#include "SceneView.h"
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
    FMinimalViewInfo CameraView;
    Camera->GetCameraView(0.0f, CameraView);
    FSceneViewProjectionData Projection;
    const FIntRect ViewRect(0, 0, 1280, 720);
    Projection.SetViewRectangle(ViewRect);
    FMinimalViewInfo::CalculateProjectionMatrixGivenViewRectangle(
        CameraView, AspectRatio_MaintainYFOV, ViewRect, Projection);
    TestTrue(TEXT("Actual engine projection preserves horizontal orthographic width despite local-player Y default"),
        FMath::IsNearlyEqual(static_cast<float>(Projection.ProjectionMatrix.M[0][0]),
            2.0f / Camera->OrthoWidth, 0.0000001f));
    TestTrue(
        TEXT("Default arm framing converts to the expected orthographic width"),
        FMath::IsNearlyEqual(
            Camera->OrthoWidth,
            ExpectedOrthoWidth(SpringArm->TargetArmLength, DefaultLegacyFovDegrees),
            0.01f));

    TArray<FVector> Footprint;
    TestTrue(
        TEXT("Orthographic camera projects a four-corner ground footprint"),
        Pawn->GetBattlefieldFootprint(FVector2D(1600.0f, 900.0f), Footprint));
    TestEqual(TEXT("Ground footprint contains four ordered corners"),
        Footprint.Num(), 4);
    for (const FVector& Corner : Footprint)
    {
        TestTrue(TEXT("Ground footprint is finite"), !Corner.ContainsNaN());
        TestTrue(TEXT("Ground footprint lies on the battlefield plane"),
            FMath::IsNearlyZero(Corner.Z, 0.01f));
    }

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
    // Reconstruct the cursor's ground point from the independent footprint
    // projection before and after zoom, including the spring-arm update.
    const FVector2D ZoomViewport(1280.0f, 720.0f);
    const FVector2D ZoomPoint(900.0f, 280.0f);
    const auto GroundAtCursor = [&]()
    {
        SpringArm->TickComponent(0.0f, LEVELTICK_All, nullptr);
        TArray<FVector> Corners;
        if (!Pawn->GetBattlefieldFootprint(ZoomViewport, Corners) || Corners.Num() != 4)
        {
            AddError(TEXT("Cursor zoom requires a valid four-corner footprint"));
            return FVector::ZeroVector;
        }
        return Corners[0] + (Corners[1] - Corners[0]) * (ZoomPoint.X / ZoomViewport.X) +
            (Corners[3] - Corners[0]) * (ZoomPoint.Y / ZoomViewport.Y);
    };
    Pawn->SetActorLocation(FVector::ZeroVector);
    Pawn->SetCameraFraming(3000.0f);
    const FVector CursorGroundBefore = GroundAtCursor();
    Pawn->ApplyZoomAtViewportPoint(-1.0f, ZoomPoint, ZoomViewport);
    TestTrue(TEXT("Off-center cursor ground point stays fixed during zoom in"),
        GroundAtCursor().Equals(CursorGroundBefore, 0.1f));
    Pawn->ApplyZoomAtViewportPoint(1.0f, ZoomPoint, ZoomViewport);
    TestTrue(TEXT("Off-center cursor ground point stays fixed during zoom out"),
        GroundAtCursor().Equals(CursorGroundBefore, 0.1f));
    TestTrue(TEXT("Cursor zoom round trip restores camera position"),
        Pawn->GetActorLocation().Equals(FVector::ZeroVector, 0.1f));
    Pawn->SetCameraFraming(1400.0f);
    SpringArm->TickComponent(0.0f, LEVELTICK_All, nullptr);
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

    const FVector EvacuationSite = Bridge->SimToWorld(
        echoes::sim::Vec2::FromTiles(6, 17));
    Pawn->PanToWorld(EvacuationSite);
    FVector2D NavigationCenter;
    TestTrue(TEXT("Boundary navigation exposes a finite center"),
        Pawn->GetNavigationCenter(NavigationCenter));
    TestTrue(TEXT("The evacuation boundary remains reachable within tutorial tolerance"),
        FVector2D::Distance(
            NavigationCenter,
            FVector2D(EvacuationSite.X, EvacuationSite.Y)) <= 200.0f);

    const uint64 BeforeNavigationRevision = Pawn->GetNavigationRevision();
    const FVector BeforeGrabPan = Pawn->GetActorLocation();
    Pawn->PanByScreenDelta(FVector2D(120.0f, -60.0f), 1600.0f);
    TestFalse(TEXT("Middle-drag translation changes presentation position"),
        Pawn->GetActorLocation().Equals(BeforeGrabPan, 0.01f));
    TestTrue(TEXT("Middle-drag records player navigation provenance"),
        Pawn->WasLastNavigationPlayerDriven() && Pawn->GetNavigationRevision() > BeforeNavigationRevision);
    TestEqual(TEXT("Middle-drag translation preserves simulation tick"),
        Simulation != nullptr ? Simulation->CurrentTick() : 0,
        TickBeforePan);
    TestEqual(TEXT("Middle-drag translation preserves simulation checksum"),
        Simulation != nullptr ? Simulation->StateChecksum() : 0,
        ChecksumBeforePan);

    Pawn->PanToWorld(FVector(100000.0f, 100000.0f, 0.0f));
    TestFalse(TEXT("Programmatic reposition cannot claim player navigation"), Pawn->WasLastNavigationPlayerDriven());
    Footprint.Reset();
    TestTrue(TEXT("Boundary pan retains a valid ground footprint"),
        Pawn->GetBattlefieldFootprint(FVector2D(1600.0f, 900.0f), Footprint));
    const float MapHalfWidth = Bridge->GetMapWidthTiles() *
        UEchoesSimulationSubsystem::TileWorldSize * 0.5f;
    const float MapHalfHeight = Bridge->GetMapHeightTiles() *
        UEchoesSimulationSubsystem::TileWorldSize * 0.5f;
    TestTrue(TEXT("Out-of-map navigation clamps the camera target to map bounds"),
        FMath::IsNearlyEqual(Pawn->GetActorLocation().X, MapHalfWidth, 0.1f) &&
        FMath::IsNearlyEqual(Pawn->GetActorLocation().Y, MapHalfHeight, 0.1f));
    for (const FVector& Corner : Footprint)
    {
        TestTrue(TEXT("Edge navigation retains finite ground-plane footprint corners"),
            !Corner.ContainsNaN() && FMath::IsNearlyZero(Corner.Z, 0.01f));
    }

    int32 FixedCallbacks = 0;
    const uint64 BeforeCatchUp = Simulation->CurrentTick();
    const FDelegateHandle FixedHandle = Bridge->OnFixedStepObserved.AddLambda([&FixedCallbacks]() { ++FixedCallbacks; });
    Bridge->SetScenarioPaused(false);
    Bridge->Tick(0.2f);
    Bridge->SetScenarioPaused(true);
    Bridge->OnFixedStepObserved.Remove(FixedHandle);
    TestTrue(TEXT("A low-frame-rate update advances multiple fixed steps"), FixedCallbacks >= 4);
    TestEqual(TEXT("Every fixed step, including catch-up, reaches observers"),
        static_cast<uint64>(FixedCallbacks), Simulation->CurrentTick() - BeforeCatchUp);
    Pawn->Destroy();
    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return !WorldWrapper.HasFailed();
}

#endif
