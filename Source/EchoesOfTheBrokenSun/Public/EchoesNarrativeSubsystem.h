#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "EchoesPrologueMissionModel.h"

#include "EchoesNarrativeSubsystem.generated.h"

class UAudioComponent;
class USoundWave;

/** One authored dialogue line as the runtime pack carries it. */
USTRUCT()
struct FEchoesNarrativeLine
{
    GENERATED_BODY()

    UPROPERTY()
    FString Id;

    UPROPERTY()
    FString Speaker;

    UPROPERTY()
    FString Signal;

    UPROPERTY()
    FString Text;
};

/** One authored in-engine storyboard shot projected from narrative source. */
USTRUCT()
struct FEchoesNarrativeCinematicShot
{
    GENERATED_BODY()

    UPROPERTY()
    FString Id;

    UPROPERTY()
    float EditorialTargetSeconds = 0.0f;

    UPROPERTY()
    TArray<FString> LineIds;

    UPROPERTY()
    TArray<FString> VisualHookIds;

    UPROPERTY()
    TArray<FString> AudioHookIds;
};

/** Authored cinematic contract projected from one mission source record. */
USTRUCT()
struct FEchoesNarrativeCinematic
{
    GENERATED_BODY()

    UPROPERTY()
    FString Id;

    UPROPERTY()
    FString TriggerId;

    UPROPERTY()
    FString Signal;

    UPROPERTY()
    FString Format;

    UPROPERTY()
    bool bNamedCharacterPhysicalPresenceAsserted = false;

    UPROPERTY()
    TArray<FEchoesNarrativeCinematicShot> Shots;

    [[nodiscard]] float GetEditorialDurationSeconds() const
    {
        float Duration = 0.0f;
        for (const FEchoesNarrativeCinematicShot& Shot : Shots)
        {
            Duration += Shot.EditorialTargetSeconds;
        }
        return Duration;
    }
};

/**
 * Loads the digest-verified narrative pack compiled from
 * `Content/Narrative/Source` and serves per-operation briefs, objectives,
 * dialogue lines, result copy, and failure copy to the presentation layer.
 *
 * Fail closed: a missing, corrupt, or digest-mismatched pack leaves the
 * subsystem not-ready and every query empty. Nothing here writes into
 * simulation state, saves, replays, or checksums.
 */
UCLASS()
class ECHOESOFTHEBROKENSUN_API UEchoesNarrativeSubsystem final
    : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    [[nodiscard]] bool IsReady() const { return bReady; }
    [[nodiscard]] const FString& GetLoadError() const { return LoadError; }
    [[nodiscard]] int32 GetOperationCount() const { return Operations.Num(); }
    [[nodiscard]] int32 GetTotalLineCount() const { return TotalLineCount; }
    [[nodiscard]] int32 GetDemoLineCount() const { return DemoLines.Num(); }
    [[nodiscard]] const TArray<FEchoesNarrativeLine>& GetDemoLines() const { return DemoLines; }
    [[nodiscard]] const FString& GetPackDigest() const { return PackDigest; }

    /** True when a narrative contract exists for the operation. */
    [[nodiscard]] bool HasOperation(EEchoesOperationMode Operation) const;

    [[nodiscard]] FString GetTitle(EEchoesOperationMode Operation) const;
    [[nodiscard]] FString GetBriefing(EEchoesOperationMode Operation) const;
    [[nodiscard]] TArray<FString> GetObjectives(
        EEchoesOperationMode Operation) const;
    [[nodiscard]] FString GetResultCopy(
        EEchoesOperationMode Operation,
        const FString& CommitStatus) const;
    [[nodiscard]] FString GetRetryCopy(EEchoesOperationMode Operation) const;
    [[nodiscard]] FString GetFailureCondition(
        EEchoesOperationMode Operation,
        const FString& ReasonCode) const;

    /** The registered in-engine storyboard contract for an operation. */
    [[nodiscard]] const FEchoesNarrativeCinematic* GetCinematic(
        EEchoesOperationMode Operation) const;

    /** Every authored line bound to one runtime signal, in authored order. */
    [[nodiscard]] TArray<FEchoesNarrativeLine> GetLinesForSignal(
        EEchoesOperationMode Operation,
        const FString& Signal) const;

    /** All lines of one operation, in authored order. */
    [[nodiscard]] const TArray<FEchoesNarrativeLine>* GetLines(
        EEchoesOperationMode Operation) const;

    /** The pack key for an operation mode; empty for Skirmish. */
    [[nodiscard]] static FString OperationPackKey(
        EEchoesOperationMode Operation);

    // --- Subtitle queue ----------------------------------------------------
    // Lines enqueue in authored order and each holds the lane for a duration
    // scaled to its length. Presentation only; an empty pack means an empty
    // queue and a silent lane.

    /** Enqueues every line bound to the operation's own start signal. */
    void EnqueueOperationStart(
        EEchoesOperationMode Operation,
        double NowSeconds);

    /** Enqueues every line bound to one exact runtime signal. */
    void EnqueueSignal(
        EEchoesOperationMode Operation,
        const FString& Signal,
        double NowSeconds);

    /** Enqueues the single authored line for one failure reason code.
     *  Runtime failure-reason delivery is not yet bound, so callers pass
     *  "generic" until it is; an unknown reason falls back to generic. */
    void EnqueueFailureLine(
        EEchoesOperationMode Operation,
        const FString& ReasonCode,
        double NowSeconds);

    /** The line currently owning the subtitle lane, if any. */
    [[nodiscard]] bool GetActiveSubtitle(
        double NowSeconds,
        FString& OutSpeaker,
        FString& OutText);

    /** Freezes or resumes the subtitle clock without consuming queued time. */
    void SetSubtitlePlaybackPaused(bool bPaused, double NowSeconds);
    [[nodiscard]] bool IsSubtitlePlaybackPaused() const
    {
        return bSubtitlePlaybackPaused;
    }

    /** Clears any queued or active lines (title screen, scenario change). */
    void ClearSubtitleQueue();

    [[nodiscard]] int32 GetQueuedLineCountForTest() const
    {
        return SubtitleQueue.Num();
    }
    [[nodiscard]] FString GetLastQueuedLineIdForTest() const
    {
        return SubtitleQueue.Num() > 0 ? SubtitleQueue.Last().Id : FString();
    }

    /** Resolves authored control tokens from current input mappings; unknown tokens refuse display. */
    [[nodiscard]] static FString ResolveInputTokens(const FString& Text);

    /** Seconds one line owns the lane: base plus per-character reading time. */
    [[nodiscard]] static double SubtitleDurationSeconds(const FString& Text);

private:
    friend class FEchoesNarrativePackTest;
    bool ValidateVoiceBindings(const FString& Json, TMap<FString, FString>& OutPaths,
        TMap<FString, double>* OutDurations = nullptr) const;

    struct FOperationNarrative
    {
        FString Title;
        FString Briefing;
        TArray<FString> Objectives;
        TArray<FEchoesNarrativeLine> Lines;
        TMap<FString, FString> Results;
        TMap<FString, FString> Failures;
        TMap<FString, FString> FailureLines;
        FString Retry;
        FEchoesNarrativeCinematic Cinematic;
    };

    void LoadPack();
    void LoadVoiceBindings();
    void StopActiveVoice();
    void OnVoiceWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);
    TWeakObjectPtr<UWorld> VoiceRoutingWorld;
    void RefreshVoiceDucking();
    UFUNCTION()
    void OnVoicePlaybackFinished();
    double StartVoiceForLine(const FEchoesNarrativeLine& Line);

    UPROPERTY(Transient)
    TMap<FString, TObjectPtr<USoundWave>> VoiceWaves;

    UPROPERTY(Transient)
    TObjectPtr<UAudioComponent> ActiveVoice;

    FString ActiveVoiceLineId;
    double ActiveVoiceDuration = 0.0;
    bool bVoiceBindingsLoaded = false;


    TMap<FString, FOperationNarrative> Operations;
    TArray<FEchoesNarrativeLine> DemoLines;
    TArray<FEchoesNarrativeLine> SubtitleQueue;
    double ActiveLineStartSeconds = -1.0;
    double SubtitlePauseStartedSeconds = -1.0;
    FString PackDigest;
    FString LoadError;
    int32 TotalLineCount = 0;
    bool bReady = false;
    bool bSubtitlePlaybackPaused = false;
};
