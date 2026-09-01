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
        if (!Require(
                !M04Workers.IsEmpty(),
                TEXT("Mission 04 exposes a construction worker")) ||
            !Require(
                Bridge->IssueCommand(
                    CommandType::ToggleWaystoneRoot,
                    M04Waystone,
                    0,
                    FVector::ZeroVector,
                    FutureWellChoice::Dormant,
                    Feedback),
                TEXT("Mission 04 Waystone accepts uproot")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge, M04Waystone]()
                    {
                        const Entity* Current =
                            Bridge->FindEntity(M04Waystone);
                        return Current != nullptr &&
                            Current->waystoneMode ==
                                echoes::sim::WaystoneMode::Mobile;
                    },
                    300),
                TEXT("Mission 04 Waystone becomes mobile")) ||
            !Require(
                Move(M04Waystone, M04Route.Roadhead),
                TEXT("Mission 04 Waystone accepts the roadhead route")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge, M04Waystone, M04Route]()
                    {
                        return IsAtSite(
                            Bridge, M04Waystone, M04Route.Roadhead);
                    },
                    2600),
                TEXT("Mission 04 Waystone reaches the roadhead")) ||
            !Require(
                Bridge->IssueCommand(
                    CommandType::ToggleWaystoneRoot,
                    M04Waystone,
                    0,
                    FVector::ZeroVector,
                    FutureWellChoice::Dormant,
                    Feedback),
                TEXT("Mission 04 Waystone accepts root")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge]()
                    {
                        return Bridge->GetUnburiedRoadPhase() ==
                            EEchoesUnburiedRoadPhase::RaiseListeningSpine;
                    },
                    400),
                TEXT("Mission 04 opens Listening Spine construction")) ||
            !Require(
                Bridge->IssueBuildCommand(
                    M04Workers[0],
                    EntityType::UtilityStructure,
                    Bridge->SimToWorld(M04Route.ListeningSpineSite),
                    Feedback),
                TEXT("Mission 04 worker accepts the Listening Spine")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge]()
                    {
                        return Bridge->GetUnburiedRoadPhase() ==
                            EEchoesUnburiedRoadPhase::RecoverMemoryShard;
                    },
                    3200),
                TEXT("Mission 04 opens shard recovery")) ||
            !Require(
                Move(M04Bearer, M04Route.MemoryShardSite),
                TEXT("Mission 04 bearer accepts shard recovery")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge]()
                    {
                        return Bridge->GetUnburiedRoadPhase() ==
                            EEchoesUnburiedRoadPhase::Complete;
                    },
                    3200),
                TEXT("Mission 04 completes through ordinary play")) ||
            !VerifyCompletion(4, EEchoesCampaignCommitStatus::Added) ||
            !AdvanceToNextMission(4))
        {
            return false;
        }

        // Mission 05: Terms of Continuance.
        const FEchoesTermsOfContinuancePlan M05Plan =
            Bridge->GetTermsOfContinuancePlan();
        const TArray<EntityId> M05Workers =
            FindOwnedEntities(Bridge, EntityType::Worker);
        if (!Require(
                !M05Workers.IsEmpty(),
                TEXT("Mission 05 exposes treaty-grid workers")))
        {
            return false;
        }
        Bridge->Tick(0.05f);
        if (Spec.FoundingChoice == FutureWellChoice::Harvest)
        {
            const Vec2 HarvestLinks[] = {
                Vec2::FromTiles(19, 21),
                Vec2::FromTiles(17, 28),
                Vec2::FromTiles(15, 34)};
            if (!Require(
                    M05Workers.Num() >= 3,
                    TEXT("Harvest treaty route exposes three workers")))
            {
                return false;
            }
            for (int32 Index = 0; Index < 3; ++Index)
            {
                if (!Require(
                        Bridge->IssueBuildCommand(
                            M05Workers[Index],
                            EntityType::Dropoff,
                            Bridge->SimToWorld(HarvestLinks[Index]),
                            Feedback),
                        FString::Printf(
                            TEXT("Harvest treaty link %d is accepted"),
                            Index + 1)))
                {
                    return false;
                }
            }
        }
        else
        {
            if (!Require(
                    Bridge->IssueBuildCommand(
                        M05Workers[0],
                        EntityType::Dropoff,
                        Bridge->SimToWorld(Vec2::FromTiles(29, 28)),
                        Feedback),
                    TEXT("Central treaty link is accepted")))
            {
                return false;
            }
        }
        if (!Require(
                TickUntil(
                    Bridge,
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
                    700),
                TEXT("Mission 05 synchronizes both treaty networks")) ||
            !Require(
                Bridge->GetSimulation()->CurrentTick() <
                    M05Plan.ContinuanceWindowEndTick,
                TEXT("Mission 05 synchronizes before the treaty deadline")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge, M05Plan]()
                    {
                        return Bridge->GetTermsOfContinuancePhase() ==
                                   EEchoesTermsOfContinuancePhase::
                                       ExtractWitnesses &&
                            Bridge->GetSimulation()->CurrentTick() >=
                                M05Plan.ContinuanceWindowEndTick;
                    },
                    1000),
                TEXT("Mission 05 holds the complete continuance window")))
        {
            return false;
        }
        const FEchoesObjectiveSnapshot M05Snapshot =
            Bridge->GetLocalObjectiveSnapshot();
        if (!Require(
                Move(
                    M05Snapshot.MeridianContinuanceWitnessId,
                    M05Plan.WitnessExtractionSite),
                TEXT("Mission 05 Meridian witness accepts extraction")) ||
            !Require(
                Move(
                    M05Snapshot.KharuunContinuanceWitnessId,
                    M05Plan.WitnessExtractionSite),
                TEXT("Mission 05 Kharuun witness accepts extraction")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge]()
                    {
                        return Bridge->GetTermsOfContinuancePhase() ==
                            EEchoesTermsOfContinuancePhase::Complete;
                    },
                    1000),
                TEXT("Mission 05 completes through ordinary play")) ||
            !VerifyCompletion(5, EEchoesCampaignCommitStatus::Added) ||
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
        Vec2 M06PowerLinkSite;
        if (!Require(
                M06Start.TalarId != 0 &&
                    M06Start.FirstCivilianId != 0 &&
                    M06Start.SecondCivilianId != 0,
                TEXT("Mission 06 exposes Talar and both civilians")) ||
            !Require(
                Move(M06Start.TalarId, M06Plan.CensusSite),
                TEXT("Mission 06 Talar accepts the census route")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge]()
                    {
                        return Bridge->GetNamesWithoutBirthsPhase() ==
                            EEchoesNamesWithoutBirthsPhase::
                                StabilizeArchive;
                    },
                    500),
                TEXT("Mission 06 locates the inherited census")) ||
            !Require(
                FindValidBuildSiteForType(
                    Bridge,
                    EntityType::Dropoff,
                    M06Plan.PowerLinkSite,
                    2,
                    M06PowerLinkSite),
                TEXT("Mission 06 exposes a valid census Power Link site")) ||
            !Require(
                Bridge->IssueBuildCommand(
                    M06Start.FirstCivilianId,
                    EntityType::Dropoff,
                    Bridge->SimToWorld(M06PowerLinkSite),
                    Feedback),
                TEXT("Mission 06 accepts the census Power Link")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge]()
                    {
                        return Bridge->GetNamesWithoutBirthsPhase() ==
                            EEchoesNamesWithoutBirthsPhase::
                                ShelterCivilians;
                    },
                    900),
                TEXT("Mission 06 powers the census archive")) ||
            !Require(
                Move(
                    M06Start.FirstCivilianId,
                    M06Plan.CivilianShelterSite),
                TEXT("Mission 06 first civilian accepts shelter")) ||
            !Require(
                Move(
                    M06Start.SecondCivilianId,
                    M06Plan.CivilianShelterSite),
                TEXT("Mission 06 second civilian accepts shelter")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge]()
                    {
                        return Bridge->GetNamesWithoutBirthsPhase() ==
                            EEchoesNamesWithoutBirthsPhase::
                                ExtractEvidence;
                    },
                    1000),
                TEXT("Mission 06 shelters both civilians")) ||
            !Require(
                Move(M06Start.TalarId, M06Plan.EvidenceExtractionSite),
                TEXT("Mission 06 Talar accepts evidence extraction")) ||
            !Require(
                TickUntil(
                    Bridge,
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
        if (!Require(
                M07Start.MigrationWaystoneId != 0 &&
                    M07Start.OruunId != 0 &&
                    !M07Workers.IsEmpty(),
                TEXT("Mission 07 exposes its Waystone, Oruun, and worker")) ||
            !Require(
                Bridge->IssueCommand(
                    CommandType::ToggleWaystoneRoot,
                    M07Start.MigrationWaystoneId,
                    0,
                    FVector::ZeroVector,
                    FutureWellChoice::Dormant,
                    Feedback),
                TEXT("Mission 07 Waystone accepts uproot")) ||
            !Require(
                TickUntil(
                    Bridge,
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
            !Require(
                Move(
                    M07Start.MigrationWaystoneId,
                    M07Plan.WaystoneAnchor),
                TEXT("Mission 07 Waystone accepts its listening route")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge, M07Start, M07Plan]()
                    {
                        return IsAtSite(
                            Bridge,
                            M07Start.MigrationWaystoneId,
                            M07Plan.WaystoneAnchor);
                    },
                    2600),
                TEXT("Mission 07 Waystone reaches its anchor")) ||
            !Require(
                Bridge->IssueCommand(
                    CommandType::ToggleWaystoneRoot,
                    M07Start.MigrationWaystoneId,
                    0,
                    FVector::ZeroVector,
                    FutureWellChoice::Dormant,
                    Feedback),
                TEXT("Mission 07 Waystone accepts root")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge]()
                    {
                        return Bridge->GetShapeOfSilencePhase() ==
                            EEchoesShapeOfSilencePhase::
                                RaiseListeningSpine;
                    },
                    400),
                TEXT("Mission 07 opens Listening Spine construction")) ||
            !Require(
                Bridge->IssueBuildCommand(
                    M07Workers[0],
                    EntityType::UtilityStructure,
                    Bridge->SimToWorld(M07Plan.ListeningSpineSite),
                    Feedback),
                TEXT("Mission 07 accepts its Listening Spine")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge]()
                    {
                        return Bridge->GetShapeOfSilencePhase() ==
                            EEchoesShapeOfSilencePhase::
                                PositionMemoryWitnesses;
                    },
                    3200),
                TEXT("Mission 07 opens paired witnessing")) ||
            !Require(
                Move(
                    M07Start.FirstMemoryWitnessId,
                    M07Plan.FirstWitnessSite),
                TEXT("Mission 07 first witness accepts its site")) ||
            !Require(
                Move(
                    M07Start.SecondMemoryWitnessId,
                    M07Plan.SecondWitnessSite),
                TEXT("Mission 07 second witness accepts its site")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge]()
                    {
                        return Bridge->GetShapeOfSilencePhase() ==
                            EEchoesShapeOfSilencePhase::ReachConfluence;
                    },
                    3400),
                TEXT("Mission 07 positions both witnesses")) ||
            !Require(
                Move(M07Start.OruunId, M07Plan.ConfluenceSite),
                TEXT("Mission 07 Oruun accepts the confluence route")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge]()
                    {
                        return Bridge->GetShapeOfSilencePhase() ==
                            EEchoesShapeOfSilencePhase::Complete;
                    },
                    3400),
                TEXT("Mission 07 completes through ordinary play")) ||
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
        if (!Require(
                M08Worker != 0,
                TEXT("Mission 08 exposes a separate construction worker")) ||
            !Require(
                Move(
                    M08Start.ShapeBesideUsTalarId,
                    M08Plan.FirstEchoSite),
                TEXT("Mission 08 Talar accepts the first-echo route")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge]()
                    {
                        return Bridge->GetShapeBesideUsPhase() ==
                            EEchoesShapeBesideUsPhase::RaiseEchoRelay;
                    },
                    3000),
                TEXT("Mission 08 reaches relay construction")) ||
            !Require(
                Bridge->IssueBuildCommand(
                    M08Worker,
                    EntityType::UtilityStructure,
                    Bridge->SimToWorld(M08Plan.EchoRelaySite),
                    Feedback),
                TEXT("Mission 08 accepts the echo relay")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge]()
                    {
                        return Bridge->GetShapeBesideUsPhase() ==
                            EEchoesShapeBesideUsPhase::
                                TraversePairedStates;
                    },
                    3400),
                TEXT("Mission 08 opens paired-state traversal")) ||
            !Require(
                Move(
                    M08Start.FirstStateWitnessId,
                    M08Plan.FirstStateSite),
                TEXT("Mission 08 first witness accepts its state")) ||
            !Require(
                Move(
                    M08Start.SecondStateWitnessId,
                    M08Plan.SecondStateSite),
                TEXT("Mission 08 second witness accepts its state")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge]()
                    {
                        return Bridge->GetShapeBesideUsPhase() ==
                            EEchoesShapeBesideUsPhase::ReachConvergence;
                    },
                    3600),
                TEXT("Mission 08 traverses both states")) ||
            !Require(
                Move(
                    M08Start.ShapeBesideUsTalarId,
                    M08Plan.ConvergenceSite),
                TEXT("Mission 08 Talar accepts convergence")) ||
            !Require(
                TickUntil(
                    Bridge,
                    [Bridge]()
                    {
                        return Bridge->GetShapeBesideUsPhase() ==
                            EEchoesShapeBesideUsPhase::Complete;
                    },
                    3600),
                TEXT("Mission 08 completes through ordinary play")) ||
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
                        return IsAtSite(
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
                    M12Plan.FirstDistrictInputSite),
                TEXT("Mission 12 Oruun accepts the first readback")) ||
            !Require(
                Move(
                    M12Start.FutureWonVerifierId,
                    M12Plan.SecondDistrictInputSite),
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
        if (!Require(
                Record != nullptr &&
                    Record->VerifiedFacts == 0xFF &&
                    Record->FinalResolution == ExpectedRecordedResolution &&
                    Record->AvailableFinalResolutions ==
                        Spec.ExpectedFinalResolutionMask &&
                    Record->FinalPlanKey == Spec.ExpectedFinalPlanKey &&
                    Record->SimulationSnapshotVersion == 24 &&
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
