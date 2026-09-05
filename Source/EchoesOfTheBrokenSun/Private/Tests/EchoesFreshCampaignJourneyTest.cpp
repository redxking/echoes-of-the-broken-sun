#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"
#include "EchoesAssemblyOfTheMissingMissionModel.h"
#include "EchoesBrokenSunMissionModel.h"
#include "EchoesCampaignProgress.h"
#include "EchoesChoirAtLumeReachMissionModel.h"
#include "EchoesCityReserveMissionModel.h"
#include "EchoesFutureThatWonMissionModel.h"
#include "EchoesNamesWithoutBirthsMissionModel.h"
#include "EchoesNoNeutralLedgerMissionModel.h"
#include "EchoesPlayerController.h"
#include "EchoesReserveAuthorityMissionModel.h"
#include "EchoesSeveralVoicesOneCommandMissionModel.h"
#include "EchoesShapeBesideUsMissionModel.h"
#include "EchoesShapeOfSilenceMissionModel.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesTermsOfContinuanceMissionModel.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Tests/AutomationCommon.h"

namespace
{
using echoes::sim::ChoirIdentityState;
using echoes::sim::CommandType;
using echoes::sim::Entity;
using echoes::sim::EntityId;
using echoes::sim::EntityType;
using echoes::sim::FutureWellChoice;
using echoes::sim::ResearchType;
using echoes::sim::Vec2;

struct FFreshRouteSpec final
{
    const TCHAR* Label = TEXT("route");
    FutureWellChoice FoundingChoice = FutureWellChoice::Dormant;
    FutureWellChoice LumeChoice = FutureWellChoice::Dormant;
    EEchoesFinalResolution FinalResolution =
        EEchoesFinalResolution::None;
    uint8 ExpectedFinalPlanKey = 0;
    uint8 ExpectedFinalResolutionMask = 0;
};

bool TickUntil(
    UEchoesSimulationSubsystem* Bridge,
    const TFunction<bool()>& Predicate,
    int32 MaximumTicks)
{
    for (int32 TickIndex = 0; TickIndex < MaximumTicks; ++TickIndex)
    {
        if (Predicate())
        {
            return true;
        }
        Bridge->Tick(0.05f);
    }
    return Predicate();
}

bool IsAtSite(
    const UEchoesSimulationSubsystem* Bridge,
    EntityId Entity,
    const Vec2& Site,
    int32 RadiusTiles = 1)
{
    const echoes::sim::Entity* Current = Bridge->FindEntity(Entity);
    if (Current == nullptr)
    {
        return false;
    }
    const int64 DeltaX =
        static_cast<int64>(Current->position.x.Raw()) - Site.x.Raw();
    const int64 DeltaY =
        static_cast<int64>(Current->position.y.Raw()) - Site.y.Raw();
    const int64 RadiusRaw =
        static_cast<int64>(RadiusTiles) * echoes::sim::kFixedScale;
    return DeltaX * DeltaX + DeltaY * DeltaY <=
        RadiusRaw * RadiusRaw;
}

TArray<EntityId> FindOwnedEntities(
    const UEchoesSimulationSubsystem* Bridge,
    EntityType Type,
    TOptional<echoes::sim::Faction> Faction = {})
{
    TArray<EntityId> Result;
    const echoes::sim::Simulation* Simulation = Bridge->GetSimulation();
    if (Simulation == nullptr)
    {
        return Result;
    }
    for (const Entity& Candidate : Simulation->Entities())
    {
        if (Candidate.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            Candidate.type == Type &&
            (!Faction.IsSet() || Candidate.faction == Faction.GetValue()))
        {
            Result.Add(Candidate.id);
        }
    }
    Result.Sort();
    return Result;
}

bool FindValidBuildSiteForType(
    const UEchoesSimulationSubsystem* Bridge,
    EntityType Type,
    const Vec2& Center,
    int32 Radius,
    Vec2& OutSite)
{
    const echoes::sim::Simulation* Simulation = Bridge->GetSimulation();
    if (Simulation == nullptr)
    {
        return false;
    }
    for (int32 DistanceSquared = 0;
         DistanceSquared <= Radius * Radius;
         ++DistanceSquared)
    {
        for (int32 DeltaY = -Radius; DeltaY <= Radius; ++DeltaY)
        {
            for (int32 DeltaX = -Radius; DeltaX <= Radius; ++DeltaX)
            {
                if (DeltaX * DeltaX + DeltaY * DeltaY != DistanceSquared)
                {
                    continue;
                }
                const Vec2 Candidate = Vec2::FromTiles(
                    Center.x.FloorToInt() + DeltaX,
                    Center.y.FloorToInt() + DeltaY);
                if (Simulation->ValidatePlacement(
                        UEchoesSimulationSubsystem::LocalPlayerId,
                        Type,
                        Candidate) == echoes::sim::PlacementResult::Valid)
                {
                    OutSite = Candidate;
                    return true;
                }
            }
        }
    }
    return false;
}

bool FindValidBuildSite(
    const UEchoesSimulationSubsystem* Bridge,
    const Vec2& Center,
    int32 Radius,
    Vec2& OutSite)
{
    return FindValidBuildSiteForType(
        Bridge,
        EntityType::UtilityStructure,
        Center,
        Radius,
        OutSite);
}

EEchoesOperationMode OperationForMission(int32 MissionNumber)
{
    static constexpr EEchoesOperationMode Operations[] = {
        EEchoesOperationMode::CampaignPrologue,
        EEchoesOperationMode::CampaignSevenAccounts,
        EEchoesOperationMode::CampaignCityReserve,
        EEchoesOperationMode::CampaignUnburiedRoad,
        EEchoesOperationMode::CampaignTermsOfContinuance,
        EEchoesOperationMode::CampaignNamesWithoutBirths,
        EEchoesOperationMode::CampaignShapeOfSilence,
        EEchoesOperationMode::CampaignShapeBesideUs,
        EEchoesOperationMode::CampaignReserveAuthority,
        EEchoesOperationMode::CampaignChoirAtLumeReach,
        EEchoesOperationMode::CampaignNoNeutralLedger,
        EEchoesOperationMode::CampaignFutureThatWon,
        EEchoesOperationMode::CampaignAssemblyOfTheMissing,
        EEchoesOperationMode::CampaignSeveralVoicesOneCommand,
        EEchoesOperationMode::CampaignTheBrokenSun};
    return Operations[FMath::Clamp(MissionNumber - 1, 0, 14)];
}

// This mapping is intentionally test-owned. Using the production
// ResolutionMask here would let the expected and observed sides share the
// same defect if two resolution identities were mapped to the wrong bits.
uint8 TestOwnedResolutionBit(EEchoesFinalResolution Resolution)
{
    switch (Resolution)
    {
        case EEchoesFinalResolution::Restoration: return 0x01;
        case EEchoesFinalResolution::ControlledStabilization: return 0x02;
        case EEchoesFinalResolution::Extinguishment: return 0x04;
        case EEchoesFinalResolution::OpenEvolution: return 0x08;
        case EEchoesFinalResolution::None: return 0;
    }
    return 0;
}

Vec2 TestOwnedFreshJourneyBrokenSunResolutionSite(
    EEchoesFinalResolution Resolution)
{
    switch (Resolution)
    {
        case EEchoesFinalResolution::Restoration:
            return Vec2::FromTiles(32, 49);
        case EEchoesFinalResolution::ControlledStabilization:
            return Vec2::FromTiles(32, 44);
        case EEchoesFinalResolution::Extinguishment:
            return Vec2::FromTiles(26, 49);
        case EEchoesFinalResolution::OpenEvolution:
            return Vec2::FromTiles(26, 54);
        case EEchoesFinalResolution::None:
            return {};
    }
    return {};
}

Vec2 TestOwnedNoNeutralRallySite(FutureWellChoice Choice)
{
    switch (Choice)
    {
        case FutureWellChoice::Harvest: return Vec2::FromTiles(18, 56);
        case FutureWellChoice::Preserve: return Vec2::FromTiles(32, 56);
        case FutureWellChoice::Reshape: return Vec2::FromTiles(32, 43);
        default: return {};
    }
}

Vec2 TestOwnedFutureWonWellApproach(FutureWellChoice Choice)
{
    switch (Choice)
    {
        case FutureWellChoice::Harvest: return Vec2::FromTiles(16, 54);
        case FutureWellChoice::Preserve: return Vec2::FromTiles(30, 54);
        case FutureWellChoice::Reshape: return Vec2::FromTiles(32, 41);
        default: return {};
    }
}

Vec2 TestOwnedUnburiedRoadRoadhead(FutureWellChoice Choice)
{
    switch (Choice)
    {
        case FutureWellChoice::Harvest: return Vec2::FromTiles(14, 28);
        case FutureWellChoice::Preserve: return Vec2::FromTiles(32, 28);
        case FutureWellChoice::Reshape: return Vec2::FromTiles(50, 28);
        default: return {};
    }
}

Vec2 TestOwnedUnburiedRoadSpineSite(FutureWellChoice Choice)
{
    switch (Choice)
    {
        case FutureWellChoice::Harvest: return Vec2::FromTiles(14, 37);
        case FutureWellChoice::Preserve: return Vec2::FromTiles(32, 37);
        case FutureWellChoice::Reshape: return Vec2::FromTiles(49, 35);
        default: return {};
    }
}

Vec2 TestOwnedUnburiedRoadShardSite(FutureWellChoice Choice)
{
    switch (Choice)
    {
        case FutureWellChoice::Harvest: return Vec2::FromTiles(20, 43);
        case FutureWellChoice::Preserve: return Vec2::FromTiles(38, 43);
        case FutureWellChoice::Reshape: return Vec2::FromTiles(44, 40);
        default: return {};
    }
}

int64 MinimumOpponentCombatDistanceSquaredRaw(
    const UEchoesSimulationSubsystem* Bridge,
    const Vec2& Site)
{
    const echoes::sim::Simulation* Simulation = Bridge->GetSimulation();
    int64 Minimum = TNumericLimits<int64>::Max();
    if (Simulation == nullptr)
    {
        return Minimum;
    }
    for (const Entity& Candidate : Simulation->Entities())
    {
        const bool bMobileCombatUnit =
            Candidate.type == EntityType::Soldier ||
            Candidate.type == EntityType::HeavyUnit ||
            Candidate.type == EntityType::ScoutUnit;
        if (Candidate.owner == UEchoesSimulationSubsystem::LocalPlayerId ||
            Candidate.owner == echoes::sim::kNeutralPlayer ||
            Candidate.hitPoints <= 0 || !bMobileCombatUnit)
        {
            continue;
        }
        const int64 DeltaX =
            static_cast<int64>(Candidate.position.x.Raw()) - Site.x.Raw();
        const int64 DeltaY =
            static_cast<int64>(Candidate.position.y.Raw()) - Site.y.Raw();
        Minimum = FMath::Min(Minimum, DeltaX * DeltaX + DeltaY * DeltaY);
    }
    return Minimum;
}

int64 MinimumEntityDistanceSquaredRaw(
    const UEchoesSimulationSubsystem* Bridge,
    const Vec2& Site)
{
    const echoes::sim::Simulation* Simulation = Bridge->GetSimulation();
    int64 Minimum = TNumericLimits<int64>::Max();
    if (Simulation == nullptr)
    {
        return Minimum;
    }
    for (const Entity& Candidate : Simulation->Entities())
    {
        if (Candidate.hitPoints <= 0)
        {
            continue;
        }
        const int64 DeltaX =
            static_cast<int64>(Candidate.position.x.Raw()) - Site.x.Raw();
        const int64 DeltaY =
            static_cast<int64>(Candidate.position.y.Raw()) - Site.y.Raw();
        Minimum = FMath::Min(Minimum, DeltaX * DeltaX + DeltaY * DeltaY);
    }
    return Minimum;
}

FString DescribeFreshJourneyEntity(
    const TCHAR* Label,
    EntityId Id,
    const Entity* Current)
{
    if (Current == nullptr)
    {
        return FString::Printf(TEXT("%s{id=%u missing}"), Label, Id);
    }
    return FString::Printf(
        TEXT("%s{id=%u owner=%u faction=%u type=%u hp=%d/%d pos=(%d,%d) completed=%s progress=%d/%d order=%u target=%u destination=(%d,%d) wellChoice=%u activation=%llu reshapeUntil=%llu}"),
        Label,
        Id,
        Current->owner,
        static_cast<uint8>(Current->faction),
        static_cast<uint8>(Current->type),
        Current->hitPoints,
        Current->maxHitPoints,
        Current->position.x.FloorToInt(),
        Current->position.y.FloorToInt(),
        Current->completed ? TEXT("true") : TEXT("false"),
        Current->constructionProgress,
        Current->constructionRequired,
        static_cast<uint8>(Current->order.type),
        Current->order.target,
        Current->order.destination.x.FloorToInt(),
        Current->order.destination.y.FloorToInt(),
        static_cast<uint8>(Current->wellChoice),
        static_cast<unsigned long long>(Current->wellActivationTick),
        static_cast<unsigned long long>(Current->reshapeUntilTick));
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesFreshCampaignJourneyTest,
    "Echoes.Runtime.Campaign.FreshJourney",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesFreshCampaignJourneyTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    FEchoesScopedTestSaveEnvironment TestEnvironment(
        *this,
        EEchoesTestSaveOverrideMode::ExactFiles);
    if (!TestTrue(
            TEXT("The journey owns isolated campaign and quick-save storage"),
            TestEnvironment.IsReady() &&
                !TestEnvironment.CampaignPath.IsEmpty() &&
                !TestEnvironment.QuickSavePath.IsEmpty()))
    {
        return false;
    }
    const FString CampaignPath = TestEnvironment.CampaignPath;

    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the fresh campaign journey world."));
        return false;
    }
    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge =
        World != nullptr
            ? World->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (!TestNotNull(TEXT("The journey world owns the simulation subsystem"),
                     Bridge) ||
        !TestTrue(TEXT("The title scenario starts"),
                  Bridge != nullptr && Bridge->StartPrototypeScenario()) ||
        !TestTrue(
            TEXT("The subsystem uses only the isolated save paths"),
            Bridge->CampaignProgressPath == TestEnvironment.CampaignPath &&
                Bridge->GetActiveQuickSavePath() ==
                    TestEnvironment.QuickSavePath))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    AEchoesPlayerController* Controller =
        World->SpawnActor<AEchoesPlayerController>();
    if (Controller != nullptr)
    {
        World->AddController(Controller);
    }
    if (!TestNotNull(TEXT("The fresh journey owns a player controller"),
                     Controller) ||
        !TestTrue(TEXT("The controller is the world's result recipient"),
                  World->GetFirstPlayerController() == Controller))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    FString Feedback;
    const auto Require = [this, &Feedback](
        bool bCondition,
        const FString& Label)
    {
        if (!bCondition)
        {
            AddError(
                Feedback.IsEmpty()
                    ? Label
                    : FString::Printf(
                          TEXT("%s — %s"), *Label, *Feedback));
        }
        Feedback.Reset();
        return bCondition;
    };
    const auto VerifyCompletion = [this, Bridge, Controller, CampaignPath,
                                   &Require, &Feedback](
        int32 MissionNumber,
        EEchoesCampaignCommitStatus ExpectedStatus)
    {
        const EEchoesCampaignMissionId Mission =
            static_cast<EEchoesCampaignMissionId>(MissionNumber);
        const FEchoesCampaignDecisionRecord* Record =
            Bridge->GetCampaignProgress().FindDecision(Mission);
        if (!Require(
                Record != nullptr,
                FString::Printf(
                    TEXT("Mission %02d produced its authoritative record"),
                    MissionNumber)) ||
            !Require(
                Bridge->GetCampaignProgress().Decisions.Num() ==
                    (ExpectedStatus ==
                             EEchoesCampaignCommitStatus::ReplayConflict
                         ? 15
                         : MissionNumber),
                FString::Printf(
                    TEXT("Mission %02d preserves the exact ledger length"),
                    MissionNumber)) ||
            !Require(
                Controller->IsMatchResultVisible() &&
                    Controller->IsCampaignResult() &&
                    Controller->WasCampaignSuccessful() &&
                    Controller->GetPresentedCampaignOperation() ==
                        OperationForMission(MissionNumber),
                FString::Printf(
                    TEXT("Mission %02d reaches the player-visible result"),
                    MissionNumber)) ||
            !Require(
                Controller->GetCampaignCommitStatus() == ExpectedStatus,
                FString::Printf(
                    TEXT("Mission %02d reports the expected commit state"),
                    MissionNumber)))
        {
            return false;
        }

        FEchoesCampaignProgress Reloaded;
        if (!Require(
                FEchoesCampaignProgressStore::LoadWithBackup(
                    CampaignPath, Reloaded, Feedback),
                FString::Printf(
                    TEXT("Mission %02d reopens persisted storage"),
                    MissionNumber)) ||
            !Require(
                Reloaded.Decisions ==
                    Bridge->GetCampaignProgress().Decisions,
                FString::Printf(
                    TEXT("Mission %02d decoded ledger matches memory"),
                    MissionNumber)))
        {
            return false;
        }
        return true;
    };
    const auto AdvanceToNextMission = [Bridge, Controller, &Require](
        int32 CompletedMission)
    {
        Controller->ConfirmPrimaryAction();
        if (!Require(
                !Controller->IsMatchResultVisible() &&
                    Controller->IsMissionBriefingVisible() &&
                    Bridge->GetOperationMode() ==
                        OperationForMission(CompletedMission + 1) &&
                    Bridge->IsScenarioPaused(),
                FString::Printf(
                    TEXT("Mission %02d Enter opens the exact next briefing"),
                    CompletedMission)))
        {
            return false;
        }
        Controller->ConfirmPrimaryAction();
        return Require(
            !Controller->IsMissionBriefingVisible() &&
                !Bridge->IsScenarioPaused(),
            FString::Printf(
                TEXT("Mission %02d briefing deploys through Enter"),
                CompletedMission + 1));
    };
    const auto PreserveQuickSaveFamily = [Bridge]()
    {
        return Bridge->GetActiveQuickSavePath();
    };

    const auto BeginFreshRoute = [Bridge, Controller, CampaignPath, &Feedback,
                                  &Require](bool bReplaceExisting)
    {
        Controller->PresentTitleScreen();
        if (!Require(
                Controller->IsTitleScreenVisible() &&
                    Bridge->IsScenarioPaused(),
                TEXT("The route begins at the ordinary title screen")))
        {
            return false;
        }
        Controller->RequestNewCampaign();
        if (bReplaceExisting)
        {
            if (!Require(
                    Controller->IsNewCampaignConfirmationArmed(),
                    TEXT("New Campaign requires explicit confirmation")))
            {
                return false;
            }
            Controller->RequestNewCampaign();
        }
        if (!Require(
                Bridge->GetCampaignProgress().Decisions.IsEmpty() &&
                    Bridge->GetCampaignJourney().State ==
                        EEchoesCampaignJourneyState::Ready &&
                    Bridge->GetCampaignJourney().CompletedMissionCount == 0,
                TEXT("New Campaign exposes an empty ledger")))
        {
            return false;
        }
        if (bReplaceExisting)
        {
            FEchoesCampaignProgress EmptyPrimary;
            if (!Require(
                    FEchoesCampaignProgressStore::LoadGeneration(
                        CampaignPath, EmptyPrimary, Feedback) &&
                        EmptyPrimary.Decisions.IsEmpty(),
                    TEXT("New Campaign persists an exact empty primary ledger")))
            {
                return false;
            }
        }
        Controller->ContinueCampaign();
        if (!Require(
                Controller->IsMissionBriefingVisible() &&
                    Bridge->GetOperationMode() ==
                        EEchoesOperationMode::CampaignPrologue &&
                    Bridge->IsScenarioPaused(),
                TEXT("Continue Campaign opens Mission 01 briefing")))
        {
            return false;
        }
        Controller->ConfirmPrimaryAction();
        return Require(
            !Controller->IsMissionBriefingVisible() &&
                !Bridge->IsScenarioPaused(),
            TEXT("Mission 01 deploys from its briefing"));
    };

    const auto RunMissionsOneThroughFive = [
        Bridge, Controller, &Feedback, &Require, &VerifyCompletion,
        &AdvanceToNextMission](const FFreshRouteSpec& Spec)
    {
        const auto Move = [Bridge, &Feedback](EntityId Actor, const Vec2& Site)
        {
            return Bridge->IssueCommand(
                CommandType::Move,
                Actor,
                0,
                Bridge->SimToWorld(Site),
                FutureWellChoice::Dormant,
                Feedback);
        };

        // Mission 01: What the Ledger Keeps.
        const EntityId Carrier = Bridge->GetArchiveCarrierId();
        const TArray<EntityId> M01Workers =
            FindOwnedEntities(Bridge, EntityType::Worker);
        EntityId Well = 0;
        for (const Entity& Candidate : Bridge->GetSimulation()->Entities())
        {
            if (Candidate.type == EntityType::FutureWell)
            {
                Well = Candidate.id;
                break;
            }
        }
        const echoes::sim::Simulation* M01Simulation =
            Bridge->GetSimulation();
        const echoes::sim::PlayerState* M01Player =
            M01Simulation != nullptr
                ? M01Simulation->FindPlayer(
                      UEchoesSimulationSubsystem::LocalPlayerId)
                : nullptr;
        const int32 M01StartingDawn =
            M01Player != nullptr ? M01Player->resources.dawnshards : -1;
        const int32 M01ReshapeDawnCost =
            M01Simulation != nullptr
                ? M01Simulation->Config().rules.futureWell.reshapeDawnCost
                : -1;
        const bool bM01FoundingReshape =
            Spec.FoundingChoice == FutureWellChoice::Reshape;
        const auto M01FoundingChoiceActivated = [Bridge, Well, &Spec]()
        {
            const echoes::sim::Simulation* Current =
                Bridge->GetSimulation();
            const Entity* CurrentWell = Bridge->FindEntity(Well);
            return Current != nullptr && CurrentWell != nullptr &&
                CurrentWell->owner ==
                    UEchoesSimulationSubsystem::LocalPlayerId &&
                CurrentWell->wellChoice == Spec.FoundingChoice &&
                CurrentWell->wellActivationTick > 0 &&
                CurrentWell->wellActivationTick <= Current->CurrentTick();
        };
        const auto M01ReshapeSpendIsExact = [
            Bridge,
            bM01FoundingReshape,
            M01StartingDawn,
            M01ReshapeDawnCost]()
        {
            if (!bM01FoundingReshape)
            {
                return true;
            }
            const echoes::sim::Simulation* Current =
                Bridge->GetSimulation();
            const echoes::sim::PlayerState* Player =
                Current != nullptr
                    ? Current->FindPlayer(
                          UEchoesSimulationSubsystem::LocalPlayerId)
                    : nullptr;
            return Player != nullptr &&
                Player->resources.dawnshards ==
                    M01StartingDawn - M01ReshapeDawnCost;
        };
        if (!Require(
                Carrier != 0 && !M01Workers.IsEmpty() && Well != 0 &&
                    M01Player != nullptr,
                TEXT("Mission 01 exposes carrier, worker, and Future Well")) ||
            !Require(
                !bM01FoundingReshape ||
                    (M01ReshapeDawnCost > 0 &&
                     M01StartingDawn >= M01ReshapeDawnCost),
                TEXT("Mission 01 funds the authored Reshape founding choice")) ||
            !Require(
                Move(
                    Carrier,
                    UEchoesSimulationSubsystem::GetArchiveRecoverySite()),
                TEXT("Mission 01 carrier accepts archive recovery")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge]()
                    {
                        return Bridge->GetProloguePhase() ==
                            EEchoesProloguePhase::DecideFutureWell;
                    },
                    800),
                TEXT("Mission 01 reaches the Future Well decision")) ||
            !Require(
                Move(M01Workers[0], Vec2::FromTiles(29, 29)),
                TEXT("Mission 01 worker accepts the scouting route")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge, Well]()
                    {
                        return Bridge->GetSimulation()->IsEntityVisibleTo(
                            UEchoesSimulationSubsystem::LocalPlayerId,
                            Well);
                    },
                    900),
                TEXT("Mission 01 scouting reveals the Future Well")) ||
            !Require(
                Bridge->IssueCommand(
                    CommandType::FutureWell,
                    M01Workers[0],
                    Well,
                    Bridge->SimToWorld(
                        Bridge->FindEntity(Well)->position),
                    Spec.FoundingChoice,
                    Feedback),
                TEXT("Mission 01 accepts the route's founding choice")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge]()
                    {
                        return Bridge->GetProloguePhase() ==
                            EEchoesProloguePhase::Withdraw;
                    },
                    900),
                TEXT("Mission 01 records the Future Well choice")) ||
            !Require(
                M01FoundingChoiceActivated(),
                TEXT("Mission 01 activates the exact founding choice")) ||
            !Require(
                M01ReshapeSpendIsExact(),
                TEXT("Mission 01 spends the exact authored Reshape cost")) ||
            !Require(
                Move(
                    Carrier,
                    UEchoesSimulationSubsystem::GetEvacuationSite()),
                TEXT("Mission 01 carrier accepts evacuation")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge]()
                    {
                        return Bridge->GetProloguePhase() ==
                            EEchoesProloguePhase::Complete;
                    },
                    900),
                TEXT("Mission 01 completes through ordinary play")) ||
            !VerifyCompletion(1, EEchoesCampaignCommitStatus::Added) ||
            !AdvanceToNextMission(1))
        {
            return false;
        }

        // Mission 02: Seven Accounts of Rain.
        const EntityId M02Waystone = Bridge->GetMigrationWaystoneId();
        const EntityId M02Bearer = Bridge->GetMemoryBearerId();
        const FEchoesSevenAccountsRoute M02Route =
            Bridge->GetSevenAccountsRoute();
        if (!Require(
                Bridge->IssueCommand(
                    CommandType::ToggleWaystoneRoot,
                    M02Waystone,
                    0,
                    FVector::ZeroVector,
                    FutureWellChoice::Dormant,
                    Feedback),
                TEXT("Mission 02 Waystone accepts uproot")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge, M02Waystone]()
                    {
                        const Entity* Current =
                            Bridge->FindEntity(M02Waystone);
                        return Current != nullptr &&
                            Current->waystoneMode ==
                                echoes::sim::WaystoneMode::Mobile;
                    },
                    300),
                TEXT("Mission 02 Waystone becomes mobile")) ||
            !Require(
                Move(M02Waystone, M02Route.WaystoneAnchor),
                TEXT("Mission 02 Waystone accepts its route")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge, M02Waystone, M02Route]()
                    {
                        return IsAtSite(
                            Bridge,
                            M02Waystone,
                            M02Route.WaystoneAnchor);
                    },
                    2400),
                TEXT("Mission 02 Waystone reaches its anchor")) ||
            !Require(
                Bridge->IssueCommand(
                    CommandType::ToggleWaystoneRoot,
                    M02Waystone,
                    0,
                    FVector::ZeroVector,
                    FutureWellChoice::Dormant,
                    Feedback),
                TEXT("Mission 02 Waystone accepts root")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge]()
                    {
                        return Bridge->GetSevenAccountsPhase() ==
                            EEchoesSevenAccountsPhase::RecallMemory;
                    },
                    400),
                TEXT("Mission 02 opens memory recall")) ||
            !Require(
                Move(M02Bearer, M02Route.MemoryAccountSite),
                TEXT("Mission 02 bearer accepts the memory route")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge]()
                    {
                        return Bridge->GetSevenAccountsPhase() ==
                            EEchoesSevenAccountsPhase::Complete;
                    },
                    2600),
                TEXT("Mission 02 completes through ordinary play")) ||
            !VerifyCompletion(2, EEchoesCampaignCommitStatus::Added) ||
            !AdvanceToNextMission(2))
        {
            return false;
        }

        // Mission 03: A City on Reserve.
        const TArray<EntityId> M03Workers =
            FindOwnedEntities(Bridge, EntityType::Worker);
        const Vec2 M03Sites[] = {
            Vec2::FromTiles(18, 10),
            Vec2::FromTiles(10, 18),
            Vec2::FromTiles(15, 15)};
        if (!Require(
                M03Workers.Num() >= 3,
                TEXT("Mission 03 exposes three construction workers")))
        {
            return false;
        }
        for (int32 Index = 0; Index < 3; ++Index)
        {
            if (!Require(
                    Bridge->IssueBuildCommand(
                        M03Workers[Index],
                        EntityType::Dropoff,
                        Bridge->SimToWorld(M03Sites[Index]),
                        Feedback),
                    FString::Printf(
                        TEXT("Mission 03 worker %d accepts its Power Link"),
                        Index + 1)))
            {
                return false;
            }
        }
        if (!Require(
                TickUntil(
                    Bridge,
                    [Bridge]()
                    {
                        return Bridge->GetCityReservePhase() ==
                            EEchoesCityReservePhase::Complete;
                    },
                    3000),
                TEXT("Mission 03 powers all three districts")) ||
            !VerifyCompletion(3, EEchoesCampaignCommitStatus::Added) ||
            !AdvanceToNextMission(3))
        {
            return false;
        }

        // Mission 04: The Unburied Road.
        const EntityId M04Waystone = Bridge->GetMigrationWaystoneId();
        const EntityId M04Bearer = Bridge->GetMemoryBearerId();
        const FEchoesUnburiedRoadRoute M04Route =
            Bridge->GetUnburiedRoadRoute();
        const TArray<EntityId> M04Workers =
            FindOwnedEntities(Bridge, EntityType::Worker);
        const TArray<EntityId> M04Cores =
            FindOwnedEntities(Bridge, EntityType::CommandCore);
        EntityId M04WorkerId =
            M04Workers.IsEmpty() ? 0 : M04Workers[0];
        const EntityId M04CoreId =
            M04Cores.IsEmpty() ? 0 : M04Cores[0];
        TArray<EntityId> M04GuardIds;
        TArray<EntityId> M04GuardTargetIds;
        TArray<FString> M04LastKnownGuards;
        FString M04FirstObservedLoss = TEXT("none");
        uint64 M04FirstObservedLossTick = 0;
        FString M04LastKnownCore = DescribeFreshJourneyEntity(
            TEXT("core"), M04CoreId, Bridge->FindEntity(M04CoreId));
        FString M04LastKnownBearer = DescribeFreshJourneyEntity(
            TEXT("bearer"), M04Bearer, Bridge->FindEntity(M04Bearer));
        FString M04LastKnownWaystone = DescribeFreshJourneyEntity(
            TEXT("waystone"),
            M04Waystone,
            Bridge->FindEntity(M04Waystone));
        FString M04LastKnownWorker = DescribeFreshJourneyEntity(
            TEXT("worker"),
            M04WorkerId,
            Bridge->FindEntity(M04WorkerId));
        bool bM04TrackProtectedLoss = false;
        const echoes::sim::Simulation* M04Simulation =
            Bridge->GetSimulation();
        const int64 M04MinimumOpponentSeparationRaw =
            22LL * echoes::sim::kFixedScale;
        const int64 M04OpponentCombatDistanceSquaredRaw =
            MinimumOpponentCombatDistanceSquaredRaw(
                Bridge, M04Route.ListeningSpineSite);
        const int64 M04MissionDomainClearanceRaw =
            3LL * echoes::sim::kFixedScale;
        const int64 M04SpineEntityDistanceSquaredRaw =
            MinimumEntityDistanceSquaredRaw(
                Bridge, M04Route.ListeningSpineSite);
        const int64 M04ShardEntityDistanceSquaredRaw =
            MinimumEntityDistanceSquaredRaw(
                Bridge, M04Route.MemoryShardSite);
        const auto ObserveMissionFourProtectedState = [
            Bridge,
            M04Waystone,
            M04Bearer,
            M04CoreId,
            &M04WorkerId,
            &M04GuardIds,
            &M04LastKnownGuards,
            &M04FirstObservedLoss,
            &M04FirstObservedLossTick,
            &M04LastKnownCore,
            &M04LastKnownBearer,
            &M04LastKnownWaystone,
            &M04LastKnownWorker,
            &bM04TrackProtectedLoss]()
        {
            if (!bM04TrackProtectedLoss)
            {
                return;
            }
            const auto ObserveProtected = [
                Bridge,
                &M04FirstObservedLoss,
                &M04FirstObservedLossTick](
                    const TCHAR* Label,
                    EntityId Id,
                    FString& LastKnown)
            {
                const Entity* Current = Bridge->FindEntity(Id);
                if (Current != nullptr && Current->hitPoints > 0)
                {
                    LastKnown = DescribeFreshJourneyEntity(
                        Label, Id, Current);
                    return;
                }
                if (M04FirstObservedLoss == TEXT("none"))
                {
                    const echoes::sim::Simulation* Simulation =
                        Bridge->GetSimulation();
                    M04FirstObservedLossTick =
                        Simulation != nullptr
                            ? Simulation->CurrentTick()
                            : 0;
                    M04FirstObservedLoss = FString::Printf(
                        TEXT("%s{id=%u observed=%s lastKnown=%s}"),
                        Label,
                        Id,
                        *DescribeFreshJourneyEntity(
                            Label, Id, Current),
                        *LastKnown);
                }
            };
            ObserveProtected(
                TEXT("core"), M04CoreId, M04LastKnownCore);
            ObserveProtected(
                TEXT("bearer"), M04Bearer, M04LastKnownBearer);
            ObserveProtected(
                TEXT("waystone"), M04Waystone, M04LastKnownWaystone);
            ObserveProtected(
                TEXT("worker"), M04WorkerId, M04LastKnownWorker);
            while (M04LastKnownGuards.Num() < M04GuardIds.Num())
            {
                M04LastKnownGuards.Add(TEXT("unobserved"));
            }
            for (int32 GuardIndex = 0;
                 GuardIndex < M04GuardIds.Num();
                 ++GuardIndex)
            {
                const FString GuardLabel = FString::Printf(
                    TEXT("guard%d"), GuardIndex + 1);
                ObserveProtected(
                    *GuardLabel,
                    M04GuardIds[GuardIndex],
                    M04LastKnownGuards[GuardIndex]);
            }
        };
        const auto WriteMissionFourDiagnostic = [
            Bridge,
            M04Route,
            M04Waystone,
            M04Bearer,
            M04CoreId,
            &M04WorkerId,
            &M04LastKnownGuards,
            &M04FirstObservedLoss,
            &M04FirstObservedLossTick,
            &M04LastKnownCore,
            &M04LastKnownBearer,
            &M04LastKnownWaystone,
            &M04LastKnownWorker,
            &ObserveMissionFourProtectedState,
            &Spec,
            &Feedback](EEchoesUnburiedRoadPhase ExpectedPhase)
        {
            ObserveMissionFourProtectedState();
            const echoes::sim::Simulation* Simulation =
                Bridge->GetSimulation();
            EntityId SpineId = 0;
            const Entity* Spine = nullptr;
            if (Simulation != nullptr)
            {
                for (const Entity& Candidate : Simulation->Entities())
                {
                    if (Candidate.owner ==
                            UEchoesSimulationSubsystem::LocalPlayerId &&
                        Candidate.type == EntityType::UtilityStructure &&
                        Candidate.position == M04Route.ListeningSpineSite)
                    {
                        SpineId = Candidate.id;
                        Spine = &Candidate;
                        break;
                    }
                }
            }
            const Entity* Waystone = Bridge->FindEntity(M04Waystone);
            FString GuardLastKnownSummary;
            for (int32 GuardIndex = 0;
                 GuardIndex < M04LastKnownGuards.Num();
                 ++GuardIndex)
            {
                if (!GuardLastKnownSummary.IsEmpty())
                {
                    GuardLastKnownSummary += TEXT(" | ");
                }
                GuardLastKnownSummary += M04LastKnownGuards[GuardIndex];
            }
            if (GuardLastKnownSummary.IsEmpty())
            {
                GuardLastKnownSummary = TEXT("none");
            }
            const FString PriorFeedback =
                Feedback.IsEmpty() ? TEXT("none") : Feedback;
            Feedback = FString::Printf(
                TEXT("[M04_DIAGNOSTIC] tick=%llu outcome=%u phase=%u expectedPhase=%u foundingChoice=%u roadhead=(%d,%d) spineSite=(%d,%d) shard=(%d,%d) waystoneMode=%u firstObservedLossTick=%llu firstObservedLoss=%s priorFeedback=%s lastKnown={%s %s %s %s guards=[%s]} current={%s %s %s %s %s}"),
                static_cast<unsigned long long>(
                    Simulation != nullptr ? Simulation->CurrentTick() : 0),
                Simulation != nullptr
                    ? static_cast<uint8>(Simulation->Outcome())
                    : 0xFF,
                static_cast<uint8>(Bridge->GetUnburiedRoadPhase()),
                static_cast<uint8>(ExpectedPhase),
                static_cast<uint8>(Spec.FoundingChoice),
                M04Route.Roadhead.x.FloorToInt(),
                M04Route.Roadhead.y.FloorToInt(),
                M04Route.ListeningSpineSite.x.FloorToInt(),
                M04Route.ListeningSpineSite.y.FloorToInt(),
                M04Route.MemoryShardSite.x.FloorToInt(),
                M04Route.MemoryShardSite.y.FloorToInt(),
                Waystone != nullptr
                    ? static_cast<uint8>(Waystone->waystoneMode)
                    : 0xFF,
                static_cast<unsigned long long>(M04FirstObservedLossTick),
                *M04FirstObservedLoss,
                *PriorFeedback,
                *M04LastKnownCore,
                *M04LastKnownBearer,
                *M04LastKnownWaystone,
                *M04LastKnownWorker,
                *GuardLastKnownSummary,
                *DescribeFreshJourneyEntity(
                    TEXT("core"),
                    M04CoreId,
                    Bridge->FindEntity(M04CoreId)),
                *DescribeFreshJourneyEntity(
                    TEXT("bearer"),
                    M04Bearer,
                    Bridge->FindEntity(M04Bearer)),
                *DescribeFreshJourneyEntity(
                    TEXT("waystone"),
                    M04Waystone,
                    Waystone),
                *DescribeFreshJourneyEntity(
                    TEXT("worker"),
                    M04WorkerId,
                    Bridge->FindEntity(M04WorkerId)),
                *DescribeFreshJourneyEntity(
                    TEXT("spine"), SpineId, Spine));
        };
        const auto TickUntilMissionFourPhase = [
            Bridge,
            &ObserveMissionFourProtectedState,
            &WriteMissionFourDiagnostic](
                EEchoesUnburiedRoadPhase ExpectedPhase,
                int32 MaximumTicks)
        {
            const bool bReached = TickUntil(
                Bridge,
                [Bridge,
                 ExpectedPhase,
                 &ObserveMissionFourProtectedState]()
                {
                    ObserveMissionFourProtectedState();
                    const EEchoesUnburiedRoadPhase CurrentPhase =
                        Bridge->GetUnburiedRoadPhase();
                    return CurrentPhase == ExpectedPhase ||
                        CurrentPhase == EEchoesUnburiedRoadPhase::Failed ||
                        (ExpectedPhase ==
                             EEchoesUnburiedRoadPhase::RecoverMemoryShard &&
                         CurrentPhase ==
                             EEchoesUnburiedRoadPhase::Complete);
                },
                MaximumTicks);
            const EEchoesUnburiedRoadPhase ReachedPhase =
                Bridge->GetUnburiedRoadPhase();
            if (bReached &&
                (ReachedPhase == ExpectedPhase ||
                 (ExpectedPhase ==
                      EEchoesUnburiedRoadPhase::RecoverMemoryShard &&
                  ReachedPhase ==
                      EEchoesUnburiedRoadPhase::Complete)))
            {
                return true;
            }
            WriteMissionFourDiagnostic(ExpectedPhase);
            return false;
        };
        const auto TickUntilMissionFourCondition = [
            Bridge,
            &Spec,
            &ObserveMissionFourProtectedState,
            &WriteMissionFourDiagnostic](
                const TFunction<bool()>& Predicate,
                EEchoesUnburiedRoadPhase ExpectedPhase,
                int32 MaximumTicks)
        {
            if (Spec.FoundingChoice != FutureWellChoice::Reshape)
            {
                return TickUntil(Bridge, Predicate, MaximumTicks);
            }
            const bool bStopped = TickUntil(
                Bridge,
                [Bridge,
                 &Predicate,
                 &ObserveMissionFourProtectedState]()
                {
                    ObserveMissionFourProtectedState();
                    return Bridge->GetUnburiedRoadPhase() ==
                            EEchoesUnburiedRoadPhase::Failed ||
                        Predicate();
                },
                MaximumTicks);
            ObserveMissionFourProtectedState();
            if (bStopped &&
                Bridge->GetUnburiedRoadPhase() !=
                    EEchoesUnburiedRoadPhase::Failed &&
                Predicate())
            {
                return true;
            }
            WriteMissionFourDiagnostic(ExpectedPhase);
            return false;
        };
        const auto PrepareMissionFourReshapeTactics = [
            Bridge,
            M04Route,
            M04Waystone,
            M04Bearer,
            M04CoreId,
            &M04WorkerId,
            &M04GuardIds,
            &M04GuardTargetIds,
            &M04LastKnownGuards,
            &M04FirstObservedLoss,
            &M04FirstObservedLossTick,
            &M04LastKnownCore,
            &M04LastKnownBearer,
            &M04LastKnownWaystone,
            &M04LastKnownWorker,
            &bM04TrackProtectedLoss,
            &WriteMissionFourDiagnostic,
            &Feedback]()
        {
            const echoes::sim::Simulation* Simulation =
                Bridge->GetSimulation();
            if (Simulation == nullptr)
            {
                Feedback = TEXT(
                    "[M04_TACTICS_SIM_MISSING] Mission 04 simulation is unavailable.");
                return false;
            }

            M04WorkerId = 0;
            int64 NearestWorkerDistanceSquared =
                TNumericLimits<int64>::Max();
            for (const Entity& Candidate : Simulation->Entities())
            {
                if (Candidate.owner !=
                        UEchoesSimulationSubsystem::LocalPlayerId ||
                    Candidate.type != EntityType::Worker ||
                    Candidate.hitPoints <= 0 || !Candidate.completed ||
                    Candidate.id == M04Bearer)
                {
                    continue;
                }
                const int64 DeltaX =
                    static_cast<int64>(Candidate.position.x.Raw()) -
                    M04Route.ListeningSpineSite.x.Raw();
                const int64 DeltaY =
                    static_cast<int64>(Candidate.position.y.Raw()) -
                    M04Route.ListeningSpineSite.y.Raw();
                const int64 DistanceSquared =
                    DeltaX * DeltaX + DeltaY * DeltaY;
                if (DistanceSquared < NearestWorkerDistanceSquared ||
                    (DistanceSquared == NearestWorkerDistanceSquared &&
                     (M04WorkerId == 0 || Candidate.id < M04WorkerId)))
                {
                    M04WorkerId = Candidate.id;
                    NearestWorkerDistanceSquared = DistanceSquared;
                }
            }
            if (M04WorkerId == 0)
            {
                Feedback = TEXT(
                    "[M04_TACTICS_WORKER_UNAVAILABLE] No live completed worker can raise the Listening Spine.");
                return false;
            }

            TArray<EntityId> AvailableSoldiers;
            TArray<EntityId> AvailableHeavies;
            for (const Entity& Candidate : Simulation->Entities())
            {
                if (Candidate.owner !=
                        UEchoesSimulationSubsystem::LocalPlayerId ||
                    Candidate.hitPoints <= 0 || !Candidate.completed ||
                    Candidate.id == M04Waystone ||
                    Candidate.id == M04WorkerId ||
                    Candidate.id == M04Bearer)
                {
                    continue;
                }
                if (Candidate.type == EntityType::Soldier)
                {
                    AvailableSoldiers.Add(Candidate.id);
                }
                else if (Candidate.type == EntityType::HeavyUnit)
                {
                    AvailableHeavies.Add(Candidate.id);
                }
            }
            AvailableSoldiers.Sort();
            AvailableHeavies.Sort();
            if (AvailableSoldiers.Num() < 3 || AvailableHeavies.IsEmpty())
            {
                Feedback = FString::Printf(
                    TEXT("[M04_TACTICS_GUARDS_UNAVAILABLE] Requires three Soldiers and one Heavy; soldiers=%d heavies=%d."),
                    AvailableSoldiers.Num(),
                    AvailableHeavies.Num());
                return false;
            }

            const auto TakeNearestGuard = [Bridge](
                TArray<EntityId>& Available,
                EntityId TargetId)
            {
                const Entity* Target = Bridge->FindEntity(TargetId);
                int32 BestIndex = INDEX_NONE;
                int64 BestDistanceSquared = TNumericLimits<int64>::Max();
                if (Target == nullptr)
                {
                    return EntityId{0};
                }
                for (int32 Index = 0; Index < Available.Num(); ++Index)
                {
                    const Entity* Guard =
                        Bridge->FindEntity(Available[Index]);
                    if (Guard == nullptr || Guard->hitPoints <= 0)
                    {
                        continue;
                    }
                    const int64 DeltaX =
                        static_cast<int64>(Guard->position.x.Raw()) -
                        Target->position.x.Raw();
                    const int64 DeltaY =
                        static_cast<int64>(Guard->position.y.Raw()) -
                        Target->position.y.Raw();
                    const int64 DistanceSquared =
                        DeltaX * DeltaX + DeltaY * DeltaY;
                    if (DistanceSquared < BestDistanceSquared ||
                        (DistanceSquared == BestDistanceSquared &&
                         (BestIndex == INDEX_NONE ||
                          Available[Index] < Available[BestIndex])))
                    {
                        BestIndex = Index;
                        BestDistanceSquared = DistanceSquared;
                    }
                }
                if (BestIndex == INDEX_NONE)
                {
                    return EntityId{0};
                }
                const EntityId Selected = Available[BestIndex];
                Available.RemoveAt(BestIndex);
                return Selected;
            };

            const EntityId BearerSoldier =
                TakeNearestGuard(AvailableSoldiers, M04Bearer);
            const EntityId WorkerSoldier =
                TakeNearestGuard(AvailableSoldiers, M04WorkerId);
            const EntityId WaystoneSoldier =
                TakeNearestGuard(AvailableSoldiers, M04Waystone);
            const EntityId BearerHeavy =
                TakeNearestGuard(AvailableHeavies, M04Bearer);
            if (BearerSoldier == 0 || WorkerSoldier == 0 ||
                WaystoneSoldier == 0 || BearerHeavy == 0)
            {
                Feedback = TEXT(
                    "[M04_TACTICS_ASSIGNMENT_FAILED] A protected target or viable guard disappeared before assignment.");
                return false;
            }

            const EntityId GuardActors[] = {
                BearerSoldier,
                BearerHeavy,
                WorkerSoldier,
                WaystoneSoldier};
            const EntityId GuardTargets[] = {
                M04Bearer,
                M04Bearer,
                M04WorkerId,
                M04Waystone};
            M04GuardIds.Reset();
            M04GuardTargetIds.Reset();
            for (int32 Index = 0; Index < UE_ARRAY_COUNT(GuardActors); ++Index)
            {
                M04GuardIds.Add(GuardActors[Index]);
                M04GuardTargetIds.Add(GuardTargets[Index]);
            }

            M04FirstObservedLoss = TEXT("none");
            M04FirstObservedLossTick = 0;
            M04LastKnownCore = DescribeFreshJourneyEntity(
                TEXT("core"), M04CoreId, Bridge->FindEntity(M04CoreId));
            M04LastKnownBearer = DescribeFreshJourneyEntity(
                TEXT("bearer"),
                M04Bearer,
                Bridge->FindEntity(M04Bearer));
            M04LastKnownWaystone = DescribeFreshJourneyEntity(
                TEXT("waystone"),
                M04Waystone,
                Bridge->FindEntity(M04Waystone));
            M04LastKnownWorker = DescribeFreshJourneyEntity(
                TEXT("worker"),
                M04WorkerId,
                Bridge->FindEntity(M04WorkerId));
            M04LastKnownGuards.Reset();
            for (int32 GuardIndex = 0;
                 GuardIndex < M04GuardIds.Num();
                 ++GuardIndex)
            {
                const FString GuardLabel = FString::Printf(
                    TEXT("guard%d"), GuardIndex + 1);
                M04LastKnownGuards.Add(DescribeFreshJourneyEntity(
                    *GuardLabel,
                    M04GuardIds[GuardIndex],
                    Bridge->FindEntity(M04GuardIds[GuardIndex])));
            }
            bM04TrackProtectedLoss = true;
            for (int32 Index = 0; Index < M04GuardIds.Num(); ++Index)
            {
                const Entity* Target =
                    Bridge->FindEntity(M04GuardTargetIds[Index]);
                if (Target == nullptr)
                {
                    Feedback = FString::Printf(
                        TEXT("[M04_TACTICS_GUARD_TARGET_MISSING] Guard target %u disappeared before command %d."),
                        M04GuardTargetIds[Index],
                        Index + 1);
                    WriteMissionFourDiagnostic(
                        EEchoesUnburiedRoadPhase::EstablishRoadhead);
                    return false;
                }
                if (!Bridge->IssueCommand(
                        CommandType::Guard,
                        M04GuardIds[Index],
                        M04GuardTargetIds[Index],
                        Bridge->SimToWorld(Target->position),
                        FutureWellChoice::Dormant,
                        Feedback))
                {
                    WriteMissionFourDiagnostic(
                        EEchoesUnburiedRoadPhase::EstablishRoadhead);
                    return false;
                }
            }
            return true;
        };
        const auto MissionFourGuardsActive = [
            Bridge,
            &M04GuardIds,
            &M04GuardTargetIds]()
        {
            if (M04GuardIds.Num() != 4 ||
                M04GuardTargetIds.Num() != M04GuardIds.Num())
            {
                return false;
            }
            for (int32 Index = 0; Index < M04GuardIds.Num(); ++Index)
            {
                const Entity* Guard =
                    Bridge->FindEntity(M04GuardIds[Index]);
                if (Guard == nullptr || Guard->hitPoints <= 0 ||
                    Guard->order.type != echoes::sim::OrderType::Guard ||
                    Guard->order.target != M04GuardTargetIds[Index])
                {
                    return false;
                }
            }
            return true;
        };
        const auto AcceptMissionFourCommand = [
            &Spec,
            &WriteMissionFourDiagnostic](
                bool bAccepted,
                EEchoesUnburiedRoadPhase ExpectedPhase)
        {
            if (!bAccepted &&
                Spec.FoundingChoice == FutureWellChoice::Reshape)
            {
                WriteMissionFourDiagnostic(ExpectedPhase);
            }
            return bAccepted;
        };
        if (!Require(
                !M04Workers.IsEmpty(),
                TEXT("Mission 04 exposes a construction worker")) ||
            !Require(
                M04Route.Roadhead ==
                        TestOwnedUnburiedRoadRoadhead(
                            Spec.FoundingChoice) &&
                    M04Route.ListeningSpineSite ==
                        TestOwnedUnburiedRoadSpineSite(
                            Spec.FoundingChoice) &&
                    M04Route.MemoryShardSite ==
                        TestOwnedUnburiedRoadShardSite(
                            Spec.FoundingChoice),
                TEXT("Mission 04 binds the independently expected route")) ||
            !Require(
                M04Simulation != nullptr &&
                    M04Simulation->IsPositionPassable(
                        M04Route.Roadhead) &&
                    M04Simulation->IsPositionPassable(
                        M04Route.ListeningSpineSite) &&
                    M04Simulation->IsPositionPassable(
                        M04Route.MemoryShardSite) &&
                    M04Simulation->ValidatePlacement(
                        UEchoesSimulationSubsystem::LocalPlayerId,
                        EntityType::UtilityStructure,
                        M04Route.ListeningSpineSite) ==
                        echoes::sim::PlacementResult::Valid,
                TEXT("Mission 04 route is open and buildable")) ||
            !Require(
                M04OpponentCombatDistanceSquaredRaw !=
                        TNumericLimits<int64>::Max() &&
                    M04OpponentCombatDistanceSquaredRaw >
                        M04MinimumOpponentSeparationRaw *
                        M04MinimumOpponentSeparationRaw,
                TEXT("Mission 04 construction begins outside the opposing approach")) ||
            !Require(
                M04SpineEntityDistanceSquaredRaw !=
                        TNumericLimits<int64>::Max() &&
                    M04ShardEntityDistanceSquaredRaw !=
                        TNumericLimits<int64>::Max() &&
                    M04SpineEntityDistanceSquaredRaw >
                        M04MissionDomainClearanceRaw *
                        M04MissionDomainClearanceRaw &&
                    M04ShardEntityDistanceSquaredRaw >
                        M04MissionDomainClearanceRaw *
                        M04MissionDomainClearanceRaw,
                TEXT("Mission 04 Spine and shard domains exclude spawned entities")) ||
            !Require(
                Spec.FoundingChoice != FutureWellChoice::Reshape ||
                    PrepareMissionFourReshapeTactics(),
                TEXT("Mission 04 Reshape assigns ordinary protected-actor tactics")) ||
            !Require(
                AcceptMissionFourCommand(
                    Bridge->IssueCommand(
                        CommandType::ToggleWaystoneRoot,
                        M04Waystone,
                        0,
                        FVector::ZeroVector,
                        FutureWellChoice::Dormant,
                        Feedback),
                    EEchoesUnburiedRoadPhase::EstablishRoadhead),
                TEXT("Mission 04 Waystone accepts uproot")) ||
            !Require(
                TickUntilMissionFourCondition(
                    [Bridge, M04Waystone]()
                    {
                        const Entity* Current =
                            Bridge->FindEntity(M04Waystone);
                        return Current != nullptr &&
                            Current->waystoneMode ==
                                echoes::sim::WaystoneMode::Mobile;
                    },
                    EEchoesUnburiedRoadPhase::EstablishRoadhead,
                    300),
                TEXT("Mission 04 Waystone becomes mobile")) ||
            !Require(
                Spec.FoundingChoice != FutureWellChoice::Reshape ||
                    TickUntilMissionFourCondition(
                        [&MissionFourGuardsActive]()
                        {
                            return MissionFourGuardsActive();
                        },
                        EEchoesUnburiedRoadPhase::EstablishRoadhead,
                        20),
                TEXT("Mission 04 Reshape Guard assignments take effect")) ||
            !Require(
                AcceptMissionFourCommand(
                    Move(M04Waystone, M04Route.Roadhead),
                    EEchoesUnburiedRoadPhase::EstablishRoadhead),
                TEXT("Mission 04 Waystone accepts the roadhead route")) ||
            !Require(
                TickUntilMissionFourCondition(
                    [Bridge, M04Waystone, M04Route]()
                    {
                        return IsAtSite(
                            Bridge, M04Waystone, M04Route.Roadhead);
                    },
                    EEchoesUnburiedRoadPhase::EstablishRoadhead,
                    2600),
                TEXT("Mission 04 Waystone reaches the roadhead")) ||
            !Require(
                AcceptMissionFourCommand(
                    Bridge->IssueCommand(
                        CommandType::ToggleWaystoneRoot,
                        M04Waystone,
                        0,
                        FVector::ZeroVector,
                        FutureWellChoice::Dormant,
                        Feedback),
                    EEchoesUnburiedRoadPhase::RaiseListeningSpine),
                TEXT("Mission 04 Waystone accepts root")) ||
            !Require(
                TickUntilMissionFourCondition(
                    [Bridge]()
                    {
                        return Bridge->GetUnburiedRoadPhase() ==
                            EEchoesUnburiedRoadPhase::RaiseListeningSpine;
                    },
                    EEchoesUnburiedRoadPhase::RaiseListeningSpine,
                    400),
                TEXT("Mission 04 opens Listening Spine construction")) ||
            !Require(
                AcceptMissionFourCommand(
                    Bridge->IssueBuildCommand(
                        M04WorkerId,
                        EntityType::UtilityStructure,
                        Bridge->SimToWorld(
                            M04Route.ListeningSpineSite),
                        Feedback),
                    EEchoesUnburiedRoadPhase::RaiseListeningSpine),
                TEXT("Mission 04 worker accepts the Listening Spine")))
        {
            return false;
        }
        if (Spec.FoundingChoice == FutureWellChoice::Reshape)
        {
            if (!Require(
                    AcceptMissionFourCommand(
                        Move(M04Bearer, M04Route.MemoryShardSite),
                        EEchoesUnburiedRoadPhase::Complete),
                    TEXT("Mission 04 Reshape bearer begins concurrent guarded shard recovery")) ||
                !Require(
                    TickUntilMissionFourPhase(
                        EEchoesUnburiedRoadPhase::Complete,
                        3200),
                    TEXT("Mission 04 Reshape completes through guarded ordinary play")))
            {
                return false;
            }
        }
        else if (!Require(
                     TickUntilMissionFourPhase(
                         EEchoesUnburiedRoadPhase::RecoverMemoryShard,
                         3200),
                     TEXT("Mission 04 opens shard recovery")) ||
                 !Require(
                     AcceptMissionFourCommand(
                         Move(M04Bearer, M04Route.MemoryShardSite),
                         EEchoesUnburiedRoadPhase::RecoverMemoryShard),
                     TEXT("Mission 04 bearer accepts shard recovery")) ||
                 !Require(
                     TickUntilMissionFourPhase(
                         EEchoesUnburiedRoadPhase::Complete,
                         3200),
                     TEXT("Mission 04 completes through ordinary play")))
        {
            return false;
        }
        if (!VerifyCompletion(4, EEchoesCampaignCommitStatus::Added) ||
            !AdvanceToNextMission(4))
        {
            return false;
        }

        // Mission 05: Terms of Continuance.
        const FEchoesTermsOfContinuancePlan M05Plan =
            Bridge->GetTermsOfContinuancePlan();
        Vec2 M05ExpectedMeridianRelaySite;
        Vec2 M05ExpectedKharuunSpineSite;
        Vec2 M05ExpectedWitnessExtractionSite;
        TArray<Vec2> M05ExpectedPlayerPowerLinkSites;
        TArray<Vec2> M05ExpectedSeedPowerLinkSites;
        switch (Spec.FoundingChoice)
        {
            case FutureWellChoice::Harvest:
                M05ExpectedMeridianRelaySite =
                    Vec2::FromTiles(14, 27);
                M05ExpectedKharuunSpineSite =
                    Vec2::FromTiles(14, 39);
                M05ExpectedWitnessExtractionSite =
                    Vec2::FromTiles(20, 47);
                M05ExpectedPlayerPowerLinkSites = {
                    Vec2::FromTiles(19, 21),
                    Vec2::FromTiles(17, 28),
                    Vec2::FromTiles(15, 34)};
                M05ExpectedSeedPowerLinkSites = {
                    Vec2::FromTiles(18, 10),
                    Vec2::FromTiles(24, 15),
                    Vec2::FromTiles(29, 20),
                    Vec2::FromTiles(29, 36),
                    Vec2::FromTiles(29, 40)};
                break;
            case FutureWellChoice::Preserve:
                M05ExpectedMeridianRelaySite =
                    Vec2::FromTiles(32, 27);
                M05ExpectedKharuunSpineSite =
                    Vec2::FromTiles(32, 39);
                M05ExpectedWitnessExtractionSite =
                    Vec2::FromTiles(32, 47);
                M05ExpectedPlayerPowerLinkSites = {
                    Vec2::FromTiles(29, 28)};
                M05ExpectedSeedPowerLinkSites = {
                    Vec2::FromTiles(18, 10),
                    Vec2::FromTiles(24, 15),
                    Vec2::FromTiles(29, 20),
                    Vec2::FromTiles(29, 36),
                    Vec2::FromTiles(29, 40)};
                break;
            case FutureWellChoice::Reshape:
                M05ExpectedMeridianRelaySite =
                    Vec2::FromTiles(50, 27);
                M05ExpectedKharuunSpineSite =
                    Vec2::FromTiles(44, 38);
                M05ExpectedWitnessExtractionSite =
                    Vec2::FromTiles(44, 47);
                M05ExpectedPlayerPowerLinkSites = {
                    Vec2::FromTiles(18, 10)};
                M05ExpectedSeedPowerLinkSites = {
                    Vec2::FromTiles(24, 15),
                    Vec2::FromTiles(30, 20),
                    Vec2::FromTiles(37, 23),
                    Vec2::FromTiles(44, 26),
                    Vec2::FromTiles(49, 32)};
                break;
            default:
                break;
        }
        if (!Require(
                M05Plan.PriorChoice == Spec.FoundingChoice &&
                    M05Plan.MeridianRelaySite ==
                        M05ExpectedMeridianRelaySite &&
                    M05Plan.KharuunSpineSite ==
                        M05ExpectedKharuunSpineSite &&
                    M05Plan.WitnessExtractionSite ==
                        M05ExpectedWitnessExtractionSite &&
                    M05Plan.PlayerPowerLinkSites ==
                        M05ExpectedPlayerPowerLinkSites &&
                    M05Plan.SeedPowerLinkSites ==
                        M05ExpectedSeedPowerLinkSites,
                TEXT("Mission 05 plan matches independent branch literals")))
        {
            return false;
        }
        const FEchoesObjectiveSnapshot M05StartSnapshot =
            Bridge->GetLocalObjectiveSnapshot();
        const TArray<EntityId> M05Workers =
            FindOwnedEntities(Bridge, EntityType::Worker);
        const TArray<EntityId> M05Cores =
            FindOwnedEntities(Bridge, EntityType::CommandCore);
        const EntityId M05CoreId =
            M05Cores.IsEmpty() ? 0 : M05Cores[0];
        EntityId M05MeridianRelayId =
            M05StartSnapshot.MeridianContinuanceRelayId;
        EntityId M05KharuunSpineId =
            M05StartSnapshot.KharuunContinuanceSpineId;
        EntityId M05MeridianWitnessId =
            M05StartSnapshot.MeridianContinuanceWitnessId;
        EntityId M05KharuunWitnessId =
            M05StartSnapshot.KharuunContinuanceWitnessId;
        TArray<EntityId> M05SelectedWorkerIds;
        TArray<Vec2> M05SelectedBuildSites;
        TArray<EntityId> M05SelectedSiteEntityIds;
        TArray<FString> M05LastKnownWorkers;
        TArray<FString> M05LastKnownSites;
        TArray<EntityId> M05SeedEntityIds;
        TArray<FString> M05LastKnownSeeds;
        FString M05FirstObservedLoss = TEXT("none");
        uint64 M05FirstObservedLossTick = 0;
        const auto DescribeMissionFiveEntity = [Bridge](
            const TCHAR* Label,
            EntityId Id)
        {
            const Entity* Current = Bridge->FindEntity(Id);
            if (Current == nullptr)
            {
                return FString::Printf(
                    TEXT("%s{id=%u alive=false hp=-1/-1 pos=(-1,-1) aegisPowered=unknown completed=unknown constructionProgress=-1/-1 order=255 target=0 destination=(-1,-1)}"),
                    Label,
                    Id);
            }
            return FString::Printf(
                TEXT("%s{id=%u alive=%s hp=%d/%d pos=(%d,%d) aegisPowered=%s completed=%s constructionProgress=%d/%d order=%u target=%u destination=(%d,%d)}"),
                Label,
                Id,
                Current->hitPoints > 0 ? TEXT("true") : TEXT("false"),
                Current->hitPoints,
                Current->maxHitPoints,
                Current->position.x.FloorToInt(),
                Current->position.y.FloorToInt(),
                Current->aegisPowered ? TEXT("true") : TEXT("false"),
                Current->completed ? TEXT("true") : TEXT("false"),
                Current->constructionProgress,
                Current->constructionRequired,
                static_cast<uint8>(Current->order.type),
                Current->order.target,
                Current->order.destination.x.FloorToInt(),
                Current->order.destination.y.FloorToInt());
        };
        const echoes::sim::Simulation* M05StartSimulation =
            Bridge->GetSimulation();
        for (int32 SeedIndex = 0;
             SeedIndex < M05Plan.SeedPowerLinkSites.Num();
             ++SeedIndex)
        {
            EntityId SeedId = 0;
            if (M05StartSimulation != nullptr)
            {
                for (const Entity& Candidate :
                     M05StartSimulation->Entities())
                {
                    if (Candidate.owner ==
                            UEchoesSimulationSubsystem::LocalPlayerId &&
                        Candidate.faction ==
                            echoes::sim::Faction::MeridianCompact &&
                        Candidate.type == EntityType::Dropoff &&
                        Candidate.position ==
                            M05Plan.SeedPowerLinkSites[SeedIndex])
                    {
                        SeedId = Candidate.id;
                        break;
                    }
                }
            }
            M05SeedEntityIds.Add(SeedId);
            const FString SeedLabel = FString::Printf(
                TEXT("seed%d"), SeedIndex + 1);
            M05LastKnownSeeds.Add(DescribeMissionFiveEntity(
                *SeedLabel, SeedId));
            if (SeedId == 0 && M05FirstObservedLoss == TEXT("none"))
            {
                M05FirstObservedLossTick =
                    M05StartSimulation != nullptr
                        ? M05StartSimulation->CurrentTick()
                        : 0;
                M05FirstObservedLoss = FString::Printf(
                    TEXT("%s{id=0 observed=%s lastKnown=%s}"),
                    *SeedLabel,
                    *DescribeMissionFiveEntity(*SeedLabel, 0),
                    *M05LastKnownSeeds.Last());
            }
        }
        FString M05LastKnownCore = DescribeMissionFiveEntity(
            TEXT("core"), M05CoreId);
        FString M05LastKnownMeridianRelay = DescribeMissionFiveEntity(
            TEXT("meridianRelay"), M05MeridianRelayId);
        FString M05LastKnownKharuunSpine = DescribeMissionFiveEntity(
            TEXT("kharuunSpine"), M05KharuunSpineId);
        FString M05LastKnownMeridianWitness = DescribeMissionFiveEntity(
            TEXT("meridianWitness"), M05MeridianWitnessId);
        FString M05LastKnownKharuunWitness = DescribeMissionFiveEntity(
            TEXT("kharuunWitness"), M05KharuunWitnessId);
        const auto ObserveMissionFiveState = [
            Bridge,
            M05CoreId,
            &M05MeridianRelayId,
            &M05KharuunSpineId,
            &M05MeridianWitnessId,
            &M05KharuunWitnessId,
            &M05SelectedWorkerIds,
            &M05SelectedBuildSites,
            &M05SelectedSiteEntityIds,
            &M05LastKnownWorkers,
            &M05LastKnownSites,
            &M05SeedEntityIds,
            &M05LastKnownSeeds,
            &M05FirstObservedLoss,
            &M05FirstObservedLossTick,
            &M05LastKnownCore,
            &M05LastKnownMeridianRelay,
            &M05LastKnownKharuunSpine,
            &M05LastKnownMeridianWitness,
            &M05LastKnownKharuunWitness,
            &DescribeMissionFiveEntity]()
        {
            const FEchoesObjectiveSnapshot Snapshot =
                Bridge->GetLocalObjectiveSnapshot();
            const auto RefreshMissionEntityId = [
                &DescribeMissionFiveEntity](
                    EntityId SnapshotId,
                    EntityId& TrackedId,
                    const TCHAR* Label,
                    FString& LastKnown)
            {
                if (TrackedId == 0 && SnapshotId != 0)
                {
                    TrackedId = SnapshotId;
                    LastKnown = DescribeMissionFiveEntity(
                        Label, TrackedId);
                }
            };
            RefreshMissionEntityId(
                Snapshot.MeridianContinuanceRelayId,
                M05MeridianRelayId,
                TEXT("meridianRelay"),
                M05LastKnownMeridianRelay);
            RefreshMissionEntityId(
                Snapshot.KharuunContinuanceSpineId,
                M05KharuunSpineId,
                TEXT("kharuunSpine"),
                M05LastKnownKharuunSpine);
            RefreshMissionEntityId(
                Snapshot.MeridianContinuanceWitnessId,
                M05MeridianWitnessId,
                TEXT("meridianWitness"),
                M05LastKnownMeridianWitness);
            RefreshMissionEntityId(
                Snapshot.KharuunContinuanceWitnessId,
                M05KharuunWitnessId,
                TEXT("kharuunWitness"),
                M05LastKnownKharuunWitness);

            const auto ObserveTrackedEntity = [
                Bridge,
                &M05FirstObservedLoss,
                &M05FirstObservedLossTick,
                &DescribeMissionFiveEntity](
                    const TCHAR* Label,
                    EntityId Id,
                    FString& LastKnown)
            {
                if (Id == 0)
                {
                    return;
                }
                const Entity* Current = Bridge->FindEntity(Id);
                if (Current != nullptr && Current->hitPoints > 0)
                {
                    LastKnown = DescribeMissionFiveEntity(Label, Id);
                    return;
                }
                if (M05FirstObservedLoss == TEXT("none"))
                {
                    const echoes::sim::Simulation* Simulation =
                        Bridge->GetSimulation();
                    M05FirstObservedLossTick =
                        Simulation != nullptr
                            ? Simulation->CurrentTick()
                            : 0;
                    M05FirstObservedLoss = FString::Printf(
                        TEXT("%s{id=%u observed=%s lastKnown=%s}"),
                        Label,
                        Id,
                        *DescribeMissionFiveEntity(Label, Id),
                        *LastKnown);
                }
            };
            ObserveTrackedEntity(
                TEXT("core"), M05CoreId, M05LastKnownCore);
            ObserveTrackedEntity(
                TEXT("meridianRelay"),
                M05MeridianRelayId,
                M05LastKnownMeridianRelay);
            ObserveTrackedEntity(
                TEXT("kharuunSpine"),
                M05KharuunSpineId,
                M05LastKnownKharuunSpine);
            ObserveTrackedEntity(
                TEXT("meridianWitness"),
                M05MeridianWitnessId,
                M05LastKnownMeridianWitness);
            ObserveTrackedEntity(
                TEXT("kharuunWitness"),
                M05KharuunWitnessId,
                M05LastKnownKharuunWitness);
            for (int32 SeedIndex = 0;
                 SeedIndex < M05SeedEntityIds.Num();
                 ++SeedIndex)
            {
                const FString SeedLabel = FString::Printf(
                    TEXT("seed%d"), SeedIndex + 1);
                ObserveTrackedEntity(
                    *SeedLabel,
                    M05SeedEntityIds[SeedIndex],
                    M05LastKnownSeeds[SeedIndex]);
            }

            const echoes::sim::Simulation* Simulation =
                Bridge->GetSimulation();
            for (int32 Index = 0;
                 Index < M05SelectedWorkerIds.Num();
                 ++Index)
            {
                const FString WorkerLabel = FString::Printf(
                    TEXT("worker%d"), Index + 1);
                ObserveTrackedEntity(
                    *WorkerLabel,
                    M05SelectedWorkerIds[Index],
                    M05LastKnownWorkers[Index]);

                if (M05SelectedSiteEntityIds[Index] == 0 &&
                    Simulation != nullptr)
                {
                    for (const Entity& Candidate : Simulation->Entities())
                    {
                    if (Candidate.owner ==
                            UEchoesSimulationSubsystem::LocalPlayerId &&
                        Candidate.faction ==
                            echoes::sim::Faction::MeridianCompact &&
                        Candidate.type == EntityType::Dropoff &&
                            Candidate.position ==
                                M05SelectedBuildSites[Index] &&
                            Candidate.hitPoints > 0)
                        {
                            M05SelectedSiteEntityIds[Index] = Candidate.id;
                            break;
                        }
                    }
                }
                if (M05SelectedSiteEntityIds[Index] != 0)
                {
                    const FString SiteLabel = FString::Printf(
                        TEXT("site%d"), Index + 1);
                    ObserveTrackedEntity(
                        *SiteLabel,
                        M05SelectedSiteEntityIds[Index],
                        M05LastKnownSites[Index]);
                }
            }
        };
        const auto WriteMissionFiveDiagnostic = [
            Bridge,
            M05CoreId,
            &M05MeridianRelayId,
            &M05KharuunSpineId,
            &M05MeridianWitnessId,
            &M05KharuunWitnessId,
            &M05SelectedWorkerIds,
            &M05SelectedBuildSites,
            &M05SelectedSiteEntityIds,
            &M05LastKnownWorkers,
            &M05LastKnownSites,
            &M05SeedEntityIds,
            &M05LastKnownSeeds,
            &M05FirstObservedLoss,
            &M05FirstObservedLossTick,
            &M05LastKnownCore,
            &M05LastKnownMeridianRelay,
            &M05LastKnownKharuunSpine,
            &M05LastKnownMeridianWitness,
            &M05LastKnownKharuunWitness,
            &DescribeMissionFiveEntity,
            &ObserveMissionFiveState,
            &Feedback,
            M05Plan](EEchoesTermsOfContinuancePhase ExpectedPhase)
        {
            ObserveMissionFiveState();
            FString AuthoredSeedSummary;
            FString LastKnownSeedSummary;
            FString CurrentSeedSummary;
            FString SelectedBuildSummary;
            FString LastKnownWorkerSummary;
            FString LastKnownSiteSummary;
            FString CurrentWorkerSummary;
            FString CurrentSiteSummary;
            const auto AppendSummary = [](
                FString& Summary,
                const FString& Item)
            {
                if (!Summary.IsEmpty())
                {
                    Summary += TEXT(" | ");
                }
                Summary += Item;
            };
            for (int32 SeedIndex = 0;
                 SeedIndex < M05SeedEntityIds.Num();
                 ++SeedIndex)
            {
                const FString SeedLabel = FString::Printf(
                    TEXT("seed%d"), SeedIndex + 1);
                AppendSummary(
                    AuthoredSeedSummary,
                    FString::Printf(
                        TEXT("seed%d{site=(%d,%d) entityId=%u requiredAegisPowered=false}"),
                        SeedIndex + 1,
                        M05Plan.SeedPowerLinkSites[SeedIndex].x.FloorToInt(),
                        M05Plan.SeedPowerLinkSites[SeedIndex].y.FloorToInt(),
                        M05SeedEntityIds[SeedIndex]));
                AppendSummary(
                    LastKnownSeedSummary,
                    M05LastKnownSeeds[SeedIndex]);
                AppendSummary(
                    CurrentSeedSummary,
                    DescribeMissionFiveEntity(
                        *SeedLabel,
                        M05SeedEntityIds[SeedIndex]));
            }
            if (AuthoredSeedSummary.IsEmpty())
            {
                AuthoredSeedSummary = TEXT("none");
                LastKnownSeedSummary = TEXT("none");
                CurrentSeedSummary = TEXT("none");
            }
            for (int32 Index = 0;
                 Index < M05SelectedWorkerIds.Num();
                 ++Index)
            {
                AppendSummary(
                    SelectedBuildSummary,
                    FString::Printf(
                        TEXT("build%d{workerId=%u site=(%d,%d) siteEntityId=%u}"),
                        Index + 1,
                        M05SelectedWorkerIds[Index],
                        M05SelectedBuildSites[Index].x.FloorToInt(),
                        M05SelectedBuildSites[Index].y.FloorToInt(),
                        M05SelectedSiteEntityIds[Index]));
                AppendSummary(
                    LastKnownWorkerSummary,
                    M05LastKnownWorkers[Index]);
                AppendSummary(
                    LastKnownSiteSummary,
                    M05LastKnownSites[Index]);
                const FString WorkerLabel = FString::Printf(
                    TEXT("worker%d"), Index + 1);
                AppendSummary(
                    CurrentWorkerSummary,
                    DescribeMissionFiveEntity(
                        *WorkerLabel,
                        M05SelectedWorkerIds[Index]));
                const FString SiteLabel = FString::Printf(
                    TEXT("site%d"), Index + 1);
                AppendSummary(
                    CurrentSiteSummary,
                    FString::Printf(
                        TEXT("selected=(%d,%d) %s"),
                        M05SelectedBuildSites[Index].x.FloorToInt(),
                        M05SelectedBuildSites[Index].y.FloorToInt(),
                        *DescribeMissionFiveEntity(
                            *SiteLabel,
                            M05SelectedSiteEntityIds[Index])));
            }
            if (SelectedBuildSummary.IsEmpty())
            {
                SelectedBuildSummary = TEXT("none");
                LastKnownWorkerSummary = TEXT("none");
                LastKnownSiteSummary = TEXT("none");
                CurrentWorkerSummary = TEXT("none");
                CurrentSiteSummary = TEXT("none");
            }
            const FString LastKnownSummary = FString::Printf(
                TEXT("%s %s %s %s %s seeds=[%s] workers=[%s] sites=[%s]"),
                *M05LastKnownCore,
                *M05LastKnownMeridianRelay,
                *M05LastKnownKharuunSpine,
                *M05LastKnownMeridianWitness,
                *M05LastKnownKharuunWitness,
                *LastKnownSeedSummary,
                *LastKnownWorkerSummary,
                *LastKnownSiteSummary);
            const FString CurrentSummary = FString::Printf(
                TEXT("%s %s %s %s %s seeds=[%s] workers=[%s] sites=[%s]"),
                *DescribeMissionFiveEntity(TEXT("core"), M05CoreId),
                *DescribeMissionFiveEntity(
                    TEXT("meridianRelay"), M05MeridianRelayId),
                *DescribeMissionFiveEntity(
                    TEXT("kharuunSpine"), M05KharuunSpineId),
                *DescribeMissionFiveEntity(
                    TEXT("meridianWitness"), M05MeridianWitnessId),
                *DescribeMissionFiveEntity(
                    TEXT("kharuunWitness"), M05KharuunWitnessId),
                *CurrentSeedSummary,
                *CurrentWorkerSummary,
                *CurrentSiteSummary);
            const FString PriorFeedback =
                Feedback.IsEmpty() ? TEXT("none") : Feedback;
            const echoes::sim::Simulation* Simulation =
                Bridge->GetSimulation();
            Feedback = FString::Printf(
                TEXT("[M05_DIAGNOSTIC] tick=%llu outcome=%u phase=%u expectedPhase=%u priorFeedback=%s firstObservedLossTick=%llu firstObservedLoss=%s authoredSeeds=[%s] selectedBuilds=[%s] lastKnown={%s} current={%s}"),
                static_cast<unsigned long long>(
                    Simulation != nullptr ? Simulation->CurrentTick() : 0),
                Simulation != nullptr
                    ? static_cast<uint8>(Simulation->Outcome())
                    : 0xFF,
                static_cast<uint8>(
                    Bridge->GetTermsOfContinuancePhase()),
                static_cast<uint8>(ExpectedPhase),
                *PriorFeedback,
                static_cast<unsigned long long>(
                    M05FirstObservedLossTick),
                *M05FirstObservedLoss,
                *AuthoredSeedSummary,
                *SelectedBuildSummary,
                *LastKnownSummary,
                *CurrentSummary);
        };
        const auto AreMissionFiveSeedsLivingCompletedUnpowered = [
            Bridge,
            &M05SeedEntityIds,
            M05Plan]()
        {
            if (M05SeedEntityIds.Num() !=
                    M05Plan.SeedPowerLinkSites.Num() ||
                M05SeedEntityIds.IsEmpty())
            {
                return false;
            }
            for (int32 SeedIndex = 0;
                 SeedIndex < M05SeedEntityIds.Num();
                 ++SeedIndex)
            {
                const Entity* Seed = Bridge->FindEntity(
                    M05SeedEntityIds[SeedIndex]);
                if (Seed == nullptr ||
                    Seed->owner !=
                        UEchoesSimulationSubsystem::LocalPlayerId ||
                    Seed->faction !=
                        echoes::sim::Faction::MeridianCompact ||
                    Seed->type != EntityType::Dropoff ||
                    Seed->position !=
                        M05Plan.SeedPowerLinkSites[SeedIndex] ||
                    Seed->hitPoints <= 0 || !Seed->completed ||
                    Seed->aegisPowered)
                {
                    return false;
                }
            }
            return true;
        };
        const auto AreMissionFivePlayerLinksLivingCompletedUnpowered = [
            Bridge,
            M05Plan]()
        {
            const echoes::sim::Simulation* Simulation =
                Bridge->GetSimulation();
            if (Simulation == nullptr ||
                M05Plan.PlayerPowerLinkSites.IsEmpty())
            {
                return false;
            }
            for (const Vec2& PlayerSite :
                 M05Plan.PlayerPowerLinkSites)
            {
                bool bPlayerLinkReady = false;
                for (const Entity& Candidate :
                     Simulation->Entities())
                {
                    if (Candidate.owner ==
                            UEchoesSimulationSubsystem::LocalPlayerId &&
                        Candidate.faction ==
                            echoes::sim::Faction::MeridianCompact &&
                        Candidate.type == EntityType::Dropoff &&
                        Candidate.position == PlayerSite &&
                        Candidate.hitPoints > 0 &&
                        Candidate.completed &&
                        !Candidate.aegisPowered)
                    {
                        bPlayerLinkReady = true;
                        break;
                    }
                }
                if (!bPlayerLinkReady)
                {
                    return false;
                }
            }
            return true;
        };
        const auto AreMissionFiveInterfacesLivingCompletedPowered = [
            Bridge,
            M05Plan]()
        {
            const FEchoesObjectiveSnapshot Snapshot =
                Bridge->GetLocalObjectiveSnapshot();
            const Entity* MeridianRelay = Bridge->FindEntity(
                Snapshot.MeridianContinuanceRelayId);
            const Entity* KharuunSpine = Bridge->FindEntity(
                Snapshot.KharuunContinuanceSpineId);
            const auto IsReadyInterface = [](
                const Entity* Interface,
                const Vec2& Site)
            {
                return Interface != nullptr &&
                    Interface->owner ==
                        UEchoesSimulationSubsystem::LocalPlayerId &&
                    Interface->faction ==
                        echoes::sim::Faction::MeridianCompact &&
                    Interface->type == EntityType::UtilityStructure &&
                    Interface->position == Site &&
                    Interface->hitPoints > 0 &&
                    Interface->completed &&
                    Interface->aegisPowered;
            };
            return Snapshot.bMeridianRelaySynchronized &&
                Snapshot.bKharuunSpineSynchronized &&
                IsReadyInterface(
                    MeridianRelay,
                    M05Plan.MeridianRelaySite) &&
                IsReadyInterface(
                    KharuunSpine,
                    M05Plan.KharuunSpineSite);
        };
        const auto TickUntilMissionFiveCondition = [
            Bridge,
            &AreMissionFiveSeedsLivingCompletedUnpowered,
            &ObserveMissionFiveState,
            &WriteMissionFiveDiagnostic](
                const TFunction<bool()>& Predicate,
                EEchoesTermsOfContinuancePhase ExpectedPhase,
                int32 MaximumTicks)
        {
            for (int32 TickIndex = 0;
                 TickIndex < MaximumTicks;
                 ++TickIndex)
            {
                ObserveMissionFiveState();
                if (!AreMissionFiveSeedsLivingCompletedUnpowered())
                {
                    WriteMissionFiveDiagnostic(ExpectedPhase);
                    return false;
                }
                if (Bridge->GetTermsOfContinuancePhase() ==
                    EEchoesTermsOfContinuancePhase::Failed)
                {
                    WriteMissionFiveDiagnostic(ExpectedPhase);
                    return false;
                }
                if (Predicate())
                {
                    return true;
                }
                Bridge->Tick(0.05f);
            }
            ObserveMissionFiveState();
            if (!AreMissionFiveSeedsLivingCompletedUnpowered())
            {
                WriteMissionFiveDiagnostic(ExpectedPhase);
                return false;
            }
            if (Bridge->GetTermsOfContinuancePhase() ==
                EEchoesTermsOfContinuancePhase::Failed)
            {
                WriteMissionFiveDiagnostic(ExpectedPhase);
                return false;
            }
            if (Predicate())
            {
                return true;
            }
            WriteMissionFiveDiagnostic(ExpectedPhase);
            return false;
        };
        const auto AcceptMissionFiveCommand = [
            &WriteMissionFiveDiagnostic](
                bool bAccepted,
                EEchoesTermsOfContinuancePhase ExpectedPhase)
        {
            if (!bAccepted)
            {
                WriteMissionFiveDiagnostic(ExpectedPhase);
            }
            return bAccepted;
        };
        const auto AcceptMissionFiveCondition = [
            &WriteMissionFiveDiagnostic](
                bool bAccepted,
                EEchoesTermsOfContinuancePhase ExpectedPhase)
        {
            if (!bAccepted)
            {
                WriteMissionFiveDiagnostic(ExpectedPhase);
            }
            return bAccepted;
        };
        if (!Require(
                AcceptMissionFiveCondition(
                    !M05Plan.PlayerPowerLinkSites.IsEmpty(),
                    EEchoesTermsOfContinuancePhase::SynchronizeNetworks),
                TEXT("Mission 05 exposes authored player Power Link sites")) ||
            !Require(
                AcceptMissionFiveCondition(
                    M05Workers.Num() >=
                        M05Plan.PlayerPowerLinkSites.Num(),
                    EEchoesTermsOfContinuancePhase::SynchronizeNetworks),
                TEXT("Mission 05 exposes one distinct worker per player Power Link")))
        {
            return false;
        }
        const auto FindOwnedMeridianDropoffAt = [Bridge](
            const Vec2& Site) -> const Entity*
        {
            const echoes::sim::Simulation* Simulation =
                Bridge->GetSimulation();
            if (Simulation == nullptr)
            {
                return nullptr;
            }
            for (const Entity& Candidate : Simulation->Entities())
            {
                if (Candidate.owner ==
                        UEchoesSimulationSubsystem::LocalPlayerId &&
                    Candidate.faction ==
                        echoes::sim::Faction::MeridianCompact &&
                    Candidate.type == EntityType::Dropoff &&
                    Candidate.position == Site)
                {
                    return &Candidate;
                }
            }
            return nullptr;
        };
        bool bAuthoredSeedsReady = true;
        for (const Vec2& SeedSite : M05Plan.SeedPowerLinkSites)
        {
            const Entity* Seed =
                FindOwnedMeridianDropoffAt(SeedSite);
            bAuthoredSeedsReady &= Seed != nullptr &&
                Seed->hitPoints > 0 && Seed->completed &&
                !Seed->aegisPowered;
        }
        bool bPlayerSitesOpen = true;
        for (const Vec2& PlayerSite : M05Plan.PlayerPowerLinkSites)
        {
            bPlayerSitesOpen &=
                Bridge->GetSimulation()->IsPositionPassable(PlayerSite) &&
                FindOwnedMeridianDropoffAt(PlayerSite) == nullptr;
        }
        if (!Require(
                AcceptMissionFiveCondition(
                    bAuthoredSeedsReady,
                    EEchoesTermsOfContinuancePhase::SynchronizeNetworks),
                TEXT("Mission 05 spawns every route-owned seed as a living, completed, unpowered Meridian Dropoff")) ||
            !Require(
                AcceptMissionFiveCondition(
                    bPlayerSitesOpen,
                    EEchoesTermsOfContinuancePhase::SynchronizeNetworks),
                TEXT("Mission 05 leaves every authored player Power Link site buildable")))
        {
            return false;
        }
        ObserveMissionFiveState();
        Bridge->Tick(0.05f);
        ObserveMissionFiveState();
        const auto FindNearestAvailableWorker = [
            Bridge,
            &M05Workers,
            &M05SelectedWorkerIds](const Vec2& Site)
        {
            EntityId NearestWorkerId = 0;
            int64 NearestDistanceSquared =
                TNumericLimits<int64>::Max();
            for (const EntityId WorkerId : M05Workers)
            {
                if (M05SelectedWorkerIds.Contains(WorkerId))
                {
                    continue;
                }
                const Entity* Worker = Bridge->FindEntity(WorkerId);
                if (Worker == nullptr || Worker->hitPoints <= 0 ||
                    !Worker->completed)
                {
                    continue;
                }
                const int64 DeltaX =
                    static_cast<int64>(Worker->position.x.Raw()) -
                    Site.x.Raw();
                const int64 DeltaY =
                    static_cast<int64>(Worker->position.y.Raw()) -
                    Site.y.Raw();
                const int64 DistanceSquared =
                    DeltaX * DeltaX + DeltaY * DeltaY;
                if (DistanceSquared < NearestDistanceSquared ||
                    (DistanceSquared == NearestDistanceSquared &&
                     (NearestWorkerId == 0 || WorkerId < NearestWorkerId)))
                {
                    NearestWorkerId = WorkerId;
                    NearestDistanceSquared = DistanceSquared;
                }
            }
            return NearestWorkerId;
        };
        for (int32 Index = 0;
             Index < M05Plan.PlayerPowerLinkSites.Num();
             ++Index)
        {
            const Vec2 PlayerSite =
                M05Plan.PlayerPowerLinkSites[Index];
            const EntityId WorkerId =
                FindNearestAvailableWorker(PlayerSite);
            if (!Require(
                    AcceptMissionFiveCondition(
                        WorkerId != 0,
                        EEchoesTermsOfContinuancePhase::
                            SynchronizeNetworks),
                    FString::Printf(
                        TEXT("Mission 05 player Power Link %d has a distinct nearest worker"),
                        Index + 1)))
            {
                return false;
            }
            M05SelectedWorkerIds.Add(WorkerId);
            M05SelectedBuildSites.Add(PlayerSite);
            M05SelectedSiteEntityIds.Add(0);
            M05LastKnownWorkers.Add(DescribeMissionFiveEntity(
                *FString::Printf(TEXT("worker%d"), Index + 1),
                WorkerId));
            M05LastKnownSites.Add(DescribeMissionFiveEntity(
                *FString::Printf(TEXT("site%d"), Index + 1),
                0));
            ObserveMissionFiveState();
            if (!Require(
                    AcceptMissionFiveCommand(
                        Bridge->IssueBuildCommand(
                            WorkerId,
                            EntityType::Dropoff,
                            Bridge->SimToWorld(PlayerSite),
                            Feedback),
                        EEchoesTermsOfContinuancePhase::
                            SynchronizeNetworks),
                    FString::Printf(
                        TEXT("Mission 05 player Power Link %d is accepted"),
                        Index + 1)))
            {
                return false;
            }
        }
        if (!Require(
                TickUntilMissionFiveCondition(
                    [Bridge]()
                    {
                        const FEchoesObjectiveSnapshot Snapshot =
                            Bridge->GetLocalObjectiveSnapshot();
                        return Snapshot.bMeridianRelaySynchronized &&
                            Snapshot.bKharuunSpineSynchronized &&
                            Bridge->GetTermsOfContinuancePhase() ==
                                EEchoesTermsOfContinuancePhase::
                                    HoldContinuanceWindow;
                    },
                    EEchoesTermsOfContinuancePhase::
                        HoldContinuanceWindow,
                    700),
                TEXT("Mission 05 synchronizes both treaty networks")) ||
            !Require(
                AcceptMissionFiveCondition(
                    AreMissionFiveSeedsLivingCompletedUnpowered(),
                    EEchoesTermsOfContinuancePhase::
                        HoldContinuanceWindow),
                TEXT("Mission 05 keeps every authored seed at its exact site as a living, completed, unpowered Meridian Dropoff before the treaty deadline")) ||
            !Require(
                AcceptMissionFiveCondition(
                    AreMissionFivePlayerLinksLivingCompletedUnpowered(),
                    EEchoesTermsOfContinuancePhase::
                        HoldContinuanceWindow),
                TEXT("Mission 05 keeps every player-built link at its exact site as a living, completed, unpowered Meridian Dropoff before the treaty deadline")) ||
            !Require(
                AcceptMissionFiveCondition(
                    AreMissionFiveInterfacesLivingCompletedPowered(),
                    EEchoesTermsOfContinuancePhase::
                        HoldContinuanceWindow),
                TEXT("Mission 05 keeps both treaty interfaces living, completed, and powered before the treaty deadline")) ||
            !Require(
                AcceptMissionFiveCondition(
                    Bridge->GetSimulation()->CurrentTick() <
                        M05Plan.ContinuanceWindowStartTick,
                    EEchoesTermsOfContinuancePhase::HoldContinuanceWindow),
                TEXT("Mission 05 synchronizes before the treaty deadline")) ||
            !Require(
                TickUntilMissionFiveCondition(
                    [Bridge, M05Plan]()
                    {
                        return Bridge->GetTermsOfContinuancePhase() ==
                                   EEchoesTermsOfContinuancePhase::
                                       ExtractWitnesses &&
                            Bridge->GetSimulation()->CurrentTick() >=
                                M05Plan.ContinuanceWindowEndTick;
                    },
                    EEchoesTermsOfContinuancePhase::ExtractWitnesses,
                    1000),
                TEXT("Mission 05 holds the complete continuance window")) ||
            !Require(
                AcceptMissionFiveCondition(
                    AreMissionFiveSeedsLivingCompletedUnpowered(),
                    EEchoesTermsOfContinuancePhase::ExtractWitnesses),
                TEXT("Mission 05 keeps every authored seed at its exact site as a living, completed, unpowered Meridian Dropoff through T900")) ||
            !Require(
                AcceptMissionFiveCondition(
                    AreMissionFivePlayerLinksLivingCompletedUnpowered(),
                    EEchoesTermsOfContinuancePhase::ExtractWitnesses),
                TEXT("Mission 05 keeps every player-built link at its exact site as a living, completed, unpowered Meridian Dropoff through T900")) ||
            !Require(
                AcceptMissionFiveCondition(
                    AreMissionFiveInterfacesLivingCompletedPowered(),
                    EEchoesTermsOfContinuancePhase::ExtractWitnesses),
                TEXT("Mission 05 keeps both treaty interfaces living, completed, and powered through T900")))
        {
            return false;
        }
        const FEchoesObjectiveSnapshot M05Snapshot =
            Bridge->GetLocalObjectiveSnapshot();
        if (!Require(
                AcceptMissionFiveCommand(
                    Move(
                        M05Snapshot.MeridianContinuanceWitnessId,
                        M05Plan.WitnessExtractionSite),
                    EEchoesTermsOfContinuancePhase::ExtractWitnesses),
                TEXT("Mission 05 Meridian witness accepts extraction")) ||
            !Require(
                AcceptMissionFiveCommand(
                    Move(
                        M05Snapshot.KharuunContinuanceWitnessId,
                        M05Plan.WitnessExtractionSite),
                    EEchoesTermsOfContinuancePhase::ExtractWitnesses),
                TEXT("Mission 05 Kharuun witness accepts extraction")) ||
            !Require(
                TickUntilMissionFiveCondition(
                    [Bridge]()
                    {
                        return Bridge->GetTermsOfContinuancePhase() ==
                            EEchoesTermsOfContinuancePhase::Complete;
                    },
                    EEchoesTermsOfContinuancePhase::Complete,
                    1000),
                TEXT("Mission 05 completes through ordinary play")))
        {
            return false;
        }
        if (!VerifyCompletion(5, EEchoesCampaignCommitStatus::Added) ||
            !AdvanceToNextMission(5))
        {
            return false;
        }
        return true;
    };

    const auto RunMissionsSixThroughTen = [
        Bridge, &Feedback, &Require, &VerifyCompletion,
        &AdvanceToNextMission, &PreserveQuickSaveFamily](
        const FFreshRouteSpec& Spec)
    {
        const auto Move = [Bridge, &Feedback](EntityId Actor, const Vec2& Site)
        {
            return Bridge->IssueCommand(
                CommandType::Move,
                Actor,
                0,
                Bridge->SimToWorld(Site),
                FutureWellChoice::Dormant,
                Feedback);
        };

        // Mission 06: Names Without Births.
        const FEchoesNamesWithoutBirthsPlan M06Plan =
            Bridge->GetNamesWithoutBirthsPlan();
        const FEchoesObjectiveSnapshot M06Start =
            Bridge->GetLocalObjectiveSnapshot();
        const TArray<EntityId> M06Cores =
            FindOwnedEntities(Bridge, EntityType::CommandCore);
        const EntityId M06CoreId =
            M06Cores.IsEmpty() ? 0 : M06Cores[0];
        FString M06FirstObservedProtectedLoss = TEXT("none");
        uint64 M06FirstObservedProtectedLossTick = 0;
        const auto ObserveMissionSixProtectedState = [
            Bridge,
            M06CoreId,
            M06Start,
            &M06FirstObservedProtectedLoss,
            &M06FirstObservedProtectedLossTick]()
        {
            const auto ObserveProtected = [
                Bridge,
                &M06FirstObservedProtectedLoss,
                &M06FirstObservedProtectedLossTick](
                    const TCHAR* Label,
                    EntityId Id)
            {
                if (M06FirstObservedProtectedLoss != TEXT("none"))
                {
                    return;
                }
                const Entity* Current = Bridge->FindEntity(Id);
                if (Id != 0 && Current != nullptr &&
                    Current->hitPoints > 0)
                {
                    return;
                }
                const echoes::sim::Simulation* Simulation =
                    Bridge->GetSimulation();
                M06FirstObservedProtectedLossTick =
                    Simulation != nullptr
                        ? Simulation->CurrentTick()
                        : 0;
                M06FirstObservedProtectedLoss =
                    DescribeFreshJourneyEntity(Label, Id, Current);
            };
            ObserveProtected(TEXT("core"), M06CoreId);
            ObserveProtected(TEXT("talar"), M06Start.TalarId);
            ObserveProtected(
                TEXT("archive"), M06Start.CensusArchiveId);
            ObserveProtected(
                TEXT("civilianA"), M06Start.FirstCivilianId);
            ObserveProtected(
                TEXT("civilianB"), M06Start.SecondCivilianId);
        };
        const auto WriteMissionSixDiagnostic = [
            Bridge,
            M06CoreId,
            M06Plan,
            M06Start,
            &M06FirstObservedProtectedLoss,
            &M06FirstObservedProtectedLossTick,
            &ObserveMissionSixProtectedState,
            &Feedback]()
        {
            ObserveMissionSixProtectedState();
            const echoes::sim::Simulation* Simulation =
                Bridge->GetSimulation();
            const FEchoesObjectiveSnapshot Current =
                Bridge->GetLocalObjectiveSnapshot();
            const FString PriorFeedback =
                Feedback.IsEmpty() ? TEXT("none") : Feedback;
            Feedback = FString::Printf(
                TEXT("[M06_DIAGNOSTIC] tick=%llu outcome=%u phase=%u archivePowered=%s expectedShelter=(%d,%d) firstObservedProtectedLossTick=%llu firstObservedProtectedLoss=%s priorFeedback=%s current={%s %s %s %s %s}"),
                static_cast<unsigned long long>(
                    Simulation != nullptr
                        ? Simulation->CurrentTick()
                        : 0),
                Simulation != nullptr
                    ? static_cast<uint8>(Simulation->Outcome())
                    : 0xFF,
                static_cast<uint8>(
                    Bridge->GetNamesWithoutBirthsPhase()),
                Current.bCensusArchivePowered
                    ? TEXT("true")
                    : TEXT("false"),
                M06Plan.CivilianShelterSite.x.FloorToInt(),
                M06Plan.CivilianShelterSite.y.FloorToInt(),
                static_cast<unsigned long long>(
                    M06FirstObservedProtectedLossTick),
                *M06FirstObservedProtectedLoss,
                *PriorFeedback,
                *DescribeFreshJourneyEntity(
                    TEXT("core"),
                    M06CoreId,
                    Bridge->FindEntity(M06CoreId)),
                *DescribeFreshJourneyEntity(
                    TEXT("talar"),
                    M06Start.TalarId,
                    Bridge->FindEntity(M06Start.TalarId)),
                *DescribeFreshJourneyEntity(
                    TEXT("archive"),
                    M06Start.CensusArchiveId,
                    Bridge->FindEntity(M06Start.CensusArchiveId)),
                *DescribeFreshJourneyEntity(
                    TEXT("civilianA"),
                    M06Start.FirstCivilianId,
                    Bridge->FindEntity(M06Start.FirstCivilianId)),
                *DescribeFreshJourneyEntity(
                    TEXT("civilianB"),
                    M06Start.SecondCivilianId,
                    Bridge->FindEntity(M06Start.SecondCivilianId)));
        };
        const auto RequireMissionSix = [
            &Require,
            &ObserveMissionSixProtectedState,
            &WriteMissionSixDiagnostic](
                bool bCondition,
                const FString& Label)
        {
            ObserveMissionSixProtectedState();
            if (!bCondition)
            {
                WriteMissionSixDiagnostic();
            }
            return Require(bCondition, Label);
        };
        const auto TickUntilMissionSixCondition = [
            Bridge,
            &ObserveMissionSixProtectedState](
                const TFunction<bool()>& Predicate,
                int32 MaximumTicks)
        {
            for (int32 TickIndex = 0;
                 TickIndex < MaximumTicks;
                 ++TickIndex)
            {
                ObserveMissionSixProtectedState();
                if (Bridge->GetNamesWithoutBirthsPhase() ==
                    EEchoesNamesWithoutBirthsPhase::Failed)
                {
                    return false;
                }
                if (Predicate())
                {
                    return true;
                }
                Bridge->Tick(0.05f);
            }
            ObserveMissionSixProtectedState();
            return Predicate();
        };
        ObserveMissionSixProtectedState();
        Vec2 M06PowerLinkSite;
        if (!RequireMissionSix(
                M06Start.TalarId != 0 &&
                    M06Start.FirstCivilianId != 0 &&
                    M06Start.SecondCivilianId != 0,
                TEXT("Mission 06 exposes Talar and both civilians")) ||
            !RequireMissionSix(
                Move(M06Start.TalarId, M06Plan.CensusSite),
                TEXT("Mission 06 Talar accepts the census route")) ||
            !RequireMissionSix(
                TickUntilMissionSixCondition(
                    [Bridge]()
                    {
                        return Bridge->GetNamesWithoutBirthsPhase() ==
                            EEchoesNamesWithoutBirthsPhase::
                                StabilizeArchive;
                    },
                    500),
                TEXT("Mission 06 locates the inherited census")) ||
            !RequireMissionSix(
                FindValidBuildSiteForType(
                    Bridge,
                    EntityType::Dropoff,
                    M06Plan.PowerLinkSite,
                    2,
                    M06PowerLinkSite),
                TEXT("Mission 06 exposes a valid census Power Link site")) ||
            !RequireMissionSix(
                Bridge->IssueBuildCommand(
                    M06Start.FirstCivilianId,
                    EntityType::Dropoff,
                    Bridge->SimToWorld(M06PowerLinkSite),
                    Feedback),
                TEXT("Mission 06 accepts the census Power Link")) ||
            !RequireMissionSix(
                TickUntilMissionSixCondition(
                    [Bridge]()
                    {
                        return Bridge->GetNamesWithoutBirthsPhase() ==
                            EEchoesNamesWithoutBirthsPhase::
                                ShelterCivilians;
                    },
                    900),
                TEXT("Mission 06 powers the census archive")) ||
            !RequireMissionSix(
                Move(
                    M06Start.FirstCivilianId,
                    M06Plan.CivilianShelterSite),
                TEXT("Mission 06 first civilian accepts shelter")) ||
            !RequireMissionSix(
                Move(
                    M06Start.SecondCivilianId,
                    M06Plan.CivilianShelterSite),
                TEXT("Mission 06 second civilian accepts shelter")) ||
            !RequireMissionSix(
                TickUntilMissionSixCondition(
                    [Bridge]()
                    {
                        return Bridge->GetNamesWithoutBirthsPhase() ==
                            EEchoesNamesWithoutBirthsPhase::
                                ExtractEvidence;
                    },
                    1000),
                TEXT("Mission 06 shelters both civilians")) ||
            !RequireMissionSix(
                Move(M06Start.TalarId, M06Plan.EvidenceExtractionSite),
                TEXT("Mission 06 Talar accepts evidence extraction")) ||
            !RequireMissionSix(
                TickUntilMissionSixCondition(
                    [Bridge]()
                    {
                        return Bridge->GetNamesWithoutBirthsPhase() ==
                            EEchoesNamesWithoutBirthsPhase::Complete;
                    },
                    1000),
                TEXT("Mission 06 completes through ordinary play")) ||
            !VerifyCompletion(6, EEchoesCampaignCommitStatus::Added) ||
            !AdvanceToNextMission(6))
        {
            return false;
        }

        // Mission 07: The Shape of Silence.
        const FEchoesShapeOfSilencePlan M07Plan =
            Bridge->GetShapeOfSilencePlan();
        const FEchoesObjectiveSnapshot M07Start =
            Bridge->GetLocalObjectiveSnapshot();
        const TArray<EntityId> M07Workers =
            FindOwnedEntities(Bridge, EntityType::Worker);
        const TArray<EntityId> M07Cores =
            FindOwnedEntities(Bridge, EntityType::CommandCore);
        const EntityId M07CoreId =
            M07Cores.IsEmpty() ? 0 : M07Cores[0];
        const TArray<EntityId> M07Soldiers =
            FindOwnedEntities(Bridge, EntityType::Soldier);
        const TArray<EntityId> M07Heavies =
            FindOwnedEntities(Bridge, EntityType::HeavyUnit);
        TArray<EntityId> M07GuardIds;
        TArray<EntityId> M07GuardTargetIds;
        EntityId M07SpineId = 0;
        FString M07FirstObservedProtectedLoss = TEXT("none");
        uint64 M07FirstObservedProtectedLossTick = 0;
        const auto ObserveMissionSevenProtectedState = [
            Bridge,
            M07CoreId,
            M07Start,
            &M07FirstObservedProtectedLoss,
            &M07FirstObservedProtectedLossTick]()
        {
            const auto ObserveProtected = [
                Bridge,
                &M07FirstObservedProtectedLoss,
                &M07FirstObservedProtectedLossTick](
                    const TCHAR* Label,
                    EntityId Id)
            {
                if (M07FirstObservedProtectedLoss != TEXT("none"))
                {
                    return;
                }
                const Entity* Current = Bridge->FindEntity(Id);
                if (Id != 0 && Current != nullptr &&
                    Current->hitPoints > 0)
                {
                    return;
                }
                const echoes::sim::Simulation* Simulation =
                    Bridge->GetSimulation();
                M07FirstObservedProtectedLossTick =
                    Simulation != nullptr
                        ? Simulation->CurrentTick()
                        : 0;
                M07FirstObservedProtectedLoss =
                    DescribeFreshJourneyEntity(Label, Id, Current);
            };
            ObserveProtected(TEXT("core"), M07CoreId);
            ObserveProtected(
                TEXT("waystone"), M07Start.MigrationWaystoneId);
            ObserveProtected(TEXT("oruun"), M07Start.OruunId);
            ObserveProtected(
                TEXT("witnessA"), M07Start.FirstMemoryWitnessId);
            ObserveProtected(
                TEXT("witnessB"), M07Start.SecondMemoryWitnessId);
        };
        const auto WriteMissionSevenDiagnostic = [
            Bridge,
            M07CoreId,
            M07Plan,
            M07Start,
            M07Workers,
            &M07GuardIds,
            &M07GuardTargetIds,
            &M07SpineId,
            &M07FirstObservedProtectedLoss,
            &M07FirstObservedProtectedLossTick,
            &ObserveMissionSevenProtectedState,
            &Feedback]()
        {
            ObserveMissionSevenProtectedState();
            const echoes::sim::Simulation* Simulation =
                Bridge->GetSimulation();
            if (M07SpineId == 0 && Simulation != nullptr)
            {
                for (const Entity& Candidate : Simulation->Entities())
                {
                    if (Candidate.owner ==
                            UEchoesSimulationSubsystem::LocalPlayerId &&
                        Candidate.type == EntityType::UtilityStructure &&
                        Candidate.position == M07Plan.ListeningSpineSite)
                    {
                        M07SpineId = Candidate.id;
                        break;
                    }
                }
            }
            FString GuardState = TEXT("none");
            if (!M07GuardIds.IsEmpty())
            {
                GuardState.Reset();
                for (int32 GuardIndex = 0;
                     GuardIndex < M07GuardIds.Num();
                     ++GuardIndex)
                {
                    if (GuardIndex > 0)
                    {
                        GuardState += TEXT(" ");
                    }
                    const FString GuardLabel = FString::Printf(
                        TEXT("guard%d"), GuardIndex + 1);
                    GuardState += DescribeFreshJourneyEntity(
                        *GuardLabel,
                        M07GuardIds[GuardIndex],
                        Bridge->FindEntity(M07GuardIds[GuardIndex]));
                    if (M07GuardTargetIds.IsValidIndex(GuardIndex))
                    {
                        GuardState += FString::Printf(
                            TEXT(" expectedTarget=%u"),
                            M07GuardTargetIds[GuardIndex]);
                    }
                }
            }
            const EntityId WorkerId =
                M07Workers.IsEmpty() ? 0 : M07Workers[0];
            const FString PriorFeedback =
                Feedback.IsEmpty() ? TEXT("none") : Feedback;
            Feedback = FString::Printf(
                TEXT("[M07_DIAGNOSTIC] tick=%llu outcome=%u phase=%u expected={anchor=(%d,%d) spine=(%d,%d) witnesses=(%d,%d):(%d,%d) confluence=(%d,%d)} firstObservedProtectedLossTick=%llu firstObservedProtectedLoss=%s priorFeedback=%s current={%s %s %s %s %s %s %s} defenders={%s}"),
                static_cast<unsigned long long>(
                    Simulation != nullptr
                        ? Simulation->CurrentTick()
                        : 0),
                Simulation != nullptr
                    ? static_cast<uint8>(Simulation->Outcome())
                    : 0xFF,
                static_cast<uint8>(Bridge->GetShapeOfSilencePhase()),
                M07Plan.WaystoneAnchor.x.FloorToInt(),
                M07Plan.WaystoneAnchor.y.FloorToInt(),
                M07Plan.ListeningSpineSite.x.FloorToInt(),
                M07Plan.ListeningSpineSite.y.FloorToInt(),
                M07Plan.FirstWitnessSite.x.FloorToInt(),
                M07Plan.FirstWitnessSite.y.FloorToInt(),
                M07Plan.SecondWitnessSite.x.FloorToInt(),
                M07Plan.SecondWitnessSite.y.FloorToInt(),
                M07Plan.ConfluenceSite.x.FloorToInt(),
                M07Plan.ConfluenceSite.y.FloorToInt(),
                static_cast<unsigned long long>(
                    M07FirstObservedProtectedLossTick),
                *M07FirstObservedProtectedLoss,
                *PriorFeedback,
                *DescribeFreshJourneyEntity(
                    TEXT("core"),
                    M07CoreId,
                    Bridge->FindEntity(M07CoreId)),
                *DescribeFreshJourneyEntity(
                    TEXT("waystone"),
                    M07Start.MigrationWaystoneId,
                    Bridge->FindEntity(M07Start.MigrationWaystoneId)),
                *DescribeFreshJourneyEntity(
                    TEXT("oruun"),
                    M07Start.OruunId,
                    Bridge->FindEntity(M07Start.OruunId)),
                *DescribeFreshJourneyEntity(
                    TEXT("witnessA"),
                    M07Start.FirstMemoryWitnessId,
                    Bridge->FindEntity(M07Start.FirstMemoryWitnessId)),
                *DescribeFreshJourneyEntity(
                    TEXT("witnessB"),
                    M07Start.SecondMemoryWitnessId,
                    Bridge->FindEntity(M07Start.SecondMemoryWitnessId)),
                *DescribeFreshJourneyEntity(
                    TEXT("worker"),
                    WorkerId,
                    Bridge->FindEntity(WorkerId)),
                *DescribeFreshJourneyEntity(
                    TEXT("spine"),
                    M07SpineId,
                    Bridge->FindEntity(M07SpineId)),
                *GuardState);
        };
        const auto RequireMissionSeven = [
            &Require,
            &ObserveMissionSevenProtectedState,
            &WriteMissionSevenDiagnostic](
                bool bCondition,
                const FString& Label)
        {
            ObserveMissionSevenProtectedState();
            if (!bCondition)
            {
                WriteMissionSevenDiagnostic();
            }
            return Require(bCondition, Label);
        };
        const auto TickUntilMissionSevenCondition = [
            Bridge,
            &ObserveMissionSevenProtectedState](
                const TFunction<bool()>& Predicate,
                int32 MaximumTicks)
        {
            for (int32 TickIndex = 0;
                 TickIndex < MaximumTicks;
                 ++TickIndex)
            {
                ObserveMissionSevenProtectedState();
                if (Bridge->GetShapeOfSilencePhase() ==
                    EEchoesShapeOfSilencePhase::Failed)
                {
                    return false;
                }
                if (Predicate())
                {
                    return true;
                }
                Bridge->Tick(0.05f);
            }
            ObserveMissionSevenProtectedState();
            return Predicate();
        };
        const auto IssueMissionSevenGuardAssignments = [
            Bridge,
            &M07GuardIds,
            &M07GuardTargetIds,
            &Feedback]()
        {
            if (M07GuardIds.Num() != 4 ||
                M07GuardTargetIds.Num() != M07GuardIds.Num())
            {
                Feedback = TEXT(
                    "[M07_TACTICS_ASSIGNMENT_INVALID] Mission 07 requires four exact Guard assignments.");
                return false;
            }
            for (int32 GuardIndex = 0;
                 GuardIndex < M07GuardIds.Num();
                 ++GuardIndex)
            {
                const Entity* Target = Bridge->FindEntity(
                    M07GuardTargetIds[GuardIndex]);
                if (Target == nullptr || Target->hitPoints <= 0 ||
                    !Bridge->IssueCommand(
                        CommandType::Guard,
                        M07GuardIds[GuardIndex],
                        M07GuardTargetIds[GuardIndex],
                        Bridge->SimToWorld(Target->position),
                        FutureWellChoice::Dormant,
                        Feedback))
                {
                    return false;
                }
            }
            return true;
        };
        const auto PrepareMissionSevenGuards = [
            M07Start,
            M07Workers,
            M07Soldiers,
            M07Heavies,
            &M07GuardIds,
            &M07GuardTargetIds,
            &IssueMissionSevenGuardAssignments,
            &Feedback]()
        {
            if (M07Workers.IsEmpty() || M07Soldiers.Num() < 3 ||
                M07Heavies.IsEmpty())
            {
                Feedback = FString::Printf(
                    TEXT("[M07_TACTICS_GUARDS_UNAVAILABLE] Requires one worker, three Soldiers, and one Heavy; workers=%d soldiers=%d heavies=%d."),
                    M07Workers.Num(),
                    M07Soldiers.Num(),
                    M07Heavies.Num());
                return false;
            }
            const EntityId GuardActors[] = {
                M07Soldiers[0],
                M07Soldiers[1],
                M07Soldiers[2],
                M07Heavies[0]};
            const EntityId GuardTargets[] = {
                M07Workers[0],
                M07Workers[0],
                M07Start.SecondMemoryWitnessId,
                M07Start.SecondMemoryWitnessId};
            M07GuardIds.Reset();
            M07GuardTargetIds.Reset();
            for (int32 GuardIndex = 0;
                 GuardIndex < UE_ARRAY_COUNT(GuardActors);
                 ++GuardIndex)
            {
                M07GuardIds.Add(GuardActors[GuardIndex]);
                M07GuardTargetIds.Add(GuardTargets[GuardIndex]);
            }
            return IssueMissionSevenGuardAssignments();
        };
        const auto MissionSevenGuardsActive = [
            Bridge,
            &M07GuardIds,
            &M07GuardTargetIds]()
        {
            if (M07GuardIds.Num() != 4 ||
                M07GuardTargetIds.Num() != M07GuardIds.Num())
            {
                return false;
            }
            for (int32 GuardIndex = 0;
                 GuardIndex < M07GuardIds.Num();
                 ++GuardIndex)
            {
                const Entity* Guard = Bridge->FindEntity(
                    M07GuardIds[GuardIndex]);
                if (Guard == nullptr || Guard->hitPoints <= 0 ||
                    Guard->order.type != echoes::sim::OrderType::Guard ||
                    Guard->order.target !=
                        M07GuardTargetIds[GuardIndex])
                {
                    return false;
                }
            }
            return true;
        };
        const auto RetargetMissionSevenSpineGuard = [
            Bridge,
            M07Plan,
            &M07SpineId,
            &M07GuardTargetIds,
            &IssueMissionSevenGuardAssignments,
            &Feedback]()
        {
            const echoes::sim::Simulation* Simulation =
                Bridge->GetSimulation();
            if (Simulation == nullptr)
            {
                Feedback = TEXT(
                    "[M07_TACTICS_SIM_MISSING] Mission 07 simulation is unavailable.");
                return false;
            }
            for (const Entity& Candidate : Simulation->Entities())
            {
                if (Candidate.owner ==
                        UEchoesSimulationSubsystem::LocalPlayerId &&
                    Candidate.type == EntityType::UtilityStructure &&
                    Candidate.completed && Candidate.hitPoints > 0 &&
                    Candidate.position == M07Plan.ListeningSpineSite)
                {
                    M07SpineId = Candidate.id;
                    break;
                }
            }
            if (M07SpineId == 0 || M07GuardTargetIds.Num() != 4)
            {
                Feedback = TEXT(
                    "[M07_TACTICS_SPINE_UNAVAILABLE] The completed Listening Spine is unavailable for Guard retargeting.");
                return false;
            }
            M07GuardTargetIds[0] = M07SpineId;
            return IssueMissionSevenGuardAssignments();
        };
        ObserveMissionSevenProtectedState();
        if (!RequireMissionSeven(
                M07Start.MigrationWaystoneId != 0 &&
                    M07Start.OruunId != 0 &&
                    !M07Workers.IsEmpty(),
                TEXT("Mission 07 exposes its Waystone, Oruun, and worker")) ||
            !RequireMissionSeven(
                PrepareMissionSevenGuards(),
                TEXT("Mission 07 assigns ordinary Guard tactics")) ||
            !RequireMissionSeven(
                TickUntilMissionSevenCondition(
                    [&MissionSevenGuardsActive]()
                    {
                        return MissionSevenGuardsActive();
                    },
                    20),
                TEXT("Mission 07 initial Guard assignments take effect")) ||
            !RequireMissionSeven(
                Bridge->IssueCommand(
                    CommandType::ToggleWaystoneRoot,
                    M07Start.MigrationWaystoneId,
                    0,
                    FVector::ZeroVector,
                    FutureWellChoice::Dormant,
                    Feedback),
                TEXT("Mission 07 Waystone accepts uproot")) ||
            !RequireMissionSeven(
                TickUntilMissionSevenCondition(
                    [Bridge, M07Start]()
                    {
                        const Entity* Current = Bridge->FindEntity(
                            M07Start.MigrationWaystoneId);
                        return Current != nullptr &&
                            Current->waystoneMode ==
                                echoes::sim::WaystoneMode::Mobile;
                    },
                    300),
                TEXT("Mission 07 Waystone becomes mobile")) ||
            !RequireMissionSeven(
                Move(
                    M07Start.MigrationWaystoneId,
                    M07Plan.WaystoneAnchor),
                TEXT("Mission 07 Waystone accepts its listening route")) ||
            !RequireMissionSeven(
                TickUntilMissionSevenCondition(
                    [Bridge, M07Start, M07Plan]()
                    {
                        return IsAtSite(
                            Bridge,
                            M07Start.MigrationWaystoneId,
                            M07Plan.WaystoneAnchor);
                    },
                    2600),
                TEXT("Mission 07 Waystone reaches its anchor")) ||
            !RequireMissionSeven(
                Bridge->IssueCommand(
                    CommandType::ToggleWaystoneRoot,
                    M07Start.MigrationWaystoneId,
                    0,
                    FVector::ZeroVector,
                    FutureWellChoice::Dormant,
                    Feedback),
                TEXT("Mission 07 Waystone accepts root")) ||
            !RequireMissionSeven(
                TickUntilMissionSevenCondition(
                    [Bridge]()
                    {
                        return Bridge->GetShapeOfSilencePhase() ==
                            EEchoesShapeOfSilencePhase::
                                RaiseListeningSpine;
                    },
                    400),
                TEXT("Mission 07 opens Listening Spine construction")) ||
            !RequireMissionSeven(
                Bridge->IssueBuildCommand(
                    M07Workers[0],
                    EntityType::UtilityStructure,
                    Bridge->SimToWorld(M07Plan.ListeningSpineSite),
                    Feedback),
                TEXT("Mission 07 accepts its Listening Spine")) ||
            !RequireMissionSeven(
                TickUntilMissionSevenCondition(
                    [Bridge]()
                    {
                        return Bridge->GetShapeOfSilencePhase() ==
                            EEchoesShapeOfSilencePhase::
                                PositionMemoryWitnesses;
                    },
                    3200),
                TEXT("Mission 07 opens paired witnessing")) ||
            !RequireMissionSeven(
                RetargetMissionSevenSpineGuard(),
                TEXT("Mission 07 retargets one escort to the Listening Spine")) ||
            !RequireMissionSeven(
                TickUntilMissionSevenCondition(
                    [&MissionSevenGuardsActive]()
                    {
                        return MissionSevenGuardsActive();
                    },
                    20),
                TEXT("Mission 07 Spine and protected-actor Guards take effect")) ||
            !RequireMissionSeven(
                Move(
                    M07Start.FirstMemoryWitnessId,
                    M07Plan.FirstWitnessSite),
                TEXT("Mission 07 first witness accepts its site")) ||
            !RequireMissionSeven(
                Move(
                    M07Start.SecondMemoryWitnessId,
                    M07Plan.SecondWitnessSite),
                TEXT("Mission 07 second witness accepts its site")) ||
            !RequireMissionSeven(
                TickUntilMissionSevenCondition(
                    [Bridge]()
                    {
                        return Bridge->GetShapeOfSilencePhase() ==
                            EEchoesShapeOfSilencePhase::ReachConfluence;
                    },
                    3400),
                TEXT("Mission 07 positions both witnesses")) ||
            !RequireMissionSeven(
                Move(M07Start.OruunId, M07Plan.ConfluenceSite),
                TEXT("Mission 07 Oruun accepts the confluence route")) ||
            !RequireMissionSeven(
                TickUntilMissionSevenCondition(
                    [Bridge]()
                    {
                        return Bridge->GetShapeOfSilencePhase() ==
                            EEchoesShapeOfSilencePhase::Complete;
                    },
                    3400),
                TEXT("Mission 07 completes through guarded ordinary play")) ||
            !VerifyCompletion(7, EEchoesCampaignCommitStatus::Added) ||
            !AdvanceToNextMission(7))
        {
            return false;
        }

        // Mission 08: The Shape Beside Us.
        const FEchoesShapeBesideUsPlan M08Plan =
            Bridge->GetShapeBesideUsPlan();
        const FEchoesObjectiveSnapshot M08Start =
            Bridge->GetLocalObjectiveSnapshot();
        EntityId M08Worker = 0;
        for (const EntityId Candidate :
             FindOwnedEntities(Bridge, EntityType::Worker))
        {
            if (Candidate != M08Start.FirstStateWitnessId)
            {
                M08Worker = Candidate;
                break;
            }
        }
        const TArray<EntityId> M08Cores =
            FindOwnedEntities(Bridge, EntityType::CommandCore);
        const EntityId M08CoreId =
            M08Cores.IsEmpty() ? 0 : M08Cores[0];
        TArray<EntityId> M08Soldiers;
        for (const EntityId Candidate :
             FindOwnedEntities(Bridge, EntityType::Soldier))
        {
            if (Candidate != M08Start.SecondStateWitnessId)
            {
                M08Soldiers.Add(Candidate);
            }
        }
        const TArray<EntityId> M08Heavies =
            FindOwnedEntities(Bridge, EntityType::HeavyUnit);
        TArray<EntityId> M08GuardIds;
        TArray<EntityId> M08GuardTargetIds;
        EntityId M08RelayId = 0;
        FString M08FirstObservedFailure = TEXT("none");
        uint64 M08FirstObservedFailureTick = 0;
        // Convoy pacing state. Both budgets are the pre-existing Mission 08
        // wait allowances (3000 ticks to reach RaiseEchoRelay, 3400 ticks to
        // open paired-state traversal); every new wait in the escort tactic
        // draws from one of them, so the mission's total tick allowance is
        // unchanged.
        const int32 M08ConvoyIncrementRaw = 2 * echoes::sim::kFixedScale;
        const int32 M08ConvoyReformRadiusTiles = 2;
        const int32 M08WorkerHoldStandoffRaw =
            (3 * echoes::sim::kFixedScale) / 2;
        int32 M08ApproachBudgetTicks = 3000;
        int32 M08RelayBudgetTicks = 3400;
        int32 M08TraversalBudgetTicks = 3600;
        int32 M08CompletionBudgetTicks = 3600;
        int32 M08TalarConvoyIncrements = 0;
        int32 M08WorkerConvoyIncrements = 0;
        int32 M08WitnessConvoyIncrements = 0;
        const TArray<int32> M08TalarEscortIndices = {0, 1, 2, 3};
        const TArray<int32> M08WorkerEscortIndices = {0, 1};
        const TArray<int32> M08WitnessAEscortIndices = {1};
        const TArray<int32> M08WitnessBEscortIndices = {2};
        const TArray<int32> M08ConvergenceEscortIndices = {3};
        const TArray<int32> M08NoEscortIndices;
        // Vision-safe stand displacement: final stands sit 2.5 tiles from
        // their plan site, chosen so the stand tile lies outside every live
        // hostile's own vision disc (standing inside one is a permanent
        // aggro pull — the opposing AI attacks any visible enemy without a
        // range bound, which is what killed witnessB at its raw site).
        // Stand legs arrive with the pacer's slack window (standoff one
        // eighth tile + one eighth slack), so the worst-case actor-to-site
        // distance is 2.5 + 0.25 = 2.75 tiles — inside the 3-tile
        // mission-fact radius with margin, and no computed stand ever
        // demands a raw-exact landing.
        const int32 M08StandOffsetRaw = (5 * echoes::sim::kFixedScale) / 2;
        FString M08StandTelemetry;
        // Leg-objective early success: a paced leg completes the moment the
        // mission-level objective it serves is already satisfied. Wired only
        // for the convergence leg — reaching the convergence fact ring
        // completes the mission, which presents the result and PAUSES the
        // simulation, so further pacing is both unnecessary and impossible.
        const TFunction<bool()> M08NoLegObjective;
        const TFunction<bool()> M08ConvergenceObjectiveMet = [Bridge]()
        {
            return Bridge->GetShapeBesideUsPhase() ==
                EEchoesShapeBesideUsPhase::Complete;
        };
        EEchoesShapeBesideUsPhase M08LastNonFailedPhase =
            Bridge->GetShapeBesideUsPhase();
        FString M08LastKnownCore = DescribeFreshJourneyEntity(
            TEXT("core"), M08CoreId, Bridge->FindEntity(M08CoreId));
        FString M08LastKnownTalar = DescribeFreshJourneyEntity(
            TEXT("talar"),
            M08Start.ShapeBesideUsTalarId,
            Bridge->FindEntity(M08Start.ShapeBesideUsTalarId));
        FString M08LastKnownWitnessA = DescribeFreshJourneyEntity(
            TEXT("witnessA"),
            M08Start.FirstStateWitnessId,
            Bridge->FindEntity(M08Start.FirstStateWitnessId));
        FString M08LastKnownWitnessB = DescribeFreshJourneyEntity(
            TEXT("witnessB"),
            M08Start.SecondStateWitnessId,
            Bridge->FindEntity(M08Start.SecondStateWitnessId));
        FString M08LastKnownWorker = DescribeFreshJourneyEntity(
            TEXT("worker"), M08Worker, Bridge->FindEntity(M08Worker));
        FString M08LastKnownRelay = TEXT("relay{id=0 unobserved}");
        const auto ObserveMissionEightProtectedState = [
            Bridge,
            M08CoreId,
            M08Plan,
            M08Start,
            M08Worker,
            &M08RelayId,
            &M08FirstObservedFailure,
            &M08FirstObservedFailureTick,
            &M08LastNonFailedPhase,
            &M08LastKnownCore,
            &M08LastKnownTalar,
            &M08LastKnownWitnessA,
            &M08LastKnownWitnessB,
            &M08LastKnownWorker,
            &M08LastKnownRelay]()
        {
            const echoes::sim::Simulation* Simulation =
                Bridge->GetSimulation();
            const auto UpdateLastKnown = [Bridge](
                const TCHAR* Label,
                EntityId Id,
                FString& LastKnown)
            {
                const Entity* Current = Bridge->FindEntity(Id);
                if (Current != nullptr)
                {
                    LastKnown = DescribeFreshJourneyEntity(
                        Label, Id, Current);
                }
            };
            UpdateLastKnown(
                TEXT("core"), M08CoreId, M08LastKnownCore);
            UpdateLastKnown(
                TEXT("talar"),
                M08Start.ShapeBesideUsTalarId,
                M08LastKnownTalar);
            UpdateLastKnown(
                TEXT("witnessA"),
                M08Start.FirstStateWitnessId,
                M08LastKnownWitnessA);
            UpdateLastKnown(
                TEXT("witnessB"),
                M08Start.SecondStateWitnessId,
                M08LastKnownWitnessB);
            UpdateLastKnown(
                TEXT("worker"), M08Worker, M08LastKnownWorker);
            if (M08RelayId == 0 && Simulation != nullptr)
            {
                for (const Entity& Candidate : Simulation->Entities())
                {
                    if (Candidate.owner ==
                            UEchoesSimulationSubsystem::LocalPlayerId &&
                        Candidate.type == EntityType::UtilityStructure &&
                        Candidate.hitPoints > 0 &&
                        Candidate.position == M08Plan.EchoRelaySite)
                    {
                        M08RelayId = Candidate.id;
                        break;
                    }
                }
            }
            if (M08RelayId != 0)
            {
                UpdateLastKnown(
                    TEXT("relay"), M08RelayId, M08LastKnownRelay);
            }

            const EEchoesShapeBesideUsPhase CurrentPhase =
                Bridge->GetShapeBesideUsPhase();
            if (CurrentPhase != EEchoesShapeBesideUsPhase::Failed)
            {
                M08LastNonFailedPhase = CurrentPhase;
            }
            if (M08FirstObservedFailure != TEXT("none"))
            {
                return;
            }
            const auto ObserveProtected = [
                Bridge,
                Simulation,
                &M08FirstObservedFailure,
                &M08FirstObservedFailureTick](
                    const TCHAR* Label,
                    EntityId Id)
            {
                if (M08FirstObservedFailure != TEXT("none"))
                {
                    return;
                }
                const Entity* Current = Bridge->FindEntity(Id);
                if (Id != 0 && Current != nullptr &&
                    Current->hitPoints > 0)
                {
                    return;
                }
                M08FirstObservedFailureTick =
                    Simulation != nullptr
                        ? Simulation->CurrentTick()
                        : 0;
                M08FirstObservedFailure =
                    DescribeFreshJourneyEntity(Label, Id, Current);
            };
            ObserveProtected(TEXT("core"), M08CoreId);
            ObserveProtected(
                TEXT("talar"), M08Start.ShapeBesideUsTalarId);
            ObserveProtected(
                TEXT("witnessA"), M08Start.FirstStateWitnessId);
            ObserveProtected(
                TEXT("witnessB"), M08Start.SecondStateWitnessId);
            if (M08FirstObservedFailure == TEXT("none") &&
                Simulation != nullptr &&
                Simulation->Outcome() !=
                    echoes::sim::MatchOutcome::Ongoing)
            {
                M08FirstObservedFailureTick = Simulation->CurrentTick();
                M08FirstObservedFailure = FString::Printf(
                    TEXT("matchOutcome{value=%u}"),
                    static_cast<uint8>(Simulation->Outcome()));
            }
        };
        const auto WriteMissionEightDiagnostic = [
            Bridge,
            M08CoreId,
            M08Plan,
            M08Start,
            M08Worker,
            &M08RelayId,
            &M08FirstObservedFailure,
            &M08FirstObservedFailureTick,
            &M08LastNonFailedPhase,
            &M08LastKnownCore,
            &M08LastKnownTalar,
            &M08LastKnownWitnessA,
            &M08LastKnownWitnessB,
            &M08LastKnownWorker,
            &M08LastKnownRelay,
            &M08GuardIds,
            &M08GuardTargetIds,
            &M08ApproachBudgetTicks,
            &M08RelayBudgetTicks,
            &M08TraversalBudgetTicks,
            &M08CompletionBudgetTicks,
            &M08TalarConvoyIncrements,
            &M08WorkerConvoyIncrements,
            &M08WitnessConvoyIncrements,
            &M08StandTelemetry,
            &ObserveMissionEightProtectedState,
            &Feedback]()
        {
            ObserveMissionEightProtectedState();
            const echoes::sim::Simulation* Simulation =
                Bridge->GetSimulation();
            const FString PriorFeedback =
                Feedback.IsEmpty() ? TEXT("none") : Feedback;
            FString DefenderState = TEXT("none");
            if (!M08GuardIds.IsEmpty())
            {
                DefenderState.Reset();
                for (int32 GuardIndex = 0;
                     GuardIndex < M08GuardIds.Num();
                     ++GuardIndex)
                {
                    if (GuardIndex > 0)
                    {
                        DefenderState += TEXT(" ");
                    }
                    const FString GuardLabel = FString::Printf(
                        TEXT("guard%d"), GuardIndex + 1);
                    DefenderState += DescribeFreshJourneyEntity(
                        *GuardLabel,
                        M08GuardIds[GuardIndex],
                        Bridge->FindEntity(M08GuardIds[GuardIndex]));
                    if (M08GuardTargetIds.IsValidIndex(GuardIndex))
                    {
                        DefenderState += FString::Printf(
                            TEXT(" expectedTarget=%u"),
                            M08GuardTargetIds[GuardIndex]);
                    }
                }
            }
            Feedback = FString::Printf(
                TEXT("[M08_DIAGNOSTIC] tick=%llu checksum=%llu outcome=%u phase=%u lastNonFailedPhase=%u expected={firstEcho=(%d,%d) relay=(%d,%d) witnesses=(%d,%d):(%d,%d) convergence=(%d,%d)} firstObservedFailureTick=%llu firstObservedFailure=%s priorFeedback=%s lastKnown={%s %s %s %s %s %s} current={%s %s %s %s %s %s} convoy={talarIncrements=%d workerIncrements=%d witnessIncrements=%d approachBudget=%d relayBudget=%d traversalBudget=%d completionBudget=%d} stands={%s} defenders={%s}"),
                static_cast<unsigned long long>(
                    Simulation != nullptr
                        ? Simulation->CurrentTick()
                        : 0),
                static_cast<unsigned long long>(
                    Simulation != nullptr
                        ? Simulation->StateChecksum()
                        : 0),
                Simulation != nullptr
                    ? static_cast<uint8>(Simulation->Outcome())
                    : 0xFF,
                static_cast<uint8>(Bridge->GetShapeBesideUsPhase()),
                static_cast<uint8>(M08LastNonFailedPhase),
                M08Plan.FirstEchoSite.x.FloorToInt(),
                M08Plan.FirstEchoSite.y.FloorToInt(),
                M08Plan.EchoRelaySite.x.FloorToInt(),
                M08Plan.EchoRelaySite.y.FloorToInt(),
                M08Plan.FirstStateSite.x.FloorToInt(),
                M08Plan.FirstStateSite.y.FloorToInt(),
                M08Plan.SecondStateSite.x.FloorToInt(),
                M08Plan.SecondStateSite.y.FloorToInt(),
                M08Plan.ConvergenceSite.x.FloorToInt(),
                M08Plan.ConvergenceSite.y.FloorToInt(),
                static_cast<unsigned long long>(
                    M08FirstObservedFailureTick),
                *M08FirstObservedFailure,
                *PriorFeedback,
                *M08LastKnownCore,
                *M08LastKnownTalar,
                *M08LastKnownWitnessA,
                *M08LastKnownWitnessB,
                *M08LastKnownWorker,
                *M08LastKnownRelay,
                *DescribeFreshJourneyEntity(
                    TEXT("core"),
                    M08CoreId,
                    Bridge->FindEntity(M08CoreId)),
                *DescribeFreshJourneyEntity(
                    TEXT("talar"),
                    M08Start.ShapeBesideUsTalarId,
                    Bridge->FindEntity(M08Start.ShapeBesideUsTalarId)),
                *DescribeFreshJourneyEntity(
                    TEXT("witnessA"),
                    M08Start.FirstStateWitnessId,
                    Bridge->FindEntity(M08Start.FirstStateWitnessId)),
                *DescribeFreshJourneyEntity(
                    TEXT("witnessB"),
                    M08Start.SecondStateWitnessId,
                    Bridge->FindEntity(M08Start.SecondStateWitnessId)),
                *DescribeFreshJourneyEntity(
                    TEXT("worker"),
                    M08Worker,
                    Bridge->FindEntity(M08Worker)),
                *DescribeFreshJourneyEntity(
                    TEXT("relay"),
                    M08RelayId,
                    Bridge->FindEntity(M08RelayId)),
                M08TalarConvoyIncrements,
                M08WorkerConvoyIncrements,
                M08WitnessConvoyIncrements,
                M08ApproachBudgetTicks,
                M08RelayBudgetTicks,
                M08TraversalBudgetTicks,
                M08CompletionBudgetTicks,
                M08StandTelemetry.IsEmpty()
                    ? TEXT("none")
                    : *M08StandTelemetry,
                *DefenderState);
        };
        const auto RequireMissionEight = [
            &Require,
            &ObserveMissionEightProtectedState,
            &WriteMissionEightDiagnostic](
                bool bCondition,
                const FString& Label)
        {
            ObserveMissionEightProtectedState();
            if (!bCondition)
            {
                WriteMissionEightDiagnostic();
            }
            return Require(bCondition, Label);
        };
        const auto TickUntilMissionEightCondition = [
            Bridge,
            &ObserveMissionEightProtectedState](
                const TFunction<bool()>& Predicate,
                int32 MaximumTicks)
        {
            for (int32 TickIndex = 0;
                 TickIndex < MaximumTicks;
                 ++TickIndex)
            {
                ObserveMissionEightProtectedState();
                if (Bridge->GetShapeBesideUsPhase() ==
                    EEchoesShapeBesideUsPhase::Failed)
                {
                    return false;
                }
                if (Predicate())
                {
                    return true;
                }
                Bridge->Tick(0.05f);
            }
            ObserveMissionEightProtectedState();
            return Bridge->GetShapeBesideUsPhase() !=
                    EEchoesShapeBesideUsPhase::Failed &&
                Predicate();
        };
        const auto FindMissionEightRelay = [
            Bridge,
            M08Plan,
            &M08RelayId]()
        {
            const Entity* Current = Bridge->FindEntity(M08RelayId);
            if (M08RelayId != 0 && Current != nullptr &&
                Current->owner ==
                    UEchoesSimulationSubsystem::LocalPlayerId &&
                Current->type == EntityType::UtilityStructure &&
                Current->hitPoints > 0 &&
                Current->position == M08Plan.EchoRelaySite)
            {
                return true;
            }
            M08RelayId = 0;
            const echoes::sim::Simulation* Simulation =
                Bridge->GetSimulation();
            if (Simulation == nullptr)
            {
                return false;
            }
            for (const Entity& Candidate : Simulation->Entities())
            {
                if (Candidate.owner ==
                        UEchoesSimulationSubsystem::LocalPlayerId &&
                    Candidate.type == EntityType::UtilityStructure &&
                    Candidate.hitPoints > 0 &&
                    Candidate.position == M08Plan.EchoRelaySite)
                {
                    M08RelayId = Candidate.id;
                    return true;
                }
            }
            return false;
        };
        const auto IssueMissionEightGuardAssignments = [
            Bridge,
            &M08GuardIds,
            &M08GuardTargetIds,
            &Feedback]()
        {
            if (M08GuardIds.Num() != 4 ||
                M08GuardTargetIds.Num() != M08GuardIds.Num())
            {
                Feedback = TEXT(
                    "[M08_TACTICS_ASSIGNMENT_INVALID] Mission 08 requires four exact Guard assignments.");
                return false;
            }
            for (int32 GuardIndex = 0;
                 GuardIndex < M08GuardIds.Num();
                 ++GuardIndex)
            {
                const Entity* Target = Bridge->FindEntity(
                    M08GuardTargetIds[GuardIndex]);
                if (Target == nullptr || Target->hitPoints <= 0 ||
                    !Bridge->IssueCommand(
                        CommandType::Guard,
                        M08GuardIds[GuardIndex],
                        M08GuardTargetIds[GuardIndex],
                        Bridge->SimToWorld(Target->position),
                        FutureWellChoice::Dormant,
                        Feedback))
                {
                    return false;
                }
            }
            return true;
        };
        const auto MissionEightGuardsActive = [
            Bridge,
            &M08GuardIds,
            &M08GuardTargetIds]()
        {
            if (M08GuardIds.Num() != 4 ||
                M08GuardTargetIds.Num() != M08GuardIds.Num())
            {
                return false;
            }
            for (int32 GuardIndex = 0;
                 GuardIndex < M08GuardIds.Num();
                 ++GuardIndex)
            {
                const Entity* Guard = Bridge->FindEntity(
                    M08GuardIds[GuardIndex]);
                if (Guard == nullptr || Guard->hitPoints <= 0 ||
                    Guard->order.type != echoes::sim::OrderType::Guard ||
                    Guard->order.target !=
                        M08GuardTargetIds[GuardIndex])
                {
                    return false;
                }
            }
            return true;
        };
        // Fail-safe against clear-on-death idling: an alive escort whose
        // expected ward also lives must hold a Guard order on that ward;
        // re-issue the Guard when the simulation cleared it. A no-op in every
        // healthy state (order already Guard) and after a real ward loss
        // (dead wards are skipped, and witness/Talar losses fail the mission
        // through the phase reducer regardless).
        const auto ReassertMissionEightGuardOrders = [
            Bridge,
            &M08GuardIds,
            &M08GuardTargetIds]()
        {
            for (int32 GuardIndex = 0;
                 GuardIndex < M08GuardIds.Num();
                 ++GuardIndex)
            {
                if (!M08GuardTargetIds.IsValidIndex(GuardIndex))
                {
                    continue;
                }
                const Entity* Guard =
                    Bridge->FindEntity(M08GuardIds[GuardIndex]);
                const Entity* Ward =
                    Bridge->FindEntity(M08GuardTargetIds[GuardIndex]);
                if (Guard == nullptr || Guard->hitPoints <= 0 ||
                    Ward == nullptr || Ward->hitPoints <= 0 ||
                    Guard->order.type == echoes::sim::OrderType::Guard)
                {
                    continue;
                }
                FString ReguardFeedback;
                (void)Bridge->IssueCommand(
                    CommandType::Guard,
                    M08GuardIds[GuardIndex],
                    M08GuardTargetIds[GuardIndex],
                    Bridge->SimToWorld(Ward->position),
                    FutureWellChoice::Dormant,
                    ReguardFeedback);
            }
        };
        // Ticks the world while draining one of the pre-existing Mission 08
        // wait budgets, with the same Failed-phase short-circuit as
        // TickUntilMissionEightCondition, re-asserting cleared Guard orders
        // each tick.
        const auto TickMissionEightWithinBudget = [
            Bridge,
            &ObserveMissionEightProtectedState,
            &ReassertMissionEightGuardOrders](
                int32& RemainingTicks,
                const TFunction<bool()>& Predicate)
        {
            while (RemainingTicks > 0)
            {
                ObserveMissionEightProtectedState();
                if (Bridge->GetShapeBesideUsPhase() ==
                    EEchoesShapeBesideUsPhase::Failed)
                {
                    return false;
                }
                ReassertMissionEightGuardOrders();
                if (Predicate())
                {
                    return true;
                }
                Bridge->Tick(0.05f);
                --RemainingTicks;
            }
            ObserveMissionEightProtectedState();
            return Bridge->GetShapeBesideUsPhase() !=
                    EEchoesShapeBesideUsPhase::Failed &&
                Predicate();
        };
        const auto MissionEightEscortsReformed = [
            Bridge,
            &M08GuardIds,
            &M08GuardTargetIds](
                const TArray<int32>& EscortIndices,
                EntityId LeadId,
                int32 RadiusTiles)
        {
            const Entity* Lead = Bridge->FindEntity(LeadId);
            if (Lead == nullptr || Lead->hitPoints <= 0)
            {
                return false;
            }
            for (const int32 EscortIndex : EscortIndices)
            {
                if (!M08GuardIds.IsValidIndex(EscortIndex) ||
                    !M08GuardTargetIds.IsValidIndex(EscortIndex))
                {
                    return false;
                }
                const Entity* Escort =
                    Bridge->FindEntity(M08GuardIds[EscortIndex]);
                if (Escort == nullptr || Escort->hitPoints <= 0 ||
                    Escort->order.type != echoes::sim::OrderType::Guard ||
                    Escort->order.target !=
                        M08GuardTargetIds[EscortIndex] ||
                    !IsAtSite(
                        Bridge,
                        M08GuardIds[EscortIndex],
                        Lead->position,
                        RadiusTiles))
                {
                    return false;
                }
            }
            return true;
        };
        const auto MissionEightConvoyCasualty = [
            Bridge,
            &M08GuardIds](
                EntityId LeadId,
                const TArray<int32>& EscortIndices)
        {
            const Entity* Lead = Bridge->FindEntity(LeadId);
            if (Lead == nullptr || Lead->hitPoints <= 0)
            {
                return true;
            }
            for (const int32 EscortIndex : EscortIndices)
            {
                if (!M08GuardIds.IsValidIndex(EscortIndex))
                {
                    return true;
                }
                const Entity* Escort =
                    Bridge->FindEntity(M08GuardIds[EscortIndex]);
                if (Escort == nullptr || Escort->hitPoints <= 0)
                {
                    return true;
                }
            }
            return false;
        };
        // The simulation rejects Hold for attack-less actors
        // ([HOLD_REQUIRES_DEFENDER]), so the worker halts through Stop, which
        // clears its order and leaves it stationary in place.
        const auto HaltMissionEightLead = [
            Bridge,
            &Feedback](
                EntityId LeadId,
                bool bLeadCanHold)
        {
            const Entity* Lead = Bridge->FindEntity(LeadId);
            if (Lead == nullptr || Lead->hitPoints <= 0)
            {
                return false;
            }
            return Bridge->IssueCommand(
                bLeadCanHold ? CommandType::Hold : CommandType::Stop,
                LeadId,
                0,
                Bridge->SimToWorld(Lead->position),
                FutureWellChoice::Dormant,
                Feedback);
        };
        // One bounded step from the lead's live position toward Goal, in pure
        // integer math on raw fixed-point coordinates. Returns false when the
        // lead already stands at the requested standoff (with one-eighth-tile
        // slack for integer truncation); a zero standoff demands exact
        // arrival, and its final step lands exactly on Goal.
        const auto ComputeMissionEightConvoyStep = [](
            const Vec2& Current,
            const Vec2& Goal,
            int32 StandoffRaw,
            int32 IncrementRaw,
            Vec2& OutStepDestination)
        {
            const int64 DeltaX =
                static_cast<int64>(Goal.x.Raw()) - Current.x.Raw();
            const int64 DeltaY =
                static_cast<int64>(Goal.y.Raw()) - Current.y.Raw();
            const int64 DistanceSquared =
                DeltaX * DeltaX + DeltaY * DeltaY;
            const int64 ArrivalRaw =
                StandoffRaw > 0
                    ? static_cast<int64>(StandoffRaw) +
                          echoes::sim::kFixedScale / 8
                    : 0;
            if (DistanceSquared <= ArrivalRaw * ArrivalRaw)
            {
                return false;
            }
            int64 Distance = 0;
            int64 SquareRemainder = DistanceSquared;
            int64 Bit = int64(1) << 62;
            while (Bit > SquareRemainder)
            {
                Bit >>= 2;
            }
            while (Bit != 0)
            {
                if (SquareRemainder >= Distance + Bit)
                {
                    SquareRemainder -= Distance + Bit;
                    Distance = (Distance >> 1) + Bit;
                }
                else
                {
                    Distance >>= 1;
                }
                Bit >>= 2;
            }
            if (Distance <= StandoffRaw)
            {
                return false;
            }
            const int64 TravelRaw = FMath::Min(
                static_cast<int64>(IncrementRaw),
                Distance - StandoffRaw);
            if (TravelRaw >= Distance)
            {
                OutStepDestination = Goal;
                return true;
            }
            OutStepDestination = Vec2::FromRaw(
                static_cast<int32>(
                    Current.x.Raw() + DeltaX * TravelRaw / Distance),
                static_cast<int32>(
                    Current.y.Raw() + DeltaY * TravelRaw / Distance));
            return true;
        };
        // True when the candidate position's tile lies outside every live
        // hostile entity's own vision disc — the same floored-tile Euclidean
        // math the simulation's UpdateVisibility/markVisible uses. A stand
        // outside every disc is never seen, so it never triggers the
        // opposing AI's visible-enemy attack pull.
        const auto MissionEightStandHiddenFromHostiles = [Bridge](
            const Vec2& Candidate)
        {
            const echoes::sim::Simulation* Simulation =
                Bridge->GetSimulation();
            if (Simulation == nullptr)
            {
                return false;
            }
            const int64 CandidateTileX = Candidate.x.FloorToInt();
            const int64 CandidateTileY = Candidate.y.FloorToInt();
            for (const Entity& Hostile : Simulation->Entities())
            {
                if (Hostile.owner ==
                        UEchoesSimulationSubsystem::LocalPlayerId ||
                    Hostile.owner == echoes::sim::kNeutralPlayer ||
                    Hostile.hitPoints <= 0 || Hostile.visionTiles <= 0)
                {
                    continue;
                }
                const int64 DeltaTileX =
                    CandidateTileX - Hostile.position.x.FloorToInt();
                const int64 DeltaTileY =
                    CandidateTileY - Hostile.position.y.FloorToInt();
                const int64 RadiusTiles = Hostile.visionTiles;
                if (DeltaTileX * DeltaTileX + DeltaTileY * DeltaTileY <=
                    RadiusTiles * RadiusTiles)
                {
                    return false;
                }
            }
            return true;
        };
        // Chooses a leg's final stand: the plan site displaced by the stand
        // offset along the direction away from the nearest live hostile,
        // rotated in fixed 30-degree steps (smallest deviation first) until
        // a candidate is passable and hidden from every hostile vision
        // disc; falls back to the raw site when none qualifies. Every stand
        // remains inside the 3-tile mission-fact radius by construction,
        // and everything derives from live simulation state and plan sites.
        const auto SelectMissionEightVisionSafeStand = [
            Bridge,
            M08StandOffsetRaw,
            &M08StandTelemetry,
            &ComputeMissionEightConvoyStep,
            &MissionEightStandHiddenFromHostiles](
                const TCHAR* StandLabel,
                const Vec2& Site) -> Vec2
        {
            const echoes::sim::Simulation* Simulation =
                Bridge->GetSimulation();
            Vec2 Chosen = Site;
            bool bFallback = true;
            if (Simulation != nullptr)
            {
                const Entity* NearestHostile = nullptr;
                int64 NearestDistanceSquared =
                    std::numeric_limits<int64>::max();
                for (const Entity& Hostile : Simulation->Entities())
                {
                    if (Hostile.owner ==
                            UEchoesSimulationSubsystem::LocalPlayerId ||
                        Hostile.owner == echoes::sim::kNeutralPlayer ||
                        Hostile.hitPoints <= 0)
                    {
                        continue;
                    }
                    const int64 DeltaX =
                        static_cast<int64>(Hostile.position.x.Raw()) -
                        Site.x.Raw();
                    const int64 DeltaY =
                        static_cast<int64>(Hostile.position.y.Raw()) -
                        Site.y.Raw();
                    const int64 DistanceSquared =
                        DeltaX * DeltaX + DeltaY * DeltaY;
                    if (DistanceSquared < NearestDistanceSquared ||
                        (DistanceSquared == NearestDistanceSquared &&
                         (NearestHostile == nullptr ||
                          Hostile.id < NearestHostile->id)))
                    {
                        NearestHostile = &Hostile;
                        NearestDistanceSquared = DistanceSquared;
                    }
                }
                int64 AwayX = 0;
                int64 AwayY = -1024;
                if (NearestHostile != nullptr)
                {
                    AwayX = static_cast<int64>(Site.x.Raw()) -
                        NearestHostile->position.x.Raw();
                    AwayY = static_cast<int64>(Site.y.Raw()) -
                        NearestHostile->position.y.Raw();
                    if (AwayX == 0 && AwayY == 0)
                    {
                        AwayY = -1024;
                    }
                }
                // cos/sin pairs in 1/1024ths for 0, +/-30, +/-60, +/-90,
                // +/-120, +/-150, 180 degrees.
                static constexpr int32 RotationTable[][2] = {
                    {1024, 0},   {887, 512},   {887, -512},
                    {512, 887},  {512, -887},  {0, 1024},
                    {0, -1024},  {-512, 887},  {-512, -887},
                    {-887, 512}, {-887, -512}, {-1024, 0}};
                for (const auto& Rotation : RotationTable)
                {
                    const int64 RotatedX =
                        (AwayX * Rotation[0] - AwayY * Rotation[1]) / 1024;
                    const int64 RotatedY =
                        (AwayX * Rotation[1] + AwayY * Rotation[0]) / 1024;
                    if (RotatedX == 0 && RotatedY == 0)
                    {
                        continue;
                    }
                    const Vec2 DirectionTarget = Vec2::FromRaw(
                        static_cast<int32>(Site.x.Raw() + RotatedX),
                        static_cast<int32>(Site.y.Raw() + RotatedY));
                    Vec2 Candidate = Site;
                    if (!ComputeMissionEightConvoyStep(
                            Site,
                            DirectionTarget,
                            0,
                            M08StandOffsetRaw,
                            Candidate))
                    {
                        continue;
                    }
                    if (Simulation->IsPositionPassable(Candidate) &&
                        MissionEightStandHiddenFromHostiles(Candidate))
                    {
                        Chosen = Candidate;
                        bFallback = false;
                        break;
                    }
                }
            }
            M08StandTelemetry += FString::Printf(
                TEXT(" %s=(%d,%d)%s"),
                StandLabel,
                Chosen.x.FloorToInt(),
                Chosen.y.FloorToInt(),
                bFallback ? TEXT("(fallback-site)") : TEXT(""));
            return Chosen;
        };
        // Bounded convoy pacing: reform first, then repeatedly move the lead
        // one computed increment toward Goal, halt it, and wait until every
        // escort stands back inside the two-tile follow radius before the
        // next increment. Destinations are always derived from live positions
        // and plan-owned sites; nothing is hard-coded.
        const auto PaceMissionEightConvoy = [
            Bridge,
            &Move,
            &Feedback,
            &ObserveMissionEightProtectedState,
            &TickMissionEightWithinBudget,
            &MissionEightEscortsReformed,
            &MissionEightConvoyCasualty,
            &HaltMissionEightLead,
            &ComputeMissionEightConvoyStep,
            M08ConvoyIncrementRaw,
            M08ConvoyReformRadiusTiles](
                const TCHAR* LegLabel,
                EntityId LeadId,
                bool bLeadCanHold,
                const Vec2& Goal,
                int32 StandoffRaw,
                const TArray<int32>& EscortIndices,
                int32& RemainingTicks,
                int32& IncrementCount,
                const TFunction<bool()>& LegObjectiveMet) -> bool
        {
            const auto FailLeg = [
                Bridge,
                LegLabel,
                LeadId,
                &Feedback,
                &RemainingTicks](const TCHAR* Reason)
            {
                Feedback = FString::Printf(
                    TEXT("[M08_CONVOY_%s] leg=%s remainingBudget=%d %s"),
                    Reason,
                    LegLabel,
                    RemainingTicks,
                    *DescribeFreshJourneyEntity(
                        TEXT("lead"),
                        LeadId,
                        Bridge->FindEntity(LeadId)));
                return false;
            };
            while (true)
            {
                if (LegObjectiveMet && LegObjectiveMet())
                {
                    return true;
                }
                if (MissionEightConvoyCasualty(LeadId, EscortIndices))
                {
                    return FailLeg(TEXT("CASUALTY"));
                }
                if (!EscortIndices.IsEmpty())
                {
                    if (!TickMissionEightWithinBudget(
                            RemainingTicks,
                            [LeadId,
                             &EscortIndices,
                             &LegObjectiveMet,
                             &MissionEightEscortsReformed,
                             &MissionEightConvoyCasualty,
                             M08ConvoyReformRadiusTiles]()
                            {
                                return (LegObjectiveMet &&
                                        LegObjectiveMet()) ||
                                    MissionEightConvoyCasualty(
                                        LeadId, EscortIndices) ||
                                    MissionEightEscortsReformed(
                                        EscortIndices,
                                        LeadId,
                                        M08ConvoyReformRadiusTiles);
                            }))
                    {
                        return FailLeg(TEXT("REFORM_TIMEOUT"));
                    }
                    if (LegObjectiveMet && LegObjectiveMet())
                    {
                        return true;
                    }
                    if (MissionEightConvoyCasualty(LeadId, EscortIndices))
                    {
                        return FailLeg(TEXT("CASUALTY"));
                    }
                }
                const Entity* Lead = Bridge->FindEntity(LeadId);
                if (Lead == nullptr || Lead->hitPoints <= 0)
                {
                    return FailLeg(TEXT("CASUALTY"));
                }
                Vec2 StepDestination;
                if (!ComputeMissionEightConvoyStep(
                        Lead->position,
                        Goal,
                        StandoffRaw,
                        M08ConvoyIncrementRaw,
                        StepDestination))
                {
                    return true;
                }
                if (RemainingTicks <= 0)
                {
                    return FailLeg(TEXT("BUDGET_EXHAUSTED"));
                }
                // The computed checkpoint governs pacing; the Move
                // destination may need to be a farther same-line rung, or
                // the leg Goal itself, when the checkpoint tile is
                // scar-blocked — Move validation rejects blocked
                // DESTINATIONS while the simulation's own pathfinding routes
                // around blocked tiles en route, so a farther passable
                // destination lets the lead detour through a crossing. The
                // checkpoint/displacement waits below keep each increment
                // bounded regardless of the path taken.
                const Vec2 StepCheckpoint = StepDestination;
                const Vec2 StepStartPosition = Lead->position;
                bool bMoveAccepted = Move(LeadId, StepDestination);
                if (!bMoveAccepted)
                {
                    const FString CheckpointRejection = FString::Printf(
                        TEXT("checkpoint=(%d,%d) rejection=%s"),
                        StepCheckpoint.x.FloorToInt(),
                        StepCheckpoint.y.FloorToInt(),
                        *Feedback);
                    Vec2 PreviousAttempt = StepDestination;
                    for (int32 RungMultiplier = 2;
                         !bMoveAccepted && RungMultiplier <= 4;
                         ++RungMultiplier)
                    {
                        Vec2 RungDestination = Goal;
                        if (RungMultiplier <= 3 &&
                            !ComputeMissionEightConvoyStep(
                                StepStartPosition,
                                Goal,
                                StandoffRaw,
                                M08ConvoyIncrementRaw * RungMultiplier,
                                RungDestination))
                        {
                            continue;
                        }
                        if (RungDestination == PreviousAttempt)
                        {
                            continue;
                        }
                        PreviousAttempt = RungDestination;
                        bMoveAccepted = Move(LeadId, RungDestination);
                        if (bMoveAccepted)
                        {
                            StepDestination = RungDestination;
                        }
                    }
                    if (!bMoveAccepted)
                    {
                        FailLeg(TEXT("STEP_REJECTED"));
                        Feedback += TEXT(" ");
                        Feedback += CheckpointRejection;
                        return false;
                    }
                }
                ++IncrementCount;
                for (int32 WarmupIndex = 0;
                     WarmupIndex < 2 && RemainingTicks > 0;
                     ++WarmupIndex)
                {
                    ObserveMissionEightProtectedState();
                    Bridge->Tick(0.05f);
                    --RemainingTicks;
                }
                const bool bFinalStep =
                    StandoffRaw == 0 && StepCheckpoint == Goal;
                if (!TickMissionEightWithinBudget(
                        RemainingTicks,
                        [Bridge,
                         LeadId,
                         StepCheckpoint,
                         StepStartPosition,
                         Goal,
                         bFinalStep,
                         &EscortIndices,
                         &LegObjectiveMet,
                         &MissionEightConvoyCasualty,
                         M08ConvoyIncrementRaw]()
                        {
                            if (LegObjectiveMet && LegObjectiveMet())
                            {
                                return true;
                            }
                            if (MissionEightConvoyCasualty(
                                    LeadId, EscortIndices))
                            {
                                return true;
                            }
                            const Entity* Current =
                                Bridge->FindEntity(LeadId);
                            if (Current->order.type ==
                                echoes::sim::OrderType::None)
                            {
                                return true;
                            }
                            if (bFinalStep)
                            {
                                return Current->position == Goal;
                            }
                            if (IsAtSite(
                                    Bridge, LeadId, StepCheckpoint, 1))
                            {
                                return true;
                            }
                            const int64 DisplacementX =
                                static_cast<int64>(
                                    Current->position.x.Raw()) -
                                StepStartPosition.x.Raw();
                            const int64 DisplacementY =
                                static_cast<int64>(
                                    Current->position.y.Raw()) -
                                StepStartPosition.y.Raw();
                            const int64 IncrementSquared =
                                static_cast<int64>(M08ConvoyIncrementRaw) *
                                M08ConvoyIncrementRaw;
                            return DisplacementX * DisplacementX +
                                    DisplacementY * DisplacementY >=
                                IncrementSquared;
                        }))
                {
                    return FailLeg(TEXT("STEP_TIMEOUT"));
                }
                if (LegObjectiveMet && LegObjectiveMet())
                {
                    return true;
                }
                if (MissionEightConvoyCasualty(LeadId, EscortIndices))
                {
                    return FailLeg(TEXT("CASUALTY"));
                }
                if (!HaltMissionEightLead(LeadId, bLeadCanHold))
                {
                    return FailLeg(TEXT("HALT_REJECTED"));
                }
            }
        };
        // The worker's construction stance: 3.3 tiles from the relay site
        // along the DOMINANT axis of the plan-owned first-echo direction,
        // sign-preserving. Placement's footprint-overlap test is per-axis —
        // blocked only when BOTH axis offsets sit inside the combined
        // half-extent band of 1.125 tiles — so one clear axis suffices: the
        // worker stands 3.3 tiles out on that axis, and any escort settled
        // inside its 2-tile follow ring keeps at least 1.3 tiles there.
        // Both actor classes are therefore outside the band on ANY
        // echo-relay axis. The prior 1.375-tile Euclidean stance split its
        // offset across axes on diagonal geometry and put the worker itself
        // inside the band ([BUILD_PLACEMENT_INVALID] code 5, Occupied).
        const Vec2 M08BuildStanceSite = [&M08Plan]()
        {
            const int32 StanceOffsetRaw =
                (33 * echoes::sim::kFixedScale) / 10;
            const int64 DeltaX =
                static_cast<int64>(M08Plan.FirstEchoSite.x.Raw()) -
                M08Plan.EchoRelaySite.x.Raw();
            const int64 DeltaY =
                static_cast<int64>(M08Plan.FirstEchoSite.y.Raw()) -
                M08Plan.EchoRelaySite.y.Raw();
            if (DeltaX == 0 && DeltaY == 0)
            {
                return M08Plan.EchoRelaySite;
            }
            const bool bDominantX =
                (DeltaX >= 0 ? DeltaX : -DeltaX) >=
                (DeltaY >= 0 ? DeltaY : -DeltaY);
            const int32 StepX = bDominantX
                ? (DeltaX >= 0 ? StanceOffsetRaw : -StanceOffsetRaw)
                : 0;
            const int32 StepY = bDominantX
                ? 0
                : (DeltaY >= 0 ? StanceOffsetRaw : -StanceOffsetRaw);
            return Vec2::FromRaw(
                M08Plan.EchoRelaySite.x.Raw() + StepX,
                M08Plan.EchoRelaySite.y.Raw() + StepY);
        }();
        const auto MissionEightWorkerHolding = [
            Bridge,
            M08Worker,
            M08Plan]()
        {
            const Entity* Worker = Bridge->FindEntity(M08Worker);
            return Worker != nullptr && Worker->hitPoints > 0 &&
                Worker->order.type == echoes::sim::OrderType::None &&
                IsAtSite(Bridge, M08Worker, M08Plan.FirstEchoSite, 2);
        };
        const auto MissionEightConvoyStagedAtRelay = [
            Bridge,
            M08Plan,
            M08Worker,
            &M08GuardIds]()
        {
            const Entity* Worker = Bridge->FindEntity(M08Worker);
            if (Worker == nullptr || Worker->hitPoints <= 0 ||
                !IsAtSite(Bridge, M08Worker, M08Plan.EchoRelaySite, 2))
            {
                return false;
            }
            if (M08GuardIds.Num() != 4)
            {
                return false;
            }
            for (int32 EscortIndex = 0; EscortIndex < 2; ++EscortIndex)
            {
                const Entity* Escort =
                    Bridge->FindEntity(M08GuardIds[EscortIndex]);
                if (Escort == nullptr || Escort->hitPoints <= 0 ||
                    !IsAtSite(
                        Bridge,
                        M08GuardIds[EscortIndex],
                        M08Plan.EchoRelaySite,
                        2))
                {
                    return false;
                }
            }
            return true;
        };
        const auto PrepareMissionEightTalarGuards = [
            M08Start,
            M08Soldiers,
            M08Heavies,
            &M08GuardIds,
            &M08GuardTargetIds,
            &IssueMissionEightGuardAssignments]()
        {
            if (M08Soldiers.Num() != 3 || M08Heavies.Num() != 1)
            {
                return false;
            }
            const EntityId GuardActors[] = {
                M08Soldiers[0],
                M08Soldiers[1],
                M08Soldiers[2],
                M08Heavies[0]};
            M08GuardIds.Reset();
            M08GuardTargetIds.Reset();
            for (const EntityId GuardActor : GuardActors)
            {
                M08GuardIds.Add(GuardActor);
                M08GuardTargetIds.Add(
                    M08Start.ShapeBesideUsTalarId);
            }
            return IssueMissionEightGuardAssignments();
        };
        const auto RetargetMissionEightRelayGuards = [
            M08Start,
            &M08RelayId,
            &M08GuardTargetIds,
            &FindMissionEightRelay,
            &IssueMissionEightGuardAssignments]()
        {
            if (!FindMissionEightRelay() ||
                M08GuardTargetIds.Num() != 4)
            {
                return false;
            }
            M08GuardTargetIds[0] = M08RelayId;
            M08GuardTargetIds[1] = M08RelayId;
            M08GuardTargetIds[2] =
                M08Start.ShapeBesideUsTalarId;
            M08GuardTargetIds[3] =
                M08Start.ShapeBesideUsTalarId;
            return IssueMissionEightGuardAssignments();
        };
        const auto RetargetMissionEightWorkerGuards = [
            M08Start,
            M08Worker,
            &M08GuardTargetIds,
            &IssueMissionEightGuardAssignments]()
        {
            if (M08GuardTargetIds.Num() != 4)
            {
                return false;
            }
            M08GuardTargetIds[0] = M08Worker;
            M08GuardTargetIds[1] = M08Worker;
            M08GuardTargetIds[2] =
                M08Start.ShapeBesideUsTalarId;
            M08GuardTargetIds[3] =
                M08Start.ShapeBesideUsTalarId;
            return IssueMissionEightGuardAssignments();
        };
        const auto RetargetMissionEightTraversalGuards = [
            Bridge,
            M08Start,
            &M08RelayId,
            &M08GuardTargetIds,
            &FindMissionEightRelay,
            &IssueMissionEightGuardAssignments]()
        {
            if (!FindMissionEightRelay() ||
                M08GuardTargetIds.Num() != 4)
            {
                return false;
            }
            const Entity* Relay = Bridge->FindEntity(M08RelayId);
            if (Relay == nullptr || Relay->hitPoints <= 0 ||
                !Relay->completed)
            {
                return false;
            }
            M08GuardTargetIds[0] = M08RelayId;
            M08GuardTargetIds[1] = M08Start.FirstStateWitnessId;
            M08GuardTargetIds[2] = M08Start.SecondStateWitnessId;
            M08GuardTargetIds[3] =
                M08Start.ShapeBesideUsTalarId;
            return IssueMissionEightGuardAssignments();
        };
        ObserveMissionEightProtectedState();
        if (!RequireMissionEight(
                M08Worker != 0,
                TEXT("Mission 08 exposes a separate construction worker")) ||
            !RequireMissionEight(
                M08Soldiers.Num() == 3 && M08Heavies.Num() == 1,
                TEXT("Mission 08 exposes exactly three generic Soldiers and one Heavy")) ||
            !RequireMissionEight(
                PrepareMissionEightTalarGuards(),
                TEXT("Mission 08 assigns all four defenders to Talar")) ||
            !RequireMissionEight(
                TickUntilMissionEightCondition(
                    [&MissionEightGuardsActive]()
                    {
                        return MissionEightGuardsActive();
                    },
                    20),
                TEXT("Mission 08 initial Talar Guards take effect")) ||
            !RequireMissionEight(
                PaceMissionEightConvoy(
                    TEXT("worker-first-echo"),
                    M08Worker,
                    false,
                    M08Plan.FirstEchoSite,
                    M08WorkerHoldStandoffRaw,
                    M08NoEscortIndices,
                    M08ApproachBudgetTicks,
                    M08WorkerConvoyIncrements,
                    M08NoLegObjective),
                TEXT("Mission 08 pre-positions the separate worker beside the first echo")) ||
            !RequireMissionEight(
                TickMissionEightWithinBudget(
                    M08ApproachBudgetTicks,
                    [&MissionEightWorkerHolding]()
                    {
                        return MissionEightWorkerHolding();
                    }),
                TEXT("Mission 08 holds the worker within two tiles of the first echo")) ||
            !RequireMissionEight(
                PaceMissionEightConvoy(
                    TEXT("talar-first-echo"),
                    M08Start.ShapeBesideUsTalarId,
                    true,
                    M08Plan.FirstEchoSite,
                    0,
                    M08TalarEscortIndices,
                    M08ApproachBudgetTicks,
                    M08TalarConvoyIncrements,
                    M08NoLegObjective),
                TEXT("Mission 08 paces Talar to the first echo behind reformed Guards")) ||
            !RequireMissionEight(
                Bridge->GetShapeBesideUsPhase() ==
                    EEchoesShapeBesideUsPhase::RaiseEchoRelay,
                TEXT("Mission 08 reaches relay construction")) ||
            !RequireMissionEight(
                MissionEightGuardsActive() &&
                    MissionEightEscortsReformed(
                        M08TalarEscortIndices,
                        M08Start.ShapeBesideUsTalarId,
                        M08ConvoyReformRadiusTiles),
                TEXT("Mission 08 proves all four Guards reformed at the phase change")) ||
            !RequireMissionEight(
                RetargetMissionEightWorkerGuards(),
                TEXT("Mission 08 retargets two Guards to escort the worker")) ||
            !RequireMissionEight(
                TickMissionEightWithinBudget(
                    M08RelayBudgetTicks,
                    [&MissionEightGuardsActive]()
                    {
                        return MissionEightGuardsActive();
                    }),
                TEXT("Mission 08 worker and Talar Guards take effect")) ||
            !RequireMissionEight(
                PaceMissionEightConvoy(
                    TEXT("worker-echo-relay"),
                    M08Worker,
                    false,
                    M08Plan.EchoRelaySite,
                    0,
                    M08WorkerEscortIndices,
                    M08RelayBudgetTicks,
                    M08WorkerConvoyIncrements,
                    M08NoLegObjective),
                TEXT("Mission 08 paces the worker to the relay site with both escorts")) ||
            !RequireMissionEight(
                MissionEightGuardsActive() &&
                    MissionEightConvoyStagedAtRelay(),
                TEXT("Mission 08 stages the worker and both escorts within two tiles of the relay")) ||
            !RequireMissionEight(
                PaceMissionEightConvoy(
                    TEXT("worker-build-stance"),
                    M08Worker,
                    false,
                    M08BuildStanceSite,
                    0,
                    M08WorkerEscortIndices,
                    M08RelayBudgetTicks,
                    M08WorkerConvoyIncrements,
                    M08NoLegObjective),
                TEXT("Mission 08 clears the relay footprint for construction")) ||
            !RequireMissionEight(
                Bridge->IssueBuildCommand(
                    M08Worker,
                    EntityType::UtilityStructure,
                    Bridge->SimToWorld(M08Plan.EchoRelaySite),
                    Feedback),
                TEXT("Mission 08 accepts the echo relay")) ||
            !RequireMissionEight(
                TickUntilMissionEightCondition(
                    [&FindMissionEightRelay]()
                    {
                        return FindMissionEightRelay();
                    },
                    20),
                TEXT("Mission 08 exposes the live echo relay")) ||
            !RequireMissionEight(
                RetargetMissionEightRelayGuards(),
                TEXT("Mission 08 retargets two defenders to the live echo relay")) ||
            !RequireMissionEight(
                TickUntilMissionEightCondition(
                    [&MissionEightGuardsActive]()
                    {
                        return MissionEightGuardsActive();
                    },
                    20),
                TEXT("Mission 08 relay and Talar Guards take effect")) ||
            !RequireMissionEight(
                TickMissionEightWithinBudget(
                    M08RelayBudgetTicks,
                    [Bridge]()
                    {
                        return Bridge->GetShapeBesideUsPhase() ==
                            EEchoesShapeBesideUsPhase::
                                TraversePairedStates;
                    }),
                TEXT("Mission 08 opens paired-state traversal")) ||
            !RequireMissionEight(
                PaceMissionEightConvoy(
                    TEXT("worker-withdrawal"),
                    M08Worker,
                    false,
                    M08Plan.FirstEchoSite,
                    M08WorkerHoldStandoffRaw,
                    M08NoEscortIndices,
                    M08RelayBudgetTicks,
                    M08WorkerConvoyIncrements,
                    M08NoLegObjective),
                TEXT("Mission 08 withdraws the worker from the contested corridor")) ||
            !RequireMissionEight(
                RetargetMissionEightTraversalGuards(),
                TEXT("Mission 08 assigns completed-relay and witness Guards")) ||
            !RequireMissionEight(
                TickUntilMissionEightCondition(
                    [&MissionEightGuardsActive]()
                    {
                        return MissionEightGuardsActive();
                    },
                    20),
                TEXT("Mission 08 traversal Guards take effect")) ||
            !RequireMissionEight(
                PaceMissionEightConvoy(
                    TEXT("witnessA-first-state"),
                    M08Start.FirstStateWitnessId,
                    false,
                    SelectMissionEightVisionSafeStand(
                        TEXT("witnessA"), M08Plan.FirstStateSite),
                    echoes::sim::kFixedScale / 8,
                    M08WitnessAEscortIndices,
                    M08TraversalBudgetTicks,
                    M08WitnessConvoyIncrements,
                    M08NoLegObjective),
                TEXT("Mission 08 first witness accepts its state")) ||
            !RequireMissionEight(
                PaceMissionEightConvoy(
                    TEXT("witnessB-second-state"),
                    M08Start.SecondStateWitnessId,
                    true,
                    SelectMissionEightVisionSafeStand(
                        TEXT("witnessB"), M08Plan.SecondStateSite),
                    echoes::sim::kFixedScale / 8,
                    M08WitnessBEscortIndices,
                    M08TraversalBudgetTicks,
                    M08WitnessConvoyIncrements,
                    M08NoLegObjective),
                TEXT("Mission 08 second witness accepts its state")) ||
            !RequireMissionEight(
                TickMissionEightWithinBudget(
                    M08TraversalBudgetTicks,
                    [Bridge]()
                    {
                        return Bridge->GetShapeBesideUsPhase() ==
                            EEchoesShapeBesideUsPhase::ReachConvergence;
                    }),
                TEXT("Mission 08 traverses both states")) ||
            !RequireMissionEight(
                PaceMissionEightConvoy(
                    TEXT("talar-convergence"),
                    M08Start.ShapeBesideUsTalarId,
                    true,
                    SelectMissionEightVisionSafeStand(
                        TEXT("convergence"), M08Plan.ConvergenceSite),
                    echoes::sim::kFixedScale / 8,
                    M08ConvergenceEscortIndices,
                    M08CompletionBudgetTicks,
                    M08TalarConvoyIncrements,
                    M08ConvergenceObjectiveMet),
                TEXT("Mission 08 Talar accepts convergence")) ||
            !RequireMissionEight(
                TickMissionEightWithinBudget(
                    M08CompletionBudgetTicks,
                    [Bridge]()
                    {
                        return Bridge->GetShapeBesideUsPhase() ==
                            EEchoesShapeBesideUsPhase::Complete;
                    }),
                TEXT("Mission 08 completes through guarded ordinary play")) ||
            !VerifyCompletion(8, EEchoesCampaignCommitStatus::Added) ||
            !AdvanceToNextMission(8))
        {
            return false;
        }

        // Mission 09: Reserve Authority, with a real quick-save boundary.
        const FEchoesReserveAuthorityPlan M09Plan =
            Bridge->GetReserveAuthorityPlan();
        const FEchoesObjectiveSnapshot M09Start =
            Bridge->GetLocalObjectiveSnapshot();
        const TArray<EntityId> M09Workers =
            FindOwnedEntities(Bridge, EntityType::Worker);
        if (!Require(
                M09Workers.Num() >= 2,
                TEXT("Mission 09 exposes two construction workers")) ||
            !Require(
                Move(M09Start.ReserveAuthorityMaraId, M09Plan.AuthoritySite),
                TEXT("Mission 09 Mara accepts the authority route")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge]()
                    {
                        return Bridge->GetReserveAuthorityPhase() ==
                            EEchoesReserveAuthorityPhase::
                                AllocateFirstDistrict;
                    },
                    3000),
                TEXT("Mission 09 secures Mara's authority")) ||
            !Require(
                Bridge->IssueBuildCommand(
                    M09Workers[0],
                    EntityType::Dropoff,
                    Bridge->SimToWorld(
                        FEchoesReserveAuthorityMissionModel::
                            RelaySiteForDistrict(
                                EEchoesCityDistrict::LifeSupport)),
                    Feedback),
                TEXT("Mission 09 allocates Life Support")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge]()
                    {
                        return Bridge->GetReserveAuthorityPhase() ==
                            EEchoesReserveAuthorityPhase::
                                AllocateSecondDistrict;
                    },
                    3400),
                TEXT("Mission 09 opens its second allocation")))
        {
            return false;
        }
        const FString M09QuickSavePath = PreserveQuickSaveFamily();
        if (!Require(
                !M09QuickSavePath.IsEmpty() &&
                    Bridge->QuickSaveScenario(Feedback) &&
                    IFileManager::Get().FileExists(*M09QuickSavePath),
                TEXT("Mission 09 writes its ledger-bound quick save")) ||
            !Require(
                Bridge->QuickLoadScenario(Feedback) &&
                    Bridge->GetReserveAuthorityPhase() ==
                        EEchoesReserveAuthorityPhase::
                            AllocateSecondDistrict &&
                    Bridge->GetLocalObjectiveSnapshot().bLifeSupportPowered,
                TEXT("Mission 09 restores its exact allocation state")) ||
            !Require(
                Bridge->IssueBuildCommand(
                    M09Workers[1],
                    EntityType::Dropoff,
                    Bridge->SimToWorld(
                        FEchoesReserveAuthorityMissionModel::
                            RelaySiteForDistrict(
                                EEchoesCityDistrict::Transit)),
                    Feedback),
                TEXT("Mission 09 allocates Transit")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge]()
                    {
                        return Bridge->GetReserveAuthorityPhase() ==
                                   EEchoesReserveAuthorityPhase::
                                       ReachDeferredDistrict &&
                            Bridge->GetReserveAuthorityDeferredDistrict() ==
                                EEchoesCityDistrict::Archive;
                    },
                    3400),
                TEXT("Mission 09 defers Archive after exactly two districts")) ||
            !Require(
                Move(
                    M09Start.ReserveAuthorityMaraId,
                    FEchoesCityReserveMissionModel::SiteForDistrict(
                        EEchoesCityDistrict::Archive)),
                TEXT("Mission 09 Mara accepts the deferred Archive route")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge]()
                    {
                        return Bridge->GetReserveAuthorityPhase() ==
                            EEchoesReserveAuthorityPhase::Complete;
                    },
                    3600),
                TEXT("Mission 09 completes through ordinary play")) ||
            !VerifyCompletion(9, EEchoesCampaignCommitStatus::Added) ||
            !AdvanceToNextMission(9))
        {
            return false;
        }

        // Mission 10: The Choir at Lume Reach.
        const FEchoesChoirAtLumeReachPlan M10Plan =
            Bridge->GetChoirAtLumeReachPlan();
        const FEchoesObjectiveSnapshot M10Start =
            Bridge->GetLocalObjectiveSnapshot();
        const TArray<EntityId> M10Workers =
            FindOwnedEntities(Bridge, EntityType::Worker);
        if (!Require(
                M10Workers.Num() >= 2,
                TEXT("Mission 10 exposes two Kharuun workers")) ||
            !Require(
                Move(M10Start.ChoirAtLumeReachOruunId, M10Plan.ContactSite),
                TEXT("Mission 10 Oruun accepts public contact")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge]()
                    {
                        return Bridge->GetChoirAtLumeReachPhase() ==
                            EEchoesChoirAtLumeReachPhase::
                                ResolveDeferredLiability;
                    },
                    3200),
                TEXT("Mission 10 establishes contact")) ||
            !Require(
                Bridge->IssueCommand(
                    CommandType::ToggleWaystoneRoot,
                    M10Start.ChoirAtLumeReachWaystoneId,
                    0,
                    FVector::ZeroVector,
                    FutureWellChoice::Dormant,
                    Feedback),
                TEXT("Mission 10 Waystone accepts uproot")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge, M10Start]()
                    {
                        const Entity* Current = Bridge->FindEntity(
                            M10Start.ChoirAtLumeReachWaystoneId);
                        return Current != nullptr &&
                            Current->waystoneMode ==
                                echoes::sim::WaystoneMode::Mobile;
                    },
                    400),
                TEXT("Mission 10 Waystone becomes mobile")) ||
            !Require(
                Move(
                    M10Start.ChoirAtLumeReachWaystoneId,
                    M10Plan.LiabilitySite),
                TEXT("Mission 10 Waystone accepts the liability route")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge, M10Start, M10Plan]()
                    {
                        return IsAtSite(
                            Bridge,
                            M10Start.ChoirAtLumeReachWaystoneId,
                            M10Plan.LiabilitySite);
                    },
                    5200),
                TEXT("Mission 10 Waystone reaches the liability site")) ||
            !Require(
                Bridge->IssueCommand(
                    CommandType::ToggleWaystoneRoot,
                    M10Start.ChoirAtLumeReachWaystoneId,
                    0,
                    FVector::ZeroVector,
                    FutureWellChoice::Dormant,
                    Feedback),
                TEXT("Mission 10 Waystone accepts root")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge]()
                    {
                        return Bridge->GetChoirAtLumeReachPhase() ==
                            EEchoesChoirAtLumeReachPhase::RaiseFirstAnchor;
                    },
                    500),
                TEXT("Mission 10 opens its first anchor")) ||
            !Require(
                Bridge->IssueBuildCommand(
                    M10Workers[0],
                    EntityType::UtilityStructure,
                    Bridge->SimToWorld(M10Plan.FirstAnchorSite),
                    Feedback),
                TEXT("Mission 10 accepts its first anchor")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge]()
                    {
                        return Bridge->GetChoirAtLumeReachPhase() ==
                            EEchoesChoirAtLumeReachPhase::RaiseSecondAnchor;
                    },
                    4200),
                TEXT("Mission 10 opens its second anchor")) ||
            !Require(
                Bridge->IssueBuildCommand(
                    M10Workers[1],
                    EntityType::UtilityStructure,
                    Bridge->SimToWorld(M10Plan.SecondAnchorSite),
                    Feedback),
                TEXT("Mission 10 accepts its second anchor")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge]()
                    {
                        return Bridge->GetChoirAtLumeReachPhase() ==
                            EEchoesChoirAtLumeReachPhase::CommitFutureWell;
                    },
                    4200),
                TEXT("Mission 10 opens its Future Well")) ||
            !Require(
                Move(M10Workers[0], Vec2::FromTiles(30, 41)),
                TEXT("Mission 10 worker accepts the Well approach")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge, M10Start]()
                    {
                        return Bridge->GetSimulation()->IsEntityVisibleTo(
                            UEchoesSimulationSubsystem::LocalPlayerId,
                            M10Start.ChoirAtLumeReachWellId);
                    },
                    1800),
                TEXT("Mission 10 legitimately reveals the Future Well")) ||
            !Require(
                Bridge->IssueCommand(
                    CommandType::FutureWell,
                    M10Workers[0],
                    M10Start.ChoirAtLumeReachWellId,
                    Bridge->SimToWorld(M10Plan.FutureWellSite),
                    Spec.LumeChoice,
                    Feedback),
                TEXT("Mission 10 accepts the route's Lume protocol")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge]()
                    {
                        return Bridge->GetChoirAtLumeReachPhase() ==
                            EEchoesChoirAtLumeReachPhase::ResolveFutureWell;
                    },
                    2200),
                TEXT("Mission 10 records its Lume protocol")) ||
            !Require(
                Move(
                    M10Start.ChoirAtLumeReachOruunId,
                    FEchoesChoirAtLumeReachMissionModel::
                        ResolutionSiteForChoice(Spec.LumeChoice)),
                TEXT("Mission 10 Oruun accepts the resolution route")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge]()
                    {
                        return Bridge->GetChoirAtLumeReachPhase() ==
                            EEchoesChoirAtLumeReachPhase::Complete;
                    },
                    5400),
                TEXT("Mission 10 completes through ordinary play")) ||
            !VerifyCompletion(10, EEchoesCampaignCommitStatus::Added) ||
            !AdvanceToNextMission(10))
        {
            return false;
        }
        return true;
    };

    const auto RunMissionsElevenThroughFourteen = [
        Bridge, &Feedback, &Require, &VerifyCompletion,
        &AdvanceToNextMission](const FFreshRouteSpec& Spec)
    {
        const auto Move = [Bridge, &Feedback](EntityId Actor, const Vec2& Site)
        {
            return Bridge->IssueCommand(
                CommandType::Move,
                Actor,
                0,
                Bridge->SimToWorld(Site),
                FutureWellChoice::Dormant,
                Feedback);
        };

        // Mission 11: No Neutral Ledger.
        const FEchoesNoNeutralLedgerPlan M11Plan =
            Bridge->GetNoNeutralLedgerPlan();
        const FEchoesObjectiveSnapshot M11Start =
            Bridge->GetLocalObjectiveSnapshot();
        const TArray<EntityId> M11Workers =
            FindOwnedEntities(Bridge, EntityType::Worker);
        const Vec2 ExpectedM11Rally =
            TestOwnedNoNeutralRallySite(Spec.LumeChoice);
        const auto CompleteMissionEleven = [
            Bridge, M11Start, M11Plan, ExpectedM11Rally, &Feedback]()
        {
            const bool bComplete = TickUntil(
                Bridge,
                [Bridge]()
                {
                    return Bridge->GetNoNeutralLedgerPhase() ==
                        EEchoesNoNeutralLedgerPhase::Complete;
                },
                6200);
            if (bComplete)
            {
                return true;
            }

            const FEchoesObjectiveSnapshot Current =
                Bridge->GetLocalObjectiveSnapshot();
            const echoes::sim::Simulation* Simulation =
                Bridge->GetSimulation();
            const uint64 CurrentTick = Simulation != nullptr
                ? Simulation->CurrentTick()
                : 0;
            Feedback = FString::Printf(
                TEXT("[M11_DIAGNOSTIC] tick=%llu phase=%u outcome=%u planKey=%u protocol=%u rally=(%d,%d) localCore={intact=%s hp=%d/%d} facts={route=%s interfaces=%s districtA=%s districtB=%s evidence=%s protocol=%s rallied=%s reshapeExpired=%s} %s %s %s %s %s %s %s %s"),
                static_cast<unsigned long long>(CurrentTick),
                static_cast<uint8>(Current.NoNeutralLedgerPhase),
                static_cast<uint8>(Current.Outcome),
                M11Plan.StablePlanKey,
                static_cast<uint8>(M11Plan.LumeProtocol),
                ExpectedM11Rally.x.FloorToInt(),
                ExpectedM11Rally.y.FloorToInt(),
                Current.bLocalCoreIntact ? TEXT("true") : TEXT("false"),
                Current.LocalCoreHitPoints,
                Current.LocalCoreMaxHitPoints,
                Current.bNoNeutralRouteSecured ? TEXT("true") : TEXT("false"),
                Current.bNoNeutralPublicInterfacesIntact
                    ? TEXT("true") : TEXT("false"),
                Current.bNoNeutralFirstDistrictIntegrated
                    ? TEXT("true") : TEXT("false"),
                Current.bNoNeutralSecondDistrictIntegrated
                    ? TEXT("true") : TEXT("false"),
                Current.bNoNeutralEvidenceAttested
                    ? TEXT("true") : TEXT("false"),
                Current.bNoNeutralProtocolApplied
                    ? TEXT("true") : TEXT("false"),
                Current.bNoNeutralCoalitionRallied
                    ? TEXT("true") : TEXT("false"),
                Current.bNoNeutralReshapeWindowExpired
                    ? TEXT("true") : TEXT("false"),
                *DescribeFreshJourneyEntity(
                    TEXT("oruun"),
                    M11Start.NoNeutralOruunId,
                    Bridge->FindEntity(M11Start.NoNeutralOruunId)),
                *DescribeFreshJourneyEntity(
                    TEXT("waystone"),
                    M11Start.NoNeutralWaystoneId,
                    Bridge->FindEntity(M11Start.NoNeutralWaystoneId)),
                *DescribeFreshJourneyEntity(
                    TEXT("witness"),
                    M11Start.NoNeutralLedgerWitnessId,
                    Bridge->FindEntity(
                        M11Start.NoNeutralLedgerWitnessId)),
                *DescribeFreshJourneyEntity(
                    TEXT("well"),
                    M11Start.NoNeutralWellId,
                    Bridge->FindEntity(M11Start.NoNeutralWellId)),
                *DescribeFreshJourneyEntity(
                    TEXT("districtA"),
                    M11Start.NoNeutralFirstDistrictInterfaceId,
                    Bridge->FindEntity(
                        M11Start.NoNeutralFirstDistrictInterfaceId)),
                *DescribeFreshJourneyEntity(
                    TEXT("districtB"),
                    M11Start.NoNeutralSecondDistrictInterfaceId,
                    Bridge->FindEntity(
                        M11Start.NoNeutralSecondDistrictInterfaceId)),
                *DescribeFreshJourneyEntity(
                    TEXT("meridianEvidence"),
                    M11Start.NoNeutralMeridianEvidenceInterfaceId,
                    Bridge->FindEntity(
                        M11Start.NoNeutralMeridianEvidenceInterfaceId)),
                *DescribeFreshJourneyEntity(
                    TEXT("kharuunEvidence"),
                    M11Start.NoNeutralKharuunEvidenceInterfaceId,
                    Bridge->FindEntity(
                        M11Start.NoNeutralKharuunEvidenceInterfaceId)));
            return false;
        };
        Vec2 M11FirstLink;
        Vec2 M11SecondLink;
        if (!Require(
                M11Plan.RallySite == ExpectedM11Rally,
                FString::Printf(
                    TEXT("Mission 11 route %s maps its protocol to the independent literal rally (%d,%d)"),
                    Spec.Label,
                    ExpectedM11Rally.x.FloorToInt(),
                    ExpectedM11Rally.y.FloorToInt())) ||
            !Require(
                M11Workers.Num() >= 2 &&
                    M11Start.NoNeutralWaystoneId != 0,
                TEXT("Mission 11 exposes its Waystone and two workers")) ||
            !Require(
                Bridge->IssueCommand(
                    CommandType::ToggleWaystoneRoot,
                    M11Start.NoNeutralWaystoneId,
                    0,
                    FVector::ZeroVector,
                    FutureWellChoice::Dormant,
                    Feedback),
                TEXT("Mission 11 Waystone accepts uproot")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge, M11Start]()
                    {
                        const Entity* Current = Bridge->FindEntity(
                            M11Start.NoNeutralWaystoneId);
                        return Current != nullptr &&
                            Current->waystoneMode ==
                                echoes::sim::WaystoneMode::Mobile;
                    },
                    500),
                TEXT("Mission 11 Waystone becomes mobile")) ||
            !Require(
                Move(M11Start.NoNeutralWaystoneId, M11Plan.RouteSite),
                TEXT("Mission 11 Waystone accepts its inherited route")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge, M11Start, M11Plan]()
                    {
                        // The route site sits inside a five-tile public gate,
                        // so the three-tile Waystone footprint is only clear
                        // once the move has actually completed there; rooting
                        // while still travelling is a legitimate rejection.
                        const Entity* Current = Bridge->FindEntity(
                            M11Start.NoNeutralWaystoneId);
                        return Current != nullptr &&
                            Current->order.type ==
                                echoes::sim::OrderType::None &&
                            IsAtSite(
                                Bridge,
                                M11Start.NoNeutralWaystoneId,
                                M11Plan.RouteSite);
                    },
                    5200),
                TEXT("Mission 11 Waystone reaches its route")) ||
            !Require(
                Bridge->IssueCommand(
                    CommandType::ToggleWaystoneRoot,
                    M11Start.NoNeutralWaystoneId,
                    0,
                    FVector::ZeroVector,
                    FutureWellChoice::Dormant,
                    Feedback),
                TEXT("Mission 11 Waystone accepts root")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge]()
                    {
                        return Bridge->GetNoNeutralLedgerPhase() ==
                            EEchoesNoNeutralLedgerPhase::
                                IntegrateDistrictContributions;
                    },
                    600),
                TEXT("Mission 11 secures the inherited route")) ||
            !Require(
                FindValidBuildSite(
                    Bridge,
                    M11Plan.FirstDistrictSite,
                    3,
                    M11FirstLink),
                TEXT("Mission 11 exposes a valid first district link")) ||
            !Require(
                FindValidBuildSite(
                    Bridge,
                    M11Plan.SecondDistrictSite,
                    3,
                    M11SecondLink),
                TEXT("Mission 11 exposes a valid second district link")) ||
            !Require(
                Bridge->IssueBuildCommand(
                    M11Workers[0],
                    EntityType::UtilityStructure,
                    Bridge->SimToWorld(M11FirstLink),
                    Feedback),
                TEXT("Mission 11 accepts the first district link")) ||
            !Require(
                Bridge->IssueBuildCommand(
                    M11Workers[1],
                    EntityType::UtilityStructure,
                    Bridge->SimToWorld(M11SecondLink),
                    Feedback),
                TEXT("Mission 11 accepts the second district link")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge]()
                    {
                        return Bridge->GetNoNeutralLedgerPhase() ==
                            EEchoesNoNeutralLedgerPhase::
                                AttestEvidenceChannels;
                    },
                    5600),
                TEXT("Mission 11 integrates both district inputs")) ||
            !Require(
                Move(
                    M11Start.NoNeutralOruunId,
                    M11Plan.KharuunEvidenceSite),
                TEXT("Mission 11 Oruun accepts Kharuun evidence")) ||
            !Require(
                Move(
                    M11Start.NoNeutralLedgerWitnessId,
                    M11Plan.MeridianEvidenceSite),
                TEXT("Mission 11 witness accepts Meridian evidence")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge]()
                    {
                        return Bridge->GetNoNeutralLedgerPhase() ==
                            EEchoesNoNeutralLedgerPhase::
                                ApplyRecordedProtocol;
                    },
                    5400),
                TEXT("Mission 11 attests both evidence channels")) ||
            !Require(
                Move(M11Workers[0], Vec2::FromTiles(30, 47)),
                TEXT("Mission 11 worker accepts the Well approach")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge, M11Start]()
                    {
                        return Bridge->GetSimulation()->IsEntityVisibleTo(
                            UEchoesSimulationSubsystem::LocalPlayerId,
                            M11Start.NoNeutralWellId);
                    },
                    2200),
                TEXT("Mission 11 legitimately reveals the Future Well")) ||
            !Require(
                Bridge->IssueCommand(
                    CommandType::FutureWell,
                    M11Workers[0],
                    M11Start.NoNeutralWellId,
                    Bridge->SimToWorld(M11Plan.FutureWellSite),
                    Spec.LumeChoice,
                    Feedback),
                TEXT("Mission 11 accepts only the recorded Lume protocol")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge]()
                    {
                        return Bridge->GetNoNeutralLedgerPhase() ==
                            EEchoesNoNeutralLedgerPhase::RallyCoalition;
                    },
                    2600),
                TEXT("Mission 11 applies the recorded protocol")) ||
            !Require(
                Move(M11Start.NoNeutralOruunId, ExpectedM11Rally),
                TEXT("Mission 11 Oruun accepts the rally")) ||
            !Require(
                Move(
                    M11Start.NoNeutralLedgerWitnessId,
                    ExpectedM11Rally),
                TEXT("Mission 11 witness accepts the rally")) ||
            !Require(
                CompleteMissionEleven(),
                TEXT("Mission 11 completes through ordinary play")) ||
            !VerifyCompletion(11, EEchoesCampaignCommitStatus::Added) ||
            !AdvanceToNextMission(11))
        {
            return false;
        }

        // Mission 12: The Future That Won.
        const FEchoesFutureThatWonPlan M12Plan =
            Bridge->GetFutureThatWonPlan();
        const FEchoesObjectiveSnapshot M12Start =
            Bridge->GetLocalObjectiveSnapshot();
        const TArray<EntityId> M12Workers =
            FindOwnedEntities(Bridge, EntityType::Worker);
        Vec2 M12FirstLink;
        Vec2 M12SecondLink;
        const Vec2 ExpectedM12WellSite =
            TestOwnedNoNeutralRallySite(Spec.LumeChoice);
        const Vec2 M12WellApproach =
            TestOwnedFutureWonWellApproach(Spec.LumeChoice);
        // Nearest passable stand: the Lume-reach terrain seals different
        // tile strips per inherited branch (the Reshape fold blocks
        // x=30-34, y=34-35, which contains the Transit contribution site),
        // so an inherited plan site can be unwalkable on exactly one route
        // variant. The mission facts measure presence within the 3-tile
        // site radius, so the Move destination is the site itself when
        // passable, else the nearest passable tile within that radius
        // (ring search, fixed scan order, deterministic) — the same thing
        // a player does when a fold seals the tile: stand beside it.
        const auto NearestPassableStand = [Bridge](const Vec2& Site)
        {
            const echoes::sim::Simulation* Simulation =
                Bridge->GetSimulation();
            if (Simulation == nullptr ||
                Simulation->IsPositionPassable(Site))
            {
                return Site;
            }
            const int32 SiteTileX = Site.x.FloorToInt();
            const int32 SiteTileY = Site.y.FloorToInt();
            for (int32 Ring = 1; Ring <= 3; ++Ring)
            {
                for (int32 OffsetY = -Ring; OffsetY <= Ring; ++OffsetY)
                {
                    for (int32 OffsetX = -Ring; OffsetX <= Ring; ++OffsetX)
                    {
                        if (FMath::Max(
                                FMath::Abs(OffsetX),
                                FMath::Abs(OffsetY)) != Ring ||
                            OffsetX * OffsetX + OffsetY * OffsetY > 9)
                        {
                            continue;
                        }
                        const Vec2 Candidate = Vec2::FromTiles(
                            SiteTileX + OffsetX, SiteTileY + OffsetY);
                        if (Simulation->IsPositionPassable(Candidate))
                        {
                            return Candidate;
                        }
                    }
                }
            }
            return Site;
        };
        if (!Require(
                M12Plan.FutureWellSite == ExpectedM12WellSite,
                FString::Printf(
                    TEXT("Mission 12 route %s inherits the independent literal Future Well (%d,%d)"),
                    Spec.Label,
                    ExpectedM12WellSite.x.FloorToInt(),
                    ExpectedM12WellSite.y.FloorToInt())) ||
            !Require(
                M12Plan.FutureWellSite != M12Plan.FirstDistrictInputSite &&
                    M12Plan.FutureWellSite !=
                        M12Plan.SecondDistrictInputSite &&
                    M12Plan.FutureWellSite != M12Plan.MeridianReadbackSite &&
                    M12Plan.FutureWellSite != M12Plan.KharuunReadbackSite,
                FString::Printf(
                    TEXT("Mission 12 route %s keeps its Future Well distinct from contributing districts and public readbacks"),
                    Spec.Label)) ||
            !Require(
                Bridge->GetSimulation()->TerrainAt(
                    M12WellApproach.x.FloorToInt(),
                    M12WellApproach.y.FloorToInt()) ==
                    echoes::sim::Terrain::Open &&
                    Bridge->GetSimulation()->IsPositionPassable(
                        M12WellApproach),
                FString::Printf(
                    TEXT("Mission 12 route %s owns an open, passable literal Well approach (%d,%d)"),
                    Spec.Label,
                    M12WellApproach.x.FloorToInt(),
                    M12WellApproach.y.FloorToInt())) ||
            !Require(
                M12Workers.Num() >= 2,
                TEXT("Mission 12 exposes two Kharuun workers")) ||
            !Require(
                Move(
                    M12Start.FutureWonOruunId,
                    M12Plan.KharuunReadbackSite),
                TEXT("Mission 12 Oruun accepts public readback")) ||
            !Require(
                Move(
                    M12Start.FutureWonVerifierId,
                    M12Plan.MeridianReadbackSite),
                TEXT("Mission 12 verifier accepts public readback")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge]()
                    {
                        return Bridge->GetFutureThatWonPhase() ==
                            EEchoesFutureThatWonPhase::
                                VerifyRecordedInputs;
                    },
                    5600),
                TEXT("Mission 12 establishes independent readback")) ||
            !Require(
                FindValidBuildSite(
                    Bridge,
                    M12Plan.FirstDistrictInputSite,
                    3,
                    M12FirstLink),
                TEXT("Mission 12 exposes a valid first input link")) ||
            !Require(
                FindValidBuildSite(
                    Bridge,
                    M12Plan.SecondDistrictInputSite,
                    3,
                    M12SecondLink),
                TEXT("Mission 12 exposes a valid second input link")) ||
            !Require(
                Bridge->IssueBuildCommand(
                    M12Workers[0],
                    EntityType::UtilityStructure,
                    Bridge->SimToWorld(M12FirstLink),
                    Feedback),
                TEXT("Mission 12 accepts its first input link")) ||
            !Require(
                Bridge->IssueBuildCommand(
                    M12Workers[1],
                    EntityType::UtilityStructure,
                    Bridge->SimToWorld(M12SecondLink),
                    Feedback),
                TEXT("Mission 12 accepts its second input link")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge]()
                    {
                        return Bridge->GetFutureThatWonPhase() ==
                            EEchoesFutureThatWonPhase::BindRecordedProtocol;
                    },
                    5600),
                TEXT("Mission 12 verifies both recorded inputs")) ||
            !Require(
                Move(M12Workers[0], M12WellApproach),
                FString::Printf(
                    TEXT("Mission 12 worker accepts the literal Well approach (%d,%d)"),
                    M12WellApproach.x.FloorToInt(),
                    M12WellApproach.y.FloorToInt())) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge, M12Start]()
                    {
                        return Bridge->GetSimulation()->IsEntityVisibleTo(
                            UEchoesSimulationSubsystem::LocalPlayerId,
                            M12Start.FutureWonWellId);
                    },
                    2600),
                TEXT("Mission 12 legitimately reveals the Future Well")) ||
            !Require(
                Bridge->IssueCommand(
                    CommandType::FutureWell,
                    M12Workers[0],
                    M12Start.FutureWonWellId,
                    Bridge->SimToWorld(M12Plan.FutureWellSite),
                    Spec.LumeChoice,
                    Feedback),
                TEXT("Mission 12 accepts the recorded Lume protocol")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge]()
                    {
                        return Bridge->GetFutureThatWonPhase() ==
                            EEchoesFutureThatWonPhase::
                                HoldStabilityWindow;
                    },
                    2600),
                TEXT("Mission 12 starts its stability hold")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge]()
                    {
                        return Bridge->GetFutureThatWonPhase() ==
                            EEchoesFutureThatWonPhase::
                                ObserveDistrictReadbacks;
                    },
                    420),
                TEXT("Mission 12 holds the complete stability window")) ||
            !Require(
                Move(
                    M12Start.FutureWonOruunId,
                    NearestPassableStand(
                        M12Plan.FirstDistrictInputSite)),
                TEXT("Mission 12 Oruun accepts the first readback")) ||
            !Require(
                Move(
                    M12Start.FutureWonVerifierId,
                    NearestPassableStand(
                        M12Plan.SecondDistrictInputSite)),
                TEXT("Mission 12 verifier accepts the second readback")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge]()
                    {
                        return Bridge->GetFutureThatWonPhase() ==
                            EEchoesFutureThatWonPhase::Complete;
                    },
                    6200),
                TEXT("Mission 12 completes through ordinary play")) ||
            !VerifyCompletion(12, EEchoesCampaignCommitStatus::Added) ||
            !AdvanceToNextMission(12))
        {
            return false;
        }

        // Mission 13: Assembly of the Missing.
        const FEchoesAssemblyOfTheMissingPlan M13Plan =
            Bridge->GetAssemblyOfTheMissingPlan();
        const FEchoesObjectiveSnapshot M13Start =
            Bridge->GetLocalObjectiveSnapshot();
        const TArray<EntityId> M13Workers =
            FindOwnedEntities(Bridge, EntityType::Worker);
        const TArray<EntityId> M13Cores =
            FindOwnedEntities(Bridge, EntityType::CommandCore);
        const EntityId M13WorkerId =
            M13Workers.IsEmpty() ? 0 : M13Workers[0];
        const EntityId M13CoreId = M13Cores.IsEmpty() ? 0 : M13Cores[0];
        Vec2 M13Link;
        const auto OpenMissionThirteenObservation = [
            Bridge, M13Start, M13Plan, M13WorkerId, M13CoreId,
            &M13Link, &Feedback]()
        {
            const bool bOpened = TickUntil(
                Bridge,
                [Bridge]()
                {
                    return Bridge->GetAssemblyOfTheMissingPhase() ==
                        EEchoesAssemblyOfTheMissingPhase::ObserveAssembly;
                },
                7000);
            if (bOpened)
            {
                return true;
            }

            const FEchoesObjectiveSnapshot Current =
                Bridge->GetLocalObjectiveSnapshot();
            const echoes::sim::Simulation* Simulation =
                Bridge->GetSimulation();
            EntityId LinkId = 0;
            const Entity* Link = nullptr;
            if (Simulation != nullptr)
            {
                for (const Entity& Candidate : Simulation->Entities())
                {
                    if (Candidate.owner ==
                            UEchoesSimulationSubsystem::LocalPlayerId &&
                        Candidate.type == EntityType::UtilityStructure &&
                        Candidate.position == M13Link)
                    {
                        LinkId = Candidate.id;
                        Link = &Candidate;
                        break;
                    }
                }
            }
            Feedback = FString::Printf(
                TEXT("[M13_LINK_DIAGNOSTIC] tick=%llu phase=%u outcome=%u planKey=%u index=(%d,%d) link=(%d,%d) facts={coreIntact=%s coreHp=%d/%d publicInterfaces=%s readback=%s linked=%s meridianWitness=%s kharuunWitness=%s} %s %s %s %s %s %s %s %s"),
                static_cast<unsigned long long>(
                    Simulation != nullptr ? Simulation->CurrentTick() : 0),
                static_cast<uint8>(Current.AssemblyOfTheMissingPhase),
                static_cast<uint8>(Current.Outcome),
                M13Plan.StablePlanKey,
                M13Plan.CrownfallIndexSite.x.FloorToInt(),
                M13Plan.CrownfallIndexSite.y.FloorToInt(),
                M13Link.x.FloorToInt(),
                M13Link.y.FloorToInt(),
                Current.bLocalCoreIntact ? TEXT("true") : TEXT("false"),
                Current.LocalCoreHitPoints,
                Current.LocalCoreMaxHitPoints,
                Current.bAssemblyPublicInterfacesIntact
                    ? TEXT("true") : TEXT("false"),
                Current.bAssemblyPublicRecordReadbackEstablished
                    ? TEXT("true") : TEXT("false"),
                Current.bAssemblyCrownfallIndexLinked
                    ? TEXT("true") : TEXT("false"),
                Current.bAssemblyMeridianWitnessObserved
                    ? TEXT("true") : TEXT("false"),
                Current.bAssemblyKharuunWitnessObserved
                    ? TEXT("true") : TEXT("false"),
                *DescribeFreshJourneyEntity(
                    TEXT("core"),
                    M13CoreId,
                    Bridge->FindEntity(M13CoreId)),
                *DescribeFreshJourneyEntity(
                    TEXT("oruun"),
                    M13Start.AssemblyOruunId,
                    Bridge->FindEntity(M13Start.AssemblyOruunId)),
                *DescribeFreshJourneyEntity(
                    TEXT("verifier"),
                    M13Start.AssemblyVerifierId,
                    Bridge->FindEntity(M13Start.AssemblyVerifierId)),
                *DescribeFreshJourneyEntity(
                    TEXT("meridianRecord"),
                    M13Start.AssemblyMeridianPublicRecordInterfaceId,
                    Bridge->FindEntity(
                        M13Start.AssemblyMeridianPublicRecordInterfaceId)),
                *DescribeFreshJourneyEntity(
                    TEXT("kharuunRecord"),
                    M13Start.AssemblyKharuunPublicRecordInterfaceId,
                    Bridge->FindEntity(
                        M13Start.AssemblyKharuunPublicRecordInterfaceId)),
                *DescribeFreshJourneyEntity(
                    TEXT("crownfallIndex"),
                    M13Start.AssemblyCrownfallIndexInterfaceId,
                    Bridge->FindEntity(
                        M13Start.AssemblyCrownfallIndexInterfaceId)),
                *DescribeFreshJourneyEntity(
                    TEXT("worker"),
                    M13WorkerId,
                    Bridge->FindEntity(M13WorkerId)),
                *DescribeFreshJourneyEntity(TEXT("link"), LinkId, Link));
            return false;
        };
        if (!Require(
                M13Plan.CrownfallIndexSite == ExpectedM11Rally,
                FString::Printf(
                    TEXT("Mission 13 route %s propagates the independent literal Crownfall index (%d,%d)"),
                    Spec.Label,
                    ExpectedM11Rally.x.FloorToInt(),
                    ExpectedM11Rally.y.FloorToInt())) ||
            !Require(
                !M13Workers.IsEmpty(),
                TEXT("Mission 13 exposes a Kharuun construction worker")) ||
            !Require(
                Move(
                    M13Start.AssemblyOruunId,
                    M13Plan.KharuunPublicRecordSite),
                TEXT("Mission 13 Oruun accepts public-record readback")) ||
            !Require(
                Move(
                    M13Start.AssemblyVerifierId,
                    M13Plan.MeridianPublicRecordSite),
                TEXT("Mission 13 verifier accepts public-record readback")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge]()
                    {
                        return Bridge->GetAssemblyOfTheMissingPhase() ==
                            EEchoesAssemblyOfTheMissingPhase::
                                LinkCrownfallIndex;
                    },
                    6000),
                TEXT("Mission 13 establishes paired public readback")) ||
            !Require(
                FindValidBuildSite(
                    Bridge,
                    M13Plan.CrownfallIndexSite,
                    3,
                    M13Link),
                TEXT("Mission 13 exposes a valid Crownfall link")) ||
            !Require(
                Bridge->IssueBuildCommand(
                    M13Workers[0],
                    EntityType::UtilityStructure,
                    Bridge->SimToWorld(M13Link),
                    Feedback),
                TEXT("Mission 13 accepts the Crownfall link")) ||
            !Require(
                OpenMissionThirteenObservation(),
                TEXT("Mission 13 opens independent observation")) ||
            !Require(
                Move(
                    M13Start.AssemblyOruunId,
                    M13Plan.KharuunAssemblyWitnessSite),
                TEXT("Mission 13 Oruun accepts assembly observation")) ||
            !Require(
                Move(
                    M13Start.AssemblyVerifierId,
                    M13Plan.MeridianAssemblyWitnessSite),
                TEXT("Mission 13 verifier accepts assembly observation")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge]()
                    {
                        return Bridge->GetAssemblyOfTheMissingPhase() ==
                            EEchoesAssemblyOfTheMissingPhase::Complete;
                    },
                    7000),
                TEXT("Mission 13 completes through ordinary play")) ||
            !VerifyCompletion(13, EEchoesCampaignCommitStatus::Added) ||
            !AdvanceToNextMission(13))
        {
            return false;
        }

        // Mission 14: Several Voices, One Command.
        const FEchoesSeveralVoicesOneCommandPlan M14Plan =
            Bridge->GetSeveralVoicesOneCommandPlan();
        FEchoesObjectiveSnapshot M14Objective =
            Bridge->GetLocalObjectiveSnapshot();
        const TArray<EntityId> M14Workers = FindOwnedEntities(
            Bridge,
            EntityType::Worker,
            echoes::sim::Faction::HollowChoir);
        if (!Require(
                M14Plan.CrisisAnchorSite == ExpectedM11Rally,
                FString::Printf(
                    TEXT("Mission 14 route %s propagates the independent literal crisis anchor (%d,%d)"),
                    Spec.Label,
                    ExpectedM11Rally.x.FloorToInt(),
                    ExpectedM11Rally.y.FloorToInt())) ||
            !Require(
                !M14Workers.IsEmpty() &&
                    M14Objective.SeveralVoicesResearchLoomId != 0,
                TEXT("Mission 14 exposes its Choir worker and Research Loom")) ||
            !Require(
                Bridge->IssueResearchCommand(
                    M14Objective.SeveralVoicesResearchLoomId,
                    ResearchType::ChoirHeldAlternatives,
                    Feedback),
                TEXT("Mission 14 accepts Held Alternatives research")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge]()
                    {
                        return Bridge->GetSeveralVoicesOneCommandPhase() ==
                            EEchoesSeveralVoicesOneCommandPhase::
                                ResolveIncompatibleVoices;
                    },
                    1000),
                TEXT("Mission 14 completes Held Alternatives")) ||
            !Require(
                Move(
                    M14Objective.SeveralVoicesPossibleVoiceId,
                    M14Plan.PossibleVoiceSite),
                TEXT("Mission 14 Possible voice accepts its site")) ||
            !Require(
                Move(
                    M14Objective.SeveralVoicesManifestVoiceId,
                    M14Plan.ManifestVoiceSite),
                TEXT("Mission 14 Manifest voice accepts its site")) ||
            !Require(
                Move(
                    M14Objective.SeveralVoicesNemeId,
                    M14Plan.NemeCommandSite),
                TEXT("Mission 14 Neme accepts the command site")) ||
            !Require(
                Bridge->IssueChoirReconciliation(
                    M14Objective.SeveralVoicesPossibleVoiceId,
                    ChoirIdentityState::Possible,
                    Feedback),
                TEXT("Mission 14 accepts Possible reconciliation")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge]()
                    {
                        return Bridge->GetSeveralVoicesOneCommandPhase() ==
                            EEchoesSeveralVoicesOneCommandPhase::
                                ResearchSharedResolution;
                    },
                    3000),
                TEXT("Mission 14 resolves both voices at their sites")))
        {
            return false;
        }
        M14Objective = Bridge->GetLocalObjectiveSnapshot();
        if (!Require(
                Bridge->IssueResearchCommand(
                    M14Objective.SeveralVoicesResearchLoomId,
                    ResearchType::ChoirSharedResolution,
                    Feedback),
                TEXT("Mission 14 accepts Shared Resolution research")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge]()
                    {
                        return Bridge->GetSeveralVoicesOneCommandPhase() ==
                            EEchoesSeveralVoicesOneCommandPhase::AnchorCrisis;
                    },
                    1000),
                TEXT("Mission 14 opens the crisis anchor")))
        {
            return false;
        }
        Vec2 M14Anchor;
        if (!Require(
                FindValidBuildSite(
                    Bridge,
                    M14Plan.CrisisAnchorSite,
                    3,
                    M14Anchor),
                TEXT("Mission 14 exposes a valid Phase Anchor site")) ||
            !Require(
                Bridge->IssueBuildCommand(
                    M14Workers[0],
                    EntityType::UtilityStructure,
                    Bridge->SimToWorld(M14Anchor),
                    Feedback),
                TEXT("Mission 14 accepts its Phase Anchor")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge]()
                    {
                        return Bridge->GetSeveralVoicesOneCommandPhase() ==
                            EEchoesSeveralVoicesOneCommandPhase::
                                HoldSharedResolution;
                    },
                    4000),
                TEXT("Mission 14 starts its crisis hold")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge]()
                    {
                        return Bridge->GetSeveralVoicesOneCommandPhase() ==
                            EEchoesSeveralVoicesOneCommandPhase::Complete;
                    },
                    1000),
                TEXT("Mission 14 holds the complete Choir contract")) ||
            !VerifyCompletion(14, EEchoesCampaignCommitStatus::Added) ||
            !AdvanceToNextMission(14))
        {
            return false;
        }
        return true;
    };

    const auto RunMissionFifteen = [
        Bridge, Controller, &Feedback, &Require, &VerifyCompletion](
        const FFreshRouteSpec& Spec,
        EEchoesFinalResolution Resolution,
        EEchoesFinalResolution ExpectedRecordedResolution,
        EEchoesCampaignCommitStatus ExpectedStatus)
    {
        const FEchoesBrokenSunPlan Plan = Bridge->GetBrokenSunPlan();
        FEchoesObjectiveSnapshot Objective =
            Bridge->GetLocalObjectiveSnapshot();
        const TArray<EntityId> Workers = FindOwnedEntities(
            Bridge,
            EntityType::Worker,
            echoes::sim::Faction::HollowChoir);
        const TArray<EntityId> ResearchLooms = FindOwnedEntities(
            Bridge,
            EntityType::Barracks,
            echoes::sim::Faction::HollowChoir);
        const EEchoesFinalResolution CandidateResolutions[] = {
            EEchoesFinalResolution::Restoration,
            EEchoesFinalResolution::ControlledStabilization,
            EEchoesFinalResolution::Extinguishment,
            EEchoesFinalResolution::OpenEvolution};
        uint8 ObservedResolutionMask = 0;
        for (const EEchoesFinalResolution Candidate : CandidateResolutions)
        {
            const uint8 CandidateMask = TestOwnedResolutionBit(Candidate);
            const bool bExpectedAvailable =
                (Spec.ExpectedFinalResolutionMask & CandidateMask) != 0;
            const bool bObservedAvailable =
                FEchoesBrokenSunMissionModel::IsResolutionAvailable(
                    Plan, Candidate);
            if (!Require(
                    CandidateMask != 0 &&
                        bObservedAvailable == bExpectedAvailable,
                    FString::Printf(
                        TEXT("Mission 15 route %s maps resolution %u to independent literal bit 0x%02X"),
                        Spec.Label,
                        static_cast<uint8>(Candidate),
                        CandidateMask)))
            {
                return false;
            }
            if (bObservedAvailable)
            {
                ObservedResolutionMask = static_cast<uint8>(
                    ObservedResolutionMask | CandidateMask);
            }
        }
        if (!Require(
                !Workers.IsEmpty() && !ResearchLooms.IsEmpty() &&
                    Resolution != EEchoesFinalResolution::None &&
                    Spec.ExpectedFinalResolutionMask != 0 &&
                    Plan.FoundingDoctrine == Spec.FoundingChoice &&
                    Plan.RecordedProtocol == Spec.LumeChoice &&
                    Plan.StablePlanKey == Spec.ExpectedFinalPlanKey &&
                    Plan.CrownfallApproachSite ==
                        TestOwnedNoNeutralRallySite(Spec.LumeChoice) &&
                    Plan.AvailableFinalResolutions ==
                        Spec.ExpectedFinalResolutionMask &&
                    ObservedResolutionMask ==
                        Spec.ExpectedFinalResolutionMask &&
                    FEchoesBrokenSunMissionModel::IsResolutionAvailable(
                        Plan, Resolution),
                FString::Printf(
                    TEXT("Mission 15 binds %s to its exact plan and earned resolutions"),
                    Spec.Label)))
        {
            return false;
        }

        Vec2 ApproachSite;
        if (!Require(
                FindValidBuildSite(
                    Bridge,
                    Plan.CrownfallApproachSite,
                    3,
                    ApproachSite),
                TEXT("Mission 15 exposes a valid Approach Anchor site")) ||
            !Require(
                Bridge->IssueBuildCommand(
                    Objective.BrokenSunWorkerId,
                    EntityType::UtilityStructure,
                    Bridge->SimToWorld(ApproachSite),
                    Feedback),
                TEXT("Mission 15 accepts its Approach Anchor")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge]()
                    {
                        return Bridge->GetBrokenSunPhase() ==
                            EEchoesBrokenSunPhase::AssembleAccord;
                    },
                    4000),
                TEXT("Mission 15 secures the Crownfall approach")) ||
            !Require(
                Bridge->IssueResearchCommand(
                    ResearchLooms[0],
                    ResearchType::ChoirHeldAlternatives,
                    Feedback),
                TEXT("Mission 15 accepts Held Alternatives research")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge]()
                    {
                        const echoes::sim::PlayerState* Player =
                            Bridge->GetSimulation()->FindPlayer(
                                UEchoesSimulationSubsystem::LocalPlayerId);
                        return Player != nullptr &&
                            Player->HasCompletedResearch(
                                ResearchType::ChoirHeldAlternatives);
                    },
                    1200),
                TEXT("Mission 15 completes Held Alternatives")))
        {
            return false;
        }

        Objective = Bridge->GetLocalObjectiveSnapshot();
        const auto Move = [Bridge, &Feedback](EntityId Actor, const Vec2& Site)
        {
            return Bridge->IssueCommand(
                CommandType::Move,
                Actor,
                0,
                Bridge->SimToWorld(Site),
                FutureWellChoice::Dormant,
                Feedback);
        };
        if (!Require(
                Bridge->IssueChoirReconciliation(
                    Objective.BrokenSunAccordVoiceId,
                    ChoirIdentityState::Possible,
                    Feedback),
                TEXT("Mission 15 accepts Possible reconciliation")) ||
            !Require(
                Move(
                    Objective.BrokenSunAccordVoiceId,
                    Plan.MaraAccordSite),
                TEXT("Mission 15 voice accepts Mara's accord site")) ||
            !Require(
                Move(
                    Objective.BrokenSunAccordHeavyId,
                    Plan.OruunAccordSite),
                TEXT("Mission 15 Heavy accepts Oruun's accord site")) ||
            !Require(
                Move(Objective.BrokenSunNemeId, Plan.NemeAccordSite),
                TEXT("Mission 15 Neme accepts the Choir accord site")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge, Objective, Plan]()
                    {
                        const Entity* Voice = Bridge->FindEntity(
                            Objective.BrokenSunAccordVoiceId);
                        const Entity* Heavy = Bridge->FindEntity(
                            Objective.BrokenSunAccordHeavyId);
                        return Voice != nullptr && Heavy != nullptr &&
                            Voice->choirIdentityState ==
                                ChoirIdentityState::Possible &&
                            Heavy->choirIdentityState ==
                                ChoirIdentityState::Manifest &&
                            IsAtSite(
                                Bridge,
                                Objective.BrokenSunAccordVoiceId,
                                Plan.MaraAccordSite,
                                3) &&
                            IsAtSite(
                                Bridge,
                                Objective.BrokenSunAccordHeavyId,
                                Plan.OruunAccordSite,
                                3) &&
                            IsAtSite(
                                Bridge,
                                Objective.BrokenSunNemeId,
                                Plan.NemeAccordSite,
                                3);
                    },
                    3000),
                TEXT("Mission 15 assembles its three-part accord")) ||
            !Require(
                Bridge->IssueResearchCommand(
                    ResearchLooms[0],
                    ResearchType::ChoirSharedResolution,
                    Feedback),
                TEXT("Mission 15 accepts Shared Resolution research")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge]()
                    {
                        return Bridge->GetBrokenSunPhase() ==
                            EEchoesBrokenSunPhase::ChooseFinalResolution;
                    },
                    5000),
                TEXT("Mission 15 opens the earned ending choice")))
        {
            return false;
        }

        const auto ChooseResolution = [Controller, Resolution]()
        {
            switch (Resolution)
            {
                case EEchoesFinalResolution::Restoration:
                    Controller->ChooseFinalRestoration();
                    return true;
                case EEchoesFinalResolution::ControlledStabilization:
                    Controller->ChooseFinalStabilization();
                    return true;
                case EEchoesFinalResolution::Extinguishment:
                    Controller->ChooseFinalExtinguishment();
                    return true;
                case EEchoesFinalResolution::OpenEvolution:
                    Controller->ChooseFinalEvolution();
                    return true;
                case EEchoesFinalResolution::None:
                    return false;
            }
            return false;
        };
        if (!Require(
                ChooseResolution() &&
                    Bridge->GetLocalObjectiveSnapshot().
                            BrokenSunPendingFinalResolution ==
                        Resolution &&
                    Bridge->GetBrokenSunPhase() ==
                        EEchoesBrokenSunPhase::ChooseFinalResolution,
                TEXT("Mission 15 first choice press arms the ending")) ||
            !Require(
                ChooseResolution() &&
                    Bridge->GetBrokenSunPhase() ==
                        EEchoesBrokenSunPhase::RaiseResolutionConduit,
                TEXT("Mission 15 second choice press confirms the ending")))
        {
            return false;
        }

        Objective = Bridge->GetLocalObjectiveSnapshot();
        const Vec2 ResolutionCenter =
            FEchoesBrokenSunMissionModel::ResolutionConvergenceSite(
                Plan, Resolution);
        Vec2 ConduitSite;
        if (!Require(
                ResolutionCenter ==
                    TestOwnedFreshJourneyBrokenSunResolutionSite(Resolution),
                TEXT("Mission 15 binds the ending to its test-owned convergence site")) ||
            !Require(
                FindValidBuildSite(
                    Bridge, ResolutionCenter, 2, ConduitSite),
                TEXT("Mission 15 exposes the ending's conduit site")) ||
            !Require(
                Bridge->IssueBuildCommand(
                    Objective.BrokenSunWorkerId,
                    EntityType::UtilityStructure,
                    Bridge->SimToWorld(ConduitSite),
                    Feedback),
                TEXT("Mission 15 accepts the Resolution Conduit")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge]()
                    {
                        return Bridge->GetBrokenSunPhase() ==
                            EEchoesBrokenSunPhase::HoldFinalResolution;
                    },
                    4000),
                TEXT("Mission 15 starts the final hold")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge]()
                    {
                        return Bridge->GetBrokenSunPhase() ==
                            EEchoesBrokenSunPhase::Complete;
                    },
                    1200),
                TEXT("Mission 15 holds the complete ending contract")))
        {
            return false;
        }

        const FEchoesCampaignDecisionRecord* Record =
            Bridge->GetCampaignProgress().FindDecision(
                EEchoesCampaignMissionId::TheBrokenSun);
        // This record is written by the live simulation as the journey
        // ends, so it pins the CURRENT native snapshot schema. Terrain
        // and object memory are now serialized into snapshots, so that
        // schema moved 24 -> 25; the replay envelope shape is unchanged
        // and stays at 24.
        if (!Require(
                Record != nullptr &&
                    Record->VerifiedFacts == 0xFF &&
                    Record->FinalResolution == ExpectedRecordedResolution &&
                    Record->AvailableFinalResolutions ==
                        Spec.ExpectedFinalResolutionMask &&
                    Record->FinalPlanKey == Spec.ExpectedFinalPlanKey &&
                    Record->SimulationSnapshotVersion == 26 &&
                    Record->CompletionTick > 0 &&
                    Record->FinalStateChecksum != 0,
                TEXT("Mission 15 retains full native ending provenance")) ||
            !VerifyCompletion(15, ExpectedStatus))
        {
            return false;
        }

        if (ExpectedStatus == EEchoesCampaignCommitStatus::Added)
        {
            Controller->ConfirmPrimaryAction();
            return Require(
                Controller->IsTitleScreenVisible() &&
                    !Controller->IsMatchResultVisible() &&
                    Bridge->GetCampaignJourney().State ==
                        EEchoesCampaignJourneyState::Complete &&
                    Bridge->GetCampaignJourney().CompletedMissionCount == 15,
                TEXT("Mission 15 Enter returns to a complete title without Mission 16"));
        }
        return true;
    };

    const FFreshRouteSpec PreserveRoute{
        TEXT("preserve-restoration"),
        FutureWellChoice::Preserve,
        FutureWellChoice::Preserve,
        EEchoesFinalResolution::Restoration,
        16,
        0x03};
    if (!BeginFreshRoute(false) ||
        !RunMissionsOneThroughFive(PreserveRoute) ||
        !RunMissionsSixThroughTen(PreserveRoute) ||
        !RunMissionsElevenThroughFourteen(PreserveRoute) ||
        !RunMissionFifteen(
            PreserveRoute,
            PreserveRoute.FinalResolution,
            PreserveRoute.FinalResolution,
            EEchoesCampaignCommitStatus::Added))
    {
        Controller->Destroy();
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const TArray<FEchoesCampaignDecisionRecord> PreserveDecisions =
        Bridge->GetCampaignProgress().Decisions;
    const FEchoesCampaignDecisionRecord* PreserveEndingRecordPtr =
        Bridge->GetCampaignProgress().FindDecision(
            EEchoesCampaignMissionId::TheBrokenSun);
    if (!Require(
            PreserveEndingRecordPtr != nullptr &&
                PreserveEndingRecordPtr->FinalResolution ==
                    EEchoesFinalResolution::Restoration,
            TEXT("The first route retains its Restoration ending")))
    {
        Controller->Destroy();
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    const FEchoesCampaignDecisionRecord PreserveEndingRecord =
        *PreserveEndingRecordPtr;

    const auto LoadExactBytes = [&Require](
        const FString& Path,
        TArray<uint8>& OutBytes,
        const FString& Label)
    {
        OutBytes.Reset();
        return Require(
            FFileHelper::LoadFileToArray(OutBytes, *Path),
            Label);
    };
    TArray<uint8> PreservePrimaryBytes;
    if (!LoadExactBytes(
            CampaignPath,
            PreservePrimaryBytes,
            TEXT("The first route has exact persisted primary bytes")))
    {
        Controller->Destroy();
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    Controller->RequestNewCampaign();
    if (!Require(
            Controller->IsNewCampaignConfirmationArmed(),
            TEXT("Replacing the complete route requires confirmation")))
    {
        Controller->Destroy();
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    Controller->RequestNewCampaign();
    FEchoesCampaignProgress EmptyPrimary;
    TArray<uint8> EmptyPrimaryBytes;
    TArray<uint8> PreserveBackupBytes;
    if (!Require(
            Controller->IsTitleScreenVisible() &&
                Bridge->IsScenarioPaused() &&
                Bridge->GetOperationMode() ==
                    EEchoesOperationMode::CampaignPrologue &&
                Bridge->GetCampaignProgress().Decisions.IsEmpty() &&
                Bridge->GetCampaignJourney().State ==
                    EEchoesCampaignJourneyState::Ready &&
                Bridge->GetCampaignJourney().CompletedMissionCount == 0 &&
                Bridge->HasRestorableCampaignBackup() &&
                Bridge->GetCampaignBackupDecisionCount() == 15,
            TEXT("Confirmed New creates a reversible empty campaign")) ||
        !Require(
            FEchoesCampaignProgressStore::LoadGeneration(
                CampaignPath,
                EmptyPrimary,
                Feedback) &&
                EmptyPrimary.Decisions.IsEmpty(),
            TEXT("The new campaign primary decodes as empty")) ||
        !LoadExactBytes(
            CampaignPath,
            EmptyPrimaryBytes,
            TEXT("The empty campaign has exact primary bytes")) ||
        !LoadExactBytes(
            CampaignPath + TEXT(".bak"),
            PreserveBackupBytes,
            TEXT("The complete route is retained as exact backup bytes")) ||
        !Require(
            PreserveBackupBytes == PreservePrimaryBytes,
            TEXT("New preserves the complete route byte for byte")))
    {
        Controller->Destroy();
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    Controller->RequestCampaignRestore();
    if (!Require(
            Controller->IsCampaignRestoreConfirmationArmed(),
            TEXT("Restoring the complete route requires confirmation")))
    {
        Controller->Destroy();
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    Controller->RequestCampaignRestore();
    TArray<uint8> RestoredPrimaryBytes;
    TArray<uint8> ReversibleEmptyBackupBytes;
    if (!Require(
            Controller->IsTitleScreenVisible() &&
                Bridge->IsScenarioPaused() &&
                Bridge->GetOperationMode() == EEchoesOperationMode::Skirmish &&
                Bridge->GetCampaignProgress().Decisions == PreserveDecisions &&
                Bridge->GetCampaignJourney().State ==
                    EEchoesCampaignJourneyState::Complete &&
                Bridge->GetCampaignJourney().CompletedMissionCount == 15 &&
                Bridge->HasRestorableCampaignBackup() &&
                Bridge->GetCampaignBackupDecisionCount() == 0,
            TEXT("Restore reactivates the exact complete journey")) ||
        !LoadExactBytes(
            CampaignPath,
            RestoredPrimaryBytes,
            TEXT("The restored route has exact primary bytes")) ||
        !LoadExactBytes(
            CampaignPath + TEXT(".bak"),
            ReversibleEmptyBackupBytes,
            TEXT("Restore retains the replaced empty generation")) ||
        !Require(
            RestoredPrimaryBytes == PreservePrimaryBytes &&
                ReversibleEmptyBackupBytes == EmptyPrimaryBytes,
            TEXT("Restore swaps the two on-disk generations exactly")))
    {
        Controller->Destroy();
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    for (int32 MissionNumber = 1; MissionNumber <= 15; ++MissionNumber)
    {
        Controller->CycleOperation();
        if (!Require(
                Bridge->GetOperationMode() ==
                    OperationForMission(MissionNumber),
                FString::Printf(
                    TEXT("The restored title selects Mission %02d in order"),
                    MissionNumber)))
        {
            Controller->Destroy();
            Bridge->StopPrototypeScenario();
            WorldWrapper.ForwardErrorMessages(this);
            return false;
        }
    }
    if (!Require(
            Controller->IsTitleScreenVisible() &&
                Bridge->IsScenarioPaused() &&
                Bridge->GetOperationMode() ==
                    EEchoesOperationMode::CampaignTheBrokenSun,
            TEXT("The restored title reaches the Mission 15 replay")))
    {
        Controller->Destroy();
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    Controller->ConfirmPrimaryAction();
    if (!Require(
            Controller->IsMissionBriefingVisible() &&
                Bridge->IsScenarioPaused(),
            TEXT("Mission 15 replay opens its ordinary briefing")))
    {
        Controller->Destroy();
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    Controller->ConfirmPrimaryAction();
    const FEchoesBrokenSunPlan ReplayPlan = Bridge->GetBrokenSunPlan();
    if (!Require(
            !Controller->IsMissionBriefingVisible() &&
                !Bridge->IsScenarioPaused() &&
                ReplayPlan.StablePlanKey ==
                    PreserveEndingRecord.FinalPlanKey &&
                ReplayPlan.AvailableFinalResolutions ==
                    PreserveEndingRecord.AvailableFinalResolutions &&
                FEchoesBrokenSunMissionModel::IsResolutionAvailable(
                    ReplayPlan,
                    EEchoesFinalResolution::ControlledStabilization),
            TEXT("The replay exposes its earned alternate ending")) ||
        !RunMissionFifteen(
            PreserveRoute,
            EEchoesFinalResolution::ControlledStabilization,
            EEchoesFinalResolution::Restoration,
            EEchoesCampaignCommitStatus::ReplayConflict))
    {
        Controller->Destroy();
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const FEchoesCampaignDecisionRecord* ConflictRecord =
        Bridge->GetCampaignProgress().FindDecision(
            EEchoesCampaignMissionId::TheBrokenSun);
    TArray<uint8> ConflictPrimaryBytes;
    TArray<uint8> ConflictBackupBytes;
    if (!Require(
            Controller->GetCampaignFinalResolution() ==
                    EEchoesFinalResolution::ControlledStabilization &&
                Controller->GetRecordedCampaignFinalResolution() ==
                    EEchoesFinalResolution::Restoration &&
                !Controller->CanAdvanceCampaignResult() &&
                Bridge->GetCampaignProgress().Decisions == PreserveDecisions &&
                ConflictRecord != nullptr &&
                *ConflictRecord == PreserveEndingRecord,
            TEXT("The conflicting replay preserves the original ending")) ||
        !LoadExactBytes(
            CampaignPath,
            ConflictPrimaryBytes,
            TEXT("The replay conflict retains primary storage")) ||
        !LoadExactBytes(
            CampaignPath + TEXT(".bak"),
            ConflictBackupBytes,
            TEXT("The replay conflict retains backup storage")) ||
        !Require(
            ConflictPrimaryBytes == PreservePrimaryBytes &&
                ConflictBackupBytes == EmptyPrimaryBytes &&
                !IFileManager::Get().FileExists(
                    *(CampaignPath + TEXT(".tmp"))) &&
                !IFileManager::Get().FileExists(
                    *(CampaignPath + TEXT(".bak.tmp"))),
            TEXT("Replay conflict leaves both generations byte exact")))
    {
        Controller->Destroy();
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    Controller->TogglePauseMenu();
    TArray<uint8> EscapedPrimaryBytes;
    TArray<uint8> EscapedBackupBytes;
    if (!Require(
            Controller->IsTitleScreenVisible() &&
                !Controller->IsMatchResultVisible() &&
                Bridge->IsScenarioPaused() &&
                Bridge->GetCampaignJourney().State ==
                    EEchoesCampaignJourneyState::Complete &&
                Bridge->GetCampaignJourney().CompletedMissionCount == 15 &&
                Bridge->GetCampaignProgress().Decisions == PreserveDecisions,
            TEXT("Escape returns conflict to the complete journey")) ||
        !LoadExactBytes(
            CampaignPath,
            EscapedPrimaryBytes,
            TEXT("Conflict navigation retains primary storage")) ||
        !LoadExactBytes(
            CampaignPath + TEXT(".bak"),
            EscapedBackupBytes,
            TEXT("Conflict navigation retains backup storage")) ||
        !Require(
            EscapedPrimaryBytes == PreservePrimaryBytes &&
                EscapedBackupBytes == EmptyPrimaryBytes,
            TEXT("Conflict navigation performs no ledger rewrite")))
    {
        Controller->Destroy();
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const FFreshRouteSpec HarvestRoute{
        TEXT("harvest-extinguishment"),
        FutureWellChoice::Harvest,
        FutureWellChoice::Harvest,
        EEchoesFinalResolution::Extinguishment,
        6,
        0x06};
    if (!BeginFreshRoute(true))
    {
        Controller->Destroy();
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    TArray<uint8> SecondRouteBackupBytes;
    if (!LoadExactBytes(
            CampaignPath + TEXT(".bak"),
            SecondRouteBackupBytes,
            TEXT("The second fresh route initially retains route one")) ||
        !Require(
            SecondRouteBackupBytes == PreservePrimaryBytes,
            TEXT("The second route starts with route one as exact backup")) ||
        !RunMissionsOneThroughFive(HarvestRoute) ||
        !RunMissionsSixThroughTen(HarvestRoute) ||
        !RunMissionsElevenThroughFourteen(HarvestRoute) ||
        !RunMissionFifteen(
            HarvestRoute,
            HarvestRoute.FinalResolution,
            HarvestRoute.FinalResolution,
            EEchoesCampaignCommitStatus::Added))
    {
        Controller->Destroy();
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const FEchoesCampaignDecisionRecord* HarvestFoundingRecord =
        Bridge->GetCampaignProgress().FindDecision(
            EEchoesCampaignMissionId::WhatTheLedgerKeeps);
    const FEchoesCampaignDecisionRecord* HarvestLumeRecord =
        Bridge->GetCampaignProgress().FindDecision(
            EEchoesCampaignMissionId::ChoirAtLumeReach);
    const FEchoesCampaignDecisionRecord* HarvestEndingRecord =
        Bridge->GetCampaignProgress().FindDecision(
            EEchoesCampaignMissionId::TheBrokenSun);
    TArray<uint8> HarvestPrimaryBytes;
    if (!LoadExactBytes(
            CampaignPath,
            HarvestPrimaryBytes,
            TEXT("The second route has exact persisted primary bytes")) ||
        !Require(
            HarvestFoundingRecord != nullptr &&
                HarvestFoundingRecord->WellChoice ==
                    FutureWellChoice::Harvest &&
                HarvestLumeRecord != nullptr &&
                HarvestLumeRecord->WellChoice == FutureWellChoice::Harvest &&
                HarvestEndingRecord != nullptr &&
                HarvestEndingRecord->FinalResolution ==
                    EEchoesFinalResolution::Extinguishment &&
                HarvestPrimaryBytes != PreservePrimaryBytes &&
                Bridge->GetCampaignJourney().State ==
                    EEchoesCampaignJourneyState::Complete &&
                Bridge->GetCampaignJourney().CompletedMissionCount == 15 &&
                Controller->IsTitleScreenVisible() &&
                !Controller->IsMatchResultVisible(),
            TEXT("The distinct Harvest route earns Extinguishment without Mission 16")))
    {
        Controller->Destroy();
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const FFreshRouteSpec PreserveReshapeRoute{
        TEXT("preserve-reshape-open-evolution"),
        FutureWellChoice::Preserve,
        FutureWellChoice::Reshape,
        EEchoesFinalResolution::OpenEvolution,
        17,
        0x0A};
    if (!BeginFreshRoute(true) ||
        !RunMissionsOneThroughFive(PreserveReshapeRoute) ||
        !RunMissionsSixThroughTen(PreserveReshapeRoute) ||
        !RunMissionsElevenThroughFourteen(PreserveReshapeRoute) ||
        !RunMissionFifteen(
            PreserveReshapeRoute,
            PreserveReshapeRoute.FinalResolution,
            PreserveReshapeRoute.FinalResolution,
            EEchoesCampaignCommitStatus::Added))
    {
        Controller->Destroy();
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const FEchoesCampaignDecisionRecord* PreserveReshapeFoundingRecord =
        Bridge->GetCampaignProgress().FindDecision(
            EEchoesCampaignMissionId::WhatTheLedgerKeeps);
    const FEchoesCampaignDecisionRecord* PreserveReshapeLumeRecord =
        Bridge->GetCampaignProgress().FindDecision(
            EEchoesCampaignMissionId::ChoirAtLumeReach);
    const FEchoesCampaignDecisionRecord* PreserveReshapeEndingRecord =
        Bridge->GetCampaignProgress().FindDecision(
            EEchoesCampaignMissionId::TheBrokenSun);
    if (!Require(
            PreserveReshapeFoundingRecord != nullptr &&
                PreserveReshapeFoundingRecord->WellChoice ==
                    PreserveReshapeRoute.FoundingChoice &&
                PreserveReshapeLumeRecord != nullptr &&
                PreserveReshapeLumeRecord->WellChoice ==
                    PreserveReshapeRoute.LumeChoice &&
                PreserveReshapeEndingRecord != nullptr &&
                PreserveReshapeEndingRecord->FinalResolution ==
                    PreserveReshapeRoute.FinalResolution &&
                PreserveReshapeEndingRecord->FinalPlanKey ==
                    PreserveReshapeRoute.ExpectedFinalPlanKey &&
                PreserveReshapeEndingRecord->AvailableFinalResolutions ==
                    PreserveReshapeRoute.ExpectedFinalResolutionMask &&
                Bridge->GetCampaignProgress().Decisions.Num() == 15 &&
                Bridge->GetCampaignJourney().State ==
                    EEchoesCampaignJourneyState::Complete &&
                Bridge->GetCampaignJourney().CompletedMissionCount == 15 &&
                Controller->IsTitleScreenVisible() &&
                !Controller->IsMatchResultVisible(),
            TEXT("The Preserve-Reshape journey earns Open Evolution under exact plan 17 and mask 0x0A without Mission 16")))
    {
        Controller->Destroy();
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const FFreshRouteSpec ReshapePreserveRoute{
        TEXT("reshape-preserve-controlled-stabilization"),
        FutureWellChoice::Reshape,
        FutureWellChoice::Preserve,
        EEchoesFinalResolution::ControlledStabilization,
        25,
        0x0B};
    if (!BeginFreshRoute(true) ||
        !RunMissionsOneThroughFive(ReshapePreserveRoute) ||
        !RunMissionsSixThroughTen(ReshapePreserveRoute) ||
        !RunMissionsElevenThroughFourteen(ReshapePreserveRoute) ||
        !RunMissionFifteen(
            ReshapePreserveRoute,
            ReshapePreserveRoute.FinalResolution,
            ReshapePreserveRoute.FinalResolution,
            EEchoesCampaignCommitStatus::Added))
    {
        Controller->Destroy();
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const FEchoesCampaignDecisionRecord* ReshapePreserveFoundingRecord =
        Bridge->GetCampaignProgress().FindDecision(
            EEchoesCampaignMissionId::WhatTheLedgerKeeps);
    const FEchoesCampaignDecisionRecord* ReshapePreserveLumeRecord =
        Bridge->GetCampaignProgress().FindDecision(
            EEchoesCampaignMissionId::ChoirAtLumeReach);
    const FEchoesCampaignDecisionRecord* ReshapePreserveEndingRecord =
        Bridge->GetCampaignProgress().FindDecision(
            EEchoesCampaignMissionId::TheBrokenSun);
    if (!Require(
            ReshapePreserveFoundingRecord != nullptr &&
                ReshapePreserveFoundingRecord->WellChoice ==
                    ReshapePreserveRoute.FoundingChoice &&
                ReshapePreserveLumeRecord != nullptr &&
                ReshapePreserveLumeRecord->WellChoice ==
                    ReshapePreserveRoute.LumeChoice &&
                ReshapePreserveEndingRecord != nullptr &&
                ReshapePreserveEndingRecord->FinalResolution ==
                    ReshapePreserveRoute.FinalResolution &&
                ReshapePreserveEndingRecord->FinalPlanKey ==
                    ReshapePreserveRoute.ExpectedFinalPlanKey &&
                ReshapePreserveEndingRecord->AvailableFinalResolutions ==
                    ReshapePreserveRoute.ExpectedFinalResolutionMask &&
                Bridge->GetCampaignProgress().Decisions.Num() == 15 &&
                Bridge->GetCampaignJourney().State ==
                    EEchoesCampaignJourneyState::Complete &&
                Bridge->GetCampaignJourney().CompletedMissionCount == 15 &&
                Controller->IsTitleScreenVisible() &&
                !Controller->IsMatchResultVisible(),
            TEXT("The Reshape-Preserve journey earns Controlled Stabilization under exact plan 25 and mask 0x0B without Mission 16")))
    {
        Controller->Destroy();
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    Controller->Destroy();
    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors() && !WorldWrapper.HasFailed();
}

#endif
