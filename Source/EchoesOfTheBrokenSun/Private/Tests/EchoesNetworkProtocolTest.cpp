#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "EchoesNetworkSession.h"
#include "EchoesSimCore/NetworkProtocol.h"

#include <algorithm>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesNetworkProtocolTest,
    "Echoes.Runtime.Network.ProtocolAdmission",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesNetworkProtocolTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    FEchoesScopedTestSaveEnvironment TestSaveEnvironment(*this);
    if (!TestSaveEnvironment.IsReady())
    {
        return false;
    }
    using namespace echoes::sim;
    using namespace echoes::sim::net;

    CompatibilityManifest Authority{};
    Authority.simulationRulesVersion = DefaultSimulationRules().version;
    Authority.buildIdSha256[0] = 0x42;
    Authority.rulesPackSha256[0] = 0x52;
    Authority.mapPackSha256[0] = 0x62;
    Authority.matchSettingsSha256[0] = 0x72;

    const std::vector<std::uint8_t> Hello =
        EncodeCompatibilityHello(Authority);
    CompatibilityManifest Remote{};
    TestTrue(TEXT("Unreal decodes the canonical compatibility packet"),
             DecodeCompatibilityHello(Hello, Remote) == DecodeStatus::Ok);
    TestTrue(TEXT("Decoded compatibility manifest is exact"),
             Remote == Authority);
    TestTrue(TEXT("Exact manifest is admitted before scenario setup"),
             CheckCompatibility(Authority, Remote) ==
                 CompatibilityStatus::Accepted);
    Remote.mapPackSha256[0] ^= 1;
    TestTrue(TEXT("Map mismatch fails closed with a stable status"),
             CheckCompatibility(Authority, Remote) ==
                 CompatibilityStatus::MapPackMismatch &&
                 StableId(CompatibilityStatus::MapPackMismatch) ==
                     "NET_MAP_PACK_MISMATCH");

    echoes::network::CommandRateLimiter RateLimiter;
    bool bInitialCommandBudgetAccepted = true;
    for (std::uint32_t Index = 0;
         Index < echoes::network::CommandRateLimiter::MaximumCommandsPerWindow;
         ++Index)
    {
        bInitialCommandBudgetAccepted &= RateLimiter.TryConsume(10.0);
    }
    TestTrue(TEXT("Per-connection command budget accepts eight requests"),
             bInitialCommandBudgetAccepted &&
                 RateLimiter.CurrentCount() == 8);
    TestFalse(TEXT("Per-connection command budget rejects the ninth request"),
              RateLimiter.TryConsume(10.5));
    TestFalse(TEXT("Per-connection command budget rejects regressive time"),
              RateLimiter.TryConsume(9.0));
    TestTrue(TEXT("Per-connection command budget reopens after one second"),
             RateLimiter.TryConsume(11.0) &&
                 RateLimiter.CurrentCount() == 1);

    echoes::network::CommandRateLimiter IntentLimiter;
    TestTrue(TEXT("One bounded batch may consume the full intent budget"),
             IntentLimiter.TryConsume(
                 20.0,
                 echoes::network::CommandRateLimiter::MaximumIntentsPerWindow) &&
                 IntentLimiter.CurrentCount() == 1 &&
                 IntentLimiter.CurrentIntentCount() == 1024);
    TestFalse(TEXT("Intent budget rejects another request in the same window"),
              IntentLimiter.TryConsume(20.5, 1));
    TestFalse(TEXT("A single over-budget intent count fails closed"),
              IntentLimiter.TryConsume(21.0, 1025));

    const CompatibilityManifest ClientManifest =
        echoes::network::BuildCompatibilityManifest();
    constexpr Digest256 ExpectedBuildId{
        0x2d, 0xf0, 0x78, 0xeb, 0xb4, 0x66, 0xe9, 0x52,
        0xbb, 0xb5, 0x89, 0x7a, 0xb7, 0xc6, 0x63, 0x1b,
        0xd8, 0x70, 0xb8, 0x28, 0x7d, 0x4e, 0x4e, 0x0f,
        0xd5, 0xd2, 0x18, 0x2e, 0xc2, 0x2b, 0x6e, 0xb7};
    TestTrue(TEXT("Compatibility identity is bound to version 0.93.0 and schema 23"),
             ClientManifest.buildIdSha256 == ExpectedBuildId);
    SimulationConfig RuntimeConfig{16, 16, 20, 77};
    RuntimeConfig.rules.contentSha256 = ClientManifest.rulesPackSha256;
    Simulation RuntimeSimulation(RuntimeConfig);
    TestTrue(TEXT("Authority creates the remote seat"),
             RuntimeSimulation.AddPlayer(
                 1, Faction::KharuunAssemblies, {1000, 500}));
    TestTrue(TEXT("Authority creates the opposing seat"),
             RuntimeSimulation.AddPlayer(
                 0, Faction::MeridianCompact, {1000, 500}));
    const EntityId Worker = RuntimeSimulation.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::Worker,
        Vec2::FromTiles(4, 4));
    TestTrue(TEXT("Authority creates the remote worker"), Worker != 0);

    CommandRequest Request{};
    Request.sequence = 1;
    Request.executeTick = 3;
    Request.type = CommandType::Move;
    Request.actor = Worker;
    Request.position = Vec2::FromTiles(5, 4);
    const std::vector<std::uint8_t> RequestBytes =
        EncodeCommandRequest(Request);
    CommandRequest DecodedRequest{};
    TestTrue(TEXT("Unreal decodes the canonical command packet"),
             DecodeCommandRequest(RequestBytes, DecodedRequest) ==
                 DecodeStatus::Ok);
    TestTrue(TEXT("Decoded command request is exact"),
             DecodedRequest == Request);

    CommandBatchRequest Batch{};
    Batch.clientBatchId = 1;
    CommandIntent FirstIntent{};
    FirstIntent.type = CommandType::Move;
    FirstIntent.actor = 10;
    FirstIntent.position = Vec2::FromTiles(5, 4);
    CommandIntent SecondIntent = FirstIntent;
    SecondIntent.actor = 20;
    SecondIntent.position = Vec2::FromTiles(6, 4);
    Batch.intents = {FirstIntent, SecondIntent};
    const std::vector<std::uint8_t> BatchBytes =
        EncodeCommandBatchRequest(Batch);
    const TArray<uint8> UnrealBatchBytes =
        echoes::network::ToByteArray(BatchBytes);
    CommandBatchRequest DecodedBatch{};
    TestTrue(TEXT("Unreal byte arrays preserve canonical multi-actor batches"),
             !UnrealBatchBytes.IsEmpty() &&
                 DecodeCommandBatchRequest(
                     echoes::network::AsByteSpan(UnrealBatchBytes),
                     DecodedBatch) == DecodeStatus::Ok &&
                 DecodedBatch == Batch);
    std::vector<std::uint8_t> TamperedBatch = BatchBytes;
    TamperedBatch.back() ^= 1;
    TestTrue(TEXT("Tampered multi-actor batches fail closed"),
             DecodeCommandBatchRequest(TamperedBatch, DecodedBatch) ==
                 DecodeStatus::IntegrityMismatch);

    CommandAdmissionContext Context{};
    Context.player = 1;
    std::string Rejection;
    TestTrue(TEXT("Connection-bound seat supplies player identity at admission"),
             AdmitCommandRequest(
                 DecodedRequest, Context, RuntimeSimulation, &Rejection) ==
                 CommandAdmissionStatus::Accepted);
    TestTrue(TEXT("Client packet cannot override server-derived player"),
             !RuntimeSimulation.CommandLog().empty() &&
                 RuntimeSimulation.CommandLog().back().player == 1);
    TestTrue(TEXT("Duplicate sequence fails before simulation mutation"),
             AdmitCommandRequest(
                 DecodedRequest, Context, RuntimeSimulation, &Rejection) ==
                 CommandAdmissionStatus::SequenceUnexpected &&
                 RuntimeSimulation.CommandLog().size() == 1);

    const CompatibilityManifest ConfiguredManifest =
        echoes::network::BuildCompatibilityManifest(&RuntimeSimulation);
    TestTrue(TEXT("Runtime and client construct one exact compatibility identity"),
             ConfiguredManifest == ClientManifest);
    TestTrue(TEXT("Ordinary deterministic simulation admits network compatibility"),
             echoes::network::SupportsNetworkSession(&RuntimeSimulation));

    SimulationConfig ProtectedConfig = RuntimeConfig;
    ProtectedConfig.protectedCommandCorePlayerMask = 0x01;
    Simulation ProtectedSimulation(ProtectedConfig);
    const CompatibilityManifest ProtectedManifest =
        echoes::network::BuildCompatibilityManifest(&ProtectedSimulation);
    TestFalse(TEXT("Protected-Core endurance fixture is categorically non-networked"),
              echoes::network::SupportsNetworkSession(&ProtectedSimulation));
    TestTrue(TEXT("Protected-Core fixture carries a rejecting match identity"),
             ProtectedManifest.matchSettingsSha256 !=
                     ClientManifest.matchSettingsSha256 &&
                 CheckCompatibility(ProtectedManifest, ClientManifest) ==
                     CompatibilityStatus::MatchSettingsMismatch);

    const EntityId HiddenLocalWorker = RuntimeSimulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Worker,
        Vec2::FromTiles(14, 14));
    const EntityId VisibleHostile = RuntimeSimulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Soldier,
        Vec2::FromTiles(5, 4));
    TestTrue(TEXT("Authority creates scoped-state fixtures"),
             HiddenLocalWorker != 0 && VisibleHostile != 0);
    const std::optional<PlayerView> RemoteView =
        RuntimeSimulation.CreatePlayerView(1);
    TestTrue(TEXT("Authority materializes the admitted seat view"),
             RemoteView.has_value());
    if (!RemoteView.has_value())
    {
        return false;
    }

    ScopedViewKeyframe Keyframe{};
    TestTrue(TEXT("PlayerView produces a bounded canonical keyframe"),
             BuildScopedViewKeyframe(
                 *RemoteView,
                 9,
                 Context.lastAcceptedSequence,
                 Keyframe,
                 &Rejection));
    TestTrue(TEXT("Hidden authority entity is absent from scoped state"),
             std::none_of(
                 Keyframe.entities.begin(),
                 Keyframe.entities.end(),
                 [&](const ScopedEntityState& Entity)
                 {
                     return Entity.id == HiddenLocalWorker;
                 }));
    const auto VisibleHostileState = std::find_if(
        Keyframe.entities.begin(),
        Keyframe.entities.end(),
        [&](const ScopedEntityState& Entity)
        {
            return Entity.id == VisibleHostile;
        });
    TestTrue(TEXT("Visible hostile is disclosed with private health redacted"),
             VisibleHostileState != Keyframe.entities.end() &&
                 VisibleHostileState->hitPoints == 1 &&
                 VisibleHostileState->maxHitPoints == 1);

    std::vector<std::uint8_t> KeyframeBytes =
        EncodeScopedViewKeyframe(Keyframe);
    ScopedViewKeyframe DecodedKeyframe{};
    TestTrue(TEXT("Scoped keyframe round-trips exactly"),
             DecodeScopedViewKeyframe(
                 KeyframeBytes, DecodedKeyframe) == DecodeStatus::Ok &&
                 DecodedKeyframe == Keyframe);
    KeyframeBytes.back() ^= 1;
    TestTrue(TEXT("Tampered scoped keyframe fails closed"),
             DecodeScopedViewKeyframe(
                 KeyframeBytes, DecodedKeyframe) ==
                 DecodeStatus::IntegrityMismatch);

    ScopedViewKeyframe DeltaTarget = Keyframe;
    ++DeltaTarget.snapshotId;
    ++DeltaTarget.simulationTick;
    ++DeltaTarget.resources.material;
    TestTrue(TEXT("Delta fixture contains a scoped entity"),
             !DeltaTarget.entities.empty());
    if (DeltaTarget.entities.empty())
    {
        return false;
    }
    DeltaTarget.entities.front().position = Vec2::FromRaw(
        DeltaTarget.entities.front().position.x.Raw() + 1,
        DeltaTarget.entities.front().position.y.Raw());
    const std::vector<std::uint8_t> DeltaTargetBytes =
        EncodeScopedViewKeyframe(DeltaTarget);
    TestTrue(TEXT("Delta target canonicalizes as a full scoped state"),
             !DeltaTargetBytes.empty() &&
                 DecodeScopedViewKeyframe(
                     DeltaTargetBytes, DeltaTarget) == DecodeStatus::Ok);

    ScopedViewDelta Delta{};
    TestTrue(TEXT("Unreal builds a base-identified scoped delta"),
             BuildScopedViewDelta(
                 Keyframe, DeltaTarget, Delta, &Rejection) &&
                 Delta.baseSnapshotId == Keyframe.snapshotId &&
                 Delta.snapshotId == DeltaTarget.snapshotId &&
                 !Delta.entityUpserts.empty());
    const std::vector<std::uint8_t> DeltaBytes =
        EncodeScopedViewDelta(Delta);
    ScopedViewDelta DecodedDelta{};
    TestTrue(TEXT("Scoped delta round-trips canonically and is compact"),
             !DeltaBytes.empty() &&
                 DeltaBytes.size() < DeltaTargetBytes.size() &&
                 DecodeScopedViewDelta(
                     DeltaBytes, DecodedDelta) == DecodeStatus::Ok &&
                 DecodedDelta == Delta);
    ScopedViewKeyframe AppliedDelta{};
    TestTrue(TEXT("Scoped delta reconstructs the exact target digest"),
             ApplyScopedViewDelta(
                 Keyframe, DecodedDelta, AppliedDelta, &Rejection) &&
                 Rejection.empty() && AppliedDelta == DeltaTarget);

    echoes::network::ScopedViewState DeltaClientView;
    TestTrue(TEXT("Client state accepts the delta only after its exact base"),
             DeltaClientView.Accept(Keyframe) ==
                     echoes::network::ScopedViewAcceptance::AcceptedFirst &&
                 DeltaClientView.AcceptDelta(DecodedDelta, &Rejection) ==
                     echoes::network::ScopedViewAcceptance::AcceptedDelta &&
                 Rejection.empty() && DeltaClientView.AcceptedCount() == 2 &&
                 DeltaClientView.Current().has_value() &&
                 *DeltaClientView.Current() == DeltaTarget);
    const ScopedViewKeyframe AcceptedDeltaState =
        *DeltaClientView.Current();
    TestTrue(TEXT("Client state ignores a stale duplicate without recovery"),
             DeltaClientView.AcceptDelta(DecodedDelta, &Rejection) ==
                     echoes::network::ScopedViewAcceptance::StaleOrDuplicate &&
                 Rejection.empty() &&
                 DeltaClientView.AcceptedCount() == 2 &&
                 *DeltaClientView.Current() == AcceptedDeltaState);
    ScopedViewDelta WrongDeltaDigest = DecodedDelta;
    WrongDeltaDigest.scopedDigest ^= 1;
    echoes::network::ScopedViewState DigestClientView;
    TestTrue(TEXT("Client state rejects a delta digest mismatch without mutation"),
             DigestClientView.Accept(Keyframe) ==
                     echoes::network::ScopedViewAcceptance::AcceptedFirst &&
                 DigestClientView.AcceptDelta(
                     WrongDeltaDigest, &Rejection) ==
                     echoes::network::ScopedViewAcceptance::DeltaRejected &&
                 Rejection == "NET_DELTA_DIGEST_MISMATCH" &&
                 DigestClientView.AcceptedCount() == 1 &&
                 DigestClientView.Current().has_value() &&
                 *DigestClientView.Current() == Keyframe);

    echoes::network::ScopedViewState ClientView;
    TestTrue(TEXT("Client view accepts the first authoritative keyframe"),
             ClientView.Accept(Keyframe) ==
                 echoes::network::ScopedViewAcceptance::AcceptedFirst);
    TestTrue(TEXT("Client view rejects duplicate lineage without mutation"),
             ClientView.Accept(Keyframe) ==
                     echoes::network::ScopedViewAcceptance::StaleOrDuplicate &&
                 ClientView.AcceptedCount() == 1);
    ScopedViewKeyframe NextKeyframe = Keyframe;
    ++NextKeyframe.snapshotId;
    ++NextKeyframe.simulationTick;
    TestTrue(TEXT("Client view advances contiguous lineage"),
             ClientView.Accept(NextKeyframe) ==
                 echoes::network::ScopedViewAcceptance::AcceptedNext);
    ScopedViewKeyframe RecoveryKeyframe = NextKeyframe;
    RecoveryKeyframe.snapshotId += 2;
    RecoveryKeyframe.simulationTick += 5;
    TestTrue(TEXT("A later full keyframe recovers a missing lineage member"),
             ClientView.Accept(RecoveryKeyframe) ==
                     echoes::network::ScopedViewAcceptance::AcceptedRecovery &&
                 ClientView.Current()->snapshotId ==
                     RecoveryKeyframe.snapshotId);
    ScopedViewKeyframe WrongSeat = RecoveryKeyframe;
    ++WrongSeat.snapshotId;
    WrongSeat.player = 0;
    TestTrue(TEXT("Client view fails closed if scoped player changes"),
             ClientView.Accept(WrongSeat) ==
                 echoes::network::ScopedViewAcceptance::PlayerChanged);

    return true;
}

#endif
