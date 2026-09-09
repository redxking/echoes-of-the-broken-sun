#include "EchoesCommandDeckModel.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesCommandDeckModelTest,
    "Echoes.Runtime.Presentation.CommandDeckModel",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEchoesCommandDeckModelTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    FEchoesScopedTestSaveEnvironment TestSaveEnvironment(*this);
    if (!TestSaveEnvironment.IsReady())
    {
        return false;
    }

    FEchoesCommandDeckProfile Profile;
    TestEqual(
        TEXT("Empty/non-command selection uses safe generic actions"),
        FEchoesCommandDeckModel::BuildPrimaryActions(Profile),
        FString(TEXT("[RMB] CONTEXT / MOVE    [X] STOP")));

    Profile.CombatCount = 1;
    const FString Combat = FEchoesCommandDeckModel::BuildPrimaryActions(Profile);
    TestTrue(TEXT("Combat exposes attack-move"), Combat.Contains(TEXT("[F] ATTACK-MOVE")));
    TestTrue(TEXT("Combat exposes guard"), Combat.Contains(TEXT("[J] GUARD")));

    Profile = {};
    Profile.WorkerCount = 1;
    const FString Worker = FEchoesCommandDeckModel::BuildPrimaryActions(Profile);
    TestTrue(TEXT("Worker exposes Barracks construction"), Worker.Contains(TEXT("[B] BARRACKS")));
    TestTrue(TEXT("Worker exposes Utility construction"), Worker.Contains(TEXT("[M] UTILITY")));

    TestTrue(TEXT("Worker exposes its existing repair gesture"),
        Worker.Contains(TEXT("[R] REPAIR")));
    const auto WorkerEntries = FEchoesCommandDeckModel::BuildActionEntries(Profile);
    TestEqual(TEXT("Worker deck retains room within the six-slot command contract"),
        WorkerEntries.Num(), 5);
    const FEchoesCommandDeckActionEntry* RepairEntry = WorkerEntries.FindByPredicate(
        [](const FEchoesCommandDeckActionEntry& Entry)
        {
            return Entry.Action == EEchoesCommandDeckAction::RepairAtCursor;
        });
    TestTrue(TEXT("Repair remains a pointer-targeted worker action"),
        RepairEntry != nullptr && RepairEntry->bRequiresCursorTarget &&
            FString(RepairEntry->Label) == TEXT("REPAIR"));

    Profile = {};
    Profile.StructureCount = 1;
    Profile.bCanCancelSelectedConstruction = true;
    const auto CancellationEntries = FEchoesCommandDeckModel::BuildActionEntries(Profile);
    TestTrue(TEXT("Only a selected unfinished structure exposes cancellation"),
        CancellationEntries.ContainsByPredicate(
            [](const FEchoesCommandDeckActionEntry& Entry)
            {
                return Entry.Action == EEchoesCommandDeckAction::CancelConstruction &&
                    !Entry.bRequiresCursorTarget;
            }));
    TestTrue(TEXT("Cancellation preserves its existing shifted input prompt"),
        FEchoesCommandDeckModel::BuildPrimaryActions(Profile).Contains(
            TEXT("[SHIFT+X] CANCEL CONSTRUCTION")));

    Profile = {};
    Profile.bHasCommandCore = true;
    TestEqual(
        TEXT("Command Core advertises only its compatible worker key"),
        FEchoesCommandDeckModel::BuildPrimaryActions(Profile),
        FString(TEXT("[Q] PRODUCE WORKER")));

    Profile = {};
    Profile.bHasBarracks = true;
    const FString Barracks = FEchoesCommandDeckModel::BuildPrimaryActions(Profile);
    TestTrue(TEXT("Barracks exposes line-unit key"), Barracks.Contains(TEXT("[E] LINE UNIT")));
    TestTrue(TEXT("Barracks exposes heavy key"), Barracks.Contains(TEXT("[;] HEAVY")));
    TestTrue(TEXT("Barracks exposes scout key"), Barracks.Contains(TEXT("['] SCOUT")));
    TestFalse(TEXT("Barracks does not advertise worker production"), Barracks.Contains(TEXT("WORKER")));

    Profile.bHasCommandCore = true;
    const FString Combined = FEchoesCommandDeckModel::BuildPrimaryActions(Profile);
    TestTrue(TEXT("Mixed production selection exposes worker"), Combined.Contains(TEXT("[Q] WORKER")));
    TestTrue(TEXT("Mixed production selection exposes technology"), Combined.Contains(TEXT("[F2] TECHNOLOGY")));

    Profile.bUseM01RoleNames = true;
    const auto M01Entries = FEchoesCommandDeckModel::BuildActionEntries(Profile);
    TestEqual(TEXT("M01 retains five compatible production/technology actions"), M01Entries.Num(), 5);
    TestEqual(TEXT("M01 names produced worker by role"), FString(M01Entries[0].Label), FString(TEXT("SURVEYOR")));
    TestEqual(TEXT("M01 retains worker production command"), M01Entries[0].Action, EEchoesCommandDeckAction::ProduceWorker);
    TestEqual(TEXT("M01 retains worker hotkey"), FString(M01Entries[0].Hotkey), FString(TEXT("Q")));
    TestTrue(TEXT("M01 summary names the Bulwark Team"), FEchoesCommandDeckModel::BuildPrimaryActions(Profile).Contains(TEXT("BULWARK TEAM")));
    TestEqual(TEXT("Production progress uses same Surveyor identity"), FString(FEchoesCommandDeckModel::GetM01RoleName(echoes::sim::EntityType::Worker)), FString(TEXT("Surveyor")));
    Profile.WorkerCount = 1;
    TestTrue(TEXT("M01 worker advertises Array Foundry construction"), FEchoesCommandDeckModel::BuildPrimaryActions(Profile).Contains(TEXT("[B] ARRAY FOUNDRY")));
    Profile.bUseM01RoleNames = false;

    Profile.WorkerCount = 1;
    TestTrue(
        TEXT("Mobile worker context takes precedence over selected structures"),
        FEchoesCommandDeckModel::BuildPrimaryActions(Profile).Contains(TEXT("[B] BARRACKS")));

    Profile.CombatCount = 1;
    TestTrue(
        TEXT("Combat context takes precedence in a mixed mobile selection"),
        FEchoesCommandDeckModel::BuildPrimaryActions(Profile).Contains(TEXT("[F] ATTACK-MOVE")));
    // Exercise the actual dispatch resolver against real queued commands and
    // phase transitions. No manually seeded deployment/cooldown state.
    using namespace echoes::sim;
    Simulation Sim(SimulationConfig{64, 64, 20, 0x42554C5741524BULL});
    if (!TestTrue(TEXT("Caster owner exists"), Sim.AddPlayer(0, Faction::MeridianCompact, {1000, 500})) ||
        !TestTrue(TEXT("Other owner exists"), Sim.AddPlayer(1, Faction::MeridianCompact, {1000, 500}))) return false;
    const auto Near = Sim.SpawnEntity(0, Faction::MeridianCompact, EntityType::HeavyUnit, Vec2::FromTiles(10, 10));
    const auto Far = Sim.SpawnEntity(0, Faction::MeridianCompact, EntityType::HeavyUnit, Vec2::FromTiles(20, 10));
    const auto Peer = Sim.SpawnEntity(0, Faction::MeridianCompact, EntityType::HeavyUnit, Vec2::FromTiles(10, 20));
    const auto Scout = Sim.SpawnEntity(0, Faction::MeridianCompact, EntityType::ScoutUnit, Vec2::FromTiles(14, 14));
    const auto Enemy = Sim.SpawnEntity(1, Faction::MeridianCompact, EntityType::HeavyUnit, Vec2::FromTiles(15, 15));
    if (!TestTrue(TEXT("Caster prerequisites spawn"), Near && Far && Peer && Scout && Enemy)) return false;
    const TArray<uint32> Mixed{Enemy, Scout, Peer, Far, Near, Near, 999999};
    const auto Resolve = [&](bool All) { return FEchoesCommandDeckModel::ResolveLocalBulwarkCasters(
        Sim, 0, Vec2::FromTiles(15, 15), Mixed, All); };
    TestTrue(TEXT("Equal-distance normal gesture chooses lowest eligible ID only"), Resolve(false) == TArray<uint32>{Near});
    TestTrue(TEXT("Ctrl gesture includes each eligible owner once"), Resolve(true) == TArray<uint32>({Near, Far, Peer}));
    TestTrue(TEXT("Empty selection cannot command the whole army"),
        FEchoesCommandDeckModel::ResolveLocalBulwarkCasters(Sim, 0, {}, {}, false).IsEmpty());
    TestTrue(TEXT("Geometric closest caster wins over a smaller entity ID"),
        FEchoesCommandDeckModel::ResolveLocalBulwarkCasters(Sim, 0, Vec2::FromTiles(19, 10), Mixed, false) == TArray<uint32>{Far});
    TestTrue(TEXT("Packed caster under the target yields to the next valid facing"),
        FEchoesCommandDeckModel::ResolveLocalBulwarkCasters(Sim, 0, Vec2::FromTiles(10, 10), Mixed, false) == TArray<uint32>{Far});
    TestTrue(TEXT("Ctrl also excludes a caster without a deployment direction"),
        FEchoesCommandDeckModel::ResolveLocalBulwarkCasters(Sim, 0, Vec2::FromTiles(10, 10), Mixed, true) == TArray<uint32>({Far, Peer}));
    Command Deploy{}; Deploy.player = 0; Deploy.actor = Near; Deploy.sequence = 1;
    Deploy.type = CommandType::ToggleDeploy; Deploy.position = Vec2::FromTiles(30, 10);
    if (!TestTrue(TEXT("First real deployment queues"), Sim.QueueCommand(Deploy))) return false;
    TestTrue(TEXT("Second gesture before a fixed step skips pending caster"), Resolve(false) == TArray<uint32>{Far});
    Deploy.actor = Far; Deploy.sequence = 2;
    if (!TestTrue(TEXT("Second deployment queues"), Sim.QueueCommand(Deploy))) return false;
    TestTrue(TEXT("Third rapid gesture selects the remaining eligible caster"), Resolve(false) == TArray<uint32>{Peer});
    Sim.Step(10);
    TestTrue(TEXT("Executing transitions remain ineligible"), Resolve(true) == TArray<uint32>{Peer});
    Sim.Step(10);
    TestTrue(TEXT("Deployed Bulwark becomes eligible for packing"), Resolve(false) == TArray<uint32>{Near});
    TestTrue(TEXT("Packing does not require a new facing direction"),
        FEchoesCommandDeckModel::ResolveLocalBulwarkCasters(Sim, 0, Vec2::FromTiles(10, 10), Mixed, false) == TArray<uint32>{Near});
    Deploy.actor = Near; Deploy.sequence = 3; Deploy.executeTick = Sim.CurrentTick();
    if (!TestTrue(TEXT("Real packing queues"), Sim.QueueCommand(Deploy))) return false;
    Sim.Step(5);
    TestTrue(TEXT("Packing caster does not steal the next gesture"), Resolve(false) == TArray<uint32>{Far});
    Sim.Step(10);
    TestTrue(TEXT("Packed Bulwark can deploy again"), Resolve(false) == TArray<uint32>{Near});

    return true;
}

#endif
