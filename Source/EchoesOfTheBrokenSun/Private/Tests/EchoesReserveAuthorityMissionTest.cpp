#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "EchoesCampaignProgress.h"
#include "EchoesReserveAuthorityMissionModel.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Tests/AutomationCommon.h"

namespace
{
struct FPreservedReserveFile final
{
    explicit FPreservedReserveFile(FString InPath) : Path(MoveTemp(InPath))
    {
        bExisted = IFileManager::Get().FileExists(*Path);
        if (bExisted)
        {
            FFileHelper::LoadFileToArray(Contents, *Path);
        }
    }

    ~FPreservedReserveFile()
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

uint8 ReserveChoiceMask(echoes::sim::FutureWellChoice Choice)
{
    switch (Choice)
    {
        case echoes::sim::FutureWellChoice::Harvest: return 1 << 0;
        case echoes::sim::FutureWellChoice::Preserve: return 1 << 1;
        case echoes::sim::FutureWellChoice::Reshape: return 1 << 2;
        default: return 0;
    }
}

FEchoesCampaignDecisionRecord MakeReservePrerequisite(
    EEchoesCampaignMissionId Mission,
    echoes::sim::FutureWellChoice Choice)
{
    FEchoesCampaignDecisionRecord Record;
    Record.Mission = Mission;
    Record.WellChoice = Choice;
    Record.AvailableWellChoices =
        Mission == EEchoesCampaignMissionId::WhatTheLedgerKeeps
            ? 0x07
            : ReserveChoiceMask(Choice);
    Record.SimulationSnapshotVersion = echoes::sim::kSnapshotVersion;
    Record.CompletionTick = 100 + static_cast<uint8>(Mission) * 300;
    Record.FinalStateChecksum = 0x9E5100ULL + static_cast<uint8>(Mission);
    switch (Mission)
    {
        case EEchoesCampaignMissionId::WhatTheLedgerKeeps:
            Record.VerifiedFacts = 0x0F;
            break;
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
        default:
            break;
    }
    return Record;
}

FString ReserveQuickSavePath(const FEchoesCampaignProgress& Progress)
{
    TArray<uint8> LedgerBytes;
    FString Error;
    if (!FEchoesCampaignProgressStore::Encode(
            Progress, LedgerBytes, Error) || LedgerBytes.IsEmpty())
    {
        return {};
    }
    const uint32 Fingerprint = FCrc::MemCrc32(
        LedgerBytes.GetData(),
        LedgerBytes.Num() - static_cast<int32>(sizeof(uint32)));
    return FPaths::Combine(
        FEchoesCampaignProgressStore::GetSaveGameDirectory(),
        FString::Printf(
            TEXT("EchoesQuickSaveReserveAuthority-%08X.bin"),
            Fingerprint));
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesReserveAuthorityMissionTest,
    "Echoes.Runtime.Campaign.ReserveAuthority",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesReserveAuthorityMissionTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    FEchoesScopedTestSaveEnvironment TestSaveEnvironment(*this);
    if (!TestSaveEnvironment.IsReady())
    {
        return false;
    }

    const FEchoesReserveAuthorityPlan HarvestPlan =
        FEchoesReserveAuthorityMissionModel::PlanForChoice(
            echoes::sim::FutureWellChoice::Harvest);
    const FEchoesReserveAuthorityPlan PreservePlan =
        FEchoesReserveAuthorityMissionModel::PlanForChoice(
            echoes::sim::FutureWellChoice::Preserve);
    const FEchoesReserveAuthorityPlan ReshapePlan =
        FEchoesReserveAuthorityMissionModel::PlanForChoice(
            echoes::sim::FutureWellChoice::Reshape);
    TestTrue(TEXT("All inherited choices produce distinct reserve doctrine"),
             HarvestPlan.AuthoritySite == echoes::sim::Vec2::FromTiles(15, 16) &&
                 PreservePlan.AuthoritySite == echoes::sim::Vec2::FromTiles(15, 15) &&
                 ReshapePlan.AuthoritySite == echoes::sim::Vec2::FromTiles(16, 15) &&
                 HarvestPlan.RecommendedFirstDistrict !=
                     PreservePlan.RecommendedFirstDistrict &&
                 PreservePlan.RecommendedFirstDistrict !=
                     ReshapePlan.RecommendedFirstDistrict);
    TestTrue(TEXT("The three district relay sites remain distinct"),
             FEchoesReserveAuthorityMissionModel::RelaySiteForDistrict(
                 EEchoesCityDistrict::LifeSupport) !=
                 FEchoesReserveAuthorityMissionModel::RelaySiteForDistrict(
                     EEchoesCityDistrict::Transit) &&
                 FEchoesReserveAuthorityMissionModel::RelaySiteForDistrict(
                     EEchoesCityDistrict::Transit) !=
                 FEchoesReserveAuthorityMissionModel::RelaySiteForDistrict(
                     EEchoesCityDistrict::Archive));

    FEchoesReserveAuthorityMissionFacts Facts;
    TestTrue(TEXT("Inactive facts stay outside mission nine"),
             FEchoesReserveAuthorityMissionModel::DeterminePhase(Facts) ==
                 EEchoesReserveAuthorityPhase::Inactive);
    Facts.bOperationActive = true;
    Facts.bLocalCoreIntact = true;
    Facts.bMaraIntact = true;
    Facts.bLifeSupportIntact = true;
    Facts.bTransitIntact = true;
    Facts.bArchiveIntact = true;
    Facts.bSkirmishStillOngoing = true;
    TestTrue(TEXT("The operation begins at the authority site"),
             FEchoesReserveAuthorityMissionModel::DeterminePhase(Facts) ==
                 EEchoesReserveAuthorityPhase::SecureAuthority);
    Facts.bAuthoritySecured = true;
    TestTrue(TEXT("Authority opens the first district allocation"),
             FEchoesReserveAuthorityMissionModel::DeterminePhase(Facts) ==
                 EEchoesReserveAuthorityPhase::AllocateFirstDistrict);
    Facts.bLifeSupportPowered = true;
    TestTrue(TEXT("One powered district opens the second allocation"),
             FEchoesReserveAuthorityMissionModel::DeterminePhase(Facts) ==
                 EEchoesReserveAuthorityPhase::AllocateSecondDistrict);
    Facts.bTransitPowered = true;
    TestTrue(TEXT("Exactly two powered districts identify Archive as deferred"),
             FEchoesReserveAuthorityMissionModel::DeferredDistrict(Facts) ==
                     EEchoesCityDistrict::Archive &&
                 FEchoesReserveAuthorityMissionModel::DeterminePhase(Facts) ==
                     EEchoesReserveAuthorityPhase::ReachDeferredDistrict);
    Facts.bMaraAtDeferredDistrict = true;
    TestTrue(TEXT("Mara reaching the intact deferred district completes"),
             FEchoesReserveAuthorityMissionModel::DeterminePhase(Facts) ==
                 EEchoesReserveAuthorityPhase::Complete);
    Facts.bArchivePowered = true;
    TestTrue(TEXT("Powering all three districts fails the finite reserve"),
             FEchoesReserveAuthorityMissionModel::DeterminePhase(Facts) ==
                 EEchoesReserveAuthorityPhase::Failed);
    Facts.bArchivePowered = false;
    Facts.bArchiveIntact = false;
    TestTrue(TEXT("Any protected district loss fails the operation"),
             FEchoesReserveAuthorityMissionModel::DeterminePhase(Facts) ==
                 EEchoesReserveAuthorityPhase::Failed);
    Facts.bArchiveIntact = true;
    Facts.bSkirmishStillOngoing = false;
    TestTrue(TEXT("Either terminal Core outcome invalidates allocation"),
             FEchoesReserveAuthorityMissionModel::DeterminePhase(Facts) ==
                 EEchoesReserveAuthorityPhase::Failed);

    const FString CampaignPath =
        FEchoesCampaignProgressStore::GetDefaultPath();
    FPreservedReserveFile PreservedPrimary(CampaignPath);
    FPreservedReserveFile PreservedBackup(CampaignPath + TEXT(".bak"));
    FPreservedReserveFile PreservedTemporary(CampaignPath + TEXT(".tmp"));
    IFileManager::Get().Delete(*CampaignPath, false, true, true);
    IFileManager::Get().Delete(*(CampaignPath + TEXT(".bak")), false, true, true);
    IFileManager::Get().Delete(*(CampaignPath + TEXT(".tmp")), false, true, true);

    FString Feedback;
    FEchoesCampaignProgress SevenRecords;
    for (const EEchoesCampaignMissionId Mission : {
             EEchoesCampaignMissionId::WhatTheLedgerKeeps,
             EEchoesCampaignMissionId::SevenAccountsOfRain,
             EEchoesCampaignMissionId::ACityOnReserve,
             EEchoesCampaignMissionId::TheUnburiedRoad,
             EEchoesCampaignMissionId::TermsOfContinuance,
             EEchoesCampaignMissionId::NamesWithoutBirths,
             EEchoesCampaignMissionId::TheShapeOfSilence})
    {
        TestTrue(TEXT("The fixture accepts a consistent prior record"),
                 SevenRecords.AppendDecision(
                     MakeReservePrerequisite(
                         Mission, echoes::sim::FutureWellChoice::Preserve),
                     Feedback) == EEchoesCampaignCommitStatus::Added);
    }
    TestTrue(TEXT("A seven-record ledger is stored for the lock check"),
             FEchoesCampaignProgressStore::SaveAtomic(
                 CampaignPath, SevenRecords, Feedback));
    {
        FTestWorldWrapper LockedWorld;
        if (!LockedWorld.CreateTestWorld(EWorldType::Game))
        {
            LockedWorld.ForwardErrorMessages(this);
            AddError(TEXT("Could not create the locked mission-nine world."));
            return false;
        }
        UEchoesSimulationSubsystem* LockedBridge =
            LockedWorld.GetTestWorld()->GetSubsystem<
                UEchoesSimulationSubsystem>();
        TestFalse(TEXT("Mission nine rejects a seven-record ledger"),
                  LockedBridge != nullptr &&
                      LockedBridge->SelectOperationMode(
                          EEchoesOperationMode::CampaignReserveAuthority,
                          Feedback));
        TestTrue(TEXT("The locked response names The Shape Beside Us"),
                 Feedback.Contains(TEXT("The Shape Beside Us")));
        LockedWorld.ForwardErrorMessages(this);
    }

    FEchoesCampaignProgress EightRecords = SevenRecords;
    TestTrue(TEXT("The eighth prerequisite record is accepted"),
             EightRecords.AppendDecision(
                 MakeReservePrerequisite(
                     EEchoesCampaignMissionId::TheShapeBesideUs,
                     echoes::sim::FutureWellChoice::Preserve),
                 Feedback) == EEchoesCampaignCommitStatus::Added);
    FEchoesCampaignDecisionRecord InvalidAllocation;
    InvalidAllocation.Mission = EEchoesCampaignMissionId::ReserveAuthority;
    InvalidAllocation.WellChoice = echoes::sim::FutureWellChoice::Preserve;
    InvalidAllocation.AvailableWellChoices = 1 << 1;
    InvalidAllocation.VerifiedFacts = 0x7F;
    InvalidAllocation.SimulationSnapshotVersion = echoes::sim::kSnapshotVersion;
    InvalidAllocation.CompletionTick = 2500;
    InvalidAllocation.FinalStateChecksum = 0x9E5109ULL;
    FEchoesCampaignProgress InvalidProgress = EightRecords;
    TestTrue(TEXT("A fabricated three-district allocation is rejected"),
             InvalidProgress.AppendDecision(InvalidAllocation, Feedback) ==
                 EEchoesCampaignCommitStatus::StorageFailure);
    InvalidAllocation.VerifiedFacts = 0x79;
    TestTrue(TEXT("A one-district allocation cannot enter the ledger"),
             InvalidProgress.AppendDecision(InvalidAllocation, Feedback) ==
                 EEchoesCampaignCommitStatus::StorageFailure);
    const FString QuickSavePath = ReserveQuickSavePath(EightRecords);
    FPreservedReserveFile PreservedQuickSave(QuickSavePath);
    FPreservedReserveFile PreservedQuickSaveBackup(
        QuickSavePath + TEXT(".bak"));
    FPreservedReserveFile PreservedQuickSaveTemporary(
        QuickSavePath + TEXT(".tmp"));
    IFileManager::Get().Delete(*QuickSavePath, false, true, true);
    IFileManager::Get().Delete(*(QuickSavePath + TEXT(".bak")), false, true, true);
    IFileManager::Get().Delete(*(QuickSavePath + TEXT(".tmp")), false, true, true);
    TestTrue(TEXT("The eight-record ledger is stored"),
             FEchoesCampaignProgressStore::SaveAtomic(
                 CampaignPath, EightRecords, Feedback));

    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the Reserve Authority test world."));
        return false;
    }
    UEchoesSimulationSubsystem* Bridge =
        WorldWrapper.GetTestWorld()->GetSubsystem<UEchoesSimulationSubsystem>();
    if (!TestNotNull(TEXT("Mission world owns the simulation subsystem"), Bridge) ||
        !TestTrue(TEXT("Eight consistent records unlock mission nine"),
                  Bridge != nullptr && Bridge->IsReserveAuthorityUnlocked()) ||
        !TestTrue(TEXT("Mission nine operation can be selected"),
                  Bridge->SelectOperationMode(
                      EEchoesOperationMode::CampaignReserveAuthority,
                      Feedback)) ||
        !TestTrue(TEXT("Mission nine scenario starts"),
                  Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const FEchoesObjectiveSnapshot Start = Bridge->GetLocalObjectiveSnapshot();
    TestTrue(TEXT("Mara and all three protected districts have authority IDs"),
             Start.ReserveAuthorityMaraId != 0 &&
                 Start.LifeSupportDistrictId != 0 &&
                 Start.TransitDistrictId != 0 &&
                 Start.ArchiveDistrictId != 0 &&
                 Start.LifeSupportDistrictId != Start.TransitDistrictId &&
                 Start.TransitDistrictId != Start.ArchiveDistrictId);
    TestTrue(TEXT("The operation begins at reserve authority"),
             Bridge->GetReserveAuthorityPhase() ==
                 EEchoesReserveAuthorityPhase::SecureAuthority);
    TestTrue(TEXT("Mission nine uses its ledger-bound quick-save slot"),
             Bridge->QuickSaveScenario(Feedback) &&
                 IFileManager::Get().FileExists(*QuickSavePath));
    TestTrue(TEXT("The initial phase reconstructs after quick load"),
             Bridge->QuickLoadScenario(Feedback) &&
                 Bridge->GetReserveAuthorityPhase() ==
                     EEchoesReserveAuthorityPhase::SecureAuthority);

    TArray<echoes::sim::EntityId> Workers;
    for (const echoes::sim::Entity& Entity : Bridge->GetSimulation()->Entities())
    {
        if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            Entity.type == echoes::sim::EntityType::Worker)
        {
            Workers.Add(Entity.id);
        }
    }
    if (!TestTrue(TEXT("At least two construction workers are available"),
                  Workers.Num() >= 2))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const auto TickUntil = [Bridge](const TFunction<bool()>& Predicate,
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
    Bridge->SetScenarioPaused(false);
    TestFalse(TEXT("District construction is locked before Mara's authority"),
              Bridge->IssueBuildCommand(
                  Workers[0],
                  echoes::sim::EntityType::Dropoff,
                  Bridge->SimToWorld(
                      FEchoesReserveAuthorityMissionModel::RelaySiteForDistrict(
                          EEchoesCityDistrict::LifeSupport)),
                  Feedback));
    TestTrue(TEXT("The early rejection names the missing authority"),
             Feedback.Contains(TEXT("RESERVE_AUTHORITY_REQUIRED")));
    TestTrue(TEXT("Mara accepts the reserve-authority route"),
             Bridge->IssueCommand(
                 echoes::sim::CommandType::Move,
                 Start.ReserveAuthorityMaraId,
                 0,
                 Bridge->SimToWorld(PreservePlan.AuthoritySite),
                 echoes::sim::FutureWellChoice::Dormant,
                 Feedback));
    TestTrue(TEXT("Mara's arrival opens district allocation"),
             TickUntil(
                 [Bridge]()
                 {
                     return Bridge->GetReserveAuthorityPhase() ==
                         EEchoesReserveAuthorityPhase::AllocateFirstDistrict;
                 },
                 3000));
    TestTrue(TEXT("A worker accepts the Life Support Power Link"),
             Bridge->IssueBuildCommand(
                 Workers[0],
                 echoes::sim::EntityType::Dropoff,
                 Bridge->SimToWorld(
                     FEchoesReserveAuthorityMissionModel::RelaySiteForDistrict(
                         EEchoesCityDistrict::LifeSupport)),
                 Feedback));
    TestTrue(TEXT("One ordinary build opens the second allocation"),
             TickUntil(
                 [Bridge]()
                 {
                     return Bridge->GetReserveAuthorityPhase() ==
                         EEchoesReserveAuthorityPhase::AllocateSecondDistrict;
                 },
                 3400));
    TestTrue(TEXT("The one-district allocation quick-saves"),
             Bridge->QuickSaveScenario(Feedback));
    TestTrue(TEXT("Quick load reconstructs the selected district"),
             Bridge->QuickLoadScenario(Feedback) &&
                 Bridge->GetReserveAuthorityPhase() ==
                     EEchoesReserveAuthorityPhase::AllocateSecondDistrict &&
                 Bridge->GetLocalObjectiveSnapshot().bLifeSupportPowered);
    TestTrue(TEXT("A second worker accepts the Transit Power Link"),
             Bridge->IssueBuildCommand(
                 Workers[1],
                 echoes::sim::EntityType::Dropoff,
                 Bridge->SimToWorld(
                     FEchoesReserveAuthorityMissionModel::RelaySiteForDistrict(
                         EEchoesCityDistrict::Transit)),
                 Feedback));
    TestTrue(TEXT("Exactly two powered districts defer Archive"),
             TickUntil(
                 [Bridge]()
                 {
                     return Bridge->GetReserveAuthorityPhase() ==
                         EEchoesReserveAuthorityPhase::ReachDeferredDistrict;
                 },
                 3400) &&
                 Bridge->GetReserveAuthorityDeferredDistrict() ==
                     EEchoesCityDistrict::Archive);
    TestFalse(TEXT("A third Power Link is rejected after allocation"),
              Bridge->IssueBuildCommand(
                  Workers[0],
                  echoes::sim::EntityType::Dropoff,
                  Bridge->SimToWorld(
                      FEchoesReserveAuthorityMissionModel::RelaySiteForDistrict(
                          EEchoesCityDistrict::Archive)),
                  Feedback));
    TestTrue(TEXT("The rejection explains the finite reserve"),
             Feedback.Contains(TEXT("RESERVE_ALLOCATION_LIMIT")));
    TestTrue(TEXT("Mara accepts the deferred Archive route"),
             Bridge->IssueCommand(
                 echoes::sim::CommandType::Move,
                 Start.ReserveAuthorityMaraId,
                 0,
                 Bridge->SimToWorld(
                     FEchoesCityReserveMissionModel::SiteForDistrict(
                         EEchoesCityDistrict::Archive)),
                 echoes::sim::FutureWellChoice::Dormant,
                 Feedback));
    TestTrue(TEXT("The ordinary path commits the ninth ledger record"),
             TickUntil(
                 [Bridge]()
                 {
                     return Bridge->GetCampaignProgress().FindDecision(
                                EEchoesCampaignMissionId::ReserveAuthority) !=
                         nullptr;
                 },
                 3600));

    const FEchoesCampaignDecisionRecord* MissionRecord =
        Bridge->GetCampaignProgress().FindDecision(
            EEchoesCampaignMissionId::ReserveAuthority);
    TestTrue(TEXT("The ninth record preserves the chosen allocation"),
             MissionRecord != nullptr &&
                 MissionRecord->WellChoice ==
                     echoes::sim::FutureWellChoice::Preserve &&
                 MissionRecord->AvailableWellChoices == (1 << 1) &&
                 MissionRecord->VerifiedFacts == 0x7B);
    FEchoesCampaignProgress Reloaded;
    TestTrue(TEXT("The nine-record campaign reloads transactionally"),
             FEchoesCampaignProgressStore::LoadWithBackup(
                 CampaignPath, Reloaded, Feedback) &&
                 Reloaded.Decisions.Num() == 9);
    if (MissionRecord != nullptr)
    {
        FEchoesCampaignDecisionRecord AlternateAllocation = *MissionRecord;
        AlternateAllocation.VerifiedFacts = 0x7D;
        AlternateAllocation.CompletionTick += 1;
        AlternateAllocation.FinalStateChecksum += 1;
        TestTrue(TEXT("An alternate irreversible allocation is a replay conflict"),
                 Reloaded.AppendDecision(AlternateAllocation, Feedback) ==
                     EEchoesCampaignCommitStatus::ReplayConflict);
        TestTrue(TEXT("The replay conflict preserves the original district pair"),
                 Reloaded.FindDecision(
                     EEchoesCampaignMissionId::ReserveAuthority) != nullptr &&
                 Reloaded.FindDecision(
                     EEchoesCampaignMissionId::ReserveAuthority)
                         ->VerifiedFacts == 0x7B);
    }

    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return true;
}

#endif
