// Author: Angelis Pseftis
#include "EchoesInputPrompt.h"
#include "GameFramework/InputSettings.h"

namespace
{
FString ModifierPrefix(bool bShift, bool bCtrl, bool bAlt, bool bCmd)
{
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
    return Prefix;
}

// Punctuation keys spell their symbol in a tile corner. The engine's short
// display name answers the rest; a letter or function key is already short.
FString ShortKeyName(const FKey& Key)
{
    static const TMap<FKey, FString> Symbols = {
        {EKeys::Semicolon, TEXT(";")}, {EKeys::Apostrophe, TEXT("'")},
        {EKeys::Comma, TEXT(",")}, {EKeys::Period, TEXT(".")},
        {EKeys::Slash, TEXT("/")}, {EKeys::Backslash, TEXT("\\")},
        {EKeys::Hyphen, TEXT("-")}, {EKeys::Equals, TEXT("=")},
        {EKeys::LeftBracket, TEXT("[")}, {EKeys::RightBracket, TEXT("]")},
        {EKeys::Tilde, TEXT("`")}, {EKeys::SpaceBar, TEXT("Space")},
        {EKeys::Escape, TEXT("Esc")}};
    if (const FString* Symbol = Symbols.Find(Key)) return *Symbol;
    return Key.GetDisplayName(false).ToString();
}

FText BoundAction(FName Name, bool bShort)
{
    TArray<FInputActionKeyMapping> Mappings;
    GetDefault<UInputSettings>()->GetActionMappingByName(Name, Mappings);
    TArray<FString> Labels;
    for (const auto& M : Mappings)
        if (M.Key.IsValid() && !M.Key.IsGamepadKey())
            Labels.AddUnique((bShort
                ? FEchoesInputPrompt::Glyph(M.Key, M.bShift, M.bCtrl, M.bAlt, M.bCmd)
                : FEchoesInputPrompt::Chord(M.Key, M.bShift, M.bCtrl, M.bAlt, M.bCmd)).ToString());
    return Labels.IsEmpty() ? NSLOCTEXT("EchoesInput", "Unassigned", "Unassigned")
        : FText::FromString(FString::Join(Labels, TEXT(" / ")));
}

FName DeckActionName(EEchoesCommandDeckAction Value)
{
    switch (Value)
    {
    case EEchoesCommandDeckAction::AttackMove: return TEXT("AttackMoveAtCursor");
    case EEchoesCommandDeckAction::Patrol: return TEXT("PatrolAtCursor");
    case EEchoesCommandDeckAction::Hold: return TEXT("HoldSelected");
    case EEchoesCommandDeckAction::Guard: return TEXT("GuardAtCursor");
    case EEchoesCommandDeckAction::Stop: return TEXT("StopSelected");
    case EEchoesCommandDeckAction::BuildBarracks: return TEXT("BuildBarracks");
    case EEchoesCommandDeckAction::BuildDropoff: return TEXT("BuildDropoff");
    case EEchoesCommandDeckAction::BuildUtility: return TEXT("BuildUtility");
    case EEchoesCommandDeckAction::ProduceWorker: return TEXT("ProduceWorker");
    case EEchoesCommandDeckAction::ProduceSoldier: return TEXT("ProduceSoldier");
    case EEchoesCommandDeckAction::ProduceHeavy: return TEXT("ProduceHeavy");
    case EEchoesCommandDeckAction::ProduceScout: return TEXT("ProduceScout");
    case EEchoesCommandDeckAction::ToggleTechnology: return TEXT("ToggleTechnologyPanel");
    case EEchoesCommandDeckAction::CycleFormation: return TEXT("CycleFormation");
    case EEchoesCommandDeckAction::ToggleBulwarkDeployment: return TEXT("ToggleBulwarkDeployment");
    // R is the existing gameplay repair/restart binding; this command is only
    // presented on a running worker deck, where it routes to repair/assist.
    case EEchoesCommandDeckAction::RepairAtCursor: return TEXT("RestartScenario");
    case EEchoesCommandDeckAction::CancelConstruction: return TEXT("CancelConstruction");
    default: return NAME_None;
    }
}
}

FText FEchoesInputPrompt::Chord(const FKey& Key, bool bShift, bool bCtrl, bool bAlt, bool bCmd)
{
    if (!Key.IsValid()) return NSLOCTEXT("EchoesInput", "Unassigned", "Unassigned");
    return FText::FromString(ModifierPrefix(bShift, bCtrl, bAlt, bCmd) + Key.GetDisplayName().ToString());
}

FText FEchoesInputPrompt::Glyph(const FKey& Key, bool bShift, bool bCtrl, bool bAlt, bool bCmd)
{
    if (!Key.IsValid()) return NSLOCTEXT("EchoesInput", "Unassigned", "Unassigned");
    return FText::FromString(ModifierPrefix(bShift, bCtrl, bAlt, bCmd) + ShortKeyName(Key));
}

FText FEchoesInputPrompt::Action(FName Name)
{
    return BoundAction(Name, false);
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
    const FName Name = DeckActionName(Value);
    return Name.IsNone() ? FText::GetEmpty() : BoundAction(Name, false);
}

FText FEchoesInputPrompt::CommandGlyph(EEchoesCommandDeckAction Value)
{
    const FName Name = DeckActionName(Value);
    return Name.IsNone() ? FText::GetEmpty() : BoundAction(Name, true);
}
