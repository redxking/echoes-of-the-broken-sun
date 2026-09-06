#pragma once

#include "EchoesMatchReplay.h"

/**
 * Makes subsystem checkpoint fixtures insensitive to the replay-continuity
 * layer. Historical mission/container bytes remain byte-for-byte fixtures;
 * current saves return their verified inner payload.
 */
inline bool ExtractReplayCheckpointPayloadForTest(
    const TArray<uint8>& Candidate,
    TArray<uint8>& OutPayload,
    FString& OutError)
{
    echoes::sim::ReplayRecord IgnoredReplayPrefix;
    return FEchoesMatchReplayStore::ExtractCheckpointPayload(
               Candidate, OutPayload, IgnoredReplayPrefix, OutError) !=
        EEchoesCheckpointReplayBindingRead::Invalid;
}

inline bool ExtractReplayCheckpointPayloadForTest(
    TArray<uint8>& InOutPayload,
    FString& OutError)
{
    TArray<uint8> Extracted;
    if (!ExtractReplayCheckpointPayloadForTest(
            InOutPayload, Extracted, OutError))
    {
        return false;
    }
    InOutPayload = MoveTemp(Extracted);
    return true;
}
