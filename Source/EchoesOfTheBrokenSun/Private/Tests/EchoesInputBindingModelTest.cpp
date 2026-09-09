// Author: Angelis Pseftis
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesInputBindingModel.h"
#include "GameFramework/InputSettings.h"

namespace
{
UInputSettings* MakeTransientInputSettings(const TArray<FEchoesInputBinding>& Bindings)
{
    UInputSettings* Settings = NewObject<UInputSettings>(
        GetTransientPackage(), NAME_None, RF_Transient);
    // Config classes may copy the CDO's mappings when constructed. Clear that
    // transient copy so this fixture contains only the supplied snapshot.
    const TArray<FInputActionKeyMapping> ExistingActions = Settings->GetActionMappings();
    const TArray<FInputAxisKeyMapping> ExistingAxes = Settings->GetAxisMappings();
    for (const FInputActionKeyMapping& Mapping : ExistingActions)
    {
        Settings->RemoveActionMapping(Mapping, false);
    }
    for (const FInputAxisKeyMapping& Mapping : ExistingAxes)
    {
        Settings->RemoveAxisMapping(Mapping, false);
    }
    for (const FEchoesInputBinding& Binding : Bindings)
    {
        if (Binding.Kind == EEchoesInputBindingKind::Action)
        {
            Settings->AddActionMapping(
                FInputActionKeyMapping(
                    Binding.Command, Binding.Key, Binding.bShift, Binding.bCtrl,
                    Binding.bAlt, Binding.bCmd),
                false);
        }
        else
        {
            Settings->AddAxisMapping(
                FInputAxisKeyMapping(Binding.Command, Binding.Key, Binding.Scale), false);
        }
    }
    return Settings;
}

const FEchoesInputBinding* FindAction(
    const TArray<FEchoesInputBinding>& Bindings,
    FName Command)
{
    return Bindings.FindByPredicate([Command](const FEchoesInputBinding& Binding)
    {
        return Binding.Kind == EEchoesInputBindingKind::Action && Binding.Command == Command;
    });
}

const FEchoesInputBinding* FindAxis(
    const TArray<FEchoesInputBinding>& Bindings,
    FName Command)
{
    return Bindings.FindByPredicate([Command](const FEchoesInputBinding& Binding)
    {
        return Binding.Kind == EEchoesInputBindingKind::Axis && Binding.Command == Command;
    });
}

bool SameSet(const TArray<FEchoesInputBinding>& Left, const TArray<FEchoesInputBinding>& Right)
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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesInputBindingModelTest,
    "Echoes.Runtime.Controls.BindingModel",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesInputBindingModelTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    TArray<FEchoesInputBinding> Defaults;
    FString Error;
    if (!TestTrue(TEXT("The model reads the authored DefaultInput.ini contract"),
                  FEchoesInputBindingModel::LoadProjectDefaults(Defaults, Error)))
    {
        AddError(Error);
        return false;
    }
    TestTrue(TEXT("The default contract contains actions"),
             Defaults.ContainsByPredicate([](const FEchoesInputBinding& Binding)
             { return Binding.Kind == EEchoesInputBindingKind::Action; }));
    TestTrue(TEXT("The default contract contains axes"),
             Defaults.ContainsByPredicate([](const FEchoesInputBinding& Binding)
             { return Binding.Kind == EEchoesInputBindingKind::Axis; }));
    TestTrue(TEXT("The default file projects Select on left mouse"),
             Defaults.ContainsByPredicate([](const FEchoesInputBinding& Binding)
             {
                 return Binding.Kind == EEchoesInputBindingKind::Action &&
                     Binding.Command == TEXT("Select") &&
                     Binding.Key == EKeys::LeftMouseButton;
             }));
    TestTrue(TEXT("The default file projects CameraForward on W"),
             Defaults.ContainsByPredicate([](const FEchoesInputBinding& Binding)
             {
                 return Binding.Kind == EEchoesInputBindingKind::Axis &&
                     Binding.Command == TEXT("CameraForward") &&
                     Binding.Key == EKeys::W &&
                     FMath::IsNearlyEqual(Binding.Scale, 1.0f);
             }));

    // This is a transient object only. The test never obtains the mutable
    // config default and never asks the model to persist a key mapping.
    UInputSettings* Settings = MakeTransientInputSettings(Defaults);
    if (!TestNotNull(TEXT("A transient input settings fixture exists"), Settings))
    {
        return false;
    }

    const FEchoesInputBinding* Select = FindAction(Defaults, TEXT("Select"));
    const FEchoesInputBinding* ContextOrder = FindAction(Defaults, TEXT("ContextOrder"));
    if (!TestNotNull(TEXT("Select is an authored input command"), Select) ||
        !TestNotNull(TEXT("Context order is an authored input command"), ContextOrder))
    {
        return false;
    }

    FEchoesInputBindingCandidate Collision;
    Collision.Existing = *Select;
    Collision.Replacement = *Select;
    Collision.Replacement.Key = ContextOrder->Key;
    Collision.Replacement.bShift = ContextOrder->bShift;
    Collision.Replacement.bCtrl = ContextOrder->bCtrl;
    Collision.Replacement.bAlt = ContextOrder->bAlt;
    Collision.Replacement.bCmd = ContextOrder->bCmd;

    const FEchoesInputBindingValidationResult CollisionResult =
        FEchoesInputBindingModel::ValidateCandidate(*Settings, Defaults, Collision);
    TestEqual(TEXT("A newly introduced shared chord is rejected before mutation"),
              CollisionResult.Code, EEchoesInputBindingValidation::NewConflict);
    TArray<FEchoesInputBinding> Unchanged;
    FEchoesInputBindingModel::EnumerateLive(*Settings, Unchanged);
    TestTrue(TEXT("Rejected capture leaves the transient fixture unchanged"),
             SameSet(Unchanged, Defaults));

    const FEchoesInputBinding* CameraForward = FindAxis(Defaults, TEXT("CameraForward"));
    if (!TestNotNull(TEXT("Camera forward is an authored axis command"), CameraForward))
    {
        return false;
    }
    FEchoesInputBindingCandidate AxisModifier;
    AxisModifier.Existing = *CameraForward;
    AxisModifier.Replacement = *CameraForward;
    AxisModifier.Replacement.bShift = true;
    const FEchoesInputBindingValidationResult AxisModifierResult =
        FEchoesInputBindingModel::ValidateCandidate(*Settings, Defaults, AxisModifier);
    TestEqual(TEXT("Axis candidates reject unsupported modifiers"),
              AxisModifierResult.Code,
              EEchoesInputBindingValidation::AxisModifiersUnsupported);
    FEchoesInputBindingValidationResult AxisApplyResult;
    Error.Reset();
    TestFalse(TEXT("Unsupported axis modifiers never mutate the fixture"),
              FEchoesInputBindingModel::ApplyCandidate(
                  *Settings, Defaults, AxisModifier,
                  EEchoesInputBindingPersistence::Transient,
                  AxisApplyResult, Error));
    TestEqual(TEXT("Rejected apply returns the validation detail"),
              Error, AxisApplyResult.Detail);

    FEchoesInputBindingCandidate ShiftedAxisConflict;
    ShiftedAxisConflict.Existing = *Select;
    ShiftedAxisConflict.Replacement = *Select;
    ShiftedAxisConflict.Replacement.Key = CameraForward->Key;
    ShiftedAxisConflict.Replacement.bShift = true;
    const FEchoesInputBindingValidationResult ShiftedAxisConflictResult =
        FEchoesInputBindingModel::ValidateCandidate(
            *Settings, Defaults, ShiftedAxisConflict);
    TestEqual(TEXT("A modified action still conflicts with an axis on the same key"),
              ShiftedAxisConflictResult.Code,
              EEchoesInputBindingValidation::NewConflict);
    FEchoesInputBindingModel::EnumerateLive(*Settings, Unchanged);
    TestTrue(TEXT("Axis conflict rejection leaves the transient fixture unchanged"),
             SameSet(Unchanged, Defaults));

    // ContinueCampaign and ChoosePreserve intentionally share C in the
    // authored default contract. Moving one away and then restoring it must
    // preserve that exact approved overlap, without generalizing it to a new
    // context policy.
    const FEchoesInputBinding* ContinueCampaign = FindAction(Defaults, TEXT("ContinueCampaign"));
    if (!TestNotNull(TEXT("Continue campaign is an authored input command"), ContinueCampaign))
    {
        return false;
    }
    FEchoesInputBindingCandidate MoveAway;
    MoveAway.Existing = *ContinueCampaign;
    MoveAway.Replacement = *ContinueCampaign;
    // This must be a key Config/DefaultInput.ini leaves unbound, or the
    // "non-conflicting" premise fails. F12 was free until the campaign map
    // moved there off the SPEC-CTL-014 selector keys.
    MoveAway.Replacement.Key = EKeys::NumPadZero;
    FEchoesInputBindingValidationResult MoveAwayResult;
    if (!TestTrue(TEXT("A non-conflicting replacement is accepted"),
                  FEchoesInputBindingModel::ApplyCandidate(
                      *Settings, Defaults, MoveAway,
                      EEchoesInputBindingPersistence::Transient,
                      MoveAwayResult, Error)))
    {
        AddError(Error);
        return false;
    }
    TArray<FEchoesInputBinding> AlteredLiveBindings;
    FEchoesInputBindingModel::EnumerateLive(*Settings, AlteredLiveBindings);
    TestFalse(TEXT("The transient live fixture now differs from the default contract"),
              SameSet(AlteredLiveBindings, Defaults));
    TArray<FEchoesInputBinding> ReloadedDefaults;
    Error.Reset();
    TestTrue(TEXT("Default loading never adopts altered live bindings"),
             FEchoesInputBindingModel::LoadProjectDefaults(ReloadedDefaults, Error) &&
                 SameSet(ReloadedDefaults, Defaults));
    FEchoesInputBindingCandidate RestoreApprovedOverlap;
    RestoreApprovedOverlap.Existing = MoveAway.Replacement;
    RestoreApprovedOverlap.Replacement = *ContinueCampaign;
    const FEchoesInputBindingValidationResult RestoreResult =
        FEchoesInputBindingModel::ValidateCandidate(
            *Settings, Defaults, RestoreApprovedOverlap);
    TestEqual(TEXT("The exact authored shared chord remains approved"),
              RestoreResult.Code, EEchoesInputBindingValidation::Accepted);

    FEchoesInputBindingValidationResult RestoreApplyResult;
    if (!TestTrue(TEXT("The approved overlap is restored atomically"),
                  FEchoesInputBindingModel::ApplyCandidate(
                      *Settings, Defaults, RestoreApprovedOverlap,
                      EEchoesInputBindingPersistence::Transient,
                      RestoreApplyResult, Error)))
    {
        AddError(Error);
        return false;
    }

    // Reset must reload actual project defaults rather than a test-owned list.
    if (!TestTrue(TEXT("Reset restores DefaultInput.ini through the transient fixture"),
                  FEchoesInputBindingModel::ResetToProjectDefaults(
                      *Settings, EEchoesInputBindingPersistence::Transient, Error)))
    {
        AddError(Error);
        return false;
    }
    TArray<FEchoesInputBinding> AfterReset;
    FEchoesInputBindingModel::EnumerateLive(*Settings, AfterReset);
    TestTrue(TEXT("Reset recreates the complete authored input contract"),
             SameSet(AfterReset, Defaults));
    return true;
}

#endif
