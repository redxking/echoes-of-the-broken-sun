// DeliveryPlan D2 exit chain, rendered review driver.
//
// Runs only when the process was started with -EchoesD2ExitReview. It walks
// the ordinary player route in a real rendered window and issues the same
// controller and bridge actions the player's bindings call: deploy a Glass
// Scar skirmish through the title, gather to a delivery, place and finish a
// structure, harvest the Future Well, train to the 30-entity limit and read
// its refusal, move and fight with visible health change, quick-save and
// quick-load, repair a damaged owned target, and end with a truthful result.
// Every stage writes a capture and a log marker.
//
// Evidence class: agent-driven in-process rendered review. It is not physical
// input, not packaged execution, and not owner acceptance. Author and owner:
// Angelis Pseftis.

#include "EchoesPlayerController.h"

#include "EchoesBuildPlacementPreview.h"
#include "EchoesOfTheBrokenSun.h"
#include "EchoesRTSCameraPawn.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesSkirmishSetup.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

namespace
{
using echoes::sim::Entity;
using echoes::sim::EntityId;
using echoes::sim::EntityType;
using echoes::sim::OrderType;
using echoes::sim::PlayerId;
using echoes::sim::Vec2;

constexpr PlayerId kReviewSeat = UEchoesSimulationSubsystem::LocalPlayerId;
constexpr float kReviewTotalBudgetSeconds = 1440.0f;

// The Well sits under fog at deployment, and the Well order needs the target
// visible, exactly as it does for the player. The worker therefore walks there
// first and the order is retried once the Well is in sight.
bool GD2WellOrderPending = false;
float GD2WellRetryElapsed = -10.0f;

// Captures follow a stage transition by a short delay so the HUD has redrawn
// the state the marker describes; a frame grabbed in the same tick still shows
// the previous status line.
constexpr float kCaptureDelaySeconds = 0.4f;
bool GD2CapturePending = false;
FString GD2CaptureName;

// A queued player command applies on the next simulation tick, and the
// controller ticks faster than the simulation. Between the two, a worker just
// ordered to build still reads as idle; the gather line must leave it alone
// or its Gather replaces the Build the moment both apply and the site is
// orphaned. Runs 10-12 of the D2 exit review produced exactly that: one idle
// worker was named builder four times and every site sat at 0/100 until a
// second worker assisted.
uint32 GD2PendingBuilderId = 0;
float GD2PendingBuilderElapsed = -10.0f;

// Stuck-unit detector for the training notes: an owned mobile unit that holds
// a movement order (Gather while moving to its deposit, Build, Move) and has
// not left a one-tile radius of where it stood twenty seconds ago is named
// with its order and target. Displacement, not exact position: a unit arguing
// with the route field on a tile boundary moves every tick and goes nowhere.
struct FD2Seen { int32 RawX = 0; int32 RawY = 0; float SinceElapsed = 0.0f; };
TMap<uint32, FD2Seen> GD2LastMoved;
float GD2StuckNoteElapsed = -100.0f;

// Diagnostic switch: -EchoesD2ExitReviewNaiveBuilder restores the builder
// choice of runs 12-13 (nearest gatherer to the Core, no mobility proof) so a
// stalled builder's trajectory can be read from the 5-second notes.
bool GD2NaiveBuilder = false;

enum class ED2Stage : int32
{
    OpenModes = 0,
    OpenBriefing,
    Deploy,
    GatherIssue,
    GatherDeliver,
    BuildIssue,
    BuildSite,
    BuildComplete,
    WellHarvest,
    TrainToLimit,
    Fight,
    SaveLoad,
    RepairStage,
    RepairWait,
    Outcome,
    Done
};

const TCHAR* StageName(int32 Stage)
{
    switch (static_cast<ED2Stage>(Stage))
    {
        case ED2Stage::OpenModes: return TEXT("open_modes");
        case ED2Stage::OpenBriefing: return TEXT("open_briefing");
        case ED2Stage::Deploy: return TEXT("deploy");
        case ED2Stage::GatherIssue: return TEXT("gather_issue");
        case ED2Stage::GatherDeliver: return TEXT("gather_deliver");
        case ED2Stage::BuildIssue: return TEXT("build_issue");
        case ED2Stage::BuildSite: return TEXT("build_site");
        case ED2Stage::BuildComplete: return TEXT("build_complete");
        case ED2Stage::WellHarvest: return TEXT("well_harvest");
        case ED2Stage::TrainToLimit: return TEXT("train_to_limit");
        case ED2Stage::Fight: return TEXT("fight");
        case ED2Stage::SaveLoad: return TEXT("save_load");
        case ED2Stage::RepairStage: return TEXT("repair_issue");
        case ED2Stage::RepairWait: return TEXT("repair_wait");
        case ED2Stage::Outcome: return TEXT("outcome");
        default: return TEXT("done");
    }
}

int64 DistanceSquaredRaw(const Vec2& A, const Vec2& B)
{
    const int64 DeltaX = static_cast<int64>(A.x.Raw()) - static_cast<int64>(B.x.Raw());
    const int64 DeltaY = static_cast<int64>(A.y.Raw()) - static_cast<int64>(B.y.Raw());
    return DeltaX * DeltaX + DeltaY * DeltaY;
}

bool IsMobileCombatType(EntityType Type)
{
    return Type == EntityType::Soldier || Type == EntityType::HeavyUnit ||
        Type == EntityType::ScoutUnit;
}

bool IsStructureType(EntityType Type)
{
    return Type == EntityType::CommandCore || Type == EntityType::Dropoff ||
        Type == EntityType::Barracks || Type == EntityType::UtilityStructure;
}

bool IsHostileTo(PlayerId Owner, PlayerId Seat)
{
    return Owner != Seat && Owner != echoes::sim::kNeutralPlayer;
}

template <typename Predicate>
const Entity* NearestEntity(
    const echoes::sim::Simulation& Sim, const Vec2& From, Predicate&& Accept)
{
    const Entity* Best = nullptr;
    int64 BestDistance = 0;
    for (const Entity& Candidate : Sim.Entities())
    {
        if (Candidate.hitPoints <= 0 || !Accept(Candidate)) continue;
        const int64 Distance = DistanceSquaredRaw(Candidate.position, From);
        if (Best == nullptr || Distance < BestDistance)
        {
            Best = &Candidate;
            BestDistance = Distance;
        }
    }
    return Best;
}

const Entity* OwnedCore(const echoes::sim::Simulation& Sim)
{
    for (const Entity& Candidate : Sim.Entities())
    {
        if (Candidate.owner == kReviewSeat && Candidate.type == EntityType::CommandCore &&
            Candidate.hitPoints > 0)
        {
            return &Candidate;
        }
    }
    return nullptr;
}

// A node serves one harvester at a time and queues the rest, so the gather
// line is spread one worker per node: least loaded first, then nearest.
// ExtraLoad carries assignments made earlier in the same pass, before the
// simulation has recorded them.
const Entity* LeastLoadedNode(
    const echoes::sim::Simulation& Sim, const Vec2& From, TMap<uint32, int32>& ExtraLoad)
{
    TMap<uint32, int32> Load = ExtraLoad;
    for (const Entity& E : Sim.Entities())
    {
        if (E.owner != kReviewSeat || E.type != EntityType::Worker || E.hitPoints <= 0) continue;
        const uint32 Node = E.order.type == OrderType::Gather ? E.order.target : E.assignedResourceNode;
        if (Node != 0) ++Load.FindOrAdd(Node);
    }
    const Entity* Best = nullptr;
    int32 BestLoad = 0;
    int64 BestDistance = 0;
    for (const Entity& Node : Sim.Entities())
    {
        if (Node.type != EntityType::ResourceNode || Node.resourceRemaining <= 0) continue;
        const int32 NodeLoad = Load.FindRef(Node.id);
        const int64 Distance = DistanceSquaredRaw(Node.position, From);
        if (Best == nullptr || NodeLoad < BestLoad || (NodeLoad == BestLoad && Distance < BestDistance))
        {
            Best = &Node;
            BestLoad = NodeLoad;
            BestDistance = Distance;
        }
    }
    if (Best != nullptr) ++ExtraLoad.FindOrAdd(Best->id);
    return Best;
}

// Sends one worker to gather: the least-loaded deposit first and, when that
// order is refused (a deposit still under fog cannot be targeted, exactly as
// for the player), the nearest deposit instead. Returns the deposit id taken.
uint32 SendToGather(
    UEchoesSimulationSubsystem& Bridge, const echoes::sim::Simulation& Sim,
    const Entity& Worker, TMap<uint32, int32>& PassLoad, FString& OutFeedback)
{
    const Entity* Spread = LeastLoadedNode(Sim, Worker.position, PassLoad);
    const Entity* Near = NearestEntity(Sim, Worker.position, [](const Entity& E) {
        return E.type == EntityType::ResourceNode && E.resourceRemaining > 0;
    });
    for (const Entity* Node : {Spread, Near})
    {
        if (Node == nullptr) continue;
        if (Bridge.IssueCommand(
                echoes::sim::CommandType::Gather, Worker.id, Node->id,
                Bridge.SimToWorld(Node->position),
                echoes::sim::FutureWellChoice::Dormant, OutFeedback))
        {
            if (Node == Near && Near != Spread) ++PassLoad.FindOrAdd(Near->id);
            return Node->id;
        }
    }
    return 0;
}

// Searches rings of tiles around the Core for a placement the player's own
// preview would accept. Supply structures must also connect to the network,
// or their capacity never counts.
bool FindPlacement(
    const echoes::sim::PlayerView& View, EntityId Builder, EntityType BuildType,
    int32 CenterTileX, int32 CenterTileY, bool bRequireConnection, Vec2& OutPosition,
    int32& OutTileX, int32& OutTileY)
{
    for (int32 Radius = 3; Radius <= 12; ++Radius)
    {
        for (int32 OffsetY = -Radius; OffsetY <= Radius; ++OffsetY)
        {
            for (int32 OffsetX = -Radius; OffsetX <= Radius; ++OffsetX)
            {
                if (FMath::Abs(OffsetX) != Radius && FMath::Abs(OffsetY) != Radius) continue;
                const Vec2 Position = Vec2::FromTiles(CenterTileX + OffsetX, CenterTileY + OffsetY);
                const FEchoesBuildPlacementEvaluation Evaluation =
                    FEchoesBuildPlacementModel::Evaluate(View, Builder, BuildType, Position);
                if (!Evaluation.IsValid()) continue;
                if (bRequireConnection && Evaluation.bNetworkRelevant && !Evaluation.bWillConnect) continue;
                OutPosition = Position;
                OutTileX = CenterTileX + OffsetX;
                OutTileY = CenterTileY + OffsetY;
                return true;
            }
        }
    }
    return false;
}

// Repair work only proceeds while the worker stands within the Meridian
// network, which is a radius around each completed Core or supply node. The
// rendezvous is therefore judged against every network node, not the Core's
// centre, which thirty units and a solid footprint keep a recalled unit from.
bool IsNearOwnedNetworkNode(const echoes::sim::Simulation& Sim, const Vec2& Position)
{
    const int64 Reach = static_cast<int64>(7 * echoes::sim::kFixedScale);
    for (const Entity& Node : Sim.Entities())
    {
        if (Node.owner != kReviewSeat || !Node.completed || Node.hitPoints <= 0) continue;
        if (Node.type != EntityType::CommandCore && Node.type != EntityType::Dropoff) continue;
        if (DistanceSquaredRaw(Position, Node.position) <= Reach * Reach) return true;
    }
    return false;
}

// Who is building this site right now, and where they stand. Diagnostic text
// for the training notes; it reads only public entity state.
FString DescribeBuilders(const echoes::sim::Simulation& Sim, const Entity& Site)
{
    FString Out;
    for (const Entity& W : Sim.Entities())
    {
        if (W.owner != kReviewSeat || W.type != EntityType::Worker || W.hitPoints <= 0) continue;
        if (W.order.type != OrderType::Build || W.order.target != Site.id) continue;
        const int64 DeltaX = static_cast<int64>(W.position.x.Raw()) - Site.position.x.Raw();
        const int64 DeltaY = static_cast<int64>(W.position.y.Raw()) - Site.position.y.Raw();
        Out += FString::Printf(
            TEXT("b%u@%d,%d(raw %d,%d)dist=%.2f res=%u slot=%d cargo=%d queue=%d "),
            W.id, W.position.x.Raw() / echoes::sim::kFixedScale, W.position.y.Raw() / echoes::sim::kFixedScale,
            W.position.x.Raw(), W.position.y.Raw(),
            FMath::Sqrt(static_cast<double>(DeltaX * DeltaX + DeltaY * DeltaY)) / echoes::sim::kFixedScale,
            W.assignedResourceNode, W.harvestSlotHeld ? 1 : 0, W.cargo, static_cast<int32>(W.orderQueue.size()));
    }
    return Out.IsEmpty() ? TEXT("none") : Out;
}

FString JoinIds(const TArray<uint32>& Ids)
{
    FString Out;
    for (const uint32 Id : Ids)
    {
        if (!Out.IsEmpty()) Out += TEXT("+");
        Out += FString::FromInt(static_cast<int32>(Id));
    }
    return Out;
}
}

void AEchoesPlayerController::StartD2ExitReview()
{
    if (bD2ExitReviewActive) return;
    bD2ExitReviewActive = true;
    D2ExitReviewStage = static_cast<int32>(ED2Stage::OpenModes);
    D2ExitReviewStageElapsedSeconds = 0.0f;
    D2ExitReviewTotalElapsedSeconds = 0.0f;
    D2ExitReviewCaptureIndex = 0;
    D2ExitReviewStagesPassed.Reset();
    D2ExitReviewStagesUnproven.Reset();
    D2ExitReviewMatterBeforeGather = 0;
    D2ExitReviewDawnBeforeWell = 0;
    D2ExitReviewBuilderId = 0;
    D2ExitReviewWellWorkerId = 0;
    D2ExitReviewWellId = 0;
    D2ExitReviewBuiltStructureId = 0;
    D2ExitReviewRepairWorkerId = 0;
    D2ExitReviewRepairTargetId = 0;
    D2ExitReviewRepairStartHitPoints = 0;
    D2ExitReviewPeakArmy = 0;
    bD2ExitReviewArmyLimitRefused = false;
    bD2ExitReviewCombatObserved = false;
    D2ExitReviewSaveTick = 0;
    D2ExitReviewSaveChecksum = 0;
    GD2WellOrderPending = false;
    GD2WellRetryElapsed = -10.0f;
    GD2CapturePending = false;
    GD2CaptureName.Reset();
    GD2PendingBuilderId = 0;
    GD2PendingBuilderElapsed = -10.0f;
    GD2LastMoved.Reset();
    GD2StuckNoteElapsed = -100.0f;
    GD2NaiveBuilder = FParse::Param(FCommandLine::Get(), TEXT("EchoesD2ExitReviewNaiveBuilder"));
    D2ExitReviewOutputDir.Reset();
    FParse::Value(FCommandLine::Get(), TEXT("EchoesD2ExitReviewOutputDir="), D2ExitReviewOutputDir);
    if (!D2ExitReviewOutputDir.IsEmpty())
    {
        IFileManager::Get().MakeDirectory(*D2ExitReviewOutputDir, true);
    }
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_D2_EXIT_REVIEW_STARTED] contract=DeliveryPlan-D2-exit map=GlassScar limit=%d outputDir=%s agentDriven=true osInjection=false unaidedHuman=false packaged=false controlledNonshipping=true"),
        echoes::sim::kMobileEntityLimit,
        *D2ExitReviewOutputDir);
}

void AEchoesPlayerController::CaptureD2ExitReview(const TCHAR* Stage)
{
    if (D2ExitReviewOutputDir.IsEmpty()) return;
    const FString OutputPath = FPaths::Combine(
        D2ExitReviewOutputDir,
        FString::Printf(TEXT("%02d-%s.png"), D2ExitReviewCaptureIndex, Stage));
    ++D2ExitReviewCaptureIndex;
    FScreenshotRequest::RequestScreenshot(OutputPath, true, false, false, FIntRect(), true);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_D2_EXIT_REVIEW_CAPTURE] stage=%s showUI=true output=%s"),
        Stage,
        *OutputPath);
}

void AEchoesPlayerController::AdvanceD2ExitReview(
    int32 NextStage, const TCHAR* PassedStage, const FString& Detail)
{
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_D2_EXIT_REVIEW_STAGE] stage=%s result=PASSED elapsed=%.1f detail=%s"),
        PassedStage,
        D2ExitReviewStageElapsedSeconds,
        *Detail);
    if (!D2ExitReviewStagesPassed.IsEmpty()) D2ExitReviewStagesPassed += TEXT(",");
    D2ExitReviewStagesPassed += PassedStage;
    if (GD2CapturePending) CaptureD2ExitReview(*GD2CaptureName);
    GD2CapturePending = true;
    GD2CaptureName = PassedStage;
    D2ExitReviewStage = NextStage;
    D2ExitReviewStageElapsedSeconds = 0.0f;
}

void AEchoesPlayerController::FinishD2ExitReview(const TCHAR* Result, const FString& Detail)
{
    if (GD2CapturePending)
    {
        CaptureD2ExitReview(*GD2CaptureName);
        GD2CapturePending = false;
    }
    bD2ExitReviewActive = false;
    const UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    const echoes::sim::Simulation* Sim = Bridge != nullptr ? Bridge->GetSimulation() : nullptr;
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_D2_EXIT_REVIEW_COMPLETE] result=%s stage=%s passed=%s unproven=%s tick=%llu outcome=%u forfeitingSeat=%u peakArmy=%d limit=%d elapsed=%.1f detail=%s agentDriven=true osInjection=false unaidedHuman=false packaged=false controlledNonshipping=true"),
        Result,
        StageName(D2ExitReviewStage),
        D2ExitReviewStagesPassed.IsEmpty() ? TEXT("none") : *D2ExitReviewStagesPassed,
        D2ExitReviewStagesUnproven.IsEmpty() ? TEXT("none") : *D2ExitReviewStagesUnproven,
        static_cast<unsigned long long>(Sim != nullptr ? Sim->CurrentTick() : 0),
        Bridge != nullptr ? static_cast<uint8>(Bridge->GetMatchOutcome()) : 0,
        Bridge != nullptr ? Bridge->GetForfeitingPlayer() : echoes::sim::kNeutralPlayer,
        D2ExitReviewPeakArmy,
        echoes::sim::kMobileEntityLimit,
        D2ExitReviewTotalElapsedSeconds,
        *Detail);
}

void AEchoesPlayerController::RunD2ExitReviewStage(float DeltaTime)
{
    D2ExitReviewStageElapsedSeconds += DeltaTime;
    D2ExitReviewTotalElapsedSeconds += DeltaTime;
    if (D2ExitReviewTotalElapsedSeconds > kReviewTotalBudgetSeconds)
    {
        FinishD2ExitReview(TEXT("FAILED"), TEXT("TOTAL_BUDGET_EXPIRED"));
        return;
    }
    const float Elapsed = D2ExitReviewStageElapsedSeconds;
    if (GD2CapturePending && Elapsed >= kCaptureDelaySeconds)
    {
        GD2CapturePending = false;
        CaptureD2ExitReview(*GD2CaptureName);
    }
    const auto Stage = [this]() { return static_cast<ED2Stage>(D2ExitReviewStage); };
    const auto Pass = [this](ED2Stage Next, const FString& Detail)
    {
        AdvanceD2ExitReview(static_cast<int32>(Next), StageName(D2ExitReviewStage), Detail);
    };
    const auto Unproven = [this](ED2Stage Next, const FString& Detail)
    {
        const TCHAR* Name = StageName(D2ExitReviewStage);
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_D2_EXIT_REVIEW_STAGE] stage=%s result=UNPROVEN elapsed=%.1f detail=%s"),
            Name,
            D2ExitReviewStageElapsedSeconds,
            *Detail);
        if (!D2ExitReviewStagesUnproven.IsEmpty()) D2ExitReviewStagesUnproven += TEXT(",");
        D2ExitReviewStagesUnproven += Name;
        if (GD2CapturePending) CaptureD2ExitReview(*GD2CaptureName);
        GD2CapturePending = true;
        GD2CaptureName = FString::Printf(TEXT("%s-unproven"), Name);
        D2ExitReviewStage = static_cast<int32>(Next);
        D2ExitReviewStageElapsedSeconds = 0.0f;
    };
    const auto Fail = [this](const TCHAR* Reason)
    {
        FinishD2ExitReview(
            TEXT("FAILED"),
            FString::Printf(
                TEXT("%s_AT_SCREEN_%d shell=%s status=%s"),
                Reason, static_cast<int32>(PlayerFlow.Current()), *ShellMessage, *GetStatusMessage()));
    };
    const auto Select = [this](const TArray<uint32>& Ids)
    {
        ClearSelection();
        for (const uint32 Id : Ids)
        {
            SelectedEntityIds.Add(Id);
            SetEntitySelected(Id, true);
        }
    };
    const auto PanCameraTo = [this](const FVector& World)
    {
        if (AEchoesRTSCameraPawn* Camera = Cast<AEchoesRTSCameraPawn>(GetPawn()))
        {
            Camera->PanToWorld(World);
        }
    };

    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;

    // Shell route. The same three actions the concession review proved, plus the
    // setup override the skirmish overlay offers while the Modes overlay is up.
    if (Stage() == ED2Stage::OpenModes)
    {
        if (Elapsed < 2.0f) return;
        HandleShellAction(EEchoesShellAction::Modes);
        if (PlayerFlow.Current() == EEchoesShellScreen::Confirmation)
        {
            HandleShellAction(EEchoesShellAction::Confirm);
        }
        if (PlayerFlow.Current() != EEchoesShellScreen::Modes)
        {
            Fail(TEXT("SKIRMISH_SETUP_UNREACHABLE"));
            return;
        }
        // An ordinary skirmish the player can pick from the setup overlay. The
        // Defensive opponent holds its own perimeter (it recalls units beyond
        // nine tiles of its Core), which is what lets one bounded session
        // train to the limit before it goes looking for the fight; Standard
        // Adaptive razed the base mid-training in the previous attempt.
        FEchoesSkirmishSetup Setup;
        Setup.MapPreset = EEchoesSkirmishMapPreset::GlassScar;
        Setup.Difficulty = EEchoesSkirmishDifficulty::Story;
        Setup.AiPersonality = echoes::sim::AiPersonality::Defensive;
        Setup.ResourceLevel = EEchoesSkirmishResourceLevel::Abundant;
        Setup.VictoryCondition = EEchoesSkirmishVictoryCondition::Corefall;
        Setup.GameSpeed = EEchoesSkirmishGameSpeed::Fast;
        FString SetupFeedback;
        const bool bSetupApplied = SetPendingSkirmishSetup(Setup, SetupFeedback);
        Pass(
            ED2Stage::OpenBriefing,
            FString::Printf(
                TEXT("setupApplied=%d map=GlassScar resources=Abundant speed=Fast difficulty=Story opponent=Defensive victory=Corefall feedback=%s"),
                bSetupApplied ? 1 : 0,
                *SetupFeedback));
        return;
    }
    if (Stage() == ED2Stage::OpenBriefing)
    {
        if (Elapsed < 0.75f) return;
        HandleShellAction(EEchoesShellAction::Primary);
        if (PlayerFlow.Current() != EEchoesShellScreen::Briefing)
        {
            Fail(TEXT("BRIEFING_UNREACHABLE"));
            return;
        }
        Pass(ED2Stage::Deploy, TEXT("screen=briefing"));
        return;
    }
    if (Stage() == ED2Stage::Deploy)
    {
        if (Elapsed < 0.75f) return;
        HandleShellAction(EEchoesShellAction::Primary);
        if (PlayerFlow.Current() != EEchoesShellScreen::Gameplay)
        {
            Fail(TEXT("DEPLOY_FAILED"));
            return;
        }
        Pass(ED2Stage::GatherIssue, TEXT("screen=gameplay"));
        return;
    }

    if (Bridge == nullptr || Bridge->GetSimulation() == nullptr || !Bridge->IsScenarioReady())
    {
        if (Elapsed > 30.0f) Fail(TEXT("SIM_NOT_READY"));
        return;
    }
    const echoes::sim::Simulation& Sim = *Bridge->GetSimulation();
    const echoes::sim::PlayerState* Seat = Sim.FindPlayer(kReviewSeat);
    if (Seat == nullptr)
    {
        Fail(TEXT("LOCAL_SEAT_MISSING"));
        return;
    }
    const Entity* Core = OwnedCore(Sim);

    // Peak army is sampled every tick after deployment so the record carries
    // the highest fielded count regardless of which stage reached it.
    {
        int32 Mobile = 0;
        for (const Entity& E : Sim.Entities())
        {
            if (E.owner == kReviewSeat && E.hitPoints > 0 &&
                (E.type == EntityType::Worker || IsMobileCombatType(E.type)))
            {
                ++Mobile;
            }
        }
        D2ExitReviewPeakArmy = FMath::Max(D2ExitReviewPeakArmy, Mobile);
    }

    if (Stage() == ED2Stage::GatherIssue)
    {
        if (Elapsed < 2.0f || Core == nullptr) return;
        TArray<uint32> Workers;
        for (const Entity& E : Sim.Entities())
        {
            if (E.owner == kReviewSeat && E.type == EntityType::Worker && E.hitPoints > 0 && E.completed)
            {
                Workers.Add(E.id);
            }
        }
        if (Workers.Num() == 0)
        {
            Fail(TEXT("NO_WORKER"));
            return;
        }
        D2ExitReviewMatterBeforeGather = Seat->resources.material;
        Select(Workers);
        int32 Accepted = 0;
        FString Assignments;
        TMap<uint32, int32> PassLoad;
        for (const uint32 Worker : Workers)
        {
            const Entity* W = Sim.FindEntity(Worker);
            if (W == nullptr) continue;
            FString Feedback;
            const uint32 Node = SendToGather(*Bridge, Sim, *W, PassLoad, Feedback);
            if (Node != 0)
            {
                ++Accepted;
                Assignments += FString::Printf(TEXT("%u>%u "), Worker, Node);
            }
            else
            {
                UE_LOG(LogEchoes, Display, TEXT("[ECHOES_D2_EXIT_REVIEW_NOTE] gatherRefused worker=%u feedback=%s"), Worker, *Feedback);
            }
        }
        if (Accepted == 0)
        {
            Fail(TEXT("GATHER_REFUSED"));
            return;
        }
        D2ExitReviewBuilderId = Workers[0];
        D2ExitReviewWellWorkerId = Workers.Num() > 1 ? Workers.Last() : 0;
        Pass(
            ED2Stage::GatherDeliver,
            FString::Printf(
                TEXT("workers=%s assignments=%saccepted=%d matterBefore=%d"),
                *JoinIds(Workers), *Assignments, Accepted, D2ExitReviewMatterBeforeGather));
        return;
    }
    if (Stage() == ED2Stage::GatherDeliver)
    {
        if (Seat->resources.material > D2ExitReviewMatterBeforeGather)
        {
            Pass(
                ED2Stage::BuildIssue,
                FString::Printf(
                    TEXT("matter=%d->%d tick=%llu"),
                    D2ExitReviewMatterBeforeGather, Seat->resources.material,
                    static_cast<unsigned long long>(Sim.CurrentTick())));
            return;
        }
        if (Elapsed > 180.0f) Fail(TEXT("NO_DELIVERY"));
        return;
    }
    if (Stage() == ED2Stage::BuildIssue)
    {
        if (Elapsed < 0.5f || Core == nullptr) return;
        const Entity* Builder = Sim.FindEntity(D2ExitReviewBuilderId);
        if (Builder == nullptr || Builder->hitPoints <= 0)
        {
            Fail(TEXT("BUILDER_LOST"));
            return;
        }
        // Send the Well worker now so its walk overlaps the construction wait.
        if (const Entity* Well = NearestEntity(Sim, Core->position, [](const Entity& E) {
                return E.type == EntityType::FutureWell;
            }))
        {
            D2ExitReviewWellId = Well->id;
            D2ExitReviewDawnBeforeWell = Seat->resources.dawnshards;
            FString WellFeedback;
            if (D2ExitReviewWellWorkerId != 0 &&
                !Bridge->IssueCommand(
                    echoes::sim::CommandType::FutureWell, D2ExitReviewWellWorkerId, Well->id,
                    Bridge->SimToWorld(Well->position),
                    echoes::sim::FutureWellChoice::Harvest, WellFeedback))
            {
                FString MoveFeedback;
                const bool bWalking = Bridge->IssueCommand(
                    echoes::sim::CommandType::Move, D2ExitReviewWellWorkerId, 0,
                    Bridge->SimToWorld(Well->position),
                    echoes::sim::FutureWellChoice::Dormant, MoveFeedback);
                UE_LOG(
                    LogEchoes, Display,
                    TEXT("[ECHOES_D2_EXIT_REVIEW_NOTE] wellCommandRefused=%s walkingToWell=%d moveFeedback=%s"),
                    *WellFeedback, bWalking ? 1 : 0, *MoveFeedback);
                if (bWalking) GD2WellOrderPending = true;
                else D2ExitReviewWellWorkerId = 0;
            }
        }
        const std::optional<echoes::sim::PlayerView> View = Bridge->GetLocalPlayerView();
        if (!View.has_value())
        {
            Fail(TEXT("NO_PLAYER_VIEW"));
            return;
        }
        const int32 CoreTileX = Core->position.x.Raw() / echoes::sim::kFixedScale;
        const int32 CoreTileY = Core->position.y.Raw() / echoes::sim::kFixedScale;
        const EntityType Candidates[] = {EntityType::Barracks, EntityType::Dropoff};
        for (const EntityType BuildType : Candidates)
        {
            Vec2 Position{};
            int32 TileX = 0;
            int32 TileY = 0;
            if (!FindPlacement(
                    *View, Builder->id, BuildType, CoreTileX, CoreTileY,
                    // REL-FAC-002.PROD: a Foundry outside the network would
                    // never train the army this chain needs.
                    BuildType == EntityType::Dropoff || BuildType == EntityType::Barracks,
                    Position, TileX, TileY))
            {
                continue;
            }
            FString Feedback;
            Select({Builder->id});
            if (!Bridge->IssueBuildCommand(Builder->id, BuildType, Bridge->SimToWorld(Position), Feedback))
            {
                UE_LOG(LogEchoes, Display, TEXT("[ECHOES_D2_EXIT_REVIEW_NOTE] buildRefused=%s"), *Feedback);
                continue;
            }
            Pass(
                ED2Stage::BuildSite,
                FString::Printf(
                    TEXT("type=%u tile=%d,%d builder=%u wellWorker=%u well=%u"),
                    static_cast<uint8>(BuildType), TileX, TileY,
                    Builder->id, D2ExitReviewWellWorkerId, D2ExitReviewWellId));
            return;
        }
        Fail(TEXT("NO_VALID_PLACEMENT"));
        return;
    }
    if (Stage() == ED2Stage::BuildSite)
    {
        for (const Entity& E : Sim.Entities())
        {
            if (E.owner == kReviewSeat && !E.completed && IsStructureType(E.type) && E.hitPoints > 0)
            {
                D2ExitReviewBuiltStructureId = E.id;
                Pass(
                    ED2Stage::BuildComplete,
                    FString::Printf(
                        TEXT("site=%u type=%u progress=%d/%d"),
                        E.id, static_cast<uint8>(E.type), E.constructionProgress, E.constructionRequired));
                return;
            }
        }
        if (Elapsed > 90.0f) Fail(TEXT("SITE_NEVER_PLACED"));
        return;
    }
    if (Stage() == ED2Stage::BuildComplete)
    {
        const Entity* Site = Sim.FindEntity(D2ExitReviewBuiltStructureId);
        if (Site == nullptr || Site->hitPoints <= 0)
        {
            Fail(TEXT("SITE_LOST"));
            return;
        }
        if (Site->completed)
        {
            Pass(
                ED2Stage::WellHarvest,
                FString::Printf(
                    TEXT("structure=%u type=%u hp=%d/%d tick=%llu"),
                    Site->id, static_cast<uint8>(Site->type), Site->hitPoints, Site->maxHitPoints,
                    static_cast<unsigned long long>(Sim.CurrentTick())));
            return;
        }
        if (Elapsed > 360.0f) Fail(TEXT("CONSTRUCTION_STALLED"));
        return;
    }
    if (Stage() == ED2Stage::WellHarvest)
    {
        if (D2ExitReviewWellWorkerId == 0)
        {
            Unproven(ED2Stage::TrainToLimit, TEXT("no Well command was accepted"));
            return;
        }
        if (GD2WellOrderPending && (Elapsed - GD2WellRetryElapsed >= 1.0f || Elapsed < GD2WellRetryElapsed))
        {
            GD2WellRetryElapsed = Elapsed;
            if (const Entity* Well = Sim.FindEntity(D2ExitReviewWellId))
            {
                FString WellFeedback;
                if (Bridge->IssueCommand(
                        echoes::sim::CommandType::FutureWell, D2ExitReviewWellWorkerId, Well->id,
                        Bridge->SimToWorld(Well->position),
                        echoes::sim::FutureWellChoice::Harvest, WellFeedback))
                {
                    GD2WellOrderPending = false;
                    Select({D2ExitReviewWellWorkerId});
                    PanCameraTo(Bridge->SimToWorld(Well->position));
                    UE_LOG(
                        LogEchoes, Display,
                        TEXT("[ECHOES_D2_EXIT_REVIEW_NOTE] wellCommandAccepted worker=%u well=%u tick=%llu"),
                        D2ExitReviewWellWorkerId, Well->id, static_cast<unsigned long long>(Sim.CurrentTick()));
                }
            }
        }
        if (Seat->resources.dawnshards > D2ExitReviewDawnBeforeWell)
        {
            if (const Entity* Well = Sim.FindEntity(D2ExitReviewWellId))
            {
                PanCameraTo(Bridge->SimToWorld(Well->position));
            }
            Select({D2ExitReviewWellWorkerId});
            Pass(
                ED2Stage::TrainToLimit,
                FString::Printf(
                    TEXT("dawn=%d->%d well=%u worker=%u tick=%llu"),
                    D2ExitReviewDawnBeforeWell, Seat->resources.dawnshards, D2ExitReviewWellId,
                    D2ExitReviewWellWorkerId, static_cast<unsigned long long>(Sim.CurrentTick())));
            return;
        }
        const Entity* Worker = Sim.FindEntity(D2ExitReviewWellWorkerId);
        if (Worker == nullptr || Worker->hitPoints <= 0 || Elapsed > 300.0f)
        {
            Unproven(
                ED2Stage::TrainToLimit,
                FString::Printf(
                    TEXT("dawn=%d->%d workerAlive=%d elapsed=%.0f"),
                    D2ExitReviewDawnBeforeWell, Seat->resources.dawnshards,
                    Worker != nullptr && Worker->hitPoints > 0 ? 1 : 0, Elapsed));
        }
        return;
    }
    if (Stage() == ED2Stage::TrainToLimit)
    {
        if (Core != nullptr) PanCameraTo(Bridge->SimToWorld(Core->position));
        TArray<uint32> Producers;
        TArray<uint32> Barracks;
        for (const Entity& E : Sim.Entities())
        {
            if (E.owner != kReviewSeat || E.hitPoints <= 0 || !E.completed) continue;
            if (E.type == EntityType::Barracks) { Barracks.Add(E.id); Producers.Add(E.id); }
            else if (E.type == EntityType::CommandCore) Producers.Add(E.id);
        }
        if (Barracks.Num() == 0)
        {
            Fail(TEXT("NO_BARRACKS"));
            return;
        }
        // Idle workers keep the economy moving while the army fills, one per
        // node where the map allows it.
        {
            TMap<uint32, int32> PassLoad;
            for (const Entity& E : Sim.Entities())
            {
                if (E.owner == kReviewSeat && E.type == EntityType::Worker && E.completed &&
                    E.hitPoints > 0 && E.order.type == OrderType::None && E.id != D2ExitReviewWellWorkerId &&
                    !(E.id == GD2PendingBuilderId && Elapsed - GD2PendingBuilderElapsed < 2.0f))
                {
                    FString Feedback;
                    (void)SendToGather(*Bridge, Sim, E, PassLoad, Feedback);
                }
            }
        }
        // Ask each producer for the unit it makes; the pair that reports the
        // limit is the pair the HUD proof must use, because the command path
        // reports funding before the limit and a Foundry short of Matter would
        // answer INSUFFICIENT_RESOURCES instead.
        uint32 LimitProducer = 0;
        EntityType LimitUnit = EntityType::Worker;
        for (const uint32 Producer : Producers)
        {
            const Entity* P = Sim.FindEntity(Producer);
            const EntityType Unit = P != nullptr && P->type == EntityType::Barracks
                ? EntityType::Soldier : EntityType::Worker;
            const echoes::sim::ProductionStartBlockReason Reason =
                Bridge->GetLocalProductionStartBlockReason(Producer, Unit);
            if (Reason == echoes::sim::ProductionStartBlockReason::MobileEntityLimit && LimitProducer == 0)
            {
                LimitProducer = Producer;
                LimitUnit = Unit;
            }
        }
        if (LimitProducer != 0)
        {
            // Read the refusal through the player's own production path so the
            // HUD status line carries it in the capture.
            Select({LimitProducer});
            ProduceUnit(LimitUnit);
            const FString Status = GetStatusMessage();
            bD2ExitReviewArmyLimitRefused = Status.Contains(TEXT("ARMY_LIMIT"));
            const std::optional<echoes::sim::PlayerView> View = Bridge->GetLocalPlayerView();
            const int32 Mobile = View.has_value() ? View->MobileEntityCount() : -1;
            const int32 Reserved = View.has_value() ? View->MobileEntityReservations() : -1;
            if (bD2ExitReviewArmyLimitRefused)
            {
                Pass(
                    ED2Stage::Fight,
                    FString::Printf(
                        TEXT("producer=%u unit=%u mobile=%d reserved=%d limit=%d status=%s"),
                        LimitProducer, static_cast<uint8>(LimitUnit), Mobile, Reserved,
                        echoes::sim::kMobileEntityLimit, *Status));
            }
            else
            {
                Unproven(
                    ED2Stage::Fight,
                    FString::Printf(
                        TEXT("limit reason reported by bridge but HUD status lacked ARMY_LIMIT: producer=%u unit=%u mobile=%d reserved=%d status=%s"),
                        LimitProducer, static_cast<uint8>(LimitUnit), Mobile, Reserved, *Status));
            }
            return;
        }
        // Stuck-unit detector (diagnostic only).
        {
            FString Stuck;
            for (const Entity& E : Sim.Entities())
            {
                if (E.owner != kReviewSeat || E.hitPoints <= 0 || !E.completed) continue;
                if (E.type != EntityType::Worker && !IsMobileCombatType(E.type)) continue;
                FD2Seen& Seen = GD2LastMoved.FindOrAdd(E.id);
                const int64 MovedX = static_cast<int64>(E.position.x.Raw()) - Seen.RawX;
                const int64 MovedY = static_cast<int64>(E.position.y.Raw()) - Seen.RawY;
                const int64 OneTile = echoes::sim::kFixedScale;
                if (Seen.SinceElapsed == 0.0f || MovedX * MovedX + MovedY * MovedY > OneTile * OneTile)
                {
                    Seen.RawX = E.position.x.Raw();
                    Seen.RawY = E.position.y.Raw();
                    Seen.SinceElapsed = D2ExitReviewTotalElapsedSeconds;
                    continue;
                }
                const bool bMovementOrder = E.order.type == OrderType::Build || E.order.type == OrderType::Move ||
                    (E.order.type == OrderType::Gather && !E.harvestSlotHeld && E.cargo == 0);
                if (bMovementOrder && D2ExitReviewTotalElapsedSeconds - Seen.SinceElapsed >= 20.0f)
                {
                    Stuck += FString::Printf(TEXT("%u:%s@%d,%d(raw %d,%d)->%u for %.0fs "), E.id,
                        E.order.type == OrderType::Build ? TEXT("Build") : E.order.type == OrderType::Move ? TEXT("Move") : TEXT("Gather"),
                        E.position.x.Raw() / echoes::sim::kFixedScale, E.position.y.Raw() / echoes::sim::kFixedScale,
                        E.position.x.Raw(), E.position.y.Raw(), E.order.target,
                        D2ExitReviewTotalElapsedSeconds - Seen.SinceElapsed);
                }
            }
            if (!Stuck.IsEmpty() && (Elapsed - GD2StuckNoteElapsed >= 10.0f || Elapsed < GD2StuckNoteElapsed))
            {
                GD2StuckNoteElapsed = Elapsed;
                UE_LOG(LogEchoes, Display, TEXT("[ECHOES_D2_EXIT_REVIEW_NOTE] stuckUnits %stick=%llu"), *Stuck,
                    static_cast<unsigned long long>(Sim.CurrentTick()));
            }
        }
        // Logistics: the Core and each connected supply node (the Dropoff type
        // in this project) add capacity; production stops at the ceiling long
        // before the 30-entity limit. Raise it the way the player does, one
        // supply node at a time near the Core, whenever the line is close to
        // the ceiling and nothing is already under construction.
        static float LastSupplyElapsed = -10.0f;
        if (Core != nullptr && (Elapsed - LastSupplyElapsed >= 2.0f || Elapsed < LastSupplyElapsed))
        {
            LastSupplyElapsed = Elapsed;
            const std::optional<echoes::sim::PlayerView> View = Bridge->GetLocalPlayerView();
            bool bSiteUnderway = false;
            int32 SupplyNodes = 0;
            const Entity* Site = nullptr;
            for (const Entity& E : Sim.Entities())
            {
                if (E.owner != kReviewSeat || E.hitPoints <= 0 || !IsStructureType(E.type)) continue;
                if (!E.completed) { bSiteUnderway = true; if (Site == nullptr) Site = &E; }
                if (E.type == EntityType::Dropoff) ++SupplyNodes;
            }
            // A site that stops progressing is what a player notices and fixes:
            // first send another worker to assist, then cancel it so the next
            // pass places a fresh one. In one attempt a node ordered at 5,5
            // never progressed and the ceiling held for the whole budget.
            static uint32 WatchedSiteId = 0;
            static int32 WatchedSiteProgress = -1;
            static float WatchedSiteProgressElapsed = 0.0f;
            static bool bWatchedSiteAssisted = false;
            if (Site == nullptr)
            {
                WatchedSiteId = 0;
            }
            else if (Site->id != WatchedSiteId || Site->constructionProgress > WatchedSiteProgress ||
                     Elapsed < WatchedSiteProgressElapsed)
            {
                WatchedSiteId = Site->id;
                WatchedSiteProgress = Site->constructionProgress;
                WatchedSiteProgressElapsed = Elapsed;
                bWatchedSiteAssisted = false;
            }
            else if (!bWatchedSiteAssisted && Elapsed - WatchedSiteProgressElapsed >= 30.0f)
            {
                bWatchedSiteAssisted = true;
                const Entity* Helper = NearestEntity(Sim, Site->position, [this](const Entity& E) {
                    return E.owner == kReviewSeat && E.type == EntityType::Worker && E.completed &&
                        E.id != D2ExitReviewWellWorkerId && E.order.type != OrderType::Build;
                });
                FString Feedback;
                const bool bAssist = Helper != nullptr &&
                    Bridge->IssueConstructionAssistCommand(Helper->id, Site->id, Feedback);
                UE_LOG(LogEchoes, Display,
                    TEXT("[ECHOES_D2_EXIT_REVIEW_NOTE] siteStalled site=%u progress=%d/%d builders=[%s] assist=%d helper=%u feedback=%s"),
                    Site->id, Site->constructionProgress, Site->constructionRequired, *DescribeBuilders(Sim, *Site),
                    bAssist ? 1 : 0, Helper != nullptr ? Helper->id : 0, *Feedback);
            }
            else if (Elapsed - WatchedSiteProgressElapsed >= 90.0f)
            {
                FString Feedback;
                const bool bCancelled = Bridge->IssueConstructionCancellation(Site->id, Feedback);
                UE_LOG(LogEchoes, Display,
                    TEXT("[ECHOES_D2_EXIT_REVIEW_NOTE] siteStalled site=%u progress=%d/%d cancelled=%d feedback=%s"),
                    Site->id, Site->constructionProgress, Site->constructionRequired, bCancelled ? 1 : 0, *Feedback);
                WatchedSiteId = 0;
            }
            const bool bNearCeiling = View.has_value() &&
                View->PopulationUsed() + 4 >= View->PopulationCapacity();
            if (View.has_value() && bNearCeiling && !bSiteUnderway && SupplyNodes < 6)
            {
                // Prefer a worker that is provably mobile: one extracting at a
                // deposit or carrying cargo has reached somewhere recently. A
                // gatherer that never arrived would take the order and never
                // move, which is what happened to worker 41 in runs 12 and 13.
                const Entity* Builder = NearestEntity(Sim, Core->position, [this](const Entity& E) {
                    return E.owner == kReviewSeat && E.type == EntityType::Worker && E.completed &&
                        E.id != D2ExitReviewWellWorkerId && E.order.type == OrderType::Gather &&
                        (GD2NaiveBuilder || E.harvestSlotHeld || E.cargo > 0);
                });
                if (Builder == nullptr)
                {
                    Builder = NearestEntity(Sim, Core->position, [this](const Entity& E) {
                        return E.owner == kReviewSeat && E.type == EntityType::Worker && E.completed &&
                            E.id != D2ExitReviewWellWorkerId && E.order.type != OrderType::Build;
                    });
                }
                Vec2 Position{};
                int32 TileX = 0;
                int32 TileY = 0;
                if (Builder != nullptr &&
                    FindPlacement(
                        *View, Builder->id, EntityType::Dropoff,
                        Core->position.x.Raw() / echoes::sim::kFixedScale,
                        Core->position.y.Raw() / echoes::sim::kFixedScale,
                        true, Position, TileX, TileY))
                {
                    FString Feedback;
                    Select({Builder->id});
                    const bool bIssued = Bridge->IssueBuildCommand(
                        Builder->id, EntityType::Dropoff, Bridge->SimToWorld(Position), Feedback);
                    if (bIssued)
                    {
                        GD2PendingBuilderId = Builder->id;
                        GD2PendingBuilderElapsed = Elapsed;
                    }
                    UE_LOG(
                        LogEchoes, Display,
                        TEXT("[ECHOES_D2_EXIT_REVIEW_NOTE] supplyNode issued=%d builder=%u builderOrder=%u tile=%d,%d logistics=%d/%d nodes=%d feedback=%s"),
                        bIssued ? 1 : 0, Builder->id, static_cast<uint8>(Builder->order.type), TileX, TileY,
                        View->PopulationUsed(), View->PopulationCapacity(), SupplyNodes, *Feedback);
                }
            }
        }
        // Issue at most one production order per producer per pass, on a
        // one-second cadence, through the player's ProduceUnit path. Workers
        // go first: they are the income that pays for everything after them.
        static float LastIssueElapsed = -10.0f;
        static float LastReasonLogElapsed = -100.0f;
        if (Elapsed - LastIssueElapsed >= 1.0f || Elapsed < LastIssueElapsed)
        {
            LastIssueElapsed = Elapsed;
            int32 WorkerCount = 0;
            for (const Entity& E : Sim.Entities())
            {
                if (E.owner == kReviewSeat && E.type == EntityType::Worker && E.hitPoints > 0) ++WorkerCount;
            }
            TArray<uint32> ReadyCores;
            for (const uint32 Producer : Producers)
            {
                const Entity* P = Sim.FindEntity(Producer);
                if (P != nullptr && P->type == EntityType::CommandCore &&
                    Bridge->GetLocalProductionStartBlockReason(Producer, EntityType::Worker) ==
                        echoes::sim::ProductionStartBlockReason::None)
                {
                    ReadyCores.Add(Producer);
                }
            }
            if (ReadyCores.Num() > 0)
            {
                Select(ReadyCores);
                ProduceUnit(EntityType::Worker);
            }
            TArray<uint32> Ready;
            for (const uint32 Producer : Barracks)
            {
                // Hold the soldier line until the gather line has some depth.
                if (WorkerCount < 8) break;
                if (Bridge->GetLocalProductionStartBlockReason(Producer, EntityType::Soldier) ==
                    echoes::sim::ProductionStartBlockReason::None)
                {
                    Ready.Add(Producer);
                }
            }
            if (Ready.Num() > 0)
            {
                Select(Ready);
                ProduceUnit(EntityType::Soldier);
            }
            bool bOpenSite = false;
            for (const Entity& E : Sim.Entities())
            {
                if (E.owner == kReviewSeat && E.hitPoints > 0 && IsStructureType(E.type) && !E.completed) { bOpenSite = true; break; }
            }
            if (Elapsed - LastReasonLogElapsed >= (bOpenSite ? 5.0f : 30.0f) || Elapsed < LastReasonLogElapsed)
            {
                LastReasonLogElapsed = Elapsed;
                FString Reasons;
                for (const uint32 Producer : Producers)
                {
                    const Entity* P = Sim.FindEntity(Producer);
                    const EntityType Unit = P != nullptr && P->type == EntityType::Barracks
                        ? EntityType::Soldier : EntityType::Worker;
                    Reasons += FString::Printf(
                        TEXT("%u:%u "), Producer,
                        static_cast<uint8>(Bridge->GetLocalProductionStartBlockReason(Producer, Unit)));
                }
                FString Sites;
                for (const Entity& E : Sim.Entities())
                {
                    if (E.owner == kReviewSeat && E.hitPoints > 0 && IsStructureType(E.type) && !E.completed)
                    {
                        Sites += FString::Printf(TEXT("%u:%d/%d@%d,%d(raw %d,%d) builders=[%s] "), E.id, E.constructionProgress,
                            E.constructionRequired, E.position.x.Raw() / echoes::sim::kFixedScale,
                            E.position.y.Raw() / echoes::sim::kFixedScale, E.position.x.Raw(), E.position.y.Raw(),
                            *DescribeBuilders(Sim, E));
                    }
                }
                UE_LOG(
                    LogEchoes, Display,
                    TEXT("[ECHOES_D2_EXIT_REVIEW_NOTE] training workers=%d peakArmy=%d matter=%d dawn=%d blockReasons=%s sites=%s tick=%llu"),
                    WorkerCount, D2ExitReviewPeakArmy, Seat->resources.material, Seat->resources.dawnshards,
                    *Reasons, Sites.IsEmpty() ? TEXT("none") : *Sites, static_cast<unsigned long long>(Sim.CurrentTick()));
            }
        }
        if (Elapsed > 600.0f)
        {
            Unproven(
                ED2Stage::Fight,
                FString::Printf(
                    TEXT("limit not reached in budget: peakArmy=%d matter=%d dawn=%d"),
                    D2ExitReviewPeakArmy, Seat->resources.material, Seat->resources.dawnshards));
        }
        return;
    }
    if (Stage() == ED2Stage::Fight)
    {
        static TMap<uint32, int32> HitPointsBefore;
        static float LastOrderElapsed = -100.0f;
        static float FirstHostileHitElapsed = -1.0f;
        static FString FirstHostileHit;
        if (Elapsed < LastOrderElapsed) { HitPointsBefore.Reset(); LastOrderElapsed = -100.0f; FirstHostileHitElapsed = -1.0f; FirstHostileHit.Reset(); }
        const Entity* EnemyCore = Core != nullptr
            ? NearestEntity(Sim, Core->position, [](const Entity& E) {
                  return E.type == EntityType::CommandCore && IsHostileTo(E.owner, kReviewSeat);
              })
            : nullptr;
        if (EnemyCore == nullptr)
        {
            Unproven(
                ED2Stage::SaveLoad,
                Core == nullptr ? TEXT("the local Command Core has fallen")
                                : TEXT("no hostile Command Core is present"));
            return;
        }
        // Detect a health change on either side before re-issuing orders. The
        // stage passes on the first hostile loss, but it holds contact until an
        // owned unit is hurt as well (or 150 s pass), because the repair stage
        // after the rewind needs a damaged owned target to exist at save time.
        const Entity* Changed = nullptr;
        int32 Before = 0;
        const Entity* HostileChanged = nullptr;
        int32 HostileBefore = 0;
        for (const Entity& E : Sim.Entities())
        {
            const bool bOwnCombat = E.owner == kReviewSeat && IsMobileCombatType(E.type);
            const bool bHostile = IsHostileTo(E.owner, kReviewSeat);
            if (!bOwnCombat && !bHostile) continue;
            if (const int32* Prior = HitPointsBefore.Find(E.id))
            {
                if (E.hitPoints < *Prior)
                {
                    if (bOwnCombat && Changed == nullptr) { Changed = &E; Before = *Prior; }
                    if (bHostile && HostileChanged == nullptr) { HostileChanged = &E; HostileBefore = *Prior; }
                }
            }
            HitPointsBefore.Add(E.id, E.hitPoints);
        }
        if (HostileChanged != nullptr && FirstHostileHitElapsed < 0.0f)
        {
            FirstHostileHitElapsed = Elapsed;
            FirstHostileHit = FString::Printf(
                TEXT("hostile=%u type=%u hp=%d->%d/%d tick=%llu"),
                HostileChanged->id, static_cast<uint8>(HostileChanged->type), HostileBefore,
                HostileChanged->hitPoints, HostileChanged->maxHitPoints,
                static_cast<unsigned long long>(Sim.CurrentTick()));
            UE_LOG(LogEchoes, Display, TEXT("[ECHOES_D2_EXIT_REVIEW_NOTE] firstHostileLoss %s"), *FirstHostileHit);
        }
        if (Changed == nullptr && FirstHostileHitElapsed >= 0.0f && Elapsed - FirstHostileHitElapsed >= 150.0f)
        {
            // Contact held long enough; take the hostile loss as the fight proof.
            Changed = HostileChanged != nullptr ? HostileChanged : nullptr;
            Before = HostileBefore;
            if (Changed == nullptr)
            {
                for (const Entity& E : Sim.Entities())
                {
                    if (IsHostileTo(E.owner, kReviewSeat) && E.hitPoints > 0) { Changed = &E; Before = E.hitPoints; break; }
                }
            }
        }
        if (Changed != nullptr)
        {
            bD2ExitReviewCombatObserved = true;
            PanCameraTo(Bridge->SimToWorld(Changed->position));
            TArray<uint32> Army;
            for (const Entity& E : Sim.Entities())
            {
                if (E.owner == kReviewSeat && IsMobileCombatType(E.type) && E.hitPoints > 0) Army.Add(E.id);
            }
            Select(Army);
            // Disengage to mid-map on our side: the save, load and repair
            // stages need a match that is still running (an earlier attempt
            // ended by Corefall before the repair could land), and the army
            // must not fall back into the base ring, where thirty bodies and
            // solid footprints have stalled builders and a repair worker. The
            // outcome stage sends it back out.
            if (Core != nullptr)
            {
                const Vec2 Home = Vec2::FromRaw(
                    Core->position.x.Raw() + 14 * echoes::sim::kFixedScale,
                    Core->position.y.Raw() + 14 * echoes::sim::kFixedScale);
                int32 Recalled = 0;
                for (const uint32 Unit : Army)
                {
                    FString Feedback;
                    if (Bridge->IssueCommand(
                            echoes::sim::CommandType::Move, Unit, 0, Bridge->SimToWorld(Home),
                            echoes::sim::FutureWellChoice::Dormant, Feedback))
                    {
                        ++Recalled;
                    }
                }
                UE_LOG(LogEchoes, Display, TEXT("[ECHOES_D2_EXIT_REVIEW_NOTE] disengage army=%d recalled=%d tick=%llu"),
                    Army.Num(), Recalled, static_cast<unsigned long long>(Sim.CurrentTick()));
            }
            Pass(
                ED2Stage::SaveLoad,
                FString::Printf(
                    TEXT("entity=%u owner=%u type=%u hp=%d->%d/%d tick=%llu army=%d firstHostileLoss=[%s]"),
                    Changed->id, Changed->owner, static_cast<uint8>(Changed->type), Before,
                    Changed->hitPoints, Changed->maxHitPoints,
                    static_cast<unsigned long long>(Sim.CurrentTick()), Army.Num(),
                    FirstHostileHit.IsEmpty() ? TEXT("none") : *FirstHostileHit));
            return;
        }
        if (Elapsed - LastOrderElapsed >= 20.0f)
        {
            LastOrderElapsed = Elapsed;
            TArray<uint32> Army;
            for (const Entity& E : Sim.Entities())
            {
                if (E.owner == kReviewSeat && IsMobileCombatType(E.type) && E.hitPoints > 0 && E.completed)
                {
                    Army.Add(E.id);
                }
            }
            Select(Army);
            int32 Accepted = 0;
            for (const uint32 Unit : Army)
            {
                FString Feedback;
                if (Bridge->IssueCommand(
                        echoes::sim::CommandType::AttackMove, Unit, 0,
                        Bridge->SimToWorld(EnemyCore->position),
                        echoes::sim::FutureWellChoice::Dormant, Feedback))
                {
                    ++Accepted;
                }
            }
            UE_LOG(
                LogEchoes, Display,
                TEXT("[ECHOES_D2_EXIT_REVIEW_NOTE] attackMove army=%d accepted=%d target=%u tick=%llu"),
                Army.Num(), Accepted, EnemyCore->id, static_cast<unsigned long long>(Sim.CurrentTick()));
        }
        if (Elapsed > 420.0f)
        {
            Unproven(ED2Stage::SaveLoad, TEXT("no health change observed within budget"));
        }
        return;
    }
    if (Stage() == ED2Stage::SaveLoad)
    {
        static int32 Phase = 0;
        static float PhaseElapsed = 0.0f;
        static uint64 PriorRequestId = 0;
        static FString SavePath;
        if (Elapsed < PhaseElapsed) { Phase = 0; }
        PhaseElapsed = Elapsed;
        if (Phase == 0)
        {
            // One and a half seconds so the disengage orders are part of the
            // saved state rather than still queued for the next tick.
            if (Elapsed < 1.5f) return;
            Bridge->SetScenarioPaused(true);
            PriorRequestId = Bridge->GetCheckpointSaveStatus().RequestId;
            D2ExitReviewSaveTick = Sim.CurrentTick();
            D2ExitReviewSaveChecksum = Sim.StateChecksum();
            QuickSaveScenario();
            UE_LOG(
                LogEchoes, Display,
                TEXT("[ECHOES_D2_EXIT_REVIEW_NOTE] quickSaveRequested tick=%llu checksum=%llu priorRequest=%llu status=%s"),
                static_cast<unsigned long long>(D2ExitReviewSaveTick),
                static_cast<unsigned long long>(D2ExitReviewSaveChecksum),
                static_cast<unsigned long long>(PriorRequestId), *GetStatusMessage());
            Phase = 1;
            return;
        }
        if (Phase == 1)
        {
            const FEchoesCheckpointSaveStatus& Status = Bridge->GetCheckpointSaveStatus();
            const bool bOurs = Status.RequestId != PriorRequestId && !Status.bAutosave;
            if (bOurs && Status.State == EEchoesCheckpointSaveState::Succeeded)
            {
                SavePath = Status.SavePath;
                UE_LOG(
                    LogEchoes, Display,
                    TEXT("[ECHOES_D2_EXIT_REVIEW_NOTE] quickSaveCommitted request=%llu tick=%llu path=%s"),
                    static_cast<unsigned long long>(Status.RequestId),
                    static_cast<unsigned long long>(Status.SimulationTick), *SavePath);
                // Let the match run on so the load has something to rewind.
                Bridge->SetScenarioPaused(false);
                Phase = 2;
                return;
            }
            if ((bOurs && Status.State == EEchoesCheckpointSaveState::Failed) || Elapsed > 60.0f)
            {
                Bridge->SetScenarioPaused(false);
                Phase = 0;
                Unproven(
                    ED2Stage::RepairStage,
                    FString::Printf(
                        TEXT("quick save not committed: ours=%d state=%u feedback=%s"),
                        bOurs ? 1 : 0, static_cast<uint8>(Status.State), *Status.Feedback));
            }
            return;
        }
        if (Phase == 2)
        {
            if (Sim.CurrentTick() < D2ExitReviewSaveTick + 300 && Elapsed < 90.0f) return;
            Bridge->SetScenarioPaused(true);
            const uint64 PreLoadTick = Sim.CurrentTick();
            QuickLoadScenario();
            const echoes::sim::Simulation* Loaded = Bridge->GetSimulation();
            const uint64 LoadedTick = Loaded != nullptr ? Loaded->CurrentTick() : 0;
            const uint64 LoadedChecksum = Loaded != nullptr ? Loaded->StateChecksum() : 0;
            const bool bRewound = Loaded != nullptr && LoadedTick == D2ExitReviewSaveTick &&
                LoadedTick < PreLoadTick && LoadedChecksum == D2ExitReviewSaveChecksum;
            Bridge->SetScenarioPaused(false);
            Phase = 0;
            const FString Detail = FString::Printf(
                TEXT("savedTick=%llu ranOnTo=%llu loadedTick=%llu savedChecksum=%llu loadedChecksum=%llu path=%s status=%s"),
                static_cast<unsigned long long>(D2ExitReviewSaveTick),
                static_cast<unsigned long long>(PreLoadTick),
                static_cast<unsigned long long>(LoadedTick),
                static_cast<unsigned long long>(D2ExitReviewSaveChecksum),
                static_cast<unsigned long long>(LoadedChecksum), *SavePath, *GetStatusMessage());
            if (bRewound) Pass(ED2Stage::RepairStage, Detail);
            else Unproven(ED2Stage::RepairStage, Detail);
            return;
        }
        return;
    }
    if (Stage() == ED2Stage::RepairStage)
    {
        if (Elapsed < 1.0f) return;
        if (Core == nullptr)
        {
            Unproven(ED2Stage::Outcome, TEXT("the local Command Core has fallen"));
            return;
        }
        static uint32 RecalledUnitId = 0;
        static float RecallElapsed = 0.0f;
        if (Elapsed < RecallElapsed) { RecalledUnitId = 0; }
        RecallElapsed = Elapsed;
        // Rendezvous six tiles west of the Core: inside the Core's eight-tile
        // network radius, away from the gather line to the east and the
        // supply nodes to the north.
        const Vec2 Rendezvous = Vec2::FromRaw(
            Core->position.x.Raw() - 6 * echoes::sim::kFixedScale, Core->position.y.Raw());
        const Entity* Worker = NearestEntity(Sim, Rendezvous, [](const Entity& E) {
            return E.owner == kReviewSeat && E.type == EntityType::Worker && E.completed &&
                E.order.type != OrderType::FutureWell && E.order.type != OrderType::Build;
        });
        if (Worker == nullptr)
        {
            Unproven(ED2Stage::Outcome, TEXT("no worker available to repair"));
            return;
        }
        // Prefer a damaged owned structure; otherwise recall the most damaged
        // combat unit to the Core so a Meridian worker can repair it in network.
        const Entity* Target = nullptr;
        for (const Entity& E : Sim.Entities())
        {
            if (E.owner == kReviewSeat && E.completed && E.hitPoints > 0 && E.hitPoints < E.maxHitPoints &&
                IsStructureType(E.type))
            {
                Target = &E;
                break;
            }
        }
        if (Target == nullptr)
        {
            const Entity* Damaged = NearestEntity(Sim, Core->position, [](const Entity& E) {
                return E.owner == kReviewSeat && E.completed && E.hitPoints < E.maxHitPoints &&
                    IsMobileCombatType(E.type);
            });
            if (Damaged == nullptr)
            {
                Unproven(ED2Stage::Outcome, TEXT("no damaged owned structure or unit to repair"));
                return;
            }
            const int64 NearRaw = static_cast<int64>(3 * echoes::sim::kFixedScale);
            const bool bNear = DistanceSquaredRaw(Damaged->position, Rendezvous) <= NearRaw * NearRaw &&
                IsNearOwnedNetworkNode(Sim, Damaged->position);
            if (!bNear)
            {
                if (RecalledUnitId != Damaged->id)
                {
                    RecalledUnitId = Damaged->id;
                    FString Feedback;
                    Select({Damaged->id});
                    Bridge->IssueCommand(
                        echoes::sim::CommandType::Move, Damaged->id, 0,
                        Bridge->SimToWorld(Rendezvous),
                        echoes::sim::FutureWellChoice::Dormant, Feedback);
                    UE_LOG(LogEchoes, Display, TEXT("[ECHOES_D2_EXIT_REVIEW_NOTE] recall unit=%u hp=%d/%d tile=%d,%d feedback=%s"),
                        Damaged->id, Damaged->hitPoints, Damaged->maxHitPoints,
                        Damaged->position.x.Raw() / echoes::sim::kFixedScale,
                        Damaged->position.y.Raw() / echoes::sim::kFixedScale, *Feedback);
                }
                if (Elapsed > 240.0f)
                {
                    Unproven(ED2Stage::Outcome, FString::Printf(
                        TEXT("damaged unit %u did not reach the rendezvous in budget (tile %d,%d)"), Damaged->id,
                        Damaged->position.x.Raw() / echoes::sim::kFixedScale,
                        Damaged->position.y.Raw() / echoes::sim::kFixedScale));
                }
                return;
            }
            Target = Damaged;
        }
        FString Feedback;
        Select({Worker->id});
        if (!Bridge->IssueRepairCommand(Worker->id, Target->id, Feedback))
        {
            if (Elapsed > 150.0f)
            {
                Unproven(ED2Stage::Outcome, FString::Printf(TEXT("repair refused: %s"), *Feedback));
            }
            return;
        }
        D2ExitReviewRepairWorkerId = Worker->id;
        D2ExitReviewRepairTargetId = Target->id;
        D2ExitReviewRepairStartHitPoints = Target->hitPoints;
        if (IsMobileCombatType(Target->type))
        {
            FString StopFeedback;
            Bridge->IssueCommand(
                echoes::sim::CommandType::Stop, Target->id, 0, Bridge->SimToWorld(Target->position),
                echoes::sim::FutureWellChoice::Dormant, StopFeedback);
        }
        PanCameraTo(Bridge->SimToWorld(Target->position));
        Pass(
            ED2Stage::RepairWait,
            FString::Printf(TEXT("worker=%u target=%u type=%u hp=%d/%d feedback=%s"),
                Worker->id, Target->id, static_cast<uint8>(Target->type), Target->hitPoints,
                Target->maxHitPoints, *Feedback));
        return;
    }
    if (Stage() == ED2Stage::RepairWait)
    {
        const Entity* Target = Sim.FindEntity(D2ExitReviewRepairTargetId);
        if (Target == nullptr || Target->hitPoints <= 0)
        {
            Unproven(ED2Stage::Outcome, TEXT("repair target was lost"));
            return;
        }
        static float LastRepairNoteElapsed = -100.0f;
        if (Elapsed < LastRepairNoteElapsed) LastRepairNoteElapsed = -100.0f;
        const Entity* RepairWorker = Sim.FindEntity(D2ExitReviewRepairWorkerId);
        if (Elapsed - LastRepairNoteElapsed >= 20.0f)
        {
            LastRepairNoteElapsed = Elapsed;
            UE_LOG(LogEchoes, Display,
                TEXT("[ECHOES_D2_EXIT_REVIEW_NOTE] repairWait worker=%u workerOrder=%u workerTile=%d,%d target=%u targetOrder=%u targetTile=%d,%d hp=%d/%d tick=%llu"),
                D2ExitReviewRepairWorkerId,
                RepairWorker != nullptr ? static_cast<uint8>(RepairWorker->order.type) : 255,
                RepairWorker != nullptr ? RepairWorker->position.x.Raw() / echoes::sim::kFixedScale : -1,
                RepairWorker != nullptr ? RepairWorker->position.y.Raw() / echoes::sim::kFixedScale : -1,
                Target->id, static_cast<uint8>(Target->order.type),
                Target->position.x.Raw() / echoes::sim::kFixedScale,
                Target->position.y.Raw() / echoes::sim::kFixedScale,
                Target->hitPoints, Target->maxHitPoints,
                static_cast<unsigned long long>(Sim.CurrentTick()));
        }
        // The repair order clears itself silently when the worker leaves the
        // network or the target moves; re-issue it while the budget lasts.
        if (RepairWorker != nullptr && RepairWorker->order.type != OrderType::Repair && Elapsed > 5.0f)
        {
            FString Feedback;
            if (Bridge->IssueRepairCommand(RepairWorker->id, Target->id, Feedback))
            {
                UE_LOG(LogEchoes, Display, TEXT("[ECHOES_D2_EXIT_REVIEW_NOTE] repairReissued worker=%u target=%u feedback=%s"),
                    RepairWorker->id, Target->id, *Feedback);
            }
        }
        // Four points is under half a second of Meridian repair; enough for the
        // health bar and the selection readout to have moved in the capture.
        if (Target->hitPoints >= D2ExitReviewRepairStartHitPoints + 4 ||
            Target->hitPoints >= Target->maxHitPoints)
        {
            Select({Target->id});
            PanCameraTo(Bridge->SimToWorld(Target->position));
            Pass(
                ED2Stage::Outcome,
                FString::Printf(TEXT("target=%u hp=%d->%d/%d tick=%llu"),
                    Target->id, D2ExitReviewRepairStartHitPoints, Target->hitPoints, Target->maxHitPoints,
                    static_cast<unsigned long long>(Sim.CurrentTick())));
            return;
        }
        if (Elapsed > 150.0f)
        {
            Unproven(ED2Stage::Outcome,
                FString::Printf(TEXT("hp stayed %d/%d"), Target->hitPoints, Target->maxHitPoints));
        }
        return;
    }
    if (Stage() == ED2Stage::Outcome)
    {
        static float LastOrderElapsed = -100.0f;
        static float ResultSeenElapsed = -1.0f;
        if (Elapsed < LastOrderElapsed) { LastOrderElapsed = -100.0f; ResultSeenElapsed = -1.0f; }
        // The previous stage's capture is still pending for the first moments;
        // the assault's camera pan must not pre-empt it.
        if (GD2CapturePending) return;
        const echoes::sim::MatchOutcome Outcome = Bridge->GetMatchOutcome();
        if (Outcome != echoes::sim::MatchOutcome::Ongoing)
        {
            if (ResultSeenElapsed < 0.0f) ResultSeenElapsed = Elapsed;
            if (!IsMatchResultVisible() && Elapsed - ResultSeenElapsed < 6.0f) return;
            const TCHAR* Kind = Outcome == echoes::sim::MatchOutcome::Player0Victory
                ? TEXT("victory") : Outcome == echoes::sim::MatchOutcome::Draw ? TEXT("draw") : TEXT("defeat");
            Pass(
                ED2Stage::Done,
                FString::Printf(TEXT("outcome=%u kind=%s resultVisible=%d forfeitingSeat=%u banner=%s"),
                    static_cast<uint8>(Outcome), Kind, IsMatchResultVisible() ? 1 : 0,
                    Bridge->GetForfeitingPlayer(), *GetStatusMessage()));
            FinishD2ExitReview(
                D2ExitReviewStagesUnproven.IsEmpty() ? TEXT("PASSED") : TEXT("PARTIAL"),
                FString::Printf(TEXT("ended by match outcome %s"), Kind));
            return;
        }
        const Entity* EnemyCore = Core != nullptr
            ? NearestEntity(Sim, Core->position, [](const Entity& E) {
                  return E.type == EntityType::CommandCore && IsHostileTo(E.owner, kReviewSeat);
              })
            : nullptr;
        if (EnemyCore != nullptr && Elapsed - LastOrderElapsed >= 25.0f)
        {
            LastOrderElapsed = Elapsed;
            TArray<uint32> Army;
            for (const Entity& E : Sim.Entities())
            {
                if (E.owner == kReviewSeat && IsMobileCombatType(E.type) && E.hitPoints > 0 && E.completed &&
                    E.id != D2ExitReviewRepairTargetId)
                {
                    Army.Add(E.id);
                }
            }
            Select(Army);
            for (const uint32 Unit : Army)
            {
                FString Feedback;
                Bridge->IssueCommand(
                    echoes::sim::CommandType::AttackMove, Unit, 0,
                    Bridge->SimToWorld(EnemyCore->position),
                    echoes::sim::FutureWellChoice::Dormant, Feedback);
            }
            PanCameraTo(Bridge->SimToWorld(EnemyCore->position));
            UE_LOG(LogEchoes, Display,
                TEXT("[ECHOES_D2_EXIT_REVIEW_NOTE] assault army=%d enemyCoreHp=%d/%d tick=%llu"),
                Army.Num(), EnemyCore->hitPoints, EnemyCore->maxHitPoints,
                static_cast<unsigned long long>(Sim.CurrentTick()));
        }
        if (Elapsed > 420.0f)
        {
            // The match did not resolve in budget: end it truthfully through the
            // player's own concession route rather than by stopping the process.
            TogglePauseMenu();
            if (PlayerFlow.Current() == EEchoesShellScreen::Pause)
            {
                HandleShellAction(EEchoesShellAction::Concede);
                if (PlayerFlow.Current() == EEchoesShellScreen::Confirmation)
                {
                    HandleShellAction(EEchoesShellAction::Confirm);
                }
            }
            const bool bResult = IsMatchResultVisible();
            const FString Detail = FString::Printf(
                TEXT("undecided in budget; conceded=%d outcome=%u forfeitingSeat=%u enemyCoreHp=%d"),
                bResult ? 1 : 0, static_cast<uint8>(Bridge->GetMatchOutcome()), Bridge->GetForfeitingPlayer(),
                EnemyCore != nullptr ? EnemyCore->hitPoints : -1);
            if (bResult) Pass(ED2Stage::Done, Detail);
            else Unproven(ED2Stage::Done, Detail);
            FinishD2ExitReview(
                D2ExitReviewStagesUnproven.IsEmpty() ? TEXT("PASSED") : TEXT("PARTIAL"),
                TEXT("ended by concession after the outcome budget"));
        }
        return;
    }
    FinishD2ExitReview(TEXT("FAILED"), TEXT("UNKNOWN_STAGE"));
}
