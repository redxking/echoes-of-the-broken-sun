#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesGameInstance.h"
#include "EchoesOnlineFrontDoorLayout.h"
#include "EchoesSkirmishSetup.h"
#include "HAL/PlatformTime.h"

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
        {TEXT("192.168.50.12:7801"), TEXT("192.168.50.12:7801")},
        {TEXT("Example-LAN.local:07801"), TEXT("example-lan.local:7801")}};
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
        TEXT("-bad.local:7777"),
        TEXT("bad-.local:7777"),
        TEXT("http://127.0.0.1:7777"),
        TEXT("127.0.0.1:7777?listen"),
        TEXT("host name:7777"),
        TEXT("fe80::1:7777")};
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
    GameInstance->BoundReconnectEndpoint = TEXT("host.local:7777");
    GameInstance->DirectConnectEndpoint = TEXT("host.local:7777");
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
    GameInstance->SetDirectConnectEndpoint(TEXT("other.local:7777"));
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
