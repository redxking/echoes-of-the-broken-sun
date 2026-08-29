#include "EchoesContactIndicatorLayout.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesContactIndicatorLayoutTest,
    "Echoes.Runtime.Presentation.ContactIndicatorLayout",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEchoesContactIndicatorLayoutTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    const FVector2D Viewport(1600.0f, 900.0f);
    const FEchoesContactIndicatorPlacement Center =
        FEchoesContactIndicatorLayout::Calculate(
            FVector2D(800.0f, 450.0f), Viewport, 1.0f);
    TestFalse(TEXT("Visible contact is not edge-clamped"), Center.bClampedToEdge);
    TestEqual(TEXT("Visible marker keeps its projection"),
              Center.MarkerPosition, FVector2D(800.0f, 450.0f));
    TestFalse(TEXT("Centered contact labels to the right"), Center.bLabelOnLeft);

    const FEchoesContactIndicatorPlacement TopLeft =
        FEchoesContactIndicatorLayout::Calculate(
            FVector2D(-400.0f, -200.0f), Viewport, 1.0f);
    TestTrue(TEXT("Top-left contact becomes an edge cue"),
             TopLeft.bClampedToEdge);
    TestTrue(TEXT("Top-left marker respects the horizontal margin"),
             FMath::IsNearlyEqual(TopLeft.MarkerPosition.X, 30.0));
    TestTrue(TEXT("Top-left marker clears the primary HUD"),
             FMath::IsNearlyEqual(TopLeft.MarkerPosition.Y, 300.0));
    TestTrue(TEXT("Left-edge label remains inside the viewport"),
             TopLeft.LabelPosition.X >= 18.0f);

    const FEchoesContactIndicatorPlacement BottomRight =
        FEchoesContactIndicatorLayout::Calculate(
            FVector2D(2400.0f, 1400.0f), Viewport, 1.0f);
    TestTrue(TEXT("Bottom-right contact becomes an edge cue"),
             BottomRight.bClampedToEdge);
    TestTrue(TEXT("Bottom-right marker respects the horizontal margin"),
             FMath::IsNearlyEqual(BottomRight.MarkerPosition.X, 1570.0));
    TestTrue(TEXT("Bottom-right marker clears the command deck"),
             FMath::IsNearlyEqual(BottomRight.MarkerPosition.Y, 672.0));
    TestTrue(TEXT("Right-edge label is placed to the left"),
             BottomRight.bLabelOnLeft);
    TestTrue(TEXT("Right-edge label remains inside the viewport"),
             BottomRight.LabelPosition.X + 270.0f <= Viewport.X - 18.0f);

    const FEchoesContactIndicatorPlacement Invalid =
        FEchoesContactIndicatorLayout::Calculate(
            FVector2D(50.0f, 50.0f), FVector2D::ZeroVector, 1.0f);
    TestEqual(TEXT("Invalid viewport fails to a zero marker"),
              Invalid.MarkerPosition, FVector2D::ZeroVector);
    TestEqual(TEXT("Visible label states anonymity"),
              FEchoesContactIndicatorLayout::BuildPrimaryLabel(2, false),
              FString(TEXT("ANONYMOUS VIBRATION 02")));
    TestEqual(TEXT("Edge label states anonymity and edge state"),
              FEchoesContactIndicatorLayout::BuildPrimaryLabel(1, true),
              FString(TEXT("ANONYMOUS VIBRATION 01  //  EDGE")));
    return true;
}

#endif
