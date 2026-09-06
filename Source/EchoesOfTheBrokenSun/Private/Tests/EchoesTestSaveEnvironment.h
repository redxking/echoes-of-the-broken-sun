#pragma once

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesCampaignProgress.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesTestBootstrap.h"
#include "HAL/CriticalSection.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Guid.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/ScopeLock.h"

enum class EEchoesTestSaveOverrideMode : uint8
{
    Directory,
    ExactFiles
};

/**
 * Gives one automation test a unique save root before it constructs a world.
 * The original process command line is restored on every exit path, and the
 * complete test root must be removable before the test can pass cleanly.
 */
class FEchoesScopedTestSaveEnvironment final
{
public:
    explicit FEchoesScopedTestSaveEnvironment(
        FAutomationTestBase& InTest,
        EEchoesTestSaveOverrideMode InMode =
            EEchoesTestSaveOverrideMode::Directory)
        : Test(InTest)
    {
        if (!EchoesTestBootstrap::IsDedicatedTestSandboxValidated())
        {
            Test.AddError(
                TEXT("[ECHOES_TEST_STORAGE_UNSANDBOXED_PROCESS] Save-I/O automation requires the validated process-level test sandbox launch."));
            return;
        }
        {
            FScopeLock Lock(&ActiveScopeMutex);
            if (bScopeActive)
            {
                Test.AddError(
                    TEXT("[ECHOES_TEST_STORAGE_OVERLAP] Save-isolated automation tests must run serially because the Unreal command line is process-global."));
                return;
            }
            bScopeActive = true;
            bOwnsActiveScope = true;
            OriginalCommandLine = FCommandLine::Get();
        }

        FString ExistingCampaignOverride;
        FString ExistingQuickSaveOverride;
        if (FParse::Value(
                *OriginalCommandLine,
                TEXT("EchoesCampaignProgressPath="),
                ExistingCampaignOverride) ||
            FParse::Value(
                *OriginalCommandLine,
                TEXT("EchoesQuickSavePath="),
                ExistingQuickSaveOverride))
        {
            Test.AddError(
                TEXT("[ECHOES_TEST_STORAGE_OVERRIDE_CONFLICT] Exact save-file overrides were already active before the test scope."));
            return;
        }

        FString SuiteRoot;
        if (!EchoesTestBootstrap::GetValidatedSuiteSaveDirectory(SuiteRoot))
        {
            Test.AddError(
                TEXT("[ECHOES_TEST_STORAGE_VALIDATED_ROOT_MISSING] Bootstrap did not provide the canonical validated suite save root."));
            return;
        }
        if (SuiteRoot.Contains(TEXT("\"")) ||
            SuiteRoot.Contains(TEXT("\r")) ||
            SuiteRoot.Contains(TEXT("\n")))
        {
            Test.AddError(
                TEXT("[ECHOES_TEST_STORAGE_INVALID_ROOT] The automation storage parent contains a character that cannot be represented safely on the Unreal command line."));
            return;
        }
        SuiteRoot = NormalizeDirectory(SuiteRoot);
        const FString PlayerSaveRoot = NormalizeDirectory(
            FPaths::Combine(
                FPaths::ProjectDir(),
                TEXT("Saved"),
                TEXT("SaveGames")));
        if (SuiteRoot == PlayerSaveRoot ||
            FPaths::IsUnderDirectory(SuiteRoot, PlayerSaveRoot))
        {
            Test.AddError(
                TEXT("[ECHOES_TEST_STORAGE_UNSAFE_ROOT] The automation storage parent may not be the real player SaveGames tree."));
            return;
        }
        Directory = NormalizeDirectory(
            FPaths::Combine(
                SuiteRoot,
                FString::Printf(
                    TEXT("Test Save-%s"),
                    *FGuid::NewGuid().ToString(EGuidFormats::Digits))));
        CampaignPath = FPaths::Combine(
            Directory,
            TEXT("EchoesCampaignProgress.bin"));
        QuickSavePath = FPaths::Combine(
            Directory,
            TEXT("EchoesQuickSave.bin"));

        if (IFileManager::Get().DirectoryExists(*Directory))
        {
            Test.AddError(FString::Printf(
                TEXT("[ECHOES_TEST_STORAGE_COLLISION] The GUID-scoped directory already exists and will not be touched: %s"),
                *Directory));
            return;
        }
        if (!IFileManager::Get().MakeDirectory(*Directory, true))
        {
            Test.AddError(FString::Printf(
                TEXT("[ECHOES_TEST_STORAGE_CREATE_FAILED] Could not create isolated test storage: %s"),
                *Directory));
            return;
        }
        bOwnsDirectory = true;

        FString ScopedArguments = FString::Printf(
            TEXT("-EchoesSaveGameDirectory=\"%s\""),
            *Directory);
        if (InMode == EEchoesTestSaveOverrideMode::ExactFiles)
        {
            ScopedArguments += FString::Printf(
                TEXT(" -EchoesCampaignProgressPath=\"%s\" -EchoesQuickSavePath=\"%s\""),
                *CampaignPath,
                *QuickSavePath);
        }
        const FString TestCommandLine = FString::Printf(
            TEXT("%s %s"),
            *ScopedArguments,
            *OriginalCommandLine);
        if (TestCommandLine.Len() >=
            static_cast<int32>(FCommandLine::MaxCommandLineSize))
        {
            Test.AddError(FString::Printf(
                TEXT("[ECHOES_TEST_STORAGE_COMMAND_LINE_TOO_LONG] Scoped command line length %d exceeds Unreal's %u-character limit."),
                TestCommandLine.Len(),
                FCommandLine::MaxCommandLineSize - 1));
            return;
        }
        bCommandLineChanged = true;
        if (!FCommandLine::Set(*TestCommandLine))
        {
            const bool bRestored = FCommandLine::Set(*OriginalCommandLine);
            bCommandLineChanged = !bRestored;
            Test.AddError(
                TEXT("[ECHOES_TEST_STORAGE_COMMAND_LINE_SET_FAILED] Unreal rejected the GUID-scoped test command line."));
            if (!bRestored)
            {
                Test.AddError(
                    TEXT("[ECHOES_TEST_STORAGE_COMMAND_LINE_RESTORE_FAILED] Unreal also rejected the original command line after the failed test override."));
            }
            return;
        }

        if (!IsPathScoped(FEchoesCampaignProgressStore::GetDefaultPath()) ||
            !IsPathScoped(UEchoesSimulationSubsystem::GetQuickSavePath()) ||
            FEchoesCampaignProgressStore::GetSaveGameDirectory() != Directory)
        {
            Test.AddError(
                TEXT("[ECHOES_TEST_STORAGE_RESOLUTION_FAILED] Campaign or quick-save storage resolved outside the GUID-scoped directory."));
            return;
        }
        bReady = true;
    }

    ~FEchoesScopedTestSaveEnvironment()
    {
        Finish();
    }

    FEchoesScopedTestSaveEnvironment(
        const FEchoesScopedTestSaveEnvironment&) = delete;
    FEchoesScopedTestSaveEnvironment& operator=(
        const FEchoesScopedTestSaveEnvironment&) = delete;

    [[nodiscard]] bool IsReady() const
    {
        return bReady;
    }

    [[nodiscard]] bool IsPathScoped(const FString& Path) const
    {
        FString NormalizedPath = FPaths::ConvertRelativePathToFull(Path);
        FPaths::NormalizeFilename(NormalizedPath);
        FPaths::CollapseRelativeDirectories(NormalizedPath);
        return FPaths::IsUnderDirectory(NormalizedPath, Directory);
    }

    bool Finish()
    {
        if (bFinished)
        {
            return bCleanupSucceeded;
        }
        bFinished = true;
        if (bCommandLineChanged)
        {
            if (!FCommandLine::Set(*OriginalCommandLine))
            {
                Test.AddError(
                    TEXT("[ECHOES_TEST_STORAGE_COMMAND_LINE_RESTORE_FAILED] Unreal rejected the original command line during test cleanup."));
            }
            bCommandLineChanged = false;
        }
        bCleanupSucceeded = !bOwnsDirectory || Directory.IsEmpty() ||
            !IFileManager::Get().DirectoryExists(*Directory) ||
            IFileManager::Get().DeleteDirectory(*Directory, false, true);
        if (bOwnsDirectory && !Directory.IsEmpty() &&
            IFileManager::Get().DirectoryExists(*Directory))
        {
            bCleanupSucceeded = false;
        }
        if (!bCleanupSucceeded)
        {
            Test.AddError(FString::Printf(
                TEXT("[ECHOES_TEST_STORAGE_CLEANUP_FAILED] Isolated test storage remains on disk: %s"),
                *Directory));
        }
        if (bOwnsActiveScope)
        {
            FScopeLock Lock(&ActiveScopeMutex);
            bScopeActive = false;
            bOwnsActiveScope = false;
        }
        return bCleanupSucceeded;
    }

    FString Directory;
    FString CampaignPath;
    FString QuickSavePath;

private:
    static FString NormalizeDirectory(FString Path)
    {
        Path = FPaths::ConvertRelativePathToFull(Path);
        FPaths::NormalizeDirectoryName(Path);
        FPaths::CollapseRelativeDirectories(Path);
        return Path;
    }

    FAutomationTestBase& Test;
    FString OriginalCommandLine;
    inline static FCriticalSection ActiveScopeMutex;
    inline static bool bScopeActive = false;
    bool bReady = false;
    bool bCommandLineChanged = false;
    bool bOwnsDirectory = false;
    bool bOwnsActiveScope = false;
    bool bFinished = false;
    bool bCleanupSucceeded = false;
};

#endif
