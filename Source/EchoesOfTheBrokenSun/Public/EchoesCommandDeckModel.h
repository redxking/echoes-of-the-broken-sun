#pragma once

#include "CoreMinimal.h"

/** Selection facts consumed by the presentation-only tactical command deck. */
struct FEchoesCommandDeckProfile final
{
    int32 WorkerCount = 0;
    int32 CombatCount = 0;
    int32 StructureCount = 0;
    int32 OtherCount = 0;
    bool bHasCommandCore = false;
    bool bHasBarracks = false;
};

/** Pure command-label model shared by the HUD and automation. */
struct FEchoesCommandDeckModel final
{
    [[nodiscard]] static FString BuildPrimaryActions(
        const FEchoesCommandDeckProfile& Profile);
};
