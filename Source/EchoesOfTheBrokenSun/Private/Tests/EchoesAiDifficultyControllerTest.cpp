#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesAiDifficultyController.h"
#include "EchoesSimCore/Simulation.h"
#include "EchoesSkirmishSetup.h"

#include <algorithm>
#include <array>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace EchoesAiDifficultyTest
{
using echoes::sim::AiPersonality;
using echoes::sim::Command;
using echoes::sim::Entity;
using echoes::sim::EntityId;
using echoes::sim::EntityType;
using echoes::sim::Faction;
using echoes::sim::PlayerView;
using echoes::sim::Simulation;
using echoes::sim::SimulationConfig;
using echoes::sim::Tick;
using echoes::sim::Vec2;

constexpr echoes::sim::PlayerId LocalPlayer = 0;
constexpr echoes::sim::PlayerId OpponentPlayer = 1;

struct FTier final
{
    const TCHAR* Name = TEXT("");
    EEchoesSkirmishDifficulty Difficulty =
        EEchoesSkirmishDifficulty::Standard;
    Tick ReactionTicks = 0;
    Tick StrategicReviewTicks = 0;
    int32 GroupCommandsPerSecond = 0;
};

constexpr std::array<FTier, 4> Tiers{{
    {TEXT("Story"), EEchoesSkirmishDifficulty::Story, 60, 200, 4},
    {TEXT("Standard"), EEchoesSkirmishDifficulty::Standard, 30, 100, 7},
    {TEXT("Veteran"), EEchoesSkirmishDifficulty::Veteran, 18, 60, 10},
    {TEXT("Sovereign"), EEchoesSkirmishDifficulty::Sovereign, 10, 40, 12},
}};

[[nodiscard]] Simulation MakeFixture()
{
    SimulationConfig Config;
    Config.mapWidthTiles = 64;
    Config.mapHeightTiles = 64;
    Config.ticksPerSecond = 20;
    Config.randomSeed = 0xD1FF'1C01ULL;
    Simulation Result(Config);
    Result.AddPlayer(LocalPlayer, Faction::MeridianCompact, {5'000, 500});
    Result.AddPlayer(OpponentPlayer, Faction::KharuunAssemblies, {5'000, 500});
    Result.SpawnEntity(
        LocalPlayer,
        Faction::MeridianCompact,
        EntityType::CommandCore,
        Vec2::FromTiles(52, 52));
    Result.SpawnEntity(
        OpponentPlayer,
        Faction::KharuunAssemblies,
        EntityType::CommandCore,
        Vec2::FromTiles(8, 8));
    for (int32 Index = 0; Index < 12; ++Index)
    {
        Result.SpawnEntity(
            OpponentPlayer,
            Faction::KharuunAssemblies,
            EntityType::Worker,
            Vec2::FromTiles(7 + Index % 4, 11 + Index / 4));
    }
    Result.SpawnResourceNode(Vec2::FromTiles(12, 12), 50'000);
    return Result;
}

[[nodiscard]] std::optional<PlayerView> OpponentView(
    const Simulation& Sim)
{
    return Sim.CreatePlayerView(OpponentPlayer);
}

[[nodiscard]] bool ActorBelongsToViewPlayer(
    const PlayerView& View,
    EntityId Actor)
{
    const auto Found = std::find_if(
        View.Entities().begin(),
        View.Entities().end(),
        [Actor](const Entity& Candidate)
        {
            return Candidate.id == Actor;
        });
    return Found != View.Entities().end() &&
        Found->owner == View.Player().id;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesAiDifficultyControllerTest,
    "Echoes.Runtime.AI.DifficultyController",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesAiDifficultyControllerTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    using namespace EchoesAiDifficultyTest;

    TestTrue(
        TEXT("Warform adaptation is reviewed as macro planning"),
        FEchoesAiDifficultyController::IsStrategicCommand(
            echoes::sim::CommandType::AdaptWarform));
    TestTrue(
        TEXT("Manifest reconciliation is reviewed as macro planning"),
        FEchoesAiDifficultyController::IsStrategicCommand(
            echoes::sim::CommandType::ReconcileToManifest));
    TestTrue(
        TEXT("Possible reconciliation is reviewed as macro planning"),
        FEchoesAiDifficultyController::IsStrategicCommand(
            echoes::sim::CommandType::ReconcileToPossible));

    for (const FTier& Tier : Tiers)
    {
        const FEchoesAiDifficultyPolicy Policy =
            FEchoesAiDifficultyPolicy::For(Tier.Difficulty);
        TestEqual(
            FString::Printf(TEXT("%s reaction ticks"), Tier.Name),
            Policy.ReactionTicks,
            Tier.ReactionTicks);
        TestEqual(
            FString::Printf(TEXT("%s strategic review ticks"), Tier.Name),
            Policy.StrategicReviewTicks,
            Tier.StrategicReviewTicks);
        TestEqual(
            FString::Printf(TEXT("%s rolling group-command ceiling"), Tier.Name),
            Policy.GroupCommandsPerSecond,
            Tier.GroupCommandsPerSecond);

        std::vector<bool> Admissions(120, false);
        for (Tick TickIndex = 0; TickIndex < Admissions.size(); ++TickIndex)
        {
            Admissions[static_cast<size_t>(TickIndex)] =
                FEchoesAiDifficultyController::IsGroupCommandTick(
                    TickIndex, 20, Policy.GroupCommandsPerSecond);
        }
        for (int32 WindowStart = 0; WindowStart <= 100; ++WindowStart)
        {
            int32 AdmissionsInWindow = 0;
            for (int32 Offset = 0; Offset < 20; ++Offset)
            {
                AdmissionsInWindow +=
                    Admissions[static_cast<size_t>(WindowStart + Offset)]
                        ? 1
                        : 0;
            }
            TestEqual(
                FString::Printf(
                    TEXT("%s rolling window at tick %d meets its exact ceiling"),
                    Tier.Name,
                    WindowStart),
                AdmissionsInWindow,
                Policy.GroupCommandsPerSecond);
        }
        TestFalse(
            FString::Printf(TEXT("%s does not review one tick early"), Tier.Name),
            FEchoesAiDifficultyController::IsStrategicReviewTick(
                Policy.StrategicReviewTicks - 1, Policy));
        TestTrue(
            FString::Printf(TEXT("%s reviews at its exact interval"), Tier.Name),
            FEchoesAiDifficultyController::IsStrategicReviewTick(
                Policy.StrategicReviewTicks, Policy));

        Simulation StrategicFixture = MakeFixture();
        const std::optional<PlayerView> InitialView =
            OpponentView(StrategicFixture);
        TestTrue(
            FString::Printf(TEXT("%s initial scoped view exists"), Tier.Name),
            InitialView.has_value());
        if (!InitialView.has_value())
        {
            continue;
        }
        const FEchoesAiDifficultyPlan InitialPlan =
            FEchoesAiDifficultyController::BuildPlan(
                *InitialView,
                AiPersonality::Adaptive,
                Policy,
                StrategicFixture.PendingCommands());
        TestTrue(
            FString::Printf(TEXT("%s initial tick is a strategic review"), Tier.Name),
            InitialPlan.bStrategicReview);
        TestEqual(
            FString::Printf(TEXT("%s admits one command group"), Tier.Name),
            static_cast<int32>(InitialPlan.Commands.size()),
            1);
        if (!InitialPlan.Commands.empty())
        {
            const Command& Planned = InitialPlan.Commands.front();
            TestTrue(
                FString::Printf(TEXT("%s prioritizes a macro decision on review"), Tier.Name),
                FEchoesAiDifficultyController::IsStrategicCommand(Planned.type));
            TestEqual(
                FString::Printf(TEXT("%s applies the exact observed reaction delay"), Tier.Name),
                Planned.executeTick - InitialPlan.ObservationTick,
                Policy.ReactionTicks);
            TestTrue(
                FString::Printf(TEXT("%s command actor is owned in the scoped view"), Tier.Name),
                ActorBelongsToViewPlayer(*InitialView, Planned.actor));
            TestEqual(
                FString::Printf(TEXT("%s command uses opponent authority"), Tier.Name),
                Planned.player,
                OpponentPlayer);
        }

        Tick TacticalTick = 1;
        while (!FEchoesAiDifficultyController::IsGroupCommandTick(
            TacticalTick, 20, Policy.GroupCommandsPerSecond))
        {
            ++TacticalTick;
        }
        StrategicFixture.Step(TacticalTick);
        const std::optional<PlayerView> TacticalView =
            OpponentView(StrategicFixture);
        if (TestTrue(
                FString::Printf(TEXT("%s tactical scoped view exists"), Tier.Name),
                TacticalView.has_value()))
        {
            const FEchoesAiDifficultyPlan TacticalPlan =
                FEchoesAiDifficultyController::BuildPlan(
                    *TacticalView,
                    AiPersonality::Adaptive,
                    Policy,
                    StrategicFixture.PendingCommands());
            TestFalse(
                FString::Printf(TEXT("%s tactical slot is not a macro review"), Tier.Name),
                TacticalPlan.bStrategicReview);
            TestEqual(
                FString::Printf(TEXT("%s tactical slot admits one command"), Tier.Name),
                static_cast<int32>(TacticalPlan.Commands.size()),
                1);
            if (!TacticalPlan.Commands.empty())
            {
                TestFalse(
                    FString::Printf(TEXT("%s suppresses macro work between reviews"), Tier.Name),
                    FEchoesAiDifficultyController::IsStrategicCommand(
                        TacticalPlan.Commands.front().type));
                TestEqual(
                    FString::Printf(TEXT("%s tactical reaction is delayed once"), Tier.Name),
                    TacticalPlan.Commands.front().executeTick - TacticalTick,
                    Policy.ReactionTicks);
            }
        }
    }

    const FEchoesAiDifficultyPolicy Sovereign =
        FEchoesAiDifficultyPolicy::For(
            EEchoesSkirmishDifficulty::Sovereign);
    Simulation RollingFixture = MakeFixture();
    std::vector<Tick> AdmittedDecisionTicks;
    std::set<EntityId> AdmittedActors;
    for (int32 StepIndex = 0; StepIndex < 100; ++StepIndex)
    {
        const std::optional<PlayerView> View = OpponentView(RollingFixture);
        if (!TestTrue(TEXT("Rolling fixture retains opponent scoped view"), View.has_value()))
        {
            break;
        }
        const FEchoesAiDifficultyPlan Plan =
            FEchoesAiDifficultyController::BuildPlan(
                *View,
                AiPersonality::Adaptive,
                Sovereign,
                RollingFixture.PendingCommands());
        for (const Command& Planned : Plan.Commands)
        {
            TestEqual(
                TEXT("Stable-policy command has one exact reaction delay"),
                Planned.executeTick - Plan.ObservationTick,
                Sovereign.ReactionTicks);
            std::string Rejection;
            const bool bQueued = RollingFixture.QueueCommand(Planned, &Rejection);
            TestTrue(
                FString::Printf(
                    TEXT("Difficulty-admitted command queues: %s"),
                    UTF8_TO_TCHAR(Rejection.c_str())),
                bQueued);
            if (bQueued)
            {
                AdmittedDecisionTicks.push_back(Plan.ObservationTick);
                AdmittedActors.insert(Planned.actor);
            }
        }
        std::set<EntityId> PendingActors;
        bool bPendingActorsUnique = true;
        for (const Command& Pending : RollingFixture.PendingCommands())
        {
            if (Pending.player == OpponentPlayer &&
                !PendingActors.insert(Pending.actor).second)
            {
                bPendingActorsUnique = false;
            }
        }
        TestTrue(
            TEXT("No opponent actor accumulates repeated deferred decisions"),
            bPendingActorsUnique);
        RollingFixture.Step();
    }
    for (Tick WindowStart = 0; WindowStart <= 80; ++WindowStart)
    {
        const int32 Count = static_cast<int32>(std::count_if(
            AdmittedDecisionTicks.begin(),
            AdmittedDecisionTicks.end(),
            [WindowStart](Tick DecisionTick)
            {
                return DecisionTick >= WindowStart &&
                    DecisionTick < WindowStart + 20;
            }));
        TestTrue(
            FString::Printf(
                TEXT("Runtime rolling command window at tick %llu respects Sovereign ceiling"),
                static_cast<unsigned long long>(WindowStart)),
            Count <= Sovereign.GroupCommandsPerSecond);
    }
    TestTrue(
        TEXT("Deterministic selection gives multiple ready actors admissions"),
        AdmittedActors.size() >= 3);

    Simulation TierChangeFixture = MakeFixture();
    const FEchoesAiDifficultyPolicy Story =
        FEchoesAiDifficultyPolicy::For(EEchoesSkirmishDifficulty::Story);
    const std::optional<PlayerView> StoryView =
        OpponentView(TierChangeFixture);
    if (TestTrue(TEXT("Tier-change fixture has a scoped view"), StoryView.has_value()))
    {
        const FEchoesAiDifficultyPlan StoryPlan =
            FEchoesAiDifficultyController::BuildPlan(
                *StoryView,
                AiPersonality::Adaptive,
                Story,
                TierChangeFixture.PendingCommands());
        std::string StoryRejection;
        const bool bStoryQueued = !StoryPlan.Commands.empty() &&
            TierChangeFixture.QueueCommand(
                StoryPlan.Commands.front(), &StoryRejection);
        TestTrue(
            TEXT("Slow-policy deferred command enters authority"),
            bStoryQueued);
        if (bStoryQueued)
        {
            const FEchoesAiDifficultyPlan FasterPlan =
                FEchoesAiDifficultyController::BuildPlan(
                    *StoryView,
                    AiPersonality::Adaptive,
                    Sovereign,
                    TierChangeFixture.PendingCommands());
            TestEqual(
                TEXT("Faster policy preserves the in-flight execution frontier"),
                FasterPlan.ScheduledExecutionTick,
                Story.ReactionTicks);
            if (!FasterPlan.Commands.empty())
            {
                TestTrue(
                    TEXT("Tier-change command uses another actor while the first is pending"),
                    FasterPlan.Commands.front().actor !=
                        StoryPlan.Commands.front().actor);
                std::string FasterRejection;
                const bool bFasterQueued = TierChangeFixture.QueueCommand(
                    FasterPlan.Commands.front(), &FasterRejection);
                TestTrue(
                    FString::Printf(
                        TEXT("Tier-change future sequence remains admissible: %s"),
                        UTF8_TO_TCHAR(FasterRejection.c_str())),
                    bFasterQueued);
            }
        }
    }

    Simulation BeforeSave = MakeFixture();
    for (int32 StepIndex = 0; StepIndex < 37; ++StepIndex)
    {
        const std::optional<PlayerView> View = OpponentView(BeforeSave);
        if (!View.has_value())
        {
            AddError(TEXT("Snapshot fixture lost its opponent view before capture."));
            return false;
        }
        const FEchoesAiDifficultyPlan Plan =
            FEchoesAiDifficultyController::BuildPlan(
                *View,
                AiPersonality::Adaptive,
                Sovereign,
                BeforeSave.PendingCommands());
        for (const Command& Planned : Plan.Commands)
        {
            std::string Rejection;
            if (!BeforeSave.QueueCommand(Planned, &Rejection))
            {
                AddError(FString::Printf(
                    TEXT("Snapshot setup command rejected: %s"),
                    UTF8_TO_TCHAR(Rejection.c_str())));
                return false;
            }
        }
        BeforeSave.Step();
    }
    const std::vector<uint8> Snapshot = BeforeSave.SaveSnapshot();
    std::string LoadError;
    std::optional<Simulation> AfterLoad =
        Simulation::LoadSnapshot(Snapshot, &LoadError);
    TestTrue(
        FString::Printf(
            TEXT("Difficulty continuation snapshot loads: %s"),
            UTF8_TO_TCHAR(LoadError.c_str())),
        AfterLoad.has_value());
    if (AfterLoad.has_value())
    {
        TestEqual(
            TEXT("Deferred commands survive the difficulty continuation snapshot"),
            static_cast<int32>(AfterLoad->PendingCommands().size()),
            static_cast<int32>(BeforeSave.PendingCommands().size()));
        for (int32 StepIndex = 0; StepIndex < 80; ++StepIndex)
        {
            const std::optional<PlayerView> OriginalView =
                OpponentView(BeforeSave);
            const std::optional<PlayerView> LoadedView =
                OpponentView(*AfterLoad);
            if (!OriginalView.has_value() || !LoadedView.has_value())
            {
                AddError(TEXT("A continuation fixture lost its scoped opponent view."));
                break;
            }
            const FEchoesAiDifficultyPlan OriginalPlan =
                FEchoesAiDifficultyController::BuildPlan(
                    *OriginalView,
                    AiPersonality::Adaptive,
                    Sovereign,
                    BeforeSave.PendingCommands());
            const FEchoesAiDifficultyPlan LoadedPlan =
                FEchoesAiDifficultyController::BuildPlan(
                    *LoadedView,
                    AiPersonality::Adaptive,
                    Sovereign,
                    AfterLoad->PendingCommands());
            const bool bPlansMatch =
                OriginalPlan.Commands == LoadedPlan.Commands;
            TestTrue(
                TEXT("Loaded policy produces the same scheduled command"),
                bPlansMatch);
            if (!bPlansMatch)
            {
                break;
            }
            for (size_t Index = 0; Index < OriginalPlan.Commands.size(); ++Index)
            {
                std::string OriginalRejection;
                std::string LoadedRejection;
                TestEqual(
                    TEXT("Original and loaded policies agree on command admission"),
                    BeforeSave.QueueCommand(
                        OriginalPlan.Commands[Index], &OriginalRejection),
                    AfterLoad->QueueCommand(
                        LoadedPlan.Commands[Index], &LoadedRejection));
                TestTrue(
                    TEXT("Original and loaded admission feedback is identical"),
                    OriginalRejection == LoadedRejection);
            }
            BeforeSave.Step();
            AfterLoad->Step();
            TestEqual(
                TEXT("Same-policy continuation remains deterministic"),
                BeforeSave.StateChecksum(),
                AfterLoad->StateChecksum());
        }
    }

    return !HasAnyErrors();
}

#endif
