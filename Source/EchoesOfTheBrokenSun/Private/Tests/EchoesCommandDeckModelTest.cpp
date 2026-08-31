#include "EchoesCommandDeckModel.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesCommandDeckModelTest,
    "Echoes.Runtime.Presentation.CommandDeckModel",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEchoesCommandDeckModelTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    FEchoesScopedTestSaveEnvironment TestSaveEnvironment(*this);
    if (!TestSaveEnvironment.IsReady())
    {
        return false;
    }

    FEchoesCommandDeckProfile Profile;
    TestEqual(
        TEXT("Empty/non-command selection uses safe generic actions"),
        FEchoesCommandDeckModel::BuildPrimaryActions(Profile),
        FString(TEXT("[RMB] CONTEXT / MOVE    [X] STOP")));

    Profile.CombatCount = 1;
    const FString Combat = FEchoesCommandDeckModel::BuildPrimaryActions(Profile);
    TestTrue(TEXT("Combat exposes attack-move"), Combat.Contains(TEXT("[F] ATTACK-MOVE")));
    TestTrue(TEXT("Combat exposes guard"), Combat.Contains(TEXT("[J] GUARD")));

    Profile = {};
    Profile.WorkerCount = 1;
    const FString Worker = FEchoesCommandDeckModel::BuildPrimaryActions(Profile);
    TestTrue(TEXT("Worker exposes Barracks construction"), Worker.Contains(TEXT("[B] BARRACKS")));
    TestTrue(TEXT("Worker exposes Utility construction"), Worker.Contains(TEXT("[M] UTILITY")));

    Profile = {};
    Profile.bHasCommandCore = true;
    TestEqual(
        TEXT("Command Core advertises only its compatible worker key"),
        FEchoesCommandDeckModel::BuildPrimaryActions(Profile),
        FString(TEXT("[Q] PRODUCE WORKER")));

    Profile = {};
    Profile.bHasBarracks = true;
    const FString Barracks = FEchoesCommandDeckModel::BuildPrimaryActions(Profile);
    TestTrue(TEXT("Barracks exposes line-unit key"), Barracks.Contains(TEXT("[E] LINE UNIT")));
    TestTrue(TEXT("Barracks exposes heavy key"), Barracks.Contains(TEXT("[;] HEAVY")));
    TestTrue(TEXT("Barracks exposes scout key"), Barracks.Contains(TEXT("['] SCOUT")));
    TestFalse(TEXT("Barracks does not advertise worker production"), Barracks.Contains(TEXT("WORKER")));

    Profile.bHasCommandCore = true;
    const FString Combined = FEchoesCommandDeckModel::BuildPrimaryActions(Profile);
    TestTrue(TEXT("Mixed production selection exposes worker"), Combined.Contains(TEXT("[Q] WORKER")));
    TestTrue(TEXT("Mixed production selection exposes technology"), Combined.Contains(TEXT("[F2] TECHNOLOGY")));

    Profile.WorkerCount = 1;
    TestTrue(
        TEXT("Mobile worker context takes precedence over selected structures"),
        FEchoesCommandDeckModel::BuildPrimaryActions(Profile).Contains(TEXT("[B] BARRACKS")));

    Profile.CombatCount = 1;
    TestTrue(
        TEXT("Combat context takes precedence in a mixed mobile selection"),
        FEchoesCommandDeckModel::BuildPrimaryActions(Profile).Contains(TEXT("[F] ATTACK-MOVE")));
    return true;
}

#endif
