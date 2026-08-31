#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesGameInstance.h"
#include "EchoesOnlineFrontDoorLayout.h"
#include "EchoesPlayerController.h"
#include "EchoesSkirmishSetup.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"

#if UE_BUILD_DEVELOPMENT && (PLATFORM_MAC || PLATFORM_UNIX)
#include <sys/stat.h>
#include <unistd.h>
#endif

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesOnlineFrontDoorTest,
    "Echoes.Runtime.Network.OnlineFrontDoor",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesOnlineFrontDoorTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    struct FEndpointCase final
    {
        const TCHAR* Candidate;
        const TCHAR* Expected;
    };
    const FEndpointCase ValidEndpoints[] = {
        {TEXT("127.0.0.1:7777"), TEXT("127.0.0.1:7777")},
        {TEXT("127.0.0.2:7801"), TEXT("127.0.0.2:7801")},
        {TEXT("127.255.255.255:65535"),
         TEXT("127.255.255.255:65535")},
        {TEXT("127.000.000.001:07801"), TEXT("127.0.0.1:7801")},
        {TEXT("LOCALHOST:7777"), TEXT("127.0.0.1:7777")}};
    for (const FEndpointCase& Endpoint : ValidEndpoints)
    {
        FString Normalized;
        FString Error;
        TestTrue(
            FString::Printf(
                TEXT("Direct endpoint is accepted: %s"),
                Endpoint.Candidate),
            UEchoesGameInstance::NormalizeDirectEndpoint(
                Endpoint.Candidate, Normalized, Error));
        TestEqual(
            FString::Printf(
                TEXT("Direct endpoint is canonicalized: %s"),
                Endpoint.Candidate),
            Normalized,
            FString(Endpoint.Expected));
        TestTrue(TEXT("Accepted endpoint has no error"), Error.IsEmpty());
    }

    const TCHAR* InvalidEndpoints[] = {
        TEXT(""),
        TEXT("127.0.0.1"),
        TEXT("127.0.0.1:0"),
        TEXT("127.0.0.1:65536"),
        TEXT("256.1.1.1:7777"),
        TEXT("0.0.0.0:7777"),
        TEXT("126.255.255.255:7777"),
        TEXT("128.0.0.1:7777"),
        TEXT("192.168.50.12:7801"),
        TEXT("8.8.8.8:7777"),
        TEXT("example-lan.local:7801"),
        TEXT("localhost.:7777"),
        TEXT("host.localhost:7777"),
        TEXT("-bad.local:7777"),
        TEXT("bad-.local:7777"),
        TEXT("http://127.0.0.1:7777"),
        TEXT("127.0.0.1:7777?listen"),
        TEXT("host name:7777"),
        TEXT("fe80::1:7777"),
        TEXT("[::1]:7777"),
        TEXT("[::ffff:127.0.0.1]:7777"),
        TEXT("::ffff:127.0.0.1:7777")};
    for (const TCHAR* Endpoint : InvalidEndpoints)
    {
        FString Normalized;
        FString Error;
        TestFalse(
            FString::Printf(
                TEXT("Unsafe or incomplete endpoint is rejected: %s"),
                Endpoint),
            UEchoesGameInstance::NormalizeDirectEndpoint(
                Endpoint, Normalized, Error));
        TestTrue(TEXT("Rejected endpoint has a stable error"),
                 !Error.IsEmpty());
        TestTrue(TEXT("Rejected endpoint has no travel target"),
                 Normalized.IsEmpty());
    }

#if UE_BUILD_DEVELOPMENT
    TestTrue(
        TEXT("Development host requires an explicit IPv4 loopback bind"),
        UEchoesGameInstance::HasExplicitDevelopmentLoopbackBind(
            TEXT("-game -MULTIHOME=127.0.0.1 -port=7777")));
    TestFalse(
        TEXT("Ambiguous zero-padded bind spelling fails closed"),
        UEchoesGameInstance::HasExplicitDevelopmentLoopbackBind(
            TEXT("-MULTIHOME=127.000.000.001")));
    TestFalse(
        TEXT("Missing bind fails closed"),
        UEchoesGameInstance::HasExplicitDevelopmentLoopbackBind(
            TEXT("-game -port=7777")));
    TestFalse(
        TEXT("Wildcard bind fails closed"),
        UEchoesGameInstance::HasExplicitDevelopmentLoopbackBind(
            TEXT("-MULTIHOME=0.0.0.0")));
    TestFalse(
        TEXT("LAN bind fails closed"),
        UEchoesGameInstance::HasExplicitDevelopmentLoopbackBind(
            TEXT("-MULTIHOME=192.168.50.12")));
    TestFalse(
        TEXT("A different loopback alias is not published as the host"),
        UEchoesGameInstance::HasExplicitDevelopmentLoopbackBind(
            TEXT("-MULTIHOME=127.0.0.2")));
#endif

#if UE_BUILD_DEVELOPMENT && (PLATFORM_MAC || PLATFORM_UNIX)
    TArray<FString> CredentialTestRoots;
    const auto MakeCredentialRunDirectory = [&CredentialTestRoots]()
    {
        const FString Root = FPaths::Combine(
            FPlatformProcess::UserTempDir(),
            FString::Printf(
                TEXT("EchoesCredentialTest-%s"),
                *FGuid::NewGuid().ToString(EGuidFormats::Digits)));
        const FString RunDirectory = FPaths::Combine(
            Root,
            FString::Printf(
                TEXT("run.%s"),
                *FGuid::NewGuid().ToString(EGuidFormats::Digits)));
        IFileManager::Get().MakeDirectory(*RunDirectory, true);
        (void)chmod(TCHAR_TO_UTF8(*RunDirectory), 0700);
        CredentialTestRoots.Add(Root);
        return RunDirectory;
    };
    const auto WriteCredentialFixture = [this](
        const FString& Path,
        const FString& Content,
        uint32 Mode)
    {
        const bool bWritten = FFileHelper::SaveStringToFile(
            Content,
            *Path,
            FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
        TestTrue(TEXT("Credential fixture is created"), bWritten);
        const int32 ChmodResult = chmod(TCHAR_TO_UTF8(*Path), Mode);
        TestEqual(TEXT("Credential fixture mode is applied"),
                  ChmodResult, 0);
        return bWritten && ChmodResult == 0;
    };

    constexpr TCHAR CredentialFixture[] =
        TEXT("00112233445566778899aabbccddeeff");
    FString NormalizedCredentialPath;
    FString CredentialFileReason;
    const FString PositiveRun = MakeCredentialRunDirectory();
    const FString PositiveFile = FPaths::Combine(
        PositiveRun, TEXT("EchoesResumeCredential.bin"));
    WriteCredentialFixture(PositiveFile, FString(), 0600);
    TestTrue(
        TEXT("Empty owner-only credential file is a valid staging target"),
        echoes::network::testing::
            ValidateDevelopmentCredentialStagingFile(
                PositiveFile,
                NormalizedCredentialPath,
                CredentialFileReason));
    TestTrue(
        TEXT("Bounded credential is staged without changing file policy"),
        echoes::network::testing::StageDevelopmentResumeCredential(
            NormalizedCredentialPath,
            CredentialFixture,
            CredentialFileReason));
    TestEqual(TEXT("Staged credential is exactly 32 bytes"),
              IFileManager::Get().FileSize(*PositiveFile),
              static_cast<int64>(32));
    FString ConsumedCredential;
    TestTrue(
        TEXT("Owner-only credential file is consumed exactly once"),
        echoes::network::testing::ConsumeDevelopmentResumeCredential(
            PositiveFile, ConsumedCredential, CredentialFileReason));
    TestEqual(TEXT("Consumed credential remains exact in memory"),
              ConsumedCredential,
              FString(CredentialFixture));
    TestFalse(TEXT("Credential file is deleted before submission"),
              IFileManager::Get().FileExists(*PositiveFile));
    ConsumedCredential.Reset();
    TestFalse(
        TEXT("A consumed credential file cannot be reused"),
        echoes::network::testing::ConsumeDevelopmentResumeCredential(
            PositiveFile, ConsumedCredential, CredentialFileReason));

    const FString NonemptyRun = MakeCredentialRunDirectory();
    const FString NonemptyFile = FPaths::Combine(
        NonemptyRun, TEXT("EchoesResumeCredential.bin"));
    WriteCredentialFixture(NonemptyFile, CredentialFixture, 0600);
    TestFalse(
        TEXT("Nonempty phase-one staging file fails closed"),
        echoes::network::testing::
            ValidateDevelopmentCredentialStagingFile(
                NonemptyFile,
                NormalizedCredentialPath,
                CredentialFileReason));

    const FString WrongModeRun = MakeCredentialRunDirectory();
    const FString WrongModeFile = FPaths::Combine(
        WrongModeRun, TEXT("EchoesResumeCredential.bin"));
    WriteCredentialFixture(WrongModeFile, FString(), 0644);
    TestFalse(
        TEXT("Credential file with group or other access fails closed"),
        echoes::network::testing::
            ValidateDevelopmentCredentialStagingFile(
                WrongModeFile,
                NormalizedCredentialPath,
                CredentialFileReason));

    const FString WrongDirectoryModeRun = MakeCredentialRunDirectory();
    const FString WrongDirectoryModeFile = FPaths::Combine(
        WrongDirectoryModeRun, TEXT("EchoesResumeCredential.bin"));
    WriteCredentialFixture(WrongDirectoryModeFile, FString(), 0600);
    (void)chmod(TCHAR_TO_UTF8(*WrongDirectoryModeRun), 0755);
    TestFalse(
        TEXT("Credential directory with group or other access fails closed"),
        echoes::network::testing::
            ValidateDevelopmentCredentialStagingFile(
                WrongDirectoryModeFile,
                NormalizedCredentialPath,
                CredentialFileReason));
    (void)chmod(TCHAR_TO_UTF8(*WrongDirectoryModeRun), 0700);

    const FString HardLinkRun = MakeCredentialRunDirectory();
    const FString HardLinkFile = FPaths::Combine(
        HardLinkRun, TEXT("EchoesResumeCredential.bin"));
    const FString HardLinkPeer =
        FPaths::Combine(HardLinkRun, TEXT("HardLinkPeer.bin"));
    WriteCredentialFixture(HardLinkFile, FString(), 0600);
    TestEqual(TEXT("Hard-link fixture is created"),
              link(
                  TCHAR_TO_UTF8(*HardLinkFile),
                  TCHAR_TO_UTF8(*HardLinkPeer)),
              0);
    TestFalse(
        TEXT("Multiply linked credential file fails closed"),
        echoes::network::testing::
            ValidateDevelopmentCredentialStagingFile(
                HardLinkFile,
                NormalizedCredentialPath,
                CredentialFileReason));

    const FString SymlinkRun = MakeCredentialRunDirectory();
    const FString SymlinkTarget =
        FPaths::Combine(SymlinkRun, TEXT("Target.bin"));
    const FString SymlinkFile = FPaths::Combine(
        SymlinkRun, TEXT("EchoesResumeCredential.bin"));
    WriteCredentialFixture(SymlinkTarget, FString(), 0600);
    TestEqual(TEXT("Symbolic-link fixture is created"),
              symlink(
                  TCHAR_TO_UTF8(*SymlinkTarget),
                  TCHAR_TO_UTF8(*SymlinkFile)),
              0);
    TestFalse(
        TEXT("Symbolic-link credential path fails closed"),
        echoes::network::testing::
            ValidateDevelopmentCredentialStagingFile(
                SymlinkFile,
                NormalizedCredentialPath,
                CredentialFileReason));
    TestEqual(TEXT("Credential leaf symbolic link is removed"),
              unlink(TCHAR_TO_UTF8(*SymlinkFile)),
              0);

    const FString FifoRun = MakeCredentialRunDirectory();
    const FString FifoFile = FPaths::Combine(
        FifoRun, TEXT("EchoesResumeCredential.bin"));
    TestEqual(TEXT("FIFO credential fixture is created"),
              mkfifo(TCHAR_TO_UTF8(*FifoFile), 0600),
              0);
    TestFalse(
        TEXT("FIFO staging path fails closed without blocking"),
        echoes::network::testing::ValidateDevelopmentCredentialStagingFile(
            FifoFile,
            NormalizedCredentialPath,
            CredentialFileReason));
    TestFalse(
        TEXT("FIFO consume path fails closed without blocking"),
        echoes::network::testing::ConsumeDevelopmentResumeCredential(
            FifoFile,
            ConsumedCredential,
            CredentialFileReason));
    TestEqual(TEXT("FIFO credential fixture is removed"),
              unlink(TCHAR_TO_UTF8(*FifoFile)),
              0);

    const FString DirectoryLeafRun = MakeCredentialRunDirectory();
    const FString DirectoryLeaf = FPaths::Combine(
        DirectoryLeafRun, TEXT("EchoesResumeCredential.bin"));
    TestTrue(TEXT("Directory credential fixture is created"),
             IFileManager::Get().MakeDirectory(*DirectoryLeaf, false));
    (void)chmod(TCHAR_TO_UTF8(*DirectoryLeaf), 0700);
    TestFalse(
        TEXT("Directory staging path fails closed as non-regular"),
        echoes::network::testing::ValidateDevelopmentCredentialStagingFile(
            DirectoryLeaf,
            NormalizedCredentialPath,
            CredentialFileReason));
    TestFalse(
        TEXT("Directory consume path fails closed as non-regular"),
        echoes::network::testing::ConsumeDevelopmentResumeCredential(
            DirectoryLeaf,
            ConsumedCredential,
            CredentialFileReason));
    TestTrue(TEXT("Directory credential fixture is removed"),
             IFileManager::Get().DeleteDirectory(
                 *DirectoryLeaf, false, false));

    const FString InvalidContentRun = MakeCredentialRunDirectory();
    const FString InvalidContentFile = FPaths::Combine(
        InvalidContentRun, TEXT("EchoesInvalidResumeCredential.bin"));
    WriteCredentialFixture(
        InvalidContentFile,
        TEXT("gggggggggggggggggggggggggggggggg"),
        0600);
    TestFalse(
        TEXT("Malformed bounded-size credential fails closed"),
        echoes::network::testing::ConsumeDevelopmentResumeCredential(
            InvalidContentFile,
            ConsumedCredential,
            CredentialFileReason));
    TestFalse(TEXT("Malformed credential file is still consumed"),
              IFileManager::Get().FileExists(*InvalidContentFile));

    const FString ShortContentRun = MakeCredentialRunDirectory();
    const FString ShortContentFile = FPaths::Combine(
        ShortContentRun, TEXT("EchoesInvalidResumeCredential.bin"));
    WriteCredentialFixture(ShortContentFile, TEXT("00112233"), 0600);
    TestFalse(
        TEXT("Short credential file fails closed"),
        echoes::network::testing::ConsumeDevelopmentResumeCredential(
            ShortContentFile,
            ConsumedCredential,
            CredentialFileReason));
    TestFalse(TEXT("Short credential file is consumed on rejection"),
              IFileManager::Get().FileExists(*ShortContentFile));

    const FString OversizedContentRun = MakeCredentialRunDirectory();
    const FString OversizedContentFile = FPaths::Combine(
        OversizedContentRun, TEXT("EchoesInvalidResumeCredential.bin"));
    WriteCredentialFixture(
        OversizedContentFile,
        TEXT("00112233445566778899aabbccddeeff0"),
        0600);
    TestFalse(
        TEXT("Oversized credential file fails closed"),
        echoes::network::testing::ConsumeDevelopmentResumeCredential(
            OversizedContentFile,
            ConsumedCredential,
            CredentialFileReason));
    TestFalse(TEXT("Oversized credential file is consumed on rejection"),
              IFileManager::Get().FileExists(*OversizedContentFile));

    const FString ParentSymlinkTargetRun =
        MakeCredentialRunDirectory();
    const FString ParentSymlinkTargetFile = FPaths::Combine(
        ParentSymlinkTargetRun, TEXT("EchoesResumeCredential.bin"));
    WriteCredentialFixture(ParentSymlinkTargetFile, FString(), 0600);
    const FString ParentSymlinkRoot = FPaths::Combine(
        FPlatformProcess::UserTempDir(),
        FString::Printf(
            TEXT("EchoesCredentialTest-%s"),
            *FGuid::NewGuid().ToString(EGuidFormats::Digits)));
    IFileManager::Get().MakeDirectory(*ParentSymlinkRoot, true);
    (void)chmod(TCHAR_TO_UTF8(*ParentSymlinkRoot), 0700);
    CredentialTestRoots.Add(ParentSymlinkRoot);
    const FString ParentSymlinkRun =
        FPaths::Combine(ParentSymlinkRoot, TEXT("run.parent-link"));
    TestEqual(TEXT("Parent-directory symbolic-link fixture is created"),
              symlink(
                  TCHAR_TO_UTF8(*ParentSymlinkTargetRun),
                  TCHAR_TO_UTF8(*ParentSymlinkRun)),
              0);
    TestFalse(
        TEXT("Symbolic-link credential parent fails closed"),
        echoes::network::testing::
            ValidateDevelopmentCredentialStagingFile(
                FPaths::Combine(
                    ParentSymlinkRun,
                    TEXT("EchoesResumeCredential.bin")),
                NormalizedCredentialPath,
                CredentialFileReason));
    TestEqual(TEXT("Credential parent symbolic link is removed"),
              unlink(TCHAR_TO_UTF8(*ParentSymlinkRun)),
              0);

    const FString BadRoot = FPaths::Combine(
        FPlatformProcess::UserTempDir(),
        TEXT("not-a-private-run"),
        TEXT("EchoesResumeCredential.bin"));
    TestFalse(
        TEXT("Credential path outside a run-prefixed root fails closed"),
        echoes::network::testing::
            ValidateDevelopmentCredentialStagingFile(
                BadRoot,
                NormalizedCredentialPath,
                CredentialFileReason));
    const FString BadLeaf = FPaths::Combine(
        PositiveRun, TEXT("UnexpectedCredentialName.bin"));
    TestFalse(
        TEXT("Unexpected credential leaf name fails closed"),
        echoes::network::testing::
            ValidateDevelopmentCredentialStagingFile(
                BadLeaf,
                NormalizedCredentialPath,
                CredentialFileReason));

    for (const FString& Root : CredentialTestRoots)
    {
        TestTrue(TEXT("Credential test fixture root is removed"),
                 IFileManager::Get().DeleteDirectory(*Root, false, true));
        TestFalse(TEXT("Credential test fixture leaves no directory"),
                  IFileManager::Get().DirectoryExists(*Root));
    }
#endif

    UEchoesGameInstance* GameInstance =
        NewObject<UEchoesGameInstance>(GetTransientPackage());
    if (!TestNotNull(TEXT("Online state model can be instantiated"),
                     GameInstance))
    {
        return false;
    }
    TestEqual(TEXT("Online front door begins idle"),
              GameInstance->GetOnlineState(),
              EEchoesOnlineFrontDoorState::Idle);
    GameInstance->OpenOnlineFrontDoor();
    TestEqual(TEXT("F8 opens Join Setup"),
              GameInstance->GetOnlineState(),
              EEchoesOnlineFrontDoorState::JoinSetup);
    GameInstance->FocusPreviousOnlineAction();
    TestEqual(TEXT("Focus wraps backward"),
              GameInstance->GetOnlineFocusIndex(), 3);
    GameInstance->FocusNextOnlineAction();
    TestEqual(TEXT("Focus wraps forward"),
              GameInstance->GetOnlineFocusIndex(), 0);
    GameInstance->FocusOnlineAction(99);
    TestEqual(TEXT("Focus clamps to a real action"),
              GameInstance->GetOnlineFocusIndex(), 3);
    TestFalse(TEXT("Endpoint editing is confined to its field"),
              GameInstance->AppendEndpointCharacter(TEXT('a')));
    GameInstance->FocusOnlineAction(1);
    GameInstance->SetDirectConnectEndpoint(TEXT("host:777"));
    TestTrue(TEXT("Endpoint field accepts a bounded address character"),
             GameInstance->AppendEndpointCharacter(TEXT('7')));
    TestEqual(TEXT("Endpoint field appends exactly one character"),
              GameInstance->GetDirectConnectEndpoint(),
              FString(TEXT("host:7777")));
    TestTrue(TEXT("Endpoint field supports backspace"),
             GameInstance->BackspaceEndpointCharacter());
    TestEqual(TEXT("Backspace removes exactly one character"),
              GameInstance->GetDirectConnectEndpoint(),
              FString(TEXT("host:777")));

    GameInstance->ReportOnlineFailure(TEXT("ONLINE_ADDRESS_PORT_REQUIRED"));
    TestEqual(TEXT("Failure is retained in the failed state"),
              GameInstance->GetOnlineState(),
              EEchoesOnlineFrontDoorState::Failed);
    TestTrue(TEXT("Failure is readable to a player"),
             !GameInstance->GetOnlineFailureMessage().IsEmpty());
    GameInstance->RetryOnlineFrontDoor(nullptr);
    TestEqual(TEXT("Retry without a resume token returns to Join Setup"),
              GameInstance->GetOnlineState(),
              EEchoesOnlineFrontDoorState::JoinSetup);

    constexpr TCHAR FirstCredential[] =
        TEXT("00112233445566778899aabbccddeeff");
    constexpr TCHAR RotatedCredential[] =
        TEXT("ffeeddccbbaa99887766554433221100");
    GameInstance->BoundReconnectEndpoint = TEXT("127.0.0.1:7777");
    GameInstance->DirectConnectEndpoint = TEXT("127.0.0.1:7777");
    GameInstance->StoreNetworkResumeCredential(FirstCredential, 120.0f);
    TestFalse(TEXT("Credential does not age while the match is connected"),
              GameInstance->HasUsableReconnectContext());
    GameInstance->NetworkResumeExpiresAtSeconds =
        FPlatformTime::Seconds() - 600.0;
    TestTrue(TEXT("Disconnect arms a fresh reconnect grace window"),
             GameInstance->ArmReconnectWindow());
    TestTrue(TEXT("Valid credential and bound endpoint enable reconnect"),
             GameInstance->HasUsableReconnectContext());
    GameInstance->bReconnectAttemptPending = true;
    FString PendingCredential;
    TestTrue(TEXT("Reconnect auto-submits only the bound credential"),
             GameInstance->TryGetPendingReconnectCredential(
                 PendingCredential));
    TestEqual(TEXT("Reconnect credential is exact"),
              PendingCredential, FString(FirstCredential));
    GameInstance->MarkReconnectAttemptAccepted();
    TestFalse(TEXT("Accepted reconnect is not submitted twice"),
              GameInstance->TryGetPendingReconnectCredential(
                  PendingCredential));
    GameInstance->StoreNetworkResumeCredential(
        RotatedCredential, 120.0f);
    TestEqual(TEXT("A resumed seat rotates its credential"),
              GameInstance->NetworkResumeCredential,
              FString(RotatedCredential));
    TestFalse(TEXT("Rotated credential waits for a future disconnect"),
              GameInstance->HasUsableReconnectContext());
    TestTrue(TEXT("A future disconnect arms the rotated credential"),
             GameInstance->ArmReconnectWindow());
    GameInstance->NetworkResumeExpiresAtSeconds =
        FPlatformTime::Seconds() - 1.0;
    TestFalse(TEXT("Expired reconnect context fails closed"),
              GameInstance->HasUsableReconnectContext());
    TestEqual(TEXT("Expired reconnect countdown reaches zero"),
              GameInstance->GetReconnectSecondsRemaining(), 0);

    GameInstance->StoreNetworkResumeCredential(
        RotatedCredential, 120.0f);
    TestTrue(TEXT("Address-change fixture arms reconnect context"),
             GameInstance->ArmReconnectWindow());
    GameInstance->SetDirectConnectEndpoint(TEXT("127.0.0.2:7777"));
    TestFalse(TEXT("Changing address clears the bound credential"),
              GameInstance->HasUsableReconnectContext());
    TestTrue(TEXT("Changing address removes the secret from memory"),
             GameInstance->NetworkResumeCredential.IsEmpty());
    GameInstance->CloseOnlineFrontDoor();
    TestEqual(TEXT("Back closes the online front door"),
              GameInstance->GetOnlineState(),
              EEchoesOnlineFrontDoorState::Idle);

    const FEchoesSkirmishSetup Canonical =
        FEchoesSkirmishSetupModel::CanonicalOnlineSetup();
    TestTrue(TEXT("Online rules are exactly canonical"),
             FEchoesSkirmishSetupModel::IsCanonicalOnlineSetup(Canonical));
    TestTrue(TEXT("Online rules fix Glass Scar and the two launch factions"),
             Canonical.MapPreset == EEchoesSkirmishMapPreset::GlassScar &&
                 Canonical.LocalFaction ==
                     echoes::sim::Faction::MeridianCompact &&
                 Canonical.OpponentFaction ==
                     echoes::sim::Faction::KharuunAssemblies &&
                 Canonical.ResourceLevel ==
                     EEchoesSkirmishResourceLevel::Standard);
    FEchoesSkirmishSetup Modified = Canonical;
    Modified.MapPreset = EEchoesSkirmishMapPreset::CrownfallBasin;
    TestFalse(TEXT("Online map modification is rejected"),
              FEchoesSkirmishSetupModel::IsCanonicalOnlineSetup(Modified));
    Modified = Canonical;
    Modified.OpponentFaction = echoes::sim::Faction::HollowChoir;
    TestFalse(TEXT("Online faction modification is rejected"),
              FEchoesSkirmishSetupModel::IsCanonicalOnlineSetup(Modified));
    Modified = Canonical;
    Modified.ResourceLevel = EEchoesSkirmishResourceLevel::Abundant;
    TestFalse(TEXT("Online resource modification is rejected"),
              FEchoesSkirmishSetupModel::IsCanonicalOnlineSetup(Modified));

    const FVector2D Viewports[] = {
        FVector2D(1280.0f, 720.0f),
        FVector2D(1600.0f, 900.0f),
        FVector2D(1920.0f, 1080.0f)};
    for (const FVector2D& Viewport : Viewports)
    {
        const FEchoesOnlineFrontDoorLayout FrontDoor =
            FEchoesOnlineFrontDoorLayout::Build(Viewport, 1.0f);
        const FEchoesOnlineLocalMenuLayout LocalMenu =
            FEchoesOnlineLocalMenuLayout::Build(Viewport, 1.0f);
        const FBox2D TitleEntry =
            FEchoesOnlineFrontDoorLayout::BuildTitleEntry(Viewport, 1.0f);
        const auto InsideViewport = [&Viewport](const FBox2D& Box)
        {
            return Box.bIsValid && Box.Min.X >= 0.0f && Box.Min.Y >= 0.0f &&
                Box.Max.X <= Viewport.X && Box.Max.Y <= Viewport.Y;
        };
        TestTrue(
            FString::Printf(
                TEXT("Online pointer targets remain safe at %.0fx%.0f"),
                Viewport.X,
                Viewport.Y),
            InsideViewport(TitleEntry) &&
                InsideViewport(FrontDoor.HostButton) &&
                InsideViewport(FrontDoor.EndpointField) &&
                InsideViewport(FrontDoor.JoinButton) &&
                InsideViewport(FrontDoor.BackButton) &&
                InsideViewport(FrontDoor.RetryButton) &&
                !FrontDoor.HostButton.Intersect(FrontDoor.EndpointField) &&
                !FrontDoor.EndpointField.Intersect(FrontDoor.JoinButton) &&
                !FrontDoor.JoinButton.Intersect(FrontDoor.BackButton) &&
                !FrontDoor.RetryButton.Intersect(FrontDoor.BackButton));
        TestTrue(
            FString::Printf(
                TEXT("Online local menu actions remain distinct at %.0fx%.0f"),
                Viewport.X,
                Viewport.Y),
            InsideViewport(LocalMenu.ResumeButton) &&
                InsideViewport(LocalMenu.LeaveButton) &&
                !LocalMenu.ResumeButton.Intersect(LocalMenu.LeaveButton));
    }

    return true;
}

#endif
