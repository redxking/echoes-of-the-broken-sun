#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesNarrativeSubsystem.h"
#include "EchoesGameInstance.h"
#include "Engine/GameInstance.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesNarrativePackTest,
    "Echoes.Runtime.Narrative.PackBinding",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesNarrativePackTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    UGameInstance* GameInstance = NewObject<UGameInstance>(GEngine);
    GameInstance->InitializeStandalone();
    UEchoesNarrativeSubsystem* Narrative =
        GameInstance->GetSubsystem<UEchoesNarrativeSubsystem>();
    if (!TestNotNull(TEXT("Narrative subsystem is available"), Narrative))
    {
        GameInstance->Shutdown();
        return false;
    }

    TestTrue(
        *FString::Printf(
            TEXT("The narrative pack loads and digest-verifies (error=%s)"),
            *Narrative->GetLoadError()),
        Narrative->IsReady());
    TestEqual(TEXT("Fifteen authored operations are bound"),
              Narrative->GetOperationCount(),
              15);
    TestEqual(TEXT("The full authored line count is carried"),
              Narrative->GetTotalLineCount(),
              308);
    TestEqual(TEXT("The pack digest is a full SHA-256"),
              Narrative->GetPackDigest().Len(),
              64);
    TestEqual(TEXT("The demo tutorial and annunciator line count is carried"),
              Narrative->GetDemoLineCount(),
              55);
    TestTrue(TEXT("Tutorial signal resolves Mara Vey survey line"),
             Narrative->GetLinesForSignal(
                 EEchoesOperationMode::Skirmish,
                 TEXT("tutorial_lesson_opened:survey")).Num() > 0);

    // Every campaign operation with an authored contract binds completely.
    const EEchoesOperationMode Authored[] = {
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
        EEchoesOperationMode::CampaignTheBrokenSun,
    };
    const TCHAR* CommitStatuses[] = {
        TEXT("Added"),
        TEXT("AlreadyRecorded"),
        TEXT("ReplayConflict"),
        TEXT("StorageFailure"),
    };
    for (const EEchoesOperationMode Operation : Authored)
    {
        const FString Key =
            UEchoesNarrativeSubsystem::OperationPackKey(Operation);
        TestTrue(
            *FString::Printf(TEXT("%s has a bound contract"), *Key),
            Narrative->HasOperation(Operation));
        TestFalse(
            *FString::Printf(TEXT("%s carries a title"), *Key),
            Narrative->GetTitle(Operation).IsEmpty());
        TestFalse(
            *FString::Printf(TEXT("%s carries a briefing"), *Key),
            Narrative->GetBriefing(Operation).IsEmpty());
        TestTrue(
            *FString::Printf(TEXT("%s carries objectives"), *Key),
            Narrative->GetObjectives(Operation).Num() >= 2);
        TestFalse(
            *FString::Printf(TEXT("%s carries retry copy"), *Key),
            Narrative->GetRetryCopy(Operation).IsEmpty());
        for (const TCHAR* Status : CommitStatuses)
        {
            TestFalse(
                *FString::Printf(
                    TEXT("%s carries %s result copy"), *Key, Status),
                Narrative->GetResultCopy(Operation, Status).IsEmpty());
        }
        TestFalse(
            *FString::Printf(TEXT("%s carries a generic failure"), *Key),
            Narrative->GetFailureCondition(Operation, TEXT("generic"))
                .IsEmpty());
        const TArray<FEchoesNarrativeLine>* Lines =
            Narrative->GetLines(Operation);
        if (!TestNotNull(
                *FString::Printf(TEXT("%s carries lines"), *Key), Lines))
        {
            continue;
        }
        TestTrue(
            *FString::Printf(TEXT("%s carries at least 15 lines"), *Key),
            Lines->Num() >= 15);
        // Every line's signal is one of the stable source-signal shapes.
        for (const FEchoesNarrativeLine& Line : *Lines)
        {
            const bool bKnownShape =
                Line.Signal.StartsWith(TEXT("phase_entered:")) ||
                Line.Signal.StartsWith(TEXT("operation_ready:")) ||
                Line.Signal == TEXT("campaign_commit_status_presented") ||
                Line.Signal == TEXT("player_requested_mission_retry");
            if (!bKnownShape)
            {
                AddError(FString::Printf(
                    TEXT("%s line %s carries unknown signal %s"),
                    *Key,
                    *Line.Id,
                    *Line.Signal));
            }
        }
        // The operation-start signal binds at least one line and names this
        // operation's own mode.
        const FString StartPrefix =
            FString::Printf(TEXT("operation_ready:%s:"), *Key);
        int32 StartLines = 0;
        for (const FEchoesNarrativeLine& Line : *Lines)
        {
            if (Line.Signal.StartsWith(StartPrefix))
            {
                ++StartLines;
            }
        }
        TestTrue(
            *FString::Printf(
                TEXT("%s binds opening lines to its own start signal"), *Key),
            StartLines >= 1);
    }

    // M01 withdrawal must never concatenate the three mutually exclusive
    // choices. Each exact signal owns its trio; the generic signal is common.
    for (const TCHAR* Choice : { TEXT("Harvest"), TEXT("Preserve"), TEXT("Reshape") })
    {
        const FString Signal = FString::Printf(TEXT("phase_entered:Withdraw:%s"), Choice);
        const TArray<FEchoesNarrativeLine> Branch = Narrative->GetLinesForSignal(
            EEchoesOperationMode::CampaignPrologue, Signal);
        TestEqual(*FString::Printf(TEXT("%s binds exactly three lines"), Choice), Branch.Num(), 3);
        const TCHAR* Speakers[] = { TEXT("mara"), TEXT("oruun"), TEXT("talar") };
        for (int32 Index = 0; Index < Branch.Num() && Index < 3; ++Index)
        {
            TestEqual(TEXT("The branch contains only its own authored IDs in order"),
                Branch[Index].Id, FString::Printf(TEXT("nar_m01_line_%s_%s_001"),
                    Speakers[Index], *FString(Choice).ToLower()));
        }
        Narrative->ClearSubtitleQueue();
        Narrative->EnqueueSignal(EEchoesOperationMode::CampaignPrologue, Signal, 10.0);
        TestEqual(TEXT("Enqueueing one choice cannot enqueue other choices"),
            Narrative->GetQueuedLineCountForTest(), 3);
    }
    const TArray<FEchoesNarrativeLine> Common = Narrative->GetLinesForSignal(
        EEchoesOperationMode::CampaignPrologue, TEXT("phase_entered:Withdraw"));
    TestEqual(TEXT("Generic withdrawal binds only its three common lines"), Common.Num(), 3);
    const TCHAR* CommonIds[] = { TEXT("nar_m01_line_mara_004"),
        TEXT("nar_m01_line_talar_004"), TEXT("nar_m01_line_oruun_004") };
    for (int32 Index = 0; Index < Common.Num() && Index < 3; ++Index)
        TestEqual(TEXT("Generic withdrawal contains no choice-specific line"),
            Common[Index].Id, FString(CommonIds[Index]));

    // --- The subtitle queue consumes lines in authored order --------------

    Narrative->ClearSubtitleQueue();
    Narrative->EnqueueOperationStart(
        EEchoesOperationMode::CampaignSevenAccounts, 100.0);
    TestEqual(TEXT("Opening lines enqueue for the deployed operation"),
              Narrative->GetQueuedLineCountForTest(),
              5);
    FString Speaker;
    FString Text;
    TestTrue(TEXT("The first authored line owns the lane immediately"),
             Narrative->GetActiveSubtitle(100.1, Speaker, Text));
    TestEqual(TEXT("The opening line speaks in Oruun's voice"),
              Speaker,
              FString(TEXT("Oruun-of-Seven-Stones")));
    const double FirstDuration =
        UEchoesNarrativeSubsystem::SubtitleDurationSeconds(Text);
    TestTrue(TEXT("Line durations scale with length within bounds"),
             FirstDuration >= 3.0 && FirstDuration <= 9.0);
    FString SecondText;
    TestTrue(TEXT("The lane advances after the first line's duration"),
             Narrative->GetActiveSubtitle(
                 100.1 + FirstDuration + 0.1, Speaker, SecondText));
    TestTrue(TEXT("The second line differs from the first"),
             SecondText != Text);
    Narrative->ClearSubtitleQueue();
    TestFalse(TEXT("A cleared queue leaves the lane silent"),
              Narrative->GetActiveSubtitle(200.0, Speaker, Text));
    Narrative->EnqueueSignal(
        EEchoesOperationMode::CampaignSevenAccounts,
        TEXT("phase_entered:Complete"),
        300.0);
    TestEqual(TEXT("Completion lines enqueue by exact signal"),
              Narrative->GetQueuedLineCountForTest(),
              3);
    Narrative->EnqueueSignal(
        EEchoesOperationMode::Skirmish,
        TEXT("phase_entered:Complete"),
        300.0);
    TestEqual(TEXT("Skirmish enqueues nothing"),
              Narrative->GetQueuedLineCountForTest(),
              3);
    Narrative->ClearSubtitleQueue();
    Narrative->EnqueueFailureLine(
        EEchoesOperationMode::CampaignSevenAccounts, TEXT("generic"), 400.0);
    TestEqual(TEXT("A failure enqueues exactly one generic line"),
              Narrative->GetQueuedLineCountForTest(),
              1);
    Narrative->EnqueueFailureLine(
        EEchoesOperationMode::CampaignSevenAccounts,
        TEXT("nonexistent_reason"),
        400.0);
    TestEqual(TEXT("An unknown reason falls back to the generic line"),
              Narrative->GetQueuedLineCountForTest(),
              2);
    Narrative->ClearSubtitleQueue();

    // Every reason code the runtime derivation can emit for the bound
    // missions must resolve to its own authored failure line.
    struct FReasonBinding
    {
        EEchoesOperationMode Operation;
        std::initializer_list<const TCHAR*> Reasons;
    };
    const FReasonBinding ReasonBindings[] = {
        {EEchoesOperationMode::CampaignPrologue,
         {TEXT("local_core_lost"), TEXT("archive_carrier_lost"),
          TEXT("future_well_lost"), TEXT("terminal_match_outcome"),
          TEXT("generic")}},
        {EEchoesOperationMode::CampaignSevenAccounts,
         {TEXT("local_core_lost"), TEXT("memory_bearer_lost"),
          TEXT("waystone_lost"), TEXT("terminal_match_outcome"),
          TEXT("generic")}},
        {EEchoesOperationMode::CampaignCityReserve,
         {TEXT("local_core_lost"), TEXT("district_structure_lost"),
          TEXT("terminal_match_outcome"), TEXT("generic")}},
        {EEchoesOperationMode::CampaignUnburiedRoad,
         {TEXT("local_core_lost"), TEXT("memory_bearer_lost"),
          TEXT("waystone_lost"), TEXT("terminal_match_outcome"),
          TEXT("generic")}},
        {EEchoesOperationMode::CampaignTermsOfContinuance,
         {TEXT("local_core_lost"), TEXT("meridian_relay_lost"),
          TEXT("kharuun_spine_lost"), TEXT("witness_lost"),
          TEXT("continuance_window_compromised"),
          TEXT("terminal_match_outcome"), TEXT("generic")}},
        {EEchoesOperationMode::CampaignNamesWithoutBirths,
         {TEXT("local_core_lost"), TEXT("talar_lost"), TEXT("archive_lost"),
          TEXT("civilian_proxy_lost"), TEXT("terminal_match_outcome"),
          TEXT("generic")}},
        {EEchoesOperationMode::CampaignShapeOfSilence,
         {TEXT("local_core_lost"), TEXT("oruun_lost"), TEXT("waystone_lost"),
          TEXT("memory_witness_lost"), TEXT("terminal_match_outcome"),
          TEXT("generic")}},
        {EEchoesOperationMode::CampaignShapeBesideUs,
         {TEXT("local_core_lost"), TEXT("talar_lost"),
          TEXT("state_witness_lost"), TEXT("terminal_match_outcome"),
          TEXT("generic")}},
        {EEchoesOperationMode::CampaignReserveAuthority,
         {TEXT("local_core_lost"), TEXT("mara_lost"),
          TEXT("district_structure_lost"), TEXT("terminal_match_outcome"),
          TEXT("generic")}},
        {EEchoesOperationMode::CampaignChoirAtLumeReach,
         {TEXT("local_core_lost"), TEXT("oruun_lost"), TEXT("waystone_lost"),
          TEXT("future_well_lost"), TEXT("reshape_window_expired"),
          TEXT("terminal_match_outcome"), TEXT("generic")}},
        {EEchoesOperationMode::CampaignNoNeutralLedger,
         {TEXT("local_core_lost"), TEXT("oruun_lost"), TEXT("waystone_lost"),
          TEXT("ledger_witness_lost"), TEXT("future_well_lost"),
          TEXT("public_interface_lost"),
          TEXT("conflicting_protocol_applied"),
          TEXT("reshape_window_expired"), TEXT("terminal_match_outcome"),
          TEXT("generic")}},
        {EEchoesOperationMode::CampaignFutureThatWon,
         {TEXT("local_core_lost"), TEXT("oruun_lost"), TEXT("verifier_lost"),
          TEXT("future_well_lost"), TEXT("public_interface_lost"),
          TEXT("conflicting_protocol_bound"), TEXT("terminal_match_outcome"),
          TEXT("generic")}},
        {EEchoesOperationMode::CampaignAssemblyOfTheMissing,
         {TEXT("local_core_lost"), TEXT("oruun_lost"), TEXT("verifier_lost"),
          TEXT("public_interface_lost"), TEXT("terminal_match_outcome"),
          TEXT("generic")}},
        {EEchoesOperationMode::CampaignSeveralVoicesOneCommand,
         {TEXT("local_core_lost"), TEXT("protected_voice_lost"),
          TEXT("neme_lost"), TEXT("research_loom_lost"),
          TEXT("crisis_contract_breached"), TEXT("terminal_match_outcome"),
          TEXT("generic")}},
        {EEchoesOperationMode::CampaignTheBrokenSun,
         {TEXT("local_core_lost"), TEXT("protected_witness_lost"),
          TEXT("command_force_lost"), TEXT("resolution_contract_breached"),
          TEXT("terminal_match_outcome"), TEXT("generic")}},
    };
    for (const FReasonBinding& Binding : ReasonBindings)
    {
        TSet<FString> BoundLineIds;
        int32 ReasonCount = 0;
        for (const TCHAR* Reason : Binding.Reasons)
        {
            Narrative->ClearSubtitleQueue();
            Narrative->EnqueueFailureLine(Binding.Operation, Reason, 500.0);
            TestEqual(
                *FString::Printf(
                    TEXT("%s failure reason %s binds one authored line"),
                    *UEchoesNarrativeSubsystem::OperationPackKey(
                        Binding.Operation),
                    Reason),
                Narrative->GetQueuedLineCountForTest(),
                1);
            BoundLineIds.Add(Narrative->GetLastQueuedLineIdForTest());
            ++ReasonCount;
        }
        // Distinct lines per reason: a silent generic fallback would
        // collapse two reasons onto one line id and fail this count.
        TestEqual(
            *FString::Printf(
                TEXT("%s binds a distinct line per authored reason"),
                *UEchoesNarrativeSubsystem::OperationPackKey(
                    Binding.Operation)),
            BoundLineIds.Num(),
            ReasonCount);
    }
    Narrative->ClearSubtitleQueue();

    // Skirmish deliberately has no narrative contract.
    TestFalse(TEXT("Skirmish has no narrative contract"),
              Narrative->HasOperation(EEchoesOperationMode::Skirmish));

    GameInstance->Shutdown();
    return true;
}

#endif
