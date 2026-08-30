#pragma once

#include "EchoesSimCore/Simulation.h"

#include <array>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace echoes::sim::net {

inline constexpr std::uint16_t kEnvelopeVersion = 1;
inline constexpr std::uint32_t kProtocolVersion = 3;
inline constexpr std::uint32_t kPlayerViewSchemaVersion = 2;
inline constexpr std::size_t kDigestBytes = 32;
inline constexpr std::size_t kMaximumPacketBytes = 512;
inline constexpr std::size_t kMaximumCommandBatchBytes = 16 * 1024;
inline constexpr std::size_t kMaximumCommandsPerBatch = 512;
inline constexpr std::size_t kMaximumScopedKeyframeBytes = 256 * 1024;
inline constexpr std::size_t kMaximumScopedDeltaBytes = 256 * 1024;
inline constexpr std::size_t kMaximumScopedEntities = 4096;
inline constexpr std::size_t kMaximumScopedTiles = 256 * 256;
inline constexpr std::size_t kMaximumVibrationSignatures = 1024;

using Digest256 = std::array<std::uint8_t, kDigestBytes>;

enum class PacketKind : std::uint8_t {
    CompatibilityHello = 1,
    CommandRequest = 2,
    ScopedViewKeyframe = 3,
    ScopedViewDelta = 4,
    CommandBatchRequest = 5,
};

enum class DecodeStatus : std::uint8_t {
    Ok = 0,
    PacketTooLarge,
    LengthMismatch,
    BadMagic,
    UnsupportedEnvelopeVersion,
    WrongPacketKind,
    ReservedFieldNonzero,
    InvalidEncoding,
    IntegrityMismatch,
};

[[nodiscard]] ECHOESSIMCORE_API std::string_view StableId(DecodeStatus status);

struct CompatibilityManifest final {
    std::uint32_t protocolVersion = kProtocolVersion;
    std::uint32_t snapshotVersion = kSnapshotVersion;
    std::uint32_t simulationRulesVersion = 2;
    std::uint32_t playerViewSchemaVersion = kPlayerViewSchemaVersion;
    std::uint64_t serializationFeatureFlags = 0;
    Digest256 buildIdSha256{};
    Digest256 rulesPackSha256{};
    Digest256 mapPackSha256{};
    Digest256 matchSettingsSha256{};

    friend bool operator==(const CompatibilityManifest&,
                           const CompatibilityManifest&) = default;
};

enum class CompatibilityStatus : std::uint8_t {
    Accepted = 0,
    ProtocolMismatch,
    SnapshotSchemaMismatch,
    SimulationRulesMismatch,
    PlayerViewSchemaMismatch,
    BuildMismatch,
    RulesPackMismatch,
    MapPackMismatch,
    SerializationFeaturesMismatch,
    MatchSettingsMismatch,
};

[[nodiscard]] ECHOESSIMCORE_API CompatibilityStatus CheckCompatibility(
    const CompatibilityManifest& authority,
    const CompatibilityManifest& remote);
[[nodiscard]] ECHOESSIMCORE_API std::string_view StableId(
    CompatibilityStatus status);

[[nodiscard]] ECHOESSIMCORE_API std::vector<std::uint8_t>
EncodeCompatibilityHello(const CompatibilityManifest& manifest);
[[nodiscard]] ECHOESSIMCORE_API DecodeStatus DecodeCompatibilityHello(
    std::span<const std::uint8_t> bytes,
    CompatibilityManifest& manifest);

// Player identity is intentionally absent. The authority binds the owning
// connection to a seat and supplies PlayerId only after this packet is decoded.
struct CommandRequest final {
    std::uint64_t sequence = 0;
    Tick executeTick = 0;
    CommandType type = CommandType::Stop;
    EntityId actor = 0;
    EntityId target = 0;
    Vec2 position{};
    EntityType buildType = EntityType::Barracks;
    FutureWellChoice wellChoice = FutureWellChoice::Dormant;
    WarformAdaptation warformAdaptation = WarformAdaptation::None;
    ResearchType researchType = ResearchType::None;

    friend bool operator==(const CommandRequest&, const CommandRequest&) = default;
};

[[nodiscard]] ECHOESSIMCORE_API std::vector<std::uint8_t>
EncodeCommandRequest(const CommandRequest& request);
[[nodiscard]] ECHOESSIMCORE_API DecodeStatus DecodeCommandRequest(
    std::span<const std::uint8_t> bytes,
    CommandRequest& request);

// A single player gesture can address a bounded, canonically ordered set of
// actors. Seat, simulation sequence, and execution tick remain authority-owned.
struct CommandIntent final {
    CommandType type = CommandType::Stop;
    EntityId actor = 0;
    EntityId target = 0;
    Vec2 position{};
    EntityType buildType = EntityType::Barracks;
    FutureWellChoice wellChoice = FutureWellChoice::Dormant;
    WarformAdaptation warformAdaptation = WarformAdaptation::None;
    ResearchType researchType = ResearchType::None;

    friend bool operator==(const CommandIntent&, const CommandIntent&) = default;
};

struct CommandBatchRequest final {
    std::uint64_t clientBatchId = 0;
    std::vector<CommandIntent> intents{};

    friend bool operator==(const CommandBatchRequest&,
                           const CommandBatchRequest&) = default;
};

[[nodiscard]] ECHOESSIMCORE_API std::vector<std::uint8_t>
EncodeCommandBatchRequest(const CommandBatchRequest& request);
[[nodiscard]] ECHOESSIMCORE_API DecodeStatus DecodeCommandBatchRequest(
    std::span<const std::uint8_t> bytes,
    CommandBatchRequest& request);

struct CommandAdmissionContext final {
    PlayerId player = 0;
    Tick minimumInputDelayTicks = 3;
    Tick maximumLeadTicks = 40;
    bool hasAcceptedSequence = false;
    std::uint64_t lastAcceptedSequence = 0;
};

enum class CommandAdmissionStatus : std::uint8_t {
    Accepted = 0,
    InvalidSeat,
    ActorNotOwned,
    SequenceUnexpected,
    TickTooEarly,
    TickTooLate,
    TickRangeInvalid,
    CommandRejected,
};

[[nodiscard]] ECHOESSIMCORE_API std::string_view StableId(
    CommandAdmissionStatus status);

// Mutates the context and simulation only after all network-level checks pass.
// Simulation remains the final authority for command semantics and capacity.
ECHOESSIMCORE_API CommandAdmissionStatus AdmitCommandRequest(
    const CommandRequest& request,
    CommandAdmissionContext& context,
    Simulation& simulation,
    std::string* simulationRejection = nullptr);

struct ScopedTileState final {
    Visibility visibility = Visibility::Unexplored;
    Terrain terrain = Terrain::Blocked;
    bool passable = false;

    friend bool operator==(const ScopedTileState&,
                           const ScopedTileState&) = default;
};

struct ScopedEntityState final {
    EntityId id = 0;
    PlayerId owner = kNeutralPlayer;
    Faction faction = Faction::MeridianCompact;
    EntityType type = EntityType::Worker;
    Vec2 position{};
    std::int32_t hitPoints = 1;
    std::int32_t maxHitPoints = 1;
    bool completed = true;
    FutureWellChoice wellChoice = FutureWellChoice::Dormant;
    bool deployed = false;
    WaystoneMode waystoneMode = WaystoneMode::NotWaystone;
    WarformAdaptation warformAdaptation = WarformAdaptation::None;
    bool aegisPowered = false;
    ChoirIdentityState choirIdentityState = ChoirIdentityState::NotChoir;
    Tick choirIdentityResolveAtTick = 0;
    Tick choirIdentityNextAvailableTick = 0;
    Tick choirCoherenceNextChargeTick = 0;

    friend bool operator==(const ScopedEntityState&,
                           const ScopedEntityState&) = default;
};

struct ScopedViewKeyframe final {
    std::uint32_t protocolVersion = kProtocolVersion;
    std::uint64_t snapshotId = 0;
    Tick simulationTick = 0;
    std::uint32_t playerViewSchemaVersion = kPlayerViewSchemaVersion;
    std::uint64_t lastAcceptedSequence = 0;
    std::int32_t mapWidthTiles = 0;
    std::int32_t mapHeightTiles = 0;
    PlayerId player = 0;
    Faction faction = Faction::MeridianCompact;
    ResourcePool resources{};
    std::int32_t populationUsed = 0;
    std::int32_t populationCapacity = 0;
    std::vector<ScopedTileState> tiles{};
    std::vector<ScopedEntityState> entities{};
    std::vector<VibrationSignature> vibrationSignatures{};
    std::uint64_t scopedDigest = 0;

    friend bool operator==(const ScopedViewKeyframe&,
                           const ScopedViewKeyframe&) = default;
};

struct ScopedTileChange final {
    std::uint32_t index = 0;
    ScopedTileState state{};

    friend bool operator==(const ScopedTileChange&,
                           const ScopedTileChange&) = default;
};

struct ScopedViewDelta final {
    std::uint32_t protocolVersion = kProtocolVersion;
    std::uint64_t snapshotId = 0;
    std::uint64_t baseSnapshotId = 0;
    Tick simulationTick = 0;
    std::uint32_t playerViewSchemaVersion = kPlayerViewSchemaVersion;
    std::uint64_t lastAcceptedSequence = 0;
    std::int32_t mapWidthTiles = 0;
    std::int32_t mapHeightTiles = 0;
    PlayerId player = 0;
    Faction faction = Faction::MeridianCompact;
    ResourcePool resources{};
    std::int32_t populationUsed = 0;
    std::int32_t populationCapacity = 0;
    std::vector<ScopedTileChange> tileChanges{};
    std::vector<ScopedEntityState> entityUpserts{};
    std::vector<EntityId> removedEntityIds{};
    std::vector<VibrationSignature> vibrationSignatures{};
    std::uint64_t scopedDigest = 0;

    friend bool operator==(const ScopedViewDelta&,
                           const ScopedViewDelta&) = default;
};

// Materializes only information already admitted by PlayerView. Hidden entities,
// authoritative random state, opponent internals, and full-state checksums are
// unavailable at this boundary.
[[nodiscard]] ECHOESSIMCORE_API bool BuildScopedViewKeyframe(
    const PlayerView& view,
    std::uint64_t snapshotId,
    std::uint64_t lastAcceptedSequence,
    ScopedViewKeyframe& keyframe,
    std::string* error = nullptr);
[[nodiscard]] ECHOESSIMCORE_API std::vector<std::uint8_t>
EncodeScopedViewKeyframe(const ScopedViewKeyframe& keyframe);
[[nodiscard]] ECHOESSIMCORE_API DecodeStatus DecodeScopedViewKeyframe(
    std::span<const std::uint8_t> bytes,
    ScopedViewKeyframe& keyframe);

[[nodiscard]] ECHOESSIMCORE_API bool BuildScopedViewDelta(
    const ScopedViewKeyframe& base,
    const ScopedViewKeyframe& current,
    ScopedViewDelta& delta,
    std::string* error = nullptr);
[[nodiscard]] ECHOESSIMCORE_API std::vector<std::uint8_t>
EncodeScopedViewDelta(const ScopedViewDelta& delta);
[[nodiscard]] ECHOESSIMCORE_API DecodeStatus DecodeScopedViewDelta(
    std::span<const std::uint8_t> bytes,
    ScopedViewDelta& delta);
[[nodiscard]] ECHOESSIMCORE_API bool ApplyScopedViewDelta(
    const ScopedViewKeyframe& base,
    const ScopedViewDelta& delta,
    ScopedViewKeyframe& current,
    std::string* error = nullptr);

}  // namespace echoes::sim::net
