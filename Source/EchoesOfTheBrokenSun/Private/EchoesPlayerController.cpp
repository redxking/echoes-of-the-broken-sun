#include "EchoesPlayerController.h"

#include "EchoesCommandMarkerView.h"
#include "EchoesEntityView.h"
#include "EchoesFogView.h"
#include "EchoesGameUserSettings.h"
#include "EchoesHudLayout.h"
#include "EchoesNetworkSession.h"
#include "EchoesOfTheBrokenSun.h"
#include "EchoesPresentationAudioSubsystem.h"
#include "EchoesPointerCombatGuardReview.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesTechnologyPanelLayout.h"
#include "EchoesTerrainView.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/EngineTypes.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/SkyLight.h"
#include "Engine/World.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "InputCoreTypes.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "TimerManager.h"
#include "UnrealClient.h"

#include <algorithm>
#include <limits>

namespace
{
constexpr float DragSelectionThresholdPixels = 8.0f;
constexpr float FormationSpacingWorldUnits = 150.0f;
constexpr int32 ControlGroupCount = 10;
constexpr float NetworkTileWorldSize = 200.0f;

[[nodiscard]] FString FactionDisplayName(echoes::sim::Faction Faction)
{
    return Faction == echoes::sim::Faction::KharuunAssemblies
               ? TEXT("KHARUUN ASSEMBLIES")
               : TEXT("MERIDIAN COMPACT");
}
}

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
    SetInputMode(InputMode);
    bShowMouseCursor = true;
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
    if (GetNetMode() == NM_Client)
    {
        GetWorldTimerManager().SetTimerForNextTick(
            this,
            &AEchoesPlayerController::SubmitNetworkCompatibilityHello);
    }
}

void AEchoesPlayerController::EndPlay(
    const EEndPlayReason::Type EndPlayReason)
{
    DestroyNetworkPresentation();
    Super::EndPlay(EndPlayReason);
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
        FPlatformMisc::RequestExit(false);
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

void AEchoesPlayerController::ServerSubmitCompatibilityHello_Implementation(
    const TArray<uint8>& Packet)
{
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    const echoes::sim::Simulation* Simulation =
        Bridge != nullptr ? Bridge->GetSimulation() : nullptr;
    if (NetworkSeat >= echoes::sim::kMaximumPlayers || Bridge == nullptr ||
        Simulation == nullptr || !Bridge->IsScenarioReady())
    {
        ClientReceiveCompatibilityResult(
            false, TEXT("NET_AUTHORITY_NOT_READY"));
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
        ClientReceiveCompatibilityResult(false, Reason);
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
        ClientReceiveCompatibilityResult(false, Reason);
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
        ClientReceiveCompatibilityResult(
            false, TEXT("NET_COMMAND_SEQUENCE_UNAVAILABLE"));
        return;
    }
    NetworkCommandContext.hasAcceptedSequence = *NextSequence > 1;
    NetworkCommandContext.lastAcceptedSequence = *NextSequence - 1;

    bNetworkCompatibilityAccepted = true;
    ClientReceiveCompatibilityResult(true, TEXT("NET_COMPATIBLE"));
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
    if (bAccepted)
    {
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_NETWORK_COMPATIBILITY_RESULT] accepted=true reason=%s"),
            *StableReason);
        if (bNetworkClientSmoke ||
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
    }
    if (!bAccepted && bNetworkClientSmoke)
    {
        FPlatformMisc::RequestExit(false);
    }
}

void AEchoesPlayerController::ServerSetNetworkReady_Implementation()
{
    if (!bNetworkCompatibilityAccepted || bNetworkReady ||
        NetworkSeat >= echoes::sim::kMaximumPlayers)
    {
        return;
    }
    bNetworkReady = true;
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_NETWORK_LOBBY] player=%u compatible=true ready=true started=false"),
        NetworkSeat);
    BeginNetworkMatch();
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
    if (DeltaBytes.empty() || DeltaBytes.size() >= KeyframeBytes.size())
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
            DeltaError.empty() ? TEXT("deltaNotSmaller")
                               : UTF8_TO_TCHAR(DeltaError.c_str()));
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
    for (const echoes::sim::net::ScopedEntityState& Scoped :
         Keyframe.entities)
    {
        LiveEntityIds.Add(Scoped.id);
        AEchoesEntityView* View = nullptr;
        if (TWeakObjectPtr<AEchoesEntityView>* Existing =
                NetworkEntityViews.Find(Scoped.id))
        {
            View = Existing->Get();
        }
        const bool bNewView = View == nullptr;
        if (bNewView)
        {
            View = World->SpawnActor<AEchoesEntityView>(
                AEchoesEntityView::StaticClass(),
                FTransform::Identity,
                SpawnParameters);
            if (View == nullptr)
            {
                return false;
            }
            View->Tags.Add(TEXT("EchoesNetworkEntityView"));
            NetworkEntityViews.Add(Scoped.id, View);
        }
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
        View->ApplyAuthoritativeState(State, bNewView);
    }
    TArray<uint32> RemovedEntityIds;
    for (const TPair<uint32, TWeakObjectPtr<AEchoesEntityView>>& Pair :
         NetworkEntityViews)
    {
        if (!LiveEntityIds.Contains(Pair.Key))
        {
            if (AEchoesEntityView* View = Pair.Value.Get())
            {
                View->Destroy();
            }
            RemovedEntityIds.Add(Pair.Key);
        }
    }
    for (const uint32 Removed : RemovedEntityIds)
    {
        NetworkEntityViews.Remove(Removed);
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
        TEXT("[ECHOES_NETWORK_PRESENTATION_SYNCED] snapshot=%llu tick=%llu entities=%d tiles=%d removed=%d ground=true terrain=true fog=true lighting=true scopedOnly=true rendered=true"),
        static_cast<unsigned long long>(Keyframe.snapshotId),
        static_cast<unsigned long long>(Keyframe.simulationTick),
        NetworkEntityViews.Num(),
        static_cast<int32>(Keyframe.tiles.size()),
        RemovedEntityIds.Num());
    return true;
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
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NETWORK_KEYFRAME_ACK_REJECTED] player=%u snapshot=%llu digest=%llu lastAck=%llu reason=NET_SNAPSHOT_LINEAGE_INVALID"),
            NetworkSeat,
            static_cast<unsigned long long>(SnapshotId),
            static_cast<unsigned long long>(ScopedDigest),
            static_cast<unsigned long long>(LastAcknowledgedNetworkSnapshotId));
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
    if (!bNetworkClientSmoke || bNetworkSmokeCompletionSent ||
        !bNetworkRemoteExecutionReceived ||
        NetworkViewState.AcceptedCount() < 2)
    {
        return;
    }
    bNetworkSmokeCompletionSent = true;
    ServerConfirmNetworkSmokeComplete(LastNetworkSnapshotId);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_NETWORK_CLIENT_SMOKE_PASSED] snapshot=%llu acceptedKeyframes=%llu separateProcess=true readyGate=true periodicState=true authorityAssignedCommands=true connectionBound=true"),
        static_cast<unsigned long long>(LastNetworkSnapshotId),
        static_cast<unsigned long long>(NetworkViewState.AcceptedCount()));
    GetWorldTimerManager().SetTimer(
        NetworkClientExitTimer,
        this,
        &AEchoesPlayerController::FinishNetworkClientSmoke,
        0.1f,
        false);
}

void AEchoesPlayerController::ServerConfirmNetworkSmokeComplete_Implementation(
    uint64 SnapshotId)
{
    if (!bNetworkCommandExecutionVerified ||
        !bNetworkHostExecutionVerified ||
        NetworkSnapshotAcknowledgementCount < 2 ||
        SnapshotId != LastAcknowledgedNetworkSnapshotId)
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_NETWORK_SERVER_SMOKE_FAILED] snapshot=%llu expectedAck=%llu remoteExecutionVerified=%s hostExecutionVerified=%s acknowledgements=%llu"),
            static_cast<unsigned long long>(SnapshotId),
            static_cast<unsigned long long>(LastAcknowledgedNetworkSnapshotId),
            bNetworkCommandExecutionVerified ? TEXT("true") : TEXT("false"),
            bNetworkHostExecutionVerified ? TEXT("true") : TEXT("false"),
            static_cast<unsigned long long>(NetworkSnapshotAcknowledgementCount));
        return;
    }
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_NETWORK_SERVER_SMOKE_PASSED] snapshot=%llu player=%u acknowledgements=%llu separateProcess=true readyGate=true periodicState=true hostRemoteDelayParity=true authorityAssignedCommands=true connectionBound=true hiddenAuthorityExcluded=true"),
        static_cast<unsigned long long>(SnapshotId),
        NetworkSeat,
        static_cast<unsigned long long>(NetworkSnapshotAcknowledgementCount));
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
    const UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    return FactionDisplayName(
        Bridge != nullptr
            ? Bridge->GetOpponentFaction()
            : echoes::sim::Faction::KharuunAssemblies);
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
    bCampaignResult = false;
    bCampaignSuccess = false;
    CampaignConsequence = echoes::sim::FutureWellChoice::Dormant;
    RecordedCampaignConsequence = echoes::sim::FutureWellChoice::Dormant;
    CampaignCommitStatus = EEchoesCampaignCommitStatus::NotApplicable;
    PresentedMatchOutcome = echoes::sim::MatchOutcome::Ongoing;
    PresentedCampaignOperation = EEchoesOperationMode::Skirmish;
    Bridge->SetScenarioPaused(true);
    SetIgnoreMoveInput(true);
    SetIgnoreLookInput(true);
    SetStatusMessage(
        FString::Printf(
            TEXT("ECHOES OF THE BROKEN SUN — F9 changes operation; %sEnter opens the brief."),
            Bridge->GetOperationMode() == EEchoesOperationMode::Skirmish
                ? TEXT("Tab changes faction; ")
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
                : TEXT("Oruun and two memory witnesses deployed; ")),
        3600.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_TITLE_READY] operation=%s operationChoice=true keyboardStart=true factionChoice=%s"),
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
            : TEXT("GlassScar"),
        Bridge->GetOperationMode() == EEchoesOperationMode::Skirmish
            ? TEXT("true")
            : TEXT("false"));
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
    ClearSelection();
    bSelectionButtonDown = false;
    bControlGroupAssignmentArmed = false;
    bTitleScreenVisible = false;
    bPauseMenuVisible = false;
    bTechnologyPanelVisible = false;
    bMatchResultVisible = false;
    bCampaignResult = false;
    RecordedCampaignConsequence = echoes::sim::FutureWellChoice::Dormant;
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
            : TEXT("GLASS SCAR OPERATIONS BRIEF — Tab changes faction; Enter deploys."),
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
        : TEXT("GlassScar"),
        (bPrologue || bSevenAccounts || bCityReserve || bUnburiedRoad ||
         bTermsOfContinuance || bNamesWithoutBirths || bShapeOfSilence)
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
    bMissionBriefingVisible = false;
    Bridge->SetScenarioPaused(false);
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
        SetStatusMessage(
            FString::Printf(
                TEXT("DEPLOYED — sync Meridian proxies %d,%d + %d,%d by T%llu; hold to T%llu; extract witnesses at %d,%d."),
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
    else
    {
        SetStatusMessage(
            FString::Printf(
                  TEXT("DEPLOYED — secure the Future Well or destroy the %s Command Core."),
                  *GetOpponentFactionLabel()),
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
    const echoes::sim::Faction NewFaction =
        Bridge->GetLocalFaction() == echoes::sim::Faction::MeridianCompact
            ? echoes::sim::Faction::KharuunAssemblies
            : echoes::sim::Faction::MeridianCompact;
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
    FString Feedback;
    if (!Bridge->SelectOperationMode(NewOperation, Feedback))
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
            TEXT("%s Press Enter when ready."),
            *Feedback),
        3600.0f);
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

void AEchoesPlayerController::RequestNewCampaign()
{
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
    if (GetNetMode() == NM_Client && bNetworkCompatibilityAccepted &&
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
    else if (bMatchResultVisible)
    {
        RestartScenario();
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
    bTechnologyPanelVisible = false;
    bMatchResultVisible = true;
    bCampaignResult = false;
    PresentedCampaignOperation = EEchoesOperationMode::Skirmish;
    PresentedMatchOutcome = Outcome;
    SetIgnoreMoveInput(true);
    SetIgnoreLookInput(true);
    FString Message =
        TEXT("DRAW — both Command Cores fell in the same deterministic tick. Press R to restart.");
    if (Outcome == echoes::sim::MatchOutcome::Player0Victory)
    {
        Message =
            TEXT("VICTORY — the opposing Command Core has fallen. Press R to restart.");
    }
    else if (Outcome == echoes::sim::MatchOutcome::Player1Victory ||
             Outcome == echoes::sim::MatchOutcome::Player2Victory ||
             Outcome == echoes::sim::MatchOutcome::Player3Victory)
    {
        Message =
            TEXT("DEFEAT — your Command Core has fallen. Press R to restart.");
    }
    SetStatusMessage(Message, 3600.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_RESULT_PRESENTED] outcome=%u keyboardRestart=true"),
        static_cast<uint8>(Outcome));
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
        float MouseX = 0.0f;
        float MouseY = 0.0f;
        if (GetMousePosition(MouseX, MouseY))
        {
            SelectionCurrentScreenPosition = FVector2D(MouseX, MouseY);
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
    BindPressed(TEXT("CyclePlayableFaction"), &AEchoesPlayerController::CyclePlayableFaction);
    BindPressed(TEXT("CycleOperation"), &AEchoesPlayerController::CycleOperation);
    BindPressed(TEXT("RequestNewCampaign"), &AEchoesPlayerController::RequestNewCampaign);
    BindPressed(TEXT("RequestCampaignRestore"), &AEchoesPlayerController::RequestCampaignRestore);
    BindPressed(TEXT("CycleOwnedEntityPrevious"), &AEchoesPlayerController::CycleOwnedEntityPrevious);
    BindPressed(TEXT("SelectCombatForce"), &AEchoesPlayerController::SelectCombatForce);
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
    if (bTechnologyPanelVisible)
    {
        float MouseX = 0.0f;
        float MouseY = 0.0f;
        if (GetMousePosition(MouseX, MouseY))
        {
            (void)HandleTechnologyPanelPointer(FVector2D(MouseX, MouseY));
        }
        return;
    }
    if (IsModalOverlayVisible())
    {
        return;
    }
    float MouseX = 0.0f;
    float MouseY = 0.0f;
    if (!GetMousePosition(MouseX, MouseY))
    {
        SetStatusMessage(TEXT("[CURSOR_UNAVAILABLE] Selection could not read the pointer position."));
        return;
    }
    SelectionStartScreenPosition = FVector2D(MouseX, MouseY);
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

    float MouseX = SelectionCurrentScreenPosition.X;
    float MouseY = SelectionCurrentScreenPosition.Y;
    if (GetMousePosition(MouseX, MouseY))
    {
        SelectionCurrentScreenPosition = FVector2D(MouseX, MouseY);
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

    if (View == nullptr ||
        View->GetOwnerPlayerId() != UEchoesSimulationSubsystem::LocalPlayerId)
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
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Orders cannot be issued."));
        return;
    }
    if (Bridge->GetMatchOutcome() != echoes::sim::MatchOutcome::Ongoing)
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
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Orders cannot be issued."));
        return;
    }
    if (Bridge->GetMatchOutcome() != echoes::sim::MatchOutcome::Ongoing)
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
    if (Bridge == nullptr)
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Orders cannot be issued."));
        return;
    }

    const AEchoesEntityView* TargetView =
        Cast<AEchoesEntityView>(HitResult.GetActor());
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
        const echoes::sim::Entity* Entity =
            Bridge != nullptr ? Bridge->FindEntity(EntityId) : nullptr;
        if (Entity != nullptr &&
            Entity->owner == UEchoesSimulationSubsystem::LocalPlayerId)
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
        const echoes::sim::Entity* Entity =
            Bridge != nullptr ? Bridge->FindEntity(EntityId) : nullptr;
        if (Entity != nullptr &&
            Entity->owner == UEchoesSimulationSubsystem::LocalPlayerId)
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
        const echoes::sim::Entity* Entity =
            Bridge != nullptr ? Bridge->FindEntity(SelectedEntityIds[Index]) : nullptr;
        if (Entity == nullptr ||
            Entity->owner != UEchoesSimulationSubsystem::LocalPlayerId)
        {
            SelectedEntityIds.RemoveAtSwap(Index, 1, EAllowShrinking::No);
        }
    }
}

bool AEchoesPlayerController::TraceCursor(FHitResult& OutHitResult)
{
    float MouseX = 0.0f;
    float MouseY = 0.0f;
    if (!GetMousePosition(MouseX, MouseY))
    {
        return false;
    }
    LastPointerScreenPosition = FVector2D(MouseX, MouseY);
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
    if (Bridge != nullptr)
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
    const echoes::sim::ResearchType Candidates[] = {
        Player->faction == echoes::sim::Faction::MeridianCompact
            ? echoes::sim::ResearchType::MeridianPrismaticTargeting
            : echoes::sim::ResearchType::KharuunEchoCartography,
        Player->faction == echoes::sim::Faction::MeridianCompact
            ? echoes::sim::ResearchType::MeridianHorizonLattice
            : echoes::sim::ResearchType::KharuunAncestralEdge,
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
    const bool bMeridian =
        Player->faction == echoes::sim::Faction::MeridianCompact;
    const echoes::sim::ResearchType Research =
        TierIndex == 0
            ? (bMeridian
                   ? echoes::sim::ResearchType::MeridianPrismaticTargeting
                   : echoes::sim::ResearchType::KharuunEchoCartography)
            : (bMeridian
                   ? echoes::sim::ResearchType::MeridianHorizonLattice
                   : echoes::sim::ResearchType::KharuunAncestralEdge);
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
    const TCHAR* Label =
        Research == echoes::sim::ResearchType::MeridianPrismaticTargeting
            ? TEXT("PRISMATIC TARGETING")
            : Research == echoes::sim::ResearchType::MeridianHorizonLattice
                  ? TEXT("HORIZON LATTICE")
                  : Research == echoes::sim::ResearchType::KharuunEchoCartography
                        ? TEXT("ECHO CARTOGRAPHY")
                        : TEXT("ANCESTRAL EDGE");
    SetStatusMessage(FString::Printf(TEXT("%s: research queued."), Label));
}

void AEchoesPlayerController::AttackMoveAtCursor()
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    PruneSelection();
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

void AEchoesPlayerController::HoldSelectedUnits()
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    PruneSelection();
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
    if (IsModalOverlayVisible())
    {
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
    if (IsModalOverlayVisible())
    {
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
            Player != nullptr &&
                    Player->faction == echoes::sim::Faction::KharuunAssemblies
                ? echoes::sim::ResearchType::KharuunEchoCartography
                : echoes::sim::ResearchType::MeridianPrismaticTargeting;
        TechnologyPanelFocusedTier =
            Player != nullptr && Player->HasCompletedResearch(FirstTechnology)
                ? 1
                : 0;
        bTechnologyPanelWasScenarioPaused = Bridge->IsScenarioPaused();
        bTechnologyPanelVisible = true;
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

void AEchoesPlayerController::TogglePauseMenu()
{
    if (bTechnologyPanelVisible)
    {
        ToggleTechnologyPanel();
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
    Bridge->SetScenarioPaused(bPauseMenuVisible);
    SetIgnoreMoveInput(bPauseMenuVisible);
    SetIgnoreLookInput(bPauseMenuVisible);
    SetStatusMessage(
        bPauseMenuVisible
            ? TEXT("FIELD MENU — Enter, Escape, or P resumes; R restarts.")
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
        bRuntimeStateKnown = true;
        bTitleScreenVisible = false;
        bPauseMenuVisible = false;
        bTechnologyPanelVisible = false;
        bMatchResultVisible = false;
        bCampaignResult = false;
        bCampaignSuccess = false;
        CampaignConsequence = echoes::sim::FutureWellChoice::Dormant;
        RecordedCampaignConsequence = echoes::sim::FutureWellChoice::Dormant;
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
                : TEXT("MATCH RESTARTED — deterministic initial state restored."));
        UE_LOG(LogEchoes, Display, TEXT("[ECHOES_RESULT_RESTARTED] outcome=0"));
    }
    else
    {
        NotifyRuntimeFailure(TEXT("ECHOES_MATCH_RESTART_FAILED"));
    }
}

void AEchoesPlayerController::SetFutureWellChoice(
    echoes::sim::FutureWellChoice Choice)
{
    if (IsModalOverlayVisible())
    {
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
    StatusMessageExpiresAt =
        GetWorld() != nullptr ? GetWorld()->GetTimeSeconds() + DisplaySeconds : 0.0;
    UE_LOG(LogEchoes, Display, TEXT("[ECHOES_PLAYER_FEEDBACK] %s"), *Message);
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
    }
    return TEXT("ORDER");
}
