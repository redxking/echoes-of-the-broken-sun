// Copyright Echoes of the Broken Sun. All Rights Reserved.
// Author: Angelis Pseftis

#if WITH_DEV_AUTOMATION_TESTS

#include "EchoesMatchReplay.h"
#include "EchoesNetworkSession.h"
#include "EchoesPlayerController.h"
#include "EchoesSkirmishSetup.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesTestSaveEnvironment.h"
#include "Engine/World.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

namespace
{
echoes::sim::ReplayRecord MakeBrowserReplay(EEchoesSkirmishMapPreset MapPreset)
{
    using namespace echoes::sim;
    SimulationConfig Config{FEchoesSkirmishSetupModel::MapWidthTiles,
        FEchoesSkirmishSetupModel::MapHeightTiles, 20, 0x42524f5753455254ULL};
    Config.rules.contentSha256 =
        echoes::network::BuildCompatibilityManifest(nullptr).rulesPackSha256;
    Simulation SimulationValue(Config);
    for (int32 Y = 0; Y < FEchoesSkirmishSetupModel::MapHeightTiles; ++Y)
    {
        for (int32 X = 0; X < FEchoesSkirmishSetupModel::MapWidthTiles; ++X)
        {
            if (FEchoesSkirmishSetupModel::IsBlockedTile(MapPreset, X, Y))
            {
                (void)SimulationValue.SetTerrainTile(X, Y, Terrain::Blocked);
            }
        }
    }
    SimulationValue.AddPlayer(
        0, Faction::MeridianCompact, {1000, 1000});
    SimulationValue.AddPlayer(
        1, Faction::KharuunAssemblies, {1000, 1000});
    SimulationValue.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::CommandCore,
        Vec2::FromTiles(2, 2));
    SimulationValue.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::CommandCore,
        Vec2::FromTiles(18, 18));
    SimulationValue.CaptureReplayBaseline();
    SimulationValue.Step(2);
    SimulationValue.ForfeitPlayer(0);
    return SimulationValue.ExportReplay();
}

bool SaveBrowserReplay(
    FAutomationTestBase& Test,
    const echoes::sim::ReplayRecord& Replay,
    const FString& ReplayId,
    const FString& MapId,
    const FDateTime& RecordedUtc)
{
    FEchoesReplayMetadata Metadata;
    Metadata.ReplayId = ReplayId;
    Metadata.MapId = MapId;
    Metadata.OperationId = TEXT("skirmish");
    Metadata.BuildIdentity = TEXT("replay-browser-automation");
    Metadata.RecordedUtc = RecordedUtc;
    Metadata.OperationType = EEchoesReplayOperationType::Skirmish;
    Metadata.bOperationCompleted = true;

    FEchoesReplayEnvelope Envelope;
    FString Error;
    if (!Test.TestTrue(
            *FString::Printf(TEXT("%s finalizes"), *ReplayId),
            FEchoesMatchReplayStore::FinalizeEnvelope(
                Metadata, Replay, Envelope, Error)))
    {
        Test.AddError(Error);
        return false;
    }
    FString Path;
    if (!Test.TestTrue(
            *FString::Printf(TEXT("%s saves"), *ReplayId),
            FEchoesMatchReplayStore::SaveAtomic(
                FEchoesMatchReplayStore::GetReplayDirectory(),
                Envelope, Path, Error)))
    {
        Test.AddError(Error);
        return false;
    }
    return true;
}

bool WaitForBrowserScan(
    FAutomationTestBase& Test,
    AEchoesPlayerController& Controller,
    const TCHAR* Assertion)
{
    const double Deadline = FPlatformTime::Seconds() + 15.0;
    while (Controller.IsReplayBrowserLoading() &&
           FPlatformTime::Seconds() < Deadline)
    {
        FPlatformProcess::SleepNoStats(0.001f);
        Controller.PollReplayBrowser();
    }
    Controller.PollReplayBrowser();
    return Test.TestFalse(Assertion, Controller.IsReplayBrowserLoading());
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesReplayBrowserTest,
    "Echoes.Runtime.Replay.AsyncBrowserGeneration",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesReplayBrowserTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    FEchoesScopedTestSaveEnvironment Saves(*this);
    if (!Saves.IsReady())
    {
        return false;
    }

    const echoes::sim::ReplayRecord Replay =
        MakeBrowserReplay(EEchoesSkirmishMapPreset::GlassScar);
    const echoes::sim::ReplayRecord CrownReplay =
        MakeBrowserReplay(EEchoesSkirmishMapPreset::CrownfallBasin);
    const echoes::sim::ReplayRecord SorynReplay =
        MakeBrowserReplay(EEchoesSkirmishMapPreset::SorynConfluence);
    if (!TestTrue(TEXT("Browser fixture replay is terminal"),
            Replay.version != 0 && Replay.finalTick > 0 &&
                Replay.forfeitingPlayer == 0 && CrownReplay.version != 0 &&
                CrownReplay.forfeitingPlayer == 0 && SorynReplay.version != 0 &&
                SorynReplay.forfeitingPlayer == 0))
    {
        return false;
    }
    TestTrue(TEXT("Archive filters use independently authored terrain baselines"),
        Replay.initialSnapshot != CrownReplay.initialSnapshot &&
        CrownReplay.initialSnapshot != SorynReplay.initialSnapshot &&
        Replay.initialSnapshot != SorynReplay.initialSnapshot);
    const FDateTime RecordedUtc(2026, 9, 5, 12, 0, 0);
    if (!SaveBrowserReplay(
            *this, Replay, TEXT("browser-glass"), TEXT("glass-scar"),
            RecordedUtc) ||
        !SaveBrowserReplay(
            *this, CrownReplay, TEXT("browser-crown"),
            TEXT("crownfall-basin"), RecordedUtc + FTimespan::FromMinutes(1)) ||
        !SaveBrowserReplay(
            *this, SorynReplay, TEXT("browser-soryn"),
            TEXT("soryn-confluence"), RecordedUtc + FTimespan::FromMinutes(2)))
    {
        return false;
    }

    FTestWorldWrapper Wrapper;
    if (!Wrapper.CreateTestWorld(EWorldType::Game))
    {
        Wrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the replay-browser test world."));
        return false;
    }
    UWorld* World = Wrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge = World != nullptr
        ? World->GetSubsystem<UEchoesSimulationSubsystem>() : nullptr;
    if (!TestTrue(TEXT("Browser title has a ready scenario"),
            Bridge != nullptr && Bridge->StartPrototypeScenario()))
    {
        return false;
    }
    AEchoesPlayerController* Controller = World != nullptr
        ? World->SpawnActor<AEchoesPlayerController>()
        : nullptr;
    if (!TestNotNull(TEXT("Replay browser controller exists"), Controller))
    {
        Wrapper.ForwardErrorMessages(this);
        return false;
    }
    Controller->PresentTitleScreen();
    TestEqual(TEXT("Browser opens through the normal title action"),
        Controller->BuildShellView().Screen, EEchoesShellScreen::Title);

    Controller->HandleShellAction(EEchoesShellAction::OpenReplayBrowser);
    const FEchoesShellView InitialLoading = Controller->BuildShellView();
    TestTrue(TEXT("Opening the browser publishes loading immediately"),
        Controller->IsReplayBrowserLoading() &&
            InitialLoading.Screen == EEchoesShellScreen::ReplayBrowser &&
            InitialLoading.Body.ToString().Contains(
                TEXT("Checking saved replays")) &&
            Controller->GetReplayBrowserEntries().IsEmpty());

    Controller->HandleShellAction(EEchoesShellAction::ReplayMapFilter, 1);
    TestTrue(TEXT("First rapid filter clears any prior publication"),
        Controller->IsReplayBrowserLoading() &&
            Controller->GetReplayBrowserEntries().IsEmpty());
    Controller->HandleShellAction(EEchoesShellAction::ReplayMapFilter, 2);
    TestTrue(TEXT("Second rapid filter still withholds stale metadata"),
        Controller->IsReplayBrowserLoading() &&
            Controller->GetReplayBrowserEntries().IsEmpty());
    Controller->HandleShellAction(EEchoesShellAction::ReplayMapFilter, 3);
    if (WaitForBrowserScan(
            *this, *Controller,
            TEXT("Newest rapid filter eventually completes")))
    {
        const TArray<FEchoesReplayMetadata>& Entries =
            Controller->GetReplayBrowserEntries();
        TestEqual(TEXT("Only the newest filter generation publishes"),
            Entries.Num(), 1);
        if (Entries.Num() == 1)
        {
            TestEqual(TEXT("Newest Soryn metadata wins"),
                Entries[0].ReplayId, FString(TEXT("browser-soryn")));
            TestEqual(TEXT("Published metadata matches newest map filter"),
                Entries[0].MapId, FString(TEXT("soryn-confluence")));
        }
        TestFalse(TEXT("Older Glass metadata never publishes"),
            Entries.ContainsByPredicate(
                [](const FEchoesReplayMetadata& Entry)
                {
                    return Entry.ReplayId == TEXT("browser-glass");
                }));
        TestFalse(TEXT("Older Crownfall metadata never publishes"),
            Entries.ContainsByPredicate(
                [](const FEchoesReplayMetadata& Entry)
                {
                    return Entry.ReplayId == TEXT("browser-crown");
                }));
    }

    // Follow the offered cyclic action: Soryn -> all maps -> Glass Scar.
    // An arbitrary nonadjacent argument is correctly rejected by the shell.
    Controller->HandleShellAction(EEchoesShellAction::ReplayMapFilter, 0);
    Controller->HandleShellAction(EEchoesShellAction::ReplayMapFilter, 1);
    TestTrue(TEXT("Exit fixture has an active replacement scan"),
        Controller->IsReplayBrowserLoading());
    Controller->HandleShellAction(EEchoesShellAction::ExitReplay);
    const FEchoesShellView TitleBeforeCompletion = Controller->BuildShellView();
    TestEqual(TEXT("Exit returns to title before worker completion"),
        TitleBeforeCompletion.Screen, EEchoesShellScreen::Title);
    Controller->DrainReplayBrowserScan();
    Controller->PollReplayBrowser();
    const FEchoesShellView TitleAfterCompletion = Controller->BuildShellView();
    TestEqual(TEXT("Stale completion cannot replace the title screen"),
        TitleAfterCompletion.Screen, TitleBeforeCompletion.Screen);
    TestEqual(TEXT("Stale completion cannot rewrite title body"),
        TitleAfterCompletion.Body.ToString(),
        TitleBeforeCompletion.Body.ToString());
    TestEqual(TEXT("Stale completion cannot rewrite title status"),
        TitleAfterCompletion.Status.ToString(),
        TitleBeforeCompletion.Status.ToString());
    TestTrue(TEXT("Exit discards browser metadata"),
        Controller->GetReplayBrowserEntries().IsEmpty());

    Controller->HandleShellAction(EEchoesShellAction::OpenReplayBrowser);
    if (WaitForBrowserScan(
            *this, *Controller,
            TEXT("Re-entered browser completes a fresh scan")))
    {
        const TArray<FEchoesReplayMetadata>& Reentered =
            Controller->GetReplayBrowserEntries();
        TestTrue(TEXT("Re-entry publishes the retained Glass filter"),
            Reentered.Num() == 1 &&
                Reentered[0].ReplayId == TEXT("browser-glass"));
    }

    Controller->HandleShellAction(EEchoesShellAction::ReplayMapFilter, 2);
    TestTrue(TEXT("Shutdown fixture owns an active scan"),
        Controller->IsReplayBrowserLoading());
    TestTrue(TEXT("Controller teardown safely retires its captured-value worker"),
        Controller->Destroy());

    Bridge->StopPrototypeScenario();
    Wrapper.ForwardErrorMessages(this);
    return !HasAnyErrors() && !Wrapper.HasFailed();
}

#endif
