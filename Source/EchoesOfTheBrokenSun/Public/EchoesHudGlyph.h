#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EchoesFieldHudView.h"
#include "EchoesHudGlyph.generated.h"

/** Resolution-independent command symbols; labels remain the accessible authority. */
UCLASS(NotBlueprintable)
class ECHOESOFTHEBROKENSUN_API UEchoesHudGlyph final : public UUserWidget
{
    GENERATED_BODY()
public:
    void SetControl(const FEchoesFieldHudControl& Control, bool bHighContrast);
protected:
    virtual int32 NativePaint(const FPaintArgs&, const FGeometry&, const FSlateRect&,
        FSlateWindowElementList&, int32, const FWidgetStyle&, bool) const override;
private:
    EEchoesFieldHudAction Action = EEchoesFieldHudAction::None;
    int32 Argument = 0;
    bool bContrast = false;
};
