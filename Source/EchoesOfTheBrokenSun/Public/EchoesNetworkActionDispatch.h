#pragma once

#include "EchoesSimCore/NetworkProtocol.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <span>
#include <vector>

namespace echoes::network {

/**
 * Client-only reservation for a Bulwark action submitted to server authority.
 * This value never enters simulation, snapshot, replay, or wire state.
 */
struct BulwarkDispatchActor final
{
    sim::EntityId actor = 0;
    bool baselineDeployed = false;
    sim::BulwarkDeploymentPhase baselinePhase =
        sim::BulwarkDeploymentPhase::None;

    friend bool operator==(const BulwarkDispatchActor&,
                           const BulwarkDispatchActor&) = default;
};

enum class BulwarkAdmissionResult : std::uint8_t
{
    Applied = 0,
    Rejected,
    UnknownBatch,
    InvalidBatchId,
    InvalidCounts,
    TickOverflow,
};

/**
 * Presentation-adapter state for SPEC-CMD-013 online Bulwark gestures.
 * The authoritative simulation still validates and executes every submitted
 * command. Reservations only prevent a rapid client gesture from selecting an
 * actor whose preceding action has not yet appeared in accepted scoped state.
 */
class BulwarkActionDispatchState final
{
public:
    [[nodiscard]] std::vector<BulwarkDispatchActor> Resolve(
        const sim::net::ScopedViewKeyframe& view,
        sim::PlayerId owner,
        sim::Vec2 target,
        std::span<const sim::EntityId> selected,
        bool allEligible) const
    {
        std::vector<const sim::net::ScopedEntityState*> candidates;
        if (selected.empty() || owner >= sim::kMaximumPlayers ||
            view.player != owner)
        {
            return {};
        }
        candidates.reserve(selected.size());
        for (const sim::EntityId selectedId : selected)
        {
            if (selectedId == 0 || IsPending(selectedId))
            {
                continue;
            }
            const auto actor = std::lower_bound(
                view.entities.begin(), view.entities.end(), selectedId,
                [](const sim::net::ScopedEntityState& entity,
                   sim::EntityId id)
                {
                    return entity.id < id;
                });
            if (actor == view.entities.end() || actor->id != selectedId ||
                actor->owner != owner ||
                actor->faction != sim::Faction::MeridianCompact ||
                actor->type != sim::EntityType::HeavyUnit ||
                !actor->completed || actor->hitPoints <= 0 ||
                actor->deploymentPhase !=
                    sim::BulwarkDeploymentPhase::None ||
                (!actor->deployed && actor->position == target))
            {
                continue;
            }
            if (std::none_of(
                    candidates.begin(), candidates.end(),
                    [selectedId](const sim::net::ScopedEntityState* existing)
                    {
                        return existing->id == selectedId;
                    }))
            {
                candidates.push_back(&*actor);
            }
        }
        if (candidates.empty())
        {
            return {};
        }
        std::sort(
            candidates.begin(), candidates.end(),
            [](const sim::net::ScopedEntityState* left,
               const sim::net::ScopedEntityState* right)
            {
                return left->id < right->id;
            });
        if (!allEligible)
        {
            const sim::net::ScopedEntityState* closest = candidates.front();
            SquaredDistance closestDistance = DistanceSquared(
                closest->position, target);
            for (const sim::net::ScopedEntityState* candidate : candidates)
            {
                const SquaredDistance distance = DistanceSquared(
                    candidate->position, target);
                if (distance < closestDistance ||
                    (distance == closestDistance &&
                     candidate->id < closest->id))
                {
                    closest = candidate;
                    closestDistance = distance;
                }
            }
            candidates.assign(1, closest);
        }

        std::vector<BulwarkDispatchActor> resolved;
        resolved.reserve(candidates.size());
        for (const sim::net::ScopedEntityState* actor : candidates)
        {
            resolved.push_back(
                {actor->id, actor->deployed, actor->deploymentPhase});
        }
        return resolved;
    }

    [[nodiscard]] bool ReserveBatch(
        std::uint64_t batchId,
        std::span<const BulwarkDispatchActor> actors)
    {
        if (batchId == 0 || batches_.contains(batchId) || actors.empty() ||
            actors.size() > sim::net::kMaximumCommandsPerBatch)
        {
            return false;
        }
        PendingBatch pending{};
        pending.actors.assign(actors.begin(), actors.end());
        pending.submittedActorCount = actors.size();
        std::sort(
            pending.actors.begin(), pending.actors.end(),
            [](const BulwarkDispatchActor& left,
               const BulwarkDispatchActor& right)
            {
                return left.actor < right.actor;
            });
        sim::EntityId priorActor = 0;
        for (const BulwarkDispatchActor& actor : pending.actors)
        {
            if (actor.actor == 0 || actor.actor == priorActor ||
                actor.baselinePhase != sim::BulwarkDeploymentPhase::None ||
                IsPending(actor.actor))
            {
                return false;
            }
            priorActor = actor.actor;
        }
        batches_.emplace(batchId, std::move(pending));
        return true;
    }

    [[nodiscard]] BulwarkAdmissionResult ApplyAdmission(
        std::uint64_t batchId,
        std::int32_t acceptedCount,
        std::int32_t rejectedCount,
        sim::Tick serverTick,
        sim::Tick inputDelayTicks,
        const std::optional<sim::net::ScopedViewKeyframe>& currentView)
    {
        if (batchId == 0)
        {
            // A malformed response has no trustworthy correlation key. Drop
            // every local reservation so no gesture can remain latched.
            Reset();
            return BulwarkAdmissionResult::InvalidBatchId;
        }
        const auto found = batches_.find(batchId);
        if (found == batches_.end())
        {
            return BulwarkAdmissionResult::UnknownBatch;
        }
        const std::int64_t totalCount =
            static_cast<std::int64_t>(acceptedCount) + rejectedCount;
        if (acceptedCount < 0 || rejectedCount < 0 ||
            totalCount != static_cast<std::int64_t>(
                found->second.submittedActorCount))
        {
            batches_.erase(found);
            return BulwarkAdmissionResult::InvalidCounts;
        }
        if (acceptedCount == 0)
        {
            batches_.erase(found);
            return BulwarkAdmissionResult::Rejected;
        }
        if (serverTick > std::numeric_limits<sim::Tick>::max() -
                inputDelayTicks)
        {
            batches_.erase(found);
            return BulwarkAdmissionResult::TickOverflow;
        }
        found->second.acknowledged = true;
        found->second.executeTick = serverTick + inputDelayTicks;
        if (currentView.has_value())
        {
            ReconcileAcceptedView(*currentView);
        }
        return BulwarkAdmissionResult::Applied;
    }

    [[nodiscard]] bool RejectBatch(std::uint64_t batchId)
    {
        return batchId != 0 && batches_.erase(batchId) != 0;
    }

    void ReconcileAcceptedView(
        const sim::net::ScopedViewKeyframe& acceptedView)
    {
        for (auto batch = batches_.begin(); batch != batches_.end();)
        {
            std::vector<BulwarkDispatchActor>& actors = batch->second.actors;
            actors.erase(
                std::remove_if(
                    actors.begin(), actors.end(),
                    [&](const BulwarkDispatchActor& pending)
                    {
                        const auto current = std::lower_bound(
                            acceptedView.entities.begin(),
                            acceptedView.entities.end(), pending.actor,
                            [](const sim::net::ScopedEntityState& entity,
                               sim::EntityId id)
                            {
                                return entity.id < id;
                            });
                        if (current == acceptedView.entities.end() ||
                            current->id != pending.actor ||
                            current->owner != acceptedView.player ||
                            current->faction !=
                                sim::Faction::MeridianCompact ||
                            current->type != sim::EntityType::HeavyUnit ||
                            !current->completed || current->hitPoints <= 0)
                        {
                            return true;
                        }
                        if (current->deployed != pending.baselineDeployed ||
                            current->deploymentPhase != pending.baselinePhase)
                        {
                            return true;
                        }
                        return batch->second.acknowledged &&
                               acceptedView.simulationTick >
                                   batch->second.executeTick;
                    }),
                actors.end());
            if (actors.empty())
            {
                batch = batches_.erase(batch);
            }
            else
            {
                ++batch;
            }
        }
    }

    void Reset()
    {
        batches_.clear();
    }

    [[nodiscard]] bool IsPending(sim::EntityId actor) const
    {
        if (actor == 0)
        {
            return false;
        }
        for (const auto& [batchId, batch] : batches_)
        {
            (void)batchId;
            if (std::any_of(
                    batch.actors.begin(), batch.actors.end(),
                    [actor](const BulwarkDispatchActor& pending)
                    {
                        return pending.actor == actor;
                    }))
            {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] bool HasBatch(std::uint64_t batchId) const
    {
        return batchId != 0 && batches_.contains(batchId);
    }

    [[nodiscard]] std::size_t PendingActorCount() const
    {
        std::size_t count = 0;
        for (const auto& [batchId, batch] : batches_)
        {
            (void)batchId;
            count += batch.actors.size();
        }
        return count;
    }

    [[nodiscard]] std::size_t PendingBatchCount() const
    {
        return batches_.size();
    }

private:
    struct SquaredDistance final
    {
        std::uint64_t high = 0;
        std::uint64_t low = 0;

        friend bool operator==(const SquaredDistance&,
                               const SquaredDistance&) = default;
        friend bool operator<(const SquaredDistance& left,
                              const SquaredDistance& right)
        {
            return left.high < right.high ||
                   (left.high == right.high && left.low < right.low);
        }
    };

    struct PendingBatch final
    {
        std::vector<BulwarkDispatchActor> actors{};
        std::size_t submittedActorCount = 0;
        bool acknowledged = false;
        sim::Tick executeTick = 0;
    };

    [[nodiscard]] static SquaredDistance DistanceSquared(
        sim::Vec2 left,
        sim::Vec2 right)
    {
        const auto magnitude = [](std::int32_t a, std::int32_t b)
        {
            const std::int64_t difference =
                static_cast<std::int64_t>(a) - b;
            return static_cast<std::uint64_t>(
                difference < 0 ? -difference : difference);
        };
        const std::uint64_t x = magnitude(left.x.Raw(), right.x.Raw());
        const std::uint64_t y = magnitude(left.y.Raw(), right.y.Raw());
        const std::uint64_t xSquared = x * x;
        const std::uint64_t ySquared = y * y;
        const std::uint64_t low = xSquared + ySquared;
        return {low < xSquared ? 1U : 0U, low};
    }

    std::map<std::uint64_t, PendingBatch> batches_{};
};

[[nodiscard]] inline const char* StableId(
    BulwarkAdmissionResult result)
{
    switch (result)
    {
        case BulwarkAdmissionResult::Applied:
            return "NET_BULWARK_ADMISSION_APPLIED";
        case BulwarkAdmissionResult::Rejected:
            return "NET_BULWARK_ADMISSION_REJECTED";
        case BulwarkAdmissionResult::UnknownBatch:
            return "NET_BULWARK_ADMISSION_UNKNOWN_BATCH";
        case BulwarkAdmissionResult::InvalidBatchId:
            return "NET_BULWARK_ADMISSION_INVALID_BATCH";
        case BulwarkAdmissionResult::InvalidCounts:
            return "NET_BULWARK_ADMISSION_INVALID_COUNTS";
        case BulwarkAdmissionResult::TickOverflow:
            return "NET_BULWARK_ADMISSION_TICK_OVERFLOW";
    }
    return "NET_BULWARK_ADMISSION_UNKNOWN";
}

}  // namespace echoes::network
