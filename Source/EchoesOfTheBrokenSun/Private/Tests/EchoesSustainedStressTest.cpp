#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "EchoesEntityView.h"
#include "EchoesSimCore/Simulation.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/FileHelper.h"
#include "Tests/AutomationCommon.h"

#include <array>
#include <cmath>
#include <limits>
#include <optional>
#include <string>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesSustainedStressTest,
    "Echoes.Runtime.Performance.SustainedFourTeamScale",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesSustainedStressTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    FEchoesScopedTestSaveEnvironment TestSaveEnvironment(*this);
    if (!TestSaveEnvironment.IsReady())
    {
        return false;
    }

    const TArray<uint8> SentinelSave{0x45, 0x43, 0x48, 0x4f, 0x53, 0x41, 0x46, 0x45};
    if (!FFileHelper::SaveArrayToFile(
            SentinelSave,
            *UEchoesSimulationSubsystem::GetQuickSavePath()))
    {
        AddError(TEXT("Could not create the isolated stress-save sentinel."));
        return false;
    }

    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the sustained-scale test world."));
        return false;
    }
    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge =
        World != nullptr
            ? World->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (!TestNotNull(TEXT("Sustained world owns the simulation subsystem"), Bridge))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    FString Feedback;
    TestTrue(TEXT("Legacy four-team fixture still starts"),
             Bridge->StartStressScenario());
    TestTrue(TEXT("Legacy fixture remains distinct from sustained mode"),
             Bridge->IsStressScenario() &&
                 !Bridge->IsSustainedStressScenario());
    TestFalse(TEXT("Legacy stress quick save is isolated"),
              Bridge->QuickSaveScenario(Feedback));
    TestTrue(TEXT("Legacy save rejection is stable"),
             Feedback.Contains(TEXT("[SAVE_STRESS_DISABLED]")));
    TestFalse(TEXT("Legacy stress quick load is isolated"),
              Bridge->QuickLoadScenario(Feedback));
    TestTrue(TEXT("Legacy load rejection is stable"),
             Feedback.Contains(TEXT("[LOAD_STRESS_DISABLED]")));
    Bridge->StopPrototypeScenario();

    TArray<uint8> RetainedSave;
    TestTrue(TEXT("Legacy stress leaves the sentinel readable"),
             FFileHelper::LoadFileToArray(
                 RetainedSave,
                 *UEchoesSimulationSubsystem::GetQuickSavePath()));
    TestTrue(TEXT("Legacy stress leaves the sentinel byte-for-byte unchanged"),
             RetainedSave == SentinelSave);

    if (!TestTrue(TEXT("Sustained four-team fixture starts"),
                  Bridge->StartSustainedStressScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    TestTrue(TEXT("Sustained fixture exposes both stress-mode identities"),
             Bridge->IsStressScenario() &&
                 Bridge->IsSustainedStressScenario());
    TestFalse(TEXT("Sustained fixture starts without a latched failure"),
              Bridge->HasSustainedStressFailed());

    const echoes::sim::Simulation* Simulation = Bridge->GetSimulation();
    if (!TestNotNull(TEXT("Sustained fixture has deterministic state"), Simulation))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    TestEqual(TEXT("Only the sustained fixture protects all four Command Cores"),
              Simulation->Config().protectedCommandCorePlayerMask,
              static_cast<uint8>(0x0f));
    TestEqual(TEXT("Sustained fixture starts with 401 authoritative entities"),
              static_cast<int32>(Simulation->Entities().size()),
              401);
    TestEqual(TEXT("Sustained fixture starts with 396 bounded commands"),
              static_cast<int32>(Simulation->CommandLog().size()),
              396);

    std::array<int32, echoes::sim::kMaximumPlayers> TeamCounts{};
    std::array<int32, echoes::sim::kMaximumPlayers> CoreCounts{};
    std::array<int32, 3> FactionCounts{};
    int32 SoldierCount = 0;
    int32 HeavyCount = 0;
    int32 ScoutCount = 0;
    int32 DormantWellCount = 0;
    for (const echoes::sim::Entity& Entity : Simulation->Entities())
    {
        if (Entity.owner < echoes::sim::kMaximumPlayers)
        {
            ++TeamCounts[Entity.owner];
            ++FactionCounts[static_cast<uint8>(Entity.faction)];
            CoreCounts[Entity.owner] +=
                Entity.type == echoes::sim::EntityType::CommandCore ? 1 : 0;
            SoldierCount +=
                Entity.type == echoes::sim::EntityType::Soldier ? 1 : 0;
            HeavyCount +=
                Entity.type == echoes::sim::EntityType::HeavyUnit ? 1 : 0;
            ScoutCount +=
                Entity.type == echoes::sim::EntityType::ScoutUnit ? 1 : 0;
        }
        else if (Entity.owner == echoes::sim::kNeutralPlayer &&
                 Entity.type == echoes::sim::EntityType::FutureWell &&
                 Entity.wellChoice == echoes::sim::FutureWellChoice::Dormant)
        {
            ++DormantWellCount;
        }
    }
    for (echoes::sim::PlayerId Player = 0;
         Player < echoes::sim::kMaximumPlayers;
         ++Player)
    {
        TestEqual(
            *FString::Printf(TEXT("Sustained team %u owns exactly 100 entities"), Player),
            TeamCounts[Player],
            100);
        TestEqual(
            *FString::Printf(TEXT("Sustained team %u owns exactly one Command Core"), Player),
            CoreCounts[Player],
            1);
    }
    TestEqual(TEXT("Meridian owns two sustained teams"), FactionCounts[0], 200);
    TestEqual(TEXT("Kharuun owns one sustained team"), FactionCounts[1], 100);
    TestEqual(TEXT("Hollow Choir owns one sustained team"), FactionCounts[2], 100);
    TestEqual(TEXT("Sustained fixture has 132 Soldiers"), SoldierCount, 132);
    TestEqual(TEXT("Sustained fixture has 132 Heavy units"), HeavyCount, 132);
    TestEqual(TEXT("Sustained fixture has 132 Scouts"), ScoutCount, 132);
    TestEqual(TEXT("Sustained fixture has one neutral dormant Future Well"),
              DormantWellCount,
              1);

    int32 VisibleViewCount = 0;
    for (TActorIterator<AEchoesEntityView> It(World); It; ++It)
    {
        ++VisibleViewCount;
    }
    TestEqual(TEXT("Sustained fixture synchronizes all 401 views before readiness"),
              VisibleViewCount,
              401);

    TestFalse(TEXT("Sustained quick save is isolated"),
              Bridge->QuickSaveScenario(Feedback));
    TestTrue(TEXT("Sustained save rejection is stable"),
             Feedback.Contains(TEXT("[SAVE_STRESS_DISABLED]")));
    TestFalse(TEXT("Sustained quick load is isolated"),
              Bridge->QuickLoadScenario(Feedback));
    TestTrue(TEXT("Sustained load rejection is stable"),
             Feedback.Contains(TEXT("[LOAD_STRESS_DISABLED]")));
    RetainedSave.Reset();
    TestTrue(TEXT("Sustained stress leaves the sentinel readable"),
             FFileHelper::LoadFileToArray(
                 RetainedSave,
                 *UEchoesSimulationSubsystem::GetQuickSavePath()));
    TestTrue(TEXT("Sustained stress leaves the sentinel byte-for-byte unchanged"),
             RetainedSave == SentinelSave);

    TestFalse(TEXT("Sustained active timing begins unarmed"),
              Bridge->IsSustainedStressTimingReady());
    Bridge->Tick(std::numeric_limits<float>::max());
    TestFalse(TEXT("An extreme finite bootstrap frame safely resets stabilization"),
              Bridge->HasSustainedStressFailed());
    TestFalse(TEXT("An extreme finite bootstrap frame does not arm timing"),
              Bridge->IsSustainedStressTimingReady());
    TestEqual(TEXT("An extreme finite bootstrap frame advances no simulation ticks"),
              Simulation->CurrentTick(),
              static_cast<echoes::sim::Tick>(0));
    Bridge->Tick(std::nextafter(
        0.25F,
        std::numeric_limits<float>::infinity()));
    TestFalse(TEXT("The pre-active bootstrap frame does not latch a time-clamp failure"),
              Bridge->HasSustainedStressFailed());
    TestEqual(TEXT("The pre-active bootstrap frame is excluded from simulation time"),
              Simulation->CurrentTick(),
              static_cast<echoes::sim::Tick>(0));
    for (int32 FrameIndex = 0; FrameIndex < 20; ++FrameIndex)
    {
        Bridge->Tick(0.01F);
    }
    TestFalse(TEXT("The stable-frame threshold alone cannot arm timing"),
              Bridge->IsSustainedStressTimingReady());
    TestEqual(TEXT("Frame-only stabilization advances no simulation ticks"),
              Simulation->CurrentTick(),
              static_cast<echoes::sim::Tick>(0));

    Bridge->Tick(0.30F);
    for (int32 FrameIndex = 0; FrameIndex < 4; ++FrameIndex)
    {
        Bridge->Tick(0.25F);
    }
    TestFalse(TEXT("The stable-time threshold alone cannot arm timing"),
              Bridge->IsSustainedStressTimingReady());
    TestEqual(TEXT("Time-only stabilization advances no simulation ticks"),
              Simulation->CurrentTick(),
              static_cast<echoes::sim::Tick>(0));

    Bridge->Tick(0.30F);
    for (int32 FrameIndex = 0; FrameIndex < 19; ++FrameIndex)
    {
        Bridge->Tick(0.05F);
    }
    TestFalse(TEXT("Nineteen stable frames do not arm timing"),
              Bridge->IsSustainedStressTimingReady());
    Bridge->Tick(0.05F);
    TestTrue(TEXT("Twenty stable frames spanning one second arm timing"),
             Bridge->IsSustainedStressTimingReady());
    TestEqual(TEXT("Timing arms at deterministic tick zero"),
              Simulation->CurrentTick(),
              static_cast<echoes::sim::Tick>(0));

    for (int32 TickIndex = 0; TickIndex < 200; ++TickIndex)
    {
        Bridge->Tick(0.05F);
    }
    Simulation = Bridge->GetSimulation();
    TestFalse(TEXT("Ten simulated seconds retain the sustained contract"),
              Bridge->HasSustainedStressFailed());
    TestEqual(TEXT("Ten simulated seconds advance exactly 200 fixed ticks"),
              Simulation->CurrentTick(),
              static_cast<echoes::sim::Tick>(200));
    TestTrue(TEXT("Natural deterministic attrition exercises replacement"),
             Bridge->GetSustainedStressReplacementCount() > 0);
    TestEqual(TEXT("Replacement retains all 401 authoritative entities"),
              static_cast<int32>(Simulation->Entities().size()),
              401);
    const uint64 FirstChecksum = Simulation->StateChecksum();
    const uint64 FirstReplacementCount =
        Bridge->GetSustainedStressReplacementCount();
    const int32 FirstCommandCount =
        static_cast<int32>(Simulation->CommandLog().size());
    std::string ReplayError;
    const echoes::sim::ReplayRecord SustainedReplay =
        Simulation->ExportReplay(&ReplayError);
    TestEqual(
        TEXT("Sustained replay export fails closed with a stable reason"),
        FString(UTF8_TO_TCHAR(ReplayError.c_str())),
        FString(TEXT("replay export is disabled")));
    TestEqual(TEXT("Rejected sustained replay has no supported version"),
              SustainedReplay.version,
              static_cast<uint32>(0));
    TestTrue(TEXT("Rejected sustained replay has no baseline payload"),
             SustainedReplay.initialSnapshot.empty());
    TestTrue(TEXT("Rejected sustained replay has no command payload"),
             SustainedReplay.commands.empty());
    TestEqual(TEXT("Rejected sustained replay has no final tick"),
              SustainedReplay.finalTick,
              static_cast<echoes::sim::Tick>(0));
    TestEqual(TEXT("Rejected sustained replay has no final checksum"),
              SustainedReplay.finalChecksum,
              static_cast<uint64>(0));

    AddExpectedError(
        TEXT("code=SIM_TIME_CLAMP"),
        EAutomationExpectedErrorFlags::Contains,
        1);
    Bridge->Tick(0.30F);
    TestTrue(TEXT("An active-window frame above 250 ms still fails closed"),
             Bridge->HasSustainedStressFailed());
    TestFalse(TEXT("An active-window time clamp clears scenario readiness"),
              Bridge->IsScenarioReady());

    if (!TestTrue(TEXT("Restart preserves sustained fixture mode"),
                  Bridge->RestartPrototypeScenario()) ||
        !TestNotNull(
            TEXT("Restarted sustained fixture has deterministic state"),
            Bridge->GetSimulation()))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    TestTrue(TEXT("Restarted fixture is still sustained"),
             Bridge->IsStressScenario() &&
                 Bridge->IsSustainedStressScenario());
    Simulation = Bridge->GetSimulation();
    TestEqual(TEXT("Restart resets the deterministic tick"),
              Simulation->CurrentTick(),
              static_cast<echoes::sim::Tick>(0));
    TestFalse(TEXT("Restart resets sustained timing stabilization"),
              Bridge->IsSustainedStressTimingReady());
    for (int32 FrameIndex = 0; FrameIndex < 20; ++FrameIndex)
    {
        Bridge->Tick(0.05F);
    }
    TestTrue(TEXT("Restart re-arms timing only after stabilization"),
             Bridge->IsSustainedStressTimingReady());
    TestEqual(TEXT("Restarted timing arms at deterministic tick zero"),
              Simulation->CurrentTick(),
              static_cast<echoes::sim::Tick>(0));
    for (int32 TickIndex = 0; TickIndex < 200; ++TickIndex)
    {
        Bridge->Tick(0.05F);
    }
    Simulation = Bridge->GetSimulation();
    TestFalse(TEXT("Restarted run retains the sustained contract"),
              Bridge->HasSustainedStressFailed());
    TestEqual(TEXT("Restarted run reproduces the authoritative checksum"),
              Simulation->StateChecksum(),
              FirstChecksum);
    TestEqual(TEXT("Restarted run reproduces the replacement count"),
              Bridge->GetSustainedStressReplacementCount(),
              FirstReplacementCount);
    TestEqual(TEXT("Restarted run reproduces the bounded command count"),
              static_cast<int32>(Simulation->CommandLog().size()),
              FirstCommandCount);
    ReplayError.clear();
    const echoes::sim::ReplayRecord RestartedSustainedReplay =
        Simulation->ExportReplay(&ReplayError);
    TestEqual(TEXT("Restart cannot re-enable sustained replay export"),
              FString(UTF8_TO_TCHAR(ReplayError.c_str())),
              FString(TEXT("replay export is disabled")));
    TestEqual(TEXT("Restarted sustained replay remains unusable"),
              RestartedSustainedReplay.version,
              static_cast<uint32>(0));

    AddExpectedError(
        TEXT("code=PAUSE_REQUESTED"),
        EAutomationExpectedErrorFlags::Contains,
        1);
    Bridge->SetScenarioPaused(true);
    TestTrue(TEXT("A forbidden pause latches sustained failure"),
             Bridge->HasSustainedStressFailed());
    TestFalse(TEXT("A failed sustained fixture is no longer ready"),
              Bridge->IsScenarioReady());
    AddExpectedError(
        TEXT("code=FAILURE_LATCHED"),
        EAutomationExpectedErrorFlags::Contains,
        1);
    TestFalse(TEXT("Same-mode start cannot mask a latched sustained failure"),
              Bridge->StartSustainedStressScenario());
    TestTrue(TEXT("Explicit restart clears the failure and rebuilds the fixture"),
             Bridge->RestartPrototypeScenario());
    TestTrue(TEXT("Restarted fixture returns ready without a failure latch"),
             Bridge->IsScenarioReady() &&
                 Bridge->IsSustainedStressScenario() &&
                 !Bridge->HasSustainedStressFailed());

    AddExpectedError(
        TEXT("code=SIM_STARTUP_TIME_INVALID"),
        EAutomationExpectedErrorFlags::Contains,
        2);
    Bridge->Tick(-0.01F);
    TestTrue(TEXT("Negative startup time fails closed"),
             Bridge->HasSustainedStressFailed());
    TestTrue(TEXT("Restart clears a negative startup-time failure"),
             Bridge->RestartPrototypeScenario());
    Bridge->Tick(std::numeric_limits<float>::quiet_NaN());
    TestTrue(TEXT("Nonfinite startup time fails closed"),
             Bridge->HasSustainedStressFailed());
    TestTrue(TEXT("Restart clears a nonfinite startup-time failure"),
             Bridge->RestartPrototypeScenario());

    Bridge->StopPrototypeScenario();
    TestFalse(TEXT("Stop clears stress mode"), Bridge->IsStressScenario());
    TestFalse(TEXT("Stop clears sustained mode"),
              Bridge->IsSustainedStressScenario());
    TestFalse(TEXT("Stop clears scenario readiness"),
              Bridge->IsScenarioReady());
    int32 RemainingViewCount = 0;
    for (TActorIterator<AEchoesEntityView> It(World); It; ++It)
    {
        ++RemainingViewCount;
    }
    TestEqual(TEXT("Stop releases every sustained entity view"),
              RemainingViewCount,
              0);

    TestTrue(TEXT("Ordinary scenario starts after sustained teardown"),
             Bridge->StartPrototypeScenario());
    Simulation = Bridge->GetSimulation();
    if (TestNotNull(TEXT("Ordinary scenario exposes deterministic state"), Simulation))
    {
        ReplayError.clear();
        const echoes::sim::ReplayRecord OrdinaryReplay =
            Simulation->ExportReplay(&ReplayError);
        TestTrue(TEXT("Ordinary replay export remains enabled"),
                 ReplayError.empty() &&
                     OrdinaryReplay.version == echoes::sim::kReplayVersion);
        std::optional<echoes::sim::Simulation> ReplayedOrdinary =
            echoes::sim::Simulation::ReplayToEnd(
                OrdinaryReplay,
                &ReplayError);
        TestTrue(TEXT("Ordinary replay remains reconstructable"),
                 ReplayedOrdinary.has_value() && ReplayError.empty());
        if (ReplayedOrdinary.has_value())
        {
            TestEqual(TEXT("Ordinary replay reconstructs the exact checksum"),
                      ReplayedOrdinary->StateChecksum(),
                      Simulation->StateChecksum());
        }
    }
    Bridge->StopPrototypeScenario();

    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif
