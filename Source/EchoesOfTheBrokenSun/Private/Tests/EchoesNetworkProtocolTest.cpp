#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "EchoesEntityView.h"
#include "EchoesNetworkSession.h"
#include "EchoesPlayerController.h"
#include "EchoesSimCore/NetworkProtocol.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

#include <algorithm>
#include <limits>

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
    // SHA-256("EchoesOfTheBrokenSun:0.93.0:protocol-3:snapshot-28:view-2").
    // Keep the compatibility identity aligned with the current native snapshot schema.
    constexpr Digest256 ExpectedBuildId{
    0xe4, 0x87, 0x8d, 0xd1, 0xc3, 0xf3, 0x5d, 0xb4,
    0x54, 0x3b, 0x2e, 0xd4, 0xfe, 0xf3, 0x82, 0x8d,
    0x77, 0xc1, 0xe5, 0x57, 0x8c, 0x3d, 0xfe, 0xf2,
    0x43, 0xf5, 0x2a, 0x66, 0x5a, 0x4d, 0x49, 0xf6};
    TestTrue(TEXT("Compatibility identity is bound to version 0.93.0 and schema 28"),
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

    FTestWorldWrapper PresentationWorldWrapper;
    if (!PresentationWorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        PresentationWorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the network-presentation pooling world."));
        return false;
    }
    UWorld* PresentationWorld = PresentationWorldWrapper.GetTestWorld();
    AEchoesPlayerController* PresentationController =
        PresentationWorld != nullptr
            ? PresentationWorld->SpawnActor<AEchoesPlayerController>()
            : nullptr;
    if (TestNotNull(TEXT("Network-presentation controller spawns"),
                    PresentationController))
    {
        UEchoesSimulationSubsystem* CommandBridge =
            PresentationWorld->GetSubsystem<UEchoesSimulationSubsystem>();
        FString FactionFeedback;
        const bool bCommandScenarioReady =
            TestNotNull(
                TEXT("Protocol-admission world owns the simulation subsystem"),
                CommandBridge) &&
            TestTrue(
                TEXT("Protocol-admission scenario starts"),
                CommandBridge != nullptr &&
                    CommandBridge->StartPrototypeScenario()) &&
            TestTrue(
                TEXT("Protocol-admission scenario deploys a local Cairnback"),
                CommandBridge->SelectLocalFaction(
                    Faction::KharuunAssemblies,
                    FactionFeedback));
        if (bCommandScenarioReady)
        {
            const Simulation* CommandSimulation =
                CommandBridge->GetSimulation();
            if (!TestNotNull(
                    TEXT("Protocol-admission scenario exposes authority"),
                    CommandSimulation))
            {
                CommandBridge->StopPrototypeScenario();
                PresentationController->Destroy();
                PresentationWorldWrapper.ForwardErrorMessages(this);
                return false;
            }
            const auto Cairnback = std::find_if(
                CommandSimulation->Entities().begin(),
                CommandSimulation->Entities().end(),
                [](const Entity& EntityState)
                {
                    return EntityState.owner ==
                               UEchoesSimulationSubsystem::LocalPlayerId &&
                           EntityState.faction ==
                               Faction::KharuunAssemblies &&
                           EntityState.type == EntityType::HeavyUnit;
                });
            const bool bCairnbackReady = TestTrue(
                TEXT("Kharuun protocol fixture contains a local Cairnback"),
                Cairnback != CommandSimulation->Entities().end());
            if (bCairnbackReady)
            {
                const EntityId CairnbackId = Cairnback->id;
                const Vec2 CairnbackPosition = Cairnback->position;
                const std::int32_t CairnbackHitPoints = Cairnback->hitPoints;
                const Tick CairnbackCooldown =
                    Cairnback->mineralCoverCooldownUntilTick;
                const PlayerState* LocalPlayer = CommandSimulation->FindPlayer(
                    UEchoesSimulationSubsystem::LocalPlayerId);
                if (!TestNotNull(
                        TEXT("Kharuun protocol fixture retains the local player"),
                        LocalPlayer))
                {
                    CommandBridge->StopPrototypeScenario();
                    PresentationController->Destroy();
                    PresentationWorldWrapper.ForwardErrorMessages(this);
                    return false;
                }
                const ResourcePool LocalResources = LocalPlayer->resources;

                const Tick InitialTick = CommandSimulation->CurrentTick();
                const std::uint64_t InitialChecksum =
                    CommandSimulation->StateChecksum();
                const std::size_t InitialCommandLogSize =
                    CommandSimulation->CommandLog().size();
                const std::size_t InitialEntityCount =
                    CommandSimulation->Entities().size();
                const std::optional<std::uint64_t> InitialNextSequence =
                    CommandSimulation->NextCommandSequence(
                        UEchoesSimulationSubsystem::LocalPlayerId);
                const bool bSequenceReady = TestTrue(
                    TEXT("Protocol fixture exposes a usable semantic sequence"),
                    InitialNextSequence.has_value() &&
                        *InitialNextSequence > 0 &&
                        *InitialNextSequence <
                            std::numeric_limits<std::uint64_t>::max() - 1);
                if (!bSequenceReady)
                {
                    CommandBridge->StopPrototypeScenario();
                    PresentationController->Destroy();
                    PresentationWorldWrapper.ForwardErrorMessages(this);
                    return false;
                }

                std::vector<Terrain> InitialTerrain;
                InitialTerrain.reserve(
                    static_cast<std::size_t>(
                        CommandSimulation->Config().mapWidthTiles) *
                    CommandSimulation->Config().mapHeightTiles);
                for (std::int32_t TileY = 0;
                     TileY < CommandSimulation->Config().mapHeightTiles;
                     ++TileY)
                {
                    for (std::int32_t TileX = 0;
                         TileX < CommandSimulation->Config().mapWidthTiles;
                         ++TileX)
                    {
                        InitialTerrain.push_back(
                            CommandSimulation->TerrainAt(TileX, TileY));
                    }
                }

                const Vec2 ExtremePositions[2]{
                    Vec2::FromRaw(
                        std::numeric_limits<std::int32_t>::min(),
                        std::numeric_limits<std::int32_t>::max()),
                    Vec2::FromRaw(
                        std::numeric_limits<std::int32_t>::max(),
                        std::numeric_limits<std::int32_t>::min())};
                const Vec2 OfflineOutOfMapPositions[2]{
                    Vec2::FromRaw(-1'000'000'000, 1'000'000'000),
                    Vec2::FromRaw(1'000'000'000, -1'000'000'000)};
                for (const Vec2 OfflinePosition : OfflineOutOfMapPositions)
                {
                    FString OfflineFeedback;
                    TestFalse(
                        TEXT("Offline mineral cover rejects a mixed out-of-map coordinate immediately"),
                        CommandBridge->IssueMineralCover(
                            CairnbackId,
                            CommandBridge->SimToWorld(OfflinePosition),
                            OfflineFeedback));
                    TestTrue(
                        TEXT("Offline mixed-coordinate rejection is terrain reason-coded"),
                        OfflineFeedback.StartsWith(
                            TEXT("[MINERAL_COVER_TERRAIN_INVALID]")));
                    TestTrue(
                        TEXT("Offline mixed-coordinate rejection leaves authority unchanged"),
                        CommandSimulation->CurrentTick() == InitialTick &&
                            CommandSimulation->StateChecksum() ==
                                InitialChecksum &&
                            CommandSimulation->CommandLog().size() ==
                                InitialCommandLogSize &&
                            CommandSimulation->NextCommandSequence(
                                UEchoesSimulationSubsystem::LocalPlayerId) ==
                                InitialNextSequence);
                }

                std::string SnapshotError;
                std::optional<Simulation> Twin = Simulation::LoadSnapshot(
                    CommandSimulation->SaveSnapshot(),
                    &SnapshotError);
                TestTrue(
                    TEXT("Protocol fixture snapshot clones exactly before admission"),
                    Twin.has_value() && SnapshotError.empty() &&
                        Twin->StateChecksum() == InitialChecksum);

                CommandBridge->SetNetworkHumanOpponent(true);
                PresentationController->bNetworkCompatibilityAccepted = true;
                PresentationController->bNetworkMatchStarted = true;
                PresentationController->NetworkSeat =
                    UEchoesSimulationSubsystem::LocalPlayerId;
                PresentationController->NetworkCommandContext = {};
                PresentationController->NetworkCommandContext.player =
                    UEchoesSimulationSubsystem::LocalPlayerId;
                PresentationController->NetworkCommandContext
                    .minimumInputDelayTicks = 3;
                PresentationController->NetworkCommandContext
                    .maximumLeadTicks = 40;
                PresentationController->NetworkCommandContext
                    .hasAcceptedSequence = *InitialNextSequence > 1;
                PresentationController->NetworkCommandContext
                    .lastAcceptedSequence = *InitialNextSequence - 1;
                PresentationController->LastAcceptedNetworkBatchId = 0;

                for (std::size_t Index = 0; Index < 2; ++Index)
                {
                    CommandBatchRequest ExtremeBatch{};
                    ExtremeBatch.clientBatchId = Index + 1;
                    CommandIntent ExtremeIntent{};
                    ExtremeIntent.type = CommandType::RaiseMineralCover;
                    ExtremeIntent.actor = CairnbackId;
                    ExtremeIntent.position = ExtremePositions[Index];
                    ExtremeBatch.intents.push_back(ExtremeIntent);
                    const std::vector<std::uint8_t> ExtremeBatchBytes =
                        EncodeCommandBatchRequest(ExtremeBatch);
                    CommandBatchRequest DecodedExtremeBatch{};
                    const bool bBatchRoundTrip = TestTrue(
                        TEXT("Mixed-extreme batch round-trips exactly through the wire codec"),
                        !ExtremeBatchBytes.empty() &&
                            DecodeCommandBatchRequest(
                                ExtremeBatchBytes,
                                DecodedExtremeBatch) == DecodeStatus::Ok &&
                            DecodedExtremeBatch == ExtremeBatch &&
                            EncodeCommandBatchRequest(DecodedExtremeBatch) ==
                                ExtremeBatchBytes &&
                            DecodedExtremeBatch.intents.front().position ==
                                ExtremePositions[Index]);
                    if (!bBatchRoundTrip)
                    {
                        continue;
                    }

                    PresentationController
                        ->ServerSubmitNetworkCommandBatch_Implementation(
                            echoes::network::ToByteArray(
                                ExtremeBatchBytes));
                    const std::uint64_t ExpectedSequence =
                        *InitialNextSequence + Index;
                    const bool bBookkeepingExact = TestTrue(
                        TEXT("Server batch consumes canonical batch and semantic bookkeeping"),
                        PresentationController
                                ->GetLastAcceptedNetworkBatchId() ==
                            Index + 1 &&
                            PresentationController->NetworkCommandContext
                                .hasAcceptedSequence &&
                            PresentationController->NetworkCommandContext
                                    .lastAcceptedSequence ==
                                ExpectedSequence &&
                            CommandSimulation->CommandLog().size() ==
                                InitialCommandLogSize + Index + 1 &&
                            CommandSimulation->NextCommandSequence(
                                UEchoesSimulationSubsystem::LocalPlayerId) ==
                                ExpectedSequence + 1 &&
                            CommandSimulation->CurrentTick() == InitialTick);
                    if (!bBookkeepingExact ||
                        CommandSimulation->CommandLog().empty())
                    {
                        continue;
                    }
                    const Command& Admitted =
                        CommandSimulation->CommandLog().back();
                    TestTrue(
                        TEXT("Authority preserves the exact mixed-extreme intent"),
                        Admitted.type == CommandType::RaiseMineralCover &&
                            Admitted.actor == CairnbackId &&
                            Admitted.position == ExtremePositions[Index] &&
                            Admitted.sequence == ExpectedSequence &&
                            Admitted.executeTick == InitialTick + 3 &&
                            CommandSimulation->ValidateMineralCover(
                                UEchoesSimulationSubsystem::LocalPlayerId,
                                CairnbackId,
                                ExtremePositions[Index]) ==
                                MineralCoverResult::InvalidPosition);
                }
                const std::optional<std::uint64_t>
                    PostAdmissionNextSequence =
                        CommandSimulation->NextCommandSequence(
                            UEchoesSimulationSubsystem::LocalPlayerId);
                TestTrue(
                    TEXT("Direct server batches expose authority-side structural admission bookkeeping"),
                    PresentationController->GetLastAcceptedNetworkBatchId() ==
                            2 &&
                        PresentationController->NetworkCommandContext
                            .hasAcceptedSequence &&
                        PresentationController->NetworkCommandContext
                                .lastAcceptedSequence ==
                            *InitialNextSequence + 1 &&
                        CommandSimulation->CommandLog().size() ==
                            InitialCommandLogSize + 2 &&
                        PostAdmissionNextSequence.has_value() &&
                        *PostAdmissionNextSequence ==
                            *InitialNextSequence + 2 &&
                        CommandSimulation->CurrentTick() == InitialTick);

                if (Twin.has_value())
                {
                    for (std::size_t Index = InitialCommandLogSize;
                         Index < CommandSimulation->CommandLog().size();
                         ++Index)
                    {
                        std::string TwinRejection;
                        TestTrue(
                            TEXT("Snapshot twin accepts the same canonical command"),
                            Twin->QueueCommand(
                                CommandSimulation->CommandLog()[Index],
                                &TwinRejection) &&
                                TwinRejection.empty());
                    }
                    TestTrue(
                        TEXT("Server and snapshot twin match after admission bookkeeping"),
                        Twin->StateChecksum() ==
                            CommandSimulation->StateChecksum());
                    for (std::uint32_t StepIndex = 0;
                         StepIndex < 4;
                         ++StepIndex)
                    {
                        CommandBridge->Tick(1.0f / 20.0f);
                        Twin->Step();
                        TestTrue(
                            TEXT("Server and snapshot twin remain exact through due execution"),
                            Twin->CurrentTick() ==
                                    CommandSimulation->CurrentTick() &&
                                Twin->StateChecksum() ==
                                    CommandSimulation->StateChecksum());
                    }
                }

                const Entity* FinalCairnback =
                    CommandSimulation->FindEntity(CairnbackId);
                const PlayerState* FinalPlayer =
                    CommandSimulation->FindPlayer(
                        UEchoesSimulationSubsystem::LocalPlayerId);
                const bool bTerrainUnchanged = [&]()
                {
                    std::size_t TerrainIndex = 0;
                    for (std::int32_t TileY = 0;
                         TileY <
                             CommandSimulation->Config().mapHeightTiles;
                         ++TileY)
                    {
                        for (std::int32_t TileX = 0;
                             TileX <
                                 CommandSimulation->Config().mapWidthTiles;
                             ++TileX, ++TerrainIndex)
                        {
                            if (CommandSimulation->TerrainAt(TileX, TileY) !=
                                InitialTerrain[TerrainIndex])
                            {
                                return false;
                            }
                        }
                    }
                    return true;
                }();
                TestTrue(
                    TEXT("Due mixed-extreme commands are deterministic gameplay no-ops"),
                    CommandSimulation->CurrentTick() == InitialTick + 4 &&
                        CommandSimulation->CommandLog().size() ==
                            InitialCommandLogSize + 2 &&
                        CommandSimulation->Entities().size() ==
                            InitialEntityCount &&
                        FinalCairnback != nullptr &&
                        FinalCairnback->position == CairnbackPosition &&
                        FinalCairnback->hitPoints == CairnbackHitPoints &&
                        FinalCairnback->mineralCoverCooldownUntilTick ==
                            CairnbackCooldown &&
                        FinalPlayer != nullptr &&
                        FinalPlayer->resources == LocalResources &&
                        std::none_of(
                            CommandSimulation->Entities().begin(),
                            CommandSimulation->Entities().end(),
                            [](const Entity& EntityState)
                            {
                                return EntityState.temporaryMineralCover;
                            }) &&
                        bTerrainUnchanged);
                for (const Vec2 ExtremePosition : ExtremePositions)
                {
                    TestTrue(
                        TEXT("Authoritative validator retains InvalidPosition after due execution"),
                        CommandSimulation->ValidateMineralCover(
                            UEchoesSimulationSubsystem::LocalPlayerId,
                            CairnbackId,
                            ExtremePosition) ==
                            MineralCoverResult::InvalidPosition);
                }
                CommandBridge->SetNetworkHumanOpponent(false);
            }
        }
        if (CommandBridge != nullptr && CommandBridge->IsScenarioReady())
        {
            CommandBridge->StopPrototypeScenario();
        }

        ScopedEntityState PossibleChoir{};
        PossibleChoir.id = 900001;
        PossibleChoir.owner = 1;
        PossibleChoir.faction = Faction::HollowChoir;
        PossibleChoir.type = EntityType::Soldier;
        PossibleChoir.position = Vec2::FromTiles(5, 5);
        PossibleChoir.hitPoints = 70;
        PossibleChoir.maxHitPoints = 90;
        PossibleChoir.choirIdentityState = ChoirIdentityState::Possible;
        PossibleChoir.choirIdentityResolveAtTick = 111;
        PossibleChoir.choirIdentityNextAvailableTick = 222;
        PossibleChoir.choirCoherenceNextChargeTick = 333;
        const Entity PossiblePresentation =
            PresentationController->BuildNetworkPresentationEntity(
                PossibleChoir);
        TestTrue(TEXT("Network mapping preserves all public Choir identity state"),
                 PossiblePresentation.choirIdentityState ==
                         PossibleChoir.choirIdentityState &&
                     PossiblePresentation.choirIdentityResolveAtTick ==
                         PossibleChoir.choirIdentityResolveAtTick &&
                     PossiblePresentation.choirIdentityNextAvailableTick ==
                         PossibleChoir.choirIdentityNextAvailableTick &&
                     PossiblePresentation.choirCoherenceNextChargeTick ==
                         PossibleChoir.choirCoherenceNextChargeTick);

        AEchoesEntityView* FirstNetworkView =
            PresentationController->AcquireNetworkEntityView();
        if (TestNotNull(TEXT("Network pool acquires an entity view"),
                        FirstNetworkView))
        {
            FirstNetworkView->ActivateForEntity(PossiblePresentation, true);
            TestTrue(TEXT("Network activation renders the possible Choir identity"),
                     FirstNetworkView->GetEntityId() == PossibleChoir.id &&
                         FirstNetworkView->GetChoirIdentityState() ==
                             ChoirIdentityState::Possible &&
                         FirstNetworkView->IsChoirIdentityStateVisible());
            const uint64 WarmMIDCount =
                FirstNetworkView->GetOwnedMIDCreationCount();
            PresentationController->ReleaseNetworkEntityView(FirstNetworkView);
            TestTrue(TEXT("Network retirement fully clears identity before pooling"),
                     FirstNetworkView->IsPreparedForPool() &&
                         FirstNetworkView->GetEntityId() == 0 &&
                         FirstNetworkView->GetChoirIdentityState() ==
                             ChoirIdentityState::NotChoir &&
                         !FirstNetworkView->IsChoirIdentityStateVisible());

            ScopedEntityState ManifestChoir = PossibleChoir;
            ManifestChoir.id = 900002;
            ManifestChoir.choirIdentityState = ChoirIdentityState::Manifest;
            ManifestChoir.choirIdentityResolveAtTick = 444;
            ManifestChoir.choirIdentityNextAvailableTick = 555;
            ManifestChoir.choirCoherenceNextChargeTick = 666;
            const Entity ManifestPresentation =
                PresentationController->BuildNetworkPresentationEntity(
                    ManifestChoir);
            AEchoesEntityView* ReusedNetworkView =
                PresentationController->AcquireNetworkEntityView();
            TestTrue(TEXT("Network acquisition deterministically reuses the retired view"),
                     ReusedNetworkView == FirstNetworkView);
            if (ReusedNetworkView != nullptr)
            {
                ReusedNetworkView->ActivateForEntity(
                    ManifestPresentation, true);
                TestTrue(TEXT("Network reuse rebinds the new scoped Choir identity"),
                         ReusedNetworkView->GetEntityId() == ManifestChoir.id &&
                             ReusedNetworkView->GetChoirIdentityState() ==
                                 ChoirIdentityState::Manifest &&
                             ReusedNetworkView->IsChoirIdentityStateVisible());
                TestEqual(TEXT("Warmed network reuse creates no additional MIDs"),
                          ReusedNetworkView->GetOwnedMIDCreationCount(),
                          WarmMIDCount);
                PresentationController->ReleaseNetworkEntityView(
                    ReusedNetworkView);
            }
        }
        PresentationController->DestroyNetworkPresentation();
        PresentationController->Destroy();
    }
    PresentationWorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors() && !PresentationWorldWrapper.HasFailed();
}

#endif
