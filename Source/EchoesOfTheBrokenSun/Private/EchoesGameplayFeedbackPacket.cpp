#include "EchoesGameplayFeedbackPacket.h"

FEchoesGameplayFeedbackPacket FEchoesGameplayFeedbackPacket::FromEvent(
    const echoes::feedback::GameplayFeedbackEvent& Event)
{
    FEchoesGameplayFeedbackPacket Packet;
    Packet.Generation = Event.generation;
    Packet.EventId = Event.eventId;
    Packet.Recipient = Event.recipient;
    Packet.Tick = Event.tick;
    Packet.Kind = static_cast<uint8>(Event.kind);
    Packet.CommandType = static_cast<uint8>(Event.commandType);
    Packet.ResolutionOutcome = static_cast<uint8>(Event.resolutionOutcome);
    Packet.Sequence = Event.sequence;
    Packet.Actor = Event.actor;
    Packet.Target = Event.target;
    Packet.Producer = Event.producer;
    Packet.ItemId = Event.itemId;
    Packet.EntityType = static_cast<uint8>(Event.entityType);
    Packet.ProductionBlockReason = static_cast<uint8>(Event.productionBlockReason);
    Packet.MatterSpent = Event.resourcesSpent.material;
    Packet.DawnshardsSpent = Event.resourcesSpent.dawnshards;
    Packet.MatterRefunded = Event.resourcesRefunded.material;
    Packet.DawnshardsRefunded = Event.resourcesRefunded.dawnshards;
    Packet.HealthDelta = Event.healthDelta;
    Packet.ProgressDelta = Event.progressDelta;
    Packet.LogisticsDelta = Event.logisticsDelta;
    return Packet;
}

echoes::feedback::GameplayFeedbackEvent FEchoesGameplayFeedbackPacket::ToEvent() const
{
    echoes::feedback::GameplayFeedbackEvent Event;
    Event.generation = Generation;
    Event.eventId = EventId;
    Event.recipient = Recipient;
    Event.tick = Tick;
    Event.kind = static_cast<echoes::feedback::GameplayFeedbackKind>(Kind);
    Event.commandType = static_cast<echoes::sim::CommandType>(CommandType);
    Event.resolutionOutcome = static_cast<echoes::sim::CommandResolutionOutcome>(ResolutionOutcome);
    Event.sequence = Sequence;
    Event.actor = Actor;
    Event.target = Target;
    Event.producer = Producer;
    Event.itemId = ItemId;
    Event.entityType = static_cast<echoes::sim::EntityType>(EntityType);
    Event.productionBlockReason = static_cast<echoes::sim::ProductionStartBlockReason>(ProductionBlockReason);
    Event.resourcesSpent = {MatterSpent, DawnshardsSpent};
    Event.resourcesRefunded = {MatterRefunded, DawnshardsRefunded};
    Event.healthDelta = HealthDelta;
    Event.progressDelta = ProgressDelta;
    Event.logisticsDelta = LogisticsDelta;
    return Event;
}

FEchoesGameplayFeedbackLossPacket FEchoesGameplayFeedbackLossPacket::FromLoss(
    const echoes::feedback::GameplayFeedbackLoss& Loss)
{
    FEchoesGameplayFeedbackLossPacket Packet;
    Packet.bReseedRequired = Loss.reseedRequired;
    Packet.bRetainedHistoryTruncated = Loss.retainedHistoryWasTruncated;
    Packet.EvictedHistoryEvents = Loss.evictedHistoryEvents;
    Packet.UntrackedCommands = Loss.untrackedCommands;
    Packet.LostTransientReceipts = Loss.lostTransientReceipts;
    Packet.MissedObservationTicks = Loss.missedObservationTicks;
    return Packet;
}

echoes::feedback::GameplayFeedbackLoss FEchoesGameplayFeedbackLossPacket::ToLoss() const
{
    echoes::feedback::GameplayFeedbackLoss Loss;
    Loss.reseedRequired = bReseedRequired;
    Loss.retainedHistoryWasTruncated = bRetainedHistoryTruncated;
    Loss.evictedHistoryEvents = EvictedHistoryEvents;
    Loss.untrackedCommands = UntrackedCommands;
    Loss.lostTransientReceipts = LostTransientReceipts;
    Loss.missedObservationTicks = MissedObservationTicks;
    return Loss;
}
