#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesTestSaveEnvironment.h"

#include "EchoesCampaignProgress.h"
#include "EchoesCityReserveMissionModel.h"
#include "EchoesPlayerController.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Tests/AutomationCommon.h"

namespace
{
struct FPreservedCityReserveFile final
{
    explicit FPreservedCityReserveFile(FString InPath)
        : Path(MoveTemp(InPath))
    {
        bExisted = IFileManager::Get().FileExists(*Path);
        if (bExisted)
        {
            FFileHelper::LoadFileToArray(Contents, *Path);
        }
    }

    ~FPreservedCityReserveFile()
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

FEchoesCampaignDecisionRecord MakePriorRecord(
    EEchoesCampaignMissionId Mission,
    echoes::sim::FutureWellChoice Choice)
{
    FEchoesCampaignDecisionRecord Record;
    Record.Mission = Mission;
    Record.WellChoice = Choice;
    Record.AvailableWellChoices =
        Mission == EEchoesCampaignMissionId::WhatTheLedgerKeeps
            ? 0x07
        : Choice == echoes::sim::FutureWellChoice::Harvest
            ? 1 << 0
        : Choice == echoes::sim::FutureWellChoice::Preserve
            ? 1 << 1
            : 1 << 2;
    Record.VerifiedFacts =
        Mission == EEchoesCampaignMissionId::WhatTheLedgerKeeps
            ? static_cast<uint8>(
                  static_cast<uint8>(EEchoesCampaignDecisionFact::ArchiveRecovered) |
                  static_cast<uint8>(EEchoesCampaignDecisionFact::CarrierEvacuated) |
                  static_cast<uint8>(EEchoesCampaignDecisionFact::LocalCoreSurvived) |
                  static_cast<uint8>(EEchoesCampaignDecisionFact::FutureWellControlled))
            : static_cast<uint8>(
                  static_cast<uint8>(EEchoesSevenAccountsCompletionFact::WaystoneRootedAtAnchor) |
                  static_cast<uint8>(EEchoesSevenAccountsCompletionFact::MemoryBearerArrived) |
                  static_cast<uint8>(EEchoesSevenAccountsCompletionFact::LocalCoreSurvived) |
                  static_cast<uint8>(EEchoesSevenAccountsCompletionFact::PriorDecisionConsumed));
    Record.CompletionTick =
        Mission == EEchoesCampaignMissionId::WhatTheLedgerKeeps ? 120 : 420;
    Record.FinalStateChecksum =
        Mission == EEchoesCampaignMissionId::WhatTheLedgerKeeps
            ? 0x7A11A2ULL
            : 0x7A11A3ULL;
    return Record;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesCityReserveMissionTest,
    "Echoes.Runtime.Campaign.ACityOnReserve",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesCityReserveMissionTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    FEchoesScopedTestSaveEnvironment TestSaveEnvironment(*this);
    if (!TestSaveEnvironment.IsReady())
    {
        return false;
    }

    const FEchoesCityReserveGrid PreserveGrid =
        FEchoesCityReserveMissionModel::GridForChoice(
            echoes::sim::FutureWellChoice::Preserve);
    TestTrue(TEXT("Preserve prioritizes archive continuity"),
             PreserveGrid.Priority == EEchoesCityDistrict::Archive &&
                 PreserveGrid.Secondary == EEchoesCityDistrict::LifeSupport &&
                 PreserveGrid.Final == EEchoesCityDistrict::Transit);
    TestTrue(TEXT("Harvest prioritizes life support"),
             FEchoesCityReserveMissionModel::GridForChoice(
                 echoes::sim::FutureWellChoice::Harvest).Priority ==
                 EEchoesCityDistrict::LifeSupport);
    TestTrue(TEXT("Reshape prioritizes transit"),
             FEchoesCityReserveMissionModel::GridForChoice(
                 echoes::sim::FutureWellChoice::Reshape).Priority ==
                 EEchoesCityDistrict::Transit);

    FEchoesCityReserveMissionFacts Facts;
    TestTrue(TEXT("Inactive facts stay outside mission three"),
             FEchoesCityReserveMissionModel::DeterminePhase(
                 Facts,
                 PreserveGrid) == EEchoesCityReservePhase::Inactive);
    Facts.bOperationActive = true;
    Facts.bLocalCoreIntact = true;
    Facts.bLifeSupportIntact = true;
    Facts.bTransitIntact = true;
    Facts.bArchiveIntact = true;
    Facts.bSkirmishStillOngoing = true;
    TestTrue(TEXT("The inherited priority opens the grid sequence"),
             FEchoesCityReserveMissionModel::DeterminePhase(
                 Facts,
                 PreserveGrid) ==
                 EEchoesCityReservePhase::StabilizePriority);
    Facts.bArchivePowered = true;
    TestTrue(TEXT("Powering the priority advances to life support"),
             FEchoesCityReserveMissionModel::DeterminePhase(
                 Facts,
                 PreserveGrid) ==
                 EEchoesCityReservePhase::StabilizeSecondary);
    Facts.bLifeSupportPowered = true;
    TestTrue(TEXT("Powering the secondary advances to transit"),
             FEchoesCityReserveMissionModel::DeterminePhase(
                 Facts,
                 PreserveGrid) ==
                 EEchoesCityReservePhase::StabilizeFinal);
    Facts.bTransitPowered = true;
    TestTrue(TEXT("All three powered districts complete the mission"),
             FEchoesCityReserveMissionModel::DeterminePhase(
                 Facts,
                 PreserveGrid) == EEchoesCityReservePhase::Complete);
    Facts.bArchiveIntact = false;
    TestTrue(TEXT("Losing any district fails the mission"),
             FEchoesCityReserveMissionModel::DeterminePhase(
                 Facts,
                 PreserveGrid) == EEchoesCityReservePhase::Failed);

    const FString CampaignPath =
        FEchoesCampaignProgressStore::GetDefaultPath();
    const FString QuickSavePath = FPaths::Combine(
        FEchoesCampaignProgressStore::GetSaveGameDirectory(),
        TEXT("EchoesQuickSaveACityOnReserve.bin"));
    FPreservedCityReserveFile PreservedPrimary(CampaignPath);
    FPreservedCityReserveFile PreservedBackup(CampaignPath + TEXT(".bak"));
    FPreservedCityReserveFile PreservedTemporary(CampaignPath + TEXT(".tmp"));
    FPreservedCityReserveFile PreservedQuickSave(QuickSavePath);
    FPreservedCityReserveFile PreservedQuickSaveBackup(
        QuickSavePath + TEXT(".bak"));
    FPreservedCityReserveFile PreservedQuickSaveBackupTemporary(
        QuickSavePath + TEXT(".bak.tmp"));
    FPreservedCityReserveFile PreservedQuickSaveTemporary(
        QuickSavePath + TEXT(".tmp"));
    IFileManager::Get().Delete(*CampaignPath, false, true, true);
    IFileManager::Get().Delete(*(CampaignPath + TEXT(".bak")), false, true, true);
    IFileManager::Get().Delete(*(CampaignPath + TEXT(".tmp")), false, true, true);
    IFileManager::Get().Delete(*QuickSavePath, false, true, true);
    IFileManager::Get().Delete(
        *(QuickSavePath + TEXT(".bak")), false, true, true);
    IFileManager::Get().Delete(
        *(QuickSavePath + TEXT(".bak.tmp")), false, true, true);
    IFileManager::Get().Delete(
        *(QuickSavePath + TEXT(".tmp")), false, true, true);

    FString Feedback;
    FEchoesCampaignProgress LockedProgress;
    TestTrue(TEXT("The locked fixture accepts mission one"),
             LockedProgress.AppendDecision(
                 MakePriorRecord(
                     EEchoesCampaignMissionId::WhatTheLedgerKeeps,
                     echoes::sim::FutureWellChoice::Preserve),
                 Feedback) == EEchoesCampaignCommitStatus::Added);
    TestTrue(TEXT("The locked fixture is stored"),
             FEchoesCampaignProgressStore::SaveAtomic(
                 CampaignPath,
                 LockedProgress,
                 Feedback));
    {
        FTestWorldWrapper LockedWorld;
        if (!LockedWorld.CreateTestWorld(EWorldType::Game))
        {
            LockedWorld.ForwardErrorMessages(this);
            AddError(TEXT("Could not create the locked mission-three world."));
            return false;
        }
        UEchoesSimulationSubsystem* LockedBridge =
            LockedWorld.GetTestWorld()->GetSubsystem<
                UEchoesSimulationSubsystem>();
        TestFalse(TEXT("Mission three rejects a one-record ledger"),
                  LockedBridge != nullptr &&
                      LockedBridge->SelectOperationMode(
                          EEchoesOperationMode::CampaignCityReserve,
                          Feedback));
        TestTrue(TEXT("The locked response names mission two"),
                 Feedback.Contains(TEXT("Seven Accounts of Rain")));
        LockedWorld.ForwardErrorMessages(this);
    }

    FEchoesCampaignProgress InconsistentProgress = LockedProgress;
    TestTrue(TEXT("A mismatch fixture accepts an individually valid mission-two record"),
             InconsistentProgress.AppendDecision(
                 MakePriorRecord(
                     EEchoesCampaignMissionId::SevenAccountsOfRain,
                     echoes::sim::FutureWellChoice::Harvest),
                 Feedback) == EEchoesCampaignCommitStatus::Added);
    TestTrue(TEXT("The mismatch fixture is stored"),
             FEchoesCampaignProgressStore::SaveAtomic(
                 CampaignPath,
                 InconsistentProgress,
                 Feedback));
    {
        FTestWorldWrapper MismatchWorld;
        if (!MismatchWorld.CreateTestWorld(EWorldType::Game))
        {
            MismatchWorld.ForwardErrorMessages(this);
            AddError(TEXT("Could not create the mismatched mission-three world."));
            return false;
        }
        UEchoesSimulationSubsystem* MismatchBridge =
            MismatchWorld.GetTestWorld()->GetSubsystem<
                UEchoesSimulationSubsystem>();
        TestFalse(TEXT("Mission three rejects inconsistent prior choices"),
                  MismatchBridge != nullptr &&
                      MismatchBridge->IsCityReserveUnlocked());
        TestFalse(TEXT("The inconsistent ledger cannot select mission three"),
                  MismatchBridge != nullptr &&
                      MismatchBridge->SelectOperationMode(
                          EEchoesOperationMode::CampaignCityReserve,
                          Feedback));
        MismatchWorld.ForwardErrorMessages(this);
    }

    FEchoesCampaignProgress SeedProgress = LockedProgress;
    TestTrue(TEXT("The fixture accepts the consistent mission-two record"),
             SeedProgress.AppendDecision(
                 MakePriorRecord(
                     EEchoesCampaignMissionId::SevenAccountsOfRain,
                     echoes::sim::FutureWellChoice::Preserve),
                 Feedback) == EEchoesCampaignCommitStatus::Added);
    TestTrue(TEXT("The two-record ledger is stored"),
             FEchoesCampaignProgressStore::SaveAtomic(
                 CampaignPath,
                 SeedProgress,
                 Feedback));

    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the City on Reserve test world."));
        return false;
    }
    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge =
        World != nullptr
            ? World->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (!TestNotNull(TEXT("Mission world owns the simulation subsystem"), Bridge) ||
        !TestTrue(TEXT("Mission three is unlocked by two consistent records"),
                  Bridge != nullptr && Bridge->IsCityReserveUnlocked()) ||
        !TestTrue(TEXT("Mission three operation can be selected"),
                  Bridge->SelectOperationMode(
                      EEchoesOperationMode::CampaignCityReserve,
                      Feedback)) ||
        !TestTrue(TEXT("Mission three scenario starts"),
                  Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    TestTrue(TEXT("Mission three locks Mara's Meridian force"),
             Bridge->GetLocalFaction() ==
                 echoes::sim::Faction::MeridianCompact);
    TestTrue(TEXT("Three authoritative district posts are bound"),
             Bridge->GetCityDistrictId(EEchoesCityDistrict::LifeSupport) != 0 &&
                 Bridge->GetCityDistrictId(EEchoesCityDistrict::Transit) != 0 &&
                 Bridge->GetCityDistrictId(EEchoesCityDistrict::Archive) != 0);
    Bridge->SetScenarioPaused(false);
    Bridge->Tick(0.05f);
    TestTrue(TEXT("The disconnected grid begins at the inherited priority"),
             Bridge->GetCityReservePhase() ==
                 EEchoesCityReservePhase::StabilizePriority);
    TestTrue(TEXT("Mission three uses an isolated quick-save slot"),
             Bridge->QuickSaveScenario(Feedback) &&
                 IFileManager::Get().FileExists(*QuickSavePath));

    TArray<echoes::sim::EntityId> Workers;
    for (const echoes::sim::Entity& Entity :
         Bridge->GetSimulation()->Entities())
    {
        if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            Entity.type == echoes::sim::EntityType::Worker)
        {
            Workers.Add(Entity.id);
        }
    }
    if (!TestEqual(TEXT("The operation has three construction workers"),
                   Workers.Num(), 3))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }

    const echoes::sim::Vec2 RelaySites[] = {
        echoes::sim::Vec2::FromTiles(18, 10),
        echoes::sim::Vec2::FromTiles(10, 18),
        echoes::sim::Vec2::FromTiles(15, 15)};
    for (int32 Index = 0; Index < 3; ++Index)
    {
        TestTrue(
            *FString::Printf(TEXT("Worker %d accepts its district Power Link"),
                             Index + 1),
            Bridge->IssueBuildCommand(
                Workers[Index],
                echoes::sim::EntityType::Dropoff,
                Bridge->SimToWorld(RelaySites[Index]),
                Feedback));
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
    TestTrue(TEXT("Ordinary construction powers all distributed districts"),
             TickUntil(
                 [Bridge]()
                 {
                     return Bridge->GetCityReservePhase() ==
                         EEchoesCityReservePhase::Complete;
                 },
                 3000));

    const FEchoesCampaignDecisionRecord* MissionRecord =
        Bridge->GetCampaignProgress().FindDecision(
            EEchoesCampaignMissionId::ACityOnReserve);
    TestTrue(TEXT("The tick path appends mission three to the ledger"),
             MissionRecord != nullptr &&
                 MissionRecord->WellChoice ==
                     echoes::sim::FutureWellChoice::Preserve &&
                 MissionRecord->AvailableWellChoices == (1 << 1));
    FEchoesCampaignProgress Reloaded;
    TestTrue(TEXT("The three-record campaign reloads transactionally"),
             FEchoesCampaignProgressStore::LoadWithBackup(
                 CampaignPath,
                 Reloaded,
                 Feedback) &&
                 Reloaded.Decisions.Num() == 3);
    Bridge->StopPrototypeScenario();
    TestTrue(
        TEXT("Recorded Mission 03 remains selectable for replay"),
        Bridge->SelectOperationMode(
            EEchoesOperationMode::CampaignCityReserve,
            Feedback) &&
            Bridge->StartPrototypeScenario());
    TestTrue(
        TEXT("A recorded Mission 03 replay can write its prerequisite-bound checkpoint"),
        Bridge->QuickSaveScenario(Feedback));
    TestTrue(
        TEXT("A recorded Mission 03 replay can restore its prerequisite-bound checkpoint"),
        Bridge->QuickLoadScenario(Feedback) &&
            Bridge->GetCityReservePhase() ==
                EEchoesCityReservePhase::StabilizePriority);

    AEchoesPlayerController* Controller =
        World->SpawnActor<AEchoesPlayerController>();
    if (TestNotNull(TEXT("Mission-three result controller can be created"),
                    Controller))
    {
        Controller->NotifyCityReserveFinished(
            true,
            echoes::sim::FutureWellChoice::Preserve,
            echoes::sim::FutureWellChoice::Preserve,
            EEchoesCampaignCommitStatus::AlreadyRecorded);
        TestTrue(TEXT("Mission three has a dedicated successful result"),
                 Controller->IsMatchResultVisible() &&
                     Controller->WasCampaignSuccessful() &&
                     Controller->GetPresentedCampaignOperation() ==
                         EEchoesOperationMode::CampaignCityReserve);
        Controller->ConfirmPrimaryAction();
        TestTrue(TEXT("Mission 03 advances to The Unburied Road briefing"),
                 Bridge->GetOperationMode() ==
                         EEchoesOperationMode::CampaignUnburiedRoad &&
                     Controller->IsMissionBriefingVisible() &&
                     Bridge->GetUnburiedRoadPhase() ==
                         EEchoesUnburiedRoadPhase::EstablishRoadhead &&
                     Bridge->IsScenarioPaused());
        Controller->Destroy();
    }

    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return true;
}

#endif
