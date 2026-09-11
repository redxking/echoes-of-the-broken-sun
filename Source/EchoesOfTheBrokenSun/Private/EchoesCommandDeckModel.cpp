#include "EchoesCommandDeckModel.h"

#include <algorithm>

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
        return TEXT("[RMB] GATHER / DELIVER / MOVE    [B] BARRACKS    [N] DROPOFF    [M] UTILITY    [R] REPAIR    [X] STOP");
    }
    if (Profile.bCanCancelSelectedConstruction)
    {
        return TEXT("[SHIFT+X] CANCEL CONSTRUCTION    [X] STOP");
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
    if (Profile.StructureCount > 0)
    {
        // Power Links and Aegis Posts act on their own; nothing to press.
        return TEXT("");
    }
    return TEXT("[RMB] CONTEXT / MOVE    [X] STOP");
}

TArray<uint32> FEchoesCommandDeckModel::ResolveLocalBulwarkCasters(
    const echoes::sim::Simulation& Simulation, echoes::sim::PlayerId Player,
    echoes::sim::Vec2 Target, const TArray<uint32>& Selection, bool bAllEligible)
{
    using namespace echoes::sim;
    std::vector<EntityId> Candidates;
    for (uint32 Id : Selection)
    {
        // The Unreal order adapter requires a direction when deploying. A
        // packed unit under the target cannot cast, while packing needs none.
        const Entity* Actor = Simulation.FindEntity(Id);
        if (Actor && !Actor->deployed && Actor->position == Target) continue;
        bool bPending = false;
        for (const Command& Pending : Simulation.PendingCommands())
            if (Pending.player == Player && Pending.actor == Id && Pending.type == CommandType::ToggleDeploy)
            { bPending = true; break; }
        if (!bPending) Candidates.push_back(Id);
    }
    TArray<uint32> Result;
    // An empty candidate list means "search all units" to SimCore. A user
    // gesture with no selected candidate must instead issue no command.
    while (!Candidates.empty())
    {
        const EntityId Caster = Simulation.FindSmartCastCaster(Player,
            CommandType::ToggleDeploy, Target, 0, Candidates);
        if (Caster == 0) break;
        Result.Add(Caster);
        if (!bAllEligible) break;
        Candidates.erase(std::remove(Candidates.begin(), Candidates.end(), Caster), Candidates.end());
    }
    return Result;
}
