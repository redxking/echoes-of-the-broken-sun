#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesSimCore/NetworkProtocol.h"

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

    Simulation Simulation({16, 16, 20, 77});
    TestTrue(TEXT("Authority creates the remote seat"),
             Simulation.AddPlayer(
                 1, Faction::KharuunAssemblies, {1000, 500}));
    const EntityId Worker = Simulation.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::Worker,
        Vec2::FromTiles(4, 4));
    TestTrue(TEXT("Authority creates the remote worker"), Worker != 0);

    CommandRequest Request{};
    Request.sequence = 1;
    Request.executeTick = 2;
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
    TestTrue(TEXT("Authenticated seat supplies player identity at admission"),
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

    return true;
}

#endif
