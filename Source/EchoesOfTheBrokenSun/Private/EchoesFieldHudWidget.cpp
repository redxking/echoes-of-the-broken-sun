#include "EchoesFieldHudWidget.h"

#include "EchoesInterfaceAudioSubsystem.h"
#include "EchoesPlayerController.h"

#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Border.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "InputCoreTypes.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/Anchors.h"

namespace
{
FLinearColor PanelColor(bool bHighContrast)
{
    return bHighContrast
        ? FLinearColor(0.0f, 0.0f, 0.0f, 0.98f)
        : FLinearColor(0.015f, 0.027f, 0.038f, 0.93f);
}

FLinearColor TextColor(bool bHighContrast)
{
    return bHighContrast
        ? FLinearColor::White
        : FLinearColor(0.86f, 0.90f, 0.87f, 1.0f);
}

FLinearColor AccentColor(bool bHighContrast)
{
    return bHighContrast
        ? FLinearColor(0.1f, 1.0f, 1.0f, 1.0f)
        : FLinearColor(0.08f, 0.78f, 0.92f, 1.0f);
}

FLinearColor ToneColor(EEchoesFieldHudTone Tone, bool bHighContrast)
{
    if (bHighContrast)
    {
        return Tone == EEchoesFieldHudTone::Muted
            ? FLinearColor(0.72f, 0.72f, 0.72f, 1.0f)
            : FLinearColor::White;
    }
    switch (Tone)
    {
        case EEchoesFieldHudTone::Accent:
            return FLinearColor(0.08f, 0.78f, 0.92f, 1.0f);
        case EEchoesFieldHudTone::Success:
            return FLinearColor(0.32f, 0.88f, 0.68f, 1.0f);
        case EEchoesFieldHudTone::Warning:
            return FLinearColor(0.96f, 0.68f, 0.18f, 1.0f);
        case EEchoesFieldHudTone::Danger:
            return FLinearColor(0.94f, 0.30f, 0.32f, 1.0f);
        case EEchoesFieldHudTone::Muted:
            return FLinearColor(0.52f, 0.59f, 0.62f, 1.0f);
        default:
            return TextColor(false);
    }
}

void ConfigureText(
    UTextBlock* Text,
    const FText& Value,
    int32 BaseSize,
    float Scale,
    const FLinearColor& Color)
{
    if (Text == nullptr)
    {
        return;
    }
    Text->SetText(Value);
    Text->SetColorAndOpacity(Color);
    Text->SetAutoWrapText(true);
    Text->SetVisibility(ESlateVisibility::HitTestInvisible);
    FSlateFontInfo Font = Text->GetFont();
    Font.Size = FMath::Clamp(FMath::RoundToInt(BaseSize * Scale), 10, 28);
    Text->SetFont(Font);
}

FText JoinedLine(const FEchoesFieldHudLine& Line)
{
    if (Line.Label.IsEmpty())
    {
        return Line.Value;
    }
    if (Line.Value.IsEmpty())
    {
        return Line.Label;
    }
    return FText::Format(
        NSLOCTEXT("EchoesFieldHud", "JoinedLine", "{0}  {1}"),
        Line.Label,
        Line.Value);
}

void DrawLine(
    FSlateWindowElementList& Elements,
    int32 Layer,
    const FGeometry& Geometry,
    const TArray<FVector2D>& Points,
    const FLinearColor& Color,
    float Thickness,
    bool bClosed = false)
{
    if (Points.Num() < 2)
    {
        return;
    }
    TArray<FVector2D> StablePoints = Points;
    if (bClosed)
    {
        const FVector2D FirstPoint = StablePoints[0];
        StablePoints.Add(FirstPoint);
    }
    FSlateDrawElement::MakeLines(
        Elements,
        Layer,
        Geometry.ToPaintGeometry(),
        StablePoints,
        ESlateDrawEffect::None,
        Color,
        true,
        Thickness);
}

void DrawBox(
    FSlateWindowElementList& Elements,
    int32 Layer,
    const FGeometry& Geometry,
    const FVector2D& Position,
    const FVector2D& Size,
    const FLinearColor& Color)
{
    if (Size.X <= 0.0f || Size.Y <= 0.0f)
    {
        return;
    }
    FSlateDrawElement::MakeBox(
        Elements,
        Layer,
        Geometry.ToPaintGeometry(Size, FSlateLayoutTransform(Position)),
        FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")),
        ESlateDrawEffect::None,
        Color);
}

FLinearColor TileColor(
    EEchoesFieldHudTileState State,
    bool bHighContrast)
{
    switch (State)
    {
        case EEchoesFieldHudTileState::ExploredBlocked:
            return bHighContrast ? FLinearColor(0.30f, 0.30f, 0.30f, 1.0f)
                                 : FLinearColor(0.10f, 0.12f, 0.13f, 1.0f);
        case EEchoesFieldHudTileState::ExploredOpen:
            return bHighContrast ? FLinearColor(0.16f, 0.16f, 0.16f, 1.0f)
                                 : FLinearColor(0.035f, 0.055f, 0.063f, 1.0f);
        case EEchoesFieldHudTileState::ExploredScarred:
            return bHighContrast ? FLinearColor(0.24f, 0.24f, 0.24f, 1.0f)
                                 : FLinearColor(0.13f, 0.085f, 0.07f, 1.0f);
        case EEchoesFieldHudTileState::VisibleBlocked:
            return bHighContrast ? FLinearColor(0.72f, 0.72f, 0.72f, 1.0f)
                                 : FLinearColor(0.22f, 0.28f, 0.29f, 1.0f);
        case EEchoesFieldHudTileState::VisibleOpen:
            return bHighContrast ? FLinearColor(0.42f, 0.42f, 0.42f, 1.0f)
                                 : FLinearColor(0.07f, 0.13f, 0.15f, 1.0f);
        case EEchoesFieldHudTileState::VisibleScarred:
            return bHighContrast ? FLinearColor(0.52f, 0.52f, 0.52f, 1.0f)
                                 : FLinearColor(0.32f, 0.18f, 0.11f, 1.0f);
        default:
            return FLinearColor(0.005f, 0.008f, 0.012f, 1.0f);
    }
}

bool SameControlIdentity(
    const FEchoesFieldHudControl& Left,
    const FEchoesFieldHudControl& Right)
{
    return Left.Action == Right.Action && Left.Argument == Right.Argument;
}

bool ResolvePointerInGeometry(
    const FGeometry& RootGeometry,
    const FGeometry& TargetGeometry,
    const FVector2D& ViewportPosition,
    float ViewportScale,
    FVector2D* OutLocal = nullptr)
{
    if (RootGeometry.GetLocalSize().X <= 0.0f ||
        RootGeometry.GetLocalSize().Y <= 0.0f ||
        TargetGeometry.GetLocalSize().X <= 0.0f ||
        TargetGeometry.GetLocalSize().Y <= 0.0f)
    {
        return false;
    }
    const FVector2D Absolute = RootGeometry.LocalToAbsolute(
        ViewportPosition / FMath::Max(0.01f, ViewportScale));
    if (!TargetGeometry.IsUnderLocation(Absolute))
    {
        return false;
    }
    if (OutLocal != nullptr)
    {
        *OutLocal = TargetGeometry.AbsoluteToLocal(Absolute);
    }
    return true;
}

void PlayFieldInterfaceCue(
    const UWidget* Widget,
    EEchoesInterfaceCue Cue)
{
    if (Widget == nullptr)
    {
        return;
    }
    if (UWorld* World = Widget->GetWorld())
    {
        if (UEchoesInterfaceAudioSubsystem* Audio =
                World->GetSubsystem<UEchoesInterfaceAudioSubsystem>())
        {
            Audio->PlayInterfaceCue(Cue);
        }
    }
}
} // namespace

UEchoesFieldHudActionButton::UEchoesFieldHudActionButton(
    const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    InitIsFocusable(true);
    SetClickMethod(EButtonClickMethod::PreciseClick);
    SetTouchMethod(EButtonTouchMethod::PreciseTap);
    SetPressMethod(EButtonPressMethod::ButtonPress);
}

void UEchoesFieldHudActionButton::Configure(
    UEchoesFieldHudWidget* InOwner,
    const FEchoesFieldHudControl& InControl,
    bool bInHighContrast,
    float InScale)
{
    Owner = InOwner;
    Action = InControl.Action;
    Argument = InControl.Argument;
    bFocusedPresentation = InControl.bFocused;
    bHighContrast = bInHighContrast;
    SetIsEnabled(InControl.bEnabled);
    OnReceivedFocus.BindUObject(this, &UEchoesFieldHudActionButton::HandleReceivedFocus);
    OnClicked.AddUniqueDynamic(this, &UEchoesFieldHudActionButton::HandleClicked);
    OnHovered.AddUniqueDynamic(this, &UEchoesFieldHudActionButton::HandleHovered);
    OnUnhovered.AddUniqueDynamic(this, &UEchoesFieldHudActionButton::HandleUnhovered);

    const FLinearColor Accent = AccentColor(bHighContrast);
    FButtonStyle Style = GetStyle();
    Style.Normal.TintColor = FSlateColor(
        InControl.bFocused
            ? Accent
            : FLinearColor(0.035f, 0.065f, 0.082f, 0.98f));
    Style.Hovered.TintColor = FSlateColor(Accent);
    Style.Pressed.TintColor = FSlateColor(
        FLinearColor(0.025f, 0.42f, 0.54f, 1.0f));
    Style.Disabled.TintColor = FSlateColor(
        FLinearColor(0.02f, 0.028f, 0.032f, 0.78f));
    SetStyle(Style);

    if (UTextBlock* Label = Cast<UTextBlock>(GetContent()))
    {
        FText Text = InControl.Detail.IsEmpty()
            ? InControl.Label
            : FText::Format(
                NSLOCTEXT("EchoesFieldHud", "ControlWithDetail", "{0}\n{1}"),
                InControl.Label,
                InControl.Detail);
        ConfigureText(Label, Text, 13, InScale, TextColor(bHighContrast));
    }
}

bool UEchoesFieldHudActionButton::Activate()
{
    UEchoesFieldHudWidget* Current = Owner.Get();
    if (Current == nullptr || !GetIsEnabled() || Action == EEchoesFieldHudAction::None)
    {
        return false;
    }
    PlayFieldInterfaceCue(this, EEchoesInterfaceCue::Select);
    Current->DispatchAction(Action, Argument);
    return true;
}

void UEchoesFieldHudActionButton::HandleClicked()
{
    if (UEchoesFieldHudWidget* Current = Owner.Get())
    {
        Current->NotifyButtonFocused(this);
    }
    Activate();
}

void UEchoesFieldHudActionButton::HandleHovered()
{
    bPointerHovered = true;
    PlayFieldInterfaceCue(this, EEchoesInterfaceCue::Hover);
}

void UEchoesFieldHudActionButton::HandleUnhovered()
{
    bPointerHovered = false;
}

void UEchoesFieldHudActionButton::HandleReceivedFocus()
{
    if (UEchoesFieldHudWidget* Current = Owner.Get())
    {
        Current->NotifyButtonFocused(this);
    }
    if (!bPointerHovered)
    {
        PlayFieldInterfaceCue(this, EEchoesInterfaceCue::Hover);
    }
}

void UEchoesFieldHudEndpointBox::Configure(
    UEchoesFieldHudWidget* InOwner,
    const FText& Endpoint)
{
    Owner = InOwner;
    if (!bHasAppliedAuthoritativeEndpoint ||
        (!HasKeyboardFocus() &&
         !Endpoint.EqualTo(LastAppliedAuthoritativeEndpoint)))
    {
        SetText(Endpoint);
        LastAppliedAuthoritativeEndpoint = Endpoint;
        bHasAppliedAuthoritativeEndpoint = true;
        bTextDirty = false;
    }
    SetHintText(NSLOCTEXT("EchoesFieldHud", "EndpointHint", "Host:Port"));
    OnTextChanged.AddUniqueDynamic(
        this,
        &UEchoesFieldHudEndpointBox::HandleTextChanged);
    OnTextCommitted.AddUniqueDynamic(
        this,
        &UEchoesFieldHudEndpointBox::HandleCommitted);
}

void UEchoesFieldHudEndpointBox::HandleTextChanged(const FText& NewText)
{
    (void)NewText;
    bTextDirty = true;
}

void UEchoesFieldHudEndpointBox::HandleCommitted(
    const FText& NewText,
    ETextCommit::Type CommitMethod)
{
    if (bTextDirty &&
        (CommitMethod == ETextCommit::OnEnter ||
         CommitMethod == ETextCommit::OnUserMovedFocus))
    {
        bTextDirty = false;
        if (UEchoesFieldHudWidget* Current = Owner.Get())
        {
            Current->DispatchEndpoint(NewText.ToString());
        }
    }
}

void UEchoesFieldHudSectionWidget::Configure(
    UEchoesFieldHudWidget* InOwner,
    EEchoesFieldHudSection InSection)
{
    Owner = InOwner;
    Section = InSection;
    SetVisibility(ESlateVisibility::Collapsed);
}

bool UEchoesFieldHudSectionWidget::CanRefreshInPlace(
    const TArray<FText>& InLines,
    const TArray<FEchoesFieldHudControl>& InControls,
    bool bShowEndpoint) const
{
    if (RootBorder == nullptr || LineTexts.Num() != InLines.Num() ||
        ActionButtons.Num() != InControls.Num() ||
        bHasEndpoint != bShowEndpoint)
    {
        return false;
    }
    for (int32 Index = 0; Index < InControls.Num(); ++Index)
    {
        if (!Controls.IsValidIndex(Index) ||
            !SameControlIdentity(Controls[Index], InControls[Index]))
        {
            return false;
        }
    }
    return true;
}

void UEchoesFieldHudSectionWidget::SetContent(
    const FText& InTitle,
    const TArray<FText>& InLines,
    const TArray<FEchoesFieldHudControl>& InControls,
    bool bInHighContrast,
    float InScale,
    bool bShowEndpoint,
    const FText& Endpoint)
{
    const bool bRefresh = CanRefreshInPlace(
        InLines,
        InControls,
        bShowEndpoint);
    Title = InTitle;
    Lines = InLines;
    Controls = InControls;
    bHighContrast = bInHighContrast;
    Scale = FMath::Clamp(InScale, 0.8f, 1.5f);
    bHasEndpoint = bShowEndpoint;
    EndpointText = Endpoint;
    if (!bRefresh)
    {
        RebuildContent();
        return;
    }

    const bool bCompact = Section == EEchoesFieldHudSection::Status ||
        Section == EEchoesFieldHudSection::Subtitle;
    RootBorder->SetPadding(bCompact
        ? FMargin(8.0f * Scale, 4.0f * Scale)
        : FMargin(10.0f * Scale));
    RootBorder->SetBrushColor(PanelColor(bHighContrast));
    if (TitleText != nullptr)
    {
        ConfigureText(TitleText, Title, bCompact ? 11 : 15, Scale,
            AccentColor(bHighContrast));
    }
    for (int32 Index = 0; Index < Lines.Num(); ++Index)
    {
        ConfigureText(LineTexts[Index], Lines[Index], 12, Scale, TextColor(bHighContrast));
    }
    for (int32 Index = 0; Index < Controls.Num(); ++Index)
    {
        ActionButtons[Index]->Configure(
            Owner.Get(), Controls[Index], bHighContrast, Scale);
    }
    if (EndpointBox != nullptr)
    {
        EndpointBox->Configure(Owner.Get(), EndpointText);
    }
}

void UEchoesFieldHudSectionWidget::RebuildContent()
{
    if (WidgetTree == nullptr)
    {
        return;
    }
    if (RootBorder == nullptr)
    {
        RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
        WidgetTree->RootWidget = RootBorder;
    }
    const bool bCompact = Section == EEchoesFieldHudSection::Status ||
        Section == EEchoesFieldHudSection::Subtitle;
    // Keep an in-progress direct-connect edit alive when a semantic refresh
    // changes the surrounding status lines or action shape. Rebuilding the
    // panel must not replace the editor with a fresh copy of the last
    // authoritative endpoint.
    TObjectPtr<UEchoesFieldHudEndpointBox> RetainedEndpointBox = EndpointBox;
    if (RetainedEndpointBox != nullptr)
    {
        RetainedEndpointBox->RemoveFromParent();
    }

    ContentBox = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass());
    if (bCompact)
    {
        ContentScroll = nullptr;
        RootBorder->SetContent(ContentBox);
    }
    else
    {
        ContentScroll = WidgetTree->ConstructWidget<UScrollBox>(
            UScrollBox::StaticClass());
        ContentScroll->AddChild(ContentBox);
        RootBorder->SetContent(ContentScroll);
    }
    ContentBox->ClearChildren();
    LineTexts.Reset();
    ActionButtons.Reset();
    EndpointBox = bHasEndpoint ? RetainedEndpointBox : nullptr;

    RootBorder->SetPadding(bCompact
        ? FMargin(8.0f * Scale, 4.0f * Scale)
        : FMargin(10.0f * Scale));
    RootBorder->SetBrushColor(PanelColor(bHighContrast));
    RootBorder->SetVisibility(ESlateVisibility::Visible);
    TitleText = nullptr;
    if (Section != EEchoesFieldHudSection::Status)
    {
        TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        ConfigureText(TitleText, Title, bCompact ? 11 : 15, Scale,
            AccentColor(bHighContrast));
        ContentBox->AddChildToVerticalBox(TitleText)->SetPadding(
            FMargin(0, 0, 0, bCompact ? 2.0f * Scale : 5.0f));
    }

    for (const FText& Line : Lines)
    {
        UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        ConfigureText(Text, Line, 12, Scale, TextColor(bHighContrast));
        ContentBox->AddChildToVerticalBox(Text)->SetPadding(FMargin(0, 1));
        LineTexts.Add(Text);
    }

    if (bHasEndpoint)
    {
        if (EndpointBox == nullptr)
        {
            EndpointBox = WidgetTree->ConstructWidget<UEchoesFieldHudEndpointBox>(
                UEchoesFieldHudEndpointBox::StaticClass());
        }
        EndpointBox->Configure(Owner.Get(), EndpointText);
        ContentBox->AddChildToVerticalBox(EndpointBox)->SetPadding(FMargin(0, 5));
    }

    UUniformGridPanel* Grid = nullptr;
    if (!Controls.IsEmpty())
    {
        Grid = WidgetTree->ConstructWidget<UUniformGridPanel>(
            UUniformGridPanel::StaticClass());
        Grid->SetSlotPadding(FMargin(3.0f));
        ContentBox->AddChildToVerticalBox(Grid)->SetPadding(FMargin(0, 6, 0, 0));
    }
    const int32 Columns = Section == EEchoesFieldHudSection::CommandCard ? 3 : 2;
    for (int32 Index = 0; Index < Controls.Num(); ++Index)
    {
        const FEchoesFieldHudControl& Control = Controls[Index];
        UEchoesFieldHudActionButton* Button =
            WidgetTree->ConstructWidget<UEchoesFieldHudActionButton>(
                UEchoesFieldHudActionButton::StaticClass());
        UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(
            UTextBlock::StaticClass());
        Button->SetContent(Label);
        Button->Configure(Owner.Get(), Control, bHighContrast, Scale);
        Grid->AddChildToUniformGrid(Button, Index / Columns, Index % Columns);
        ActionButtons.Add(Button);
    }
}

bool UEchoesFieldHudSectionWidget::ScrollActionIntoView(
    UEchoesFieldHudActionButton* Button)
{
    if (ContentScroll == nullptr || Button == nullptr ||
        !ActionButtons.ContainsByPredicate(
            [Button](const TObjectPtr<UEchoesFieldHudActionButton>& Candidate)
            {
                return Candidate.Get() == Button;
            }))
    {
        return false;
    }
    ContentScroll->ScrollWidgetIntoView(
        Button,
        false,
        EDescendantScrollDestination::IntoView);
    return true;
}

bool UEchoesFieldHudSectionWidget::FocusEndpointEditor()
{
    if (EndpointBox == nullptr)
    {
        return false;
    }
    EndpointBox->SetKeyboardFocus();
    if (ContentScroll != nullptr)
    {
        ContentScroll->ScrollWidgetIntoView(
            EndpointBox,
            false,
            EDescendantScrollDestination::IntoView);
    }
    return true;
}

TSharedRef<SWidget> UEchoesFieldHudSectionWidget::RebuildWidget()
{
    if (RootBorder == nullptr)
    {
        RebuildContent();
    }
    return Super::RebuildWidget();
}

FReply UEchoesFieldHudSectionWidget::NativeOnMouseButtonDown(
    const FGeometry& InGeometry,
    const FPointerEvent& InMouseEvent)
{
    (void)InGeometry;
    (void)InMouseEvent;
    return FReply::Handled();
}

void UEchoesFieldHudMinimapWidget::Configure(UEchoesFieldHudWidget* InOwner)
{
    Owner = InOwner;
}

void UEchoesFieldHudMinimapWidget::SetView(
    const FEchoesFieldHudMinimapView& InView,
    bool bInHighContrast)
{
    View = InView;
    bHighContrast = bInHighContrast;
    RefreshMissionLabels();
    InvalidateLayoutAndVolatility();
}

void UEchoesFieldHudMinimapWidget::RefreshMissionLabels()
{
    if (WidgetTree == nullptr)
    {
        return;
    }
    if (RootCanvas == nullptr)
    {
        RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(
            UCanvasPanel::StaticClass());
        RootCanvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        WidgetTree->RootWidget = RootCanvas;
    }
    while (MissionLabels.Num() > View.MissionMarkers.Num())
    {
        if (UTextBlock* Label = MissionLabels.Pop())
        {
            Label->RemoveFromParent();
        }
    }
    while (MissionLabels.Num() < View.MissionMarkers.Num())
    {
        UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(
            UTextBlock::StaticClass());
        ConfigureText(Label, FText::GetEmpty(), 9, 1.0f,
            AccentColor(bHighContrast));
        RootCanvas->AddChildToCanvas(Label);
        MissionLabels.Add(Label);
    }
    for (int32 Index = 0; Index < View.MissionMarkers.Num(); ++Index)
    {
        const FEchoesFieldHudMissionMarker& Marker = View.MissionMarkers[Index];
        UTextBlock* Label = MissionLabels[Index];
        ConfigureText(Label, Marker.Label, 9, 1.0f,
            ToneColor(Marker.Tone, bHighContrast));
        if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Label->Slot))
        {
            Slot->SetAnchors(FAnchors(
                FMath::Clamp(Marker.NormalizedMapPosition.X, 0.0f, 1.0f),
                FMath::Clamp(Marker.NormalizedMapPosition.Y, 0.0f, 1.0f)));
            Slot->SetAlignment(FVector2D(0.5f, 0.5f));
            Slot->SetSize(FVector2D(42.0f, 18.0f));
            Slot->SetPosition(FVector2D::ZeroVector);
        }
    }
}

TSharedRef<SWidget> UEchoesFieldHudMinimapWidget::RebuildWidget()
{
    if (RootCanvas == nullptr)
    {
        RefreshMissionLabels();
    }
    return Super::RebuildWidget();
}

int32 UEchoesFieldHudMinimapWidget::NativePaint(
    const FPaintArgs& Args,
    const FGeometry& AllottedGeometry,
    const FSlateRect& MyCullingRect,
    FSlateWindowElementList& OutDrawElements,
    int32 LayerId,
    const FWidgetStyle& InWidgetStyle,
    bool bParentEnabled) const
{
    const int32 BaseLayer = Super::NativePaint(
        Args, AllottedGeometry, MyCullingRect, OutDrawElements,
        LayerId, InWidgetStyle, bParentEnabled);
    const FVector2D Size = AllottedGeometry.GetLocalSize();
    DrawBox(OutDrawElements, BaseLayer + 1, AllottedGeometry,
        FVector2D::ZeroVector, Size, PanelColor(bHighContrast));
    if (View.Width <= 0 || View.Height <= 0 ||
        View.Tiles.Num() != View.Width * View.Height)
    {
        return BaseLayer + 1;
    }
    const FVector2D Cell(
        Size.X / static_cast<float>(View.Width),
        Size.Y / static_cast<float>(View.Height));
    for (int32 Y = 0; Y < View.Height; ++Y)
    {
        int32 RunStart = 0;
        while (RunStart < View.Width)
        {
            const EEchoesFieldHudTileState State =
                View.Tiles[Y * View.Width + RunStart];
            int32 RunEnd = RunStart + 1;
            while (RunEnd < View.Width &&
                View.Tiles[Y * View.Width + RunEnd] == State)
            {
                ++RunEnd;
            }
            DrawBox(OutDrawElements, BaseLayer + 2, AllottedGeometry,
                FVector2D(Cell.X * RunStart, Cell.Y * Y),
                FVector2D(Cell.X * (RunEnd - RunStart) + 0.5f, Cell.Y + 0.5f),
                TileColor(State, bHighContrast));
            RunStart = RunEnd;
        }
    }

    const auto MapPoint = [Size](const FVector2D& Normalized)
    {
        return FVector2D(
            FMath::Clamp(Normalized.X, 0.0f, 1.0f) * Size.X,
            FMath::Clamp(Normalized.Y, 0.0f, 1.0f) * Size.Y);
    };
    for (const FEchoesFieldHudMissionMarker& Marker : View.MissionMarkers)
    {
        const FVector2D Point = MapPoint(Marker.NormalizedMapPosition);
        DrawLine(OutDrawElements, BaseLayer + 3, AllottedGeometry,
            {Point + FVector2D(0, -5), Point + FVector2D(5, 0),
             Point + FVector2D(0, 5), Point + FVector2D(-5, 0)},
            ToneColor(Marker.Tone, bHighContrast), 1.7f, true);
    }
    for (const FEchoesFieldHudMapMarker& Marker : View.Markers)
    {
        const FVector2D Point = MapPoint(Marker.NormalizedPosition);
        const float Radius = Marker.bFutureWell ? 4.5f : Marker.bResource ? 3.5f : 2.5f;
        const FLinearColor Color = Marker.bFriendly
            ? AccentColor(bHighContrast)
            : Marker.bRemembered
                ? FLinearColor(0.55f, 0.55f, 0.55f, 0.78f)
                : FLinearColor(0.95f, 0.52f, 0.18f, 1.0f);
        if (Marker.bFutureWell)
        {
            DrawLine(OutDrawElements, BaseLayer + 4, AllottedGeometry,
                {Point + FVector2D(0, -Radius),
                 Point + FVector2D(Radius * 0.72f, -Radius * 0.72f),
                 Point + FVector2D(Radius, 0),
                 Point + FVector2D(Radius * 0.72f, Radius * 0.72f),
                 Point + FVector2D(0, Radius),
                 Point + FVector2D(-Radius * 0.72f, Radius * 0.72f),
                 Point + FVector2D(-Radius, 0),
                 Point + FVector2D(-Radius * 0.72f, -Radius * 0.72f)},
                Marker.bTelegraphed
                    ? FLinearColor(0.96f, 0.68f, 0.18f, 1.0f)
                    : Color,
                Marker.bTelegraphed ? 2.5f : 1.8f,
                true);
        }
        else if (Marker.bResource)
        {
            DrawLine(OutDrawElements, BaseLayer + 4, AllottedGeometry,
                {Point + FVector2D(-Radius, 0), Point + FVector2D(Radius, 0)},
                Color, 1.5f);
            DrawLine(OutDrawElements, BaseLayer + 4, AllottedGeometry,
                {Point + FVector2D(0, -Radius), Point + FVector2D(0, Radius)},
                Color, 1.5f);
        }
        else if (Marker.bFriendly)
        {
            DrawLine(OutDrawElements, BaseLayer + 4, AllottedGeometry,
                {Point + FVector2D(-Radius, -Radius), Point + FVector2D(Radius, -Radius),
                 Point + FVector2D(Radius, Radius), Point + FVector2D(-Radius, Radius)},
                Color, 1.5f, true);
        }
        else
        {
            DrawLine(OutDrawElements, BaseLayer + 4, AllottedGeometry,
                {Point + FVector2D(0, -Radius), Point + FVector2D(Radius, 0),
                 Point + FVector2D(0, Radius), Point + FVector2D(-Radius, 0)},
                Color, 1.5f, true);
        }
    }
    for (const FEchoesFieldHudContact& Contact : View.Contacts)
    {
        const FVector2D Point = MapPoint(Contact.NormalizedMapPosition);
        DrawLine(OutDrawElements, BaseLayer + 5, AllottedGeometry,
            {Point + FVector2D(0, -5), Point + FVector2D(5, 0),
             Point + FVector2D(0, 5), Point + FVector2D(-5, 0)},
            FLinearColor(0.96f, 0.68f, 0.18f, 1.0f), 2.0f, true);
    }
    if (View.CameraFrustum.Num() >= 3)
    {
        TArray<FVector2D> Frustum;
        Frustum.Reserve(View.CameraFrustum.Num());
        for (const FVector2D& Point : View.CameraFrustum)
        {
            Frustum.Add(MapPoint(Point));
        }
        DrawLine(OutDrawElements, BaseLayer + 6, AllottedGeometry,
            Frustum, FLinearColor::White, 1.5f, true);
    }
    return BaseLayer + 6;
}

bool UEchoesFieldHudMinimapWidget::DispatchPointer(
    const FGeometry& Geometry,
    const FPointerEvent& Event,
    bool bIssueOrder) const
{
    UEchoesFieldHudWidget* Current = Owner.Get();
    const FVector2D Size = Geometry.GetLocalSize();
    if (Current == nullptr || Size.X <= 0.0f || Size.Y <= 0.0f)
    {
        return false;
    }
    const FVector2D Local = Geometry.AbsoluteToLocal(
        Event.GetScreenSpacePosition());
    const FVector2D Normalized(
        FMath::Clamp(Local.X / Size.X, 0.0f, 1.0f),
        FMath::Clamp(Local.Y / Size.Y, 0.0f, 1.0f));
    return Current->DispatchMinimapPointer(Normalized, bIssueOrder);
}

FReply UEchoesFieldHudMinimapWidget::NativeOnMouseButtonDown(
    const FGeometry& InGeometry,
    const FPointerEvent& InMouseEvent)
{
    const bool bRight = InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton;
    if (bRight)
    {
        DispatchPointer(InGeometry, InMouseEvent, true);
        return FReply::Handled();
    }
    if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
    {
        return FReply::Handled();
    }
    DispatchPointer(InGeometry, InMouseEvent, false);
    bDragging = true;
    FReply Reply = FReply::Handled();
    if (const TSharedPtr<SWidget> Cached = GetCachedWidget())
    {
        Reply.CaptureMouse(Cached.ToSharedRef());
    }
    return Reply;
}

FReply UEchoesFieldHudMinimapWidget::NativeOnMouseMove(
    const FGeometry& InGeometry,
    const FPointerEvent& InMouseEvent)
{
    if (!bDragging ||
        !InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
    {
        bDragging = false;
        return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
    }
    DispatchPointer(InGeometry, InMouseEvent, false);
    return FReply::Handled();
}

FReply UEchoesFieldHudMinimapWidget::NativeOnMouseButtonUp(
    const FGeometry& InGeometry,
    const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton ||
        !bDragging)
    {
        return FReply::Handled();
    }
    DispatchPointer(InGeometry, InMouseEvent, false);
    bDragging = false;
    return FReply::Handled().ReleaseMouseCapture();
}

void UEchoesFieldHudMinimapWidget::NativeOnMouseCaptureLost(
    const FCaptureLostEvent& CaptureLostEvent)
{
    bDragging = false;
    Super::NativeOnMouseCaptureLost(CaptureLostEvent);
}

void UEchoesFieldHudCampaignMapWidget::Configure(
    UEchoesFieldHudWidget* InOwner)
{
    Owner = InOwner;
}

void UEchoesFieldHudCampaignMapWidget::SetView(
    const FEchoesFieldHudCampaignView& InView,
    bool bInHighContrast,
    float InScale)
{
    View = InView;
    bHighContrast = bInHighContrast;
    Scale = FMath::Clamp(InScale, 0.8f, 1.5f);
    RebuildNodes();
    InvalidateLayoutAndVolatility();
}

void UEchoesFieldHudCampaignMapWidget::RebuildNodes()
{
    if (WidgetTree == nullptr)
    {
        return;
    }
    if (RootCanvas == nullptr)
    {
        RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(
            UCanvasPanel::StaticClass());
        RootCanvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        WidgetTree->RootWidget = RootCanvas;
    }
    TArray<FEchoesFieldHudControl> NextControls =
        View.Controls.FilterByPredicate(
            [](const FEchoesFieldHudControl& Control)
            {
                return Control.Action == EEchoesFieldHudAction::CampaignSelectNode;
            });
    bool bStable = NodeButtons.Num() == NextControls.Num() &&
        NodeControls.Num() == NextControls.Num();
    for (int32 Index = 0; bStable && Index < NextControls.Num(); ++Index)
    {
        bStable = SameControlIdentity(NodeControls[Index], NextControls[Index]);
    }
    if (!bStable)
    {
        for (UEchoesFieldHudActionButton* Button : NodeButtons)
        {
            if (Button != nullptr)
            {
                Button->RemoveFromParent();
            }
        }
        NodeButtons.Reset();
    }
    NodeControls = NextControls;
    const FVector2D AuthoredMapSize = View.Layout.MapCanvas.GetSize();
    const bool bHasAuthoredMapCanvas =
        AuthoredMapSize.X > 0.0f && AuthoredMapSize.Y > 0.0f;
    const FVector2D MapOrigin = bHasAuthoredMapCanvas
        ? View.Layout.MapCanvas.Min
        : FVector2D::ZeroVector;
    const FVector2D MapSize = bHasAuthoredMapCanvas
        ? AuthoredMapSize
        : View.Layout.ViewportSize;
    if (MapSize.X <= 0.0f || MapSize.Y <= 0.0f)
    {
        return;
    }
    for (int32 ControlIndex = 0;
         ControlIndex < NodeControls.Num();
         ++ControlIndex)
    {
        const FEchoesFieldHudControl& Control = NodeControls[ControlIndex];
        const FEchoesCampaignMapNode* Node = View.Layout.Nodes.FindByPredicate(
            [&Control](const FEchoesCampaignMapNode& Candidate)
            {
                return Candidate.Index == Control.Argument;
            });
        if (Node == nullptr)
        {
            continue;
        }
        UEchoesFieldHudActionButton* Button = bStable
            ? NodeButtons[ControlIndex].Get()
            : WidgetTree->ConstructWidget<UEchoesFieldHudActionButton>(
                UEchoesFieldHudActionButton::StaticClass());
        if (!bStable)
        {
            UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(
                UTextBlock::StaticClass());
            Button->SetContent(Label);
        }
        Button->Configure(Owner.Get(), Control, bHighContrast, Scale);
        UCanvasPanelSlot* Slot = bStable
            ? Cast<UCanvasPanelSlot>(Button->Slot)
            : RootCanvas->AddChildToCanvas(Button);
        if (Slot == nullptr)
        {
            continue;
        }
        const FVector2D Normalized =
            (Node->ScreenPos - MapOrigin) / MapSize;
        Slot->SetAnchors(FAnchors(
            FMath::Clamp(Normalized.X, 0.0f, 1.0f),
            FMath::Clamp(Normalized.Y, 0.0f, 1.0f)));
        Slot->SetAlignment(FVector2D(0.5f, 0.5f));
        Slot->SetSize(FVector2D(76.0f, 42.0f));
        Slot->SetPosition(FVector2D::ZeroVector);
        if (!bStable)
        {
            NodeButtons.Add(Button);
        }
    }
}

TSharedRef<SWidget> UEchoesFieldHudCampaignMapWidget::RebuildWidget()
{
    if (RootCanvas == nullptr)
    {
        RebuildNodes();
    }
    return Super::RebuildWidget();
}

int32 UEchoesFieldHudCampaignMapWidget::NativePaint(
    const FPaintArgs& Args,
    const FGeometry& AllottedGeometry,
    const FSlateRect& MyCullingRect,
    FSlateWindowElementList& OutDrawElements,
    int32 LayerId,
    const FWidgetStyle& InWidgetStyle,
    bool bParentEnabled) const
{
    const int32 BaseLayer = Super::NativePaint(
        Args, AllottedGeometry, MyCullingRect, OutDrawElements,
        LayerId, InWidgetStyle, bParentEnabled);
    const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
    DrawBox(OutDrawElements, BaseLayer + 1, AllottedGeometry,
        FVector2D::ZeroVector, LocalSize, PanelColor(bHighContrast));
    const FVector2D MapSize = View.Layout.MapCanvas.GetSize();
    const auto LocalPoint = [this, LocalSize, MapSize](const FVector2D& Point)
    {
        if (MapSize.X <= 0.0f || MapSize.Y <= 0.0f)
        {
            return FVector2D::ZeroVector;
        }
        const FVector2D Normalized =
            (Point - View.Layout.MapCanvas.Min) / MapSize;
        return FVector2D(
            Normalized.X * LocalSize.X,
            Normalized.Y * LocalSize.Y);
    };
    for (const FEchoesCampaignMapCorridor& Corridor : View.Layout.Corridors)
    {
        DrawLine(OutDrawElements, BaseLayer + 2, AllottedGeometry,
            {LocalPoint(Corridor.StartPos), LocalPoint(Corridor.EndPos)},
            Corridor.bActive ? AccentColor(bHighContrast)
                             : FLinearColor(0.20f, 0.24f, 0.26f, 0.72f),
            Corridor.bActive ? 2.5f : 1.0f);
    }
    for (const FEchoesCampaignMapNode& Node : View.Layout.Nodes)
    {
        const FVector2D Point = LocalPoint(Node.ScreenPos);
        const float Radius = FMath::Max(
            5.0f,
            Node.Radius * FMath::Min(
                LocalSize.X / FMath::Max(1.0f, MapSize.X),
                LocalSize.Y / FMath::Max(1.0f, MapSize.Y)));
        FLinearColor Color = FLinearColor(0.30f, 0.34f, 0.36f, 1.0f);
        if (Node.State == EEchoesCampaignNodeState::Available)
        {
            Color = AccentColor(bHighContrast);
        }
        else if (Node.State == EEchoesCampaignNodeState::Completed)
        {
            Color = FLinearColor(0.96f, 0.68f, 0.18f, 1.0f);
        }
        DrawLine(OutDrawElements, BaseLayer + 3, AllottedGeometry,
            {Point + FVector2D(0, -Radius), Point + FVector2D(Radius, 0),
             Point + FVector2D(0, Radius), Point + FVector2D(-Radius, 0)},
            Color, Node.Index == View.Layout.ActiveMissionIndex ? 3.0f : 1.8f, true);
    }
    return BaseLayer + 3;
}

FReply UEchoesFieldHudCampaignMapWidget::NativeOnMouseButtonDown(
    const FGeometry& InGeometry,
    const FPointerEvent& InMouseEvent)
{
    UEchoesFieldHudWidget* Current = Owner.Get();
    const FVector2D Local = InGeometry.AbsoluteToLocal(
        InMouseEvent.GetScreenSpacePosition());
    const FVector2D LocalSize = InGeometry.GetLocalSize();
    const FVector2D MapSize = View.Layout.MapCanvas.GetSize();
    if (Current != nullptr && LocalSize.X > 0.0f && LocalSize.Y > 0.0f &&
        MapSize.X > 0.0f && MapSize.Y > 0.0f)
    {
        const FVector2D LayoutPoint(
            View.Layout.MapCanvas.Min.X + Local.X * MapSize.X / LocalSize.X,
            View.Layout.MapCanvas.Min.Y + Local.Y * MapSize.Y / LocalSize.Y);
        const int32 NodeIndex = View.Layout.HitTestNode(LayoutPoint);
        if (NodeIndex >= 0)
        {
            Current->DispatchAction(
                EEchoesFieldHudAction::CampaignSelectNode,
                NodeIndex);
        }
    }
    return FReply::Handled();
}

void UEchoesFieldHudTargetingWidget::SetView(
    const FEchoesFieldHudTargetingView& InView,
    bool bInHighContrast)
{
    View = InView;
    bHighContrast = bInHighContrast;
    SetVisibility(
        View.bKeyboardTargetVisible || View.bSelectionDragVisible
            ? ESlateVisibility::HitTestInvisible
            : ESlateVisibility::Collapsed);
    InvalidateLayoutAndVolatility();
}

int32 UEchoesFieldHudTargetingWidget::NativePaint(
    const FPaintArgs& Args,
    const FGeometry& AllottedGeometry,
    const FSlateRect& MyCullingRect,
    FSlateWindowElementList& OutDrawElements,
    int32 LayerId,
    const FWidgetStyle& InWidgetStyle,
    bool bParentEnabled) const
{
    const int32 BaseLayer = Super::NativePaint(
        Args, AllottedGeometry, MyCullingRect, OutDrawElements,
        LayerId, InWidgetStyle, bParentEnabled);
    const FLinearColor Color = AccentColor(bHighContrast);
    if (View.bSelectionDragVisible)
    {
        const FVector2D Size = AllottedGeometry.GetLocalSize();
        const FVector2D SelectionStart =
            View.SelectionStartNormalized * Size;
        const FVector2D SelectionEnd =
            View.SelectionEndNormalized * Size;
        const FVector2D Min(
            FMath::Min(SelectionStart.X, SelectionEnd.X),
            FMath::Min(SelectionStart.Y, SelectionEnd.Y));
        const FVector2D Max(
            FMath::Max(SelectionStart.X, SelectionEnd.X),
            FMath::Max(SelectionStart.Y, SelectionEnd.Y));
        DrawLine(OutDrawElements, BaseLayer + 1, AllottedGeometry,
            {Min, FVector2D(Max.X, Min.Y), Max, FVector2D(Min.X, Max.Y)},
            Color, 1.5f, true);
    }
    if (View.bKeyboardTargetVisible)
    {
        const FVector2D Center =
            AllottedGeometry.GetLocalSize() * 0.5f +
            View.KeyboardTargetNormalizedOffset * AllottedGeometry.GetLocalSize();
        DrawLine(OutDrawElements, BaseLayer + 2, AllottedGeometry,
            {Center + FVector2D(0, -11), Center + FVector2D(11, 0),
             Center + FVector2D(0, 11), Center + FVector2D(-11, 0)},
            Color, 2.0f, true);
    }
    return BaseLayer + 2;
}

void UEchoesFieldHudContactWidget::SetContact(
    const FEchoesFieldHudContact& InContact,
    bool bInHighContrast,
    float InScale)
{
    Contact = InContact;
    bHighContrast = bInHighContrast;
    Scale = FMath::Clamp(InScale, 0.8f, 1.5f);
    if (RootBorder == nullptr)
    {
        RebuildContent();
        return;
    }
    RootBorder->SetBrushColor(PanelColor(bHighContrast));
    const FText Label = Contact.SecondaryLabel.IsEmpty()
        ? Contact.PrimaryLabel
        : FText::Format(
            NSLOCTEXT("EchoesFieldHud", "ContactLabel", "{0}\n{1}"),
            Contact.PrimaryLabel,
            Contact.SecondaryLabel);
    ConfigureText(ContactText, Label, 11, Scale,
        Contact.bClampedToScreenEdge
            ? FLinearColor(0.96f, 0.68f, 0.18f, 1.0f)
            : AccentColor(bHighContrast));
}

void UEchoesFieldHudContactWidget::RebuildContent()
{
    if (WidgetTree == nullptr)
    {
        return;
    }
    RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    RootBorder->SetPadding(FMargin(6.0f));
    RootBorder->SetBrushColor(PanelColor(bHighContrast));
    RootBorder->SetVisibility(ESlateVisibility::HitTestInvisible);
    ContactText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    RootBorder->SetContent(ContactText);
    WidgetTree->RootWidget = RootBorder;
    SetContact(Contact, bHighContrast, Scale);
}

TSharedRef<SWidget> UEchoesFieldHudContactWidget::RebuildWidget()
{
    if (RootBorder == nullptr)
    {
        RebuildContent();
    }
    return Super::RebuildWidget();
}

UEchoesFieldHudWidget::UEchoesFieldHudWidget(
    const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetIsFocusable(true);
    SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UEchoesFieldHudWidget::Configure(AEchoesPlayerController* InController)
{
    Controller = InController;
}

AEchoesPlayerController* UEchoesFieldHudWidget::ResolveController() const
{
    if (AEchoesPlayerController* Current = Controller.Get())
    {
        return Current;
    }
    return Cast<AEchoesPlayerController>(GetOwningPlayer());
}

void UEchoesFieldHudWidget::DispatchAction(
    EEchoesFieldHudAction Action,
    int32 Argument)
{
    if (AEchoesPlayerController* Current = ResolveController())
    {
        Current->HandleFieldHudAction(Action, Argument);
    }
}

bool UEchoesFieldHudWidget::DispatchMinimapPointer(
    const FVector2D& NormalizedMapPosition,
    bool bIssueOrder)
{
    if (AEchoesPlayerController* Current = ResolveController())
    {
        return Current->HandleFieldHudPointer(
            NormalizedMapPosition,
            bIssueOrder);
    }
    return false;
}

bool UEchoesFieldHudWidget::IsPointerOverMinimap(
    const FVector2D& ScreenPosition) const
{
    if (MinimapWidget == nullptr ||
        MinimapWidget->GetVisibility() == ESlateVisibility::Collapsed)
    {
        return false;
    }
    return ResolvePointerInGeometry(
        GetCachedGeometry(),
        MinimapWidget->GetCachedGeometry(),
        ScreenPosition,
        UWidgetLayoutLibrary::GetViewportScale(this));
}

bool UEchoesFieldHudWidget::IsPointerOverChrome(
    const FVector2D& ScreenPosition) const
{
    const FGeometry RootGeometry = GetCachedGeometry();
    const float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(this);
    for (const UEchoesFieldHudSectionWidget* Section : Sections)
    {
        if (Section != nullptr &&
            Section->GetVisibility() != ESlateVisibility::Collapsed &&
            ResolvePointerInGeometry(
                RootGeometry,
                Section->GetCachedGeometry(),
                ScreenPosition,
                ViewportScale))
        {
            return true;
        }
    }
    if (IsPointerOverMinimap(ScreenPosition))
    {
        return true;
    }
    return CampaignMapWidget != nullptr &&
        CampaignMapWidget->GetVisibility() != ESlateVisibility::Collapsed &&
        ResolvePointerInGeometry(
            RootGeometry,
            CampaignMapWidget->GetCachedGeometry(),
            ScreenPosition,
            ViewportScale);
}

bool UEchoesFieldHudWidget::HandlePointerAtViewportPosition(
    const FVector2D& ScreenPosition,
    bool bIssueOrder)
{
    if (MinimapWidget == nullptr ||
        MinimapWidget->GetVisibility() == ESlateVisibility::Collapsed)
    {
        return false;
    }
    FVector2D Local;
    const FGeometry MinimapGeometry = MinimapWidget->GetCachedGeometry();
    if (!ResolvePointerInGeometry(
            GetCachedGeometry(),
            MinimapGeometry,
            ScreenPosition,
            UWidgetLayoutLibrary::GetViewportScale(this),
            &Local))
    {
        return false;
    }
    const FVector2D Size = MinimapGeometry.GetLocalSize();
    if (Size.X <= 0.0f || Size.Y <= 0.0f)
    {
        return false;
    }
    return DispatchMinimapPointer(
        FVector2D(
            FMath::Clamp(Local.X / Size.X, 0.0f, 1.0f),
            FMath::Clamp(Local.Y / Size.Y, 0.0f, 1.0f)),
        bIssueOrder);
}

void UEchoesFieldHudWidget::DispatchEndpoint(const FString& Endpoint)
{
    if (AEchoesPlayerController* Current = ResolveController())
    {
        Current->HandleFieldHudEndpoint(Endpoint);
    }
}

void UEchoesFieldHudWidget::AddSection(
    EEchoesFieldHudSection Section,
    const FAnchors& Anchors,
    const FMargin& Offsets)
{
    UEchoesFieldHudSectionWidget* Panel =
        WidgetTree->ConstructWidget<UEchoesFieldHudSectionWidget>(
            UEchoesFieldHudSectionWidget::StaticClass());
    Panel->Configure(this, Section);
    UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(Panel);
    Slot->SetAnchors(Anchors);
    Slot->SetOffsets(Offsets);
    Sections.Add(Panel);
}

void UEchoesFieldHudWidget::BuildStableTree()
{
    if (WidgetTree == nullptr)
    {
        return;
    }
    Sections.Reset();
    ContactWidgets.Reset();
    RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(
        UCanvasPanel::StaticClass());
    RootCanvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    WidgetTree->RootWidget = RootCanvas;

    AddSection(EEchoesFieldHudSection::ResourceLedger,
        FAnchors(0.58f, 0.02f, 0.98f, 0.22f), FMargin(0));
    AddSection(EEchoesFieldHudSection::Objectives,
        FAnchors(0.02f, 0.02f, 0.36f, 0.28f), FMargin(0));
    AddSection(EEchoesFieldHudSection::Status,
        FAnchors(0.02f, 0.61f, 0.54f, 0.71f), FMargin(0));
    AddSection(EEchoesFieldHudSection::Subtitle,
        FAnchors(0.22f, 0.47f, 0.78f, 0.59f), FMargin(0));
    AddSection(EEchoesFieldHudSection::Selection,
        FAnchors(0.21f, 0.72f, 0.62f, 0.98f), FMargin(0));
    AddSection(EEchoesFieldHudSection::CommandCard,
        FAnchors(0.63f, 0.72f, 0.98f, 0.98f), FMargin(0));
    AddSection(EEchoesFieldHudSection::Technology,
        FAnchors(0.20f, 0.12f, 0.80f, 0.70f), FMargin(0));
    AddSection(EEchoesFieldHudSection::CampaignInspector,
        FAnchors(0.68f, 0.16f, 0.97f, 0.88f), FMargin(0));
    AddSection(EEchoesFieldHudSection::OnlineFrontDoor,
        FAnchors(0.25f, 0.15f, 0.75f, 0.82f), FMargin(0));
    AddSection(EEchoesFieldHudSection::OnlineLocalMenu,
        FAnchors(0.31f, 0.24f, 0.69f, 0.76f), FMargin(0));
    AddSection(EEchoesFieldHudSection::Reconnect,
        FAnchors(0.24f, 0.03f, 0.76f, 0.16f), FMargin(0));

    MinimapWidget = WidgetTree->ConstructWidget<UEchoesFieldHudMinimapWidget>(
        UEchoesFieldHudMinimapWidget::StaticClass());
    MinimapWidget->Configure(this);
    MinimapWidget->SetVisibility(ESlateVisibility::Collapsed);
    UCanvasPanelSlot* MinimapSlot = RootCanvas->AddChildToCanvas(MinimapWidget);
    MinimapSlot->SetAnchors(FAnchors(0.02f, 0.72f, 0.20f, 0.98f));
    MinimapSlot->SetOffsets(FMargin(0));

    CampaignMapWidget =
        WidgetTree->ConstructWidget<UEchoesFieldHudCampaignMapWidget>(
            UEchoesFieldHudCampaignMapWidget::StaticClass());
    CampaignMapWidget->Configure(this);
    CampaignMapWidget->SetVisibility(ESlateVisibility::Collapsed);
    UCanvasPanelSlot* CampaignSlot = RootCanvas->AddChildToCanvas(CampaignMapWidget);
    CampaignSlot->SetAnchors(FAnchors(0.02f, 0.08f, 0.67f, 0.92f));
    CampaignSlot->SetOffsets(FMargin(0));

    TargetingWidget = WidgetTree->ConstructWidget<UEchoesFieldHudTargetingWidget>(
        UEchoesFieldHudTargetingWidget::StaticClass());
    TargetingWidget->SetVisibility(ESlateVisibility::Collapsed);
    UCanvasPanelSlot* TargetingSlot = RootCanvas->AddChildToCanvas(TargetingWidget);
    TargetingSlot->SetAnchors(FAnchors(0, 0, 1, 1));
    TargetingSlot->SetOffsets(FMargin(0));
}

void UEchoesFieldHudWidget::RefreshContactWidgets()
{
    const bool bVisibleSurface =
        View.Surface == EEchoesFieldHudSurface::Battlefield ||
        View.Surface == EEchoesFieldHudSurface::Replay;
    const int32 Required = bVisibleSurface
        ? View.Minimap.Contacts.FilterByPredicate(
            [](const FEchoesFieldHudContact& Contact)
            {
                return Contact.bScreenPlacementValid;
            }).Num()
        : 0;
    while (ContactWidgets.Num() > Required)
    {
        if (UEchoesFieldHudContactWidget* Contact = ContactWidgets.Pop())
        {
            Contact->RemoveFromParent();
        }
    }
    while (ContactWidgets.Num() < Required)
    {
        UEchoesFieldHudContactWidget* Contact =
            WidgetTree->ConstructWidget<UEchoesFieldHudContactWidget>(
                UEchoesFieldHudContactWidget::StaticClass());
        Contact->SetVisibility(ESlateVisibility::HitTestInvisible);
        RootCanvas->AddChildToCanvas(Contact);
        ContactWidgets.Add(Contact);
    }
    int32 WidgetIndex = 0;
    for (const FEchoesFieldHudContact& Contact : View.Minimap.Contacts)
    {
        if (!Contact.bScreenPlacementValid)
        {
            continue;
        }
        UEchoesFieldHudContactWidget* ContactWidget = ContactWidgets[WidgetIndex++];
        ContactWidget->SetContact(Contact, View.bHighContrast, View.HudScale);
        if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(ContactWidget->Slot))
        {
            Slot->SetAnchors(FAnchors(
                Contact.NormalizedScreenPosition.X,
                Contact.NormalizedScreenPosition.Y));
            Slot->SetAlignment(FVector2D(0.5f, 0.5f));
            Slot->SetSize(FVector2D(230.0f, 52.0f) *
                FMath::Clamp(View.HudScale, 0.8f, 1.5f));
            Slot->SetPosition(FVector2D::ZeroVector);
        }
    }
}

TSharedRef<SWidget> UEchoesFieldHudWidget::RebuildWidget()
{
    if (RootCanvas == nullptr)
    {
        BuildStableTree();
        if (bHasView)
        {
            ApplyView();
        }
    }
    return Super::RebuildWidget();
}

void UEchoesFieldHudWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (RootCanvas == nullptr)
    {
        BuildStableTree();
    }
    if (bHasView)
    {
        ApplyView();
    }
}

UEchoesFieldHudSectionWidget* UEchoesFieldHudWidget::GetSection(
    EEchoesFieldHudSection Section) const
{
    for (UEchoesFieldHudSectionWidget* Candidate : Sections)
    {
        if (Candidate != nullptr && Candidate->GetSection() == Section)
        {
            return Candidate;
        }
    }
    return nullptr;
}

void UEchoesFieldHudWidget::SetView(const FEchoesFieldHudView& InView)
{
    TArray<UEchoesFieldHudActionButton*> PriorButtons;
    GatherActionButtons(PriorButtons);
    const bool bButtonHadKeyboardFocus = PriorButtons.IsValidIndex(
            FocusedButtonIndex) &&
        PriorButtons[FocusedButtonIndex]->HasKeyboardFocus();
    UEchoesFieldHudSectionWidget* PriorOnline = GetSection(
        EEchoesFieldHudSection::OnlineFrontDoor);
    const bool bEndpointHadKeyboardFocus = PriorOnline != nullptr &&
        PriorOnline->IsEndpointEditing();
    if (PriorButtons.IsValidIndex(FocusedButtonIndex))
    {
        FocusedAction = PriorButtons[FocusedButtonIndex]->GetAction();
        FocusedArgument = PriorButtons[FocusedButtonIndex]->GetArgument();
    }

    View = InView;
    bHasView = true;
    if (RootCanvas == nullptr)
    {
        BuildStableTree();
    }
    ApplyView();

    TArray<UEchoesFieldHudActionButton*> Buttons;
    GatherActionButtons(Buttons);
    FocusedButtonIndex = FindButtonIndex(
        Buttons,
        FocusedAction,
        FocusedArgument);
    if (!Buttons.IsValidIndex(FocusedButtonIndex) ||
        !Buttons[FocusedButtonIndex]->GetIsEnabled())
    {
        FocusedButtonIndex = FindDefaultButtonIndex(Buttons);
        if (Buttons.IsValidIndex(FocusedButtonIndex))
        {
            FocusedAction = Buttons[FocusedButtonIndex]->GetAction();
            FocusedArgument = Buttons[FocusedButtonIndex]->GetArgument();
        }
        else
        {
            FocusedAction = EEchoesFieldHudAction::None;
            FocusedArgument = 0;
        }
    }
    if (bEndpointHadKeyboardFocus)
    {
        if (UEchoesFieldHudSectionWidget* Online = GetSection(
                EEchoesFieldHudSection::OnlineFrontDoor))
        {
            if (Online->FocusEndpointEditor())
            {
                FocusedButtonIndex = FindButtonIndex(
                    Buttons,
                    EEchoesFieldHudAction::OnlineEditEndpoint,
                    0);
                FocusedAction = EEchoesFieldHudAction::OnlineEditEndpoint;
                FocusedArgument = 0;
                return;
            }
        }
    }
    if ((bButtonHadKeyboardFocus || bEndpointHadKeyboardFocus) &&
        Buttons.IsValidIndex(FocusedButtonIndex))
    {
        FocusButtonAtIndex(FocusedButtonIndex, Buttons);
    }
}

void UEchoesFieldHudWidget::ApplyView()
{
    if (RootCanvas == nullptr)
    {
        return;
    }
    const float Scale = FMath::Clamp(View.HudScale, 0.8f, 1.5f);
    const bool bBattlefield =
        View.Surface == EEchoesFieldHudSurface::Battlefield ||
        View.Surface == EEchoesFieldHudSurface::Replay;

    TArray<FText> Lines;
    UEchoesFieldHudSectionWidget* Panel = GetSection(
        EEchoesFieldHudSection::ResourceLedger);
    Lines = {
        FText::Format(NSLOCTEXT("EchoesFieldHud", "ResourceSummary",
            "MATTER {0}   DAWN {1}   LOGISTICS {2}/{3}"),
            View.Resources.Matter, View.Resources.Dawn,
            View.Resources.PopulationUsed, View.Resources.PopulationCapacity),
        View.Resources.MatchState,
        View.Resources.ResearchStatus,
        FText::Format(NSLOCTEXT("EchoesFieldHud", "Forces", "{0} / {1}"),
            View.Resources.LocalFaction, View.Resources.OpponentFaction)};
    Lines.RemoveAll([](const FText& Text) { return Text.IsEmpty(); });
    Panel->SetContent(NSLOCTEXT("EchoesFieldHud", "Ledger", "FIELD LEDGER"),
        Lines, {}, View.bHighContrast, Scale);
    Panel->SetVisibility(bBattlefield && View.Resources.bVisible
        ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

    Lines.Reset();
    for (const FEchoesFieldHudSelectionEntry& Entry : View.Selection.Entries)
    {
        FText Summary = FText::Format(
            NSLOCTEXT("EchoesFieldHud", "SelectionEntry",
                "{0} x{1}  HP {2}/{3}  ARM {4}  DMG {5}  {6}"),
            Entry.Name, Entry.Count, Entry.HitPoints, Entry.MaxHitPoints,
            Entry.Armor, Entry.Damage, Entry.Order);
        if (!Entry.Production.IsEmpty())
        {
            Summary = FText::Format(
                NSLOCTEXT("EchoesFieldHud", "SelectionProduction",
                    "{0}\nPRODUCTION  {1}  {2}%"),
                Summary, Entry.Production, Entry.ProductionPercent);
        }
        if (Entry.CargoCapacity > 0)
        {
            Summary = FText::Format(
                NSLOCTEXT("EchoesFieldHud", "SelectionCargo",
                    "{0}\nCARGO  {1}/{2}"),
                Summary, Entry.Cargo, Entry.CargoCapacity);
        }
        Lines.Add(Summary);
    }
    Panel = GetSection(EEchoesFieldHudSection::Selection);
    Panel->SetContent(NSLOCTEXT("EchoesFieldHud", "Selection", "SELECTION"),
        Lines, {}, View.bHighContrast, Scale);
    Panel->SetVisibility(bBattlefield && View.Selection.bVisible
        ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

    Panel = GetSection(EEchoesFieldHudSection::CommandCard);
    Lines = View.Commands.Formation.IsEmpty()
        ? TArray<FText>{}
        : TArray<FText>{FText::Format(
            NSLOCTEXT("EchoesFieldHud", "Formation", "FORMATION  {0}"),
            View.Commands.Formation)};
    Panel->SetContent(NSLOCTEXT("EchoesFieldHud", "Commands", "COMMAND CARD"),
        Lines, View.Commands.Controls, View.bHighContrast, Scale);
    Panel->SetVisibility(bBattlefield && View.Commands.bVisible
        ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

    Lines.Reset();
    for (const FEchoesFieldHudLine& Line : View.ObjectiveLines)
    {
        Lines.Add(JoinedLine(Line));
    }
    Panel = GetSection(EEchoesFieldHudSection::Objectives);
    Panel->SetContent(View.ObjectiveTitle, Lines, {}, View.bHighContrast, Scale);
    Panel->SetVisibility(bBattlefield && View.bObjectiveVisible
        ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

    Panel = GetSection(EEchoesFieldHudSection::Status);
    Panel->SetContent(NSLOCTEXT("EchoesFieldHud", "Status", "STATUS"),
        View.Status.IsEmpty() ? TArray<FText>{} : TArray<FText>{View.Status},
        {}, View.bHighContrast, Scale);
    Panel->SetVisibility(bBattlefield && !View.Status.IsEmpty()
        ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

    Panel = GetSection(EEchoesFieldHudSection::Subtitle);
    Panel->SetContent(View.SubtitleSpeaker,
        View.Subtitle.IsEmpty() ? TArray<FText>{} : TArray<FText>{View.Subtitle},
        {}, View.bHighContrast, Scale);
    Panel->SetVisibility(bBattlefield && !View.Subtitle.IsEmpty()
        ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

    TArray<FEchoesFieldHudControl> TechnologyControls =
        View.Technology.Controls;
    Lines.Reset();
    if (View.Technology.bVisible)
    {
        for (const FEchoesFieldHudTechnologyTier& Tier : View.Technology.Tiers)
        {
            Lines.Add(FText::Format(
                NSLOCTEXT("EchoesFieldHud", "TechTier", "TIER {0} — {1}: {2} [{3}]"),
                Tier.Tier + 1, Tier.Name, Tier.Cost, Tier.State));
        }
    }
    Panel = GetSection(EEchoesFieldHudSection::Technology);
    if (!View.Technology.ActiveResearch.IsEmpty())
    {
        Lines.Insert(View.Technology.ActiveResearch, 0);
    }
    Panel->SetContent(View.Technology.Title, Lines, TechnologyControls,
        View.bHighContrast, Scale);
    Panel->SetVisibility(bBattlefield && View.Technology.bVisible
        ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

    CampaignMapWidget->SetView(View.Campaign, View.bHighContrast, Scale);
    CampaignMapWidget->SetVisibility(
        View.Surface == EEchoesFieldHudSurface::CampaignOperations &&
        View.Campaign.bVisible
            ? ESlateVisibility::Visible
            : ESlateVisibility::Collapsed);
    Panel = GetSection(EEchoesFieldHudSection::CampaignInspector);
    Lines = {View.Campaign.LedgerSummary, View.Campaign.SelectedSector,
        View.Campaign.SelectedTitle, View.Campaign.SelectedBiome,
        View.Campaign.SelectedStatus, View.Campaign.Briefing,
        View.Campaign.Reward};
    Lines.RemoveAll([](const FText& Text) { return Text.IsEmpty(); });
    TArray<FEchoesFieldHudControl> CampaignInspectorControls =
        View.Campaign.Controls.FilterByPredicate(
            [](const FEchoesFieldHudControl& Control)
            {
                return Control.Action != EEchoesFieldHudAction::CampaignSelectNode;
            });
    Panel->SetContent(View.Campaign.Title, Lines, CampaignInspectorControls,
        View.bHighContrast, Scale);
    Panel->SetVisibility(
        View.Surface == EEchoesFieldHudSurface::CampaignOperations &&
        View.Campaign.bVisible
            ? ESlateVisibility::Visible
            : ESlateVisibility::Collapsed);

    Lines = {View.Online.State, View.Online.Failure};
    Lines.RemoveAll([](const FText& Text) { return Text.IsEmpty(); });
    Panel = GetSection(EEchoesFieldHudSection::OnlineFrontDoor);
    const bool bEndpointEditable = View.Online.Controls.ContainsByPredicate(
        [](const FEchoesFieldHudControl& Control)
        {
            return Control.bEnabled &&
                Control.Action == EEchoesFieldHudAction::OnlineEditEndpoint;
        });
    Panel->SetContent(View.Online.Title, Lines, View.Online.Controls,
        View.bHighContrast, Scale, bEndpointEditable, View.Online.Endpoint);
    Panel->SetVisibility(
        (View.Surface == EEchoesFieldHudSurface::OnlineFrontDoor ||
         View.Surface == EEchoesFieldHudSurface::NetworkLobby) && View.Online.bVisible
            ? ESlateVisibility::Visible
            : ESlateVisibility::Collapsed);

    Panel = GetSection(EEchoesFieldHudSection::OnlineLocalMenu);
    Panel->SetContent(View.Online.Title, Lines, View.Online.Controls,
        View.bHighContrast, Scale);
    Panel->SetVisibility(
        View.Surface == EEchoesFieldHudSurface::OnlineLocalMenu && View.Online.bVisible
            ? ESlateVisibility::Visible
            : ESlateVisibility::Collapsed);

    Panel = GetSection(EEchoesFieldHudSection::Reconnect);
    Lines = {View.Online.State, View.Online.Reconnect, View.Online.Failure};
    Lines.RemoveAll([](const FText& Text) { return Text.IsEmpty(); });
    Panel->SetContent(View.Online.Title, Lines, View.Online.Controls,
        View.bHighContrast, Scale);
    Panel->SetVisibility(
        View.Surface == EEchoesFieldHudSurface::Reconnect && View.Online.bVisible
            ? ESlateVisibility::Visible
            : ESlateVisibility::Collapsed);

    MinimapWidget->SetView(View.Minimap, View.bHighContrast);
    MinimapWidget->SetVisibility(bBattlefield && View.Minimap.bVisible
        ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    RefreshContactWidgets();
    TargetingWidget->SetView(View.Targeting, View.bHighContrast);
}

void UEchoesFieldHudWidget::GatherActionButtons(
    TArray<UEchoesFieldHudActionButton*>& OutButtons) const
{
    OutButtons.Reset();
    for (UEchoesFieldHudSectionWidget* Section : Sections)
    {
        if (Section == nullptr || Section->GetVisibility() == ESlateVisibility::Collapsed)
        {
            continue;
        }
        for (int32 Index = 0; Index < Section->GetActionButtonCount(); ++Index)
        {
            if (UEchoesFieldHudActionButton* Button = Section->GetActionButton(Index))
            {
                OutButtons.Add(Button);
            }
        }
    }
    if (CampaignMapWidget != nullptr &&
        CampaignMapWidget->GetVisibility() != ESlateVisibility::Collapsed)
    {
        for (int32 Index = 0;
             Index < CampaignMapWidget->GetNodeButtonCount();
             ++Index)
        {
            if (UEchoesFieldHudActionButton* Button =
                CampaignMapWidget->GetNodeButton(Index))
            {
                OutButtons.Add(Button);
            }
        }
    }
}

int32 UEchoesFieldHudWidget::GetActionButtonCount() const
{
    TArray<UEchoesFieldHudActionButton*> Buttons;
    GatherActionButtons(Buttons);
    return Buttons.Num();
}

void UEchoesFieldHudWidget::NotifyButtonFocused(
    UEchoesFieldHudActionButton* Button)
{
    TArray<UEchoesFieldHudActionButton*> Buttons;
    GatherActionButtons(Buttons);
    FocusedButtonIndex = Buttons.IndexOfByKey(Button);
    if (Buttons.IsValidIndex(FocusedButtonIndex))
    {
        FocusedAction = Buttons[FocusedButtonIndex]->GetAction();
        FocusedArgument = Buttons[FocusedButtonIndex]->GetArgument();
    }
}

int32 UEchoesFieldHudWidget::FindButtonIndex(
    const TArray<UEchoesFieldHudActionButton*>& Buttons,
    EEchoesFieldHudAction Action,
    int32 Argument) const
{
    if (Action == EEchoesFieldHudAction::None)
    {
        return INDEX_NONE;
    }
    return Buttons.IndexOfByPredicate(
        [Action, Argument](const UEchoesFieldHudActionButton* Button)
        {
            return Button != nullptr && Button->GetAction() == Action &&
                Button->GetArgument() == Argument;
        });
}

int32 UEchoesFieldHudWidget::FindDefaultButtonIndex(
    const TArray<UEchoesFieldHudActionButton*>& Buttons) const
{
    const int32 Preferred = Buttons.IndexOfByPredicate(
        [](const UEchoesFieldHudActionButton* Button)
        {
            return Button != nullptr && Button->GetIsEnabled() &&
                Button->IsPreferredFocus();
        });
    if (Preferred != INDEX_NONE)
    {
        return Preferred;
    }
    static constexpr EEchoesFieldHudAction SafeActions[] = {
        EEchoesFieldHudAction::OnlineResume,
        EEchoesFieldHudAction::CampaignBack,
        EEchoesFieldHudAction::OnlineBack,
        EEchoesFieldHudAction::ToggleTechnology};
    for (const EEchoesFieldHudAction SafeAction : SafeActions)
    {
        const int32 Safe = Buttons.IndexOfByPredicate(
            [SafeAction](const UEchoesFieldHudActionButton* Button)
            {
                return Button != nullptr && Button->GetIsEnabled() &&
                    Button->GetAction() == SafeAction;
            });
        if (Safe != INDEX_NONE)
        {
            return Safe;
        }
    }
    return Buttons.IndexOfByPredicate(
        [](const UEchoesFieldHudActionButton* Button)
        {
            return Button != nullptr && Button->GetIsEnabled();
        });
}

bool UEchoesFieldHudWidget::FocusButtonAtIndex(
    int32 Index,
    const TArray<UEchoesFieldHudActionButton*>& Buttons)
{
    if (!Buttons.IsValidIndex(Index) || !Buttons[Index]->GetIsEnabled())
    {
        return false;
    }
    FocusedButtonIndex = Index;
    FocusedAction = Buttons[Index]->GetAction();
    FocusedArgument = Buttons[Index]->GetArgument();
    Buttons[Index]->SetKeyboardFocus();
    for (UEchoesFieldHudSectionWidget* Section : Sections)
    {
        if (Section != nullptr && Section->ScrollActionIntoView(Buttons[Index]))
        {
            break;
        }
    }
    return true;
}

bool UEchoesFieldHudWidget::FocusDefaultAction()
{
    TArray<UEchoesFieldHudActionButton*> Buttons;
    GatherActionButtons(Buttons);
    int32 Index = FindButtonIndex(Buttons, FocusedAction, FocusedArgument);
    if (!Buttons.IsValidIndex(Index) || !Buttons[Index]->GetIsEnabled())
    {
        Index = FindDefaultButtonIndex(Buttons);
    }
    if (!Buttons.IsValidIndex(Index))
    {
        FocusedButtonIndex = INDEX_NONE;
        FocusedAction = EEchoesFieldHudAction::None;
        FocusedArgument = 0;
        return false;
    }
    if (Buttons[Index]->GetAction() ==
        EEchoesFieldHudAction::OnlineEditEndpoint)
    {
        if (UEchoesFieldHudSectionWidget* Online = GetSection(
                EEchoesFieldHudSection::OnlineFrontDoor);
            Online != nullptr && Online->FocusEndpointEditor())
        {
            FocusedButtonIndex = Index;
            FocusedAction = Buttons[Index]->GetAction();
            FocusedArgument = Buttons[Index]->GetArgument();
            return true;
        }
    }
    return FocusButtonAtIndex(Index, Buttons);
}

bool UEchoesFieldHudWidget::FocusNext(bool bReverse)
{
    TArray<UEchoesFieldHudActionButton*> Buttons;
    GatherActionButtons(Buttons);
    if (Buttons.IsEmpty())
    {
        FocusedButtonIndex = INDEX_NONE;
        return false;
    }
    const int32 Direction = bReverse ? -1 : 1;
    int32 Index = FocusedButtonIndex;
    for (int32 Attempt = 0; Attempt < Buttons.Num(); ++Attempt)
    {
        Index = (Index + Direction + Buttons.Num()) % Buttons.Num();
        if (Buttons[Index]->GetIsEnabled())
        {
            return FocusButtonAtIndex(Index, Buttons);
        }
    }
    FocusedButtonIndex = INDEX_NONE;
    return false;
}

bool UEchoesFieldHudWidget::ActivateFocused()
{
    TArray<UEchoesFieldHudActionButton*> Buttons;
    GatherActionButtons(Buttons);
    return Buttons.IsValidIndex(FocusedButtonIndex) &&
        Buttons[FocusedButtonIndex]->Activate();
}

bool UEchoesFieldHudWidget::IsModalSurface() const
{
    return View.Technology.bVisible ||
        View.Surface == EEchoesFieldHudSurface::CampaignOperations ||
        View.Surface == EEchoesFieldHudSurface::OnlineFrontDoor ||
        View.Surface == EEchoesFieldHudSurface::NetworkLobby ||
        View.Surface == EEchoesFieldHudSurface::OnlineLocalMenu ||
        View.Surface == EEchoesFieldHudSurface::Reconnect;
}

FReply UEchoesFieldHudWidget::NativeOnPreviewKeyDown(
    const FGeometry& InGeometry,
    const FKeyEvent& InKeyEvent)
{
    if (!IsModalSurface())
    {
        return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
    }
    const FKey Key = InKeyEvent.GetKey();
    const UEchoesFieldHudSectionWidget* OnlinePanel =
        GetSection(EEchoesFieldHudSection::OnlineFrontDoor);
    if (OnlinePanel != nullptr && OnlinePanel->IsEndpointEditing() &&
        Key != EKeys::Tab)
    {
        return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
    }
    if (Key == EKeys::Tab || Key == EKeys::Down || Key == EKeys::Right)
    {
        return FocusNext(InKeyEvent.IsShiftDown())
            ? FReply::Handled()
            : FReply::Unhandled();
    }
    if (Key == EKeys::Up || Key == EKeys::Left)
    {
        return FocusNext(true) ? FReply::Handled() : FReply::Unhandled();
    }
    if (Key == EKeys::Enter || Key == EKeys::SpaceBar)
    {
        return ActivateFocused() ? FReply::Handled() : FReply::Unhandled();
    }
    return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}
