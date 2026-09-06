#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "EchoesSimCore/Simulation.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesSkirmishSetup.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"
#include "Tests/AutomationCommon.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace EchoesAiSkirmishSmoke
{
constexpr echoes::sim::Tick SimulationTickBudget = 600;
constexpr double RunWallClockBudgetSeconds = 30.0;
constexpr std::size_t CommandCapacityHeadroom = 1024;

enum class ESmokeTermination : std::uint8_t
{
    AuthoritativeOutcome,
    BoundedWindowComplete,
    WallClockBudgetExhausted,
    TickProgressFailure,
    QueueCapacityGuard,
    SetupFailure,
    ViewFailure,
    ReplayFailure
};

struct FScenarioSpec final
{
    const TCHAR* Label = TEXT("");
    echoes::sim::Faction LocalFaction =
        echoes::sim::Faction::MeridianCompact;
    echoes::sim::Faction OpponentFaction =
        echoes::sim::Faction::KharuunAssemblies;
    EEchoesSkirmishMapPreset MapPreset =
        EEchoesSkirmishMapPreset::GlassScar;
    EEchoesSkirmishResourceLevel ResourceLevel =
        EEchoesSkirmishResourceLevel::Standard;
    // Defensive, not Balanced: the release boundary authorises exactly five
    // doctrines (Defensive, Raider, Economic, Expansionist, Adaptive) and
    // Balanced is not among them, so skirmish setup now refuses it. This test
    // compares runs against each other rather than against pinned checksums,
    // so the substitution changes which match is played, not what is proven.
    echoes::sim::AiPersonality LocalPersonality =
        echoes::sim::AiPersonality::Defensive;
    echoes::sim::AiPersonality OpponentPersonality =
        echoes::sim::AiPersonality::Adaptive;
};

struct FStateSample final
{
    echoes::sim::Tick Tick = 0;
    std::uint64_t Checksum = 0;
    echoes::sim::MatchOutcome Outcome =
        echoes::sim::MatchOutcome::Ongoing;
    std::uint64_t EntityCount = 0;
    std::uint64_t CommandLogCount = 0;
    std::array<echoes::sim::ResourcePool, 2> Resources{};
    std::array<std::int32_t, 2> PopulationUsed{};
    std::array<std::int32_t, 2> PopulationCapacity{};

    friend bool operator==(const FStateSample&, const FStateSample&) = default;
};

struct FRejectedCommand final
{
    echoes::sim::Command Command{};
    std::string Reason{};

    friend bool operator==(
        const FRejectedCommand&,
        const FRejectedCommand&) = default;
};

struct FRunResult final
{
    bool bCompleted = false;
    bool bAuthoritativeTerminal = false;
    std::uint64_t InitialSnapshotTraceDigest = 0;
    std::uint64_t InitialChecksum = 0;
    std::vector<echoes::sim::Command> GeneratedCommands{};
    std::vector<echoes::sim::Command> AdmittedCommands{};
    std::vector<FRejectedCommand> RejectedCommands{};
    std::map<std::string, std::uint64_t> RejectionHistogram{};
    std::vector<FStateSample> Samples{};
    std::uint64_t HiddenTargetReferences = 0;
    std::uint64_t GeneratorMismatches = 0;
    ESmokeTermination Termination = ESmokeTermination::SetupFailure;
    echoes::sim::MatchOutcome Outcome =
        echoes::sim::MatchOutcome::Ongoing;
    echoes::sim::Tick FinalTick = 0;
    std::uint64_t FinalChecksum = 0;
    echoes::sim::ReplayRecord ExportedReplay{};
    bool bReplaySucceeded = false;
    echoes::sim::Tick ReplayTick = 0;
    std::uint64_t ReplayChecksum = 0;
    echoes::sim::MatchOutcome ReplayOutcome =
        echoes::sim::MatchOutcome::Ongoing;
    double WallClockSeconds = 0.0;
    FString Failure{};
};

class FTraceHasher final
{
public:
    void AddByte(std::uint8_t Value)
    {
        Hash ^= Value;
        Hash *= 1099511628211ULL;
    }

    void AddU64(std::uint64_t Value)
    {
        for (std::uint32_t Shift = 0; Shift < 64; Shift += 8)
        {
            AddByte(static_cast<std::uint8_t>((Value >> Shift) & 0xffULL));
        }
    }

    void AddI32(std::int32_t Value)
    {
        AddU64(static_cast<std::uint32_t>(Value));
    }

    [[nodiscard]] std::uint64_t Value() const
    {
        return Hash;
    }

private:
    std::uint64_t Hash = 14695981039346656037ULL;
};

[[nodiscard]] const TCHAR* StableName(ESmokeTermination Termination)
{
    switch (Termination)
    {
        case ESmokeTermination::AuthoritativeOutcome:
            return TEXT("authoritative_outcome");
        case ESmokeTermination::BoundedWindowComplete:
            return TEXT("bounded_window_complete_ongoing");
        case ESmokeTermination::WallClockBudgetExhausted:
            return TEXT("wall_clock_budget_exhausted");
        case ESmokeTermination::TickProgressFailure:
            return TEXT("tick_progress_failure");
        case ESmokeTermination::QueueCapacityGuard:
            return TEXT("queue_capacity_guard");
        case ESmokeTermination::SetupFailure:
            return TEXT("setup_failure");
        case ESmokeTermination::ViewFailure:
            return TEXT("view_failure");
        case ESmokeTermination::ReplayFailure:
            return TEXT("replay_failure");
    }
    return TEXT("setup_failure");
}

[[nodiscard]] std::uint64_t SnapshotTraceDigest(
    const std::vector<std::uint8_t>& Bytes)
{
    FTraceHasher Hasher;
    Hasher.AddU64(Bytes.size());
    for (const std::uint8_t Byte : Bytes)
    {
        Hasher.AddByte(Byte);
    }
    return Hasher.Value();
}

[[nodiscard]] FString ContentDigestHex(
    const std::array<std::uint8_t, 32>& Digest)
{
    FString Hex;
    Hex.Reserve(64);
    for (const std::uint8_t Byte : Digest)
    {
        Hex += FString::Printf(
            TEXT("%02x"),
            static_cast<unsigned int>(Byte));
    }
    return Hex;
}

void AddCommand(FTraceHasher& Hasher, const echoes::sim::Command& Command)
{
    Hasher.AddU64(Command.executeTick);
    Hasher.AddByte(Command.player);
    Hasher.AddU64(Command.sequence);
    Hasher.AddByte(static_cast<std::uint8_t>(Command.type));
    Hasher.AddU64(Command.actor);
    Hasher.AddU64(Command.target);
    Hasher.AddI32(Command.position.x.Raw());
    Hasher.AddI32(Command.position.y.Raw());
    Hasher.AddByte(static_cast<std::uint8_t>(Command.buildType));
    Hasher.AddByte(static_cast<std::uint8_t>(Command.wellChoice));
    Hasher.AddByte(static_cast<std::uint8_t>(Command.warformAdaptation));
    Hasher.AddByte(static_cast<std::uint8_t>(Command.researchType));
}

[[nodiscard]] std::uint64_t CommandTraceDigest(
    const std::vector<echoes::sim::Command>& Commands)
{
    FTraceHasher Hasher;
    Hasher.AddU64(Commands.size());
    for (const echoes::sim::Command& Command : Commands)
    {
        AddCommand(Hasher, Command);
    }
    return Hasher.Value();
}

[[nodiscard]] bool CanonicalCommandLess(
    const echoes::sim::Command& Lhs,
    const echoes::sim::Command& Rhs)
{
    return std::tie(
               Lhs.executeTick,
               Lhs.player,
               Lhs.sequence,
               Lhs.type,
               Lhs.actor,
               Lhs.target,
               Lhs.position.x,
               Lhs.position.y,
               Lhs.buildType,
               Lhs.wellChoice,
               Lhs.warformAdaptation,
               Lhs.researchType) <
        std::tie(
               Rhs.executeTick,
               Rhs.player,
               Rhs.sequence,
               Rhs.type,
               Rhs.actor,
               Rhs.target,
               Rhs.position.x,
               Rhs.position.y,
               Rhs.buildType,
               Rhs.wellChoice,
               Rhs.warformAdaptation,
               Rhs.researchType);
}

[[nodiscard]] bool ReplayRecordsMatch(
    const echoes::sim::ReplayRecord& Lhs,
    const echoes::sim::ReplayRecord& Rhs)
{
    return Lhs.version == Rhs.version &&
        Lhs.initialSnapshot == Rhs.initialSnapshot &&
        Lhs.commands == Rhs.commands &&
        Lhs.finalTick == Rhs.finalTick &&
        Lhs.finalChecksum == Rhs.finalChecksum;
}

[[nodiscard]] std::uint64_t SampleTraceDigest(
    const std::vector<FStateSample>& Samples)
{
    FTraceHasher Hasher;
    Hasher.AddU64(Samples.size());
    for (const FStateSample& Sample : Samples)
    {
        Hasher.AddU64(Sample.Tick);
        Hasher.AddU64(Sample.Checksum);
        Hasher.AddByte(static_cast<std::uint8_t>(Sample.Outcome));
        Hasher.AddU64(Sample.EntityCount);
        Hasher.AddU64(Sample.CommandLogCount);
        for (std::size_t PlayerIndex = 0;
             PlayerIndex < Sample.Resources.size();
             ++PlayerIndex)
        {
            Hasher.AddI32(Sample.Resources[PlayerIndex].material);
            Hasher.AddI32(Sample.Resources[PlayerIndex].dawnshards);
            Hasher.AddI32(Sample.PopulationUsed[PlayerIndex]);
            Hasher.AddI32(Sample.PopulationCapacity[PlayerIndex]);
        }
    }
    return Hasher.Value();
}

[[nodiscard]] const echoes::sim::Entity* FindViewEntity(
    const echoes::sim::PlayerView& View,
    echoes::sim::EntityId EntityId)
{
    const auto Found = std::find_if(
        View.Entities().begin(),
        View.Entities().end(),
        [EntityId](const echoes::sim::Entity& Entity)
        {
            return Entity.id == EntityId;
        });
    return Found != View.Entities().end() ? &*Found : nullptr;
}

[[nodiscard]] bool IsCombatUnit(echoes::sim::EntityType Type)
{
    return Type == echoes::sim::EntityType::Soldier ||
        Type == echoes::sim::EntityType::HeavyUnit ||
        Type == echoes::sim::EntityType::ScoutUnit;
}

[[nodiscard]] bool IsPositionInsideMap(
    const echoes::sim::PlayerView& View,
    echoes::sim::Vec2 Position)
{
    const std::int64_t MaximumX =
        static_cast<std::int64_t>(View.Config().mapWidthTiles) *
        echoes::sim::kFixedScale;
    const std::int64_t MaximumY =
        static_cast<std::int64_t>(View.Config().mapHeightTiles) *
        echoes::sim::kFixedScale;
    return Position.x.Raw() >= 0 && Position.y.Raw() >= 0 &&
        static_cast<std::int64_t>(Position.x.Raw()) < MaximumX &&
        static_cast<std::int64_t>(Position.y.Raw()) < MaximumY;
}

[[nodiscard]] bool CommandRequiresTarget(echoes::sim::CommandType Type)
{
    switch (Type)
    {
        case echoes::sim::CommandType::Gather:
        case echoes::sim::CommandType::Deliver:
        case echoes::sim::CommandType::Attack:
        case echoes::sim::CommandType::FutureWell:
        case echoes::sim::CommandType::Guard:
        case echoes::sim::CommandType::AdaptWarform:
            return true;
        default:
            return false;
    }
}

[[nodiscard]] bool CommandUsesPosition(echoes::sim::CommandType Type)
{
    switch (Type)
    {
        case echoes::sim::CommandType::Move:
        case echoes::sim::CommandType::Build:
        case echoes::sim::CommandType::AttackMove:
        case echoes::sim::CommandType::Patrol:
        case echoes::sim::CommandType::ToggleDeploy:
        case echoes::sim::CommandType::RaiseMineralCover:
            return true;
        default:
            return false;
    }
}

[[nodiscard]] bool ValidateGeneratedCommand(
    const echoes::sim::Simulation& Simulation,
    const echoes::sim::PlayerView& View,
    const echoes::sim::Command& Command,
    std::set<echoes::sim::EntityId>& UsedActors,
    std::uint64_t DecisionIndex,
    std::uint64_t CommandIndex,
    std::uint64_t& HiddenTargetReferences,
    FString& OutFailure)
{
    const auto Reject =
        [&](const TCHAR* Reason)
        {
            OutFailure = FString::Printf(
                TEXT("decision=%llu command=%llu player=%u actor=%u type=%u reason=%s"),
                static_cast<unsigned long long>(DecisionIndex),
                static_cast<unsigned long long>(CommandIndex),
                static_cast<unsigned int>(Command.player),
                Command.actor,
                static_cast<unsigned int>(Command.type),
                Reason);
            return false;
        };

    if (Command.player != View.Player().id)
    {
        return Reject(TEXT("command player differs from scoped view"));
    }
    if (Command.executeTick != View.CurrentTick())
    {
        return Reject(TEXT("command tick differs from decision tick"));
    }
    if (Command.sequence == 0)
    {
        return Reject(TEXT("command sequence is zero"));
    }

    const echoes::sim::Entity* Actor =
        FindViewEntity(View, Command.actor);
    if (Actor == nullptr)
    {
        return Reject(TEXT("actor is absent from the scoped view"));
    }
    if (Actor->owner != View.Player().id || !Actor->completed ||
        Actor->hitPoints <= 0)
    {
        return Reject(TEXT("actor is not an owned live completed entity"));
    }
    if (!UsedActors.insert(Actor->id).second)
    {
        return Reject(TEXT("actor is used more than once in one decision"));
    }

    const bool bRequiresTarget = CommandRequiresTarget(Command.type);
    if (bRequiresTarget && Command.target == 0)
    {
        return Reject(TEXT("targeted command has no target"));
    }
    if (!bRequiresTarget && Command.target != 0)
    {
        return Reject(TEXT("untargeted command carries a target id"));
    }

    const echoes::sim::Entity* Target =
        Command.target != 0 ? FindViewEntity(View, Command.target) : nullptr;
    if (Command.target != 0 && Target == nullptr)
    {
        if (Simulation.FindEntity(Command.target) != nullptr)
        {
            ++HiddenTargetReferences;
            return Reject(TEXT("command references an authoritative hidden entity"));
        }
        return Reject(TEXT("command references an unknown entity"));
    }
    if (CommandUsesPosition(Command.type) &&
        !IsPositionInsideMap(View, Command.position))
    {
        return Reject(TEXT("command position is outside runtime map bounds"));
    }

    switch (Command.type)
    {
        case echoes::sim::CommandType::Move:
            return Actor->movementPerTickRaw > 0
                ? true
                : Reject(TEXT("move actor has no movement capability"));

        case echoes::sim::CommandType::Gather:
            return Actor->type == echoes::sim::EntityType::Worker &&
                    Target != nullptr &&
                    Target->type == echoes::sim::EntityType::ResourceNode &&
                    Target->resourceRemaining > 0
                ? true
                : Reject(TEXT("gather actor or visible resource target is invalid"));

        case echoes::sim::CommandType::Deliver:
        {
            const bool bOperationalKharuunDropoff =
                Target != nullptr &&
                (Target->faction !=
                     echoes::sim::Faction::KharuunAssemblies ||
                 Target->type != echoes::sim::EntityType::Dropoff ||
                 Target->waystoneMode == echoes::sim::WaystoneMode::Rooted);
            return Actor->type == echoes::sim::EntityType::Worker &&
                    Target != nullptr &&
                    Target->owner == View.Player().id && Target->completed &&
                    (Target->type == echoes::sim::EntityType::CommandCore ||
                     Target->type == echoes::sim::EntityType::Dropoff) &&
                    bOperationalKharuunDropoff
                ? true
                : Reject(TEXT("deliver actor or visible owned dropoff is invalid"));
        }

        case echoes::sim::CommandType::Build:
            return Actor->type == echoes::sim::EntityType::Worker &&
                    Actor->order.type != echoes::sim::OrderType::Build &&
                    (Command.buildType == echoes::sim::EntityType::Barracks ||
                     Command.buildType == echoes::sim::EntityType::Dropoff) &&
                    View.VisibilityAt(Command.position) ==
                        echoes::sim::Visibility::Visible
                ? true
                : Reject(TEXT("build actor, type, or visible placement is invalid"));

        case echoes::sim::CommandType::Attack:
        {
            const bool bProtectedCore =
                Target != nullptr &&
                Target->type == echoes::sim::EntityType::CommandCore &&
                Target->owner < echoes::sim::kMaximumPlayers &&
                (View.Config().protectedCommandCorePlayerMask &
                 static_cast<std::uint8_t>(1U << Target->owner)) != 0;
            return IsCombatUnit(Actor->type) && Actor->attackDamage > 0 &&
                    Target != nullptr &&
                    Target->owner != echoes::sim::kNeutralPlayer &&
                    Target->owner != View.Player().id && !bProtectedCore
                ? true
                : Reject(TEXT("attack actor or visible enemy target is invalid"));
        }

        case echoes::sim::CommandType::FutureWell:
            return Actor->type == echoes::sim::EntityType::Worker &&
                    Target != nullptr &&
                    Target->type == echoes::sim::EntityType::FutureWell &&
                    Target->wellChoice ==
                        echoes::sim::FutureWellChoice::Dormant &&
                    Command.wellChoice >
                        echoes::sim::FutureWellChoice::Dormant &&
                    Command.wellChoice <=
                        echoes::sim::FutureWellChoice::Reshape
                ? true
                : Reject(TEXT("Future Well actor, target, or choice is invalid"));

        case echoes::sim::CommandType::Produce:
            return (Actor->type == echoes::sim::EntityType::CommandCore &&
                    Command.buildType == echoes::sim::EntityType::Worker) ||
                    (Actor->type == echoes::sim::EntityType::Barracks &&
                     Command.buildType == echoes::sim::EntityType::Soldier)
                ? true
                : Reject(TEXT("production actor or unit type is invalid"));

        case echoes::sim::CommandType::Research:
            return Actor->type == echoes::sim::EntityType::Barracks &&
                    Command.researchType > echoes::sim::ResearchType::None &&
                    Command.researchType <=
                        echoes::sim::ResearchType::ChoirSharedResolution
                ? true
                : Reject(TEXT("research actor or technology is invalid"));

        case echoes::sim::CommandType::ReconcileToManifest:
        case echoes::sim::CommandType::ReconcileToPossible:
            return Actor->faction == echoes::sim::Faction::HollowChoir &&
                    IsCombatUnit(Actor->type)
                ? true
                : Reject(TEXT("Choir reconciliation actor is invalid"));

        case echoes::sim::CommandType::AttackMove:
            return IsCombatUnit(Actor->type) && Actor->attackDamage > 0 &&
                    Actor->movementPerTickRaw > 0
                ? true
                : Reject(TEXT("attack-move actor is invalid"));

        case echoes::sim::CommandType::Hold:
            return IsCombatUnit(Actor->type) && Actor->attackDamage > 0
                ? true
                : Reject(TEXT("hold actor is invalid"));

        case echoes::sim::CommandType::AdaptWarform:
            return Actor->faction ==
                        echoes::sim::Faction::KharuunAssemblies &&
                    IsCombatUnit(Actor->type) && Target != nullptr &&
                    Target->owner == View.Player().id && Target->completed &&
                    Target->faction ==
                        echoes::sim::Faction::KharuunAssemblies &&
                    Target->type == echoes::sim::EntityType::Barracks &&
                    Command.warformAdaptation >
                        echoes::sim::WarformAdaptation::None &&
                    Command.warformAdaptation <=
                        echoes::sim::WarformAdaptation::Striker
                ? true
                : Reject(TEXT("warform actor, visible site, or adaptation is invalid"));

        case echoes::sim::CommandType::RaiseMineralCover:
            return Actor->faction ==
                        echoes::sim::Faction::KharuunAssemblies &&
                    Actor->type == echoes::sim::EntityType::HeavyUnit &&
                    View.VisibilityAt(Command.position) ==
                        echoes::sim::Visibility::Visible
                ? true
                : Reject(TEXT("mineral-cover actor or visible position is invalid"));

        case echoes::sim::CommandType::Stop:
        case echoes::sim::CommandType::Guard:
        case echoes::sim::CommandType::Patrol:
        case echoes::sim::CommandType::ToggleDeploy:
        case echoes::sim::CommandType::ActivateRelaySupply:
        case echoes::sim::CommandType::ToggleWaystoneRoot:
            return Reject(TEXT("AI emitted an unsupported smoke command type"));
    }
    return Reject(TEXT("AI emitted an unknown command type"));
}

void AppendStateSample(
    const echoes::sim::Simulation& Simulation,
    std::vector<FStateSample>& Samples)
{
    FStateSample Sample;
    Sample.Tick = Simulation.CurrentTick();
    Sample.Checksum = Simulation.StateChecksum();
    Sample.Outcome = Simulation.Outcome();
    Sample.EntityCount = Simulation.Entities().size();
    Sample.CommandLogCount = Simulation.CommandLog().size();
    const std::array<echoes::sim::PlayerId, 2> Players{{
        UEchoesSimulationSubsystem::LocalPlayerId,
        UEchoesSimulationSubsystem::OpponentPlayerId}};
    for (std::size_t PlayerIndex = 0;
         PlayerIndex < Players.size();
         ++PlayerIndex)
    {
        const echoes::sim::PlayerState* Player =
            Simulation.FindPlayer(Players[PlayerIndex]);
        if (Player != nullptr)
        {
            Sample.Resources[PlayerIndex] = Player->resources;
        }
        Sample.PopulationUsed[PlayerIndex] =
            Simulation.PopulationUsed(Players[PlayerIndex]);
        Sample.PopulationCapacity[PlayerIndex] =
            Simulation.PopulationCapacity(Players[PlayerIndex]);
    }
    Samples.push_back(Sample);
}

[[nodiscard]] FRunResult ExecuteSmokeRun(
    const std::vector<std::uint8_t>& InitialSnapshot,
    const FScenarioSpec& Spec)
{
    const double StartedAt = FPlatformTime::Seconds();
    const double DeadlineAt = StartedAt + RunWallClockBudgetSeconds;
    FRunResult Result;
    Result.InitialSnapshotTraceDigest =
        SnapshotTraceDigest(InitialSnapshot);
    const auto HasWallClockBudget =
        [&](const TCHAR* Phase)
        {
            if (FPlatformTime::Seconds() <= DeadlineAt)
            {
                return true;
            }
            Result.bCompleted = false;
            Result.Termination =
                ESmokeTermination::WallClockBudgetExhausted;
            Result.Failure = FString::Printf(
                TEXT("run exceeded the %.1f-second monotonic wall-clock budget during %s"),
                RunWallClockBudgetSeconds,
                Phase);
            return false;
        };
    auto Finish = [&]() mutable -> FRunResult
    {
        Result.WallClockSeconds = FPlatformTime::Seconds() - StartedAt;
        if (Result.Failure.IsEmpty() &&
            Result.WallClockSeconds > RunWallClockBudgetSeconds)
        {
            Result.bCompleted = false;
            Result.Termination =
                ESmokeTermination::WallClockBudgetExhausted;
            Result.Failure = FString::Printf(
                TEXT("run exceeded the %.1f-second wall-clock acceptance budget"),
                RunWallClockBudgetSeconds);
        }
        return std::move(Result);
    };

    std::string LoadError;
    std::optional<echoes::sim::Simulation> Loaded =
        echoes::sim::Simulation::LoadSnapshot(
            InitialSnapshot,
            &LoadError);
    if (!Loaded.has_value())
    {
        Result.Termination = ESmokeTermination::SetupFailure;
        Result.Failure = FString::Printf(
            TEXT("snapshot load failed: %s"),
            UTF8_TO_TCHAR(LoadError.c_str()));
        return Finish();
    }

    echoes::sim::Simulation& Simulation = *Loaded;
    Result.InitialChecksum = Simulation.StateChecksum();
    if (Simulation.FindPlayer(
            UEchoesSimulationSubsystem::LocalPlayerId) == nullptr ||
        Simulation.FindPlayer(
            UEchoesSimulationSubsystem::OpponentPlayerId) == nullptr ||
        Simulation.Config().ticksPerSecond == 0)
    {
        Result.Termination = ESmokeTermination::SetupFailure;
        Result.Failure =
            TEXT("snapshot lacks both active seats or a valid tick rate");
        Result.FinalTick = Simulation.CurrentTick();
        Result.FinalChecksum = Simulation.StateChecksum();
        Result.Outcome = Simulation.Outcome();
        return Finish();
    }

    Simulation.CaptureReplayBaseline();
    AppendStateSample(Simulation, Result.Samples);
    const echoes::sim::Tick StartTick = Simulation.CurrentTick();
    echoes::sim::Tick StepAttempts = 0;
    std::uint64_t DecisionIndex = 0;
    const std::array<echoes::sim::PlayerId, 2> Players{{
        UEchoesSimulationSubsystem::LocalPlayerId,
        UEchoesSimulationSubsystem::OpponentPlayerId}};
    const std::array<echoes::sim::AiPersonality, 2> Personalities{{
        Spec.LocalPersonality,
        Spec.OpponentPersonality}};

    while (Simulation.Outcome() == echoes::sim::MatchOutcome::Ongoing &&
           StepAttempts < SimulationTickBudget)
    {
        if (!HasWallClockBudget(TEXT("bounded simulation loop")))
        {
            break;
        }
        if (Simulation.CurrentTick() < StartTick ||
            Simulation.CurrentTick() - StartTick != StepAttempts)
        {
            Result.Termination = ESmokeTermination::TickProgressFailure;
            Result.Failure = FString::Printf(
                TEXT("simulation tick diverged from bounded step count: start=%llu current=%llu attempts=%llu"),
                static_cast<unsigned long long>(StartTick),
                static_cast<unsigned long long>(Simulation.CurrentTick()),
                static_cast<unsigned long long>(StepAttempts));
            break;
        }
        if (Simulation.CurrentTick() %
                Simulation.Config().ticksPerSecond ==
            0)
        {
            if (Simulation.CommandLog().size() >=
                echoes::sim::kMaximumCommandLogEntries -
                    CommandCapacityHeadroom)
            {
                Result.Termination = ESmokeTermination::QueueCapacityGuard;
                Result.Failure = TEXT("command log reached the capacity guard");
                break;
            }

            for (std::size_t PlayerIndex = 0;
                 PlayerIndex < Players.size();
                 ++PlayerIndex)
            {
                const echoes::sim::PlayerId Player = Players[PlayerIndex];
                if (!HasWallClockBudget(TEXT("AI decision generation")))
                {
                    break;
                }
                const std::optional<echoes::sim::PlayerView> View =
                    Simulation.CreatePlayerView(Player);
                if (!View.has_value())
                {
                    Result.Termination = ESmokeTermination::ViewFailure;
                    Result.Failure = FString::Printf(
                        TEXT("decision=%llu player=%u scoped view unavailable"),
                        static_cast<unsigned long long>(DecisionIndex),
                        static_cast<unsigned int>(Player));
                    break;
                }

                const std::vector<echoes::sim::Command> PureCommands =
                    echoes::sim::Simulation::GenerateAiCommands(
                        *View,
                        Personalities[PlayerIndex]);
                if (!HasWallClockBudget(TEXT("pure AI generator")))
                {
                    break;
                }
                const std::vector<echoes::sim::Command>
                    CompatibilityCommands = Simulation.GenerateAiCommands(
                        Player,
                        Personalities[PlayerIndex]);
                if (!HasWallClockBudget(
                        TEXT("compatibility AI generator")))
                {
                    break;
                }
                if (PureCommands != CompatibilityCommands)
                {
                    ++Result.GeneratorMismatches;
                    Result.Termination = ESmokeTermination::ViewFailure;
                    Result.Failure = FString::Printf(
                        TEXT("decision=%llu player=%u pure and compatibility generators differ"),
                        static_cast<unsigned long long>(DecisionIndex),
                        static_cast<unsigned int>(Player));
                    break;
                }
                if (Simulation.CommandLog().size() + PureCommands.size() >=
                    echoes::sim::kMaximumCommandLogEntries -
                        CommandCapacityHeadroom)
                {
                    Result.Termination =
                        ESmokeTermination::QueueCapacityGuard;
                    Result.Failure = FString::Printf(
                        TEXT("decision=%llu player=%u generated commands cross the capacity guard"),
                        static_cast<unsigned long long>(DecisionIndex),
                        static_cast<unsigned int>(Player));
                    break;
                }

                Result.GeneratedCommands.insert(
                    Result.GeneratedCommands.end(),
                    PureCommands.begin(),
                    PureCommands.end());
                std::set<echoes::sim::EntityId> UsedActors;
                for (std::size_t CommandIndex = 0;
                     CommandIndex < PureCommands.size();
                     ++CommandIndex)
                {
                    if (!HasWallClockBudget(
                            TEXT("AI command validation and admission")))
                    {
                        break;
                    }
                    const echoes::sim::Command& Command =
                        PureCommands[CommandIndex];
                    FString ValidationFailure;
                    if (!ValidateGeneratedCommand(
                            Simulation,
                            *View,
                            Command,
                            UsedActors,
                            DecisionIndex,
                            CommandIndex,
                            Result.HiddenTargetReferences,
                            ValidationFailure))
                    {
                        Result.Termination = ESmokeTermination::ViewFailure;
                        Result.Failure = ValidationFailure;
                        break;
                    }

                    std::string Rejection;
                    if (Simulation.QueueCommand(Command, &Rejection))
                    {
                        Result.AdmittedCommands.push_back(Command);
                    }
                    else
                    {
                        const std::string StableRejection = Rejection.empty()
                            ? std::string("<empty>")
                            : Rejection;
                        Result.RejectedCommands.push_back(
                            {Command, StableRejection});
                        ++Result.RejectionHistogram[StableRejection];
                        Result.Termination = ESmokeTermination::ViewFailure;
                        Result.Failure = FString::Printf(
                            TEXT("decision=%llu player=%u structurally rejected: %s"),
                            static_cast<unsigned long long>(DecisionIndex),
                            static_cast<unsigned int>(Player),
                            UTF8_TO_TCHAR(StableRejection.c_str()));
                        break;
                    }
                }
                if (!Result.Failure.IsEmpty())
                {
                    break;
                }
            }
            ++DecisionIndex;
            if (!Result.Failure.IsEmpty())
            {
                break;
            }
        }

        if (!HasWallClockBudget(TEXT("simulation step")))
        {
            break;
        }
        const echoes::sim::Tick TickBeforeStep = Simulation.CurrentTick();
        ++StepAttempts;
        Simulation.Step();
        const echoes::sim::Tick TickAfterStep = Simulation.CurrentTick();
        if (TickAfterStep <= TickBeforeStep)
        {
            Result.Termination = ESmokeTermination::TickProgressFailure;
            Result.Failure = FString::Printf(
                TEXT("simulation tick stalled or regressed: before=%llu after=%llu attempt=%llu"),
                static_cast<unsigned long long>(TickBeforeStep),
                static_cast<unsigned long long>(TickAfterStep),
                static_cast<unsigned long long>(StepAttempts));
            break;
        }
        if (TickAfterStep - TickBeforeStep != 1)
        {
            Result.Termination = ESmokeTermination::TickProgressFailure;
            Result.Failure = FString::Printf(
                TEXT("single simulation step advanced an unexpected tick count: before=%llu after=%llu"),
                static_cast<unsigned long long>(TickBeforeStep),
                static_cast<unsigned long long>(TickAfterStep));
            break;
        }
        if (!HasWallClockBudget(TEXT("simulation step")))
        {
            break;
        }
        if (Simulation.CurrentTick() %
                Simulation.Config().ticksPerSecond ==
                0 ||
            Simulation.Outcome() != echoes::sim::MatchOutcome::Ongoing)
        {
            AppendStateSample(Simulation, Result.Samples);
        }
    }

    Result.FinalTick = Simulation.CurrentTick();
    Result.FinalChecksum = Simulation.StateChecksum();
    Result.Outcome = Simulation.Outcome();
    Result.bAuthoritativeTerminal =
        Result.Outcome != echoes::sim::MatchOutcome::Ongoing;
    if (Result.Samples.empty() ||
        Result.Samples.back().Tick != Result.FinalTick)
    {
        AppendStateSample(Simulation, Result.Samples);
    }
    if (!Result.Failure.IsEmpty())
    {
        return Finish();
    }

    if (Result.GeneratedCommands.empty())
    {
        Result.Termination = ESmokeTermination::ViewFailure;
        Result.Failure = TEXT("bounded run generated no AI commands");
        return Finish();
    }
    if (Result.GeneratedCommands.size() !=
        Result.AdmittedCommands.size() + Result.RejectedCommands.size())
    {
        Result.Termination = ESmokeTermination::ViewFailure;
        Result.Failure = TEXT("generated/admitted/rejected accounting differs");
        return Finish();
    }
    if (Simulation.CommandLog() != Result.AdmittedCommands)
    {
        Result.Termination = ESmokeTermination::ViewFailure;
        Result.Failure = TEXT("simulation command log differs from admitted records");
        return Finish();
    }
    if (!Result.RejectedCommands.empty() ||
        !Result.RejectionHistogram.empty())
    {
        Result.Termination = ESmokeTermination::ViewFailure;
        Result.Failure = TEXT("bounded run contains structural queue rejections");
        return Finish();
    }

    Result.Termination =
        Result.Outcome == echoes::sim::MatchOutcome::Ongoing
            ? ESmokeTermination::BoundedWindowComplete
            : ESmokeTermination::AuthoritativeOutcome;

    if (!HasWallClockBudget(TEXT("replay export")))
    {
        return Finish();
    }
    std::string ReplayError;
    const echoes::sim::ReplayRecord Replay =
        Simulation.ExportReplay(&ReplayError);
    Result.ExportedReplay = Replay;
    if (!HasWallClockBudget(TEXT("replay export")))
    {
        return Finish();
    }
    if (!ReplayError.empty())
    {
        Result.Termination = ESmokeTermination::ReplayFailure;
        Result.Failure = FString::Printf(
            TEXT("replay export failed: %s"),
            UTF8_TO_TCHAR(ReplayError.c_str()));
        return Finish();
    }
    if (Replay.version != echoes::sim::kReplayVersion)
    {
        Result.Termination = ESmokeTermination::ReplayFailure;
        Result.Failure = FString::Printf(
            TEXT("replay version differs: expected=%u actual=%u"),
            static_cast<unsigned int>(echoes::sim::kReplayVersion),
            static_cast<unsigned int>(Replay.version));
        return Finish();
    }
    if (Replay.initialSnapshot != InitialSnapshot)
    {
        Result.Termination = ESmokeTermination::ReplayFailure;
        Result.Failure = TEXT("replay baseline snapshot differs from the bounded-run input");
        return Finish();
    }

    std::vector<echoes::sim::Command> ExpectedReplayCommands =
        Result.AdmittedCommands;
    std::sort(
        ExpectedReplayCommands.begin(),
        ExpectedReplayCommands.end(),
        CanonicalCommandLess);
    if (Replay.commands != ExpectedReplayCommands)
    {
        Result.Termination = ESmokeTermination::ReplayFailure;
        Result.Failure = FString::Printf(
            TEXT("replay command records differ: expected=%llu actual=%llu"),
            static_cast<unsigned long long>(
                ExpectedReplayCommands.size()),
            static_cast<unsigned long long>(Replay.commands.size()));
        return Finish();
    }
    if (Replay.finalTick != Result.FinalTick ||
        Replay.finalChecksum != Result.FinalChecksum)
    {
        Result.Termination = ESmokeTermination::ReplayFailure;
        Result.Failure = FString::Printf(
            TEXT("replay terminal record differs: tick=%llu/%llu checksum=%llu/%llu"),
            static_cast<unsigned long long>(Replay.finalTick),
            static_cast<unsigned long long>(Result.FinalTick),
            static_cast<unsigned long long>(Replay.finalChecksum),
            static_cast<unsigned long long>(Result.FinalChecksum));
        return Finish();
    }

    if (!HasWallClockBudget(TEXT("replay execution")))
    {
        return Finish();
    }
    ReplayError.clear();
    const std::optional<echoes::sim::Simulation> Replayed =
        echoes::sim::Simulation::ReplayToEnd(Replay, &ReplayError);
    if (!HasWallClockBudget(TEXT("replay execution")))
    {
        return Finish();
    }
    if (!Replayed.has_value())
    {
        Result.Termination = ESmokeTermination::ReplayFailure;
        Result.Failure = FString::Printf(
            TEXT("replay failed: %s"),
            UTF8_TO_TCHAR(ReplayError.c_str()));
        return Finish();
    }
    Result.bReplaySucceeded = true;
    Result.ReplayTick = Replayed->CurrentTick();
    Result.ReplayChecksum = Replayed->StateChecksum();
    Result.ReplayOutcome = Replayed->Outcome();
    if (Result.ReplayTick != Result.FinalTick ||
        Result.ReplayChecksum != Result.FinalChecksum ||
        Result.ReplayOutcome != Result.Outcome)
    {
        Result.Termination = ESmokeTermination::ReplayFailure;
        Result.Failure = TEXT("replay terminal state differs from bounded run");
        return Finish();
    }

    Result.bCompleted = true;
    return Finish();
}

[[nodiscard]] bool DeterministicResultsMatch(
    const FRunResult& First,
    const FRunResult& Second,
    FString& OutDifference)
{
    const auto Differ =
        [&](bool bMatches, const TCHAR* Field)
        {
            if (!bMatches && OutDifference.IsEmpty())
            {
                OutDifference = Field;
            }
            return bMatches;
        };

    bool bMatches = true;
    bMatches &= Differ(
        First.InitialSnapshotTraceDigest ==
            Second.InitialSnapshotTraceDigest,
        TEXT("initial snapshot trace digest"));
    bMatches &= Differ(
        First.InitialChecksum == Second.InitialChecksum,
        TEXT("initial checksum"));
    bMatches &= Differ(
        First.GeneratedCommands == Second.GeneratedCommands,
        TEXT("full generated command records"));
    bMatches &= Differ(
        First.AdmittedCommands == Second.AdmittedCommands,
        TEXT("full admitted command records"));
    bMatches &= Differ(
        First.RejectedCommands == Second.RejectedCommands,
        TEXT("rejected command records"));
    bMatches &= Differ(
        First.RejectionHistogram == Second.RejectionHistogram,
        TEXT("rejection histogram"));
    bMatches &= Differ(
        First.Samples == Second.Samples,
        TEXT("fixed-cadence state samples"));
    bMatches &= Differ(
        First.HiddenTargetReferences == Second.HiddenTargetReferences,
        TEXT("hidden-target count"));
    bMatches &= Differ(
        First.GeneratorMismatches == Second.GeneratorMismatches,
        TEXT("generator parity count"));
    bMatches &= Differ(
        First.Termination == Second.Termination,
        TEXT("termination label"));
    bMatches &= Differ(
        First.bAuthoritativeTerminal == Second.bAuthoritativeTerminal,
        TEXT("authoritative terminal flag"));
    bMatches &= Differ(
        First.Outcome == Second.Outcome,
        TEXT("authoritative outcome"));
    bMatches &= Differ(
        First.FinalTick == Second.FinalTick,
        TEXT("final tick"));
    bMatches &= Differ(
        First.FinalChecksum == Second.FinalChecksum,
        TEXT("final checksum"));
    bMatches &= Differ(
        ReplayRecordsMatch(
            First.ExportedReplay,
            Second.ExportedReplay),
        TEXT("full exported replay record"));
    bMatches &= Differ(
        First.bReplaySucceeded == Second.bReplaySucceeded &&
            First.ReplayTick == Second.ReplayTick &&
            First.ReplayChecksum == Second.ReplayChecksum &&
            First.ReplayOutcome == Second.ReplayOutcome,
        TEXT("replay result"));
    return bMatches;
}
} // namespace EchoesAiSkirmishSmoke

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesAiSkirmishDeterminismSmokeTest,
    "Echoes.Runtime.AI.SkirmishDeterminismSmoke",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesAiSkirmishDeterminismSmokeTest::RunTest(
    const FString& Parameters)
{
    (void)Parameters;
    using namespace EchoesAiSkirmishSmoke;

    FEchoesScopedTestSaveEnvironment TestSaveEnvironment(*this);
    if (!TestSaveEnvironment.IsReady())
    {
        return false;
    }

    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the AI skirmish smoke world."));
        return false;
    }

    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge = World != nullptr
        ? World->GetSubsystem<UEchoesSimulationSubsystem>()
        : nullptr;
    if (!TestNotNull(TEXT("AI smoke world owns the simulation subsystem"),
                     Bridge) ||
        !TestTrue(TEXT("AI smoke bootstrap scenario starts"),
                  Bridge != nullptr && Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const std::array<FScenarioSpec, 6> Scenarios{{
        {TEXT("glass_meridian_kharuun"),
         echoes::sim::Faction::MeridianCompact,
         echoes::sim::Faction::KharuunAssemblies,
         EEchoesSkirmishMapPreset::GlassScar,
         EEchoesSkirmishResourceLevel::Scarce,
         echoes::sim::AiPersonality::Economic,
         echoes::sim::AiPersonality::Defensive},
        {TEXT("crownfall_kharuun_choir"),
         echoes::sim::Faction::KharuunAssemblies,
         echoes::sim::Faction::HollowChoir,
         EEchoesSkirmishMapPreset::CrownfallBasin,
         EEchoesSkirmishResourceLevel::Standard,
         echoes::sim::AiPersonality::Expansionist,
         echoes::sim::AiPersonality::Defensive},
        {TEXT("soryn_choir_meridian"),
         echoes::sim::Faction::HollowChoir,
         echoes::sim::Faction::MeridianCompact,
         EEchoesSkirmishMapPreset::SorynConfluence,
         EEchoesSkirmishResourceLevel::Abundant,
         echoes::sim::AiPersonality::Adaptive,
         echoes::sim::AiPersonality::Raider},
        {TEXT("crownfall_meridian_choir"),
         echoes::sim::Faction::MeridianCompact,
         echoes::sim::Faction::HollowChoir,
         EEchoesSkirmishMapPreset::CrownfallBasin,
         EEchoesSkirmishResourceLevel::Abundant,
         echoes::sim::AiPersonality::Expansionist,
         echoes::sim::AiPersonality::Economic},
        {TEXT("soryn_kharuun_meridian"),
         echoes::sim::Faction::KharuunAssemblies,
         echoes::sim::Faction::MeridianCompact,
         EEchoesSkirmishMapPreset::SorynConfluence,
         EEchoesSkirmishResourceLevel::Scarce,
         echoes::sim::AiPersonality::Defensive,
         echoes::sim::AiPersonality::Expansionist},
        {TEXT("glass_choir_kharuun"),
         echoes::sim::Faction::HollowChoir,
         echoes::sim::Faction::KharuunAssemblies,
         EEchoesSkirmishMapPreset::GlassScar,
         EEchoesSkirmishResourceLevel::Standard,
         echoes::sim::AiPersonality::Raider,
         echoes::sim::AiPersonality::Adaptive}}};

    std::array<std::int32_t, 6> LocalPersonalityCounts{};
    std::array<std::int32_t, 6> OpponentPersonalityCounts{};
    std::array<std::int32_t, 3> MapCounts{};
    std::array<std::int32_t, 3> ResourceCounts{};
    std::array<std::int32_t, 3> LocalFactionCounts{};
    std::array<std::int32_t, 3> OpponentFactionCounts{};
    std::set<std::uint16_t> DirectedMatchups;
    const auto MatchupKey =
        [](echoes::sim::Faction Local, echoes::sim::Faction Opponent)
        {
            return static_cast<std::uint16_t>(
                (static_cast<std::uint16_t>(Local) << 8U) |
                static_cast<std::uint16_t>(Opponent));
        };
    const std::set<std::uint16_t> ExpectedDirectedMatchups{
        MatchupKey(echoes::sim::Faction::MeridianCompact,
                   echoes::sim::Faction::KharuunAssemblies),
        MatchupKey(echoes::sim::Faction::MeridianCompact,
                   echoes::sim::Faction::HollowChoir),
        MatchupKey(echoes::sim::Faction::KharuunAssemblies,
                   echoes::sim::Faction::MeridianCompact),
        MatchupKey(echoes::sim::Faction::KharuunAssemblies,
                   echoes::sim::Faction::HollowChoir),
        MatchupKey(echoes::sim::Faction::HollowChoir,
                   echoes::sim::Faction::MeridianCompact),
        MatchupKey(echoes::sim::Faction::HollowChoir,
                   echoes::sim::Faction::KharuunAssemblies)};
    for (const FScenarioSpec& Spec : Scenarios)
    {
        ++LocalPersonalityCounts[static_cast<std::size_t>(
            Spec.LocalPersonality)];
        ++OpponentPersonalityCounts[static_cast<std::size_t>(
            Spec.OpponentPersonality)];
        ++MapCounts[static_cast<std::size_t>(Spec.MapPreset)];
        ++ResourceCounts[static_cast<std::size_t>(Spec.ResourceLevel)];
        ++LocalFactionCounts[static_cast<std::size_t>(Spec.LocalFaction)];
        ++OpponentFactionCounts[static_cast<std::size_t>(
            Spec.OpponentFaction)];
        DirectedMatchups.insert(MatchupKey(
            Spec.LocalFaction,
            Spec.OpponentFaction));
    }
    const auto AllCountsEqual =
        [](const auto& Counts, std::int32_t Expected)
        {
            return std::all_of(
                Counts.begin(),
                Counts.end(),
                [Expected](std::int32_t Count)
                {
                    return Count == Expected;
                });
        };
    // Six scenarios across FIVE authored doctrines, so "once each" is no longer
    // arithmetically possible: the release boundary retired
    // AiPersonality::Balanced, leaving Defensive, Raider, Economic,
    // Expansionist and Adaptive. The coverage intent is preserved by asserting
    // what actually matters -- every authored doctrine is exercised in both
    // seats, and the retired one is exercised in neither -- rather than by
    // weakening the check to fit. The Balanced assertion is new coverage: it
    // fails if a retired doctrine is ever reintroduced into this table.
    constexpr std::size_t kBalancedIndex =
        static_cast<std::size_t>(echoes::sim::AiPersonality::Balanced);
    const auto AuthoredDoctrinesCovered =
        [](const std::array<std::int32_t, 6>& Counts,
           std::int32_t ExpectedTotal)
        {
            std::int32_t Total = 0;
            for (std::size_t Index = 0; Index < Counts.size(); ++Index)
            {
                if (Index == kBalancedIndex)
                {
                    continue;
                }
                if (Counts[Index] < 1)
                {
                    return false;
                }
                Total += Counts[Index];
            }
            return Total == ExpectedTotal;
        };
    const std::int32_t ScenarioCount =
        static_cast<std::int32_t>(Scenarios.size());
    TestTrue(TEXT("Every authored AI doctrine is exercised in each seat"),
             AuthoredDoctrinesCovered(LocalPersonalityCounts, ScenarioCount) &&
                 AuthoredDoctrinesCovered(
                     OpponentPersonalityCounts, ScenarioCount));
    TestTrue(TEXT("The retired Balanced doctrine is exercised in neither seat"),
             LocalPersonalityCounts[kBalancedIndex] == 0 &&
                 OpponentPersonalityCounts[kBalancedIndex] == 0);
    TestTrue(TEXT("Each map and resource profile appears twice"),
             AllCountsEqual(MapCounts, 2) &&
                 AllCountsEqual(ResourceCounts, 2));
    TestTrue(TEXT("Each faction occupies each seat twice"),
             AllCountsEqual(LocalFactionCounts, 2) &&
                 AllCountsEqual(OpponentFactionCounts, 2));
    TestTrue(TEXT("All six directed non-mirror matchups are present"),
             DirectedMatchups == ExpectedDirectedMatchups);

    std::uint64_t CompletedRuns = 0;
    for (const FScenarioSpec& Spec : Scenarios)
    {
        FEchoesSkirmishSetup Setup;
        Setup.LocalFaction = Spec.LocalFaction;
        Setup.OpponentFaction = Spec.OpponentFaction;
        Setup.MapPreset = Spec.MapPreset;
        Setup.AiPersonality = Spec.OpponentPersonality;
        Setup.ResourceLevel = Spec.ResourceLevel;

        FString Feedback;
        if (!Bridge->ApplySkirmishSetup(Setup, Feedback))
        {
            AddError(FString::Printf(
                TEXT("[ECHOES_AI_SKIRMISH_SMOKE] scenario=%s termination=setup_failure detail=%s"),
                Spec.Label,
                *Feedback));
            continue;
        }

        const echoes::sim::Simulation* RuntimeSimulation =
            Bridge->GetSimulation();
        const echoes::sim::PlayerState* LocalPlayer =
            RuntimeSimulation != nullptr
                ? RuntimeSimulation->FindPlayer(
                      UEchoesSimulationSubsystem::LocalPlayerId)
                : nullptr;
        const echoes::sim::PlayerState* OpponentPlayer =
            RuntimeSimulation != nullptr
                ? RuntimeSimulation->FindPlayer(
                      UEchoesSimulationSubsystem::OpponentPlayerId)
                : nullptr;
        const bool bContentDigestPresent =
            RuntimeSimulation != nullptr &&
            std::any_of(
                RuntimeSimulation->Config().rules.contentSha256.begin(),
                RuntimeSimulation->Config().rules.contentSha256.end(),
                [](std::uint8_t Byte)
                {
                    return Byte != 0;
                });
        if (RuntimeSimulation == nullptr ||
            Bridge->GetActiveSkirmishSetup() != Setup ||
            LocalPlayer == nullptr || OpponentPlayer == nullptr ||
            LocalPlayer->faction != Spec.LocalFaction ||
            OpponentPlayer->faction != Spec.OpponentFaction ||
            !bContentDigestPresent)
        {
            AddError(FString::Printf(
                TEXT("[ECHOES_AI_SKIRMISH_SMOKE] scenario=%s ")
                TEXT("termination=setup_failure detail=runtime setup, ")
                TEXT("seats, or content provenance missing"),
                Spec.Label));
            continue;
        }

        const std::vector<std::uint8_t> InitialSnapshot =
            RuntimeSimulation->SaveSnapshot();
        if (InitialSnapshot.empty())
        {
            AddError(FString::Printf(
                TEXT("[ECHOES_AI_SKIRMISH_SMOKE] scenario=%s ")
                TEXT("termination=setup_failure detail=runtime snapshot empty"),
                Spec.Label));
            continue;
        }
        const FString RuntimeContentSha256 = ContentDigestHex(
            RuntimeSimulation->Config().rules.contentSha256);

        FRunResult First = ExecuteSmokeRun(InitialSnapshot, Spec);
        FRunResult Second = ExecuteSmokeRun(InitialSnapshot, Spec);
        CompletedRuns += First.bCompleted ? 1 : 0;
        CompletedRuns += Second.bCompleted ? 1 : 0;

        const FRunResult* Runs[] = {&First, &Second};
        for (std::size_t DuplicateIndex = 0;
             DuplicateIndex < UE_ARRAY_COUNT(Runs);
             ++DuplicateIndex)
        {
            const FRunResult& Run = *Runs[DuplicateIndex];
            AddInfo(FString::Printf(
                TEXT("[ECHOES_AI_SKIRMISH_SMOKE] scenario=%s duplicate=%llu ")
                TEXT("map=%s localFaction=%s opponentFaction=%s ")
                TEXT("localPersonality=%s opponentPersonality=%s ")
                TEXT("fixedSeed=%llu contentSha256=%s tickBudget=%llu ")
                TEXT("snapshotTraceDigest=%llu initialChecksum=%llu ")
                TEXT("generated=%llu admitted=%llu rejected=%llu ")
                TEXT("hiddenTargetReferences=%llu generatorMismatches=%llu ")
                TEXT("samples=%llu sampleTraceDigest=%llu ")
                TEXT("commandTraceDigest=%llu termination=%s outcome=%u ")
                TEXT("authoritativeTerminal=%s ")
                TEXT("finalTick=%llu finalChecksum=%llu replay=%s ")
                TEXT("replayVersion=%u replayCommands=%llu ")
                TEXT("replayBaselineTraceDigest=%llu ")
                TEXT("replayCommandTraceDigest=%llu replayChecksum=%llu ")
                TEXT("elapsedSeconds=%.6f ")
                TEXT("deterministicSmokeOnly=true seedMatrixQualified=false ")
                TEXT("seatSymmetryQualified=false balanceQualified=false ")
                TEXT("fairnessQualified=false gameplayQualityQualified=false"),
                Spec.Label,
                static_cast<unsigned long long>(DuplicateIndex + 1),
                FEchoesSkirmishSetupModel::MapDisplayName(Spec.MapPreset),
                FEchoesSkirmishSetupModel::FactionDisplayName(
                    Spec.LocalFaction),
                FEchoesSkirmishSetupModel::FactionDisplayName(
                    Spec.OpponentFaction),
                FEchoesSkirmishSetupModel::AiDisplayName(
                    Spec.LocalPersonality),
                FEchoesSkirmishSetupModel::AiDisplayName(
                    Spec.OpponentPersonality),
                static_cast<unsigned long long>(
                    RuntimeSimulation->Config().randomSeed),
                *RuntimeContentSha256,
                static_cast<unsigned long long>(SimulationTickBudget),
                static_cast<unsigned long long>(
                    Run.InitialSnapshotTraceDigest),
                static_cast<unsigned long long>(Run.InitialChecksum),
                static_cast<unsigned long long>(Run.GeneratedCommands.size()),
                static_cast<unsigned long long>(Run.AdmittedCommands.size()),
                static_cast<unsigned long long>(Run.RejectedCommands.size()),
                static_cast<unsigned long long>(Run.HiddenTargetReferences),
                static_cast<unsigned long long>(Run.GeneratorMismatches),
                static_cast<unsigned long long>(Run.Samples.size()),
                static_cast<unsigned long long>(
                    SampleTraceDigest(Run.Samples)),
                static_cast<unsigned long long>(
                    CommandTraceDigest(Run.GeneratedCommands)),
                StableName(Run.Termination),
                static_cast<unsigned int>(Run.Outcome),
                Run.bAuthoritativeTerminal ? TEXT("true") : TEXT("false"),
                static_cast<unsigned long long>(Run.FinalTick),
                static_cast<unsigned long long>(Run.FinalChecksum),
                Run.bReplaySucceeded ? TEXT("matched") : TEXT("failed"),
                static_cast<unsigned int>(Run.ExportedReplay.version),
                static_cast<unsigned long long>(
                    Run.ExportedReplay.commands.size()),
                static_cast<unsigned long long>(SnapshotTraceDigest(
                    Run.ExportedReplay.initialSnapshot)),
                static_cast<unsigned long long>(CommandTraceDigest(
                    Run.ExportedReplay.commands)),
                static_cast<unsigned long long>(Run.ReplayChecksum),
                Run.WallClockSeconds));
            if (!Run.bCompleted)
            {
                AddError(FString::Printf(
                    TEXT("[ECHOES_AI_SKIRMISH_SMOKE_FAILED] scenario=%s ")
                    TEXT("duplicate=%llu termination=%s detail=%s"),
                    Spec.Label,
                    static_cast<unsigned long long>(DuplicateIndex + 1),
                    StableName(Run.Termination),
                    Run.Failure.IsEmpty()
                        ? TEXT("unspecified bounded-run failure")
                        : *Run.Failure));
            }
        }

        FString Difference;
        if (!DeterministicResultsMatch(First, Second, Difference))
        {
            AddError(FString::Printf(
                TEXT("[ECHOES_AI_SKIRMISH_DUPLICATE_MISMATCH] scenario=%s field=%s"),
                Spec.Label,
                *Difference));
        }
    }

    TestEqual(
        TEXT("Six fixed scenarios each complete two bounded AI runs"),
        CompletedRuns,
        static_cast<std::uint64_t>(12));

    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors() && !WorldWrapper.HasFailed();
}

#endif
