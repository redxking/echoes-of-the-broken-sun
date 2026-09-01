#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "EchoesCinematicSubsystem.generated.h"

class ALevelSequenceActor;
class ACineCameraActor;
class ULevelSequence;
class ULevelSequencePlayer;

/** The authored sequences the cinematic pipeline can play. */
UENUM()
enum class EEchoesCinematicSequence : uint8
{
    /** The D1 reference sequence: one measured, ground-referenced move over
     *  the battlefield origin, proving trigger, playback, skip, and return. */
    Reference
};

/**
 * The Sequencer pipeline for in-engine cinematics (release track D).
 *
 * Sequences are built procedurally over registered assets at runtime, under
 * one camera language: measured, ground-referenced moves at world scale, no
 * impossible swoops. Playback pauses the simulation cleanly, skip is always
 * available, and returning to play restores exactly the state the sequence
 * found. Sequences are presentation-only: they read state and write nothing
 * back into simulation, saves, replays, or checksums.
 */
UCLASS()
class ECHOESOFTHEBROKENSUN_API UEchoesCinematicSubsystem final
    : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(
            UEchoesCinematicSubsystem, STATGROUP_Tickables);
    }
    virtual bool IsTickableInEditor() const override { return false; }

    /** Starts one authored sequence; false when one is already playing or
     *  the world cannot host it. Pauses the simulation while playing. */
    bool PlaySequence(EEchoesCinematicSequence Sequence);

    /** Skips the active sequence immediately and returns to play. */
    void SkipActiveSequence();

    /**
     * The data-driven trigger path: maps an authored campaign trigger signal
     * to the sequence it starts, so cinematics are bound from source-authored
     * state rather than ad-hoc calls. Unset when the signal drives no
     * sequence. Track D registers operation intros/outros here as they are
     * authored.
     */
    [[nodiscard]] static TOptional<EEchoesCinematicSequence>
    ResolveSequenceForSignal(const FString& Signal);

    [[nodiscard]] bool IsSequenceActive() const { return bSequenceActive; }

    /** Reference-sequence camera language constants. */
    [[nodiscard]] static constexpr float GetReferenceDurationSeconds()
    {
        return 8.0f;
    }
    [[nodiscard]] static constexpr float GetReferenceCameraHeight()
    {
        return 900.0f;
    }

#if WITH_DEV_AUTOMATION_TESTS
    /** Advances the active sequence player directly, without a world tick. */
    void AdvanceActiveSequenceForTest(float DeltaSeconds);
    [[nodiscard]] ALevelSequenceActor* GetSequenceActorForTest() const
    {
        return SequenceActor;
    }
    [[nodiscard]] ACineCameraActor* GetCameraActorForTest() const
    {
        return CameraActor;
    }
    [[nodiscard]] int32 GetCompletedPlaybackCountForTest() const
    {
        return CompletedPlaybackCount;
    }
    [[nodiscard]] bool WasScenarioPausedBySequenceForTest() const
    {
        return bPausedScenario;
    }
#endif

private:
    [[nodiscard]] ULevelSequence* BuildReferenceSequence();
    void FinishActiveSequence(bool bSkipped);

    UPROPERTY(Transient)
    TObjectPtr<ULevelSequence> ActiveSequence;

    UPROPERTY(Transient)
    TObjectPtr<ALevelSequenceActor> SequenceActor;

    UPROPERTY(Transient)
    TObjectPtr<ULevelSequencePlayer> SequencePlayer;

    UPROPERTY(Transient)
    TObjectPtr<ACineCameraActor> CameraActor;

    bool bSequenceActive = false;
    bool bPausedScenario = false;
    bool bScenarioWasPaused = false;
    int32 CompletedPlaybackCount = 0;
};
