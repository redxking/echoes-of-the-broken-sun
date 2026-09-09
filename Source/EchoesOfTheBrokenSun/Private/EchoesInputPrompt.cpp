// Author: Angelis Pseftis
#include "EchoesInputPrompt.h"
#include "GameFramework/InputSettings.h"

FText FEchoesInputPrompt::Chord(const FKey& Key, bool bShift, bool bCtrl, bool bAlt, bool bCmd)
{
    if (!Key.IsValid()) return NSLOCTEXT("EchoesInput", "Unassigned", "Unassigned");
    FString Prefix;
#if PLATFORM_MAC
    if (bCtrl) Prefix += TEXT("Command+");
    if (bAlt) Prefix += TEXT("Option+");
#else
    if (bCtrl) Prefix += TEXT("Ctrl+");
    if (bAlt) Prefix += TEXT("Alt+");
#endif
    if (bShift) Prefix += TEXT("Shift+");
#if PLATFORM_MAC
    if (bCmd) Prefix += TEXT("Control+");
#else
    if (bCmd) Prefix += TEXT("Command+");
#endif
    return FText::FromString(Prefix + Key.GetDisplayName().ToString());
}

FText FEchoesInputPrompt::Action(FName Name)
{
    TArray<FInputActionKeyMapping> Mappings;
    GetDefault<UInputSettings>()->GetActionMappingByName(Name, Mappings);
    TArray<FString> Labels;
    for (const auto& M : Mappings)
        if (M.Key.IsValid() && !M.Key.IsGamepadKey())
            Labels.AddUnique(Chord(M.Key, M.bShift, M.bCtrl, M.bAlt, M.bCmd).ToString());
    return Labels.IsEmpty() ? NSLOCTEXT("EchoesInput", "Unassigned", "Unassigned")
        : FText::FromString(FString::Join(Labels, TEXT(" / ")));
}

FText FEchoesInputPrompt::Axis(FName Name)
{
    TArray<FInputAxisKeyMapping> Mappings;
    GetDefault<UInputSettings>()->GetAxisMappingByName(Name, Mappings);
    TArray<FString> Labels;
    for (const auto& M : Mappings)
        if (M.Key.IsValid() && !M.Key.IsGamepadKey() && M.Scale != 0)
            Labels.AddUnique(M.Key.GetDisplayName().ToString());
    return Labels.IsEmpty() ? NSLOCTEXT("EchoesInput", "Unassigned", "Unassigned")
        : FText::FromString(FString::Join(Labels, TEXT(" / ")));
}

FText FEchoesInputPrompt::Command(EEchoesCommandDeckAction Value)
{
    FName Name;
    switch (Value)
    {
    case EEchoesCommandDeckAction::AttackMove: Name = TEXT("AttackMoveAtCursor"); break;
    case EEchoesCommandDeckAction::Patrol: Name = TEXT("PatrolAtCursor"); break;
    case EEchoesCommandDeckAction::Hold: Name = TEXT("HoldSelected"); break;
    case EEchoesCommandDeckAction::Guard: Name = TEXT("GuardAtCursor"); break;
    case EEchoesCommandDeckAction::Stop: Name = TEXT("StopSelected"); break;
    case EEchoesCommandDeckAction::BuildBarracks: Name = TEXT("BuildBarracks"); break;
    case EEchoesCommandDeckAction::BuildDropoff: Name = TEXT("BuildDropoff"); break;
    case EEchoesCommandDeckAction::BuildUtility: Name = TEXT("BuildUtility"); break;
    case EEchoesCommandDeckAction::ProduceWorker: Name = TEXT("ProduceWorker"); break;
    case EEchoesCommandDeckAction::ProduceSoldier: Name = TEXT("ProduceSoldier"); break;
    case EEchoesCommandDeckAction::ProduceHeavy: Name = TEXT("ProduceHeavy"); break;
    case EEchoesCommandDeckAction::ProduceScout: Name = TEXT("ProduceScout"); break;
    case EEchoesCommandDeckAction::ToggleTechnology: Name = TEXT("ToggleTechnologyPanel"); break;
    case EEchoesCommandDeckAction::CycleFormation: Name = TEXT("CycleFormation"); break;
    case EEchoesCommandDeckAction::ToggleBulwarkDeployment: Name = TEXT("ToggleBulwarkDeployment"); break;
    // R is the existing gameplay repair/restart binding; this command is only
    // presented on a running worker deck, where it routes to repair/assist.
    case EEchoesCommandDeckAction::RepairAtCursor: Name = TEXT("RestartScenario"); break;
    case EEchoesCommandDeckAction::CancelConstruction: Name = TEXT("CancelConstruction"); break;
    default: return FText::GetEmpty();
    }
    return Action(Name);
}
