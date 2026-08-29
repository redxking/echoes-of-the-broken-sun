#include "EchoesCommandDeckModel.h"

FString FEchoesCommandDeckModel::BuildPrimaryActions(
    const FEchoesCommandDeckProfile& Profile)
{
    if (Profile.CombatCount > 0)
    {
        return TEXT("[RMB] MOVE / CONTEXT    [F] ATTACK-MOVE    [T] PATROL    [H] HOLD    [J] GUARD    [X] STOP");
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
