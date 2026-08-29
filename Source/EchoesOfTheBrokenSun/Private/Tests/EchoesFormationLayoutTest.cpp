#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesFormationLayout.h"
#include "EchoesPlayerController.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesFormationLayoutTest,
    "Echoes.Runtime.Presentation.FormationLayout",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesFormationLayoutTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    const FVector Anchor(1000.0f, 2000.0f, 40.0f);
    const FVector Forward(0.0f, 1.0f, 0.0f);
    constexpr float Spacing = 150.0f;

    const TArray<FVector> Empty = FEchoesFormationLayout::BuildDestinations(
        Anchor,
        Forward,
        0,
        EEchoesFormationType::Box,
        Spacing);
    TestTrue(TEXT("Zero units produce no formation destinations"), Empty.IsEmpty());

    const TArray<FVector> Box = FEchoesFormationLayout::BuildDestinations(
        Anchor,
        Forward,
        5,
        EEchoesFormationType::Box,
        Spacing);
    TestEqual(TEXT("Box contains every requested destination"), Box.Num(), 5);
    FVector BoxCentroid = FVector::ZeroVector;
    for (const FVector& Destination : Box)
    {
        BoxCentroid += Destination;
    }
    BoxCentroid /= static_cast<float>(Box.Num());
    TestTrue(TEXT("Box is centered on the requested anchor"),
             BoxCentroid.Equals(Anchor, 0.01f));

    const TArray<FVector> Line = FEchoesFormationLayout::BuildDestinations(
        Anchor,
        Forward,
        5,
        EEchoesFormationType::Line,
        Spacing);
    TestEqual(TEXT("Line contains every requested destination"), Line.Num(), 5);
    for (const FVector& Destination : Line)
    {
        TestTrue(TEXT("Line stays perpendicular to travel"),
                 FMath::IsNearlyEqual(Destination.Y, Anchor.Y));
        TestTrue(TEXT("Line retains battlefield height"),
                 FMath::IsNearlyEqual(Destination.Z, Anchor.Z));
    }
    TestTrue(TEXT("Line endpoints are symmetrically spaced"),
             FMath::IsNearlyEqual(
                 FVector::Dist(Line[0], Line[4]),
                 Spacing * 4.0f));

    const TArray<FVector> Wedge = FEchoesFormationLayout::BuildDestinations(
        Anchor,
        Forward,
        5,
        EEchoesFormationType::Wedge,
        Spacing);
    TestEqual(TEXT("Wedge contains every requested destination"), Wedge.Num(), 5);
    TestTrue(TEXT("Wedge apex lands on the requested anchor"),
             Wedge[0].Equals(Anchor, 0.01f));
    TestTrue(TEXT("First wedge pair trails the apex"),
             Wedge[1].Y < Anchor.Y && Wedge[2].Y < Anchor.Y);
    TestTrue(TEXT("First wedge pair is laterally symmetric"),
             FMath::IsNearlyEqual(
                 FMath::Abs(Wedge[1].X - Anchor.X),
                 FMath::Abs(Wedge[2].X - Anchor.X)));
    TestTrue(TEXT("Second wedge rank trails the first"),
             Wedge[3].Y < Wedge[1].Y && Wedge[4].Y < Wedge[2].Y);

    for (const TArray<FVector>* Formation : {&Box, &Line, &Wedge})
    {
        for (int32 Left = 0; Left < Formation->Num(); ++Left)
        {
            for (int32 Right = Left + 1; Right < Formation->Num(); ++Right)
            {
                TestTrue(
                    TEXT("Formation destinations never overlap"),
                    FVector::DistSquared((*Formation)[Left], (*Formation)[Right]) >
                        1.0f);
            }
        }
    }

    TestEqual(TEXT("Box exposes a stable HUD label"),
              FEchoesFormationLayout::DisplayName(EEchoesFormationType::Box),
              FString(TEXT("BOX")));
    TestEqual(TEXT("Line exposes a stable HUD label"),
              FEchoesFormationLayout::DisplayName(EEchoesFormationType::Line),
              FString(TEXT("LINE")));
    TestEqual(TEXT("Wedge exposes a stable HUD label"),
              FEchoesFormationLayout::DisplayName(EEchoesFormationType::Wedge),
              FString(TEXT("WEDGE")));

    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the formation-controller test world."));
        return false;
    }
    AEchoesPlayerController* Controller =
        WorldWrapper.GetTestWorld()->SpawnActor<AEchoesPlayerController>();
    if (TestNotNull(TEXT("Formation controller spawns"), Controller))
    {
        TestTrue(TEXT("Formation defaults to Box"),
                 Controller->GetFormationType() == EEchoesFormationType::Box);
        Controller->CycleFormation();
        TestTrue(TEXT("First formation cycle selects Line"),
                 Controller->GetFormationType() == EEchoesFormationType::Line);
        Controller->CycleFormation();
        TestTrue(TEXT("Second formation cycle selects Wedge"),
                 Controller->GetFormationType() == EEchoesFormationType::Wedge);
        Controller->CycleFormation();
        TestTrue(TEXT("Third formation cycle returns to Box"),
                 Controller->GetFormationType() == EEchoesFormationType::Box);
    }
    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif
