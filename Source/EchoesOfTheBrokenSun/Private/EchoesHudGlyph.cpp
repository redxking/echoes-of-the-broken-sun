#include "EchoesHudGlyph.h"
#include "EchoesCommandDeckModel.h"
#include "Rendering/DrawElements.h"
#include <initializer_list>

void UEchoesHudGlyph::SetControl(const FEchoesFieldHudControl& Control, bool bHighContrast)
{
    Action = Control.Action;
    Argument = Control.Argument;
    bContrast = bHighContrast;
    SetVisibility(ESlateVisibility::HitTestInvisible);
}

int32 UEchoesHudGlyph::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry,
    const FSlateRect& Clip, FSlateWindowElementList& Elements, int32 Layer,
    const FWidgetStyle& Style, bool bEnabled) const
{
    const int32 Base = Super::NativePaint(Args, Geometry, Clip, Elements, Layer, Style, bEnabled);
    const FVector2D Size = Geometry.GetLocalSize();
    const float Unit = FMath::Min(Size.X, Size.Y) / 32.f;
    const FVector2D Origin = (Size - FVector2D(32 * Unit)) * .5f;
    const FLinearColor Color = !bEnabled ? FLinearColor(.32f,.35f,.36f,1)
        : bContrast ? FLinearColor::White : FLinearColor(.58f,.83f,.86f,1);
    const auto Line = [&](std::initializer_list<FVector2D> Points)
    {
        TArray<FVector2D> Path;
        for (const auto& Point : Points) Path.Add(Origin + Point * Unit);
        FSlateDrawElement::MakeLines(Elements, Base + 1, Geometry.ToPaintGeometry(), Path,
            bEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect,
            Color * Style.GetColorAndOpacityTint(), true, 1.6f);
    };
    const auto Box = [&](float X, float Y, float W, float H)
    { Line({{X,Y},{X+W,Y},{X+W,Y+H},{X,Y+H},{X,Y}}); };
    if (Action == EEchoesFieldHudAction::CommandDeck)
    {
        switch (static_cast<EEchoesCommandDeckAction>(Argument))
        {
        case EEchoesCommandDeckAction::AttackMove:
            Line({{6,26},{26,6},{17,7}}); Line({{26,6},{25,15}}); Line({{6,6},{26,26}}); break;
        case EEchoesCommandDeckAction::Patrol:
            Line({{5,12},{24,12},{20,8}}); Line({{27,20},{8,20},{12,24}}); break;
        case EEchoesCommandDeckAction::Hold:
            Box(6,5,6,22); Box(20,5,6,22); break;
        case EEchoesCommandDeckAction::Guard:
            Line({{16,3},{27,8},{25,21},{16,29},{7,21},{5,8},{16,3}});
            Line({{16,10},{16,22}}); Line({{10,16},{22,16}}); break;
        case EEchoesCommandDeckAction::ToggleBulwarkDeployment:
            // Six discrete panes, three on either side of the centre seam.
            for (int32 Pane = 0; Pane < 6; ++Pane) Box(2 + Pane * 5, 6, 3, 20);
            break;
        case EEchoesCommandDeckAction::RepairAtCursor:
            Line({{7,7},{13,13},{9,17},{16,24},{23,17},{19,13},{25,7}});
            Line({{9,24},{23,10}});
            break;
        case EEchoesCommandDeckAction::CancelConstruction:
            Box(5,5,22,22); Line({{9,9},{23,23}}); Line({{23,9},{9,23}});
            break;
        case EEchoesCommandDeckAction::Stop:
            Line({{10,4},{22,4},{28,10},{28,22},{22,28},{10,28},{4,22},{4,10},{10,4}}); Box(11,11,10,10); break;
        case EEchoesCommandDeckAction::BuildBarracks:
        case EEchoesCommandDeckAction::BuildDropoff:
        case EEchoesCommandDeckAction::BuildUtility:
            Line({{5,27},{5,13},{16,5},{27,13},{27,27},{5,27}}); Box(12,17,8,10);
            if (Argument == static_cast<int32>(EEchoesCommandDeckAction::BuildDropoff)) Line({{19,7},{13,16},{20,16},{14,24}});
            if (Argument == static_cast<int32>(EEchoesCommandDeckAction::BuildUtility)) Line({{7,8},{25,8},{25,4}});
            break;
        case EEchoesCommandDeckAction::ProduceWorker:
            Line({{6,7},{14,15},{11,18},{3,10}}); Line({{14,15},{26,27},{29,24},{17,12}}); break;
        case EEchoesCommandDeckAction::ProduceSoldier:
            Line({{9,28},{23,4},{27,7},{13,31}}); Line({{7,19},{18,25}}); break;
        case EEchoesCommandDeckAction::ProduceHeavy:
            Box(5,8,22,18); Line({{10,8},{10,3},{22,3},{22,8}}); Line({{9,14},{23,14}}); break;
        case EEchoesCommandDeckAction::ProduceScout:
            Line({{3,20},{16,4},{29,20},{21,17},{16,26},{11,17},{3,20}}); break;
        case EEchoesCommandDeckAction::CycleFormation:
            Box(3,4,7,7); Box(22,4,7,7); Box(12,21,7,7); Line({{7,14},{16,18},{25,14}}); break;
        case EEchoesCommandDeckAction::ToggleTechnology:
            Box(11,3,10,7); Box(2,22,10,7); Box(20,22,10,7); Line({{16,10},{16,17},{7,17},{7,22}}); Line({{16,17},{25,17},{25,22}}); break;
        default: Box(7,7,18,18); break;
        }
    }
    else if (Action == EEchoesFieldHudAction::ActivateRelaySupply)
    {
        Line({{16,3},{16,29}}); Line({{8,9},{4,16},{8,23}}); Line({{24,9},{28,16},{24,23}}); Box(12,12,8,8);
    }
    else { Box(7,7,18,18); Line({{12,16},{20,16}}); }
    return Base + 1;
}
