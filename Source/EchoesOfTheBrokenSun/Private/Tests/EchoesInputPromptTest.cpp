// Author: Angelis Pseftis
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "EchoesInputPrompt.h"
#include "GameFramework/InputSettings.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEchoesInputPromptTest,
    "Echoes.Runtime.Controls.ActiveCommandPrompts",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext |
    EAutomationTestFlags::EngineFilter)

bool FEchoesInputPromptTest::RunTest(const FString& Parameters)
{
    UInputSettings* Settings = GetMutableDefault<UInputSettings>();
    const FName Name(TEXT("AttackMoveAtCursor"));
    TArray<FInputActionKeyMapping> Prior;
    Settings->GetActionMappingByName(Name, Prior);
    // Restore the live in-memory fixture even if a future assertion returns early.
    struct FRestore final
    {
        UInputSettings* Settings;
        FName Name;
        TArray<FInputActionKeyMapping> Prior;
        ~FRestore()
        {
            TArray<FInputActionKeyMapping> Current;
            Settings->GetActionMappingByName(Name, Current);
            for (const auto& M : Current) Settings->RemoveActionMapping(M, false);
            for (const auto& M : Prior) Settings->AddActionMapping(M, false);
        }
    } Restore{Settings, Name, Prior};
    for (const auto& M : Prior) Settings->RemoveActionMapping(M, false);
    TestEqual(TEXT("An unassigned command never advertises its authored default F"),
        FEchoesInputPrompt::Command(EEchoesCommandDeckAction::AttackMove).ToString(),
        NSLOCTEXT("EchoesInput", "Unassigned", "Unassigned").ToString());
    Settings->AddActionMapping(FInputActionKeyMapping(Name, EKeys::K, true, true), false);
#if PLATFORM_MAC
    const FString Expected(TEXT("Command+Shift+K"));
#else
    const FString Expected(TEXT("Ctrl+Shift+K"));
#endif
    TestEqual(TEXT("Command tile follows the actual physical chord"),
        FEchoesInputPrompt::Command(EEchoesCommandDeckAction::AttackMove).ToString(), Expected);
    Settings->AddActionMapping(FInputActionKeyMapping(Name, EKeys::L), false);
    const FString Multiple = FEchoesInputPrompt::Command(EEchoesCommandDeckAction::AttackMove).ToString();
    TestTrue(TEXT("Both active alternatives remain discoverable"),
        Multiple.Contains(Expected) && Multiple.Contains(EKeys::L.GetDisplayName().ToString()) && Multiple.Contains(TEXT(" / ")));
    TestTrue(TEXT("No command has no invented input hint"),
        FEchoesInputPrompt::Command(EEchoesCommandDeckAction::None).IsEmpty());
    // This test never saves mappings or rebuilds a player's active keymap.
    return true;
}
#endif
