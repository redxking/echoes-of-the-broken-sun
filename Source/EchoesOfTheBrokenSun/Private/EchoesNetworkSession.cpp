#include "EchoesNetworkSession.h"

namespace echoes::network {
namespace {

constexpr sim::net::Digest256 BuildId{
    0x00, 0xd1, 0x1e, 0x70, 0x73, 0x96, 0x34, 0x0f,
    0xa2, 0xd6, 0xd3, 0x56, 0xf8, 0x67, 0x28, 0x76,
    0x4a, 0x67, 0xb3, 0xc1, 0x49, 0xa0, 0x02, 0xb7,
    0x41, 0x29, 0xfc, 0xc9, 0x52, 0x26, 0x29, 0xc6};
constexpr sim::net::Digest256 CanonicalRulesPack{
    0x10, 0x0f, 0x1f, 0xcd, 0x18, 0x4c, 0xf9, 0x4f,
    0xe9, 0xb2, 0x1d, 0x3f, 0x59, 0x17, 0x14, 0xa2,
    0xe3, 0x3c, 0xc9, 0x2b, 0x60, 0xf0, 0x18, 0xbc,
    0x65, 0x23, 0x86, 0x86, 0x75, 0x15, 0x6f, 0xa0};
constexpr sim::net::Digest256 GlassScarMapPack{
    0x61, 0x74, 0x16, 0xd2, 0x5f, 0x95, 0x0c, 0x17,
    0xd6, 0x8d, 0x57, 0x00, 0xf3, 0xc9, 0x46, 0x11,
    0x80, 0x3b, 0x80, 0x7b, 0x10, 0x0d, 0x91, 0x58,
    0x07, 0x4b, 0x2f, 0x98, 0x05, 0xd9, 0x62, 0xff};
constexpr sim::net::Digest256 SkirmishSettings{
    0xde, 0x67, 0xa8, 0xed, 0x5c, 0x09, 0x06, 0xf2,
    0x9e, 0x2c, 0xb9, 0x78, 0x18, 0x2d, 0x05, 0x9a,
    0xc5, 0xa6, 0x27, 0xb8, 0xf4, 0xc5, 0xe4, 0x73,
    0xd8, 0xb7, 0x55, 0x0b, 0x05, 0x67, 0x84, 0xcb};

}  // namespace

sim::net::CompatibilityManifest BuildCompatibilityManifest(
    const sim::Simulation* simulation) {
    sim::net::CompatibilityManifest manifest{};
    manifest.simulationRulesVersion =
        simulation != nullptr
            ? simulation->Config().rules.version
            : sim::DefaultSimulationRules().version;
    manifest.buildIdSha256 = BuildId;
    manifest.rulesPackSha256 =
        simulation != nullptr
            ? simulation->Config().rules.contentSha256
            : CanonicalRulesPack;
    manifest.mapPackSha256 = GlassScarMapPack;
    manifest.matchSettingsSha256 = SkirmishSettings;
    return manifest;
}

ScopedViewAcceptance ScopedViewState::Accept(
    const sim::net::ScopedViewKeyframe& keyframe)
{
    if (keyframe.snapshotId == 0 ||
        keyframe.player >= sim::kMaximumPlayers)
    {
        return ScopedViewAcceptance::InvalidSnapshot;
    }
    if (!current_.has_value())
    {
        current_ = keyframe;
        ++acceptedCount_;
        return ScopedViewAcceptance::AcceptedFirst;
    }
    if (keyframe.player != current_->player)
    {
        return ScopedViewAcceptance::PlayerChanged;
    }
    if (keyframe.snapshotId <= current_->snapshotId)
    {
        return ScopedViewAcceptance::StaleOrDuplicate;
    }
    const ScopedViewAcceptance result =
        keyframe.snapshotId == current_->snapshotId + 1
            ? ScopedViewAcceptance::AcceptedNext
            : ScopedViewAcceptance::AcceptedRecovery;
    current_ = keyframe;
    ++acceptedCount_;
    return result;
}

const char* StableId(ScopedViewAcceptance acceptance)
{
    switch (acceptance)
    {
        case ScopedViewAcceptance::AcceptedFirst:
            return "NET_VIEW_ACCEPTED_FIRST";
        case ScopedViewAcceptance::AcceptedNext:
            return "NET_VIEW_ACCEPTED_NEXT";
        case ScopedViewAcceptance::AcceptedRecovery:
            return "NET_VIEW_ACCEPTED_RECOVERY";
        case ScopedViewAcceptance::InvalidSnapshot:
            return "NET_VIEW_INVALID_SNAPSHOT";
        case ScopedViewAcceptance::PlayerChanged:
            return "NET_VIEW_PLAYER_CHANGED";
        case ScopedViewAcceptance::StaleOrDuplicate:
            return "NET_VIEW_STALE_OR_DUPLICATE";
    }
    return "NET_VIEW_UNKNOWN";
}

}  // namespace echoes::network
