#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/Slider.h"
#include "EchoesPlayerFlow.h"
#include "EchoesShellWidget.generated.h"

class AEchoesPlayerController;
class UOverlay;
class UScrollBox;
class UTextBlock;
class UEchoesShellWidget;

/** Pointer slider; adjacent step controls provide the same setting to keyboard users. */
UCLASS()
class ECHOESOFTHEBROKENSUN_API UEchoesShellValueSlider final : public USlider
{
    GENERATED_BODY()
public:
    void Configure(UEchoesShellWidget* InShell, const FEchoesShellSlider& Model);
private:
    UFUNCTION() void BeginEdit();
    UFUNCTION() void ChangeValue(float NewValue);
    UFUNCTION() void EndEdit();
    TWeakObjectPtr<UEchoesShellWidget> Shell;
    EEchoesShellAction Action = EEchoesShellAction::HudScaleValue;
};

UCLASS()
class ECHOESOFTHEBROKENSUN_API UEchoesShellActionButton final : public UButton
{
    GENERATED_BODY()

public:
    UEchoesShellActionButton(
        const FObjectInitializer& ObjectInitializer);
    void Configure(
        UEchoesShellWidget* InShell,
        AEchoesPlayerController* InController,
        EEchoesShellAction InAction,
        int32 InArgument);
    bool Activate();
    void ApplyPresentation(bool bFocused, bool bHighContrast);

private:
    UFUNCTION()
    void HandleClicked();
    UFUNCTION()
    void HandleHovered();
    UFUNCTION()
    void HandleUnhovered();
    void HandleReceivedFocus();

    bool bPresentationFocused = false;
    bool bPresentationHighContrast = false;
    TWeakObjectPtr<UEchoesShellWidget> Shell;
    TWeakObjectPtr<AEchoesPlayerController> Controller;
    EEchoesShellAction Action = EEchoesShellAction::Back;
    int32 Argument = 0;
};

UCLASS()
class ECHOESOFTHEBROKENSUN_API UEchoesShellWidget final : public UUserWidget
{
    GENERATED_BODY()

public:
    UEchoesShellWidget(const FObjectInitializer& ObjectInitializer);

    void SetView(const FEchoesShellView& InView);
    int32 GetButtonCount() const { return ActionButtons.Num(); }
    int32 GetFocusedButtonIndex() const { return FocusedButtonIndex; }
    UEchoesShellActionButton* GetActionButton(int32 Index) const { return ActionButtons.IsValidIndex(Index) ? ActionButtons[Index] : nullptr; }
    bool ActivateFocused();
    bool ActivateButtonUnderLocation(const FVector2D& ScreenPosition);
    bool FocusNext(bool bReverse = false);
    bool HandleNavigationKey(const FKey& Key, bool bShift, bool bRepeat = false);
    void NotifyButtonFocused(UEchoesShellActionButton* Button);
    void BeginValueEdit() { bEditingValue = true; }
    void UpdateValue(EEchoesShellAction Action, float Value, bool bCommit);

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    virtual FReply NativeOnPreviewKeyDown(
        const FGeometry& InGeometry,
        const FKeyEvent& InKeyEvent) override;
    virtual FReply NativeOnKeyDown(
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
    virtual FReply NativeOnMouseWheel(
        const FGeometry& InGeometry,
        const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseMove(
        const FGeometry& InGeometry,
        const FPointerEvent& InMouseEvent) override;

private:
    bool IsSameView(const FEchoesShellView& Candidate) const;
    void RebuildView();
    bool FocusButton(int32 Index);
    bool FocusAction(EEchoesShellAction Action, int32 Argument);
    bool FocusDefaultButton();
    void RefreshButtonPresentation();
    AEchoesPlayerController* ResolveController() const;

    FEchoesShellView View;
    bool bHasView = false;
    bool bEditingValue = false;
    bool bSuppressFocusScroll = false;
    int32 FocusedButtonIndex = INDEX_NONE;

    UPROPERTY(Transient)
    TObjectPtr<UOverlay> RootOverlay;
    UPROPERTY(Transient)
    TObjectPtr<UScrollBox> ContentScroll;
    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> EyebrowText;
    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> TitleText;
    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> BodyText;
    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> StatusText;
    UPROPERTY(Transient)
    TArray<TObjectPtr<UEchoesShellActionButton>> ActionButtons;
    UPROPERTY(Transient)
    TArray<TObjectPtr<UEchoesShellValueSlider>> ValueSliders;
    UPROPERTY(Transient)
    TArray<TObjectPtr<UTextBlock>> SliderLabels;
};
