#include "EchoesShellWidget.h"

#include "EchoesInterfaceAudioSubsystem.h"
#include "EchoesPlayerController.h"
#include "EchoesResultChart.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/ButtonSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"
#include "EchoesOfTheBrokenSun.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
constexpr float ShellMaximumWidth = 900.0f;

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

void ConfigureText(
    UTextBlock* Text,
    const FText& Content,
    int32 FontSize,
    const FLinearColor& Color)
{
    Text->SetText(Content);
    Text->SetColorAndOpacity(Color);
    Text->SetAutoWrapText(true);
    FSlateFontInfo Font = Text->GetFont();
    Font.Size = FontSize;
    Text->SetFont(Font);
}

bool SameButton(
    const FEchoesShellButton& Left,
    const FEchoesShellButton& Right)
{
    return Left.Label.EqualTo(Right.Label) &&
        Left.Action == Right.Action &&
        Left.Argument == Right.Argument &&
        Left.bEnabled == Right.bEnabled;
}
} // namespace

void UEchoesShellValueSlider::Configure(UEchoesShellWidget* InShell, const FEchoesShellSlider& Model)
{
    Shell = InShell;
    Action = Model.Action;
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
    SetClickMethod(EButtonClickMethod::PreciseClick);
    SetTouchMethod(EButtonTouchMethod::PreciseTap);
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
    AEchoesPlayerController* Current = Controller.Get();
    if (!GetIsEnabled() || Current == nullptr)
    {
        return false;
    }
    Current->HandleShellAction(Action, Argument);
    return true;
}

void UEchoesShellActionButton::ApplyPresentation(
    bool bFocused,
    bool bHighContrast)
{
    bPresentationFocused = bFocused;
    bPresentationHighContrast = bHighContrast;
    const FLinearColor Accent = ShellAccentColor(bHighContrast);
    const FLinearColor Resting = bHighContrast
        ? FLinearColor(0.08f, 0.08f, 0.08f, 1.0f)
        : FLinearColor(0.045f, 0.075f, 0.095f, 1.0f);
    FButtonStyle Style = GetStyle();
    Style.Normal.TintColor = FSlateColor(bFocused ? Accent : Resting);
    Style.Hovered.TintColor = FSlateColor(Accent);
    Style.Pressed.TintColor = FSlateColor(
        bHighContrast
            ? FLinearColor::White
            : FLinearColor(0.03f, 0.48f, 0.62f, 1.0f));
    Style.Disabled.TintColor = FSlateColor(
        FLinearColor(0.025f, 0.035f, 0.04f, 0.72f));
    SetStyle(Style);
    // Light labels lose contrast over the bright focus/hover accent.
    if (UBorder* Padding = Cast<UBorder>(GetContent()))
        if (UTextBlock* Label = Cast<UTextBlock>(Padding->GetContent()))
            Label->SetColorAndOpacity(bFocused || IsHovered()
                ? FLinearColor(0.006f, 0.012f, 0.016f, 1.0f) : ShellTextColor(bHighContrast));
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
    bool bStableTransport = bSameScreen && InView.Screen == EEchoesShellScreen::ReplayTransport &&
        View.Scale == InView.Scale && View.bHighContrast == InView.bHighContrast &&
        ActionButtons.Num() == InView.Buttons.Num() && ValueSliders.Num() == InView.Sliders.Num() && TitleText && BodyText && StatusText;
    for (int32 Index = 0; bStableTransport && Index < InView.Buttons.Num(); ++Index)
        bStableTransport = View.Buttons[Index].Action == InView.Buttons[Index].Action && View.Buttons[Index].Argument == InView.Buttons[Index].Argument;
    if (bStableTransport)
    {
        // Advancing playback updates numbers, not the input tree under a held key or pointer.
        View = InView;
        TitleText->SetText(View.Title); BodyText->SetText(View.Body); StatusText->SetText(View.Status);
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

FReply UEchoesShellWidget::NativeOnPreviewKeyDown(
    const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    const FKey Key = InKeyEvent.GetKey();
    // Own navigation before a child SButton substitutes spatial navigation or
    // defers Enter activation to key-up on a now-rebuilt child.
    if (Key == EKeys::Tab || Key == EKeys::Up || Key == EKeys::Down ||
        Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::Escape)
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
    if (Key == EKeys::Tab)
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
    return FReply::Handled();
}

bool UEchoesShellWidget::ActivateFocused()
{
    return ActionButtons.IsValidIndex(FocusedButtonIndex) &&
        ActionButtons[FocusedButtonIndex]->Activate();
}

bool UEchoesShellWidget::ActivateButtonUnderLocation(const FVector2D& ScreenPosition)
{
    FVector2D AbsolutePos = ScreenPosition;
    if (GetCachedWidget().IsValid())
    {
        AbsolutePos = GetCachedGeometry().LocalToAbsolute(ScreenPosition);
    }
    const FVector2D SlateCursorPos = FSlateApplication::IsInitialized()
        ? FSlateApplication::Get().GetCursorPos()
        : AbsolutePos;

    for (int32 Index = 0; Index < ActionButtons.Num(); ++Index)
    {
        UEchoesShellActionButton* Button = ActionButtons[Index];
        if (Button && Button->GetIsEnabled() && Button->GetCachedWidget().IsValid())
        {
            const FGeometry& ButtonGeom = Button->GetCachedWidget()->GetTickSpaceGeometry();
            if (ButtonGeom.IsUnderLocation(AbsolutePos) ||
                ButtonGeom.IsUnderLocation(ScreenPosition) ||
                ButtonGeom.IsUnderLocation(SlateCursorPos))
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
    EyebrowText = nullptr;
    TitleText = nullptr;
    BodyText = nullptr;
    StatusText = nullptr;
    const float Scale = FMath::Clamp(View.Scale, 0.8f, 1.5f);
    const bool bTransport = View.Screen == EEchoesShellScreen::ReplayTransport;
    RootOverlay->SetVisibility(bTransport ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Visible);
    const FLinearColor TextColor = ShellTextColor(View.bHighContrast);
    const FLinearColor AccentColor = ShellAccentColor(View.bHighContrast);

    UBorder* Scrim = WidgetTree->ConstructWidget<UBorder>();
    Scrim->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.68f));
    if (bTransport) Scrim->SetVisibility(ESlateVisibility::Collapsed);
    UOverlaySlot* ScrimSlot = RootOverlay->AddChildToOverlay(Scrim);
    ScrimSlot->SetHorizontalAlignment(HAlign_Fill);
    ScrimSlot->SetVerticalAlignment(VAlign_Fill);

    UBorder* SafeFrame = WidgetTree->ConstructWidget<UBorder>();
    SafeFrame->SetBrushColor(FLinearColor::Transparent);
    SafeFrame->SetPadding(FMargin(24.0f * Scale));
    SafeFrame->SetHorizontalAlignment(bTransport ? HAlign_Right : HAlign_Center);
    SafeFrame->SetVerticalAlignment(bTransport ? VAlign_Top : VAlign_Fill);
    if (bTransport) SafeFrame->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    UOverlaySlot* SafeSlot = RootOverlay->AddChildToOverlay(SafeFrame);
    SafeSlot->SetHorizontalAlignment(HAlign_Fill);
    SafeSlot->SetVerticalAlignment(VAlign_Fill);

    USizeBox* WidthLimit = WidgetTree->ConstructWidget<USizeBox>();
    WidthLimit->SetWidthOverride(bTransport ? 550.f : ShellMaximumWidth);
    WidthLimit->SetMaxDesiredWidth(bTransport ? 550.f : ShellMaximumWidth);
    if (bTransport) WidthLimit->SetHeightOverride(290 * Scale);
    SafeFrame->SetContent(WidthLimit);

    UBorder* Panel = WidgetTree->ConstructWidget<UBorder>();
    Panel->SetBrushColor(ShellPanelColor(View.bHighContrast));
    Panel->SetPadding(FMargin((bTransport ? 12.f : 30.f) * Scale));
    WidthLimit->SetContent(Panel);

    ContentScroll = WidgetTree->ConstructWidget<UScrollBox>();
    ContentScroll->SetConsumeMouseWheel(EConsumeMouseWheel::Always);
    ContentScroll->SetScrollBarVisibility(ESlateVisibility::Visible);
    Panel->SetContent(ContentScroll);

    UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>();
    ContentScroll->AddChild(Content);

    EyebrowText = WidgetTree->ConstructWidget<UTextBlock>();
    ConfigureText(EyebrowText, View.Eyebrow,
        FMath::RoundToInt(12.0f * Scale), AccentColor);
    if (bTransport) EyebrowText->SetVisibility(ESlateVisibility::Collapsed);
    Content->AddChildToVerticalBox(EyebrowText)->SetPadding(
        FMargin(0.0f, 0.0f, 0.0f, 8.0f * Scale));

    TitleText = WidgetTree->ConstructWidget<UTextBlock>();
    ConfigureText(TitleText, View.Title,
        FMath::RoundToInt((bTransport ? 18.f : 30.f) * Scale), TextColor);
    Content->AddChildToVerticalBox(TitleText)->SetPadding(
        FMargin(0.0f, 0.0f, 0.0f, 18.0f * Scale));

    BodyText = WidgetTree->ConstructWidget<UTextBlock>();
    ConfigureText(BodyText, View.Body,
        FMath::RoundToInt(16.0f * Scale), TextColor);
    BodyText->SetLineHeightPercentage(1.15f);
    Content->AddChildToVerticalBox(BodyText)->SetPadding(
        FMargin(0.0f, 0.0f, 0.0f, 18.0f * Scale));

    UBorder* StatusFrame = WidgetTree->ConstructWidget<UBorder>();
    StatusFrame->SetBrushColor(
        View.bHighContrast
            ? FLinearColor(0.12f, 0.12f, 0.12f, 1.0f)
            : FLinearColor(0.10f, 0.075f, 0.035f, 1.0f));
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

    for (const FEchoesShellSlider& Model : View.Sliders)
    {
        UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
        ConfigureText(Label, Model.Label, FMath::RoundToInt(16.0f * Scale), TextColor);
        Content->AddChildToVerticalBox(Label)->SetPadding(FMargin(0, 0, 0, 8 * Scale));
        UEchoesShellValueSlider* Slider = WidgetTree->ConstructWidget<UEchoesShellValueSlider>();
        Slider->Configure(this, Model);
        ValueSliders.Add(Slider);
        SliderLabels.Add(Label);
        Content->AddChildToVerticalBox(Slider)->SetPadding(FMargin(8 * Scale, 0, 8 * Scale, 20 * Scale));
    }

    UWrapBox* TransportButtons = nullptr;
    if (bTransport)
    {
        TransportButtons = WidgetTree->ConstructWidget<UWrapBox>();
        TransportButtons->SetInnerSlotPadding(FVector2D(8, 8));
        Content->AddChildToVerticalBox(TransportButtons);
    }
    for (const FEchoesShellButton& ButtonModel : View.Buttons)
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
            FMath::RoundToInt(16.0f * Scale), TextColor);
        Label->SetJustification(ETextJustify::Center);
        Label->SetVisibility(ESlateVisibility::HitTestInvisible);
        UBorder* ButtonPadding = WidgetTree->ConstructWidget<UBorder>();
        ButtonPadding->SetBrushColor(FLinearColor::Transparent);
        ButtonPadding->SetPadding(
            FMargin(16.0f * Scale, 11.0f * Scale));
        ButtonPadding->SetVisibility(ESlateVisibility::HitTestInvisible);
        ButtonPadding->SetContent(Label);
        Button->SetContent(ButtonPadding);
        if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(ButtonPadding->Slot))
            ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
        if (TransportButtons) TransportButtons->AddChildToWrapBox(Button);
        else Content->AddChildToVerticalBox(Button)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 9.0f * Scale));
        ActionButtons.Add(Button);
    }
    FocusedButtonIndex = INDEX_NONE;
    RefreshButtonPresentation();
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
