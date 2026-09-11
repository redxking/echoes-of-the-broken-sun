#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "EchoesFieldHudView.h"
#include "EchoesFieldHudWidget.generated.h"

class AEchoesPlayerController;
class UBorder;
class UCanvasPanel;
class UScrollBox;
class UTextBlock;
class UUniformGridPanel;
class UVerticalBox;
class UProgressBar;
struct FAnchors;

enum class EEchoesFieldHudSection : uint8
{
    ResourceLedger,
    Selection,
    CommandCard,
    Objectives,
    Status,
    Subtitle,
    Technology,
    CampaignInspector,
    OnlineFrontDoor,
    OnlineLocalMenu,
    Reconnect,
    TutorialModal
};

UCLASS(NotBlueprintable)
class ECHOESOFTHEBROKENSUN_API UEchoesFieldHudActionButton final
    : public UButton
{
    GENERATED_BODY()

public:
    UEchoesFieldHudActionButton(const FObjectInitializer& ObjectInitializer);
    void Configure(
        class UEchoesFieldHudWidget* InOwner,
        const FEchoesFieldHudControl& InControl,
        bool bInHighContrast,
        float InScale);
    void SetPresentationLabel(UTextBlock* Label) { PresentationLabel = Label; }
    UTextBlock* GetPresentationLabel() const { return PresentationLabel; }
    /** Command-card tiles carry the binding in a small corner readout instead
     * of a second label line, so a hotkey never changes the tile height. */
    void SetPresentationHotkey(UTextBlock* Label) { HotkeyLabel = Label; }
    UTextBlock* GetPresentationHotkey() const { return HotkeyLabel; }
    /** Console controls show their label only and carry the detail in the
     * tooltip, so a queue's four controls fit one non-scrolling row. */
    void SetCompactPresentation(bool bInCompact) { bCompactPresentation = bInCompact; }
    bool Activate();
    [[nodiscard]] EEchoesFieldHudAction GetAction() const { return Action; }
    [[nodiscard]] int32 GetArgument() const { return Argument; }
    [[nodiscard]] bool IsPreferredFocus() const
    {
        return bFocusedPresentation;
    }

private:
    UFUNCTION() void HandleClicked();
    UFUNCTION() void HandleHovered();
    UFUNCTION() void HandleUnhovered();
    void HandleReceivedFocus();
    void HandleLostFocus();
    void RefreshKeyboardPresentation();

    TWeakObjectPtr<class UEchoesFieldHudWidget> Owner;
    EEchoesFieldHudAction Action = EEchoesFieldHudAction::None;
    int32 Argument = 0;
    bool bFocusedPresentation = false;
    bool bHighContrast = false;
    bool bPointerHovered = false;
    bool bKeyboardFocused = false;
    bool bCompactPresentation = false;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> PresentationLabel;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> HotkeyLabel;
};

UCLASS(NotBlueprintable)
class ECHOESOFTHEBROKENSUN_API UEchoesFieldHudEndpointBox final
    : public UEditableTextBox
{
    GENERATED_BODY()

public:
    void Configure(class UEchoesFieldHudWidget* InOwner, const FText& Endpoint);

private:
    UFUNCTION() void HandleTextChanged(const FText& NewText);
    UFUNCTION() void HandleCommitted(
        const FText& NewText,
        ETextCommit::Type CommitMethod);
    TWeakObjectPtr<class UEchoesFieldHudWidget> Owner;
    FText LastAppliedAuthoritativeEndpoint;
    bool bHasAppliedAuthoritativeEndpoint = false;
    bool bTextDirty = false;
};

/** A stable, independently refreshable UMG field-HUD panel. */
UCLASS(NotBlueprintable)
class ECHOESOFTHEBROKENSUN_API UEchoesFieldHudSectionWidget final
    : public UUserWidget
{
    GENERATED_BODY()

public:
    void Configure(
        class UEchoesFieldHudWidget* InOwner,
        EEchoesFieldHudSection InSection);
    void SetContent(
        const FText& InTitle,
        const TArray<FText>& InLines,
        const TArray<FEchoesFieldHudControl>& InControls,
        bool bInHighContrast,
        float InScale,
        bool bShowEndpoint = false,
        const FText& Endpoint = FText::GetEmpty());
    void SetResourceTelemetry(const FEchoesFieldHudResourceView& Resources);
    int32 GetResourceReadoutCount() const { return ResourceValues.Num(); }
    UTextBlock* GetResourceReadout(int32 Index) const { return ResourceValues.IsValidIndex(Index) ? ResourceValues[Index].Get() : nullptr; }
    UTextBlock* GetResourceLabel(int32 Index) const { return ResourceLabels.IsValidIndex(Index) ? ResourceLabels[Index].Get() : nullptr; }
    /** Visible identity line; values come only from the scoped resource view. */
    UTextBlock* GetResourceIdentityReadout() const { return ResourceIdentityText; }
    /** Visible match/research line; empty source fields remain absent. */
    UTextBlock* GetResourceContextReadout() const { return ResourceSummaryText; }
    /** Full resource telemetry hit target when the semantic monitor action is available. */
    UEchoesFieldHudActionButton* GetResourceActionButton() const { return ResourceActionButton; }
    void SetSelectionTelemetry(const FEchoesFieldHudSelectionView& Selection);
    /** A production cancellation review replaces the entity telemetry: the
     * refund facts and Back/Confirm must fit the card without scrolling. */
    void SetTelemetrySuppressed(bool bSuppressed);
    int32 GetHealthReadoutCount() const { return HealthBars.Num(); }
    UProgressBar* GetHealthReadout(int32 Index) const { return HealthBars.IsValidIndex(Index) ? HealthBars[Index].Get() : nullptr; }
    [[nodiscard]] EEchoesFieldHudSection GetSection() const { return Section; }
    [[nodiscard]] int32 GetActionButtonCount() const { return ActionButtons.Num(); }
    [[nodiscard]] UEchoesFieldHudActionButton* GetActionButton(int32 Index) const
    {
        return ActionButtons.IsValidIndex(Index) ? ActionButtons[Index] : nullptr;
    }
    [[nodiscard]] UEchoesFieldHudEndpointBox* GetEndpointBox() const
    {
        return EndpointBox;
    }
    [[nodiscard]] bool IsEndpointEditing() const
    {
        return EndpointBox != nullptr && EndpointBox->HasKeyboardFocus();
    }
    [[nodiscard]] bool UsesScrollableContent() const
    {
        return ContentScroll != nullptr;
    }
    bool ScrollActionIntoView(UEchoesFieldHudActionButton* Button);
    bool FocusEndpointEditor();

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual int32 NativePaint(const FPaintArgs&, const FGeometry&, const FSlateRect&,
        FSlateWindowElementList&, int32, const FWidgetStyle&, bool) const override;
    virtual FReply NativeOnMouseButtonDown(
        const FGeometry& InGeometry,
        const FPointerEvent& InMouseEvent) override;

private:
    void RebuildContent();
    void RefreshSectionTooltip();
    bool CanRefreshInPlace(
        const TArray<FText>& InLines,
        const TArray<FEchoesFieldHudControl>& InControls,
        bool bShowEndpoint) const;

    TWeakObjectPtr<class UEchoesFieldHudWidget> Owner;
    EEchoesFieldHudSection Section = EEchoesFieldHudSection::Status;
    FText Title;
    TArray<FText> Lines;
    TArray<FEchoesFieldHudControl> Controls;
    bool bHighContrast = false;
    float Scale = 1.0f;
    bool bHasEndpoint = false;
    bool bTelemetrySuppressed = false;
    FText EndpointText;
    FEchoesFieldHudResourceView ResourceTelemetry;
    UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> ResourceLabels;
    UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> ResourceValues;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ResourceIdentityText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ResourceSummaryText;
    TArray<FEchoesFieldHudSelectionEntry> SelectionEntries;
    UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> TelemetryLabels;
    UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> TelemetryDetails;
    UPROPERTY(Transient) TArray<TObjectPtr<UProgressBar>> HealthBars;

    UPROPERTY(Transient) TObjectPtr<UBorder> RootBorder;
    UPROPERTY(Transient) TObjectPtr<UScrollBox> ContentScroll;
    UPROPERTY(Transient) TObjectPtr<UVerticalBox> ContentBox;
    /** ResourceLedger only: whole compact telemetry region is a real action button. */
    UPROPERTY(Transient) TObjectPtr<UEchoesFieldHudActionButton> ResourceActionButton;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> TitleText;
    UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> LineTexts;
    UPROPERTY(Transient) TArray<TObjectPtr<UEchoesFieldHudActionButton>> ActionButtons;
    UPROPERTY(Transient) TObjectPtr<UEchoesFieldHudEndpointBox> EndpointBox;
};

/** Fair-information minimap geometry and its normalized pointer seam. */
UCLASS(NotBlueprintable)
class ECHOESOFTHEBROKENSUN_API UEchoesFieldHudMinimapWidget final
    : public UUserWidget
{
    GENERATED_BODY()

public:
    void Configure(class UEchoesFieldHudWidget* InOwner);
    void SetView(
        const FEchoesFieldHudMinimapView& InView,
        bool bInHighContrast,
        bool bInReducedFlashing = false);

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual int32 NativePaint(
        const FPaintArgs& Args,
        const FGeometry& AllottedGeometry,
        const FSlateRect& MyCullingRect,
        FSlateWindowElementList& OutDrawElements,
        int32 LayerId,
        const FWidgetStyle& InWidgetStyle,
        bool bParentEnabled) const override;
    virtual FReply NativeOnMouseButtonDown(
        const FGeometry& InGeometry,
        const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseMove(
        const FGeometry& InGeometry,
        const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseButtonUp(
        const FGeometry& InGeometry,
        const FPointerEvent& InMouseEvent) override;
    virtual void NativeOnMouseCaptureLost(
        const FCaptureLostEvent& CaptureLostEvent) override;

private:
    void RefreshMissionLabels();
    bool DispatchPointer(
        const FGeometry& Geometry,
        const FPointerEvent& Event,
        bool bIssueOrder) const;
    TWeakObjectPtr<class UEchoesFieldHudWidget> Owner;
    FEchoesFieldHudMinimapView View;
    bool bHighContrast = false;
    bool bReducedFlashing = false;
    bool bDragging = false;
    UPROPERTY(Transient) TObjectPtr<UCanvasPanel> RootCanvas;
    UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> MissionLabels;
};

/** Campaign topology geometry; text and actions remain ordinary child widgets. */
UCLASS(NotBlueprintable)
class ECHOESOFTHEBROKENSUN_API UEchoesFieldHudCampaignMapWidget final
    : public UUserWidget
{
    GENERATED_BODY()

public:
    void Configure(class UEchoesFieldHudWidget* InOwner);
    void SetView(
        const FEchoesFieldHudCampaignView& InView,
        bool bInHighContrast,
        float InScale);
    [[nodiscard]] int32 GetNodeButtonCount() const { return NodeButtons.Num(); }
    [[nodiscard]] UEchoesFieldHudActionButton* GetNodeButton(int32 Index) const
    {
        return NodeButtons.IsValidIndex(Index) ? NodeButtons[Index] : nullptr;
    }

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual int32 NativePaint(
        const FPaintArgs& Args,
        const FGeometry& AllottedGeometry,
        const FSlateRect& MyCullingRect,
        FSlateWindowElementList& OutDrawElements,
        int32 LayerId,
        const FWidgetStyle& InWidgetStyle,
        bool bParentEnabled) const override;
    virtual FReply NativeOnMouseButtonDown(
        const FGeometry& InGeometry,
        const FPointerEvent& InMouseEvent) override;

private:
    void RebuildNodes();
    TWeakObjectPtr<class UEchoesFieldHudWidget> Owner;
    FEchoesFieldHudCampaignView View;
    TArray<FEchoesFieldHudControl> NodeControls;
    bool bHighContrast = false;
    float Scale = 1.0f;
    UPROPERTY(Transient) TObjectPtr<UCanvasPanel> RootCanvas;
    UPROPERTY(Transient) TArray<TObjectPtr<UEchoesFieldHudActionButton>> NodeButtons;
};

/** Selection marquee and keyboard targeting reticle only. */
UCLASS(NotBlueprintable)
class ECHOESOFTHEBROKENSUN_API UEchoesFieldHudTargetingWidget final
    : public UUserWidget
{
    GENERATED_BODY()

public:
    void SetView(
        const FEchoesFieldHudTargetingView& InView,
        bool bInHighContrast);

protected:
    virtual int32 NativePaint(
        const FPaintArgs& Args,
        const FGeometry& AllottedGeometry,
        const FSlateRect& MyCullingRect,
        FSlateWindowElementList& OutDrawElements,
        int32 LayerId,
        const FWidgetStyle& InWidgetStyle,
        bool bParentEnabled) const override;

private:
    FEchoesFieldHudTargetingView View;
    bool bHighContrast = false;
};

/** Screen-space anonymous contact label; it contains no source entity id. */
UCLASS(NotBlueprintable)
class ECHOESOFTHEBROKENSUN_API UEchoesFieldHudContactWidget final
    : public UUserWidget
{
    GENERATED_BODY()

public:
    void SetContact(
        const FEchoesFieldHudContact& InContact,
        bool bInHighContrast,
        float InScale);

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;

private:
    void RebuildContent();
    FEchoesFieldHudContact Contact;
    bool bHighContrast = false;
    float Scale = 1.0f;
    UPROPERTY(Transient) TObjectPtr<UBorder> RootBorder;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ContactText;
};

UCLASS(NotBlueprintable)
class ECHOESOFTHEBROKENSUN_API UEchoesFieldHudWidget final
    : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Shared pixel-space layout; also usable before first paint in isolated Slate hosts. */
    void ApplyConsoleLayout(const FVector2D& ViewportPixels);
    /** The one pixel space this widget draws, places and hit-tests in.
     *
     * Every console consumer must resolve through here. Mixing this with an
     * engine-reported viewport size is the recorded mismatch that clipped the
     * console away while the window was borderless at a different resolution. */
    [[nodiscard]] FVector2D ResolveConsolePixels() const;
    /** The confined layout for this frame, in the space above. */
    [[nodiscard]] struct FEchoesHudLayout ResolveConsoleLayout() const;
    /** View.HudScale grown by this surface's DPI (FEchoesHudLayout::EffectiveScale). */
    [[nodiscard]] float EffectiveHudScale() const;
    UEchoesFieldHudWidget(const FObjectInitializer& ObjectInitializer);
    void Configure(AEchoesPlayerController* InController);
    void SetView(const FEchoesFieldHudView& InView);
    void DispatchAction(EEchoesFieldHudAction Action, int32 Argument);
    bool DispatchMinimapPointer(
        const FVector2D& NormalizedMapPosition,
        bool bIssueOrder);
    void DispatchEndpoint(const FString& Endpoint);
    void NotifyButtonFocused(UEchoesFieldHudActionButton* Button);
    /** Focuses the retained semantic action, or a safe enabled modal default. */
    bool FocusDefaultAction();
    [[nodiscard]] bool HandlePointerAtViewportPosition(
        const FVector2D& ScreenPosition,
        bool bIssueOrder);
    [[nodiscard]] bool IsPointerOverChrome(
        const FVector2D& ScreenPosition) const;
    [[nodiscard]] bool IsPointerOverMinimap(
        const FVector2D& ScreenPosition) const;

    [[nodiscard]] int32 GetSectionCount() const { return Sections.Num(); }
    [[nodiscard]] int32 GetActionButtonCount() const;
    [[nodiscard]] EEchoesFieldHudAction GetFocusedAction() const
    {
        return FocusedAction;
    }
    [[nodiscard]] int32 GetFocusedArgument() const
    {
        return FocusedArgument;
    }
    [[nodiscard]] UEchoesFieldHudSectionWidget* GetSection(
        EEchoesFieldHudSection Section) const;
    [[nodiscard]] UEchoesFieldHudMinimapWidget* GetMinimapWidget() const
    {
        return MinimapWidget;
    }
    [[nodiscard]] UEchoesFieldHudActionButton* GetMenuButton() const
    {
        return MenuButton;
    }
    [[nodiscard]] UEchoesFieldHudCampaignMapWidget* GetCampaignMapWidget() const
    {
        return CampaignMapWidget;
    }
    [[nodiscard]] UEchoesFieldHudTargetingWidget* GetTargetingWidget() const
    {
        return TargetingWidget;
    }
    [[nodiscard]] int32 GetContactWidgetCount() const
    {
        return ContactWidgets.Num();
    }

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual int32 NativePaint(
        const FPaintArgs& Args,
        const FGeometry& AllottedGeometry,
        const FSlateRect& MyCullingRect,
        FSlateWindowElementList& OutDrawElements,
        int32 LayerId,
        const FWidgetStyle& InWidgetStyle,
        bool bParentEnabled) const override;
    virtual FReply NativeOnPreviewKeyDown(
        const FGeometry& InGeometry,
        const FKeyEvent& InKeyEvent) override;
    virtual FReply NativeOnKeyUp(
        const FGeometry& InGeometry,
        const FKeyEvent& InKeyEvent) override;
    virtual FReply NativeOnMouseButtonDown(
        const FGeometry& InGeometry,
        const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseButtonUp(
        const FGeometry& InGeometry,
        const FPointerEvent& InMouseEvent) override;
    virtual void NativeOnMouseCaptureLost(
        const FCaptureLostEvent& CaptureLostEvent) override;
    virtual void NativeOnFocusLost(
        const FFocusEvent& InFocusEvent) override;

private:
    void BuildStableTree();
    void ApplyView();
    void AddSection(
        EEchoesFieldHudSection Section,
        const FAnchors& Anchors,
        const FMargin& Offsets);
    void RefreshContactWidgets();
    bool FocusNext(bool bReverse);
    bool FocusButtonAtIndex(
        int32 Index,
        const TArray<UEchoesFieldHudActionButton*>& Buttons);
    int32 FindDefaultButtonIndex(
        const TArray<UEchoesFieldHudActionButton*>& Buttons) const;
    int32 FindButtonIndex(
        const TArray<UEchoesFieldHudActionButton*>& Buttons,
        EEchoesFieldHudAction Action,
        int32 Argument) const;
    bool ActivateFocused();
    bool IsModalSurface() const;
    void GatherActionButtons(
        TArray<UEchoesFieldHudActionButton*>& OutButtons) const;
    AEchoesPlayerController* ResolveController() const;

    FEchoesFieldHudView View;
    bool bHasView = false;
    int32 FocusedButtonIndex = INDEX_NONE;
    EEchoesFieldHudAction FocusedAction = EEchoesFieldHudAction::None;
    int32 FocusedArgument = 0;
    TWeakObjectPtr<AEchoesPlayerController> Controller;
    mutable float HoldToSkipCurrentSeconds = 0.0f;
    mutable bool bHoldToSkipPointerPressed = false;
    mutable bool bHoldToSkipSpacePressed = false;

    // Keep painted skip chrome reachable without intercepting the battlefield.
    UPROPERTY(Transient) TObjectPtr<UEchoesFieldHudActionButton> MenuButton;
    UPROPERTY(Transient) TObjectPtr<UBorder> TutorialSkipHitTarget;
    UPROPERTY(Transient) TObjectPtr<UBorder> ConsoleBacking;
    UPROPERTY(Transient) TObjectPtr<UCanvasPanel> RootCanvas;
    UPROPERTY(Transient) TArray<TObjectPtr<UEchoesFieldHudSectionWidget>> Sections;
    UPROPERTY(Transient) TObjectPtr<UEchoesFieldHudMinimapWidget> MinimapWidget;
    UPROPERTY(Transient) TObjectPtr<UEchoesFieldHudCampaignMapWidget> CampaignMapWidget;
    UPROPERTY(Transient) TObjectPtr<UEchoesFieldHudTargetingWidget> TargetingWidget;
    UPROPERTY(Transient) TArray<TObjectPtr<UEchoesFieldHudContactWidget>> ContactWidgets;
};
