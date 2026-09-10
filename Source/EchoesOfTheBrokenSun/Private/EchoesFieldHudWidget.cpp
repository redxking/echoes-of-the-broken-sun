#include "EchoesFieldHudWidget.h"

#include "EchoesInterfaceAudioSubsystem.h"
#include "EchoesPlayerController.h"
#include "EchoesHudLayout.h"
#include "EchoesHudGlyph.h"
#include "EchoesTypeface.h"
#include "Engine/Font.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"

#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Border.h"
#include "Framework/Application/SlateApplication.h"
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

FLinearColor ConsoleBackingColor(bool bHighContrast)
{
    // The instrument-bar frame should reveal the battlefield between its
    // readable panel islands. Individual panel backings retain PanelColor.
    return bHighContrast
        ? FLinearColor(0.0f, 0.0f, 0.0f, 0.98f)
        : FLinearColor(0.015f, 0.027f, 0.038f, 0.18f);
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

/** The project's vendored faces, or the engine default if one is missing.
 *
 * Space Grotesk carries interface chrome and IBM Plex Mono carries numeric
 * readouts, per the resolved typeface decision. The accessors already fall back
 * to an engine font and log when a vendored file cannot be found, so a missing
 * file degrades to readable stock text rather than to no text at all. */
[[nodiscard]] FSlateFontInfo BrandedFont(bool bReadout, int32 Size)
{
    UFont* Face = bReadout ? EchoesTypeface::Readout() : EchoesTypeface::Chrome();
    if (Face == nullptr)
    {
        return FCoreStyle::GetDefaultFontStyle(bReadout ? "Mono" : "Regular", Size);
    }
    return FSlateFontInfo(Face, Size, TEXT("Regular"));
}

void ConfigureText(
    UTextBlock* Text,
    const FText& Value,
    int32 BaseSize,
    float Scale,
    const FLinearColor& Color,
    bool bReadout = false)
{
    if (Text == nullptr)
    {
        return;
    }
    Text->SetText(Value);
    Text->SetColorAndOpacity(Color);
    Text->SetAutoWrapText(true);
    Text->SetVisibility(ESlateVisibility::HitTestInvisible);
    // The face is set here rather than inherited: every console text block is
    // created from the widget default, so inheriting left the whole HUD in
    // stock Roboto while five vendored faces shipped unused.
    Text->SetFont(BrandedFont(
        bReadout, FMath::Clamp(FMath::RoundToInt(BaseSize * Scale), 10, 36)));
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

/**
 * SPEC-HUD-003 asks the selection card to answer purpose, strong use, limitation
 * and counterplay. The bottom console only has room for the full answer when one
 * thing is selected; a mixed selection keeps the per-entry vitals readable and
 * shows purpose alone, which is the part that still identifies each entry.
 */
FText SelectionDetails(
    const FEchoesFieldHudSelectionEntry& Entry,
    const bool bSingleSelection)
{
    FText Summary = FText::Format(
        NSLOCTEXT("EchoesFieldHud", "SelectionVitals",
            "{0}   ARMOR {1}   DAMAGE {2}"),
        Entry.Order, Entry.Armor, Entry.Damage);
    if (!Entry.Role.IsEmpty())
    {
        Summary = FText::Format(
            NSLOCTEXT("EchoesFieldHud", "SelectionRole",
                "ROLE  {0}\n{1}"),
            Entry.Role, Summary);
    }
    if (!Entry.Purpose.IsEmpty())
    {
        Summary = FText::Format(
            NSLOCTEXT("EchoesFieldHud", "SelectionPurpose",
                "{0}\nPURPOSE  {1}"),
            Summary, Entry.Purpose);
    }
    if (bSingleSelection)
    {
        if (!Entry.StrongUse.IsEmpty())
        {
            Summary = FText::Format(
                NSLOCTEXT("EchoesFieldHud", "SelectionStrongUse",
                    "{0}\nSTRONG USE  {1}"),
                Summary, Entry.StrongUse);
        }
        if (!Entry.Limitation.IsEmpty())
        {
            Summary = FText::Format(
                NSLOCTEXT("EchoesFieldHud", "SelectionLimitation",
                    "{0}\nLIMITATION  {1}"),
                Summary, Entry.Limitation);
        }
        if (!Entry.Counterplay.IsEmpty())
        {
            Summary = FText::Format(
                NSLOCTEXT("EchoesFieldHud", "SelectionCounterplay",
                    "{0}\nCOUNTERPLAY  {1}"),
                Summary, Entry.Counterplay);
        }
    }
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
    if (!Entry.Faction.IsEmpty())
        Summary = FText::Format(NSLOCTEXT("EchoesFieldHud", "SelectionFaction", "{0}  ·  {1}"), Entry.Faction, Summary);
    return Summary;
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
    OnLostFocus.BindUObject(this, &UEchoesFieldHudActionButton::HandleLostFocus);
    OnClicked.AddUniqueDynamic(this, &UEchoesFieldHudActionButton::HandleClicked);
    OnHovered.AddUniqueDynamic(this, &UEchoesFieldHudActionButton::HandleHovered);
    OnUnhovered.AddUniqueDynamic(this, &UEchoesFieldHudActionButton::HandleUnhovered);

    // Dark focus fill keeps ceramic labels legible; the border and focus marker
    // carry the accent instead of washing out the entire control.
    FButtonStyle Style;
    Style.Hovered = FSlateRoundedBoxBrush(FLinearColor(.055f,.10f,.12f,1), 1.f, AccentColor(bHighContrast), 2.f);
    Style.Pressed = FSlateRoundedBoxBrush(FLinearColor(.16f,.13f,.075f,1), 1.f, FLinearColor(.9f,.65f,.25f,1), 2.f);
    Style.Disabled = FSlateColorBrush(FLinearColor(.025f,.03f,.032f,1));
    // The ledger already owns scaled padding. Its monitor action wraps the
    // whole readout, so ordinary fixed button padding would overflow at 80%
    // and 100%, and pressed padding would shift the telemetry while clicking.
    const bool bResourceMonitor = Action == EEchoesFieldHudAction::OpenResourceMonitor;
    Style.NormalPadding = bResourceMonitor ? FMargin(0) : FMargin(6, 5);
    Style.PressedPadding = bResourceMonitor ? FMargin(0) : FMargin(6, 6, 6, 4);
    SetStyle(Style);
    RefreshKeyboardPresentation();

    UTextBlock* Label = PresentationLabel ? PresentationLabel.Get() : Cast<UTextBlock>(GetContent());
    if (Label)
    {
        FText Text = InControl.Detail.IsEmpty()
            ? InControl.Label
            : FText::Format(
                NSLOCTEXT("EchoesFieldHud", "ControlWithDetail", "{0}\n{1}"),
                InControl.Label,
                InControl.Detail);
        ConfigureText(Label, Text, 18, InScale, TextColor(bHighContrast));
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
    bKeyboardFocused = true;
    RefreshKeyboardPresentation();
    if (UEchoesFieldHudWidget* Current = Owner.Get())
    {
        Current->NotifyButtonFocused(this);
    }
    if (!bPointerHovered)
    {
        PlayFieldInterfaceCue(this, EEchoesInterfaceCue::Hover);
    }
}

void UEchoesFieldHudActionButton::HandleLostFocus()
{
    bKeyboardFocused = false;
    RefreshKeyboardPresentation();
}

void UEchoesFieldHudActionButton::RefreshKeyboardPresentation()
{
    FButtonStyle Style = GetStyle();
    const FLinearColor Resting = bFocusedPresentation
        ? FLinearColor(.055f,.11f,.13f,1) : FLinearColor(.035f,.047f,.052f,1);
    const FLinearColor Edge = bKeyboardFocused
        ? (bHighContrast ? FLinearColor::Yellow : FLinearColor(.95f,.68f,.25f,1))
        : bFocusedPresentation ? AccentColor(bHighContrast) : FLinearColor(.16f,.25f,.27f,1);
    Style.Normal = FSlateRoundedBoxBrush(bKeyboardFocused
        ? FLinearColor(.13f,.105f,.055f,1) : Resting, 1.f, Edge,
        bKeyboardFocused ? 2.f : 1.f);
    SetStyle(Style);
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
    const bool bRefresh = bHighContrast == bInHighContrast &&
        FMath::IsNearlyEqual(Scale, FMath::Clamp(InScale, .8f, 1.5f)) && CanRefreshInPlace(
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
        Section == EEchoesFieldHudSection::Subtitle ||
        Section == EEchoesFieldHudSection::ResourceLedger;
    RootBorder->SetPadding(bCompact
        ? FMargin(8.0f * Scale, 4.0f * Scale)
        : FMargin(10.0f * Scale));
    RootBorder->SetBrushColor(PanelColor(bHighContrast));
    if (TitleText != nullptr)
    {
        ConfigureText(TitleText, Title, bCompact ? 14 : 16, Scale,
            AccentColor(bHighContrast));
    }
    for (int32 Index = 0; Index < Lines.Num(); ++Index)
    {
        ConfigureText(LineTexts[Index], Lines[Index], 18, Scale, TextColor(bHighContrast));
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

void UEchoesFieldHudSectionWidget::SetResourceTelemetry(const FEchoesFieldHudResourceView& Resources)
{
    ResourceTelemetry = Resources;
    if (Section != EEchoesFieldHudSection::ResourceLedger) return;
    if (RootBorder && ResourceValues.Num() != 3) RebuildContent();
    if (ResourceValues.Num() != 3 || ResourceIdentityText == nullptr ||
        ResourceSummaryText == nullptr) return;
    ResourceValues[0]->SetText(FText::AsNumber(Resources.Matter));
    ResourceValues[1]->SetText(FText::AsNumber(Resources.Dawn));
    ResourceValues[2]->SetText(FText::Format(NSLOCTEXT("EchoesFieldHud", "LogisticsValue", "{0}/{1}"),
        Resources.PopulationUsed, Resources.PopulationCapacity));
    ResourceValues[2]->SetColorAndOpacity(Resources.PopulationUsed > Resources.PopulationCapacity
        ? ToneColor(EEchoesFieldHudTone::Warning, bHighContrast) : TextColor(bHighContrast));
    TArray<FText> Factions{Resources.LocalFaction, Resources.OpponentFaction};
    Factions.RemoveAll([](const FText& Text) { return Text.IsEmpty(); });
    ResourceIdentityText->SetText(FText::Join(FText::FromString(TEXT("  //  ")), Factions));
    TArray<FText> Summary{Resources.MatchState, Resources.ResearchStatus};
    Summary.RemoveAll([](const FText& Text) { return Text.IsEmpty(); });
    ResourceSummaryText->SetText(FText::Join(FText::FromString(TEXT("  //  ")), Summary));
    const FText ResourceTooltip = FText::Format(
        NSLOCTEXT("EchoesFieldHud", "ResourceContext", "{0}\nMatter {1}; Dawn {2}; Logistics {3}/{4}\n{5}"),
        ResourceIdentityText->GetText(), Resources.Matter, Resources.Dawn,
        Resources.PopulationUsed, Resources.PopulationCapacity, ResourceSummaryText->GetText());
    if (ResourceActionButton != nullptr)
    {
        ResourceActionButton->SetToolTipText(FText::Format(
            NSLOCTEXT("EchoesFieldHud", "ResourceActionTooltip", "{0}\n\n{1}"),
            ResourceTooltip, ResourceTelemetry.MonitorControl.Label));
    }
    else
    {
        SetToolTipText(ResourceTooltip);
    }
}

void UEchoesFieldHudSectionWidget::SetSelectionTelemetry(const FEchoesFieldHudSelectionView& Selection)
{
    bool bRebuild = SelectionEntries.Num() != Selection.Entries.Num();
    for (int32 I = 0; !bRebuild && I < SelectionEntries.Num(); ++I)
        bRebuild = SelectionEntries[I].EntityId != Selection.Entries[I].EntityId;
    SelectionEntries = Selection.Entries;
    if (bRebuild && RootBorder) RebuildContent();
    for (int32 I = 0; I < SelectionEntries.Num() && I < HealthBars.Num(); ++I)
    {
        const auto& Entry = SelectionEntries[I];
        TelemetryDetails[I]->SetText(
            SelectionDetails(Entry, SelectionEntries.Num() == 1));
        TelemetryLabels[I]->SetText(FText::Format(NSLOCTEXT("EchoesFieldHud", "HealthTelemetry", "{0}  ×{1}     HEALTH {2}/{3}"),
            Entry.Name, Entry.Count, Entry.HitPoints, Entry.MaxHitPoints));
        HealthBars[I]->SetPercent(Entry.MaxHitPoints > 0
            ? FMath::Clamp(static_cast<float>(Entry.HitPoints) / Entry.MaxHitPoints, 0.f, 1.f) : 0.f);
        HealthBars[I]->SetFillColorAndOpacity(AccentColor(bHighContrast));
    }
}

int32 UEchoesFieldHudSectionWidget::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry,
    const FSlateRect& Clip, FSlateWindowElementList& Elements, int32 Layer,
    const FWidgetStyle& Style, bool bEnabled) const
{
    const int32 Base = Super::NativePaint(Args, Geometry, Clip, Elements, Layer, Style, bEnabled);
    const FVector2D Size = Geometry.GetLocalSize();
    if (Size.X < 20 || Size.Y < 10) return Base;
    const float Cut = FMath::Min(10.f * Scale, static_cast<float>(Size.Y / 4));
    const FLinearColor Edge = bHighContrast ? FLinearColor::White : FLinearColor(.13f,.30f,.33f,1);
    TArray<FVector2D> Frame{{1,Cut},{Cut,1},{Size.X-2,1},{Size.X-2,Size.Y-Cut},
        {Size.X-Cut,Size.Y-2},{1,Size.Y-2},{1,Cut}};
    FSlateDrawElement::MakeLines(Elements, Base + 1, Geometry.ToPaintGeometry(), Frame,
        ESlateDrawEffect::None, Edge, true, 1.f);
    DrawLine(Elements, Base + 1, Geometry, {{Cut+5,1}, {FMath::Min(Size.X-5.,80.*Scale),1}},
        Section == EEchoesFieldHudSection::Objectives || Section == EEchoesFieldHudSection::Status
            ? FLinearColor(.85f,.55f,.18f,1) : AccentColor(bHighContrast), 2.f);
    return Base + 1;
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
        Section == EEchoesFieldHudSection::Subtitle ||
        Section == EEchoesFieldHudSection::ResourceLedger;
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
    RootBorder->SetClipping(Section == EEchoesFieldHudSection::ResourceLedger
        ? EWidgetClipping::ClipToBounds : EWidgetClipping::Inherit);
    RootBorder->SetVisibility(ESlateVisibility::Visible);
    TitleText = nullptr;
    if (Section != EEchoesFieldHudSection::Status && Section != EEchoesFieldHudSection::ResourceLedger)
    {
        TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        ConfigureText(TitleText, Title, bCompact ? 14 : 16, Scale,
            AccentColor(bHighContrast));
        ContentBox->AddChildToVerticalBox(TitleText)->SetPadding(
            FMargin(0, 0, 0, bCompact ? 2.0f * Scale : 5.0f));
    }

    ResourceLabels.Reset();
    ResourceValues.Reset();
    ResourceIdentityText = nullptr;
    ResourceSummaryText = nullptr;
    ResourceActionButton = nullptr;
    if (Section == EEchoesFieldHudSection::ResourceLedger)
    {
        UHorizontalBox* ResourceRow = WidgetTree->ConstructWidget<UHorizontalBox>();
        ContentBox->AddChildToVerticalBox(ResourceRow);
        const FText Labels[] = {
            NSLOCTEXT("EchoesFieldHud", "MatterLabel", "MATTER"),
            NSLOCTEXT("EchoesFieldHud", "DawnLabel", "DAWN"),
            NSLOCTEXT("EchoesFieldHud", "LogisticsLabel", "LOGISTICS")};
        for (int32 Index = 0; Index < 3; ++Index)
        {
            UHorizontalBox* Column = WidgetTree->ConstructWidget<UHorizontalBox>();
            auto* ColumnSlot = ResourceRow->AddChildToHorizontalBox(Column);
            ColumnSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            ColumnSlot->SetPadding(FMargin(2 * Scale, 0));
            UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
            ConfigureText(Label, Labels[Index], 14, Scale,
                Index == 0 ? AccentColor(bHighContrast) : Index == 1
                    ? ToneColor(EEchoesFieldHudTone::Warning, bHighContrast) : TextColor(bHighContrast));
            Column->AddChildToHorizontalBox(Label)->SetPadding(FMargin(0, 0, 3 * Scale, 0));
            ResourceLabels.Add(Label);
            UTextBlock* Value = WidgetTree->ConstructWidget<UTextBlock>();
            ConfigureText(Value, FText::GetEmpty(), 18, Scale, TextColor(bHighContrast), true);
            Column->AddChildToHorizontalBox(Value);
            ResourceValues.Add(Value);
        }
        ResourceIdentityText = WidgetTree->ConstructWidget<UTextBlock>();
        ConfigureText(ResourceIdentityText, FText::GetEmpty(), 14, Scale,
            ToneColor(EEchoesFieldHudTone::Muted, bHighContrast));
        ResourceIdentityText->SetAutoWrapText(false);
        ContentBox->AddChildToVerticalBox(ResourceIdentityText);
        ResourceSummaryText = WidgetTree->ConstructWidget<UTextBlock>();
        ConfigureText(ResourceSummaryText, FText::GetEmpty(), 14, Scale,
            ToneColor(EEchoesFieldHudTone::Muted, bHighContrast));
        ResourceSummaryText->SetAutoWrapText(false);
        ContentBox->AddChildToVerticalBox(ResourceSummaryText);
        SetResourceTelemetry(ResourceTelemetry);
        if (Controls.Num() == 1 &&
            Controls[0].Action != EEchoesFieldHudAction::None)
        {
            // Preserve the compact readout tree: its one semantic action wraps
            // every existing telemetry child instead of adding a second label.
            ResourceActionButton = WidgetTree->ConstructWidget<UEchoesFieldHudActionButton>(
                UEchoesFieldHudActionButton::StaticClass(), TEXT("ResourceMonitorAction"));
            ContentBox->RemoveFromParent();
            ResourceActionButton->SetContent(ContentBox);
            if (UButtonSlot* ContentSlot = Cast<UButtonSlot>(ContentBox->Slot))
            {
                ContentSlot->SetHorizontalAlignment(HAlign_Fill);
                ContentSlot->SetVerticalAlignment(VAlign_Fill);
            }
            ResourceActionButton->Configure(
                Owner.Get(), Controls[0], bHighContrast, Scale);
            ResourceActionButton->SetToolTipText(FText::Format(
                NSLOCTEXT("EchoesFieldHud", "ResourceMonitorTooltip", "{0}. {1}"),
                Controls[0].Label, Controls[0].Detail));
            RootBorder->SetContent(ResourceActionButton);
            ActionButtons.Add(ResourceActionButton);
        }
    }

    TelemetryLabels.Reset();
    TelemetryDetails.Reset();
    HealthBars.Reset();
    for (const auto& Entry : SelectionEntries)
    {
        UTextBlock* Readout = WidgetTree->ConstructWidget<UTextBlock>();
        ConfigureText(Readout, FText::Format(NSLOCTEXT("EchoesFieldHud", "HealthTelemetry", "{0}  ×{1}     HEALTH {2}/{3}"),
            Entry.Name, Entry.Count, Entry.HitPoints, Entry.MaxHitPoints), 18, Scale, TextColor(bHighContrast));
        ContentBox->AddChildToVerticalBox(Readout)->SetPadding(FMargin(0, 2, 0, 4));
        TelemetryLabels.Add(Readout);
        UProgressBar* Health = WidgetTree->ConstructWidget<UProgressBar>();
        Health->SetVisibility(ESlateVisibility::HitTestInvisible);
        FProgressBarStyle TrackStyle;
        TrackStyle.BackgroundImage = FSlateColorBrush(FLinearColor(.055f,.07f,.075f,1));
        TrackStyle.FillImage = FSlateColorBrush(FLinearColor::White);
        Health->SetWidgetStyle(TrackStyle);
        Health->SetBorderPadding(FVector2D::ZeroVector);
        Health->SetPercent(Entry.MaxHitPoints > 0 ? FMath::Clamp(static_cast<float>(Entry.HitPoints) / Entry.MaxHitPoints, 0.f, 1.f) : 0.f);
        Health->SetFillColorAndOpacity(AccentColor(bHighContrast));
        USizeBox* Track = WidgetTree->ConstructWidget<USizeBox>();
        Track->SetHeightOverride(5.f * Scale);
        Track->SetContent(Health);
        ContentBox->AddChildToVerticalBox(Track)->SetPadding(FMargin(0, 0, 0, 9 * Scale));
        HealthBars.Add(Health);
        UTextBlock* Detail = WidgetTree->ConstructWidget<UTextBlock>();
        ConfigureText(Detail, SelectionDetails(Entry, SelectionEntries.Num() == 1),
            16, Scale, TextColor(bHighContrast));
        ContentBox->AddChildToVerticalBox(Detail)->SetPadding(FMargin(0, 0, 0, 10 * Scale));
        TelemetryDetails.Add(Detail);
    }

    for (const FText& Line : Lines)
    {
        UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        ConfigureText(Text, Line, 18, Scale, TextColor(bHighContrast));
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
    if (!Controls.IsEmpty() && Section != EEchoesFieldHudSection::ResourceLedger)
    {
        Grid = WidgetTree->ConstructWidget<UUniformGridPanel>(
            UUniformGridPanel::StaticClass());
        Grid->SetSlotPadding(FMargin(3.0f));
        ContentBox->AddChildToVerticalBox(Grid)->SetPadding(FMargin(0, 6, 0, 0));
    }
    const int32 Columns = Section == EEchoesFieldHudSection::CommandCard ? 3 : 2;
    for (int32 Index = 0;
         Section != EEchoesFieldHudSection::ResourceLedger && Index < Controls.Num();
         ++Index)
    {
        const FEchoesFieldHudControl& Control = Controls[Index];
        UEchoesFieldHudActionButton* Button =
            WidgetTree->ConstructWidget<UEchoesFieldHudActionButton>(
                UEchoesFieldHudActionButton::StaticClass());
        UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(
            UTextBlock::StaticClass());
        Button->SetPresentationLabel(Label);
        if (Section == EEchoesFieldHudSection::CommandCard)
        {
            UVerticalBox* Tile = WidgetTree->ConstructWidget<UVerticalBox>();
            UEchoesHudGlyph* Glyph = WidgetTree->ConstructWidget<UEchoesHudGlyph>();
            Glyph->SetControl(Control, bHighContrast);
            USizeBox* IconBox = WidgetTree->ConstructWidget<USizeBox>();
            IconBox->SetHeightOverride(42.f * Scale);
            IconBox->SetContent(Glyph);
            Tile->AddChildToVerticalBox(IconBox);
            Label->SetJustification(ETextJustify::Center);
            Tile->AddChildToVerticalBox(Label)->SetPadding(FMargin(2, 3, 2, 2));
            Button->SetContent(Tile);
        }
        else Button->SetContent(Label);
        if (UButtonSlot* ContentSlot = Cast<UButtonSlot>(Button->GetContent()->Slot))
            ContentSlot->SetHorizontalAlignment(HAlign_Fill);
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
    bool bInHighContrast,
    bool bInReducedFlashing)
{
    View = InView;
    bHighContrast = bInHighContrast;
    bReducedFlashing = bInReducedFlashing;
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
        // Mission labels identify minimap destinations but must not consume
        // the pointer press intended to navigate to that destination.
        Label->SetVisibility(ESlateVisibility::HitTestInvisible);
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
    // The attack pulse: three expanding rings from the raised location, so the
    // eye is pulled to a place rather than merely told that something happened
    // somewhere. Reduced flashing holds it as a single static ring, which is
    // still a location and still readable.
    if (View.Alert.bActive)
    {
        const FVector2D AlertPoint = MapPoint(View.Alert.NormalizedMapPosition);
        const FLinearColor AlertColor = bHighContrast
            ? FLinearColor(1.0f, 0.85f, 0.1f, 1.0f)
            : FLinearColor(1.0f, 0.35f, 0.2f, 0.95f);
        const double Now = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.0;
        const double Age = FMath::Max(0.0, Now - View.Alert.RaisedSeconds);
        const int32 RingCount = bReducedFlashing ? 1 : 3;
        for (int32 Ring = 0; Ring < RingCount; ++Ring)
        {
            const double Phase = bReducedFlashing
                ? 0.55
                : FMath::Fmod(Age * 1.6 + Ring * 0.33, 1.0);
            const float Radius = static_cast<float>(4.0 + Phase * 12.0);
            const float Alpha = static_cast<float>(1.0 - Phase);
            DrawBox(OutDrawElements, BaseLayer + 6, AllottedGeometry,
                AlertPoint - FVector2D(Radius, Radius),
                FVector2D(Radius * 2.0f, 2.0f),
                AlertColor.CopyWithNewOpacity(Alpha * AlertColor.A));
            DrawBox(OutDrawElements, BaseLayer + 6, AllottedGeometry,
                AlertPoint - FVector2D(Radius, -Radius),
                FVector2D(Radius * 2.0f, 2.0f),
                AlertColor.CopyWithNewOpacity(Alpha * AlertColor.A));
            DrawBox(OutDrawElements, BaseLayer + 6, AllottedGeometry,
                AlertPoint - FVector2D(Radius, Radius),
                FVector2D(2.0f, Radius * 2.0f),
                AlertColor.CopyWithNewOpacity(Alpha * AlertColor.A));
            DrawBox(OutDrawElements, BaseLayer + 6, AllottedGeometry,
                AlertPoint + FVector2D(Radius, -Radius),
                FVector2D(2.0f, Radius * 2.0f),
                AlertColor.CopyWithNewOpacity(Alpha * AlertColor.A));
        }
    }
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
    DrawLine(OutDrawElements, BaseLayer + 7, AllottedGeometry,
        {{1,1},{Size.X-1,1},{Size.X-1,Size.Y-1},{1,Size.Y-1}}, AccentColor(bHighContrast), 1.5f, true);
    return BaseLayer + 7;
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
    if (View.Surface == EEchoesFieldHudSurface::Battlefield || View.Surface == EEchoesFieldHudSurface::Replay)
    {
        const FEchoesHudLayout Layout = ResolveConsoleLayout();
        if ((Layout.bBottomBarVisible &&
             Layout.BottomBar.IsInsideOrOn(ScreenPosition)) ||
            (Layout.bMenuVisible && Layout.MenuPanel.IsInsideOrOn(ScreenPosition)))
        {
            return true;
        }
    }
    if (View.bTutorialActive && !View.TutorialSkipModal.bVisible)
    {
        const float PanelWidth = 190.0f;
        const float PanelHeight = 34.0f;
        const FVector2D LocalSize = RootGeometry.GetLocalSize();
        const FVector2D SkipPos(LocalSize.X - PanelWidth - 20.0f, 16.0f);
        const FBox2D SkipBox(SkipPos, SkipPos + FVector2D(PanelWidth, PanelHeight));
        const FVector2D LocalPointer = ScreenPosition / FMath::Max(0.01f, ViewportScale);
        if (SkipBox.IsInside(LocalPointer))
        {
            return true;
        }
    }
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

    // SObjectWidget paints children before NativePaint. Put the backing in
    // the canvas itself so it cannot be composited over text and controls.
    ConsoleBacking = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), TEXT("ConsoleBacking"));
    ConsoleBacking->SetVisibility(ESlateVisibility::Collapsed);
    auto* BackingSlot = RootCanvas->AddChildToCanvas(ConsoleBacking);
    BackingSlot->SetZOrder(-100);

    MenuButton = WidgetTree->ConstructWidget<UEchoesFieldHudActionButton>(
        UEchoesFieldHudActionButton::StaticClass(), TEXT("BattlefieldMenuButton"));
    UTextBlock* MenuLabel = WidgetTree->ConstructWidget<UTextBlock>(
        UTextBlock::StaticClass(), TEXT("BattlefieldMenuLabel"));
    MenuButton->SetContent(MenuLabel);
    MenuButton->SetPresentationLabel(MenuLabel);
    MenuButton->SetVisibility(ESlateVisibility::Collapsed);
    UCanvasPanelSlot* MenuSlot = RootCanvas->AddChildToCanvas(MenuButton);
    MenuSlot->SetZOrder(90);

    // NativePaint does not register mouse targets. This transparent child
    // supplies the exact painted skip rectangle to Slate's hit-test path;
    // its unhandled press bubbles to our existing hold/capture handler.
    TutorialSkipHitTarget = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), TEXT("TutorialSkipHitTarget"));
    TutorialSkipHitTarget->SetBrushColor(FLinearColor::Transparent);
    TutorialSkipHitTarget->SetToolTipText(NSLOCTEXT("EchoesFieldHud", "SkipHoldHelp",
        "Hold Space, or hold the left mouse button here, to review tutorial skip options."));
    TutorialSkipHitTarget->SetVisibility(ESlateVisibility::Collapsed);
    auto* SkipSlot = RootCanvas->AddChildToCanvas(TutorialSkipHitTarget);
    SkipSlot->SetAnchors(FAnchors(1, 0));
    SkipSlot->SetOffsets(FMargin(-210, 16, 190, 34));
    SkipSlot->SetZOrder(100);


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
    AddSection(EEchoesFieldHudSection::TutorialModal,
        FAnchors(0.30f, 0.28f, 0.70f, 0.72f), FMargin(0));

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
    // The pass below consumes one widget per valid contact. A hidden surface
    // requires none, so the array was just emptied; walking the contacts anyway
    // would index element zero of an empty array and abort the process.
    if (ContactWidgets.IsEmpty())
    {
        return;
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
    if (TutorialSkipHitTarget)
        TutorialSkipHitTarget->SetVisibility(View.bTutorialActive && !View.TutorialSkipModal.bVisible
            ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    const float Scale = FMath::Clamp(View.HudScale, 0.8f, 1.5f);
    const bool bBattlefield =
        View.Surface == EEchoesFieldHudSurface::Battlefield ||
        View.Surface == EEchoesFieldHudSurface::Replay;

    if (MenuButton != nullptr)
    {
        MenuButton->Configure(this, View.Menu.Control, View.bHighContrast, Scale);
        MenuButton->SetVisibility(bBattlefield && View.Menu.bVisible
            ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }

    TArray<FText> Lines;
    UEchoesFieldHudSectionWidget* Panel = GetSection(
        EEchoesFieldHudSection::ResourceLedger);
    TArray<FEchoesFieldHudControl> ResourceControls;
    if (View.Resources.MonitorControl.Action != EEchoesFieldHudAction::None)
    {
        ResourceControls.Add(View.Resources.MonitorControl);
    }
    Panel->SetContent(FText::GetEmpty(), {}, ResourceControls, View.bHighContrast, Scale);
    Panel->SetResourceTelemetry(View.Resources);
    Panel->SetVisibility(bBattlefield && View.Resources.bVisible
        ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

    Lines.Reset();
    if (View.Production.bVisible)
    {
        if (View.Production.Cancellation.bVisible)
        {
            const FEchoesFieldHudProductionCancellationView& Cancellation =
                View.Production.Cancellation;
            Lines.Add(NSLOCTEXT(
                "EchoesFieldHud", "ProductionCancellationHeading",
                "CANCEL PRODUCTION?"));
            Lines.Add(Cancellation.bActive
                ? FText::Format(
                      Cancellation.RefundPercent == 75
                          ? NSLOCTEXT(
                                "EchoesFieldHud",
                                "ProductionCancellationActiveBelowHalf",
                                "{0}  {1}% COMPLETE\nBELOW 50% // REFUND RATE {2}%\nINVESTED  {3} Matter / {4} Dawn\nREFUND  {5} Matter / {6} Dawn")
                          : NSLOCTEXT(
                                "EchoesFieldHud",
                                "ProductionCancellationActiveHalfOrLater",
                                "{0}  {1}% COMPLETE\nAT OR ABOVE 50% // REFUND RATE {2}%\nINVESTED  {3} Matter / {4} Dawn\nREFUND  {5} Matter / {6} Dawn"),
                      Cancellation.Unit,
                      FText::AsNumber(Cancellation.ProgressPercent),
                      FText::AsNumber(Cancellation.RefundPercent),
                      FText::AsNumber(Cancellation.InvestedMatter),
                      FText::AsNumber(Cancellation.InvestedDawn),
                      FText::AsNumber(Cancellation.RefundMatter),
                      FText::AsNumber(Cancellation.RefundDawn))
                : FText::Format(
                      NSLOCTEXT(
                          "EchoesFieldHud", "ProductionCancellationWaiting",
                          "WAITING {0}\nNOT YET CHARGED\nREFUND  {1} Matter / {2} Dawn"),
                      Cancellation.Unit,
                      FText::AsNumber(Cancellation.RefundMatter),
                      FText::AsNumber(Cancellation.RefundDawn)));
        }
        else
        {
            Lines.Add(NSLOCTEXT(
                "EchoesFieldHud", "ProductionQueueHeading", "PRODUCTION QUEUE"));
            if (View.Production.Items.IsEmpty())
            {
                Lines.Add(NSLOCTEXT(
                    "EchoesFieldHud", "ProductionQueueEmpty", "ACTIVE  NONE"));
            }
            for (const FEchoesFieldHudProductionItem& Item : View.Production.Items)
            {
                if (Item.bActive)
                {
                    Lines.Add(FText::Format(
                        NSLOCTEXT(
                            "EchoesFieldHud",
                            "ProductionActiveItem",
                            "ACTIVE  {0}  {1}%\nINVESTED  {2} Matter / {3} Dawn\nLOGISTICS  {4} RESERVED"),
                        Item.Unit,
                        FText::AsNumber(Item.ProgressPercent),
                        FText::AsNumber(Item.InvestedMatter),
                        FText::AsNumber(Item.InvestedDawn),
                        FText::AsNumber(Item.Logistics)));
                }
                else
                {
                    Lines.Add(FText::Format(
                        NSLOCTEXT(
                            "EchoesFieldHud",
                            "ProductionWaitingItem",
                            "WAITING {0}  {1}\nUNPAID COST  {2} Matter / {3} Dawn\nLOGISTICS  {4} ON START"),
                        FText::AsNumber(Item.Slot),
                        Item.Unit,
                        FText::AsNumber(Item.ConfiguredMatter),
                        FText::AsNumber(Item.ConfiguredDawn),
                        FText::AsNumber(Item.Logistics)));
                }
            }
            if (View.Production.RallyWaypointCount > 0)
            {
                Lines.Add(FText::Format(
                    NSLOCTEXT(
                        "EchoesFieldHud",
                        "ProductionRallyRoute",
                        "RALLY  {0} {0}|plural(one=WAYPOINT,other=WAYPOINTS)"),
                    View.Production.RallyWaypointCount));
            }
            if (View.Production.bRallyNeedsAttention)
            {
                Lines.Add(NSLOCTEXT(
                    "EchoesFieldHud", "ProductionRallyAlert", "RALLY ROUTE NEEDS ATTENTION"));
            }
            if (View.Production.bSpawnBlocked)
            {
                Lines.Add(NSLOCTEXT(
                    "EchoesFieldHud", "ProductionSpawnBlocked", "[SPAWN BLOCKED] Clear the emergence area."));
            }
        }
    }
    Panel = GetSection(EEchoesFieldHudSection::Selection);
    Panel->SetSelectionTelemetry(View.Selection);
    Panel->SetContent(NSLOCTEXT("EchoesFieldHud", "Selection", "SELECTION"),
        Lines, View.Production.Controls, View.bHighContrast, Scale);
    Panel->SetVisibility(bBattlefield && View.Selection.bVisible
        ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

    Panel = GetSection(EEchoesFieldHudSection::CommandCard);
    Lines = View.Commands.Formation.IsEmpty()
        ? TArray<FText>{}
        : TArray<FText>{FText::Format(
            NSLOCTEXT("EchoesFieldHud", "Formation", "FORMATION  {0}"),
            View.Commands.Formation)};
    if (!View.Commands.AbilityStatus.IsEmpty()) Lines.Add(View.Commands.AbilityStatus);
    auto CommandControls = View.Commands.Controls;
    for (auto& Control : CommandControls)
    {
        // Availability is readable text outside the disabled input subtree.
        // Timers must not resize buttons or shift adjacent command targets.
        if (Control.Action == EEchoesFieldHudAction::ActivateRelaySupply ||
            (Control.Action == EEchoesFieldHudAction::CommandDeck &&
             Control.Argument == static_cast<int32>(EEchoesCommandDeckAction::ToggleBulwarkDeployment)))
            Control.Detail = FText::GetEmpty();
    }
    Panel->SetContent(NSLOCTEXT("EchoesFieldHud", "Commands", "COMMAND CARD"),
        Lines, CommandControls, View.bHighContrast, Scale);
    Panel->SetVisibility(bBattlefield && View.Commands.bVisible
        ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

    Lines.Reset();
    for (const FEchoesFieldHudLine& Line : View.ObjectiveLines)
    {
        Lines.Add(JoinedLine(Line));
    }
    Panel = GetSection(EEchoesFieldHudSection::Objectives);
    // Captions remain readable in the console without hiding objective controls.
    if (!View.Subtitle.IsEmpty())
        Lines.Insert(FText::Format(NSLOCTEXT("EchoesFieldHud", "CaptionInConsole", "{0}: {1}"),
            View.SubtitleSpeaker, View.Subtitle), 0);
    if (View.bTutorialActive && !View.Status.IsEmpty()) Lines.Add(View.Status);
    Panel->SetContent(View.ObjectiveTitle, Lines, View.ObjectiveControls, View.bHighContrast, Scale);
    Panel->SetVisibility(bBattlefield && (View.bObjectiveVisible || !View.Subtitle.IsEmpty() || (View.bTutorialActive && !View.Status.IsEmpty()))
        ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

    Panel = GetSection(EEchoesFieldHudSection::Status);
    Panel->SetContent(NSLOCTEXT("EchoesFieldHud", "Status", "STATUS"),
        View.Status.IsEmpty() ? TArray<FText>{} : TArray<FText>{View.Status},
        {}, View.bHighContrast, Scale);
    Panel->SetVisibility(bBattlefield && !View.bTutorialActive && !View.Status.IsEmpty()
        ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

    Panel = GetSection(EEchoesFieldHudSection::Subtitle);
    Panel->SetContent(View.SubtitleSpeaker,
        View.Subtitle.IsEmpty() ? TArray<FText>{} : TArray<FText>{View.Subtitle},
        {}, View.bHighContrast, Scale);
    Panel->SetVisibility(ESlateVisibility::Collapsed); // Caption is published with objectives above.

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

    Panel = GetSection(EEchoesFieldHudSection::TutorialModal);
    if (Panel != nullptr)
    {
        Lines.Reset();
        if (!View.TutorialSkipModal.Description.IsEmpty())
        {
            Lines.Add(View.TutorialSkipModal.Description);
        }
        Panel->SetContent(
            View.TutorialSkipModal.Title,
            Lines,
            View.TutorialSkipModal.Controls,
            View.bHighContrast,
            Scale);
        Panel->SetVisibility(
            View.TutorialSkipModal.bVisible
                ? ESlateVisibility::Visible
                : ESlateVisibility::Collapsed);
    }

    MinimapWidget->SetView(
        View.Minimap, View.bHighContrast, View.bReducedFlashing);
    MinimapWidget->SetVisibility(bBattlefield && View.Minimap.bVisible
        ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    RefreshContactWidgets();
    TargetingWidget->SetView(View.Targeting, View.bHighContrast);
}

void UEchoesFieldHudWidget::GatherActionButtons(
    TArray<UEchoesFieldHudActionButton*>& OutButtons) const
{
    OutButtons.Reset();
    if (MenuButton != nullptr &&
        MenuButton->GetVisibility() != ESlateVisibility::Collapsed)
    {
        OutButtons.Add(MenuButton);
    }
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
    int32 Index = View.Production.Cancellation.bVisible
        ? FindButtonIndex(
              Buttons,
              EEchoesFieldHudAction::ProductionCancelBack,
              0)
        : FindButtonIndex(Buttons, FocusedAction, FocusedArgument);
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
    return View.TutorialSkipModal.bVisible ||
        View.Technology.bVisible ||
        View.Production.Cancellation.bVisible ||
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
        if (View.bTutorialActive && !View.TutorialSkipModal.bVisible &&
            InKeyEvent.GetKey() == EKeys::SpaceBar && !InKeyEvent.IsRepeat())
        {
            bHoldToSkipSpacePressed = true;
            return FReply::Handled();
        }
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

FReply UEchoesFieldHudWidget::NativeOnKeyUp(
    const FGeometry& InGeometry,
    const FKeyEvent& InKeyEvent)
{
    if (InKeyEvent.GetKey() == EKeys::SpaceBar && bHoldToSkipSpacePressed)
    {
        bHoldToSkipSpacePressed = false;
        HoldToSkipCurrentSeconds = 0.0f;
        return FReply::Handled();
    }
    return Super::NativeOnKeyUp(InGeometry, InKeyEvent);
}

FVector2D UEchoesFieldHudWidget::ResolveConsolePixels() const
{
    const FGeometry Geometry = GetCachedGeometry();
    const FVector2D Local = Geometry.GetLocalSize();
    const float Dpi = FMath::Max(0.01f, UWidgetLayoutLibrary::GetViewportScale(this));
    // Before the first paint a Slate host reports no geometry. Falling back to
    // the engine viewport there is safe because nothing has been drawn yet; the
    // next tick replaces it with the surface actually on screen.
    if (Local.X < 1.0 || Local.Y < 1.0)
    {
        FVector2D Viewport = FVector2D::ZeroVector;
        if (const UWorld* World = GetWorld())
        {
            Viewport = UWidgetLayoutLibrary::GetViewportSize(const_cast<UWorld*>(World));
        }
        return Viewport.X >= 1.0 && Viewport.Y >= 1.0 ? Viewport : FVector2D(1280.0, 720.0);
    }
    return Local * Dpi;
}

FEchoesHudLayout UEchoesFieldHudWidget::ResolveConsoleLayout() const
{
    return FEchoesHudLayout::Build(
        ResolveConsolePixels(), View.HudScale, !View.Status.IsEmpty());
}

void UEchoesFieldHudWidget::ApplyConsoleLayout(const FVector2D& ViewportPixels)
{
    // Arrange from this frame's viewport size, not last frame's child geometry.
    // Convert the shared physical-pixel contract exactly once for UMG DPI.
    const float Dpi = FMath::Max(0.01f, UWidgetLayoutLibrary::GetViewportScale(this));
    const FEchoesHudLayout Layout = FEchoesHudLayout::Build(
        ViewportPixels, View.HudScale, !View.Status.IsEmpty());
    const bool bField = View.Surface == EEchoesFieldHudSurface::Battlefield ||
        View.Surface == EEchoesFieldHudSurface::Replay;
    const auto Place = [Dpi, bField](UWidget* Widget, const FBox2D& Rect, bool bVisible)
    {
        auto* Slot = Widget ? Cast<UCanvasPanelSlot>(Widget->Slot) : nullptr;
        if (!Slot) return;
        Widget->SetVisibility(bField && bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        Slot->SetAnchors(FAnchors(0, 0));
        Slot->SetPosition(Rect.Min / Dpi);
        Slot->SetSize(FVector2D(FMath::Max(0.0, Rect.GetSize().X), FMath::Max(0.0, Rect.GetSize().Y)) / Dpi);
    };
    if (ConsoleBacking)
    {
        ConsoleBacking->SetBrushColor(ConsoleBackingColor(View.bHighContrast));
        Place(ConsoleBacking, Layout.BottomBar, Layout.bBottomBarVisible);
    }
    Place(MenuButton, Layout.MenuPanel,
        Layout.bMenuVisible && View.Menu.bVisible);
    Place(GetSection(EEchoesFieldHudSection::ResourceLedger), Layout.ResourcePanel,
        Layout.bResourceVisible && View.Resources.bVisible);
    Place(GetSection(EEchoesFieldHudSection::Objectives), Layout.ObjectivePanel,
        Layout.bObjectiveVisible && (View.bObjectiveVisible || !View.Subtitle.IsEmpty() ||
            (View.bTutorialActive && !View.Status.IsEmpty())));
    Place(GetSection(EEchoesFieldHudSection::Selection), Layout.SelectionPanel,
        Layout.bSelectionVisible && View.Selection.bVisible);
    Place(GetSection(EEchoesFieldHudSection::CommandCard), Layout.CommandDeckPanel,
        Layout.bCommandDeckVisible && View.Commands.bVisible);
    Place(GetSection(EEchoesFieldHudSection::Status), Layout.StatusPanel,
        Layout.bStatusVisible && !View.bTutorialActive);
    Place(MinimapWidget, Layout.MinimapPanel, Layout.bMinimapVisible && View.Minimap.bVisible);

}

void UEchoesFieldHudWidget::NativeTick(
    const FGeometry& MyGeometry,
    float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    ApplyConsoleLayout(ResolveConsolePixels());

    if (View.bTutorialActive && !View.TutorialSkipModal.bVisible)
    {
        if (bHoldToSkipPointerPressed || bHoldToSkipSpacePressed)
        {
            HoldToSkipCurrentSeconds += InDeltaTime;
            if (HoldToSkipCurrentSeconds >= 1.5f)
            {
                HoldToSkipCurrentSeconds = 0.0f;
                bHoldToSkipPointerPressed = false;
                bHoldToSkipSpacePressed = false;
                // The hold is complete; the modal must receive the next click.
                if (HasMouseCapture() && FSlateApplication::IsInitialized())
                    FSlateApplication::Get().ReleaseAllPointerCapture();
                DispatchAction(EEchoesFieldHudAction::OpenTutorialSkipModal, 0);
            }
        }
        else
        {
            HoldToSkipCurrentSeconds = 0.0f;
        }
    }
    else
    {
        HoldToSkipCurrentSeconds = 0.0f;
        bHoldToSkipPointerPressed = false;
        bHoldToSkipSpacePressed = false;
    }
}

int32 UEchoesFieldHudWidget::NativePaint(
    const FPaintArgs& Args,
    const FGeometry& AllottedGeometry,
    const FSlateRect& MyCullingRect,
    FSlateWindowElementList& OutDrawElements,
    int32 LayerId,
    const FWidgetStyle& InWidgetStyle,
    bool bParentEnabled) const
{
    int32 MaxLayer = Super::NativePaint(
        Args, AllottedGeometry, MyCullingRect, OutDrawElements,
        LayerId, InWidgetStyle, bParentEnabled);

    // Project live, player-scoped geometry each paint so camera movement cannot
    // leave stale ranges on screen. Clip it above the console; it owns no input.
    if (APlayerController* Player = GetOwningPlayer();
        Player && (!View.NetworkCoverage.IsEmpty() || !View.OrderRoutes.IsEmpty()))
    {
        const FVector2D Size = AllottedGeometry.GetLocalSize();
        const float Scale = FMath::Max(0.01f, UWidgetLayoutLibrary::GetViewportScale(this));
        const FEchoesHudLayout Layout = FEchoesHudLayout::Build(Size * Scale, View.HudScale, !View.Status.IsEmpty());
        OutDrawElements.PushClip(FSlateClippingZone(AllottedGeometry.ToPaintGeometry(
            FVector2D(Size.X, Layout.BottomBar.Min.Y / Scale), FSlateLayoutTransform())));
        const auto Line = [&](const FVector& A, const FVector& B, const FLinearColor& Color)
        {
            FVector2D PA, PB;
            if (Player->ProjectWorldLocationToScreen(A, PA, true) &&
                Player->ProjectWorldLocationToScreen(B, PB, true))
            {
                TArray<FVector2D> Points{PA / Scale, PB / Scale};
                FSlateDrawElement::MakeLines(OutDrawElements, MaxLayer + 1,
                    AllottedGeometry.ToPaintGeometry(), Points, ESlateDrawEffect::None,
                    Color, true, 2.0f);
            }
        };
        for (const FEchoesNetworkConnectionView& Link : View.NetworkConnections)
            Line(Link.From, Link.To, FLinearColor(0.1f, 0.85f, 1.0f, 0.9f));
        for (const FEchoesNetworkCoverageView& Coverage : View.NetworkCoverage)
        {
            constexpr int32 Segments = 96;
            for (int32 I = 0; I < Segments; ++I)
            {
                // Broken white outline means potential range only; solid cyan is live.
                if (!Coverage.bOperational && I % 2) continue;
                const float A = 2.0f * PI * I / Segments;
                const float B = 2.0f * PI * (I + 1) / Segments;
                Line(Coverage.Center + FVector(FMath::Cos(A), FMath::Sin(A), 0) * Coverage.Radius,
                     Coverage.Center + FVector(FMath::Cos(B), FMath::Sin(B), 0) * Coverage.Radius,
                     Coverage.bOperational ? FLinearColor(0.1f, 0.85f, 1.0f, 0.95f) : FLinearColor(1, 1, 1, 0.8f));
            }
        }
        // The plan the player already gave: a segment per queued leg with its
        // number at the far end. Without this a multi-leg route existed only as
        // a waypoint count in text, so a three-leg march showed the digit 3 and
        // nothing on the ground. Drawn only while its owner is selected.
        if (!View.OrderRoutes.IsEmpty())
        {
            // Reduced flashing removes the emphasis contrast rather than the
            // route; the plan itself is information, not decoration.
            const FLinearColor MarchColor = View.bHighContrast
                ? FLinearColor(1.0f, 0.95f, 0.2f, 0.98f)
                : FLinearColor(0.35f, 0.95f, 0.7f, 0.92f);
            const FLinearColor RallyColor = View.bHighContrast
                ? FLinearColor(1.0f, 0.6f, 0.1f, 0.98f)
                : FLinearColor(0.98f, 0.7f, 0.2f, 0.92f);
            const FSlateFontInfo OrdinalFont = BrandedFont(true, 11);
            for (const FEchoesFieldHudRouteLeg& Leg : View.OrderRoutes)
            {
                const FLinearColor LegColor = Leg.bRally ? RallyColor : MarchColor;
                Line(Leg.From, Leg.To, LegColor);
                FVector2D Pip;
                if (!Player->ProjectWorldLocationToScreen(Leg.To, Pip, true))
                {
                    continue;
                }
                const FVector2D PipLocal = Pip / Scale;
                constexpr float PipSize = 10.0f;
                DrawBox(OutDrawElements, MaxLayer + 2, AllottedGeometry,
                    PipLocal - FVector2D(PipSize * 0.5f, PipSize * 0.5f),
                    FVector2D(PipSize, PipSize), LegColor);
                FSlateDrawElement::MakeText(
                    OutDrawElements,
                    MaxLayer + 3,
                    AllottedGeometry.ToPaintGeometry(
                        FVector2D(24.0f, 16.0f),
                        FSlateLayoutTransform(
                            PipLocal + FVector2D(PipSize * 0.6f, -PipSize))),
                    FString::FromInt(Leg.Ordinal),
                    OrdinalFont,
                    ESlateDrawEffect::None,
                    LegColor);
            }
        }
        OutDrawElements.PopClip();
        ++MaxLayer;
    }

    if (!View.bTutorialActive)
    {
        return MaxLayer;
    }

    const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
    if (LocalSize.X <= 0.0f || LocalSize.Y <= 0.0f)
    {
        return MaxLayer;
    }

    // 1. Spotlight & UI Dimming (SPEC-TUT-005)
    if (View.TutorialSpotlight.bActive && !View.TutorialSkipModal.bVisible)
    {
        const float ViewportScale = FMath::Max(0.01f, UWidgetLayoutLibrary::GetViewportScale(this));
        const FVector2D LocalCenter = View.TutorialSpotlight.ScreenCenter / ViewportScale;
        const FVector2D HalfSize = (View.TutorialSpotlight.ScreenSize * 0.5f) / ViewportScale;

        const float MinX = FMath::Clamp(LocalCenter.X - HalfSize.X, 0.0f, LocalSize.X);
        const float MaxX = FMath::Clamp(LocalCenter.X + HalfSize.X, 0.0f, LocalSize.X);
        const float MinY = FMath::Clamp(LocalCenter.Y - HalfSize.Y, 0.0f, LocalSize.Y);
        const float MaxY = FMath::Clamp(LocalCenter.Y + HalfSize.Y, 0.0f, LocalSize.Y);

        const FLinearColor DimColor = View.bHighContrast
            ? FLinearColor(0.0f, 0.0f, 0.0f, 0.75f)
            : FLinearColor(0.0f, 0.0f, 0.0f, 0.58f);

        // Dim 4 boxes around the cutout hole
        if (MinY > 0.0f)
        {
            DrawBox(OutDrawElements, MaxLayer + 1, AllottedGeometry,
                FVector2D(0.0f, 0.0f), FVector2D(LocalSize.X, MinY), DimColor);
        }
        if (MaxY < LocalSize.Y)
        {
            DrawBox(OutDrawElements, MaxLayer + 1, AllottedGeometry,
                FVector2D(0.0f, MaxY), FVector2D(LocalSize.X, LocalSize.Y - MaxY), DimColor);
        }
        if (MinX > 0.0f && MaxY > MinY)
        {
            DrawBox(OutDrawElements, MaxLayer + 1, AllottedGeometry,
                FVector2D(0.0f, MinY), FVector2D(MinX, MaxY - MinY), DimColor);
        }
        if (MaxX < LocalSize.X && MaxY > MinY)
        {
            DrawBox(OutDrawElements, MaxLayer + 1, AllottedGeometry,
                FVector2D(MaxX, MinY), FVector2D(LocalSize.X - MaxX, MaxY - MinY), DimColor);
        }

        // Spotlight highlight border
        const FLinearColor SpotlightColor = View.bHighContrast
            ? FLinearColor(1.0f, 0.85f, 0.1f, 1.0f)
            : FLinearColor(0.96f, 0.68f, 0.18f, 0.95f);

        DrawLine(OutDrawElements, MaxLayer + 2, AllottedGeometry,
            { FVector2D(MinX, MinY), FVector2D(MaxX, MinY),
              FVector2D(MaxX, MaxY), FVector2D(MinX, MaxY) },
            SpotlightColor, 2.0f, true);

        // Target name label
        if (!View.TutorialSpotlight.TargetName.IsEmpty())
        {
            const FSlateFontInfo NameFont = BrandedFont(false, 12);
            const FVector2D LabelPos(MinX, FMath::Max(4.0f, MinY - 22.0f));
            FSlateDrawElement::MakeText(
                OutDrawElements,
                MaxLayer + 3,
                AllottedGeometry.ToPaintGeometry(
                    FVector2D(250.0f, 22.0f),
                    FSlateLayoutTransform(LabelPos)),
                View.TutorialSpotlight.TargetName.ToString(),
                NameFont,
                ESlateDrawEffect::None,
                SpotlightColor);
        }

        // Action prompt badge directly beneath spotlight (SPEC-TUT-005)
        if (!View.TutorialSpotlight.ActionPrompt.IsEmpty())
        {
            const FSlateFontInfo ActionFont = BrandedFont(false, 10);
            const FString ActionText = View.TutorialSpotlight.ActionPrompt.ToString();
            const float BadgeWidth = FMath::Clamp(static_cast<float>(ActionText.Len()) * 7.5f + 20.0f, 160.0f, 340.0f);
            const float BadgeHeight = 24.0f;
            const FVector2D BadgePos(LocalCenter.X - BadgeWidth * 0.5f, FMath::Min(LocalSize.Y - 28.0f, MaxY + 6.0f));

            DrawBox(OutDrawElements, MaxLayer + 3, AllottedGeometry,
                BadgePos, FVector2D(BadgeWidth, BadgeHeight),
                FLinearColor(0.02f, 0.04f, 0.08f, 0.92f));

            DrawLine(OutDrawElements, MaxLayer + 4, AllottedGeometry,
                { BadgePos, BadgePos + FVector2D(BadgeWidth, 0.0f),
                  BadgePos + FVector2D(BadgeWidth, BadgeHeight),
                  BadgePos + FVector2D(0.0f, BadgeHeight) },
                SpotlightColor, 1.2f, true);

            FSlateDrawElement::MakeText(
                OutDrawElements,
                MaxLayer + 5,
                AllottedGeometry.ToPaintGeometry(
                    FVector2D(BadgeWidth - 8.0f, BadgeHeight),
                    FSlateLayoutTransform(BadgePos + FVector2D(8.0f, 4.0f))),
                ActionText,
                ActionFont,
                ESlateDrawEffect::None,
                View.bHighContrast ? FLinearColor::White : FLinearColor(0.98f, 0.94f, 0.85f, 1.0f));
        }

        // Ghost indicator pointing towards spotlight center (SPEC-TUT-005)
        const UWorld* World = GetWorld();
        const float TimeSeconds = World != nullptr ? World->GetTimeSeconds() : 0.0f;
        const float Oscillation = View.bReducedMotion
            ? 0.0f
            : FMath::Sin(TimeSeconds * 4.0f) * 6.0f;

        const FVector2D IndicatorTip(LocalCenter.X, FMath::Max(14.0f, MinY - 6.0f + Oscillation));
        const TArray<FVector2D> ArrowPoints = {
            IndicatorTip + FVector2D(-10.0f, -12.0f),
            IndicatorTip,
            IndicatorTip + FVector2D(10.0f, -12.0f)
        };
        DrawLine(OutDrawElements, MaxLayer + 3, AllottedGeometry,
            ArrowPoints, SpotlightColor, 2.5f, false);

        // Animated input indicator (click pulse, static when reduced motion)
        const float PulseTime = View.bReducedMotion ? 0.0f : FMath::Fmod(TimeSeconds * 2.0f, 1.0f);
        const float ClickRadius = View.bReducedMotion ? 18.0f : 12.0f + PulseTime * 14.0f;
        const float ClickAlpha = View.bReducedMotion ? 0.8f : (1.0f - PulseTime) * 0.9f;
        TArray<FVector2D> ClickRing;
        for (int32 i = 0; i <= 16; ++i)
        {
            const float Angle = static_cast<float>(i) / 16.0f * 2.0f * PI;
            ClickRing.Add(LocalCenter + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * ClickRadius);
        }
        DrawLine(OutDrawElements, MaxLayer + 3, AllottedGeometry,
            ClickRing, SpotlightColor.CopyWithNewOpacity(ClickAlpha), 1.8f, true);
    }

    // 2. Top-right low-emphasis "Hold to skip" control with circular meter (SPEC-TUT-006)
    if (!View.TutorialSkipModal.bVisible)
    {
        const float PanelWidth = 190.0f;
        const float PanelHeight = 34.0f;
        const FVector2D SkipPos(LocalSize.X - PanelWidth - 20.0f, 16.0f);
        const FVector2D SkipDim(PanelWidth, PanelHeight);

        // Panel background
        const FLinearColor PanelBg = View.bHighContrast
            ? FLinearColor(0.0f, 0.0f, 0.0f, 0.85f)
            : FLinearColor(0.02f, 0.03f, 0.05f, 0.70f);
        DrawBox(OutDrawElements, MaxLayer + 2, AllottedGeometry,
            SkipPos, SkipDim, PanelBg);

        // Border
        const FLinearColor BorderCol = View.bHighContrast
            ? FLinearColor(0.6f, 0.6f, 0.6f, 0.8f)
            : FLinearColor(0.25f, 0.35f, 0.40f, 0.55f);
        DrawLine(OutDrawElements, MaxLayer + 3, AllottedGeometry,
            { SkipPos, SkipPos + FVector2D(PanelWidth, 0.0f),
              SkipPos + FVector2D(PanelWidth, PanelHeight),
              SkipPos + FVector2D(0.0f, PanelHeight) },
            BorderCol, 1.2f, true);

        // Text
        const FSlateFontInfo SkipFont = BrandedFont(false, 12);
        const FLinearColor TextCol = View.bHighContrast
            ? FLinearColor::White
            : FLinearColor(0.72f, 0.78f, 0.82f, 0.90f);
        FSlateDrawElement::MakeText(
            OutDrawElements,
            MaxLayer + 4,
            AllottedGeometry.ToPaintGeometry(
                FVector2D(135.0f, 20.0f),
                FSlateLayoutTransform(SkipPos + FVector2D(10.0f, 9.0f))),
            NSLOCTEXT("EchoesFieldHud", "HoldSpaceToSkip", "HOLD SPACE TO SKIP").ToString(),
            SkipFont,
            ESlateDrawEffect::None,
            TextCol);

        // Circular progress meter
        const FVector2D MeterCenter = SkipPos + FVector2D(PanelWidth - 20.0f, PanelHeight * 0.5f);
        const float MeterRadius = 8.5f;

        // Background meter circle (16 segments)
        TArray<FVector2D> BgCircle;
        constexpr int32 CircleSegments = 16;
        for (int32 i = 0; i <= CircleSegments; ++i)
        {
            const float Angle = (static_cast<float>(i) / CircleSegments) * 2.0f * PI;
            BgCircle.Add(MeterCenter + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * MeterRadius);
        }
        DrawLine(OutDrawElements, MaxLayer + 3, AllottedGeometry,
            BgCircle, FLinearColor(0.25f, 0.30f, 0.35f, 0.5f), 1.2f, true);

        // Active hold arc
        const float HoldProgress = FMath::Clamp(HoldToSkipCurrentSeconds / 1.5f, 0.0f, 1.0f);
        if (HoldProgress > 0.0f)
        {
            TArray<FVector2D> ArcPoints;
            const int32 ArcSegments = FMath::Max(2, FMath::CeilToInt(HoldProgress * 20.0f));
            for (int32 i = 0; i <= ArcSegments; ++i)
            {
                const float Frac = static_cast<float>(i) / ArcSegments;
                const float Angle = -PI * 0.5f + Frac * HoldProgress * 2.0f * PI;
                ArcPoints.Add(MeterCenter + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * MeterRadius);
            }
            const FLinearColor MeterColor = View.bHighContrast
                ? FLinearColor(1.0f, 0.85f, 0.1f, 1.0f)
                : FLinearColor(0.96f, 0.68f, 0.18f, 1.0f);
            DrawLine(OutDrawElements, MaxLayer + 4, AllottedGeometry,
                ArcPoints, MeterColor, 2.2f, false);
        }
    }

    // 3. Top-Center Tutorial Instruction Banner (SPEC-TUT-005)
    if (!View.TutorialSkipModal.bVisible && !View.TutorialInstruction.IsEmpty())
    {
        const FString InstructionStr = View.TutorialInstruction.ToString();
        const FString TitleStr = !View.TutorialLessonTitle.IsEmpty()
            ? View.TutorialLessonTitle.ToString()
            : TEXT("TUTORIAL OBJECTIVE");

        const float BannerWidth = FMath::Clamp(LocalSize.X * 0.52f, 460.0f, 680.0f);
        const float BannerHeight = 52.0f;
        const float Dpi = FMath::Max(0.01f, UWidgetLayoutLibrary::GetViewportScale(this));
        const FEchoesHudLayout Layout = FEchoesHudLayout::Build(LocalSize * Dpi, View.HudScale, true);
        const FVector2D BannerPos((LocalSize.X - BannerWidth) * 0.5f, Layout.StatusPanel.Min.Y / Dpi);

        // Dark high-contrast background
        const FLinearColor BannerBg = View.bHighContrast
            ? FLinearColor(0.0f, 0.0f, 0.0f, 0.94f)
            : FLinearColor(0.02f, 0.04f, 0.08f, 0.90f);
        DrawBox(OutDrawElements, MaxLayer + 3, AllottedGeometry,
            BannerPos, FVector2D(BannerWidth, BannerHeight), BannerBg);

        // Amber accent border
        const FLinearColor BannerBorder = View.bHighContrast
            ? FLinearColor(1.0f, 0.85f, 0.1f, 1.0f)
            : FLinearColor(0.96f, 0.68f, 0.18f, 0.95f);
        DrawLine(OutDrawElements, MaxLayer + 4, AllottedGeometry,
            { BannerPos, BannerPos + FVector2D(BannerWidth, 0.0f),
              BannerPos + FVector2D(BannerWidth, BannerHeight),
              BannerPos + FVector2D(0.0f, BannerHeight) },
            BannerBorder, 1.5f, true);

        // Header: Lesson title
        const FSlateFontInfo HeaderFont = BrandedFont(false, 10);
        FSlateDrawElement::MakeText(
            OutDrawElements,
            MaxLayer + 5,
            AllottedGeometry.ToPaintGeometry(
                FVector2D(BannerWidth - 24.0f, 16.0f),
                FSlateLayoutTransform(BannerPos + FVector2D(12.0f, 6.0f))),
            TitleStr,
            HeaderFont,
            ESlateDrawEffect::None,
            BannerBorder);

        // Body: Active instruction
        const FSlateFontInfo BodyFont = BrandedFont(false, 11);
        const FLinearColor BodyColor = View.bHighContrast
            ? FLinearColor::White
            : FLinearColor(0.95f, 0.97f, 1.0f, 1.0f);
        FSlateDrawElement::MakeText(
            OutDrawElements,
            MaxLayer + 5,
            AllottedGeometry.ToPaintGeometry(
                FVector2D(BannerWidth - 24.0f, 24.0f),
                FSlateLayoutTransform(BannerPos + FVector2D(12.0f, 24.0f))),
            InstructionStr,
            BodyFont,
            ESlateDrawEffect::None,
            BodyColor);
    }

    return MaxLayer + 6;
}

FReply UEchoesFieldHudWidget::NativeOnMouseButtonDown(
    const FGeometry& InGeometry,
    const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton &&
        View.bTutorialActive && !View.TutorialSkipModal.bVisible)
    {
        const FVector2D LocalPos = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
        const float PanelWidth = 190.0f;
        const float PanelHeight = 34.0f;
        const FVector2D LocalSize = InGeometry.GetLocalSize();
        const FVector2D SkipPos(LocalSize.X - PanelWidth - 20.0f, 16.0f);
        const FBox2D SkipBox(SkipPos, SkipPos + FVector2D(PanelWidth, PanelHeight));
        if (SkipBox.IsInside(LocalPos))
        {
            bHoldToSkipPointerPressed = true;
            return FReply::Handled().CaptureMouse(TakeWidget());
        }
    }
    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UEchoesFieldHudWidget::NativeOnMouseButtonUp(
    const FGeometry& InGeometry,
    const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bHoldToSkipPointerPressed)
    {
        bHoldToSkipPointerPressed = false;
        HoldToSkipCurrentSeconds = 0.0f;
        return FReply::Handled().ReleaseMouseCapture();
    }
    return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UEchoesFieldHudWidget::NativeOnMouseCaptureLost(
    const FCaptureLostEvent& CaptureLostEvent)
{
    Super::NativeOnMouseCaptureLost(CaptureLostEvent);
    bHoldToSkipPointerPressed = false;
    HoldToSkipCurrentSeconds = 0.0f;
}

void UEchoesFieldHudWidget::NativeOnFocusLost(
    const FFocusEvent& InFocusEvent)
{
    Super::NativeOnFocusLost(InFocusEvent);
    bHoldToSkipPointerPressed = false;
    bHoldToSkipSpacePressed = false;
    HoldToSkipCurrentSeconds = 0.0f;
}
