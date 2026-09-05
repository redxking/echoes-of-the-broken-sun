#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "EchoesTestSaveEnvironment.h"
#include "EchoesEntityView.h"
#include "EchoesGameUserSettings.h"
#include "EchoesSimulationSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEchoesM01WellExpiryPresentationTest,
    "Echoes.Runtime.Presentation.M01WellExpiry",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext |
    EAutomationTestFlags::EngineFilter)

bool FEchoesM01WellExpiryPresentationTest::RunTest(const FString& Parameters)
{
    FEchoesScopedTestSaveEnvironment Saves(*this);
    if (!Saves.IsReady()) return false;
    FTestWorldWrapper Wrapper;
    if (!Wrapper.CreateTestWorld(EWorldType::Game)) return false;
    UWorld* World = Wrapper.GetTestWorld();
    auto* Bridge = World->GetSubsystem<UEchoesSimulationSubsystem>();
    FString Feedback;
    if (!TestNotNull(TEXT("World owns bridge"), Bridge) ||
        !Bridge->StartPrototypeScenario() ||
        !TestTrue(TEXT("M01 selected"), Bridge->SelectOperationMode(
            EEchoesOperationMode::CampaignPrologue, Feedback))) return false;
    auto* Settings = UEchoesGameUserSettings::Get();
    const bool PriorMotion = Settings && Settings->IsReducedMotionEnabled();
    if (Settings) Settings->SetReducedMotionEnabled(false);
    auto* View = World->SpawnActor<AEchoesEntityView>();
    if (!TestNotNull(TEXT("Well presentation spawns"), View))
    {
        if (Settings) Settings->SetReducedMotionEnabled(PriorMotion);
        Bridge->StopPrototypeScenario();
        return false;
    }
    echoes::sim::Entity State{};
    State.id = 990001; State.owner = 0;
    State.type = echoes::sim::EntityType::FutureWell;
    State.position = echoes::sim::Vec2::FromTiles(32, 32);
    State.hitPoints = State.maxHitPoints = 1;
    State.completed = true;
    State.wellChoice = echoes::sim::FutureWellChoice::Reshape;
    State.reshapeUntilTick = 1800;
    View->ActivateForEntity(State, true);
    UStaticMeshComponent* Orbit = nullptr;
    UStaticMeshComponent* Core = nullptr;
    UStaticMeshComponent* Body = nullptr;
    UStaticMeshComponent* CollapseRim = nullptr;
    TArray<UStaticMeshComponent*> Components;
    View->GetComponents(Components);
    for (auto* Component : Components)
    {
        if (Component->GetFName() == TEXT("FutureWellOrbitOuter")) Orbit = Component;
        if (Component->GetFName() == TEXT("FutureWellCore")) Core = Component;
        if (Component->GetFName() == TEXT("BodyMesh")) Body = Component;
        if (Component->GetFName() == TEXT("FutureWellCollapseRim")) CollapseRim = Component;
    }
    if (TestNotNull(TEXT("Orbit exists"), Orbit) && TestNotNull(TEXT("Core exists"), Core))
    {
        const FQuat Initial = Orbit->GetRelativeRotation().Quaternion();
        View->Tick(.25f);
        TestFalse(TEXT("Active Well orbit advances"), Initial.Equals(Orbit->GetRelativeRotation().Quaternion()));
        auto* Material = Cast<UMaterialInstanceDynamic>(Core->GetMaterial(3));
        const float ActiveEmission = Material ? Material->K2_GetScalarParameterValue(TEXT("EmissiveStrength")) : 0;
        const FQuat LastActive = Orbit->GetRelativeRotation().Quaternion();
        // Only the deadline changes: appearance must react without a new choice.
        State.reshapeUntilTick = 0;
        View->ApplyAuthoritativeState(State, false);
        TestFalse(TEXT("Expired route telegraph is inactive"), View->IsReshapeTelegraphActive());
        TestTrue(TEXT("Recorded protocol identity survives expiry"),
                 View->GetFutureWellVisualChoice() == echoes::sim::FutureWellChoice::Reshape);
        TestTrue(TEXT("Expiry preserves orbit pose without snapping"), LastActive.Equals(Orbit->GetRelativeRotation().Quaternion()));
        View->Tick(.5f);
        TestTrue(TEXT("Expired orbit stops"), LastActive.Equals(Orbit->GetRelativeRotation().Quaternion()));
        Material = Cast<UMaterialInstanceDynamic>(Core->GetMaterial(3));
        TestTrue(TEXT("Expired emission becomes a dim residual"), Material && ActiveEmission > 0 &&
                 Material->K2_GetScalarParameterValue(TEXT("EmissiveStrength")) < ActiveEmission * .25f);
        State.reshapeUntilTick = 3600;
        View->ApplyAuthoritativeState(State, false);
        const FQuat Reactivated = Orbit->GetRelativeRotation().Quaternion();
        View->Tick(.25f);
        TestFalse(TEXT("A new authoritative window resumes motion"), Reactivated.Equals(Orbit->GetRelativeRotation().Quaternion()));
    }
    if (TestNotNull(TEXT("Physical Well basin exists"), Body) &&
        TestNotNull(TEXT("Harvest collapse rim exists"), CollapseRim))
    {
        for (const auto Choice : {echoes::sim::FutureWellChoice::Dormant,
             echoes::sim::FutureWellChoice::Harvest, echoes::sim::FutureWellChoice::Preserve,
             echoes::sim::FutureWellChoice::Reshape})
        {
            State.wellChoice = Choice;
            View->ApplyAuthoritativeState(State, false); View->Tick(.1f);
            if (Choice == echoes::sim::FutureWellChoice::Harvest)
            {
                const FBox CollapseBounds = CollapseRim->GetStaticMesh()->GetBoundingBox().TransformBy(
                    CollapseRim->GetComponentTransform());
                TestTrue(TEXT("Harvest replaces the tall Well with a visible terminal remnant"),
                    View->IsFutureWellCollapsedRemnantVisible() && !Body->IsVisible());
                TestTrue(TEXT("Harvest remnant remains within the named 600 cm collapse radius"),
                    CollapseBounds.GetExtent().X <= 330 && CollapseBounds.GetExtent().Y <= 330);
                TestTrue(TEXT("Harvest remnant is a low collapsed basin rather than a tall active Well"),
                    CollapseBounds.GetSize().Z < 40);
            }
            else
            {
                const FBox Bounds = Body->GetStaticMesh()->GetBoundingBox().TransformBy(Body->GetComponentTransform());
                TestTrue(TEXT("M01 physical basin fits its compact interaction footprint through active-state changes"),
                    Bounds.GetExtent().X <= 110 && Bounds.GetExtent().Y <= 110);
                TestTrue(TEXT("Active basin retains its vertical construction"), Bounds.GetSize().Z > 40);
            }
        }
    }
    View->PrepareForPool(); View->Destroy();
    if (Settings) Settings->SetReducedMotionEnabled(PriorMotion);
    Bridge->StopPrototypeScenario();
    AddInfo(TEXT("Synthetic presentation snapshots; actual 90-second EDT expiry is a separate retained check."));
    return true;
}
#endif
