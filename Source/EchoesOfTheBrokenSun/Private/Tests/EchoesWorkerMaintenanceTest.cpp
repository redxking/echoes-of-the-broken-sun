#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "EchoesTestSaveEnvironment.h"
#include "EchoesPlayerController.h"
#include "EchoesRTSCameraPawn.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

namespace
{
bool SelectMaintenanceEntity(AEchoesPlayerController& Controller, uint32 Id, int32 Count)
{
    for (int32 Attempt = 0; Attempt < Count + 2; ++Attempt)
    {
        Controller.CycleOwnedEntityPrevious();
        const auto& Selected = Controller.GetSelectedEntityIds();
        if (Selected.Num() == 1 && Selected[0] == Id) return true;
    }
    return false;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEchoesWorkerMaintenanceTest,
    "Echoes.Runtime.Gameplay.WorkerMaintenanceActions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEchoesWorkerMaintenanceTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    using namespace echoes::sim;
    FEchoesScopedTestSaveEnvironment Saves(*this);
    if (!Saves.IsReady()) return false;
    FTestWorldWrapper Fixture;
    if (!Fixture.CreateTestWorld(EWorldType::Game))
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    UWorld* World = Fixture.GetTestWorld();
    auto* Bridge = World ? World->GetSubsystem<UEchoesSimulationSubsystem>() : nullptr;
    if (!TestNotNull(TEXT("Maintenance bridge exists"), Bridge) ||
        !TestTrue(TEXT("Maintenance scenario starts"), Bridge && Bridge->StartPrototypeScenario())) return false;
    auto* Controller = World->SpawnActor<AEchoesPlayerController>();
    auto* Camera = World->SpawnActor<AEchoesRTSCameraPawn>();
    if (!TestNotNull(TEXT("Maintenance controller exists"), Controller) ||
        !TestNotNull(TEXT("Maintenance camera exists"), Camera)) return false;
    Controller->InitInputSystem();
    Controller->Possess(Camera);
    if (Controller->IsTitleScreenVisible()) Controller->ConfirmPrimaryAction();
    if (Controller->IsMissionBriefingVisible()) Controller->ConfirmPrimaryAction();
    auto* Sim = const_cast<Simulation*>(Bridge->GetSimulation());
    if (!TestNotNull(TEXT("Maintenance authority exists"), Sim)) return false;
    uint32 WorkerId = 0;
    uint32 CoreId = 0;
    for (const auto& Entity : Sim->Entities())
    {
        if (Entity.owner != UEchoesSimulationSubsystem::LocalPlayerId) continue;
        if (!WorkerId && Entity.type == EntityType::Worker) WorkerId = Entity.id;
        if (!CoreId && Entity.type == EntityType::CommandCore) CoreId = Entity.id;
    }
    if (!TestTrue(TEXT("Fixture has a worker and its network core"), WorkerId && CoreId)) return false;
    Entity* Core = Sim->MutableEntityForTesting(CoreId);
    Entity* Worker = Sim->MutableEntityForTesting(WorkerId);
    // Explicit test setup: damage and nearby worker. All outcomes below use
    // controller dispatch and fixed simulation ticks, never health repair calls.
    Core->hitPoints = Core->maxHitPoints - 20;
    Worker->position = {Core->position.x + Fixed::FromInt(2), Core->position.y};
    Worker->order = {};
    Worker->orderQueue.clear();
    const int32 HpBefore = Core->hitPoints;
    const int32 MoneyBefore = Sim->FindPlayer(0)->resources.material;
    if (!TestTrue(TEXT("Actual selection cycle chooses the worker"),
        SelectMaintenanceEntity(*Controller, WorkerId, Sim->Entities().size()))) return false;
    TestTrue(TEXT("Damaged allied target invokes maintenance context"),
        Controller->TryIssueWorkerMaintenanceContext(CoreId));
    for (int32 Tick = 0; Tick < 22; ++Tick) Bridge->Tick(0.05f);
    TestTrue(TEXT("Controller repair increases target health"), Sim->FindEntity(CoreId)->hitPoints > HpBefore);
    TestTrue(TEXT("Controller repair consumes Matter"), Sim->FindPlayer(0)->resources.material < MoneyBefore);
    TestFalse(TEXT("Self repair is refused through the same controller action"),
        Controller->IssueSelectedWorkerMaintenance(WorkerId, false));

    // An authored site is a paid-footprint fixture. Assist must not attempt
    // placement or charge the price of another building.
    const Vec2 SitePosition = {Sim->FindEntity(CoreId)->position.x + Fixed::FromInt(4), Sim->FindEntity(CoreId)->position.y};
    const uint32 SiteId = Sim->SpawnEntity(0, Faction::MeridianCompact,
        EntityType::Dropoff, SitePosition);
    if (!TestTrue(TEXT("Construction fixture site spawns"), SiteId != 0)) return false;
    Entity* Site = Sim->MutableEntityForTesting(SiteId);
    Site->completed = false;
    Site->constructionProgress = 10;
    Site->constructionRequired = 100;
    Site->constructionInvestedCost = {90, 10};
    Site->hitPoints = 45;
    Worker = Sim->MutableEntityForTesting(WorkerId);
    Worker->position = {SitePosition.x + Fixed::FromInt(1), SitePosition.y};
    Worker->order = {};
    const ResourcePool BeforeAssist = Sim->FindPlayer(0)->resources;
    TestTrue(TEXT("An unfinished site invokes assist context"),
        Controller->TryIssueWorkerMaintenanceContext(SiteId));
    Bridge->Tick(0.05f);
    Bridge->Tick(0.05f);
    TestTrue(TEXT("The worker has the site's construction order"),
        Sim->FindEntity(WorkerId)->order.type == OrderType::Build &&
        Sim->FindEntity(WorkerId)->order.target == SiteId);
    TestTrue(TEXT("Assist does not charge a second building price"), Sim->FindPlayer(0)->resources == BeforeAssist);
    if (!TestTrue(TEXT("Actual selection cycle chooses the site"),
        SelectMaintenanceEntity(*Controller, SiteId, Sim->Entities().size()))) return false;
    Controller->CancelSelectedConstruction();
    Bridge->Tick(0.05f);
    Bridge->Tick(0.05f);
    TestTrue(TEXT("Cancellation removes the unfinished structure"), Sim->FindEntity(SiteId) == nullptr);
    TestTrue(TEXT("Cancellation clears assisting worker orders"),
        Sim->FindEntity(WorkerId)->order.target != SiteId);
    TestTrue(TEXT("Controller cancellation uses the authoritative 75 percent refund"),
        Sim->FindPlayer(0)->resources == ResourcePool{BeforeAssist.material + 67, BeforeAssist.dawnshards + 7});
    Controller->Destroy();
    Camera->Destroy();
    Bridge->StopPrototypeScenario();
    Fixture.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif
