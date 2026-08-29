#include "EchoesContentSubsystem.h"

#include "Dom/JsonObject.h"
#include "EchoesOfTheBrokenSun.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
constexpr int32 ExpectedPackVersion = 1;
constexpr int32 ExpectedSchemaVersion = 1;

constexpr uint32 Sha256RoundConstants[64] = {
    0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U,
    0x3956c25bU, 0x59f111f1U, 0x923f82a4U, 0xab1c5ed5U,
    0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U,
    0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U,
    0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU,
    0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
    0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U,
    0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U,
    0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U,
    0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U,
    0xa2bfe8a1U, 0xa81a664bU, 0xc24b8b70U, 0xc76c51a3U,
    0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U,
    0x19a4c116U, 0x1e376c08U, 0x2748774cU, 0x34b0bcb5U,
    0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
    0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U,
    0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U,
};

[[nodiscard]] constexpr uint32 RotateRight(const uint32 Value, const uint32 Count)
{
    return (Value >> Count) | (Value << (32U - Count));
}

void TransformSha256Block(const uint8* Block, uint32 (&State)[8])
{
    uint32 Schedule[64]{};
    for (int32 Index = 0; Index < 16; ++Index)
    {
        const int32 Offset = Index * 4;
        Schedule[Index] =
            (static_cast<uint32>(Block[Offset]) << 24U) |
            (static_cast<uint32>(Block[Offset + 1]) << 16U) |
            (static_cast<uint32>(Block[Offset + 2]) << 8U) |
            static_cast<uint32>(Block[Offset + 3]);
    }
    for (int32 Index = 16; Index < 64; ++Index)
    {
        const uint32 S0 =
            RotateRight(Schedule[Index - 15], 7U) ^
            RotateRight(Schedule[Index - 15], 18U) ^
            (Schedule[Index - 15] >> 3U);
        const uint32 S1 =
            RotateRight(Schedule[Index - 2], 17U) ^
            RotateRight(Schedule[Index - 2], 19U) ^
            (Schedule[Index - 2] >> 10U);
        Schedule[Index] = Schedule[Index - 16] + S0 +
            Schedule[Index - 7] + S1;
    }

    uint32 A = State[0];
    uint32 B = State[1];
    uint32 C = State[2];
    uint32 D = State[3];
    uint32 E = State[4];
    uint32 F = State[5];
    uint32 G = State[6];
    uint32 H = State[7];
    for (int32 Index = 0; Index < 64; ++Index)
    {
        const uint32 Sum1 = RotateRight(E, 6U) ^ RotateRight(E, 11U) ^
            RotateRight(E, 25U);
        const uint32 Choice = (E & F) ^ ((~E) & G);
        const uint32 Temporary1 = H + Sum1 + Choice +
            Sha256RoundConstants[Index] + Schedule[Index];
        const uint32 Sum0 = RotateRight(A, 2U) ^ RotateRight(A, 13U) ^
            RotateRight(A, 22U);
        const uint32 Majority = (A & B) ^ (A & C) ^ (B & C);
        const uint32 Temporary2 = Sum0 + Majority;

        H = G;
        G = F;
        F = E;
        E = D + Temporary1;
        D = C;
        C = B;
        B = A;
        A = Temporary1 + Temporary2;
    }

    State[0] += A;
    State[1] += B;
    State[2] += C;
    State[3] += D;
    State[4] += E;
    State[5] += F;
    State[6] += G;
    State[7] += H;
}

[[nodiscard]] FString ComputeSha256Hex(const TArray<uint8>& Bytes)
{
    uint32 State[8] = {
        0x6a09e667U, 0xbb67ae85U, 0x3c6ef372U, 0xa54ff53aU,
        0x510e527fU, 0x9b05688cU, 0x1f83d9abU, 0x5be0cd19U,
    };

    int32 Offset = 0;
    while (Bytes.Num() - Offset >= 64)
    {
        TransformSha256Block(Bytes.GetData() + Offset, State);
        Offset += 64;
    }

    uint8 FinalBlocks[128]{};
    const int32 Remaining = Bytes.Num() - Offset;
    if (Remaining > 0)
    {
        FMemory::Memcpy(FinalBlocks, Bytes.GetData() + Offset, Remaining);
    }
    FinalBlocks[Remaining] = 0x80U;
    const int32 FinalLength = Remaining < 56 ? 64 : 128;
    const uint64 BitLength = static_cast<uint64>(Bytes.Num()) * 8ULL;
    for (int32 Index = 0; Index < 8; ++Index)
    {
        FinalBlocks[FinalLength - 1 - Index] =
            static_cast<uint8>(BitLength >> (Index * 8));
    }
    TransformSha256Block(FinalBlocks, State);
    if (FinalLength == 128)
    {
        TransformSha256Block(FinalBlocks + 64, State);
    }

    FString Digest;
    Digest.Reserve(64);
    for (const uint32 Word : State)
    {
        Digest += FString::Printf(TEXT("%08x"), Word);
    }
    return Digest;
}

[[nodiscard]] bool ReadRequiredString(
    const TSharedPtr<FJsonObject>& Object,
    const TCHAR* Field,
    FString& OutValue,
    const FString& Path,
    FString& OutError)
{
    if (!Object.IsValid() || !Object->TryGetStringField(Field, OutValue) ||
        OutValue.IsEmpty())
    {
        OutError = FString::Printf(TEXT("CONTENT_FIELD_INVALID:%s.%s"), *Path, Field);
        return false;
    }
    return true;
}

[[nodiscard]] bool ReadRequiredInteger(
    const TSharedPtr<FJsonObject>& Object,
    const TCHAR* Field,
    int32& OutValue,
    const FString& Path,
    FString& OutError,
    int32 Minimum = 0)
{
    double Number = 0.0;
    if (!Object.IsValid() || !Object->TryGetNumberField(Field, Number) ||
        Number < static_cast<double>(Minimum) ||
        Number > static_cast<double>(MAX_int32) ||
        FMath::FloorToDouble(Number) != Number)
    {
        OutError = FString::Printf(TEXT("CONTENT_FIELD_INVALID:%s.%s"), *Path, Field);
        return false;
    }
    OutValue = static_cast<int32>(Number);
    return true;
}

[[nodiscard]] bool ReadCost(
    const TSharedPtr<FJsonObject>& Object,
    int32& OutMatter,
    int32& OutDawn,
    const FString& Path,
    FString& OutError)
{
    const TSharedPtr<FJsonObject>* Cost = nullptr;
    if (!Object.IsValid() || !Object->TryGetObjectField(TEXT("cost"), Cost) ||
        Cost == nullptr || !Cost->IsValid())
    {
        OutError = FString::Printf(TEXT("CONTENT_FIELD_INVALID:%s.cost"), *Path);
        return false;
    }
    return ReadRequiredInteger(*Cost, TEXT("matter"), OutMatter, Path + TEXT(".cost"), OutError) &&
           ReadRequiredInteger(*Cost, TEXT("dawn"), OutDawn, Path + TEXT(".cost"), OutError);
}

[[nodiscard]] bool ValidateUniqueId(
    const FString& Id,
    TSet<FString>& Seen,
    const FString& Path,
    FString& OutError)
{
    if (Seen.Contains(Id))
    {
        OutError = FString::Printf(TEXT("CONTENT_DUPLICATE_ID:%s:%s"), *Path, *Id);
        return false;
    }
    Seen.Add(Id);
    return true;
}
}

int32 FEchoesContentCatalog::PlayableFactionCount() const
{
    return Factions.FilterByPredicate(
        [](const FEchoesFactionContent& Faction)
        {
            return Faction.bVerticalSlicePlayable;
        }).Num();
}

const FEchoesUnitContent* FEchoesContentCatalog::FindUnit(const FString& Id) const
{
    return Units.FindByPredicate(
        [&Id](const FEchoesUnitContent& Unit)
        {
            return Unit.Id == Id;
        });
}

const FEchoesBuildingContent* FEchoesContentCatalog::FindBuilding(const FString& Id) const
{
    return Buildings.FindByPredicate(
        [&Id](const FEchoesBuildingContent& Building)
        {
            return Building.Id == Id;
        });
}

bool FEchoesContentCatalog::BuildSimulationRules(
    const uint32 TicksPerSecond,
    echoes::sim::SimulationRules& OutRules,
    FString& OutError) const
{
    OutRules = {};
    OutError.Reset();
    if (TicksPerSecond == 0 || Sha256.Len() != 64)
    {
        OutError = TEXT("SIM_RULES_HEADER_INVALID");
        return false;
    }

    const auto HexNibble = [](const TCHAR Character) -> int32
    {
        if (Character >= TEXT('0') && Character <= TEXT('9'))
        {
            return Character - TEXT('0');
        }
        if (Character >= TEXT('a') && Character <= TEXT('f'))
        {
            return Character - TEXT('a') + 10;
        }
        if (Character >= TEXT('A') && Character <= TEXT('F'))
        {
            return Character - TEXT('A') + 10;
        }
        return -1;
    };
    for (int32 Index = 0; Index < 32; ++Index)
    {
        const int32 High = HexNibble(Sha256[Index * 2]);
        const int32 Low = HexNibble(Sha256[Index * 2 + 1]);
        if (High < 0 || Low < 0)
        {
            OutError = TEXT("SIM_RULES_DIGEST_INVALID");
            return false;
        }
        OutRules.contentSha256[static_cast<std::size_t>(Index)] =
            static_cast<uint8>((High << 4) | Low);
    }

    struct FUnitBinding final
    {
        const TCHAR* Id;
        const TCHAR* FactionId;
        const TCHAR* Role;
        echoes::sim::Faction Faction;
        echoes::sim::EntityType Type;
    };
    constexpr FUnitBinding UnitBindings[] = {
        {TEXT("mc_surveyor"), TEXT("meridian_compact"), TEXT("worker"),
         echoes::sim::Faction::MeridianCompact, echoes::sim::EntityType::Worker},
        {TEXT("mc_lancer"), TEXT("meridian_compact"), TEXT("ranged_line"),
         echoes::sim::Faction::MeridianCompact, echoes::sim::EntityType::Soldier},
        {TEXT("mc_bulwark_team"), TEXT("meridian_compact"), TEXT("heavy_screen"),
         echoes::sim::Faction::MeridianCompact, echoes::sim::EntityType::HeavyUnit},
        {TEXT("mc_relay_skiff"), TEXT("meridian_compact"), TEXT("scout_support"),
         echoes::sim::Faction::MeridianCompact, echoes::sim::EntityType::ScoutUnit},
        {TEXT("ka_tender"), TEXT("kharuun_assemblies"), TEXT("worker"),
         echoes::sim::Faction::KharuunAssemblies, echoes::sim::EntityType::Worker},
        {TEXT("ka_riftstalker"), TEXT("kharuun_assemblies"), TEXT("mobile_skirmisher"),
         echoes::sim::Faction::KharuunAssemblies, echoes::sim::EntityType::Soldier},
        {TEXT("ka_cairnback"), TEXT("kharuun_assemblies"), TEXT("assault_screen"),
         echoes::sim::Faction::KharuunAssemblies, echoes::sim::EntityType::HeavyUnit},
        {TEXT("ka_resonant"), TEXT("kharuun_assemblies"), TEXT("scout_counter_scout"),
         echoes::sim::Faction::KharuunAssemblies, echoes::sim::EntityType::ScoutUnit},
    };
    for (const FUnitBinding& Binding : UnitBindings)
    {
        const FEchoesUnitContent* Unit = FindUnit(Binding.Id);
        if (Unit == nullptr || Unit->FactionId != Binding.FactionId ||
            Unit->Role != Binding.Role)
        {
            OutError = FString::Printf(TEXT("SIM_RULES_UNIT_BINDING_INVALID:%s"), Binding.Id);
            return false;
        }
        echoes::sim::EntityArchetypeRules& Rules =
            OutRules.archetypes[static_cast<std::size_t>(Binding.Faction)]
                               [static_cast<std::size_t>(Binding.Type)];
        Rules.cost = {Unit->MatterCost, Unit->DawnCost};
        Rules.maxHitPoints = Unit->MaxHealth;
        const int64 MovementNumerator =
            static_cast<int64>(Unit->MoveSpeedCentimetersPerSecond) *
            echoes::sim::kFixedScale;
        const int64 MovementDenominator =
            static_cast<int64>(TicksPerSecond) * 100;
        Rules.movementPerTickRaw = static_cast<int32>(
            MovementNumerator / MovementDenominator);
        Rules.visionTiles = FMath::DivideAndRoundUp(Unit->SightCentimeters, 100);
        Rules.attackRangeRaw = static_cast<int32>(
            static_cast<int64>(Unit->AttackRangeCentimeters) *
            echoes::sim::kFixedScale / 100);
        Rules.attackDamage = Unit->AttackDamage;
        Rules.attackPeriodTicks = Unit->AttackCooldownTicks;
        Rules.workRate = Unit->WorkRate;
        Rules.cargoCapacity = Unit->CargoCapacity;
        Rules.populationCost = Unit->PopulationCost;
        Rules.productionTicks = Unit->ProductionTicks;
        Rules.footprintHalfExtentRaw = echoes::sim::kFixedScale / 8;
        if (Rules.movementPerTickRaw <= 0)
        {
            OutError = FString::Printf(TEXT("SIM_RULES_UNIT_MOVEMENT_INVALID:%s"), Binding.Id);
            return false;
        }
        if (Binding.Faction == echoes::sim::Faction::MeridianCompact &&
            Binding.Type == echoes::sim::EntityType::HeavyUnit)
        {
            if (Unit->DeploymentCoverDepthCentimeters <= 0 ||
                Unit->DeploymentCoverHalfWidthCentimeters <= 0 ||
                Unit->DeploymentDamageReductionPercent <= 0 ||
                Unit->DeploymentDamageReductionPercent >= 100 ||
                Unit->DeploymentMoveSpeedPercent <= 0 ||
                Unit->DeploymentMoveSpeedPercent >= 100)
            {
                OutError = TEXT("SIM_RULES_BULWARK_DEPLOYMENT_INVALID");
                return false;
            }
            OutRules.bulwarkDeployment.coverDepthRaw = static_cast<int32>(
                static_cast<int64>(Unit->DeploymentCoverDepthCentimeters) *
                echoes::sim::kFixedScale / 100);
            OutRules.bulwarkDeployment.coverHalfWidthRaw = static_cast<int32>(
                static_cast<int64>(Unit->DeploymentCoverHalfWidthCentimeters) *
                echoes::sim::kFixedScale / 100);
            OutRules.bulwarkDeployment.damageReductionPercent =
                Unit->DeploymentDamageReductionPercent;
            OutRules.bulwarkDeployment.deployedMovementPercent =
                Unit->DeploymentMoveSpeedPercent;
        }
    }

    struct FBuildingBinding final
    {
        const TCHAR* Id;
        const TCHAR* FactionId;
        const TCHAR* Role;
        echoes::sim::Faction Faction;
        echoes::sim::EntityType Type;
    };
    constexpr FBuildingBinding BuildingBindings[] = {
        {TEXT("mc_anchor"), TEXT("meridian_compact"), TEXT("headquarters_dropoff"),
         echoes::sim::Faction::MeridianCompact, echoes::sim::EntityType::CommandCore},
        {TEXT("mc_power_link"), TEXT("meridian_compact"), TEXT("supply_node"),
         echoes::sim::Faction::MeridianCompact, echoes::sim::EntityType::Dropoff},
        {TEXT("mc_array_foundry"), TEXT("meridian_compact"), TEXT("production"),
         echoes::sim::Faction::MeridianCompact, echoes::sim::EntityType::Barracks},
        {TEXT("mc_aegis_post"), TEXT("meridian_compact"), TEXT("defense"),
         echoes::sim::Faction::MeridianCompact, echoes::sim::EntityType::UtilityStructure},
        {TEXT("ka_memory_hearth"), TEXT("kharuun_assemblies"), TEXT("headquarters_dropoff"),
         echoes::sim::Faction::KharuunAssemblies, echoes::sim::EntityType::CommandCore},
        {TEXT("ka_waystone"), TEXT("kharuun_assemblies"), TEXT("mobile_supply_node"),
         echoes::sim::Faction::KharuunAssemblies, echoes::sim::EntityType::Dropoff},
        {TEXT("ka_growth_basin"), TEXT("kharuun_assemblies"), TEXT("production"),
         echoes::sim::Faction::KharuunAssemblies, echoes::sim::EntityType::Barracks},
        {TEXT("ka_listening_spine"), TEXT("kharuun_assemblies"), TEXT("detection"),
         echoes::sim::Faction::KharuunAssemblies, echoes::sim::EntityType::UtilityStructure},
    };
    for (const FBuildingBinding& Binding : BuildingBindings)
    {
        const FEchoesBuildingContent* Building = FindBuilding(Binding.Id);
        if (Building == nullptr || Building->FactionId != Binding.FactionId ||
            Building->Role != Binding.Role)
        {
            OutError = FString::Printf(TEXT("SIM_RULES_BUILDING_BINDING_INVALID:%s"), Binding.Id);
            return false;
        }
        echoes::sim::EntityArchetypeRules& Rules =
            OutRules.archetypes[static_cast<std::size_t>(Binding.Faction)]
                               [static_cast<std::size_t>(Binding.Type)];
        Rules.cost = {Building->MatterCost, Building->DawnCost};
        Rules.maxHitPoints = Building->MaxHealth;
        Rules.visionTiles = FMath::DivideAndRoundUp(Building->SightCentimeters, 100);
        Rules.constructionRequired = Building->ConstructionTicks;
        Rules.populationCapacity = Building->LogisticsCapacity;
        Rules.footprintHalfExtentRaw =
            FMath::Max(Building->FootprintCells.X, Building->FootprintCells.Y) *
            echoes::sim::kFixedScale / 2;
    }

    OutRules.futureWell.harvestImmediateDawn = FutureWell.HarvestImmediateDawn;
    OutRules.futureWell.preserveDawnPerInterval = FutureWell.PreserveDawnPerInterval;
    OutRules.futureWell.preserveIntervalTicks = FutureWell.PreserveIntervalTicks;
    OutRules.futureWell.preserveVisionTiles =
        FMath::DivideAndRoundUp(FutureWell.PreserveVisionRadiusCentimeters, 100);
    OutRules.futureWell.reshapeDawnCost = FutureWell.ReshapeDawnCost;
    OutRules.futureWell.reshapeDurationMinimumTicks =
        FutureWell.ReshapeManifestDurationTicks;
    OutRules.futureWell.reshapeDurationMaximumTicks =
        FutureWell.ReshapeManifestDurationTicks;
    return true;
}

bool FEchoesContentCatalog::LoadCanonicalPack(
    const FString& PackPath,
    const FString& DigestPath,
    FEchoesContentCatalog& OutCatalog,
    FString& OutError)
{
    OutCatalog = {};
    OutError.Reset();

    TArray<uint8> PackBytes;
    if (!FFileHelper::LoadFileToArray(PackBytes, *PackPath) || PackBytes.IsEmpty())
    {
        OutError = TEXT("CONTENT_PACK_MISSING");
        return false;
    }
    const FString ActualDigest = ComputeSha256Hex(PackBytes);
    FString DigestText;
    if (!FFileHelper::LoadFileToString(DigestText, *DigestPath))
    {
        OutError = TEXT("CONTENT_DIGEST_MISSING");
        return false;
    }
    DigestText.TrimStartAndEndInline();
    const FString ExpectedDigest = DigestText.ToLower();
    if (ExpectedDigest.Len() != 64 || ExpectedDigest != ActualDigest)
    {
        OutError = TEXT("CONTENT_DIGEST_MISMATCH");
        return false;
    }

    FUTF8ToTCHAR Converted(
        reinterpret_cast<const ANSICHAR*>(PackBytes.GetData()),
        PackBytes.Num());
    const FString JsonText(Converted.Length(), Converted.Get());
    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        OutError = TEXT("CONTENT_PACK_PARSE_FAILED");
        return false;
    }

    FString PackFormat;
    if (!ReadRequiredString(Root, TEXT("pack_format"), PackFormat, TEXT("root"), OutError) ||
        PackFormat != TEXT("echoes-content-pack") ||
        !ReadRequiredInteger(Root, TEXT("pack_version"), OutCatalog.PackVersion, TEXT("root"), OutError, 1) ||
        !ReadRequiredInteger(Root, TEXT("schema_version"), OutCatalog.SchemaVersion, TEXT("root"), OutError, 1))
    {
        if (OutError.IsEmpty())
        {
            OutError = TEXT("CONTENT_PACK_FORMAT_INVALID");
        }
        return false;
    }
    if (OutCatalog.PackVersion != ExpectedPackVersion ||
        OutCatalog.SchemaVersion != ExpectedSchemaVersion)
    {
        OutError = TEXT("CONTENT_VERSION_UNSUPPORTED");
        return false;
    }

    const TArray<TSharedPtr<FJsonValue>>* FactionValues = nullptr;
    const TArray<TSharedPtr<FJsonValue>>* UnitValues = nullptr;
    const TArray<TSharedPtr<FJsonValue>>* BuildingValues = nullptr;
    if (!Root->TryGetArrayField(TEXT("factions"), FactionValues) ||
        !Root->TryGetArrayField(TEXT("units"), UnitValues) ||
        !Root->TryGetArrayField(TEXT("buildings"), BuildingValues) ||
        FactionValues == nullptr || UnitValues == nullptr || BuildingValues == nullptr ||
        FactionValues->IsEmpty() || UnitValues->IsEmpty() || BuildingValues->IsEmpty())
    {
        OutError = TEXT("CONTENT_COLLECTION_MISSING");
        return false;
    }

    TSet<FString> FactionIds;
    for (int32 Index = 0; Index < FactionValues->Num(); ++Index)
    {
        const FString Path = FString::Printf(TEXT("factions[%d]"), Index);
        const TSharedPtr<FJsonObject> Object = (*FactionValues)[Index]->AsObject();
        FEchoesFactionContent Faction;
        if (!ReadRequiredString(Object, TEXT("id"), Faction.Id, Path, OutError) ||
            !ReadRequiredString(Object, TEXT("display_name"), Faction.DisplayName, Path, OutError) ||
            !Object.IsValid() ||
            !Object->TryGetBoolField(TEXT("vertical_slice_playable"), Faction.bVerticalSlicePlayable) ||
            !ValidateUniqueId(Faction.Id, FactionIds, Path, OutError))
        {
            if (OutError.IsEmpty())
            {
                OutError = FString::Printf(TEXT("CONTENT_FIELD_INVALID:%s.vertical_slice_playable"), *Path);
            }
            return false;
        }
        OutCatalog.Factions.Add(MoveTemp(Faction));
    }

    TSet<FString> UnitIds;
    for (int32 Index = 0; Index < UnitValues->Num(); ++Index)
    {
        const FString Path = FString::Printf(TEXT("units[%d]"), Index);
        const TSharedPtr<FJsonObject> Object = (*UnitValues)[Index]->AsObject();
        FEchoesUnitContent Unit;
        if (!ReadRequiredString(Object, TEXT("id"), Unit.Id, Path, OutError) ||
            !ReadRequiredString(Object, TEXT("display_name"), Unit.DisplayName, Path, OutError) ||
            !ReadRequiredString(Object, TEXT("faction"), Unit.FactionId, Path, OutError) ||
            !ReadRequiredString(Object, TEXT("role"), Unit.Role, Path, OutError) ||
            !FactionIds.Contains(Unit.FactionId) ||
            !ValidateUniqueId(Unit.Id, UnitIds, Path, OutError) ||
            !ReadCost(Object, Unit.MatterCost, Unit.DawnCost, Path, OutError) ||
            !ReadRequiredInteger(Object, TEXT("max_health"), Unit.MaxHealth, Path, OutError, 1) ||
            !ReadRequiredInteger(Object, TEXT("move_speed_cm_s"), Unit.MoveSpeedCentimetersPerSecond, Path, OutError, 1) ||
            !ReadRequiredInteger(Object, TEXT("sight_cm"), Unit.SightCentimeters, Path, OutError, 1) ||
            !ReadRequiredInteger(Object, TEXT("population_cost"), Unit.PopulationCost, Path, OutError, 1) ||
            !ReadRequiredInteger(Object, TEXT("production_ticks"), Unit.ProductionTicks, Path, OutError, 1) ||
            !ReadRequiredInteger(Object, TEXT("work_rate"), Unit.WorkRate, Path, OutError) ||
            !ReadRequiredInteger(Object, TEXT("cargo_capacity"), Unit.CargoCapacity, Path, OutError))
        {
            if (OutError.IsEmpty())
            {
                OutError = FString::Printf(TEXT("CONTENT_REFERENCE_INVALID:%s.faction"), *Path);
            }
            return false;
        }
        const TSharedPtr<FJsonObject>* Attack = nullptr;
        if (Object->TryGetObjectField(TEXT("attack"), Attack) && Attack != nullptr && Attack->IsValid())
        {
            if (!ReadRequiredInteger(*Attack, TEXT("damage"), Unit.AttackDamage, Path + TEXT(".attack"), OutError, 1) ||
                !ReadRequiredInteger(*Attack, TEXT("range_cm"), Unit.AttackRangeCentimeters, Path + TEXT(".attack"), OutError) ||
                !ReadRequiredInteger(*Attack, TEXT("cooldown_ticks"), Unit.AttackCooldownTicks, Path + TEXT(".attack"), OutError, 1))
            {
                return false;
            }
        }
        const TSharedPtr<FJsonObject>* Deployment = nullptr;
        if (Object->TryGetObjectField(TEXT("deployment"), Deployment) &&
            Deployment != nullptr && Deployment->IsValid())
        {
            if (!ReadRequiredInteger(*Deployment, TEXT("cover_depth_cm"), Unit.DeploymentCoverDepthCentimeters, Path + TEXT(".deployment"), OutError, 1) ||
                !ReadRequiredInteger(*Deployment, TEXT("cover_half_width_cm"), Unit.DeploymentCoverHalfWidthCentimeters, Path + TEXT(".deployment"), OutError, 1) ||
                !ReadRequiredInteger(*Deployment, TEXT("damage_reduction_percent"), Unit.DeploymentDamageReductionPercent, Path + TEXT(".deployment"), OutError, 1) ||
                !ReadRequiredInteger(*Deployment, TEXT("move_speed_percent"), Unit.DeploymentMoveSpeedPercent, Path + TEXT(".deployment"), OutError, 1))
            {
                return false;
            }
        }
        OutCatalog.Units.Add(MoveTemp(Unit));
    }

    TSet<FString> BuildingIds;
    for (int32 Index = 0; Index < BuildingValues->Num(); ++Index)
    {
        const FString Path = FString::Printf(TEXT("buildings[%d]"), Index);
        const TSharedPtr<FJsonObject> Object = (*BuildingValues)[Index]->AsObject();
        FEchoesBuildingContent Building;
        if (!ReadRequiredString(Object, TEXT("id"), Building.Id, Path, OutError) ||
            !ReadRequiredString(Object, TEXT("display_name"), Building.DisplayName, Path, OutError) ||
            !ReadRequiredString(Object, TEXT("faction"), Building.FactionId, Path, OutError) ||
            !ReadRequiredString(Object, TEXT("role"), Building.Role, Path, OutError) ||
            !FactionIds.Contains(Building.FactionId) ||
            !ValidateUniqueId(Building.Id, BuildingIds, Path, OutError) ||
            !ReadCost(Object, Building.MatterCost, Building.DawnCost, Path, OutError) ||
            !ReadRequiredInteger(Object, TEXT("max_health"), Building.MaxHealth, Path, OutError, 1) ||
            !ReadRequiredInteger(Object, TEXT("sight_cm"), Building.SightCentimeters, Path, OutError, 1) ||
            !ReadRequiredInteger(Object, TEXT("construction_ticks"), Building.ConstructionTicks, Path, OutError, 1) ||
            !ReadRequiredInteger(Object, TEXT("logistics_capacity"), Building.LogisticsCapacity, Path, OutError))
        {
            if (OutError.IsEmpty())
            {
                OutError = FString::Printf(TEXT("CONTENT_REFERENCE_INVALID:%s.faction"), *Path);
            }
            return false;
        }
        const TArray<TSharedPtr<FJsonValue>>* Footprint = nullptr;
        if (!Object->TryGetArrayField(TEXT("footprint_cells"), Footprint) ||
            Footprint == nullptr || Footprint->Num() != 2 ||
            !(*Footprint)[0].IsValid() || !(*Footprint)[1].IsValid())
        {
            OutError = FString::Printf(TEXT("CONTENT_FIELD_INVALID:%s.footprint_cells"), *Path);
            return false;
        }
        Building.FootprintCells.X = static_cast<int32>((*Footprint)[0]->AsNumber());
        Building.FootprintCells.Y = static_cast<int32>((*Footprint)[1]->AsNumber());
        if (Building.FootprintCells.X <= 0 || Building.FootprintCells.Y <= 0)
        {
            OutError = FString::Printf(TEXT("CONTENT_FIELD_INVALID:%s.footprint_cells"), *Path);
            return false;
        }
        OutCatalog.Buildings.Add(MoveTemp(Building));
    }

    const TSharedPtr<FJsonObject>* FutureWell = nullptr;
    if (!Root->TryGetObjectField(TEXT("future_wells"), FutureWell) ||
        FutureWell == nullptr || !FutureWell->IsValid())
    {
        OutError = TEXT("CONTENT_FUTURE_WELL_MISSING");
        return false;
    }
    const TSharedPtr<FJsonObject>* Harvest = nullptr;
    const TSharedPtr<FJsonObject>* Preserve = nullptr;
    const TSharedPtr<FJsonObject>* Reshape = nullptr;
    if (!ReadRequiredInteger(*FutureWell, TEXT("capture_radius_cm"), OutCatalog.FutureWell.CaptureRadiusCentimeters, TEXT("future_wells"), OutError, 1) ||
        !ReadRequiredInteger(*FutureWell, TEXT("capture_ticks"), OutCatalog.FutureWell.CaptureTicks, TEXT("future_wells"), OutError, 1) ||
        !(*FutureWell)->TryGetObjectField(TEXT("harvest"), Harvest) || Harvest == nullptr || !Harvest->IsValid() ||
        !(*FutureWell)->TryGetObjectField(TEXT("preserve"), Preserve) || Preserve == nullptr || !Preserve->IsValid() ||
        !(*FutureWell)->TryGetObjectField(TEXT("reshape"), Reshape) || Reshape == nullptr || !Reshape->IsValid() ||
        !ReadRequiredInteger(*Harvest, TEXT("immediate_dawn"), OutCatalog.FutureWell.HarvestImmediateDawn, TEXT("future_wells.harvest"), OutError, 1) ||
        !ReadRequiredInteger(*Harvest, TEXT("telegraph_ticks"), OutCatalog.FutureWell.HarvestTelegraphTicks, TEXT("future_wells.harvest"), OutError, 1) ||
        !ReadRequiredInteger(*Preserve, TEXT("dawn_per_interval"), OutCatalog.FutureWell.PreserveDawnPerInterval, TEXT("future_wells.preserve"), OutError, 1) ||
        !ReadRequiredInteger(*Preserve, TEXT("interval_ticks"), OutCatalog.FutureWell.PreserveIntervalTicks, TEXT("future_wells.preserve"), OutError, 1) ||
        !ReadRequiredInteger(*Preserve, TEXT("vision_radius_cm"), OutCatalog.FutureWell.PreserveVisionRadiusCentimeters, TEXT("future_wells.preserve"), OutError, 1) ||
        !ReadRequiredInteger(*Reshape, TEXT("dawn_cost"), OutCatalog.FutureWell.ReshapeDawnCost, TEXT("future_wells.reshape"), OutError, 1) ||
        !ReadRequiredInteger(*Reshape, TEXT("manifest_duration_ticks"), OutCatalog.FutureWell.ReshapeManifestDurationTicks, TEXT("future_wells.reshape"), OutError, 1) ||
        !ReadRequiredInteger(*Reshape, TEXT("telegraph_ticks"), OutCatalog.FutureWell.ReshapeTelegraphTicks, TEXT("future_wells.reshape"), OutError, 1))
    {
        if (OutError.IsEmpty())
        {
            OutError = TEXT("CONTENT_FUTURE_WELL_INVALID");
        }
        return false;
    }

    OutCatalog.Sha256 = ActualDigest;
    if (OutCatalog.PlayableFactionCount() < 2)
    {
        OutError = TEXT("CONTENT_PLAYABLE_FACTIONS_INSUFFICIENT");
        OutCatalog = {};
        return false;
    }
    for (const FEchoesFactionContent& Faction : OutCatalog.Factions)
    {
        if (!Faction.bVerticalSlicePlayable)
        {
            continue;
        }
        int32 UnitCount = 0;
        int32 BuildingCount = 0;
        bool bHasWorker = false;
        bool bHasHeadquarters = false;
        bool bHasLogistics = false;
        bool bHasProduction = false;
        TSet<FString> UnitRoles;
        TSet<FString> BuildingRoles;
        for (const FEchoesUnitContent& Unit : OutCatalog.Units)
        {
            if (Unit.FactionId == Faction.Id)
            {
                ++UnitCount;
                UnitRoles.Add(Unit.Role);
                bHasWorker |= Unit.Role == TEXT("worker");
            }
        }
        for (const FEchoesBuildingContent& Building : OutCatalog.Buildings)
        {
            if (Building.FactionId == Faction.Id)
            {
                ++BuildingCount;
                BuildingRoles.Add(Building.Role);
                bHasHeadquarters |= Building.Role == TEXT("headquarters_dropoff");
                bHasLogistics |= Building.Role == TEXT("supply_node") ||
                    Building.Role == TEXT("mobile_supply_node");
                bHasProduction |= Building.Role == TEXT("production");
            }
        }
        if (UnitCount < 4 || UnitRoles.Num() < 4 || !bHasWorker ||
            BuildingCount < 4 || BuildingRoles.Num() < 4 ||
            !bHasHeadquarters || !bHasLogistics || !bHasProduction)
        {
            OutError = FString::Printf(
                TEXT("CONTENT_PLAYABLE_ROSTER_INCOMPLETE:%s"),
                *Faction.Id);
            OutCatalog = {};
            return false;
        }
    }
    return true;
}

FString UEchoesContentSubsystem::GetCanonicalPackPath()
{
    return FPaths::Combine(
        FPaths::ProjectContentDir(),
        TEXT("Data/Generated/EchoesContentPack.json"));
}

FString UEchoesContentSubsystem::GetCanonicalDigestPath()
{
    return GetCanonicalPackPath() + TEXT(".sha256");
}

void UEchoesContentSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    bReady = FEchoesContentCatalog::LoadCanonicalPack(
        GetCanonicalPackPath(),
        GetCanonicalDigestPath(),
        Catalog,
        FailureReason);
    if (!bReady)
    {
        UE_LOG(
            LogEchoes,
            Error,
            TEXT("[ECHOES_CONTENT_FAILED] reason=%s"),
            *FailureReason);
        return;
    }
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_CONTENT_READY] packVersion=%d schema=%d factions=%d playable=%d units=%d buildings=%d sha256=%s source=canonical"),
        Catalog.PackVersion,
        Catalog.SchemaVersion,
        Catalog.Factions.Num(),
        Catalog.PlayableFactionCount(),
        Catalog.Units.Num(),
        Catalog.Buildings.Num(),
        *Catalog.Sha256);
}

void UEchoesContentSubsystem::Deinitialize()
{
    bReady = false;
    FailureReason.Reset();
    Catalog = {};
    Super::Deinitialize();
}
