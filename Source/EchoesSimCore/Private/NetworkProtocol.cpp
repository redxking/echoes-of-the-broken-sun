#include "EchoesSimCore/NetworkProtocol.h"

#include <bit>
#include <limits>

namespace echoes::sim::net {
namespace {

constexpr std::uint32_t kPacketMagic = 0x50534245U;  // "EBSP" in wire order.
constexpr std::size_t kHeaderBytes = 8;
constexpr std::size_t kIntegrityBytes = 4;
constexpr std::size_t kCompatibilityPacketBytes =
    kHeaderBytes + 4 * sizeof(std::uint32_t) + sizeof(std::uint64_t) +
    4 * kDigestBytes + kIntegrityBytes;
constexpr std::size_t kCommandPacketBytes =
    kHeaderBytes + 2 * sizeof(std::uint64_t) + sizeof(std::uint8_t) +
    2 * sizeof(std::uint32_t) + 2 * sizeof(std::int32_t) +
    4 * sizeof(std::uint8_t) + kIntegrityBytes;

class Writer final {
public:
    void U8(std::uint8_t value) { bytes_.push_back(value); }

    void U16(std::uint16_t value) {
        U8(static_cast<std::uint8_t>(value));
        U8(static_cast<std::uint8_t>(value >> 8));
    }

    void U32(std::uint32_t value) {
        for (std::uint32_t shift = 0; shift < 32; shift += 8) {
            U8(static_cast<std::uint8_t>(value >> shift));
        }
    }

    void U64(std::uint64_t value) {
        for (std::uint32_t shift = 0; shift < 64; shift += 8) {
            U8(static_cast<std::uint8_t>(value >> shift));
        }
    }

    void I32(std::int32_t value) { U32(std::bit_cast<std::uint32_t>(value)); }

    void Digest(const Digest256& digest) {
        bytes_.insert(bytes_.end(), digest.begin(), digest.end());
    }

    [[nodiscard]] const std::vector<std::uint8_t>& Bytes() const {
        return bytes_;
    }

    [[nodiscard]] std::vector<std::uint8_t> Finish() && {
        return std::move(bytes_);
    }

private:
    std::vector<std::uint8_t> bytes_{};
};

class Reader final {
public:
    explicit Reader(std::span<const std::uint8_t> bytes) : bytes_(bytes) {}

    bool U8(std::uint8_t& value) {
        if (offset_ >= bytes_.size()) {
            return false;
        }
        value = bytes_[offset_++];
        return true;
    }

    bool U16(std::uint16_t& value) {
        std::uint8_t low = 0;
        std::uint8_t high = 0;
        if (!U8(low) || !U8(high)) {
            return false;
        }
        value = static_cast<std::uint16_t>(low) |
                static_cast<std::uint16_t>(high) << 8;
        return true;
    }

    bool U32(std::uint32_t& value) {
        value = 0;
        for (std::uint32_t shift = 0; shift < 32; shift += 8) {
            std::uint8_t part = 0;
            if (!U8(part)) {
                return false;
            }
            value |= static_cast<std::uint32_t>(part) << shift;
        }
        return true;
    }

    bool U64(std::uint64_t& value) {
        value = 0;
        for (std::uint32_t shift = 0; shift < 64; shift += 8) {
            std::uint8_t part = 0;
            if (!U8(part)) {
                return false;
            }
            value |= static_cast<std::uint64_t>(part) << shift;
        }
        return true;
    }

    bool I32(std::int32_t& value) {
        std::uint32_t raw = 0;
        if (!U32(raw)) {
            return false;
        }
        value = std::bit_cast<std::int32_t>(raw);
        return true;
    }

    bool Digest(Digest256& digest) {
        if (bytes_.size() - offset_ < digest.size()) {
            return false;
        }
        for (std::uint8_t& value : digest) {
            value = bytes_[offset_++];
        }
        return true;
    }

    [[nodiscard]] std::size_t Remaining() const {
        return bytes_.size() - offset_;
    }

private:
    std::span<const std::uint8_t> bytes_;
    std::size_t offset_ = 0;
};

[[nodiscard]] std::uint32_t Crc32(std::span<const std::uint8_t> bytes) {
    std::uint32_t crc = 0xffffffffU;
    for (const std::uint8_t byte : bytes) {
        crc ^= byte;
        for (std::uint8_t bit = 0; bit < 8; ++bit) {
            const std::uint32_t mask =
                0U - static_cast<std::uint32_t>(crc & 1U);
            crc = (crc >> 1) ^ (0xedb88320U & mask);
        }
    }
    return ~crc;
}

void WriteHeader(Writer& writer, PacketKind kind) {
    writer.U32(kPacketMagic);
    writer.U16(kEnvelopeVersion);
    writer.U8(static_cast<std::uint8_t>(kind));
    writer.U8(0);
}

DecodeStatus ReadHeader(Reader& reader, PacketKind expectedKind) {
    std::uint32_t magic = 0;
    std::uint16_t version = 0;
    std::uint8_t kind = 0;
    std::uint8_t reserved = 0;
    if (!reader.U32(magic) || !reader.U16(version) || !reader.U8(kind) ||
        !reader.U8(reserved)) {
        return DecodeStatus::LengthMismatch;
    }
    if (magic != kPacketMagic) {
        return DecodeStatus::BadMagic;
    }
    if (version != kEnvelopeVersion) {
        return DecodeStatus::UnsupportedEnvelopeVersion;
    }
    if (kind != static_cast<std::uint8_t>(expectedKind)) {
        return DecodeStatus::WrongPacketKind;
    }
    if (reserved != 0) {
        return DecodeStatus::ReservedFieldNonzero;
    }
    return DecodeStatus::Ok;
}

DecodeStatus ValidatePacket(std::span<const std::uint8_t> bytes,
                            std::size_t expectedBytes) {
    if (bytes.size() > kMaximumPacketBytes) {
        return DecodeStatus::PacketTooLarge;
    }
    if (bytes.size() != expectedBytes) {
        return DecodeStatus::LengthMismatch;
    }
    Reader integrityReader(bytes.last(kIntegrityBytes));
    std::uint32_t encodedCrc = 0;
    if (!integrityReader.U32(encodedCrc) ||
        encodedCrc != Crc32(bytes.first(bytes.size() - kIntegrityBytes))) {
        return DecodeStatus::IntegrityMismatch;
    }
    return DecodeStatus::Ok;
}

void AppendIntegrity(Writer& writer) {
    writer.U32(Crc32(writer.Bytes()));
}

[[nodiscard]] bool IsValidCommandTypeEncoding(std::uint8_t value) {
    return value <= static_cast<std::uint8_t>(CommandType::Research);
}

[[nodiscard]] bool IsValidEntityTypeEncoding(std::uint8_t value) {
    return value <= static_cast<std::uint8_t>(EntityType::UtilityStructure);
}

[[nodiscard]] bool IsValidWellChoiceEncoding(std::uint8_t value) {
    return value <= static_cast<std::uint8_t>(FutureWellChoice::Reshape);
}

[[nodiscard]] bool IsValidWarformAdaptationEncoding(std::uint8_t value) {
    return value <= static_cast<std::uint8_t>(WarformAdaptation::Striker);
}

[[nodiscard]] bool IsValidResearchTypeEncoding(std::uint8_t value) {
    return value <=
           static_cast<std::uint8_t>(ResearchType::KharuunAncestralEdge);
}

[[nodiscard]] bool CheckedAdd(Tick lhs, Tick rhs, Tick& result) {
    if (rhs > std::numeric_limits<Tick>::max() - lhs) {
        return false;
    }
    result = lhs + rhs;
    return true;
}

}  // namespace

std::string_view StableId(DecodeStatus status) {
    switch (status) {
        case DecodeStatus::Ok: return "NET_DECODE_OK";
        case DecodeStatus::PacketTooLarge: return "NET_PACKET_TOO_LARGE";
        case DecodeStatus::LengthMismatch: return "NET_LENGTH_MISMATCH";
        case DecodeStatus::BadMagic: return "NET_BAD_MAGIC";
        case DecodeStatus::UnsupportedEnvelopeVersion:
            return "NET_ENVELOPE_VERSION_UNSUPPORTED";
        case DecodeStatus::WrongPacketKind: return "NET_PACKET_KIND_MISMATCH";
        case DecodeStatus::ReservedFieldNonzero:
            return "NET_RESERVED_FIELD_NONZERO";
        case DecodeStatus::InvalidEncoding: return "NET_ENCODING_INVALID";
        case DecodeStatus::IntegrityMismatch: return "NET_INTEGRITY_MISMATCH";
    }
    return "NET_DECODE_UNKNOWN";
}

CompatibilityStatus CheckCompatibility(const CompatibilityManifest& authority,
                                       const CompatibilityManifest& remote) {
    if (remote.protocolVersion != authority.protocolVersion) {
        return CompatibilityStatus::ProtocolMismatch;
    }
    if (remote.snapshotVersion != authority.snapshotVersion) {
        return CompatibilityStatus::SnapshotSchemaMismatch;
    }
    if (remote.simulationRulesVersion != authority.simulationRulesVersion) {
        return CompatibilityStatus::SimulationRulesMismatch;
    }
    if (remote.playerViewSchemaVersion != authority.playerViewSchemaVersion) {
        return CompatibilityStatus::PlayerViewSchemaMismatch;
    }
    if (remote.buildIdSha256 != authority.buildIdSha256) {
        return CompatibilityStatus::BuildMismatch;
    }
    if (remote.rulesPackSha256 != authority.rulesPackSha256) {
        return CompatibilityStatus::RulesPackMismatch;
    }
    if (remote.mapPackSha256 != authority.mapPackSha256) {
        return CompatibilityStatus::MapPackMismatch;
    }
    if (remote.serializationFeatureFlags != authority.serializationFeatureFlags) {
        return CompatibilityStatus::SerializationFeaturesMismatch;
    }
    if (remote.matchSettingsSha256 != authority.matchSettingsSha256) {
        return CompatibilityStatus::MatchSettingsMismatch;
    }
    return CompatibilityStatus::Accepted;
}

std::string_view StableId(CompatibilityStatus status) {
    switch (status) {
        case CompatibilityStatus::Accepted: return "NET_COMPATIBLE";
        case CompatibilityStatus::ProtocolMismatch:
            return "NET_PROTOCOL_MISMATCH";
        case CompatibilityStatus::SnapshotSchemaMismatch:
            return "NET_SNAPSHOT_SCHEMA_MISMATCH";
        case CompatibilityStatus::SimulationRulesMismatch:
            return "NET_SIM_RULES_MISMATCH";
        case CompatibilityStatus::PlayerViewSchemaMismatch:
            return "NET_PLAYER_VIEW_SCHEMA_MISMATCH";
        case CompatibilityStatus::BuildMismatch: return "NET_BUILD_MISMATCH";
        case CompatibilityStatus::RulesPackMismatch:
            return "NET_RULES_PACK_MISMATCH";
        case CompatibilityStatus::MapPackMismatch:
            return "NET_MAP_PACK_MISMATCH";
        case CompatibilityStatus::SerializationFeaturesMismatch:
            return "NET_SERIALIZATION_FEATURES_MISMATCH";
        case CompatibilityStatus::MatchSettingsMismatch:
            return "NET_MATCH_SETTINGS_MISMATCH";
    }
    return "NET_COMPATIBILITY_UNKNOWN";
}

std::vector<std::uint8_t> EncodeCompatibilityHello(
    const CompatibilityManifest& manifest) {
    Writer writer;
    WriteHeader(writer, PacketKind::CompatibilityHello);
    writer.U32(manifest.protocolVersion);
    writer.U32(manifest.snapshotVersion);
    writer.U32(manifest.simulationRulesVersion);
    writer.U32(manifest.playerViewSchemaVersion);
    writer.U64(manifest.serializationFeatureFlags);
    writer.Digest(manifest.buildIdSha256);
    writer.Digest(manifest.rulesPackSha256);
    writer.Digest(manifest.mapPackSha256);
    writer.Digest(manifest.matchSettingsSha256);
    AppendIntegrity(writer);
    return std::move(writer).Finish();
}

DecodeStatus DecodeCompatibilityHello(std::span<const std::uint8_t> bytes,
                                      CompatibilityManifest& manifest) {
    const DecodeStatus packetStatus =
        ValidatePacket(bytes, kCompatibilityPacketBytes);
    if (packetStatus != DecodeStatus::Ok) {
        return packetStatus;
    }
    Reader reader(bytes.first(bytes.size() - kIntegrityBytes));
    const DecodeStatus headerStatus =
        ReadHeader(reader, PacketKind::CompatibilityHello);
    if (headerStatus != DecodeStatus::Ok) {
        return headerStatus;
    }
    CompatibilityManifest decoded{};
    if (!reader.U32(decoded.protocolVersion) ||
        !reader.U32(decoded.snapshotVersion) ||
        !reader.U32(decoded.simulationRulesVersion) ||
        !reader.U32(decoded.playerViewSchemaVersion) ||
        !reader.U64(decoded.serializationFeatureFlags) ||
        !reader.Digest(decoded.buildIdSha256) ||
        !reader.Digest(decoded.rulesPackSha256) ||
        !reader.Digest(decoded.mapPackSha256) ||
        !reader.Digest(decoded.matchSettingsSha256) || reader.Remaining() != 0) {
        return DecodeStatus::InvalidEncoding;
    }
    manifest = decoded;
    return DecodeStatus::Ok;
}

std::vector<std::uint8_t> EncodeCommandRequest(const CommandRequest& request) {
    Writer writer;
    WriteHeader(writer, PacketKind::CommandRequest);
    writer.U64(request.sequence);
    writer.U64(request.executeTick);
    writer.U8(static_cast<std::uint8_t>(request.type));
    writer.U32(request.actor);
    writer.U32(request.target);
    writer.I32(request.position.x.Raw());
    writer.I32(request.position.y.Raw());
    writer.U8(static_cast<std::uint8_t>(request.buildType));
    writer.U8(static_cast<std::uint8_t>(request.wellChoice));
    writer.U8(static_cast<std::uint8_t>(request.warformAdaptation));
    writer.U8(static_cast<std::uint8_t>(request.researchType));
    AppendIntegrity(writer);
    return std::move(writer).Finish();
}

DecodeStatus DecodeCommandRequest(std::span<const std::uint8_t> bytes,
                                  CommandRequest& request) {
    const DecodeStatus packetStatus = ValidatePacket(bytes, kCommandPacketBytes);
    if (packetStatus != DecodeStatus::Ok) {
        return packetStatus;
    }
    Reader reader(bytes.first(bytes.size() - kIntegrityBytes));
    const DecodeStatus headerStatus = ReadHeader(reader, PacketKind::CommandRequest);
    if (headerStatus != DecodeStatus::Ok) {
        return headerStatus;
    }

    CommandRequest decoded{};
    std::uint8_t commandType = 0;
    std::uint8_t buildType = 0;
    std::uint8_t wellChoice = 0;
    std::uint8_t adaptation = 0;
    std::uint8_t research = 0;
    std::int32_t positionX = 0;
    std::int32_t positionY = 0;
    if (!reader.U64(decoded.sequence) || !reader.U64(decoded.executeTick) ||
        !reader.U8(commandType) || !reader.U32(decoded.actor) ||
        !reader.U32(decoded.target) || !reader.I32(positionX) ||
        !reader.I32(positionY) || !reader.U8(buildType) ||
        !reader.U8(wellChoice) || !reader.U8(adaptation) ||
        !reader.U8(research) || reader.Remaining() != 0 || decoded.actor == 0 ||
        !IsValidCommandTypeEncoding(commandType) ||
        !IsValidEntityTypeEncoding(buildType) ||
        !IsValidWellChoiceEncoding(wellChoice) ||
        !IsValidWarformAdaptationEncoding(adaptation) ||
        !IsValidResearchTypeEncoding(research)) {
        return DecodeStatus::InvalidEncoding;
    }
    decoded.type = static_cast<CommandType>(commandType);
    decoded.position = Vec2::FromRaw(positionX, positionY);
    decoded.buildType = static_cast<EntityType>(buildType);
    decoded.wellChoice = static_cast<FutureWellChoice>(wellChoice);
    decoded.warformAdaptation = static_cast<WarformAdaptation>(adaptation);
    decoded.researchType = static_cast<ResearchType>(research);
    request = decoded;
    return DecodeStatus::Ok;
}

std::string_view StableId(CommandAdmissionStatus status) {
    switch (status) {
        case CommandAdmissionStatus::Accepted: return "NET_CMD_ACCEPTED";
        case CommandAdmissionStatus::InvalidSeat: return "NET_CMD_INVALID_SEAT";
        case CommandAdmissionStatus::ActorNotOwned:
            return "NET_CMD_ACTOR_NOT_OWNED";
        case CommandAdmissionStatus::SequenceUnexpected:
            return "NET_CMD_SEQUENCE_UNEXPECTED";
        case CommandAdmissionStatus::TickTooEarly:
            return "NET_CMD_TICK_TOO_EARLY";
        case CommandAdmissionStatus::TickTooLate: return "NET_CMD_TICK_TOO_LATE";
        case CommandAdmissionStatus::TickRangeInvalid:
            return "NET_CMD_TICK_RANGE_INVALID";
        case CommandAdmissionStatus::CommandRejected:
            return "NET_CMD_SIMULATION_REJECTED";
    }
    return "NET_CMD_ADMISSION_UNKNOWN";
}

CommandAdmissionStatus AdmitCommandRequest(
    const CommandRequest& request,
    CommandAdmissionContext& context,
    Simulation& simulation,
    std::string* simulationRejection) {
    if (simulationRejection != nullptr) {
        simulationRejection->clear();
    }
    if (simulation.FindPlayer(context.player) == nullptr) {
        return CommandAdmissionStatus::InvalidSeat;
    }
    const Entity* actor = simulation.FindEntity(request.actor);
    if (actor == nullptr || actor->owner != context.player) {
        return CommandAdmissionStatus::ActorNotOwned;
    }
    std::uint64_t expectedSequence = 1;
    if (context.hasAcceptedSequence) {
        expectedSequence =
            context.lastAcceptedSequence ==
                    std::numeric_limits<std::uint64_t>::max()
                ? 0
                : context.lastAcceptedSequence + 1;
    }
    if (expectedSequence == 0 || request.sequence != expectedSequence) {
        return CommandAdmissionStatus::SequenceUnexpected;
    }

    Tick earliestTick = 0;
    Tick latestTick = 0;
    if (context.minimumInputDelayTicks > context.maximumLeadTicks ||
        !CheckedAdd(simulation.CurrentTick(), context.minimumInputDelayTicks,
                    earliestTick) ||
        !CheckedAdd(simulation.CurrentTick(), context.maximumLeadTicks,
                    latestTick)) {
        return CommandAdmissionStatus::TickRangeInvalid;
    }
    if (request.executeTick < earliestTick) {
        return CommandAdmissionStatus::TickTooEarly;
    }
    if (request.executeTick > latestTick) {
        return CommandAdmissionStatus::TickTooLate;
    }

    Command command{};
    command.executeTick = request.executeTick;
    command.player = context.player;
    command.sequence = request.sequence;
    command.type = request.type;
    command.actor = request.actor;
    command.target = request.target;
    command.position = request.position;
    command.buildType = request.buildType;
    command.wellChoice = request.wellChoice;
    command.warformAdaptation = request.warformAdaptation;
    command.researchType = request.researchType;
    if (!simulation.QueueCommand(command, simulationRejection)) {
        return CommandAdmissionStatus::CommandRejected;
    }

    context.hasAcceptedSequence = true;
    context.lastAcceptedSequence = request.sequence;
    return CommandAdmissionStatus::Accepted;
}

}  // namespace echoes::sim::net
