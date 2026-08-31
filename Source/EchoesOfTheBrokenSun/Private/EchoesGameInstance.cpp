#include "EchoesGameInstance.h"

#include "EchoesOfTheBrokenSun.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "TimerManager.h"

namespace
{
constexpr TCHAR EntryMap[] = TEXT("/Engine/Maps/Entry");
constexpr TCHAR ListenTravelUrl[] = TEXT("/Engine/Maps/Entry?listen");
constexpr int32 MaximumEndpointLength = 255;

bool IsBoundedResumeCredential(const FString& Credential)
{
    if (Credential.Len() != 32)
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

bool IsAsciiAlphaNumeric(TCHAR Character)
{
    return (Character >= TEXT('a') && Character <= TEXT('z')) ||
        (Character >= TEXT('A') && Character <= TEXT('Z')) ||
        (Character >= TEXT('0') && Character <= TEXT('9'));
}

bool TryCanonicalizeIpv4(
    const FString& Host,
    FString& OutCanonical,
    bool& OutLoopback)
{
    OutCanonical.Reset();
    OutLoopback = false;
    TArray<FString> Octets;
    Host.ParseIntoArray(Octets, TEXT("."), false);
    if (Octets.Num() != 4)
    {
        return false;
    }
    int32 Values[4] = {};
    for (int32 Index = 0; Index < Octets.Num(); ++Index)
    {
        const FString& Octet = Octets[Index];
        if (Octet.IsEmpty() || Octet.Len() > 3)
        {
            return false;
        }
        for (const TCHAR Character : Octet)
        {
            if (!FChar::IsDigit(Character))
            {
                return false;
            }
        }
        const int32 Value = FCString::Atoi(*Octet);
        if (Value < 0 || Value > 255)
        {
            return false;
        }
        Values[Index] = Value;
    }
    OutCanonical = FString::Printf(
        TEXT("%d.%d.%d.%d"),
        Values[0],
        Values[1],
        Values[2],
        Values[3]);
    OutLoopback = Values[0] == 127;
    return true;
}
}

void UEchoesGameInstance::Init()
{
    Super::Init();
    if (GEngine != nullptr)
    {
        GEngine->OnNetworkFailure().AddUObject(
            this, &UEchoesGameInstance::HandleNetworkFailure);
        GEngine->OnTravelFailure().AddUObject(
            this, &UEchoesGameInstance::HandleTravelFailure);
    }
}

void UEchoesGameInstance::Shutdown()
{
    if (GEngine != nullptr)
    {
        GEngine->OnNetworkFailure().RemoveAll(this);
        GEngine->OnTravelFailure().RemoveAll(this);
    }
    Super::Shutdown();
}

void UEchoesGameInstance::OpenOnlineFrontDoor()
{
    if (OnlineState != EEchoesOnlineFrontDoorState::Idle &&
        OnlineState != EEchoesOnlineFrontDoorState::Failed)
    {
        return;
    }
    OnlineState = EEchoesOnlineFrontDoorState::JoinSetup;
    OnlineFailureMessage.Reset();
    OnlineFocusIndex = 0;
    bIntentionalReturnPending = false;
    bCompletedOnlineResult = false;
}

void UEchoesGameInstance::CloseOnlineFrontDoor()
{
    ClearReconnectContext(TEXT("ONLINE_MENU_CLOSED"));
    OnlineState = EEchoesOnlineFrontDoorState::Idle;
    OnlineFailureMessage.Reset();
    OnlineFocusIndex = 0;
    bPlayerInitiatedOnlineSession = false;
}

void UEchoesGameInstance::RetryOnlineFrontDoor(
    APlayerController* Controller)
{
    if (OnlineState != EEchoesOnlineFrontDoorState::Failed)
    {
        return;
    }
    if (HasUsableReconnectContext() && RequestReconnect(Controller))
    {
        return;
    }
    OnlineState = EEchoesOnlineFrontDoorState::JoinSetup;
    OnlineFailureMessage.Reset();
    OnlineFocusIndex = 1;
}

void UEchoesGameInstance::FocusPreviousOnlineAction()
{
    if (OnlineState == EEchoesOnlineFrontDoorState::JoinSetup)
    {
        OnlineFocusIndex =
            (OnlineFocusIndex + OnlineActionCount - 1) % OnlineActionCount;
    }
}

void UEchoesGameInstance::FocusNextOnlineAction()
{
    if (OnlineState == EEchoesOnlineFrontDoorState::JoinSetup)
    {
        OnlineFocusIndex = (OnlineFocusIndex + 1) % OnlineActionCount;
    }
}

void UEchoesGameInstance::FocusOnlineAction(int32 FocusIndex)
{
    if (OnlineState == EEchoesOnlineFrontDoorState::JoinSetup)
    {
        OnlineFocusIndex = FMath::Clamp(
            FocusIndex, 0, OnlineActionCount - 1);
    }
}

bool UEchoesGameInstance::AppendEndpointCharacter(TCHAR Character)
{
    if (OnlineState != EEchoesOnlineFrontDoorState::JoinSetup ||
        OnlineFocusIndex != 1 ||
        DirectConnectEndpoint.Len() >= MaximumEndpointLength)
    {
        return false;
    }
    if (!IsAsciiAlphaNumeric(Character) && Character != TEXT('.') &&
        Character != TEXT(':') && Character != TEXT('-'))
    {
        return false;
    }
    if (!BoundReconnectEndpoint.IsEmpty())
    {
        ClearReconnectContext(TEXT("ONLINE_ADDRESS_CHANGED"));
    }
    DirectConnectEndpoint.AppendChar(FChar::ToLower(Character));
    return true;
}

bool UEchoesGameInstance::BackspaceEndpointCharacter()
{
    if (OnlineState != EEchoesOnlineFrontDoorState::JoinSetup ||
        OnlineFocusIndex != 1 || DirectConnectEndpoint.IsEmpty())
    {
        return false;
    }
    if (!BoundReconnectEndpoint.IsEmpty())
    {
        ClearReconnectContext(TEXT("ONLINE_ADDRESS_CHANGED"));
    }
    DirectConnectEndpoint.LeftChopInline(1, EAllowShrinking::No);
    return true;
}

void UEchoesGameInstance::SetDirectConnectEndpoint(const FString& Endpoint)
{
    if (OnlineState != EEchoesOnlineFrontDoorState::JoinSetup)
    {
        return;
    }
    const FString BoundedEndpoint = Endpoint.Left(MaximumEndpointLength);
    if (!BoundReconnectEndpoint.IsEmpty() &&
        !BoundReconnectEndpoint.Equals(
            BoundedEndpoint, ESearchCase::IgnoreCase))
    {
        ClearReconnectContext(TEXT("ONLINE_ADDRESS_CHANGED"));
    }
    DirectConnectEndpoint = BoundedEndpoint;
}

bool UEchoesGameInstance::RequestFixedRulesHost(UWorld* World)
{
    if (OnlineState != EEchoesOnlineFrontDoorState::JoinSetup ||
        World == nullptr || World->GetNetMode() != NM_Standalone)
    {
        ReportOnlineFailure(TEXT("ONLINE_HOST_START_UNAVAILABLE"));
        return false;
    }
#if !UE_BUILD_DEVELOPMENT
    ReportOnlineFailure(TEXT("ONLINE_DEVELOPMENT_ONLY"));
    return false;
#else
    if (!HasExplicitDevelopmentLoopbackBind(FCommandLine::Get()))
    {
        ReportOnlineFailure(TEXT("ONLINE_LOOPBACK_BIND_REQUIRED"));
        return false;
    }
#endif
    OnlineState = EEchoesOnlineFrontDoorState::Hosting;
    OnlineFailureMessage.Reset();
    OnlineFocusIndex = 0;
    bPlayerInitiatedOnlineSession = true;
    bIntentionalReturnPending = false;
    ResolveHostShareEndpoint();
    if (!World->ServerTravel(ListenTravelUrl, true))
    {
        ReportOnlineFailure(TEXT("ONLINE_HOST_TRAVEL_FAILED"));
        return false;
    }
    int32 ListenPort = 7777;
    int32 RequestedPort = 0;
    if (FParse::Value(FCommandLine::Get(), TEXT("port="), RequestedPort) &&
        RequestedPort >= 1 && RequestedPort <= 65535)
    {
        ListenPort = RequestedPort;
    }
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_ONLINE_FRONT_DOOR_HOST] requested=true map=/Engine/Maps/Entry listen=true rules=fixed_glass_scar port=%d security=development_loopback_only bind=127.0.0.1"),
        ListenPort);
    return true;
}

void UEchoesGameInstance::ResolveHostShareEndpoint()
{
    int32 Port = 7777;
    int32 RequestedPort = 0;
    if (FParse::Value(FCommandLine::Get(), TEXT("port="), RequestedPort) &&
        RequestedPort >= 1 && RequestedPort <= 65535)
    {
        Port = RequestedPort;
    }
    HostShareEndpoint = FString::Printf(TEXT("127.0.0.1:%d"), Port);
}

bool UEchoesGameInstance::RequestDirectJoin(APlayerController* Controller)
{
    if (OnlineState != EEchoesOnlineFrontDoorState::JoinSetup ||
        Controller == nullptr || !Controller->IsLocalController() ||
        Controller->GetWorld() == nullptr ||
        Controller->GetWorld()->GetNetMode() != NM_Standalone)
    {
        ReportOnlineFailure(TEXT("ONLINE_JOIN_START_UNAVAILABLE"));
        return false;
    }
#if !UE_BUILD_DEVELOPMENT
    ReportOnlineFailure(TEXT("ONLINE_DEVELOPMENT_ONLY"));
    return false;
#endif
    FString Normalized;
    FString ValidationError;
    if (!NormalizeDirectEndpoint(
            DirectConnectEndpoint, Normalized, ValidationError))
    {
        ReportOnlineFailure(ValidationError);
        return false;
    }
    if (!BoundReconnectEndpoint.IsEmpty() &&
        !BoundReconnectEndpoint.Equals(Normalized, ESearchCase::IgnoreCase))
    {
        ClearReconnectContext(TEXT("ONLINE_ADDRESS_CHANGED"));
    }
    DirectConnectEndpoint = Normalized;
    BoundReconnectEndpoint = Normalized;
    bReconnectAttemptPending = HasUsableReconnectContext();
    OnlineState = EEchoesOnlineFrontDoorState::Connecting;
    OnlineFailureMessage.Reset();
    bPlayerInitiatedOnlineSession = true;
    bIntentionalReturnPending = false;
    Controller->ClientTravel(DirectConnectEndpoint, TRAVEL_Absolute);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_ONLINE_FRONT_DOOR_JOIN] requested=true endpoint=%s options=false fixedRules=true"),
        *DirectConnectEndpoint);
    return true;
}

bool UEchoesGameInstance::RequestReconnect(APlayerController* Controller)
{
    if ((OnlineState != EEchoesOnlineFrontDoorState::Failed &&
         OnlineState != EEchoesOnlineFrontDoorState::JoinSetup) ||
        Controller == nullptr || !Controller->IsLocalController() ||
        Controller->GetWorld() == nullptr ||
        Controller->GetWorld()->GetNetMode() != NM_Standalone ||
        !HasUsableReconnectContext())
    {
        return false;
    }
#if !UE_BUILD_DEVELOPMENT
    ReportOnlineFailure(TEXT("ONLINE_DEVELOPMENT_ONLY"));
    return false;
#endif
    DirectConnectEndpoint = BoundReconnectEndpoint;
    OnlineState = EEchoesOnlineFrontDoorState::Connecting;
    OnlineFailureMessage.Reset();
    OnlineFocusIndex = 0;
    bPlayerInitiatedOnlineSession = true;
    bIntentionalReturnPending = false;
    bReconnectAttemptPending = true;
    Controller->ClientTravel(BoundReconnectEndpoint, TRAVEL_Absolute);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_ONLINE_RECONNECT_REQUESTED] endpoint=%s credentialLogged=false remainingSeconds=%d"),
        *BoundReconnectEndpoint,
        GetReconnectSecondsRemaining());
    return true;
}

void UEchoesGameInstance::CancelOnlineRequest(APlayerController* Controller)
{
    const EEchoesOnlineFrontDoorState PreviousState = OnlineState;
    if (PreviousState == EEchoesOnlineFrontDoorState::Idle)
    {
        return;
    }
    UWorld* World = Controller != nullptr ? Controller->GetWorld() : GetWorld();
    const bool bReturningFromTravel =
        PreviousState == EEchoesOnlineFrontDoorState::Hosting ||
        PreviousState == EEchoesOnlineFrontDoorState::Connecting ||
        PreviousState == EEchoesOnlineFrontDoorState::ClientLobby ||
        (World != nullptr && World->GetNetMode() != NM_Standalone);
    ClearReconnectContext(TEXT("ONLINE_INTENTIONAL_CANCEL"));
    OnlineState = bReturningFromTravel
        ? EEchoesOnlineFrontDoorState::JoinSetup
        : EEchoesOnlineFrontDoorState::Idle;
    OnlineFailureMessage.Reset();
    OnlineFocusIndex = 0;
    bPlayerInitiatedOnlineSession = false;
    bIntentionalReturnPending = bReturningFromTravel;
    if (bIntentionalReturnPending && World != nullptr)
    {
        QueueEntryTravelRetainingState();
    }
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_ONLINE_FRONT_DOOR_CANCELLED] previousState=%u returnToOperations=%s"),
        static_cast<uint8>(PreviousState),
        bIntentionalReturnPending ? TEXT("true") : TEXT("false"));
}

void UEchoesGameInstance::ReturnToOnlineFrontDoor(
    APlayerController* Controller)
{
    ClearReconnectContext(TEXT("ONLINE_INTENTIONAL_LEAVE"));
    bCompletedOnlineResult = false;
    OnlineState = EEchoesOnlineFrontDoorState::JoinSetup;
    OnlineFailureMessage.Reset();
    OnlineFocusIndex = 0;
    bPlayerInitiatedOnlineSession = false;
    bIntentionalReturnPending = true;
    UWorld* World = Controller != nullptr ? Controller->GetWorld() : GetWorld();
    if (World == nullptr)
    {
        bIntentionalReturnPending = false;
        return;
    }
    QueueEntryTravelRetainingState();
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_ONLINE_RETURN_TO_MENU] requested=true retainedEndpoint=true"));
}

void UEchoesGameInstance::ReturnToFailedFrontDoor(
    APlayerController* Controller)
{
    if (OnlineState != EEchoesOnlineFrontDoorState::Failed)
    {
        return;
    }
    UWorld* World = Controller != nullptr ? Controller->GetWorld() : GetWorld();
    if (World == nullptr)
    {
        return;
    }
    if (IsStandaloneEntryWorld(World))
    {
        bIntentionalReturnPending = false;
        bEntryTravelQueued = false;
        return;
    }
    bIntentionalReturnPending = true;
    QueueEntryTravelRetainingState();
}

void UEchoesGameInstance::NotifyControllerReady(APlayerController* Controller)
{
    if (Controller == nullptr || Controller->GetWorld() == nullptr)
    {
        return;
    }
    if (bIntentionalReturnPending &&
        IsStandaloneEntryWorld(Controller->GetWorld()))
    {
        bIntentionalReturnPending = false;
        bEntryTravelQueued = false;
    }
    if (bCompletedOnlineResult &&
        IsStandaloneEntryWorld(Controller->GetWorld()))
    {
        OnlineState = EEchoesOnlineFrontDoorState::JoinSetup;
        OnlineFailureMessage.Reset();
        OnlineFocusIndex = 0;
        bPlayerInitiatedOnlineSession = false;
        bCompletedOnlineResult = false;
    }
}

void UEchoesGameInstance::MarkClientLobby()
{
    if (OnlineState == EEchoesOnlineFrontDoorState::Connecting)
    {
        OnlineState = EEchoesOnlineFrontDoorState::ClientLobby;
        OnlineFailureMessage.Reset();
    }
}

void UEchoesGameInstance::MarkNetworkMatchStarted()
{
    if (bPlayerInitiatedOnlineSession)
    {
        OnlineState = EEchoesOnlineFrontDoorState::Idle;
        OnlineFailureMessage.Reset();
    }
}

void UEchoesGameInstance::MarkNetworkMatchResultReceived()
{
    ClearReconnectContext(TEXT("ONLINE_MATCH_COMPLETE"));
    bCompletedOnlineResult = true;
}

void UEchoesGameInstance::StoreNetworkResumeCredential(
    const FString& Credential,
    float GraceSeconds)
{
    if (!IsBoundedResumeCredential(Credential) || GraceSeconds <= 0.0f ||
        BoundReconnectEndpoint.IsEmpty())
    {
        ClearReconnectContext(TEXT("ONLINE_RESUME_CONTEXT_INVALID"));
        return;
    }
    NetworkResumeCredential = Credential;
    NetworkResumeGraceSeconds = GraceSeconds;
    NetworkResumeExpiresAtSeconds = 0.0;
    bReconnectWindowArmed = false;
    bReconnectAttemptPending = false;
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_ONLINE_RECONNECT_CONTEXT] stored=true endpoint=%s graceSeconds=%.0f armed=false credentialLogged=false persistence=memory_only"),
        *BoundReconnectEndpoint,
        GraceSeconds);
}

bool UEchoesGameInstance::HasUsableReconnectContext() const
{
    return HasStoredReconnectCredential() && bReconnectWindowArmed &&
        FPlatformTime::Seconds() < NetworkResumeExpiresAtSeconds;
}

bool UEchoesGameInstance::HasStoredReconnectCredential() const
{
    return !BoundReconnectEndpoint.IsEmpty() &&
        IsBoundedResumeCredential(NetworkResumeCredential) &&
        NetworkResumeGraceSeconds > 0.0f;
}

bool UEchoesGameInstance::ArmReconnectWindow()
{
    if (bReconnectWindowArmed)
    {
        return HasUsableReconnectContext();
    }
    if (!HasStoredReconnectCredential())
    {
        return false;
    }
    NetworkResumeExpiresAtSeconds =
        FPlatformTime::Seconds() +
        static_cast<double>(NetworkResumeGraceSeconds);
    bReconnectWindowArmed = true;
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_ONLINE_RECONNECT_CONTEXT] stored=true armed=true endpoint=%s graceSeconds=%.0f credentialLogged=false trigger=networkFailure"),
        *BoundReconnectEndpoint,
        NetworkResumeGraceSeconds);
    return true;
}

int32 UEchoesGameInstance::GetReconnectSecondsRemaining() const
{
    if (!HasUsableReconnectContext())
    {
        return 0;
    }
    return FMath::Max(
        0,
        FMath::CeilToInt(
            NetworkResumeExpiresAtSeconds - FPlatformTime::Seconds()));
}

bool UEchoesGameInstance::TryGetPendingReconnectCredential(
    FString& OutCredential) const
{
    OutCredential.Reset();
    if (!bReconnectAttemptPending || !HasUsableReconnectContext() ||
        !DirectConnectEndpoint.Equals(
            BoundReconnectEndpoint, ESearchCase::IgnoreCase))
    {
        return false;
    }
    OutCredential = NetworkResumeCredential;
    return true;
}

void UEchoesGameInstance::MarkReconnectAttemptAccepted()
{
    bReconnectAttemptPending = false;
}

void UEchoesGameInstance::ClearReconnectContext(const TCHAR* StableReason)
{
    const bool bHadContext = !NetworkResumeCredential.IsEmpty() ||
        !BoundReconnectEndpoint.IsEmpty();
    NetworkResumeCredential.Reset();
    BoundReconnectEndpoint.Reset();
    NetworkResumeExpiresAtSeconds = 0.0;
    NetworkResumeGraceSeconds = 0.0f;
    bReconnectAttemptPending = false;
    bReconnectWindowArmed = false;
    if (bHadContext)
    {
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_ONLINE_RECONNECT_CONTEXT] stored=false reason=%s credentialLogged=false"),
            StableReason != nullptr ? StableReason : TEXT("ONLINE_CONTEXT_CLEARED"));
    }
}

void UEchoesGameInstance::ReportOnlineFailure(
    const FString& StableReason,
    bool bPreserveReconnect)
{
    if (!bPreserveReconnect || !HasUsableReconnectContext())
    {
        ClearReconnectContext(TEXT("ONLINE_TERMINAL_FAILURE"));
    }
    OnlineState = EEchoesOnlineFrontDoorState::Failed;
    OnlineFailureMessage = PlayerFacingFailure(StableReason);
    OnlineFocusIndex = 0;
    bPlayerInitiatedOnlineSession = false;
    bIntentionalReturnPending = false;
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_ONLINE_FRONT_DOOR_FAILED] reason=%s retainedEndpoint=true"),
        *StableReason.Left(160));
}

bool UEchoesGameInstance::NormalizeDirectEndpoint(
    const FString& Candidate,
    FString& OutNormalized,
    FString& OutError)
{
    OutNormalized.Reset();
    OutError.Reset();
    if (Candidate.IsEmpty() || Candidate.Len() > MaximumEndpointLength)
    {
        OutError = TEXT("ONLINE_ADDRESS_LENGTH_INVALID");
        return false;
    }
    const FString Trimmed = Candidate.TrimStartAndEnd();
    if (Trimmed != Candidate || Candidate.Contains(TEXT("://")) ||
        Candidate.Contains(TEXT("?")) || Candidate.Contains(TEXT("#")) ||
        Candidate.Contains(TEXT("/")) || Candidate.Contains(TEXT("\\")) ||
        Candidate.Contains(TEXT("@")))
    {
        OutError = TEXT("ONLINE_ADDRESS_FORMAT_INVALID");
        return false;
    }
    for (const TCHAR Character : Candidate)
    {
        if (FChar::IsWhitespace(Character) || FChar::IsControl(Character))
        {
            OutError = TEXT("ONLINE_ADDRESS_FORMAT_INVALID");
            return false;
        }
    }
    int32 ColonIndex = INDEX_NONE;
    if (!Candidate.FindLastChar(TEXT(':'), ColonIndex) ||
        ColonIndex <= 0 || ColonIndex >= Candidate.Len() - 1 ||
        Candidate.Left(ColonIndex).Contains(TEXT(":")))
    {
        OutError = TEXT("ONLINE_ADDRESS_PORT_REQUIRED");
        return false;
    }
    const FString Host = Candidate.Left(ColonIndex).ToLower();
    const FString PortText = Candidate.Mid(ColonIndex + 1);
    for (const TCHAR Character : PortText)
    {
        if (!FChar::IsDigit(Character))
        {
            OutError = TEXT("ONLINE_ADDRESS_PORT_INVALID");
            return false;
        }
    }
    const int64 Port = FCString::Atoi64(*PortText);
    if (Port < 1 || Port > 65535)
    {
        OutError = TEXT("ONLINE_ADDRESS_PORT_INVALID");
        return false;
    }
    FString CanonicalHost;
    if (Host == TEXT("localhost"))
    {
        CanonicalHost = TEXT("127.0.0.1");
    }
    else
    {
        bool bNumericHost = true;
        for (const TCHAR Character : Host)
        {
            bNumericHost &=
                FChar::IsDigit(Character) || Character == TEXT('.');
        }
        if (!bNumericHost)
        {
            OutError = TEXT("ONLINE_ADDRESS_LOOPBACK_REQUIRED");
            return false;
        }
        bool bLoopback = false;
        if (!TryCanonicalizeIpv4(Host, CanonicalHost, bLoopback))
        {
            OutError = TEXT("ONLINE_ADDRESS_HOST_INVALID");
            return false;
        }
        if (!bLoopback)
        {
            OutError = TEXT("ONLINE_ADDRESS_LOOPBACK_REQUIRED");
            return false;
        }
    }
    OutNormalized = FString::Printf(
        TEXT("%s:%lld"), *CanonicalHost, Port);
    FURL Parsed(nullptr, *OutNormalized, TRAVEL_Absolute);
    if (Parsed.Valid == 0 || Parsed.Host.IsEmpty() || Parsed.Port != Port ||
        Parsed.Op.Num() != 0)
    {
        OutNormalized.Reset();
        OutError = TEXT("ONLINE_ADDRESS_URL_INVALID");
        return false;
    }
    return true;
}

bool UEchoesGameInstance::HasExplicitDevelopmentLoopbackBind(
    const TCHAR* CommandLine)
{
#if !UE_BUILD_DEVELOPMENT
    (void)CommandLine;
    return false;
#else
    if (CommandLine == nullptr)
    {
        return false;
    }
    FString BindHost;
    if (!FParse::Value(CommandLine, TEXT("MULTIHOME="), BindHost) ||
        BindHost.IsEmpty())
    {
        return false;
    }
    FString Canonical;
    bool bLoopback = false;
    return TryCanonicalizeIpv4(BindHost, Canonical, bLoopback) &&
        bLoopback && BindHost == TEXT("127.0.0.1") &&
        Canonical == TEXT("127.0.0.1");
#endif
}

void UEchoesGameInstance::HandleNetworkFailure(
    UWorld* World,
    UNetDriver* NetDriver,
    ENetworkFailure::Type FailureType,
    const FString& ErrorString)
{
    (void)World;
    (void)NetDriver;
    (void)FailureType;
    if (bCompletedOnlineResult)
    {
        OnlineState = EEchoesOnlineFrontDoorState::JoinSetup;
        OnlineFailureMessage.Reset();
        bPlayerInitiatedOnlineSession = false;
        bIntentionalReturnPending = true;
        QueueEntryTravelRetainingState();
        return;
    }
    if (bIntentionalReturnPending || !bPlayerInitiatedOnlineSession)
    {
        return;
    }
    const bool bCanReconnect = ArmReconnectWindow();
    ReportOnlineFailure(ErrorString.IsEmpty()
        ? TEXT("ONLINE_NETWORK_FAILURE")
        : ErrorString,
        bCanReconnect);
    if (World != nullptr && !IsStandaloneEntryWorld(World))
    {
        ReturnToFailedFrontDoor(nullptr);
    }
}

void UEchoesGameInstance::HandleTravelFailure(
    UWorld* World,
    ETravelFailure::Type FailureType,
    const FString& ErrorString)
{
    (void)World;
    (void)FailureType;
    if (bIntentionalReturnPending || !bPlayerInitiatedOnlineSession)
    {
        return;
    }
    const bool bCanReconnect = ArmReconnectWindow();
    ReportOnlineFailure(ErrorString.IsEmpty()
        ? TEXT("ONLINE_TRAVEL_FAILURE")
        : ErrorString,
        bCanReconnect);
    if (World != nullptr && !IsStandaloneEntryWorld(World))
    {
        ReturnToFailedFrontDoor(nullptr);
    }
}

void UEchoesGameInstance::QueueEntryTravelRetainingState()
{
    UWorld* World = GetWorld();
    if (World == nullptr || bEntryTravelQueued)
    {
        return;
    }
    if (IsStandaloneEntryWorld(World))
    {
        bIntentionalReturnPending = false;
        return;
    }
    bEntryTravelQueued = true;
    World->GetTimerManager().SetTimerForNextTick(
        FTimerDelegate::CreateUObject(
            this,
            &UEchoesGameInstance::TravelToEntryRetainingState));
}

void UEchoesGameInstance::TravelToEntryRetainingState()
{
    bEntryTravelQueued = false;
    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return;
    }
    if (IsStandaloneEntryWorld(World))
    {
        bIntentionalReturnPending = false;
        return;
    }
    bIntentionalReturnPending = true;
    UGameplayStatics::OpenLevel(World, FName(EntryMap), true);
}

bool UEchoesGameInstance::IsStandaloneEntryWorld(const UWorld* World)
{
    return World != nullptr && World->GetNetMode() == NM_Standalone &&
        World->GetOutermost() != nullptr &&
        World->GetOutermost()->GetName().EndsWith(TEXT("/Entry"));
}

FString UEchoesGameInstance::PlayerFacingFailure(
    const FString& StableReason)
{
    if (StableReason.Contains(TEXT("SEAT_UNAVAILABLE")))
    {
        return TEXT("That match already has two players.");
    }
    if (StableReason.Contains(TEXT("COMPAT")) ||
        StableReason.Contains(TEXT("MISMATCH")) ||
        StableReason.Contains(TEXT("Outdated")))
    {
        return TEXT("The host uses a different game build or fixed ruleset.");
    }
    if (StableReason.Contains(TEXT("Timeout"), ESearchCase::IgnoreCase) ||
        StableReason.Contains(TEXT("timed out"), ESearchCase::IgnoreCase))
    {
        return TEXT("The host did not answer before the connection timed out.");
    }
    if (StableReason.Contains(TEXT("lost"), ESearchCase::IgnoreCase) ||
        StableReason.Contains(TEXT("closed"), ESearchCase::IgnoreCase))
    {
        return TEXT("The connection to the other player was lost.");
    }
    if (StableReason.Contains(TEXT("ADDRESS")))
    {
        return TEXT("Development multiplayer accepts only localhost followed by a port, such as 127.0.0.1:7777.");
    }
    if (StableReason.Contains(TEXT("LOOPBACK")) ||
        StableReason.Contains(TEXT("DEVELOPMENT_ONLY")))
    {
        return TEXT("Online play is currently limited to a Development build on this Mac.");
    }
    if (StableReason.Contains(TEXT("HOST")) ||
        StableReason.Contains(TEXT("TRAVEL")))
    {
        return TEXT("The online match could not be opened. Return to Operations and try again.");
    }
    return TEXT("The host could not be reached. Check the address and try again.");
}
