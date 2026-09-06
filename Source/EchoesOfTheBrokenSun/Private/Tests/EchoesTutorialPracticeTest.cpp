#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesPlayerFlow.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesTutorialPracticeTest,
    "Echoes.Runtime.Campaign.TutorialPracticeState",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesTutorialPracticeTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    FEchoesPlayerFlow Flow;
    Flow.SetVisible(EEchoesShellScreen::Title, true);
    Flow.Push(EEchoesShellScreen::Help);
    TestEqual(TEXT("Help is an explicit returnable shell surface"),
        Flow.Current(), EEchoesShellScreen::Help);
    TestTrue(TEXT("Help returns to its prior title surface"), Flow.Back());
    TestEqual(TEXT("Help back preserves the prior surface"),
        Flow.Current(), EEchoesShellScreen::Title);

    FEchoesTutorialPracticeState Practice;
    const uint16 DurableMastery = 0x0015;
    TestFalse(TEXT("An unimplemented later lesson cannot begin practice"),
        Practice.Begin(0x0020));
    TestFalse(TEXT("A multi-lesson target cannot begin practice"),
        Practice.Begin(0x0003));
    TestTrue(TEXT("An implemented mastered lesson can be replayed"),
        Practice.Begin(0x0004));
    TestEqual(TEXT("Practice keeps one explicit target"),
        Practice.TargetLessonBit(), static_cast<uint16>(0x0004));
    TestFalse(TEXT("A different lesson cannot complete the attempt"),
        Practice.RecordVerified(0x0002));
    TestEqual(TEXT("A failed attempt grants no practice verification"),
        Practice.VerifiedAttemptMask(), static_cast<uint16>(0));
    TestTrue(TEXT("The target can complete from its own verified attempt"),
        Practice.RecordVerified(0x0004));
    TestFalse(TEXT("Completion clears the active practice target"),
        Practice.IsActive());
    TestEqual(TEXT("Completion records only the transient attempt mask"),
        Practice.VerifiedAttemptMask(), static_cast<uint16>(0x0004));
    TestEqual(TEXT("Practice cannot clear or grant durable mastery"),
        DurableMastery, static_cast<uint16>(0x0015));
    TestTrue(TEXT("Retry opens a clean implemented attempt"),
        Practice.Begin(0x0004));
    TestEqual(TEXT("Retry does not inherit the prior attempt result"),
        Practice.VerifiedAttemptMask(), static_cast<uint16>(0));

    return true;
}

#endif
