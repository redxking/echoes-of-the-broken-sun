#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesShellWidget.h"
#include "EchoesTestSaveEnvironment.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"
#include "Engine/World.h"
#include "Input/HittestGrid.h"
#include "InputCoreTypes.h"
#include "Misc/App.h"
#include "Rendering/DrawElements.h"
#include "Tests/AutomationCommon.h"
#include "Types/PaintArgs.h"
#include "Widgets/SWindow.h"
#include "Widgets/SWidget.h"

namespace
{
FEchoesShellButton ShellButton(
    const TCHAR* Label,
    EEchoesShellAction Action,
    int32 Argument = 0,
    bool bEnabled = true)
{
    FEchoesShellButton Button;
    Button.Label = FText::FromString(Label);
    Button.Action = Action;
    Button.Argument = Argument;
    Button.bEnabled = bEnabled;
    return Button;
}

FEchoesShellView ShellView(
    EEchoesShellScreen Screen,
    float Scale,
    TArray<FEchoesShellButton> Buttons)
{
    FEchoesShellView View;
    View.Screen = Screen;
    View.Eyebrow = FText::FromString(TEXT("FIELDWORK INTERFACE"));
    View.Title = FText::FromString(TEXT("Shell test"));
    View.Body = FText::FromString(TEXT("Keyboard and refresh coverage."));
    View.Status = FText::FromString(TEXT("Ready"));
    View.Scale = Scale;
    View.Buttons = MoveTemp(Buttons);
    return View;
}

FReply SendKey(const TSharedRef<SWidget>& SlateWidget, const FKey& Key)
{
    const FKeyEvent Event(
        Key,
        FModifierKeysState(),
        0,
        false,
        0,
        0);
    return SlateWidget->OnPreviewKeyDown(FGeometry(), Event);
}

int32 CountActionButtons(UWidget* Root)
{
    TArray<UWidget*> Descendants;
    UWidgetTree::GetChildWidgets(Root, Descendants);
    return Descendants.FilterByPredicate(
        [](const UWidget* Candidate)
        {
            return Candidate != nullptr &&
                Candidate->IsA<UEchoesShellActionButton>();
        }).Num();
}
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesShellWidgetTest,
    "Echoes.Runtime.UI.ShellWidgetRefreshAndFocus",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesShellWidgetTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    FEchoesScopedTestSaveEnvironment Storage(*this);
    if (!Storage.IsReady()) return false;

    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the shell-widget test world."));
        return false;
    }

    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesShellWidget* Widget = World != nullptr
        ? CreateWidget<UEchoesShellWidget>(
              World,
              UEchoesShellWidget::StaticClass(),
              TEXT("ShellWidgetUnderTest"))
        : nullptr;
    if (!TestNotNull(TEXT("Native shell widget is created"), Widget))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const TSharedRef<SWidget> ColdSlate = Widget->TakeWidget();
    UWidget* const ColdRoot = Widget->GetRootWidget();
    TestNotNull(TEXT("Cold attachment constructs the root before any view is supplied"), ColdRoot);
    Widget->SetView(ShellView(
        EEchoesShellScreen::Options,
        0.8f,
        {
            ShellButton(TEXT("Disabled"), EEchoesShellAction::Back, 0, false),
            ShellButton(TEXT("Primary"), EEchoesShellAction::Primary, 7),
            ShellButton(TEXT("Credits"), EEchoesShellAction::Credits)
        }));
    TestEqual(
        TEXT("Small scale starts on the first enabled action"),
        Widget->GetFocusedButtonIndex(),
        1);

    const TSharedRef<SWidget> InitialSlate = Widget->TakeWidget();
    UWidget* const InitialRoot = Widget->GetRootWidget();
    TestTrue(TEXT("First view replaces content beneath the already attached root"), InitialRoot == ColdRoot && &InitialSlate.Get() == &ColdSlate.Get());
    TestEqual(TEXT("Cold-attached hierarchy receives the first view's buttons"), CountActionButtons(ColdRoot), 3);
    TestNotNull(TEXT("TakeWidget builds a native UMG root"), InitialRoot);
    TestTrue(TEXT("Preview navigation owns Up before child buttons"), SendKey(InitialSlate, EKeys::Up).IsEventHandled());
    TestEqual(TEXT("Reverse navigation wraps to the last enabled action"), Widget->GetFocusedButtonIndex(), 2);
    SendKey(InitialSlate, EKeys::Down);
    TestEqual(TEXT("Forward wrap skips the disabled first action"), Widget->GetFocusedButtonIndex(), 1);
    TestTrue(
        TEXT("Down is handled at 0.8 scale"),
        SendKey(InitialSlate, EKeys::Down).IsEventHandled());
    TestEqual(
        TEXT("Keyboard navigation reaches the next enabled action at 0.8 scale"),
        Widget->GetFocusedButtonIndex(),
        2);

    Widget->SetView(ShellView(
        EEchoesShellScreen::Options,
        1.5f,
        {
            ShellButton(TEXT("Credits"), EEchoesShellAction::Credits),
            ShellButton(TEXT("Disabled"), EEchoesShellAction::Back, 0, false),
            ShellButton(TEXT("Primary"), EEchoesShellAction::Primary, 7),
            ShellButton(TEXT("Save"), EEchoesShellAction::Save, 2)
        }));
    const TSharedRef<SWidget> RefreshedSlate = Widget->TakeWidget();
    TestTrue(
        TEXT("View refresh retains the UMG root consumed by cached Slate"),
        Widget->GetRootWidget() == InitialRoot);
    TestTrue(
        TEXT("TakeWidget retains its cached Slate wrapper through refresh"),
        &RefreshedSlate.Get() == &InitialSlate.Get());
    TestEqual(
        TEXT("Stable root owns the replacement button hierarchy"),
        CountActionButtons(InitialRoot),
        4);
    TestEqual(
        TEXT("Same-screen refresh retains focus by action and argument"),
        Widget->GetFocusedButtonIndex(),
        0);
    TestTrue(
        TEXT("Down is handled at 1.5 scale"),
        SendKey(RefreshedSlate, EKeys::Down).IsEventHandled());
    TestEqual(
        TEXT("Keyboard navigation skips a disabled action at 1.5 scale"),
        Widget->GetFocusedButtonIndex(),
        2);

    Widget->SetView(ShellView(
        EEchoesShellScreen::Confirmation,
        1.5f,
        {
            ShellButton(TEXT("Confirm"), EEchoesShellAction::Confirm),
            ShellButton(TEXT("Unavailable"), EEchoesShellAction::Primary, 0, false),
            ShellButton(TEXT("Cancel"), EEchoesShellAction::Cancel, 42)
        }));
    TestEqual(
        TEXT("Confirmation defaults to Cancel instead of the prior index"),
        Widget->GetFocusedButtonIndex(),
        2);
    TestTrue(
        TEXT("Confirmation refresh still retains the original UMG root"),
        Widget->GetRootWidget() == InitialRoot);

    Widget->SetView(ShellView(
        EEchoesShellScreen::Options,
        1.0f,
        {
            ShellButton(TEXT("Disabled back"), EEchoesShellAction::Back, 0, false),
            ShellButton(TEXT("Disabled primary"), EEchoesShellAction::Primary, 0, false)
        }));
    TestEqual(
        TEXT("An all-disabled view leaves no focused action"),
        Widget->GetFocusedButtonIndex(),
        INDEX_NONE);
    TestFalse(
        TEXT("Disabled actions cannot be reached by keyboard navigation"),
        Widget->FocusNext(false));
    TestFalse(
        TEXT("Disabled actions cannot be activated"),
        Widget->ActivateFocused());

    Widget->SetView(ShellView(
        EEchoesShellScreen::Options,
        1.0f,
        {
            ShellButton(TEXT("Enabled option"), EEchoesShellAction::Options, 0, true),
            ShellButton(TEXT("Disabled option"), EEchoesShellAction::Back, 0, false)
        }));
    TestEqual(TEXT("Enabled option is focused"), Widget->GetFocusedButtonIndex(), 0);
    TestNotNull(TEXT("Button 0 is accessible"), Widget->GetActionButton(0));
    TestNotNull(TEXT("Button 1 is accessible"), Widget->GetActionButton(1));
    TestFalse(
        TEXT("Pointer click offscreen returns false without crashing"),
        Widget->ActivateButtonUnderLocation(FVector2D(-1000.f, -1000.f)));

    Widget->SetView(ShellView(
        EEchoesShellScreen::Options,
        1.0f,
        {
            ShellButton(TEXT("First action"), EEchoesShellAction::Options),
            ShellButton(TEXT("Second action"), EEchoesShellAction::Credits)
        }));
    TestEqual(
        TEXT("Pointer routing fixture starts on the first enabled action"),
        Widget->GetFocusedButtonIndex(),
        0);

    const TSharedRef<SWidget> PointerSlate = Widget->TakeWidget();
    const TSharedRef<SWindow> PaintWindow = SNew(SWindow)
        .ClientSize(FVector2D(1280, 720));
    PaintWindow->SetContent(PointerSlate);
    PointerSlate->SlatePrepass(1.0f);
    FHittestGrid HittestGrid;
    const FPaintArgs PaintArgs(
        &PaintWindow.Get(), HittestGrid, FVector2f::ZeroVector,
        FApp::GetCurrentTime(), FApp::GetDeltaTime());
    const FGeometry PaintGeometry = FGeometry::MakeRoot(
        FVector2f(1280, 720), FSlateLayoutTransform());
    FSlateWindowElementList PaintElements(PaintWindow);
    PointerSlate->Paint(
        PaintArgs, PaintGeometry, FSlateRect(0, 0, 1280, 720),
        PaintElements, 0, FWidgetStyle(), true);

    UEchoesShellActionButton* SecondButton = Widget->GetActionButton(1);
    TestNotNull(TEXT("Second pointer target is available"), SecondButton);
    if (SecondButton != nullptr)
    {
        const FGeometry ButtonGeometry = SecondButton->GetCachedGeometry();
        TestTrue(
            TEXT("Real Slate paint gives the second action nonzero cached geometry"),
            ButtonGeometry.GetLocalSize().X > 0.0f &&
                ButtonGeometry.GetLocalSize().Y > 0.0f);
        const FVector2D Center = ButtonGeometry.LocalToAbsolute(
            ButtonGeometry.GetLocalSize() * 0.5f);
        const auto PressAtCenter =
            [&PointerSlate, &PaintGeometry, &Center](const FKey& Button)
            {
                return PointerSlate->OnMouseButtonDown(
                    PaintGeometry,
                    FPointerEvent(
                        0, Center, Center, TSet<FKey>{Button}, Button, 0.0f,
                        FModifierKeysState()));
            };

        TestTrue(
            TEXT("Right click inside a shell action is consumed"),
            PressAtCenter(EKeys::RightMouseButton).IsEventHandled());
        TestEqual(
            TEXT("Right click does not move shell action focus"),
            Widget->GetFocusedButtonIndex(),
            0);
        TestTrue(
            TEXT("Middle click inside a shell action is consumed"),
            PressAtCenter(EKeys::MiddleMouseButton).IsEventHandled());
        TestEqual(
            TEXT("Middle click does not move shell action focus"),
            Widget->GetFocusedButtonIndex(),
            0);
        TestTrue(
            TEXT("Left click inside a shell action is consumed"),
            PressAtCenter(EKeys::LeftMouseButton).IsEventHandled());
        TestEqual(
            TEXT("Left click moves focus to the pointed shell action"),
            Widget->GetFocusedButtonIndex(),
            1);
    }

    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif
