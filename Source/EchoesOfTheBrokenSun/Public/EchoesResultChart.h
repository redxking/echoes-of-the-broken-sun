#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "EchoesPlayerFlow.h"
#include "EchoesResultChart.generated.h"

/** Modular UMG results plot. Labels and numeric summaries remain selectable UI text. */
UCLASS()
class ECHOESOFTHEBROKENSUN_API UEchoesResultChart final : public UWidget
{
    GENERATED_BODY()
public:
    void SetChart(const FEchoesShellChart& InChart) { Chart = InChart; }
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
private:
    FEchoesShellChart Chart;
};
