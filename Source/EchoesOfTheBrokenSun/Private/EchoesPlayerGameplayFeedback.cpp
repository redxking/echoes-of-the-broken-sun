#include "EchoesPlayerController.h"

#include "EchoesGameMode.h"
#include "EchoesOfTheBrokenSun.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"


namespace
{
void TraceGameplayFeedbackEvent(const TCHAR* Direction, const FEchoesGameplayFeedbackPacket& Packet)
{
    UE_LOG(LogEchoes, Verbose,
        TEXT("[ECHOES_GAMEPLAY_FEEDBACK_EVENT_%s] player=%u generation=%llu event=%llu tick=%llu kind=%u sequence=%llu actor=%u target=%u"),
        Direction, Packet.Recipient,
        static_cast<unsigned long long>(Packet.Generation),
        static_cast<unsigned long long>(Packet.EventId),
        static_cast<unsigned long long>(Packet.Tick), Packet.Kind,
        static_cast<unsigned long long>(Packet.Sequence), Packet.Actor, Packet.Target);
}
}

const echoes::feedback::GameplayFeedbackState& AEchoesPlayerController::GetGameplayFeedback() const
{
    if (GetNetMode() == NM_Client) return NetworkGameplayFeedback;
    const auto* Bridge = GetWorld() ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>() : nullptr;
    static const echoes::feedback::GameplayFeedbackState Unavailable;
    return Bridge ? Bridge->GetGameplayFeedbackForPlayer() : Unavailable;
}

void AEchoesPlayerController::ResetNetworkGameplayFeedback()
{
    if (GetWorld()) GetWorldTimerManager().ClearTimer(GameplayFeedbackReseedTimer);
    NetworkGameplayFeedback.Reset();
    bGameplayFeedbackReseedRequested = false;
    bSentGameplayFeedbackInitialized = false;
    SentGameplayFeedbackLoss = {};
    SentGameplayFeedbackGeneration = 0;
    SentGameplayFeedbackEventId = 0;
    LastGameplayFeedbackReseedSeconds = -1.0;
    LastGameplayFeedbackRequestSeconds = -1.0;
}

void AEchoesPlayerController::PublishGameplayFeedback()
{
    if (!HasAuthority() || IsLocalController() || !bNetworkMatchStarted ||
        !bNetworkCompatibilityAccepted || NetworkSeat >= echoes::sim::kMaximumPlayers) return;
    const auto* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AEchoesGameMode>() : nullptr;
    const auto* Bridge = GetWorld() ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>() : nullptr;
    if (!GameMode || !GameMode->IsBoundNetworkController(this) || !Bridge ||
        !Bridge->IsScenarioReady()) return;
    const auto& Feed = Bridge->GetGameplayFeedbackForPlayer(NetworkSeat);
    const auto& History = Feed.History();
    const uint64 Generation = Feed.Generation();
    const uint64 Floor = History.empty() ? Feed.LastEventId() : History.front().eventId - 1;
    const bool bReset = !bSentGameplayFeedbackInitialized ||
        SentGameplayFeedbackGeneration != Generation ||
        SentGameplayFeedbackEventId < Floor;
    const uint64 After = bReset ? Floor : SentGameplayFeedbackEventId;
    TArray<FEchoesGameplayFeedbackPacket> Packets;
    for (const auto& Event : History)
    {
        if (Event.eventId > After) Packets.Add(FEchoesGameplayFeedbackPacket::FromEvent(Event));
    }
    if (!bReset && Packets.IsEmpty() && Feed.Loss() == SentGameplayFeedbackLoss) return;
    // The owning actor's reliable channel orders a reseed and its events.
    // Every payload comes from this seat's PlayerView/accepted commands.
    ClientReceiveGameplayFeedback(Generation, NetworkSeat, After,
        bReset, bReset && Floor != 0,
        FEchoesGameplayFeedbackLossPacket::FromLoss(Feed.Loss()), Packets);
    for (const auto& Packet : Packets) TraceGameplayFeedbackEvent(TEXT("SENT"), Packet);
    bSentGameplayFeedbackInitialized = true;
    SentGameplayFeedbackGeneration = Generation;
    SentGameplayFeedbackEventId = Feed.LastEventId();
    SentGameplayFeedbackLoss = Feed.Loss();
    UE_LOG(LogEchoes, Verbose,
        TEXT("[ECHOES_GAMEPLAY_FEEDBACK_SENT] player=%u generation=%llu after=%llu last=%llu events=%d reset=%s recipientOnly=true"),
        NetworkSeat, static_cast<unsigned long long>(Generation),
        static_cast<unsigned long long>(After), static_cast<unsigned long long>(SentGameplayFeedbackEventId),
        Packets.Num(), bReset ? TEXT("true") : TEXT("false"));
}

void AEchoesPlayerController::ClientReceiveGameplayFeedback_Implementation(
    uint64 Generation, uint8 Recipient, uint64 AfterEventId,
    bool bResetStream, bool bHistoryTruncated,
    const FEchoesGameplayFeedbackLossPacket& Loss,
    const TArray<FEchoesGameplayFeedbackPacket>& Events)
{
    using echoes::feedback::GameplayFeedbackStatus;
    if (GetNetMode() != NM_Client || !bNetworkCompatibilityAccepted ||
        !bNetworkMatchStarted || Generation == 0 || Recipient != NetworkSeat ||
        Recipient >= echoes::sim::kMaximumPlayers ||
        Events.Num() > static_cast<int32>(echoes::feedback::GameplayFeedbackState::HistoryLimit)) return;
    if (bGameplayFeedbackReseedRequested && !bResetStream)
    {
        RequestGameplayFeedbackReseed();
        return;
    }
    // Validate the whole delivery before publishing a partial history.
    auto Staged = NetworkGameplayFeedback;
    if (bResetStream)
    {
        if (Staged.BeginRemoteStream(Generation, Recipient, AfterEventId, bHistoryTruncated) !=
            GameplayFeedbackStatus::Accepted) return;
    }
    else if (Generation != Staged.Generation() || AfterEventId != Staged.LastEventId())
    {
        RequestGameplayFeedbackReseed();
        return;
    }
    for (const auto& Packet : Events)
    {
        if (Staged.IngestRemote(Packet.ToEvent(), Recipient, Generation) != GameplayFeedbackStatus::Accepted)
        {
            RequestGameplayFeedbackReseed();
            return;
        }
    }
    const auto LossStatus = Staged.ReportRemoteObservationLoss(Loss.ToLoss(), Recipient, Generation);
    if (LossStatus != GameplayFeedbackStatus::Accepted &&
        LossStatus != GameplayFeedbackStatus::NoChange) return;
    NetworkGameplayFeedback = MoveTemp(Staged);
    for (const auto& Packet : Events) TraceGameplayFeedbackEvent(TEXT("RECEIVED"), Packet);
    UE_LOG(LogEchoes, Verbose,
        TEXT("[ECHOES_GAMEPLAY_FEEDBACK_RECEIVED] player=%u generation=%llu after=%llu last=%llu events=%d reset=%s exactNext=true"),
        Recipient, static_cast<unsigned long long>(Generation),
        static_cast<unsigned long long>(AfterEventId),
        static_cast<unsigned long long>(NetworkGameplayFeedback.LastEventId()),
        Events.Num(), bResetStream ? TEXT("true") : TEXT("false"));
    bGameplayFeedbackReseedRequested = false;
    GetWorldTimerManager().ClearTimer(GameplayFeedbackReseedTimer);
}

void AEchoesPlayerController::RequestGameplayFeedbackReseed()
{
    if (GetNetMode() != NM_Client || !bNetworkCompatibilityAccepted ||
        !bNetworkMatchStarted || !GetWorld()) return;
    bGameplayFeedbackReseedRequested = true;
    const double Now = FPlatformTime::Seconds();
    const double Remaining = LastGameplayFeedbackRequestSeconds < 0.0 ? 0.0 :
        FMath::Max(0.0, 1.0 - (Now - LastGameplayFeedbackRequestSeconds));
    if (Remaining <= 0.0)
    {
        LastGameplayFeedbackRequestSeconds = Now;
        ServerRequestGameplayFeedbackReseed();
    }
    if (!bGameplayFeedbackReseedRequested) return;
    // Retry even if no further gameplay events arrive. A rate-limited server
    // response or a quiet stream must not strand a pending reseed forever.
    GetWorldTimerManager().SetTimer(GameplayFeedbackReseedTimer, this,
        &AEchoesPlayerController::RequestGameplayFeedbackReseed,
        static_cast<float>(Remaining > 0.0 ? Remaining + 0.05 : 1.05), false);
}

void AEchoesPlayerController::ServerRequestGameplayFeedbackReseed_Implementation()
{
    const auto* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AEchoesGameMode>() : nullptr;
    if (!HasAuthority() || !bNetworkCompatibilityAccepted || !bNetworkMatchStarted ||
        !GameMode || !GameMode->IsBoundNetworkController(this)) return;
    const double Now = FPlatformTime::Seconds();
    if (LastGameplayFeedbackReseedSeconds >= 0.0 &&
        Now - LastGameplayFeedbackReseedSeconds < 1.0) return;
    LastGameplayFeedbackReseedSeconds = Now;
    bSentGameplayFeedbackInitialized = false;
    PublishGameplayFeedback();
}
