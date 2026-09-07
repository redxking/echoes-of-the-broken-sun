#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesNarrativeSubsystem.h"
#include "EchoesGameInstance.h"
#include "Engine/GameInstance.h"
#include "GameFramework/InputSettings.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesNarrativePackTest,
    "Echoes.Runtime.Narrative.PackBinding",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesNarrativePackTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    UInputSettings* InputSettings = GetMutableDefault<UInputSettings>();
    if (!TestNotNull(TEXT("Input settings prerequisite exists"), InputSettings)) return false;
    TArray<FInputActionKeyMapping> PriorRecenter;
    InputSettings->GetActionMappingByName(TEXT("SnapKeyboardTargetToSelection"), PriorRecenter);
    for (const auto& Mapping : PriorRecenter) InputSettings->RemoveActionMapping(Mapping, false);
    const FInputActionKeyMapping TestMapping(TEXT("SnapKeyboardTargetToSelection"), EKeys::K, true, true);
    InputSettings->AddActionMapping(TestMapping, false);
    const FString ResolvedControl = UEchoesNarrativeSubsystem::ResolveInputTokens(TEXT("{recenter_key} brings you home."));
#if PLATFORM_MAC
    const TCHAR* ExpectedPhysicalChord = TEXT("Command+Shift+K");
#else
    const TCHAR* ExpectedPhysicalChord = TEXT("Ctrl+Shift+K");
#endif
    TestTrue(TEXT("Subtitle control tokens use the current physical platform key binding"),
        ResolvedControl.Contains(ExpectedPhysicalChord) && !ResolvedControl.Contains(TEXT("{")));
    TestTrue(TEXT("Unknown authored control tokens refuse display"),
        UEchoesNarrativeSubsystem::ResolveInputTokens(TEXT("{unknown_key}" )).IsEmpty());
    InputSettings->RemoveActionMapping(TestMapping, false);
    for (const auto& Mapping : PriorRecenter) InputSettings->AddActionMapping(Mapping, false);


    // Each prompt must follow runtime remapping and must not invent a default
    // when the player has deliberately left an action unassigned. Never save
    // these fixture mappings to the user's configuration.
    const TPair<FName, FString> PromptActions[] = {
        {TEXT("Select"), TEXT("{select_key}")},
        {TEXT("CameraZoomIn"), TEXT("{zoom_in_key}")},
        {TEXT("CameraZoomOut"), TEXT("{zoom_out_key}")},
        {TEXT("ArmControlGroupAssignment"), TEXT("{assign_group_key}")},
        {TEXT("RecallControlGroup1"), TEXT("{recall_group_key}")}};
    for (const auto& Prompt : PromptActions)
    {
        TArray<FInputActionKeyMapping> Prior;
        InputSettings->GetActionMappingByName(Prompt.Key, Prior);
        for (const auto& Mapping : Prior) InputSettings->RemoveActionMapping(Mapping, false);
        TestEqual(TEXT("Unassigned tutorial action reports the missing control"),
            UEchoesNarrativeSubsystem::ResolveInputTokens(Prompt.Value),
            NSLOCTEXT("EchoesNarrative", "ControlUnassigned", "unassigned control").ToString());
        const FInputActionKeyMapping Remapped(Prompt.Key, EKeys::K, true, true);
        InputSettings->AddActionMapping(Remapped, false);
        TestEqual(TEXT("Tutorial prompt uses the remapped physical chord"),
            UEchoesNarrativeSubsystem::ResolveInputTokens(Prompt.Value), FString(ExpectedPhysicalChord));
        InputSettings->RemoveActionMapping(Remapped, false);
        for (const auto& Mapping : Prior) InputSettings->AddActionMapping(Mapping, false);
    }
    TArray<FInputAxisKeyMapping> PriorForward, PriorRight;
    InputSettings->GetAxisMappingByName(TEXT("CameraForward"), PriorForward);
    InputSettings->GetAxisMappingByName(TEXT("CameraRight"), PriorRight);
    for (const auto& Mapping : PriorForward) InputSettings->RemoveAxisMapping(Mapping, false);
    for (const auto& Mapping : PriorRight) InputSettings->RemoveAxisMapping(Mapping, false);
    const FInputAxisKeyMapping PanMapping(TEXT("CameraForward"), EKeys::Up, 1.0f);
    InputSettings->AddAxisMapping(PanMapping, false);
    TestEqual(TEXT("Tutorial pan prompt follows the current axis mapping"),
        UEchoesNarrativeSubsystem::ResolveInputTokens(TEXT("{pan_keys}")), EKeys::Up.GetDisplayName().ToString());
    InputSettings->RemoveAxisMapping(PanMapping, false);
    for (const auto& Mapping : PriorForward) InputSettings->AddAxisMapping(Mapping, false);
    for (const auto& Mapping : PriorRight) InputSettings->AddAxisMapping(Mapping, false);

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

    Narrative->ClearSubtitleQueue();
    Narrative->EnqueueSignal(EEchoesOperationMode::Skirmish,
        TEXT("tutorial_lesson_opened:survey"), 10.0);
    double SurveyTime = 10.0;
    bool bObservedResolvedRecenter = false;
    const int32 SurveyLines = Narrative->GetQueuedLineCountForTest();
    for (int32 Index = 0; Index < SurveyLines; ++Index)
    {
        FString SurveySpeaker, SurveyText;
        TestTrue(TEXT("Authored Survey line reaches the actual subtitle lane"),
            Narrative->GetActiveSubtitle(SurveyTime, SurveySpeaker, SurveyText));
        TestFalse(TEXT("Subtitle lane never publishes an unresolved control token"),
            SurveyText.Contains(TEXT("{")) || SurveyText.Contains(TEXT("}")));
        bObservedResolvedRecenter |= SurveyText.Contains(TEXT("with your Anchor selected"));
        SurveyTime += UEchoesNarrativeSubsystem::SubtitleDurationSeconds(SurveyText) + 0.1;
    }
    TestTrue(TEXT("Survey publishes the recenter control precondition"), bObservedResolvedRecenter);
    Narrative->SetSubtitlePlaybackPaused(true, SurveyTime);
    Narrative->ClearSubtitleQueue();
    TestFalse(TEXT("Changing scenario cannot retain the previous paused subtitle clock"),
        Narrative->IsSubtitlePlaybackPaused());

    TSharedRef<FJsonObject> VoiceManifest = MakeShared<FJsonObject>();
    VoiceManifest->SetNumberField(TEXT("schema_version"), 1);
    VoiceManifest->SetStringField(TEXT("pack_sha256"), Narrative->GetPackDigest());
    TArray<TSharedPtr<FJsonValue>> VoiceRows;
    const auto* M01Lines = Narrative->GetLines(EEchoesOperationMode::CampaignPrologue);
    if (!TestNotNull(TEXT("M01 voice validation has canonical source"), M01Lines))
    {
        GameInstance->Shutdown();
        return false;
    }
    for (const auto& Line : *M01Lines)
    {
        auto Row = MakeShared<FJsonObject>();
        Row->SetStringField(TEXT("line_id"), Line.Id);
        Row->SetStringField(TEXT("speaker"), Line.Speaker);
        Row->SetStringField(TEXT("text"), Line.Text);
        Row->SetStringField(TEXT("runtime_signal"), Line.Signal);
        const FString Hook = Line.Id.Replace(TEXT("nar_m01_line_"), TEXT("aud_m01_vo_"));
        Row->SetStringField(TEXT("asset_path"), FString::Printf(TEXT("/Game/Audio/Voice/%s.%s"), *Hook, *Hook));
        Row->SetNumberField(TEXT("duration_seconds"), 4.0);
        VoiceRows.Add(MakeShared<FJsonValueObject>(Row));
    }
    VoiceManifest->SetArrayField(TEXT("lines"), VoiceRows);
    TMap<FString, FString> ValidatedPaths;
    const auto ValidateManifest = [&]()
    {
        FString Json;
        FJsonSerializer::Serialize(VoiceManifest, TJsonWriterFactory<>::Create(&Json));
        return Narrative->ValidateVoiceBindings(Json, ValidatedPaths);
    };
    TestTrue(TEXT("Current exact canonical voice mapping validates before loading assets"), ValidateManifest());
    TestEqual(TEXT("All M01 mappings validate together"), ValidatedPaths.Num(), M01Lines->Num());
    VoiceManifest->SetStringField(TEXT("pack_sha256"), TEXT("stale"));
    TestFalse(TEXT("A stale compiled pack refuses all voice mappings"), ValidateManifest());
    TestTrue(TEXT("Refused voice mapping exposes no partial binding"), ValidatedPaths.IsEmpty());
    VoiceManifest->SetStringField(TEXT("pack_sha256"), Narrative->GetPackDigest());
    const auto FirstVoiceRow = VoiceRows[0]->AsObject();
    FirstVoiceRow->SetStringField(TEXT("runtime_signal"), TEXT("phase_entered:Withdraw"));
    TestFalse(TEXT("Stale branch signals refuse voice binding"), ValidateManifest());
    FirstVoiceRow->SetStringField(TEXT("runtime_signal"), (*M01Lines)[0].Signal);
    FirstVoiceRow->SetStringField(TEXT("asset_path"), TEXT("/Game/Audio/Voice/aud_m01_vo_wrong.aud_m01_vo_wrong"));
    TestFalse(TEXT("Another line's voice asset cannot replace this canonical hook"), ValidateManifest());

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
        const FEchoesNarrativeCinematic* Cinematic =
            Narrative->GetCinematic(Operation);
        if (TestNotNull(
                *FString::Printf(TEXT("%s carries a cinematic"), *Key),
                Cinematic))
        {
            TestEqual(
                *FString::Printf(
                    TEXT("%s carries the authored four-shot storyboard"),
                    *Key),
                Cinematic->Shots.Num(),
                4);
            TestTrue(
                *FString::Printf(
                    TEXT("%s carries a positive editorial duration"), *Key),
                Cinematic->GetEditorialDurationSeconds() > 0.0f);
            TestFalse(
                *FString::Printf(
                    TEXT("%s does not invent named-character bodies"), *Key),
                Cinematic->bNamedCharacterPhysicalPresenceAsserted);
        }
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

    // M01's runtime contract is projected from the canonical source rather
    // than duplicated in the cinematic implementation.
    const FEchoesNarrativeCinematic* M01Cinematic = Narrative->GetCinematic(
        EEchoesOperationMode::CampaignPrologue);
    if (TestNotNull(TEXT("M01 opening cinematic is bound"), M01Cinematic))
    {
        TestEqual(TEXT("M01 cinematic keeps its canonical id"),
                  M01Cinematic->Id,
                  FString(TEXT("nar_m01_cin_opening")));
        TestEqual(TEXT("M01 cinematic keeps its canonical trigger"),
                  M01Cinematic->TriggerId,
                  FString(TEXT("nar_m01_evt_operation_started")));
        TestEqual(TEXT("M01 cinematic resolves its source trigger signal"),
                  M01Cinematic->Signal,
                  FString(TEXT(
                      "operation_ready:CampaignPrologue:RecoverArchive")));
        TestEqual(TEXT("M01 cinematic duration is source-authored"),
                  M01Cinematic->GetEditorialDurationSeconds(),
                  32.7f);
        const float ExpectedShotSeconds[] = {10.7f, 6.7f, 9.9f, 5.4f};
        for (int32 Index = 0;
             Index < M01Cinematic->Shots.Num() && Index < 4;
             ++Index)
        {
            const FEchoesNarrativeCinematicShot& Shot =
                M01Cinematic->Shots[Index];
            TestEqual(
                *FString::Printf(TEXT("M01 shot %d preserves its id"), Index + 1),
                Shot.Id,
                FString::Printf(TEXT("nar_m01_shot_%03d"), Index + 1));
            TestEqual(
                *FString::Printf(
                    TEXT("M01 shot %d preserves its duration"), Index + 1),
                Shot.EditorialTargetSeconds,
                ExpectedShotSeconds[Index]);
            TestTrue(TEXT("Each M01 shot keeps line references"),
                     !Shot.LineIds.IsEmpty());
            TestTrue(TEXT("Each M01 shot keeps its visual hook"),
                     !Shot.VisualHookIds.IsEmpty());
            TestTrue(TEXT("Each M01 shot keeps its audio hook"),
                     !Shot.AudioHookIds.IsEmpty());
        }
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
    Narrative->SetSubtitlePlaybackPaused(true, 101.0);
    TestTrue(TEXT("Subtitle clock reports paused"),
             Narrative->IsSubtitlePlaybackPaused());
    FString PausedText;
    TestTrue(TEXT("A long wall-clock pause retains the active subtitle"),
             Narrative->GetActiveSubtitle(
                 1001.0, Speaker, PausedText));
    TestEqual(TEXT("Pause consumes no subtitle time"), PausedText, Text);
    Narrative->SetSubtitlePlaybackPaused(false, 201.0);
    TestFalse(TEXT("Subtitle clock reports resumed"),
              Narrative->IsSubtitlePlaybackPaused());
    FString ResumedText;
    TestTrue(TEXT("Resume continues the same subtitle without a jump"),
             Narrative->GetActiveSubtitle(
                 201.1, Speaker, ResumedText));
    TestEqual(TEXT("Resume preserves accumulated subtitle elapsed time"),
              ResumedText,
              Text);
    FString SecondText;
    TestTrue(TEXT("The lane advances after the first line's duration"),
             Narrative->GetActiveSubtitle(
                 200.1 + FirstDuration + 0.1, Speaker, SecondText));
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
