#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesAiDifficultyController.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesSkirmishSetup.h"
#include "EchoesTestSaveEnvironment.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"
#include "Tests/AutomationCommon.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace EchoesAiLongRun
{
constexpr echoes::sim::Tick ProlongedMatchTick = 45ULL * 60ULL * 20ULL;
constexpr echoes::sim::Tick PostAdvisoryDiagnosticTicks = 6000;
constexpr echoes::sim::Tick MaximumMatchTicks =
    ProlongedMatchTick + PostAdvisoryDiagnosticTicks;
constexpr echoes::sim::Tick NoProgressDiagnosticTicks = 6000;
constexpr double ScenarioWallClockBudgetSeconds = 120.0;

struct FScenario final
{
    const TCHAR* Label = TEXT("");
    echoes::sim::Faction LocalFaction =
        echoes::sim::Faction::MeridianCompact;
    echoes::sim::Faction OpponentFaction =
        echoes::sim::Faction::KharuunAssemblies;
    EEchoesSkirmishMapPreset Map = EEchoesSkirmishMapPreset::GlassScar;
    echoes::sim::AiPersonality LocalPersonality =
        echoes::sim::AiPersonality::Raider;
    echoes::sim::AiPersonality OpponentPersonality =
        echoes::sim::AiPersonality::Adaptive;
};

struct FProgressState final
{
    std::array<int64, 2> CoreHitPoints{};
    std::array<int64, 2> OwnedHitPoints{};
    std::array<int32, 2> Workers{};
    std::array<int32, 2> CombatUnits{};
    std::array<int32, 2> Producers{};
    std::array<echoes::sim::ResourcePool, 2> Resources{};
    int64 ResourceRemaining = 0;
    int64 WellLifecycle = 0;
    uint64 MinimumCombatDistanceSquared = MAX_uint64;

    friend bool operator==(const FProgressState&, const FProgressState&) = default;
};

struct FCheckpointCoverage final
{
    bool bEconomyAttempted = false;
    bool bCombatAttempted = false;
    bool bWellAttempted = false;
    bool bEconomy = false;
    bool bCombat = false;
    bool bWell = false;
};

[[nodiscard]] bool HasMaterialProgress(
    const FProgressState& Before,
    const FProgressState& After,
    uint64 BestCombatDistanceSquared)
{
    return Before.CoreHitPoints != After.CoreHitPoints ||
        Before.OwnedHitPoints != After.OwnedHitPoints ||
        Before.Workers != After.Workers ||
        Before.CombatUnits != After.CombatUnits ||
        Before.Producers != After.Producers ||
        Before.Resources != After.Resources ||
        Before.ResourceRemaining != After.ResourceRemaining ||
        Before.WellLifecycle != After.WellLifecycle ||
        After.MinimumCombatDistanceSquared < BestCombatDistanceSquared;
}

[[nodiscard]] bool IsCombatUnit(echoes::sim::EntityType Type)
{
    return Type == echoes::sim::EntityType::Soldier ||
        Type == echoes::sim::EntityType::HeavyUnit ||
        Type == echoes::sim::EntityType::ScoutUnit;
}

[[nodiscard]] FProgressState CaptureProgress(
    const echoes::sim::Simulation& Simulation)
{
    FProgressState Result;
    for (echoes::sim::PlayerId Player = 0; Player < 2; ++Player)
    {
        if (const echoes::sim::PlayerState* State = Simulation.FindPlayer(Player))
        {
            Result.Resources[Player] = State->resources;
        }
    }
    for (const echoes::sim::Entity& Entity : Simulation.Entities())
    {
        if (Entity.type == echoes::sim::EntityType::ResourceNode)
        {
            Result.ResourceRemaining += Entity.resourceRemaining;
            continue;
        }
        if (Entity.type == echoes::sim::EntityType::FutureWell)
        {
            Result.WellLifecycle +=
                static_cast<int64>(Entity.owner) * 1000000000LL +
                static_cast<int64>(Entity.wellChoice) * 1000000LL +
                static_cast<int64>(Entity.wellPendingChoice) * 10000LL +
                static_cast<int64>(Entity.wellCaptureProgress) * 10LL +
                static_cast<int64>(Entity.wellProtocolTicks > 0);
            continue;
        }
        if (Entity.owner >= 2 || Entity.hitPoints <= 0)
        {
            continue;
        }
        Result.OwnedHitPoints[Entity.owner] += Entity.hitPoints;
        Result.Workers[Entity.owner] +=
            Entity.type == echoes::sim::EntityType::Worker ? 1 : 0;
        Result.CombatUnits[Entity.owner] += IsCombatUnit(Entity.type) ? 1 : 0;
        Result.Producers[Entity.owner] +=
            Entity.type == echoes::sim::EntityType::CommandCore ||
                    Entity.type == echoes::sim::EntityType::Barracks
                ? 1
                : 0;
        if (Entity.type == echoes::sim::EntityType::CommandCore)
        {
            Result.CoreHitPoints[Entity.owner] += Entity.hitPoints;
        }
    }
    for (const echoes::sim::Entity& Actor : Simulation.Entities())
    {
        if (Actor.owner >= 2 || !IsCombatUnit(Actor.type) ||
            Actor.hitPoints <= 0)
        {
            continue;
        }
        for (const echoes::sim::Entity& Target : Simulation.Entities())
        {
            if (Target.hitPoints <= 0 ||
                !Simulation.Config().IsHostile(Actor.owner, Target.owner))
            {
                continue;
            }
            const int64 DeltaX = static_cast<int64>(Actor.position.x.Raw()) -
                Target.position.x.Raw();
            const int64 DeltaY = static_cast<int64>(Actor.position.y.Raw()) -
                Target.position.y.Raw();
            Result.MinimumCombatDistanceSquared = FMath::Min<uint64>(
                Result.MinimumCombatDistanceSquared,
                static_cast<uint64>(DeltaX * DeltaX + DeltaY * DeltaY));
        }
    }
    return Result;
}

[[nodiscard]] bool HasEconomyInProgress(
    const echoes::sim::Simulation& Simulation)
{
    bool bWorkerActive = false;
    bool bProductionActive = false;
    for (const echoes::sim::Entity& Entity : Simulation.Entities())
    {
        if (Entity.owner >= 2 || Entity.hitPoints <= 0)
        {
            continue;
        }
        bWorkerActive |= Entity.type == echoes::sim::EntityType::Worker &&
            (Entity.cargo > 0 || Entity.harvestTicks > 0 ||
             Entity.order.type == echoes::sim::OrderType::Gather ||
             Entity.order.type == echoes::sim::OrderType::Deliver);
        bProductionActive |= Entity.productionRequired > 0;
    }
    return bWorkerActive && bProductionActive;
}

[[nodiscard]] bool HasCombatInProgress(
    const echoes::sim::Simulation& Simulation)
{
    if (!Simulation.Projectiles().empty())
    {
        return true;
    }
    for (const echoes::sim::Entity& Entity : Simulation.Entities())
    {
        if (Entity.owner < 2 && Entity.hitPoints > 0 &&
            Entity.hitPoints < Entity.maxHitPoints &&
            (IsCombatUnit(Entity.type) ||
             Entity.type == echoes::sim::EntityType::CommandCore ||
             Entity.type == echoes::sim::EntityType::Barracks ||
             Entity.type == echoes::sim::EntityType::Dropoff))
        {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool HasWellInProgress(
    const echoes::sim::Simulation& Simulation)
{
    for (const echoes::sim::Entity& Entity : Simulation.Entities())
    {
        if (Entity.type == echoes::sim::EntityType::FutureWell &&
            (Entity.wellCaptureProgress > 0 || Entity.wellProtocolTicks > 0))
        {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool RoundTripCheckpoint(
    FAutomationTestBase& Test,
    const TCHAR* ScenarioLabel,
    const TCHAR* Phase,
    std::optional<echoes::sim::Simulation>& Simulation)
{
    std::string ReplayError;
    const echoes::sim::ReplayRecord Prefix =
        Simulation->ExportReplay(&ReplayError);
    if (!ReplayError.empty())
    {
        Test.AddError(FString::Printf(
            TEXT("[ECHOES_AI_CHECKPOINT_FAILED] scenario=%s phase=%s replay=%s"),
            ScenarioLabel,
            Phase,
            UTF8_TO_TCHAR(ReplayError.c_str())));
        return false;
    }
    const echoes::sim::Tick TickBefore = Simulation->CurrentTick();
    const uint64 ChecksumBefore = Simulation->StateChecksum();
    const std::vector<echoes::sim::Command> PendingBefore(
        Simulation->PendingCommands().begin(),
        Simulation->PendingCommands().end());
    const std::vector<uint8> Snapshot = Simulation->SaveSnapshot();
    std::string LoadError;
    std::optional<echoes::sim::Simulation> Restored =
        echoes::sim::Simulation::LoadSnapshot(Snapshot, &LoadError);
    if (!Restored.has_value() || !LoadError.empty() ||
        Restored->CurrentTick() != TickBefore ||
        Restored->StateChecksum() != ChecksumBefore ||
        !std::equal(
            PendingBefore.begin(), PendingBefore.end(),
            Restored->PendingCommands().begin(),
            Restored->PendingCommands().end()))
    {
        Test.AddError(FString::Printf(
            TEXT("[ECHOES_AI_CHECKPOINT_FAILED] scenario=%s phase=%s tick=%llu detail=%s"),
            ScenarioLabel,
            Phase,
            static_cast<unsigned long long>(TickBefore),
            LoadError.empty() ? TEXT("restored state differs")
                              : UTF8_TO_TCHAR(LoadError.c_str())));
        return false;
    }
    if (!Restored->ContinueReplayRecording(Prefix, &ReplayError))
    {
        Test.AddError(FString::Printf(
            TEXT("[ECHOES_AI_CHECKPOINT_FAILED] scenario=%s phase=%s replayContinuation=%s"),
            ScenarioLabel,
            Phase,
            UTF8_TO_TCHAR(ReplayError.c_str())));
        return false;
    }
    Simulation = MoveTemp(Restored);
    Test.AddInfo(FString::Printf(
        TEXT("[ECHOES_AI_CHECKPOINT_RESTORED] scenario=%s phase=%s tick=%llu checksum=%llu pending=%llu"),
        ScenarioLabel,
        Phase,
        static_cast<unsigned long long>(TickBefore),
        static_cast<unsigned long long>(ChecksumBefore),
        static_cast<unsigned long long>(PendingBefore.size())));
    return true;
}

[[nodiscard]] FString StallDetail(
    const echoes::sim::Simulation& Simulation,
    const FProgressState& State,
    echoes::sim::Tick LastProgressTick,
    const std::array<echoes::sim::AiPersonality, 2>& Personalities)
{
    std::array<uint64, 2> Generated{};
    std::array<uint64, 2> VisibleHostiles{};
    std::array<uint64, 2> RememberedHostiles{};
    for (echoes::sim::PlayerId Player = 0; Player < 2; ++Player)
    {
        const std::optional<echoes::sim::PlayerView> View =
            Simulation.CreatePlayerView(Player);
        if (!View.has_value())
        {
            continue;
        }
        Generated[Player] = echoes::sim::Simulation::GenerateAiCommands(
            *View, Personalities[Player]).size();
        for (const echoes::sim::Entity& Entity : View->Entities())
        {
            VisibleHostiles[Player] +=
                Simulation.Config().IsHostile(Player, Entity.owner) ? 1 : 0;
        }
        for (const echoes::sim::RememberedObject& Object : View->RememberedObjects())
        {
            RememberedHostiles[Player] +=
                Simulation.Config().IsHostile(Player, Object.owner) ? 1 : 0;
        }
    }
    const TCHAR* Action = Generated[0] == 0 || Generated[1] == 0
        ? TEXT("no_commands_for_live_seat")
        : VisibleHostiles[0] == 0 && VisibleHostiles[1] == 0 &&
                RememberedHostiles[0] == 0 && RememberedHostiles[1] == 0
            ? TEXT("scouting_never_established_target_memory")
            : TEXT("legal_commands_failed_to_convert_into_corefall");
    return FString::Printf(
        TEXT("tick=%llu lastProgress=%llu noProgressTicks=%llu coreHp=%lld/%lld workers=%d/%d combat=%d/%d producers=%d/%d generated=%llu/%llu visibleHostiles=%llu/%llu rememberedHostiles=%llu/%llu pending=%llu minCombatDistanceSq=%llu action=%s"),
        static_cast<unsigned long long>(Simulation.CurrentTick()),
        static_cast<unsigned long long>(LastProgressTick),
        static_cast<unsigned long long>(Simulation.CurrentTick() - LastProgressTick),
        static_cast<long long>(State.CoreHitPoints[0]),
        static_cast<long long>(State.CoreHitPoints[1]),
        State.Workers[0], State.Workers[1],
        State.CombatUnits[0], State.CombatUnits[1],
        State.Producers[0], State.Producers[1],
        static_cast<unsigned long long>(Generated[0]),
        static_cast<unsigned long long>(Generated[1]),
        static_cast<unsigned long long>(VisibleHostiles[0]),
        static_cast<unsigned long long>(VisibleHostiles[1]),
        static_cast<unsigned long long>(RememberedHostiles[0]),
        static_cast<unsigned long long>(RememberedHostiles[1]),
        static_cast<unsigned long long>(Simulation.PendingCommands().size()),
        static_cast<unsigned long long>(State.MinimumCombatDistanceSquared),
        Action);
}
} // namespace EchoesAiLongRun

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesAiLongRunRuntimeTest,
    "Echoes.Runtime.AI.StandardLongRunCorefall",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesAiLongRunRuntimeTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    using namespace EchoesAiLongRun;

    FEchoesScopedTestSaveEnvironment TestSaveEnvironment(*this);
    if (!TestSaveEnvironment.IsReady())
    {
        return false;
    }
    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge = World != nullptr
        ? World->GetSubsystem<UEchoesSimulationSubsystem>()
        : nullptr;
    if (!TestNotNull(TEXT("Long-run world owns the simulation subsystem"), Bridge) ||
        !TestTrue(TEXT("Long-run bootstrap scenario starts"),
                  Bridge != nullptr && Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const std::array<FScenario, 3> Scenarios{{
        {TEXT("glass_meridian_kharuun"),
         echoes::sim::Faction::MeridianCompact,
         echoes::sim::Faction::KharuunAssemblies,
         EEchoesSkirmishMapPreset::GlassScar,
         echoes::sim::AiPersonality::Raider,
         echoes::sim::AiPersonality::Adaptive},
        {TEXT("crownfall_kharuun_choir"),
         echoes::sim::Faction::KharuunAssemblies,
         echoes::sim::Faction::HollowChoir,
         EEchoesSkirmishMapPreset::CrownfallBasin,
         echoes::sim::AiPersonality::Expansionist,
         echoes::sim::AiPersonality::Defensive},
        {TEXT("soryn_choir_meridian"),
         echoes::sim::Faction::HollowChoir,
         echoes::sim::Faction::MeridianCompact,
         EEchoesSkirmishMapPreset::SorynConfluence,
         echoes::sim::AiPersonality::Adaptive,
         echoes::sim::AiPersonality::Economic}}};
    const FEchoesAiDifficultyPolicy StandardPolicy =
        FEchoesAiDifficultyPolicy::For(EEchoesSkirmishDifficulty::Standard);
    bool bRestoredEconomyCheckpoint = false;
    bool bRestoredWellCheckpoint = false;
    bool bRestoredCombatCheckpoint = false;

    for (const FScenario& Scenario : Scenarios)
    {
        FEchoesSkirmishSetup Setup = FEchoesSkirmishSetupModel::DefaultSetup();
        Setup.LocalFaction = Scenario.LocalFaction;
        Setup.OpponentFaction = Scenario.OpponentFaction;
        Setup.MapPreset = Scenario.Map;
        Setup.AiPersonality = Scenario.OpponentPersonality;
        Setup.Difficulty = EEchoesSkirmishDifficulty::Standard;
        Setup.ResourceLevel = EEchoesSkirmishResourceLevel::Standard;
        FString SetupFeedback;
        if (!Bridge->ApplySkirmishSetup(Setup, SetupFeedback))
        {
            AddError(FString::Printf(
                TEXT("[ECHOES_AI_LONG_RUN_SETUP_FAILED] scenario=%s detail=%s"),
                Scenario.Label,
                *SetupFeedback));
            continue;
        }
        const echoes::sim::Simulation* Runtime = Bridge->GetSimulation();
        std::optional<echoes::sim::Simulation> Simulation = Runtime != nullptr
            ? echoes::sim::Simulation::LoadSnapshot(Runtime->SaveSnapshot())
            : std::nullopt;
        if (!Simulation.has_value())
        {
            AddError(FString::Printf(
                TEXT("[ECHOES_AI_LONG_RUN_SETUP_FAILED] scenario=%s detail=runtime snapshot unavailable"),
                Scenario.Label));
            continue;
        }
        Simulation->CaptureReplayBaseline();
        const std::array<echoes::sim::AiPersonality, 2> Personalities{{
            Scenario.LocalPersonality,
            Scenario.OpponentPersonality}};
        FCheckpointCoverage Checkpoints;
        FProgressState Progress = CaptureProgress(*Simulation);
        uint64 BestCombatDistanceSquared = Progress.MinimumCombatDistanceSquared;
        echoes::sim::Tick LastProgressTick = Simulation->CurrentTick();
        uint64 AdmittedCommands = 0;
        FString Failure;
        const double StartedAt = FPlatformTime::Seconds();

        while (Simulation->Outcome() == echoes::sim::MatchOutcome::Ongoing &&
               Simulation->CurrentTick() < MaximumMatchTicks)
        {
            if (FPlatformTime::Seconds() - StartedAt >
                ScenarioWallClockBudgetSeconds)
            {
                Failure = TEXT("wall_clock_budget_exhausted");
                break;
            }
            const echoes::sim::Tick Tick = Simulation->CurrentTick();
            if (FEchoesAiDifficultyController::IsGroupCommandTick(
                    Tick,
                    Simulation->Config().ticksPerSecond,
                    StandardPolicy.GroupCommandsPerSecond))
            {
                for (echoes::sim::PlayerId Player = 0; Player < 2; ++Player)
                {
                    const std::optional<echoes::sim::PlayerView> View =
                        Simulation->CreatePlayerView(Player);
                    if (!View.has_value())
                    {
                        Failure = FString::Printf(
                            TEXT("scoped_view_unavailable_player_%u"),
                            static_cast<uint32>(Player));
                        break;
                    }
                    const FEchoesAiDifficultyPlan Plan =
                        FEchoesAiDifficultyController::BuildPlan(
                            *View,
                            Personalities[Player],
                            StandardPolicy,
                            Simulation->PendingCommands());
                    for (const echoes::sim::Command& Command : Plan.Commands)
                    {
                        std::string Rejection;
                        if (!Simulation->QueueCommand(Command, &Rejection))
                        {
                            Failure = FString::Printf(
                                TEXT("command_rejected_player_%u_%s"),
                                static_cast<uint32>(Player),
                                UTF8_TO_TCHAR(Rejection.c_str()));
                            break;
                        }
                        ++AdmittedCommands;
                    }
                    if (!Failure.IsEmpty())
                    {
                        break;
                    }
                }
            }
            if (!Failure.IsEmpty())
            {
                break;
            }
            Simulation->Step();
            const FProgressState Current = CaptureProgress(*Simulation);
            if (HasMaterialProgress(
                    Progress, Current, BestCombatDistanceSquared))
            {
                LastProgressTick = Simulation->CurrentTick();
            }
            BestCombatDistanceSquared = FMath::Min(
                BestCombatDistanceSquared,
                Current.MinimumCombatDistanceSquared);
            Progress = Current;
            if (!Checkpoints.bEconomyAttempted &&
                HasEconomyInProgress(*Simulation))
            {
                Checkpoints.bEconomyAttempted = true;
                Checkpoints.bEconomy = RoundTripCheckpoint(
                    *this, Scenario.Label, TEXT("economy"), Simulation);
            }
            if (!Checkpoints.bWellAttempted &&
                HasWellInProgress(*Simulation))
            {
                Checkpoints.bWellAttempted = true;
                Checkpoints.bWell = RoundTripCheckpoint(
                    *this, Scenario.Label, TEXT("well"), Simulation);
            }
            if (!Checkpoints.bCombatAttempted &&
                HasCombatInProgress(*Simulation))
            {
                Checkpoints.bCombatAttempted = true;
                Checkpoints.bCombat = RoundTripCheckpoint(
                    *this, Scenario.Label, TEXT("combat"), Simulation);
            }
            if (Simulation->CurrentTick() == ProlongedMatchTick)
            {
                AddInfo(FString::Printf(
                    TEXT("[ECHOES_AI_PROLONGED_BOUNDARY] scenario=%s tick=%llu outcome=ongoing diagnosticOnly=true forcedResult=false"),
                    Scenario.Label,
                    static_cast<unsigned long long>(ProlongedMatchTick)));
            }
        }

        Progress = CaptureProgress(*Simulation);
        const echoes::sim::MatchOutcome Outcome = Simulation->Outcome();
        if (Failure.IsEmpty() && Outcome == echoes::sim::MatchOutcome::Ongoing)
        {
            Failure = Simulation->CurrentTick() - LastProgressTick >=
                    NoProgressDiagnosticTicks
                ? TEXT("material_progress_stall")
                : TEXT("terminal_not_reached_within_bound");
        }
        const FString Detail = StallDetail(
            *Simulation, Progress, LastProgressTick, Personalities);
        AddInfo(FString::Printf(
            TEXT("[ECHOES_AI_LONG_RUN] scenario=%s map=%s localFaction=%s opponentFaction=%s difficulty=Standard seed=%llu tick=%llu outcome=%u admitted=%llu economyCheckpoint=%s wellCheckpoint=%s combatCheckpoint=%s termination=%s detail=\"%s\" actualRules=true controllerPlan=true grantedOutcome=false boostedDamage=false"),
            Scenario.Label,
            FEchoesSkirmishSetupModel::MapDisplayName(Scenario.Map),
            FEchoesSkirmishSetupModel::FactionDisplayName(Scenario.LocalFaction),
            FEchoesSkirmishSetupModel::FactionDisplayName(Scenario.OpponentFaction),
            static_cast<unsigned long long>(Simulation->Config().randomSeed),
            static_cast<unsigned long long>(Simulation->CurrentTick()),
            static_cast<uint32>(Outcome),
            static_cast<unsigned long long>(AdmittedCommands),
            Checkpoints.bEconomy ? TEXT("restored") : TEXT("missing"),
            Checkpoints.bWell ? TEXT("restored") : TEXT("missing"),
            Checkpoints.bCombat ? TEXT("restored") : TEXT("missing"),
            Failure.IsEmpty() ? TEXT("authoritative_corefall") : *Failure,
            *Detail));
        bRestoredEconomyCheckpoint |= Checkpoints.bEconomy;
        bRestoredWellCheckpoint |= Checkpoints.bWell;
        bRestoredCombatCheckpoint |= Checkpoints.bCombat;
        if (!Failure.IsEmpty())
        {
            AddError(FString::Printf(
                TEXT("[ECHOES_AI_LONG_RUN_FAILED] scenario=%s termination=%s %s"),
                Scenario.Label,
                *Failure,
                *Detail));
            continue;
        }
        TestTrue(
            FString::Printf(TEXT("%s reaches authoritative Corefall"), Scenario.Label),
            Outcome == echoes::sim::MatchOutcome::Player0Victory ||
                Outcome == echoes::sim::MatchOutcome::Player1Victory);

        std::string ReplayError;
        const echoes::sim::ReplayRecord Replay =
            Simulation->ExportReplay(&ReplayError);
        const std::optional<echoes::sim::Simulation> Replayed =
            ReplayError.empty()
                ? echoes::sim::Simulation::ReplayToEnd(Replay, &ReplayError)
                : std::nullopt;
        TestTrue(
            FString::Printf(TEXT("%s final replay reproduces the terminal match"),
                            Scenario.Label),
            Replayed.has_value() && ReplayError.empty() &&
                Replayed->CurrentTick() == Simulation->CurrentTick() &&
                Replayed->StateChecksum() == Simulation->StateChecksum() &&
                Replayed->Outcome() == Outcome);
    }

    // A fast terminal scenario may legitimately finish before one transient
    // phase occurs. The three-map lane collectively must still exercise every
    // required in-progress persistence seam while each run independently
    // proves an authoritative Corefall above.
    TestTrue(TEXT("Long-run scenarios restore an economy checkpoint"),
             bRestoredEconomyCheckpoint);
    TestTrue(TEXT("Long-run scenarios restore a Future Well checkpoint"),
             bRestoredWellCheckpoint);
    TestTrue(TEXT("Long-run scenarios restore a combat checkpoint"),
             bRestoredCombatCheckpoint);

    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors() && !WorldWrapper.HasFailed();
}

#endif
