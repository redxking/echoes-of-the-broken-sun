#pragma once

#include "CoreMinimal.h"

/** Stable reasons for refusing the outer campaign-map checkpoint envelope. */
enum class EEchoesCampaignMapCheckpointFailure : uint8
{
    None,
    Unbound,
    Unsupported,
    Stale,
    Integrity
};

/**
 * The campaign map binding that a saved tactical payload must match exactly.
 * Strings are persisted as canonical UTF-8 bytes; they are identifiers, never display text.
 */
struct ECHOESOFTHEBROKENSUN_API FEchoesCampaignMapCheckpointIdentity final
{
    uint8 MissionOrdinal = 0;
    FString Doctrine;
    FString MapId;
    FString SourceSha256;
    FString TerrainIdentitySha256;

    [[nodiscard]] bool IsBound() const;

    friend bool operator==(
        const FEchoesCampaignMapCheckpointIdentity&,
        const FEchoesCampaignMapCheckpointIdentity&) = default;
};

/**
 * A versioned, CRC-protected envelope around an existing campaign snapshot.
 * It deliberately does not parse or transform the inner payload.
 */
class ECHOESOFTHEBROKENSUN_API FEchoesCampaignMapCheckpoint final
{
public:
    static constexpr uint16 Version = 1;
    static constexpr int32 MaximumPayloadBytes = 64 * 1024 * 1024;

    /** Builds a new outer envelope without modifying OutEnvelope on refusal. */
    [[nodiscard]] static bool Wrap(
        const FEchoesCampaignMapCheckpointIdentity& Identity,
        const TArray<uint8>& Payload,
        TArray<uint8>& OutEnvelope,
        EEchoesCampaignMapCheckpointFailure& OutFailure);

    /**
     * Parses a structurally valid envelope and returns its claimed identity. This is inspection only;
     * callers must use Extract before admitting the payload into an active campaign.
     */
    [[nodiscard]] static bool Inspect(
        const TArray<uint8>& Envelope,
        FEchoesCampaignMapCheckpointIdentity& OutIdentity,
        TArray<uint8>& OutPayload,
        EEchoesCampaignMapCheckpointFailure& OutFailure);

    /**
     * Validates and extracts the original payload only when its binding equals ExpectedIdentity.
     * OutPayload remains untouched on every refusal.
     */
    [[nodiscard]] static bool Extract(
        const TArray<uint8>& Envelope,
        const FEchoesCampaignMapCheckpointIdentity& ExpectedIdentity,
        TArray<uint8>& OutPayload,
        EEchoesCampaignMapCheckpointFailure& OutFailure);
};
