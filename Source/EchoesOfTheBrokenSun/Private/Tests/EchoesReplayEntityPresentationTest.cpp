// Author and owner: Angelis Pseftis
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesEntityView.h"
#include "EchoesMatchReplay.h"
#include "EchoesNetworkSession.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesTestSaveEnvironment.h"
#include "Engine/World.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "Tests/AutomationCommon.h"

namespace
{
template <typename Digest>
FString ReplayPresentationDigestHex(const Digest& Value)
{
    FString Result;
    Result.Reserve(static_cast<int32>(Value.size() * 2));
    for (const uint8 Byte : Value)
    {
        Result += FString::Printf(TEXT("%02x"), Byte);
    }
    return Result;
}

bool SavePresentationReplay(
    FAutomationTestBase& Test,
    const UEchoesSimulationSubsystem& Bridge,
    const echoes::sim::ReplayRecord& Replay,
    EEchoesReplayOperationType OperationType,
    const FString& OperationId,
    const FString& MapId,
    const FString& Directory,
    FString& OutPath)
{
    const echoes::sim::net::CompatibilityManifest Compatibility =
        echoes::network::BuildCompatibilityManifest(Bridge.GetSimulation());
    FEchoesReplayMetadata Metadata;
    Metadata.ReplayId = FString::Printf(
        TEXT("entity-presentation-%s"),
        *FGuid::NewGuid().ToString(EGuidFormats::Digits));
    Metadata.MapId = MapId;
    Metadata.OperationId = OperationId;
    Metadata.BuildIdentity = ReplayPresentationDigestHex(
        Compatibility.buildIdSha256);
    Metadata.RulesIdentity = ReplayPresentationDigestHex(
        Compatibility.rulesPackSha256);
    Metadata.RecordedUtc = FDateTime(2026, 9, 5, 12, 0, 0);
    Metadata.OperationType = OperationType;
    Metadata.bOperationCompleted = true;
    if (OperationType == EEchoesReplayOperationType::Campaign)
    {
        Metadata.OperationResult =
            EEchoesReplayOperationResult::CampaignSuccess;
        Metadata.OutcomeCause =
            EEchoesReplayOutcomeCause::CampaignObjectivesComplete;
        Metadata.OutcomeReasonId = TEXT("presentation_fixture_complete");
    }

    FString Error;
    FEchoesReplayEnvelope Envelope;
    if (!Test.TestTrue(
            TEXT("Cross-operation replay finalizes"),
            FEchoesMatchReplayStore::FinalizeEnvelope(
                Metadata, Replay, Envelope, Error)))
    {
        Test.AddError(Error);
        return false;
    }
    if (!Test.TestTrue(
            TEXT("Cross-operation replay saves"),
            FEchoesMatchReplayStore::SaveAtomic(
                Directory, Envelope, OutPath, Error)))
    {
        Test.AddError(Error);
        return false;
    }
    return true;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesReplayEntityPresentationTest,
    "Echoes.Runtime.Replay.DetachedEntityPresentation",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesReplayEntityPresentationTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    FEchoesScopedTestSaveEnvironment Saves(*this);
    if (!Saves.IsReady())
    {
        return false;
    }
    FTestWorldWrapper Wrapper;
    if (!Wrapper.CreateTestWorld(EWorldType::Game))
    {
        Wrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the replay presentation test world."));
        return false;
    }
    UWorld* World = Wrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge = World != nullptr
        ? World->GetSubsystem<UEchoesSimulationSubsystem>()
        : nullptr;
    FString Feedback;
    if (!TestNotNull(TEXT("Replay presentation world owns bridge"), Bridge) ||
        !TestTrue(
            TEXT("Replay presentation scenario starts"),
            Bridge != nullptr && Bridge->StartPrototypeScenario()) ||
        !TestTrue(
            TEXT("M01 source scenario starts"),
            Bridge->SelectOperationMode(
                EEchoesOperationMode::CampaignPrologue, Feedback)))
    {
        AddError(Feedback);
        Wrapper.ForwardErrorMessages(this);
        return false;
    }

    const echoes::sim::EntityId M01CarrierId = Bridge->GetArchiveCarrierId();
    std::string ExportError;
    const echoes::sim::ReplayRecord M01Replay =
        Bridge->GetSimulation()->ExportReplay(&ExportError);
    if (!TestTrue(TEXT("M01 replay source exports"), M01Replay.version != 0) ||
        !TestEqual(
            TEXT("M01 canonical replay carrier identity is stable"),
            M01CarrierId,
            static_cast<echoes::sim::EntityId>(11)))
    {
        AddError(UTF8_TO_TCHAR(ExportError.c_str()));
        Bridge->StopPrototypeScenario();
        return false;
    }

    if (!TestTrue(
            TEXT("Skirmish source scenario starts"),
            Bridge->SelectOperationMode(
                EEchoesOperationMode::Skirmish, Feedback)) ||
        !TestTrue(
            TEXT("Skirmish reaches a normal terminal result for archival"),
            Bridge->ConcedeOfflineMatch(Feedback)))
    {
        AddError(Feedback);
        Bridge->StopPrototypeScenario();
        return false;
    }
    const echoes::sim::ReplayRecord SkirmishReplay =
        Bridge->GetSimulation()->ExportReplay(&ExportError);
    if (!TestTrue(
            TEXT("Skirmish replay source exports"),
            SkirmishReplay.version != 0))
    {
        AddError(UTF8_TO_TCHAR(ExportError.c_str()));
        Bridge->StopPrototypeScenario();
        return false;
    }

    const FString ReplayDirectory = FPaths::Combine(
        FEchoesCampaignProgressStore::GetSaveGameDirectory(),
        TEXT("ReplayEntityPresentation"));
    FString M01Path;
    FString SkirmishPath;
    if (!SavePresentationReplay(
            *this,
            *Bridge,
            M01Replay,
            EEchoesReplayOperationType::Campaign,
            TEXT("m01-what-the-ledger-keeps"),
            TEXT("glass-scar-evacuation-margin"),
            ReplayDirectory,
            M01Path) ||
        !SavePresentationReplay(
            *this,
            *Bridge,
            SkirmishReplay,
            EEchoesReplayOperationType::Skirmish,
            TEXT("skirmish"),
            TEXT("glass-scar"),
            ReplayDirectory,
            SkirmishPath))
    {
        Bridge->StopPrototypeScenario();
        return false;
    }

    // Live authority is still the terminal skirmish. The replay presentation
    // must nevertheless recover M01's named carrier from the replay snapshot.
    if (TestTrue(TEXT("M01 replay opens over live skirmish"),
                 Bridge->BeginReplay(M01Path, Feedback)))
    {
        const AEchoesEntityView* CarrierView =
            Bridge->FindEntityView(M01CarrierId);
        if (TestNotNull(TEXT("M01 replay presents its carrier"), CarrierView))
        {
            TestEqual(
                TEXT("M01 replay names its own carrier"),
                CarrierView->GetDisplayName(),
                FString(TEXT("Archive Carrier")));
        }
        Bridge->EndReplay();
    }
    else
    {
        AddError(Feedback);
    }

    if (!TestTrue(
            TEXT("Live M01 restarts for inverse identity check"),
            Bridge->SelectOperationMode(
                EEchoesOperationMode::CampaignPrologue, Feedback)))
    {
        AddError(Feedback);
        Bridge->StopPrototypeScenario();
        return false;
    }
    const echoes::sim::EntityId LiveCarrierId = Bridge->GetArchiveCarrierId();
    if (TestTrue(TEXT("Skirmish replay opens over live M01"),
                 Bridge->BeginReplay(SkirmishPath, Feedback)))
    {
        const AEchoesEntityView* SameIdSkirmishView =
            Bridge->FindEntityView(LiveCarrierId);
        if (TestNotNull(
                TEXT("Skirmish replay presents the live carrier numeric ID"),
                SameIdSkirmishView))
        {
            TestNotEqual(
                TEXT("Live M01 ID does not rename a skirmish replay entity"),
                SameIdSkirmishView->GetDisplayName(),
                FString(TEXT("Archive Carrier")));
        }
        Bridge->EndReplay();
    }
    else
    {
        AddError(Feedback);
    }

    Bridge->StopPrototypeScenario();
    Wrapper.ForwardErrorMessages(this);
    AddInfo(TEXT("Source-authored cross-operation replay fixture; no rendered visual acceptance is claimed."));
    return !HasAnyErrors() && !Wrapper.HasFailed();
}

#endif
