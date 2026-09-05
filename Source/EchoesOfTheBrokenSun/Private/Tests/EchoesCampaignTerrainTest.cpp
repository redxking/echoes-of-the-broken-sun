// Author: Angelis Pseftis
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "EchoesCampaignTerrainBinding.h"
#include "Misc/Crc.h"
#include "EchoesCampaignMapCheckpoint.h"
#include "EchoesTestSaveEnvironment.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/World.h"
#include "Misc/FileHelper.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEchoesCampaignTerrainTest, "Echoes.Runtime.Map.CampaignTerrain",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)
bool FEchoesCampaignTerrainTest::RunTest(const FString& Parameters)
{
    using namespace echoes;
    const sim::FutureWellChoice Choices[] = {sim::FutureWellChoice::Harvest,
        sim::FutureWellChoice::Preserve, sim::FutureWellChoice::Reshape};
    TSet<FString> MapIds;
    TSet<uint32> Layouts;
    for (uint8 Mission = 1; Mission <= 15; ++Mission)
    {
        TArray<uint8> FullIdentity;
        for (auto Choice : Choices)
        {
            sim::SimulationConfig Config;
            Config.rules = sim::DefaultSimulationRules();
            sim::Simulation Simulation(Config);
            const auto Result = world::ApplyCampaignTerrain(Simulation, Mission, Choice);
            if (!TestTrue(*FString::Printf(TEXT("M%02d variant %d binds"), Mission, static_cast<int>(Choice)), Result.ok)) continue;
            MapIds.Add(UTF8_TO_TCHAR(Result.map_id));
            int32 Blocked = 0;
            for (int32 Y = 0; Y < 64; ++Y)
                for (int32 X = 0; X < 64; ++X)
                {
                    const bool bBlocked = Simulation.TerrainAt(X,Y) == sim::Terrain::Blocked;
                    Blocked += bBlocked;
                    FullIdentity.Add(bBlocked);
                    TestEqual(TEXT("bound cell matches compiled truth"), !bBlocked,
                        world::IsCampaignTerrainPassable(Mission, Choice, X,Y));
                }
            TestEqual(TEXT("bound census matches"), Blocked, Result.blocked_cells);
            if (Mission <= 9)
            {
                int32 WellX = -1, WellY = -1;
                if (TestTrue(TEXT("Generic campaign Well has an explicit source anchor"),
                    world::FindCampaignMapAnchor(Mission, "future-well", WellX, WellY)))
                    for (int32 Y = WellY-1; Y <= WellY; ++Y)
                        for (int32 X = WellX-1; X <= WellX; ++X)
                            TestTrue(TEXT("Well foundation remains clear under every founding choice"),
                                world::IsCampaignTerrainPassable(Mission, Choice, X, Y));
            }
        }
        Layouts.Add(FCrc::MemCrc32(FullIdentity.GetData(), FullIdentity.Num()));
    }
    TestEqual(TEXT("fifteen independent map identities"), MapIds.Num(), 15);
    TestEqual(TEXT("fifteen different complete terrain layouts"), Layouts.Num(), 15);
    for (uint8 Invalid : {static_cast<uint8>(0), static_cast<uint8>(16), static_cast<uint8>(255)})
        TestFalse(TEXT("invalid mission refuses"), world::CheckCampaignTerrain(Invalid, Choices[0]).ok);
    TestFalse(TEXT("unresolved doctrine refuses"), world::CheckCampaignTerrain(2, sim::FutureWellChoice::Dormant).ok);
    int32 MissingX = 91, MissingY = 92;
    TestFalse(TEXT("Unknown anchor refuses"), world::FindCampaignMapAnchor(1,"not-authored",MissingX,MissingY));
    TestTrue(TEXT("Refused anchor preserves outputs"), MissingX == 91 && MissingY == 92);
    sim::SimulationConfig SmallConfig;
    SmallConfig.mapWidthTiles = 32; SmallConfig.mapHeightTiles = 32;
    sim::Simulation Small(SmallConfig);
    TestFalse(TEXT("wrong grid refuses before mutation"), world::ApplyCampaignTerrain(Small, 1, Choices[0]).ok);
    for (int32 Y = 0; Y < 32; ++Y)
        for (int32 X = 0; X < 32; ++X)
            TestTrue(TEXT("refused grid stays untouched"), Small.TerrainAt(X,Y) == sim::Terrain::Open);
    return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEchoesCampaignTerrainCheckpointTest,
    "Echoes.Runtime.Persistence.CampaignMapAdmission",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)
bool FEchoesCampaignTerrainCheckpointTest::RunTest(const FString& Parameters)
{
    FEchoesScopedTestSaveEnvironment Saves(*this);
    if (!Saves.IsReady()) return false;
    FTestWorldWrapper World;
    if (!World.CreateTestWorld(EWorldType::Game)) return false;
    auto* Bridge = World.GetTestWorld()->GetSubsystem<UEchoesSimulationSubsystem>();
    FString Feedback;
    if (!Bridge || !TestTrue(TEXT("M01 starts for checkpoint admission"),
        Bridge->SelectOperationMode(EEchoesOperationMode::CampaignPrologue, Feedback) && Bridge->StartPrototypeScenario()))
        return false;
    if (!TestTrue(TEXT("M01 saves with map identity"), Bridge->QuickSaveScenario(Feedback))) return false;
    const FString Path = Bridge->GetActiveQuickSavePath();
    TArray<uint8> Valid, Inner, Stale;
    FEchoesCampaignMapCheckpointIdentity Identity;
    EEchoesCampaignMapCheckpointFailure Failure{};
    if (!FFileHelper::LoadFileToArray(Valid, *Path) ||
        !TestTrue(TEXT("saved M01 actually carries map envelope"), FEchoesCampaignMapCheckpoint::Inspect(Valid, Identity, Inner, Failure)))
        return false;
    const auto Checksum = Bridge->GetSimulation()->StateChecksum();
    const auto Tick = Bridge->GetSimulation()->CurrentTick();
    Identity.MapId += TEXT("-stale");
    if (!FEchoesCampaignMapCheckpoint::Wrap(Identity, Inner, Stale, Failure) ||
        !FFileHelper::SaveArrayToFile(Stale, *Path)) return false;
    TestFalse(TEXT("CRC-valid wrong map refuses admission"), Bridge->QuickLoadScenario(Feedback));
    TestTrue(TEXT("wrong map reports stable refusal"), Feedback.Contains(TEXT("CAMPAIGN_MAP_STALE")));
    TestEqual(TEXT("wrong map leaves live checksum unchanged"), Bridge->GetSimulation()->StateChecksum(), Checksum);
    TestEqual(TEXT("wrong map leaves live tick unchanged"), Bridge->GetSimulation()->CurrentTick(), Tick);
    if (!FFileHelper::SaveArrayToFile(Inner, *Path)) return false;
    TestFalse(TEXT("pre-cutover unbound campaign save refuses"), Bridge->QuickLoadScenario(Feedback));
    TestTrue(TEXT("unbound map reports stable refusal"), Feedback.Contains(TEXT("CAMPAIGN_MAP_UNBOUND")));
    TestEqual(TEXT("unbound map leaves live state unchanged"), Bridge->GetSimulation()->StateChecksum(), Checksum);
    if (!FFileHelper::SaveArrayToFile(Valid, *Path)) return false;
    TestTrue(TEXT("current map identity round-trips"), Bridge->QuickLoadScenario(Feedback));
    TestEqual(TEXT("round-trip keeps authoritative state"), Bridge->GetSimulation()->StateChecksum(), Checksum);
    Bridge->StopPrototypeScenario();
    World.ForwardErrorMessages(this);
    return !HasAnyErrors();
}
#endif
