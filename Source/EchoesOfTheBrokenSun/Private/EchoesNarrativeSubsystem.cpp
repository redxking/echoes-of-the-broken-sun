#include "EchoesNarrativeSubsystem.h"

#include "Dom/JsonObject.h"
#include "EchoesHashUtility.h"
#include "EchoesOfTheBrokenSun.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
constexpr int32 NarrativePackVersion = 1;

[[nodiscard]] FString NarrativePackPath()
{
    return FPaths::ProjectContentDir() /
        TEXT("Narrative/Generated/EchoesNarrativePack.json");
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
        if (!(*Entry)->TryGetStringField(TEXT("title"), Narrative.Title) ||
            !(*Entry)->TryGetStringField(TEXT("briefing"), Narrative.Briefing) ||
            !(*Entry)->TryGetStringField(TEXT("retry"), Narrative.Retry) ||
            !(*Entry)->TryGetArrayField(TEXT("objectives"), ObjectiveValues) ||
            !(*Entry)->TryGetArrayField(TEXT("lines"), LineValues) ||
            !(*Entry)->TryGetObjectField(TEXT("results"), ResultValues) ||
            !(*Entry)->TryGetObjectField(TEXT("failures"), FailureValues) ||
            Narrative.Briefing.IsEmpty() || ObjectiveValues == nullptr ||
            LineValues == nullptr || ResultValues == nullptr ||
            FailureValues == nullptr)
        {
            LoadError = FString::Printf(
                TEXT("NARRATIVE_OPERATION_INVALID:%s"), *Pair.Key);
            Operations.Reset();
            return;
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
        DemoObject != nullptr && (*DemoObject)->IsValid())
    {
        for (const auto& DemoPair : (*DemoObject)->Values)
        {
            const TSharedPtr<FJsonObject>* CategoryObj = nullptr;
            if (DemoPair.Value.IsValid() && DemoPair.Value->TryGetObject(CategoryObj) &&
                CategoryObj != nullptr && (*CategoryObj)->IsValid())
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
                            LineObject != nullptr && (*LineObject)->IsValid() &&
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
        ActiveLineStartSeconds = NowSeconds;
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
        ActiveLineStartSeconds = NowSeconds;
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
                ActiveLineStartSeconds = NowSeconds;
            }
            return;
        }
    }
}

bool UEchoesNarrativeSubsystem::GetActiveSubtitle(
    double NowSeconds,
    FString& OutSpeaker,
    FString& OutText)
{
    while (!SubtitleQueue.IsEmpty())
    {
        const FEchoesNarrativeLine& Head = SubtitleQueue[0];
        const double Duration = SubtitleDurationSeconds(Head.Text);
        if (ActiveLineStartSeconds < 0.0)
        {
            ActiveLineStartSeconds = NowSeconds;
        }
        if (NowSeconds - ActiveLineStartSeconds < Duration)
        {
            OutSpeaker = Head.Speaker;
            OutText = Head.Text;
            return true;
        }
        SubtitleQueue.RemoveAt(0);
        ActiveLineStartSeconds = SubtitleQueue.IsEmpty() ? -1.0 : NowSeconds;
    }
    return false;
}

void UEchoesNarrativeSubsystem::ClearSubtitleQueue()
{
    SubtitleQueue.Reset();
    ActiveLineStartSeconds = -1.0;
}
