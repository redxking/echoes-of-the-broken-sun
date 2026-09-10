#include "EchoesShellWidget.h"

#include "EchoesInterfaceAudioSubsystem.h"
#include "EchoesPlayerController.h"
#include "EchoesResultChart.h"
#include "EchoesTypeface.h"
#include "Engine/Font.h"

#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "Engine/World.h"
#include "Styling/CoreStyle.h"
#include "Engine/Texture2D.h"
#include "InputCoreTypes.h"
#include "EchoesOfTheBrokenSun.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
constexpr float ShellMaximumWidth = 780.0f;
constexpr float ShellTitleWidth = 420.0f;
constexpr float ShellDialogWidth = 620.0f;

FLinearColor ShellPanelColor(bool bHighContrast)
{
    return bHighContrast
        ? FLinearColor(0.0f, 0.0f, 0.0f, 0.99f)
        : FLinearColor(0.018f, 0.031f, 0.043f, 0.96f);
}

FLinearColor ShellTextColor(bool bHighContrast)
{
    return bHighContrast
        ? FLinearColor::White
        : FLinearColor(0.86f, 0.90f, 0.87f, 1.0f);
}

FLinearColor ShellAccentColor(bool bHighContrast)
{
    return bHighContrast
        ? FLinearColor(0.20f, 1.0f, 1.0f, 1.0f)
        : FLinearColor(0.08f, 0.78f, 0.92f, 1.0f);
}

FLinearColor ShellAmberColor(bool bHighContrast)
{
    return bHighContrast
        ? FLinearColor::Yellow
        : FLinearColor(1.0f, 0.72f, 0.27f, 1.0f);
}

FLinearColor ShellMutedColor(bool bHighContrast)
{
    return bHighContrast
        ? FLinearColor(0.78f, 0.78f, 0.78f, 1.0f)
        : FLinearColor(0.47f, 0.57f, 0.61f, 1.0f);
}

FSlateRoundedBoxBrush ShellFrameBrush(
    const FLinearColor& Fill,
    const FLinearColor& Outline,
    float OutlineWidth = 1.0f)
{
    // Small radii preserve the command-deck's machined, planar character.
    return FSlateRoundedBoxBrush(Fill, 2.0f, Outline, OutlineWidth);
}

bool IsBridgeNavigationScreen(EEchoesShellScreen Screen)
{
    return Screen == EEchoesShellScreen::Title;
}

bool IsDialogScreen(EEchoesShellScreen Screen)
{
    return Screen == EEchoesShellScreen::Confirmation ||
        Screen == EEchoesShellScreen::DisplayConfirmation ||
        Screen == EEchoesShellScreen::Error;
}

bool IsCriticalScreen(EEchoesShellScreen Screen)
{
    return Screen == EEchoesShellScreen::Error ||
        Screen == EEchoesShellScreen::Confirmation ||
        Screen == EEchoesShellScreen::DisplayConfirmation;
}

enum class EEchoesOptionSection : uint8
{
    None,
    Accessibility,
    Controls,
    Camera,
    Display,
    Audio,
};

EEchoesOptionSection OptionSectionFor(EEchoesShellAction Action)
{
    switch (Action)
    {
        case EEchoesShellAction::OpenControls:
            return EEchoesOptionSection::Controls;
        case EEchoesShellAction::HudScaleDown:
        case EEchoesShellAction::HudScaleUp:
        case EEchoesShellAction::HighContrast:
        case EEchoesShellAction::ReducedMotion:
        case EEchoesShellAction::ReducedFlashing:
            return EEchoesOptionSection::Accessibility;
        case EEchoesShellAction::EdgePan:
        case EEchoesShellAction::CameraPanDown:
        case EEchoesShellAction::CameraPanUp:
        case EEchoesShellAction::CameraZoomDown:
        case EEchoesShellAction::CameraZoomUp:
            return EEchoesOptionSection::Camera;
        case EEchoesShellAction::ResolutionPrevious:
        case EEchoesShellAction::ResolutionNext:
        case EEchoesShellAction::WindowMode:
        case EEchoesShellAction::ApplyDisplay:
            return EEchoesOptionSection::Display;
        case EEchoesShellAction::DynamicRange:
        case EEchoesShellAction::MasterDown:
        case EEchoesShellAction::MasterUp:
        case EEchoesShellAction::MusicDown:
        case EEchoesShellAction::MusicUp:
        case EEchoesShellAction::DialogueDown:
        case EEchoesShellAction::DialogueUp:
        case EEchoesShellAction::EffectsDown:
        case EEchoesShellAction::EffectsUp:
        case EEchoesShellAction::InterfaceDown:
        case EEchoesShellAction::InterfaceUp:
        case EEchoesShellAction::AmbienceDown:
        case EEchoesShellAction::AmbienceUp:
            return EEchoesOptionSection::Audio;
        default:
            return EEchoesOptionSection::None;
    }
}

FText OptionSectionTitle(EEchoesOptionSection Section)
{
    switch (Section)
    {
        case EEchoesOptionSection::Accessibility:
            return NSLOCTEXT("EchoesShell", "OptionsAccessibility", "Accessibility");
        case EEchoesOptionSection::Controls:
            return NSLOCTEXT("EchoesShell", "OptionsControls", "Controls");
        case EEchoesOptionSection::Camera:
            return NSLOCTEXT("EchoesShell", "OptionsCamera", "Camera");
        case EEchoesOptionSection::Display:
            return NSLOCTEXT("EchoesShell", "OptionsDisplay", "Display");
        case EEchoesOptionSection::Audio:
            return NSLOCTEXT("EchoesShell", "OptionsAudio", "Audio");
        default:
            return FText::GetEmpty();
    }
}

bool IsExactOptionPair(
    EEchoesShellAction First,
    EEchoesShellAction Second)
{
    return (First == EEchoesShellAction::HudScaleDown && Second == EEchoesShellAction::HudScaleUp) ||
        (First == EEchoesShellAction::CameraPanDown && Second == EEchoesShellAction::CameraPanUp) ||
        (First == EEchoesShellAction::CameraZoomDown && Second == EEchoesShellAction::CameraZoomUp) ||
        (First == EEchoesShellAction::ResolutionPrevious && Second == EEchoesShellAction::ResolutionNext) ||
        (First == EEchoesShellAction::MasterDown && Second == EEchoesShellAction::MasterUp) ||
        (First == EEchoesShellAction::MusicDown && Second == EEchoesShellAction::MusicUp) ||
        (First == EEchoesShellAction::DialogueDown && Second == EEchoesShellAction::DialogueUp) ||
        (First == EEchoesShellAction::EffectsDown && Second == EEchoesShellAction::EffectsUp) ||
        (First == EEchoesShellAction::InterfaceDown && Second == EEchoesShellAction::InterfaceUp) ||
        (First == EEchoesShellAction::AmbienceDown && Second == EEchoesShellAction::AmbienceUp);
}

FText TitlePresentationText(const FEchoesShellView& View)
{
    // The title route keeps its model binding. Only the shipped game name gets
    // the intentional masthead break; test and future localized route titles
    // remain the exact text supplied by the flow model.
    if (View.Screen == EEchoesShellScreen::Title &&
        View.Title.ToString().Equals(TEXT("Echoes of the Broken Sun"),
            ESearchCase::IgnoreCase))
    {
        return NSLOCTEXT("EchoesShell", "BrandedGameTitle", "ECHOES OF THE\nBROKEN SUN");
    }
    return View.Title;
}

/**
 * The shell's branded face. Reading the widget's existing font and changing
 * only its size left every menu, the title and the results screen in stock
 * Roboto while the vendored Space Grotesk shipped unused beside them. A
 * missing font file still degrades to a readable engine face rather than to
 * no text at all, and `EchoesTypeface` logs that fallback.
 */
[[nodiscard]] FSlateFontInfo ShellFont(int32 Size)
{
    UFont* Face = EchoesTypeface::Chrome();
    return Face != nullptr
        ? FSlateFontInfo(Face, Size, TEXT("Regular"))
        : FCoreStyle::GetDefaultFontStyle("Regular", Size);
}

void ConfigureText(
    UTextBlock* Text,
    const FText& Content,
    int32 FontSize,
    const FLinearColor& Color)
{
    Text->SetText(Content);
    Text->SetColorAndOpacity(Color);
    Text->SetAutoWrapText(true);
    Text->SetFont(ShellFont(FontSize));
}

bool SameButton(
    const FEchoesShellButton& Left,
    const FEchoesShellButton& Right)
{
    return Left.Label.EqualTo(Right.Label) && Left.Section.EqualTo(Right.Section) &&
        Left.Action == Right.Action &&
        Left.Argument == Right.Argument &&
        Left.bEnabled == Right.bEnabled;
}
} // namespace

void UEchoesShellValueSlider::Configure(UEchoesShellWidget* InShell, const FEchoesShellSlider& Model)
{
    Shell = InShell;
    Action = Model.Action;
    const bool bHighContrast = InShell != nullptr && InShell->IsHighContrastPresentation();
    const float Scale = InShell != nullptr ? InShell->GetPresentationScale() : 1.0f;
    const FLinearColor Cyan = ShellAccentColor(bHighContrast);
    FSliderStyle Style = GetWidgetStyle();
    Style.NormalBarImage = ShellFrameBrush(
        bHighContrast ? FLinearColor(0.08f, 0.08f, 0.08f, 1.0f) : FLinearColor(0.016f, 0.038f, 0.052f, 1.0f),
        bHighContrast ? FLinearColor::White : FLinearColor(0.13f, 0.32f, 0.40f, 1.0f));
    Style.HoveredBarImage = ShellFrameBrush(
        bHighContrast ? FLinearColor(0.10f, 0.10f, 0.10f, 1.0f) : FLinearColor(0.020f, 0.075f, 0.094f, 1.0f), Cyan);
    Style.DisabledBarImage = ShellFrameBrush(
        FLinearColor(0.02f, 0.02f, 0.02f, 1.0f), ShellMutedColor(bHighContrast).CopyWithNewOpacity(0.45f));
    Style.NormalThumbImage = ShellFrameBrush(Cyan, Cyan, 1.0f);
    Style.HoveredThumbImage = ShellFrameBrush(ShellAmberColor(bHighContrast), ShellAmberColor(bHighContrast), 1.0f);
    Style.DisabledThumbImage = ShellFrameBrush(ShellMutedColor(bHighContrast), ShellMutedColor(bHighContrast), 1.0f);
    // Rounded brushes have no intrinsic size. Give every thumb state a real
    // pointer target; otherwise Slate paints a zero-area thumb and a 4px row.
    Style.NormalThumbImage.ImageSize = FVector2D(FMath::Max(12.f, 12.f * Scale), FMath::Max(24.f, 24.f * Scale));
    Style.HoveredThumbImage.ImageSize = Style.NormalThumbImage.ImageSize;
    Style.DisabledThumbImage.ImageSize = Style.NormalThumbImage.ImageSize;
    Style.BarThickness = 4.0f * Scale;
    SetWidgetStyle(Style);
    SetMinValue(Model.Minimum);
    SetMaxValue(Model.Maximum);
    SetValue(Model.Value);
    IsFocusable = false;
    OnMouseCaptureBegin.AddUniqueDynamic(this, &UEchoesShellValueSlider::BeginEdit);
    OnValueChanged.AddUniqueDynamic(this, &UEchoesShellValueSlider::ChangeValue);
    OnMouseCaptureEnd.AddUniqueDynamic(this, &UEchoesShellValueSlider::EndEdit);
}

void UEchoesShellValueSlider::BeginEdit()
{
    if (UEchoesShellWidget* Current = Shell.Get()) Current->BeginValueEdit();
}

void UEchoesShellValueSlider::ChangeValue(float NewValue)
{
    if (UEchoesShellWidget* Current = Shell.Get()) Current->UpdateValue(Action, NewValue, false);
}

void UEchoesShellValueSlider::EndEdit()
{
    if (UEchoesShellWidget* Current = Shell.Get()) Current->UpdateValue(Action, GetValue(), true);
}

void UEchoesShellWidget::UpdateValue(EEchoesShellAction Action, float Value, bool bCommit)
{
    if (bCommit) bEditingValue = false;
    if (AEchoesPlayerController* Current = ResolveController()) Current->HandleShellValue(Action, Value, bCommit);
}

UEchoesShellActionButton::UEchoesShellActionButton(
    const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    InitIsFocusable(true);
    SetClickMethod(EButtonClickMethod::MouseDown);
    SetTouchMethod(EButtonTouchMethod::Down);
    SetPressMethod(EButtonPressMethod::ButtonPress);
}

void UEchoesShellActionButton::Configure(
    UEchoesShellWidget* InShell,
    AEchoesPlayerController* InController,
    EEchoesShellAction InAction,
    int32 InArgument)
{
    Shell = InShell;
    Controller = InController;
    Action = InAction;
    Argument = InArgument;
    OnReceivedFocus.BindUObject(
        this, &UEchoesShellActionButton::HandleReceivedFocus);
    OnClicked.AddUniqueDynamic(this, &UEchoesShellActionButton::HandleClicked);
    OnHovered.AddUniqueDynamic(this, &UEchoesShellActionButton::HandleHovered);
    OnUnhovered.AddUniqueDynamic(this, &UEchoesShellActionButton::HandleUnhovered);
}

bool UEchoesShellActionButton::Activate()
{
    if (!GetIsEnabled())
    {
        return false;
    }
    if (AEchoesPlayerController* Current = Controller.Get())
    {
#if !UE_BUILD_SHIPPING
        if (FParse::Param(FCommandLine::Get(), TEXT("EchoesShellInputTrace")))
            UE_LOG(LogEchoes, Display, TEXT("[ECHOES_SHELL_INPUT] route=activate action=%d argument=%d"),
                static_cast<int32>(Action), Argument);
#endif
        Current->HandleShellAction(Action, Argument);
    }
    return true;
}

void UEchoesShellActionButton::ApplyPresentation(
    bool bFocused,
    bool bHighContrast)
{
    bPresentationFocused = bFocused;
    bPresentationHighContrast = bHighContrast;
    const FLinearColor Cyan = ShellAccentColor(bHighContrast);
    const FLinearColor Amber = ShellAmberColor(bHighContrast);
    const FLinearColor Resting = bHighContrast
        ? FLinearColor(0.045f, 0.045f, 0.045f, 1.0f)
        : FLinearColor(0.021f, 0.044f, 0.060f, 0.98f);
    const FLinearColor RestingOutline = bHighContrast
        ? FLinearColor::White
        : FLinearColor(0.14f, 0.34f, 0.42f, 0.95f);
    const FLinearColor Disabled = bHighContrast
        ? FLinearColor(0.10f, 0.10f, 0.10f, 1.0f)
        : FLinearColor(0.012f, 0.022f, 0.028f, 0.84f);
    FButtonStyle Style = GetStyle();
    // Keep controls light: the active state is carried by its precise amber
    // outline, not by a large saturated fill that competes with the bridge.
    Style.Normal = ShellFrameBrush(
        bFocused ? FLinearColor(0.095f, 0.066f, 0.026f, 0.98f) : Resting,
        bFocused ? Amber : RestingOutline,
        bFocused ? 2.0f : 1.0f);
    Style.Hovered = ShellFrameBrush(
        FLinearColor(0.018f, 0.105f, 0.135f, 0.98f), Cyan, 2.0f);
    Style.Pressed = ShellFrameBrush(
        bHighContrast ? FLinearColor(0.12f, 0.12f, 0.12f, 1.0f) : FLinearColor(0.025f, 0.19f, 0.24f, 1.0f),
        Cyan, 2.0f);
    Style.Disabled = ShellFrameBrush(Disabled, ShellMutedColor(bHighContrast).CopyWithNewOpacity(0.45f));
    SetStyle(Style);
    // The fill stays dark in every state, so text remains readable over each
    // button and the state distinction does not rely on colour alone.
    if (UBorder* Padding = Cast<UBorder>(GetContent()))
        if (UTextBlock* Label = Cast<UTextBlock>(Padding->GetContent()))
            Label->SetColorAndOpacity(!GetIsEnabled()
                ? ShellMutedColor(bHighContrast)
                : bFocused ? ShellAmberColor(bHighContrast) : ShellTextColor(bHighContrast));
}

void UEchoesShellActionButton::HandleReceivedFocus()
{
    if (UEchoesShellWidget* CurrentShell = Shell.Get())
    {
        CurrentShell->NotifyButtonFocused(this);
    }
}

void UEchoesShellActionButton::HandleClicked()
{
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("EchoesShellInputTrace")))
        UE_LOG(LogEchoes, Display, TEXT("[ECHOES_SHELL_INPUT] route=clicked action=%d"), static_cast<int32>(Action));
#endif
    if (UEchoesShellWidget* CurrentShell = Shell.Get())
    {
        CurrentShell->NotifyButtonFocused(this);
    }
    Activate();
}

void UEchoesShellActionButton::HandleHovered()
{
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("EchoesShellInputTrace")))
        UE_LOG(LogEchoes, Display, TEXT("[ECHOES_SHELL_INPUT] route=hover action=%d"), static_cast<int32>(Action));
#endif
    ApplyPresentation(bPresentationFocused, bPresentationHighContrast);
    if (AEchoesPlayerController* Current = Controller.Get())
    {
        if (UWorld* World = Current->GetWorld())
        {
            if (UEchoesInterfaceAudioSubsystem* Audio =
                World->GetSubsystem<UEchoesInterfaceAudioSubsystem>())
            {
                Audio->PlayInterfaceCue(EEchoesInterfaceCue::Hover);
            }
        }
    }
}

void UEchoesShellActionButton::HandleUnhovered()
{
    ApplyPresentation(bPresentationFocused, bPresentationHighContrast);
}

UEchoesShellWidget::UEchoesShellWidget(
    const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetIsFocusable(true);
    // This is a hard native reference when the imported plate exists, making
    // it discoverable to cooking. LOAD_Quiet keeps an older checkout or a
    // source-only build on the charcoal fallback without a startup failure.
    static ConstructorHelpers::FObjectFinderOptional<UTexture2D> BridgePlate(
        TEXT("/Game/Art/UI/T_EBS_CommandBridge.T_EBS_CommandBridge"), LOAD_Quiet);
    CommandBridgeTexture = BridgePlate.Get();
}

void UEchoesShellWidget::SetView(const FEchoesShellView& InView)
{
    // Keep the active Slate slider alive until pointer release. Changes still
    // update the settings immediately; the final view rebuild follows commit.
    if (bEditingValue && View.Screen == InView.Screen) return;
    bEditingValue = false;
    if (IsSameView(InView))
    {
        return;
    }
    const bool bSameScreen = bHasView && View.Screen == InView.Screen;
    bool bStableTransport = bSameScreen &&
        (InView.Screen == EEchoesShellScreen::ReplayTransport || InView.Screen == EEchoesShellScreen::FeedbackHistory || InView.Screen == EEchoesShellScreen::ResourceMonitor) &&
        View.Scale == InView.Scale && View.bHighContrast == InView.bHighContrast &&
        ActionButtons.Num() == InView.Buttons.Num() && ValueSliders.Num() == InView.Sliders.Num() && TitleText && BodyText && StatusText;
    for (int32 Index = 0; bStableTransport && Index < InView.Buttons.Num(); ++Index)
        bStableTransport = View.Buttons[Index].Action == InView.Buttons[Index].Action && View.Buttons[Index].Argument == InView.Buttons[Index].Argument;
    if (bStableTransport)
    {
        // Playback and incoming history update text without moving focus or
        // recreating the scroll view while the player is reading.
        View = InView;
        TitleText->SetText(View.Title); BodyText->SetText(View.Body); StatusText->SetText(View.Status);
        if (EyebrowText) EyebrowText->SetText(View.Eyebrow);
        if (auto* Frame = Cast<UBorder>(StatusText->GetParent()))
            Frame->SetVisibility(View.Status.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
        for (int32 Index = 0; Index < View.Buttons.Num(); ++Index)
        {
            ActionButtons[Index]->SetIsEnabled(View.Buttons[Index].bEnabled);
            if (auto* Padding = Cast<UBorder>(ActionButtons[Index]->GetContent()))
                if (auto* Label = Cast<UTextBlock>(Padding->GetContent())) Label->SetText(View.Buttons[Index].Label);
        }
        for (int32 Index = 0; Index < View.Sliders.Num(); ++Index)
        {
            SliderLabels[Index]->SetText(View.Sliders[Index].Label);
            ValueSliders[Index]->SetValue(View.Sliders[Index].Value);
        }
        RefreshButtonPresentation();
        return;
    }
    EEchoesShellAction PreviousAction = EEchoesShellAction::Back;
    int32 PreviousArgument = 0;
    const bool bCanRestoreAction = bSameScreen &&
        View.Buttons.IsValidIndex(FocusedButtonIndex) &&
        View.Buttons[FocusedButtonIndex].bEnabled;
    if (bCanRestoreAction)
    {
        PreviousAction = View.Buttons[FocusedButtonIndex].Action;
        PreviousArgument = View.Buttons[FocusedButtonIndex].Argument;
    }
    View = InView;
    bHasView = true;
    RebuildView();
    if (View.Screen == EEchoesShellScreen::ReplayTransport) return;
    bSuppressFocusScroll = View.Screen == EEchoesShellScreen::Results && !bSameScreen;
    if (!(bCanRestoreAction && FocusAction(PreviousAction, PreviousArgument)))
    {
        FocusDefaultButton();
    }
    bSuppressFocusScroll = false;
}

TSharedRef<SWidget> UEchoesShellWidget::RebuildWidget()
{
    // Build the stable UMG root before Slate caches its child on first attachment.
    if (RootOverlay == nullptr) RebuildView();
    return Super::RebuildWidget();
}

void UEchoesShellWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (RootOverlay == nullptr)
    {
        RebuildView();
    }
    if (FocusedButtonIndex == INDEX_NONE)
    {
        FocusNext(false);
    }
    else
    {
        FocusButton(FocusedButtonIndex);
    }
}

void UEchoesShellWidget::NativeTick(
    const FGeometry& MyGeometry,
    float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    // Resize the existing frame only. Rebuilding here would disrupt a held
    // slider, keyboard focus, and the modal input tree.
    UpdatePanelLimits(MyGeometry.GetLocalSize().X, MyGeometry.GetLocalSize().Y);
}

FReply UEchoesShellWidget::NativeOnPreviewKeyDown(
    const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    if (AEchoesPlayerController* Controller = ResolveController(); Controller && Controller->IsCapturingControlBinding())
    {
        if (!InKeyEvent.IsRepeat())
            Controller->CaptureControlBinding(InKeyEvent.GetKey(), InKeyEvent.IsShiftDown(),
                InKeyEvent.IsControlDown(), InKeyEvent.IsAltDown(), InKeyEvent.IsCommandDown());
        return FReply::Handled();
    }
    const FKey Key = InKeyEvent.GetKey();
    // Own navigation before a child SButton substitutes spatial navigation or
    // defers Enter activation to key-up on a now-rebuilt child.
    if (Key == EKeys::Tab || Key == EKeys::Up || Key == EKeys::Down ||
        Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::Escape ||
        Key == EKeys::Home || Key == EKeys::End)
        return NativeOnKeyDown(InGeometry, InKeyEvent);
    return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

FReply UEchoesShellWidget::NativeOnKeyDown(
    const FGeometry& InGeometry,
    const FKeyEvent& InKeyEvent)
{
    const bool bHandled = HandleNavigationKey(InKeyEvent.GetKey(), InKeyEvent.IsShiftDown(), InKeyEvent.IsRepeat());
    if (!bHandled && View.Screen == EEchoesShellScreen::ReplayTransport) return FReply::Unhandled();
    return FReply::Handled();
}

bool UEchoesShellWidget::HandleNavigationKey(const FKey& Key, bool bShift, bool bRepeat)
{
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("EchoesShellInputTrace")))
        UE_LOG(LogEchoes, Display, TEXT("[ECHOES_SHELL_INPUT] route=widget_key key=%s repeat=%s"), *Key.ToString(), bRepeat ? TEXT("true") : TEXT("false"));
#endif
    if (Key == EKeys::Home || Key == EKeys::End)
    {
        const bool bReverse = Key == EKeys::End;
        for (int32 Offset = 0; Offset < ActionButtons.Num(); ++Offset)
        {
            const int32 Index = bReverse ? ActionButtons.Num() - 1 - Offset : Offset;
            if (FocusButton(Index)) break;
        }
    }
    else if (Key == EKeys::Tab)
    {
        FocusNext(bShift);
    }
    else if (Key == EKeys::Up)
    {
        FocusNext(true);
    }
    else if (Key == EKeys::Down)
    {
        FocusNext(false);
    }
    else if (Key == EKeys::Enter || Key == EKeys::SpaceBar)
    {
        if (!bRepeat) ActivateFocused();
    }
    else if (Key == EKeys::Escape)
    {
        if (AEchoesPlayerController* Controller = ResolveController(); Controller && !bRepeat)
        {
            Controller->HandleShellAction(EEchoesShellAction::Back, 0);
        }
    }
    else return false;
    return true;
}

FReply UEchoesShellWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
#if !UE_BUILD_SHIPPING
    static int32 TraceCount = 0;
    if (TraceCount < 24 && FParse::Param(FCommandLine::Get(), TEXT("EchoesShellInputTrace")))
    {
        ++TraceCount;
        const FVector2D Screen = InMouseEvent.GetScreenSpacePosition();
        const FVector2D Local = InGeometry.AbsoluteToLocal(Screen);
        UE_LOG(LogEchoes, Display, TEXT("[ECHOES_SHELL_INPUT] route=mouse_move screen=(%.1f,%.1f) local=(%.1f,%.1f)"), Screen.X, Screen.Y, Local.X, Local.Y);
    }
#endif
    return FReply::Handled();
}

FReply UEchoesShellWidget::NativeOnKeyUp(
    const FGeometry& InGeometry,
    const FKeyEvent& InKeyEvent)
{
    if (View.Screen == EEchoesShellScreen::ReplayTransport) return FReply::Unhandled();
    return FReply::Handled();
}

FReply UEchoesShellWidget::NativeOnPreviewMouseButtonDown(
    const FGeometry& InGeometry,
    const FPointerEvent& InMouseEvent)
{
    if (AEchoesPlayerController* Controller = ResolveController(); Controller && Controller->IsCapturingControlBinding())
    {
        if ((InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton || InMouseEvent.IsTouchEvent()) &&
            ActivateButtonUnderLocation(InMouseEvent.GetScreenSpacePosition()))
            return FReply::Handled();
        Controller->CaptureControlBinding(InMouseEvent.GetEffectingButton(), InMouseEvent.IsShiftDown(),
            InMouseEvent.IsControlDown(), InMouseEvent.IsAltDown(), InMouseEvent.IsCommandDown());
        return FReply::Handled();
    }
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton || InMouseEvent.IsTouchEvent())
    {
        const FVector2D ScreenPos = InMouseEvent.GetScreenSpacePosition();
        if (ActivateButtonUnderLocation(ScreenPos))
        {
            return FReply::Handled();
        }
    }
    return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UEchoesShellWidget::NativeOnMouseButtonDown(
    const FGeometry& InGeometry,
    const FPointerEvent& InMouseEvent)
{
    SetKeyboardFocus();
    // Non-primary presses bubble from Slate buttons. Consume them at the modal
    // boundary without treating right-click or middle-drag as an action.
    if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton && !InMouseEvent.IsTouchEvent())
        return FReply::Handled();
    const FVector2D ScreenPos = InMouseEvent.GetScreenSpacePosition();
    for (int32 Index = 0; Index < ActionButtons.Num(); ++Index)
    {
        UEchoesShellActionButton* Button = ActionButtons[Index];
        if (Button && Button->GetIsEnabled() && Button->GetCachedWidget().IsValid())
        {
            if (Button->GetCachedWidget()->GetTickSpaceGeometry().IsUnderLocation(ScreenPos))
            {
                FocusButton(Index);
                Button->Activate();
                return FReply::Handled();
            }
        }
    }
    return FReply::Handled();
}

FReply UEchoesShellWidget::NativeOnMouseButtonUp(
    const FGeometry& InGeometry,
    const FPointerEvent& InMouseEvent)
{
    return FReply::Handled();
}

FReply UEchoesShellWidget::NativeOnMouseWheel(
    const FGeometry& InGeometry,
    const FPointerEvent& InMouseEvent)
{
    if (AEchoesPlayerController* Controller = ResolveController(); Controller && Controller->IsCapturingControlBinding())
    {
        if (InMouseEvent.GetWheelDelta() != 0)
            Controller->CaptureControlBinding(InMouseEvent.GetWheelDelta() > 0 ? EKeys::MouseScrollUp : EKeys::MouseScrollDown,
                InMouseEvent.IsShiftDown(), InMouseEvent.IsControlDown(), InMouseEvent.IsAltDown(), InMouseEvent.IsCommandDown());
    }
    return FReply::Handled();
}

bool UEchoesShellWidget::ActivateFocused()
{
    return ActionButtons.IsValidIndex(FocusedButtonIndex) &&
        ActionButtons[FocusedButtonIndex]->Activate();
}

bool UEchoesShellWidget::ActivateButtonUnderLocation(const FVector2D& ScreenPosition)
{
    // Slate pointer events and GetTickSpaceGeometry use absolute screen space.
    // Do not reinterpret a miss as local coordinates or a different cursor: an
    // off-target press must never activate a neighbouring menu action.
    for (int32 Index = 0; Index < ActionButtons.Num(); ++Index)
    {
        UEchoesShellActionButton* Button = ActionButtons[Index];
        if (Button && Button->GetIsEnabled() && Button->GetCachedWidget().IsValid())
        {
            const FGeometry& ButtonGeom = Button->GetCachedWidget()->GetTickSpaceGeometry();
            if (ButtonGeom.IsUnderLocation(ScreenPosition))
            {
                FocusButton(Index);
                return Button->Activate();
            }
        }
    }
    return false;
}

bool UEchoesShellWidget::FocusNext(bool bReverse)
{
    if (ActionButtons.IsEmpty())
    {
        FocusedButtonIndex = INDEX_NONE;
        SetKeyboardFocus();
        return false;
    }
    const int32 Direction = bReverse ? -1 : 1;
    const int32 Start = ActionButtons.IsValidIndex(FocusedButtonIndex)
        ? FocusedButtonIndex
        : bReverse ? 0 : ActionButtons.Num() - 1;
    for (int32 Step = 1; Step <= ActionButtons.Num(); ++Step)
    {
        const int32 Index =
            (Start + Direction * Step + ActionButtons.Num() * 2) %
            ActionButtons.Num();
        if (ActionButtons[Index]->GetIsEnabled())
        {
            return FocusButton(Index);
        }
    }
    return false;
}

void UEchoesShellWidget::NotifyButtonFocused(
    UEchoesShellActionButton* Button)
{
    const int32 Index = ActionButtons.IndexOfByKey(Button);
    if (Index != INDEX_NONE && FocusedButtonIndex != Index)
    {
        FocusedButtonIndex = Index;
        RefreshButtonPresentation();
    }
}

bool UEchoesShellWidget::IsSameView(
    const FEchoesShellView& Candidate) const
{
    if (!bHasView || View.Screen != Candidate.Screen ||
        !View.Eyebrow.EqualTo(Candidate.Eyebrow) ||
        !View.Title.EqualTo(Candidate.Title) ||
        !View.Body.EqualTo(Candidate.Body) ||
        !View.Status.EqualTo(Candidate.Status) ||
        View.bHighContrast != Candidate.bHighContrast ||
        !FMath::IsNearlyEqual(View.Scale, Candidate.Scale) ||
        View.Buttons.Num() != Candidate.Buttons.Num() ||
        View.Sliders.Num() != Candidate.Sliders.Num() ||
        View.Charts.Num() != Candidate.Charts.Num())
    {
        return false;
    }
    for (int32 Index = 0; Index < View.Buttons.Num(); ++Index)
    {
        if (!SameButton(View.Buttons[Index], Candidate.Buttons[Index]))
        {
            return false;
        }
    }
    for (int32 Index = 0; Index < View.Sliders.Num(); ++Index)
    {
        const auto& A = View.Sliders[Index];
        const auto& B = Candidate.Sliders[Index];
        if (!A.Label.EqualTo(B.Label) || A.Action != B.Action || A.Value != B.Value ||
            A.Minimum != B.Minimum || A.Maximum != B.Maximum) return false;
    }
    for (int32 Index = 0; Index < View.Charts.Num(); ++Index)
    {
        const auto& A = View.Charts[Index]; const auto& B = Candidate.Charts[Index];
        if (!A.Title.EqualTo(B.Title) || !A.Unit.EqualTo(B.Unit) || A.Series.Num() != B.Series.Num()) return false;
        for (int32 Series = 0; Series < A.Series.Num(); ++Series)
            if (!A.Series[Series].Label.EqualTo(B.Series[Series].Label) ||
                A.Series[Series].Color != B.Series[Series].Color || A.Series[Series].Samples != B.Series[Series].Samples) return false;
    }
    return true;
}

void UEchoesShellWidget::RebuildView()
{
    if (WidgetTree == nullptr)
    {
        return;
    }
    if (RootOverlay == nullptr)
    {
        RootOverlay = WidgetTree->ConstructWidget<UOverlay>();
        WidgetTree->RootWidget = RootOverlay;
        RootOverlay->SetVisibility(ESlateVisibility::Visible);
    }
    else
    {
        RootOverlay->ClearChildren();
    }
    ActionButtons.Reset();
    ValueSliders.Reset();
    SliderLabels.Reset();
    ContentScroll = nullptr;
    CommandBridgeBackground = nullptr;
    WidthLimit = nullptr;
    RequestedPanelWidth = 0.0f;
    PanelSafeInset = 0.0f;
    ResolvedPanelWidth = -1.0f;
    bTitleNavigationLayout = false;
    PanelVerticalInset = 0.0f;
    ResolvedPanelHeight = -1.0f;
    MastheadWidth = nullptr;
    RequestedMastheadWidth = 0.0f;
    ResolvedMastheadWidth = -1.0f;
    EyebrowText = nullptr;
    TitleText = nullptr;
    BodyText = nullptr;
    StatusText = nullptr;
    const float Scale = FMath::Clamp(View.Scale, 0.8f, 1.5f);
    const bool bTransport = View.Screen == EEchoesShellScreen::ReplayTransport;
    const bool bBridgeNavigation = IsBridgeNavigationScreen(View.Screen);
    const bool bDialog = IsDialogScreen(View.Screen);
    const bool bCritical = IsCriticalScreen(View.Screen);
    const bool bShowCommandBridgePlate = bBridgeNavigation && CommandBridgeTexture != nullptr;
    RootOverlay->SetVisibility(bTransport ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Visible);
    const FLinearColor TextColor = ShellTextColor(View.bHighContrast);
    const FLinearColor AccentColor = ShellAccentColor(View.bHighContrast);

    if (bShowCommandBridgePlate)
    {
        UScaleBox* BridgeScale = WidgetTree->ConstructWidget<UScaleBox>();
        BridgeScale->SetStretch(EStretch::ScaleToFill);
        BridgeScale->SetStretchDirection(EStretchDirection::Both);
        BridgeScale->SetClipping(EWidgetClipping::ClipToBounds);
        BridgeScale->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        CommandBridgeBackground = WidgetTree->ConstructWidget<UImage>();
        // Keep the plate's real source ratio as the ScaleBox input; ScaleToFill
        // may crop at a viewport edge but must never squash the bridge.
        CommandBridgeBackground->SetBrushFromTexture(CommandBridgeTexture, true);
        CommandBridgeBackground->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        BridgeScale->AddChild(CommandBridgeBackground);
        UOverlaySlot* BridgeSlot = RootOverlay->AddChildToOverlay(BridgeScale);
        BridgeSlot->SetHorizontalAlignment(HAlign_Fill);
        BridgeSlot->SetVerticalAlignment(VAlign_Fill);
    }

    UBorder* Scrim = WidgetTree->ConstructWidget<UBorder>();
    // The title gets only a light global veil; localized panel backing carries
    // the readability burden so the observation-deck plate stays visible.
    Scrim->SetBrushColor(FLinearColor(0.002f, 0.008f, 0.015f,
        bShowCommandBridgePlate ? 0.12f : bBridgeNavigation ? 0.52f : 0.74f));
    if (bTransport) Scrim->SetVisibility(ESlateVisibility::Collapsed);
    UOverlaySlot* ScrimSlot = RootOverlay->AddChildToOverlay(Scrim);
    ScrimSlot->SetHorizontalAlignment(HAlign_Fill);
    ScrimSlot->SetVerticalAlignment(VAlign_Fill);

    if (bBridgeNavigation)
    {
        MastheadWidth = WidgetTree->ConstructWidget<USizeBox>();
        RequestedMastheadWidth = 640.0f * Scale;
        MastheadWidth->SetWidthOverride(RequestedMastheadWidth);
        MastheadWidth->SetMaxDesiredWidth(RequestedMastheadWidth);
        MastheadWidth->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        UVerticalBox* Masthead = WidgetTree->ConstructWidget<UVerticalBox>();
        MastheadWidth->SetContent(Masthead);

        EyebrowText = WidgetTree->ConstructWidget<UTextBlock>();
        ConfigureText(EyebrowText, View.Eyebrow,
            FMath::RoundToInt(11.0f * Scale), AccentColor);
        EyebrowText->SetJustification(ETextJustify::Center);
        EyebrowText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        Masthead->AddChildToVerticalBox(EyebrowText)->SetPadding(
            FMargin(0.0f, 0.0f, 0.0f, 5.0f * Scale));

        TitleText = WidgetTree->ConstructWidget<UTextBlock>();
        ConfigureText(TitleText, TitlePresentationText(View),
            FMath::RoundToInt(42.0f * Scale), TextColor);
        TitleText->SetJustification(ETextJustify::Center);
        TitleText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        Masthead->AddChildToVerticalBox(TitleText)->SetPadding(
            FMargin(0.0f, 0.0f, 0.0f, 6.0f * Scale));

        BodyText = WidgetTree->ConstructWidget<UTextBlock>();
        ConfigureText(BodyText, View.Body,
            FMath::RoundToInt(14.0f * Scale), TextColor.CopyWithNewOpacity(0.92f));
        BodyText->SetLineHeightPercentage(1.10f);
        BodyText->SetJustification(ETextJustify::Center);
        BodyText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        Masthead->AddChildToVerticalBox(BodyText);

        UOverlaySlot* MastheadSlot = RootOverlay->AddChildToOverlay(MastheadWidth);
        MastheadSlot->SetHorizontalAlignment(HAlign_Center);
        MastheadSlot->SetVerticalAlignment(VAlign_Top);
        MastheadSlot->SetPadding(FMargin(24.0f * Scale, 28.0f * Scale, 24.0f * Scale, 0.0f));
    }

    UBorder* SafeFrame = WidgetTree->ConstructWidget<UBorder>();
    SafeFrame->SetBrushColor(FLinearColor::Transparent);
    SafeFrame->SetPadding(FMargin((bBridgeNavigation ? 48.0f : 24.0f) * Scale));
    SafeFrame->SetHorizontalAlignment(bTransport ? HAlign_Right : bBridgeNavigation ? HAlign_Left : HAlign_Center);
    // A short ordinary route should size to its actual content and be centered
    // over the world. The SizeBox still caps long menus, where its scroll box
    // owns the overflow, while title and transport retain their distinct
    // anchors.
    SafeFrame->SetVerticalAlignment(bTransport ? VAlign_Top : bBridgeNavigation ? VAlign_Bottom : VAlign_Center);
    SafeFrame->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    UOverlaySlot* SafeSlot = RootOverlay->AddChildToOverlay(SafeFrame);
    SafeSlot->SetHorizontalAlignment(HAlign_Fill);
    SafeSlot->SetVerticalAlignment(bBridgeNavigation ? VAlign_Bottom : VAlign_Fill);

    WidthLimit = WidgetTree->ConstructWidget<USizeBox>();
    const float PanelWidth = bTransport ? 550.f : bBridgeNavigation ? ShellTitleWidth : bDialog ? ShellDialogWidth : ShellMaximumWidth;
    RequestedPanelWidth = PanelWidth * Scale;
    const FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(this);
    const float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(this);
    const float LogicalViewportWidth = ViewportScale > KINDA_SMALL_NUMBER
        ? ViewportSize.X / ViewportScale
        : ViewportSize.X;
    PanelSafeInset = (bBridgeNavigation ? 48.0f : 24.0f) * Scale;
    bTitleNavigationLayout = bBridgeNavigation;
    PanelVerticalInset = (bBridgeNavigation ? 48.0f : 24.0f) * Scale;
    // At 150% HUD scale a native title deck is 630px wide. Clamp its actual
    // width to the currently available safe area rather than relying on a
    // SizeBox overflow that can hide controls on a narrow window.
    UpdatePanelLimits(LogicalViewportWidth,
        ViewportScale > KINDA_SMALL_NUMBER ? ViewportSize.Y / ViewportScale : ViewportSize.Y);
    if (bTransport) WidthLimit->SetHeightOverride(290 * Scale);
    SafeFrame->SetContent(WidthLimit);

    UBorder* OuterFrame = WidgetTree->ConstructWidget<UBorder>();
    const FLinearColor FrameColor = bCritical
        ? ShellAmberColor(View.bHighContrast)
        : ShellAccentColor(View.bHighContrast);
    OuterFrame->SetBrush(ShellFrameBrush(
        FLinearColor(0.003f, 0.012f, 0.020f, 0.78f),
        FrameColor.CopyWithNewOpacity(View.bHighContrast ? 1.0f : 0.70f),
        bCritical ? 2.0f : 1.0f));
    OuterFrame->SetPadding(FMargin(2.0f * Scale));
    WidthLimit->SetContent(OuterFrame);

    UBorder* Panel = WidgetTree->ConstructWidget<UBorder>();
    Panel->SetBrush(ShellFrameBrush(ShellPanelColor(View.bHighContrast),
        FLinearColor(0.0f, 0.0f, 0.0f, 0.0f)));
    Panel->SetPadding(FMargin((bTransport ? 12.f : bBridgeNavigation ? 24.f : 28.f) * Scale));
    OuterFrame->SetContent(Panel);

    ContentScroll = WidgetTree->ConstructWidget<UScrollBox>();
    ContentScroll->SetConsumeMouseWheel(View.Screen == EEchoesShellScreen::ControlCapture
        ? EConsumeMouseWheel::Never : EConsumeMouseWheel::Always);
    ContentScroll->SetScrollBarVisibility(ESlateVisibility::Visible);
    Panel->SetContent(ContentScroll);

    UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>();
    ContentScroll->AddChild(Content);

    UBorder* HeaderRule = WidgetTree->ConstructWidget<UBorder>();
    HeaderRule->SetBrushColor(FrameColor.CopyWithNewOpacity(View.bHighContrast ? 1.0f : 0.86f));
    HeaderRule->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    Content->AddChildToVerticalBox(HeaderRule);
    if (UVerticalBoxSlot* HeaderRuleSlot = Cast<UVerticalBoxSlot>(HeaderRule->Slot))
    {
        HeaderRuleSlot->SetHorizontalAlignment(HAlign_Fill);
        HeaderRuleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f * Scale));
    }
    HeaderRule->SetPadding(FMargin(0.0f, 1.0f * Scale));

    if (!bBridgeNavigation)
    {
        EyebrowText = WidgetTree->ConstructWidget<UTextBlock>();
        ConfigureText(EyebrowText, View.Eyebrow,
            FMath::RoundToInt(11.0f * Scale), AccentColor);
        if (bTransport) EyebrowText->SetVisibility(ESlateVisibility::Collapsed);
        Content->AddChildToVerticalBox(EyebrowText)->SetPadding(
            FMargin(0.0f, 0.0f, 0.0f, 8.0f * Scale));

        TitleText = WidgetTree->ConstructWidget<UTextBlock>();
        ConfigureText(TitleText, View.Title,
            FMath::RoundToInt((bTransport ? 18.f : 28.f) * Scale), TextColor);
        Content->AddChildToVerticalBox(TitleText)->SetPadding(
            FMargin(0.0f, 0.0f, 0.0f, 18.0f * Scale));

        BodyText = WidgetTree->ConstructWidget<UTextBlock>();
        ConfigureText(BodyText, View.Body,
            FMath::RoundToInt(16.f * Scale), TextColor);
        BodyText->SetLineHeightPercentage(1.15f);
        // Command history keeps its filters ahead of the potentially long
        // event list, so changing category never requires reaching its end.
        if (View.Screen != EEchoesShellScreen::FeedbackHistory && View.Screen != EEchoesShellScreen::ResourceMonitor)
            Content->AddChildToVerticalBox(BodyText)->SetPadding(
                FMargin(0.0f, 0.0f, 0.0f, 18.0f * Scale));
    }

    UBorder* StatusFrame = WidgetTree->ConstructWidget<UBorder>();
    StatusFrame->SetBrush(ShellFrameBrush(
        View.bHighContrast
            ? FLinearColor(0.12f, 0.12f, 0.12f, 1.0f)
            : FLinearColor(0.095f, 0.066f, 0.026f, 0.98f),
        ShellAmberColor(View.bHighContrast).CopyWithNewOpacity(0.75f)));
    StatusFrame->SetPadding(FMargin(12.0f * Scale));
    StatusText = WidgetTree->ConstructWidget<UTextBlock>();
    ConfigureText(StatusText, View.Status,
        FMath::RoundToInt(14.0f * Scale),
        View.bHighContrast ? FLinearColor::Yellow : FLinearColor(0.95f, 0.70f, 0.24f, 1.0f));
    StatusFrame->SetContent(StatusText);
    StatusFrame->SetVisibility(View.Status.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    Content->AddChildToVerticalBox(StatusFrame)->SetPadding(
        FMargin(0.0f, 0.0f, 0.0f, 20.0f * Scale));

    for (const FEchoesShellChart& Model : View.Charts)
    {
        UTextBlock* Heading = WidgetTree->ConstructWidget<UTextBlock>();
        ConfigureText(Heading, FText::Format(NSLOCTEXT("EchoesResults", "ChartHeading", "{0} ({1})"), Model.Title, Model.Unit),
            FMath::RoundToInt(18 * Scale), TextColor);
        Content->AddChildToVerticalBox(Heading)->SetPadding(FMargin(0, 14 * Scale, 0, 8 * Scale));
        UEchoesResultChart* Chart = WidgetTree->ConstructWidget<UEchoesResultChart>();
        Chart->SetChart(Model);
        Content->AddChildToVerticalBox(Chart)->SetPadding(FMargin(0, 0, 0, 8 * Scale));
        TArray<FText> Legend;
        for (int32 Index = 0; Index < Model.Series.Num(); ++Index)
            Legend.Add(FText::Format(NSLOCTEXT("EchoesResults", "ChartLegend", "{0}: {1}"), Model.Series[Index].Label,
                Index == 0 ? NSLOCTEXT("EchoesResults", "Solid", "solid") :
                Index == 1 ? NSLOCTEXT("EchoesResults", "ShortDash", "short dash") :
                Index == 2 ? NSLOCTEXT("EchoesResults", "MediumDash", "medium dash") : NSLOCTEXT("EchoesResults", "LongDash", "long dash")));
        UTextBlock* LegendText = WidgetTree->ConstructWidget<UTextBlock>();
        ConfigureText(LegendText, FText::Join(FText::FromString(TEXT(" · ")), Legend), FMath::RoundToInt(14 * Scale), TextColor);
        Content->AddChildToVerticalBox(LegendText)->SetPadding(FMargin(0, 0, 0, 16 * Scale));
    }

    const bool bOptionsScreen = View.Screen == EEchoesShellScreen::Options;
    if (bOptionsScreen && !View.Sliders.IsEmpty())
    {
        UTextBlock* Heading = WidgetTree->ConstructWidget<UTextBlock>();
        ConfigureText(Heading,
            OptionSectionTitle(EEchoesOptionSection::Accessibility),
            FMath::RoundToInt(13.0f * Scale), AccentColor);
        Heading->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        Content->AddChildToVerticalBox(Heading)->SetPadding(
            FMargin(0.0f, 4.0f * Scale, 0.0f, 7.0f * Scale));
    }
    for (const FEchoesShellSlider& Model : View.Sliders)
    {
        UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
        ConfigureText(Label, Model.Label, FMath::RoundToInt(15.0f * Scale), TextColor);
        Content->AddChildToVerticalBox(Label)->SetPadding(FMargin(0, 0, 0, 8 * Scale));
        UEchoesShellValueSlider* Slider = WidgetTree->ConstructWidget<UEchoesShellValueSlider>();
        Slider->Configure(this, Model);
        ValueSliders.Add(Slider);
        SliderLabels.Add(Label);
        Content->AddChildToVerticalBox(Slider)->SetPadding(FMargin(8 * Scale, 0, 8 * Scale, 20 * Scale));
    }

    UWrapBox* TransportButtons = nullptr;
    if (bTransport || View.Screen == EEchoesShellScreen::FeedbackHistory || View.Screen == EEchoesShellScreen::ResourceMonitor)
    {
        TransportButtons = WidgetTree->ConstructWidget<UWrapBox>();
        TransportButtons->SetInnerSlotPadding(FVector2D(8, 8));
        Content->AddChildToVerticalBox(TransportButtons);
    }
    const bool bOptionsLayout = bOptionsScreen;
    const auto CreateButton = [this, Scale, bBridgeNavigation, TextColor](
                                  const FEchoesShellButton& ButtonModel)
    {
        UEchoesShellActionButton* Button =
            WidgetTree->ConstructWidget<UEchoesShellActionButton>();
        Button->Configure(
            this,
            ResolveController(),
            ButtonModel.Action,
            ButtonModel.Argument);
        Button->SetIsEnabled(ButtonModel.bEnabled);

        UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
        ConfigureText(Label, ButtonModel.Label,
            FMath::RoundToInt((bBridgeNavigation ? 14.0f : 16.0f) * Scale), TextColor);
        Label->SetJustification(ETextJustify::Center);
        Label->SetVisibility(ESlateVisibility::HitTestInvisible);
        UBorder* ButtonPadding = WidgetTree->ConstructWidget<UBorder>();
        ButtonPadding->SetBrushColor(FLinearColor::Transparent);
        ButtonPadding->SetPadding(
            FMargin((bBridgeNavigation ? 14.0f : 18.0f) * Scale,
                (bBridgeNavigation ? 6.0f : 10.0f) * Scale));
        ButtonPadding->SetVisibility(ESlateVisibility::HitTestInvisible);
        ButtonPadding->SetContent(Label);
        Button->SetContent(ButtonPadding);
        if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(ButtonPadding->Slot))
            ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
        ActionButtons.Add(Button);
        return Button;
    };
    const auto AddFullWidthButton = [Content, TransportButtons, Scale, bBridgeNavigation](
                                        UEchoesShellActionButton* Button)
    {
        if (TransportButtons != nullptr)
        {
            TransportButtons->AddChildToWrapBox(Button);
            return;
        }
        Content->AddChildToVerticalBox(Button)->SetPadding(
            FMargin(0.0f, 0.0f, 0.0f, (bBridgeNavigation ? 4.0f : 9.0f) * Scale));
    };

    EEchoesOptionSection PreviousSection =
        bOptionsLayout && !View.Sliders.IsEmpty()
            ? EEchoesOptionSection::Accessibility
            : EEchoesOptionSection::None;
    FText PreviousModelSection;
    for (int32 ButtonIndex = 0; ButtonIndex < View.Buttons.Num(); ++ButtonIndex)
    {
        const FEchoesShellButton& ButtonModel = View.Buttons[ButtonIndex];
        if (!ButtonModel.Section.IsEmpty() && !ButtonModel.Section.EqualTo(PreviousModelSection))
        {
            UTextBlock* Heading = WidgetTree->ConstructWidget<UTextBlock>();
            ConfigureText(Heading, ButtonModel.Section, FMath::RoundToInt(16 * Scale), AccentColor);
            Content->AddChildToVerticalBox(Heading)->SetPadding(FMargin(0, 14 * Scale, 0, 8 * Scale));
        }
        PreviousModelSection = ButtonModel.Section;
        const EEchoesOptionSection Section = bOptionsLayout
            ? OptionSectionFor(ButtonModel.Action)
            : EEchoesOptionSection::None;
        if (Section != EEchoesOptionSection::None && Section != PreviousSection)
        {
            UTextBlock* Heading = WidgetTree->ConstructWidget<UTextBlock>();
            ConfigureText(Heading, OptionSectionTitle(Section),
                FMath::RoundToInt(13.0f * Scale), AccentColor);
            Heading->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
            Content->AddChildToVerticalBox(Heading)->SetPadding(
                FMargin(0.0f, PreviousSection == EEchoesOptionSection::None ? 4.0f * Scale : 14.0f * Scale,
                    0.0f, 7.0f * Scale));
        }
        PreviousSection = Section;

        UEchoesShellActionButton* FirstButton = CreateButton(ButtonModel);
        const bool bPair = bOptionsLayout && View.Buttons.IsValidIndex(ButtonIndex + 1) &&
            IsExactOptionPair(ButtonModel.Action, View.Buttons[ButtonIndex + 1].Action);
        if (!bPair)
        {
            AddFullWidthButton(FirstButton);
            continue;
        }

        // Pair only the known adjacent contracts. Both source entries are
        // still created and appended in model order, preserving focus and
        // ActionButtons index identity while presenting equal-width controls.
        UHorizontalBox* PairRow = WidgetTree->ConstructWidget<UHorizontalBox>();
        UHorizontalBoxSlot* FirstSlot = PairRow->AddChildToHorizontalBox(FirstButton);
        FirstSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        FirstSlot->SetPadding(FMargin(0.0f, 0.0f, 4.0f * Scale, 0.0f));
        UEchoesShellActionButton* SecondButton = CreateButton(View.Buttons[++ButtonIndex]);
        UHorizontalBoxSlot* SecondSlot = PairRow->AddChildToHorizontalBox(SecondButton);
        SecondSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        SecondSlot->SetPadding(FMargin(4.0f * Scale, 0.0f, 0.0f, 0.0f));
        Content->AddChildToVerticalBox(PairRow)->SetPadding(
            FMargin(0.0f, 0.0f, 0.0f, 9.0f * Scale));
    }
    if ((View.Screen == EEchoesShellScreen::FeedbackHistory || View.Screen == EEchoesShellScreen::ResourceMonitor) && BodyText)
        Content->AddChildToVerticalBox(BodyText)->SetPadding(FMargin(0, 14 * Scale, 0, 0));
    FocusedButtonIndex = INDEX_NONE;
    RefreshButtonPresentation();
}

void UEchoesShellWidget::UpdatePanelLimits(
    float LogicalViewportWidth,
    float LogicalViewportHeight)
{
    if (WidthLimit == nullptr || RequestedPanelWidth <= 0.0f)
    {
        return;
    }
    const float AvailableWidth = LogicalViewportWidth > 0.0f
        ? FMath::Max(1.0f, LogicalViewportWidth - PanelSafeInset * 2.0f)
        : RequestedPanelWidth;
    const float NewWidth = FMath::Min(RequestedPanelWidth, AvailableWidth);
    if (!FMath::IsNearlyEqual(ResolvedPanelWidth, NewWidth))
    {
        ResolvedPanelWidth = NewWidth;
        WidthLimit->SetWidthOverride(NewWidth);
        WidthLimit->SetMaxDesiredWidth(NewWidth);
    }
    if (MastheadWidth != nullptr && RequestedMastheadWidth > 0.0f)
    {
        const float NewMastheadWidth = LogicalViewportWidth > 0.0f
            ? FMath::Min(RequestedMastheadWidth,
                FMath::Max(1.0f, LogicalViewportWidth - PanelSafeInset * 2.0f))
            : RequestedMastheadWidth;
        if (!FMath::IsNearlyEqual(ResolvedMastheadWidth, NewMastheadWidth))
        {
            ResolvedMastheadWidth = NewMastheadWidth;
            MastheadWidth->SetWidthOverride(NewMastheadWidth);
            MastheadWidth->SetMaxDesiredWidth(NewMastheadWidth);
        }
    }
    if (LogicalViewportHeight <= 0.0f)
    {
        return;
    }
    if (View.Screen == EEchoesShellScreen::ReplayTransport)
    {
        return;
    }
    // This is a ceiling, not a forced height. Briefings and confirmations
    // therefore stay as compact as their content, while long options and
    // results views receive a bounded scroll viewport. The title keeps its
    // lower-left composition within a smaller observation-deck envelope.
    const float NewHeight = FMath::Max(1.0f, FMath::Min(
        bTitleNavigationLayout ? LogicalViewportHeight * 0.58f : LogicalViewportHeight - PanelVerticalInset * 2.0f,
        LogicalViewportHeight - PanelVerticalInset * 2.0f));
    if (!FMath::IsNearlyEqual(ResolvedPanelHeight, NewHeight))
    {
        ResolvedPanelHeight = NewHeight;
        WidthLimit->SetMaxDesiredHeight(NewHeight);
    }
}

bool UEchoesShellWidget::FocusButton(int32 Index)
{
    if (!ActionButtons.IsValidIndex(Index) ||
        !ActionButtons[Index]->GetIsEnabled())
    {
        return false;
    }
    FocusedButtonIndex = Index;
    RefreshButtonPresentation();
    ActionButtons[Index]->SetKeyboardFocus();
    if (ContentScroll != nullptr && !bSuppressFocusScroll)
    {
        ContentScroll->ScrollWidgetIntoView(
            ActionButtons[Index],
            false,
            EDescendantScrollDestination::IntoView);
    }
    return true;
}

bool UEchoesShellWidget::FocusAction(
    EEchoesShellAction Action,
    int32 Argument)
{
    for (int32 Index = 0; Index < View.Buttons.Num(); ++Index)
    {
        const FEchoesShellButton& Button = View.Buttons[Index];
        if (Button.bEnabled && Button.Action == Action &&
            Button.Argument == Argument)
        {
            return FocusButton(Index);
        }
    }
    return false;
}

bool UEchoesShellWidget::FocusDefaultButton()
{
    FocusedButtonIndex = INDEX_NONE;
    if (View.Screen == EEchoesShellScreen::Confirmation)
    {
        for (int32 Index = 0; Index < View.Buttons.Num(); ++Index)
        {
            const FEchoesShellButton& Button = View.Buttons[Index];
            if (Button.bEnabled &&
                Button.Action == EEchoesShellAction::Cancel)
            {
                return FocusButton(Index);
            }
        }
    }
    return FocusNext(false);
}

void UEchoesShellWidget::RefreshButtonPresentation()
{
    for (int32 Index = 0; Index < ActionButtons.Num(); ++Index)
    {
        ActionButtons[Index]->ApplyPresentation(
            Index == FocusedButtonIndex,
            View.bHighContrast);
    }
}

AEchoesPlayerController* UEchoesShellWidget::ResolveController() const
{
    return Cast<AEchoesPlayerController>(GetOwningPlayer());
}
