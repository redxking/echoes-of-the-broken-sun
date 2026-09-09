// Copyright Echoes of the Broken Sun. All Rights Reserved.
// Author: Angelis Pseftis

#if WITH_DEV_AUTOMATION_TESTS

#include "EchoesInputBindingModel.h"
#include "EchoesPlayerController.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesTestSaveEnvironment.h"

#include "CoreGlobals.h"
#include "Engine/World.h"
#include "GameFramework/InputSettings.h"
#include "HAL/FileManager.h"
#include "InputCoreTypes.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Tests/AutomationCommon.h"

namespace
{
struct FDiskImage final
{
    TArray<uint8> Bytes;
    bool bExisted = false;
};

bool CaptureDiskImage(const FString& Path, FDiskImage& OutImage)
{
    OutImage = FDiskImage();
    OutImage.bExisted = IFileManager::Get().FileExists(*Path);
    return !OutImage.bExisted || FFileHelper::LoadFileToArray(OutImage.Bytes, *Path);
}

bool SameDiskImage(const FDiskImage& Left, const FDiskImage& Right)
{
    return Left.bExisted == Right.bExisted && Left.Bytes == Right.Bytes;
}

bool SameBindingSet(
    const TArray<FEchoesInputBinding>& Left,
    const TArray<FEchoesInputBinding>& Right)
{
    if (Left.Num() != Right.Num()) return false;
    TArray<FEchoesInputBinding> Unmatched = Right;
    for (const FEchoesInputBinding& Binding : Left)
    {
        if (Unmatched.RemoveSingle(Binding) == 0) return false;
    }
    return Unmatched.IsEmpty();
}

TArray<FEchoesInputBinding> LiveBindings(const UInputSettings& InputSettings)
{
    TArray<FEchoesInputBinding> Bindings;
    FEchoesInputBindingModel::EnumerateLive(InputSettings, Bindings);
    return Bindings;
}

constexpr TCHAR PersistedInputSettingsSection[] =
    TEXT("/Script/Engine.InputSettings");

bool ParsePersistedAction(
    const FString& Text,
    FEchoesInputBinding& OutBinding)
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

bool ParsePersistedAxis(
    const FString& Text,
    FEchoesInputBinding& OutBinding)
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

bool ReadPersistedBindings(
    const FConfigFile& ReloadInputBase,
    const FConfigCommandStream& CommandLineOverrides,
    const FString& Path,
    TArray<FEchoesInputBinding>& OutBindings,
    bool& bOutHasInputSettingsSection,
    FString& OutError)
{
    OutBindings.Reset();
    bOutHasInputSettingsSection = false;
    OutError.Reset();

    // UE saves a delta (including array commands), not a full copy of live
    // settings. Reconstruct startup from the retained pre-saved hierarchy,
    // the physical saved file, and command-line overrides in that order.
    // Never borrow live InMemoryFile/RuntimeChanges as proof of disk.
    FConfigFile DiskConfig = ReloadInputBase;
    if (IFileManager::Get().FileExists(*Path))
    {
        FString SavedText;
        if (!FFileHelper::LoadFileToString(SavedText, *Path))
        {
            OutError = FString::Printf(TEXT("Could not read persisted input config: %s"), *Path);
            return false;
        }
        FConfigFile RawDisk;
        RawDisk.ProcessInputFileContents(SavedText, Path);
        bOutHasInputSettingsSection = RawDisk.Contains(PersistedInputSettingsSection);
        // UE's saved stream parser is not exported from Core. Its public
        // ProcessInputFileContents counterpart also parses with symbol commands
        // disabled, preserving repeated keys as ArrayAdd entries. Recreate that
        // stream and let UE ApplyFile perform saved-file first-key replacement.
        FConfigCommandStream SavedLayer;
        for (const auto& Section : RawDisk)
        {
            auto& SavedSection = SavedLayer.FindOrAdd(Section.Key);
            for (const auto& Value : Section.Value)
            {
                SavedSection.Add(Value.Key, FConfigValue(
                    Value.Value.GetSavedValue(), FConfigValue::EValueType::ArrayAdd));
            }
        }
        SavedLayer.bIsSavedConfigFile = true;
        DiskConfig.ApplyFile(&SavedLayer);
    }

    DiskConfig.ApplyFile(&CommandLineOverrides);
    TArray<FString> ActionLines;
    TArray<FString> AxisLines;
    DiskConfig.GetArray(
        PersistedInputSettingsSection,
        TEXT("ActionMappings"),
        ActionLines);
    DiskConfig.GetArray(
        PersistedInputSettingsSection,
        TEXT("AxisMappings"),
        AxisLines);

    for (const FString& ActionLine : ActionLines)
    {
        FEchoesInputBinding Binding;
        if (!ParsePersistedAction(ActionLine, Binding))
        {
            OutError = FString::Printf(
                TEXT("Invalid persisted action mapping: %s"),
                *ActionLine);
            OutBindings.Reset();
            return false;
        }
        OutBindings.Add(Binding);
    }
    for (const FString& AxisLine : AxisLines)
    {
        FEchoesInputBinding Binding;
        if (!ParsePersistedAxis(AxisLine, Binding))
        {
            OutError = FString::Printf(
                TEXT("Invalid persisted axis mapping: %s"),
                *AxisLine);
            OutBindings.Reset();
            return false;
        }
        OutBindings.Add(Binding);
    }
    return true;
}

bool IsChordFree(
    const TArray<FEchoesInputBinding>& Bindings,
    const FKey& Key)
{
    FEchoesInputBinding Probe;
    Probe.Key = Key;
    return !Bindings.ContainsByPredicate(
        [&Probe](const FEchoesInputBinding& Binding)
        {
            return Binding.HasSameChord(Probe);
        });
}

FKey FindFreeActionKey(
    const TArray<FEchoesInputBinding>& Bindings,
    const FKey& Excluded = FKey())
{
    const FKey Preferred(TEXT("F13"));
    if (Preferred.IsValid() && Preferred != Excluded &&
        Preferred.IsBindableToActions() && IsChordFree(Bindings, Preferred))
    {
        return Preferred;
    }

    TArray<FKey> Candidates;
    EKeys::GetAllKeys(Candidates);
    for (const FKey& Key : Candidates)
    {
        if (!Key.IsValid() || Key == Excluded || Key == EKeys::Escape ||
            Key == EKeys::AnyKey ||
            Key.IsModifierKey() || !Key.IsBindableToActions() ||
            Key.IsAxis1D() || Key.IsAxis2D() || Key.IsAxis3D() ||
            Key.IsTouch() || Key.IsVirtual() ||
            Key.GetMenuCategory() != EKeys::NAME_KeyboardCategory)
        {
            continue;
        }
        if (IsChordFree(Bindings, Key)) return Key;
    }
    return FKey();
}

void RemoveBinding(UInputSettings& InputSettings, const FEchoesInputBinding& Binding)
{
    if (Binding.Kind == EEchoesInputBindingKind::Action)
    {
        InputSettings.RemoveActionMapping(
            FInputActionKeyMapping(
                Binding.Command,
                Binding.Key,
                Binding.bShift,
                Binding.bCtrl,
                Binding.bAlt,
                Binding.bCmd),
            false);
    }
    else
    {
        InputSettings.RemoveAxisMapping(
            FInputAxisKeyMapping(Binding.Command, Binding.Key, Binding.Scale),
            false);
    }
    InputSettings.ForceRebuildKeymaps();
}

void AddBinding(UInputSettings& InputSettings, const FEchoesInputBinding& Binding)
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
    }
    else
    {
        InputSettings.AddAxisMapping(
            FInputAxisKeyMapping(Binding.Command, Binding.Key, Binding.Scale),
            false);
    }
    InputSettings.ForceRebuildKeymaps();
}

/**
 * Protects the process-global legacy input settings before any persistence
 * path is exercised. The test refuses to mutate unless UE's actual Input
 * branch resolves beneath the launcher's isolated -UserDir.
 */
class FScopedControlsInputConfig final
{
public:
    explicit FScopedControlsInputConfig(FAutomationTestBase& InTest)
        : Test(InTest), PriorCommandLine(FCommandLine::Get()), InputKey(GInputIni)
    {
        InputSettings = GetMutableDefault<UInputSettings>();
        const FConfigBranch* InputBranch =
            GConfig->FindBranch(TEXT("Input"), InputKey);
        InputIni = InputBranch ? InputBranch->IniPath : FString();

        FString UserDir;
        const FString FullInputIni = FPaths::ConvertRelativePathToFull(InputIni);
        if (InputSettings == nullptr || InputIni.IsEmpty() ||
            !FParse::Value(FCommandLine::Get(), TEXT("UserDir="), UserDir) ||
            !FPaths::IsUnderDirectory(
                FullInputIni,
                FPaths::ConvertRelativePathToFull(UserDir)))
        {
            Test.AddError(FString::Printf(
                TEXT("Controls persistence requires Input inside the isolated UserDir. key=%s path=%s userDir=%s"),
                *InputKey,
                *InputIni,
                *UserDir));
            return;
        }

        FConfigFile ReloadBase;
        if (!InputBranch->CombinedStaticLayers.IsEmpty())
        {
            ReloadBase = InputBranch->CombinedStaticLayers;
            for (FConfigBranch::DynamicLayerList::TIterator Node(
                    InputBranch->DynamicLayers.GetHead()); Node; ++Node)
            {
                ReloadBase.ApplyFile(*Node);
            }
        }
        else
        {
            // NoReplay does not retain the separate static layers. Its final
            // hierarchy still excludes saved/runtime edits, but includes CLI
            // overrides, which cannot safely be moved before the saved layer.
            if (!InputBranch->CommandLineOverrides.IsEmpty())
            {
                Test.AddError(TEXT("Cannot reconstruct no-replay Input with command-line overrides."));
                return;
            }
            ReloadBase = InputBranch->FinalCombinedLayers;
        }
        if (ReloadBase.IsEmpty())
        {
            Test.AddError(TEXT("Controls persistence has no retained pre-saved Input hierarchy."));
            return;
        }
        ReloadInputBase = MakeUnique<FConfigFile>(ReloadBase);
        InputCommandLineOverrides = MakeUnique<FConfigCommandStream>(InputBranch->CommandLineOverrides);
        Test.AddInfo(FString::Printf(TEXT("Input reload baseline: static sections=%d, final sections=%d, command-line sections=%d"),
            InputBranch->CombinedStaticLayers.Num(), InputBranch->FinalCombinedLayers.Num(),
            InputBranch->CommandLineOverrides.Num()));

        OriginalActions = InputSettings->GetActionMappings();
        OriginalAxes = InputSettings->GetAxisMappings();
        OriginalFile.bExisted = IFileManager::Get().FileExists(*InputIni);
        if (OriginalFile.bExisted &&
            !FFileHelper::LoadFileToArray(OriginalFile.Bytes, *InputIni))
        {
            Test.AddError(TEXT("Could not preserve the isolated input config."));
            return;
        }
        if (const FConfigFile* Cached = GConfig->FindConfigFile(InputKey))
        {
            OriginalCache = MakeUnique<FConfigFile>(*Cached);
        }

        FCommandLine::Append(TEXT(" -MultiprocessSaveConfig"));
        bReady = true;
    }

    ~FScopedControlsInputConfig()
    {
        FCommandLine::Set(*PriorCommandLine);
        if (!bReady) return;

        const TArray<FInputActionKeyMapping> CurrentActions =
            InputSettings->GetActionMappings();
        const TArray<FInputAxisKeyMapping> CurrentAxes =
            InputSettings->GetAxisMappings();
        for (const FInputActionKeyMapping& Mapping : CurrentActions)
        {
            InputSettings->RemoveActionMapping(Mapping, false);
        }
        for (const FInputAxisKeyMapping& Mapping : CurrentAxes)
        {
            InputSettings->RemoveAxisMapping(Mapping, false);
        }
        for (const FInputActionKeyMapping& Mapping : OriginalActions)
        {
            InputSettings->AddActionMapping(Mapping, false);
        }
        for (const FInputAxisKeyMapping& Mapping : OriginalAxes)
        {
            InputSettings->AddAxisMapping(Mapping, false);
        }
        InputSettings->ForceRebuildKeymaps();

        if (OriginalCache)
        {
            GConfig->SetFile(InputKey, OriginalCache.Get());
        }
        else
        {
            GConfig->Remove(InputKey);
        }
        const bool bDiskRestored = OriginalFile.bExisted
            ? FFileHelper::SaveArrayToFile(OriginalFile.Bytes, *InputIni)
            : IFileManager::Get().Delete(*InputIni, false, true);
        Test.TestTrue(
            TEXT("Isolated input config restored after persistence test"),
            bDiskRestored);
    }

    bool IsReady() const { return bReady; }
    const FString& GetInputIni() const { return InputIni; }
    const FConfigFile& GetReloadInputBase() const { return *ReloadInputBase; }
    const FConfigCommandStream& GetCommandLineOverrides() const { return *InputCommandLineOverrides; }

private:
    FAutomationTestBase& Test;
    FString PriorCommandLine;
    FString InputKey;
    FString InputIni;
    UInputSettings* InputSettings = nullptr;
    TArray<FInputActionKeyMapping> OriginalActions;
    TArray<FInputAxisKeyMapping> OriginalAxes;
    FDiskImage OriginalFile;
    TUniquePtr<FConfigFile> OriginalCache;
    TUniquePtr<FConfigFile> ReloadInputBase;
    TUniquePtr<FConfigCommandStream> InputCommandLineOverrides;
    bool bReady = false;
};
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesControlsPersistenceTest,
    "Echoes.Runtime.UI.ControlsPersistence",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesControlsPersistenceTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    FEchoesScopedTestSaveEnvironment Storage(*this);
    if (!Storage.IsReady()) return false;
    FScopedControlsInputConfig ProtectedInput(*this);
    if (!ProtectedInput.IsReady()) return false;

    UInputSettings* InputSettings = GetMutableDefault<UInputSettings>();
    TArray<FEchoesInputBinding> Defaults;
    FString Error;
    if (!TestTrue(TEXT("Authored default bindings load"),
            FEchoesInputBindingModel::LoadProjectDefaults(Defaults, Error)))
    {
        AddError(Error);
        return false;
    }

    FTestWorldWrapper Wrapper;
    if (!Wrapper.CreateTestWorld(EWorldType::Game)) return false;
    UWorld* World = Wrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge =
        World->GetSubsystem<UEchoesSimulationSubsystem>();
    if (!TestTrue(TEXT("Scenario starts"),
            Bridge != nullptr && Bridge->StartPrototypeScenario()))
    {
        return false;
    }
    AEchoesPlayerController* Controller =
        World->SpawnActor<AEchoesPlayerController>();
    if (!TestNotNull(TEXT("Controls persistence route owner exists"), Controller))
    {
        Bridge->StopPrototypeScenario();
        Wrapper.ForwardErrorMessages(this);
        return false;
    }
    Controller->PresentTitleScreen();
    TestTrue(TEXT("Isolated profile initializes"),
        Controller->InitializePlayerProfile());
    Controller->HandleShellAction(EEchoesShellAction::Options);
    Controller->HandleShellAction(EEchoesShellAction::OpenControls);

    TArray<FEchoesInputBinding> Current = LiveBindings(*InputSettings);
    const int32 AcceptedSourceIndex = Current.IndexOfByPredicate(
        [&Defaults](const FEchoesInputBinding& Binding)
        {
            return Binding.Kind == EEchoesInputBindingKind::Action &&
                Defaults.Contains(Binding);
        });
    const FKey AcceptedKey = FindFreeActionKey(Current);
    if (!TestTrue(TEXT("A contract action and free capture key are available"),
            Current.IsValidIndex(AcceptedSourceIndex) && AcceptedKey.IsValid()))
    {
        Bridge->StopPrototypeScenario();
        Wrapper.ForwardErrorMessages(this);
        return false;
    }

    const FEchoesInputBinding AcceptedSource = Current[AcceptedSourceIndex];
    FEchoesInputBinding AcceptedReplacement = AcceptedSource;
    AcceptedReplacement.Key = AcceptedKey;
    AcceptedReplacement.bShift = false;
    AcceptedReplacement.bCtrl = false;
    AcceptedReplacement.bAlt = false;
    AcceptedReplacement.bCmd = false;
    FDiskImage BeforeAccepted;
    FDiskImage AfterAccepted;
    TestTrue(TEXT("Input config can be read before accepted capture"),
        CaptureDiskImage(ProtectedInput.GetInputIni(), BeforeAccepted));
    Controller->HandleShellAction(
        EEchoesShellAction::EditBinding,
        AcceptedSourceIndex);
    Controller->CaptureControlBinding(
        AcceptedKey, false, false, false, false);
    Current = LiveBindings(*InputSettings);
    TestTrue(TEXT("Accepted capture installs the exact replacement"),
        Current.Contains(AcceptedReplacement) &&
        !Current.Contains(AcceptedSource));
    TestEqual(TEXT("Accepted capture returns to Controls"),
        Controller->GetPlayerFlow().Current(),
        EEchoesShellScreen::Controls);
    TestFalse(TEXT("Accepted capture reports its result"),
        Controller->BuildShellView().Status.IsEmpty());
    TestTrue(TEXT("Input config can be read after accepted capture"),
        CaptureDiskImage(ProtectedInput.GetInputIni(), AfterAccepted));
    TestFalse(TEXT("Accepted capture persists a changed input config"),
        SameDiskImage(BeforeAccepted, AfterAccepted));
    TArray<FEchoesInputBinding> AcceptedDiskBindings;
    bool bAcceptedDiskHasInputSettings = false;
    FString AcceptedDiskError;
    const bool bAcceptedDiskReadable = ReadPersistedBindings(
        ProtectedInput.GetReloadInputBase(),
        ProtectedInput.GetCommandLineOverrides(),
        ProtectedInput.GetInputIni(),
        AcceptedDiskBindings,
        bAcceptedDiskHasInputSettings,
        AcceptedDiskError);
    TestTrue(TEXT("Accepted input config mappings can be parsed from disk"),
        bAcceptedDiskReadable);
    if (!bAcceptedDiskReadable)
    {
        AddError(AcceptedDiskError);
    }
    TestTrue(TEXT("Accepted input config serializes InputSettings"),
        bAcceptedDiskHasInputSettings);
    TestTrue(TEXT("Disk reload restores the accepted replacement"),
        AcceptedDiskBindings.Contains(AcceptedReplacement));
    TestFalse(TEXT("Disk reload excludes the replaced source binding"),
        AcceptedDiskBindings.Contains(AcceptedSource));
    TestTrue(TEXT("Disk-reloaded accepted mapping set matches live mappings"),
        SameBindingSet(AcceptedDiskBindings, Current));

    // A second command cannot take the newly occupied non-default chord.
    const int32 CollisionSourceIndex = Current.IndexOfByPredicate(
        [&AcceptedReplacement](const FEchoesInputBinding& Binding)
        {
            return Binding.Kind == EEchoesInputBindingKind::Action &&
                Binding.Command != AcceptedReplacement.Command;
        });
    if (TestTrue(TEXT("A second action exists for collision validation"),
            Current.IsValidIndex(CollisionSourceIndex)))
    {
        const TArray<FEchoesInputBinding> BeforeCollision = Current;
        FDiskImage CollisionDiskBefore;
        FDiskImage CollisionDiskAfter;
        TestTrue(TEXT("Collision baseline input config is readable"),
            CaptureDiskImage(ProtectedInput.GetInputIni(), CollisionDiskBefore));
        Controller->HandleShellAction(
            EEchoesShellAction::EditBinding,
            CollisionSourceIndex);
        Controller->CaptureControlBinding(
            AcceptedKey, false, false, false, false);
        const TArray<FEchoesInputBinding> AfterCollision =
            LiveBindings(*InputSettings);
        TestTrue(TEXT("Conflicting capture remains in capture mode"),
            Controller->IsCapturingControlBinding());
        TestFalse(TEXT("Conflicting capture reports its rejection"),
            Controller->BuildShellView().Status.IsEmpty());
        TestTrue(TEXT("Conflicting capture leaves mappings unchanged"),
            SameBindingSet(BeforeCollision, AfterCollision));
        TestTrue(TEXT("Collision result input config is readable"),
            CaptureDiskImage(ProtectedInput.GetInputIni(), CollisionDiskAfter));
        TestTrue(TEXT("Conflicting capture performs no persistent write"),
            SameDiskImage(CollisionDiskBefore, CollisionDiskAfter));
        Controller->CaptureControlBinding(
            EKeys::Escape, false, false, false, false);
        FDiskImage CollisionCancelDisk;
        TestTrue(TEXT("Cancelled capture input config is readable"),
            CaptureDiskImage(ProtectedInput.GetInputIni(), CollisionCancelDisk));
        TestTrue(TEXT("Escape cancellation performs no persistent write"),
            SameDiskImage(CollisionDiskAfter, CollisionCancelDisk));
        Current = LiveBindings(*InputSettings);
    }

    // The selected binding's complete identity must still exist at apply time.
    const int32 StaleSourceIndex = Current.IndexOfByPredicate(
        [&AcceptedReplacement](const FEchoesInputBinding& Binding)
        {
            return Binding.Kind == EEchoesInputBindingKind::Action &&
                Binding.Command != AcceptedReplacement.Command;
        });
    const FKey StaleReplacementKey =
        FindFreeActionKey(Current, AcceptedKey);
    if (TestTrue(TEXT("A stable-identity source and free key are available"),
            Current.IsValidIndex(StaleSourceIndex) &&
            StaleReplacementKey.IsValid()))
    {
        const FEchoesInputBinding StaleSource = Current[StaleSourceIndex];
        Controller->HandleShellAction(
            EEchoesShellAction::EditBinding,
            StaleSourceIndex);
        RemoveBinding(*InputSettings, StaleSource);
        const TArray<FEchoesInputBinding> BeforeStaleApply =
            LiveBindings(*InputSettings);
        FDiskImage StaleDiskBefore;
        FDiskImage StaleDiskAfter;
        TestTrue(TEXT("Stale-identity baseline input config is readable"),
            CaptureDiskImage(ProtectedInput.GetInputIni(), StaleDiskBefore));
        Controller->CaptureControlBinding(
            StaleReplacementKey, false, false, false, false);
        TestTrue(TEXT("Removed selected identity is rejected"),
            Controller->IsCapturingControlBinding());
        TestFalse(TEXT("Stale identity rejection is visible"),
            Controller->BuildShellView().Status.IsEmpty());
        TestTrue(TEXT("Stale identity rejection leaves mappings unchanged"),
            SameBindingSet(
                BeforeStaleApply,
                LiveBindings(*InputSettings)));
        TestTrue(TEXT("Stale-identity result input config is readable"),
            CaptureDiskImage(ProtectedInput.GetInputIni(), StaleDiskAfter));
        TestTrue(TEXT("Stale identity rejection performs no persistent write"),
            SameDiskImage(StaleDiskBefore, StaleDiskAfter));
        AddBinding(*InputSettings, StaleSource);
        Controller->CaptureControlBinding(
            EKeys::Escape, false, false, false, false);
        Current = LiveBindings(*InputSettings);
    }

    // Axis direction and scale remain fixed; modifiers cannot be introduced.
    const int32 AxisIndex = Current.IndexOfByPredicate(
        [](const FEchoesInputBinding& Binding)
        {
            return Binding.Kind == EEchoesInputBindingKind::Axis;
        });
    const FKey AxisReplacementKey = FindFreeActionKey(Current, AcceptedKey);
    if (TestTrue(TEXT("An axis and free key are available"),
            Current.IsValidIndex(AxisIndex) && AxisReplacementKey.IsValid()))
    {
        const TArray<FEchoesInputBinding> BeforeAxis = Current;
        FDiskImage AxisDiskBefore;
        FDiskImage AxisDiskAfter;
        TestTrue(TEXT("Axis baseline input config is readable"),
            CaptureDiskImage(ProtectedInput.GetInputIni(), AxisDiskBefore));
        Controller->HandleShellAction(EEchoesShellAction::EditBinding, AxisIndex);
        Controller->CaptureControlBinding(
            AxisReplacementKey, true, false, false, false);
        TestTrue(TEXT("Modified axis chord remains in capture mode"),
            Controller->IsCapturingControlBinding());
        TestFalse(TEXT("Axis modifier rejection is visible"),
            Controller->BuildShellView().Status.IsEmpty());
        TestTrue(TEXT("Axis modifier rejection leaves mappings unchanged"),
            SameBindingSet(BeforeAxis, LiveBindings(*InputSettings)));
        TestTrue(TEXT("Axis result input config is readable"),
            CaptureDiskImage(ProtectedInput.GetInputIni(), AxisDiskAfter));
        TestTrue(TEXT("Axis modifier rejection performs no persistent write"),
            SameDiskImage(AxisDiskBefore, AxisDiskAfter));
        Controller->CaptureControlBinding(
            EKeys::Escape, false, false, false, false);
    }

    // The only UI path to reset is the destructive-action confirmation.
    FDiskImage ResetDiskBefore;
    FDiskImage ResetDiskAfter;
    TestTrue(TEXT("Reset baseline input config is readable"),
        CaptureDiskImage(ProtectedInput.GetInputIni(), ResetDiskBefore));
    Controller->HandleShellAction(EEchoesShellAction::ResetBindings);
    TestEqual(TEXT("Reset waits at confirmation"),
        Controller->GetPlayerFlow().Current(),
        EEchoesShellScreen::Confirmation);
    Controller->HandleShellAction(EEchoesShellAction::Confirm);
    TestEqual(TEXT("Confirmed reset returns to Controls"),
        Controller->GetPlayerFlow().Current(),
        EEchoesShellScreen::Controls);
    TestTrue(TEXT("Confirmed reset matches the authored defaults"),
        SameBindingSet(LiveBindings(*InputSettings), Defaults));
    TestTrue(TEXT("Reset result input config is readable"),
        CaptureDiskImage(ProtectedInput.GetInputIni(), ResetDiskAfter));
    TestFalse(TEXT("Confirmed reset persists the default mapping set"),
        SameDiskImage(ResetDiskBefore, ResetDiskAfter));
    TArray<FEchoesInputBinding> ResetDiskBindings;
    bool bResetDiskHasInputSettings = false;
    FString ResetDiskError;
    const bool bResetDiskReadable = ReadPersistedBindings(
        ProtectedInput.GetReloadInputBase(),
        ProtectedInput.GetCommandLineOverrides(),
        ProtectedInput.GetInputIni(),
        ResetDiskBindings,
        bResetDiskHasInputSettings,
        ResetDiskError);
    TestTrue(TEXT("Reset input config mappings can be parsed from disk"),
        bResetDiskReadable);
    if (!bResetDiskReadable)
    {
        AddError(ResetDiskError);
    }
    TestTrue(TEXT("Reloaded reset config resolves to the authored defaults"),
        SameBindingSet(ResetDiskBindings, Defaults));

    Bridge->StopPrototypeScenario();
    Wrapper.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif
