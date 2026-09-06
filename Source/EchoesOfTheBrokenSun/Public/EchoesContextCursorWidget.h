#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EchoesContextCursor.h"
#include "EchoesContextCursorWidget.generated.h"

/**
 * Player-owned software cursor. It never queries the simulation; the owning
 * controller supplies a state resolved from its scoped presentation view.
 */
UCLASS(NotBlueprintable)
class ECHOESOFTHEBROKENSUN_API UEchoesContextCursorWidget final
    : public UUserWidget
{
    GENERATED_BODY()

public:
    void SetCursorState(EEchoesContextCursor NewState, bool bNewHighContrast);
    void SetCursorScale(float NewScale);

    [[nodiscard]] EEchoesContextCursor GetCursorState() const
    {
        return CursorState;
    }

protected:
    virtual void NativeConstruct() override;
    virtual int32 NativePaint(
        const FPaintArgs& Args,
        const FGeometry& AllottedGeometry,
        const FSlateRect& MyCullingRect,
        FSlateWindowElementList& OutDrawElements,
        int32 LayerId,
        const FWidgetStyle& InWidgetStyle,
        bool bParentEnabled) const override;

private:
    EEchoesContextCursor CursorState = EEchoesContextCursor::DefaultPointer;
    bool bHighContrast = false;
    float CursorScale = 1.0f;
};

