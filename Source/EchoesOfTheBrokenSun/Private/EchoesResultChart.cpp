#include "EchoesResultChart.h"

#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SLeafWidget.h"

namespace
{
class SEchoesResultChart final : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SEchoesResultChart) {} SLATE_END_ARGS()
    void Construct(const FArguments&, const FEchoesShellChart& InChart) { Chart = InChart; }
    virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(700, 190); }
    virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry,
        const FSlateRect&, FSlateWindowElementList& Elements, int32 Layer,
        const FWidgetStyle&, bool) const override
    {
        const FVector2D Size = Geometry.GetLocalSize();
        const FVector2D Origin(54, 12);
        const FVector2D Plot(FMath::Max(1.0, Size.X - 66), FMath::Max(1.0, Size.Y - 40));
        double MaxX = 1, MaxY = 1;
        for (const auto& Series : Chart.Series)
            for (const auto& Point : Series.Samples)
                if (FMath::IsFinite(Point.X) && FMath::IsFinite(Point.Y))
                { MaxX = FMath::Max(MaxX, Point.X); MaxY = FMath::Max(MaxY, Point.Y); }
        const auto Position = [&](const FVector2D& Point)
        {
            return FVector2D(Origin.X + FMath::Clamp(Point.X / MaxX, 0.0, 1.0) * Plot.X,
                Origin.Y + Plot.Y * (1 - FMath::Clamp(Point.Y / MaxY, 0.0, 1.0)));
        };
        const auto Line = [&](const TArray<FVector2D>& Points, FLinearColor Color, float Thickness)
        {
            FSlateDrawElement::MakeLines(Elements, Layer, Geometry.ToPaintGeometry(), Points,
                ESlateDrawEffect::None, Color, true, Thickness);
        };
        Line({Origin, Origin + FVector2D(0, Plot.Y), Origin + Plot}, FLinearColor(.7f,.75f,.8f), 1.5f);
        const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Regular", 12);
        const auto Text = [&](FVector2D At, const FString& Value)
        {
            FSlateDrawElement::MakeText(Elements, Layer + 1,
                Geometry.ToPaintGeometry(FVector2D(100, 20), FSlateLayoutTransform(At)),
                Value, Font, ESlateDrawEffect::None, FLinearColor::White);
        };
        Text(FVector2D(0, Origin.Y), FString::Printf(TEXT("%.0f"), MaxY));
        Text(FVector2D(28, Origin.Y + Plot.Y - 12), TEXT("0"));
        Text(Origin + FVector2D(0, Plot.Y + 7), TEXT("0:00"));
        const int32 Seconds = FMath::Max(0, FMath::RoundToInt(MaxX));
        Text(Origin + Plot + FVector2D(-52, 7), FString::Printf(TEXT("%d:%02d"), Seconds / 60, Seconds % 60));
        for (int32 SeriesIndex = 0; SeriesIndex < Chart.Series.Num(); ++SeriesIndex)
        {
            const auto& Series = Chart.Series[SeriesIndex];
            for (int32 Index = 1; Index < Series.Samples.Num(); ++Index)
            {
                const FVector2D A = Series.Samples[Index-1], B = Series.Samples[Index];
                if (!FMath::IsFinite(A.X) || !FMath::IsFinite(A.Y) || !FMath::IsFinite(B.X) || !FMath::IsFinite(B.Y)) continue;
                const FVector2D Start = Position(A), End = Position(B);
                // Distinct dash lengths keep seats distinguishable without color.
                if (SeriesIndex == 0) Line({Start, End}, Series.Color, 2.5f);
                else
                {
                    const double Length = FVector2D::Distance(Start, End);
                    const double Dash = 3.0 + 3.0 * SeriesIndex;
                    for (double Offset = 0; Offset < Length; Offset += Dash + 4.0)
                        Line({FMath::Lerp(Start, End, Offset / Length),
                            FMath::Lerp(Start, End, FMath::Min(Length, Offset + Dash) / Length)}, Series.Color, 2.5f);
                }
            }
        }
        return Layer + 1;
    }
private:
    FEchoesShellChart Chart;
};
}

TSharedRef<SWidget> UEchoesResultChart::RebuildWidget()
{
    return SNew(SEchoesResultChart, Chart);
}
