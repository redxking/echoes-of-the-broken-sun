#include "EchoesNetworkSession.h"

#include <cmath>

namespace echoes::network {
namespace {

// SHA-256("EchoesOfTheBrokenSun:0.84.0:protocol-2:snapshot-20:view-1").
constexpr sim::net::Digest256 BuildId{
    0x8c, 0x33, 0x1f, 0xe1, 0x0c, 0x29, 0xe7, 0x1f,
    0xca, 0xd8, 0xdb, 0x11, 0x98, 0xfc, 0xc1, 0x6a,
    0x4e, 0x58, 0xcf, 0x21, 0xec, 0x3e, 0xa3, 0x1c,
    0x06, 0x43, 0x13, 0x21, 0xb0, 0x99, 0x6a, 0xea};
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

bool CommandRateLimiter::TryConsume(
    double nowSeconds,
    std::uint32_t intentCount)
{
    if (!std::isfinite(nowSeconds) || nowSeconds < 0.0 || intentCount == 0 ||
        intentCount > MaximumIntentsPerWindow ||
        (windowStartSeconds_ >= 0.0 && nowSeconds < windowStartSeconds_))
    {
        return false;
    }
    if (windowStartSeconds_ < 0.0 ||
        nowSeconds - windowStartSeconds_ >= WindowSeconds)
    {
        windowStartSeconds_ = nowSeconds;
        requestCount_ = 0;
        intentCount_ = 0;
    }
    if (requestCount_ >= MaximumCommandsPerWindow ||
        intentCount_ > MaximumIntentsPerWindow - intentCount)
    {
        return false;
    }
    ++requestCount_;
    intentCount_ += intentCount;
    return true;
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

ScopedViewAcceptance ScopedViewState::AcceptDelta(
    const sim::net::ScopedViewDelta& delta,
    std::string* error)
{
    if (error != nullptr)
    {
        error->clear();
    }
    if (!current_.has_value())
    {
        if (error != nullptr)
        {
            *error = "NET_DELTA_BASE_MISSING";
        }
        return ScopedViewAcceptance::BaseMissing;
    }
    if (delta.player != current_->player)
    {
        return ScopedViewAcceptance::PlayerChanged;
    }
    if (delta.snapshotId <= current_->snapshotId)
    {
        return ScopedViewAcceptance::StaleOrDuplicate;
    }
    if (delta.baseSnapshotId != current_->snapshotId)
    {
        if (error != nullptr)
        {
            *error = "NET_DELTA_BASE_MISSING";
        }
        return ScopedViewAcceptance::BaseMissing;
    }
    sim::net::ScopedViewKeyframe applied{};
    if (!sim::net::ApplyScopedViewDelta(
            *current_, delta, applied, error))
    {
        return ScopedViewAcceptance::DeltaRejected;
    }
    current_ = std::move(applied);
    ++acceptedCount_;
    return ScopedViewAcceptance::AcceptedDelta;
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
        case ScopedViewAcceptance::AcceptedDelta:
            return "NET_VIEW_ACCEPTED_DELTA";
        case ScopedViewAcceptance::InvalidSnapshot:
            return "NET_VIEW_INVALID_SNAPSHOT";
        case ScopedViewAcceptance::PlayerChanged:
            return "NET_VIEW_PLAYER_CHANGED";
        case ScopedViewAcceptance::StaleOrDuplicate:
            return "NET_VIEW_STALE_OR_DUPLICATE";
        case ScopedViewAcceptance::BaseMissing:
            return "NET_VIEW_BASE_MISSING";
        case ScopedViewAcceptance::DeltaRejected:
            return "NET_VIEW_DELTA_REJECTED";
    }
    return "NET_VIEW_UNKNOWN";
}

}  // namespace echoes::network
