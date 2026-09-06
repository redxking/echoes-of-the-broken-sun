// Author and owner: Angelis Pseftis
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesEntityView.h"
#include "EchoesMatchReplay.h"
#include "EchoesNetworkSession.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesSkirmishSetup.h"
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

    echoes::sim::Simulation* M01Simulation =
        const_cast<echoes::sim::Simulation*>(Bridge->GetSimulation());
    if (!TestNotNull(TEXT("M01 replay fixture owns simulation"), M01Simulation))
    {
        Bridge->StopPrototypeScenario();
        return false;
    }
    const echoes::sim::EntityId M01CarrierId = Bridge->GetArchiveCarrierId();
    const echoes::sim::Vec2 M01CarrierPosition =
        echoes::sim::Vec2::FromTiles(15, 6);
    const echoes::sim::Vec2 LaterScoutPosition =
        echoes::sim::Vec2::FromTiles(18, 16);
    const echoes::sim::EntityId LaterScoutId = M01Simulation->SpawnEntity(
        UEchoesSimulationSubsystem::LocalPlayerId,
        echoes::sim::Faction::MeridianCompact,
        echoes::sim::EntityType::ScoutUnit,
        LaterScoutPosition);
    if (!TestTrue(
            TEXT("Replay identity fixture adds a distinct local Relay Skiff"),
            LaterScoutId != 0))
    {
        Bridge->StopPrototypeScenario();
        return false;
    }
    M01Simulation->CaptureReplayBaseline();
    const echoes::sim::Entity* M01Carrier =
        M01Simulation->FindEntity(M01CarrierId);
    const echoes::sim::Entity* LaterScout =
        M01Simulation->FindEntity(LaterScoutId);
    std::string ExportError;
    const echoes::sim::ReplayRecord M01Replay =
        M01Simulation->ExportReplay(&ExportError);
    if (!TestTrue(TEXT("M01 replay source exports"), M01Replay.version != 0) ||
        !TestTrue(
            TEXT("M01 carrier is the authored local Relay Skiff"),
            M01Carrier != nullptr &&
            M01Carrier->owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            M01Carrier->faction == echoes::sim::Faction::MeridianCompact &&
            M01Carrier->type == echoes::sim::EntityType::ScoutUnit &&
            M01Carrier->position == M01CarrierPosition) ||
        !TestTrue(
            TEXT("Additional Relay Skiff retains its separate fixture identity"),
            LaterScoutId != M01CarrierId && LaterScout != nullptr &&
            LaterScout->owner ==
                UEchoesSimulationSubsystem::LocalPlayerId &&
            LaterScout->faction ==
                echoes::sim::Faction::MeridianCompact &&
            LaterScout->type ==
                echoes::sim::EntityType::ScoutUnit &&
            LaterScout->position == LaterScoutPosition))
    {
        AddError(UTF8_TO_TCHAR(ExportError.c_str()));
        Bridge->StopPrototypeScenario();
        return false;
    }

    if (!TestTrue(
            TEXT("Skirmish source scenario starts"),
            Bridge->SelectOperationMode(
                EEchoesOperationMode::Skirmish, Feedback)))
    {
        AddError(Feedback);
        Bridge->StopPrototypeScenario();
        return false;
    }
    const FEchoesSkirmishSetup SkirmishSetup =
        Bridge->GetActiveSkirmishSetup();
    const TArray<FIntPoint> SkirmishLocalSpawns =
        FEchoesSkirmishSetupModel::LocalSpawnTiles(
            SkirmishSetup.MapPreset);
    echoes::sim::EntityId SkirmishScoutId = 0;
    for (const echoes::sim::Entity& Entity :
         Bridge->GetSimulation()->Entities())
    {
        if (Entity.owner != UEchoesSimulationSubsystem::LocalPlayerId ||
            Entity.faction != SkirmishSetup.LocalFaction ||
            Entity.type != echoes::sim::EntityType::ScoutUnit)
        {
            continue;
        }
        for (const FIntPoint& AuthoredTile : SkirmishLocalSpawns)
        {
            if (Entity.position == echoes::sim::Vec2::FromTiles(
                    AuthoredTile.X, AuthoredTile.Y))
            {
                SkirmishScoutId = Entity.id;
                break;
            }
        }
    }
    if (!TestTrue(
            TEXT("Skirmish replay fixture resolves its authored local scout"),
            SkirmishScoutId != 0) ||
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
        const AEchoesEntityView* LaterScoutView =
            Bridge->FindEntityView(LaterScoutId);
        if (TestNotNull(
                TEXT("M01 replay presents the post-objective Relay Skiff"),
                LaterScoutView))
        {
            TestNotEqual(
                TEXT("A later Relay Skiff does not inherit the carrier role"),
                LaterScoutView->GetDisplayName(),
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
    if (TestTrue(TEXT("Skirmish replay opens over live M01"),
                 Bridge->BeginReplay(SkirmishPath, Feedback)))
    {
        const AEchoesEntityView* SkirmishScoutView =
            Bridge->FindEntityView(SkirmishScoutId);
        if (TestNotNull(
                TEXT("Skirmish replay presents its authored local scout"),
                SkirmishScoutView))
        {
            TestTrue(
                TEXT("Skirmish replay retains the scout owner, faction and role"),
                SkirmishScoutView->GetOwnerPlayerId() ==
                    UEchoesSimulationSubsystem::LocalPlayerId &&
                SkirmishScoutView->GetEntityFaction() ==
                    SkirmishSetup.LocalFaction &&
                SkirmishScoutView->GetEntityType() ==
                    echoes::sim::EntityType::ScoutUnit);
            TestNotEqual(
                TEXT("A skirmish scout does not inherit M01's carrier role"),
                SkirmishScoutView->GetDisplayName(),
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
