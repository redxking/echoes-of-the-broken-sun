#include "EchoesPlayerController.h"

#include "EchoesAmbienceSubsystem.h"
#include "EchoesCommandMarkerView.h"
#include "EchoesEntityView.h"
#include "EchoesFogView.h"
#include "EchoesFactionPolicy.h"
#include "EchoesGameMode.h"
#include "EchoesGameInstance.h"
#include "EchoesGameUserSettings.h"
#include "EchoesHudLayout.h"
#include "EchoesInterfaceAudioSubsystem.h"
#include "EchoesMusicSubsystem.h"
#include "EchoesNarrativeSubsystem.h"
#include "EchoesNetworkSession.h"
#include "EchoesOnlineFrontDoorLayout.h"
#include "EchoesOfTheBrokenSun.h"
#include "EchoesPresentationAudioSubsystem.h"
#include "EchoesPointerCombatGuardReview.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesSkirmishOverlayLayout.h"
#include "EchoesTechnologyPanelLayout.h"
#include "EchoesTerrainView.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/EngineTypes.h"
#include "Engine/GameViewportClient.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/SkyLight.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/PlatformApplicationMisc.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "InputCoreTypes.h"
#include "InputKeyEventArgs.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "TimerManager.h"
#include "UnrealClient.h"
#include "Widgets/SViewport.h"

#include <algorithm>
#include <limits>

#if UE_BUILD_DEVELOPMENT && WITH_DEV_AUTOMATION_TESTS && \
    (PLATFORM_MAC || PLATFORM_UNIX)
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace
{
constexpr float DragSelectionThresholdPixels = 8.0f;
constexpr float FormationSpacingWorldUnits = 150.0f;
constexpr int32 ControlGroupCount = 10;
constexpr float NetworkTileWorldSize = 200.0f;

[[nodiscard]] FString FactionDisplayName(echoes::sim::Faction Faction)
{
    return echoes::presentation::FactionDisplayName(Faction);
}

[[nodiscard]] bool OutcomeBelongsToSeat(
    echoes::sim::MatchOutcome Outcome,
    uint8 Seat)
{
    switch (Outcome)
    {
        case echoes::sim::MatchOutcome::Player0Victory: return Seat == 0;
        case echoes::sim::MatchOutcome::Player1Victory: return Seat == 1;
        case echoes::sim::MatchOutcome::Player2Victory: return Seat == 2;
        case echoes::sim::MatchOutcome::Player3Victory: return Seat == 3;
        default: return false;
    }
}

constexpr int32 DevelopmentResumeCredentialLength = 32;

[[nodiscard]] bool IsBoundedClientResumeCredential(
    const FString& Credential)
{
    if (Credential.Len() != DevelopmentResumeCredentialLength)
    {
        return false;
    }
    for (const TCHAR Character : Credential)
    {
        if (!FChar::IsHexDigit(Character))
        {
            return false;
        }
    }
    return true;
}

#if UE_BUILD_DEVELOPMENT && WITH_DEV_AUTOMATION_TESTS
[[nodiscard]] bool NormalizeDevelopmentCredentialFilePath(
    const FString& Candidate,
    FString& OutNormalized,
    FString& OutReason)
{
    OutNormalized.Reset();
    OutReason.Reset();
    if (Candidate.IsEmpty() || Candidate.Len() > 2048 ||
        Candidate != Candidate.TrimStartAndEnd() ||
        FPaths::IsRelative(Candidate))
    {
        OutReason = TEXT("NET_RESUME_CREDENTIAL_FILE_PATH_INVALID");
        return false;
    }
    for (const TCHAR Character : Candidate)
    {
        if (FChar::IsControl(Character))
        {
            OutReason = TEXT("NET_RESUME_CREDENTIAL_FILE_PATH_INVALID");
            return false;
        }
    }
    OutNormalized = Candidate;
    FPaths::NormalizeFilename(OutNormalized);
    if (!FPaths::CollapseRelativeDirectories(OutNormalized))
    {
        OutNormalized.Reset();
        OutReason = TEXT("NET_RESUME_CREDENTIAL_FILE_PATH_INVALID");
        return false;
    }
    const FString FileName = FPaths::GetCleanFilename(OutNormalized);
    if (FileName != TEXT("EchoesResumeCredential.bin") &&
        FileName != TEXT("EchoesInvalidResumeCredential.bin"))
    {
        OutNormalized.Reset();
        OutReason = TEXT("NET_RESUME_CREDENTIAL_FILE_NAME_INVALID");
        return false;
    }
    FString ParentDirectory = FPaths::GetPath(OutNormalized);
    FPaths::NormalizeDirectoryName(ParentDirectory);
    if (!FPaths::GetCleanFilename(ParentDirectory).StartsWith(TEXT("run.")))
    {
        OutNormalized.Reset();
        OutReason = TEXT("NET_RESUME_CREDENTIAL_FILE_ROOT_INVALID");
        return false;
    }
    return true;
}

#if PLATFORM_MAC || PLATFORM_UNIX
[[nodiscard]] bool OpenDevelopmentCredentialFile(
    const FString& NormalizedPath,
    int FileOpenFlags,
    int& OutDirectoryHandle,
    int& OutFileHandle,
    FString& OutLeafName,
    FString& OutReason)
{
    OutDirectoryHandle = -1;
    OutFileHandle = -1;
    OutLeafName = FPaths::GetCleanFilename(NormalizedPath);
    const FString ParentDirectory = FPaths::GetPath(NormalizedPath);
    OutDirectoryHandle = open(
        TCHAR_TO_UTF8(*ParentDirectory),
        O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    if (OutDirectoryHandle < 0)
    {
        OutReason = TEXT("NET_RESUME_CREDENTIAL_DIRECTORY_OPEN_FAILED");
        return false;
    }
    struct stat DirectoryState {};
    if (fstat(OutDirectoryHandle, &DirectoryState) != 0 ||
        !S_ISDIR(DirectoryState.st_mode) ||
        DirectoryState.st_uid != geteuid() ||
        (DirectoryState.st_mode & 0777) != 0700)
    {
        close(OutDirectoryHandle);
        OutDirectoryHandle = -1;
        OutReason = TEXT("NET_RESUME_CREDENTIAL_DIRECTORY_NOT_PRIVATE");
        return false;
    }
    OutFileHandle = openat(
        OutDirectoryHandle,
        TCHAR_TO_UTF8(*OutLeafName),
        FileOpenFlags | O_NOFOLLOW | O_CLOEXEC | O_NONBLOCK);
    if (OutFileHandle < 0)
    {
        close(OutDirectoryHandle);
        OutDirectoryHandle = -1;
        OutReason = TEXT("NET_RESUME_CREDENTIAL_FILE_OPEN_FAILED");
        return false;
    }
    struct stat FileState {};
    if (fstat(OutFileHandle, &FileState) != 0 ||
        !S_ISREG(FileState.st_mode) || FileState.st_uid != geteuid() ||
        FileState.st_nlink != 1 || (FileState.st_mode & 0777) != 0600)
    {
        close(OutFileHandle);
        close(OutDirectoryHandle);
        OutFileHandle = -1;
        OutDirectoryHandle = -1;
        OutReason = TEXT("NET_RESUME_CREDENTIAL_FILE_NOT_PRIVATE");
        return false;
    }
    return true;
}

[[nodiscard]] bool OpenedDevelopmentCredentialFileStillNamesEntry(
    int DirectoryHandle,
    int FileHandle,
    const FString& LeafName)
{
    struct stat OpenedFileState {};
    struct stat DirectoryEntryState {};
    return fstat(FileHandle, &OpenedFileState) == 0 &&
        fstatat(
            DirectoryHandle,
            TCHAR_TO_UTF8(*LeafName),
            &DirectoryEntryState,
            AT_SYMLINK_NOFOLLOW) == 0 &&
        S_ISREG(DirectoryEntryState.st_mode) &&
        OpenedFileState.st_dev == DirectoryEntryState.st_dev &&
        OpenedFileState.st_ino == DirectoryEntryState.st_ino;
}

[[nodiscard]] bool RemoveOpenedDevelopmentCredentialFile(
    int DirectoryHandle,
    int FileHandle,
    const FString& LeafName)
{
    if (!OpenedDevelopmentCredentialFileStillNamesEntry(
            DirectoryHandle, FileHandle, LeafName) ||
        unlinkat(DirectoryHandle, TCHAR_TO_UTF8(*LeafName), 0) != 0)
    {
        return false;
    }
    struct stat UnlinkedFileState {};
    return fstat(FileHandle, &UnlinkedFileState) == 0 &&
        UnlinkedFileState.st_nlink == 0;
}

void CloseDevelopmentCredentialFile(
    int& DirectoryHandle,
    int& FileHandle)
{
    if (FileHandle >= 0)
    {
        close(FileHandle);
        FileHandle = -1;
    }
    if (DirectoryHandle >= 0)
    {
        close(DirectoryHandle);
        DirectoryHandle = -1;
    }
}
#endif

[[nodiscard]] bool ValidateDevelopmentCredentialStagingFile(
    const FString& Candidate,
    FString& OutNormalized,
    FString& OutReason)
{
    if (!NormalizeDevelopmentCredentialFilePath(
            Candidate, OutNormalized, OutReason))
    {
        return false;
    }
#if PLATFORM_MAC || PLATFORM_UNIX
    int DirectoryHandle = -1;
    int FileHandle = -1;
    FString LeafName;
    if (!OpenDevelopmentCredentialFile(
            OutNormalized,
            O_WRONLY,
            DirectoryHandle,
            FileHandle,
            LeafName,
            OutReason))
    {
        return false;
    }
    struct stat FileState {};
    const bool bEmpty =
        fstat(FileHandle, &FileState) == 0 && FileState.st_size == 0;
    const bool bEntryUnchanged =
        OpenedDevelopmentCredentialFileStillNamesEntry(
            DirectoryHandle, FileHandle, LeafName);
    CloseDevelopmentCredentialFile(DirectoryHandle, FileHandle);
    if (!bEntryUnchanged)
    {
        OutReason = TEXT("NET_RESUME_CREDENTIAL_FILE_ENTRY_CHANGED");
        return false;
    }
    if (!bEmpty)
    {
        OutReason = TEXT("NET_RESUME_CREDENTIAL_STAGING_FILE_NOT_EMPTY");
        return false;
    }
    return true;
#else
    OutReason = TEXT("NET_RESUME_CREDENTIAL_FILE_PLATFORM_UNSUPPORTED");
    return false;
#endif
}

[[nodiscard]] bool StageDevelopmentResumeCredential(
    const FString& NormalizedPath,
    const FString& Credential,
    FString& OutReason)
{
    if (!IsBoundedClientResumeCredential(Credential))
    {
        OutReason = TEXT("NET_RESUME_CREDENTIAL_CONTENT_INVALID");
        return false;
    }
#if PLATFORM_MAC || PLATFORM_UNIX
    int DirectoryHandle = -1;
    int FileHandle = -1;
    FString LeafName;
    if (!OpenDevelopmentCredentialFile(
            NormalizedPath,
            O_WRONLY,
            DirectoryHandle,
            FileHandle,
            LeafName,
            OutReason))
    {
        return false;
    }
    struct stat FileState {};
    if (fstat(FileHandle, &FileState) != 0 || FileState.st_size != 0 ||
        ftruncate(FileHandle, 0) != 0)
    {
        CloseDevelopmentCredentialFile(DirectoryHandle, FileHandle);
        OutReason = TEXT("NET_RESUME_CREDENTIAL_STAGING_FILE_INVALID");
        return false;
    }
    FTCHARToUTF8 CredentialUtf8(*Credential);
    int32 BytesWritten = 0;
    while (BytesWritten < CredentialUtf8.Length())
    {
        const ssize_t Result = write(
            FileHandle,
            CredentialUtf8.Get() + BytesWritten,
            static_cast<size_t>(CredentialUtf8.Length() - BytesWritten));
        if (Result <= 0)
        {
            (void)RemoveOpenedDevelopmentCredentialFile(
                DirectoryHandle, FileHandle, LeafName);
            CloseDevelopmentCredentialFile(DirectoryHandle, FileHandle);
            OutReason = TEXT("NET_RESUME_CREDENTIAL_FILE_WRITE_FAILED");
            return false;
        }
        BytesWritten += static_cast<int32>(Result);
    }
    if (fsync(FileHandle) != 0)
    {
        (void)RemoveOpenedDevelopmentCredentialFile(
            DirectoryHandle, FileHandle, LeafName);
        CloseDevelopmentCredentialFile(DirectoryHandle, FileHandle);
        OutReason = TEXT("NET_RESUME_CREDENTIAL_FILE_SYNC_FAILED");
        return false;
    }
    if (!OpenedDevelopmentCredentialFileStillNamesEntry(
            DirectoryHandle, FileHandle, LeafName))
    {
        CloseDevelopmentCredentialFile(DirectoryHandle, FileHandle);
        OutReason = TEXT("NET_RESUME_CREDENTIAL_FILE_ENTRY_CHANGED");
        return false;
    }
    CloseDevelopmentCredentialFile(DirectoryHandle, FileHandle);
    return true;
#else
    (void)NormalizedPath;
    OutReason = TEXT("NET_RESUME_CREDENTIAL_FILE_PLATFORM_UNSUPPORTED");
    return false;
#endif
}

[[nodiscard]] bool ConsumeDevelopmentResumeCredential(
    const FString& Candidate,
    FString& OutCredential,
    FString& OutReason)
{
    OutCredential.Reset();
    FString NormalizedPath;
    if (!NormalizeDevelopmentCredentialFilePath(
            Candidate, NormalizedPath, OutReason))
    {
        return false;
    }
#if PLATFORM_MAC || PLATFORM_UNIX
    int DirectoryHandle = -1;
    int FileHandle = -1;
    FString LeafName;
    if (!OpenDevelopmentCredentialFile(
            NormalizedPath,
            O_RDONLY,
            DirectoryHandle,
            FileHandle,
            LeafName,
            OutReason))
    {
        return false;
    }
    struct stat FileState {};
    const bool bExpectedSize =
        fstat(FileHandle, &FileState) == 0 &&
        FileState.st_size == DevelopmentResumeCredentialLength;
    if (!RemoveOpenedDevelopmentCredentialFile(
            DirectoryHandle, FileHandle, LeafName))
    {
        CloseDevelopmentCredentialFile(DirectoryHandle, FileHandle);
        OutReason = TEXT("NET_RESUME_CREDENTIAL_FILE_CONSUME_FAILED");
        return false;
    }
    close(DirectoryHandle);
    DirectoryHandle = -1;
    if (!bExpectedSize)
    {
        close(FileHandle);
        FileHandle = -1;
        OutReason = TEXT("NET_RESUME_CREDENTIAL_FILE_CONTENT_INVALID");
        return false;
    }
    char CredentialBytes[DevelopmentResumeCredentialLength + 1] = {};
    int32 BytesRead = 0;
    while (BytesRead < DevelopmentResumeCredentialLength)
    {
        const ssize_t Result = read(
            FileHandle,
            CredentialBytes + BytesRead,
            static_cast<size_t>(
                DevelopmentResumeCredentialLength - BytesRead));
        if (Result <= 0)
        {
            close(FileHandle);
            FileHandle = -1;
            FMemory::Memzero(CredentialBytes, sizeof(CredentialBytes));
            OutReason = TEXT("NET_RESUME_CREDENTIAL_FILE_READ_FAILED");
            return false;
        }
        BytesRead += static_cast<int32>(Result);
    }
    close(FileHandle);
    FileHandle = -1;
    OutCredential = FString(UTF8_TO_TCHAR(CredentialBytes));
    FMemory::Memzero(CredentialBytes, sizeof(CredentialBytes));
    if (!IsBoundedClientResumeCredential(OutCredential))
    {
        OutCredential.Reset();
        OutReason = TEXT("NET_RESUME_CREDENTIAL_FILE_CONTENT_INVALID");
        return false;
    }
    return true;
#else
    OutReason = TEXT("NET_RESUME_CREDENTIAL_FILE_PLATFORM_UNSUPPORTED");
    return false;
#endif
}
#endif
}

#if UE_BUILD_DEVELOPMENT && WITH_DEV_AUTOMATION_TESTS
namespace echoes::network::testing
{
bool ValidateDevelopmentCredentialStagingFile(
    const FString& Candidate,
    FString& OutNormalized,
    FString& OutReason)
{
    return ::ValidateDevelopmentCredentialStagingFile(
        Candidate, OutNormalized, OutReason);
}

bool StageDevelopmentResumeCredential(
    const FString& NormalizedPath,
    const FString& Credential,
    FString& OutReason)
{
    return ::StageDevelopmentResumeCredential(
        NormalizedPath, Credential, OutReason);
}

bool ConsumeDevelopmentResumeCredential(
    const FString& Candidate,
    FString& OutCredential,
    FString& OutReason)
{
    return ::ConsumeDevelopmentResumeCredential(
        Candidate, OutCredential, OutReason);
}
}
#endif

AEchoesPlayerController::AEchoesPlayerController()
{
    bShowMouseCursor = true;
    bEnableClickEvents = false;
    bEnableMouseOverEvents = true;
    DefaultMouseCursor = EMouseCursor::Crosshairs;
}

void AEchoesPlayerController::BeginPlay()
{
    Super::BeginPlay();

    FInputModeGameAndUI InputMode;
    InputMode.SetHideCursorDuringCapture(false);
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    if (UGameViewportClient* GameViewport =
            GetWorld() != nullptr ? GetWorld()->GetGameViewport() : nullptr)
    {
        InputMode.SetWidgetToFocus(GameViewport->GetGameViewportWidget());
    }
    SetInputMode(InputMode);
    bShowMouseCursor = true;
    if (UEchoesGameInstance* EchoesGameInstance = GetEchoesGameInstance())
    {
        EchoesGameInstance->NotifyControllerReady(this);
    }
    if (!bRuntimeStateKnown)
    {
        const UEchoesSimulationSubsystem* Bridge =
            GetWorld() != nullptr
                ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
                : nullptr;
        if (Bridge != nullptr && Bridge->IsScenarioReady())
        {
            NotifyRuntimeReady();
        }
        else
        {
            SetStatusMessage(
                TEXT("Initializing runtime technical prototype..."),
                15.0f);
        }
    }

    bNetworkClientSmoke =
        FParse::Param(FCommandLine::Get(), TEXT("EchoesNetworkClientSmoke"));
    bNetworkMatchSmoke =
        FParse::Param(
            FCommandLine::Get(), TEXT("EchoesNetworkMatchClientSmoke"));
#if UE_BUILD_DEVELOPMENT && WITH_DEV_AUTOMATION_TESTS
    bNetworkReconnectPhaseOneSmoke = FParse::Param(
        FCommandLine::Get(), TEXT("EchoesNetworkReconnectPhaseOne"));
    bNetworkReconnectPhaseTwoSmoke = FParse::Param(
        FCommandLine::Get(), TEXT("EchoesNetworkReconnectPhaseTwo"));
#else
    bNetworkReconnectPhaseOneSmoke = false;
    bNetworkReconnectPhaseTwoSmoke = false;
#endif
    if (GetNetMode() == NM_Client && bNetworkReconnectPhaseOneSmoke &&
        bNetworkReconnectPhaseTwoSmoke)
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NETWORK_RECONNECT_CLIENT_FAILED] reason=CONFLICTING_RECONNECT_PHASES"));
        FPlatformMisc::RequestExit(false);
        return;
    }
    bNetworkDelayFirstDeltaForSmoke = FParse::Param(
        FCommandLine::Get(), TEXT("EchoesNetworkDelayFirstDelta"));
    bNetworkDuplicateFirstDeltaForSmoke = FParse::Param(
        FCommandLine::Get(), TEXT("EchoesNetworkDuplicateFirstDelta"));
    bNetworkReorderFirstTwoDeltasForSmoke = FParse::Param(
        FCommandLine::Get(), TEXT("EchoesNetworkReorderFirstTwoDeltas"));
    bNetworkDropDeltaBurstForSmoke = FParse::Param(
        FCommandLine::Get(), TEXT("EchoesNetworkDropDeltaBurst"));
    const int32 NetworkFaultModeCount =
        (FParse::Param(
             FCommandLine::Get(), TEXT("EchoesNetworkDropFirstDelta"))
             ? 1
             : 0) +
        (bNetworkDelayFirstDeltaForSmoke ? 1 : 0) +
        (bNetworkDuplicateFirstDeltaForSmoke ? 1 : 0) +
        (bNetworkReorderFirstTwoDeltasForSmoke ? 1 : 0) +
        (bNetworkDropDeltaBurstForSmoke ? 1 : 0);
    if (GetNetMode() == NM_Client && NetworkFaultModeCount > 1)
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NETWORK_FAULT_MODE_FAILED] reason=CONFLICTING_FAULT_MODES count=%d"),
            NetworkFaultModeCount);
        FPlatformMisc::RequestExit(false);
        return;
    }
    if (GetNetMode() == NM_Client)
    {
        StartNetworkHandshakeTimeout();
        FString RequestedResumeCredential;
        bool bResumeSubmissionScheduled = false;
#if UE_BUILD_DEVELOPMENT && WITH_DEV_AUTOMATION_TESTS
        FString RequestedCredentialFile;
        const bool bCredentialFileProvided = FParse::Value(
            FCommandLine::Get(),
            TEXT("EchoesNetworkResumeCredentialFile="),
            RequestedCredentialFile);
        if (bNetworkReconnectPhaseOneSmoke ||
            bNetworkReconnectPhaseTwoSmoke)
        {
            if (!bCredentialFileProvided || RequestedCredentialFile.IsEmpty())
            {
                UE_LOG(
                    LogEchoes,
                    Error,
                    TEXT("[ECHOES_NETWORK_RECONNECT_CLIENT_FAILED] reason=NET_RESUME_CREDENTIAL_FILE_REQUIRED"));
                FPlatformMisc::RequestExit(false);
                return;
            }
            FString FileReason;
            if (bNetworkReconnectPhaseOneSmoke)
            {
                if (!ValidateDevelopmentCredentialStagingFile(
                        RequestedCredentialFile,
                        DevelopmentResumeCredentialFilePath,
                        FileReason))
                {
                    UE_LOG(
                        LogEchoes,
                        Error,
                        TEXT("[ECHOES_NETWORK_RECONNECT_CLIENT_FAILED] reason=%s credentialLogged=false"),
                        *FileReason);
                    FPlatformMisc::RequestExit(false);
                    return;
                }
            }
            else if (!ConsumeDevelopmentResumeCredential(
                         RequestedCredentialFile,
                         RequestedResumeCredential,
                         FileReason))
            {
                UE_LOG(
                    LogEchoes,
                    Error,
                    TEXT("[ECHOES_NETWORK_RECONNECT_CLIENT_FAILED] reason=%s credentialLogged=false credentialFileConsumed=false"),
                    *FileReason);
                FPlatformMisc::RequestExit(false);
                return;
            }
            else
            {
                NetworkResumeCredential = RequestedResumeCredential;
                GetWorldTimerManager().SetTimerForNextTick(
                    this,
                    &AEchoesPlayerController::SubmitNetworkResumeCredential);
                bResumeSubmissionScheduled = true;
                UE_LOG(
                    LogEchoes,
                    Display,
                    TEXT("[ECHOES_NETWORK_RESUME_CREDENTIAL_FILE] consumed=true deletedBeforeSubmit=true mode=owner_only credentialLogged=false"));
            }
        }
        else if (bCredentialFileProvided)
        {
            UE_LOG(
                LogEchoes,
                Error,
                TEXT("[ECHOES_NETWORK_RECONNECT_CLIENT_FAILED] reason=NET_RESUME_CREDENTIAL_FILE_OUTSIDE_DEVELOPMENT_SMOKE"));
            FPlatformMisc::RequestExit(false);
            return;
        }
#endif
        if (!bResumeSubmissionScheduled)
        {
            if (UEchoesGameInstance* EchoesGameInstance =
                    GetEchoesGameInstance();
                EchoesGameInstance != nullptr &&
                EchoesGameInstance->TryGetPendingReconnectCredential(
                    RequestedResumeCredential))
            {
                NetworkResumeCredential = RequestedResumeCredential;
                GetWorldTimerManager().SetTimerForNextTick(
                    this,
                    &AEchoesPlayerController::SubmitNetworkResumeCredential);
            }
            else
            {
                GetWorldTimerManager().SetTimerForNextTick(
                    this,
                    &AEchoesPlayerController::SubmitNetworkCompatibilityHello);
            }
        }
    }
#if UE_BUILD_DEVELOPMENT
    else if (GetNetMode() == NM_Standalone &&
             FParse::Param(
                 FCommandLine::Get(),
                 TEXT("EchoesOnlineFrontDoorHostSmoke")))
    {
        GetWorldTimerManager().SetTimerForNextTick(
            this,
            &AEchoesPlayerController::StartOnlineFrontDoorHostSmoke);
    }
    else if (GetNetMode() == NM_Standalone)
    {
        FString OnlineClientSmokeEndpoint;
        if (FParse::Value(
                FCommandLine::Get(),
                TEXT("EchoesOnlineFrontDoorClientSmoke="),
                OnlineClientSmokeEndpoint) &&
            !OnlineClientSmokeEndpoint.IsEmpty())
        {
            GetWorldTimerManager().SetTimerForNextTick(
                this,
                &AEchoesPlayerController::StartOnlineFrontDoorClientSmoke);
        }
    }
#endif
}

void AEchoesPlayerController::EndPlay(
    const EEndPlayReason::Type EndPlayReason)
{
    ClearNetworkConnectionTimeouts();
    GetWorldTimerManager().ClearTimer(NetworkResultAcknowledgementTimer);
    DestroyNetworkPresentation();
    Super::EndPlay(EndPlayReason);
}

bool AEchoesPlayerController::InputKey(const FInputKeyEventArgs& Params)
{
    if (HandleOnlineEndpointKey(Params))
    {
        return true;
    }
    return Super::InputKey(Params);
}

UEchoesGameInstance* AEchoesPlayerController::GetEchoesGameInstance() const
{
    return GetGameInstance<UEchoesGameInstance>();
}

bool AEchoesPlayerController::HandleOnlineEndpointKey(
    const FInputKeyEventArgs& Params)
{
    UEchoesGameInstance* EchoesGameInstance = GetEchoesGameInstance();
    if (EchoesGameInstance == nullptr ||
        EchoesGameInstance->GetOnlineState() !=
            EEchoesOnlineFrontDoorState::JoinSetup ||
        EchoesGameInstance->GetOnlineFocusIndex() != 1 ||
        (Params.Event != IE_Pressed && Params.Event != IE_Repeat))
    {
        return false;
    }
    if (Params.Key == EKeys::BackSpace)
    {
        (void)EchoesGameInstance->BackspaceEndpointCharacter();
        return true;
    }
    if (Params.Key == EKeys::Delete)
    {
        EchoesGameInstance->SetDirectConnectEndpoint(FString());
        return true;
    }
    if (Params.Key == EKeys::V && Params.Event == IE_Pressed &&
        (IsInputKeyDown(EKeys::LeftCommand) ||
         IsInputKeyDown(EKeys::RightCommand)))
    {
        FString ClipboardText;
        FPlatformApplicationMisc::ClipboardPaste(ClipboardText);
        EchoesGameInstance->SetDirectConnectEndpoint(
            ClipboardText.TrimStartAndEnd());
        return true;
    }

    const FString KeyName = Params.Key.GetFName().ToString();
    if (KeyName.Len() == 1 &&
        FChar::IsAlpha(KeyName[0]))
    {
        return EchoesGameInstance->AppendEndpointCharacter(KeyName[0]);
    }
    static const FKey NumberKeys[] = {
        EKeys::Zero, EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four,
        EKeys::Five, EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine};
    static const FKey NumpadKeys[] = {
        EKeys::NumPadZero, EKeys::NumPadOne, EKeys::NumPadTwo,
        EKeys::NumPadThree, EKeys::NumPadFour, EKeys::NumPadFive,
        EKeys::NumPadSix, EKeys::NumPadSeven, EKeys::NumPadEight,
        EKeys::NumPadNine};
    for (int32 Digit = 0; Digit < 10; ++Digit)
    {
        if (Params.Key == NumberKeys[Digit] ||
            Params.Key == NumpadKeys[Digit])
        {
            return EchoesGameInstance->AppendEndpointCharacter(
                static_cast<TCHAR>(TEXT('0') + Digit));
        }
    }
    if (Params.Key == EKeys::Period || Params.Key == EKeys::Decimal)
    {
        return EchoesGameInstance->AppendEndpointCharacter(TEXT('.'));
    }
    if (Params.Key == EKeys::Hyphen || Params.Key == EKeys::Subtract)
    {
        return EchoesGameInstance->AppendEndpointCharacter(TEXT('-'));
    }
    if (Params.Key == EKeys::Semicolon &&
        (IsInputKeyDown(EKeys::LeftShift) ||
         IsInputKeyDown(EKeys::RightShift)))
    {
        return EchoesGameInstance->AppendEndpointCharacter(TEXT(':'));
    }
    return false;
}

bool AEchoesPlayerController::IsOnlineFrontDoorVisible() const
{
    const UEchoesGameInstance* EchoesGameInstance =
        GetGameInstance<UEchoesGameInstance>();
    if (EchoesGameInstance == nullptr)
    {
        return false;
    }
    const EEchoesOnlineFrontDoorState State =
        EchoesGameInstance->GetOnlineState();
    return State == EEchoesOnlineFrontDoorState::JoinSetup ||
        State == EEchoesOnlineFrontDoorState::Hosting ||
        State == EEchoesOnlineFrontDoorState::Connecting ||
        State == EEchoesOnlineFrontDoorState::Failed;
}

void AEchoesPlayerController::OpenOnlineFrontDoor()
{
    UEchoesGameInstance* EchoesGameInstance = GetEchoesGameInstance();
    if (!bTitleScreenVisible || !IsLocalController() ||
        GetNetMode() != NM_Standalone || EchoesGameInstance == nullptr)
    {
        return;
    }
    EchoesGameInstance->OpenOnlineFrontDoor();
    ClearSelection();
    bSelectionButtonDown = false;
    SetIgnoreMoveInput(true);
    SetIgnoreLookInput(true);
    SetStatusMessage(
        TEXT("ONLINE 1v1 — host fixed Glass Scar rules or enter a direct host address."),
        3600.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_ONLINE_FRONT_DOOR_OPENED] source=operations fixedRules=true host=true directJoin=true"));
}

void AEchoesPlayerController::ConfirmOnlineFrontDoorAction()
{
    UEchoesGameInstance* EchoesGameInstance = GetEchoesGameInstance();
    if (EchoesGameInstance == nullptr)
    {
        return;
    }
    if (EchoesGameInstance->GetOnlineState() ==
        EEchoesOnlineFrontDoorState::Failed)
    {
        EchoesGameInstance->RetryOnlineFrontDoor(this);
        return;
    }
    if (EchoesGameInstance->GetOnlineState() ==
        EEchoesOnlineFrontDoorState::Hosting)
    {
        CopyOnlineHostEndpoint();
        return;
    }
    if (EchoesGameInstance->GetOnlineState() !=
        EEchoesOnlineFrontDoorState::JoinSetup)
    {
        return;
    }
    switch (EchoesGameInstance->GetOnlineFocusIndex())
    {
        case 0:
            (void)EchoesGameInstance->RequestFixedRulesHost(GetWorld());
            break;
        case 1:
        case 2:
            (void)EchoesGameInstance->RequestDirectJoin(this);
            break;
        case 3:
            EchoesGameInstance->CancelOnlineRequest(this);
            SetIgnoreMoveInput(true);
            SetIgnoreLookInput(true);
            break;
        default:
            break;
    }
}

void AEchoesPlayerController::CopyOnlineHostEndpoint()
{
    const UEchoesGameInstance* EchoesGameInstance = GetEchoesGameInstance();
    if (EchoesGameInstance == nullptr ||
        EchoesGameInstance->GetOnlineState() !=
            EEchoesOnlineFrontDoorState::Hosting ||
        EchoesGameInstance->GetHostShareEndpoint().IsEmpty())
    {
        SetStatusMessage(
            TEXT("[LAN_ADDRESS_UNAVAILABLE] This Mac has no usable LAN IPv4 address to copy."),
            6.0f);
        return;
    }
    FPlatformApplicationMisc::ClipboardCopy(
        *EchoesGameInstance->GetHostShareEndpoint());
    SetStatusMessage(
        TEXT("LAN DIRECT ADDRESS COPIED — share it only with the intended opponent."),
        6.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_ONLINE_HOST_ADDRESS_COPIED] endpoint=%s scope=lan_direct firewallNatNotAssured=true"),
        *EchoesGameInstance->GetHostShareEndpoint());
}

void AEchoesPlayerController::CancelOnlineFrontDoor()
{
    if (UEchoesGameInstance* EchoesGameInstance = GetEchoesGameInstance())
    {
        EchoesGameInstance->CancelOnlineRequest(this);
    }
}

void AEchoesPlayerController::LeaveOnlineMatch()
{
    const bool bActiveOnlineMatch = IsActiveOnlineNetworkMatch();
    if (!bActiveOnlineMatch && !CanLeaveNetworkMatchToOnlineMenu())
    {
        return;
    }
    if (bActiveOnlineMatch && !bOnlineLocalMenuVisible)
    {
        return;
    }
    if (bActiveOnlineMatch && GetNetMode() == NM_Client)
    {
        ServerLeaveNetworkMatch();
    }
    else if (bActiveOnlineMatch && GetNetMode() == NM_ListenServer)
    {
        AEchoesGameMode* GameMode =
            GetWorld() != nullptr
                ? GetWorld()->GetAuthGameMode<AEchoesGameMode>()
                : nullptr;
        if (GameMode == nullptr ||
            !GameMode->SurrenderNetworkHost(
                TEXT("NET_HOST_SURRENDERED")))
        {
            SetStatusMessage(
                TEXT("[ONLINE_LEAVE_FAILED] The host surrender could not be recorded."),
                6.0f);
        }
        return;
    }
    bOnlineLocalMenuVisible = false;
    if (UEchoesGameInstance* EchoesGameInstance = GetEchoesGameInstance())
    {
        EchoesGameInstance->ReturnToOnlineFrontDoor(this);
    }
}

void AEchoesPlayerController::BeginHostedNetworkMatchPresentation()
{
    if (!IsLocalController() || GetNetMode() != NM_ListenServer)
    {
        return;
    }
    // BeginNetworkMatch runs on the remote player's server-side controller.
    // Mirror the authoritative match state onto the local host controller so
    // local-only controls (pause/menu/result routing) cannot fall back to the
    // standalone skirmish path.
    NetworkSeat = 0;
    bNetworkCompatibilityAccepted = true;
    bNetworkReady = true;
    bNetworkMatchStarted = true;
    bTitleScreenVisible = false;
    bMissionBriefingVisible = false;
    bPauseMenuVisible = false;
    bOnlineLocalMenuVisible = false;
    bTechnologyPanelVisible = false;
    bMatchResultVisible = false;
    SetIgnoreMoveInput(false);
    SetIgnoreLookInput(false);
    SetStatusMessage(
        TEXT("ONLINE MATCH STARTED — you command the Meridian Compact on Glass Scar."),
        8.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_ONLINE_HOST_PRESENTATION_STARTED] title=false input=true seat=0"));
}

#if !UE_BUILD_SHIPPING
void AEchoesPlayerController::StartOnlineFrontDoorHostSmoke()
{
    UEchoesGameInstance* EchoesGameInstance = GetEchoesGameInstance();
    if (EchoesGameInstance == nullptr ||
        EchoesGameInstance->GetOnlineState() !=
            EEchoesOnlineFrontDoorState::Idle)
    {
        return;
    }
    EchoesGameInstance->OpenOnlineFrontDoor();
    if (EchoesGameInstance->RequestFixedRulesHost(GetWorld()))
    {
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_ONLINE_FRONT_DOOR_HOST_SMOKE] apiPath=true requested=true"));
    }
}

void AEchoesPlayerController::StartOnlineFrontDoorClientSmoke()
{
    FString Endpoint;
    if (!FParse::Value(
            FCommandLine::Get(),
            TEXT("EchoesOnlineFrontDoorClientSmoke="),
            Endpoint) || Endpoint.IsEmpty())
    {
        return;
    }
    UEchoesGameInstance* EchoesGameInstance = GetEchoesGameInstance();
    if (EchoesGameInstance == nullptr ||
        EchoesGameInstance->GetOnlineState() !=
            EEchoesOnlineFrontDoorState::Idle)
    {
        return;
    }
    EchoesGameInstance->OpenOnlineFrontDoor();
    EchoesGameInstance->SetDirectConnectEndpoint(Endpoint);
    EchoesGameInstance->FocusOnlineAction(2);
    if (EchoesGameInstance->RequestDirectJoin(this))
    {
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_ONLINE_FRONT_DOOR_CLIENT_SMOKE] apiPath=true requested=true endpoint=%s"),
            *Endpoint);
    }
}
#endif

bool AEchoesPlayerController::ResolvePointerScreenPosition(
    FVector2D& OutScreenPosition,
    FVector2D* OutViewportSize)
{
    int32 ViewportX = 0;
    int32 ViewportY = 0;
    GetViewportSize(ViewportX, ViewportY);
    const FVector2D ViewportSize(ViewportX, ViewportY);
    if (OutViewportSize != nullptr)
    {
        *OutViewportSize = ViewportSize;
    }

    float CachedMouseX = 0.0f;
    float CachedMouseY = 0.0f;
    const bool bCachedMouseAvailable =
        GetMousePosition(CachedMouseX, CachedMouseY);
    if (ViewportX > 0 && ViewportY > 0 &&
        FSlateApplication::IsInitialized())
    {
        const UGameViewportClient* GameViewport =
            GetWorld() != nullptr ? GetWorld()->GetGameViewport() : nullptr;
        const TSharedPtr<SViewport> ViewportWidget =
            GameViewport != nullptr
                ? GameViewport->GetGameViewportWidget()
                : nullptr;
        if (ViewportWidget.IsValid())
        {
            const FGeometry& Geometry = ViewportWidget->GetCachedGeometry();
            const FVector2D LocalSize(
                Geometry.GetLocalSize().X,
                Geometry.GetLocalSize().Y);
            if (LocalSize.X > UE_SMALL_NUMBER &&
                LocalSize.Y > UE_SMALL_NUMBER)
            {
                const FVector2D AbsolutePosition =
                    FSlateApplication::Get().GetCursorPos();
                const FVector2D LocalPosition =
                    Geometry.AbsoluteToLocal(AbsolutePosition);
                const FVector2D ResolvedPosition(
                    LocalPosition.X * ViewportSize.X / LocalSize.X,
                    LocalPosition.Y * ViewportSize.Y / LocalSize.Y);
                if (FMath::IsFinite(ResolvedPosition.X) &&
                    FMath::IsFinite(ResolvedPosition.Y))
                {
                    OutScreenPosition = ResolvedPosition;
                    return true;
                }
            }
        }
    }

    if (bCachedMouseAvailable)
    {
        OutScreenPosition = FVector2D(CachedMouseX, CachedMouseY);
        return true;
    }
    return false;
}

void AEchoesPlayerController::ConfigureNetworkSeat(uint8 Seat)
{
    if (!HasAuthority() || Seat >= echoes::sim::kMaximumPlayers)
    {
        return;
    }
    NetworkSeat = Seat;
    NetworkCommandContext = {};
    NetworkCommandContext.player = Seat;
    NetworkCommandContext.minimumInputDelayTicks = 3;
    NetworkCommandContext.maximumLeadTicks = 40;
}

void AEchoesPlayerController::ConfigureNetworkResume(
    uint8 Seat,
    uint64 RestoredLastAcceptedBatchId,
    uint64 DisconnectTick,
    bool bMatchWasStarted)
{
    ConfigureNetworkSeat(Seat);
    if (!HasAuthority() || Seat >= echoes::sim::kMaximumPlayers)
    {
        return;
    }
    LastAcceptedNetworkBatchId = RestoredLastAcceptedBatchId;
    NetworkResumeDisconnectTick = DisconnectTick;
    bNetworkResumePending = true;
    bNetworkResumeMatchWasStarted = bMatchWasStarted;
}

void AEchoesPlayerController::ConfigureNetworkResumeCredential(
    const FString& Credential)
{
    if (!HasAuthority())
    {
        return;
    }
    NetworkResumeCredential = Credential;
}

void AEchoesPlayerController::RejectNetworkSessionFromServer(
    const FString& StableReason)
{
    if (!HasAuthority())
    {
        return;
    }
    ClientReceiveOnlineSessionFailure(StableReason.Left(160));
}

void AEchoesPlayerController::PresentNetworkReconnectGrace(
    float GraceSeconds)
{
    if (!IsLocalController() || GetNetMode() != NM_ListenServer ||
        GraceSeconds <= 0.0f)
    {
        return;
    }
    bOpponentReconnectGraceActive = true;
    OpponentReconnectExpiresAtSeconds =
        FPlatformTime::Seconds() + static_cast<double>(GraceSeconds);
    bOnlineLocalMenuVisible = false;
    bTechnologyPanelVisible = false;
    SetIgnoreMoveInput(true);
    SetIgnoreLookInput(true);
    SetStatusMessage(
        TEXT("OPPONENT DISCONNECTED — the authority is paused while their seat is reserved."),
        GraceSeconds);
}

void AEchoesPlayerController::ClearNetworkReconnectGrace()
{
    bOpponentReconnectGraceActive = false;
    OpponentReconnectExpiresAtSeconds = 0.0;
    if (!bMatchResultVisible && !bOnlineLocalMenuVisible &&
        !bPauseMenuVisible && !bTechnologyPanelVisible)
    {
        SetIgnoreMoveInput(false);
        SetIgnoreLookInput(false);
    }
}

void AEchoesPlayerController::NotifyNetworkOpponentForfeit(
    uint64 FinalTick,
    const FString& StableReason,
    bool bWaitForResultRecipient)
{
    if (!IsLocalController() || GetNetMode() != NM_ListenServer)
    {
        return;
    }
    ClearNetworkReconnectGrace();
    PresentedFinalTick = FinalTick;
    bNetworkResultExitEnabled = !bWaitForResultRecipient;
    NotifyMatchFinished(echoes::sim::MatchOutcome::Player0Victory);
    SetStatusMessage(
        bWaitForResultRecipient
            ? TEXT("VICTORY BY FORFEIT — confirming final delivery before the Online menu unlocks.")
            : TEXT("VICTORY BY FORFEIT — the opponent did not return before reconnect grace expired. Enter leaves for the Online menu."),
        3600.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_NETWORK_FORFEIT_RESULT] outcome=1 finalTick=%llu reason=%s resultRecipient=%s hostLeaveEnabled=%s"),
        static_cast<unsigned long long>(FinalTick),
        *StableReason.Left(96),
        bWaitForResultRecipient ? TEXT("true") : TEXT("false"),
        bWaitForResultRecipient ? TEXT("false") : TEXT("true"));
}

void AEchoesPlayerController::NotifyNetworkHostSurrender(
    uint64 FinalTick,
    const FString& StableReason)
{
    if (!IsLocalController() || GetNetMode() != NM_ListenServer)
    {
        return;
    }
    ClearNetworkReconnectGrace();
    PresentedFinalTick = FinalTick;
    bNetworkResultExitEnabled = false;
    bReturnHostToOnlineAfterResultDelivery = true;
    NotifyMatchFinished(echoes::sim::MatchOutcome::Player1Victory);
    SetStatusMessage(
        TEXT("MATCH SURRENDERED — delivering the final result before returning to the Online menu."),
        12.0f);
    GetWorldTimerManager().SetTimer(
        NetworkResultAcknowledgementTimer,
        this,
        &AEchoesPlayerController::AllowHostNetworkResultExitAfterTimeout,
        10.0f,
        false);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_NETWORK_HOST_SURRENDER_RESULT] outcome=2 finalTick=%llu reason=%s returnAfterAck=true"),
        static_cast<unsigned long long>(FinalTick),
        *StableReason.Left(96));
}

bool AEchoesPlayerController::IsOpponentReconnectGraceActive() const
{
    return bOpponentReconnectGraceActive &&
        GetOpponentReconnectSecondsRemaining() > 0;
}

int32 AEchoesPlayerController::GetOpponentReconnectSecondsRemaining() const
{
    if (!bOpponentReconnectGraceActive)
    {
        return 0;
    }
    return FMath::Max(
        0,
        FMath::CeilToInt(
            OpponentReconnectExpiresAtSeconds - FPlatformTime::Seconds()));
}

bool AEchoesPlayerController::IsNetworkClientControlActive() const
{
    return GetNetMode() == NM_Client && IsLocalController() &&
           bNetworkCompatibilityAccepted && bNetworkMatchStarted &&
           NetworkSeat < echoes::sim::kMaximumPlayers &&
           GetNetworkScopedView() != nullptr && !bMatchResultVisible;
}

const echoes::sim::net::ScopedEntityState*
AEchoesPlayerController::FindNetworkEntity(uint32 EntityId) const
{
    const echoes::sim::net::ScopedViewKeyframe* View = GetNetworkScopedView();
    if (View == nullptr)
    {
        return nullptr;
    }
    const auto Entity = std::lower_bound(
        View->entities.begin(),
        View->entities.end(),
        EntityId,
        [](const echoes::sim::net::ScopedEntityState& Candidate, uint32 Id)
        {
            return Candidate.id < Id;
        });
    return Entity != View->entities.end() && Entity->id == EntityId
               ? &*Entity
               : nullptr;
}

FVector AEchoesPlayerController::NetworkSimToWorld(
    const echoes::sim::Vec2& Position) const
{
    const echoes::sim::net::ScopedViewKeyframe* View = GetNetworkScopedView();
    if (View == nullptr)
    {
        return FVector::ZeroVector;
    }
    const float MapHalfX = static_cast<float>(View->mapWidthTiles) * 0.5f;
    const float MapHalfY = static_cast<float>(View->mapHeightTiles) * 0.5f;
    const float TileX = static_cast<float>(Position.x.Raw()) /
                        static_cast<float>(echoes::sim::kFixedScale);
    const float TileY = static_cast<float>(Position.y.Raw()) /
                        static_cast<float>(echoes::sim::kFixedScale);
    return FVector(
        (TileX - MapHalfX) * NetworkTileWorldSize,
        (TileY - MapHalfY) * NetworkTileWorldSize,
        0.0f);
}

echoes::sim::Vec2 AEchoesPlayerController::NetworkWorldToSim(
    const FVector& Position) const
{
    const echoes::sim::net::ScopedViewKeyframe* View = GetNetworkScopedView();
    if (View == nullptr)
    {
        return {};
    }
    const double MapHalfX = static_cast<double>(View->mapWidthTiles) * 0.5;
    const double MapHalfY = static_cast<double>(View->mapHeightTiles) * 0.5;
    return echoes::sim::Vec2::FromRaw(
        FMath::RoundToInt32(
            (static_cast<double>(Position.X) / NetworkTileWorldSize + MapHalfX) *
            echoes::sim::kFixedScale),
        FMath::RoundToInt32(
            (static_cast<double>(Position.Y) / NetworkTileWorldSize + MapHalfY) *
            echoes::sim::kFixedScale));
}

bool AEchoesPlayerController::SubmitNetworkCommandBatch(
    TArray<echoes::sim::net::CommandIntent> Intents,
    const FString& OrderLabel,
    const FVector& MarkerLocation,
    EEchoesCommandMarkerType MarkerType)
{
    if (!IsNetworkClientControlActive())
    {
        SetStatusMessage(TEXT("[NETWORK_NOT_READY] The remote battlefield is not accepting orders."));
        return false;
    }
    if (Intents.IsEmpty() ||
        Intents.Num() >
            static_cast<int32>(echoes::sim::net::kMaximumCommandsPerBatch) ||
        NextNetworkBatchId == 0 ||
        NextNetworkBatchId == std::numeric_limits<uint64>::max())
    {
        SetStatusMessage(TEXT("[NETWORK_BATCH_INVALID] The selected order exceeds the bounded command batch."));
        return false;
    }
    Intents.Sort(
        [](const echoes::sim::net::CommandIntent& Left,
           const echoes::sim::net::CommandIntent& Right)
        {
            return Left.actor < Right.actor;
        });
    for (int32 Index = 1; Index < Intents.Num(); ++Index)
    {
        if (Intents[Index - 1].actor == Intents[Index].actor)
        {
            SetStatusMessage(TEXT("[NETWORK_BATCH_DUPLICATE] Each selected actor may receive one order per gesture."));
            return false;
        }
    }

    echoes::sim::net::CommandBatchRequest Request{};
    Request.clientBatchId = NextNetworkBatchId++;
    Request.intents.reserve(static_cast<std::size_t>(Intents.Num()));
    for (const echoes::sim::net::CommandIntent& Intent : Intents)
    {
        Request.intents.push_back(Intent);
    }
    const std::vector<std::uint8_t> Encoded =
        echoes::sim::net::EncodeCommandBatchRequest(Request);
    if (Encoded.empty())
    {
        SetStatusMessage(TEXT("[NETWORK_BATCH_ENCODING_FAILED] The order was not sent."));
        return false;
    }
    ServerSubmitNetworkCommandBatch(echoes::network::ToByteArray(Encoded));
    SetStatusMessage(
        FString::Printf(
            TEXT("%s: %d actor intent%s submitted for authority admission."),
            *OrderLabel,
            Intents.Num(),
            Intents.Num() == 1 ? TEXT("") : TEXT("s")),
        4.0f);
    ShowAcceptedCommandMarker(
        MarkerLocation, MarkerType, Intents.Num());
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_NETWORK_COMMAND_BATCH_SENT] batch=%llu intents=%d bytes=%d canonicalActors=true authorityAssignsSequence=true authorityAssignsTick=true"),
        static_cast<unsigned long long>(Request.clientBatchId),
        Intents.Num(),
        static_cast<int32>(Encoded.size()));
    return true;
}

bool AEchoesPlayerController::SubmitNetworkSelectionCommand(
    echoes::sim::CommandType CommandType,
    uint32 TargetId,
    const FVector& Destination,
    bool bUseFormation,
    bool bUseActorPosition,
    const FString& OrderLabel,
    EEchoesCommandMarkerType MarkerType,
    echoes::sim::EntityType BuildType,
    echoes::sim::WarformAdaptation Adaptation,
    echoes::sim::ResearchType Research)
{
    PruneSelection();
    if (SelectedEntityIds.IsEmpty())
    {
        SetStatusMessage(TEXT("[NO_SELECTION] Select one or more owned entities first."));
        return false;
    }
    const TArray<FVector> FormationDestinations =
        bUseFormation
            ? BuildSelectedFormationDestinations(
                  Destination, SelectedEntityIds.Num())
            : TArray<FVector>{};
    TArray<echoes::sim::net::CommandIntent> Intents;
    Intents.Reserve(SelectedEntityIds.Num());
    for (int32 Index = 0; Index < SelectedEntityIds.Num(); ++Index)
    {
        const echoes::sim::net::ScopedEntityState* Actor =
            FindNetworkEntity(SelectedEntityIds[Index]);
        if (Actor == nullptr || Actor->owner != NetworkSeat)
        {
            continue;
        }
        echoes::sim::net::CommandIntent Intent{};
        Intent.type = CommandType;
        Intent.actor = Actor->id;
        Intent.target = TargetId;
        Intent.position = bUseActorPosition
            ? Actor->position
            : NetworkWorldToSim(
                  bUseFormation ? FormationDestinations[Index] : Destination);
        Intent.wellChoice = FutureWellChoice;
        Intent.buildType = BuildType;
        Intent.warformAdaptation = Adaptation;
        Intent.researchType = Research;
        Intents.Add(Intent);
    }
    return SubmitNetworkCommandBatch(
        MoveTemp(Intents), OrderLabel, Destination, MarkerType);
}

void AEchoesPlayerController::SubmitNetworkResumeCredential()
{
    if (!IsLocalController() || GetNetMode() != NM_Client ||
        NetworkResumeCredential.IsEmpty())
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NETWORK_RESUME_CLIENT_FAILED] reason=NET_RESUME_CREDENTIAL_UNAVAILABLE"));
        HandlePlayerOnlineFailure(
            TEXT("NET_RESUME_CREDENTIAL_UNAVAILABLE"), false);
        return;
    }
    ServerSubmitNetworkResumeCredential(NetworkResumeCredential);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_NETWORK_RESUME_CREDENTIAL_SENT] bytes=%d transport=reliableRpc credentialLogged=false compatibilityDeferred=true"),
        NetworkResumeCredential.Len());
}

void AEchoesPlayerController::ServerSubmitNetworkResumeCredential_Implementation(
    const FString& Credential)
{
    AEchoesGameMode* GameMode =
        GetWorld() != nullptr
            ? GetWorld()->GetAuthGameMode<AEchoesGameMode>()
            : nullptr;
    FString Error;
    const bool bAccepted =
        GameMode != nullptr &&
        GameMode->TryResumeNetworkPlayer(this, Credential, Error);
    if (!bAccepted && Error.IsEmpty())
    {
        Error = TEXT("NET_RESUME_AUTHORITY_UNAVAILABLE");
    }
    ClientReceiveNetworkResumeCredentialResult(
        bAccepted,
        bAccepted ? TEXT("NET_RESUME_CREDENTIAL_ACCEPTED") : Error);
    if (!bAccepted && GameMode != nullptr)
    {
        GameMode->RejectNetworkResumeAttempt(this, Error);
    }
}

void AEchoesPlayerController::ClientReceiveNetworkResumeCredentialResult_Implementation(
    bool bAccepted,
    const FString& StableReason)
{
    if (bAccepted)
    {
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_NETWORK_RESUME_CREDENTIAL_RESULT] accepted=true reason=%s credentialLogged=false"),
            *StableReason);
    }
    else
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NETWORK_RESUME_CREDENTIAL_RESULT] accepted=false reason=%s credentialLogged=false"),
            *StableReason);
    }
    if (!bAccepted)
    {
        ClearNetworkConnectionTimeouts();
        if (bNetworkReconnectPhaseOneSmoke ||
            bNetworkReconnectPhaseTwoSmoke)
        {
            FPlatformMisc::RequestExit(false);
        }
        else
        {
            HandlePlayerOnlineFailure(StableReason, false);
        }
        return;
    }
    if (UEchoesGameInstance* EchoesGameInstance = GetEchoesGameInstance())
    {
        EchoesGameInstance->MarkReconnectAttemptAccepted();
    }
    SubmitNetworkCompatibilityHello();
}

void AEchoesPlayerController::ClientReceiveOnlineSessionFailure_Implementation(
    const FString& StableReason)
{
    UE_LOG(
        LogEchoes,
        Error,
        TEXT("[ECHOES_NETWORK_SESSION_REJECTED] reason=%s playerFacing=true"),
        *StableReason.Left(160));
    if (bNetworkClientSmoke || bNetworkMatchSmoke ||
        bNetworkReconnectPhaseOneSmoke || bNetworkReconnectPhaseTwoSmoke)
    {
        FPlatformMisc::RequestExit(false);
        return;
    }
    HandlePlayerOnlineFailure(StableReason, false);
}

void AEchoesPlayerController::SubmitNetworkCompatibilityHello()
{
    if (!IsLocalController() || GetNetMode() != NM_Client)
    {
        return;
    }
    const echoes::sim::net::CompatibilityManifest Manifest =
        echoes::network::BuildCompatibilityManifest();
    const std::vector<std::uint8_t> Encoded =
        echoes::sim::net::EncodeCompatibilityHello(Manifest);
    if (Encoded.empty())
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NETWORK_CLIENT_FAILED] reason=NET_HELLO_ENCODING_FAILED"));
        HandlePlayerOnlineFailure(TEXT("NET_HELLO_ENCODING_FAILED"), false);
        return;
    }
    ServerSubmitCompatibilityHello(echoes::network::ToByteArray(Encoded));
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_NETWORK_HELLO_SENT] bytes=%d protocol=%u snapshotSchema=%u playerViewSchema=%u"),
        static_cast<int32>(Encoded.size()),
        Manifest.protocolVersion,
        Manifest.snapshotVersion,
        Manifest.playerViewSchemaVersion);
}

void AEchoesPlayerController::RejectNetworkCompatibility(
    const FString& StableReason)
{
    if (!HasAuthority())
    {
        return;
    }
    ClientReceiveCompatibilityResult(false, StableReason.Left(160));
    if (AEchoesGameMode* GameMode =
            GetWorld() != nullptr
                ? GetWorld()->GetAuthGameMode<AEchoesGameMode>()
                : nullptr)
    {
        GameMode->ReleaseNetworkSeat(this, StableReason, true);
    }
}

void AEchoesPlayerController::ServerSubmitCompatibilityHello_Implementation(
    const TArray<uint8>& Packet)
{
    AEchoesGameMode* GameMode =
        GetWorld() != nullptr
            ? GetWorld()->GetAuthGameMode<AEchoesGameMode>()
            : nullptr;
    if (GameMode == nullptr || !GameMode->IsBoundNetworkController(this))
    {
        if (GameMode != nullptr &&
            GameMode->IsAwaitingNetworkResumeCredential(this))
        {
            constexpr const TCHAR* ResumeReason =
                TEXT("NET_RESUME_CREDENTIAL_REQUIRED");
            GameMode->RejectNetworkResumeAttempt(this, ResumeReason);
            return;
        }
        const FString Reason = TEXT("NET_CONNECTION_NOT_BOUND");
        ClientReceiveCompatibilityResult(false, Reason);
        RejectNetworkSessionFromServer(Reason);
        ClientReturnToMainMenuWithTextReason(FText::FromString(Reason));
        return;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    const echoes::sim::Simulation* Simulation =
        Bridge != nullptr ? Bridge->GetSimulation() : nullptr;
    if (NetworkSeat >= echoes::sim::kMaximumPlayers || Bridge == nullptr ||
        Simulation == nullptr || !Bridge->IsScenarioReady())
    {
        RejectNetworkCompatibility(TEXT("NET_AUTHORITY_NOT_READY"));
        return;
    }
    if (!echoes::network::SupportsNetworkSession(Simulation) ||
        Bridge->GetOperationMode() != EEchoesOperationMode::Skirmish ||
        !FEchoesSkirmishSetupModel::IsCanonicalOnlineSetup(
            Bridge->GetActiveSkirmishSetup()))
    {
        constexpr const TCHAR* Reason = TEXT("NET_MATCH_SETTINGS_MISMATCH");
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NETWORK_COMPATIBILITY_REJECTED] player=%u reason=%s"),
            NetworkSeat,
            Reason);
        RejectNetworkCompatibility(Reason);
        return;
    }

    echoes::sim::net::CompatibilityManifest Remote{};
    const echoes::sim::net::DecodeStatus Decode =
        echoes::sim::net::DecodeCompatibilityHello(
            echoes::network::AsByteSpan(Packet), Remote);
    if (Decode != echoes::sim::net::DecodeStatus::Ok)
    {
        const FString Reason(
            UTF8_TO_TCHAR(echoes::sim::net::StableId(Decode).data()));
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NETWORK_COMPATIBILITY_REJECTED] player=%u reason=%s"),
            NetworkSeat,
            *Reason);
        RejectNetworkCompatibility(Reason);
        return;
    }

    const echoes::sim::net::CompatibilityManifest Authority =
        echoes::network::BuildCompatibilityManifest(Simulation);
    const echoes::sim::net::CompatibilityStatus Compatibility =
        echoes::sim::net::CheckCompatibility(Authority, Remote);
    if (Compatibility != echoes::sim::net::CompatibilityStatus::Accepted)
    {
        const FString Reason(
            UTF8_TO_TCHAR(echoes::sim::net::StableId(Compatibility).data()));
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NETWORK_COMPATIBILITY_REJECTED] player=%u reason=%s"),
            NetworkSeat,
            *Reason);
        RejectNetworkCompatibility(Reason);
        return;
    }

    const std::optional<std::uint64_t> NextSequence =
        Simulation->NextCommandSequence(NetworkSeat);
    if (!NextSequence.has_value() || *NextSequence == 0)
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NETWORK_COMPATIBILITY_REJECTED] player=%u reason=NET_COMMAND_SEQUENCE_UNAVAILABLE"),
            NetworkSeat);
        RejectNetworkCompatibility(TEXT("NET_COMMAND_SEQUENCE_UNAVAILABLE"));
        return;
    }
    NetworkCommandContext.hasAcceptedSequence = *NextSequence > 1;
    NetworkCommandContext.lastAcceptedSequence = *NextSequence - 1;

    if (!GameMode->NotifyNetworkCompatibilityAccepted(this))
    {
        RejectNetworkCompatibility(
            TEXT("NET_RESUME_ADMISSION_EXPIRED"));
        return;
    }
    bNetworkCompatibilityAccepted = true;
    ClientReceiveCompatibilityResult(true, TEXT("NET_COMPATIBLE"));
    if (bNetworkResumePending && bNetworkResumeMatchWasStarted)
    {
        bNetworkReady = true;
        if (ResumeNetworkMatch() && GameMode != nullptr)
        {
            GameMode->NotifyNetworkPlayerReady(this);
            ClientReceiveNetworkResumeCredential(
                NetworkResumeCredential, 120.0f);
        }
        return;
    }
    if (bNetworkResumePending)
    {
        ClientReceiveNetworkResumeState(
            true,
            LastAcceptedNetworkBatchId + 1,
            NetworkCommandContext.lastAcceptedSequence,
            Simulation->CurrentTick(),
            NetworkResumeDisconnectTick);
    }
    else
    {
        ClientReceiveNetworkResumeCredential(
            NetworkResumeCredential, 120.0f);
    }
    ClientReceiveNetworkLobbyState(
        false, NetworkSeat, Simulation->CurrentTick(), 3);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_NETWORK_LOBBY] player=%u compatible=true ready=false started=false authorityTick=%llu"),
        NetworkSeat,
        static_cast<unsigned long long>(Simulation->CurrentTick()));
}

void AEchoesPlayerController::ClientReceiveCompatibilityResult_Implementation(
    bool bAccepted,
    const FString& StableReason)
{
    bNetworkCompatibilityAccepted = bAccepted;
    UEchoesGameInstance* EchoesGameInstance = GetEchoesGameInstance();
    if (bAccepted)
    {
        GetWorldTimerManager().ClearTimer(NetworkHandshakeTimer);
        StartNetworkReadyTimeout();
        if (EchoesGameInstance != nullptr)
        {
            EchoesGameInstance->MarkClientLobby();
        }
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_NETWORK_COMPATIBILITY_RESULT] accepted=true reason=%s"),
            *StableReason);
        if (bNetworkClientSmoke || bNetworkMatchSmoke ||
            bNetworkReconnectPhaseOneSmoke ||
            bNetworkReconnectPhaseTwoSmoke ||
            FParse::Param(
                FCommandLine::Get(), TEXT("EchoesNetworkVisualReview")))
        {
            ServerSetNetworkReady();
        }
        else
        {
            SetStatusMessage(
                TEXT("ONLINE LOBBY — compatibility accepted. Press Enter when ready."),
                3600.0f);
        }
    }
    else
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NETWORK_COMPATIBILITY_RESULT] accepted=false reason=%s"),
            *StableReason);
        ClearNetworkConnectionTimeouts();
        if (EchoesGameInstance != nullptr &&
            EchoesGameInstance->IsPlayerInitiatedOnlineSession())
        {
            HandlePlayerOnlineFailure(StableReason, false);
        }
    }
    if (!bAccepted &&
        (bNetworkClientSmoke || bNetworkMatchSmoke ||
         bNetworkReconnectPhaseOneSmoke || bNetworkReconnectPhaseTwoSmoke))
    {
        FPlatformMisc::RequestExit(false);
    }
}

void AEchoesPlayerController::ServerSetNetworkReady_Implementation()
{
    AEchoesGameMode* GameMode =
        GetWorld() != nullptr
            ? GetWorld()->GetAuthGameMode<AEchoesGameMode>()
            : nullptr;
    if (!bNetworkCompatibilityAccepted || bNetworkReady ||
        NetworkSeat >= echoes::sim::kMaximumPlayers || GameMode == nullptr ||
        !GameMode->IsBoundNetworkController(this))
    {
        return;
    }
    bNetworkReady = true;
    const bool bCompletingPreMatchResume = bNetworkResumePending;
    GameMode->NotifyNetworkPlayerReady(this);
    if (bCompletingPreMatchResume)
    {
        ClientReceiveNetworkResumeCredential(
            NetworkResumeCredential, 120.0f);
    }
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_NETWORK_LOBBY] player=%u compatible=true ready=true started=false"),
        NetworkSeat);
    BeginNetworkMatch();
}

void AEchoesPlayerController::ServerLeaveNetworkMatch_Implementation()
{
    AEchoesGameMode* GameMode =
        GetWorld() != nullptr
            ? GetWorld()->GetAuthGameMode<AEchoesGameMode>()
            : nullptr;
    if (!HasAuthority() || !bNetworkMatchStarted || GameMode == nullptr ||
        !GameMode->IsBoundNetworkController(this))
    {
        return;
    }
    (void)GameMode->ForfeitNetworkOpponent(
        TEXT("NET_PLAYER_LEFT_MATCH"));
}

void AEchoesPlayerController::StartNetworkHandshakeTimeout()
{
    if (!IsLocalController() || GetNetMode() != NM_Client)
    {
        return;
    }
    GetWorldTimerManager().SetTimer(
        NetworkHandshakeTimer,
        this,
        &AEchoesPlayerController::HandleNetworkHandshakeTimeout,
        15.0f,
        false);
}

void AEchoesPlayerController::StartNetworkReadyTimeout()
{
    if (!IsLocalController() || GetNetMode() != NM_Client)
    {
        return;
    }
    GetWorldTimerManager().SetTimer(
        NetworkReadyTimer,
        this,
        &AEchoesPlayerController::HandleNetworkReadyTimeout,
        45.0f,
        false);
}

void AEchoesPlayerController::ClearNetworkConnectionTimeouts()
{
    GetWorldTimerManager().ClearTimer(NetworkHandshakeTimer);
    GetWorldTimerManager().ClearTimer(NetworkReadyTimer);
}

void AEchoesPlayerController::HandleNetworkHandshakeTimeout()
{
    if (bNetworkCompatibilityAccepted)
    {
        return;
    }
    if (bNetworkClientSmoke || bNetworkMatchSmoke ||
        bNetworkReconnectPhaseOneSmoke || bNetworkReconnectPhaseTwoSmoke)
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NETWORK_CLIENT_FAILED] reason=ONLINE_HANDSHAKE_TIMEOUT"));
        FPlatformMisc::RequestExit(false);
        return;
    }
    HandlePlayerOnlineFailure(TEXT("ONLINE_HANDSHAKE_TIMEOUT"), false);
}

void AEchoesPlayerController::HandleNetworkReadyTimeout()
{
    if (bNetworkMatchStarted)
    {
        return;
    }
    if (bNetworkClientSmoke || bNetworkMatchSmoke ||
        bNetworkReconnectPhaseOneSmoke || bNetworkReconnectPhaseTwoSmoke)
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NETWORK_CLIENT_FAILED] reason=ONLINE_READY_TIMEOUT"));
        FPlatformMisc::RequestExit(false);
        return;
    }
    HandlePlayerOnlineFailure(TEXT("ONLINE_READY_TIMEOUT"), false);
}

void AEchoesPlayerController::HandlePlayerOnlineFailure(
    const FString& StableReason,
    bool bPreserveReconnect)
{
    ClearNetworkConnectionTimeouts();
    UEchoesGameInstance* EchoesGameInstance = GetEchoesGameInstance();
    if (EchoesGameInstance == nullptr)
    {
        return;
    }
    EchoesGameInstance->ReportOnlineFailure(
        StableReason, bPreserveReconnect);
    EchoesGameInstance->ReturnToFailedFrontDoor(this);
}

void AEchoesPlayerController::BeginNetworkMatch()
{
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    const echoes::sim::Simulation* Simulation =
        Bridge != nullptr ? Bridge->GetSimulation() : nullptr;
    if (!HasAuthority() || !bNetworkReady || bNetworkMatchStarted ||
        Bridge == nullptr || Simulation == nullptr)
    {
        return;
    }
    bNetworkMatchStarted = true;
    Bridge->SetNetworkHumanOpponent(true);
    if (FParse::Param(
            FCommandLine::Get(), TEXT("EchoesNetworkListenSmoke")))
    {
        QueueNetworkSmokeHostCommand();
    }
    Bridge->SetScenarioPaused(false);
    if (UEchoesGameInstance* EchoesGameInstance = GetEchoesGameInstance())
    {
        EchoesGameInstance->MarkNetworkMatchStarted();
    }
    AEchoesPlayerController* HostController = nullptr;
    if (GetWorld() != nullptr)
    {
        for (FConstPlayerControllerIterator It =
                 GetWorld()->GetPlayerControllerIterator();
             It;
             ++It)
        {
            AEchoesPlayerController* Candidate =
                Cast<AEchoesPlayerController>(It->Get());
            if (Candidate != nullptr && Candidate->IsLocalController())
            {
                HostController = Candidate;
                break;
            }
        }
    }
    if (HostController != nullptr)
    {
        HostController->bNetworkResultExitEnabled = false;
        HostController->BeginHostedNetworkMatchPresentation();
    }
    ClientReceiveNetworkLobbyState(
        true, NetworkSeat, Simulation->CurrentTick(), 3);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_NETWORK_MATCH_STARTED] player=%u authorityTick=%llu inputDelayTicks=3 readyGate=true"),
        NetworkSeat,
        static_cast<unsigned long long>(Simulation->CurrentTick()));
    SendScopedKeyframe();
    GetWorldTimerManager().SetTimer(
        NetworkKeyframeTimer,
        this,
        &AEchoesPlayerController::SendScopedUpdate,
        0.5f,
        true);
}

bool AEchoesPlayerController::ResumeNetworkMatch()
{
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    const echoes::sim::Simulation* Simulation =
        Bridge != nullptr ? Bridge->GetSimulation() : nullptr;
    if (!HasAuthority() || !bNetworkResumePending ||
        !bNetworkResumeMatchWasStarted || !bNetworkCompatibilityAccepted ||
        !bNetworkReady || Bridge == nullptr || Simulation == nullptr)
    {
        return false;
    }
    bNetworkMatchStarted = true;
    Bridge->SetNetworkHumanOpponent(true);
    Bridge->SetScenarioPaused(false);
    ClientReceiveNetworkLobbyState(
        true, NetworkSeat, Simulation->CurrentTick(), 3);
    ClientReceiveNetworkResumeState(
        true,
        LastAcceptedNetworkBatchId + 1,
        NetworkCommandContext.lastAcceptedSequence,
        Simulation->CurrentTick(),
        NetworkResumeDisconnectTick);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_NETWORK_MATCH_RESUMED] player=%u disconnectTick=%llu authorityTick=%llu lastAcceptedSequence=%llu nextBatch=%llu fullKeyframe=true aiControl=false"),
        NetworkSeat,
        static_cast<unsigned long long>(NetworkResumeDisconnectTick),
        static_cast<unsigned long long>(Simulation->CurrentTick()),
        static_cast<unsigned long long>(
            NetworkCommandContext.lastAcceptedSequence),
        static_cast<unsigned long long>(LastAcceptedNetworkBatchId + 1));
    SendScopedKeyframe();
    GetWorldTimerManager().SetTimer(
        NetworkKeyframeTimer,
        this,
        &AEchoesPlayerController::SendScopedUpdate,
        0.5f,
        true);
    return true;
}

void AEchoesPlayerController::ClientReceiveNetworkLobbyState_Implementation(
    bool bStarted,
    uint8 AssignedSeat,
    uint64 AuthorityTick,
    uint8 InputDelayTicks)
{
    if (AssignedSeat >= echoes::sim::kMaximumPlayers)
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NETWORK_LOBBY_REJECTED] reason=NET_VIEW_INVALID_PLAYER player=%u"),
            AssignedSeat);
        return;
    }
    NetworkSeat = AssignedSeat;
    bNetworkMatchStarted = bStarted;
    if (bStarted)
    {
        GetWorldTimerManager().ClearTimer(NetworkReadyTimer);
        if (UEchoesGameInstance* EchoesGameInstance =
                GetEchoesGameInstance())
        {
            EchoesGameInstance->MarkNetworkMatchStarted();
        }
    }
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_NETWORK_LOBBY_RESULT] compatible=%s started=%s seat=%u authorityTick=%llu inputDelayTicks=%u"),
        bNetworkCompatibilityAccepted ? TEXT("true") : TEXT("false"),
        bStarted ? TEXT("true") : TEXT("false"),
        NetworkSeat,
        static_cast<unsigned long long>(AuthorityTick),
        InputDelayTicks);
}

void AEchoesPlayerController::ClientReceiveNetworkResumeCredential_Implementation(
    const FString& Credential,
    float GraceSeconds)
{
    if (!IsBoundedClientResumeCredential(Credential) ||
        GraceSeconds <= 0.0f)
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NETWORK_RESUME_CREDENTIAL_REJECTED] reason=NET_RESUME_CREDENTIAL_INVALID"));
        return;
    }
    NetworkResumeCredential = Credential;
    if (UEchoesGameInstance* EchoesGameInstance = GetEchoesGameInstance())
    {
        EchoesGameInstance->StoreNetworkResumeCredential(
            Credential, GraceSeconds);
    }
#if UE_BUILD_DEVELOPMENT && WITH_DEV_AUTOMATION_TESTS
    if (bNetworkReconnectPhaseOneSmoke)
    {
        FString FileReason;
        if (DevelopmentResumeCredentialFilePath.IsEmpty() ||
            !StageDevelopmentResumeCredential(
                DevelopmentResumeCredentialFilePath,
                Credential,
                FileReason))
        {
            NetworkResumeCredential.Reset();
            if (UEchoesGameInstance* EchoesGameInstance =
                    GetEchoesGameInstance())
            {
                EchoesGameInstance->ClearReconnectContext(
                    TEXT("NET_RESUME_CREDENTIAL_STAGING_FAILED"));
            }
            DevelopmentResumeCredentialFilePath.Reset();
            UE_LOG(
                LogEchoes,
                Error,
                TEXT("[ECHOES_NETWORK_RECONNECT_CLIENT_FAILED] reason=%s credentialLogged=false credentialStaged=false"),
                FileReason.IsEmpty()
                    ? TEXT("NET_RESUME_CREDENTIAL_STAGING_FAILED")
                    : *FileReason);
            FPlatformMisc::RequestExit(false);
            return;
        }
        DevelopmentResumeCredentialFilePath.Reset();
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_NETWORK_RESUME_CREDENTIAL] issued=true credentialLogged=false credentialStaged=true storage=owner_only_one_use_file graceSeconds=%.0f exposure=developmentLoopbackSmokeOnly"),
            GraceSeconds);
    }
    else
#endif
    {
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_NETWORK_RESUME_CREDENTIAL] issued=true credentialLogged=false credentialStaged=false graceSeconds=%.0f exposure=memoryOnly"),
            GraceSeconds);
    }
}

void AEchoesPlayerController::ClientReceiveNetworkResumeState_Implementation(
    bool bResumed,
    uint64 RestoredNextBatchId,
    uint64 LastAcceptedSequence,
    uint64 AuthorityTick,
    uint64 DisconnectTick)
{
    if (!bResumed || RestoredNextBatchId == 0 ||
        AuthorityTick < DisconnectTick)
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NETWORK_RESUME_STATE_REJECTED] resumed=%s nextBatch=%llu authorityTick=%llu disconnectTick=%llu reason=NET_RESUME_STATE_INVALID"),
            bResumed ? TEXT("true") : TEXT("false"),
            static_cast<unsigned long long>(RestoredNextBatchId),
            static_cast<unsigned long long>(AuthorityTick),
            static_cast<unsigned long long>(DisconnectTick));
        if (bNetworkReconnectPhaseTwoSmoke)
        {
            FPlatformMisc::RequestExit(false);
        }
        return;
    }
    bNetworkResumeAccepted = true;
    NetworkResumeDisconnectTick = DisconnectTick;
    NextNetworkBatchId = RestoredNextBatchId;
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_NETWORK_RESUME_STATE] resumed=true seat=%u disconnectTick=%llu authorityTick=%llu lastAcceptedSequence=%llu nextBatch=%llu exactSequence=true exactBatch=true"),
        NetworkSeat,
        static_cast<unsigned long long>(DisconnectTick),
        static_cast<unsigned long long>(AuthorityTick),
        static_cast<unsigned long long>(LastAcceptedSequence),
        static_cast<unsigned long long>(RestoredNextBatchId));
}

bool AEchoesPlayerController::BuildNextScopedKeyframe(
    echoes::sim::net::ScopedViewKeyframe& OutKeyframe,
    FString& OutError)
{
    OutError.Reset();
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    const echoes::sim::Simulation* Simulation =
        Bridge != nullptr ? Bridge->GetSimulation() : nullptr;
    if (!HasAuthority() || !bNetworkMatchStarted || Simulation == nullptr)
    {
        OutError = TEXT("NET_AUTHORITY_NOT_READY");
        return false;
    }
    if (LastNetworkSnapshotId == std::numeric_limits<uint64>::max())
    {
        OutError = TEXT("NET_SNAPSHOT_ID_EXHAUSTED");
        return false;
    }
    const std::optional<echoes::sim::PlayerView> View =
        Simulation->CreatePlayerView(NetworkSeat);
    echoes::sim::net::ScopedViewKeyframe Keyframe{};
    std::string KeyframeError;
    if (!View.has_value() ||
        !echoes::sim::net::BuildScopedViewKeyframe(
            *View,
            LastNetworkSnapshotId + 1,
            NetworkCommandContext.lastAcceptedSequence,
            Keyframe,
            &KeyframeError))
    {
        OutError = KeyframeError.empty()
            ? TEXT("NET_PLAYER_VIEW_UNAVAILABLE")
            : FString(UTF8_TO_TCHAR(KeyframeError.c_str()));
        return false;
    }
    LastNetworkSnapshotId = Keyframe.snapshotId;
    OutKeyframe = std::move(Keyframe);
    return true;
}

void AEchoesPlayerController::SendScopedKeyframe()
{
    if (PendingNetworkSnapshotDigests.Num() >= 8)
    {
        GetWorldTimerManager().ClearTimer(NetworkKeyframeTimer);
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NETWORK_STATE_STALLED] player=%u pendingSnapshots=%d reason=NET_SNAPSHOT_ACK_WINDOW_EXHAUSTED"),
            NetworkSeat,
            PendingNetworkSnapshotDigests.Num());
        ClientReturnToMainMenuWithTextReason(
            FText::FromString(TEXT("NET_SNAPSHOT_ACK_WINDOW_EXHAUSTED")));
        return;
    }
    echoes::sim::net::ScopedViewKeyframe Keyframe{};
    FString KeyframeError;
    if (!BuildNextScopedKeyframe(Keyframe, KeyframeError))
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NETWORK_KEYFRAME_FAILED] player=%u reason=%s"),
            NetworkSeat,
            *KeyframeError);
        return;
    }
    const std::vector<std::uint8_t> Encoded =
        echoes::sim::net::EncodeScopedViewKeyframe(Keyframe);
    if (Encoded.empty())
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NETWORK_KEYFRAME_FAILED] player=%u reason=NET_KEYFRAME_ENCODING_FAILED"),
            NetworkSeat);
        return;
    }
    PendingNetworkSnapshotDigests.Add(
        Keyframe.snapshotId, Keyframe.scopedDigest);
    LastSentNetworkKeyframe = Keyframe;
    ClientReceiveScopedKeyframe(echoes::network::ToByteArray(Encoded));
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_NETWORK_KEYFRAME_SENT] player=%u snapshot=%llu previous=%llu tick=%llu bytes=%d entities=%d tiles=%d digest=%llu hiddenAuthorityExcluded=true"),
        NetworkSeat,
        static_cast<unsigned long long>(Keyframe.snapshotId),
        static_cast<unsigned long long>(Keyframe.snapshotId - 1),
        static_cast<unsigned long long>(Keyframe.simulationTick),
        static_cast<int32>(Encoded.size()),
        static_cast<int32>(Keyframe.entities.size()),
        static_cast<int32>(Keyframe.tiles.size()),
        static_cast<unsigned long long>(Keyframe.scopedDigest));
}

void AEchoesPlayerController::SendScopedUpdate()
{
    if (!LastSentNetworkKeyframe.has_value())
    {
        SendScopedKeyframe();
        return;
    }
    if (PendingNetworkSnapshotDigests.Num() >= 8)
    {
        SendScopedKeyframe();
        return;
    }
    echoes::sim::net::ScopedViewKeyframe Current{};
    FString KeyframeError;
    if (!BuildNextScopedKeyframe(Current, KeyframeError))
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NETWORK_DELTA_FAILED] player=%u reason=%s"),
            NetworkSeat,
            *KeyframeError);
        return;
    }
    echoes::sim::net::ScopedViewDelta Delta{};
    std::string DeltaError;
    const bool bDeltaBuilt = echoes::sim::net::BuildScopedViewDelta(
        *LastSentNetworkKeyframe, Current, Delta, &DeltaError);
    const std::vector<std::uint8_t> DeltaBytes =
        bDeltaBuilt
            ? echoes::sim::net::EncodeScopedViewDelta(Delta)
            : std::vector<std::uint8_t>{};
    const std::vector<std::uint8_t> KeyframeBytes =
        echoes::sim::net::EncodeScopedViewKeyframe(Current);
    if (KeyframeBytes.empty())
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NETWORK_DELTA_FAILED] player=%u reason=NET_KEYFRAME_ENCODING_FAILED"),
            NetworkSeat);
        return;
    }
    PendingNetworkSnapshotDigests.Add(
        Current.snapshotId, Current.scopedDigest);
    LastSentNetworkKeyframe = Current;
    const UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    const echoes::sim::Simulation* Simulation =
        Bridge != nullptr ? Bridge->GetSimulation() : nullptr;
    const bool bFinalState =
        Simulation != nullptr &&
        Simulation->Outcome() != echoes::sim::MatchOutcome::Ongoing;
    if (bFinalState || DeltaBytes.empty() ||
        DeltaBytes.size() >= KeyframeBytes.size())
    {
        ClientReceiveScopedKeyframe(
            echoes::network::ToByteArray(KeyframeBytes));
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_NETWORK_KEYFRAME_SENT] player=%u snapshot=%llu previous=%llu tick=%llu bytes=%d entities=%d tiles=%d digest=%llu fallback=%s hiddenAuthorityExcluded=true"),
            NetworkSeat,
            static_cast<unsigned long long>(Current.snapshotId),
            static_cast<unsigned long long>(Current.snapshotId - 1),
            static_cast<unsigned long long>(Current.simulationTick),
            static_cast<int32>(KeyframeBytes.size()),
            static_cast<int32>(Current.entities.size()),
            static_cast<int32>(Current.tiles.size()),
            static_cast<unsigned long long>(Current.scopedDigest),
            bFinalState ? TEXT("finalReliableKeyframe")
            : DeltaError.empty() ? TEXT("deltaNotSmaller")
                               : UTF8_TO_TCHAR(DeltaError.c_str()));
        PublishNetworkMatchResultIfFinished(Current);
        return;
    }
    ClientReceiveScopedDelta(echoes::network::ToByteArray(DeltaBytes));
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_NETWORK_DELTA_SENT] player=%u snapshot=%llu base=%llu tick=%llu bytes=%d fullBytes=%d tileChanges=%d upserts=%d removals=%d digest=%llu hiddenAuthorityExcluded=true"),
        NetworkSeat,
        static_cast<unsigned long long>(Delta.snapshotId),
        static_cast<unsigned long long>(Delta.baseSnapshotId),
        static_cast<unsigned long long>(Delta.simulationTick),
        static_cast<int32>(DeltaBytes.size()),
        static_cast<int32>(KeyframeBytes.size()),
        static_cast<int32>(Delta.tileChanges.size()),
        static_cast<int32>(Delta.entityUpserts.size()),
        static_cast<int32>(Delta.removedEntityIds.size()),
        static_cast<unsigned long long>(Delta.scopedDigest));
    PublishNetworkMatchResultIfFinished(Current);
}

void AEchoesPlayerController::PublishNetworkMatchResultIfFinished(
    const echoes::sim::net::ScopedViewKeyframe& FinalView)
{
    const UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    const echoes::sim::Simulation* Simulation =
        Bridge != nullptr ? Bridge->GetSimulation() : nullptr;
    const echoes::sim::MatchOutcome Outcome =
        Simulation != nullptr
            ? Simulation->Outcome()
            : echoes::sim::MatchOutcome::Ongoing;
    if (!HasAuthority() || bNetworkMatchResultSent || Simulation == nullptr ||
        Outcome == echoes::sim::MatchOutcome::Ongoing ||
        FinalView.simulationTick != Simulation->CurrentTick())
    {
        return;
    }
    bNetworkMatchResultSent = true;
    NetworkSentResultOutcome = static_cast<uint8>(Outcome);
    NetworkSentResultTick = FinalView.simulationTick;
    NetworkSentResultSnapshotId = FinalView.snapshotId;
    NetworkSentResultScopedDigest = FinalView.scopedDigest;
    GetWorldTimerManager().ClearTimer(NetworkKeyframeTimer);
    ClientReceiveNetworkMatchResult(
        static_cast<uint8>(Outcome),
        FinalView.simulationTick,
        FinalView.snapshotId,
        FinalView.scopedDigest);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_NETWORK_MATCH_RESULT_SENT] player=%u outcome=%u finalTick=%llu finalSnapshot=%llu finalDigest=%llu reliableFinalKeyframe=true"),
        NetworkSeat,
        static_cast<uint8>(Outcome),
        static_cast<unsigned long long>(FinalView.simulationTick),
        static_cast<unsigned long long>(FinalView.snapshotId),
        static_cast<unsigned long long>(FinalView.scopedDigest));
    if (AEchoesGameMode* GameMode =
            GetWorld() != nullptr
                ? GetWorld()->GetAuthGameMode<AEchoesGameMode>()
                : nullptr)
    {
        GameMode->NotifyNetworkMatchFinished();
    }
    BeginHostNetworkResultDeliveryWait();
}

void AEchoesPlayerController::ClientReceiveNetworkMatchResult_Implementation(
    uint8 OutcomeValue,
    uint64 FinalTick,
    uint64 FinalSnapshotId,
    uint64 FinalScopedDigest)
{
    const echoes::sim::MatchOutcome Outcome =
        static_cast<echoes::sim::MatchOutcome>(OutcomeValue);
    const echoes::sim::net::ScopedViewKeyframe* View = GetNetworkScopedView();
    if (bNetworkMatchResultReceived ||
        Outcome == echoes::sim::MatchOutcome::Ongoing ||
        OutcomeValue >
            static_cast<uint8>(echoes::sim::MatchOutcome::Player3Victory) ||
        View == nullptr || View->snapshotId != FinalSnapshotId ||
        View->simulationTick != FinalTick ||
        View->scopedDigest != FinalScopedDigest)
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NETWORK_MATCH_RESULT_REJECTED] outcome=%u finalTick=%llu finalSnapshot=%llu finalDigest=%llu acceptedSnapshot=%llu acceptedTick=%llu acceptedDigest=%llu reason=NET_RESULT_STATE_MISMATCH"),
            OutcomeValue,
            static_cast<unsigned long long>(FinalTick),
            static_cast<unsigned long long>(FinalSnapshotId),
            static_cast<unsigned long long>(FinalScopedDigest),
            static_cast<unsigned long long>(View != nullptr ? View->snapshotId : 0),
            static_cast<unsigned long long>(View != nullptr ? View->simulationTick : 0),
            static_cast<unsigned long long>(View != nullptr ? View->scopedDigest : 0));
        return;
    }
    bNetworkMatchResultReceived = true;
    PresentedFinalTick = FinalTick;
    bNetworkResultExitEnabled = true;
    if (UEchoesGameInstance* EchoesGameInstance = GetEchoesGameInstance())
    {
        EchoesGameInstance->MarkNetworkMatchResultReceived();
    }
    NotifyMatchFinished(Outcome);
    ServerAcknowledgeNetworkMatchResult(
        OutcomeValue,
        FinalTick,
        FinalSnapshotId,
        FinalScopedDigest);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_NETWORK_MATCH_RESULT_RECEIVED] seat=%u outcome=%u finalTick=%llu finalSnapshot=%llu finalDigest=%llu stateMatched=true"),
        NetworkSeat,
        OutcomeValue,
        static_cast<unsigned long long>(FinalTick),
        static_cast<unsigned long long>(FinalSnapshotId),
        static_cast<unsigned long long>(FinalScopedDigest));
    if (bNetworkMatchSmoke)
    {
        const bool bPresentedVictory = DidPresentedLocalPlayerWin();
        if (!bNetworkMatchCommandSubmitted || !bNetworkMatchBatchAdmitted ||
            !bPresentedVictory || Outcome != echoes::sim::MatchOutcome::Player1Victory)
        {
            UE_LOG(
                LogEchoes,
                Error,
                TEXT("[ECHOES_NETWORK_MATCH_CLIENT_SMOKE_FAILED] submitted=%s batchAdmitted=%s seat=%u outcome=%u presentedVictory=%s"),
                bNetworkMatchCommandSubmitted ? TEXT("true") : TEXT("false"),
                bNetworkMatchBatchAdmitted ? TEXT("true") : TEXT("false"),
                NetworkSeat,
                OutcomeValue,
                bPresentedVictory ? TEXT("true") : TEXT("false"));
            FPlatformMisc::RequestExit(false);
            return;
        }
        bNetworkMatchSmokeCompletionSent = true;
        ServerConfirmNetworkMatchSmokeComplete(
            OutcomeValue,
            FinalTick,
            FinalSnapshotId,
            FinalScopedDigest);
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_NETWORK_MATCH_CLIENT_SMOKE_PASSED] seat=%u outcome=%u finalTick=%llu finalSnapshot=%llu finalDigest=%llu batchAdmitted=true ordinaryCombatResolution=true presentedVictory=true finalStateMatched=true separateProcess=true"),
            NetworkSeat,
            OutcomeValue,
            static_cast<unsigned long long>(FinalTick),
            static_cast<unsigned long long>(FinalSnapshotId),
            static_cast<unsigned long long>(FinalScopedDigest));
        GetWorldTimerManager().SetTimer(
            NetworkClientExitTimer,
            this,
            &AEchoesPlayerController::FinishNetworkClientSmoke,
            0.5f,
            false);
    }
}

void AEchoesPlayerController::ServerAcknowledgeNetworkMatchResult_Implementation(
    uint8 OutcomeValue,
    uint64 FinalTick,
    uint64 FinalSnapshotId,
    uint64 FinalScopedDigest)
{
    const bool bExact = HasAuthority() && bNetworkMatchResultSent &&
        OutcomeValue == NetworkSentResultOutcome &&
        FinalTick == NetworkSentResultTick &&
        FinalSnapshotId == NetworkSentResultSnapshotId &&
        FinalScopedDigest == NetworkSentResultScopedDigest;
    if (!bExact)
    {
        UE_LOG(
            LogEchoes,
            Warning,
            TEXT("[ECHOES_NETWORK_RESULT_ACK_REJECTED] outcome=%u finalTick=%llu finalSnapshot=%llu finalDigest=%llu reason=NET_RESULT_ACK_MISMATCH"),
            OutcomeValue,
            static_cast<unsigned long long>(FinalTick),
            static_cast<unsigned long long>(FinalSnapshotId),
            static_cast<unsigned long long>(FinalScopedDigest));
        return;
    }
    bNetworkMatchResultAcknowledged = true;
    EnableHostNetworkResultExit(true, TEXT("NET_RESULT_ACKNOWLEDGED"));
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_NETWORK_RESULT_ACKNOWLEDGED] outcome=%u finalTick=%llu finalSnapshot=%llu finalDigest=%llu exact=true"),
        OutcomeValue,
        static_cast<unsigned long long>(FinalTick),
        static_cast<unsigned long long>(FinalSnapshotId),
        static_cast<unsigned long long>(FinalScopedDigest));
}

void AEchoesPlayerController::BeginHostNetworkResultDeliveryWait()
{
    if (!HasAuthority() || !bNetworkMatchResultSent)
    {
        return;
    }
    for (FConstPlayerControllerIterator It =
             GetWorld()->GetPlayerControllerIterator();
         It;
         ++It)
    {
        AEchoesPlayerController* Host =
            Cast<AEchoesPlayerController>(It->Get());
        if (Host != nullptr && Host->IsLocalController())
        {
            Host->PresentedFinalTick = NetworkSentResultTick;
            Host->bNetworkResultExitEnabled = false;
            Host->bNetworkMatchResultAcknowledged = false;
            Host->NetworkSentResultOutcome = NetworkSentResultOutcome;
            Host->NetworkSentResultTick = NetworkSentResultTick;
            Host->NetworkSentResultSnapshotId = NetworkSentResultSnapshotId;
            Host->NetworkSentResultScopedDigest =
                NetworkSentResultScopedDigest;
            Host->GetWorldTimerManager().SetTimer(
                Host->NetworkResultAcknowledgementTimer,
                Host,
                &AEchoesPlayerController::AllowHostNetworkResultExitAfterTimeout,
                10.0f,
                false);
            break;
        }
    }
}

void AEchoesPlayerController::AllowHostNetworkResultExitAfterTimeout()
{
    if (bNetworkMatchResultAcknowledged)
    {
        return;
    }
    UE_LOG(
        LogEchoes,
        Warning,
        TEXT("[ECHOES_NETWORK_RESULT_ACK_TIMEOUT] finalTick=%llu finalSnapshot=%llu finalResultRetained=true hostLeaveEnabled=true"),
        static_cast<unsigned long long>(NetworkSentResultTick),
        static_cast<unsigned long long>(NetworkSentResultSnapshotId));
    EnableHostNetworkResultExit(false, TEXT("NET_RESULT_ACK_TIMEOUT"));
}

void AEchoesPlayerController::EnableHostNetworkResultExit(
    bool bAcknowledged,
    const FString& StableReason)
{
    if (!HasAuthority() || GetWorld() == nullptr)
    {
        return;
    }
    for (FConstPlayerControllerIterator It =
             GetWorld()->GetPlayerControllerIterator();
         It;
         ++It)
    {
        AEchoesPlayerController* Host =
            Cast<AEchoesPlayerController>(It->Get());
        if (Host == nullptr || !Host->IsLocalController())
        {
            continue;
        }
        Host->bNetworkResultExitEnabled = true;
        Host->bNetworkMatchResultAcknowledged = bAcknowledged;
        Host->GetWorldTimerManager().ClearTimer(
            Host->NetworkResultAcknowledgementTimer);
        Host->SetStatusMessage(
            bAcknowledged
                ? TEXT("ONLINE RESULT DELIVERED — Enter leaves for the Online menu.")
                : TEXT("ONLINE RESULT RETAINED — the opponent did not confirm receipt; Enter leaves for the Online menu."),
            3600.0f);
        if (Host->bReturnHostToOnlineAfterResultDelivery)
        {
            Host->bReturnHostToOnlineAfterResultDelivery = false;
            if (UEchoesGameInstance* EchoesGameInstance =
                    Host->GetEchoesGameInstance())
            {
                EchoesGameInstance->ReturnToOnlineFrontDoor(Host);
            }
        }
        break;
    }
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_NETWORK_HOST_RESULT_EXIT] enabled=true acknowledged=%s reason=%s"),
        bAcknowledged ? TEXT("true") : TEXT("false"),
        *StableReason.Left(96));
}

void AEchoesPlayerController::ClientReceiveScopedKeyframe_Implementation(
    const TArray<uint8>& Packet)
{
    echoes::sim::net::ScopedViewKeyframe Keyframe{};
    const echoes::sim::net::DecodeStatus Decode =
        echoes::sim::net::DecodeScopedViewKeyframe(
            echoes::network::AsByteSpan(Packet), Keyframe);
    if (Decode != echoes::sim::net::DecodeStatus::Ok)
    {
        const FString Reason(
            UTF8_TO_TCHAR(echoes::sim::net::StableId(Decode).data()));
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NETWORK_KEYFRAME_REJECTED] reason=%s"),
            *Reason);
        RequestScopedKeyframeRecovery(Reason);
        return;
    }
    if (Keyframe.player >= echoes::sim::kMaximumPlayers)
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NETWORK_KEYFRAME_REJECTED] reason=NET_VIEW_INVALID_PLAYER player=%u"),
            Keyframe.player);
        if (bNetworkClientSmoke)
        {
            FPlatformMisc::RequestExit(false);
        }
        return;
    }
    const echoes::network::ScopedViewAcceptance Acceptance =
        NetworkViewState.Accept(Keyframe);
    if (Acceptance ==
            echoes::network::ScopedViewAcceptance::InvalidSnapshot ||
        Acceptance == echoes::network::ScopedViewAcceptance::PlayerChanged)
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NETWORK_KEYFRAME_REJECTED] reason=%s snapshot=%llu"),
            UTF8_TO_TCHAR(echoes::network::StableId(Acceptance)),
            static_cast<unsigned long long>(Keyframe.snapshotId));
        if (bNetworkClientSmoke)
        {
            FPlatformMisc::RequestExit(false);
        }
        return;
    }
    if (Acceptance ==
        echoes::network::ScopedViewAcceptance::StaleOrDuplicate)
    {
        UE_LOG(
            LogEchoes,
            Warning,
            TEXT("[ECHOES_NETWORK_KEYFRAME_IGNORED] reason=%s snapshot=%llu acceptedSnapshot=%llu"),
            UTF8_TO_TCHAR(echoes::network::StableId(Acceptance)),
            static_cast<unsigned long long>(Keyframe.snapshotId),
            static_cast<unsigned long long>(LastNetworkSnapshotId));
        return;
    }
    const uint64 PreviousSnapshotId = LastNetworkSnapshotId;
    LastNetworkSnapshotId = Keyframe.snapshotId;
    if (Acceptance ==
        echoes::network::ScopedViewAcceptance::AcceptedRecovery)
    {
        bNetworkFaultRecoveryObserved = true;
        UE_LOG(
            LogEchoes,
            Warning,
            TEXT("[ECHOES_NETWORK_KEYFRAME_RECOVERY] previous=%llu recovered=%llu fullKeyframe=true"),
            static_cast<unsigned long long>(PreviousSnapshotId),
            static_cast<unsigned long long>(Keyframe.snapshotId));
    }
    ServerAcknowledgeScopedKeyframe(
        Keyframe.snapshotId, Keyframe.scopedDigest);
    if (!SyncNetworkPresentation(Keyframe))
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NETWORK_PRESENTATION_FAILED] snapshot=%llu source=keyframe"),
            static_cast<unsigned long long>(Keyframe.snapshotId));
    }
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_NETWORK_KEYFRAME_RECEIVED] player=%u snapshot=%llu previous=%llu tick=%llu bytes=%d entities=%d tiles=%d digest=%llu lineage=%s hiddenAuthorityExcluded=true"),
        Keyframe.player,
        static_cast<unsigned long long>(Keyframe.snapshotId),
        static_cast<unsigned long long>(PreviousSnapshotId),
        static_cast<unsigned long long>(Keyframe.simulationTick),
        Packet.Num(),
        static_cast<int32>(Keyframe.entities.size()),
        static_cast<int32>(Keyframe.tiles.size()),
        static_cast<unsigned long long>(Keyframe.scopedDigest),
        UTF8_TO_TCHAR(echoes::network::StableId(Acceptance)));
    TrySubmitNetworkMatchSmoke(Keyframe);
    TryAdvanceNetworkReconnectSmoke(Keyframe);
    TryFinishNetworkClientSmoke();
    if (!bNetworkClientSmoke || bNetworkCommandSubmitted)
    {
        return;
    }

    const auto OwnedWorker = std::find_if(
        Keyframe.entities.begin(),
        Keyframe.entities.end(),
        [&](const echoes::sim::net::ScopedEntityState& Entity)
        {
            return Entity.owner == Keyframe.player &&
                   Entity.type == echoes::sim::EntityType::Worker;
        });
    if (OwnedWorker == Keyframe.entities.end())
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NETWORK_CLIENT_FAILED] reason=NET_OWNED_WORKER_UNAVAILABLE"));
        FPlatformMisc::RequestExit(false);
        return;
    }
    echoes::sim::net::CommandRequest Request{};
    Request.sequence = Keyframe.lastAcceptedSequence + 1;
    Request.executeTick = 0;
    Request.type = echoes::sim::CommandType::Move;
    Request.actor = OwnedWorker->id;
    const int32 MaximumXRaw =
        Keyframe.mapWidthTiles * echoes::sim::kFixedScale - 1;
    Request.position = echoes::sim::Vec2::FromRaw(
        FMath::Min(
            OwnedWorker->position.x.Raw() + echoes::sim::kFixedScale,
            MaximumXRaw),
        OwnedWorker->position.y.Raw());
    const std::vector<std::uint8_t> Encoded =
        echoes::sim::net::EncodeCommandRequest(Request);
    if (Encoded.empty())
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NETWORK_CLIENT_FAILED] reason=NET_COMMAND_ENCODING_FAILED"));
        FPlatformMisc::RequestExit(false);
        return;
    }
    ServerSubmitNetworkCommand(echoes::network::ToByteArray(Encoded));
    bNetworkCommandSubmitted = true;
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_NETWORK_COMMAND_SENT] sequence=%llu requestedExecuteTick=%llu actor=%u targetRaw=(%d,%d) bytes=%d authorityAssignsTick=true"),
        static_cast<unsigned long long>(Request.sequence),
        static_cast<unsigned long long>(Request.executeTick),
        Request.actor,
        Request.position.x.Raw(),
        Request.position.y.Raw(),
        static_cast<int32>(Encoded.size()));
}

void AEchoesPlayerController::ClientReceiveScopedDelta_Implementation(
    const TArray<uint8>& Packet)
{
    if (FParse::Param(
            FCommandLine::Get(), TEXT("EchoesNetworkDropFirstDelta")) &&
        !bNetworkDroppedFirstDeltaForSmoke)
    {
        bNetworkDroppedFirstDeltaForSmoke = true;
        UE_LOG(
            LogEchoes,
            Warning,
            TEXT("[ECHOES_NETWORK_DELTA_DROPPED] injected=true bytes=%d acceptedSnapshot=%llu"),
            Packet.Num(),
            static_cast<unsigned long long>(LastNetworkSnapshotId));
        return;
    }

    if (bNetworkDropDeltaBurstForSmoke && NetworkDroppedDeltaCount < 3)
    {
        ++NetworkDroppedDeltaCount;
        bNetworkFaultInjectionPerformed = true;
        UE_LOG(
            LogEchoes,
            Warning,
            TEXT("[ECHOES_NETWORK_DELTA_BURST_DROPPED] injected=true ordinal=%u burstSize=3 bytes=%d acceptedSnapshot=%llu"),
            NetworkDroppedDeltaCount,
            Packet.Num(),
            static_cast<unsigned long long>(LastNetworkSnapshotId));
        return;
    }

    if (bNetworkDelayFirstDeltaForSmoke &&
        !bNetworkFaultInjectionPerformed)
    {
        bNetworkFaultInjectionPerformed = true;
        PendingNetworkFaultDelta = Packet;
        GetWorldTimerManager().SetTimer(
            NetworkFaultDeliveryTimer,
            this,
            &AEchoesPlayerController::DeliverDelayedNetworkDelta,
            0.25f,
            false);
        UE_LOG(
            LogEchoes,
            Warning,
            TEXT("[ECHOES_NETWORK_DELTA_DELAYED] injected=true delayMilliseconds=250 bytes=%d acceptedSnapshot=%llu"),
            Packet.Num(),
            static_cast<unsigned long long>(LastNetworkSnapshotId));
        return;
    }

    if (bNetworkReorderFirstTwoDeltasForSmoke &&
        !bNetworkFaultInjectionPerformed)
    {
        if (PendingNetworkFaultDelta.IsEmpty())
        {
            PendingNetworkFaultDelta = Packet;
            UE_LOG(
                LogEchoes,
                Warning,
                TEXT("[ECHOES_NETWORK_DELTA_REORDER_HELD] injected=true bytes=%d acceptedSnapshot=%llu"),
                Packet.Num(),
                static_cast<unsigned long long>(LastNetworkSnapshotId));
            return;
        }
        bNetworkFaultInjectionPerformed = true;
        TArray<uint8> HeldPacket = MoveTemp(PendingNetworkFaultDelta);
        PendingNetworkFaultDelta.Reset();
        UE_LOG(
            LogEchoes,
            Warning,
            TEXT("[ECHOES_NETWORK_DELTA_REORDERED] injected=true newerBytes=%d heldBytes=%d deliveryOrder=newerThenOlder acceptedSnapshot=%llu"),
            Packet.Num(),
            HeldPacket.Num(),
            static_cast<unsigned long long>(LastNetworkSnapshotId));
        ProcessScopedDeltaPacket(Packet);
        ProcessScopedDeltaPacket(HeldPacket);
        return;
    }

    if (bNetworkDuplicateFirstDeltaForSmoke &&
        !bNetworkFaultInjectionPerformed)
    {
        bNetworkFaultInjectionPerformed = true;
        UE_LOG(
            LogEchoes,
            Warning,
            TEXT("[ECHOES_NETWORK_DELTA_DUPLICATED] injected=true bytes=%d acceptedSnapshot=%llu"),
            Packet.Num(),
            static_cast<unsigned long long>(LastNetworkSnapshotId));
        ProcessScopedDeltaPacket(Packet);
        ProcessScopedDeltaPacket(Packet);
        return;
    }

    ProcessScopedDeltaPacket(Packet);
}

void AEchoesPlayerController::DeliverDelayedNetworkDelta()
{
    if (PendingNetworkFaultDelta.IsEmpty())
    {
        return;
    }
    TArray<uint8> Packet = MoveTemp(PendingNetworkFaultDelta);
    PendingNetworkFaultDelta.Reset();
    bNetworkDelayedDeltaDelivered = true;
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_NETWORK_DELTA_DELAY_COMPLETE] injected=true bytes=%d acceptedSnapshot=%llu"),
        Packet.Num(),
        static_cast<unsigned long long>(LastNetworkSnapshotId));
    ProcessScopedDeltaPacket(Packet);
}

void AEchoesPlayerController::ProcessScopedDeltaPacket(
    const TArray<uint8>& Packet)
{
    echoes::sim::net::ScopedViewDelta Delta{};
    const echoes::sim::net::DecodeStatus Decode =
        echoes::sim::net::DecodeScopedViewDelta(
            echoes::network::AsByteSpan(Packet), Delta);
    if (Decode != echoes::sim::net::DecodeStatus::Ok)
    {
        RequestScopedKeyframeRecovery(
            FString(UTF8_TO_TCHAR(
                echoes::sim::net::StableId(Decode).data())));
        return;
    }
    std::string ApplyError;
    const echoes::network::ScopedViewAcceptance Acceptance =
        NetworkViewState.AcceptDelta(Delta, &ApplyError);
    if (Acceptance ==
        echoes::network::ScopedViewAcceptance::StaleOrDuplicate)
    {
        bNetworkDuplicateDeltaIgnored = true;
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_NETWORK_DELTA_IGNORED] snapshot=%llu base=%llu acceptedSnapshot=%llu reason=NET_VIEW_STALE_OR_DUPLICATE recoveryRequested=false"),
            static_cast<unsigned long long>(Delta.snapshotId),
            static_cast<unsigned long long>(Delta.baseSnapshotId),
            static_cast<unsigned long long>(LastNetworkSnapshotId));
        TryFinishNetworkClientSmoke();
        return;
    }
    if (Acceptance != echoes::network::ScopedViewAcceptance::AcceptedDelta)
    {
        const FString Reason = ApplyError.empty()
            ? FString(UTF8_TO_TCHAR(
                  echoes::network::StableId(Acceptance)))
            : FString(UTF8_TO_TCHAR(ApplyError.c_str()));
        UE_LOG(
            LogEchoes,
            Warning,
            TEXT("[ECHOES_NETWORK_DELTA_REJECTED] snapshot=%llu base=%llu acceptedSnapshot=%llu reason=%s"),
            static_cast<unsigned long long>(Delta.snapshotId),
            static_cast<unsigned long long>(Delta.baseSnapshotId),
            static_cast<unsigned long long>(LastNetworkSnapshotId),
            *Reason);
        RequestScopedKeyframeRecovery(Reason);
        return;
    }
    const echoes::sim::net::ScopedViewKeyframe* Current =
        GetNetworkScopedView();
    if (Current == nullptr)
    {
        RequestScopedKeyframeRecovery(TEXT("NET_VIEW_UNAVAILABLE"));
        return;
    }
    const uint64 PreviousSnapshotId = LastNetworkSnapshotId;
    LastNetworkSnapshotId = Current->snapshotId;
    ServerAcknowledgeScopedKeyframe(
        Current->snapshotId, Current->scopedDigest);
    if (!SyncNetworkPresentation(*Current))
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NETWORK_PRESENTATION_FAILED] snapshot=%llu source=delta"),
            static_cast<unsigned long long>(Current->snapshotId));
    }
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_NETWORK_DELTA_RECEIVED] player=%u snapshot=%llu base=%llu previous=%llu tick=%llu bytes=%d tileChanges=%d upserts=%d removals=%d digest=%llu lineage=NET_VIEW_ACCEPTED_DELTA hiddenAuthorityExcluded=true"),
        Current->player,
        static_cast<unsigned long long>(Current->snapshotId),
        static_cast<unsigned long long>(Delta.baseSnapshotId),
        static_cast<unsigned long long>(PreviousSnapshotId),
        static_cast<unsigned long long>(Current->simulationTick),
        Packet.Num(),
        static_cast<int32>(Delta.tileChanges.size()),
        static_cast<int32>(Delta.entityUpserts.size()),
        static_cast<int32>(Delta.removedEntityIds.size()),
        static_cast<unsigned long long>(Current->scopedDigest));
    TryAdvanceNetworkReconnectSmoke(*Current);
    TryFinishNetworkClientSmoke();
}

void AEchoesPlayerController::RequestScopedKeyframeRecovery(
    const FString& Reason)
{
    const double Now = FPlatformTime::Seconds();
    if (Now - LastScopedRecoveryRequestClientSeconds < 1.0)
    {
        UE_LOG(
            LogEchoes,
            Warning,
            TEXT("[ECHOES_NETWORK_KEYFRAME_REQUEST_THROTTLED] acceptedSnapshot=%llu reason=%s"),
            static_cast<unsigned long long>(LastNetworkSnapshotId),
            *Reason);
        return;
    }
    LastScopedRecoveryRequestClientSeconds = Now;
    ServerRequestScopedKeyframe(LastNetworkSnapshotId);
    UE_LOG(
        LogEchoes,
        Warning,
        TEXT("[ECHOES_NETWORK_KEYFRAME_RECOVERY_REQUESTED] acceptedSnapshot=%llu reason=%s rateLimited=true"),
        static_cast<unsigned long long>(LastNetworkSnapshotId),
        *Reason);
}

echoes::sim::Entity AEchoesPlayerController::BuildNetworkPresentationEntity(
    const echoes::sim::net::ScopedEntityState& Scoped)
{
    echoes::sim::Entity State{};
    State.id = Scoped.id;
    State.owner = Scoped.owner;
    State.faction = Scoped.faction;
    State.type = Scoped.type;
    State.position = Scoped.position;
    State.hitPoints = Scoped.hitPoints;
    State.maxHitPoints = Scoped.maxHitPoints;
    State.completed = Scoped.completed;
    State.wellChoice = Scoped.wellChoice;
    State.deployed = Scoped.deployed;
    State.waystoneMode = Scoped.waystoneMode;
    State.warformAdaptation = Scoped.warformAdaptation;
    State.aegisPowered = Scoped.aegisPowered;
    State.choirIdentityState = Scoped.choirIdentityState;
    State.choirIdentityResolveAtTick = Scoped.choirIdentityResolveAtTick;
    State.choirIdentityNextAvailableTick =
        Scoped.choirIdentityNextAvailableTick;
    State.choirCoherenceNextChargeTick =
        Scoped.choirCoherenceNextChargeTick;
    return State;
}

bool AEchoesPlayerController::SyncNetworkPresentation(
    const echoes::sim::net::ScopedViewKeyframe& Keyframe)
{
    UWorld* World = GetWorld();
    if (GetNetMode() != NM_Client || World == nullptr ||
        Keyframe.mapWidthTiles <= 0 || Keyframe.mapHeightTiles <= 0)
    {
        return GetNetMode() != NM_Client;
    }
    FActorSpawnParameters SpawnParameters;
    SpawnParameters.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    if (!NetworkDirectionalLight.IsValid() || !NetworkSkyLight.IsValid())
    {
        ADirectionalLight* Sun = World->SpawnActor<ADirectionalLight>(
            FVector(0.0f, 0.0f, 1800.0f),
            FRotator(-55.0f, -35.0f, 0.0f),
            SpawnParameters);
        ASkyLight* Sky = World->SpawnActor<ASkyLight>(
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            SpawnParameters);
        UDirectionalLightComponent* SunComponent =
            Sun != nullptr
                ? Cast<UDirectionalLightComponent>(Sun->GetLightComponent())
                : nullptr;
        if (Sun == nullptr || Sky == nullptr || SunComponent == nullptr)
        {
            if (Sun != nullptr)
            {
                Sun->Destroy();
            }
            if (Sky != nullptr)
            {
                Sky->Destroy();
            }
            return false;
        }
        SunComponent->SetIntensity(12.0f);
        SunComponent->SetLightColor(FLinearColor(1.0f, 0.86f, 0.72f));
        Sky->GetLightComponent()->SetIntensity(1.1f);
        Sun->Tags.Add(TEXT("EchoesNetworkPresentationLight"));
        Sky->Tags.Add(TEXT("EchoesNetworkPresentationLight"));
        NetworkDirectionalLight = Sun;
        NetworkSkyLight = Sky;
    }

    if (!NetworkGroundView.IsValid())
    {
        AStaticMeshActor* Ground = World->SpawnActor<AStaticMeshActor>(
            AStaticMeshActor::StaticClass(),
            FVector(0.0f, 0.0f, -18.0f),
            FRotator::ZeroRotator,
            SpawnParameters);
        UStaticMesh* Cube = LoadObject<UStaticMesh>(
            nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
        UMaterialInterface* Surface = LoadObject<UMaterialInterface>(
            nullptr,
            TEXT("/Game/Art/Generated/Materials/M_EchoesWorldSurface.M_EchoesWorldSurface"));
        if (Ground == nullptr || Cube == nullptr || Surface == nullptr)
        {
            if (Ground != nullptr)
            {
                Ground->Destroy();
            }
            return false;
        }
        UStaticMeshComponent* Mesh = Ground->GetStaticMeshComponent();
        Mesh->SetMobility(EComponentMobility::Movable);
        Mesh->SetStaticMesh(Cube);
        Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
        Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
        Mesh->SetGenerateOverlapEvents(false);
        Mesh->SetCastShadow(false);
        Mesh->SetReceivesDecals(true);
        UMaterialInstanceDynamic* Material =
            UMaterialInstanceDynamic::Create(Surface, Ground);
        if (Material == nullptr)
        {
            Ground->Destroy();
            return false;
        }
        Material->SetVectorParameterValue(
            TEXT("Color"), FLinearColor(0.035f, 0.018f, 0.020f));
        Material->SetScalarParameterValue(TEXT("Metallic"), 0.18f);
        Material->SetScalarParameterValue(TEXT("Roughness"), 0.72f);
        Material->SetScalarParameterValue(TEXT("EmissiveStrength"), 0.0f);
        Mesh->SetMaterial(0, Material);
        Ground->SetActorScale3D(FVector(
            static_cast<float>(Keyframe.mapWidthTiles) *
                NetworkTileWorldSize / 100.0f,
            static_cast<float>(Keyframe.mapHeightTiles) *
                NetworkTileWorldSize / 100.0f,
            0.12f));
        Ground->Tags.Add(TEXT("EchoesNetworkGround"));
        NetworkGroundView = Ground;
    }
    if (!NetworkTerrainView.IsValid())
    {
        AEchoesTerrainView* Terrain = World->SpawnActor<AEchoesTerrainView>(
            AEchoesTerrainView::StaticClass(),
            FTransform::Identity,
            SpawnParameters);
        if (Terrain == nullptr ||
            !Terrain->InitializeScopedTerrain(
                Keyframe.mapWidthTiles,
                Keyframe.mapHeightTiles,
                NetworkTileWorldSize))
        {
            if (Terrain != nullptr)
            {
                Terrain->Destroy();
            }
            return false;
        }
        NetworkTerrainView = Terrain;
    }
    if (!NetworkFogView.IsValid())
    {
        AEchoesFogView* Fog = World->SpawnActor<AEchoesFogView>(
            AEchoesFogView::StaticClass(),
            FTransform::Identity,
            SpawnParameters);
        if (Fog == nullptr ||
            !Fog->InitializeScopedFog(
                Keyframe.mapWidthTiles,
                Keyframe.mapHeightTiles,
                NetworkTileWorldSize))
        {
            if (Fog != nullptr)
            {
                Fog->Destroy();
            }
            return false;
        }
        NetworkFogView = Fog;
    }
    if (!NetworkTerrainView->SyncScopedTerrain(Keyframe.tiles) ||
        !NetworkFogView->SyncScopedVisibility(Keyframe.tiles))
    {
        return false;
    }

    TSet<uint32> LiveEntityIds;
    LiveEntityIds.Reserve(static_cast<int32>(Keyframe.entities.size()));
    for (const echoes::sim::net::ScopedEntityState& Scoped : Keyframe.entities)
    {
        if (LiveEntityIds.Contains(Scoped.id))
        {
            UE_LOG(
                LogEchoes,
                Error,
                TEXT("[ECHOES_NETWORK_PRESENTATION_FAILED] reason=DUPLICATE_ENTITY_ID entity=%u snapshot=%llu"),
                Scoped.id,
                static_cast<unsigned long long>(Keyframe.snapshotId));
            return false;
        }
        LiveEntityIds.Add(Scoped.id);
    }

    // Retire in stable identity order before acquiring replacements so a
    // visibility delta cannot create a transient spawn-before-retire spike.
    TArray<uint32> RemovedEntityIds;
    for (const TPair<uint32, TWeakObjectPtr<AEchoesEntityView>>& Pair :
         NetworkEntityViews)
    {
        if (!LiveEntityIds.Contains(Pair.Key))
        {
            RemovedEntityIds.Add(Pair.Key);
        }
    }
    RemovedEntityIds.Sort();
    for (const uint32 Removed : RemovedEntityIds)
    {
        AEchoesEntityView* View = nullptr;
        if (const TWeakObjectPtr<AEchoesEntityView>* Existing =
                NetworkEntityViews.Find(Removed))
        {
            View = Existing->Get();
        }
        NetworkEntityViews.Remove(Removed);
        ReleaseNetworkEntityView(View);
    }

    for (const echoes::sim::net::ScopedEntityState& Scoped : Keyframe.entities)
    {
        AEchoesEntityView* View = nullptr;
        if (TWeakObjectPtr<AEchoesEntityView>* Existing =
                NetworkEntityViews.Find(Scoped.id))
        {
            View = Existing->Get();
        }
        const bool bAcquiredView = View == nullptr;
        if (bAcquiredView)
        {
            View = AcquireNetworkEntityView();
            if (View == nullptr)
            {
                return false;
            }
            NetworkEntityViews.Add(Scoped.id, View);
        }
        const echoes::sim::Entity State =
            BuildNetworkPresentationEntity(Scoped);
        if (bAcquiredView)
        {
            View->ActivateForEntity(State, true);
        }
        else
        {
            View->ApplyAuthoritativeState(State, false);
        }
    }
    const bool bFirstPresentation = !bNetworkRemoteBattlefieldReady;
    bNetworkRemoteBattlefieldReady = true;
    if (bFirstPresentation)
    {
        SetStatusMessage(
            TEXT("REMOTE BATTLEFIELD — visibility-scoped authoritative state active."),
            3600.0f);
    }
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_NETWORK_PRESENTATION_SYNCED] snapshot=%llu tick=%llu entities=%d pooled=%d poolCapacity=%d tiles=%d removed=%d retireBeforeAcquire=true ground=true terrain=true fog=true lighting=true scopedOnly=true rendered=true"),
        static_cast<unsigned long long>(Keyframe.snapshotId),
        static_cast<unsigned long long>(Keyframe.simulationTick),
        NetworkEntityViews.Num(),
        NetworkFreeEntityViews.Num(),
        UEchoesSimulationSubsystem::GetEntityViewFreePoolCapacity(),
        static_cast<int32>(Keyframe.tiles.size()),
        RemovedEntityIds.Num());
    return true;
}

AEchoesEntityView* AEchoesPlayerController::AcquireNetworkEntityView()
{
    while (!NetworkFreeEntityViews.IsEmpty())
    {
        AEchoesEntityView* View =
            NetworkFreeEntityViews.Pop(EAllowShrinking::No);
        if (IsValid(View) && !View->IsActorBeingDestroyed())
        {
            return View;
        }
    }

    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return nullptr;
    }
    FActorSpawnParameters SpawnParameters;
    SpawnParameters.ObjectFlags |= RF_Transient;
    SpawnParameters.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AEchoesEntityView* View = World->SpawnActor<AEchoesEntityView>(
        AEchoesEntityView::StaticClass(),
        FTransform::Identity,
        SpawnParameters);
    if (View != nullptr)
    {
        View->Tags.Add(TEXT("EchoesNetworkEntityView"));
        View->PrepareForPool();
    }
    return View;
}

void AEchoesPlayerController::ReleaseNetworkEntityView(
    AEchoesEntityView* View)
{
    if (!IsValid(View) || View->IsActorBeingDestroyed())
    {
        return;
    }
    View->PrepareForPool();
    if (NetworkFreeEntityViews.Num() <
        UEchoesSimulationSubsystem::GetEntityViewFreePoolCapacity())
    {
        NetworkFreeEntityViews.Add(View);
        return;
    }
    View->Destroy();
}

void AEchoesPlayerController::DestroyNetworkPresentation()
{
    for (const TPair<uint32, TWeakObjectPtr<AEchoesEntityView>>& Pair :
         NetworkEntityViews)
    {
        if (AEchoesEntityView* View = Pair.Value.Get())
        {
            View->Destroy();
        }
    }
    NetworkEntityViews.Reset();
    for (AEchoesEntityView* View : NetworkFreeEntityViews)
    {
        if (IsValid(View) && !View->IsActorBeingDestroyed())
        {
            View->Destroy();
        }
    }
    NetworkFreeEntityViews.Reset();
    if (AEchoesFogView* Fog = NetworkFogView.Get())
    {
        Fog->Destroy();
    }
    if (AEchoesTerrainView* Terrain = NetworkTerrainView.Get())
    {
        Terrain->Destroy();
    }
    if (AStaticMeshActor* Ground = NetworkGroundView.Get())
    {
        Ground->Destroy();
    }
    if (ADirectionalLight* Sun = NetworkDirectionalLight.Get())
    {
        Sun->Destroy();
    }
    if (ASkyLight* Sky = NetworkSkyLight.Get())
    {
        Sky->Destroy();
    }
    NetworkFogView.Reset();
    NetworkTerrainView.Reset();
    NetworkGroundView.Reset();
    NetworkDirectionalLight.Reset();
    NetworkSkyLight.Reset();
    bNetworkRemoteBattlefieldReady = false;
}

void AEchoesPlayerController::ServerAcknowledgeScopedKeyframe_Implementation(
    uint64 SnapshotId,
    uint64 ScopedDigest)
{
    const uint64* ExpectedDigest =
        PendingNetworkSnapshotDigests.Find(SnapshotId);
    if (ExpectedDigest == nullptr || *ExpectedDigest != ScopedDigest ||
        SnapshotId <= LastAcknowledgedNetworkSnapshotId)
    {
        // A bound client can repeat invalid acknowledgements indefinitely;
        // one line per window keeps the rejection observable without an
        // unbounded log channel.
        static double LastAckRejectionLogSeconds = -1.0;
        static uint64 SuppressedAckRejectionLogs = 0;
        const double Now = FPlatformTime::Seconds();
        if (LastAckRejectionLogSeconds >= 0.0 &&
            Now - LastAckRejectionLogSeconds < 5.0)
        {
            ++SuppressedAckRejectionLogs;
            return;
        }
        LastAckRejectionLogSeconds = Now;
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NETWORK_KEYFRAME_ACK_REJECTED] player=%u snapshot=%llu digest=%llu lastAck=%llu reason=NET_SNAPSHOT_LINEAGE_INVALID suppressedRepeats=%llu"),
            NetworkSeat,
            static_cast<unsigned long long>(SnapshotId),
            static_cast<unsigned long long>(ScopedDigest),
            static_cast<unsigned long long>(LastAcknowledgedNetworkSnapshotId),
            static_cast<unsigned long long>(SuppressedAckRejectionLogs));
        SuppressedAckRejectionLogs = 0;
        return;
    }
    int32 RetiredSnapshotCount = 0;
    for (auto It = PendingNetworkSnapshotDigests.CreateIterator(); It; ++It)
    {
        if (It.Key() <= SnapshotId)
        {
            It.RemoveCurrent();
            ++RetiredSnapshotCount;
        }
    }
    LastAcknowledgedNetworkSnapshotId = SnapshotId;
    ++NetworkSnapshotAcknowledgementCount;
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_NETWORK_KEYFRAME_ACKNOWLEDGED] player=%u snapshot=%llu digest=%llu acknowledgements=%llu retired=%d pendingSnapshots=%d lineageExact=true"),
        NetworkSeat,
        static_cast<unsigned long long>(SnapshotId),
        static_cast<unsigned long long>(ScopedDigest),
        static_cast<unsigned long long>(NetworkSnapshotAcknowledgementCount),
        RetiredSnapshotCount,
        PendingNetworkSnapshotDigests.Num());
}

void AEchoesPlayerController::ServerRequestScopedKeyframe_Implementation(
    uint64 LastAcceptedSnapshotId)
{
    const double Now = FPlatformTime::Seconds();
    if (Now - LastScopedRecoveryRequestServerSeconds < 1.0)
    {
        UE_LOG(
            LogEchoes,
            Warning,
            TEXT("[ECHOES_NETWORK_KEYFRAME_REQUEST_REJECTED] player=%u lastAccepted=%llu reason=NET_RECOVERY_RATE_LIMITED"),
            NetworkSeat,
            static_cast<unsigned long long>(LastAcceptedSnapshotId));
        return;
    }
    LastScopedRecoveryRequestServerSeconds = Now;
    UE_LOG(
        LogEchoes,
        Warning,
        TEXT("[ECHOES_NETWORK_KEYFRAME_REQUESTED] player=%u lastAccepted=%llu authorityLatest=%llu recovery=fullKeyframe"),
        NetworkSeat,
        static_cast<unsigned long long>(LastAcceptedSnapshotId),
        static_cast<unsigned long long>(LastNetworkSnapshotId));
    SendScopedKeyframe();
}

void AEchoesPlayerController::ServerSubmitNetworkCommand_Implementation(
    const TArray<uint8>& Packet)
{
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (!bNetworkCompatibilityAccepted || !bNetworkMatchStarted ||
        Bridge == nullptr)
    {
        ClientReceiveCommandAdmission(
            static_cast<uint8>(
                echoes::sim::net::CommandAdmissionStatus::InvalidSeat),
            0,
            !bNetworkCompatibilityAccepted
                ? TEXT("NET_COMPATIBILITY_REQUIRED")
                : TEXT("NET_MATCH_NOT_STARTED"));
        return;
    }
    const double CommandNow = FPlatformTime::Seconds();
    if (!NetworkCommandRateLimiter.TryConsume(CommandNow))
    {
        ClientReceiveCommandAdmission(
            static_cast<uint8>(
                echoes::sim::net::CommandAdmissionStatus::CommandRejected),
            Bridge->GetSimulation() != nullptr
                ? Bridge->GetSimulation()->CurrentTick()
                : 0,
            TEXT("NET_COMMAND_RATE_LIMITED"));
        UE_LOG(
            LogEchoes,
            Warning,
            TEXT("[ECHOES_NETWORK_COMMAND_RATE_LIMITED] player=%u commands=%u windowSeconds=1 limit=8"),
            NetworkSeat,
            NetworkCommandRateLimiter.CurrentCount());
        return;
    }
    echoes::sim::net::CommandRequest Request{};
    const echoes::sim::net::DecodeStatus Decode =
        echoes::sim::net::DecodeCommandRequest(
            echoes::network::AsByteSpan(Packet), Request);
    if (Decode != echoes::sim::net::DecodeStatus::Ok)
    {
        ClientReceiveCommandAdmission(
            static_cast<uint8>(
                echoes::sim::net::CommandAdmissionStatus::CommandRejected),
            Bridge->GetSimulation() != nullptr
                ? Bridge->GetSimulation()->CurrentTick()
                : 0,
            FString(UTF8_TO_TCHAR(
                echoes::sim::net::StableId(Decode).data())));
        return;
    }
    const uint64 RequestedExecuteTick = Request.executeTick;
    const echoes::sim::Simulation* Simulation = Bridge->GetSimulation();
    if (Simulation == nullptr ||
        Simulation->CurrentTick() >
            std::numeric_limits<echoes::sim::Tick>::max() - 3)
    {
        ClientReceiveCommandAdmission(
            static_cast<uint8>(
                echoes::sim::net::CommandAdmissionStatus::TickRangeInvalid),
            Simulation != nullptr ? Simulation->CurrentTick() : 0,
            TEXT("NET_AUTHORITY_TICK_RANGE_INVALID"));
        return;
    }
    Request.executeTick = Simulation->CurrentTick() + 3;
    const echoes::sim::Entity* Actor = Bridge->FindEntity(Request.actor);
    PendingRemoteInitialPosition =
        Actor != nullptr ? Actor->position : echoes::sim::Vec2{};
    std::string Rejection;
    const echoes::sim::net::CommandAdmissionStatus Admission =
        Bridge->AdmitNetworkCommand(
            Request, NetworkCommandContext, &Rejection);
    const uint64 ServerTick =
        Bridge->GetSimulation() != nullptr
            ? Bridge->GetSimulation()->CurrentTick()
            : 0;
    const FString RejectionText = Rejection.empty()
        ? FString()
        : FString(UTF8_TO_TCHAR(Rejection.c_str()));
    ClientReceiveCommandAdmission(
        static_cast<uint8>(Admission), ServerTick, RejectionText);
    if (Admission == echoes::sim::net::CommandAdmissionStatus::Accepted)
    {
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_NETWORK_COMMAND_ADMISSION] player=%u status=%s sequence=%llu requestedExecuteTick=%llu assignedExecuteTick=%llu actor=%u serverTick=%llu authorityAssigned=true simulationReason=%s"),
            NetworkSeat,
            UTF8_TO_TCHAR(echoes::sim::net::StableId(Admission).data()),
            static_cast<unsigned long long>(Request.sequence),
            static_cast<unsigned long long>(RequestedExecuteTick),
            static_cast<unsigned long long>(Request.executeTick),
            Request.actor,
            static_cast<unsigned long long>(ServerTick),
            RejectionText.IsEmpty() ? TEXT("none") : *RejectionText);
    }
    else
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NETWORK_COMMAND_ADMISSION] player=%u status=%s sequence=%llu requestedExecuteTick=%llu assignedExecuteTick=%llu actor=%u serverTick=%llu authorityAssigned=true simulationReason=%s"),
            NetworkSeat,
            UTF8_TO_TCHAR(echoes::sim::net::StableId(Admission).data()),
            static_cast<unsigned long long>(Request.sequence),
            static_cast<unsigned long long>(RequestedExecuteTick),
            static_cast<unsigned long long>(Request.executeTick),
            Request.actor,
            static_cast<unsigned long long>(ServerTick),
            RejectionText.IsEmpty() ? TEXT("none") : *RejectionText);
    }
    if (Admission == echoes::sim::net::CommandAdmissionStatus::Accepted)
    {
        PendingRemoteCommand = Request;
        GetWorldTimerManager().SetTimer(
            NetworkExecutionTimer,
            this,
            &AEchoesPlayerController::VerifyRemoteCommandExecution,
            1.0f,
            false);
    }
}

void AEchoesPlayerController::ServerSubmitNetworkCommandBatch_Implementation(
    const TArray<uint8>& Packet)
{
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    const echoes::sim::Simulation* Simulation =
        Bridge != nullptr ? Bridge->GetSimulation() : nullptr;
    if (!bNetworkCompatibilityAccepted || !bNetworkMatchStarted ||
        Bridge == nullptr || Simulation == nullptr)
    {
        ClientReceiveCommandBatchAdmission(
            0,
            0,
            0,
            Simulation != nullptr ? Simulation->CurrentTick() : 0,
            !bNetworkCompatibilityAccepted
                ? TEXT("NET_COMPATIBILITY_REQUIRED")
                : TEXT("NET_MATCH_NOT_STARTED"));
        return;
    }

    echoes::sim::net::CommandBatchRequest Batch{};
    const echoes::sim::net::DecodeStatus Decode =
        echoes::sim::net::DecodeCommandBatchRequest(
            echoes::network::AsByteSpan(Packet), Batch);
    if (Decode != echoes::sim::net::DecodeStatus::Ok)
    {
        ClientReceiveCommandBatchAdmission(
            0,
            0,
            0,
            Simulation->CurrentTick(),
            FString(UTF8_TO_TCHAR(
                echoes::sim::net::StableId(Decode).data())));
        return;
    }

    const uint64 ExpectedBatchId = LastAcceptedNetworkBatchId + 1;
    if (ExpectedBatchId == 0 || Batch.clientBatchId != ExpectedBatchId)
    {
        ClientReceiveCommandBatchAdmission(
            Batch.clientBatchId,
            0,
            static_cast<int32>(Batch.intents.size()),
            Simulation->CurrentTick(),
            TEXT("NET_COMMAND_BATCH_SEQUENCE_UNEXPECTED"));
        UE_LOG(
            LogEchoes,
            Warning,
            TEXT("[ECHOES_NETWORK_COMMAND_BATCH_REJECTED] player=%u batch=%llu expected=%llu reason=NET_COMMAND_BATCH_SEQUENCE_UNEXPECTED"),
            NetworkSeat,
            static_cast<unsigned long long>(Batch.clientBatchId),
            static_cast<unsigned long long>(ExpectedBatchId));
        return;
    }
    // A syntactically valid next batch consumes the client-batch sequence even
    // when rate or semantic admission rejects every contained intent.
    LastAcceptedNetworkBatchId = Batch.clientBatchId;

    const double CommandNow = FPlatformTime::Seconds();
    if (!NetworkCommandRateLimiter.TryConsume(
            CommandNow,
            static_cast<std::uint32_t>(Batch.intents.size())))
    {
        ClientReceiveCommandBatchAdmission(
            Batch.clientBatchId,
            0,
            static_cast<int32>(Batch.intents.size()),
            Simulation->CurrentTick(),
            TEXT("NET_COMMAND_RATE_LIMITED"));
        UE_LOG(
            LogEchoes,
            Warning,
            TEXT("[ECHOES_NETWORK_COMMAND_RATE_LIMITED] player=%u requests=%u intents=%u windowSeconds=1 requestLimit=%u intentLimit=%u"),
            NetworkSeat,
            NetworkCommandRateLimiter.CurrentCount(),
            NetworkCommandRateLimiter.CurrentIntentCount(),
            echoes::network::CommandRateLimiter::MaximumCommandsPerWindow,
            echoes::network::CommandRateLimiter::MaximumIntentsPerWindow);
        return;
    }
    if (Simulation->CurrentTick() >
        std::numeric_limits<echoes::sim::Tick>::max() - 3)
    {
        ClientReceiveCommandBatchAdmission(
            Batch.clientBatchId,
            0,
            static_cast<int32>(Batch.intents.size()),
            Simulation->CurrentTick(),
            TEXT("NET_AUTHORITY_TICK_RANGE_INVALID"));
        return;
    }

    const uint64 AssignedExecuteTick = Simulation->CurrentTick() + 3;
    int32 AcceptedCount = 0;
    int32 RejectedCount = 0;
    FString FirstRejection;
    for (const echoes::sim::net::CommandIntent& Intent : Batch.intents)
    {
        uint64 NextSequence = 1;
        if (NetworkCommandContext.hasAcceptedSequence)
        {
            if (NetworkCommandContext.lastAcceptedSequence ==
                std::numeric_limits<uint64>::max())
            {
                ++RejectedCount;
                if (FirstRejection.IsEmpty())
                {
                    FirstRejection = TEXT("NET_COMMAND_SEQUENCE_EXHAUSTED");
                }
                continue;
            }
            NextSequence = NetworkCommandContext.lastAcceptedSequence + 1;
        }
        echoes::sim::net::CommandRequest Request{};
        Request.sequence = NextSequence;
        Request.executeTick = AssignedExecuteTick;
        Request.type = Intent.type;
        Request.actor = Intent.actor;
        Request.target = Intent.target;
        Request.position = Intent.position;
        Request.buildType = Intent.buildType;
        Request.wellChoice = Intent.wellChoice;
        Request.warformAdaptation = Intent.warformAdaptation;
        Request.researchType = Intent.researchType;
        std::string Rejection;
        const echoes::sim::net::CommandAdmissionStatus Admission =
            Bridge->AdmitNetworkCommand(
                Request, NetworkCommandContext, &Rejection);
        if (Admission == echoes::sim::net::CommandAdmissionStatus::Accepted)
        {
            ++AcceptedCount;
        }
        else
        {
            ++RejectedCount;
            if (FirstRejection.IsEmpty())
            {
                FirstRejection = Rejection.empty()
                    ? FString(UTF8_TO_TCHAR(
                          echoes::sim::net::StableId(Admission).data()))
                    : FString(UTF8_TO_TCHAR(Rejection.c_str()));
            }
        }
        UE_LOG(
            LogEchoes,
            Verbose,
            TEXT("[ECHOES_NETWORK_COMMAND_BATCH_INTENT] player=%u batch=%llu actor=%u type=%u status=%s assignedSequence=%llu assignedExecuteTick=%llu"),
            NetworkSeat,
            static_cast<unsigned long long>(Batch.clientBatchId),
            Intent.actor,
            static_cast<uint8>(Intent.type),
            UTF8_TO_TCHAR(echoes::sim::net::StableId(Admission).data()),
            static_cast<unsigned long long>(NextSequence),
            static_cast<unsigned long long>(AssignedExecuteTick));
    }

    ClientReceiveCommandBatchAdmission(
        Batch.clientBatchId,
        AcceptedCount,
        RejectedCount,
        Simulation->CurrentTick(),
        FirstRejection);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_NETWORK_COMMAND_BATCH_ADMISSION] player=%u batch=%llu intents=%d accepted=%d rejected=%d assignedExecuteTick=%llu lastAcceptedSequence=%llu authorityAssigned=true firstRejection=%s"),
        NetworkSeat,
        static_cast<unsigned long long>(Batch.clientBatchId),
        static_cast<int32>(Batch.intents.size()),
        AcceptedCount,
        RejectedCount,
        static_cast<unsigned long long>(AssignedExecuteTick),
        static_cast<unsigned long long>(
            NetworkCommandContext.lastAcceptedSequence),
        FirstRejection.IsEmpty() ? TEXT("none") : *FirstRejection);
}

void AEchoesPlayerController::ClientReceiveCommandBatchAdmission_Implementation(
    uint64 BatchId,
    int32 AcceptedCount,
    int32 RejectedCount,
    uint64 ServerTick,
    const FString& FirstRejection)
{
    const FString Detail = FirstRejection.IsEmpty()
        ? TEXT("none")
        : FirstRejection;
    if (AcceptedCount > 0)
    {
        SetStatusMessage(
            FString::Printf(
                TEXT("ONLINE ORDER %llu — authority accepted %d, rejected %d."),
                static_cast<unsigned long long>(BatchId),
                AcceptedCount,
                RejectedCount),
            4.0f);
    }
    else
    {
        SetStatusMessage(
            FString::Printf(
                TEXT("[ONLINE_ORDER_REJECTED] Batch %llu: %s"),
                static_cast<unsigned long long>(BatchId),
                *Detail),
            5.0f);
    }
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_NETWORK_COMMAND_BATCH_RESULT] batch=%llu accepted=%d rejected=%d serverTick=%llu firstRejection=%s"),
        static_cast<unsigned long long>(BatchId),
        AcceptedCount,
        RejectedCount,
        static_cast<unsigned long long>(ServerTick),
        *Detail);
    if (bNetworkMatchSmoke && BatchId == 1 && AcceptedCount >= 2 &&
        RejectedCount == 0)
    {
        bNetworkMatchBatchAdmitted = true;
    }
    if ((bNetworkReconnectPhaseOneSmoke ||
         bNetworkReconnectPhaseTwoSmoke) &&
        BatchId == NetworkReconnectExpectedBatchId &&
        AcceptedCount == 1 && RejectedCount == 0)
    {
        bNetworkReconnectBatchAdmitted = true;
    }
}

void AEchoesPlayerController::QueueNetworkSmokeHostCommand()
{
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    const echoes::sim::Simulation* Simulation =
        Bridge != nullptr ? Bridge->GetSimulation() : nullptr;
    if (Bridge == nullptr || Simulation == nullptr)
    {
        return;
    }
    const auto Worker = std::find_if(
        Simulation->Entities().begin(),
        Simulation->Entities().end(),
        [](const echoes::sim::Entity& Entity)
        {
            return Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
                   Entity.type == echoes::sim::EntityType::Worker;
        });
    if (Worker == Simulation->Entities().end())
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NETWORK_HOST_COMMAND_FAILED] reason=NET_HOST_WORKER_UNAVAILABLE"));
        return;
    }
    PendingHostCommandActor = Worker->id;
    PendingHostCommandInitialPosition = Worker->position;
    PendingHostCommandTargetPosition = echoes::sim::Vec2::FromRaw(
        Worker->position.x.Raw() + echoes::sim::kFixedScale,
        Worker->position.y.Raw());
    PendingHostCommandExecuteTick = Simulation->CurrentTick() + 3;
    FString Feedback;
    if (!Bridge->IssueCommand(
            echoes::sim::CommandType::Move,
            PendingHostCommandActor,
            0,
            Bridge->SimToWorld(PendingHostCommandTargetPosition),
            echoes::sim::FutureWellChoice::Dormant,
            Feedback))
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NETWORK_HOST_COMMAND_FAILED] actor=%u reason=%s"),
            PendingHostCommandActor,
            *Feedback);
        PendingHostCommandActor = 0;
        return;
    }
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_NETWORK_HOST_COMMAND_QUEUED] player=%u actor=%u assignedExecuteTick=%llu authorityTick=%llu delayTicks=3 targetRaw=(%d,%d)"),
        UEchoesSimulationSubsystem::LocalPlayerId,
        PendingHostCommandActor,
        static_cast<unsigned long long>(PendingHostCommandExecuteTick),
        static_cast<unsigned long long>(Simulation->CurrentTick()),
        PendingHostCommandTargetPosition.x.Raw(),
        PendingHostCommandTargetPosition.y.Raw());
}

void AEchoesPlayerController::ClientReceiveCommandAdmission_Implementation(
    uint8 Status,
    uint64 ServerTick,
    const FString& SimulationReason)
{
    const auto Admission =
        static_cast<echoes::sim::net::CommandAdmissionStatus>(Status);
    if (Admission == echoes::sim::net::CommandAdmissionStatus::Accepted)
    {
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_NETWORK_COMMAND_RESULT] status=%s serverTick=%llu simulationReason=%s"),
            UTF8_TO_TCHAR(echoes::sim::net::StableId(Admission).data()),
            static_cast<unsigned long long>(ServerTick),
            SimulationReason.IsEmpty() ? TEXT("none") : *SimulationReason);
    }
    else
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NETWORK_COMMAND_RESULT] status=%s serverTick=%llu simulationReason=%s"),
            UTF8_TO_TCHAR(echoes::sim::net::StableId(Admission).data()),
            static_cast<unsigned long long>(ServerTick),
            SimulationReason.IsEmpty() ? TEXT("none") : *SimulationReason);
    }
    if (Admission != echoes::sim::net::CommandAdmissionStatus::Accepted &&
        bNetworkClientSmoke)
    {
        FPlatformMisc::RequestExit(false);
    }
}

void AEchoesPlayerController::VerifyRemoteCommandExecution()
{
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    const echoes::sim::Simulation* Simulation =
        Bridge != nullptr ? Bridge->GetSimulation() : nullptr;
    const echoes::sim::Entity* Actor =
        Bridge != nullptr ? Bridge->FindEntity(PendingRemoteCommand.actor)
                          : nullptr;
    const bool bExecuted =
        Simulation != nullptr && Actor != nullptr &&
        Simulation->CurrentTick() > PendingRemoteCommand.executeTick &&
        Actor->position == PendingRemoteCommand.position &&
        Actor->position != PendingRemoteInitialPosition;
    const echoes::sim::Entity* HostActor =
        Bridge != nullptr ? Bridge->FindEntity(PendingHostCommandActor)
                          : nullptr;
    bNetworkHostExecutionVerified =
        Simulation != nullptr && HostActor != nullptr &&
        Simulation->CurrentTick() > PendingHostCommandExecuteTick &&
        HostActor->position == PendingHostCommandTargetPosition &&
        HostActor->position != PendingHostCommandInitialPosition;
    bNetworkCommandExecutionVerified = bExecuted;
    const uint64 ServerTick =
        Simulation != nullptr ? Simulation->CurrentTick() : 0;
    ClientReceiveCommandExecution(
        bExecuted,
        PendingRemoteCommand.actor,
        Actor != nullptr ? Actor->position.x.Raw() : 0,
        Actor != nullptr ? Actor->position.y.Raw() : 0,
        ServerTick);
    if (bExecuted)
    {
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_NETWORK_COMMAND_EXECUTION] executed=true actor=%u positionRaw=(%d,%d) expectedRaw=(%d,%d) serverTick=%llu"),
            PendingRemoteCommand.actor,
            Actor != nullptr ? Actor->position.x.Raw() : 0,
            Actor != nullptr ? Actor->position.y.Raw() : 0,
            PendingRemoteCommand.position.x.Raw(),
            PendingRemoteCommand.position.y.Raw(),
            static_cast<unsigned long long>(ServerTick));
    }
    else
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NETWORK_COMMAND_EXECUTION] executed=false actor=%u positionRaw=(%d,%d) expectedRaw=(%d,%d) serverTick=%llu"),
            PendingRemoteCommand.actor,
            Actor != nullptr ? Actor->position.x.Raw() : 0,
            Actor != nullptr ? Actor->position.y.Raw() : 0,
            PendingRemoteCommand.position.x.Raw(),
            PendingRemoteCommand.position.y.Raw(),
            static_cast<unsigned long long>(ServerTick));
    }
    if (bNetworkHostExecutionVerified)
    {
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_NETWORK_HOST_COMMAND_EXECUTION] executed=true actor=%u positionRaw=(%d,%d) expectedRaw=(%d,%d) serverTick=%llu delayTicks=3"),
            PendingHostCommandActor,
            HostActor->position.x.Raw(),
            HostActor->position.y.Raw(),
            PendingHostCommandTargetPosition.x.Raw(),
            PendingHostCommandTargetPosition.y.Raw(),
            static_cast<unsigned long long>(ServerTick));
    }
    else
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NETWORK_HOST_COMMAND_EXECUTION] executed=false actor=%u positionRaw=(%d,%d) expectedRaw=(%d,%d) serverTick=%llu delayTicks=3"),
            PendingHostCommandActor,
            HostActor != nullptr ? HostActor->position.x.Raw() : 0,
            HostActor != nullptr ? HostActor->position.y.Raw() : 0,
            PendingHostCommandTargetPosition.x.Raw(),
            PendingHostCommandTargetPosition.y.Raw(),
            static_cast<unsigned long long>(ServerTick));
    }
}

void AEchoesPlayerController::ClientReceiveCommandExecution_Implementation(
    bool bExecuted,
    uint32 ActorId,
    int32 PositionXRaw,
    int32 PositionYRaw,
    uint64 ServerTick)
{
    if (bExecuted)
    {
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_NETWORK_EXECUTION_RESULT] executed=true actor=%u positionRaw=(%d,%d) serverTick=%llu"),
            ActorId,
            PositionXRaw,
            PositionYRaw,
            static_cast<unsigned long long>(ServerTick));
    }
    else
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NETWORK_EXECUTION_RESULT] executed=false actor=%u positionRaw=(%d,%d) serverTick=%llu"),
            ActorId,
            PositionXRaw,
            PositionYRaw,
            static_cast<unsigned long long>(ServerTick));
    }
    if (!bNetworkClientSmoke)
    {
        return;
    }
    if (!bExecuted)
    {
        FPlatformMisc::RequestExit(false);
        return;
    }
    bNetworkRemoteExecutionReceived = true;
    TryFinishNetworkClientSmoke();
}

void AEchoesPlayerController::TryFinishNetworkClientSmoke()
{
    const bool bRecoveryRequired =
        FParse::Param(
            FCommandLine::Get(), TEXT("EchoesNetworkDropFirstDelta")) ||
        bNetworkReorderFirstTwoDeltasForSmoke ||
        bNetworkDropDeltaBurstForSmoke;
    const bool bFaultAcceptanceSatisfied =
        (!bRecoveryRequired || bNetworkFaultRecoveryObserved) &&
        (!bNetworkDelayFirstDeltaForSmoke ||
         bNetworkDelayedDeltaDelivered) &&
        (!bNetworkDuplicateFirstDeltaForSmoke ||
         bNetworkDuplicateDeltaIgnored);
    if (!bNetworkClientSmoke || bNetworkSmokeCompletionSent ||
        !bNetworkRemoteExecutionReceived ||
        NetworkViewState.AcceptedCount() < 2 ||
        !bFaultAcceptanceSatisfied)
    {
        return;
    }
    bNetworkSmokeCompletionSent = true;
    NetworkSmokeCompletionSnapshotId = LastNetworkSnapshotId;
    ServerConfirmNetworkSmokeComplete(NetworkSmokeCompletionSnapshotId);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_NETWORK_CLIENT_COMPLETION_REQUESTED] snapshot=%llu acceptedKeyframes=%llu reliable=true waitingForAuthority=true"),
        static_cast<unsigned long long>(LastNetworkSnapshotId),
        static_cast<unsigned long long>(NetworkViewState.AcceptedCount()));
}

void AEchoesPlayerController::TrySubmitNetworkMatchSmoke(
    const echoes::sim::net::ScopedViewKeyframe& Keyframe)
{
    if (!bNetworkMatchSmoke || bNetworkMatchCommandSubmitted ||
        Keyframe.player != NetworkSeat || !IsNetworkClientControlActive())
    {
        return;
    }

    const auto AuthorityCore = std::find_if(
        Keyframe.entities.begin(),
        Keyframe.entities.end(),
        [&](const echoes::sim::net::ScopedEntityState& Entity)
        {
            return Entity.owner != NetworkSeat &&
                   Entity.owner != echoes::sim::kNeutralPlayer &&
                   Entity.type == echoes::sim::EntityType::CommandCore;
        });
    if (AuthorityCore == Keyframe.entities.end())
    {
        return;
    }

    ClearSelection();
    for (const echoes::sim::net::ScopedEntityState& Entity : Keyframe.entities)
    {
        if (Entity.owner == NetworkSeat &&
            Entity.type == echoes::sim::EntityType::Soldier)
        {
            SelectedEntityIds.Add(Entity.id);
            SetEntitySelected(Entity.id, true);
        }
    }
    if (SelectedEntityIds.Num() < 2)
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NETWORK_MATCH_CLIENT_SMOKE_FAILED] reason=NET_MATCH_FIXTURE_ATTACKERS_UNAVAILABLE selected=%d"),
            SelectedEntityIds.Num());
        FPlatformMisc::RequestExit(false);
        return;
    }

    const FVector TargetWorld = NetworkSimToWorld(AuthorityCore->position);
    const int32 SubmittedActorCount = SelectedEntityIds.Num();
    bNetworkMatchCommandSubmitted = SubmitNetworkSelectionCommand(
        echoes::sim::CommandType::Attack,
        AuthorityCore->id,
        TargetWorld,
        false,
        false,
        TEXT("MATCH SMOKE DIRECT ATTACK"),
        EEchoesCommandMarkerType::Attack);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_NETWORK_MATCH_ORDER_SUBMITTED] submitted=%s selectedActors=%d targetCore=%u selectionAdapter=true orderAdapter=true batched=true"),
        bNetworkMatchCommandSubmitted ? TEXT("true") : TEXT("false"),
        SubmittedActorCount,
        AuthorityCore->id);
    if (!bNetworkMatchCommandSubmitted)
    {
        FPlatformMisc::RequestExit(false);
    }
}

bool AEchoesPlayerController::SubmitNetworkReconnectSmokeBatch(
    const echoes::sim::net::ScopedViewKeyframe& Keyframe)
{
    const auto OwnedWorker = std::find_if(
        Keyframe.entities.begin(),
        Keyframe.entities.end(),
        [&](const echoes::sim::net::ScopedEntityState& Entity)
        {
            return Entity.owner == NetworkSeat &&
                   Entity.type == echoes::sim::EntityType::Worker;
        });
    if (OwnedWorker == Keyframe.entities.end())
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NETWORK_RECONNECT_CLIENT_FAILED] reason=NET_OWNED_WORKER_UNAVAILABLE"));
        return false;
    }
    const int32 MaximumXRaw =
        Keyframe.mapWidthTiles * echoes::sim::kFixedScale - 1;
    echoes::sim::net::CommandIntent Intent{};
    Intent.type = echoes::sim::CommandType::Move;
    Intent.actor = OwnedWorker->id;
    Intent.position = echoes::sim::Vec2::FromRaw(
        FMath::Min(
            OwnedWorker->position.x.Raw() + echoes::sim::kFixedScale,
            MaximumXRaw),
        OwnedWorker->position.y.Raw());
    TArray<echoes::sim::net::CommandIntent> Intents;
    Intents.Add(Intent);
    NetworkReconnectExpectedBatchId = NextNetworkBatchId;
    NetworkReconnectExpectedSequence = Keyframe.lastAcceptedSequence + 1;
    NetworkReconnectActorId = OwnedWorker->id;
    NetworkReconnectInitialPosition = OwnedWorker->position;
    const bool bSubmitted = SubmitNetworkCommandBatch(
        MoveTemp(Intents),
        bNetworkReconnectPhaseOneSmoke
            ? TEXT("RECONNECT PHASE ONE MOVE")
            : TEXT("RECONNECT PHASE TWO MOVE"),
        NetworkSimToWorld(Intent.position),
        EEchoesCommandMarkerType::Move);
    if (bSubmitted)
    {
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_NETWORK_RECONNECT_ORDER] phase=%u submitted=true actor=%u batch=%llu expectedSequence=%llu"),
            bNetworkReconnectPhaseOneSmoke ? 1 : 2,
            NetworkReconnectActorId,
            static_cast<unsigned long long>(NetworkReconnectExpectedBatchId),
            static_cast<unsigned long long>(NetworkReconnectExpectedSequence));
    }
    else
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NETWORK_RECONNECT_ORDER] phase=%u submitted=false actor=%u batch=%llu expectedSequence=%llu"),
            bNetworkReconnectPhaseOneSmoke ? 1 : 2,
            NetworkReconnectActorId,
            static_cast<unsigned long long>(NetworkReconnectExpectedBatchId),
            static_cast<unsigned long long>(NetworkReconnectExpectedSequence));
    }
    return bSubmitted;
}

void AEchoesPlayerController::TryAdvanceNetworkReconnectSmoke(
    const echoes::sim::net::ScopedViewKeyframe& Keyframe)
{
    const bool bReconnectSmoke =
        bNetworkReconnectPhaseOneSmoke || bNetworkReconnectPhaseTwoSmoke;
    if (!bReconnectSmoke || bNetworkReconnectCompletionSent ||
        !IsNetworkClientControlActive())
    {
        return;
    }
    if (bNetworkReconnectPhaseTwoSmoke && !bNetworkResumeAccepted)
    {
        return;
    }
    if (!bNetworkReconnectBatchSubmitted)
    {
        if (bNetworkReconnectPhaseOneSmoke && NextNetworkBatchId != 1)
        {
            UE_LOG(
                LogEchoes,
                Error,
                TEXT("[ECHOES_NETWORK_RECONNECT_CLIENT_FAILED] phase=1 reason=NET_INITIAL_BATCH_SEQUENCE_INVALID nextBatch=%llu"),
                static_cast<unsigned long long>(NextNetworkBatchId));
            FPlatformMisc::RequestExit(false);
            return;
        }
        if (bNetworkReconnectPhaseTwoSmoke && NextNetworkBatchId != 2)
        {
            UE_LOG(
                LogEchoes,
                Error,
                TEXT("[ECHOES_NETWORK_RECONNECT_CLIENT_FAILED] phase=2 reason=NET_RESTORED_BATCH_SEQUENCE_INVALID nextBatch=%llu"),
                static_cast<unsigned long long>(NextNetworkBatchId));
            FPlatformMisc::RequestExit(false);
            return;
        }
        bNetworkReconnectBatchSubmitted =
            SubmitNetworkReconnectSmokeBatch(Keyframe);
        if (!bNetworkReconnectBatchSubmitted)
        {
            FPlatformMisc::RequestExit(false);
        }
        return;
    }
    if (!bNetworkReconnectBatchAdmitted ||
        Keyframe.lastAcceptedSequence < NetworkReconnectExpectedSequence)
    {
        return;
    }
    if (bNetworkReconnectPhaseOneSmoke)
    {
        if (NetworkResumeCredential.IsEmpty())
        {
            return;
        }
        bNetworkReconnectCompletionSent = true;
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_NETWORK_RECONNECT_PHASE_ONE_PASSED] credentialStaged=true credentialLogged=false snapshot=%llu tick=%llu lastAcceptedSequence=%llu nextBatch=%llu commandAdmitted=true intentionalDisconnect=true"),
            static_cast<unsigned long long>(Keyframe.snapshotId),
            static_cast<unsigned long long>(Keyframe.simulationTick),
            static_cast<unsigned long long>(Keyframe.lastAcceptedSequence),
            static_cast<unsigned long long>(NextNetworkBatchId));
        GetWorldTimerManager().SetTimer(
            NetworkClientExitTimer,
            this,
            &AEchoesPlayerController::FinishNetworkClientSmoke,
            0.1f,
            false);
        return;
    }

    const echoes::sim::net::ScopedEntityState* Actor =
        FindNetworkEntity(NetworkReconnectActorId);
    if (Actor == nullptr || Actor->position == NetworkReconnectInitialPosition ||
        Keyframe.simulationTick <= NetworkResumeDisconnectTick)
    {
        return;
    }
    bNetworkReconnectCompletionSent = true;
    ServerConfirmNetworkReconnectSmokeComplete(
        Keyframe.snapshotId,
        Keyframe.lastAcceptedSequence,
        NetworkReconnectExpectedBatchId);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_NETWORK_RECONNECT_PHASE_TWO_PASSED] seat=%u snapshot=%llu tick=%llu disconnectTick=%llu lastAcceptedSequence=%llu batch=%llu commandExecuted=true fullKeyframeResync=true credentialRotated=true separateProcess=true"),
        NetworkSeat,
        static_cast<unsigned long long>(Keyframe.snapshotId),
        static_cast<unsigned long long>(Keyframe.simulationTick),
        static_cast<unsigned long long>(NetworkResumeDisconnectTick),
        static_cast<unsigned long long>(Keyframe.lastAcceptedSequence),
        static_cast<unsigned long long>(NetworkReconnectExpectedBatchId));
    GetWorldTimerManager().SetTimer(
        NetworkClientExitTimer,
        this,
        &AEchoesPlayerController::FinishNetworkClientSmoke,
        0.5f,
        false);
}

void AEchoesPlayerController::ServerConfirmNetworkSmokeComplete_Implementation(
    uint64 SnapshotId)
{
    // Smoke confirmations are meaningful only to an authority that itself
    // launched in the matching smoke mode; an ordinary match server ignores
    // them entirely so a bound client cannot use this RPC as a log or probe
    // channel.
    if (!FParse::Param(
            FCommandLine::Get(), TEXT("EchoesNetworkListenSmoke")))
    {
        return;
    }
    if (!bNetworkCommandExecutionVerified ||
        !bNetworkHostExecutionVerified ||
        NetworkSnapshotAcknowledgementCount < 2 ||
        SnapshotId != LastAcknowledgedNetworkSnapshotId)
    {
        static double LastSmokeFailureLogSeconds = -1.0;
        static uint64 SuppressedSmokeFailureLogs = 0;
        const double Now = FPlatformTime::Seconds();
        if (LastSmokeFailureLogSeconds >= 0.0 &&
            Now - LastSmokeFailureLogSeconds < 5.0)
        {
            ++SuppressedSmokeFailureLogs;
            return;
        }
        LastSmokeFailureLogSeconds = Now;
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NETWORK_SERVER_SMOKE_FAILED] snapshot=%llu expectedAck=%llu remoteExecutionVerified=%s hostExecutionVerified=%s acknowledgements=%llu suppressedRepeats=%llu"),
            static_cast<unsigned long long>(SnapshotId),
            static_cast<unsigned long long>(LastAcknowledgedNetworkSnapshotId),
            bNetworkCommandExecutionVerified ? TEXT("true") : TEXT("false"),
            bNetworkHostExecutionVerified ? TEXT("true") : TEXT("false"),
            static_cast<unsigned long long>(NetworkSnapshotAcknowledgementCount),
            static_cast<unsigned long long>(SuppressedSmokeFailureLogs));
        SuppressedSmokeFailureLogs = 0;
        return;
    }
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_NETWORK_SERVER_SMOKE_PASSED] snapshot=%llu player=%u acknowledgements=%llu separateProcess=true readyGate=true periodicState=true hostRemoteDelayParity=true authorityAssignedCommands=true connectionBound=true hiddenAuthorityExcluded=true"),
        static_cast<unsigned long long>(SnapshotId),
        NetworkSeat,
        static_cast<unsigned long long>(NetworkSnapshotAcknowledgementCount));
    ClientConfirmNetworkSmokeComplete(SnapshotId);
    if (FParse::Param(
            FCommandLine::Get(), TEXT("EchoesNetworkListenSmoke")))
    {
        GetWorldTimerManager().SetTimer(
            NetworkServerExitTimer,
            FTimerDelegate::CreateLambda(
                []()
                {
                    FPlatformMisc::RequestExit(false);
                }),
            2.0f,
            false);
    }
}

void AEchoesPlayerController::ClientConfirmNetworkSmokeComplete_Implementation(
    uint64 SnapshotId)
{
    if (!bNetworkClientSmoke || !bNetworkSmokeCompletionSent ||
        SnapshotId != NetworkSmokeCompletionSnapshotId)
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NETWORK_CLIENT_SMOKE_FAILED] reason=NET_COMPLETION_CONFIRMATION_MISMATCH snapshot=%llu expected=%llu completionRequested=%s"),
            static_cast<unsigned long long>(SnapshotId),
            static_cast<unsigned long long>(NetworkSmokeCompletionSnapshotId),
            bNetworkSmokeCompletionSent ? TEXT("true") : TEXT("false"));
        // Only a client that itself entered smoke mode may terminate on a
        // failed confirmation; an ordinary client must survive a stray or
        // hostile smoke-confirm RPC.
        if (bNetworkClientSmoke)
        {
            FPlatformMisc::RequestExit(false);
        }
        return;
    }
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_NETWORK_CLIENT_SMOKE_PASSED] snapshot=%llu acceptedKeyframes=%llu authorityConfirmed=true reliableCompletion=true separateProcess=true readyGate=true periodicState=true authorityAssignedCommands=true connectionBound=true"),
        static_cast<unsigned long long>(SnapshotId),
        static_cast<unsigned long long>(NetworkViewState.AcceptedCount()));
    GetWorldTimerManager().SetTimer(
        NetworkClientExitTimer,
        this,
        &AEchoesPlayerController::FinishNetworkClientSmoke,
        0.5f,
        false);
}

void AEchoesPlayerController::ServerConfirmNetworkMatchSmokeComplete_Implementation(
    uint8 OutcomeValue,
    uint64 FinalTick,
    uint64 FinalSnapshotId,
    uint64 FinalScopedDigest)
{
    // Meaningful only to an authority launched in match-smoke mode; ordinary
    // servers ignore the RPC entirely.
    if (!FParse::Param(
            FCommandLine::Get(), TEXT("EchoesNetworkMatchSmoke")))
    {
        return;
    }
    const UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    const echoes::sim::Simulation* Simulation =
        Bridge != nullptr ? Bridge->GetSimulation() : nullptr;
    const bool bFinalViewExact = LastSentNetworkKeyframe.has_value() &&
        LastSentNetworkKeyframe->snapshotId == FinalSnapshotId &&
        LastSentNetworkKeyframe->simulationTick == FinalTick &&
        LastSentNetworkKeyframe->scopedDigest == FinalScopedDigest;
    const bool bAcknowledgedExact =
        LastAcknowledgedNetworkSnapshotId == FinalSnapshotId &&
        !PendingNetworkSnapshotDigests.Contains(FinalSnapshotId);
    const bool bOutcomeExact = Simulation != nullptr &&
        static_cast<uint8>(Simulation->Outcome()) == OutcomeValue &&
        Simulation->CurrentTick() == FinalTick &&
        Simulation->Outcome() == echoes::sim::MatchOutcome::Player1Victory;
    if (!bNetworkMatchResultSent || !bFinalViewExact ||
        !bAcknowledgedExact || !bOutcomeExact)
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NETWORK_MATCH_SERVER_SMOKE_FAILED] resultSent=%s finalViewExact=%s acknowledgedExact=%s outcomeExact=%s outcome=%u finalTick=%llu finalSnapshot=%llu finalDigest=%llu"),
            bNetworkMatchResultSent ? TEXT("true") : TEXT("false"),
            bFinalViewExact ? TEXT("true") : TEXT("false"),
            bAcknowledgedExact ? TEXT("true") : TEXT("false"),
            bOutcomeExact ? TEXT("true") : TEXT("false"),
            OutcomeValue,
            static_cast<unsigned long long>(FinalTick),
            static_cast<unsigned long long>(FinalSnapshotId),
            static_cast<unsigned long long>(FinalScopedDigest));
        return;
    }
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_NETWORK_MATCH_SERVER_SMOKE_PASSED] player=%u outcome=%u finalTick=%llu finalSnapshot=%llu finalDigest=%llu batchAuthority=true ordinaryCombatResolution=true reliableFinalKeyframe=true exactAcknowledgement=true separateProcess=true"),
        NetworkSeat,
        OutcomeValue,
        static_cast<unsigned long long>(FinalTick),
        static_cast<unsigned long long>(FinalSnapshotId),
        static_cast<unsigned long long>(FinalScopedDigest));
    if (FParse::Param(
            FCommandLine::Get(), TEXT("EchoesNetworkMatchSmoke")))
    {
        GetWorldTimerManager().SetTimer(
            NetworkServerExitTimer,
            FTimerDelegate::CreateLambda(
                []()
                {
                    FPlatformMisc::RequestExit(false);
                }),
            0.75f,
            false);
    }
}

void AEchoesPlayerController::ServerConfirmNetworkReconnectSmokeComplete_Implementation(
    uint64 SnapshotId,
    uint64 ReportedLastAcceptedSequence,
    uint64 ReportedLastAcceptedBatchId)
{
    // Meaningful only to an authority launched in reconnect-smoke mode;
    // ordinary servers ignore the RPC entirely.
    if (!FParse::Param(
            FCommandLine::Get(), TEXT("EchoesNetworkReconnectSmoke")))
    {
        return;
    }
    const UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    const echoes::sim::Simulation* Simulation =
        Bridge != nullptr ? Bridge->GetSimulation() : nullptr;
    const bool bSnapshotExact =
        SnapshotId == LastAcknowledgedNetworkSnapshotId &&
        !PendingNetworkSnapshotDigests.Contains(SnapshotId);
    const bool bSequenceExact =
        NetworkCommandContext.hasAcceptedSequence &&
        NetworkCommandContext.lastAcceptedSequence ==
            ReportedLastAcceptedSequence;
    const bool bBatchExact =
        LastAcceptedNetworkBatchId == ReportedLastAcceptedBatchId &&
        ReportedLastAcceptedBatchId == 2;
    const bool bTickAdvanced =
        Simulation != nullptr &&
        Simulation->CurrentTick() > NetworkResumeDisconnectTick;
    if (!bNetworkResumePending || !bNetworkResumeMatchWasStarted ||
        !bSnapshotExact || !bSequenceExact || !bBatchExact || !bTickAdvanced)
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NETWORK_RECONNECT_SERVER_FAILED] resumePending=%s matchWasStarted=%s snapshotExact=%s sequenceExact=%s batchExact=%s tickAdvanced=%s snapshot=%llu acknowledged=%llu sequence=%llu batch=%llu"),
            bNetworkResumePending ? TEXT("true") : TEXT("false"),
            bNetworkResumeMatchWasStarted ? TEXT("true") : TEXT("false"),
            bSnapshotExact ? TEXT("true") : TEXT("false"),
            bSequenceExact ? TEXT("true") : TEXT("false"),
            bBatchExact ? TEXT("true") : TEXT("false"),
            bTickAdvanced ? TEXT("true") : TEXT("false"),
            static_cast<unsigned long long>(SnapshotId),
            static_cast<unsigned long long>(LastAcknowledgedNetworkSnapshotId),
            static_cast<unsigned long long>(ReportedLastAcceptedSequence),
            static_cast<unsigned long long>(ReportedLastAcceptedBatchId));
        return;
    }
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_NETWORK_RECONNECT_SERVER_PASSED] player=%u disconnectTick=%llu authorityTick=%llu snapshot=%llu lastAcceptedSequence=%llu batch=%llu seatReservationConsumed=true credentialMatched=true credentialRotated=true aiControl=false fullKeyframeResync=true commandExecuted=true separateProcess=true"),
        NetworkSeat,
        static_cast<unsigned long long>(NetworkResumeDisconnectTick),
        static_cast<unsigned long long>(Simulation->CurrentTick()),
        static_cast<unsigned long long>(SnapshotId),
        static_cast<unsigned long long>(ReportedLastAcceptedSequence),
        static_cast<unsigned long long>(ReportedLastAcceptedBatchId));
    if (FParse::Param(
            FCommandLine::Get(), TEXT("EchoesNetworkReconnectSmoke")))
    {
        GetWorldTimerManager().SetTimer(
            NetworkServerExitTimer,
            FTimerDelegate::CreateLambda(
                []()
                {
                    FPlatformMisc::RequestExit(false);
                }),
            0.75f,
            false);
    }
}

void AEchoesPlayerController::FinishNetworkClientSmoke()
{
    FPlatformMisc::RequestExit(false);
}

void AEchoesPlayerController::NotifyRuntimeReady()
{
    bRuntimeStateKnown = true;
    SetStatusMessage(
        FString::Printf(
            TEXT("Runtime prototype ready. Select owned %s units, then right-click a destination or target."),
            *GetLocalFactionLabel()),
        7.0f);
}

void AEchoesPlayerController::StartPointerCombatGuardReview()
{
#if !UE_BUILD_SHIPPING
    if (bPointerCombatGuardReviewActive)
    {
        return;
    }
    FEchoesPointerCombatGuardReview ReviewConfiguration;
    FString RequestedVariant;
    if (!FEchoesPointerCombatGuardReview::TryFromCommandLine(
            ReviewConfiguration,
            RequestedVariant))
    {
        SetStatusMessage(
            FString::Printf(
                TEXT("CONTROLLED POINTER REVIEW FAILED — unsupported variant %s."),
                *RequestedVariant),
            3600.0f);
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_POINTER_COMBAT_GUARD_REVIEW_FAILED] stage=0 reason=INVALID_VARIANT requested=%s controlledNonshipping=true"),
            *RequestedVariant);
        return;
    }
    PointerReviewVariant = ReviewConfiguration.Variant;
    PointerReviewHudScale = ReviewConfiguration.HudScale;
    PointerReviewExpectedViewport = ReviewConfiguration.ExpectedViewport;
    if (UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get())
    {
        Settings->SetHudScale(PointerReviewHudScale);
    }
    ClearSelection();
    bKeyboardTargetingEnabled = false;
    PointerReviewDefenderId = 0;
    PointerReviewProtectedId = 0;
    PointerReviewHostileId = 0;
    PointerReviewInitialHostileHitPoints = 0;
    PointerReviewStage = 0;
    PointerReviewStageElapsedSeconds = 0.0f;
    PointerReviewTotalElapsedSeconds = 0.0f;
    bPointerCombatGuardReviewActive = true;
    SetStatusMessage(
        TEXT("CONTROLLED REVIEW — preparing exact-coordinate pointer selection, Guard, and direct attack."),
        3600.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_POINTER_COMBAT_GUARD_REVIEW_STARTED] variant=%s hudScale=%.2f expectedViewport=(%d,%d) exactScreenCoordinates=true controllerBindings=true authoritativeCommands=true nonOcclusionRequired=true controlledNonshipping=true"),
        *PointerReviewVariant,
        PointerReviewHudScale,
        PointerReviewExpectedViewport.X,
        PointerReviewExpectedViewport.Y);
#endif
}

FString AEchoesPlayerController::GetLocalFactionLabel() const
{
    if (GetNetMode() == NM_Client)
    {
        if (const echoes::sim::net::ScopedViewKeyframe* NetworkView =
                GetNetworkScopedView())
        {
            return FactionDisplayName(NetworkView->faction);
        }
    }
    if (IsSkirmishSetupVisible() ||
        IsSkirmishDeploymentSummaryVisible())
    {
        return FEchoesSkirmishSetupModel::FactionDisplayName(
            PendingSkirmishSetup.LocalFaction);
    }
    const UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    return FactionDisplayName(
        Bridge != nullptr
            ? Bridge->GetLocalFaction()
            : echoes::sim::Faction::MeridianCompact);
}

FString AEchoesPlayerController::GetOpponentFactionLabel() const
{
    if (GetNetMode() == NM_Client)
    {
        if (const echoes::sim::net::ScopedViewKeyframe* NetworkView =
                GetNetworkScopedView())
        {
            return FactionDisplayName(
                NetworkView->faction ==
                        echoes::sim::Faction::KharuunAssemblies
                    ? echoes::sim::Faction::MeridianCompact
                    : echoes::sim::Faction::KharuunAssemblies);
        }
    }
    if (IsSkirmishSetupVisible() ||
        IsSkirmishDeploymentSummaryVisible())
    {
        return FEchoesSkirmishSetupModel::FactionDisplayName(
            PendingSkirmishSetup.OpponentFaction);
    }
    const UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    return FactionDisplayName(
        Bridge != nullptr
            ? Bridge->GetOpponentFaction()
            : echoes::sim::Faction::KharuunAssemblies);
}

bool AEchoesPlayerController::DidPresentedLocalPlayerWin() const
{
    const uint8 LocalSeat =
        GetNetMode() == NM_Client && NetworkSeat < echoes::sim::kMaximumPlayers
            ? NetworkSeat
            : UEchoesSimulationSubsystem::LocalPlayerId;
    return OutcomeBelongsToSeat(PresentedMatchOutcome, LocalSeat);
}

bool AEchoesPlayerController::IsSkirmishSetupVisible() const
{
    const UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    return bTitleScreenVisible && Bridge != nullptr &&
        Bridge->GetOperationMode() == EEchoesOperationMode::Skirmish &&
        !Bridge->IsNetworkHumanOpponentEnabled();
}

bool AEchoesPlayerController::IsSkirmishDeploymentSummaryVisible() const
{
    const UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    return bMissionBriefingVisible && Bridge != nullptr &&
        Bridge->GetOperationMode() == EEchoesOperationMode::Skirmish &&
        !Bridge->IsNetworkHumanOpponentEnabled();
}

bool AEchoesPlayerController::CanReturnCompletedSkirmishToOperations() const
{
    const UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    return bMatchResultVisible && !bCampaignResult && Bridge != nullptr &&
        Bridge->GetOperationMode() == EEchoesOperationMode::Skirmish &&
        !Bridge->IsNetworkHumanOpponentEnabled();
}

bool AEchoesPlayerController::CanLeaveNetworkMatchToOnlineMenu() const
{
    return IsOnlineMatchResult() &&
        (GetNetMode() == NM_Client || bNetworkResultExitEnabled);
}

bool AEchoesPlayerController::IsOnlineMatchResult() const
{
    if (!bMatchResultVisible || bCampaignResult)
    {
        return false;
    }
    const UEchoesGameInstance* EchoesGameInstance =
        GetGameInstance<UEchoesGameInstance>();
    const UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    return GetNetMode() != NM_Standalone ||
        (EchoesGameInstance != nullptr &&
         EchoesGameInstance->IsPlayerInitiatedOnlineSession()) ||
        (Bridge != nullptr && Bridge->IsNetworkHumanOpponentEnabled());
}

bool AEchoesPlayerController::IsActiveOnlineNetworkMatch() const
{
    if (!bNetworkMatchStarted || bMatchResultVisible)
    {
        return false;
    }
    const UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    return GetNetMode() != NM_Standalone ||
        (Bridge != nullptr && Bridge->IsNetworkHumanOpponentEnabled());
}

bool AEchoesPlayerController::SetPendingSkirmishSetup(
    const FEchoesSkirmishSetup& Setup,
    FString& OutFeedback)
{
    if (!IsSkirmishSetupVisible())
    {
        OutFeedback = TEXT("[SKIRMISH_SETUP_TITLE_REQUIRED] Open the skirmish setup from Operations.");
        return false;
    }
    if (!FEchoesSkirmishSetupModel::Validate(Setup, OutFeedback))
    {
        return false;
    }
    PendingSkirmishSetup = Setup;
    OutFeedback = TEXT("SKIRMISH SETUP UPDATED — no active match state was changed.");
    return true;
}

void AEchoesPlayerController::FocusPreviousSkirmishSetting()
{
    if (UEchoesGameInstance* EchoesGameInstance = GetEchoesGameInstance();
        EchoesGameInstance != nullptr &&
        EchoesGameInstance->GetOnlineState() ==
            EEchoesOnlineFrontDoorState::JoinSetup)
    {
        EchoesGameInstance->FocusPreviousOnlineAction();
        return;
    }
    if (!IsSkirmishSetupVisible())
    {
        return;
    }
    SkirmishSetupFocusRow = (SkirmishSetupFocusRow + 4) % 5;
    SetStatusMessage(
        FString::Printf(
            TEXT("SKIRMISH SETUP — row %d of 5 selected; Left/Right changes the value."),
            SkirmishSetupFocusRow + 1),
        3600.0f);
}

void AEchoesPlayerController::FocusNextSkirmishSetting()
{
    if (UEchoesGameInstance* EchoesGameInstance = GetEchoesGameInstance();
        EchoesGameInstance != nullptr &&
        EchoesGameInstance->GetOnlineState() ==
            EEchoesOnlineFrontDoorState::JoinSetup)
    {
        EchoesGameInstance->FocusNextOnlineAction();
        return;
    }
    if (!IsSkirmishSetupVisible())
    {
        return;
    }
    SkirmishSetupFocusRow = (SkirmishSetupFocusRow + 1) % 5;
    SetStatusMessage(
        FString::Printf(
            TEXT("SKIRMISH SETUP — row %d of 5 selected; Left/Right changes the value."),
            SkirmishSetupFocusRow + 1),
        3600.0f);
}

void AEchoesPlayerController::DecreaseSkirmishSetting()
{
    if (!IsSkirmishSetupVisible())
    {
        return;
    }
    switch (SkirmishSetupFocusRow)
    {
        case 0:
            PendingSkirmishSetup =
                FEchoesSkirmishSetupModel::WithNextFaction(
                    PendingSkirmishSetup, true, -1);
            break;
        case 1:
            PendingSkirmishSetup =
                FEchoesSkirmishSetupModel::WithNextFaction(
                    PendingSkirmishSetup, false, -1);
            break;
        case 2:
            PendingSkirmishSetup =
                FEchoesSkirmishSetupModel::WithNextMap(
                    PendingSkirmishSetup, -1);
            break;
        case 3:
            PendingSkirmishSetup =
                FEchoesSkirmishSetupModel::WithNextAi(
                    PendingSkirmishSetup, -1);
            break;
        case 4:
            PendingSkirmishSetup =
                FEchoesSkirmishSetupModel::WithNextResources(
                    PendingSkirmishSetup, -1);
            break;
        default:
            SkirmishSetupFocusRow = 0;
            break;
    }
    SetStatusMessage(
        TEXT("SKIRMISH SETUP UPDATED — active simulation unchanged; Enter reviews deployment."),
        3600.0f);
}

void AEchoesPlayerController::IncreaseSkirmishSetting()
{
    if (!IsSkirmishSetupVisible())
    {
        return;
    }
    switch (SkirmishSetupFocusRow)
    {
        case 0:
            PendingSkirmishSetup =
                FEchoesSkirmishSetupModel::WithNextFaction(
                    PendingSkirmishSetup, true, 1);
            break;
        case 1:
            PendingSkirmishSetup =
                FEchoesSkirmishSetupModel::WithNextFaction(
                    PendingSkirmishSetup, false, 1);
            break;
        case 2:
            PendingSkirmishSetup =
                FEchoesSkirmishSetupModel::WithNextMap(
                    PendingSkirmishSetup, 1);
            break;
        case 3:
            PendingSkirmishSetup =
                FEchoesSkirmishSetupModel::WithNextAi(
                    PendingSkirmishSetup, 1);
            break;
        case 4:
            PendingSkirmishSetup =
                FEchoesSkirmishSetupModel::WithNextResources(
                    PendingSkirmishSetup, 1);
            break;
        default:
            SkirmishSetupFocusRow = 0;
            break;
    }
    SetStatusMessage(
        TEXT("SKIRMISH SETUP UPDATED — active simulation unchanged; Enter reviews deployment."),
        3600.0f);
}

void AEchoesPlayerController::ReturnToSkirmishSetup()
{
    if (!IsSkirmishDeploymentSummaryVisible())
    {
        return;
    }
    bMissionBriefingVisible = false;
    bTitleScreenVisible = true;
    SetIgnoreMoveInput(true);
    SetIgnoreLookInput(true);
    SetStatusMessage(
        TEXT("SKIRMISH SETUP — deployment review closed; pending choices retained and active simulation unchanged."),
        3600.0f);
}

void AEchoesPlayerController::PresentTitleAudio()
{
    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return;
    }
    if (UEchoesMusicSubsystem* Music =
            World->GetSubsystem<UEchoesMusicSubsystem>())
    {
        Music->SetThreatLayers(false, false);
        Music->ClearThreatContext();
        Music->SetMusicContext(EEchoesMusicContext::Title);
    }
    if (UEchoesAmbienceSubsystem* Ambience =
            World->GetSubsystem<UEchoesAmbienceSubsystem>())
    {
        Ambience->SetWellProximity(false);
        Ambience->SetAmbienceBed(EEchoesAmbienceBed::None);
    }
    if (UEchoesNarrativeSubsystem* Narrative =
            GetGameInstance() != nullptr
                ? GetGameInstance()->GetSubsystem<UEchoesNarrativeSubsystem>()
                : nullptr)
    {
        Narrative->ClearSubtitleQueue();
    }
}

void AEchoesPlayerController::PresentDeploymentAudio()
{
    UWorld* World = GetWorld();
    UEchoesSimulationSubsystem* Bridge =
        World != nullptr
            ? World->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (World == nullptr || Bridge == nullptr)
    {
        return;
    }
    UEchoesMusicSubsystem* Music =
        World->GetSubsystem<UEchoesMusicSubsystem>();
    UEchoesAmbienceSubsystem* Ambience =
        World->GetSubsystem<UEchoesAmbienceSubsystem>();

    const EEchoesOperationMode Operation = Bridge->GetOperationMode();
    EEchoesAmbienceBed Bed = EEchoesAmbienceBed::GlassScar;
    if (Operation == EEchoesOperationMode::Skirmish)
    {
        const FEchoesSkirmishSetup Setup = Bridge->GetActiveSkirmishSetup();
        if (Setup.MapPreset == EEchoesSkirmishMapPreset::CrownfallBasin)
        {
            Bed = EEchoesAmbienceBed::Crownfall;
        }
        if (Music != nullptr)
        {
            Music->SetThreatContext(Setup.LocalFaction, Setup.OpponentFaction);
            Music->SetMusicContext(
                EEchoesMusicContext::FactionTheme,
                Setup.LocalFaction);
        }
    }
    else
    {
        // Mission index 1..15 in declaration order; acts break 5/10/15.
        const int32 MissionIndex =
            static_cast<int32>(Operation) -
            static_cast<int32>(EEchoesOperationMode::CampaignPrologue) + 1;
        const int32 ActIndex =
            MissionIndex <= 5 ? 1 : MissionIndex <= 10 ? 2 : 3;
        // Sites the Bible pins to a bed: the ark-city grid missions, Lume
        // Reach, and the Act III Crownfall approach. The remaining missions
        // play on Soryn's open vitrified terrain and use the Glass Scar wind.
        if (Operation == EEchoesOperationMode::CampaignCityReserve ||
            Operation == EEchoesOperationMode::CampaignReserveAuthority)
        {
            Bed = EEchoesAmbienceBed::ArkCity;
        }
        else if (Operation == EEchoesOperationMode::CampaignChoirAtLumeReach)
        {
            Bed = EEchoesAmbienceBed::LumeReach;
        }
        else if (MissionIndex >= 11)
        {
            Bed = EEchoesAmbienceBed::Crownfall;
        }
        if (Music != nullptr)
        {
            Music->ClearThreatContext();
            Music->SetMusicContext(
                EEchoesMusicContext::ActBed,
                echoes::sim::Faction::MeridianCompact,
                ActIndex);
        }
    }
    if (Ambience != nullptr)
    {
        Ambience->SetAmbienceBed(Bed);
    }
}

void AEchoesPlayerController::PresentResultAudio(bool bSuccess)
{
    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return;
    }
    if (UEchoesMusicSubsystem* Music =
            World->GetSubsystem<UEchoesMusicSubsystem>())
    {
        Music->SetThreatLayers(false, false);
        Music->PlayStinger(
            bSuccess ? EEchoesMusicStinger::Victory
                     : EEchoesMusicStinger::Defeat);
    }
    if (UEchoesNarrativeSubsystem* Narrative =
            GetGameInstance() != nullptr
                ? GetGameInstance()->GetSubsystem<UEchoesNarrativeSubsystem>()
                : nullptr)
    {
        const UEchoesSimulationSubsystem* Bridge =
            World->GetSubsystem<UEchoesSimulationSubsystem>();
        if (Bridge != nullptr)
        {
            Narrative->ClearSubtitleQueue();
            if (bSuccess)
            {
                Narrative->EnqueueSignal(
                    Bridge->GetOperationMode(),
                    TEXT("phase_entered:Complete"),
                    World->GetRealTimeSeconds());
            }
            else
            {
                Narrative->EnqueueFailureLine(
                    Bridge->GetOperationMode(),
                    Bridge->GetMissionFailureReasonCode(),
                    World->GetRealTimeSeconds());
            }
        }
    }
}

void AEchoesPlayerController::PresentEndingAudio(
    EEchoesFinalResolution RecordedResolution,
    bool bSuccess)
{
    if (!bSuccess || RecordedResolution == EEchoesFinalResolution::None)
    {
        PresentResultAudio(bSuccess);
        return;
    }
    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return;
    }
    if (UEchoesNarrativeSubsystem* Narrative =
            GetGameInstance() != nullptr
                ? GetGameInstance()->GetSubsystem<UEchoesNarrativeSubsystem>()
                : nullptr)
    {
        Narrative->ClearSubtitleQueue();
        Narrative->EnqueueSignal(
            EEchoesOperationMode::CampaignTheBrokenSun,
            TEXT("phase_entered:Complete"),
            World->GetRealTimeSeconds());
    }
    if (UEchoesMusicSubsystem* Music =
            World->GetSubsystem<UEchoesMusicSubsystem>())
    {
        Music->SetThreatLayers(false, false);
        EEchoesMusicStinger Stinger = EEchoesMusicStinger::EndingRestoration;
        switch (RecordedResolution)
        {
            case EEchoesFinalResolution::Restoration:
                break;
            case EEchoesFinalResolution::ControlledStabilization:
                Stinger = EEchoesMusicStinger::EndingStabilization;
                break;
            case EEchoesFinalResolution::Extinguishment:
                Stinger = EEchoesMusicStinger::EndingExtinguishment;
                break;
            case EEchoesFinalResolution::OpenEvolution:
                Stinger = EEchoesMusicStinger::EndingOpenEvolution;
                break;
            case EEchoesFinalResolution::None:
            default:
                break;
        }
        Music->PlayStinger(Stinger);
    }
}

void AEchoesPlayerController::PresentTitleScreen()
{
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[TITLE_SIM_NOT_READY] The operation is unavailable."));
        return;
    }
    if (IsSkirmishDeploymentSummaryVisible())
    {
        PendingSkirmishSetup = Bridge->GetActiveSkirmishSetup();
        SkirmishSetupFocusRow = 0;
    }
    SynchronizeBoundCampaignProtocol();
    ClearSelection();
    bSelectionButtonDown = false;
    bControlGroupAssignmentArmed = false;
    bTitleScreenVisible = true;
    bMissionBriefingVisible = false;
    bPauseMenuVisible = false;
    bTechnologyPanelVisible = false;
    bMatchResultVisible = false;
    bNewCampaignConfirmationArmed = false;
    NewCampaignConfirmationExpiresAt = 0.0;
    bCampaignRestoreConfirmationArmed = false;
    CampaignRestoreConfirmationExpiresAt = 0.0;
    bReturnToOperationsConfirmationArmed = false;
    ReturnToOperationsConfirmationExpiresAt = 0.0;
    bCampaignResult = false;
    bCampaignSuccess = false;
    CampaignConsequence = echoes::sim::FutureWellChoice::Dormant;
    RecordedCampaignConsequence = echoes::sim::FutureWellChoice::Dormant;
    CampaignFinalResolution = EEchoesFinalResolution::None;
    RecordedCampaignFinalResolution = EEchoesFinalResolution::None;
    CampaignCommitStatus = EEchoesCampaignCommitStatus::NotApplicable;
    PresentedMatchOutcome = echoes::sim::MatchOutcome::Ongoing;
    PresentedCampaignOperation = EEchoesOperationMode::Skirmish;
    Bridge->SetScenarioPaused(true);
    PresentTitleAudio();
    SetIgnoreMoveInput(true);
    SetIgnoreLookInput(true);
    SetStatusMessage(
        FString::Printf(
            TEXT("ECHOES OF THE BROKEN SUN — F9 changes operation; C continues the campaign; %sEnter opens the selected brief."),
            Bridge->GetOperationMode() == EEchoesOperationMode::Skirmish
                ? TEXT("arrows configure skirmish; ")
            : Bridge->GetOperationMode() ==
                    EEchoesOperationMode::CampaignPrologue
                ? TEXT("Mara Vey deployed; ")
            : Bridge->GetOperationMode() ==
                    EEchoesOperationMode::CampaignSevenAccounts
                ? TEXT("Oruun deployed; ")
            : Bridge->GetOperationMode() ==
                    EEchoesOperationMode::CampaignCityReserve
                ? TEXT("Mara Vey deployed; ")
            : Bridge->GetOperationMode() ==
                    EEchoesOperationMode::CampaignUnburiedRoad
                ? TEXT("Oruun deployed; ")
            : Bridge->GetOperationMode() ==
                    EEchoesOperationMode::CampaignTermsOfContinuance
                ? TEXT("Meridian treaty proxies deployed; ")
            : Bridge->GetOperationMode() ==
                    EEchoesOperationMode::CampaignNamesWithoutBirths
                ? TEXT("Talar and two civilian proxies deployed; ")
            : Bridge->GetOperationMode() ==
                    EEchoesOperationMode::CampaignShapeOfSilence
                ? TEXT("Oruun and two memory witnesses deployed; ")
            : Bridge->GetOperationMode() ==
                    EEchoesOperationMode::CampaignShapeBesideUs
                ? TEXT("Talar and two state witnesses deployed; ")
            : Bridge->GetOperationMode() ==
                    EEchoesOperationMode::CampaignReserveAuthority
                ? TEXT("Mara and three reserve districts deployed; ")
            : Bridge->GetOperationMode() ==
                    EEchoesOperationMode::CampaignChoirAtLumeReach
                ? TEXT("Oruun's Kharuun listening force deployed; Mara is liaison-only; ")
            : Bridge->GetOperationMode() ==
                    EEchoesOperationMode::CampaignNoNeutralLedger
                ? TEXT("Oruun's Kharuun ledger force deployed; Meridian and Choir interfaces remain public and non-commandable; ")
            : Bridge->GetOperationMode() ==
                    EEchoesOperationMode::CampaignFutureThatWon
                ? TEXT("Oruun and an independent verifier deployed under Kharuun authority; Rhyse's restoration demonstrator remains public and non-commandable; ")
            : Bridge->GetOperationMode() ==
                    EEchoesOperationMode::CampaignAssemblyOfTheMissing
                ? TEXT("Oruun and an independent verifier deployed under Kharuun authority; the Meridian, Kharuun, and Crownfall public record interfaces remain neutral and non-commandable; ")
            : Bridge->GetOperationMode() ==
                    EEchoesOperationMode::CampaignSeveralVoicesOneCommand
                ? TEXT("Neme and the local Hollow Choir deployed under player command; identity resolution and the crisis hold use visible deterministic timers; ")
                : TEXT("The final Hollow Choir command force is deployed; Mara, Oruun, and Talar are protected neutral witnesses; four explicit resolutions are earned across campaign routes; ")),
        3600.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_TITLE_READY] operation=%s operationChoice=true campaignContinue=true keyboardStart=true factionChoice=%s"),
        Bridge->GetOperationMode() == EEchoesOperationMode::CampaignPrologue
            ? TEXT("WhatTheLedgerKeeps")
        : Bridge->GetOperationMode() ==
                EEchoesOperationMode::CampaignSevenAccounts
            ? TEXT("SevenAccountsOfRain")
        : Bridge->GetOperationMode() ==
                EEchoesOperationMode::CampaignCityReserve
            ? TEXT("ACityOnReserve")
        : Bridge->GetOperationMode() ==
                EEchoesOperationMode::CampaignUnburiedRoad
            ? TEXT("TheUnburiedRoad")
        : Bridge->GetOperationMode() ==
                EEchoesOperationMode::CampaignTermsOfContinuance
            ? TEXT("TermsOfContinuance")
        : Bridge->GetOperationMode() ==
                EEchoesOperationMode::CampaignNamesWithoutBirths
            ? TEXT("NamesWithoutBirths")
        : Bridge->GetOperationMode() ==
                EEchoesOperationMode::CampaignShapeOfSilence
            ? TEXT("TheShapeOfSilence")
        : Bridge->GetOperationMode() ==
                EEchoesOperationMode::CampaignShapeBesideUs
            ? TEXT("TheShapeBesideUs")
        : Bridge->GetOperationMode() ==
                EEchoesOperationMode::CampaignReserveAuthority
            ? TEXT("ReserveAuthority")
        : Bridge->GetOperationMode() ==
                EEchoesOperationMode::CampaignChoirAtLumeReach
            ? TEXT("ChoirAtLumeReach")
        : Bridge->GetOperationMode() ==
                EEchoesOperationMode::CampaignNoNeutralLedger
            ? TEXT("NoNeutralLedger")
        : Bridge->GetOperationMode() ==
                EEchoesOperationMode::CampaignFutureThatWon
            ? TEXT("TheFutureThatWon")
        : Bridge->GetOperationMode() ==
                EEchoesOperationMode::CampaignAssemblyOfTheMissing
            ? TEXT("AssemblyOfTheMissing")
        : Bridge->GetOperationMode() ==
                EEchoesOperationMode::CampaignSeveralVoicesOneCommand
            ? TEXT("SeveralVoicesOneCommand")
        : Bridge->GetOperationMode() ==
                EEchoesOperationMode::CampaignTheBrokenSun
            ? TEXT("TheBrokenSun")
            : TEXT("GlassScar"),
        Bridge->GetOperationMode() == EEchoesOperationMode::Skirmish
            ? TEXT("true")
            : TEXT("false"));

#if !UE_BUILD_SHIPPING
    // Rendered-review fixture: open the selected operation brief without
    // ordinary input so headless captures can photograph the briefing panel.
    if (FParse::Param(FCommandLine::Get(), TEXT("EchoesArtReviewOpenBrief")))
    {
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_ART_REVIEW_OPEN_BRIEF] editorOnly=true"));
        ConfirmTitleScreen();
        if (FParse::Param(FCommandLine::Get(), TEXT("EchoesArtReviewDeploy")))
        {
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_ART_REVIEW_DEPLOY] editorOnly=true"));
            ConfirmMissionBriefing();
        }
    }
#endif
}

void AEchoesPlayerController::ConfirmTitleScreen()
{
    if (!bTitleScreenVisible)
    {
        return;
    }
    bNewCampaignConfirmationArmed = false;
    NewCampaignConfirmationExpiresAt = 0.0;
    bCampaignRestoreConfirmationArmed = false;
    CampaignRestoreConfirmationExpiresAt = 0.0;
    bTitleScreenVisible = false;
    UE_LOG(LogEchoes, Display, TEXT("[ECHOES_TITLE_CONFIRMED] next=OperationsBrief"));
    PresentMissionBriefing();
}

void AEchoesPlayerController::PresentMissionBriefing()
{
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[BRIEFING_SIM_NOT_READY] Mission briefing is unavailable."));
        return;
    }
    SynchronizeBoundCampaignProtocol();
    ClearSelection();
    bSelectionButtonDown = false;
    bControlGroupAssignmentArmed = false;
    bTitleScreenVisible = false;
    bPauseMenuVisible = false;
    bTechnologyPanelVisible = false;
    bMatchResultVisible = false;
    bCampaignResult = false;
    RecordedCampaignConsequence = echoes::sim::FutureWellChoice::Dormant;
    CampaignFinalResolution = EEchoesFinalResolution::None;
    RecordedCampaignFinalResolution = EEchoesFinalResolution::None;
    CampaignCommitStatus = EEchoesCampaignCommitStatus::NotApplicable;
    PresentedMatchOutcome = echoes::sim::MatchOutcome::Ongoing;
    bMissionBriefingVisible = true;
    Bridge->SetScenarioPaused(true);
    SetIgnoreMoveInput(true);
    SetIgnoreLookInput(true);
    const bool bPrologue =
        Bridge->GetOperationMode() == EEchoesOperationMode::CampaignPrologue;
    const bool bSevenAccounts =
        Bridge->GetOperationMode() ==
        EEchoesOperationMode::CampaignSevenAccounts;
    const bool bCityReserve =
        Bridge->GetOperationMode() ==
        EEchoesOperationMode::CampaignCityReserve;
    const bool bUnburiedRoad =
        Bridge->GetOperationMode() ==
        EEchoesOperationMode::CampaignUnburiedRoad;
    const bool bTermsOfContinuance =
        Bridge->GetOperationMode() ==
        EEchoesOperationMode::CampaignTermsOfContinuance;
    const bool bNamesWithoutBirths =
        Bridge->GetOperationMode() ==
        EEchoesOperationMode::CampaignNamesWithoutBirths;
    const bool bShapeOfSilence =
        Bridge->GetOperationMode() ==
        EEchoesOperationMode::CampaignShapeOfSilence;
    const bool bShapeBesideUs =
        Bridge->GetOperationMode() ==
        EEchoesOperationMode::CampaignShapeBesideUs;
    const bool bReserveAuthority =
        Bridge->GetOperationMode() ==
        EEchoesOperationMode::CampaignReserveAuthority;
    const bool bChoirAtLumeReach =
        Bridge->GetOperationMode() ==
        EEchoesOperationMode::CampaignChoirAtLumeReach;
    const bool bNoNeutralLedger =
        Bridge->GetOperationMode() ==
        EEchoesOperationMode::CampaignNoNeutralLedger;
    const bool bFutureThatWon =
        Bridge->GetOperationMode() ==
        EEchoesOperationMode::CampaignFutureThatWon;
    const bool bAssemblyOfTheMissing =
        Bridge->GetOperationMode() ==
        EEchoesOperationMode::CampaignAssemblyOfTheMissing;
    const bool bSeveralVoicesOneCommand =
        Bridge->GetOperationMode() ==
        EEchoesOperationMode::CampaignSeveralVoicesOneCommand;
    const bool bBrokenSun =
        Bridge->GetOperationMode() ==
        EEchoesOperationMode::CampaignTheBrokenSun;
    SetStatusMessage(
        bPrologue
            ? TEXT("WHAT THE LEDGER KEEPS — recover the archive, decide the Well, and withdraw. Enter deploys Mara Vey.")
        : bSevenAccounts
            ? TEXT("SEVEN ACCOUNTS OF RAIN — migrate the Waystone, then bring Oruun to the inherited account. Enter deploys.")
        : bCityReserve
            ? TEXT("A CITY ON RESERVE — reconnect three ark-city districts in the inherited priority order. Enter deploys Mara Vey.")
        : bUnburiedRoad
            ? TEXT("THE UNBURIED ROAD — root the Waystone, raise a Listening Spine, and recover the missing shard. Enter deploys Oruun.")
        : bTermsOfContinuance
            ? TEXT("TERMS OF CONTINUANCE — synchronize both treaty proxies, hold the fixed window, then extract both witness proxies. Enter deploys Meridian authority.")
        : bNamesWithoutBirths
            ? TEXT("NAMES WITHOUT BIRTHS — Talar must locate the inherited census trace, a worker must power its archive, both civilian proxies must reach shelter, and Talar must extract the evidence. Enter deploys Meridian authority.")
        : bShapeOfSilence
            ? TEXT("THE SHAPE OF SILENCE — root the Waystone, raise a Listening Spine, position both memory witnesses, then bring Oruun to the confluence. Enter deploys Kharuun authority.")
        : bShapeBesideUs
            ? TEXT("THE SHAPE BESIDE US — follow Neme's first echo, raise a relay, traverse both overlapping states, then bring Talar to the convergence. Enter deploys Meridian authority.")
        : bReserveAuthority
            ? TEXT("RESERVE AUTHORITY — secure Mara's authority site, power exactly two failing districts, then bring Mara to the deferred district. Enter deploys Meridian authority.")
        : bChoirAtLumeReach
            ? TEXT("THE CHOIR AT LUME REACH — establish contact with Oruun, root the Waystone at the deferred liability, raise both Listening Spines, commit this operation's Future Well, then reach its branch resolution. Mara remains an off-map liaison; the local Choir is not commandable. Enter deploys Kharuun authority.")
        : bNoNeutralLedger
            ? TEXT("NO NEUTRAL LEDGER — secure the inherited route, integrate the two powered district systems, attest the public Meridian and Kharuun evidence channels, apply the exact recorded Lume protocol, then rally Oruun and the ledger witness. Only Oruun's Kharuun force is commandable. Enter deploys Kharuun authority.")
        : bFutureThatWon
            ? TEXT("THE FUTURE THAT WON — establish two-person public readback, link both recorded district inputs, activate only the recorded Lume protocol, hold the bounded stability window, then observe both district readbacks. Rhyse is represented only by attributable public apparatus. Enter deploys Kharuun authority.")
        : bAssemblyOfTheMissing
            ? TEXT("ASSEMBLY OF THE MISSING — place Oruun and the independent verifier at the separate neutral public record interfaces, link the neutral Crownfall index with a Kharuun Listening Spine, then observe both independent assembly witness sites. This operation records public observations only; it does not assign responsibility or hidden authorship. Enter deploys Kharuun authority.")
        : bSeveralVoicesOneCommand
            ? TEXT("SEVERAL VOICES, ONE COMMAND — research Held Alternatives, resolve one protected voice to Possible while the other remains Manifest, place both voices and Neme at their inherited sites, research Shared Resolution, then raise and hold a Phase Anchor through the visible crisis timer. Enter deploys Hollow Choir authority.")
        : bBrokenSun
            ? TEXT("THE BROKEN SUN — secure the inherited Crownfall approach, assemble Mara, Oruun, Neme, and Talar's witnessed accord, explicitly arm and confirm one earned final resolution, then raise its distinct conduit and hold the exact contract. Enter deploys Hollow Choir authority.")
            : *FString::Printf(
                  TEXT("SKIRMISH DEPLOYMENT REVIEW — %s against %s on %s; AI %s; %s. Enter applies the complete setup and deploys; Escape returns to setup."),
                  FEchoesSkirmishSetupModel::FactionDisplayName(
                      PendingSkirmishSetup.LocalFaction),
                  FEchoesSkirmishSetupModel::FactionDisplayName(
                      PendingSkirmishSetup.OpponentFaction),
                  FEchoesSkirmishSetupModel::MapDisplayName(
                      PendingSkirmishSetup.MapPreset),
                  FEchoesSkirmishSetupModel::AiDisplayName(
                      PendingSkirmishSetup.AiPersonality),
                  FEchoesSkirmishSetupModel::ResourceDisplayName(
                      PendingSkirmishSetup.ResourceLevel)),
        3600.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_BRIEFING_READY] operation=%s paused=true keyboardStart=true factionChoice=%s"),
        bPrologue ? TEXT("WhatTheLedgerKeeps")
        : bSevenAccounts ? TEXT("SevenAccountsOfRain")
        : bCityReserve ? TEXT("ACityOnReserve")
        : bUnburiedRoad ? TEXT("TheUnburiedRoad")
        : bTermsOfContinuance ? TEXT("TermsOfContinuance")
        : bNamesWithoutBirths ? TEXT("NamesWithoutBirths")
        : bShapeOfSilence ? TEXT("TheShapeOfSilence")
        : bShapeBesideUs ? TEXT("TheShapeBesideUs")
        : bReserveAuthority ? TEXT("ReserveAuthority")
        : bChoirAtLumeReach ? TEXT("ChoirAtLumeReach")
        : bNoNeutralLedger ? TEXT("NoNeutralLedger")
        : bFutureThatWon ? TEXT("TheFutureThatWon")
        : bAssemblyOfTheMissing ? TEXT("AssemblyOfTheMissing")
        : bSeveralVoicesOneCommand ? TEXT("SeveralVoicesOneCommand")
        : bBrokenSun ? TEXT("TheBrokenSun")
        : TEXT("GlassScar"),
        (bPrologue || bSevenAccounts || bCityReserve || bUnburiedRoad ||
         bTermsOfContinuance || bNamesWithoutBirths || bShapeOfSilence ||
         bShapeBesideUs || bReserveAuthority || bChoirAtLumeReach ||
         bNoNeutralLedger || bFutureThatWon || bAssemblyOfTheMissing ||
         bSeveralVoicesOneCommand || bBrokenSun)
            ? TEXT("false")
            : TEXT("true"));
}

void AEchoesPlayerController::ConfirmMissionBriefing()
{
    if (!bMissionBriefingVisible)
    {
        return;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[BRIEFING_SIM_NOT_READY] Deployment could not begin."));
        return;
    }
    if (IsSkirmishDeploymentSummaryVisible())
    {
        FString DeploymentFeedback;
        if (!Bridge->ApplySkirmishSetup(
                PendingSkirmishSetup, DeploymentFeedback))
        {
            SetStatusMessage(DeploymentFeedback, 15.0f);
            UE_LOG(
                LogEchoes,
                Error,
                TEXT("[ECHOES_SKIRMISH_DEPLOYMENT_REJECTED] detail=%s summaryRetained=true"),
                *DeploymentFeedback);
            return;
        }
        PendingSkirmishSetup = Bridge->GetActiveSkirmishSetup();
    }
    SynchronizeBoundCampaignProtocol();
    bMissionBriefingVisible = false;
    if (UEchoesInterfaceAudioSubsystem* InterfaceAudio =
            GetWorld()->GetSubsystem<UEchoesInterfaceAudioSubsystem>())
    {
        InterfaceAudio->PlayInterfaceCue(EEchoesInterfaceCue::BriefAdvance);
    }
    Bridge->SetScenarioPaused(false);
    PresentDeploymentAudio();
    if (UEchoesNarrativeSubsystem* Narrative =
            GetGameInstance() != nullptr
                ? GetGameInstance()->GetSubsystem<UEchoesNarrativeSubsystem>()
                : nullptr)
    {
        Narrative->ClearSubtitleQueue();
        Narrative->EnqueueOperationStart(
            Bridge->GetOperationMode(),
            GetWorld()->GetRealTimeSeconds());
    }
    SetIgnoreMoveInput(false);
    SetIgnoreLookInput(false);
    if (Bridge->GetOperationMode() == EEchoesOperationMode::CampaignPrologue)
    {
        SetStatusMessage(TEXT("DEPLOYED — select Mara Vey's scout carrier and recover the archive at tile 22,18."), 8.0f);
    }
    else if (Bridge->GetOperationMode() ==
             EEchoesOperationMode::CampaignSevenAccounts)
    {
        const FEchoesSevenAccountsRoute Route = Bridge->GetSevenAccountsRoute();
        SetStatusMessage(
            FString::Printf(
                TEXT("DEPLOYED — uproot and re-root the Waystone at %d,%d; then bring Oruun to %d,%d."),
                Route.WaystoneAnchor.x.FloorToInt(),
                Route.WaystoneAnchor.y.FloorToInt(),
                Route.MemoryAccountSite.x.FloorToInt(),
                Route.MemoryAccountSite.y.FloorToInt()),
            10.0f);
    }
    else if (Bridge->GetOperationMode() ==
             EEchoesOperationMode::CampaignCityReserve)
    {
        const FEchoesCityReserveGrid Grid = Bridge->GetCityReserveGrid();
        SetStatusMessage(
            FString::Printf(
                TEXT("DEPLOYED — build Power Links until %s, %s, and %s district posts are powered."),
                FEchoesCityReserveMissionModel::DistrictDisplayName(
                    Grid.Priority),
                FEchoesCityReserveMissionModel::DistrictDisplayName(
                    Grid.Secondary),
                FEchoesCityReserveMissionModel::DistrictDisplayName(
                    Grid.Final)),
            12.0f);
    }
    else if (Bridge->GetOperationMode() ==
             EEchoesOperationMode::CampaignUnburiedRoad)
    {
        const FEchoesUnburiedRoadRoute Route = Bridge->GetUnburiedRoadRoute();
        SetStatusMessage(
            FString::Printf(
                TEXT("DEPLOYED — root the Waystone at %d,%d; build a Listening Spine at %d,%d; bring Oruun to the shard at %d,%d."),
                Route.Roadhead.x.FloorToInt(),
                Route.Roadhead.y.FloorToInt(),
                Route.ListeningSpineSite.x.FloorToInt(),
                Route.ListeningSpineSite.y.FloorToInt(),
                Route.MemoryShardSite.x.FloorToInt(),
                Route.MemoryShardSite.y.FloorToInt()),
            14.0f);
    }
    else if (Bridge->GetOperationMode() ==
             EEchoesOperationMode::CampaignTermsOfContinuance)
    {
        const FEchoesTermsOfContinuancePlan Plan =
            Bridge->GetTermsOfContinuancePlan();
        FString PlayerLinkSites;
        for (const auto& Site : Plan.PlayerPowerLinkSites)
        {
            if (!PlayerLinkSites.IsEmpty())
            {
                PlayerLinkSites += TEXT(", ");
            }
            PlayerLinkSites += FString::Printf(
                TEXT("%d,%d"),
                Site.x.FloorToInt(),
                Site.y.FloorToInt());
        }
        SetStatusMessage(
            FString::Printf(
                TEXT("DEPLOYED — build Power Links at %s; then sync Meridian proxies %d,%d + %d,%d by T%llu; hold to T%llu; extract witnesses at %d,%d."),
                *PlayerLinkSites,
                Plan.MeridianRelaySite.x.FloorToInt(),
                Plan.MeridianRelaySite.y.FloorToInt(),
                Plan.KharuunSpineSite.x.FloorToInt(),
                Plan.KharuunSpineSite.y.FloorToInt(),
                static_cast<unsigned long long>(
                    Plan.ContinuanceWindowStartTick),
                static_cast<unsigned long long>(
                    Plan.ContinuanceWindowEndTick),
                Plan.WitnessExtractionSite.x.FloorToInt(),
                Plan.WitnessExtractionSite.y.FloorToInt()),
            16.0f);
    }
    else if (Bridge->GetOperationMode() ==
             EEchoesOperationMode::CampaignNamesWithoutBirths)
    {
        const FEchoesNamesWithoutBirthsPlan Plan =
            Bridge->GetNamesWithoutBirthsPlan();
        SetStatusMessage(
            FString::Printf(
                TEXT("DEPLOYED — bring Talar to census %d,%d; build its Power Link at %d,%d; shelter both civilians at %d,%d; extract Talar at %d,%d."),
                Plan.CensusSite.x.FloorToInt(),
                Plan.CensusSite.y.FloorToInt(),
                Plan.PowerLinkSite.x.FloorToInt(),
                Plan.PowerLinkSite.y.FloorToInt(),
                Plan.CivilianShelterSite.x.FloorToInt(),
                Plan.CivilianShelterSite.y.FloorToInt(),
                Plan.EvidenceExtractionSite.x.FloorToInt(),
                Plan.EvidenceExtractionSite.y.FloorToInt()),
            16.0f);
    }
    else if (Bridge->GetOperationMode() ==
             EEchoesOperationMode::CampaignShapeOfSilence)
    {
        const FEchoesShapeOfSilencePlan Plan =
            Bridge->GetShapeOfSilencePlan();
        SetStatusMessage(
            FString::Printf(
                TEXT("DEPLOYED — root the Waystone at %d,%d; raise a Listening Spine at %d,%d; place witnesses at %d,%d and %d,%d; bring Oruun to %d,%d."),
                Plan.WaystoneAnchor.x.FloorToInt(),
                Plan.WaystoneAnchor.y.FloorToInt(),
                Plan.ListeningSpineSite.x.FloorToInt(),
                Plan.ListeningSpineSite.y.FloorToInt(),
                Plan.FirstWitnessSite.x.FloorToInt(),
                Plan.FirstWitnessSite.y.FloorToInt(),
                Plan.SecondWitnessSite.x.FloorToInt(),
                Plan.SecondWitnessSite.y.FloorToInt(),
                Plan.ConfluenceSite.x.FloorToInt(),
                Plan.ConfluenceSite.y.FloorToInt()),
            18.0f);
    }
    else if (Bridge->GetOperationMode() ==
             EEchoesOperationMode::CampaignShapeBesideUs)
    {
        const FEchoesShapeBesideUsPlan Plan =
            Bridge->GetShapeBesideUsPlan();
        SetStatusMessage(
            FString::Printf(
                TEXT("DEPLOYED — bring Talar to the first echo at %d,%d; raise a relay at %d,%d; place witnesses at %d,%d and %d,%d; bring Talar to %d,%d."),
                Plan.FirstEchoSite.x.FloorToInt(),
                Plan.FirstEchoSite.y.FloorToInt(),
                Plan.EchoRelaySite.x.FloorToInt(),
                Plan.EchoRelaySite.y.FloorToInt(),
                Plan.FirstStateSite.x.FloorToInt(),
                Plan.FirstStateSite.y.FloorToInt(),
                Plan.SecondStateSite.x.FloorToInt(),
                Plan.SecondStateSite.y.FloorToInt(),
                Plan.ConvergenceSite.x.FloorToInt(),
                Plan.ConvergenceSite.y.FloorToInt()),
            18.0f);
    }
    else if (Bridge->GetOperationMode() ==
             EEchoesOperationMode::CampaignReserveAuthority)
    {
        const FEchoesReserveAuthorityPlan Plan =
            Bridge->GetReserveAuthorityPlan();
        SetStatusMessage(
            FString::Printf(
                TEXT("DEPLOYED — bring Mara to authority %d,%d; use [N] Power Links to power exactly two districts; then bring Mara to the deferred district. %s is the inherited recommendation, not a forced choice."),
                Plan.AuthoritySite.x.FloorToInt(),
                Plan.AuthoritySite.y.FloorToInt(),
                FEchoesCityReserveMissionModel::DistrictDisplayName(
                    Plan.RecommendedFirstDistrict)),
            18.0f);
    }
    else if (Bridge->GetOperationMode() ==
             EEchoesOperationMode::CampaignChoirAtLumeReach)
    {
        const FEchoesChoirAtLumeReachPlan Plan =
            Bridge->GetChoirAtLumeReachPlan();
        SetStatusMessage(
            FString::Printf(
                TEXT("MISSION 10 — CONTACT %d,%d > LIABILITY %d,%d > SPINES %d,%d + %d,%d > WELL %d,%d"),
                Plan.ContactSite.x.FloorToInt(),
                Plan.ContactSite.y.FloorToInt(),
                Plan.LiabilitySite.x.FloorToInt(),
                Plan.LiabilitySite.y.FloorToInt(),
                Plan.FirstAnchorSite.x.FloorToInt(),
                Plan.FirstAnchorSite.y.FloorToInt(),
                Plan.SecondAnchorSite.x.FloorToInt(),
                Plan.SecondAnchorSite.y.FloorToInt(),
                Plan.FutureWellSite.x.FloorToInt(),
                Plan.FutureWellSite.y.FloorToInt()),
            22.0f);
    }
    else if (Bridge->GetOperationMode() ==
             EEchoesOperationMode::CampaignNoNeutralLedger)
    {
        const FEchoesNoNeutralLedgerPlan Plan =
            Bridge->GetNoNeutralLedgerPlan();
        SetStatusMessage(
            FString::Printf(
                TEXT("MISSION 11 — ROUTE %d,%d > KHARUUN LINKS NEAR %s %d,%d + %s %d,%d > EVIDENCE %d,%d + %d,%d > RECORDED WELL %d,%d > RALLY %d,%d"),
                Plan.RouteSite.x.FloorToInt(),
                Plan.RouteSite.y.FloorToInt(),
                FEchoesCityReserveMissionModel::DistrictDisplayName(
                    Plan.FirstContributingDistrict),
                Plan.FirstDistrictSite.x.FloorToInt(),
                Plan.FirstDistrictSite.y.FloorToInt(),
                FEchoesCityReserveMissionModel::DistrictDisplayName(
                    Plan.SecondContributingDistrict),
                Plan.SecondDistrictSite.x.FloorToInt(),
                Plan.SecondDistrictSite.y.FloorToInt(),
                Plan.MeridianEvidenceSite.x.FloorToInt(),
                Plan.MeridianEvidenceSite.y.FloorToInt(),
                Plan.KharuunEvidenceSite.x.FloorToInt(),
                Plan.KharuunEvidenceSite.y.FloorToInt(),
                Plan.FutureWellSite.x.FloorToInt(),
                Plan.FutureWellSite.y.FloorToInt(),
                Plan.RallySite.x.FloorToInt(),
                Plan.RallySite.y.FloorToInt()),
            28.0f);
    }
    else if (Bridge->GetOperationMode() ==
             EEchoesOperationMode::CampaignFutureThatWon)
    {
        const FEchoesFutureThatWonPlan Plan =
            Bridge->GetFutureThatWonPlan();
        SetStatusMessage(
            FString::Printf(
                TEXT("MISSION 12 — READBACK %d,%d + %d,%d > KHARUUN LINKS NEAR %s %d,%d + %s %d,%d > RECORDED %s WELL %d,%d > HOLD %llu TICKS > OBSERVE %d,%d + %d,%d"),
                Plan.KharuunReadbackSite.x.FloorToInt(),
                Plan.KharuunReadbackSite.y.FloorToInt(),
                Plan.MeridianReadbackSite.x.FloorToInt(),
                Plan.MeridianReadbackSite.y.FloorToInt(),
                FEchoesCityReserveMissionModel::DistrictDisplayName(
                    Plan.FirstContributingDistrict),
                Plan.FirstDistrictInputSite.x.FloorToInt(),
                Plan.FirstDistrictInputSite.y.FloorToInt(),
                FEchoesCityReserveMissionModel::DistrictDisplayName(
                    Plan.SecondContributingDistrict),
                Plan.SecondDistrictInputSite.x.FloorToInt(),
                Plan.SecondDistrictInputSite.y.FloorToInt(),
                Plan.ProtocolDisplayName,
                Plan.FutureWellSite.x.FloorToInt(),
                Plan.FutureWellSite.y.FloorToInt(),
                static_cast<unsigned long long>(Plan.StabilityWindowTicks),
                Plan.FirstDistrictInputSite.x.FloorToInt(),
                Plan.FirstDistrictInputSite.y.FloorToInt(),
                Plan.SecondDistrictInputSite.x.FloorToInt(),
                Plan.SecondDistrictInputSite.y.FloorToInt()),
            32.0f);
    }
    else if (Bridge->GetOperationMode() ==
             EEchoesOperationMode::CampaignAssemblyOfTheMissing)
    {
        const FEchoesAssemblyOfTheMissingPlan Plan =
            Bridge->GetAssemblyOfTheMissingPlan();
        SetStatusMessage(
            FString::Printf(
                TEXT("MISSION 13 — PUBLIC READBACK KHARUUN %d,%d + MERIDIAN %d,%d > LISTENING SPINE NEAR CROWNFALL INDEX %d,%d > OBSERVE KHARUUN %d,%d + MERIDIAN %d,%d"),
                Plan.KharuunPublicRecordSite.x.FloorToInt(),
                Plan.KharuunPublicRecordSite.y.FloorToInt(),
                Plan.MeridianPublicRecordSite.x.FloorToInt(),
                Plan.MeridianPublicRecordSite.y.FloorToInt(),
                Plan.CrownfallIndexSite.x.FloorToInt(),
                Plan.CrownfallIndexSite.y.FloorToInt(),
                Plan.KharuunAssemblyWitnessSite.x.FloorToInt(),
                Plan.KharuunAssemblyWitnessSite.y.FloorToInt(),
                Plan.MeridianAssemblyWitnessSite.x.FloorToInt(),
                Plan.MeridianAssemblyWitnessSite.y.FloorToInt()),
            32.0f);
    }
    else if (Bridge->GetOperationMode() ==
             EEchoesOperationMode::CampaignSeveralVoicesOneCommand)
    {
        SetStatusMessage(
            TEXT("M14 // F2 HELD > SOLDIER SHIFT+F4 TO P // HEAVY TO M // NEME TO N // F2 SHARED > WORKER [M] AT A // HOLD 8s"),
            36.0f);
    }
    else if (Bridge->GetOperationMode() ==
             EEchoesOperationMode::CampaignTheBrokenSun)
    {
        SetStatusMessage(
            TEXT("M15 // WORKER [M] APPROACH > F2 HELD > SOLDIER SHIFT+F4 POSSIBLE AT MARA > HEAVY MANIFEST AT ORUUN > NEME AT ACCORD > F2 SHARED > SHIFT+1/2/3/4 TWICE > WORKER [M] CONDUIT > HOLD"),
            48.0f);
    }
    else
    {
        SetStatusMessage(
            FString::Printf(
                  TEXT("DEPLOYED — %s against %s on %s; AI %s; secure the Future Well or destroy the opposing Command Core."),
                  FEchoesSkirmishSetupModel::FactionDisplayName(
                      PendingSkirmishSetup.LocalFaction),
                  FEchoesSkirmishSetupModel::FactionDisplayName(
                      PendingSkirmishSetup.OpponentFaction),
                  FEchoesSkirmishSetupModel::MapDisplayName(
                      PendingSkirmishSetup.MapPreset),
                  FEchoesSkirmishSetupModel::AiDisplayName(
                      PendingSkirmishSetup.AiPersonality)),
            8.0f);
    }
    UE_LOG(LogEchoes, Display, TEXT("[ECHOES_BRIEFING_DISMISSED] paused=false"));
}

void AEchoesPlayerController::CyclePlayableFaction()
{
    if (!bTitleScreenVisible && !bMissionBriefingVisible)
    {
        CycleOwnedEntity(1);
        return;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[FACTION_SIM_NOT_READY] Faction choice is unavailable."));
        return;
    }
    if (Bridge->GetOperationMode() == EEchoesOperationMode::Skirmish)
    {
        if (Bridge->IsNetworkHumanOpponentEnabled())
        {
            SetStatusMessage(
                TEXT("SKIRMISH SETUP LOCKED — network match settings are negotiated by match authority."),
                8.0f);
            return;
        }
        if (!bTitleScreenVisible)
        {
            SetStatusMessage(
                TEXT("DEPLOYMENT REVIEW — Escape returns to setup; Enter applies the shown deployment."),
                3600.0f);
            return;
        }
        PendingSkirmishSetup =
            FEchoesSkirmishSetupModel::WithNextFaction(
                PendingSkirmishSetup, true, 1);
        SetStatusMessage(
            FString::Printf(
                TEXT("LOCAL FORCE: %s — pending only; active simulation unchanged."),
                FEchoesSkirmishSetupModel::FactionDisplayName(
                    PendingSkirmishSetup.LocalFaction)),
            3600.0f);
        return;
    }
    if (Bridge->GetOperationMode() == EEchoesOperationMode::CampaignPrologue)
    {
        SetStatusMessage(TEXT("FACTION LOCKED: What the Ledger Keeps follows Mara Vey and the Meridian Compact."));
        return;
    }
    if (Bridge->GetOperationMode() ==
        EEchoesOperationMode::CampaignSevenAccounts)
    {
        SetStatusMessage(TEXT("FACTION LOCKED: Seven Accounts of Rain follows Oruun and the Kharuun Assemblies."));
        return;
    }
    if (Bridge->GetOperationMode() ==
        EEchoesOperationMode::CampaignCityReserve)
    {
        SetStatusMessage(TEXT("FACTION LOCKED: A City on Reserve follows Mara Vey and the Meridian Compact."));
        return;
    }
    if (Bridge->GetOperationMode() ==
        EEchoesOperationMode::CampaignUnburiedRoad)
    {
        SetStatusMessage(TEXT("FACTION LOCKED: The Unburied Road follows Oruun and the Kharuun Assemblies."));
        return;
    }
    if (Bridge->GetOperationMode() ==
        EEchoesOperationMode::CampaignTermsOfContinuance)
    {
        SetStatusMessage(TEXT("FACTION LOCKED: Terms of Continuance uses Meridian-authoritative treaty and witness proxies; mixed-faction command is not implemented."));
        return;
    }
    if (Bridge->GetOperationMode() ==
        EEchoesOperationMode::CampaignNamesWithoutBirths)
    {
        SetStatusMessage(TEXT("FACTION LOCKED: Names Without Births uses Meridian-authoritative Talar and civilian proxies."));
        return;
    }
    if (Bridge->GetOperationMode() ==
        EEchoesOperationMode::CampaignShapeOfSilence)
    {
        SetStatusMessage(TEXT("FACTION LOCKED: The Shape of Silence follows Oruun and two Kharuun memory witnesses."));
        return;
    }
    if (Bridge->GetOperationMode() ==
        EEchoesOperationMode::CampaignShapeBesideUs)
    {
        SetStatusMessage(TEXT("FACTION LOCKED: The Shape Beside Us follows Talar and two Meridian state witnesses."));
        return;
    }
    if (Bridge->GetOperationMode() ==
        EEchoesOperationMode::CampaignReserveAuthority)
    {
        SetStatusMessage(TEXT("FACTION LOCKED: Reserve Authority follows Mara and the Meridian district reserve network."));
        return;
    }
    if (Bridge->GetOperationMode() ==
        EEchoesOperationMode::CampaignChoirAtLumeReach)
    {
        SetStatusMessage(TEXT("FACTION LOCKED: The Choir at Lume Reach follows Oruun's Kharuun listening force. Mara is an off-map liaison, and the local Choir is not a commandable faction."));
        return;
    }
    if (Bridge->GetOperationMode() ==
        EEchoesOperationMode::CampaignNoNeutralLedger)
    {
        SetStatusMessage(TEXT("FACTION LOCKED: No Neutral Ledger follows Oruun's Kharuun ledger force. Meridian district systems and the Hollow Choir are public interfaces, not commandable factions."));
        return;
    }
    if (Bridge->GetOperationMode() ==
        EEchoesOperationMode::CampaignFutureThatWon)
    {
        SetStatusMessage(TEXT("FACTION LOCKED: The Future That Won follows Oruun and an independent verifier under Kharuun authority. Rhyse's demonstrator and Meridian readbacks are public interfaces, not commandable factions."));
        return;
    }
    if (Bridge->GetOperationMode() ==
        EEchoesOperationMode::CampaignAssemblyOfTheMissing)
    {
        SetStatusMessage(TEXT("FACTION LOCKED: Assembly of the Missing follows Oruun and an independent verifier under Kharuun authority. The Meridian, Kharuun, and Crownfall public record interfaces are neutral and non-commandable."));
        return;
    }
    if (Bridge->GetOperationMode() ==
        EEchoesOperationMode::CampaignSeveralVoicesOneCommand)
    {
        SetStatusMessage(TEXT("FACTION LOCKED: Several Voices, One Command follows Neme and a player-commanded Hollow Choir force."));
        return;
    }
    if (Bridge->GetOperationMode() ==
        EEchoesOperationMode::CampaignTheBrokenSun)
    {
        SetStatusMessage(TEXT("FACTION LOCKED: The Broken Sun retains Hollow Choir command. Mara, Oruun, and Talar are protected neutral witnesses, not mixed-faction command units."));
        return;
    }
    const echoes::sim::Faction NewFaction =
        echoes::presentation::NextPlayableFaction(
            Bridge->GetLocalFaction());
    FString Feedback;
    if (!Bridge->SelectLocalFaction(NewFaction, Feedback))
    {
        SetStatusMessage(Feedback);
        return;
    }
    ClearSelection();
    ClearControlGroups();
    bSelectionButtonDown = false;
    bControlGroupAssignmentArmed = false;
    Bridge->SetScenarioPaused(true);
    SetStatusMessage(
        FString::Printf(
            TEXT("FACTION SELECTED: %s — opposition: %s. Press Enter when ready."),
            *GetLocalFactionLabel(),
            *GetOpponentFactionLabel()),
        3600.0f);
}

void AEchoesPlayerController::CycleOperation()
{
    if (!bTitleScreenVisible && !bMissionBriefingVisible)
    {
        return;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[OPERATION_SIM_NOT_READY] Operation choice is unavailable."));
        return;
    }
    bNewCampaignConfirmationArmed = false;
    NewCampaignConfirmationExpiresAt = 0.0;
    bCampaignRestoreConfirmationArmed = false;
    CampaignRestoreConfirmationExpiresAt = 0.0;
    EEchoesOperationMode NewOperation = EEchoesOperationMode::Skirmish;
    if (Bridge->GetOperationMode() == EEchoesOperationMode::Skirmish)
    {
        NewOperation = EEchoesOperationMode::CampaignPrologue;
    }
    else if (Bridge->GetOperationMode() ==
                 EEchoesOperationMode::CampaignPrologue &&
             Bridge->IsSevenAccountsUnlocked())
    {
        NewOperation = EEchoesOperationMode::CampaignSevenAccounts;
    }
    else if (Bridge->GetOperationMode() ==
                 EEchoesOperationMode::CampaignSevenAccounts &&
             Bridge->IsCityReserveUnlocked())
    {
        NewOperation = EEchoesOperationMode::CampaignCityReserve;
    }
    else if (Bridge->GetOperationMode() ==
                 EEchoesOperationMode::CampaignCityReserve &&
             Bridge->IsUnburiedRoadUnlocked())
    {
        NewOperation = EEchoesOperationMode::CampaignUnburiedRoad;
    }
    else if (Bridge->GetOperationMode() ==
                 EEchoesOperationMode::CampaignUnburiedRoad &&
             Bridge->IsTermsOfContinuanceUnlocked())
    {
        NewOperation =
            EEchoesOperationMode::CampaignTermsOfContinuance;
    }
    else if (Bridge->GetOperationMode() ==
                 EEchoesOperationMode::CampaignTermsOfContinuance &&
             Bridge->IsNamesWithoutBirthsUnlocked())
    {
        NewOperation = EEchoesOperationMode::CampaignNamesWithoutBirths;
    }
    else if (Bridge->GetOperationMode() ==
                 EEchoesOperationMode::CampaignNamesWithoutBirths &&
             Bridge->IsShapeOfSilenceUnlocked())
    {
        NewOperation = EEchoesOperationMode::CampaignShapeOfSilence;
    }
    else if (Bridge->GetOperationMode() ==
                 EEchoesOperationMode::CampaignShapeOfSilence &&
             Bridge->IsShapeBesideUsUnlocked())
    {
        NewOperation = EEchoesOperationMode::CampaignShapeBesideUs;
    }
    else if (Bridge->GetOperationMode() ==
                 EEchoesOperationMode::CampaignShapeBesideUs &&
             Bridge->IsReserveAuthorityUnlocked())
    {
        NewOperation = EEchoesOperationMode::CampaignReserveAuthority;
    }
    else if (Bridge->GetOperationMode() ==
                 EEchoesOperationMode::CampaignReserveAuthority &&
             Bridge->IsChoirAtLumeReachUnlocked())
    {
        NewOperation = EEchoesOperationMode::CampaignChoirAtLumeReach;
    }
    else if (Bridge->GetOperationMode() ==
                 EEchoesOperationMode::CampaignChoirAtLumeReach &&
             Bridge->IsNoNeutralLedgerUnlocked())
    {
        NewOperation = EEchoesOperationMode::CampaignNoNeutralLedger;
    }
    else if (Bridge->GetOperationMode() ==
                 EEchoesOperationMode::CampaignNoNeutralLedger &&
             Bridge->IsFutureThatWonUnlocked())
    {
        NewOperation = EEchoesOperationMode::CampaignFutureThatWon;
    }
    else if (Bridge->GetOperationMode() ==
                 EEchoesOperationMode::CampaignFutureThatWon &&
             Bridge->IsAssemblyOfTheMissingUnlocked())
    {
        NewOperation =
            EEchoesOperationMode::CampaignAssemblyOfTheMissing;
    }
    else if (Bridge->GetOperationMode() ==
                 EEchoesOperationMode::CampaignAssemblyOfTheMissing &&
             Bridge->IsSeveralVoicesOneCommandUnlocked())
    {
        NewOperation =
            EEchoesOperationMode::CampaignSeveralVoicesOneCommand;
    }
    else if (Bridge->GetOperationMode() ==
                 EEchoesOperationMode::CampaignSeveralVoicesOneCommand &&
             Bridge->IsBrokenSunUnlocked())
    {
        NewOperation = EEchoesOperationMode::CampaignTheBrokenSun;
    }
    FString Feedback;
    if (!Bridge->SelectOperationMode(NewOperation, Feedback))
    {
        SetStatusMessage(Feedback);
        return;
    }
    if (NewOperation == EEchoesOperationMode::Skirmish)
    {
        PendingSkirmishSetup = Bridge->GetActiveSkirmishSetup();
        SkirmishSetupFocusRow = 0;
    }
    SynchronizeBoundCampaignProtocol();
    ClearSelection();
    ClearControlGroups();
    bSelectionButtonDown = false;
    bControlGroupAssignmentArmed = false;
    Bridge->SetScenarioPaused(true);
    SetStatusMessage(
        FString::Printf(
            TEXT("%s Press Enter when ready."),
            *Feedback),
        3600.0f);
}

void AEchoesPlayerController::ContinueCampaign()
{
    const bool bContinuingFromTitle = bTitleScreenVisible;
    const bool bContinuingFromResult =
        bMatchResultVisible && CanAdvanceCampaignResult();
    if (!bContinuingFromTitle && !bContinuingFromResult)
    {
        return;
    }

    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(
            TEXT("[CAMPAIGN_CONTINUE_SIM_NOT_READY] Campaign continuation is unavailable."),
            12.0f);
        return;
    }

    bNewCampaignConfirmationArmed = false;
    NewCampaignConfirmationExpiresAt = 0.0;
    bCampaignRestoreConfirmationArmed = false;
    CampaignRestoreConfirmationExpiresAt = 0.0;
    const FEchoesCampaignJourney Journey = Bridge->GetCampaignJourney();
    if (Journey.State == EEchoesCampaignJourneyState::Complete)
    {
        if (bContinuingFromResult)
        {
            PresentTitleScreen();
        }
        SetStatusMessage(
            TEXT("CAMPAIGN COMPLETE — the recorded Broken Sun resolution is preserved in the campaign ledger. Choose an operation to replay or start a new campaign."),
            3600.0f);
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_CAMPAIGN_CONTINUE] state=complete records=%d source=%s mission16=false"),
            Journey.CompletedMissionCount,
            bContinuingFromResult ? TEXT("result") : TEXT("title"));
        return;
    }
    if (Journey.State != EEchoesCampaignJourneyState::Ready)
    {
        SetStatusMessage(
            TEXT("[CAMPAIGN_CONTINUE_UNAVAILABLE] The active ledger does not admit a safe next mission. Restore a validated prior generation or start a new campaign."),
            15.0f);
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_CAMPAIGN_CONTINUE] state=unavailable records=%d source=%s"),
            Journey.CompletedMissionCount,
            bContinuingFromResult ? TEXT("result") : TEXT("title"));
        return;
    }

    FString Feedback;
    if (!Bridge->SelectOperationMode(Journey.NextOperation, Feedback))
    {
        SetStatusMessage(Feedback, 15.0f);
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_CAMPAIGN_CONTINUE] state=selection_failed records=%d next=%s source=%s detail=%s"),
            Journey.CompletedMissionCount,
            FEchoesCampaignJourneyModel::OperationDisplayName(
                Journey.NextOperation),
            bContinuingFromResult ? TEXT("result") : TEXT("title"),
            *Feedback);
        return;
    }

    SynchronizeBoundCampaignProtocol();
    ClearSelection();
    ClearControlGroups();
    bSelectionButtonDown = false;
    bControlGroupAssignmentArmed = false;
    PresentMissionBriefing();
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_CAMPAIGN_CONTINUE] state=ready records=%d next=%s source=%s briefing=true paused=true"),
        Journey.CompletedMissionCount,
        FEchoesCampaignJourneyModel::OperationDisplayName(
            Journey.NextOperation),
        bContinuingFromResult ? TEXT("result") : TEXT("title"));
}

bool AEchoesPlayerController::IsNewCampaignConfirmationArmed() const
{
    return bNewCampaignConfirmationArmed && GetWorld() != nullptr &&
           GetWorld()->GetTimeSeconds() <=
               NewCampaignConfirmationExpiresAt;
}

bool AEchoesPlayerController::IsCampaignRestoreConfirmationArmed() const
{
    return bCampaignRestoreConfirmationArmed && GetWorld() != nullptr &&
           GetWorld()->GetTimeSeconds() <=
               CampaignRestoreConfirmationExpiresAt;
}

bool AEchoesPlayerController::IsReturnToOperationsConfirmationArmed() const
{
    return bReturnToOperationsConfirmationArmed && GetWorld() != nullptr &&
        GetWorld()->GetTimeSeconds() <=
            ReturnToOperationsConfirmationExpiresAt;
}

void AEchoesPlayerController::RequestReturnToOperations()
{
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (!bPauseMenuVisible || Bridge == nullptr ||
        Bridge->GetOperationMode() != EEchoesOperationMode::Skirmish ||
        Bridge->IsNetworkHumanOpponentEnabled())
    {
        SetStatusMessage(
            TEXT("[RETURN_TO_OPERATIONS_UNAVAILABLE] Pause an active skirmish before leaving the field."));
        return;
    }
    if (!IsReturnToOperationsConfirmationArmed())
    {
        bReturnToOperationsConfirmationArmed = true;
        ReturnToOperationsConfirmationExpiresAt =
            GetWorld()->GetTimeSeconds() + 10.0;
        SetStatusMessage(
            TEXT("RETURN TO OPERATIONS ARMED — press F10 / Menu again within 10 seconds. The field remains paused; resume it unchanged or replace it by changing the setup."),
            10.0f);
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_RETURN_TO_OPERATIONS_ARMED] confirmationSeconds=10 skirmishOnly=true simulationPaused=true"));
        return;
    }

    bReturnToOperationsConfirmationArmed = false;
    ReturnToOperationsConfirmationExpiresAt = 0.0;
    ClearSelection();
    ClearControlGroups();
    bSelectionButtonDown = false;
    bControlGroupAssignmentArmed = false;
    PendingSkirmishSetup = Bridge->GetActiveSkirmishSetup();
    SkirmishSetupFocusRow = 0;
    PresentTitleScreen();
    if (bTitleScreenVisible)
    {
        SetStatusMessage(
            TEXT("RETURNED TO OPERATIONS — the field is paused. The current setup resumes the same match; changing setup replaces it after confirmation."),
            3600.0f);
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_RETURNED_TO_OPERATIONS] title=true fieldPaused=true campaignLedgerUnchanged=true"));
    }
}

void AEchoesPlayerController::RequestNewCampaign()
{
    if (bOnlineLocalMenuVisible)
    {
        LeaveOnlineMatch();
        return;
    }
    if (bPauseMenuVisible)
    {
        RequestReturnToOperations();
        return;
    }
    if (!bTitleScreenVisible)
    {
        SetStatusMessage(TEXT("[NEW_CAMPAIGN_TITLE_REQUIRED] Return to the title screen before replacing campaign progress."));
        return;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[NEW_CAMPAIGN_SIM_NOT_READY] Campaign reset is unavailable."));
        return;
    }
    bCampaignRestoreConfirmationArmed = false;
    CampaignRestoreConfirmationExpiresAt = 0.0;
    if (Bridge->GetCampaignProgress().Decisions.IsEmpty())
    {
        bNewCampaignConfirmationArmed = false;
        NewCampaignConfirmationExpiresAt = 0.0;
        SetStatusMessage(TEXT("NEW CAMPAIGN: the campaign ledger is already empty."));
        return;
    }
    if (!IsNewCampaignConfirmationArmed())
    {
        bNewCampaignConfirmationArmed = true;
        NewCampaignConfirmationExpiresAt =
            GetWorld()->GetTimeSeconds() + 10.0;
        SetStatusMessage(
            TEXT("NEW CAMPAIGN ARMED — press F10 again within 10 seconds to replace active progress. One prior ledger generation will be retained as backup."),
            10.0f);
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_NEW_CAMPAIGN_ARMED] records=%d confirmationSeconds=10 backupRetained=true"),
            Bridge->GetCampaignProgress().Decisions.Num());
        return;
    }

    FString Feedback;
    if (!Bridge->StartNewCampaign(Feedback))
    {
        bNewCampaignConfirmationArmed = false;
        NewCampaignConfirmationExpiresAt = 0.0;
        SetStatusMessage(Feedback, 12.0f);
        return;
    }
    bNewCampaignConfirmationArmed = false;
    NewCampaignConfirmationExpiresAt = 0.0;
    ClearSelection();
    ClearControlGroups();
    bSelectionButtonDown = false;
    bControlGroupAssignmentArmed = false;
    bCampaignResult = false;
    bCampaignSuccess = false;
    CampaignConsequence = echoes::sim::FutureWellChoice::Dormant;
    RecordedCampaignConsequence = echoes::sim::FutureWellChoice::Dormant;
    CampaignFinalResolution = EEchoesFinalResolution::None;
    RecordedCampaignFinalResolution = EEchoesFinalResolution::None;
    CampaignCommitStatus = EEchoesCampaignCommitStatus::NotApplicable;
    PresentedCampaignOperation = EEchoesOperationMode::Skirmish;
    PresentedMatchOutcome = echoes::sim::MatchOutcome::Ongoing;
    SetStatusMessage(Feedback, 12.0f);
}

void AEchoesPlayerController::RequestCampaignRestore()
{
    if (!bTitleScreenVisible)
    {
        SetStatusMessage(TEXT("[CAMPAIGN_RESTORE_TITLE_REQUIRED] Return to the title screen before restoring campaign progress."));
        return;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[CAMPAIGN_RESTORE_SIM_NOT_READY] Campaign recovery is unavailable."));
        return;
    }
    bNewCampaignConfirmationArmed = false;
    NewCampaignConfirmationExpiresAt = 0.0;
    if (!Bridge->HasRestorableCampaignBackup())
    {
        bCampaignRestoreConfirmationArmed = false;
        CampaignRestoreConfirmationExpiresAt = 0.0;
        SetStatusMessage(TEXT("CAMPAIGN RECOVERY: no distinct validated prior generation is available."));
        return;
    }
    if (!IsCampaignRestoreConfirmationArmed())
    {
        bCampaignRestoreConfirmationArmed = true;
        CampaignRestoreConfirmationExpiresAt =
            GetWorld()->GetTimeSeconds() + 30.0;
        SetStatusMessage(
            FString::Printf(
                TEXT("CAMPAIGN RESTORE ARMED — press Page Up again within 30 seconds to activate the validated %d-record prior generation. The current generation will become the backup."),
                Bridge->GetCampaignBackupDecisionCount()),
            30.0f);
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_CAMPAIGN_RESTORE_ARMED] activeRecords=%d backupRecords=%d confirmationSeconds=30 reversible=true"),
            Bridge->GetCampaignProgress().Decisions.Num(),
            Bridge->GetCampaignBackupDecisionCount());
        return;
    }

    FString Feedback;
    if (!Bridge->RestoreCampaignBackup(Feedback))
    {
        bCampaignRestoreConfirmationArmed = false;
        CampaignRestoreConfirmationExpiresAt = 0.0;
        SetStatusMessage(Feedback, 12.0f);
        return;
    }
    bCampaignRestoreConfirmationArmed = false;
    CampaignRestoreConfirmationExpiresAt = 0.0;
    ClearSelection();
    ClearControlGroups();
    bSelectionButtonDown = false;
    bControlGroupAssignmentArmed = false;
    bCampaignResult = false;
    bCampaignSuccess = false;
    CampaignConsequence = echoes::sim::FutureWellChoice::Dormant;
    RecordedCampaignConsequence = echoes::sim::FutureWellChoice::Dormant;
    CampaignFinalResolution = EEchoesFinalResolution::None;
    RecordedCampaignFinalResolution = EEchoesFinalResolution::None;
    CampaignCommitStatus = EEchoesCampaignCommitStatus::NotApplicable;
    PresentedCampaignOperation = EEchoesOperationMode::Skirmish;
    PresentedMatchOutcome = echoes::sim::MatchOutcome::Ongoing;
    SetStatusMessage(Feedback, 12.0f);
}

void AEchoesPlayerController::CycleOwnedEntityPrevious()
{
    if (bTitleScreenVisible || bMissionBriefingVisible)
    {
        CyclePlayableFaction();
        return;
    }
    CycleOwnedEntity(-1);
}

void AEchoesPlayerController::SelectCombatForce()
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    const echoes::sim::Simulation* Simulation =
        Bridge != nullptr ? Bridge->GetSimulation() : nullptr;
    if (Bridge == nullptr || Simulation == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Combat-force selection is unavailable."));
        return;
    }

    TArray<uint32> CombatIds;
    for (const echoes::sim::Entity& Entity : Simulation->Entities())
    {
        const bool bCombatUnit =
            Entity.type == echoes::sim::EntityType::Soldier ||
            Entity.type == echoes::sim::EntityType::HeavyUnit ||
            Entity.type == echoes::sim::EntityType::ScoutUnit;
        if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            Entity.hitPoints > 0 && bCombatUnit &&
            !Entity.temporaryMineralCover &&
            Bridge->FindEntityView(Entity.id) != nullptr)
        {
            CombatIds.Add(Entity.id);
        }
    }
    CombatIds.Sort();
    if (CombatIds.IsEmpty())
    {
        SetStatusMessage(TEXT("[NO_COMBAT_FORCE] No live owned combat unit is visible."));
        return;
    }

    ClearSelection();
    for (const uint32 EntityId : CombatIds)
    {
        SelectedEntityIds.Add(EntityId);
        SetEntitySelected(EntityId, true);
    }
    SetStatusMessage(
        FString::Printf(
            TEXT("COMBAT FORCE: %d visible owned units selected // End centers force"),
            CombatIds.Num()),
        5.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_KEYBOARD_FORCE_SELECT] count=%d source=owned_presentation_views hiddenStateRead=false"),
        CombatIds.Num());
}

void AEchoesPlayerController::CycleFormation()
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    switch (CurrentFormation)
    {
        case EEchoesFormationType::Box:
            CurrentFormation = EEchoesFormationType::Line;
            break;
        case EEchoesFormationType::Line:
            CurrentFormation = EEchoesFormationType::Wedge;
            break;
        case EEchoesFormationType::Wedge:
            CurrentFormation = EEchoesFormationType::Box;
            break;
    }
    SetStatusMessage(FString::Printf(
        TEXT("FORMATION: %s — Move, Attack-move, and Patrol will align to the destination."),
        *GetFormationLabel()),
        5.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_FORMATION_SELECTED] type=%s commandAuthority=destinations_only replaySafe=true"),
        *GetFormationLabel());
}

void AEchoesPlayerController::ToggleKeyboardTargeting()
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    bKeyboardTargetingEnabled = !bKeyboardTargetingEnabled;
    KeyboardTargetOffset = FVector2D::ZeroVector;
    SetStatusMessage(
        bKeyboardTargetingEnabled
            ? TEXT("KEYBOARD TARGET: arrows move reticle // Space orders // F/B/N/M/F6 use reticle // Home exits")
            : TEXT("POINTER TARGET: cursor-directed orders restored."),
        6.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_KEYBOARD_TARGET_MODE] enabled=%s source=screen_reticle offsetPx=(0,0) hiddenStateRead=false"),
        bKeyboardTargetingEnabled ? TEXT("true") : TEXT("false"));
}

void AEchoesPlayerController::NudgeKeyboardTarget(const FVector2D& Direction)
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    int32 ViewportWidth = 0;
    int32 ViewportHeight = 0;
    GetViewportSize(ViewportWidth, ViewportHeight);
    if (ViewportWidth <= 0 || ViewportHeight <= 0)
    {
        return;
    }
    bKeyboardTargetingEnabled = true;
    constexpr float StepPixels = 64.0f;
    constexpr float EdgeMarginPixels = 32.0f;
    KeyboardTargetOffset += Direction * StepPixels;
    KeyboardTargetOffset.X = FMath::Clamp(
        KeyboardTargetOffset.X,
        -(static_cast<float>(ViewportWidth) * 0.5f - EdgeMarginPixels),
        static_cast<float>(ViewportWidth) * 0.5f - EdgeMarginPixels);
    KeyboardTargetOffset.Y = FMath::Clamp(
        KeyboardTargetOffset.Y,
        -(static_cast<float>(ViewportHeight) * 0.5f - EdgeMarginPixels),
        static_cast<float>(ViewportHeight) * 0.5f - EdgeMarginPixels);
    SetStatusMessage(
        FString::Printf(
            TEXT("KEYBOARD TARGET: offset (%+.0f, %+.0f) px // Space orders // Home resets/exits"),
            KeyboardTargetOffset.X,
            KeyboardTargetOffset.Y),
        2.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_KEYBOARD_TARGET_NUDGE] offsetPx=(%.0f,%.0f) source=screen_reticle hiddenStateRead=false"),
        KeyboardTargetOffset.X,
        KeyboardTargetOffset.Y);
}

void AEchoesPlayerController::NudgeKeyboardTargetLeft()
{
    NudgeKeyboardTarget(FVector2D(-1.0f, 0.0f));
}

void AEchoesPlayerController::NudgeKeyboardTargetRight()
{
    NudgeKeyboardTarget(FVector2D(1.0f, 0.0f));
}

void AEchoesPlayerController::SnapKeyboardTargetToSelection()
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    PruneSelection();
    if (SelectedEntityIds.IsEmpty())
    {
        SetStatusMessage(TEXT("[SNAP_REQUIRES_SELECTION] Select one or more visible owned entities first."));
        return;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    APawn* CameraPawn = GetPawn();
    if (Bridge == nullptr || CameraPawn == nullptr)
    {
        SetStatusMessage(TEXT("[SELECTED_VIEW_UNAVAILABLE] The selected presentation views are unavailable."));
        return;
    }

    FVector Centroid = FVector::ZeroVector;
    int32 VisibleSelectionCount = 0;
    for (const uint32 EntityId : SelectedEntityIds)
    {
        const echoes::sim::Entity* Entity = Bridge->FindEntity(EntityId);
        const AEchoesEntityView* View = Bridge->FindEntityView(EntityId);
        if (Entity == nullptr ||
            Entity->owner != UEchoesSimulationSubsystem::LocalPlayerId ||
            Entity->hitPoints <= 0 || View == nullptr)
        {
            continue;
        }
        Centroid += View->GetActorLocation();
        ++VisibleSelectionCount;
    }
    if (VisibleSelectionCount == 0)
    {
        SetStatusMessage(TEXT("[SELECTED_VIEW_UNAVAILABLE] No selected owned presentation view is available."));
        return;
    }
    Centroid /= static_cast<float>(VisibleSelectionCount);
    FVector CameraLocation = CameraPawn->GetActorLocation();
    CameraLocation.X = Centroid.X;
    CameraLocation.Y = Centroid.Y;
    CameraPawn->SetActorLocation(CameraLocation);
    KeyboardTargetOffset = FVector2D::ZeroVector;
    bKeyboardTargetingEnabled = true;
    SetStatusMessage(
        TEXT("KEYBOARD TARGET: camera and reticle centered on selected visible force // arrows choose a visible destination."),
        4.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_KEYBOARD_TARGET_SNAP] count=%d centroid=(%.1f,%.1f) offsetPx=(0,0) cameraCentered=true source=selected_owned_views hiddenStateRead=false"),
        VisibleSelectionCount,
        Centroid.X,
        Centroid.Y);
}

void AEchoesPlayerController::CycleOwnedEntity(int32 Direction)
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    const echoes::sim::Simulation* Simulation =
        Bridge != nullptr ? Bridge->GetSimulation() : nullptr;
    if (Bridge == nullptr || Simulation == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Keyboard selection is unavailable."));
        return;
    }

    TArray<uint32> Candidates;
    for (const echoes::sim::Entity& Entity : Simulation->Entities())
    {
        if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            Entity.hitPoints > 0 && !Entity.temporaryMineralCover &&
            Bridge->FindEntityView(Entity.id) != nullptr)
        {
            Candidates.Add(Entity.id);
        }
    }
    Candidates.Sort();
    if (Candidates.IsEmpty())
    {
        SetStatusMessage(TEXT("[NO_OWNED_ENTITIES] No live owned entity can be selected."));
        return;
    }

    int32 CandidateIndex = Direction < 0 ? Candidates.Num() - 1 : 0;
    if (SelectedEntityIds.Num() == 1)
    {
        const int32 CurrentIndex = Candidates.IndexOfByKey(SelectedEntityIds[0]);
        if (CurrentIndex != INDEX_NONE)
        {
            CandidateIndex =
                (CurrentIndex + (Direction < 0 ? -1 : 1) + Candidates.Num()) %
                Candidates.Num();
        }
    }

    ClearSelection();
    const uint32 SelectedId = Candidates[CandidateIndex];
    SelectedEntityIds.Add(SelectedId);
    SetEntitySelected(SelectedId, true);
    const AEchoesEntityView* View = Bridge->FindEntityView(SelectedId);
    SetStatusMessage(
        FString::Printf(
            TEXT("KEYBOARD SELECT: %s  //  entity %u  //  Tab next / Backspace previous"),
            View != nullptr ? *View->GetDisplayName() : TEXT("owned entity"),
            SelectedId),
        4.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_KEYBOARD_SELECTION] entity=%u index=%d total=%d direction=%s owned=true"),
        SelectedId,
        CandidateIndex,
        Candidates.Num(),
        Direction < 0 ? TEXT("previous") : TEXT("next"));
}

void AEchoesPlayerController::ConfirmPrimaryAction()
{
    if (IsOnlineFrontDoorVisible())
    {
        ConfirmOnlineFrontDoorAction();
    }
    else if (GetNetMode() == NM_Client && bNetworkCompatibilityAccepted &&
        !bNetworkMatchStarted)
    {
        ServerSetNetworkReady();
        SetStatusMessage(
            TEXT("ONLINE LOBBY — ready submitted; waiting for authority start."),
            3600.0f);
    }
    else if (bTitleScreenVisible)
    {
        ConfirmTitleScreen();
    }
    else if (bMissionBriefingVisible)
    {
        ConfirmMissionBriefing();
    }
    else if (bOnlineLocalMenuVisible)
    {
        TogglePauseMenu();
    }
    else if (bMatchResultVisible)
    {
        if (IsOnlineMatchResult())
        {
            if (CanLeaveNetworkMatchToOnlineMenu())
            {
                LeaveOnlineMatch();
            }
            else
            {
                SetStatusMessage(
                    TEXT("ONLINE RESULT — waiting briefly for final delivery confirmation."),
                    4.0f);
            }
        }
        else if (CanAdvanceCampaignResult())
        {
            ContinueCampaign();
        }
        else if (CanReturnCompletedSkirmishToOperations())
        {
            if (UEchoesSimulationSubsystem* Bridge =
                    GetWorld() != nullptr
                        ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
                        : nullptr)
            {
                PendingSkirmishSetup = Bridge->GetActiveSkirmishSetup();
                SkirmishSetupFocusRow = 0;
            }
            PresentTitleScreen();
            if (bTitleScreenVisible)
            {
                SetStatusMessage(
                    TEXT("OPERATIONS — the completed field remains retained; review the same setup or adjust it before the next deployment."),
                    3600.0f);
            }
        }
        else
        {
            RestartScenario();
        }
    }
    else if (bTechnologyPanelVisible)
    {
        ResearchTechnologyByTier(TechnologyPanelFocusedTier);
    }
    else if (bPauseMenuVisible)
    {
        TogglePauseMenu();
    }
}

void AEchoesPlayerController::NotifyRuntimeFailure(const FString& FailureCode)
{
    bRuntimeStateKnown = true;
    SetStatusMessage(
        FString::Printf(
            TEXT("[%s] Runtime prototype initialization failed; inspect LogEchoes."),
            *FailureCode),
        15.0f);
}

void AEchoesPlayerController::NotifyMatchFinished(
    echoes::sim::MatchOutcome Outcome)
{
    ClearSelection();
    bControlGroupAssignmentArmed = false;
    bSelectionButtonDown = false;
    bTitleScreenVisible = false;
    bMissionBriefingVisible = false;
    bPauseMenuVisible = false;
    bOnlineLocalMenuVisible = false;
    bTechnologyPanelVisible = false;
    bMatchResultVisible = true;
    bCampaignResult = false;
    PresentedCampaignOperation = EEchoesOperationMode::Skirmish;
    PresentedMatchOutcome = Outcome;
    if (GetNetMode() != NM_Client)
    {
        const UEchoesSimulationSubsystem* Bridge =
            GetWorld() != nullptr
                ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
                : nullptr;
        if (Bridge != nullptr && Bridge->GetSimulation() != nullptr)
        {
            PresentedFinalTick = Bridge->GetSimulation()->CurrentTick();
        }
    }
    SetIgnoreMoveInput(true);
    SetIgnoreLookInput(true);
    if (Outcome != echoes::sim::MatchOutcome::Draw)
    {
        PresentResultAudio(OutcomeBelongsToSeat(
            Outcome,
            GetNetMode() == NM_Client &&
                    NetworkSeat < echoes::sim::kMaximumPlayers
                ? NetworkSeat
                : UEchoesSimulationSubsystem::LocalPlayerId));
    }
    FString Message =
        TEXT("DRAW — both Command Cores fell in the same deterministic tick.");
    if (OutcomeBelongsToSeat(
            Outcome,
            GetNetMode() == NM_Client &&
                    NetworkSeat < echoes::sim::kMaximumPlayers
                ? NetworkSeat
                : UEchoesSimulationSubsystem::LocalPlayerId))
    {
        Message = TEXT("VICTORY — the opposing Command Core has fallen.");
    }
    else if (Outcome != echoes::sim::MatchOutcome::Draw)
    {
        Message = TEXT("DEFEAT — your Command Core has fallen.");
    }
    const bool bOnlineResult = IsOnlineMatchResult();
    if (bOnlineResult && GetNetMode() == NM_ListenServer)
    {
        if (AEchoesGameMode* GameMode =
                GetWorld() != nullptr
                    ? GetWorld()->GetAuthGameMode<AEchoesGameMode>()
                    : nullptr)
        {
            GameMode->NotifyNetworkMatchFinished();
        }
        if (!bNetworkResultExitEnabled &&
            !GetWorldTimerManager().IsTimerActive(
                NetworkResultAcknowledgementTimer))
        {
            GetWorldTimerManager().SetTimer(
                NetworkResultAcknowledgementTimer,
                this,
                &AEchoesPlayerController::AllowHostNetworkResultExitAfterTimeout,
                10.0f,
                false);
        }
    }
    Message += bOnlineResult
        ? CanLeaveNetworkMatchToOnlineMenu()
            ? TEXT(" Press Enter, controller A, or R to leave for the Online 1v1 menu.")
            : TEXT(" Final result delivery is being confirmed; the Online menu will unlock shortly.")
        : CanReturnCompletedSkirmishToOperations()
            ? TEXT(" Press Enter or controller A to return to Operations, or R to restart.")
            : TEXT(" Press R to restart.");
    SetStatusMessage(Message, 3600.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_RESULT_PRESENTED] outcome=%u primaryAction=%s keyboardRestart=%s"),
        static_cast<uint8>(Outcome),
        bOnlineResult
            ? TEXT("online_menu")
        : CanReturnCompletedSkirmishToOperations()
            ? TEXT("return_to_operations")
            : TEXT("restart"),
        bOnlineResult ? TEXT("false") : TEXT("true"));
}

void AEchoesPlayerController::NotifyCampaignPrologueFinished(
    bool bSuccess,
    echoes::sim::FutureWellChoice Consequence,
    echoes::sim::FutureWellChoice RecordedConsequence,
    EEchoesCampaignCommitStatus CommitStatus)
{
    ClearSelection();
    bControlGroupAssignmentArmed = false;
    bSelectionButtonDown = false;
    bTitleScreenVisible = false;
    bMissionBriefingVisible = false;
    bPauseMenuVisible = false;
    bTechnologyPanelVisible = false;
    bMatchResultVisible = true;
    bCampaignResult = true;
    PresentedCampaignOperation = EEchoesOperationMode::CampaignPrologue;
    bCampaignSuccess = bSuccess;
    PresentResultAudio(bSuccess);
    CampaignConsequence = Consequence;
    RecordedCampaignConsequence = RecordedConsequence;
    CampaignCommitStatus = CommitStatus;
    FutureWellChoice = Consequence;
    PresentedMatchOutcome = bSuccess
        ? echoes::sim::MatchOutcome::Player0Victory
        : echoes::sim::MatchOutcome::Player1Victory;
    SetIgnoreMoveInput(true);
    SetIgnoreLookInput(true);
    FString ResultMessage =
        TEXT("MISSION FAILED — the archive carrier or withdrawal line was lost. Press R to replay.");
    if (bSuccess)
    {
        ResultMessage = FString::Printf(
            TEXT("MISSION COMPLETE — archive recovered, %s protocol completed, and Mara Vey withdrew to Lume Reach."),
            *GetFutureWellChoiceLabel());
        if (CommitStatus == EEchoesCampaignCommitStatus::Added)
        {
            ResultMessage += TEXT(" Campaign ledger committed. Press R to replay.");
        }
        else if (CommitStatus == EEchoesCampaignCommitStatus::AlreadyRecorded)
        {
            ResultMessage += TEXT(" Campaign ledger already contains this decision. Press R to replay.");
        }
        else if (CommitStatus == EEchoesCampaignCommitStatus::ReplayConflict)
        {
            ResultMessage += TEXT(" Replay choice retained for this result; the original campaign decision remains unchanged. Press R to replay.");
        }
        else
        {
            ResultMessage += TEXT(" Campaign progress was not saved. Press R to replay.");
        }
    }
    SetStatusMessage(ResultMessage, 3600.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_CAMPAIGN_RESULT_PRESENTED] mission=WhatTheLedgerKeeps success=%s consequence=%u recordedConsequence=%u campaignStatus=%u keyboardRestart=true"),
        bSuccess ? TEXT("true") : TEXT("false"),
        static_cast<uint8>(Consequence),
        static_cast<uint8>(RecordedConsequence),
        static_cast<uint8>(CommitStatus));
}

void AEchoesPlayerController::NotifySevenAccountsFinished(
    bool bSuccess,
    echoes::sim::FutureWellChoice Consequence,
    echoes::sim::FutureWellChoice RecordedConsequence,
    EEchoesCampaignCommitStatus CommitStatus)
{
    ClearSelection();
    bControlGroupAssignmentArmed = false;
    bSelectionButtonDown = false;
    bTitleScreenVisible = false;
    bMissionBriefingVisible = false;
    bPauseMenuVisible = false;
    bTechnologyPanelVisible = false;
    bMatchResultVisible = true;
    bCampaignResult = true;
    bCampaignSuccess = bSuccess;
    PresentResultAudio(bSuccess);
    PresentedCampaignOperation =
        EEchoesOperationMode::CampaignSevenAccounts;
    CampaignConsequence = Consequence;
    RecordedCampaignConsequence = RecordedConsequence;
    CampaignCommitStatus = CommitStatus;
    FutureWellChoice = Consequence;
    PresentedMatchOutcome = bSuccess
        ? echoes::sim::MatchOutcome::Player0Victory
        : echoes::sim::MatchOutcome::Player1Victory;
    SetIgnoreMoveInput(true);
    SetIgnoreLookInput(true);
    FString ResultMessage =
        TEXT("MISSION FAILED — Oruun, the Waystone, the local Core, or the migration route was lost. Press R to replay.");
    if (bSuccess)
    {
        ResultMessage = FString::Printf(
            TEXT("MISSION COMPLETE — the %s route is rooted and Oruun reached the matching account."),
            *GetFutureWellChoiceLabel());
        if (CommitStatus == EEchoesCampaignCommitStatus::Added)
        {
            ResultMessage += TEXT(" Campaign ledger committed. Press R to replay.");
        }
        else if (CommitStatus == EEchoesCampaignCommitStatus::AlreadyRecorded)
        {
            ResultMessage += TEXT(" Campaign ledger already contains this route. Press R to replay.");
        }
        else
        {
            ResultMessage += TEXT(" Campaign progress was not saved. Press R to replay.");
        }
    }
    SetStatusMessage(ResultMessage, 3600.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_CAMPAIGN_RESULT_PRESENTED] mission=SevenAccountsOfRain success=%s consequence=%u recordedConsequence=%u campaignStatus=%u keyboardRestart=true"),
        bSuccess ? TEXT("true") : TEXT("false"),
        static_cast<uint8>(Consequence),
        static_cast<uint8>(RecordedConsequence),
        static_cast<uint8>(CommitStatus));
}

void AEchoesPlayerController::NotifyCityReserveFinished(
    bool bSuccess,
    echoes::sim::FutureWellChoice Consequence,
    echoes::sim::FutureWellChoice RecordedConsequence,
    EEchoesCampaignCommitStatus CommitStatus)
{
    ClearSelection();
    bControlGroupAssignmentArmed = false;
    bSelectionButtonDown = false;
    bTitleScreenVisible = false;
    bMissionBriefingVisible = false;
    bPauseMenuVisible = false;
    bTechnologyPanelVisible = false;
    bMatchResultVisible = true;
    bCampaignResult = true;
    bCampaignSuccess = bSuccess;
    PresentResultAudio(bSuccess);
    PresentedCampaignOperation =
        EEchoesOperationMode::CampaignCityReserve;
    CampaignConsequence = Consequence;
    RecordedCampaignConsequence = RecordedConsequence;
    CampaignCommitStatus = CommitStatus;
    FutureWellChoice = Consequence;
    PresentedMatchOutcome = bSuccess
        ? echoes::sim::MatchOutcome::Player0Victory
        : echoes::sim::MatchOutcome::Player1Victory;
    SetIgnoreMoveInput(true);
    SetIgnoreLookInput(true);
    FString ResultMessage =
        TEXT("MISSION FAILED — a reserve district, the local Core, or the grid line was lost. Press R to replay.");
    if (bSuccess)
    {
        ResultMessage = FString::Printf(
            TEXT("MISSION COMPLETE — all three ark-city districts are powered under the inherited %s reserve plan."),
            *GetFutureWellChoiceLabel());
        if (CommitStatus == EEchoesCampaignCommitStatus::Added)
        {
            ResultMessage += TEXT(" Campaign ledger committed. Press R to replay.");
        }
        else if (CommitStatus == EEchoesCampaignCommitStatus::AlreadyRecorded)
        {
            ResultMessage += TEXT(" Campaign ledger already contains this grid result. Press R to replay.");
        }
        else
        {
            ResultMessage += TEXT(" Campaign progress was not saved. Press R to replay.");
        }
    }
    SetStatusMessage(ResultMessage, 3600.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_CAMPAIGN_RESULT_PRESENTED] mission=ACityOnReserve success=%s consequence=%u recordedConsequence=%u campaignStatus=%u keyboardRestart=true"),
        bSuccess ? TEXT("true") : TEXT("false"),
        static_cast<uint8>(Consequence),
        static_cast<uint8>(RecordedConsequence),
        static_cast<uint8>(CommitStatus));
}

void AEchoesPlayerController::NotifyUnburiedRoadFinished(
    bool bSuccess,
    echoes::sim::FutureWellChoice Consequence,
    echoes::sim::FutureWellChoice RecordedConsequence,
    EEchoesCampaignCommitStatus CommitStatus)
{
    ClearSelection();
    bControlGroupAssignmentArmed = false;
    bSelectionButtonDown = false;
    bTitleScreenVisible = false;
    bMissionBriefingVisible = false;
    bPauseMenuVisible = false;
    bTechnologyPanelVisible = false;
    bMatchResultVisible = true;
    bCampaignResult = true;
    bCampaignSuccess = bSuccess;
    PresentResultAudio(bSuccess);
    PresentedCampaignOperation =
        EEchoesOperationMode::CampaignUnburiedRoad;
    CampaignConsequence = Consequence;
    RecordedCampaignConsequence = RecordedConsequence;
    CampaignCommitStatus = CommitStatus;
    FutureWellChoice = Consequence;
    PresentedMatchOutcome = bSuccess
        ? echoes::sim::MatchOutcome::Player0Victory
        : echoes::sim::MatchOutcome::Player1Victory;
    SetIgnoreMoveInput(true);
    SetIgnoreLookInput(true);
    FString ResultMessage =
        TEXT("MISSION FAILED — Oruun, the Waystone, the Listening Spine, the local Core, or the unburied route was lost. Press R to replay.");
    if (bSuccess)
    {
        ResultMessage = FString::Printf(
            TEXT("MISSION COMPLETE — Oruun recovered the missing shard beyond the inherited %s route."),
            *GetFutureWellChoiceLabel());
        if (CommitStatus == EEchoesCampaignCommitStatus::Added)
        {
            ResultMessage += TEXT(" Campaign ledger committed. Press R to replay.");
        }
        else if (CommitStatus == EEchoesCampaignCommitStatus::AlreadyRecorded)
        {
            ResultMessage += TEXT(" Campaign ledger already contains this recovery. Press R to replay.");
        }
        else
        {
            ResultMessage += TEXT(" Campaign progress was not saved. Press R to replay.");
        }
    }
    SetStatusMessage(ResultMessage, 3600.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_CAMPAIGN_RESULT_PRESENTED] mission=TheUnburiedRoad success=%s consequence=%u recordedConsequence=%u campaignStatus=%u keyboardRestart=true"),
        bSuccess ? TEXT("true") : TEXT("false"),
        static_cast<uint8>(Consequence),
        static_cast<uint8>(RecordedConsequence),
        static_cast<uint8>(CommitStatus));
}

void AEchoesPlayerController::NotifyTermsOfContinuanceFinished(
    bool bSuccess,
    echoes::sim::FutureWellChoice Consequence,
    echoes::sim::FutureWellChoice RecordedConsequence,
    EEchoesCampaignCommitStatus CommitStatus)
{
    ClearSelection();
    bControlGroupAssignmentArmed = false;
    bSelectionButtonDown = false;
    bTitleScreenVisible = false;
    bMissionBriefingVisible = false;
    bPauseMenuVisible = false;
    bTechnologyPanelVisible = false;
    bMatchResultVisible = true;
    bCampaignResult = true;
    bCampaignSuccess = bSuccess;
    PresentResultAudio(bSuccess);
    PresentedCampaignOperation =
        EEchoesOperationMode::CampaignTermsOfContinuance;
    CampaignConsequence = Consequence;
    RecordedCampaignConsequence = RecordedConsequence;
    CampaignCommitStatus = CommitStatus;
    FutureWellChoice = Consequence;
    PresentedMatchOutcome = bSuccess
        ? echoes::sim::MatchOutcome::Player0Victory
        : echoes::sim::MatchOutcome::Player1Victory;
    SetIgnoreMoveInput(true);
    SetIgnoreLookInput(true);
    FString ResultMessage =
        TEXT("MISSION FAILED — a witness, network, local Core, or the continuance window was lost. Press R to replay.");
    if (bSuccess)
    {
        ResultMessage = FString::Printf(
            TEXT("MISSION COMPLETE — both witness proxies survived generic unresolved pressure and extracted under the inherited %s accord."),
            *GetFutureWellChoiceLabel());
        if (CommitStatus == EEchoesCampaignCommitStatus::Added)
        {
            ResultMessage += TEXT(" Campaign ledger committed. Press R to replay.");
        }
        else if (CommitStatus ==
                 EEchoesCampaignCommitStatus::AlreadyRecorded)
        {
            ResultMessage += TEXT(" Campaign ledger already contains this continuance result. Press R to replay.");
        }
        else
        {
            ResultMessage += TEXT(" Campaign progress was not saved. Press R to replay.");
        }
    }
    SetStatusMessage(ResultMessage, 3600.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_CAMPAIGN_RESULT_PRESENTED] mission=TermsOfContinuance success=%s consequence=%u recordedConsequence=%u campaignStatus=%u keyboardRestart=true"),
        bSuccess ? TEXT("true") : TEXT("false"),
        static_cast<uint8>(Consequence),
        static_cast<uint8>(RecordedConsequence),
        static_cast<uint8>(CommitStatus));
}

void AEchoesPlayerController::NotifyNamesWithoutBirthsFinished(
    bool bSuccess,
    echoes::sim::FutureWellChoice Consequence,
    echoes::sim::FutureWellChoice RecordedConsequence,
    EEchoesCampaignCommitStatus CommitStatus)
{
    ClearSelection();
    bControlGroupAssignmentArmed = false;
    bSelectionButtonDown = false;
    bTitleScreenVisible = false;
    bMissionBriefingVisible = false;
    bPauseMenuVisible = false;
    bTechnologyPanelVisible = false;
    bMatchResultVisible = true;
    bCampaignResult = true;
    bCampaignSuccess = bSuccess;
    PresentResultAudio(bSuccess);
    PresentedCampaignOperation =
        EEchoesOperationMode::CampaignNamesWithoutBirths;
    CampaignConsequence = Consequence;
    RecordedCampaignConsequence = RecordedConsequence;
    CampaignCommitStatus = CommitStatus;
    FutureWellChoice = Consequence;
    PresentedMatchOutcome = bSuccess
        ? echoes::sim::MatchOutcome::Player0Victory
        : echoes::sim::MatchOutcome::Player1Victory;
    SetIgnoreMoveInput(true);
    SetIgnoreLookInput(true);
    FString ResultMessage =
        TEXT("MISSION FAILED — Talar, the census archive, a civilian proxy, the local Core, or the operation was lost. Press R to replay.");
    if (bSuccess)
    {
        ResultMessage = FString::Printf(
            TEXT("MISSION COMPLETE — Talar extracted the %s census trace after the archive was powered and both civilian proxies reached shelter."),
            *GetFutureWellChoiceLabel());
        if (CommitStatus == EEchoesCampaignCommitStatus::Added)
        {
            ResultMessage += TEXT(" Campaign ledger committed. Press R to replay.");
        }
        else if (CommitStatus == EEchoesCampaignCommitStatus::AlreadyRecorded)
        {
            ResultMessage += TEXT(" Campaign ledger already contains this census recovery. Press R to replay.");
        }
        else
        {
            ResultMessage += TEXT(" Campaign progress was not saved. Press R to replay.");
        }
    }
    SetStatusMessage(ResultMessage, 3600.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_CAMPAIGN_RESULT_PRESENTED] mission=NamesWithoutBirths success=%s consequence=%u recordedConsequence=%u campaignStatus=%u keyboardRestart=true"),
        bSuccess ? TEXT("true") : TEXT("false"),
        static_cast<uint8>(Consequence),
        static_cast<uint8>(RecordedConsequence),
        static_cast<uint8>(CommitStatus));
}

void AEchoesPlayerController::NotifyShapeOfSilenceFinished(
    bool bSuccess,
    echoes::sim::FutureWellChoice Consequence,
    echoes::sim::FutureWellChoice RecordedConsequence,
    EEchoesCampaignCommitStatus CommitStatus)
{
    ClearSelection();
    bControlGroupAssignmentArmed = false;
    bSelectionButtonDown = false;
    bTitleScreenVisible = false;
    bMissionBriefingVisible = false;
    bPauseMenuVisible = false;
    bTechnologyPanelVisible = false;
    bMatchResultVisible = true;
    bCampaignResult = true;
    bCampaignSuccess = bSuccess;
    PresentResultAudio(bSuccess);
    PresentedCampaignOperation =
        EEchoesOperationMode::CampaignShapeOfSilence;
    CampaignConsequence = Consequence;
    RecordedCampaignConsequence = RecordedConsequence;
    CampaignCommitStatus = CommitStatus;
    FutureWellChoice = Consequence;
    PresentedMatchOutcome = bSuccess
        ? echoes::sim::MatchOutcome::Player0Victory
        : echoes::sim::MatchOutcome::Player1Victory;
    SetIgnoreMoveInput(true);
    SetIgnoreLookInput(true);
    FString ResultMessage =
        TEXT("MISSION FAILED — Oruun, a memory witness, the Waystone, the local Core, or the operation was lost. Press R to replay.");
    if (bSuccess)
    {
        ResultMessage = FString::Printf(
            TEXT("MISSION COMPLETE — the %s memory hollow corresponded with the recovered census absence. The record establishes correspondence, not cause or hidden authorship."),
            *GetFutureWellChoiceLabel());
        if (CommitStatus == EEchoesCampaignCommitStatus::Added)
        {
            ResultMessage += TEXT(" Campaign ledger committed. Press R to replay.");
        }
        else if (CommitStatus ==
                 EEchoesCampaignCommitStatus::AlreadyRecorded)
        {
            ResultMessage += TEXT(" Campaign ledger already contains this listening result. Press R to replay.");
        }
        else
        {
            ResultMessage += TEXT(" Campaign progress was not saved. Press R to replay.");
        }
    }
    SetStatusMessage(ResultMessage, 3600.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_CAMPAIGN_RESULT_PRESENTED] mission=TheShapeOfSilence success=%s consequence=%u recordedConsequence=%u campaignStatus=%u keyboardRestart=true claimBoundary=correspondenceOnly"),
        bSuccess ? TEXT("true") : TEXT("false"),
        static_cast<uint8>(Consequence),
        static_cast<uint8>(RecordedConsequence),
        static_cast<uint8>(CommitStatus));
}

void AEchoesPlayerController::NotifyShapeBesideUsFinished(
    bool bSuccess,
    echoes::sim::FutureWellChoice Consequence,
    echoes::sim::FutureWellChoice RecordedConsequence,
    EEchoesCampaignCommitStatus CommitStatus)
{
    ClearSelection();
    bControlGroupAssignmentArmed = false;
    bSelectionButtonDown = false;
    bTitleScreenVisible = false;
    bMissionBriefingVisible = false;
    bPauseMenuVisible = false;
    bTechnologyPanelVisible = false;
    bMatchResultVisible = true;
    bCampaignResult = true;
    bCampaignSuccess = bSuccess;
    PresentResultAudio(bSuccess);
    PresentedCampaignOperation =
        EEchoesOperationMode::CampaignShapeBesideUs;
    CampaignConsequence = Consequence;
    RecordedCampaignConsequence = RecordedConsequence;
    CampaignCommitStatus = CommitStatus;
    FutureWellChoice = Consequence;
    PresentedMatchOutcome = bSuccess
        ? echoes::sim::MatchOutcome::Player0Victory
        : echoes::sim::MatchOutcome::Player1Victory;
    SetIgnoreMoveInput(true);
    SetIgnoreLookInput(true);
    FString ResultMessage =
        TEXT("MISSION FAILED — Talar, a state witness, the local Core, or the operation was lost. Press R to replay.");
    if (bSuccess)
    {
        ResultMessage = FString::Printf(
            TEXT("MISSION COMPLETE — the %s overlap answered Talar's route with repeatable, actionable correspondence. The record establishes reciprocal contact, not a unified Choir identity, hidden authorship, or cause."),
            *GetFutureWellChoiceLabel());
        if (CommitStatus == EEchoesCampaignCommitStatus::Added)
        {
            ResultMessage += TEXT(" Campaign ledger committed. Press R to replay.");
        }
        else if (CommitStatus ==
                 EEchoesCampaignCommitStatus::AlreadyRecorded)
        {
            ResultMessage += TEXT(" Campaign ledger already contains this contact result. Press R to replay.");
        }
        else
        {
            ResultMessage += TEXT(" Campaign progress was not saved. Press R to replay.");
        }
    }
    SetStatusMessage(ResultMessage, 3600.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_CAMPAIGN_RESULT_PRESENTED] mission=TheShapeBesideUs success=%s consequence=%u recordedConsequence=%u campaignStatus=%u keyboardRestart=true claimBoundary=reciprocalContactOnly hollowChoirFactionImplemented=false"),
        bSuccess ? TEXT("true") : TEXT("false"),
        static_cast<uint8>(Consequence),
        static_cast<uint8>(RecordedConsequence),
        static_cast<uint8>(CommitStatus));
}

void AEchoesPlayerController::NotifyReserveAuthorityFinished(
    bool bSuccess,
    echoes::sim::FutureWellChoice Consequence,
    echoes::sim::FutureWellChoice RecordedConsequence,
    EEchoesCityDistrict DeferredDistrict,
    EEchoesCityDistrict RecordedDeferredDistrict,
    EEchoesCampaignCommitStatus CommitStatus)
{
    ClearSelection();
    bControlGroupAssignmentArmed = false;
    bSelectionButtonDown = false;
    bTitleScreenVisible = false;
    bMissionBriefingVisible = false;
    bPauseMenuVisible = false;
    bTechnologyPanelVisible = false;
    bMatchResultVisible = true;
    bCampaignResult = true;
    bCampaignSuccess = bSuccess;
    PresentResultAudio(bSuccess);
    PresentedCampaignOperation =
        EEchoesOperationMode::CampaignReserveAuthority;
    CampaignConsequence = Consequence;
    RecordedCampaignConsequence = RecordedConsequence;
    CampaignCommitStatus = CommitStatus;
    FutureWellChoice = Consequence;
    PresentedMatchOutcome = bSuccess
        ? echoes::sim::MatchOutcome::Player0Victory
        : echoes::sim::MatchOutcome::Player1Victory;
    SetIgnoreMoveInput(true);
    SetIgnoreLookInput(true);
    FString ResultMessage =
        TEXT("MISSION FAILED — Mara, a reserve district, the local Core, or the two-district allocation contract was lost. Press R to replay.");
    if (bSuccess)
    {
        ResultMessage = FString::Printf(
            TEXT("MISSION COMPLETE — two districts retain reserve power; %s remains intact but deferred. This records one local allocation, not wider city recovery or unmodeled civilian survival."),
            FEchoesCityReserveMissionModel::DistrictDisplayName(
                DeferredDistrict));
        if (CommitStatus == EEchoesCampaignCommitStatus::Added)
        {
            ResultMessage += TEXT(" Campaign ledger committed. Press R to replay.");
        }
        else if (CommitStatus ==
                 EEchoesCampaignCommitStatus::AlreadyRecorded)
        {
            ResultMessage += TEXT(" Campaign ledger already contains this allocation. Press R to replay.");
        }
        else if (CommitStatus == EEchoesCampaignCommitStatus::ReplayConflict)
        {
            ResultMessage += FString::Printf(
                TEXT(" The earlier irreversible record instead deferred %s; it was not rewritten. Press R to replay."),
                FEchoesCityReserveMissionModel::DistrictDisplayName(
                    RecordedDeferredDistrict));
        }
        else
        {
            ResultMessage += TEXT(" Campaign progress was not saved. Press R to replay.");
        }
    }
    SetStatusMessage(ResultMessage, 3600.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_CAMPAIGN_RESULT_PRESENTED] mission=ReserveAuthority success=%s consequence=%u recordedConsequence=%u deferred=%u recordedDeferred=%u campaignStatus=%u keyboardRestart=true claimBoundary=localAllocationOnly widerCityRestored=false civilianSurvivalUnmodeled=true"),
        bSuccess ? TEXT("true") : TEXT("false"),
        static_cast<uint8>(Consequence),
        static_cast<uint8>(RecordedConsequence),
        static_cast<uint8>(DeferredDistrict),
        static_cast<uint8>(RecordedDeferredDistrict),
        static_cast<uint8>(CommitStatus));
}

void AEchoesPlayerController::NotifyChoirAtLumeReachFinished(
    bool bSuccess,
    echoes::sim::FutureWellChoice Consequence,
    echoes::sim::FutureWellChoice RecordedConsequence,
    EEchoesCampaignCommitStatus CommitStatus)
{
    ClearSelection();
    bControlGroupAssignmentArmed = false;
    bSelectionButtonDown = false;
    bTitleScreenVisible = false;
    bMissionBriefingVisible = false;
    bPauseMenuVisible = false;
    bTechnologyPanelVisible = false;
    bMatchResultVisible = true;
    bCampaignResult = true;
    bCampaignSuccess = bSuccess;
    PresentResultAudio(bSuccess);
    PresentedCampaignOperation =
        EEchoesOperationMode::CampaignChoirAtLumeReach;
    CampaignConsequence = Consequence;
    RecordedCampaignConsequence = RecordedConsequence;
    CampaignCommitStatus = CommitStatus;
    FutureWellChoice = Consequence;
    PresentedMatchOutcome = bSuccess
        ? echoes::sim::MatchOutcome::Player0Victory
        : echoes::sim::MatchOutcome::Player1Victory;
    SetIgnoreMoveInput(true);
    SetIgnoreLookInput(true);
    FString ResultMessage =
        TEXT("MISSION FAILED — Oruun, the Waystone, a committed Listening Spine, the local Core, the Lume Well, or the active Reshape exit window was lost. Press R to replay.");
    if (bSuccess)
    {
        ResultMessage = FString::Printf(
            TEXT("MISSION COMPLETE — the %s protocol resolved the public Lume Reach route after both Listening Spines held. This records one local contact operation and Well decision; it does not make the Choir playable, identify hidden authorship, or prove wider causation."),
            *GetFutureWellChoiceLabel());
        if (CommitStatus == EEchoesCampaignCommitStatus::Added)
        {
            ResultMessage += TEXT(" Campaign ledger committed. Press R to replay.");
        }
        else if (CommitStatus ==
                 EEchoesCampaignCommitStatus::AlreadyRecorded)
        {
            ResultMessage += TEXT(" Campaign ledger already contains this Lume Reach decision. Press R to replay.");
        }
        else if (CommitStatus == EEchoesCampaignCommitStatus::ReplayConflict)
        {
            const TCHAR* RecordedLabel =
                RecordedConsequence ==
                        echoes::sim::FutureWellChoice::Harvest
                    ? TEXT("Harvest")
                : RecordedConsequence ==
                        echoes::sim::FutureWellChoice::Preserve
                    ? TEXT("Preserve")
                : RecordedConsequence ==
                        echoes::sim::FutureWellChoice::Reshape
                    ? TEXT("Reshape")
                    : TEXT("Dormant");
            ResultMessage += FString::Printf(
                TEXT(" The earlier irreversible record retains %s; it was not rewritten. Press R to replay."),
                RecordedLabel);
        }
        else
        {
            ResultMessage += TEXT(" Campaign progress was not saved. Press R to replay.");
        }
    }
    SetStatusMessage(ResultMessage, 3600.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_CAMPAIGN_RESULT_PRESENTED] mission=ChoirAtLumeReach success=%s consequence=%u recordedConsequence=%u campaignStatus=%u keyboardRestart=true claimBoundary=localContactOperationOnly maraPresence=liaisonOnly choirPresence=nonPlayablePublicContact mixedFactionCommand=false hiddenAttribution=false causationClaim=false"),
        bSuccess ? TEXT("true") : TEXT("false"),
        static_cast<uint8>(Consequence),
        static_cast<uint8>(RecordedConsequence),
        static_cast<uint8>(CommitStatus));
}

void AEchoesPlayerController::NotifyNoNeutralLedgerFinished(
    bool bSuccess,
    echoes::sim::FutureWellChoice Consequence,
    echoes::sim::FutureWellChoice RecordedConsequence,
    EEchoesCampaignCommitStatus CommitStatus)
{
    ClearSelection();
    bControlGroupAssignmentArmed = false;
    bSelectionButtonDown = false;
    bTitleScreenVisible = false;
    bMissionBriefingVisible = false;
    bPauseMenuVisible = false;
    bTechnologyPanelVisible = false;
    bMatchResultVisible = true;
    bCampaignResult = true;
    bCampaignSuccess = bSuccess;
    PresentResultAudio(bSuccess);
    PresentedCampaignOperation =
        EEchoesOperationMode::CampaignNoNeutralLedger;
    CampaignConsequence = Consequence;
    RecordedCampaignConsequence = RecordedConsequence;
    CampaignCommitStatus = CommitStatus;
    FutureWellChoice = Consequence;
    PresentedMatchOutcome = bSuccess
        ? echoes::sim::MatchOutcome::Player0Victory
        : echoes::sim::MatchOutcome::Player1Victory;
    SetIgnoreMoveInput(true);
    SetIgnoreLookInput(true);

    FString ResultMessage =
        TEXT("MISSION FAILED — Oruun, the ledger witness, the Waystone, the local Core, the Lume Well, or the active Reshape rally window was lost. No coalition record was committed. Press R to replay.");
    if (bSuccess)
    {
        ResultMessage = FString::Printf(
            TEXT("MISSION COMPLETE — the inherited route, both contributing district systems, and both public evidence channels now support the recorded %s protocol. This records one local coalition rally; it does not create mixed-faction command, identify hidden authorship, establish casualty counts, or prove wider causation."),
            *GetFutureWellChoiceLabel());
        if (CommitStatus == EEchoesCampaignCommitStatus::Added)
        {
            ResultMessage +=
                TEXT(" Campaign ledger committed. Press R to replay.");
        }
        else if (CommitStatus ==
                 EEchoesCampaignCommitStatus::AlreadyRecorded)
        {
            ResultMessage +=
                TEXT(" Campaign ledger already contains this coalition record. Press R to replay.");
        }
        else if (CommitStatus ==
                 EEchoesCampaignCommitStatus::ReplayConflict)
        {
            const TCHAR* RecordedLabel =
                RecordedConsequence ==
                        echoes::sim::FutureWellChoice::Harvest
                    ? TEXT("Harvest")
                : RecordedConsequence ==
                        echoes::sim::FutureWellChoice::Preserve
                    ? TEXT("Preserve")
                : RecordedConsequence ==
                        echoes::sim::FutureWellChoice::Reshape
                    ? TEXT("Reshape")
                    : TEXT("Dormant");
            ResultMessage += FString::Printf(
                TEXT(" The earlier irreversible record retains %s; it was not rewritten. Press R to replay."),
                RecordedLabel);
        }
        else
        {
            ResultMessage +=
                TEXT(" Campaign progress was not saved. Press R to replay.");
        }
    }
    SetStatusMessage(ResultMessage, 3600.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_CAMPAIGN_RESULT_PRESENTED] mission=NoNeutralLedger success=%s consequence=%u recordedConsequence=%u campaignStatus=%u keyboardRestart=true claimBoundary=localCoalitionRallyOnly commandAuthority=Kharuun meridianInterfaces=publicNonCommandable choirPresence=publicNonCommandable mixedFactionCommand=false hiddenTrustScore=false hiddenAttribution=false casualtyClaim=false causationClaim=false"),
        bSuccess ? TEXT("true") : TEXT("false"),
        static_cast<uint8>(Consequence),
        static_cast<uint8>(RecordedConsequence),
        static_cast<uint8>(CommitStatus));
}

void AEchoesPlayerController::NotifyFutureThatWonFinished(
    bool bSuccess,
    echoes::sim::FutureWellChoice Consequence,
    echoes::sim::FutureWellChoice RecordedConsequence,
    EEchoesCampaignCommitStatus CommitStatus)
{
    ClearSelection();
    bControlGroupAssignmentArmed = false;
    bSelectionButtonDown = false;
    bTitleScreenVisible = false;
    bMissionBriefingVisible = false;
    bPauseMenuVisible = false;
    bTechnologyPanelVisible = false;
    bMatchResultVisible = true;
    bCampaignResult = true;
    bCampaignSuccess = bSuccess;
    PresentResultAudio(bSuccess);
    PresentedCampaignOperation =
        EEchoesOperationMode::CampaignFutureThatWon;
    CampaignConsequence = Consequence;
    RecordedCampaignConsequence = RecordedConsequence;
    CampaignCommitStatus = CommitStatus;
    FutureWellChoice = Consequence;
    PresentedMatchOutcome = bSuccess
        ? echoes::sim::MatchOutcome::Player0Victory
        : echoes::sim::MatchOutcome::Player1Victory;
    SetIgnoreMoveInput(true);
    SetIgnoreLookInput(true);

    FString ResultMessage =
        TEXT("MISSION FAILED — Oruun, the independent verifier, the local Core, the Future Well, or one of the public readback interfaces was lost before the bounded demonstration completed. No restoration record was committed. Press R to replay.");
    if (bSuccess)
    {
        ResultMessage = FString::Printf(
            TEXT("MISSION COMPLETE — the recorded %s protocol held through the bounded local stability window and both district readbacks were observed. This records one local protocol/readback result; it does not establish civilian counts, population restoration, a permanent future, trust, consent, or ethical justification."),
            *GetFutureWellChoiceLabel());
        if (CommitStatus == EEchoesCampaignCommitStatus::Added)
        {
            ResultMessage +=
                TEXT(" Campaign ledger committed. Press R to replay.");
        }
        else if (CommitStatus ==
                 EEchoesCampaignCommitStatus::AlreadyRecorded)
        {
            ResultMessage +=
                TEXT(" Campaign ledger already contains this bounded demonstration record. Press R to replay.");
        }
        else if (CommitStatus ==
                 EEchoesCampaignCommitStatus::ReplayConflict)
        {
            const TCHAR* RecordedLabel =
                RecordedConsequence ==
                        echoes::sim::FutureWellChoice::Harvest
                    ? TEXT("Harvest")
                : RecordedConsequence ==
                        echoes::sim::FutureWellChoice::Preserve
                    ? TEXT("Preserve")
                : RecordedConsequence ==
                        echoes::sim::FutureWellChoice::Reshape
                    ? TEXT("Reshape")
                    : TEXT("Dormant");
            ResultMessage += FString::Printf(
                TEXT(" The earlier irreversible record retains %s; it was not rewritten. Press R to replay."),
                RecordedLabel);
        }
        else
        {
            ResultMessage +=
                TEXT(" Campaign progress was not saved. Press R to replay.");
        }
    }
    SetStatusMessage(ResultMessage, 3600.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_CAMPAIGN_RESULT_PRESENTED] mission=TheFutureThatWon success=%s consequence=%u recordedConsequence=%u campaignStatus=%u keyboardRestart=true claimBoundary=boundedLocalProtocolReadbackOnly commandAuthority=Kharuun rhysePresence=attributablePublicApparatusOnly mixedFactionCommand=false civilianCountsUnmodeled=true populationRestorationUnproven=true permanentFutureUnproven=true trustUnproven=true consentUnproven=true ethicalJustificationUnproven=true"),
        bSuccess ? TEXT("true") : TEXT("false"),
        static_cast<uint8>(Consequence),
        static_cast<uint8>(RecordedConsequence),
        static_cast<uint8>(CommitStatus));
}

void AEchoesPlayerController::NotifyAssemblyOfTheMissingFinished(
    bool bSuccess,
    echoes::sim::FutureWellChoice Consequence,
    echoes::sim::FutureWellChoice RecordedConsequence,
    EEchoesCampaignCommitStatus CommitStatus)
{
    ClearSelection();
    bControlGroupAssignmentArmed = false;
    bSelectionButtonDown = false;
    bTitleScreenVisible = false;
    bMissionBriefingVisible = false;
    bPauseMenuVisible = false;
    bTechnologyPanelVisible = false;
    bMatchResultVisible = true;
    bCampaignResult = true;
    bCampaignSuccess = bSuccess;
    PresentResultAudio(bSuccess);
    PresentedCampaignOperation =
        EEchoesOperationMode::CampaignAssemblyOfTheMissing;
    CampaignConsequence = Consequence;
    RecordedCampaignConsequence = RecordedConsequence;
    CampaignCommitStatus = CommitStatus;
    FutureWellChoice = Consequence;
    PresentedMatchOutcome = bSuccess
        ? echoes::sim::MatchOutcome::Player0Victory
        : echoes::sim::MatchOutcome::Player1Victory;
    SetIgnoreMoveInput(true);
    SetIgnoreLookInput(true);

    FString ResultMessage =
        TEXT("MISSION FAILED — Oruun, the independent verifier, the local Core, or one of the neutral public record interfaces was lost before the bounded assembly completed. No public-assembly record was committed. Press R to replay.");
    if (bSuccess)
    {
        ResultMessage = FString::Printf(
            TEXT("MISSION COMPLETE — the separate Meridian and Kharuun public record interfaces were read, the neutral Crownfall index was linked, and both independent assembly witness sites were observed under the recorded %s protocol context. This records one bounded public assembly only; it does not assign responsibility, establish hidden authorship, prove trust or consent, count civilians, or create mixed-faction command."),
            *GetFutureWellChoiceLabel());
        if (CommitStatus == EEchoesCampaignCommitStatus::Added)
        {
            ResultMessage +=
                TEXT(" Campaign ledger committed. Press R to replay.");
        }
        else if (CommitStatus ==
                 EEchoesCampaignCommitStatus::AlreadyRecorded)
        {
            ResultMessage +=
                TEXT(" Campaign ledger already contains this public-assembly record. Press R to replay.");
        }
        else if (CommitStatus ==
                 EEchoesCampaignCommitStatus::ReplayConflict)
        {
            const TCHAR* RecordedLabel =
                RecordedConsequence ==
                        echoes::sim::FutureWellChoice::Harvest
                    ? TEXT("Harvest")
                : RecordedConsequence ==
                        echoes::sim::FutureWellChoice::Preserve
                    ? TEXT("Preserve")
                : RecordedConsequence ==
                        echoes::sim::FutureWellChoice::Reshape
                    ? TEXT("Reshape")
                    : TEXT("Dormant");
            ResultMessage += FString::Printf(
                TEXT(" The earlier irreversible record retains %s; it was not rewritten. Press R to replay."),
                RecordedLabel);
        }
        else
        {
            ResultMessage +=
                TEXT(" Campaign progress was not saved. Press R to replay.");
        }
    }
    SetStatusMessage(ResultMessage, 3600.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_CAMPAIGN_RESULT_PRESENTED] mission=AssemblyOfTheMissing success=%s consequence=%u recordedConsequence=%u campaignStatus=%u keyboardRestart=true claimBoundary=boundedPublicRecordAssemblyOnly commandAuthority=Kharuun publicInterfaces=neutralNonCommandable mixedFactionCommand=false responsibilityUnassigned=true hiddenAuthorshipUnproven=true trustUnproven=true consentUnproven=true civilianStateUnmodeled=true cryptographicAuthenticityUnproven=true"),
        bSuccess ? TEXT("true") : TEXT("false"),
        static_cast<uint8>(Consequence),
        static_cast<uint8>(RecordedConsequence),
        static_cast<uint8>(CommitStatus));
}

void AEchoesPlayerController::NotifySeveralVoicesOneCommandFinished(
    bool bSuccess,
    echoes::sim::FutureWellChoice Consequence,
    echoes::sim::FutureWellChoice RecordedConsequence,
    EEchoesCampaignCommitStatus CommitStatus)
{
    ClearSelection();
    bControlGroupAssignmentArmed = false;
    bSelectionButtonDown = false;
    bTitleScreenVisible = false;
    bMissionBriefingVisible = false;
    bPauseMenuVisible = false;
    bTechnologyPanelVisible = false;
    bMatchResultVisible = true;
    bCampaignResult = true;
    bCampaignSuccess = bSuccess;
    PresentResultAudio(bSuccess);
    PresentedCampaignOperation =
        EEchoesOperationMode::CampaignSeveralVoicesOneCommand;
    CampaignConsequence = Consequence;
    RecordedCampaignConsequence = RecordedConsequence;
    CampaignCommitStatus = CommitStatus;
    FutureWellChoice = Consequence;
    PresentedMatchOutcome = bSuccess
        ? echoes::sim::MatchOutcome::Player0Victory
        : echoes::sim::MatchOutcome::Player1Victory;
    SetIgnoreMoveInput(true);
    SetIgnoreLookInput(true);

    FString ResultMessage =
        TEXT("MISSION FAILED — the local Core, Neme, a protected voice, or the Research Loom was lost, the match ended, or the Phase Anchor was raised outside the required identity contract. No Choir-command record was committed. Press R to replay.");
    if (bSuccess)
    {
        ResultMessage = FString::Printf(
            TEXT("MISSION COMPLETE — under the inherited %s protocol context, one protected voice resolved to Possible, one remained Manifest, Neme held command at the inherited site, and the Phase Anchor sustained both incompatible capabilities through the full crisis window. This completes one bounded player-commanded Hollow Choir perspective; it does not decide the Choir's final fate or establish campaign balance."),
            *GetFutureWellChoiceLabel());
        if (CommitStatus == EEchoesCampaignCommitStatus::Added)
        {
            ResultMessage +=
                TEXT(" Campaign ledger committed. Press R to replay.");
        }
        else if (CommitStatus ==
                 EEchoesCampaignCommitStatus::AlreadyRecorded)
        {
            ResultMessage +=
                TEXT(" Campaign ledger already contains this Choir-command record. Press R to replay.");
        }
        else if (CommitStatus ==
                 EEchoesCampaignCommitStatus::ReplayConflict)
        {
            const TCHAR* RecordedLabel =
                RecordedConsequence ==
                        echoes::sim::FutureWellChoice::Harvest
                    ? TEXT("Harvest")
                : RecordedConsequence ==
                        echoes::sim::FutureWellChoice::Preserve
                    ? TEXT("Preserve")
                : RecordedConsequence ==
                        echoes::sim::FutureWellChoice::Reshape
                    ? TEXT("Reshape")
                    : TEXT("Dormant");
            ResultMessage += FString::Printf(
                TEXT(" The earlier irreversible record retains %s; it was not rewritten. Press R to replay."),
                RecordedLabel);
        }
        else
        {
            ResultMessage +=
                TEXT(" Campaign progress was not saved. Press R to replay.");
        }
    }
    SetStatusMessage(ResultMessage, 3600.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_CAMPAIGN_RESULT_PRESENTED] mission=SeveralVoicesOneCommand success=%s consequence=%u recordedConsequence=%u campaignStatus=%u keyboardRestart=true claimBoundary=boundedChoirCommandCrisisOnly commandAuthority=HollowChoir identityTimers=visibleDeterministic crisisWindowSeconds=8 finalChoirFateDecided=false campaignBalanceUnproven=true ordinaryHumanCompletionUnproven=true releaseReadinessUnproven=true"),
        bSuccess ? TEXT("true") : TEXT("false"),
        static_cast<uint8>(Consequence),
        static_cast<uint8>(RecordedConsequence),
        static_cast<uint8>(CommitStatus));
}

void AEchoesPlayerController::NotifyBrokenSunFinished(
    bool bSuccess,
    EEchoesFinalResolution Resolution,
    EEchoesFinalResolution RecordedResolution,
    EEchoesCampaignCommitStatus CommitStatus)
{
    ClearSelection();
    bControlGroupAssignmentArmed = false;
    bSelectionButtonDown = false;
    bTitleScreenVisible = false;
    bMissionBriefingVisible = false;
    bPauseMenuVisible = false;
    bTechnologyPanelVisible = false;
    bMatchResultVisible = true;
    bCampaignResult = true;
    bCampaignSuccess = bSuccess;
    PresentEndingAudio(RecordedResolution, bSuccess);
    PresentedCampaignOperation =
        EEchoesOperationMode::CampaignTheBrokenSun;
    CampaignConsequence = echoes::sim::FutureWellChoice::Dormant;
    RecordedCampaignConsequence = echoes::sim::FutureWellChoice::Dormant;
    CampaignFinalResolution = Resolution;
    RecordedCampaignFinalResolution = RecordedResolution;
    CampaignCommitStatus = CommitStatus;
    PresentedMatchOutcome = bSuccess
        ? echoes::sim::MatchOutcome::Player0Victory
        : echoes::sim::MatchOutcome::Player1Victory;
    SetIgnoreMoveInput(true);
    SetIgnoreLookInput(true);

    FString ResultMessage =
        TEXT("MISSION FAILED — the local Core, a required command voice, one of the named witnesses, or an exact objective structure left the final contract. No campaign ending was committed. Press R to replay.");
    if (bSuccess)
    {
        ResultMessage = FString::Printf(
            TEXT("MISSION COMPLETE — %s held through its full deterministic resolution window. %s"),
            FEchoesBrokenSunMissionModel::ResolutionDisplayName(
                Resolution),
            FEchoesBrokenSunMissionModel::ResolutionCostSummary(
                Resolution));
        if (CommitStatus == EEchoesCampaignCommitStatus::Added)
        {
            ResultMessage +=
                TEXT(" Campaign ending committed to the ledger. Press R to replay the battle without rewriting it.");
        }
        else if (CommitStatus ==
                 EEchoesCampaignCommitStatus::AlreadyRecorded)
        {
            ResultMessage +=
                TEXT(" The campaign ledger already contains this ending. Press R to replay.");
        }
        else if (CommitStatus ==
                 EEchoesCampaignCommitStatus::ReplayConflict)
        {
            ResultMessage += FString::Printf(
                TEXT(" This was an alternate simulated outcome; the campaign ending remains %s and was not rewritten. Press R to replay."),
                FEchoesBrokenSunMissionModel::ResolutionDisplayName(
                    RecordedResolution));
        }
        else
        {
            ResultMessage +=
                TEXT(" The operation completed, but campaign storage did not commit an ending. Press R to replay.");
        }
    }
    SetStatusMessage(ResultMessage, 3600.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_CAMPAIGN_RESULT_PRESENTED] mission=TheBrokenSun success=%s resolution=%s recordedResolution=%s campaignStatus=%u keyboardRestart=true commandAuthority=HollowChoir mixedFactionCommand=false namedWitnesses=protectedNeutral finalOperationMechanics=bounded deterministicHold=true broadNarrativeConsequencesUnmodeled=true campaignBalanceUnproven=true ordinaryHumanCompletionUnproven=true releaseReadinessUnproven=true"),
        bSuccess ? TEXT("true") : TEXT("false"),
        FEchoesBrokenSunMissionModel::ResolutionStableName(Resolution),
        FEchoesBrokenSunMissionModel::ResolutionStableName(
            RecordedResolution),
        static_cast<uint8>(CommitStatus));
}

void AEchoesPlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);
#if !UE_BUILD_SHIPPING
    if (bPointerCombatGuardReviewActive)
    {
        RunPointerCombatGuardReviewStage(DeltaTime);
    }
#endif
    if (bSelectionButtonDown)
    {
        FVector2D PointerPosition = FVector2D::ZeroVector;
        if (ResolvePointerScreenPosition(PointerPosition))
        {
            SelectionCurrentScreenPosition = PointerPosition;
        }
    }
    PruneSelection();
    if (bControlGroupAssignmentArmed &&
        GetWorld() != nullptr &&
        GetWorld()->GetTimeSeconds() > ControlGroupAssignmentExpiresAt)
    {
        bControlGroupAssignmentArmed = false;
    }
}

bool AEchoesPlayerController::MoveReviewPointerToEntity(
    uint32 EntityId,
    const TCHAR* StageLabel)
{
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    AEchoesEntityView* View =
        Bridge != nullptr ? Bridge->FindEntityView(EntityId) : nullptr;
    if (View == nullptr)
    {
        return false;
    }

    FVector BoundsOrigin = FVector::ZeroVector;
    FVector BoundsExtent = FVector::ZeroVector;
    View->GetActorBounds(false, BoundsOrigin, BoundsExtent);
    FVector2D ScreenPosition = FVector2D::ZeroVector;
    if (!ProjectWorldLocationToScreen(BoundsOrigin, ScreenPosition, false))
    {
        return false;
    }
    FBox2D ProjectedBounds(ForceInit);
    for (int32 CornerIndex = 0; CornerIndex < 8; ++CornerIndex)
    {
        const FVector WorldCorner = BoundsOrigin + FVector(
            (CornerIndex & 1) != 0 ? BoundsExtent.X : -BoundsExtent.X,
            (CornerIndex & 2) != 0 ? BoundsExtent.Y : -BoundsExtent.Y,
            (CornerIndex & 4) != 0 ? BoundsExtent.Z : -BoundsExtent.Z);
        FVector2D ProjectedCorner = FVector2D::ZeroVector;
        if (!ProjectWorldLocationToScreen(WorldCorner, ProjectedCorner, false))
        {
            return false;
        }
        ProjectedBounds += ProjectedCorner;
    }

    int32 ViewportWidth = 0;
    int32 ViewportHeight = 0;
    GetViewportSize(ViewportWidth, ViewportHeight);
    if (ViewportWidth != PointerReviewExpectedViewport.X ||
        ViewportHeight != PointerReviewExpectedViewport.Y)
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_POINTER_REVIEW_VIEWPORT_MISMATCH] variant=%s expected=(%d,%d) actual=(%d,%d)"),
            *PointerReviewVariant,
            PointerReviewExpectedViewport.X,
            PointerReviewExpectedViewport.Y,
            ViewportWidth,
            ViewportHeight);
        return false;
    }
    if (ViewportWidth <= 0 || ViewportHeight <= 0 ||
        ScreenPosition.X < 0.0f || ScreenPosition.Y < 0.0f ||
        ScreenPosition.X >= static_cast<float>(ViewportWidth) ||
        ScreenPosition.Y >= static_cast<float>(ViewportHeight))
    {
        return false;
    }

    const FVector2D ViewportSize(
        static_cast<float>(ViewportWidth),
        static_cast<float>(ViewportHeight));
    const FEchoesHudLayout Layout = FEchoesHudLayout::Build(
        ViewportSize,
        PointerReviewHudScale,
        true);
    if (!Layout.IsBattlefieldBoxClear(ProjectedBounds, ViewportSize))
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_POINTER_REVIEW_OCCLUDED] variant=%s stage=%s entity=%u screen=(%.1f,%.1f) bounds=(%.1f,%.1f)-(%.1f,%.1f) viewport=(%d,%d) hudScale=%.2f"),
            *PointerReviewVariant,
            StageLabel,
            EntityId,
            ScreenPosition.X,
            ScreenPosition.Y,
            ProjectedBounds.Min.X,
            ProjectedBounds.Min.Y,
            ProjectedBounds.Max.X,
            ProjectedBounds.Max.Y,
            ViewportWidth,
            ViewportHeight,
            PointerReviewHudScale);
        return false;
    }

    SetMouseLocation(
        FMath::RoundToInt(ScreenPosition.X),
        FMath::RoundToInt(ScreenPosition.Y));
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_POINTER_REVIEW_COORDINATE] variant=%s stage=%s entity=%u screen=(%.1f,%.1f) bounds=(%.1f,%.1f)-(%.1f,%.1f) viewport=(%d,%d) hudScale=%.2f projectedFromLiveView=true fullBoundsVisible=true hudOcclusion=false"),
        *PointerReviewVariant,
        StageLabel,
        EntityId,
        ScreenPosition.X,
        ScreenPosition.Y,
        ProjectedBounds.Min.X,
        ProjectedBounds.Min.Y,
        ProjectedBounds.Max.X,
        ProjectedBounds.Max.Y,
        ViewportWidth,
        ViewportHeight,
        PointerReviewHudScale);
    return true;
}

void AEchoesPlayerController::FailPointerCombatGuardReview(
    const FString& Reason)
{
    bPointerCombatGuardReviewActive = false;
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge != nullptr)
    {
        Bridge->SetScenarioPaused(true);
    }
    SetStatusMessage(
        FString::Printf(TEXT("CONTROLLED POINTER REVIEW FAILED — %s"), *Reason),
        3600.0f);
    UE_LOG(
        LogEchoes,
        Error,
        TEXT("[ECHOES_POINTER_COMBAT_GUARD_REVIEW_FAILED] variant=%s stage=%d reason=%s exactScreenCoordinates=true nonOcclusionRequired=true controlledNonshipping=true"),
        *PointerReviewVariant,
        PointerReviewStage,
        *Reason);
}

void AEchoesPlayerController::RunPointerCombatGuardReviewStage(float DeltaTime)
{
#if !UE_BUILD_SHIPPING
    PointerReviewStageElapsedSeconds += DeltaTime;
    PointerReviewTotalElapsedSeconds += DeltaTime;
    if (PointerReviewTotalElapsedSeconds > 15.0f)
    {
        FailPointerCombatGuardReview(TEXT("TIMEOUT"));
        return;
    }

    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    const echoes::sim::Simulation* Simulation =
        Bridge != nullptr ? Bridge->GetSimulation() : nullptr;
    if (Bridge == nullptr || Simulation == nullptr || !Bridge->IsScenarioReady())
    {
        FailPointerCombatGuardReview(TEXT("SIM_NOT_READY"));
        return;
    }

    const auto Advance = [this](int32 NextStage)
    {
        PointerReviewStage = NextStage;
        PointerReviewStageElapsedSeconds = 0.0f;
    };

    if (PointerReviewStage == 0)
    {
        if (PointerReviewStageElapsedSeconds < 0.75f)
        {
            return;
        }
        for (const echoes::sim::Entity& Entity : Simulation->Entities())
        {
            if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
                Entity.type == echoes::sim::EntityType::HeavyUnit)
            {
                PointerReviewDefenderId = Entity.id;
            }
            else if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
                     Entity.type == echoes::sim::EntityType::Worker)
            {
                PointerReviewProtectedId = Entity.id;
            }
            else if (Entity.owner == UEchoesSimulationSubsystem::OpponentPlayerId &&
                     Entity.type == echoes::sim::EntityType::Soldier)
            {
                PointerReviewHostileId = Entity.id;
                PointerReviewInitialHostileHitPoints = Entity.hitPoints;
            }
        }
        if (PointerReviewDefenderId == 0 || PointerReviewProtectedId == 0 ||
            PointerReviewHostileId == 0)
        {
            FailPointerCombatGuardReview(TEXT("FIXTURE_ENTITIES_UNAVAILABLE"));
            return;
        }
        if (!MoveReviewPointerToEntity(
                PointerReviewDefenderId,
                TEXT("select_defender")))
        {
            FailPointerCombatGuardReview(TEXT("DEFENDER_PROJECTION_FAILED"));
            return;
        }
        Advance(1);
        return;
    }

    if (PointerReviewStage == 1)
    {
        if (PointerReviewStageElapsedSeconds < 0.15f)
        {
            return;
        }
        SelectionPressed();
        SelectionReleased();
        if (SelectedEntityIds.Num() != 1 ||
            SelectedEntityIds[0] != PointerReviewDefenderId)
        {
            FailPointerCombatGuardReview(TEXT("POINTER_SELECTION_REJECTED"));
            return;
        }
        if (!MoveReviewPointerToEntity(
                PointerReviewProtectedId,
                TEXT("guard_target")))
        {
            FailPointerCombatGuardReview(TEXT("GUARD_TARGET_PROJECTION_FAILED"));
            return;
        }
        Advance(2);
        return;
    }

    if (PointerReviewStage == 2)
    {
        if (PointerReviewStageElapsedSeconds < 0.15f)
        {
            return;
        }
        GuardAtCursor();
        Advance(3);
        return;
    }

    if (PointerReviewStage == 3)
    {
        const echoes::sim::Entity* Defender =
            Bridge->FindEntity(PointerReviewDefenderId);
        if (Defender == nullptr)
        {
            FailPointerCombatGuardReview(TEXT("DEFENDER_LOST"));
            return;
        }
        if (Defender->order.type != echoes::sim::OrderType::Guard ||
            Defender->order.target != PointerReviewProtectedId)
        {
            return;
        }
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_POINTER_GUARD_OBSERVED] defender=%u protected=%u order=Guard authoritativeState=true"),
            PointerReviewDefenderId,
            PointerReviewProtectedId);
        if (!MoveReviewPointerToEntity(
                PointerReviewHostileId,
                TEXT("direct_attack_target")))
        {
            FailPointerCombatGuardReview(TEXT("HOSTILE_PROJECTION_FAILED"));
            return;
        }
        Advance(4);
        return;
    }

    if (PointerReviewStage == 4)
    {
        if (PointerReviewStageElapsedSeconds < 0.15f)
        {
            return;
        }
        const echoes::sim::Entity* Hostile =
            Bridge->FindEntity(PointerReviewHostileId);
        if (Hostile == nullptr)
        {
            FailPointerCombatGuardReview(TEXT("HOSTILE_LOST_BEFORE_ATTACK"));
            return;
        }
        PointerReviewInitialHostileHitPoints = Hostile->hitPoints;
        ContextOrderPressed();
        Advance(5);
        return;
    }

    if (PointerReviewStage == 5)
    {
        const echoes::sim::Entity* Defender =
            Bridge->FindEntity(PointerReviewDefenderId);
        const echoes::sim::Entity* Hostile =
            Bridge->FindEntity(PointerReviewHostileId);
        const bool bAttackOrderObserved =
            Defender != nullptr &&
            Defender->order.type == echoes::sim::OrderType::Attack &&
            Defender->order.target == PointerReviewHostileId;
        const bool bDamageObserved =
            Hostile == nullptr ||
            Hostile->hitPoints < PointerReviewInitialHostileHitPoints;
        if (!bAttackOrderObserved && !bDamageObserved)
        {
            return;
        }
        if (!bDamageObserved)
        {
            return;
        }

        Bridge->SetScenarioPaused(true);
        bPointerCombatGuardReviewActive = false;
        const int32 FinalHitPoints = Hostile != nullptr ? Hostile->hitPoints : 0;
        SetStatusMessage(
            FString::Printf(
                TEXT("CONTROLLED REVIEW PASSED — exact-coordinate LMB selected Bulwark %u; J guarded Surveyor %u; RMB attacked Riftstalker %u (%d -> %d HP)."),
                PointerReviewDefenderId,
                PointerReviewProtectedId,
                PointerReviewHostileId,
                PointerReviewInitialHostileHitPoints,
                FinalHitPoints),
            3600.0f);
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_POINTER_COMBAT_GUARD_REVIEW_COMPLETE] variant=%s hudScale=%.2f defender=%u protected=%u hostile=%u initialHp=%d finalHp=%d selectedVia=LMB guardVia=J attackVia=RMB exactScreenCoordinates=true hudOcclusion=false controllerBindings=true authoritativeCommands=true authoritativeDamage=true osInjection=false unaidedHuman=false controlledNonshipping=true"),
            *PointerReviewVariant,
            PointerReviewHudScale,
            PointerReviewDefenderId,
            PointerReviewProtectedId,
            PointerReviewHostileId,
            PointerReviewInitialHostileHitPoints,
            FinalHitPoints);

        FString OutputPath;
        if (FParse::Value(
                FCommandLine::Get(),
                TEXT("EchoesPointerCombatGuardReviewOutput="),
                OutputPath) &&
            !OutputPath.IsEmpty())
        {
            FScreenshotRequest::RequestScreenshot(
                OutputPath,
                true,
                false,
                false,
                FIntRect(),
                true);
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_POINTER_COMBAT_GUARD_CAPTURE] variant=%s requested=true showUI=true output=%s"),
                *PointerReviewVariant,
                *OutputPath);
        }
    }
#endif
}

void AEchoesPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    check(InputComponent != nullptr);

    InputComponent->BindAction(
        TEXT("Select"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::SelectionPressed);
    InputComponent->BindAction(
        TEXT("Select"),
        IE_Released,
        this,
        &AEchoesPlayerController::SelectionReleased);
    InputComponent->BindAction(
        TEXT("ContextOrder"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::ContextOrderPressed);
    InputComponent->BindAction(
        TEXT("ChooseHarvest"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::ChooseHarvest);
    InputComponent->BindAction(
        TEXT("ChoosePreserve"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::ChoosePreserve);
    InputComponent->BindAction(
        TEXT("ChooseReshape"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::ChooseReshape);
    InputComponent->BindAction(
        TEXT("ChooseFinalRestoration"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::ChooseFinalRestoration);
    InputComponent->BindAction(
        TEXT("ChooseFinalStabilization"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::ChooseFinalStabilization);
    InputComponent->BindAction(
        TEXT("ChooseFinalExtinguishment"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::ChooseFinalExtinguishment);
    InputComponent->BindAction(
        TEXT("ChooseFinalEvolution"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::ChooseFinalEvolution);
    InputComponent->BindAction(
        TEXT("BuildBarracks"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::BuildBarracks);
    InputComponent->BindAction(
        TEXT("BuildDropoff"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::BuildDropoff);
    InputComponent->BindAction(
        TEXT("BuildUtility"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::BuildUtility);
    InputComponent->BindAction(
        TEXT("ProduceWorker"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::ProduceWorker);
    InputComponent->BindAction(
        TEXT("ProduceSoldier"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::ProduceSoldier);
    InputComponent->BindAction(
        TEXT("ProduceHeavy"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::ProduceHeavy);
    InputComponent->BindAction(
        TEXT("ProduceScout"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::ProduceScout);
    InputComponent->BindAction(
        TEXT("ResearchNext"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::ResearchNextTechnology);
    InputComponent->BindAction(
        TEXT("ToggleTechnologyPanel"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::ToggleTechnologyPanel);
    InputComponent->BindAction(
        TEXT("TechnologyFocusPrevious"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::FocusPreviousTechnologyTier);
    InputComponent->BindAction(
        TEXT("TechnologyFocusNext"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::FocusNextTechnologyTier);
    InputComponent->BindAction(
        TEXT("AttackMoveAtCursor"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::AttackMoveAtCursor);
    InputComponent->BindAction(
        TEXT("PatrolAtCursor"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::PatrolAtCursor);
    InputComponent->BindAction(
        TEXT("HoldSelected"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::HoldSelectedUnits);
    InputComponent->BindAction(
        TEXT("GuardAtCursor"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::GuardAtCursor);
    InputComponent->BindAction(
        TEXT("StopSelected"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::StopSelectedUnits);
    InputComponent->BindAction(
        TEXT("ToggleBulwarkDeployment"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::ToggleBulwarkDeploymentAtCursor);
    InputComponent->BindAction(
        TEXT("ActivateRelaySupply"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::ActivateRelaySupply);
    InputComponent->BindAction(
        TEXT("ToggleWaystoneRoot"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::ToggleWaystoneRoot);
    InputComponent->BindAction(
        TEXT("AdaptWarformCarapace"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::AdaptSelectedWarformsCarapace);
    InputComponent->BindAction(
        TEXT("AdaptWarformStriker"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::AdaptSelectedWarformsStriker);
    InputComponent->BindAction(
        TEXT("RaiseMineralCover"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::RaiseSelectedCairnbackCoverAtCursor);
    InputComponent->BindAction(
        TEXT("ReconcileChoirManifest"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::ReconcileSelectedChoirToManifest);
    InputComponent->BindAction(
        TEXT("ReconcileChoirPossible"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::ReconcileSelectedChoirToPossible);
    InputComponent->BindAction(
        TEXT("PauseScenario"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::TogglePauseMenu);
    InputComponent->BindAction(
        TEXT("RestartScenario"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::RestartScenario);
    InputComponent->BindAction(
        TEXT("QuickSaveScenario"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::QuickSaveScenario);
    InputComponent->BindAction(
        TEXT("QuickLoadScenario"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::QuickLoadScenario);
    const auto BindPressed = [this](
                                 const FName ActionName,
                                 void (AEchoesPlayerController::*Handler)())
    {
        InputComponent->BindAction(ActionName, IE_Pressed, this, Handler);
    };
    BindPressed(
        TEXT("ArmControlGroupAssignment"),
        &AEchoesPlayerController::ArmControlGroupAssignment);
    BindPressed(TEXT("CycleHudScale"), &AEchoesPlayerController::CycleHudScale);
    BindPressed(TEXT("ToggleHighContrast"), &AEchoesPlayerController::ToggleHighContrast);
    BindPressed(TEXT("ToggleReducedMotion"), &AEchoesPlayerController::ToggleReducedMotion);
    BindPressed(TEXT("ToggleReducedFlashing"), &AEchoesPlayerController::ToggleReducedFlashing);
    BindPressed(TEXT("ToggleEdgePan"), &AEchoesPlayerController::ToggleEdgePan);
    BindPressed(TEXT("DecreaseCameraPanSpeed"), &AEchoesPlayerController::DecreaseCameraPanSpeed);
    BindPressed(TEXT("IncreaseCameraPanSpeed"), &AEchoesPlayerController::IncreaseCameraPanSpeed);
    BindPressed(TEXT("DecreaseCameraZoomSpeed"), &AEchoesPlayerController::DecreaseCameraZoomSpeed);
    BindPressed(TEXT("IncreaseCameraZoomSpeed"), &AEchoesPlayerController::IncreaseCameraZoomSpeed);
    BindPressed(TEXT("CycleEffectsVolume"), &AEchoesPlayerController::CycleEffectsVolume);
    BindPressed(TEXT("ToggleReducedDynamicRange"), &AEchoesPlayerController::ToggleReducedDynamicRange);
    BindPressed(TEXT("ConfirmPrimaryAction"), &AEchoesPlayerController::ConfirmPrimaryAction);
    BindPressed(TEXT("SkirmishFocusPrevious"), &AEchoesPlayerController::FocusPreviousSkirmishSetting);
    BindPressed(TEXT("SkirmishFocusNext"), &AEchoesPlayerController::FocusNextSkirmishSetting);
    BindPressed(TEXT("SkirmishValuePrevious"), &AEchoesPlayerController::DecreaseSkirmishSetting);
    BindPressed(TEXT("SkirmishValueNext"), &AEchoesPlayerController::IncreaseSkirmishSetting);
    BindPressed(TEXT("CyclePlayableFaction"), &AEchoesPlayerController::CyclePlayableFaction);
    BindPressed(TEXT("CycleOperation"), &AEchoesPlayerController::CycleOperation);
    BindPressed(TEXT("ContinueCampaign"), &AEchoesPlayerController::ContinueCampaign);
    BindPressed(TEXT("RequestNewCampaign"), &AEchoesPlayerController::RequestNewCampaign);
    BindPressed(TEXT("LeaveOnlineMatch"), &AEchoesPlayerController::LeaveOnlineMatch);
    BindPressed(TEXT("RequestCampaignRestore"), &AEchoesPlayerController::RequestCampaignRestore);
    BindPressed(TEXT("CycleOwnedEntityPrevious"), &AEchoesPlayerController::CycleOwnedEntityPrevious);
    BindPressed(TEXT("SelectCombatForce"), &AEchoesPlayerController::SelectCombatForce);
    BindPressed(TEXT("OpenOnlineFrontDoor"), &AEchoesPlayerController::OpenOnlineFrontDoor);
    BindPressed(TEXT("CycleFormation"), &AEchoesPlayerController::CycleFormation);
    BindPressed(TEXT("ToggleKeyboardTargeting"), &AEchoesPlayerController::ToggleKeyboardTargeting);
    BindPressed(TEXT("KeyboardContextOrder"), &AEchoesPlayerController::KeyboardContextOrderPressed);
    BindPressed(TEXT("KeyboardTargetLeft"), &AEchoesPlayerController::NudgeKeyboardTargetLeft);
    BindPressed(TEXT("KeyboardTargetRight"), &AEchoesPlayerController::NudgeKeyboardTargetRight);
    BindPressed(TEXT("SnapKeyboardTargetToSelection"), &AEchoesPlayerController::SnapKeyboardTargetToSelection);
    BindPressed(TEXT("RecallControlGroup1"), &AEchoesPlayerController::RecallControlGroup1);
    BindPressed(TEXT("RecallControlGroup2"), &AEchoesPlayerController::RecallControlGroup2);
    BindPressed(TEXT("RecallControlGroup3"), &AEchoesPlayerController::RecallControlGroup3);
    BindPressed(TEXT("RecallControlGroup4"), &AEchoesPlayerController::RecallControlGroup4);
    BindPressed(TEXT("RecallControlGroup5"), &AEchoesPlayerController::RecallControlGroup5);
    BindPressed(TEXT("RecallControlGroup6"), &AEchoesPlayerController::RecallControlGroup6);
    BindPressed(TEXT("RecallControlGroup7"), &AEchoesPlayerController::RecallControlGroup7);
    BindPressed(TEXT("RecallControlGroup8"), &AEchoesPlayerController::RecallControlGroup8);
    BindPressed(TEXT("RecallControlGroup9"), &AEchoesPlayerController::RecallControlGroup9);
    BindPressed(TEXT("RecallControlGroup0"), &AEchoesPlayerController::RecallControlGroup0);
}

void AEchoesPlayerController::SelectionPressed()
{
    FVector2D PointerPosition = FVector2D::ZeroVector;
    FVector2D ViewportSize = FVector2D::ZeroVector;
    const bool bPointerAvailable =
        ResolvePointerScreenPosition(PointerPosition, &ViewportSize);
    if (bTechnologyPanelVisible)
    {
        if (bPointerAvailable)
        {
            (void)HandleTechnologyPanelPointer(PointerPosition);
        }
        return;
    }
    if (IsModalOverlayVisible())
    {
        if (bPointerAvailable &&
            ViewportSize.X > 0.0f && ViewportSize.Y > 0.0f)
        {
            const UEchoesGameUserSettings* Settings =
                UEchoesGameUserSettings::Get();
            (void)HandleModalOverlayPointer(
                PointerPosition,
                ViewportSize,
                Settings != nullptr ? Settings->GetHudScale() : 1.0f);
        }
        return;
    }
    if (!bPointerAvailable)
    {
        SetStatusMessage(TEXT("[CURSOR_UNAVAILABLE] Selection could not read the pointer position."));
        return;
    }
    SelectionStartScreenPosition = PointerPosition;
    SelectionCurrentScreenPosition = SelectionStartScreenPosition;
    bSelectionButtonDown = true;
}

void AEchoesPlayerController::SelectionReleased()
{
    if (IsModalOverlayVisible())
    {
        bSelectionButtonDown = false;
        return;
    }
    if (!bSelectionButtonDown)
    {
        return;
    }

    FVector2D PointerPosition = SelectionCurrentScreenPosition;
    if (ResolvePointerScreenPosition(PointerPosition))
    {
        SelectionCurrentScreenPosition = PointerPosition;
    }
    bSelectionButtonDown = false;

    const bool bAdditive = IsInputKeyDown(EKeys::LeftShift) ||
                           IsInputKeyDown(EKeys::RightShift);
    if (FVector2D::Distance(
            SelectionStartScreenPosition,
            SelectionCurrentScreenPosition) >= DragSelectionThresholdPixels)
    {
        SelectInScreenRectangle(bAdditive);
    }
    else
    {
        SelectAtCursor(bAdditive);
    }
}

void AEchoesPlayerController::SelectAtCursor(bool bAdditive)
{
    FHitResult HitResult;
    AEchoesEntityView* View = nullptr;
    if (TraceCursor(HitResult))
    {
        View = Cast<AEchoesEntityView>(HitResult.GetActor());
    }

    const uint8 SelectableOwner =
        GetNetMode() == NM_Client ? NetworkSeat
                                  : UEchoesSimulationSubsystem::LocalPlayerId;
    if (View == nullptr || View->GetOwnerPlayerId() != SelectableOwner)
    {
        if (!bAdditive)
        {
            ClearSelection();
        }
        return;
    }

    const uint32 EntityId = View->GetEntityId();
    if (!bAdditive)
    {
        ClearSelection();
    }

    if (bAdditive && SelectedEntityIds.Contains(EntityId))
    {
        SetEntitySelected(EntityId, false);
        SelectedEntityIds.Remove(EntityId);
    }
    else if (!SelectedEntityIds.Contains(EntityId))
    {
        SelectedEntityIds.Add(EntityId);
        SetEntitySelected(EntityId, true);
    }

    SetStatusMessage(
        FString::Printf(
            TEXT("Selected %d owned entit%s."),
            SelectedEntityIds.Num(),
            SelectedEntityIds.Num() == 1 ? TEXT("y") : TEXT("ies")),
        2.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_POINTER_SELECTION] screen=(%.1f,%.1f) entity=%u selected=%d additive=%s ownerScoped=true"),
        LastPointerScreenPosition.X,
        LastPointerScreenPosition.Y,
        EntityId,
        SelectedEntityIds.Num(),
        bAdditive ? TEXT("true") : TEXT("false"));
}

void AEchoesPlayerController::SelectInScreenRectangle(bool bAdditive)
{
    if (GetNetMode() == NM_Client)
    {
        const echoes::sim::net::ScopedViewKeyframe* NetworkView =
            GetNetworkScopedView();
        if (!IsNetworkClientControlActive() || NetworkView == nullptr)
        {
            SetStatusMessage(TEXT("[NETWORK_NOT_READY] Drag selection is unavailable."));
            return;
        }
        if (!bAdditive)
        {
            ClearSelection();
        }
        const float MinX = FMath::Min(
            SelectionStartScreenPosition.X,
            SelectionCurrentScreenPosition.X);
        const float MaxX = FMath::Max(
            SelectionStartScreenPosition.X,
            SelectionCurrentScreenPosition.X);
        const float MinY = FMath::Min(
            SelectionStartScreenPosition.Y,
            SelectionCurrentScreenPosition.Y);
        const float MaxY = FMath::Max(
            SelectionStartScreenPosition.Y,
            SelectionCurrentScreenPosition.Y);
        for (const echoes::sim::net::ScopedEntityState& Entity :
             NetworkView->entities)
        {
            if (Entity.owner != NetworkSeat)
            {
                continue;
            }
            const TWeakObjectPtr<AEchoesEntityView>* StoredView =
                NetworkEntityViews.Find(Entity.id);
            AEchoesEntityView* EntityView =
                StoredView != nullptr ? StoredView->Get() : nullptr;
            if (EntityView == nullptr)
            {
                continue;
            }
            FVector2D ScreenPosition;
            if (ProjectWorldLocationToScreen(
                    EntityView->GetActorLocation() +
                        FVector(0.0f, 0.0f, 60.0f),
                    ScreenPosition,
                    false) &&
                ScreenPosition.X >= MinX && ScreenPosition.X <= MaxX &&
                ScreenPosition.Y >= MinY && ScreenPosition.Y <= MaxY &&
                !SelectedEntityIds.Contains(Entity.id))
            {
                SelectedEntityIds.Add(Entity.id);
                EntityView->SetSelected(true);
            }
        }
        SetStatusMessage(
            FString::Printf(
                TEXT("ONLINE DRAG SELECT: %d owned entit%s."),
                SelectedEntityIds.Num(),
                SelectedEntityIds.Num() == 1 ? TEXT("y") : TEXT("ies")),
            2.0f);
        return;
    }

    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    const echoes::sim::Simulation* Sim =
        Bridge != nullptr ? Bridge->GetSimulation() : nullptr;
    if (Bridge == nullptr || Sim == nullptr)
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Drag selection is unavailable."));
        return;
    }

    if (!bAdditive)
    {
        ClearSelection();
    }

    const float MinX = FMath::Min(
        SelectionStartScreenPosition.X,
        SelectionCurrentScreenPosition.X);
    const float MaxX = FMath::Max(
        SelectionStartScreenPosition.X,
        SelectionCurrentScreenPosition.X);
    const float MinY = FMath::Min(
        SelectionStartScreenPosition.Y,
        SelectionCurrentScreenPosition.Y);
    const float MaxY = FMath::Max(
        SelectionStartScreenPosition.Y,
        SelectionCurrentScreenPosition.Y);

    for (const echoes::sim::Entity& Entity : Sim->Entities())
    {
        if (Entity.owner != UEchoesSimulationSubsystem::LocalPlayerId)
        {
            continue;
        }
        AEchoesEntityView* View = Bridge->FindEntityView(Entity.id);
        if (View == nullptr)
        {
            continue;
        }

        FVector2D ScreenPosition;
        if (ProjectWorldLocationToScreen(
                View->GetActorLocation() + FVector(0.0f, 0.0f, 60.0f),
                ScreenPosition,
                false) &&
            ScreenPosition.X >= MinX && ScreenPosition.X <= MaxX &&
            ScreenPosition.Y >= MinY && ScreenPosition.Y <= MaxY &&
            !SelectedEntityIds.Contains(Entity.id))
        {
            SelectedEntityIds.Add(Entity.id);
            View->SetSelected(true);
        }
    }

    SetStatusMessage(
        FString::Printf(
            TEXT("Drag-selected %d owned entit%s."),
            SelectedEntityIds.Num(),
            SelectedEntityIds.Num() == 1 ? TEXT("y") : TEXT("ies")),
        2.0f);
}

void AEchoesPlayerController::ContextOrderPressed()
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    PruneSelection();
    if (SelectedEntityIds.IsEmpty())
    {
        SetStatusMessage(TEXT("[NO_SELECTION] Select one or more owned units first."));
        return;
    }

    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (GetNetMode() == NM_Client)
    {
        if (!IsNetworkClientControlActive())
        {
            SetStatusMessage(TEXT("[NETWORK_NOT_READY] Orders cannot be issued."));
            return;
        }
    }
    else if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Orders cannot be issued."));
        return;
    }
    if (GetNetMode() != NM_Client &&
        Bridge->GetMatchOutcome() != echoes::sim::MatchOutcome::Ongoing)
    {
        SetStatusMessage(TEXT("[MATCH_FINISHED] Press R to restart."));
        return;
    }

    FHitResult HitResult;
    if (!TraceCursor(HitResult))
    {
        SetStatusMessage(TEXT("[NO_WORLD_HIT] Point at the battlefield or an entity."));
        return;
    }

    IssueContextOrder(HitResult, true);
}

void AEchoesPlayerController::KeyboardContextOrderPressed()
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    PruneSelection();
    if (SelectedEntityIds.IsEmpty())
    {
        SetStatusMessage(TEXT("[NO_SELECTION] Select one or more owned units first."));
        return;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (GetNetMode() == NM_Client)
    {
        if (!IsNetworkClientControlActive())
        {
            SetStatusMessage(TEXT("[NETWORK_NOT_READY] Orders cannot be issued."));
            return;
        }
    }
    else if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Orders cannot be issued."));
        return;
    }
    if (GetNetMode() != NM_Client &&
        Bridge->GetMatchOutcome() != echoes::sim::MatchOutcome::Ongoing)
    {
        SetStatusMessage(TEXT("[MATCH_FINISHED] Press R to restart."));
        return;
    }
    FHitResult HitResult;
    if (!TraceKeyboardTarget(HitResult))
    {
        SetStatusMessage(TEXT("[NO_KEYBOARD_TARGET] Move the reticle until it crosses the battlefield or an entity."));
        return;
    }
    if (!bKeyboardTargetingEnabled)
    {
        bKeyboardTargetingEnabled = true;
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_KEYBOARD_TARGET_MODE] enabled=true source=screen_reticle hiddenStateRead=false implicit=space"));
    }
    IssueContextOrder(HitResult, false);
}

void AEchoesPlayerController::IssueContextOrder(
    const FHitResult& HitResult,
    bool bPointerSource)
{
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    const AEchoesEntityView* TargetView =
        Cast<AEchoesEntityView>(HitResult.GetActor());
    if (GetNetMode() == NM_Client)
    {
        const echoes::sim::net::ScopedEntityState* TargetEntity =
            TargetView != nullptr
                ? FindNetworkEntity(TargetView->GetEntityId())
                : nullptr;
        echoes::sim::CommandType CommandType =
            echoes::sim::CommandType::Move;
        uint32 TargetId = 0;
        FVector Destination = HitResult.Location;
        if (TargetEntity != nullptr)
        {
            TargetId = TargetEntity->id;
            Destination = NetworkSimToWorld(TargetEntity->position);
            if (TargetEntity->type == echoes::sim::EntityType::ResourceNode)
            {
                CommandType = echoes::sim::CommandType::Gather;
            }
            else if (TargetEntity->type == echoes::sim::EntityType::FutureWell)
            {
                CommandType = echoes::sim::CommandType::FutureWell;
            }
            else if (TargetEntity->owner != echoes::sim::kNeutralPlayer &&
                     TargetEntity->owner != NetworkSeat)
            {
                CommandType = echoes::sim::CommandType::Attack;
            }
            else if (TargetEntity->owner == NetworkSeat &&
                     (TargetEntity->type ==
                          echoes::sim::EntityType::CommandCore ||
                      TargetEntity->type == echoes::sim::EntityType::Dropoff))
            {
                CommandType = echoes::sim::CommandType::Deliver;
            }
        }
        const TArray<FVector> FormationDestinations =
            BuildSelectedFormationDestinations(
                Destination, SelectedEntityIds.Num());
        TArray<echoes::sim::net::CommandIntent> Intents;
        Intents.Reserve(SelectedEntityIds.Num());
        for (int32 Index = 0; Index < SelectedEntityIds.Num(); ++Index)
        {
            const echoes::sim::net::ScopedEntityState* Actor =
                FindNetworkEntity(SelectedEntityIds[Index]);
            if (Actor == nullptr || Actor->owner != NetworkSeat)
            {
                continue;
            }
            echoes::sim::net::CommandIntent Intent{};
            Intent.type = CommandType;
            Intent.actor = Actor->id;
            Intent.target = TargetId;
            Intent.position = NetworkWorldToSim(
                CommandType == echoes::sim::CommandType::Move
                    ? FormationDestinations[Index]
                    : Destination);
            Intent.wellChoice = FutureWellChoice;
            Intents.Add(Intent);
        }
        const FString OrderLabel =
            CommandType == echoes::sim::CommandType::Move
                ? FString::Printf(TEXT("ONLINE MOVE / %s"), *GetFormationLabel())
                : FString::Printf(
                      TEXT("ONLINE %s"), *CommandLabel(CommandType));
        const EEchoesCommandMarkerType MarkerType =
            CommandType == echoes::sim::CommandType::Attack
                ? EEchoesCommandMarkerType::Attack
            : CommandType == echoes::sim::CommandType::Move
                ? EEchoesCommandMarkerType::Move
                : EEchoesCommandMarkerType::Interact;
        (void)SubmitNetworkCommandBatch(
            MoveTemp(Intents), OrderLabel, Destination, MarkerType);
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_NETWORK_CONTEXT_ORDER] source=%s command=%s target=%u selected=%d visibleHit=%s"),
            bPointerSource ? TEXT("pointer") : TEXT("keyboard_reticle"),
            *CommandLabel(CommandType),
            TargetId,
            SelectedEntityIds.Num(),
            TargetView != nullptr ? TEXT("true") : TEXT("false"));
        return;
    }
    if (Bridge == nullptr)
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Orders cannot be issued."));
        return;
    }
    SynchronizeBoundCampaignProtocol();

    const echoes::sim::Entity* TargetEntity =
        TargetView != nullptr
            ? Bridge->FindEntity(TargetView->GetEntityId())
            : nullptr;

    echoes::sim::CommandType CommandType = echoes::sim::CommandType::Move;
    uint32 TargetId = 0;
    FVector Destination = HitResult.Location;
    if (TargetEntity != nullptr)
    {
        TargetId = TargetEntity->id;
        Destination = Bridge->SimToWorld(TargetEntity->position);
        if (TargetEntity->type == echoes::sim::EntityType::ResourceNode)
        {
            CommandType = echoes::sim::CommandType::Gather;
        }
        else if (TargetEntity->type == echoes::sim::EntityType::FutureWell)
        {
            CommandType = echoes::sim::CommandType::FutureWell;
        }
        else if (TargetEntity->owner != echoes::sim::kNeutralPlayer &&
                 TargetEntity->owner != UEchoesSimulationSubsystem::LocalPlayerId)
        {
            CommandType = echoes::sim::CommandType::Attack;
        }
        else if (TargetEntity->owner == UEchoesSimulationSubsystem::LocalPlayerId &&
                 (TargetEntity->type == echoes::sim::EntityType::CommandCore ||
                  TargetEntity->type == echoes::sim::EntityType::Dropoff))
        {
            CommandType = echoes::sim::CommandType::Deliver;
        }
    }

    const int32 UnitCount = SelectedEntityIds.Num();
    const TArray<FVector> FormationDestinations =
        BuildSelectedFormationDestinations(Destination, UnitCount);
    int32 AcceptedCount = 0;
    int32 RejectedCount = 0;
    FString LastRejection;
    for (int32 Index = 0; Index < UnitCount; ++Index)
    {
        echoes::sim::CommandType ActorCommandType = CommandType;
        uint32 ActorTargetId = TargetId;
        const echoes::sim::Entity* ActorState =
            Bridge->FindEntity(SelectedEntityIds[Index]);
        if (CommandType == echoes::sim::CommandType::Deliver &&
            (ActorState == nullptr ||
             ActorState->type != echoes::sim::EntityType::Worker ||
             ActorState->cargo <= 0))
        {
            ActorCommandType = echoes::sim::CommandType::Move;
            ActorTargetId = 0;
        }

        FVector UnitDestination = Destination;
        if (ActorCommandType == echoes::sim::CommandType::Move)
        {
            UnitDestination = FormationDestinations[Index];
        }

        FString Feedback;
        if (Bridge->IssueCommand(
                ActorCommandType,
                SelectedEntityIds[Index],
                ActorTargetId,
                UnitDestination,
                FutureWellChoice,
                Feedback))
        {
            ++AcceptedCount;
        }
        else
        {
            ++RejectedCount;
            LastRejection = Feedback;
        }
    }

    if (AcceptedCount > 0)
    {
        const FString OrderLabel =
            CommandType == echoes::sim::CommandType::Deliver
                ? TEXT("CONTEXT MOVE / DELIVER")
                : CommandType == echoes::sim::CommandType::Move
                      ? FString::Printf(
                            TEXT("MOVE / %s"),
                            *GetFormationLabel())
                      : CommandLabel(CommandType);
        const FString RejectionSuffix =
            RejectedCount > 0
                ? FString::Printf(TEXT(", %d rejected."), RejectedCount)
                : TEXT(".");
        SetStatusMessage(
            FString::Printf(
                TEXT("%s: %d queued%s"),
                *OrderLabel,
                AcceptedCount,
                *RejectionSuffix));
        ShowAcceptedCommandMarker(
            Destination,
            CommandType == echoes::sim::CommandType::Attack
                ? EEchoesCommandMarkerType::Attack
                : CommandType == echoes::sim::CommandType::Move
                      ? EEchoesCommandMarkerType::Move
                      : EEchoesCommandMarkerType::Interact,
            AcceptedCount);
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_CONTEXT_ORDER_ACCEPTED] source=%s screen=(%.1f,%.1f) command=%s target=%u accepted=%d rejected=%d visibleHit=%s"),
            bPointerSource ? TEXT("pointer") : TEXT("keyboard_reticle"),
            bPointerSource ? LastPointerScreenPosition.X : -1.0f,
            bPointerSource ? LastPointerScreenPosition.Y : -1.0f,
            *CommandLabel(CommandType),
            TargetId,
            AcceptedCount,
            RejectedCount,
            TargetView != nullptr ? TEXT("true") : TEXT("false"));
    }
    else
    {
        SetStatusMessage(LastRejection.IsEmpty()
                             ? TEXT("[ORDER_REJECTED] No selected entity accepted the order.")
                             : LastRejection);
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_CONTEXT_ORDER_REJECTED] source=%s screen=(%.1f,%.1f) command=%s target=%u rejected=%d reason=%s"),
            bPointerSource ? TEXT("pointer") : TEXT("keyboard_reticle"),
            bPointerSource ? LastPointerScreenPosition.X : -1.0f,
            bPointerSource ? LastPointerScreenPosition.Y : -1.0f,
            *CommandLabel(CommandType),
            TargetId,
            RejectedCount,
            LastRejection.IsEmpty() ? TEXT("ORDER_REJECTED") : *LastRejection);
    }
}

void AEchoesPlayerController::SetEntitySelected(uint32 EntityId, bool bSelected)
{
    if (GetNetMode() == NM_Client)
    {
        const TWeakObjectPtr<AEchoesEntityView>* StoredView =
            NetworkEntityViews.Find(EntityId);
        if (StoredView != nullptr)
        {
            if (AEchoesEntityView* View = StoredView->Get())
            {
                View->SetSelected(bSelected);
            }
        }
        return;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge != nullptr)
    {
        if (AEchoesEntityView* View = Bridge->FindEntityView(EntityId))
        {
            View->SetSelected(bSelected);
        }
    }
}

void AEchoesPlayerController::ClearSelection()
{
    for (const uint32 EntityId : SelectedEntityIds)
    {
        SetEntitySelected(EntityId, false);
    }
    SelectedEntityIds.Reset();
}

bool AEchoesPlayerController::SetControlGroup(
    int32 GroupIndex,
    const TArray<uint32>& EntityIds,
    FString& OutFeedback)
{
    OutFeedback.Reset();
    if (GroupIndex < 0 || GroupIndex >= ControlGroupCount)
    {
        OutFeedback = TEXT("[GROUP_INDEX_INVALID] Control group must be between 0 and 9.");
        return false;
    }
    if (EntityIds.IsEmpty())
    {
        ControlGroups[GroupIndex].Reset();
        OutFeedback = FString::Printf(
            TEXT("CONTROL GROUP %d CLEARED."),
            ControlGroupDisplayNumber(GroupIndex));
        return true;
    }

    const UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    TArray<uint32> ValidIds;
    for (const uint32 EntityId : EntityIds)
    {
        const echoes::sim::net::ScopedEntityState* NetworkEntity =
            GetNetMode() == NM_Client ? FindNetworkEntity(EntityId) : nullptr;
        const echoes::sim::Entity* Entity =
            GetNetMode() != NM_Client && Bridge != nullptr
                ? Bridge->FindEntity(EntityId)
                : nullptr;
        if ((NetworkEntity != nullptr && NetworkEntity->owner == NetworkSeat) ||
            (Entity != nullptr &&
             Entity->owner == UEchoesSimulationSubsystem::LocalPlayerId))
        {
            ValidIds.AddUnique(EntityId);
        }
    }
    if (ValidIds.IsEmpty())
    {
        OutFeedback = TEXT("[GROUP_NO_VALID_ENTITIES] No live local entities were assigned.");
        return false;
    }
    ValidIds.Sort();
    ControlGroups[GroupIndex] = MoveTemp(ValidIds);
    OutFeedback = FString::Printf(
        TEXT("CONTROL GROUP %d: %d entit%s assigned."),
        ControlGroupDisplayNumber(GroupIndex),
        ControlGroups[GroupIndex].Num(),
        ControlGroups[GroupIndex].Num() == 1 ? TEXT("y") : TEXT("ies"));
    return true;
}

TArray<uint32> AEchoesPlayerController::GetValidControlGroup(
    int32 GroupIndex) const
{
    TArray<uint32> ValidIds;
    if (GroupIndex < 0 || GroupIndex >= ControlGroupCount)
    {
        return ValidIds;
    }
    const UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    for (const uint32 EntityId : ControlGroups[GroupIndex])
    {
        const echoes::sim::net::ScopedEntityState* NetworkEntity =
            GetNetMode() == NM_Client ? FindNetworkEntity(EntityId) : nullptr;
        const echoes::sim::Entity* Entity =
            GetNetMode() != NM_Client && Bridge != nullptr
                ? Bridge->FindEntity(EntityId)
                : nullptr;
        if ((NetworkEntity != nullptr && NetworkEntity->owner == NetworkSeat) ||
            (Entity != nullptr &&
             Entity->owner == UEchoesSimulationSubsystem::LocalPlayerId))
        {
            ValidIds.Add(EntityId);
        }
    }
    return ValidIds;
}

int32 AEchoesPlayerController::ControlGroupDisplayNumber(int32 GroupIndex)
{
    return GroupIndex == ControlGroupCount - 1 ? 0 : GroupIndex + 1;
}

void AEchoesPlayerController::ClearControlGroups()
{
    for (TArray<uint32>& Group : ControlGroups)
    {
        Group.Reset();
    }
}

void AEchoesPlayerController::AssignControlGroupFromSelection(int32 GroupIndex)
{
    PruneSelection();
    FString Feedback;
    SetControlGroup(GroupIndex, SelectedEntityIds, Feedback);
    SetStatusMessage(Feedback);
}

void AEchoesPlayerController::ArmControlGroupAssignment()
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    bControlGroupAssignmentArmed = true;
    ControlGroupAssignmentExpiresAt =
        GetWorld() != nullptr ? GetWorld()->GetTimeSeconds() + 5.0 : 5.0;
    SetStatusMessage(
        TEXT("GROUP ASSIGNMENT ARMED — press 1-0 within five seconds."),
        5.0f);
}

void AEchoesPlayerController::RecallControlGroup(int32 GroupIndex)
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    if (bControlGroupAssignmentArmed)
    {
        bControlGroupAssignmentArmed = false;
        AssignControlGroupFromSelection(GroupIndex);
        return;
    }
    TArray<uint32> ValidIds = GetValidControlGroup(GroupIndex);
    if (ValidIds.IsEmpty())
    {
        ControlGroups[GroupIndex].Reset();
        SetStatusMessage(FString::Printf(
            TEXT("[GROUP_EMPTY] Control group %d has no live entities."),
            ControlGroupDisplayNumber(GroupIndex)));
        return;
    }
    ControlGroups[GroupIndex] = ValidIds;
    ClearSelection();
    for (const uint32 EntityId : ValidIds)
    {
        SelectedEntityIds.Add(EntityId);
        SetEntitySelected(EntityId, true);
    }
    SetStatusMessage(FString::Printf(
        TEXT("CONTROL GROUP %d: %d entit%s selected."),
        ControlGroupDisplayNumber(GroupIndex),
        ValidIds.Num(),
        ValidIds.Num() == 1 ? TEXT("y") : TEXT("ies")));
}

#define DEFINE_CONTROL_GROUP_HANDLER(DisplayNumber, GroupIndex)              \
    void AEchoesPlayerController::RecallControlGroup##DisplayNumber()         \
    {                                                                         \
        RecallControlGroup(GroupIndex);                                        \
    }

DEFINE_CONTROL_GROUP_HANDLER(1, 0)
DEFINE_CONTROL_GROUP_HANDLER(2, 1)
DEFINE_CONTROL_GROUP_HANDLER(3, 2)
DEFINE_CONTROL_GROUP_HANDLER(4, 3)
DEFINE_CONTROL_GROUP_HANDLER(5, 4)
DEFINE_CONTROL_GROUP_HANDLER(6, 5)
DEFINE_CONTROL_GROUP_HANDLER(7, 6)
DEFINE_CONTROL_GROUP_HANDLER(8, 7)
DEFINE_CONTROL_GROUP_HANDLER(9, 8)
DEFINE_CONTROL_GROUP_HANDLER(0, 9)

#undef DEFINE_CONTROL_GROUP_HANDLER

void AEchoesPlayerController::PruneSelection()
{
    const UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    for (int32 Index = SelectedEntityIds.Num() - 1; Index >= 0; --Index)
    {
        const echoes::sim::net::ScopedEntityState* NetworkEntity =
            GetNetMode() == NM_Client
                ? FindNetworkEntity(SelectedEntityIds[Index])
                : nullptr;
        const echoes::sim::Entity* Entity =
            GetNetMode() != NM_Client && Bridge != nullptr
                ? Bridge->FindEntity(SelectedEntityIds[Index])
                : nullptr;
        const bool bValid = GetNetMode() == NM_Client
            ? NetworkEntity != nullptr && NetworkEntity->owner == NetworkSeat
            : Entity != nullptr &&
                  Entity->owner == UEchoesSimulationSubsystem::LocalPlayerId;
        if (!bValid)
        {
            SelectedEntityIds.RemoveAtSwap(Index, 1, EAllowShrinking::No);
        }
    }
}

bool AEchoesPlayerController::TraceCursor(FHitResult& OutHitResult)
{
    if (!ResolvePointerScreenPosition(LastPointerScreenPosition))
    {
        return false;
    }
    return GetHitResultAtScreenPosition(
        LastPointerScreenPosition,
        ECC_Visibility,
        true,
        OutHitResult);
}

bool AEchoesPlayerController::TraceKeyboardTarget(FHitResult& OutHitResult)
{
    int32 ViewportWidth = 0;
    int32 ViewportHeight = 0;
    GetViewportSize(ViewportWidth, ViewportHeight);
    if (ViewportWidth <= 0 || ViewportHeight <= 0)
    {
        return false;
    }
    return GetHitResultAtScreenPosition(
        FVector2D(
            static_cast<float>(ViewportWidth) * 0.5f,
            static_cast<float>(ViewportHeight) * 0.5f) + KeyboardTargetOffset,
        ECC_Visibility,
        true,
        OutHitResult);
}

bool AEchoesPlayerController::TraceCommandTarget(FHitResult& OutHitResult)
{
    return bKeyboardTargetingEnabled
        ? TraceKeyboardTarget(OutHitResult)
        : TraceCursor(OutHitResult);
}

TArray<FVector> AEchoesPlayerController::BuildSelectedFormationDestinations(
    const FVector& Anchor,
    int32 UnitCount)
{
    const UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    FVector Centroid = FVector::ZeroVector;
    int32 PositionCount = 0;
    if (GetNetMode() == NM_Client)
    {
        for (const uint32 EntityId : SelectedEntityIds)
        {
            const echoes::sim::net::ScopedEntityState* Entity =
                FindNetworkEntity(EntityId);
            if (Entity != nullptr)
            {
                Centroid += NetworkSimToWorld(Entity->position);
                ++PositionCount;
            }
        }
    }
    else if (Bridge != nullptr)
    {
        for (const uint32 EntityId : SelectedEntityIds)
        {
            const echoes::sim::Entity* Entity = Bridge->FindEntity(EntityId);
            if (Entity != nullptr)
            {
                Centroid += Bridge->SimToWorld(Entity->position);
                ++PositionCount;
            }
        }
    }
    if (PositionCount > 0)
    {
        Centroid /= static_cast<float>(PositionCount);
        FVector NewForward = Anchor - Centroid;
        NewForward.Z = 0.0f;
        if (NewForward.SizeSquared() > 1.0f)
        {
            LastFormationForward = NewForward.GetSafeNormal();
        }
    }
    return FEchoesFormationLayout::BuildDestinations(
        Anchor,
        LastFormationForward,
        UnitCount,
        CurrentFormation,
        FormationSpacingWorldUnits);
}

void AEchoesPlayerController::ChooseHarvest()
{
    SetFutureWellChoice(echoes::sim::FutureWellChoice::Harvest);
}

void AEchoesPlayerController::ChoosePreserve()
{
    SetFutureWellChoice(echoes::sim::FutureWellChoice::Preserve);
}

void AEchoesPlayerController::ChooseReshape()
{
    SetFutureWellChoice(echoes::sim::FutureWellChoice::Reshape);
}

void AEchoesPlayerController::ChooseFinalRestoration()
{
    ChooseFinalResolution(EEchoesFinalResolution::Restoration);
}

void AEchoesPlayerController::ChooseFinalStabilization()
{
    ChooseFinalResolution(
        EEchoesFinalResolution::ControlledStabilization);
}

void AEchoesPlayerController::ChooseFinalExtinguishment()
{
    ChooseFinalResolution(EEchoesFinalResolution::Extinguishment);
}

void AEchoesPlayerController::ChooseFinalEvolution()
{
    ChooseFinalResolution(EEchoesFinalResolution::OpenEvolution);
}

void AEchoesPlayerController::ChooseFinalResolution(
    EEchoesFinalResolution Resolution)
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    UEchoesSimulationSubsystem* Bridge = GetWorld() != nullptr
        ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
        : nullptr;
    if (Bridge == nullptr ||
        Bridge->GetOperationMode() !=
            EEchoesOperationMode::CampaignTheBrokenSun)
    {
        SetStatusMessage(
            TEXT("[FINAL_RESOLUTION_UNAVAILABLE] Final resolutions are chosen only during The Broken Sun."),
            4.0f);
        return;
    }
    FString Feedback;
    const bool bAccepted = Bridge->ChooseFinalResolution(
        Resolution,
        Feedback);
    SetStatusMessage(
        Feedback,
        bAccepted ? 12.0f : 6.0f);
    if (bAccepted)
    {
        if (UEchoesPresentationAudioSubsystem* Audio =
                GetWorld()->GetSubsystem<
                    UEchoesPresentationAudioSubsystem>())
        {
            Audio->PlayCommandConfirmation();
        }
    }
}

void AEchoesPlayerController::BuildBarracks()
{
    BuildAtCursor(echoes::sim::EntityType::Barracks);
}

void AEchoesPlayerController::BuildDropoff()
{
    BuildAtCursor(echoes::sim::EntityType::Dropoff);
}

void AEchoesPlayerController::BuildUtility()
{
    BuildAtCursor(echoes::sim::EntityType::UtilityStructure);
}

void AEchoesPlayerController::ProduceWorker()
{
    ProduceUnit(echoes::sim::EntityType::Worker);
}

void AEchoesPlayerController::ProduceSoldier()
{
    ProduceUnit(echoes::sim::EntityType::Soldier);
}

void AEchoesPlayerController::ProduceHeavy()
{
    ProduceUnit(echoes::sim::EntityType::HeavyUnit);
}

void AEchoesPlayerController::ProduceScout()
{
    ProduceUnit(echoes::sim::EntityType::ScoutUnit);
}

void AEchoesPlayerController::ResearchNextTechnology()
{
    if (IsModalOverlayVisible() && !bTechnologyPanelVisible)
    {
        return;
    }
    if (GetNetMode() == NM_Client)
    {
        const echoes::sim::net::ScopedViewKeyframe* View =
            GetNetworkScopedView();
        if (!IsNetworkClientControlActive() || View == nullptr)
        {
            SetStatusMessage(TEXT("[NETWORK_NOT_READY] Research is unavailable."));
            return;
        }
        const echoes::presentation::FFactionTechnologyProfile Profile =
            echoes::presentation::TechnologyProfile(View->faction);
        const echoes::sim::ResearchType Research =
            TechnologyPanelFocusedTier == 0
                ? Profile.TierOne
                : Profile.TierTwo;
        ResearchTechnology(Research);
        TechnologyPanelFocusedTier = 1;
        return;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    const echoes::sim::Simulation* Simulation =
        Bridge != nullptr ? Bridge->GetSimulation() : nullptr;
    const echoes::sim::PlayerState* Player =
        Simulation != nullptr
            ? Simulation->FindPlayer(UEchoesSimulationSubsystem::LocalPlayerId)
            : nullptr;
    if (Bridge == nullptr || Simulation == nullptr || Player == nullptr)
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Research is unavailable."));
        return;
    }
    const echoes::presentation::FFactionTechnologyProfile Profile =
        echoes::presentation::TechnologyProfile(Player->faction);
    const echoes::sim::ResearchType Candidates[] = {
        Profile.TierOne,
        Profile.TierTwo,
    };
    for (const echoes::sim::ResearchType Research : Candidates)
    {
        if (Player->HasCompletedResearch(Research))
        {
            continue;
        }
        ResearchTechnology(Research);
        return;
    }
    SetStatusMessage(TEXT("RESEARCH COMPLETE: both faction technologies are operational."));
}

void AEchoesPlayerController::ResearchTechnologyByTier(int32 TierIndex)
{
    if (GetNetMode() == NM_Client)
    {
        const echoes::sim::net::ScopedViewKeyframe* View =
            GetNetworkScopedView();
        if (View == nullptr || TierIndex < 0 || TierIndex > 1)
        {
            SetStatusMessage(TEXT("[RESEARCH_TECHNOLOGY_INVALID] Technology selection is unavailable."));
            return;
        }
        const echoes::presentation::FFactionTechnologyProfile Profile =
            echoes::presentation::TechnologyProfile(View->faction);
        ResearchTechnology(
            TierIndex == 0
                ? Profile.TierOne
                : Profile.TierTwo);
        return;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    const echoes::sim::Simulation* Simulation =
        Bridge != nullptr ? Bridge->GetSimulation() : nullptr;
    const echoes::sim::PlayerState* Player =
        Simulation != nullptr
            ? Simulation->FindPlayer(UEchoesSimulationSubsystem::LocalPlayerId)
            : nullptr;
    if (Player == nullptr || TierIndex < 0 || TierIndex > 1)
    {
        SetStatusMessage(TEXT("[RESEARCH_TECHNOLOGY_INVALID] Technology selection is unavailable."));
        return;
    }
    const echoes::presentation::FFactionTechnologyProfile Profile =
        echoes::presentation::TechnologyProfile(Player->faction);
    const echoes::sim::ResearchType Research =
        TierIndex == 0
            ? Profile.TierOne
            : Profile.TierTwo;
    ResearchTechnology(Research);
}

void AEchoesPlayerController::ResearchTechnology(
    echoes::sim::ResearchType Research)
{
    if (IsModalOverlayVisible() && !bTechnologyPanelVisible)
    {
        return;
    }
    PruneSelection();
    if (GetNetMode() == NM_Client)
    {
        if (!IsNetworkClientControlActive())
        {
            SetStatusMessage(TEXT("[NETWORK_NOT_READY] Research is unavailable."));
            return;
        }
        for (const uint32 EntityId : SelectedEntityIds)
        {
            const echoes::sim::net::ScopedEntityState* Entity =
                FindNetworkEntity(EntityId);
            if (Entity == nullptr || Entity->owner != NetworkSeat ||
                Entity->type != echoes::sim::EntityType::Barracks)
            {
                continue;
            }
            echoes::sim::net::CommandIntent Intent{};
            Intent.type = echoes::sim::CommandType::Research;
            Intent.actor = Entity->id;
            Intent.position = Entity->position;
            Intent.researchType = Research;
            TArray<echoes::sim::net::CommandIntent> Intents;
            Intents.Add(Intent);
            (void)SubmitNetworkCommandBatch(
                MoveTemp(Intents),
                TEXT("ONLINE RESEARCH"),
                NetworkSimToWorld(Entity->position),
                EEchoesCommandMarkerType::Interact);
            return;
        }
        SetStatusMessage(TEXT("[RESEARCH_PRODUCER_INVALID] Select an owned production structure."));
        return;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    const echoes::sim::Simulation* Simulation =
        Bridge != nullptr ? Bridge->GetSimulation() : nullptr;
    const echoes::sim::PlayerState* Player =
        Simulation != nullptr
            ? Simulation->FindPlayer(UEchoesSimulationSubsystem::LocalPlayerId)
            : nullptr;
    if (Bridge == nullptr || Simulation == nullptr || Player == nullptr)
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Research is unavailable."));
        return;
    }
    if (Player->activeResearch != echoes::sim::ResearchType::None)
    {
        SetStatusMessage(TEXT("[RESEARCH_BUSY] A technology is already in progress."));
        return;
    }
    uint32 ProducerId = 0;
    for (const uint32 EntityId : SelectedEntityIds)
    {
        const echoes::sim::Entity* Entity = Bridge->FindEntity(EntityId);
        if (Entity != nullptr &&
            Entity->owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            Entity->type == echoes::sim::EntityType::Barracks)
        {
            ProducerId = EntityId;
            break;
        }
    }
    if (ProducerId == 0)
    {
        SetStatusMessage(TEXT("[RESEARCH_PRODUCER_INVALID] Select a production structure before choosing a technology."));
        return;
    }
    FString Feedback;
    if (!Bridge->IssueResearchCommand(ProducerId, Research, Feedback))
    {
        SetStatusMessage(Feedback);
        return;
    }
    const TCHAR* Label = TEXT("UNKNOWN TECHNOLOGY");
    switch (Research)
    {
        case echoes::sim::ResearchType::MeridianPrismaticTargeting:
            Label = TEXT("PRISMATIC TARGETING");
            break;
        case echoes::sim::ResearchType::MeridianHorizonLattice:
            Label = TEXT("HORIZON LATTICE");
            break;
        case echoes::sim::ResearchType::KharuunEchoCartography:
            Label = TEXT("ECHO CARTOGRAPHY");
            break;
        case echoes::sim::ResearchType::KharuunAncestralEdge:
            Label = TEXT("ANCESTRAL EDGE");
            break;
        case echoes::sim::ResearchType::ChoirHeldAlternatives:
            Label = TEXT("HELD ALTERNATIVES");
            break;
        case echoes::sim::ResearchType::ChoirSharedResolution:
            Label = TEXT("SHARED RESOLUTION");
            break;
        case echoes::sim::ResearchType::None:
            break;
    }
    SetStatusMessage(FString::Printf(TEXT("%s: research queued."), Label));
}

void AEchoesPlayerController::AttackMoveAtCursor()
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    PruneSelection();
    if (GetNetMode() == NM_Client)
    {
        FHitResult HitResult;
        if (!IsNetworkClientControlActive() ||
            !TraceCommandTarget(HitResult))
        {
            SetStatusMessage(TEXT("[NETWORK_TARGET_UNAVAILABLE] Attack-move requires an active remote battlefield target."));
            return;
        }
        (void)SubmitNetworkSelectionCommand(
            echoes::sim::CommandType::AttackMove,
            0,
            HitResult.Location,
            true,
            false,
            FString::Printf(
                TEXT("ONLINE ATTACK-MOVE / %s"), *GetFormationLabel()),
            EEchoesCommandMarkerType::AttackMove);
        return;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Attack-move is unavailable."));
        return;
    }
    if (Bridge->GetMatchOutcome() != echoes::sim::MatchOutcome::Ongoing)
    {
        SetStatusMessage(TEXT("[MATCH_FINISHED] Press R to restart."));
        return;
    }
    if (SelectedEntityIds.IsEmpty())
    {
        SetStatusMessage(TEXT("[NO_SELECTION] Select one or more owned combat units first."));
        return;
    }
    FHitResult HitResult;
    if (!TraceCommandTarget(HitResult))
    {
        SetStatusMessage(TEXT("[NO_WORLD_HIT] Target an attack-move destination with the pointer or center reticle."));
        return;
    }

    const int32 UnitCount = SelectedEntityIds.Num();
    const TArray<FVector> FormationDestinations =
        BuildSelectedFormationDestinations(HitResult.Location, UnitCount);
    int32 AcceptedCount = 0;
    int32 RejectedCount = 0;
    FString LastRejection;
    for (int32 Index = 0; Index < UnitCount; ++Index)
    {
        const FVector UnitDestination = FormationDestinations[Index];
        FString Feedback;
        if (Bridge->IssueCommand(
                echoes::sim::CommandType::AttackMove,
                SelectedEntityIds[Index],
                0,
                UnitDestination,
                FutureWellChoice,
                Feedback))
        {
            ++AcceptedCount;
        }
        else
        {
            ++RejectedCount;
            LastRejection = Feedback;
        }
    }
    if (AcceptedCount > 0)
    {
        const FString RejectionSuffix =
            RejectedCount > 0
                ? FString::Printf(TEXT(", %d rejected."), RejectedCount)
                : TEXT(".");
        SetStatusMessage(FString::Printf(
            TEXT("ATTACK-MOVE / %s: %d queued%s"),
            *GetFormationLabel(),
            AcceptedCount,
            *RejectionSuffix));
        ShowAcceptedCommandMarker(
            HitResult.Location,
            EEchoesCommandMarkerType::AttackMove,
            AcceptedCount);
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_ATTACK_MOVE_ACCEPTED] source=%s screen=(%.1f,%.1f) accepted=%d rejected=%d formation=%s"),
            bKeyboardTargetingEnabled ? TEXT("keyboard_reticle") : TEXT("pointer"),
            bKeyboardTargetingEnabled ? -1.0f : LastPointerScreenPosition.X,
            bKeyboardTargetingEnabled ? -1.0f : LastPointerScreenPosition.Y,
            AcceptedCount,
            RejectedCount,
            *GetFormationLabel());
    }
    else
    {
        SetStatusMessage(
            LastRejection.IsEmpty()
                ? TEXT("[ATTACK_MOVE_REJECTED] No selected entity can attack-move.")
                : LastRejection);
    }
}

void AEchoesPlayerController::PatrolAtCursor()
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    PruneSelection();
    if (GetNetMode() == NM_Client)
    {
        FHitResult HitResult;
        if (!IsNetworkClientControlActive() ||
            !TraceCommandTarget(HitResult))
        {
            SetStatusMessage(TEXT("[NETWORK_TARGET_UNAVAILABLE] Patrol requires an active remote battlefield target."));
            return;
        }
        (void)SubmitNetworkSelectionCommand(
            echoes::sim::CommandType::Patrol,
            0,
            HitResult.Location,
            true,
            false,
            FString::Printf(
                TEXT("ONLINE PATROL / %s"), *GetFormationLabel()),
            EEchoesCommandMarkerType::Patrol);
        return;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Patrol is unavailable."));
        return;
    }
    if (Bridge->GetMatchOutcome() != echoes::sim::MatchOutcome::Ongoing)
    {
        SetStatusMessage(TEXT("[MATCH_FINISHED] Press R to restart."));
        return;
    }
    if (SelectedEntityIds.IsEmpty())
    {
        SetStatusMessage(TEXT("[NO_SELECTION] Select one or more owned combat units first."));
        return;
    }
    FHitResult HitResult;
    if (!TraceCommandTarget(HitResult))
    {
        SetStatusMessage(TEXT("[NO_WORLD_HIT] Target a patrol endpoint with the pointer or center reticle."));
        return;
    }

    const int32 UnitCount = SelectedEntityIds.Num();
    const TArray<FVector> FormationDestinations =
        BuildSelectedFormationDestinations(HitResult.Location, UnitCount);
    int32 AcceptedCount = 0;
    int32 RejectedCount = 0;
    FString LastRejection;
    for (int32 Index = 0; Index < UnitCount; ++Index)
    {
        const FVector UnitDestination = FormationDestinations[Index];
        FString Feedback;
        if (Bridge->IssueCommand(
                echoes::sim::CommandType::Patrol,
                SelectedEntityIds[Index],
                0,
                UnitDestination,
                FutureWellChoice,
                Feedback))
        {
            ++AcceptedCount;
        }
        else
        {
            ++RejectedCount;
            LastRejection = Feedback;
        }
    }
    if (AcceptedCount > 0)
    {
        const FString RejectionSuffix =
            RejectedCount > 0
                ? FString::Printf(TEXT(", %d rejected."), RejectedCount)
                : TEXT(".");
        SetStatusMessage(FString::Printf(
            TEXT("PATROL / %s: %d route%s assigned%s"),
            *GetFormationLabel(),
            AcceptedCount,
            AcceptedCount == 1 ? TEXT("") : TEXT("s"),
            *RejectionSuffix));
        ShowAcceptedCommandMarker(
            HitResult.Location,
            EEchoesCommandMarkerType::Patrol,
            AcceptedCount);
    }
    else
    {
        SetStatusMessage(
            LastRejection.IsEmpty()
                ? TEXT("[PATROL_REJECTED] No selected entity can patrol.")
                : LastRejection);
    }
}

void AEchoesPlayerController::StopSelectedUnits()
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    PruneSelection();
    if (GetNetMode() == NM_Client)
    {
        if (!IsNetworkClientControlActive())
        {
            SetStatusMessage(TEXT("[NETWORK_NOT_READY] Stop is unavailable."));
            return;
        }
        (void)SubmitNetworkSelectionCommand(
            echoes::sim::CommandType::Stop,
            0,
            FVector::ZeroVector,
            false,
            true,
            TEXT("ONLINE STOP"),
            EEchoesCommandMarkerType::Interact);
        return;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Stop is unavailable."));
        return;
    }
    if (SelectedEntityIds.IsEmpty())
    {
        SetStatusMessage(TEXT("[NO_SELECTION] Select one or more owned units first."));
        return;
    }
    const echoes::sim::Simulation* Simulation = Bridge->GetSimulation();
    const echoes::sim::PlayerState* Player =
        Simulation != nullptr
            ? Simulation->FindPlayer(UEchoesSimulationSubsystem::LocalPlayerId)
            : nullptr;
    const bool bCancellingResearch =
        Player != nullptr &&
        Player->activeResearch != echoes::sim::ResearchType::None &&
        SelectedEntityIds.Contains(Player->researchProducer);
    const uint32 ResearchProducer =
        bCancellingResearch ? Player->researchProducer : 0;
    const echoes::sim::ResearchType InterruptedResearch =
        bCancellingResearch
            ? Player->activeResearch
            : echoes::sim::ResearchType::None;
    int32 AcceptedCount = 0;
    FString LastRejection;
    for (const uint32 EntityId : SelectedEntityIds)
    {
        const echoes::sim::Entity* Entity = Bridge->FindEntity(EntityId);
        FString Feedback;
        if (Entity != nullptr && Bridge->IssueCommand(
                echoes::sim::CommandType::Stop,
                EntityId,
                0,
                Bridge->SimToWorld(Entity->position),
                FutureWellChoice,
                Feedback))
        {
            ++AcceptedCount;
        }
        else
        {
            LastRejection = Feedback;
        }
    }
    const FString StopFeedback =
        bCancellingResearch && AcceptedCount > 0
            ? TEXT("RESEARCH INTERRUPTION QUEUED: selected producer stopped // costs will not be refunded.")
            : AcceptedCount > 0
                  ? FString::Printf(
                        TEXT("STOP: %d unit%s stopped."),
                        AcceptedCount,
                        AcceptedCount == 1 ? TEXT("") : TEXT("s"))
                  : LastRejection.IsEmpty()
                        ? TEXT("[STOP_REJECTED] No selected entity accepted the order.")
                        : LastRejection;
    SetStatusMessage(StopFeedback);
    if (bCancellingResearch && AcceptedCount > 0)
    {
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_RESEARCH_CANCEL_QUEUED] player=%u producer=%u technology=%u costsRefunded=false input=stop"),
            UEchoesSimulationSubsystem::LocalPlayerId,
            ResearchProducer,
            static_cast<uint8>(InterruptedResearch));
    }
}

void AEchoesPlayerController::ToggleBulwarkDeploymentAtCursor()
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    PruneSelection();
    if (GetNetMode() == NM_Client)
    {
        FHitResult HitResult;
        if (!IsNetworkClientControlActive() ||
            !TraceCommandTarget(HitResult))
        {
            SetStatusMessage(TEXT("[NETWORK_TARGET_UNAVAILABLE] Bulwark deployment requires a remote battlefield direction."));
            return;
        }
        (void)SubmitNetworkSelectionCommand(
            echoes::sim::CommandType::ToggleDeploy,
            0,
            HitResult.Location,
            false,
            false,
            TEXT("ONLINE BULWARK DEPLOY / PACK"),
            EEchoesCommandMarkerType::Interact);
        return;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Bulwark deployment is unavailable."));
        return;
    }
    if (Bridge->GetMatchOutcome() != echoes::sim::MatchOutcome::Ongoing)
    {
        SetStatusMessage(TEXT("[MATCH_FINISHED] Press R to restart."));
        return;
    }
    FHitResult HitResult;
    if (!TraceCommandTarget(HitResult))
    {
        SetStatusMessage(TEXT("[NO_WORLD_HIT] Target the threat direction with the pointer or center reticle."));
        return;
    }

    int32 DeployedCount = 0;
    int32 PackedCount = 0;
    int32 RejectedCount = 0;
    FString LastRejection;
    for (const uint32 EntityId : SelectedEntityIds)
    {
        const echoes::sim::Entity* Entity = Bridge->FindEntity(EntityId);
        if (Entity == nullptr ||
            Entity->faction != echoes::sim::Faction::MeridianCompact ||
            Entity->type != echoes::sim::EntityType::HeavyUnit)
        {
            ++RejectedCount;
            continue;
        }
        const bool bWasDeployed = Entity->deployed;
        FString Feedback;
        if (Bridge->IssueCommand(
                echoes::sim::CommandType::ToggleDeploy,
                EntityId,
                0,
                HitResult.Location,
                FutureWellChoice,
                Feedback))
        {
            if (bWasDeployed)
            {
                ++PackedCount;
            }
            else
            {
                ++DeployedCount;
            }
        }
        else
        {
            ++RejectedCount;
            LastRejection = Feedback;
        }
    }
    if (DeployedCount + PackedCount > 0)
    {
        SetStatusMessage(FString::Printf(
            TEXT("BULWARK: %d deploying toward cursor, %d packing, %d rejected."),
            DeployedCount,
            PackedCount,
            RejectedCount));
    }
    else
    {
        SetStatusMessage(
            LastRejection.IsEmpty()
                ? TEXT("[BULWARK_REQUIRED] Select a Meridian Bulwark Team.")
                : LastRejection);
    }
}

void AEchoesPlayerController::ActivateRelaySupply()
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    PruneSelection();
    if (GetNetMode() == NM_Client)
    {
        if (!IsNetworkClientControlActive())
        {
            SetStatusMessage(TEXT("[NETWORK_NOT_READY] Relay supply is unavailable."));
            return;
        }
        (void)SubmitNetworkSelectionCommand(
            echoes::sim::CommandType::ActivateRelaySupply,
            0,
            FVector::ZeroVector,
            false,
            true,
            TEXT("ONLINE RELAY SUPPLY"),
            EEchoesCommandMarkerType::Interact);
        return;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Relay supply is unavailable."));
        return;
    }
    if (Bridge->GetMatchOutcome() != echoes::sim::MatchOutcome::Ongoing)
    {
        SetStatusMessage(TEXT("[MATCH_FINISHED] Press R to restart."));
        return;
    }

    int32 AcceptedCount = 0;
    int32 RejectedCount = 0;
    FString LastRejection;
    for (const uint32 EntityId : SelectedEntityIds)
    {
        const echoes::sim::Entity* Entity = Bridge->FindEntity(EntityId);
        if (Entity == nullptr ||
            Entity->faction != echoes::sim::Faction::MeridianCompact ||
            Entity->type != echoes::sim::EntityType::ScoutUnit)
        {
            ++RejectedCount;
            continue;
        }
        FString Feedback;
        if (Bridge->IssueCommand(
                echoes::sim::CommandType::ActivateRelaySupply,
                EntityId,
                0,
                Bridge->SimToWorld(Entity->position),
                FutureWellChoice,
                Feedback))
        {
            ++AcceptedCount;
        }
        else
        {
            ++RejectedCount;
            LastRejection = Feedback;
        }
    }
    SetStatusMessage(
        AcceptedCount > 0
            ? FString::Printf(
                  TEXT("RELAY SUPPLY: %d extension%s activated, %d rejected."),
                  AcceptedCount,
                  AcceptedCount == 1 ? TEXT("") : TEXT("s"),
                  RejectedCount)
            : LastRejection.IsEmpty()
                  ? TEXT("[RELAY_REQUIRED] Select a connected Meridian Relay Skiff.")
                  : LastRejection);
}

void AEchoesPlayerController::ToggleWaystoneRoot()
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    PruneSelection();
    if (GetNetMode() == NM_Client)
    {
        if (!IsNetworkClientControlActive())
        {
            SetStatusMessage(TEXT("[NETWORK_NOT_READY] Waystone migration is unavailable."));
            return;
        }
        (void)SubmitNetworkSelectionCommand(
            echoes::sim::CommandType::ToggleWaystoneRoot,
            0,
            FVector::ZeroVector,
            false,
            true,
            TEXT("ONLINE WAYSTONE ROOT / MIGRATE"),
            EEchoesCommandMarkerType::Interact);
        return;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Waystone migration is unavailable."));
        return;
    }
    if (Bridge->GetMatchOutcome() != echoes::sim::MatchOutcome::Ongoing)
    {
        SetStatusMessage(TEXT("[MATCH_FINISHED] Press R to restart."));
        return;
    }

    int32 AcceptedCount = 0;
    int32 RejectedCount = 0;
    FString LastRejection;
    for (const uint32 EntityId : SelectedEntityIds)
    {
        const echoes::sim::Entity* Entity = Bridge->FindEntity(EntityId);
        if (Entity == nullptr ||
            Entity->faction != echoes::sim::Faction::KharuunAssemblies ||
            Entity->type != echoes::sim::EntityType::Dropoff)
        {
            ++RejectedCount;
            continue;
        }
        FString Feedback;
        if (Bridge->IssueCommand(
                echoes::sim::CommandType::ToggleWaystoneRoot,
                EntityId,
                0,
                Bridge->SimToWorld(Entity->position),
                FutureWellChoice,
                Feedback))
        {
            ++AcceptedCount;
        }
        else
        {
            ++RejectedCount;
            LastRejection = Feedback;
        }
    }
    SetStatusMessage(
        AcceptedCount > 0
            ? FString::Printf(
                  TEXT("WAYSTONE: %d state change%s started, %d rejected."),
                  AcceptedCount,
                  AcceptedCount == 1 ? TEXT("") : TEXT("s"),
                  RejectedCount)
            : LastRejection.IsEmpty()
                  ? TEXT("[WAYSTONE_REQUIRED] Select a Kharuun Waystone.")
                  : LastRejection);
}

void AEchoesPlayerController::AdaptSelectedWarformsCarapace()
{
    AdaptSelectedWarforms(echoes::sim::WarformAdaptation::Carapace);
}

void AEchoesPlayerController::AdaptSelectedWarformsStriker()
{
    AdaptSelectedWarforms(echoes::sim::WarformAdaptation::Striker);
}

void AEchoesPlayerController::AdaptSelectedWarforms(
    echoes::sim::WarformAdaptation Adaptation)
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    PruneSelection();
    if (GetNetMode() == NM_Client)
    {
        const echoes::sim::net::ScopedViewKeyframe* View =
            GetNetworkScopedView();
        if (!IsNetworkClientControlActive() || View == nullptr)
        {
            SetStatusMessage(TEXT("[NETWORK_NOT_READY] Warform adaptation is unavailable."));
            return;
        }
        TArray<echoes::sim::net::CommandIntent> Intents;
        for (const uint32 EntityId : SelectedEntityIds)
        {
            const echoes::sim::net::ScopedEntityState* Actor =
                FindNetworkEntity(EntityId);
            if (Actor == nullptr || Actor->owner != NetworkSeat ||
                Actor->faction != echoes::sim::Faction::KharuunAssemblies ||
                (Actor->type != echoes::sim::EntityType::Soldier &&
                 Actor->type != echoes::sim::EntityType::HeavyUnit &&
                 Actor->type != echoes::sim::EntityType::ScoutUnit))
            {
                continue;
            }
            const echoes::sim::net::ScopedEntityState* NearestBasin = nullptr;
            uint64 NearestDistance = TNumericLimits<uint64>::Max();
            for (const echoes::sim::net::ScopedEntityState& Candidate :
                 View->entities)
            {
                if (Candidate.owner != NetworkSeat || !Candidate.completed ||
                    Candidate.type != echoes::sim::EntityType::Barracks)
                {
                    continue;
                }
                const int64 DeltaX =
                    static_cast<int64>(Actor->position.x.Raw()) -
                    Candidate.position.x.Raw();
                const int64 DeltaY =
                    static_cast<int64>(Actor->position.y.Raw()) -
                    Candidate.position.y.Raw();
                const uint64 Distance = static_cast<uint64>(
                    DeltaX * DeltaX + DeltaY * DeltaY);
                if (Distance < NearestDistance ||
                    (Distance == NearestDistance &&
                     (NearestBasin == nullptr ||
                      Candidate.id < NearestBasin->id)))
                {
                    NearestDistance = Distance;
                    NearestBasin = &Candidate;
                }
            }
            if (NearestBasin == nullptr)
            {
                continue;
            }
            echoes::sim::net::CommandIntent Intent{};
            Intent.type = echoes::sim::CommandType::AdaptWarform;
            Intent.actor = Actor->id;
            Intent.target = NearestBasin->id;
            Intent.position = Actor->position;
            Intent.warformAdaptation = Adaptation;
            Intents.Add(Intent);
        }
        (void)SubmitNetworkCommandBatch(
            MoveTemp(Intents),
            Adaptation == echoes::sim::WarformAdaptation::Carapace
                ? TEXT("ONLINE CARAPACE MOLT")
                : TEXT("ONLINE STRIKER MOLT"),
            FVector::ZeroVector,
            EEchoesCommandMarkerType::Interact);
        return;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    const echoes::sim::Simulation* Simulation =
        Bridge != nullptr ? Bridge->GetSimulation() : nullptr;
    if (Bridge == nullptr || Simulation == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Warform adaptation is unavailable."));
        return;
    }
    if (Bridge->GetMatchOutcome() != echoes::sim::MatchOutcome::Ongoing)
    {
        SetStatusMessage(TEXT("[MATCH_FINISHED] Press R to restart."));
        return;
    }

    int32 AcceptedCount = 0;
    int32 RejectedCount = 0;
    FString LastRejection;
    for (const uint32 EntityId : SelectedEntityIds)
    {
        const echoes::sim::Entity* Entity = Bridge->FindEntity(EntityId);
        if (Entity == nullptr ||
            Entity->faction != echoes::sim::Faction::KharuunAssemblies ||
            (Entity->type != echoes::sim::EntityType::Soldier &&
             Entity->type != echoes::sim::EntityType::HeavyUnit &&
             Entity->type != echoes::sim::EntityType::ScoutUnit))
        {
            ++RejectedCount;
            continue;
        }
        uint32 NearestBasin = 0;
        uint64 NearestDistance = TNumericLimits<uint64>::Max();
        for (const echoes::sim::Entity& Candidate : Simulation->Entities())
        {
            if (Candidate.owner != Entity->owner || !Candidate.completed ||
                Candidate.hitPoints <= 0 ||
                Candidate.faction != echoes::sim::Faction::KharuunAssemblies ||
                Candidate.type != echoes::sim::EntityType::Barracks)
            {
                continue;
            }
            const int64 DeltaX = static_cast<int64>(Entity->position.x.Raw()) -
                                 Candidate.position.x.Raw();
            const int64 DeltaY = static_cast<int64>(Entity->position.y.Raw()) -
                                 Candidate.position.y.Raw();
            const uint64 Distance = static_cast<uint64>(
                DeltaX * DeltaX + DeltaY * DeltaY);
            if (Distance < NearestDistance ||
                (Distance == NearestDistance && Candidate.id < NearestBasin))
            {
                NearestDistance = Distance;
                NearestBasin = Candidate.id;
            }
        }
        FString Feedback;
        if (NearestBasin != 0 && Bridge->IssueWarformAdaptation(
                EntityId, NearestBasin, Adaptation, Feedback))
        {
            ++AcceptedCount;
        }
        else
        {
            ++RejectedCount;
            LastRejection = NearestBasin == 0
                ? TEXT("[GROWTH_BASIN_REQUIRED] No completed friendly Growth Basin is available.")
                : Feedback;
        }
    }
    const TCHAR* FormName =
        Adaptation == echoes::sim::WarformAdaptation::Carapace
            ? TEXT("CARAPACE")
            : TEXT("STRIKER");
    SetStatusMessage(
        AcceptedCount > 0
            ? FString::Printf(
                  TEXT("%s MOLT: %d warform%s started, %d rejected."),
                  FormName,
                  AcceptedCount,
                  AcceptedCount == 1 ? TEXT("") : TEXT("s"),
                  RejectedCount)
            : LastRejection.IsEmpty()
                  ? TEXT("[WARFORM_REQUIRED] Select a Kharuun combat warform.")
                  : LastRejection);
}

void AEchoesPlayerController::RaiseSelectedCairnbackCoverAtCursor()
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    PruneSelection();
    if (GetNetMode() == NM_Client)
    {
        FHitResult HitResult;
        if (!IsNetworkClientControlActive() ||
            !TraceCommandTarget(HitResult))
        {
            SetStatusMessage(TEXT("[NETWORK_TARGET_UNAVAILABLE] Mineral cover requires an active remote battlefield target."));
            return;
        }
        (void)SubmitNetworkSelectionCommand(
            echoes::sim::CommandType::RaiseMineralCover,
            0,
            HitResult.Location,
            false,
            false,
            TEXT("ONLINE MINERAL COVER"),
            EEchoesCommandMarkerType::Build);
        return;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Mineral cover is unavailable."));
        return;
    }
    if (Bridge->GetMatchOutcome() != echoes::sim::MatchOutcome::Ongoing)
    {
        SetStatusMessage(TEXT("[MATCH_FINISHED] Press R to restart."));
        return;
    }
    FHitResult HitResult;
    if (!TraceCommandTarget(HitResult))
    {
        SetStatusMessage(TEXT("[NO_WORLD_HIT] Target a clear cover position with the pointer or center reticle."));
        return;
    }

    int32 AcceptedCount = 0;
    int32 RejectedCount = 0;
    FString LastRejection;
    for (const uint32 EntityId : SelectedEntityIds)
    {
        const echoes::sim::Entity* Entity = Bridge->FindEntity(EntityId);
        if (Entity == nullptr ||
            Entity->faction != echoes::sim::Faction::KharuunAssemblies ||
            Entity->type != echoes::sim::EntityType::HeavyUnit ||
            Entity->temporaryMineralCover)
        {
            ++RejectedCount;
            continue;
        }
        FString Feedback;
        if (Bridge->IssueMineralCover(EntityId, HitResult.Location, Feedback))
        {
            ++AcceptedCount;
        }
        else
        {
            ++RejectedCount;
            LastRejection = Feedback;
        }
    }
    SetStatusMessage(
        AcceptedCount > 0
            ? FString::Printf(
                  TEXT("MINERAL COVER: %d barrier%s raised, %d rejected."),
                  AcceptedCount,
                  AcceptedCount == 1 ? TEXT("") : TEXT("s"),
                  RejectedCount)
            : LastRejection.IsEmpty()
                  ? TEXT("[CAIRNBACK_REQUIRED] Select a Kharuun Cairnback.")
                  : LastRejection);
}

void AEchoesPlayerController::ReconcileSelectedChoirToManifest()
{
    ReconcileSelectedChoirIdentities(
        echoes::sim::ChoirIdentityState::Manifest);
}

void AEchoesPlayerController::ReconcileSelectedChoirToPossible()
{
    ReconcileSelectedChoirIdentities(
        echoes::sim::ChoirIdentityState::Possible);
}

void AEchoesPlayerController::ReconcileSelectedChoirIdentities(
    echoes::sim::ChoirIdentityState StableState)
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    PruneSelection();
    if (StableState != echoes::sim::ChoirIdentityState::Manifest &&
        StableState != echoes::sim::ChoirIdentityState::Possible)
    {
        SetStatusMessage(TEXT("[CHOIR_IDENTITY_INVALID] Choose Manifest or Possible identity."));
        return;
    }
    const TCHAR* IdentityName =
        StableState == echoes::sim::ChoirIdentityState::Manifest
            ? TEXT("MANIFEST")
            : TEXT("POSSIBLE");
    const echoes::sim::CommandType CommandType =
        StableState == echoes::sim::ChoirIdentityState::Manifest
            ? echoes::sim::CommandType::ReconcileToManifest
            : echoes::sim::CommandType::ReconcileToPossible;

    if (GetNetMode() == NM_Client)
    {
        if (!IsNetworkClientControlActive())
        {
            SetStatusMessage(TEXT("[NETWORK_NOT_READY] Choir reconciliation is unavailable."));
            return;
        }
        TArray<echoes::sim::net::CommandIntent> Intents;
        for (const uint32 EntityId : SelectedEntityIds)
        {
            const echoes::sim::net::ScopedEntityState* Actor =
                FindNetworkEntity(EntityId);
            if (Actor == nullptr || Actor->owner != NetworkSeat ||
                Actor->faction != echoes::sim::Faction::HollowChoir ||
                (Actor->type != echoes::sim::EntityType::Soldier &&
                 Actor->type != echoes::sim::EntityType::HeavyUnit &&
                 Actor->type != echoes::sim::EntityType::ScoutUnit))
            {
                continue;
            }
            echoes::sim::net::CommandIntent Intent{};
            Intent.type = CommandType;
            Intent.actor = Actor->id;
            Intent.position = Actor->position;
            Intents.Add(Intent);
        }
        (void)SubmitNetworkCommandBatch(
            MoveTemp(Intents),
            FString::Printf(TEXT("ONLINE CHOIR %s"), IdentityName),
            FVector::ZeroVector,
            EEchoesCommandMarkerType::Interact);
        return;
    }

    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Choir reconciliation is unavailable."));
        return;
    }
    if (Bridge->GetOperationMode() != EEchoesOperationMode::Skirmish &&
        Bridge->GetOperationMode() !=
            EEchoesOperationMode::CampaignSeveralVoicesOneCommand &&
        Bridge->GetOperationMode() !=
            EEchoesOperationMode::CampaignTheBrokenSun)
    {
        SetStatusMessage(TEXT("[CHOIR_COMMAND_AUTHORITY_REQUIRED] This operation does not grant command over a Hollow Choir force."));
        return;
    }
    if (Bridge->GetMatchOutcome() != echoes::sim::MatchOutcome::Ongoing)
    {
        SetStatusMessage(TEXT("[MATCH_FINISHED] Press R to restart."));
        return;
    }

    int32 AcceptedCount = 0;
    int32 RejectedCount = 0;
    FString LastRejection;
    for (const uint32 EntityId : SelectedEntityIds)
    {
        const echoes::sim::Entity* Entity = Bridge->FindEntity(EntityId);
        if (Entity == nullptr ||
            Entity->faction != echoes::sim::Faction::HollowChoir ||
            (Entity->type != echoes::sim::EntityType::Soldier &&
             Entity->type != echoes::sim::EntityType::HeavyUnit &&
             Entity->type != echoes::sim::EntityType::ScoutUnit))
        {
            ++RejectedCount;
            continue;
        }
        FString Feedback;
        if (Bridge->IssueChoirReconciliation(
                EntityId, StableState, Feedback))
        {
            ++AcceptedCount;
        }
        else
        {
            ++RejectedCount;
            LastRejection = Feedback;
        }
    }
    SetStatusMessage(
        AcceptedCount > 0
            ? FString::Printf(
                  TEXT("CHOIR %s: %d voice%s reconciling, %d rejected."),
                  IdentityName,
                  AcceptedCount,
                  AcceptedCount == 1 ? TEXT("") : TEXT("s"),
                  RejectedCount)
            : LastRejection.IsEmpty()
                  ? TEXT("[CHOIR_IDENTITY_REQUIRED] Select a Hollow Choir combat voice.")
                  : LastRejection);
}

void AEchoesPlayerController::HoldSelectedUnits()
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    PruneSelection();
    if (GetNetMode() == NM_Client)
    {
        if (!IsNetworkClientControlActive())
        {
            SetStatusMessage(TEXT("[NETWORK_NOT_READY] Hold position is unavailable."));
            return;
        }
        (void)SubmitNetworkSelectionCommand(
            echoes::sim::CommandType::Hold,
            0,
            FVector::ZeroVector,
            false,
            true,
            TEXT("ONLINE HOLD POSITION"),
            EEchoesCommandMarkerType::Interact);
        return;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Hold position is unavailable."));
        return;
    }
    if (SelectedEntityIds.IsEmpty())
    {
        SetStatusMessage(TEXT("[NO_SELECTION] Select one or more owned defenders first."));
        return;
    }

    int32 AcceptedCount = 0;
    int32 RejectedCount = 0;
    FString LastRejection;
    for (const uint32 EntityId : SelectedEntityIds)
    {
        const echoes::sim::Entity* Entity = Bridge->FindEntity(EntityId);
        FString Feedback;
        if (Entity != nullptr && Bridge->IssueCommand(
                echoes::sim::CommandType::Hold,
                EntityId,
                0,
                Bridge->SimToWorld(Entity->position),
                FutureWellChoice,
                Feedback))
        {
            ++AcceptedCount;
        }
        else
        {
            ++RejectedCount;
            LastRejection = Feedback;
        }
    }
    if (AcceptedCount > 0)
    {
        const FString RejectionSuffix =
            RejectedCount > 0
                ? FString::Printf(TEXT(", %d rejected."), RejectedCount)
                : TEXT(".");
        SetStatusMessage(FString::Printf(
            TEXT("HOLD POSITION: %d defender%s anchored%s"),
            AcceptedCount,
            AcceptedCount == 1 ? TEXT("") : TEXT("s"),
            *RejectionSuffix));
    }
    else
    {
        SetStatusMessage(
            LastRejection.IsEmpty()
                ? TEXT("[HOLD_REJECTED] No selected entity can defend a position.")
                : LastRejection);
    }
}

void AEchoesPlayerController::GuardAtCursor()
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    PruneSelection();
    if (GetNetMode() == NM_Client)
    {
        FHitResult HitResult;
        if (!IsNetworkClientControlActive() ||
            !TraceCommandTarget(HitResult))
        {
            SetStatusMessage(TEXT("[NETWORK_TARGET_UNAVAILABLE] Guard requires a visible owned target."));
            return;
        }
        const AEchoesEntityView* TargetView =
            Cast<AEchoesEntityView>(HitResult.GetActor());
        const echoes::sim::net::ScopedEntityState* Target =
            TargetView != nullptr
                ? FindNetworkEntity(TargetView->GetEntityId())
                : nullptr;
        if (Target == nullptr || Target->owner != NetworkSeat)
        {
            SetStatusMessage(TEXT("[GUARD_TARGET_INVALID] Point at a live owned entity."));
            return;
        }
        (void)SubmitNetworkSelectionCommand(
            echoes::sim::CommandType::Guard,
            Target->id,
            NetworkSimToWorld(Target->position),
            false,
            false,
            TEXT("ONLINE GUARD"),
            EEchoesCommandMarkerType::Guard);
        return;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Guard is unavailable."));
        return;
    }
    if (Bridge->GetMatchOutcome() != echoes::sim::MatchOutcome::Ongoing)
    {
        SetStatusMessage(TEXT("[MATCH_FINISHED] Press R to restart."));
        return;
    }
    if (SelectedEntityIds.IsEmpty())
    {
        SetStatusMessage(TEXT("[NO_SELECTION] Select one or more owned defenders first."));
        return;
    }
    FHitResult HitResult;
    if (!TraceCommandTarget(HitResult))
    {
        SetStatusMessage(TEXT("[NO_WORLD_HIT] Target the owned entity to guard with the pointer or center reticle."));
        return;
    }
    const AEchoesEntityView* TargetView =
        Cast<AEchoesEntityView>(HitResult.GetActor());
    const echoes::sim::Entity* Target =
        TargetView != nullptr
            ? Bridge->FindEntity(TargetView->GetEntityId())
            : nullptr;
    if (Target == nullptr ||
        Target->owner != UEchoesSimulationSubsystem::LocalPlayerId)
    {
        SetStatusMessage(TEXT("[GUARD_TARGET_INVALID] Point at a live owned entity."));
        return;
    }

    int32 AcceptedCount = 0;
    int32 RejectedCount = 0;
    FString LastRejection;
    for (const uint32 EntityId : SelectedEntityIds)
    {
        FString Feedback;
        if (Bridge->IssueCommand(
                echoes::sim::CommandType::Guard,
                EntityId,
                Target->id,
                Bridge->SimToWorld(Target->position),
                FutureWellChoice,
                Feedback))
        {
            ++AcceptedCount;
        }
        else
        {
            ++RejectedCount;
            LastRejection = Feedback;
        }
    }
    if (AcceptedCount > 0)
    {
        const FString RejectionSuffix =
            RejectedCount > 0
                ? FString::Printf(TEXT(", %d rejected."), RejectedCount)
                : TEXT(".");
        SetStatusMessage(FString::Printf(
            TEXT("GUARD: %d defender%s assigned to entity %u%s"),
            AcceptedCount,
            AcceptedCount == 1 ? TEXT("") : TEXT("s"),
            Target->id,
            *RejectionSuffix));
        ShowAcceptedCommandMarker(
            Bridge->SimToWorld(Target->position),
            EEchoesCommandMarkerType::Guard,
            AcceptedCount);
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_GUARD_ACCEPTED] source=%s screen=(%.1f,%.1f) target=%u accepted=%d rejected=%d ownerScoped=true"),
            bKeyboardTargetingEnabled ? TEXT("keyboard_reticle") : TEXT("pointer"),
            bKeyboardTargetingEnabled ? -1.0f : LastPointerScreenPosition.X,
            bKeyboardTargetingEnabled ? -1.0f : LastPointerScreenPosition.Y,
            Target->id,
            AcceptedCount,
            RejectedCount);
    }
    else
    {
        SetStatusMessage(
            LastRejection.IsEmpty()
                ? TEXT("[GUARD_REJECTED] No selected entity accepted the guard order.")
                : LastRejection);
    }
}

void AEchoesPlayerController::QuickSaveScenario()
{
    if (IsModalOverlayVisible() || IsActiveOnlineNetworkMatch())
    {
        if (IsActiveOnlineNetworkMatch())
        {
            SetStatusMessage(
                TEXT("[ONLINE_SAVE_DISABLED] Online authority state cannot be written to a local checkpoint."),
                5.0f);
        }
        return;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    FString Feedback;
    if (Bridge == nullptr)
    {
        Feedback = TEXT("[SAVE_SIM_NOT_READY] No active scenario can be saved.");
    }
    else
    {
        Bridge->QuickSaveScenario(Feedback);
    }
    SetStatusMessage(Feedback, 6.0f);
}

void AEchoesPlayerController::QuickLoadScenario()
{
    if (IsModalOverlayVisible() || IsActiveOnlineNetworkMatch())
    {
        if (IsActiveOnlineNetworkMatch())
        {
            SetStatusMessage(
                TEXT("[ONLINE_LOAD_DISABLED] A local checkpoint cannot replace online authority state."),
                5.0f);
        }
        return;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    FString Feedback;
    if (Bridge == nullptr)
    {
        Feedback = TEXT("[LOAD_SIM_NOT_READY] Start a scenario before loading.");
    }
    else if (Bridge->QuickLoadScenario(Feedback))
    {
        ClearSelection();
        bControlGroupAssignmentArmed = false;
    }
    SetStatusMessage(Feedback, 7.0f);
}

void AEchoesPlayerController::CycleHudScale()
{
    UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get();
    if (Settings == nullptr)
    {
        SetStatusMessage(TEXT("[SETTINGS_UNAVAILABLE] UI scale could not be changed."));
        return;
    }
    const float CurrentScale = Settings->GetHudScale();
    const float NewScale =
        CurrentScale < 0.99f ? 1.0f
        : CurrentScale < 1.14f ? 1.15f
        : CurrentScale < 1.34f ? 1.35f
                               : 0.85f;
    Settings->SetHudScale(NewScale);
    Settings->SaveSettings();
    SetStatusMessage(FString::Printf(
        TEXT("ACCESSIBILITY: UI scale set to %d%%."),
        FMath::RoundToInt(NewScale * 100.0f)));
}

void AEchoesPlayerController::ToggleHighContrast()
{
    UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get();
    if (Settings == nullptr)
    {
        SetStatusMessage(TEXT("[SETTINGS_UNAVAILABLE] High contrast could not be changed."));
        return;
    }
    const bool bEnabled = !Settings->IsHighContrastHudEnabled();
    Settings->SetHighContrastHudEnabled(bEnabled);
    Settings->SaveSettings();
    SetStatusMessage(FString::Printf(
        TEXT("ACCESSIBILITY: high-contrast HUD %s."),
        bEnabled ? TEXT("enabled") : TEXT("disabled")));
}

void AEchoesPlayerController::ToggleReducedMotion()
{
    UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get();
    if (Settings == nullptr)
    {
        SetStatusMessage(TEXT("[SETTINGS_UNAVAILABLE] Reduced motion could not be changed."));
        return;
    }
    const bool bEnabled = !Settings->IsReducedMotionEnabled();
    Settings->SetReducedMotionEnabled(bEnabled);
    Settings->SaveSettings();
    SetStatusMessage(FString::Printf(
        TEXT("ACCESSIBILITY: reduced camera motion %s."),
        bEnabled ? TEXT("enabled") : TEXT("disabled")));
}

void AEchoesPlayerController::ToggleReducedFlashing()
{
    UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get();
    if (Settings == nullptr)
    {
        SetStatusMessage(TEXT("[SETTINGS_UNAVAILABLE] Reduced flashing could not be changed."));
        return;
    }
    const bool bEnabled = !Settings->IsReducedFlashingEnabled();
    Settings->SetReducedFlashingEnabled(bEnabled);
    Settings->SaveSettings();
    SetStatusMessage(FString::Printf(
        TEXT("ACCESSIBILITY: reduced combat flashing %s."),
        bEnabled ? TEXT("enabled") : TEXT("disabled")));
}

void AEchoesPlayerController::ToggleEdgePan()
{
    UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get();
    if (Settings == nullptr)
    {
        SetStatusMessage(TEXT("[SETTINGS_UNAVAILABLE] Edge pan could not be changed."));
        return;
    }
    const bool bEnabled = !Settings->IsEdgePanEnabled();
    Settings->SetEdgePanEnabled(bEnabled);
    Settings->SaveSettings();
    SetStatusMessage(FString::Printf(
        TEXT("CONTROLS: screen-edge camera pan %s."),
        bEnabled ? TEXT("enabled") : TEXT("disabled")));
}

void AEchoesPlayerController::AdjustCameraPanSpeed(float Delta)
{
    UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get();
    if (Settings == nullptr)
    {
        SetStatusMessage(TEXT("[SETTINGS_UNAVAILABLE] Camera pan speed could not be changed."));
        return;
    }
    Settings->SetCameraPanSpeedScale(Settings->GetCameraPanSpeedScale() + Delta);
    Settings->SaveSettings();
    SetStatusMessage(FString::Printf(
        TEXT("CONTROLS: camera pan speed set to %d%%."),
        FMath::RoundToInt(Settings->GetCameraPanSpeedScale() * 100.0f)));
}

void AEchoesPlayerController::AdjustCameraZoomSpeed(float Delta)
{
    UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get();
    if (Settings == nullptr)
    {
        SetStatusMessage(TEXT("[SETTINGS_UNAVAILABLE] Camera zoom speed could not be changed."));
        return;
    }
    Settings->SetCameraZoomScale(Settings->GetCameraZoomScale() + Delta);
    Settings->SaveSettings();
    SetStatusMessage(FString::Printf(
        TEXT("CONTROLS: camera zoom step set to %d%%."),
        FMath::RoundToInt(Settings->GetCameraZoomScale() * 100.0f)));
}

void AEchoesPlayerController::DecreaseCameraPanSpeed()
{
    AdjustCameraPanSpeed(-0.25f);
}

void AEchoesPlayerController::IncreaseCameraPanSpeed()
{
    AdjustCameraPanSpeed(0.25f);
}

void AEchoesPlayerController::DecreaseCameraZoomSpeed()
{
    AdjustCameraZoomSpeed(-0.25f);
}

void AEchoesPlayerController::IncreaseCameraZoomSpeed()
{
    AdjustCameraZoomSpeed(0.25f);
}

void AEchoesPlayerController::CycleEffectsVolume()
{
    UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get();
    if (Settings == nullptr)
    {
        SetStatusMessage(TEXT("[SETTINGS_UNAVAILABLE] Effects volume could not be changed."));
        return;
    }
    const float Current = Settings->GetEffectsVolume();
    const float Next = Current > 0.8f ? 0.6f : Current > 0.2f ? 0.0f : 1.0f;
    Settings->SetEffectsVolume(Next);
    Settings->SaveSettings();
    SetStatusMessage(FString::Printf(
        TEXT("AUDIO: effects volume set to %d%%."),
        FMath::RoundToInt(Next * 100.0f)));
}

void AEchoesPlayerController::ToggleReducedDynamicRange()
{
    UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get();
    if (Settings == nullptr)
    {
        SetStatusMessage(TEXT("[SETTINGS_UNAVAILABLE] Dynamic range could not be changed."));
        return;
    }
    const bool bEnabled = !Settings->IsReducedDynamicRangeEnabled();
    Settings->SetReducedDynamicRangeEnabled(bEnabled);
    Settings->SaveSettings();
    SetStatusMessage(FString::Printf(
        TEXT("AUDIO: reduced dynamic range %s."),
        bEnabled ? TEXT("enabled") : TEXT("disabled")));
}

void AEchoesPlayerController::BuildAtCursor(
    echoes::sim::EntityType BuildingType)
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    PruneSelection();
    if (GetNetMode() == NM_Client)
    {
        if (!IsNetworkClientControlActive())
        {
            SetStatusMessage(TEXT("[NETWORK_NOT_READY] Construction is unavailable."));
            return;
        }
        uint32 WorkerId = 0;
        for (const uint32 EntityId : SelectedEntityIds)
        {
            const echoes::sim::net::ScopedEntityState* Entity =
                FindNetworkEntity(EntityId);
            if (Entity != nullptr && Entity->owner == NetworkSeat &&
                Entity->type == echoes::sim::EntityType::Worker)
            {
                WorkerId = EntityId;
                break;
            }
        }
        FHitResult HitResult;
        if (WorkerId == 0 || !TraceCommandTarget(HitResult))
        {
            SetStatusMessage(TEXT("[BUILD_REQUIRES_WORKER] Select an owned worker and target visible open ground."));
            return;
        }
        echoes::sim::net::CommandIntent Intent{};
        Intent.type = echoes::sim::CommandType::Build;
        Intent.actor = WorkerId;
        Intent.position = NetworkWorldToSim(HitResult.Location);
        Intent.buildType = BuildingType;
        TArray<echoes::sim::net::CommandIntent> Intents;
        Intents.Add(Intent);
        (void)SubmitNetworkCommandBatch(
            MoveTemp(Intents),
            TEXT("ONLINE CONSTRUCTION"),
            HitResult.Location,
            EEchoesCommandMarkerType::Build);
        return;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Construction is unavailable."));
        return;
    }
    uint32 WorkerId = 0;
    for (const uint32 EntityId : SelectedEntityIds)
    {
        const echoes::sim::Entity* Entity = Bridge->FindEntity(EntityId);
        if (Entity != nullptr &&
            Entity->type == echoes::sim::EntityType::Worker)
        {
            WorkerId = EntityId;
            break;
        }
    }
    if (WorkerId == 0)
    {
        SetStatusMessage(
            TEXT("[BUILD_REQUIRES_WORKER] Select a worker, point at open ground, then press B, N, or M."));
        return;
    }
    FHitResult HitResult;
    if (!TraceCommandTarget(HitResult))
    {
        SetStatusMessage(TEXT("[NO_WORLD_HIT] Target open battlefield ground with the pointer or keyboard reticle."));
        return;
    }
    FString Feedback;
    if (Bridge->IssueBuildCommand(
            WorkerId,
            BuildingType,
            HitResult.Location,
            Feedback))
    {
        SetStatusMessage(
            BuildingType == echoes::sim::EntityType::Barracks
                ? TEXT("PRODUCTION STRUCTURE: construction order queued.")
                : BuildingType == echoes::sim::EntityType::UtilityStructure
                      ? TEXT("FACTION UTILITY: construction order queued.")
                      : TEXT("LOGISTICS STRUCTURE: construction order queued."));
        ShowAcceptedCommandMarker(
            HitResult.Location,
            EEchoesCommandMarkerType::Build,
            1);
    }
    else
    {
        SetStatusMessage(Feedback);
    }
}

void AEchoesPlayerController::ShowAcceptedCommandMarker(
    const FVector& WorldLocation,
    EEchoesCommandMarkerType MarkerType,
    int32 AcceptedCount)
{
    UWorld* World = GetWorld();
    if (World == nullptr || AcceptedCount <= 0)
    {
        return;
    }

    if (UEchoesPresentationAudioSubsystem* Audio =
            World->GetSubsystem<UEchoesPresentationAudioSubsystem>())
    {
        Audio->PlayCommandConfirmation();
    }

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.ObjectFlags |= RF_Transient;
    SpawnParameters.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AEchoesCommandMarkerView* Marker = World->SpawnActor<AEchoesCommandMarkerView>(
        WorldLocation + FVector(0.0f, 0.0f, 8.0f),
        FRotator::ZeroRotator,
        SpawnParameters);
    if (Marker == nullptr)
    {
        UE_LOG(
            LogEchoes,
            Warning,
            TEXT("[ECHOES_COMMAND_MARKER_FAILED] accepted=%d authorityChanged=false"),
            AcceptedCount);
        return;
    }

    const UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get();
    const bool bReducedMotion =
        Settings != nullptr && Settings->IsReducedMotionEnabled();
    const bool bReducedFlashing =
        Settings != nullptr && Settings->IsReducedFlashingEnabled();
    Marker->InitializeMarker(MarkerType, bReducedMotion, bReducedFlashing);

    const TCHAR* MarkerLabel = TEXT("move");
    switch (MarkerType)
    {
        case EEchoesCommandMarkerType::Attack:
            MarkerLabel = TEXT("attack");
            break;
        case EEchoesCommandMarkerType::AttackMove:
            MarkerLabel = TEXT("attack_move");
            break;
        case EEchoesCommandMarkerType::Patrol:
            MarkerLabel = TEXT("patrol");
            break;
        case EEchoesCommandMarkerType::Guard:
            MarkerLabel = TEXT("guard");
            break;
        case EEchoesCommandMarkerType::Build:
            MarkerLabel = TEXT("build");
            break;
        case EEchoesCommandMarkerType::Interact:
            MarkerLabel = TEXT("interact");
            break;
        case EEchoesCommandMarkerType::Move:
            break;
    }
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_COMMAND_MARKER] type=%s accepted=%d formation=%s vfx=selection-command-vfx-v2 authored=true collision=false navigation=false authoritative=false reducedMotion=%s reducedFlashing=%s finalArt=false"),
        MarkerLabel,
        AcceptedCount,
        *GetFormationLabel(),
        bReducedMotion ? TEXT("true") : TEXT("false"),
        bReducedFlashing ? TEXT("true") : TEXT("false"));
}

void AEchoesPlayerController::ProduceUnit(echoes::sim::EntityType UnitType)
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    PruneSelection();
    if (GetNetMode() == NM_Client)
    {
        if (!IsNetworkClientControlActive())
        {
            SetStatusMessage(TEXT("[NETWORK_NOT_READY] Production is unavailable."));
            return;
        }
        TArray<echoes::sim::net::CommandIntent> Intents;
        for (const uint32 EntityId : SelectedEntityIds)
        {
            const echoes::sim::net::ScopedEntityState* Entity =
                FindNetworkEntity(EntityId);
            const bool bCompatible = Entity != nullptr &&
                Entity->owner == NetworkSeat &&
                ((UnitType == echoes::sim::EntityType::Worker &&
                  Entity->type == echoes::sim::EntityType::CommandCore) ||
                 ((UnitType == echoes::sim::EntityType::Soldier ||
                   UnitType == echoes::sim::EntityType::HeavyUnit ||
                   UnitType == echoes::sim::EntityType::ScoutUnit) &&
                  Entity->type == echoes::sim::EntityType::Barracks));
            if (!bCompatible)
            {
                continue;
            }
            echoes::sim::net::CommandIntent Intent{};
            Intent.type = echoes::sim::CommandType::Produce;
            Intent.actor = Entity->id;
            Intent.position = Entity->position;
            Intent.buildType = UnitType;
            Intents.Add(Intent);
        }
        (void)SubmitNetworkCommandBatch(
            MoveTemp(Intents),
            TEXT("ONLINE PRODUCTION"),
            FVector::ZeroVector,
            EEchoesCommandMarkerType::Interact);
        return;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Production is unavailable."));
        return;
    }
    int32 Accepted = 0;
    FString LastFeedback;
    for (const uint32 EntityId : SelectedEntityIds)
    {
        const echoes::sim::Entity* Entity = Bridge->FindEntity(EntityId);
        const bool bCompatible = Entity != nullptr &&
            ((UnitType == echoes::sim::EntityType::Worker &&
              Entity->type == echoes::sim::EntityType::CommandCore) ||
             ((UnitType == echoes::sim::EntityType::Soldier ||
               UnitType == echoes::sim::EntityType::HeavyUnit ||
               UnitType == echoes::sim::EntityType::ScoutUnit) &&
              Entity->type == echoes::sim::EntityType::Barracks));
        if (!bCompatible)
        {
            continue;
        }
        FString Feedback;
        if (Bridge->IssueProductionCommand(EntityId, UnitType, Feedback))
        {
            ++Accepted;
        }
        else
        {
            LastFeedback = Feedback;
        }
    }
    if (Accepted > 0)
    {
        SetStatusMessage(
            FString::Printf(
                TEXT("%s: %d production order%s queued."),
                UnitType == echoes::sim::EntityType::Worker
                    ? TEXT("WORKER")
                    : UnitType == echoes::sim::EntityType::HeavyUnit
                          ? TEXT("HEAVY UNIT")
                          : UnitType == echoes::sim::EntityType::ScoutUnit
                                ? TEXT("SCOUT UNIT")
                                : TEXT("LINE UNIT"),
                Accepted,
                Accepted == 1 ? TEXT("") : TEXT("s")));
    }
    else
    {
        SetStatusMessage(
            LastFeedback.IsEmpty()
                ? TEXT("[NO_COMPATIBLE_PRODUCER] Select a headquarters for Q or a production structure for E, semicolon, or apostrophe.")
                : LastFeedback);
    }
}

void AEchoesPlayerController::ToggleTechnologyPanel()
{
    if (IsActiveOnlineNetworkMatch() ||
        IsOpponentReconnectGraceActive())
    {
        SetStatusMessage(
            TEXT("ONLINE MATCH — the Technology Archive does not pause or replace authority play."),
            5.0f);
        return;
    }
    if (bTitleScreenVisible || bMissionBriefingVisible ||
        bPauseMenuVisible || bMatchResultVisible)
    {
        return;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Technologies are unavailable."));
        return;
    }
    if (Bridge->GetMatchOutcome() != echoes::sim::MatchOutcome::Ongoing)
    {
        SetStatusMessage(TEXT("[MATCH_FINISHED] Press R to restart."));
        return;
    }

    if (bTechnologyPanelVisible)
    {
        bTechnologyPanelVisible = false;
        Bridge->SetScenarioPaused(bTechnologyPanelWasScenarioPaused);
        SetIgnoreMoveInput(bTechnologyPanelWasScenarioPaused);
        SetIgnoreLookInput(bTechnologyPanelWasScenarioPaused);
        SetStatusMessage(TEXT("TECHNOLOGY ARCHIVE CLOSED."));
    }
    else
    {
        const echoes::sim::Simulation* Simulation = Bridge->GetSimulation();
        const echoes::sim::PlayerState* Player =
            Simulation != nullptr
                ? Simulation->FindPlayer(UEchoesSimulationSubsystem::LocalPlayerId)
                : nullptr;
        const echoes::sim::ResearchType FirstTechnology =
            Player != nullptr
                ? echoes::presentation::TechnologyProfile(Player->faction)
                      .TierOne
                : echoes::sim::ResearchType::None;
        TechnologyPanelFocusedTier =
            Player != nullptr && Player->HasCompletedResearch(FirstTechnology)
                ? 1
                : 0;
        bTechnologyPanelWasScenarioPaused = Bridge->IsScenarioPaused();
        bTechnologyPanelVisible = true;
        if (UEchoesInterfaceAudioSubsystem* InterfaceAudio =
                GetWorld()->GetSubsystem<UEchoesInterfaceAudioSubsystem>())
        {
            InterfaceAudio->PlayInterfaceCue(EEchoesInterfaceCue::MenuOpen);
        }
        bSelectionButtonDown = false;
        Bridge->SetScenarioPaused(true);
        SetIgnoreMoveInput(true);
        SetIgnoreLookInput(true);
        SetStatusMessage(
            TEXT("TECHNOLOGY ARCHIVE — Up/Down chooses a tier; Enter activates it; Shift+R chooses the next available project."),
            3600.0f);
    }
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_TECHNOLOGY_PANEL] visible=%s paused=%s focusedTier=%d pointerRows=true keyboardFocus=true keyboardConfirm=true"),
        bTechnologyPanelVisible ? TEXT("true") : TEXT("false"),
        Bridge->IsScenarioPaused() ? TEXT("true") : TEXT("false"),
        TechnologyPanelFocusedTier + 1);
}

void AEchoesPlayerController::FocusPreviousTechnologyTier()
{
    if (!bTechnologyPanelVisible)
    {
        NudgeKeyboardTarget(FVector2D(0.0f, -1.0f));
        return;
    }
    TechnologyPanelFocusedTier =
        FMath::Clamp(TechnologyPanelFocusedTier - 1, 0, 1);
    SetStatusMessage(
        FString::Printf(
            TEXT("TECHNOLOGY ARCHIVE — Tier %d focused; press Enter to activate."),
            TechnologyPanelFocusedTier + 1),
        3600.0f);
}

void AEchoesPlayerController::FocusNextTechnologyTier()
{
    if (!bTechnologyPanelVisible)
    {
        NudgeKeyboardTarget(FVector2D(0.0f, 1.0f));
        return;
    }
    TechnologyPanelFocusedTier =
        FMath::Clamp(TechnologyPanelFocusedTier + 1, 0, 1);
    SetStatusMessage(
        FString::Printf(
            TEXT("TECHNOLOGY ARCHIVE — Tier %d focused; press Enter to activate."),
            TechnologyPanelFocusedTier + 1),
        3600.0f);
}

bool AEchoesPlayerController::HandleTechnologyPanelPointer(
    const FVector2D& ScreenPosition)
{
    if (!bTechnologyPanelVisible)
    {
        return false;
    }
    int32 ViewportX = 0;
    int32 ViewportY = 0;
    GetViewportSize(ViewportX, ViewportY);
    if (ViewportX <= 0 || ViewportY <= 0)
    {
        SetStatusMessage(TEXT("[VIEWPORT_UNAVAILABLE] Technology selection could not resolve the screen."));
        return true;
    }
    const UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get();
    const FEchoesTechnologyPanelLayout Layout =
        FEchoesTechnologyPanelLayout::Build(
            FVector2D(ViewportX, ViewportY),
            Settings != nullptr ? Settings->GetHudScale() : 1.0f);
    if (Layout.CloseButton.IsInsideOrOn(ScreenPosition))
    {
        ToggleTechnologyPanel();
        return true;
    }
    for (int32 TierIndex = 0; TierIndex < 2; ++TierIndex)
    {
        if (Layout.TechnologyRows[TierIndex].IsInsideOrOn(ScreenPosition))
        {
            TechnologyPanelFocusedTier = TierIndex;
            ResearchTechnologyByTier(TierIndex);
            return true;
        }
    }
    return true;
}

bool AEchoesPlayerController::HandleOnlineFrontDoorPointer(
    const FVector2D& ScreenPosition,
    const FVector2D& ViewportSize,
    float HudScale)
{
    UEchoesGameInstance* EchoesGameInstance = GetEchoesGameInstance();
    if (EchoesGameInstance == nullptr || !IsOnlineFrontDoorVisible())
    {
        return false;
    }
    const FEchoesOnlineFrontDoorLayout Layout =
        FEchoesOnlineFrontDoorLayout::Build(ViewportSize, HudScale);
    const EEchoesOnlineFrontDoorState State =
        EchoesGameInstance->GetOnlineState();
    if (State == EEchoesOnlineFrontDoorState::JoinSetup)
    {
        if (Layout.HostButton.IsInsideOrOn(ScreenPosition))
        {
            EchoesGameInstance->FocusOnlineAction(0);
            ConfirmOnlineFrontDoorAction();
        }
        else if (Layout.EndpointField.IsInsideOrOn(ScreenPosition))
        {
            EchoesGameInstance->FocusOnlineAction(1);
        }
        else if (Layout.JoinButton.IsInsideOrOn(ScreenPosition))
        {
            EchoesGameInstance->FocusOnlineAction(2);
            ConfirmOnlineFrontDoorAction();
        }
        else if (Layout.BackButton.IsInsideOrOn(ScreenPosition))
        {
            EchoesGameInstance->FocusOnlineAction(3);
            CancelOnlineFrontDoor();
        }
        return true;
    }
    if (State == EEchoesOnlineFrontDoorState::Failed)
    {
        if (Layout.RetryButton.IsInsideOrOn(ScreenPosition))
        {
            EchoesGameInstance->RetryOnlineFrontDoor(this);
        }
        else if (Layout.BackButton.IsInsideOrOn(ScreenPosition))
        {
            CancelOnlineFrontDoor();
        }
        return true;
    }
    if (State == EEchoesOnlineFrontDoorState::Hosting &&
        Layout.EndpointField.IsInsideOrOn(ScreenPosition))
    {
        CopyOnlineHostEndpoint();
        return true;
    }
    if (Layout.BackButton.IsInsideOrOn(ScreenPosition))
    {
        CancelOnlineFrontDoor();
    }
    return true;
}

bool AEchoesPlayerController::HandleModalOverlayPointer(
    const FVector2D& ScreenPosition,
    const FVector2D& ViewportSize,
    float HudScale)
{
    if (!IsModalOverlayVisible() || bTechnologyPanelVisible)
    {
        return false;
    }
    if (ViewportSize.X <= 0.0f || ViewportSize.Y <= 0.0f)
    {
        SetStatusMessage(
            TEXT("[VIEWPORT_UNAVAILABLE] Overlay selection could not resolve the screen."));
        return true;
    }

    if (bOnlineLocalMenuVisible)
    {
        const FEchoesOnlineLocalMenuLayout Layout =
            FEchoesOnlineLocalMenuLayout::Build(ViewportSize, HudScale);
        if (Layout.ResumeButton.IsInsideOrOn(ScreenPosition))
        {
            TogglePauseMenu();
        }
        else if (Layout.LeaveButton.IsInsideOrOn(ScreenPosition))
        {
            LeaveOnlineMatch();
        }
        return true;
    }

    if (IsOnlineFrontDoorVisible())
    {
        return HandleOnlineFrontDoorPointer(
            ScreenPosition, ViewportSize, HudScale);
    }

    if (bTitleScreenVisible &&
        FEchoesOnlineFrontDoorLayout::BuildTitleEntry(
            ViewportSize, HudScale).IsInsideOrOn(ScreenPosition))
    {
        OpenOnlineFrontDoor();
        return true;
    }

    if (IsSkirmishSetupVisible())
    {
        const FEchoesSkirmishSetupOverlayLayout Layout =
            FEchoesSkirmishSetupOverlayLayout::Build(
                ViewportSize,
                HudScale);
        for (int32 Row = 0; Row < 5; ++Row)
        {
            if (!Layout.SettingRows[Row].IsInsideOrOn(ScreenPosition))
            {
                continue;
            }
            SkirmishSetupFocusRow = Row;
            if (Layout.SettingDecrease[Row].IsInsideOrOn(ScreenPosition))
            {
                DecreaseSkirmishSetting();
            }
            else
            {
                IncreaseSkirmishSetting();
            }
            return true;
        }
        if (Layout.ReviewButton.IsInsideOrOn(ScreenPosition))
        {
            ConfirmPrimaryAction();
        }
        return true;
    }

    if (IsSkirmishDeploymentSummaryVisible())
    {
        const FEchoesSkirmishSummaryOverlayLayout Layout =
            FEchoesSkirmishSummaryOverlayLayout::Build(
                ViewportSize,
                HudScale);
        if (Layout.BackButton.IsInsideOrOn(ScreenPosition))
        {
            ReturnToSkirmishSetup();
        }
        else if (Layout.DeployButton.IsInsideOrOn(ScreenPosition))
        {
            ConfirmMissionBriefing();
        }
        return true;
    }

    if (bPauseMenuVisible)
    {
        const FEchoesPauseOverlayLayout Layout =
            FEchoesPauseOverlayLayout::Build(ViewportSize, HudScale);
        const UEchoesSimulationSubsystem* Bridge =
            GetWorld() != nullptr
                ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
                : nullptr;
        const bool bCanReturnToOperations = Bridge != nullptr &&
            Bridge->GetOperationMode() == EEchoesOperationMode::Skirmish &&
            !Bridge->IsNetworkHumanOpponentEnabled();
        if (Layout.ResumeButton.IsInsideOrOn(ScreenPosition))
        {
            TogglePauseMenu();
        }
        else if (Layout.RestartButton.IsInsideOrOn(ScreenPosition))
        {
            RestartScenario();
        }
        else if (bCanReturnToOperations &&
                 Layout.ReturnButton.IsInsideOrOn(ScreenPosition))
        {
            RequestReturnToOperations();
        }
        else if (Layout.PrimaryButton.IsInsideOrOn(ScreenPosition))
        {
            if (bCanReturnToOperations &&
                IsReturnToOperationsConfirmationArmed())
            {
                RequestReturnToOperations();
            }
            else
            {
                TogglePauseMenu();
            }
        }
        return true;
    }

    if (bMatchResultVisible)
    {
        const FEchoesResultOverlayLayout Layout =
            FEchoesResultOverlayLayout::Build(ViewportSize, HudScale);
        if (IsOnlineMatchResult())
        {
            if (CanLeaveNetworkMatchToOnlineMenu() &&
                Layout.FullButton.IsInsideOrOn(ScreenPosition))
            {
                LeaveOnlineMatch();
            }
            return true;
        }
        const bool bReplayConflict = bCampaignResult &&
            CampaignCommitStatus ==
                EEchoesCampaignCommitStatus::ReplayConflict;
        const bool bHasDistinctPrimaryAction =
            CanReturnCompletedSkirmishToOperations() ||
            CanAdvanceCampaignResult() || bReplayConflict;
        if (bHasDistinctPrimaryAction &&
            Layout.PrimaryButton.IsInsideOrOn(ScreenPosition))
        {
            if (bReplayConflict)
            {
                TogglePauseMenu();
            }
            else
            {
                ConfirmPrimaryAction();
            }
        }
        else if (bHasDistinctPrimaryAction &&
                 Layout.RestartButton.IsInsideOrOn(ScreenPosition))
        {
            RestartScenario();
        }
        else if (!bHasDistinctPrimaryAction &&
                 Layout.FullButton.IsInsideOrOn(ScreenPosition))
        {
            if (bCampaignResult)
            {
                ConfirmPrimaryAction();
            }
            else
            {
                RestartScenario();
            }
        }
        return true;
    }

    return true;
}

void AEchoesPlayerController::TogglePauseMenu()
{
    if (UEchoesGameInstance* EchoesGameInstance = GetEchoesGameInstance();
        EchoesGameInstance != nullptr &&
        EchoesGameInstance->GetOnlineState() !=
            EEchoesOnlineFrontDoorState::Idle)
    {
        CancelOnlineFrontDoor();
        return;
    }
    if (IsActiveOnlineNetworkMatch())
    {
        bOnlineLocalMenuVisible = !bOnlineLocalMenuVisible;
        SetIgnoreMoveInput(bOnlineLocalMenuVisible);
        SetIgnoreLookInput(bOnlineLocalMenuVisible);
        SetStatusMessage(
            bOnlineLocalMenuVisible
                ? TEXT("ONLINE MATCH MENU — local controls are held; the authority state is unchanged. Enter or Escape resumes; choose Leave Online to exit.")
                : TEXT("ONLINE MATCH CONTROLS RESUMED."),
            bOnlineLocalMenuVisible ? 3600.0f : 3.0f);
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_ONLINE_LOCAL_MENU] visible=%s authorityPauseChanged=false restart=false"),
            bOnlineLocalMenuVisible ? TEXT("true") : TEXT("false"));
        return;
    }
    if (bTechnologyPanelVisible)
    {
        ToggleTechnologyPanel();
        return;
    }
    if (IsSkirmishDeploymentSummaryVisible())
    {
        ReturnToSkirmishSetup();
        return;
    }
    if (bMatchResultVisible && bCampaignResult &&
        CampaignCommitStatus == EEchoesCampaignCommitStatus::ReplayConflict)
    {
        PresentTitleScreen();
        if (bTitleScreenVisible)
        {
            SetStatusMessage(
                TEXT("CAMPAIGN JOURNEY — the original irreversible ledger record remains active. Press C to continue from that record, or choose an operation to replay."),
                3600.0f);
            UE_LOG(
                LogEchoes,
                Display,
                TEXT("[ECHOES_REPLAY_CONFLICT_RETURN] destination=campaign_journey ledgerRewritten=false action=pause_scenario"));
        }
        return;
    }
    if (bTitleScreenVisible || bMissionBriefingVisible || bMatchResultVisible)
    {
        return;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Pause is unavailable."));
        return;
    }
    if (Bridge->GetMatchOutcome() != echoes::sim::MatchOutcome::Ongoing)
    {
        SetStatusMessage(TEXT("[MATCH_FINISHED] Press R to restart."));
        return;
    }
    bPauseMenuVisible = !bPauseMenuVisible;
    if (UEchoesInterfaceAudioSubsystem* InterfaceAudio =
            GetWorld()->GetSubsystem<UEchoesInterfaceAudioSubsystem>())
    {
        InterfaceAudio->PlayInterfaceCue(
            bPauseMenuVisible ? EEchoesInterfaceCue::MenuOpen
                              : EEchoesInterfaceCue::MenuClose);
    }
    bReturnToOperationsConfirmationArmed = false;
    ReturnToOperationsConfirmationExpiresAt = 0.0;
    Bridge->SetScenarioPaused(bPauseMenuVisible);
    SetIgnoreMoveInput(bPauseMenuVisible);
    SetIgnoreLookInput(bPauseMenuVisible);
    SetStatusMessage(
        bPauseMenuVisible
            ? Bridge->GetOperationMode() == EEchoesOperationMode::Skirmish &&
                    !Bridge->IsNetworkHumanOpponentEnabled()
                ? TEXT("FIELD MENU — Enter, Escape, or P resumes; R restarts; F10 / Menu returns to Operations with confirmation.")
                : TEXT("FIELD MENU — Enter, Escape, or P resumes; R restarts.")
            : TEXT("MATCH RESUMED."),
        bPauseMenuVisible ? 3600.0f : 3.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_PAUSE_MENU] visible=%s paused=%s"),
        bPauseMenuVisible ? TEXT("true") : TEXT("false"),
        Bridge->IsScenarioPaused() ? TEXT("true") : TEXT("false"));
}

void AEchoesPlayerController::RestartScenario()
{
    // Legacy action mappings may also dispatch the unmodified R action while
    // the Shift+R research chord is held. The chord belongs to ResearchNext.
    if (IsInputKeyDown(EKeys::LeftShift) ||
        IsInputKeyDown(EKeys::RightShift))
    {
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_RESTART_SUPPRESSED] reason=research_chord modifier=shift"));
        return;
    }
    if (IsOnlineMatchResult())
    {
        if (CanLeaveNetworkMatchToOnlineMenu())
        {
            LeaveOnlineMatch();
        }
        else
        {
            SetStatusMessage(
                TEXT("ONLINE RESULT — restart is disabled; waiting briefly for final delivery confirmation."),
                4.0f);
        }
        return;
    }
    if (IsActiveOnlineNetworkMatch())
    {
        SetStatusMessage(
            TEXT("ONLINE MATCH — restart is disabled. Open the Online Match menu to resume or leave."),
            5.0f);
        return;
    }
    if (bTitleScreenVisible || bMissionBriefingVisible)
    {
        return;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    ClearSelection();
    ClearControlGroups();
    bControlGroupAssignmentArmed = false;
    if (Bridge != nullptr && Bridge->RestartPrototypeScenario())
    {
        SynchronizeBoundCampaignProtocol();
        bRuntimeStateKnown = true;
        bTitleScreenVisible = false;
        bPauseMenuVisible = false;
        bTechnologyPanelVisible = false;
        bMatchResultVisible = false;
        bCampaignResult = false;
        bCampaignSuccess = false;
        CampaignConsequence = echoes::sim::FutureWellChoice::Dormant;
        RecordedCampaignConsequence = echoes::sim::FutureWellChoice::Dormant;
        CampaignFinalResolution = EEchoesFinalResolution::None;
        RecordedCampaignFinalResolution = EEchoesFinalResolution::None;
        CampaignCommitStatus = EEchoesCampaignCommitStatus::NotApplicable;
        PresentedMatchOutcome = echoes::sim::MatchOutcome::Ongoing;
        PresentedCampaignOperation = EEchoesOperationMode::Skirmish;
        SetIgnoreMoveInput(false);
        SetIgnoreLookInput(false);
        SetStatusMessage(
            Bridge->GetOperationMode() == EEchoesOperationMode::CampaignPrologue
                ? TEXT("MISSION RESTARTED — Mara Vey's archive recovery begins again from the deterministic initial state.")
            : Bridge->GetOperationMode() ==
                    EEchoesOperationMode::CampaignSevenAccounts
                ? TEXT("MISSION RESTARTED — Oruun's migration begins again from the inherited route state.")
            : Bridge->GetOperationMode() ==
                    EEchoesOperationMode::CampaignCityReserve
                ? TEXT("MISSION RESTARTED — Mara Vey's reserve grid returns to its deterministic initial state.")
            : Bridge->GetOperationMode() ==
                    EEchoesOperationMode::CampaignUnburiedRoad
                ? TEXT("MISSION RESTARTED — Oruun's road recovery returns to its deterministic initial state.")
            : Bridge->GetOperationMode() ==
                    EEchoesOperationMode::CampaignTermsOfContinuance
                ? TEXT("MISSION RESTARTED — the Meridian-authoritative treaty proxy scenario returns to its deterministic initial state.")
            : Bridge->GetOperationMode() ==
                    EEchoesOperationMode::CampaignNamesWithoutBirths
                ? TEXT("MISSION RESTARTED — Talar's protected census recovery returns to its deterministic initial state.")
            : Bridge->GetOperationMode() ==
                    EEchoesOperationMode::CampaignShapeOfSilence
                ? TEXT("MISSION RESTARTED — Oruun's listening operation returns to its deterministic initial state.")
            : Bridge->GetOperationMode() ==
                    EEchoesOperationMode::CampaignShapeBesideUs
                ? TEXT("MISSION RESTARTED — Talar's overlap contact operation returns to its deterministic initial state.")
            : Bridge->GetOperationMode() ==
                    EEchoesOperationMode::CampaignReserveAuthority
                ? TEXT("MISSION RESTARTED — Mara's district allocation returns to its deterministic initial state.")
            : Bridge->GetOperationMode() ==
                    EEchoesOperationMode::CampaignChoirAtLumeReach
                ? TEXT("MISSION RESTARTED — Oruun's Lume Reach contact operation returns to its deterministic initial state; Mara remains liaison-only.")
            : Bridge->GetOperationMode() ==
                    EEchoesOperationMode::CampaignNoNeutralLedger
                ? TEXT("MISSION RESTARTED — Oruun's coalition admission route returns to its deterministic ten-record initial state.")
            : Bridge->GetOperationMode() ==
                    EEchoesOperationMode::CampaignFutureThatWon
                ? TEXT("MISSION RESTARTED — Oruun and the independent verifier return to the deterministic eleven-record restoration-demonstrator state.")
            : Bridge->GetOperationMode() ==
                    EEchoesOperationMode::CampaignAssemblyOfTheMissing
                ? TEXT("MISSION RESTARTED — Oruun, the independent verifier, and the three neutral public record interfaces return to the deterministic twelve-record assembly state.")
            : Bridge->GetOperationMode() ==
                    EEchoesOperationMode::CampaignSeveralVoicesOneCommand
                ? TEXT("MISSION RESTARTED — Neme and the protected Hollow Choir voices return to the deterministic thirteen-record crisis state.")
            : Bridge->GetOperationMode() ==
                    EEchoesOperationMode::CampaignTheBrokenSun
                ? TEXT("MISSION RESTARTED — Neme, Mara, Oruun, Talar, and the Hollow Choir command force return to the deterministic fourteen-record final-operation state.")
                : TEXT("MATCH RESTARTED — deterministic initial state restored."));
        UE_LOG(LogEchoes, Display, TEXT("[ECHOES_RESULT_RESTARTED] outcome=0"));
    }
    else
    {
        NotifyRuntimeFailure(TEXT("ECHOES_MATCH_RESTART_FAILED"));
    }
}

void AEchoesPlayerController::SynchronizeBoundCampaignProtocol()
{
    const UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr)
    {
        return;
    }
    if (Bridge->GetOperationMode() ==
        EEchoesOperationMode::CampaignNoNeutralLedger)
    {
        FutureWellChoice =
            Bridge->GetNoNeutralLedgerPlan().LumeProtocol;
    }
    else if (Bridge->GetOperationMode() ==
             EEchoesOperationMode::CampaignFutureThatWon)
    {
        FutureWellChoice =
            Bridge->GetFutureThatWonPlan().RecordedProtocol;
    }
    else if (Bridge->GetOperationMode() ==
             EEchoesOperationMode::CampaignAssemblyOfTheMissing)
    {
        FutureWellChoice =
            Bridge->GetAssemblyOfTheMissingPlan().RecordedProtocol;
    }
    else if (Bridge->GetOperationMode() ==
             EEchoesOperationMode::CampaignSeveralVoicesOneCommand)
    {
        FutureWellChoice =
            Bridge->GetSeveralVoicesOneCommandPlan().RecordedProtocol;
    }
    else if (Bridge->GetOperationMode() ==
             EEchoesOperationMode::CampaignTheBrokenSun)
    {
        FutureWellChoice = Bridge->GetBrokenSunPlan().RecordedProtocol;
    }
}

void AEchoesPlayerController::SetFutureWellChoice(
    echoes::sim::FutureWellChoice Choice)
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    const UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge != nullptr &&
        Bridge->GetOperationMode() ==
            EEchoesOperationMode::CampaignNoNeutralLedger)
    {
        const FEchoesNoNeutralLedgerPlan Plan =
            Bridge->GetNoNeutralLedgerPlan();
        FutureWellChoice = Plan.LumeProtocol;
        SetStatusMessage(
            FString::Printf(
                TEXT("No Neutral Ledger admits only the recorded %s protocol. Right-click the Lume Well with a worker after both evidence channels attest."),
                Plan.ProtocolDisplayName),
            7.0f);
        return;
    }
    if (Bridge != nullptr &&
        Bridge->GetOperationMode() ==
            EEchoesOperationMode::CampaignFutureThatWon)
    {
        const FEchoesFutureThatWonPlan Plan =
            Bridge->GetFutureThatWonPlan();
        FutureWellChoice = Plan.RecordedProtocol;
        SetStatusMessage(
            FString::Printf(
                TEXT("The Future That Won admits only the recorded %s protocol. Right-click the Future Well with a worker after independent readback and both district inputs are verified."),
                Plan.ProtocolDisplayName),
            8.0f);
        return;
    }
    if (Bridge != nullptr &&
        Bridge->GetOperationMode() ==
            EEchoesOperationMode::CampaignAssemblyOfTheMissing)
    {
        const FEchoesAssemblyOfTheMissingPlan Plan =
            Bridge->GetAssemblyOfTheMissingPlan();
        FutureWellChoice = Plan.RecordedProtocol;
        SetStatusMessage(
            FString::Printf(
                TEXT("Assembly of the Missing retains the recorded %s protocol as ledger context only. This operation exposes no selectable Future Well protocol."),
                Plan.ProtocolDisplayName),
            8.0f);
        return;
    }
    if (Bridge != nullptr &&
        Bridge->GetOperationMode() ==
            EEchoesOperationMode::CampaignSeveralVoicesOneCommand)
    {
        const FEchoesSeveralVoicesOneCommandPlan Plan =
            Bridge->GetSeveralVoicesOneCommandPlan();
        FutureWellChoice = Plan.RecordedProtocol;
        SetStatusMessage(
            FString::Printf(
                TEXT("Several Voices, One Command retains the recorded %s protocol as ledger context only. This operation exposes no selectable Future Well protocol."),
                Plan.ProtocolDisplayName),
            8.0f);
        return;
    }
    if (Bridge != nullptr &&
        Bridge->GetOperationMode() ==
            EEchoesOperationMode::CampaignTheBrokenSun)
    {
        const FEchoesBrokenSunPlan Plan = Bridge->GetBrokenSunPlan();
        FutureWellChoice = Plan.RecordedProtocol;
        SetStatusMessage(
            FString::Printf(
                TEXT("The Broken Sun retains the recorded %s protocol as ledger context. Choose an eligible final resolution with Shift+1 through Shift+4 after assembling the accord."),
                Plan.ProtocolDisplayName),
            9.0f);
        return;
    }
    FutureWellChoice = Choice;
    SetStatusMessage(
        FString::Printf(
            TEXT("Future Well protocol set to %s. Right-click a dormant Well with a worker selected."),
            *GetFutureWellChoiceLabel()),
        5.0f);
}

void AEchoesPlayerController::SetStatusMessage(
    const FString& Message,
    float DisplaySeconds)
{
    StatusMessage = Message;
    if (bMatchResultVisible && bCampaignResult &&
        CampaignCommitStatus == EEchoesCampaignCommitStatus::ReplayConflict)
    {
        StatusMessage.ReplaceInline(
            TEXT("Campaign progress was not saved."),
            TEXT("The replay outcome was not recorded; the existing campaign record remains authoritative."));
        StatusMessage.ReplaceInline(
            TEXT("Press R to replay."),
            TEXT("Press Escape to return to the campaign journey or R to replay."));
    }
    if (bMatchResultVisible && CanAdvanceCampaignResult())
    {
        if (PresentedCampaignOperation ==
            EEchoesOperationMode::CampaignTheBrokenSun)
        {
            StatusMessage.ReplaceInline(
                TEXT("Press R to replay the battle without rewriting it."),
                TEXT("Press Enter to return to title or R to replay the battle without rewriting it."));
            StatusMessage.ReplaceInline(
                TEXT("Press R to replay."),
                TEXT("Press Enter to return to title or R to replay."));
        }
        else
        {
            StatusMessage.ReplaceInline(
                TEXT("Press R to replay."),
                TEXT("Press Enter to continue the campaign or R to replay."));
        }
    }
    StatusMessageExpiresAt =
        GetWorld() != nullptr ? GetWorld()->GetTimeSeconds() + DisplaySeconds : 0.0;
    // A leading bracketed token is this controller's stable failure-reason
    // convention. Pair every such rejection with the distinct rejection cue;
    // the subsystem's admission window absorbs repeat-key bursts.
    if (StatusMessage.StartsWith(TEXT("[")) && GetWorld() != nullptr)
    {
        if (UEchoesInterfaceAudioSubsystem* InterfaceAudio =
                GetWorld()->GetSubsystem<UEchoesInterfaceAudioSubsystem>())
        {
            InterfaceAudio->PlayInterfaceCue(EEchoesInterfaceCue::Reject);
        }
    }
    UE_LOG(LogEchoes, Display, TEXT("[ECHOES_PLAYER_FEEDBACK] %s"), *StatusMessage);
}

bool AEchoesPlayerController::IsDraggingSelection() const
{
    return bSelectionButtonDown &&
           FVector2D::Distance(
               SelectionStartScreenPosition,
               SelectionCurrentScreenPosition) >= DragSelectionThresholdPixels;
}

FVector2D AEchoesPlayerController::GetSelectionStartScreenPosition() const
{
    return SelectionStartScreenPosition;
}

FVector2D AEchoesPlayerController::GetSelectionCurrentScreenPosition() const
{
    return SelectionCurrentScreenPosition;
}

const TArray<uint32>& AEchoesPlayerController::GetSelectedEntityIds() const
{
    return SelectedEntityIds;
}

echoes::sim::FutureWellChoice AEchoesPlayerController::GetFutureWellChoice() const
{
    return FutureWellChoice;
}

FString AEchoesPlayerController::GetFutureWellChoiceLabel() const
{
    switch (FutureWellChoice)
    {
        case echoes::sim::FutureWellChoice::Harvest:
            return TEXT("HARVEST");
        case echoes::sim::FutureWellChoice::Preserve:
            return TEXT("PRESERVE");
        case echoes::sim::FutureWellChoice::Reshape:
            return TEXT("RESHAPE");
        case echoes::sim::FutureWellChoice::Dormant:
            return TEXT("DORMANT");
    }
    return TEXT("UNKNOWN");
}

FString AEchoesPlayerController::GetStatusMessage() const
{
    if (GetWorld() == nullptr || GetWorld()->GetTimeSeconds() > StatusMessageExpiresAt)
    {
        return FString();
    }
    return StatusMessage;
}

FString AEchoesPlayerController::CommandLabel(
    echoes::sim::CommandType CommandType) const
{
    switch (CommandType)
    {
        case echoes::sim::CommandType::Stop:
            return TEXT("STOP");
        case echoes::sim::CommandType::Move:
            return TEXT("MOVE");
        case echoes::sim::CommandType::Gather:
            return TEXT("GATHER MATTER");
        case echoes::sim::CommandType::Deliver:
            return TEXT("DELIVER MATTER");
        case echoes::sim::CommandType::Build:
            return TEXT("BUILD");
        case echoes::sim::CommandType::Attack:
            return TEXT("ATTACK");
        case echoes::sim::CommandType::FutureWell:
            return FString::Printf(TEXT("FUTURE WELL: %s"), *GetFutureWellChoiceLabel());
        case echoes::sim::CommandType::Produce:
            return TEXT("PRODUCE");
        case echoes::sim::CommandType::AttackMove:
            return TEXT("ATTACK-MOVE");
        case echoes::sim::CommandType::Hold:
            return TEXT("HOLD POSITION");
        case echoes::sim::CommandType::Guard:
            return TEXT("GUARD");
        case echoes::sim::CommandType::Patrol:
            return TEXT("PATROL");
        case echoes::sim::CommandType::ToggleDeploy:
            return TEXT("TOGGLE BULWARK DEPLOYMENT");
        case echoes::sim::CommandType::ActivateRelaySupply:
            return TEXT("ACTIVATE RELAY SUPPLY");
        case echoes::sim::CommandType::ToggleWaystoneRoot:
            return TEXT("TOGGLE WAYSTONE ROOT");
        case echoes::sim::CommandType::AdaptWarform:
            return TEXT("ADAPT WARFORM");
        case echoes::sim::CommandType::RaiseMineralCover:
            return TEXT("RAISE MINERAL COVER");
        case echoes::sim::CommandType::Research:
            return TEXT("RESEARCH");
        case echoes::sim::CommandType::ReconcileToManifest:
            return TEXT("RECONCILE TO MANIFEST");
        case echoes::sim::CommandType::ReconcileToPossible:
            return TEXT("RECONCILE TO POSSIBLE");
    }
    return TEXT("ORDER");
}
