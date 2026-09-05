// Author and owner: Angelis Pseftis
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "EchoesTestSaveEnvironment.h"
#include "EchoesEntityView.h"
#include "EchoesGameUserSettings.h"
#include "EchoesSimulationSubsystem.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEchoesM01SurveyorRigTest,
    "Echoes.Runtime.Presentation.M01SurveyorRig",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FEchoesM01SurveyorRigTest::RunTest(const FString& Parameters)
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
    if (Settings) Settings->SetReducedMotionEnabled(false);
    auto* View = World->SpawnActor<AEchoesEntityView>();
    if (!View) { if (Settings) Settings->SetReducedMotionEnabled(PriorMotion); return false; }
    echoes::sim::Entity State{};
    State.id = 990041; State.owner = 0; State.faction = echoes::sim::Faction::MeridianCompact;
    State.type = echoes::sim::EntityType::Worker;
    // Mirrors the first live M01 gather departure: the worker begins at the
    // evacuation-margin service edge and leaves on the initial 63-degree
    // diagonal toward resource 24,16 before its gather/delivery transition.
    State.position = Bridge->WorldToSim(FVector(-3600.0f, -4000.0f, 0.0f));
    State.hitPoints = State.maxHitPoints = 90; State.completed = true;
    State.order.type = echoes::sim::OrderType::Gather;
    State.order.destination = echoes::sim::Vec2::FromTiles(24, 16);
    View->ActivateForEntity(State, true);
    TArray<UStaticMeshComponent*> Components;
    View->GetComponents(Components);
    UStaticMeshComponent* Feet[2] = {nullptr, nullptr};
    UStaticMeshComponent* Upper[2] = {nullptr, nullptr};
    UStaticMeshComponent* Body = nullptr;
    UStaticMeshComponent* Lower[2] = {nullptr, nullptr};
    int32 Parts = 0;
    for (auto* Component : Components)
    {
        if (Component->GetName() == TEXT("M01SurveyorLeftFoot")) Feet[0] = Component;
        if (Component->GetName() == TEXT("M01SurveyorRightFoot")) Feet[1] = Component;
        if (Component->GetName() == TEXT("M01SurveyorLeftUpper")) Upper[0] = Component;
        if (Component->GetName() == TEXT("M01SurveyorRightUpper")) Upper[1] = Component;
        if (Component->GetName() == TEXT("BodyMesh")) Body = Component;
        if (Component->GetName() == TEXT("M01SurveyorLeftLower")) Lower[0] = Component;
        if (Component->GetName() == TEXT("M01SurveyorRightLower")) Lower[1] = Component;
        if (Component->GetName().StartsWith(TEXT("M01Surveyor")))
        {
            ++Parts;
            TestTrue(TEXT("Articulated parts are visible and meshed"), Component->IsVisible() && Component->GetStaticMesh());
            TestTrue(TEXT("Legs cannot affect any command/selection ground hit"), Component->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
            TestFalse(TEXT("Legs never affect navigation"), Component->CanEverAffectNavigation());
        }
    }
    TestEqual(TEXT("M01 uses six separate leg parts"), Parts, 6);
    if (Feet[0] && Feet[1] && Body)
    {
        USceneComponent* LegRoot = Feet[0]->GetAttachParent();
        TestTrue(TEXT("M01 leg parts share an articulated pelvis root"), LegRoot != nullptr);
        TestTrue(TEXT("M01 torso excludes welded legs"), Body->GetStaticMesh() &&
            Body->GetStaticMesh()->GetName() == TEXT("SM_Meridian_M01SurveyorBody"));
        FVector PreviousFeet[2] = {Feet[0]->GetComponentLocation(), Feet[1]->GetComponentLocation()};
        FVector PreviousRoot = View->GetActorLocation();
        float PreviousPelvisYaw = Body->GetRelativeRotation().Yaw;
        int32 StancePairs = 0; int32 SwingSamples = 0; float MaxStanceDrift = 0;
        for (int32 Step = 0; Step < 140; ++Step)
        {
            // Faithful initial M01 gather approach: live I1 starts with a
            // diagonal heading near 63 degrees, then bends into the archive
            // service route. The axis shuttle below remains a separate
            // reversal stress fixture.
            const float Angle = Step < 70 ? FMath::DegreesToRadians(63.0f) :
                FMath::DegreesToRadians(63.0f + (Step - 70) * .69f);
            const int32 DX = FMath::RoundToInt(41.0f * FMath::Cos(Angle));
            const int32 DY = FMath::RoundToInt(41.0f * FMath::Sin(Angle));
            State.position = echoes::sim::Vec2::FromRaw(State.position.x.Raw() + DX, State.position.y.Raw() + DY);
            if (Step == 50 || Step == 51)
            {
                const FVector Before = Feet[0]->GetComponentLocation();
                State.owner = Step == 50 ? 1 : 0;
                View->ApplyAuthoritativeState(State, false);
                TestTrue(TEXT("Appearance refresh preserves the current planted/swing endpoint"),
                    Feet[0]->GetComponentLocation().Equals(Before,.001));
            }
            else View->ApplyAuthoritativeState(State, false);
            View->Tick(.02f);
            const FVector Root = View->GetActorLocation();
            if (LegRoot)
            {
                const float BodyYaw = Body->GetRelativeRotation().Yaw;
                TestTrue(TEXT("Surveyor torso and leg root retain one pelvis frame"),
                    FMath::IsNearlyZero(FMath::FindDeltaAngleDegrees(BodyYaw, LegRoot->GetRelativeRotation().Yaw), .01f));
                if (Step > 0)
                {
                    TestTrue(TEXT("Surveyor pelvis turn remains bounded per 20 ms view update"),
                        FMath::Abs(FMath::FindDeltaAngleDegrees(PreviousPelvisYaw, BodyYaw)) <= 14.41f);
                }
                PreviousPelvisYaw = BodyYaw;
            }
            for (int32 Side = 0; Side < 2; ++Side)
            {
                const FVector Foot = Feet[Side]->GetComponentLocation();
                const float Z = Foot.Z - Root.Z;
                if (Z > 10.0f) ++SwingSamples;
                if (FMath::IsNearlyEqual(Z, 3.0f, .1f) &&
                    FMath::IsNearlyEqual(PreviousFeet[Side].Z - PreviousRoot.Z, 3.0f, .1f) &&
                    FVector::Dist2D(Root, PreviousRoot) > 1.0f)
                {
                    ++StancePairs;
                    MaxStanceDrift = FMath::Max(MaxStanceDrift, static_cast<float>(FVector::Dist2D(Foot, PreviousFeet[Side])));
                }
                TestTrue(TEXT("Soles never fall below the authored ground envelope"), Z >= 2.9f);
                PreviousFeet[Side] = Foot;
            }
            for (int32 Side = 0; Side < 2; ++Side)
            {
                if (!Lower[Side]) continue;
                TestTrue(TEXT("Shin geometry clears the ground at its ankle fitting"),
                    Lower[Side]->Bounds.Origin.Z - Lower[Side]->Bounds.BoxExtent.Z >= Root.Z + 2.0f);
                const FVector Ankle = Lower[Side]->GetComponentTransform().TransformPosition(FVector(46.0f, 0.0f, 0.0f));
                const FVector ExpectedAnkle = Feet[Side]->GetComponentTransform().TransformPosition(FVector(0.0f, 0.0f, 10.0f));
                TestTrue(TEXT("Final rendered shin endpoint reaches its scaled ankle fitting"),
                    FVector::Dist(Ankle, ExpectedAnkle) <= 1.0f);
            }
            PreviousRoot = Root;
        }
        TestTrue(TEXT("Observed many planted samples during actual root translation and turning"), StancePairs > 50);
        TestTrue(FString::Printf(TEXT("Stance sole stays fixed in world (max %.3f cm)"), MaxStanceDrift), MaxStanceDrift <= 1.0f);
        TestTrue(TEXT("Swing clamps lift clear of terrain"), SwingSamples > 20);
        TestEqual(TEXT("Initial diagonal M01 gather departure uses no emergency replant"),
            View->GetM01SurveyorEmergencyReplantCount(), 0);
        // The first route reaches a real presentation Gather state, then a
        // Deliver state, before the reversal-only movement fixture begins.
        State.harvestTicks = 8; State.harvestSlotHeld = true; State.harvestState = echoes::sim::HarvestState::Harvesting;
        State.order.type = echoes::sim::OrderType::Gather;
        View->ApplyAuthoritativeState(State, false); View->Tick(.05f);
        TestTrue(TEXT("Initial diagonal route reaches active gather presentation"), View->IsGatherBeamActive());
        State.harvestTicks = 0; State.harvestSlotHeld = false; State.harvestState = echoes::sim::HarvestState::Idle; State.cargo = 12;
        State.order.type = echoes::sim::OrderType::Deliver;
        View->ApplyAuthoritativeState(State, false); View->Tick(.05f);
        TestFalse(TEXT("Gather contact clears for authoritative delivery transition"), View->IsGatherBeamActive());
        View->ApplyAuthoritativeState(State, false);
        // Displayed-path-derived J1 stress fixture. Positions are resampled
        // from the retained 60 Hz live trace at 20 Hz snapshot intervals,
        // preserving the observed route duration and turn velocity. These are
        // not asserted to be original authoritative snapshots.
        const TArray<FVector> J1Forward = {
            {-3600.0f, -4000.0f, 0.0f}, // +0.00s,
            {-3600.0f, -4000.0f, 0.0f}, // +0.05s,
            {-3596.4f, -3992.8f, 0.0f}, // +0.10s,
            {-3585.9f, -3971.7f, 0.0f}, // +0.15s,
            {-3575.2f, -3950.4f, 0.0f}, // +0.20s,
            {-3561.7f, -3923.5f, 0.0f}, // +0.25s,
            {-3543.9f, -3887.7f, 0.0f}, // +0.30s,
            {-3526.3f, -3852.6f, 0.0f}, // +0.35s,
            {-3510.3f, -3820.7f, 0.0f}, // +0.40s,
            {-3496.8f, -3793.5f, 0.0f}, // +0.45s,
            {-3482.3f, -3764.5f, 0.0f}, // +0.50s,
            {-3464.1f, -3728.3f, 0.0f}, // +0.55s,
            {-3446.4f, -3692.7f, 0.0f}, // +0.60s,
            {-3430.3f, -3660.7f, 0.0f}, // +0.65s,
            {-3417.0f, -3634.0f, 0.0f}, // +0.70s,
            {-3402.5f, -3604.9f, 0.0f}, // +0.75s,
            {-3384.2f, -3568.3f, 0.0f}, // +0.80s,
            {-3366.3f, -3532.7f, 0.0f}, // +0.85s,
            {-3350.2f, -3500.4f, 0.0f}, // +0.90s,
            {-3336.8f, -3473.6f, 0.0f}, // +0.95s,
            {-3322.6f, -3445.2f, 0.0f}, // +1.00s,
            {-3304.2f, -3408.3f, 0.0f}, // +1.05s,
            {-3289.8f, -3379.7f, 0.0f}, // +1.10s,
            {-3284.2f, -3368.4f, 0.0f}, // +1.15s,
            {-3281.5f, -3363.0f, 0.0f}, // +1.20s,
            {-3280.4f, -3360.9f, 0.0f}, // +1.25s,
            {-3280.0f, -3360.0f, 0.0f}, // +1.30s,
            {-3279.8f, -3359.7f, 0.0f}, // +1.35s,
            {-3279.7f, -3359.5f, 0.0f}, // +1.40s,
            {-3279.7f, -3359.4f, 0.0f}, // +1.45s
        };
        const TArray<FVector> J1Withdraw = {
            {-3279.7f, -3359.4f, 0.0f}, // +0.00s,
            {-3290.8f, -3369.7f, 0.0f}, // +0.05s,
            {-3311.6f, -3389.2f, 0.0f}, // +0.10s,
            {-3328.9f, -3405.3f, 0.0f}, // +0.15s,
            {-3351.8f, -3426.6f, 0.0f}, // +0.20s,
            {-3380.4f, -3453.3f, 0.0f}, // +0.25s,
            {-3409.4f, -3480.4f, 0.0f}, // +0.30s,
            {-3437.6f, -3506.7f, 0.0f}, // +0.35s,
            {-3457.4f, -3525.2f, 0.0f}, // +0.40s,
            {-3481.7f, -3547.8f, 0.0f}, // +0.45s,
            {-3510.8f, -3575.0f, 0.0f}, // +0.50s,
            {-3540.0f, -3602.2f, 0.0f}, // +0.55s,
            {-3568.3f, -3628.6f, 0.0f}, // +0.60s,
            {-3588.1f, -3647.1f, 0.0f}, // +0.65s,
            {-3612.6f, -3669.9f, 0.0f}, // +0.70s,
            {-3641.5f, -3696.9f, 0.0f}, // +0.75s,
            {-3671.0f, -3724.4f, 0.0f}, // +0.80s
        };
        // The retained capture continues for .5841 seconds between the last
        // withdrawal and the first return sample. These 20 Hz resamples keep
        // that ordinary displayed route continuous rather than fabricating a
        // 388 cm authoritative jump in the fixture.
        const TArray<FVector> J1WithdrawReturnBridge = {
            {-3699.0f, -3750.5f, 0.0f}, // +0.05s
            {-3719.2f, -3769.3f, 0.0f}, // +0.10s
            {-3743.6f, -3791.9f, 0.0f}, // +0.15s
            {-3772.9f, -3819.1f, 0.0f}, // +0.20s
            {-3802.6f, -3846.6f, 0.0f}, // +0.25s
            {-3830.6f, -3872.6f, 0.0f}, // +0.30s
            {-3850.8f, -3891.2f, 0.0f}, // +0.35s
            {-3875.3f, -3914.0f, 0.0f}, // +0.40s
            {-3904.7f, -3941.2f, 0.0f}, // +0.45s
            {-3934.0f, -3968.3f, 0.0f}, // +0.50s
            {-3950.3f, -3983.4f, 0.0f}, // +0.55s
        };
        const TArray<FVector> J1Return = {
            {-3955.3f, -3988.0f, 0.0f}, // +0.00s,
            {-3944.9f, -3976.8f, 0.0f}, // +0.05s,
            {-3921.5f, -3952.3f, 0.0f}, // +0.10s,
            {-3896.1f, -3925.8f, 0.0f}, // +0.15s,
            {-3871.8f, -3900.5f, 0.0f}, // +0.20s,
            {-3851.6f, -3879.5f, 0.0f}, // +0.25s,
            {-3829.5f, -3856.5f, 0.0f}, // +0.30s,
            {-3801.1f, -3827.0f, 0.0f}, // +0.35s,
            {-3773.7f, -3798.5f, 0.0f}, // +0.40s,
            {-3748.8f, -3772.7f, 0.0f}, // +0.45s,
            {-3728.0f, -3751.1f, 0.0f}, // +0.50s,
            {-3705.7f, -3727.8f, 0.0f}, // +0.55s,
            {-3677.5f, -3698.5f, 0.0f}, // +0.60s,
            {-3650.0f, -3669.9f, 0.0f}, // +0.65s,
            {-3625.5f, -3644.4f, 0.0f}, // +0.70s,
            {-3604.2f, -3622.4f, 0.0f}, // +0.75s,
            {-3577.5f, -3594.5f, 0.0f}, // +0.80s,
            {-3550.9f, -3566.8f, 0.0f}, // +0.85s,
            {-3525.9f, -3540.7f, 0.0f}, // +0.90s,
            {-3504.8f, -3518.7f, 0.0f}, // +0.95s,
            {-3482.7f, -3495.6f, 0.0f}, // +1.00s,
            {-3454.6f, -3466.3f, 0.0f}, // +1.05s,
            {-3427.2f, -3437.6f, 0.0f}, // +1.10s,
            {-3402.6f, -3411.9f, 0.0f}, // +1.15s,
            {-3373.5f, -3381.4f, 0.0f}, // +1.20s,
            {-3355.0f, -3362.1f, 0.0f}, // +1.25s,
            {-3334.2f, -3340.3f, 0.0f}, // +1.30s,
            {-3324.1f, -3329.7f, 0.0f}, // +1.35s,
            {-3320.2f, -3325.6f, 0.0f}, // +1.40s,
            {-3318.3f, -3323.6f, 0.0f}, // +1.45s,
            {-3317.5f, -3322.8f, 0.0f}, // +1.50s,
            {-3317.2f, -3322.5f, 0.0f}, // +1.55s,
            {-3317.1f, -3322.4f, 0.0f}, // +1.60s
        };
        const TArray<FVector> J1FinalWithdraw = {
            {-3317.0f, -3322.3f, 0.0f}, // +0.00s,
            {-3331.6f, -3336.8f, 0.0f}, // +0.05s,
            {-3354.6f, -3359.6f, 0.0f}, // +0.10s,
            {-3379.5f, -3384.3f, 0.0f}, // +0.15s,
            {-3401.4f, -3406.1f, 0.0f}, // +0.20s,
            {-3422.5f, -3426.9f, 0.0f}, // +0.25s,
            {-3449.6f, -3453.9f, 0.0f}, // +0.30s,
            {-3477.4f, -3481.4f, 0.0f}, // +0.35s,
            {-3505.1f, -3509.0f, 0.0f}, // +0.40s,
            {-3527.6f, -3531.3f, 0.0f}, // +0.45s,
            {-3548.8f, -3552.3f, 0.0f}, // +0.50s,
            {-3575.7f, -3579.0f, 0.0f}, // +0.55s
        };
        auto ReplayJ1 = [&](const TArray<FVector>& Points, echoes::sim::OrderType Order, const TCHAR* Segment)
        {
            State.order.type = Order;
            for (int32 Index = 0; Index < Points.Num(); ++Index)
            {
                const FVector& Point = Points[Index];
                const int32 EmergencyBeforePoint = View->GetM01SurveyorEmergencyReplantCount();
                State.position = Bridge->WorldToSim(Point);
                View->ApplyAuthoritativeState(State, false);
                for (int32 Frame = 0; Frame < 3; ++Frame) View->Tick(1.0f / 60.0f);
                if (LegRoot)
                {
                    TestTrue(TEXT("J1 rendered torso and legs remain in the same pelvis frame"),
                        FMath::IsNearlyZero(FMath::FindDeltaAngleDegrees(
                            Body->GetRelativeRotation().Yaw, LegRoot->GetRelativeRotation().Yaw), .01f));
                }
                for (int32 Side = 0; Side < 2; ++Side)
                {
                    if (!Lower[Side]) continue;
                    const FVector Ankle = Lower[Side]->GetComponentTransform().TransformPosition(FVector(46.0f, 0.0f, 0.0f));
                    const FVector ExpectedAnkle = Feet[Side]->GetComponentTransform().TransformPosition(FVector(0.0f, 0.0f, 10.0f));
                    TestTrue(TEXT("J1 final rendered ankle endpoint remains exact"),
                        FVector::Dist(Ankle, ExpectedAnkle) <= 1.0f);
                }
                if (View->GetM01SurveyorEmergencyReplantCount() != EmergencyBeforePoint)
                {
                    AddError(FString::Printf(TEXT("J1 guard boundary segment=%s point=%d root=%s phase=%.4f target=%.2f feet=(%s,%s) swing=(%d,%d) starts=%d landings=%d emergency=%d"),
                        Segment, Index, *View->GetActorLocation().ToString(), View->GetM01SurveyorGaitPhase(),
                        View->GetM01SurveyorTargetHeadingYaw(), *View->GetM01SurveyorFootWorld(0).ToString(),
                        *View->GetM01SurveyorFootWorld(1).ToString(), View->IsM01SurveyorFootSwinging(0) ? 1 : 0,
                        View->IsM01SurveyorFootSwinging(1) ? 1 : 0, View->GetM01SurveyorPlannedSwingStartCount(),
                        View->GetM01SurveyorPlannedLandingCount(), View->GetM01SurveyorEmergencyReplantCount()));
                }
            }
        };
        const int32 EmergencyBeforeJ1 = View->GetM01SurveyorEmergencyReplantCount();
        State.harvestTicks = 0; State.harvestSlotHeld = false; State.harvestState = echoes::sim::HarvestState::Idle; State.cargo = 0;
        // Start the displayed-path fixture from its retained first point,
        // rather than teleporting the completed I1 route and then measuring
        // unrelated interpolation catch-up as J1 motion.
        State.order.type = echoes::sim::OrderType::Gather;
        State.position = Bridge->WorldToSim(J1Forward[0]);
        View->ApplyAuthoritativeState(State, true);
        ReplayJ1(J1Forward, echoes::sim::OrderType::Gather, TEXT("forward"));
        State.harvestTicks = 8; State.harvestSlotHeld = true; State.harvestState = echoes::sim::HarvestState::Harvesting; View->ApplyAuthoritativeState(State, false);
        for (int32 Frame = 0; Frame < 18; ++Frame) View->Tick(1.0f / 60.0f);
        // First J1 withdrawal: authority reverses target direction while the
        // displayed root is still at the gathered point. The zero-display
        // frame must retain both anchors; the first visible frame performs
        // one C0 late swing and preserves its opposite support.
        State.harvestTicks = 0; State.harvestSlotHeld = false; State.harvestState = echoes::sim::HarvestState::Idle; State.cargo = 12; State.order.type = echoes::sim::OrderType::Deliver;
        // [0] duplicates the final forward point. Apply [1] to create the
        // observed changed target before intentionally holding display still.
        State.position = Bridge->WorldToSim(J1Withdraw[1]);
        View->ApplyAuthoritativeState(State, false);
        // Use the final non-stationary 63-degree displayed segment; the
        // following gathered samples are intentionally stationary.
        const FVector ForwardDisplay = (J1Forward[20] - J1Forward[19]).GetSafeNormal2D();
        const FVector WithdrawTarget(FMath::Cos(FMath::DegreesToRadians(View->GetM01SurveyorTargetHeadingYaw())),
            FMath::Sin(FMath::DegreesToRadians(View->GetM01SurveyorTargetHeadingYaw())), 0.0f);
        TestTrue(TEXT("J1 withdrawal target opposes the retained displayed travel"),
            FVector::DotProduct(ForwardDisplay, WithdrawTarget) < -.35f);
        const FVector DelayedFeet[2] = {View->GetM01SurveyorFootWorld(0), View->GetM01SurveyorFootWorld(1)};
        const int32 DelayedEmergency = View->GetM01SurveyorEmergencyReplantCount();
        const int32 DelayedResets = View->GetM01SurveyorPoseResetCount();
        const int32 DelayedStarts = View->GetM01SurveyorPlannedSwingStartCount();
        View->SetAuthoritativeWorldLocation(View->GetActorLocation()); View->Tick(1.0f / 60.0f);
        TestTrue(TEXT("Delayed J1 target edge preserves both foot anchors"),
            View->GetM01SurveyorFootWorld(0).Equals(DelayedFeet[0], .01f) &&
            View->GetM01SurveyorFootWorld(1).Equals(DelayedFeet[1], .01f));
        TestEqual(TEXT("Delayed J1 target edge has no emergency replant"), View->GetM01SurveyorEmergencyReplantCount(), DelayedEmergency);
        TestEqual(TEXT("Delayed J1 target edge has no pose reset"), View->GetM01SurveyorPoseResetCount(), DelayedResets);
        const FVector BeforeWithdrawStart[2] = {View->GetM01SurveyorFootSwingStartWorld(0), View->GetM01SurveyorFootSwingStartWorld(1)};
        State.position = Bridge->WorldToSim(J1Withdraw[2]); View->ApplyAuthoritativeState(State, false);
        View->Tick(1.0f / 60.0f);
        TestEqual(TEXT("First visible J1 withdrawal starts exactly one late swing"),
            View->GetM01SurveyorPlannedSwingStartCount(), DelayedStarts + 1);
        const int32 WithdrawSwingSide = !View->GetM01SurveyorFootSwingStartWorld(0).Equals(BeforeWithdrawStart[0], .01f) ? 0 : 1;
        TestTrue(TEXT("First visible J1 withdrawal enters C0"),
            View->GetM01SurveyorFootWorld(WithdrawSwingSide).Equals(View->GetM01SurveyorFootSwingStartWorld(WithdrawSwingSide), .01f));
        TestTrue(TEXT("First visible J1 withdrawal retains opposite anchor"),
            View->GetM01SurveyorFootWorld(1 - WithdrawSwingSide).Equals(DelayedFeet[1 - WithdrawSwingSide], .01f));
        for (int32 Frame = 0; Frame < 2; ++Frame) View->Tick(1.0f / 60.0f);
        TestEqual(TEXT("Delayed J1 withdrawal does not duplicate its rephase"),
            View->GetM01SurveyorPlannedSwingStartCount(), DelayedStarts + 1);
        for (int32 Index = 3; Index < J1Withdraw.Num(); ++Index)
            ReplayJ1(TArray<FVector>{J1Withdraw[Index]}, echoes::sim::OrderType::Deliver, TEXT("withdraw"));
        ReplayJ1(J1WithdrawReturnBridge, echoes::sim::OrderType::Gather, TEXT("withdraw-return-bridge"));
        ReplayJ1(J1Return, echoes::sim::OrderType::Gather, TEXT("return"));
        View->ApplyAuthoritativeState(State, false);
        for (int32 Frame = 0; Frame < 18; ++Frame) View->Tick(1.0f / 60.0f);
        // Change the visible heading while reduced motion is enabled. Its
        // steady pose must re-anchor the shared torso/leg pelvis at the
        // current visible heading, rather than freezing the pre-RM yaw.
        State.harvestTicks = 0; State.harvestSlotHeld = false; State.harvestState = echoes::sim::HarvestState::Idle; State.cargo = 12;
        State.order.type = echoes::sim::OrderType::Deliver;
        const float HeadingBeforeReducedMotionTurn = View->GetHeadingYaw();
        // The final return settles at J1FinalWithdraw[0]; select [1] here so
        // this is a real changed heading rather than a stationary RM sample.
        State.position = Bridge->WorldToSim(J1FinalWithdraw[1]);
        View->ApplyAuthoritativeState(State, false);
        TestTrue(TEXT("Reduced-motion fixture receives a real changed heading"),
            FMath::Abs(FMath::FindDeltaAngleDegrees(
                HeadingBeforeReducedMotionTurn, View->GetM01SurveyorTargetHeadingYaw())) > 10.0f);
        if (Settings) Settings->SetReducedMotionEnabled(true);
        View->Tick(.05f);
        if (LegRoot)
        {
            TestTrue(TEXT("Reduced-motion heading change synchronizes torso and leg pelvis"),
                FMath::IsNearlyZero(FMath::FindDeltaAngleDegrees(
                    Body->GetRelativeRotation().Yaw, LegRoot->GetRelativeRotation().Yaw), .01f));
            TestTrue(TEXT("Reduced-motion pelvis follows the current visible heading"),
                FMath::IsNearlyZero(FMath::FindDeltaAngleDegrees(
                    Body->GetRelativeRotation().Yaw, View->GetHeadingYaw()), .01f));
        }
        if (Settings) Settings->SetReducedMotionEnabled(false);
        View->Tick(.05f);
        // The post-RM first departure has no inherited route direction. It
        // must begin one C0 late swing with its support anchored and no new
        // reset after the accessibility transition itself.
        const int32 PostRMResets = View->GetM01SurveyorPoseResetCount();
        const int32 PostRMStarts = View->GetM01SurveyorPlannedSwingStartCount();
        // Keep the already active [1] authoritative state. Reapplying it as
        // a synthetic zero-distance snapshot would report a stop and settle
        // the support that this continued-motion check is meant to preserve.
        View->SetAuthoritativeWorldLocation(View->GetActorLocation()); View->Tick(1.0f / 60.0f);
        const FVector PostRMFeet[2] = {View->GetM01SurveyorFootWorld(0), View->GetM01SurveyorFootWorld(1)};
        State.position = Bridge->WorldToSim(J1FinalWithdraw[2]); View->ApplyAuthoritativeState(State, false);
        View->Tick(1.0f / 60.0f);
        TestEqual(TEXT("Post-RM first departure has no additional pose reset"), View->GetM01SurveyorPoseResetCount(), PostRMResets);
        TestEqual(TEXT("Post-RM first departure starts one C0 late swing"), View->GetM01SurveyorPlannedSwingStartCount(), PostRMStarts + 1);
        const int32 PostRMSwingSide = View->IsM01SurveyorFootSwinging(0) ? 0 : 1;
        TestTrue(TEXT("Post-RM first departure enters C0"),
            View->GetM01SurveyorFootWorld(PostRMSwingSide).Equals(View->GetM01SurveyorFootSwingStartWorld(PostRMSwingSide), .01f));
        TestTrue(TEXT("Post-RM first departure preserves opposite support"),
            View->GetM01SurveyorFootWorld(1 - PostRMSwingSide).Equals(PostRMFeet[1 - PostRMSwingSide], .01f));
        for (int32 Index = 3; Index < J1FinalWithdraw.Num(); ++Index)
            ReplayJ1(TArray<FVector>{J1FinalWithdraw[Index]}, echoes::sim::OrderType::Deliver, TEXT("post-rm-withdraw"));
        // Once the exchanged supports clear, the bounded pelvis must catch
        // the continuously eased torso heading; a hold may delay it but never
        // freeze it permanently.
        for (int32 Frame = 0; Frame < 60; ++Frame) View->Tick(1.0f / 60.0f);
        if (LegRoot)
        {
            TestTrue(TEXT("Post-exchange pelvis converges to the visible heading"),
                FMath::IsNearlyZero(FMath::FindDeltaAngleDegrees(
                    Body->GetRelativeRotation().Yaw, View->GetHeadingYaw()), .01f));
        }
        TestEqual(TEXT("Observed J1 diagonal gather/delivery turns use no emergency replant"),
            View->GetM01SurveyorEmergencyReplantCount(), EmergencyBeforeJ1);
        View->ApplyAuthoritativeState(State, false);
        for (int32 Frame = 0; Frame < 100; ++Frame) View->Tick(.02f);
        const FVector Stopped[2] = {Feet[0]->GetComponentLocation(), Feet[1]->GetComponentLocation()};
        View->Tick(.1f);
        TestTrue(TEXT("Stopped feet settle without indefinite drift"), Feet[0]->GetComponentLocation().Equals(Stopped[0],.1) && Feet[1]->GetComponentLocation().Equals(Stopped[1],.1));

        // Authoritative updates arrive at 20 Hz while the view runs at 60 Hz.
        // Four sharp ordinary reversals must retarget only the airborne foot;
        // its opposite support remains planted and the IK safety replant stays
        // unused. These are presentation snapshots, never simulation writes.
        const int32 EmergencyBeforeShuttle = View->GetM01SurveyorEmergencyReplantCount();
        const int32 ResetBeforeShuttle = View->GetM01SurveyorPoseResetCount();
        const int32 SwingStartsBeforeShuttle = View->GetM01SurveyorPlannedSwingStartCount();
        const int32 LandingsBeforeShuttle = View->GetM01SurveyorPlannedLandingCount();
        float ShuttleStanceDrift = 0.0f;
        int32 ShuttleStancePairs = 0;
        auto RigContext = [&](int32 Side)
        {
            return FString::Printf(
                TEXT("side=%d phase=%.4f swing=%d root=%s foot=%s start=%s target=%s heading=%.2f plannedHeading=%.2f emergency=%d resets=%d"),
                Side, View->GetM01SurveyorGaitPhase(),
                View->IsM01SurveyorFootSwinging(Side) ? 1 : 0,
                *View->GetActorLocation().ToString(),
                *View->GetM01SurveyorFootWorld(Side).ToString(),
                *View->GetM01SurveyorFootSwingStartWorld(Side).ToString(),
                *View->GetM01SurveyorFootSwingEndWorld(Side).ToString(),
                View->GetHeadingYaw(), View->GetM01SurveyorTargetHeadingYaw(),
                View->GetM01SurveyorEmergencyReplantCount(),
                View->GetM01SurveyorPoseResetCount());
        };
        auto RunShuttleLeg = [&](int32 RawDelta, bool bExpectRestartEntry)
        {
            FVector PriorFoot[2] = {
                View->GetM01SurveyorFootWorld(0), View->GetM01SurveyorFootWorld(1)};
            bool PriorSwing[2] = {
                View->IsM01SurveyorFootSwinging(0), View->IsM01SurveyorFootSwinging(1)};
            FVector PriorSwingStart[2] = {
                View->GetM01SurveyorFootSwingStartWorld(0), View->GetM01SurveyorFootSwingStartWorld(1)};
            FVector PriorRoot = View->GetActorLocation();
            int32 RestartSwingSide = INDEX_NONE;
            FVector RestartEntryRoot = FVector::ZeroVector;
            FVector RestartSupportAnchor = FVector::ZeroVector;
            float RestartMaxFrameTravel = 0.0f;
            bool bRestartEntryObserved = false;
            bool bRestartLandingObserved = false;
            bool bRestartLiftObserved = false;
            for (int32 Snapshot = 0; Snapshot < 16; ++Snapshot)
            {
                State.position = echoes::sim::Vec2::FromRaw(
                    State.position.x.Raw() + RawDelta, State.position.y.Raw());
                View->ApplyAuthoritativeState(State, false);
                for (int32 Frame = 0; Frame < 3; ++Frame)
                {
                    View->Tick(1.0f / 60.0f);
                    const FVector Root = View->GetActorLocation();
                    for (int32 Side = 0; Side < 2; ++Side)
                    {
                        const FVector Foot = View->GetM01SurveyorFootWorld(Side);
                        const bool bSwing = View->IsM01SurveyorFootSwinging(Side);
                        const float FrameTravel = FVector::Dist2D(Root, PriorRoot);
                        RestartMaxFrameTravel = FMath::Max(RestartMaxFrameTravel, FrameTravel);
                        const FVector SwingStart = View->GetM01SurveyorFootSwingStartWorld(Side);
                        // A restart enters from stance; an uninterrupted
                        // reversal retargets its already airborne foot. Both
                        // establish a new C0 start, but only the latter keeps
                        // bSwing true across the single view update.
                        const bool bNewSwing = !PriorSwing[Side] && bSwing;
                        const bool bRetargetedAirSwing = PriorSwing[Side] && bSwing &&
                            !SwingStart.Equals(PriorSwingStart[Side], .01f) && Foot.Equals(SwingStart, .01f);
                        if (bExpectRestartEntry && RestartSwingSide == INDEX_NONE && (bNewSwing || bRetargetedAirSwing))
                        {
                            RestartSwingSide = Side;
                            bRestartEntryObserved = true;
                            RestartEntryRoot = Root;
                            RestartSupportAnchor = PriorFoot[1 - Side];
                            TestTrue(FString::Printf(TEXT("Late swing enters C0 at its retained world start (%s)"),
                                *RigContext(Side)), Foot.Equals(SwingStart, .01f));
                            TestTrue(FString::Printf(TEXT("Late swing keeps the opposite support anchor exact (%s)"),
                                *RigContext(1 - Side)),
                                View->GetM01SurveyorFootWorld(1 - Side).Equals(RestartSupportAnchor, .01f));
                        }
                        if (RestartSwingSide != INDEX_NONE)
                        {
                            const float SinceEntry = FVector::Dist2D(Root, RestartEntryRoot);
                            const float Allowance = RestartMaxFrameTravel + .1f;
                            if (Side == RestartSwingSide && !bRestartLandingObserved && !bSwing && PriorSwing[Side])
                            {
                                bRestartLandingObserved = true;
                                TestTrue(FString::Printf(TEXT("Restart selected foot lands within 32 cm plus frame allowance (%.3f %s)"),
                                    SinceEntry, *RigContext(Side)), SinceEntry <= 32.0f + Allowance);
                            }
                            if (Side != RestartSwingSide && !bRestartLiftObserved && bSwing && !PriorSwing[Side])
                            {
                                bRestartLiftObserved = true;
                                TestTrue(FString::Printf(TEXT("Restart opposite foot lifts within 48 cm plus frame allowance (%.3f %s)"),
                                    SinceEntry, *RigContext(Side)), SinceEntry <= 48.0f + Allowance);
                            }
                        }
                        if (!bSwing && !PriorSwing[Side] &&
                            !FMath::IsNearlyZero(FVector::Dist2D(Root, PriorRoot), .01f))
                        {
                            ++ShuttleStancePairs;
                            const float Drift = FVector::Dist2D(Foot, PriorFoot[Side]);
                            ShuttleStanceDrift = FMath::Max(ShuttleStanceDrift, Drift);
                            TestTrue(FString::Printf(TEXT("Planted shuttle foot stays exact (drift=%.3f %s)"),
                                Drift, *RigContext(Side)), Drift <= 1.0f);
                        }
                        PriorFoot[Side] = Foot;
                        PriorSwing[Side] = bSwing;
                        PriorSwingStart[Side] = SwingStart;
                        if (Lower[Side])
                        {
                            // The lower mesh starts at the knee and is authored
                            // 46 cm along +X to its ankle. Check that endpoint,
                            // not the unrelated hip-to-sole distance.
                            const FVector Ankle = Lower[Side]->GetComponentTransform().TransformPosition(
                                FVector(46.0f, 0.0f, 0.0f));
                            // The authored ankle fitting is +10 source cm, then
                            // the component's runtime 1.5 scale is applied.
                            const FVector ExpectedAnkle = Feet[Side]->GetComponentTransform().TransformPosition(
                                FVector(0.0f, 0.0f, 10.0f));
                            const float EndpointError = FVector::Dist(Ankle, ExpectedAnkle);
                            TestTrue(FString::Printf(TEXT("Shin reaches its actual ankle endpoint (error=%.3f %s)"),
                                EndpointError, *RigContext(Side)), EndpointError <= 1.0f);
                        }
                    }
                    PriorRoot = Root;
                }
            }
            if (bExpectRestartEntry)
            {
                TestTrue(TEXT("Late-swing fixture observed its selected entry"), bRestartEntryObserved);
                TestTrue(TEXT("Late-swing fixture observed selected-foot landing"), bRestartLandingObserved);
                TestTrue(TEXT("Late-swing fixture observed opposite-foot lift"), bRestartLiftObserved);
            }
        };
        RunShuttleLeg(92, false);
        // No stop between these legs: exercises the current-air-foot reversal
        // rephase independently of the restart planner.
        RunShuttleLeg(-92, true);
        View->ApplyAuthoritativeState(State, false);
        for (int32 Frame = 0; Frame < 24; ++Frame) View->Tick(1.0f / 60.0f);
        for (int32 Reversal = 0; Reversal < 4; ++Reversal)
        {
            RunShuttleLeg(Reversal % 2 == 0 ? -92 : 92, true);
            // Stationary authoritative snapshots exercise stop/work settling
            // without altering the worker's command or resource state.
            View->ApplyAuthoritativeState(State, false);
            for (int32 Frame = 0; Frame < 24; ++Frame) View->Tick(1.0f / 60.0f);
        }
        // Explicit restart fixtures: one after a complete work settle in the
        // same direction, then one after a partial settle in the reverse
        // direction. Both retain their existing anchors until the selected
        // late swing begins from its exact current pose.
        View->ApplyAuthoritativeState(State, false);
        for (int32 Frame = 0; Frame < 24; ++Frame) View->Tick(1.0f / 60.0f);
        RunShuttleLeg(92, true);
        View->ApplyAuthoritativeState(State, false);
        for (int32 Frame = 0; Frame < 5; ++Frame) View->Tick(1.0f / 60.0f);
        RunShuttleLeg(-92, true);

        // An active authoritative movement update may precede visible root
        // interpolation. It must retain the current phase and not open a new
        // restart swing on that zero-distance view tick.
        State.position = echoes::sim::Vec2::FromRaw(State.position.x.Raw() + 3, State.position.y.Raw());
        View->ApplyAuthoritativeState(State, false);
        const float ZeroDisplayPhase = View->GetM01SurveyorGaitPhase();
        const int32 ZeroDisplayStarts = View->GetM01SurveyorPlannedSwingStartCount();
        const FVector ZeroDisplayFeet[2] = {
            View->GetM01SurveyorFootWorld(0), View->GetM01SurveyorFootWorld(1)};
        View->SetAuthoritativeWorldLocation(View->GetActorLocation());
        View->Tick(1.0f / 60.0f);
        TestTrue(TEXT("Active zero-display frame retains gait phase"),
            FMath::IsNearlyEqual(View->GetM01SurveyorGaitPhase(), ZeroDisplayPhase, .0001f));
        TestEqual(TEXT("Active zero-display frame does not start a restart arc"),
            View->GetM01SurveyorPlannedSwingStartCount(), ZeroDisplayStarts);
        TestTrue(TEXT("Active zero-display frame preserves both world foot anchors"),
            View->GetM01SurveyorFootWorld(0).Equals(ZeroDisplayFeet[0], .01f) &&
            View->GetM01SurveyorFootWorld(1).Equals(ZeroDisplayFeet[1], .01f));
        // Resume actual displayed movement before any stop. A retained active
        // state must advance normally, never consume a false restart edge.
        State.position = echoes::sim::Vec2::FromRaw(State.position.x.Raw() + 3, State.position.y.Raw());
        View->ApplyAuthoritativeState(State, false);
        const int32 ResumeStarts = View->GetM01SurveyorPlannedSwingStartCount();
        View->Tick(1.0f / 60.0f);
        TestEqual(TEXT("Resumed active movement does not false-restart or rephase"),
            View->GetM01SurveyorPlannedSwingStartCount(), ResumeStarts);

        // Stop while the selected restart foot is still airborne. First
        // establish a completed stop, then restart and halt during its first
        // late swing. The final authoritative stop must complete the grounded
        // .28 second settle rather than leaving the arc suspended at an old
        // world endpoint.
        View->ApplyAuthoritativeState(State, false);
        for (int32 Frame = 0; Frame < 18; ++Frame) View->Tick(1.0f / 60.0f);
        State.position = echoes::sim::Vec2::FromRaw(State.position.x.Raw() + 92, State.position.y.Raw());
        View->ApplyAuthoritativeState(State, false);
        for (int32 Frame = 0; Frame < 3; ++Frame) View->Tick(1.0f / 60.0f);
        TestTrue(TEXT("Restart leg is airborne before its authoritative stop"),
            View->IsM01SurveyorFootSwinging(0) || View->IsM01SurveyorFootSwinging(1));
        View->ApplyAuthoritativeState(State, false);
        for (int32 Frame = 0; Frame < 18; ++Frame) View->Tick(1.0f / 60.0f);
        TestFalse(TEXT("Authoritative midair stop clears left swing state after settle"), View->IsM01SurveyorFootSwinging(0));
        TestFalse(TEXT("Authoritative midair stop clears right swing state after settle"), View->IsM01SurveyorFootSwinging(1));
        TestTrue(TEXT("Authoritative midair stop returns soles to the ground envelope"),
            FMath::IsNearlyEqual(Feet[0]->GetComponentLocation().Z - View->GetActorLocation().Z, 3.0f, .2f) &&
            FMath::IsNearlyEqual(Feet[1]->GetComponentLocation().Z - View->GetActorLocation().Z, 3.0f, .2f));

        TestTrue(TEXT("Ordinary reversal shuttle preserves many planted world-space samples"), ShuttleStancePairs > 40);
        TestTrue(FString::Printf(TEXT("Ordinary reversal stance drift is bounded (%.3f cm)"), ShuttleStanceDrift), ShuttleStanceDrift <= 1.0f);
        TestEqual(TEXT("Ordinary reversal shuttle uses no emergency replants"),
            View->GetM01SurveyorEmergencyReplantCount(), EmergencyBeforeShuttle);
        TestEqual(TEXT("Ordinary reversal shuttle does not reset its pose"),
            View->GetM01SurveyorPoseResetCount(), ResetBeforeShuttle);
        TestTrue(TEXT("Five-leg shuttle starts planned swing arcs"),
            View->GetM01SurveyorPlannedSwingStartCount() - SwingStartsBeforeShuttle >= 5);
        TestTrue(TEXT("Five-leg shuttle completes planned swing landings"),
            View->GetM01SurveyorPlannedLandingCount() - LandingsBeforeShuttle >= 5);
        if (Settings)
        {
            Settings->SetReducedMotionEnabled(true); View->Tick(.1f);
            const FTransform Reduced = Feet[0]->GetRelativeTransform();
            View->Tick(.25f);
            TestTrue(TEXT("Reduced motion retains neutral, steady leg geometry"), Feet[0]->GetRelativeTransform().Equals(Reduced,.01));
            TestTrue(TEXT("Reduced motion removes body bob"), Body->GetRelativeLocation().IsNearlyZero(.01));
        }
        if (Settings) Settings->SetReducedMotionEnabled(false);
        View->Tick(.02f);
        const int32 DiscontinuityBefore = View->GetM01SurveyorDiscontinuityPoseResetCount();
        State.position = echoes::sim::Vec2::FromRaw(State.position.x.Raw() + echoes::sim::kFixedScale,
            State.position.y.Raw());
        View->ApplyAuthoritativeState(State, false); View->Tick(.05f);
        TestTrue(TEXT("A large non-teleport update resets both sole anchors near the root"),
            FVector::Dist2D(Feet[0]->GetComponentLocation(), View->GetActorLocation()) < 80 &&
            FVector::Dist2D(Feet[1]->GetComponentLocation(), View->GetActorLocation()) < 80);
        TestEqual(TEXT("Large non-teleport jump is separately classified as a discontinuity"),
            View->GetM01SurveyorDiscontinuityPoseResetCount(), DiscontinuityBefore + 1);
        View->ApplyAuthoritativeState(State, true); View->Tick(.02f);
        State.position = echoes::sim::Vec2::FromRaw(State.position.x.Raw() + 3, State.position.y.Raw());
        View->ApplyAuthoritativeState(State, false); View->Tick(.016f);
        for (int32 Frame = 0; Frame < 40; ++Frame) View->Tick(.001f);
        TestTrue(TEXT("Slow authoritative movement retains load transfer between snapshots"),
            Body->GetRelativeLocation().Z > .001f);
        State.position = echoes::sim::Vec2::FromTiles(18,14);
        View->ApplyAuthoritativeState(State, true); View->Tick(.02f);
        TestTrue(TEXT("Teleport resets foot anchors near the new root"), FVector::Dist2D(Feet[0]->GetComponentLocation(), View->GetActorLocation()) < 80);
        // SPEC-MOV-010 requires actual angular progress, not just an upper
        // bound that would also pass a frozen or exponentially slowed torso.
        if (const auto* TurnSource = Bridge->FindEntity(6))
        {
            auto* TurnView = World->SpawnActor<AEchoesEntityView>();
            TestNotNull(TEXT("M01 turn-rate fixture spawns a separate view"), TurnView);
            if (TurnView)
            {
                auto TurnState = *TurnSource;
                TurnState.id = 990042;
                TurnView->ActivateForEntity(TurnState, true);
                TurnView->SetAuthoritativeHeadingYaw(0.0f);
                TurnView->Tick(.02f);
                const int32 TurnResets = TurnView->GetM01SurveyorPoseResetCount();
                const int32 TurnEmergency = TurnView->GetM01SurveyorEmergencyReplantCount();
                TurnState.position = Bridge->WorldToSim(
                    TurnView->GetActorLocation() + FVector(0, 40, 0));
                TurnView->ApplyAuthoritativeState(TurnState, false);
                TurnView->Tick(.02f);
                TestTrue(TEXT("M01 unconstrained heading advances at 720 degrees per second"),
                    FMath::IsNearlyEqual(TurnView->GetHeadingYaw(), 14.4f, .05f));
                TArray<UStaticMeshComponent*> TurnComponents;
                TurnView->GetComponents(TurnComponents);
                bool bCheckedRenderedTurn = false;
                for (auto* Component : TurnComponents)
                {
                    if (Component->GetName() == TEXT("BodyMesh"))
                    {
                        bCheckedRenderedTurn = true;
                        TestTrue(TEXT("M01 unconstrained rendered torso advances at the authored rate"),
                            FMath::IsNearlyEqual(Component->GetRelativeRotation().Yaw, 14.4f, .05f));
                    }
                }
                TestTrue(TEXT("M01 turn-rate check observes the rendered body"), bCheckedRenderedTurn);
                TestEqual(TEXT("M01 authored-rate turn does not reset the pose"),
                    TurnView->GetM01SurveyorPoseResetCount(), TurnResets);
                TestEqual(TEXT("M01 authored-rate turn keeps reachable contacts"),
                    TurnView->GetM01SurveyorEmergencyReplantCount(), TurnEmergency);
                TurnView->Destroy();
            }
        }

        // Durable ordinary-route regression: issue the actual campaign Gather
        // command to worker 6 and resource 24, then drive authoritative
        // simulation at 20 Hz while its real view receives both ordinary 60
        // Hz presentation ticks and bounded 30 Hz frames. This is deliberately
        // not a displayed-coordinate fixture.
        const echoes::sim::Entity* LiveWorker = Bridge->FindEntity(6);
        const echoes::sim::Entity* LiveResource = Bridge->FindEntity(24);
        AEchoesEntityView* LiveWorkerView = Bridge->FindEntityView(6);
        TestTrue(TEXT("M01 live Gather regression finds worker 6"), LiveWorker &&
            LiveWorker->type == echoes::sim::EntityType::Worker);
        TestTrue(TEXT("M01 live Gather regression finds resource 24"), LiveResource &&
            LiveResource->type == echoes::sim::EntityType::ResourceNode);
        TestTrue(TEXT("M01 live Gather regression finds worker 6 presentation"), LiveWorkerView != nullptr);
        if (LiveWorker && LiveResource && LiveWorkerView)
        {
            const uint32 LiveOwner = LiveWorker->owner;
            Feedback.Reset();
            Bridge->SetScenarioPaused(false);
            TestTrue(TEXT("M01 worker 6 accepts Gather on resource 24"),
                Bridge->IssueCommand(echoes::sim::CommandType::Gather, 6, 24,
                    Bridge->SimToWorld(LiveResource->position),
                    echoes::sim::FutureWellChoice::Dormant, Feedback));
            const int32 LiveEmergencyBefore = LiveWorkerView->GetM01SurveyorEmergencyReplantCount();
            const int32 LiveDiscontinuitiesBefore = LiveWorkerView->GetM01SurveyorDiscontinuityPoseResetCount();
            const int32 LiveStartsBefore = LiveWorkerView->GetM01SurveyorPlannedSwingStartCount();
            const echoes::sim::Simulation* LiveSimulation = Bridge->GetSimulation();
            const uint64 LiveTickBefore = LiveSimulation ? LiveSimulation->CurrentTick() : 0;
            const echoes::sim::PlayerState* LivePlayer = LiveSimulation
                ? LiveSimulation->FindPlayer(LiveOwner) : nullptr;
            const int32 LiveMatterBefore = LivePlayer ? LivePlayer->resources.material : 0;
            bool bObservedGather = false;
            bool bObservedDelivery = false;
            bool bLiveRouteValid = LiveSimulation != nullptr && LivePlayer != nullptr;
            int32 PreviousCargo = LiveWorker->cargo;
            bool bPreviousDelivery = LiveWorker->order.type == echoes::sim::OrderType::Deliver;
            int32 CompletedDeliveryTransitions = 0;
            float LiveMaxStanceDrift = 0.0f;
            int32 LiveStancePairs = 0;
            const auto TickLiveView = [&](float DeltaSeconds)
            {
                const FVector RootBefore = LiveWorkerView->GetActorLocation();
                const FVector FeetBefore[2] = {LiveWorkerView->GetM01SurveyorFootWorld(0),
                    LiveWorkerView->GetM01SurveyorFootWorld(1)};
                const bool SwingBefore[2] = {LiveWorkerView->IsM01SurveyorFootSwinging(0),
                    LiveWorkerView->IsM01SurveyorFootSwinging(1)};
                const int32 ResetsBefore = LiveWorkerView->GetM01SurveyorPoseResetCount();
                const int32 EmergencyBefore = LiveWorkerView->GetM01SurveyorEmergencyReplantCount();
                LiveWorkerView->Tick(DeltaSeconds);
                if (LiveWorkerView->IsLocomotionMotionActive() &&
                    FVector::Dist2D(RootBefore, LiveWorkerView->GetActorLocation()) > .01f &&
                    LiveWorkerView->GetM01SurveyorPoseResetCount() == ResetsBefore &&
                    LiveWorkerView->GetM01SurveyorEmergencyReplantCount() == EmergencyBefore)
                {
                    for (int32 Side = 0; Side < 2; ++Side)
                    {
                        if (!SwingBefore[Side] && !LiveWorkerView->IsM01SurveyorFootSwinging(Side))
                        {
                            ++LiveStancePairs;
                            LiveMaxStanceDrift = FMath::Max(LiveMaxStanceDrift,
                                static_cast<float>(FVector::Dist2D(FeetBefore[Side],
                                    LiveWorkerView->GetM01SurveyorFootWorld(Side))));
                        }
                    }
                }
            };
            for (int32 SimulationTick = 0; SimulationTick < 1800; ++SimulationTick)
            {
                Bridge->Tick(.05f);
                const echoes::sim::Entity* LiveState = Bridge->FindEntity(6);
                AEchoesEntityView* CurrentLiveWorkerView = Bridge->FindEntityView(6);
                if (!LiveState || !CurrentLiveWorkerView)
                {
                    AddError(TEXT("M01 live Gather route lost its worker or presentation view"));
                    bLiveRouteValid = false;
                    break;
                }
                LiveWorkerView = CurrentLiveWorkerView;
                bObservedGather |= LiveState->order.type == echoes::sim::OrderType::Gather;
                bObservedDelivery |= LiveState->order.type == echoes::sim::OrderType::Deliver;
                if (bPreviousDelivery && PreviousCargo > 0 && LiveState->cargo == 0)
                    ++CompletedDeliveryTransitions;
                PreviousCargo = LiveState->cargo;
                bPreviousDelivery = LiveState->order.type == echoes::sim::OrderType::Deliver;
                if ((SimulationTick & 1) == 0)
                {
                    TickLiveView(1.0f / 30.0f);
                    TickLiveView(1.0f / 60.0f);
                }
                else
                {
                    for (int32 ViewTick = 0; ViewTick < 3; ++ViewTick)
                        TickLiveView(1.0f / 60.0f);
                }
            }
            LiveSimulation = Bridge->GetSimulation();
            const uint64 LiveTickAfter = LiveSimulation ? LiveSimulation->CurrentTick() : 0;
            LivePlayer = LiveSimulation ? LiveSimulation->FindPlayer(LiveOwner) : nullptr;
            const int32 LiveMatterAfter = LivePlayer ? LivePlayer->resources.material : LiveMatterBefore;
            TestTrue(TEXT("M01 live Gather route retained a valid worker and view"), bLiveRouteValid);
            TestTrue(TEXT("M01 live Gather route advances a full ninety authoritative seconds"),
                LiveTickAfter >= LiveTickBefore + 1800);
            TestTrue(TEXT("M01 live Gather route exposes actual Gather state"), bObservedGather);
            TestTrue(TEXT("M01 live Gather route reaches delivery after collection"), bObservedDelivery);
            TestTrue(TEXT("M01 live Gather route completes repeated cargo deliveries"),
                CompletedDeliveryTransitions >= 3);
            TestTrue(TEXT("M01 live Gather route credits controlled local Matter"),
                LiveMatterAfter >= LiveMatterBefore + 30);
            if (bLiveRouteValid)
            {
                TestTrue(TEXT("M01 live Gather route exercises repeated normal gait boundaries"),
                    LiveWorkerView->GetM01SurveyorPlannedSwingStartCount() >= LiveStartsBefore + 8);
                TestEqual(TEXT("M01 live Gather/delivery route uses no emergency replant"),
                    LiveWorkerView->GetM01SurveyorEmergencyReplantCount(), LiveEmergencyBefore);
                TestEqual(TEXT("M01 live Gather/delivery route uses no discontinuity reset"),
                    LiveWorkerView->GetM01SurveyorDiscontinuityPoseResetCount(), LiveDiscontinuitiesBefore);
                TestTrue(TEXT("M01 live route exercises planted support during motion"), LiveStancePairs > 100);
                TestTrue(TEXT("M01 live route keeps planted support within one centimetre"), LiveMaxStanceDrift <= 1.0f);
            }
        }
        View->PrepareForPool();
        TestFalse(TEXT("Pooled legs are hidden"), Feet[0]->IsVisible());
        TestNull(TEXT("Pooled legs release mesh and material"), Feet[0]->GetStaticMesh());
        TestNull(TEXT("Pooled leg MID is cleared"), Feet[0]->GetMaterial(0));
        State.type = echoes::sim::EntityType::Soldier;
        View->ActivateForEntity(State, true); View->Tick(.02f);
        TestFalse(TEXT("A reused Lancer view has no stale Surveyor limb"), Feet[0]->IsVisible());
        TestTrue(TEXT("Lancer remains the approved combined mesh"), Body->GetStaticMesh() && Body->GetStaticMesh()->GetName() == TEXT("SM_Meridian_Lancer"));
    }
    View->PrepareForPool();
    if (Bridge->SelectOperationMode(EEchoesOperationMode::Skirmish, Feedback))
    {
        State.type = echoes::sim::EntityType::Worker;
        View->ActivateForEntity(State, true); View->Tick(.02f);
        TestTrue(TEXT("Non-M01 Surveyor keeps its original complete mesh"), Body && Body->GetStaticMesh() &&
            Body->GetStaticMesh()->GetName() == TEXT("SM_Meridian_Surveyor"));
        if (Feet[0]) TestFalse(TEXT("Non-M01 Surveyor cannot retain M01 limbs"), Feet[0]->IsVisible());
    }
    else AddError(TEXT("Cannot establish non-M01 control scenario"));
    View->Destroy();
    if (Settings) Settings->SetReducedMotionEnabled(PriorMotion);
    Bridge->StopPrototypeScenario();
    AddInfo(TEXT("Synthetic authoritative presentation snapshots; retained ordinary runtime gait remains a separate EDT/PKG check."));
    return true;
}
#endif
