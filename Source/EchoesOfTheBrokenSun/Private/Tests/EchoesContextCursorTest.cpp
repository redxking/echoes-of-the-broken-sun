#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesContextCursor.h"
#include "EchoesContextCursorWidget.h"
#include "EchoesTestSaveEnvironment.h"

#include "Blueprint/UserWidget.h"
#include "Input/HittestGrid.h"
#include "Misc/App.h"
#include "Rendering/DrawElements.h"
#include "Tests/AutomationCommon.h"
#include "Types/PaintArgs.h"
#include "Widgets/SWindow.h"

namespace
{
int32 CountDrawElements(const FSlateDrawElementMap& ElementMap)
{
    int32 Total = 0;
    VisitTupleElements(
        [&Total](const auto& Elements)
        {
            Total += Elements.Num();
        },
        ElementMap);
    return Total;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesContextCursorModelTest,
    "Echoes.Runtime.Input.ContextCursorModel",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesContextCursorModelTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    const auto Resolve = [](FEchoesContextCursorFacts Facts)
    {
        return FEchoesContextCursorModel::Resolve(Facts);
    };

    TestEqual(TEXT("Empty context uses the default pointer"),
        Resolve({}), EEchoesContextCursor::DefaultPointer);

    FEchoesContextCursorFacts Friendly;
    Friendly.bFriendlyEntity = true;
    TestEqual(TEXT("Owned visible entity uses selection brackets"),
        Resolve(Friendly), EEchoesContextCursor::FriendlySelection);

    FEchoesContextCursorFacts Hostile;
    Hostile.bHostileEntity = true;
    TestEqual(TEXT("Visible hostile uses the attack reticle"),
        Resolve(Hostile), EEchoesContextCursor::EnemyAttack);

    FEchoesContextCursorFacts Gather;
    Gather.bGatherableEntity = true;
    Gather.bHostileEntity = true;
    TestEqual(TEXT("Resource context takes gather priority"),
        Resolve(Gather), EEchoesContextCursor::Gather);

    FEchoesContextCursorFacts Build;
    Build.bBuildPlacement = true;
    Build.bPlacementValid = true;
    Build.bHostileEntity = true;
    TestEqual(TEXT("Valid placement uses the blueprint cursor"),
        Resolve(Build), EEchoesContextCursor::Build);
    Build.bPlacementValid = false;
    TestEqual(TEXT("Blocked placement uses the invalid cross"),
        Resolve(Build), EEchoesContextCursor::Invalid);

    FEchoesContextCursorFacts Minimap;
    Minimap.bOverMinimap = true;
    Minimap.bBuildPlacement = true;
    TestEqual(TEXT("Minimap uses its radar cursor"),
        Resolve(Minimap), EEchoesContextCursor::Minimap);

    FEchoesContextCursorFacts Modal;
    Modal.bModal = true;
    Modal.bOverMinimap = true;
    Modal.bHostileEntity = true;
    TestEqual(TEXT("Modal UI restores the default pointer"),
        Resolve(Modal), EEchoesContextCursor::DefaultPointer);

    TSet<FName> Shapes;
    for (uint8 Index = 0;
         Index <= static_cast<uint8>(EEchoesContextCursor::Minimap);
         ++Index)
    {
        const EEchoesContextCursor Cursor =
            static_cast<EEchoesContextCursor>(Index);
        Shapes.Add(FEchoesContextCursorModel::Style(Cursor, false).Shape);
        TestFalse(TEXT("Every cursor has a stable name"),
            FString(FEchoesContextCursorModel::StableName(Cursor)).IsEmpty());
    }
    TestEqual(TEXT("All seven contexts use distinct non-color shapes"),
        Shapes.Num(), 7);

    FEchoesScopedTestSaveEnvironment Storage(*this);
    if (!Storage.IsReady())
    {
        return false;
    }
    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the context-cursor paint test world."));
        return false;
    }
    UEchoesContextCursorWidget* CursorWidget =
        CreateWidget<UEchoesContextCursorWidget>(
            WorldWrapper.GetTestWorld(),
            UEchoesContextCursorWidget::StaticClass(),
            TEXT("ContextCursorPaintUnderTest"));
    if (!TestNotNull(TEXT("Software cursor widget is created"), CursorWidget))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const TSharedRef<SWidget> SlateCursor = CursorWidget->TakeWidget();
    const TSharedRef<SWindow> PaintWindow = SNew(SWindow)
        .ClientSize(FVector2D(48.0f, 48.0f));
    PaintWindow->SetContent(SlateCursor);
    FHittestGrid HittestGrid;
    const FGeometry Geometry = FGeometry::MakeRoot(
        FVector2f(48.0f, 48.0f),
        FSlateLayoutTransform());
    const FSlateRect CullingRect(0.0f, 0.0f, 48.0f, 48.0f);

    for (uint8 Index = 0;
         Index <= static_cast<uint8>(EEchoesContextCursor::Minimap);
         ++Index)
    {
        const EEchoesContextCursor Cursor =
            static_cast<EEchoesContextCursor>(Index);
        CursorWidget->SetCursorState(Cursor, (Index % 2) != 0);
        FSlateWindowElementList DrawElements(PaintWindow);
        const FPaintArgs PaintArgs(
            &PaintWindow.Get(),
            HittestGrid,
            FVector2f::ZeroVector,
            FApp::GetCurrentTime(),
            FApp::GetDeltaTime());
        const int32 PaintedLayer = SlateCursor->Paint(
            PaintArgs,
            Geometry,
            CullingRect,
            DrawElements,
            0,
            FWidgetStyle(),
            true);
        const int32 DrawElementCount = CountDrawElements(
            DrawElements.GetUncachedDrawElements());
        TestTrue(
            *FString::Printf(
                TEXT("%s traverses the real Slate paint path"),
                FEchoesContextCursorModel::StableName(Cursor)),
            PaintedLayer > 0 && DrawElementCount > 0);
    }

    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif
