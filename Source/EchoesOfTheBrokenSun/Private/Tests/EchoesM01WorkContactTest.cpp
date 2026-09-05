// Author and owner: Angelis Pseftis
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "EchoesTestSaveEnvironment.h"
#include "EchoesEntityView.h"
#include "EchoesGameUserSettings.h"
#include "EchoesSimulationSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEchoesM01WorkContactTest,
    "Echoes.Runtime.Presentation.M01WorkContact",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FEchoesM01WorkContactTest::RunTest(const FString& Parameters)
{
    FEchoesScopedTestSaveEnvironment Saves(*this);
    if (!Saves.IsReady()) return false;
    FTestWorldWrapper Wrapper;
    if (!Wrapper.CreateTestWorld(EWorldType::Game)) return false;
    auto* World = Wrapper.GetTestWorld();
    auto* Bridge = World->GetSubsystem<UEchoesSimulationSubsystem>();
    FString Feedback;
    if (!Bridge || !Bridge->StartPrototypeScenario() || !Bridge->SelectOperationMode(
        EEchoesOperationMode::CampaignPrologue, Feedback)) return false;
    auto* Settings = UEchoesGameUserSettings::Get();
    const bool PriorMotion = Settings && Settings->IsReducedMotionEnabled();
    const bool PriorFlash = Settings && Settings->IsReducedFlashingEnabled();
    if (Settings) { Settings->SetReducedMotionEnabled(false); Settings->SetReducedFlashingEnabled(false); }
    auto* View = World->SpawnActor<AEchoesEntityView>();
    if (!TestNotNull(TEXT("Worker presentation spawns"), View))
    {
        if (Settings) { Settings->SetReducedMotionEnabled(PriorMotion); Settings->SetReducedFlashingEnabled(PriorFlash); }
        Bridge->StopPrototypeScenario();
        return false;
    }
    echoes::sim::Entity State{};
    State.id = 990004; State.owner = 0; State.faction = echoes::sim::Faction::MeridianCompact;
    State.type = echoes::sim::EntityType::Worker;
    State.position = echoes::sim::Vec2::FromTiles(10, 10);
    State.hitPoints = State.maxHitPoints = 90; State.completed = true;
    State.harvestTicks = 8; State.harvestSlotHeld = true; State.harvestState = echoes::sim::HarvestState::Harvesting; State.order.type = echoes::sim::OrderType::Gather;
    State.order.destination = echoes::sim::Vec2::FromTiles(10, 11);
    View->ActivateForEntity(State, true);
    UStaticMeshComponent* Beam = nullptr;
    UStaticMeshComponent* Body = nullptr;
    TArray<UStaticMeshComponent*> Components;
    View->GetComponents(Components);
    for (auto* Component : Components)
    {
        if (Component->GetFName() == TEXT("GatherBeam")) Beam = Component;
        if (Component->GetFName() == TEXT("BodyMesh")) Body = Component;
    }
    if (TestNotNull(TEXT("Gather beam exists"), Beam) && TestNotNull(TEXT("Worker body exists"), Body))
    {
        View->Tick(.1f);
        const auto CheckEndpoints = [&]()
        {
            TestTrue(TEXT("Only actual gathering draws the contact"), Beam->IsVisible());
            const FVector Start = Body->GetComponentTransform().TransformPosition(FVector(82,52,54));
            const FVector Target = Bridge->SimToWorld(State.order.destination) + FVector(0,0,65);
            const FVector EndA = Beam->GetComponentTransform().TransformPosition(FVector(0,0,-50));
            const FVector EndB = Beam->GetComponentTransform().TransformPosition(FVector(0,0,50));
            TestTrue(TEXT("Beam begins at the animated drill tip"), EndA.Equals(Start,.1));
            TestTrue(TEXT("Beam ends at the non-axis-aligned resource destination"), EndB.Equals(Target,.1));
            TestTrue(TEXT("Beam cannot intercept ground input"), Beam->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
            TestFalse(TEXT("Beam cannot affect navigation"), Beam->CanEverAffectNavigation());
        };
        CheckEndpoints();
        View->Tick(.23f);
        CheckEndpoints();
        if (Settings)
        {
            Settings->SetReducedFlashingEnabled(true);
            View->Tick(.2f);
            auto* Material = Cast<UMaterialInstanceDynamic>(Beam->GetMaterial(0));
            const float First = Material ? Material->K2_GetScalarParameterValue(TEXT("EmissiveStrength")) : -1;
            View->Tick(.4f);
            TestTrue(TEXT("Reduced flashing keeps steady low beam emission"), Material && First <= 1.0f &&
                FMath::IsNearlyEqual(First, Material->K2_GetScalarParameterValue(TEXT("EmissiveStrength"))));
            Settings->SetReducedMotionEnabled(true);
            View->Tick(.2f);
            CheckEndpoints();
        }
        for (const auto Order : {echoes::sim::OrderType::Move, echoes::sim::OrderType::Deliver, echoes::sim::OrderType::None})
        {
            State.order.type = Order; // stale work ticks must not imply current work
            View->ApplyAuthoritativeState(State, false); View->Tick(.1f);
            TestFalse(TEXT("Travel, delivery and cancellation stop the gather contact"), Beam->IsVisible());
        }
        View->PrepareForPool();
        TestFalse(TEXT("Pooling clears target-bound work"), View->IsGatherBeamActive());
    }
    View->Destroy();
    if (Settings) { Settings->SetReducedMotionEnabled(PriorMotion); Settings->SetReducedFlashingEnabled(PriorFlash); }
    Bridge->StopPrototypeScenario();
    AddInfo(TEXT("Synthetic presentation snapshots; ordinary gather movie is separate EDT evidence."));
    return true;
}
#endif
