#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesContentSubsystem.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesContentCatalogTest,
    "Echoes.Runtime.Content.CanonicalPack",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesContentCatalogTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    FEchoesContentCatalog Catalog;
    FString Error;
    const bool bLoaded = FEchoesContentCatalog::LoadCanonicalPack(
        UEchoesContentSubsystem::GetCanonicalPackPath(),
        UEchoesContentSubsystem::GetCanonicalDigestPath(),
        Catalog,
        Error);
    TestTrue(*FString::Printf(TEXT("Canonical content loads: %s"), *Error), bLoaded);
    if (!bLoaded)
    {
        return false;
    }

    TestEqual(TEXT("Canonical pack format version is one"), Catalog.PackVersion, 1);
    TestEqual(TEXT("Canonical content schema is one"), Catalog.SchemaVersion, 1);
    TestEqual(TEXT("Three authored factions are present"), Catalog.Factions.Num(), 3);
    TestEqual(TEXT("Two factions are slice-playable"), Catalog.PlayableFactionCount(), 2);
    TestEqual(TEXT("Four authored units are present"), Catalog.Units.Num(), 4);
    TestEqual(TEXT("Four authored buildings are present"), Catalog.Buildings.Num(), 4);
    TestEqual(
        TEXT("Canonical SHA-256 matches the compiler output"),
        Catalog.Sha256,
        FString(TEXT("c83daba6a8743c1077e8c86553b7734c083210d304f65e9aa979e57861bdf1d9")));

    const FEchoesUnitContent* Lancer = Catalog.FindUnit(TEXT("mc_lancer"));
    if (TestNotNull(TEXT("Meridian Lancer is addressable by stable ID"), Lancer))
    {
        TestEqual(TEXT("Lancer health comes from authored data"), Lancer->MaxHealth, 145);
        TestEqual(TEXT("Lancer attack damage comes from authored data"), Lancer->AttackDamage, 18);
        TestEqual(TEXT("Lancer cooldown comes from authored data"), Lancer->AttackCooldownTicks, 30);
    }
    const FEchoesBuildingContent* Hearth =
        Catalog.FindBuilding(TEXT("ka_memory_hearth"));
    if (TestNotNull(TEXT("Kharuun Memory Hearth is addressable by stable ID"), Hearth))
    {
        TestEqual(TEXT("Memory Hearth health comes from authored data"), Hearth->MaxHealth, 1300);
        TestEqual(TEXT("Memory Hearth logistics come from authored data"), Hearth->LogisticsCapacity, 12);
    }
    TestEqual(
        TEXT("Reshape duration comes from authored data"),
        Catalog.FutureWell.ReshapeManifestDurationTicks,
        1800);

    const FString TemporaryDirectory = FPaths::Combine(
        FPaths::ProjectSavedDir(),
        TEXT("Automation/ContentCatalogDigestMismatch"));
    IFileManager::Get().DeleteDirectory(*TemporaryDirectory, false, true);
    IFileManager::Get().MakeDirectory(*TemporaryDirectory, true);
    const FString TamperedPack =
        FPaths::Combine(TemporaryDirectory, TEXT("EchoesContentPack.json"));
    const FString TamperedDigest = TamperedPack + TEXT(".sha256");
    FString PackText;
    FString DigestText;
    const bool bCopied =
        FFileHelper::LoadFileToString(PackText, *UEchoesContentSubsystem::GetCanonicalPackPath()) &&
        FFileHelper::LoadFileToString(DigestText, *UEchoesContentSubsystem::GetCanonicalDigestPath()) &&
        FFileHelper::SaveStringToFile(PackText + TEXT(" "), *TamperedPack) &&
        FFileHelper::SaveStringToFile(DigestText, *TamperedDigest);
    TestTrue(TEXT("Tampered content fixture was created"), bCopied);
    if (bCopied)
    {
        FEchoesContentCatalog Rejected;
        FString Rejection;
        TestFalse(
            TEXT("Digest mismatch prevents catalog creation"),
            FEchoesContentCatalog::LoadCanonicalPack(
                TamperedPack,
                TamperedDigest,
                Rejected,
                Rejection));
        TestEqual(
            TEXT("Digest mismatch has a stable rejection reason"),
            Rejection,
            FString(TEXT("CONTENT_DIGEST_MISMATCH")));
    }
    IFileManager::Get().DeleteDirectory(*TemporaryDirectory, false, true);
    return true;
}

#endif
