#include "EchoesNarrativeSubsystem.h"

#include "Dom/JsonObject.h"
#include "Components/AudioComponent.h"
#include "EchoesAudioMixSubsystem.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWave.h"
#include "Sound/SoundSubmix.h"
#include "UObject/StrongObjectPtr.h"
#include "GameFramework/InputSettings.h"
#include "EchoesHashUtility.h"
#include "EchoesOfTheBrokenSun.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
constexpr int32 NarrativePackVersion = 2;

[[nodiscard]] FString NarrativePackPath()
{
    return FPaths::ProjectContentDir() /
        TEXT("Narrative/Generated/EchoesNarrativePack.json");
}

[[nodiscard]] bool ReadRequiredStringArray(
    const FJsonObject& Object,
    const TCHAR* Field,
    TArray<FString>& OutValues)
{
    const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
    if (!Object.TryGetArrayField(Field, Values) || Values == nullptr)
    {
        return false;
    }
    OutValues.Reset();
    for (const TSharedPtr<FJsonValue>& Value : *Values)
    {
        FString Text;
        if (!Value.IsValid() || !Value->TryGetString(Text) || Text.IsEmpty())
        {
            OutValues.Reset();
            return false;
        }
        OutValues.Add(MoveTemp(Text));
    }
    return !OutValues.IsEmpty();
}
}

FString UEchoesNarrativeSubsystem::OperationPackKey(
    EEchoesOperationMode Operation)
{
    switch (Operation)
    {
        case EEchoesOperationMode::Skirmish:
            return FString();
        case EEchoesOperationMode::CampaignPrologue:
            return TEXT("CampaignPrologue");
        case EEchoesOperationMode::CampaignSevenAccounts:
            return TEXT("CampaignSevenAccounts");
        case EEchoesOperationMode::CampaignCityReserve:
            return TEXT("CampaignCityReserve");
        case EEchoesOperationMode::CampaignUnburiedRoad:
            return TEXT("CampaignUnburiedRoad");
        case EEchoesOperationMode::CampaignTermsOfContinuance:
            return TEXT("CampaignTermsOfContinuance");
        case EEchoesOperationMode::CampaignNamesWithoutBirths:
            return TEXT("CampaignNamesWithoutBirths");
        case EEchoesOperationMode::CampaignShapeOfSilence:
            return TEXT("CampaignShapeOfSilence");
        case EEchoesOperationMode::CampaignShapeBesideUs:
            return TEXT("CampaignShapeBesideUs");
        case EEchoesOperationMode::CampaignReserveAuthority:
            return TEXT("CampaignReserveAuthority");
        case EEchoesOperationMode::CampaignChoirAtLumeReach:
            return TEXT("CampaignChoirAtLumeReach");
        case EEchoesOperationMode::CampaignNoNeutralLedger:
            return TEXT("CampaignNoNeutralLedger");
        case EEchoesOperationMode::CampaignFutureThatWon:
            return TEXT("CampaignFutureThatWon");
        case EEchoesOperationMode::CampaignAssemblyOfTheMissing:
            return TEXT("CampaignAssemblyOfTheMissing");
        case EEchoesOperationMode::CampaignSeveralVoicesOneCommand:
            return TEXT("CampaignSeveralVoicesOneCommand");
        case EEchoesOperationMode::CampaignTheBrokenSun:
            return TEXT("CampaignTheBrokenSun");
    }
    return FString();
}

void UEchoesNarrativeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    FWorldDelegates::OnWorldCleanup.AddUObject(this, &UEchoesNarrativeSubsystem::OnVoiceWorldCleanup);
    LoadPack();
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_NARRATIVE_READY] ready=%s operations=%d lines=%d sha256=%s error=%s runtimeAuthority=presentation"),
        bReady ? TEXT("true") : TEXT("false"),
        Operations.Num(),
        TotalLineCount,
        *PackDigest,
        LoadError.IsEmpty() ? TEXT("none") : *LoadError);
}

void UEchoesNarrativeSubsystem::LoadPack()
{
    bReady = false;
    Operations.Reset();
    TotalLineCount = 0;
    LoadError.Reset();

    const FString PackPath = NarrativePackPath();
    TArray<uint8> PackBytes;
    if (!FFileHelper::LoadFileToArray(PackBytes, *PackPath) ||
        PackBytes.IsEmpty())
    {
        LoadError = TEXT("NARRATIVE_PACK_MISSING");
        return;
    }
    const FString ActualDigest = EchoesHash::ComputeSha256Hex(PackBytes);
    FString DigestText;
    if (!FFileHelper::LoadFileToString(DigestText, *(PackPath + TEXT(".sha256"))))
    {
        LoadError = TEXT("NARRATIVE_DIGEST_MISSING");
        return;
    }
    DigestText.TrimStartAndEndInline();
    if (DigestText.Len() != 64 || DigestText.ToLower() != ActualDigest)
    {
        LoadError = TEXT("NARRATIVE_DIGEST_MISMATCH");
        return;
    }

    const FUTF8ToTCHAR Converted(
        reinterpret_cast<const ANSICHAR*>(PackBytes.GetData()),
        PackBytes.Num());
    const FString JsonText(Converted.Length(), Converted.Get());
    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader =
        TJsonReaderFactory<>::Create(JsonText);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        LoadError = TEXT("NARRATIVE_PACK_PARSE_FAILED");
        return;
    }

    FString PackFormat;
    int32 PackVersion = 0;
    if (!Root->TryGetStringField(TEXT("pack_format"), PackFormat) ||
        PackFormat != TEXT("echoes-narrative-pack") ||
        !Root->TryGetNumberField(TEXT("pack_version"), PackVersion) ||
        PackVersion != NarrativePackVersion)
    {
        LoadError = TEXT("NARRATIVE_PACK_FORMAT_INVALID");
        return;
    }

    const TSharedPtr<FJsonObject>* OperationsObject = nullptr;
    if (!Root->TryGetObjectField(TEXT("operations"), OperationsObject) ||
        OperationsObject == nullptr || !OperationsObject->IsValid())
    {
        LoadError = TEXT("NARRATIVE_OPERATIONS_INVALID");
        return;
    }

    for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair :
         (*OperationsObject)->Values)
    {
        const TSharedPtr<FJsonObject>* Entry = nullptr;
        if (!Pair.Value.IsValid() || !Pair.Value->TryGetObject(Entry) ||
            Entry == nullptr || !Entry->IsValid())
        {
            LoadError = FString::Printf(
                TEXT("NARRATIVE_OPERATION_INVALID:%s"), *Pair.Key);
            Operations.Reset();
            return;
        }
        FOperationNarrative Narrative;
        const TArray<TSharedPtr<FJsonValue>>* ObjectiveValues = nullptr;
        const TArray<TSharedPtr<FJsonValue>>* LineValues = nullptr;
        const TSharedPtr<FJsonObject>* ResultValues = nullptr;
        const TSharedPtr<FJsonObject>* FailureValues = nullptr;
        const TSharedPtr<FJsonObject>* CinematicObject = nullptr;
        if (!(*Entry)->TryGetStringField(TEXT("title"), Narrative.Title) ||
            !(*Entry)->TryGetStringField(TEXT("briefing"), Narrative.Briefing) ||
            !(*Entry)->TryGetStringField(TEXT("retry"), Narrative.Retry) ||
            !(*Entry)->TryGetArrayField(TEXT("objectives"), ObjectiveValues) ||
            !(*Entry)->TryGetArrayField(TEXT("lines"), LineValues) ||
            !(*Entry)->TryGetObjectField(TEXT("results"), ResultValues) ||
            !(*Entry)->TryGetObjectField(TEXT("failures"), FailureValues) ||
            !(*Entry)->TryGetObjectField(TEXT("cinematic"), CinematicObject) ||
            Narrative.Briefing.IsEmpty() || ObjectiveValues == nullptr ||
            LineValues == nullptr || ResultValues == nullptr ||
            FailureValues == nullptr || CinematicObject == nullptr ||
            !CinematicObject->IsValid())
        {
            LoadError = FString::Printf(
                TEXT("NARRATIVE_OPERATION_INVALID:%s"), *Pair.Key);
            Operations.Reset();
            return;
        }
        bool bNamedCharacterPresence = false;
        const TArray<TSharedPtr<FJsonValue>>* ShotValues = nullptr;
        if (!(*CinematicObject)->TryGetStringField(
                TEXT("id"), Narrative.Cinematic.Id) ||
            !(*CinematicObject)->TryGetStringField(
                TEXT("trigger_id"), Narrative.Cinematic.TriggerId) ||
            !(*CinematicObject)->TryGetStringField(
                TEXT("signal"), Narrative.Cinematic.Signal) ||
            !(*CinematicObject)->TryGetStringField(
                TEXT("format"), Narrative.Cinematic.Format) ||
            !(*CinematicObject)->TryGetBoolField(
                TEXT("named_character_physical_presence_asserted"),
                bNamedCharacterPresence) ||
            !(*CinematicObject)->TryGetArrayField(TEXT("shots"), ShotValues) ||
            Narrative.Cinematic.Id.IsEmpty() ||
            Narrative.Cinematic.TriggerId.IsEmpty() ||
            Narrative.Cinematic.Signal.IsEmpty() ||
            Narrative.Cinematic.Format != TEXT("in_engine_storyboard") ||
            ShotValues == nullptr || ShotValues->IsEmpty())
        {
            LoadError = FString::Printf(
                TEXT("NARRATIVE_CINEMATIC_INVALID:%s"), *Pair.Key);
            Operations.Reset();
            return;
        }
        Narrative.Cinematic.bNamedCharacterPhysicalPresenceAsserted =
            bNamedCharacterPresence;
        for (const TSharedPtr<FJsonValue>& ShotValue : *ShotValues)
        {
            const TSharedPtr<FJsonObject>* ShotObject = nullptr;
            FEchoesNarrativeCinematicShot Shot;
            double EditorialTargetSeconds = 0.0;
            if (!ShotValue.IsValid() || !ShotValue->TryGetObject(ShotObject) ||
                ShotObject == nullptr || !ShotObject->IsValid() ||
                !(*ShotObject)->TryGetStringField(TEXT("id"), Shot.Id) ||
                !(*ShotObject)->TryGetNumberField(
                    TEXT("editorial_target_seconds"),
                    EditorialTargetSeconds) ||
                Shot.Id.IsEmpty() || EditorialTargetSeconds <= 0.0 ||
                !FMath::IsFinite(EditorialTargetSeconds) ||
                !ReadRequiredStringArray(
                    **ShotObject, TEXT("line_ids"), Shot.LineIds) ||
                !ReadRequiredStringArray(
                    **ShotObject, TEXT("visual_hook_ids"), Shot.VisualHookIds) ||
                !ReadRequiredStringArray(
                    **ShotObject, TEXT("audio_hook_ids"), Shot.AudioHookIds))
            {
                LoadError = FString::Printf(
                    TEXT("NARRATIVE_CINEMATIC_SHOT_INVALID:%s"), *Pair.Key);
                Operations.Reset();
                return;
            }
            Shot.EditorialTargetSeconds =
                static_cast<float>(EditorialTargetSeconds);
            Narrative.Cinematic.Shots.Add(MoveTemp(Shot));
        }
        for (const TSharedPtr<FJsonValue>& Objective : *ObjectiveValues)
        {
            FString Text;
            if (!Objective.IsValid() || !Objective->TryGetString(Text) ||
                Text.IsEmpty())
            {
                LoadError = FString::Printf(
                    TEXT("NARRATIVE_OBJECTIVE_INVALID:%s"), *Pair.Key);
                Operations.Reset();
                return;
            }
            Narrative.Objectives.Add(MoveTemp(Text));
        }
        for (const TSharedPtr<FJsonValue>& LineValue : *LineValues)
        {
            const TSharedPtr<FJsonObject>* LineObject = nullptr;
            FEchoesNarrativeLine Line;
            if (!LineValue.IsValid() ||
                !LineValue->TryGetObject(LineObject) ||
                LineObject == nullptr || !LineObject->IsValid() ||
                !(*LineObject)->TryGetStringField(TEXT("id"), Line.Id) ||
                !(*LineObject)->TryGetStringField(TEXT("speaker"), Line.Speaker) ||
                !(*LineObject)->TryGetStringField(TEXT("signal"), Line.Signal) ||
                !(*LineObject)->TryGetStringField(TEXT("text"), Line.Text) ||
                Line.Id.IsEmpty() || Line.Speaker.IsEmpty() ||
                Line.Signal.IsEmpty() || Line.Text.IsEmpty())
            {
                LoadError = FString::Printf(
                    TEXT("NARRATIVE_LINE_INVALID:%s"), *Pair.Key);
                Operations.Reset();
                return;
            }
            Narrative.Lines.Add(MoveTemp(Line));
        }
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Result :
             (*ResultValues)->Values)
        {
            FString Text;
            if (!Result.Value.IsValid() || !Result.Value->TryGetString(Text) ||
                Text.IsEmpty())
            {
                LoadError = FString::Printf(
                    TEXT("NARRATIVE_RESULT_INVALID:%s"), *Pair.Key);
                Operations.Reset();
                return;
            }
            Narrative.Results.Add(Result.Key, MoveTemp(Text));
        }
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Failure :
             (*FailureValues)->Values)
        {
            FString Text;
            if (!Failure.Value.IsValid() ||
                !Failure.Value->TryGetString(Text) || Text.IsEmpty())
            {
                LoadError = FString::Printf(
                    TEXT("NARRATIVE_FAILURE_INVALID:%s"), *Pair.Key);
                Operations.Reset();
                return;
            }
            Narrative.Failures.Add(Failure.Key, MoveTemp(Text));
        }
        const TSharedPtr<FJsonObject>* FailureLineValues = nullptr;
        if (!(*Entry)->TryGetObjectField(
                TEXT("failure_lines"), FailureLineValues) ||
            FailureLineValues == nullptr || !FailureLineValues->IsValid())
        {
            LoadError = FString::Printf(
                TEXT("NARRATIVE_FAILURE_LINES_INVALID:%s"), *Pair.Key);
            Operations.Reset();
            return;
        }
        for (const TPair<FString, TSharedPtr<FJsonValue>>& FailureLine :
             (*FailureLineValues)->Values)
        {
            FString LineId;
            if (!FailureLine.Value.IsValid() ||
                !FailureLine.Value->TryGetString(LineId) || LineId.IsEmpty())
            {
                LoadError = FString::Printf(
                    TEXT("NARRATIVE_FAILURE_LINES_INVALID:%s"), *Pair.Key);
                Operations.Reset();
                return;
            }
            Narrative.FailureLines.Add(FailureLine.Key, MoveTemp(LineId));
        }
        TotalLineCount += Narrative.Lines.Num();
        Operations.Add(Pair.Key, MoveTemp(Narrative));
    }

    // Parse demo subtree carrying system_voice and tutorial lines
    DemoLines.Reset();
    const TSharedPtr<FJsonObject>* DemoObject = nullptr;
    if (Root->TryGetObjectField(TEXT("demo"), DemoObject) &&
        DemoObject != nullptr && DemoObject->IsValid())
    {
        for (const auto& DemoPair : (*DemoObject)->Values)
        {
            const TSharedPtr<FJsonObject>* CategoryObj = nullptr;
            if (DemoPair.Value.IsValid() && DemoPair.Value->TryGetObject(CategoryObj) &&
                CategoryObj != nullptr && CategoryObj->IsValid())
            {
                const TArray<TSharedPtr<FJsonValue>>* DemoLineValues = nullptr;
                if ((*CategoryObj)->TryGetArrayField(TEXT("lines"), DemoLineValues) &&
                    DemoLineValues != nullptr)
                {
                    for (const TSharedPtr<FJsonValue>& LineValue : *DemoLineValues)
                    {
                        const TSharedPtr<FJsonObject>* LineObject = nullptr;
                        FEchoesNarrativeLine Line;
                        if (LineValue.IsValid() && LineValue->TryGetObject(LineObject) &&
                            LineObject != nullptr && LineObject->IsValid() &&
                            (*LineObject)->TryGetStringField(TEXT("id"), Line.Id) &&
                            (*LineObject)->TryGetStringField(TEXT("speaker"), Line.Speaker) &&
                            (*LineObject)->TryGetStringField(TEXT("signal"), Line.Signal) &&
                            (*LineObject)->TryGetStringField(TEXT("text"), Line.Text))
                        {
                            DemoLines.Add(MoveTemp(Line));
                        }
                    }
                }
            }
        }
    }

    PackDigest = ActualDigest;
    bReady = true;
}

bool UEchoesNarrativeSubsystem::HasOperation(
    EEchoesOperationMode Operation) const
{
    return bReady && Operations.Contains(OperationPackKey(Operation));
}

FString UEchoesNarrativeSubsystem::GetTitle(
    EEchoesOperationMode Operation) const
{
    const FOperationNarrative* Found =
        Operations.Find(OperationPackKey(Operation));
    return Found != nullptr ? Found->Title : FString();
}

FString UEchoesNarrativeSubsystem::GetBriefing(
    EEchoesOperationMode Operation) const
{
    const FOperationNarrative* Found =
        Operations.Find(OperationPackKey(Operation));
    return Found != nullptr ? Found->Briefing : FString();
}

TArray<FString> UEchoesNarrativeSubsystem::GetObjectives(
    EEchoesOperationMode Operation) const
{
    const FOperationNarrative* Found =
        Operations.Find(OperationPackKey(Operation));
    return Found != nullptr ? Found->Objectives : TArray<FString>();
}

FString UEchoesNarrativeSubsystem::GetResultCopy(
    EEchoesOperationMode Operation,
    const FString& CommitStatus) const
{
    const FOperationNarrative* Found =
        Operations.Find(OperationPackKey(Operation));
    if (Found == nullptr)
    {
        return FString();
    }
    const FString* Copy = Found->Results.Find(CommitStatus);
    return Copy != nullptr ? *Copy : FString();
}

FString UEchoesNarrativeSubsystem::GetRetryCopy(
    EEchoesOperationMode Operation) const
{
    const FOperationNarrative* Found =
        Operations.Find(OperationPackKey(Operation));
    return Found != nullptr ? Found->Retry : FString();
}

FString UEchoesNarrativeSubsystem::GetFailureCondition(
    EEchoesOperationMode Operation,
    const FString& ReasonCode) const
{
    const FOperationNarrative* Found =
        Operations.Find(OperationPackKey(Operation));
    if (Found == nullptr)
    {
        return FString();
    }
    const FString* Condition = Found->Failures.Find(ReasonCode);
    return Condition != nullptr ? *Condition : FString();
}

const FEchoesNarrativeCinematic* UEchoesNarrativeSubsystem::GetCinematic(
    EEchoesOperationMode Operation) const
{
    const FOperationNarrative* Found =
        Operations.Find(OperationPackKey(Operation));
    return Found != nullptr ? &Found->Cinematic : nullptr;
}

TArray<FEchoesNarrativeLine> UEchoesNarrativeSubsystem::GetLinesForSignal(
    EEchoesOperationMode Operation,
    const FString& Signal) const
{
    TArray<FEchoesNarrativeLine> Matched;
    const FOperationNarrative* Found =
        Operations.Find(OperationPackKey(Operation));
    if (Found != nullptr)
    {
        for (const FEchoesNarrativeLine& Line : Found->Lines)
        {
            if (Line.Signal == Signal)
            {
                Matched.Add(Line);
            }
        }
    }
    if (Matched.IsEmpty())
    {
        for (const FEchoesNarrativeLine& Line : DemoLines)
        {
            if (Line.Signal == Signal)
            {
                Matched.Add(Line);
            }
        }
    }
    return Matched;
}

const TArray<FEchoesNarrativeLine>* UEchoesNarrativeSubsystem::GetLines(
    EEchoesOperationMode Operation) const
{
    const FOperationNarrative* Found =
        Operations.Find(OperationPackKey(Operation));
    return Found != nullptr ? &Found->Lines : nullptr;
}

double UEchoesNarrativeSubsystem::SubtitleDurationSeconds(const FString& Text)
{
    return FMath::Clamp(2.4 + 0.045 * static_cast<double>(Text.Len()), 3.0, 9.0);
}

void UEchoesNarrativeSubsystem::EnqueueOperationStart(
    EEchoesOperationMode Operation,
    double NowSeconds)
{
    const FString Key = OperationPackKey(Operation);
    const FOperationNarrative* Found = Operations.Find(Key);
    if (Found == nullptr)
    {
        return;
    }
    const FString StartPrefix =
        FString::Printf(TEXT("operation_ready:%s:"), *Key);
    for (const FEchoesNarrativeLine& Line : Found->Lines)
    {
        if (Line.Signal.StartsWith(StartPrefix))
        {
            SubtitleQueue.Add(Line);
        }
    }
    if (ActiveLineStartSeconds < 0.0 && !SubtitleQueue.IsEmpty())
    {
        ActiveLineStartSeconds = bSubtitlePlaybackPaused
            ? SubtitlePauseStartedSeconds
            : NowSeconds;
    }
}

void UEchoesNarrativeSubsystem::EnqueueSignal(
    EEchoesOperationMode Operation,
    const FString& Signal,
    double NowSeconds)
{
    const FOperationNarrative* Found =
        Operations.Find(OperationPackKey(Operation));
    bool bAdded = false;
    if (Found != nullptr)
    {
        for (const FEchoesNarrativeLine& Line : Found->Lines)
        {
            if (Line.Signal == Signal)
            {
                SubtitleQueue.Add(Line);
                bAdded = true;
            }
        }
    }
    if (!bAdded)
    {
        for (const FEchoesNarrativeLine& Line : DemoLines)
        {
            if (Line.Signal == Signal)
            {
                SubtitleQueue.Add(Line);
                bAdded = true;
            }
        }
    }
    if (ActiveLineStartSeconds < 0.0 && !SubtitleQueue.IsEmpty())
    {
        ActiveLineStartSeconds = bSubtitlePlaybackPaused
            ? SubtitlePauseStartedSeconds
            : NowSeconds;
    }
}

void UEchoesNarrativeSubsystem::EnqueueFailureLine(
    EEchoesOperationMode Operation,
    const FString& ReasonCode,
    double NowSeconds)
{
    const FOperationNarrative* Found =
        Operations.Find(OperationPackKey(Operation));
    if (Found == nullptr)
    {
        return;
    }
    const FString* LineId = Found->FailureLines.Find(ReasonCode);
    if (LineId == nullptr)
    {
        LineId = Found->FailureLines.Find(TEXT("generic"));
    }
    if (LineId == nullptr)
    {
        return;
    }
    for (const FEchoesNarrativeLine& Line : Found->Lines)
    {
        if (Line.Id == *LineId)
        {
            SubtitleQueue.Add(Line);
            if (ActiveLineStartSeconds < 0.0)
            {
                ActiveLineStartSeconds = bSubtitlePlaybackPaused
                    ? SubtitlePauseStartedSeconds
                    : NowSeconds;
            }
            return;
        }
    }
}

FString UEchoesNarrativeSubsystem::ResolveInputTokens(const FString& Text)
{
    const auto Binding = [](const FName Action)
    {
        TArray<FInputActionKeyMapping> Mappings;
        GetDefault<UInputSettings>()->GetActionMappingByName(Action, Mappings);
        for (const auto& Mapping : Mappings)
        {
            if (!Mapping.Key.IsValid() || Mapping.Key.IsGamepadKey()) continue;
            FString Label;
            // FMacApplication exposes physical Command as the engine Control modifier.
            // Display the physical key the player must press, not the internal flag name.
#if PLATFORM_MAC
            if (Mapping.bCtrl) Label += TEXT("Command+");
            if (Mapping.bAlt) Label += TEXT("Option+");
#else
            if (Mapping.bCtrl) Label += TEXT("Ctrl+");
            if (Mapping.bAlt) Label += TEXT("Alt+");
#endif
            if (Mapping.bShift) Label += TEXT("Shift+");
#if PLATFORM_MAC
            if (Mapping.bCmd) Label += TEXT("Control+");
#else
            if (Mapping.bCmd) Label += TEXT("Command+");
#endif
            return Label + Mapping.Key.GetDisplayName().ToString();
        }
        return NSLOCTEXT("EchoesNarrative", "ControlUnassigned", "unassigned control").ToString();
    };
    FString Resolved = Text;
    // The current recenter action centers the selected entity. State this
    // precondition so the authored Anchor instruction remains truthful.
    Resolved.ReplaceInline(TEXT("{recenter_key}"), *FText::Format(
        NSLOCTEXT("EchoesNarrative", "AnchorRecenterControl", "{0} (with your Anchor selected)"),
        FText::FromString(Binding(TEXT("SnapKeyboardTargetToSelection")))).ToString());
    // The same active mappings drive both narrative and guided HUD instructions.
    Resolved.ReplaceInline(TEXT("{select_key}"), *Binding(TEXT("Select")));
    Resolved.ReplaceInline(TEXT("{zoom_in_key}"), *Binding(TEXT("CameraZoomIn")));
    Resolved.ReplaceInline(TEXT("{zoom_out_key}"), *Binding(TEXT("CameraZoomOut")));
    Resolved.ReplaceInline(TEXT("{assign_group_key}"), *Binding(TEXT("ArmControlGroupAssignment")));
    Resolved.ReplaceInline(TEXT("{recall_group_key}"), *Binding(TEXT("RecallControlGroup1")));
    if (Resolved.Contains(TEXT("{pan_keys}")))
    {
        TArray<FString> Labels;
        for (const FName Axis : {FName(TEXT("CameraForward")), FName(TEXT("CameraRight"))})
        {
            TArray<FInputAxisKeyMapping> Mappings;
            GetDefault<UInputSettings>()->GetAxisMappingByName(Axis, Mappings);
            for (const auto& Mapping : Mappings)
                if (Mapping.Key.IsValid() && !Mapping.Key.IsGamepadKey() && Mapping.Scale != 0.0f)
                    Labels.AddUnique(Mapping.Key.GetDisplayName().ToString());
        }
        const FString Pan = Labels.IsEmpty()
            ? NSLOCTEXT("EchoesNarrative", "ControlUnassigned", "unassigned control").ToString()
            : FString::Join(Labels, TEXT(" / "));
        Resolved.ReplaceInline(TEXT("{pan_keys}"), *Pan);
    }
    Resolved.ReplaceInline(TEXT("{guard_key}"), *Binding(TEXT("GuardAtCursor")));
    Resolved.ReplaceInline(TEXT("{alert_key}"), *Binding(TEXT("JumpToLatestAlert")));
    return Resolved.Contains(TEXT("{")) || Resolved.Contains(TEXT("}")) ? FString() : Resolved;
}

bool UEchoesNarrativeSubsystem::GetActiveSubtitle(
    double NowSeconds,
    FString& OutSpeaker,
    FString& OutText)
{
    const double SubtitleNowSeconds = bSubtitlePlaybackPaused
        ? SubtitlePauseStartedSeconds
        : NowSeconds;
    while (!SubtitleQueue.IsEmpty())
    {
        const FEchoesNarrativeLine& Head = SubtitleQueue[0];
        const FString ResolvedText = ResolveInputTokens(Head.Text);
        if (ResolvedText.IsEmpty())
        {
            SubtitleQueue.RemoveAt(0);
            ActiveLineStartSeconds = SubtitleQueue.IsEmpty() ? -1.0 : SubtitleNowSeconds;
            continue;
        }
        if (ActiveVoiceLineId != Head.Id)
        {
            StopActiveVoice();
            ActiveVoiceLineId = Head.Id;
            ActiveVoiceDuration = StartVoiceForLine(Head);
            // Voice and subtitle start together even if presentation was delayed.
            if (ActiveVoiceDuration > 0.0) ActiveLineStartSeconds = SubtitleNowSeconds;
        }
        RefreshVoiceDucking();
        const double Duration = FMath::Max(SubtitleDurationSeconds(ResolvedText), ActiveVoiceDuration);
        if (ActiveLineStartSeconds < 0.0)
        {
            ActiveLineStartSeconds = SubtitleNowSeconds;
        }
        if (SubtitleNowSeconds - ActiveLineStartSeconds < Duration)
        {
            OutSpeaker = Head.Speaker;
            OutText = ResolvedText;
            return true;
        }
        StopActiveVoice();
        SubtitleQueue.RemoveAt(0);
        ActiveLineStartSeconds = SubtitleQueue.IsEmpty()
            ? -1.0
            : SubtitleNowSeconds;
    }
    return false;
}

void UEchoesNarrativeSubsystem::SetSubtitlePlaybackPaused(
    const bool bPaused,
    const double NowSeconds)
{
    if (bPaused == bSubtitlePlaybackPaused)
    {
        return;
    }
    if (bPaused)
    {
        SubtitlePauseStartedSeconds = NowSeconds;
        bSubtitlePlaybackPaused = true;
        if (ActiveVoice) ActiveVoice->SetPaused(true);
        RefreshVoiceDucking();
        return;
    }

    const double PausedDuration = FMath::Max(
        0.0, NowSeconds - SubtitlePauseStartedSeconds);
    if (ActiveLineStartSeconds >= 0.0)
    {
        ActiveLineStartSeconds += PausedDuration;
    }
    SubtitlePauseStartedSeconds = -1.0;
    bSubtitlePlaybackPaused = false;
    if (ActiveVoice) ActiveVoice->SetPaused(false);
    RefreshVoiceDucking();
}

void UEchoesNarrativeSubsystem::ClearSubtitleQueue()
{
    StopActiveVoice();
    SubtitleQueue.Reset();
    ActiveLineStartSeconds = -1.0;
    SubtitlePauseStartedSeconds = -1.0;
    bSubtitlePlaybackPaused = false;
}

void UEchoesNarrativeSubsystem::Deinitialize()
{
    FWorldDelegates::OnWorldCleanup.RemoveAll(this);
    ClearSubtitleQueue();
    for (auto& Pair : VoiceWaves)
        if (Pair.Value) Pair.Value->SoundSubmixObject = nullptr;
    VoiceWaves.Reset();
    Super::Deinitialize();
}

void UEchoesNarrativeSubsystem::LoadVoiceBindings()
{
    if (bVoiceBindingsLoaded) return;
    bVoiceBindingsLoaded = true;
    const FString Path = FPaths::ProjectContentDir() / TEXT("Audio/Source/Narrative/m01_voice_bindings.json");
    TArray<uint8> Bytes;
    FString RecordedDigest;
    if (!FFileHelper::LoadFileToArray(Bytes, *Path) ||
        !FFileHelper::LoadFileToString(RecordedDigest, *(Path + TEXT(".sha256")))) return;
    if (Bytes.IsEmpty()) return;
    RecordedDigest.TrimStartAndEndInline();
    if (RecordedDigest != EchoesHash::ComputeSha256Hex(Bytes)) return;
    const FUTF8ToTCHAR Converted(reinterpret_cast<const ANSICHAR*>(Bytes.GetData()), Bytes.Num());
    const FString Json(Converted.Length(), Converted.Get());
    TMap<FString, FString> Paths;
    TMap<FString, double> Durations;
    if (!ValidateVoiceBindings(Json, Paths, &Durations)) return;
    // Validate the entire source mapping before loading any registered asset.
    TMap<FString, TObjectPtr<USoundWave>> Loaded;
    TArray<TStrongObjectPtr<USoundWave>> Retained;
    for (const auto& Pair : Paths)
    {
        USoundWave* Wave = LoadObject<USoundWave>(nullptr, *Pair.Value);
        if (!Wave || Wave->bLooping || !FMath::IsFinite(Wave->GetDuration()) ||
            Wave->GetDuration() <= 0 ||
            FMath::Abs(static_cast<double>(Wave->GetDuration()) - Durations.FindChecked(Pair.Key)) > 0.01)
        {
            UE_LOG(LogEchoes, Warning, TEXT("[ECHOES_NARRATIVE_VOICE_REFUSED] line=%s reason=asset_missing_or_duration_mismatch"), *Pair.Key);
            return;
        }
        Retained.Emplace(Wave);
        Loaded.Add(Pair.Key, Wave);
    }
    VoiceWaves = MoveTemp(Loaded);
    UE_LOG(LogEchoes, Display, TEXT("[ECHOES_NARRATIVE_VOICE] registered=%d expected=%d listeningVerified=false"), VoiceWaves.Num(), Paths.Num());
}

double UEchoesNarrativeSubsystem::StartVoiceForLine(const FEchoesNarrativeLine& Line)
{
    UWorld* World = GetWorld();
    if (!World || !World->IsGameWorld()) return 0.0;
    LoadVoiceBindings();
    const auto* Found = VoiceWaves.Find(Line.Id);
    USoundWave* Wave = Found ? Found->Get() : nullptr;
    auto* Mix = World->GetSubsystem<UEchoesAudioMixSubsystem>();
    if (!Wave || !Mix || !Mix->GetCategorySubmix(EEchoesAudioCategory::Dialogue)) return 0.0;
    VoiceRoutingWorld = World;
    Wave->SoundSubmixObject = Mix->GetCategorySubmix(EEchoesAudioCategory::Dialogue);
    ActiveVoice = UGameplayStatics::SpawnSound2D(World, Wave, 1.0f, 1.0f, 0.0f, nullptr, false, false);
    if (!ActiveVoice) return 0.0;
    ActiveVoice->OnAudioFinished.AddDynamic(this, &UEchoesNarrativeSubsystem::OnVoicePlaybackFinished);
    ActiveVoice->SetPaused(bSubtitlePlaybackPaused);
    RefreshVoiceDucking();
    return Wave->GetDuration();
}

void UEchoesNarrativeSubsystem::RefreshVoiceDucking()
{
    if (UWorld* World = GetWorld())
        if (auto* Mix = World->GetSubsystem<UEchoesAudioMixSubsystem>())
            Mix->SetDialogueDuckingActive(ActiveVoice && ActiveVoice->IsPlaying() &&
                !bSubtitlePlaybackPaused && Mix->GetAppliedCategoryGain(EEchoesAudioCategory::Dialogue) > 0.0f);
}

void UEchoesNarrativeSubsystem::StopActiveVoice()
{
    if (ActiveVoice)
    {
        ActiveVoice->OnAudioFinished.RemoveDynamic(this, &UEchoesNarrativeSubsystem::OnVoicePlaybackFinished);
        ActiveVoice->Stop();
        ActiveVoice->DestroyComponent();
        ActiveVoice = nullptr;
    }
    ActiveVoiceLineId.Reset();
    ActiveVoiceDuration = 0.0;
    RefreshVoiceDucking();
}

void UEchoesNarrativeSubsystem::OnVoicePlaybackFinished()
{
    if (UWorld* World = GetWorld())
        if (auto* Mix = World->GetSubsystem<UEchoesAudioMixSubsystem>())
            Mix->SetDialogueDuckingActive(false);
}

bool UEchoesNarrativeSubsystem::ValidateVoiceBindings(
    const FString& Json, TMap<FString, FString>& OutPaths,
    TMap<FString, double>* OutDurations) const
{
    if (OutDurations) OutDurations->Reset();
    TMap<FString, double> Durations;
    OutPaths.Reset();
    TSharedPtr<FJsonObject> Root;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Root) || !Root.IsValid()) return false;
    double Version = 0;
    FString Digest;
    const TArray<TSharedPtr<FJsonValue>>* Lines = nullptr;
    if (!Root->TryGetNumberField(TEXT("schema_version"), Version) || Version != 1 ||
        !Root->TryGetStringField(TEXT("pack_sha256"), Digest) || Digest != PackDigest ||
        !Root->TryGetArrayField(TEXT("lines"), Lines) || !Lines) return false;
    const TArray<FEchoesNarrativeLine>* Canonical = GetLines(EEchoesOperationMode::CampaignPrologue);
    if (!Canonical || Lines->Num() != Canonical->Num()) return false;
    OutPaths.Reset();
    TMap<FString, FString> Paths;
    TSet<FString> SeenAssets;
    for (const auto& Value : *Lines)
    {
        const TSharedPtr<FJsonObject>* Row = nullptr;
        if (!Value.IsValid() || !Value->TryGetObject(Row) || !Row || !Row->IsValid()) return false;
        FString Id, Speaker, Text, Signal, Asset;
        double Duration = 0;
        if (!(*Row)->TryGetStringField(TEXT("line_id"), Id) ||
            !(*Row)->TryGetStringField(TEXT("speaker"), Speaker) ||
            !(*Row)->TryGetStringField(TEXT("text"), Text) ||
            !(*Row)->TryGetStringField(TEXT("runtime_signal"), Signal) ||
            !(*Row)->TryGetStringField(TEXT("asset_path"), Asset) ||
            !(*Row)->TryGetNumberField(TEXT("duration_seconds"), Duration) ||
            !FMath::IsFinite(Duration) || Duration <= 0 || Duration > 120 ||
            !Asset.StartsWith(TEXT("/Game/Audio/Voice/aud_m01_vo_")) ||
            Asset.Contains(TEXT("..")) || Paths.Contains(Id) || SeenAssets.Contains(Asset)) return false;
        const FEchoesNarrativeLine* Match = Canonical->FindByPredicate(
            [&Id](const FEchoesNarrativeLine& Line) { return Line.Id == Id; });
        if (!Match || Match->Speaker != Speaker || Match->Text != Text || Match->Signal != Signal) return false;
        const FString Hook = Id.Replace(TEXT("nar_m01_line_"), TEXT("aud_m01_vo_"));
        if (Asset != FString::Printf(TEXT("/Game/Audio/Voice/%s.%s"), *Hook, *Hook)) return false;
        Paths.Add(Id, Asset);
        Durations.Add(Id, Duration);
        SeenAssets.Add(Asset);
    }
    OutPaths = MoveTemp(Paths);
    if (OutDurations) *OutDurations = MoveTemp(Durations);
    return true;
}

void UEchoesNarrativeSubsystem::OnVoiceWorldCleanup(UWorld* World, bool, bool)
{
    if (VoiceRoutingWorld.Get() != World) return;
    ClearSubtitleQueue();
    for (auto& Pair : VoiceWaves)
        if (Pair.Value) Pair.Value->SoundSubmixObject = nullptr;
    VoiceRoutingWorld.Reset();
}
