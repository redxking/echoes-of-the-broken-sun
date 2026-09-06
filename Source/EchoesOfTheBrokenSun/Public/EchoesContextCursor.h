#pragma once

#include "CoreMinimal.h"
#include "EchoesContextCursor.generated.h"

/** REL-UI-011's seven player-facing pointer contexts. */
UENUM(BlueprintType)
enum class EEchoesContextCursor : uint8
{
    DefaultPointer,
    FriendlySelection,
    EnemyAttack,
    Gather,
    Build,
    Invalid,
    Minimap
};

/**
 * Player-scoped observations used to resolve one cursor state. Callers must
 * derive entity facts from PlayerView or a scoped network view.
 */
struct FEchoesContextCursorFacts final
{
    bool bModal = false;
    bool bOverMinimap = false;
    bool bBuildPlacement = false;
    bool bPlacementValid = false;
    bool bTargetActionArmed = false;
    bool bTargetActionValid = true;
    bool bFriendlyEntity = false;
    bool bHostileEntity = false;
    bool bGatherableEntity = false;
};

/** Stable visual vocabulary shared by the software widget and tests. */
struct FEchoesContextCursorStyle final
{
    FName Shape;
    FLinearColor Primary;
    FLinearColor Secondary;
};

struct FEchoesContextCursorModel final
{
    [[nodiscard]] static EEchoesContextCursor Resolve(
        const FEchoesContextCursorFacts& Facts);

    [[nodiscard]] static FEchoesContextCursorStyle Style(
        EEchoesContextCursor Cursor,
        bool bHighContrast);

    [[nodiscard]] static const TCHAR* StableName(
        EEchoesContextCursor Cursor);
};

