// Author and owner: Angelis Pseftis
// M01 presentation rig. All endpoints follow displayed authoritative motion;
// no collision queries, hidden-entity reads, navigation, or simulation writes.
#include "EchoesEntityView.h"
#include "EchoesSimulationSubsystem.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"

void AEchoesEntityView::ResetM01SurveyorRig()
{
    bUsingM01SurveyorRig = bM01RigInitialized = bM01RigMoving = bM01RigLocomotionActive = bM01RestartPending = bM01RigReduced = false;
    M01GaitPhase = 0.0f;
    M01SettleTime = .28f;
    M01PreviousRoot = FVector::ZeroVector;
    M01LastDisplayedTravelForward = FVector::ForwardVector;
    M01EmergencyReplantCount = 0;
    M01PoseResetCount = 0;
    M01DiscontinuityPoseResetCount = 0;
    M01PlannedSwingStartCount = 0;
    M01PlannedLandingCount = 0;
    M01NextRestartRephaseSide = 0;
    bM01HasLastTravelDirection = false;
    bM01TargetTurnRephaseLatched = false;
    M01SurveyorPelvisYaw = 0.0f;
    bM01SurveyorPelvisYawInitialized = false;
    for (auto& Foot : M01Feet) Foot = FM01FootPose{};
    if (M01SurveyorRigRoot) M01SurveyorRigRoot->SetRelativeTransform(FTransform::Identity);
    for (UStaticMeshComponent* Part : M01SurveyorParts)
    {
        Part->SetVisibility(false);
        Part->SetRelativeTransform(FTransform::Identity);
        Part->SetStaticMesh(nullptr);
        for (int32 Slot = 0; Slot < 4; ++Slot) Part->SetMaterial(Slot, nullptr);
    }
}

void AEchoesEntityView::ConfigureM01SurveyorRig()
{
    const bool bEligible = UsesProloguePresentation() &&
        EntityFaction == echoes::sim::Faction::MeridianCompact &&
        EntityType == echoes::sim::EntityType::Worker && bUsingAuthoredRosterMesh;
    if (!bEligible)
    {
        if (bUsingM01SurveyorRig) ResetM01SurveyorRig();
        return;
    }
    UStaticMesh* Meshes[4];
    const TCHAR* Names[] = {TEXT("Body"), TEXT("Upper"), TEXT("Lower"), TEXT("Foot")};
    for (int32 Index = 0; Index < 4; ++Index)
    {
        const FString Name = FString::Printf(TEXT("SM_Meridian_M01Surveyor%s"), Names[Index]);
        Meshes[Index] = LoadObject<UStaticMesh>(nullptr,
            *FString::Printf(TEXT("/Game/Art/Generated/Meridian/Units/%s.%s"), *Name, *Name));
        if (!Meshes[Index])
        {
            // ConfigureAppearance has already bound the complete standard mesh.
            // Never render a torso with missing limbs after a partial asset load.
            ResetM01SurveyorRig();
            return;
        }
    }
    if (!M01SurveyorRigRoot)
    {
        M01SurveyorRigRoot = NewObject<USceneComponent>(this, TEXT("M01SurveyorRigRoot"));
        AddInstanceComponent(M01SurveyorRigRoot);
        M01SurveyorRigRoot->SetupAttachment(BodyPivot);
        M01SurveyorRigRoot->RegisterComponent();
        for (int32 Side = 0; Side < 2; ++Side)
            for (int32 Segment = 0; Segment < 3; ++Segment)
            {
                auto* Part = NewObject<UStaticMeshComponent>(this,
                    *FString::Printf(TEXT("M01Surveyor%s%s"), Side ? TEXT("Right") : TEXT("Left"), Names[Segment + 1]));
                AddInstanceComponent(Part);
                Part->SetupAttachment(M01SurveyorRigRoot);
                Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                Part->SetCollisionResponseToAllChannels(ECR_Ignore);
                Part->SetGenerateOverlapEvents(false);
                Part->SetCanEverAffectNavigation(false);
                Part->SetReceivesDecals(false);
                Part->SetCastShadow(true);
                Part->RegisterComponent();
                M01SurveyorParts.Add(Part);
            }
    }
    BodyMesh->SetStaticMesh(Meshes[0]);
    for (int32 Index = 0; Index < M01SurveyorParts.Num(); ++Index)
    {
        UStaticMeshComponent* Part = M01SurveyorParts[Index];
        Part->SetStaticMesh(Meshes[Index % 3 + 1]);
        for (int32 Slot = 0; Slot < BodyMaterials.Num(); ++Slot) Part->SetMaterial(Slot, BodyMaterials[Slot]);
        Part->SetVisibility(true);
    }
    const bool bWasUsingRig = bUsingM01SurveyorRig;
    bUsingM01SurveyorRig = true;
    // Do not reset an existing gait when selection/damage refreshes appearance.
    if (!bWasUsingRig || !bM01RigInitialized)
        UpdateM01SurveyorRig(0.0f, bMotionReducedMotionApplied);
}

void AEchoesEntityView::UpdateM01SurveyorRig(float DeltaSeconds, bool bReducedMotion)
{
    if (!M01SurveyorRigRoot || M01SurveyorParts.Num() != 6) return;
    constexpr float StrideWorld = 160.0f;
    constexpr float StanceFraction = .60f;
    constexpr float SettleDuration = .14f;
    const FVector Root = GetActorLocation();
    const bool bInitialPose = !bM01RigInitialized;
    if (!bM01SurveyorPelvisYawInitialized || bInitialPose)
    {
        M01SurveyorPelvisYaw = CurrentHeadingYaw;
        bM01SurveyorPelvisYawInitialized = true;
    }
    M01SurveyorRigRoot->SetRelativeRotation(FRotator(0, M01SurveyorPelvisYaw, 0));
    FTransform Rig = M01SurveyorRigRoot->GetComponentTransform();
    const FQuat Facing = FRotator(0, CurrentHeadingYaw, 0).Quaternion();
    // CurrentHeadingYaw intentionally eases the visible torso. Landings use
    // the authoritative-visible target heading so an ordinary sharp reversal
    // never launches a foot in the direction the old torso happened to face.
    const FQuat PlannedFacing = FRotator(0, TargetHeadingYaw, 0).Quaternion();
    const FVector PlannedForward = PlannedFacing.GetForwardVector().GetSafeNormal();
    const FVector PlannedRight = PlannedFacing.GetRightVector().GetSafeNormal();
    const FVector Travel = Root - M01PreviousRoot;
    const float Distance = Travel.Size2D();
    const float DisplaySpeed = DeltaSeconds > UE_SMALL_NUMBER ? Distance / DeltaSeconds : 0;
    // A single normal 20 Hz Surveyor update is a short visible step. A displayed
    // root jump over 64 cm is a separately classified discontinuity;
    // it may reset both anchors. Reversals and ordinary interpolation never do.
    const bool bDiscontinuity = !bInitialPose && Distance >= 64.0f;
    const bool bAccessibilityPoseChange = bReducedMotion != bM01RigReduced;
    const bool bReset = bInitialPose || bDiscontinuity || bAccessibilityPoseChange;
    // A reset and reduced-motion pose are authored steady poses. They use the
    // current visible heading before Neutral() records any ankle anchors, so
    // the torso and leg root cannot remain frozen at a pre-accessibility turn.
    if (bReset || bReducedMotion)
    {
        M01SurveyorPelvisYaw = CurrentHeadingYaw;
        M01SurveyorRigRoot->SetRelativeRotation(FRotator(0, M01SurveyorPelvisYaw, 0));
        Rig = M01SurveyorRigRoot->GetComponentTransform();
    }
    const auto MaxPelvisReach = [&](float CandidateYaw)
    {
        const FTransform Candidate(FRotator(0, CandidateYaw, 0).Quaternion(),
            Rig.GetLocation(), Rig.GetScale3D());
        float MaxReach = 0.0f;
        for (int32 Side = 0; Side < 2; ++Side)
        {
            const FVector Hip(2.0f, Side ? 21.0f : -21.0f, 69.0f);
            const FVector End = Candidate.InverseTransformPosition(M01Feet[Side].World) + FVector(0, 0, 10);
            MaxReach = FMath::Max(MaxReach, (End - Hip).Size());
        }
        return MaxReach;
    };
    const auto Neutral = [&](int32 Side)
    {
        FVector Point = Rig.TransformPosition(FVector(18, Side ? 21 : -21, 0));
        Point.Z = Root.Z + 3.0f; // flush M01 route and apron upper envelope
        return Point;
    };
    if (bReset || bReducedMotion)
    {
        ++M01PoseResetCount;
        // An accessibility restore begins from its authored steady pose.
        // Do not carry the pre-reduced-motion route vector into that new
        // visible departure, but keep ordinary reduced-mode reset counting
        // and discontinuity classification unchanged.
        if (bAccessibilityPoseChange)
        {
            bM01HasLastTravelDirection = false;
            bM01TargetTurnRephaseLatched = false;
        }
        if (bReset)
            bM01TargetTurnRephaseLatched = false;
        if (bDiscontinuity) ++M01DiscontinuityPoseResetCount;
        M01GaitPhase = 0;
        M01SettleTime = SettleDuration * 2;
        bM01RigMoving = false;
        bM01RigLocomotionActive = false;
        bM01RestartPending = false;
        for (int32 Side = 0; Side < 2; ++Side)
        {
            auto& Foot = M01Feet[Side];
            Foot = FM01FootPose{};
            Foot.World = Foot.SwingStart = Foot.SwingEnd = Foot.SettleStart = Neutral(Side);
            Foot.Rotation = Foot.SwingRotation = Facing;
        }
        bM01RigInitialized = true;
    }
    // A new swing begins only after the displayed root has actually moved.
    // Authoritative locomotion can be known before the interpolated root has
    // advanced; an existing arc stays visible across those zero-distance view
    // frames, but a fresh arc never opens there.
    const bool bHasDisplayedTravel = Distance > UE_KINDA_SMALL_NUMBER;
    const bool bMoving = !bReducedMotion && !bReset && bLocomotionActive && bHasDisplayedTravel;
    const bool bHasAirborneFoot = M01Feet[0].bSwing || M01Feet[1].bSwing;
    const bool bStoppedThisFrame = !bLocomotionActive && bM01RigLocomotionActive;
    if (bMoving)
    {
        const FVector DisplayedTravelForward = Travel.GetSafeNormal2D();
        // A restart follows an actual stop/work settle. It may continue in the
        // same direction or reverse; both cases select one late swing while
        // preserving the opposite exact world-space support. A zero-distance
        // active frame does not consume this pending edge.
        // Initial movement can depart the spawn pose on a diagonal before any
        // stop has armed a restart. Give that first visible step the same
        // bounded late-swing treatment as a restart; otherwise an X-aligned
        // planted neutral can outrun its support reach during the first turn.
        const bool bInitialDeparture = !bM01HasLastTravelDirection && !bHasAirborneFoot;
        const bool bRestart = bM01RestartPending && !bHasAirborneFoot;
        const bool bTargetTurnOpposes = bM01HasLastTravelDirection &&
            FVector::DotProduct(PlannedForward, M01LastDisplayedTravelForward) < -0.35f;
        if (!bTargetTurnOpposes)
            bM01TargetTurnRephaseLatched = false;
        // The authority can reverse a gather/delivery target one or more view
        // frames before interpolation changes its displayed travel. Rephase
        // once at that target edge, rather than waiting for a now-stretched
        // planted support to cross the hard visual reach guard.
        const bool bTargetReversal = bTargetTurnOpposes && !bM01TargetTurnRephaseLatched;
        const bool bDisplayedReversal = bM01HasLastTravelDirection &&
            FVector::DotProduct(DisplayedTravelForward, M01LastDisplayedTravelForward) < -0.35f;
        const bool bReversal = (bDisplayedReversal && !bM01TargetTurnRephaseLatched) || bTargetReversal;
        bool bSkipEntryAdvance = false;
        const auto BeginLateRephase = [&](bool bPreferCurrentAirborne)
        {
            const FTransform PlannedRig(PlannedFacing, Rig.GetLocation(), Rig.GetScale3D());
            float SafeSupportTravel[2] = {0.0f, 0.0f};
            constexpr float SoftReach = 83.5f;
            constexpr float SupportMargin = 2.0f;
            const float ScaleX = FMath::Max(FMath::Abs(PlannedRig.GetScale3D().X), .001f);
            for (int32 Side = 0; Side < 2; ++Side)
            {
                const FVector Hip(2.0f, Side ? 21.0f : -21.0f, 69.0f);
                const FVector End = PlannedRig.InverseTransformPosition(M01Feet[Side].World) + FVector(0, 0, 10);
                const FVector Delta = End - Hip;
                const float RadialX = FMath::Sqrt(FMath::Max(0.0f,
                    SoftReach * SoftReach - Delta.Y * Delta.Y - Delta.Z * Delta.Z));
                SafeSupportTravel[Side] = FMath::Clamp(ScaleX * FMath::Max(0.0f,
                    Delta.X + RadialX) - SupportMargin, 0.0f, StrideWorld);
            }
            int32 RephaseSide = INDEX_NONE;
            if (bPreferCurrentAirborne && (M01Feet[0].bSwing != M01Feet[1].bSwing))
                RephaseSide = M01Feet[0].bSwing ? 0 : 1;
            int32 SupportSide = RephaseSide == INDEX_NONE ? 0 : 1 - RephaseSide;
            if (RephaseSide == INDEX_NONE)
            {
                if (FMath::IsNearlyEqual(SafeSupportTravel[0], SafeSupportTravel[1], .01f))
                    SupportSide = 1 - M01NextRestartRephaseSide;
                else
                    SupportSide = SafeSupportTravel[1] > SafeSupportTravel[0] ? 1 : 0;
                RephaseSide = 1 - SupportSide;
            }
            M01NextRestartRephaseSide = 1 - RephaseSide;
            const float SelectedPhase = FMath::Clamp(FMath::Max(.80f,
                1.10f - SafeSupportTravel[SupportSide] / StrideWorld), .80f, .99f);
            M01GaitPhase = RephaseSide == 0 ? SelectedPhase : FMath::Fmod(SelectedPhase + .5f, 1.0f);
            // An uninterrupted reversal retargets the current airborne foot
            // from its exact current world pose; a restart starts from its
            // settled foot. Both paths enter the new late arc C0 below.
            M01Feet[RephaseSide].bSwing = false;
        };
        if (bInitialDeparture || bRestart)
        {
            BeginLateRephase(false);
            bM01RestartPending = false;
            // A stop/restart can coincide with the authority's target turn.
            // Consume that same edge here so the delayed displayed reversal
            // cannot schedule a duplicate C0 rephase on the next view tick.
            if (bTargetReversal)
                bM01TargetTurnRephaseLatched = true;
            bSkipEntryAdvance = true;
        }
        else if (bReversal)
        {
            BeginLateRephase(true);
            if (bTargetReversal)
                bM01TargetTurnRephaseLatched = true;
            bSkipEntryAdvance = true;
        }

        const float GaitPhaseBeforeAdvance = M01GaitPhase;
        if (!bSkipEntryAdvance)
            M01GaitPhase = FMath::Fmod(M01GaitPhase + Distance / StrideWorld, 1.0f);
        M01SettleTime = 0;
        for (int32 Side = 0; Side < 2; ++Side)
        {
            auto& Foot = M01Feet[Side];
            const float PreviousPhase = FMath::Fmod(GaitPhaseBeforeAdvance + Side * .5f, 1.0f);
            const float Phase = FMath::Fmod(M01GaitPhase + Side * .5f, 1.0f);
            const bool bSwing = Phase >= StanceFraction;
            const auto DesiredEnd = [&](const FVector& PlanningRoot, float PlanningPhase)
            {
                // On a straight path Root plus this phase term is constant.
                // Recompute airborne targets so an actual heading update is
                // represented by the visible planned foot, never a stale end.
                FVector End = PlanningRoot + PlannedForward *
                    (StrideWorld * (1.0f - PlanningPhase) + 48.0f) +
                    PlannedRight * (Side ? 31.5f : -31.5f);
                End.Z = PlanningRoot.Z + 3.0f;
                return End;
            };
            if (bSwing && !Foot.bSwing)
            {
                Foot.SwingStart = Foot.World;
                Foot.SwingRotation = Foot.Rotation;
                // A normal gait boundary may have crossed .60 between view
                // samples. Evaluate its elapsed airborne fraction now so the
                // old support does not remain C0 through a later root pose.
                // Explicit restart/reversal entries deliberately set their
                // late phase this frame and retain exact C0 instead.
                Foot.SwingStartPhase = bSkipEntryAdvance ? Phase : StanceFraction;
                Foot.SwingEnd = DesiredEnd(Root, Phase);
                ++M01PlannedSwingStartCount;
            }
            if (bSwing)
            {
                Foot.SwingEnd = DesiredEnd(Root, Phase);
                const float T = FMath::Clamp((Phase - Foot.SwingStartPhase) /
                    FMath::Max(1.0f - Foot.SwingStartPhase, .001f), 0.0f, 1.0f);
                const float Smooth = T * T * (3.0f - 2.0f * T);
                Foot.World = FMath::Lerp(Foot.SwingStart, Foot.SwingEnd, Smooth);
                Foot.World.Z += FMath::Sin(T * PI) * 22.0f;
                Foot.Rotation = FQuat::Slerp(Foot.SwingRotation, PlannedFacing, Smooth);
            }
            else if (Foot.bSwing)
            {
                FVector LandingRoot = Root;
                if (Phase < PreviousPhase)
                    LandingRoot -= PlannedForward * (StrideWorld * Phase);
                Foot.SwingEnd = DesiredEnd(LandingRoot, 1.0f);
                Foot.World = Foot.SwingEnd;
                Foot.Rotation = PlannedFacing;
                ++M01PlannedLandingCount;
            }
            Foot.bSwing = bSwing;
        }
        M01LastDisplayedTravelForward = DisplayedTravelForward;
        bM01HasLastTravelDirection = true;
    }
    else if (!bReducedMotion && !bReset && !bLocomotionActive)
    {
        if (bM01RigMoving || bStoppedThisFrame)
        {
            for (auto& Foot : M01Feet) { Foot.SettleStart = Foot.World; Foot.SwingRotation = Foot.Rotation; }
            M01SettleTime = 0;
        }
        M01SettleTime = FMath::Min(M01SettleTime + DeltaSeconds, SettleDuration * 2);
        for (int32 Side = 0; Side < 2; ++Side)
        {
            auto& Foot = M01Feet[Side];
            const float T = FMath::Clamp((M01SettleTime - Side * SettleDuration) / SettleDuration, 0.0f, 1.0f);
            const float Smooth = T * T * (3.0f - 2.0f * T);
            Foot.World = FMath::Lerp(Foot.SettleStart, Neutral(Side), Smooth);
            Foot.World.Z += FMath::Sin(T * PI) * 8.0f;
            Foot.Rotation = FQuat::Slerp(Foot.SwingRotation, Facing, Smooth);
            Foot.bSwing = false;
        }
    }
    M01PreviousRoot = Root;
    if (bStoppedThisFrame)
        bM01RestartPending = true;
    bM01RigMoving = bMoving;
    bM01RigLocomotionActive = bLocomotionActive;
    bM01RigReduced = bReducedMotion;
    if (!bReset && !bReducedMotion)
    {
        // Select the pelvis heading only after the new foot pose exists. The
        // accepted yaw therefore matches the exact pose about to be rendered,
        // including a lifted endpoint and this frame's root advance. The
        // torso follows the already eased visible heading at a bounded rate;
        // it never snaps through a sharp authoritative gather/deliver turn.
        constexpr float PelvisTurnRateDegreesPerSecond = 720.0f; // SPEC-MOV-010
        const float MaxYawStep = PelvisTurnRateDegreesPerSecond * FMath::Max(DeltaSeconds, 0.0f);
        const float YawToVisibleHeading = FMath::FindDeltaAngleDegrees(M01SurveyorPelvisYaw, CurrentHeadingYaw);
        const float DesiredYaw = M01SurveyorPelvisYaw + FMath::Clamp(YawToVisibleHeading, -MaxYawStep, MaxYawStep);
        const float StartYaw = M01SurveyorPelvisYaw;
        const float StartReach = MaxPelvisReach(StartYaw);
        float BestSoftYaw = StartYaw;
        float BestHardImprovementYaw = StartYaw;
        float BestHardImprovementReach = StartReach;
        bool bFoundSoftYaw = false;
        bool bFoundHardImprovement = false;
        const float YawDelta = FMath::FindDeltaAngleDegrees(StartYaw, DesiredYaw);
        // Even when this frame's root advance has pushed the previously
        // accepted yaw above the soft envelope, inspect the bounded visual
        // interval. A small turn can reduce reach before the strict 87.5 cm
        // emergency-only guard would be needed.
        for (int32 Step = 0; Step <= 12; ++Step)
        {
            const float CandidateYaw = StartYaw + YawDelta * (static_cast<float>(Step) / 12.0f);
            const float CandidateReach = MaxPelvisReach(CandidateYaw);
            if (CandidateReach <= 83.5f)
            {
                // Prefer the furthest safe point toward the visible heading;
                // it remains within this tick's rate cap.
                BestSoftYaw = CandidateYaw;
                bFoundSoftYaw = true;
            }
            else if (CandidateReach < 87.5f && CandidateReach < BestHardImprovementReach)
            {
                BestHardImprovementYaw = CandidateYaw;
                BestHardImprovementReach = CandidateReach;
                bFoundHardImprovement = true;
            }
        }
        M01SurveyorPelvisYaw = bFoundSoftYaw ? BestSoftYaw :
            (bFoundHardImprovement ? BestHardImprovementYaw : StartYaw);
        M01SurveyorRigRoot->SetRelativeRotation(FRotator(0, M01SurveyorPelvisYaw, 0));
        Rig = M01SurveyorRigRoot->GetComponentTransform();
    }
    BodyMesh->SetRelativeScale3D(FVector::OneVector);
    const float Transfer = bMoving ? FMath::Sin(M01GaitPhase * 4.0f * PI) : 0.0f;
    BodyMesh->SetRelativeLocation(FVector(0, 0, FMath::Abs(Transfer) * 1.2f));
    // The body mesh and the articulated hip root must share the same pelvis
    // frame. CurrentHeadingYaw remains the authoritative-visible turn signal;
    // M01SurveyorPelvisYaw is its reach-safe rendered realization.
    BodyMesh->SetRelativeRotation(FRotator(0, M01SurveyorPelvisYaw, Transfer * .8f));

    for (int32 Side = 0; Side < 2; ++Side)
    {
        auto& Foot = M01Feet[Side];
        const FVector Hip(2, Side ? 21 : -21, 69);
        // Foot.World is the sole contact; the ankle fitting is on its upper
        // face. Attaching the shin at the sole would drive its thickness into
        // the terrain despite a numerically grounded foot.
        FVector End = Rig.InverseTransformPosition(Foot.World) + FVector(0, 0, 10);
        FVector Delta = End - Hip;
        float Length = Delta.Size();
        if (Length > 87.5f || Length < 4.5f)
        {
            // This is intentionally an emergency-only visual safety net. A
            // true discontinuity reinitializes before IK; ordinary reversals
            // must retain their planted support and never reach this branch.
            ++M01EmergencyReplantCount;
            Foot.World = Neutral(Side);
            Foot.Rotation = Facing;
            Foot.bSwing = false;
            End = Rig.InverseTransformPosition(Foot.World) + FVector(0, 0, 10);
            Delta = End - Hip;
            Length = Delta.Size();
        }
        const FVector Axis = Delta / FMath::Max(Length, .001f);
        const FVector Pole(1, Side ? .08f : -.08f, 0);
        const FVector Bend = (Pole - Axis * FVector::DotProduct(Pole, Axis)).GetSafeNormal();
        const float Along = (42.0f * 42.0f - 46.0f * 46.0f + Length * Length) /
            FMath::Max(2.0f * Length, .001f);
        const FVector Knee = Hip + Axis * Along + Bend *
            FMath::Sqrt(FMath::Max(0.0f, 42.0f * 42.0f - Along * Along));
        UStaticMeshComponent* Upper = M01SurveyorParts[Side * 3];
        UStaticMeshComponent* Lower = M01SurveyorParts[Side * 3 + 1];
        UStaticMeshComponent* Sole = M01SurveyorParts[Side * 3 + 2];
        Upper->SetRelativeLocationAndRotation(Hip, FQuat::FindBetweenNormals(FVector::ForwardVector, (Knee - Hip).GetSafeNormal()));
        Lower->SetRelativeLocationAndRotation(Knee, FQuat::FindBetweenNormals(FVector::ForwardVector, (End - Knee).GetSafeNormal()));
        Upper->SetRelativeScale3D(FVector::OneVector);
        Lower->SetRelativeScale3D(FVector::OneVector);
        Sole->SetWorldLocationAndRotation(Foot.World, Foot.Rotation);
        Sole->SetRelativeScale3D(FVector::OneVector);
    }
}
