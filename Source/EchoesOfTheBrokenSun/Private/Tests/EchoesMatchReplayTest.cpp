#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "EchoesMatchReplay.h"
#include "EchoesNetworkSession.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesSkirmishSetup.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"
#include "Engine/World.h"
#include "Misc/Crc.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "Tests/AutomationCommon.h"

namespace
{
constexpr int32 ReplayPrefixTerminalMarkerOffset = 30;
constexpr int32 ReplayPrefixSnapshotLengthOffset = 31;

void ApplyReplayTestTerrain(
    echoes::sim::Simulation& Simulation,
    EEchoesSkirmishMapPreset MapPreset)
{
    for (int32 Y = 0; Y < FEchoesSkirmishSetupModel::MapHeightTiles; ++Y)
    {
        for (int32 X = 0; X < FEchoesSkirmishSetupModel::MapWidthTiles; ++X)
        {
            if (FEchoesSkirmishSetupModel::IsBlockedTile(MapPreset, X, Y))
            {
                (void)Simulation.SetTerrainTile(
                    X, Y, echoes::sim::Terrain::Blocked);
            }
        }
    }
}

void RewriteReplayTestChecksum(TArray<uint8>& Bytes)
{
    constexpr int32 ChecksumBytes = 4;
    if (Bytes.Num() < ChecksumBytes)
    {
        return;
    }
    const int32 ChecksumOffset = Bytes.Num() - ChecksumBytes;
    const uint32 Checksum = FCrc::MemCrc32(Bytes.GetData(), ChecksumOffset);
    for (int32 Index = 0; Index < ChecksumBytes; ++Index)
    {
        Bytes[ChecksumOffset + Index] =
            static_cast<uint8>(Checksum >> (Index * 8));
    }
}

bool ReplaceReplayTestAscii(
    TArray<uint8>& Bytes,
    const ANSICHAR* Expected,
    const ANSICHAR* Replacement)
{
    const int32 Length = FCStringAnsi::Strlen(Expected);
    if (Length <= 0 || FCStringAnsi::Strlen(Replacement) != Length)
    {
        return false;
    }
    for (int32 Offset = 0; Offset + Length <= Bytes.Num(); ++Offset)
    {
        if (FMemory::Memcmp(Bytes.GetData() + Offset, Expected, Length) == 0)
        {
            FMemory::Memcpy(Bytes.GetData() + Offset, Replacement, Length);
            return true;
        }
    }
    return false;
}

bool ReplaceReplayTestU64(
    TArray<uint8>& Bytes,
    uint64 Expected,
    uint64 Replacement)
{
    constexpr int32 EncodedBytes = 8;
    for (int32 Offset = 0; Offset + EncodedBytes <= Bytes.Num(); ++Offset)
    {
        bool bMatches = true;
        for (int32 Index = 0; Index < EncodedBytes; ++Index)
        {
            if (Bytes[Offset + Index] !=
                static_cast<uint8>(Expected >> (Index * 8)))
            {
                bMatches = false;
                break;
            }
        }
        if (!bMatches) continue;
        for (int32 Index = 0; Index < EncodedBytes; ++Index)
        {
            Bytes[Offset + Index] =
                static_cast<uint8>(Replacement >> (Index * 8));
        }
        return true;
    }
    return false;
}

template <typename Digest>
FString ReplayTestDigestHex(const Digest& Value)
{
    FString Result;
    Result.Reserve(static_cast<int32>(Value.size() * 2));
    for (const uint8 Byte : Value)
    {
        Result += FString::Printf(TEXT("%02x"), Byte);
    }
    return Result;
}

struct FReplayTestFile final
{
    ~FReplayTestFile()
    {
        if (!Path.IsEmpty())
        {
            IFileManager::Get().Delete(*Path, false, true, true);
            IFileManager::Get().Delete(*(Path + TEXT(".tmp")), false, true, true);
        }
    }

    FString Path;
};

echoes::sim::ReplayRecord MakeTerminalReplay(
    bool bUseUninstalledRulesIdentity = false)
{
    using namespace echoes::sim;
    SimulationConfig Config{
        FEchoesSkirmishSetupModel::MapWidthTiles,
        FEchoesSkirmishSetupModel::MapHeightTiles,
        20,
        0x52554e54494d4552ULL};
    // Direct core fixtures bypass the authored content catalog. Bind them to
    // the installed pack while retaining the test-scoped combat override.
    Config.rules.contentSha256 =
        echoes::network::BuildCompatibilityManifest(nullptr).rulesPackSha256;
    auto& Soldier = Config.rules.archetypes
        [static_cast<size_t>(Faction::MeridianCompact)]
        [static_cast<size_t>(EntityType::Soldier)];
    Soldier.attackDamage = 5000;
    Soldier.attackRangeRaw = 3 * kFixedScale;
    Soldier.attackPeriodTicks = 1;
    if (bUseUninstalledRulesIdentity)
    {
        Config.rules.contentSha256[0] ^= 0xffU;
    }

    Simulation Simulation(Config);
    ApplyReplayTestTerrain(Simulation, EEchoesSkirmishMapPreset::GlassScar);
    Simulation.AddPlayer(0, Faction::MeridianCompact, {1000, 1000});
    Simulation.AddPlayer(1, Faction::KharuunAssemblies, {1000, 1000});
    Simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::CommandCore,
        Vec2::FromTiles(2, 2));
    const EntityId EnemyCore = Simulation.SpawnEntity(
        1, Faction::KharuunAssemblies, EntityType::CommandCore,
        Vec2::FromTiles(18, 18));
    const EntityId Attacker = Simulation.SpawnEntity(
        0, Faction::MeridianCompact, EntityType::Soldier,
        Vec2::FromTiles(17, 18));
    Simulation.CaptureReplayBaseline();
    Command Attack;
    Attack.executeTick = 10;
    Attack.player = 0;
    Attack.sequence = 1;
    Attack.type = CommandType::Attack;
    Attack.actor = Attacker;
    Attack.target = EnemyCore;
    Simulation.QueueCommand(Attack);
    Simulation.Step(12);
    return Simulation.ExportReplay();
}

echoes::sim::ReplayRecord MakeConcessionReplay()
{
    using namespace echoes::sim;
    SimulationConfig Config{
        FEchoesSkirmishSetupModel::MapWidthTiles,
        FEchoesSkirmishSetupModel::MapHeightTiles,
        20,
        0x434f4e43454445ULL};
    // Direct core fixtures bypass the authored content catalog.
    Config.rules.contentSha256 =
        echoes::network::BuildCompatibilityManifest(nullptr).rulesPackSha256;
    Simulation Simulation(Config);
    ApplyReplayTestTerrain(Simulation, EEchoesSkirmishMapPreset::GlassScar);
    Simulation.AddPlayer(0, Faction::MeridianCompact, {1000, 1000});
    Simulation.AddPlayer(1, Faction::KharuunAssemblies, {1000, 1000});
    Simulation.SpawnEntity(0, Faction::MeridianCompact,
        EntityType::CommandCore, Vec2::FromTiles(2, 2));
    Simulation.SpawnEntity(1, Faction::KharuunAssemblies,
        EntityType::CommandCore, Vec2::FromTiles(18, 18));
    Simulation.CaptureReplayBaseline();
    Simulation.Step(7);
    Simulation.ForfeitPlayer(0);
    return Simulation.ExportReplay();
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesMatchReplayTest,
    "Echoes.Runtime.Replay.AuthorityStorageAndTransport",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesMatchReplayTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    FEchoesScopedTestSaveEnvironment TestSaveEnvironment(*this);
    if (!TestSaveEnvironment.IsReady())
    {
        return false;
    }

    const echoes::sim::ReplayRecord Replay = MakeTerminalReplay();
    FEchoesReplayMetadata Metadata;
    Metadata.ReplayId = FString::Printf(
        TEXT("p2-test-%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits));
    Metadata.MapId = TEXT("glass-scar");
    Metadata.OperationId = TEXT("skirmish");
    Metadata.BuildIdentity = TEXT("automation-local-dirty");
    Metadata.RulesIdentity = TEXT("automation-rules-local");
    Metadata.RecordedUtc = FDateTime(2026, 9, 5, 12, 0, 0);
    Metadata.OperationType = EEchoesReplayOperationType::Skirmish;
    Metadata.bOperationCompleted = true;

    FString Error;
    FEchoesReplayEnvelope Envelope;
    if (!TestTrue(
            TEXT("A terminal replay produces authoritative metadata and report"),
            FEchoesMatchReplayStore::FinalizeEnvelope(
                Metadata, Replay, Envelope, Error)))
    {
        AddError(Error);
        return false;
    }
    TestEqual(TEXT("Replay coverage starts at the captured baseline"),
              Envelope.Metadata.CoverageStartTick, 0ULL);
    TestEqual(TEXT("Replay duration is derived from ticks"),
              Envelope.Metadata.DurationTicks, 12ULL);
    TestTrue(TEXT("The terminal outcome is preserved"),
             Envelope.Metadata.Outcome ==
                 echoes::sim::MatchOutcome::Player0Victory);
    TestEqual(TEXT("The report checksum binds replay metadata"),
              Envelope.Metadata.FinalChecksum, Replay.finalChecksum);
    TestEqual(TEXT("Both active faction seats are listed"),
              Envelope.Metadata.PlayerFactions.Num(), 2);
    const FString InstalledRulesIdentity = ReplayTestDigestHex(
        echoes::network::BuildCompatibilityManifest(nullptr).rulesPackSha256);
    TestEqual(
        TEXT("Replay rules identity is re-derived from its baseline"),
        Envelope.Metadata.RulesIdentity,
        InstalledRulesIdentity);
    TestTrue(
        TEXT("Serialized-baseline identity remains distinct from rules identity"),
        !Envelope.Metadata.ContentIdentity.IsEmpty() &&
            Envelope.Metadata.ContentIdentity !=
                Envelope.Metadata.RulesIdentity);

    const echoes::sim::ReplayRecord UninstalledRulesReplay =
        MakeTerminalReplay(true);
    FEchoesReplayEnvelope UninstalledRulesEnvelope;
    TestFalse(
        TEXT("A replay from an uninstalled rules pack is not publishable"),
        FEchoesMatchReplayStore::FinalizeEnvelope(
            Metadata,
            UninstalledRulesReplay,
            UninstalledRulesEnvelope,
            Error));
    TestTrue(TEXT("Uninstalled replay rules rejection is attributable"),
             Error.Contains(TEXT("not installed")));

    int32 CancellationChecks = 0;
    FEchoesReplayEnvelope CancelledEnvelope = Envelope;
    TestFalse(
        TEXT("Cancellation during baseline parsing publishes no replay authority"),
        FEchoesMatchReplayStore::FinalizeEnvelope(
            Metadata,
            Replay,
            CancelledEnvelope,
            Error,
            [&CancellationChecks]
            {
                return ++CancellationChecks >= 8;
            }));
    TestTrue(TEXT("Cancelled finalization clears staged output"),
             CancelledEnvelope.Metadata.ReplayId.IsEmpty() &&
             CancelledEnvelope.Replay.initialSnapshot.empty());
    TestTrue(TEXT("Cancelled finalization is attributable"),
             Error.Contains(TEXT("CANCELLED")) ||
                 Error.Contains(TEXT("cancelled")));

    FEchoesReplayMetadata UnsupportedMapMetadata = Metadata;
    UnsupportedMapMetadata.MapId = TEXT("unknown-map");
    FEchoesReplayEnvelope RejectedEnvelope;
    TestFalse(
        TEXT("Replay finalization rejects an unknown skirmish map"),
        FEchoesMatchReplayStore::FinalizeEnvelope(
            UnsupportedMapMetadata, Replay, RejectedEnvelope, Error));
    TestTrue(TEXT("Unknown map rejection is attributable"),
             Error.Contains(TEXT("map identity")));
    FEchoesReplayMetadata RelabeledMapMetadata = Metadata;
    RelabeledMapMetadata.MapId = TEXT("crownfall-basin");
    TestFalse(
        TEXT("A canonical replay cannot be relabeled as another supported map"),
        FEchoesMatchReplayStore::FinalizeEnvelope(
            RelabeledMapMetadata, Replay, RejectedEnvelope, Error));
    TestTrue(TEXT("Supported-map relabel rejection identifies terrain binding"),
             Error.Contains(TEXT("baseline terrain")));
    FEchoesReplayMetadata MismatchedOperationMetadata = Metadata;
    MismatchedOperationMetadata.OperationType =
        EEchoesReplayOperationType::Campaign;
    MismatchedOperationMetadata.OperationResult =
        EEchoesReplayOperationResult::CampaignSuccess;
    MismatchedOperationMetadata.OutcomeCause =
        EEchoesReplayOutcomeCause::CampaignObjectivesComplete;
    MismatchedOperationMetadata.OutcomeReasonId = TEXT("campaign-test-outcome");
    TestFalse(
        TEXT("Replay finalization rejects a mismatched operation type"),
        FEchoesMatchReplayStore::FinalizeEnvelope(
            MismatchedOperationMetadata, Replay, RejectedEnvelope, Error));
    TestTrue(TEXT("Mismatched operation rejection is attributable"),
             Error.Contains(TEXT("campaign operation")));

    FEchoesReplayMetadata ContradictoryCampaignMetadata = Metadata;
    ContradictoryCampaignMetadata.OperationType =
        EEchoesReplayOperationType::Campaign;
    ContradictoryCampaignMetadata.OperationId =
        TEXT("m01-what-the-ledger-keeps");
    ContradictoryCampaignMetadata.MapId =
        TEXT("glass-scar-evacuation-margin");
    ContradictoryCampaignMetadata.OutcomeReasonId =
        TEXT("campaign-test-outcome");
    ContradictoryCampaignMetadata.OperationResult =
        EEchoesReplayOperationResult::CampaignSuccess;
    ContradictoryCampaignMetadata.OutcomeCause =
        EEchoesReplayOutcomeCause::CampaignFailurePredicate;
    TestFalse(
        TEXT("Campaign success cannot carry a failure-predicate cause"),
        FEchoesMatchReplayStore::FinalizeEnvelope(
            ContradictoryCampaignMetadata,
            Replay,
            RejectedEnvelope,
            Error));
    ContradictoryCampaignMetadata.OperationResult =
        EEchoesReplayOperationResult::CampaignFailure;
    ContradictoryCampaignMetadata.OutcomeCause =
        EEchoesReplayOutcomeCause::CampaignObjectivesComplete;
    TestFalse(
        TEXT("Campaign failure cannot carry an objectives-complete cause"),
        FEchoesMatchReplayStore::FinalizeEnvelope(
            ContradictoryCampaignMetadata,
            Replay,
            RejectedEnvelope,
            Error));
    TestTrue(TEXT("Contradictory campaign outcome rejection is attributable"),
             Error.Contains(TEXT("coherent completed outcome")));

    TArray<uint8> Encoded;
    if (!TestTrue(TEXT("Replay envelope encodes"),
                  FEchoesMatchReplayStore::Encode(
                      Envelope, Encoded, Error)))
    {
        AddError(Error);
        return false;
    }
    FEchoesReplayEnvelope Decoded;
    if (!TestTrue(TEXT("Replay envelope decodes and replays"),
                  FEchoesMatchReplayStore::Decode(
                      Encoded, Decoded, Error)))
    {
        AddError(Error);
        return false;
    }
    TestEqual(TEXT("Replay command count round-trips"),
              static_cast<int32>(Decoded.Replay.commands.size()), 1);
    TestEqual(TEXT("Decoded report reaches the same final tick"),
              Decoded.Report.finalTick, Replay.finalTick);

    const uint64 RecordedTicks =
        static_cast<uint64>(Metadata.RecordedUtc.GetTicks());
    const uint64 MaximumDateTicks =
        static_cast<uint64>(FDateTime::MaxValue().GetTicks());
    TArray<uint8> OutOfRangeDateBytes = Encoded;
    if (!TestTrue(
            TEXT("Out-of-range date fixture finds the recorded UTC field"),
            ReplaceReplayTestU64(
                OutOfRangeDateBytes, RecordedTicks, MaximumDateTicks + 1)))
    {
        return false;
    }
    RewriteReplayTestChecksum(OutOfRangeDateBytes);
    TestFalse(
        TEXT("Replay metadata rejects a date beyond FDateTime range"),
        FEchoesMatchReplayStore::Decode(
            OutOfRangeDateBytes, Decoded, Error));
    TestTrue(TEXT("Out-of-range replay date is attributable"),
             Error.Contains(TEXT("metadata is malformed")));
    TArray<uint8> MaximumUnsignedDateBytes = Encoded;
    if (!TestTrue(
            TEXT("Maximum-uint date fixture finds the recorded UTC field"),
            ReplaceReplayTestU64(
                MaximumUnsignedDateBytes,
                RecordedTicks,
                MAX_uint64)))
    {
        return false;
    }
    RewriteReplayTestChecksum(MaximumUnsignedDateBytes);
    TestFalse(
        TEXT("Replay metadata rejects the maximum unsigned timestamp"),
        FEchoesMatchReplayStore::Decode(
            MaximumUnsignedDateBytes, Decoded, Error));

    FEchoesReplayResultAuthority ResultAuthority;
    const uint64 GenerationA = ResultAuthority.BeginResult();
    const uint64 GenerationB = ResultAuthority.BeginResult();
    FEchoesReplayArchiveResult ResultB;
    ResultB.Generation = GenerationB;
    ResultB.bFinalized = true;
    ResultB.bSucceeded = true;
    ResultB.Envelope = Envelope;
    ResultB.Envelope.Metadata.ReplayId = TEXT("result-b");
    ResultB.Envelope.Metadata.FilePath = TEXT("result-b.echoesreplay");
    TestTrue(TEXT("The newest result publishes while an older archive is pending"),
             ResultAuthority.Publish(MoveTemp(ResultB)));
    FEchoesReplayArchiveResult DelayedResultA;
    DelayedResultA.Generation = GenerationA;
    DelayedResultA.bFinalized = true;
    DelayedResultA.bSucceeded = true;
    DelayedResultA.Envelope = Envelope;
    DelayedResultA.Envelope.Metadata.ReplayId = TEXT("result-a");
    TestFalse(TEXT("A delayed older archive cannot replace the current result"),
              ResultAuthority.Publish(MoveTemp(DelayedResultA)));
    TestTrue(TEXT("The completed replay remains bound to result B"),
             ResultAuthority.GetCompleted() != nullptr &&
             ResultAuthority.GetCompleted()->Metadata.ReplayId ==
                 TEXT("result-b"));

    const uint64 GenerationC = ResultAuthority.BeginResult();
    FEchoesReplayArchiveResult FailedStorageResult;
    FailedStorageResult.Generation = GenerationC;
    FailedStorageResult.bFinalized = true;
    FailedStorageResult.bSucceeded = false;
    FailedStorageResult.Error = TEXT("injected storage failure");
    FailedStorageResult.Envelope = Envelope;
    FailedStorageResult.Envelope.Metadata.ReplayId = TEXT("result-c");
    TestTrue(TEXT("A finalized result publishes despite disk failure"),
             ResultAuthority.Publish(MoveTemp(FailedStorageResult)));
    TestTrue(TEXT("Disk failure retains the current authoritative report"),
             ResultAuthority.GetState() ==
                 EEchoesReplayArchiveState::Failed &&
             ResultAuthority.GetCompleted() != nullptr &&
             ResultAuthority.GetCompleted()->Metadata.ReplayId ==
                 TEXT("result-c") &&
             ResultAuthority.GetCompleted()->Metadata.FilePath.IsEmpty());

    TArray<uint8> PrefixBytes;
    echoes::sim::ReplayRecord PrefixRoundTrip;
    if (!TestTrue(TEXT("Replay prefix encodes for mid-match snapshot attachment"),
                  FEchoesMatchReplayStore::EncodeReplayRecord(
                      Replay, PrefixBytes, Error)))
    {
        AddError(Error);
        return false;
    }
    if (!TestTrue(TEXT("Replay prefix decodes with authority validation"),
                  FEchoesMatchReplayStore::DecodeReplayRecord(
                      PrefixBytes, PrefixRoundTrip, Error)))
    {
        AddError(Error);
        return false;
    }
    TestEqual(TEXT("Current replay version round-trips"),
              PrefixRoundTrip.version, echoes::sim::kReplayVersion);
    TestTrue(TEXT("Replay-prefix commands round-trip exactly"),
             PrefixRoundTrip.commands == Replay.commands);
    TestEqual(TEXT("Replay-prefix checksum round-trips exactly"),
              PrefixRoundTrip.finalChecksum, Replay.finalChecksum);

    echoes::sim::ReplayRecord LegacyReplay = Replay;
    LegacyReplay.version = echoes::sim::kLegacyReplayVersion;
    LegacyReplay.forfeitingPlayer = echoes::sim::kNeutralPlayer;
    TArray<uint8> LegacyPrefixBytes;
    echoes::sim::ReplayRecord LegacyRoundTrip;
    if (!TestTrue(TEXT("Legacy v24 replay prefix remains encodable"),
                  FEchoesMatchReplayStore::EncodeReplayRecord(
                      LegacyReplay, LegacyPrefixBytes, Error)) ||
        !TestTrue(TEXT("Legacy v24 replay prefix remains decodable"),
                  FEchoesMatchReplayStore::DecodeReplayRecord(
                      LegacyPrefixBytes, LegacyRoundTrip, Error)))
    {
        AddError(Error);
        return false;
    }
    TestEqual(TEXT("Legacy replay version round-trips"),
              LegacyRoundTrip.version, echoes::sim::kLegacyReplayVersion);
    TestEqual(TEXT("Legacy replay has no forfeit marker"),
              LegacyRoundTrip.forfeitingPlayer,
              echoes::sim::kNeutralPlayer);

    TArray<uint8> MalformedLegacyPrefix = LegacyPrefixBytes;
    MalformedLegacyPrefix[ReplayPrefixTerminalMarkerOffset] = 0;
    RewriteReplayTestChecksum(MalformedLegacyPrefix);
    TestFalse(TEXT("Legacy v24 replay rejects a v25 forfeit marker"),
              FEchoesMatchReplayStore::DecodeReplayRecord(
                  MalformedLegacyPrefix, LegacyRoundTrip, Error));
    TestTrue(TEXT("Malformed legacy marker has an authority reason"),
             Error.Contains(TEXT("forfeit marker")));

    TArray<uint8> MalformedCurrentPrefix = PrefixBytes;
    MalformedCurrentPrefix[ReplayPrefixSnapshotLengthOffset] ^= 0x01;
    RewriteReplayTestChecksum(MalformedCurrentPrefix);
    TestFalse(TEXT("Current v25 replay rejects malformed payload lengths"),
              FEchoesMatchReplayStore::DecodeReplayRecord(
                  MalformedCurrentPrefix, PrefixRoundTrip, Error));
    TestTrue(TEXT("Malformed current payload has a length reason"),
             Error.Contains(TEXT("payload lengths")));

    const TArray<uint8> OpaqueCheckpoint{1, 3, 3, 7};
    TArray<uint8> BoundCheckpoint;
    TArray<uint8> ValidatedReplayBytes;
    if (!TestTrue(TEXT("Checkpoint binds its replay prefix"),
                  FEchoesMatchReplayStore::BindCheckpointPayload(
                      OpaqueCheckpoint,
                      Replay,
                      BoundCheckpoint,
                      ValidatedReplayBytes,
                      Error)))
    {
        AddError(Error);
        return false;
    }
    TestTrue(TEXT("Checkpoint binding returns the exact validated prefix bytes"),
             ValidatedReplayBytes == PrefixBytes);
    TArray<uint8> ExtractedCheckpoint;
    echoes::sim::ReplayRecord ExtractedPrefix;
    if (!TestTrue(TEXT("Bound checkpoint is recognized"),
                  FEchoesMatchReplayStore::ExtractCheckpointPayload(
                      BoundCheckpoint, ExtractedCheckpoint, ExtractedPrefix,
                      Error) == EEchoesCheckpointReplayBindingRead::Bound))
    {
        AddError(Error);
        return false;
    }
    TestTrue(TEXT("Opaque checkpoint round-trips unchanged"),
             ExtractedCheckpoint == OpaqueCheckpoint);
    TestEqual(TEXT("Bound replay checksum round-trips"),
              ExtractedPrefix.finalChecksum, Replay.finalChecksum);
    TestTrue(
        TEXT("Trusted generated checkpoint extraction accepts its exact replay proof"),
        FEchoesMatchReplayStore::ExtractGeneratedCheckpointPayload(
            BoundCheckpoint,
            ValidatedReplayBytes,
            ExtractedCheckpoint,
            Error));
    TestTrue(TEXT("Trusted generated extraction preserves the opaque checkpoint"),
             ExtractedCheckpoint == OpaqueCheckpoint);
    constexpr int32 CheckpointReplayHeaderBytes = 8 + 2 + 4 + 4;
    TArray<uint8> TamperedBoundCheckpoint = BoundCheckpoint;
    const int32 EmbeddedReplayMutationOffset =
        CheckpointReplayHeaderBytes + OpaqueCheckpoint.Num() +
        ValidatedReplayBytes.Num() / 2;
    if (!TestTrue(TEXT("Replay proof mismatch fixture has embedded payload data"),
                  TamperedBoundCheckpoint.IsValidIndex(
                      EmbeddedReplayMutationOffset)))
    {
        return false;
    }
    TamperedBoundCheckpoint[EmbeddedReplayMutationOffset] ^= 0x01;
    RewriteReplayTestChecksum(TamperedBoundCheckpoint);
    TestFalse(
        TEXT("Trusted generated extraction rejects CRC-valid modified replay bytes"),
        FEchoesMatchReplayStore::ExtractGeneratedCheckpointPayload(
            TamperedBoundCheckpoint,
            ValidatedReplayBytes,
            ExtractedCheckpoint,
            Error));
    TestTrue(TEXT("Replay proof mismatch is attributable and publishes no payload"),
             Error.Contains(TEXT("does not match")) &&
                 ExtractedCheckpoint.IsEmpty());
    TestTrue(TEXT("Legacy checkpoint remains loadable and explicitly unbound"),
             FEchoesMatchReplayStore::ExtractCheckpointPayload(
                 OpaqueCheckpoint, ExtractedCheckpoint, ExtractedPrefix,
                 Error) == EEchoesCheckpointReplayBindingRead::LegacyUnbound &&
                 ExtractedCheckpoint == OpaqueCheckpoint);
    if (!TestTrue(TEXT("Bound checkpoint corruption test has payload data"),
                  !BoundCheckpoint.IsEmpty()))
    {
        return false;
    }
    BoundCheckpoint.Last() ^= 0x80;
    TestTrue(TEXT("Corrupt bound checkpoint fails closed"),
             FEchoesMatchReplayStore::ExtractCheckpointPayload(
                 BoundCheckpoint, ExtractedCheckpoint, ExtractedPrefix,
                 Error) == EEchoesCheckpointReplayBindingRead::Invalid);

    TArray<uint8> Corrupt = Encoded;
    Corrupt[Corrupt.Num() / 2] ^= 0x40;
    TestFalse(TEXT("Corrupt replay envelope fails closed"),
              FEchoesMatchReplayStore::Decode(Corrupt, Decoded, Error));
    TestTrue(TEXT("Corruption returns an attributable checksum reason"),
             Error.Contains(TEXT("checksum")));

    TArray<uint8> ForgedRulesBytes = Encoded;
    const FString ForgedRulesIdentity =
        Envelope.Metadata.RulesIdentity == FString::ChrN(64, TEXT('f'))
            ? FString::ChrN(64, TEXT('e'))
            : FString::ChrN(64, TEXT('f'));
    const FTCHARToUTF8 RecordedRulesUtf8(
        *Envelope.Metadata.RulesIdentity);
    const FTCHARToUTF8 ForgedRulesUtf8(*ForgedRulesIdentity);
    if (!TestTrue(
            TEXT("Rules-forgery fixture rewrites the recorded digest"),
            ReplaceReplayTestAscii(
                ForgedRulesBytes,
                RecordedRulesUtf8.Get(),
                ForgedRulesUtf8.Get())))
    {
        return false;
    }
    RewriteReplayTestChecksum(ForgedRulesBytes);
    TestFalse(
        TEXT("CRC-valid rules metadata forgery fails baseline derivation"),
        FEchoesMatchReplayStore::Decode(
            ForgedRulesBytes, Decoded, Error));
    TestTrue(TEXT("Rules metadata forgery is attributable"),
             Error.Contains(TEXT("metadata does not match")));

    const FString Directory = FEchoesMatchReplayStore::GetReplayDirectory();
    FReplayTestFile Stored;
    if (!TestTrue(TEXT("Replay writes transactionally"),
                  FEchoesMatchReplayStore::SaveAtomic(
                      Directory, Envelope, Stored.Path, Error)))
    {
        AddError(Error);
        return false;
    }
    FEchoesReplayEnvelope Loaded;
    TestTrue(TEXT("Stored replay loads and validates"),
             FEchoesMatchReplayStore::Load(Stored.Path, Loaded, Error));
    FEchoesReplayBrowserFilter Filter;
    Filter.MapId = Metadata.MapId;
    TArray<FString> BrowserErrors;
    const TArray<FEchoesReplayMetadata> Listed =
        FEchoesMatchReplayStore::ListMetadata(
            Directory, Filter, BrowserErrors);
    TestTrue(TEXT("Replay browser scan has no corrupt entry"),
             BrowserErrors.IsEmpty());
    TestTrue(TEXT("Map filtering retains the written replay"),
             Listed.ContainsByPredicate([&](const FEchoesReplayMetadata& Item)
             {
                 return Item.ReplayId == Metadata.ReplayId;
             }));
    TArray<FString> CancelledBrowserErrors;
    const TArray<FEchoesReplayMetadata> CancelledBrowserResults =
        FEchoesMatchReplayStore::ListMetadata(
            Directory,
            Filter,
            CancelledBrowserErrors,
            [] { return true; });
    TestTrue(
        TEXT("Cancelled browser validation publishes no partial metadata"),
        CancelledBrowserResults.IsEmpty() &&
            CancelledBrowserErrors.ContainsByPredicate(
                [](const FString& Item)
                {
                    return Item.Contains(TEXT("REPLAY_BROWSER_CANCELLED"));
                }));

    // The CRC is an accidental-corruption check, not authority. A caller can
    // rewrite metadata and recompute it, so browser admission must perform the
    // full semantic replay decode before advertising an entry.
    TArray<uint8> ForgedMapBytes = Encoded;
    if (!TestTrue(
            TEXT("CRC-valid metadata-forgery fixture rewrites one map field"),
            ReplaceReplayTestAscii(
                ForgedMapBytes, "glass-scar", "bogus-scar")))
    {
        return false;
    }
    RewriteReplayTestChecksum(ForgedMapBytes);
    FReplayTestFile ForgedMapFile;
    ForgedMapFile.Path = FPaths::Combine(
        Directory,
        FString::Printf(
            TEXT("forged-map-%s.echoesreplay"),
            *FGuid::NewGuid().ToString(EGuidFormats::Digits)));
    if (!TestTrue(
            TEXT("CRC-valid metadata-forgery fixture is written"),
            FFileHelper::SaveArrayToFile(
                ForgedMapBytes, *ForgedMapFile.Path)))
    {
        return false;
    }
    FEchoesReplayBrowserFilter ForgedMapFilter;
    ForgedMapFilter.MapId = TEXT("bogus-scar");
    TArray<FString> ForgedMapErrors;
    const TArray<FEchoesReplayMetadata> ForgedMapResults =
        FEchoesMatchReplayStore::ListMetadata(
            Directory, ForgedMapFilter, ForgedMapErrors);
    TestTrue(
        TEXT("Browser omits CRC-valid semantically invalid metadata"),
        ForgedMapResults.IsEmpty() &&
            ForgedMapErrors.ContainsByPredicate([](const FString& Item)
            {
                return Item.Contains(TEXT("map identity"));
            }));

    constexpr int64 ReplayFileBound = 256LL * 1024LL * 1024LL;
    FReplayTestFile OversizeFile;
    OversizeFile.Path = FPaths::Combine(
        Directory,
        FString::Printf(
            TEXT("oversize-%s.echoesreplay"),
            *FGuid::NewGuid().ToString(EGuidFormats::Digits)));
    IPlatformFile& PlatformFile =
        FPlatformFileManager::Get().GetPlatformFile();
    TUniquePtr<IFileHandle> OversizeWriter(
        PlatformFile.OpenWrite(*OversizeFile.Path));
    uint8 LastByte = 0;
    const bool bOversizeFixtureWritten = OversizeWriter &&
        OversizeWriter->Seek(ReplayFileBound) &&
        OversizeWriter->Write(&LastByte, 1);
    OversizeWriter.Reset();
    if (!TestTrue(
            TEXT("Oversize replay fixture is created without buffering it"),
            bOversizeFixtureWritten))
    {
        return false;
    }
    FEchoesReplayEnvelope OversizeEnvelope;
    TestFalse(
        TEXT("Replay load refuses an oversize file before allocation"),
        FEchoesMatchReplayStore::Load(
            OversizeFile.Path, OversizeEnvelope, Error));
    TestTrue(TEXT("Oversize replay rejection is attributable"),
             Error.Contains(TEXT("size is invalid")));

    FEchoesReplayPlaybackSession Playback;
    if (!TestTrue(TEXT("Validated replay initializes paused at baseline"),
                  Playback.Initialize(
                      Envelope,
                      Envelope.Metadata.BuildIdentity,
                      Envelope.Metadata.RulesIdentity,
                      Envelope.Metadata.ContentIdentity,
                      Error)))
    {
        AddError(Error);
        return false;
    }
    FEchoesReplayPlaybackSession Mismatch;
    TestFalse(TEXT("Playback refuses a different build identity"),
              Mismatch.Initialize(
                  Envelope,
                  TEXT("another-build"),
                  Envelope.Metadata.RulesIdentity,
                  Envelope.Metadata.ContentIdentity,
                  Error));
    TestFalse(TEXT("Playback refuses a different rules identity"),
              Mismatch.Initialize(
                  Envelope,
                  Envelope.Metadata.BuildIdentity,
                  TEXT("another-rules-pack"),
                  Envelope.Metadata.ContentIdentity,
                  Error));
    TestFalse(TEXT("Playback refuses a different content identity"),
              Mismatch.Initialize(
                  Envelope,
                  Envelope.Metadata.BuildIdentity,
                  Envelope.Metadata.RulesIdentity,
                  TEXT("another-content-identity"),
                  Error));
    TestTrue(TEXT("Playback starts paused"), Playback.IsPaused());
    TestEqual(TEXT("Playback starts at baseline tick"),
              Playback.GetCurrentTick(), 0ULL);
    TestTrue(
        TEXT("Playback keeps future recorded input outside the staged simulation"),
        Playback.GetSimulation() != nullptr &&
            Playback.GetSimulation()->PendingCommands().empty());
    TestFalse(TEXT("Cancelled seek does not publish rebuilt playback state"),
              Playback.Seek(Replay.finalTick, Error, [] { return true; }));
    TestEqual(TEXT("Cancelled seek preserves the prior playback tick"),
              Playback.GetCurrentTick(), 0ULL);
    TestTrue(TEXT("Cancelled seek reports its authority boundary"),
             Error.Contains(TEXT("REPLAY_VALIDATION_CANCELLED")));
    int32 SuccessfulFinalSeekChecks = 0;
    TestTrue(
        TEXT("A measured seek reaches the final state for cancellation staging"),
        Playback.Seek(
            Replay.finalTick,
            Error,
            [&SuccessfulFinalSeekChecks]
            {
                ++SuccessfulFinalSeekChecks;
                return false;
            }));
    TestTrue(TEXT("Measured seek exercises the cancellation predicate"),
             SuccessfulFinalSeekChecks > 0);
    TestTrue(TEXT("Playback returns to baseline before the late-cancel check"),
             Playback.Seek(0, Error));
    const uint64 BeforeLateCancellationTick = Playback.GetCurrentTick();
    const uint64 BeforeLateCancellationChecksum =
        Playback.GetSimulation() != nullptr
            ? Playback.GetSimulation()->StateChecksum()
            : 0;
    int32 LateCancellationChecks = 0;
    TestFalse(
        TEXT("Cancellation at the final publication gate rejects the staged seek"),
        Playback.Seek(
            Replay.finalTick,
            Error,
            [&LateCancellationChecks, SuccessfulFinalSeekChecks]
            {
                return ++LateCancellationChecks >= SuccessfulFinalSeekChecks;
            }));
    TestTrue(TEXT("Late-cancelled seek is attributable"),
             Error.Contains(TEXT("REPLAY_VALIDATION_CANCELLED")));
    TestTrue(
        TEXT("Late-cancelled seek preserves the prior playback authority"),
        Playback.GetCurrentTick() == BeforeLateCancellationTick &&
            Playback.GetSimulation() != nullptr &&
            Playback.GetSimulation()->StateChecksum() ==
                BeforeLateCancellationChecksum);
    TestTrue(TEXT("Player two perspective is scoped and available"),
             Playback.SetPerspective(
                 EEchoesReplayPerspective::Player1, Error) &&
                 Playback.GetPlayerView().has_value());
    TestTrue(TEXT("Observer perspective intentionally has no PlayerView"),
             Playback.SetPerspective(
                 EEchoesReplayPerspective::OmniscientObserver, Error) &&
                 !Playback.GetPlayerView().has_value());
    Playback.SetSpeed(EEchoesReplaySpeed::Half);
    Playback.SetPaused(false);
    TestTrue(TEXT("First half-speed cadence holds the current tick"),
             Playback.AdvanceOneCadence(Error));
    TestEqual(TEXT("Half-speed phase one advances zero ticks"),
              Playback.GetCurrentTick(), 0ULL);
    TestTrue(TEXT("Second half-speed cadence advances one tick"),
             Playback.AdvanceOneCadence(Error));
    TestEqual(TEXT("Half-speed phase two advances one tick"),
              Playback.GetCurrentTick(), 1ULL);
    Playback.SetPaused(true);
    TestTrue(TEXT("Paused tick-step advances exactly one tick"),
             Playback.StepForward(Error));
    TestEqual(TEXT("Tick-step result"), Playback.GetCurrentTick(), 2ULL);
    TestTrue(TEXT("Absolute seek rebuilds the authoritative state"),
             Playback.Seek(Replay.finalTick, Error));
    TestEqual(TEXT("Seek reaches the final tick"),
              Playback.GetCurrentTick(), Replay.finalTick);
    TestTrue(TEXT("Seeked state has the recorded outcome"),
             Playback.GetSimulation() != nullptr &&
                 Playback.GetSimulation()->Outcome() ==
                     echoes::sim::MatchOutcome::Player0Victory);
    TestTrue(TEXT("Seeked state matches the recorded final checksum"),
             Playback.GetSimulation() != nullptr &&
                 Playback.GetSimulation()->StateChecksum() ==
                     Replay.finalChecksum);
    TestFalse(TEXT("Seek outside the recording fails closed"),
              Playback.Seek(Replay.finalTick + 1, Error));

    const echoes::sim::ReplayRecord Concession = MakeConcessionReplay();
    TestEqual(TEXT("Concession records the forfeiting player"),
              Concession.forfeitingPlayer, static_cast<uint8>(0));
    FEchoesReplayEnvelope ConcessionEnvelope;
    Metadata.ReplayId = FString::Printf(
        TEXT("p2-concede-%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits));
    TestTrue(TEXT("Concession replay finalizes authoritatively"),
             FEchoesMatchReplayStore::FinalizeEnvelope(
                 Metadata, Concession, ConcessionEnvelope, Error));
    TestTrue(TEXT("Concession cause remains typed"),
             ConcessionEnvelope.Metadata.OutcomeCause ==
                 EEchoesReplayOutcomeCause::PlayerForfeit &&
             ConcessionEnvelope.Report.outcomeCause ==
                 echoes::sim::MatchOutcomeCause::PlayerForfeit);
    TArray<uint8> ConcessionPrefix;
    echoes::sim::ReplayRecord ConcessionRoundTrip;
    if (!TestTrue(TEXT("Current v25 concession prefix encodes"),
                  FEchoesMatchReplayStore::EncodeReplayRecord(
                      Concession, ConcessionPrefix, Error)) ||
        !TestTrue(TEXT("Current v25 concession prefix decodes"),
                  FEchoesMatchReplayStore::DecodeReplayRecord(
                      ConcessionPrefix, ConcessionRoundTrip, Error)))
    {
        AddError(Error);
        return false;
    }
    TestEqual(TEXT("Concession forfeit marker round-trips"),
              ConcessionRoundTrip.forfeitingPlayer, static_cast<uint8>(0));

    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the replay bridge test world."));
        return false;
    }
    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge = World != nullptr
        ? World->GetSubsystem<UEchoesSimulationSubsystem>()
        : nullptr;
    if (!TestNotNull(TEXT("Replay bridge test owns a simulation subsystem"),
                     Bridge) ||
        !TestTrue(TEXT("Replay bridge live scenario starts"),
                  Bridge != nullptr && Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    const uint64 LiveTick = Bridge->GetSimulation()->CurrentTick();
    const uint64 LiveChecksum = Bridge->GetSimulation()->StateChecksum();
    TestFalse(
        TEXT("Runtime bridge refuses a CRC-valid unsupported map"),
        Bridge->BeginReplay(ForgedMapFile.Path, Error));
    TestTrue(
        TEXT("Rejected replay cannot mutate live or presentation authority"),
        !Bridge->IsReplayPlaybackActive() &&
            Bridge->GetSimulation()->CurrentTick() == LiveTick &&
            Bridge->GetSimulation()->StateChecksum() == LiveChecksum);
    const echoes::sim::net::CompatibilityManifest Compatibility =
        echoes::network::BuildCompatibilityManifest(Bridge->GetSimulation());
    FEchoesReplayMetadata BridgeMetadata = Metadata;
    BridgeMetadata.ReplayId = FString::Printf(
        TEXT("p2-bridge-%s"),
        *FGuid::NewGuid().ToString(EGuidFormats::Digits));
    BridgeMetadata.MapId = TEXT("glass-scar");
    BridgeMetadata.OperationId = TEXT("skirmish");
    BridgeMetadata.OperationType = EEchoesReplayOperationType::Skirmish;
    BridgeMetadata.BuildIdentity =
        ReplayTestDigestHex(Compatibility.buildIdSha256);
    BridgeMetadata.RulesIdentity =
        ReplayTestDigestHex(Compatibility.rulesPackSha256);
    FEchoesReplayEnvelope BridgeEnvelope;
    if (!TestTrue(TEXT("Bridge replay metadata finalizes"),
                  FEchoesMatchReplayStore::FinalizeEnvelope(
                      BridgeMetadata, Replay, BridgeEnvelope, Error)))
    {
        AddError(Error);
        return false;
    }
    FReplayTestFile BridgeStored;
    if (!TestTrue(TEXT("Bridge replay saves before playback"),
                  FEchoesMatchReplayStore::SaveAtomic(
                      Directory, BridgeEnvelope, BridgeStored.Path, Error)) ||
        !TestTrue(TEXT("Saved replay opens through the runtime bridge"),
                  Bridge->BeginReplay(BridgeStored.Path, Error)))
    {
        AddError(Error);
        return false;
    }
    TestTrue(TEXT("Opened replay metadata is the saved envelope"),
             Bridge->GetActiveReplayMetadata() != nullptr &&
             Bridge->GetActiveReplayMetadata()->ReplayId ==
                 BridgeMetadata.ReplayId);
    TestEqual(TEXT("Replay presentation uses the detached map width"),
              Bridge->GetMapWidthTiles(),
              FEchoesSkirmishSetupModel::MapWidthTiles);
    TestEqual(TEXT("Replay presentation uses the detached map height"),
              Bridge->GetMapHeightTiles(),
              FEchoesSkirmishSetupModel::MapHeightTiles);
    const FVector ReplayCenter = Bridge->SimToWorld(
        echoes::sim::Vec2::FromTiles(32, 32));
    TestTrue(TEXT("Replay map center projects to world origin"),
             ReplayCenter.IsNearlyZero());
    const echoes::sim::Vec2 ReplayOrigin =
        Bridge->WorldToSim(FVector::ZeroVector);
    TestTrue(TEXT("World origin maps to replay tile 32,32"),
             ReplayOrigin == echoes::sim::Vec2::FromTiles(32, 32));

    const uint64 BeforeFailedSync =
        Bridge->GetReplayPlaybackState().CurrentTick;
    Bridge->FailNextReplayPresentationSyncForTesting();
    TestFalse(TEXT("Presentation sync failure rejects the step"),
              Bridge->StepReplay(Error));
    const FEchoesReplayPlaybackState FailedSync =
        Bridge->GetReplayPlaybackState();
    TestTrue(TEXT("Sync failure pauses and publishes an attributable error"),
             FailedSync.bPaused &&
             FailedSync.CurrentTick == BeforeFailedSync + 1 &&
             FailedSync.Error.Contains(TEXT("REPLAY_PRESENTATION_FAILED")));
    TestTrue(TEXT("Retry synchronizes the exact pending tick"),
             Bridge->StepReplay(Error));
    const FEchoesReplayPlaybackState RecoveredSync =
        Bridge->GetReplayPlaybackState();
    TestTrue(TEXT("Retry does not apply the step twice"),
             RecoveredSync.CurrentTick == FailedSync.CurrentTick &&
             RecoveredSync.Error.IsEmpty());

    constexpr uint64 SeekFailureTarget = 5;
    Bridge->FailNextReplayPresentationSyncForTesting();
    TestFalse(TEXT("Presentation sync failure rejects seek completion"),
              Bridge->SeekReplayTick(SeekFailureTarget, Error));
    const FEchoesReplayPlaybackState FailedSeekSync =
        Bridge->GetReplayPlaybackState();
    TestTrue(TEXT("Failed seek retains its exact authoritative target"),
             FailedSeekSync.bPaused &&
             FailedSeekSync.CurrentTick == SeekFailureTarget);
    TestTrue(TEXT("Seek retry synchronizes without rebuilding another target"),
             Bridge->SeekReplayTick(SeekFailureTarget, Error));
    TestEqual(TEXT("Seek retry retains the requested tick"),
              Bridge->GetReplayPlaybackState().CurrentTick,
              SeekFailureTarget);

    Bridge->SetReplayPaused(false);
    Bridge->FailNextReplayPresentationSyncForTesting();
    Bridge->Tick(0.05F);
    const FEchoesReplayPlaybackState FailedAutomaticSync =
        Bridge->GetReplayPlaybackState();
    TestTrue(TEXT("Automatic playback pauses and publishes sync failure"),
             FailedAutomaticSync.bPaused &&
             FailedAutomaticSync.CurrentTick ==
                 SeekFailureTarget + 1 &&
             FailedAutomaticSync.Error.Contains(
                 TEXT("REPLAY_PRESENTATION_FAILED")));
    Bridge->Tick(0.0F);
    const FEchoesReplayPlaybackState RecoveredAutomaticSync =
        Bridge->GetReplayPlaybackState();
    TestTrue(TEXT("Automatic sync retry does not advance a second time"),
             RecoveredAutomaticSync.bPaused &&
             RecoveredAutomaticSync.CurrentTick ==
                 FailedAutomaticSync.CurrentTick &&
             RecoveredAutomaticSync.Error.IsEmpty());

    Bridge->EndReplay();
    TestTrue(TEXT("Observer playback preserves live authority"),
             Bridge->GetSimulation() != nullptr &&
             Bridge->GetSimulation()->CurrentTick() == LiveTick &&
             Bridge->GetSimulation()->StateChecksum() == LiveChecksum);
    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return true;
}

#endif
