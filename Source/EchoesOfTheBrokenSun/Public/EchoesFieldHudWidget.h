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
    Reconnect
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

    TWeakObjectPtr<class UEchoesFieldHudWidget> Owner;
    EEchoesFieldHudAction Action = EEchoesFieldHudAction::None;
    int32 Argument = 0;
    bool bFocusedPresentation = false;
    bool bHighContrast = false;
    bool bPointerHovered = false;
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
    virtual FReply NativeOnMouseButtonDown(
        const FGeometry& InGeometry,
        const FPointerEvent& InMouseEvent) override;

private:
    void RebuildContent();
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
    FText EndpointText;

    UPROPERTY(Transient) TObjectPtr<UBorder> RootBorder;
    UPROPERTY(Transient) TObjectPtr<UScrollBox> ContentScroll;
    UPROPERTY(Transient) TObjectPtr<UVerticalBox> ContentBox;
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
        bool bInHighContrast);

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
    virtual FReply NativeOnPreviewKeyDown(
        const FGeometry& InGeometry,
        const FKeyEvent& InKeyEvent) override;

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

    UPROPERTY(Transient) TObjectPtr<UCanvasPanel> RootCanvas;
    UPROPERTY(Transient) TArray<TObjectPtr<UEchoesFieldHudSectionWidget>> Sections;
    UPROPERTY(Transient) TObjectPtr<UEchoesFieldHudMinimapWidget> MinimapWidget;
    UPROPERTY(Transient) TObjectPtr<UEchoesFieldHudCampaignMapWidget> CampaignMapWidget;
    UPROPERTY(Transient) TObjectPtr<UEchoesFieldHudTargetingWidget> TargetingWidget;
    UPROPERTY(Transient) TArray<TObjectPtr<UEchoesFieldHudContactWidget>> ContactWidgets;
};
