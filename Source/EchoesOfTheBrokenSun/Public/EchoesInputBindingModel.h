// Author: Angelis Pseftis
#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"

class UInputSettings;

/** The two legacy input-mapping contracts consumed by the player controller. */
enum class EEchoesInputBindingKind : uint8
{
    Action,
    Axis
};

/** Whether an accepted mapping should be saved through UInputSettings. */
enum class EEchoesInputBindingPersistence : uint8
{
    Transient,
    Persist
};

/** A UI-neutral projection of one action or axis entry in UInputSettings. */
struct FEchoesInputBinding
{
    EEchoesInputBindingKind Kind = EEchoesInputBindingKind::Action;
    FName Command;
    FKey Key;
    bool bShift = false;
    bool bCtrl = false;
    bool bAlt = false;
    bool bCmd = false;
    float Scale = 1.0f;

    [[nodiscard]] bool IsValid() const;
    [[nodiscard]] bool HasSameCommand(const FEchoesInputBinding& Other) const;
    [[nodiscard]] bool HasSameChord(const FEchoesInputBinding& Other) const;
    [[nodiscard]] bool operator==(const FEchoesInputBinding& Other) const;
};

/** Replaces exactly one currently-installed binding for its own command. */
struct FEchoesInputBindingCandidate
{
    FEchoesInputBinding Existing;
    FEchoesInputBinding Replacement;
};

enum class EEchoesInputBindingValidation : uint8
{
    Accepted,
    NoChange,
    InvalidBinding,
    CommandNotInDefaultContract,
    ExistingBindingNotFound,
    CommandChanged,
    AxisScaleChanged,
    AxisModifiersUnsupported,
    NewConflict,
    DefaultPolicyUnavailable
};

/** Result for a capture dialog: conflicts identify the bindings occupying its chord. */
struct FEchoesInputBindingValidationResult
{
    EEchoesInputBindingValidation Code = EEchoesInputBindingValidation::InvalidBinding;
    TArray<FEchoesInputBinding> Conflicts;
    FString Detail;

    [[nodiscard]] bool IsAccepted() const
    {
        return Code == EEchoesInputBindingValidation::Accepted ||
            Code == EEchoesInputBindingValidation::NoChange;
    }
};

/**
 * Input-binding catalog, validation, and mutation boundary for the Controls UI.
 *
 * The default contract and its approved shared chords are parsed from the
 * project's DefaultInput.ini. A shared chord is accepted only when the complete
 * set of bindings on it exactly matches that default contract; this preserves
 * existing context-specific defaults without authorizing new overlaps.
 */
class ECHOESOFTHEBROKENSUN_API FEchoesInputBindingModel
{
public:
    /** Enumerates every currently installed legacy action and axis mapping. */
    static void EnumerateLive(
        const UInputSettings& InputSettings,
        TArray<FEchoesInputBinding>& OutBindings);

    /** Loads action and axis defaults from Config/DefaultInput.ini. */
    static bool LoadProjectDefaults(
        TArray<FEchoesInputBinding>& OutBindings,
        FString& OutError);

    /** Validates a replacement before any UInputSettings mutation occurs. */
    static FEchoesInputBindingValidationResult ValidateCandidate(
        const UInputSettings& InputSettings,
        const TArray<FEchoesInputBinding>& DefaultBindings,
        const FEchoesInputBindingCandidate& Candidate);

    /**
     * Validates then replaces the complete mapping snapshot as one logical
     * operation. Persistence occurs only after the in-memory replacement is
     * verified and only when Persistence is Persist.
     */
    static bool ApplyCandidate(
        UInputSettings& InputSettings,
        const TArray<FEchoesInputBinding>& DefaultBindings,
        const FEchoesInputBindingCandidate& Candidate,
        EEchoesInputBindingPersistence Persistence,
        FEchoesInputBindingValidationResult& OutValidation,
        FString& OutError);

    /** Restores the mappings parsed from DefaultInput.ini, never a copied catalog. */
    static bool ResetToProjectDefaults(
        UInputSettings& InputSettings,
        EEchoesInputBindingPersistence Persistence,
        FString& OutError);
};
