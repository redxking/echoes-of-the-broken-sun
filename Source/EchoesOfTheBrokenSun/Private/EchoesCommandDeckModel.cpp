#include "EchoesCommandDeckModel.h"

const TCHAR* FEchoesCommandDeckModel::GetM01RoleName(echoes::sim::EntityType Type)
{
    switch (Type)
    {
        case echoes::sim::EntityType::Worker: return TEXT("Surveyor");
        case echoes::sim::EntityType::Soldier: return TEXT("Lancer");
        case echoes::sim::EntityType::HeavyUnit: return TEXT("Bulwark Team");
        case echoes::sim::EntityType::ScoutUnit: return TEXT("Relay Skiff");
        case echoes::sim::EntityType::Barracks: return TEXT("Array Foundry");
        case echoes::sim::EntityType::Dropoff: return TEXT("Power Link");
        case echoes::sim::EntityType::UtilityStructure: return TEXT("Aegis Post");
        default: return TEXT("Unit");
    }
}

FString FEchoesCommandDeckModel::BuildPrimaryActions(
    const FEchoesCommandDeckProfile& Profile)
{
    if (Profile.CombatCount > 0)
    {
        return TEXT("[RMB] MOVE / CONTEXT    [F] ATTACK-MOVE    [T] PATROL    [H] HOLD    [J] GUARD    [X] STOP");
    }
    if (Profile.bUseM01RoleNames)
    {
        FString Actions = Profile.WorkerCount > 0
            ? TEXT("[RMB] GATHER / DELIVER / MOVE    ") : TEXT("");
        for (const FEchoesCommandDeckActionEntry& Entry : BuildActionEntries(Profile))
        {
            if (!Actions.IsEmpty() && !Actions.EndsWith(TEXT("    "))) Actions += TEXT("    ");
            Actions += FString::Printf(TEXT("[%s] %s"), Entry.Hotkey, Entry.Label);
        }
        return Actions;
    }
    if (Profile.WorkerCount > 0)
    {
        return TEXT("[RMB] GATHER / DELIVER / MOVE    [B] BARRACKS    [N] DROPOFF    [M] UTILITY    [X] STOP");
    }
    if (Profile.bHasCommandCore && Profile.bHasBarracks)
    {
        return TEXT("[Q] WORKER    [E] LINE UNIT    [;] HEAVY    ['] SCOUT    [F2] TECHNOLOGY");
    }
    if (Profile.bHasCommandCore)
    {
        return TEXT("[Q] PRODUCE WORKER");
    }
    if (Profile.bHasBarracks)
    {
        return TEXT("[E] LINE UNIT    [;] HEAVY    ['] SCOUT    [F2] TECHNOLOGY");
    }
    return TEXT("[RMB] CONTEXT / MOVE    [X] STOP");
}
