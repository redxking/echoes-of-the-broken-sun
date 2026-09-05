// GENERATED FILE - do not edit by hand.
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
namespace echoes::world::mission_landmarks {
struct Record { const char* id; std::uint8_t kind, x, y; std::uint16_t yaw; bool requires_blocked; std::uint8_t footprint_x0, footprint_x1, footprint_y0, footprint_y1; std::int8_t pivot_x_half_tiles, pivot_y_half_tiles; };
struct Pack { std::string_view mission_code, map_id, operation_mode, terrain_source_sha256, source_sha256; std::uint8_t mission_ordinal; const Record* records; std::size_t record_count; const char* const* kind_names; std::size_t kind_count; };

namespace m01 {
inline constexpr const char* kMapId = "glass-scar-evacuation-margin";
inline constexpr const char* kOperationMode = "CampaignPrologue";
inline constexpr std::uint8_t kMissionOrdinal = 1;
inline constexpr const char* kTerrainSourceSha256 = "8ae50fa5adf740f0f7f0508c151e82c4e86b7f3a1e70cf323717ee536418669b";
inline constexpr const char* kSourceSha256 = "6d423c243a3ba813e9336fe431eccf821075ea1cda70629a7604d79d18eef9cc";
inline constexpr std::array<const char*, 6> kKindNames{{"ArchiveCradle", "ArchiveFrame", "RoutePaving", "ServiceConduit", "ArchiveApron", "ArchiveLoadingFace"}};
inline constexpr std::array<Record, 28> kRecords{{
    Record{"outpost-cradle-west", 0, 0, 11, 90, true, 0, 0, 11, 11, 0, 0},
    Record{"outpost-frame-east", 1, 20, 10, 270, true, 20, 20, 10, 10, 0, 0},
    Record{"archive-frame-north", 1, 26, 18, 180, true, 26, 26, 18, 18, 0, 0},
    Record{"withdrawal-paving-06", 2, 6, 17, 0, false, 6, 6, 17, 17, 0, 0},
    Record{"withdrawal-paving-07", 2, 7, 17, 0, false, 7, 7, 17, 17, 0, 0},
    Record{"withdrawal-paving-08", 2, 8, 17, 0, false, 8, 8, 17, 17, 0, 0},
    Record{"withdrawal-paving-09", 2, 9, 17, 0, false, 9, 9, 17, 17, 0, 0},
    Record{"withdrawal-paving-10", 2, 10, 17, 0, false, 10, 10, 17, 17, 0, 0},
    Record{"withdrawal-paving-11", 2, 11, 17, 0, false, 11, 11, 17, 17, 0, 0},
    Record{"withdrawal-paving-12", 2, 12, 17, 0, false, 12, 12, 17, 17, 0, 0},
    Record{"withdrawal-paving-13", 2, 13, 17, 0, false, 13, 13, 17, 17, 0, 0},
    Record{"withdrawal-paving-14", 2, 14, 17, 0, false, 14, 14, 17, 17, 0, 0},
    Record{"withdrawal-paving-15", 2, 15, 17, 0, false, 15, 15, 17, 17, 0, 0},
    Record{"withdrawal-paving-16", 2, 16, 17, 0, false, 16, 16, 17, 17, 0, 0},
    Record{"archive-paving-17", 2, 17, 17, 0, false, 17, 17, 17, 17, 0, 0},
    Record{"causeway-south-paving", 2, 32, 29, 90, false, 32, 32, 29, 29, 0, 0},
    Record{"causeway-deck-paving", 2, 32, 32, 90, false, 32, 32, 32, 32, 0, 0},
    Record{"causeway-north-paving", 2, 32, 35, 90, false, 32, 32, 35, 35, 0, 0},
    Record{"outpost-conduit", 3, 1, 11, 90, true, 1, 1, 11, 11, 0, 0},
    Record{"east-outpost-conduit", 3, 19, 10, 270, true, 19, 19, 10, 10, 0, 0},
    Record{"archive-return-conduit", 3, 28, 18, 270, true, 28, 28, 18, 18, 0, 0},
    Record{"withdrawal-frame-west", 1, 3, 20, 90, true, 3, 3, 20, 20, 0, 0},
    Record{"withdrawal-conduit-return", 3, 9, 20, 180, true, 9, 9, 20, 20, 0, 0},
    Record{"archive-lip-frame-a", 1, 24, 14, 0, true, 24, 24, 14, 14, 0, 0},
    Record{"archive-lip-frame-b", 1, 26, 14, 0, true, 26, 26, 14, 14, 0, 0},
    Record{"causeway-service-conduit", 3, 27, 26, 90, true, 27, 27, 26, 26, 0, 0},
    Record{"archive-loading-apron", 4, 20, 18, 0, false, 18, 22, 16, 19, 0, -1},
    Record{"archive-loading-face", 5, 25, 19, 0, true, 23, 27, 19, 19, 0, 0},
}};
} // namespace m01

namespace m02 {
inline constexpr const char* kMapId = "shivergrass-migration-basin";
inline constexpr const char* kOperationMode = "CampaignSevenAccounts";
inline constexpr std::uint8_t kMissionOrdinal = 2;
inline constexpr const char* kTerrainSourceSha256 = "5024db41cd825e2e948a84860b9f58f2a71690a650929cafa80a24fef6458f6c";
inline constexpr const char* kSourceSha256 = "7f1b5391d2285dd33ba1548459c1afa848d415106656eb8d58041209f7b86392";
inline constexpr std::array<const char*, 3> kKindNames{{"ObservationSill", "RootingShoulder", "PassagePaving"}};
inline constexpr std::array<Record, 47> kRecords{{
    Record{"rootingshoulder-24-10", 1, 24, 10, 90, true, 24, 24, 10, 10, 0, 0},
    Record{"rootingshoulder-29-09", 1, 29, 9, 270, true, 29, 29, 9, 9, 0, 0},
    Record{"rootingshoulder-02-29", 1, 2, 29, 270, true, 2, 2, 29, 29, 0, 0},
    Record{"rootingshoulder-56-31", 1, 56, 31, 90, true, 56, 56, 31, 31, 0, 0},
    Record{"rootingshoulder-36-31", 1, 36, 31, 180, true, 36, 36, 31, 31, 0, 0},
    Record{"rootingshoulder-37-33", 1, 37, 33, 0, true, 37, 37, 33, 33, 0, 0},
    Record{"rootingshoulder-40-51", 1, 40, 51, 180, true, 40, 40, 51, 51, 0, 0},
    Record{"passagepaving-16-18", 2, 16, 18, 0, false, 16, 16, 18, 18, 0, 0},
    Record{"passagepaving-17-21", 2, 17, 21, 0, false, 17, 17, 21, 21, 0, 0},
    Record{"passagepaving-18-24", 2, 18, 24, 0, false, 18, 18, 24, 24, 0, 0},
    Record{"passagepaving-19-27", 2, 19, 27, 0, false, 19, 19, 27, 27, 0, 0},
    Record{"passagepaving-20-29", 2, 20, 29, 0, false, 20, 20, 29, 29, 0, 0},
    Record{"passagepaving-20-34", 2, 20, 34, 0, false, 20, 20, 34, 34, 0, 0},
    Record{"passagepaving-20-37", 2, 20, 37, 0, false, 20, 20, 37, 37, 0, 0},
    Record{"passagepaving-20-39", 2, 20, 39, 0, false, 20, 20, 39, 39, 0, 0},
    Record{"passagepaving-20-41", 2, 20, 41, 0, false, 20, 20, 41, 41, 0, 0},
    Record{"passagepaving-21-42", 2, 21, 42, 0, false, 21, 21, 42, 42, 0, 0},
    Record{"passagepaving-22-43", 2, 22, 43, 0, false, 22, 22, 43, 43, 0, 0},
    Record{"passagepaving-23-43", 2, 23, 43, 0, false, 23, 23, 43, 43, 0, 0},
    Record{"passagepaving-23-44", 2, 23, 44, 0, false, 23, 23, 44, 44, 0, 0},
    Record{"passagepaving-24-44", 2, 24, 44, 0, false, 24, 24, 44, 44, 0, 0},
    Record{"passagepaving-31-35", 2, 31, 35, 0, false, 31, 31, 35, 35, 0, 0},
    Record{"passagepaving-33-37", 2, 33, 37, 0, false, 33, 33, 37, 37, 0, 0},
    Record{"passagepaving-35-39", 2, 35, 39, 0, false, 35, 35, 39, 39, 0, 0},
    Record{"passagepaving-35-40", 2, 35, 40, 0, false, 35, 35, 40, 40, 0, 0},
    Record{"passagepaving-36-40", 2, 36, 40, 0, false, 36, 36, 40, 40, 0, 0},
    Record{"passagepaving-37-42", 2, 37, 42, 0, false, 37, 37, 42, 42, 0, 0},
    Record{"passagepaving-38-42", 2, 38, 42, 0, false, 38, 38, 42, 42, 0, 0},
    Record{"passagepaving-38-43", 2, 38, 43, 0, false, 38, 38, 43, 43, 0, 0},
    Record{"passagepaving-39-43", 2, 39, 43, 0, false, 39, 39, 43, 43, 0, 0},
    Record{"passagepaving-39-35", 2, 39, 35, 0, false, 39, 39, 35, 35, 0, 0},
    Record{"passagepaving-40-37", 2, 40, 37, 0, false, 40, 40, 37, 37, 0, 0},
    Record{"passagepaving-40-40", 2, 40, 40, 0, false, 40, 40, 40, 40, 0, 0},
    Record{"passagepaving-40-42", 2, 40, 42, 0, false, 40, 40, 42, 42, 0, 0},
    Record{"passagepaving-41-42", 2, 41, 42, 0, false, 41, 41, 42, 42, 0, 0},
    Record{"passagepaving-42-43", 2, 42, 43, 0, false, 42, 42, 43, 43, 0, 0},
    Record{"passagepaving-43-43", 2, 43, 43, 0, false, 43, 43, 43, 43, 0, 0},
    Record{"passagepaving-43-44", 2, 43, 44, 0, false, 43, 43, 44, 44, 0, 0},
    Record{"passagepaving-44-44", 2, 44, 44, 0, false, 44, 44, 44, 44, 0, 0},
    Record{"observationsill-26-14", 0, 26, 14, 0, true, 25, 27, 14, 14, 0, 0},
    Record{"observationsill-27-10", 0, 27, 10, 0, true, 26, 28, 10, 10, 0, 0},
    Record{"observationsill-02-23", 0, 2, 23, 90, true, 2, 2, 22, 24, 0, 0},
    Record{"observationsill-02-40", 0, 2, 40, 90, true, 2, 2, 39, 41, 0, 0},
    Record{"observationsill-56-23", 0, 56, 23, 270, true, 56, 56, 22, 24, 0, 0},
    Record{"observationsill-56-40", 0, 56, 40, 270, true, 56, 56, 39, 41, 0, 0},
    Record{"observationsill-38-49", 0, 38, 49, 180, true, 37, 39, 49, 49, 0, 0},
    Record{"observationsill-40-54", 0, 40, 54, 180, true, 39, 41, 54, 54, 0, 0},
}};
} // namespace m02

namespace m03 {
inline constexpr const char* kMapId = "ark-city-reserve-service";
inline constexpr const char* kOperationMode = "CampaignCityReserve";
inline constexpr std::uint8_t kMissionOrdinal = 3;
inline constexpr const char* kTerrainSourceSha256 = "28d3373a6af7413311e85b5eb62d9700787f3a782289452c8431159dc498c6e4";
inline constexpr const char* kSourceSha256 = "4796b082cd95e340151f3204ab2568d0775a6c7b7d0d1480406c15f7b83800a9";
inline constexpr std::array<const char*, 4> kKindNames{{"LifeSupportBank", "TransitSupport", "ArchiveStack", "ReservePaving"}};
inline constexpr std::array<Record, 31> kRecords{{
    Record{"life-support-bank-west", 0, 1, 10, 0, true, 0, 2, 10, 10, 0, 0},
    Record{"life-support-bank-east", 0, 4, 18, 180, true, 3, 5, 18, 18, 0, 0},
    Record{"life-trunk-24-10", 3, 24, 10, 0, false, 24, 24, 10, 10, 0, 0},
    Record{"life-trunk-20-10", 3, 20, 10, 0, false, 20, 20, 10, 10, 0, 0},
    Record{"life-trunk-16-10", 3, 16, 10, 0, false, 16, 16, 10, 10, 0, 0},
    Record{"life-trunk-12-10", 3, 12, 10, 0, false, 12, 12, 10, 10, 0, 0},
    Record{"life-trunk-08-10", 3, 8, 10, 0, false, 8, 8, 10, 10, 0, 0},
    Record{"life-trunk-06-10", 3, 6, 10, 0, false, 6, 6, 10, 10, 0, 0},
    Record{"transit-support-north", 1, 36, 12, 0, true, 35, 37, 12, 12, 0, 0},
    Record{"transit-support-south", 1, 38, 6, 180, true, 37, 39, 6, 6, 0, 0},
    Record{"transit-trunk-10-24", 3, 10, 24, 0, false, 10, 10, 24, 24, 0, 0},
    Record{"transit-trunk-14-22", 3, 14, 22, 0, false, 14, 14, 22, 22, 0, 0},
    Record{"transit-trunk-18-20", 3, 18, 20, 0, false, 18, 18, 20, 20, 0, 0},
    Record{"transit-trunk-22-18", 3, 22, 18, 0, false, 22, 22, 18, 18, 0, 0},
    Record{"transit-trunk-26-16", 3, 26, 16, 0, false, 26, 26, 16, 16, 0, 0},
    Record{"transit-trunk-30-14", 3, 30, 14, 0, false, 30, 30, 14, 14, 0, 0},
    Record{"transit-trunk-34-14", 3, 34, 14, 0, false, 34, 34, 14, 14, 0, 0},
    Record{"archive-stack-west", 2, 43, 28, 0, true, 42, 44, 28, 28, 0, 0},
    Record{"archive-stack-east", 2, 46, 31, 180, true, 45, 47, 31, 31, 0, 0},
    Record{"archive-trunk-20-20", 3, 20, 20, 0, false, 20, 20, 20, 20, 0, 0},
    Record{"archive-trunk-24-22", 3, 24, 22, 0, false, 24, 24, 22, 22, 0, 0},
    Record{"archive-trunk-28-24", 3, 28, 24, 0, false, 28, 28, 24, 24, 0, 0},
    Record{"archive-trunk-32-26", 3, 32, 26, 0, false, 32, 32, 26, 26, 0, 0},
    Record{"archive-trunk-36-28", 3, 36, 28, 0, false, 36, 36, 28, 28, 0, 0},
    Record{"archive-trunk-40-28", 3, 40, 28, 0, false, 40, 40, 28, 28, 0, 0},
    Record{"well-reserve-trunk-32-32", 3, 32, 32, 0, false, 32, 32, 32, 32, 0, 0},
    Record{"well-reserve-trunk-28-33", 3, 28, 33, 0, false, 28, 28, 33, 33, 0, 0},
    Record{"well-reserve-trunk-24-34", 3, 24, 34, 0, false, 24, 24, 34, 34, 0, 0},
    Record{"well-reserve-trunk-20-35", 3, 20, 35, 0, false, 20, 20, 35, 35, 0, 0},
    Record{"well-reserve-trunk-16-36", 3, 16, 36, 0, false, 16, 16, 36, 36, 0, 0},
    Record{"well-reserve-trunk-14-37", 3, 14, 37, 0, false, 14, 14, 37, 37, 0, 0},
}};
} // namespace m03

inline constexpr std::array<Pack, 3> kPacks{{
    {"M01", m01::kMapId, m01::kOperationMode, m01::kTerrainSourceSha256, m01::kSourceSha256, m01::kMissionOrdinal, m01::kRecords.data(), m01::kRecords.size(), m01::kKindNames.data(), m01::kKindNames.size()},
    {"M02", m02::kMapId, m02::kOperationMode, m02::kTerrainSourceSha256, m02::kSourceSha256, m02::kMissionOrdinal, m02::kRecords.data(), m02::kRecords.size(), m02::kKindNames.data(), m02::kKindNames.size()},
    {"M03", m03::kMapId, m03::kOperationMode, m03::kTerrainSourceSha256, m03::kSourceSha256, m03::kMissionOrdinal, m03::kRecords.data(), m03::kRecords.size(), m03::kKindNames.data(), m03::kKindNames.size()},
}};
inline constexpr const Pack* FindPack(std::uint8_t ordinal, std::string_view map_id) { for (const auto& pack : kPacks) if (pack.mission_ordinal == ordinal && pack.map_id == map_id) return &pack; return nullptr; }

// Legacy M01 aliases preserve existing generated-header consumers.
inline constexpr const char* kMapId = m01::kMapId;
inline constexpr std::uint8_t kMissionOrdinal = m01::kMissionOrdinal;
inline constexpr const char* kTerrainSourceSha256 = m01::kTerrainSourceSha256;
inline constexpr const char* kSourceSha256 = m01::kSourceSha256;
inline constexpr const auto& kRecords = m01::kRecords;
inline constexpr std::uint32_t kArchiveCradleCount = 1;
inline constexpr std::uint32_t kArchiveFrameCount = 5;
inline constexpr std::uint32_t kRoutePavingCount = 15;
inline constexpr std::uint32_t kServiceConduitCount = 5;
inline constexpr std::uint32_t kArchiveApronCount = 1;
inline constexpr std::uint32_t kArchiveLoadingFaceCount = 1;
} // namespace echoes::world::mission_landmarks
