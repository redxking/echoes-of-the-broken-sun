#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "EchoesNarrativeSubsystem.h"
#include "EchoesPlayerController.h"
#include "EchoesPlayerProfile.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

/**
 * The mission briefing. REL-UI-010 requires objectives and irreversible-choice
 * warnings; the screen was the operation label, one hardcoded sentence chosen
 * from a sixteen-way chain, and a Deploy button, while 226 authored strings
 * across fifteen operations sat unread in the narrative pack. This asserts the
 * briefing is resolved from the pack rather than restated in C++, that the
 * permanent Future Well decision is warned about before the player can reach
 * it, and that the drill which never writes the ledger does not claim it does.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesMissionBriefingTerminalTest,
    "Echoes.Runtime.UI.MissionBriefingTerminal",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesMissionBriefingTerminalTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    FEchoesScopedTestSaveEnvironment SaveEnvironment(*this);
    if (!SaveEnvironment.IsReady()) return false;

    FTestWorldWrapper Wrapper;
    if (!Wrapper.CreateTestWorld(EWorldType::Game))
    {
        Wrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the briefing-terminal test world."));
        return false;
    }
    UWorld* World = Wrapper.GetTestWorld();
    auto* Bridge = World != nullptr
        ? World->GetSubsystem<UEchoesSimulationSubsystem>() : nullptr;
    if (!TestNotNull(TEXT("World owns the simulation subsystem"), Bridge))
    {
        return false;
    }
    UGameInstance* GameInstance = World->GetGameInstance();
    const UEchoesNarrativeSubsystem* Narrative = GameInstance != nullptr
        ? GameInstance->GetSubsystem<UEchoesNarrativeSubsystem>() : nullptr;
    if (!TestNotNull(TEXT("The narrative pack subsystem is available"), Narrative))
    {
        return false;
    }

    FString Feedback;
    if (!TestTrue(
            TEXT("Mission 01 can be selected"),
            Bridge->SelectOperationMode(
                EEchoesOperationMode::CampaignPrologue, Feedback)) ||
        !TestTrue(TEXT("Mission 01 stages"), Bridge->StartPrototypeScenario()))
    {
        AddError(Feedback);
        Wrapper.ForwardErrorMessages(this);
        return false;
    }

    auto* Controller = World->SpawnActor<AEchoesPlayerController>();
    if (!TestNotNull(TEXT("Briefing controller spawns"), Controller))
    {
        Bridge->StopPrototypeScenario();
        Wrapper.ForwardErrorMessages(this);
        return false;
    }
    Controller->InitInputSystem();
    Controller->InitializePlayerProfile();
    Controller->PresentMissionBriefing();

    const FEchoesShellView Briefing = Controller->BuildShellView();
    TestTrue(
        TEXT("The briefing screen is presented"),
        Briefing.Screen == EEchoesShellScreen::Briefing);
    const FString Body = Briefing.Body.ToString();

    // --- Objectives come from the pack -------------------------------------
    const TArray<FString> Objectives =
        Narrative->GetObjectives(EEchoesOperationMode::CampaignPrologue);
    if (!TestTrue(
            TEXT("Mission 01 has authored objectives to show"),
            Objectives.Num() > 0))
    {
        Controller->Destroy();
        Bridge->StopPrototypeScenario();
        return false;
    }
    for (const FString& Objective : Objectives)
    {
        TestTrue(
            FString::Printf(
                TEXT("The briefing lists the authored objective: %s"),
                *Objective.Left(48)),
            Body.Contains(Objective));
    }
    const FString AuthoredBriefing =
        Narrative->GetBriefing(EEchoesOperationMode::CampaignPrologue);
    if (!AuthoredBriefing.IsEmpty())
    {
        TestTrue(
            TEXT("The briefing body is the authored briefing, not a restatement"),
            Body.Contains(AuthoredBriefing));
    }

    // --- The permanent decision is disclosed before deployment -------------
    bool bStagesFutureWell = false;
    if (const auto* Simulation = Bridge->GetSimulation())
    {
        for (const auto& Entity : Simulation->Entities())
        {
            bStagesFutureWell = bStagesFutureWell ||
                Entity.type == echoes::sim::EntityType::FutureWell;
        }
    }
    TestTrue(
        TEXT("Mission 01 stages the Future Well the warning is about"),
        bStagesFutureWell);
    TestTrue(
        TEXT("The briefing warns that the Well protocol is irreversible"),
        Body.Contains(TEXT("IRREVERSIBLE DECISION")));

    // --- Deploy is still gated on a staged scenario ------------------------
    const FEchoesShellButton* Deploy = Briefing.Buttons.FindByPredicate(
        [](const FEchoesShellButton& Button)
        { return Button.Action == EEchoesShellAction::Primary; });
    TestTrue(
        TEXT("A staged mission offers Deploy"),
        Deploy != nullptr && Deploy->bEnabled);

    // --- The readiness drill never claims a ledger write -------------------
    Bridge->StopPrototypeScenario();
    if (TestTrue(
            TEXT("The readiness drill can be selected"),
            Bridge->SelectOperationMode(
                EEchoesOperationMode::TrainingReadiness, Feedback) &&
                Bridge->StartPrototypeScenario()))
    {
        Controller->PresentMissionBriefing();
        const FString DrillBody = Controller->BuildShellView().Body.ToString();
        TestFalse(
            TEXT("The drill briefing makes no irreversible-ledger claim"),
            DrillBody.Contains(TEXT("IRREVERSIBLE DECISION")));
    }

    Controller->Destroy();
    Bridge->StopPrototypeScenario();
    Wrapper.ForwardErrorMessages(this);
    return true;
}

#endif
