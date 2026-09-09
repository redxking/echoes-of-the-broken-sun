// Author: Angelis Pseftis
#include "EchoesPlayerController.h"
#include "EchoesInputBindingModel.h"
#include "EchoesInputPrompt.h"
#include "EchoesGameUserSettings.h"
#include "GameFramework/InputSettings.h"

namespace
{
FText BindingName(const FEchoesInputBinding& Binding)
{
    if (Binding.Command == TEXT("RestartScenario"))
        return NSLOCTEXT("EchoesControls", "RepairOrRestart", "Repair target / restart after results");
    if (Binding.Command == TEXT("CameraForward"))
        return Binding.Scale < 0
            ? NSLOCTEXT("EchoesControls", "PanBack", "Pan camera backward")
            : NSLOCTEXT("EchoesControls", "PanForward", "Pan camera forward");
    if (Binding.Command == TEXT("CameraRight"))
        return Binding.Scale < 0
            ? NSLOCTEXT("EchoesControls", "PanLeft", "Pan camera left")
            : NSLOCTEXT("EchoesControls", "PanRight", "Pan camera right");
    FString Name = FName::NameToDisplayString(Binding.Command.ToString(), false);
    if (Binding.Kind == EEchoesInputBindingKind::Axis)
        Name += Binding.Scale < 0 ? TEXT(" (negative direction)") : TEXT(" (positive direction)");
    return FText::FromString(Name);
}
int32 BindingSection(const FEchoesInputBinding& Row)
{
    const FString Name = Row.Command.ToString();
    if (Row.Kind == EEchoesInputBindingKind::Axis || Name.StartsWith(TEXT("Camera"))) return 0;
    if (Name.Contains(TEXT("Select")) || Name.Contains(TEXT("ControlGroup")) || Name.StartsWith(TEXT("CycleOwned")) || Name.Contains(TEXT("KeyboardTarget"))) return 1;
    if (Name == TEXT("RestartScenario")) return 2;
    if (Name.Contains(TEXT("Order")) || Name.Contains(TEXT("AtCursor")) || Name == TEXT("CycleFormation")) return 2;
    if (Name.StartsWith(TEXT("Build")) || Name.StartsWith(TEXT("Produce")) || Name.Contains(TEXT("Production")) || Name.Contains(TEXT("Research")) || Name.Contains(TEXT("Technology"))) return 3;
    if (Name.Contains(TEXT("Campaign")) || Name.Contains(TEXT("Online")) || Name.Contains(TEXT("Scenario")) || Name.Contains(TEXT("Pause")) || Name.Contains(TEXT("Checkpoint")) || Name.Contains(TEXT("PlayableFaction"))) return 5;
    return 4;
}
FText SectionLabel(int32 Section)
{
    switch (Section)
    {
    case 0: return NSLOCTEXT("EchoesControls", "CameraSection", "Camera");
    case 1: return NSLOCTEXT("EchoesControls", "SelectionSection", "Selection and groups");
    case 2: return NSLOCTEXT("EchoesControls", "OrdersSection", "Orders");
    case 3: return NSLOCTEXT("EchoesControls", "ProductionSection", "Construction and production");
    case 4: return NSLOCTEXT("EchoesControls", "AbilitiesSection", "Abilities and world interactions");
    default: return NSLOCTEXT("EchoesControls", "CampaignSection", "Campaign and match controls");
    }
}
TArray<FEchoesInputBinding> ControlRows()
{
    TArray<FEchoesInputBinding> Rows;
    FEchoesInputBindingModel::EnumerateLive(*GetDefault<UInputSettings>(), Rows);
    return Rows;
}
}

FEchoesShellView AEchoesPlayerController::BuildControlsShellView() const
{
    FEchoesShellView View;
    View.Screen = PlayerFlow.Current();
    View.Eyebrow = NSLOCTEXT("EchoesControls", "Eyebrow", "COMMAND SETTINGS");
    if (const auto* Settings = UEchoesGameUserSettings::Get())
    {
        View.Scale = Settings->GetHudScale();
        View.bHighContrast = Settings->IsHighContrastHudEnabled();
    }
    View.Status = FText::FromString(ControlBindingMessage);
    const auto Rows = ControlRows();
    if (View.Screen == EEchoesShellScreen::ControlCapture)
    {
        View.Title = NSLOCTEXT("EchoesControls", "CaptureTitle", "Choose a new binding");
        View.Body = PendingControlBinding.IsSet()
            ? FText::Format(NSLOCTEXT("EchoesControls", "CaptureBody", "{0}\nPress a key or mouse button, or move the mouse wheel. Hold any modifiers with the key. Escape cancels. Conflicting bindings are rejected before applying."), BindingName(PendingControlBinding.GetValue()))
            : NSLOCTEXT("EchoesControls", "MissingBinding", "This binding is no longer available. Return to Controls and choose it again.");
        View.Buttons.Add({NSLOCTEXT("EchoesControls", "Cancel", "Cancel"), EEchoesShellAction::CancelBinding});
        return View;
    }
    View.Title = NSLOCTEXT("EchoesControls", "Title", "Controls");
    View.Body = NSLOCTEXT("EchoesControls", "Body", "Choose a command to change its binding. Alternate bindings are listed separately. Menu navigation uses Tab, arrows, Enter and Escape. Home and End jump to the first and last control.");
    for (int32 Section = 0; Section < 6; ++Section)
    {
        for (int32 Index = 0; Index < Rows.Num(); ++Index)
        {
            const auto& Row = Rows[Index];
            if (BindingSection(Row) != Section) continue;
            View.Buttons.Add({FText::Format(NSLOCTEXT("EchoesControls", "Row", "{0} — {1}"), BindingName(Row),
                FEchoesInputPrompt::Chord(Row.Key, Row.bShift, Row.bCtrl, Row.bAlt, Row.bCmd)),
                EEchoesShellAction::EditBinding, Index, true, SectionLabel(Section)});
        }
    }
    View.Buttons.Add({NSLOCTEXT("EchoesControls", "Reset", "Restore default bindings…"), EEchoesShellAction::ResetBindings});
    View.Buttons.Add({NSLOCTEXT("EchoesControls", "Back", "Back"), EEchoesShellAction::Back});
    return View;
}

bool AEchoesPlayerController::IsCapturingControlBinding() const
{
    return PlayerFlow.Current() == EEchoesShellScreen::ControlCapture;
}

bool AEchoesPlayerController::HandleControlsShellAction(EEchoesShellAction Action, int32 Argument)
{
    if (Action == EEchoesShellAction::OpenControls)
    {
        ControlBindingMessage.Reset();
        PendingControlBindingIndex = INDEX_NONE;
        PendingControlBinding.Reset();
        PlayerFlow.Push(EEchoesShellScreen::Controls);
        return true;
    }
    if (Action == EEchoesShellAction::EditBinding && PlayerFlow.Current() == EEchoesShellScreen::Controls)
    {
        const auto Rows = ControlRows();
        if (Rows.IsValidIndex(Argument))
        {
            PendingControlBindingIndex = Argument;
            PendingControlBinding = Rows[Argument];
            ControlBindingMessage.Reset();
            PlayerFlow.Push(EEchoesShellScreen::ControlCapture);
        }
        return true;
    }
    if ((Action == EEchoesShellAction::CancelBinding || Action == EEchoesShellAction::Back) && IsCapturingControlBinding())
    {
        PendingControlBindingIndex = INDEX_NONE;
        PendingControlBinding.Reset();
        ControlBindingMessage.Reset();
        PlayerFlow.Back();
        return true;
    }
    return false;
}

void AEchoesPlayerController::ApplyConfirmedDefaultBindings()
{
    if (PlayerFlow.Current() != EEchoesShellScreen::Controls) return;
    FString Error;
    const bool bApplied = FEchoesInputBindingModel::ResetToProjectDefaults(*GetMutableDefault<UInputSettings>(), EEchoesInputBindingPersistence::Persist, Error);
    ControlBindingMessage = bApplied ? NSLOCTEXT("EchoesControls", "DefaultsApplied", "Default bindings applied.").ToString() : Error;
}

void AEchoesPlayerController::CaptureControlBinding(const FKey& Key, bool bShift, bool bCtrl, bool bAlt, bool bCmd)
{
    if (!IsCapturingControlBinding()) return;
#if PLATFORM_MAC
    // Slate reports the physical keys, but on Mac UPlayerInput::IsCtrlPressed
    // answers for Command and IsCmdPressed answers for Control. Storing the
    // physical state verbatim would save a Command chord that only fires on
    // Control. Swap here, at the single funnel, so a captured chord replays as
    // the player pressed it.
    Swap(bCtrl, bCmd);
#endif
    if (Key == EKeys::Escape)
    {
        HandleControlsShellAction(EEchoesShellAction::CancelBinding, 0);
        RefreshShell();
        return;
    }
    if (!Key.IsValid() || Key.IsModifierKey()) return;
    if (!PendingControlBinding.IsSet()) return;
    FEchoesInputBindingCandidate Candidate;
    Candidate.Existing = PendingControlBinding.GetValue();
    Candidate.Replacement = Candidate.Existing;
    Candidate.Replacement.Key = Key;
    Candidate.Replacement.bShift = bShift;
    Candidate.Replacement.bCtrl = bCtrl;
    Candidate.Replacement.bAlt = bAlt;
    Candidate.Replacement.bCmd = bCmd;
    TArray<FEchoesInputBinding> Defaults;
    FString Error;
    if (!FEchoesInputBindingModel::LoadProjectDefaults(Defaults, Error))
    {
        ControlBindingMessage = Error;
        RefreshShell();
        return;
    }
    FEchoesInputBindingValidationResult Validation;
    if (FEchoesInputBindingModel::ApplyCandidate(*GetMutableDefault<UInputSettings>(), Defaults, Candidate,
        EEchoesInputBindingPersistence::Persist, Validation, Error))
    {
        ControlBindingMessage = FText::Format(NSLOCTEXT("EchoesControls", "Applied", "{0}: {1} applied."),
            BindingName(Candidate.Replacement), FEchoesInputPrompt::Chord(Key,bShift,bCtrl,bAlt,bCmd)).ToString();
        PendingControlBindingIndex = INDEX_NONE;
        PendingControlBinding.Reset();
        PlayerFlow.Back();
    }
    else
    {
        ControlBindingMessage = Error.IsEmpty() ? Validation.Detail : Error;
        for (const auto& Conflict : Validation.Conflicts)
            ControlBindingMessage += TEXT("\n") + BindingName(Conflict).ToString();
    }
    RefreshShell();
}
