#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

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

    const CompatibilityManifest ClientManifest =
        echoes::network::BuildCompatibilityManifest();
    SimulationConfig RuntimeConfig{16, 16, 20, 77};
    RuntimeConfig.rules.contentSha256 = ClientManifest.rulesPackSha256;
    Simulation Simulation(RuntimeConfig);
    TestTrue(TEXT("Authority creates the remote seat"),
             Simulation.AddPlayer(
                 1, Faction::KharuunAssemblies, {1000, 500}));
    TestTrue(TEXT("Authority creates the opposing seat"),
             Simulation.AddPlayer(
                 0, Faction::MeridianCompact, {1000, 500}));
    const EntityId Worker = Simulation.SpawnEntity(
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

    CommandAdmissionContext Context{};
    Context.player = 1;
    std::string Rejection;
    TestTrue(TEXT("Connection-bound seat supplies player identity at admission"),
             AdmitCommandRequest(
                 DecodedRequest, Context, Simulation, &Rejection) ==
                 CommandAdmissionStatus::Accepted);
    TestTrue(TEXT("Client packet cannot override server-derived player"),
             !Simulation.CommandLog().empty() &&
                 Simulation.CommandLog().back().player == 1);
    TestTrue(TEXT("Duplicate sequence fails before simulation mutation"),
             AdmitCommandRequest(
                 DecodedRequest, Context, Simulation, &Rejection) ==
                 CommandAdmissionStatus::SequenceUnexpected &&
                 Simulation.CommandLog().size() == 1);

    const CompatibilityManifest ConfiguredManifest =
        echoes::network::BuildCompatibilityManifest(&Simulation);
    TestTrue(TEXT("Runtime and client construct one exact compatibility identity"),
             ConfiguredManifest == ClientManifest);

    const EntityId HiddenLocalWorker = Simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Worker,
        Vec2::FromTiles(14, 14));
    const EntityId VisibleHostile = Simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Soldier,
        Vec2::FromTiles(5, 4));
    TestTrue(TEXT("Authority creates scoped-state fixtures"),
             HiddenLocalWorker != 0 && VisibleHostile != 0);
    const std::optional<PlayerView> RemoteView =
        Simulation.CreatePlayerView(1);
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

    return true;
}

#endif
