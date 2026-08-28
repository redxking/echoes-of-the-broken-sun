#include "EchoesRTSCameraPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/SceneComponent.h"
#include "EchoesPlayerController.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"

AEchoesRTSCameraPawn::AEchoesRTSCameraPawn()
{
    PrimaryActorTick.bCanEverTick = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(SceneRoot);
    SpringArm->TargetArmLength = 3800.0f;
    SpringArm->SetRelativeRotation(FRotator(-58.0f, -45.0f, 0.0f));
    SpringArm->bDoCollisionTest = false;
    SpringArm->bEnableCameraLag = true;
    SpringArm->CameraLagSpeed = 12.0f;

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
    Camera->SetFieldOfView(55.0f);

}

void AEchoesRTSCameraPawn::BeginPlay()
{
    Super::BeginPlay();
    bEdgePanArmed = false;
    SetActorLocation(FVector(-3000.0f, -3000.0f, 100.0f));
}

void AEchoesRTSCameraPawn::SetupPlayerInputComponent(
    UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    check(PlayerInputComponent != nullptr);

    PlayerInputComponent->BindAxis(
        TEXT("CameraForward"),
        this,
        &AEchoesRTSCameraPawn::SetForwardInput);
    PlayerInputComponent->BindAxis(
        TEXT("CameraRight"),
        this,
        &AEchoesRTSCameraPawn::SetRightInput);
    PlayerInputComponent->BindAction(
        TEXT("CameraZoomIn"),
        IE_Pressed,
        this,
        &AEchoesRTSCameraPawn::ZoomIn);
    PlayerInputComponent->BindAction(
        TEXT("CameraZoomOut"),
        IE_Pressed,
        this,
        &AEchoesRTSCameraPawn::ZoomOut);
}

void AEchoesRTSCameraPawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    FVector2D EdgeInput = FVector2D::ZeroVector;
    APlayerController* Controller = Cast<APlayerController>(GetController());
    const AEchoesPlayerController* EchoesController =
        Cast<AEchoesPlayerController>(Controller);
    if (Controller != nullptr &&
        (EchoesController == nullptr || !EchoesController->IsDraggingSelection()))
    {
        int32 ViewportWidth = 0;
        int32 ViewportHeight = 0;
        float MouseX = 0.0f;
        float MouseY = 0.0f;
        Controller->GetViewportSize(ViewportWidth, ViewportHeight);
        if (ViewportWidth > 0 && ViewportHeight > 0 &&
            Controller->GetMousePosition(MouseX, MouseY))
        {
            const bool bMouseInsideHorizontalEdges =
                MouseX > EdgePanPixels &&
                MouseX < static_cast<float>(ViewportWidth) - EdgePanPixels;
            const bool bMouseInsideVerticalEdges =
                MouseY > EdgePanPixels &&
                MouseY < static_cast<float>(ViewportHeight) - EdgePanPixels;
            if (!bEdgePanArmed)
            {
                // Do not let a pointer parked at an edge during application launch
                // move the camera before the player has entered the battlefield view.
                bEdgePanArmed =
                    bMouseInsideHorizontalEdges && bMouseInsideVerticalEdges;
            }
            else if (MouseX <= EdgePanPixels)
            {
                EdgeInput.Y = -1.0f;
            }
            else if (MouseX >= static_cast<float>(ViewportWidth) - EdgePanPixels)
            {
                EdgeInput.Y = 1.0f;
            }
            if (bEdgePanArmed && MouseY <= EdgePanPixels)
            {
                EdgeInput.X = 1.0f;
            }
            else if (bEdgePanArmed &&
                     MouseY >= static_cast<float>(ViewportHeight) - EdgePanPixels)
            {
                EdgeInput.X = -1.0f;
            }
        }
    }

    const float AppliedForward = FMath::Clamp(ForwardInput + EdgeInput.X, -1.0f, 1.0f);
    const float AppliedRight = FMath::Clamp(RightInput + EdgeInput.Y, -1.0f, 1.0f);
    const FRotator HorizontalViewRotation(
        0.0f,
        SpringArm->GetComponentRotation().Yaw,
        0.0f);
    const FVector ViewForward = HorizontalViewRotation.Vector();
    const FVector ViewRight = FRotationMatrix(HorizontalViewRotation).GetUnitAxis(EAxis::Y);
    const FVector PanDelta =
        (ViewForward * AppliedForward + ViewRight * AppliedRight) *
        PanSpeed * DeltaSeconds;
    AddActorWorldOffset(PanDelta, false, nullptr, ETeleportType::None);
    ClampToBattlefield();
}

void AEchoesRTSCameraPawn::SetForwardInput(float Value)
{
    ForwardInput = Value;
}

void AEchoesRTSCameraPawn::SetRightInput(float Value)
{
    RightInput = Value;
}

void AEchoesRTSCameraPawn::ZoomIn()
{
    ApplyZoom(-1.0f);
}

void AEchoesRTSCameraPawn::ZoomOut()
{
    ApplyZoom(1.0f);
}

void AEchoesRTSCameraPawn::ApplyZoom(float Direction)
{
    SpringArm->TargetArmLength = FMath::Clamp(
        SpringArm->TargetArmLength + ZoomStep * Direction,
        MinimumZoom,
        MaximumZoom);
}

void AEchoesRTSCameraPawn::ClampToBattlefield()
{
    const UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr)
    {
        return;
    }

    const float HalfWidth =
        static_cast<float>(Bridge->GetMapWidthTiles()) *
        UEchoesSimulationSubsystem::TileWorldSize * 0.5f;
    const float HalfHeight =
        static_cast<float>(Bridge->GetMapHeightTiles()) *
        UEchoesSimulationSubsystem::TileWorldSize * 0.5f;
    FVector Location = GetActorLocation();
    Location.X = FMath::Clamp(Location.X, -HalfWidth, HalfWidth);
    Location.Y = FMath::Clamp(Location.Y, -HalfHeight, HalfHeight);
    SetActorLocation(Location);
}
