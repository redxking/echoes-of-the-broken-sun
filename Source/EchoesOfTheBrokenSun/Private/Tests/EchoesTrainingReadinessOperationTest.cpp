#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "EchoesCampaignTerrainBinding.h"
#include "EchoesPlayerController.h"
#include "EchoesMatchReplay.h"
#include "EchoesNetworkSession.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesSkirmishSetup.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Tests/AutomationCommon.h"

#include <algorithm>

namespace
{
constexpr uint64 ReadinessSeed = 0xE0C0'B5A1ULL;

template <typename Digest>
FString DigestHex(const Digest& Value)
{
    FString Result;
    Result.Reserve(static_cast<int32>(Value.size() * 2));
    for (const uint8 Byte : Value)
    {
        Result += FString::Printf(TEXT("%02x"), Byte);
    }
    return Result;
}

bool HasCommandFrom(
    std::span<const echoes::sim::Command> Commands,
    echoes::sim::PlayerId Player)
{
    return std::any_of(
        Commands.begin(), Commands.end(),
        [Player](const echoes::sim::Command& Command)
        {
            return Command.player == Player;
        });
}

void RewriteChecksum(TArray<uint8>& Bytes)
{
    constexpr int32 ChecksumBytes = 4;
    const int32 ChecksumOffset = Bytes.Num() - ChecksumBytes;
    const uint32 Checksum = FCrc::MemCrc32(Bytes.GetData(), ChecksumOffset);
    for (int32 Index = 0; Index < ChecksumBytes; ++Index)
    {
        Bytes[ChecksumOffset + Index] =
            static_cast<uint8>(Checksum >> (Index * 8));
    }
}

echoes::sim::ReplayRecord MakeLegacyCompatibleSkirmishReplay()
{
    using namespace echoes::sim;
    SimulationConfig Config{
        FEchoesSkirmishSetupModel::MapWidthTiles,
        FEchoesSkirmishSetupModel::MapHeightTiles,
        20,
        0x4c45474143595250ULL};
    Config.rules.contentSha256 =
        echoes::network::BuildCompatibilityManifest(nullptr).rulesPackSha256;
    EntityArchetypeRules& Soldier = Config.rules.archetypes
        [static_cast<size_t>(Faction::MeridianCompact)]
        [static_cast<size_t>(EntityType::Soldier)];
    Soldier.attackDamage = 5000;
    Soldier.attackRangeRaw = 3 * kFixedScale;
    Soldier.attackPeriodTicks = 1;

    Simulation Candidate(Config);
    for (int32 Y = 0; Y < FEchoesSkirmishSetupModel::MapHeightTiles; ++Y)
    {
        for (int32 X = 0; X < FEchoesSkirmishSetupModel::MapWidthTiles; ++X)
        {
            if (FEchoesSkirmishSetupModel::IsBlockedTile(
                    EEchoesSkirmishMapPreset::GlassScar, X, Y))
            {
                (void)Candidate.SetTerrainTile(X, Y, Terrain::Blocked);
            }
        }
    }
    (void)Candidate.AddPlayer(0, Faction::MeridianCompact, {1000, 1000});
    (void)Candidate.AddPlayer(1, Faction::KharuunAssemblies, {1000, 1000});
    (void)Candidate.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::CommandCore,
        Vec2::FromTiles(2, 2));
    const EntityId EnemyCore = Candidate.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::CommandCore,
        Vec2::FromTiles(18, 18));
    const EntityId Attacker = Candidate.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Soldier,
        Vec2::FromTiles(17, 18));
    Candidate.CaptureReplayBaseline();
    Command Attack;
    Attack.executeTick = 1;
    Attack.player = 0;
    Attack.sequence = 1;
    Attack.type = CommandType::Attack;
    Attack.actor = Attacker;
    Attack.target = EnemyCore;
    (void)Candidate.QueueCommand(Attack);
    for (int32 Tick = 0;
         Tick < 20 && Candidate.Outcome() == MatchOutcome::Ongoing;
         ++Tick)
    {
        Candidate.Step();
    }
    return Candidate.ExportReplay();
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesTrainingReadinessOperationTest,
    "Echoes.Runtime.Training.ReadinessOperationPersistenceAndReplay",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesTrainingReadinessOperationTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    static_assert(
        static_cast<uint8>(EEchoesOperationMode::CampaignTheBrokenSun) == 15);
    static_assert(
        static_cast<uint8>(EEchoesOperationMode::TrainingReadiness) == 16);

    FEchoesScopedTestSaveEnvironment SaveEnvironment(*this);
    if (!SaveEnvironment.IsReady()) return false;

    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the training readiness test world."));
        return false;
    }
    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge = World != nullptr
        ? World->GetSubsystem<UEchoesSimulationSubsystem>()
        : nullptr;
    if (!TestNotNull(TEXT("World owns the simulation subsystem"), Bridge))
    {
        return false;
    }

    FString Feedback;
    if (!TestTrue(
            TEXT("Training readiness can be selected"),
            Bridge->SelectOperationMode(
                EEchoesOperationMode::TrainingReadiness, Feedback)) ||
        !TestTrue(
            TEXT("Training readiness starts"),
            Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const echoes::sim::Simulation* Simulation = Bridge->GetSimulation();
    if (!TestNotNull(TEXT("Training owns an authoritative simulation"), Simulation))
    {
        return false;
    }
    TestEqual(
        TEXT("Training keeps its explicit operation identity"),
        Bridge->GetOperationMode(), EEchoesOperationMode::TrainingReadiness);
    TestEqual(
        TEXT("Training reuses the authorized M01 seed"),
        Simulation->Config().randomSeed, ReadinessSeed);
    TestTrue(
        TEXT("Training retains ordinary hostile Corefall relationships"),
        Simulation->Config().hostilityMasks ==
            echoes::sim::kDefaultHostilityMasks);

    int32 LocalWorkers = 0;
    int32 LocalSoldiers = 0;
    int32 OpponentEntities = 0;
    for (const echoes::sim::Entity& Entity : Simulation->Entities())
    {
        if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId)
        {
            LocalWorkers += Entity.type == echoes::sim::EntityType::Worker;
            LocalSoldiers += Entity.type == echoes::sim::EntityType::Soldier;
        }
        OpponentEntities +=
            Entity.owner == UEchoesSimulationSubsystem::OpponentPlayerId;
    }
    TestEqual(TEXT("Training reuses six M01 Surveyors"), LocalWorkers, 6);
    TestEqual(TEXT("Training reuses two M01 Lancers"), LocalSoldiers, 2);
    TestTrue(TEXT("Training preserves the M01 opponent force"), OpponentEntities > 0);

    const echoes::world::CampaignTerrainResult M01Terrain =
        echoes::world::CheckCampaignTerrain(
            1, echoes::sim::FutureWellChoice::Preserve);
    TestTrue(TEXT("Authored M01 terrain is available"), M01Terrain.ok);
    TestEqual(
        TEXT("Training uses the M01 map identity"),
        FString(UTF8_TO_TCHAR(M01Terrain.map_id)),
        FString(TEXT("glass-scar-evacuation-margin")));
    for (int32 Y = 0; Y < 64; ++Y)
    {
        for (int32 X = 0; X < 64; ++X)
        {
            const echoes::sim::Terrain Expected =
                echoes::world::IsCampaignTerrainPassable(
                    1, echoes::sim::FutureWellChoice::Preserve, X, Y)
                    ? echoes::sim::Terrain::Open
                    : echoes::sim::Terrain::Blocked;
            if (Simulation->TerrainAt(X, Y) != Expected)
            {
                AddError(FString::Printf(
                    TEXT("Training terrain diverged from authored M01 at %d,%d."),
                    X, Y));
                return false;
            }
        }
    }

    const int32 TrainingPendingBefore =
        static_cast<int32>(Simulation->PendingCommands().size());
    Bridge->QueueOpponentCommands();
    TestEqual(
        TEXT("Training does not queue proactive opponent commands"),
        static_cast<int32>(Simulation->PendingCommands().size()),
        TrainingPendingBefore);
    TestFalse(
        TEXT("Training pending state contains no opponent command"),
        HasCommandFrom(
            Simulation->PendingCommands(),
            UEchoesSimulationSubsystem::OpponentPlayerId));

    const TArray<FEchoesCampaignDecisionRecord> LedgerBefore =
        Bridge->GetCampaignProgress().Decisions;
    if (!TestTrue(TEXT("Training quick-save succeeds"), Bridge->QuickSaveScenario(Feedback)) ||
        !TestTrue(TEXT("Training quick-save commits"), Bridge->WaitForCheckpointSaves(Feedback)))
    {
        return false;
    }
    const FString TrainingSavePath = Bridge->GetActiveQuickSavePath();
    TArray<uint8> TrainingSaveBytes;
    TestTrue(
        TEXT("Training checkpoint exists"),
        FFileHelper::LoadFileToArray(TrainingSaveBytes, *TrainingSavePath));
    uint8 ContainerVersion = 0;
    EEchoesOperationMode ContainerOperation = EEchoesOperationMode::Skirmish;
    echoes::sim::Faction ContainerFaction = echoes::sim::Faction::HollowChoir;
    uint64 SetupIdentity = 0;
    uint32 ContainerCrc = 0;
    TArray<uint8> ContainerPayload;
    FString InspectError;
    TestTrue(
        TEXT("Training checkpoint inspection succeeds"),
        UEchoesSimulationSubsystem::InspectSaveContainer(
            TrainingSaveBytes, ContainerVersion, ContainerOperation,
            ContainerFaction, SetupIdentity, ContainerCrc,
            ContainerPayload, InspectError));
    TestEqual(
        TEXT("Checkpoint carries TrainingReadiness identity"),
        ContainerOperation, EEchoesOperationMode::TrainingReadiness);
    TestEqual(
        TEXT("Checkpoint carries fixed Meridian faction"),
        ContainerFaction, echoes::sim::Faction::MeridianCompact);
    TestTrue(TEXT("Checkpoint carries nonzero fixed setup identity"), SetupIdentity != 0);

    FString ValidationError;
    TestFalse(
        TEXT("Wrong training setup identity is rejected"),
        Bridge->ValidateCheckpointFileOnDisk(
            TrainingSavePath, SetupIdentity ^ 1ULL, ValidationError));

    const FString CorruptPath = FPaths::Combine(
        FEchoesCampaignProgressStore::GetSaveGameDirectory(),
        TEXT("EchoesTrainingReadinessCorrupt.bin"));
    TArray<uint8> CorruptBytes = TrainingSaveBytes;
    CorruptBytes.Last() ^= 0x80U;
    TestTrue(
        TEXT("Corrupt checkpoint fixture writes"),
        FFileHelper::SaveArrayToFile(CorruptBytes, *CorruptPath));
    TestFalse(
        TEXT("Corrupt training checkpoint is rejected"),
        Bridge->LoadScenarioFromPath(CorruptPath, Feedback));
    IFileManager::Get().Delete(*CorruptPath, false, true, true);

    Bridge->StopPrototypeScenario();
    if (!TestTrue(
            TEXT("Skirmish selection is accepted for wrong-operation admission"),
            Bridge->SelectOperationMode(
                EEchoesOperationMode::Skirmish, Feedback)) ||
        !TestTrue(
            TEXT("Skirmish authority is live for wrong-operation admission"),
            Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    TestFalse(
        TEXT("Training checkpoint cannot masquerade as skirmish"),
        Bridge->LoadScenarioFromPath(TrainingSavePath, Feedback));
    if (!TestTrue(
            TEXT("Training operation can be restored for recovery"),
            Bridge->SelectOperationMode(
                EEchoesOperationMode::TrainingReadiness, Feedback)) ||
        !TestTrue(
            TEXT("Training operation has live authority before recovery"),
            Bridge->IsScenarioReady()) ||
        !TestTrue(
            TEXT("Training checkpoint resumes"),
            Bridge->LoadScenarioFromPath(TrainingSavePath, Feedback)))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    Simulation = Bridge->GetSimulation();
    if (!TestNotNull(
            TEXT("Resumed training owns an authoritative simulation"),
            Simulation))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    Bridge->QueueOpponentCommands();
    TestFalse(
        TEXT("Resumed training remains passive"),
        Simulation != nullptr && HasCommandFrom(
            Simulation->PendingCommands(),
            UEchoesSimulationSubsystem::OpponentPlayerId));

    uint32 OpponentCore = 0;
    echoes::sim::Vec2 OpponentCorePosition;
    TArray<uint32> Attackers;
    for (const echoes::sim::Entity& Entity : Simulation->Entities())
    {
        if (Entity.owner == UEchoesSimulationSubsystem::OpponentPlayerId &&
            Entity.type == echoes::sim::EntityType::CommandCore)
        {
            OpponentCore = Entity.id;
            OpponentCorePosition = Entity.position;
        }
        if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            Entity.attackDamage > 0 && Entity.movementPerTickRaw > 0)
        {
            Attackers.Add(Entity.id);
        }
    }
    TestTrue(TEXT("Training retains an opponent Core"), OpponentCore != 0);
    TestTrue(TEXT("Training retains mobile combat units"), !Attackers.IsEmpty());
    for (const uint32 Attacker : Attackers)
    {
        TestTrue(
            TEXT("Readiness scouting attack-move queues"),
            Bridge->IssueCommand(
                echoes::sim::CommandType::AttackMove,
                Attacker,
                0,
                Bridge->SimToWorld(OpponentCorePosition),
                echoes::sim::FutureWellChoice::Dormant,
                Feedback));
    }
    for (int32 Tick = 0;
         Tick < 20000 && OpponentCore != 0 &&
         !Simulation->IsEntityVisibleTo(
             UEchoesSimulationSubsystem::LocalPlayerId, OpponentCore) &&
         Simulation->Outcome() == echoes::sim::MatchOutcome::Ongoing;
         ++Tick)
    {
        Bridge->QueueOpponentCommands();
        Bridge->Simulation->Step();
    }
    const bool bOpponentCoreVisible = OpponentCore != 0 &&
        Simulation->IsEntityVisibleTo(
            UEchoesSimulationSubsystem::LocalPlayerId, OpponentCore);
    TestTrue(
        TEXT("Readiness attack-move establishes legal Core visibility"),
        bOpponentCoreVisible);
    // Keep the ordinary attack-move active until the force reaches and starts
    // damaging the passive Core. A focus-fire command intentionally has a
    // bounded chase radius, so issuing it at the edge of vision would stop the
    // approach before weapon range.
    const int32 OpponentCoreInitialHitPoints =
        Simulation->FindEntity(OpponentCore) != nullptr
            ? Simulation->FindEntity(OpponentCore)->hitPoints
            : 0;
    for (int32 Tick = 0;
         Tick < 20000 &&
         Simulation->Outcome() == echoes::sim::MatchOutcome::Ongoing &&
         Simulation->FindEntity(OpponentCore) != nullptr &&
         Simulation->FindEntity(OpponentCore)->hitPoints >=
             OpponentCoreInitialHitPoints;
         ++Tick)
    {
        Bridge->QueueOpponentCommands();
        Bridge->Simulation->Step();
    }
    const echoes::sim::Entity* ApproachedCore =
        Simulation->FindEntity(OpponentCore);
    if (bOpponentCoreVisible && ApproachedCore != nullptr &&
        ApproachedCore->hitPoints > 0)
    {
        for (const uint32 Attacker : Attackers)
        {
            const echoes::sim::Entity* SurvivingAttacker =
                Simulation->FindEntity(Attacker);
            if (SurvivingAttacker == nullptr ||
                SurvivingAttacker->hitPoints <= 0)
            {
                continue;
            }
            TestTrue(
                TEXT("Visible readiness Core attack queues"),
                Bridge->IssueCommand(
                    echoes::sim::CommandType::Attack,
                    Attacker,
                    OpponentCore,
                    Bridge->SimToWorld(OpponentCorePosition),
                    echoes::sim::FutureWellChoice::Dormant,
                    Feedback));
        }
    }
    for (int32 Tick = 0;
         Tick < 20000 &&
         Simulation->Outcome() == echoes::sim::MatchOutcome::Ongoing;
         ++Tick)
    {
        Bridge->QueueOpponentCommands();
        Bridge->Simulation->Step();
    }
    const echoes::sim::Entity* CoreAfterReadinessRoute =
        Simulation->FindEntity(OpponentCore);
    int32 SurvivingReadinessAttackers = 0;
    for (const uint32 Attacker : Attackers)
    {
        const echoes::sim::Entity* Entity = Simulation->FindEntity(Attacker);
        SurvivingReadinessAttackers +=
            Entity != nullptr && Entity->hitPoints > 0 ? 1 : 0;
    }
    AddInfo(FString::Printf(
        TEXT("Training Corefall route outcome=%u core_hp=%d surviving_attackers=%d tick=%llu"),
        static_cast<uint8>(Simulation->Outcome()),
        CoreAfterReadinessRoute != nullptr ? CoreAfterReadinessRoute->hitPoints : 0,
        SurvivingReadinessAttackers,
        static_cast<unsigned long long>(Simulation->CurrentTick())));
    TestEqual(
        TEXT("Passive training retains a real player-won Corefall"),
        Simulation->Outcome(), echoes::sim::MatchOutcome::Player0Victory);
    TestTrue(
        TEXT("Training Corefall does not advance the campaign ledger"),
        Bridge->GetCampaignProgress().Decisions == LedgerBefore);

    std::string ReplayError;
    const echoes::sim::ReplayRecord TrainingReplay =
        Simulation->ExportReplay(&ReplayError);
    FEchoesReplayMetadata TrainingMetadata;
    TrainingMetadata.ReplayId = TEXT("training-readiness-test");
    TrainingMetadata.MapId = TEXT("glass-scar-evacuation-margin");
    TrainingMetadata.OperationId = TEXT("training-readiness");
    TrainingMetadata.BuildIdentity = DigestHex(
        echoes::network::BuildCompatibilityManifest(Simulation).buildIdSha256);
    TrainingMetadata.RecordedUtc = FDateTime::UtcNow();
    TrainingMetadata.OperationType = EEchoesReplayOperationType::Training;
    TrainingMetadata.bOperationCompleted = true;
    TrainingMetadata.OperationResult =
        EEchoesReplayOperationResult::TrainingSuccess;
    TrainingMetadata.OutcomeCause =
        EEchoesReplayOutcomeCause::CommandCoreLoss;
    TrainingMetadata.OutcomeReasonId =
        TEXT("training_opponent_core_destroyed");

    FEchoesReplayEnvelope TrainingEnvelope;
    TestTrue(
        TEXT("Training replay finalizes with explicit identity"),
        FEchoesMatchReplayStore::FinalizeEnvelope(
            TrainingMetadata, TrainingReplay,
            TrainingEnvelope, Feedback));
    TestEqual(
        TEXT("Final replay remains Training"),
        TrainingEnvelope.Metadata.OperationType,
        EEchoesReplayOperationType::Training);
    TestTrue(
        TEXT("Training replay carries no campaign record"),
        TrainingEnvelope.Metadata.IrreversibleRecordId.IsEmpty());
    const FEchoesReplayEnvelope CanonicalTrainingEnvelope = TrainingEnvelope;

    FEchoesReplayMetadata MasqueradingCampaign = TrainingMetadata;
    MasqueradingCampaign.OperationType = EEchoesReplayOperationType::Campaign;
    MasqueradingCampaign.OperationResult =
        EEchoesReplayOperationResult::CampaignSuccess;
    MasqueradingCampaign.OutcomeCause =
        EEchoesReplayOutcomeCause::CampaignObjectivesComplete;
    TestFalse(
        TEXT("Training replay cannot masquerade as campaign"),
        FEchoesMatchReplayStore::FinalizeEnvelope(
            MasqueradingCampaign, TrainingReplay,
            TrainingEnvelope, Feedback));
    FEchoesReplayMetadata MasqueradingSkirmish = TrainingMetadata;
    MasqueradingSkirmish.OperationType = EEchoesReplayOperationType::Skirmish;
    MasqueradingSkirmish.OperationResult =
        EEchoesReplayOperationResult::Player0Victory;
    MasqueradingSkirmish.OutcomeCause =
        EEchoesReplayOutcomeCause::CommandCoreLoss;
    TestFalse(
        TEXT("Training replay cannot masquerade as skirmish"),
        FEchoesMatchReplayStore::FinalizeEnvelope(
            MasqueradingSkirmish, TrainingReplay,
            TrainingEnvelope, Feedback));

    TArray<uint8> EncodedTraining;
    TestTrue(
        TEXT("Training replay encodes"),
        FEchoesMatchReplayStore::Encode(
            CanonicalTrainingEnvelope, EncodedTraining, Feedback));
    if (!EncodedTraining.IsEmpty())
    {
        EncodedTraining.Last() ^= 0x40U;
    }
    TestFalse(
        TEXT("Corrupt training replay is rejected"),
        FEchoesMatchReplayStore::Decode(
            EncodedTraining, TrainingEnvelope, Feedback));

    const echoes::sim::ReplayRecord LegacyReplay =
        MakeLegacyCompatibleSkirmishReplay();
    FEchoesReplayMetadata LegacyMetadata;
    LegacyMetadata.ReplayId = TEXT("legacy-v4-skirmish-test");
    LegacyMetadata.MapId = TEXT("glass-scar");
    LegacyMetadata.OperationId = TEXT("skirmish");
    LegacyMetadata.BuildIdentity = TEXT("test-build");
    LegacyMetadata.RecordedUtc = FDateTime::UtcNow();
    LegacyMetadata.OperationType = EEchoesReplayOperationType::Skirmish;
    LegacyMetadata.bOperationCompleted = true;
    FEchoesReplayEnvelope LegacyEnvelope;
    TestTrue(
        TEXT("Legacy compatibility fixture finalizes"),
        FEchoesMatchReplayStore::FinalizeEnvelope(
            LegacyMetadata, LegacyReplay, LegacyEnvelope, Feedback));
    TArray<uint8> LegacyBytes;
    TestTrue(
        TEXT("Legacy compatibility fixture encodes"),
        FEchoesMatchReplayStore::Encode(
            LegacyEnvelope, LegacyBytes, Feedback));
    if (LegacyBytes.Num() >= 14)
    {
        LegacyBytes[8] = 4;
        LegacyBytes[9] = 0;
        RewriteChecksum(LegacyBytes);
    }
    TestTrue(
        TEXT("Previously written v4 replay remains readable"),
        FEchoesMatchReplayStore::Decode(
            LegacyBytes, LegacyEnvelope, Feedback));

    Bridge->StopPrototypeScenario();
    TestTrue(
        TEXT("Ordinary M01 can still start"),
        Bridge->SelectOperationMode(
            EEchoesOperationMode::CampaignPrologue, Feedback) &&
            Bridge->StartPrototypeScenario());
    Bridge->QueueOpponentCommands();
    TestTrue(
        TEXT("Ordinary M01 opponent AI remains active"),
        HasCommandFrom(
            Bridge->Simulation->PendingCommands(),
            UEchoesSimulationSubsystem::OpponentPlayerId));

    Bridge->StopPrototypeScenario();
    TestTrue(
        TEXT("Ordinary skirmish can still start"),
        Bridge->SelectOperationMode(
            EEchoesOperationMode::Skirmish, Feedback) &&
            Bridge->StartPrototypeScenario());
    Bridge->QueueOpponentCommands();
    TestTrue(
        TEXT("Ordinary skirmish opponent AI remains active"),
        HasCommandFrom(
            Bridge->Simulation->PendingCommands(),
            UEchoesSimulationSubsystem::OpponentPlayerId));

    Bridge->StopPrototypeScenario();
    // Exercise the controller commit boundary after a deliberate skip. This is an
    // integration regression, not evidence that a player performed the lesson.
    auto* TutorialController = World->SpawnActor<AEchoesPlayerController>();
    if (!TestNotNull(TEXT("Skip progression controller exists"), TutorialController)) return false;
    if (!TestTrue(TEXT("Skip progression training prerequisites start"),
        Bridge->SelectOperationMode(EEchoesOperationMode::TrainingReadiness, Feedback) &&
        Bridge->StartPrototypeScenario()))
    {
        TutorialController->Destroy();
        return false;
    }
    TutorialController->bTutorialOperationAuthorized = true;
    TutorialController->bPlayerProfileAvailable = true;
    TutorialController->PlayerProfile.TutorialVerifiedMask = 0;
    TutorialController->TutorialPresentedLessonBit = 1;
    TutorialController->OpenTutorialSkipModal();
    TutorialController->SkipTutorialCurrentStep();
    TestEqual(TEXT("Survey skip records only Survey"), TutorialController->GetTutorialSkippedMask(), uint16(1));
    TutorialController->TutorialPresentedLessonBit = 2;
    TestTrue(TEXT("Genuine Roster completion after Survey skip is accepted"),
        TutorialController->CommitTutorialLesson(2, TEXT("roster")));
    TestFalse(TEXT("Duplicate genuine completion after a skip is rejected"),
        TutorialController->CommitTutorialLesson(2, TEXT("roster")));
    TutorialController->TutorialPresentedLessonBit = 4;
    TestTrue(TEXT("Roster completion after a skip unlocks the next genuine lesson"),
        TutorialController->CommitTutorialLesson(4, TEXT("muster")));
    TestEqual(TEXT("Completed lessons are not falsely recorded as skipped"),
        TutorialController->GetTutorialSkippedMask(), uint16(1));
    TestEqual(TEXT("Skip route cannot forge durable mastery"),
        TutorialController->PlayerProfile.TutorialVerifiedMask, uint16(0));
    TestEqual(TEXT("Guidance progress includes skip and genuine subsequent completions"),
        TutorialController->GetTutorialProgressMask(), uint16(7));
    TestEqual(TEXT("Session completion records only genuinely completed lessons"),
        TutorialController->TutorialSessionVerifiedMask, uint16(6));
    TutorialController->ResetTutorialObservation();
    TestEqual(TEXT("Observer reset preserves current-session guidance progress"),
        TutorialController->GetTutorialProgressMask(), uint16(7));
    TutorialController->Destroy();
    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return true;
}

#endif
