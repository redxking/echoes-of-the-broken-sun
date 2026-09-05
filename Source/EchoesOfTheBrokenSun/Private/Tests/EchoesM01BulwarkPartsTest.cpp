// Author and owner: Angelis Pseftis
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "EchoesTestSaveEnvironment.h"
#include "EchoesEntityView.h"
#include "EchoesGameUserSettings.h"
#include "EchoesSimulationSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEchoesM01BulwarkPartsTest,
    "Echoes.Runtime.Presentation.M01BulwarkParts",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FEchoesM01BulwarkPartsTest::RunTest(const FString& Parameters)
{
    FEchoesScopedTestSaveEnvironment Saves(*this);
    if (!Saves.IsReady()) return false;
    FTestWorldWrapper Wrapper;
    if (!Wrapper.CreateTestWorld(EWorldType::Game)) return false;
    auto* World = Wrapper.GetTestWorld();
    auto* Bridge = World->GetSubsystem<UEchoesSimulationSubsystem>();
    FString Feedback;
    if (!Bridge || !Bridge->StartPrototypeScenario() ||
        !Bridge->SelectOperationMode(EEchoesOperationMode::CampaignPrologue, Feedback)) return false;
    auto* Settings = UEchoesGameUserSettings::Get();
    const bool PriorReduced = Settings && Settings->IsReducedMotionEnabled();
    if (Settings) Settings->SetReducedMotionEnabled(false);
    auto* View = World->SpawnActor<AEchoesEntityView>();
    if (!View) { if (Settings) Settings->SetReducedMotionEnabled(PriorReduced); return false; }
    echoes::sim::Entity State{};
    State.id = 990042; State.owner = 0;
    State.faction = echoes::sim::Faction::MeridianCompact;
    State.type = echoes::sim::EntityType::HeavyUnit;
    State.position = echoes::sim::Vec2::FromTiles(10,10);
    State.hitPoints = State.maxHitPoints = 260; State.completed = true;
    View->ActivateForEntity(State, true);
    TArray<UStaticMeshComponent*> Components;
    View->GetComponents(Components);
    UStaticMeshComponent* Wings[2] = {nullptr,nullptr};
    UStaticMeshComponent* Body = nullptr;
    for (auto* Component : Components)
    {
        if (Component->GetName() == TEXT("BodyMesh")) Body = Component;
        if (Component->GetName() == TEXT("M01BulwarkLeftWing")) Wings[0] = Component;
        if (Component->GetName() == TEXT("M01BulwarkRightWing")) Wings[1] = Component;
    }
    TestNotNull(TEXT("M01 left wing exists"), Wings[0]);
    TestNotNull(TEXT("M01 right wing exists"), Wings[1]);
    if (Body && Wings[0] && Wings[1])
    {
        TestTrue(TEXT("M01 chassis excludes the fixed barrier"), Body->GetStaticMesh() &&
            Body->GetStaticMesh()->GetName() == TEXT("SM_Meridian_M01BulwarkBody"));
        for (int32 Side=0; Side<2; ++Side)
        {
            const auto* Wing=Wings[Side];
            TestTrue(TEXT("Wings have a visible mesh and retain body attachment"),
                Wing->IsVisible() && Wing->GetStaticMesh() && Wing->GetAttachParent()==Body);
            const FName ExpectedName = Side ? TEXT("SM_Meridian_M01BulwarkRightWing") : TEXT("SM_Meridian_M01BulwarkLeftWing");
            TestTrue(TEXT("Each derivative uses its exact left/right asset identity"),
                Wing->GetStaticMesh() && Wing->GetStaticMesh()->GetFName()==ExpectedName);
            TestTrue(TEXT("Packed wings tuck alongside chassis"),
                FMath::IsNearlyEqual(Wing->GetRelativeRotation().Yaw, Side ? 85.0f : -85.0f, .01f));
            TestTrue(TEXT("Pivot remains at the authored source hinge"),
                Wing->GetRelativeLocation().Equals(FVector(26,Side ? 24 : -24,72),.01f));
            TestTrue(TEXT("Wings cannot intercept commands or collision"),
                Wing->GetCollisionEnabled()==ECollisionEnabled::NoCollision && !Wing->GetGenerateOverlapEvents());
            TestFalse(TEXT("Wings cannot alter navigation"), Wing->CanEverAffectNavigation());
            for (int32 Slot=0; Slot<4; ++Slot)
                TestTrue(TEXT("Wing and chassis share the current faction surface"), Wing->GetMaterial(Slot)==Body->GetMaterial(Slot));
        }
        TestFalse(TEXT("Packed barrier does not claim deployed protection"), View->IsDeploymentCoverVisible());
        const FVector Root = View->GetActorLocation();
        State.deployed = true; State.deploymentFacing = echoes::sim::Vec2::FromTiles(0,1);
        View->ApplyAuthoritativeState(State,false); View->Tick(.05f);
        TestTrue(TEXT("Authoritative deployment begins a visible unfold"),
            FMath::Abs(Wings[0]->GetRelativeRotation().Yaw)<85 && FMath::Abs(Wings[0]->GetRelativeRotation().Yaw)>0);
        for (int32 Frame=0; Frame<36; ++Frame) View->Tick(1.0f/60.0f);
        for (const auto* Wing : Wings)
        {
            TestTrue(TEXT("Deployed wing restores exact approved assembled transform"), Wing->GetRelativeRotation().IsNearlyZero(.01f));
            TestTrue(TEXT("Deployed screen follows body deployment facing"),
                Wing->GetComponentQuat().Equals(Body->GetComponentQuat(),.001f));
        }
        TestTrue(TEXT("Deployed barrier is reported from the visible wing assembly"), View->IsDeploymentCoverVisible());
        TestTrue(TEXT("Deploy pose never displaces the authoritative root"), View->GetActorLocation().Equals(Root,.01f));
        TestTrue(TEXT("Body faces authoritative north deployment"), FMath::Abs(FMath::FindDeltaAngleDegrees(Body->GetRelativeRotation().Yaw,90.0f))<.2f);
        for (int32 Frame=0; Frame<180; ++Frame)
        {
            View->Tick(1.0f/60.0f);
            TestTrue(TEXT("Deployed idle screen keeps authoritative facing beyond the servo cycle"),
                FMath::Abs(FMath::FindDeltaAngleDegrees(Body->GetRelativeRotation().Yaw,90.0f))<.2f &&
                Wings[0]->GetComponentQuat().Equals(Body->GetComponentQuat(),.001f));
        }
        State.deployed=false; View->ApplyAuthoritativeState(State,false); View->Tick(.05f);
        TestTrue(TEXT("Normal pack-up begins a bounded fold"),
            FMath::Abs(Wings[0]->GetRelativeRotation().Yaw)>0 && FMath::Abs(Wings[0]->GetRelativeRotation().Yaw)<85);
        for (int32 Frame=0; Frame<30; ++Frame) View->Tick(1.0f/60.0f);
        TestTrue(TEXT("Normal pack-up reaches its tucked hinge pose"),
            FMath::IsNearlyEqual(Wings[0]->GetRelativeRotation().Yaw,-85.0f,.01f));
        State.deployed=true; View->ApplyAuthoritativeState(State,false);
        for (int32 Frame=0; Frame<30; ++Frame) View->Tick(1.0f/60.0f);
        State.owner=1; View->ApplyAuthoritativeState(State,false);
        TestTrue(TEXT("Appearance refresh retains deployed wing pose"), Wings[0]->GetRelativeRotation().IsNearlyZero(.01f));
        if (Settings) Settings->SetReducedMotionEnabled(true);
        State.deployed=false; View->ApplyAuthoritativeState(State,false); View->Tick(.016f);
        TestTrue(TEXT("Reduced motion shows the actual packed state without a fold animation"),
            FMath::IsNearlyEqual(Wings[0]->GetRelativeRotation().Yaw,-85.0f,.01f));
        const FTransform Reduced=Wings[0]->GetRelativeTransform(); View->Tick(.2f);
        TestTrue(TEXT("Reduced packed geometry remains steady"), Wings[0]->GetRelativeTransform().Equals(Reduced,.01f));
        View->PrepareForPool();
        for (const auto* Wing : Wings)
        {
            TestFalse(TEXT("Pooled wing is hidden"), Wing->IsVisible());
            TestNull(TEXT("Pooled wing releases its mesh"), Wing->GetStaticMesh());
            TestNull(TEXT("Pooled wing releases faction material"), Wing->GetMaterial(0));
        }
        if (Bridge->SelectOperationMode(EEchoesOperationMode::Skirmish,Feedback))
        {
            State.deployed=true;
            View->ActivateForEntity(State,true);
            TestTrue(TEXT("Other operations preserve their existing deployed cover path"), View->IsDeploymentCoverVisible());
            TestNull(TEXT("Other operations cannot retain derivative mesh references"), Wings[0]->GetStaticMesh());
            TestTrue(TEXT("Other operations retain the standard complete Bulwark"), Body->GetStaticMesh() &&
                Body->GetStaticMesh()->GetName()==TEXT("SM_Meridian_Bulwark"));
            TestFalse(TEXT("Other operations cannot inherit an M01 wing"), Wings[0]->IsVisible());
        }
        else AddError(TEXT("Cannot establish non-M01 control scenario"));
    }
    if (Settings) Settings->SetReducedMotionEnabled(PriorReduced);
    View->Destroy();
    Bridge->StopPrototypeScenario();
    AddInfo(TEXT("Synthetic authoritative presentation states; runtime seam and motion inspection remain separate evidence."));
    return true;
}
#endif
