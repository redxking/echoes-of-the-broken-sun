// Author: Angelis Pseftis
#include "EchoesInputBindingModel.h"

#include "GameFramework/InputSettings.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"

namespace
{
constexpr TCHAR InputSettingsSection[] = TEXT("/Script/Engine.InputSettings");

bool ContainsBinding(
    const TArray<FEchoesInputBinding>& Bindings,
    const FEchoesInputBinding& Needle)
{
    return Bindings.ContainsByPredicate(
        [&Needle](const FEchoesInputBinding& Binding) { return Binding == Needle; });
}

bool IsContractCommand(
    const TArray<FEchoesInputBinding>& DefaultBindings,
    const FEchoesInputBinding& Binding)
{
    return DefaultBindings.ContainsByPredicate(
        [&Binding](const FEchoesInputBinding& DefaultBinding)
        {
            return DefaultBinding.HasSameCommand(Binding);
        });
}

void GetChordBindings(
    const TArray<FEchoesInputBinding>& Bindings,
    const FEchoesInputBinding& Chord,
    TArray<FEchoesInputBinding>& OutBindings)
{
    OutBindings.Reset();
    for (const FEchoesInputBinding& Binding : Bindings)
    {
        if (Binding.HasSameChord(Chord))
        {
            OutBindings.Add(Binding);
        }
    }
}

bool HasSameBindingSet(
    const TArray<FEchoesInputBinding>& Left,
    const TArray<FEchoesInputBinding>& Right)
{
    if (Left.Num() != Right.Num())
    {
        return false;
    }

    TArray<FEchoesInputBinding> Unmatched = Right;
    for (const FEchoesInputBinding& Binding : Left)
    {
        if (Unmatched.RemoveSingle(Binding) == 0)
        {
            return false;
        }
    }
    return Unmatched.IsEmpty();
}

bool ParseActionMapping(const FString& Text, FEchoesInputBinding& OutBinding)
{
    FString CommandText;
    FString KeyText;
    if (!FParse::Value(*Text, TEXT("ActionName="), CommandText) ||
        !FParse::Value(*Text, TEXT("Key="), KeyText))
    {
        return false;
    }

    OutBinding = FEchoesInputBinding();
    OutBinding.Kind = EEchoesInputBindingKind::Action;
    OutBinding.Command = FName(*CommandText);
    OutBinding.Key = FKey(FName(*KeyText));
    FParse::Bool(*Text, TEXT("bShift="), OutBinding.bShift);
    FParse::Bool(*Text, TEXT("bCtrl="), OutBinding.bCtrl);
    FParse::Bool(*Text, TEXT("bAlt="), OutBinding.bAlt);
    FParse::Bool(*Text, TEXT("bCmd="), OutBinding.bCmd);
    return OutBinding.IsValid();
}

bool ParseAxisMapping(const FString& Text, FEchoesInputBinding& OutBinding)
{
    FString CommandText;
    FString KeyText;
    if (!FParse::Value(*Text, TEXT("AxisName="), CommandText) ||
        !FParse::Value(*Text, TEXT("Key="), KeyText))
    {
        return false;
    }

    OutBinding = FEchoesInputBinding();
    OutBinding.Kind = EEchoesInputBindingKind::Axis;
    OutBinding.Command = FName(*CommandText);
    OutBinding.Key = FKey(FName(*KeyText));
    if (!FParse::Value(*Text, TEXT("Scale="), OutBinding.Scale))
    {
        return false;
    }
    return OutBinding.IsValid();
}

bool LoadBindingsFromConfig(
    const FConfigFile& Config,
    const FString& SourceDescription,
    TArray<FEchoesInputBinding>& OutBindings,
    FString& OutError)
{
    OutBindings.Reset();
    TArray<FString> ActionLines;
    TArray<FString> AxisLines;
    Config.GetArray(InputSettingsSection, TEXT("ActionMappings"), ActionLines);
    Config.GetArray(InputSettingsSection, TEXT("AxisMappings"), AxisLines);

    for (const FString& ActionLine : ActionLines)
    {
        FEchoesInputBinding Binding;
        if (!ParseActionMapping(ActionLine, Binding))
        {
            OutError = FString::Printf(
                TEXT("Invalid action mapping in %s: %s"),
                *SourceDescription, *ActionLine);
            OutBindings.Reset();
            return false;
        }
        OutBindings.Add(Binding);
    }
    for (const FString& AxisLine : AxisLines)
    {
        FEchoesInputBinding Binding;
        if (!ParseAxisMapping(AxisLine, Binding))
        {
            OutError = FString::Printf(
                TEXT("Invalid axis mapping in %s: %s"),
                *SourceDescription, *AxisLine);
            OutBindings.Reset();
            return false;
        }
        OutBindings.Add(Binding);
    }

    if (OutBindings.IsEmpty())
    {
        OutError = FString::Printf(
            TEXT("No input mappings were found in %s."), *SourceDescription);
        return false;
    }
    return true;
}

void AddToInputSettings(UInputSettings& InputSettings, const FEchoesInputBinding& Binding)
{
    if (Binding.Kind == EEchoesInputBindingKind::Action)
    {
        InputSettings.AddActionMapping(
            FInputActionKeyMapping(
                Binding.Command,
                Binding.Key,
                Binding.bShift,
                Binding.bCtrl,
                Binding.bAlt,
                Binding.bCmd),
            false);
        return;
    }

    InputSettings.AddAxisMapping(
        FInputAxisKeyMapping(Binding.Command, Binding.Key, Binding.Scale), false);
}

bool ReplaceSnapshot(
    UInputSettings& InputSettings,
    const TArray<FEchoesInputBinding>& Desired,
    FString& OutError)
{
    const TArray<FInputActionKeyMapping> ExistingActions = InputSettings.GetActionMappings();
    const TArray<FInputAxisKeyMapping> ExistingAxes = InputSettings.GetAxisMappings();
    for (const FInputActionKeyMapping& Mapping : ExistingActions)
    {
        InputSettings.RemoveActionMapping(Mapping, false);
    }
    for (const FInputAxisKeyMapping& Mapping : ExistingAxes)
    {
        InputSettings.RemoveAxisMapping(Mapping, false);
    }
    for (const FEchoesInputBinding& Binding : Desired)
    {
        AddToInputSettings(InputSettings, Binding);
    }
    InputSettings.ForceRebuildKeymaps();

    TArray<FEchoesInputBinding> Installed;
    FEchoesInputBindingModel::EnumerateLive(InputSettings, Installed);
    if (HasSameBindingSet(Installed, Desired))
    {
        return true;
    }

    OutError = TEXT("UInputSettings did not retain the requested mapping snapshot.");
    return false;
}

void RestoreSnapshot(
    UInputSettings& InputSettings,
    const TArray<FEchoesInputBinding>& Snapshot)
{
    FString IgnoredError;
    ReplaceSnapshot(InputSettings, Snapshot, IgnoredError);
}
}

bool FEchoesInputBinding::IsValid() const
{
    return !Command.IsNone() && Key.IsValid();
}

bool FEchoesInputBinding::HasSameCommand(const FEchoesInputBinding& Other) const
{
    return Kind == Other.Kind && Command == Other.Command;
}

bool FEchoesInputBinding::HasSameChord(const FEchoesInputBinding& Other) const
{
    if (Key != Other.Key)
    {
        return false;
    }

    // Legacy axis mappings do not carry modifiers. A shifted action on W
    // therefore still competes with the CameraForward W axis mapping.
    if (Kind == EEchoesInputBindingKind::Axis ||
        Other.Kind == EEchoesInputBindingKind::Axis)
    {
        return true;
    }
    return bShift == Other.bShift &&
        bCtrl == Other.bCtrl &&
        bAlt == Other.bAlt &&
        bCmd == Other.bCmd;
}

bool FEchoesInputBinding::operator==(const FEchoesInputBinding& Other) const
{
    return HasSameCommand(Other) && HasSameChord(Other) &&
        FMath::IsNearlyEqual(Scale, Other.Scale);
}

void FEchoesInputBindingModel::EnumerateLive(
    const UInputSettings& InputSettings,
    TArray<FEchoesInputBinding>& OutBindings)
{
    OutBindings.Reset();
    for (const FInputActionKeyMapping& Mapping : InputSettings.GetActionMappings())
    {
        FEchoesInputBinding& Binding = OutBindings.AddDefaulted_GetRef();
        Binding.Kind = EEchoesInputBindingKind::Action;
        Binding.Command = Mapping.ActionName;
        Binding.Key = Mapping.Key;
        Binding.bShift = Mapping.bShift;
        Binding.bCtrl = Mapping.bCtrl;
        Binding.bAlt = Mapping.bAlt;
        Binding.bCmd = Mapping.bCmd;
    }
    for (const FInputAxisKeyMapping& Mapping : InputSettings.GetAxisMappings())
    {
        FEchoesInputBinding& Binding = OutBindings.AddDefaulted_GetRef();
        Binding.Kind = EEchoesInputBindingKind::Axis;
        Binding.Command = Mapping.AxisName;
        Binding.Key = Mapping.Key;
        Binding.Scale = Mapping.Scale;
    }
}

bool FEchoesInputBindingModel::LoadProjectDefaults(
    TArray<FEchoesInputBinding>& OutBindings,
    FString& OutError)
{
    OutBindings.Reset();
    OutError.Reset();

    const FString DefaultInputPath = FPaths::Combine(
        FPaths::ProjectConfigDir(), TEXT("DefaultInput.ini"));
    if (FPaths::FileExists(DefaultInputPath))
    {
        FConfigFile DefaultInput;
        // Read() intentionally treats array-operation prefixes as literal text.
        // DefaultInput.ini uses +ActionMappings/+AxisMappings, so Combine is
        // the required parser path for the authored input contract.
        if (!DefaultInput.Combine(DefaultInputPath, true))
        {
            OutError = FString::Printf(TEXT("Could not parse %s."), *DefaultInputPath);
            return false;
        }
        return LoadBindingsFromConfig(
            DefaultInput, DefaultInputPath, OutBindings, OutError);
    }

    // Cooked builds may omit the loose project default file. UE keeps these
    // static layers apart from SavedLayer, RuntimeChanges, command-line
    // overrides, and InMemoryFile, so this path cannot adopt player bindings
    // as reset defaults.
    const FConfigBranch* InputBranch = GConfig != nullptr
        ? GConfig->FindBranch(TEXT("Input"), GInputIni)
        : nullptr;
    if (InputBranch == nullptr || InputBranch->CombinedStaticLayers.Num() == 0)
    {
        OutError = FString::Printf(
            TEXT("Could not find %s or a static Input config branch."),
            *DefaultInputPath);
        return false;
    }
    return LoadBindingsFromConfig(
        InputBranch->CombinedStaticLayers,
        TEXT("the static Input config branch"),
        OutBindings,
        OutError);
}

FEchoesInputBindingValidationResult FEchoesInputBindingModel::ValidateCandidate(
    const UInputSettings& InputSettings,
    const TArray<FEchoesInputBinding>& DefaultBindings,
    const FEchoesInputBindingCandidate& Candidate)
{
    FEchoesInputBindingValidationResult Result;
    if (DefaultBindings.IsEmpty())
    {
        Result.Code = EEchoesInputBindingValidation::DefaultPolicyUnavailable;
        Result.Detail = TEXT("The default input contract is unavailable.");
        return Result;
    }
    if (!Candidate.Existing.IsValid() || !Candidate.Replacement.IsValid())
    {
        Result.Code = EEchoesInputBindingValidation::InvalidBinding;
        Result.Detail = TEXT("A command name and a valid key are required.");
        return Result;
    }
    if (!Candidate.Existing.HasSameCommand(Candidate.Replacement))
    {
        Result.Code = EEchoesInputBindingValidation::CommandChanged;
        Result.Detail = TEXT("A capture may replace only the selected command.");
        return Result;
    }
    if (Candidate.Existing.Kind == EEchoesInputBindingKind::Axis &&
        !FMath::IsNearlyEqual(Candidate.Existing.Scale, Candidate.Replacement.Scale))
    {
        Result.Code = EEchoesInputBindingValidation::AxisScaleChanged;
        Result.Detail = TEXT("A capture may not change an axis direction or scale.");
        return Result;
    }
    if (Candidate.Existing.Kind == EEchoesInputBindingKind::Axis &&
        (Candidate.Replacement.bShift || Candidate.Replacement.bCtrl ||
            Candidate.Replacement.bAlt || Candidate.Replacement.bCmd))
    {
        Result.Code = EEchoesInputBindingValidation::AxisModifiersUnsupported;
        Result.Detail = TEXT("Axis mappings do not support modifier keys.");
        return Result;
    }
    if (!IsContractCommand(DefaultBindings, Candidate.Existing))
    {
        Result.Code = EEchoesInputBindingValidation::CommandNotInDefaultContract;
        Result.Detail = TEXT("The selected command is not present in DefaultInput.ini.");
        return Result;
    }

    TArray<FEchoesInputBinding> Current;
    EnumerateLive(InputSettings, Current);
    if (!ContainsBinding(Current, Candidate.Existing))
    {
        Result.Code = EEchoesInputBindingValidation::ExistingBindingNotFound;
        Result.Detail = TEXT("The selected binding is no longer installed.");
        return Result;
    }
    if (Candidate.Existing == Candidate.Replacement)
    {
        Result.Code = EEchoesInputBindingValidation::NoChange;
        return Result;
    }

    TArray<FEchoesInputBinding> Prospective = Current;
    Prospective.RemoveSingle(Candidate.Existing);
    Prospective.Add(Candidate.Replacement);

    TArray<FEchoesInputBinding> Occupants;
    GetChordBindings(Prospective, Candidate.Replacement, Occupants);
    if (Occupants.Num() > 1)
    {
        TArray<FEchoesInputBinding> DefaultOccupants;
        GetChordBindings(DefaultBindings, Candidate.Replacement, DefaultOccupants);
        if (!HasSameBindingSet(Occupants, DefaultOccupants))
        {
            Result.Code = EEchoesInputBindingValidation::NewConflict;
            Result.Conflicts = MoveTemp(Occupants);
            Result.Detail = TEXT("That input chord is already assigned outside an approved default overlap.");
            return Result;
        }
    }

    Result.Code = EEchoesInputBindingValidation::Accepted;
    return Result;
}

bool FEchoesInputBindingModel::ApplyCandidate(
    UInputSettings& InputSettings,
    const TArray<FEchoesInputBinding>& DefaultBindings,
    const FEchoesInputBindingCandidate& Candidate,
    EEchoesInputBindingPersistence Persistence,
    FEchoesInputBindingValidationResult& OutValidation,
    FString& OutError)
{
    OutError.Reset();
    OutValidation = ValidateCandidate(InputSettings, DefaultBindings, Candidate);
    if (!OutValidation.IsAccepted())
    {
        OutError = OutValidation.Detail;
        return false;
    }
    if (OutValidation.Code == EEchoesInputBindingValidation::NoChange)
    {
        return true;
    }

    TArray<FEchoesInputBinding> Before;
    EnumerateLive(InputSettings, Before);
    TArray<FEchoesInputBinding> Desired = Before;
    Desired.RemoveSingle(Candidate.Existing);
    Desired.Add(Candidate.Replacement);
    if (!ReplaceSnapshot(InputSettings, Desired, OutError))
    {
        RestoreSnapshot(InputSettings, Before);
        return false;
    }
    if (Persistence == EEchoesInputBindingPersistence::Persist)
    {
        InputSettings.SaveKeyMappings();
    }
    return true;
}

bool FEchoesInputBindingModel::ResetToProjectDefaults(
    UInputSettings& InputSettings,
    EEchoesInputBindingPersistence Persistence,
    FString& OutError)
{
    TArray<FEchoesInputBinding> Defaults;
    if (!LoadProjectDefaults(Defaults, OutError))
    {
        return false;
    }

    TArray<FEchoesInputBinding> Before;
    EnumerateLive(InputSettings, Before);
    if (!ReplaceSnapshot(InputSettings, Defaults, OutError))
    {
        RestoreSnapshot(InputSettings, Before);
        return false;
    }
    if (Persistence == EEchoesInputBindingPersistence::Persist)
    {
        InputSettings.SaveKeyMappings();
    }
    return true;
}
