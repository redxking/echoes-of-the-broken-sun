#pragma once

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"

/** Test-only preservation. Call IsReady before mutating the captured path.
 * Unreadable originals are never deleted or restored from an empty buffer.
 */
class FEchoesPreservedTestFile final
{
public:
    explicit FEchoesPreservedTestFile(FString InPath)
        : Path(MoveTemp(InPath)), Test(FAutomationTestFramework::Get().GetCurrentTest())
    {
        bExisted = IFileManager::Get().FileExists(*Path);
        bReady = Test != nullptr && (!bExisted || FFileHelper::LoadFileToArray(Contents, *Path));
        if (!bReady && Test) Test->AddError(FString::Printf(
            TEXT("[FIXTURE_CAPTURE_FAILED] Cannot preserve %s; setup must stop."), *Path));
    }
    FEchoesPreservedTestFile(const FEchoesPreservedTestFile&) = delete;
    FEchoesPreservedTestFile& operator=(const FEchoesPreservedTestFile&) = delete;
    ~FEchoesPreservedTestFile()
    {
        if (!bReady) return;
        const bool bRestored = bExisted
            ? FFileHelper::SaveArrayToFile(Contents, *Path)
            : (!IFileManager::Get().FileExists(*Path) ||
                IFileManager::Get().Delete(*Path, false, true, true));
        if (!bRestored) Test->AddError(FString::Printf(
            TEXT("[FIXTURE_RESTORE_FAILED] Could not restore %s."), *Path));
    }
    [[nodiscard]] bool IsReady() const { return bReady; }
private:
    FString Path;
    TArray<uint8> Contents;
    FAutomationTestBase* Test = nullptr;
    bool bExisted = false;
    bool bReady = false;
};
#endif
