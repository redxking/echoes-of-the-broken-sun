#include "EchoesSimCore/NetworkProtocol.h"

#include <algorithm>
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
constexpr std::size_t kCommandIntentBytes =
    sizeof(std::uint8_t) + 2 * sizeof(std::uint32_t) +
    2 * sizeof(std::int32_t) + 4 * sizeof(std::uint8_t);
constexpr std::size_t kCommandBatchMinimumBytes =
    kHeaderBytes + sizeof(std::uint64_t) + 2 * sizeof(std::uint16_t) +
    kIntegrityBytes;
constexpr std::size_t kScopedEntityBytes = 52;
constexpr std::size_t kScopedKeyframeMinimumBytes = 90;
constexpr std::size_t kScopedTileChangeBytes = 5;
constexpr std::size_t kScopedDeltaMinimumBytes = 102;

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

[[nodiscard]] std::uint64_t Fnv1a64(std::span<const std::uint8_t> bytes) {
    std::uint64_t digest = 14695981039346656037ULL;
    for (const std::uint8_t byte : bytes) {
        digest ^= byte;
        digest *= 1099511628211ULL;
    }
    return digest;
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
                            std::size_t expectedBytes,
                            std::size_t maximumBytes = kMaximumPacketBytes) {
    if (bytes.size() > maximumBytes) {
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

DecodeStatus ValidateVariablePacket(std::span<const std::uint8_t> bytes,
                                    std::size_t minimumBytes,
                                    std::size_t maximumBytes) {
    if (bytes.size() > maximumBytes) {
        return DecodeStatus::PacketTooLarge;
    }
    if (bytes.size() < minimumBytes) {
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
    return value <= static_cast<std::uint8_t>(CommandType::ReconcileToPossible);
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
           static_cast<std::uint8_t>(ResearchType::ChoirSharedResolution);
}

[[nodiscard]] bool IsValidFactionEncoding(std::uint8_t value) {
    return value <= static_cast<std::uint8_t>(Faction::HollowChoir);
}

[[nodiscard]] bool IsValidChoirIdentityEncoding(std::uint8_t value) {
    return value <=
           static_cast<std::uint8_t>(ChoirIdentityState::DualResolvePossible);
}

[[nodiscard]] bool IsValidVisibilityEncoding(std::uint8_t value) {
    return value <= static_cast<std::uint8_t>(Visibility::Visible);
}

[[nodiscard]] bool IsValidTerrainEncoding(std::uint8_t value) {
    return value <= static_cast<std::uint8_t>(Terrain::Scarred);
}

[[nodiscard]] bool IsValidWaystoneModeEncoding(std::uint8_t value) {
    return value <= static_cast<std::uint8_t>(WaystoneMode::Rooting);
}

[[nodiscard]] bool IsValidScopedPlayer(PlayerId player) {
    return player < kMaximumPlayers;
}

void WriteCommandIntent(Writer& writer, const CommandIntent& intent) {
    writer.U8(static_cast<std::uint8_t>(intent.type));
    writer.U32(intent.actor);
    writer.U32(intent.target);
    writer.I32(intent.position.x.Raw());
    writer.I32(intent.position.y.Raw());
    writer.U8(static_cast<std::uint8_t>(intent.buildType));
    writer.U8(static_cast<std::uint8_t>(intent.wellChoice));
    writer.U8(static_cast<std::uint8_t>(intent.warformAdaptation));
    writer.U8(static_cast<std::uint8_t>(intent.researchType));
}

[[nodiscard]] bool ReadCommandIntent(Reader& reader, CommandIntent& intent) {
    std::uint8_t commandType = 0;
    std::uint8_t buildType = 0;
    std::uint8_t wellChoice = 0;
    std::uint8_t adaptation = 0;
    std::uint8_t research = 0;
    std::int32_t positionX = 0;
    std::int32_t positionY = 0;
    if (!reader.U8(commandType) || !reader.U32(intent.actor) ||
        !reader.U32(intent.target) || !reader.I32(positionX) ||
        !reader.I32(positionY) || !reader.U8(buildType) ||
        !reader.U8(wellChoice) || !reader.U8(adaptation) ||
        !reader.U8(research) || intent.actor == 0 ||
        !IsValidCommandTypeEncoding(commandType) ||
        !IsValidEntityTypeEncoding(buildType) ||
        !IsValidWellChoiceEncoding(wellChoice) ||
        !IsValidWarformAdaptationEncoding(adaptation) ||
        !IsValidResearchTypeEncoding(research)) {
        return false;
    }
    intent.type = static_cast<CommandType>(commandType);
    intent.position = Vec2::FromRaw(positionX, positionY);
    intent.buildType = static_cast<EntityType>(buildType);
    intent.wellChoice = static_cast<FutureWellChoice>(wellChoice);
    intent.warformAdaptation = static_cast<WarformAdaptation>(adaptation);
    intent.researchType = static_cast<ResearchType>(research);
    return true;
}

[[nodiscard]] bool IsValidScopedOwner(PlayerId player) {
    return IsValidScopedPlayer(player) || player == kNeutralPlayer;
}

[[nodiscard]] bool IsValidScopedChoirState(const ScopedEntityState& entity,
                                           Tick simulationTick) {
    const bool identityUnit = entity.faction == Faction::HollowChoir &&
        (entity.type == EntityType::Soldier ||
         entity.type == EntityType::HeavyUnit ||
         entity.type == EntityType::ScoutUnit);
    const bool resolving =
        entity.choirIdentityState ==
            ChoirIdentityState::DualResolveManifest ||
        entity.choirIdentityState ==
            ChoirIdentityState::DualResolvePossible;
    if (!IsValidChoirIdentityEncoding(
            static_cast<std::uint8_t>(entity.choirIdentityState)) ||
        (entity.choirIdentityState != ChoirIdentityState::NotChoir &&
         !identityUnit) ||
        (entity.choirIdentityState == ChoirIdentityState::NotChoir &&
         (entity.choirIdentityResolveAtTick != 0 ||
          entity.choirIdentityNextAvailableTick != 0)) ||
        (resolving &&
         (entity.choirIdentityResolveAtTick <= simulationTick ||
          entity.choirIdentityNextAvailableTick <
              entity.choirIdentityResolveAtTick)) ||
        (!resolving && entity.choirIdentityResolveAtTick != 0)) {
        return false;
    }
    if (entity.choirCoherenceNextChargeTick == 0) {
        return true;
    }
    return entity.owner != kNeutralPlayer &&
           entity.faction == Faction::HollowChoir &&
           (entity.type == EntityType::Dropoff ||
            entity.type == EntityType::Barracks ||
            entity.type == EntityType::UtilityStructure);
}

[[nodiscard]] bool CheckedMultiplySize(std::size_t lhs,
                                       std::size_t rhs,
                                       std::size_t& result) {
    if (lhs != 0 && rhs > std::numeric_limits<std::size_t>::max() / lhs) {
        return false;
    }
    result = lhs * rhs;
    return true;
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

std::vector<std::uint8_t> EncodeCommandBatchRequest(
    const CommandBatchRequest& request) {
    if (request.clientBatchId == 0 || request.intents.empty() ||
        request.intents.size() > kMaximumCommandsPerBatch) {
        return {};
    }
    EntityId priorActor = 0;
    for (const CommandIntent& intent : request.intents) {
        if (intent.actor <= priorActor ||
            !IsValidCommandTypeEncoding(
                static_cast<std::uint8_t>(intent.type)) ||
            !IsValidEntityTypeEncoding(
                static_cast<std::uint8_t>(intent.buildType)) ||
            !IsValidWellChoiceEncoding(
                static_cast<std::uint8_t>(intent.wellChoice)) ||
            !IsValidWarformAdaptationEncoding(
                static_cast<std::uint8_t>(intent.warformAdaptation)) ||
            !IsValidResearchTypeEncoding(
                static_cast<std::uint8_t>(intent.researchType))) {
            return {};
        }
        priorActor = intent.actor;
    }
    Writer writer;
    WriteHeader(writer, PacketKind::CommandBatchRequest);
    writer.U64(request.clientBatchId);
    writer.U16(static_cast<std::uint16_t>(request.intents.size()));
    writer.U16(0);
    for (const CommandIntent& intent : request.intents) {
        WriteCommandIntent(writer, intent);
    }
    AppendIntegrity(writer);
    std::vector<std::uint8_t> encoded = std::move(writer).Finish();
    return encoded.size() <= kMaximumCommandBatchBytes
               ? std::move(encoded)
               : std::vector<std::uint8_t>{};
}

DecodeStatus DecodeCommandBatchRequest(
    std::span<const std::uint8_t> bytes,
    CommandBatchRequest& request) {
    const DecodeStatus packetStatus = ValidateVariablePacket(
        bytes, kCommandBatchMinimumBytes, kMaximumCommandBatchBytes);
    if (packetStatus != DecodeStatus::Ok) {
        return packetStatus;
    }
    Reader reader(bytes.first(bytes.size() - kIntegrityBytes));
    const DecodeStatus headerStatus =
        ReadHeader(reader, PacketKind::CommandBatchRequest);
    if (headerStatus != DecodeStatus::Ok) {
        return headerStatus;
    }
    CommandBatchRequest decoded{};
    std::uint16_t intentCount = 0;
    std::uint16_t reserved = 0;
    std::size_t intentBytes = 0;
    if (!reader.U64(decoded.clientBatchId) || !reader.U16(intentCount) ||
        !reader.U16(reserved) || decoded.clientBatchId == 0 || reserved != 0 ||
        intentCount == 0 || intentCount > kMaximumCommandsPerBatch ||
        !CheckedMultiplySize(intentCount, kCommandIntentBytes, intentBytes) ||
        reader.Remaining() != intentBytes) {
        return DecodeStatus::InvalidEncoding;
    }
    decoded.intents.reserve(intentCount);
    EntityId priorActor = 0;
    for (std::uint16_t index = 0; index < intentCount; ++index) {
        CommandIntent intent{};
        if (!ReadCommandIntent(reader, intent) || intent.actor <= priorActor) {
            return DecodeStatus::InvalidEncoding;
        }
        priorActor = intent.actor;
        decoded.intents.push_back(intent);
    }
    if (reader.Remaining() != 0) {
        return DecodeStatus::InvalidEncoding;
    }
    request = std::move(decoded);
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

bool BuildScopedViewKeyframe(const PlayerView& view,
                             std::uint64_t snapshotId,
                             std::uint64_t lastAcceptedSequence,
                             ScopedViewKeyframe& keyframe,
                             std::string* error) {
    if (error != nullptr) {
        error->clear();
    }
    const SimulationConfig& config = view.Config();
    if (snapshotId == 0 || config.mapWidthTiles <= 0 ||
        config.mapHeightTiles <= 0 ||
        config.mapWidthTiles > 256 || config.mapHeightTiles > 256 ||
        !IsValidScopedPlayer(view.Player().id) ||
        !IsValidFactionEncoding(
            static_cast<std::uint8_t>(view.Player().faction))) {
        if (error != nullptr) {
            *error = "NET_SCOPED_VIEW_INVALID";
        }
        return false;
    }
    std::size_t tileCount = 0;
    if (!CheckedMultiplySize(
            static_cast<std::size_t>(config.mapWidthTiles),
            static_cast<std::size_t>(config.mapHeightTiles), tileCount) ||
        tileCount > kMaximumScopedTiles ||
        view.Entities().size() > kMaximumScopedEntities ||
        view.VibrationSignatures().size() > kMaximumVibrationSignatures) {
        if (error != nullptr) {
            *error = "NET_SCOPED_VIEW_CAPACITY";
        }
        return false;
    }

    ScopedViewKeyframe built{};
    built.snapshotId = snapshotId;
    built.simulationTick = view.CurrentTick();
    built.lastAcceptedSequence = lastAcceptedSequence;
    built.mapWidthTiles = config.mapWidthTiles;
    built.mapHeightTiles = config.mapHeightTiles;
    built.player = view.Player().id;
    built.faction = view.Player().faction;
    built.resources = view.Player().resources;
    built.populationUsed = view.PopulationUsed();
    built.populationCapacity = view.PopulationCapacity();
    built.tiles.reserve(tileCount);
    for (std::int32_t y = 0; y < config.mapHeightTiles; ++y) {
        for (std::int32_t x = 0; x < config.mapWidthTiles; ++x) {
            const Vec2 position = Vec2::FromTiles(x, y);
            const Visibility visibility = view.VisibilityAt(position);
            const Terrain terrain = view.TerrainAt(x, y);
            built.tiles.push_back(
                {visibility, terrain, view.IsPositionPassable(position)});
        }
    }
    built.entities.reserve(view.Entities().size());
    for (const Entity& entity : view.Entities()) {
        built.entities.push_back(
            {entity.id,
             entity.owner,
             entity.faction,
             entity.type,
             entity.position,
             entity.hitPoints,
             entity.maxHitPoints,
             entity.completed,
             entity.wellChoice,
             entity.deployed,
             entity.waystoneMode,
             entity.warformAdaptation,
             entity.aegisPowered,
             entity.choirIdentityState,
             entity.choirIdentityResolveAtTick,
             entity.choirIdentityNextAvailableTick,
             entity.choirCoherenceNextChargeTick});
    }
    std::sort(
        built.entities.begin(), built.entities.end(),
        [](const ScopedEntityState& lhs, const ScopedEntityState& rhs) {
            return lhs.id < rhs.id;
        });
    built.vibrationSignatures = view.VibrationSignatures();

    const std::vector<std::uint8_t> encoded = EncodeScopedViewKeyframe(built);
    ScopedViewKeyframe finalized{};
    if (encoded.empty() ||
        DecodeScopedViewKeyframe(encoded, finalized) != DecodeStatus::Ok) {
        if (error != nullptr) {
            *error = "NET_SCOPED_VIEW_ENCODING_FAILED";
        }
        return false;
    }
    keyframe = std::move(finalized);
    return true;
}

std::vector<std::uint8_t> EncodeScopedViewKeyframe(
    const ScopedViewKeyframe& keyframe) {
    if (keyframe.snapshotId == 0 || keyframe.mapWidthTiles <= 0 ||
        keyframe.mapHeightTiles <= 0 || keyframe.mapWidthTiles > 256 ||
        keyframe.mapHeightTiles > 256 ||
        !IsValidScopedPlayer(keyframe.player) ||
        !IsValidFactionEncoding(static_cast<std::uint8_t>(keyframe.faction)) ||
        keyframe.resources.material < 0 || keyframe.resources.dawnshards < 0 ||
        keyframe.populationUsed < 0 || keyframe.populationCapacity < 0 ||
        keyframe.entities.size() > kMaximumScopedEntities ||
        keyframe.vibrationSignatures.size() >
            kMaximumVibrationSignatures) {
        return {};
    }
    std::size_t expectedTiles = 0;
    if (!CheckedMultiplySize(
            static_cast<std::size_t>(keyframe.mapWidthTiles),
            static_cast<std::size_t>(keyframe.mapHeightTiles), expectedTiles) ||
        expectedTiles != keyframe.tiles.size() ||
        expectedTiles > kMaximumScopedTiles) {
        return {};
    }

    Writer writer;
    WriteHeader(writer, PacketKind::ScopedViewKeyframe);
    writer.U32(keyframe.protocolVersion);
    writer.U64(keyframe.snapshotId);
    writer.U64(keyframe.simulationTick);
    writer.U32(keyframe.playerViewSchemaVersion);
    writer.U64(keyframe.lastAcceptedSequence);
    writer.I32(keyframe.mapWidthTiles);
    writer.I32(keyframe.mapHeightTiles);
    writer.U8(keyframe.player);
    writer.U8(static_cast<std::uint8_t>(keyframe.faction));
    writer.I32(keyframe.resources.material);
    writer.I32(keyframe.resources.dawnshards);
    writer.I32(keyframe.populationUsed);
    writer.I32(keyframe.populationCapacity);
    writer.U32(static_cast<std::uint32_t>(keyframe.tiles.size()));
    writer.U32(static_cast<std::uint32_t>(keyframe.entities.size()));
    writer.U32(static_cast<std::uint32_t>(
        keyframe.vibrationSignatures.size()));
    for (const ScopedTileState& tile : keyframe.tiles) {
        if (!IsValidVisibilityEncoding(
                static_cast<std::uint8_t>(tile.visibility)) ||
            !IsValidTerrainEncoding(static_cast<std::uint8_t>(tile.terrain)) ||
            (tile.visibility == Visibility::Unexplored &&
             (tile.terrain != Terrain::Blocked || tile.passable))) {
            return {};
        }
        writer.U8(
            static_cast<std::uint8_t>(tile.visibility) |
            (static_cast<std::uint8_t>(tile.terrain) << 2) |
            (tile.passable ? 0x10U : 0U));
    }
    EntityId priorEntity = 0;
    for (const ScopedEntityState& entity : keyframe.entities) {
        if (entity.id == 0 || entity.id <= priorEntity ||
            !IsValidScopedOwner(entity.owner) ||
            !IsValidFactionEncoding(static_cast<std::uint8_t>(entity.faction)) ||
            !IsValidEntityTypeEncoding(static_cast<std::uint8_t>(entity.type)) ||
            entity.hitPoints <= 0 || entity.maxHitPoints <= 0 ||
            entity.hitPoints > entity.maxHitPoints ||
            !IsValidWellChoiceEncoding(
                static_cast<std::uint8_t>(entity.wellChoice)) ||
            !IsValidWaystoneModeEncoding(
                static_cast<std::uint8_t>(entity.waystoneMode)) ||
            !IsValidWarformAdaptationEncoding(
                static_cast<std::uint8_t>(entity.warformAdaptation)) ||
            !IsValidScopedChoirState(entity, keyframe.simulationTick)) {
            return {};
        }
        priorEntity = entity.id;
        writer.U32(entity.id);
        writer.U8(entity.owner);
        writer.U8(static_cast<std::uint8_t>(entity.faction));
        writer.U8(static_cast<std::uint8_t>(entity.type));
        writer.I32(entity.position.x.Raw());
        writer.I32(entity.position.y.Raw());
        writer.I32(entity.hitPoints);
        writer.I32(entity.maxHitPoints);
        writer.U8((entity.completed ? 0x01U : 0U) |
                  (entity.deployed ? 0x02U : 0U) |
                  (entity.aegisPowered ? 0x04U : 0U));
        writer.U8(static_cast<std::uint8_t>(entity.wellChoice));
        writer.U8(static_cast<std::uint8_t>(entity.waystoneMode));
        writer.U8(static_cast<std::uint8_t>(entity.warformAdaptation));
        writer.U8(static_cast<std::uint8_t>(entity.choirIdentityState));
        writer.U64(entity.choirIdentityResolveAtTick);
        writer.U64(entity.choirIdentityNextAvailableTick);
        writer.U64(entity.choirCoherenceNextChargeTick);
    }
    Vec2 priorSignature = Vec2::FromRaw(-1, -1);
    for (const VibrationSignature& signature :
         keyframe.vibrationSignatures) {
        const Vec2 position = signature.approximatePosition;
        if (position.x.Raw() < 0 || position.y.Raw() < 0 ||
            position.x.Raw() >= keyframe.mapWidthTiles * kFixedScale ||
            position.y.Raw() >= keyframe.mapHeightTiles * kFixedScale ||
            (priorSignature.x.Raw() >= 0 &&
             (position.x.Raw() < priorSignature.x.Raw() ||
              (position.x.Raw() == priorSignature.x.Raw() &&
               position.y.Raw() <= priorSignature.y.Raw())))) {
            return {};
        }
        priorSignature = position;
        writer.I32(position.x.Raw());
        writer.I32(position.y.Raw());
    }
    const std::uint64_t digest = Fnv1a64(writer.Bytes());
    writer.U64(digest);
    AppendIntegrity(writer);
    std::vector<std::uint8_t> encoded = std::move(writer).Finish();
    if (encoded.size() > kMaximumScopedKeyframeBytes) {
        return {};
    }
    return encoded;
}

DecodeStatus DecodeScopedViewKeyframe(std::span<const std::uint8_t> bytes,
                                      ScopedViewKeyframe& keyframe) {
    const DecodeStatus packetStatus = ValidateVariablePacket(
        bytes, kScopedKeyframeMinimumBytes, kMaximumScopedKeyframeBytes);
    if (packetStatus != DecodeStatus::Ok) {
        return packetStatus;
    }
    Reader reader(bytes.first(bytes.size() - kIntegrityBytes));
    const DecodeStatus headerStatus =
        ReadHeader(reader, PacketKind::ScopedViewKeyframe);
    if (headerStatus != DecodeStatus::Ok) {
        return headerStatus;
    }

    ScopedViewKeyframe decoded{};
    std::uint8_t faction = 0;
    std::uint32_t tileCount = 0;
    std::uint32_t entityCount = 0;
    std::uint32_t signatureCount = 0;
    if (!reader.U32(decoded.protocolVersion) ||
        !reader.U64(decoded.snapshotId) ||
        !reader.U64(decoded.simulationTick) ||
        !reader.U32(decoded.playerViewSchemaVersion) ||
        !reader.U64(decoded.lastAcceptedSequence) ||
        !reader.I32(decoded.mapWidthTiles) ||
        !reader.I32(decoded.mapHeightTiles) || !reader.U8(decoded.player) ||
        !reader.U8(faction) || !reader.I32(decoded.resources.material) ||
        !reader.I32(decoded.resources.dawnshards) ||
        !reader.I32(decoded.populationUsed) ||
        !reader.I32(decoded.populationCapacity) || !reader.U32(tileCount) ||
        !reader.U32(entityCount) || !reader.U32(signatureCount)) {
        return DecodeStatus::InvalidEncoding;
    }
    decoded.faction = static_cast<Faction>(faction);
    std::size_t expectedTiles = 0;
    std::size_t entityBytes = 0;
    std::size_t signatureBytes = 0;
    if (decoded.protocolVersion != kProtocolVersion ||
        decoded.playerViewSchemaVersion != kPlayerViewSchemaVersion ||
        decoded.snapshotId == 0 || decoded.mapWidthTiles <= 0 ||
        decoded.mapHeightTiles <= 0 || decoded.mapWidthTiles > 256 ||
        decoded.mapHeightTiles > 256 ||
        !IsValidScopedPlayer(decoded.player) ||
        !IsValidFactionEncoding(faction) || decoded.resources.material < 0 ||
        decoded.resources.dawnshards < 0 || decoded.populationUsed < 0 ||
        decoded.populationCapacity < 0 ||
        !CheckedMultiplySize(
            static_cast<std::size_t>(decoded.mapWidthTiles),
            static_cast<std::size_t>(decoded.mapHeightTiles), expectedTiles) ||
        expectedTiles != tileCount || tileCount > kMaximumScopedTiles ||
        entityCount > kMaximumScopedEntities ||
        signatureCount > kMaximumVibrationSignatures ||
        !CheckedMultiplySize(entityCount, kScopedEntityBytes, entityBytes) ||
        !CheckedMultiplySize(signatureCount, 8, signatureBytes) ||
        reader.Remaining() != tileCount + entityBytes + signatureBytes + 8) {
        return DecodeStatus::InvalidEncoding;
    }

    decoded.tiles.reserve(tileCount);
    for (std::uint32_t index = 0; index < tileCount; ++index) {
        std::uint8_t packed = 0;
        if (!reader.U8(packed) || (packed & 0xe0U) != 0 ||
            !IsValidVisibilityEncoding(packed & 0x03U) ||
            !IsValidTerrainEncoding((packed >> 2) & 0x03U)) {
            return DecodeStatus::InvalidEncoding;
        }
        const ScopedTileState tile{
            static_cast<Visibility>(packed & 0x03U),
            static_cast<Terrain>((packed >> 2) & 0x03U),
            (packed & 0x10U) != 0};
        if (tile.visibility == Visibility::Unexplored &&
            (tile.terrain != Terrain::Blocked || tile.passable)) {
            return DecodeStatus::InvalidEncoding;
        }
        decoded.tiles.push_back(tile);
    }

    decoded.entities.reserve(entityCount);
    EntityId priorEntity = 0;
    for (std::uint32_t index = 0; index < entityCount; ++index) {
        ScopedEntityState entity{};
        std::uint8_t factionValue = 0;
        std::uint8_t type = 0;
        std::uint8_t flags = 0;
        std::uint8_t wellChoice = 0;
        std::uint8_t waystoneMode = 0;
        std::uint8_t adaptation = 0;
        std::uint8_t choirIdentity = 0;
        std::int32_t positionX = 0;
        std::int32_t positionY = 0;
        if (!reader.U32(entity.id) || !reader.U8(entity.owner) ||
            !reader.U8(factionValue) || !reader.U8(type) ||
            !reader.I32(positionX) || !reader.I32(positionY) ||
            !reader.I32(entity.hitPoints) ||
            !reader.I32(entity.maxHitPoints) || !reader.U8(flags) ||
            !reader.U8(wellChoice) || !reader.U8(waystoneMode) ||
            !reader.U8(adaptation) || !reader.U8(choirIdentity) ||
            !reader.U64(entity.choirIdentityResolveAtTick) ||
            !reader.U64(entity.choirIdentityNextAvailableTick) ||
            !reader.U64(entity.choirCoherenceNextChargeTick) || entity.id == 0 ||
            entity.id <= priorEntity || !IsValidScopedOwner(entity.owner) ||
            !IsValidFactionEncoding(factionValue) ||
            !IsValidEntityTypeEncoding(type) || (flags & 0xf8U) != 0 ||
            entity.hitPoints <= 0 || entity.maxHitPoints <= 0 ||
            entity.hitPoints > entity.maxHitPoints ||
            !IsValidWellChoiceEncoding(wellChoice) ||
            !IsValidWaystoneModeEncoding(waystoneMode) ||
            !IsValidWarformAdaptationEncoding(adaptation) ||
            !IsValidChoirIdentityEncoding(choirIdentity)) {
            return DecodeStatus::InvalidEncoding;
        }
        priorEntity = entity.id;
        entity.faction = static_cast<Faction>(factionValue);
        entity.type = static_cast<EntityType>(type);
        entity.position = Vec2::FromRaw(positionX, positionY);
        entity.completed = (flags & 0x01U) != 0;
        entity.deployed = (flags & 0x02U) != 0;
        entity.aegisPowered = (flags & 0x04U) != 0;
        entity.wellChoice = static_cast<FutureWellChoice>(wellChoice);
        entity.waystoneMode = static_cast<WaystoneMode>(waystoneMode);
        entity.warformAdaptation =
            static_cast<WarformAdaptation>(adaptation);
        entity.choirIdentityState =
            static_cast<ChoirIdentityState>(choirIdentity);
        if (!IsValidScopedChoirState(entity, decoded.simulationTick)) {
            return DecodeStatus::InvalidEncoding;
        }
        decoded.entities.push_back(entity);
    }

    decoded.vibrationSignatures.reserve(signatureCount);
    Vec2 priorSignature = Vec2::FromRaw(-1, -1);
    for (std::uint32_t index = 0; index < signatureCount; ++index) {
        std::int32_t x = 0;
        std::int32_t y = 0;
        if (!reader.I32(x) || !reader.I32(y) || x < 0 || y < 0 ||
            x >= decoded.mapWidthTiles * kFixedScale ||
            y >= decoded.mapHeightTiles * kFixedScale ||
            (priorSignature.x.Raw() >= 0 &&
             (x < priorSignature.x.Raw() ||
              (x == priorSignature.x.Raw() &&
               y <= priorSignature.y.Raw())))) {
            return DecodeStatus::InvalidEncoding;
        }
        priorSignature = Vec2::FromRaw(x, y);
        decoded.vibrationSignatures.push_back({priorSignature});
    }

    const std::size_t digestOffset = bytes.size() - kIntegrityBytes - 8;
    if (!reader.U64(decoded.scopedDigest) || reader.Remaining() != 0 ||
        decoded.scopedDigest != Fnv1a64(bytes.first(digestOffset))) {
        return DecodeStatus::IntegrityMismatch;
    }
    keyframe = std::move(decoded);
    return DecodeStatus::Ok;
}

bool BuildScopedViewDelta(const ScopedViewKeyframe& base,
                          const ScopedViewKeyframe& current,
                          ScopedViewDelta& delta,
                          std::string* error) {
    if (error != nullptr) {
        error->clear();
    }
    ScopedViewKeyframe verifiedBase{};
    ScopedViewKeyframe verifiedCurrent{};
    const std::vector<std::uint8_t> baseBytes =
        EncodeScopedViewKeyframe(base);
    const std::vector<std::uint8_t> currentBytes =
        EncodeScopedViewKeyframe(current);
    if (baseBytes.empty() || currentBytes.empty() ||
        DecodeScopedViewKeyframe(baseBytes, verifiedBase) != DecodeStatus::Ok ||
        DecodeScopedViewKeyframe(currentBytes, verifiedCurrent) !=
            DecodeStatus::Ok ||
        verifiedBase != base || verifiedCurrent != current ||
        current.snapshotId <= base.snapshotId ||
        current.simulationTick < base.simulationTick ||
        current.protocolVersion != base.protocolVersion ||
        current.playerViewSchemaVersion != base.playerViewSchemaVersion ||
        current.mapWidthTiles != base.mapWidthTiles ||
        current.mapHeightTiles != base.mapHeightTiles ||
        current.player != base.player || current.faction != base.faction) {
        if (error != nullptr) {
            *error = "NET_DELTA_BASE_INCOMPATIBLE";
        }
        return false;
    }

    ScopedViewDelta built{};
    built.snapshotId = current.snapshotId;
    built.baseSnapshotId = base.snapshotId;
    built.simulationTick = current.simulationTick;
    built.lastAcceptedSequence = current.lastAcceptedSequence;
    built.mapWidthTiles = current.mapWidthTiles;
    built.mapHeightTiles = current.mapHeightTiles;
    built.player = current.player;
    built.faction = current.faction;
    built.resources = current.resources;
    built.populationUsed = current.populationUsed;
    built.populationCapacity = current.populationCapacity;
    built.vibrationSignatures = current.vibrationSignatures;
    built.scopedDigest = current.scopedDigest;

    for (std::size_t index = 0; index < current.tiles.size(); ++index) {
        if (current.tiles[index] != base.tiles[index]) {
            built.tileChanges.push_back(
                {static_cast<std::uint32_t>(index), current.tiles[index]});
        }
    }

    std::size_t baseIndex = 0;
    std::size_t currentIndex = 0;
    while (baseIndex < base.entities.size() ||
           currentIndex < current.entities.size()) {
        if (currentIndex >= current.entities.size() ||
            (baseIndex < base.entities.size() &&
             base.entities[baseIndex].id < current.entities[currentIndex].id)) {
            built.removedEntityIds.push_back(base.entities[baseIndex].id);
            ++baseIndex;
        } else if (baseIndex >= base.entities.size() ||
                   current.entities[currentIndex].id <
                       base.entities[baseIndex].id) {
            built.entityUpserts.push_back(current.entities[currentIndex]);
            ++currentIndex;
        } else {
            if (base.entities[baseIndex] != current.entities[currentIndex]) {
                built.entityUpserts.push_back(current.entities[currentIndex]);
            }
            ++baseIndex;
            ++currentIndex;
        }
    }

    const std::vector<std::uint8_t> encoded = EncodeScopedViewDelta(built);
    ScopedViewDelta finalized{};
    if (encoded.empty() ||
        DecodeScopedViewDelta(encoded, finalized) != DecodeStatus::Ok) {
        if (error != nullptr) {
            *error = "NET_DELTA_ENCODING_FAILED";
        }
        return false;
    }
    delta = std::move(finalized);
    return true;
}

std::vector<std::uint8_t> EncodeScopedViewDelta(
    const ScopedViewDelta& delta) {
    std::size_t tileCount = 0;
    if (delta.protocolVersion != kProtocolVersion ||
        delta.playerViewSchemaVersion != kPlayerViewSchemaVersion ||
        delta.snapshotId == 0 || delta.baseSnapshotId == 0 ||
        delta.snapshotId <= delta.baseSnapshotId ||
        delta.mapWidthTiles <= 0 || delta.mapHeightTiles <= 0 ||
        delta.mapWidthTiles > 256 || delta.mapHeightTiles > 256 ||
        !IsValidScopedPlayer(delta.player) ||
        !IsValidFactionEncoding(static_cast<std::uint8_t>(delta.faction)) ||
        delta.resources.material < 0 || delta.resources.dawnshards < 0 ||
        delta.populationUsed < 0 || delta.populationCapacity < 0 ||
        delta.scopedDigest == 0 ||
        delta.entityUpserts.size() > kMaximumScopedEntities ||
        delta.removedEntityIds.size() > kMaximumScopedEntities ||
        delta.vibrationSignatures.size() > kMaximumVibrationSignatures ||
        !CheckedMultiplySize(
            static_cast<std::size_t>(delta.mapWidthTiles),
            static_cast<std::size_t>(delta.mapHeightTiles), tileCount) ||
        tileCount > kMaximumScopedTiles ||
        delta.tileChanges.size() > tileCount) {
        return {};
    }

    Writer writer;
    WriteHeader(writer, PacketKind::ScopedViewDelta);
    writer.U32(delta.protocolVersion);
    writer.U64(delta.snapshotId);
    writer.U64(delta.baseSnapshotId);
    writer.U64(delta.simulationTick);
    writer.U32(delta.playerViewSchemaVersion);
    writer.U64(delta.lastAcceptedSequence);
    writer.I32(delta.mapWidthTiles);
    writer.I32(delta.mapHeightTiles);
    writer.U8(delta.player);
    writer.U8(static_cast<std::uint8_t>(delta.faction));
    writer.I32(delta.resources.material);
    writer.I32(delta.resources.dawnshards);
    writer.I32(delta.populationUsed);
    writer.I32(delta.populationCapacity);
    writer.U32(static_cast<std::uint32_t>(delta.tileChanges.size()));
    writer.U32(static_cast<std::uint32_t>(delta.entityUpserts.size()));
    writer.U32(static_cast<std::uint32_t>(delta.removedEntityIds.size()));
    writer.U32(static_cast<std::uint32_t>(
        delta.vibrationSignatures.size()));

    std::uint32_t priorTileIndex = 0;
    bool hasPriorTile = false;
    for (const ScopedTileChange& change : delta.tileChanges) {
        const ScopedTileState& tile = change.state;
        if (change.index >= tileCount ||
            (hasPriorTile && change.index <= priorTileIndex) ||
            !IsValidVisibilityEncoding(
                static_cast<std::uint8_t>(tile.visibility)) ||
            !IsValidTerrainEncoding(static_cast<std::uint8_t>(tile.terrain)) ||
            (tile.visibility == Visibility::Unexplored &&
             (tile.terrain != Terrain::Blocked || tile.passable))) {
            return {};
        }
        priorTileIndex = change.index;
        hasPriorTile = true;
        writer.U32(change.index);
        writer.U8(
            static_cast<std::uint8_t>(tile.visibility) |
            (static_cast<std::uint8_t>(tile.terrain) << 2) |
            (tile.passable ? 0x10U : 0U));
    }

    EntityId priorEntity = 0;
    for (const ScopedEntityState& entity : delta.entityUpserts) {
        if (entity.id == 0 || entity.id <= priorEntity ||
            !IsValidScopedOwner(entity.owner) ||
            !IsValidFactionEncoding(static_cast<std::uint8_t>(entity.faction)) ||
            !IsValidEntityTypeEncoding(static_cast<std::uint8_t>(entity.type)) ||
            entity.hitPoints <= 0 || entity.maxHitPoints <= 0 ||
            entity.hitPoints > entity.maxHitPoints ||
            !IsValidWellChoiceEncoding(
                static_cast<std::uint8_t>(entity.wellChoice)) ||
            !IsValidWaystoneModeEncoding(
                static_cast<std::uint8_t>(entity.waystoneMode)) ||
            !IsValidWarformAdaptationEncoding(
                static_cast<std::uint8_t>(entity.warformAdaptation)) ||
            !IsValidScopedChoirState(entity, delta.simulationTick)) {
            return {};
        }
        priorEntity = entity.id;
        writer.U32(entity.id);
        writer.U8(entity.owner);
        writer.U8(static_cast<std::uint8_t>(entity.faction));
        writer.U8(static_cast<std::uint8_t>(entity.type));
        writer.I32(entity.position.x.Raw());
        writer.I32(entity.position.y.Raw());
        writer.I32(entity.hitPoints);
        writer.I32(entity.maxHitPoints);
        writer.U8((entity.completed ? 0x01U : 0U) |
                  (entity.deployed ? 0x02U : 0U) |
                  (entity.aegisPowered ? 0x04U : 0U));
        writer.U8(static_cast<std::uint8_t>(entity.wellChoice));
        writer.U8(static_cast<std::uint8_t>(entity.waystoneMode));
        writer.U8(static_cast<std::uint8_t>(entity.warformAdaptation));
        writer.U8(static_cast<std::uint8_t>(entity.choirIdentityState));
        writer.U64(entity.choirIdentityResolveAtTick);
        writer.U64(entity.choirIdentityNextAvailableTick);
        writer.U64(entity.choirCoherenceNextChargeTick);
    }

    EntityId priorRemoved = 0;
    for (const EntityId removed : delta.removedEntityIds) {
        const auto matchingUpsert = std::lower_bound(
            delta.entityUpserts.begin(),
            delta.entityUpserts.end(),
            removed,
            [](const ScopedEntityState& entity, EntityId id) {
                return entity.id < id;
            });
        if (removed == 0 || removed <= priorRemoved ||
            (matchingUpsert != delta.entityUpserts.end() &&
             matchingUpsert->id == removed)) {
            return {};
        }
        priorRemoved = removed;
        writer.U32(removed);
    }

    Vec2 priorSignature = Vec2::FromRaw(-1, -1);
    for (const VibrationSignature& signature :
         delta.vibrationSignatures) {
        const Vec2 position = signature.approximatePosition;
        if (position.x.Raw() < 0 || position.y.Raw() < 0 ||
            position.x.Raw() >= delta.mapWidthTiles * kFixedScale ||
            position.y.Raw() >= delta.mapHeightTiles * kFixedScale ||
            (priorSignature.x.Raw() >= 0 &&
             (position.x.Raw() < priorSignature.x.Raw() ||
              (position.x.Raw() == priorSignature.x.Raw() &&
               position.y.Raw() <= priorSignature.y.Raw())))) {
            return {};
        }
        priorSignature = position;
        writer.I32(position.x.Raw());
        writer.I32(position.y.Raw());
    }
    writer.U64(delta.scopedDigest);
    AppendIntegrity(writer);
    std::vector<std::uint8_t> encoded = std::move(writer).Finish();
    if (encoded.size() > kMaximumScopedDeltaBytes) {
        return {};
    }
    return encoded;
}

DecodeStatus DecodeScopedViewDelta(std::span<const std::uint8_t> bytes,
                                   ScopedViewDelta& delta) {
    const DecodeStatus packetStatus = ValidateVariablePacket(
        bytes, kScopedDeltaMinimumBytes, kMaximumScopedDeltaBytes);
    if (packetStatus != DecodeStatus::Ok) {
        return packetStatus;
    }
    Reader reader(bytes.first(bytes.size() - kIntegrityBytes));
    const DecodeStatus headerStatus =
        ReadHeader(reader, PacketKind::ScopedViewDelta);
    if (headerStatus != DecodeStatus::Ok) {
        return headerStatus;
    }

    ScopedViewDelta decoded{};
    std::uint8_t faction = 0;
    std::uint32_t tileChangeCount = 0;
    std::uint32_t upsertCount = 0;
    std::uint32_t removedCount = 0;
    std::uint32_t signatureCount = 0;
    if (!reader.U32(decoded.protocolVersion) ||
        !reader.U64(decoded.snapshotId) ||
        !reader.U64(decoded.baseSnapshotId) ||
        !reader.U64(decoded.simulationTick) ||
        !reader.U32(decoded.playerViewSchemaVersion) ||
        !reader.U64(decoded.lastAcceptedSequence) ||
        !reader.I32(decoded.mapWidthTiles) ||
        !reader.I32(decoded.mapHeightTiles) || !reader.U8(decoded.player) ||
        !reader.U8(faction) || !reader.I32(decoded.resources.material) ||
        !reader.I32(decoded.resources.dawnshards) ||
        !reader.I32(decoded.populationUsed) ||
        !reader.I32(decoded.populationCapacity) ||
        !reader.U32(tileChangeCount) || !reader.U32(upsertCount) ||
        !reader.U32(removedCount) || !reader.U32(signatureCount)) {
        return DecodeStatus::InvalidEncoding;
    }
    decoded.faction = static_cast<Faction>(faction);
    std::size_t tileCount = 0;
    std::size_t tileChangeBytes = 0;
    std::size_t upsertBytes = 0;
    std::size_t removedBytes = 0;
    std::size_t signatureBytes = 0;
    if (decoded.protocolVersion != kProtocolVersion ||
        decoded.playerViewSchemaVersion != kPlayerViewSchemaVersion ||
        decoded.snapshotId == 0 || decoded.baseSnapshotId == 0 ||
        decoded.snapshotId <= decoded.baseSnapshotId ||
        decoded.mapWidthTiles <= 0 || decoded.mapHeightTiles <= 0 ||
        decoded.mapWidthTiles > 256 || decoded.mapHeightTiles > 256 ||
        !IsValidScopedPlayer(decoded.player) ||
        !IsValidFactionEncoding(faction) || decoded.resources.material < 0 ||
        decoded.resources.dawnshards < 0 || decoded.populationUsed < 0 ||
        decoded.populationCapacity < 0 ||
        !CheckedMultiplySize(
            static_cast<std::size_t>(decoded.mapWidthTiles),
            static_cast<std::size_t>(decoded.mapHeightTiles), tileCount) ||
        tileCount > kMaximumScopedTiles || tileChangeCount > tileCount ||
        upsertCount > kMaximumScopedEntities ||
        removedCount > kMaximumScopedEntities ||
        signatureCount > kMaximumVibrationSignatures ||
        !CheckedMultiplySize(
            tileChangeCount, kScopedTileChangeBytes, tileChangeBytes) ||
        !CheckedMultiplySize(upsertCount, kScopedEntityBytes, upsertBytes) ||
        !CheckedMultiplySize(removedCount, 4, removedBytes) ||
        !CheckedMultiplySize(signatureCount, 8, signatureBytes)) {
        return DecodeStatus::InvalidEncoding;
    }
    const std::size_t expectedVariableBytes =
        tileChangeBytes + upsertBytes + removedBytes + signatureBytes + 8;
    if (expectedVariableBytes != reader.Remaining()) {
        return DecodeStatus::InvalidEncoding;
    }

    decoded.tileChanges.reserve(tileChangeCount);
    std::uint32_t priorTileIndex = 0;
    bool hasPriorTile = false;
    for (std::uint32_t index = 0; index < tileChangeCount; ++index) {
        ScopedTileChange change{};
        std::uint8_t packed = 0;
        if (!reader.U32(change.index) || !reader.U8(packed) ||
            change.index >= tileCount ||
            (hasPriorTile && change.index <= priorTileIndex) ||
            (packed & 0xe0U) != 0 ||
            !IsValidVisibilityEncoding(packed & 0x03U) ||
            !IsValidTerrainEncoding((packed >> 2) & 0x03U)) {
            return DecodeStatus::InvalidEncoding;
        }
        change.state = {
            static_cast<Visibility>(packed & 0x03U),
            static_cast<Terrain>((packed >> 2) & 0x03U),
            (packed & 0x10U) != 0};
        if (change.state.visibility == Visibility::Unexplored &&
            (change.state.terrain != Terrain::Blocked ||
             change.state.passable)) {
            return DecodeStatus::InvalidEncoding;
        }
        priorTileIndex = change.index;
        hasPriorTile = true;
        decoded.tileChanges.push_back(change);
    }

    decoded.entityUpserts.reserve(upsertCount);
    EntityId priorEntity = 0;
    for (std::uint32_t index = 0; index < upsertCount; ++index) {
        ScopedEntityState entity{};
        std::uint8_t factionValue = 0;
        std::uint8_t type = 0;
        std::uint8_t flags = 0;
        std::uint8_t wellChoice = 0;
        std::uint8_t waystoneMode = 0;
        std::uint8_t adaptation = 0;
        std::uint8_t choirIdentity = 0;
        std::int32_t positionX = 0;
        std::int32_t positionY = 0;
        if (!reader.U32(entity.id) || !reader.U8(entity.owner) ||
            !reader.U8(factionValue) || !reader.U8(type) ||
            !reader.I32(positionX) || !reader.I32(positionY) ||
            !reader.I32(entity.hitPoints) ||
            !reader.I32(entity.maxHitPoints) || !reader.U8(flags) ||
            !reader.U8(wellChoice) || !reader.U8(waystoneMode) ||
            !reader.U8(adaptation) || !reader.U8(choirIdentity) ||
            !reader.U64(entity.choirIdentityResolveAtTick) ||
            !reader.U64(entity.choirIdentityNextAvailableTick) ||
            !reader.U64(entity.choirCoherenceNextChargeTick) || entity.id == 0 ||
            entity.id <= priorEntity || !IsValidScopedOwner(entity.owner) ||
            !IsValidFactionEncoding(factionValue) ||
            !IsValidEntityTypeEncoding(type) || (flags & 0xf8U) != 0 ||
            entity.hitPoints <= 0 || entity.maxHitPoints <= 0 ||
            entity.hitPoints > entity.maxHitPoints ||
            !IsValidWellChoiceEncoding(wellChoice) ||
            !IsValidWaystoneModeEncoding(waystoneMode) ||
            !IsValidWarformAdaptationEncoding(adaptation) ||
            !IsValidChoirIdentityEncoding(choirIdentity)) {
            return DecodeStatus::InvalidEncoding;
        }
        priorEntity = entity.id;
        entity.faction = static_cast<Faction>(factionValue);
        entity.type = static_cast<EntityType>(type);
        entity.position = Vec2::FromRaw(positionX, positionY);
        entity.completed = (flags & 0x01U) != 0;
        entity.deployed = (flags & 0x02U) != 0;
        entity.aegisPowered = (flags & 0x04U) != 0;
        entity.wellChoice = static_cast<FutureWellChoice>(wellChoice);
        entity.waystoneMode = static_cast<WaystoneMode>(waystoneMode);
        entity.warformAdaptation =
            static_cast<WarformAdaptation>(adaptation);
        entity.choirIdentityState =
            static_cast<ChoirIdentityState>(choirIdentity);
        if (!IsValidScopedChoirState(entity, decoded.simulationTick)) {
            return DecodeStatus::InvalidEncoding;
        }
        decoded.entityUpserts.push_back(entity);
    }

    decoded.removedEntityIds.reserve(removedCount);
    EntityId priorRemoved = 0;
    for (std::uint32_t index = 0; index < removedCount; ++index) {
        EntityId removed = 0;
        if (!reader.U32(removed)) {
            return DecodeStatus::InvalidEncoding;
        }
        const auto matchingUpsert = std::lower_bound(
            decoded.entityUpserts.begin(),
            decoded.entityUpserts.end(),
            removed,
            [](const ScopedEntityState& entity, EntityId id) {
                return entity.id < id;
            });
        if (removed == 0 || removed <= priorRemoved ||
            (matchingUpsert != decoded.entityUpserts.end() &&
             matchingUpsert->id == removed)) {
            return DecodeStatus::InvalidEncoding;
        }
        priorRemoved = removed;
        decoded.removedEntityIds.push_back(removed);
    }

    decoded.vibrationSignatures.reserve(signatureCount);
    Vec2 priorSignature = Vec2::FromRaw(-1, -1);
    for (std::uint32_t index = 0; index < signatureCount; ++index) {
        std::int32_t x = 0;
        std::int32_t y = 0;
        if (!reader.I32(x) || !reader.I32(y) || x < 0 || y < 0 ||
            x >= decoded.mapWidthTiles * kFixedScale ||
            y >= decoded.mapHeightTiles * kFixedScale ||
            (priorSignature.x.Raw() >= 0 &&
             (x < priorSignature.x.Raw() ||
              (x == priorSignature.x.Raw() &&
               y <= priorSignature.y.Raw())))) {
            return DecodeStatus::InvalidEncoding;
        }
        priorSignature = Vec2::FromRaw(x, y);
        decoded.vibrationSignatures.push_back({priorSignature});
    }
    if (!reader.U64(decoded.scopedDigest) || reader.Remaining() != 0) {
        return DecodeStatus::InvalidEncoding;
    }
    delta = std::move(decoded);
    return DecodeStatus::Ok;
}

bool ApplyScopedViewDelta(const ScopedViewKeyframe& base,
                          const ScopedViewDelta& delta,
                          ScopedViewKeyframe& current,
                          std::string* error) {
    if (error != nullptr) {
        error->clear();
    }
    if (base.snapshotId == 0 || delta.baseSnapshotId != base.snapshotId ||
        delta.snapshotId <= delta.baseSnapshotId ||
        delta.simulationTick < base.simulationTick ||
        delta.protocolVersion != base.protocolVersion ||
        delta.playerViewSchemaVersion != base.playerViewSchemaVersion ||
        delta.mapWidthTiles != base.mapWidthTiles ||
        delta.mapHeightTiles != base.mapHeightTiles ||
        delta.player != base.player || delta.faction != base.faction) {
        if (error != nullptr) {
            *error = "NET_DELTA_BASE_MISSING";
        }
        return false;
    }

    ScopedViewKeyframe applied = base;
    applied.snapshotId = delta.snapshotId;
    applied.simulationTick = delta.simulationTick;
    applied.lastAcceptedSequence = delta.lastAcceptedSequence;
    applied.resources = delta.resources;
    applied.populationUsed = delta.populationUsed;
    applied.populationCapacity = delta.populationCapacity;
    applied.vibrationSignatures = delta.vibrationSignatures;
    for (const ScopedTileChange& change : delta.tileChanges) {
        if (change.index >= applied.tiles.size()) {
            if (error != nullptr) {
                *error = "NET_DELTA_TILE_INVALID";
            }
            return false;
        }
        applied.tiles[change.index] = change.state;
    }
    for (const EntityId removed : delta.removedEntityIds) {
        const auto found = std::lower_bound(
            applied.entities.begin(), applied.entities.end(), removed,
            [](const ScopedEntityState& entity, EntityId id) {
                return entity.id < id;
            });
        if (found == applied.entities.end() || found->id != removed) {
            if (error != nullptr) {
                *error = "NET_DELTA_REMOVAL_INVALID";
            }
            return false;
        }
        applied.entities.erase(found);
    }
    for (const ScopedEntityState& upsert : delta.entityUpserts) {
        const auto found = std::lower_bound(
            applied.entities.begin(), applied.entities.end(), upsert.id,
            [](const ScopedEntityState& entity, EntityId id) {
                return entity.id < id;
            });
        if (found != applied.entities.end() && found->id == upsert.id) {
            *found = upsert;
        } else {
            applied.entities.insert(found, upsert);
        }
    }

    const std::vector<std::uint8_t> encoded =
        EncodeScopedViewKeyframe(applied);
    ScopedViewKeyframe finalized{};
    if (encoded.empty() ||
        DecodeScopedViewKeyframe(encoded, finalized) != DecodeStatus::Ok ||
        finalized.scopedDigest != delta.scopedDigest) {
        if (error != nullptr) {
            *error = "NET_DELTA_DIGEST_MISMATCH";
        }
        return false;
    }
    current = std::move(finalized);
    return true;
}

}  // namespace echoes::sim::net
