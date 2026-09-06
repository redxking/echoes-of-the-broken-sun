#include "EchoesContextCursor.h"

EEchoesContextCursor FEchoesContextCursorModel::Resolve(
    const FEchoesContextCursorFacts& Facts)
{
    if (Facts.bModal)
    {
        return EEchoesContextCursor::DefaultPointer;
    }
    if (Facts.bOverMinimap)
    {
        return EEchoesContextCursor::Minimap;
    }
    if (Facts.bBuildPlacement)
    {
        return Facts.bPlacementValid
            ? EEchoesContextCursor::Build
            : EEchoesContextCursor::Invalid;
    }
    if (Facts.bTargetActionArmed && !Facts.bTargetActionValid)
    {
        return EEchoesContextCursor::Invalid;
    }
    if (Facts.bGatherableEntity)
    {
        return EEchoesContextCursor::Gather;
    }
    if (Facts.bHostileEntity)
    {
        return EEchoesContextCursor::EnemyAttack;
    }
    if (Facts.bFriendlyEntity)
    {
        return EEchoesContextCursor::FriendlySelection;
    }
    return EEchoesContextCursor::DefaultPointer;
}

FEchoesContextCursorStyle FEchoesContextCursorModel::Style(
    EEchoesContextCursor Cursor,
    bool bHighContrast)
{
    const FLinearColor Pale = bHighContrast
        ? FLinearColor::White
        : FLinearColor(0.78f, 0.92f, 1.0f, 1.0f);
    const FLinearColor Dark = bHighContrast
        ? FLinearColor::Black
        : FLinearColor(0.015f, 0.025f, 0.045f, 0.95f);
    switch (Cursor)
    {
        case EEchoesContextCursor::FriendlySelection:
            return {TEXT("FriendlyBrackets"), Pale,
                    FLinearColor(0.12f, 0.86f, 0.92f, 1.0f)};
        case EEchoesContextCursor::EnemyAttack:
            return {TEXT("AttackDiamond"), Pale,
                    FLinearColor(1.0f, 0.24f, 0.18f, 1.0f)};
        case EEchoesContextCursor::Gather:
            return {TEXT("GatherPickaxe"), Pale,
                    FLinearColor(0.98f, 0.72f, 0.20f, 1.0f)};
        case EEchoesContextCursor::Build:
            return {TEXT("BuildBlueprint"), Pale,
                    FLinearColor(0.18f, 0.72f, 1.0f, 1.0f)};
        case EEchoesContextCursor::Invalid:
            return {TEXT("InvalidCross"), Pale,
                    FLinearColor(1.0f, 0.20f, 0.16f, 1.0f)};
        case EEchoesContextCursor::Minimap:
            return {TEXT("MinimapRadar"), Pale,
                    FLinearColor(0.34f, 1.0f, 0.62f, 1.0f)};
        case EEchoesContextCursor::DefaultPointer:
        default:
            return {TEXT("DefaultArrow"), Pale, Dark};
    }
}

const TCHAR* FEchoesContextCursorModel::StableName(
    EEchoesContextCursor Cursor)
{
    switch (Cursor)
    {
        case EEchoesContextCursor::FriendlySelection: return TEXT("friendly-selection");
        case EEchoesContextCursor::EnemyAttack: return TEXT("enemy-attack");
        case EEchoesContextCursor::Gather: return TEXT("gather");
        case EEchoesContextCursor::Build: return TEXT("build");
        case EEchoesContextCursor::Invalid: return TEXT("invalid");
        case EEchoesContextCursor::Minimap: return TEXT("minimap");
        case EEchoesContextCursor::DefaultPointer:
        default: return TEXT("default");
    }
}

