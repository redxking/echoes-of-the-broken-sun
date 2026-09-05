#pragma once

#include "CoreMinimal.h"
#include "EchoesSimCore/Simulation.h"

/** Selection facts consumed by the presentation-only tactical command deck. */
struct FEchoesCommandDeckProfile final
{
    int32 WorkerCount = 0;
    int32 CombatCount = 0;
    int32 StructureCount = 0;
    int32 OtherCount = 0;
    bool bHasCommandCore = false;
    bool bHasBarracks = false;
    bool bUseM01RoleNames = false;
};

/** Command-deck actions that a pointer may activate. */
enum class EEchoesCommandDeckAction : uint8
{
    None,
    AttackMove,
    Patrol,
    Hold,
    Guard,
    Stop,
    BuildBarracks,
    BuildDropoff,
    BuildUtility,
    ProduceWorker,
    ProduceSoldier,
    ProduceHeavy,
    ProduceScout,
    ToggleTechnology,
    CycleFormation
};

/**
 * One clickable deck control. `bRequiresCursorTarget` marks the actions whose
 * existing handlers resolve a battlefield position at the cursor: clicking
 * such a button arms the order and the next battlefield click supplies the
 * target, rather than firing the order at the deck panel itself.
 */
struct FEchoesCommandDeckActionEntry final
{
    EEchoesCommandDeckAction Action = EEchoesCommandDeckAction::None;
    const TCHAR* Label = TEXT("");
    const TCHAR* Hotkey = TEXT("");
    bool bRequiresCursorTarget = false;
};

/** Pure command-label model shared by the HUD and automation. */
struct FEchoesCommandDeckModel final
{
    /** Authored M01 Meridian roles; does not change command types or availability. */
    [[nodiscard]] static const TCHAR* GetM01RoleName(echoes::sim::EntityType Type);

    [[nodiscard]] static FString BuildPrimaryActions(
        const FEchoesCommandDeckProfile& Profile);

    /**
     * The same primary actions BuildPrimaryActions describes, as structured
     * clickable entries in the same order. The right-mouse context entry is
     * omitted: it is already a mouse action and needs no button.
     */
    [[nodiscard]] static TArray<FEchoesCommandDeckActionEntry, TInlineAllocator<6>>
    BuildActionEntries(const FEchoesCommandDeckProfile& Profile)
    {
        TArray<FEchoesCommandDeckActionEntry, TInlineAllocator<6>> Entries;
        const auto Add = [&Entries](
                             EEchoesCommandDeckAction Action,
                             const TCHAR* Label,
                             const TCHAR* Hotkey,
                             bool bRequiresCursorTarget)
        {
            if (Entries.Num() < 6)
            {
                Entries.Add(FEchoesCommandDeckActionEntry{
                    Action, Label, Hotkey, bRequiresCursorTarget});
            }
        };

        if (Profile.CombatCount > 0)
        {
            Add(EEchoesCommandDeckAction::AttackMove,
                TEXT("ATTACK-MOVE"), TEXT("F"), true);
            Add(EEchoesCommandDeckAction::Patrol, TEXT("PATROL"), TEXT("T"), true);
            Add(EEchoesCommandDeckAction::Hold, TEXT("HOLD"), TEXT("H"), false);
            Add(EEchoesCommandDeckAction::Guard, TEXT("GUARD"), TEXT("J"), true);
            Add(EEchoesCommandDeckAction::Stop, TEXT("STOP"), TEXT("X"), false);
            Add(EEchoesCommandDeckAction::CycleFormation,
                TEXT("FORMATION"), TEXT("F8"), false);
            return Entries;
        }
        if (Profile.WorkerCount > 0)
        {
            Add(EEchoesCommandDeckAction::BuildBarracks,
                Profile.bUseM01RoleNames ? TEXT("ARRAY FOUNDRY") : TEXT("BARRACKS"), TEXT("B"), true);
            Add(EEchoesCommandDeckAction::BuildDropoff,
                Profile.bUseM01RoleNames ? TEXT("POWER LINK") : TEXT("DROPOFF"), TEXT("N"), true);
            Add(EEchoesCommandDeckAction::BuildUtility,
                Profile.bUseM01RoleNames ? TEXT("AEGIS POST") : TEXT("UTILITY"), TEXT("M"), true);
            Add(EEchoesCommandDeckAction::Stop, TEXT("STOP"), TEXT("X"), false);
            return Entries;
        }
        if (Profile.bHasCommandCore)
        {
            Add(EEchoesCommandDeckAction::ProduceWorker,
                Profile.bUseM01RoleNames ? TEXT("SURVEYOR") : TEXT("WORKER"), TEXT("Q"), false);
        }
        if (Profile.bHasBarracks)
        {
            Add(EEchoesCommandDeckAction::ProduceSoldier,
                Profile.bUseM01RoleNames ? TEXT("LANCER") : TEXT("LINE UNIT"), TEXT("E"), false);
            Add(EEchoesCommandDeckAction::ProduceHeavy,
                Profile.bUseM01RoleNames ? TEXT("BULWARK TEAM") : TEXT("HEAVY"), TEXT(";"), false);
            Add(EEchoesCommandDeckAction::ProduceScout,
                Profile.bUseM01RoleNames ? TEXT("RELAY SKIFF") : TEXT("SCOUT"), TEXT("'"), false);
        }
        if (Profile.bHasBarracks)
        {
            Add(EEchoesCommandDeckAction::ToggleTechnology,
                TEXT("TECHNOLOGY"), TEXT("F2"), false);
        }
        if (Entries.IsEmpty())
        {
            Add(EEchoesCommandDeckAction::Stop, TEXT("STOP"), TEXT("X"), false);
        }
        return Entries;
    }
};
