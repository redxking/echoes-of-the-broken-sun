#include "EchoesContextCursorWidget.h"

#include "Rendering/DrawElements.h"

#include <initializer_list>

namespace
{
void DrawStroke(
    FSlateWindowElementList& Elements,
    int32 Layer,
    const FGeometry& Geometry,
    std::initializer_list<FVector2D> Points,
    const FLinearColor& Color,
    float Thickness,
    bool bClosed = false)
{
    TArray<FVector2D> Line;
    for (const FVector2D Point : Points)
    {
        Line.Add(Point);
    }
    if (bClosed && !Line.IsEmpty())
    {
        const FVector2D FirstPoint = Line[0];
        Line.Add(FirstPoint);
    }
    FSlateDrawElement::MakeLines(
        Elements,
        Layer,
        Geometry.ToPaintGeometry(),
        Line,
        ESlateDrawEffect::None,
        Color,
        true,
        Thickness);
}
}

void UEchoesContextCursorWidget::NativeConstruct()
{
    Super::NativeConstruct();
    SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UEchoesContextCursorWidget::SetCursorState(
    EEchoesContextCursor NewState,
    bool bNewHighContrast)
{
    if (CursorState != NewState || bHighContrast != bNewHighContrast)
    {
        CursorState = NewState;
        bHighContrast = bNewHighContrast;
        InvalidateLayoutAndVolatility();
    }
}

void UEchoesContextCursorWidget::SetCursorScale(float NewScale)
{
    CursorScale = FMath::Clamp(NewScale, 0.75f, 2.0f);
}

int32 UEchoesContextCursorWidget::NativePaint(
    const FPaintArgs& Args,
    const FGeometry& AllottedGeometry,
    const FSlateRect& MyCullingRect,
    FSlateWindowElementList& OutDrawElements,
    int32 LayerId,
    const FWidgetStyle& InWidgetStyle,
    bool bParentEnabled) const
{
    const int32 BaseLayer = Super::NativePaint(
        Args,
        AllottedGeometry,
        MyCullingRect,
        OutDrawElements,
        LayerId,
        InWidgetStyle,
        bParentEnabled);
    const FEchoesContextCursorStyle Style =
        FEchoesContextCursorModel::Style(CursorState, bHighContrast);
    const FVector2D Size = AllottedGeometry.GetLocalSize();
    const float S = FMath::Max(12.0f, FMath::Min(Size.X, Size.Y) * CursorScale);
    const float C = S * 0.5f;
    const float T = bHighContrast ? 3.5f : 2.5f;
    const FLinearColor P = Style.Primary;
    const FLinearColor A = Style.Secondary;

    switch (CursorState)
    {
        case EEchoesContextCursor::FriendlySelection:
            DrawStroke(OutDrawElements, BaseLayer + 1, AllottedGeometry,
                {{2, 12}, {2, 2}, {12, 2}}, A, T);
            DrawStroke(OutDrawElements, BaseLayer + 1, AllottedGeometry,
                {{S - 12, 2}, {S - 2, 2}, {S - 2, 12}}, A, T);
            DrawStroke(OutDrawElements, BaseLayer + 1, AllottedGeometry,
                {{2, S - 12}, {2, S - 2}, {12, S - 2}}, P, T);
            DrawStroke(OutDrawElements, BaseLayer + 1, AllottedGeometry,
                {{S - 12, S - 2}, {S - 2, S - 2}, {S - 2, S - 12}}, P, T);
            break;
        case EEchoesContextCursor::EnemyAttack:
            DrawStroke(OutDrawElements, BaseLayer + 1, AllottedGeometry,
                {{C, 2}, {S - 2, C}, {C, S - 2}, {2, C}}, A, T, true);
            DrawStroke(OutDrawElements, BaseLayer + 2, AllottedGeometry,
                {{C, 8}, {C, S - 8}}, P, T);
            DrawStroke(OutDrawElements, BaseLayer + 2, AllottedGeometry,
                {{8, C}, {S - 8, C}}, P, T);
            break;
        case EEchoesContextCursor::Gather:
            DrawStroke(OutDrawElements, BaseLayer + 1, AllottedGeometry,
                {{8, S - 5}, {S - 10, 9}}, A, T + 1.0f);
            DrawStroke(OutDrawElements, BaseLayer + 2, AllottedGeometry,
                {{9, 8}, {C, 3}, {S - 5, 11}}, P, T + 1.0f);
            break;
        case EEchoesContextCursor::Build:
            DrawStroke(OutDrawElements, BaseLayer + 1, AllottedGeometry,
                {{3, 3}, {S - 3, 3}, {S - 3, S - 3}, {3, S - 3}}, A, T, true);
            DrawStroke(OutDrawElements, BaseLayer + 2, AllottedGeometry,
                {{C, 3}, {C, S - 3}}, P, T);
            DrawStroke(OutDrawElements, BaseLayer + 2, AllottedGeometry,
                {{3, C}, {S - 3, C}}, P, T);
            break;
        case EEchoesContextCursor::Invalid:
            DrawStroke(OutDrawElements, BaseLayer + 1, AllottedGeometry,
                {{4, 4}, {S - 4, S - 4}}, A, T + 1.5f);
            DrawStroke(OutDrawElements, BaseLayer + 1, AllottedGeometry,
                {{S - 4, 4}, {4, S - 4}}, A, T + 1.5f);
            break;
        case EEchoesContextCursor::Minimap:
            DrawStroke(OutDrawElements, BaseLayer + 1, AllottedGeometry,
                {{C, 2}, {S - 7, 7}, {S - 2, C}, {S - 7, S - 7},
                 {C, S - 2}, {7, S - 7}, {2, C}, {7, 7}}, A, T, true);
            DrawStroke(OutDrawElements, BaseLayer + 2, AllottedGeometry,
                {{C, C}, {S - 5, 8}}, P, T);
            break;
        case EEchoesContextCursor::DefaultPointer:
        default:
            DrawStroke(OutDrawElements, BaseLayer + 1, AllottedGeometry,
                {{3, 2}, {3, S - 5}, {10, S - 12}, {15, S - 2},
                 {20, S - 5}, {15, S - 15}, {S - 5, S - 15}}, P, T, true);
            break;
    }
    return BaseLayer + 2;
}
