#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "EchoesTestSaveEnvironment.h"
#include "EchoesGameplayFeedbackPacket.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEchoesGameplayFeedbackTest,
    "Echoes.Runtime.Gameplay.ActionFeedback",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEchoesGameplayFeedbackTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    using namespace echoes::sim;
    using namespace echoes::feedback;
    FEchoesScopedTestSaveEnvironment Saves(*this);
    if (!Saves.IsReady()) return false;
    FTestWorldWrapper Fixture;
    if (!Fixture.CreateTestWorld(EWorldType::Game)) return false;
    UWorld* World = Fixture.GetTestWorld();
    auto* Bridge = World ? World->GetSubsystem<UEchoesSimulationSubsystem>() : nullptr;
    if (!TestNotNull(TEXT("Feedback bridge exists"), Bridge) ||
        !TestTrue(TEXT("Feedback scenario starts"), Bridge && Bridge->StartPrototypeScenario())) return false;
    auto* Sim = const_cast<Simulation*>(Bridge->GetSimulation());
    if (!TestNotNull(TEXT("Feedback authority exists"), Sim)) return false;
    uint32 WorkerId = 0;
    uint32 CoreId = 0;
    for (const auto& Entity : Sim->Entities())
    {
        if (Entity.owner != 0) continue;
        if (!WorkerId && Entity.type == EntityType::Worker) WorkerId = Entity.id;
        if (!CoreId && Entity.type == EntityType::CommandCore) CoreId = Entity.id;
    }
    if (!TestTrue(TEXT("Fixture owns a worker and core"), WorkerId && CoreId)) return false;
    const uint64 Generation = Bridge->GetScenarioAuthorityGeneration();
    FString Feedback;
    TestTrue(TEXT("Stop is admitted through the gameplay bridge"),
        Bridge->IssueCommand(CommandType::Stop, WorkerId, 0,
            Bridge->SimToWorld(Sim->FindEntity(WorkerId)->position),
            FutureWellChoice::Dormant, Feedback));
    const auto FirstSequence = Bridge->GetLastAcceptedLocalCommandSequence();
    if (!TestTrue(TEXT("Stop has an accepted command sequence"), FirstSequence.IsSet())) return false;
    const uint64 AfterQueueChecksum = Sim->StateChecksum();
    const auto& QueuedFeed = Bridge->GetGameplayFeedbackForPlayer();
    TestTrue(TEXT("Admission exposes queued evidence before execution"),
        std::any_of(QueuedFeed.History().begin(), QueuedFeed.History().end(),
            [&](const auto& Event) { return Event.kind == GameplayFeedbackKind::Queued && Event.sequence == *FirstSequence; }));
    TestEqual(TEXT("Reading feedback never changes authoritative state"), Sim->StateChecksum(), AfterQueueChecksum);
    Bridge->Tick(0.25f); // All fixed catch-up ticks must be observed.
    const auto HasEvent = [&](GameplayFeedbackKind Kind, uint64 Sequence)
    {
        const auto& History = Bridge->GetGameplayFeedbackForPlayer().History();
        return std::any_of(History.begin(), History.end(), [&](const auto& Event)
            { return Event.kind == Kind && Event.sequence == Sequence; });
    };
    TestTrue(TEXT("Actual fixed execution exposes Applied"), HasEvent(GameplayFeedbackKind::Applied, *FirstSequence));

    // Explicit fixture setup creates an already-paid site. Dispatch both
    // commands before their shared execution tick to exercise semantic refusal.
    const Vec2 SitePosition{Sim->FindEntity(CoreId)->position.x + Fixed::FromInt(4),
        Sim->FindEntity(CoreId)->position.y};
    const uint32 SiteId = Sim->SpawnEntity(0, Faction::MeridianCompact,
        EntityType::Dropoff, SitePosition);
    if (!TestTrue(TEXT("Feedback construction site exists"), SiteId != 0)) return false;
    auto* Site = Sim->MutableEntityForTesting(SiteId);
    Site->completed = false;
    Site->constructionProgress = 10;
    Site->constructionRequired = 100;
    Site->constructionInvestedCost = {90, 10};
    Site->hitPoints = 45;
    auto* Worker = Sim->MutableEntityForTesting(WorkerId);
    Worker->position = {SitePosition.x + Fixed::FromInt(1), SitePosition.y};
    Worker->order = {};
    Worker->orderQueue.clear();
    TestTrue(TEXT("Cancellation is admitted before execution"),
        Bridge->IssueConstructionCancellation(SiteId, Feedback));
    TestTrue(TEXT("Assist is admitted while the site still exists"),
        Bridge->IssueConstructionAssistCommand(WorkerId, SiteId, Feedback));
    const auto AssistSequence = Bridge->GetLastAcceptedLocalCommandSequence();
    if (!TestTrue(TEXT("Assist has a command sequence"), AssistSequence.IsSet())) return false;
    Bridge->Tick(0.25f);
    TestTrue(TEXT("Cancellation wins and removes the paid site"), Sim->FindEntity(SiteId) == nullptr);
    TestTrue(TEXT("A queued assist exposes actual NoEffect after cancellation"),
        HasEvent(GameplayFeedbackKind::NoEffect, *AssistSequence));
    const auto& History = Bridge->GetGameplayFeedbackForPlayer().History();
    TestTrue(TEXT("The real cancellation refund is retained for its owner"),
        std::any_of(History.begin(), History.end(), [&](const auto& Event)
            { return Event.kind == GameplayFeedbackKind::ConstructionCancelled &&
                Event.resourcesRefunded == FeedbackResourceDelta{67, 7}; }));
    const auto& OpponentHistory = Bridge->GetGameplayFeedbackForPlayer(1).History();
    TestTrue(TEXT("Another player's history contains only its own receipt scope"),
        std::all_of(OpponentHistory.begin(), OpponentHistory.end(), [&](const auto& Event)
            { return Event.recipient == 1 && Event.actor != WorkerId; }));

    // The Unreal transport preserves exact typed evidence. The receiving
    // observer validates generation, owner, order and enum domains separately.
    GameplayFeedbackState Remote;
    if (!History.empty())
    {
        const auto& Event = History.back();
        (void)Remote.BeginRemoteStream(Event.generation, Event.recipient, Event.eventId - 1, true);
        const auto Packet = FEchoesGameplayFeedbackPacket::FromEvent(Event);
        TestTrue(TEXT("Unreal feedback packet preserves a verified receipt"),
            Remote.IngestRemote(Packet.ToEvent(), Event.recipient, Event.generation) == GameplayFeedbackStatus::Accepted);
        TestEqual(TEXT("Remote observer retains the event ID"), Remote.LastEventId(), Event.eventId);
        auto Invalid = Packet;
        Invalid.EventId = Event.eventId + 1;
        Invalid.Kind = 255;
        TestTrue(TEXT("Unknown transport event kind is refused"),
            Remote.IngestRemote(Invalid.ToEvent(), Event.recipient, Event.generation) == GameplayFeedbackStatus::InvalidEvent);
    }
    Bridge->StopPrototypeScenario();
    TestTrue(TEXT("Stopped authority exposes no stale gameplay feedback"),
        Bridge->GetGameplayFeedbackForPlayer().History().empty());
    TestTrue(TEXT("Replacement authority starts"), Bridge->StartPrototypeScenario());
    TestTrue(TEXT("Replacement authority changes feedback generation"),
        Bridge->GetScenarioAuthorityGeneration() != Generation);
    TestTrue(TEXT("Replacement cannot inherit former action history"),
        Bridge->GetGameplayFeedbackForPlayer().History().empty());
    Bridge->StopPrototypeScenario();
    Fixture.ForwardErrorMessages(this);
    return !HasAnyErrors() && !Fixture.HasFailed();
}
#endif
