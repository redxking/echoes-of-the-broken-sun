#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/Crc.h"

#include "EchoesCampaignMapCheckpoint.h"

namespace
{
constexpr int32 VersionOffset = 8;

FEchoesCampaignMapCheckpointIdentity MakeIdentity()
{
    return {
        1,
        TEXT("preserve"),
        TEXT("m01-glass-scar"),
        TEXT("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"),
        TEXT("fedcba9876543210fedcba9876543210fedcba9876543210fedcba9876543210")};
}

void RewriteChecksum(TArray<uint8>& Bytes)
{
    const int32 Offset = Bytes.Num() - 4;
    const uint32 Crc = FCrc::MemCrc32(Bytes.GetData(), Offset);
    for (int32 ByteIndex = 0; ByteIndex < 4; ++ByteIndex)
    {
        Bytes[Offset + ByteIndex] = static_cast<uint8>(Crc >> (ByteIndex * 8));
    }
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesCampaignMapCheckpointTest,
    "Echoes.Runtime.Persistence.CampaignMapCheckpoint",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesCampaignMapCheckpointTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    const FEchoesCampaignMapCheckpointIdentity Identity = MakeIdentity();
    const TArray<uint8> Payload = {0xE1, 0xC0, 0x00, 0x7F, 0x12};
    EEchoesCampaignMapCheckpointFailure Failure =
        EEchoesCampaignMapCheckpointFailure::Integrity;
    TArray<uint8> Envelope;
    if (!TestTrue(
            TEXT("Wrap writes a current map-bound envelope"),
            FEchoesCampaignMapCheckpoint::Wrap(
                Identity, Payload, Envelope, Failure)))
    {
        return false;
    }
    TestEqual(TEXT("Successful wrap has no failure"), Failure,
        EEchoesCampaignMapCheckpointFailure::None);

    FEchoesCampaignMapCheckpointIdentity InspectedIdentity;
    TArray<uint8> InspectedPayload;
    TestTrue(
        TEXT("Inspection validates the outer format and CRC"),
        FEchoesCampaignMapCheckpoint::Inspect(
            Envelope, InspectedIdentity, InspectedPayload, Failure));
    TestTrue(TEXT("Inspection preserves its identity"), InspectedIdentity == Identity);
    TestTrue(TEXT("Inspection preserves bytes without snapshot interpretation"),
        InspectedPayload == Payload);

    TArray<uint8> RestoredPayload = {0xAA};
    TestTrue(
        TEXT("Extraction admits the exact expected identity"),
        FEchoesCampaignMapCheckpoint::Extract(
            Envelope, Identity, RestoredPayload, Failure));
    TestTrue(TEXT("Exact identity extraction preserves the original payload"),
        RestoredPayload == Payload);

    FEchoesCampaignMapCheckpointIdentity WrongIdentity = Identity;
    WrongIdentity.TerrainIdentitySha256 =
        TEXT("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    const TArray<uint8> UnchangedOutput = {0x5A, 0x5B};
    RestoredPayload = UnchangedOutput;
    TestFalse(
        TEXT("A valid envelope for another terrain identity is stale"),
        FEchoesCampaignMapCheckpoint::Extract(
            Envelope, WrongIdentity, RestoredPayload, Failure));
    TestEqual(TEXT("Stale identity has its stable reason"), Failure,
        EEchoesCampaignMapCheckpointFailure::Stale);
    TestTrue(TEXT("Stale refusal leaves payload output unchanged"),
        RestoredPayload == UnchangedOutput);

    TArray<uint8> Corrupted = Envelope;
    Corrupted[VersionOffset + 2] ^= 0x80;
    RestoredPayload = UnchangedOutput;
    TestFalse(
        TEXT("CRC corruption is rejected before extraction"),
        FEchoesCampaignMapCheckpoint::Extract(
            Corrupted, Identity, RestoredPayload, Failure));
    TestEqual(TEXT("CRC corruption has integrity reason"), Failure,
        EEchoesCampaignMapCheckpointFailure::Integrity);
    TestTrue(TEXT("Corruption refusal leaves payload output unchanged"),
        RestoredPayload == UnchangedOutput);

    TArray<uint8> Truncated = Envelope;
    Truncated.Pop();
    RestoredPayload = UnchangedOutput;
    TestFalse(
        TEXT("Truncated envelope is rejected"),
        FEchoesCampaignMapCheckpoint::Extract(
            Truncated, Identity, RestoredPayload, Failure));
    TestEqual(TEXT("Truncation has integrity reason"), Failure,
        EEchoesCampaignMapCheckpointFailure::Integrity);
    TestTrue(TEXT("Truncation refusal leaves payload output unchanged"),
        RestoredPayload == UnchangedOutput);

    TArray<uint8> FutureVersion = Envelope;
    FutureVersion[VersionOffset] = 2;
    FutureVersion[VersionOffset + 1] = 0;
    RewriteChecksum(FutureVersion);
    RestoredPayload = UnchangedOutput;
    TestFalse(
        TEXT("A checksum-valid unsupported version is refused"),
        FEchoesCampaignMapCheckpoint::Extract(
            FutureVersion, Identity, RestoredPayload, Failure));
    TestEqual(TEXT("Unsupported version has stable reason"), Failure,
        EEchoesCampaignMapCheckpointFailure::Unsupported);
    TestTrue(TEXT("Unsupported version leaves payload output unchanged"),
        RestoredPayload == UnchangedOutput);

    const TArray<uint8> LegacyPayload = {0x01, 0x02, 0x03};
    RestoredPayload = UnchangedOutput;
    TestFalse(
        TEXT("Legacy payload without a campaign map envelope is refused"),
        FEchoesCampaignMapCheckpoint::Extract(
            LegacyPayload, Identity, RestoredPayload, Failure));
    TestEqual(TEXT("Legacy payload is explicitly unbound"), Failure,
        EEchoesCampaignMapCheckpointFailure::Unbound);
    TestTrue(TEXT("Unbound refusal leaves payload output unchanged"),
        RestoredPayload == UnchangedOutput);

    TArray<uint8> OversizedPayload;
    OversizedPayload.SetNumUninitialized(
        FEchoesCampaignMapCheckpoint::MaximumPayloadBytes + 1);
    TArray<uint8> ExistingEnvelope = {0x44};
    TestFalse(
        TEXT("Oversized payload is refused before envelope mutation"),
        FEchoesCampaignMapCheckpoint::Wrap(
            Identity, OversizedPayload, ExistingEnvelope, Failure));
    TestEqual(TEXT("Oversized payload has integrity reason"), Failure,
        EEchoesCampaignMapCheckpointFailure::Integrity);
    TestTrue(TEXT("Oversized refusal leaves envelope output unchanged"),
        ExistingEnvelope == TArray<uint8>({0x44}));

    FEchoesCampaignMapCheckpointIdentity UnboundIdentity = Identity;
    UnboundIdentity.SourceSha256.Reset();
    RestoredPayload = UnchangedOutput;
    TestFalse(
        TEXT("An unbound expected identity is refused"),
        FEchoesCampaignMapCheckpoint::Extract(
            Envelope, UnboundIdentity, RestoredPayload, Failure));
    TestEqual(TEXT("Unbound expected identity has stable reason"), Failure,
        EEchoesCampaignMapCheckpointFailure::Unbound);
    TestTrue(TEXT("Unbound expected identity leaves payload output unchanged"),
        RestoredPayload == UnchangedOutput);

    return true;
}

#endif
