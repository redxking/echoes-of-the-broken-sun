#pragma once

#include "EchoesSimCore/Simulation.h"

#include <array>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace echoes::sim::net {

inline constexpr std::uint16_t kEnvelopeVersion = 1;
inline constexpr std::uint32_t kProtocolVersion = 1;
inline constexpr std::uint32_t kPlayerViewSchemaVersion = 1;
inline constexpr std::size_t kDigestBytes = 32;
inline constexpr std::size_t kMaximumPacketBytes = 512;

using Digest256 = std::array<std::uint8_t, kDigestBytes>;

enum class PacketKind : std::uint8_t {
    CompatibilityHello = 1,
    CommandRequest = 2,
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
    std::uint32_t simulationRulesVersion = 1;
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

// Player identity is intentionally absent. The authority binds the authenticated
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

struct CommandAdmissionContext final {
    PlayerId player = 0;
    Tick minimumInputDelayTicks = 2;
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

}  // namespace echoes::sim::net
