#include "EchoesFormationLayout.h"

TArray<FVector> FEchoesFormationLayout::BuildDestinations(
    const FVector& Anchor,
    const FVector& Forward,
    int32 UnitCount,
    EEchoesFormationType Formation,
    float SpacingWorldUnits)
{
    TArray<FVector> Destinations;
    if (UnitCount <= 0)
    {
        return Destinations;
    }

    Destinations.Reserve(UnitCount);
    FVector2D Forward2D(Forward.X, Forward.Y);
    if (!Forward2D.Normalize())
    {
        Forward2D = FVector2D(1.0f, 0.0f);
    }
    const FVector ForwardAxis(Forward2D.X, Forward2D.Y, 0.0f);
    const FVector RightAxis(-Forward2D.Y, Forward2D.X, 0.0f);
    const float Spacing = FMath::Max(1.0f, SpacingWorldUnits);

    if (Formation == EEchoesFormationType::Line)
    {
        for (int32 Index = 0; Index < UnitCount; ++Index)
        {
            const float Lateral =
                (static_cast<float>(Index) -
                 static_cast<float>(UnitCount - 1) * 0.5f) *
                Spacing;
            Destinations.Add(Anchor + RightAxis * Lateral);
        }
        return Destinations;
    }

    if (Formation == EEchoesFormationType::Wedge)
    {
        Destinations.Add(Anchor);
        for (int32 Index = 1; Index < UnitCount; ++Index)
        {
            const int32 Rank = (Index + 1) / 2;
            const float Side = Index % 2 == 1 ? -1.0f : 1.0f;
            Destinations.Add(
                Anchor - ForwardAxis * (static_cast<float>(Rank) * Spacing * 0.85f) +
                RightAxis * (Side * static_cast<float>(Rank) * Spacing * 0.75f));
        }
        return Destinations;
    }

    const int32 Width = FMath::Max(
        1,
        FMath::CeilToInt(FMath::Sqrt(static_cast<float>(UnitCount))));
    const int32 Rows = FMath::DivideAndRoundUp(UnitCount, Width);
    for (int32 Row = 0; Row < Rows; ++Row)
    {
        const int32 RowStart = Row * Width;
        const int32 RowCount = FMath::Min(Width, UnitCount - RowStart);
        const float Longitudinal =
            (static_cast<float>(Row) -
             static_cast<float>(Rows - 1) * 0.5f) *
            Spacing;
        for (int32 Column = 0; Column < RowCount; ++Column)
        {
            const float Lateral =
                (static_cast<float>(Column) -
                 static_cast<float>(RowCount - 1) * 0.5f) *
                Spacing;
            Destinations.Add(
                Anchor + ForwardAxis * Longitudinal + RightAxis * Lateral);
        }
    }
    FVector DestinationCentroid = FVector::ZeroVector;
    for (const FVector& Destination : Destinations)
    {
        DestinationCentroid += Destination;
    }
    DestinationCentroid /= static_cast<float>(Destinations.Num());
    const FVector CenteringOffset = Anchor - DestinationCentroid;
    for (FVector& Destination : Destinations)
    {
        Destination += CenteringOffset;
    }
    return Destinations;
}

FString FEchoesFormationLayout::DisplayName(EEchoesFormationType Formation)
{
    switch (Formation)
    {
        case EEchoesFormationType::Line:
            return TEXT("LINE");
        case EEchoesFormationType::Wedge:
            return TEXT("WEDGE");
        case EEchoesFormationType::Box:
        default:
            return TEXT("BOX");
    }
}
