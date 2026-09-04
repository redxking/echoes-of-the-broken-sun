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
    Reference,

    /** Title Sequence: The Fracture of Soryn (72s). */
    TitleSequence,

    /** Act I -> II Transition: Descent to the Unburied Road (28s). */
    Act1ToAct2Transition,

    /** Act II -> III Transition: The Crownfall Rift (30s). */
    Act2ToAct3Transition,

    /** Act III Climax Transition: The Solar-Fall Convergence (24s). */
    Act3ClimaxTransition,

    /** Ending A: Restoration - Reconstituting the central power grid (32s). */
    EndingRestoration,

    /** Ending B: Controlled Stabilization - Assembly quotas and balanced flow (32s). */
    EndingControlledStabilization,

    /** Ending C: Extinguishment - Draining the solar core into cold silence (32s). */
    EndingExtinguishment,

    /** Ending D: Open Evolution - Releasing the choir harmonic frequency (32s). */
    EndingOpenEvolution
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

    /** Authoring duration query for any sequence. */
    [[nodiscard]] static float GetSequenceDurationSeconds(
        EEchoesCinematicSequence Sequence);

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
    [[nodiscard]] ULevelSequence* BuildCustomSequence(
        const TCHAR* SequenceName,
        const TCHAR* CameraBindingName,
        float DurationSeconds,
        const FVector& StartLocation,
        const FRotator& StartRotation,
        const FVector& EndLocation,
        const FRotator& EndRotation);

    [[nodiscard]] ULevelSequence* BuildReferenceSequence();
    [[nodiscard]] ULevelSequence* BuildTitleSequence();
    [[nodiscard]] ULevelSequence* BuildAct1ToAct2Sequence();
    [[nodiscard]] ULevelSequence* BuildAct2ToAct3Sequence();
    [[nodiscard]] ULevelSequence* BuildAct3ClimaxSequence();
    [[nodiscard]] ULevelSequence* BuildEndingRestorationSequence();
    [[nodiscard]] ULevelSequence* BuildEndingControlledStabilizationSequence();
    [[nodiscard]] ULevelSequence* BuildEndingExtinguishmentSequence();
    [[nodiscard]] ULevelSequence* BuildEndingOpenEvolutionSequence();

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
