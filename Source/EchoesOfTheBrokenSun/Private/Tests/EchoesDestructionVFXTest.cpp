#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "EchoesDestructionView.h"
#include "EchoesEntityView.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesDestructionVFXTest,
    "Echoes.Runtime.Presentation.DestructionVFX",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesDestructionVFXTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    FEchoesScopedTestSaveEnvironment TestSaveEnvironment(*this);
    if (!TestSaveEnvironment.IsReady())
    {
        return false;
    }

    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the destruction-VFX test world."));
        return false;
    }
    UWorld* World = WorldWrapper.GetTestWorld();
    if (!TestNotNull(TEXT("Destruction-VFX test world is available"), World))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    FLinearColor MeridianDestructionColor = FLinearColor::Transparent;
    AEchoesDestructionView* Standard =
        World->SpawnActor<AEchoesDestructionView>();
    if (TestNotNull(TEXT("Standard destruction actor spawns"), Standard))
    {
        Standard->InitializeDestruction(
            echoes::sim::Faction::MeridianCompact,
            echoes::sim::EntityType::Soldier,
            false,
            false,
            1.6f);
        MeridianDestructionColor = Standard->GetBaseColor();
        TestTrue(TEXT("Standard destruction uses authored VFX assets"),
                 Standard->IsUsingAuthoredVFXAssets());
        TestTrue(TEXT("Standard destruction has no collision or overlaps"),
                 Standard->HasCollisionDisabled());
        TestTrue(TEXT("Standard destruction cannot affect navigation"),
                 Standard->HasNavigationDisabled());
        const FVector RingScaleBeforeTick = Standard->GetRingScale();
        const FVector ShardLocationBeforeTick = Standard->GetShardALocation();
        const float EmissionBeforeTick = Standard->GetCurrentEmissiveStrength();
        Standard->Tick(0.4f);
        TestFalse(TEXT("Standard destruction expands its shock ring"),
                  Standard->GetRingScale().Equals(RingScaleBeforeTick));
        TestFalse(TEXT("Standard destruction moves its debris accent"),
                  Standard->GetShardALocation().Equals(ShardLocationBeforeTick));
        TestTrue(TEXT("Standard destruction uses one decaying emission envelope"),
                 Standard->GetCurrentEmissiveStrength() < EmissionBeforeTick);
        const uint64 WarmMIDCount = Standard->GetOwnedMIDCreationCount();
        TestEqual(TEXT("A destruction actor owns exactly four reusable MIDs"),
                  WarmMIDCount,
                  static_cast<uint64>(4));
        Standard->RegisterOverflowCoalesced();
        TestEqual(TEXT("Overflow coalescing is presentation-only state"),
                  Standard->GetCoalescedOverflowCount(),
                  static_cast<uint64>(1));
        Standard->PrepareForPool();
        TestFalse(TEXT("A pooled destruction actor is inactive"),
                  Standard->IsPresentationActive());
        TestTrue(TEXT("A pooled destruction actor is hidden"),
                 Standard->IsHidden());
        TestFalse(TEXT("A pooled destruction actor cannot tick"),
                  Standard->IsActorTickEnabled());
        TestEqual(TEXT("Pooling clears coalesced overflow state"),
                  Standard->GetCoalescedOverflowCount(),
                  static_cast<uint64>(0));
        Standard->InitializeDestruction(
            echoes::sim::Faction::HollowChoir,
            echoes::sim::EntityType::CommandCore,
            true,
            true,
            1.6f);
        TestEqual(TEXT("Cross-faction reuse creates no additional MIDs"),
                  Standard->GetOwnedMIDCreationCount(),
                  WarmMIDCount);
        TestTrue(TEXT("Cross-faction reuse applies the new accessibility state"),
                 Standard->IsReducedMotionApplied() &&
                     Standard->IsReducedFlashingApplied());
        TestFalse(TEXT("Cross-faction reuse applies the new faction palette"),
                  Standard->GetBaseColor().Equals(MeridianDestructionColor));
    }

    AEchoesDestructionView* Reduced =
        World->SpawnActor<AEchoesDestructionView>();
    if (TestNotNull(TEXT("Reduced destruction actor spawns"), Reduced))
    {
        Reduced->InitializeDestruction(
            echoes::sim::Faction::KharuunAssemblies,
            echoes::sim::EntityType::CommandCore,
            true,
            true,
            30.0f);
        TestTrue(TEXT("Reduced destruction records reduced motion"),
                 Reduced->IsReducedMotionApplied());
        TestTrue(TEXT("Reduced destruction records reduced flashing"),
                 Reduced->IsReducedFlashingApplied());
        TestEqual(TEXT("Review lifetime is bounded to thirty seconds"),
                  Reduced->GetPresentationLifetimeSeconds(),
                  30.0f);
        const FVector RingScaleBeforeTick = Reduced->GetRingScale();
        const FVector ShardLocationBeforeTick = Reduced->GetShardALocation();
        Reduced->Tick(0.4f);
        TestTrue(TEXT("Reduced-motion destruction keeps its shock ring steady"),
                 Reduced->GetRingScale().Equals(RingScaleBeforeTick));
        TestTrue(TEXT("Reduced-motion destruction keeps its debris steady"),
                 Reduced->GetShardALocation().Equals(ShardLocationBeforeTick));
        TestTrue(TEXT("Reduced-flashing destruction uses steady low emission"),
                 Reduced->GetCurrentEmissiveStrength() <= 1.15f);
        TestTrue(TEXT("Large structures scale the presentation without authority"),
                 Reduced->GetActorScale3D().X > 1.0f);
    }

    AEchoesDestructionView* Choir =
        World->SpawnActor<AEchoesDestructionView>();
    if (TestNotNull(TEXT("Choir destruction actor spawns"), Choir))
    {
        Choir->InitializeDestruction(
            echoes::sim::Faction::HollowChoir,
            echoes::sim::EntityType::HeavyUnit,
            false,
            false,
            1.6f);
        TestTrue(TEXT("Choir destruction uses authored shared VFX geometry"),
                 Choir->IsUsingAuthoredVFXAssets());
        TestTrue(TEXT("Choir destruction remains collision-free"),
                 Choir->HasCollisionDisabled() &&
                     Choir->HasNavigationDisabled());
        TestFalse(
            TEXT("Choir destruction has a distinct phase-pink palette"),
            Standard != nullptr &&
                Choir->GetBaseColor().Equals(MeridianDestructionColor));
        TestFalse(
            TEXT("Choir destruction does not inherit Kharuun orange"),
            Reduced != nullptr &&
                Choir->GetBaseColor().Equals(Reduced->GetBaseColor()));
    }

    if (Standard != nullptr)
    {
        Standard->Destroy();
    }
    if (Reduced != nullptr)
    {
        Reduced->Destroy();
    }
    if (Choir != nullptr)
    {
        Choir->Destroy();
    }

    UEchoesSimulationSubsystem* Bridge =
        World->GetSubsystem<UEchoesSimulationSubsystem>();
    if (TestNotNull(TEXT("Test world owns the simulation subsystem"), Bridge) &&
        TestTrue(TEXT("Prototype scenario starts for destruction integration"),
                 Bridge->StartPrototypeScenario()))
    {
        echoes::sim::EntityId VisibleOwnedUnitId = 0;
        const echoes::sim::Simulation* Simulation = Bridge->GetSimulation();
        if (Simulation != nullptr)
        {
            for (const echoes::sim::Entity& Entity : Simulation->Entities())
            {
                if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
                    Entity.type == echoes::sim::EntityType::Soldier &&
                    Bridge->FindEntityView(Entity.id) != nullptr)
                {
                    VisibleOwnedUnitId = Entity.id;
                    break;
                }
            }
        }
        if (TestTrue(TEXT("A visible owned combat view is available"),
                     VisibleOwnedUnitId != 0))
        {
            echoes::sim::Simulation* MutableSimulation =
                const_cast<echoes::sim::Simulation*>(Bridge->GetSimulation());
            const echoes::sim::Entity* Unit =
                Bridge->FindEntity(VisibleOwnedUnitId);
            if (TestNotNull(TEXT("The visible combat entity remains authoritative"),
                            Unit) &&
                TestNotNull(TEXT("The test owns a mutable simulation fixture"),
                            MutableSimulation))
            {
                const echoes::sim::Vec2 UnitPosition = Unit->position;
                const echoes::sim::EntityId TargetId =
                    MutableSimulation->SpawnEntity(
                        UEchoesSimulationSubsystem::OpponentPlayerId,
                        echoes::sim::Faction::KharuunAssemblies,
                        echoes::sim::EntityType::Soldier,
                        UnitPosition);
                TArray<echoes::sim::EntityId> AttackerIds;
                for (int32 Index = 0; Index < 12; ++Index)
                {
                    const echoes::sim::EntityId AttackerId =
                        MutableSimulation->SpawnEntity(
                            UEchoesSimulationSubsystem::LocalPlayerId,
                            echoes::sim::Faction::MeridianCompact,
                            echoes::sim::EntityType::HeavyUnit,
                            UnitPosition);
                    if (AttackerId != 0)
                    {
                        AttackerIds.Add(AttackerId);
                    }
                }
                Bridge->Tick(0.05f);
                TestNotNull(TEXT("The hostile destruction target receives a visible view"),
                            Bridge->FindEntityView(TargetId));
                int32 QueuedAttackers = 0;
                for (const echoes::sim::EntityId AttackerId : AttackerIds)
                {
                    FString Feedback;
                    QueuedAttackers += Bridge->IssueCommand(
                        echoes::sim::CommandType::Attack,
                        AttackerId,
                        TargetId,
                        Bridge->SimToWorld(UnitPosition),
                        echoes::sim::FutureWellChoice::Dormant,
                        Feedback)
                                           ? 1
                                           : 0;
                }
                TestTrue(TEXT("The integration fixture queues bounded lethal pressure"),
                         QueuedAttackers > 0);
                for (int32 TickIndex = 0;
                     TickIndex < 500 &&
                     Bridge->FindEntity(TargetId) != nullptr;
                     ++TickIndex)
                {
                    Bridge->Tick(0.05f);
                }
                TestNull(TEXT("Authoritative removal clears the entity"),
                         Bridge->FindEntity(TargetId));
                TestNull(TEXT("Authoritative removal clears the live entity view"),
                         Bridge->FindEntityView(TargetId));
                AEchoesDestructionView* IntegratedDestruction = nullptr;
                for (TActorIterator<AEchoesDestructionView> It(World); It; ++It)
                {
                    if (!It->IsActorBeingDestroyed())
                    {
                        IntegratedDestruction = *It;
                        break;
                    }
                }
                if (TestNotNull(
                        TEXT("Authoritative visible removal spawns transient destruction presentation"),
                        IntegratedDestruction))
                {
                    const uint64 ChecksumBeforePresentationTick =
                        Bridge->GetSimulation()->StateChecksum();
                    IntegratedDestruction->Tick(0.2f);
                    TestEqual(
                        TEXT("Presentation-only destruction tick cannot change simulation checksum"),
                        Bridge->GetSimulation()->StateChecksum(),
                        ChecksumBeforePresentationTick);
                }
            }
        }
        Bridge->StopPrototypeScenario();
    }

    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors() && !WorldWrapper.HasFailed();
}

#endif
