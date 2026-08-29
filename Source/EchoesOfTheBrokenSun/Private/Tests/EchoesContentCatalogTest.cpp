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
    TestEqual(TEXT("Eight authored units are present"), Catalog.Units.Num(), 8);
    TestEqual(TEXT("Eight authored buildings are present"), Catalog.Buildings.Num(), 8);
    TestEqual(
        TEXT("Canonical SHA-256 matches the compiler output"),
        Catalog.Sha256,
        FString(TEXT("13d939a3f3c720c6c56adaf06c7fef43a31f3c9e1916822660951e51fc71843b")));

    echoes::sim::SimulationRules Rules;
    FString RulesError;
    TestTrue(
        *FString::Printf(TEXT("Canonical data maps to deterministic rules: %s"), *RulesError),
        Catalog.BuildSimulationRules(20, Rules, RulesError));
    TestEqual(TEXT("Meridian Lancer health enters simulation rules"),
              Rules.archetypes[0][1].maxHitPoints, 145);
    TestEqual(TEXT("Kharuun Riftstalker range enters fixed-point rules"),
              Rules.archetypes[1][1].attackRangeRaw, 5120);
    TestEqual(TEXT("Meridian Anchor footprint enters simulation rules"),
              Rules.archetypes[0][2].footprintHalfExtentRaw, 2560);
    TestEqual(TEXT("Meridian Bulwark health enters distinct heavy rules"),
              Rules.archetypes[0][7].maxHitPoints, 260);
    TestEqual(TEXT("Kharuun Resonant sight enters distinct scout rules"),
              Rules.archetypes[1][8].visionTiles, 16);
    TestEqual(TEXT("Kharuun Listening Spine enters utility rules"),
              Rules.archetypes[1][9].maxHitPoints, 440);
    TestEqual(TEXT("Authored Future Well duration enters simulation rules"),
              Rules.futureWell.reshapeDurationMinimumTicks,
              static_cast<echoes::sim::Tick>(1800));
    TestEqual(TEXT("Bulwark cover depth enters fixed-point rules"),
              Rules.bulwarkDeployment.coverDepthRaw, 3584);
    TestEqual(TEXT("Bulwark cover half-width enters fixed-point rules"),
              Rules.bulwarkDeployment.coverHalfWidthRaw, 2560);
    TestEqual(TEXT("Bulwark cover reduction enters simulation rules"),
              Rules.bulwarkDeployment.damageReductionPercent, 40);
    TestEqual(TEXT("Bulwark deployed movement enters simulation rules"),
              Rules.bulwarkDeployment.deployedMovementPercent, 35);
    TestEqual(TEXT("Relay connection radius enters fixed-point rules"),
              Rules.relaySupply.connectionRadiusRaw, 7168);
    TestEqual(TEXT("Relay capacity enters simulation rules"),
              Rules.relaySupply.capacityBonus, 4);
    TestEqual(TEXT("Relay duration enters simulation rules"),
              Rules.relaySupply.durationTicks,
              static_cast<echoes::sim::Tick>(400));
    TestEqual(TEXT("Relay cooldown enters simulation rules"),
              Rules.relaySupply.cooldownTicks,
              static_cast<echoes::sim::Tick>(800));
    TestEqual(TEXT("Waystone movement enters fixed-point rules"),
              Rules.waystoneMigration.movementPerTickRaw, 61);
    TestEqual(TEXT("Waystone uproot duration enters simulation rules"),
              Rules.waystoneMigration.uprootTicks,
              static_cast<echoes::sim::Tick>(40));
    TestEqual(TEXT("Waystone root duration enters simulation rules"),
              Rules.waystoneMigration.rootTicks,
              static_cast<echoes::sim::Tick>(60));
    TestEqual(TEXT("Waystone exposure enters simulation rules"),
              Rules.waystoneMigration.mobileDamageTakenPercent, 125);

    const FEchoesUnitContent* Lancer = Catalog.FindUnit(TEXT("mc_lancer"));
    if (TestNotNull(TEXT("Meridian Lancer is addressable by stable ID"), Lancer))
    {
        TestEqual(TEXT("Lancer display name is authored"), Lancer->DisplayName, FString(TEXT("Lancer")));
        TestEqual(TEXT("Lancer health comes from authored data"), Lancer->MaxHealth, 145);
        TestEqual(TEXT("Lancer attack damage comes from authored data"), Lancer->AttackDamage, 18);
        TestEqual(TEXT("Lancer cooldown comes from authored data"), Lancer->AttackCooldownTicks, 30);
    }
    const FEchoesUnitContent* Resonant = Catalog.FindUnit(TEXT("ka_resonant"));
    if (TestNotNull(TEXT("Kharuun Resonant completes the slice roster"), Resonant))
    {
        TestEqual(TEXT("Resonant role is authored"), Resonant->Role, FString(TEXT("scout_counter_scout")));
        TestEqual(TEXT("Resonant sight comes from authored data"), Resonant->SightCentimeters, 1550);
    }
    const FEchoesUnitContent* Relay = Catalog.FindUnit(TEXT("mc_relay_skiff"));
    if (TestNotNull(TEXT("Meridian Relay is addressable by stable ID"), Relay))
    {
        TestEqual(TEXT("Relay connection radius is authored"),
                  Relay->SupplyConnectionRadiusCentimeters, 700);
        TestEqual(TEXT("Relay capacity bonus is authored"),
                  Relay->SupplyCapacityBonus, 4);
        TestEqual(TEXT("Relay duration is authored"),
                  Relay->SupplyDurationTicks, 400);
        TestEqual(TEXT("Relay cooldown is authored"),
                  Relay->SupplyCooldownTicks, 800);
    }
    const FEchoesUnitContent* Bulwark = Catalog.FindUnit(TEXT("mc_bulwark_team"));
    if (TestNotNull(TEXT("Meridian Bulwark is addressable by stable ID"), Bulwark))
    {
        TestEqual(TEXT("Bulwark cover depth is authored"),
                  Bulwark->DeploymentCoverDepthCentimeters, 350);
        TestEqual(TEXT("Bulwark damage reduction is authored"),
                  Bulwark->DeploymentDamageReductionPercent, 40);
        TestEqual(TEXT("Bulwark movement tradeoff is authored"),
                  Bulwark->DeploymentMoveSpeedPercent, 35);
    }
    const FEchoesBuildingContent* Hearth =
        Catalog.FindBuilding(TEXT("ka_memory_hearth"));
    if (TestNotNull(TEXT("Kharuun Memory Hearth is addressable by stable ID"), Hearth))
    {
        TestEqual(TEXT("Memory Hearth display name is authored"), Hearth->DisplayName, FString(TEXT("Memory Hearth")));
        TestEqual(TEXT("Memory Hearth health comes from authored data"), Hearth->MaxHealth, 1300);
        TestEqual(TEXT("Memory Hearth logistics come from authored data"), Hearth->LogisticsCapacity, 12);
    }
    const FEchoesBuildingContent* Waystone =
        Catalog.FindBuilding(TEXT("ka_waystone"));
    if (TestNotNull(TEXT("Kharuun Waystone is addressable by stable ID"), Waystone))
    {
        TestEqual(TEXT("Waystone movement is authored"),
                  Waystone->MigrationMoveSpeedCentimetersPerSecond, 120);
        TestEqual(TEXT("Waystone uproot timing is authored"),
                  Waystone->MigrationUprootTicks, 40);
        TestEqual(TEXT("Waystone root timing is authored"),
                  Waystone->MigrationRootTicks, 60);
        TestEqual(TEXT("Waystone mobile vulnerability is authored"),
                  Waystone->MigrationMobileDamageTakenPercent, 125);
    }
    const FEchoesBuildingContent* Foundry =
        Catalog.FindBuilding(TEXT("mc_array_foundry"));
    if (TestNotNull(TEXT("Meridian Array Foundry completes the slice roster"), Foundry))
    {
        TestEqual(TEXT("Array Foundry is a production structure"), Foundry->Role, FString(TEXT("production")));
        TestEqual(TEXT("Array Foundry footprint is authored"), Foundry->FootprintCells, FIntPoint(4, 4));
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
