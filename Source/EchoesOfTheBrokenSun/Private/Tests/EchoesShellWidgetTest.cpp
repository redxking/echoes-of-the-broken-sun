#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesShellWidget.h"
#include "EchoesTestSaveEnvironment.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
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

    // A standalone paint fixture has no viewport tick. Preserve the first-paint
    // discovery, then let Slate settle deferred auto-wrap/size invalidation.
    if (auto* FirstPaintButton = Widget->GetActionButton(1))
        AddInfo(FString::Printf(TEXT("First shell paint: desired=%s cached=%s"),
            *FirstPaintButton->GetDesiredSize().ToString(),
            *FirstPaintButton->GetCachedGeometry().GetLocalSize().ToString()));
    PointerSlate->SlatePrepass(1.0f);
    // A fresh draw list is required for the settled paint; reusing the first
    // one would blend discovery and final geometry in this fixture.
    FSlateWindowElementList SettledPaintElements(PaintWindow);
    PointerSlate->Paint(
        PaintArgs, PaintGeometry, FSlateRect(0, 0, 1280, 720),
        SettledPaintElements, 0, FWidgetStyle(), true);

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
        TestFalse(
            TEXT("An absolute off-target press cannot fall back to another cursor location"),
            Widget->ActivateButtonUnderLocation(FVector2D(-1000.f, -1000.f)));
        TestEqual(TEXT("An off-target press preserves focus"),
            Widget->GetFocusedButtonIndex(), 1);
        TestTrue(
            TEXT("ActivateButtonUnderLocation directly activates the pointed button"),
            Widget->ActivateButtonUnderLocation(Center));
        TestEqual(
            TEXT("Direct button activation maintains focused index"),
            Widget->GetFocusedButtonIndex(),
            1);
        const FPointerEvent PreviewLeftClick(
            0, Center, Center, TSet<FKey>{EKeys::LeftMouseButton},
            EKeys::LeftMouseButton, 0.0f, FModifierKeysState());
        TestTrue(
            TEXT("Preview left click inside a shell action is consumed"),
            PointerSlate->OnPreviewMouseButtonDown(PaintGeometry, PreviewLeftClick).IsEventHandled());
    }

    const auto PaintSettledLayout = [&PaintWindow, &PaintGeometry](const TSharedRef<SWidget>& Slate)
    {
        PaintWindow->SetContent(Slate);
        Slate->SlatePrepass(1.0f);
        FHittestGrid DiscoveryGrid;
        const FPaintArgs DiscoveryArgs(
            &PaintWindow.Get(), DiscoveryGrid, FVector2f::ZeroVector,
            FApp::GetCurrentTime(), FApp::GetDeltaTime());
        FSlateWindowElementList DiscoveryElements(PaintWindow);
        Slate->Paint(DiscoveryArgs, PaintGeometry, FSlateRect(0, 0, 1280, 720),
            DiscoveryElements, 0, FWidgetStyle(), true);
        Slate->SlatePrepass(1.0f);
        FHittestGrid SettledGrid;
        const FPaintArgs SettledArgs(
            &PaintWindow.Get(), SettledGrid, FVector2f::ZeroVector,
            FApp::GetCurrentTime(), FApp::GetDeltaTime());
        FSlateWindowElementList SettledElements(PaintWindow);
        Slate->Paint(SettledArgs, PaintGeometry, FSlateRect(0, 0, 1280, 720),
            SettledElements, 0, FWidgetStyle(), true);
    };

    for (const float UiScale : {.8f, 1.f, 1.5f})
    {
        Widget->SetView(ShellView(
            EEchoesShellScreen::Briefing,
            UiScale,
            {
                ShellButton(TEXT("Proceed"), EEchoesShellAction::Primary),
                ShellButton(TEXT("Return"), EEchoesShellAction::Back)
            }));
        PaintSettledLayout(Widget->TakeWidget());
        UEchoesShellActionButton* FirstBriefingAction = Widget->GetActionButton(0);
        UEchoesShellActionButton* LastBriefingAction = Widget->GetActionButton(1);
        TestNotNull(TEXT("Briefing keeps its first action"), FirstBriefingAction);
        TestNotNull(TEXT("Briefing keeps its second action"), LastBriefingAction);
        if (FirstBriefingAction != nullptr && LastBriefingAction != nullptr)
        {
            const FGeometry FirstGeometry = FirstBriefingAction->GetCachedGeometry();
            const FGeometry LastGeometry = LastBriefingAction->GetCachedGeometry();
            const float FirstTop = FirstGeometry.GetAbsolutePosition().Y;
            const float LastBottom = LastGeometry.GetAbsolutePosition().Y +
                LastGeometry.GetLocalSize().Y;
            TestTrue(TEXT("Briefing actions remain inside the safe viewport"),
                FirstTop >= 0.0f && LastBottom <= 720.0f);
            TestTrue(TEXT("Briefing actions keep a compact content-height range"),
                LastBottom - FirstTop < 180.0f * UiScale);
            TestTrue(TEXT("Briefing content is centered instead of filling the panel"),
                FMath::Abs((FirstTop + LastBottom) * 0.5f - 360.0f) <
                    180.0f * UiScale);
        }

        TArray<FEchoesShellButton> LongOptions;
        for (int32 Index = 0; Index < 24; ++Index)
        {
            LongOptions.Add(ShellButton(
                *FString::Printf(TEXT("Setting %d"), Index + 1),
                EEchoesShellAction::Options,
                Index));
        }
        Widget->SetView(ShellView(EEchoesShellScreen::Options, UiScale,
            MoveTemp(LongOptions)));
        PaintSettledLayout(Widget->TakeWidget());
        TArray<UWidget*> LongMenuDescendants;
        UWidgetTree::GetChildWidgets(Widget->GetRootWidget(), LongMenuDescendants);
        UScrollBox* ScrollViewport = nullptr;
        for (UWidget* Candidate : LongMenuDescendants)
        {
            if ((ScrollViewport = Cast<UScrollBox>(Candidate)) != nullptr)
            {
                break;
            }
        }
        TestTrue(TEXT("Long options retain a visible scroll viewport"),
            ScrollViewport != nullptr &&
            ScrollViewport->GetVisibility() == ESlateVisibility::Visible &&
            ScrollViewport->GetCachedGeometry().GetLocalSize().Y > 0.0f &&
            ScrollViewport->GetCachedGeometry().GetLocalSize().Y <= 720.0f);

        const TArray<FEchoesShellButton> GroupedOptions = {
            ShellButton(TEXT("UI scale decrease"), EEchoesShellAction::HudScaleDown, 101),
            ShellButton(TEXT("UI scale increase"), EEchoesShellAction::HudScaleUp, 102),
            ShellButton(TEXT("High contrast"), EEchoesShellAction::HighContrast),
            ShellButton(TEXT("Reduced motion"), EEchoesShellAction::ReducedMotion),
            ShellButton(TEXT("Reduced flashing"), EEchoesShellAction::ReducedFlashing),
            ShellButton(TEXT("Controls and key bindings"), EEchoesShellAction::OpenControls),
            ShellButton(TEXT("Edge pan"), EEchoesShellAction::EdgePan),
            ShellButton(TEXT("Pan decrease"), EEchoesShellAction::CameraPanDown),
            ShellButton(TEXT("Pan increase"), EEchoesShellAction::CameraPanUp),
            ShellButton(TEXT("Zoom decrease"), EEchoesShellAction::CameraZoomDown),
            ShellButton(TEXT("Zoom increase"), EEchoesShellAction::CameraZoomUp),
            ShellButton(TEXT("Resolution previous"), EEchoesShellAction::ResolutionPrevious),
            ShellButton(TEXT("Resolution next"), EEchoesShellAction::ResolutionNext),
            ShellButton(TEXT("Window mode"), EEchoesShellAction::WindowMode),
            ShellButton(TEXT("Apply display"), EEchoesShellAction::ApplyDisplay),
            ShellButton(TEXT("Reduced dynamic range"), EEchoesShellAction::DynamicRange),
            ShellButton(TEXT("Master decrease"), EEchoesShellAction::MasterDown, 201),
            ShellButton(TEXT("Master increase"), EEchoesShellAction::MasterUp, 202),
            ShellButton(TEXT("Music decrease"), EEchoesShellAction::MusicDown),
            ShellButton(TEXT("Music increase"), EEchoesShellAction::MusicUp),
            ShellButton(TEXT("Dialogue decrease"), EEchoesShellAction::DialogueDown),
            ShellButton(TEXT("Dialogue increase"), EEchoesShellAction::DialogueUp),
            ShellButton(TEXT("Effects decrease"), EEchoesShellAction::EffectsDown),
            ShellButton(TEXT("Effects increase"), EEchoesShellAction::EffectsUp),
            ShellButton(TEXT("Interface decrease"), EEchoesShellAction::InterfaceDown),
            ShellButton(TEXT("Interface increase"), EEchoesShellAction::InterfaceUp),
            ShellButton(TEXT("Ambience decrease"), EEchoesShellAction::AmbienceDown),
            ShellButton(TEXT("Ambience increase"), EEchoesShellAction::AmbienceUp),
            ShellButton(TEXT("Back"), EEchoesShellAction::Back, 999)
        };
        FEchoesShellView GroupedView = ShellView(EEchoesShellScreen::Options, UiScale, GroupedOptions);
        GroupedView.Sliders.Add({FText::FromString(TEXT("UI scale")),
            EEchoesShellAction::HudScaleValue, UiScale, .8f, 1.5f});
        Widget->SetView(GroupedView);
        PaintSettledLayout(Widget->TakeWidget());
        TArray<UWidget*> GroupedDescendants;
        UWidgetTree::GetChildWidgets(Widget->GetRootWidget(), GroupedDescendants);
        int32 AccessibilityHeadings = 0;
        UScrollBox* OptionsScroll = nullptr;
        bool bCameraHeading = false;
        bool bDisplayHeading = false;
        bool bAudioHeading = false;
        for (UWidget* Candidate : GroupedDescendants)
        {
            if (auto* Scroll = Cast<UScrollBox>(Candidate)) OptionsScroll = Scroll;
            if (const UTextBlock* Text = Cast<UTextBlock>(Candidate))
            {
                const FString Value = Text->GetText().ToString();
                AccessibilityHeadings += Value == TEXT("Accessibility") ? 1 : 0;
                bCameraHeading = bCameraHeading || Value == TEXT("Camera");
                bDisplayHeading = bDisplayHeading || Value == TEXT("Display");
                bAudioHeading = bAudioHeading || Value == TEXT("Audio");
            }
        }
        TestEqual(TEXT("Options contains exactly one Accessibility heading"), AccessibilityHeadings, 1);
        TestTrue(TEXT("Options contains the Camera heading"), bCameraHeading);
        TestTrue(TEXT("Options contains the Display heading"), bDisplayHeading);
        TestTrue(TEXT("Options contains the Audio heading"), bAudioHeading);
        TestEqual(TEXT("Grouped options retain every source action"),
            Widget->GetButtonCount(), GroupedOptions.Num());
        for (int32 Index = 0; Index < GroupedOptions.Num(); ++Index)
        {
            const UEchoesShellActionButton* Button = Widget->GetActionButton(Index);
            TestTrue(TEXT("Grouped option keeps its button index"),
                Button != nullptr && Button->GetAction() == GroupedOptions[Index].Action &&
                Button->GetArgument() == GroupedOptions[Index].Argument);
        }
        const auto CheckEqualWidthPair = [this, Widget, OptionsScroll, &PaintSettledLayout](
            EEchoesShellAction LeftAction, EEchoesShellAction RightAction)
        {
            UEchoesShellActionButton* Left = nullptr;
            UEchoesShellActionButton* Right = nullptr;
            for (int32 Index = 0; Index < Widget->GetButtonCount(); ++Index)
            {
                auto* Button = Widget->GetActionButton(Index);
                if (Button->GetAction() == LeftAction) Left = Button;
                if (Button->GetAction() == RightAction) Right = Button;
            }
            if (!TestNotNull(TEXT("Paired option left action exists"), Left) ||
                !TestNotNull(TEXT("Paired option right action exists"), Right))
            {
                return;
            }
            if (!TestNotNull(TEXT("Options exposes its scroll viewport"), OptionsScroll)) return;
            OptionsScroll->ScrollWidgetIntoView(Left, false, EDescendantScrollDestination::Center);
            PaintSettledLayout(Widget->TakeWidget());
            const FGeometry LeftGeometry = Left->GetCachedGeometry();
            const FGeometry RightGeometry = Right->GetCachedGeometry();
            TestTrue(TEXT("Paired option controls have measured positive geometry"),
                LeftGeometry.GetLocalSize().X > 0 && RightGeometry.GetLocalSize().X > 0);
            TestTrue(TEXT("Paired option controls share a row"),
                FMath::IsNearlyEqual(LeftGeometry.GetAbsolutePosition().Y,
                    RightGeometry.GetAbsolutePosition().Y, 1.0f));
            TestTrue(TEXT("Paired option controls have equal widths"),
                FMath::IsNearlyEqual(LeftGeometry.GetLocalSize().X,
                    RightGeometry.GetLocalSize().X, 1.0f));
            TestTrue(TEXT("Paired option controls retain left-to-right order"),
                LeftGeometry.GetAbsolutePosition().X < RightGeometry.GetAbsolutePosition().X);
        };
        CheckEqualWidthPair(EEchoesShellAction::HudScaleDown, EEchoesShellAction::HudScaleUp);
        CheckEqualWidthPair(EEchoesShellAction::CameraPanDown, EEchoesShellAction::CameraPanUp);
        CheckEqualWidthPair(EEchoesShellAction::CameraZoomDown, EEchoesShellAction::CameraZoomUp);
        CheckEqualWidthPair(EEchoesShellAction::ResolutionPrevious, EEchoesShellAction::ResolutionNext);
        CheckEqualWidthPair(EEchoesShellAction::MasterDown, EEchoesShellAction::MasterUp);
        CheckEqualWidthPair(EEchoesShellAction::MusicDown, EEchoesShellAction::MusicUp);
        CheckEqualWidthPair(EEchoesShellAction::DialogueDown, EEchoesShellAction::DialogueUp);
        CheckEqualWidthPair(EEchoesShellAction::EffectsDown, EEchoesShellAction::EffectsUp);
        CheckEqualWidthPair(EEchoesShellAction::InterfaceDown, EEchoesShellAction::InterfaceUp);
        CheckEqualWidthPair(EEchoesShellAction::AmbienceDown, EEchoesShellAction::AmbienceUp);
        TestEqual(TEXT("Grouped options begin on the first source action"),
            Widget->GetFocusedButtonIndex(), 0);
        TestTrue(TEXT("Grouped options retain focus traversal into a pair"),
            Widget->FocusNext(false));
        TestEqual(TEXT("Focus traversal preserves the next source index"),
            Widget->GetFocusedButtonIndex(), 1);
    }

    FEchoesShellView BrandedTitle = ShellView(
        EEchoesShellScreen::Title, 1.0f,
        {ShellButton(TEXT("Continue"), EEchoesShellAction::Primary)});
    BrandedTitle.Title = FText::FromString(TEXT("Echoes of the Broken Sun"));
    Widget->SetView(BrandedTitle);
    TArray<UWidget*> TitleDescendants;
    UWidgetTree::GetChildWidgets(Widget->GetRootWidget(), TitleDescendants);
    bool bFoundBalancedMasthead = false;
    for (UWidget* Candidate : TitleDescendants)
    {
        if (const UTextBlock* Text = Cast<UTextBlock>(Candidate))
        {
            bFoundBalancedMasthead = bFoundBalancedMasthead ||
                Text->GetText().ToString() == TEXT("ECHOES OF THE\nBROKEN SUN");
        }
    }
    TestTrue(TEXT("Title uses the deliberate two-line game-name masthead"),
        bFoundBalancedMasthead);

    // A custom rounded brush defaults to zero intrinsic image size. Verify
    // real slider layout, not only the existence of a settings control.
    for (const float UiScale : {.8f, 1.f, 1.5f})
    {
        for (const bool bContrast : {false, true})
        {
            auto Settings = ShellView(EEchoesShellScreen::Options, UiScale,
                {ShellButton(TEXT("Back"), EEchoesShellAction::Back)});
            Settings.bHighContrast = bContrast;
            FEchoesShellSlider ScaleControl;
            ScaleControl.Label = FText::FromString(TEXT("HUD scale"));
            ScaleControl.Value = UiScale;
            Settings.Sliders.Add(ScaleControl);
            Widget->SetView(Settings);
            Widget->TakeWidget()->SlatePrepass(1.f);
            TArray<UWidget*> Descendants;
            UWidgetTree::GetChildWidgets(Widget->GetRootWidget(), Descendants);
            int32 FoundSliders = 0;
            for (UWidget* Child : Descendants)
                if (auto* Slider = Cast<UEchoesShellValueSlider>(Child))
                {
                    ++FoundSliders;
                    TestTrue(TEXT("Settings slider retains a visible, usable thumb at every scale"),
                        Slider->GetWidgetStyle().NormalThumbImage.ImageSize.X >= 12.f &&
                        Slider->GetWidgetStyle().NormalThumbImage.ImageSize.Y >= 24.f);
                    TestTrue(TEXT("Slider row reserves the thumb height"), Slider->GetDesiredSize().Y >= 24.f);
                    TestTrue(TEXT("Hover and disabled thumbs retain identical geometry"),
                        Slider->GetWidgetStyle().NormalThumbImage.ImageSize == Slider->GetWidgetStyle().HoveredThumbImage.ImageSize &&
                        Slider->GetWidgetStyle().NormalThumbImage.ImageSize == Slider->GetWidgetStyle().DisabledThumbImage.ImageSize);
                }
            TestEqual(TEXT("Settings fixture exposes exactly one native slider"), FoundSliders, 1);
        }
    }

    Widget->SetView(ShellView(EEchoesShellScreen::Controls, 1.f,
        {ShellButton(TEXT("Unavailable"), EEchoesShellAction::EditBinding, 1, false),
         ShellButton(TEXT("Editable"), EEchoesShellAction::EditBinding, 2),
         ShellButton(TEXT("Back"), EEchoesShellAction::Back),
         ShellButton(TEXT("Unavailable last"), EEchoesShellAction::EditBinding, 3, false)}));
    Widget->HandleNavigationKey(EKeys::End, false);
    TestEqual(TEXT("End reaches last enabled control"), Widget->GetFocusedButtonIndex(), 2);
    Widget->HandleNavigationKey(EKeys::Home, false);
    TestEqual(TEXT("Home reaches first enabled control"), Widget->GetFocusedButtonIndex(), 1);

    // Incoming history must not reconstruct focused filters or reset reading position.
    FEchoesShellView History = ShellView(EEchoesShellScreen::FeedbackHistory, 1.f,
        {ShellButton(TEXT("All"), EEchoesShellAction::FeedbackHistoryAll),
         ShellButton(TEXT("Orders"), EEchoesShellAction::FeedbackHistoryOrders),
         ShellButton(TEXT("Back"), EEchoesShellAction::Back)});
    History.Body = FText::FromString(FString::ChrN(2000, TEXT('H')));
    Widget->SetView(History);
    Widget->TakeWidget();
    UEchoesShellActionButton* HistoryFilter = Widget->GetActionButton(1);
    Widget->FocusNext(false);
    TArray<UWidget*> HistoryChildren;
    UWidgetTree::GetChildWidgets(Widget->GetRootWidget(), HistoryChildren);
    UScrollBox* HistoryScroll = nullptr;
    for (UWidget* Child : HistoryChildren)
        if (auto* Scroll = Cast<UScrollBox>(Child)) { HistoryScroll = Scroll; break; }
    if (TestNotNull(TEXT("History has a reading viewport"), HistoryScroll))
    {
        HistoryScroll->SetScrollOffset(120.f);
        History.Body = FText::FromString(FString::ChrN(2100, TEXT('H')));
        Widget->SetView(History);
        TestTrue(TEXT("Incoming history preserves the filter widget"), HistoryFilter == Widget->GetActionButton(1));
        TestEqual(TEXT("Incoming history preserves keyboard focus"), Widget->GetFocusedButtonIndex(), 1);
        TestEqual(TEXT("Incoming history preserves requested reading offset"), HistoryScroll->GetScrollOffset(), 120.f);
    }

    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif
