#pragma once

#include "EchoesSimCore/Simulation.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <utility>
#include <vector>

namespace echoes::feedback {

/** Transient feed vocabulary. It is not snapshot, replay, or wire schema. */
enum class GameplayFeedbackKind : std::uint8_t
{
    Queued = 0,
    Applied,
    NoEffect,
    ConstructionCreated,
    ConstructionProgressed,
    ConstructionCompleted,
    ConstructionCancelled,
    RepairApplied,
    ProductionQueued,
    ProductionActivated,
    ProductionCancelled,
    ProductionCompleted,
    ProductionSpawnBlocked,
    ProductionSpawnResumed,
};

struct FeedbackResourceDelta final
{
    std::int64_t material = 0;
    std::int64_t dawnshards = 0;

    friend bool operator==(const FeedbackResourceDelta&,
                           const FeedbackResourceDelta&) = default;
};

/**
 * Exact player-scoped observations. Zero/default fields mean the source
 * receipt did not carry that datum; the feed never derives rates or causes.
 */
struct GameplayFeedbackEvent final
{
    std::uint64_t generation = 0;
    std::uint64_t eventId = 0;
    sim::PlayerId recipient = sim::kNeutralPlayer;
    sim::Tick tick = 0;
    GameplayFeedbackKind kind = GameplayFeedbackKind::Queued;
    sim::CommandType commandType = sim::CommandType::Stop;
    sim::CommandResolutionOutcome resolutionOutcome =
        sim::CommandResolutionOutcome::NoEffect;
    std::uint64_t sequence = 0;
    sim::EntityId actor = 0;
    sim::EntityId target = 0;
    sim::EntityId producer = 0;
    sim::ProductionItemId itemId = 0;
    sim::EntityType entityType = sim::EntityType::Worker;
    sim::ProductionStartBlockReason productionBlockReason =
        sim::ProductionStartBlockReason::None;
    FeedbackResourceDelta resourcesSpent{};
    FeedbackResourceDelta resourcesRefunded{};
    std::int64_t healthDelta = 0;
    std::int64_t progressDelta = 0;
    std::int64_t logisticsDelta = 0;

    friend bool operator==(const GameplayFeedbackEvent&,
                           const GameplayFeedbackEvent&) = default;
};

enum class GameplayFeedbackStatus : std::uint8_t
{
    Accepted = 0,
    NoChange,
    Duplicate,
    OutOfOrder,
    WrongRecipient,
    WrongGeneration,
    InvalidEvent,
    CapacityExceeded,
    ObservationGap,
    ExecutionUnavailable,
};

struct GameplayFeedbackLoss final
{
    bool reseedRequired = false;
    bool retainedHistoryWasTruncated = false;
    std::uint64_t evictedHistoryEvents = 0;
    std::uint64_t untrackedCommands = 0;
    std::uint64_t lostTransientReceipts = 0;
    std::uint64_t missedObservationTicks = 0;

    friend bool operator==(const GameplayFeedbackLoss&,
                           const GameplayFeedbackLoss&) = default;
};

/**
 * Bounded presentation adapter for exact authoritative receipts.
 *
 * Pending commands never evict: the 129th command is refused and raises the
 * reseed signal rather than later fabricating a resolved event. Construction
 * progress and repair receipts are summed from exact source deltas per
 * actor-target pair and flushed every 20 receipt ticks, after 20 inactive
 * ticks, or before a terminal construction event. This preserves exact total
 * deltas without letting continuous work evict discrete alerts. Discrete
 * construction/production transitions, including SpawnBlocked and
 * SpawnResumed, are never suppressed.
 */
class GameplayFeedbackState final
{
public:
    static constexpr std::size_t HistoryLimit = 128;
    static constexpr std::size_t PendingLimit = 128;
    static constexpr std::size_t CoalescerLimit = 128;
    static constexpr sim::Tick CoalesceTicks = 20;

    [[nodiscard]] GameplayFeedbackStatus BeginGeneration(
        std::uint64_t generation,
        sim::PlayerId recipient)
    {
        if (generation == 0)
        {
            return GameplayFeedbackStatus::WrongGeneration;
        }
        if (recipient >= sim::kMaximumPlayers)
        {
            return GameplayFeedbackStatus::WrongRecipient;
        }
        if (generation_ == generation)
        {
            return recipient_ == recipient
                ? GameplayFeedbackStatus::NoChange
                : GameplayFeedbackStatus::WrongRecipient;
        }
        ResetFor(generation, recipient, 0);
        return GameplayFeedbackStatus::Accepted;
    }

    /**
     * Trusted reliable-stream reset. afterEventId is the server-declared
     * retained-history floor; ordinary event ingestion can never change it.
     */
    [[nodiscard]] GameplayFeedbackStatus BeginRemoteStream(
        std::uint64_t generation,
        sim::PlayerId recipient,
        std::uint64_t afterEventId,
        bool historyWasTruncated = false)
    {
        if (generation == 0)
        {
            return GameplayFeedbackStatus::WrongGeneration;
        }
        if (recipient >= sim::kMaximumPlayers)
        {
            return GameplayFeedbackStatus::WrongRecipient;
        }
        ResetFor(generation, recipient, afterEventId);
        loss_.retainedHistoryWasTruncated = historyWasTruncated;
        return GameplayFeedbackStatus::Accepted;
    }

    /** Call only after Simulation::QueueCommand returned true. */
    [[nodiscard]] GameplayFeedbackStatus TrackQueued(
        const sim::Command& command,
        std::uint64_t generation,
        std::optional<sim::Tick> observedTick = std::nullopt)
    {
        const GameplayFeedbackStatus generationStatus =
            EnsureLocalGeneration(generation, command.player);
        if (generationStatus != GameplayFeedbackStatus::Accepted &&
            generationStatus != GameplayFeedbackStatus::NoChange)
        {
            return generationStatus;
        }
        if (command.player != recipient_ || command.sequence == 0 ||
            !IsValidCommandType(command.type))
        {
            return GameplayFeedbackStatus::InvalidEvent;
        }
        if (!sim::Simulation::IsExecutableCommandTick(command.executeTick))
        {
            // Historical envelopes can contain the terminal boundary tick,
            // but Step will never execute it. Expose missing evidence without
            // inventing a result or occupying an unresolvable pending slot.
            MarkLoss(loss_.untrackedCommands);
            return GameplayFeedbackStatus::ExecutionUnavailable;
        }
        if (pending_.contains(command.sequence))
        {
            return GameplayFeedbackStatus::Duplicate;
        }
        if (pending_.size() >= PendingLimit)
        {
            MarkLoss(loss_.untrackedCommands);
            return GameplayFeedbackStatus::CapacityExceeded;
        }
        GameplayFeedbackEvent event{};
        event.recipient = command.player;
        // The caller can supply Simulation::CurrentTick() so this is the
        // admission observation, not the command's future execution tick.
        event.tick = observedTick.value_or(command.executeTick);
        event.kind = GameplayFeedbackKind::Queued;
        event.commandType = command.type;
        event.sequence = command.sequence;
        event.actor = command.actor;
        event.target = command.target;
        if (!AppendLocal(event))
        {
            MarkLoss(loss_.untrackedCommands);
            return GameplayFeedbackStatus::CapacityExceeded;
        }
        pending_.emplace(
            command.sequence,
            PendingCommand{
                command.executeTick,
                command.type,
                command.actor,
                command.target});
        return GameplayFeedbackStatus::Accepted;
    }

    /** Call once immediately after every completed fixed Simulation::Step. */
    [[nodiscard]] GameplayFeedbackStatus ObserveFixedStep(
        const sim::Simulation& simulation,
        sim::PlayerId player,
        std::uint64_t generation)
    {
        const GameplayFeedbackStatus generationStatus =
            EnsureLocalGeneration(generation, player);
        if (generationStatus != GameplayFeedbackStatus::Accepted &&
            generationStatus != GameplayFeedbackStatus::NoChange)
        {
            return generationStatus;
        }
        const sim::Tick currentTick = simulation.CurrentTick();
        if (hasObservedTick_ && currentTick < lastObservedTick_)
        {
            loss_.reseedRequired = true;
            return GameplayFeedbackStatus::OutOfOrder;
        }
        if (hasObservedTick_ && currentTick == lastObservedTick_)
        {
            return GameplayFeedbackStatus::Duplicate;
        }
        GameplayFeedbackStatus result = GameplayFeedbackStatus::Accepted;
        if (hasObservedTick_ && currentTick - lastObservedTick_ > 1)
        {
            const sim::Tick missed = currentTick - lastObservedTick_ - 1;
            SaturatingAdd(loss_.missedObservationTicks, missed);
            loss_.reseedRequired = true;
            result = GameplayFeedbackStatus::ObservationGap;
        }

        ResolvePending(simulation, currentTick);
        const std::optional<sim::PlayerView> view =
            simulation.CreatePlayerView(player);
        if (!view.has_value())
        {
            return GameplayFeedbackStatus::WrongRecipient;
        }
        IngestScopedReceipts(*view);
        hasObservedTick_ = true;
        lastObservedTick_ = currentTick;
        return result;
    }

    /**
     * Exact-next ingestion for a future recipient-only reliable RPC. A gap,
     * generation change, or owner mismatch is rejected and never rebases the
     * stream; BeginRemoteStream is the only trusted remote reset path.
     */
    [[nodiscard]] GameplayFeedbackStatus IngestRemote(
        const GameplayFeedbackEvent& event,
        sim::PlayerId expectedRecipient,
        std::uint64_t expectedGeneration)
    {
        if (expectedRecipient >= sim::kMaximumPlayers ||
            event.recipient != expectedRecipient)
        {
            return GameplayFeedbackStatus::WrongRecipient;
        }
        if (expectedGeneration == 0 || event.generation != expectedGeneration ||
            generation_ != expectedGeneration)
        {
            return GameplayFeedbackStatus::WrongGeneration;
        }
        if (recipient_ != expectedRecipient)
        {
            return GameplayFeedbackStatus::WrongRecipient;
        }
        if (!ValidateEvent(event))
        {
            return GameplayFeedbackStatus::InvalidEvent;
        }
        if (event.eventId <= lastEventId_)
        {
            return GameplayFeedbackStatus::Duplicate;
        }
        if (lastEventId_ == std::numeric_limits<std::uint64_t>::max() ||
            event.eventId != lastEventId_ + 1)
        {
            loss_.reseedRequired = true;
            return GameplayFeedbackStatus::OutOfOrder;
        }
        AppendRetained(event);
        lastEventId_ = event.eventId;
        return GameplayFeedbackStatus::Accepted;
    }

    /**
     * Merge an owning server publisher's cumulative loss report. Counters are
     * maxima, so reliable retransmission cannot double count. This reports an
     * incomplete source but never changes generation, event ID, or history;
     * only BeginRemoteStream may establish a new stream cursor.
     */
    [[nodiscard]] GameplayFeedbackStatus ReportRemoteObservationLoss(
        const GameplayFeedbackLoss& source,
        sim::PlayerId expectedRecipient,
        std::uint64_t expectedGeneration)
    {
        if (expectedRecipient >= sim::kMaximumPlayers ||
            recipient_ != expectedRecipient)
        {
            return GameplayFeedbackStatus::WrongRecipient;
        }
        if (expectedGeneration == 0 || generation_ != expectedGeneration)
        {
            return GameplayFeedbackStatus::WrongGeneration;
        }
        const GameplayFeedbackLoss before = loss_;
        loss_.reseedRequired =
            loss_.reseedRequired || source.reseedRequired;
        loss_.retainedHistoryWasTruncated =
            loss_.retainedHistoryWasTruncated ||
            source.retainedHistoryWasTruncated;
        loss_.evictedHistoryEvents = std::max(
            loss_.evictedHistoryEvents, source.evictedHistoryEvents);
        loss_.untrackedCommands = std::max(
            loss_.untrackedCommands, source.untrackedCommands);
        loss_.lostTransientReceipts = std::max(
            loss_.lostTransientReceipts, source.lostTransientReceipts);
        loss_.missedObservationTicks = std::max(
            loss_.missedObservationTicks, source.missedObservationTicks);
        return loss_ == before
            ? GameplayFeedbackStatus::NoChange
            : GameplayFeedbackStatus::Accepted;
    }

    void ClearReseedSignal()
    {
        loss_.reseedRequired = false;
        loss_.retainedHistoryWasTruncated = false;
    }

    void Reset()
    {
        generation_ = 0;
        recipient_ = sim::kNeutralPlayer;
        lastEventId_ = 0;
        lastObservedTick_ = 0;
        hasObservedTick_ = false;
        history_.clear();
        pending_.clear();
        constructionProgress_.clear();
        repairProgress_.clear();
        loss_ = {};
    }

    [[nodiscard]] std::uint64_t Generation() const { return generation_; }
    [[nodiscard]] sim::PlayerId Recipient() const { return recipient_; }
    [[nodiscard]] std::uint64_t LastEventId() const { return lastEventId_; }
    [[nodiscard]] std::uint64_t FirstRetainedEventId() const
    {
        return history_.empty() ? lastEventId_ : history_.front().eventId;
    }
    [[nodiscard]] std::size_t PendingCount() const { return pending_.size(); }
    [[nodiscard]] const std::vector<GameplayFeedbackEvent>& History() const
    {
        return history_;
    }
    [[nodiscard]] const GameplayFeedbackLoss& Loss() const { return loss_; }

private:
    struct PendingCommand final
    {
        sim::Tick executeTick = 0;
        sim::CommandType type = sim::CommandType::Stop;
        sim::EntityId actor = 0;
        sim::EntityId target = 0;
    };

    struct ProgressAccumulator final
    {
        sim::Tick firstTick = 0;
        sim::Tick lastTick = 0;
        sim::EntityId actor = 0;
        sim::EntityId target = 0;
        std::uint64_t sequence = 0;
        std::int64_t progress = 0;
        std::int64_t health = 0;
        std::int64_t matter = 0;
    };

    using ActivityKey = std::pair<sim::EntityId, sim::EntityId>;

    [[nodiscard]] GameplayFeedbackStatus EnsureLocalGeneration(
        std::uint64_t generation,
        sim::PlayerId recipient)
    {
        if (generation == 0)
        {
            return GameplayFeedbackStatus::WrongGeneration;
        }
        if (recipient >= sim::kMaximumPlayers)
        {
            return GameplayFeedbackStatus::WrongRecipient;
        }
        if (generation_ != generation)
        {
            ResetFor(generation, recipient, 0);
            return GameplayFeedbackStatus::Accepted;
        }
        return recipient_ == recipient
            ? GameplayFeedbackStatus::NoChange
            : GameplayFeedbackStatus::WrongRecipient;
    }

    void ResetFor(
        std::uint64_t generation,
        sim::PlayerId recipient,
        std::uint64_t lastEventId)
    {
        generation_ = generation;
        recipient_ = recipient;
        lastEventId_ = lastEventId;
        lastObservedTick_ = 0;
        hasObservedTick_ = false;
        history_.clear();
        pending_.clear();
        constructionProgress_.clear();
        repairProgress_.clear();
        loss_ = {};
    }

    [[nodiscard]] bool AppendLocal(GameplayFeedbackEvent event)
    {
        if (lastEventId_ == std::numeric_limits<std::uint64_t>::max())
        {
            loss_.reseedRequired = true;
            return false;
        }
        event.generation = generation_;
        event.eventId = lastEventId_ + 1;
        event.recipient = recipient_;
        if (!ValidateEvent(event))
        {
            return false;
        }
        lastEventId_ = event.eventId;
        AppendRetained(event);
        return true;
    }

    void AppendRetained(const GameplayFeedbackEvent& event)
    {
        if (history_.size() == HistoryLimit)
        {
            history_.erase(history_.begin());
            // A retained-history rollover is observable, but it does not make
            // the live exact-next stream incomplete for an up-to-date reader.
            SaturatingAdd(loss_.evictedHistoryEvents, 1);
        }
        history_.push_back(event);
    }

    void ResolvePending(
        const sim::Simulation& simulation,
        sim::Tick currentTick)
    {
        for (auto pending = pending_.begin(); pending != pending_.end();)
        {
            const std::optional<sim::CommandResolutionReceipt> receipt =
                simulation.FindCommandResolutionReceipt(
                    recipient_, pending->first);
            if (receipt.has_value())
            {
                if (receipt->player != recipient_ ||
                    receipt->commandType != pending->second.type)
                {
                    MarkLoss(loss_.untrackedCommands);
                    pending = pending_.erase(pending);
                    continue;
                }
                GameplayFeedbackEvent event{};
                event.recipient = recipient_;
                event.tick = receipt->assignedExecutionTick;
                event.kind =
                    receipt->outcome ==
                            sim::CommandResolutionOutcome::Applied
                        ? GameplayFeedbackKind::Applied
                        : GameplayFeedbackKind::NoEffect;
                event.commandType = receipt->commandType;
                event.resolutionOutcome = receipt->outcome;
                event.sequence = pending->first;
                event.actor = pending->second.actor;
                event.target = pending->second.target;
                if (!AppendLocal(event))
                {
                    MarkLoss(loss_.untrackedCommands);
                }
                pending = pending_.erase(pending);
                continue;
            }
            const sim::Tick executeTick = pending->second.executeTick;
            const bool receiptExpired =
                executeTick <=
                    std::numeric_limits<sim::Tick>::max() -
                        sim::kCommandResolutionReceiptRetentionTicks &&
                currentTick >
                    executeTick + sim::kCommandResolutionReceiptRetentionTicks;
            if (receiptExpired)
            {
                MarkLoss(loss_.untrackedCommands);
                pending = pending_.erase(pending);
                continue;
            }
            ++pending;
        }
    }

    void IngestScopedReceipts(const sim::PlayerView& view)
    {
        std::vector<ActivityKey> activeConstruction;
        for (const sim::ConstructionReceipt& receipt :
             view.ConstructionReceipts())
        {
            if (receipt.transition == sim::ConstructionTransition::Progressed)
            {
                activeConstruction.emplace_back(
                    receipt.worker, receipt.structure);
            }
        }
        FlushInactive(
            constructionProgress_, activeConstruction,
            view.CurrentTick(), true);

        std::vector<ActivityKey> activeRepair;
        activeRepair.reserve(view.RepairReceipts().size());
        for (const sim::RepairReceipt& receipt : view.RepairReceipts())
        {
            activeRepair.emplace_back(receipt.worker, receipt.target);
        }
        FlushInactive(
            repairProgress_, activeRepair,
            view.CurrentTick(), false);

        for (const sim::ConstructionReceipt& receipt :
             view.ConstructionReceipts())
        {
            if (receipt.transition == sim::ConstructionTransition::Progressed)
            {
                AccumulateConstruction(receipt);
                continue;
            }
            FlushConstructionTarget(receipt.structure);
            GameplayFeedbackEvent event{};
            event.recipient = recipient_;
            event.tick = receipt.tick;
            event.kind = ConstructionKind(receipt.transition);
            event.sequence = receipt.commandSequence;
            event.actor = receipt.worker;
            event.target = receipt.structure;
            event.progressDelta = receipt.progressDelta;
            event.resourcesSpent = ToDelta(receipt.charged);
            event.resourcesRefunded = ToDelta(receipt.refund);
            if (!AppendLocal(event))
            {
                MarkLoss(loss_.lostTransientReceipts);
            }
        }
        for (const sim::RepairReceipt& receipt : view.RepairReceipts())
        {
            AccumulateRepair(receipt);
        }
        for (const sim::ProductionTransitionReceipt& receipt :
             view.ProductionTransitions())
        {
            GameplayFeedbackEvent event{};
            event.recipient = recipient_;
            event.tick = receipt.tick;
            event.kind = ProductionKind(receipt.transition);
            event.sequence = receipt.commandSequence;
            event.actor = receipt.spawnedEntity;
            event.producer = receipt.producer;
            event.itemId = receipt.itemId;
            event.entityType = receipt.unitType;
            event.productionBlockReason = receipt.blockReason;
            event.resourcesSpent = ToDelta(receipt.charged);
            event.resourcesRefunded = ToDelta(receipt.refunded);
            event.logisticsDelta = receipt.logisticsDelta;
            if (!AppendLocal(event))
            {
                MarkLoss(loss_.lostTransientReceipts);
            }
        }
    }

    void AccumulateConstruction(const sim::ConstructionReceipt& receipt)
    {
        const ActivityKey key{receipt.worker, receipt.structure};
        ProgressAccumulator* accumulator = FindOrCreate(
            constructionProgress_, key, receipt.tick,
            receipt.worker, receipt.structure, receipt.commandSequence);
        if (accumulator == nullptr)
        {
            return;
        }
        accumulator->lastTick = receipt.tick;
        SaturatingAdd(accumulator->progress, receipt.progressDelta);
        if (receipt.tick >= accumulator->firstTick &&
            receipt.tick - accumulator->firstTick >= CoalesceTicks - 1)
        {
            FlushConstruction(key);
        }
    }

    void AccumulateRepair(const sim::RepairReceipt& receipt)
    {
        const ActivityKey key{receipt.worker, receipt.target};
        ProgressAccumulator* accumulator = FindOrCreate(
            repairProgress_, key, receipt.tick,
            receipt.worker, receipt.target, 0);
        if (accumulator == nullptr)
        {
            return;
        }
        accumulator->lastTick = receipt.tick;
        SaturatingAdd(accumulator->health, receipt.hitPointsRestored);
        SaturatingAdd(accumulator->matter, receipt.matterSpent);
        if (receipt.tick >= accumulator->firstTick &&
            receipt.tick - accumulator->firstTick >= CoalesceTicks - 1)
        {
            FlushRepair(key);
        }
    }

    template <typename Map>
    [[nodiscard]] ProgressAccumulator* FindOrCreate(
        Map& accumulators,
        const ActivityKey& key,
        sim::Tick tick,
        sim::EntityId actor,
        sim::EntityId target,
        std::uint64_t sequence)
    {
        auto found = accumulators.find(key);
        if (found != accumulators.end())
        {
            return &found->second;
        }
        if (accumulators.size() >= CoalescerLimit)
        {
            MarkLoss(loss_.lostTransientReceipts);
            return nullptr;
        }
        const auto [inserted, created] = accumulators.emplace(
            key,
            ProgressAccumulator{
                tick, tick, actor, target, sequence, 0, 0, 0});
        (void)created;
        return &inserted->second;
    }

    template <typename Map>
    void FlushInactive(
        Map& accumulators,
        const std::vector<ActivityKey>& active,
        sim::Tick currentTick,
        bool construction)
    {
        std::vector<ActivityKey> inactive;
        for (const auto& [key, accumulator] : accumulators)
        {
            (void)accumulator;
            if (std::find(active.begin(), active.end(), key) == active.end() &&
                currentTick >= accumulator.lastTick &&
                currentTick - accumulator.lastTick >= CoalesceTicks)
            {
                inactive.push_back(key);
            }
        }
        for (const ActivityKey& key : inactive)
        {
            if (construction)
            {
                FlushConstruction(key);
            }
            else
            {
                FlushRepair(key);
            }
        }
    }

    void FlushConstructionTarget(sim::EntityId target)
    {
        std::vector<ActivityKey> matching;
        for (const auto& [key, accumulator] : constructionProgress_)
        {
            (void)accumulator;
            if (key.second == target)
            {
                matching.push_back(key);
            }
        }
        for (const ActivityKey& key : matching)
        {
            FlushConstruction(key);
        }
    }

    void FlushConstruction(const ActivityKey& key)
    {
        const auto found = constructionProgress_.find(key);
        if (found == constructionProgress_.end())
        {
            return;
        }
        const ProgressAccumulator value = found->second;
        constructionProgress_.erase(found);
        GameplayFeedbackEvent event{};
        event.recipient = recipient_;
        event.tick = value.lastTick;
        event.kind = GameplayFeedbackKind::ConstructionProgressed;
        event.sequence = value.sequence;
        event.actor = value.actor;
        event.target = value.target;
        event.progressDelta = value.progress;
        if (!AppendLocal(event))
        {
            MarkLoss(loss_.lostTransientReceipts);
        }
    }

    void FlushRepair(const ActivityKey& key)
    {
        const auto found = repairProgress_.find(key);
        if (found == repairProgress_.end())
        {
            return;
        }
        const ProgressAccumulator value = found->second;
        repairProgress_.erase(found);
        GameplayFeedbackEvent event{};
        event.recipient = recipient_;
        event.tick = value.lastTick;
        event.kind = GameplayFeedbackKind::RepairApplied;
        event.actor = value.actor;
        event.target = value.target;
        event.resourcesSpent.material = value.matter;
        event.healthDelta = value.health;
        if (!AppendLocal(event))
        {
            MarkLoss(loss_.lostTransientReceipts);
        }
    }

    [[nodiscard]] static GameplayFeedbackKind ConstructionKind(
        sim::ConstructionTransition transition)
    {
        switch (transition)
        {
            case sim::ConstructionTransition::Created:
                return GameplayFeedbackKind::ConstructionCreated;
            case sim::ConstructionTransition::Progressed:
                return GameplayFeedbackKind::ConstructionProgressed;
            case sim::ConstructionTransition::Completed:
                return GameplayFeedbackKind::ConstructionCompleted;
            case sim::ConstructionTransition::Cancelled:
                return GameplayFeedbackKind::ConstructionCancelled;
        }
        return GameplayFeedbackKind::ConstructionProgressed;
    }

    [[nodiscard]] static GameplayFeedbackKind ProductionKind(
        sim::ProductionTransition transition)
    {
        switch (transition)
        {
            case sim::ProductionTransition::Queued:
                return GameplayFeedbackKind::ProductionQueued;
            case sim::ProductionTransition::Activated:
                return GameplayFeedbackKind::ProductionActivated;
            case sim::ProductionTransition::Cancelled:
                return GameplayFeedbackKind::ProductionCancelled;
            case sim::ProductionTransition::Completed:
                return GameplayFeedbackKind::ProductionCompleted;
            case sim::ProductionTransition::SpawnBlocked:
                return GameplayFeedbackKind::ProductionSpawnBlocked;
            case sim::ProductionTransition::SpawnResumed:
                return GameplayFeedbackKind::ProductionSpawnResumed;
        }
        return GameplayFeedbackKind::ProductionQueued;
    }

    [[nodiscard]] static FeedbackResourceDelta ToDelta(
        const sim::ResourcePool& resources)
    {
        return {resources.material, resources.dawnshards};
    }

    [[nodiscard]] static bool IsValidCommandType(sim::CommandType type)
    {
        return static_cast<std::uint8_t>(type) <=
               static_cast<std::uint8_t>(
                   sim::CommandType::CancelConstruction);
    }

    [[nodiscard]] static bool IsValidResolution(
        sim::CommandResolutionOutcome outcome)
    {
        return static_cast<std::uint8_t>(outcome) <=
               static_cast<std::uint8_t>(
                   sim::CommandResolutionOutcome::DestinationOccupied);
    }

    [[nodiscard]] static bool NonNegative(const FeedbackResourceDelta& delta)
    {
        return delta.material >= 0 && delta.dawnshards >= 0;
    }

    [[nodiscard]] static bool ValidateEvent(
        const GameplayFeedbackEvent& event)
    {
        if (event.generation == 0 || event.eventId == 0 ||
            event.recipient >= sim::kMaximumPlayers ||
            !NonNegative(event.resourcesSpent) ||
            !NonNegative(event.resourcesRefunded) ||
            event.healthDelta < 0 || event.progressDelta < 0 ||
            !IsValidCommandType(event.commandType) ||
            !IsValidResolution(event.resolutionOutcome) ||
            static_cast<std::uint8_t>(event.entityType) >
                static_cast<std::uint8_t>(sim::EntityType::UtilityStructure) ||
            static_cast<std::uint8_t>(event.productionBlockReason) >
                static_cast<std::uint8_t>(
                    sim::ProductionStartBlockReason::MobileEntityLimit))
        {
            return false;
        }
        switch (event.kind)
        {
            case GameplayFeedbackKind::Queued:
                return event.sequence != 0;
            case GameplayFeedbackKind::Applied:
                return event.sequence != 0 &&
                       event.resolutionOutcome ==
                           sim::CommandResolutionOutcome::Applied;
            case GameplayFeedbackKind::NoEffect:
                return event.sequence != 0 &&
                       event.resolutionOutcome !=
                           sim::CommandResolutionOutcome::Applied;
            case GameplayFeedbackKind::ConstructionCreated:
            case GameplayFeedbackKind::ConstructionCompleted:
            case GameplayFeedbackKind::ConstructionCancelled:
                return event.target != 0;
            case GameplayFeedbackKind::ConstructionProgressed:
                return event.actor != 0 && event.target != 0 &&
                       event.progressDelta > 0;
            case GameplayFeedbackKind::RepairApplied:
                return event.actor != 0 && event.target != 0 &&
                       event.healthDelta > 0;
            case GameplayFeedbackKind::ProductionQueued:
            case GameplayFeedbackKind::ProductionActivated:
            case GameplayFeedbackKind::ProductionCancelled:
            case GameplayFeedbackKind::ProductionCompleted:
            case GameplayFeedbackKind::ProductionSpawnBlocked:
            case GameplayFeedbackKind::ProductionSpawnResumed:
                return event.producer != 0 && event.itemId != 0;
        }
        return false;
    }

    static void SaturatingAdd(std::uint64_t& target, std::uint64_t value)
    {
        target = value > std::numeric_limits<std::uint64_t>::max() - target
            ? std::numeric_limits<std::uint64_t>::max()
            : target + value;
    }

    static void SaturatingAdd(std::int64_t& target, std::int64_t value)
    {
        if (value <= 0)
        {
            return;
        }
        target = value > std::numeric_limits<std::int64_t>::max() - target
            ? std::numeric_limits<std::int64_t>::max()
            : target + value;
    }

    void MarkLoss(std::uint64_t& counter)
    {
        SaturatingAdd(counter, 1);
        loss_.reseedRequired = true;
    }

    std::uint64_t generation_ = 0;
    sim::PlayerId recipient_ = sim::kNeutralPlayer;
    std::uint64_t lastEventId_ = 0;
    sim::Tick lastObservedTick_ = 0;
    bool hasObservedTick_ = false;
    std::vector<GameplayFeedbackEvent> history_{};
    std::map<std::uint64_t, PendingCommand> pending_{};
    std::map<ActivityKey, ProgressAccumulator> constructionProgress_{};
    std::map<ActivityKey, ProgressAccumulator> repairProgress_{};
    GameplayFeedbackLoss loss_{};
};

}  // namespace echoes::feedback
