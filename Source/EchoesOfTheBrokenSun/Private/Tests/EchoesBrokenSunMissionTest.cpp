#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "EchoesBrokenSunMissionModel.h"
#include "EchoesCampaignProgress.h"
#include "EchoesPlayerController.h"
#include "EchoesSnapshotMigrationTestHelpers.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Tests/AutomationCommon.h"

namespace
{
struct FPreservedBrokenSunFile final
{
    explicit FPreservedBrokenSunFile(FString InPath)
        : Path(MoveTemp(InPath))
    {
        bExisted = IFileManager::Get().FileExists(*Path);
        if (bExisted)
        {
            FFileHelper::LoadFileToArray(Contents, *Path);
        }
    }

    ~FPreservedBrokenSunFile()
    {
        IFileManager::Get().Delete(*Path, false, true, true);
        if (bExisted)
        {
            FFileHelper::SaveArrayToFile(Contents, *Path);
        }
    }

    FString Path;
    TArray<uint8> Contents;
    bool bExisted = false;
};

uint8 BrokenSunWellMask(echoes::sim::FutureWellChoice Choice)
{
    switch (Choice)
    {
        case echoes::sim::FutureWellChoice::Harvest: return 1 << 0;
        case echoes::sim::FutureWellChoice::Preserve: return 1 << 1;
        case echoes::sim::FutureWellChoice::Reshape: return 1 << 2;
        default: return 0;
    }
}

FEchoesCampaignDecisionRecord MakeBrokenSunPrerequisiteRecord(
    EEchoesCampaignMissionId Mission,
    echoes::sim::FutureWellChoice FoundingChoice,
    echoes::sim::FutureWellChoice LumeChoice,
    uint8 ReserveFacts = 0x7B)
{
    FEchoesCampaignDecisionRecord Record;
    Record.Mission = Mission;
    const bool bLumeReceipt =
        Mission == EEchoesCampaignMissionId::ChoirAtLumeReach ||
        Mission == EEchoesCampaignMissionId::NoNeutralLedger ||
        Mission == EEchoesCampaignMissionId::TheFutureThatWon ||
        Mission == EEchoesCampaignMissionId::AssemblyOfTheMissing ||
        Mission == EEchoesCampaignMissionId::SeveralVoicesOneCommand;
    Record.WellChoice = bLumeReceipt ? LumeChoice : FoundingChoice;
    Record.AvailableWellChoices =
        Mission == EEchoesCampaignMissionId::WhatTheLedgerKeeps ||
                Mission == EEchoesCampaignMissionId::ChoirAtLumeReach
            ? 0x07
        : bLumeReceipt
            ? BrokenSunWellMask(LumeChoice)
            : BrokenSunWellMask(FoundingChoice);
    Record.SimulationSnapshotVersion = echoes::sim::kSnapshotVersion;
    Record.CompletionTick = 1000 + static_cast<uint8>(Mission) * 400;
    Record.FinalStateChecksum =
        0xB15C0000ULL + static_cast<uint8>(Mission);
    switch (Mission)
    {
        case EEchoesCampaignMissionId::WhatTheLedgerKeeps:
        case EEchoesCampaignMissionId::SevenAccountsOfRain:
            Record.VerifiedFacts = 0x0F;
            break;
        case EEchoesCampaignMissionId::ACityOnReserve:
        case EEchoesCampaignMissionId::TheUnburiedRoad:
            Record.VerifiedFacts = 0x1F;
            break;
        case EEchoesCampaignMissionId::TermsOfContinuance:
        case EEchoesCampaignMissionId::NamesWithoutBirths:
        case EEchoesCampaignMissionId::TheShapeOfSilence:
        case EEchoesCampaignMissionId::TheShapeBesideUs:
            Record.VerifiedFacts = 0x3F;
            break;
        case EEchoesCampaignMissionId::ReserveAuthority:
            Record.VerifiedFacts = ReserveFacts;
            break;
        case EEchoesCampaignMissionId::ChoirAtLumeReach:
        case EEchoesCampaignMissionId::NoNeutralLedger:
        case EEchoesCampaignMissionId::TheFutureThatWon:
        case EEchoesCampaignMissionId::AssemblyOfTheMissing:
        case EEchoesCampaignMissionId::SeveralVoicesOneCommand:
        case EEchoesCampaignMissionId::TheBrokenSun:
            Record.VerifiedFacts = 0xFF;
            break;
    }
    return Record;
}

FEchoesCampaignProgress MakeBrokenSunPrerequisites(
    echoes::sim::FutureWellChoice FoundingChoice,
    echoes::sim::FutureWellChoice LumeChoice,
    uint8 ReserveFacts,
    int32 ThroughMission,
    FString& OutFeedback)
{
    FEchoesCampaignProgress Progress;
    for (int32 MissionValue = 1;
         MissionValue <= FMath::Min(ThroughMission, 14);
         ++MissionValue)
    {
        Progress.AppendDecision(
            MakeBrokenSunPrerequisiteRecord(
                static_cast<EEchoesCampaignMissionId>(MissionValue),
                FoundingChoice,
                LumeChoice,
                ReserveFacts),
            OutFeedback);
    }
    return Progress;
}

FEchoesCampaignDecisionRecord MakeBrokenSunFinalRecord(
    const FEchoesBrokenSunPlan& Plan,
    EEchoesFinalResolution Resolution)
{
    FEchoesCampaignDecisionRecord Record;
    Record.Mission = EEchoesCampaignMissionId::TheBrokenSun;
    Record.WellChoice = Plan.RecordedProtocol;
    Record.AvailableWellChoices = BrokenSunWellMask(Plan.RecordedProtocol);
    Record.VerifiedFacts = 0xFF;
    Record.FinalResolution = Resolution;
    Record.AvailableFinalResolutions = Plan.AvailableFinalResolutions;
    Record.FinalPlanKey = Plan.StablePlanKey;
    Record.SimulationSnapshotVersion = echoes::sim::kSnapshotVersion;
    Record.CompletionTick = 7600;
    Record.FinalStateChecksum = 0xB15CF1A1ULL;
    return Record;
}

void AppendLegacyU16(TArray<uint8>& Bytes, uint16 Value)
{
    Bytes.Add(static_cast<uint8>(Value));
    Bytes.Add(static_cast<uint8>(Value >> 8));
}

void AppendLegacyU32(TArray<uint8>& Bytes, uint32 Value)
{
    for (int32 ByteIndex = 0; ByteIndex < 4; ++ByteIndex)
    {
        Bytes.Add(static_cast<uint8>(Value >> (ByteIndex * 8)));
    }
}

void AppendLegacyU64(TArray<uint8>& Bytes, uint64 Value)
{
    for (int32 ByteIndex = 0; ByteIndex < 8; ++ByteIndex)
    {
        Bytes.Add(static_cast<uint8>(Value >> (ByteIndex * 8)));
    }
}

TArray<uint8> EncodeLegacyBrokenSunPrerequisites(
    const FEchoesCampaignProgress& Progress)
{
    static constexpr uint8 Magic[] = {
        'E', 'C', 'H', 'O', 'C', 'P', 'G', '1'};
    TArray<uint8> Bytes;
    Bytes.Append(Magic, UE_ARRAY_COUNT(Magic));
    AppendLegacyU16(Bytes, 1);
    AppendLegacyU16(
        Bytes,
        static_cast<uint16>(Progress.Decisions.Num()));
    for (const FEchoesCampaignDecisionRecord& Record : Progress.Decisions)
    {
        Bytes.Add(static_cast<uint8>(Record.Mission));
        Bytes.Add(static_cast<uint8>(Record.WellChoice));
        Bytes.Add(Record.AvailableWellChoices);
        Bytes.Add(Record.VerifiedFacts);
        AppendLegacyU32(Bytes, Record.SimulationSnapshotVersion);
        AppendLegacyU64(Bytes, Record.CompletionTick);
        AppendLegacyU64(Bytes, Record.FinalStateChecksum);
    }
    AppendLegacyU32(
        Bytes,
        FCrc::MemCrc32(Bytes.GetData(), Bytes.Num()));
    return Bytes;
}

FString BrokenSunQuickSavePath(
    const FEchoesCampaignProgress& PrerequisiteProgress)
{
    TArray<uint8> LedgerBytes;
    FString Error;
    if (!FEchoesCampaignProgressStore::Encode(
            PrerequisiteProgress,
            LedgerBytes,
            Error) ||
        LedgerBytes.Num() <= static_cast<int32>(sizeof(uint32)))
    {
        return {};
    }
    const uint32 Fingerprint = FCrc::MemCrc32(
        LedgerBytes.GetData(),
        LedgerBytes.Num() - static_cast<int32>(sizeof(uint32)));
    return FPaths::Combine(
        FEchoesCampaignProgressStore::GetSaveGameDirectory(),
        FString::Printf(
            TEXT("EchoesQuickSaveTheBrokenSun-%08X.bin"),
            Fingerprint));
}

echoes::sim::Vec2 TestOwnedBrokenSunApproachSite(
    echoes::sim::FutureWellChoice Protocol)
{
    using echoes::sim::FutureWellChoice;
    using echoes::sim::Vec2;
    switch (Protocol)
    {
        case FutureWellChoice::Harvest: return Vec2::FromTiles(18, 56);
        case FutureWellChoice::Preserve: return Vec2::FromTiles(32, 56);
        case FutureWellChoice::Reshape: return Vec2::FromTiles(32, 43);
        default: return {};
    }
}

echoes::sim::Vec2 TestOwnedBrokenSunResolutionSite(
    EEchoesFinalResolution Resolution)
{
    using echoes::sim::Vec2;
    switch (Resolution)
    {
        case EEchoesFinalResolution::Restoration:
            return Vec2::FromTiles(32, 49);
        case EEchoesFinalResolution::ControlledStabilization:
            return Vec2::FromTiles(32, 44);
        case EEchoesFinalResolution::Extinguishment:
            return Vec2::FromTiles(26, 49);
        case EEchoesFinalResolution::OpenEvolution:
            return Vec2::FromTiles(26, 54);
        case EEchoesFinalResolution::None:
            return {};
    }
    return {};
}

bool BrokenSunObjectiveDomainsAreDisjoint(
    const echoes::sim::Vec2& First,
    int32 FirstRadius,
    const echoes::sim::Vec2& Second,
    int32 SecondRadius)
{
    const int64 DeltaX = static_cast<int64>(First.x.Raw()) - Second.x.Raw();
    const int64 DeltaY = static_cast<int64>(First.y.Raw()) - Second.y.Raw();
    const int64 CombinedRadius =
        static_cast<int64>(FirstRadius + SecondRadius) *
        echoes::sim::kFixedScale;
    return DeltaX * DeltaX + DeltaY * DeltaY >
        CombinedRadius * CombinedRadius;
}

FString DescribeBrokenSunEntity(
    const TCHAR* Label,
    echoes::sim::EntityId Id,
    const echoes::sim::Entity* Current)
{
    if (Current == nullptr)
    {
        return FString::Printf(TEXT("%s{id=%u missing}"), Label, Id);
    }
    return FString::Printf(
        TEXT("%s{id=%u owner=%u faction=%u type=%u hp=%d/%d pos=(%d,%d) completed=%s progress=%d/%d order=%u target=%u anchor=(%d,%d) destination=(%d,%d)}"),
        Label,
        Id,
        Current->owner,
        static_cast<uint8>(Current->faction),
        static_cast<uint8>(Current->type),
        Current->hitPoints,
        Current->maxHitPoints,
        Current->position.x.FloorToInt(),
        Current->position.y.FloorToInt(),
        Current->completed ? TEXT("true") : TEXT("false"),
        Current->constructionProgress,
        Current->constructionRequired,
        static_cast<uint8>(Current->order.type),
        Current->order.target,
        Current->order.anchor.x.FloorToInt(),
        Current->order.anchor.y.FloorToInt(),
        Current->order.destination.x.FloorToInt(),
        Current->order.destination.y.FloorToInt());
}

struct FBrokenSunResolutionPersistenceSpec final
{
    const TCHAR* Label = TEXT("unavailable");
    echoes::sim::FutureWellChoice FoundingChoice =
        echoes::sim::FutureWellChoice::Dormant;
    echoes::sim::FutureWellChoice RecordedProtocol =
        echoes::sim::FutureWellChoice::Dormant;
    EEchoesFinalResolution SelectedResolution =
        EEchoesFinalResolution::None;
    uint8 ExpectedPlanKey = 0xFF;
    uint8 ExpectedAvailability = 0;
    echoes::sim::Vec2 ExpectedApproachSite{};
    echoes::sim::Vec2 ExpectedApproachBuildSite{};
    echoes::sim::Vec2 ExpectedResolutionSite{};
};

bool RunBrokenSunResolutionPersistenceCase(
    FAutomationTestBase& Test,
    const FBrokenSunResolutionPersistenceSpec& Spec)
{
    using echoes::sim::ChoirIdentityState;
    using echoes::sim::CommandType;
    using echoes::sim::EntityType;
    using echoes::sim::Faction;
    using echoes::sim::FutureWellChoice;
    using echoes::sim::ResearchType;
    using echoes::sim::Vec2;

    bool bPassed = true;
    const auto Check = [&Test, &Spec, &bPassed](
        const FString& Detail,
        bool bCondition)
    {
        if (!bCondition)
        {
            Test.AddError(FString::Printf(
                TEXT("%s: %s"),
                Spec.Label,
                *Detail));
            bPassed = false;
        }
        return bCondition;
    };

    FString Feedback;
    const FString CampaignPath =
        FEchoesCampaignProgressStore::GetDefaultPath();
    const FString CampaignBackupPath = CampaignPath + TEXT(".bak");
    const FString CampaignTemporaryPath = CampaignPath + TEXT(".tmp");
    FPreservedBrokenSunFile PreservedCampaign(CampaignPath);
    FPreservedBrokenSunFile PreservedCampaignBackup(CampaignBackupPath);
    FPreservedBrokenSunFile PreservedCampaignTemporary(
        CampaignTemporaryPath);
    for (const FString& Path : {
             CampaignPath,
             CampaignBackupPath,
             CampaignTemporaryPath})
    {
        IFileManager::Get().Delete(*Path, false, true, true);
    }
    if (!Check(
            TEXT("the isolated campaign store starts without residue"),
            !IFileManager::Get().FileExists(*CampaignPath) &&
                !IFileManager::Get().FileExists(*CampaignBackupPath) &&
                !IFileManager::Get().FileExists(*CampaignTemporaryPath)))
    {
        return false;
    }

    FEchoesBrokenSunPlan ExpectedPlan;
    if (!Check(
            TEXT("the exact inherited tuple projects a final plan"),
            FEchoesBrokenSunMissionModel::TryPlanForLedger(
                Spec.FoundingChoice,
                0x7B,
                Spec.RecordedProtocol,
                ExpectedPlan)) ||
        !Check(
            TEXT("the projected plan retains the expected choices, districts, key, and full eligibility mask"),
            ExpectedPlan.FoundingDoctrine == Spec.FoundingChoice &&
                ExpectedPlan.RecordedProtocol == Spec.RecordedProtocol &&
                ExpectedPlan.FirstContributingDistrict ==
                    EEchoesCityDistrict::LifeSupport &&
                ExpectedPlan.SecondContributingDistrict ==
                    EEchoesCityDistrict::Transit &&
                ExpectedPlan.DeferredDistrict ==
                    EEchoesCityDistrict::Archive &&
                ExpectedPlan.StablePlanKey == Spec.ExpectedPlanKey &&
                ExpectedPlan.AvailableFinalResolutions ==
                    Spec.ExpectedAvailability &&
                ExpectedPlan.CrownfallApproachSite ==
                    Spec.ExpectedApproachSite) ||
        !Check(
            TEXT("the selected resolution is one member of the complete eligibility set"),
            FEchoesBrokenSunMissionModel::IsResolutionAvailable(
                ExpectedPlan,
                Spec.SelectedResolution)))
    {
        return false;
    }

    FEchoesCampaignProgress Prerequisites = MakeBrokenSunPrerequisites(
        Spec.FoundingChoice,
        Spec.RecordedProtocol,
        0x7B,
        14,
        Feedback);
    FEchoesCampaignProgress ReloadedPrerequisites;
    if (!Check(
            TEXT("the synthetic prerequisite ledger is validator-accepted and contains exactly fourteen records"),
            Prerequisites.Decisions.Num() == 14 &&
                FEchoesCampaignProgressStore::SaveAtomic(
                    CampaignPath,
                    Prerequisites,
                    Feedback) &&
                FEchoesCampaignProgressStore::LoadWithBackup(
                    CampaignPath,
                    ReloadedPrerequisites,
                    Feedback) &&
                ReloadedPrerequisites.Decisions ==
                    Prerequisites.Decisions) ||
        !Check(
            TEXT("seeding the first generation creates no backup or temporary residue"),
            !IFileManager::Get().FileExists(*CampaignBackupPath) &&
                !IFileManager::Get().FileExists(*CampaignTemporaryPath)))
    {
        return false;
    }

    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(&Test);
        Check(TEXT("the Mission 15 test world can be created"), false);
        return false;
    }
    UEchoesSimulationSubsystem* Bridge =
        WorldWrapper.GetTestWorld()->GetSubsystem<
            UEchoesSimulationSubsystem>();
    bool bScenarioStarted = false;
    const auto FinishWorld = [&]()
    {
        if (Bridge != nullptr && bScenarioStarted)
        {
            Bridge->StopPrototypeScenario();
        }
        WorldWrapper.ForwardErrorMessages(&Test);
        return bPassed && !WorldWrapper.HasFailed();
    };

    if (!Check(
            TEXT("the test world owns a simulation subsystem"),
            Bridge != nullptr) ||
        !Check(
            TEXT("the exact fourteen-record ledger unlocks Mission 15"),
            Bridge != nullptr && Bridge->IsBrokenSunUnlocked()) ||
        !Check(
            TEXT("the Mission 15 operation can be selected"),
            Bridge->SelectOperationMode(
                EEchoesOperationMode::CampaignTheBrokenSun,
                Feedback)) ||
        !Check(
            TEXT("the Mission 15 operation can start"),
            Bridge->StartPrototypeScenario()))
    {
        return FinishWorld();
    }
    bScenarioStarted = true;

    const FEchoesBrokenSunPlan RuntimePlan = Bridge->GetBrokenSunPlan();
    if (!Check(
            TEXT("the runtime plan matches the exact inherited plan"),
            RuntimePlan.FoundingDoctrine == Spec.FoundingChoice &&
                RuntimePlan.RecordedProtocol == Spec.RecordedProtocol &&
                RuntimePlan.FirstContributingDistrict ==
                    EEchoesCityDistrict::LifeSupport &&
                RuntimePlan.SecondContributingDistrict ==
                    EEchoesCityDistrict::Transit &&
                RuntimePlan.DeferredDistrict ==
                    EEchoesCityDistrict::Archive &&
                RuntimePlan.StablePlanKey == Spec.ExpectedPlanKey &&
                RuntimePlan.AvailableFinalResolutions ==
                    Spec.ExpectedAvailability &&
                RuntimePlan.CrownfallApproachSite ==
                    Spec.ExpectedApproachSite))
    {
        return FinishWorld();
    }

    FEchoesObjectiveSnapshot Objective =
        Bridge->GetLocalObjectiveSnapshot();
    const echoes::sim::Entity* Voice =
        Bridge->FindEntity(Objective.BrokenSunAccordVoiceId);
    const echoes::sim::Entity* Heavy =
        Bridge->FindEntity(Objective.BrokenSunAccordHeavyId);
    const echoes::sim::Entity* Neme =
        Bridge->FindEntity(Objective.BrokenSunNemeId);
    const echoes::sim::Entity* Worker =
        Bridge->FindEntity(Objective.BrokenSunWorkerId);
    if (!Check(
            TEXT("the runtime exposes the required Hollow Choir command force"),
            Voice != nullptr && Heavy != nullptr && Neme != nullptr &&
                Worker != nullptr &&
                Voice->owner ==
                    UEchoesSimulationSubsystem::LocalPlayerId &&
                Heavy->owner ==
                    UEchoesSimulationSubsystem::LocalPlayerId &&
                Neme->owner ==
                    UEchoesSimulationSubsystem::LocalPlayerId &&
                Worker->owner ==
                    UEchoesSimulationSubsystem::LocalPlayerId &&
                Voice->faction == Faction::HollowChoir &&
                Heavy->faction == Faction::HollowChoir &&
                Neme->faction == Faction::HollowChoir &&
                Worker->faction == Faction::HollowChoir))
    {
        return FinishWorld();
    }

    echoes::sim::EntityId ResearchLoomId = 0;
    for (const echoes::sim::Entity& Entity :
         Bridge->GetSimulation()->Entities())
    {
        if (Entity.owner ==
                UEchoesSimulationSubsystem::LocalPlayerId &&
            Entity.faction == Faction::HollowChoir &&
            Entity.type == EntityType::Barracks)
        {
            ResearchLoomId = Entity.id;
            break;
        }
    }
    if (!Check(
            TEXT("the final command force includes a Research Loom"),
            ResearchLoomId != 0))
    {
        return FinishWorld();
    }

    const auto TickUntil = [Bridge](
        const TFunction<bool()>& Predicate,
        int32 MaximumTicks)
    {
        for (int32 TickIndex = 0; TickIndex < MaximumTicks; ++TickIndex)
        {
            if (Predicate())
            {
                return true;
            }
            Bridge->Tick(0.05f);
        }
        return Predicate();
    };
    const auto FindBuildSite = [Bridge](
        const Vec2& Center,
        int32 Radius,
        Vec2& OutSite)
    {
        const echoes::sim::Simulation* Simulation =
            Bridge->GetSimulation();
        if (Simulation == nullptr)
        {
            return false;
        }
        for (int32 DeltaY = -Radius; DeltaY <= Radius; ++DeltaY)
        {
            for (int32 DeltaX = -Radius; DeltaX <= Radius; ++DeltaX)
            {
                if (DeltaX * DeltaX + DeltaY * DeltaY >
                    Radius * Radius)
                {
                    continue;
                }
                const Vec2 Candidate = Vec2::FromTiles(
                    Center.x.FloorToInt() + DeltaX,
                    Center.y.FloorToInt() + DeltaY);
                if (Simulation->ValidatePlacement(
                        UEchoesSimulationSubsystem::LocalPlayerId,
                        EntityType::UtilityStructure,
                        Candidate) ==
                    echoes::sim::PlacementResult::Valid)
                {
                    OutSite = Candidate;
                    return true;
                }
            }
        }
        return false;
    };
    const auto IsWithinContractRadius = [](
        const Vec2& Position,
        const Vec2& Site,
        int32 Radius)
    {
        const int64 DeltaX = static_cast<int64>(Position.x.Raw()) -
            Site.x.Raw();
        const int64 DeltaY = static_cast<int64>(Position.y.Raw()) -
            Site.y.Raw();
        const int64 RadiusRaw = Vec2::FromTiles(Radius, 0).x.Raw();
        return DeltaX * DeltaX + DeltaY * DeltaY <=
            RadiusRaw * RadiusRaw;
    };

    Bridge->SetScenarioPaused(false);
    Vec2 ApproachBuildSite;
    if (!Check(
            TEXT("the literal approach center is open in the authored Lume terrain"),
            Bridge->GetSimulation()->TerrainAt(
                Spec.ExpectedApproachSite.x.FloorToInt(),
                Spec.ExpectedApproachSite.y.FloorToInt()) ==
                echoes::sim::Terrain::Open) ||
        !Check(
            TEXT("the approach exposes a valid construction footprint"),
            FindBuildSite(
                Spec.ExpectedApproachSite,
                3,
                ApproachBuildSite)) ||
        !Check(
            FString::Printf(
                TEXT("the plan-specific deterministic approach probe matches independent literal (%d,%d); observed (%d,%d)"),
                Spec.ExpectedApproachBuildSite.x.FloorToInt(),
                Spec.ExpectedApproachBuildSite.y.FloorToInt(),
                ApproachBuildSite.x.FloorToInt(),
                ApproachBuildSite.y.FloorToInt()),
            Spec.ExpectedApproachBuildSite == Vec2{} ||
                ApproachBuildSite == Spec.ExpectedApproachBuildSite))
    {
        return FinishWorld();
    }

    const auto ApproachBuildSequence =
        Bridge->GetSimulation()->NextCommandSequence(
            UEchoesSimulationSubsystem::LocalPlayerId);
    if (!Check(
            TEXT("the approach build has a stable command sequence"),
            ApproachBuildSequence.has_value()) ||
        !Check(
            TEXT("the worker accepts the approach-anchor build"),
            Bridge->IssueBuildCommand(
                Objective.BrokenSunWorkerId,
                EntityType::UtilityStructure,
                Bridge->SimToWorld(ApproachBuildSite),
                Feedback)))
    {
        return FinishWorld();
    }

    echoes::sim::EntityId ObservedApproachId = 0;
    bool bReceiptObserved = false;
    bool bReceiptApplied = false;
    int32 ReceiptOutcome = -1;
    bool bSiteCreated = false;
    bool bConstructionProgressed = false;
    bool bSiteCompleted = false;
    bool bApproachBound = false;
    bool bAssembleAccordReached = false;
    uint64 LastSeenSiteTick = 0;
    int32 LastSeenSiteHitPoints = 0;
    int32 LastSeenSiteMaxHitPoints = 0;
    int32 LastSeenConstructionProgress = 0;
    int32 LastSeenConstructionRequired = 0;
    bool bLastSeenSiteCompleted = false;
    bool bSiteDisappeared = false;
    uint64 SiteDisappearanceTick = 0;
    const auto ObserveApproach = [&]()
    {
        if (ApproachBuildSequence.has_value())
        {
            const auto Receipt =
                Bridge->GetSimulation()->FindCommandResolutionReceipt(
                    UEchoesSimulationSubsystem::LocalPlayerId,
                    *ApproachBuildSequence);
            if (Receipt.has_value())
            {
                bReceiptObserved = true;
                ReceiptOutcome = static_cast<int32>(Receipt->outcome);
                bReceiptApplied =
                    Receipt->outcome ==
                    echoes::sim::CommandResolutionOutcome::Applied;
            }
        }

        const echoes::sim::Entity* Site = ObservedApproachId != 0
            ? Bridge->FindEntity(ObservedApproachId)
            : nullptr;
        if (Site == nullptr)
        {
            for (const echoes::sim::Entity& Candidate :
                 Bridge->GetSimulation()->Entities())
            {
                if (Candidate.owner ==
                        UEchoesSimulationSubsystem::LocalPlayerId &&
                    Candidate.faction == Faction::HollowChoir &&
                    Candidate.type == EntityType::UtilityStructure &&
                    Candidate.position == ApproachBuildSite)
                {
                    ObservedApproachId = Candidate.id;
                    Site = &Candidate;
                    break;
                }
            }
        }
        if (Site != nullptr)
        {
            bSiteCreated = true;
            bConstructionProgressed = bConstructionProgressed ||
                Site->constructionProgress > 0 || Site->completed;
            bSiteCompleted = bSiteCompleted ||
                (Site->completed && Site->hitPoints > 0);
            LastSeenSiteTick =
                Bridge->GetSimulation()->CurrentTick();
            LastSeenSiteHitPoints = Site->hitPoints;
            LastSeenSiteMaxHitPoints = Site->maxHitPoints;
            LastSeenConstructionProgress = Site->constructionProgress;
            LastSeenConstructionRequired = Site->constructionRequired;
            bLastSeenSiteCompleted = Site->completed;
        }
        else if (bSiteCreated && !bSiteDisappeared)
        {
            bSiteDisappeared = true;
            SiteDisappearanceTick =
                Bridge->GetSimulation()->CurrentTick();
        }

        const FEchoesObjectiveSnapshot Current =
            Bridge->GetLocalObjectiveSnapshot();
        bApproachBound = bApproachBound ||
            (Current.bBrokenSunApproachSecured &&
             Current.BrokenSunApproachAnchorId != 0);
        bAssembleAccordReached = bAssembleAccordReached ||
            Current.BrokenSunPhase ==
                EEchoesBrokenSunPhase::AssembleAccord;
    };
    for (int32 TickIndex = 0;
         TickIndex < 4000 && !bAssembleAccordReached;
         ++TickIndex)
    {
        ObserveApproach();
        if (Bridge->GetBrokenSunPhase() ==
            EEchoesBrokenSunPhase::Failed)
        {
            break;
        }
        if (!bAssembleAccordReached)
        {
            Bridge->Tick(0.05f);
        }
    }
    ObserveApproach();

    Objective = Bridge->GetLocalObjectiveSnapshot();
    const echoes::sim::Entity* CurrentWorker =
        Bridge->FindEntity(Objective.BrokenSunWorkerId);
    const echoes::sim::Entity* CurrentSite = ObservedApproachId != 0
        ? Bridge->FindEntity(ObservedApproachId)
        : nullptr;
    const echoes::sim::Entity* BoundSite =
        Objective.BrokenSunApproachAnchorId != 0
            ? Bridge->FindEntity(Objective.BrokenSunApproachAnchorId)
            : nullptr;
    const echoes::sim::Entity* NearestOpponent = nullptr;
    int64 NearestOpponentDistanceSquaredRaw = 0;
    for (const echoes::sim::Entity& Candidate :
         Bridge->GetSimulation()->Entities())
    {
        if (Candidate.owner !=
            UEchoesSimulationSubsystem::OpponentPlayerId)
        {
            continue;
        }
        const int64 DeltaX =
            static_cast<int64>(Candidate.position.x.Raw()) -
            ApproachBuildSite.x.Raw();
        const int64 DeltaY =
            static_cast<int64>(Candidate.position.y.Raw()) -
            ApproachBuildSite.y.Raw();
        const int64 DistanceSquared =
            DeltaX * DeltaX + DeltaY * DeltaY;
        if (NearestOpponent == nullptr ||
            DistanceSquared < NearestOpponentDistanceSquaredRaw)
        {
            NearestOpponent = &Candidate;
            NearestOpponentDistanceSquaredRaw = DistanceSquared;
        }
    }
    const FString ApproachDiagnostic = FString::Printf(
        TEXT("[M15_APPROACH_DIAGNOSTIC] tick=%llu phase=%u outcome=%u planKey=%u center=(%d,%d) build=(%d,%d) sequence=%s receipt={seen=%s applied=%s outcome=%d} observed={siteId=%u created=%s progressed=%s completed=%s bound=%s assemble=%s objectiveAnchor=%u secured=%s} lastSeen={tick=%llu hp=%d/%d progress=%d/%d completed=%s disappeared=%s disappearanceTick=%llu} nearestOpponentDistanceSquaredRaw=%lld %s %s %s"),
        static_cast<unsigned long long>(
            Bridge->GetSimulation()->CurrentTick()),
        static_cast<uint8>(Objective.BrokenSunPhase),
        static_cast<uint8>(Objective.Outcome),
        RuntimePlan.StablePlanKey,
        Spec.ExpectedApproachSite.x.FloorToInt(),
        Spec.ExpectedApproachSite.y.FloorToInt(),
        ApproachBuildSite.x.FloorToInt(),
        ApproachBuildSite.y.FloorToInt(),
        ApproachBuildSequence.has_value() ? TEXT("present") : TEXT("missing"),
        bReceiptObserved ? TEXT("true") : TEXT("false"),
        bReceiptApplied ? TEXT("true") : TEXT("false"),
        ReceiptOutcome,
        ObservedApproachId,
        bSiteCreated ? TEXT("true") : TEXT("false"),
        bConstructionProgressed ? TEXT("true") : TEXT("false"),
        bSiteCompleted ? TEXT("true") : TEXT("false"),
        bApproachBound ? TEXT("true") : TEXT("false"),
        bAssembleAccordReached ? TEXT("true") : TEXT("false"),
        Objective.BrokenSunApproachAnchorId,
        Objective.bBrokenSunApproachSecured ? TEXT("true") : TEXT("false"),
        static_cast<unsigned long long>(LastSeenSiteTick),
        LastSeenSiteHitPoints,
        LastSeenSiteMaxHitPoints,
        LastSeenConstructionProgress,
        LastSeenConstructionRequired,
        bLastSeenSiteCompleted ? TEXT("true") : TEXT("false"),
        bSiteDisappeared ? TEXT("true") : TEXT("false"),
        static_cast<unsigned long long>(SiteDisappearanceTick),
        static_cast<long long>(NearestOpponentDistanceSquaredRaw),
        *DescribeBrokenSunEntity(
            TEXT("worker"),
            Objective.BrokenSunWorkerId,
            CurrentWorker),
        *DescribeBrokenSunEntity(
            TEXT("site"),
            ObservedApproachId,
            CurrentSite),
        *DescribeBrokenSunEntity(
            TEXT("nearestOpponent"),
            NearestOpponent != nullptr ? NearestOpponent->id : 0,
            NearestOpponent));
    bool bApproachContractPassed = true;
    bApproachContractPassed = Check(
        TEXT("the admitted approach build resolves as applied"),
        bReceiptObserved && bReceiptApplied) &&
        bApproachContractPassed;
    bApproachContractPassed = Check(
        TEXT("the applied build creates the exact owned approach site"),
        bSiteCreated && ObservedApproachId != 0) &&
        bApproachContractPassed;
    bApproachContractPassed = Check(
        TEXT("the approach worker makes construction progress"),
        bConstructionProgressed) && bApproachContractPassed;
    bApproachContractPassed = Check(
        TEXT("the exact approach structure completes intact"),
        bSiteCompleted) && bApproachContractPassed;
    bApproachContractPassed = Check(
        FString::Printf(
            TEXT("the completed approach advances to accord assembly — %s"),
            *ApproachDiagnostic),
        bAssembleAccordReached && bApproachBound &&
            Objective.BrokenSunPhase ==
                EEchoesBrokenSunPhase::AssembleAccord &&
            Objective.bBrokenSunApproachSecured &&
            Objective.BrokenSunApproachAnchorId == ObservedApproachId &&
            BoundSite != nullptr && BoundSite->completed &&
            BoundSite->hitPoints > 0 &&
            BoundSite->position == ApproachBuildSite &&
            IsWithinContractRadius(
                BoundSite->position,
                Spec.ExpectedApproachSite,
                3)) && bApproachContractPassed;
    if (!bApproachContractPassed)
    {
        return FinishWorld();
    }

    if (!Check(
            TEXT("the Research Loom accepts Held Alternatives"),
            Bridge->IssueResearchCommand(
                ResearchLoomId,
                ResearchType::ChoirHeldAlternatives,
                Feedback)) ||
        !Check(
            TEXT("Held Alternatives completes deterministically"),
            TickUntil(
                [Bridge]()
                {
                    const echoes::sim::PlayerState* Player =
                        Bridge->GetSimulation()->FindPlayer(
                            UEchoesSimulationSubsystem::LocalPlayerId);
                    return Player != nullptr &&
                        Player->HasCompletedResearch(
                            ResearchType::ChoirHeldAlternatives);
                },
                1200)) ||
        !Check(
            TEXT("the accord voice accepts the Possible state"),
            Bridge->IssueChoirReconciliation(
                Objective.BrokenSunAccordVoiceId,
                ChoirIdentityState::Possible,
                Feedback)) ||
        !Check(
            TEXT("the Possible voice accepts the Mara accord move"),
            Bridge->IssueCommand(
                CommandType::Move,
                Objective.BrokenSunAccordVoiceId,
                0,
                Bridge->SimToWorld(RuntimePlan.MaraAccordSite),
                FutureWellChoice::Dormant,
                Feedback)) ||
        !Check(
            TEXT("the Manifest heavy accepts the Oruun accord move"),
            Bridge->IssueCommand(
                CommandType::Move,
                Objective.BrokenSunAccordHeavyId,
                0,
                Bridge->SimToWorld(RuntimePlan.OruunAccordSite),
                FutureWellChoice::Dormant,
                Feedback)) ||
        !Check(
            TEXT("Neme accepts the Choir accord move"),
            Bridge->IssueCommand(
                CommandType::Move,
                Objective.BrokenSunNemeId,
                0,
                Bridge->SimToWorld(RuntimePlan.NemeAccordSite),
                FutureWellChoice::Dormant,
                Feedback)) ||
        !Check(
            TEXT("all three accord participants settle at their exact sites"),
            TickUntil(
                [Bridge, RuntimePlan, IsWithinContractRadius]()
                {
                    const FEchoesObjectiveSnapshot Current =
                        Bridge->GetLocalObjectiveSnapshot();
                    const echoes::sim::Entity* CurrentVoice =
                        Bridge->FindEntity(
                            Current.BrokenSunAccordVoiceId);
                    const echoes::sim::Entity* CurrentHeavy =
                        Bridge->FindEntity(
                            Current.BrokenSunAccordHeavyId);
                    const echoes::sim::Entity* CurrentNeme =
                        Bridge->FindEntity(Current.BrokenSunNemeId);
                    return CurrentVoice != nullptr &&
                        CurrentHeavy != nullptr &&
                        CurrentNeme != nullptr &&
                        CurrentVoice->choirIdentityState ==
                            ChoirIdentityState::Possible &&
                        CurrentHeavy->choirIdentityState ==
                            ChoirIdentityState::Manifest &&
                        IsWithinContractRadius(
                            CurrentVoice->position,
                            RuntimePlan.MaraAccordSite,
                            3) &&
                        IsWithinContractRadius(
                            CurrentHeavy->position,
                            RuntimePlan.OruunAccordSite,
                            3) &&
                        IsWithinContractRadius(
                            CurrentNeme->position,
                            RuntimePlan.NemeAccordSite,
                            3);
                },
                3000)) ||
        !Check(
            TEXT("the Research Loom accepts Shared Resolution"),
            Bridge->IssueResearchCommand(
                ResearchLoomId,
                ResearchType::ChoirSharedResolution,
                Feedback)) ||
        !Check(
            TEXT("the witnessed accord opens final-resolution selection"),
            TickUntil(
                [Bridge]()
                {
                    return Bridge->GetBrokenSunPhase() ==
                        EEchoesBrokenSunPhase::ChooseFinalResolution;
                },
                5000)))
    {
        return FinishWorld();
    }

    Objective = Bridge->GetLocalObjectiveSnapshot();
    if (!Check(
            TEXT("the choice snapshot exposes the full exact eligibility set and no selected ending"),
            Objective.BrokenSunAvailableFinalResolutions ==
                    Spec.ExpectedAvailability &&
                Objective.BrokenSunPendingFinalResolution ==
                    EEchoesFinalResolution::None &&
                Objective.BrokenSunFinalResolution ==
                    EEchoesFinalResolution::None) ||
        !Check(
            TEXT("the first press arms only the requested eligible ending"),
            Bridge->ChooseFinalResolution(
                Spec.SelectedResolution,
                Feedback)))
    {
        return FinishWorld();
    }
    Objective = Bridge->GetLocalObjectiveSnapshot();
    if (!Check(
            TEXT("the armed state separates the pending selection from the complete eligibility set"),
            Objective.BrokenSunAvailableFinalResolutions ==
                    Spec.ExpectedAvailability &&
                Objective.BrokenSunPendingFinalResolution ==
                    Spec.SelectedResolution &&
                Objective.BrokenSunFinalResolution ==
                    EEchoesFinalResolution::None) ||
        !Check(
            TEXT("the second identical press locks the requested ending"),
            Bridge->ChooseFinalResolution(
                Spec.SelectedResolution,
                Feedback)))
    {
        return FinishWorld();
    }
    Objective = Bridge->GetLocalObjectiveSnapshot();
    if (!Check(
            TEXT("the locked ending remains distinct from its complete eligibility set"),
            Objective.BrokenSunAvailableFinalResolutions ==
                    Spec.ExpectedAvailability &&
                Objective.BrokenSunFinalResolution ==
                    Spec.SelectedResolution &&
                Bridge->GetBrokenSunPhase() ==
                    EEchoesBrokenSunPhase::RaiseResolutionConduit))
    {
        return FinishWorld();
    }

    const Vec2 ResolutionCenter =
        FEchoesBrokenSunMissionModel::ResolutionConvergenceSite(
            RuntimePlan,
            Spec.SelectedResolution);
    Vec2 ConduitBuildSite;
    if (!Check(
            TEXT("the selected ending uses its test-owned convergence site"),
            ResolutionCenter == Spec.ExpectedResolutionSite) ||
        !Check(
            TEXT("the selected ending exposes a valid conduit footprint"),
            FindBuildSite(ResolutionCenter, 2, ConduitBuildSite)) ||
        !Check(
            TEXT("the worker accepts the exact resolution-conduit build"),
            Bridge->IssueBuildCommand(
                Objective.BrokenSunWorkerId,
                EntityType::UtilityStructure,
                Bridge->SimToWorld(ConduitBuildSite),
                Feedback)) ||
        !Check(
            TEXT("the completed conduit enters the deterministic final hold"),
            TickUntil(
                [Bridge]()
                {
                    return Bridge->GetBrokenSunPhase() ==
                        EEchoesBrokenSunPhase::HoldFinalResolution;
                },
                4000)))
    {
        return FinishWorld();
    }

    Objective = Bridge->GetLocalObjectiveSnapshot();
    if (!Check(
            TEXT("the final hold exposes the selected conduit and remaining window"),
            Objective.bBrokenSunResolutionConduitComplete &&
                Objective.BrokenSunResolutionConduitId != 0 &&
                Objective.BrokenSunResolutionTicksRemaining > 0 &&
                Objective.BrokenSunResolutionTicksRemaining <=
                    FEchoesBrokenSunMissionModel::ResolutionHoldTicks(
                        RuntimePlan,
                        Spec.SelectedResolution)) ||
        !Check(
            TEXT("holding the exact contract appends Mission 15"),
            TickUntil(
                [Bridge]()
                {
                    return Bridge->GetCampaignProgress().FindDecision(
                               EEchoesCampaignMissionId::TheBrokenSun) !=
                        nullptr;
                },
                5000)))
    {
        return FinishWorld();
    }

    const FEchoesCampaignDecisionRecord* MissionRecord =
        Bridge->GetCampaignProgress().FindDecision(
            EEchoesCampaignMissionId::TheBrokenSun);
    const FEchoesCampaignJourney RuntimeJourney =
        Bridge->GetCampaignJourney();
    if (!Check(
            TEXT("the runtime stores the exact selected ending, full eligibility, plan key, and provenance"),
            MissionRecord != nullptr &&
                MissionRecord->Mission ==
                    EEchoesCampaignMissionId::TheBrokenSun &&
                MissionRecord->WellChoice == Spec.RecordedProtocol &&
                MissionRecord->AvailableWellChoices ==
                    BrokenSunWellMask(Spec.RecordedProtocol) &&
                MissionRecord->VerifiedFacts == 0xFF &&
                MissionRecord->FinalResolution ==
                    Spec.SelectedResolution &&
                MissionRecord->AvailableFinalResolutions ==
                    Spec.ExpectedAvailability &&
                MissionRecord->FinalPlanKey == Spec.ExpectedPlanKey &&
                MissionRecord->SimulationSnapshotVersion ==
                    echoes::sim::kSnapshotVersion &&
                MissionRecord->CompletionTick > 0 &&
                MissionRecord->FinalStateChecksum != 0 &&
                Bridge->IsScenarioPaused()) ||
        !Check(
            TEXT("the runtime journey terminates at fifteen records without exposing Mission 16"),
            RuntimeJourney.State ==
                    EEchoesCampaignJourneyState::Complete &&
                RuntimeJourney.CompletedMissionCount == 15 &&
                Bridge->GetCampaignProgress().Decisions.Num() == 15))
    {
        return FinishWorld();
    }

    const FEchoesCampaignDecisionRecord RuntimeMissionRecord =
        *MissionRecord;
    FEchoesCampaignProgress ExpectedPersistedCampaign = Prerequisites;
    ExpectedPersistedCampaign.Decisions.Add(RuntimeMissionRecord);

    FEchoesCampaignProgress ReloadedCampaign;
    FEchoesCampaignProgress RetainedPrerequisiteGeneration;
    if (!Check(
            TEXT("the committed primary exactly matches the fourteen prerequisites plus the runtime Mission 15 record"),
            FEchoesCampaignProgressStore::LoadGeneration(
                CampaignPath,
                ReloadedCampaign,
                Feedback) &&
                ReloadedCampaign.Decisions ==
                    ExpectedPersistedCampaign.Decisions) ||
        !Check(
            TEXT("the retained backup is the exact fourteen-record prerequisite generation"),
            FEchoesCampaignProgressStore::LoadGeneration(
                CampaignBackupPath,
                RetainedPrerequisiteGeneration,
                Feedback) &&
                RetainedPrerequisiteGeneration.Decisions ==
                    Prerequisites.Decisions) ||
        !Check(
            TEXT("the persisted Mission 15 record matches the authoritative runtime record"),
            ReloadedCampaign.FindDecision(
                    EEchoesCampaignMissionId::TheBrokenSun) != nullptr &&
                *ReloadedCampaign.FindDecision(
                    EEchoesCampaignMissionId::TheBrokenSun) ==
                    RuntimeMissionRecord))
    {
        return FinishWorld();
    }

    const FEchoesCampaignJourney ReloadedJourney =
        FEchoesCampaignJourneyModel::Resolve(ReloadedCampaign);
    Check(
        TEXT("the reloaded ledger remains complete at Mission 15 without a Mission 16 projection"),
        ReloadedJourney.State ==
                EEchoesCampaignJourneyState::Complete &&
            ReloadedJourney.CompletedMissionCount == 15);
    return FinishWorld();
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesBrokenSunMissionTest,
    "Echoes.Runtime.Campaign.TheBrokenSun",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesBrokenSunMissionTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    FEchoesScopedTestSaveEnvironment TestSaveEnvironment(*this);
    if (!TestSaveEnvironment.IsReady())
    {
        return false;
    }
    using echoes::sim::ChoirIdentityState;
    using echoes::sim::CommandType;
    using echoes::sim::EntityType;
    using echoes::sim::Faction;
    using echoes::sim::FutureWellChoice;
    using echoes::sim::ResearchType;
    using echoes::sim::Vec2;

    const FutureWellChoice Choices[] = {
        FutureWellChoice::Harvest,
        FutureWellChoice::Preserve,
        FutureWellChoice::Reshape};
    const uint8 ReservePairs[] = {0x7E, 0x7D, 0x7B};
    TSet<uint8> PlanKeys;
    uint8 CoveredResolutionMask = 0;
    int32 PlanContracts = 0;
    for (const FutureWellChoice FoundingChoice : Choices)
    {
        for (const uint8 ReserveFacts : ReservePairs)
        {
            for (const FutureWellChoice Protocol : Choices)
            {
                FEchoesBrokenSunPlan Plan;
                const bool bPlanned =
                    FEchoesBrokenSunMissionModel::TryPlanForLedger(
                        FoundingChoice,
                        ReserveFacts,
                        Protocol,
                        Plan);
                TestTrue(
                    TEXT("Every accepted inherited tuple has one explicit final plan"),
                    bPlanned);
                if (!bPlanned)
                {
                    continue;
                }
                TestTrue(
                    TEXT("Every final plan inherits the test-owned literal approach for its recorded protocol"),
                    Plan.CrownfallApproachSite ==
                        TestOwnedBrokenSunApproachSite(Protocol));

                uint8 ExpectedMask = static_cast<uint8>(
                    EEchoesFinalResolutionAvailability::
                        ControlledStabilization);
                const uint8 LifeSupportBit = static_cast<uint8>(
                    EEchoesReserveAuthorityCompletionFact::
                        LifeSupportPowered);
                if (Protocol == FutureWellChoice::Preserve &&
                    (ReserveFacts & LifeSupportBit) != 0)
                {
                    ExpectedMask |= static_cast<uint8>(
                        EEchoesFinalResolutionAvailability::Restoration);
                }
                if (FoundingChoice == FutureWellChoice::Harvest ||
                    Protocol == FutureWellChoice::Harvest)
                {
                    ExpectedMask |= static_cast<uint8>(
                        EEchoesFinalResolutionAvailability::Extinguishment);
                }
                if (FoundingChoice == FutureWellChoice::Reshape ||
                    Protocol == FutureWellChoice::Reshape)
                {
                    ExpectedMask |= static_cast<uint8>(
                        EEchoesFinalResolutionAvailability::OpenEvolution);
                }
                TestEqual(
                    TEXT("The earned ending mask is derived only from explicit ledger facts"),
                    Plan.AvailableFinalResolutions,
                    ExpectedMask);
                const echoes::sim::Vec2 RestorationSite =
                    FEchoesBrokenSunMissionModel::ResolutionConvergenceSite(
                        Plan,
                        EEchoesFinalResolution::Restoration);
                const echoes::sim::Vec2 ControlledSite =
                    FEchoesBrokenSunMissionModel::ResolutionConvergenceSite(
                        Plan,
                        EEchoesFinalResolution::ControlledStabilization);
                const echoes::sim::Vec2 ExtinguishmentSite =
                    FEchoesBrokenSunMissionModel::ResolutionConvergenceSite(
                        Plan,
                        EEchoesFinalResolution::Extinguishment);
                const echoes::sim::Vec2 OpenEvolutionSite =
                    FEchoesBrokenSunMissionModel::ResolutionConvergenceSite(
                        Plan,
                        EEchoesFinalResolution::OpenEvolution);
                TestTrue(
                    TEXT("Every ending retains its test-owned convergence site"),
                    RestorationSite == TestOwnedBrokenSunResolutionSite(
                        EEchoesFinalResolution::Restoration) &&
                        ControlledSite == TestOwnedBrokenSunResolutionSite(
                            EEchoesFinalResolution::ControlledStabilization) &&
                        ExtinguishmentSite == TestOwnedBrokenSunResolutionSite(
                            EEchoesFinalResolution::Extinguishment) &&
                        OpenEvolutionSite == TestOwnedBrokenSunResolutionSite(
                            EEchoesFinalResolution::OpenEvolution));
                TestTrue(
                    TEXT("All four ending domains remain non-overlapping"),
                    BrokenSunObjectiveDomainsAreDisjoint(
                        RestorationSite, 2, ControlledSite, 2) &&
                        BrokenSunObjectiveDomainsAreDisjoint(
                            RestorationSite, 2, ExtinguishmentSite, 2) &&
                        BrokenSunObjectiveDomainsAreDisjoint(
                            RestorationSite, 2, OpenEvolutionSite, 2) &&
                        BrokenSunObjectiveDomainsAreDisjoint(
                            ControlledSite, 2, ExtinguishmentSite, 2) &&
                        BrokenSunObjectiveDomainsAreDisjoint(
                            ControlledSite, 2, OpenEvolutionSite, 2) &&
                        BrokenSunObjectiveDomainsAreDisjoint(
                            ExtinguishmentSite, 2, OpenEvolutionSite, 2));
                TestTrue(
                    TEXT("Open Evolution cannot alias the inherited approach objective"),
                    BrokenSunObjectiveDomainsAreDisjoint(
                        Plan.CrownfallApproachSite,
                        3,
                        OpenEvolutionSite,
                        2));
                TestTrue(
                    TEXT("Every named ending has a nonzero distinct hold"),
                    FEchoesBrokenSunMissionModel::ResolutionHoldTicks(
                        Plan,
                        EEchoesFinalResolution::Restoration) > 0 &&
                        FEchoesBrokenSunMissionModel::ResolutionHoldTicks(
                            Plan,
                            EEchoesFinalResolution::
                                ControlledStabilization) > 0 &&
                        FEchoesBrokenSunMissionModel::ResolutionHoldTicks(
                            Plan,
                            EEchoesFinalResolution::Extinguishment) > 0 &&
                        FEchoesBrokenSunMissionModel::ResolutionHoldTicks(
                            Plan,
                            EEchoesFinalResolution::OpenEvolution) > 0);
                PlanKeys.Add(Plan.StablePlanKey);
                CoveredResolutionMask |= Plan.AvailableFinalResolutions;
                ++PlanContracts;
            }
        }
    }
    TestEqual(TEXT("The final projection retains 27 valid plans"),
              PlanContracts, 27);
    TestEqual(TEXT("All final plans retain unique stable keys"),
              PlanKeys.Num(), 27);
    TestEqual(TEXT("All four named endings are reachable across campaign routes"),
              CoveredResolutionMask, static_cast<uint8>(0x0F));
    FEchoesBrokenSunPlan InvalidPlan;
    TestFalse(
        TEXT("A malformed reserve allocation cannot define Mission 15"),
        FEchoesBrokenSunMissionModel::TryPlanForLedger(
            FutureWellChoice::Harvest,
            0x79,
            FutureWellChoice::Preserve,
            InvalidPlan));
    TestFalse(
        TEXT("A dormant recorded protocol cannot define Mission 15"),
        FEchoesBrokenSunMissionModel::TryPlanForLedger(
            FutureWellChoice::Harvest,
            0x7B,
            FutureWellChoice::Dormant,
            InvalidPlan));

    FEchoesBrokenSunMissionFacts Facts;
    TestEqual(TEXT("Inactive facts remain outside Mission 15"),
              FEchoesBrokenSunMissionModel::DeterminePhase(Facts),
              EEchoesBrokenSunPhase::Inactive);
    Facts.bOperationActive = true;
    Facts.bLocalCoreIntact = true;
    Facts.bMaraIntact = true;
    Facts.bOruunIntact = true;
    Facts.bTalarIntact = true;
    Facts.bNemeIntact = true;
    Facts.bCommandForceIntact = true;
    Facts.bSkirmishStillOngoing = true;
    TestEqual(TEXT("Mission 15 begins at the Crownfall approach"),
              FEchoesBrokenSunMissionModel::DeterminePhase(Facts),
              EEchoesBrokenSunPhase::SecureCrownfallApproach);
    Facts.bApproachAnchorComplete = true;
    TestEqual(TEXT("The secured approach opens accord assembly"),
              FEchoesBrokenSunMissionModel::DeterminePhase(Facts),
              EEchoesBrokenSunPhase::AssembleAccord);
    Facts.bMeridianAccordEstablished = true;
    Facts.bKharuunAccordEstablished = true;
    Facts.bChoirAccordEstablished = true;
    TestEqual(TEXT("The complete accord opens the visible ending choice"),
              FEchoesBrokenSunMissionModel::DeterminePhase(Facts),
              EEchoesBrokenSunPhase::ChooseFinalResolution);
    Facts.SelectedResolution =
        EEchoesFinalResolution::ControlledStabilization;
    Facts.bSelectedResolutionEligible = true;
    TestEqual(TEXT("A valid confirmed ending opens its exact conduit"),
              FEchoesBrokenSunMissionModel::DeterminePhase(Facts),
              EEchoesBrokenSunPhase::RaiseResolutionConduit);
    Facts.bResolutionConduitComplete = true;
    TestEqual(TEXT("The exact conduit opens the deterministic hold"),
              FEchoesBrokenSunMissionModel::DeterminePhase(Facts),
              EEchoesBrokenSunPhase::HoldFinalResolution);
    Facts.bResolutionWindowHeld = true;
    TestEqual(TEXT("Only the complete final contract succeeds"),
              FEchoesBrokenSunMissionModel::DeterminePhase(Facts),
              EEchoesBrokenSunPhase::Complete);
    Facts.bResolutionContractFailed = true;
    TestEqual(TEXT("A latched contract failure overrides repaired facts"),
              FEchoesBrokenSunMissionModel::DeterminePhase(Facts),
              EEchoesBrokenSunPhase::Failed);

    const TArray<TPair<FString, TFunction<void(FEchoesBrokenSunMissionFacts&)>>>
        ProtectedLosses = {
            {TEXT("local core"), [](FEchoesBrokenSunMissionFacts& Value)
                { Value.bLocalCoreIntact = false; }},
            {TEXT("Mara"), [](FEchoesBrokenSunMissionFacts& Value)
                { Value.bMaraIntact = false; }},
            {TEXT("Oruun"), [](FEchoesBrokenSunMissionFacts& Value)
                { Value.bOruunIntact = false; }},
            {TEXT("Talar"), [](FEchoesBrokenSunMissionFacts& Value)
                { Value.bTalarIntact = false; }},
            {TEXT("Neme"), [](FEchoesBrokenSunMissionFacts& Value)
                { Value.bNemeIntact = false; }},
            {TEXT("command force"), [](FEchoesBrokenSunMissionFacts& Value)
                { Value.bCommandForceIntact = false; }},
            {TEXT("ongoing match"), [](FEchoesBrokenSunMissionFacts& Value)
                { Value.bSkirmishStillOngoing = false; }}};
    for (const auto& Loss : ProtectedLosses)
    {
        FEchoesBrokenSunMissionFacts FailedFacts = Facts;
        FailedFacts.bResolutionContractFailed = false;
        Loss.Value(FailedFacts);
        TestEqual(
            *FString::Printf(
                TEXT("Loss of %s fails closed"),
                *Loss.Key),
            FEchoesBrokenSunMissionModel::DeterminePhase(FailedFacts),
            EEchoesBrokenSunPhase::Failed);
    }
    FEchoesBrokenSunMissionFacts Illicit = Facts;
    Illicit.bResolutionContractFailed = false;
    Illicit.bApproachAnchorComplete = false;
    TestEqual(TEXT("A selected ending before the approach fails closed"),
              FEchoesBrokenSunMissionModel::DeterminePhase(Illicit),
              EEchoesBrokenSunPhase::Failed);
    Illicit = Facts;
    Illicit.bResolutionContractFailed = false;
    Illicit.SelectedResolution = EEchoesFinalResolution::None;
    Illicit.bSelectedResolutionEligible = false;
    TestEqual(TEXT("A conduit before selection fails closed"),
              FEchoesBrokenSunMissionModel::DeterminePhase(Illicit),
              EEchoesBrokenSunPhase::Failed);

    FString Feedback;
    FEchoesCampaignProgress FourteenRecords = MakeBrokenSunPrerequisites(
        FutureWellChoice::Harvest,
        FutureWellChoice::Preserve,
        0x7B,
        14,
        Feedback);
    TestEqual(TEXT("The accepted prerequisite has exactly fourteen records"),
              FourteenRecords.Decisions.Num(), 14);
    FEchoesBrokenSunPlan RuntimePlan;
    TestTrue(
        TEXT("The runtime fixture projects one valid final plan"),
        FEchoesBrokenSunMissionModel::TryPlanForLedger(
            FutureWellChoice::Harvest,
            0x7B,
            FutureWellChoice::Preserve,
            RuntimePlan));

    const TArray<uint8> LegacyBytes =
        EncodeLegacyBrokenSunPrerequisites(FourteenRecords);
    FEchoesCampaignProgress Migrated;
    TestTrue(
        TEXT("A schema-1 fourteen-record campaign migrates into Mission 15 eligibility"),
        FEchoesCampaignProgressStore::Decode(
            LegacyBytes,
            Migrated,
            Feedback) &&
            Migrated.Decisions.Num() == 14);
    TArray<uint8> SchemaTwoBytes;
    FEchoesCampaignProgress RoundTrippedMigration;
    TestTrue(
        TEXT("The migrated chain rewrites and validates under schema 2"),
        FEchoesCampaignProgressStore::Encode(
            Migrated,
            SchemaTwoBytes,
            Feedback) &&
            SchemaTwoBytes.Num() == 12 + 14 * 27 + 4 &&
            FEchoesCampaignProgressStore::Decode(
                SchemaTwoBytes,
                RoundTrippedMigration,
                Feedback) &&
            RoundTrippedMigration.Decisions == Migrated.Decisions);

    uint8 AppendedEndingMask = 0;
    const EEchoesFinalResolution Resolutions[] = {
        EEchoesFinalResolution::Restoration,
        EEchoesFinalResolution::ControlledStabilization,
        EEchoesFinalResolution::Extinguishment,
        EEchoesFinalResolution::OpenEvolution};
    for (const EEchoesFinalResolution Resolution : Resolutions)
    {
        if (!FEchoesBrokenSunMissionModel::IsResolutionAvailable(
                RuntimePlan,
                Resolution))
        {
            continue;
        }
        FEchoesCampaignProgress Candidate = FourteenRecords;
        const FEchoesCampaignDecisionRecord FinalRecord =
            MakeBrokenSunFinalRecord(RuntimePlan, Resolution);
        TestEqual(
            TEXT("Every earned resolution appends one exact Mission 15 record"),
            Candidate.AppendDecision(FinalRecord, Feedback),
            EEchoesCampaignCommitStatus::Added);
        TestEqual(
            TEXT("An identical final replay is idempotent"),
            Candidate.AppendDecision(FinalRecord, Feedback),
            EEchoesCampaignCommitStatus::AlreadyRecorded);
        FEchoesCampaignDecisionRecord Conflict = FinalRecord;
        Conflict.FinalResolution =
            Resolution == EEchoesFinalResolution::ControlledStabilization
                ? EEchoesFinalResolution::Restoration
                : EEchoesFinalResolution::ControlledStabilization;
        TestEqual(
            TEXT("A divergent final replay cannot rewrite the ledger"),
            Candidate.AppendDecision(Conflict, Feedback),
            EEchoesCampaignCommitStatus::ReplayConflict);
        AppendedEndingMask |=
            FEchoesBrokenSunMissionModel::ResolutionMask(Resolution);
    }
    TestEqual(
        TEXT("The selected runtime route admits its exact earned endings"),
        AppendedEndingMask,
        RuntimePlan.AvailableFinalResolutions);

    FEchoesCampaignDecisionRecord InvalidFinal = MakeBrokenSunFinalRecord(
        RuntimePlan,
        EEchoesFinalResolution::ControlledStabilization);
    InvalidFinal.AvailableFinalResolutions ^= static_cast<uint8>(
        EEchoesFinalResolutionAvailability::OpenEvolution);
    FEchoesCampaignProgress InvalidProgress = FourteenRecords;
    TestTrue(
        TEXT("A fabricated ending eligibility mask is rejected"),
        InvalidProgress.AppendDecision(InvalidFinal, Feedback) ==
                EEchoesCampaignCommitStatus::StorageFailure &&
            Feedback.Contains(TEXT("FINALE_PROJECTION_INVALID")));
    InvalidFinal = MakeBrokenSunFinalRecord(
        RuntimePlan,
        EEchoesFinalResolution::ControlledStabilization);
    InvalidFinal.FinalPlanKey =
        static_cast<uint8>((RuntimePlan.StablePlanKey + 1) % 27);
    InvalidProgress = FourteenRecords;
    TestTrue(
        TEXT("A fabricated final plan key is rejected"),
        InvalidProgress.AppendDecision(InvalidFinal, Feedback) ==
                EEchoesCampaignCommitStatus::StorageFailure &&
            Feedback.Contains(TEXT("FINALE_PROJECTION_INVALID")));
    InvalidFinal = MakeBrokenSunFinalRecord(
        RuntimePlan,
        static_cast<EEchoesFinalResolution>(5));
    InvalidProgress = FourteenRecords;
    TestTrue(
        TEXT("An unknown ending enum is rejected"),
        InvalidProgress.AppendDecision(InvalidFinal, Feedback) ==
            EEchoesCampaignCommitStatus::StorageFailure);
    InvalidFinal = MakeBrokenSunFinalRecord(
        RuntimePlan,
        EEchoesFinalResolution::ControlledStabilization);
    InvalidFinal.WellChoice = FutureWellChoice::Harvest;
    InvalidFinal.AvailableWellChoices =
        BrokenSunWellMask(FutureWellChoice::Harvest);
    InvalidProgress = FourteenRecords;
    TestTrue(
        TEXT("Mission 15 cannot rewrite Mission 14's protocol"),
        InvalidProgress.AppendDecision(InvalidFinal, Feedback) ==
                EEchoesCampaignCommitStatus::StorageFailure &&
            Feedback.Contains(TEXT("FINALE_LUME_PROTOCOL")));

    FEchoesCampaignProgress Reordered = FourteenRecords;
    Swap(Reordered.Decisions[12], Reordered.Decisions[13]);
    TestTrue(
        TEXT("A reordered M13-M14 chain cannot admit Mission 15"),
        Reordered.AppendDecision(
            MakeBrokenSunFinalRecord(
                RuntimePlan,
                EEchoesFinalResolution::ControlledStabilization),
            Feedback) == EEchoesCampaignCommitStatus::StorageFailure &&
            Feedback.Contains(TEXT("FINALE_LEDGER_ORDER")));

    const FString CampaignPath =
        FEchoesCampaignProgressStore::GetDefaultPath();
    FPreservedBrokenSunFile PreservedCampaign(CampaignPath);
    FPreservedBrokenSunFile PreservedCampaignBackup(
        CampaignPath + TEXT(".bak"));
    FPreservedBrokenSunFile PreservedCampaignTemporary(
        CampaignPath + TEXT(".tmp"));
    for (const FString& Path : {
             CampaignPath,
             CampaignPath + TEXT(".bak"),
             CampaignPath + TEXT(".tmp")})
    {
        IFileManager::Get().Delete(*Path, false, true, true);
    }

    FEchoesCampaignProgress ThirteenRecords = MakeBrokenSunPrerequisites(
        FutureWellChoice::Harvest,
        FutureWellChoice::Preserve,
        0x7B,
        13,
        Feedback);
    TestTrue(
        TEXT("The thirteen-record lock fixture is stored"),
        FEchoesCampaignProgressStore::SaveAtomic(
            CampaignPath,
            ThirteenRecords,
            Feedback));
    {
        FTestWorldWrapper LockedWorld;
        if (!LockedWorld.CreateTestWorld(EWorldType::Game))
        {
            LockedWorld.ForwardErrorMessages(this);
            AddError(TEXT("Could not create the locked Mission 15 world."));
            return false;
        }
        UEchoesSimulationSubsystem* LockedBridge =
            LockedWorld.GetTestWorld()->GetSubsystem<
                UEchoesSimulationSubsystem>();
        TestFalse(
            TEXT("Mission 15 rejects a ledger without Mission 14"),
            LockedBridge != nullptr &&
                LockedBridge->SelectOperationMode(
                    EEchoesOperationMode::CampaignTheBrokenSun,
                    Feedback));
        TestTrue(
            TEXT("The lock response names Several Voices, One Command"),
            Feedback.Contains(TEXT("Several Voices, One Command")));
        LockedWorld.ForwardErrorMessages(this);
    }

    const FString QuickSavePath =
        BrokenSunQuickSavePath(FourteenRecords);
    FPreservedBrokenSunFile PreservedQuickSave(QuickSavePath);
    FPreservedBrokenSunFile PreservedQuickSaveBackup(
        QuickSavePath + TEXT(".bak"));
    FPreservedBrokenSunFile PreservedQuickSaveStagedBackup(
        QuickSavePath + TEXT(".bak.tmp"));
    FPreservedBrokenSunFile PreservedQuickSaveTemporary(
        QuickSavePath + TEXT(".tmp"));
    for (const FString& Path : {
             QuickSavePath,
             QuickSavePath + TEXT(".bak"),
             QuickSavePath + TEXT(".bak.tmp"),
             QuickSavePath + TEXT(".tmp")})
    {
        IFileManager::Get().Delete(*Path, false, true, true);
    }
    TestTrue(
        TEXT("The exact fourteen-record campaign is stored"),
        FEchoesCampaignProgressStore::SaveAtomic(
            CampaignPath,
            FourteenRecords,
            Feedback));

    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the Mission 15 test world."));
        return false;
    }
    UEchoesSimulationSubsystem* Bridge =
        WorldWrapper.GetTestWorld()->GetSubsystem<
            UEchoesSimulationSubsystem>();
    if (!TestNotNull(TEXT("Mission 15 owns a simulation subsystem"), Bridge) ||
        !TestTrue(TEXT("Fourteen exact records unlock Mission 15"),
                  Bridge != nullptr && Bridge->IsBrokenSunUnlocked()) ||
        !TestTrue(TEXT("Mission 15 can be selected"),
                  Bridge->SelectOperationMode(
                      EEchoesOperationMode::CampaignTheBrokenSun,
                      Feedback)) ||
        !TestTrue(TEXT("Mission 15 can start"),
                  Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    FEchoesObjectiveSnapshot Objective =
        Bridge->GetLocalObjectiveSnapshot();
    const echoes::sim::Entity* Voice =
        Bridge->FindEntity(Objective.BrokenSunAccordVoiceId);
    const echoes::sim::Entity* Heavy =
        Bridge->FindEntity(Objective.BrokenSunAccordHeavyId);
    const echoes::sim::Entity* Neme =
        Bridge->FindEntity(Objective.BrokenSunNemeId);
    const echoes::sim::Entity* Worker =
        Bridge->FindEntity(Objective.BrokenSunWorkerId);
    const echoes::sim::Entity* Mara =
        Bridge->FindEntity(Objective.BrokenSunMaraId);
    const echoes::sim::Entity* Oruun =
        Bridge->FindEntity(Objective.BrokenSunOruunId);
    const echoes::sim::Entity* Talar =
        Bridge->FindEntity(Objective.BrokenSunTalarId);
    TestTrue(
        TEXT("Mission 15 launches the exact command force and three protected neutral witnesses"),
        Voice != nullptr && Heavy != nullptr && Neme != nullptr &&
            Worker != nullptr && Mara != nullptr && Oruun != nullptr &&
            Talar != nullptr &&
            Voice->owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            Heavy->owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            Neme->owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            Worker->owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            Voice->faction == Faction::HollowChoir &&
            Heavy->faction == Faction::HollowChoir &&
            Neme->faction == Faction::HollowChoir &&
            Worker->faction == Faction::HollowChoir &&
            Mara->owner == 2 &&
            Mara->faction == Faction::MeridianCompact &&
            Oruun->owner == 3 &&
            Oruun->faction == Faction::KharuunAssemblies &&
            Talar->owner == 2 &&
            Talar->faction == Faction::MeridianCompact);
    TestEqual(TEXT("Mission 15 begins at the ordered approach"),
              Bridge->GetBrokenSunPhase(),
              EEchoesBrokenSunPhase::SecureCrownfallApproach);

    echoes::sim::EntityId ResearchLoomId = 0;
    for (const echoes::sim::Entity& Entity :
         Bridge->GetSimulation()->Entities())
    {
        if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            Entity.faction == Faction::HollowChoir &&
            Entity.type == EntityType::Barracks)
        {
            ResearchLoomId = Entity.id;
            break;
        }
    }
    if (!TestTrue(TEXT("The final force includes a Research Loom"),
                  ResearchLoomId != 0))
    {
        Bridge->StopPrototypeScenario();
        return false;
    }

    const auto TickUntil = [Bridge](
        const TFunction<bool()>& Predicate,
        int32 MaximumTicks)
    {
        for (int32 TickIndex = 0; TickIndex < MaximumTicks; ++TickIndex)
        {
            if (Predicate())
            {
                return true;
            }
            Bridge->Tick(0.05f);
        }
        return Predicate();
    };
    const auto FindBuildSite = [Bridge](
        const Vec2& Center,
        int32 Radius,
        Vec2& OutSite)
    {
        const echoes::sim::Simulation* Sim = Bridge->GetSimulation();
        if (Sim == nullptr)
        {
            return false;
        }
        for (int32 DeltaY = -Radius; DeltaY <= Radius; ++DeltaY)
        {
            for (int32 DeltaX = -Radius; DeltaX <= Radius; ++DeltaX)
            {
                if (DeltaX * DeltaX + DeltaY * DeltaY >
                    Radius * Radius)
                {
                    continue;
                }
                const Vec2 Candidate = Vec2::FromTiles(
                    Center.x.FloorToInt() + DeltaX,
                    Center.y.FloorToInt() + DeltaY);
                if (Sim->ValidatePlacement(
                        UEchoesSimulationSubsystem::LocalPlayerId,
                        EntityType::UtilityStructure,
                        Candidate) ==
                    echoes::sim::PlacementResult::Valid)
                {
                    OutSite = Candidate;
                    return true;
                }
            }
        }
        return false;
    };

    Bridge->SetScenarioPaused(false);
    TestFalse(
        TEXT("A final resolution cannot be armed before the accord"),
        Bridge->ChooseFinalResolution(
            EEchoesFinalResolution::ControlledStabilization,
            Feedback));
    TestTrue(TEXT("The premature choice is reason-coded"),
             Feedback.Contains(TEXT("FINAL_RESOLUTION_PREREQUISITES")));
    TestFalse(
        TEXT("A voice cannot resolve before Held Alternatives"),
        Bridge->IssueChoirReconciliation(
            Objective.BrokenSunAccordVoiceId,
            ChoirIdentityState::Possible,
            Feedback));
    TestTrue(TEXT("The early reconciliation is reason-coded"),
             Feedback.Contains(TEXT("CHOIR_HELD_ALTERNATIVES_REQUIRED")));

    Vec2 ApproachBuildSite;
    TestTrue(
        TEXT("The approach contract exposes a valid construction footprint"),
        FindBuildSite(RuntimePlan.CrownfallApproachSite, 3,
                      ApproachBuildSite));
    const auto V24BuildSequence =
        Bridge->GetSimulation()->NextCommandSequence(
            UEchoesSimulationSubsystem::LocalPlayerId);
    TestTrue(
        TEXT("The schema-24 approach build has a stable receipt sequence"),
        V24BuildSequence.has_value());
    TestTrue(
        TEXT("The exact approach anchor accepts an ordinary worker build"),
        Bridge->IssueBuildCommand(
            Objective.BrokenSunWorkerId,
            EntityType::UtilityStructure,
            Bridge->SimToWorld(ApproachBuildSite),
            Feedback));
    TestTrue(
        TEXT("The completed anchor advances into accord assembly"),
        TickUntil(
            [Bridge]()
            {
                return Bridge->GetBrokenSunPhase() ==
                    EEchoesBrokenSunPhase::AssembleAccord;
            },
            4000));
    Objective = Bridge->GetLocalObjectiveSnapshot();
    TestTrue(
        TEXT("The exact approach objective is bound to one completed structure"),
        Objective.bBrokenSunApproachSecured &&
            Objective.BrokenSunApproachAnchorId != 0);
    TestTrue(
        TEXT("The native schema-24 source retains the approach-build receipt"),
        V24BuildSequence.has_value() &&
            Bridge->GetSimulation()->FindCommandResolutionReceipt(
                UEchoesSimulationSubsystem::LocalPlayerId,
                *V24BuildSequence)
                .has_value());
    TestTrue(
        TEXT("A pre-choice checkpoint preserves the exact approach"),
        Bridge->QuickSaveScenario(Feedback) &&
            IFileManager::Get().FileExists(*QuickSavePath));
    const uint64 V22ExpectedTick = Bridge->GetSimulation()->CurrentTick();
    const auto V22ExpectedNextCommandSequence =
        Bridge->GetSimulation()->NextCommandSequence(
            UEchoesSimulationSubsystem::LocalPlayerId);
    const FEchoesObjectiveSnapshot V22ExpectedObjective = Objective;
    TArray<uint8> V24Checkpoint;
    EchoesSnapshotMigrationTestHelpers::FEmbeddedSnapshotV24Layout
        V24Layout;
    TestTrue(
        TEXT("The Mission 15 schema-24 checkpoint exposes its bounded receipt block"),
        FFileHelper::LoadFileToArray(V24Checkpoint, *QuickSavePath) &&
            EchoesSnapshotMigrationTestHelpers::
                InspectMission15EnvelopeSnapshotV24(
                    V24Checkpoint, V24Layout) &&
            V24Layout.ReceiptCount > 0U &&
            V24Layout.ReceiptBlockSize ==
                4 + static_cast<int32>(V24Layout.ReceiptCount) * 19);
    TArray<uint8> V22Checkpoint = V24Checkpoint;
    const uint64 ExpectedV24ToV22Shrink = 5ULL +
        static_cast<uint64>(V24Layout.ReceiptCount) * 19ULL;
    TestTrue(
        TEXT("The Mission 15 checkpoint converts through schema 23 to its genuine schema-22 shape"),
        EchoesSnapshotMigrationTestHelpers::
                ConvertMission15EnvelopeSnapshotV24ToV22(V22Checkpoint) &&
            EchoesSnapshotMigrationTestHelpers::Mission15SnapshotVersion(
                V22Checkpoint) == 22U &&
            static_cast<uint64>(V24Checkpoint.Num() - V22Checkpoint.Num()) ==
                ExpectedV24ToV22Shrink &&
            FMemory::Memcmp(
                V24Checkpoint.GetData(), V22Checkpoint.GetData(), 34) == 0 &&
            FMemory::Memcmp(
                V24Checkpoint.GetData() + 38,
                V22Checkpoint.GetData() + 38,
                V24Layout.SnapshotOffset - 38) == 0 &&
            FFileHelper::SaveArrayToFile(V22Checkpoint, *QuickSavePath));
    TestTrue(
        TEXT("The converted Mission 15 primary is the only loadable generation"),
        !IFileManager::Get().FileExists(
            *(QuickSavePath + TEXT(".bak"))) &&
            !IFileManager::Get().FileExists(
                *(QuickSavePath + TEXT(".bak.tmp"))) &&
            !IFileManager::Get().FileExists(
                *(QuickSavePath + TEXT(".tmp"))));
    TestTrue(
        TEXT("Mission 15 loads the genuine schema-22 primary without trailing payload"),
        Bridge->QuickLoadScenario(Feedback) &&
            !Feedback.Contains(TEXT("prior-generation backup")) &&
            !Feedback.Contains(TEXT("staged prior-generation recovery")));
    Objective = Bridge->GetLocalObjectiveSnapshot();
    const echoes::sim::Entity* LoadedApproachAnchor =
        Bridge->FindEntity(Objective.BrokenSunApproachAnchorId);
    TestTrue(
        TEXT("Mission 15 schema migration preserves receipt-independent mission state"),
        Bridge->GetSimulation()->CurrentTick() == V22ExpectedTick &&
            Bridge->GetSimulation()->NextCommandSequence(
                UEchoesSimulationSubsystem::LocalPlayerId) ==
                V22ExpectedNextCommandSequence &&
            Bridge->GetBrokenSunPhase() ==
                EEchoesBrokenSunPhase::AssembleAccord &&
            Objective.bBrokenSunApproachSecured &&
            Objective.BrokenSunApproachAnchorId ==
                V22ExpectedObjective.BrokenSunApproachAnchorId &&
            Objective.BrokenSunAccordVoiceId ==
                V22ExpectedObjective.BrokenSunAccordVoiceId &&
            Objective.BrokenSunAccordHeavyId ==
                V22ExpectedObjective.BrokenSunAccordHeavyId &&
            Objective.BrokenSunNemeId ==
                V22ExpectedObjective.BrokenSunNemeId &&
            Objective.BrokenSunWorkerId ==
                V22ExpectedObjective.BrokenSunWorkerId &&
            Objective.BrokenSunMaraId ==
                V22ExpectedObjective.BrokenSunMaraId &&
            Objective.BrokenSunOruunId ==
                V22ExpectedObjective.BrokenSunOruunId &&
            Objective.BrokenSunTalarId ==
                V22ExpectedObjective.BrokenSunTalarId &&
            LoadedApproachAnchor != nullptr &&
            LoadedApproachAnchor->owner ==
                UEchoesSimulationSubsystem::LocalPlayerId &&
            LoadedApproachAnchor->faction == Faction::HollowChoir &&
            LoadedApproachAnchor->type == EntityType::UtilityStructure &&
            LoadedApproachAnchor->completed &&
            LoadedApproachAnchor->position == ApproachBuildSite &&
            V24BuildSequence.has_value() &&
            !Bridge->GetSimulation()->FindCommandResolutionReceipt(
                UEchoesSimulationSubsystem::LocalPlayerId,
                *V24BuildSequence)
                .has_value());

    TestTrue(
        TEXT("The Research Loom accepts Held Alternatives"),
        Bridge->IssueResearchCommand(
            ResearchLoomId,
            ResearchType::ChoirHeldAlternatives,
            Feedback));
    TestTrue(
        TEXT("Held Alternatives completes deterministically"),
        TickUntil(
            [Bridge]()
            {
                const echoes::sim::PlayerState* Player =
                    Bridge->GetSimulation()->FindPlayer(
                        UEchoesSimulationSubsystem::LocalPlayerId);
                return Player != nullptr &&
                    Player->HasCompletedResearch(
                        ResearchType::ChoirHeldAlternatives);
            },
            1200));
    TestTrue(
        TEXT("The protected voice accepts Possible resolution in Mission 15"),
        Bridge->IssueChoirReconciliation(
            Objective.BrokenSunAccordVoiceId,
            ChoirIdentityState::Possible,
            Feedback));
    TestTrue(
        TEXT("The local voice moves to Mara's public accord site"),
        Bridge->IssueCommand(
            CommandType::Move,
            Objective.BrokenSunAccordVoiceId,
            0,
            Bridge->SimToWorld(RuntimePlan.MaraAccordSite),
            FutureWellChoice::Dormant,
            Feedback));
    TestTrue(
        TEXT("The local Heavy moves to Oruun's public accord site"),
        Bridge->IssueCommand(
            CommandType::Move,
            Objective.BrokenSunAccordHeavyId,
            0,
            Bridge->SimToWorld(RuntimePlan.OruunAccordSite),
            FutureWellChoice::Dormant,
            Feedback));
    TestTrue(
        TEXT("Neme moves to the Choir accord site"),
        Bridge->IssueCommand(
            CommandType::Move,
            Objective.BrokenSunNemeId,
            0,
            Bridge->SimToWorld(RuntimePlan.NemeAccordSite),
            FutureWellChoice::Dormant,
            Feedback));
    const auto IsWithinContractRadius = [](const Vec2& Position,
                                           const Vec2& Site,
                                           int32 Radius)
    {
        const int64 DeltaX = static_cast<int64>(Position.x.Raw()) -
            Site.x.Raw();
        const int64 DeltaY = static_cast<int64>(Position.y.Raw()) -
            Site.y.Raw();
        const int64 RadiusRaw = Vec2::FromTiles(Radius, 0).x.Raw();
        return DeltaX * DeltaX + DeltaY * DeltaY <=
            RadiusRaw * RadiusRaw;
    };
    TestTrue(
        TEXT("Possible, Manifest, and Neme settle at their three command sites"),
        TickUntil(
            [Bridge, RuntimePlan, IsWithinContractRadius]()
            {
                const FEchoesObjectiveSnapshot Current =
                    Bridge->GetLocalObjectiveSnapshot();
                const echoes::sim::Entity* CurrentVoice =
                    Bridge->FindEntity(Current.BrokenSunAccordVoiceId);
                const echoes::sim::Entity* CurrentHeavy =
                    Bridge->FindEntity(Current.BrokenSunAccordHeavyId);
                const echoes::sim::Entity* CurrentNeme =
                    Bridge->FindEntity(Current.BrokenSunNemeId);
                return CurrentVoice != nullptr &&
                    CurrentHeavy != nullptr && CurrentNeme != nullptr &&
                    CurrentVoice->choirIdentityState ==
                        ChoirIdentityState::Possible &&
                    CurrentHeavy->choirIdentityState ==
                        ChoirIdentityState::Manifest &&
                    IsWithinContractRadius(
                        CurrentVoice->position,
                        RuntimePlan.MaraAccordSite,
                        3) &&
                    IsWithinContractRadius(
                        CurrentHeavy->position,
                        RuntimePlan.OruunAccordSite,
                        3) &&
                    IsWithinContractRadius(
                        CurrentNeme->position,
                        RuntimePlan.NemeAccordSite,
                        3);
            },
            3000));
    TestTrue(
        TEXT("The Research Loom accepts Shared Resolution"),
        Bridge->IssueResearchCommand(
            ResearchLoomId,
            ResearchType::ChoirSharedResolution,
            Feedback));
    TestTrue(
        TEXT("The witnessed three-part accord opens the ending choice"),
        TickUntil(
            [Bridge]()
            {
                return Bridge->GetBrokenSunPhase() ==
                    EEchoesBrokenSunPhase::ChooseFinalResolution;
            },
            5000));
    Objective = Bridge->GetLocalObjectiveSnapshot();
    TestTrue(
        TEXT("All three accord channels are visible and complete"),
        Objective.bBrokenSunMeridianAccordEstablished &&
            Objective.bBrokenSunKharuunAccordEstablished &&
            Objective.bBrokenSunChoirAccordEstablished);

    TestFalse(
        TEXT("An unearned ending remains unavailable on this route"),
        Bridge->ChooseFinalResolution(
            EEchoesFinalResolution::OpenEvolution,
            Feedback));
    TestTrue(TEXT("The unearned ending is reason-coded"),
             Feedback.Contains(TEXT("FINAL_RESOLUTION_UNEARNED")));
    TestTrue(
        TEXT("The first valid press arms but does not lock the ending"),
        Bridge->ChooseFinalResolution(
            EEchoesFinalResolution::ControlledStabilization,
            Feedback));
    Objective = Bridge->GetLocalObjectiveSnapshot();
    TestTrue(
        TEXT("The armed checkpoint exposes pending without selected state"),
        Objective.BrokenSunPendingFinalResolution ==
                EEchoesFinalResolution::ControlledStabilization &&
            Objective.BrokenSunFinalResolution ==
                EEchoesFinalResolution::None);
    TestTrue(
        TEXT("The legacy-loaded Mission 15 state resaves as native schema 24"),
        Bridge->QuickSaveScenario(Feedback));
    TArray<uint8> ResavedV24Primary;
    EchoesSnapshotMigrationTestHelpers::FEmbeddedSnapshotV24Layout
        ResavedV24Layout;
    TestTrue(
        TEXT("The Mission 15 primary records native schema 24 after legacy load"),
        FFileHelper::LoadFileToArray(
            ResavedV24Primary, *QuickSavePath) &&
            EchoesSnapshotMigrationTestHelpers::
                InspectMission15EnvelopeSnapshotV24(
                    ResavedV24Primary,
                    ResavedV24Layout) &&
            EchoesSnapshotMigrationTestHelpers::Mission15SnapshotVersion(
                ResavedV24Primary) == 24U);
    TestTrue(
        TEXT("The native Mission 15 primary remains directly loadable"),
        !IFileManager::Get().FileExists(
            *(QuickSavePath + TEXT(".bak.tmp"))) &&
            Bridge->QuickLoadScenario(Feedback) &&
            !Feedback.Contains(TEXT("prior-generation backup")) &&
            !Feedback.Contains(TEXT("staged prior-generation recovery")));
    Objective = Bridge->GetLocalObjectiveSnapshot();
    TestTrue(
        TEXT("Quick load restores the exact armed-but-unconfirmed choice"),
        Objective.BrokenSunPendingFinalResolution ==
                EEchoesFinalResolution::ControlledStabilization &&
            Objective.BrokenSunFinalResolution ==
                EEchoesFinalResolution::None &&
            Bridge->GetBrokenSunPhase() ==
                EEchoesBrokenSunPhase::ChooseFinalResolution);
    TArray<uint8> RetainedV22Backup;
    TestTrue(
        TEXT("The first schema-24 resave retains the valid schema-22 Mission 15 generation"),
        FFileHelper::LoadFileToArray(
            RetainedV22Backup,
            *(QuickSavePath + TEXT(".bak"))) &&
            EchoesSnapshotMigrationTestHelpers::Mission15SnapshotVersion(
                RetainedV22Backup) == 22U);
    TestTrue(
        TEXT("The second identical press locks the ending for this operation"),
        Bridge->ChooseFinalResolution(
            EEchoesFinalResolution::ControlledStabilization,
            Feedback));
    TestTrue(
        TEXT("The locked ending advances to its distinct conduit"),
        Bridge->GetBrokenSunPhase() ==
            EEchoesBrokenSunPhase::RaiseResolutionConduit);

    const Vec2 ResolutionCenter =
        FEchoesBrokenSunMissionModel::ResolutionConvergenceSite(
            RuntimePlan,
            EEchoesFinalResolution::ControlledStabilization);
    Vec2 ConduitBuildSite;
    TestTrue(
        TEXT("The selected ending exposes a valid distinct conduit footprint"),
        FindBuildSite(ResolutionCenter, 2, ConduitBuildSite));
    TestTrue(
        TEXT("The exact Resolution Conduit accepts an ordinary worker build"),
        Bridge->IssueBuildCommand(
            Objective.BrokenSunWorkerId,
            EntityType::UtilityStructure,
            Bridge->SimToWorld(ConduitBuildSite),
            Feedback));
    TestTrue(
        TEXT("The completed conduit begins the final deterministic hold"),
        TickUntil(
            [Bridge]()
            {
                return Bridge->GetBrokenSunPhase() ==
                    EEchoesBrokenSunPhase::HoldFinalResolution;
            },
            4000));
    Objective = Bridge->GetLocalObjectiveSnapshot();
    const uint64 InitialHoldRemaining =
        Objective.BrokenSunResolutionTicksRemaining;
    TestTrue(
        TEXT("The tracker exposes the exact conduit and remaining hold"),
        Objective.bBrokenSunResolutionConduitComplete &&
            Objective.BrokenSunResolutionConduitId != 0 &&
            Objective.BrokenSunResolutionConduitId !=
                Objective.BrokenSunApproachAnchorId &&
            InitialHoldRemaining > 0 &&
            InitialHoldRemaining <= RuntimePlan.ResolutionHoldTicks);
    TestTrue(
        TEXT("The active hold round-trips through the ledger-bound checkpoint"),
        Bridge->QuickSaveScenario(Feedback) &&
            Bridge->QuickLoadScenario(Feedback) &&
            Bridge->GetBrokenSunPhase() ==
                EEchoesBrokenSunPhase::HoldFinalResolution &&
            Bridge->GetLocalObjectiveSnapshot().
                    BrokenSunResolutionTicksRemaining ==
                InitialHoldRemaining);

    Vec2 ContractBreakSite;
    bool bFoundContractBreakSite = false;
    const echoes::sim::Simulation* ActiveSimulation =
        Bridge->GetSimulation();
    if (ActiveSimulation != nullptr)
    {
        for (int32 TileY = 1;
             TileY < ActiveSimulation->Config().mapHeightTiles - 1 &&
             !bFoundContractBreakSite;
             ++TileY)
        {
            for (int32 TileX = 1;
                 TileX < ActiveSimulation->Config().mapWidthTiles - 1;
                 ++TileX)
            {
                const int32 DeltaX =
                    TileX - RuntimePlan.NemeAccordSite.x.FloorToInt();
                const int32 DeltaY =
                    TileY - RuntimePlan.NemeAccordSite.y.FloorToInt();
                const Vec2 Candidate = Vec2::FromTiles(TileX, TileY);
                if (DeltaX * DeltaX + DeltaY * DeltaY >= 144 &&
                    ActiveSimulation->IsPositionPassable(Candidate))
                {
                    ContractBreakSite = Candidate;
                    bFoundContractBreakSite = true;
                    break;
                }
            }
        }
    }
    TestTrue(TEXT("A reachable site exists outside Neme's final contract"),
             bFoundContractBreakSite);
    TestTrue(
        TEXT("Neme accepts a movement order that breaches the active hold"),
        bFoundContractBreakSite &&
            Bridge->IssueCommand(
                CommandType::Move,
                Objective.BrokenSunNemeId,
                0,
                Bridge->SimToWorld(ContractBreakSite),
                FutureWellChoice::Dormant,
                Feedback));
    TestTrue(
        TEXT("Leaving the accord irreversibly fails the final contract"),
        TickUntil(
            [Bridge]()
            {
                return Bridge->GetBrokenSunPhase() ==
                    EEchoesBrokenSunPhase::Failed;
            },
            2500));
    TestTrue(
        TEXT("The failed final contract writes its irreversible latch"),
        Bridge->QuickSaveScenario(Feedback) &&
            Bridge->QuickLoadScenario(Feedback) &&
            Bridge->GetBrokenSunPhase() ==
                EEchoesBrokenSunPhase::Failed &&
            Bridge->GetLocalObjectiveSnapshot().
                bBrokenSunResolutionContractFailed);

    TArray<uint8> ValidHoldBackupBytes;
    TestTrue(
        TEXT("The ledger-bound active-hold recovery generation is readable"),
        FFileHelper::LoadFileToArray(
            ValidHoldBackupBytes,
            *(QuickSavePath + TEXT(".bak"))) &&
            !ValidHoldBackupBytes.IsEmpty());
    TArray<uint8> CorruptedFailedCheckpoint;
    TestTrue(
        TEXT("The failed primary checkpoint can be corrupted for backup recovery"),
        FFileHelper::LoadFileToArray(
            CorruptedFailedCheckpoint,
            *QuickSavePath) &&
            CorruptedFailedCheckpoint.Num() > 16);
    if (CorruptedFailedCheckpoint.Num() > 16)
    {
        CorruptedFailedCheckpoint[16] ^= 0x5A;
        TestTrue(
            TEXT("The corrupted failed checkpoint is written"),
            FFileHelper::SaveArrayToFile(
                CorruptedFailedCheckpoint,
                *QuickSavePath));
    }
    TestTrue(
        TEXT("The retained prior generation restores the valid active hold"),
        Bridge->QuickLoadScenario(Feedback) &&
            Feedback.Contains(TEXT("prior-generation backup")) &&
            Bridge->GetBrokenSunPhase() ==
                EEchoesBrokenSunPhase::HoldFinalResolution &&
            !Bridge->GetLocalObjectiveSnapshot().
                bBrokenSunResolutionContractFailed);
    TestTrue(
        TEXT("Mission 15 can save after recovering from a corrupt primary"),
        Bridge->QuickSaveScenario(Feedback) &&
            Feedback.Contains(
                TEXT("validated recovery checkpoint was preserved")));
    TArray<uint8> PreservedHoldBackupBytes;
    TestTrue(
        TEXT("The ledger-bound valid backup survives corrupt-primary replacement"),
        FFileHelper::LoadFileToArray(
            PreservedHoldBackupBytes,
            *(QuickSavePath + TEXT(".bak"))) &&
            PreservedHoldBackupBytes == ValidHoldBackupBytes);
    TestTrue(
        TEXT("Holding the restored exact contract commits Mission 15"),
        TickUntil(
            [Bridge]()
            {
                return Bridge->GetCampaignProgress().FindDecision(
                           EEchoesCampaignMissionId::TheBrokenSun) !=
                    nullptr;
            },
            1200));
    const FEchoesCampaignDecisionRecord* MissionRecord =
        Bridge->GetCampaignProgress().FindDecision(
            EEchoesCampaignMissionId::TheBrokenSun);
    TestTrue(
        TEXT("Mission 15 stores the exact resolution projection and native provenance"),
        MissionRecord != nullptr &&
            MissionRecord->WellChoice == FutureWellChoice::Preserve &&
            MissionRecord->AvailableWellChoices ==
                BrokenSunWellMask(FutureWellChoice::Preserve) &&
            MissionRecord->VerifiedFacts == 0xFF &&
            MissionRecord->FinalResolution ==
                EEchoesFinalResolution::ControlledStabilization &&
            MissionRecord->AvailableFinalResolutions ==
                RuntimePlan.AvailableFinalResolutions &&
            MissionRecord->FinalPlanKey == RuntimePlan.StablePlanKey &&
            MissionRecord->SimulationSnapshotVersion == 24 &&
            MissionRecord->CompletionTick > 0 &&
            MissionRecord->FinalStateChecksum != 0 &&
            Bridge->IsScenarioPaused());
    FEchoesCampaignProgress Reloaded;
    TestTrue(
        TEXT("The fifteen-record campaign reloads transactionally"),
        FEchoesCampaignProgressStore::LoadWithBackup(
            CampaignPath,
            Reloaded,
            Feedback) &&
            Reloaded.Decisions.Num() == 15);
    if (MissionRecord != nullptr)
    {
        TestEqual(
            TEXT("The completed ending replays idempotently"),
            Reloaded.AppendDecision(*MissionRecord, Feedback),
            EEchoesCampaignCommitStatus::AlreadyRecorded);
        FEchoesCampaignDecisionRecord RuntimeConflict = *MissionRecord;
        RuntimeConflict.FinalResolution =
            EEchoesFinalResolution::Restoration;
        TestEqual(
            TEXT("A different earned ending cannot rewrite the final record"),
            Reloaded.AppendDecision(RuntimeConflict, Feedback),
            EEchoesCampaignCommitStatus::ReplayConflict);
    }

    AEchoesPlayerController* EndingController =
        WorldWrapper.GetTestWorld()->SpawnActor<AEchoesPlayerController>();
    if (TestNotNull(
            TEXT("The completed campaign can present its terminal journey"),
            EndingController))
    {
        EndingController->NotifyBrokenSunFinished(
            true,
            EEchoesFinalResolution::ControlledStabilization,
            EEchoesFinalResolution::ControlledStabilization,
            EEchoesCampaignCommitStatus::AlreadyRecorded);
        EndingController->ConfirmPrimaryAction();
        TestTrue(
            TEXT("Mission 15 Enter returns to title without inventing Mission 16"),
            EndingController->IsTitleScreenVisible() &&
                !EndingController->IsMatchResultVisible() &&
                Bridge->GetOperationMode() ==
                    EEchoesOperationMode::CampaignTheBrokenSun &&
                Bridge->GetCampaignJourney().State ==
                    EEchoesCampaignJourneyState::Complete);
        EndingController->Destroy();
    }

    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return !HasAnyErrors() && !WorldWrapper.HasFailed();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesBrokenSunAlternateResolutionPersistenceTest,
    "Echoes.Runtime.Campaign.TheBrokenSunAlternateResolutionPersistence",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesBrokenSunAlternateResolutionPersistenceTest::RunTest(
    const FString& Parameters)
{
    (void)Parameters;

    FEchoesScopedTestSaveEnvironment TestSaveEnvironment(*this);
    if (!TestSaveEnvironment.IsReady())
    {
        return false;
    }

    const FBrokenSunResolutionPersistenceSpec Cases[] = {
        {
            TEXT("plan 17 Preserve/Reshape to Open Evolution"),
            echoes::sim::FutureWellChoice::Preserve,
            echoes::sim::FutureWellChoice::Reshape,
            EEchoesFinalResolution::OpenEvolution,
            17,
            0x0A,
            echoes::sim::Vec2::FromTiles(32, 43),
            echoes::sim::Vec2::FromTiles(32, 40),
            echoes::sim::Vec2::FromTiles(26, 54),
        },
        {
            TEXT("plan 25 Reshape/Preserve to Controlled Stabilization"),
            echoes::sim::FutureWellChoice::Reshape,
            echoes::sim::FutureWellChoice::Preserve,
            EEchoesFinalResolution::ControlledStabilization,
            25,
            0x0B,
            echoes::sim::Vec2::FromTiles(32, 56),
            {},
            echoes::sim::Vec2::FromTiles(32, 44),
        },
    };

    bool bPassed = true;
    for (const FBrokenSunResolutionPersistenceSpec& Case : Cases)
    {
        bPassed = RunBrokenSunResolutionPersistenceCase(*this, Case) &&
            bPassed;
        const FString CampaignPath =
            FEchoesCampaignProgressStore::GetDefaultPath();
        const bool bCleaned =
            !IFileManager::Get().FileExists(*CampaignPath) &&
            !IFileManager::Get().FileExists(
                *(CampaignPath + TEXT(".bak"))) &&
            !IFileManager::Get().FileExists(
                *(CampaignPath + TEXT(".tmp")));
        TestTrue(
            *FString::Printf(
                TEXT("%s: campaign primary, backup, and temporary generations are removed before the next case"),
                Case.Label),
            bCleaned);
        bPassed = bCleaned && bPassed;
    }

    return bPassed && !HasAnyErrors();
}

#endif
