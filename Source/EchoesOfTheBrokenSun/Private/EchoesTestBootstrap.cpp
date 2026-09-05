#include "EchoesTestBootstrap.h"

#include "EchoesCampaignProgress.h"
#include "HAL/PlatformProcess.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#if PLATFORM_MAC || PLATFORM_UNIX
#include <limits.h>
#include <stdlib.h>
#include <sys/stat.h>
#endif

namespace
{
constexpr TCHAR TestSandboxFlag[] = TEXT("EchoesTestSandbox");
constexpr TCHAR TestSandboxManifestKey[] =
    TEXT("EchoesTestSandboxManifest=");
constexpr TCHAR SaveDirectoryKey[] = TEXT("EchoesSaveGameDirectory=");
constexpr TCHAR UserDirectoryKey[] = TEXT("UserDir=");
constexpr TCHAR ManifestFormat[] = TEXT("EchoesTestSandbox/v1");
bool GDedicatedTestSandboxValidated = false;

FString NormalizeDirectory(FString Path)
{
    Path = FPaths::ConvertRelativePathToFull(Path);
    FPaths::NormalizeDirectoryName(Path);
    FPaths::CollapseRelativeDirectories(Path);
    return Path;
}

bool IsUnderOrEqual(const FString& Candidate, const FString& Parent)
{
    return Candidate == Parent || FPaths::IsUnderDirectory(Candidate, Parent);
}

bool IsUnderEitherTemporaryRoot(
    const FString& Candidate,
    const FString& LexicalTemporaryRoot,
    const FString& CanonicalTemporaryRoot)
{
    return IsUnderOrEqual(Candidate, LexicalTemporaryRoot) ||
        IsUnderOrEqual(Candidate, CanonicalTemporaryRoot);
}

bool CanonicalizeExistingSandboxPath(
    const FString& Path,
    FString& OutCanonical,
    FString& OutFailure)
{
#if PLATFORM_MAC || PLATFORM_UNIX
    const FTCHARToUTF8 Utf8Path(*Path);
    char CanonicalPath[PATH_MAX] = {};
    if (realpath(Utf8Path.Get(), CanonicalPath) == nullptr)
    {
        OutFailure = TEXT("[ECHOES_TEST_SANDBOX_ROUTE_REALPATH_FAILED]");
        return false;
    }
    struct stat LinkStatus = {};
    if (lstat(Utf8Path.Get(), &LinkStatus) != 0 ||
        S_ISLNK(LinkStatus.st_mode))
    {
        OutFailure = TEXT("[ECHOES_TEST_SANDBOX_ROUTE_SYMLINK_REJECTED]");
        return false;
    }
    OutCanonical = NormalizeDirectory(UTF8_TO_TCHAR(CanonicalPath));
    return true;
#else
    OutFailure = TEXT("[ECHOES_TEST_SANDBOX_REALPATH_UNSUPPORTED_PLATFORM]");
    return false;
#endif
}

bool IsAutomationRun(const TCHAR* CommandLine)
{
    FString ExecCommands;
    return FParse::Value(CommandLine, TEXT("ExecCmds="), ExecCommands) &&
        ExecCommands.Contains(
            TEXT("Automation RunTests"),
            ESearchCase::IgnoreCase);
}

bool ReadManifest(
    const FString& ManifestPath,
    FString& OutRoot,
    FString& OutSaveDirectory,
    FString& OutUserDirectory,
    FString& OutFailure)
{
    FString ManifestText;
    if (!FFileHelper::LoadFileToString(ManifestText, *ManifestPath))
    {
        OutFailure = TEXT("[ECHOES_TEST_SANDBOX_MANIFEST_UNREADABLE]");
        return false;
    }

    TSharedPtr<FJsonObject> Manifest;
    const TSharedRef<TJsonReader<>> Reader =
        TJsonReaderFactory<>::Create(ManifestText);
    if (!FJsonSerializer::Deserialize(Reader, Manifest) || !Manifest.IsValid())
    {
        OutFailure = TEXT("[ECHOES_TEST_SANDBOX_MANIFEST_INVALID_JSON]");
        return false;
    }

    FString Format;
    if (!Manifest->TryGetStringField(TEXT("format"), Format) ||
        Format != ManifestFormat ||
        !Manifest->TryGetStringField(TEXT("root"), OutRoot) ||
        !Manifest->TryGetStringField(TEXT("save_dir"), OutSaveDirectory) ||
        !Manifest->TryGetStringField(TEXT("user_dir"), OutUserDirectory) ||
        OutRoot.IsEmpty() || OutSaveDirectory.IsEmpty() ||
        OutUserDirectory.IsEmpty())
    {
        OutFailure = TEXT("[ECHOES_TEST_SANDBOX_MANIFEST_SCHEMA_INVALID]");
        return false;
    }
    return true;
}
}

bool EchoesTestBootstrap::ValidateBeforeGameInstance(FString& OutFailure)
{
    OutFailure.Reset();
    GDedicatedTestSandboxValidated = false;
    const TCHAR* CommandLine = FCommandLine::Get();
    const bool bDedicatedTestMode =
        FParse::Param(CommandLine, TestSandboxFlag);
    const bool bAutomationRun = IsAutomationRun(CommandLine);
    if (!bDedicatedTestMode && !bAutomationRun)
    {
        return true;
    }

#if UE_BUILD_SHIPPING
    OutFailure = TEXT("[ECHOES_TEST_SANDBOX_SHIPPING_FORBIDDEN]");
    return false;
#else
    if (!bDedicatedTestMode)
    {
        OutFailure = TEXT("[ECHOES_TEST_SANDBOX_FLAG_MISSING]");
        return false;
    }

    FString ManifestPath;
    FString CommandSaveDirectory;
    FString CommandUserDirectory;
    if (!FParse::Value(
            CommandLine, TestSandboxManifestKey, ManifestPath) ||
        !FParse::Value(CommandLine, SaveDirectoryKey, CommandSaveDirectory) ||
        !FParse::Value(CommandLine, UserDirectoryKey, CommandUserDirectory))
    {
        OutFailure = TEXT("[ECHOES_TEST_SANDBOX_ROUTING_MISSING]");
        return false;
    }

    ManifestPath = FPaths::ConvertRelativePathToFull(ManifestPath);
    FPaths::NormalizeFilename(ManifestPath);
    FPaths::CollapseRelativeDirectories(ManifestPath);
    CommandSaveDirectory = NormalizeDirectory(CommandSaveDirectory);
    CommandUserDirectory = NormalizeDirectory(CommandUserDirectory);
    const FString TemporaryRoot = NormalizeDirectory(
        FPlatformProcess::UserTempDir());
    // macOS may report /var/... while mkdtemp/realpath records the same
    // directory below /private/var/.... Canonicalize only this known system
    // temporary root before looking up any caller-controlled route.
    FString CanonicalTemporaryRoot;
    if (!CanonicalizeExistingSandboxPath(
            TemporaryRoot, CanonicalTemporaryRoot, OutFailure))
    {
        return false;
    }
    // These lexical checks run before opening the manifest or resolving a
    // route. A forged command line therefore cannot direct bootstrap to
    // inspect a production path. On macOS the accepted roots are only the
    // system temporary directory as reported by Foundation and its canonical
    // /private/var alias; both name the same OS-owned temporary hierarchy.
    if (!IsUnderEitherTemporaryRoot(
            ManifestPath, TemporaryRoot, CanonicalTemporaryRoot) ||
        !IsUnderEitherTemporaryRoot(
            CommandSaveDirectory, TemporaryRoot, CanonicalTemporaryRoot) ||
        !IsUnderEitherTemporaryRoot(
            CommandUserDirectory, TemporaryRoot, CanonicalTemporaryRoot))
    {
        OutFailure = TEXT("[ECHOES_TEST_SANDBOX_ROUTE_NOT_TEMPORARY]");
        return false;
    }
    FString CanonicalManifestPath;
    if (!CanonicalizeExistingSandboxPath(
            ManifestPath, CanonicalManifestPath, OutFailure))
    {
        return false;
    }

    FString ManifestRoot;
    FString ManifestSaveDirectory;
    FString ManifestUserDirectory;
    if (!ReadManifest(
            ManifestPath,
            ManifestRoot,
            ManifestSaveDirectory,
            ManifestUserDirectory,
            OutFailure))
    {
        return false;
    }

    ManifestRoot = NormalizeDirectory(ManifestRoot);
    ManifestSaveDirectory = NormalizeDirectory(ManifestSaveDirectory);
    ManifestUserDirectory = NormalizeDirectory(ManifestUserDirectory);
    if (!IsUnderEitherTemporaryRoot(
            ManifestRoot, TemporaryRoot, CanonicalTemporaryRoot) ||
        !IsUnderEitherTemporaryRoot(
            ManifestSaveDirectory, TemporaryRoot, CanonicalTemporaryRoot) ||
        !IsUnderEitherTemporaryRoot(
            ManifestUserDirectory, TemporaryRoot, CanonicalTemporaryRoot) ||
        !IsUnderOrEqual(ManifestSaveDirectory, ManifestRoot) ||
        !IsUnderOrEqual(ManifestUserDirectory, ManifestRoot))
    {
        OutFailure = TEXT("[ECHOES_TEST_SANDBOX_ROUTE_NOT_TEMPORARY]");
        return false;
    }
    FString CanonicalRoot;
    FString CanonicalManifestSaveDirectory;
    FString CanonicalManifestUserDirectory;
    FString CanonicalCommandSaveDirectory;
    FString CanonicalCommandUserDirectory;
    if (!CanonicalizeExistingSandboxPath(
            ManifestRoot, CanonicalRoot, OutFailure) ||
        !CanonicalizeExistingSandboxPath(
            ManifestSaveDirectory, CanonicalManifestSaveDirectory, OutFailure) ||
        !CanonicalizeExistingSandboxPath(
            ManifestUserDirectory, CanonicalManifestUserDirectory, OutFailure) ||
        !CanonicalizeExistingSandboxPath(
            CommandSaveDirectory, CanonicalCommandSaveDirectory, OutFailure) ||
        !CanonicalizeExistingSandboxPath(
            CommandUserDirectory, CanonicalCommandUserDirectory, OutFailure))
    {
        return false;
    }
    const FString PlayerSaveRoot = NormalizeDirectory(FPaths::Combine(
        FPaths::ProjectSavedDir(), TEXT("SaveGames")));

    if (!IsUnderOrEqual(CanonicalRoot, CanonicalTemporaryRoot) ||
        !IsUnderOrEqual(CanonicalManifestSaveDirectory, CanonicalRoot) ||
        !IsUnderOrEqual(CanonicalManifestUserDirectory, CanonicalRoot) ||
        !IsUnderOrEqual(CanonicalManifestPath, CanonicalRoot) ||
        CanonicalCommandSaveDirectory != CanonicalManifestSaveDirectory ||
        CanonicalCommandUserDirectory != CanonicalManifestUserDirectory ||
        CanonicalManifestSaveDirectory == PlayerSaveRoot ||
        FPaths::IsUnderDirectory(CanonicalManifestSaveDirectory, PlayerSaveRoot))
    {
        OutFailure = TEXT("[ECHOES_TEST_SANDBOX_ROUTING_INVALID]");
        return false;
    }

    FString CanonicalStoreDirectory;
    if (!CanonicalizeExistingSandboxPath(
            NormalizeDirectory(FEchoesCampaignProgressStore::GetSaveGameDirectory()),
            CanonicalStoreDirectory,
            OutFailure) ||
        CanonicalStoreDirectory != CanonicalManifestSaveDirectory)
    {
        OutFailure = TEXT("[ECHOES_TEST_SANDBOX_STORE_ROUTE_MISMATCH]");
        return false;
    }
    GDedicatedTestSandboxValidated = true;
    return true;
#endif
}

bool EchoesTestBootstrap::IsDedicatedTestSandboxValidated()
{
    return GDedicatedTestSandboxValidated;
}
