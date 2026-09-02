#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "EchoesCommandDeckLayout.h"
#include "EchoesCommandDeckModel.h"
#include "EchoesHudLayout.h"
#include "EchoesPlayerController.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesTitleOverlayLayout.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

namespace
{
struct FPointerSurfaceCase final
{
    const TCHAR* Label = TEXT("");
    FVector2D ViewportSize = FVector2D::ZeroVector;
    float HudScale = 1.0f;
};

const FPointerSurfaceCase PointerSurfaceCases[] = {
    {TEXT("1280x720 @0.85"), FVector2D(1280.0f, 720.0f), 0.85f},
    {TEXT("1280x720 @1.00"), FVector2D(1280.0f, 720.0f), 1.00f},
    {TEXT("1366x768 @1.15"), FVector2D(1366.0f, 768.0f), 1.15f},
    {TEXT("1440x900 @1.00"), FVector2D(1440.0f, 900.0f), 1.00f},
    {TEXT("1920x1080 @1.35"), FVector2D(1920.0f, 1080.0f), 1.35f},
    {TEXT("2560x1440 @1.00"), FVector2D(2560.0f, 1440.0f), 1.00f}};

[[nodiscard]] bool BoxContains(const FBox2D& Outer, const FBox2D& Inner)
{
    return Inner.Min.X >= Outer.Min.X && Inner.Min.Y >= Outer.Min.Y &&
           Inner.Max.X <= Outer.Max.X && Inner.Max.Y <= Outer.Max.Y;
}

[[nodiscard]] bool BoxesOverlap(const FBox2D& First, const FBox2D& Second)
{
    return First.Min.X < Second.Max.X && Second.Min.X < First.Max.X &&
           First.Min.Y < Second.Max.Y && Second.Min.Y < First.Max.Y;
}

[[nodiscard]] bool IsUsableControl(const FBox2D& Box)
{
    return Box.GetSize().X >= 40.0f && Box.GetSize().Y >= 12.0f;
}

/** Every profile shape the deck model can produce. */
const FEchoesCommandDeckProfile DeckProfiles[] = {
    FEchoesCommandDeckProfile{0, 3, 0, 0, false, false},
    FEchoesCommandDeckProfile{2, 0, 0, 0, false, false},
    FEchoesCommandDeckProfile{0, 0, 1, 0, true, false},
    FEchoesCommandDeckProfile{0, 0, 1, 0, false, true},
    FEchoesCommandDeckProfile{0, 0, 2, 0, true, true},
    FEchoesCommandDeckProfile{0, 0, 0, 1, false, false}};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesPointerSurfaceCoverageTest,
    "Echoes.Runtime.Controls.PointerSurfaceCoverage",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesPointerSurfaceCoverageTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    FEchoesScopedTestSaveEnvironment TestSaveEnvironment(*this);
    if (!TestSaveEnvironment.IsReady())
    {
        return false;
    }

    AddInfo(
        TEXT("Scope: shared-layout geometry plus bounded direct invocation of "
             "the public pointer seams. This is not physical-input evidence; "
             "the packaged interaction matrix (DEMO-INP-014) and the owner's "
             "physical acceptance (DEMO-INP-015) remain open."));

    // 1. Every drawn control has a usable, in-panel, non-overlapping box.
    for (const FPointerSurfaceCase& Case : PointerSurfaceCases)
    {
        FEchoesTitleOverlayFacts AllFacts;
        AllFacts.bContinueAvailable = true;
        AllFacts.bNewCampaignAvailable = true;
        AllFacts.bRestoreAvailable = true;
        const FEchoesTitleOverlayLayout Title =
            FEchoesTitleOverlayLayout::Build(Case.ViewportSize, AllFacts);
        const FBox2D TitleControls[] = {
            Title.OperationButton, Title.FactionButton, Title.ContinueButton,
            Title.NewCampaignButton, Title.RestoreButton,
            Title.OpenBriefButton};
        for (const FBox2D& Control : TitleControls)
        {
            TestTrue(
                *FString::Printf(
                    TEXT("%s: title control is usable"), Case.Label),
                IsUsableControl(Control));
            TestTrue(
                *FString::Printf(
                    TEXT("%s: title control stays inside its panel"),
                    Case.Label),
                BoxContains(Title.Panel, Control));
        }
        for (int32 First = 0; First < UE_ARRAY_COUNT(TitleControls); ++First)
        {
            for (int32 Second = First + 1;
                 Second < UE_ARRAY_COUNT(TitleControls); ++Second)
            {
                TestFalse(
                    *FString::Printf(
                        TEXT("%s: title controls %d and %d do not overlap"),
                        Case.Label, First, Second),
                    BoxesOverlap(TitleControls[First], TitleControls[Second]));
            }
        }

        const FEchoesBriefingOverlayLayout Briefing =
            FEchoesBriefingOverlayLayout::Build(Case.ViewportSize);
        TestTrue(
            *FString::Printf(TEXT("%s: briefing operation control usable"),
                             Case.Label),
            IsUsableControl(Briefing.OperationButton) &&
                BoxContains(Briefing.Panel, Briefing.OperationButton));
        TestTrue(
            *FString::Printf(TEXT("%s: briefing deploy control usable"),
                             Case.Label),
            IsUsableControl(Briefing.DeployButton) &&
                BoxContains(Briefing.Panel, Briefing.DeployButton));
        TestFalse(
            *FString::Printf(TEXT("%s: briefing controls do not overlap"),
                             Case.Label),
            BoxesOverlap(Briefing.OperationButton, Briefing.DeployButton));

        const FEchoesLobbyOverlayLayout Lobby =
            FEchoesLobbyOverlayLayout::Build(
                Case.ViewportSize, Case.HudScale);
        TestTrue(
            *FString::Printf(TEXT("%s: lobby ready control usable"),
                             Case.Label),
            IsUsableControl(Lobby.ReadyButton) &&
                BoxContains(Lobby.Panel, Lobby.ReadyButton));

        // 2. Deck buttons stay inside the deck panel and never overlap. Where
        // the panel is too small the layout must report no buttons at all
        // rather than unusable ones.
        const FEchoesHudLayout HudLayout = FEchoesHudLayout::Build(
            Case.ViewportSize, Case.HudScale, true);
        for (const FEchoesCommandDeckProfile& Profile : DeckProfiles)
        {
            const TArray<FEchoesCommandDeckActionEntry, TInlineAllocator<6>>
                Entries = FEchoesCommandDeckModel::BuildActionEntries(Profile);
            TestTrue(
                *FString::Printf(
                    TEXT("%s: deck profile yields at least one action"),
                    Case.Label),
                Entries.Num() > 0);
            const FEchoesCommandDeckLayout Deck =
                FEchoesCommandDeckLayout::Build(
                    HudLayout.CommandDeckPanel, Case.HudScale, Entries.Num());
            if (Deck.Buttons.IsEmpty())
            {
                continue;
            }
            TestEqual(
                *FString::Printf(
                    TEXT("%s: deck exposes one button per action"),
                    Case.Label),
                Deck.Buttons.Num(),
                Entries.Num());
            for (int32 Index = 0; Index < Deck.Buttons.Num(); ++Index)
            {
                TestTrue(
                    *FString::Printf(
                        TEXT("%s: deck button %d stays inside its panel"),
                        Case.Label, Index),
                    BoxContains(HudLayout.CommandDeckPanel,
                                Deck.Buttons[Index]));
                TestEqual(
                    *FString::Printf(
                        TEXT("%s: deck button %d hit-tests to itself"),
                        Case.Label, Index),
                    Deck.HitTest(Deck.Buttons[Index].GetCenter()),
                    Index);
                for (int32 Other = Index + 1; Other < Deck.Buttons.Num();
                     ++Other)
                {
                    TestFalse(
                        *FString::Printf(
                            TEXT("%s: deck buttons %d and %d do not overlap"),
                            Case.Label, Index, Other),
                        BoxesOverlap(Deck.Buttons[Index],
                                     Deck.Buttons[Other]));
                }
            }
        }
    }

    // 3. Cursor-targeted deck actions must arm rather than fire at the panel.
    {
        const TArray<FEchoesCommandDeckActionEntry, TInlineAllocator<6>>
            WorkerEntries = FEchoesCommandDeckModel::BuildActionEntries(
                FEchoesCommandDeckProfile{2, 0, 0, 0, false, false});
        bool bFoundBuild = false;
        for (const FEchoesCommandDeckActionEntry& Entry : WorkerEntries)
        {
            if (Entry.Action == EEchoesCommandDeckAction::BuildBarracks)
            {
                bFoundBuild = true;
                TestTrue(
                    TEXT("Build actions require a battlefield target"),
                    Entry.bRequiresCursorTarget);
            }
            if (Entry.Action == EEchoesCommandDeckAction::Stop)
            {
                TestFalse(
                    TEXT("Stop resolves immediately"),
                    Entry.bRequiresCursorTarget);
            }
        }
        TestTrue(TEXT("Worker deck offers its build actions"), bFoundBuild);
    }

    // 4. Live seams: a press on the interface is consumed, a press on the
    // battlefield is not, and the campaign title routes to its handlers.
    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Pointer coverage could not create its test world."));
        return false;
    }
    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge = World != nullptr
        ? World->GetSubsystem<UEchoesSimulationSubsystem>()
        : nullptr;
    if (Bridge == nullptr || !Bridge->StartPrototypeScenario())
    {
        AddError(TEXT("Pointer coverage could not start the scenario."));
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    AEchoesPlayerController* Controller =
        World->SpawnActor<AEchoesPlayerController>();
    if (Controller == nullptr)
    {
        AddError(TEXT("Pointer coverage could not spawn the controller."));
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const FVector2D ViewportSize(1920.0f, 1080.0f);
    const FEchoesHudLayout HudLayout =
        FEchoesHudLayout::Build(ViewportSize, 1.0f, true);
    TestTrue(
        TEXT("A press on the main HUD panel is consumed"),
        Controller->HandleBattlefieldPointerPressed(
            HudLayout.MainPanel.GetCenter(), ViewportSize));
    if (HudLayout.bCommandDeckVisible)
    {
        TestTrue(
            TEXT("A press on the command deck is consumed"),
            Controller->HandleBattlefieldPointerPressed(
                HudLayout.CommandDeckPanel.GetCenter(), ViewportSize));
    }
    if (HudLayout.bMinimapVisible)
    {
        TestTrue(
            TEXT("A press on the minimap is consumed"),
            Controller->HandleBattlefieldPointerPressed(
                HudLayout.MinimapPanel.GetCenter(), ViewportSize));
    }
    const FVector2D ClearPoint(
        ViewportSize.X * 0.5f,
        HudLayout.MainPanel.Max.Y + 40.0f);
    if (HudLayout.IsBattlefieldPointClear(ClearPoint, ViewportSize))
    {
        TestFalse(
            TEXT("A press on clear battlefield is not consumed"),
            Controller->HandleBattlefieldPointerPressed(
                ClearPoint, ViewportSize));
    }
    TestEqual(
        TEXT("No deck action is armed by interface presses"),
        static_cast<int32>(Controller->GetArmedDeckAction()),
        static_cast<int32>(EEchoesCommandDeckAction::None));

    // An armed order must not survive a scenario transition. Every end,
    // restart, load, and menu return clears the selection, and an armed order
    // is meaningless without one; without the disarm an armed build could
    // fire into the first battlefield click of the next context.
    Controller->ActivateCommandDeckAction(
        EEchoesCommandDeckAction::BuildBarracks);
    TestEqual(
        TEXT("A cursor-targeted deck action arms rather than firing"),
        static_cast<int32>(Controller->GetArmedDeckAction()),
        static_cast<int32>(EEchoesCommandDeckAction::BuildBarracks));
    Controller->PresentTitleScreen();
    TestEqual(
        TEXT("A scenario transition disarms the pending order"),
        static_cast<int32>(Controller->GetArmedDeckAction()),
        static_cast<int32>(EEchoesCommandDeckAction::None));

    // Campaign title: leave skirmish so the campaign control block applies,
    // then prove the pointer reaches the same handler the F9 key uses.
    Controller->PresentTitleScreen();
    Controller->CycleOperation();
    if (Controller->IsTitleScreenVisible() &&
        !Controller->IsSkirmishSetupVisible())
    {
        const EEchoesOperationMode BeforeMode = Bridge->GetOperationMode();
        const FEchoesTitleOverlayLayout Title =
            FEchoesTitleOverlayLayout::Build(
                ViewportSize, Controller->BuildTitleOverlayFacts());
        TestTrue(
            TEXT("Campaign title consumes its own pointer presses"),
            Controller->HandleModalOverlayPointer(
                Title.OperationButton.GetCenter(), ViewportSize, 1.0f));
        TestNotEqual(
            TEXT("Clicking the operation control changes the operation"),
            static_cast<int32>(Bridge->GetOperationMode()),
            static_cast<int32>(BeforeMode));
    }
    else
    {
        AddInfo(
            TEXT("Campaign title context unavailable in this fixture; the "
                 "campaign-control routing case did not run."));
    }

    Controller->Destroy();
    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif
