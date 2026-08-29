#include "EchoesUnburiedRoadMissionModel.h"

EEchoesUnburiedRoadPhase FEchoesUnburiedRoadMissionModel::DeterminePhase(
    const FEchoesUnburiedRoadMissionFacts& Facts)
{
    if (!Facts.bOperationActive)
    {
        return EEchoesUnburiedRoadPhase::Inactive;
    }
    if (!Facts.bLocalCoreIntact || !Facts.bMemoryBearerIntact ||
        !Facts.bWaystoneIntact || !Facts.bSkirmishStillOngoing)
    {
        return EEchoesUnburiedRoadPhase::Failed;
    }
    if (!Facts.bWaystoneRootedAtRoadhead)
    {
        return EEchoesUnburiedRoadPhase::EstablishRoadhead;
    }
    if (!Facts.bListeningSpineComplete)
    {
        return EEchoesUnburiedRoadPhase::RaiseListeningSpine;
    }
    if (!Facts.bMemoryBearerAtShard)
    {
        return EEchoesUnburiedRoadPhase::RecoverMemoryShard;
    }
    return EEchoesUnburiedRoadPhase::Complete;
}

const TCHAR* FEchoesUnburiedRoadMissionModel::StableName(
    EEchoesUnburiedRoadPhase Phase)
{
    switch (Phase)
    {
        case EEchoesUnburiedRoadPhase::Inactive: return TEXT("inactive");
        case EEchoesUnburiedRoadPhase::EstablishRoadhead:
            return TEXT("establish_roadhead");
        case EEchoesUnburiedRoadPhase::RaiseListeningSpine:
            return TEXT("raise_listening_spine");
        case EEchoesUnburiedRoadPhase::RecoverMemoryShard:
            return TEXT("recover_memory_shard");
        case EEchoesUnburiedRoadPhase::Complete: return TEXT("complete");
        case EEchoesUnburiedRoadPhase::Failed: return TEXT("failed");
    }
    return TEXT("unknown");
}

FEchoesUnburiedRoadRoute FEchoesUnburiedRoadMissionModel::RouteForChoice(
    echoes::sim::FutureWellChoice Choice)
{
    using echoes::sim::FutureWellChoice;
    using echoes::sim::Vec2;
    switch (Choice)
    {
        case FutureWellChoice::Harvest:
            return {
                Choice,
                Vec2::FromTiles(14, 28),
                Vec2::FromTiles(14, 37),
                Vec2::FromTiles(20, 43),
                TEXT("ash_cut"),
                TEXT("ASH CUT")};
        case FutureWellChoice::Preserve:
            return {
                Choice,
                Vec2::FromTiles(32, 28),
                Vec2::FromTiles(32, 37),
                Vec2::FromTiles(38, 43),
                TEXT("buried_causeway"),
                TEXT("BURIED CAUSEWAY")};
        case FutureWellChoice::Reshape:
            return {
                Choice,
                Vec2::FromTiles(50, 28),
                Vec2::FromTiles(50, 37),
                Vec2::FromTiles(44, 43),
                TEXT("folded_verge"),
                TEXT("FOLDED VERGE")};
        default:
            return {};
    }
}
