// GENERATED FILE - do not edit by hand.
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
namespace echoes::world::mission_landmarks {
struct Record { const char* id; std::uint8_t kind, x, y; std::uint16_t yaw; bool requires_blocked; std::uint8_t footprint_x0, footprint_x1, footprint_y0, footprint_y1; std::int8_t pivot_x_half_tiles, pivot_y_half_tiles; };
struct Pack { std::string_view mission_code, map_id, operation_mode, terrain_source_sha256, source_sha256; std::uint8_t mission_ordinal; const Record* records; std::size_t record_count; const char* const* kind_names; const char* const* kind_meshes; std::size_t kind_count; };

namespace m01 {
inline constexpr const char* kMapId = "glass-scar-evacuation-margin";
inline constexpr const char* kOperationMode = "CampaignPrologue";
inline constexpr std::uint8_t kMissionOrdinal = 1;
inline constexpr const char* kTerrainSourceSha256 = "7b66cf21c7ba5a37f373f9b42f341e6bec8955c18e4716f5ab3ad1a042f2e7ee";
inline constexpr const char* kSourceSha256 = "8eaad11b674e8c6fe34a7f8e7bcdf0e9e60b43cb66e4f9a4200387a781cdae35";
inline constexpr std::array<const char*, 6> kKindNames{{"ArchiveCradle", "ArchiveFrame", "RoutePaving", "ServiceConduit", "ArchiveApron", "ArchiveLoadingFace"}};
inline constexpr std::array<const char*, 6> kKindMeshes{{"M01ArchiveCradle", "M01ArchiveFrame", "M01RoutePaving", "M01ServiceConduit", "M01ArchiveApron", "M01ArchiveLoadingFace"}};
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
inline constexpr std::array<const char*, 3> kKindMeshes{{"M02ObservationSill", "M02RootingShoulder", "M02PassagePaving"}};
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
inline constexpr std::array<const char*, 4> kKindMeshes{{"M03LifeSupportBank", "M03TransitSupport", "M03ArchiveStack", "M03ReservePaving"}};
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

namespace m04 {
inline constexpr const char* kMapId = "unburied-road-vaults";
inline constexpr const char* kOperationMode = "CampaignUnburiedRoad";
inline constexpr std::uint8_t kMissionOrdinal = 4;
inline constexpr const char* kTerrainSourceSha256 = "532c6953893689090f55a6e03147b1824537a6e707de36a7cd9eb9130141d36d";
inline constexpr const char* kSourceSha256 = "7bfd514b7ffc7f07d90d8934e5f96fe021c0777063a4f69e8aaa7aec747ab206";
inline constexpr std::array<const char*, 3> kKindNames{{"RoadVault", "RoadPaving", "SpineMarker"}};
inline constexpr std::array<const char*, 3> kKindMeshes{{"CavernFormation", "CavernGround", "BasaltFormation"}};
inline constexpr std::array<Record, 27> kRecords{{
    Record{"vault-paving-01", 1, 14, 28, 0, false, 14, 14, 28, 28, 0, 0},
    Record{"vault-roadvault-01a", 0, 17, 31, 0, true, 17, 17, 31, 31, 0, 0},
    Record{"vault-spinemarker-01b", 2, 11, 31, 90, true, 11, 11, 31, 31, 0, 0},
    Record{"vault-paving-02", 1, 14, 37, 90, false, 14, 14, 37, 37, 0, 0},
    Record{"vault-roadvault-02a", 0, 17, 35, 90, true, 17, 17, 35, 35, 0, 0},
    Record{"vault-spinemarker-02b", 2, 11, 35, 180, true, 11, 11, 35, 35, 0, 0},
    Record{"vault-paving-03", 1, 20, 43, 180, false, 20, 20, 43, 43, 0, 0},
    Record{"vault-roadvault-03a", 0, 18, 35, 180, true, 18, 18, 35, 35, 0, 0},
    Record{"vault-spinemarker-03b", 2, 20, 32, 270, true, 20, 20, 32, 32, 0, 0},
    Record{"vault-paving-04", 1, 32, 28, 270, false, 32, 32, 28, 28, 0, 0},
    Record{"vault-roadvault-04a", 0, 36, 31, 270, true, 36, 36, 31, 31, 0, 0},
    Record{"vault-spinemarker-04b", 2, 28, 31, 0, true, 28, 28, 31, 31, 0, 0},
    Record{"vault-paving-05", 1, 32, 37, 0, false, 32, 32, 37, 37, 0, 0},
    Record{"vault-roadvault-05a", 0, 36, 35, 0, true, 36, 36, 35, 35, 0, 0},
    Record{"vault-spinemarker-05b", 2, 28, 35, 90, true, 28, 28, 35, 35, 0, 0},
    Record{"vault-paving-06", 1, 38, 43, 90, false, 38, 38, 43, 43, 0, 0},
    Record{"vault-roadvault-06a", 0, 38, 35, 90, true, 38, 38, 35, 35, 0, 0},
    Record{"vault-spinemarker-06b", 2, 37, 35, 180, true, 37, 37, 35, 35, 0, 0},
    Record{"vault-paving-07", 1, 50, 28, 180, false, 50, 50, 28, 28, 0, 0},
    Record{"vault-roadvault-07a", 0, 47, 31, 180, true, 47, 47, 31, 31, 0, 0},
    Record{"vault-spinemarker-07b", 2, 53, 31, 270, true, 53, 53, 31, 31, 0, 0},
    Record{"vault-paving-08", 1, 49, 35, 270, false, 49, 49, 35, 35, 0, 0},
    Record{"vault-roadvault-08a", 0, 47, 35, 270, true, 47, 47, 35, 35, 0, 0},
    Record{"vault-spinemarker-08b", 2, 47, 34, 0, true, 47, 47, 34, 34, 0, 0},
    Record{"vault-paving-09", 1, 44, 40, 0, false, 44, 44, 40, 40, 0, 0},
    Record{"vault-roadvault-09a", 0, 44, 35, 0, true, 44, 44, 35, 35, 0, 0},
    Record{"vault-spinemarker-09b", 2, 44, 34, 90, true, 44, 44, 34, 34, 0, 0},
}};
} // namespace m04

namespace m05 {
inline constexpr const char* kMapId = "terms-of-continuance-corridor";
inline constexpr const char* kOperationMode = "CampaignTermsOfContinuance";
inline constexpr std::uint8_t kMissionOrdinal = 5;
inline constexpr const char* kTerrainSourceSha256 = "fa1443c58ef754de49d09218ce5b499f269f5a3776397fe86553bcf255110a4e";
inline constexpr const char* kSourceSha256 = "72a3b9fcd269bf0abb21bf11a80cdf240099d8e9cd423464e08338eb97ac863b";
inline constexpr std::array<const char*, 3> kKindNames{{"CeasefireMarker", "CorridorPaving", "RelayMast"}};
inline constexpr std::array<const char*, 3> kKindMeshes{{"CivicFormation", "CivicGround", "BasaltFormation"}};
inline constexpr std::array<Record, 27> kRecords{{
    Record{"corridor-paving-01", 1, 14, 27, 0, false, 14, 14, 27, 27, 0, 0},
    Record{"corridor-ceasefiremarker-01a", 0, 17, 30, 0, true, 17, 17, 30, 30, 0, 0},
    Record{"corridor-relaymast-01b", 2, 11, 30, 90, true, 11, 11, 30, 30, 0, 0},
    Record{"corridor-paving-02", 1, 14, 39, 90, false, 14, 14, 39, 39, 0, 0},
    Record{"corridor-ceasefiremarker-02a", 0, 10, 42, 90, true, 10, 10, 42, 42, 0, 0},
    Record{"corridor-relaymast-02b", 2, 10, 43, 180, true, 10, 10, 43, 43, 0, 0},
    Record{"corridor-paving-03", 1, 20, 47, 180, false, 20, 20, 47, 47, 0, 0},
    Record{"corridor-ceasefiremarker-03a", 0, 10, 47, 180, true, 10, 10, 47, 47, 0, 0},
    Record{"corridor-relaymast-03b", 2, 9, 47, 270, true, 9, 9, 47, 47, 0, 0},
    Record{"corridor-paving-04", 1, 32, 27, 270, false, 32, 32, 27, 27, 0, 0},
    Record{"corridor-ceasefiremarker-04a", 0, 37, 30, 270, true, 37, 37, 30, 30, 0, 0},
    Record{"corridor-relaymast-04b", 2, 27, 30, 0, true, 27, 27, 30, 30, 0, 0},
    Record{"corridor-paving-05", 1, 32, 39, 0, false, 32, 32, 39, 39, 0, 0},
    Record{"corridor-ceasefiremarker-05a", 0, 27, 34, 0, true, 27, 27, 34, 34, 0, 0},
    Record{"corridor-relaymast-05b", 2, 37, 34, 90, true, 37, 37, 34, 34, 0, 0},
    Record{"corridor-paving-06", 1, 32, 47, 90, false, 32, 32, 47, 47, 0, 0},
    Record{"corridor-ceasefiremarker-06a", 0, 35, 60, 90, true, 35, 35, 60, 60, 0, 0},
    Record{"corridor-relaymast-06b", 2, 28, 60, 180, true, 28, 28, 60, 60, 0, 0},
    Record{"corridor-paving-07", 1, 50, 27, 180, false, 50, 50, 27, 27, 0, 0},
    Record{"corridor-ceasefiremarker-07a", 0, 47, 30, 180, true, 47, 47, 30, 30, 0, 0},
    Record{"corridor-relaymast-07b", 2, 53, 30, 270, true, 53, 53, 30, 30, 0, 0},
    Record{"corridor-paving-08", 1, 44, 38, 270, false, 44, 44, 38, 38, 0, 0},
    Record{"corridor-ceasefiremarker-08a", 0, 44, 34, 270, true, 44, 44, 34, 34, 0, 0},
    Record{"corridor-relaymast-08b", 2, 44, 33, 0, true, 44, 44, 33, 33, 0, 0},
    Record{"corridor-paving-09", 1, 44, 47, 0, false, 44, 44, 47, 47, 0, 0},
    Record{"corridor-ceasefiremarker-09a", 0, 53, 46, 0, true, 53, 53, 46, 46, 0, 0},
    Record{"corridor-relaymast-09b", 2, 54, 46, 90, true, 54, 54, 46, 46, 0, 0},
}};
} // namespace m05

namespace m06 {
inline constexpr const char* kMapId = "names-without-births-district";
inline constexpr const char* kOperationMode = "CampaignNamesWithoutBirths";
inline constexpr std::uint8_t kMissionOrdinal = 6;
inline constexpr const char* kTerrainSourceSha256 = "a70ce5089096fe776884eead58441bb19538a34bc818ae52adf6b3ddcb6190a4";
inline constexpr const char* kSourceSha256 = "805fe721a9424bd6cae39600cf08efd62173fca326d7fd9bbef92ec3ee9d02d4";
inline constexpr std::array<const char*, 3> kKindNames{{"CensusHall", "DistrictPaving", "ShelterFrame"}};
inline constexpr std::array<const char*, 3> kKindMeshes{{"CivicFormation", "CivicGround", "BasaltFormation"}};
inline constexpr std::array<Record, 36> kRecords{{
    Record{"district-paving-01", 1, 16, 22, 0, false, 16, 16, 22, 22, 0, 0},
    Record{"district-censushall-01a", 0, 11, 28, 0, true, 11, 11, 28, 28, 0, 0},
    Record{"district-shelterframe-01b", 2, 10, 28, 90, true, 10, 10, 28, 28, 0, 0},
    Record{"district-paving-02", 1, 16, 16, 90, false, 16, 16, 16, 16, 0, 0},
    Record{"district-censushall-02a", 0, 11, 29, 90, true, 11, 11, 29, 29, 0, 0},
    Record{"district-shelterframe-02b", 2, 26, 25, 180, true, 26, 26, 25, 25, 0, 0},
    Record{"district-paving-03", 1, 14, 48, 180, false, 14, 14, 48, 48, 0, 0},
    Record{"district-censushall-03a", 0, 11, 41, 180, true, 11, 11, 41, 41, 0, 0},
    Record{"district-shelterframe-03b", 2, 14, 58, 270, true, 14, 14, 58, 58, 0, 0},
    Record{"district-paving-04", 1, 22, 44, 270, false, 22, 22, 44, 44, 0, 0},
    Record{"district-censushall-04a", 0, 26, 40, 270, true, 26, 26, 40, 40, 0, 0},
    Record{"district-shelterframe-04b", 2, 26, 39, 0, true, 26, 26, 39, 39, 0, 0},
    Record{"district-paving-05", 1, 32, 22, 0, false, 32, 32, 22, 22, 0, 0},
    Record{"district-censushall-05a", 0, 33, 25, 0, true, 33, 33, 25, 25, 0, 0},
    Record{"district-shelterframe-05b", 2, 34, 25, 90, true, 34, 34, 25, 25, 0, 0},
    Record{"district-paving-06", 1, 28, 16, 90, false, 28, 28, 16, 16, 0, 0},
    Record{"district-censushall-06a", 0, 28, 25, 90, true, 28, 28, 25, 25, 0, 0},
    Record{"district-shelterframe-06b", 2, 29, 25, 180, true, 29, 29, 25, 25, 0, 0},
    Record{"district-paving-07", 1, 32, 48, 180, false, 32, 32, 48, 48, 0, 0},
    Record{"district-censushall-07a", 0, 33, 40, 180, true, 33, 33, 40, 40, 0, 0},
    Record{"district-shelterframe-07b", 2, 30, 40, 270, true, 30, 30, 40, 40, 0, 0},
    Record{"district-paving-08", 1, 32, 44, 270, false, 32, 32, 44, 44, 0, 0},
    Record{"district-censushall-08a", 0, 33, 39, 270, true, 33, 33, 39, 39, 0, 0},
    Record{"district-shelterframe-08b", 2, 34, 40, 0, true, 34, 34, 40, 40, 0, 0},
    Record{"district-paving-09", 1, 48, 22, 0, false, 48, 48, 22, 22, 0, 0},
    Record{"district-censushall-09a", 0, 51, 28, 0, true, 51, 51, 28, 28, 0, 0},
    Record{"district-shelterframe-09b", 2, 52, 28, 90, true, 52, 52, 28, 28, 0, 0},
    Record{"district-paving-10", 1, 45, 16, 90, false, 45, 45, 16, 16, 0, 0},
    Record{"district-censushall-10a", 0, 45, 13, 90, true, 45, 45, 13, 13, 0, 0},
    Record{"district-shelterframe-10b", 2, 45, 12, 180, true, 45, 45, 12, 12, 0, 0},
    Record{"district-paving-11", 1, 39, 37, 180, false, 39, 39, 37, 37, 0, 0},
    Record{"district-censushall-11a", 0, 37, 37, 180, true, 37, 37, 37, 37, 0, 0},
    Record{"district-shelterframe-11b", 2, 36, 37, 270, true, 36, 36, 37, 37, 0, 0},
    Record{"district-paving-12", 1, 46, 37, 270, false, 46, 46, 37, 37, 0, 0},
    Record{"district-censushall-12a", 0, 51, 37, 270, true, 51, 51, 37, 37, 0, 0},
    Record{"district-shelterframe-12b", 2, 52, 37, 0, true, 52, 52, 37, 37, 0, 0},
}};
} // namespace m06

namespace m07 {
inline constexpr const char* kMapId = "listening-spine-ridge";
inline constexpr const char* kOperationMode = "CampaignShapeOfSilence";
inline constexpr std::uint8_t kMissionOrdinal = 7;
inline constexpr const char* kTerrainSourceSha256 = "7977033d5ced5032741fcebe4dffbd243803a61e61e1af382e7de8faafa6bfd0";
inline constexpr const char* kSourceSha256 = "bcdc4f505b7cf97dca578454ab9d9f52e329350c7eaa68a5521c2e296536cf60";
inline constexpr std::array<const char*, 3> kKindNames{{"ListeningSpine", "RidgePaving", "AnchorStone"}};
inline constexpr std::array<const char*, 3> kKindMeshes{{"ChoirFormation", "ChoirGround", "BasaltFormation"}};
inline constexpr std::array<Record, 18> kRecords{{
    Record{"ridge-paving-01", 1, 14, 28, 0, false, 14, 14, 28, 28, 0, 0},
    Record{"ridge-listeningspine-01a", 0, 5, 28, 0, true, 5, 5, 28, 28, 0, 0},
    Record{"ridge-anchorstone-01b", 2, 5, 27, 90, true, 5, 5, 27, 27, 0, 0},
    Record{"ridge-paving-02", 1, 14, 50, 90, false, 14, 14, 50, 50, 0, 0},
    Record{"ridge-listeningspine-02a", 0, 19, 54, 90, true, 19, 19, 54, 54, 0, 0},
    Record{"ridge-anchorstone-02b", 2, 19, 55, 180, true, 19, 19, 55, 55, 0, 0},
    Record{"ridge-paving-03", 1, 32, 28, 180, false, 32, 32, 28, 28, 0, 0},
    Record{"ridge-listeningspine-03a", 0, 32, 13, 180, true, 32, 32, 13, 13, 0, 0},
    Record{"ridge-anchorstone-03b", 2, 32, 12, 270, true, 32, 32, 12, 12, 0, 0},
    Record{"ridge-paving-04", 1, 32, 50, 270, false, 32, 32, 50, 50, 0, 0},
    Record{"ridge-listeningspine-04a", 0, 32, 54, 270, true, 32, 32, 54, 54, 0, 0},
    Record{"ridge-anchorstone-04b", 2, 32, 55, 0, true, 32, 32, 55, 55, 0, 0},
    Record{"ridge-paving-05", 1, 48, 20, 0, false, 48, 48, 20, 20, 0, 0},
    Record{"ridge-listeningspine-05a", 0, 53, 20, 0, true, 53, 53, 20, 20, 0, 0},
    Record{"ridge-anchorstone-05b", 2, 53, 19, 90, true, 53, 53, 19, 19, 0, 0},
    Record{"ridge-paving-06", 1, 25, 50, 90, false, 25, 25, 50, 50, 0, 0},
    Record{"ridge-listeningspine-06a", 0, 25, 54, 90, true, 25, 25, 54, 54, 0, 0},
    Record{"ridge-anchorstone-06b", 2, 24, 54, 180, true, 24, 24, 54, 54, 0, 0},
}};
} // namespace m07

inline constexpr std::array<Pack, 7> kPacks{{
    {"M01", m01::kMapId, m01::kOperationMode, m01::kTerrainSourceSha256, m01::kSourceSha256, m01::kMissionOrdinal, m01::kRecords.data(), m01::kRecords.size(), m01::kKindNames.data(), m01::kKindMeshes.data(), m01::kKindNames.size()},
    {"M02", m02::kMapId, m02::kOperationMode, m02::kTerrainSourceSha256, m02::kSourceSha256, m02::kMissionOrdinal, m02::kRecords.data(), m02::kRecords.size(), m02::kKindNames.data(), m02::kKindMeshes.data(), m02::kKindNames.size()},
    {"M03", m03::kMapId, m03::kOperationMode, m03::kTerrainSourceSha256, m03::kSourceSha256, m03::kMissionOrdinal, m03::kRecords.data(), m03::kRecords.size(), m03::kKindNames.data(), m03::kKindMeshes.data(), m03::kKindNames.size()},
    {"M04", m04::kMapId, m04::kOperationMode, m04::kTerrainSourceSha256, m04::kSourceSha256, m04::kMissionOrdinal, m04::kRecords.data(), m04::kRecords.size(), m04::kKindNames.data(), m04::kKindMeshes.data(), m04::kKindNames.size()},
    {"M05", m05::kMapId, m05::kOperationMode, m05::kTerrainSourceSha256, m05::kSourceSha256, m05::kMissionOrdinal, m05::kRecords.data(), m05::kRecords.size(), m05::kKindNames.data(), m05::kKindMeshes.data(), m05::kKindNames.size()},
    {"M06", m06::kMapId, m06::kOperationMode, m06::kTerrainSourceSha256, m06::kSourceSha256, m06::kMissionOrdinal, m06::kRecords.data(), m06::kRecords.size(), m06::kKindNames.data(), m06::kKindMeshes.data(), m06::kKindNames.size()},
    {"M07", m07::kMapId, m07::kOperationMode, m07::kTerrainSourceSha256, m07::kSourceSha256, m07::kMissionOrdinal, m07::kRecords.data(), m07::kRecords.size(), m07::kKindNames.data(), m07::kKindMeshes.data(), m07::kKindNames.size()},
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
